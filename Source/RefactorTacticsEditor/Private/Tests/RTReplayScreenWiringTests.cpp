// #2379 — le due schermate del replay chiamano DAVVERO il loro ViewModel.
//
// 🔑 **Perche' un test d'asset e non uno runtime.** `URTMatchHistoryWidgetBase` lo dichiara di se':
// *«Qui non c'e' layout. Il `.uasset` `WBP_RT_*` fa aspetto e disposizione; questo file dichiara **cosa il
// widget puo' fare**»*. Il C++ espone `LoadMatches` e `OpenMatchAsRecordedObserver`; **chiamarle e' compito
// del Blueprint**, e nessuna asserzione runtime legge cosa un Blueprint chiama. E' lo stesso limite che
// `RTMainMenuEntryWiringTests.cpp` esiste per aggirare, nella stessa sede: qui c'e' `UnrealEd`, e da qui si
// puo' dipendere da `BlueprintGraph`.
//
// 🔴 **Il difetto misurato il 2026-09-05 su `origin/main` `6c3ff208`**: `LoadMatches`, `SelectMatch`,
// `OpenMatch*`, `StepTurnForward`, `GetTurnLabel` e `Close` hanno **zero** occorrenze nei due `.uasset`,
// mentre `"Text Block"` ne ha 4 e 6. In PIE la cronologia mostra due placeholder e nient'altro.
//
// ⛔ **Cosa questi test NON coprono, e va saputo prima di fidarsene**:
//
//  - **quale pulsante** chiama una funzione. Legare il singolo widget alla sua chiamata richiede il nome
//    della proprieta' del pulsante, e quei nomi nascono con il cablaggio: oggi i due Blueprint non hanno
//    i widget da nominare. Questa meta' va aggiunta quando esistono, ed e' scritta in D007 della issue.
//    Il rischio che copre e' reale — un pulsante «avanti» legato a `StepTurnBackward` passa di qui.
//  - l'aspetto, il focus visibile, la percorribilita' da tastiera: vivono nel layout e restano di
//    `PIE-V01-FRONTEND-REPLAY`.
//
// ⚠️ **Cio' che coprono e' comunque discriminante**: nessuna di queste funzioni e' chiamata da un altro
// punto dei due Blueprint, quindi la loro assenza non e' ambigua come lo era `PushScreen` per `#2326` —
// dove il conteggio non distingueva `EntryReplay` da `EntrySetting`.

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* MatchHistoryBlueprintPath = TEXT("/Game/RT/UI/Framework/WBP_RT_MatchHistory.WBP_RT_MatchHistory");
	const TCHAR* ReplayViewerBlueprintPath = TEXT("/Game/RT/UI/Framework/WBP_RT_ReplayViewer.WBP_RT_ReplayViewer");

	/**
	 * Tutti i grafi in cui un widget puo' aver messo una chiamata: gli ubergraph **e** le funzioni.
	 *
	 * ⚠️ Le funzioni non sono un di piu': un `ListView` UMG cabla la riga in `OnListItemObjectSet`, che e'
	 * una funzione e non un evento dell'ubergraph. Cercare nei soli ubergraph darebbe rosso su un widget
	 * cablato correttamente — cioe' un falso difetto proprio sulla forma piu' probabile della soluzione.
	 */
	void CollectGraphs(const UBlueprint* Blueprint, TArray<UEdGraph*>& OutGraphs)
	{
		if (!Blueprint)
		{
			return;
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (Graph) { OutGraphs.Add(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->FunctionGraphs)
		{
			if (Graph) { OutGraphs.Add(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->EventGraphs)
		{
			if (Graph) { OutGraphs.Add(Graph); }
		}
	}

	/** Il nome della funzione bersaglio di un nodo di chiamata, risolto o dichiarato. */
	FString CalledFunctionName(const UK2Node_CallFunction* Call)
	{
		if (!Call)
		{
			return FString();
		}
		const UFunction* Target = Call->GetTargetFunction();
		return Target ? Target->GetName() : Call->FunctionReference.GetMemberName().ToString();
	}

	/** `true` se un qualunque grafo del Blueprint chiama `FunctionName`. `OutSeen` porta cio' che ha visto. */
	bool BlueprintCalls(const UBlueprint* Blueprint, const FString& FunctionName, TArray<FString>& OutSeen)
	{
		TArray<UEdGraph*> Graphs;
		CollectGraphs(Blueprint, Graphs);

		bool bFound = false;
		for (const UEdGraph* Graph : Graphs)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
				{
					const FString Name = CalledFunctionName(Call);
					OutSeen.AddUnique(Name);
					if (Name == FunctionName)
					{
						bFound = true;
					}
				}
			}
		}
		return bFound;
	}

	/** Il messaggio di un fallimento: cosa si cercava e cosa il grafo chiama davvero. */
	FString Referto(const FString& Cercata, const TArray<FString>& Viste)
	{
		if (Viste.Num() == 0)
		{
			return FString::Printf(TEXT("'%s' non e' chiamata, e il grafo non chiama NESSUNA funzione: e' un guscio"), *Cercata);
		}
		return FString::Printf(TEXT("'%s' non e' chiamata. Il grafo chiama: %s"), *Cercata, *FString::Join(Viste, TEXT(", ")));
	}
}

/**
 * `WBP_RT_MatchHistory` chiede l'indice a chi lo possiede — `#2379`, fetta A.
 *
 * `LoadMatches` e' l'unica porta verso `history.rtindex` esposta al Blueprint, e non e' chiamata da
 * nient'altro: se non compare qui, la lista non ha da dove prendere le partite, e cio' che si vede a
 * schermo non dipende dagli archivi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryCallsLoadMatchesTest,
	"RefactorTactics.Editor.MatchHistoryCallsLoadMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryCallsLoadMatchesTest::RunTest(const FString&)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, MatchHistoryBlueprintPath);
	if (!TestNotNull(TEXT("il Blueprint della cronologia si carica"), Blueprint))
	{
		return false;
	}

	TArray<FString> Viste;
	const bool bChiama = BlueprintCalls(Blueprint, TEXT("LoadMatches"), Viste);
	TestTrue(*Referto(TEXT("LoadMatches"), Viste), bChiama);

	return true;
}

/**
 * La riga aperta passa dalla porta giusta — `#2379`, fetta A.
 *
 * 🔑 **`OpenMatchAsRecordedObserver`, non `OpenMatch`**, e la differenza e' una decisione gia' presa:
 * `RTReplayViewerSubsystem.h:164` la dichiara *«la porta che una schermata deve chiamare»*, perche'
 * `OpenMatchAsTeam` vuole un `TeamId` che nessuna UI conosce e `OpenMatch` darebbe la vista neutrale **per
 * omissione invece che per scelta**. Un cablaggio su `OpenMatch` funzionerebbe, e sarebbe l'errore che
 * quella riga esiste per impedire: e' esattamente il caso che un test deve saper distinguere.
 *
 * ⚠️ **`SelectMatch` prima del `PushScreen`** e' il contratto di `RTReplayScreenWidgets.cpp:83`:
 * invertirli aprirebbe il viewer su nulla. Qui si verifica che la chiamata ci sia; l'ordine lo pinna il
 * test di navigazione runtime, che sa eseguire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryOpensRecordedObserverPortTest,
	"RefactorTactics.Editor.MatchHistoryOpensTheRecordedObserverPort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryOpensRecordedObserverPortTest::RunTest(const FString&)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, MatchHistoryBlueprintPath);
	if (!TestNotNull(TEXT("il Blueprint della cronologia si carica"), Blueprint))
	{
		return false;
	}

	TArray<FString> Viste;
	const bool bSeleziona = BlueprintCalls(Blueprint, TEXT("SelectMatch"), Viste);
	TestTrue(*Referto(TEXT("SelectMatch"), Viste), bSeleziona);

	const bool bApreDallaPortaGiusta = Viste.Contains(TEXT("OpenMatchAsRecordedObserver"));
	const bool bApreDallaPortaNeutrale = Viste.Contains(TEXT("OpenMatch"));

	TestTrue(
		*FString::Printf(
			TEXT("la cronologia apre con 'OpenMatchAsRecordedObserver'. Il grafo chiama: %s"),
			*FString::Join(Viste, TEXT(", "))),
		bApreDallaPortaGiusta);

	// ⛔ La porta neutrale non e' un ripiego accettabile qui: `OpenMatchAsRecordedObserver` ricade da se'
	// sulla vista completa quando l'archivio non dichiara un osservatore, quindi non c'e' un caso in cui
	// `OpenMatch` sia la scelta di una schermata.
	TestFalse(
		TEXT("e non con 'OpenMatch', che darebbe la vista neutrale per omissione (RTReplayViewerSubsystem.h:164)"),
		bApreDallaPortaNeutrale);

	return true;
}

/**
 * `WBP_RT_ReplayViewer` mostra la partita e la fa avanzare — `#2379`, fetta B.
 *
 * Le quattro funzioni cercate qui sono cio' che rende il viewer un viewer: senza `OpenMatchAsRecordedObserver`
 * non c'e' partita aperta, senza `GetTurnLabel` non si sa dove si e', senza i passi non si va avanti.
 *
 * ⛔ **Non verifica quale pulsante chiami quale passo** — vedi la nota in testa al file. Un «avanti» legato
 * a `StepTurnBackward` passa di qui, e quella meta' va aggiunta quando i pulsanti hanno un nome.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerCallsItsViewModelTest,
	"RefactorTactics.Editor.ReplayViewerCallsItsViewModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayViewerCallsItsViewModelTest::RunTest(const FString&)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ReplayViewerBlueprintPath);
	if (!TestNotNull(TEXT("il Blueprint del viewer si carica"), Blueprint))
	{
		return false;
	}

	TArray<FString> Viste;
	BlueprintCalls(Blueprint, TEXT("__nessuna__"), Viste); // riempie `Viste` una volta sola

	for (const TCHAR* Richiesta : {
			TEXT("OpenMatchAsRecordedObserver"),
			TEXT("GetTurnLabel"),
			TEXT("StepTurnForward"),
			TEXT("StepTurnBackward") })
	{
		TestTrue(*Referto(Richiesta, Viste), Viste.Contains(FString(Richiesta)));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
