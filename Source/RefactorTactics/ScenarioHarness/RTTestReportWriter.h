#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenarioHarness/RTTestResult.h"
#include "RTTestReportWriter.generated.h"

/**
 * Scrive il report machine-readable di una esecuzione.
 *
 * E' il punto per cui l'harness esiste: dopo una run deve bastare aprire **un** file per sapere cosa e'
 * successo, invece di leggere migliaia di righe di log Unreal. Il formato e' versionato (`schemaVersion`)
 * perche' chi lo legge deve poter rifiutare un formato che non conosce invece di interpretarlo a caso.
 *
 * Struttura: `Saved/RTTests/<ScenarioId>/<RunId>/result.json`.
 * `Saved/` e' gia' escluso da git: i report sono artefatti, non sorgenti.
 */
UCLASS()
class REFACTORTACTICS_API URTTestReportWriter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Versione dello schema di `result.json`. Va incrementata a ogni cambio non retrocompatibile. */
	static constexpr int32 SchemaVersion = 1;

	/** Radice degli artefatti di test: `Saved/RTTests/`. */
	static FString RunsRoot();

	/**
	 * Serializza il risultato in JSON. Separata dalla scrittura su disco perche' e' **pura**: i test possono
	 * verificare il contenuto del report senza toccare il filesystem.
	 */
	static FString ToJson(const FRTTestResult& Result, const FString& RunId);

	/**
	 * Scrive `result.json` nella cartella della run e ritorna il percorso della cartella in `OutDirectory`.
	 * `RunId` vuoto -> ne genera uno dall'orario locale.
	 */
	static bool Write(const FRTTestResult& Result, const FString& RunId, FString& OutDirectory, FString& OutError);

	/** Cartella dell'ultima run di uno scenario (la piu' recente per nome, che e' cronologico). Vuota se non ce ne sono. */
	static FString FindLatestRunDirectory(const FString& ScenarioId);
};
