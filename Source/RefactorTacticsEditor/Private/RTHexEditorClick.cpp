#include "RTHexEditorClick.h"
#include "RTHexTransitionGlyph.h"
#include "RTHexEditorModeSettings.h" // #921: il flag dell'overlay vive nel mode, non nei tool
#include "ContextObjectStore.h"
#include "InteractiveToolManager.h"
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
#include "Map/RTMapDependencyLibrary.h" // FRTMapElementHandle (#1864)
#include "Map/RTMapEditLibrary.h"       // ResolveInteriorWall, per disegnare il muro selezionato
#include "Map/RTGeometryGrammar.h"      // ToPolyline: la giacitura del muro, derivata dall'authority
#include "RTHexSelectionStore.h"        // la selezione condivisa che i tool disegnano
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

bool ShouldShowSurfaceOverlay(const UContextObjectStore* Store)
{
	if (!Store)
	{
		return false;
	}

	// ⚠️ **Lo store dei tool E' quello in cui il mode pubblica, e non serve nessuna risalita.**
	// `UEdMode::GetDefaultToolScope()` vale `EToolsContextScope::EdMode`, quindi sia `RegisterTool` sia il
	// `GetToolManager()` del mode lavorano sul ModeToolsContext: il settings viene messo esattamente nello
	// store che i sette `Render` interrogano. La risalita all'outer che `FindContext` fa serve al caso
	// opposto — raggiungere un oggetto pubblicato dal ModeManager, uno scope piu' LARGO — e qui non e' in
	// gioco. Il commento precedente diceva il contrario, e insegnava un modello sbagliato del ciclo di vita.
	const URTHexEditorModeSettings* Settings =
		const_cast<UContextObjectStore*>(Store)->FindContext<URTHexEditorModeSettings>();

	return Settings ? Settings->bShowSurfaceOverlay : false;
}

bool PublishSurfaceOverlaySettings(UContextObjectStore* Store, UObject* Settings)
{
	// Il `Cast` e' la meta' che conta: e' lo STESSO tipo che `ShouldShowSurfaceOverlay` cerca con
	// `FindContext`, e scriverlo qui e' cio' che impedisce alle due sedi di divergere.
	if (!Store || !Cast<URTHexEditorModeSettings>(Settings))
	{
		return false;
	}
	return Store->AddContextObject(Settings);
}

void WithdrawSurfaceOverlaySettings(UContextObjectStore* Store, UObject* Settings)
{
	if (Store && Settings)
	{
		Store->RemoveContextObject(Settings);
	}
}

bool ShouldShowSurfaceOverlay(const UInteractiveToolManager* ToolManager)
{
	if (!ToolManager)
	{
		return false;
	}
	return ShouldShowSurfaceOverlay(ToolManager->GetContextObjectStore());
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

bool NearestTransition(const ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
	FRTMapElementHandle& OutHandle, float* OutDistance)
{
	const URTHexMapAsset* Map = Actor ? Actor->MapAsset : nullptr;
	if (!Map || Map->Transitions.Num() == 0)
	{
		return false;
	}

	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	const FVector RayO = ClickPos.WorldRay.Origin;
	const FVector RayD = ClickPos.WorldRay.Direction;

	int32 BestIdx = INDEX_NONE;
	float BestDist = TNumericLimits<float>::Max();
	for (int32 I = 0; I < Map->Transitions.Num(); ++I)
	{
		const FRTHexEdge& E = Map->Transitions[I];
		const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
		const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
		const float Dist = URTHexLibrary::DistanceRayToSegment(RayO, RayD, A, B);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestIdx = I;
		}
	}

	// ⚠️ La soglia e' quella che `URTHexArchTool::RemoveNearestArch` usava da sempre: non un numero nuovo,
	// lo stesso numero in un posto dove lo possono leggere in due.
	if (BestIdx == INDEX_NONE || BestDist > HexSize * 0.6f)
	{
		return false;
	}

	OutHandle = FRTMapElementHandle::ForTransition(Map->Transitions[BestIdx].From, Map->Transitions[BestIdx].To);
	if (OutDistance)
	{
		*OutDistance = BestDist;
	}
	return true;
}

void DrawSelectedElement(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor,
	const FRTMapElementHandle& Handle, const FVector& Origin, float HexSize, float LayerHeight)
{
	if (!PDI || !Actor)
	{
		return;
	}

	// Le stesse due costanti che il disegno aveva quando viveva dentro `URTHexSelectTool`: sollevare il
	// tratto lo tiene sopra la faccia del prisma, e lo spessore lo distingue dal contorno di una cella.
	const FVector Lift(0.f, 0.f, 4.f);
	constexpr float Thick = 4.0f;

	switch (Handle.Kind)
	{
	case ERTMapElementKind::Cell:
	{
		const FVector Centre = URTHexLibrary::AxialToWorld(Handle.Cell, Origin, HexSize, LayerHeight);
		DrawHexMarker(PDI, Centre, HexSize * 0.9f, FColor::Yellow);
		break;
	}

	case ERTMapElementKind::Cover:
	case ERTMapElementKind::Door:
	{
		// Il LATO, non la cella: si disegna fra i due vertici piu' vicini al centro del bordo.
		//
		// ⚠️ Trovati per distanza invece che per indice: la corrispondenza «bordo N ↔ vertici N e N+1» e' una
		// convenzione che vive dentro `HexCorners`, e riscriverla qui sarebbe la seconda copia che prima o
		// poi diverge. Per distanza il risultato e' corretto per costruzione.
		const FVector Mid = URTHexLibrary::EdgeMidpointWorld(Handle.Cell, Handle.Edge, Origin, HexSize, LayerHeight);
		const FVector Centre = URTHexLibrary::AxialToWorld(Handle.Cell, Origin, HexSize, LayerHeight);

		TArray<FVector> Corners = URTHexLibrary::HexCorners(Centre, HexSize);
		Corners.Sort([&Mid](const FVector& A, const FVector& B)
		{
			return FVector::DistSquaredXY(A, Mid) < FVector::DistSquaredXY(B, Mid);
		});

		if (Corners.Num() >= 2)
		{
			const FColor Colour = (Handle.Kind == ERTMapElementKind::Door) ? FColor::Cyan : FColor::Orange;
			PDI->DrawLine(Corners[0] + Lift, Corners[1] + Lift, Colour, SDPG_Foreground, Thick);
		}
		break;
	}

	case ERTMapElementKind::InteriorWall:
	{
		// La GIACITURA vera del muro, non un simbolo al centro della cella: e' l'unico modo per distinguere
		// due muri interni sulla stessa cella, che e' precisamente il caso che il ciclo deve saper scorrere.
		const URTHexMapAsset* Map = Actor->MapAsset;
		const int32 Index = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
		if (Index == INDEX_NONE)
		{
			break;
		}

		const FRTHexInteriorWall& Wall = Map->InteriorWalls[Index];
		const FVector Centre = URTHexLibrary::AxialToWorld(Wall.Cell, Origin, HexSize, LayerHeight);

		// `ToPolyline` e' il derivato di calcolo del segmento: il float nasce qui, a valle dell'authority.
		const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, HexSize);
		for (int32 I = 0; I + 1 < Line.Points.Num(); ++I)
		{
			const FVector A(Centre.X + Line.Points[I].X, Centre.Y + Line.Points[I].Y, Centre.Z);
			const FVector B(Centre.X + Line.Points[I + 1].X, Centre.Y + Line.Points[I + 1].Y, Centre.Z);
			PDI->DrawLine(A + Lift, B + Lift, FColor::Green, SDPG_Foreground, Thick);
		}
		break;
	}

	case ERTMapElementKind::Transition:
	{
		// 🔑 **L'arco intero, da centro a centro.** E' l'unica forma che lo dice: un arco non sta su un
		// bordo e non sta dentro una cella — collega due celle su LAYER diversi, e disegnarne un simbolo
		// su una delle due nasconderebbe proprio la cosa che lo distingue da tutto il resto.
		//
		// ⚠️ Prima del 2026-09-23 questo ramo non c'era e si cadeva su `default: break`: una transizione
		// selezionata non si sarebbe vista. Il `Kind` era dichiarato, nessuno lo produceva, e il buco
		// sarebbe uscito al primo produttore.
		const FVector A = URTHexLibrary::AxialToWorld(Handle.Cell, Origin, HexSize, LayerHeight);
		const FVector B = URTHexLibrary::AxialToWorld(Handle.To, Origin, HexSize, LayerHeight);
		PDI->DrawLine(A + Lift, B + Lift, FColor::Magenta, SDPG_Foreground, Thick);

		// I due estremi marcati: senza, a picco l'arco si legge come un segmento qualunque fra due punti,
		// e non si vede QUALI celle collega.
		DrawHexMarker(PDI, A, HexSize * 0.35f, FColor::Magenta);
		DrawHexMarker(PDI, B, HexSize * 0.35f, FColor::Magenta);
		break;
	}

	default:
		break;
	}
}

/**
 * Un arco, coi suoi due canali per lato. Statica: la scelta dei canali e' di `RTHexTransition::Describe`,
 * qui c'e' solo il modo di metterla sullo schermo.
 */
static void DrawTransitionGlyph(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B,
	const RTHexTransition::FGlyph& G)
{
	const FVector Delta = B - A;
	const FVector Dir = Delta.GetSafeNormal();
	if (Dir.IsNearlyZero()) { return; }

	// ⚠️ **Un arco puo' essere VERTICALE, ed e' il caso normale dell'ascensore**: stessa cella, due layer,
	// quindi `Dir` coincide con `UpVector` e il prodotto vettoriale degenera a zero. Senza questo salto le
	// tacche e la barra — cioe' i due canali che questa issue aggiunge — sparirebbero proprio sul tipo che
	// piu' spesso sale dritto.
	FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::CrossProduct(Dir, FVector::ForwardVector).GetSafeNormal();
	}

	const FColor Tint = G.Tint;

	// IL CORPO. Continuo o tratteggiato: primo canale dello stato.
	if (G.Stroke == RTHexTransition::EStroke::Solid)
	{
		PDI->DrawLine(A, B, Tint, SDPG_Foreground, 2.f);
	}
	else
	{
		// Undici tratti, sei disegnati: abbastanza fitto da leggersi come «linea», abbastanza rado da non
		// confondersi con una continua.
		constexpr int32 Segmenti = 11;
		for (int32 K = 0; K < Segmenti; K += 2)
		{
			const FVector P0 = A + Delta * (static_cast<float>(K) / Segmenti);
			const FVector P1 = A + Delta * (static_cast<float>(K + 1) / Segmenti);
			PDI->DrawLine(P0, P1, Tint, SDPG_Foreground, 2.f);
		}
	}

	// LA PUNTA: il verso e' un dato, e `From`/`To` non sono intercambiabili.
	const float H = 18.f;
	PDI->DrawLine(B, B - Dir * H + Side * (H * 0.5f), Tint, SDPG_Foreground, 2.f);
	PDI->DrawLine(B, B - Dir * H - Side * (H * 0.5f), Tint, SDPG_Foreground, 2.f);

	// LE TACCHE: secondo canale del tipo, e l'unico che sopravvive alla scala di grigi. Stanno sulla prima
	// meta' dell'arco, lontano dalla punta, perche' la' non competono con nient'altro.
	const float Tacca = FMath::Max(6.f, static_cast<float>(Delta.Size()) * 0.05f);
	for (int32 T = 0; T < G.Ticks; ++T)
	{
		const float U = 0.16f + 0.05f * T;
		const FVector C = A + Delta * U;
		PDI->DrawLine(C - Side * Tacca, C + Side * Tacca, Tint, SDPG_Foreground, 2.f);
	}

	// LA BARRA: secondo canale dello stato, e SOLO per `Destroyed`. E' cio' che lo separa da `Inactive`,
	// che il tratteggio accomuna — entrambi dicono «non si passa», ma uno si riaccende e l'altro no.
	if (G.bCrossed)
	{
		const FVector M = A + Delta * 0.5f;
		const float R = FMath::Max(10.f, static_cast<float>(Delta.Size()) * 0.07f);
		const FVector D1 = (Dir + Side).GetSafeNormal() * R;
		const FVector D2 = (Dir - Side).GetSafeNormal() * R;
		PDI->DrawLine(M - D1, M + D1, Tint, SDPG_Foreground, 3.f);
		PDI->DrawLine(M - D2, M + D2, Tint, SDPG_Foreground, 3.f);
	}
}

void DrawTransitions(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor)
{
	if (!PDI || !Actor || !Actor->MapAsset) { return; }

	const URTHexMapAsset* Map = Actor->MapAsset;
	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;

	// Coerente con l'overlay e con `RebuildInstances`: in `ActiveOnly` si mostra solo cio' che tocca il
	// layer attivo. Un arco lo tocca se **uno dei due** estremi ci sta — e' il piano da cui si parte o
	// quello a cui si arriva, e in entrambi i casi chi lavora deve saperlo.
	const bool bActiveOnly = (Actor->LayerView == ERTLayerViewMode::ActiveOnly);
	const int32 ActiveLayer = Actor->ActiveLayer;

	for (const FRTHexEdge& Edge : Map->Transitions)
	{
		if (bActiveOnly && Edge.From.Layer != ActiveLayer && Edge.To.Layer != ActiveLayer) { continue; }

		// ⛔ Nessuna scelta di resa qui dentro: quali canali, con quali valori, lo dice la funzione pura.
		const RTHexTransition::FGlyph G = RTHexTransition::Describe(Edge);

		// Alzate sopra il disco della cella, o la linea sparirebbe dentro la mesh.
		const FVector A = URTHexLibrary::AxialToWorld(G.From, Origin, HexSize, LayerH) + FVector(0, 0, 4.0);
		const FVector B = URTHexLibrary::AxialToWorld(G.To, Origin, HexSize, LayerH) + FVector(0, 0, 4.0);
		DrawTransitionGlyph(PDI, A, B, G);
	}
}

void DrawSharedSelection(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor)
{
	const URTHexSelectionStore* Store =
		GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr;
	if (!PDI || !Actor || !Store || Store->GetSelection().Num() == 0)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);

	for (const FRTMapElementHandle& Handle : Store->GetSelection())
	{
		DrawSelectedElement(PDI, Actor, Handle, Origin, HexSize, LayerH);
	}
}
} // namespace RTHexEditor
