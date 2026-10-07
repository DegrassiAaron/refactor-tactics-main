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
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
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
 *
 * 🔑 **Il bersaglio dichiarato**: una carica porta quello sulla sua `PlannedDashCell` (sezione 1); uno scatto che
 * non e' una carica non ne dichiara nessuno e scrive `0` (sezione 3). ✅ Validato per mutazione: scrivere sempre `0`
 * fa cadere l'asserto della sezione 1 (e di `ChargeActivatesInDashNotInBlast`).
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
		TestEqual(TEXT("🔴 e il bersaglio dichiarato di una carica: l'unita' sulla cella pianificata"),
			TM->ResolvedTimelineForTest()[IdxAttivazione].TargetStableUnitId, Bersaglio->StableUnitId);
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

	// --- 3. Uno scatto che non e' una carica: nessun bersaglio dichiarato -------------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		// Un avversario in campo ma lontano dalla traiettoria: lo scatto non ha niente da dichiarare.
		ARTUnit* Altro = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 4));
		ARTUnit* Corridore = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Altro || !Corridore) { return false; }

		const int32 Scia = IndiceAbilitaAttivazione(Corridore, TEXT("Hero.Muiren.FluidTrail"));
		if (!TestTrue(TEXT("premessa: Muiren ha FluidTrail"), Scia != INDEX_NONE)) { return false; }
		Corridore->PlannedDashAbility = Scia;
		Corridore->PlannedDashCell = FRTCellId(2, 0);
		Corridore->PlannedCell = Corridore->Cell;

		TM->LockInAndResolve();

		const int32 Sid = Corridore->StableUnitId;
		const int32 IdxAttivazione = PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::AbilityActivated && E.Phase == ERTMatchPhase::Dash
				&& E.SourceStableUnitId == Sid;
		});
		if (!TestTrue(TEXT("⛔ premessa: lo scatto non-carica si e' attivato"), IdxAttivazione != INDEX_NONE)) { return false; }
		TestEqual(TEXT("🔴 uno scatto che non e' una carica non dichiara un bersaglio: 0"),
			TM->ResolvedTimelineForTest()[IdxAttivazione].TargetStableUnitId, 0);
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
 * Una copertura rifiutata IN APPLICAZIONE non si attiva — la meta' del `Ruling` che il test accanto non vede.
 *
 * 🔑 Il test accanto rifiuta nella RACCOLTA («nessun bordo dichiarato»), prima che la mappa venga toccata. Qui
 * il piano e' completo e valido, e a rifiutarlo e' `AddCover` quando applica: il bordo e' gia' riparato. Sono
 * due `continue` diversi, e un'emissione spostata prima del secondo non cade sul test accanto.
 * ⛔ La premessa legge il TurnLog: `CoverRejected` presente e `CoverCreated` assente, cosi' il «zero» non e' vero
 * per un piano che non e' mai arrivato all'applicazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnCoverStructureAlreadyRepairedDoesNotActivateTest,
	"RefactorTactics.Turn.CoverStructureAlreadyRepairedDoesNotActivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnCoverStructureAlreadyRepairedDoesNotActivateTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* Map = SpawnAttivazioneMap(World);
	if (!TestNotNull(TEXT("mappa di prova"), Map)) { return false; }

	ARTUnit* Branth = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Branth) { return false; }

	const int32 Pannello = IndiceAbilitaAttivazione(Branth, TEXT("Hero.Branth.KineticPanel"));
	if (!TestTrue(TEXT("premessa: Branth ha KineticPanel"), Pannello != INDEX_NONE)) { return false; }
	Branth->PlannedAbilityIndex = Pannello;
	Branth->bAttackTargetsCell = true;
	Branth->PlannedAttackCell = FRTCellId(1, 0);
	Branth->bHasPlannedCoverEdge = true;
	Branth->PlannedCoverEdge = ERTHexDirection::E;
	Branth->PlannedCell = Branth->Cell;

	// Il bordo che il pannello vuole erigere e' GIA' riparato: lo stesso piano valido del test accanto, ma
	// `AddCover` lo rifiutera' in applicazione.
	if (!TestTrue(TEXT("⛔ premessa: la copertura preesistente e' stata installata"),
		URTHexCoverLibrary::AddCover(Map, FRTCellId(1, 0), ERTHexDirection::E, ERTHexCoverType::Low, 30)))
	{
		return false;
	}

	TM->LockInAndResolve();

	auto HaEsito = [TM](ERTEnvironmentOutcome Esito)
	{
		return TM->GetTurnLog().ContainsByPredicate([Esito](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Esito); });
	};
	if (!TestTrue(TEXT("⛔ premessa: il pannello e' stato rifiutato"), HaEsito(ERTEnvironmentOutcome::CoverRejected))) { return false; }
	if (!TestFalse(TEXT("⛔ premessa: e nessuna copertura e' stata eretta"), HaEsito(ERTEnvironmentOutcome::CoverCreated))) { return false; }

	int32 Attivazioni = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Branth->StableUnitId) { ++Attivazioni; }
	}
	TestEqual(TEXT("🔴 rifiutata da AddCover: nessuna attivazione"), Attivazioni, 0);
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

/**
 * Un turno con un intento per fase produce UNA attivazione per fase — spec §5.1.
 *
 * 🔴 **La disposizione e' quella di `Visual.Core.PhaseOrder`, con tre intenti aggiunti**: lo scatto spende il
 * movimento (D-028), quindi Prep, Dash e Blast stanno su tre unita' diverse. `Action.Move` e `Action.Wait` ci
 * sono e non si attivano.
 * ⛔ **Controllo positivo**: una cura in piu' produce una quarta attivazione col suo `ActionId`. Senza, «tre»
 * sarebbe vero anche per un produttore che emette un numero fisso.
 * ✅ Validato per mutazione: togliere `EmitAttackIntentActivations(Ctx);` fa cadere il conteggio del Blast.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnAbilityActivatedOncePerIntentTest,
	"RefactorTactics.Turn.AbilityActivatedIsEmittedOncePerIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnAbilityActivatedOncePerIntentTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConCura, TArray<FRTResolvedEvent>& OutAttivazioni) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* F1 = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
		ARTUnit* R1 = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 1));
		ARTUnit* B1 = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		ARTUnit* V1 = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, -1));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !F1 || !R1 || !B1 || !V1) { return false; }

		R1->PlannedAbilityIndex = IndiceAbilitaAttivazione(R1, TEXT("Hero.Muiren.TideGuard")); // Prep
		R1->PlannedCell = FRTCellId(-1, 1);                                                       // e cammina
		B1->PlannedDashAbility = IndiceAbilitaAttivazione(B1, TEXT("Hero.Branth.Ram"));           // Dash
		B1->PlannedDashCell = F1->Cell;
		B1->PlannedCell = B1->Cell;
		V1->PlannedAbilityIndex = IndiceAbilitaAttivazione(V1, TEXT("Hero.Ivrin.PulseShot"));     // Blast
		V1->PlannedAttackTarget = F1;
		V1->PlannedCell = V1->Cell;
		// F1: `Action.Wait`, oppure — nel controllo positivo — una cura su se stessa.
		F1->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(F1, bConCura ? TEXT("Action.Heal") : TEXT("Action.Wait"));
		F1->PlannedCell = F1->Cell;
		if (!TestTrue(TEXT("premessa: i tre intenti del catalogo esistono"),
			R1->PlannedAbilityIndex != INDEX_NONE && B1->PlannedDashAbility != INDEX_NONE && V1->PlannedAbilityIndex != INDEX_NONE))
		{
			return false;
		}

		TM->LockInAndResolve();

		OutAttivazioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated) { OutAttivazioni.Add(Ev); }
		}
		auto Una = [&OutAttivazioni](ERTMatchPhase Fase, int32 Sorgente, const TCHAR* Azione)
		{
			return OutAttivazioni.FilterByPredicate([&](const FRTResolvedEvent& E)
				{ return E.Phase == Fase && E.SourceStableUnitId == Sorgente && E.ActionId == FName(Azione); }).Num();
		};
		TestEqual(TEXT("Prep: TideGuard, una volta"), Una(ERTMatchPhase::Prep, R1->StableUnitId, TEXT("Hero.Muiren.TideGuard")), 1);
		TestEqual(TEXT("Dash: Ram, una volta"), Una(ERTMatchPhase::Dash, B1->StableUnitId, TEXT("Hero.Branth.Ram")), 1);
		TestEqual(TEXT("Blast: PulseShot, una volta"), Una(ERTMatchPhase::Blast, V1->StableUnitId, TEXT("Hero.Ivrin.PulseShot")), 1);
		// ⛔ I due esclusi, con il loro SOGGETTO. ⏱️ *La prima stesura asseriva «nessuna attivazione porta
		// `Action.Move`»: vero per costruzione, perche' nessun sito emette con quell'id — un asserto senza
		// soggetto.* Ora: R1 cammina davvero (premessa: il suo `Move` di fase Move c'e') e la fase Move non ha
		// attivazioni; F1 ha `Action.Wait` nel piano e, nel giro senza cura, nessuna attivazione.
		const bool bR1Cammina = TM->ResolvedTimelineForTest().ContainsByPredicate([R1](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Move
				&& E.SourceStableUnitId == R1->StableUnitId;
		});
		if (!TestTrue(TEXT("⛔ premessa: R1 ha camminato nel Move"), bR1Cammina)) { return false; }
		TestEqual(TEXT("⛔ il Move normale non si attiva"), OutAttivazioni.FilterByPredicate(
			[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Move; }).Num(), 0);
		if (!bConCura)
		{
			TestEqual(TEXT("⛔ Action.Wait non si attiva"), OutAttivazioni.FilterByPredicate(
				[F1](const FRTResolvedEvent& E) { return E.SourceStableUnitId == F1->StableUnitId; }).Num(), 0);
		}
		return true;
	};

	TArray<FRTResolvedEvent> Attivazioni;
	if (!GiraIlTurno(/*bConCura=*/ false, Attivazioni)) { return false; }
	TestEqual(TEXT("🔴 tre intenti, tre attivazioni"), Attivazioni.Num(), 3);

	if (!GiraIlTurno(/*bConCura=*/ true, Attivazioni)) { return false; }
	TestEqual(TEXT("✅ controllo positivo: la cura e' la quarta"), Attivazioni.Num(), 4);
	TestTrue(TEXT("✅ e porta il suo ActionId"), Attivazioni.ContainsByPredicate([](const FRTResolvedEvent& E)
		{ return E.Phase == ERTMatchPhase::Blast && E.ActionId == FName(TEXT("Action.Heal")); }));
	return true;
}

/**
 * Le quattro sorgenti del Blast si attivano nell'ordine dei PASS — Cleanse, Heal, Arc, Attack (spec §2.2).
 * Un `ModifyArc` fuori portata non si attiva: il ramo esce prima di `PendingArcOps.Add`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnBlastActivatesAllFourSourcesTest,
	"RefactorTactics.Turn.BlastActivatesAllFourSources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnBlastActivatesAllFourSourcesTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bArcoInPortata, TArray<FName>& OutAzioni) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Purificatore = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Ferito       = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(1, 2));
		ARTUnit* Curatore     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 2));
		ARTUnit* Arcista      = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),
			bArcoInPortata ? FRTCellId(2, 2) : FRTCellId(-4, 0));
		ARTUnit* Tiratore     = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Purificatore || !Ferito || !Curatore || !Arcista || !Tiratore) { return false; }

		// Nel secondo giro la Cleanse non trova lo stato dichiarato: `NoEffect`, ma SPESA — e si attiva lo
		// stesso (decisione (c): il gesto, non l'esito).
		if (bArcoInPortata) { Purificatore->ApplyStatus(TAG_Status_Root, 3); }
		Purificatore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Purificatore, TEXT("Action.Cleanse"), 3);
		Purificatore->PlannedCleansePriority = { TAG_Status_Root };
		Purificatore->PlannedCell = Purificatore->Cell;

		Ferito->Health = FMath::Max(1, Ferito->Health - 30);
		const int32 Cura = RTAbilityFixtures::AddCoreAbility(Curatore, TEXT("Action.Heal"));
		Curatore->PlannedAbilityIndex = Cura;
		Curatore->PlannedAttackTarget = Ferito;
		Curatore->PlannedCell = Curatore->Cell;
		// Nel secondo giro la cura e' SENZA EFFETTO (`Amount <= 0`, il ramo `NoEffect` di `CollectHealActions`):
		// e' partita, spesa da `MarkAbilitySpent`, quindi si attiva lo stesso (decisione (c)).
		if (!bArcoInPortata)
		{
			for (FRTActionEffectSpec& Spec : Curatore->Abilities[Cura]->Def.Effects)
			{
				if (Spec.Effect == ERTActionEffect::Heal) { Spec.Amount = 0; }
			}
		}

		const int32 Arco = RTAbilityFixtures::AddCoreAbility(Arcista, TEXT("Action.ModifyArc"));
		Arcista->PlannedAbilityIndex = Arco;
		Arcista->PlannedAttackTarget = Ferito;
		Arcista->PlannedCell = Arcista->Cell;
		const bool bInPortata = URTHexLibrary::HexDistance(Arcista->Cell, Ferito->Cell)
			<= Arcista->GetAbility(Arco)->Def.RangeCells;
		if (!TestEqual(TEXT("premessa: la portata dell'arco e' quella che il caso dichiara"), bInPortata, bArcoInPortata))
		{
			return false;
		}

		Tiratore->PlannedAbilityIndex = IndiceAbilitaAttivazione(Tiratore, TEXT("Hero.Branth.ImpactShot"));
		Tiratore->PlannedAttackTarget = Purificatore;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->LockInAndResolve();

		if (!bArcoInPortata)
		{
			const bool bCuraVuota = TM->GetTurnLog().ContainsByPredicate([Curatore](const FRTTurnLogEntry& E)
			{
				return E.Category == ERTLogCategory::Fallback && E.UnitId == Curatore->StableUnitId
					&& E.ActionId == FName(TEXT("Action.Heal"))
					&& E.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect);
			});
			if (!TestTrue(TEXT("⛔ premessa: la cura senza effetto ha la sua voce NoEffect"), bCuraVuota)) { return false; }
		}

		OutAzioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == ERTMatchPhase::Blast)
			{
				OutAzioni.Add(Ev.ActionId);
			}
		}
		return true;
	};

	TArray<FName> Azioni;
	if (!GiraIlTurno(/*bArcoInPortata=*/ true, Azioni)) { return false; }
	const TArray<FName> Attese = { FName(TEXT("Action.Cleanse")), FName(TEXT("Action.Heal")),
		FName(TEXT("Action.ModifyArc")), FName(TEXT("Hero.Branth.ImpactShot")) };
	TestEqual(TEXT("🔴 quattro sorgenti, nell'ordine dei pass"), Azioni, Attese);

	if (!GiraIlTurno(/*bArcoInPortata=*/ false, Azioni)) { return false; }
	TestFalse(TEXT("⛔ un arco fuori portata non si attiva"), Azioni.Contains(FName(TEXT("Action.ModifyArc"))));
	TestTrue(TEXT("🔑 una cura senza effetto (Amount <= 0, NoEffect) si attiva: e' stata spesa"),
		Azioni.Contains(FName(TEXT("Action.Heal"))));
	TestTrue(TEXT("🔑 una Cleanse senza effetto (NoEffect) si attiva: e' stata spesa"),
		Azioni.Contains(FName(TEXT("Action.Cleanse"))));
	return true;
}

/**
 * Per ogni intento di Blast l'attivazione precede l'impronta e i colpi con la stessa `(Source, ActionId)`.
 * ⛔ Nulla sui `StructureHit`: in timeline PRECEDONO l'attivazione (spec §2.2), li riordina solo la sequenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnActivationPrecedesFootprintAndHitsTest,
	"RefactorTactics.Turn.AbilityActivatedPrecedesItsFootprintAndHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnActivationPrecedesFootprintAndHitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World, 12);

	ARTUnit* Attaccante = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Bersaglio  = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { return false; }
	Attaccante->PlannedAbilityIndex = 0;
	Attaccante->PlannedAttackTarget = Bersaglio;
	Attaccante->PlannedCell = FRTCellId(2, 3);

	TM->LockInAndResolve();

	const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
	int32 Confronti = 0;
	for (int32 a = 0; a < T.Num(); ++a)
	{
		if (T[a].Type != ERTResolvedEventType::AbilityActivated || T[a].Phase != ERTMatchPhase::Blast) { continue; }
		for (int32 j = 0; j < T.Num(); ++j)
		{
			const bool bStessoAtto = T[j].SourceStableUnitId == T[a].SourceStableUnitId && T[j].ActionId == T[a].ActionId;
			const bool bImprontaOColpo = T[j].Type == ERTResolvedEventType::AttackFootprint
				|| T[j].Type == ERTResolvedEventType::Attack;
			if (bStessoAtto && bImprontaOColpo)
			{
				TestTrue(*FString::Printf(TEXT("attivazione %d prima dell'evento %d dello stesso atto"), a, j), a < j);
				++Confronti;
			}
		}
	}
	// ⛔ ANTI-VACUITA': almeno un'impronta e un colpo confrontati, o il ciclo e' vero per assenza.
	TestTrue(TEXT("⛔ almeno due confronti (impronta e colpo)"), Confronti >= 2);
	return true;
}

/**
 * La carica si attiva nel Dash e NON di nuovo nel Blast: gli impatti entrano con `IntentAbilityIndex =
 * INDEX_NONE` e `IntentDefs = Impact.Def`, che e' il `Def` dello scatto (`AppendChargeImpactIntents`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnChargeActivatesInDashNotInBlastTest,
	"RefactorTactics.Turn.ChargeActivatesInDashNotInBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnChargeActivatesInDashNotInBlastTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World);

	ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
	ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bersaglio || !Caricatore) { return false; }
	Caricatore->PlannedDashAbility = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->LockInAndResolve();

	const FName Ram(TEXT("Hero.Branth.Ram"));
	int32 Attivazioni = 0, ColpiDiImpatto = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.ActionId == Ram)
		{
			++Attivazioni;
			TestEqual(TEXT("l'attivazione della carica e' di fase Dash"),
				static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Dash));
			TestEqual(TEXT("🔴 e porta il bersaglio dichiarato: la vittima della carica"),
				Ev.TargetStableUnitId, Bersaglio->StableUnitId);
		}
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.ActionId == Ram
			&& Ev.SourceStableUnitId == Caricatore->StableUnitId && Ev.Phase == ERTMatchPhase::Blast)
		{
			++ColpiDiImpatto;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: l'impatto ha colpito nel Blast con la chiave dello scatto"), ColpiDiImpatto > 0))
	{
		return false;
	}
	TestEqual(TEXT("🔴 una sola attivazione: Dash, non anche Blast"), Attivazioni, 1);
	return true;
}

/**
 * Interrotto: nessuna attivazione. Bloccato dalla linea di tiro: SI attiva (`Ruling`: il gesto, non l'esito).
 * Morto prima del Blast: nessuna (guardia `IsAlive()` di `ApplyInterrupts` e di `EmitAttackIntentActivations`). Fuori portata: nessuna,
 * perche' `Fallback Cancelled` lo toglie prima che diventi un intento del Blast.
 * ✅ Validato per mutazione: tenere `InterruptedIntents` locale in `ApplyInterrupts` fa cadere il primo ramo;
 * togliere la guardia `IsAlive()` da `EmitAttackIntentActivations` fa cadere il terzo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnInterruptedOrDeadDoesNotActivateTest,
	"RefactorTactics.Turn.InterruptedOrDeadIntentDoesNotActivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnInterruptedOrDeadDoesNotActivateTest::RunTest(const FString&)
{
	auto AttivazioniDi = [](const ARTTurnManager* TM, const ARTUnit* U)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == U->StableUnitId) { ++N; }
		}
		return N;
	};

	// --- 1. Interrotto (montaggio dei test di Interrupt in `RTControlActionTests.cpp`, distanze di UNA cella) ----------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 6);
		ARTUnit* Interruttore = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0));
		ARTUnit* Attaccante   = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 0));
		ARTUnit* Vittima      = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(5, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Interruttore || !Attaccante || !Vittima) { return false; }

		Interruttore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Interruttore, TEXT("Action.Interrupt"));
		Interruttore->PlannedAttackTarget = Attaccante;
		Interruttore->PlannedCell = Interruttore->Cell;
		Attaccante->PlannedAbilityIndex = 0;
		Attaccante->PlannedAttackTarget = Vittima;
		Attaccante->PlannedCell = Attaccante->Cell;

		TM->LockInAndResolve();

		const bool bCancellato = TM->GetTurnLog().ContainsByPredicate([Attaccante](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback && E.UnitId == Attaccante->StableUnitId
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
		});
		if (!TestTrue(TEXT("⛔ premessa: l'azione e' stata CANCELLATA, non degradata"), bCancellato)) { return false; }
		TestEqual(TEXT("🔴 l'interrotto non si attiva"), AttivazioniDi(TM, Attaccante), 0);
		TestEqual(TEXT("✅ l'interruttore si'"), AttivazioniDi(TM, Interruttore), 1);
	}

	// --- 2. Linea di tiro bloccata da un muro alto: il gesto c'e' stato --------------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		URTHexMapAsset* Map = SpawnAttivazioneMap(World, 6);
		ARTUnit* Tiratore  = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Map || !Tiratore || !Bersaglio) { return false; }

		// Il bordo verso (1,0) e' l'indice di quella cella nell'anello: la stessa convenzione di
		// `ResolveCoverStructures` (`Ring[EdgeIndex]`).
		const TArray<FRTCellId> Anello = URTHexLibrary::Neighbors(FRTCellId(0, 0));
		const int32 Bordo = Anello.IndexOfByKey(FRTCellId(1, 0));
		if (!TestTrue(TEXT("premessa: (1,0) e' un vicino di (0,0)"), Bordo != INDEX_NONE)) { return false; }
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 0), static_cast<ERTHexDirection>(Bordo), ERTHexCoverType::High, 100);

		Tiratore->PlannedAbilityIndex = IndiceAbilitaAttivazione(Tiratore, TEXT("Hero.Ivrin.PulseShot"));
		Tiratore->PlannedAttackTarget = Bersaglio;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->LockInAndResolve();

		int32 Colpi = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Attack && Ev.SourceStableUnitId == Tiratore->StableUnitId) { ++Colpi; }
		}
		if (!TestEqual(TEXT("⛔ premessa: il muro ha fermato il colpo"), Colpi, 0)) { return false; }
		TestEqual(TEXT("🔑 bloccato dalla linea di tiro: SI attiva"), AttivazioniDi(TM, Tiratore), 1);
	}

	// --- 3. Morto prima del Blast: il piano gli resta addosso, la guardia no ----------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 6);
		ARTUnit* Morto     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, 0));
		// ⚠️ **Un compagno vivo**, lontano: senza, la squadra del morto e' estinta e il turno non arriva al Blast —
		// il ramo sarebbe vacuo, e togliere la guardia `IsAlive()` lascerebbe il test verde.
		ARTUnit* Compagno  = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-5, 3));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Morto || !Bersaglio || !Compagno) { return false; }
		Morto->PlannedAbilityIndex = 0;
		Morto->PlannedAttackTarget = Bersaglio;
		Morto->PlannedCell = Morto->Cell;
		// Come i test di Interrupt in `RTControlActionTests.cpp`: conta la guardia del resolver, non il modo in cui l'unita'
		// e' caduta — in Prep e nel Dash nessun danno si applica prima del Blast.
		Morto->Health = 0;

		TM->LockInAndResolve();
		TestEqual(TEXT("🔴 chi e' morto prima del Blast non si attiva"), AttivazioniDi(TM, Morto), 0);
	}

	// --- 4. Fuori portata: `Fallback Cancelled` PRIMA di entrare nel Blast (decisione (6), IMPLEMENTATION DRIFT)
	//
	// `CollectAttackIntents` manda a `ApplyFallback` ogni motivo che non sia `NoLineOfSight`/`NoMap`, e con un
	// `Cancel` fa `continue` prima di `Intents.Add` (`CollectAttackIntents`): l'intento non esiste
	// nel Blast, quindi non si attiva. Il rifiuto lo racconta il TurnLog — ed e' la premessa del ramo.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 8);
		ARTUnit* Lontano   = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(6, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Lontano || !Bersaglio) { return false; }
		const int32 Colpo = IndiceAbilitaAttivazione(Lontano, TEXT("Hero.Branth.ImpactShot"));
		if (!TestTrue(TEXT("premessa: il bersaglio e' oltre la portata"), Colpo != INDEX_NONE
			&& URTHexLibrary::HexDistance(Lontano->Cell, Bersaglio->Cell) > Lontano->GetAbility(Colpo)->Def.RangeCells))
		{
			return false;
		}
		Lontano->PlannedAbilityIndex = Colpo;
		Lontano->PlannedAttackTarget = Bersaglio;
		Lontano->PlannedCell = Lontano->Cell;

		TM->LockInAndResolve();

		const bool bAnnullato = TM->GetTurnLog().ContainsByPredicate([Lontano](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback && E.UnitId == Lontano->StableUnitId
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange);
		});
		if (!TestTrue(TEXT("⛔ premessa: il TurnLog registra Fallback Cancelled per fuori portata"), bAnnullato))
		{
			return false;
		}
		TestEqual(TEXT("🔴 fuori portata: nessuna attivazione"), AttivazioniDi(TM, Lontano), 0);
	}
	return true;
}

/**
 * Un intento di un'abilita' LEGACY, senza `ActionId`, non si attiva — e non fa scattare l'`ensureMsgf` dell'helper.
 *
 * `CollectAttackIntents` ammette abilita' senza `ActionId` (`Instance.Def.ActionId.IsNone()` e' un caso
 * previsto li'), e `ARTUnit::EnsureDefaultAbilities` ne crea tre — «Attacco», «Colpo pesante», «Ultimate» — con
 * `MakeAbility`. D1 dice «ogni intento CON un `ActionId`»: senza la guardia in `EmitAttackIntentActivations`
 * l'helper emetterebbe un `ensureMsgf` per ogni colpo di un archetipo legacy.
 * ⛔ **Anti-vacuita'**: la premessa e' che il colpo AVVENGA (un `Attack` in timeline) e che l'abilita' sia
 * davvero senza `ActionId`; «zero attivazioni» da solo sarebbe vero anche se l'intento non esistesse.
 * ✅ Validato per mutazione: togliere la guardia `ActionId.IsNone()` fa scattare l'ensure e cadere il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLegacyIntentWithoutActionIdTest,
	"RefactorTactics.Turn.LegacyIntentWithoutActionIdDoesNotActivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLegacyIntentWithoutActionIdTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World);

	// Un'unita' SENZA eroe: e' `BeginPlay` -> `EnsureDefaultAbilities` a darle il kit legacy. `DispatchBeginPlay`
	// non e' decorativo: senza, `Abilities` resta vuoto e il test non avrebbe nessuna abilita' da pianificare.
	ARTUnit* Legacy = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("unita' legacy"), Legacy)) { return false; }
	Legacy->TeamId = 0;
	Legacy->bIsBotControlled = false;
	UGameplayStatics::FinishSpawningActor(Legacy, FTransform::Identity);
	Legacy->DispatchBeginPlay();
	Legacy->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bersaglio) { return false; }

	const URTActionData* Attacco = Legacy->GetAbility(0);
	if (!TestTrue(TEXT("premessa: l'abilita' legacy esiste, e non porta un ActionId"),
		Attacco != nullptr && Attacco->Def.ActionId.IsNone()))
	{
		return false;
	}
	if (!TestTrue(TEXT("premessa: il bersaglio e' in portata"),
		URTHexLibrary::HexDistance(Legacy->Cell, Bersaglio->Cell) <= Attacco->RangeCells))
	{
		return false;
	}
	// ⚠️ **Il colpo va ABILITATO a mano**: `MakeAbility` lascia `Def.bCountsAsAttack` a `false`, e
	// `CollectHexAttacks` scarta un intento che non conta come attacco — l'abilita' legacy cosi' com'e' produce
	// l'intento ma nessun `Attack`, quindi un asserto sul colpo sarebbe impossibile e uno sul solo conteggio
	// delle attivazioni vacuo. Con il flag acceso l'intento ha un colpo osservabile e continua a non avere
	// `ActionId`: e' proprio la combinazione che la guardia deve tenere fuori dall'helper.
	Legacy->Abilities[0]->Def.bCountsAsAttack = true;
	Legacy->PlannedAbilityIndex = 0;
	Legacy->PlannedAttackTarget = Bersaglio;
	Legacy->PlannedCell = Legacy->Cell;

	TM->LockInAndResolve();

	int32 Colpi = 0, Attivazioni = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.SourceStableUnitId != Legacy->StableUnitId) { continue; }
		if (Ev.Type == ERTResolvedEventType::Attack) { ++Colpi; }
		if (Ev.Type == ERTResolvedEventType::AbilityActivated) { ++Attivazioni; }
	}
	if (!TestTrue(TEXT("⛔ premessa: il colpo legacy e' avvenuto (un Attack in timeline)"), Colpi >= 1)) { return false; }
	TestEqual(TEXT("🔴 un intento senza ActionId non si attiva"), Attivazioni, 0);
	return true;
}

/**
 * Il verdetto di [D-223] si congela all'emissione: la squadra della sorgente vede la propria attivazione, una
 * squadra assente dalla partita no (fail-closed). Spec §2.1 e §2.5.
 *
 * ⚠️ **Precondizione dichiarata**: `FreezeVerdict` itera solo le squadre presenti in `TeamKnowledgeState`, e il
 * Blast lo rinfresca in testa (`RefreshTeamKnowledgeForBlast`). Per la Prep serve il refresh di pianificazione:
 * qui lo si chiama esplicitamente, ed e' la stessa precondizione delle righe di log.
 * ✅ Validato per mutazione: togliere `Ev.SourceVerdict = ...` da `EmitAbilityActivated` fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnActivationFreezesSourceVerdictTest,
	"RefactorTactics.Turn.AbilityActivatedFreezesTheSourceVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnActivationFreezesSourceVerdictTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World);

	ARTUnit* Scudo     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
	ARTUnit* Tiratore  = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Scudo || !Tiratore || !Bersaglio) { return false; }

	Scudo->PlannedAbilityIndex = IndiceAbilitaAttivazione(Scudo, TEXT("Hero.Muiren.TideGuard"));
	Scudo->PlannedCell = Scudo->Cell;
	Tiratore->PlannedAbilityIndex = 0;
	Tiratore->PlannedAttackTarget = Bersaglio;
	Tiratore->PlannedCell = Tiratore->Cell;

	TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep
	TM->LockInAndResolve();

	const int32 SquadraAssente = 7; // nessuna unita' in campo: fuori da `TeamKnowledgeState`
	int32 Viste = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type != ERTResolvedEventType::AbilityActivated) { continue; }
		++Viste;
		TestTrue(*FString::Printf(TEXT("%s: la squadra della sorgente la vede"), *Ev.ActionId.ToString()),
			Ev.SourceVerdict.AllowsTeam(0));
		TestFalse(*FString::Printf(TEXT("%s: ⛔ una squadra assente no (fail-closed)"), *Ev.ActionId.ToString()),
			Ev.SourceVerdict.AllowsTeam(SquadraAssente));
	}
	TestEqual(TEXT("⛔ anti-vacuita': le due attivazioni, Prep e Blast"), Viste, 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
