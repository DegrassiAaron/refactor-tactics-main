#include "Tools/RTHexArchTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "InteractiveGizmoManager.h"
#include "BaseGizmos/TransformProxy.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "InteractiveGizmo.h" // ETransformGizmoSubElements
#include "Map/RTHexCellData.h" // ERTHexTransitionKind
#include "Map/RTMapEditLibrary.h"        // DeleteElement: la REGOLA della cancellazione, una sola (#1864)
#include "Map/RTMapDependencyLibrary.h"  // FRTMapElementHandle
#include "RTHexSelectionStore.h"         // la selezione condivisa
#include "Editor.h"                      // GEditor
#include "ScopedTransaction.h"           // una gesture = un Undo
#include "Framework/Application/SlateApplication.h" // Ctrl accumula, come in Select

#define LOCTEXT_NAMESPACE "URTHexArchTool"

// Colore per Kind e freccia stanno in `RTHexEditor` (RTHexEditorClick.h): li usa anche l'overlay, che
// mostra le transizioni SEMPRE e non solo mentre le si crea. Due definizioni dello stesso vocabolario
// visivo prima o poi divergono.

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
		// 🔑 **Ctrl SELEZIONA l'arco invece di cancellarlo** (#1864, casella 1: «un click seleziona
		// ... una transizione»). E' lo stesso idioma di `URTHexSelectTool` — Ctrl accumula — e non
		// tocca il gesto esistente: senza Ctrl, Remove cancella come ha sempre fatto.
		//
		// ⚠️ Il hit-test e' quello che questo tool possiede da sempre, ora in
		// `RTHexEditor::NearestTransition`: la spec §13.3 assegna al tool il test di viewport degli archi,
		// perche' `ElementsAt` risponde a «che cosa c'e' sotto questo bordo» e un arco non sta su un bordo.
		const bool bAdditive = FSlateApplication::IsInitialized()
			&& FSlateApplication::Get().GetModifierKeys().IsControlDown();

		if (bAdditive)
		{
			FRTMapElementHandle Handle;
			float Distanza = 0.f;
			if (!RTHexEditor::NearestTransition(Actor, ClickPos, Handle, &Distanza))
			{
				UE_LOG(LogTemp, Log, TEXT("[HexMode] Nessun arco entro la soglia: niente da selezionare."));
				return;
			}

			if (URTHexSelectionStore* Store = GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr)
			{
				const bool bNuovo = Store->AddHandle(Handle);
				UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco %s (dist %.1f): %s. Selezione: %s."),
					*Handle.Cell.ToString(), Distanza,
					bNuovo ? TEXT("selezionato") : TEXT("gia' in selezione"),
					*URTHexSelectionStore::Describe(Store->GetSelection()));
			}
			return;
		}

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
		// «su una cella From» e non «su una NUOVA cella From»: `OnClicked` passa questo motivo anche quando
		// si ri-clicca la cella gia' memorizzata, e l'aggettivo avrebbe affermato un cambio mai avvenuto.
		case ERTArchPendingClose::ReClick:          return TEXT("re-click su una cella From");
		case ERTArchPendingClose::Shutdown:         return TEXT("Shutdown del tool (cambio tool o uscita dal mode)");
		case ERTArchPendingClose::Committed:        return TEXT("Commit: la transizione e' stata scritta");
		case ERTArchPendingClose::ClearedByUser:    return TEXT("ClearArch dal pannello");
		case ERTArchPendingClose::SwitchedToRemove: return TEXT("passaggio a Remove");
		// `Count` e' un `case` esplicito perche' senza lo `switch` non copre l'enum e Clang emette
		// `-Wswitch`. Non e' un motivo di chiusura: se compare nel log, qualcuno l'ha passata per errore.
		case ERTArchPendingClose::Count:            return TEXT("<Count: sentinella, non un motivo di chiusura>");
		}
		return TEXT("<motivo non mappato>");
	}

	// ⚠️ **Su MSVC — il toolchain di questo progetto — questa e' la rete PRIMARIA, non la secondaria**:
	// `-Wswitch` avvisa su Clang, ma C4062 e' spento per default e lo `switch` esaustivo qui sopra non
	// protegge nulla. Da qui i due assert.
	//
	// ⚠️ **Si pinna il TOTALE, non la posizione dell'ultimo motivo**, e la differenza non e' di stile.
	// Una relazione del tipo «l'ultimo motivo + 1 == Count» scatta solo per un inserimento **immediatamente
	// prima** di `Count`: inserendo altrove — dopo `Shutdown`, per dire — tutti gli ordinali successivi
	// scalano insieme e la relazione resta vera, quindi il motivo nuovo arriva a runtime senza `case` e il
	// log degrada a `<motivo non mappato>`. Copriva una posizione su sei.
	// Con il totale, qualunque inserimento fa fallire la compilazione.
	// ⚠️ Resta scoperto un enumeratore appeso **dopo** `Count`: li' il totale non cambia, e senza reflection
	// non c'e' difesa. `Count` deve restare l'ultimo — sta nel commento della sentinella.
	// Se fallisce: aggiungi il `case`, POI aggiorna il numero. Mai il contrario.
	static_assert(static_cast<uint8>(ERTArchPendingClose::Count) == 5,
		"ERTArchPendingClose e' cambiato: aggiungi il case in ArchPendingCloseToString, POI aggiorna questo numero.");
}

/**
 * `#996`, passo 1: quando un gizmo sparisce, questa riga dice chi l'ha chiuso.
 *
 * ⚠️ **Chiusura vera e chiamata a vuoto hanno righe diverse, e non e' verbosita'.** Le chiamanti la invocano
 * anche quando non c'e' niente da chiudere — `RemoveNearestArch` a ogni click in Remove, `OnClicked` al primo
 * click in Add — e una riga unica avrebbe annunciato «passaggio a Remove» o «re-click» su transizioni mai
 * avvenute, in un log il cui unico compito e' disambiguare.
 *
 * ⚠️ **Entrambe a `Log`, e la scelta e' deliberata.** Il segnale piu' importante di `#996` e' l'ASSENZA di
 * righe: se al gesto il gizmo sparisce e qui non compare nulla, `DestroyPendingGizmo` non e' stata chiamata
 * e la causa sta altrove. Mettere il caso a vuoto a `Verbose` — che a verbosita' di default non si stampa —
 * avrebbe reso il silenzio ambiguo fra «non chiamata» e «chiamata senza nulla di pendente», cioe' avrebbe
 * tolto proprio la garanzia per cui il log esiste. Una riga in piu' per click a vuoto e' il prezzo, e non
 * mente su cosa e' successo.
 */
void URTHexArchTool::DestroyPendingGizmo(ERTArchPendingClose Reason)
{
	// `Count` non e' un motivo: chi la passa ha sbagliato, e va detto dove succede invece di lasciarlo
	// leggere a chi passa dal log.
	ensureMsgf(Reason != ERTArchPendingClose::Count,
		TEXT("DestroyPendingGizmo chiamata con la sentinella Count: passa un motivo vero."));

	// `bHasFrom` da solo, e non `bHasFrom || Gizmo`: `OnClicked` alza il flag PRIMA di creare il gizmo e
	// questa funzione li azzera insieme, quindi non esiste uno stato con gizmo e senza flag. Scriverli in
	// disgiunzione suggerirebbe due segnali indipendenti che non ci sono.
	// Letto PRIMA di azzerare: dopo, ogni chiamata sembrerebbe a vuoto.
	const bool bCeraQualcosaDaChiudere = bHasFrom;

	if (bCeraQualcosaDaChiudere)
	{
		// ⚠️ **`Gizmo != nullptr` NON dice che il gizmo esista**, ed e' la cosa che questa stessa issue ha
		// misurato: il puntatore e' una `UPROPERTY` forte e sopravvive a `Shutdown()`, che azzera il
		// `GizmoActor` dentro l'oggetto. Stampare «presente» su quel test farebbe dire alla riga il falso
		// proprio nello scenario di `#996` — gizmo chiuso da fuori, poi `ClearArch`. Si legge `IsVisible()`,
		// lo stesso criterio del rilevatore in `Render`.
		// ⚠️ `bHasFrom` non si stampa: questo ramo esiste solo quando e' vero, quindi sarebbe un `1` fisso
		// travestito da misura.
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco: chiusura del pendente — %s (gizmo %s)."),
			ArchPendingCloseToString(Reason),
			(Gizmo && Gizmo->IsVisible()) ? TEXT("vivo") : TEXT("gia' chiuso da fuori"));
	}
	else
	{
		// ⚠️ **Il motivo NON si stampa qui**, ed e' il punto dell'AC 7. Le chiamanti passano il proprio
		// motivo a prescindere: `RemoveNearestArch` manda `SwitchedToRemove` a ogni click in Remove e
		// `OnClicked` manda `ReClick` anche al primo click. Riportarlo su una chiamata a vuoto farebbe
		// comparire «passaggio a Remove» o «re-click» su transizioni mai avvenute — la sovraffermazione
		// che questa riga esiste per togliere. Qui conta che la chiamata c'e' stata, non con che etichetta.
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco: nessun pendente da chiudere (chiamata a vuoto)."));
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

	// ⛔ RIFIUTO AL GESTO (#1869): in v0.1 una scala collega solo layer adiacenti.
	//
	// 🔑 E' lo STESSO predicato che `ValidateMapDetailed` applica alla collezione, chiamato qui perche' i due
	// strati sono due momenti: qui si impedisce di scriverla, la' si segnala quella che c'e' gia' — dentro un asset
	// di versione precedente, o ricostruito. Uno non sostituisce l'altro.
	//
	// ⚠️ Si esce SENZA distruggere il gizmo pendente, come fa il rifiuto qui sopra: il gesto resta aperto e
	// chi ha mirato il piano sbagliato trascina sul giusto, invece di ricominciare.
	if (!URTHexArcLibrary::IsTransitionLayerSpanLegal(From, To, Kind))
	{
		// La diagnosi porta i DUE layer e il salto, non «non valido» (#1869, Debug/Logging): chi ha
		// sbagliato deve leggere fra quali piani, e di quanto.
		UE_LOG(LogTemp, Warning,
			TEXT("[HexMode] Arco RIFIUTATO: scala da %s (layer %d) a %s (layer %d) salta %d layer. ")
			TEXT("In v0.1 una scala collega solo layer adiacenti."),
			*From.ToString(), From.Layer, *To.ToString(), To.Layer,
			URTHexArcLibrary::TransitionLayerSpan(From, To));
		return;
	}

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
		// Copia From/To PRIMA di rimuovere (la rimozione muta l'array `Transitions`).
		const FRTCellId F = Map->Transitions[BestIdx].From;
		const FRTCellId T = Map->Transitions[BestIdx].To;

		// 🔑 **Si passa da `DeleteElement`, non da `RemoveTransitionData`** (#1864). Il gesto per chi
		// guarda e' identico — un click in Remove toglie l'arco — ma la REGOLA di che cosa muore
		// cancellando un elemento autorato ora vive in **un posto solo**, per ogni tipo. Due
		// implementazioni della stessa regola sono il modo in cui la regola diverge, ed e' il vincolo che
		// il corpo di #712 dichiarava gia' per il validator e la cottura.
		//
		// ⚠️ E la transazione la apre **questo** chiamante: `URTMapEditLibrary` dichiara di non
		// aprirne (`RTMapEditLibrary.h`), cosi' la cascata resta un solo Ctrl+Z.
		URTHexMapAsset* Scrivibile = Actor->MapAsset;
		const FScopedTransaction Transaction(
			NSLOCTEXT("RTHexArchTool", "RemoveArch", "Cancella un arco di transizione"));
		Scrivibile->Modify();

		const ERTMapEditOutcome Esito =
			URTMapEditLibrary::DeleteElement(Scrivibile, FRTMapElementHandle::ForTransition(F, T));

		Actor->RebuildInstances();
		// ⚠️ La RAGIONE, non il numero: `esito 4` obbliga chi legge ad aprire l'enum e contare (#1864).
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco %s -> %s (dist %.1f): %s."),
			*F.ToString(), *T.ToString(), BestDist, *URTMapEditLibrary::DescribeOutcome(Esito));
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

	if (RTHexEditor::ShouldShowSurfaceOverlay(GetToolManager()))
	{
		// ⌫ **Il parametro `bIncludeTransitions` non esiste piu'** (#1768). Serviva a non vedere doppie le
		// frecce, perche' questo tool le disegnava anche per conto proprio; ora le disegna **solo**
		// `DrawTransitions`, una volta, per tutti e sette gli strumenti.
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}

	// 🔑 **Le transizioni, con QUALUNQUE strumento attivo e senza dipendere da un toggle** (#1768).
	// Fuori dal blocco qui sopra di proposito: `bShowSurfaceOverlay` spegne i marcatori di superficie,
	// che sono una preferenza di chi dipinge — un arco assente dallo schermo e' invece una mappa che
	// mente per omissione, ed e' il difetto che #1768 chiude.
	RTHexEditor::DrawTransitions(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));

	// 🔑 **La selezione condivisa si vede anche da qui** (#1864, casella 2). Senza, lo store era
	// condiviso per COSTRUZIONE — un `UEditorSubsystem` fuori dai property set — e per NESSUN
	// consumatore: solo Select lo leggeva e lo disegnava. Un elemento selezionato che sparisce cambiando
	// strumento e' il difetto di #921 nella sua forma di selezione.
	RTHexEditor::DrawSharedSelection(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));

	const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);

	// ⌫ **Il ciclo che disegnava qui le transizioni e' stato rimosso** (#1768). Era l'ultimo residuo del
	// difetto: le frecce di questo tool erano incondizionate, quelle dell'overlay no, e a overlay spento
	// restava vero che *«un arco si vede solo mentre il tool Arch e' aperto»*. Ora la sede e' una sola —
	// `DrawTransitions`, chiamata qui sopra come dagli altri sei — e porta anche i due canali per `Kind` e
	// per `State` che questo ciclo non aveva.

	// Arco pendente (indipendente dall'asset).
	if (bHasFrom)
	{
		// ⏳ **Qui va il rilevatore di «gizmo chiuso senza di noi» — `#996` AC 2/3, tracciato in `#1218`.**
		// Non e' un buco da riempire alla prima occasione: la misura c'e', il disegno no.
		//
		// **Cosa si osserva.** Non `!Gizmo`: e' una `UPROPERTY` forte, tiene l'oggetto raggiungibile e non si
		// azzera — `UInteractiveGizmoManager::DestroyGizmo` rilascia il proprio riferimento e non marca
		// garbage. Cambia cio' che sta **dentro**: `UCombinedTransformGizmo::Shutdown()` fa
		// `GizmoActor->Destroy(); GizmoActor = nullptr;` (UE 5.8.1). Quindi il segnale e' il `GizmoActor`,
		// leggibile da `GetGizmoActor()` — e **non** da `IsVisible()`, che vale
		// `IsValid(GizmoActor) && !IsHidden()` e confonde «distrutto» con «nascosto da `SetVisibility`».
		//
		// **Cosa resta da decidere, e perche' non si e' deciso qui.**
		//  · dove agganciarsi: `Render` gira a ogni frame e non e' un canale di stato, mentre
		//    `OnAboutToClearActiveTarget` e `OnVisibilityChanged` sono eventi esatti e senza latch;
		//  · se limitarsi a osservare o **riparare**: oggi il pendente resterebbe disegnato, con
		//    `Properties->bHasFrom` a `true` e `CommitArch()` capace di scrivere da un `To` stale.
		// Sono scelte di comportamento — chi vince fra il gesto dell'autore e il recupero — e vanno prese
		// col caso d'uso in mano, non dentro una funzione di disegno.

		RTHexEditor::DrawHexMarker(PDI, FromWorld, MarkerRadius, FColor::Green);
		if (bToValid)
		{
			RTHexEditor::DrawHexMarker(PDI, ToWorld, MarkerRadius, FColor::Blue);
			RTHexEditor::DrawArrow(PDI, FromWorld, ToWorld, FColor::White);
		}
	}
}

#undef LOCTEXT_NAMESPACE
