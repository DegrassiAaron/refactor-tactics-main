// Copyright RefactorTactics. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTBreakdownTests
{
	/** Tre unita' identiche: 100 HP, nessuno scudo se non dichiarato. */
	TArray<FRTUnitCombatState> MakeUnits()
	{
		return { FRTUnitCombatState(100, 0, 0), FRTUnitCombatState(100, 0, 0), FRTUnitCombatState(100, 0, 0) };
	}

	FRTAttack MakeAttack(int32 Target, int32 Power, int32 Attacker)
	{
		FRTAttack A(Target, Power, Attacker);
		// Il colpo entra nella catena come lo produrrebbe `ToAttacks`: uno stadio `Catalog` e basta.
		A.Breakdown.Emplace(ERTDamageStage::Catalog, FName(TEXT("intent")), ERTDamageOp::Add, Power, 0, Power);
		return A;
	}

	const FRTDamageStageEntry* Find(const FRTDamageBreakdown& B, ERTDamageStage Stage)
	{
		return B.Stages.FindByPredicate([Stage](const FRTDamageStageEntry& E) { return E.Stage == Stage; });
	}
}

/**
 * L'ULTIMO `After` DEL BREAKDOWN E' IL DANNO CHE GLI HP HANNO SUBITO — `#1951`.
 *
 * 🔑 **E' cio' che rende il breakdown una LETTURA e non un secondo calcolo.** Se l'ultimo stadio dicesse
 * un numero diverso da quello che la partita ha applicato, il registro spiegherebbe un danno che non e'
 * avvenuto — ed e' il modo in cui una diagnostica diventa peggio di nessuna diagnostica.
 *
 * ⚠️ Il valore confrontato e' il danno EFFETTIVO, che e' la definizione di `FRTTurnLogEntry::Amount`. Il
 * confronto con la voce di log vera vive in uno scenario; qui si pinna l'identita' alla fonte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownMatchesDamageDealtTest,
	"RefactorTactics.Damage.BreakdownFinalValueMatchesDamageDealt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownMatchesDamageDealtTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	// Uno scudo base da 5 sul bersaglio: 22 nominali -> 17 sugli HP, la catena gia' misurata da `D-224`.
	TArray<FRTUnitCombatState> Units = MakeUnits();
	Units[1] = FRTUnitCombatState(120, 5, 0);

	const TArray<FRTAttack> Attacks = { MakeAttack(1, 22, 0) };

	TMap<int32, FRTDamageBreakdown> ByTarget;
	const TArray<FRTUnitCombatState> After = URTCombatResolver::ResolveAttacksWithBreakdown(Units, Attacks, ByTarget);

	const int32 HpLost = Units[1].Health - After[1].Health;
	TestEqual(TEXT("lo scudo base ha fermato 5 dei 22"), HpLost, 17);

	const FRTDamageBreakdown* B = ByTarget.Find(1);
	if (!TestNotNull(TEXT("il bersaglio ha un registro"), B)) { return false; }
	if (!TestTrue(TEXT("il registro non e' vuoto"), B->Stages.Num() > 0)) { return false; }

	TestEqual(TEXT("l'ultimo After e' il danno subito davvero"), B->Stages.Last().After, HpLost);
	TestEqual(TEXT("e l'ultimo stadio e' l'assorbimento dello scudo"),
		static_cast<int32>(B->Stages.Last().Stage), static_cast<int32>(ERTDamageStage::ShieldAbsorption));

	// ⛔ La controprova che il registro non sia una costante: senza scudo l'ultimo `After` cambia.
	const TArray<FRTUnitCombatState> Bare = MakeUnits();
	TMap<int32, FRTDamageBreakdown> BareByTarget;
	URTCombatResolver::ResolveAttacksWithBreakdown(Bare, Attacks, BareByTarget);
	TestEqual(TEXT("senza scudo passano tutti e 22"), BareByTarget[1].Stages.Last().After, 22);

	return true;
}

/**
 * PERMUTARE I COLPI NON CAMBIA LA SEQUENZA DEGLI STADI — `#1951`.
 *
 * 🔴 **La versione precedente di `ResolveAttacks` iterava una `TMap`**, e per gli stati era indifferente:
 * ogni bersaglio si risolve dal proprio stato iniziale. Per un REGISTRO non lo e' — un elenco che cambia
 * ordine fra due esecuzioni non e' verificabile, ed e' un vincolo dichiarato dalla issue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownOrderIsStableTest,
	"RefactorTactics.Damage.BreakdownOrderIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownOrderIsStableTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();
	const TArray<FRTAttack> Straight = { MakeAttack(1, 10, 0), MakeAttack(2, 7, 0), MakeAttack(1, 4, 0) };
	const TArray<FRTAttack> Swapped  = { MakeAttack(2, 7, 0), MakeAttack(1, 10, 0), MakeAttack(1, 4, 0) };

	TMap<int32, FRTDamageBreakdown> A, B;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, Straight, A);
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, Swapped, B);

	const int32 Targets[] = { 1, 2 };
	for (const int32 Target : Targets)
	{
		const FRTDamageBreakdown* L = A.Find(Target);
		const FRTDamageBreakdown* R = B.Find(Target);
		if (!TestNotNull(TEXT("registro a sinistra"), L) || !TestNotNull(TEXT("registro a destra"), R))
		{
			return false;
		}
		if (!TestEqual(FString::Printf(TEXT("bersaglio %d: stesso numero di stadi"), Target),
			L->Stages.Num(), R->Stages.Num()))
		{
			continue;
		}
		for (int32 i = 0; i < L->Stages.Num(); ++i)
		{
			TestEqual(FString::Printf(TEXT("bersaglio %d, stadio %d"), Target, i),
				static_cast<int32>(L->Stages[i].Stage), static_cast<int32>(R->Stages[i].Stage));
			TestEqual(FString::Printf(TEXT("bersaglio %d, stadio %d: After"), Target, i),
				L->Stages[i].After, R->Stages[i].After);
		}
	}
	return true;
}

/**
 * UNO STADIO CHE NON SI APPLICA NON COMPARE — `#1951`.
 *
 * 🔑 **Un elenco che contiene tutto non spiega niente.** La differenza fra «la Guardia non ha assorbito» e
 * «la Guardia non c'era» si legge solo se il secondo caso lascia il registro muto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownOmitsStagesThatDidNotApplyTest,
	"RefactorTactics.Damage.BreakdownOmitsStagesThatDidNotApply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownOmitsStagesThatDidNotApplyTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();

	// Un colpo solo, nessuna mitigazione: niente somma per bersaglio, niente pool, niente delta.
	TMap<int32, FRTDamageBreakdown> ByTarget;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, { MakeAttack(1, 9, 0) }, ByTarget);

	const FRTDamageBreakdown& B = ByTarget[1];
	TestNull(TEXT("nessuna somma per bersaglio con un colpo solo"), Find(B, ERTDamageStage::TargetSum));
	TestNull(TEXT("nessun pool: il Deflect non c'era"), Find(B, ERTDamageStage::AbsorptionPool));
	// ⏱️ **Riga aggiunta da [D-408]**: la Guardia scriveva `AbsorptionPool` e scrive ora `EveryHitDelta`,
	// quindi senza questa la riga sopra avrebbe smesso di coprirla — e il docstring avrebbe continuato a
	// dire *«la Guardia non c'era»* senza nessuna assertion dietro.
	TestNull(TEXT("nessun delta per colpo: la Guardia non c'era"), Find(B, ERTDamageStage::EveryHitDelta));
	TestNull(TEXT("nessun delta di primo colpo"), Find(B, ERTDamageStage::FirstHitDelta));

	// ✅ La controprova: con DUE colpi la somma compare, quindi l'assenza sopra e' una scelta e non un buco.
	TMap<int32, FRTDamageBreakdown> Two;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, { MakeAttack(1, 9, 0), MakeAttack(1, 3, 0) }, Two);
	TestNotNull(TEXT("con due colpi la somma compare"), Find(Two[1], ERTDamageStage::TargetSum));

	return true;
}

/**
 * LA GUARDIA FRONTALE E QUELLA ALLE SPALLE RACCONTANO DUE STORIE PER LO STESSO DANNO NOMINALE — `#1951`.
 *
 * E' il caso che [D-408] + [D-206] decidono: solo l'arco frontale e' ridotto, e un colpo alle spalle passa
 * **intero**. Senza registro i due esiti si distinguono solo dagli HP.
 *
 * ⏱️ **Modellava la Guardia come un POOL fino al 2026-09-20**, e restava verde perche' chiamava
 * `ApplyAbsorptionPool` direttamente: la produzione era gia' passata a `ApplyEligibleHitDelta`, quindi la
 * copertura frontale-vs-spalle del breakdown — la proprieta' di `#1951` per cui questo file esiste — non
 * proteggeva piu' niente di cio' che gira davvero. Trovato da una code review, **nello stesso file** in cui
 * il difetto gemello era gia' stato corretto un test piu' sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownGuardTellsTwoStoriesTest,
	"RefactorTactics.Damage.BreakdownGuardFrontalAndBehindDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownGuardTellsTwoStoriesTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();
	const TArray<FRTAttack> Attacks = { MakeAttack(1, 12, 0) };
	const TArray<int32> Riduzione = { 0, -URTCombatLibrary::GuardFirstHitReduction, 0 };
	const TArray<bool> Frontally = { true };
	const TArray<bool> FromBehind = { false };

	TMap<int32, FRTDamageBreakdown> Frontal, Behind;
	URTCombatResolver::ResolveAttacksWithBreakdown(
		Units, URTCombatResolver::ApplyEligibleHitDelta(Attacks, Riduzione, Frontally, URTCombatLibrary::GuardPerHitSource), Frontal);
	URTCombatResolver::ResolveAttacksWithBreakdown(
		Units, URTCombatResolver::ApplyEligibleHitDelta(Attacks, Riduzione, FromBehind, URTCombatLibrary::GuardPerHitSource), Behind);

	const FRTDamageStageEntry* FrontGuard = Find(Frontal[1], ERTDamageStage::EveryHitDelta);
	if (TestNotNull(TEXT("frontale: la Guardia ha morso"), FrontGuard))
	{
		// ⚠️ `Operand` e' il delta DICHIARATO (15), non quanto ne e' stato applicato: il colpo valeva 12 e
		// il clamp ha fatto il resto. Sono `Before` e `After` a dire quanto ha tolto davvero, ed e' la
		// convenzione della famiglia dei delta — il pool invece registrava l'assorbito.
		TestEqual(TEXT("e dichiara i 15 della regola"), FrontGuard->Operand, URTCombatLibrary::GuardFirstHitReduction);
		TestEqual(TEXT("sul colpo da 12"), FrontGuard->Before, 12);
		TestEqual(TEXT("che esce azzerato"), FrontGuard->After, 0);
		TestEqual(TEXT("e nomina la propria decisione"), FrontGuard->SourceId, URTCombatLibrary::GuardPerHitSource);
	}
	TestEqual(TEXT("frontale: niente arriva agli HP"), Frontal[1].Stages.Last().After, 0);

	TestNull(TEXT("alle spalle: la Guardia non compare affatto"), Find(Behind[1], ERTDamageStage::EveryHitDelta));
	// ⛔ E nemmeno come pool: dopo [D-408] uno stadio `AbsorptionPool` in questa scena vorrebbe dire che
	// qualcuno ha rimesso la Guardia sul percorso del `Deflect`.
	TestNull(TEXT("alle spalle: e nessun pool, che qui non c'entra"), Find(Behind[1], ERTDamageStage::AbsorptionPool));
	TestEqual(TEXT("alle spalle: passano tutti e 12"), Behind[1].Stages.Last().After, 12);

	return true;
}

/**
 * DUE MITIGAZIONI, DUE PROVENIENZE — `#2213`, riallineato da [D-408].
 *
 * 🔴 **Questo test componeva DUE POOL, e dal 2026-09-20 la produzione non lo fa piu'.** [D-408]
 * ritira il pool della `Guard`: `RTTurnManager` compone ora `ApplyAbsorptionPool` (il `Deflect`, [D-309])
 * e poi `ApplyEligibleHitDelta` (la Guardia). Il test restava verde — chiama le funzioni direttamente — ma
 * la sua premessa dichiarata, *«compone i due pool come fa `RTTurnManager`»*, era diventata falsa,
 * e con essa l'utilita' dell'intero file: un test di provenienza che rispecchia una composizione che non
 * esiste non protegge la composizione che esiste.
 *
 * 🔴 **Il difetto che questo test e' nato per prendere**: `ApplyAbsorptionPool` scriveva l'etichetta di
 * stadio come un LETTERALE nel proprio corpo — `D-292 · Status.Guarded` — e da [D-309] i chiamanti di
 * produzione sono DUE (`RTTurnManager.cpp`: prima il pool di `Deflect`, poi quello di `Guard`, [D-312]).
 * Un assorbimento della reazione finiva quindi nel breakdown attribuito alla GUARDIA, con un numero di
 * decisione e un tag di stato che non la riguardano.
 *
 * 🔑 **Perche' nessuno se n'era accorto**: nessun test asserisce un `SourceId`, e nessun consumatore di
 * produzione legge il breakdown — `ResolveAttacksWithBreakdown` ha per chiamanti il suo wrapper e i test.
 * Il difetto era LATENTE, non invisibile: il breakdown esiste da `#1951` perche' il TurnLog dica da dove
 * viene un numero, e i suoi consumatori arrivano con `#1937`.
 *
 * ⚠️ **Cio' che questo test NON prova, ed e' dichiarato invece che taciuto.** Compone le due mitigazioni
 * come fa `RTTurnManager`, ma le chiama DIRETTAMENTE: resta quindi verde qualunque provenienza passino le
 * due chiamate reali del manager. Non e' pigrizia — `ARTTurnManager` passa da `ResolveAttacks`, il wrapper che
 * costruisce il breakdown e lo SCARTA in un `TMap` locale, quindi dal percorso di partita non esce niente
 * da osservare. E' lo stesso limite di `Combat.DeflectAbsorbsBeforeGuardReduces` — ma li'
 * `Combat.GuardAndDeflectAbsorbInDeclaredOrder` lo chiude passando dal manager, perche' l'ordine dei pool
 * si vede negli HP. Un'ETICHETTA no: finche' nessuno legge il breakdown, il lato chiamante e' protetto da
 * una code review e non da un test. ✅ Cio' che l'uso delle costanti condivise
 * (`URTCombatLibrary::GuardPoolSource`, `ReactionReductionPoolSource`) aggiunge e' che un refuso non e'
 * piu' possibile: prima il test portava una copia PROPRIA dei due letterali, quindi un errore di battitura
 * ai chiamanti di produzione sarebbe rimasto verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownPoolNamesItsOwnSourceTest,
	"RefactorTactics.Damage.BreakdownPoolNamesItsOwnSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownPoolNamesItsOwnSourceTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();
	const TArray<FRTAttack> Attacks = { MakeAttack(1, 12, 0) };
	const TArray<bool> Eligible = { true };

	// 🔑 LE DUE MITIGAZIONI COMPOSTE, non risolte separatamente: e' la forma di `RTTurnManager.cpp` —
	// `Deflect` prima, `Guard` poi ([D-312]) — e produce DUE voci nello STESSO breakdown. Risolverle in due
	// passate darebbe una voce per elenco, cioe' un `Find`-per-stadio non ambiguo per costruzione:
	// esattamente l'ambiguita' che in produzione non c'e'. *La prima stesura faceva cosi'; trovato da una
	// code review.*
	//
	// ⏱️ **Da [D-408] i due stadi sono DIVERSI**: il `Deflect` resta `AbsorptionPool`, la Guardia scrive
	// `EveryHitDelta`. Il difetto di `#2213` — due voci con lo stesso `FName` — resta pero' possibile
	// **dentro** ciascuno stadio, e la provenienza va asserita per questo.
	//
	// I valori sono PICCOLI e scelti perche' **entrambe** mordano: 5 alla reazione e 5 alla Guardia, cosi'
	// il colpo da 12 scende a 7 e poi a 2. Coi valori pieni la prima assorbirebbe tutto e la seconda non
	// lascerebbe voce; e con una riduzione piu' grande del residuo il clamp a zero renderebbe illeggibile
	// quanto ne ha tolto davvero.
	const TArray<int32> ReactionPool   = { 0, 5, 0 };
	const TArray<int32> GuardReduction = { 0, -5, 0 };

	TMap<int32, FRTDamageBreakdown> ByTarget;
	URTCombatResolver::ResolveAttacksWithBreakdown(
		Units,
		URTCombatResolver::ApplyEligibleHitDelta(
			URTCombatResolver::ApplyAbsorptionPool(Attacks, ReactionPool, Eligible,
				URTCombatLibrary::ReactionReductionPoolSource),
			GuardReduction, Eligible, URTCombatLibrary::GuardPerHitSource),
		ByTarget);

	// `Find` e non `operator[]`: una chiave assente deve far fallire QUESTO test, non abbattere la passata
	// di automation. E' il pattern che `BreakdownFinalValueMatchesDamageDealt` usa gia' in questo file.
	const FRTDamageBreakdown* B = ByTarget.Find(1);
	if (!TestNotNull(TEXT("il bersaglio ha un registro"), B)) { return false; }

	TArray<const FRTDamageStageEntry*> Mitigazioni;
	for (const FRTDamageStageEntry& E : B->Stages)
	{
		if (E.Stage == ERTDamageStage::AbsorptionPool || E.Stage == ERTDamageStage::EveryHitDelta)
		{
			Mitigazioni.Add(&E);
		}
	}

	if (!TestEqual(TEXT("due mitigazioni hanno morso, e lasciano DUE voci nello stesso registro"),
		Mitigazioni.Num(), 2))
	{
		return false;
	}

	// IL DIFETTO, in due righe: prima di `#2213` queste due voci portavano lo STESSO `FName`, e la seconda
	// era quella della Guardia — quindi l'assorbimento della reazione risultava suo.
	TestEqual(TEXT("la prima voce e' della reazione, e nomina la SUA decisione"),
		Mitigazioni[0]->SourceId, URTCombatLibrary::ReactionReductionPoolSource);
	TestEqual(TEXT("la seconda e' della Guardia, e nomina la propria"),
		Mitigazioni[1]->SourceId, URTCombatLibrary::GuardPerHitSource);

	// ⚠️ **E gli STADI sono diversi da [D-408]**, che e' la meta' nuova di questa asserzione: il `Deflect`
	// e' un pool, la Guardia no. Se un domani tornassero allo stesso stadio, il `Find`-per-stadio del resto
	// del file tornerebbe ambiguo e questa riga lo direbbe.
	TestEqual(TEXT("la reazione scrive lo stadio del POOL"),
		Mitigazioni[0]->Stage, ERTDamageStage::AbsorptionPool);
	TestEqual(TEXT("la Guardia scrive lo stadio del delta PER COLPO"),
		Mitigazioni[1]->Stage, ERTDamageStage::EveryHitDelta);

	// ⚠️ **E l'ordine e' quello di [D-312]**, leggibile qui perche' le due voci convivono: la reazione
	// assorbe per prima. Un elenco che le contenesse invertite descriverebbe un bilanciamento diverso.
	TestEqual(TEXT("la reazione ha assorbito il suo budget intero"), Mitigazioni[0]->Operand, 5);
	TestEqual(TEXT("e la Guardia ha ridotto il residuo"), Mitigazioni[1]->Operand, 5);
	TestEqual(TEXT("il colpo da 12 esce a 2"), Mitigazioni[1]->After, 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
