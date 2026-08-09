#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Vero se l'azione dichiara un effetto del tipo dato (qualunque quantita'). Nome distinto per file. */
	bool RivaDeclaresEffect(const URTActionData* Ability, ERTActionEffect Kind)
	{
		if (!Ability) { return false; }
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == Kind) { return true; }
		}
		return false;
	}

	int32 RivaEffectAmount(const TArray<FRTActionEffectSpec>& Effects, ERTActionEffect Kind)
	{
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == Kind) { return Spec.Amount; }
		}
		return 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRivaMatchesCatalogTest,
	"RefactorTactics.Heroes.Riva.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRivaMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §2 del catalogo eroi v0.1.
	URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	if (!TestNotNull(TEXT("Riva costruita"), Riva)) { return false; }

	TestEqual(TEXT("HeroId"), Riva->HeroId, FName(TEXT("Hero.Riva")));
	TestEqual(TEXT("salute"), Riva->MaxHealth, 95);
	TestEqual(TEXT("movimento"), Riva->MovePoints, 5);
	TestEqual(TEXT("vista"), Riva->VisionRange, 5);
	TestEqual(TEXT("resistenza push"), Riva->PushResistance, 0);
	TestEqual(TEXT("affinita'"), Riva->Affinity, FName(TEXT("Affinity.Water")));
	// Simmetrica a Flux: la stessa combo si legge da entrambi i lati con lo stesso nome.
	TestEqual(TEXT("debolezza simmetrica a Flux"), Riva->Weakness, FName(TEXT("Affinity.Electricity")));

	if (!TestEqual(TEXT("cinque azioni"), Riva->Actions.Num(), 5)) { return false; }

	const URTActionData* PressureJet = Riva->Actions[0];
	TestEqual(TEXT("PressureJet: 16 danni"), RivaEffectAmount(PressureJet->Def.Effects, ERTActionEffect::Damage), 16);
	TestTrue(TEXT("PressureJet: applica Wet"), RivaDeclaresEffect(PressureJet, ERTActionEffect::Status));
	TestEqual(TEXT("PressureJet: Push 1"), RivaEffectAmount(PressureJet->Def.Effects, ERTActionEffect::Push), 1);
	TestEqual(TEXT("PressureJet: nessuna ricarica (e' l'attacco base)"), PressureJet->Def.CooldownTurns, 0);
	TestTrue(TEXT("PressureJet: forma a linea"), PressureJet->Shape == ERTAbilityShape::Line);

	// FluidTrail NON e' piu' uno scatto (D-039, issue #282): e' il produttore d'acqua del roster, cablato su
	// `Action.CreateWater`. Il cambio e' di kit, non di numeri — Riva perde la mobilita' rapida — e va scritto
	// qui perche' questo test E' la documentazione vincolante del kit: se un giorno tornasse un Dash, deve
	// cadere qualcosa.
	const URTActionData* FluidTrail = Riva->Actions[2];
	const FRTActionDef WaterDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateWater"));
	TestEqual(TEXT("FluidTrail: portata dal core, non un numero nuovo"), FluidTrail->Def.RangeCells, WaterDef.RangeCells);
	TestEqual(TEXT("FluidTrail: cooldown 2"), FluidTrail->Def.CooldownTurns, 2);
	TestTrue(TEXT("FluidTrail: risolve nell'ambiente"),
		FluidTrail->Def.ResolutionPhase == ERTResolutionPhase::Environment);
	// Nessun residuo dello scatto: uno stile di movimento rimasto verrebbe letto come intenzione.
	TestTrue(TEXT("FluidTrail: non si muove piu'"), FluidTrail->Def.MovementStyle == ERTMovementStyle::None);

	const URTActionData* MistVeil = Riva->Actions[3];
	TestEqual(TEXT("MistVeil: cooldown 3"), MistVeil->Def.CooldownTurns, 3);

	// Riva e' un roster valido di per se'.
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Riva });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Riva e' strutturalmente valida"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRivaTideHealsAlliesWetsEnemiesTest,
	"RefactorTactics.Heroes.Riva.TideHealsAlliesWetsEnemies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRivaTideHealsAlliesWetsEnemiesTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: CircularTide dichiara ENTRAMBI gli effetti (cura per gli alleati, Wet per i
	// nemici) come dati. Verifica la dichiarazione, non l'applicazione: nessun resolver oggi differenzia
	// l'effetto fra alleati e nemici della stessa area (limite dichiarato sotto).
	const URTActionData* CircularTide = URTHeroCatalogLibrary::MakeRiva()->Actions[1];

	TestTrue(TEXT("dichiara una cura"), RivaDeclaresEffect(CircularTide, ERTActionEffect::Heal));
	TestEqual(TEXT("cura 18"), RivaEffectAmount(CircularTide->Def.Effects, ERTActionEffect::Heal), 18);

	TestTrue(TEXT("dichiara Wet"), RivaDeclaresEffect(CircularTide, ERTActionEffect::Status));
	bool bFoundWet = false;
	for (const FRTActionEffectSpec& Spec : CircularTide->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Wet)
		{
			bFoundWet = true;
			TestEqual(TEXT("Wet dura 1 turno"), Spec.StatusDuration, 1);
		}
	}
	TestTrue(TEXT("ed e' proprio Status.Wet"), bFoundWet);

	TestEqual(TEXT("portata 4"), CircularTide->Def.RangeCells, 4);
	TestEqual(TEXT("cooldown 2"), CircularTide->Def.CooldownTurns, 2);
	TestTrue(TEXT("area"), CircularTide->Shape == ERTAbilityShape::Area);
	TestEqual(TEXT("raggio 1"), CircularTide->AreaRadius, 1);

	// Limite dichiarato: `bFriendlyFire` (CP 4.6) decide SE un alleato viene colpito da un'area, non CON
	// QUALE effetto — quindi "cura gli alleati, applica Wet ai nemici" non e' ancora eseguibile da
	// `CollectHexAttacks`/`ProduceEvents` cosi' come sono oggi. La differenziazione arriva quando Riva sara'
	// davvero cablata in un turno (CP 6.6+), non prima: qui si verifica solo che i due numeri esistano.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRivaVariantTradeoffTest,
	"RefactorTactics.Heroes.Riva.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRivaVariantTradeoffTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: nessuna variante di CircularTide e' migliore in ogni parametro.
	const URTActionData* CircularTide = URTHeroCatalogLibrary::MakeRiva()->Actions[1];
	if (!TestEqual(TEXT("due varianti"), CircularTide->Variants.Num(), 2)) { return false; }

	const FRTAbilityVariant& Healing = CircularTide->Variants[0];
	const FRTAbilityVariant& Impact = CircularTide->Variants[1];

	TestEqual(TEXT("curativa: cura 24"), RivaEffectAmount(Healing.Effects, ERTActionEffect::Heal), 24);
	TestFalse(TEXT("curativa: nessun Wet ai nemici"), [&Healing] {
		for (const FRTActionEffectSpec& Spec : Healing.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Status) { return true; }
		}
		return false;
	}());

	TestEqual(TEXT("urto: cura 10"), RivaEffectAmount(Impact.Effects, ERTActionEffect::Heal), 10);
	TestEqual(TEXT("urto: Push 1"), RivaEffectAmount(Impact.Effects, ERTActionEffect::Push), 1);

	// La prova della DoD: la curativa vince sulla cura (24 > 10), l'urto vince sull'utilita' di controllo
	// (Push 1, che la curativa non ha). Nessuna delle due domina l'altra in ogni parametro.
	const int32 HealingAmount = RivaEffectAmount(Healing.Effects, ERTActionEffect::Heal);
	const int32 ImpactAmount = RivaEffectAmount(Impact.Effects, ERTActionEffect::Heal);
	const int32 ImpactPush = RivaEffectAmount(Impact.Effects, ERTActionEffect::Push);

	TestTrue(TEXT("curativa cura di piu'"), HealingAmount > ImpactAmount);
	TestTrue(TEXT("urto ha un vantaggio che la curativa non ha (Push)"), ImpactPush > 0);

	// Ne' l'una ne' l'altra e' un puro potenziamento della base (18): entrambe cambiano qualcosa in cambio.
	TestTrue(TEXT("curativa costa il setup Wet"), HealingAmount > 18);
	TestTrue(TEXT("urto costa cura"), ImpactAmount < 18);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
