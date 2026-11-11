#include "Player/Overlay/RTPlanningPreview.h"
#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Combat/RTRangeVerdict.h"
#include "Map/RTHexMapAsset.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Player/Overlay/RTFocusArea.h"
#include "Player/Planning/RTPlanningSnapshot.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/Playback/RTPlanPreview.h"
#include "Turn/RTTurnManager.h"                // PlayerAimMap: la mira si giudica sulla mappa della squadra
#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Resolution/RTHexSimLibrary.h"
#include "UI/HUD/RTHudViewModel.h"             // MakeBlastPreviewPlan: la traduzione unita' -> anteprima e' una sola (#3620)
#include "Unit/RTUnit.h"

namespace
{
	/** Mappa esagonale del livello + contesto geometrico (unica fonte di scala). nullptr = livello senza mappa. */
	ARTHexMapActor* PreviewHexMapWithContext(const UWorld* World, FVector& OutOrigin, float& OutHexSize,
		float& OutLayerHeight, const URTHexMapAsset*& OutAsset)
	{
		OutAsset = nullptr;
		ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
		if (HexMap)
		{
			OutAsset = HexMap->GetHexContext(OutOrigin, OutHexSize, OutLayerHeight);
		}
		return HexMap;
	}
}


bool RTPlanningPreview::ChiedeUnBersaglio(const URTActionData& Armata)
{
	return !URTCatalogLibrary::IsFastMovement(Armata.Def) && Armata.Def.Slot != ERTActionSlot::Reaction
		&& URTPointerLibrary::TargetKindForAction(Armata.Def, Armata.bSelfTarget, Armata.Shape)
			!= ERTPointerTargetKind::None;
}

bool RTPlanningPreview::MostraLaPortata(const ARTUnit* Unit, const URTActionData& Armata)
{
	return ChiedeUnBersaglio(Armata) && Unit->IsAlive() && Armata.RangeCells > 0;
}

FRTCellId RTPlanningPreview::OrigineDiMira(const ARTUnit* Unit, const URTActionData& Armata)
{
	return Unit->AimOriginFor(Armata.Def.ResolutionPhase);
}

void RTPlanningPreview::Refresh(const UWorld* World, const ARTUnit* Unit, ARTHexMapActor* KnownHexMap,
	const FRTFocusMoveAreas* FocusAreas, const TArray<FRTRangeCellVerdict>* SkillRange)
{
	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	// `#3511`: l'ingresso unico ha gia' cercato la mappa, e una seconda ricerca itererebbe gli actor del livello.
	ARTHexMapActor* HexMap = KnownHexMap;
	if (HexMap)
	{
		Map = HexMap->GetHexContext(Origin, HexSize, LayerH);
	}
	else
	{
		HexMap = PreviewHexMapWithContext(World, Origin, HexSize, LayerH, Map);
	}
	if (HexMap && FocusAreas != nullptr)
	{
		// 🔑 **Il fuoco ha la mappa, e il piano cede** (`#3750`, spec §6.3): l'arbitro ha detto `Focus`, quindi si parte da
		// una mappa pulita dallo STESSO reset e si scrive il solo canale del fuoco. E' ancora questa funzione l'unica che
		// scrive i setter delle anteprime: la sorgente e' un ingresso, non una seconda sede.
		//
		// Sta PRIMA di `PlayerAimMap`: la mira e la copia ottimistica servono al piano, e il fuoco le scarterebbe (review).
		//
		// La corsa si disegna AL NETTO della normale (scelta della sessione, da giudicare in PIE): la libreria le
		// restituisce entrambe per intero, e la differenza si fa qui, dove si disegna.
		HexMap->ClearPlanningPreview();
		const TSet<FRTCellId> Normal(FocusAreas->Normal);
		TArray<FRTCellId> RunOnly;
		for (const FRTCellId& Cell : FocusAreas->Run)
		{
			if (!Normal.Contains(Cell))
			{
				RunOnly.Add(Cell);
			}
		}
		HexMap->SetPreviewFocusAreas(FocusAreas->Normal, RunOnly);
		return;
	}
	if (HexMap && SkillRange != nullptr)
	{
		// 🔑 **La portata di una skill ha la mappa, e il piano cede** (`#3750`, fetta D, spec §6.3): come per il fuoco, si parte
		// dal reset unico e si scrive il solo canale dell'hover. Quando il cursore esce, l'arbitro dice un'altra sorgente e
		// il ramo che la scrive riparte da una mappa in cui questo canale e' gia' spento.
		HexMap->ClearPlanningPreview();
		TArray<FRTCellId> Free;
		TArray<FRTCellId> Blocked;
		TArray<FRTCellId> Unverdicted;
		for (const FRTRangeCellVerdict& Entry : *SkillRange)
		{
			switch (Entry.Verdict)
			{
			case ERTRangeVerdict::Free:        Free.Add(Entry.Cell); break;
			case ERTRangeVerdict::Blocked:     Blocked.Add(Entry.Cell); break;
			case ERTRangeVerdict::Unverdicted: Unverdicted.Add(Entry.Cell); break;
			}
		}
		HexMap->SetPreviewSkillRange(Free, Blocked, Unverdicted);
		return;
	}
	// 🔴 La mira si giudica sulla stessa mappa del percorso ([D-465] (3), `#2794`): portata viola e rifiuto del
	// bersaglio pianificato leggono la copia ottimistica della squadra.
	Map = ARTTurnManager::PlayerAimMap(World, Unit ? Unit->TeamId : INDEX_NONE, Map);
	if (!HexMap)
	{
		return;
	}
	if (!Unit)
	{
		// 🔑 **Il reset sta in UN punto, sull'attore** (`ARTHexMapActor::ClearPlanningPreview`, `#3511`): qui c'era
		// l'elenco dei setter, e ogni canale dimenticato restava a schermo — la rotta durante la risoluzione (`#2390`),
		// la timeline (`#172`), il tratto di LOS rimasto indefinitamente (`#3064`). Chi aggiunge un'area la spegne
		// accanto al proprio setter, e questo ramo non cresce piu'.
		//
		// ⚠️ **`LastMoveRoutes` non sostituisce la rotta**: `ARTHUD::DrawHUD` disegna la scia grigia dei percorsi DAVVERO
		// avvenuti su un canale diverso, quindi le due tracce convivevano invece di darsi il cambio.
		HexMap->ClearPlanningPreview();
		return;
	}

	// 🔑 **Il piano ha la mappa: il fuoco cede** (`#3750`). Il ramo del fuoco parte da `ClearPlanningPreview`, ma questo
	// scrive solo i propri canali: senza questa riga l'area di un fuoco gia' uscito (armare, un waypoint, `RMB`) restava a
	// schermo accanto al ventaglio. L'ha trovata il test del ciclo, non la lettura: l'arbitro deve far cedere nei DUE versi.
	HexMap->SetPreviewFocusAreas(TArray<FRTCellId>(), TArray<FRTCellId>());
	// ...e la portata di una skill in hover (`#3750`, fetta D): stessa ragione, stesso verso. Senza questa riga la portata a tre
	// stati restava a schermo accanto al ventaglio quando il cursore usciva dallo slot.
	HexMap->SetPreviewSkillRange(TArray<FRTCellId>(), TArray<FRTCellId>(), TArray<FRTCellId>());

	// 🔴 **La ROTTA si scrive dal piano, qui, e da nessun altro** (`#3511`, review; `#3892`). Con la regola derivata dallo
	// stato le aree si spengono e tornano senza un evento: `ClearPlanningPreview` azzera anche la rotta, e il ritorno dalla
	// pausa con un piano gia' tracciato la lasciava spenta mentre ventaglio, portata e timeline tornavano.
	// ⏱️ *Fino a `#3892` sei siti la scrivevano a mano prima di chiamare il rinfresco: erano ridondanti, e facevano due
	// verita' sul percorso.* Chi cambia il piano chiama `RefreshPlanningOverlay` e basta.
	HexMap->SetPreviewPath(Unit->PlannedPath);

	// Dove puo' ANCORA arrivare: budget, blocchi, occupanti e archi sono gia' applicati, e i waypoint gia'
	// pianificati hanno gia' speso la loro parte (#877). Il ventaglio risponde a «quanto mi resta», non a
	// «quanto avevo»: senza i waypoint mostrerebbe un raggio che il piano in corso ha gia' consumato.
	FRTHexSnapshot Snapshot;
	int32 UnitId = INDEX_NONE;
	TArray<ARTUnit*> Units;
	TArray<FRTCellId> Reachable;
	bool bHasSnapshot = false;
	if (RTPlanningSnapshot::For(World, Unit, Snapshot, UnitId, &Units))
	{
		bHasSnapshot = true;
		for (const FRTHexReachableCell& R :
			URTHexSimLibrary::ReachableCellsAfterPlan(Snapshot, UnitId, Unit->PlannedWaypoints))
		{
			Reachable.Add(R.Cell);
		}
	}

	// 🔑 **In targeting la PORTATA prende il posto del ventaglio** (`#3507`, decisione d'autore del 2026-10-06): con
	// un'azione a bersaglio armata la domanda e' «dove posso mirare», non «dove posso andare».
	// ⛔ Le celle vengono da `TargetableRangeCells`, la classificazione del click, dall'origine di mira.
	//
	// ⚠️ **Spegnere il ventaglio e mostrare la portata sono due domande** (`#3517`). `Action.Wait` ha portata `0`,
	// cioe' nessuna area di mira che prenda il posto del ventaglio: il ventaglio resta, e la portata che contornava
	// la cella dell'unita' sparisce (referto del 2026-10-06, §13: erano due difetti).
	bool bMira = false;
	TArray<FRTCellId> Portata;
	if (const URTActionData* Armata = Unit->GetAbility(Unit->SelectedAbilityIndex))
	{
		bMira = ChiedeUnBersaglio(*Armata) && Armata->RangeCells > 0;
		if (MostraLaPortata(Unit, *Armata))
		{
			Portata = URTCombatLibrary::TargetableRangeCells(Map, OrigineDiMira(Unit, *Armata), Armata->RangeCells,
				Armata->Def.LineOfSightPolicy, Armata->Def.MinRangeCells);
		}
	}
	HexMap->SetPreviewReachableCells(bMira ? TArray<FRTCellId>() : Reachable);
	HexMap->SetPreviewRangeCells(Portata);

	// Da dove agira' e su cosa. La derivazione sta in `URTHexCombatLibrary::MakeBlastPreview`, che e'
	// pura e testabile headless: qui si TRADUCE il piano, non si decide.
	//
	// 🔴 **Il ramo del bersaglio a CELLA e' la meta' che prima mancava.** Il codice leggeva il solo
	// `PlannedAttackTarget`, che `HandleTargetCell` azzera per costruzione (il bersaglio e' la cella):
	// dichiarare un'area su un varco vuoto SPEGNEVA l'anteprima invece di mostrarla — il caso che `#737`
	// aveva dichiarato coperto («l'area colpita in preview prima del click»).
	//
	// 🔴 **E l'origine non e' piu' `Unit->Cell` in ogni caso.** La fase Dash precede il Blast, quindi chi
	// ha pianificato uno scatto e poi un attacco sparera' da dove sara' arrivato. `PlannedDashMoves()` e' la
	// stessa domanda che `ResolveDash` si pone, rifiuto dello stato compreso ([D-471]). ⚠️ Da #3509 decide anche
	// la FASE dell'azione, e la carica ne resta fuori ([D-464], `AimOriginCell`).
	// La traduzione sta in `URTHudViewModel::MakeBlastPreviewPlan` ([D-492], #3620): la usano anche i segni sulle
	// unita' dell'HUD, cosi' anteprima e overlay non possono piu' dire due cose diverse sullo stesso piano.
	const FRTBlastPreviewPlan PreviewPlan = URTHudViewModel::MakeBlastPreviewPlan(*Unit, UnitId, Units);
	const TArray<FRTHexCombatUnit> HexUnits = URTHudViewModel::MakeHexCombatUnits(Units);
	// L'azione principale serve ancora piu' sotto, alla timeline delle fasi: e' la stessa che il piano ha tradotto.
	const URTActionData* Ability = Unit->GetAbility(Unit->PlannedAbilityIndex);

	const FRTBlastPreview Blast = URTHexCombatLibrary::MakeBlastPreview(PreviewPlan, HexUnits);
	HexMap->SetPreviewHitCells(Blast.HitCells, Blast.AllyCells);

	// Origine e mira si accendono solo con un bersaglio davvero previsualizzabile: una linea che parte e
	// non arriva da nessuna parte direbbe che c'e' un attacco dove non c'e'.
	const bool bHasAim = Blast.HitCells.Num() > 0;
	const FRTCellId AimCell = PreviewPlan.bTargetsCell
		? PreviewPlan.TargetCell
		: (HexUnits.IsValidIndex(PreviewPlan.TargetId) ? HexUnits[PreviewPlan.TargetId].Cell : FRTCellId());
	HexMap->SetPreviewAttack(Blast.Origin, AimCell, bHasAim, Blast.bOriginFromPlannedDash);

	// ➕ **LA TIMELINE DEL PIANO, una voce per fase** (`CP 11.5`, `#172`).
	//
	// 🔑 **Si TRADUCE il piano, non si decide niente.** Tutto ciò che segue riempie una struct di
	// ingresso; l'origine, l'area, il percorso e il facing li deriva `MakePlanPreview`, che è pura e
	// verificabile headless — e che a sua volta chiama le funzioni del resolver invece di riscriverle.
	//
	// ⚠️ **Senza snapshot non si costruisce una timeline finta.** `BuildCompositeHexPath` ha bisogno
	// dello stato autorevole: darle uno snapshot vuoto produrrebbe un percorso che il resolver non
	// percorrerà mai, cioè precisamente la divergenza che questo checkpoint esiste per impedire.
	if (bHasSnapshot)
	{
		FRTPlanPreviewInput Timeline;
		Timeline.UnitId = UnitId;

		// 🔴 **La reazione e' armata da `PlannedReactionAbility`, NON da `ReactionProfileId`**, e la
		// prima stesura leggeva il secondo. Quello e' configurazione persistente dell'eroe — sta accanto
		// ad `Affinity` e `Weakness`, e la pianificazione non lo scrive mai — quindi sbagliava in
		// **entrambi** i versi: un eroe con un profilo configurato risultava armato ogni turno anche senza
		// aver pianificato niente, e chi arma sul profilo base (`NAME_None`) spariva dalla timeline. Lo
		// slot per-turno e' quello che `ClearReactionPlan` azzera.
		// ⚖️ **E non `URTCatalogLibrary::ArmsReaction`, perche' la domanda e' un'altra** (`#3628`, decisione
		// d'autore del 2026-10-11). Il roster chiede «quest'unita' ha armato qualcosa che aprira' una finestra?» e
		// conta anche l'Overwatch ([D-478]). Qui si chiede «c'e' una voce di Prep per l'azione dello slot
		// `Reaction`?». L'Overwatch come principale ha gia' la sua voce di Prep (`bAzioneNelPrep` in
		// `MakePlanPreview`): contarlo qui la disegnerebbe due volte. E il dato `FRTReactionPreview` — che oggi nessuno
		// disegna — porrebbe `WatchOrigin` a fine scatto, mentre l'Overwatch si arma da dove l'unita' e' prima dello
		// scatto ([D-470]).
		Timeline.bReactionArmed = Unit->PlannedReactionAbility != INDEX_NONE;
		Timeline.ReactionProfileId = Unit->ReactionProfileId;
		if (const URTActionData* Reazione = Unit->GetAbility(Unit->PlannedReactionAbility))
		{
			Timeline.PrepActionId = Reazione->Def.ActionId;
		}

		// 🔴 **E lo scatto si legge da `PlannedDashAbility`.** `PlannedDashCell` e' dichiarata
		// *«valida solo se `PlannedDashAbility` e' impostata»*, si costruisce a `(0,0,0)` e nessuno la
		// azzera — `ResolveDash` pulisce la sola abilita'. Confrontarla con la cella corrente, come faceva
		// la prima stesura, dava uno scatto FANTASMA verso l'origine della mappa a ogni unita' che non ci
		// stesse sopra, e dopo un turno risolto ridisegnava la destinazione dello scatto PRECEDENTE.
		Timeline.bDashPlanned = Unit->PlannedDashAbility != INDEX_NONE
			&& !(Unit->PlannedDashCell == Unit->Cell);
		Timeline.bDashResolves = Unit->PlannedDashMoves(); // [D-471]
		Timeline.PlannedDashCell = Unit->PlannedDashCell;
		if (const URTActionData* Scatto = Unit->GetAbility(Unit->PlannedDashAbility))
		{
			Timeline.DashActionId = Scatto->Def.ActionId;
			// [D-524]: il fantasma legge la STESSA retta del resolver, con lo stile e la portata che il resolver
			// userebbe. Una carica oltre la portata si ferma dove la portata finisce ([D-516]): lo dice la funzione
			// del movimento, non un conteggio a parte.
			Timeline.DashStyle = Scatto->Def.MovementStyle;
			Timeline.DashRange = Unit->EffectiveDashRangeOf(Unit->PlannedDashAbility);
		}

		Timeline.Blast = PreviewPlan;
		Timeline.PlannedWaypoints = Unit->PlannedWaypoints;
		Timeline.bMoveResolves = !Unit->PlannedNormalMoveDeniedByStun(); // [D-569]: la domanda del resolver, non un secondo giudizio
		// ⚠️ **Ogni fase riempita porta il proprio `ActionId`**, e la prima stesura riempiva il solo
		// Blast. La entry dichiara che `NAME_None` significa «fase che il piano non riempie»: con tre fasi
		// su quattro vuote, un consumatore non poteva distinguere le due cose.
		Timeline.MoveActionId = TEXT("Action.Move");
		if (Ability)
		{
			Timeline.BlastActionId = Ability->Def.ActionId;
		}

		// ➕ **Il motivo del rifiuto, calcolato sul bersaglio DEL PIANO** (`#172`).
		//
		// ⌫ **Una prima stesura trasportava `ARTHUD::LastRefusal`, ed era sbagliato.** Quel campo e'
		// dichiarato *«nasce da un click e muore col click seguente»*: e' legato all'ULTIMO CLICK, non al
		// bersaglio corrente del piano. Pianificando un attacco valido su A e poi cliccando B fuori
		// portata, ogni refresh successivo che non fosse un click — un waypoint annullato, la fine del
		// playback — rileggeva `Range` e declassava a `Uncertain` un piano che non aveva niente che non
		// andasse. E preso da `GetFirstPlayerController()` sarebbe stato, in split-screen, il rifiuto di
		// un ALTRO osservatore: proprio la lettura incrociata che [D-225] vieta.
		//
		// 🔑 **Si COMPONGONO le due funzioni canoniche, non se ne riscrive una.** `ClassifyHexTargeting`
		// piu' `RefusalForObserver` e' la stessa coppia, nello stesso ordine, che usa il sito del click; il
		// flag di conoscenza e' quello del bersaglio pianificato, letto qui e non altrove.
		if (Ability && !Unit->bAttackTargetsCell)
		{
			if (const ARTUnit* Bersaglio = Unit->PlannedAttackTarget.Get())
			{
				// La coppia ha ora un nome, e lo stesso nome lo leggono gli slot di [D-459] (#3483).
				// [D-464]: dall'origine della fase, come il click. ⏱️ *Fino a #3509 da `Unit->Cell`*.
				Timeline.BlastTargetRefusal = URTCombatLibrary::RefusalForKnownTarget(
					Map, Unit->AimOriginFor(Ability->Def.ResolutionPhase), Bersaglio->Cell, Ability->RangeCells,
					Ability->Def.LineOfSightPolicy, Bersaglio->IsKnownToObserver(), Ability->Def.MinRangeCells,
					URTCombatLibrary::CrossLayerAimFor(Ability->Def));
			}
		}

		HexMap->SetPlanPreview(
			URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Timeline, HexUnits));
	}
	else
	{
		HexMap->SetPlanPreview(FRTPlanPreview());
	}
}
