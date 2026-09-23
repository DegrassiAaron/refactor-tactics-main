#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RTHexWorkGridActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
struct FRTCellId;

/**
 * IL PORTATORE della griglia di lavoro nel viewport d'editor (#622). **Solo presentazione.**
 *
 * ## Perche' un actor a parte, e non un componente di `ARTHexMapActor`
 *
 * ⛔ **Un `CreateDefaultSubobject` su `ARTHexMapActor` si serializzerebbe nel `.umap`** — lo dichiara il
 * codice stesso a proposito di `KnowledgeVolumes` (`RTHexMapActor.cpp:1815-1819`: *«una `.umap` salvata con
 * istanze non avrebbe piu' nulla che le pulisca»*). Il terzo criterio del DoD di questa issue e'
 * letteralmente *«nessuna geometria salvata nel `.umap`»*, quindi la vista non puo' vivere li'.
 *
 * ⛔ **E non c'e' spazio comunque**: `ERTRebuildFamily` e' un `uint8` con gli otto bit gia' assegnati
 * (`RTHexMapActor.h:86-108`). Una nona famiglia non e' una riga, e' un cambio di tipo su una maschera che
 * gira anche in partita — per una vista che esiste solo in editor.
 *
 * 🔑 **Il precedente in casa e' `ARTScenarioPreviewActor`**, e questo actor ne ripete la forma per intero:
 * `RF_Transient`, fuori dall'outliner, `bTemporaryEditorActor`, ISM senza collisione ne' ombre, nessun
 * tick. Non entra nel livello, non lo sporca, non si salva.
 *
 * ## ⚠️ Nessuna collisione, e non e' un'ottimizzazione
 *
 * `RTHexEditor::ResolveClickedCell` spara un `LineTraceSingleByObjectType` su `AllObjects` e poi filtra con
 * `IsPickOnSelectableCell` (`RTHexEditorClick.cpp:99-113`). Un ISM collidibile non ruberebbe il click con
 * un errore: lo farebbe **ripiegare in silenzio**, e il pennello scriverebbe altrove. E' lo stesso difetto
 * che #588 ha chiuso guardando il *componente* invece dell'actor.
 *
 * ⛔ Nessuna regola qui dentro: **quali** coordinate marcare lo decide `RTHexWorkGrid::BuildPlan`, che e'
 * puro e provato headless. Questo actor riceve un elenco e lo posa.
 */
UCLASS(NotPlaceable, Transient)
class ARTHexWorkGridActor : public AActor
{
	GENERATED_BODY()

public:
	ARTHexWorkGridActor();

	/**
	 * Il tag che marca questo actor.
	 *
	 * ⚠️ **Non serve a `FindTargetMapActor`** — quella cerca `ARTHexMapActor` e questo non lo e', quindi
	 * l'iteratore non lo incontra nemmeno. Serve a chi guarda: un actor senza nome nel mondo, invisibile
	 * nell'outliner, e' esattamente il tipo di cosa che qualcuno un giorno cerchera' di capire.
	 */
	static const FName WorkGridTag;

	/**
	 * Posa un anello fantasma per ogni coordinata, al posto dei precedenti.
	 *
	 * `Origin`, `HexSize` e `LayerHeight` vengono da `ARTHexMapActor::GetHexContext`, che e' l'unico punto
	 * da cui passano le conversioni cella↔mondo: ricavarli altrimenti farebbe divergere la griglia dalla
	 * mappa che le sta sotto.
	 */
	void ShowCells(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize, float LayerHeight);

	/** Toglie ogni anello. Idempotente. */
	void ClearCells();

	/**
	 * Quanti anelli sono posati.
	 *
	 * ⚠️ Si legge dallo **stato reale delle istanze** e non da un contatore a parte: un contatore
	 * proverebbe che la funzione sa contare, non che ha disegnato. E' la stessa scelta di
	 * `ARTScenarioPreviewActor::NumBorderPanels`.
	 */
	int32 NumGhosts() const;

private:
	/**
	 * Tinta dell'anello fantasma.
	 *
	 * ⚠️ **Default `M_HexCell`, ed e' un PRESTITO dichiarato** — l'unico materiale versionato del progetto
	 * che legge i `PerInstanceCustomData` per istanza, come Emissive (`RTHexMapActor.h:236-247`). Il nome
	 * parla della griglia di gioco e non della griglia di lavoro: se un giorno questa vorra' una resa
	 * propria, la sostituzione e' **questa riga**.
	 *
	 * 🔴 **Il riuso su un ISM che non e' della board e' un rischio che `D-183` dichiara NON verificato**
	 * (`RTScenarioPreviewActor.h:109-113`). Se a schermo l'anello non si tinge, il degrado e' **silenzioso**
	 * — grigio, non un errore. Lo dice la seduta `PIE-MAPED-GRID`, non un test.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|WorkGrid")
	TSoftObjectPtr<UMaterialInterface> GhostMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Core/Grid/M_HexCell.M_HexCell")));

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Ghosts;

	/**
	 * L'anello esagonale fantasma, costruito in codice e cachato per processo.
	 *
	 * ⚠️ **Non chiamabile dal costruttore del CDO**: crea una `UObject`. La mesh si assegna alla prima posa,
	 * come `ARTHexMapActor` fa nel proprio `RebuildInstances` (`RTHexMapActor.cpp:750-753`).
	 */
	static UStaticMesh* GetWorkGridRingMesh();
};
