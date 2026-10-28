// Le ricariche dedotte di un nemico (`#3751`, fetta F6 della spec dell'ispettore del mondo): il registro di cio'
// che una squadra ha visto usare, e la riga della scheda che ne deriva.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: l'azione cancellata da un Interrupt
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTSeenUseLedger.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h" // FRTAbilityCooldownView: il tipo che porta la ricarica VERA, controllo positivo della scansione
#include "UI/RTInspect.h"
#include "UI/RTPlayerEventProjector.h"
#include "Unit/RTUnit.h"
#include "UObject/UnrealType.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	constexpr int32 SeenUnit = 7;
	constexpr int32 SeenOtherUnit = 8;
	constexpr int32 SeenObserverTeam = 1;

	FRTKnowledgeVerdict SeenOnlyTeam(int32 TeamId)
	{
		FRTKnowledgeVerdict Verdict;
		Verdict.AllowTeam(TeamId);
		return Verdict;
	}

	/** Un colpo dato da `SeenUnit` con `ActionId`, nel turno `Turn`, letto dalla sola squadra osservatrice. */
	FRTTurnLogEntry SeenHit(FName ActionId = TEXT("Hero.Test.Shot"), int32 Turn = 3)
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Combat;
		Entry.Outcome = static_cast<uint8>(ERTCombatOutcome::Hit);
		Entry.SrcCell = FRTCellId(-1, 0, 0);
		Entry.TgtCell = FRTCellId(0, 0, 0);
		Entry.Amount = 10;
		Entry.ActionId = ActionId;
		Entry.UnitId = SeenUnit;
		Entry.TurnNumber = Turn;
		Entry.Verdict = SeenOnlyTeam(SeenObserverTeam);
		return Entry;
	}

	int32 SeenAfter(const FRTTurnLogEntry& Entry, FName ActionId = TEXT("Hero.Test.Shot"), int32 UnitId = SeenUnit)
	{
		FRTSeenUseLedger Ledger;
		Ledger.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam });
		return Ledger.LastSeenTurn(SeenObserverTeam, UnitId, ActionId);
	}

	FString SeenText(const FRTInspectView& View) { return FString::Join(View.Lines, TEXT("\n")); }
}

// Una voce conta solo se il suo soggetto e' chi AGISCE. I casi che P4 chiede di escludere hanno ciascuno il suo
// controllo, e il controllo positivo sta accanto: senza, «nessuna riga» non distingue un predicato giusto da uno spento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseOnlyTheActorCountsTest,
	"RefactorTactics.SeenUse.OnlyAUseByItsSubjectCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseOnlyTheActorCountsTest::RunTest(const FString&)
{
	// Controllo positivo: il colpo dato da `SeenUnit` lo accredita a lui, nel turno della voce.
	TestEqual(TEXT("un colpo dato: l'ultimo turno visto e' quello della voce"), SeenAfter(SeenHit()), 3);

	// Danno ambientale: `UnitId` e' chi SUBISCE, e l'azione e' la causa, non di chi la subisce.
	{
		FRTTurnLogEntry Burning = SeenHit(TAG_Status_Burning.GetTag().GetTagName());
		Burning.SrcCell = Burning.TgtCell;
		TestTrue(TEXT("premessa: la tassonomia la legge come danno ambientale"), URTTurnLogLibrary::IsEnvironmentalDamage(Burning));
		TestEqual(TEXT("danno ambientale: chi lo subisce non ha agito"),
			SeenAfter(Burning, TAG_Status_Burning.GetTag().GetTagName()), 0);
	}

	// L'azione cancellata da un `Interrupt`: `UnitId` e' chi l'ha subita (#1437).
	{
		FRTTurnLogEntry Interrupted = SeenHit();
		Interrupted.Category = ERTLogCategory::Fallback;
		Interrupted.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
		Interrupted.Amount = static_cast<int32>(ERTActionInvalidReason::Interrupted);
		TestTrue(TEXT("premessa: e' una voce di chi SUBISCE"), URTTurnLogLibrary::IsSubjectTheSufferer(Interrupted));
		TestEqual(TEXT("azione cancellata: la subisce, non l'ha usata"), SeenAfter(Interrupted), 0);
	}

	// Una guardia scavalcata da un colpo alle spalle: la voce descrive il DIFENSORE.
	{
		FRTTurnLogEntry Bypassed = SeenHit();
		Bypassed.Category = ERTLogCategory::Facing;
		Bypassed.Outcome = static_cast<uint8>(ERTFacingOutcome::RearHitBypassedGuard);
		TestEqual(TEXT("guardia scavalcata: il difensore non ha agito"), SeenAfter(Bypassed), 0);
	}

	// 🔴 Lo spostamento forzato porta in `UnitId` chi viene spostato e in `ActionId` l'azione di chi spinge (#307):
	// il caso che un test su `UnitId` + `ActionId` accrediterebbe alla vittima.
	{
		FRTTurnLogEntry Pushed = SeenHit(TEXT("Hero.Test.Push"));
		Pushed.Category = ERTLogCategory::Move;
		Pushed.Outcome = static_cast<uint8>(ERTMoveOutcome::Displaced);
		TestEqual(TEXT("spostato da un altro: l'azione di chi spinge non e' sua"),
			SeenAfter(Pushed, TEXT("Hero.Test.Push")), 0);
	}

	// Uno stato porta il TAG in `ActionId`: non e' un'azione del kit, e non accredita niente.
	{
		FRTTurnLogEntry StatusBirth = SeenHit(TAG_Status_Burning.GetTag().GetTagName());
		StatusBirth.Category = ERTLogCategory::Status;
		StatusBirth.Outcome = static_cast<uint8>(ERTStatusOutcome::AppliedByAction);
		StatusBirth.SrcCell = StatusBirth.TgtCell;
		TestEqual(TEXT("uno stato nato non e' un uso"), SeenAfter(StatusBirth, TAG_Status_Burning.GetTag().GetTagName()), 0);
	}

	// Un attacco fermato dalla copertura: la ricarica non e' detto che sia stata pagata, e la deduzione tace.
	{
		FRTTurnLogEntry Stopped = SeenHit();
		Stopped.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
		TestEqual(TEXT("senza linea di tiro: nessuna riga"), SeenAfter(Stopped), 0);
	}

	// La cura ha un agente vero, anche se non e' danno. Controllo positivo dell'unione.
	{
		FRTTurnLogEntry Healed = SeenHit(TEXT("Hero.Test.Heal"), 5);
		Healed.Outcome = static_cast<uint8>(ERTCombatOutcome::Healed);
		TestEqual(TEXT("una cura data a un altro e' un uso"), SeenAfter(Healed, TEXT("Hero.Test.Heal")), 5);

		// Il limite dichiarato nella spec: una cura su se stessi ha le due celle uguali, e `IsSubjectTheSufferer` la
		// legge come danno ambientale. La deduzione tace invece di accreditare un uso che non sa classificare.
		FRTTurnLogEntry SelfHeal = Healed;
		SelfHeal.TgtCell = SelfHeal.SrcCell;
		TestEqual(TEXT("una cura su se stessi non lascia riga (limite dichiarato)"), SeenAfter(SelfHeal, TEXT("Hero.Test.Heal")), 0);
	}

	// Senza soggetto, azione o turno la voce non dice chi ha fatto che cosa e quando.
	{
		FRTTurnLogEntry NoUnit = SeenHit(); NoUnit.UnitId = 0;
		FRTTurnLogEntry NoAction = SeenHit(NAME_None);
		FRTTurnLogEntry NoTurn = SeenHit(TEXT("Hero.Test.Shot"), 0);
		TestEqual(TEXT("senza unita': niente"), SeenAfter(NoUnit, TEXT("Hero.Test.Shot"), 0), 0);
		TestEqual(TEXT("senza azione: niente"), SeenAfter(NoAction, NAME_None), 0);
		TestEqual(TEXT("senza turno: niente"), SeenAfter(NoTurn), 0);
	}
	return true;
}

// Chi legge e' chi il verdetto autorizza; il turno piu' alto vince; e le coppie non si mescolano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseAuthorizedTeamsTest,
	"RefactorTactics.SeenUse.OnlyAuthorizedTeamsLearnAndTheLatestTurnWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseAuthorizedTeamsTest::RunTest(const FString&)
{
	const TArray<int32> Teams = { 0, SeenObserverTeam };

	// Un uso FUORI VISTA: stessa voce, verdetto che non autorizza nessuna delle due squadre.
	{
		FRTTurnLogEntry Unseen = SeenHit();
		Unseen.Verdict = FRTKnowledgeVerdict::NoOne();
		TestEqual(TEXT("uso fuori vista: nessuna riga"), SeenAfter(Unseen), 0);
	}

	// Una squadra sola e' autorizzata: l'altra non ne sa nulla.
	{
		FRTSeenUseLedger Ledger;
		Ledger.Observe(SeenHit(), Teams);
		TestEqual(TEXT("la squadra autorizzata ha visto"), Ledger.LastSeenTurn(SeenObserverTeam, SeenUnit, TEXT("Hero.Test.Shot")), 3);
		TestEqual(TEXT("l'altra squadra no"), Ledger.LastSeenTurn(0, SeenUnit, TEXT("Hero.Test.Shot")), 0);
		TestEqual(TEXT("una squadra non osservatrice non e' mai registrata"), Ledger.Num(), 1);
	}

	// Il turno piu' alto vince, e una voce vecchia non fa tornare indietro la stima.
	{
		FRTSeenUseLedger Ledger;
		Ledger.Observe(SeenHit(TEXT("Hero.Test.Shot"), 3), Teams);
		Ledger.Observe(SeenHit(TEXT("Hero.Test.Shot"), 6), Teams);
		Ledger.Observe(SeenHit(TEXT("Hero.Test.Shot"), 4), Teams);
		TestEqual(TEXT("l'ultimo turno visto e' il piu' alto"), Ledger.LastSeenTurn(SeenObserverTeam, SeenUnit, TEXT("Hero.Test.Shot")), 6);
		TestEqual(TEXT("e le tre voci sono UNA coppia"), Ledger.Num(), 1);
	}

	// Coppie distinte: un'altra unita' e un'altra azione non si mescolano.
	{
		FRTSeenUseLedger Ledger;
		FRTTurnLogEntry OtherUnit = SeenHit(TEXT("Hero.Test.Shot"), 5); OtherUnit.UnitId = SeenOtherUnit;
		Ledger.Observe(SeenHit(TEXT("Hero.Test.Shot"), 3), Teams);
		Ledger.Observe(OtherUnit, Teams);
		Ledger.Observe(SeenHit(TEXT("Hero.Test.Blast"), 4), Teams);
		TestEqual(TEXT("la prima unita' vede il proprio turno"), Ledger.LastSeenTurn(SeenObserverTeam, SeenUnit, TEXT("Hero.Test.Shot")), 3);
		TestEqual(TEXT("l'altra unita' il suo"), Ledger.LastSeenTurn(SeenObserverTeam, SeenOtherUnit, TEXT("Hero.Test.Shot")), 5);
		TestEqual(TEXT("un'altra azione la sua"), Ledger.LastSeenTurn(SeenObserverTeam, SeenUnit, TEXT("Hero.Test.Blast")), 4);
		TestEqual(TEXT("un'azione mai vista no"), Ledger.LastSeenTurn(SeenObserverTeam, SeenUnit, TEXT("Hero.Test.Other")), 0);
	}
	return true;
}

// `Turn/` non puo' includere `UI/`, quindi il predicato di autorizzazione e' scritto una seconda volta: questo test
// lo tiene allineato al filtro del registro a schermo, sulle stesse voci.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseAgreesWithTheEventFilterTest,
	"RefactorTactics.SeenUse.AuthorizationAgreesWithThePlayerEventFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseAgreesWithTheEventFilterTest::RunTest(const FString&)
{
	int32 Authorized = 0;
	int32 Refused = 0;
	for (const FRTKnowledgeVerdict& Verdict : { FRTKnowledgeVerdict::NoOne(), FRTKnowledgeVerdict::Everyone(),
		SeenOnlyTeam(0), SeenOnlyTeam(SeenObserverTeam) })
	{
		for (const int32 Team : { 0, SeenObserverTeam })
		{
			FRTTurnLogEntry Entry = SeenHit();
			Entry.Verdict = Verdict;
			FRTSeenUseLedger Ledger;
			Ledger.Observe(Entry, TArray<int32>{ Team });
			const bool bLedger = Ledger.LastSeenTurn(Team, SeenUnit, TEXT("Hero.Test.Shot")) > 0;
			const bool bFilter = URTPlayerEventProjector::IsAuthorized(Entry, Team);
			TestEqual(TEXT("il registro impara solo cio' che il filtro del registro a schermo autorizza"), bLedger, bFilter);
			(bFilter ? Authorized : Refused)++;
		}
	}
	TestTrue(TEXT("il controllo ha visto sia voci autorizzate sia rifiutate"), Authorized > 0 && Refused > 0);
	return true;
}

// La stima e' il conteggio vero: lo si prova contro `ConsumeAbility`/`TickCooldowns`, non contro una formula riscritta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseMatchesTheRealCountdownTest,
	"RefactorTactics.UI.InspectDeducedCooldownMatchesTheRealCountdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseMatchesTheRealCountdownTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	int32 Examined = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (Unit == nullptr) { continue; }
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->DispatchBeginPlay(); // `AbilityCooldowns` si allinea al kit qui: senza, ConsumeAbility non scrive

		for (int32 Index = 0; Index < Unit->NumAbilities(); ++Index)
		{
			const URTActionData* Action = Unit->GetAbility(Index);
			if (Action == nullptr || Action->CooldownTurns <= 0) { continue; }
			++Examined;

			const int32 UsedAtTurn = 5; // il turno T in cui la voce e' stata vista
			FRTInspectDeducedCooldown Row;
			if (!TestTrue(TEXT("un'azione con ricarica e vista usare ha una riga"),
				URTInspectLibrary::MakeDeducedCooldown(Action->Def.ActionId, FText::FromString(TEXT("T")),
					Action->CooldownTurns, UsedAtTurn, Row)))
			{
				continue;
			}

			// La meccanica vera: la ricarica si paga durante la risoluzione del turno T, e ogni pianificazione e'
			// preceduta dal Cleanup del turno prima, che ne toglie uno. Pianificando il turno `AvailableFromTurn`
			// l'azione deve essere usabile, e pianificando uno qualunque dei turni prima no.
			Unit->ConsumeAbility(Index);
			if (!TestEqual(TEXT("premessa: ConsumeAbility ha scritto la ricarica"), Unit->GetAbilityCooldown(Index), Action->CooldownTurns))
			{
				continue;
			}
			for (int32 PlanningTurn = UsedAtTurn + 1; PlanningTurn <= Row.AvailableFromTurn; ++PlanningTurn)
			{
				Unit->TickCooldowns(); // il Cleanup del turno PlanningTurn - 1
				if (PlanningTurn < Row.AvailableFromTurn)
				{
					TestTrue(FString::Printf(TEXT("%s: pianificando il turno %d e' ancora in ricarica"),
						*Action->Def.ActionId.ToString(), PlanningTurn), Unit->GetAbilityCooldown(Index) > 0);
				}
				else
				{
					TestEqual(FString::Printf(TEXT("%s: pianificando il turno %d la ricarica e' finita"),
						*Action->Def.ActionId.ToString(), PlanningTurn), Unit->GetAbilityCooldown(Index), 0);
				}
			}
		}
		Unit->Destroy();
	}
	TestTrue(TEXT("il controllo ha esaminato azioni con ricarica"), Examined > 0);
	// E una ricarica di 1 e' «ogni turno»: disponibile dal turno dopo, non due dopo.
	FRTInspectDeducedCooldown EveryTurn;
	TestTrue(TEXT("ricarica 1"), URTInspectLibrary::MakeDeducedCooldown(TEXT("A"), FText::FromString(TEXT("A")), 1, 4, EveryTurn));
	TestEqual(TEXT("ricarica 1: usata al turno 4, di nuovo usabile dal 5"), EveryTurn.AvailableFromTurn, 5);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// Niente riga dove non c'e' niente da dire, e la riga si legge come una stima.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseLinesReadAsAnEstimateTest,
	"RefactorTactics.UI.InspectDeducedLinesReadAsAnEstimate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseLinesReadAsAnEstimateTest::RunTest(const FString&)
{
	FRTInspectDeducedCooldown Row;
	const FText Title = FText::FromString(TEXT("Colpo di prova"));
	TestFalse(TEXT("mai vista usare: nessuna riga"), URTInspectLibrary::MakeDeducedCooldown(TEXT("A"), Title, 3, 0, Row));
	TestFalse(TEXT("senza ricarica: nessuna riga"), URTInspectLibrary::MakeDeducedCooldown(TEXT("A"), Title, 0, 4, Row));
	TestFalse(TEXT("senza identita': nessuna riga"), URTInspectLibrary::MakeDeducedCooldown(NAME_None, Title, 3, 4, Row));
	TestFalse(TEXT("senza titolo: nessuna riga"), URTInspectLibrary::MakeDeducedCooldown(TEXT("A"), FText::GetEmpty(), 3, 4, Row));
	TestTrue(TEXT("controllo positivo: vista al turno 4, ricarica 3"), URTInspectLibrary::MakeDeducedCooldown(TEXT("A"), Title, 3, 4, Row));
	TestEqual(TEXT("disponibile dal turno 7"), Row.AvailableFromTurn, 7);

	FRTInspectSubject Subject;
	Subject.DisplayName = FText::FromString(TEXT("Nemico"));
	const FString Without = SeenText(URTInspectLibrary::Compose(Subject));
	TestFalse(TEXT("senza righe dedotte non c'e' l'intestazione"), Without.Contains(TEXT("Ricariche")));

	Subject.DeducedCooldowns.Add(Row);
	const FString With = SeenText(URTInspectLibrary::Compose(Subject));
	TestTrue(TEXT("l'intestazione dichiara una stima"), With.Contains(TEXT("Ricariche (stima")));
	TestTrue(TEXT("la riga nomina l'azione"), With.Contains(TEXT("Colpo di prova")));
	TestTrue(TEXT("e l'ultimo turno visto"), With.Contains(TEXT("turno 4")));
	TestTrue(TEXT("e quando torna disponibile"), With.Contains(TEXT("dal turno 7")));
	return true;
}

// Nessun campo della scheda porta la ricarica vera: per reflection, sulla riga e sul soggetto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseRowCarriesNoTrueCooldownTest,
	"RefactorTactics.UI.InspectDeducedRowCarriesNoTrueCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseRowCarriesNoTrueCooldownTest::RunTest(const FString&)
{
	// La riga ha esattamente i quattro campi dichiarati: un quinto (un contatore residuo, un «usabile ora») sarebbe
	// la ricarica vera che P4 chiude fuori.
	TSet<FString> Names;
	for (TFieldIterator<FProperty> It(FRTInspectDeducedCooldown::StaticStruct()); It; ++It) { Names.Add(It->GetName()); }
	const TSet<FString> Expected = { TEXT("ActionId"), TEXT("Title"), TEXT("LastSeenTurn"), TEXT("AvailableFromTurn") };
	TestTrue(TEXT("la riga porta solo i quattro campi dichiarati"), Names.Num() == Expected.Num() && Names.Includes(Expected));

	// Sul soggetto, l'unico campo che nomina una ricarica e' quello dedotto; il resto ha il nome del kit.
	auto TrueCooldownFields = [](const UStruct* Type)
	{
		TArray<FString> Offenders;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			const FString Name = It->GetName();
			if ((Name.Contains(TEXT("Cooldown")) || Name.Contains(TEXT("Remaining")) || Name.Contains(TEXT("Usable")))
				&& Name != TEXT("DeducedCooldowns"))
			{
				Offenders.Add(Name);
			}
		}
		return Offenders;
	};
	TestEqual(TEXT("sul soggetto solo il campo dedotto nomina una ricarica"), TrueCooldownFields(FRTInspectSubject::StaticStruct()).Num(), 0);
	TestEqual(TEXT("e sul tooltip del kit, che la scheda riusa, nessuno"), TrueCooldownFields(FRTActionTooltipView::StaticStruct()).Num(), 0);
	TestNotNull(TEXT("e quel campo esiste"), FRTInspectSubject::StaticStruct()->FindPropertyByName(TEXT("DeducedCooldowns")));

	// Il controllo sa vedere: lo stesso scan, sul tipo che porta la ricarica VERA di uno slot, la trova.
	TestTrue(TEXT("anti-vacuita': la scansione trova `TurnsRemaining`/`bUsableNow` su FRTAbilityCooldownView"),
		TrueCooldownFields(FRTAbilityCooldownView::StaticStruct()).Num() > 0);
	return true;
}

// Dalla risoluzione vera: un attacco visto dalla squadra lascia una riga, la scheda la mostra, e lo stesso attacco
// fuori vista dà la stessa scheda di una partita in cui non e' mai avvenuto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseRealTurnTest,
	"RefactorTactics.SeenUse.ARealSeenAttackLeavesARowAndAnUnseenOneDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseRealTurnTest::RunTest(const FString&)
{
	struct FBoard
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Attacker = nullptr; // squadra 0: e' lui che si ispeziona
		ARTUnit* Defender = nullptr; // squadra 1: e' chi guarda
	};
	auto Build = [this](FBoard& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("il mondo di prova esiste"), Out.World)) { return false; }
		ARTHexMapActor* Map = Out.World->SpawnActor<ARTHexMapActor>();
		Map->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		Out.TM = Out.World->SpawnActor<ARTTurnManager>();
		auto Spawn = [&](int32 TeamId, const FRTCellId& Cell)
		{
			ARTUnit* U = Out.World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
			if (U == nullptr) { return static_cast<ARTUnit*>(nullptr); }
			U->TeamId = TeamId;
			U->bIsBotControlled = false; // i piani li scriviamo noi
			U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
			UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
			U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, 250.f);
			return U;
		};
		Out.Defender = Spawn(SeenObserverTeam, FRTCellId(0, 0, 0));
		Out.Attacker = Spawn(0, FRTCellId(-1, 0, 0));
		return TestNotNull(TEXT("turn manager"), Out.TM) && TestNotNull(TEXT("difensore"), Out.Defender)
			&& TestNotNull(TEXT("attaccante"), Out.Attacker);
	};
	auto Resolve = [](FBoard& Board, bool bAttack)
	{
		if (bAttack)
		{
			Board.Attacker->PlannedAbilityIndex = 0; // PulseShot
			Board.Attacker->PlannedAttackTarget = Board.Defender;
		}
		Board.TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && Board.TM->IsResolving(); ++I) { Board.TM->Tick(0.05f); }
	};

	FBoard Attack;
	if (!Build(Attack)) { RTWorldFixtures::DestroyWorld(Attack.World); return false; }
	Resolve(Attack, /*bAttack=*/ true);

	const URTActionData* Shot = Attack.Attacker->GetAbility(0);
	if (!TestNotNull(TEXT("l'attaccante ha un'azione 0"), Shot)) { RTWorldFixtures::DestroyWorld(Attack.World); return false; }
	const FRTTurnLogEntry* Hit = Attack.TM->GetTurnLog().FindByPredicate([&](const FRTTurnLogEntry& E)
	{
		return E.UnitId == Attack.Attacker->StableUnitId && FRTSeenUseLedger::IsUseByItsSubject(E);
	});
	if (!TestNotNull(TEXT("premessa: il turno ha scritto una voce d'uso dell'attaccante"), Hit))
	{
		RTWorldFixtures::DestroyWorld(Attack.World);
		return false;
	}
	TestEqual(TEXT("premessa: la voce porta l'azione del kit"), Hit->ActionId, Shot->Def.ActionId);
	TestTrue(TEXT("premessa: la squadra del difensore e' autorizzata a leggere la voce"), Hit->Verdict.AllowsTeam(SeenObserverTeam));

	// La misura che la spec chiedeva (§6.4): il verdetto di una voce si congela contro l'unita' che la scrive, la
	// stessa che finisce in `UnitId`. Su TUTTE le voci vere di questo turno con un'unita', non solo sulla prima.
	int32 WithUnit = 0;
	for (const FRTTurnLogEntry& Entry : Attack.TM->GetTurnLog())
	{
		if (Entry.UnitId == 0) { continue; }
		++WithUnit;
		TestEqual(TEXT("il verdetto e' congelato contro l'unita' di `UnitId`"), Entry.VerdictSubject.StableUnitId, Entry.UnitId);
	}
	TestTrue(TEXT("il controllo ha esaminato delle voci"), WithUnit > 0);

	const FRTSeenUseLedger& Ledger = Attack.TM->GetSeenUses();
	TestEqual(TEXT("il TurnManager ha imparato l'uso nel turno della voce"),
		Ledger.LastSeenTurn(SeenObserverTeam, Attack.Attacker->StableUnitId, Shot->Def.ActionId), Hit->TurnNumber);

	// La scheda: stessa unita', ma con una ricarica di catalogo (PulseShot non ne ha), costruita sulla voce VERA.
	FRTSeenUseLedger Reread;
	for (const FRTTurnLogEntry& Entry : Attack.TM->GetTurnLog()) { Reread.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam }); }
	TestEqual(TEXT("rileggendo il TurnLog si ottiene lo stesso registro"), Reread.Num(), Ledger.Num());

	// Il mondo gemello: la stessa partita senza nessun attacco, e la stessa partita con l'attacco ma letto come se
	// fosse avvenuto fuori vista (verdetto che non autorizza nessuno). Devono dare la stessa scheda.
	FBoard Quiet;
	if (!Build(Quiet)) { RTWorldFixtures::DestroyWorld(Attack.World); RTWorldFixtures::DestroyWorld(Quiet.World); return false; }
	Resolve(Quiet, /*bAttack=*/ false);

	FRTSeenUseLedger BlindReading;
	for (FRTTurnLogEntry Entry : Attack.TM->GetTurnLog())
	{
		Entry.Verdict = FRTKnowledgeVerdict::NoOne();
		BlindReading.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam });
	}
	// Si confronta cio' che F6 produce — le righe dedotte e il loro testo — e non la scheda intera: PV, verso e stati
	// di un attaccante che ha appena combattuto differiscono dal gemello fermo per ragioni che F6 non tocca.
	auto Deduced = [](const FRTInspectSubject& Subject)
	{
		FRTInspectSubject OnlyDeduced;
		OnlyDeduced.DeducedCooldowns = Subject.DeducedCooldowns;
		return SeenText(URTInspectLibrary::Compose(OnlyDeduced));
	};
	const FString BlindDeduced = Deduced(ARTHUD::BuildInspectSubject(Attack.Attacker, SeenObserverTeam, true, &BlindReading));
	const FString QuietDeduced = Deduced(ARTHUD::BuildInspectSubject(Quiet.Attacker, SeenObserverTeam, true, &Quiet.TM->GetSeenUses()));
	TestEqual(TEXT("un uso fuori vista e un uso mai avvenuto danno la stessa sezione di ricariche"), BlindDeduced, QuietDeduced);
	TestEqual(TEXT("e i due registri sono vuoti"), BlindReading.Num() + Quiet.TM->GetSeenUses().Num(), 0);
	TestTrue(TEXT("controllo positivo: lo stesso attacco, letto da chi lo vede, il registro non e' vuoto"), Ledger.Num() > 0);

	// PulseShot non ha ricarica, quindi la sezione e' vuota da entrambi i lati e il confronto qui sopra non basta.
	// Controllo positivo sulla scheda: la voce VERA — stessa unita', stesso turno, stesso verdetto — con l'azione
	// sostituita da una che ha una ricarica di catalogo (indice 1, InterceptShot).
	const URTActionData* Intercept = Attack.Attacker->GetAbility(1);
	if (TestTrue(TEXT("premessa: l'azione 1 ha una ricarica di catalogo"), Intercept != nullptr && Intercept->CooldownTurns > 0))
	{
		FRTTurnLogEntry SeenEntry = *Hit;
		SeenEntry.ActionId = Intercept->Def.ActionId;
		FRTSeenUseLedger SeenLedger;
		SeenLedger.Observe(SeenEntry, TArray<int32>{ 0, SeenObserverTeam });
		const FString SeenDeduced = Deduced(ARTHUD::BuildInspectSubject(Attack.Attacker, SeenObserverTeam, true, &SeenLedger));
		TestTrue(TEXT("visto: la scheda ha la sezione, dichiarata stima"), SeenDeduced.Contains(TEXT("Ricariche (stima")));

		FRTTurnLogEntry UnseenEntry = SeenEntry;
		UnseenEntry.Verdict = FRTKnowledgeVerdict::NoOne();
		FRTSeenUseLedger UnseenLedger;
		UnseenLedger.Observe(UnseenEntry, TArray<int32>{ 0, SeenObserverTeam });
		TestEqual(TEXT("fuori vista: la stessa voce non lascia traccia sulla scheda"),
			Deduced(ARTHUD::BuildInspectSubject(Attack.Attacker, SeenObserverTeam, true, &UnseenLedger)), QuietDeduced);
	}

	RTWorldFixtures::DestroyWorld(Attack.World);
	RTWorldFixtures::DestroyWorld(Quiet.World);
	return true;
}

// Sulla scheda: l'azione vista usare, con una ricarica di catalogo, compare per un NEMICO e non per un'unita' propria.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseSubjectTest,
	"RefactorTactics.UI.InspectSubjectDeducesCooldownsOnlyForAnEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseSubjectTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Enemy = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("l'unita' esiste"), Enemy)) { RTWorldFixtures::DestroyWorld(World); return false; }
	Enemy->TeamId = 0;
	// L'id stabile lo assegna chi registra le unita' in partita; in un mondo senza `TurnManager` vale 0, e una voce
	// «senza unita'» non dice chi ha agito. Lo si dichiara come farebbe la partita.
	Enemy->StableUnitId = SeenUnit;
	Enemy->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
	UGameplayStatics::FinishSpawningActor(Enemy, FTransform::Identity);

	// Ivrin: indice 1 = InterceptShot, ricarica 2.
	const URTActionData* Intercept = Enemy->GetAbility(1);
	if (!TestTrue(TEXT("premessa: l'azione 1 ha una ricarica di catalogo"), Intercept != nullptr && Intercept->CooldownTurns > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTTurnLogEntry Used = SeenHit(Intercept->Def.ActionId, 4);
	Used.UnitId = Enemy->StableUnitId;
	FRTSeenUseLedger Ledger;
	Ledger.Observe(Used, TArray<int32>{ 0, SeenObserverTeam });

	const FRTInspectSubject EnemySubject = ARTHUD::BuildInspectSubject(Enemy, SeenObserverTeam, true, &Ledger);
	if (TestEqual(TEXT("un nemico: una riga, per l'unica azione vista"), EnemySubject.DeducedCooldowns.Num(), 1))
	{
		TestEqual(TEXT("l'azione vista"), EnemySubject.DeducedCooldowns[0].ActionId, Intercept->Def.ActionId);
		TestEqual(TEXT("l'ultimo turno visto"), EnemySubject.DeducedCooldowns[0].LastSeenTurn, 4);
		TestEqual(TEXT("e il turno da cui torna disponibile"), EnemySubject.DeducedCooldowns[0].AvailableFromTurn, 4 + Intercept->CooldownTurns);
	}

	// Senza registro, nessuna riga: il default e' il comportamento di prima.
	TestEqual(TEXT("senza registro: nessuna riga"), ARTHUD::BuildInspectSubject(Enemy, SeenObserverTeam, true).DeducedCooldowns.Num(), 0);
	// Un'unita' della squadra di chi guarda: si leggono le ricariche vere, non le dedotte.
	// 🔴 Il registro deve CONOSCERE l'unita' dalla parte della sua stessa squadra, altrimenti l'asserto non esercita la
	// guardia: con un verdetto che autorizza solo la squadra 1, la squadra 0 non ha comunque nessuna riga (la prima
	// stesura passava anche senza la guardia, e la mutazione l'ha scoperto). Qui la voce e' letta da tutti.
	FRTTurnLogEntry UsedByAll = Used;
	UsedByAll.Verdict = FRTKnowledgeVerdict::Everyone();
	FRTSeenUseLedger KnowsBothSides;
	KnowsBothSides.Observe(UsedByAll, TArray<int32>{ 0, SeenObserverTeam });
	TestTrue(TEXT("premessa: il registro conosce l'uso dalla squadra dell'unita' stessa"),
		KnowsBothSides.LastSeenTurn(/*ObserverTeamId=*/ 0, Enemy->StableUnitId, Intercept->Def.ActionId) > 0);
	TestEqual(TEXT("controllo positivo: lo stesso registro, letto da chi non e' della sua squadra, da' la riga"),
		ARTHUD::BuildInspectSubject(Enemy, SeenObserverTeam, true, &KnowsBothSides).DeducedCooldowns.Num(), 1);
	TestEqual(TEXT("un'unita' propria: nessuna riga dedotta, anche se il registro la conosce"),
		ARTHUD::BuildInspectSubject(Enemy, /*PlayerTeamId=*/ 0, true, &KnowsBothSides).DeducedCooldowns.Num(), 0);
	// La squadra che NON ha visto: un registro che non la conosce non ne inventa.
	FRTSeenUseLedger Empty;
	TestEqual(TEXT("registro vuoto: nessuna riga"), ARTHUD::BuildInspectSubject(Enemy, SeenObserverTeam, true, &Empty).DeducedCooldowns.Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
