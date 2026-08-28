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
	 * categoria `ReactionDecision` porta anche gli esiti in cui **nessuna finestra si e' aperta** —
	 * `HoldImmediate` (cardinalita' <= 1) e `HoldCollapsedByCondition` (la condizione dichiarata ha ridotto
	 * le risposte a una). Quelli si committano subito e non occupano un secondo del giocatore. Contare tutte
	 * le voci gonfierebbe la baseline con attese che non esistono, e sarebbe il difetto che i due esiti sono
	 * stati separati per rendere visibile.
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

	/** Intestazione del CSV: quattordici colonne, nello stesso ordine di CsvRow. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvHeader();

	/** Una riga CSV: tutti interi con %d, quindi nessuna virgola decimale introdotta dal locale. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvRow(const FRTPacingSample& Sample);
};
