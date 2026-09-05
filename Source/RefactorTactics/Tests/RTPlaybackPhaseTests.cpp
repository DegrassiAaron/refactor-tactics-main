#include "Misc/AutomationTest.h"
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
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(4, 2));
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

	ARTUnit* Mover = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 2));
	ARTUnit* Still = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(6, 6));
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

	ARTUnit* Corta = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Lunga = SpawnPlaybackUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 4));
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

#endif // WITH_DEV_AUTOMATION_TESTS
