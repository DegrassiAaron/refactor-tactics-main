// #2326 — l'ingresso alla cronologia esiste DAVVERO, e porta dove dice.
//
// 🔑 **Questo file chiude un buco che un altro test dichiara di avere.**
// `Source/RefactorTactics/Tests/RTFrontendWidgetAssetTests.cpp`, nel commento di
// `MainMenuGraphAsksTheNavigatorToStart`, scrive di NON poter attribuire una chiamata al pulsante che la
// origina — «UMG compila **ogni** evento del widget in un unico `ExecuteUbergraph_WBP_RT_MainMenu`, la cui
// lista di referenze e' l'unione di PLAY, SETTINGS e QUIT» — e nomina la sede giusta per rimediare:
// `Source/RefactorTacticsEditor/Private/Tests/`, che ha `UnrealEd` e da cui si puo' dipendere da
// `BlueprintGraph`.
//
// ⚠️ **Per l'ingresso della cronologia quel limite non e' teorico: e' fatale.** `PushScreen` e' gia'
// chiamata da `EntrySetting`, quindi l'asserzione «il grafo chiama `PushScreen`» sarebbe **verde anche con
// l'ingresso cancellato**. Un test scritto con la tecnica runtime non avrebbe potuto distinguere i due
// mondi, ed e' la ragione per cui questo esiste: qui si seguono i **pin**, dal nodo evento di
// `EntryReplay` fino alla chiamata, e si legge l'id di schermata che le arriva.
//
// ⛔ **Cosa NON copre**: l'aspetto, il focus visibile e la percorribilita' da tastiera. Vivono nel layout
// del `.uasset` e restano di `PIE-NAV-REPLAY`. Un ingresso cablato su un widget largo zero pixel passa di
// qui.

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* MainMenuBlueprintPath = TEXT("/Game/RT/UI/Framework/WBP_RT_MainMenu.WBP_RT_MainMenu");

	/**
	 * Il nodo evento di `PropertyName.EventName`, cercato in tutti gli ubergraph.
	 *
	 * ⚠️ Confronta **due** nomi, non uno: il widget e il delegate. Un match sul solo widget troverebbe un
	 * secondo evento dello stesso pulsante (`OnHovered`, per dire) e il cammino partirebbe dal posto
	 * sbagliato senza che nulla lo segnali.
	 */
	UK2Node_ComponentBoundEvent* FindBoundEvent(const UBlueprint* Blueprint, FName PropertyName, FName EventName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (!Graph)
			{
				continue;
			}
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_ComponentBoundEvent* Bound = Cast<UK2Node_ComponentBoundEvent>(Node);
				if (Bound
					&& Bound->GetComponentPropertyName() == PropertyName
					&& Bound->DelegatePropertyName == EventName)
				{
					return Bound;
				}
			}
		}
		return nullptr;
	}

	/** Il pin di esecuzione in uscita di un nodo, se ne ha uno. */
	UEdGraphPin* ExecOut(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == TEXT("exec"))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	/**
	 * Segue la catena di esecuzione da `Start` e ritorna la prima chiamata a `FunctionName`.
	 *
	 * `Visited` non e' prudenza generica: un grafo con un ciclo di esecuzione farebbe girare la funzione
	 * per sempre, e un test che non termina non e' un test rosso — e' una suite che non finisce.
	 *
	 * `OutSteps` porta i nodi attraversati, cosi' un fallimento dice **dove** il cammino si e' fermato
	 * invece del solo fatto che non e' arrivato.
	 */
	UK2Node_CallFunction* FollowExecToCall(
		UEdGraphNode* Start, const FString& FunctionName, TArray<FString>& OutSteps)
	{
		TSet<UEdGraphNode*> Visited;
		UEdGraphNode* Current = Start;

		while (Current && !Visited.Contains(Current))
		{
			Visited.Add(Current);

			UEdGraphPin* Out = ExecOut(Current);
			if (!Out || Out->LinkedTo.Num() == 0)
			{
				return nullptr;
			}

			UEdGraphNode* Next = Out->LinkedTo[0] ? Out->LinkedTo[0]->GetOwningNode() : nullptr;
			if (!Next)
			{
				return nullptr;
			}

			OutSteps.Add(Next->GetClass()->GetName());

			if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Next))
			{
				const UFunction* Target = Call->GetTargetFunction();
				const FString Name = Target ? Target->GetName() : Call->FunctionReference.GetMemberName().ToString();
				OutSteps.Add(FString::Printf(TEXT("  -> chiama %s"), *Name));
				if (Name == FunctionName)
				{
					return Call;
				}
			}

			Current = Next;
		}
		return nullptr;
	}
}

/**
 * `EntryReplay.OnEntryClicked` porta a `PushScreen("MatchHistory")` — `#2326`.
 *
 * Dal fatto piu' generale al piu' specifico:
 *
 *  1. il Blueprint del menu si carica;
 *  2. esiste il nodo evento **di `EntryReplay`**, e per il delegate `OnEntryClicked`;
 *  3. da quel nodo la catena di esecuzione raggiunge una chiamata a `PushScreen`;
 *  4. l'id di schermata che le arriva e' **`MatchHistory`**, letto dal pin e non presunto.
 *
 * 🔑 **Il punto 4 e' l'unico non vacuo**, e regge tutto il resto: `PushScreen` e' gia' chiamata da
 * `EntrySetting` con `"Settings"`, quindi fermarsi al punto 3 darebbe verde su un menu senza cronologia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuReplayEntryPushesMatchHistoryTest,
	"RefactorTactics.Editor.MainMenuReplayEntryPushesMatchHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuReplayEntryPushesMatchHistoryTest::RunTest(const FString&)
{
	// `UBlueprint` e non `UWidgetBlueprint`: la classe base basta per gli ubergraph, e cosi' questo modulo
	// non deve dipendere da `UMGEditor`.
	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, MainMenuBlueprintPath));

	if (!TestNotNull(TEXT("WBP_RT_MainMenu si carica come Blueprint"), Blueprint))
	{
		return false;
	}

	UK2Node_ComponentBoundEvent* Bound =
		FindBoundEvent(Blueprint, TEXT("EntryReplay"), TEXT("OnEntryClicked"));

	if (!Bound)
	{
		// L'evidenza prima del verdetto: quali eventi ci sono davvero, cosi' un rename dell'entry si
		// diagnostica dal log invece che riaprendo l'editor.
		TArray<FString> Found;
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (!Graph) { continue; }
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (const UK2Node_ComponentBoundEvent* Other = Cast<UK2Node_ComponentBoundEvent>(Node))
				{
					Found.Add(FString::Printf(TEXT("%s.%s"),
						*Other->GetComponentPropertyName().ToString(),
						*Other->DelegatePropertyName.ToString()));
				}
			}
		}
		AddError(FString::Printf(
			TEXT("nessun evento 'EntryReplay.OnEntryClicked' nel menu. Gli eventi cablati sono: %s. ")
			TEXT("Senza questo nodo la voce REPLAY non fa niente, e nessun altro test lo direbbe."),
			Found.Num() > 0 ? *FString::Join(Found, TEXT(", ")) : TEXT("nessuno")));
		return false;
	}

	TArray<FString> Steps;
	UK2Node_CallFunction* Push = FollowExecToCall(Bound, TEXT("PushScreen"), Steps);

	AddInfo(FString::Printf(TEXT("cammino da EntryReplay.OnEntryClicked: %s"),
		Steps.Num() > 0 ? *FString::Join(Steps, TEXT(" | ")) : TEXT("nessun passo")));

	if (!TestNotNull(TEXT("la catena raggiunge PushScreen"), Push))
	{
		return false;
	}

	UEdGraphPin* ScreenIdPin = Push->FindPin(TEXT("ScreenId"), EGPD_Input);
	if (!TestNotNull(TEXT("PushScreen ha il pin ScreenId"), ScreenIdPin))
	{
		return false;
	}

	// ⚠️ Il valore va letto dal **default** del pin, e vale solo se il pin non e' collegato: un id calcolato
	// a runtime non sarebbe leggibile qui, e dichiararlo uguale a `MatchHistory` sarebbe un'invenzione.
	if (ScreenIdPin->LinkedTo.Num() > 0)
	{
		AddError(TEXT(
			"il pin ScreenId e' COLLEGATO: l'id arriva da un calcolo, non da un letterale. Questo test non "
			"puo' dire quale schermata venga spinta, e il verdetto va spostato su una verifica in PIE."));
		return false;
	}

	TestEqual(TEXT("l'id di schermata spinto e' MatchHistory"),
		ScreenIdPin->DefaultValue, FString(TEXT("MatchHistory")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
