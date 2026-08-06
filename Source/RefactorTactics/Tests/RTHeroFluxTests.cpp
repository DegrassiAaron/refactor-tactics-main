#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Danno dichiarato da un'azione (0 se non ne dichiara). Nome distinto per file (unity build). */
	int32 FluxDeclaredDamage(const URTActionData* Ability)
	{
		if (!Ability) { return 0; }
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
		}
		return 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFluxMatchesCatalogTest,
	"RefactorTactics.Heroes.Flux.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFluxMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §1 del catalogo eroi v0.1: documento e codice non possono divergere in silenzio.
	URTHeroData* Flux = URTHeroCatalogLibrary::MakeFlux();
	if (!TestNotNull(TEXT("Flux costruito"), Flux)) { return false; }

	TestEqual(TEXT("HeroId"), Flux->HeroId, FName(TEXT("Hero.Flux")));
	TestEqual(TEXT("salute"), Flux->MaxHealth, 90);
	TestEqual(TEXT("movimento"), Flux->MovePoints, 5);
	TestEqual(TEXT("vista"), Flux->VisionRange, 6);
	TestEqual(TEXT("resistenza push"), Flux->PushResistance, 0);
	TestEqual(TEXT("affinita'"), Flux->Affinity, FName(TEXT("Affinity.Electricity")));
	TestFalse(TEXT("la debolezza e' dichiarata (non NAME_None)"), Flux->Weakness.IsNone());

	if (!TestEqual(TEXT("cinque azioni"), Flux->Actions.Num(), 5)) { return false; }

	const URTActionData* ArcPulse = Flux->Actions[0];
	TestEqual(TEXT("ArcPulse: ActionId"), ArcPulse->Def.ActionId, FName(TEXT("Flux.ArcPulse")));
	TestEqual(TEXT("ArcPulse: range 4"), ArcPulse->Def.RangeCells, 4);
	TestEqual(TEXT("ArcPulse: 22 danni (fascia medio raggio)"), FluxDeclaredDamage(ArcPulse), 22);
	TestEqual(TEXT("ArcPulse: nessuna ricarica (e' l'attacco base)"), ArcPulse->Def.CooldownTurns, 0);

	const URTActionData* LinearDischarge = Flux->Actions[1];
	TestEqual(TEXT("LinearDischarge: 24 danni"), FluxDeclaredDamage(LinearDischarge), 24);
	TestEqual(TEXT("LinearDischarge: cooldown 2"), LinearDischarge->Def.CooldownTurns, 2);
	TestTrue(TEXT("LinearDischarge: forma a linea"), LinearDischarge->Shape == ERTAbilityShape::Line);

	const URTActionData* Overload = Flux->Actions[3];
	TestEqual(TEXT("Overload: 18 danni"), FluxDeclaredDamage(Overload), 18);
	TestEqual(TEXT("Overload: cooldown 3"), Overload->Def.CooldownTurns, 3);

	const URTActionData* ReactiveCapacitor = Flux->Actions[4];
	if (TestEqual(TEXT("ReactiveCapacitor: un solo effetto"), ReactiveCapacitor->Def.Effects.Num(), 1))
	{
		TestTrue(TEXT("ReactiveCapacitor: ed e' lo scudo"),
			ReactiveCapacitor->Def.Effects[0].Effect == ERTActionEffect::Shield
			&& ReactiveCapacitor->Def.Effects[0].Amount == 15);
	}
	TestEqual(TEXT("ReactiveCapacitor: cooldown 3"), ReactiveCapacitor->Def.CooldownTurns, 3);

	// Flux e' un roster valido di per se': passa lo stesso validator che il roster completo dovra' passare.
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Flux });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Flux e' strutturalmente valido"), Errors.Num(), 0);

	// Limiti dichiarati: ConductiveNode e Overload non hanno gli effetti "cella conduttiva"/"interrupt
	// dispositivi" (nessun sistema li consuma, E7/E8/E9), ReactiveCapacitor non ha il contrattacco
	// (nessuno slot Reazione ne' un modo di riferire l'attaccante a runtime, E5). Qui si verifica solo che
	// l'azione ESISTA come dato, non che l'effetto mancante sia presente — sarebbe l'errore opposto.
	TestEqual(TEXT("ConductiveNode: nessun effetto rappresentabile ancora"), Flux->Actions[2]->Def.Effects.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFluxWetBonusTest,
	"RefactorTactics.Heroes.Flux.WetBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFluxWetBonusTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il bonus NON e' nella lista Effects (e' condizionale al bersaglio, non un
	// danno fisso): passa da `EffectiveAttackPower`, la stessa funzione del bonus di cella, riusata per un
	// bonus di STATO invece che di terreno.
	const int32 BaseDamage = FluxDeclaredDamage(URTHeroCatalogLibrary::MakeFlux()->Actions[1]);
	if (!TestEqual(TEXT("il danno base dichiarato e' 24"), BaseDamage, 24)) { return false; }

	const int32 AgainstDry = URTCombatLibrary::EffectiveAttackPower(BaseDamage, /*OccupantDamageBonus*/ 0);
	const int32 AgainstWet = URTCombatLibrary::EffectiveAttackPower(BaseDamage, URTCombatLibrary::FluxWetDischargeBonus);

	TestEqual(TEXT("bersaglio asciutto: 24"), AgainstDry, 24);
	TestEqual(TEXT("bersaglio Wet: 24 + 8 = 32"), AgainstWet, 32);
	TestEqual(TEXT("il bonus e' esattamente 8"), URTCombatLibrary::FluxWetDischargeBonus, 8);

	// Il bonus si somma a un altro bonus di cella (es. altura), non lo sostituisce: e' un caso normale di
	// `EffectiveAttackPower`, non uno speciale per Wet.
	TestEqual(TEXT("Wet + altura: si sommano"),
		URTCombatLibrary::EffectiveAttackPower(BaseDamage, URTCombatLibrary::FluxWetDischargeBonus + 3), 35);

	// Limite dichiarato: qui si verifica la FORMULA, non il resolver. Nessuna unita' legge ancora
	// `Status.Wet` durante il Blast (`CollectHexAttacks` non conosce lo stato del bersaglio) — il bonus
	// diventa osservabile in partita quando CP 6.6 collega Flux al turno E quando Riva (CP 6.3) applica
	// `Status.Wet` per davvero.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFluxVariantTradeoffTest,
	"RefactorTactics.Heroes.Flux.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFluxVariantTradeoffTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: "nessuna variante migliore in ogni parametro" — lo stesso principio degli
	// equipaggiamenti (URTEquipmentData::Drawback obbligatorio), applicato alla variante di un'abilita'.
	const URTActionData* LinearDischarge = URTHeroCatalogLibrary::MakeFlux()->Actions[1];
	if (!TestEqual(TEXT("LinearDischarge ha esattamente due varianti"), LinearDischarge->Variants.Num(), 2))
	{
		return false;
	}

	const FRTAbilityVariant& Concentrated = LinearDischarge->Variants[0];
	const FRTAbilityVariant& Branched = LinearDischarge->Variants[1];

	TestEqual(TEXT("concentrata: un solo evento di danno"), Concentrated.Effects.Num(), 1);
	TestEqual(TEXT("concentrata: 30 danni (24 + 6)"),
		Concentrated.Effects.Num() > 0 ? Concentrated.Effects[0].Amount : 0, 30);

	TestEqual(TEXT("ramificata: due eventi di danno (due bersagli)"), Branched.Effects.Num(), 2);
	for (const FRTActionEffectSpec& Spec : Branched.Effects)
	{
		TestEqual(TEXT("ramificata: 18 danni per bersaglio (24 - 6)"), Spec.Amount, 18);
	}

	// La prova della DoD: il danno per singolo bersaglio della concentrata (30) e' STRETTAMENTE maggiore di
	// quello della ramificata (18) — la concentrata non e' mai peggio sul bersaglio che colpisce — ma il
	// danno TOTALE possibile della ramificata (18+18=36, con un secondo nemico in linea) supera quello della
	// concentrata (30). Ognuna vince su un parametro e perde sull'altro: nessuna domina.
	const int32 ConcentratedSingleTarget = Concentrated.Effects[0].Amount;
	const int32 BranchedPerTarget = Branched.Effects[0].Amount;
	const int32 BranchedTotalIfBothLand = Branched.Effects[0].Amount + Branched.Effects[1].Amount;

	TestTrue(TEXT("concentrata: piu' danno sul bersaglio singolo"), ConcentratedSingleTarget > BranchedPerTarget);
	TestTrue(TEXT("ramificata: piu' danno totale se colpisce due bersagli"),
		BranchedTotalIfBothLand > ConcentratedSingleTarget);

	// Nessuna variante e' un puro potenziamento della base (24): entrambe cambiano qualcosa in cambio.
	TestTrue(TEXT("concentrata costa la propagazione"), ConcentratedSingleTarget > 24);
	TestTrue(TEXT("ramificata costa danno per bersaglio"), BranchedPerTarget < 24);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
