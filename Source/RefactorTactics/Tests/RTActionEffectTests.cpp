#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionEvent.h"
#include "Turn/RTActionQueue.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Azione pianificata con un effetto dichiarato. Nome distinto per file (unity build). */
	FRTActionInstance EffectAction(ERTActionEffect Effect, int32 Amount, int32 Source, int32 Target,
		FGameplayTag Status = FGameplayTag(), int32 StatusTurns = 0)
	{
		FRTActionInstance Instance;
		Instance.Def.ActionId = TEXT("Action.Test");
		Instance.Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Instance.Def.Effect = Effect;
		Instance.Def.EffectAmount = Amount;
		Instance.Def.StatusToApply = Status;
		Instance.Def.StatusDuration = StatusTurns;
		Instance.SourceUnitId = Source;
		Instance.TargetUnitId = Target;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectProducesEventsTest,
	"RefactorTactics.Actions.EffectsProduceEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectProducesEventsTest::RunTest(const FString&)
{
	// Ogni effetto dichiarato produce l'evento corrispondente, con l'entita' e le identita' giuste.
	{
		const TArray<FRTActionEvent> Events =
			URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Damage, 30, /*Src*/ 1, /*Tgt*/ 2));
		TestEqual(TEXT("danno: un evento"), Events.Num(), 1);
		TestTrue(TEXT("danno: tipo corretto"), Events.Num() == 1 && Events[0].Kind == ERTActionEffect::Damage);
		TestEqual(TEXT("danno: entita'"), Events[0].Amount, 30);
		TestEqual(TEXT("danno: bersaglio"), Events[0].TargetUnitId, 2);
		TestEqual(TEXT("danno: sorgente (serve alla direzione della spinta e al TurnLog)"), Events[0].SourceUnitId, 1);
	}
	{
		const TArray<FRTActionEvent> Events =
			URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Shield, 40, 1, 1));
		TestTrue(TEXT("scudo su se stessi"), Events.Num() == 1 && Events[0].Kind == ERTActionEffect::Shield);
		TestEqual(TEXT("scudo: entita'"), Events[0].Amount, 40);
	}
	{
		const TArray<FRTActionEvent> Events =
			URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Heal, 20, 1, 2));
		TestTrue(TEXT("cura"), Events.Num() == 1 && Events[0].Kind == ERTActionEffect::Heal);
	}
	{
		const TArray<FRTActionEvent> Events = URTActionEffectLibrary::ProduceEvents(
			EffectAction(ERTActionEffect::Status, 0, 1, 2, TAG_Status_Slow, /*Turni*/ 2));
		TestTrue(TEXT("stato"), Events.Num() == 1 && Events[0].Kind == ERTActionEffect::Status);
		TestTrue(TEXT("stato: il tag arriva nell'evento"), Events[0].StatusTag == TAG_Status_Slow);
		TestEqual(TEXT("stato: la durata viaggia in Amount"), Events[0].Amount, 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectDegenerateTest,
	"RefactorTactics.Actions.EffectsRejectDegenerateInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectDegenerateTest::RunTest(const FString&)
{
	// Un'azione senza effetto non produce nulla: e' il caso di `Action.Wait`, che occupa lo slot e basta.
	TestEqual(TEXT("nessun effetto -> nessun evento"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::None, 0, 1, 2)).Num(), 0);

	// Entita' non positiva: nessun evento, invece di uno che "cura zero" e sporca il log.
	TestEqual(TEXT("danno 0 -> nessun evento"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Damage, 0, 1, 2)).Num(), 0);
	TestEqual(TEXT("cura negativa -> nessun evento (una cura non fa male)"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Heal, -5, 1, 2)).Num(), 0);

	// Bersaglio assente: nessun evento (l'azione e' stata invalidata a monte).
	TestEqual(TEXT("bersaglio assente -> nessun evento"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Damage, 30, 1, INDEX_NONE)).Num(), 0);

	// Stato senza tag o senza durata: niente evento, altrimenti si applicherebbe "qualcosa" di indefinito.
	TestEqual(TEXT("stato senza tag -> nessun evento"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Status, 0, 1, 2, FGameplayTag(), 2)).Num(), 0);
	TestEqual(TEXT("stato senza durata -> nessun evento"),
		URTActionEffectLibrary::ProduceEvents(EffectAction(ERTActionEffect::Status, 0, 1, 2, TAG_Status_Root, 0)).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectNewOneNeedsNoOrchestratorTest,
	"RefactorTactics.Actions.NewEffectNeedsNoOrchestratorChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectNewOneNeedsNoOrchestratorTest::RunTest(const FString&)
{
	// Criterio d'accettazione della spec E4: un'azione con un effetto DIVERSO passa dallo stesso percorso,
	// senza che l'orchestratore sappia di che effetto si tratti. Qui si verifica la proprieta' che lo rende
	// possibile: `ProduceEvents` e' totale sugli effetti dichiarati e il chiamante lavora sugli EVENTI.
	const ERTActionEffect All[] = {
		ERTActionEffect::None, ERTActionEffect::Damage, ERTActionEffect::Heal,
		ERTActionEffect::Shield, ERTActionEffect::Push, ERTActionEffect::Status
	};
	for (const ERTActionEffect Effect : All)
	{
		FRTActionInstance Instance = EffectAction(Effect, 10, /*Src*/ 0, /*Tgt*/ 1, TAG_Status_Slow, 1);
		const TArray<FRTActionEvent> Events = URTActionEffectLibrary::ProduceEvents(Instance);

		// Nessun effetto dichiarato deve "cadere fuori" dalla traduzione: o produce un evento del proprio
		// tipo, o non produce nulla per una ragione dichiarata (None).
		if (Effect == ERTActionEffect::None)
		{
			TestEqual(TEXT("None non produce eventi"), Events.Num(), 0);
		}
		else
		{
			TestEqual(FString::Printf(TEXT("l'effetto %d produce un evento"), static_cast<int32>(Effect)),
				Events.Num(), 1);
			TestTrue(TEXT("l'evento ha il tipo dell'effetto"), Events.Num() == 1 && Events[0].Kind == Effect);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
