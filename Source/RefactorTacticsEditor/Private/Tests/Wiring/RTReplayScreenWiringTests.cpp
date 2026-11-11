// #2379 — le due schermate del replay chiamano DAVVERO la loro classe base.
//
// 🔑 **Perche' un test d'asset e non uno runtime.** `URTMatchHistoryWidgetBase` lo dichiara di se':
// *«Qui non c'e' layout. Il `.uasset` `WBP_RT_*` fa aspetto e disposizione; questo file dichiara **cosa il
// widget puo' fare**»*. Il C++ espone `LoadMatches` e `OpenMatch`; **chiamarle e' compito del Blueprint**,
// e nessuna asserzione runtime legge cosa un Blueprint chiama. E' lo stesso limite che
// `RTMainMenuEntryWiringTests.cpp` esiste per aggirare, nella stessa sede: qui c'e' `UnrealEd`, e da qui si
// puo' dipendere da `BlueprintGraph`.
//
// 🔴 **Il difetto misurato il 2026-09-05 su `origin/main` `6c3ff208`**: caricando i due Blueprint e
// ispezionandone i nodi non esiste **una sola** `K2Node_CallFunction`. In PIE la cronologia mostra due
// `Text Block` e nient'altro — ripetuto con 677 voci nell'indice e un archivio da 12 turni sul disco.
//
// ⚠️ **Le funzioni cercate sono quelle della CLASSE BASE, non del subsystem**, e la prima stesura di questo
// file sbagliava proprio qui: pretendeva `OpenMatchAsRecordedObserver` dalla cronologia e vietava
// `OpenMatch`. E' l'opposto del contratto —
//
//   - `URTMatchHistoryWidgetBase::OpenMatch(Id)` *«sceglie una partita e VA al viewer: dichiara la
//     selezione, poi naviga»*, passando da `SelectAndNavigate`. E' cio' che la lista deve chiamare.
//   - `URTReplayViewerWidgetBase::OpenSelected()` apre *«con gli occhi di chi l'ha giocata»*, e passa da
//     `OpenMatchAsRecordedObserver` **da se'**: la porta giusta e' gia' scelta in C++, e il Blueprint non
//     la vede ne' la puo' sbagliare.
//
// Il test vecchio avrebbe bocciato il cablaggio corretto e accettato quello sbagliato.
//
// 🔑 **E per questo si legge la classe proprietaria, non il solo nome.** `OpenMatch` esiste **due volte**:
// sul widget e su `URTReplayViewerSubsystem`. Un test che confrontasse il nome nudo non distinguerebbe una
// lista che naviga da una che apre l'archivio per conto proprio — cioe' esattamente il difetto che
// `SelectAndNavigate` documenta come vietato: *«aprirlo qui vorrebbe dire che la lista sa riprodurre»*.
//
// ⛔ **Cosa NON coprono**: quale pulsante chiama cosa. Quei nomi nascono col cablaggio, e la meta'
// mancante e' in D007 della issue. E l'aspetto, il focus e la tastiera restano di `PIE-V01-FRONTEND-REPLAY`.

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* MatchHistoryBlueprintPath = TEXT("/Game/RT/UI/Framework/WBP_RT_MatchHistory.WBP_RT_MatchHistory");
	const TCHAR* ReplayViewerBlueprintPath = TEXT("/Game/RT/UI/Framework/WBP_RT_ReplayViewer.WBP_RT_ReplayViewer");

	/**
	 * Tutti i grafi in cui un widget puo' aver messo una chiamata: gli ubergraph **e** le funzioni.
	 *
	 * ⚠️ Le funzioni non sono un di piu': un cablaggio puo' vivere in una funzione del widget invece che in
	 * un evento, e cercare nei soli ubergraph darebbe rosso su un widget cablato bene — un falso difetto.
	 */
	void CollectGraphs(const UBlueprint* Blueprint, TArray<UEdGraph*>& OutGraphs)
	{
		if (!Blueprint)
		{
			return;
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->UbergraphPages)
		{
			if (Graph) { OutGraphs.AddUnique(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->FunctionGraphs)
		{
			if (Graph) { OutGraphs.AddUnique(Graph); }
		}
		for (const TObjectPtr<UEdGraph>& Graph : Blueprint->EventGraphs)
		{
			if (Graph) { OutGraphs.AddUnique(Graph); }
		}
	}

	/**
	 * `Classe::Funzione` per ogni chiamata del Blueprint.
	 *
	 * 🔑 La classe fa la differenza fra due funzioni omonime, e qui ce ne sono: `OpenMatch` sta sul widget
	 * **e** sul subsystem. Quando il bersaglio non e' risolvibile si ripiega sul nome dichiarato nella
	 * `FunctionReference`, che e' cio' che resta leggibile in un Blueprint non compilato.
	 */
	void CollectCalls(const UBlueprint* Blueprint, TArray<FString>& OutCalls)
	{
		TArray<UEdGraph*> Graphs;
		CollectGraphs(Blueprint, Graphs);

		for (const UEdGraph* Graph : Graphs)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				const UK2Node_CallFunction* Call = Cast<const UK2Node_CallFunction>(Node);
				if (!Call)
				{
					continue;
				}
				if (const UFunction* Target = Call->GetTargetFunction())
				{
					const UClass* Owner = Target->GetOwnerClass();
					OutCalls.AddUnique(FString::Printf(TEXT("%s::%s"),
						Owner ? *Owner->GetName() : TEXT("?"), *Target->GetName()));
				}
				else
				{
					OutCalls.AddUnique(FString::Printf(TEXT("?::%s"),
						*Call->FunctionReference.GetMemberName().ToString()));
				}
			}
		}
	}

	/** `true` se una delle chiamate e' `Funzione`, su qualunque classe. */
	bool Chiama(const TArray<FString>& Calls, const FString& Funzione)
	{
		for (const FString& Call : Calls)
		{
			FString Classe, Nome;
			if (Call.Split(TEXT("::"), &Classe, &Nome) && Nome == Funzione)
			{
				return true;
			}
		}
		return false;
	}

	/** Il referto di un fallimento: cosa mancava, e cosa il grafo chiama davvero. */
	FString Referto(const FString& Cercata, const TArray<FString>& Calls)
	{
		if (Calls.Num() == 0)
		{
			return FString::Printf(
				TEXT("'%s' non e' chiamata, e il grafo non chiama NESSUNA funzione: e' un guscio"), *Cercata);
		}
		return FString::Printf(TEXT("'%s' non e' chiamata. Il grafo chiama: %s"),
			*Cercata, *FString::Join(Calls, TEXT(", ")));
	}
}

/**
 * `WBP_RT_MatchHistory` chiede l'indice a chi lo possiede — `#2379`, fetta A.
 *
 * `LoadMatches` e' l'unica porta verso `history.rtindex` esposta al Blueprint: se non compare qui, cio' che
 * si vede a schermo non dipende dagli archivi.
 *
 * ⚠️ Verifica anche `GetEmptyNoticeVisibility`, e non e' un extra: `LoadMatches` da sola disegnerebbe una
 * lista vuota identica a una lista illeggibile, e la classe base dichiara che distinguerle e' **della
 * schermata** — *«e' la schermata a dover distinguere "vuoto" da "non ho potuto leggere"»*.
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

	TArray<FString> Calls;
	CollectCalls(Blueprint, Calls);

	TestTrue(*Referto(TEXT("LoadMatches"), Calls), Chiama(Calls, TEXT("LoadMatches")));
	TestTrue(*Referto(TEXT("GetEmptyNoticeVisibility"), Calls), Chiama(Calls, TEXT("GetEmptyNoticeVisibility")));

	return true;
}

/**
 * Aprire una riga passa dalla porta della classe base — `#2379`, fetta A.
 *
 * 🔑 **`URTMatchHistoryWidgetBase::OpenMatch`, e la classe conta.** Quella funzione *«dichiara la selezione,
 * poi naviga»* attraverso `SelectAndNavigate`, il cui ordine e' il contratto: `PushScreen` presenta il
 * widget in modo sincrono, quindi il viewer consuma la selezione **durante** la chiamata.
 *
 * ⛔ **La lista non deve chiamare il subsystem.** `URTReplayViewerSubsystem::OpenMatch` ha lo stesso nome e
 * un altro significato: aprirebbe l'archivio **qui**, e `SelectAndNavigate` lo vieta per iscritto —
 * *«aprirlo qui vorrebbe dire che la lista sa riprodurre, e che un archivio resta aperto anche quando la
 * navigazione e' stata rifiutata»*. Un test sul nome nudo non vedrebbe la differenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryOpensThroughTheBaseClassTest,
	"RefactorTactics.Editor.MatchHistoryOpensThroughTheBaseClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryOpensThroughTheBaseClassTest::RunTest(const FString&)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, MatchHistoryBlueprintPath);
	if (!TestNotNull(TEXT("il Blueprint della cronologia si carica"), Blueprint))
	{
		return false;
	}

	TArray<FString> Calls;
	CollectCalls(Blueprint, Calls);

	const bool bDallaClasseBase = Calls.Contains(TEXT("RTMatchHistoryWidgetBase::OpenMatch"));
	const bool bDalSubsystem = Calls.Contains(TEXT("RTReplayViewerSubsystem::OpenMatch"));

	TestTrue(
		*FString::Printf(TEXT("la lista apre con RTMatchHistoryWidgetBase::OpenMatch. Il grafo chiama: %s"),
			Calls.Num() ? *FString::Join(Calls, TEXT(", ")) : TEXT("nulla, e' un guscio")),
		bDallaClasseBase);

	TestFalse(
		TEXT("e non con RTReplayViewerSubsystem::OpenMatch, che aprirebbe l'archivio nella lista "
			 "(SelectAndNavigate lo vieta)"),
		bDalSubsystem);

	return true;
}

/**
 * `WBP_RT_ReplayViewer` apre, spiega e torna — `#2379`, fetta B.
 *
 * Le tre funzioni sono cio' che rende il viewer un viewer, e ognuna copre un difetto diverso:
 *
 *  - `OpenSelected` — senza, non c'e' partita aperta. Passa da `OpenMatchAsRecordedObserver` da se': la
 *    scelta della porta e' in C++, e il Blueprint non puo' sbagliarla.
 *  - `GetOpenFailureText` — e' il criterio dei **quattro esiti** che il panel R6 chiedeva dal 2026-08-16.
 *    Il testo lo compone il C++ *«per la stessa ragione di `GetPhaseText()`: quattro rami in un graph node
 *    sono quattro occasioni di scriverne tre»*. Senza questa chiamata, un archivio illeggibile e' una
 *    schermata muta.
 *  - `Back` — non e' navigazione e basta: **chiude l'archivio**, e senza, `FRTReplaySession::Traces` resta
 *    in memoria per tutta la vita del processo, perche' un subsystem di `GameInstance` sopravvive a ogni
 *    caricamento di livello.
 *
 * ⛔ Non verifica quale pulsante chiami cosa — vedi la nota in testa al file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerCallsItsBaseClassTest,
	"RefactorTactics.Editor.ReplayViewerCallsItsBaseClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayViewerCallsItsBaseClassTest::RunTest(const FString&)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ReplayViewerBlueprintPath);
	if (!TestNotNull(TEXT("il Blueprint del viewer si carica"), Blueprint))
	{
		return false;
	}

	TArray<FString> Calls;
	CollectCalls(Blueprint, Calls);

	for (const TCHAR* Richiesta : { TEXT("OpenSelected"), TEXT("GetOpenFailureText"), TEXT("Back") })
	{
		TestTrue(*Referto(Richiesta, Calls), Chiama(Calls, FString(Richiesta)));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
