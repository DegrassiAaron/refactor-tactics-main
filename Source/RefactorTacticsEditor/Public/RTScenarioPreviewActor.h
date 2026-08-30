#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RTScenarioPreviewActor.generated.h"

class UInstancedStaticMeshComponent;
struct FRTScenarioUnitView;

/**
 * I marcatori delle unita' dello scenario aperto, nel viewport d'editor. **Solo presentazione.**
 *
 * ## Perche' un actor a parte e non un componente di `ARTHexMapActor`
 *
 * `ARTHexMapActor` e' un actor di RUNTIME: possiede la mappa in partita e ne disegna celle, rilievo,
 * blocchi, bordi e superfici. Le unita' di uno **scenario in authoring** non sono una proprieta' della mappa
 * — sono il contenuto di un file JSON che il designer sta guardando — e appenderle li' significherebbe
 * aggiungere al runtime un canale che esiste solo per l'editor.
 *
 * ## 🔴 Transient, e la ragione e' che il livello non deve accorgersene
 *
 * Questo actor nasce con `RF_Transient` e fuori dall'outliner: **non entra nel livello, non lo sporca, non
 * si salva**. Un'anteprima che dirtyfica `L_DevSandbox` mette il designer davanti a una richiesta di
 * salvataggio per aver soltanto *guardato* uno scenario — e il salvataggio accettato per distrazione e' come
 * un'anteprima diventa contenuto.
 *
 * ## ⚠️ Il tag, e il difetto che evita
 *
 * L'anteprima posa anche un `ARTHexMapActor` transiente per la mappa dello scenario. `FindTargetMapActor`
 * (`RTHexEditorClick.cpp`) cerca *l'unico* `ARTHexMapActor` del mondo e **risponde `nullptr` quando ne trova
 * piu' d'uno**: senza precauzione, aprire un'anteprima disattiverebbe in silenzio i cinque tool di disegno
 * su una mappa d'autore aperta accanto. Gli actor dell'anteprima portano per questo `PreviewTag`, e quella
 * ricerca li salta.
 *
 * ⛔ Nessuna regola di gioco qui: dove sta un marcatore e verso dove punta lo decide
 * `RTScenarioViewport::MarkerTransform`, che e' puro e testato.
 */
UCLASS(NotPlaceable, Transient)
class ARTScenarioPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ARTScenarioPreviewActor();

	/**
	 * Il tag che marca gli actor dell'anteprima — **questo e anche l'`ARTHexMapActor` transiente**.
	 * Vive qui perche' chi lo deve leggere (`RTHexEditorClick`) non deve conoscere il sottosistema.
	 */
	static const FName PreviewTag;

	/**
	 * Posa un marcatore per ogni unita', al posto dei precedenti.
	 *
	 * `Origin`, `HexSize` e `LayerHeight` vengono da `ARTHexMapActor::GetHexContext`, che e' l'unico punto da
	 * cui passano le conversioni cella<->mondo: ricavarli altrimenti farebbe divergere l'anteprima dalla
	 * mappa che le sta sotto.
	 */
	void ShowUnits(const TArray<FRTScenarioUnitView>& Units,
		const FVector& Origin, float HexSize, float LayerHeight);

	/** Toglie ogni marcatore. Idempotente. */
	void ClearUnits();

	/** Quanti marcatori sono posati. E' cio' che un automation test puo' contare senza guardare a schermo. */
	int32 NumMarkers() const;

private:
	/** Corpo del marcatore: il cilindro di `ARTUnit`, non una forma nuova. */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Bodies;

	/** Anello a terra: distingue la squadra per RAGGIO, perche' il kit non ha ancora materiali (#1714). */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> TeamRings;

	/** Cuneo di orientamento: una FORMA davanti al corpo, non un colore — il facing si deve vedere. */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FacingWedges;
};
