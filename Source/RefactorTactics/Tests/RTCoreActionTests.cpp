#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Definizione dal catalogo delle azioni generiche. Nome distinto per file (unity build). */
	FRTActionDef CoreActionDef(const TCHAR* Id)
	{
		return URTCatalogLibrary::FindCoreAction(FName(Id));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoreActionsMatchDocumentTest,
	"RefactorTactics.Actions.CoreActionsMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoreActionsMatchDocumentTest::RunTest(const FString&)
{
	// Le sei azioni fondamentali con fase, priorita' e fallback della tabella §1 del catalogo. Il documento e
	// il codice non possono divergere in silenzio: se una riga cambia di la', questo test diventa rosso.
	struct FExpected
	{
		const TCHAR* Id;
		ERTMatchPhase Macro;
		int32 Priority;
		ERTActionFallback Fallback;
		ERTActionSlot Slot;
	};
	const FExpected Expected[] = {
		{ TEXT("Action.Wait"),        ERTMatchPhase::Move,  100, ERTActionFallback::Stop,   ERTActionSlot::None },
		{ TEXT("Action.Move"),        ERTMatchPhase::Move,   50, ERTActionFallback::Stop,   ERTActionSlot::Movement },
		{ TEXT("Action.BasicAttack"), ERTMatchPhase::Blast,  50, ERTActionFallback::Cancel, ERTActionSlot::Main },
		{ TEXT("Action.Guard"),       ERTMatchPhase::Prep,   40, ERTActionFallback::Cancel, ERTActionSlot::Main },
		{ TEXT("Action.Activate"),    ERTMatchPhase::Blast,  70, ERTActionFallback::Cancel, ERTActionSlot::Main },
		{ TEXT("Action.Interact"),    ERTMatchPhase::Blast,  80, ERTActionFallback::Cancel, ERTActionSlot::Main },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = CoreActionDef(E.Id);
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: macro-fase"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Macro);
		TestEqual(FString::Printf(TEXT("%s: priorita'"), E.Id), Def.Priority, E.Priority);
		TestTrue(FString::Printf(TEXT("%s: fallback"), E.Id), Def.Fallback == E.Fallback);
		TestTrue(FString::Printf(TEXT("%s: slot"), E.Id), Def.Slot == E.Slot);
	}

	// Il Move risolve DOPO il Blast: e' la divergenza dichiarata dall'ADR-0003 §3, non un dettaglio.
	TestTrue(TEXT("il movimento normale segue gli attacchi"),
		static_cast<uint8>(ERTMatchPhase::Move) > static_cast<uint8>(ERTMatchPhase::Blast));

	// `Activate` e `Interact` agiscono solo su cio' che e' ADIACENTE.
	TestEqual(TEXT("Activate: solo adiacente"), CoreActionDef(TEXT("Action.Activate")).RangeCells, 1);
	TestEqual(TEXT("Interact: solo adiacente"), CoreActionDef(TEXT("Action.Interact")).RangeCells, 1);

	// L'intero catalogo generico passa dal validator, nuove voci comprese.
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("le azioni generiche restano valide"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaitAllowsFacingTest,
	"RefactorTactics.Actions.Wait.AllowsFacingAndReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaitAllowsFacingTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. `Wait` non e' «non fare niente»: e' non spendere NULLA. Non occupa il
	// movimento ne' l'azione principale, quindi cio' che non costa slot — facing, reazione preparata, stance
	// gia' attiva, contesa di un obiettivo — resta possibile.
	const FRTActionDef Wait = CoreActionDef(TEXT("Action.Wait"));
	if (!TestTrue(TEXT("Action.Wait e' nel catalogo"), Wait.ActionId == FName(TEXT("Action.Wait"))))
	{
		return false;
	}

	TestTrue(TEXT("non occupa alcuno slot del turno"), Wait.Slot == ERTActionSlot::None);
	TestEqual(TEXT("non produce effetti"), Wait.Effects.Num(), 0);
	TestEqual(TEXT("risolve per ultima, cosi' non anticipa nulla"), Wait.Priority, 100);
	TestFalse(TEXT("non e' interrompibile: non c'e' nulla da interrompere"), Wait.bCanBeInterrupted);

	// Confronto che rende esplicita la differenza: l'azione principale e il movimento SI spendono.
	TestTrue(TEXT("l'attacco base occupa l'azione principale"),
		CoreActionDef(TEXT("Action.BasicAttack")).Slot == ERTActionSlot::Main);
	TestTrue(TEXT("il movimento occupa lo slot movimento"),
		CoreActionDef(TEXT("Action.Move")).Slot == ERTActionSlot::Movement);

	// Limite dichiarato: facing e reazioni non esistono ancora (E5 e la presentazione). Qui si fissa la
	// proprieta' che li rendera' possibili — che `Wait` non consumi lo slot che servirebbe loro.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBasicAttackBandTest,
	"RefactorTactics.Actions.BasicAttack.DamageByRangeBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBasicAttackBandTest::RunTest(const FString&)
{
	// Tabella delle fasce (catalogo §1): piu' lontano si colpisce, meno si fa male.
	TestEqual(TEXT("corpo a corpo (r1): 28"), URTCatalogLibrary::BasicAttackDamageForRange(1), 28);
	TestEqual(TEXT("corto raggio (r3): 25"), URTCatalogLibrary::BasicAttackDamageForRange(3), 25);
	TestEqual(TEXT("medio raggio (r4): 22"), URTCatalogLibrary::BasicAttackDamageForRange(4), 22);
	TestEqual(TEXT("lungo raggio (r6): 20"), URTCatalogLibrary::BasicAttackDamageForRange(6), 20);

	// La curva non e' mai crescente: nessuna portata paga meno di una piu' corta.
	for (int32 Range = 1; Range < 10; ++Range)
	{
		TestTrue(FString::Printf(TEXT("r%d non fa piu' male di r%d"), Range + 1, Range),
			URTCatalogLibrary::BasicAttackDamageForRange(Range + 1)
			<= URTCatalogLibrary::BasicAttackDamageForRange(Range));
	}

	// L'azione costruita per un'arma ha UN solo ID e i due numeri della sua fascia.
	const FRTActionDef Melee = URTCatalogLibrary::MakeBasicAttack(1);
	TestTrue(TEXT("stesso ActionId per tutti gli eroi"), Melee.ActionId == FName(TEXT("Action.BasicAttack")));
	TestEqual(TEXT("portata dell'arma"), Melee.RangeCells, 1);
	TestEqual(TEXT("un solo effetto: il danno"), Melee.Effects.Num(), 1);
	TestTrue(TEXT("ed e' il danno della fascia"), Melee.Effects.Num() == 1
		&& Melee.Effects[0].Effect == ERTActionEffect::Damage && Melee.Effects[0].Amount == 28);

	const FRTActionDef Ranged = URTCatalogLibrary::MakeBasicAttack(6);
	TestTrue(TEXT("a lungo raggio il danno scende"), Ranged.Effects.Num() == 1 && Ranged.Effects[0].Amount == 20);

	// Portata degenere: si ricade sul corpo a corpo invece di produrre un'arma a portata zero.
	TestEqual(TEXT("portata 0 -> corpo a corpo"), URTCatalogLibrary::MakeBasicAttack(0).RangeCells, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardFirstHitOnlyTest,
	"RefactorTactics.Actions.Guard.FirstHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardFirstHitOnlyTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. La guardia vale sul PRIMO danno diretto: il secondo colpo dello stesso turno
	// arriva intero. Passa dalla stessa funzione di `Status.Exposed` (CP 4.2), che applica i delta validi una
	// volta sola per bersaglio — quindi la regola resta ordine-indipendente.
	TArray<int32> Delta;
	Delta.Init(0, 2);
	Delta[1] = -URTCombatLibrary::GuardFirstHitReduction;

	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30), FRTAttack(1, 30) };
	const TArray<FRTAttack> Guarded = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);

	if (!TestEqual(TEXT("due colpi restano due"), Guarded.Num(), 2)) { return false; }
	TestEqual(TEXT("il primo colpo e' ridotto di 15"), Guarded[0].Power, 15);
	TestEqual(TEXT("il secondo arriva intero: la guardia vale una volta"), Guarded[1].Power, 30);

	// Un colpo piu' debole della riduzione viene annullato, non trasformato in cura.
	const TArray<FRTAttack> Small = URTCombatResolver::ApplyFirstHitDelta({ FRTAttack(1, 10) }, Delta);
	TestEqual(TEXT("un colpo da 10 viene azzerato"), Small[0].Power, 0);

	// Guardia ED esposizione insieme: i due delta si cumulano (+5 -15 = -10), esito prevedibile di aver fatto
	// entrambe le cose nello stesso turno.
	TArray<int32> Both;
	Both.Init(0, 2);
	Both[1] = URTCombatLibrary::ExposedFirstHitBonus - URTCombatLibrary::GuardFirstHitReduction;
	const TArray<FRTAttack> Mixed = URTCombatResolver::ApplyFirstHitDelta({ FRTAttack(1, 30) }, Both);
	TestEqual(TEXT("esposto e in guardia: 30 + 5 - 15"), Mixed[0].Power, 20);

	// L'azione che lo produce e' dati: Guard dichiara lo stato, non un numero nell'orchestratore.
	const FRTActionDef Guard = CoreActionDef(TEXT("Action.Guard"));
	TestTrue(TEXT("Guard si prepara nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Guard.ResolutionPhase) == ERTMatchPhase::Prep);
	TestEqual(TEXT("Guard dichiara un solo effetto"), Guard.Effects.Num(), 1);
	TestTrue(TEXT("ed e' Status.Guarded per un turno (scade nel Cleanup)"),
		Guard.Effects.Num() == 1 && Guard.Effects[0].Effect == ERTActionEffect::Status
		&& Guard.Effects[0].StatusTag == TAG_Status_Guarded && Guard.Effects[0].StatusDuration == 1);
	TestFalse(TEXT("la guardia non e' interrompibile"), Guard.bCanBeInterrupted);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
