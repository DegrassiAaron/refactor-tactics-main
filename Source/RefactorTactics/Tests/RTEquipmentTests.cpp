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

#endif // WITH_DEV_AUTOMATION_TESTS
