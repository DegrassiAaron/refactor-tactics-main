// La formula del danno di D-589, con le scelte di D-595: gli esempi della tabella di controllo di E53 (CP 53.1)
// diventano asserti sulla funzione pura `URTCombatLibrary::MitigateDirectDamage` (CP 53.2, #4013).
//
// La tabella e' `docs/roadmap/plans/e53-tabella-di-controllo-danno-2026-11-09.md` §3–§4: colpi del catalogo
// di oggi ×10 contro le DEF del GDD. Qui ogni esempio e' un test che chiama la funzione con i NUMERI della
// tabella, non con il catalogo: il catalogo non porta ancora DEF, RES, bonus alle spalle ne' la scala ×10
// (quelli entrano con CP 53.3), e un test che li leggesse dal catalogo oggi sarebbe vuoto o rosso per
// costruzione. Cio' che e' pinnabile adesso e' la forma della formula, e lo e' con il controllo positivo
// in coda: a difesa zero e senza moltiplicatori la funzione e' l'identita' sui valori di oggi, quindi
// NESSUN golden cambia finche' il resolver non la chiama con una difesa diversa da zero.
//
// Copertura e Guardia non compaiono nella funzione: si sottraggono DOPO, piatte (D-589 (3), D-590 (3)).
// L'esempio 3 della tabella lo mostra componendo a mano i due passi.

#include "Misc/AutomationTest.h"
#include "Combat/RTCombatLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Senza bonus alle spalle e senza moltiplicatori: la sola difesa. */
	int32 Mitigate(int32 Base, int32 Defense)
	{
		return URTCombatLibrary::MitigateDirectDamage(Base, /*RearBonus*/ 0, /*ModPercent*/ 100, Defense);
	}
}

/** Esempio 1 — frontale, nudo: PrecisionAttack 240 contro DEF 25 → floor(240 × 100 / 125) = 192. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaFrontalHitAgainstDefense,
	"RefactorTactics.Combat.Formula.FrontalHitAgainstDefense",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaFrontalHitAgainstDefense::RunTest(const FString&)
{
	TestEqual(TEXT("240 contro DEF 25"), Mitigate(240, 25), 192);
	TestEqual(TEXT("350 contro DEF 35"), Mitigate(350, 35), 259);
	TestEqual(TEXT("120 contro DEF 60"), Mitigate(120, 60), 75);
	TestEqual(TEXT("160 contro DEF 15"), Mitigate(160, 15), 139);
	return true;
}

/** Esempio 2 — alle spalle: il bonus dell'attaccante si somma alla Base PRIMA della mitigazione (D-587 (3)). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaRearBonusAddsToBaseBeforeMitigation,
	"RefactorTactics.Combat.Formula.RearBonusAddsToBaseBeforeMitigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaRearBonusAddsToBaseBeforeMitigation::RunTest(const FString&)
{
	// (240 + 60) × 100 / 125 = 240: il bonus e' mitigato come il resto del colpo, non sommato dopo.
	TestEqual(TEXT("240 + 60 alle spalle contro DEF 25"), URTCombatLibrary::MitigateDirectDamage(240, 60, 100, 25), 240);
	TestEqual(TEXT("di fronte il bonus e' zero"), URTCombatLibrary::MitigateDirectDamage(240, 0, 100, 25), 192);
	// D-599: Kallari (90) con Mist Slash 210 contro DEF 25 → 240; Steel (30) con Shield Bash 150 → 144.
	TestEqual(TEXT("Kallari 210 + 90 contro DEF 25"), URTCombatLibrary::MitigateDirectDamage(210, 90, 100, 25), 240);
	TestEqual(TEXT("Steel 150 + 30 contro DEF 25"), URTCombatLibrary::MitigateDirectDamage(150, 30, 100, 25), 144);
	// Sommato DOPO la mitigazione darebbe 252, non 240: e' la differenza che D-587 (3) ha deciso.
	TestNotEqual(TEXT("non e' «mitigato poi sommato»"), URTCombatLibrary::MitigateDirectDamage(240, 60, 100, 25), 192 + 60);
	return true;
}

/** Esempio 7 — `Exposed` come moltiplicatore 1,2 in Mod (D-595 (2)): 240 × 1,2 contro DEF 25 → floor(288 × 100 / 125) = 230. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaExposedMultipliesTheBase,
	"RefactorTactics.Combat.Formula.ExposedMultipliesTheBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaExposedMultipliesTheBase::RunTest(const FString&)
{
	TestEqual(TEXT("il catalogo dichiara 120 (CP 53.3b)"), URTCombatLibrary::ExposedModPercent, 120);
	TestEqual(TEXT("240 Exposed contro DEF 25"),
		URTCombatLibrary::MitigateDirectDamage(240, 0, URTCombatLibrary::ExposedModPercent, 25), 230);
	// Il moltiplicatore agisce anche sul bonus alle spalle: (240 + 60) × 1,2 × 100 / 125 = 288.
	TestEqual(TEXT("240 + 60 Exposed contro DEF 25"), URTCombatLibrary::MitigateDirectDamage(240, 60, 120, 25), 288);
	// Un Mod a 100 e' neutro per costruzione.
	TestEqual(TEXT("Mod 100 e' neutro"), URTCombatLibrary::MitigateDirectDamage(240, 0, 100, 25), Mitigate(240, 25));
	return true;
}

/** D-595 (1): per difetto, una volta sola. Le celle in cui «al piu' vicino» darebbe un punto in piu'. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaRoundsDownOnce,
	"RefactorTactics.Combat.Formula.RoundsDownOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaRoundsDownOnce::RunTest(const FString&)
{
	// 120 × 100 / 135 = 88,88…: per difetto 88 (al piu' vicino darebbe 89).
	TestEqual(TEXT("Mortar 120 contro DEF 35"), Mitigate(120, 35), 88);
	// 200 × 100 / 115 = 173,91…: per difetto 173 (al piu' vicino darebbe 174).
	TestEqual(TEXT("Charge 200 contro DEF 15"), Mitigate(200, 15), 173);
	// Con Exposed e bonus insieme l'arrotondamento resta UNO solo, in fondo: (120 + 30) × 1,2 × 100 / 135 = 133,33… → 133.
	TestEqual(TEXT("un solo floor dopo il prodotto"), URTCombatLibrary::MitigateDirectDamage(120, 30, 120, 35), 133);
	return true;
}

/** Esempio 6 — la RES dell'elemento usa la stessa forma: Electrify 200 contro RES 20 → 166; la propagazione 120 → 100. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaResistanceUsesTheSameForm,
	"RefactorTactics.Combat.Formula.ResistanceUsesTheSameForm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaResistanceUsesTheSameForm::RunTest(const FString&)
{
	TestEqual(TEXT("Electrify 200 contro RES 20"), Mitigate(200, 20), 166);
	TestEqual(TEXT("propagazione 120 contro RES 20"), Mitigate(120, 20), 100);
	return true;
}

/**
 * Esempio 3 — copertura e Guardia restano FUORI dalla funzione e si sottraggono dopo, piatte: HeavyAttack 350
 * contro DEF 35 in copertura bassa (100) e in Guard (150) → 259 − 250 = 9. E' il primo caso di controllo di
 * CP 53.3 (D-595 (4)): il test fissa la COMPOSIZIONE, non il numero come buono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaFlatReductionsApplyAfterMitigation,
	"RefactorTactics.Combat.Formula.FlatReductionsApplyAfterMitigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaFlatReductionsApplyAfterMitigation::RunTest(const FString&)
{
	const int32 Mitigated = Mitigate(350, 35);
	TestEqual(TEXT("mitigato"), Mitigated, 259);
	const int32 Cover = 100, Guard = 150;
	TestEqual(TEXT("poi le riduzioni piatte"), FMath::Max(0, Mitigated - Cover - Guard), 9);
	// Esempio 5 — il max(0, …) lavora: Mortar 120 contro DEF 60 con entrambe → 75 − 250 < 0 → 0.
	TestEqual(TEXT("Mortar contro il muro fa zero"), FMath::Max(0, Mitigate(120, 60) - Cover - Guard), 0);
	// [D-602] (CP 53.3c) ha tarato i due valori con la DEF in mano: copertura 60 e Guardia 100. Lo stesso esempio 3
	// con i valori del catalogo vale 99, non 9 — ed e' la ragione della taratura.
	TestEqual(TEXT("D-602: copertura e Guardia del catalogo sono 60 e 100"),
		URTCombatLibrary::LowCoverDamageReduction * 1000 + URTCombatLibrary::GuardReductionDefault, 60 * 1000 + 100);
	TestEqual(TEXT("esempio 3 con i valori di D-602: 259 − 60 − 100 = 99"),
		FMath::Max(0, Mitigated - URTCombatLibrary::LowCoverDamageReduction - URTCombatLibrary::GuardReductionDefault), 99);
	return true;
}

/**
 * Controllo positivo — a difesa zero e senza moltiplicatori la funzione e' l'IDENTITA' sui valori del catalogo
 * di oggi (scala attuale, non ×10). E' la ragione per cui questo commit non cambia nessun golden: il resolver
 * non la chiama ancora, e quando la chiamera' con DEF 0 produrra' gli stessi numeri di adesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFormulaZeroDefenseIsIdentity,
	"RefactorTactics.Combat.Formula.ZeroDefenseIsIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFormulaZeroDefenseIsIdentity::RunTest(const FString&)
{
	// I danni dichiarati oggi in `RTCatalogLibrary.cpp` e `RTHeroCatalogLibrary.cpp` (letti il 2026-11-09).
	const int32 CatalogValuesToday[] = { 8, 10, 12, 14, 16, 18, 20, 21, 22, 24, 35 };
	for (const int32 V : CatalogValuesToday)
	{
		TestEqual(*FString::Printf(TEXT("DEF 0, Mod 100: %d resta %d"), V, V), Mitigate(V, 0), V);
	}
	TestEqual(TEXT("zero resta zero"), Mitigate(0, 25), 0);
	// Valori negativi valgono zero (dichiarato nella documentazione della funzione): una Base negativa non
	// cura, una Defense negativa non amplifica finche' una decisione non introduce l'Armor firmata del PRD.
	TestEqual(TEXT("Base negativa → 0"), URTCombatLibrary::MitigateDirectDamage(-10, 0, 100, 0), 0);
	TestEqual(TEXT("Defense negativa vale 0"), URTCombatLibrary::MitigateDirectDamage(240, 0, 100, -50), 240);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
