#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 7.1 (`#60`) — le sei varianti d'arma del catalogo equipaggiamento §1.
 *
 * La regola sotto esame non e' «i numeri sono questi» ma «nessuna scelta e' gratis»: una variante che
 * migliorasse ogni parametro sarebbe potere verticale, che il canone esclude. Per questo il test itera su
 * TUTTE le varianti invece di controllarne una: un catalogo si rompe dalla riga che nessuno guarda.
 */
namespace
{
	/** Il primo effetto `Damage` di una def, o 0 se non ne dichiara. */
	int32 DirectDamage(const FRTActionDef& Def)
	{
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
		}
		return 0;
	}

	/** Vero se la def dichiara l'effetto richiesto. */
	bool HasEffect(const FRTActionDef& Def, ERTActionEffect Kind)
	{
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == Kind) { return true; }
		}
		return false;
	}

	/** L'attacco base di Vektor: `PulseShot`, 21 danni a portata 4 (catalogo eroi). */
	FRTActionDef VektorBasicAttack()
	{
		return URTHeroCatalogLibrary::MakeVektor()->Actions[0]->Def;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWeaponVariantTradeoffTest,
	"RefactorTactics.Equipment.WeaponVariantHasTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWeaponVariantTradeoffTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	if (!TestEqual(TEXT("sei varianti, come il catalogo §1"), Variants.Num(), 6)) { return false; }

	// Il validator e' l'oracolo: e' lo stesso che il catalogo usa, non una seconda regola scritta qui.
	TArray<const URTEquipmentData*> AsConst;
	for (const URTEquipmentData* V : Variants) { AsConst.Add(V); }
	const TArray<FString> Errors = URTCatalogLibrary::ValidateEquipment(AsConst);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("il catalogo delle varianti e' strutturalmente valido"), Errors.Num(), 0);

	const FRTActionDef Base = VektorBasicAttack();

	for (const URTEquipmentData* V : Variants)
	{
		const FString Id = V->EquipmentId.ToString();

		TestTrue(*FString::Printf(TEXT("%s: e' una variante d'arma"), *Id),
			V->Slot == ERTEquipmentSlot::WeaponVariant);
		TestFalse(*FString::Printf(TEXT("%s: svantaggio dichiarato a parole"), *Id), V->Drawback.IsEmpty());

		// Lo svantaggio MISURABILE: almeno un delta che peggiora. E' la stessa regola del validator, ripetuta
		// qui sul singolo elemento perche' il messaggio dica QUALE variante e' gratis, invece di «il catalogo
		// non e' valido».
		const bool bPays = V->DamageDelta < 0 || V->RangeDeltaCells < 0 || V->CooldownDeltaTurns > 0;
		TestTrue(*FString::Printf(TEXT("%s: paga qualcosa (danno %+d, portata %+d, ricarica %+d)"),
			*Id, V->DamageDelta, V->RangeDeltaCells, V->CooldownDeltaTurns), bPays);

		// E il costo dev'essere VERO sull'azione prodotta, non solo sul dato: e' la differenza fra dichiarare
		// un trade-off e applicarlo. Se `ApplyWeaponVariant` ignorasse i delta, il blocco sopra resterebbe
		// verde e questo cadrebbe.
		const FRTActionDef Modified = URTCatalogLibrary::ApplyWeaponVariant(Base, V);
		const bool bWorseSomewhere =
			DirectDamage(Modified) < DirectDamage(Base)
			|| Modified.RangeCells < Base.RangeCells
			|| Modified.CooldownTurns > Base.CooldownTurns;
		TestTrue(*FString::Printf(TEXT("%s: l'attacco modificato e' peggiore in almeno un parametro"), *Id),
			bWorseSomewhere);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrecisionRangeAndDamageTest,
	"RefactorTactics.Equipment.Precision.RangeAndDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrecisionRangeAndDamageTest::RunTest(const FString&)
{
	// Nome vincolante del DoD di CP 7.1. Qui i numeri contano: +1 portata / -4 danni (catalogo §1).
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Precision = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Precision"))) { Precision = V; break; }
	}
	if (!TestNotNull(TEXT("Weapon.Precision e' nel catalogo"), Precision)) { return false; }

	const FRTActionDef Base = VektorBasicAttack();
	const FRTActionDef Modified = URTCatalogLibrary::ApplyWeaponVariant(Base, Precision);

	// I delta si SOMMANO alla portata dell'arma, non la sostituiscono: la variante non sa quale attacco base
	// sta modificando, e su un eroe con portata diversa deve valere «+1» e non «4».
	TestEqual(TEXT("portata: +1 rispetto all'attacco base"), Modified.RangeCells, Base.RangeCells + 1);
	TestEqual(TEXT("danno: -4 rispetto all'attacco base"), DirectDamage(Modified), DirectDamage(Base) - 4);

	// Cio' che la variante NON tocca deve restare identico: un modificatore che cambiasse fase o priorita'
	// riscriverebbe l'azione invece di modificarla.
	TestEqual(TEXT("l'ActionId non cambia: e' sempre l'attacco base dell'eroe"), Modified.ActionId, Base.ActionId);
	TestTrue(TEXT("la fase di risoluzione non cambia"), Modified.ResolutionPhase == Base.ResolutionPhase);
	TestEqual(TEXT("la priorita' non cambia"), Modified.Priority, Base.Priority);
	TestEqual(TEXT("la ricarica non cambia"), Modified.CooldownTurns, Base.CooldownTurns);

	// La def prodotta dev'essere un'azione LEGALE: se una variante potesse generare portata negativa, il
	// catalogo la rifiuterebbe e la variante sarebbe ingiocabile invece che piu' corta.
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions({ Modified });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("l'attacco modificato resta un'azione valida"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantAddedEffectsTest,
	"RefactorTactics.Equipment.VariantAddsEffectsWithoutLosingTheOnesItHad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantAddedEffectsTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Impact = nullptr;
	const URTEquipmentData* Suppressive = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Impact"))) { Impact = V; }
		if (V->EquipmentId == FName(TEXT("Weapon.Suppressive"))) { Suppressive = V; }
	}
	if (!TestNotNull(TEXT("Weapon.Impact"), Impact) || !TestNotNull(TEXT("Weapon.Suppressive"), Suppressive))
	{
		return false;
	}

	// `Riva.PressureJet` e' l'attacco base che gia' fa TRE cose: danno, `Wet` e spinta. E' il caso che
	// distingue «aggiunge» da «sostituisce»: applicando una variante, cio' che l'arma faceva deve restare.
	const FRTActionDef Riva = URTHeroCatalogLibrary::MakeRiva()->Actions[0]->Def;
	const int32 EffectsBefore = Riva.Effects.Num();

	const FRTActionDef Suppressed = URTCatalogLibrary::ApplyWeaponVariant(Riva, Suppressive);
	TestEqual(TEXT("la Soppressione aggiunge un effetto, non ne sostituisce"),
		Suppressed.Effects.Num(), EffectsBefore + 1);
	TestTrue(TEXT("l'attacco di Riva continua a bagnare"), HasEffect(Suppressed, ERTActionEffect::Status));
	TestTrue(TEXT("e continua a spingere"), HasEffect(Suppressed, ERTActionEffect::Push));

	bool bHasSlow = false;
	for (const FRTActionEffectSpec& Spec : Suppressed.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Slow) { bHasSlow = true; }
	}
	TestTrue(TEXT("e ora rallenta"), bHasSlow);

	// L'Impatto su un'arma a portata 1 non deve produrre portata -1: si resta a contatto. Senza il clamp la
	// def uscirebbe illegale, e `ValidateActions` la rifiuterebbe.
	FRTActionDef Melee = Riva;
	Melee.RangeCells = 1;
	const FRTActionDef Pushed = URTCatalogLibrary::ApplyWeaponVariant(Melee, Impact);
	TestEqual(TEXT("portata 1 meno 1 resta 0, non -1"), Pushed.RangeCells, 0);
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions({ Pushed });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("e la def resta valida"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSplitHasNoConsumerTest,
	"RefactorTactics.Equipment.SplitHasNoConsumerYet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSplitHasNoConsumerTest::RunTest(const FString&)
{
	// ⚠️ Questo test **pinna un limite, non una funzionalita'**, ed e' scritto per diventare rosso.
	//
	// `Weapon.Split` compra «un bersaglio aggiuntivo» con -6 danni, ma il motore della v0.1 non ha alcuna
	// cardinalita' dei bersagli: `FRTActionDef` descrive forma e raggio. Applicare la variante quindi produce
	// **solo** il suo svantaggio, e chi la equipaggia oggi ha un'arma peggiore e basta.
	//
	// Senza questo test la meta' buona resterebbe per strada in silenzio. Il giorno in cui qualcuno cablera'
	// la cardinalita', questo cade e chiede di essere riscritto — che e' il momento giusto per accorgersene.
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Split = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Split"))) { Split = V; break; }
	}
	if (!TestNotNull(TEXT("Weapon.Split e' nel catalogo"), Split)) { return false; }

	TestEqual(TEXT("il dato dichiara il bersaglio in piu'"), Split->ExtraTargets, 1);

	const FRTActionDef Base = VektorBasicAttack();
	const FRTActionDef Modified = URTCatalogLibrary::ApplyWeaponVariant(Base, Split);
	TestEqual(TEXT("ma sull'azione prodotta si vede solo lo svantaggio"),
		DirectDamage(Modified), DirectDamage(Base) - 6);

	// La prova che la cardinalita' non e' cambiata e' che **non esiste un posto dove potrebbe cambiare**:
	// `FRTActionDef` dichiara portata e raggio di superficie, mai un numero di bersagli (la forma vive su
	// `FRTHexAttackIntent`, che la variante non tocca). Quindi si verifica che la variante non abbia aggiunto
	// nulla all'azione — nessun effetto, nessun raggio — e che il suo unico segno sia il danno in meno.
	TestEqual(TEXT("Split non aggiunge effetti: il bersaglio in piu' non e' esprimibile"),
		Split->AddedEffects.Num(), 0);
	TestEqual(TEXT("il numero di effetti dell'azione non cambia"), Modified.Effects.Num(), Base.Effects.Num());
	TestEqual(TEXT("il raggio di superficie resta quello dell'arma"),
		Modified.SurfaceRadius, Base.SurfaceRadius);
	return true;
}

// =====================================================================================================
// CP 7.3 metà catalogo (#62) — i moduli di reazione che l'infrastruttura E5 sa già far scattare.
//
// La proprietà sotto esame non è «esistono tre moduli» ma «sono reazioni VERE»: costruiti su un'azione core
// che è già una reazione, quindi visibili al pass che le valuta. Un modulo costruito su un'azione principale
// non fallirebbe — sarebbe inerte, che è peggio, perché il catalogo lo mostrerebbe come una scelta.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionModuleSingleActivationTest,
	"RefactorTactics.Equipment.ReactionModule.SingleActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionModuleSingleActivationTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Modules = URTCatalogLibrary::MakeReactionModules();
	if (!TestEqual(TEXT("tre moduli: gli altri quattro sono in #505"), Modules.Num(), 3)) { return false; }

	TArray<const URTEquipmentData*> AsConst;
	for (const URTEquipmentData* M : Modules) { AsConst.Add(M); }
	const TArray<FString> Errors = URTCatalogLibrary::ValidateEquipment(AsConst);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("il catalogo dei moduli e' strutturalmente valido"), Errors.Num(), 0);

	for (const URTEquipmentData* M : Modules)
	{
		const FString Id = M->EquipmentId.ToString();
		TestTrue(*FString::Printf(TEXT("%s: slot reazione"), *Id), M->Slot == ERTEquipmentSlot::ReactionModule);

		// «Una attivazione per turno» e' garantita dal resolver sul percorso E5, non da un cooldown
		// dell'oggetto: un cooldown qui sarebbe un SECONDO limite, non dichiarato dal catalogo §3, e i due
		// potrebbero divergere senza che nessuno se ne accorga.
		TestEqual(*FString::Printf(TEXT("%s: nessuna ricarica propria"), *Id), M->CooldownTurns, 0);

		// Il vincolo che rende il modulo una reazione vera.
		URTActionData* Action = URTCatalogLibrary::MakeEquipmentAction(M, nullptr);
		if (!TestNotNull(*FString::Printf(TEXT("%s: concede un'azione"), *Id), Action)) { continue; }

		TestTrue(*FString::Printf(TEXT("%s: l'azione concessa occupa lo slot Reaction"), *Id),
			Action->Def.Slot == ERTActionSlot::Reaction);
		TestTrue(*FString::Printf(TEXT("%s: e dichiara un trigger, altrimenti non scatterebbe mai"), *Id),
			Action->Def.ReactionTrigger != ERTReactionTrigger::None);
		TestEqual(*FString::Printf(TEXT("%s: nel TurnLog si legge il modulo"), *Id),
			Action->Def.ActionId, M->EquipmentId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCounterShotUsesExistingTriggerTest,
	"RefactorTactics.Equipment.CounterShotUsesExistingTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCounterShotUsesExistingTriggerTest::RunTest(const FString&)
{
	// Nome vincolante del DoD: il modulo non deve introdurre un SECONDO motore di reazioni. La prova è che
	// fase, priorità e trigger vengano dall'azione core — confrontati col core, non con costanti riscritte
	// qui: se domani `Action.Counter` cambia fase, questo test cade invece di restare verde su un numero morto.
	const TArray<URTEquipmentData*> Modules = URTCatalogLibrary::MakeReactionModules();
	const URTEquipmentData* CounterShot = nullptr;
	const URTEquipmentData* Shield = nullptr;
	for (const URTEquipmentData* M : Modules)
	{
		if (M->EquipmentId == FName(TEXT("Reaction.CounterShot"))) { CounterShot = M; }
		if (M->EquipmentId == FName(TEXT("Reaction.ReactiveShield"))) { Shield = M; }
	}
	if (!TestNotNull(TEXT("Reaction.CounterShot"), CounterShot)) { return false; }
	if (!TestNotNull(TEXT("Reaction.ReactiveShield"), Shield)) { return false; }

	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter"));
	URTActionData* Action = URTCatalogLibrary::MakeEquipmentAction(CounterShot, nullptr);
	if (!TestNotNull(TEXT("l'azione concessa"), Action)) { return false; }

	TestTrue(TEXT("stessa fase del contrattacco core"), Action->Def.ResolutionPhase == Core.ResolutionPhase);
	TestEqual(TEXT("stessa priorita'"), Action->Def.Priority, Core.Priority);
	TestTrue(TEXT("stesso trigger, cioe' lo stesso pass di valutazione"),
		Action->Def.ReactionTrigger == Core.ReactionTrigger);
	TestTrue(TEXT("e quel trigger e' HitByDirectAttack"),
		Action->Def.ReactionTrigger == ERTReactionTrigger::HitByDirectAttack);

	// I numeri invece sono del MODULO: 14 contro i 16 del core. È ciò che `GrantedEffects` esiste per fare,
	// e senza il quale l'unico modo di avere un valore diverso sarebbe una seconda azione nel catalogo.
	TestEqual(TEXT("14 danni, non i 16 dell'azione core"), DirectDamage(Action->Def), 14);
	TestNotEqual(TEXT("e infatti divergono, come dichiara il catalogo §3"),
		DirectDamage(Action->Def), DirectDamage(Core));

	// Due moduli sullo STESSO trigger con mestieri opposti: è la prova che il regime dipende dai dati e non
	// dall'azione (ADR-0004 §2). Se il contrattacco fosse cablato all'ActionId, questo non sarebbe possibile.
	URTActionData* ShieldAction = URTCatalogLibrary::MakeEquipmentAction(Shield, nullptr);
	if (!TestNotNull(TEXT("l'azione dello scudo reattivo"), ShieldAction)) { return false; }
	TestTrue(TEXT("lo scudo reattivo condivide il trigger del contrattacco"),
		ShieldAction->Def.ReactionTrigger == Action->Def.ReactionTrigger);
	TestEqual(TEXT("ma non fa danno"), DirectDamage(ShieldAction->Def), 0);
	bool bShields = false;
	for (const FRTActionEffectSpec& Spec : ShieldAction->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Shield && Spec.Amount == 15) { bShields = true; }
	}
	TestTrue(TEXT("para per 15"), bShields);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEmergencyDashNotExpressibleTest,
	"RefactorTactics.Equipment.EmergencyDashIsNotExpressibleYet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEmergencyDashNotExpressibleTest::RunTest(const FString&)
{
	// ⚠️ Secondo test che **pinna un limite**, come `SplitHasNoConsumerYet`.
	//
	// `Reaction.EmergencyDash` ha un trigger che esiste (`sei bersagliato` → `HitByDirectAttack`), quindi la
	// divisione fra #62 e #505 — fatta sul trigger — lo metteva in questa metà. Ma il vincolo vero è un
	// altro: il suo effetto è `Reposition 1`, cioè «chi reagisce si sposta», e nessun `ERTActionEffect` lo
	// esprime. `Push` e `Pull` spostano il BERSAGLIO, mai la sorgente.
	//
	// Il test verifica che il modulo NON sia nel catalogo, così nessuno lo aggiunge con un effetto vuoto
	// convinto che basti il trigger. Diventerà rosso il giorno in cui esisterà un effetto di auto-spostamento,
	// ed è il momento giusto per costruirlo.
	const TArray<URTEquipmentData*> Modules = URTCatalogLibrary::MakeReactionModules();
	for (const URTEquipmentData* M : Modules)
	{
		TestTrue(TEXT("EmergencyDash non e' nel catalogo dei moduli costruiti"),
			M->EquipmentId != FName(TEXT("Reaction.EmergencyDash")));
	}

	// E la ragione, verificata invece che raccontata: nessuna azione core sposta chi la usa. `Action.Reposition`
	// esiste ma non è una reazione — è in FastMovement — quindi non può nemmeno fare da base al modulo.
	const FRTActionDef Reposition = URTCatalogLibrary::FindCoreAction(TEXT("Action.Reposition"));
	TestFalse(TEXT("Action.Reposition esiste"), Reposition.ActionId.IsNone());
	TestTrue(TEXT("ma non e' una reazione: non ha trigger"),
		Reposition.ReactionTrigger == ERTReactionTrigger::None);
	TestTrue(TEXT("e non occupa lo slot Reaction"), Reposition.Slot != ERTActionSlot::Reaction);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
