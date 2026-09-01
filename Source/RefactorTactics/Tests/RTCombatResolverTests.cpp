#include "Misc/AutomationTest.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackSingleTest,
	"RefactorTactics.Combat.SingleAttackAppliesDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackSingleTest::RunTest(const FString&)
{
	// U0 (100/0) colpisce U1 (100/20) con 30 -> scudo 20 assorbe, 10 agli HP: U1 = 90/0.
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 20} };
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U0 intatta"), Out[0].Health, 100);
	TestEqual(TEXT("U1 HP 90"), Out[1].Health, 90);
	TestEqual(TEXT("U1 scudo 0"), Out[1].Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackFocusFireTest,
	"RefactorTactics.Combat.FocusFireSumsDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackFocusFireTest::RunTest(const FString&)
{
	// U0 e U1 colpiscono entrambe U2 (100/20) con 30 ciascuna -> 60 danni: 20 scudo + 40 HP = 60.
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 0}, {100, 20} };
	const TArray<FRTAttack> Attacks = { FRTAttack(2, 30), FRTAttack(2, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U2 HP 60"), Out[2].Health, 60);
	TestEqual(TEXT("U2 scudo 0"), Out[2].Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackMutualTest,
	"RefactorTactics.Combat.MutualAttackUsesInitialState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackMutualTest::RunTest(const FString&)
{
	// U0 (20/0) e U1 (20/0) si colpiscono con 30: entrambe muoiono, perche' il danno
	// e' calcolato sullo stato iniziale (nessuna delle due "salta" il colpo morendo prima).
	const TArray<FRTUnitCombatState> Units = { {20, 0}, {20, 0} };
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30), FRTAttack(0, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U0 morta"), Out[0].Health, 0);
	TestEqual(TEXT("U1 morta"), Out[1].Health, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackOrderIndependentTest,
	"RefactorTactics.Combat.AttackResolutionIsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackOrderIndependentTest::RunTest(const FString&)
{
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 0}, {100, 20} };
	const TArray<FRTAttack> Forward = { FRTAttack(2, 30), FRTAttack(2, 30), FRTAttack(0, 50) };
	TArray<FRTAttack> Backward = Forward;
	Algo::Reverse(Backward);

	const TArray<FRTUnitCombatState> A = URTCombatResolver::ResolveAttacks(Units, Forward);
	const TArray<FRTUnitCombatState> B = URTCombatResolver::ResolveAttacks(Units, Backward);

	bool bSame = (A.Num() == B.Num());
	for (int32 i = 0; i < A.Num() && bSame; ++i)
	{
		bSame = (A[i].Health == B[i].Health && A[i].Shield == B[i].Shield);
	}
	TestTrue(TEXT("stesso esito invertendo l'ordine degli attacchi"), bSame);
	TestEqual(TEXT("U0 HP 50"), A[0].Health, 50);
	TestEqual(TEXT("U2 HP 60"), A[2].Health, 60);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTExposedFirstHitTest,
	"RefactorTactics.Status.Exposed.FirstDirectHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTExposedFirstHitTest::RunTest(const FString&)
{
	// U1 e' Exposed (ha scattato allo scoperto), U2 no. Tre colpi in arrivo: due su U1, uno su U2.
	TArray<int32> Delta;
	Delta.Init(0, 3);
	Delta[1] = URTCombatLibrary::ExposedFirstHitBonus;

	const TArray<FRTAttack> Attacks = { FRTAttack(1, 20), FRTAttack(1, 10), FRTAttack(2, 20) };
	const TArray<FRTAttack> Boosted = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);

	if (!TestEqual(TEXT("nessun colpo si perde per strada"), Boosted.Num(), 3)) { return false; }
	TestEqual(TEXT("il primo colpo su chi e' esposto prende +5"), Boosted[0].Power, 25);
	TestEqual(TEXT("il secondo colpo sullo stesso bersaglio NON lo prende: vale una volta sola"), Boosted[1].Power, 10);
	TestEqual(TEXT("chi non e' esposto incassa il danno nominale"), Boosted[2].Power, 20);

	// Ordine-indipendenza (invariante #3): il TOTALE per bersaglio non cambia se i colpi arrivano in un altro
	// ordine — cambia solo a quale colpo e' attribuito il bonus, e i danni si sommano.
	TArray<FRTAttack> Reversed = Attacks;
	Algo::Reverse(Reversed);
	const TArray<FRTAttack> BoostedRev = URTCombatResolver::ApplyFirstHitDelta(Reversed, Delta);
	int32 SumDirect = 0, SumReversed = 0;
	for (const FRTAttack& A : Boosted)    { if (A.TargetIndex == 1) { SumDirect += A.Power; } }
	for (const FRTAttack& A : BoostedRev) { if (A.TargetIndex == 1) { SumReversed += A.Power; } }
	TestEqual(TEXT("danno totale sull'esposto: 20 + 10 + 5"), SumDirect, 35);
	TestEqual(TEXT("invertendo l'ordine dei colpi il totale e' identico"), SumReversed, SumDirect);

	// Nessuno esposto: gli attacchi restano quelli dichiarati (nessun ritocco silenzioso).
	TArray<int32> NoDelta;
	NoDelta.Init(0, 3);
	const TArray<FRTAttack> Untouched = URTCombatResolver::ApplyFirstHitDelta(Attacks, NoDelta);
	TestEqual(TEXT("senza stato il primo colpo non cambia"), Untouched[0].Power, 20);

	// Un delta negativo (la Guardia di CP 4.4) puo' annullare un colpo, non curare il bersaglio.
	TArray<int32> Guarded;
	Guarded.Init(0, 3);
	Guarded[1] = -50;
	const TArray<FRTAttack> Blocked = URTCombatResolver::ApplyFirstHitDelta(Attacks, Guarded);
	TestEqual(TEXT("un delta piu' grande del colpo lo azzera, non lo inverte"), Blocked[0].Power, 0);

	// Bersaglio fuori dall'array dei delta: nessun crash e nessuna modifica.
	TArray<int32> Short;
	Short.Init(0, 1);
	const TArray<FRTAttack> Safe = URTCombatResolver::ApplyFirstHitDelta(Attacks, Short);
	TestEqual(TEXT("indice fuori dai delta: colpo invariato"), Safe[0].Power, 20);
	return true;
}

/**
 * L'invariante che ha motivato [D-292], misurata sul percorso che la Guardia usa DAVVERO.
 *
 * Storia, perche' conta: questo test e' nato **rosso** contro `ApplyFirstHitDelta`, dove la Guardia passava
 * prima di [D-292]. Un bersaglio in Guardia colpito da 10 e da 30 incassava **30** o **25** a seconda di
 * quale colpo fosse primo nell'array — la riduzione che avanzava si perdeva nel clamp `Max(0, ...)`, e a
 * scegliere era l'indice dell'attaccante. Il difetto era coperto da `Status.Exposed.FirstDirectHitOnly`, che
 * afferma la stessa invariante ma la prova con un delta **positivo** (`+5`), dove il clamp non morde mai.
 *
 * Col pool la proprieta' vale **per costruzione**: cio' che un colpo non consuma resta, quindi il totale
 * assorbito e' lo stesso in ogni ordine. Il test resta perche' e' l'invariante, non la cronaca: se qualcuno
 * riportasse la Guardia su un delta di primo colpo, tornerebbe rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardPoolPermutationTest,
	"RefactorTactics.Combat.GuardPoolIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardPoolPermutationTest::RunTest(const FString&)
{
	TArray<int32> Pool;
	Pool.Init(0, 2);
	Pool[1] = URTCombatLibrary::GuardFirstHitReduction;   // 15

	// Due colpi frontali di taglia diversa: il caso ordinario di due attaccanti sullo stesso bersaglio.
	const TArray<FRTAttack> Small = { FRTAttack(1, 10, 0), FRTAttack(1, 30, 2) };
	TArray<FRTAttack> Large = Small;
	Algo::Reverse(Large);
	const TArray<bool> Eligible = { true, true };

	auto SumOn = [](const TArray<FRTAttack>& In, int32 Target)
	{
		int32 Sum = 0;
		for (const FRTAttack& A : In) { if (A.TargetIndex == Target) { Sum += A.Power; } }
		return Sum;
	};

	const int32 SmallFirst = SumOn(URTCombatResolver::ApplyAbsorptionPool(Small, Pool, Eligible), 1);
	const int32 LargeFirst = SumOn(URTCombatResolver::ApplyAbsorptionPool(Large, Pool, Eligible), 1);

	// ANTI-VACUITA': un pool che non assorbisse niente sarebbe invariante e inutile. 40 e' il danno nominale.
	TestNotEqual(TEXT("la Guardia toglie qualcosa: il totale non e' quello nominale"), SmallFirst, 40);

	TestEqual(TEXT("il totale sul bersaglio in Guardia non dipende da quale colpo arriva per primo"),
		SmallFirst, LargeFirst);
	TestEqual(TEXT("e vale 40 - 15, senza avanzi persi"), SmallFirst, 25);

	// ✅ CIO' CHE IL POOL HA SISTEMATO, ed era il canary di #1918.
	//
	// Fino a [D-309] qui c'era un `TestNotEqual` DELIBERATO — «Deflect e' ancora sensibile all'ordine: e'
	// debito, non una proprieta'» — messo perche' diventasse rosso il giorno in cui qualcuno avesse risolto
	// il problema. E' quel giorno. Il caso resta misurato, con l'esito nuovo: `Deflect` passa da
	// `ApplyAbsorptionPool`, l'avanzo non si perde piu' nel clamp, e il totale non dipende dall'ordine.
	TArray<int32> DeflectPool;
	DeflectPool.Init(0, 2);
	DeflectPool[1] = URTCombatLibrary::DeflectDamageReduction;   // 20 — POSITIVO: e' un budget, non un delta

	const int32 DeflectSmallFirst = SumOn(URTCombatResolver::ApplyAbsorptionPool(Small, DeflectPool, Eligible), 1);
	const int32 DeflectLargeFirst = SumOn(URTCombatResolver::ApplyAbsorptionPool(Large, DeflectPool, Eligible), 1);

	// ANTI-VACUITA', come sopra per la Guardia: un pool che non assorbisse niente sarebbe invariante e
	// inutile, e l'uguaglianza sotto passerebbe senza dire nulla.
	TestNotEqual(TEXT("il Deflect toglie qualcosa: il totale non e' quello nominale"), DeflectSmallFirst, 40);

	TestEqual(TEXT("il totale sul bersaglio con Deflect non dipende da quale colpo arriva per primo"),
		DeflectSmallFirst, DeflectLargeFirst);
	TestEqual(TEXT("e vale 40 - 20, senza avanzi persi nel clamp"), DeflectSmallFirst, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardPoolNotConsumedFromBehindTest,
	"RefactorTactics.Combat.GuardPoolIsNotConsumedFromBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardPoolNotConsumedFromBehindTest::RunTest(const FString&)
{
	TArray<int32> Pool;
	Pool.Init(0, 2);
	Pool[1] = URTCombatLibrary::GuardFirstHitReduction;   // 15

	// Colpo 0 dalle SPALLE (30), colpo 1 dal DAVANTI (10).
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30, 0), FRTAttack(1, 10, 2) };
	const TArray<bool> Eligible = { false, true };

	const TArray<FRTAttack> Out = URTCombatResolver::ApplyAbsorptionPool(Attacks, Pool, Eligible);

	TestEqual(TEXT("il colpo alle spalle passa intero"), Out[0].Power, 30);
	TestEqual(TEXT("il colpo frontale e' assorbito, e il budget bastava"), Out[1].Power, 0);

	// ANTI-VACUITA': se la maschera fosse ignorata, il colpo da 30 avrebbe consumato tutto il pool e il
	// frontale sarebbe passato intero. I due asserti sopra separano i due casi solo se questo vale.
	const TArray<bool> AllEligible = { true, true };
	const TArray<FRTAttack> Ignored = URTCombatResolver::ApplyAbsorptionPool(Attacks, Pool, AllEligible);
	TestEqual(TEXT("senza maschera sarebbe il colpo alle spalle a mangiare il budget"), Ignored[0].Power, 15);
	TestEqual(TEXT("...e il frontale passerebbe intero"), Ignored[1].Power, 10);
	return true;
}

/**
 * L'ORDINE dei due pool, che [D-312] ha dovuto decidere perche' NON era inerte.
 *
 * 🔑 `Guard` e' eleggibile SOLO sui colpi frontali ([D-206]), `Deflect` su tutti i colpi diretti — non
 * ha clausola d'arco. Le due maschere sono quindi PARZIALMENTE sovrapposte, ed e' esattamente la condizione
 * in cui l'ordine d'assorbimento cambia l'esito: su 2940 configurazioni raggiungibili con i numeri reali,
 * 558 danno un totale diverso a seconda di quale pool consuma per primo.
 *
 * ⚠️ Il caso minimo NON e' quello che viene in mente per primo. Con un colpo grosso alle spalle e uno
 * piccolo davanti l'ordine risulta INERTE (il pool largo si esaurisce comunque sul colpo grosso), e misurare
 * solo quel caso avrebbe fatto dichiarare la domanda chiusa. Serve il contrario: piccolo davanti, grosso
 * dietro.
 *
 * Questo test pinna la decisione. Se qualcuno invertisse le due chiamate in `RTTurnManager.cpp` diventerebbe
 * rosso, ed e' cio' per cui esiste: quell'ordine e' bilanciamento, non stile.
 *
 * 🔴 **CIO' CHE QUESTO TEST NON PROVA, ed e' stato misurato invece che dedotto.** Chiama
 * `ApplyAbsorptionPool` DIRETTAMENTE, quindi resta verde qualunque ordine usi `RTTurnManager`.
 * Alla scrittura, invertendo le due chiamate nella catena reale **100 test su 100 restavano verdi**:
 * nessuno proteggeva l'ordine nel chiamante.
 *
 * ✅ Quel buco e' chiuso da `Combat.GuardAndDeflectAbsorbInDeclaredOrder`
 * (`RTFacingDefenseTests.cpp`), che passa da `ARTTurnManager` con Guardia e reazione attive insieme e
 * cade se l'ordine viene invertito. Questo test resta perche' dice una cosa che quello non dice: **che i
 * due ordini divergono affatto**, cioe' la premessa senza la quale [D-312] sarebbe una decisione su niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectAbsorbsBeforeGuardTest,
	"RefactorTactics.Combat.DeflectPoolAbsorbsBeforeGuardPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectAbsorbsBeforeGuardTest::RunTest(const FString&)
{
	// Colpo 0: 5 danni, FRONTALE.  Colpo 1: 20 danni, DALLE SPALLE.
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 5, 0), FRTAttack(1, 20, 2) };

	TArray<int32> GuardPool;
	GuardPool.Init(0, 2);
	GuardPool[1] = URTCombatLibrary::GuardFirstHitReduction;     // 15
	const TArray<bool> bFrontal = { true, false };               // solo il colpo 0 e' frontale

	TArray<int32> DeflectPool;
	DeflectPool.Init(0, 2);
	DeflectPool[1] = URTCombatLibrary::DeflectDamageReduction;   // 20
	const TArray<bool> bDirect = { true, true };                 // nessun filtro d'arco

	auto SumOn = [](const TArray<FRTAttack>& In, int32 Target)
	{
		int32 Sum = 0;
		for (const FRTAttack& A : In) { if (A.TargetIndex == Target) { Sum += A.Power; } }
		return Sum;
	};

	// L'ordine canonico: Deflect assorbe, poi Guard copre cio' che resta.
	const int32 Canonico = SumOn(
		URTCombatResolver::ApplyAbsorptionPool(
			URTCombatResolver::ApplyAbsorptionPool(Attacks, DeflectPool, bDirect),
			GuardPool, bFrontal), 1);

	// L'ordine invertito, calcolato QUI e non altrove: e' l'anti-vacuita' di questo test.
	const int32 Invertito = SumOn(
		URTCombatResolver::ApplyAbsorptionPool(
			URTCombatResolver::ApplyAbsorptionPool(Attacks, GuardPool, bFrontal),
			DeflectPool, bDirect), 1);

	// ANTI-VACUITA': se i due ordini coincidessero, l'asserzione sotto passerebbe per una ragione che non e'
	// quella dichiarata — e [D-312] sarebbe stata una decisione su niente.
	TestNotEqual(TEXT("i due ordini NON danno lo stesso esito: e' la premessa di D-312"),
		Canonico, Invertito);

	TestEqual(TEXT("ordine canonico (Deflect prima): il bersaglio riceve 5"), Canonico, 5);
	TestEqual(TEXT("ordine invertito (Guard prima): riceverebbe 0"), Invertito, 0);

	return true;
}

/**
 * Cio' che un colpo non consuma RESTA. E' la proprieta' che distingue il pool da `ApplyFirstHitDelta`, dove
 * l'avanzo spariva nel clamp — ed e' la ragione per cui la somma torna commutativa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardPoolRemainderTest,
	"RefactorTactics.Combat.GuardPoolRemainderIsNotWasted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardPoolRemainderTest::RunTest(const FString&)
{
	TArray<int32> Pool;
	Pool.Init(0, 2);
	Pool[1] = URTCombatLibrary::GuardFirstHitReduction;   // 15

	// Tre colpi frontali da 5: il vecchio meccanismo ne azzerava UNO e lasciava passare 10.
	const TArray<FRTAttack> Small = { FRTAttack(1, 5, 0), FRTAttack(1, 5, 2), FRTAttack(1, 5, 3) };
	const TArray<bool> Eligible = { true, true, true };
	const TArray<FRTAttack> Out = URTCombatResolver::ApplyAbsorptionPool(Small, Pool, Eligible);

	int32 Sum = 0;
	for (const FRTAttack& A : Out) { Sum += A.Power; }
	TestEqual(TEXT("il budget copre tutti e tre i colpi: nessun avanzo sprecato"), Sum, 0);

	// E un budget piu' grande del danno non cura: il colpo si ferma a zero, non diventa negativo.
	TArray<int32> Huge;
	Huge.Init(0, 2);
	Huge[1] = 100;
	const TArray<FRTAttack> Clamped = URTCombatResolver::ApplyAbsorptionPool(Small, Huge, Eligible);
	for (const FRTAttack& A : Clamped)
	{
		TestEqual(TEXT("un pool piu' grande del colpo lo azzera, non lo inverte"), A.Power, 0);
	}
	return true;
}

/**
 * NON-REGRESSIONE del caso comune. Contro UN colpo solo il pool dev'essere indistinguibile dal vecchio
 * «-15 al primo colpo»: se cambiasse anche li', [D-292] sarebbe un cambio di bilanciamento molto piu' largo
 * di quello dichiarato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardSingleHitUnchangedTest,
	"RefactorTactics.Combat.SingleHitAgainstGuardIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardSingleHitUnchangedTest::RunTest(const FString&)
{
	const int32 Guard = URTCombatLibrary::GuardFirstHitReduction;
	const TArray<bool> Eligible = { true };

	// Tre taglie: sopra il budget, uguale, sotto.
	for (const int32 Power : { 30, Guard, 10 })
	{
		TArray<int32> Pool;      Pool.Init(0, 2);      Pool[1] = Guard;
		TArray<int32> Delta;     Delta.Init(0, 2);     Delta[1] = -Guard;

		const TArray<FRTAttack> One = { FRTAttack(1, Power, 0) };
		const int32 WithPool  = URTCombatResolver::ApplyAbsorptionPool(One, Pool, Eligible)[0].Power;
		const int32 WithDelta = URTCombatResolver::ApplyFirstHitDelta(One, Delta)[0].Power;

		TestEqual(*FString::Printf(TEXT("un colpo solo da %d: pool e delta danno lo stesso esito"), Power),
			WithPool, WithDelta);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
