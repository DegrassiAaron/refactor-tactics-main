#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTPacing.h"
#include "Turn/RTTurnLog.h" // FRTTurnLogEntry: le finestre aperte si CONTANO dal fatto registrato
#include "RTPacingLibrary.generated.h"

/** Calcoli puri sulla telemetria di pacing (nessun Actor, nessun file, testabili headless). */
UCLASS()
class REFACTORTACTICS_API URTPacingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Percentile con metodo NEAREST-RANK su un array GIA' ORDINATO in modo crescente: nessuna
	 * interpolazione, quindi il risultato e' sempre un valore realmente osservato e il test si scrive a mano.
	 * Rango 1-based = ceil(Percentile/100 * N). Array vuoto -> 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static int32 PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile);

	/**
	 * Sommario dei campioni. `CutoffWindowMs` e' la soglia che separa un TAGLIO (timeout con input piu'
	 * recente della soglia) da un'ATTESA A VUOTO: e' un parametro esplicito e non una costante sepolta,
	 * perche' e' una decisione di design ritarabile. Array vuoto -> sommario tutto a zero (fail-closed).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FRTPacingSummary SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs);

	/**
	 * Quante finestre di reazione si sono APERTE, fra le voci di `Entries`, ai responder di `ResponderUnitIds`
	 * (CP 14.6, `#166`). Insieme vuoto -> 0.
	 *
	 * 🔴 **Aperta non vuol dire registrata**, e la differenza e' tutto il valore di questa funzione: la
	 * categoria `ReactionDecision` porta anche esiti che **non hanno occupato nessuno**:
	 *
	 * - `HoldImmediate` (cardinalita' <= 1) e `HoldCollapsedByCondition` (la condizione dichiarata ha ridotto
	 *   le risposte a una): commit immediati, nessuna finestra si apre.
	 * - `HoldNoDecider`: la finestra **esiste** e nessuno puo' rispondere — e' l'unita' umana senza UI, cioe'
	 *   **ogni** finestra del giocatore finche' CP 14.6 non consegna l'interfaccia. Contarla scriverebbe
	 *   `3,0 s` di attesa per una persona mai interpellata: l'inflazione sistematica, non l'eccezione.
	 *
	 * Contarli tutti gonfierebbe la baseline con attese che non esistono — il difetto che quegli esiti sono
	 * stati separati per rendere visibile.
	 *
	 * L'elenco e' **positivo** e passa da uno `switch` senza `default`: un esito nuovo non entra nel
	 * conteggio finche' qualcuno non lo dichiara, ed e' il compilatore a chiederlo. Un «tutto tranne» avrebbe
	 * contato per difetto ogni valore aggiunto in coda all'enum — che e' il modo in cui quell'enum cresce
	 * per prescrizione.
	 *
	 * Il filtro per responder e' obbligatorio e non un comodo: [D-167] distingue due unita' armate dello
	 * STESSO giocatore — due finestre in fila su una persona — da due di squadre diverse, che sono due attese
	 * parallele. Un conteggio di partita non sa dire quale delle due sta misurando.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static int32 CountOpenedReactionWindows(const TArray<FRTTurnLogEntry>& Entries,
		const TSet<int32>& ResponderUnitIds);

	/**
	 * Il tempo che quelle finestre possono occupare al giocatore, in secondi: `Windows × WindowSeconds`.
	 *
	 * ⚠️ **E' un LIMITE SUPERIORE, e va letto come tale**: vale se ogni finestra arriva a scadenza. Chi
	 * risponde subito ne consuma una frazione, ed e' esattamente il gradiente su cui
	 * `spec-decision-time-bank.md` §3.3 costruisce il bank. Questa funzione produce il tetto — il numero che
	 * `InitialBankMs` deve poter coprire — non la media, che senza decisori veri non esiste.
	 *
	 * Valori negativi in ingresso danno 0: una finestra che dura meno di zero non e' un'attesa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static float ReactionDecisionSecondsUpperBound(int32 OpenedWindows, float WindowSeconds);

	/**
	 * Riempie i conteggi di APERTURA del campione: unita' vive per squadra e spazio di decisione.
	 *
	 * Due regole, entrambe gia' scritte nel codice che questa funzione sostituisce e nessuna delle due
	 * ovvia:
	 *
	 *  - **le vive si contano di entrambe le squadre**, perche' `UnitsAliveTeam0/1` descrive lo stato del
	 *    campo e non della squadra misurata;
	 *  - **`ActionsAvailable` conta solo `PacingTeamId`**, perche' misura lo spazio di decisione di **chi
	 *    decide**. Sommare anche l'avversario darebbe un numero che cresce quando il nemico ha piu' opzioni,
	 *    cioe' esattamente il contrario di cio' che la metrica dichiara.
	 *
	 * Un'unita' morta non porta azioni: e' esclusa da entrambi i conteggi. I campi non toccati del campione
	 * restano come sono — la funzione riempie, non azzera.
	 */
	// Non `UFUNCTION`: `FRTPacingUnitFacts` non e' un tipo Blueprint e non deve diventarlo. E' l'ingresso
	// interno di una regola, non un dato che qualcuno consulta — e renderlo `BlueprintType` solo per
	// soddisfare UHT allargherebbe la superficie esposta per una ragione che non e' del dominio.
	static void ApplyOpeningCounts(const TArray<FRTPacingUnitFacts>& Units, int32 PacingTeamId,
		FRTPacingSample& Sample);

	/**
	 * Gli id da passare a `CountOpenedReactionWindows` come responder: le unita' di `PacingTeamId`, **id
	 * `0` escluso**, in ordine crescente.
	 *
	 * ⚠️ **Nessun filtro su `bIsAlive`, e non e' una dimenticanza** — e' la differenza che la separa da
	 * `ApplyOpeningCounts`. Un'unita' caduta **durante** il turno ha comunque potuto aprire finestre prima
	 * di cadere, e scartarla farebbe sparire proprio le attese dei turni piu' concitati, cioe' quelle che
	 * tarano il bank.
	 *
	 * 🔴 **Lo `0` non entra** ([D-063]): e' riservato a «nessuna unita' dichiarata», e un'unita' spawnata
	 * dopo il congelamento del roster lo conserva. Con `0` nel set, una voce di log senza soggetto — o
	 * un'evocazione avversaria nella stessa condizione — finirebbe nel bank del giocatore misurato: la
	 * confusione fra squadre che il filtro per responder esiste per impedire ([D-167]).
	 *
	 * Ordinato perche' il risultato di una telemetria non deve dipendere dall'ordine in cui gli Actor sono
	 * stati raccolti, nemmeno quando non decide nulla.
	 */
	// Non `UFUNCTION`, per la stessa ragione di `ApplyOpeningCounts`.
	static TArray<int32> RespondersForPacing(const TArray<FRTPacingUnitFacts>& Units, int32 PacingTeamId);

	/** Intestazione del CSV: quattordici colonne, nello stesso ordine di CsvRow. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvHeader();

	/** Una riga CSV: tutti interi con %d, quindi nessuna virgola decimale introdotta dal locale. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvRow(const FRTPacingSample& Sample);
};
