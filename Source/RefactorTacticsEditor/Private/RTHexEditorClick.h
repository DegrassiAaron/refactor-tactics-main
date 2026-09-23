#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h" // ERTHexSurface, ERTHexTransitionKind

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
	 * rosso interno. Disegna **anche** le transizioni e i cerchi delle celle irraggiungibili.
	 *
	 * 🔴 **`bIncludeTransitions = false` serve al solo tool Arch, e non e' un'opzione di gusto.** `URTHexArchTool`
	 * disegna gia' le transizioni per conto proprio, e **incondizionatamente** — e' cio' che `PIE-HEX-MODE-F`
	 * ha verificato ✅ e che #921 dichiara fuori scope. Con l'overlay acceso le stesse frecce arriverebbero da
	 * due sorgenti: una a quota cella dal tool, una a `+4` in Z da qui, stesso colore. Si vedrebbero doppie e
	 * sfalsate, e in `LayerView = ActiveOnly` sarebbero anche **incoerenti**, perche' l'overlay filtra per
	 * layer attivo e il ciclo del tool no.
	 */
	void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor,
		bool bIncludeTransitions = true);

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
