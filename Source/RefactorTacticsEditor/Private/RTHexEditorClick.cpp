#include "RTHexEditorClick.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "InputState.h"            // FInputDeviceRay
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"           // TActorIterator
#include "Editor.h"                // GEditor
#include "Selection.h"             // USelection
#include "ScopedTransaction.h"     // FScopedTransaction (SetActiveLayer annullabile)
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RTScenarioPreviewActor.h"  // ARTScenarioPreviewActor::PreviewTag: gli actor d'anteprima non sono "la mappa"

namespace
{
	/**
	 * Colore di un piano di CONTESTO: il colore della superficie sbiadito verso il grigio scuro al crescere
	 * della distanza dal layer attivo.
	 *
	 * Si attenua il colore invece di usare l'alpha perche' le linee del PDI non lo rispettano in modo
	 * affidabile: un ghost «trasparente» resterebbe brillante quanto il piano su cui si lavora, che e'
	 * esattamente la confusione da cui questa modalita' deve tirare fuori.
	 */
	FColor RTGhostColor(const FColor& Base, int32 Distance)
	{
		const float Extra = static_cast<float>(FMath::Max(0, Distance - 1));
		const float T = FMath::Clamp(0.55f + 0.15f * Extra, 0.f, 0.88f);
		return FMath::Lerp(FLinearColor(Base), FLinearColor(0.06f, 0.06f, 0.07f), T).ToFColor(/*bSRGB=*/ true);
	}
}

namespace RTHexEditor
{
ARTHexMapActor* FindTargetMapActor(UWorld* World)
{
	// Preferenza: un ARTHexMapActor selezionato nel Level Editor.
	if (GEditor)
	{
		if (USelection* Sel = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator It(*Sel); It; ++It)
			{
				if (ARTHexMapActor* A = Cast<ARTHexMapActor>(*It))
				{
					return A;
				}
			}
		}
	}
	// Fallback: l'unico ARTHexMapActor nel mondo.
	//
	// ⚠️ **Gli actor dell'anteprima di scenario (#1753) non contano.** `URTScenarioPreviewSubsystem`
	// posa un `ARTHexMapActor` TRANSIENTE per mostrare l'arena dello scenario selezionato: senza questo
	// salto, aprire un'anteprima accanto a una mappa d'autore farebbe trovare DUE actor a questa
	// ricerca, che risponde `nullptr` quando sono piu' d'uno — e i cinque tool di disegno si
	// spegnerebbero senza un messaggio. L'anteprima si riconosce dal tag e non dalla classe, perche'
	// `TActorIterator` trova anche le sottoclassi.
	ARTHexMapActor* Found = nullptr;
	if (World)
	{
		for (TActorIterator<ARTHexMapActor> It(World); It; ++It)
		{
			if (It->Tags.Contains(ARTScenarioPreviewActor::PreviewTag)) { continue; }
			if (Found) { return nullptr; } // piu' di uno e nessuno selezionato: ambiguo -> nessuna azione
			Found = *It;
		}
	}
	return Found;
}

bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
	FRTCellId& OutCell, FVector& OutCenter, FVector* OutClickedPoint)
{
	if (!Actor) { return false; }

	// Contesto geometrico dall'unica fonte condivisa col runtime (scala dall'asset, origine dall'actor).
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);
	const int32 Layer = Actor->ActiveLayer; // in editor il piano lo decide il layer attivo, non la quota del click

	// Qui resta la sola parte che ha bisogno del MONDO: sparare il raggio e decidere se il colpo vale. La
	// geometria — punto scelto, cella, centro — vive in `URTHexLibrary::ResolveRayToCellOnLayer`, dove e'
	// pura e ha dei test: in questo modulo non ce ne sono, e non e' una dimenticanza ma una proprieta' di
	// `Source/RefactorTacticsEditor/`.
	const FVector RayStart = ClickPos.WorldRay.Origin;
	const FVector RayEnd = ClickPos.WorldRay.PointAt(999999.0);
	FHitResult Result;
	// Solo la griglia SELEZIONABILE del target: ignora unita'/ostacoli tra camera e mappa, e ignora gli altri
	// componenti dello stesso actor. Confrontare l'ACTOR non basterebbe: `Result.Item` e' l'indice di istanza
	// del componente colpito, quindi un colpo sul rilievo del costo verrebbe risolto contro le celle di
	// `Cells` e produrrebbe una cella valida e SBAGLIATA — il pennello dipingerebbe altrove, senza errori.
	bool bHitTarget = World
		&& World->LineTraceSingleByObjectType(Result, RayStart, RayEnd,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects))
		&& Actor->IsPickOnSelectableCell(Result.GetComponent(), Result.Item);

	// In AllLayers l'ISM contiene un'istanza per OGNI piano mostrato, e il raggio colpisce la prima che incontra
	// — che puo' stare su un layer diverso da quello attivo. Se il colpo non e' sul layer attivo si scarta: la
	// funzione pura ripiega sul piano, e il perche' e' scritto li'.
	//
	// L'indice di istanza e' gia' stato validato sopra da `IsPickOnSelectableCell` — senza, `CellForInstance`
	// risponderebbe `(0,0,0)`, una cella valida, a un indice fuori range. Qui resta la sola domanda sul piano.
	if (bHitTarget && Actor->CellForInstance(Result.Item).Layer != Layer)
	{
		bHitTarget = false;
	}

	OutCell = URTHexLibrary::ResolveRayToCellOnLayer(ClickPos.WorldRay.Origin, ClickPos.WorldRay.Direction,
		Origin, HexSize, LayerH, Layer, bHitTarget, Result.ImpactPoint);
	OutCenter = URTHexLibrary::AxialToWorld(OutCell, Origin, HexSize, LayerH);

	if (OutClickedPoint)
	{
		// Col colpo si sa dove ha mirato l'autore; senza, il centro cella — e allora il bordo e' arbitrario
		// ma deterministico, che e' meglio di un lato che cambia a ogni click identico.
		*OutClickedPoint = bHitTarget ? Result.ImpactPoint : OutCenter;
	}

	return true;
}

void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color,
	float Thickness)
{
	if (!PDI) { return; }
	// Contorno dall'unica definizione condivisa col runtime (+2 di Z per non finire dentro la mesh).
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center + FVector(0, 0, 2.0), Radius);
	for (int32 I = 0; I < Corners.Num(); ++I)
	{
		PDI->DrawLine(Corners[I], Corners[(I + 1) % Corners.Num()], Color, SDPG_Foreground, Thickness);
	}
}

bool SetActiveLayer(ARTHexMapActor* Actor, int32 NewLayer)
{
	if (!Actor || Actor->ActiveLayer == NewLayer)
	{
		return false; // idempotente: niente transazione per un cambio che non cambia nulla
	}
	const FScopedTransaction Transaction(NSLOCTEXT("RTHexEditor", "HexSetActiveLayer", "Hex: Layer attivo"));
	Actor->Modify();
	Actor->ActiveLayer = NewLayer;
	Actor->RebuildInstances(); // assegnare da codice non passa da PostEditChangeProperty
	return true;
}
FColor TransitionKindColor(ERTHexTransitionKind Kind)
{
	switch (Kind)
	{
	case ERTHexTransitionKind::Stair:    return FColor(80, 200, 255);
	case ERTHexTransitionKind::Ramp:     return FColor(120, 255, 120);
	case ERTHexTransitionKind::Bridge:   return FColor(255, 200, 80);
	case ERTHexTransitionKind::Tunnel:   return FColor(200, 120, 255);
	case ERTHexTransitionKind::Elevator: return FColor(255, 120, 120);
	case ERTHexTransitionKind::Jump:     return FColor(255, 255, 255);
	default:                             return FColor(200, 200, 200);
	}
}

void DrawArrow(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B, const FColor& Color)
{
	if (!PDI) { return; }
	PDI->DrawLine(A, B, Color, SDPG_Foreground, 2.f);
	const FVector Dir = (B - A).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		const float H = 18.f;
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		PDI->DrawLine(B, B - Dir * H + Side * (H * 0.5), Color, SDPG_Foreground, 2.f);
		PDI->DrawLine(B, B - Dir * H - Side * (H * 0.5), Color, SDPG_Foreground, 2.f);
	}
}

FColor SurfaceColor(ERTHexSurface Surface)
{
	// Delega al runtime: una sola tavolozza per il marker dell'editor e per l'overlay in partita, altrimenti la
	// stessa cella cambierebbe colore fra i due contesti.
	return URTHexLibrary::SurfaceColor(Surface);
}

void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor)
{
	if (!PDI || !Actor || !Actor->MapAsset) { return; }
	const URTHexMapAsset* Map = Actor->MapAsset;
	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	// Coerente con RebuildInstances: in ActiveOnly l'overlay mostra solo il layer attivo (niente piani impilati).
	const bool bActiveOnly = (Actor->LayerView == ERTLayerViewMode::ActiveOnly);
	const bool bFocus = (Actor->LayerView == ERTLayerViewMode::Focus);
	const int32 ActiveLayer = Actor->ActiveLayer;
	const int32 GhostRange = FMath::Max(0, Actor->GhostLayerRange);
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		const int32 LayerDistance = FMath::Abs(Cell.Id.Layer - ActiveLayer);
		if (LayerDistance != 0)
		{
			if (bActiveOnly) { continue; }
			if (bFocus)
			{
				// Piano di CONTESTO: solo la sagoma, attenuata e sottile. Niente marcatori di regola —
				// blocca-movimento e blocca-vista appartengono al piano su cui si lavora, e ripeterli su
				// ogni piano vicino renderebbe illeggibile proprio quello attivo.
				if (LayerDistance > GhostRange) { continue; }
				const FVector GhostCenter = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerH);
				DrawHexMarker(PDI, GhostCenter, HexSize * 0.85f,
					RTGhostColor(SurfaceColor(Cell.Surface), LayerDistance), /*Thickness=*/ 1.0f);
				continue;
			}
		}
		const FVector Center = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerH);
		DrawHexMarker(PDI, Center, HexSize * 0.85f, SurfaceColor(Cell.Surface));
		// Due marcatori DISTINTI, come in partita (`ARTHexMapActor::DrawCellOverlay`): sono due regole diverse
		// — dove non si passa e dove non si vede — e una cella puo' avere l'una, l'altra o entrambe. Stessi
		// raggi del runtime, cosi' chi dipinge vede la mappa come la vedra' giocando.
		if (Cell.bBlocksLineOfSight)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.64f, URTHexLibrary::SightBlockerColor()); // giallo: non si vede attraverso
		}
		if (Cell.bBlocksMovement)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.45f, URTHexLibrary::BlockedCellColor()); // rosso: non ci si passa
		}
	}

	// Celle di PARTENZA, calcolate con la stessa funzione che allestisce la partita. Non sono un dato della
	// mappa: si spostano da sole appena una cella viene aggiunta o resa impercorribile, e senza vederle si
	// disegna una mappa senza sapere da dove partono le squadre — lo si scopre dal log a lavoro finito.
	//
	// Anello ESTERNO (1.0), piu' largo del contorno di superficie: non compete con i marcatori di regola, che
	// stanno tutti dentro.
	// Transizioni: l'UNICO modo in cui i layer si collegano. Fuori dal tool Arch non si vedevano, quindi chi
	// dipingeva con Paint o Fill non sapeva se una zona fosse collegata — e una piattaforma senza arco e'
	// irraggiungibile senza dirlo. Si disegnano sempre, non solo mentre le si crea.
	for (const FRTHexEdge& Edge : Map->Transitions)
	{
		if (bActiveOnly && Edge.From.Layer != ActiveLayer && Edge.To.Layer != ActiveLayer) { continue; }
		const FVector A = URTHexLibrary::AxialToWorld(Edge.From, Origin, HexSize, LayerH);
		const FVector B = URTHexLibrary::AxialToWorld(Edge.To, Origin, HexSize, LayerH);
		// Alzate sopra il disco della cella, o la linea sparirebbe dentro la mesh.
		DrawArrow(PDI, A + FVector(0, 0, 4.0), B + FVector(0, 0, 4.0), TransitionKindColor(Edge.Kind));
	}

	// Celle che NESSUNO raggiunge: calcolate dall'actor a ogni ricostruzione, non qui — una visita del grafo a
	// ogni frame sarebbe lavoro ripetuto per un dato che cambia solo quando la mappa cambia.
	for (const FRTCellId& Cell : Actor->GetUnreachableCells())
	{
		if (bActiveOnly && Cell.Layer != ActiveLayer) { continue; }
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerH);
		DrawHexMarker(PDI, Center, HexSize * 0.95f, URTHexLibrary::UnreachableCellColor(), 3.0f);
	}

	const int32 NumPerTeam = 2; // la v0.1 e' 2v2; il resto della scala e' un problema del formato, non dell'overlay
	const TArray<FRTCellId> Starts = URTMatchSetupLibrary::PickStartCells(Map, NumPerTeam, ActiveLayer);
	for (int32 I = 0; I < Starts.Num(); ++I)
	{
		// Anche in Focus: le partenze sono un marcatore brillante e appartengono al piano di lavoro, non al
		// contesto (`PickStartCells` le cerca gia' sul layer attivo; il filtro regge se un giorno cambiasse).
		if ((bActiveOnly || bFocus) && Starts[I].Layer != ActiveLayer) { continue; }
		const FVector Center = URTHexLibrary::AxialToWorld(Starts[I], Origin, HexSize, LayerH);
		const FColor Color = (I < NumPerTeam)
			? URTHexLibrary::SpawnTeam0Color()
			: URTHexLibrary::SpawnTeam1Color();
		DrawHexMarker(PDI, Center, HexSize * 1.0f, Color);
	}
}
} // namespace RTHexEditor
