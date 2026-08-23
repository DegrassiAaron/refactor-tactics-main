#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTTurnLogEntry MakeEntry(ERTMatchPhase Phase, ERTLogCategory Cat, uint8 Outcome,
		const FRTCellId& Src, const FRTCellId& Tgt, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.Phase = Phase;
		E.Category = Cat;
		E.Outcome = Outcome;
		E.SrcCell = Src;
		E.TgtCell = Tgt;
		E.Amount = Amount;
		return E;
	}
}

// EntryLess: ordine TOTALE -> distingue anche l'ULTIMO campo della catena, antisimmetrico.
// L'ultimo non e' piu' `Amount` da un pezzo: dopo di lui vengono `ActionId`, poi `TurnNumber`,
// `GraphRevision` e `UnitId` (v6, D-063/D-067), e da ultimo `Priority` (v7, #79). La catena
// autorevole sta in `spec-turnlog.md` §6 — qui si cita, non si duplica, perche' un elenco copiato
// e' esattamente cio' che e' invecchiato tre volte.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogEntryLessTest,
	"RefactorTactics.TurnLog.EntryLessTotalOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogEntryLessTest::RunTest(const FString&)
{
	const FRTCellId C(1, 1);
	const FRTTurnLogEntry A = MakeEntry(ERTMatchPhase::Move, ERTLogCategory::Move, 0, C, C, 5);
	const FRTTurnLogEntry B = MakeEntry(ERTMatchPhase::Move, ERTLogCategory::Move, 0, C, C, 10); // solo Amount diverso
	TestTrue(TEXT("A<B per Amount (ordine totale fino all'ultimo campo)"), URTTurnLogLibrary::EntryLess(A, B));
	TestFalse(TEXT("non B<A (antisimmetrico)"), URTTurnLogLibrary::EntryLess(B, A));
	return true;
}

// HashTurnLog: permutazione-invariante (stesse voci in ordine diverso -> stesso hash) e sensibile alle differenze.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogHashTest,
	"RefactorTactics.TurnLog.HashPermutationInvariantAndSensitive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogHashTest::RunTest(const FString&)
{
	const FRTCellId C1(1, 1), C2(2, 2), C3(3, 3);
	const FRTTurnLogEntry E1 = MakeEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   1, C1, C2, 3);
	const FRTTurnLogEntry E2 = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, 2, C2, C3, 7);
	const FRTTurnLogEntry E3 = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, 2, C2, C3, 8); // Amount 8 != 7

	TArray<FRTTurnLogEntry> L12; L12.Add(E1); L12.Add(E2);
	TArray<FRTTurnLogEntry> L21; L21.Add(E2); L21.Add(E1); // stesse voci, ordine inverso
	TArray<FRTTurnLogEntry> L13; L13.Add(E1); L13.Add(E3); // una voce differisce (Amount)

	const uint32 H12 = URTTurnLogLibrary::HashTurnLog(L12);
	const uint32 H21 = URTTurnLogLibrary::HashTurnLog(L21);
	const uint32 H13 = URTTurnLogLibrary::HashTurnLog(L13);

	TestEqual(TEXT("permutazione-invariante: [E1,E2] == [E2,E1]"), H12, H21);
	TestNotEqual(TEXT("sensibile: [E1,E2] != [E1,E3]"), H12, H13);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Descrizione leggibile: il reason code deve arrivare al giocatore, con le coordinate assiali
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogDescribeTest,
	"RefactorTactics.TurnLog.DescribeEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogDescribeTest::RunTest(const FString&)
{
	FRTTurnLogEntry Contested;
	Contested.Phase = ERTMatchPhase::Move;
	Contested.Category = ERTLogCategory::Move;
	Contested.Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedContested);
	Contested.SrcCell = FRTCellId(1, -2, 0);
	Contested.TgtCell = FRTCellId(1, -2, 0);
	Contested.Amount = 0;

	const FString ContestedText = URTTurnLogLibrary::DescribeEntry(Contested);
	TestTrue(TEXT("la cella e' in coordinate assiali (q,r,L)"), ContestedText.Contains(TEXT("q=1")) && ContestedText.Contains(TEXT("r=-2")));
	TestTrue(TEXT("il motivo e' leggibile"), ContestedText.Contains(TEXT("contesa")));

	FRTTurnLogEntry NoLos;
	NoLos.Phase = ERTMatchPhase::Blast;
	NoLos.Category = ERTLogCategory::Combat;
	NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	NoLos.SrcCell = FRTCellId(0, 0, 0);
	NoLos.TgtCell = FRTCellId(3, 0, 1);

	const FString NoLosText = URTTurnLogLibrary::DescribeEntry(NoLos);
	TestTrue(TEXT("compaiono attaccante e bersaglio"), NoLosText.Contains(TEXT("q=0")) && NoLosText.Contains(TEXT("q=3")));
	TestTrue(TEXT("il layer del bersaglio e' visibile"), NoLosText.Contains(TEXT("L=1")));
	TestTrue(TEXT("il motivo e' leggibile"), NoLosText.Contains(TEXT("linea di tiro")));

	FRTTurnLogEntry Lethal;
	Lethal.Phase = ERTMatchPhase::Blast;
	Lethal.Category = ERTLogCategory::Combat;
	Lethal.Outcome = static_cast<uint8>(ERTCombatOutcome::Lethal);
	Lethal.SrcCell = FRTCellId(0, 0, 0);
	Lethal.TgtCell = FRTCellId(1, 0, 0);
	Lethal.Amount = 40;

	const FString LethalText = URTTurnLogLibrary::DescribeEntry(Lethal);
	TestTrue(TEXT("il danno compare"), LethalText.Contains(TEXT("40")));
	TestTrue(TEXT("l'esito letale e' dichiarato"), LethalText.Contains(TEXT("elimin")));

	// Voci diverse devono leggersi diverse: una descrizione costante passerebbe le prove precedenti.
	TestNotEqual(TEXT("descrizioni distinte per esiti distinti"), ContestedText, NoLosText);
	TestNotEqual(TEXT("descrizioni distinte per danno diverso"), LethalText, NoLosText);
	return true;
}

/**
 * `TurnLog.InflictedDamageExcludesWhatItSays` — le esclusioni documentate di `IsDamageInflictedByActor`
 * hanno un test, invece di vivere in un commento (`#1150`).
 *
 * 🔴 **Senza questo, la meta' del predicato non era coperta.** L'unico test del lavoro era uno scenario
 * d'integrazione che esercita `Hit` e le due cause ambientali: cambiare `TerrainBonus` in `default:`, o
 * aggiungere `Healed` agli accettati — che gonfierebbe ogni aggregazione dell'importo curato — lasciava la
 * suite intera verde. Trovato in code review.
 *
 * ⚠️ Voci costruite a mano di proposito: qui si misura il PREDICATO, non chi scrive le voci. Che qualcuno
 * le scriva davvero lo prova `Actions.Hazard.SufferedAndInflictedAreTellableApart`, sul percorso vero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInflictedDamagePredicateTest,
	"RefactorTactics.TurnLog.InflictedDamageExcludesWhatItSays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInflictedDamagePredicateTest::RunTest(const FString&)
{
	// Un colpo qualunque: attore dichiarato, celle diverse, esito di danno.
	auto Colpo = [](ERTCombatOutcome Esito)
	{
		FRTTurnLogEntry E;
		E.Category = ERTLogCategory::Combat;
		E.Outcome = static_cast<uint8>(Esito);
		E.UnitId = 7;
		E.SrcCell = FRTCellId(0, 0, 0);
		E.TgtCell = FRTCellId(1, 0, 0);
		E.Amount = 20;
		E.ActionId = FName(TEXT("Hero.Wraith.PulseShot"));
		return E;
	};

	// I QUATTRO che contano.
	TestTrue(TEXT("Hit e' danno inflitto"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::Hit)));
	TestTrue(TEXT("ShieldAbsorbed lo e' (il colpo e' arrivato, lo scudo l'ha retto)"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::ShieldAbsorbed)));
	TestTrue(TEXT("Lethal lo e'"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::Lethal)));
	TestTrue(TEXT("TerrainBonus lo e': e' un colpo a segno, con un bonus di posizione"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::TerrainBonus)));

	// I DUE esclusi, e per ragioni opposte.
	TestFalse(TEXT("Healed no: ha un agente vero, ma non e' danno"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::Healed)));
	TestFalse(TEXT("NoLineOfSight no: ha agente e categoria giusti, e zero danno"),
		URTTurnLogLibrary::IsDamageInflictedByActor(Colpo(ERTCombatOutcome::NoLineOfSight)));

	// Lo ZERO non e' un attore.
	{
		FRTTurnLogEntry Anonima = Colpo(ERTCombatOutcome::Hit);
		Anonima.UnitId = 0;
		TestFalse(TEXT("`UnitId == 0` significa «nessuna unita' dichiarata», non un attore"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Anonima));
	}

	// Le due CAUSE ambientali, e la rete che le prende anche se l'elenco non le conosce.
	{
		FRTTurnLogEntry Terreno = Colpo(ERTCombatOutcome::Hit);
		Terreno.ActionId = FName(TEXT("Terrain.Fire"));
		Terreno.TgtCell = Terreno.SrcCell;
		TestTrue(TEXT("Terrain.* e' ambientale"), URTTurnLogLibrary::IsEnvironmentalDamage(Terreno));
		TestFalse(TEXT("e non e' danno inflitto"), URTTurnLogLibrary::IsDamageInflictedByActor(Terreno));

		FRTTurnLogEntry Stato = Colpo(ERTCombatOutcome::Hit);
		Stato.ActionId = FName(TEXT("Status.Burning"));
		Stato.TgtCell = Stato.SrcCell;
		TestTrue(TEXT("Status.Burning e' ambientale"), URTTurnLogLibrary::IsEnvironmentalDamage(Stato));

		// 🔴 **La rete: una causa ambientale che l'elenco NON conosce.** E' il caso di `#1077`, che sta
		// portando gli stati nel TurnLog. Senza `SrcCell == TgtCell` questa voce risulterebbe danno
		// INFLITTO, cioe' accreditata a chi la subisce — il verso pericoloso.
		FRTTurnLogEntry Ignota = Colpo(ERTCombatOutcome::Hit);
		Ignota.ActionId = FName(TEXT("Status.Poison"));
		Ignota.TgtCell = Ignota.SrcCell;
		TestTrue(TEXT("una causa ambientale ignota fallisce CHIUSO, non aperto"),
			URTTurnLogLibrary::IsEnvironmentalDamage(Ignota));
		TestFalse(TEXT("e non viene accreditata a chi la subisce"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Ignota));
	}

	// Le altre due categorie che portano danno inflitto, e i loro esiti che non lo portano.
	{
		FRTTurnLogEntry Previsione;
		Previsione.Category = ERTLogCategory::Predictive;
		Previsione.UnitId = 7;
		Previsione.SrcCell = FRTCellId(0, 0, 0);
		Previsione.TgtCell = FRTCellId(2, 0, 0);
		Previsione.Amount = 16;
		Previsione.Outcome = static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched);
		TestTrue(TEXT("una previsione azzeccata e' danno inflitto"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Previsione));
		Previsione.Outcome = static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed);
		TestFalse(TEXT("un whiff no: la voce esiste, il danno non c'e'"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Previsione));

		FRTTurnLogEntry Overwatch;
		Overwatch.Category = ERTLogCategory::ReactionDecision;
		Overwatch.UnitId = 7;
		Overwatch.SrcCell = FRTCellId(0, 0, 0);
		Overwatch.TgtCell = FRTCellId(2, 0, 0);
		Overwatch.Amount = 12;
		Overwatch.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::FireChosen);
		TestTrue(TEXT("un overwatch che spara e' danno inflitto"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Overwatch));
		Overwatch.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::HoldChosen);
		TestFalse(TEXT("chi tiene il fuoco non infligge niente"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Overwatch));
	}

	// Una categoria che non porta danno: `Fallback` mette in `Amount` un `ERTActionInvalidReason`, non un
	// numero di punti vita. Un predicato «Amount > 0» sommerebbe codici di errore.
	{
		FRTTurnLogEntry Fallback;
		Fallback.Category = ERTLogCategory::Fallback;
		Fallback.UnitId = 7;
		Fallback.Amount = 3;
		TestFalse(TEXT("Fallback non e' danno inflitto"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Fallback));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
