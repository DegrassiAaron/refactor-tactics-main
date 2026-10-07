// Helper condivisi dei test che toccano la radice del Lab di scenari (#3544).
//
// 🔴 **Esiste perche' due copie del «file di scenario minimo» derivavano una dall'altra.** I test
// dell'indice (`RTScenarioIndexTests.cpp`) e quelli del Lab (`RTLabViewModelTests.cpp`) scrivevano ognuno
// il proprio header JSON `{ "scenarioId": ..., "tags": [...] }` e ognuno la propria radice di prova: se
// il formato minimo che l'indice legge cambia, una delle due copie resta ferma e il test che la usa
// continua a passare su un file che l'indice non legge piu' (o a fallire dall'altra parte, lontano da chi
// ha cambiato il formato). Una sola copia, un solo posto da aggiornare.
//
// 🔑 **Il «gruppo» tiene distinte le cartelle dei due file.** `LabRootDiProva(Gruppo, Nome)` vive sotto
// `AutomationTransientDir()/<Gruppo>/<Nome>`: i test dell'indice passano `RTLabIndex`, quelli del Lab
// `RTLabPrepare`, come prima. Due famiglie che girano nella stessa sessione non si cancellano le cartelle.
//
// ⛔ **L'override della radice si azzera SEMPRE, e a carico del chiamante.** `ApriRadiceDiProva` imposta
// `URTScenarioLoader::SetLabScenariosRootOverrideForTest`; `ChiudiRadiceDiProva` lo azzera. Il chiamante
// li accoppia con `ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };` subito dopo l'apertura: un `return`
// anticipato da un `TestTrue` fallito, altrimenti, lascerebbe l'override acceso e colerebbe nei test
// successivi, dove il rosso non si collega piu' a chi l'ha causato.
//
// ⛔ **Namespace NOMINATO e funzioni `inline`**, come `RTConsoleVariableGuardForTest.h`: una copia per
// unita' di traduzione e' codice duplicato nel binario, e un helper anonimo in un header e' una trappola
// ODR. Una funzione **non** `inline` qui sarebbe la collisione vera, ed e' il limite 3 dell'oracolo di
// `RTTestGuardTests.cpp` — gli header non sono guardati da quel gate.
//
// ⚠️ **Nessuna guardia `WITH_DEV_AUTOMATION_TESTS` attorno a questo file**, ed e' deliberato, per la
// stessa ragione di `RTConsoleVariableGuardForTest.h`: i `.cpp` lo includono FUORI dalla propria, quindi
// deve compilare in ogni target. Non usa nulla di `Misc/AutomationTest.h`: solo `FPaths`, `IFileManager`,
// `FFileHelper` e i tipi dell'indice e del loader, tutti presenti in ogni target.

#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"

namespace RTScenarioTestSupport
{
	/** Una radice del Lab di prova, sotto la cartella transiente dell'automation: mai `Saved/RTLab` vero. */
	inline FString LabRootDiProva(const TCHAR* Gruppo, const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), Gruppo, Nome);
	}

	/** Imposta una radice del Lab di prova vuota. Chi la chiama la azzera con `ON_SCOPE_EXIT`. */
	inline FString ApriRadiceDiProva(const TCHAR* Gruppo, const TCHAR* Nome)
	{
		const FString Root = LabRootDiProva(Gruppo, Nome);
		IFileManager::Get().DeleteDirectory(*Root, false, true);
		IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(Root);
		return Root;
	}

	/** Azzera l'override della radice e cancella la cartella di prova. */
	inline void ChiudiRadiceDiProva(const FString& Root)
	{
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString());
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	/** Scrive un file di scenario minimo — solo header — nella cartella data. */
	inline bool ScriviHeaderScenario(const FString& Dir, const TCHAR* NomeFile, const TCHAR* Id, const TCHAR* Tags)
	{
		const FString Testo = FString::Printf(TEXT("{ \"scenarioId\": \"%s\", \"tags\": [%s] }"), Id, Tags);
		return FFileHelper::SaveStringToFile(Testo, *FPaths::Combine(Dir, NomeFile),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	inline bool ContieneId(const TArray<FRTScenarioEntry>& Entries, const TCHAR* Id)
	{
		return Entries.ContainsByPredicate([Id](const FRTScenarioEntry& E) { return E.ScenarioId == Id; });
	}

	/** Quanti `*.json` ci sono (ricorsivamente) sotto la radice data. */
	inline int32 FileJsonIn(const FString& Root)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.json"), true, false);
		return Files.Num();
	}
}
