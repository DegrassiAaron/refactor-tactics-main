#pragma once

#include "CoreMinimal.h"

struct FRTScenarioSummary;
struct FRTScenarioUnitView;

/**
 * Perche' l'elenco del launcher e' vuoto (#1705).
 *
 * ⚠️ **Le due cause si somigliano sullo schermo e non sono la stessa cosa.** «Nessuno scenario porta
 * entrambi i tag» e «la ricerca non trova niente fra quelli che li portano» chiedono due gesti opposti:
 * la prima si cura allargando i filtri, la seconda cancellando la ricerca. Un elenco che dice soltanto
 * «vuoto» manda a smontare i filtri chi doveva solo svuotare la casella di testo.
 */
enum class ERTLauncherListState : uint8
{
	/** Ci sono voci da mostrare. */
	Populated,

	/** I due tag in intersezione non lasciano passare nessuno scenario: la ricerca non c'entra. */
	NoTagMatches,

	/** I tag lasciavano passare qualcosa, e la ricerca testuale l'ha azzerato. */
	NoSearchMatches,
};

/**
 * La parte di #1705 che si puo' misurare senza un editor vivo.
 *
 * Stessa scelta di `URTDevSandboxLauncherSubsystem::ShouldOpenFor` (#1680), e per la stessa ragione: di
 * una slice fatta di Slate, cio' che un automation test vede e' solo la decisione. Tenendo qui la ricerca,
 * la classificazione del vuoto e la formattazione del readout, il pannello resta un guscio che dispone
 * widget — e cio' che puo' sbagliare ha un test.
 *
 * ⛔ **Niente qui tocca il disco.** L'elenco filtrato arriva gia' fatto da `URTScenarioIndex::ListIds`,
 * che e' la stessa funzione del Details Panel di `ARTGameMode`: un secondo catalogo sarebbe la seconda
 * autorita' che la roadmap vieta, e queste funzioni non sono in condizione di crearne uno nemmeno per
 * sbaglio — non sanno da dove vengano gli id che ricevono.
 */
class FRTLauncherScenarioBrowser
{
public:
	/**
	 * Restringe per sottostringa l'elenco **gia' filtrato per tag**, senza riordinarlo.
	 *
	 * ⚠️ **Additiva per costruzione, non per disciplina.** Il risultato e' sempre un sottoinsieme di cio'
	 * che entra, perche' la funzione non sa da nessuna parte dove pescare altri id: e' il modo piu' solido
	 * di rispettare l'AC «la ricerca restringe l'elenco filtrato». Una ricerca che ripartisse dall'indice
	 * potrebbe far ricomparire scenari che i tag avevano escluso, e sembrerebbe funzionare.
	 *
	 * Ricerca vuota = identita', quindi «cercare a filtri vuoti cerca su tutti gli 88» non e' un caso
	 * speciale ma la composizione di due identita'. Confronto senza distinzione di maiuscole: gli id sono
	 * scritti in `Camel.Case` (`Movement.Basic`) e chi cerca digita `movement`.
	 */
	static TArray<FString> ApplySearch(const TArray<FString>& FilteredIds, const FString& Search);

	/**
	 * Quale delle due cause ha svuotato l'elenco.
	 *
	 * ⚠️ **Prende due conteggi e non gli elenchi**, perche' la distinzione sta tutta nel confronto fra il
	 * prima e il dopo della ricerca: `VisibleCount` e' quanto resta, `FilteredCount` quanto c'era prima di
	 * cercare. Con il solo elenco finale la domanda non e' rispondibile.
	 */
	static ERTLauncherListState Classify(int32 FilteredCount, int32 VisibleCount);

	/** Il messaggio che il pannello mostra al posto della lista. Vuoto quando `Populated`. */
	static FText DescribeEmptyState(ERTLauncherListState State);

	/**
	 * Il terreno **come lo scenario lo dichiara**: `fixture <nome>` oppure `radius <n>`.
	 *
	 * ⛔ **Nessuna traduzione dei due in un terzo vocabolario**, che e' l'AC esplicito di #1705: un
	 * allestimento salvato e un raggio da cui generare l'arena non sono due modi di dire la stessa cosa, e
	 * un readout che li appiattisse su «mappa: media» toglierebbe di mezzo proprio l'informazione per cui
	 * si guarda il readout prima di aprire.
	 *
	 * ⚠️ Il corpus li ha entrambi — 21 fixture e 67 raggio (#1705) — quindi nessuno dei due rami e'
	 * teorico. Se un giorno uno scenario non dichiarasse nessuno dei due, questa funzione lo dice invece
	 * di scegliere un default: un raggio `0` inventato sarebbe indistinguibile da un raggio `0` vero.
	 */
	static FString DescribeTerrain(const FRTScenarioSummary& Summary);

	/**
	 * La composizione per squadra, nell'ordine dei `TeamId`: `team 0: 2 · team 1: 2`.
	 *
	 * ⚠️ **E' un readout, non una colonna della lista.** Si legge dopo l'apertura di UNO scenario perche'
	 * `FRTScenarioUnitView` esiste solo a scenario aperto: farne un asse di filtro imporrebbe di aprirli
	 * tutti a ogni ridisegno, che e' il guardrail per cui `ReadHeader` esiste.
	 */
	static FString DescribeComposition(const TArray<FRTScenarioUnitView>& Units);

	/**
	 * Le righe del readout, nell'ordine in cui il pannello le mostra.
	 *
	 * Tutto viene da `FRTScenarioSummary` e dalle viste: nessuna aritmetica propria, nessun conteggio
	 * ricavato altrove. Se un numero e' sbagliato, e' sbagliato a monte — e questo e' voluto.
	 */
	static TArray<FString> BuildReadout(const FRTScenarioSummary& Summary, const TArray<FRTScenarioUnitView>& Units);
};
