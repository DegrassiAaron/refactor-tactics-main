#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	int32 BastionEffectAmount(const TArray<FRTActionEffectSpec>& Effects, ERTActionEffect Kind)
	{
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == Kind) { return Spec.Amount; }
		}
		return 0;
	}

	/** Parametro di catalogo di una variante, o INDEX_NONE se non dichiarato. */
	int32 BastionVariantParam(const FRTAbilityVariant& Variant, const TCHAR* Key)
	{
		const int32* Found = Variant.Parameters.Find(FName(Key));
		return Found ? *Found : INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionMatchesCatalogTest,
	"RefactorTactics.Heroes.Bastion.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §3 del catalogo eroi v0.1.
	URTHeroData* Bastion = URTHeroCatalogLibrary::MakeBastion();
	if (!TestNotNull(TEXT("Bastion costruito"), Bastion)) { return false; }

	TestEqual(TEXT("HeroId"), Bastion->HeroId, FName(TEXT("Hero.Bastion")));
	TestEqual(TEXT("salute"), Bastion->MaxHealth, 120);
	TestEqual(TEXT("movimento"), Bastion->MovePoints, 4);
	TestEqual(TEXT("vista"), Bastion->VisionRange, 5);
	TestEqual(TEXT("affinita'"), Bastion->Affinity, FName(TEXT("Affinity.Structures")));
	TestEqual(TEXT("debolezza simmetrica a Vektor"), Bastion->Weakness, FName(TEXT("Affinity.Movement")));

	if (!TestEqual(TEXT("cinque azioni"), Bastion->Actions.Num(), 5)) { return false; }

	const URTActionData* ImpactShot = Bastion->Actions[0];
	TestEqual(TEXT("ImpactShot: 24 danni"), BastionEffectAmount(ImpactShot->Def.Effects, ERTActionEffect::Damage), 24);
	TestEqual(TEXT("ImpactShot: range 3"), ImpactShot->Def.RangeCells, 3);
	TestEqual(TEXT("ImpactShot: nessuna ricarica (e' l'attacco base)"), ImpactShot->Def.CooldownTurns, 0);

	// L'attacco base di Bastion NON viene dalla tabella a fasce: a range 3 la fascia darebbe 25, il catalogo
	// eroi dice 24. Il test lo rende esplicito, cosi' la divergenza resta una scelta e non una svista.
	TestNotEqual(TEXT("non e' il danno generico della fascia corto raggio"),
		BastionEffectAmount(ImpactShot->Def.Effects, ERTActionEffect::Damage),
		URTCatalogLibrary::BasicAttackDamageForRange(3));

	const URTActionData* Ram = Bastion->Actions[3];
	TestEqual(TEXT("Ram: 20 danni"), BastionEffectAmount(Ram->Def.Effects, ERTActionEffect::Damage), 20);
	TestEqual(TEXT("Ram: Push 1"), BastionEffectAmount(Ram->Def.Effects, ERTActionEffect::Push), 1);
	TestEqual(TEXT("Ram: cooldown 2"), Ram->Def.CooldownTurns, 2);

	// Ram e' `Action.Charge` con un nome d'eroe: stessa fase, stesso stile di movimento, stessi effetti.
	// Se il catalogo azioni cambiasse la carica, questo test lo direbbe subito.
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("Ram: risolve nella stessa fase della carica"),
		Ram->Def.ResolutionPhase == ChargeDef.ResolutionPhase);
	TestTrue(TEXT("Ram: si muove come una carica (si ferma addosso al nemico)"),
		Ram->Def.MovementStyle == ChargeDef.MovementStyle);
	TestEqual(TEXT("Ram: stessa portata della carica"), Ram->Def.RangeCells, ChargeDef.RangeCells);

	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Bastion });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Bastion e' strutturalmente valido"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionPushResistanceTest,
	"RefactorTactics.Heroes.Bastion.PushResistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionPushResistanceTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. La resistenza push 1 e' l'unica del roster: e' cio' che Bastion compra con
	// il movimento piu' basso (4 MP) e la vista corta, ed e' la prova che nessun eroe domina in ogni
	// parametro (gate di chiusura dell'epic E6).
	URTHeroData* Bastion = URTHeroCatalogLibrary::MakeBastion();
	TestEqual(TEXT("Bastion resiste a una spinta di 1"), Bastion->PushResistance, 1);

	URTHeroData* Flux = URTHeroCatalogLibrary::MakeFlux();
	URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	TestEqual(TEXT("Flux non resiste"), Flux->PushResistance, 0);
	TestEqual(TEXT("Riva non resiste"), Riva->PushResistance, 0);

	// Il prezzo, in dati: piu' salute di tutti, ma il movimento piu' basso finora.
	TestTrue(TEXT("piu' salute di Flux e Riva"),
		Bastion->MaxHealth > Flux->MaxHealth && Bastion->MaxHealth > Riva->MaxHealth);
	TestTrue(TEXT("ma meno movimento"),
		Bastion->MovePoints < Flux->MovePoints && Bastion->MovePoints < Riva->MovePoints);

	// Limite dichiarato: `PushResistance` e' un DATO senza consumatore. Il resolver applica oggi solo la
	// resistenza temporanea di `Status.Guarded` (`URTCombatLibrary::GuardResistedPushDistance`), non quella
	// base dell'eroe — vale da CP 6.1, e questo test verifica il dato, non il suo effetto in partita.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionPanelCreatesCoverTest,
	"RefactorTactics.Heroes.Bastion.PanelCreatesCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionPanelCreatesCoverTest::RunTest(const FString&)
{
	// Nome vincolante della DoD — ma **la copertura non esiste**: `FRTHexCellData` non ha coperture e
	// `ERTActionEffect` non sa crearne (E9, issue #69/#73). Questo test verifica cio' che si PUO' verificare
	// oggi — identita', fase, portata, cooldown — e fissa la proprieta' che rende il resto aggiungibile
	// dopo: il pannello si prepara PRIMA che i colpi partano.
	URTHeroData* Bastion = URTHeroCatalogLibrary::MakeBastion();
	const URTActionData* Panel = Bastion->Actions[1];

	TestEqual(TEXT("KineticPanel: ActionId"), Panel->Def.ActionId, FName(TEXT("Bastion.KineticPanel")));
	TestEqual(TEXT("KineticPanel: cooldown 2"), Panel->Def.CooldownTurns, 2);
	TestEqual(TEXT("KineticPanel: si piazza su una cella adiacente"), Panel->Def.RangeCells, 1);

	// Si costruisce nel Prep: una copertura che arrivasse dopo i colpi non proteggerebbe da nulla. E' la
	// regola che sopravvivera' a E9 anche quando l'effetto sara' reale.
	TestTrue(TEXT("KineticPanel: si prepara prima del Blast"),
		URTCatalogLibrary::MapResolutionPhase(Panel->Def.ResolutionPhase) == ERTMatchPhase::Prep);
	TestTrue(TEXT("Reconfigure: anche"), URTCatalogLibrary::MapResolutionPhase(
		Bastion->Actions[2]->Def.ResolutionPhase) == ERTMatchPhase::Prep);

	// **Limite dichiarato, verificato come tale**: nessun effetto e' dichiarato, perche' non esiste un
	// effetto che crei strutture. Il giorno in cui E9 lo aggiungera', questo test diventera' rosso — ed e'
	// esattamente cio' che deve succedere: e' il promemoria che l'abilita' e' a meta'.
	TestEqual(TEXT("KineticPanel: nessun effetto rappresentabile (E9)"), Panel->Def.Effects.Num(), 0);
	TestEqual(TEXT("Reconfigure: nessun effetto rappresentabile (E9)"), Bastion->Actions[2]->Def.Effects.Num(), 0);
	TestEqual(TEXT("Interposition: nessun effetto rappresentabile (E5)"), Bastion->Actions[4]->Def.Effects.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionVariantTradeoffTest,
	"RefactorTactics.Heroes.Bastion.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionVariantTradeoffTest::RunTest(const FString&)
{
	// Il compromesso di Bastion e' fatto di numeri che NON sono effetti (integrita', durata): vivono in
	// `FRTAbilityVariant::Parameters`, dichiarati finche' E9 non li consumera'.
	const URTActionData* Panel = URTHeroCatalogLibrary::MakeBastion()->Actions[1];
	if (!TestEqual(TEXT("due varianti"), Panel->Variants.Num(), 2)) { return false; }

	const FRTAbilityVariant& Reinforced = Panel->Variants[0];
	const FRTAbilityVariant& Adaptive = Panel->Variants[1];

	const int32 ReinforcedIntegrity = BastionVariantParam(Reinforced, TEXT("Integrity"));
	const int32 AdaptiveIntegrity = BastionVariantParam(Adaptive, TEXT("Integrity"));
	TestEqual(TEXT("rinforzato: integrita' 45"), ReinforcedIntegrity, 45);
	TestEqual(TEXT("adattivo: integrita' 25"), AdaptiveIntegrity, 25);

	// Nessuna variante e' migliore in ogni parametro: il rinforzato compra integrita' con la DURATA
	// (un turno solo), l'adattivo compra una rotazione gratuita con l'integrita'.
	TestTrue(TEXT("rinforzato: piu' integrita'"), ReinforcedIntegrity > AdaptiveIntegrity);
	TestEqual(TEXT("rinforzato: dura un solo turno"),
		BastionVariantParam(Reinforced, TEXT("DurationTurns")), 1);
	TestEqual(TEXT("rinforzato: nessuna rotazione gratuita"),
		BastionVariantParam(Reinforced, TEXT("FreeRotations")), 0);
	TestEqual(TEXT("adattivo: una rotazione gratuita"),
		BastionVariantParam(Adaptive, TEXT("FreeRotations")), 1);
	TestEqual(TEXT("adattivo: non scade da solo"),
		BastionVariantParam(Adaptive, TEXT("DurationTurns")), 0);

	// Entrambe si scostano dal pannello base del catalogo terreni (`Structure.KineticPanel`: integrita' 30),
	// una in su e una in giu': nessuna e' un puro potenziamento.
	TestTrue(TEXT("rinforzato: sopra la base (30)"), ReinforcedIntegrity > 30);
	TestTrue(TEXT("adattivo: sotto la base (30)"), AdaptiveIntegrity < 30);

	// Ogni variante dichiara il proprio compromesso a parole: e' cio' che l'UI mostrera' a chi sceglie.
	TestFalse(TEXT("rinforzato: compromesso dichiarato"), Reinforced.Tradeoff.IsEmpty());
	TestFalse(TEXT("adattivo: compromesso dichiarato"), Adaptive.Tradeoff.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
