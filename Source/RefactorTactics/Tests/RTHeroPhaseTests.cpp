#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexCellData.h" // ERTHexSurface: la superficie che MistVeil dichiara di creare

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseMatchesCatalogTest,
	"RefactorTactics.Heroes.Phase.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §2 del catalogo eroi v0.1.
	URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	if (!TestNotNull(TEXT("Phase costruita"), Phase)) { return false; }

	TestEqual(TEXT("HeroId"), Phase->HeroId, FName(TEXT("Hero.Phase")));
	TestEqual(TEXT("salute"), Phase->MaxHealth, 95);
	TestEqual(TEXT("movimento"), Phase->MovePoints, 5);
	TestEqual(TEXT("vista"), Phase->VisionRange, 5);
	TestEqual(TEXT("resistenza push"), Phase->PushResistance, 0);
	TestEqual(TEXT("affinita'"), Phase->Affinity, FName(TEXT("Affinity.Water")));
	// Simmetrica a Gadget: la stessa combo si legge da entrambi i lati con lo stesso nome.
	TestEqual(TEXT("debolezza simmetrica a Gadget"), Phase->Weakness, FName(TEXT("Affinity.Electricity")));

	if (!TestEqual(TEXT("cinque azioni"), Phase->Actions.Num(), 5)) { return false; }

	const URTActionData* PressureJet = Phase->Actions[0];
	TestEqual(TEXT("PressureJet: 16 danni"), RivaEffectAmount(PressureJet->Def.Effects, ERTActionEffect::Damage), 16);
	TestTrue(TEXT("PressureJet: applica Wet"), RivaDeclaresEffect(PressureJet, ERTActionEffect::Status));
	TestEqual(TEXT("PressureJet: Push 1"), RivaEffectAmount(PressureJet->Def.Effects, ERTActionEffect::Push), 1);
	TestEqual(TEXT("PressureJet: nessuna ricarica (e' l'attacco base)"), PressureJet->Def.CooldownTurns, 0);
	TestTrue(TEXT("PressureJet: forma a linea"), PressureJet->Shape == ERTAbilityShape::Line);

	// FluidTrail TORNA a essere uno scatto (#1006), e il commento precedente prevedeva questo giro: diceva
	// «se un giorno tornasse un Dash, deve cadere qualcosa». E' caduto, ed e' stato sostituito qui.
	//
	// Il perche' non e' di mobilita': viene da #995. Phase e' **abilitata** a Water, non padrona — grado
	// `Access`, cioe' UNA sola capability elementale — e il catalogo ne dichiarava tre: `PressureJet`
	// (Apply Wet), `CircularTide` (Apply Wet) e questa (Generate della superficie). Resta `PressureJet`.
	// D-046 aveva cablato questa su `Action.CreateWater` per dare un owner all'acqua; l'owner ora e'
	// l'EQUIPAGGIAMENTO — `Gadget.Sprinkler` porta gia' `Action.CreateWater` — e per la grammatica di #995
	// un Generic Equipment e' `External Access` e non fa proficiency.
	//
	// ⚠️ Il costo e' dichiarato in #1006 e non va dimenticato leggendo solo questa riga: il roster perde
	// l'unico produttore INNATO di superficie acqua, quindi `Gadget.ConductiveNode` — che propaga sul grafo
	// conduttivo — dipende dalla mappa o dallo Sprinkler. La vetrina Conflux di D-046 ne risente.
	const URTActionData* FluidTrail = Phase->Actions[2];
	const FRTActionDef DashDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	TestEqual(TEXT("FluidTrail: portata dal core, non un numero nuovo"), FluidTrail->Def.RangeCells, DashDef.RangeCells);
	TestEqual(TEXT("FluidTrail: cooldown 2, il proprio e non quello del core"), FluidTrail->Def.CooldownTurns, 2);
	TestTrue(TEXT("FluidTrail: e' movimento rapido, non ambiente"),
		FluidTrail->Def.ResolutionPhase == ERTResolutionPhase::FastMovement);
	TestTrue(TEXT("FluidTrail: si muove di nuovo, in linea"),
		FluidTrail->Def.MovementStyle == ERTMovementStyle::LinearDash);
	// Lo SLOT e' la meta' della decisione, non un dettaglio: uno scatto occupa il Movimento (D-028), cosi'
	// chi lo usa conserva l'azione principale — *schivo e sparo*. Con lo slot `Main` di default sarebbe una
	// mobilita' che costa un attacco, cioe' un'altra abilita'.
	TestTrue(TEXT("FluidTrail: occupa il Movimento (D-028), non l'azione principale"),
		FluidTrail->Def.Slot == ERTActionSlot::Movement);
	// Nessun residuo dell'acqua: un flag di superficie rimasto verrebbe letto come intenzione, ed e' il modo
	// piu' silenzioso in cui questa abilita' e' gia' fallita una volta (issue #353 su MistVeil).
	TestFalse(TEXT("FluidTrail: non crea piu' superficie"), FluidTrail->Def.bCreatesSurface);
	TestEqual(TEXT("FluidTrail: nessun effetto dichiarato"), FluidTrail->Def.Effects.Num(), 0);

	// MistVeil crea DAVVERO il fumo (issue #353). Come per FluidTrail, questo test e' la documentazione
	// vincolante del kit: se un giorno tornasse `Preparation`, o il flag sparisse, deve cadere qualcosa —
	// perche' il modo in cui questa abilita' falliva era il piu' silenzioso possibile (risolveva, e non
	// succedeva niente).
	const URTActionData* MistVeil = Phase->Actions[3];
	const FRTActionDef IgniteDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Ignite"));
	TestEqual(TEXT("MistVeil: cooldown 3"), MistVeil->Def.CooldownTurns, 3);
	TestTrue(TEXT("MistVeil: crea una superficie"), MistVeil->Def.bCreatesSurface);
	TestTrue(TEXT("MistVeil: e' fumo"), MistVeil->Def.SurfaceCreated == ERTHexSurface::Smoke);
	TestEqual(TEXT("MistVeil: raggio 1, come dice il catalogo"), MistVeil->Def.SurfaceRadius, 1);
	// La fase e' il punto della issue: in `Preparation` non arrivava mai a `ResolveEnvironment`.
	TestTrue(TEXT("MistVeil: risolve nell'ambiente, o non viene mai eseguita"),
		MistVeil->Def.ResolutionPhase == ERTResolutionPhase::Environment);
	TestEqual(TEXT("MistVeil: portata dal core, non un numero nuovo"),
		MistVeil->Def.RangeCells, IgniteDef.RangeCells);
	TestEqual(TEXT("MistVeil: priorita' dal core — ordina DENTRO la fase in cui e' andata a vivere"),
		MistVeil->Def.Priority, IgniteDef.Priority);

	// Phase e' un roster valido di per se'.
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Phase });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Phase e' strutturalmente valida"), Errors.Num(), 0);
	return true;
}

/**
 * `CircularTide` cura e basta: il `Wet` esce dalla sua dichiarazione (#1006).
 *
 * ⚠️ **Questo test SOSTITUISCE `Heroes.Phase.TideHealsAlliesWetsEnemies`, non lo affianca.** Quel nome
 * pinnava la doppia natura — cura agli alleati, `Wet` ai nemici — che era il contenuto di CP 6.3. Il nome
 * e' cambiato insieme al contratto perche' un test che si chiama `...WetsEnemies` e non verifica piu'
 * nessun `Wet` e' peggio di un test cancellato: resta verde e racconta un kit che non esiste.
 *
 * Il perche' viene da #995 via #1006: Phase e' **abilitata** a Water, non padrona — grado `Access`, una
 * sola capability elementale — e il catalogo ne dichiarava tre. Resta `PressureJet`.
 *
 * ⚠️ **Costo di gameplay dichiarato**: quel `Wet` ad area era il preparatore della combo con Gadget
 * (`LinearDischarge` fa **+8 su bersaglio `Wet`**). Dopo questa modifica la combo passa solo per la linea
 * di `PressureJet`, che colpisce meno bersagli. E' il prezzo accettato con l'opzione C di #1006.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseTideHealsWithoutWettingTest,
	"RefactorTactics.Heroes.Phase.TideHealsWithoutWetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseTideHealsWithoutWettingTest::RunTest(const FString&)
{
	const URTActionData* CircularTide = URTHeroCatalogLibrary::MakePhase()->Actions[1];

	// Le assertion ancora vive del test precedente: la cura e i numeri di forma non cambiano.
	TestTrue(TEXT("dichiara una cura"), RivaDeclaresEffect(CircularTide, ERTActionEffect::Heal));
	TestEqual(TEXT("cura 18"), RivaEffectAmount(CircularTide->Def.Effects, ERTActionEffect::Heal), 18);
	TestEqual(TEXT("portata 4"), CircularTide->Def.RangeCells, 4);
	TestEqual(TEXT("cooldown 2"), CircularTide->Def.CooldownTurns, 2);
	TestTrue(TEXT("area"), CircularTide->Shape == ERTAbilityShape::Area);
	TestEqual(TEXT("raggio 1"), CircularTide->AreaRadius, 1);

	// L'assertion NUOVA, ed e' quella per cui questo test esiste: nessuno `Status` dichiarato, e in
	// particolare nessun `Wet`. Le due condizioni sono scritte separate di proposito — la prima cade se
	// qualcuno rimette un tag qualsiasi, la seconda dice quale tag stiamo sorvegliando.
	TestFalse(TEXT("non dichiara piu' alcuno Status"),
		RivaDeclaresEffect(CircularTide, ERTActionEffect::Status));
	for (const FRTActionEffectSpec& Spec : CircularTide->Def.Effects)
	{
		TestTrue(TEXT("nessun effetto e' Status.Wet"),
			!(Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Wet));
	}

	// La cura resta l'unico effetto: senza questa riga passerebbe anche un `Effects` svuotato del tutto,
	// che sarebbe un'abilita' inerte invece di una cura ad area.
	TestEqual(TEXT("la cura e' l'unico effetto rimasto"), CircularTide->Def.Effects.Num(), 1);

	// Limite dichiarato, ereditato e ancora valido: `bFriendlyFire` (CP 4.6) decide SE un alleato viene
	// colpito da un'area, non CON QUALE effetto. Qui si verifica la dichiarazione, non l'applicazione.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseVariantTradeoffTest,
	"RefactorTactics.Heroes.Phase.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseVariantTradeoffTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: nessuna variante di CircularTide e' migliore in ogni parametro.
	const URTActionData* CircularTide = URTHeroCatalogLibrary::MakePhase()->Actions[1];
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
