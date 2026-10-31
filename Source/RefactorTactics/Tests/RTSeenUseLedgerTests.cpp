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

	/** Quante righe lascia la voce. Piu' stretto di `SeenAfter`: una riga con turno 0 risponderebbe `0` e passerebbe. */
	int32 SeenRows(const FRTTurnLogEntry& Entry)
	{
		FRTSeenUseLedger Ledger;
		Ledger.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam });
		return Ledger.Num();
	}

	FString SeenText(const FRTInspectView& View) { return FString::Join(View.Lines, TEXT("\n")); }

	/** Le righe della scheda che iniziano con `Prefix`, nell'ordine (la stima delle ricariche dedotte inizia con «Stima»). */
	TArray<FString> SeenLinesWith(const FRTInspectView& View, const TCHAR* Prefix)
	{
		TArray<FString> Out;
		for (const FString& Line : View.Lines)
		{
			if (Line.StartsWith(Prefix, ESearchCase::CaseSensitive)) { Out.Add(Line); }
		}
		return Out;
	}
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
	// ⌫ Ritrattato il 2026-10-31 (`#3134`, [D-414]): da `R4` un attacco bloccato dal muro PAGA la ricarica, quindi la prima
	// meta' non e' piu' vera. La deduzione tace perche' il registro conta solo danno e cura e non distingue l'uso, non
	// perche' la ricarica non sia pagata. L'asserto qui sotto non cambia.
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
		// Si contano le RIGHE e non il turno letto: una riga con turno 0 risponderebbe `0` a `LastSeenTurn` e passerebbe.
		TestEqual(TEXT("senza unita': nessuna riga"), SeenRows(NoUnit), 0);
		TestEqual(TEXT("senza azione: nessuna riga"), SeenRows(NoAction), 0);
		TestEqual(TEXT("senza turno: nessuna riga"), SeenRows(NoTurn), 0);
		TestEqual(TEXT("controllo positivo: la stessa voce completa lascia UNA riga"), SeenRows(SeenHit()), 1);

		// La guardia vale anche per la cura, che non passa da `IsDamageInflictedByActor` e ha il suo ramo.
		FRTTurnLogEntry HealNoUnit = SeenHit(TEXT("Hero.Test.Heal")); HealNoUnit.Outcome = static_cast<uint8>(ERTCombatOutcome::Healed); HealNoUnit.UnitId = 0;
		FRTTurnLogEntry HealNoTurn = SeenHit(TEXT("Hero.Test.Heal"), 0); HealNoTurn.Outcome = static_cast<uint8>(ERTCombatOutcome::Healed);
		TestEqual(TEXT("una cura senza unita': nessuna riga"), SeenRows(HealNoUnit), 0);
		TestEqual(TEXT("una cura senza turno: nessuna riga"), SeenRows(HealNoTurn), 0);
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

// Niente riga dove non c'e' niente da dire, e la riga si legge come una stima: dal 2026-10-31 fra i dettagli della skill sotto il cursore
// (la sezione «Ricariche» non c'e' piu'). Stessi fatti di prima, spostati nei dettagli.
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
	TestFalse(TEXT("senza righe dedotte non c'e' ne' una sezione ne' una stima"), Without.Contains(TEXT("Ricariche")) || Without.Contains(TEXT("Stima")));

	// Il soggetto HA la riga dedotta ma nessuna skill e' sotto il cursore: la riga non si disegna da sola, e la sezione di prima non c'e' piu'.
	Subject.DeducedCooldowns.Add(Row);
	const FString NoHover = SeenText(URTInspectLibrary::Compose(Subject));
	TestFalse(TEXT("senza una skill sotto il cursore la sezione «Ricariche» non c'e'"), NoHover.Contains(TEXT("Ricariche")));
	TestFalse(TEXT("ne' la stima"), NoHover.Contains(TEXT("Stima")));
	TestFalse(TEXT("ne' l'azione vista usare, ne' i suoi turni"), NoHover.Contains(TEXT("Colpo di prova")) || NoHover.Contains(TEXT("turno 4")));

	// Una skill sotto il cursore la cui azione NON e' stata vista usare: i dettagli ci sono, la stima no.
	Subject.HoveredSkill.ActionId = TEXT("A");
	Subject.HoveredSkill.Title = Title;
	const FString NotSeen = SeenText(URTInspectLibrary::Compose(Subject));
	TestTrue(TEXT("i dettagli ci sono: il titolo della skill"), NotSeen.Contains(TEXT("Colpo di prova")));
	TestFalse(TEXT("senza avvistamento nei dettagli, nessuna stima e nessuna sezione"), NotSeen.Contains(TEXT("Stima")) || NotSeen.Contains(TEXT("Ricariche")));

	// Con la riga dedotta nei dettagli: una riga sola, dichiarata stima, dopo il titolo, coi due turni e nessun conteggio residuo.
	Subject.HoveredSkill.Deduced.Add(Row);
	const FRTInspectView WithView = URTInspectLibrary::Compose(Subject);
	const FString With = SeenText(WithView);
	TestFalse(TEXT("la sezione «Ricariche» non c'e' piu', nemmeno con la stima"), With.Contains(TEXT("Ricariche")));
	int32 TitleLine = INDEX_NONE;
	TArray<int32> EstimateLines;
	for (int32 I = 0; I < WithView.Lines.Num(); ++I)
	{
		if (WithView.Lines[I] == TEXT("Colpo di prova")) { TitleLine = I; }
		if (WithView.Lines[I].StartsWith(TEXT("Stima"))) { EstimateLines.Add(I); }
	}
	if (TestEqual(TEXT("la stima e' UNA riga, che dichiara di esserlo"), EstimateLines.Num(), 1))
	{
		const FString& Estimate = WithView.Lines[EstimateLines[0]];
		TestTrue(TEXT("la stima viene dopo il titolo dell'azione a cui si riferisce"), TitleLine != INDEX_NONE && EstimateLines[0] > TitleLine);
		TestTrue(TEXT("e dice l'ultimo turno visto"), Estimate.Contains(TEXT("turno 4")));
		TestTrue(TEXT("e quando torna disponibile"), Estimate.Contains(TEXT("dal turno 7")));
		// I numeri della riga sono i due turni e basta: un terzo numero sarebbe un conto dei turni che restano, cioe' la ricarica vera.
		TArray<int32> Numbers;
		FString Digits;
		for (const TCHAR Ch : Estimate + TEXT(" "))
		{
			if (FChar::IsDigit(Ch)) { Digits.AppendChar(Ch); }
			else if (!Digits.IsEmpty()) { Numbers.Add(FCString::Atoi(*Digits)); Digits.Reset(); }
		}
		TestEqual(TEXT("la riga porta i due turni e nessun conteggio residuo"), Numbers, TArray<int32>{ 4, 7 });
	}

	// 🔑 L'ORDINE (review finale del 2026-10-31): titolo, righe del tooltip, STIMA, poi le righe dei pezzi. La finestra della scheda taglia la
	// coda, e quella che deve cadere per ultima e' il catalogo (i pezzi), non l'informazione che viene dall'osservazione. Una skill con una
	// riga di tooltip e un pezzo che ha vantaggio e svantaggio, e la stessa stima.
	Subject.HoveredSkill.Lines.Add(FText::FromString(TEXT("Portata  3")));
	FRTInspectEquipmentLine Piece;
	Piece.Name = FText::FromString(TEXT("Pezzo di prova"));
	Piece.Slot = ERTEquipmentSlot::Gadget;
	Piece.Advantage = FText::FromString(TEXT("un vantaggio"));
	Piece.Drawback = FText::FromString(TEXT("uno svantaggio"));
	Subject.HoveredSkill.Equipment.Add(Piece);
	const FRTInspectView OrderView = URTInspectLibrary::Compose(Subject);
	int32 TooltipLine = INDEX_NONE;
	int32 EstimateLine = INDEX_NONE;
	int32 FirstPieceLine = INDEX_NONE;   // la prima riga di un pezzo: il suo «Nome (Slot)», o la sua riga `+`/`-`
	int32 PlusLine = INDEX_NONE;
	int32 MinusLine = INDEX_NONE;
	for (int32 I = 0; I < OrderView.Lines.Num(); ++I)
	{
		const FString& Line = OrderView.Lines[I];
		if (Line == TEXT("Portata  3")) { TooltipLine = I; }
		if (Line.StartsWith(TEXT("Stima"))) { EstimateLine = I; }
		if (Line.StartsWith(TEXT("  + "))) { PlusLine = I; }
		if (Line.StartsWith(TEXT("  - "))) { MinusLine = I; }
		if ((Line.StartsWith(TEXT("Pezzo di prova (")) || Line.StartsWith(TEXT("  + ")) || Line.StartsWith(TEXT("  - "))) && FirstPieceLine == INDEX_NONE)
		{
			FirstPieceLine = I;
		}
	}
	// Controllo positivo: le righe che si confrontano esistono tutte (un indice assente confronterebbe INDEX_NONE con INDEX_NONE).
	TestTrue(TEXT("controllo positivo: ci sono la riga del tooltip, la stima, e le righe `+` e `-` del pezzo"),
		TooltipLine != INDEX_NONE && EstimateLine != INDEX_NONE && PlusLine != INDEX_NONE && MinusLine != INDEX_NONE && FirstPieceLine != INDEX_NONE);
	TestTrue(TEXT("la stima viene dopo le righe del tooltip"), TooltipLine != INDEX_NONE && EstimateLine > TooltipLine);
	TestTrue(TEXT("e PRIMA della prima riga di un pezzo: la finestra taglia la coda, e la coda e' il catalogo"),
		EstimateLine != INDEX_NONE && FirstPieceLine != INDEX_NONE && EstimateLine < FirstPieceLine);
	TestTrue(TEXT("e prima delle righe `+` e `-`"), EstimateLine != INDEX_NONE && EstimateLine < PlusLine && EstimateLine < MinusLine);
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

	// Il nome non basta (`TurnsLeft` o `bReady` passerebbero): sul soggetto l'unico booleano e' `bShowFacing` e non ci sono
	// numeri sciolti. Un contatore residuo o un «usabile ora» sarebbero esattamente l'uno o l'altro.
	auto LooseScalars = [](const UStruct* Type)
	{
		TArray<FString> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			if (CastField<FBoolProperty>(*It) != nullptr || CastField<FNumericProperty>(*It) != nullptr)
			{
				Out.Add(It->GetName());
			}
		}
		return Out;
	};
	TestEqual(TEXT("sul soggetto l'unico scalare sciolto e' `bShowFacing`"), LooseScalars(FRTInspectSubject::StaticStruct()), TArray<FString>{ TEXT("bShowFacing") });
	TestTrue(TEXT("anti-vacuita': lo stesso scan trova `TurnsRemaining` e `bUsableNow` su FRTAbilityCooldownView"),
		LooseScalars(FRTAbilityCooldownView::StaticStruct()).Num() >= 2);
	TestEqual(TEXT("e sul tooltip del kit, che la scheda riusa, nessuno"), TrueCooldownFields(FRTActionTooltipView::StaticStruct()).Num(), 0);
	TestNotNull(TEXT("e quel campo esiste"), FRTInspectSubject::StaticStruct()->FindPropertyByName(TEXT("DeducedCooldowns")));

	// La stima ora si vede nei dettagli della skill sotto il cursore (2026-10-31): lo stesso conto sul dettaglio. La stima e' un array di
	// `FRTInspectDeducedCooldown` (le quattro colonne pinnate sopra), e il resto non porta ne' una ricarica ne' un numero sciolto.
	TestEqual(TEXT("sul dettaglio della skill nessun campo nomina una ricarica vera"), TrueCooldownFields(FRTInspectSkillDetail::StaticStruct()).Num(), 0);
	TestEqual(TEXT("e nessun booleano o numero sciolto: un contatore residuo o un «usabile ora» sarebbero esattamente l'uno o l'altro"),
		LooseScalars(FRTInspectSkillDetail::StaticStruct()).Num(), 0);
	const FArrayProperty* DetailDeduced = CastField<FArrayProperty>(FRTInspectSkillDetail::StaticStruct()->FindPropertyByName(TEXT("Deduced")));
	const FStructProperty* DetailDeducedRow = DetailDeduced != nullptr ? CastField<FStructProperty>(DetailDeduced->Inner) : nullptr;
	TestTrue(TEXT("e `Deduced` e' la riga dedotta di quattro colonne, non un'altra"),
		DetailDeducedRow != nullptr && DetailDeducedRow->Struct == FRTInspectDeducedCooldown::StaticStruct());

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

	// PulseShot non ha ricarica di catalogo, e senza non c'e' niente da dedurre ne' da proteggere. L'azione e' una copia
	// dell'unita' (`MakeIvrin` crea oggetti nuovi a ogni chiamata): la ricarica che `ConsumeAbility` legge si dichiara
	// qui, su questa unita' sola.
	constexpr int32 RealCooldown = 3;
	auto GiveShotACooldown = [&](FBoard& Board)
	{
		if (Board.Attacker != nullptr && Board.Attacker->Abilities.IsValidIndex(0) && Board.Attacker->Abilities[0] != nullptr)
		{
			Board.Attacker->Abilities[0]->CooldownTurns = RealCooldown;
		}
	};

	FBoard Attack;
	if (!Build(Attack)) { RTWorldFixtures::DestroyWorld(Attack.World); return false; }
	GiveShotACooldown(Attack);
	Resolve(Attack, /*bAttack=*/ true);

	const URTActionData* Shot = Attack.Attacker->GetAbility(0);
	if (!TestNotNull(TEXT("l'attaccante ha un'azione 0"), Shot)) { RTWorldFixtures::DestroyWorld(Attack.World); return false; }
	TestEqual(TEXT("premessa: l'azione 0 ha la ricarica di catalogo dichiarata"), Shot->CooldownTurns, RealCooldown);
	const FRTTurnLogEntry* Hit = Attack.TM->GetTurnLog().FindByPredicate([&](const FRTTurnLogEntry& E)
	{
		return E.UnitId == Attack.Attacker->StableUnitId && FRTSeenUseLedger::IsUseByItsSubject(E);
	});
	if (!TestNotNull(TEXT("premessa: il turno ha scritto una voce d'uso dell'attaccante"), Hit))
	{
		RTWorldFixtures::DestroyWorld(Attack.World);
		return false;
	}
	TestEqual(TEXT("l'ActionId loggato e' quello del kit, per un'azione CON ricarica"), Hit->ActionId, Shot->Def.ActionId);
	TestTrue(TEXT("premessa: la squadra del difensore e' autorizzata a leggere la voce"), Hit->Verdict.AllowsTeam(SeenObserverTeam));

	// Il verdetto di una voce si congela contro l'unita' che la scrive (spec §6.4). Vale per costruzione —
	// `AppendLogEntry` legge `Actor->StableUnitId` in entrambi i punti — e questo test tiene ferma la costruzione sulle
	// voci vere di un turno: se qualcuno scrivesse l'una da un'unita' e l'altro da un'altra, qui diventa rosso.
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

	FRTSeenUseLedger Reread;
	for (const FRTTurnLogEntry& Entry : Attack.TM->GetTurnLog()) { Reread.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam }); }
	TestEqual(TEXT("rileggendo il TurnLog si ottiene lo stesso registro"), Reread.Num(), Ledger.Num());

	// Si confronta cio' che F6 produce — la riga della stima, nei dettagli della skill sotto il cursore (la posizione 0, il colpo) — e non la scheda
	// intera: PV, verso e stati di un attaccante che ha appena combattuto differiscono dal gemello fermo per ragioni che F6 non tocca.
	// (Prima del 2026-10-31 era la sezione «Ricariche»: i fatti sono gli stessi, ora nei dettagli.)
	auto Deduced = [](const ARTUnit* Unit, const FRTSeenUseLedger* Seen)
	{
		FString Out;
		const FRTInspectView View = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Unit, SeenObserverTeam, true, Seen, /*HoverAbilityIndex=*/ 0));
		for (const FString& Line : View.Lines)
		{
			if (Line.StartsWith(TEXT("Stima"))) { Out += Line + TEXT("\n"); }
		}
		return Out;
	};

	// La scheda sulla voce VERA, passata da `AppendLogEntry` e dal registro del TurnManager.
	const FRTInspectSubject SeenSubject = ARTHUD::BuildInspectSubject(Attack.Attacker, SeenObserverTeam, true, &Ledger);
	if (TestEqual(TEXT("la scheda ha una riga, per l'azione vista"), SeenSubject.DeducedCooldowns.Num(), 1))
	{
		const FRTInspectDeducedCooldown& Row = SeenSubject.DeducedCooldowns[0];
		TestEqual(TEXT("l'azione vista"), Row.ActionId, Shot->Def.ActionId);
		TestEqual(TEXT("l'ultimo turno visto"), Row.LastSeenTurn, Hit->TurnNumber);
		TestEqual(TEXT("e il turno da cui torna disponibile"), Row.AvailableFromTurn, Hit->TurnNumber + RealCooldown);

		// 🔑 L'ORDINE, SU UN TURNO VERO (rilievo della review). Il pagamento avviene in risoluzione e il Cleanup dello
		// STESSO turno ne toglie uno: pianificando il turno dopo restano `RealCooldown - 1`, e i turni che mancano alla
		// disponibilita' dichiarata sono esattamente la ricarica residua. Spostare il Cleanup, o il pagamento, fa
		// slittare di uno questa uguaglianza.
		TestEqual(TEXT("premessa: il turno e' concluso e si pianifica il successivo"), Attack.TM->GetTurnNumber(), Hit->TurnNumber + 1);
		TestEqual(TEXT("la ricarica vera residua e' scesa di UNO nel Cleanup dello stesso turno"),
			Attack.Attacker->GetAbilityCooldown(0), RealCooldown - 1);
		TestEqual(TEXT("e coincide con i turni che mancano alla disponibilita' dichiarata dalla scheda"),
			Attack.Attacker->GetAbilityCooldown(0), Row.AvailableFromTurn - Attack.TM->GetTurnNumber());
	}
	const FString SeenEstimate = Deduced(Attack.Attacker, &Ledger);
	TestTrue(TEXT("la riga dei dettagli della skill vista dichiara una stima"), SeenEstimate.StartsWith(TEXT("Stima")));
	TestTrue(TEXT("e porta l'ultimo turno visto e quello da cui torna disponibile, del turno vero"),
		SeenEstimate.Contains(FString::Printf(TEXT("al turno %d"), Hit->TurnNumber))
		&& SeenEstimate.Contains(FString::Printf(TEXT("dal turno %d"), Hit->TurnNumber + RealCooldown)));

	// Il mondo gemello: la stessa partita senza nessun attacco. E' fermo: la sua ricarica VERA e' a zero. Quella del mondo
	// dell'attacco e' in corso, ed e' il canale che P4 vieta: se la scheda la leggesse, le due righe di stima differirebbero.
	FBoard Quiet;
	if (!Build(Quiet)) { RTWorldFixtures::DestroyWorld(Attack.World); RTWorldFixtures::DestroyWorld(Quiet.World); return false; }
	GiveShotACooldown(Quiet);
	Resolve(Quiet, /*bAttack=*/ false);
	TestTrue(TEXT("premessa: nel mondo dell'attacco la ricarica VERA e' in corso"), Attack.Attacker->GetAbilityCooldown(0) > 0);
	TestEqual(TEXT("premessa: nel gemello fermo non lo e'"), Quiet.Attacker->GetAbilityCooldown(0), 0);

	// Lo stesso attacco, letto come se fosse avvenuto fuori vista (verdetto che non autorizza nessuno).
	FRTSeenUseLedger BlindReading;
	for (FRTTurnLogEntry Entry : Attack.TM->GetTurnLog())
	{
		Entry.Verdict = FRTKnowledgeVerdict::NoOne();
		BlindReading.Observe(Entry, TArray<int32>{ 0, SeenObserverTeam });
	}
	const FString BlindDeduced = Deduced(Attack.Attacker, &BlindReading);
	const FString QuietDeduced = Deduced(Quiet.Attacker, &Quiet.TM->GetSeenUses());
	TestEqual(TEXT("un uso fuori vista e un uso mai avvenuto danno la stessa stima, a ricariche vere diverse"), BlindDeduced, QuietDeduced);
	TestTrue(TEXT("e nessuna delle due ha una stima da mostrare"), BlindDeduced.IsEmpty() && QuietDeduced.IsEmpty());
	TestEqual(TEXT("e i due registri sono vuoti"), BlindReading.Num() + Quiet.TM->GetSeenUses().Num(), 0);
	TestTrue(TEXT("controllo positivo: lo stesso attacco, letto da chi lo vede, il registro non e' vuoto"), Ledger.Num() > 0);
	TestNotEqual(TEXT("controllo positivo: e la stima di chi lo vede non e' quella vuota"), SeenEstimate, QuietDeduced);

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

/**
 * LA STIMA STA NEI DETTAGLI DELLA SKILL SOTTO IL CURSORE, E SOLO SE LA SQUADRA L'HA VISTA USARE (`#3748`, decisione d'autore del 2026-10-31;
 * `#3751`). La sezione «Ricariche» e' sparita dal box: la riga di UNA azione compare quando il cursore e' sul suo slot, e mai come contatore residuo.
 *
 * Un nemico Ivrin osservato dalla squadra `SeenObserverTeam`, con un registro in cui ha usato `InterceptShot` (posizione 1, ricarica di catalogo) al
 * turno 4. Il controllo positivo e' il primo caso (la riga c'e', una sola, dichiarata stima, coi due turni e basta); gli altri la tolgono ciascuno per una
 * sola ragione: nessun cursore, un'altra posizione, nessun registro, un registro vuoto, un uso di un'altra azione, un uso fuori vista, un'azione senza
 * ricarica di catalogo, un'unita' della propria squadra. E la ricarica VERA in corso non cambia nulla: la riga e' la stessa con o senza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeenUseHoveredSkillTest,
	"RefactorTactics.UI.InspectCardHoveredSkillShowsTheDeducedCooldownOnlyIfSeen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeenUseHoveredSkillTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Enemy = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("l'unita' esiste"), Enemy)) { RTWorldFixtures::DestroyWorld(World); return false; }
	Enemy->TeamId = 0;
	Enemy->StableUnitId = SeenUnit;
	Enemy->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
	UGameplayStatics::FinishSpawningActor(Enemy, FTransform::Identity);
	Enemy->DispatchBeginPlay(); // `AbilityCooldowns` si allinea al kit qui: senza, `ConsumeAbility` non scrive

	constexpr int32 SeenPosition = 1;  // InterceptShot, con una ricarica di catalogo
	constexpr int32 ShotPosition = 0;  // PulseShot, senza
	const URTActionData* Intercept = Enemy->GetAbility(SeenPosition);
	const URTActionData* Shot = Enemy->GetAbility(ShotPosition);
	if (!TestTrue(TEXT("premessa: la posizione 1 ha una ricarica di catalogo e la 0 no"),
		Intercept != nullptr && Intercept->CooldownTurns > 0 && Shot != nullptr && Shot->CooldownTurns == 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	// Un'altra azione CON ricarica di catalogo, per il caso «l'uso e' di un'altra azione».
	int32 OtherPosition = INDEX_NONE;
	for (int32 I = 0; I < Enemy->NumAbilities() && OtherPosition == INDEX_NONE; ++I)
	{
		const URTActionData* Candidate = Enemy->GetAbility(I);
		if (I != SeenPosition && Candidate != nullptr && Candidate->CooldownTurns > 0 && Candidate->Def.ActionId != Intercept->Def.ActionId) { OtherPosition = I; }
	}
	if (!TestNotEqual(TEXT("premessa: un'altra azione del kit ha una ricarica di catalogo"), OtherPosition, static_cast<int32>(INDEX_NONE)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const auto Ledgered = [](FName ActionId, int32 Turn, const FRTKnowledgeVerdict& Verdict)
	{
		FRTTurnLogEntry Used = SeenHit(ActionId, Turn);
		Used.UnitId = SeenUnit;
		Used.Verdict = Verdict;
		FRTSeenUseLedger Ledger;
		Ledger.Observe(Used, TArray<int32>{ 0, SeenObserverTeam });
		return Ledger;
	};
	const FRTSeenUseLedger Seen = Ledgered(Intercept->Def.ActionId, 4, SeenOnlyTeam(SeenObserverTeam));
	if (!TestEqual(TEXT("premessa: il registro ha visto l'uso dalla squadra osservatrice"),
		Seen.LastSeenTurn(SeenObserverTeam, SeenUnit, Intercept->Def.ActionId), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const auto ViewOf = [&](int32 PlayerTeam, const FRTSeenUseLedger* Ledger, int32 Hover)
	{
		return URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Enemy, PlayerTeam, true, Ledger, Hover));
	};
	const auto EstimateLines = [](const FRTInspectView& View) { return SeenLinesWith(View, TEXT("Stima")); };

	// --- il controllo positivo: la skill vista usare, col cursore sopra ---------------------------------------------
	const FRTInspectSubject SeenSubject = ARTHUD::BuildInspectSubject(Enemy, SeenObserverTeam, true, &Seen, SeenPosition);
	TestEqual(TEXT("controllo positivo: i dettagli portano UNA stima"), SeenSubject.HoveredSkill.Deduced.Num(), 1);
	const FRTInspectView SeenView = URTInspectLibrary::Compose(SeenSubject);
	const TArray<FString> Estimate = EstimateLines(SeenView);
	if (!TestEqual(TEXT("controllo positivo: e una riga di stima"), Estimate.Num(), 1)) { RTWorldFixtures::DestroyWorld(World); return false; }
	TestTrue(TEXT("la stima dice l'ultimo turno visto"), Estimate[0].Contains(TEXT("turno 4")));
	TestTrue(TEXT("e quando torna disponibile, dal catalogo"), Estimate[0].Contains(FString::Printf(TEXT("dal turno %d"), 4 + Intercept->CooldownTurns)));
	TestTrue(TEXT("e i dettagli sono quelli della skill: il titolo del tooltip c'e'"),
		SeenView.Lines.Contains(URTHudViewModel::BuildCatalogTooltip(URTHudViewModel::BuildCatalogActionRow(Intercept, SeenPosition, NAME_None)).Title.ToString()));

	// Mai un contatore residuo: la ricarica VERA, in corso, non cambia la riga ne' la aggiunge. La premessa e' che sia davvero in corso.
	Enemy->ConsumeAbility(SeenPosition);
	if (TestTrue(TEXT("premessa: la ricarica vera di quella posizione e' in corso"), Enemy->GetAbilityCooldown(SeenPosition) > 0))
	{
		TestEqual(TEXT("con la ricarica vera in corso la riga di stima e' la stessa"),
			EstimateLines(ViewOf(SeenObserverTeam, &Seen, SeenPosition)), Estimate);
		TestEqual(TEXT("e i dettagli interi sono gli stessi, riga per riga"), SeenText(ViewOf(SeenObserverTeam, &Seen, SeenPosition)), SeenText(SeenView));
	}

	// --- ognuno dei casi che la tolgono, per una sola ragione ---------------------------------------------------------
	TestEqual(TEXT("senza il cursore su una skill: nessuna stima, benche' il registro l'abbia vista"),
		EstimateLines(ViewOf(SeenObserverTeam, &Seen, INDEX_NONE)).Num(), 0);
	TestFalse(TEXT("e nessuna sezione «Ricariche»"), SeenText(ViewOf(SeenObserverTeam, &Seen, INDEX_NONE)).Contains(TEXT("Ricariche")));
	TestEqual(TEXT("col cursore su un'altra posizione, mai vista usare: nessuna stima"), EstimateLines(ViewOf(SeenObserverTeam, &Seen, ShotPosition)).Num(), 0);
	TestEqual(TEXT("col cursore su un'altra azione con ricarica, mai vista usare: nessuna stima"),
		EstimateLines(ViewOf(SeenObserverTeam, &Seen, OtherPosition)).Num(), 0);
	TestEqual(TEXT("senza registro: nessuna stima"), EstimateLines(ViewOf(SeenObserverTeam, nullptr, SeenPosition)).Num(), 0);
	FRTSeenUseLedger Empty;
	TestEqual(TEXT("con un registro vuoto: nessuna stima"), EstimateLines(ViewOf(SeenObserverTeam, &Empty, SeenPosition)).Num(), 0);

	const FRTSeenUseLedger OtherAction = Ledgered(Enemy->GetAbility(OtherPosition)->Def.ActionId, 4, SeenOnlyTeam(SeenObserverTeam));
	TestEqual(TEXT("l'uso e' di un'ALTRA azione: nessuna stima sulla skill sotto il cursore"), EstimateLines(ViewOf(SeenObserverTeam, &OtherAction, SeenPosition)).Num(), 0);
	TestEqual(TEXT("controllo positivo: e quella stessa azione, col cursore sul suo slot, ha la sua"),
		EstimateLines(ViewOf(SeenObserverTeam, &OtherAction, OtherPosition)).Num(), 1);

	const FRTSeenUseLedger Unseen = Ledgered(Intercept->Def.ActionId, 4, FRTKnowledgeVerdict::NoOne());
	TestEqual(TEXT("un uso FUORI VISTA non lascia riga: nessuna stima"), EstimateLines(ViewOf(SeenObserverTeam, &Unseen, SeenPosition)).Num(), 0);
	TestEqual(TEXT("e il registro fuori vista e' vuoto (la premessa)"), Unseen.Num(), 0);

	const FRTSeenUseLedger ShotUsed = Ledgered(Shot->Def.ActionId, 4, SeenOnlyTeam(SeenObserverTeam));
	TestEqual(TEXT("premessa: il registro ha visto l'uso del colpo, che non ha ricarica di catalogo"),
		ShotUsed.LastSeenTurn(SeenObserverTeam, SeenUnit, Shot->Def.ActionId), 4);
	TestEqual(TEXT("un'azione senza ricarica di catalogo, anche vista usare: nessuna stima («torna disponibile subito» non e' una stima)"),
		EstimateLines(ViewOf(SeenObserverTeam, &ShotUsed, ShotPosition)).Num(), 0);

	// Un'unita' della squadra di chi guarda: si leggono le ricariche vere, non le dedotte. Il registro la CONOSCE dalla sua stessa squadra, o l'asserto
	// non eserciterebbe la guardia (e la stessa scheda, letta da un'altra squadra, ha la sua stima: controllo positivo).
	const FRTSeenUseLedger KnowsBothSides = Ledgered(Intercept->Def.ActionId, 4, FRTKnowledgeVerdict::Everyone());
	TestEqual(TEXT("controllo positivo: lo stesso registro, letto da chi non e' della sua squadra, ha la stima"),
		EstimateLines(ViewOf(SeenObserverTeam, &KnowsBothSides, SeenPosition)).Num(), 1);
	TestEqual(TEXT("un'unita' della propria squadra: nessuna stima, anche se il registro la conosce"),
		EstimateLines(ViewOf(/*PlayerTeam=*/ 0, &KnowsBothSides, SeenPosition)).Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
