#pragma once

#include "CoreMinimal.h"
#include "BaseBehaviors/BehaviorTargetInterfaces.h" // IHoverBehaviorTarget
#include "BaseTools/SingleClickTool.h"
#include "InteractiveToolBuilder.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"

#include "RTHexProbeTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

UCLASS()
class URTHexProbeToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/**
 * Il pannello della sonda.
 *
 * ⚠️ **`HeroId` e' l'unico campo editabile, e non e' un dettaglio di comodo.** #711 chiede che profilo e
 * budget vengano da *«dati reali, non da costanti d'editor»*: qui si sceglie **quale eroe**, e il budget lo
 * DERIVA il catalogo. Un `Budget` scrivibile a mano avrebbe risposto alla domanda sbagliata — «dove
 * arriverebbe un'unita' con 7 punti» invece di «dove arriva Wraith».
 */
UCLASS()
class URTHexProbeToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** L'eroe del catalogo di cui si sonda il movimento. Un id sconosciuto e' dichiarato, non sostituito. */
	UPROPERTY(EditAnywhere, Category = "Sonda")
	FName HeroId;

	/** I punti movimento, **letti dal catalogo** (`URTHeroData::MovePoints`). Sola lettura di proposito. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	int32 Budget = 0;

	/** La cella di partenza, scelta con un click. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	FString Start = TEXT("—");

	/** Quante celle il ventaglio contiene, partenza inclusa. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	int32 Reachable = 0;

	/** La cella sotto il cursore. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	FString Hovered = TEXT("—");

	/** `2 / 4 MP` per una cella raggiungibile, `—` per una esclusa. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	FString Cost = TEXT("—");

	/** I passi del percorso in hover: celle meno la partenza. Non e' il costo. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	int32 Steps = 0;

	/** **Perche' quella cella no** — una frase per motivo, mai una che ne copra due. */
	UPROPERTY(VisibleAnywhere, Category = "Sonda")
	FString Reason = TEXT("—");
};

/**
 * **La sonda di movimento** (#711): si sceglie un eroe, si clicca una cella di partenza, e passando sopra le
 * celle si legge **dove arriva** con quel budget, **per quale percorso** — e quando non ci arriva, **perche'
 * no**.
 *
 * ## ⛔ Cosa questo tool NON fa, ed e' la sua unica ragione d'essere
 *
 * Non cerca percorsi. Chiama `URTHexSimLibrary::ReachableCells`, che e' lo **stesso** Dijkstra da cui esce il
 * ventaglio che il giocatore vede e che il resolver poi concede; il percorso in hover lo ricostruisce
 * risalendo `FromCell` — cioe' la risposta che quel Dijkstra ha gia' calcolato — e il motivo dell'esclusione
 * lo chiede a `ClassifyProbeCell`. ⛔ *«Nessun Dijkstra/A\* parallelo nell'Editor. Una seconda ricerca e' una
 * seconda risposta: il giorno in cui divergono, la sonda mente e nessun test se ne accorge.»*
 *
 * ## 🔑 Da dove viene il budget
 *
 * Da `URTHeroCatalogLibrary::GetHeroRoster()` → `URTHeroData::MovePoints`, che e' la fonte da cui lo prende
 * anche `ARTUnit` (`MoveRange = Hero->MovePoints`) e la stessa che il Composer usa in
 * `FRTScenarioDraft::GetReachableCells`. Non e' una stima: e' il valore.
 *
 * ## ⚠️ Cosa questa sonda NON rappresenta
 *
 * Il **primo passo di un turno**, da fermo, senza status. `Action.Root` azzera il budget e `Action.Slow` alza
 * il costo per cella (CP 4.7); una sonda d'editor non ha una partita da cui leggerli, e mostrarli senza
 * averli sarebbe una previsione, non una misura. Il ventaglio dopo un piano gia' scritto e' un'altra
 * domanda, e ha gia' la sua funzione — `ReachableCellsAfterPlan` (#877).
 *
 * ## 🔴 Il ventaglio non si invalida, perche' non si conserva
 *
 * #711 chiede che dopo una modifica di superficie, blocco o transizione il set sia ricalcolato — *«sulla
 * revisione dell'asset, non su un refresh a tempo»*. Qui il problema e' risolto a monte: **lo snapshot non
 * viene tenuto**, si ricostruisce a ogni domanda filtrata dal gate. Una cache avrebbe avuto bisogno di una
 * regola di invalidazione, e quella regola esiste gia' altrove (`IsSnapshotStale`): averne una seconda qui
 * sarebbe stato il difetto che questo tool esiste per non avere.
 *
 * ⚠️ **E c'e' un vincolo che lo impone comunque**: `FRTHexSnapshot` porta un puntatore alla mappa che NON e'
 * una `UPROPERTY`, e il suo owner dichiara che non va conservata oltre la fase che la produce. Un tool che se
 * la tenesse fra un hover e l'altro terrebbe un puntatore che il GC non conosce.
 */
UCLASS()
class URTHexProbeTool : public USingleClickTool, public IHoverBehaviorTarget
{
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	// --- IHoverBehaviorTarget ------------------------------------------------------------------------
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	/** Quante classificazioni sono state fatte da quando il tool esiste. E' cio' che dimostra che l'hover filtra. */
	int32 GetQueryCount() const { return QueryCount; }

private:
	ARTHexMapActor* FindTargetMapActor() const;

	/**
	 * Rilegge il budget dal catalogo e lo MEMORIZZA in `Properties->Budget`.
	 *
	 * 🔴 **Si chiama quando l'eroe CAMBIA, mai a ogni hover.** `GetHeroRoster()` costruisce il roster da
	 * zero — quattro `URTHeroData` piu' una `NewObject` per ciascuna delle loro azioni — e la prima stesura
	 * di questo tool lo pagava **due volte per cella sorvolata**, dentro `MakeProbeSnapshot`. Il game thread
	 * si saturava e l'Editor smetteva di seguire il mouse.
	 */
	void RefreshBudgetFromCatalog();

	/**
	 * Lo snapshot della mappa con la sola unita' sondata. Costruito e consumato nella stessa chiamata: non
	 * esce di qui, e non diventa un membro (vedi il perche' esteso sulla classe).
	 */
	FRTHexSnapshot MakeProbeSnapshot(const URTHexMapAsset* Map) const;

	/**
	 * Rifa' il ventaglio da uno snapshot GIA' COSTRUITO. Nessuna decisione: una chiamata al runtime.
	 *
	 * ⚠️ Prende lo snapshot invece di costruirselo perche' `MakeSnapshot` ricalcola l'hash dell'intera
	 * mappa: farlo due volte per evento era la meta' dell'altro spreco.
	 */
	void RebuildReachableSet(const FRTHexSnapshot& Snapshot);

	/** Risolve la cella sotto il raggio e, se e' cambiata, riclassifica. */
	void UpdateHoveredCell(const FInputDeviceRay& DevicePos);

	/** Riscrive le righe dal verdetto canonico. Non decide niente. */
	void RefreshReadout();

	UPROPERTY()
	TObjectPtr<URTHexProbeToolProperties> Properties;

	UPROPERTY()
	TObjectPtr<UWorld> TargetWorld;

	FRTCellId StartCell;
	bool bHasStart = false;
	FVector StartWorld = FVector::ZeroVector;

	FRTCellId HoveredCell;
	bool bHasHovered = false;

	/**
	 * Il ventaglio corrente. Sono celle e costi — nessun puntatore — quindi tenerlo fra un evento e l'altro
	 * e' sicuro, al contrario dello snapshot da cui esce.
	 */
	TArray<FRTHexReachableCell> ReachableSet;

	/** Il percorso mostrato, in coordinate mondo. Vuoto se la cella sorvolata non si raggiunge. */
	TArray<FVector> HoverPathWorld;

	int32 QueryCount = 0;

	/**
	 * L'eroe corrisponde a uno del catalogo? Deriva da `RefreshBudgetFromCatalog` e viaggia fino al readout:
	 * un `HeroId` sbagliato non e' «budget zero», ed e' il pannello che deve dirlo.
	 */
	bool bKnownHero = false;
};
