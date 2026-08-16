#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
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

	void SpawnHeroReactMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
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

	constexpr int32 HeroReactInterpositionIndex = 4; // Riktor
	constexpr int32 HeroReactDeflectionIndex = 3;    // Wraith
	constexpr int32 HeroReactCapacitorIndex = 4;     // Gadget
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorInterpositionSlotTest,
	"RefactorTactics.Heroes.BastionInterpositionUsesReactionSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorInterpositionSlotTest::RunTest(const FString&)
{
	// Lo slot `Reaction` non e' un'etichetta: significa che l'eroe puo' agire E tenere pronta la reazione
	// nello stesso turno. Si verifica sul dato e poi in partita, con Riktor che attacca *mentre* si interpone.
	URTHeroData* BastionData = URTHeroCatalogLibrary::MakeRiktor();
	const URTActionData* Interposition = BastionData->Actions[HeroReactInterpositionIndex];
	const FRTActionDef CoreIntercept = URTCatalogLibrary::FindCoreAction(TEXT("Action.Intercept"));

	TestEqual(TEXT("identita' d'eroe"), Interposition->Def.ActionId, FName(TEXT("Bastion.Interposition")));
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

	URTHeroData* RivaData = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* VektorData = URTHeroCatalogLibrary::MakeWraith();
	ARTUnit* Riktor = SpawnHeroReactUnit(World, BastionData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHeroReactUnit(World, RivaData, /*Team*/ 0, FRTCellId(1, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, VektorData, /*Team*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Riktor"), Riktor) || !TestNotNull(TEXT("alleata"), Ally)
		|| !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	// Azione principale E reazione nello stesso turno: e' esattamente cio' che lo slot dedicato consente.
	Riktor->PlannedAbilityIndex = 0; // ImpactShot, portata 3
	Riktor->PlannedAttackTarget = Enemy;
	Riktor->PlannedReactionAbility = HeroReactInterpositionIndex;

	Enemy->PlannedAbilityIndex = 0; // PulseShot sull'alleata: fa scattare l'interposizione
	Enemy->PlannedAttackTarget = Ally;

	const int32 EnemyBefore = Enemy->Health;
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione si attiva una sola volta, con la sua identita'"),
		CountHeroReactActivations(TM, TEXT("Bastion.Interposition")), 1);
	TestEqual(TEXT("e l'azione principale parte lo stesso"),
		EnemyBefore - Enemy->Health, HeroReactDeclaredDamage(BastionData->Actions[0]));

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRiktorInterpositionRedirectsTest,
	"RefactorTactics.Heroes.BastionInterpositionRedirectsDirectHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRiktorInterpositionRedirectsTest::RunTest(const FString&)
{
	// Il colpo diretto all'alleata lo incassa Riktor: e' la semantica di `Action.Intercept`, riusata senza
	// riscriverla. Riktor e' l'eroe con piu' salute del roster — interporsi e' cio' che sa fare.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* BastionData = URTHeroCatalogLibrary::MakeRiktor();
	URTHeroData* RivaData = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* VektorData = URTHeroCatalogLibrary::MakeWraith();
	ARTUnit* Riktor = SpawnHeroReactUnit(World, BastionData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHeroReactUnit(World, RivaData, /*Team*/ 0, FRTCellId(1, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, VektorData, /*Team*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Riktor"), Riktor) || !TestNotNull(TEXT("alleata"), Ally)
		|| !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	Riktor->PlannedReactionAbility = HeroReactInterpositionIndex;
	Riktor->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // PulseShot, colpo singolo
	Enemy->PlannedAttackTarget = Ally;

	const int32 BastionBefore = Riktor->Health;
	const int32 AllyBefore = Ally->Health;
	const int32 Shot = HeroReactDeclaredDamage(VektorData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Bastion.Interposition")), 1);
	TestEqual(TEXT("il colpo lo incassa Riktor"), BastionBefore - Riktor->Health, Shot);
	TestEqual(TEXT("e l'alleata non subisce nulla"), Ally->Health, AllyBefore);

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWraithDeflectionReducesTest,
	"RefactorTactics.Heroes.VektorDeflectionReducesDirectHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWraithDeflectionReducesTest::RunTest(const FString&)
{
	// -20 sul colpo diretto che l'ha innescata, dalla semantica di `Action.Deflect`. Wraith non ha difese
	// passive: la deviazione e' cio' che compra con la fragilita'.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* VektorData = URTHeroCatalogLibrary::MakeWraith();
	URTHeroData* FluxData = URTHeroCatalogLibrary::MakeGadget();
	ARTUnit* Wraith = SpawnHeroReactUnit(World, VektorData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, FluxData, /*Team*/ 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Wraith"), Wraith) || !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	Wraith->PlannedReactionAbility = HeroReactDeflectionIndex;
	Wraith->PlannedAbilityIndex = INDEX_NONE;
	Enemy->PlannedAbilityIndex = 0; // ArcPulse, colpo singolo
	Enemy->PlannedAttackTarget = Wraith;

	const int32 Before = Wraith->Health;
	const int32 Shot = HeroReactDeclaredDamage(FluxData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Vektor.Deflection")), 1);
	TestEqual(TEXT("il colpo arriva ridotto di 20"),
		Before - Wraith->Health, Shot - URTCombatLibrary::DeflectDamageReduction);
	TestEqual(TEXT("non riflette: chi ha colpito non incassa nulla"), Enemy->Health, Enemy->MaxHealth);

	DestroyHeroReactWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGadgetCapacitorShieldsAndCountersTest,
	"RefactorTactics.Heroes.FluxReactiveCapacitorShieldsAndCounters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGadgetCapacitorShieldsAndCountersTest::RunTest(const FString&)
{
	// DUE effetti nella stessa reazione (CP 5.5): scudo a Gadget **e** danno a chi l'ha colpito. Prima del
	// motore componibile ne sarebbe arrivato uno solo — ed e' il motivo per cui questo checkpoint dipende
	// da quello.
	UWorld* World = MakeHeroReactWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHeroReactMap(World);

	URTHeroData* FluxData = URTHeroCatalogLibrary::MakeGadget();
	URTHeroData* VektorData = URTHeroCatalogLibrary::MakeWraith();
	ARTUnit* Gadget = SpawnHeroReactUnit(World, FluxData, /*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnHeroReactUnit(World, VektorData, /*Team*/ 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("nemico"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyHeroReactWorld(World);
		return false;
	}

	const URTActionData* Capacitor = FluxData->Actions[HeroReactCapacitorIndex];
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

	const int32 FluxBefore = Gadget->Health;
	const int32 EnemyBefore = Enemy->Health;
	const int32 Shot = HeroReactDeclaredDamage(VektorData->Actions[0]);
	RunHeroReactTurn(TM);

	TestEqual(TEXT("la reazione risulta attivata nel TurnLog"),
		CountHeroReactActivations(TM, TEXT("Flux.ReactiveCapacitor")), 1);
	TestEqual(TEXT("lo scudo assorbe la sua parte del colpo"),
		FluxBefore - Gadget->Health, Shot - ShieldAmount);
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
	// ⚠️ **La quinta e' USCITA da questo elenco il 2026-08-10** (E18 CP 18.2, D-016): `Wraith.InterceptShot`
	// non e' piu' «rinviata», e' diventata una **Predictive Action**. La sua verifica sta sotto, e afferma il
	// contrario di quella che c'era prima: non «nessuno la raccoglie», ma «la raccoglie il boundary del Move».
	// Il test non e' stato cancellato perche' la domanda che poneva resta viva — *chi valuta questa azione?* —
	// ed e' cambiata la risposta, non la domanda.
	URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
	URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();

	struct FWired { const URTActionData* Action; const TCHAR* Id; };
	const FWired Wired[] = {
		{ Gadget->Actions[HeroReactCapacitorIndex],        TEXT("Flux.ReactiveCapacitor") },
		{ Riktor->Actions[HeroReactInterpositionIndex], TEXT("Bastion.Interposition") },
		{ Wraith->Actions[HeroReactDeflectionIndex],     TEXT("Vektor.Deflection") },
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
		{ Phase->Actions[4], TEXT("Riva.FlowReaction") },
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
	const URTActionData* Intercept = Wraith->Actions[1];
	TestEqual(TEXT("Vektor.InterceptShot: identita'"), Intercept->Def.ActionId, FName(TEXT("Vektor.InterceptShot")));
	TestTrue(TEXT("non e' una reazione: non occupa lo slot Reazione"),
		Intercept->Def.Slot != ERTActionSlot::Reaction);
	TestTrue(TEXT("e non dichiara un trigger di reazione"),
		Intercept->Def.ReactionTrigger == ERTReactionTrigger::None);
	TestTrue(TEXT("ma NON e' piu' orfana: e' predittiva, e il boundary del Move la valuta"),
		Intercept->Def.PredictiveTargeting == ERTPredictiveTargeting::LockCell
		&& Intercept->Def.PredictionBoundary == ERTPredictionBoundary::MovementEntry);

	// Il roster resta strutturalmente valido: il cablaggio non ha cambiato il numero di azioni ne' le varianti.
	const TArray<const URTHeroData*> Roster = { Gadget, Phase, Riktor, Wraith };
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes(Roster);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("roster valido dopo il cablaggio"), Errors.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
