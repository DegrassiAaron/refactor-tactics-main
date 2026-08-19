#include "Tools/RTHexArchTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "InteractiveGizmoManager.h"
#include "BaseGizmos/TransformProxy.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "InteractiveGizmo.h" // ETransformGizmoSubElements
#include "Map/RTHexCellData.h" // ERTHexTransitionKind

#define LOCTEXT_NAMESPACE "URTHexArchTool"

namespace
{
	// Colore per tipo di transizione (solo visualizzazione).
	// Colore per Kind e freccia stanno in `RTHexEditor` (RTHexEditorClick.h): li usa anche l'overlay, che
	// mostra le transizioni SEMPRE e non solo mentre le si crea. Due definizioni dello stesso vocabolario
	// visivo prima o poi divergono.
}

UInteractiveTool* URTHexArchToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexArchTool* NewTool = NewObject<URTHexArchTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexArchTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexArchTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexArchToolProperties>(this);
	AddToolPropertySource(Properties);
	Properties->WeakTool = this;
}

void URTHexArchToolProperties::Commit()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->CommitArch(); }
}

void URTHexArchToolProperties::ClearArch()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->ClearPending(); }
}

void URTHexArchTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio."));
		return;
	}
	if (Properties && Properties->Operation == ERTHexArchOp::Remove)
	{
		RemoveNearestArch(Actor, ClickPos);
		return;
	}
	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	DestroyPendingGizmo(ERTArchPendingClose::ReClick); // no duplicati su re-click

	TargetActor = Actor;
	From = Cell;
	To = Cell;
	bHasFrom = true;
	bToValid = false;
	FromWorld = Center;
	MarkerRadius = (Actor->MapAsset ? Actor->MapAsset->HexSize : Actor->HexSize) * 0.9f;

	Proxy = NewObject<UTransformProxy>(this);
	Proxy->SetTransform(FTransform(Center));
	Gizmo = GetToolManager()->GetPairedGizmoManager()->CreateCustomTransformGizmo(
		ETransformGizmoSubElements::TranslateAllAxes, this);
	Gizmo->SetActiveTarget(Proxy, nullptr);
	Proxy->OnTransformChanged.AddUObject(this, &URTHexArchTool::OnGizmoMoved);

	if (Properties) { Properties->From = From; Properties->bHasFrom = true; Properties->To = To; Properties->bToValid = false; }
	UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco: From %s, gizmo spawnato."), *From.ToString());
}

void URTHexArchTool::Shutdown(EToolShutdownType ShutdownType)
{
	DestroyPendingGizmo(ERTArchPendingClose::Shutdown);
	USingleClickTool::Shutdown(ShutdownType);
}

namespace
{
	const TCHAR* ArchPendingCloseToString(ERTArchPendingClose Reason)
	{
		switch (Reason)
		{
		case ERTArchPendingClose::ReClick:          return TEXT("re-click su una nuova cella From");
		case ERTArchPendingClose::Shutdown:         return TEXT("Shutdown del tool (cambio tool o uscita dal mode)");
		case ERTArchPendingClose::Committed:        return TEXT("Commit: la transizione e' stata scritta");
		case ERTArchPendingClose::ClearedByUser:    return TEXT("ClearArch dal pannello");
		case ERTArchPendingClose::SwitchedToRemove: return TEXT("passaggio a Remove");
		// ⚠️ **`Count` e' un `case` esplicito, non una dimenticanza.** Senza, lo `switch` non copre tutti
		// gli enumeratori e Clang emette `-Wswitch` — che sotto promozione dei warning e' un errore di
		// compilazione in un file che oggi compila pulito. Non e' un motivo di chiusura: se arriva qui,
		// qualcuno l'ha passata a `DestroyPendingGizmo`, e il testo deve dirlo invece di travestirsi da
		// motivo plausibile.
		case ERTArchPendingClose::Count:            return TEXT("<Count: sentinella, non un motivo di chiusura>");
		}
		return TEXT("<motivo non mappato>");
	}

	// 🔴 **Questo assert pinna il CONTEGGIO, non la mappatura, e il suo messaggio non deve invitare ad
	// aggirarlo** (corretto in code review, `#1052`). La prima stesura diceva «aggiorna
	// `ArchPendingCloseToString` prima di toccare questo numero», e la via di minor resistenza era
	// **bumpare il 5 a 6**: dopo di che il codice compila senza `case` per il motivo nuovo e il log degrada
	// in silenzio — esattamente cio' che l'assert doveva impedire.
	// La difesa vera contro un enumeratore non mappato e' lo `switch` esaustivo qui sopra (`-Wswitch` su
	// Clang); questo assert e' la rete **secondaria**, e serve su MSVC, dove C4062 e' spento per default.
	// ∴ se fallisce: aggiungi il `case`, poi aggiorna il numero. Mai il contrario.
	static_assert(static_cast<uint8>(ERTArchPendingClose::Count) == 5,
		"ERTArchPendingClose e' cambiato: aggiungi il case in ArchPendingCloseToString, POI aggiorna questo numero.");
}

/**
 * `#996`, passo 1. Questa funzione era l'unica del giro a NON loggare, mentre tutte le sue cinque chiamanti
 * loggano: quando un gizmo spariva, il registro conteneva tutto tranne la riga che diceva chi l'aveva chiuso.
 *
 * ⚠️ **Il segnale piu' importante di questo log e' la sua ASSENZA.** Se al gesto che #996 descrive — modificare
 * la Transform Location dell'actor con un arco pendente — il gizmo sparisce e qui NON compare nessuna riga,
 * allora `DestroyPendingGizmo` non e' stata chiamata: il tool e' vivo, e la causa sta altrove. Un log che tace
 * e' un dato solo per chi sa che doveva parlare, e questo commento e' il posto dove sta scritto.
 *
 * Per questo si stampano anche `bHasFrom` e la presenza del gizmo PRIMA di azzerarli: distinguono una
 * chiusura vera da una chiamata a vuoto, che le cinque chiamanti fanno regolarmente.
 *
 * 🔴 **E fino a `#1052` le due cose finivano nella stessa riga, che affermava una transizione mai avvenuta.**
 * `RemoveNearestArch` e' chiamata a **ogni** click mentre `Operation == Remove`, quindi il secondo click e i
 * successivi stampavano *«passaggio a Remove»* senza che nessun passaggio fosse avvenuto; simmetricamente il
 * **primo** click in Add stampava *«re-click su una nuova cella From»* con `bHasFrom=0` e nessun gizmo. In un
 * log il cui unico compito e' disambiguare, e' la stessa sovraffermazione che il commento dell'enum esiste
 * per prevenire — e che quel commento a sua volta commetteva.
 * ∴ la chiamata a vuoto ha ora una riga **propria**, e a `Verbose`: l'evidenza che la chiamata c'e' stata
 * resta, ma non si traveste da chiusura.
 */
void URTHexArchTool::DestroyPendingGizmo(ERTArchPendingClose Reason)
{
	// Letto PRIMA di azzerare: dopo, ogni chiamata sembrerebbe a vuoto.
	const bool bCeraQualcosaDaChiudere = bHasFrom || (Gizmo != nullptr);

	if (bCeraQualcosaDaChiudere)
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco: chiusura del pendente — %s (bHasFrom=%d, gizmo %s)."),
			ArchPendingCloseToString(Reason),
			bHasFrom ? 1 : 0,
			Gizmo ? TEXT("presente") : TEXT("assente"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[HexMode] Arco: nessun pendente da chiudere (chiamata con motivo %s)."),
			ArchPendingCloseToString(Reason));
	}

	if (GetToolManager() && GetToolManager()->GetPairedGizmoManager())
	{
		GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);
	}
	Gizmo = nullptr;
	Proxy = nullptr;
	bHasFrom = false;
	bToValid = false;
	if (Properties) { Properties->bHasFrom = false; Properties->bToValid = false; }
}

void URTHexArchTool::OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform)
{
	if (bSnapping || !TargetActor || !bHasFrom || !InProxy) { return; }

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	const URTHexMapAsset* Map = TargetActor->GetHexContext(Origin, HexSize, LayerH);

	const FVector W = InTransform.GetLocation();
	const FRTCellId Cell = URTHexLibrary::WorldToCellId(W, Origin, HexSize, LayerH);
	To = Cell;
	// Valido solo se distinto da From e se ENTRAMBE le celle esistono (Commit scriverebbe altrimenti a vuoto).
	bToValid = (Cell != From) && Map && Map->ContainsCell(Cell) && Map->ContainsCell(From);
	ToWorld = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerH);

	// Ri-snap al centro della cella. Si scrive sul GIZMO, non sul proxy, e non e' una preferenza:
	//
	//   `UTransformProxy::SetTransform` aggiorna `SharedTransform` e fa broadcast di `OnTransformChanged`,
	//   ma non muove niente a schermo — e `OnTransformChanged` non compare in tutto
	//   `CombinedTransformGizmo.cpp`: il gizmo **non si iscrive al proprio proxy**. Il `GizmoActor` viene
	//   posizionato una volta sola in `SetActiveTarget` e poi guidato dai sub-gizmo, quindi scrivere sul
	//   proxy lasciava il gizmo dov'era: il ri-snap si vedeva al gesto DOPO, quando un sub-gizmo
	//   ricominciava a interagire e rileggeva lo stato (#931).
	//
	// ⚠️ `ReinitializeGizmoTransform` e NON `SetNewGizmoTransform`: la seconda «genera gli stessi eventi
	// Change/Modify, e quindi funziona con Undo/Redo», e un ri-snap automatico nell'undo stack sarebbe un
	// passo che l'utente non ha compiuto — con `PIE-HEX-MODE-E` che verifica proprio Undo/Redo.
	//
	// La guardia resta: `Reinitialize` non rientra in `OnGizmoMoved`, ma il proxy va comunque allineato
	// perche' e' lui il bersaglio letto altrove, e quella scrittura si' che ri-emette l'evento.
	bSnapping = true;
	InProxy->SetTransform(FTransform(ToWorld));
	if (Gizmo)
	{
		Gizmo->ReinitializeGizmoTransform(FTransform(ToWorld));
	}
	bSnapping = false;

	if (Properties) { Properties->To = To; Properties->bToValid = bToValid; }
}

void URTHexArchTool::CommitArch()
{
	if (!TargetActor || !bHasFrom || !bToValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Arco: niente da committare (serve From + To valido)."));
		return;
	}
	const ERTHexTransitionKind Kind = Properties ? Properties->Kind : ERTHexTransitionKind::Stair;
	const int32 Cost = Properties ? Properties->Cost : 2;
	const bool bBidir = Properties ? Properties->bBidirectional : true;
	TargetActor->AddTransitionData(From, To, Cost, Kind, bBidir);
	DestroyPendingGizmo(ERTArchPendingClose::Committed);
}

void URTHexArchTool::ClearPending()
{
	DestroyPendingGizmo(ERTArchPendingClose::ClearedByUser);
}

void URTHexArchTool::RemoveNearestArch(ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos)
{
	DestroyPendingGizmo(ERTArchPendingClose::SwitchedToRemove); // esci da un eventuale Add pendente

	const URTHexMapAsset* Map = Actor->MapAsset;
	if (!Map || Map->Transitions.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Remove: nessuna transizione nell'asset."));
		return;
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
		if (Dist < BestDist) { BestDist = Dist; BestIdx = I; }
	}

	if (BestIdx != INDEX_NONE && BestDist <= HexSize * 0.6f)
	{
		// Copia From/To PRIMA di rimuovere (RemoveTransitionData muta l'array Transitions).
		const FRTCellId F = Map->Transitions[BestIdx].From;
		const FRTCellId T = Map->Transitions[BestIdx].To;
		Actor->RemoveTransitionData(F, T, /*bBothDirections=*/true);
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco rimosso %s -> %s (dist %.1f)."), *F.ToString(), *T.ToString(), BestDist);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Nessun arco entro soglia (min dist %.1f)."), BestDist);
	}
}

void URTHexArchTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);

	// Transizioni esistenti (solo se l'asset e' popolato).
	if (Actor && Actor->MapAsset)
	{
		const FVector Origin = Actor->GetActorLocation();
		const float HexSize = Actor->MapAsset->HexSize;
		const float LayerH = Actor->MapAsset->LayerHeight;
		for (const FRTHexEdge& E : Actor->MapAsset->Transitions)
		{
			const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
			const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
			RTHexEditor::DrawArrow(PDI, A, B, RTHexEditor::TransitionKindColor(E.Kind));
		}
	}

	// Arco pendente (indipendente dall'asset).
	if (bHasFrom)
	{
		// ⛔ **`bHasFrom && !Gizmo` NON e' osservabile, e non e' un'inferenza dalla dichiarazione.**
		// Misurato in `UInteractiveGizmoManager::DestroyGizmo` (UE 5.8.1): deregistra dall'input router,
		// chiama `Shutdown()`, rimuove dalla propria `ActiveGizmos` e invalida — **nessun `MarkAsGarbage`**.
		// Il manager rilascia quindi solo il proprio riferimento; il nostro `Gizmo` e' una `UPROPERTY`
		// forte, l'oggetto resta raggiungibile e la GC non lo raccoglie ∴ il puntatore non si azzera.
		// ⚠️ La ragione conta: un `UPROPERTY` forte **viene** azzerato se il referente e' marcato garbage,
		// quindi «e' forte» da solo non basta a concludere — e la prima stesura di questo commento si
		// fermava li'. Chi volesse osservare davvero il caso ha due strade, entrambe da verificare
		// sull'API prima di scrivere: un canale che marchi l'oggetto, o una callback di distruzione.

		RTHexEditor::DrawHexMarker(PDI, FromWorld, MarkerRadius, FColor::Green);
		if (bToValid)
		{
			RTHexEditor::DrawHexMarker(PDI, ToWorld, MarkerRadius, FColor::Blue);
			RTHexEditor::DrawArrow(PDI, FromWorld, ToWorld, FColor::White);
		}
	}
}

#undef LOCTEXT_NAMESPACE
