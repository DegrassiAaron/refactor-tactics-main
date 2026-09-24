#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h" // ERTHexSurface, ERTHexTransitionKind
#include "Map/RTGeometryGrammar.h" // ERTAnchorPairRefusal: la domanda «e' un click?» la risponde la grammatica

class UWorld;
class ARTHexMapActor;
class FPrimitiveDrawInterface;
class UContextObjectStore;
class UInteractiveToolManager;
struct FInputDeviceRay;
struct FRTMapElementHandle;
enum class ERTHexSurface : uint8;

/** Helper condivisi tra i tool click dell'Editor Mode hex (SelectTool, PaintTool, ...). */
namespace RTHexEditor
{
	/** ARTHexMapActor bersaglio: selezionato nel Level Editor; altrimenti l'unico presente; altrimenti nullptr (ambiguo). */
	ARTHexMapActor* FindTargetMapActor(UWorld* World);

	/** Risolve la cella cliccata sul layer attivo dell'actor (raycast ISM del target, fallback piano del layer).
	 *  Ritorna false se Actor è nullptr. OutCenter = centro-mondo della cella (per il marker). */
	/**
	 * ⚠️ `OutClickedPoint` (opzionale) e' il punto su cui si e' cliccato, non il centro della cella: serve a
	 * `URTHexLibrary::NearestEdgeDirection` per sapere QUALE BORDO l'autore stava mirando (#1864).
	 *
	 * Quando il raycast non ha un colpo utile e si ripiega sul piano del layer, riceve il **centro cella** —
	 * il che rende il bordo scelto arbitrario ma deterministico. E' dichiarato invece che nascosto: senza un
	 * colpo, «quale lato» e' una domanda a cui la geometria non puo' rispondere.
	 */
	bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
		FRTCellId& OutCell, FVector& OutCenter, FVector* OutClickedPoint = nullptr);

	/**
	 * Scrive il layer attivo sull'actor e ne ricostruisce la vista. Transazionale (annullabile) e idempotente:
	 * se il valore non cambia non apre nessuna transazione e ritorna false.
	 *
	 * Esiste perche' i tool possono cambiare il piano dal proprio pannello: assegnare `ActiveLayer` da codice
	 * non passa da `PostEditChangeProperty`, quindi senza la ricostruzione esplicita la vista resterebbe ferma
	 * sul piano precedente.
	 */
	bool SetActiveLayer(ARTHexMapActor* Actor, int32 NewLayer);

	/** Disegna un esagono pointy-top (marker) sul PDI. */
	void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color,
		float Thickness = 2.0f);

	/** Colore d'overlay per una superficie cella (presentazione editor). */
	FColor SurfaceColor(ERTHexSurface Surface);

	/**
	 * Overlay debug: ogni cella dell'asset come esagono colorato per superficie; le bloccate con un esagono
	 * rosso interno, piu' i cerchi delle celle irraggiungibili e le celle di partenza.
	 *
	 * ⌫ **Non disegna piu' le transizioni, e il parametro `bIncludeTransitions` non esiste piu'** (#1768).
	 * Stavano qui dal 2026-09-21 (#921), che le aveva portate fuori dal solo tool Arch — ma sotto il toggle
	 * `bShowSurfaceOverlay`, e il tool Arch continuava a disegnarle per conto proprio **incondizionatamente**
	 * per non vederle doppie. 🔴 **Conseguenza misurata: a overlay SPENTO le transizioni tornavano a
	 * vedersi col solo Arch**, cioe' il difetto che #1768 esiste per chiudere sopravviveva intatto in quella
	 * condizione. Ora hanno un canale proprio, `DrawTransitions`, che non dipende da nessun toggle.
	 */
	void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor);

	/**
	 * Le TRANSIZIONI, con qualunque strumento del mode sia attivo (#1768).
	 *
	 * 🔑 **Sono l'unico modo in cui due layer si collegano**, e una piattaforma senza arco e'
	 * irraggiungibile senza dirlo. Per questo non stanno sotto `bShowSurfaceOverlay`: quel toggle spegne i
	 * marcatori di superficie, che sono una preferenza di chi dipinge; un arco assente dallo schermo e' una
	 * mappa che mente per omissione.
	 *
	 * ⛔ **Due canali per il tipo e due per lo stato**, e non e' decorazione: `D-146` scrive *«l'encoding e'
	 * ridondante: mai solo il colore»*, e le sei tinte non reggono la scala di grigi — `Tunnel` ed `Elevator`
	 * distano `1.9` di luminanza su `255`. Quali siano i canali lo decide `RTHexTransition::Describe`, che e'
	 * puro e provato headless; qui si disegna soltanto.
	 */
	void DrawTransitions(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor);

	/**
	 * L'overlay delle superfici e' acceso? (#921) — la domanda che ciascuno dei sette `Render` pone, e la
	 * risposta che **nessuno di loro decide da se'**.
	 *
	 * 🔑 **Questa e' la sede dell'AC 8, ed e' il motivo per cui la funzione esiste invece di essere tre righe
	 * ripetute.** Il flag e' un `URTHexEditorModeSettings` che `URTHexEditorMode::Enter` pubblica nel context
	 * store; un tool costruito fuori dal mode — o interrogato prima che il mode sia entrato — non lo trova.
	 * Senza una sede unica, ognuno dei sette sceglierebbe da se' cosa fare in quel caso, e la scelta si
	 * scoprirebbe **a schermo**.
	 *
	 * ⛔ **Assente significa SPENTO, non acceso.** Un overlay che comparisse da solo dove il mode non c'e'
	 * disegnerebbe sopra il lavoro di chi non l'ha chiesto; uno che resta spento si accende con un click.
	 * L'errore reversibile e' il secondo.
	 */
	bool ShouldShowSurfaceOverlay(const UContextObjectStore* Store);

	/** Come sopra, partendo dal tool manager — la via che un `Render` ha davvero sottomano. */
	bool ShouldShowSurfaceOverlay(const UInteractiveToolManager* ToolManager);

	/**
	 * Mette il settings del mode nello store **da cui `ShouldShowSurfaceOverlay` lo rileggera'** (#921).
	 *
	 * 🔑 **Esiste per tenere le due meta' nello stesso file.** La pubblicazione avviene in
	 * `URTHexEditorMode::Enter()` e la lettura dentro sette `Render`: sono sedi lontane, e nulla nel
	 * compilatore obbliga il tipo pubblicato a essere quello cercato. Passando di qui, l'accoppiamento e'
	 * scritto una volta e un test lo esercita — mentre due chiamate dirette a `AddContextObject` e
	 * `FindContext` potrebbero divergere in silenzio, e il difetto si vedrebbe solo a schermo.
	 *
	 * Restituisce `false` — e non pubblica nulla — se lo store manca o se l'oggetto non e' del tipo che il
	 * lettore cerca. ⚠️ **Rifiutare invece di accettare e' deliberato**: un settings del tipo sbagliato
	 * pubblicato «con successo» darebbe un overlay morto senza che niente lo dica.
	 */
	bool PublishSurfaceOverlaySettings(UContextObjectStore* Store, UObject* Settings);

	/** Toglie dallo store cio' che `PublishSurfaceOverlaySettings` ha messo. */
	void WithdrawSurfaceOverlaySettings(UContextObjectStore* Store, UObject* Settings);

	/**
	 * Colore di una transizione per tipo, e freccia From->To. Stanno qui e non nel tool Arch perche' ora hanno
	 * DUE consumatori: il tool, che le disegna mentre le si crea, e l'overlay, che le mostra sempre. Due
	 * definizioni dello stesso vocabolario visivo prima o poi divergono — stessa ragione per cui la tavolozza
	 * delle superfici e' una sola.
	 */
	FColor TransitionKindColor(ERTHexTransitionKind Kind);

	/**
	 * E' il momento di rifare la VALIDAZIONE della mappa? (#1864, casella 8)
	 *
	 * 🔴 **Esiste perche' validare a ogni cambiamento sarebbe validare a ogni fotogramma.**
	 * `URTHexMapAsset::ValidateMap()` chiama in coda `ValidateMapDetailed`, che per OGNI cella esegue
	 * `ComputeMask`, `HasLegalPlacement` ed `EnumerateCoverOptions` — il lavoro geometrico della
	 * cottura dell'intera mappa. E `URTHexEditorMode::ModeTick` gira una volta per fotogramma.
	 *
	 * ⚠️ La guardia su `Revision` che il readout usa da `#1186` **non basta per questo**: durante un
	 * trascinamento del pennello la mappa cambia a ogni fotogramma, quindi quella guardia lascia passare
	 * tutto. Un readout che rallenta il gesto che descrive e' peggio di nessun readout.
	 *
	 * 🔑 **La regola e' «un tick di quiete»**: finche' la revisione si muove si aspetta; il primo
	 * fotogramma in cui non si e' mossa, e c'e' lavoro in attesa, si valida **una volta sola**. Durante
	 * una pennellata le validazioni sono zero, e al rilascio una.
	 *
	 * ⛔ **Non usa il tempo**: un debounce a millisecondi renderebbe il numero di validazioni dipendente
	 * dal frame rate, cioe' dalla macchina.
	 *
	 * Pura, con lo stato passato per riferimento: si prova headless senza aprire un `UEdMode`.
	 */
	bool ShouldRevalidate(int32 CurrentRevision, int32& InOutLastSeen, bool& InOutPending);

	/**
	 * QUESTO GESTO E' UNA SELEZIONE, o un disegno che non e' riuscito? (#1864, casella 2)
	 *
	 * 🔑 **Il tool Geometry disegnava e basta: vedeva la selezione condivisa e non poteva scriverla.**
	 * La casella 2 chiede che sia condivisa *fra* Select, Geometry e Arch, e Geometry era un consumatore —
	 * chiamava `DrawSharedSelection` e nient'altro. Questa funzione e' la regola che gli manca.
	 *
	 * ⛔ **Il gesto che oggi non produce NULLA e' quello libero, ed e' l'unico che si puo' prendere.**
	 * `OnClickRelease` esce senza toccare la mappa quando lo snap non ha prodotto un segmento; fra i modi
	 * in cui puo' non produrlo, uno solo significa *«non stavo disegnando»*:
	 *
	 * ```text
	 * SameAnchor        i due estremi sono lo STESSO anchor -> non c'e' lunghezza: e' un CLICK
	 * DifferentCell     il trascinamento ha attraversato due celle  -> stava disegnando, e ha sbagliato
	 * DifferentLayer    idem, su due piani                          -> stava disegnando
	 * NoAxis            le ventiquattro coppie inesprimibili        -> stava disegnando
	 * ```
	 *
	 * ⚠️ **Prendere anche gli altri sarebbe rubare il gesto al disegno**: chi trascina fra due anchor che
	 * nessun asse congiunge sta disegnando, e vedersi cambiare la selezione al rilascio e' peggio che non
	 * vedere niente — perche' il ghost gli aveva gia' detto che il muro non si puo' fare.
	 *
	 * 🔴 **E la misura e' quella della GRAMMATICA, non dei pixel.** L'engine offre
	 * `USingleClickOrDragInputBehavior`, che distingue click e trascinamento con
	 * `ClickDistanceThreshold = 5.0` **in pixel di schermo**: i pixel dipendono dalla camera, gli anchor
	 * no. Allo zoom sbagliato il muro piu' corto esprimibile smetterebbe di essere disegnabile, e lo stesso
	 * gesto sulla stessa mappa cambierebbe esito a seconda di dove sta l'occhio. Qui la domanda
	 * *«i due estremi sono lo stesso punto della grammatica?»* la risponde gia' il runtime
	 * (`URTGeometryGrammarLibrary::ExplainPair`), ed e' invariante allo zoom.
	 *
	 * Pura: si prova headless senza aprire un `UEdMode`.
	 */
	bool GestureIsASelection(ERTAnchorPairRefusal Refusal);

	/**
	 * Applica alla selezione condivisa il click su una cella — la META' che Select e Geometry hanno in
	 * comune (#1864, casella 2).
	 *
	 * 🔑 **Estratta invece che copiata.** Il gesto e' lo stesso in entrambi i tool — risolvere il bordo
	 * mirato, leggere `Ctrl`, aggiungere o sostituire, e aggiornare il readout — e due stesure della stessa
	 * regola divergono: e' la ragione per cui `DrawSelectedElement` era gia' salita qui da
	 * `URTHexSelectTool`, e vale identica per la scrittura.
	 *
	 * ⚠️ **`Ctrl` si legge QUI**, cosi' i due tool non possono avere due convenzioni: con il modificatore
	 * si accumula, senza si sostituisce. E' la stessa lettura che Select faceva da solo.
	 *
	 * ⛔ Non e' provabile headless — passa da `GEditor->GetEditorSubsystem` — ed e' dichiarato: cio' che
	 * si poteva rendere puro e' `GestureIsASelection`, che decide *se* arrivare qui. Che i due tool ci
	 * arrivino davvero lo dice una seduta, ed e' `PIE-MAPED-SEL-CONDIVISA`.
	 *
	 * `false` se manca lo store o la mappa: per un chiamante e' un esito normale, non un errore.
	 */
	bool ApplyClickToSelection(const ARTHexMapActor* Actor, const FRTCellId& Cell,
		const FVector& ClickedPoint);

	void DrawArrow(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B, const FColor& Color);

	/**
	 * L'arco di transizione piu' vicino al raggio del click, come **handle** (#1864).
	 *
	 * 🔑 **Il hit-test di un arco e' di VIEWPORT, e la spec §13.3 lo assegna al tool**: un arco
	 * collega celle su layer diversi e non giace su un bordo, quindi `URTMapEditLibrary::ElementsAt` —
	 * che risponde a «che cosa c'e' sotto questo bordo» — non lo raggiunge e non deve fingere di
	 * farlo. Questa funzione e' quel test, estratto da `URTHexArchTool::RemoveNearestArch` dove viveva
	 * privato, perche' ora serve anche a **selezionare** e non solo a cancellare.
	 *
	 * ⚠️ La soglia e' `HexSize * 0.6`, la stessa che il tool Arch usa da sempre: non e' un numero
	 * nuovo, e' quello di prima messo dove lo possono leggere in due.
	 *
	 * `false` se non c'e' nessun arco entro la soglia. `OutDistance` e' facoltativo e serve al log.
	 */
	bool NearestTransition(const ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
		FRTMapElementHandle& OutHandle, float* OutDistance = nullptr);

	/**
	 * Disegna UN elemento selezionato.
	 *
	 * 🔑 **Viveva privato in `URTHexSelectTool` fino al 2026-09-23**, ed e' salito qui perche' il
	 * criterio di #1864 chiede che la selezione sia condivisa *fra* Select, Geometry e Arch: una selezione
	 * che solo lo strumento che l'ha fatta sa disegnare non e' condivisa, e' passata di mano.
	 */
	void DrawSelectedElement(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor,
		const FRTMapElementHandle& Handle, const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * Disegna l'INTERA selezione condivisa, leggendola dal suo store.
	 *
	 * ⚠️ Chiamarla dal `Render` di un tool e' cio' che rende la selezione visibile cambiando
	 * strumento — che e' il punto per cui lo store vive fuori dai `UInteractiveToolPropertySet`
	 * (§13.3, e il difetto che #921 aveva misurato).
	 */
	void DrawSharedSelection(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor);
}
