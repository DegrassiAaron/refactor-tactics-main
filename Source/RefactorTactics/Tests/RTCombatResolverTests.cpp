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
 * **L'invariante che ha motivato [D-292], ri-asserita sul modello di [D-408]** — e il nome non dice piu'
 * «pool».
 *
 * Storia, perche' conta. Questo test nacque **rosso** contro `ApplyFirstHitDelta`, dove la Guardia passava
 * prima di `D-292`: un bersaglio colpito da 10 e da 30 incassava **30** o **25** a seconda di quale colpo
 * fosse primo nell'array — la riduzione che avanzava si perdeva nel clamp `Max(0, ...)`, e a scegliere era
 * l'indice dell'attaccante. Il pool la rese vera per costruzione.
 *
 * 🔑 **[D-408] ritira il pool e la proprieta' REGGE LO STESSO, per una ragione diversa**: senza un budget
 * non c'e' un avanzo da perdere. Ogni colpo eleggibile riceve la stessa riduzione, quindi il totale non
 * dipende dall'ordine. ⛔ Il test si riscrive, non si cancella: e' l'invariante, non la cronaca del
 * meccanismo che un tempo la garantiva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardReductionPermutationTest,
	"RefactorTactics.Combat.GuardReductionIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardReductionPermutationTest::RunTest(const FString&)
{
	TArray<int32> Riduzione;
	Riduzione.Init(0, 2);
	Riduzione[1] = -URTCombatLibrary::GuardFirstHitReduction;   // -15, il default di catalogo

	// Due colpi frontali di taglia diversa: il caso ordinario di due attaccanti sullo stesso bersaglio.
	const TArray<FRTAttack> Piccolo = { FRTAttack(1, 10, 0), FRTAttack(1, 30, 2) };
	TArray<FRTAttack> Grande = Piccolo;
	Algo::Reverse(Grande);
	const TArray<bool> Eleggibili = { true, true };

	auto SommaSu = [](const TArray<FRTAttack>& In, int32 Target)
	{
		int32 Somma = 0;
		for (const FRTAttack& A : In) { if (A.TargetIndex == Target) { Somma += A.Power; } }
		return Somma;
	};

	const int32 PiccoloPrima = SommaSu(URTCombatResolver::ApplyEligibleHitDelta(
		Piccolo, Riduzione, Eleggibili, URTCombatLibrary::GuardPerHitSource), 1);
	const int32 GrandePrima = SommaSu(URTCombatResolver::ApplyEligibleHitDelta(
		Grande, Riduzione, Eleggibili, URTCombatLibrary::GuardPerHitSource), 1);

	// ANTI-VACUITA': una riduzione che non riducesse niente sarebbe invariante e inutile. 40 e' il nominale.
	TestNotEqual(TEXT("la Guardia toglie qualcosa: il totale non e' quello nominale"), PiccoloPrima, 40);

	TestEqual(TEXT("il totale non dipende da quale colpo arriva per primo"), PiccoloPrima, GrandePrima);

	// 🔴 **E il NUMERO e' quello nuovo, non quello del pool.** Col pool il totale era `40 - 15 = 25`: un
	// tetto di 15 per l'intero turno. Con la riduzione per colpo il primo colpo si azzera (10 - 15 -> 0) e
	// il secondo scende a 15, per un totale di **15**. E' il cambio di tetto che [D-408] dichiara VOLUTO.
	TestEqual(TEXT("ogni colpo frontale e' ridotto: 0 + 15, non 40 - 15"), PiccoloPrima, 15);
	return true;
}

/**
 * **L'invariante del `Deflect`, che [D-408] NON tocca — e che la riscrittura della Guardia aveva portato
 * via con se'** ([D-309] + [D-312]).
 *
 * 🔴 **Questo test esiste perche' cancellarne un altro aveva cancellato anche lui.** Fino al 2026-09-20 le
 * due meta' stavano nello stesso corpo: `Combat.GuardPoolIsPermutationInvariant` asseriva la commutativita'
 * del pool della Guardia **e**, sotto, quella del `Deflect` — con un commento che la chiamava *«il canary di
 * #1918»*, perche' fino a [D-309] li' c'era un `TestNotEqual` deliberato, messo per diventare rosso il
 * giorno in cui qualcuno avesse risolto il problema. Riscrivendo la meta' della Guardia sul modello nuovo,
 * la meta' del `Deflect` e' uscita insieme al nome. La suite e' rimasta verde: una copertura che sparisce
 * non fallisce.
 *
 * ⛔ **Ed era una copertura DICHIARATA, non incidentale.** Tre fonti vive la nominano per nome:
 * `roadmap-pia.md` (riga *«Deflect: pool commutativa, con anti-mutazione»*), `v0.1-definition-of-done.md`
 * `G4` — dove e' **uno dei tre percorsi di determinismo** che `PIA-5.1` pretende — e
 * `Spec.Reaction.DeflectionReducesByTwenty`, che dichiara il `TestEqual(..., 20)` qui sotto come **l'unico
 * posto** in cui il numero del `Deflect` e' pinnato su un letterale a livello di resolver.
 *
 * 🔑 **[D-408] prescriveva esattamente questo**: *«non si cancella: si riscrive sul modello nuovo — e'
 * l'invariante, non il pool, la cosa da conservare»*. Per la Guardia l'invariante si riscrive
 * (`Combat.GuardReductionIsPermutationInvariant`); per il `Deflect` non c'e' niente da riscrivere, perche'
 * il modello non e' cambiato: si conserva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectPoolPermutationTest,
	"RefactorTactics.Combat.DeflectPoolIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectPoolPermutationTest::RunTest(const FString&)
{
	TArray<int32> Pool;
	Pool.Init(0, 2);
	Pool[1] = URTCombatLibrary::DeflectDamageReduction;   // 20 — POSITIVO: e' un budget, non un delta

	// Gli stessi due colpi di taglia diversa del test della Guardia: e' il caso in cui `ApplyFirstHitDelta`
	// perdeva l'avanzo nel clamp, e quanto ne perdesse dipendeva da quale colpo fosse primo.
	const TArray<FRTAttack> Piccolo = { FRTAttack(1, 10, 0), FRTAttack(1, 30, 2) };
	TArray<FRTAttack> Grande = Piccolo;
	Algo::Reverse(Grande);
	const TArray<bool> Eleggibili = { true, true };

	auto SommaSu = [](const TArray<FRTAttack>& In, int32 Target)
	{
		int32 Somma = 0;
		for (const FRTAttack& A : In) { if (A.TargetIndex == Target) { Somma += A.Power; } }
		return Somma;
	};

	const int32 PiccoloPrima = SommaSu(URTCombatResolver::ApplyAbsorptionPool(
		Piccolo, Pool, Eleggibili, URTCombatLibrary::ReactionReductionPoolSource), 1);
	const int32 GrandePrima = SommaSu(URTCombatResolver::ApplyAbsorptionPool(
		Grande, Pool, Eleggibili, URTCombatLibrary::ReactionReductionPoolSource), 1);

	// ANTI-VACUITA': un pool che non assorbisse niente sarebbe invariante e inutile. 40 e' il nominale.
	TestNotEqual(TEXT("il Deflect toglie qualcosa: il totale non e' quello nominale"), PiccoloPrima, 40);

	TestEqual(TEXT("il totale con Deflect non dipende da quale colpo arriva per primo"),
		PiccoloPrima, GrandePrima);

	// ⚠️ Il LETTERALE, e non `40 - DeflectDamageReduction`: e' il pin che
	// `Spec.Reaction.DeflectionReducesByTwenty` dichiara come l'unico a livello di resolver. Derivarlo dalla
	// costante lo renderebbe verde anche cambiandola, che e' il difetto che quel file racconta.
	TestEqual(TEXT("e vale 40 - 20, senza avanzi persi nel clamp"), PiccoloPrima, 20);
	return true;
}

/**
 * **La riduzione per colpo NON ha un tetto, e sostituisce `GuardPoolRemainderIsNotWasted`** ([D-408]).
 *
 * ⛔ **Il test vecchio non e' applicabile e non e' stato «riscritto»: la sua domanda non esiste piu'.**
 * *«L'avanzo del pool non si spreca»* presuppone un avanzo, cioe' un budget. Senza budget non c'e' niente
 * da sprecare — e la proprieta' interessante e' quella opposta, che il pool limitava e la riduzione no.
 *
 * 🔑 **E' la conseguenza che [D-408] dichiara deliberata**, non un effetto collaterale: la sorgente §13.8
 * riconosce che una riduzione per colpo costa di piu' contro sequenze di colpi piccoli, e la vuole. Il
 * prezzo va reso leggibile e provato, non normalizzato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardReductionHasNoCeilingTest,
	"RefactorTactics.Combat.GuardReductionHasNoCeiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardReductionHasNoCeilingTest::RunTest(const FString&)
{
	const int32 Valore = URTCombatLibrary::GuardFirstHitReduction;   // 15
	TArray<int32> Riduzione;  Riduzione.Init(0, 2);  Riduzione[1] = -Valore;
	TArray<int32> Budget;     Budget.Init(0, 2);     Budget[1] = Valore;

	// Quattro colpi piccoli: il caso in cui i due modelli divergono di piu'.
	const TArray<FRTAttack> Colpi = {
		FRTAttack(1, 8, 0), FRTAttack(1, 8, 2), FRTAttack(1, 8, 3), FRTAttack(1, 8, 4) };
	const TArray<bool> Eleggibili = { true, true, true, true };

	auto Totale = [](const TArray<FRTAttack>& In)
	{
		int32 Somma = 0;
		for (const FRTAttack& A : In) { Somma += A.Power; }
		return Somma;
	};

	const int32 ConRiduzione = Totale(URTCombatResolver::ApplyEligibleHitDelta(
		Colpi, Riduzione, Eleggibili, URTCombatLibrary::GuardPerHitSource));
	const int32 ConPool = Totale(URTCombatResolver::ApplyAbsorptionPool(
		Colpi, Budget, Eleggibili, URTCombatLibrary::GuardPoolSource));

	// Quattro colpi da 8: nominale 32. Il pool ne assorbe 15 in tutto -> 17. La riduzione ne toglie 8 per
	// colpo (ciascuno si azzera, non cura) -> 0.
	TestEqual(TEXT("il pool aveva un tetto: 32 nominali meno 15 di budget"), ConPool, 17);
	TestEqual(TEXT("la riduzione per colpo non ne ha: ogni colpo si azzera"), ConRiduzione, 0);

	// ANTI-VACUITA': i due modelli DEVONO divergere qui, altrimenti il test non misura il cambio di tetto.
	TestNotEqual(TEXT("i due modelli divergono su colpi piccoli"), ConRiduzione, ConPool);
	return true;
}

/**
 * **L'arco frontale: un colpo alle spalle non e' ridotto** ([D-206] + [D-408]).
 *
 * ⏱️ *Era `GuardPoolIsNotConsumedFromBehind`: con un budget la domanda era «lo consuma?», senza budget
 * diventa «lo riduce?». La geometria non cambia — `D-408` non tocca [D-206] — cambia cosa si osserva.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardReductionSkipsBehindTest,
	"RefactorTactics.Combat.GuardReductionSkipsHitsFromBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardReductionSkipsBehindTest::RunTest(const FString&)
{
	TArray<int32> Riduzione;
	Riduzione.Init(0, 2);
	Riduzione[1] = -URTCombatLibrary::GuardFirstHitReduction;   // -15

	// Colpo 0 dalle SPALLE (30), colpo 1 dal DAVANTI (20).
	const TArray<FRTAttack> Colpi = { FRTAttack(1, 30, 0), FRTAttack(1, 20, 2) };
	const TArray<bool> Eleggibili = { false, true };

	const TArray<FRTAttack> Out = URTCombatResolver::ApplyEligibleHitDelta(
		Colpi, Riduzione, Eleggibili, URTCombatLibrary::GuardPerHitSource);

	TestEqual(TEXT("il colpo alle spalle passa intero"), Out[0].Power, 30);
	TestEqual(TEXT("il colpo frontale e' ridotto"), Out[1].Power, 5);

	// ANTI-VACUITA': se la maschera fosse ignorata, ANCHE il colpo alle spalle scenderebbe. I due asserti
	// sopra distinguono i due casi solo se questo vale.
	const TArray<bool> TuttiEleggibili = { true, true };
	const TArray<FRTAttack> Ignorata = URTCombatResolver::ApplyEligibleHitDelta(
		Colpi, Riduzione, TuttiEleggibili, URTCombatLibrary::GuardPerHitSource);
	TestEqual(TEXT("ignorando la maschera anche il colpo alle spalle scenderebbe"), Ignorata[0].Power, 15);
	return true;
}

/**
 * **Un colpo solo grande quanto la riduzione o piu': i due modelli coincidono** ([D-408]).
 *
 * 🔑 **E' la ragione per cui il corpus non si accorse di [D-292]** (`#1919`), e resta vera al contrario:
 * migrando da pool a riduzione per colpo, **qualunque** scena a colpo singolo produce lo stesso numero.
 * Non e' una coincidenza aritmetica, e' un'identita': su un colpo solo il pool da
 * `Power - min(Budget, Power)` e la riduzione da `max(0, Power - Budget)`, che sono **la stessa
 * funzione**. Cio' che distingue i due modelli sono i colpi MULTIPLI, e solo quelli.
 *
 * ⛔ **La prima stesura tolse il caso piccolo (`10`) dicendo che li' i modelli divergono. Era FALSO**:
 * con un colpo da 10 contro 15, il pool assorbe `min(15,10) = 10` e la riduzione fa `max(0, 10-15)` —
 * entrambi 0. Una copertura valida cancellata con una ragione sbagliata, e proprio quella che regge
 * l'analisi d'impatto sul corpus. Trovato da una code review; il caso e' tornato, ed e' ora il piu'
 * importante dei tre.
 *
 * ⚠️ Serve proprio per delimitare `GuardReductionHasNoCeiling`: senza, «i due modelli divergono» si
 * leggerebbe come «divergono sempre», mentre divergono **solo** su piu' colpi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardSingleHitUnchangedTest,
	"RefactorTactics.Combat.SingleHitAgainstGuardIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardSingleHitUnchangedTest::RunTest(const FString&)
{
	const int32 Valore = URTCombatLibrary::GuardFirstHitReduction;
	const TArray<bool> Eleggibile = { true };

	// Tre taglie: **sopra** il valore, **uguale**, e **sotto**. La terza e' quella che conta — e' il caso
	// in cui l'avanzo del pool esisterebbe, e su UN colpo solo non c'e' comunque niente da riportare.
	for (const int32 Potenza : { 30, Valore, 10 })
	{
		TArray<int32> Budget;    Budget.Init(0, 2);     Budget[1] = Valore;
		TArray<int32> Riduzione; Riduzione.Init(0, 2);  Riduzione[1] = -Valore;

		const TArray<FRTAttack> Uno = { FRTAttack(1, Potenza, 0) };
		const int32 ConPool = URTCombatResolver::ApplyAbsorptionPool(
			Uno, Budget, Eleggibile, URTCombatLibrary::GuardPoolSource)[0].Power;
		const int32 ConRiduzione = URTCombatResolver::ApplyEligibleHitDelta(
			Uno, Riduzione, Eleggibile, URTCombatLibrary::GuardPerHitSource)[0].Power;

		TestEqual(*FString::Printf(TEXT("un colpo solo da %d: i due modelli coincidono"), Potenza),
			ConPool, ConRiduzione);
	}

	// ANTI-VACUITA': se la Guardia non togliesse niente, i due modelli coinciderebbero banalmente e le
	// tre righe sopra non direbbero nulla. 30 e' la taglia piu' grande del ciclo.
	{
		TArray<int32> Riduzione; Riduzione.Init(0, 2); Riduzione[1] = -Valore;
		const TArray<FRTAttack> Grosso = { FRTAttack(1, 30, 0) };
		TestEqual(TEXT("e la Guardia toglie davvero qualcosa: 30 - 15"),
			URTCombatResolver::ApplyEligibleHitDelta(
				Grosso, Riduzione, Eleggibile, URTCombatLibrary::GuardPerHitSource)[0].Power, 15);
	}
	return true;
}

/**
 * **`Deflect` RESTA un pool, e la divergenza e' voluta** ([D-309], che [D-408] **non** ritira).
 *
 * 🔴 **Senza questo test la divergenza sarebbe invisibile, e il prossimo passaggio la «normalizzerebbe»
 * credendo di riparare.** Il repository ha ora **due** modelli difensivi, ed e' una conseguenza dichiarata:
 * `Deflect` e' una **reazione** — una risposta puntuale a un colpo, che un tetto limita sensatamente — e la
 * Guardia una **stance**, una postura di turno il cui prezzo e' gia' lo slot speso.
 *
 * ⚠️ Rendere `Deflect` una riduzione per colpo sarebbe una decisione, non una pulizia: `D-309` va superata,
 * non dedotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectStaysAPoolTest,
	"RefactorTactics.Combat.DeflectStaysAPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectStaysAPoolTest::RunTest(const FString&)
{
	const int32 Budget = URTCombatLibrary::DeflectDamageReduction;   // 20, POSITIVO: e' un budget
	TArray<int32> Pool;  Pool.Init(0, 2);  Pool[1] = Budget;
	const TArray<bool> Eleggibili = { true, true, true };

	// Tre colpi da 9: nominale 27. Un pool da 20 ne lascia passare 7.
	const TArray<FRTAttack> Colpi = { FRTAttack(1, 9, 0), FRTAttack(1, 9, 2), FRTAttack(1, 9, 3) };
	const TArray<FRTAttack> Out = URTCombatResolver::ApplyAbsorptionPool(
		Colpi, Pool, Eleggibili, URTCombatLibrary::ReactionReductionPoolSource);

	int32 Totale = 0;
	for (const FRTAttack& A : Out) { Totale += A.Power; }

	TestEqual(TEXT("il Deflect ha ancora un TETTO: 27 nominali meno 20 di budget"), Totale, 7);

	// 🔴 E la Guardia, sugli STESSI colpi, non ce l'ha: e' la divergenza, misurata invece che dichiarata.
	TArray<int32> Riduzione;  Riduzione.Init(0, 2);  Riduzione[1] = -9;
	const TArray<FRTAttack> Guardata = URTCombatResolver::ApplyEligibleHitDelta(
		Colpi, Riduzione, Eleggibili, URTCombatLibrary::GuardPerHitSource);
	int32 TotaleGuardia = 0;
	for (const FRTAttack& A : Guardata) { TotaleGuardia += A.Power; }
	TestEqual(TEXT("una riduzione da 9 per colpo azzera tutti e tre"), TotaleGuardia, 0);
	TestNotEqual(TEXT("i due modelli difensivi danno numeri diversi, ed e' voluto"), Totale, TotaleGuardia);
	return true;
}

/**
 * **L'ordine: il `Deflect` assorbe PRIMA che la Guardia riduca** ([D-312], invariata da [D-408]).
 *
 * ⏱️ *Era `DeflectPoolAbsorbsBeforeGuardPool`. L'ordine non cambia — `D-312` non e' toccata — cambia il
 * secondo dei due meccanismi, e quindi il numero che ne esce.*
 *
 * 🔑 **L'ordine conta ancora, e va misurato invece che dedotto**: il `Deflect` consuma un budget, quindi
 * quanto ne resta dipende da quanto ha gia' morso; la Guardia toglie una quota fissa per colpo, che non
 * dipende da cosa e' venuto prima. Invertirli cambia il totale.
 *
 * ⛔ **Le due maschere sono ASIMMETRICHE, e non e' un dettaglio della scena: e' la premessa di [D-312].**
 * `Guard` e' eleggibile solo sui colpi FRONTALI ([D-206]), `Deflect` su tutti — nessuna clausola d'arco —
 * quindi le maschere si sovrappongono **parzialmente**, ed e' esattamente la condizione in cui l'ordine
 * cambia l'esito. La scena e' quella del test che questo sostituisce: un colpo PICCOLO davanti e uno
 * GRANDE alle spalle, che e' contro-intuitivo e voluto.
 *
 * ⚠️ **La prima stesura passava `{true, true}` a entrambe**, perdendo l'asimmetria: l'ordine restava
 * distinguibile per un'altra ragione, ma la ragione DICHIARATA da `RTTurnManager.cpp` — dove il commento
 * cita ancora le maschere parziali — non era piu' esercitata da nessuno a livello di resolver. Trovato
 * da una code review.
 *
 * ⛔ **E cio' che questo test NON prova, che la prima stesura aveva anch'esso perso**: chiama le due
 * funzioni DIRETTAMENTE, quindi resta verde qualunque ordine usi `RTTurnManager` — misurato allora,
 * invertendo le due chiamate reali 100 test su 100 restavano verdi. A chiudere il buco e'
 * `Combat.GuardAndDeflectAbsorbInDeclaredOrder`, che passa dal manager.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectBeforeGuardTest,
	"RefactorTactics.Combat.DeflectAbsorbsBeforeGuardReduces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectBeforeGuardTest::RunTest(const FString&)
{
	TArray<int32> PoolDeflect;  PoolDeflect.Init(0, 2);
	PoolDeflect[1] = URTCombatLibrary::DeflectDamageReduction;        // 20 — budget
	TArray<int32> Riduzione;    Riduzione.Init(0, 2);
	Riduzione[1] = -URTCombatLibrary::GuardFirstHitReduction;         // -15 — quota per colpo

	// Colpo 0: 5 danni, FRONTALE.  Colpo 1: 20 danni, DALLE SPALLE.
	const TArray<FRTAttack> Colpi = { FRTAttack(1, 5, 0), FRTAttack(1, 20, 2) };
	const TArray<bool> Frontali = { true, false };   // la Guardia copre solo il davanti ([D-206])
	const TArray<bool> Diretti  = { true, true };    // il Deflect non ha clausola d'arco ([D-309])

	auto Totale = [](const TArray<FRTAttack>& In)
	{
		int32 S = 0;
		for (const FRTAttack& A : In) { S += A.Power; }
		return S;
	};

	// L'ordine di produzione: prima il `Deflect`, poi la Guardia.
	const int32 DeflectPrima = Totale(URTCombatResolver::ApplyEligibleHitDelta(
		URTCombatResolver::ApplyAbsorptionPool(Colpi, PoolDeflect, Diretti,
			URTCombatLibrary::ReactionReductionPoolSource),
		Riduzione, Frontali, URTCombatLibrary::GuardPerHitSource));

	// L'ordine opposto, che [D-312] scarta.
	const int32 GuardiaPrima = Totale(URTCombatResolver::ApplyAbsorptionPool(
		URTCombatResolver::ApplyEligibleHitDelta(Colpi, Riduzione, Frontali,
			URTCombatLibrary::GuardPerHitSource),
		PoolDeflect, Diretti, URTCombatLibrary::ReactionReductionPoolSource));

	// ANTI-VACUITA', ed e' il punto: se i due ordini dessero lo stesso numero, `D-312` non avrebbe un
	// soggetto e questo test non misurerebbe niente.
	TestNotEqual(TEXT("i due ordini danno numeri diversi: la decisione ha un soggetto"),
		DeflectPrima, GuardiaPrima);

	// 25 nominali. Deflect (su entrambi): 5->0 col budget a 15, poi 20->5 esaurendolo. La Guardia vede
	// solo il colpo 0, gia' a zero, e non ha niente da togliere. Totale **5**.
	TestEqual(TEXT("nell'ordine di [D-312] il bersaglio riceve 5"), DeflectPrima, 5);
	// Invertito: la Guardia azzera il colpo frontale da 5 e non tocca quello alle spalle; poi il Deflect
	// assorbe i 20 di dietro col suo budget intatto. Totale **0**, cioe' PIU' protezione.
	TestEqual(TEXT("nell'ordine opposto sarebbe 0, cioe' PIU' protezione"), GuardiaPrima, 0);
	return true;
}

/**
 * **Uno stadio che non ha cambiato niente NON compare nel registro** (`#1951` + [D-408]).
 *
 * 🔴 **Il caso e' quello di produzione, non un limite**: [D-312] fa passare il `Deflect` PRIMA della
 * Guardia, quindi un colpo frontale che il pool della reazione ha gia' azzerato arriva alla Guardia con
 * `Power == 0`. Senza la guardia in `ApplyEligibleHitDelta` il breakdown porterebbe una voce che dichiara
 * una riduzione di 15 su un colpo da cui non ha tolto niente.
 *
 * ⚠️ **E' una regressione che la migrazione avrebbe introdotto in silenzio**: `ApplyAbsorptionPool` la
 * regola ce l'ha (`if (Absorbed > 0)`), e la Guardia l'ha persa passando a un delta. Trovata da una code
 * review, non da un rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardWritesNoEmptyStageTest,
	"RefactorTactics.Combat.GuardWritesNoStageWhenItRemovesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardWritesNoEmptyStageTest::RunTest(const FString&)
{
	TArray<int32> Riduzione;  Riduzione.Init(0, 2);  Riduzione[1] = -15;
	const TArray<bool> Eleggibile = { true };

	auto VociGuardia = [](const FRTAttack& A)
	{
		int32 N = 0;
		for (const FRTDamageStageEntry& E : A.Breakdown)
		{
			if (E.Stage == ERTDamageStage::EveryHitDelta
				&& E.SourceId == URTCombatLibrary::GuardPerHitSource) { ++N; }
		}
		return N;
	};

	// Il colpo arriva gia' a zero: e' cio' che il `Deflect` produce su un colpo piu' piccolo del suo budget.
	const TArray<FRTAttack> Azzerato = { FRTAttack(1, 0, 0) };
	const FRTAttack& Muto = URTCombatResolver::ApplyEligibleHitDelta(
		Azzerato, Riduzione, Eleggibile, URTCombatLibrary::GuardPerHitSource)[0];
	TestEqual(TEXT("su un colpo gia' a zero la Guardia non lascia voce"), VociGuardia(Muto), 0);

	// ANTI-VACUITA': se la guardia fosse troppo larga anche il caso che MORDE resterebbe muto, e la riga
	// sopra passerebbe per la ragione sbagliata.
	const TArray<FRTAttack> Vero = { FRTAttack(1, 12, 0) };
	const FRTAttack& Morso = URTCombatResolver::ApplyEligibleHitDelta(
		Vero, Riduzione, Eleggibile, URTCombatLibrary::GuardPerHitSource)[0];
	TestEqual(TEXT("su un colpo vero la voce c'e'"), VociGuardia(Morso), 1);
	TestEqual(TEXT("e il colpo e' sceso a zero"), Morso.Power, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
