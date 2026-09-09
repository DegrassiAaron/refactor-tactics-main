#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 6.7 — le reazioni degli eroi FUNZIONANO IN PARTITA, non solo a catalogo.
 *
 * Questi test girano su unita' configurate con `ConfigureFromHeroData`, cioe' il percorso vero degli eroi
 * (CP 6.6), e leggono l'esito dal TurnLog e dagli HP: un test che ricostruisse a mano gli effetti resterebbe
 * verde anche scollegando il catalogo.
 *
 * Prefissi `HeroReact*` negli helper: nella unity build questo file condivide la translation unit con gli
 * altri test degli eroi e delle reazioni, e i namespace anonimi si fondono.
 */
namespace
{
	UWorld* MakeHeroReactWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHeroReactWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Restituisce l'actor: chi deve posare una copertura ha bisogno dell'asset, non solo della board. */
	ARTHexMapActor* SpawnHeroReactMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	/** Un'unita' configurata dal CATALOGO EROI: statistiche, abilita' e reazioni sono quelle spedite. */
	ARTUnit* SpawnHeroReactUnit(UWorld* World, const URTHeroData* Hero, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermi: questi test guardano il Blast
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
	}

	void RunHeroReactTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Quante volte una reazione con QUESTA identita' risulta attivata nel TurnLog. */
	int32 CountHeroReactActivations(const ARTTurnManager* TM, const TCHAR* ActionId)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction
				&& E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated)
				&& E.ActionId == FName(ActionId))
			{
				++N;
			}
		}
		return N;
	}

	/** Il danno dichiarato da un'azione d'eroe: il numero viene dal catalogo, non da una costante del test. */
	int32 HeroReactDeclaredDamage(const URTActionData* Action)
	{
		if (!Action) { return 0; }
		for (const FRTActionEffectSpec& Spec : Action->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
		}
		return 0;
	}

	constexpr int32 HeroReactInterpositionIndex = 4; // Branth
	constexpr int32 HeroReactDeflectionIndex = 3;    // Ivrin
	constexpr int32 HeroReactCapacitorIndex = 4;     // Gadget
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthInterpositionSlotTest,
	"RefactorTactics.Heroes.BranthInterpositionUsesReactionSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthInterpositionSlotTest::RunTest(const FString&)
{
	// Lo slot `Reaction` non e' un'etichetta: significa che l'eroe puo' agire E tenere pronta la reazione
	// nello stesso turno. Si verifica sul dato e poi in partita, con Branth che attacca *mentre* si interpone.
	URTHeroData* BranthData = URTHeroCatalogLibrary::MakeBranth();
	const URTActionData* Interposition = BranthData->Actions[HeroReactInterpositionIndex];
	const FRTActionDef CoreIntercept = URTCatalogLibrary::FindCoreAction(TEXT("Action.Intercept"));

	TestEqual(TEXT("identita' d'eroe"), Interposition->Def.ActionId, FName(TEXT("Hero.Branth.Interposition")));
	TestTrue(TEXT("occupa lo slot Reazione"), Interposition->Def.Slot == ERTActionSlot::Reaction);
	TestTrue(TEXT("trigger: un alleato colpito da un attacco diretto"),
		Interposition->Def.ReactionTrigger == ERTReactionTrigger::AllyHitByDirectAttack);
	TestEqual(TEXT("portata 2, come la semantica core"), Interposition->Def.RangeCells, CoreIntercept.RangeCells);
	TestEqual(TEXT("priorita' della semantica core (risolve prima delle altre reazioni)"),
		Interposition->Def.Priority, CoreIntercept.Priority);
	TestEqual(TEXT("cooldown proprio dell'eroe, non quello del core"), Interposition->Def.CooldownTurns, 3);
	TestNotEqual(TEXT("e i due cooldown differiscono davvero"),
		Interposition->Def.CooldownTurns, CoreIntercept.CooldownTurns);

	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* PhaseData = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* IvrinData = URTHeroCatalogLibrary::MakeIvrin();
	ARTUnit* Branth = SpawnHeroReactUnit(World, BranthData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHeroReactUnit(World, PhaseData, /*Team*/ 0, FRTCellId(1, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, IvrinData, /*Team*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("alleata"), Ally)
		|| !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	// Azione principale E reazione nello stesso turno: e' esattamente cio' che lo slot dedicato consente.
	Branth->PlannedAbilityIndex = 0; // ImpactShot, portata 3
	Branth->PlannedAttackTarget = Enemy;
	Branth->PlannedReactionAbility = HeroReactInterpositionIndex;

	Enemy->PlannedAbilityIndex = 0; // PulseShot sull'alleata: fa scattare l'interposizione
	Enemy->PlannedAttackTarget = Ally;

	const int32 EnemyBefore = Enemy->Health;
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione si attiva una sola volta, con la sua identita'"),
		CountHeroReactActivations(TM, TEXT("Hero.Branth.Interposition")), 1);
	TestEqual(TEXT("e l'azione principale parte lo stesso"),
		EnemyBefore - Enemy->Health, HeroReactDeclaredDamage(BranthData->Actions[0]));

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthInterpositionRedirectsTest,
	"RefactorTactics.Heroes.BranthInterpositionRedirectsDirectHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthInterpositionRedirectsTest::RunTest(const FString&)
{
	// Il colpo diretto all'alleata lo incassa Branth: e' la semantica di `Action.Intercept`, riusata senza
	// riscriverla. Branth e' l'eroe con piu' salute del roster — interporsi e' cio' che sa fare.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* BranthData = URTHeroCatalogLibrary::MakeBranth();
	URTHeroData* PhaseData = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* IvrinData = URTHeroCatalogLibrary::MakeIvrin();
	ARTUnit* Branth = SpawnHeroReactUnit(World, BranthData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHeroReactUnit(World, PhaseData, /*Team*/ 0, FRTCellId(1, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, IvrinData, /*Team*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("alleata"), Ally)
		|| !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	Branth->PlannedReactionAbility = HeroReactInterpositionIndex;
	Branth->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // PulseShot, colpo singolo
	Enemy->PlannedAttackTarget = Ally;

	const int32 BranthBefore = Branth->Health;
	const int32 AllyBefore = Ally->Health;
	const int32 Shot = HeroReactDeclaredDamage(IvrinData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Hero.Branth.Interposition")), 1);
	TestEqual(TEXT("il colpo lo incassa Branth"), BranthBefore - Branth->Health, Shot);
	TestEqual(TEXT("e l'alleata non subisce nulla"), Ally->Health, AllyBefore);

	DestroyHeroReactWorld(World);
	return true;
}

/**
 * **La copertura si rivalida su chi il colpo lo incassa DAVVERO, non su chi era il bersaglio.**
 *
 * 🔑 **L'oracolo e' il DANNO, non la sostituzione** (`PIA-1.3`, `#2616`). Che il colpo arrivi a Branth lo
 * misura gia' `Heroes.BranthInterpositionRedirectsDirectHit`; un test che si fermasse li' sarebbe **vacuo**
 * rispetto a questa proprieta' — resterebbe verde anche se la copertura non venisse rivalidata, perche' il
 * bersaglio cambia comunque.
 *
 * La regola e' [D-017], e vive in `ARTTurnManager::ResolveInterceptions`:
 *
 * > *«Riscrivere il solo `TargetId` non basta: la copertura e' gia' dentro il `Power`, calcolata sul bordo
 * > davanti a chi era il bersaglio quando i colpi sono stati raccolti. Il colpo arriverebbe a chi si
 * > interpone protetto dal muretto di qualcun altro.»*
 *
 * La fixture da' coperture **diverse** ai due bersagli, che e' cio' che rende l'asserzione discriminante:
 * l'alleata e' **scoperta**, Branth ha un riparo **basso** sul bordo da cui il colpo entra. Senza
 * `RedirectHitTo`, Branth incasserebbe il colpo pieno calcolato su di lei.
 *
 * ⚠️ **`Low` e non `High`, ed e' la riga che decide se il test misura qualcosa**: `HexCoverDamageReduction`
 * attenua solo la copertura bassa — l'alta blocca la linea di tiro, non il danno. Con `High` la riduzione
 * nominale sarebbe `0` e il test sarebbe verde per la ragione sbagliata. E' la trappola gia' pagata da
 * `RTCombatLogTests.cpp`, che la dichiara accanto alla propria fixture.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthInterpositionRevalidatesCoverTest,
	"RefactorTactics.Heroes.BranthInterpositionRevalidatesCoverOnEffectiveTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthInterpositionRevalidatesCoverTest::RunTest(const FString&)
{
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* Map = SpawnHeroReactMap(World);
	if (!TestNotNull(TEXT("mappa"), Map)) { DestroyHeroReactWorld(World); return false; }

	URTHeroData* BranthData = URTHeroCatalogLibrary::MakeBranth();
	URTHeroData* PhaseData = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* IvrinData = URTHeroCatalogLibrary::MakeIvrin();
	// Stesso layout del test gemello: l'attaccante sta a EST, quindi il colpo entra dal bordo `E`.
	ARTUnit* Branth = SpawnHeroReactUnit(World, BranthData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHeroReactUnit(World, PhaseData, /*Team*/ 0, FRTCellId(1, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, IvrinData, /*Team*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("alleata"), Ally)
		|| !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	// 🔑 **La copertura sta su BRANTH e non sull'alleata**: e' l'asimmetria che rende il test discriminante.
	// Si parte dalla cella esistente e le si aggiunge la faccia — costruirne una nuova con lo stesso `Id`
	// perderebbe il terreno posato dalla fixture.
	if (URTHexMapAsset* Asset = Map->MapAsset)
	{
		if (const FRTHexCellData* Esistente = Asset->FindCell(FRTCellId(0, 0, 0)))
		{
			FRTHexCellData Riparo = *Esistente;
			Riparo.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, 50));
			Asset->AddOrUpdateCell(Riparo);
		}
	}
	Branth->Facing = ERTHexDirection::E; // il riparo vale nell'arco frontale: guarda chi spara

	Branth->PlannedReactionAbility = HeroReactInterpositionIndex;
	Branth->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // PulseShot, colpo singolo
	Enemy->PlannedAttackTarget = Ally;

	const int32 BranthBefore = Branth->Health;
	const int32 AllyBefore = Ally->Health;
	const int32 Shot = HeroReactDeclaredDamage(IvrinData->Actions[0]);
	RunHeroReactTurn(TM);

	// Premesse: senza queste, l'asserzione sul danno misurerebbe un turno che non e' successo.
	if (!TestEqual(TEXT("premessa: la reazione si e' attivata"),
			CountHeroReactActivations(TM, TEXT("Hero.Branth.Interposition")), 1)
		|| !TestEqual(TEXT("premessa: l'alleata non incassa nulla"), Ally->Health, AllyBefore))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	// 🔴 **L'asserzione.** Il colpo era stato calcolato su un bersaglio SCOPERTO; lo incassa un bersaglio
	// RIPARATO, e il numero deve dirlo. Senza rivalidazione qui si leggerebbe `Shot`.
	TestEqual(TEXT("il danno riflette la copertura di CHI INCASSA, non quella del bersaglio originale"),
		BranthBefore - Branth->Health, Shot - URTCombatLibrary::LowCoverDamageReduction);

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIvrinDeflectionReducesTest,
	"RefactorTactics.Heroes.IvrinDeflectionReducesDirectHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIvrinDeflectionReducesTest::RunTest(const FString&)
{
	// Un pool di 20 assorbibili sui colpi diretti del boundary, dalla semantica di `Action.Deflect` ([D-309]).
	// ⚠️ Qui c'e' UN SOLO colpo, quindi pool e sconto-sul-primo-colpo danno lo stesso numero e questo test
	// non li distingue: a farlo e' `Spec.Reaction.DeflectionPoolSpansMultipleHits` (`#2190`), con quattro colpi
	// piu' piccoli del budget. Ivrin non ha difese passive: la deviazione e' cio' che compra con la fragilita'.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* IvrinData = URTHeroCatalogLibrary::MakeIvrin();
	URTHeroData* GadgetData = URTHeroCatalogLibrary::MakeGadget();
	ARTUnit* Ivrin = SpawnHeroReactUnit(World, IvrinData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, GadgetData, /*Team*/ 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Ivrin"), Ivrin) || !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	Ivrin->PlannedReactionAbility = HeroReactDeflectionIndex;
	Ivrin->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // ArcPulse, colpo singolo
	Enemy->PlannedAttackTarget = Ivrin;

	const int32 Before = Ivrin->Health;
	const int32 Shot = HeroReactDeclaredDamage(GadgetData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Hero.Ivrin.Deflection")), 1);
	TestEqual(TEXT("il colpo arriva ridotto di 20"),
		Before - Ivrin->Health, Shot - URTCombatLibrary::DeflectDamageReduction);
	TestEqual(TEXT("non riflette: chi ha colpito non incassa nulla"), Enemy->Health, Enemy->MaxHealth);

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGadgetCapacitorShieldsAndCountersTest,
	"RefactorTactics.Heroes.GadgetReactiveCapacitorShieldsAndCounters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGadgetCapacitorShieldsAndCountersTest::RunTest(const FString&)
{
	// DUE effetti nella stessa reazione (CP 5.5): scudo a Gadget **e** danno a chi l'ha colpito. Prima del
	// motore componibile ne sarebbe arrivato uno solo — ed e' il motivo per cui questo checkpoint dipende
	// da quello.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* GadgetData = URTHeroCatalogLibrary::MakeGadget();
	URTHeroData* IvrinData = URTHeroCatalogLibrary::MakeIvrin();
	ARTUnit* Gadget = SpawnHeroReactUnit(World, GadgetData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, IvrinData, /*Team*/ 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	const URTActionData* Capacitor = GadgetData->Actions[HeroReactCapacitorIndex];
	int32 ShieldAmount = 0;
	for (const FRTActionEffectSpec& Spec : Capacitor->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Shield) { ShieldAmount = Spec.Amount; }
	}
	const int32 CounterAmount = HeroReactDeclaredDamage(Capacitor);
	TestEqual(TEXT("il catalogo dichiara lo scudo da 15"), ShieldAmount, 15);
	TestEqual(TEXT("e i 10 danni all'attaccante"), CounterAmount, 10);

	Gadget->PlannedReactionAbility = HeroReactCapacitorIndex;
	Gadget->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // PulseShot, colpo singolo
	Enemy->PlannedAttackTarget = Gadget;

	const int32 GadgetBefore = Gadget->Health;
	const int32 EnemyBefore = Enemy->Health;
	const int32 Shot = HeroReactDeclaredDamage(IvrinData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Hero.Gadget.ReactiveCapacitor")), 1);
	TestEqual(TEXT("lo scudo assorbe la sua parte del colpo"),
		GadgetBefore - Gadget->Health, Shot - ShieldAmount);
	TestEqual(TEXT("e l'attaccante incassa il contraccolpo"), EnemyBefore - Enemy->Health, CounterAmount);

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroReactionsAreDeclaredTest,
	"RefactorTactics.Heroes.ReactionsDeclaredOrDeferred",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroReactionsAreDeclaredTest::RunTest(const FString&)
{
	// Cinque reazioni a catalogo: TRE cablate qui, UNA rinviata a E14 (`Phase.FlowReaction`: movimento
	// reattivo, ADR-0004). Il rinvio e' dichiarato *come dato* — slot `None`, nessun trigger — non lasciato
	// all'interpretazione di chi legge: una reazione a meta' con lo slot giusto verrebbe raccolta dal pass e
	// non farebbe nulla, in silenzio.
	//
	// ⚠️ **La quinta e' USCITA da questo elenco il 2026-08-10** (E18 CP 18.2, D-016): `Ivrin.InterceptShot`
	// non e' piu' «rinviata», e' diventata una **Predictive Action**. La sua verifica sta sotto, e afferma il
	// contrario di quella che c'era prima: non «nessuno la raccoglie», ma «la raccoglie il boundary del Move».
	// Il test non e' stato cancellato perche' la domanda che poneva resta viva — *chi valuta questa azione?* —
	// ed e' cambiata la risposta, non la domanda.
	URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
	URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* Branth = URTHeroCatalogLibrary::MakeBranth();
	URTHeroData* Ivrin = URTHeroCatalogLibrary::MakeIvrin();

	struct FWired { const URTActionData* Action; const TCHAR* Id; };
	const FWired Wired[] = {
		{ Gadget->Actions[HeroReactCapacitorIndex],        TEXT("Hero.Gadget.ReactiveCapacitor") },
		{ Branth->Actions[HeroReactInterpositionIndex], TEXT("Hero.Branth.Interposition") },
		{ Ivrin->Actions[HeroReactDeflectionIndex],     TEXT("Hero.Ivrin.Deflection") },
	};
	for (const FWired& W : Wired)
	{
		TestEqual(FString::Printf(TEXT("%s: identita'"), W.Id), W.Action->Def.ActionId, FName(W.Id));
		TestTrue(FString::Printf(TEXT("%s: slot Reazione"), W.Id), W.Action->Def.Slot == ERTActionSlot::Reaction);
		TestTrue(FString::Printf(TEXT("%s: ha un trigger"), W.Id),
			W.Action->Def.ReactionTrigger != ERTReactionTrigger::None);
		TestTrue(FString::Printf(TEXT("%s: risolve nel Blast"), W.Id),
			URTCatalogLibrary::MapResolutionPhase(W.Action->Def.ResolutionPhase) == ERTMatchPhase::Blast);
		// Il campo legacy e' quello che `ARTUnit::ConsumeAbility` legge davvero: se restasse a zero, la
		// reazione tornerebbe disponibile ogni turno senza che nulla lo segnali.
		TestEqual(FString::Printf(TEXT("%s: cooldown specchiato sul campo legacy"), W.Id),
			W.Action->CooldownTurns, W.Action->Def.CooldownTurns);
	}

	struct FDeferred { const URTActionData* Action; const TCHAR* Id; };
	const FDeferred Deferred[] = {
		{ Phase->Actions[4], TEXT("Hero.Phase.FlowReaction") },
	};
	for (const FDeferred& D : Deferred)
	{
		TestEqual(FString::Printf(TEXT("%s: identita'"), D.Id), D.Action->Def.ActionId, FName(D.Id));
		TestTrue(FString::Printf(TEXT("%s: rinviata a E14, quindi NON occupa lo slot Reazione"), D.Id),
			D.Action->Def.Slot != ERTActionSlot::Reaction);
		TestTrue(FString::Printf(TEXT("%s: e non dichiara un trigger che nessuno valuterebbe"), D.Id),
			D.Action->Def.ReactionTrigger == ERTReactionTrigger::None);
	}

	// La MIGRATA. Il rinvio si chiude dichiarando chi la valuta, non togliendo la riga: se `InterceptShot`
	// tornasse senza trigger E senza campi predittivi, sarebbe di nuovo un'azione che nessuno raccoglie — e
	// nessun test se ne accorgerebbe. Questa e' la verifica che il buco non si riapra in silenzio.
	const URTActionData* Intercept = Ivrin->Actions[1];
	TestEqual(TEXT("Hero.Ivrin.InterceptShot: identita'"), Intercept->Def.ActionId, FName(TEXT("Hero.Ivrin.InterceptShot")));
	TestTrue(TEXT("non e' una reazione: non occupa lo slot Reazione"),
		Intercept->Def.Slot != ERTActionSlot::Reaction);
	TestTrue(TEXT("e non dichiara un trigger di reazione"),
		Intercept->Def.ReactionTrigger == ERTReactionTrigger::None);
	TestTrue(TEXT("ma NON e' piu' orfana: e' predittiva, e il boundary del Move la valuta"),
		Intercept->Def.PredictiveTargeting == ERTPredictiveTargeting::LockCell
		&& Intercept->Def.PredictionBoundary == ERTPredictionBoundary::MovementEntry);

	// Il roster resta strutturalmente valido: il cablaggio non ha cambiato il numero di azioni ne' le varianti.
	const TArray<const URTHeroData*> Roster = { Gadget, Phase, Branth, Ivrin };
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes(Roster);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("roster valido dopo il cablaggio"), Errors.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
