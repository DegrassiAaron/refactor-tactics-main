#include "Player/Planning/RTFacingDeclaration.h"
#include "Player/Planning/RTFacingHost.h"
#include "Player/Planning/RTPlanningSnapshot.h"
#include "Ability/RTActionData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTTurnManager.h"                       // PlayerAimMap: la mappa di mira di una squadra
#include "Turn/Playback/RTPlaybackLibrary.h"          // DirectionYaw: l'anteprima del facing usa la stessa geometria del playback
#include "Turn/Resolution/RTFacingLibrary.h"          // CP 11.8: la legalita' della rotazione si CHIEDE, non si riscrive qui
#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Resolution/RTMovementActionLibrary.h"
#include "Unit/RTUnit.h"
#include "RefactorTactics.h"

namespace
{
	/** La geometria della mappa del livello. `false` senza un attore della mappa; l'asset puo' mancare anche con l'attore. */
	bool FacingHexContext(const UWorld* World, FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight,
		const URTHexMapAsset*& OutAsset)
	{
		OutAsset = nullptr;
		ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
		if (!HexMap)
		{
			return false;
		}
		OutAsset = HexMap->GetHexContext(OutOrigin, OutHexSize, OutLayerHeight);
		return true;
	}
}

void FRTFacingDeclaration::Begin(const FRTFacingHost& Host)
{
	if (!Host.GetFacingUnit())
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione: nessuna unita' selezionata"));
		return;
	}
	bDeclaring = true;
}

void FRTFacingDeclaration::End(const FRTFacingHost& Host)
{
	bDeclaring = false;
	// L'anteprima dell'hover non sopravvive al selettore: la mesh torna al verso che l'unita' avra' davvero.
	if (HoverSector.IsSet())
	{
		HoverSector.Reset();
		PreviewPlanned(Host, Host.GetFacingUnit());
	}
}

void FRTFacingDeclaration::PreviewPlanned(const FRTFacingHost& Host, ARTUnit* Unit)
{
	if (Unit == nullptr)
	{
		return;
	}

	// Il facing che l'unita' AVRA': la dichiarazione vince sul derivato, esattamente come a fine Move nel
	// TurnManager. Se le due regole divergessero, l'anteprima mentirebbe.
	ERTHexDirection Previsto = Unit->Facing;
	if (Unit->bDeclaresPlannedFacing)
	{
		Previsto = Unit->PlannedFacing;
	}
	else if (Unit->PlannedPath.Num() > 1)
	{
		// 🔴 **Il PRIMO passo, non l'ultimo.** In pianificazione l'unita' e' ancora sulla cella di
		// partenza: orientarla secondo l'ultimo passo del percorso la fa guardare verso una direzione che
		// assumera' dall'altra parte della mappa, e che da qui puo' essere l'opposta di dove sta per
		// incamminarsi. Chi guarda vede una figura ferma rivolta dalla parte sbagliata.
		//
		// ⚠️ **Questa e' l'anteprima della PARTENZA e non del facing finale**, che resta `FacingFromPath` e
		// lo scrive il resolver a fine Move. Le due coincidono su un percorso dritto e divergono su uno che
		// curva: durante il playback la mesh ruota passo per passo e ci arriva, e a fine risoluzione il
		// TurnManager la riallinea al valore logico.
		ERTHexDirection Partenza = Unit->Facing;
		if (URTHexLibrary::DirectionBetween(Unit->PlannedPath[0], Unit->PlannedPath[1], Partenza))
		{
			Previsto = Partenza;
		}
	}

	PreviewToward(Host, Unit, Previsto);
}

void FRTFacingDeclaration::PreviewToward(const FRTFacingHost& Host, ARTUnit* Unit, ERTHexDirection Direction)
{
	if (Unit == nullptr)
	{
		return;
	}
	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	if (!FacingHexContext(Host.GetFacingWorld(), Origin, HexSize, LayerH, Map))
	{
		return;
	}

	// Lo yaw si ricava dalla geometria come fa il TurnManager a fine playback: dal centro della cella al
	// centro del vicino nella direzione voluta. Ricavarlo dall'enum con una tabella sarebbe una seconda
	// verita' da tenere allineata alla prima.
	const FVector Here = Unit->WorldForCell(Unit->Cell, Origin, HexSize, LayerH);
	const FVector There = Unit->WorldForCell(URTHexLibrary::Neighbor(Unit->Cell, Direction), Origin, HexSize, LayerH);
	Unit->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Here, There), 0.f));
}

void FRTFacingDeclaration::UpdateHoverFromRay(const FRTFacingHost& Host, bool bHasRay, const FVector& RayOrigin,
	const FVector& RayDir)
{
	if (!bDeclaring && !HoverSector.IsSet())
	{
		return;
	}
	ARTUnit* Unit = Host.GetFacingUnit();
	TOptional<ERTHexDirection> Sotto;
	// `#3748`: con la barra in sola lettura il lato non si anticipa, nemmeno con un selettore rimasto aperto prima della fissazione.
	if (Unit && bDeclaring && bHasRay && !Host.IsFacingWriteBlocked() && !Host.IsFacingInputInert())
	{
		FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
		ERTHexDirection Settore = ERTHexDirection::E;
		if (FacingHexContext(Host.GetFacingWorld(), Origin, HexSize, LayerH, Map)
			&& URTPointerLibrary::ResolveFacingClick(RayOrigin, RayDir, CellFor(Unit), Origin, HexSize, LayerH,
				DeadZoneFraction * HexSize, /*bSelectorOpen=*/ true, Settore) == ERTFacingClick::Side
			&& IsLegalForPlan(Host, Unit, Settore))
		{
			Sotto = Settore;
		}
	}
	if (Sotto == HoverSector)
	{
		return; // niente da rifare: la mesh e' gia' li'
	}
	HoverSector = Sotto;
	if (!Unit)
	{
		return;
	}
	if (Sotto.IsSet())
	{
		PreviewToward(Host, Unit, Sotto.GetValue());
	}
	else
	{
		PreviewPlanned(Host, Unit);
	}
}

bool FRTFacingDeclaration::HandleSector(FRTFacingHost& Host, ERTHexDirection Sector)
{
	// #971 — quinto dei cinque siti `Order`. `HandleClick` ci arriva dal secondo click e sarebbe gia'
	// coperta a monte; questa funzione e' pubblica e raggiungibile da sola, quindi la guardia sta qui.
	if (Host.IsFacingInputInert())
	{
		return false;
	}

	ARTUnit* Unit = Host.GetFacingUnit();
	if (!Unit)
	{
		return false;
	}
	if (Host.GetFacingPointerContext() != ERTPointerContext::Facing)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione: non si sta dichiarando un facing"));
		return false;
	}

	const bool bLegal = IsLegalForPlan(Host, Unit, Sector);

	if (!bLegal)
	{
		// Rifiutata, MAI corretta in silenzio verso la legale piu' vicina: e' la regola di `URTFacingLibrary`,
		// e qui la si rispetta invece di riscriverla.
		UE_LOG(LogRT, Log,
			TEXT("[RT] Rotazione %d fuori dal budget di pivot (Move %d / Dash %d) per il movimento pianificato"),
			(int32)Sector, Unit->MoveEndPivotMaxSteps, Unit->DashEndPivotMaxSteps);
		return false;
	}

	Unit->DeclarePlannedFacing(Sector);
	bDeclaring = false;
	HoverSector.Reset(); // la mesh va sul verso dichiarato, qui sotto

	// La dichiarazione si vede SUBITO: un tasto che non produce nessun riscontro a schermo e' un tasto
	// che il giocatore crede rotto.
	PreviewPlanned(Host, Unit);

	Host.RecordFacingOrder();
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s dichiara rotazione %d — il movimento e' chiuso (Back per riaprirlo)"),
		*Unit->GetName(), (int32)Sector);
	return true;
}

FRTCellId FRTFacingDeclaration::CellFor(const ARTUnit* Unit)
{
	if (!Unit)
	{
		return FRTCellId();
	}
	// Lo scatto sostituisce il movimento: la sua cella e' quella in cui l'unita' finira'. `PlannedDashMoves`, e non il
	// solo indice: uno scatto sulla propria cella non e' uno scatto, e uno che lo stato nega lascia l'unita' dov'e'
	// ([D-471]).
	if (Unit->PlannedDashMoves())
	{
		return Unit->PlannedDashCell;
	}
	// [D-569]: un Move che lo stordimento nega non sposta l'unita', e il verso si dichiara da dove restera' — come per lo scatto.
	if (Unit->PlannedPath.Num() > 1 && !Unit->PlannedNormalMoveDeniedByStun())
	{
		return Unit->PlannedPath.Last();
	}
	return Unit->Cell;
}

void FRTFacingDeclaration::PlannedMovement(const FRTFacingHost& Host, const ARTUnit* Unit, ERTMovementStyle& OutStyle,
	TArray<FRTCellId>& OutPath)
{
	OutStyle = ERTMovementStyle::None;
	OutPath.Reset();
	if (!Unit)
	{
		return;
	}

	if (Unit->PlannedDashMoves()) // [D-471]: uno scatto negato dallo stato non muove l'unita'
	{
		const URTActionData* Scatto = Unit->GetAbility(Unit->PlannedDashAbility);
		if (Scatto)
		{
			// 🔑 Lo stile del CATALOGO, come lo legge il resolver (`DashUsed->Def.MovementStyle`).
			OutStyle = Scatto->Def.MovementStyle;
			OutPath.Add(Unit->Cell);
			// La rotta serve per l'ultimo passo. In linea e' quella che il resolver percorrera' — `ResolveLinearMove`,
			// cioe' la retta di `MovementLine` fermata dove la fermerebbe —; a budget si prende il percorso piu'
			// breve. Entrambe sono previsioni: il resolver rivalida sulla rotta vera.
			//
			// 🔑 **[D-524] Non basta la retta fino alla cella pianificata.** Una carica si ferma ADIACENTE al nemico, e
			// una carica oltre la portata si ferma alla portata: l'ultimo passo PERCORSO e' quello, e su una retta fuori
			// asse e' un passo diverso dall'ultimo della retta intera — sull'asse coincidono, per questo non si
			// vedeva. ⏱️ *Fino a [D-524] la rotta si cercava camminando le sole sei direzioni, e su un bersaglio fuori
			// asse restava la sola partenza: il controller proponeva versi che il resolver poi rifiutava.*
			if (URTMovementActionLibrary::IsLinear(Scatto->Def.MovementStyle))
			{
				FRTHexSnapshot Snapshot;
				int32 UnitId = INDEX_NONE;
				TArray<ARTUnit*> SnapshotUnits;
				if (RTPlanningSnapshot::For(Host.GetFacingWorld(), Unit, Snapshot, UnitId, &SnapshotUnits) && Snapshot.Map)
				{
					TSet<int32> Hostiles;
					for (int32 i = 0; i < SnapshotUnits.Num(); ++i)
					{
						const ARTUnit* Other = SnapshotUnits[i];
						if (Other && Other->IsAlive() && Other->TeamId != Unit->TeamId) { Hostiles.Add(i); }
					}
					const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(Snapshot.MapForPlanning(),
						Unit->Cell, Unit->PlannedDashCell, Unit->EffectiveDashRangeOf(Unit->PlannedDashAbility),
						Scatto->Def.MovementStyle, Snapshot.Occupancy, Hostiles);
					OutPath.Append(Linear.Entered);
				}
			}
			else
			{
				FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
				if (FacingHexContext(Host.GetFacingWorld(), Origin, HexSize, LayerH, Map) && Map)
				{
					const FRTHexPathResult Breve = URTHexPathLibrary::FindPathAvoiding(
						ARTTurnManager::PlayerAimMap(Host.GetFacingWorld(), Unit->TeamId, Map), Unit->Cell, Unit->PlannedDashCell,
						nullptr, 0);
					if (Breve.Status == ERTHexPathStatus::Success && Breve.Path.Num() >= 2)
					{
						OutPath = Breve.Path;
					}
				}
			}
			return;
		}
	}

	// [D-569]: un Move negato dallo stordimento non percorre niente, e il resolver giudica il verso di chi resta fermo (sei direzioni).
	if (Unit->PlannedPath.Num() > 1 && !Unit->PlannedNormalMoveDeniedByStun())
	{
		OutStyle = ERTMovementStyle::Budget;
		OutPath = Unit->PlannedPath;
	}
}

bool FRTFacingDeclaration::IsLegalForPlan(const FRTFacingHost& Host, const ARTUnit* Unit, ERTHexDirection Sector)
{
	if (!Unit)
	{
		return false;
	}
	// Lo stile e la rotta su cui si misura la legalita' sono quelli del movimento PIANIFICATO, letti come li
	// applica il resolver: uno scatto col proprio stile di catalogo, un percorso a budget, o fermo (sei direzioni).
	// ⏱️ *Fino a `#291` qui c'era `PlannedPath.Num() > 1 ? Budget : None`*: uno scatto pianificato veniva giudicato
	// sul budget Move, e il controller accettava versi che il resolver poi rifiutava ([D-367]).
	//
	// ⚠️ Questa e' una PREVISIONE, non il verdetto. Il resolver rivalida a fine Move su `MovementStyleThisTurn`
	// e `WalkedThisTurn`, cioe' su quel che e' successo davvero: un percorso puo' essere interrotto, e la
	// dichiarazione allora cade con `DeclarationRejected`. La UI propone, il servizio decide — §3 dell'owner.
	ERTMovementStyle Style = ERTMovementStyle::None;
	TArray<FRTCellId> Rotta;
	PlannedMovement(Host, Unit, Style, Rotta);

	ERTHexDirection Applied = Unit->Facing;
	return URTFacingLibrary::TryApplyDeclaredFacing(Style, Rotta, Unit->Facing, Sector, Unit->PivotBudget(), Applied);
}

void FRTFacingDeclaration::CancelDeclared(const FRTFacingHost& Host, ARTUnit* Unit, const TCHAR* Perche)
{
	if (!Unit || !Unit->bDeclaresPlannedFacing)
	{
		return;
	}
	Unit->RetractPlannedFacing();
	PreviewPlanned(Host, Unit);
	UE_LOG(LogRT, Log, TEXT("[RT] %s: verso dichiarato annullato (%s) — il movimento e' di nuovo aperto"),
		*Unit->GetName(), Perche);
}

bool FRTFacingDeclaration::TryOpenSelector(const FRTFacingHost& Host, const FRTCellId& ClickedCell)
{
	// `#3748`: aprire il selettore chiude il movimento dell'unita' selezionata, quindi con la barra in sola lettura e' una scrittura del piano.
	if (Host.IsFacingWriteBlocked() || Host.IsFacingInputInert())
	{
		return false;
	}
	ARTUnit* Unit = Host.GetFacingUnit();
	if (!Unit)
	{
		return false;
	}
	// Solo sulla cella finale: la propria da fermo, la destinazione in marcia ([D-463]).
	if (!(CellFor(Unit) == ClickedCell))
	{
		return false;
	}
	// [D-528] (3): la cella finale di una carica e' quella del nemico che punta, e li' il click ispeziona.
	if (Host.IsFacingChargeTargetAt(Unit, ClickedCell))
	{
		return false;
	}
	// Un bersaglio da scegliere ha la precedenza: il click e' suo.
	if (Host.GetFacingPointerContext() == ERTPointerContext::Targeting && Host.GetFacingTargetKind() != ERTPointerTargetKind::None)
	{
		return false;
	}
	Begin(Host);
	UE_LOG(LogRT, Log, TEXT("[RT] %s: scegli il verso — click nella direzione in cui guardare (Back per chiudere)"),
		*Unit->GetName());
	return true;
}

bool FRTFacingDeclaration::HandleClick(FRTFacingHost& Host, const FRTCellId& Cell, ERTHexDirection Sector)
{
	// `#3748`: un selettore rimasto aperto prima della fissazione non dichiara un verso mentre la barra e' in sola lettura.
	if (Host.IsFacingWriteBlocked() || Host.IsFacingInputInert())
	{
		return false;
	}
	ARTUnit* Unit = Host.GetFacingUnit();
	if (!Unit)
	{
		return false;
	}
	const FRTCellId Finale = CellFor(Unit);
	if (!(Cell == Finale))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Verso: (%d,%d,L%d) non e' la cella finale del movimento"),
			Cell.X, Cell.Y, Cell.Layer);
		return false;
	}
	const bool bDaFermo = Finale == Unit->Cell;
	if (bDaFermo && !bDeclaring)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Verso: da fermo si sceglie dopo un click sulla propria cella"));
		return false;
	}
	// [D-528] (3): un click diretto su un lato dell'esagono del nemico puntato dalla carica non dichiara un verso.
	if (!bDeclaring && Host.IsFacingChargeTargetAt(Unit, Cell))
	{
		return false;
	}

	const bool bEraAperto = bDeclaring;
	Begin(Host);
	if (HandleSector(Host, Sector))
	{
		return true;
	}
	// Illegale: nessuna correzione. Un selettore APERTO resta aperto per un altro lato ([D-463]); un click diretto sul
	// lato, in marcia, non ne aveva uno, e lasciarlo aperto mangerebbe il click successivo.
	if (!bEraAperto)
	{
		End(Host);
	}
	return false;
}

bool FRTFacingDeclaration::HandlesClickOnFinalCell(const FRTFacingHost& Host, const ARTUnit* Unit) const
{
	// 🔴 `#3748`: con la barra in sola lettura il selettore NON reclama il click, ed e' il punto — un click sull'unita' propria selezionata, che ha
	// una destinazione pianificata, cadrebbe sul pavimento dell'esagono finale e verrebbe consumato come scelta del verso PRIMA della selezione.
	// Senza questa riga la barra resterebbe sul nemico: il click deve arrivare a `SelectUnit`, che la riporta.
	if (!Unit || Host.IsFacingWriteBlocked() || Host.IsFacingInputInert())
	{
		return false;
	}
	const ERTPointerContext Context = Host.GetFacingPointerContext();
	if (Context == ERTPointerContext::Targeting && Host.GetFacingTargetKind() != ERTPointerTargetKind::None)
	{
		return false; // un bersaglio da scegliere ha la precedenza
	}
	const FRTCellId Finale = CellFor(Unit);
	const bool bDaFermo = Finale == Unit->Cell;
	const bool bSelettoreAperto = Context == ERTPointerContext::Facing;
	if (bDaFermo && !bSelettoreAperto)
	{
		return false; // da fermo il primo click apre il selettore, e lo fa il ramo della selezione
	}
	// 🔴 [D-528] (3): per una carica la cella finale e' quella del nemico puntato. Su quella cella il click prosegue: `DispatchUnitClick` lo
	// ispeziona, e `HandleClickOnCell` ispeziona il nemico se il raggio ha colpito il pavimento.
	if (!bSelettoreAperto && Host.IsFacingChargeTargetAt(Unit, Finale))
	{
		return false;
	}
	return true;
}

bool FRTFacingDeclaration::TryHandleClickUnderCursor(FRTFacingHost& Host, ARTUnit* Unit)
{
	if (!HandlesClickOnFinalCell(Host, Unit))
	{
		return false;
	}
	const FRTCellId Finale = CellFor(Unit);
	const bool bSelettoreAperto = Host.GetFacingPointerContext() == ERTPointerContext::Facing;

	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	if (!FacingHexContext(Host.GetFacingWorld(), Origin, HexSize, LayerH, Map) || !Map)
	{
		return false;
	}
	FVector RayOrigin, RayDir;
	if (!Host.DeprojectFacingCursor(RayOrigin, RayDir) || FMath::IsNearlyZero(RayDir.Z)) // #3661
	{
		return false;
	}

	// 🔑 **Il PAVIMENTO della cella finale, non la mesh colpita** ([D-367]): un personaggio fermo copre la propria
	// cella col corpo, e senza questa proiezione non potrebbe scegliersi il verso. La geometria e' della libreria.
	ERTHexDirection Settore = ERTHexDirection::E;
	switch (URTPointerLibrary::ResolveFacingClick(RayOrigin, RayDir, Finale, Origin, HexSize, LayerH,
		DeadZoneFraction * HexSize, bSelettoreAperto, Settore))
	{
	case ERTFacingClick::Miss:
	case ERTFacingClick::OtherCell:
		return false;
	case ERTFacingClick::Center:
		if (!bSelettoreAperto)
		{
			// In marcia il secondo click sul centro — il segno del waypoint — apre la scelta ([D-463]).
			TryOpenSelector(Host, Finale);
			return true;
		}
		UE_LOG(LogRT, Log, TEXT("[RT] Verso: click al centro dell'esagono — clicca nella direzione in cui guardare"));
		return true; // consumato: il centro non sceglie e non diventa un waypoint
	case ERTFacingClick::Side:
		HandleClick(Host, Finale, Settore);
		return true;
	}
	return false;
}
