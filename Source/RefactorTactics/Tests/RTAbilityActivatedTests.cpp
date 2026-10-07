// IL MOMENTO: `AbilityActivated` (#3549, spec «il momento»).
//
// 🔑 **Cosa misura questo file**: che il resolver emetta UNA attivazione per intento, nel punto in cui lo
// accetta, nell'ordine che gia' ha. ⛔ **Non misura il playback** — quello e' `RTPlaybackActivationTests.cpp`
// — e non tocca golden, hash o archivi: la timeline non entra ne' in `StateHash` ne' nel TurnLog.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	URTHexMapAsset* SpawnAttivazioneMap(UWorld* World, int32 Radius = 8)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnAttivazioneUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** L'indice nel kit dell'abilita' con questo `ActionId`, letto dal CATALOGO dell'unita'. */
	int32 IndiceAbilitaAttivazione(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}

	/** Gli indici di timeline delle attivazioni, in ordine di timeline. */
	TArray<int32> IndiciAttivazioni(const ARTTurnManager* TM)
	{
		TArray<int32> Out;
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i = 0; i < T.Num(); ++i)
		{
			if (T[i].Type == ERTResolvedEventType::AbilityActivated) { Out.Add(i); }
		}
		return Out;
	}

	/** Il primo indice di timeline che soddisfa il predicato, `INDEX_NONE` se nessuno. */
	template <typename TPred>
	int32 PrimoIndiceAttivazione(const ARTTurnManager* TM, TPred Pred)
	{
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i = 0; i < T.Num(); ++i)
		{
			if (Pred(T[i])) { return i; }
		}
		return INDEX_NONE;
	}
}

/**
 * La Prep attiva ogni tipo di intento che accetta, nei suoi ordini — spec §2.2.
 *
 * 🔑 **Due ordini nella stessa fase**: l'Overwatch esce dal ciclo di raccolta, le istanze dal ciclo di
 * consumo dopo `SortActionInstances`. Il primo precede il secondo, ed e' l'ordine del resolver.
 * 🔴 **E lo stordimento non si attiva**: l'intento e' rifiutato, non accettato. Il controllo positivo e' lo
 * stesso turno senza stordimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnPrepActivatesEveryKindOfIntentTest,
	"RefactorTactics.Turn.PrepActivatesEveryKindOfIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnPrepActivatesEveryKindOfIntentTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bStordito, int32& OutScudi, int32& OutOverwatch, int32& OutStordito,
		int32& OutIndiceOverwatch, int32& OutIndiceScudo) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Scudo = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		ARTUnit* Sentinella = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		ARTUnit* Stordito = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Scudo || !Sentinella || !Stordito) { return false; }

		const int32 TideGuard = IndiceAbilitaAttivazione(Scudo, TEXT("Hero.Muiren.TideGuard"));
		if (!TestTrue(TEXT("premessa: Muiren ha TideGuard"), TideGuard != INDEX_NONE)) { return false; }
		Scudo->PlannedAbilityIndex = TideGuard;
		Scudo->PlannedCell = Scudo->Cell;

		Sentinella->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Sentinella, TEXT("Action.Overwatch"));
		Sentinella->PlannedCell = Sentinella->Cell;

		Stordito->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Stordito, TEXT("Action.Shield"));
		Stordito->PlannedCell = Stordito->Cell;
		if (bStordito) { Stordito->ApplyStatus(TAG_Status_Stunned, 2); }

		TM->LockInAndResolve();

		OutScudi = OutOverwatch = OutStordito = 0;
		OutIndiceOverwatch = OutIndiceScudo = INDEX_NONE;
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i : IndiciAttivazioni(TM))
		{
			const FRTResolvedEvent& Ev = T[i];
			TestEqual(TEXT("ogni attivazione di questo turno e' di fase Prep"),
				static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Prep));
			if (Ev.SourceStableUnitId == Scudo->StableUnitId) { ++OutScudi; OutIndiceScudo = i; }
			if (Ev.SourceStableUnitId == Sentinella->StableUnitId) { ++OutOverwatch; OutIndiceOverwatch = i; }
			if (Ev.SourceStableUnitId == Stordito->StableUnitId) { ++OutStordito; }
			if (Ev.SourceStableUnitId == Scudo->StableUnitId)
			{
				TestEqual(TEXT("in Prep il bersaglio e' chi usa l'azione"), Ev.TargetStableUnitId, Scudo->StableUnitId);
				TestEqual(TEXT("e l'azione e' quella del catalogo"), Ev.ActionId, FName(TEXT("Hero.Muiren.TideGuard")));
			}
		}
		return true;
	};

	int32 Scudi = 0, Overwatch = 0, Stordito = 0, IdxOw = INDEX_NONE, IdxScudo = INDEX_NONE;
	if (!GiraIlTurno(/*bStordito=*/ true, Scudi, Overwatch, Stordito, IdxOw, IdxScudo)) { return false; }
	TestEqual(TEXT("un'istanza di Prep: una attivazione"), Scudi, 1);
	TestEqual(TEXT("un Overwatch armato: una attivazione"), Overwatch, 1);
	TestEqual(TEXT("🔴 uno stordito non si attiva: l'intento e' rifiutato"), Stordito, 0);
	TestTrue(TEXT("l'Overwatch (ciclo di raccolta) precede l'istanza (ciclo di consumo)"), IdxOw < IdxScudo);

	// ⛔ Controllo positivo: senza stordimento la stessa unita' SI attiva. Senza, «zero» sopra sarebbe vero
	// anche per un'emissione che ignora `Action.Shield`.
	if (!GiraIlTurno(/*bStordito=*/ false, Scudi, Overwatch, Stordito, IdxOw, IdxScudo)) { return false; }
	TestEqual(TEXT("✅ senza stordimento la stessa unita' si attiva"), Stordito, 1);
	return true;
}

/**
 * Nel Dash l'attivazione PRECEDE il `Move` dello stesso scatto — D4, spec §2.2.
 *
 * 🔑 **E una carica che non entra in nessuna cella si attiva lo stesso**: il primo ciclo di `ResolveDash`
 * emetteva solo con `Resolved[i].Entered.Num() > 0`, e l'attivazione non deve dipendere dallo spostamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnDashActivationPrecedesItsMoveTest,
	"RefactorTactics.Turn.DashActivationPrecedesItsMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnDashActivationPrecedesItsMoveTest::RunTest(const FString&)
{
	// --- 1. Uno scatto che si muove: la disposizione di `Visual.Core.PhaseOrder` -----------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
		ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bersaglio || !Caricatore) { return false; }

		const int32 Ram = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
		if (!TestTrue(TEXT("premessa: Branth ha Ram"), Ram != INDEX_NONE)) { return false; }
		Caricatore->PlannedDashAbility = Ram;
		Caricatore->PlannedDashCell = Bersaglio->Cell;
		Caricatore->PlannedCell = Caricatore->Cell;

		TM->LockInAndResolve();

		const int32 Sid = Caricatore->StableUnitId;
		const int32 IdxAttivazione = PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::AbilityActivated && E.Phase == ERTMatchPhase::Dash
				&& E.SourceStableUnitId == Sid;
		});
		const int32 IdxMove = PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid;
		});
		if (!TestTrue(TEXT("⛔ premessa: lo scatto si e' mosso"), IdxMove != INDEX_NONE)) { return false; }
		if (!TestTrue(TEXT("lo scatto si e' attivato"), IdxAttivazione != INDEX_NONE)) { return false; }
		TestTrue(TEXT("🔴 l'attivazione PRECEDE il Move dello stesso scatto"), IdxAttivazione < IdxMove);
		TestEqual(TEXT("con l'azione dello scatto"),
			TM->ResolvedTimelineForTest()[IdxAttivazione].ActionId, FName(TEXT("Hero.Branth.Ram")));
	}

	// --- 2. Una carica che non entra in nessuna cella: il bersaglio e' gia' adiacente ------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
		ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bersaglio || !Caricatore) { return false; }

		Caricatore->PlannedDashAbility = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
		Caricatore->PlannedDashCell = Bersaglio->Cell;
		Caricatore->PlannedCell = Caricatore->Cell;

		TM->LockInAndResolve();

		const int32 Sid = Caricatore->StableUnitId;
		TestEqual(TEXT("⛔ premessa: nessun Move di fase Dash per chi era gia' adiacente"),
			PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
			{ return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid; }),
			static_cast<int32>(INDEX_NONE));
		TestTrue(TEXT("🔴 e l'attivazione c'e' lo stesso"),
			PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
			{ return E.Type == ERTResolvedEventType::AbilityActivated && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid; })
			!= INDEX_NONE);
	}
	return true;
}

/**
 * Una copertura di Prep si attiva solo se APPLICATA — `Ruling` di spec §2.2 (Review Focus 1).
 *
 * 🔑 Stessa unita', stesso piano, due esiti: con il bordo dichiarato `ResolveCoverStructures` la applica
 * (`CoverCreated`), senza bordo la rifiuta con `Reject` (`CoverRejected`, «nessun bordo dichiarato»). Un
 * rifiuto non e' un gesto accettato: zero attivazioni. Le premesse leggono il TurnLog, cosi' il «zero» non e'
 * vero per un piano che non e' mai arrivato alla funzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnCoverStructureActivatesOnlyWhenAppliedTest,
	"RefactorTactics.Turn.CoverStructureActivatesOnlyWhenApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnCoverStructureActivatesOnlyWhenAppliedTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConBordo, int32& OutAttivazioni, bool& OutCreata, bool& OutRifiutata) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Branth = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth) { return false; }

		const int32 Pannello = IndiceAbilitaAttivazione(Branth, TEXT("Hero.Branth.KineticPanel"));
		if (!TestTrue(TEXT("premessa: Branth ha KineticPanel"), Pannello != INDEX_NONE)) { return false; }
		Branth->PlannedAbilityIndex = Pannello;
		Branth->bAttackTargetsCell = true;
		Branth->PlannedAttackCell = FRTCellId(1, 0); // a portata: la portata del pannello e' 3 (#2283)
		Branth->bHasPlannedCoverEdge = bConBordo;
		Branth->PlannedCoverEdge = ERTHexDirection::E;
		Branth->PlannedCell = Branth->Cell;

		TM->LockInAndResolve();

		OutAttivazioni = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Branth->StableUnitId)
			{
				++OutAttivazioni;
				TestEqual(TEXT("di fase Prep"), static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Prep));
				TestEqual(TEXT("con l'azione del pannello"), Ev.ActionId, FName(TEXT("Hero.Branth.KineticPanel")));
			}
		}
		auto HaEsito = [TM](ERTEnvironmentOutcome Esito)
		{
			return TM->GetTurnLog().ContainsByPredicate([Esito](const FRTTurnLogEntry& E)
				{ return E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Esito); });
		};
		OutCreata = HaEsito(ERTEnvironmentOutcome::CoverCreated);
		OutRifiutata = HaEsito(ERTEnvironmentOutcome::CoverRejected);
		return true;
	};

	int32 Attivazioni = 0;
	bool bCreata = false, bRifiutata = false;
	if (!GiraIlTurno(/*bConBordo=*/ true, Attivazioni, bCreata, bRifiutata)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: con il bordo la copertura e' stata eretta"), bCreata)) { return false; }
	TestEqual(TEXT("🔴 applicata: una attivazione"), Attivazioni, 1);

	if (!GiraIlTurno(/*bConBordo=*/ false, Attivazioni, bCreata, bRifiutata)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: senza bordo il pannello e' stato rifiutato"), bRifiutata)) { return false; }
	TestEqual(TEXT("🔴 rifiutata: nessuna attivazione"), Attivazioni, 0);
	return true;
}

/**
 * La predittiva SENZA cella si attiva — `Ruling` di spec §2.2 (Review Focus 2): l'unita' ha speso l'azione,
 * anche se non arma niente (`RTTurnManager.cpp:4268`). La cella mirata e' allora quella di chi agisce.
 * ⛔ Controllo positivo: con la cella dichiarata si attiva anch'essa, e porta QUELLA cella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnPredictiveWithoutCellActivatesTest,
	"RefactorTactics.Turn.PredictiveWithoutCellStillActivates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnPredictiveWithoutCellActivatesTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConCella, TArray<FRTResolvedEvent>& OutAttivazioni, FRTCellId& OutCellaIvrin) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Ivrin = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin) { return false; }

		// `Hero.Ivrin.InterceptShot` e' la Predictive Action del roster (`PredictiveTargeting = LockCell`,
		// `Ability/RTHeroCatalogLibrary.cpp:950`).
		const int32 Intercetto = IndiceAbilitaAttivazione(Ivrin, TEXT("Hero.Ivrin.InterceptShot"));
		if (!TestTrue(TEXT("premessa: Ivrin ha InterceptShot"), Intercetto != INDEX_NONE)) { return false; }
		Ivrin->PlannedAbilityIndex = Intercetto;
		Ivrin->bAttackTargetsCell = bConCella;
		Ivrin->PlannedAttackCell = FRTCellId(2, 0);
		Ivrin->PlannedCell = Ivrin->Cell;
		OutCellaIvrin = Ivrin->Cell;

		TM->LockInAndResolve();

		OutAttivazioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated) { OutAttivazioni.Add(Ev); }
		}
		return true;
	};

	TArray<FRTResolvedEvent> Attivazioni;
	FRTCellId CellaIvrin;
	if (!GiraIlTurno(/*bConCella=*/ false, Attivazioni, CellaIvrin)) { return false; }
	if (TestEqual(TEXT("🔴 senza cella: una attivazione"), Attivazioni.Num(), 1))
	{
		TestEqual(TEXT("di fase Prep"), static_cast<int32>(Attivazioni[0].Phase), static_cast<int32>(ERTMatchPhase::Prep));
		TestEqual(TEXT("con l'azione predittiva"), Attivazioni[0].ActionId, FName(TEXT("Hero.Ivrin.InterceptShot")));
		TestEqual(TEXT("e la cella di chi agisce: nessuna previsione dichiarata"), Attivazioni[0].AimCell, CellaIvrin);
	}

	if (!GiraIlTurno(/*bConCella=*/ true, Attivazioni, CellaIvrin)) { return false; }
	if (TestEqual(TEXT("✅ con la cella: una attivazione"), Attivazioni.Num(), 1))
	{
		TestEqual(TEXT("✅ che porta la cella prevista"), Attivazioni[0].AimCell, FRTCellId(2, 0));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
