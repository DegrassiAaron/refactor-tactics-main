#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/StaticMeshComponent.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "UI/RTHudViewModel.h" // BuildMatchHeader: la vista dell'header si costruisce come la costruisce l'HUD
#include "UI/RTHUD.h" // ComposeControlLabel: la sede unica dei gesti che l'header mostra (D-538)
#include "UI/RTScreenHudWidgets.h" // URTTurnHeaderWidget: la resa delle tavole A e D (#3635)
#include "UI/RTUIPalette.h"
#include "Turn/RTMatchFormatData.h" // FRTMatchRules: limite e soglia del formato
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo per il playback (nomi distinti per file: in unity build i test condividono la translation unit). */
	UWorld* MakePlaybackWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPlaybackWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	/**
	 * Mappa esagonale del mondo di prova. Senza, il turno non risolve NIENTE: il movimento e' fail-closed
	 * senza mappa autorevole, quindi nessuna fase produce animazioni e il playback salta direttamente alla
	 * fine. Entrambi i test di questo file passavano cosi' — uno avvertendo, l'altro perche' «l'unita' ferma
	 * non e' mai corsa» e' vera per definizione quando non corre nessuno (issue #147).
	 *
	 * Raggio 12: contiene le celle usate dagli scenari, (6,6) compresa (distanza 12 dal centro).
	 */
	URTHexMapAsset* SpawnPlaybackMap(UWorld* World, int32 Radius = 12)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnPlaybackUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		// Stessa geometria che il MapActor dichiara (HexSize 100, LayerHeight 250): se il posizionamento e il
		// playback usassero due scale diverse, le unita' si muoverebbero verso celle che non sono le loro.
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Avanza il playback finche' la fase riprodotta non e' quella attesa (o finche' non finisce). */
	bool AdvanceUntilPhase(ARTTurnManager* TM, const FString& PhaseName, int32 MaxSteps = 400)
	{
		for (int32 I = 0; I < MaxSteps && TM->IsResolving(); ++I)
		{
			if (TM->GetPlaybackPhaseName() == PhaseName)
			{
				return true;
			}
			TM->Tick(0.05f);
		}
		return TM->IsResolving() && TM->GetPlaybackPhaseName() == PhaseName;
	}

	void AdvanceUntilDone(ARTTurnManager* TM, int32 MaxSteps = 400)
	{
		for (int32 I = 0; I < MaxSteps && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

/**
 * Il flag di corsa (letto dall'AnimBP) deve valere solo per la fase in cui l'unita' si muove davvero.
 * Regressione osservata in PIE: chi scattava nel Dash restava "in corsa" per tutto il Blast, perche' il flag
 * veniva spento solo a fine risoluzione. Qui si programma il turno invece di guardarlo: piani espliciti,
 * risoluzione, e il playback avanzato a mano con Tick.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackMovingFlagPhaseTest,
	"RefactorTactics.Playback.MovingFlagClearedOnPhaseChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackMovingFlagPhaseTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	SpawnPlaybackMap(World);

	// Due unita' vicine: il Guardian scatta, il Ranger avversario resta fermo e fa da bersaglio.
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	TestNotNull(TEXT("TurnManager spawnato"), TM);
	if (!TM || !Dasher || !Target) { DestroyPlaybackWorld(World); return false; }

	// Piano: scatto verso una cella adiacente + attacco, cosi' la risoluzione ha sia Dash sia Blast.
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	TestTrue(TEXT("il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE);
	if (DashIdx == INDEX_NONE) { DestroyPlaybackWorld(World); return false; }

	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;          // attacco base
	Dasher->PlannedAttackTarget = Target;

	TM->LockInAndResolve();

	// Durante il Dash chi scatta risulta in corsa...
	//
	// Se la fase non viene riprodotta il test FALLISCE, non avverte: un `AddWarning` finisce fra le centinaia
	// di righe della suite e nessuno lo legge, mentre le due asserzioni che seguono — le uniche che coprono il
	// difetto per cui questo test esiste — non verrebbero mai eseguite. Restare verdi senza provare nulla e'
	// peggio che rompersi.
	if (!TestTrue(TEXT("la fase Dash viene riprodotta"), AdvanceUntilPhase(TM, TEXT("Dash"))))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	TestTrue(TEXT("nel Dash l'unita' che scatta e' in corsa"), Dasher->bIsMovingVisually);

	// ...e appena si passa al Blast NON deve piu' esserlo (era il difetto).
	if (!TestTrue(TEXT("la fase Blast viene riprodotta"), AdvanceUntilPhase(TM, TEXT("Blast"))))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	TestFalse(TEXT("nel Blast l'unita' non e' piu' in corsa"), Dasher->bIsMovingVisually);

	// A risoluzione conclusa nessuno resta in corsa.
	AdvanceUntilDone(TM);
	TestFalse(TEXT("a fine risoluzione nessuna corsa residua (attaccante)"), Dasher->bIsMovingVisually);
	TestFalse(TEXT("a fine risoluzione nessuna corsa residua (bersaglio)"), Target->bIsMovingVisually);

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * Un'unita' che non si muove non deve mai risultare in corsa, in nessuna fase: l'AnimBP la terrebbe a correre
 * sul posto pur restando ferma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStillUnitNeverRunsTest,
	"RefactorTactics.Playback.StillUnitNeverRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStillUnitNeverRunsTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	SpawnPlaybackMap(World);

	ARTUnit* Mover = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 2));
	ARTUnit* Still = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(6, 6));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Still) { DestroyPlaybackWorld(World); return false; }

	// Solo Mover ha un piano di movimento; Still resta dov'e'.
	Mover->PlannedCell = FRTCellId(4, 2);
	TM->LockInAndResolve();

	bool bStillEverRan = false;
	bool bMoverEverRan = false;
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		bStillEverRan |= Still->bIsMovingVisually;
		bMoverEverRan |= Mover->bIsMovingVisually;
		TM->Tick(0.05f);
	}
	bStillEverRan |= Still->bIsMovingVisually;

	TestFalse(TEXT("l'unita' ferma non risulta mai in corsa"), bStillEverRan);
	TestFalse(TEXT("nessuna corsa residua a fine risoluzione"), Mover->bIsMovingVisually);

	// Senza questa riga il test e' vacuo: se NESSUNO si muove — perche' il mondo non ha una mappa e il
	// movimento non risolve — «l'unita' ferma non e' mai corsa» e' vera per definizione, e la guardia non
	// guarda niente.
	TestTrue(TEXT("lo scenario ha davvero mosso qualcuno"), bMoverEverRan);
	TestEqual(TEXT("e il movimento e' arrivato dove pianificato"), Mover->Cell.ToString(), FRTCellId(4, 2, 0).ToString());

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * Due rotte di lunghezza diversa nella stessa fase: chi arriva prima smette di correre e torna a riposo, mentre
 * l'altra corre ancora (`#3519`).
 *
 * 🔑 **L'istante si cerca, non si calcola.** Si avanza a passi brevi finche' la rotta corta e' ferma e la lunga si
 * muove ancora: e' esattamente cio' che l'autore ha visto, e non dipende dalla velocita' di playback ne' dalla
 * lunghezza della fase. Senza quell'istante il test fallisce sulla premessa invece di passare a vuoto.
 *
 * ⚠️ **La posa ha un controllo positivo**: il `Lean` del passo `Normal` inclina il corpo di chi si muove e resta
 * al valore finale, quindi «a riposo» discrimina solo se il corpo di chi corre ancora NON lo e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackShorterRouteStopsOnArrivalTest,
	"RefactorTactics.Playback.ShorterRouteStopsRunningOnArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackShorterRouteStopsOnArrivalTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	SpawnPlaybackMap(World);

	// Stessa squadra e righe diverse: nessun bersaglio, nessuna reazione, nessuna cella in comune. Solo due rotte.
	ARTUnit* Long = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 2));
	ARTUnit* Short = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 5));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Long || !Short) { DestroyPlaybackWorld(World); return false; }

	auto Corpo = [](AActor* Actor) -> UStaticMeshComponent*
	{
		TArray<UStaticMeshComponent*> Comps;
		Actor->GetComponents<UStaticMeshComponent>(Comps);
		for (UStaticMeshComponent* C : Comps)
		{
			if (C && C->GetName() == TEXT("Mesh")) { return C; }
		}
		return nullptr;
	};
	UStaticMeshComponent* LongBody = Corpo(Long);
	UStaticMeshComponent* ShortBody = Corpo(Short);
	if (!TestTrue(TEXT("premessa: entrambe le unita' hanno il corpo segnaposto"), LongBody != nullptr && ShortBody != nullptr))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	Long->PlannedCell = FRTCellId(6, 2);  // quattro segmenti
	Short->PlannedCell = FRTCellId(3, 5); // uno
	TM->LockInAndResolve();

	if (!TestTrue(TEXT("la fase Move viene riprodotta"), AdvanceUntilPhase(TM, TEXT("Move"))))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	FVector ShortPrima = Short->GetActorLocation();
	FVector LongPrima = Long->GetActorLocation();
	bool bShortSiEMossa = false;
	bool bIstanteTrovato = false;
	for (int32 I = 0; I < 1000 && TM->IsResolving() && TM->GetPlaybackPhaseName() == TEXT("Move"); ++I)
	{
		TM->Tick(0.02f);
		const FVector ShortOra = Short->GetActorLocation();
		const FVector LongOra = Long->GetActorLocation();
		const bool bShortFerma = ShortOra.Equals(ShortPrima, 0.01);
		const bool bLongInMoto = !LongOra.Equals(LongPrima, 0.01);
		bShortSiEMossa |= !bShortFerma;
		if (bShortSiEMossa && bShortFerma && bLongInMoto)
		{
			bIstanteTrovato = true;
			break;
		}
		ShortPrima = ShortOra;
		LongPrima = LongOra;
	}
	if (!TestTrue(TEXT("premessa: c'e' un istante in cui la rotta corta e' conclusa e la lunga no"), bIstanteTrovato))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	TestFalse(TEXT("arrivata, la rotta corta non corre piu'"), Short->bIsMovingVisually);
	TestTrue(TEXT("arrivata, la sua velocita' visiva e' nulla"), Short->GetVelocity().IsNearlyZero());
	TestTrue(TEXT("arrivata, la sua posa e' a riposo"), ShortBody->GetRelativeRotation().IsNearlyZero());
	TestTrue(TEXT("controllo: la rotta lunga corre ancora"), Long->bIsMovingVisually);
	TestFalse(TEXT("controllo: il corpo di chi corre ancora non e' a riposo"), LongBody->GetRelativeRotation().IsNearlyZero());

	AdvanceUntilDone(TM);
	TestFalse(TEXT("a fine risoluzione nessuna corsa residua"), Long->bIsMovingVisually || Short->bIsMovingVisually);
	TestEqual(TEXT("la rotta lunga e' arrivata dove pianificato"), Long->Cell.ToString(), FRTCellId(6, 2, 0).ToString());
	TestEqual(TEXT("la rotta corta e' arrivata dove pianificato"), Short->Cell.ToString(), FRTCellId(3, 5, 0).ToString());

	DestroyPlaybackWorld(World);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// VELOCITA' DICHIARATA (CP E21.2, `#288`) — l'ingresso che gli AnimBP dei pack Paragon leggono.
//
// Quei grafi scelgono idle/corsa e la direzione del blendspace da `GetVelocity()`. `ARTUnit` non ha un
// movement component — il playback la sposta per interpolazione — quindi `AActor::GetVelocity()` sarebbe
// **sempre zero** e ogni AnimBP agganciato resterebbe fermo in idle senza dire niente.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisualVelocityTest,
	"RefactorTactics.Playback.DeclaredVelocityFollowsTheVisualMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisualVelocityTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	if (!TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	Unit->SetActorLocation(FVector::ZeroVector);

	// Da ferma la velocita' dichiarata e' ZERO, e non «piccola»: e' cio' che tiene l'AnimBP in idle.
	TestTrue(TEXT("ferma: velocita' dichiarata nulla"), Unit->GetVelocity().IsNearlyZero());

	// Uno spostamento di presentazione verso +X. `bIsMovingVisually` lo scrive il TurnManager: qui si
	// riproduce lo stato che il playback produce, senza passare dal playback.
	Unit->bIsMovingVisually = true;
	Unit->SetVisualLocation(FVector(300.f, 0.f, 0.f));

	const FVector Corsa = Unit->GetVelocity();
	TestFalse(TEXT("in movimento: la velocita' non e' nulla"), Corsa.IsNearlyZero());
	TestTrue(TEXT("e punta dove l'unita' si e' spostata (+X)"),
		FVector::DotProduct(Corsa.GetSafeNormal(), FVector::ForwardVector) > 0.99f);
	// `FVector::Size()` e' `double` in UE5 e `VisualRunSpeed` e' `float`: il confronto si scrive con un
	// tipo solo, altrimenti l'overload di `TestEqual` e' ambiguo e non compila.
	TestEqual(TEXT("il modulo e' VisualRunSpeed"),
		static_cast<float>(Corsa.Size()), Unit->VisualRunSpeed, 0.01f);

	// Cambio di direzione: verso +Y. La velocita' deve SEGUIRE lo spostamento, non restare quella di prima.
	Unit->SetVisualLocation(FVector(300.f, 300.f, 0.f));
	TestTrue(TEXT("dopo una svolta punta nella nuova direzione (+Y)"),
		FVector::DotProduct(Unit->GetVelocity().GetSafeNormal(), FVector::RightVector) > 0.99f);

	// ⚠️ **La direzione si aggiorna anche senza facing.** Un'unita' con `bFaceMovementDirection = false` si
	// muove lo stesso, e l'animazione di corsa deve sapere dove sta andando anche quando la mesh non si
	// volta: se il calcolo vivesse dentro il ramo del facing, questo caso resterebbe alla direzione vecchia.
	Unit->bFaceMovementDirection = false;
	Unit->SetVisualLocation(FVector(300.f, 300.f - 300.f, 0.f)); // torna verso -Y
	TestTrue(TEXT("senza facing la direzione segue comunque (-Y)"),
		FVector::DotProduct(Unit->GetVelocity().GetSafeNormal(), -FVector::RightVector) > 0.99f);

	// A fine risoluzione il TurnManager spegne il flag: l'unita' torna a dichiarare zero SENZA muoversi.
	Unit->bIsMovingVisually = false;
	TestTrue(TEXT("flag spento: si torna a zero"), Unit->GetVelocity().IsNearlyZero());

	DestroyPlaybackWorld(World);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// DUE CELLE E DIECI CELLE (`#2370`) — la stessa fase, due percorsi, due momenti di arrivo.
//
// Il test di libreria (`RefactorTactics.Playback.RouteAlphaIsPerRouteNotPerPhase`) misura la formula. Questo
// misura il **playback vero**: piani, risoluzione, `TickPlayback`, e la posizione degli Actor sul mondo.
// Senza, la formula potrebbe essere giusta e non essere chiamata da nessuno.
// ---------------------------------------------------------------------------------------------------------

/**
 * **Il percorso corto arriva prima, e quello lungo prosegue.**
 *
 * 🔴 Il difetto che questo test copre: fino al 2026-09-05 `TickPlayback` dava a ogni animazione della fase
 * lo stesso `Alpha`, normalizzato su `PhaseDur` — che vale il percorso PIU' LUNGO. Poiche'
 * `InterpolateAlongPath` distribuisce `Alpha` sull'intero percorso, 2 celle e 10 celle **arrivavano
 * insieme**: la corta si trascinava a `rate * 2/10`, cioe' cinque volte sotto il rate base dichiarato.
 *
 * ⚠️ **La durata di fase non cambia**, e il test lo verifica: la lunga chiude quando la fase chiude. Cio'
 * che cambia e' che la corta ci arriva prima invece di essere rallentata per aspettarla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackShortRouteArrivesFirstTest,
	"RefactorTactics.Playback.ShortRouteArrivesBeforeLongRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackShortRouteArrivesFirstTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// Raggio 16: la cella (10,4) dista 14 dal centro, e senza mappa sotto i piedi il movimento e'
	// fail-closed — nessuna anim, e il test resterebbe verde senza aver mosso nessuno.
	SpawnPlaybackMap(World, 16);

	ARTUnit* Corta = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
	ARTUnit* Lunga = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 4));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Corta || !Lunga) { DestroyPlaybackWorld(World); return false; }

	// Il budget di movimento non e' l'oggetto di questa misura: si alza per poter chiedere dieci celle.
	Lunga->MoveRange = 12;

	// Righe diverse (`r` = 0 e 4): i due percorsi non si incrociano e nessuno deve deviare.
	Corta->PlannedCell = FRTCellId(2, 0);   // 2 celle
	Lunga->PlannedCell = FRTCellId(10, 4);  // 10 celle

	TM->LockInAndResolve();

	if (!TestTrue(TEXT("la fase Move viene riprodotta"), AdvanceUntilPhase(TM, TEXT("Move"))))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	// Campionamento a passo fisso: per ciascuna si registra il tick in cui si e' mossa l'ULTIMA volta.
	// E' il suo momento di arrivo, e non richiede di conoscere la geometria della mappa dal test.
	const float Dt = 0.05f;
	FVector PrecCorta = Corta->GetActorLocation();
	FVector PrecLunga = Lunga->GetActorLocation();
	int32 ArrivoCorta = -1;
	int32 ArrivoLunga = -1;
	int32 PrimoMovimentoCorta = -1;
	int32 PrimoMovimentoLunga = -1;
	int32 Tick = 0;

	for (; Tick < 600 && TM->IsResolving() && TM->GetPlaybackPhaseName() == TEXT("Move"); ++Tick)
	{
		TM->Tick(Dt);

		const FVector OraCorta = Corta->GetActorLocation();
		const FVector OraLunga = Lunga->GetActorLocation();

		if (!OraCorta.Equals(PrecCorta, 0.01f))
		{
			ArrivoCorta = Tick;
			if (PrimoMovimentoCorta < 0) { PrimoMovimentoCorta = Tick; }
		}
		if (!OraLunga.Equals(PrecLunga, 0.01f))
		{
			ArrivoLunga = Tick;
			if (PrimoMovimentoLunga < 0) { PrimoMovimentoLunga = Tick; }
		}
		PrecCorta = OraCorta;
		PrecLunga = OraLunga;
	}

	// --- ⛔ ANTI-VACUITA': se non si e' mosso nessuno, ogni confronto sotto e' vero per vuoto ------------
	if (!TestTrue(TEXT("l'unita' corta si e' mossa davvero"), ArrivoCorta >= 0)
		|| !TestTrue(TEXT("e anche quella lunga"), ArrivoLunga >= 0))
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	// --- 🔴 L'ASSERZIONE CHE PORTA IL PESO -------------------------------------------------------------
	// Con l'`Alpha` condiviso i due arrivi coincidevano. Il margine e' largo di proposito: 2 celle su 10
	// arrivano intorno al 20% della fase, e chiedere «meno della meta'» non e' sensibile al frame rate.
	TestTrue(FString::Printf(TEXT("⛔ la corta arriva PRIMA della lunga (tick %d < %d)"), ArrivoCorta, ArrivoLunga),
		ArrivoCorta < ArrivoLunga);
	TestTrue(FString::Printf(TEXT("⛔ e con un margine reale, non per un tick (tick %d < %d/2)"), ArrivoCorta, ArrivoLunga),
		static_cast<float>(ArrivoCorta) < static_cast<float>(ArrivoLunga) * 0.5f);

	// --- Scenario M2: concorrenti, non serializzate ----------------------------------------------------
	// ⚠️ Nessuna A-poi-B indotta dall'ordine di `MoveAnims` o da `StableUnitId`: partono insieme.
	TestEqual(TEXT("le due unita' partono nello stesso tick"), PrimoMovimentoCorta, PrimoMovimentoLunga);

	// --- La fase non si e' accorciata: la lunga chiude quando chiude la fase ---------------------------
	// ⚠️ Questa NON discrimina il difetto — anche con l'`Alpha` condiviso la lunga arrivava a fine fase.
	// Sta qui per l'altra meta' dell'invariante: il fix accorcia l'attesa della corta, non la fase.
	TestTrue(FString::Printf(TEXT("la lunga si muove fin quasi a fine fase (arrivo %d, tick %d)"), ArrivoLunga, Tick),
		static_cast<float>(ArrivoLunga) > static_cast<float>(Tick) * 0.8f);

	// --- Lo stato logico e' quello pianificato: la presentazione non ha deciso niente ------------------
	AdvanceUntilDone(TM);
	TestEqual(TEXT("la corta e' arrivata dove pianificato"),
		Corta->Cell.ToString(), FRTCellId(2, 0, 0).ToString());
	TestEqual(TEXT("e la lunga anche"),
		Lunga->Cell.ToString(), FRTCellId(10, 4, 0).ToString());

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * **L'header porta la fase RIPRODOTTA, non quella logica** ([D-479], #3612).
 *
 * 🔑 Il caso che il test esiste per prendere e' plausibile: `FRTMatchHeaderView::PlaybackPhase` riempito con
 * `Phase`. Senza sospensioni `RunPhaseLoop` risolve tutte le fasi prima del playback e riporta `Phase` a
 * `Planning`, quindi un header cosi' direbbe PIANIFICAZIONE per tutta la riproduzione, e nessuna cella si
 * accenderebbe. Qui il turno e' vero (scatto, attacco e una rotta, cioe' Dash, Blast e Move), e la vista si
 * costruisce a ogni tick del playback con `BuildMatchHeader`, come la costruisce l'HUD.
 *
 * ⚠️ **Prep non e' nel turno**: la accende solo un'azione predittiva armata, e la fixture ne farebbe un altro
 * test. La copre il confronto con `GetPlaybackPhase()` a ogni tick, che vale per qualunque fase riprodotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHeaderPlaybackPhaseTest,
	"RefactorTactics.HudViewModel.MatchHeaderCarriesThePlayedPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHeaderPlaybackPhaseTest::RunTest(const FString&)
{
	const auto Nome = [](const ERTMatchPhase Fase)
	{
		return StaticEnum<ERTMatchPhase>()->GetNameStringByValue(static_cast<int64>(Fase));
	};

	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	SpawnPlaybackMap(World);

	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("TurnManager spawnato"), TM) || !Dasher || !Target) { DestroyPlaybackWorld(World); return false; }

	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;          // attacco base
	Dasher->PlannedAttackTarget = Target;
	Target->PlannedCell = FRTCellId(4, 5); // la sua rotta si riproduce in Move, dopo il Blast che la colpisce

	TestEqual(TEXT("prima del turno: nessuna fase riprodotta"),
		Nome(URTHudViewModel::BuildMatchHeader(TM).PlaybackPhase), Nome(ERTMatchPhase::Planning));

	TM->LockInAndResolve();

	TSet<ERTMatchPhase> Mostrate;
	int32 Tick = 0;
	int32 DiversaDallaSorgente = 0;  // il campo dice altro da `GetPlaybackPhase()`
	int32 Cleanup = 0;               // `PlaybackPhases` non contiene mai `Cleanup`
	int32 DiversaDallaLogica = 0;    // la distinzione che D-479 punto 2 misura
	for (; Tick < 600 && TM->IsResolving(); ++Tick)
	{
		const FRTMatchHeaderView Header = URTHudViewModel::BuildMatchHeader(TM);
		Mostrate.Add(Header.PlaybackPhase);
		DiversaDallaSorgente += (Header.PlaybackPhase != TM->GetPlaybackPhase()) ? 1 : 0;
		Cleanup += (Header.PlaybackPhase == ERTMatchPhase::Cleanup) ? 1 : 0;
		DiversaDallaLogica += (Header.PlaybackPhase != Header.Phase) ? 1 : 0;
		TM->Tick(0.05f);
	}

	if (!TestFalse(TEXT("premessa: il playback e' finito entro il tetto di tick"), TM->IsResolving()))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	TestEqual(TEXT("a ogni tick il campo e' la fase che il playback mostra"), DiversaDallaSorgente, 0);
	TestEqual(TEXT("il campo non vale mai Cleanup"), Cleanup, 0);
	TestTrue(TEXT("l'header ha mostrato il Dash"), Mostrate.Contains(ERTMatchPhase::Dash));
	TestTrue(TEXT("l'header ha mostrato il Blast"), Mostrate.Contains(ERTMatchPhase::Blast));
	TestTrue(TEXT("l'header ha mostrato il Move"), Mostrate.Contains(ERTMatchPhase::Move));
	TestTrue(*FString::Printf(TEXT("in %d tick la fase riprodotta e' stata almeno una volta diversa da quella logica"), Tick),
		DiversaDallaLogica > 0);
	TestEqual(TEXT("dopo il playback: nessuna fase riprodotta"),
		Nome(URTHudViewModel::BuildMatchHeader(TM).PlaybackPhase), Nome(ERTMatchPhase::Planning));

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * 🔑 **L'HEADER COME LE TAVOLE A E D** (#3635, T8). Il banco e' quello di `MatchHeaderCarriesThePlayedPhase`: un
 * turno vero con Dash, Blast e Move, riprodotto tick per tick. Il nome resta `ScreenHud.*` perche' e' il widget a
 * essere sotto misura.
 *
 * Le cinque cose della DoD:
 *  1. in Risoluzione la cella attiva e' la fase RIPRODOTTA, e le altre sono spente;
 *  2. in Pianificazione c'e' il gruppo della fase logica, con la parola in maiuscolo;
 *  3. il timer e' vuoto quando non si applica, e `00:22` quando restano 21,2 s;
 *  4. il limite del round e' assente senza formato, e «/ 12» con;
 *  5. l'obiettivo e' assente senza una cella obiettivo, e «2–1 (a 3)» con.
 *
 * ⚠️ **I testi si confrontano CaseSensitive**: `TestEqual` su `FString` ignora le maiuscole, e «PIANIFICAZIONE»
 * contro «Pianificazione» e' proprio cio' che va distinto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnHeaderLookFollowsTheBoardTest,
	"RefactorTactics.ScreenHud.TurnHeaderLookFollowsTheBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnHeaderLookFollowsTheBoardTest::RunTest(const FString&)
{
	const auto Uguale = [](const FText& Testo, const TCHAR* Atteso)
	{
		return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive);
	};
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };
	const auto Colore = [](ERTUIToken Token) { return URTUIPalette::ColorFor(Token); };

	// --- 3a. Il formato del timer, senza mondo ----------------------------------------------------------------------
	TestTrue(TEXT("3: 21,2 s si leggono 00:22 (per eccesso)"), Uguale(URTTurnHeaderWidget::FormatCommitTimer(21.2f), TEXT("00:22")));
	TestTrue(TEXT("3: 65 s si leggono 01:05"), Uguale(URTTurnHeaderWidget::FormatCommitTimer(65.f), TEXT("01:05")));
	TestTrue(TEXT("3: zero non si mostra"), URTTurnHeaderWidget::FormatCommitTimer(0.f).IsEmpty());
	TestTrue(TEXT("3: un negativo non si mostra"), URTTurnHeaderWidget::FormatCommitTimer(-1.f).IsEmpty());

	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	URTHexMapAsset* Mappa = SpawnPlaybackMap(World);

	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTTurnHeaderWidget* W = NewObject<URTTurnHeaderWidget>(World);
	if (!TestNotNull(TEXT("TurnManager spawnato"), TM) || !Dasher || !Target || !Mappa || !W)
	{
		DestroyPlaybackWorld(World);
		return false;
	}

	const auto Cornice = [W]()
	{
		UBorder* B = NewObject<UBorder>(W);
		FSlateBrush Brush = B->Background;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		B->SetBrush(Brush);
		return B;
	};
	W->HeaderFrame = Cornice();
	W->RoundValueText = NewObject<UTextBlock>(W);
	W->RoundLimitText = NewObject<UTextBlock>(W);
	W->PlanningGroup = NewObject<UBorder>(W);
	W->ResolutionGroup = NewObject<UBorder>(W);
	W->PhaseText = NewObject<UTextBlock>(W);
	W->PhaseMark = Cornice();
	W->TimerText = NewObject<UTextBlock>(W);
	W->ObjectiveBox = NewObject<UBorder>(W);
	W->ObjectiveScoreText = NewObject<UTextBlock>(W);
	W->ObjectiveThresholdText = NewObject<UTextBlock>(W);
	struct FCellaDiProva { ERTMatchPhase Fase; ERTUIToken Colore; UBorder* Cornice; UBorder* Marca; UTextBlock* Sigla; };
	TArray<FCellaDiProva> Celle;
	const auto Cella = [&](ERTMatchPhase Fase, ERTUIToken Token, TObjectPtr<UBorder>& C, TObjectPtr<UBorder>& M, TObjectPtr<UTextBlock>& S)
	{
		C = Cornice();
		M = Cornice();
		S = NewObject<UTextBlock>(W);
		Celle.Add({ Fase, Token, C.Get(), M.Get(), S.Get() });
	};
	Cella(ERTMatchPhase::Prep, ERTUIToken::Phase_Prep, W->PhaseCell_Prep, W->PhaseCellMark_Prep, W->PhaseCellText_Prep);
	Cella(ERTMatchPhase::Dash, ERTUIToken::Phase_Dash, W->PhaseCell_Dash, W->PhaseCellMark_Dash, W->PhaseCellText_Dash);
	Cella(ERTMatchPhase::Blast, ERTUIToken::Phase_Blast, W->PhaseCell_Blast, W->PhaseCellMark_Blast, W->PhaseCellText_Blast);
	Cella(ERTMatchPhase::Move, ERTUIToken::Phase_Move, W->PhaseCell_Move, W->PhaseCellMark_Move, W->PhaseCellText_Move);
	W->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// --- A. Pianificazione, senza formato e senza obiettivo ------------------------------------------------------------
	W->RefreshLook();
	TestTrue(TEXT("2: in Pianificazione c'e' il gruppo della fase logica"), Visibile(W->PlanningGroup));
	TestFalse(TEXT("2: e non quello della Risoluzione"), Visibile(W->ResolutionGroup));
	TestTrue(TEXT("2: la parola e' in maiuscolo"), Uguale(W->PhaseText->GetText(), TEXT("PIANIFICAZIONE")));
	TestTrue(TEXT("2: in Cyan"), W->PhaseText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Cyan)));
	TestTrue(TEXT("3: senza timer armato il timer e' vuoto"), W->TimerText->GetText().IsEmpty());
	TestFalse(TEXT("4: senza formato il limite non c'e'"), Visibile(W->RoundLimitText));
	TestFalse(TEXT("5: senza cella obiettivo l'obiettivo non c'e'"), Visibile(W->ObjectiveBox));
	TestTrue(TEXT("A: il riquadro ha il contorno Frame_Deep"),
		W->HeaderFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Frame_Deep)));
	for (const FCellaDiProva& C : Celle)
	{
		TestEqual(TEXT("A: fuori dalla Risoluzione nessuna cella e' attiva"), C.Cornice->Background.OutlineSettings.Width, 1.f);
	}

	// --- B. Con formato e obiettivo ------------------------------------------------------------------------------------
	FRTMatchRules Regole = TM->GetMatchRules();
	Regole.RoundLimit = 12;
	Regole.ScoreToWin = 3;
	TM->SetMatchRules(Regole);
	if (!TestTrue(TEXT("premessa: la mappa ha celle"), Mappa->Cells.Num() > 0))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Mappa->Cells[0].bIsObjective = true;
	// Un punteggio asimmetrico: con 0–0 uno scambio delle due squadre resterebbe verde (review di #3635).
	TM->AddTeamScore(0, 2);
	TM->AddTeamScore(1, 1);
	// Un timer vero: il widget deve mostrare il commit, `00:30`, e non solo saperlo formattare.
	TM->SetPlanningSeconds(30.f);
	W->RefreshLook(/*bRecheckObjective=*/ true);
	TestTrue(TEXT("3: con un Planning di 30 s armato il timer dice 00:30"), Uguale(W->TimerText->GetText(), TEXT("00:30")));
	TestTrue(TEXT("3: e il getter dice lo stesso"), Uguale(W->GetTimerText(), TEXT("00:30")));
	TM->SetPlanningSeconds(0.f);
	W->RefreshLook(/*bRecheckObjective=*/ false);
	TestTrue(TEXT("3: disarmato, il timer torna vuoto"), W->TimerText->GetText().IsEmpty());
	TestTrue(TEXT("4: col formato il limite c'e'"), Visibile(W->RoundLimitText));
	TestTrue(TEXT("4: e dice «/ 12»"), Uguale(W->RoundLimitText->GetText(), TEXT("/ 12")));
	TestTrue(TEXT("5: con la cella obiettivo l'obiettivo c'e'"), Visibile(W->ObjectiveBox));
	TestTrue(TEXT("5: la soglia c'e' e dice «(a 3)»"),
		Visibile(W->ObjectiveThresholdText) && Uguale(W->ObjectiveThresholdText->GetText(), TEXT("(a 3)")));
	TestTrue(TEXT("5: il punteggio e' squadra 0 – squadra 1, «2–1»"), Uguale(W->ObjectiveScoreText->GetText(), TEXT("2–1")));

	// --- C. Risoluzione: la cella attiva e' la fase RIPRODOTTA -------------------------------------------------------------
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;
	Dasher->PlannedAttackTarget = Target;
	Target->PlannedCell = FRTCellId(4, 5);
	TM->LockInAndResolve();

	TSet<ERTMatchPhase> Viste;
	int32 TickRisoluzione = 0;
	int32 Sbagliate = 0;
	for (int32 Tick = 0; Tick < 600 && TM->IsResolving(); ++Tick)
	{
		W->RefreshLook(/*bRecheckObjective=*/ false);
		const ERTMatchPhase Riprodotta = URTHudViewModel::BuildMatchHeader(TM).PlaybackPhase;
		++TickRisoluzione;
		Viste.Add(Riprodotta);
		Sbagliate += (!Visibile(W->ResolutionGroup) || Visibile(W->PlanningGroup)) ? 1 : 0;
		// L'obiettivo si vede ANCHE durante la riproduzione, ed e' il momento in cui conta di piu': un obiettivo conteso
		// cambia mano proprio li'. Vale perche' il riquadro dipende dalla mappa e non dalla fase (D-539).
		Sbagliate += !Visibile(W->ObjectiveBox) ? 1 : 0;
		for (const FCellaDiProva& C : Celle)
		{
			const bool bAttesa = C.Fase == Riprodotta;
			const FLinearColor ColoreFase = Colore(C.Colore);
			const bool bAttiva = C.Cornice->Background.OutlineSettings.Width == 2.f
				&& C.Cornice->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(ColoreFase)
				&& C.Cornice->GetBrushColor().Equals(ColoreFase.CopyWithNewOpacity(0.2f))
				&& C.Sigla->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::White))
				&& C.Marca->GetBrushColor().Equals(ColoreFase)
				&& C.Marca->Background.OutlineSettings.RoundingType == ESlateBrushRoundingType::FixedRadius;
			const bool bSpenta = C.Cornice->Background.OutlineSettings.Width == 1.f
				&& C.Cornice->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Frame_Off))
				&& C.Cornice->GetBrushColor().A == 0.f
				&& C.Sigla->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Disabled))
				&& C.Marca->Background.OutlineSettings.RoundingType == ESlateBrushRoundingType::HalfHeightRadius;
			Sbagliate += (bAttesa ? !bAttiva : !bSpenta) ? 1 : 0;
		}
		TM->Tick(0.05f);
	}
	TestFalse(TEXT("premessa: il playback e' finito entro il tetto di tick"), TM->IsResolving());
	TestTrue(TEXT("premessa: ci sono stati tick di Risoluzione"), TickRisoluzione > 0);
	TestTrue(TEXT("1: l'header ha acceso il Dash"), Viste.Contains(ERTMatchPhase::Dash));
	TestTrue(TEXT("1: e il Blast"), Viste.Contains(ERTMatchPhase::Blast));
	TestTrue(TEXT("1: e il Move"), Viste.Contains(ERTMatchPhase::Move));
	TestEqual(*FString::Printf(TEXT("1: in %d tick, a ogni tick la sola cella attiva e' la fase riprodotta, i gruppi sono giusti e l'obiettivo resta visibile"),
		TickRisoluzione), Sbagliate, 0);

	// --- D. Dopo il playback si torna alla Pianificazione ---------------------------------------------------------------
	W->RefreshLook(/*bRecheckObjective=*/ false);
	TestTrue(TEXT("D: dopo il playback torna il gruppo della Pianificazione"), Visibile(W->PlanningGroup));
	TestFalse(TEXT("D: e il gruppo della Risoluzione si chiude"), Visibile(W->ResolutionGroup));

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * IL SUGGERIMENTO DEI GESTI NELL'HEADER (D-538, #2764). `Spazio`, `V`, `RMB` e `P` funzionavano e nessun widget lo
 * diceva: i testi vivevano solo dentro una riga di stato senza piu' chiamanti. Il banco e' quello di
 * `TurnHeaderLookFollowsTheBoard`: un turno vero con Dash, Blast e Move, riprodotto tick per tick.
 *
 * 🔴 **Un tasto si nomina solo se risponde** (revisione indipendente di #3847). In questo banco il giocatore e' la
 * squadra 0 e comanda un'unita' VIVA e non bot: e' una partita presidiata, quindi `V` e' spenta (D-350) e `Spazio`
 * e `RMB` sono vivi. Il resto del test rovescia il quadro.
 *
 * Le cose misurate:
 *  1. senza contesto di partita l'etichetta e' vuota;
 *  2. in Pianificazione PRESIDIATA resta la sola velocita' SENZA il tasto: `Velocit\u00e0: x1`;
 *  3. la scelta di chi guarda si legge (`x4`), anche fuori dalla Risoluzione;
 *  4. in Risoluzione, A OGNI TICK, il testo del widget e' quello del compositore con i gesti di una partita
 *     presidiata, nomina `(Spazio: salta)` e NON `(V)`;
 *  5. a playback finito il gesto di salto sparisce e resta la velocita';
 *  6. in sessione NON presidiata il quadro si rovescia: `V` si nomina, `Spazio` e `RMB` no;
 *  7. chi osserva (squadra senza unita' vive, o con tutte le unita' cadute) riavrebbe `V`, e senza manager niente
 *     e' vivo.
 *
 * Il confronto del punto 4 e' per UGUAGLIANZA col compositore, ma con i gesti SCRITTI A MANO nel test: se la lettura
 * dei gesti dicesse una cosa diversa dalla regola dei tasti, il widget e il compositore concorderebbero fra loro e
 * il test passerebbe lo stesso. I testi si confrontano CaseSensitive.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnHeaderControlHintTest,
	"RefactorTactics.ScreenHud.TurnHeaderShowsWhatTheKeysDoNow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnHeaderControlHintTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	URTHexMapAsset* Mappa = SpawnPlaybackMap(World);

	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTTurnHeaderWidget* W = NewObject<URTTurnHeaderWidget>(World);
	if (!TestNotNull(TEXT("TurnManager spawnato"), TM) || !Dasher || !Target || !Mappa || !W)
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	W->ControlHintText = NewObject<UTextBlock>(W);

	const auto Scritto = [W]() { return W->ControlHintText->GetText().ToString(); };
	const auto Uguale = [](const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); };

	// I gesti di una partita PRESIDIATA, scritti a mano: Spazio e RMB vivi, V spenta perche' chi gioca comanda Dasher.
	FRTLiveGestures Presidiata;
	Presidiata.bSkip = true;
	Presidiata.bUnready = true;
	Presidiata.bSpeed = false;
	const auto Composto = [TM, &Presidiata]()
	{
		return ARTHUD::ComposeControlLabel(URTHudViewModel::BuildMatchHeader(TM), TM->GetPlaybackProgress01(),
			TM->ViewerPlaybackSpeed, Presidiata);
	};

	// --- 1. Senza contesto di partita ------------------------------------------------------------------------------
	W->RefreshLook(/*bRecheckObjective=*/ false);
	TestTrue(TEXT("1: senza contesto l'etichetta e' vuota"), Scritto().IsEmpty());
	TestTrue(TEXT("1: e il getter dice lo stesso"), W->GetControlHintText().IsEmpty());

	W->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// --- 2. Pianificazione presidiata: la sola velocita', SENZA il tasto ---------------------------------------------
	TM->ViewerPlaybackSpeed = 1.f;
	W->RefreshLook(false);
	TestTrue(TEXT("2: in Pianificazione presidiata la riga nomina la velocita' e NON il tasto, che e' spento"),
		Uguale(Scritto(), TEXT("Velocit\u00e0: x1")));
	TestTrue(TEXT("2: il getter dice lo stesso del testo scritto"), Uguale(W->GetControlHintText().ToString(), Scritto()));

	// --- 3. La scelta di chi guarda si legge ------------------------------------------------------------------------
	TM->ViewerPlaybackSpeed = 4.f;
	W->RefreshLook(false);
	TestTrue(TEXT("3: la velocita' scelta, x4, si legge anche in Pianificazione"),
		Uguale(Scritto(), TEXT("Velocit\u00e0: x4")));

	// --- 4. Risoluzione: a ogni tick il widget dice cio' che dice il compositore -----------------------------------
	TM->ViewerPlaybackSpeed = 2.f;
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;
	Dasher->PlannedAttackTarget = Target;
	Target->PlannedCell = FRTCellId(4, 5);
	TM->LockInAndResolve();

	int32 TickRisoluzione = 0;
	int32 DiversoDalCompositore = 0;
	int32 SenzaGestoDiSalto = 0;
	int32 ConTastoV = 0;
	FString PrimaRiga;
	for (int32 Tick = 0; Tick < 600 && TM->IsResolving(); ++Tick)
	{
		W->RefreshLook(/*bRecheckObjective=*/ false);
		const FString Atteso = Composto();
		if (TickRisoluzione == 0)
		{
			PrimaRiga = Scritto();
		}
		++TickRisoluzione;
		DiversoDalCompositore += Uguale(Scritto(), Atteso) ? 0 : 1;
		DiversoDalCompositore += Uguale(W->GetControlHintText().ToString(), Atteso) ? 0 : 1;
		SenzaGestoDiSalto += Scritto().Contains(TEXT("(Spazio: salta)"), ESearchCase::CaseSensitive) ? 0 : 1;
		ConTastoV += Scritto().Contains(TEXT("(V)"), ESearchCase::CaseSensitive) ? 1 : 0;
		TM->Tick(0.05f);
	}
	TestFalse(TEXT("premessa: il playback e' finito entro il tetto di tick"), TM->IsResolving());
	TestTrue(TEXT("premessa: ci sono stati tick di Risoluzione"), TickRisoluzione > 0);
	TestEqual(*FString::Printf(TEXT("4: in %d tick il testo del widget e' sempre quello del compositore"), TickRisoluzione),
		DiversoDalCompositore, 0);
	TestEqual(TEXT("4: e a ogni tick nomina il gesto per saltare, che in una partita presidiata e' vivo"), SenzaGestoDiSalto, 0);
	TestEqual(TEXT("4: e a nessun tick promette V, che da presidiati e' spenta"), ConTastoV, 0);
	TestTrue(*FString::Printf(TEXT("4: e chiude con la velocita' senza tasto (\"%s\")"), *PrimaRiga),
		PrimaRiga.EndsWith(TEXT("Velocit\u00e0: x2"), ESearchCase::CaseSensitive));

	// --- 5. Dopo il playback il salto sparisce, la velocita' resta ----------------------------------------------------
	W->RefreshLook(false);
	TestFalse(TEXT("5: a playback finito il gesto di salto non c'e' piu'"),
		Scritto().Contains(TEXT("(Spazio: salta)"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("5: ma la velocita' resta nominata"), Scritto().Contains(TEXT("Velocit\u00e0: x2"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("5: ed e' ancora quella del compositore"), Uguale(Scritto(), Composto()));

	// --- 6. Sessione NON presidiata: il quadro si rovescia ---------------------------------------------------------
	// Come fa il bootstrapper con `Config.bAutobattle`: il TurnManager viene informato, e tutte le unita' sono bot.
	TM->SetUnattendedSession(true);
	Dasher->bIsBotControlled = true;
	Target->bIsBotControlled = true;
	W->RefreshLook(false);
	TestTrue(TEXT("6: in autobattle la velocita' si nomina COL tasto: la manopola e' viva"),
		Uguale(Scritto(), TEXT("Velocit\u00e0: x2 (V)")));

	const TArray<ARTUnit*> Unita = { Dasher, Target };
	const FRTLiveGestures Autobattle = URTHudViewModel::ReadLiveGestures(TM, /*PlayerTeamId=*/ 0, Unita);
	TestFalse(TEXT("6: in autobattle Spazio e' inerte (OnLockIn esce su IsPlanningInputInert)"), Autobattle.bSkip);
	TestFalse(TEXT("6: ne' RMB, perche' non c'e' un umano che possa disdire"), Autobattle.bUnready);
	TestTrue(TEXT("6: ma V e' viva"), Autobattle.bSpeed);

	FRTMatchHeaderView InRiproduzione = URTHudViewModel::BuildMatchHeader(TM);
	InRiproduzione.bResolving = true;
	TestFalse(TEXT("6: e il suggerimento in riproduzione non nomina Spazio"),
		ARTHUD::ComposeControlHint(InRiproduzione, 0.456f, Autobattle).Contains(TEXT("Spazio"), ESearchCase::CaseSensitive));

	// --- 7. Chi osserva, e chi non ha un manager -------------------------------------------------------------------
	TM->SetUnattendedSession(false);
	Dasher->bIsBotControlled = false;
	Target->bIsBotControlled = false;
	const FRTLiveGestures Giocatore = URTHudViewModel::ReadLiveGestures(TM, /*PlayerTeamId=*/ 0, Unita);
	TestTrue(TEXT("7: partita presidiata: Spazio e RMB vivi"), Giocatore.bSkip && Giocatore.bUnready);
	TestFalse(TEXT("7: e V spenta, perche' la squadra 0 comanda un'unita' viva (D-350)"), Giocatore.bSpeed);

	const FRTLiveGestures Osservatore = URTHudViewModel::ReadLiveGestures(TM, /*PlayerTeamId=*/ 2, Unita);
	TestTrue(TEXT("7: una squadra senza unita' osserva: la manopola e' sua"), Osservatore.bSpeed);

	Dasher->Health = 0;
	const FRTLiveGestures SenzaUnita = URTHudViewModel::ReadLiveGestures(TM, /*PlayerTeamId=*/ 0, Unita);
	TestTrue(TEXT("7: chi ha perso tutte le unita' sta osservando, e la manopola torna sua"), SenzaUnita.bSpeed);

	const FRTLiveGestures SenzaManager = URTHudViewModel::ReadLiveGestures(nullptr, 0, Unita);
	TestFalse(TEXT("7: senza manager nessun tasto si nomina"),
		SenzaManager.bSkip || SenzaManager.bUnready || SenzaManager.bSpeed);

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * LE RETICENZE DELL'OBIETTIVO NELL'HEADER (D-539). Il progresso sull'obiettivo e' l'esempio perfetto di regola che un
 * controllo a schermo non vede: l'ASSENZA di un pezzo di testo non somiglia a un difetto. Le tre reticenze vivevano solo
 * dentro `ARTHUD::ComposeMatchStatusLine` (ritirata con D-539); qui si pinnano sul widget che le applica davvero (`RefreshLook`).
 *
 *  1. senza una cella obiettivo l'obiettivo NON c'e', anche con soglia e punteggio: su una mappa senza obiettivo `2-1`
 *     (o `0-0`) sarebbe un punteggio inventato per una gara che nessuno sta correndo;
 *  2. con l'obiettivo ma senza soglia (`ScoreToWin == 0`, la via per obiettivo e' disattivata in v0.1) il progresso si
 *     vede e il «(a N)» tace: «a 0» leggerebbe come «gia' vinta»;
 *  3. `0-0` CON obiettivo dichiarato si mostra: e' il punteggio vero di una gara in parita' al calcio d'inizio. Una
 *     guardia `(Team0Score > 0 || Team1Score > 0)` farebbe sparire la riga proprio all'inizio;
 *  4. con soglia e punteggio il widget dice squadra 0 - squadra 1 e il traguardo, e togliendo la cella torna il silenzio.
 *
 * I confronti sono CaseSensitive: `TestEqual` su `FString` ignora le maiuscole.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnHeaderObjectiveReticencesTest,
	"RefactorTactics.ScreenHud.TurnHeaderObjectiveReticences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnHeaderObjectiveReticencesTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	URTHexMapAsset* Mappa = SpawnPlaybackMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTTurnHeaderWidget* W = NewObject<URTTurnHeaderWidget>(World);
	if (!TestNotNull(TEXT("TurnManager spawnato"), TM) || !Mappa || !W || !TestTrue(TEXT("premessa: la mappa ha celle"), Mappa->Cells.Num() > 0))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	W->ObjectiveBox = NewObject<UBorder>(W);
	W->ObjectiveScoreText = NewObject<UTextBlock>(W);
	W->ObjectiveThresholdText = NewObject<UTextBlock>(W);
	W->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	const auto Visibile = [](const UWidget* Widget) { return Widget && Widget->GetVisibility() != ESlateVisibility::Collapsed; };
	const auto Uguale = [](const FText& Testo, const TCHAR* Atteso) { return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive); };
	const auto ImpostaSoglia = [TM](int32 ScoreToWin)
	{
		FRTMatchRules Regole = TM->GetMatchRules();
		Regole.ScoreToWin = ScoreToWin;
		TM->SetMatchRules(Regole);
	};

	// --- 1. Nessuna cella obiettivo: silenzio, anche con soglia dichiarata ----------------------------------------------
	ImpostaSoglia(3);
	TM->AddTeamScore(0, 2);
	TM->AddTeamScore(1, 1);
	Mappa->Cells[0].bIsObjective = false;
	W->RefreshLook(/*bRecheckObjective=*/ true);
	TestFalse(TEXT("1: senza cella obiettivo il riquadro non c'e', anche con soglia e punteggio 2-1"), Visibile(W->ObjectiveBox));

	// --- 2. Con la cella ma SENZA soglia: il progresso si vede, il traguardo tace ------------------------------------------
	Mappa->Cells[0].bIsObjective = true;
	ImpostaSoglia(0);
	W->RefreshLook(true);
	TestTrue(TEXT("2: con la cella obiettivo il riquadro c'e'"), Visibile(W->ObjectiveBox));
	TestTrue(TEXT("2: il progresso e' squadra 0 - squadra 1"), Uguale(W->ObjectiveScoreText->GetText(), TEXT("2–1")));
	TestFalse(TEXT("2: ma senza soglia il «(a N)» tace: «a 0» leggerebbe come «gia' vinta»"), Visibile(W->ObjectiveThresholdText));

	// --- 3. Con la soglia: anche il traguardo ----------------------------------------------------------------------------
	ImpostaSoglia(3);
	W->RefreshLook(false);
	TestTrue(TEXT("3: con la soglia dichiarata il traguardo si vede"), Visibile(W->ObjectiveThresholdText));
	TestTrue(TEXT("3: e dice «(a 3)»"), Uguale(W->ObjectiveThresholdText->GetText(), TEXT("(a 3)")));

	// --- 4. La soglia SENZA obiettivo tace anche lei ---------------------------------------------------------------------
	Mappa->Cells[0].bIsObjective = false;
	W->RefreshLook(true);
	TestFalse(TEXT("4: tolta la cella torna il silenzio, e con lui il traguardo"), Visibile(W->ObjectiveBox));
	// Il traguardo tace PER CONTO SUO: nel test `ObjectiveThresholdText` e' un widget libero, non sta dentro `ObjectiveBox`,
	// quindi il silenzio non puo' venire dal contenitore. La soglia vale ancora 3.
	TestFalse(TEXT("4: e il traguardo (a 3) tace anche da solo, senza dipendere da dove l'asset lo monta"),
		Visibile(W->ObjectiveThresholdText));

	DestroyPlaybackWorld(World);

	// --- 5. 0-0 CON obiettivo si mostra: mondo nuovo, perche' il punteggio non scende ---------------------------------------
	UWorld* World2 = MakePlaybackWorld();
	if (!TestNotNull(TEXT("secondo World creato"), World2)) { return false; }
	URTHexMapAsset* Mappa2 = SpawnPlaybackMap(World2);
	ARTTurnManager* TM2 = World2->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTTurnHeaderWidget* W2 = NewObject<URTTurnHeaderWidget>(World2);
	if (!TestNotNull(TEXT("secondo TurnManager"), TM2) || !Mappa2 || !W2 || Mappa2->Cells.Num() == 0)
	{
		DestroyPlaybackWorld(World2);
		return false;
	}
	W2->ObjectiveBox = NewObject<UBorder>(W2);
	W2->ObjectiveScoreText = NewObject<UTextBlock>(W2);
	W2->ObjectiveThresholdText = NewObject<UTextBlock>(W2);
	W2->SetMatchContextForTest(TM2, 0);
	Mappa2->Cells[0].bIsObjective = true;
	W2->RefreshLook(true);
	TestTrue(TEXT("5: con l'obiettivo dichiarato il riquadro c'e' anche a 0-0"), Visibile(W2->ObjectiveBox));
	TestTrue(TEXT("5: e dice «0–0», che e' un punteggio vero e non un'assenza"),
		Uguale(W2->ObjectiveScoreText->GetText(), TEXT("0–0")));

	DestroyPlaybackWorld(World2);
	return true;
}

/**
 * 🔑 **LA RIGA DEL REGISTRO COME LA TAVOLA D** (#3637, T8): striscia e sigla della fase, «#n», chip `REAZ.`, frase.
 * ⚠️ Confronti CaseSensitive: `TestEqual` su `FString` ignora le maiuscole.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLineLookTest,
	"RefactorTactics.ScreenHud.EventLineShowsPhaseNumberAndReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEventLineLookTest::RunTest(const FString&)
{
	URTPlayerEventLineWidget* Riga = NewObject<URTPlayerEventLineWidget>();
	if (!TestNotNull(TEXT("riga"), Riga)) { return false; }
	Riga->PhaseStrip = NewObject<UBorder>(Riga);
	Riga->PhaseLabelText = NewObject<UTextBlock>(Riga);
	Riga->SequenceText = NewObject<UTextBlock>(Riga);
	Riga->ReactionBadge = NewObject<UBorder>(Riga);
	Riga->LineText = NewObject<UTextBlock>(Riga);
	const auto Uguale = [](const FText& Testo, const TCHAR* Atteso) { return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive); };
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };

	FRTPlayerEventLineView Vista;
	Vista.Text = FText::FromString(TEXT("Reazione scattata"));
	Vista.Phase = ERTMatchPhase::Blast;
	Vista.Sequence = 3;
	Vista.bReaction = true;
	Riga->SetLine(Vista);
	TestTrue(TEXT("la striscia ha il colore del Blast"), Riga->PhaseStrip->GetBrushColor().Equals(URTUIPalette::ColorFor(ERTUIToken::Phase_Blast)));
	TestTrue(TEXT("la sigla dice BLAST"), Uguale(Riga->PhaseLabelText->GetText(), TEXT("BLAST")));
	TestTrue(TEXT("in Text_Primary, come la tavola"),
		Riga->PhaseLabelText->GetColorAndOpacity().GetSpecifiedColor().Equals(URTUIPalette::ColorFor(ERTUIToken::Text_Primary)));
	TestTrue(TEXT("il numero in Text_Secondary"),
		Riga->SequenceText->GetColorAndOpacity().GetSpecifiedColor().Equals(URTUIPalette::ColorFor(ERTUIToken::Text_Secondary)));
	TestTrue(TEXT("il numero dice #3"), Uguale(Riga->SequenceText->GetText(), TEXT("#3")));
	TestTrue(TEXT("una reazione ha il chip"), Visibile(Riga->ReactionBadge));
	TestTrue(TEXT("e la frase e' quella della vista"), Riga->LineText->GetText().EqualTo(Vista.Text));

	Vista.Phase = ERTMatchPhase::Move;
	Vista.bReaction = false;
	Riga->SetLine(Vista);
	TestTrue(TEXT("una riga del Move ha il suo colore"), Riga->PhaseStrip->GetBrushColor().Equals(URTUIPalette::ColorFor(ERTUIToken::Phase_Move)));
	TestFalse(TEXT("e senza reazione il chip si chiude"), Visibile(Riga->ReactionBadge));
	return true;
}

/**
 * 🔑 **IL REGISTRO IN RISOLUZIONE COSTRUISCE LE RIGHE DALLA CLASSE DEI DEFAULT, E SENZA UN TURNO DA MOSTRARE RESTA CHIUSO**
 * (#3637, [D-496]; #3860, [D-548]). Il banco di `MatchHeaderCarriesThePlayedPhase`: un turno vero, riprodotto.
 *
 * Fino a [D-548] questo test si chiamava `EventLogShowsOnlyInResolution` e pinnava «in Pianificazione il registro e'
 * chiuso» come regola. Dopo [D-548] la regola e' un'altra — in Pianificazione mostra il turno appena risolto — e quello
 * che il test pinna ancora e' un caso solo: **un `TurnLog` vuoto**, cioe' il primo turno. Il caso con un turno da
 * mostrare e' di `EventLogKeepsThePreviousTurnInPlanning`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogLookTest,
	"RefactorTactics.ScreenHud.EventLogShowsTheCurrentTurnInResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEventLogLookTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	SpawnPlaybackMap(World);
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTPlayerEventLogWidget* Log = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !Dasher || !Target || !Log)
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Log->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Log->LogRoot = NewObject<UBorder>(Log);
	Log->LogTurnText = NewObject<UTextBlock>(Log);
	UVerticalBox* Righe = NewObject<UVerticalBox>(Log);
	Log->LinesBox = Righe;
	Log->LineClass = URTPlayerEventLineWidget::StaticClass();
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };

	// --- Pianificazione al primo turno: nessun turno precedente da mostrare, il registro e' chiuso ([D-548]) ----------------
	Log->RefreshLook();
	TestFalse(TEXT("in Pianificazione, senza un turno risolto, il registro e' chiuso"), Visibile(Log->LogRoot));
	TestTrue(TEXT("e dice il round come l'header, ROUND"),
		Log->LogTurnText->GetText().ToString().StartsWith(TEXT("ROUND "), ESearchCase::CaseSensitive));

	// --- Risoluzione: il registro c'e', una riga per voce del feed ------------------------------------------------------
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;
	Dasher->PlannedAttackTarget = Target;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il turno e' in risoluzione"), TM->IsResolving()))
	{
		DestroyPlaybackWorld(World);
		return false;
	}
	Log->RefreshLook();
	TestTrue(TEXT("in Risoluzione il registro c'e'"), Visibile(Log->LogRoot));
	const TArray<FRTPlayerEventLineView> Feed = Log->GetFeed();
	if (TestTrue(TEXT("premessa: il turno ha delle righe"), Feed.Num() > 0))
	{
		TestEqual(TEXT("una riga per voce del feed"), Log->GetLineWidgets().Num(), Feed.Num());
		TestEqual(TEXT("e tutte nel pannello"), Righe->GetChildrenCount(), Feed.Num());
		for (int32 i = 0; i < Feed.Num() && i < Log->GetLineWidgets().Num(); ++i)
		{
			TestEqual(*FString::Printf(TEXT("la riga %d porta il suo numero"), i), Log->GetLineWidgets()[i]->GetLine().Sequence, Feed[i].Sequence);
		}
	}

	// --- Un secondo giro riusa le righe -----------------------------------------------------------------------------------
	const URTPlayerEventLineWidget* Prima = Log->GetLineWidgets().Num() > 0 ? Log->GetLineWidgets()[0].Get() : nullptr;
	Log->RefreshLook();
	TestTrue(TEXT("le righe si riusano"), Prima == nullptr || Log->GetLineWidgets()[0] == Prima);
	TestEqual(TEXT("e il pannello non raddoppia"), Righe->GetChildrenCount(), Log->GetLineWidgets().Num());

	DestroyPlaybackWorld(World);
	return true;
}

namespace
{
	/** Il piano di un turno vero: il Guardian scatta di una cella e attacca. Falso se l'unita' non ha uno scatto. */
	bool PlanDashOnTarget(ARTUnit* Dasher, ARTUnit* Target)
	{
		const int32 DashIdx = Dasher->FindDashAbilityIndex();
		if (DashIdx == INDEX_NONE) { return false; }
		Dasher->PlannedDashAbility = DashIdx;
		Dasher->PlannedDashCell = FRTCellId(3, 2);
		Dasher->PlannedAbilityIndex = 0;
		Dasher->PlannedAttackTarget = Target;
		return true;
	}
}

/**
 * 🔑 **IN PIANIFICAZIONE IL REGISTRO MOSTRA IL TURNO APPENA RISOLTO, MARCATO «PRECEDENTE», FINO AL COMMIT SEGUENTE**
 * (#3860, [D-548]; emenda [D-496] (2)).
 *
 * Un turno vero, dalla Pianificazione alla Pianificazione successiva, con il registro osservato a ogni passo. Le gambe:
 *   0. al primo turno non c'e' niente da rileggere: chiuso, e nessuna riga;
 *   1. in Risoluzione il registro e' quello di prima: «ROUND 01», senza «PRECEDENTE», e le righe portano il turno 1;
 *   2. a turno risolto (round 2) il registro e' APERTO, con le stesse righe, e l'intestazione dice il turno delle righe e
 *      che e' passato: «ROUND 01 · PRECEDENTE» — non il round corrente, 02;
 *   3. un giro di rilettura riusa le righe;
 *   4. al commit seguente le righe vecchie lasciano il posto a quelle del turno nuovo, e «PRECEDENTE» sparisce.
 *
 * Il cambio di modo si osserva con `RefreshLook(false)`, cioe' SENZA il giro dei 5 Hz: e' il modo di vedere che l'ingresso
 * in un modo rilegge il feed subito invece di aspettare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogPreviousTurnTest,
	"RefactorTactics.ScreenHud.EventLogKeepsThePreviousTurnInPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEventLogPreviousTurnTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyPlaybackWorld(World); };
	SpawnPlaybackMap(World);
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTPlayerEventLogWidget* Log = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !Dasher || !Target || !Log) { return false; }
	Log->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Log->LogRoot = NewObject<UBorder>(Log);
	Log->LogTurnText = NewObject<UTextBlock>(Log);
	UVerticalBox* Righe = NewObject<UVerticalBox>(Log);
	Log->LinesBox = Righe;
	Log->LineClass = URTPlayerEventLineWidget::StaticClass();
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };
	const auto Intestazione = [Log]() { return Log->LogTurnText->GetText().ToString(); };
	const auto Uguale = [](const FString& A, const TCHAR* B) { return A.Equals(B, ESearchCase::CaseSensitive); };

	// --- 0. Primo turno: niente da rileggere ---------------------------------------------------------------------------
	Log->RefreshLook();
	TestFalse(TEXT("0: al primo turno il registro e' chiuso, non c'e' un turno precedente"), Visibile(Log->LogRoot));
	TestEqual(TEXT("0: e non costruisce righe"), Log->GetLineWidgets().Num(), 0);

	// --- 1. Il turno si risolve: il registro e' quello di sempre -----------------------------------------------------------
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), PlanDashOnTarget(Dasher, Target))) { return false; }
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il turno e' in risoluzione"), TM->IsResolving())) { return false; }
	Log->RefreshLook(/*bRereadFeed=*/ false);
	TestTrue(TEXT("1: in Risoluzione il registro c'e'"), Visibile(Log->LogRoot));
	const TArray<FRTPlayerEventLineView> FeedDelTurno = Log->GetFeed();
	if (!TestTrue(TEXT("premessa: il turno ha delle righe"), FeedDelTurno.Num() > 0)) { return false; }
	TestEqual(TEXT("1: l'ingresso in Risoluzione rilegge il feed subito, senza il giro dei 5 Hz"),
		Log->GetLineWidgets().Num(), FeedDelTurno.Num());
	TestTrue(TEXT("1: l'intestazione dice il round, ROUND 01"), Uguale(Intestazione(), TEXT("ROUND 01")));
	TestEqual(TEXT("premessa: le righe del primo turno portano il turno 1"), FeedDelTurno[0].TurnNumber, 1);

	// --- 2. Il turno e' risolto: Pianificazione del turno 2 --------------------------------------------------------------
	AdvanceUntilDone(TM);
	if (!TestFalse(TEXT("premessa: la risoluzione e' finita"), TM->IsResolving())
		|| !TestTrue(TEXT("premessa: si e' tornati in Pianificazione"), TM->GetPhase() == ERTMatchPhase::Planning)
		|| !TestEqual(TEXT("premessa: il round corrente e' il 2"), URTHudViewModel::BuildMatchHeader(TM).Round, 2))
	{
		return false;
	}
	Log->RefreshLook(/*bRereadFeed=*/ false);
	TestTrue(TEXT("2: in Pianificazione, con un turno risolto, il registro e' aperto"), Visibile(Log->LogRoot));
	TestEqual(TEXT("2: l'ingresso in Pianificazione rilegge il feed subito"), Log->GetLineWidgets().Num(), FeedDelTurno.Num());
	TestTrue(TEXT("2: l'intestazione dice il turno DELLE RIGHE e che e' passato, non il round corrente"),
		Uguale(Intestazione(), TEXT("ROUND 01 \u00B7 PRECEDENTE")));
	for (int32 i = 0; i < FeedDelTurno.Num() && i < Log->GetLineWidgets().Num(); ++i)
	{
		const URTPlayerEventLineWidget* Riga = Log->GetLineWidgets()[i];
		if (!TestNotNull(*FString::Printf(TEXT("2: la riga %d esiste"), i), Riga)) { continue; }
		TestEqual(*FString::Printf(TEXT("2: la riga %d e' del turno 1"), i), Riga->GetLine().TurnNumber, 1);
		TestTrue(*FString::Printf(TEXT("2: la riga %d e' la stessa che si leggeva in Risoluzione"), i),
			Riga->GetLine().Text.EqualTo(FeedDelTurno[i].Text));
	}

	// --- 3. Un giro di rilettura riusa le righe ---------------------------------------------------------------------------
	const URTPlayerEventLineWidget* Prima = Log->GetLineWidgets().Num() > 0 ? Log->GetLineWidgets()[0].Get() : nullptr;
	Log->RefreshLook(/*bRereadFeed=*/ true);
	TestTrue(TEXT("3: le righe si riusano"), Prima == nullptr || Log->GetLineWidgets()[0] == Prima);
	TestEqual(TEXT("3: e il pannello non raddoppia"), Righe->GetChildrenCount(), Log->GetLineWidgets().Num());

	// --- 4. Il commit seguente: le righe vecchie lasciano il posto a quelle del turno nuovo ------------------------------
	Dasher->PlannedAbilityIndex = 0;
	Dasher->PlannedAttackTarget = Target;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: anche il secondo turno e' in risoluzione"), TM->IsResolving())) { return false; }
	Log->RefreshLook(/*bRereadFeed=*/ false);
	const TArray<FRTPlayerEventLineView> FeedDelSecondo = Log->GetFeed();
	if (!TestTrue(TEXT("premessa: il secondo turno ha delle righe"), FeedDelSecondo.Num() > 0)) { return false; }
	TestTrue(TEXT("4: in Risoluzione l'intestazione e' di nuovo il round, ROUND 02, senza PRECEDENTE"), Uguale(Intestazione(), TEXT("ROUND 02")));
	TestEqual(TEXT("4: le righe sono quelle del turno nuovo"), Log->GetLineWidgets().Num(), FeedDelSecondo.Num());
	for (int32 i = 0; i < Log->GetLineWidgets().Num(); ++i)
	{
		const URTPlayerEventLineWidget* Riga = Log->GetLineWidgets()[i];
		if (TestNotNull(*FString::Printf(TEXT("4: la riga %d esiste"), i), Riga))
		{
			TestEqual(*FString::Printf(TEXT("4: la riga %d e' del turno 2"), i), Riga->GetLine().TurnNumber, 2);
		}
	}
	return true;
}

/**
 * 🔴 **IN PIANIFICAZIONE LE RIGHE SI RIPROIETTANO PER L'OSSERVATORE DEL MOMENTO** (#3860, [D-548] (4)).
 *
 * Il registro in Pianificazione non conserva copie: rilegge il `TurnLog` ad ogni giro, con gli osservatori che ci sono
 * adesso. La prova e' un cambio di osservatore a registro gia' aperto: una voce autorizzata solo alla squadra 0 compare
 * per la squadra 0 e SPARISCE quando chi guarda diventa la squadra 1. Una copia delle righe gia' proiettate la lascerebbe
 * a schermo — e mostrerebbe a una squadra cio' che e' autorizzato a un'altra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogPlanningReprojectsTest,
	"RefactorTactics.ScreenHud.EventLogInPlanningReprojectsForTheCurrentObserver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEventLogPlanningReprojectsTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyPlaybackWorld(World); };
	SpawnPlaybackMap(World);
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTPlayerEventLogWidget* Log = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !Dasher || !Target || !Log) { return false; }
	Log->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Log->LogRoot = NewObject<UBorder>(Log);
	Log->LogTurnText = NewObject<UTextBlock>(Log);
	Log->LinesBox = NewObject<UVerticalBox>(Log);
	Log->LineClass = URTPlayerEventLineWidget::StaticClass();

	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), PlanDashOnTarget(Dasher, Target))) { return false; }
	TM->LockInAndResolve();
	AdvanceUntilDone(TM);
	if (!TestTrue(TEXT("premessa: si e' tornati in Pianificazione"),
		TM->GetPhase() == ERTMatchPhase::Planning && !TM->IsResolving())) { return false; }

	// Una voce del turno appena risolto, autorizzata SOLO alla squadra 0.
	int32 Turno = 0;
	for (const FRTTurnLogEntry& Voce : TM->GetTurnLog()) { Turno = FMath::Max(Turno, Voce.TurnNumber); }
	constexpr int32 UnitaPrivata = 9001; // non e' un'unita' in campo: il proiettore non la cerca
	FRTTurnLogEntry Privata;
	Privata.Category = ERTLogCategory::Combat;
	Privata.Outcome = static_cast<uint8>(ERTCombatOutcome::Hit);
	Privata.UnitId = UnitaPrivata;
	Privata.TurnNumber = Turno;
	Privata.Phase = ERTMatchPhase::Blast;
	Privata.Amount = 7;
	Privata.Verdict.AllowTeam(0);
	TM->AppendTurnLogEntryForTest(Privata);

	const auto HaLaRiga = [](const URTPlayerEventLogWidget* Widget, const int32 Unita)
	{
		for (const URTPlayerEventLineWidget* Riga : Widget->GetLineWidgets())
		{
			if (Riga && Riga->GetLine().PrimaryStableUnitId == Unita) { return true; }
		}
		return false;
	};

	Log->RefreshLook(/*bRereadFeed=*/ true);
	TestTrue(TEXT("premessa: per la squadra 0 il registro e' aperto in Pianificazione"),
		Log->LogRoot->GetVisibility() != ESlateVisibility::Collapsed);
	if (!TestTrue(TEXT("premessa: la squadra 0 legge la voce privata"), HaLaRiga(Log, UnitaPrivata))) { return false; }

	// Chi guarda diventa la squadra 1: la voce non e' sua, e la riga NON puo' restare.
	Log->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 1);
	Log->RefreshLook(/*bRereadFeed=*/ true);
	TestFalse(TEXT("per la squadra 1 la voce privata della squadra 0 non c'e' piu'"), HaLaRiga(Log, UnitaPrivata));
	return true;
}

/**
 * IL REGISTRO RISCRIVE LA RIGA QUANDO CAMBIA IL TESTO E NON IL NUMERO DI RIGHE (#3073).
 *
 * Il difetto che la issue ha misurato stava nel grafo di `WBP_RT_EventLog`, tolto da #3637: il feed si
 * ricostruiva solo se cambiava il NUMERO di righe (`Array_Length(feed) != GetChildrenCount(FeedBox)`).
 * La dominanza del proiettore SOSTITUISCE sul posto - un danno che diventa KO e' ancora una riga - quindi
 * il conteggio non si muove e a schermo restava il testo vecchio. Ora il registro e' C++ e rilegge il feed a
 * 5 Hz (`RefreshLook(true)` e' il giro che `NativeTick` fa a ogni scadenza): questo test pinna che la
 * rilettura riscriva le righe che ci sono, non che ne costruisca di nuove.
 *
 * ATTENZIONE: nasce VERDE, perche' il difetto non esiste piu' su `main`. E' una guardia, e la sua validita'
 * si prova con le mutazioni, non con un rosso alla base: la rilettura ridotta al solo conteggio delle righe
 * deve renderlo rosso nella gamba 2 e nella 3, e il ridisegno che salta `SetLine` quando fase, numero,
 * reazione e importanza coincidono (ignorando il testo) deve renderlo rosso nella gamba 3.
 *
 * Il turno vero serve a una cosa sola: portare il manager in Risoluzione, senza la quale il registro e'
 * chiuso e non costruisce righe. Il contenuto che conta e' iniettato nel `TurnLog` con voci di unita'
 * sintetiche, cosi' il test non dipende da cosa quel turno ha scritto. Tre gambe:
 *   1. un danno sull'unita' sintetica porta il feed da N a N+1 righe (controllo positivo: il giro che anche
 *      la vecchia guardia faceva scattare);
 *   2. un colpo letale sulla stessa unita' la sostituisce sul posto: stesso numero di righe, testo diverso.
 *      Cambia pero' anche l'importanza (Important -> Critical): da sola questa gamba non isola il testo;
 *   3. uno spostamento subito (Move/Displaced, Important, rango 20) sostituito da un colpo (Combat/Hit,
 *      Important, rango 80) sulla stessa unita' e nella stessa fase: fase, numero e importanza restano
 *      uguali, cambia SOLO il testo. E' la gamba che distingue una rilettura che guarda il testo da una che
 *      guarda i metadati della riga.
 * Si confronta il testo che la riga SCRIVE (`GetLine`) e quello del blocco di testo che il giocatore vede
 * (`LineText`, contro un letterale e non contro il solo `GetFeed()`), e che la riga sia la stessa istanza:
 * riusata e riscritta, non ricostruita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogRewritesLineWhenCountIsUnchangedTest,
	"RefactorTactics.ScreenHud.EventLogRewritesLineWhenCountIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEventLogRewritesLineWhenCountIsUnchangedTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyPlaybackWorld(World); };
	SpawnPlaybackMap(World);
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTPlayerEventLogWidget* Log = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !Dasher || !Target || !Log) { return false; }
	Log->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Log->LogRoot = NewObject<UBorder>(Log);
	Log->LogTurnText = NewObject<UTextBlock>(Log);
	UVerticalBox* Righe = NewObject<UVerticalBox>(Log);
	Log->LinesBox = Righe;
	Log->LineClass = URTPlayerEventLineWidget::StaticClass();

	// --- Un turno vero, solo per entrare in Risoluzione ------------------------------------------------------------
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE)) { return false; }
	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTCellId(3, 2);
	Dasher->PlannedAbilityIndex = 0;
	Dasher->PlannedAttackTarget = Target;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il turno e' in risoluzione"), TM->IsResolving())) { return false; }

	// Le voci iniettate stanno nel turno PIU' RECENTE del log: il feed legge solo quello.
	int32 Turno = 0;
	for (const FRTTurnLogEntry& Voce : TM->GetTurnLog()) { Turno = FMath::Max(Turno, Voce.TurnNumber); }
	constexpr int32 UnitaDelDanno = 9001;       // non sono unita' in campo: il proiettore non le cerca
	constexpr int32 UnitaDelloSpostamento = 9002;
	const auto VoceDi = [Turno](const ERTLogCategory Categoria, const uint8 Esito, const int32 Unita)
	{
		FRTTurnLogEntry Voce;
		Voce.Category = Categoria;
		Voce.Outcome = Esito;
		Voce.UnitId = Unita;
		Voce.TurnNumber = Turno;
		Voce.Phase = ERTMatchPhase::Blast; // la stessa fase per tutte: nella gamba 3 non deve cambiare
		Voce.Amount = 7;
		Voce.Verdict.AllowTeam(0); // la squadra di chi guarda la conosce
		return Voce;
	};
	const auto Combattimento = [&VoceDi](const ERTCombatOutcome Esito, const int32 Unita)
	{
		return VoceDi(ERTLogCategory::Combat, static_cast<uint8>(Esito), Unita);
	};
	const auto Movimento = [&VoceDi](const ERTMoveOutcome Esito, const int32 Unita)
	{
		return VoceDi(ERTLogCategory::Move, static_cast<uint8>(Esito), Unita);
	};
	const auto RigaDellUnita = [](const TArray<FRTPlayerEventLineView>& Feed, const int32 Unita) -> int32
	{
		for (int32 i = 0; i < Feed.Num(); ++i)
		{
			if (Feed[i].PrimaryStableUnitId == Unita) { return i; }
		}
		return INDEX_NONE;
	};
	const auto Letterale = [](const FText& Testo, const TCHAR* Atteso)
	{
		return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive);
	};
	// Le righe del WBP hanno un blocco di testo; qui le righe sono C++ puro, quindi lo si da' a quelle che non
	// ce l'hanno e le si riscrive col testo che gia' portano. Cosi' si osserva il testo VISIBILE, non solo la
	// vista che la riga conserva.
	const auto DaiIlBloccoDiTesto = [this, Log]()
	{
		bool bTutte = true;
		for (URTPlayerEventLineWidget* Riga : Log->GetLineWidgets())
		{
			if (!TestNotNull(TEXT("ogni riga costruita esiste"), Riga)) { bTutte = false; continue; }
			if (Riga->LineText == nullptr)
			{
				Riga->LineText = NewObject<UTextBlock>(Riga);
				Riga->SetLine(Riga->GetLine());
			}
		}
		return bTutte;
	};
	// Ogni riga mostra, nel blocco di testo visibile, il testo del feed attuale.
	const auto TutteMostranoIlFeed = [this, Log](const TArray<FRTPlayerEventLineView>& Feed, const TCHAR* Gamba)
	{
		for (int32 i = 0; i < Feed.Num() && i < Log->GetLineWidgets().Num(); ++i)
		{
			const URTPlayerEventLineWidget* UnaRiga = Log->GetLineWidgets()[i];
			if (!TestNotNull(*FString::Printf(TEXT("%s: la riga %d esiste"), Gamba, i), UnaRiga)) { continue; }
			TestTrue(*FString::Printf(TEXT("%s: la riga %d mostra il testo del feed attuale"), Gamba, i),
				UnaRiga->LineText && UnaRiga->LineText->GetText().EqualTo(Feed[i].Text));
		}
	};

	// --- Il turno vero: il registro costruisce le sue righe ---------------------------------------------------------
	Log->RefreshLook(/*bRereadFeed=*/ true);
	const int32 RigheDelTurno = Log->GetLineWidgets().Num();
	const TArray<FRTPlayerEventLineView> FeedDelTurno = Log->GetFeed();
	if (!TestEqual(TEXT("premessa: il registro ha una riga per voce del feed"), RigheDelTurno, FeedDelTurno.Num())
		|| !TestTrue(TEXT("premessa: c'e' posto per due righe in piu' senza che il budget del feed ne tolga una"),
			RigheDelTurno + 2 < URTHudViewModel::MaxFeedLines)
		|| !TestEqual(TEXT("premessa: l'unita' del danno non ha ancora una riga"), RigaDellUnita(FeedDelTurno, UnitaDelDanno), INDEX_NONE)
		|| !TestEqual(TEXT("premessa: nemmeno quella dello spostamento"), RigaDellUnita(FeedDelTurno, UnitaDelloSpostamento), INDEX_NONE))
	{
		return false;
	}

	// --- 1. CONTROLLO POSITIVO: il numero di righe cambia, e la riga nuova compare con il suo testo -------------------
	TM->AppendTurnLogEntryForTest(Combattimento(ERTCombatOutcome::Hit, UnitaDelDanno));
	Log->RefreshLook(/*bRereadFeed=*/ true);
	const TArray<FRTPlayerEventLineView> FeedConDanno = Log->GetFeed();
	const int32 Indice = RigaDellUnita(FeedConDanno, UnitaDelDanno);
	if (!TestEqual(TEXT("1: il danno aggiunge UNA riga al feed"), FeedConDanno.Num(), RigheDelTurno + 1)
		|| !TestTrue(TEXT("1: e' quella dell'unita' del danno"), Indice != INDEX_NONE)
		|| !TestEqual(TEXT("1: il registro ha costruito la riga nuova"), Log->GetLineWidgets().Num(), RigheDelTurno + 1)
		|| !TestEqual(TEXT("1: e il pannello la porta"), Righe->GetChildrenCount(), RigheDelTurno + 1))
	{
		return false;
	}
	URTPlayerEventLineWidget* Riga = Log->GetLineWidgets()[Indice];
	if (!TestNotNull(TEXT("1: la riga dell'unita' del danno"), Riga)) { return false; }
	TestTrue(TEXT("1: la riga porta il testo del danno"), Riga->GetLine().Text.EqualTo(FeedConDanno[Indice].Text));
	TestFalse(TEXT("1: e non e' ancora il KO"), Letterale(FeedConDanno[Indice].Text, TEXT("Eliminata")));
	if (!DaiIlBloccoDiTesto()) { return false; }
	const FText TestoDelDanno = Riga->LineText->GetText();
	TestTrue(TEXT("premessa: la riga mostra il testo del danno"), TestoDelDanno.EqualTo(FeedConDanno[Indice].Text));

	// --- 2. LO STESSO NUMERO DI RIGHE, UN TESTO DIVERSO: il KO sostituisce il danno sul posto -----------------------
	TM->AppendTurnLogEntryForTest(Combattimento(ERTCombatOutcome::Lethal, UnitaDelDanno));
	const TArray<FRTPlayerEventLineView> FeedConKO = Log->GetFeed();
	if (!TestEqual(TEXT("premessa: il KO non aggiunge una riga, sostituisce quella del danno"), FeedConKO.Num(), FeedConDanno.Num())
		|| !TestEqual(TEXT("premessa: e la riga resta al suo posto"), RigaDellUnita(FeedConKO, UnitaDelDanno), Indice)
		|| !TestFalse(TEXT("premessa: ma il feed ora dice un'altra cosa"), FeedConKO[Indice].Text.EqualTo(TestoDelDanno)))
	{
		return false;
	}

	Log->RefreshLook(/*bRereadFeed=*/ true); // il giro dei 5 Hz di `NativeTick`

	TestEqual(TEXT("2: il registro ha ancora lo stesso numero di righe"), Log->GetLineWidgets().Num(), FeedConKO.Num());
	TestEqual(TEXT("2: e il pannello non ne ha di nuove"), Righe->GetChildrenCount(), FeedConKO.Num());
	TestTrue(TEXT("2: la riga e' la stessa istanza: riusata e riscritta, non ricostruita"), Log->GetLineWidgets()[Indice] == Riga);
	TestTrue(TEXT("2: la riga conserva la vista nuova, quella del KO"), Riga->GetLine().Text.EqualTo(FeedConKO[Indice].Text));
	TestTrue(TEXT("2: e il blocco di testo che il giocatore legge dice il KO, non il danno"),
		Letterale(Riga->LineText->GetText(), TEXT("Eliminata")));
	TestFalse(TEXT("2: il testo del danno non resta a schermo"), Riga->LineText->GetText().EqualTo(TestoDelDanno));
	TutteMostranoIlFeed(FeedConKO, TEXT("2"));

	// --- 3. SOLO IL TESTO CAMBIA: fase, numero, reazione e importanza restano gli stessi ---------------------------
	// Lo spostamento subito entra come riga nuova (il numero di righe cambia, ancora una volta).
	TM->AppendTurnLogEntryForTest(Movimento(ERTMoveOutcome::Displaced, UnitaDelloSpostamento));
	Log->RefreshLook(/*bRereadFeed=*/ true);
	const TArray<FRTPlayerEventLineView> FeedConSpostamento = Log->GetFeed();
	const int32 IndiceSpostamento = RigaDellUnita(FeedConSpostamento, UnitaDelloSpostamento);
	if (!TestEqual(TEXT("3: lo spostamento aggiunge UNA riga al feed"), FeedConSpostamento.Num(), FeedConKO.Num() + 1)
		|| !TestTrue(TEXT("3: e' quella dell'unita' dello spostamento"), IndiceSpostamento != INDEX_NONE)
		|| !TestEqual(TEXT("3: il registro ha costruito la riga nuova"), Log->GetLineWidgets().Num(), FeedConSpostamento.Num()))
	{
		return false;
	}
	URTPlayerEventLineWidget* RigaSpostata = Log->GetLineWidgets()[IndiceSpostamento];
	if (!TestNotNull(TEXT("3: la riga dell'unita' dello spostamento"), RigaSpostata) || !DaiIlBloccoDiTesto()) { return false; }
	if (!TestTrue(TEXT("premessa: la riga mostra lo spostamento, 'Spostata'"),
		Letterale(RigaSpostata->LineText->GetText(), TEXT("Spostata"))))
	{
		return false;
	}

	// Il colpo sulla stessa unita' (rango 80 contro 20) prende il posto dello spostamento.
	TM->AppendTurnLogEntryForTest(Combattimento(ERTCombatOutcome::Hit, UnitaDelloSpostamento));
	const TArray<FRTPlayerEventLineView> FeedConColpo = Log->GetFeed();
	if (!TestEqual(TEXT("premessa: il colpo non aggiunge una riga, sostituisce lo spostamento"), FeedConColpo.Num(), FeedConSpostamento.Num())
		|| !TestEqual(TEXT("premessa: e la riga resta al suo posto"), RigaDellUnita(FeedConColpo, UnitaDelloSpostamento), IndiceSpostamento))
	{
		return false;
	}
	// Le premesse che fanno di questa gamba quella del SOLO testo: ogni campo che `RTSameLine` confronta, tranne il testo.
	const FRTPlayerEventLineView& Prima = FeedConSpostamento[IndiceSpostamento];
	const FRTPlayerEventLineView& Dopo = FeedConColpo[IndiceSpostamento];
	if (!TestEqual(TEXT("premessa: la fase e' la stessa"), static_cast<int32>(Dopo.Phase), static_cast<int32>(Prima.Phase))
		|| !TestEqual(TEXT("premessa: il numero e' lo stesso"), Dopo.Sequence, Prima.Sequence)
		|| !TestEqual(TEXT("premessa: l'importanza e' la stessa"), static_cast<int32>(Dopo.Importance), static_cast<int32>(Prima.Importance))
		|| !TestTrue(TEXT("premessa: la reazione e' la stessa"), Dopo.bReaction == Prima.bReaction)
		|| !TestFalse(TEXT("premessa: il testo e' l'unica differenza"), Dopo.Text.EqualTo(Prima.Text)))
	{
		return false;
	}

	Log->RefreshLook(/*bRereadFeed=*/ true); // il giro dei 5 Hz di `NativeTick`

	TestEqual(TEXT("3: il registro ha ancora lo stesso numero di righe"), Log->GetLineWidgets().Num(), FeedConColpo.Num());
	TestEqual(TEXT("3: e il pannello non ne ha di nuove"), Righe->GetChildrenCount(), FeedConColpo.Num());
	TestTrue(TEXT("3: la riga e' la stessa istanza: riusata e riscritta, non ricostruita"),
		Log->GetLineWidgets()[IndiceSpostamento] == RigaSpostata);
	TestTrue(TEXT("3: la riga conserva la vista nuova, quella del colpo"), RigaSpostata->GetLine().Text.EqualTo(Dopo.Text));
	TestTrue(TEXT("3: e il blocco di testo che il giocatore legge dice il colpo, non la spinta"),
		Letterale(RigaSpostata->LineText->GetText(), TEXT("Colpita \u2014 7")));
	TestFalse(TEXT("3: 'Spostata' non resta a schermo"), Letterale(RigaSpostata->LineText->GetText(), TEXT("Spostata")));
	TutteMostranoIlFeed(FeedConColpo, TEXT("3"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
