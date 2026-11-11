#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h" // FRTCellId: le celle attraversate
#include "Perception/RTTeamKnowledge.h" // FRTKnowledgeVerdict + ObservedPrefixLength: la regola del troncamento
#include "RTMoveRoute.generated.h"

/**
 * Una rotta percorsa nell'ultima risoluzione, col SOGGETTO accanto alle celle.
 *
 * Stessa forma di `FRTCombatLogLine` e per la stessa ragione: un dato destinato alla presentazione porta
 * l'identita' di CHI lo ha prodotto, perche' a valle non c'e' modo di ricostruirla.
 *
 * 🔴 **L'indice dell'array non e' mai stato un'identita', e non puo' diventarlo** (`#1497`): la raccolta e'
 * COMPATTATA — aggiunge una voce solo per chi si e' davvero mosso — quindi con le unita' 1 e 3 in movimento
 * e la 2 ferma le due rotte stanno agli indici `0` e `1`. Una stesura precedente di `rt.Debug.DrawPaths`
 * stampava «unita %d» su quell'indice e nominava un'unita' che non si era mossa.
 *
 * Senza questo campo la traccia non e' filtrabile contro la conoscenza di squadra, e il percorso di un
 * nemico che non si vede resta disegnato a schermo.
 */
USTRUCT()
struct FRTMoveRoute
{
	GENERATED_BODY()

	/** Chi ha percorso questa rotta. `ARTUnit::StableUnitId`, mai un indice di array. */
	UPROPERTY()
	int32 StableUnitId = INDEX_NONE;

	/** Cella di partenza seguita dalle celle attraversate, nell'ordine in cui sono state percorse. */
	UPROPERTY()
	TArray<FRTCellId> Cells;

	/**
	 * Chi puo' vedere disegnata CIASCUNA cella di `Cells`, deciso quando la rotta e' stata raccolta
	 * ([D-223], emendamento del 2026-08-28). Parallelo a `Cells`, stesso indice.
	 *
	 * 🔴 **Un verdetto per cella e non uno per rotta, perche' una rotta non e' un fatto puntuale.** Una riga
	 * di log ha un istante; una rotta e' una traiettoria che ne attraversa due — partenza e arrivo — e un
	 * verdetto congelato sulla partenza autorizzerebbe a disegnare l'arrivo. Sono i due errori speculari che
	 * [D-223] esiste per chiudere: il **leak** (la polilinea entra nella nebbia) e la **contraddizione** (la
	 * traccia nascosta mentre il modello e' disegnato).
	 *
	 * ⚠️ **E il caso piu' comune non e' agli estremi**: `VisibleCells` e' un insieme *bucato* — LOS, cono
	 * frontale e close range — non un raggio, quindi il mezzo di un percorso puo' sparire mentre partenza e
	 * arrivo si vedono. Un verdetto per rotta non lo copre in nessuna delle sue forme.
	 *
	 * 🔴 **Non si consuma leggendo questo array**: la regola vive in `VisibleTrailFor`, che tronca. Chi
	 * ciclasse qui saltando le celle non ammesse disegnerebbe un segmento fra due celle non adiacenti, cioe'
	 * una linea che attraversa proprio il tratto da nascondere.
	 */
	UPROPERTY()
	TArray<FRTKnowledgeVerdict> CellVerdicts;
};

/**
 * Il filtro della traccia: puro, senza mondo, senza Actor.
 *
 * ## Perche' vive qui e non nel `TurnManager` (`#1818`)
 *
 * Ci ha vissuto fino al 2026-09-03, e con esso `FRTMoveRoute`: `RTKnowledgeViewTests` includeva
 * `Turn/RTTurnManager.h` — **1 935 righe** — per una struct di tre campi e una funzione statica, senza
 * toccare l'Actor nemmeno una volta. E' il gemello di `URTCombatLogLibrary`, uscito per la stessa ragione
 * e nella stessa fetta.
 */
UCLASS()
class REFACTORTACTICS_API URTMoveRouteLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il tratto di una rotta che un osservatore puo' vedere disegnato: il PREFISSO che il verdetto ammette.
	 *
	 * E' il gemello di `URTCombatLogLibrary::ComposeVisibleLogLines` per il secondo canale che [D-223]
	 * congela, ed e' puro per la stessa ragione di `ARTHUD::ShouldDrawUnitOverlay`: `DrawHUD` non ha
	 * copertura headless, quindi la regola vive dove la si puo' interrogare senza montare un HUD ne' una
	 * partita.
	 *
	 * 🔴 **Tronca, non salta.** La rotta porta il tratto OSSERVATO e si interrompe dove l'osservatore ha
	 * perso il soggetto: *«ho visto questa parte del suo movimento»* e' l'unica frase vera in ogni caso.
	 * Saltare le celle non ammesse e riprendere piu' avanti disegnerebbe un segmento fra due celle non
	 * adiacenti — una linea tesa attraverso il tratto che si voleva nascondere, cioe' il leak in forma
	 * peggiore.
	 *
	 * ⚠️ **Fail-closed due volte**: un verdetto assente non ammette nessuno (`FRTKnowledgeVerdict` nasce a
	 * maschera vuota), e una rotta i cui verdetti non siano allineati alle celle non si disegna affatto —
	 * senza quel controllo, un `Cells.Add` futuro senza il verdetto corrispondente leggerebbe fuori array.
	 *
	 * ⚠️ Un tratto di UNA cella non produce nessun segmento a schermo, ed e' corretto: la cella di partenza
	 * era gia' osservata: disegnarne il punto non aggiunge nulla che l'osservatore non sapesse.
	 */
	static TArray<FRTCellId> VisibleTrailFor(const FRTMoveRoute& Route, int32 ObserverTeamId);
};
