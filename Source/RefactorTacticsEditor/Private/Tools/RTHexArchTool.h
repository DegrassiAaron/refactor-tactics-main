#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexTransitionKind
#include "RTHexArchTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;
class UTransformProxy;
class UCombinedTransformGizmo;

/** Operazione del tool archi: crea (gizmo) o rimuove (click sull'arco). */
UENUM()
enum class ERTHexArchOp : uint8
{
	Add,
	Remove
};

/**
 * Perche' un arco pendente e' stato chiuso — `#996`.
 *
 * Le cinque voci sono le cinque chiamanti di `DestroyPendingGizmo`, e servono a una domanda sola: quando un
 * gizmo sparisce, e' stato chiuso da noi o e' sparito da solo? Senza il motivo, il log direbbe *che* e'
 * successo e non *perche'*, che e' esattamente l'ambiguita' che la issue esiste per sciogliere.
 *
 * ⚠️ **Non c'e' un valore di default, ed e' deliberato**: il parametro senza default costringe una sesta
 * chiamante — se un giorno nascera' — a dichiarare la propria ragione invece di scivolare dentro un
 * `Unknown` che nessuno noterebbe leggendo il log.
 */
enum class ERTArchPendingClose : uint8
{
	/** `OnClicked`: si riparte da una nuova cella `From`. */
	ReClick,
	/** Il tool termina: cambio tool, oppure uscita dal mode. */
	Shutdown,
	/** `CommitArch`: la transizione e' stata scritta, il pendente ha finito il suo compito. */
	Committed,
	/** `ClearPending`: il bottone `ClearArch` del pannello. */
	ClearedByUser,
	/** `RemoveNearestArch`: si esce da un Add pendente perche' l'operazione e' passata a Remove. */
	SwitchedToRemove
};

/** Factory del tool archi. */
UCLASS()
class URTHexArchToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Palette del tool archi: parametri della transizione + readout. (Bottoni Commit/Clear aggiunti in H5c.2b.) */
UCLASS(Transient)
class URTHexArchToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	ERTHexArchOp Operation = ERTHexArchOp::Add;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco", meta = (ClampMin = "0"))
	int32 Cost = 2;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	bool bBidirectional = true;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId From;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bHasFrom = false;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId To;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bToValid = false;

	/** Back-pointer al tool per i bottoni (impostato in Setup). */
	TWeakObjectPtr<class URTHexArchTool> WeakTool;

	/** Crea la transizione From->To con i parametri correnti. */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void Commit();

	/** Annulla l'arco pendente (nessuna scrittura). */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void ClearArch();
};

/**
 * Crea transizioni (FRTHexEdge) nel viewport: click su From, gizmo per To (H5c.2b). In H5c.2a disegna solo le
 * transizioni esistenti dell'asset (rese visibili per la prima volta). Non modifica dati in questa fetta.
 */
UCLASS()
class URTHexArchTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	void CommitArch();
	void ClearPending();

protected:
	void OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform);

	/** Chiude l'arco pendente e ne scrive il motivo nel log (`#996`). Il motivo e' obbligatorio: vedi l'enum. */
	void DestroyPendingGizmo(ERTArchPendingClose Reason);

	/** Rimuove la transizione la cui linea From->To e' piu' vicina al ray del click, entro soglia. */
	void RemoveNearestArch(ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos);

	UPROPERTY()
	TObjectPtr<UTransformProxy> Proxy;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo;

	UPROPERTY()
	TObjectPtr<ARTHexMapActor> TargetActor = nullptr;

	FRTCellId From;
	FRTCellId To;
	bool bHasFrom = false;
	bool bToValid = false;
	bool bSnapping = false;
	FVector FromWorld = FVector::ZeroVector;
	FVector ToWorld = FVector::ZeroVector;
	float MarkerRadius = 90.f;

	UPROPERTY()
	TObjectPtr<URTHexArchToolProperties> Properties;

	UWorld* TargetWorld = nullptr;
};
