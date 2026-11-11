#include "Player/Overlay/RTPlanningOverlayArbiter.h"
#include "Player/Overlay/RTPlanningOverlayHost.h"
#include "Player/Overlay/RTFocusArea.h"
#include "Player/Overlay/RTPlanningPreview.h"
#include "Ability/RTActionData.h"
#include "Combat/RTRangeVerdict.h"
#include "EngineUtils.h"                    // TActorIterator: la vista di chi guarda si costruisce sulle unita' del mondo
#include "Kismet/GameplayStatics.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTKnowledgeView.h"
#include "Turn/RTTurnManager.h"
#include "UI/HUD/RTHudViewModel.h"          // BuildObserverKnowledgeView: la stessa vista del velo
#include "UI/Inspect/RTInspect.h"           // LiveEntry: il cancello del fuoco e' uno
#include "Unit/RTUnit.h"

namespace
{
	/** Il TurnManager del livello, o nullptr: turno, fine partita e conoscenza della squadra si leggono dall'autorita'. */
	ARTTurnManager* OverlayTurnManager(const FRTPlanningOverlayHost& Host)
	{
		return Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(Host.GetOverlayWorld(), ARTTurnManager::StaticClass()));
	}
}

FRTPlanningOverlayInputs FRTPlanningOverlayArbiter::Gather(const FRTPlanningOverlayHost& Host,
	const ARTUnit* Unit, const ARTHexMapActor* HexMap) const
{
	FRTPlanningOverlayInputs Inputs;
	Inputs.Unit = Unit;
	Inputs.bUnitAlive = Unit && Unit->IsAlive();
	// Il contesto e il tipo di bersaglio si LEGGONO dal puntatore, che e' l'autorita': ricalcolarli qui sarebbe la
	// seconda verita' che diverge. ⚠️ Entrambi guardano l'unita' selezionata, che in ogni chiamata di produzione e'
	// `Unit`: `RefreshPlanningOverlay` la riceve dopo che `SelectedActor` e' stato scritto.
	Inputs.Context = Host.GetOverlayPointerContext();
	Inputs.ArmedAbilityIndex = Unit ? Unit->SelectedAbilityIndex : INDEX_NONE;
	Inputs.ArmedTargetKind = Host.GetOverlayTargetKind();
	if (HexMap)
	{
		Inputs.HoveredCell = HexMap->GetHoveredCell();
		Inputs.bHoveredValid = HexMap->IsHoveredCellValid();
	}
	const TOptional<ERTHexDirection> FacingHoverSector = Host.GetOverlayFacingSector();
	Inputs.FacingSector = FacingHoverSector.IsSet() ? static_cast<int32>(FacingHoverSector.GetValue()) : INDEX_NONE;
	Inputs.ActiveLayer = Host.GetOverlayActiveLayer();
	Inputs.PlanRevision = PlanningOverlayRevision;
	// ⚠️ **Costa una ricerca di actor** (`GetActorOfClass` itera il livello), come `GetPointerContext` subito sopra e come
	// `UpdateGlance` in questo stesso tick: e' il prezzo di leggere turno e fine partita dall'autorita' invece di copiarli.
	if (const ARTTurnManager* TM = OverlayTurnManager(Host))
	{
		Inputs.TurnIndex = TM->GetTurnNumber();
		// `GetPointerContext` non dichiara la fine partita di proposito: a partita finita il puntatore resta in
		// `Planning`, e le aree si spengono da qui.
		Inputs.bMatchEnded = TM->GetPhase() == ERTMatchPhase::MatchEnded;
	}
	return Inputs;
}

void FRTPlanningOverlayArbiter::Apply(const FRTPlanningOverlayHost& Host,
	ARTHexMapActor* HexMap, const ARTUnit* Unit, const FRTPlanningOverlayInputs& Inputs)
{
	if (bHasPlanningOverlayInputs && Inputs == LastPlanningOverlayInputs)
	{
		return;
	}
	const bool bPlanChanged = !bHasPlanningOverlayInputs || !Inputs.SamePlanLevel(LastPlanningOverlayInputs);
	LastPlanningOverlayInputs = Inputs;
	bHasPlanningOverlayInputs = true;
	++PlanningOverlayComputations;

	// 🔑 **Una sola variabile dice chi scrive la mappa** (`#3750`, spec §6.3): il fuoco o il piano, mai i due insieme, e
	// nessuno se le aree non si mostrano. E' derivata dagli ingressi, quindi l'uscita dimenticata si corregge da sola.
	const ERTPreviewSource Source = Inputs.ActiveSource();

	// 🔑 **La regola e' derivata dallo stato, non da un evento**: se le aree non si mostrano, si azzerano TUTTE, dallo
	// stesso reset dell'attore. Un'uscita che nessuno ha elencato (commit, playback, `Modal`, KO, fine partita) si
	// corregge qui al primo tick in cui lo stato la dichiara.
	if (Source == ERTPreviewSource::None)
	{
		if (bPlanChanged)
		{
			HexMap->ClearPlanningPreview();
		}
		return;
	}

	// ⛔ **Il solo cambio di cella NON rifa' ventaglio e portata** (criterio di `#711`): oggi nessuna area dipende
	// dall'hover. Quando `#3420` aggiungera' l'area sotto il cursore, il suo calcolo entra qui, accanto a questa riga.
	if (bPlanChanged)
	{
		++PlanningOverlayPlanComputations;
		if (Source == ERTPreviewSource::Focus)
		{
			// Le due aree del soggetto a fuoco, sulla mappa AUTOREVOLE ristretta a cio' che la squadra vede adesso (le celle
			// non in vista sono muri nel calcolo, P3) e dai soli VALORI del soggetto: il piano del nemico non entra.
			FVector Origin; float HexSize; float LayerH;
			const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerH);
			const ARTTurnManager* TM = OverlayTurnManager(Host);
			FRTFocusMoveAreas Areas;
			if (Map != nullptr && TM != nullptr)
			{
				Areas = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map,
					TM->PlaybackKnowledgeForTeam(Host.GetOverlayObserverTeamId()), Inputs.FocusMove);
			}
			RTPlanningPreview::Refresh(Host.GetOverlayWorld(), Unit, HexMap, &Areas);
		}
		else if (Source == ERTPreviewSource::SkillHover)
		{
			// La portata della skill sotto il cursore, a tre stati (`#3750`, fetta D): l'insieme e i verdetti si calcolano su una
			// mappa in cui ogni cella che la squadra non vede adesso e' neutra, e un verdetto si da' solo per una linea
			// interamente in vista. La mappa e' l'AUTOREVOLE: e' `URTRangeVerdictLibrary` a ripulirla.
			FVector Origin; float HexSize; float LayerH;
			const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerH);
			const ARTTurnManager* TM = OverlayTurnManager(Host);
			TArray<FRTRangeCellVerdict> Verdicts;
			if (Map != nullptr && TM != nullptr)
			{
				Verdicts = URTRangeVerdictLibrary::ClassifyFor(GetTransientPackage(), Map,
					TM->PlaybackKnowledgeForTeam(Host.GetOverlayObserverTeamId()), Inputs.SkillRange.Origin,
					Inputs.SkillRange.RangeCells, Inputs.SkillRange.Policy, Inputs.SkillRange.MinRangeCells);
			}
			RTPlanningPreview::Refresh(Host.GetOverlayWorld(), Unit, HexMap, nullptr, &Verdicts);
		}
		else
		{
			RTPlanningPreview::Refresh(Host.GetOverlayWorld(), Unit, HexMap);
		}
	}
}

namespace
{
	/**
	 * Un'impronta di `VisibleCells`, indipendente dall'ordine: le aree del fuoco e la portata di una skill dipendono da cio' che
	 * si vede, e se la vista cambia devono rifarsi. Somma di hash rimescolati piu' il conto (due viste con le stesse celle, in
	 * ordine diverso, danno la stessa impronta). Solo presentazione: una collisione lascerebbe un'area non rifatta fino al cambio
	 * successivo.
	 */
	uint32 VisibleCellsStamp(const TArray<FRTCellId>& Visible)
	{
		uint32 Stamp = static_cast<uint32>(Visible.Num());
		for (const FRTCellId& Cell : Visible)
		{
			Stamp += HashCombineFast(GetTypeHash(Cell), 0x9E3779B9u);
		}
		return Stamp;
	}
}

void FRTPlanningOverlayArbiter::SetSkillHover(const ARTUnit* Unit, int32 AbilityIndex)
{
	SkillHoverUnit = Unit;
	SkillHoverIndex = Unit != nullptr ? AbilityIndex : INDEX_NONE;
}

void FRTPlanningOverlayArbiter::ClearSkillHover(const ARTUnit* Unit, int32 AbilityIndex)
{
	// Solo se e' proprio QUESTO hover: lo slot A esce dopo che lo slot B e' entrato, e non deve cancellare B.
	if (SkillHoverUnit.Get() == Unit && SkillHoverIndex == AbilityIndex)
	{
		SkillHoverUnit = nullptr;
		SkillHoverIndex = INDEX_NONE;
	}
}

const ARTUnit* FRTPlanningOverlayArbiter::GetSkillHoverUnit() const
{
	return SkillHoverUnit.Get();
}

void FRTPlanningOverlayArbiter::GatherSkillHover(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit,
	FRTPlanningOverlayInputs& Inputs) const
{
	const ARTUnit* SkillOwner = SkillHoverUnit.Get();
	const int32 ObserverTeamId = Host.GetOverlayObserverTeamId();
	if (SkillOwner == nullptr || !SkillOwner->IsAlive())
	{
		return;
	}
	// Di chi puo' essere la skill sotto il cursore: della BARRA propria, cioe' dell'unita' comandata e della squadra di chi guarda; o
	// della barra in sola lettura (`#3748`), cioe' dell'unita' ispezionata e fissata, propria o nemica. Il confronto con `Unit` e' gia' del
	// risolutore (che scarta l'hover di chi non e' ne' l'una ne' l'altra prima di arrivare qui): qui e' la precondizione propria della
	// funzione. Il cancello di squadra e quello della fissazione non hanno altri guardiani.
	const bool bCommanded = SkillOwner == Unit && SkillOwner->TeamId == ObserverTeamId;
	const bool bFocused = Host.IsOverlayInspectorPinned() && SkillOwner == Host.GetOverlayInspectedUnit();
	if (!bCommanded && !bFocused)
	{
		return;
	}
	const ARTTurnManager* TM = OverlayTurnManager(Host);
	if (TM == nullptr)
	{
		return;
	}

	// 🔴 **Un nemico a fuoco: il cancello PRIMA di leggere la sua skill, e l'origine e' la cella della VOCE DELLA VISTA, mai il piano**
	// (P3, D-552). `AimOriginFor` leggerebbe lo scatto pianificato del nemico, e la portata mostrerebbe da dove mirera': il suo piano.
	// Il cancello e' lo stesso del fuoco, e un'unica funzione lo decide (`URTInspectLibrary::LiveEntry`): una voce `Live` nella vista di
	// chi guarda. Una voce `Remembered` ha la cella del RICORDO, che puo' essere una cella che oggi si vede: e' li' che il cancello conta.
	// `ARTUnit::IsKnownToObserver()` non decide niente. La vista costa una ricerca di unita' a ogni tick, ma solo mentre il cursore e'
	// su uno slot della barra in sola lettura.
	FRTKnowledgeView EnemyView;
	const FRTKnowledgeEntry* EnemyEntry = nullptr;
	if (SkillOwner->TeamId != ObserverTeamId)
	{
		TArray<ARTUnit*> Units;
		for (TActorIterator<ARTUnit> It(Host.GetOverlayWorld()); It; ++It)
		{
			Units.Add(*It);
		}
		EnemyView = URTHudViewModel::BuildObserverKnowledgeView(TM, Units, ObserverTeamId);
		EnemyEntry = URTInspectLibrary::LiveEntry(EnemyView, SkillOwner->StableUnitId);
		if (EnemyEntry == nullptr)
		{
			return;
		}
	}

	const URTActionData* Ability = SkillOwner->GetAbility(SkillHoverIndex);
	// Le stesse due condizioni della portata armata: l'azione chiede un bersaglio e ha una portata positiva. Una skill senza
	// portata non ha niente da mostrare, e il piano non deve cedere la mappa per niente.
	if (Ability == nullptr || !RTPlanningPreview::MostraLaPortata(SkillOwner, *Ability))
	{
		return;
	}

	// Un'unita' propria: stessa origine della portata armata (`AimOriginFor`), cosi' che l'insieme in hover coincida con quello
	// dell'azione armata. Il suo scatto pianificato e' un dato PROPRIO.
	const FRTCellId Origin = EnemyEntry != nullptr ? EnemyEntry->Cell : RTPlanningPreview::OrigineDiMira(SkillOwner, *Ability);
	const TArray<FRTCellId>& Visible = TM->PlaybackKnowledgeForTeam(ObserverTeamId).VisibleCells;
	// La precondizione di `URTRangeVerdictLibrary`: chi tira dev'essere su una cella che la squadra vede. Se non lo e' (uno
	// scatto pianificato verso una cella mai vista), non c'e' una portata da mostrare e il piano resta com'e'.
	if (!Visible.Contains(Origin))
	{
		return;
	}

	Inputs.HoverSkillUnit = SkillOwner;
	Inputs.SkillRange.Origin = Origin;
	Inputs.SkillRange.RangeCells = Ability->RangeCells;
	Inputs.SkillRange.MinRangeCells = Ability->Def.MinRangeCells;
	Inputs.SkillRange.Policy = Ability->Def.LineOfSightPolicy;
	Inputs.SkillKnowledgeStamp = VisibleCellsStamp(Visible);
	// Lo slot della barra propria (unita' comandata) e quello della barra in sola lettura (unita' ispezionata) sono due produttori della
	// stessa sorgente: l'arbitro li distingue da qui.
	Inputs.bSkillHoverOfInspected = !bCommanded;
}

void FRTPlanningOverlayArbiter::GatherFocus(const FRTPlanningOverlayHost& Host, FRTPlanningOverlayInputs& Inputs) const
{
	const ARTUnit* Subject = Host.GetOverlayInspectedUnit();
	if (Subject == nullptr || !Subject->IsAlive())
	{
		return;
	}
	const int32 ObserverTeamId = Host.GetOverlayObserverTeamId();
	// Un'unita' propria o alleata a fuoco non e' coperta da D-552, che parla dei nemici: resta fuori da F5 (scelta della
	// sessione). ⚠️ «Alleata» e' una squadra diversa dalla propria che nessuno ha ancora dichiarato tale: oggi non c'e' un
	// vocabolario di alleanze a runtime fuori dalla risoluzione del movimento.
	if (Subject->TeamId == ObserverTeamId)
	{
		return;
	}
	const ARTTurnManager* TM = OverlayTurnManager(Host);
	if (TM == nullptr)
	{
		return;
	}

	TArray<ARTUnit*> Units;
	for (TActorIterator<ARTUnit> It(Host.GetOverlayWorld()); It; ++It)
	{
		Units.Add(*It);
	}
	// 🔑 La stessa vista del velo. Il cancello e' UNO: una voce `Live` (P2). `ARTUnit::IsKnownToObserver()` nasce vero e qui
	// non decide niente.
	const FRTKnowledgeView View = URTHudViewModel::BuildObserverKnowledgeView(TM, Units, ObserverTeamId);
	const FRTKnowledgeEntry* Entry = URTInspectLibrary::LiveEntry(View, Subject->StableUnitId);
	if (Entry == nullptr)
	{
		return;
	}

	Inputs.FocusSubject = Subject;
	Inputs.FocusMove = URTFocusAreaLibrary::SubjectFor(Subject);
	// La cella e' quella della VOCE DELLA VISTA, non quella dell'attore: da qui in giu' si lavora su cio' che l'osservatore sa.
	Inputs.FocusMove.Cell = Entry->Cell;

	Inputs.FocusKnowledgeStamp = VisibleCellsStamp(TM->PlaybackKnowledgeForTeam(ObserverTeamId).VisibleCells);
}

FRTPlanningOverlayInputs FRTPlanningOverlayArbiter::Resolve(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit,
	const ARTHexMapActor* HexMap)
{
	FRTPlanningOverlayInputs Inputs = Gather(Host, Unit, HexMap);

	// Il cancello costa una ricerca di unita': lo si paga solo se c'e' qualcosa da decidere, cioe' una fissazione nuova o
	// un'area ancora sulla mappa. Un'ispezione che persiste dopo che l'area e' uscita non costa niente per fotogramma.
	if (InspectionPinSerial != FocusLatch.PinSerial || FocusLatch.bOnMap)
	{
		GatherFocus(Host, Inputs);
	}
	Inputs.bFocusOnMap = FocusLatch.Observe(InspectionPinSerial, Inputs);

	// L'hover su una skill (`#3750`, fetta D): costa solo se il cursore e' su uno slot. Non passa dal latch del fuoco: non e' un
	// gesto di pianificazione, e quando finisce torna la sorgente di prima.
	if (SkillHoverIndex != INDEX_NONE)
	{
		// L'hover e' di chi si comanda (la barra propria) o di chi e' a fuoco (la barra in sola lettura, `#3748`): chiusa la scheda, o
		// fissata un'altra unita', l'hover di uno slot che non c'e' piu' si scarta qui, senza aspettare l'uscita del cursore che Slate puo'
		// non mandare.
		//
		// 🔴 **L'hover dell'unita' ispezionata vale finche' la BARRA segue l'ispezione** (`bDockFollowsInspection`), non finche' la scheda e'
		// fissata: `SelectUnit` spegne il flag ma non chiude la scheda, e la barra torna all'unita' propria mentre il cursore e' ancora su uno
		// slot che Slate non ricostruisce. Con il solo `bInspectorPinned` la portata del nemico resterebbe sulla mappa sotto una barra che non e'
		// piu' sua. ⚠️ `PinInspection` accende il flag per OGNI fissazione, anche quella di un'unita' propria: per lei l'hover resta ammesso come
		// prima, e `IsDockReadOnly` (che guarda chi non si comanda) decide da solo se la barra e' in sola lettura.
		const ARTUnit* HoverOwner = SkillHoverUnit.Get();
		const bool bOwnedByInspected = Host.DoesOverlayDockFollowInspection() && Host.IsOverlayInspectorPinned() && HoverOwner != nullptr && HoverOwner == Host.GetOverlayInspectedUnit();
		if (HoverOwner != Unit && !bOwnedByInspected)
		{
			// La selezione e' cambiata con il cursore fermo sulla barra (`N`). Se il dock ricostruisce gli slot con il kit dell'unita'
			// nuova, Slate manda un ingresso al nuovo slot e `SetSkillHover` riscrive l'hover (visto in PIE il 2026-10-29, evidenza
			// `docs/technical/evidence/3750`); se riassegna lo stesso slot l'ingresso puo' non arrivare, e l'hover di una skill
			// dell'unita' di prima riapparirebbe alla prossima selezione di quella unita'. Si scarta qui in ogni caso.
			SkillHoverUnit = nullptr;
			SkillHoverIndex = INDEX_NONE;
		}
		else
		{
			GatherSkillHover(Host, Unit, Inputs);
		}
	}

	// 🔑 **Normalizzazione**: i campi di una sorgente valgono i default quando quella sorgente non tiene la mappa. Un soggetto la
	// cui area e' gia' uscita non deve far ricalcolare il piano quando si muove, e la portata di una skill non piu' sotto il
	// cursore nemmeno.
	const ERTPreviewSource Source = Inputs.ActiveSource();
	if (Source != ERTPreviewSource::Focus)
	{
		Inputs.FocusSubject = nullptr;
		Inputs.FocusMove = FRTFocusMoveSubject();
		Inputs.FocusKnowledgeStamp = 0;
		Inputs.bFocusOnMap = false;
	}
	if (Source != ERTPreviewSource::SkillHover)
	{
		Inputs.HoverSkillUnit = nullptr;
		Inputs.SkillRange = FRTRangeSubject();
		Inputs.SkillKnowledgeStamp = 0;
		Inputs.bSkillHoverOfInspected = false;
	}
	return Inputs;
}

void FRTPlanningOverlayArbiter::Update(const FRTPlanningOverlayHost& Host, ARTHexMapActor* KnownHexMap)
{
	ARTHexMapActor* HexMap = KnownHexMap ? KnownHexMap : ARTHexMapActor::FindInWorld(Host.GetOverlayWorld());
	if (!HexMap)
	{
		return;
	}
	const ARTUnit* Unit = Host.GetOverlayUnit();
	Apply(Host, HexMap, Unit, Resolve(Host, Unit, HexMap));
}

void FRTPlanningOverlayArbiter::Refresh(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit)
{
	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(Host.GetOverlayWorld());
	if (!HexMap)
	{
		return;
	}
	// La revisione sale PRIMA di leggere gli ingressi: un waypoint aggiunto non cambia nessun altro campo, e senza
	// questo salto il confronto lo scarterebbe come «niente di nuovo».
	++PlanningOverlayRevision;
	Apply(Host, HexMap, Unit, Resolve(Host, Unit, HexMap));
}
