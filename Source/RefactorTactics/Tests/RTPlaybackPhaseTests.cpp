#include "Misc/AutomationTest.h"
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
	TestEqual(*FString::Printf(TEXT("1: in %d tick, a ogni tick la sola cella attiva e' la fase riprodotta, e i gruppi sono giusti"),
		TickRisoluzione), Sbagliate, 0);

	// --- D. Dopo il playback si torna alla Pianificazione ---------------------------------------------------------------
	W->RefreshLook(/*bRecheckObjective=*/ false);
	TestTrue(TEXT("D: dopo il playback torna il gruppo della Pianificazione"), Visibile(W->PlanningGroup));
	TestFalse(TEXT("D: e il gruppo della Risoluzione si chiude"), Visibile(W->ResolutionGroup));

	DestroyPlaybackWorld(World);
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
 * 🔑 **IL REGISTRO COMPARE SOLO IN RISOLUZIONE, E COSTRUISCE LE RIGHE DALLA CLASSE DEI DEFAULT** (#3637, [D-496]).
 * Il banco di `MatchHeaderCarriesThePlayedPhase`: un turno vero, riprodotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEventLogLookTest,
	"RefactorTactics.ScreenHud.EventLogShowsOnlyInResolution",
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

	// --- Pianificazione: il registro non c'e' ([D-496] punto 2) ---------------------------------------------------------
	Log->RefreshLook();
	TestFalse(TEXT("in Pianificazione il registro e' chiuso"), Visibile(Log->LogRoot));
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

#endif // WITH_DEV_AUTOMATION_TESTS
