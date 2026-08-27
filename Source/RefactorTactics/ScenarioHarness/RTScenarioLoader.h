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
	// 1 → 2 con `#512`: la versione che ammette `decisions` a livello di turno. I 76 scenari esistenti
	// restano a 1 e non si toccano — il gate confronta con `>`, quindi una versione piu' bassa e' sempre
	// leggibile. Il verso che conta e' l'altro: una build vecchia deve RIFIUTARE uno scenario che non sa
	// leggere invece di ignorarne i campi, e da `#926` gli scenari viaggiano dentro il pacchetto.
	//
	// 2 → 3 con `#314` fetta 4: `decisions.respond` accetta le risposte di un **Reaction Profile**
	// (`Hold Ground`, `SIDESTEP`, …) oltre a `FIRE`/`HOLD`.
	//
	// ⚠️ **Il bump e' stato ARGOMENTATO e non dato per scontato, perche' il cambiamento e' solo espansivo**:
	// nessun file `version: 2` valido oggi cambia significato, e la chiave `decisions` esisteva gia'. La
	// ragione per cui si bumpa lo stesso e' il verso che conta — un file che usasse `respond: "SIDESTEP"`
	// dichiarandosi `version: 2` verrebbe **accettato** da una build a `SupportedVersion = 2`, che poi lo
	// rifiuterebbe con «risposta sconosciuta»: un messaggio che accusa il FILE mentre il difetto e' la build
	// troppo vecchia. Con la `3` il rifiuto arriva dal gate di versione e dice la cosa giusta.
	// 3 → 4 con `#957`: le chiavi del **free-run** (`freeRun`, `maxTurns`, `repeatCount`, `requires` di
	// scenario). Il bump segue lo stesso ragionamento della `3`, e qui il verso che conta morde piu' forte:
	// una build a `SupportedVersion = 3` **ignorerebbe** `freeRun` e leggerebbe un file con `turns` vuoto —
	// cioe' uno scenario che verifica il solo stato iniziale. Non uscirebbe rosso: uscirebbe **verde**, senza
	// aver giocato un turno. Un verde per assenza di partita e' il peggiore degli esiti, perche' nessuno va a
	// guardarlo. Con la `4` il rifiuto arriva dal gate di versione e nomina la build.
	static constexpr int32 SupportedVersion = 4;

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

	/**
	 * Le regole di piazzamento di **una** unita': id non vuoto e non duplicato, loadout legale, eroe esistente,
	 * cella dentro l'arena, cella non gia' occupata, cella che non blocca il movimento, HP/scudo/vista coerenti.
	 *
	 * ⚠️ **Esiste perche' `Validate` risponde alla domanda sbagliata per un editor.** `Validate` giudica lo
	 * scenario INTERO, e uno scenario in costruzione e' quasi sempre invalido — non ha ancora assertion, spesso
	 * non ha ancora la seconda squadra. Usarlo come gate a ogni piazzamento renderebbe impossibile costruire uno
	 * scenario: ogni `AddUnit` verrebbe rifiutato per qualcosa che non c'entra con l'unita' che si sta piazzando.
	 *
	 * L'alternativa era che l'editor si scrivesse i propri controlli, cioe' una **seconda copia** delle regole
	 * che diverge dalla prima al primo campo aggiunto. Questa funzione e' l'estrazione che evita entrambe:
	 * `ValidateScenarioUnits` la chiama in ciclo, l'authoring la chiama per una unita' sola, e la regola resta
	 * scritta in un posto.
	 *
	 * @param IgnoreUnitIndex Indice in `Scenario.Units` da **saltare** nei confronti fra unita'. `INDEX_NONE` per
	 *        una unita' che non e' ancora nell'array (piazzamento nuovo); l'indice dell'unita' stessa quando la
	 *        si sta spostando o rivalidando, altrimenti collidera' con se' stessa e nessuno potra' mai muoversi.
	 */
	static bool ValidateUnitPlacement(const FRTTestScenario& Scenario, const FRTScenarioUnit& Unit,
		int32 IgnoreUnitIndex, FString& OutError);

	/**
	 * Serializza uno scenario nel formato JSON che `LoadFromString` rilegge. Il verso mancante del loader.
	 *
	 * Sta qui, e non in un `URTScenarioWriter` a parte, per una ragione sola: **il formato ha un owner solo**.
	 * Lettura e scrittura sono due meta' della stessa regola, e separarle in due classi renderebbe possibile
	 * aggiungere una chiave da una parte e non dall'altra — che e' esattamente il difetto che un authoring
	 * visuale introdurrebbe se dovesse conoscere il JSON da se'. L'implementazione vive in
	 * `RTScenarioWriter.cpp` perche' il `.cpp` del loader ha gia' 1769 righe, ma la dichiarazione e' una.
	 *
	 * Chiama `Validate` **prima** di produrre qualunque testo: uno scenario invalido non viene serializzato a
	 * meta'. In piu' verifica che la `version` dichiarata basti per le chiavi effettivamente usate — senza
	 * quel controllo un round-trip potrebbe scrivere un file che il loader poi rifiuta, e uno scenario
	 * costruito in memoria (dall'editor) e' l'unico posto da cui quello stato puo' arrivare.
	 *
	 * Forma canonica: campi in ordine fisso, default omessi, nessuna dipendenza dall'ordine di iterazione di
	 * una `TMap`. Due scritture dello stesso scenario producono lo stesso testo.
	 *
	 * @return true se lo scenario e' stato serializzato. Altrimenti `OutError` nomina **il campo** che lo ha
	 *         impedito, e `OutJson` resta intatto.
	 */
	static bool SaveToString(const FRTTestScenario& Scenario, FString& OutJson, FString& OutError);

	/**
	 * Come `SaveToString`, scrivendo su disco. La scrittura e' **esplicita**: nessun salvataggio implicito.
	 *
	 * Il file non viene toccato se lo scenario non passa `Validate` — un file a meta' e' peggio di un file
	 * non scritto, perche' il secondo si nota subito.
	 *
	 * ⚠️ Il percorso NON influisce sullo `ScenarioId`: l'identita' e' dichiarata dal file, non dedotta dalla
	 * cartella. Salvare altrove produce lo stesso `scenarioId` (`RTScenarioIndex.h` per il perche').
	 */
	static bool SaveToFile(const FRTTestScenario& Scenario, const FString& FilePath, FString& OutError);

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
