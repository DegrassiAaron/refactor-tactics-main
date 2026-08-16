#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorMatchesCatalogTest,
	"RefactorTactics.Heroes.Riktor.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §3 del catalogo eroi v0.1.
	URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	if (!TestNotNull(TEXT("Riktor costruito"), Riktor)) { return false; }

	TestEqual(TEXT("HeroId"), Riktor->HeroId, FName(TEXT("Hero.Riktor")));
	TestEqual(TEXT("salute"), Riktor->MaxHealth, 120);
	TestEqual(TEXT("movimento"), Riktor->MovePoints, 4);
	TestEqual(TEXT("vista"), Riktor->VisionRange, 5);
	TestEqual(TEXT("affinita'"), Riktor->Affinity, FName(TEXT("Affinity.Structures")));
	TestEqual(TEXT("debolezza simmetrica a Wraith"), Riktor->Weakness, FName(TEXT("Affinity.Movement")));

	if (!TestEqual(TEXT("cinque azioni"), Riktor->Actions.Num(), 5)) { return false; }

	const URTActionData* ImpactShot = Riktor->Actions[0];
	TestEqual(TEXT("ImpactShot: 8 danni"), BastionEffectAmount(ImpactShot->Def.Effects, ERTActionEffect::Damage), 8);
	TestEqual(TEXT("ImpactShot: range 3"), ImpactShot->Def.RangeCells, 3);
	TestEqual(TEXT("ImpactShot: nessuna ricarica (e' l'attacco base)"), ImpactShot->Def.CooldownTurns, 0);

	// ADR-0007: l'attacco base di Riktor appartiene alla famiglia Utility/Emergency, e la utility e' `Slow`.
	// Senza questo assert il danno basso sarebbe indistinguibile da un nerf senza contropartita — cioe' dalla
	// falsa scelta che la decisione esiste per evitare.
	bool bFoundSlow = false;
	for (const FRTActionEffectSpec& Spec : ImpactShot->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Slow)
		{
			bFoundSlow = true;
			TestEqual(TEXT("lo Slow dura 1 turno"), Spec.StatusDuration, 1);
		}
	}
	TestTrue(TEXT("ImpactShot: applica Status.Slow"), bFoundSlow);

	// L'attacco base di Riktor NON viene dalla tabella a fasce: a range 3 la fascia darebbe 25, il catalogo
	// eroi dice 8. Il test lo rende esplicito, cosi' la divergenza resta una scelta e non una svista.
	TestNotEqual(TEXT("non e' il danno generico della fascia corto raggio"),
		BastionEffectAmount(ImpactShot->Def.Effects, ERTActionEffect::Damage),
		URTCatalogLibrary::BasicAttackDamageForRange(3));

	const URTActionData* Ram = Riktor->Actions[3];
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

	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Riktor });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Riktor e' strutturalmente valido"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorPushResistanceTest,
	"RefactorTactics.Heroes.Riktor.PushResistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorPushResistanceTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. **Il senso del test si e' rovesciato il 2026-08-10** (D-075, #402): fino a
	// qui pinnava `PushResistance = 1` come cio' che Riktor compra col movimento piu' basso. Ma siccome
	// ogni spinta del gioco vale 1 e la resistenza e' una soglia (D-038), quel valore rendeva Riktor immune
	// a OGNI spostamento, sempre, gratis — non era la statistica dichiarata, era un'immunita' che nessuno
	// aveva deciso. Ora il test pinna il roster **tutto a zero**, ed e' l'unico posto che diventa rosso se
	// qualcuno rimette una resistenza nativa senza passare da una decisione.
	URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
	URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
	TestEqual(TEXT("Riktor non ha piu' resistenza nativa"), Riktor->PushResistance, 0);
	TestEqual(TEXT("Gadget non resiste"), Gadget->PushResistance, 0);
	TestEqual(TEXT("Phase non resiste"), Phase->PushResistance, 0);
	TestEqual(TEXT("Wraith non resiste"), Wraith->PushResistance, 0);

	// Il prezzo, in dati: piu' salute di tutti, ma il movimento piu' basso finora.
	TestTrue(TEXT("piu' salute di Gadget e Phase"),
		Riktor->MaxHealth > Gadget->MaxHealth && Riktor->MaxHealth > Phase->MaxHealth);
	TestTrue(TEXT("ma meno movimento"),
		Riktor->MovePoints < Gadget->MovePoints && Riktor->MovePoints < Phase->MovePoints);

	// Il commento che stava qui diceva `PushResistance` "un DATO senza consumatore", e che il resolver
	// applicava solo `GuardResistedPushDistance`. **Era invecchiato**: il ramo `ERTActionEffect::Push` di
	// `RTTurnManager.cpp` la consuma da quando D-038 e' stata cablata. Rimosso il 2026-08-10 con #402.
	//
	// Il limite vero e' un altro, e vale la pena scriverlo: il campo ha oggi **zero utenti nel roster**.
	// La meccanica e' dormiente per scelta (vedi `MakeRiktor`), non morta — e questo test misura il dato,
	// mentre l'effetto in partita e' pinnato da `Spec.Combat.BastionIgnoresAllPushes`, che verifica che
	// Riktor venga spostato come chiunque altro.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorPanelCreatesCoverTest,
	"RefactorTactics.Heroes.Riktor.PanelCreatesCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorPanelCreatesCoverTest::RunTest(const FString&)
{
	// **Aggiornato a CP 9.5 (2026-08-09)**: fino a qui questo test verificava un'abilita' a meta' — identita',
	// fase, portata — e diceva a chiare lettere «il giorno in cui E9 lo aggiungera', questo test diventera'
	// rosso, ed e' esattamente cio' che deve succedere». E' successo: il pannello ora erige una copertura.
	URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	const URTActionData* Panel = Riktor->Actions[1];

	TestEqual(TEXT("KineticPanel: ActionId"), Panel->Def.ActionId, FName(TEXT("Bastion.KineticPanel")));
	TestEqual(TEXT("KineticPanel: cooldown 2"), Panel->Def.CooldownTurns, 2);

	// I numeri vengono dal CORE, non da una seconda copia scritta a mano nel catalogo eroi: e' lo stesso riuso
	// di `Ram` su `Action.Charge`. Il test lo confronta con il core invece che con la costante 3, cosi' se
	// domani la portata cambia da una parte sola, cade.
	const FRTActionDef CoreCover = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover"));
	TestEqual(TEXT("KineticPanel: portata ereditata da Action.CreateCover"),
		Panel->Def.RangeCells, CoreCover.RangeCells);
	TestEqual(TEXT("e vale 3, come dichiara il catalogo azioni"), Panel->Def.RangeCells, 3);

	// Si costruisce nel Prep: una copertura che arrivasse dopo i colpi non proteggerebbe da nulla.
	TestTrue(TEXT("KineticPanel: si prepara prima del Blast"),
		URTCatalogLibrary::MapResolutionPhase(Panel->Def.ResolutionPhase) == ERTMatchPhase::Prep);
	TestTrue(TEXT("Reconfigure: anche"), URTCatalogLibrary::MapResolutionPhase(
		Riktor->Actions[2]->Def.ResolutionPhase) == ERTMatchPhase::Prep);

	// L'abilita' non e' piu' inerte, e la prova NON e' che abbia acquistato effetti: `Effects` resta vuoto, ed
	// e' corretto — il suo esito e' una modifica della MAPPA, che `FRTActionEffectSpec` non sa esprimere.
	// Quel che e' cambiato e' che dichiara la propria operazione come dato, e il resolver la legge.
	TestTrue(TEXT("KineticPanel: dichiara di erigere una copertura"),
		Panel->Def.StructureOp == ERTStructureOp::CreateCover);
	TestEqual(TEXT("KineticPanel: nessun effetto su unita', e non e' un limite"), Panel->Def.Effects.Num(), 0);

	// `Interposition` non e' piu' in questa lista (CP 6.7): era «nessun effetto perche' E5 non c'e'», ed e'
	// diventata «nessun effetto **proprio**, perche' interporsi non e' un effetto» — cambia CHI subisce un
	// colpo altrui, e quello lo fa la semantica di `Action.Intercept` che ora riusa. La verifica del suo
	// comportamento sta in `Heroes.BastionInterposition*` (`RTHeroReactionTests.cpp`), dove puo' essere
	// osservata in partita invece che contata a catalogo.
	TestTrue(TEXT("Interposition: cablata come reazione, non piu' inerte"),
		Riktor->Actions[4]->Def.Slot == ERTActionSlot::Reaction);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorVariantTradeoffTest,
	"RefactorTactics.Heroes.Riktor.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorVariantTradeoffTest::RunTest(const FString&)
{
	// Il compromesso di Riktor e' fatto di numeri che NON sono effetti (integrita', durata): vivono in
	// `FRTAbilityVariant::Parameters`, dichiarati finche' E9 non li consumera'.
	const URTActionData* Panel = URTHeroCatalogLibrary::MakeRiktor()->Actions[1];
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
