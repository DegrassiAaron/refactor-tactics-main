#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "RTScenarioLoader.generated.h"

/**
 * Caricamento e VALIDAZIONE degli scenari di test dal formato JSON.
 *
 * La validazione e' la meta' che conta: uno scenario che nomina un eroe inesistente o una cella fuori
 * dall'arena deve fallire **subito e dicendo perche'**, non produrre un `Fail` di gioco che sembrerebbe una
 * regressione. E' la distinzione `Error` vs `Fail` (§TEST AUTOMATICI del documento di specifica).
 *
 * Funzioni pure: nessun mondo, nessun Actor. Testabili headless.
 */
UCLASS()
class REFACTORTACTICS_API URTScenarioLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Versione massima del formato che questo loader sa leggere. Un file piu' nuovo viene rifiutato con un messaggio esplicito. */
	static constexpr int32 SupportedVersion = 1;

	/**
	 * Interpreta il testo JSON di uno scenario.
	 *
	 * @return true se lo scenario e' valido e utilizzabile. In caso contrario `OutError` spiega **cosa** non va,
	 *         in una riga leggibile da chi ha scritto il file (non uno stack trace).
	 */
	static bool LoadFromString(const FString& JsonText, FRTTestScenario& OutScenario, FString& OutError);

	/** Come `LoadFromString`, leggendo da disco. Percorso mancante o illeggibile -> false con motivo. */
	static bool LoadFromFile(const FString& FilePath, FRTTestScenario& OutScenario, FString& OutError);

	/**
	 * Verifica la coerenza interna di uno scenario gia' interpretato: ID unita' univoci e non vuoti, eroi
	 * esistenti nel catalogo, celle dentro l'arena, intent che nominano unita' dichiarate, assertion complete.
	 * Separata dal parsing perche' vale anche per scenari costruiti da codice (i test del runner).
	 */
	static bool Validate(const FRTTestScenario& Scenario, FString& OutError);

	/** Radice degli scenari versionati: `<Progetto>/Scenarios/`. */
	static FString ScenariosRoot();

	/**
	 * L'enum degli esiti che appartiene a una categoria del TurnLog: `ERTMoveOutcome` se `Move`,
	 * `ERTEnvironmentOutcome` se `Environment`, e cosi' via. `nullptr` se la categoria non ne ha uno.
	 *
	 * Pubblica perche' la usano in due: il loader per tradurre i NOMI del JSON in valori, e la sessione per
	 * ritradurli in nomi nei messaggi di fallimento. Con due copie, un esito aggiunto in coda comparirebbe da
	 * una parte sola, e il report direbbe «esito 12» dove il file diceva `CoverExpired`.
	 */
	static const UEnum* OutcomeEnumForCategory(ERTLogCategory Category);

	/** Nome leggibile di un evento del TurnLog: `Environment.BridgeRemoved`. Sconosciuto -> il valore grezzo. */
	static FString DescribeLogEvent(ERTLogCategory Category, uint8 Outcome);

	// Il percorso di uno scenario NON si calcola più dal suo ID: dal momento che l'ID è dichiarato dal file
	// e le cartelle sono libere, l'unico modo di sapere dove vive è chiederlo all'indice
	// (`URTScenarioIndex::ResolvePath`). Vedi `RTScenarioIndex.h` per il perché.
};
