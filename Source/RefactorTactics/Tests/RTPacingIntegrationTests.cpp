#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPacing.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Combat/RTOffensiveActionLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Telemetria di pacing con un mondo vero. Helper locali come in RTHexBotIntegrationTests.cpp e
 * RTHexCombatIntegrationTests.cpp: ogni file d'integrazione tiene i propri, in namespace anonimo.
 */
namespace
{
	UWorld* MakeHexPacingWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexPacingWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnHexPacingMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnHexPacingUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay(); // senza, i cooldown restano vuoti e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * TurnManager pronto a giocare. DispatchBeginPlay e' NECESSARIO: e' BeginPlay a chiamare
	 * StartPlanningTimer, che apre il campione di pacing del PRIMO turno. Senza, i campioni sarebbero
	 * sistematicamente uno in meno dei turni giocati.
	 */
	ARTTurnManager* SpawnHexPacingTurnManager(UWorld* World)
	{
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (TM) { TM->DispatchBeginPlay(); }
		return TM;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingSamplePerTurnTest,
	"RefactorTactics.Pacing.EveryTurnProducesOneSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingSamplePerTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 5);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, -3));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
	{
		RTWorldFixtures::PlayOneTurn(TM);
		++TurnsPlayed;
	}

	// Un campione per turno, ULTIMO COMPRESO. Il turno decisivo passa da ConcludeTurn, che pero' esce
	// prima di incrementare TurnNumber quando la partita finisce: se il campione si chiudesse dopo quel
	// ritorno anticipato, l'unico turno che decide la partita non verrebbe mai misurato.
	TestEqual(TEXT("un campione per turno giocato"), TM->GetPacingSamples().Num(), TurnsPlayed);
	TestTrue(TEXT("la partita si e' decisa"), TM->GetPhase() == ERTMatchPhase::MatchEnded);

	// I numeri di turno sono progressivi da 1: nessun buco, nessun doppione.
	for (int32 I = 0; I < TM->GetPacingSamples().Num(); ++I)
	{
		TestEqual(FString::Printf(TEXT("campione %d e' del turno %d"), I, I + 1),
			TM->GetPacingSamples()[I].TurnNumber, I + 1);
	}

	DestroyHexPacingWorld(World);
	return true;
}

/**
 * Un campione MAI APERTO non si chiude con un numero inventato.
 *
 * 🔴 `PacingPlanningStart` vale `0.0` finche' `BeginPacingSample()` non lo scrive, e a chiamarla e' solo
 * `StartPlanningTimer()` — cioe' `BeginPlay`. Chi arriva a `LockInAndResolve()` senza passare di li' NON e'
 * un caso di confine: sono **36 file** fra test e harness (`RTScenarioRunner.cpp` e `RTScenarioSession.cpp`
 * inclusi), contati sul branch. Il TurnManager di questo test non fa `DispatchBeginPlay` apposta: e' quel
 * percorso, non una configurazione artificiosa.
 *
 * Su Windows `FPlatformTime::Seconds()` non e' un tempo dall'avvio del processo — sono i secondi del
 * contatore ad alta risoluzione **piu' `16777216.0`** — quindi `(Now - 0.0) * 1000.0` sta intorno a
 * `1.7e10`, che `FMath::RoundToInt` tronca in un `int32` il cui massimo e' `2.1e9`: conversione fuori
 * range, comportamento non definito, in pratica un valore spazzatura o negativo.
 *
 * Il test NON asserisce il valore spazzatura — sarebbe pinnare un UB. Asserisce cio' che deve valere: il
 * turno viene misurato in cio' che e' misurabile (il contesto), e i tempi dichiarano di non esserlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingUnopenedSampleTest,
	"RefactorTactics.Pacing.UnopenedSampleIsDeclaredUnmeasured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingUnopenedSampleTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 3);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 1));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -1));

	// ⚠️ NIENTE `DispatchBeginPlay`, ed e' il punto del test: senza, `StartPlanningTimer` non gira e il
	// campione del primo turno non viene mai aperto. E' come ci arrivano i test headless e lo Scenario
	// Harness.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Un'uscita anticipata MUTA verrebbe riportata come Success: l'automation ignora il `bool` di
	// `RunTest`. Ogni puntatore si asserisce, come due righe sopra col mondo.
	const bool bSetup = TestNotNull(TEXT("il TurnManager e' stato creato"), TM)
		&& TestNotNull(TEXT("l'unita' di squadra 0"), A1)
		&& TestNotNull(TEXT("l'unita' di squadra 1"), B1);
	if (!bSetup) { DestroyHexPacingWorld(World); return false; }

	RTWorldFixtures::PlayOneTurn(TM);

	// Il turno E' STATO GIOCATO: senza questa ancora, l'asserzione sui campioni passerebbe anche se
	// `LockInAndResolve` fosse uscito subito senza risolvere niente.
	TestTrue(TEXT("il turno e' stato risolto per davvero"), TM->GetTurnLog().Num() > 0);

	if (!TestEqual(TEXT("il turno e' stato misurato lo stesso"), TM->GetPacingSamples().Num(), 1))
	{
		DestroyHexPacingWorld(World);
		return false;
	}

	const FRTPacingSample& S = TM->GetPacingSamples()[0];

	// 🔴 I TEMPI non hanno un'origine e lo dichiarano. Non zero: zero e' un lock-in istantaneo, cioe' un
	// valore legittimo — sarebbe il dato plausibile e falso che nessun errore segnala.
	TestEqual(TEXT("MsToLockIn si dichiara non misurato"), S.MsToLockIn, FRTPacingSample::Unmeasured);
	TestEqual(TEXT("e MsSinceLastInput, che ci ricade sopra"),
		S.MsSinceLastInput, FRTPacingSample::Unmeasured);
	TestEqual(TEXT("e MsToFirstInput, per lo stesso motivo"),
		S.MsToFirstInput, FRTPacingSample::Unmeasured);

	// Il CONTESTO invece e' misurabile, e va misurato: buttare il turno perderebbe un dato vero — e non e'
	// un caso di laboratorio, `SetPlanningSeconds()` arma il timer senza aprire il campione, quindi un
	// `OnPlanningTimeout` vero puo' arrivare al lock-in con il campione chiuso.
	TestEqual(TEXT("le due unita' vive sono state contate"), S.UnitsAliveTeam0 + S.UnitsAliveTeam1, 2);
	TestEqual(TEXT("ed e' il campione del turno 1"), S.TurnNumber, 1);

	DestroyHexPacingWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCompositionTest,
	"RefactorTactics.Pacing.RecordsDecisionComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCompositionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 3);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(-2, 1));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -1));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !B1) { DestroyHexPacingWorld(World); return false; }

	// Il turno 1 e' in pianificazione: simuliamo la mano del giocatore senza passare dal controller.
	TM->RecordPlanningInput(ERTPlanningInput::Selection);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Undo);
	TM->RecordPlanningInput(ERTPlanningInput::Click); // attivita' generica: non incrementa nessun contatore

	RTWorldFixtures::PlayOneTurn(TM);

	if (!TestTrue(TEXT("almeno un campione"), TM->GetPacingSamples().Num() >= 1))
	{
		DestroyHexPacingWorld(World);
		return false;
	}
	const FRTPacingSample& S = TM->GetPacingSamples()[0];
	TestEqual(TEXT("una selezione"), S.SelectionCount, 1);
	TestEqual(TEXT("due ordini"), S.OrderCount, 2);
	TestEqual(TEXT("un annullamento"), S.UndoCount, 1);
	TestTrue(TEXT("il lock-in manuale non e' un timeout"), S.LockInSource == ERTLockInSource::Input);
	TestEqual(TEXT("due unita' vive, una per squadra"), S.UnitsAliveTeam0 + S.UnitsAliveTeam1, 2);

	DestroyHexPacingWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingHashInvarianceTest,
	"RefactorTactics.Pacing.DoesNotAffectTurnLogHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingHashInvarianceTest::RunTest(const FString&)
{
	// La stessa partita giocata due volte, con la sonda spenta e accesa: gli esiti autoritativi devono
	// essere IDENTICI hash per hash. E' la dimostrazione eseguibile dell'invariante: la telemetria non ha
	// ritorno verso il gameplay. Oggi passa quasi per costruzione; serve per quando qualcuno sara' tentato
	// di far leggere un tempo di parete a una decisione.
	auto PlayMatchHashes = [this](bool bRecord, TArray<uint32>& OutHashes) -> bool
	{
		UWorld* World = MakeHexPacingWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnHexPacingMap(World, /*Radius=*/ 5);

		ARTUnit* A1 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(-4, 2));
		ARTUnit* A2 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
		ARTUnit* B1 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(4, -2));
		ARTUnit* B2 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, -3));
		ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
		if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

		TM->bRecordPacing = bRecord;
		int32 TurnsPlayed = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
		{
			RTWorldFixtures::PlayOneTurn(TM);
			++TurnsPlayed;
			OutHashes.Add(URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog()));
		}
		DestroyHexPacingWorld(World);
		return true;
	};

	TArray<uint32> Off;
	TArray<uint32> On;
	if (!PlayMatchHashes(/*bRecord=*/ false, Off)) { return false; }
	if (!PlayMatchHashes(/*bRecord=*/ true,  On))  { return false; }

	TestEqual(TEXT("stesso numero di turni con sonda accesa e spenta"), On.Num(), Off.Num());
	const int32 Count = FMath::Min(On.Num(), Off.Num());
	for (int32 I = 0; I < Count; ++I)
	{
		// TestTrue e non TestEqual: gli hash sono uint32 e le overload di TestEqual sono ambigue su quel tipo.
		TestTrue(FString::Printf(TEXT("turno %d: hash del TurnLog identico"), I + 1), On[I] == Off[I]);
	}
	return true;
}


/**
 * IL CALL SITE SCRIVE DAVVERO, E SENZA QUESTO TEST IL CAMPO POTREBBE NON ESSERE SCRITTO DA NESSUNO -
 * `#2516`.
 *
 🔴 **E' il test che impedisce il gate cieco.** L'aggregazione si prova su campioni costruiti a mano, e
 * resterebbe verde anche se `CandidatesPerEvent` non fosse mai popolata in partita: due test verdi e una
 * metrica che riporta sempre zero. Qui si gioca un turno vero e si pretende che almeno un evento sia
 * stato registrato.
 *
 🔑 **Si asserisce il numero di EVENTI e non quello dei candidati.** I candidati dipendono da chi ha
 * armato una reazione, cioe' dal bilanciamento del bot: un'assertion su quel numero sarebbe un test di
 * bilanciamento travestito, e diventerebbe rossa al primo ritocco dei pesi. Cio' che questa riga difende
 * e' il **cablaggio**.
 *
 ⚠️ **La premessa e' che qualcuno si muova**: la raccolta avviene dentro la risoluzione del movimento,
 * quindi un turno in cui nessuno muove non produce eventi - e sarebbe un verde per assenza. La si asserisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCandidateEventsAreWiredTest,
	"RefactorTactics.Pacing.CandidateEventsAreRecordedByTheResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCandidateEventsAreWiredTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 5);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexPacingUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexPacingUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, -3));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

	// 🔴 **Il turno si scrive a mano, e la ragione e' una misura.** Con `PlayOneTurn` il test dava
	// **0 eventi su 6 turni**: il call site esce prima di raccogliere quando nessuno ha armato una
	// reazione — `if (Watchers.Num() == 0) { return ERTMovementAdvanceResult::Advanced; }` — e i bot di
	// questo allestimento non ne armano. Un test cosi' sarebbe stato rosso per **assenza dello scenario**,
	// non per un difetto.
	//
	// 🔑 **E l'armamento e' `PlannedAbilityIndex` con `Action.Overwatch`, non `PlannedReactionAbility`**:
	// i guardiani escono da `ArmedOverwatches`, che `ResolvePrep` popola da chi ha PIANIFICATO l'Overwatch.
	// Misurato: con la sola reazione armata il test dava ancora 0 eventi.
	// ⚠️ **E va DOPO `PlanBotsForTest`**: la pianificazione del bot azzera e riscrive
	// `PlannedReactionAbility`, quindi armare prima verrebbe sovrascritto senza che niente lo dica.
	int32 IdxReazione = INDEX_NONE;
	for (int32 i = 0; i < B1->NumAbilities(); ++i)
	{
		const URTActionData* A = B1->GetAbility(i);
		if (A && A->Def.ActionId == TEXT("Action.Overwatch")) { IdxReazione = i; break; }
	}
	if (!TestTrue(TEXT("premessa: il guardiano ha Action.Overwatch nel kit"),
			IdxReazione != INDEX_NONE))
	{
		DestroyHexPacingWorld(World); return false;
	}

	// 🔴 **Il PRIMO turno attraversa la linea per costruzione** (#3229). Fino al 2026-10-10 il boundary si
	// apriva quando un movimento NATO DAGLI SCONTRI fra bot finiva nella linea di un guardiano — misurato: al
	// turno 5, su una partita qualsiasi. Con la mira che non insegue ([D-415]) la partita prende un'altra
	// strada, i bot restano a sparare da fermi e nessuno attraversa piu' una linea: il test diventava rosso per
	// ASSENZA dello scenario, cioe' misurava il bilanciamento dei bot invece del cablaggio del cronometro.
	//
	// 🔑 Qui la linea si calcola con la STESSA funzione del resolver (`MakeSuppressiveZone`, dalla cella del
	// guardiano verso il suo facing per la portata dell'arma), il guardiano guarda verso A e sta fermo, e A1
	// cammina su una cella della linea. Dal secondo turno decidono di nuovo i bot, come prima.
	const URTHexMapAsset* Mappa = nullptr;
	if (const ARTHexMapActor* MapActor = Cast<ARTHexMapActor>(
			UGameplayStatics::GetActorOfClass(World, ARTHexMapActor::StaticClass())))
	{
		Mappa = MapActor->MapAsset;
	}
	int32 PortataGuardiano = 0;
	for (int32 i = 0; i < B1->NumAbilities(); ++i)
	{
		const URTActionData* A = B1->GetAbility(i);
		if (A && A->Def.BaseActionId == FName(TEXT("Action.BasicAttack"))) { PortataGuardiano = FMath::Max(1, A->Def.RangeCells); break; }
	}
	const FRTSuppressiveZone Linea = URTOffensiveActionLibrary::MakeSuppressiveZone(Mappa, /*Owner*/ 2, B1->TeamId,
		B1->Cell, URTHexLibrary::Neighbor(B1->Cell, ERTHexDirection::W), PortataGuardiano, /*Damage*/ 1);
	FRTCellId Attraversata;
	bool bAttraversabile = false;
	for (const FRTCellId& Cella : Linea.Cells)
	{
		if (URTHexLibrary::HexDistance(A1->Cell, Cella) <= A1->GetEffectiveMoveRange()
			&& (!bAttraversabile || URTHexLibrary::HexDistance(A1->Cell, Cella) < URTHexLibrary::HexDistance(A1->Cell, Attraversata)))
		{
			Attraversata = Cella;
			bAttraversabile = true;
		}
	}
	if (!TestTrue(TEXT("premessa: una cella della linea del guardiano e' alla portata di movimento di A1"),
			Mappa != nullptr && bAttraversabile))
	{
		DestroyHexPacingWorld(World); return false;
	}
	const FRTCellId PartenzaA1 = A1->Cell;

	int32 Giocati = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Giocati < 8)
	{
		TM->PlanBotsForTest();
		B1->PlannedAbilityIndex = IdxReazione;
		B2->PlannedAbilityIndex = IdxReazione;
		if (Giocati == 0)
		{
			for (ARTUnit* Guardiano : { B1, B2 })
			{
				Guardiano->Facing = ERTHexDirection::W;     // verso A: la linea nasce dal facing al Prep
				Guardiano->bDeclaresPlannedFacing = false;
				Guardiano->PlannedCell = Guardiano->Cell;   // fermo: la linea resta dov'e' stata calcolata
				Guardiano->PlannedPath.Reset();
				Guardiano->PlannedWaypoints.Reset();
				Guardiano->PlannedDashAbility = INDEX_NONE;
			}
			A1->PlannedAbilityIndex = INDEX_NONE;           // solo movimento: niente attacco ne' scatto
			A1->ClearPlannedAttack();
			A1->PlannedDashAbility = INDEX_NONE;
			A1->PlannedCell = Attraversata;
			A1->PlannedPath.Reset();
			A1->PlannedWaypoints.Reset();
		}
		// E la SECONDA raccolta, per la stessa ragione: la raccolta gira solo dentro il movimento NORMALE (il
		// Dash non passa dai guardiani), e dopo il primo contatto i bot restano a sparare o scattano. Al turno 2
		// A2 fa un passo normale su una cella libera accanto: la raccolta si ripete per costruzione.
		FRTCellId PassoA2;
		bool bPassoA2 = false;
		if (Giocati == 1 && A2->IsAlive() && Mappa)
		{
			TArray<AActor*> Tutte;
			UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Tutte);
			for (int32 d = 0; d < 6 && !bPassoA2; ++d)
			{
				const FRTCellId Vicina = URTHexLibrary::Neighbor(A2->Cell, static_cast<ERTHexDirection>(d));
				bool bLibera = Mappa->ContainsCell(Vicina);
				for (const AActor* Attore : Tutte)
				{
					const ARTUnit* U = Cast<ARTUnit>(Attore);
					if (U && U->IsAlive() && U->Cell == Vicina) { bLibera = false; }
				}
				if (bLibera) { PassoA2 = Vicina; bPassoA2 = true; }
			}
			if (bPassoA2)
			{
				A2->PlannedAbilityIndex = INDEX_NONE;
				A2->ClearPlannedAttack();
				A2->PlannedDashAbility = INDEX_NONE;
				A2->PlannedCell = PassoA2;
				A2->PlannedPath.Reset();
				A2->PlannedWaypoints.Reset();
			}
		}
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		if (Giocati == 0)
		{
			// Controllo positivo del turno scritto a mano: A1 si e' mosso (fino alla linea, o fermato su di essa).
			TestTrue(TEXT("premessa: al primo turno A1 si e' mosso verso la linea"), A1->Cell != PartenzaA1);
		}
		if (Giocati == 1)
		{
			TestTrue(TEXT("premessa: al secondo turno A2 ha fatto il suo passo normale"), bPassoA2 && A2->Cell == PassoA2);
		}
		++Giocati;
	}

	if (!TestTrue(TEXT("premessa: qualche turno e' stato giocato"), TM->GetPacingSamples().Num() > 0))
	{
		DestroyHexPacingWorld(World); return false;
	}

	int32 EventiTotali = 0;
	int32 TurniConEventi = 0;
	int32 BoundaryTotali = 0;
	int32 DisallineamentiRaccolta = 0;
	double RaccoltaMassimaMs = 0.0;
	double RaccoltaTotaleMs = 0.0;
	double BoundaryMassimoMs = 0.0;
	for (const FRTPacingSample& S : TM->GetPacingSamples())
	{
		EventiTotali += S.CandidatesPerEvent.Num();
		if (S.CandidatesPerEvent.Num() > 0) { ++TurniConEventi; }

		// 🔑 **L'invariante del cronometro della raccolta e' di LUNGHEZZA, non di valore** (`#2516`).
		// Le due registrazioni stanno nella stessa guardia al call site, quindi gli array crescono
		// insieme: verificarlo qui e' cio' che rende quella forma un'invariante invece di una
		// coincidenza, e un domani in cui qualcuno sposta una delle due `Add` diventa rosso.
		if (S.CandidateCollectionCpuMs.Num() != S.CandidatesPerEvent.Num()) { ++DisallineamentiRaccolta; }

		BoundaryTotali += S.BoundaryCpuMs.Num();
		for (double Ms : S.CandidateCollectionCpuMs)
		{
			RaccoltaTotaleMs += Ms;
			RaccoltaMassimaMs = FMath::Max(RaccoltaMassimaMs, Ms);
		}
		for (double Ms : S.BoundaryCpuMs)
		{
			BoundaryMassimoMs = FMath::Max(BoundaryMassimoMs, Ms);
		}
	}

	// 🔴 Con la registrazione tolta dal call site, questa riga cade e le altre no.
	TestTrue(*FString::Printf(
		TEXT("il resolver registra gli eventi di raccolta (%d eventi su %d turni)"),
		EventiTotali, TM->GetPacingSamples().Num()), EventiTotali > 0);
	TestTrue(TEXT("e non in un turno solo: con un guardiano armato la raccolta si ripete"),
		TurniConEventi > 1);

	// 🔴 Con uno dei due cronometri tolto dal call site, questa riga cade. E' il gemello della
	// riga sopra per `#2516`, e verifica la LUNGHEZZA perche' e' cio' che la forma garantisce: una
	// durata e' un numero che dipende dalla macchina, e asserirla renderebbe il gate un misuratore
	// di CPU invece che di cablaggio.
	TestEqual(TEXT("ogni raccolta e' cronometrata: i due array crescono insieme"),
		DisallineamentiRaccolta, 0);

	// 🔴 **E almeno un boundary si APRE davvero**, altrimenti `BoundaryCpuMs` resterebbe un array
	// vuoto e i suoi percentili sarebbero zero su una sessione intera, verdi e privi di senso. Il
	// confronto e' `> 0` e non un numero: quante volte un boundary si apra dipende dallo scenario, e
	// pinnarlo qui legherebbe il cablaggio a una fixture invece che al meccanismo.
	TestTrue(*FString::Printf(TEXT("almeno un boundary e' stato cronometrato (ne ho visti %d)"),
		BoundaryTotali), BoundaryTotali > 0);

	// ⚠️ **Le grandezze si STAMPANO, non si asseriscono.** Servivano a decidere l'unita' dei due
	// campi — `double` millisecondi invece di `int32` — e la decisione poggia su questi numeri invece
	// che su una stima. Restano visibili a ogni esecuzione: il giorno in cui la raccolta diventasse
	// cara, il numero e' gia' nel log e nessuno deve andarlo a cercare.
	AddInfo(FString::Printf(
		TEXT("[#2516] raccolte %d (totale %.4f ms, max %.4f ms) | boundary %d (max %.4f ms)"),
		EventiTotali, RaccoltaTotaleMs, RaccoltaMassimaMs, BoundaryTotali, BoundaryMassimoMs));

	DestroyHexPacingWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
