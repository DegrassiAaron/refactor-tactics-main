#pragma once

#include "CoreMinimal.h"
#include "BaseBehaviors/BehaviorTargetInterfaces.h" // IHoverBehaviorTarget
#include "BaseTools/SingleClickTool.h"
#include "InteractiveToolBuilder.h"
#include "Map/RTCellId.h"

#include "RTHexLosTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

UCLASS()
class URTHexLosToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/**
 * Il pannello dell'ispettore. Tre righe, non una UI.
 *
 * `VisibleAnywhere` ovunque: **niente qui si edita**. Un campo modificabile suggerirebbe che l'ispettore
 * accetti un'origine scritta a mano, e la selezione e' del click.
 */
UCLASS()
class URTHexLosToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** L'origine, scelta con un click. Vuota finche' non si clicca. */
	UPROPERTY(VisibleAnywhere, Category = "LOS")
	FString Selected = TEXT("—");

	/** La cella sotto il cursore. */
	UPROPERTY(VisibleAnywhere, Category = "LOS")
	FString Hovered = TEXT("—");

	/** `CLEAR` · `BLOCKED` · `—`. */
	UPROPERTY(VisibleAnywhere, Category = "LOS")
	FString LOS = TEXT("—");

	/** La causa canonica col suo punto, o `unavailable`. Mai una ragione inventata. */
	UPROPERTY(VisibleAnywhere, Category = "LOS")
	FString Reason = TEXT("—");

	/** Il piano su cui la linea sta ragionando: quello del TIRATORE. */
	UPROPERTY(VisibleAnywhere, Category = "LOS")
	int32 Layer = 0;
};

/**
 * **L'ispettore della LOS canonica** (#1755): si clicca l'origine, si passa sopra un bersaglio, e il
 * pannello dice se la vista passa e — quando non passa — **perche'**.
 *
 * ## ⛔ Cosa questo tool NON fa, ed e' la sua unica ragione d'essere
 *
 * Non calcola una linea di vista. Chiama `URTHexVisionLibrary::DescribeLineOfSight`, che e' lo **stesso**
 * attraversamento da cui esce il `bool` che il gioco usa per decidere — bot, combat, criteri d'arena e
 * percezione. Un ispettore che rifacesse la query potrebbe divergere dall'autorita' proprio quando serve, e
 * un debug che mente e' peggio di un debug che manca.
 *
 * ⚠️ **La riga si verifica per ASSENZA, e va detto cosa deve mancare davvero.** Questo file include
 * `RTHexLibrary` — ma solo per `AxialToWorld`, cioe' per sapere DOVE disegnare il punto di blocco che
 * il runtime ha gia' scelto. Cio' che non deve comparire e' l'algoritmo: nessun `HexLine`, nessun
 * `URTHexCoverLibrary::BlocksTraversal`, nessuna lettura di `bBlocksLineOfSight`. Sono quelli i tre
 * nomi che, comparendo qui, farebbero nascere la seconda LOS.
 *
 * ## ⚠️ Il caveat che il pannello DEVE dichiarare
 *
 * La LOS **non guarda il layer del bersaglio**: la linea resta su quello del tiratore — *«da terra si spara
 * sotto un ponte, da un piano superiore si spara oltre le coperture basse»*. Su mappa multilivello un
 * verdetto senza il piano su cui e' stato deciso e' una lettura falsa, ed e' per questo che `Layer` e' una
 * riga del pannello e non un dettaglio.
 *
 * ## 🔴 Primo hover del progetto, e il guardrail che porta con se'
 *
 * I cinque tool esistenti sono tutti a click (`USingleClickTool` / `UClickDragTool`): questo e' il primo che
 * usa `UMouseHoverBehavior`. Il rischio che introduce e' dichiarato da #1755 — ⛔ *«nessun Tick; una query
 * LOS su hover puo' essere event-driven dal cambio della cella hoverata, non per-frame se nulla cambia»* —
 * e la difesa e' `RTHexLos::ShouldRequery`, che e' pura e testata: il mouse produce eventi mentre si muove
 * DENTRO la stessa cella, e senza quel filtro l'ispettore sarebbe una query per fotogramma travestita da
 * event-driven.
 */
UCLASS()
class URTHexLosTool : public USingleClickTool, public IHoverBehaviorTarget
{
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	// --- IHoverBehaviorTarget ------------------------------------------------------------------------
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	/** Quante query LOS sono state fatte da quando il tool esiste. E' cio' che dimostra che l'hover filtra. */
	int32 GetQueryCount() const { return QueryCount; }

private:
	ARTHexMapActor* FindTargetMapActor() const;

	/** Risolve la cella sotto il raggio e, se e' cambiata, richiede il verdetto. */
	void UpdateHoveredCell(const FInputDeviceRay& DevicePos);

	/** Riscrive le tre righe dal verdetto canonico. Non decide niente. */
	void RefreshReadout();

	UPROPERTY()
	TObjectPtr<URTHexLosToolProperties> Properties;

	UPROPERTY()
	TObjectPtr<UWorld> TargetWorld;

	FRTCellId OriginCell;
	bool bHasOrigin = false;
	FVector OriginWorld = FVector::ZeroVector;

	FRTCellId HoveredCell;
	bool bHasHovered = false;
	FVector HoveredWorld = FVector::ZeroVector;

	/** Il punto in cui la linea si ferma, per disegnarlo. Valido solo se il verdetto e' `BLOCKED`. */
	FVector BlockedWorld = FVector::ZeroVector;
	bool bHasBlockPoint = false;

	int32 QueryCount = 0;
};
