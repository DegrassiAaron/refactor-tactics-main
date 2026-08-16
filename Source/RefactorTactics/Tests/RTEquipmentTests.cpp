#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

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

	/** L'attacco base di Wraith: `PulseShot`, 21 danni a portata 4 (catalogo eroi). */
	FRTActionDef WraithBasicAttack()
	{
		return URTHeroCatalogLibrary::MakeWraith()->Actions[0]->Def;
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

	const FRTActionDef Base = WraithBasicAttack();

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

	const FRTActionDef Base = WraithBasicAttack();
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

	// `Phase.PressureJet` e' l'attacco base che gia' fa TRE cose: danno, `Wet` e spinta. E' il caso che
	// distingue «aggiunge» da «sostituisce»: applicando una variante, cio' che l'arma faceva deve restare.
	const FRTActionDef Phase = URTHeroCatalogLibrary::MakePhase()->Actions[0]->Def;
	const int32 EffectsBefore = Phase.Effects.Num();

	const FRTActionDef Suppressed = URTCatalogLibrary::ApplyWeaponVariant(Phase, Suppressive);
	TestEqual(TEXT("la Soppressione aggiunge un effetto, non ne sostituisce"),
		Suppressed.Effects.Num(), EffectsBefore + 1);
	TestTrue(TEXT("l'attacco di Phase continua a bagnare"), HasEffect(Suppressed, ERTActionEffect::Status));
	TestTrue(TEXT("e continua a spingere"), HasEffect(Suppressed, ERTActionEffect::Push));

	bool bHasSlow = false;
	for (const FRTActionEffectSpec& Spec : Suppressed.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Slow) { bHasSlow = true; }
	}
	TestTrue(TEXT("e ora rallenta"), bHasSlow);

	// L'Impatto su un'arma a portata 1 non deve produrre portata -1: si resta a contatto. Senza il clamp la
	// def uscirebbe illegale, e `ValidateActions` la rifiuterebbe.
	FRTActionDef Melee = Phase;
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

	const FRTActionDef Base = WraithBasicAttack();
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
	// ⚠️ Il numero e' salito da 4 a 5 con `Reaction.Anchor` (CP 7.5, `#505`), e sale ancora quando arrivano gli
	// altri due: e' un limite che si asserisce, non una costante da inseguire. I due assenti mancano per la
	// stessa ragione — un punto di valutazione ancora da aprire (`URTReactionLibrary::PassPointFor`) — e non
	// per un dato mancante: `HazardEscape` vuole il Cleanup fra le superfici e il danno di `Burning`,
	// `Cleanse` il momento in cui uno stato di controllo e' stato appena applicato.
	if (!TestEqual(TEXT("i sette moduli del catalogo §3: nessuno resta fuori"), Modules.Num(), 7))
	{
		return false;
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEmergencyDashExistsTest,
	"RefactorTactics.Equipment.EmergencyDashRepositionsTheReactor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEmergencyDashExistsTest::RunTest(const FString&)
{
	// ⚠️ Questo test **era il suo contrario** fino a D-093: si chiamava `EmergencyDashIsNotExpressibleYet` e
	// pinnava l'ASSENZA del modulo, perché nessun `ERTActionEffect` sapeva spostare la sorgente. Era scritto
	// per diventare rosso quando il limite fosse caduto, ed è caduto: `SelfReposition` esiste, e il test
	// cambia di segno invece di essere cancellato — così la storia resta leggibile.
	const TArray<URTEquipmentData*> Modules = URTCatalogLibrary::MakeReactionModules();
	const URTEquipmentData* Dash = nullptr;
	for (const URTEquipmentData* M : Modules)
	{
		if (M->EquipmentId == FName(TEXT("Reaction.EmergencyDash"))) { Dash = M; break; }
	}
	if (!TestNotNull(TEXT("Reaction.EmergencyDash e' nel catalogo"), Dash)) { return false; }

	// L'effetto è `SelfReposition`, non `Push`: la differenza è CHI si muove, ed è tutta la ragione per cui
	// il modulo non esisteva.
	bool bSelfMoves = false;
	for (const FRTActionEffectSpec& Spec : Dash->GrantedEffects)
	{
		if (Spec.Effect == ERTActionEffect::SelfReposition && Spec.Amount == 1) { bSelfMoves = true; }
		TestTrue(TEXT("non usa Push: quello sposterebbe il BERSAGLIO"), Spec.Effect != ERTActionEffect::Push);
	}
	TestTrue(TEXT("sposta di 1 chi reagisce"), bSelfMoves);

	// E resta una reazione vera, con il trigger ereditato come gli altri tre moduli.
	URTActionData* Action = URTCatalogLibrary::MakeEquipmentAction(Dash, nullptr);
	if (!TestNotNull(TEXT("concede un'azione"), Action)) { return false; }
	TestTrue(TEXT("slot Reaction"), Action->Def.Slot == ERTActionSlot::Reaction);
	TestTrue(TEXT("scatta quando sei bersagliato"),
		Action->Def.ReactionTrigger == ERTReactionTrigger::HitByDirectAttack);
	return true;
}

// =====================================================================================================
// D-085 — le spinte additive della stessa azione si SOMMANO.
//
// `Phase.PressureJet` spinge già di 1; `Weapon.Impact` aggiunge `Push 1`. Il catalogo dice che il
// risultato è **una spinta di 2**, e non due spinte da 1: sono cose osservabilmente diverse — una spinta
// di 2 attraversa la cella intermedia, due da 1 la valutano due volte.
//
// Il test esiste perché il difetto era reale e l'aveva introdotto CP 7.1: `AddedEffects` accoda un secondo
// `FRTActionEffectSpec(Push, 1)`, `ProduceEvents` ne fa **due eventi**, e `ResolveCombat` conta gli EVENTI
// invece degli ATTACCANTI. Con due eventi il bersaglio finiva nel ramo «forze contraddittorie» (#420) e
// **non si muoveva affatto**, mentre il TurnLog dichiarava `OpposingForces` — con un attaccante solo in
// campo. Una causa scritta, precisa e falsa: esattamente il difetto che `PushCause`/`PullCause` esistono
// per evitare.
// =====================================================================================================
namespace
{
	UWorld* MakeEquipWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyEquipWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnEquipMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnEquipUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, URTHeroData* Hero)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	void RunEquipTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	/**
	 * Sostituisce l'attacco base dell'unità con la sua versione modificata dalla variante.
	 *
	 * ⚠️ Chiama la funzione di **produzione**, e non riallinea i campi specchio per conto suo: fino al
	 * 2026-08-11 lo faceva, ed era l'unico posto in tutto il progetto dove quel ponte esisteva. Un helper di
	 * test che fa la cosa giusta al posto della produzione verifica un percorso che la partita non
	 * attraverserà, e nessun test diventa rosso quando manca.
	 */
	void EquipVariantOnBasicAttack(ARTUnit* Unit, const URTEquipmentData* Variant)
	{
		if (!Unit || Unit->Abilities.Num() == 0) { return; }
		URTCatalogLibrary::EquipWeaponVariant(Unit->Abilities[0], Variant);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTImpactVariantStacksPushTest,
	"RefactorTactics.Equipment.ImpactVariantStacksPush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTImpactVariantStacksPushTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Impact = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Impact"))) { Impact = V; break; }
	}
	if (!TestNotNull(TEXT("Weapon.Impact"), Impact)) { return false; }

	UWorld* World = MakeEquipWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEquipMap(World, 8);

	// Phase spinge da (0,0) verso (1,0): il bersaglio deve finire a (3,0), due celle più in là.
	ARTUnit* Phase = SpawnEquipUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakePhase());
	ARTUnit* Bersaglio = SpawnEquipUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeWraith());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Phase"), Phase) || !TestNotNull(TEXT("bersaglio"), Bersaglio) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEquipWorld(World);
		return false;
	}

	EquipVariantOnBasicAttack(Phase, Impact);

	// Due spec `Push` nella stessa azione: è la configurazione che produceva il difetto.
	int32 PushSpecs = 0;
	for (const FRTActionEffectSpec& Spec : Phase->Abilities[0]->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Push) { ++PushSpecs; }
	}
	TestEqual(TEXT("l'attacco modificato dichiara due spinte separate"), PushSpecs, 2);

	Phase->PlannedAbilityIndex = 0;
	Phase->PlannedAttackTarget = Bersaglio;
	Phase->PlannedCell = Phase->Cell;
	Bersaglio->PlannedAbilityIndex = INDEX_NONE;
	Bersaglio->PlannedCell = Bersaglio->Cell;

	RunEquipTurn(TM);

	// Il cuore: **una** spinta di 2, non due da 1 e non zero.
	TestEqual(TEXT("il bersaglio arretra di DUE celle (D-085)"), Bersaglio->Cell, FRTCellId(3, 0));

	// E il controllo che smaschera il difetto vero: se il resolver contasse gli eventi invece degli
	// attaccanti, il bersaglio resterebbe a (1,0) — fermo — con `OpposingForces` nel log. Fermo e spinto di
	// una sola cella sono entrambi sbagliati, e vanno esclusi separatamente perché hanno cause diverse.
	TestTrue(TEXT("non e' rimasto fermo per «forze contraddittorie»: l'attaccante e' uno solo"),
		Bersaglio->Cell != FRTCellId(1, 0));
	TestTrue(TEXT("e non e' arretrato di una cella sola"), Bersaglio->Cell != FRTCellId(2, 0));

	DestroyEquipWorld(World);
	return true;
}

// =====================================================================================================
// CP 7.2 (#61) — i gadget che il motore sa già far funzionare.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGadgetCooldownEnforcedTest,
	"RefactorTactics.Equipment.Gadget.CooldownEnforced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGadgetCooldownEnforcedTest::RunTest(const FString&)
{
	// Nome vincolante del DoD. La regola del catalogo §2 è dichiarata **una volta sopra la tabella** — «tutti
	// i gadget hanno cooldown 3» — quindi il test itera su tutti invece di controllarne uno: è una proprietà
	// dell'insieme, e un gadget nuovo che se ne dimenticasse deve cadere qui.
	const TArray<URTEquipmentData*> Gadgets = URTCatalogLibrary::MakeGadgets();
	if (!TestEqual(TEXT("quattro gadget: gli altri quattro non sono esprimibili"), Gadgets.Num(), 4))
	{
		return false;
	}

	TArray<const URTEquipmentData*> AsConst;
	for (const URTEquipmentData* G : Gadgets) { AsConst.Add(G); }
	const TArray<FString> Errors = URTCatalogLibrary::ValidateEquipment(AsConst);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("il catalogo dei gadget e' strutturalmente valido"), Errors.Num(), 0);

	for (const URTEquipmentData* G : Gadgets)
	{
		const FString Id = G->EquipmentId.ToString();
		TestTrue(*FString::Printf(TEXT("%s: slot gadget"), *Id), G->Slot == ERTEquipmentSlot::Gadget);
		TestEqual(*FString::Printf(TEXT("%s: ricarica 3 turni"), *Id), G->CooldownTurns, 3);

		// Il cooldown dev'essere quello del GADGET anche sull'azione concessa: l'azione core ne ha uno suo
		// (`Action.Heal` 1, `Action.CreateWater` 2) e senza la sostituzione il gadget si ricaricherebbe coi
		// tempi di un'altra cosa.
		URTActionData* Action = URTCatalogLibrary::MakeEquipmentAction(G, nullptr);
		if (!TestNotNull(*FString::Printf(TEXT("%s: concede un'azione"), *Id), Action)) { continue; }
		TestEqual(*FString::Printf(TEXT("%s: l'azione concessa eredita la ricarica del gadget"), *Id),
			Action->Def.CooldownTurns, 3);
		TestEqual(*FString::Printf(TEXT("%s: nel TurnLog si legge il gadget"), *Id),
			Action->Def.ActionId, G->EquipmentId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGadgetNumbersMatchCatalogTest,
	"RefactorTactics.Equipment.Gadget.NumbersMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGadgetNumbersMatchCatalogTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Gadgets = URTCatalogLibrary::MakeGadgets();
	auto Trova = [&Gadgets](const TCHAR* Id) -> const URTEquipmentData*
	{
		for (const URTEquipmentData* G : Gadgets)
		{
			if (G->EquipmentId == FName(Id)) { return G; }
		}
		return nullptr;
	};

	// Medkit: cura 18 (catalogo §2), non i 20 dell'azione core. Il confronto è **contro il core**, così se
	// domani `Action.Heal` cambia i due numeri non si allineano in silenzio.
	const URTEquipmentData* Medkit = Trova(TEXT("Gadget.Medkit"));
	if (!TestNotNull(TEXT("Gadget.Medkit"), Medkit)) { return false; }
	URTActionData* Cura = URTCatalogLibrary::MakeEquipmentAction(Medkit, nullptr);
	if (!TestNotNull(TEXT("l'azione del medkit"), Cura)) { return false; }
	int32 Heal = 0;
	for (const FRTActionEffectSpec& S : Cura->Def.Effects)
	{
		if (S.Effect == ERTActionEffect::Heal) { Heal = S.Amount; }
	}
	TestEqual(TEXT("Medkit: cura 18"), Heal, 18);
	const FRTActionDef CoreHeal = URTCatalogLibrary::FindCoreAction(TEXT("Action.Heal"));
	TestNotEqual(TEXT("e diverge dai 20 del core, come dichiara il catalogo"),
		Heal, [&CoreHeal]{ for (const FRTActionEffectSpec& S : CoreHeal.Effects)
			{ if (S.Effect == ERTActionEffect::Heal) { return S.Amount; } } return 0; }());

	// BreachCharge: 35 alla STRUTTURA e **zero** alle unità. È il punto in cui `GrantedEffects` sostituisce
	// invece di aggiungere: `Action.HeavyAttack` fa 35 a un'unità e 20 a una struttura, e una carica da
	// sfondamento che ferisse le persone sarebbe un'altra cosa.
	const URTEquipmentData* Breach = Trova(TEXT("Gadget.BreachCharge"));
	if (!TestNotNull(TEXT("Gadget.BreachCharge"), Breach)) { return false; }
	URTActionData* Carica = URTCatalogLibrary::MakeEquipmentAction(Breach, nullptr);
	if (!TestNotNull(TEXT("l'azione della carica"), Carica)) { return false; }
	int32 Struttura = 0, Unita = 0;
	for (const FRTActionEffectSpec& S : Carica->Def.Effects)
	{
		if (S.Effect == ERTActionEffect::DamageStructure) { Struttura = S.Amount; }
		if (S.Effect == ERTActionEffect::Damage) { Unita = S.Amount; }
	}
	TestEqual(TEXT("BreachCharge: 35 alla struttura"), Struttura, 35);
	TestEqual(TEXT("BreachCharge: nessun danno alle unita'"), Unita, 0);

	// Sprinkler: il suo esito è una SUPERFICIE, non un effetto. Il raggio 1 del catalogo equipaggiamento è
	// già quello di `Action.CreateWater`, e il gadget lo eredita invece di riscriverlo.
	const URTEquipmentData* Sprinkler = Trova(TEXT("Gadget.Sprinkler"));
	if (!TestNotNull(TEXT("Gadget.Sprinkler"), Sprinkler)) { return false; }
	URTActionData* Acqua = URTCatalogLibrary::MakeEquipmentAction(Sprinkler, nullptr);
	if (!TestNotNull(TEXT("l'azione dello sprinkler"), Acqua)) { return false; }
	TestTrue(TEXT("Sprinkler: crea una superficie"), Acqua->Def.bCreatesSurface);
	TestTrue(TEXT("e la superficie e' acqua bassa"), Acqua->Def.SurfaceCreated == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("raggio 1, ereditato dal core"), Acqua->Def.SurfaceRadius, 1);
	TestEqual(TEXT("nessun effetto proprio: una superficie non e' un FRTActionEffectSpec"),
		Sprinkler->GrantedEffects.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGadgetsWithoutEngineSupportTest,
	"RefactorTactics.Equipment.Gadget.FourAreNotExpressibleYet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGadgetsWithoutEngineSupportTest::RunTest(const FString&)
{
	// ⚠️ Test che **pinna limiti**, come `SplitHasNoConsumerYet`. (Il terzo era `EmergencyDashIsNotExpressibleYet`,
	// diventato `EmergencyDashRepositionsTheReactor` quando D-093 ha fatto cadere il suo limite.)
	// Quattro gadget del catalogo §2 non sono costruiti, per quattro ragioni diverse — e la differenza conta,
	// perché porta a lavori diversi. Il test verifica che non compaiano, e per due di essi verifica **anche
	// la ragione**, così non basta aggiungerli per farlo tornare verde.
	const TArray<URTEquipmentData*> Gadgets = URTCatalogLibrary::MakeGadgets();
	const TCHAR* Assenti[] = { TEXT("Gadget.SmokeEmitter"), TEXT("Gadget.Insulator"),
		TEXT("Gadget.Sensor"), TEXT("Gadget.Anchor") };
	for (const TCHAR* Id : Assenti)
	{
		bool bPresente = false;
		for (const URTEquipmentData* G : Gadgets)
		{
			if (G->EquipmentId == FName(Id)) { bPresente = true; }
		}
		TestFalse(*FString::Printf(TEXT("%s non e' costruito"), Id), bPresente);
	}

	// `SmokeEmitter`: nessuna azione CORE crea fumo. Se un giorno esistesse, questo cade e chiede il gadget.
	bool bSmokeCore = false;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (Def.bCreatesSurface && Def.SurfaceCreated == ERTHexSurface::Smoke) { bSmokeCore = true; }
	}
	TestFalse(TEXT("nessuna azione core crea fumo: e' per questo che SmokeEmitter non esiste"), bSmokeCore);

	// `Anchor`: `PushResistance` è una soglia permanente, non un contatore per turno. Il roster è a zero dopo
	// D-075, e un gadget che la alzasse renderebbe immune a OGNI spinta — tutte valgono 1.
	TestEqual(TEXT("il roster non ha resistenza nativa (D-075), e il gadget non puo' introdurla come soglia"),
		URTHeroCatalogLibrary::MakeRiktor()->PushResistance, 0);
	return true;
}

// =====================================================================================================
// D-089 (`WV-3`) — i default di variante per eroe, e l'avviso sulle coppie che non hanno senso.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefaultWeaponVariantsTest,
	"RefactorTactics.Equipment.DefaultVariantPerHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefaultWeaponVariantsTest::RunTest(const FString&)
{
	TestEqual(TEXT("Gadget: Precisione"), URTCatalogLibrary::DefaultWeaponVariantFor(TEXT("Hero.Gadget")),
		FName(TEXT("Weapon.Precision")));
	TestEqual(TEXT("Phase: Impatto"), URTCatalogLibrary::DefaultWeaponVariantFor(TEXT("Hero.Phase")),
		FName(TEXT("Weapon.Impact")));
	TestEqual(TEXT("Wraith: Soppressione"), URTCatalogLibrary::DefaultWeaponVariantFor(TEXT("Hero.Wraith")),
		FName(TEXT("Weapon.Suppressive")));
	TestEqual(TEXT("Riktor: Impatto"), URTCatalogLibrary::DefaultWeaponVariantFor(TEXT("Hero.Riktor")),
		FName(TEXT("Weapon.Impact")));

	// Un eroe sconosciuto NON ricade su un default: meglio nessuna variante che una sbagliata in silenzio.
	TestTrue(TEXT("un eroe senza riga ottiene None"),
		URTCatalogLibrary::DefaultWeaponVariantFor(TEXT("Hero.NonEsiste")).IsNone());

	// Nessun default usa `Overcharge` finché il suo costo è `WV-1` (#510): un default il cui prezzo si
	// decide dopo cambierebbe insieme a quella risposta. Il test lo pinna, così la scelta resta consapevole.
	const TCHAR* Roster[] = { TEXT("Hero.Gadget"), TEXT("Hero.Phase"), TEXT("Hero.Wraith"), TEXT("Hero.Riktor") };
	for (const TCHAR* H : Roster)
	{
		TestTrue(*FString::Printf(TEXT("%s non ha Overcharge come default"), H),
			URTCatalogLibrary::DefaultWeaponVariantFor(H) != FName(TEXT("Weapon.Overcharge")));
	}

	// E ogni default dev'essere una variante che esiste davvero nel catalogo.
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	for (const TCHAR* H : Roster)
	{
		const FName Def = URTCatalogLibrary::DefaultWeaponVariantFor(H);
		bool bEsiste = false;
		for (const URTEquipmentData* V : Variants) { if (V->EquipmentId == Def) { bEsiste = true; } }
		TestTrue(*FString::Printf(TEXT("%s: il default '%s' e' nel catalogo"), H, *Def.ToString()), bEsiste);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantWarningsTest,
	"RefactorTactics.Equipment.WarnsOnPointlessVariantPairing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantWarningsTest::RunTest(const FString&)
{
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	auto Trova = [&Variants](const TCHAR* Id) -> const URTEquipmentData*
	{
		for (const URTEquipmentData* V : Variants) { if (V->EquipmentId == FName(Id)) { return V; } }
		return nullptr;
	};

	const FRTActionDef Riktor = URTHeroCatalogLibrary::MakeRiktor()->Actions[0]->Def;
	const FRTActionDef Wraith = WraithBasicAttack();

	// Il caso concreto che ha fatto nascere la regola: `ImpactShot` rallenta già, quindi `Suppressive` fa
	// pagare 5 danni su 8 per un effetto che l'eroe possiede — legale e privo di senso (D-086).
	const URTEquipmentData* Suppressive = Trova(TEXT("Weapon.Suppressive"));
	if (!TestNotNull(TEXT("Weapon.Suppressive"), Suppressive)) { return false; }
	const TArray<FString> SuRiktor = URTCatalogLibrary::WarnOnVariantForAttack(Riktor, Suppressive);
	TestTrue(TEXT("Riktor + Soppressione: avvisato"), SuRiktor.Num() > 0);
	bool bNominaLoStatus = false;
	for (const FString& W : SuRiktor) { bNominaLoStatus |= W.Contains(TEXT("Slow")); }
	TestTrue(TEXT("e l'avviso dice QUALE status e' duplicato"), bNominaLoStatus);

	// Il gemello di controllo: lo stesso identico dato su un attacco che NON rallenta non avvisa. Senza
	// questo, un avviso che scattasse sempre passerebbe il test sopra e non direbbe niente.
	const TArray<FString> SuWraith = URTCatalogLibrary::WarnOnVariantForAttack(Wraith, Suppressive);
	TestEqual(TEXT("Wraith + Soppressione: nessun avviso, il suo attacco non rallenta"), SuWraith.Num(), 0);

	// I default scelti da D-089 devono essere puliti: se un default producesse un avviso, la decisione
	// sarebbe da rivedere — ed è esattamente il momento in cui vogliamo saperlo.
	URTHeroData* Eroi[] = { URTHeroCatalogLibrary::MakeGadget(), URTHeroCatalogLibrary::MakePhase(),
		URTHeroCatalogLibrary::MakeWraith(), URTHeroCatalogLibrary::MakeRiktor() };
	for (URTHeroData* Eroe : Eroi)
	{
		const FName DefId = URTCatalogLibrary::DefaultWeaponVariantFor(Eroe->HeroId);
		const URTEquipmentData* Def = Trova(*DefId.ToString());
		if (!Def) { continue; }
		const TArray<FString> W = URTCatalogLibrary::WarnOnVariantForAttack(Eroe->Actions[0]->Def, Def);
		for (const FString& Msg : W) { AddError(Msg); }
		TestEqual(*FString::Printf(TEXT("%s: il default non produce avvisi"), *Eroe->HeroId.ToString()),
			W.Num(), 0);
	}
	return true;
}

// =====================================================================================================
// CP 7.4 metà regola (#63) — 1+1+1, e nessuna progressione in partita.
//
// ⚠️ La metà **default** non è qui, e non è una dimenticanza: dei quattro loadout consigliati dal catalogo
// §4 solo quello di Riktor è interamente costruibile — a Gadget manca `Gadget.Insulator` (E36), a Wraith
// `Gadget.Sensor` (E13) e `Reaction.EmergencyDash` (#505), a Phase `Reaction.HazardEscape` (#505). Caricarli
// oggi significherebbe scrivere default con dei buchi.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLoadoutExactlyOneEachTest,
	"RefactorTactics.Equipment.LoadoutExactlyOneEach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLoadoutExactlyOneEachTest::RunTest(const FString&)
{
	// Nome vincolante del DoD. Il loadout legale si costruisce da ciò che esiste davvero nel catalogo, non
	// da oggetti inventati nel test: se domani un pezzo sparisse, questo test lo direbbe.
	const URTEquipmentData* Variante = URTCatalogLibrary::MakeWeaponVariants()[0];
	const URTEquipmentData* Gadget = URTCatalogLibrary::MakeGadgets()[0];
	const URTEquipmentData* Modulo = URTCatalogLibrary::MakeReactionModules()[0];

	const TArray<const URTEquipmentData*> Legale = { Variante, Gadget, Modulo };
	const TArray<FString> Ok = URTCatalogLibrary::ValidateLoadout(Legale);
	for (const FString& E : Ok) { AddError(E); }
	TestEqual(TEXT("1+1+1 e' accettato"), Ok.Num(), 0);

	// ZERO e DUE sono errori diversi, e il messaggio deve distinguerli: portano a correzioni opposte.
	{
		const TArray<const URTEquipmentData*> SenzaGadget = { Variante, Modulo };
		const TArray<FString> E = URTCatalogLibrary::ValidateLoadout(SenzaGadget);
		TestTrue(TEXT("senza gadget: rifiutato"), E.Num() > 0);
		bool bDiceCosaManca = false;
		for (const FString& M : E) { bDiceCosaManca |= M.Contains(TEXT("manca")) && M.Contains(TEXT("gadget")); }
		TestTrue(TEXT("e l'errore dice che MANCA il gadget"), bDiceCosaManca);
	}
	{
		const TArray<const URTEquipmentData*> DueGadget = { Variante, Gadget,
			URTCatalogLibrary::MakeGadgets()[1], Modulo };
		const TArray<FString> E = URTCatalogLibrary::ValidateLoadout(DueGadget);
		TestTrue(TEXT("con due gadget: rifiutato"), E.Num() > 0);
		bool bDiceQuanti = false;
		for (const FString& M : E) { bDiceQuanti |= M.Contains(TEXT("2")) && M.Contains(TEXT("gadget")); }
		TestTrue(TEXT("e l'errore dice QUANTI ce ne sono"), bDiceQuanti);
	}
	{
		// Un loadout vuoto sbaglia in tre modi, e li deve dire tutti e tre: chi lo corregge un errore per
		// volta farebbe tre giri.
		const TArray<FString> E = URTCatalogLibrary::ValidateLoadout({});
		TestEqual(TEXT("loadout vuoto: tre errori, uno per slot"), E.Num(), 3);
	}
	{
		// Forma giusta, contenuto rotto: un pezzo senza svantaggio non è un loadout valido.
		URTEquipmentData* Rotto = NewObject<URTEquipmentData>();
		Rotto->EquipmentId = TEXT("Weapon.Test");
		Rotto->Slot = ERTEquipmentSlot::WeaponVariant; // nessun Drawback, nessun delta
		const TArray<const URTEquipmentData*> FormaOk = { Rotto, Gadget, Modulo };
		TestTrue(TEXT("1+1+1 con un pezzo invalido: rifiutato"),
			URTCatalogLibrary::ValidateLoadout(FormaOk).Num() > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoInMatchProgressionTest,
	"RefactorTactics.Equipment.NoInMatchProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoInMatchProgressionTest::RunTest(const FString&)
{
	// Nome vincolante del DoD: «nessun livello, rarità, upgrade numerico o equipaggiamento casuale; nessuna
	// progressione durante la partita».
	//
	// Verificarlo su un'istanza direbbe poco — un campo a zero sembra assente. Si verifica sul **tipo**, con
	// la reflection: se qualcuno aggiungesse `Level`, `Rarity` o `Tier` a `URTEquipmentData`, questo test
	// diventerebbe rosso il giorno stesso, che è l'unico momento in cui la discussione è ancora economica.
	const TCHAR* Vietati[] = { TEXT("Level"), TEXT("Rarity"), TEXT("Tier"), TEXT("Upgrade"),
		TEXT("Experience"), TEXT("XP"), TEXT("Rank"), TEXT("Quality") };

	for (TFieldIterator<FProperty> It(URTEquipmentData::StaticClass()); It; ++It)
	{
		const FString Nome = It->GetName();
		for (const TCHAR* Vietato : Vietati)
		{
			TestFalse(*FString::Printf(
				TEXT("URTEquipmentData non deve avere un campo di progressione: trovato '%s'"), *Nome),
				Nome.Contains(Vietato));
		}
	}

	// E il catalogo spedito non deve contenere due volte lo stesso oggetto con numeri diversi, che è il modo
	// in cui la progressione entra senza un campo che la nomini — «Medkit» e «Medkit+».
	TArray<const URTEquipmentData*> Tutto;
	for (const URTEquipmentData* V : URTCatalogLibrary::MakeWeaponVariants()) { Tutto.Add(V); }
	for (const URTEquipmentData* G : URTCatalogLibrary::MakeGadgets()) { Tutto.Add(G); }
	for (const URTEquipmentData* M : URTCatalogLibrary::MakeReactionModules()) { Tutto.Add(M); }

	const TArray<FString> Errors = URTCatalogLibrary::ValidateEquipment(Tutto);
	for (const FString& E : Errors) { AddError(E); }
	TestEqual(TEXT("il catalogo intero non ha id duplicati ne' pezzi invalidi"), Errors.Num(), 0);

	// Nessun DisplayName è prefisso di un altro con un suffisso di potenziamento.
	for (const URTEquipmentData* A : Tutto)
	{
		const FString Nome = A->EquipmentId.ToString();
		TestFalse(*FString::Printf(TEXT("%s: nessun suffisso di potenziamento"), *Nome),
			Nome.EndsWith(TEXT("+")) || Nome.EndsWith(TEXT("II")) || Nome.EndsWith(TEXT("Mk2")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushTwoAgainstDefencesTest,
	"RefactorTactics.Equipment.PushTwoSeparatesGuardFromBrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushTwoAgainstDefencesTest::RunTest(const FString&)
{
	// ⚠️ Questo test copre un caso che **prima di CP 7.1 non poteva esistere**, ed è la ragione per cui esiste.
	//
	// [D-074] aveva deciso che «ogni spinta del gioco vale 1», e su quella premessa il resolver dichiarava a
	// commento che `Guard` intercetta OGNI spinta e che `Brace` «non aggiunge copertura». Con `Weapon.Impact`
	// su `Phase.PressureJet` — che D-089 ha reso il **default** di Phase — una spinta di 2 esiste in partita, e
	// quella premessa è caduta.
	//
	// La conseguenza non è cosmetica: a distanza 2 `Guard` cede e `Brace` regge, quindi le due difese
	// smettono di essere distinguibili solo dal danno. È esattamente l'asse che `BAL-1` (#403) sta decidendo,
	// e che D-074 aveva dichiarato non osservabile in v0.1.
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Impact = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Impact"))) { Impact = V; break; }
	}
	if (!TestNotNull(TEXT("Weapon.Impact"), Impact)) { return false; }

	// Il caso in ESAME: chi è in guardia contro una spinta di 2. `GuardResistedPushDistance` vale 1, quindi
	// la guardia deve CEDERE — «la guardia non è un'ancora».
	{
		UWorld* World = MakeEquipWorld();
		if (!TestNotNull(TEXT("world guard"), World)) { return false; }
		SpawnEquipMap(World, 8);
		ARTUnit* Phase = SpawnEquipUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakePhase());
		ARTUnit* Guardato = SpawnEquipUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeWraith());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (Phase && Guardato && TM)
		{
			EquipVariantOnBasicAttack(Phase, Impact);
			Guardato->ApplyStatus(TAG_Status_Guarded, 1);
			Phase->PlannedAbilityIndex = 0;
			Phase->PlannedAttackTarget = Guardato;
			Phase->PlannedCell = Phase->Cell;
			Guardato->PlannedAbilityIndex = INDEX_NONE;
			Guardato->PlannedCell = Guardato->Cell;
			RunEquipTurn(TM);

			TestTrue(TEXT("in guardia contro una spinta di 2: la guardia CEDE e l'unita' arretra"),
				Guardato->Cell != FRTCellId(1, 0));
		}
		DestroyEquipWorld(World);
	}

	// Il gemello: chi è in `Brace` contro la stessa identica spinta REGGE, perché quel ramo non guarda la
	// distanza. Senza questa metà, il test sopra proverebbe solo che «qualcosa si è mosso».
	{
		UWorld* World = MakeEquipWorld();
		if (!TestNotNull(TEXT("world brace"), World)) { return false; }
		SpawnEquipMap(World, 8);
		ARTUnit* Phase = SpawnEquipUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakePhase());
		ARTUnit* Piantato = SpawnEquipUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeWraith());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (Phase && Piantato && TM)
		{
			EquipVariantOnBasicAttack(Phase, Impact);
			Piantato->ApplyStatus(TAG_Status_Braced, 1);
			Phase->PlannedAbilityIndex = 0;
			Phase->PlannedAttackTarget = Piantato;
			Phase->PlannedCell = Phase->Cell;
			Piantato->PlannedAbilityIndex = INDEX_NONE;
			Piantato->PlannedCell = Piantato->Cell;
			RunEquipTurn(TM);

			TestEqual(TEXT("in Brace contro la stessa spinta di 2: REGGE, e resta dov'e'"),
				Piantato->Cell, FRTCellId(1, 0));
		}
		DestroyEquipWorld(World);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantReachesTheLegacyMirrorsTest,
	"RefactorTactics.Equipment.VariantReachesWhatTheGameActuallyReads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantReachesTheLegacyMirrorsTest::RunTest(const FString&)
{
	// ⚠️ Il test che mancava, e la sua assenza era il difetto.
	//
	// `ApplyWeaponVariant` lavora su `FRTActionDef`, ma in partita chi decide legge i campi **specchio** di
	// `URTActionData`: il bot pianifica su `Ability->RangeCells` e `Ability->Power`, e il resolver costruisce
	// l'intento con `Intent.RangeCells = Ability->RangeCells`. Aggiornare solo `->Def` lascia il gioco a
	// leggere i numeri di prima — e nessun test se ne accorgeva, perché il ponte viveva in un helper di test.
	const TArray<URTEquipmentData*> Variants = URTCatalogLibrary::MakeWeaponVariants();
	const URTEquipmentData* Precision = nullptr;
	const URTEquipmentData* Overcharge = nullptr;
	for (const URTEquipmentData* V : Variants)
	{
		if (V->EquipmentId == FName(TEXT("Weapon.Precision"))) { Precision = V; }
		if (V->EquipmentId == FName(TEXT("Weapon.Overcharge"))) { Overcharge = V; }
	}
	if (!TestNotNull(TEXT("Weapon.Precision"), Precision)) { return false; }
	if (!TestNotNull(TEXT("Weapon.Overcharge"), Overcharge)) { return false; }

	// Istanza costruita come la costruisce il gioco: dal roster, non a mano.
	URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
	URTActionData* Basic = Wraith->Actions[0];
	const int32 RangePrima = Basic->RangeCells;
	const int32 PowerPrima = Basic->Power;

	URTCatalogLibrary::EquipWeaponVariant(Basic, Precision);

	// `Def` e specchio devono dire la STESSA cosa: è l'invariante che il loadout non deve poter rompere.
	TestEqual(TEXT("lo specchio della portata segue Def"), Basic->RangeCells, Basic->Def.RangeCells);
	TestEqual(TEXT("lo specchio della ricarica segue Def"), Basic->CooldownTurns, Basic->Def.CooldownTurns);
	TestEqual(TEXT("Precision: la portata che il BOT legge e' cresciuta di 1"), Basic->RangeCells, RangePrima + 1);
	TestEqual(TEXT("e la potenza che il bot legge e' scesa di 4"), Basic->Power, PowerPrima - 4);

	// `Overcharge` è il caso che perderebbe di più: la ricarica è il suo intero prezzo (D-090). Se lo
	// specchio non la ricevesse, la variante sarebbe un bonus gratis in partita.
	URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
	URTActionData* GadgetBasic = Gadget->Actions[0];
	const int32 CooldownPrima = GadgetBasic->CooldownTurns;
	URTCatalogLibrary::EquipWeaponVariant(GadgetBasic, Overcharge);
	TestEqual(TEXT("Overcharge: la ricarica arriva allo specchio, non solo a Def"),
		GadgetBasic->CooldownTurns, CooldownPrima + 1);
	TestEqual(TEXT("e coincide con Def"), GadgetBasic->CooldownTurns, GadgetBasic->Def.CooldownTurns);

	// Riequipaggiare due volte non accumula: `Power` si RICALCOLA dagli effetti, non si somma.
	URTCatalogLibrary::EquipWeaponVariant(GadgetBasic, Overcharge);
	int32 DannoDichiarato = 0;
	for (const FRTActionEffectSpec& S : GadgetBasic->Def.Effects)
	{
		if (S.Effect == ERTActionEffect::Damage) { DannoDichiarato = S.Amount; break; }
	}
	TestEqual(TEXT("Power resta uguale al danno dichiarato, senza accumulare"),
		GadgetBasic->Power, DannoDichiarato);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEmergencyDashInPlayTest,
	"RefactorTactics.Equipment.EmergencyDashMovesTheReactorInPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEmergencyDashInPlayTest::RunTest(const FString&)
{
	// Il modulo a catalogo non basta: `SelfReposition` è nato perché nessun effetto sapeva spostare la
	// sorgente, quindi la prova è che l'unità **finisca altrove** dopo essere stata bersagliata.
	const TArray<URTEquipmentData*> Modules = URTCatalogLibrary::MakeReactionModules();
	const URTEquipmentData* Dash = nullptr;
	for (const URTEquipmentData* M : Modules)
	{
		if (M->EquipmentId == FName(TEXT("Reaction.EmergencyDash"))) { Dash = M; break; }
	}
	if (!TestNotNull(TEXT("Reaction.EmergencyDash"), Dash)) { return false; }

	UWorld* World = MakeEquipWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEquipMap(World, 8);

	// L'attaccante è a (0,0), chi reagisce a (1,0): la fuga lo porta a (2,0), lontano dalla minaccia.
	ARTUnit* Attaccante = SpawnEquipUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakeWraith());
	ARTUnit* Reattore = SpawnEquipUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeWraith());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Attaccante || !Reattore || !TM) { DestroyEquipWorld(World); return false; }

	URTActionData* Reazione = URTCatalogLibrary::MakeEquipmentAction(Dash, Reattore);
	if (!TestNotNull(TEXT("l'azione del modulo"), Reazione)) { DestroyEquipWorld(World); return false; }
	// L'abilita' si accoda al kit gia' costruito da `ConfigureFromHeroData` in `SpawnEquipUnit`. I cooldown
	// sono un array PARALLELO ad `Abilities`, e `SyncAbilityCooldowns` e' privata: si riallinea passando dal
	// costruttore del kit, che e' anche il percorso che il gioco usa davvero.
	Reattore->Abilities.Add(Reazione);
	Reattore->PlannedReactionAbility = Reattore->Abilities.Num() - 1;

	Attaccante->PlannedAbilityIndex = 0;             // attacco base
	Attaccante->PlannedAttackTarget = Reattore;
	Attaccante->PlannedCell = Attaccante->Cell;
	Reattore->PlannedAbilityIndex = INDEX_NONE;
	Reattore->PlannedCell = Reattore->Cell;

	RunEquipTurn(TM);

	// La prova: si è spostato, e **allontanandosi** dall'attaccante.
	TestTrue(TEXT("chi reagisce non e' rimasto dov'era"), Reattore->Cell != FRTCellId(1, 0));
	const int32 DistPrima = URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(1, 0));
	const int32 DistDopo = URTHexLibrary::HexDistance(FRTCellId(0, 0), Reattore->Cell);
	TestTrue(TEXT("e si e' ALLONTANATO dall'attaccante, non avvicinato"), DistDopo > DistPrima);

	// L'attaccante non si muove: `SelfReposition` sposta la sorgente della REAZIONE, e nessun altro.
	TestEqual(TEXT("l'attaccante resta dov'era"), Attaccante->Cell, FRTCellId(0, 0));

	DestroyEquipWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
