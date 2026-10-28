#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "InputState.h" // FInputDeviceRay / FInputRayHit
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h" // HexArea
#include "Terrain/RTTerrainLibrary.h"

#define LOCTEXT_NAMESPACE "URTHexPaintTool"

#if WITH_EDITOR
void URTHexPaintToolProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(URTHexPaintToolProperties, Surface))
	{
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Surface);
		MoveCost = Def.MoveCost;
		bBlocksMovement = false; // nessun terreno del catalogo v0.1 blocca il movimento normale
	}
}
#endif

UInteractiveTool* URTHexPaintToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexPaintTool* NewTool = NewObject<URTHexPaintTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexPaintTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexPaintTool::Setup()
{
	UClickDragTool::Setup();
	Properties = NewObject<URTHexPaintToolProperties>(this);

	// Il pannello parte dal piano su cui l'actor e' gia' impostato: senza questo mostrerebbe 0 finche' non si
	// dipinge, e il primo cambio dal pannello sposterebbe il pennello su un piano che l'utente non ha scelto.
	if (const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld))
	{
		Properties->ActiveLayer = Actor->ActiveLayer;
	}

	AddToolPropertySource(Properties);
}

void URTHexPaintTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (!Properties || PropertySet != Properties || !Property) { return; }
	if (Property->GetFName() != GET_MEMBER_NAME_CHECKED(URTHexPaintToolProperties, ActiveLayer)) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		Properties->ActiveLayer = 0;
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio: layer attivo non applicato."));
		return;
	}
	RTHexEditor::SetActiveLayer(Actor, Properties->ActiveLayer);
}

FInputRayHit URTHexPaintTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// Accetta ogni click nel viewport (la cella si risolve in OnClickPress). bHit=true via profondità.
	return FInputRayHit(TNumericLimits<double>::Max());
}

/**
 * Le famiglie di istanze che una pennellata puo' cambiare — `#1865`, punto 3.
 *
 * 🔑 **Il pennello scrive quattro campi e nessun altro**: `Surface`, `MoveCost`, `bBlocksMovement`,
 * `bBlocksLineOfSight`. La matrice campo→famiglia dice quali toccano — `Cells` (colore), `Glyphs`
 * (`SurfaceRingCount`), `Relief` (`ReliefHeightForCost`), `Blockers` — piu' `Borders`, perche' una
 * pennellata puo' CREARE una cella e ogni cella nuova porta il suo contorno.
 *
 * ⛔ **`EdgeFeatures` resta fuori**: dipende da `Covers`, `Doors` e muri interni, che il pennello non
 * scrive mai — e una cella appena creata li ha ai valori di default, quindi non c'e' niente da posare.
 *
 * 🔴 **`Bodies` NON resta fuori** (`#2732`), anche se il pennello non scrive `BodyFill`: il corpo di una
 * cella dipende da cio' che le sta SOTTO nella stessa colonna — `URTStructuralBodyLibrary::DeriveBodies` lo
 * tronca sulla faccia superiore della cella sottostante. Una pennellata che CREA una cella sotto un corpo ne
 * cambia il `BottomZ`, e senza questa famiglia il corpo mantiene la vecchia estensione e attraversa
 * visibilmente la cella appena dipinta per tutta la pennellata.
 *
 * 🔴 **Nemmeno `SurfaceVolumes`**, per la ragione opposta: qui il campo lo scrive il pennello. Il volume con
 * cui una superficie si dichiara nello spazio lo decide `URTHexLibrary::SurfaceVolumeFor(Surface)`, e
 * `Surface` e' uno dei quattro campi. Il fumo dipinto colorava la cella e posava il glifo, ma non il volume.
 *
 * ⚠️ **Entrambi sono incoerenze del TRASCINAMENTO, non dati sbagliati.** `EndStrokeIfActive` ricostruisce
 * tutto al rilascio (`RebuildInstances()` senza maschera), quindi il corpo stantio e il volume mancante
 * durano fino al rilascio e poi si riallineano. Una stesura precedente di questo commento diceva «fino a una
 * gomma o a un cambio d'asset», ereditata dal testo della issue, ed era falsa. Il prezzo e' il lavoro in piu'
 * sul percorso piu' caldo dell'authoring — `DeriveBodies` copia e ordina tutte le celle a ogni chiamata e, per
 * ogni cella che dichiara un corpo, le scorre di nuovo: costo B per N, con B le celle con corpo e N le totali —
 * e **non e' stato misurato**: lo si paga per non mostrare, durante il trascinamento, un corpo che attraversa
 * il pavimento appena dipinto.
 *
 * ⛔ **L'ERASE non usa questa maschera, ed e' la ragione per cui la funzione prende `bPaint`**:
 * cancellare una cella la toglie da OGNI famiglia, comprese quelle che il pennello non scrive. Una cella
 * con coperture lascerebbe pannelli orfani su un pavimento che non c'e' piu'.
 */
ERTRebuildFamily URTHexPaintTool::BrushRebuildMask(bool bPaint)
{
	return bPaint
		? (ERTRebuildFamily::Cells | ERTRebuildFamily::Glyphs | ERTRebuildFamily::Relief
			| ERTRebuildFamily::Blockers | ERTRebuildFamily::Borders | ERTRebuildFamily::Bodies
			| ERTRebuildFamily::SurfaceVolumes)
		: ERTRebuildFamily::All;
}

bool URTHexPaintTool::ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld)
{
	URTHexMapAsset* Map = Actor->MapAsset; // il caller garantisce Actor && Map non nulli
	const bool bPaint = (Properties->Operation == ERTHexPaintOp::Paint);
	const bool bCenterExistedBefore = (Map->FindCell(CenterCell) != nullptr); // per il readout, pre-mutazione

	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(CenterCell, FMath::Max(0, Properties->BrushRadius));
	bool bAnyChanged = false;
	for (const FRTCellId& C : Area)
	{
		if (PaintedThisStroke.Contains(C)) { continue; } // dedup per-cella nella pennellata
		// Apri la transazione/stroke LAZY solo se questa cella cambia davvero (paint cambia sempre; erase solo se esiste).
		if (bPaint || Map->ContainsCell(C))
		{
			EnsureStrokeOpen();
		}
		// Il flag della vista si passa SEMPRE: il pennello lo scrive come scrive `bBlocksMovement`, cosi'
		// dipingere con il flag spento su una cella che lo aveva lo toglie. Senza, il muro sarebbe
		// irreversibile dall'editor — il difetto opposto a quello che questa property risolve.
		int32 CascadeRemoved = 0;
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement,
				TOptional<bool>(Properties->bBlocksLineOfSight))
			: Map->EraseCellInStroke(C, &CascadeRemoved);
		PaintedThisStroke.Add(C);
		bAnyChanged = bAnyChanged || bApplied;
		if (!bPaint && bApplied)
		{
			++StrokeCellsErased;
			StrokeCascadeRemoved += CascadeRemoved;
		}
	}

	// Readout/marker sul centro.
	MarkerColor = bPaint ? FColor::Green : FColor::Red;
	Properties->bLastExisted = bCenterExistedBefore;
	Properties->LastCell = CenterCell;
	Properties->ActiveLayer = Actor->ActiveLayer;
	MarkerCenter = CenterWorld;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;
	return bAnyChanged;
}

void URTHexPaintTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Paint: nessun ARTHexMapActor con MapAsset (nessuna pennellata)."));
		return; // niente stroke senza asset (M3)
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, PressPos, Cell, Center)) { return; }

	// La transazione + BeginStroke si aprono LAZY al primo cambiamento reale (EnsureStrokeOpen): niente transazione
	// no-op quando l'erase non tocca celle esistenti (garanzia ripristinata rispetto al click singolo di H5c.1).
	TargetActor = Actor;
	bStrokeActive = true;
	bStrokeOpened = false;
	PaintedThisStroke.Reset();
	StrokeCellsErased = 0;
	StrokeCascadeRemoved = 0;

	if (ApplyBrushAt(Actor, Cell, Center))
	{
		Actor->RebuildInstances(BrushRebuildMask(Properties->Operation == ERTHexPaintOp::Paint));
	}
}

void URTHexPaintTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bStrokeActive || !TargetActor || !TargetActor->MapAsset) { return; }

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, TargetActor, DragPos, Cell, Center)) { return; }

	if (ApplyBrushAt(TargetActor, Cell, Center))
	{
		// Il punto piu' caldo del percorso di authoring: gira a OGNI cella attraversata dal trascinamento.
		TargetActor->RebuildInstances(BrushRebuildMask(Properties->Operation == ERTHexPaintOp::Paint));
	}
}

void URTHexPaintTool::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	EndStrokeIfActive();
}

void URTHexPaintTool::OnTerminateDragSequence()
{
	EndStrokeIfActive();
}

void URTHexPaintTool::EndStrokeIfActive()
{
	if (bStrokeOpened && TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->EndStroke();
		TargetActor->RebuildInstances(); // riallinea InstanceCells all'ordine post-SortCells
		if (StrokeCellsErased > 0)
		{
			// Quante celle E quanti elementi la cascata ha portato via: un arco verso una cella fuori dall'area sparisce
			// con lei, e chi dipinge deve poterlo leggere (#3322).
			UE_LOG(LogTemp, Log,
				TEXT("[HexMode] Erase: %d celle cancellate, %d elementi portati via con esse (muri interni, archi, binding)."),
				StrokeCellsErased, StrokeCascadeRemoved);
		}
	}
	StrokeTransaction.Reset(); // chiude/commit la transazione (o no-op se non aperta)
	bStrokeActive = false;
	bStrokeOpened = false;
	TargetActor = nullptr;
	PaintedThisStroke.Reset();
}

void URTHexPaintTool::EnsureStrokeOpen()
{
	if (bStrokeOpened) { return; }
	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	if (TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->BeginStroke(); // Modify() dopo l'apertura della transazione: cattura lo stato pre-pennellata
	}
	bStrokeOpened = true;
}

void URTHexPaintTool::Shutdown(EToolShutdownType ShutdownType)
{
	EndStrokeIfActive(); // chiude uno stroke aperto a un cambio-tool/uscita mode a metà drag (S1)
	UClickDragTool::Shutdown(ShutdownType);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (RTHexEditor::ShouldShowSurfaceOverlay(GetToolManager()))
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}

	// 🔑 **Le transizioni, con QUALUNQUE strumento attivo e senza dipendere da un toggle** (#1768).
	// Fuori dal blocco qui sopra di proposito: `bShowSurfaceOverlay` spegne i marcatori di superficie,
	// che sono una preferenza di chi dipinge — un arco assente dallo schermo e' invece una mappa che
	// mente per omissione, ed e' il difetto che #1768 chiude.
	RTHexEditor::DrawTransitions(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	if (bHasMarker)
	{
		RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, MarkerColor);
	}
}

#undef LOCTEXT_NAMESPACE
