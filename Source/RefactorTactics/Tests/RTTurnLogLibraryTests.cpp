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
		E.ActionId = FName(TEXT("Hero.Ivrin.PulseShot"));
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
		// ⚠️ **A discriminare e' il TOKEN e non piu' l'esito** (`#1118`): le due voci qui sotto hanno la
		// STESSA ragione — `Chosen`, ha deciso — e portano risposte diverse. E' il caso che il vecchio
		// `FireChosen`/`HoldChosen` rendeva impossibile scrivere, perche' la risposta non aveva un campo.
		Overwatch.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::Chosen);
		Overwatch.ReactionResponse = TEXT("FIRE:3");
		TestTrue(TEXT("un overwatch che spara e' danno inflitto"),
			URTTurnLogLibrary::IsDamageInflictedByActor(Overwatch));
		Overwatch.ReactionResponse = TEXT("HOLD");
		TestFalse(TEXT("chi tiene il fuoco non infligge niente, a parita' di ragione"),
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

/**
 * `TargetUnknown` deve avere un testo PROPRIO. Cade nel `default` -> il giocatore legge «non eseguibile»,
 * che e' esattamente cio' che la conoscenza parziale NON sta dicendo: non «non si puo'», ma «per la tua
 * squadra quel bersaglio non c'e'».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogTargetUnknownIsDescribedTest,
	"RefactorTactics.TurnLog.TargetUnknownIsDescribed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogTargetUnknownIsDescribedTest::RunTest(const FString&)
{
	const FString Fallback = URTTurnLogLibrary::DescribeInvalidReason(
		ERTActionInvalidReason::InsufficientMovementPoints);
	const FString Unknown = URTTurnLogLibrary::DescribeInvalidReason(
		ERTActionInvalidReason::TargetUnknown);

	// ⚠️ Anti-vacuita': senza questa riga il test passerebbe anche cambiando il TESTO DEL DEFAULT invece di
	// aggiungere il case. `InsufficientMovementPoints` non ha un case e per D-190 non ne avra' mai uno,
	// quindi e' la sonda giusta per dimostrare che il default e' ancora raggiungibile e ancora quello.
	TestEqual(TEXT("il default esiste ancora ed e' invariato"), Fallback, TEXT("non eseguibile"));

	TestNotEqual(TEXT("TargetUnknown non cade piu' nel default"), Unknown, Fallback);
	TestEqual(TEXT("e dice di CONOSCENZA, non di geometria"),
		Unknown, TEXT("bersaglio ignoto alla squadra"));
	return true;
}

/**
 * `TurnLog.EveryMoveOutcomeIsDescribed` — **ogni** valore di `ERTMoveOutcome` ha un testo, e il ramo di
 * fallback resta raggiungibile (`#2628`).
 *
 * 🔴 **Il difetto che questo test esiste per rendere impossibile.** Misurato il 2026-09-12: `DescribeEntry`
 * traduceva **13 valori su 20** e i restanti sette — `BlockedByTopology`, `BlockedByCycle` e le quattro
 * cadute, piu' `StoppedByEdgeGuard` — cadevano nel fallback, che stampa *«esito di movimento non tradotto
 * (15)»*. Due di loro si osservavano in una run normale della suite. Nulla era rosso: l'esaustivita'
 * dell'enum non era coperta da nessuna parte, ed e' la ragione per cui quattro valori aggiunti dopo la
 * stesura della issue erano gia' rimasti indietro a loro volta.
 *
 * ⚠️ **Il fallback NON viene tolto e questo test lo tiene vivo**, con la stessa disciplina anti-vacuita' di
 * `TargetUnknownIsDescribed`: la prima asserzione lo raggiunge con un byte fuori enum. Senza, cancellare il
 * `default` renderebbe il test verde per la ragione sbagliata — e il prossimo valore aggiunto in coda
 * leggerebbe «resta» invece di dichiararsi non tradotto.
 *
 * ⛔ **Non asserisce QUALE sia il testo di ciascun esito**: quelle sono decisioni di vocabolario che
 * appartengono al codice, e pinnarle qui renderebbe rosso ogni ritocco di una frase. Cio' che si fissa e'
 * che un testo ci sia, che non sia il fallback, e che non sia quello di un altro esito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEveryMoveOutcomeIsDescribedTest,
	"RefactorTactics.TurnLog.EveryMoveOutcomeIsDescribed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEveryMoveOutcomeIsDescribedTest::RunTest(const FString&)
{
	const FRTCellId Src(1, -2, 0);
	const FRTCellId Tgt(2, -2, 1);

	auto Describe = [&Src, &Tgt](uint8 RawOutcome)
	{
		return URTTurnLogLibrary::DescribeEntry(
			MakeEntry(ERTMatchPhase::Move, ERTLogCategory::Move, RawOutcome, Src, Tgt, /*Amount=*/ 2));
	};

	// 🔑 **IL CONTROLLO POSITIVO, e viene prima.** `200` non e' un enumeratore e non lo diventera': e' la
	// sonda che dimostra che il ramo di fallback e' ancora li' e ancora quello. Se un giorno sparisse, questa
	// riga cade — e cade PRIMA che il resto del test dichiari verde un'esaustivita' ottenuta togliendo la rete.
	const FString FuoriEnum = Describe(200);
	TestTrue(TEXT("il ramo di fallback e' ancora raggiungibile e si dichiara tale"),
		FuoriEnum.Contains(TEXT("non tradotto")) && FuoriEnum.Contains(TEXT("(200)")));

	const UEnum* Enum = StaticEnum<ERTMoveOutcome>();
	if (!TestNotNull(TEXT("l'enum degli esiti di movimento e' riflesso"), Enum))
	{
		return false;
	}

	// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
	const int32 Valori = Enum->NumEnums() - 1;
	TestTrue(TEXT("l'enum ha dei valori da coprire"), Valori > 0);

	TMap<FString, FString> TestoPerEsito;
	for (int32 i = 0; i < Valori; ++i)
	{
		const int64 Valore = Enum->GetValueByIndex(i);
		const FString Nome = Enum->GetNameStringByIndex(i);
		const FString Testo = Describe(static_cast<uint8>(Valore));

		// Non cade nel fallback. Si guardano DUE cose e non una: il testo letterale della rete, che coglie
		// anche un `default` riformulato, e l'uguaglianza con cio' che il fallback produrrebbe per QUESTO
		// valore, che coglie un `case` scritto per copia e rimasto identico alla rete.
		TestFalse(FString::Printf(TEXT("%s non cade nel ramo di fallback"), *Nome),
			Testo.Contains(TEXT("non tradotto")));
		TestNotEqual(FString::Printf(TEXT("%s non rende come il fallback"), *Nome),
			Testo, FuoriEnum.Replace(TEXT("(200)"), *FString::Printf(TEXT("(%lld)"), Valore)));
		TestFalse(FString::Printf(TEXT("%s ha un testo non vuoto"), *Nome), Testo.IsEmpty());

		// ⚠️ **Due esiti con la STESSA riga sono un buco che le prove sopra non vedono.** La DoD di `#2628`
		// chiede un testo che dica *«cosa e' successo»*, e due valori che si leggono identici non lo dicono:
		// e' il modo in cui un `case` aggiunto per copia passa la revisione. Le quattro cadute sono
		// precisamente il caso in cui e' facile che accada.
		if (const FString* Gemello = TestoPerEsito.Find(Testo))
		{
			AddError(FString::Printf(TEXT("%s e %s si leggono identici: '%s'"), *Nome, **Gemello, *Testo));
		}
		else
		{
			TestoPerEsito.Add(Testo, Nome);
		}
	}

	return true;
}

/**
 * **Una voce AMBIENTALE non si racconta come un attacco** (`#3110`).
 *
 * 🔴 `Entry.Outcome` e' un `uint8` condiviso il cui enum dipende dalla categoria. Prima di `#3110`
 * `DescribeEntry` cadeva nello switch di `ERTCombatOutcome` senza guardia, e una voce ambientale veniva
 * reinterpretata **per posizione**: `SurfaceChanged` (0) letta come `Hit` (0) usciva come *«N danni»*, con
 * `Amount` — che per una superficie e' la DURATA IN TURNI — stampato come danno inflitto.
 *
 * Trovato leggendo il TurnLog di una partita vera: sette righe `2 danni (Hero.Muiren.MistVeil)` per
 * un'azione che non dichiara danno, e il `2` era la durata del fumo.
 *
 * ⛔ **Verifica di mutazione**: tolta la guardia `Category == Environment` da `DescribeEntry`, i tre
 * asserti qui sotto diventano rossi e il quarto — il colpo vero — resta verde. E' la coppia che distingue
 * «il rendering ambientale e' sbagliato» da «il rendering e' sbagliato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogEnvironmentIsNotCombatTest,
	"RefactorTactics.TurnLog.EnvironmentEntriesAreNotNarratedAsCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogEnvironmentIsNotCombatTest::RunTest(const FString&)
{
	const FRTCellId C(1, 0, 0);

	// La superficie creata: `Amount` sono i TURNI, non i danni.
	FRTTurnLogEntry Surface = MakeEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Environment,
		static_cast<uint8>(ERTEnvironmentOutcome::SurfaceChanged), C, C, /*Amount*/ 2);
	Surface.ActionId = FName(TEXT("Hero.Muiren.MistVeil"));
	const FString SurfaceText = URTTurnLogLibrary::DescribeEntry(Surface);

	TestFalse(TEXT("una superficie che cambia non infligge danni"),
		SurfaceText.Contains(TEXT("danni")));
	TestTrue(TEXT("e dice invece cosa e' successo"),
		SurfaceText.Contains(TEXT("la superficie cambia")));

	// 🔴 Il caso che costa di piu' se torna: un rifiuto raccontato come un'uccisione, perche'
	// `SurfaceRejected` e `Lethal` valgono entrambi 2.
	FRTTurnLogEntry Rejected = MakeEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Environment,
		static_cast<uint8>(ERTEnvironmentOutcome::SurfaceRejected), C, C, /*Amount*/ 0);
	TestFalse(TEXT("una trasformazione RIFIUTATA non elimina nessuno"),
		URTTurnLogLibrary::DescribeEntry(Rejected).Contains(TEXT("eliminata")));

	// Il gemello che NON deve muoversi: stessa posizione d'enum, categoria `Combat`.
	FRTTurnLogEntry Hit = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), C, C, /*Amount*/ 2);
	TestTrue(TEXT("un colpo vero resta un colpo"),
		URTTurnLogLibrary::DescribeEntry(Hit).Contains(TEXT("danni")));

	return true;
}

/**
 * **Una voce di OBIETTIVO si descrive** (`#3110`, criterio 2).
 *
 * `#3114` ha chiuso la caduta nello switch di combattimento, ma ha lasciato aperta la domanda che questo
 * test risponde: `ReactionClash` e `Objective` passavano dal ramo che le DICHIARA senza descrittore, e se
 * producessero voci reali **non era stato misurato**.
 *
 * Misurato su `origin/main` `b0d7de96`, e le due meta' hanno esito opposto:
 *
 * - `Objective` **ha** un produttore non-test raggiungibile in partita -- `ARTTurnManager`, nel Cleanup,
 *   `Objective.Control` (`RTTurnManager.cpp:1780`), scritto a ogni turno in cui la mappa ha un obiettivo,
 *   `Unclaimed` e `Contested` compresi. Serve il suo descrittore, ed e' questo test;
 * - `ReactionClash` **non ne ha**: `MakeClashLogEntries` ha tre chiamanti e sono **tutti test**
 *   (`RTOverwatchTriggerTests.cpp`). `RTScenarioSession.cpp:336` lo dichiara -- *«nessun punto del resolver
 *   le chiama»* -- ed e' la capability BLOCCATA di `#314`. Resta al ramo che la dichiara: il suo descrittore
 *   nascera' col chiamante, e scriverlo ora sarebbe la guardia a tappeto che il criterio vieta.
 *
 * `Amount` porta i PUNTI assegnati, non danni: zero quando l'obiettivo e' conteso o di nessuno -- lo
 * dichiara la riga che lo scrive.
 *
 * 🔑 La collisione da cui #3110 nasce e' viva anche qui: `Team0Scores` e `ERTCombatOutcome::Lethal` valgono
 * **entrambi 2**. Senza guardia una squadra che segna uscirebbe come un'unita' eliminata.
 *
 * ⛔ **Verifica di mutazione**: tolta la guardia `Category == Objective`, le voci tornano al ramo
 * *«senza descrittore»* e i primi asserti diventano rossi; il colpo vero -- il controllo -- resta verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogObjectiveIsDescribedTest,
	"RefactorTactics.TurnLog.ObjectiveEntriesAreDescribed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogObjectiveIsDescribedTest::RunTest(const FString&)
{
	// Le due celle coincidono, come le scrive il produttore: la voce non descrive uno spostamento.
	const FRTCellId O(2, -1, 0);

	auto Obiettivo = [&O](ERTObjectiveOutcome Esito, int32 Punti)
	{
		FRTTurnLogEntry E = MakeEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Objective,
			static_cast<uint8>(Esito), O, O, Punti);
		E.ActionId = FName(TEXT("Objective.Control"));
		return URTTurnLogLibrary::DescribeEntry(E);
	};

	// 🔑 **IL CONTROLLO POSITIVO, e viene prima** -- la stessa disciplina di
	// `EveryMoveOutcomeIsDescribed`. Dare a `Objective` il suo descrittore lo toglie dalla rete, e vi lascia
	// `ReactionClash` come unico consumatore: senza questa riga, un refactor che cancellasse la guardia
	// `Category != Combat` non farebbe cadere NIENTE qui, e le voci di clash tornerebbero a uscire come danni
	// -- cioe' `#3110` verbatim, sulla sola categoria che questa misura ha lasciato di proposito alla rete.
	const FRTTurnLogEntry Clash = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::ReactionClash,
		static_cast<uint8>(ERTClashLogEvent::OpportunityCreated), O, O, /*Amount*/ 3);
	TestTrue(TEXT("la rete e' ancora li': una categoria senza descrittore si DICHIARA"),
		URTTurnLogLibrary::DescribeEntry(Clash).Contains(TEXT("senza descrittore")));

	const FString Segna0 = Obiettivo(ERTObjectiveOutcome::Team0Scores, /*Punti*/ 1);
	TestFalse(TEXT("una squadra che segna non esce piu' come categoria senza descrittore"),
		Segna0.Contains(TEXT("senza descrittore")));
	TestTrue(TEXT("dice CHI controlla l'obiettivo"), Segna0.Contains(TEXT("squadra 0")));
	TestTrue(TEXT("e quanti punti ha preso"), Segna0.Contains(TEXT("(+1)")));
	TestFalse(TEXT("e non elimina nessuno: `Team0Scores` e `Lethal` valgono entrambi 2"),
		Segna0.Contains(TEXT("eliminata")));

	// La SIMMETRICA, e non e' ridondante: senza, l'unico rilevatore di un `case` copiato sarebbe la mappa dei
	// gemelli qui sotto, che distingue le due squadre SOLO perche' i loro `Amount` coincidono. Cambiane uno e
	// un punto della squadra 1 verrebbe raccontato come un punto della squadra 0, con la suite verde.
	const FString Segna1 = Obiettivo(ERTObjectiveOutcome::Team1Scores, /*Punti*/ 1);
	TestTrue(TEXT("e la squadra 1 e' la squadra 1"), Segna1.Contains(TEXT("squadra 1")));

	// Conteso: la voce si scrive anche quando il punteggio non si muove, ed e' voluto -- senza, un turno
	// conteso e un turno vuoto sarebbero indistinguibili, e la contesa e' cio' che il checkpoint aggiunge.
	const FString Conteso = Obiettivo(ERTObjectiveOutcome::Contested, /*Punti*/ 0);
	TestFalse(TEXT("un obiettivo conteso non esce come categoria senza descrittore"),
		Conteso.Contains(TEXT("senza descrittore")));
	// `"(+"` e non `"+"`: il secondo scandaglia la riga INTERA -- cella e coda dell'azione comprese -- e
	// passerebbe per accidente, cadendo il giorno in cui un `ActionId` o un assiale firmato portasse un `+`.
	TestFalse(TEXT("e non assegna punti a nessuno"), Conteso.Contains(TEXT("(+")));

	// `Amount` FUORI DAL PLAUSIBILE. `DescribeEntry` gira anche su tracce deserializzate e su log di scenario,
	// dove `Outcome` e `Amount` sono campi indipendenti: un `Team0Scores` con zero punti e' una contraddizione
	// -- dice «segna» e «niente» -- e `(+0)` la renderebbe come una frase sicura e sbagliata. E' la stessa
	// ragione per cui `HitCameFromSide` rifiuta di indovinare invece di clampare.
	const FString Assurdo = Obiettivo(ERTObjectiveOutcome::Team0Scores, /*Punti*/ 0);
	TestFalse(TEXT("una squadra che segna zero punti non dichiara «+0»"),
		Assurdo.Contains(TEXT("(+0)")));
	TestTrue(TEXT("lo dichiara non tradotto, invece di indovinare"),
		Assurdo.Contains(TEXT("non tradotti")));

	// 🔑 **L'esaustivita' si RIFLETTE, non si elenca a mano.** Un quinto enumeratore aggiunto domani in
	// `ERTObjectiveOutcome` finisce qui da solo; un elenco scritto a mano lo mancherebbe, e la voce nuova
	// cadrebbe nella rete -- cioe' proprio lo stato che `#3110` esiste per chiudere -- con questo test verde.
	// ⚠️ E la rete NON e' `-Wswitch`: la build non promuove i warning a errori (nessun `bWarningsAsErrors`
	// nei `.Build.cs` ne' nei `.Target.cs`, misurato), quindi il gate e' questo ciclo e nient'altro.
	const UEnum* Enum = StaticEnum<ERTObjectiveOutcome>();
	if (!TestNotNull(TEXT("l'enum degli esiti di obiettivo e' riflesso"), Enum))
	{
		return false;
	}

	// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
	const int32 Valori = Enum->NumEnums() - 1;
	TestTrue(TEXT("l'enum ha dei valori da coprire"), Valori > 0);

	TMap<FString, FString> TestoPerEsito;
	for (int32 i = 0; i < Valori; ++i)
	{
		const FString Nome = Enum->GetNameStringByIndex(i);
		const FString Testo = Obiettivo(
			static_cast<ERTObjectiveOutcome>(Enum->GetValueByIndex(i)), /*Punti*/ 1);

		TestFalse(FString::Printf(TEXT("%s ha il suo descrittore"), *Nome),
			Testo.Contains(TEXT("senza descrittore")));

		// Due frasi identiche renderebbero il log incapace di distinguere «nessuno era presente» da «erano in
		// due», che e' precisamente cio' che la contesa aggiunge alla partita.
		if (const FString* Gemello = TestoPerEsito.Find(Testo))
		{
			AddError(FString::Printf(TEXT("%s e %s si leggono identici: '%s'"),
				*Nome, **Gemello, *Testo));
		}
		else
		{
			TestoPerEsito.Add(Testo, Nome);
		}
	}

	// Il controllo di combattimento, che distingue «il rendering dell'obiettivo e' sbagliato» da «il rendering
	// e' sbagliato». ⚠️ Duplica quello di `EnvironmentEntriesAreNotNarratedAsCombat`: estrarlo tocca un test
	// gia' mergiato, ed e' FOLLOW-UP -- un helper usato da uno solo non e' un helper.
	const FRTTurnLogEntry Colpo = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), O, O, /*Amount*/ 2);
	TestTrue(TEXT("un colpo vero resta un colpo"),
		URTTurnLogLibrary::DescribeEntry(Colpo).Contains(TEXT("danni")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
