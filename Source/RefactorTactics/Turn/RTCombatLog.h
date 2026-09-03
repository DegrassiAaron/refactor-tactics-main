#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h" // FRTCellId: la cella del FATTO, che il soggetto porta con se'
#include "Perception/RTTeamKnowledge.h" // FRTKnowledgeVerdict: chi puo' leggere una riga, deciso alla nascita
#include "RTCombatLog.generated.h"

class ARTUnit;

/**
 * Di CHI parla una riga di log: un'unita', oppure il mondo. Mai «non ci ho pensato».
 *
 * 🔴 **E' un tipo e non un `int32` perche' il gate diventi il compilatore** (`#1499`). Il vecchio parametro
 * aveva un default `INDEX_NONE` fail-open: un sito nuovo che nominava un nemico passava il filtro di
 * conoscenza per omissione, e l'omissione non fa rumore. Ora non esiste conversione implicita da `int32`:
 * `AddLogEvent(Testo, INDEX_NONE)` non compila, e un sito senza soggetto deve **dichiararsi** `World()`.
 *
 * ⚠️ **`World()` non e' il vecchio default con un altro nome.** Dice «questa riga riguarda tutti» — una
 * superficie che scade, il marker di turno, la fine partita — ed e' una scelta che si legge. Il default
 * diceva soltanto che nessuno aveva deciso.
 */
struct FRTLogSubject
{
	/** Un'unita' viva, con tutto cio' che serve a congelarne il verdetto ([D-223]). */
	static FRTLogSubject Unit(const ARTUnit* InUnit);

	/**
	 * La stessa unita', ma nella cella in cui il FATTO e' avvenuto — non in quella dove l'Actor si trova.
	 *
	 * 🔴 **Esiste perche' dentro `ResolveMovement` le due divergono** (`#2142`). `ARTUnit::Cell` viene
	 * scritta da `PlaceOnCell`, che gira **dopo** il ciclo dei micro-step e dopo il boundary predittivo:
	 * per tutto il tratto in mezzo la posizione autorevole sta in `State.Pos[i]` (o, chiuso il ciclo, in
	 * `Resolved[i].Final`), e `Unit()` congelerebbe il verdetto sulla cella di **partenza del turno**.
	 *
	 * ⚠️ **Non e' un'ottimizzazione ne' un caso limite**: la cella e' l'ingresso che `ClassifyTarget`
	 * confronta con le celle visibili della squadra, quindi sbagliarla fa vedere una riga a chi non deve e
	 * la nasconde a chi la vedrebbe. Chi scrive un produttore che gira prima di `PlaceOnCell` usa questa
	 * forma; chiunque altro continua a usare `Unit()`, dove `Cell` **e'** la cella del fatto.
	 */
	static FRTLogSubject UnitAt(const ARTUnit* InUnit, const FRTCellId& InFactCell);

	/**
	 * Un soggetto che porta GIA' la propria risposta, congelata altrove e prima.
	 *
	 * 🔴 **E' la forma del canale derivato dal TurnLog**, ed esiste perche' a fine turno il verdetto non e'
	 * piu' calcolabile correttamente: la conoscenza disponibile e' quella del Blast, le celle sono gia'
	 * post-Move, e `AwarenessOfUnit` confronta proprio quei due. La voce lo ha calcolato quando e' nata
	 * (`AppendLogEntry`), e qui si trasporta.
	 *
	 * ⚠️ **Non esiste una forma che prenda il solo `StableUnitId`**, ed e' deliberato: da un id soltanto il
	 * verdetto non si calcola — `ClassifyTarget` vuole anche squadra e cella — e una forma del genere
	 * inviterebbe a produrre righe fail-closed senza accorgersene.
	 */
	static FRTLogSubject Frozen(int32 InStableUnitId, const FRTKnowledgeVerdict& InVerdict);

	/** Un fatto che riguarda tutti: nessun soggetto da conoscere, nessuna ragione per nasconderlo. */
	static FRTLogSubject World();

	bool IsWorld() const { return bWorld; }
	int32 GetStableUnitId() const { return StableUnitId; }
	const ARTUnit* GetUnit() const { return Unit_; }

	/** Vero se il verdetto viaggia col soggetto: chi lo consuma non deve ricalcolarlo. */
	bool HasFrozenVerdict() const { return bFrozen; }
	const FRTKnowledgeVerdict& GetFrozenVerdict() const { return FrozenVerdict; }

	/**
	 * La cella in cui il fatto e' avvenuto: quella dichiarata da `UnitAt`, altrimenti quella dell'Actor.
	 *
	 * Un solo accesso, cosi' che verdetto e soggetto d'audit non possano leggere celle diverse — che e'
	 * precisamente il modo in cui `#2142` si e' presentato: due scritture nella stessa funzione, una sola
	 * corretta.
	 *
	 * ⚠️ **Senza unita' e senza cella dichiarata rende `FRTCellId()`, che e' la cella `(0,0,0)` — vera, e su
	 * ogni arena generata anche centrale.** Non e' un valore sicuro: e' il caso che i due chiamanti non
	 * raggiungono, ed e' la loro guardia a garantirlo, non questa funzione. `FreezeVerdictFor` esce
	 * `NoOne()` sul soggetto senza unita' **prima** di chiedere la cella, e `AppendLogEntry` la chiede solo
	 * dentro il proprio `if (Actor)`. Chi aggiunge un terzo chiamante porta con se' quella guardia — o la
	 * mette qui.
	 */
	FRTCellId GetFactCell() const;

private:
	FRTLogSubject() = default;

	bool bWorld = false;
	bool bFrozen = false;
	/** Vero se `FactCell` e' stata DICHIARATA dal produttore: senza, la cella e' quella dell'Actor. */
	bool bFactCell = false;
	int32 StableUnitId = INDEX_NONE;
	const ARTUnit* Unit_ = nullptr;
	FRTCellId FactCell;
	FRTKnowledgeVerdict FrozenVerdict;
};

/**
 * Una riga di combat log, col SOGGETTO accanto al testo.
 *
 * Il soggetto e' `ARTUnit::StableUnitId` — l'identita' che attraversa fasi e turni — oppure `INDEX_NONE`
 * per le righe che parlano del MONDO e non di un'unita' («Turno 3 - pianificazione», una superficie che
 * scade). Senza questo campo il filtro dovrebbe cercare coordinate dentro una stringa gia' formattata.
 */
USTRUCT()
struct FRTCombatLogLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Text;

	/**
	 * Chi ha prodotto il fatto. Resta per diagnosi e per i test: il filtro NON lo usa piu'.
	 *
	 * ⛔ **E non deve tornare a usarlo** (`#1499`). Il gemello di questo campo in `FRTDescribedLine` porta
	 * la storia per esteso; qui basta la conseguenza: `Verdict`, due righe sotto, e' l'unica risposta alla
	 * domanda «chi puo' leggerla», e il suo default **nasconde**. Il default di questo `INDEX_NONE`
	 * significava invece «la leggono tutti», ed e' il fail-open che `#1499` ha chiuso.
	 */
	UPROPERTY()
	int32 SubjectStableUnitId = INDEX_NONE;

	/**
	 * Chi puo' leggere questa riga, deciso quando la riga e' nata ([D-223]).
	 *
	 * 🔴 **Il default nasconde**: una riga che arrivasse qui senza verdetto non si legge. E' l'opposto del
	 * default che `#1499` ha rimosso, ed e' la direzione giusta per un filtro di privacy — si perde una
	 * riga, non si regala una posizione.
	 */
	UPROPERTY()
	FRTKnowledgeVerdict Verdict;
};

/**
 * Il filtro del combat log: pura, senza mondo, senza Actor.
 *
 * ## Perche' vive qui e non nel `TurnManager` (`#1818`)
 *
 * Ci ha vissuto fino al 2026-09-03, e con essa i due tipi qui sopra: chiunque volesse una
 * `FRTCombatLogLine` doveva includere `Turn/RTTurnManager.h`, **2 047 righe**, per una struct di tre
 * campi. Il principio #1 di `architettura-codice.md` dice che gli Actor orchestrano e non contengono la
 * matematica; questa non e' matematica, ma e' la stessa cosa nella forma piu' semplice — una regola che
 * non ha bisogno di un Actor per essere vera.
 *
 * ⚠️ **La misura che questa estrazione NON muove**: i test del combat log continuano a spawnare un
 * `ARTTurnManager`, e devono. Undici dei sedici in `RTCombatLogTests.cpp` girano turni veri per verificare
 * che il FLUSSO produca il log giusto — quello e' il loro oggetto, non il filtro. Misurato: il 97,5 % degli
 * `SpawnActor<ARTTurnManager>` del corpus sta in test di integrazione legittimi.
 */
UCLASS()
class REFACTORTACTICS_API URTCombatLogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Le righe che un osservatore puo' leggere. Pura: la si interroga senza montare una partita.
	 *
	 * 🔴 **Nessuna vista costruita qui, ed e' il punto di [D-223].** La domanda «puo' leggerla?» ha gia'
	 * una risposta, decisa quando la riga e' nata: interrogare la conoscenza di ADESSO risponderebbe a una
	 * domanda diversa, e per un soggetto nel frattempo distrutto non risponderebbe affatto.
	 *
	 * 🔴 Una riga il cui soggetto e' ignoto **sparisce intera**, non viene oscurata: una riga oscurata
	 * dice comunque che qualcosa e' successo, e quando e' successo.
	 * L'ORDINE di produzione si conserva: un combat log riordinato non e' un log.
	 *
	 * 🔴 **Ignoto significa «non visto ORA»**, non «senza voce»: un soggetto `Remembered` ha una voce, ma
	 * le coordinate stampate nella riga sono quelle attuali, cioe' cio' che la squadra ha smesso di sapere.
	 * Stessa regola di `ARTHUD::ShouldDrawUnitOverlay`, e per la stessa ragione.
	 *
	 * ⚠️ Il verdetto e' fail-closed di default: una riga che arrivasse qui senza verdetto non si legge.
	 */
	static TArray<FString> ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines, int32 ObserverTeamId);
};
