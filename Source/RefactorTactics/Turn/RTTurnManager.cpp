#include "Turn/RTTurnManager.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTPlanValidationLibrary.h" // CP 38.2: la legalita' del piano si CHIEDE al commit
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTPredictiveLibrary.h" // boundary della Predictive Action (E18): la decisione sta nel puro
#include "Turn/RTReactionOpportunityTypes.h" // Decision Boundary dell'Overwatch (CP 14.5): opportunity e decisione
#include "Combat/RTOffensiveActionLibrary.h" // MakeSuppressiveZone: la zona dell'Overwatch E' quella della soppressione
#include "Perception/RTPerceptionLibrary.h" // TeamAwarenessOfCell: il trigger richiede `Rilevato` (ADR-0004 §6)
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h" // `EquipmentId`: distingue una reazione di loadout da una di kit
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTFacingLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Replay/RTMatchHistoryLibrary.h" // indice delle partite: una riga per partita, fuori dagli archivi (#416)
#include "Replay/RTReplayRecorderLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

ARTTurnManager::ARTTurnManager()
{
	// Tick abilitato solo durante il playback della risoluzione (presentazione, non logica).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARTTurnManager::BeginPlay()
{
	Super::BeginPlay();

	// ⚠️ La registrazione NON parte da sola qui, ed e' deliberato: `BeginPlay` gira anche per i 27 file di
	// test e per lo `ScenarioHarness` che spawnano un TurnManager a mano, e farli scrivere su disco
	// sarebbe un effetto collaterale che nessuno ha chiesto. La avvia chi allestisce una partita vera —
	// il GameMode — con `BeginReplayRecording()`.
	StartPlanningTimer();
}

void ARTTurnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsResolving)
	{
		TickPlayback(DeltaSeconds);
	}
}

FRTLogSubject FRTLogSubject::Unit(const ARTUnit* InUnit)
{
	FRTLogSubject S;
	S.Unit_ = InUnit;
	S.StableUnitId = InUnit ? InUnit->StableUnitId : INDEX_NONE;
	return S;
}

FRTLogSubject FRTLogSubject::Frozen(int32 InStableUnitId, const FRTKnowledgeVerdict& InVerdict)
{
	FRTLogSubject S;
	S.bFrozen = true;
	S.StableUnitId = InStableUnitId;
	S.FrozenVerdict = InVerdict;
	return S;
}

FRTLogSubject FRTLogSubject::World()
{
	FRTLogSubject S;
	S.bWorld = true;
	return S;
}

void ARTTurnManager::AddLogEvent(const FString& Message, FRTLogSubject Subject)
{
	// 🔴 Il log di SVILUPPO resta COMPLETO. E' diagnosi, non un canale del giocatore: mutilarlo renderebbe
	// impossibile capire una partita andata storta, e nessun avversario lo legge.
	UE_LOG(LogRT, Log, TEXT("[RT] %s"), *Message);

	FRTCombatLogLine Line;
	Line.Text = Message;
	Line.SubjectStableUnitId = Subject.GetStableUnitId();
	Line.Verdict = FreezeVerdictFor(Subject);

	RecentEvents.Add(MoveTemp(Line));
	while (RecentEvents.Num() > MaxLogLines)
	{
		RecentEvents.RemoveAt(0);
	}
}

FRTKnowledgeVerdict ARTTurnManager::FreezeVerdictFor(const FRTLogSubject& Subject) const
{
	// Il verdetto e' gia' stato deciso quando il fatto e' accaduto: si trasporta, non si ricalcola.
	if (Subject.HasFrozenVerdict())
	{
		return Subject.GetFrozenVerdict();
	}

	// Un fatto di mondo riguarda tutti, e lo dichiara: non e' l'assenza di una decisione.
	if (Subject.IsWorld())
	{
		return FRTKnowledgeVerdict::Everyone();
	}

	// Fail-closed se l'attore non c'e': `ClassifyTarget` vuole squadra e cella, e senza di esse una riga
	// non si legge — mai si legge per sbaglio.
	const ARTUnit* U = Subject.GetUnit();
	if (U == nullptr)
	{
		return FRTKnowledgeVerdict::NoOne();
	}

	FRTKnowledgeSubject S;
	S.StableUnitId = U->StableUnitId;
	S.TeamId = U->TeamId;
	S.Cell = U->Cell;   // la cella di ADESSO, che al momento della scrittura e' quella del fatto
	S.bAlive = U->IsAlive();

	return URTTeamKnowledgeLibrary::FreezeVerdict(TeamKnowledgeState, S);
}

TArray<FString> ARTTurnManager::GetRecentEvents() const
{
	// Vista COMPLETA, non filtrata: e' la forma che RecentEvents aveva prima di portare un soggetto. Chi
	// disegna per un giocatore chiama GetRecentEventsForTeam, non questa.
	TArray<FString> Out;
	Out.Reserve(RecentEvents.Num());
	for (const FRTCombatLogLine& L : RecentEvents)
	{
		Out.Add(L.Text);
	}
	return Out;
}

TArray<FString> ARTTurnManager::ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines,
	int32 ObserverTeamId)
{
	TArray<FString> Out;
	Out.Reserve(Lines.Num());
	for (const FRTCombatLogLine& L : Lines)
	{
		// 🔴 **Nessuna vista costruita qui, ed e' il punto di [D-223].** La domanda «puo' leggerla?» ha gia'
		// una risposta, decisa quando la riga e' nata: interrogare la conoscenza di ADESSO risponderebbe a
		// una domanda diversa, e per un soggetto nel frattempo distrutto non risponderebbe affatto.
		//
		// ⚠️ Il verdetto e' fail-closed di default: una riga che arrivasse qui senza verdetto non si legge.
		if (L.Verdict.AllowsTeam(ObserverTeamId))
		{
			Out.Add(L.Text);
		}
	}
	return Out; // ordine di produzione, mai riordinato
}

TArray<FRTCellId> ARTTurnManager::VisibleTrailFor(const FRTMoveRoute& Route, int32 ObserverTeamId)
{
	TArray<FRTCellId> Trail;

	// Fail-closed sul disallineamento: senza questo controllo un `Cells.Add` futuro senza il verdetto
	// corrispondente leggerebbe fuori dall'array dei verdetti. Una rotta malformata non si disegna.
	if (Route.CellVerdicts.Num() != Route.Cells.Num())
	{
		return Trail;
	}

	Trail.Reserve(Route.Cells.Num());
	for (int32 i = 0; i < Route.Cells.Num(); ++i)
	{
		// 🔴 `break`, non `continue`. Riprendere dopo un buco unirebbe due celle NON adiacenti con un
		// segmento dritto, che passa esattamente sopra il tratto da nascondere: peggio che non filtrare.
		if (!Route.CellVerdicts[i].AllowsTeam(ObserverTeamId))
		{
			break;
		}
		Trail.Add(Route.Cells[i]);
	}
	return Trail;
}

namespace
{
	/** Gli osservatori di una squadra, alle posizioni con cui si decide il verdetto della traccia. */
	struct FRTRouteObserverTeam
	{
		int32 TeamId = INDEX_NONE;
		TArray<FRTPerceiver> Observers;
	};

	/**
	 * Un gruppo di osservatori per ogni squadra VIVA, dalle posizioni correnti degli attori.
	 *
	 * ⚠️ **Chiamata PRIMA di `PlaceOnCell`**, quindi le celle sono quelle di inizio fase: e' il campione
	 * dichiarato come limite in [D-223] — `TeamKnowledgeState` ha due sole assegnazioni per turno, entrambe
	 * per fase, e un campione per micro-step non esiste. Risponde bene quando a nascondere e' il movimento
	 * del NEMICO, sbaglia quando e' quello dell'osservatore.
	 *
	 * Costruita UNA volta per risoluzione e non per rotta: `VisibleCells` ricostruisce l'area visibile a
	 * ogni chiamata, e rifarla per ogni cella di ogni percorso sarebbe lo stesso lavoro moltiplicato.
	 *
	 * L'ordine e' quello dei `TeamId` crescenti, come `RefreshTeamKnowledgeForPlanning`: l'ordine di un
	 * `TSet` dipende dall'hash, e qui si itera (invariante #3).
	 */
	TArray<FRTRouteObserverTeam> BuildRouteObserverTeams(const TArray<ARTUnit*>& Units)
	{
		TSet<int32> Teams;
		for (const ARTUnit* U : Units)
		{
			if (IsValid(U) && U->IsAlive()) { Teams.Add(U->TeamId); }
		}
		TArray<int32> SortedTeams = Teams.Array();
		SortedTeams.Sort();

		TArray<FRTRouteObserverTeam> Out;
		Out.Reserve(SortedTeams.Num());
		for (int32 TeamId : SortedTeams)
		{
			FRTRouteObserverTeam Entry;
			Entry.TeamId = TeamId;
			for (const ARTUnit* U : Units)
			{
				// Un cadavere non vede: stessa guardia di `RefreshTeamKnowledgeForPlanning`.
				if (!IsValid(U) || !U->IsAlive() || U->TeamId != TeamId) { continue; }
				FRTPerceiver P;
				P.Cell = U->Cell;
				P.Facing = U->Facing;
				P.VisionRange = U->VisionRange;
				Entry.Observers.Add(P);
			}
			Out.Add(MoveTemp(Entry));
		}
		return Out;
	}

	/**
	 * Chi puo' vedere disegnata UNA cella percorsa da un soggetto di `SubjectTeamId` ([D-223]).
	 *
	 * 🔴 **La squadra del soggetto vede sempre la propria traccia**, e non passa dalla percezione: sai dove
	 * si e' mosso il tuo anche se in quel momento nessun compagno lo guardava. E' lo stesso ramo che
	 * `ARTHUD::ShouldDrawUnitOverlay` risolve con `bIsOwnTeam` e che `ClassifyTarget` risolve con
	 * `TargetTeamId == Knowledge.TeamId`: senza, la traccia di un proprio esploratore sparirebbe a chi lo ha
	 * mandato avanti, che non e' conoscenza parziale ma amnesia.
	 *
	 * ⚠️ **Il predicato e' `TeamAwarenessOfCell`, quello che la produzione usa gia'** nel ciclo a micro-step
	 * del Move: riscrivere qui un confronto su `VisibleCells` sarebbe la terza via che [D-223] vieta — due
	 * riletture della stessa regola, libere di divergere.
	 *
	 * Fail-closed per costruzione: una squadra assente da `Teams` non riceve il bit, e senza mappa il
	 * verdetto e' `NoOne()` — la traccia sparisce, mai il contrario.
	 */
	FRTKnowledgeVerdict FreezeRouteCellVerdict(const URTHexMapAsset* Map,
		const TArray<FRTRouteObserverTeam>& Teams, int32 SubjectTeamId, const FRTCellId& Cell)
	{
		FRTKnowledgeVerdict Verdict;
		for (const FRTRouteObserverTeam& Team : Teams)
		{
			if (Team.TeamId == SubjectTeamId
				|| URTPerceptionLibrary::TeamAwarenessOfCell(Map, Team.Observers, Cell) == ERTAwareness::Detected)
			{
				Verdict.AllowTeam(Team.TeamId);
			}
		}
		return Verdict;
	}

	/** I verdetti di una rotta intera, uno per cella e nello stesso ordine. */
	void FreezeRouteVerdicts(const URTHexMapAsset* Map, const TArray<FRTRouteObserverTeam>& Teams,
		int32 SubjectTeamId, const TArray<FRTCellId>& Cells, TArray<FRTKnowledgeVerdict>& Out)
	{
		Out.Reset();
		Out.Reserve(Cells.Num());
		for (const FRTCellId& Cell : Cells)
		{
			Out.Add(FreezeRouteCellVerdict(Map, Teams, SubjectTeamId, Cell));
		}
	}
}

TArray<FString> ARTTurnManager::GetRecentEventsForTeam(int32 ObserverTeamId) const
{
	// ✅ **Venticinque righe sparite con [D-223], e vale la pena dire cosa facevano**: `GetAllActorsOfClass`,
	// la costruzione dei `Subjects` e una `ViewForTeam` completa, a ogni frame di disegno, per rispondere a
	// una domanda che ogni riga porta gia' con se'.
	//
	// 🔴 E non era solo lavoro sprecato: era la domanda SBAGLIATA. La vista si costruiva sulle unita' del
	// mondo di ADESSO, quindi un'unita' distrutta a fine turno non c'era piu' fra i soggetti e ogni riga
	// che la nominava spariva — anche per la squadra che l'aveva vista cadere.
	return ComposeVisibleLogLines(RecentEvents, ObserverTeamId);
}

TArray<FRTCellId> ARTTurnManager::CellsEnteredAlong(const TArray<FRTCellId>& Path)
{
	// La cella di PARTENZA non e' una cella «entrata»: l'unita' ci stava gia'. Confonderle farebbe applicare
	// due volte gli effetti di quella cella — una all'arrivo e una alla partenza dello spostamento successivo —
	// e per un fuoco significherebbe il doppio del danno dichiarato dal catalogo.
	TArray<FRTCellId> Entered;
	if (Path.Num() < 2) { return Entered; }
	Entered.Reserve(Path.Num() - 1);
	for (int32 I = 1; I < Path.Num(); ++I)
	{
		Entered.Add(Path[I]);
	}
	return Entered;
}

/**
 * La voce di NASCITA di uno stato (#1077), costruita in UN posto solo.
 *
 * 🔴 **Era copiata in tre siti, e i tre sono derivati esattamente nei campi che non condividevano** —
 * trovato in code review: la fase, la cella e la regola del sentinella. Un helper non e' eleganza qui,
 * e' il punto in cui quelle tre regole stanno scritte una volta.
 *
 * ⚠️ **La regola del sentinella sta QUI**: `ApplyStatus` accetta `PersistentWhileOnCell` (-1) da
 * qualunque chiamante, e -1 **non e' una durata**. La forma di vita finisce nell'esito e `Amount` porta
 * un conteggio solo quando un conteggio esiste — prima lo garantiva il solo sito del terreno, quindi la
 * spec lo dichiarava per tutti e valeva per uno su tre.
 */
FRTTurnLogEntry ARTTurnManager::MakeStatusBirthEntry(ERTMatchPhase InPhase, FGameplayTag Tag, const FRTCellId& Cell,
	int32 RequestedTurns, bool bFromTerrain)
{
	FRTTurnLogEntry E;
	E.Phase = InPhase;
	E.Category = ERTLogCategory::Status;
	E.ActionId = Tag.GetTagName();
	E.SrcCell = Cell;
	E.TgtCell = Cell;
	const bool bLegatoAllaCella = (RequestedTurns == ARTUnit::PersistentWhileOnCell);
	E.Amount = bLegatoAllaCella ? 0 : RequestedTurns;
	E.Outcome = static_cast<uint8>(bLegatoAllaCella
		? ERTStatusOutcome::AppliedWhileOnCell
		: (bFromTerrain ? ERTStatusOutcome::AppliedByTerrain : ERTStatusOutcome::AppliedByAction));
	return E;
}

/** La voce di MORTE di uno stato: revoca (una mossa) o scadenza (il tempo). Sempre nel Cleanup. */
void ARTTurnManager::ApplyStatusLogged(ARTUnit* Unit, FGameplayTag Tag, int32 Turns)
{
	if (Unit == nullptr)
	{
		return;
	}

	// 🔴 **Un solo posto scrive la voce di spegnimento** (`#1314`). La regola «`Wet` toglie `Burning`»
	// vive in `ARTUnit::ApplyStatus` proprio per valere su ogni sorgente di bagnato; se la VOCE la
	// scrivessero i quattro siti che applicano uno status, il primo che se ne dimenticasse renderebbe muto
	// un percorso — che e' esattamente il difetto misurato da questa issue.
	if (Unit->ApplyStatus(Tag, Turns))
	{
		FRTTurnLogEntry Spento =
			MakeStatusDeathEntry(TAG_Status_Burning, Unit->Cell, ERTStatusOutcome::Extinguished);
		AppendLogEntry(Spento, Unit);
	}
}

FRTTurnLogEntry ARTTurnManager::MakeStatusDeathEntry(FGameplayTag Tag, const FRTCellId& Cell, ERTStatusOutcome Outcome)
{
	FRTTurnLogEntry E;
	E.Phase = ERTMatchPhase::Cleanup;
	E.Category = ERTLogCategory::Status;
	E.ActionId = Tag.GetTagName();
	E.SrcCell = Cell;
	E.TgtCell = Cell;
	E.Outcome = static_cast<uint8>(Outcome);
	return E;
}

void ARTTurnManager::ApplyTerrainOnEnterEffects(const URTHexMapAsset* Map, ARTUnit* Unit,
	const TArray<FRTCellId>& Entered, ERTMatchPhase InPhase)
{
	if (!Map || !Unit) { return; }

	for (const FRTCellId& Cell : Entered)
	{
		const FRTHexCellData* CellData = Map->FindCell(Cell);
		if (!CellData) { continue; }

		// Gli effetti li DICHIARA il catalogo terreni, non uno switch qui: aggiungere un terreno pericoloso
		// (o cambiarne i numeri) non tocca il resolver. Vocabolario condiviso con le azioni (FRTActionEffectSpec).
		const FRTTerrainDef Terrain = URTTerrainLibrary::FindTerrainDef(CellData->Surface);
		for (const FRTActionEffectSpec& Effect : Terrain.OnEnterEffects)
		{
			if (Effect.Effect == ERTActionEffect::Damage)
			{
				const int32 HpPrima = Unit->Health; // serve DOPO, per classificare l'esito
				// `Environmental`: e' danno da TERRENO, non un colpo. Lo scudo base non lo ferma ([D-224]).
				const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Effect.Amount,
					ERTDamageSource::Environmental, Unit->Shield, Unit->GetTemporaryShield(), Unit->Health);
				// ApplyCombatState, non l'assegnazione diretta: e' la stessa contabilita' del danno da azione ed
				// e' l'unica che erode anche TemporaryShield. Scrivendo Health/Shield a mano lo scudo temporaneo
				// resterebbe al valore vecchio e il Cleanup lo sottrarrebbe una seconda volta.
				Unit->ApplyCombatState(Result.Health, Result.Shield);

				// ➕ **La voce canonica del danno da terreno** (`#1067`), gemella di quella del `Burning`
				// (`#625`). Fino al 2026-08-16 questo danno esisteva **solo** in `AddLogEvent` — un `UE_LOG`
				// piu' un buffer circolare troncato — e chi entrava nel fuoco con pochi HP **moriva senza
				// lasciare niente**: nessun `Lethal`, nessun soggetto, e `DescribeFirstDivergence` senza un
				// punto da nominare. Era il pezzo PIU' GROSSO dei due: `Fire` fa 10 danni all'ingresso
				// contro gli 8 del Cleanup.
				//
				// ⚠️ Categoria `Combat` e causa in `ActionId`, come per il `Burning`: la domanda e' «quanti
				// punti vita ha cambiato, e a chi». Qui la causa e' la **superficie**, non uno status —
				// `Terrain.<Surface>` — cosi' il replay distingue i due danni del fuoco senza doverli
				// dedurre dalla fase.
				FRTTurnLogEntry Hazard;
				Hazard.Phase = InPhase;
				Hazard.Category = ERTLogCategory::Combat;
				// Il prefisso arriva dalla libreria del TurnLog e non da un letterale: lo condivide con
				// `IsEnvironmentalDamage`, che su questa causa decide se `UnitId` sia chi agisce o chi subisce
				// (`#1150`). Due letterali uguali per abitudine divergono al primo che cambia (`D-098`).
				Hazard.ActionId = FName(*FString::Printf(TEXT("%s%s"),
					URTTurnLogLibrary::TerrainCausePrefix(),
					*StaticEnum<ERTHexSurface>()->GetNameStringByValue((int64)CellData->Surface)));
				// La cella che ha colpito, che qui **e' davvero la causa** — al contrario del `Burning`, che
				// segue l'unita' anche fuori dal fuoco. `TgtCell` e' la stessa: chi subisce ci sta sopra.
				Hazard.SrcCell = Cell;
				Hazard.TgtCell = Cell;
				Hazard.Amount = Effect.Amount;
				Hazard.Outcome = static_cast<uint8>(
					URTCombatLibrary::ClassifyCombatOutcome(HpPrima, Result.Health, /*AttackerDmgBonus*/ 0));
				// Il soggetto e' chi subisce: in un danno da terreno non c'e' un attaccante. Stessa scelta di
				// `#625`, con la stessa tensione dichiarata rispetto a «`UnitId` = chi ha agito».
				AppendLogEntry(Hazard, Unit);

				// ⚠️ `AddLogEvent` **resta**: e' la vista leggibile, non la traccia.
				AddLogEvent(FString::Printf(TEXT("%s: %d danni da terreno (q=%d,r=%d,L%d)"),
					*Unit->GetName(), Effect.Amount, Cell.X, Cell.Y, Cell.Layer), FRTLogSubject::Unit(Unit));
			}
			else if (Effect.Effect == ERTActionEffect::Status)
			{
				// Durata 0 nel catalogo = "finche' sulla cella" (CP 8.2): qui diventa la sentinella che
				// ARTUnit riconosce. La revoca la fa il Cleanup leggendo lo STESSO catalogo, non una
				// seconda tabella (URTTerrainLibrary::CellBoundStatusesFor).
				const int32 Duration = (Effect.StatusDuration == 0)
					? ARTUnit::PersistentWhileOnCell
					: Effect.StatusDuration;
				ApplyStatusLogged(Unit, Effect.StatusTag, Duration);
				// #1077: la NASCITA dello stato entra nel TurnLog. La forma di vita sta nell'esito, non in
				// `Amount`: `PersistentWhileOnCell` vale -1 e non e' una durata.
				// ⚠️ **`InPhase`, non il membro `Phase`**: durante la Cleanup il membro vale `Planning` — la
				// firma di questa funzione lo dichiara in otto righe, e la prima stesura di questa voce le
				// aveva ignorate. Un `Fire` che concede `Burning` nella Cleanup avrebbe scritto una fase mai
				// avvenuta, per giunta dentro l'hash del turno. Trovato in code review.
				// ⚠️ E la cella e' `Cell`, quella del ciclo che ha concesso lo stato, non `Unit->Cell`:
				// `ResolveMovement` chiama qui DOPO `PlaceOnCell`, quindi il membro e' gia' la cella finale
				// e uno stato preso a meta' percorso sarebbe stato ancorato all'arrivo. La voce `Hazard`
				// venti righe sopra usa `Cell` per la stessa ragione.
				if (Duration == ARTUnit::PersistentWhileOnCell || Duration > 0)
				{
					FRTTurnLogEntry Nato = MakeStatusBirthEntry(InPhase, Effect.StatusTag, Cell, Duration,
						/*bFromTerrain=*/ true);
					AppendLogEntry(Nato, Unit);
				}
				AddLogEvent(FString::Printf(TEXT("%s: %s da terreno"), *Unit->GetName(), *Effect.StatusTag.ToString()), FRTLogSubject::Unit(Unit));
			}
		}
	}
}

void ARTTurnManager::RefreshTeamKnowledgeForPlanning(const TArray<ARTUnit*>& Live)
{
	FVector Origin; float CellSize = 0.f; float LayerH = 0.f;
	const URTHexMapAsset* Map = GetHexContext(Origin, CellSize, LayerH);

	TSet<int32> Teams;
	for (const ARTUnit* U : Live) { if (U && U->IsAlive()) { Teams.Add(U->TeamId); } }
	TArray<int32> SortedTeams = Teams.Array();
	SortedTeams.Sort(); // l'ordine di un TSet dipende dall'hash: qui si itera, quindi si ordina

	TArray<FRTTeamKnowledge> Refreshed;
	for (int32 TeamId : SortedTeams)
	{
		TArray<FRTPerceiver> Observers;
		TArray<FRTLastKnownContact> EnemiesNow;
		for (ARTUnit* U : Live)
		{
			if (!U || !U->IsAlive()) { continue; } // un cadavere non vede e non si nasconde
			if (U->TeamId == TeamId)
			{
				FRTPerceiver P;
				P.Cell = U->Cell;
				P.Facing = U->Facing;
				P.VisionRange = U->VisionRange;
				Observers.Add(P);
			}
			else
			{
				// Identita' STABILE: e' la chiave con cui la memoria attraversa i turni. `TurnNumber` in
				// ingresso e' ignorato — lo scrive `Observe`, unica a sapere QUANDO si osserva.
				EnemiesNow.Add(FRTLastKnownContact(U->StableUnitId, U->Cell, /*ignorato*/ 0));
			}
		}
		Refreshed.Add(URTTeamKnowledgeLibrary::Observe(Map, TeamId, TurnNumber,
			Observers, EnemiesNow, KnowledgeForTeam(TeamId)));
	}
	TeamKnowledgeState = MoveTemp(Refreshed);
	// Chi disegna il velo rilegge QUI, non a `Tick` ([D-227]).
	OnTeamKnowledgeRefreshed.Broadcast(TurnNumber);
}

void ARTTurnManager::RefreshTeamKnowledgeNow()
{
	// Le unita' VIVE, con la stessa via che usa `PlanBots`: `MakeCurrentSnapshot` le raccoglie ordinate e
	// scarta i morti. Riusarla — invece di un secondo `GetAllActorsOfClass` qui — tiene una sola definizione
	// di «chi partecipa»: due raccolte diverse divergerebbero al primo cambio di regola sui cadaveri.
	TArray<ARTUnit*> Units;
	MakeCurrentSnapshot(Units);

	// ⚠️ Con `Units` vuoto questa chiamata NON e' un no-op innocuo: `RefreshTeamKnowledgeForPlanning`
	// costruisce l'elenco delle squadre DAI vivi, quindi zero unita' -> zero squadre -> `TeamKnowledgeState`
	// vuoto, cioe' esattamente lo stato che questa funzione esiste per superare. Chi la chiama deve averle
	// gia' spawnate; il difetto di `#1762` nasce proprio dal chiamarla (indirettamente) troppo presto.
	RefreshTeamKnowledgeForPlanning(Units);
}

void ARTTurnManager::PlanBots()
{

	// Osservabilita' del tuning: i pesi correnti, una riga per turno (verifica delle modifiche in PIE).
	// UE_LOG diretto (non AddLogEvent) per non riempire il combat log della HUD.
	UE_LOG(LogRT, Log, TEXT("[RT] Pesi bot: WKill=%d WDamage=%d WThreat=%d WKiteViolation=%d WApproach=%d WElevation=%d"),
		WKill, WDamage, WThreat, WKiteViolation, WApproach, WElevation);

	// 🔴 **L'invariante si verifica sull'ISTANZA, e una volta per partita** (`#1276`).
	//
	// `WElevation * MaxLayer < WApproach` e' l'unica difesa contro lo stato assorbente di `#1088` — il bot
	// che si parcheggia in quota e la partita che non si decide. L'header lo dichiara accanto al campo, e
	// dichiara anche il modo di riaprirlo: *«alzarlo da qui in editor lo riapre»*.
	//
	// ⚠️ **Il test che lo pinna legge `GetDefault<ARTTurnManager>()`, cioe' il CDO.** Ma `ARTGameMode`
	// RIUSA un `ARTTurnManager` gia' presente nel livello invece di spawnarlo, e un'istanza piazzata
	// serializza i propri `UPROPERTY` nel `.umap`: un livello che portasse ancora `WElevation = 20`
	// riaprirebbe il difetto **mentre il test resta verde**. Un test non puo' vederlo — non carica i
	// livelli — quindi il presidio sta qui, dove i pesi e la mappa sono entrambi quelli veri.
	//
	// ⚠️ E vale anche per `MaxLayer`: l'invariante non e' una proprieta' dei soli pesi, ma del loro rapporto
	// con la mappa in gioco. Gli stessi pesi reggono su due layer e cedono su tre.
	if (!bBotWeightInvariantChecked)
	{
		bBotWeightInvariantChecked = true;

		FVector InvOrigin; float InvHexSize; float InvLayerH;
		if (const URTHexMapAsset* Map = GetHexContext(InvOrigin, InvHexSize, InvLayerH))
		{
			int32 MaxLayer = 0;
			for (const FRTHexCellData& Cell : Map->Cells)
			{
				MaxLayer = FMath::Max(MaxLayer, Cell.Id.Layer);
			}

			if (WElevation * MaxLayer >= WApproach)
			{
				// Fail-loud e non fail-closed: la partita si gioca lo stesso, ma chi raccoglie una misura di
				// bilanciamento deve sapere che questa non vale. Un peso serializzato invisibile falserebbe
				// qualunque numero raccolto su `#149`.
				UE_LOG(LogRT, Error,
					TEXT("[RT] INVARIANTE PESI BOT VIOLATA: WElevation(%d) * MaxLayer(%d) = %d >= WApproach(%d). "
						 "Il bot puo' parcheggiarsi in quota (#1088) e la partita puo' non decidersi. "
						 "Controlla i pesi sull'ARTTurnManager di QUESTO livello: un'istanza serializzata nel "
						 ".umap vince sui default C++."),
					WElevation, MaxLayer, WElevation * MaxLayer, WApproach);
			}
		}
	}

	// Stesso snapshot autorevole del movimento e dell'input: mappa, occupazione e budget congelati.
	// Le mosse candidate nascono da ReachableCells, quindi il bot NON puo' proporre una mossa illegale
	// (niente celle inesistenti, bloccate, occupate o fuori budget) e non rifa' pathfinding per conto suo.
	// CP 13.5 — l'IDENTITA' dev'esistere prima della conoscenza, perche' e' la sua chiave.
	//
	// `EnsureMatchRoster` viveva solo in `LockInAndResolve`, cioe' a valle della pianificazione: al primo
	// turno ogni `StableUnitId` valeva ancora **0**. Finche' nessuno leggeva quel campo in pianificazione non
	// si vedeva; adesso lo legge `ClassifyTarget`, e con tutti gli id a zero i contatti di unita' diverse si
	// sarebbero fusi in uno — il ricordo di un nemico avrebbe risposto per un altro.
	// La funzione e' idempotente per costruzione («l'identita' si assegna una volta»), quindi chiamarla anche
	// qui non riassegna niente: sposta solo *quando* la prima assegnazione avviene.
	EnsureMatchRoster();

	TArray<ARTUnit*> Units;
	const FRTHexSnapshot BaseSnapshot = MakeCurrentSnapshot(Units); // solo unita' vive; Units[i].UnitId == i

	// #1088 — UNO SNAPSHOT DI PIANIFICAZIONE PER SQUADRA, e non era cosi'.
	//
	// Fino a qui ogni bot pianificava sullo stesso snapshot congelato prima del ciclo: la seconda unita' non
	// sapeva cosa avesse scelto la prima, sceglieva la stessa cella, e la risoluzione simultanea le fermava
	// entrambe (`BlockedContested`). Deterministico, quindi il turno dopo ricreava la contesa identica — e la
	// partita si bloccava. Misurato sull'arena spedita: 24 contese in 12 turni, TUTTE fra compagni di
	// squadra, zero mosse.
	//
	// ⛔ **Per squadra, e non e' un'ottimizzazione: e' fairness** (CP 13.5, `RT-FEAT-BOT-FAIRNESS`). Le
	// prenotazioni sono informazione sui piani, e i piani di una squadra sono privati: con un solo snapshot
	// condiviso un bot eviterebbe la cella dove sta per andare un AVVERSARIO, cioe' schiverebbe un intento
	// che nessun giocatore puo' vedere. Due squadre che si contendono la stessa cella devono continuare a
	// contendersela — quella e' una collisione legittima, e la risolve il resolver.
	// ⚠️ Costruiti TUTTI in anticipo, e non su richiesta dentro il ciclo: un `Add` in corsa puo' riallocare
	// la mappa e invalidare un riferimento gia' preso — e quel riferimento resterebbe vivo per l'intero corpo
	// dell'iterazione. Con le squadre note prima, la mappa non viene piu' toccata mentre qualcuno la guarda.
	TMap<int32, FRTHexSnapshot> PlanningSnapshots;
	for (const ARTUnit* U : Units)
	{
		if (U && U->bIsBotControlled && !PlanningSnapshots.Contains(U->TeamId))
		{
			PlanningSnapshots.Add(U->TeamId, BaseSnapshot);
		}
	}

	// Prenota nello snapshot della squadra la rotta che il bot ha appena scelto.
	//
	// 🔴 **LIMITE MISURATO, e va letto prima di credere che questa prenotazione basti.** Cio' che arriva
	// all'esecuzione e' la DESTINAZIONE, non la rotta: `ResolveMovement` ricalcola il percorso su uno
	// snapshot fresco, dove nessuna prenotazione esiste, quindi per ogni compagna dopo la prima la rotta
	// eseguita puo' tornare a essere quella DIRETTA — proprio quella che la prenotazione aveva scartato.
	// ∴ questa prenotazione garantisce **destinazioni distinte**, non **percorsi disgiunti**, e la meta' del
	// difetto di #1088 fatta di collisioni di percorso (12 contese su 24) resta possibile in linea di
	// principio. Sulla configurazione spedita non si osserva — misurato, `fermo: cella contesa` = 0 in 12
	// round — ma «non osservato» non e' «impedito».
	//
	// ⛔ **Fissare qui `PlannedPath` NON e' la soluzione, ed e' stato provato**: `ResolveMovement` accetta un
	// `PlannedPath` gia' pronto **senza** riapplicare l'occupazione fresca — la sua validazione autorevole
	// vive nel ramo che ricalcola — e una rotta scelta in pianificazione e' vecchia di due fasi (Dash e Blast
	// muovono, spingono e uccidono). Il risultato misurato e' **due unita' sulla stessa cella**
	// (`HexMatch.TestArenaKeepsUnitsOnLegalCells`, turno 9). La correzione giusta e' far accumulare le rotte
	// **dentro** `ResolveMovement`, dove l'occupazione e' fresca e varrebbe anche per le unita' umane: e'
	// piu' larga di #1088 e va aperta a parte.
	//
	// ⚠️ **Solo il movimento NORMALE**, e la ragione e' la geometria: lo scatto ha traiettoria LINEARE mentre
	// `ReservePlannedRoute` cammina il grafo, quindi per uno scatto prenoterebbe celle che non verranno
	// attraversate. Della fase Dash si prenota la sola cella d'ARRIVO — vedi piu' sotto: e' li' che l'unita'
	// si trovera' quando il Move gira, quindi e' l'unica che una compagna non deve poter scegliere.
	auto ReserveNormalMove = [](FRTHexSnapshot& TeamSnapshot, ARTUnit* PlannedBot, int32 PlannedIdx)
	{
		if (!PlannedBot)
		{
			return;
		}
		if (PlannedBot->PlannedDashAbility != INDEX_NONE)
		{
			// Scatta: la rotta e' della fase Dash e non passa di qui, ma la cella su cui ATTERRA sara'
			// occupata quando il Move risolve. Senza prenotarla, una compagna la sceglie come destinazione e
			// al proprio turno di movimento trova la strada sbarrata: un turno speso per niente.
			if (!(PlannedBot->PlannedDashCell == PlannedBot->Cell)
				&& !TeamSnapshot.Occupancy.Contains(PlannedBot->PlannedDashCell))
			{
				TeamSnapshot.Occupancy.Add(PlannedBot->PlannedDashCell, PlannedIdx);
			}
			return;
		}
		URTHexBotLibrary::ReservePlannedRoute(TeamSnapshot, PlannedIdx, PlannedBot->PlannedCell);
	};

	// CP 13.5 — la conoscenza dev'esistere PRIMA che qualcuno ci pianifichi sopra. `ResolveCombat` la
	// rinfresca a valle del Dash, cioe' DOPO: al primo turno sarebbe vuota, e un bot che pianifica su una
	// conoscenza vuota non e' parziale, e' cieco — il filtro sembrerebbe funzionare mentre produce un bot
	// che non fa niente.
	RefreshTeamKnowledgeForPlanning(Units);

	// Gli id di TUTTO l'equipaggiamento spedito, per distinguere un'abilita' concessa dal LOADOUT da una del
	// KIT (`#1403`, [D-220]). Si chiede al catalogo, non all'indice.
	//
	// ⚠️ **Tutto l'equipaggiamento, non i soli moduli reazione**: `EquipLoadout` passa da
	// `MakeEquipmentAction` per ogni pezzo non-arma, **gadget compresi**, e quella funzione scrive
	// `Def.ActionId = Item->EquipmentId` per tutti. Un gadget costruito su un'azione di slot reazione
	// finirebbe archiviato fra le abilita' di kit — e sarebbe la stessa «origine per accidente» che questa
	// riga esiste per togliere, un livello piu' sotto.
	//
	// ⚠️ **`static`, quindi una volta per processo**: `FindEquipment` ricostruisce i tre cataloghi a ogni
	// chiamata — **diciassette** `NewObject` piu' le `FText` — e `PlanBots` gira a ogni turno. E' l'idioma
	// che `DefaultReactionModuleFor` e `DefaultGadgetFor` gia' usano due funzioni piu' su.
	static const TSet<FName> IdEquipaggiamento = []()
	{
		TSet<FName> Ids;
		for (const TArray<URTEquipmentData*>& Catalogo :
			{ URTCatalogLibrary::MakeWeaponVariants(), URTCatalogLibrary::MakeGadgets(),
			  URTCatalogLibrary::MakeReactionModules() })
		{
			for (const URTEquipmentData* Pezzo : Catalogo)
			{
				if (Pezzo) { Ids.Add(Pezzo->EquipmentId); }
			}
		}
		return Ids;
	}();

	for (int32 BotIdx = 0; BotIdx < Units.Num(); ++BotIdx)
	{
		ARTUnit* Bot = Units[BotIdx];
		if (!Bot->bIsBotControlled)
		{
			continue;
		}

		Bot->PlannedCell = Bot->Cell;   // default: fermo
		Bot->PlannedAttackTarget = nullptr;
		Bot->PlannedAbilityIndex = INDEX_NONE;
		Bot->PlannedPath.Reset();       // il bot pianifica destinazioni, non percorsi a waypoint
		Bot->PlannedWaypoints.Reset();
		// 🔴 Anche lo SCATTO, che fino al 2026-08-25 restava fuori da questo azzeramento: quattro campi
		// su cinque ripartivano da zero e il quinto no. Un dash che il resolver non ha consumato — perche'
		// la sua destinazione non era piu' raggiungibile, o perche' l'azione era in ricarica — sopravviveva
		// alla ripianificazione e si sommava al movimento deciso in QUESTO turno.
		//
		// ⚠️ Il difetto era invisibile finche' la carica occupava la principale: `[Ram(Main), Move(Movement)]`
		// e' un piano legale, e nessuno guardava. Con [D-191] la carica e' mobilita', quindi le due voci si
		// contendono lo slot e `ValidatePlan` lo dichiara `SlotOccupied` — misurato dal bot, non dedotto.
		Bot->PlannedDashAbility = INDEX_NONE;
		// ⚠️ **E lo slot REAZIONE, che era il sesto campo su sei a non ripartire da zero** (`#1403`,
		// [D-220]): fino a oggi dipendeva solo da `ClearReactionPlan()` nel Cleanup, che non gira sul
		// passaggio di `BeginPlay` ne' quando `PlanBotsForTest()` precede `LockInAndResolve()`. Da [D-220]
		// «nessuna reazione utilizzabile» e' un esito raggiungibile da due categorie indipendenti — kit e
		// loadout — quindi un indice stantio sopravviverebbe piu' spesso di prima. Si passa dalla porta di
		// [D-109]: azzera lo slot **e la sua condizione**, che sono una cosa sola.
		Bot->ClearReactionPlan();

		// Lo snapshot su cui QUESTO bot pianifica: quello della sua squadra, che porta le prenotazioni delle
		// compagne gia' passate di qui. Esistono tutti da prima del ciclo, quindi qui non si inserisce nulla
		// e il riferimento non puo' essere invalidato da una riallocazione.
		FRTHexSnapshot* TeamSnapshotPtr = PlanningSnapshots.Find(Bot->TeamId);
		if (!TeamSnapshotPtr)
		{
			continue; // non puo' accadere: la mappa e' costruita sugli stessi bot che questo ciclo visita
		}
		// UN SOLO nome, e non due: un alias `const` accanto a uno scrivibile dichiarerebbe un'immutabilita'
		// che non c'e' — la prenotazione a fine iterazione scrive proprio qui dentro.
		FRTHexSnapshot& Snapshot = *TeamSnapshotPtr;

		// Difesa: se ferito (sotto meta' HP) e ha un'abilita' che lo RIMETTE IN PIEDI, la usa e salta il turno.
		//
		// «Supporto» qui significa curare o schermare, non genericamente «agire su di se'»: il filtro era
		// `bSelfTarget` e basta, e finche' nessuna azione dichiarava quel flag la differenza non si vedeva.
		// Appena `Action.Guard` e `Action.Brace` l'hanno dichiarato — sono generiche, quindi le ha OGNI eroe —
		// un bot sotto meta' HP entrava qui ogni turno: Guard ha cooldown 0, quindi e' sempre pronta, e il
		// `continue` gli fa saltare l'attacco. Risultato: il bot ferito si mette in guardia per sempre e la
		// partita non finisce (`HexMatch.PlaysToCompletion`).
		//
		// Il ramo era scritto per `Guardian.Barrier`, che di cooldown ne aveva 3 e dava 40 di scudo. Chiedere
		// un effetto curativo lo riporta a quel significato senza dipendere dai cooldown, che sono
		// bilanciamento e cambiano.
		bool bUsedSupport = false;
		for (int32 A = 0; A < Bot->NumAbilities(); ++A)
		{
			const URTActionData* Ab = Bot->GetAbility(A);
			bool bRestores = false;
			if (Ab)
			{
				for (const FRTActionEffectSpec& Spec : Ab->Def.Effects)
				{
					if (Spec.Effect == ERTActionEffect::Heal || Spec.Effect == ERTActionEffect::Shield)
					{
						bRestores = true;
						break;
					}
				}
			}
			if (Ab && Ab->bSelfTarget && bRestores && Bot->CanUseAbility(A) && Bot->Health * 2 < Bot->MaxHealth)
			{
				Bot->PlannedAbilityIndex = A;
				bUsedSupport = true;
				break;
			}
		}
		// REAZIONE (`#601`): il bot arma la reazione che ha, se ne ha una pronta. Lo slot e' indipendente da
		// Movimento e Principale, quindi non compete con nient'altro e si dichiara PRIMA di ogni `continue`
		// del resto della pianificazione — altrimenti un bot che cura o che scatta uscirebbe dal ciclo senza
		// armarla.
		//
		// Nessuna euristica su QUANDO conviene: il trigger e' dichiarato dall'abilita' e valutato dal
		// resolver, e una reazione non armata non costa nulla a nessuno.
		//
		// Senza questa riga meta' delle unita' della v0.1 non reagirebbe mai, e il playtest misurerebbe un
		// gioco diverso da quello progettato: i sette moduli di CP 7.5 sarebbero verdi nei test e assenti in
		// partita.
		//
		// 🔴 **LA REGOLA: prima quella di KIT, il modulo di loadout come riserva** (`#1403`, [D-220]).
		//
		// ⚠️ **E' il comportamento che c'era gia': cambia che ora e' DICHIARATO.** Fino al 2026-08-27 questa
		// riga diceva *«sceglierne una fra due sarebbe una decisione di bot (E15) — con un solo slot reazione
		// nel kit di ogni eroe, oggi non si pone»*, e la premessa era falsa da [D-218]: Riktor porta
		// `Interposition` (kit) **e** `Reaction.Cleanse` (modulo). Il `break` sul primo trovato sceglieva per
		// **ordine di indice** — e dava il kit solo perche' `EquipLoadout` accoda. Stessa risposta, per
		// accidente invece che per regola.
		//
		// La preferenza ha una ragione, ed e' l'identita': la reazione di kit e' cio' che l'eroe **e'** — il
		// catalogo eroi la descrive cosi' — e il modulo e' cio' che la composizione gli **aggiunge**. Quando
		// il kit e' in ricarica il modulo copre, che e' il mestiere di una riserva.
		//
		// ⚠️ **Quale delle due convenga davvero e' una domanda aperta** (`BOT-REACT-1`), e vale per E15: i
		// moduli hanno `CooldownTurns = 0`, quindi un bot che preferisse il loadout non armerebbe **mai piu'**
		// la reazione d'eroe. Invertire questa riga non e' una pulizia — e' un cambio di gioco.
		int32 DalKit = INDEX_NONE;
		int32 DalLoadout = INDEX_NONE;
		for (int32 R = 0; R < Bot->NumAbilities(); ++R)
		{
			const URTActionData* Reaction = Bot->GetAbility(R);
			if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction || !Bot->CanUseAbility(R))
			{
				continue;
			}
			// L'origine si chiede al CATALOGO: `MakeEquipmentAction` scrive `Def.ActionId = EquipmentId`.
			// Dedurla dalla posizione — «i moduli stanno in fondo perche' `Add` accoda» — e' esattamente
			// l'accidente che questa riga smette di usare.
			int32& Candidato = IdEquipaggiamento.Contains(Reaction->Def.ActionId) ? DalLoadout : DalKit;
			if (Candidato == INDEX_NONE) { Candidato = R; } // a parita' di origine, il primo: deterministico
		}

		const int32 Armata = (DalKit != INDEX_NONE) ? DalKit : DalLoadout;
		if (const URTActionData* Reazione = Bot->GetAbility(Armata)) // `nullptr` per `INDEX_NONE`
		{
			Bot->PlannedReactionAbility = Armata;
			AddLogEvent(FString::Printf(TEXT("%s: arma %s (reazione)"),
				*Bot->GetName(), *Reazione->Def.ActionId.ToString()), FRTLogSubject::Unit(Bot));
		}

		if (bUsedSupport)
		{
			continue;
		}

		// Contesto di valutazione: i nemici che la SQUADRA DEL BOT conosce (celle, gittata effettiva,
		// HP+scudo) e pesi dal tuning.
		// L'ordine dei nemici viene da Units (ordine stabile dello snapshot): il punteggio non dipende
		// dall'ordine di enumerazione degli Actor.
		//
		// CP 13.5 — IL BOT PIANIFICA SULLA CONOSCENZA DELLA SUA SQUADRA (#160, RT-FEAT-BOT-FAIRNESS).
		// Fino a qui `Ctx.Enemies` conteneva *tutte* le unita' nemiche vive, senza filtro di percezione: il
		// bot vedeva ogni posizione avversaria mentre il giocatore no. Non era una svista nascosta — la spec
		// lo dichiarava (`docs/gameplay/spec-bot-hex.md` §6) — ma rendeva falsa la promessa che la Wiki fa al
		// giocatore, «il bot non vede piu' di te», e invalidava per costruzione ogni playtest contro di lui.
		//
		// La regola e' la STESSA del targeting umano (`ClassifyTarget`, piu' sotto in questo file): non un
		// secondo modello di conoscenza per il bot, che divergerebbe dal primo alla prima modifica.
		const FRTTeamKnowledge BotKnowledge = KnowledgeForTeam(Bot->TeamId);
		FRTHexBotContext Ctx;
		Ctx.Origin = Bot->Cell;
		// Da dove il bot guarda ORA: e' il punto di partenza della stima di come sara' orientato a fine turno
		// (CP 13.5). Chi resta fermo e non attacca conserva questo.
		Ctx.SelfFacing = Bot->Facing;
		// Il kiting lo DERIVA il bot dalla portata dell'attacco base: e' un comportamento dell'IA, non una
		// caratteristica dell'unita' (che quando la muove il giocatore non lo consulta mai).
		Ctx.KiteStandoff = URTHexBotLibrary::DeriveKiteStandoff(Bot->AttackRange);


		Ctx.WKill = WKill;
		Ctx.WDamage = WDamage;
		Ctx.WThreat = WThreat;
		Ctx.WKiteViolation = WKiteViolation;
		Ctx.WApproach = WApproach;
		Ctx.WElevation = WElevation;
		Ctx.WEngage = WEngage;
		Ctx.WEngageDecay = WEngageDecay;
		// La memoria per unita' del termine di ingaggio: quanti turni consecutivi questa unita' non
		// pianifica un attacco (#1300, D-185). Si aggiorna piu' sotto, a piano scelto.
		Ctx.IdleTurns = BotIdleTurns.FindRef(Bot->StableUnitId);

		TArray<int32> EnemyUnitIndex; // parallelo a Ctx.Enemies: indice dell'unita' in Units
		ARTUnit* Nearest = nullptr;
		// La cella da cui il kiter fugge, come la CONOSCE la squadra: su un contatto incerto e' il ricordo,
		// non la posizione vera. Senza questo campo la fuga userebbe `Nearest->Cell` — cioe' il bot
		// scapperebbe da dove il nemico e' davvero, che e' l'onniscienza rientrata dalla finestra.
		FRTCellId NearestKnownCell;
		int32 NearestDistance = MAX_int32;
		for (int32 j = 0; j < Units.Num(); ++j)
		{
			ARTUnit* Other = Units[j];
			if (Other->TeamId == Bot->TeamId)
			{
				// Alleati: servono a pesare il collaterale di un'area (#213). Il bot NON si conta fra loro
				// perche' `CollectHexAttacks` salta sempre l'attaccante.
				if (Other != Bot)
				{
					Ctx.Allies.Add(Other->Cell);
					Ctx.AllyHealth.Add(Other->Health + Other->Shield);
				}
				continue;
			}
			int32 EnemyReach = Other->AttackRange;
			for (int32 a = 0; a < Other->NumAbilities(); ++a)
			{
				const URTActionData* EAb = Other->GetAbility(a);
				// La minaccia e' cio' che il nemico puo' COLPIRE: una mobilita' rapida sposta, non fa danno a
				// distanza, e contarla gonfierebbe la portata percepita di ogni eroe che ne ha una.
				if (EAb && !URTCatalogLibrary::IsFastMovement(EAb->Def))
				{
					EnemyReach = FMath::Max(EnemyReach, EAb->RangeCells);
				}
			}
			// Cosa la squadra sa di questo nemico. `EnemyReach` NON passa di qui: gittate e forme sono
			// catalogo, cioe' dato pubblico — sapere che Phase ha portata 5 non e' sapere dov'e' Phase.
			FRTCellId KnownCell = Other->Cell;
			int32 KnownHealth = Other->Health + Other->Shield;
			switch (URTTeamKnowledgeLibrary::ClassifyTarget(BotKnowledge, Other->StableUnitId,
				Other->TeamId, Other->Cell))
			{
			case ERTTargetKnowledge::Allowed:
				break; // la squadra lo vede: cella e condizione attuali, come sempre

			case ERTTargetKnowledge::CellOnly:
			{
				// Contatto INCERTO: vale la cella dell'ULTIMO contatto, mai quella attuale — altrimenti il
				// ricordo inseguirebbe il bersaglio, che e' il modo silenzioso di continuare a vederlo.
				if (!URTTeamKnowledgeLibrary::LastKnownCell(BotKnowledge, Other->StableUnitId, KnownCell))
				{
					continue; // incerto senza ricordo: non e' ne' bersaglio ne' minaccia contabilizzabile
				}
				// ⚠️ **Gli HP correnti sarebbero la fuga esatta che il canary deve prendere**: direbbero al
				// bot che un'unita' che NON vede e' quasi morta, e lo manderebbe a finirla. Cio' che la
				// squadra conosce di un ricordo e' l'IDENTITA' (`StableUnitId` -> eroe -> catalogo), non la
				// condizione: si assume quindi integro, con un valore pubblico.
				//
				// ⚠️ Limite dichiarato: cosi' si PERDE anche informazione legittima — se la squadra l'ha
				// visto a 10 HP un turno fa, quel dato c'era. Il modello corretto e' un HP nel contatto, cioe'
				// un campo in `FRTLastKnownContact` e un incremento di `FRTTeamKnowledge::CurrentVersion`:
				// una decisione di formato, non un dettaglio di questo checkpoint. L'errore va nella
				// direzione sicura — il bot sottostima le occasioni, non ne inventa.
				KnownHealth = Other->MaxHealth;
				break;
			}

			default:
				continue; // ignoto alla squadra: per il bot quella cella e' vuota
			}

			Ctx.Enemies.Add(KnownCell);
			Ctx.EnemyRanges.Add(EnemyReach);
			Ctx.EnemyHealth.Add(KnownHealth);
			// CP 13.5 — l'ORIENTAMENTO del nemico, che decide se la sua copertura vale (ADR-0005 §4a).
			//
			// Si prende quello corrente e non si filtra, ed e' corretto: il facing e' cio' che la mesh mostra,
			// quindi il giocatore umano lo legge allo stesso modo. A restare privato e' l'INTENTO di rotazione
			// (`Facing.IntentIsTeamFiltered`), che qui non passa.
			//
			// ⚠️ Su un contatto `CellOnly` la cella e' quella del RICORDO ma il facing e' quello ATTUALE: e' una
			// piccola incoerenza voluta, perche' l'alternativa — ricordare anche l'orientamento — vorrebbe un
			// campo in `FRTLastKnownContact` e un incremento di `FRTTeamKnowledge::CurrentVersion`, cioe' una
			// decisione di formato. L'errore va nella direzione sicura: la copertura si calcola fra la cella
			// ricordata e la mia, e un facing piu' aggiornato del ricordo non rivela DOVE sia l'unita'.
			Ctx.EnemyFacings.Add(Other->Facing);
			EnemyUnitIndex.Add(j);

			// La distanza si misura da cio' che si CONOSCE: su un contatto incerto e' la cella del ricordo.
			const int32 Distance = URTHexLibrary::HexDistance(Bot->Cell, KnownCell);
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Other;
				NearestKnownCell = KnownCell;
			}
		}

		// CP 13.5 — NESSUN CONTATTO: si cerca, non ci si ferma.
		//
		// Prima del filtro di percezione questo caso non esisteva: `Ctx.Enemies` conteneva sempre tutti i
		// nemici vivi, quindi c'era sempre qualcuno verso cui avvicinarsi. Con la conoscenza parziale una
		// squadra puo' non sapere dove sia nessuno — ed e' la condizione NORMALE del primo turno, perche' su
		// una mappa di raggio 5 gli schieramenti opposti distano piu' della vista di chiunque.
		//
		// ⚠️ **Senza questo ramo la partita non finisce.** Con `Ctx.Enemies` vuoto lo scoring perde i termini
		// di minaccia e di avvicinamento, ogni cella vale uguale, e il bot resta fermo per sempre: due squadre
		// cieche che si aspettano. Non e' «il bot perde il contatto e sbaglia» (che il DoD ammette): e' un bot
		// che smette di giocare, e l'ha misurato `HexMatch.PlaysToCompletion` diventando rosso.
		//
		// La condotta e' la piu' povera che ristabilisce il contatto: avvicinarsi al CENTRO della mappa, che
		// e' geometria pubblica — zero informazione nascosta. Non e' una ricerca intelligente e non pretende
		// di esserlo: i goal veri (`SecureObjective`, `GatherInformation`) sono E26, e questo ramo e' il posto
		// in cui atterreranno. Deterministica: distanza minima dal centro, poi `StableLess`.
		// **Livello 3 di #1287: la condizione si estende da «non so dove sia nessuno» a «non ho nessuno che
		// posso ingaggiare».**
		//
		// Il caso che mancava: contatto NOTO ma non raggiungibile in modo utile. Misurato sulla mappa
		// d'autore — le due squadre si fermano ai lati dell'ostacolo centrale, che blocca vista e passo, a due
		// e tre celle di distanza in linea d'aria. `Ctx.Enemies` non e' vuoto, quindi questo ramo non entrava;
		// e il punteggio, che misura la distanza in linea d'aria, diceva «sei vicino, resta». Dodici turni,
		// 42 voci di TurnLog su 48 con esito `Stayed`, zero `Combat`.
		bool bQualcunoDaIngaggiare = false;
		if (Ctx.Enemies.Num() > 0 && Snapshot.Map)
		{
			for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snapshot, BotIdx))
			{
				for (const FRTCellId& KnownEnemy : Ctx.Enemies)
				{
					if (URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, R.Cell, KnownEnemy))
					{
						bQualcunoDaIngaggiare = true;
						break;
					}
				}
				if (bQualcunoDaIngaggiare) { break; }
			}
		}

		if (Ctx.Enemies.Num() == 0 || !bQualcunoDaIngaggiare)
		{
			if (Snapshot.Map && Snapshot.Map->Cells.Num() > 0)
			{
				// **Il PUNTO DI OSSERVAZIONE (#1287)**, quando un contatto noto esiste ma non e' ingaggiabile: la
				// cella percorribile piu' vicina PER CAMMINO da cui quel contatto si vedrebbe.
				//
				// ⚠️ **Per cammino e non in linea d'aria**, ed e' la differenza fra funzionare e no: con un
				// ostacolo in mezzo la meta e' geometricamente vicina e topologicamente lontana, e minimizzare la
				// distanza in linea d'aria incastra il bot contro il muro — che e' il difetto originale, ripetuto
				// un livello piu' in la'.
				//
				// ⚠️ Usa la MEMORIA del contatto (`FRTLastKnownContact`, CP 13.4), non le posizioni vere: il bot
				// va dove ha visto qualcuno, non dove qualcuno e'.
				//
				// ⛔ Non e' un pattern di ricerca: niente memoria di dove ha gia' guardato, niente settori, niente
				// coordinamento. Quelli sono E26 (#326), e chiedono stato per unita' che il bot oggi non ha.
				FRTCellId SeekCell;
				bool bHaMeta = false;
				if (Ctx.Enemies.Num() > 0)
				{
					int32 MiglioreCosto = MAX_int32;
					for (const FRTHexCellData& C : Snapshot.Map->Cells)
					{
						if (C.bBlocksMovement) { continue; }
						bool bVede = false;
						for (const FRTCellId& KnownEnemy : Ctx.Enemies)
						{
							if (URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, C.Id, KnownEnemy)) { bVede = true; break; }
						}
						if (!bVede) { continue; }

						const FRTHexPathResult Verso = URTHexSimLibrary::FindPathForUnit(Snapshot, BotIdx, C.Id);
						if (Verso.Path.Num() == 0) { continue; } // irraggiungibile: non e' una meta
						if (Verso.TotalCost < MiglioreCosto
							|| (Verso.TotalCost == MiglioreCosto && URTHexLibrary::StableLess(C.Id, SeekCell)))
						{
							MiglioreCosto = Verso.TotalCost;
							SeekCell = C.Id;
							bHaMeta = true;
						}
					}
				}

				if (!bHaMeta)
				{
					// Nessun contatto noto, o nessuna cella lo vede: il CENTRO, la condotta di CP 13.5. Geometria
					// pubblica, zero informazione nascosta.
					int64 SumX = 0, SumY = 0;
					for (const FRTHexCellData& C : Snapshot.Map->Cells) { SumX += C.Id.X; SumY += C.Id.Y; }
					const int32 N = Snapshot.Map->Cells.Num();
					const FRTCellId Barycentre(static_cast<int32>(SumX / N), static_cast<int32>(SumY / N), 0);

					SeekCell = Snapshot.Map->Cells[0].Id;
					int32 BestToBary = MAX_int32;
					for (const FRTHexCellData& C : Snapshot.Map->Cells)
					{
						// ⚠️ **Percorribile**, e l'assenza di questo filtro ha fermato l'intera partita. Su
						// `L_HexArena` il baricentro e' `(0,0)`, che blocca il passo: la meta era una cella in cui
						// non si puo' entrare, quindi nessun cammino, quindi nessun passo. Il codice precedente si
						// AVVICINAVA alla meta e sopravviveva a una meta impenetrabile; seguire un cammino no.
						if (C.bBlocksMovement) { continue; }
						const int32 D = URTHexLibrary::HexDistance(C.Id, Barycentre);
						if (D < BestToBary || (D == BestToBary && URTHexLibrary::StableLess(C.Id, SeekCell)))
						{
							BestToBary = D;
							SeekCell = C.Id;
						}
					}
				}

				// **Si SEGUE il cammino**, non si minimizza una distanza: il prefisso percorribile entro il
				// budget. Restare vince a parita' (cammino vuoto = si e' gia' a destinazione).
				FRTCellId Best = Bot->Cell;
				const FRTHexPathResult Rotta = URTHexSimLibrary::FindPathForUnit(Snapshot, BotIdx, SeekCell);
				const TArray<FRTCellId> Passi = URTHexSimLibrary::TruncatePathToBudget(Snapshot, BotIdx, Rotta.Path);
				if (Passi.Num() > 1)
				{
					Best = Passi.Last();
				}
				else
				{
					// Nessun cammino: ci si AVVICINA, che e' la condotta di CP 13.5 e non richiede che la meta sia
					// raggiungibile. Restare vince a parita', quindi un bot gia' al punto migliore non oscilla.
					int32 BestDistance = URTHexLibrary::HexDistance(Bot->Cell, SeekCell);
					for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snapshot, BotIdx))
					{
						const int32 D = URTHexLibrary::HexDistance(R.Cell, SeekCell);
						if (D < BestDistance || (D == BestDistance && URTHexLibrary::StableLess(R.Cell, Best)))
						{
							BestDistance = D;
							Best = R.Cell;
						}
					}
				}
				Bot->PlannedCell = Best;
			}
			// #1088 — anche qui, ed e' il ramo che il difetto colpiva per primo: due compagne che cercano il
			// contatto puntano ENTRAMBE la cella piu' vicina al centro, che e' una sola.
			ReserveNormalMove(Snapshot, Bot, BotIdx);
			continue; // niente da bersagliare: nessun attacco, nessuno scatto verso un nemico che non si conosce
		}

		// Scatto disponibile per questo turno (serve sia alla fuga sia alle candidate di riposizionamento).
		const int32 DashIdx = Bot->FindDashAbilityIndex();
		const URTActionData* DashAb = Bot->GetAbility(DashIdx);
		const bool bDashReady = DashAb && URTCatalogLibrary::IsFastMovement(DashAb->Def) && Bot->CanUseAbility(DashIdx);

		// Portata dello scatto letta come la legge ResolveDash: dal CATALOGO se l'azione ne fa parte,
		// altrimenti dal campo legacy dell'asset. Se il bot leggesse un numero diverso da quello che il
		// resolver usera', proporrebbe scatti fuori portata (o si negherebbe quelli buoni).
		const int32 DashDeclaredRange = bDashReady
			? (DashAb->Def.ActionId.IsNone() ? DashAb->RangeCells : DashAb->Def.RangeCells)
			: 0;
		const int32 DashBudget = bDashReady ? Bot->GetEffectiveDashRange(DashDeclaredRange) : 0;
		const ERTMovementStyle DashStyle = bDashReady ? DashAb->Def.MovementStyle : ERTMovementStyle::None;

		// Nemici del bot, per UnitId dello snapshot: la carica li tratta come bersagli, gli altri stili come
		// ostacoli. Sono gli stessi indici che ResolveDash passa a ResolveLinearMove.
		TSet<int32> DashHostiles;
		for (int32 j = 0; j < Units.Num(); ++j)
		{
			if (Units[j] && Units[j]->IsAlive() && Units[j]->TeamId != Bot->TeamId) { DashHostiles.Add(j); }
		}

		// 🔴 **Dalla BASE, non dallo snapshot di squadra, e la differenza e' una fase.** Le prenotazioni
		// descrivono dove le compagne andranno nel MOVE; il Dash risolve PRIMA del Move, quando quelle celle
		// sono ancora vuote. Copiandole qui, `ResolveLinearMove` e `IsLinearReachable` — che trattano ogni
		// occupante non ostile come un corpo solido — scarterebbero cariche e scatti perfettamente legali,
		// in silenzio. La prenotazione e' del Move: che sia CONSUMATA solo dal Move.
		FRTHexSnapshot DashSnapshot = BaseSnapshot;
		if (bDashReady)
		{
			// Le candidate nascono da `ReachableCells`, che spende PUNTI MOVIMENTO (Dijkstra sui costi). Ma la
			// portata di una mobilita' LINEARE si misura in CELLE — il catalogo dice che il terreno non la
			// riduce. Passare la portata direttamente come budget tronca le candidate sul terreno caro: su
			// acqua (costo 2) uno scatto da 5 celle ne vedrebbe 2, e le celle 3-5 non verrebbero mai
			// proposte benche' il resolver le raggiunga. E' la stessa divergenza celle-vs-MP di #140, un
			// gradino piu' a monte: il filtro puo' solo SCARTARE candidate, non farle nascere.
			//
			// Si allarga quindi il budget al caso peggiore (portata x costo della cella piu' cara della
			// mappa) e si lascia che `IsDashReachable` poti cio' che non e' in linea.
			int32 CandidateBudget = DashBudget;
			if (URTMovementActionLibrary::IsLinear(DashStyle) && Snapshot.Map)
			{
				int32 MaxCellCost = 1;
				for (const FRTHexCellData& Cell : Snapshot.Map->Cells)
				{
					MaxCellCost = FMath::Max(MaxCellCost, Cell.TotalMoveCost());
				}
				CandidateBudget = DashBudget * MaxCellCost;
			}
			DashSnapshot.Units[BotIdx].MoveBudget = CandidateBudget;
		}

		// Il bot valuta la raggiungibilita' con lo STESSO codice che la fase Dash usa per eseguirla
		// (issue #140): il grafo genera le candidate, ma e' la linearita' a dire quali sopravvivono.
		//
		// L'instradamento per STILE e' quello di ResolveDash: solo le mobilita' LINEARI passano da
		// `ResolveLinearMove`. Una mobilita' a budget (`Action.Sprint`) risolve col pathfinding, lo stesso
		// grafo da cui le candidate sono nate — quindi li' non c'e' nulla da filtrare, e applicare il filtro
		// lineare scarterebbe mosse perfettamente legali.
		//
		// Anche il GATE "questa e' un'azione di scatto" e' lo stesso (#142): `URTCatalogLibrary::IsFastMovement`
		// legge la fase del catalogo, qui come in ResolveDash. Prima le due risposte divergevano e le azioni
		// degli eroi — che dichiarano la fase e nient'altro — non venivano mai pianificate come scatto.
		auto IsDashReachable = [&](const FRTHexSnapshot& Snap, const FRTCellId& Goal) -> bool
		{
			if (!URTMovementActionLibrary::IsLinear(DashStyle))
			{
				return true;
			}
			return URTMovementActionLibrary::IsLinearReachable(
				Snap.Map, Bot->Cell, Goal, DashBudget, DashStyle, Snap.Occupancy, DashHostiles);
		};

		// Priorita' ritirata: se un nemico e' molto vicino (meta' dello standoff), il kiter fugge SUBITO,
		// rinunciando al tiro. Guardia del bot quadrato, conservata: non passa dalla utility.
		const int32 Standoff = URTHexBotLibrary::DeriveKiteStandoff(Bot->AttackRange);
		const bool bKiter = Standoff > 0;
		if (bKiter && Nearest && NearestDistance <= Standoff / 2)
		{
			if (bDashReady)
			{
				// Anche la fuga del kiter passa da ReachableCells (grafo): se la cella scelta non e'
				// raggiungibile in LINEA, lo scatto verrebbe rifiutato e il panico si tradurrebbe in un turno
				// perso. Meglio non scattare e lasciare decidere al movimento normale.
				const FRTCellId Dest = URTHexBotLibrary::BestKiteCell(DashSnapshot, BotIdx, NearestKnownCell);
				if (Dest != Bot->Cell && IsDashReachable(DashSnapshot, Dest))
				{
					Bot->PlannedDashAbility = DashIdx;
					Bot->PlannedDashCell = Dest;
					AddLogEvent(FString::Printf(TEXT("%s: scatto difensivo (schiva) -> (q=%d,r=%d,L%d)"),
						*Bot->GetName(), Dest.X, Dest.Y, Dest.Layer), FRTLogSubject::Unit(Bot));
					continue;
				}
			}
			Bot->PlannedCell = URTHexBotLibrary::BestKiteCell(Snapshot, BotIdx, NearestKnownCell);
			AddLogEvent(FString::Printf(TEXT("%s: arretra -> (q=%d,r=%d,L%d)"),
				*Bot->GetName(), Bot->PlannedCell.X, Bot->PlannedCell.Y, Bot->PlannedCell.Layer), FRTLogSubject::Unit(Bot));
			ReserveNormalMove(Snapshot, Bot, BotIdx);
			continue;
		}

		// --- Pool di candidate ---------------------------------------------------------------------
		// Un'unica utility sceglie fra: restare e sparare, riposizionarsi, scattare e sparare, scattare
		// per riposizionarsi. L'ATTACCO vale solo dalla cella in cui il bot si trovera' nel Blast: quella
		// attuale (il Move viene DOPO il Blast) o quella post-scatto (il Dash viene PRIMA).
		TArray<FRTHexBotPlan> Plans;
		TArray<int32> PlanAbility;  // abilita' d'attacco della candidata (INDEX_NONE = solo movimento)
		TArray<bool> PlanViaDash;   // la candidata si raggiunge con lo scatto
		// Vero se la candidata e' una CARICA: allora si punta la cella del NEMICO (`Ctx.Enemies[TargetIndex]`),
		// non `DestCell` — che per una carica e' dove ci si ferma, cioe' davanti al bersaglio. Serve un flag e
		// non una cella-sentinella: `FRTCellId()` vale (0,0,0), che e' una cella vera della mappa.
		TArray<bool> PlanIsCharge;

		auto AddCandidates = [&](const FRTHexSnapshot& Snap, int32 AbilityIndex, int32 Range, int32 Damage,
			bool bViaDash, bool bAttacksOnly)
		{
			FRTHexBotContext LocalCtx = Ctx;
			LocalCtx.AttackRange = Range;
			LocalCtx.AttackDamage = Damage;

			// Forma dell'azione valutata: senza, ogni attacco verrebbe pesato come un colpo singolo e un'area
			// non mostrerebbe ne' i nemici presi in piu' ne' il compagno investito (#213).
			if (const URTActionData* ShapedAbility = (AbilityIndex != INDEX_NONE) ? Bot->GetAbility(AbilityIndex) : nullptr)
			{
				LocalCtx.AttackShape = ShapedAbility->Shape;
				LocalCtx.AttackAreaRadius = ShapedAbility->AreaRadius;
				LocalCtx.bAttackFriendlyFire = ShapedAbility->Def.bFriendlyFire;
			}
			for (const FRTHexBotPlan& Candidate : URTHexBotLibrary::BuildCandidates(Snap, BotIdx, LocalCtx))
			{
				if (bAttacksOnly && !Candidate.bHasAttack) { continue; }
				// Le candidate nascono da ReachableCells, che segue il GRAFO. Lo scatto invece e' lineare
				// (CP 4.5): senza questo filtro il bot proporrebbe scatti che ResolveDash rifiuta, sprecando
				// l'abilita' in silenzio. L'invariante "il bot non propone mosse illegali" vale anche qui.
				if (bViaDash && !IsDashReachable(Snap, Candidate.DestCell))
				{
					continue;
				}
				Plans.Add(Candidate);
				PlanAbility.Add(Candidate.bHasAttack ? AbilityIndex : INDEX_NONE);
				PlanViaDash.Add(bViaDash);
				PlanIsCharge.Add(false);
			}
		};

		// 1) Riposizionamento col movimento normale (gittata 0 -> nessun attacco: nel Blast il bot e' ancora qui).
		AddCandidates(Snapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ false, /*bAttacksOnly*/ false);

		// 2) Attacco da FERMO, un'abilita' per volta: budget 0 -> l'unica cella candidata e' quella attuale.
		FRTHexSnapshot StaySnapshot = Snapshot;
		StaySnapshot.Units[BotIdx].MoveBudget = 0;
		for (int32 A = 0; A < Bot->NumAbilities(); ++A)
		{
			const URTActionData* Ability = Bot->GetAbility(A);
			if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
				|| !Bot->CanUseAbility(A)) { continue; }
			AddCandidates(StaySnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ false, /*bAttacksOnly*/ true);
		}

		// 3) CARICA: l'unico modo di scattare E colpire nello stesso turno, perche' il danno e' dell'azione di
		// movimento stessa e non di una seconda azione principale (#145). Le candidate non possono nascere da
		// `ReachableCells`: quella cerca celle LIBERE, mentre una carica punta la cella OCCUPATA dal nemico e
		// si ferma davanti. Si generano quindi dai bersagli, chiedendo al resolver se la traiettoria li
		// raggiunge — lo stesso codice che poi la eseguira'.
		if (bDashReady && DashStyle == ERTMovementStyle::LinearCharge)
		{
			const int32 ImpactDamage = URTCatalogLibrary::FirstDamage(DashAb->Def);
			for (int32 e = 0; e < Ctx.Enemies.Num(); ++e)
			{
				// Occupazione dalla BASE, come per `DashSnapshot`: la carica risolve nella fase Dash, e una
				// cella prenotata per il Move di una compagna li' e' ancora vuota. Con `Snapshot.Occupancy`
				// la traiettoria si fermerebbe su un corpo che non c'e' e la candidata sparirebbe in silenzio.
				const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
					BaseSnapshot.Map, Bot->Cell, Ctx.Enemies[e], DashBudget, DashStyle,
					BaseSnapshot.Occupancy, DashHostiles);

				// Vale solo se l'impatto colpisce PROPRIO quel nemico: una traiettoria che ne incontra un altro
				// prima e' una candidata diversa, e la genera il suo giro di ciclo.
				if (Linear.Stop != ERTLinearStop::Impact
					|| !EnemyUnitIndex.IsValidIndex(e) || Units[EnemyUnitIndex[e]] != Units[Linear.ImpactUnitId])
				{
					continue;
				}

				FRTHexBotPlan Charge;
				Charge.DestCell = Linear.Final;   // dove il bot si ferma: adiacente al bersaglio
				Charge.bHasAttack = true;
				Charge.TargetIndex = e;
				Charge.AttackDamage = ImpactDamage;
				Charge.TargetHealth = Ctx.EnemyHealth.IsValidIndex(e) ? Ctx.EnemyHealth[e] : 0;
				Plans.Add(Charge);
				PlanAbility.Add(INDEX_NONE);      // il colpo NON e' una seconda azione: e' l'impatto della carica
				PlanViaDash.Add(true);
				PlanIsCharge.Add(true);
			}
		}

		// 4) Scatto + attacco, e scatto per riposizionarsi.
		//
		// NOTA (#145, aggiornata da D-028): scatto e attacco sono ora slot DIVERSI — movimento e principale —
		// quindi pianificarli insieme e' legale, ed e' la scelta *schivo e sparo*. Il prezzo c'e' e non e' piu'
		// implicito: chi scatta non prosegue col Move (lo applica il resolver piu' sotto), chi carica si.
		//
		// Resta il problema di bilanciamento che la nota segnalava, e resta misurato sugli ARCHETIPI: per il
		// Guardian «scatto + Sweep» fa 30 danni e spinta 2 con cooldown 0, la Charge 20 e spinta 1 con
		// cooldown 3. Sul roster eroi i numeri sono altri. Il meccanismo qui sopra e' corretto; a renderlo
		// utile e' il bilanciamento — voce `BAL-1` del backlog, che parte da una misura e non da una correzione.
		if (bDashReady)
		{
			for (int32 A = 0; A < Bot->NumAbilities(); ++A)
			{
				const URTActionData* Ability = Bot->GetAbility(A);
				if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
					|| !Bot->CanUseAbility(A)) { continue; }
				AddCandidates(DashSnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ true, /*bAttacksOnly*/ true);
			}
			AddCandidates(DashSnapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ true, /*bAttacksOnly*/ false);
		}

		const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(Snapshot.Map, Plans, Ctx);

		// Da quale candidata viene il piano scelto (per sapere abilita' e se passa dallo scatto). Le candidate
		// di movimento normale sono in testa: a parita' di campi si preferisce NON consumare lo scatto.
		int32 BestIdx = INDEX_NONE;
		for (int32 p = 0; p < Plans.Num(); ++p)
		{
			if (Plans[p].DestCell == Best.DestCell && Plans[p].bHasAttack == Best.bHasAttack
				&& Plans[p].TargetIndex == Best.TargetIndex && Plans[p].AttackDamage == Best.AttackDamage)
			{
				BestIdx = p;
				break;
			}
		}

		const bool bViaDash = Plans.IsValidIndex(BestIdx) && PlanViaDash[BestIdx];
		const bool bIsCharge = Plans.IsValidIndex(BestIdx) && PlanIsCharge[BestIdx];
		const int32 BestAbility = Plans.IsValidIndex(BestIdx) ? PlanAbility[BestIdx] : INDEX_NONE;
		ARTUnit* Target = (Best.bHasAttack && EnemyUnitIndex.IsValidIndex(Best.TargetIndex))
			? Units[EnemyUnitIndex[Best.TargetIndex]] : nullptr;
		const int32 Score = URTHexBotLibrary::ScorePlan(Snapshot.Map, Best, Ctx);

		// La memoria si aggiorna UNA VOLTA per round: `PlanBotsForTest()` e `LockInAndResolve()`
		// pianificano entrambi lo stesso round, e senza guardia il decadimento andrebbe al doppio.
		{
			int32& UltimoRound = BotIdleRound.FindOrAdd(Bot->StableUnitId, -1);
			if (UltimoRound != TurnNumber)
			{
				UltimoRound = TurnNumber;
				int32& TurniInerti = BotIdleTurns.FindOrAdd(Bot->StableUnitId, 0);
				TurniInerti = Best.bHasAttack ? 0 : TurniInerti + 1;
			}
		}

		if (bIsCharge && Target && Ctx.Enemies.IsValidIndex(Best.TargetIndex))
		{
			// CARICA: si punta la cella del bersaglio e la fase Dash si ferma addosso a lui registrando
			// l'impatto. Nessun `PlannedAbilityIndex`: il colpo e' dell'azione di movimento, e pianificare
			// anche un'azione principale significherebbe spendere due volte lo stesso slot.
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Ctx.Enemies[Best.TargetIndex];
			// Il soggetto e' il BOT, non il bersaglio: e' la sua posizione e la sua intenzione che trapelano
			// qui. Il bersaglio e' gia' filtrato dalla riga che lo riguarda.
			AddLogEvent(FString::Printf(TEXT("%s: utility -> CARICA su %s (impatto da (q=%d,r=%d,L%d)) score=%d"),
				*Bot->GetName(), *Target->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score), FRTLogSubject::Unit(Bot));
		}
		else if (bViaDash && Target && BestAbility != INDEX_NONE)
		{
			// Scatta (fase Dash) e attacca dalla cella post-scatto: nel Blast, che segue il Dash, il bot e' li'.
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Best.DestCell;
			Bot->PlannedAbilityIndex = BestAbility;
			Bot->PlannedAttackTarget = Target;
			// Soggetto = il BOT (vedi nota sulla CARICA sopra).
			AddLogEvent(FString::Printf(TEXT("%s: utility -> scatto (q=%d,r=%d,L%d) + attacca %s score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->GetName(), Score), FRTLogSubject::Unit(Bot));
		}
		else if (Target && BestAbility != INDEX_NONE)
		{
			// Resta e attacca dalla cella attuale (Best.DestCell == cella d'origine).
			Bot->PlannedCell = Best.DestCell;
			Bot->PlannedAbilityIndex = BestAbility;
			Bot->PlannedAttackTarget = Target;
			// Soggetto = il BOT (vedi nota sulla CARICA sopra).
			AddLogEvent(FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) attacca %s score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->GetName(), Score), FRTLogSubject::Unit(Bot));
		}
		else if (bViaDash)
		{
			// Riposizionamento rapido con lo scatto (nessun tiro disponibile da nessuna cella).
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Best.DestCell;
			AddLogEvent(FString::Printf(TEXT("%s: scatto -> (q=%d,r=%d,L%d) score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score), FRTLogSubject::Unit(Bot));
		}
		else
		{
			// Posizionamento con il movimento normale (o "resta", se l'utility preferisce la cella attuale).
			Bot->PlannedCell = Best.DestCell;
			AddLogEvent(FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) score=%d%s"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score,
				Best.DestCell == Bot->Cell ? TEXT(" (resta)") : TEXT("")), FRTLogSubject::Unit(Bot));
		}

		// #1088 — l'ultima cosa che il bot fa: dichiarare alle compagne dove sta andando. Copre i quattro
		// rami qui sopra; i due `continue` piu' in alto prenotano per conto proprio, perche' escono prima.
		ReserveNormalMove(Snapshot, Bot, BotIdx);
	}
}

void ARTTurnManager::SetPlanningSeconds(float NewSeconds)
{
	PlanningSeconds = FMath::Max(0.f, NewSeconds);

	// Se la pianificazione e' GIA' in corso, il timer va rifatto: cambiare solo il campo lascerebbe scorrere
	// quello vecchio, e il valore nuovo varrebbe dal turno dopo — cioe' proprio nel turno in cui serviva non
	// farebbe niente.
	UWorld* World = GetWorld();
	if (World && Phase == ERTMatchPhase::Planning && !bIsResolving)
	{
		World->GetTimerManager().ClearTimer(PlanningTimerHandle);
		if (PlanningSeconds > 0.f)
		{
			World->GetTimerManager().SetTimer(PlanningTimerHandle, this,
				&ARTTurnManager::OnPlanningTimeout, PlanningSeconds, false);
		}
	}
}

void ARTTurnManager::StartPlanningTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	BeginPacingSample(); // apre il campione: il cronometro parte quando parte la pianificazione

	PlanBots(); // il bot pianifica a inizio turno

	World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	if (PlanningSeconds > 0.f)
	{
		World->GetTimerManager().SetTimer(PlanningTimerHandle, this, &ARTTurnManager::OnPlanningTimeout, PlanningSeconds, false);
	}
	AddLogEvent(FString::Printf(TEXT("Turno %d - pianificazione"), TurnNumber), FRTLogSubject::World());
}

void ARTTurnManager::OnPlanningTimeout()
{
	UE_LOG(LogRT, Log, TEXT("[RT] Timer scaduto -> lock-in automatico"));
	PacingCurrent.LockInSource = ERTLockInSource::Timeout; // non l'ha chiusa il giocatore
	LockInAndResolve();
}

float ARTTurnManager::GetPlanningTimeRemaining() const
{
	if (const UWorld* World = GetWorld())
	{
		const float Remaining = World->GetTimerManager().GetTimerRemaining(PlanningTimerHandle);
		return Remaining > 0.f ? Remaining : 0.f;
	}
	return 0.f;
}

void ARTTurnManager::LockInAndResolve()
{
	if (Phase != ERTMatchPhase::Planning || bIsResolving)
	{
		return; // gia' in risoluzione o non in pianificazione: ignora un secondo lock-in
	}

	// L'identita' di partita si fissa QUI, prima che il turno produca la sua prima voce di TurnLog (#405).
	// Questo e' il punto comune ai due percorsi: il gioco ci arriva da `StartPlanningTimer`, lo Scenario
	// Harness chiama `LockInAndResolve` direttamente senza passare dal timer.
	EnsureMatchRoster();

	// Sonda di pacing: chiude i tempi della pianificazione. Telemetria, nessun effetto sul turno.
	{
		// 🔴 Il campione puo' non essere mai stato APERTO: `BeginPacingSample()` la chiama solo
		// `StartPlanningTimer()`, cioe' `BeginPlay`, e qui ci arrivano anche i test headless e lo Scenario
		// Harness — il secondo dei due percorsi che il commento sopra `EnsureMatchRoster` gia' nomina. Anche
		// `SetPlanningSeconds()` arma il timer senza aprire nulla, quindi un `OnPlanningTimeout` VERO puo'
		// arrivare qui con il campione chiuso.
		//
		// Si apre adesso, come `EnsureMatchRoster` otto righe sopra e per la stessa ragione: il CONTESTO —
		// unita' vive, azioni disponibili, numero di turno — e' misurabile e va misurato. Buttare il turno
		// perderebbe un dato vero.
		//
		// ⚠️ Ma i TEMPI no: l'origine sarebbe «adesso», e `MsToLockIn` verrebbe zero. Zero e' un lock-in
		// istantaneo, cioe' un valore legittimo: sarebbe il dato plausibile e falso che `Unmeasured` esiste
		// per non produrre. I tre tempi dichiarano di non essere stati misurati (`#1421`).
		const bool bWasOpen = bPacingSampleOpen;
		if (!bWasOpen)
		{
			BeginPacingSample();
		}

		// #971 — la SECONDA causa, e arriva allo stesso esito per la stessa ragione. In una sessione non
		// presidiata il campione viene aperto regolarmente da `StartPlanningTimer`, quindi i tempi
		// sarebbero tutti misurabili e tutti veri di un cronometro che nessuno guardava: `MsToLockIn` e'
		// la durata del Planning, e il ramo `else` qui sotto scriverebbe `MsSinceLastInput = MsToLockIn`
		// classificando il turno fra le **attese a vuoto**. E' la classificazione giusta per un umano che
		// non ha toccato niente, e falsa per una partita in cui non c'era nessun umano: `SummarizeSamples`
		// li sommerebbe agli stessi contatori. Terzo esito della domanda che #971 poneva come binaria
		// (*«registra o tace?»*) — si registra il CONTESTO e si dichiara che i tempi non sono misurati,
		// che e' cio' che `Unmeasured` esiste per fare da #1421.
		//
		// ⚠️ I CONTEGGI restano: `SelectionCount`/`OrderCount`/`UndoCount` valgono zero, ed e' un fatto
		// vero — nessun input e' stato accettato, perche' `ARTPlayerController::IsPlanningInputInert()` li
		// ha resi inerti a monte. Solo i tre TEMPI sarebbero plausibili e falsi.
		if (!bWasOpen || bUnattendedSession)
		{
			PacingCurrent.MsToLockIn = FRTPacingSample::Unmeasured;
			PacingCurrent.MsSinceLastInput = FRTPacingSample::Unmeasured;
			PacingCurrent.MsToFirstInput = FRTPacingSample::Unmeasured;
		}
		else
		{
			// ⚠️ Un clamp al posto di tutto questo toglierebbe il comportamento non definito e lascerebbe il
			// dato falso: su Windows `FPlatformTime::Seconds()` non e' un tempo dall'avvio del processo
			// (porta dentro `16777216.0`), quindi `(Now - 0.0) * 1000.0` vale circa `1.7e10` e
			// `FMath::RoundToInt` lo tronca in un `int32` che arriva a `2.1e9`. Clampato sarebbe
			// `INT32_MAX`: un numero, e comunque non un tempo di pianificazione.
			const double Now = FPlatformTime::Seconds();
			PacingCurrent.MsToLockIn = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
			// Senza nessun input, "tempo dall'ultimo input" e' l'intera pianificazione: cosi' un turno passato
			// inerte finisce fra le attese a vuoto e non fra i tagli, che e' la classificazione corretta.
			PacingCurrent.MsSinceLastInput = bPacingHadInput
				? FMath::RoundToInt((Now - PacingLastInput) * 1000.0)
				: PacingCurrent.MsToLockIn;
			if (!bPacingHadInput)
			{
				PacingCurrent.MsToFirstInput = PacingCurrent.MsToLockIn;
			}
		}
	}

	// Chiude la pianificazione: ferma il timer (utile anche per il lock-in manuale).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	}

	// Nuova risoluzione: azzera la timeline del turno (verra' popolata dalle fasi).
	ResolvedTimeline.Reset();
	TurnLog.Reset();
	bPrepActiveThisTurn = false;

	// DOPO l'azzeramento e PRIMA delle fasi: il verdetto sul piano appartiene al turno che sta per
	// risolversi, non a quello appena chiuso. Invertire l'ordine lo cancellerebbe appena scritto.
	ValidatePlansAtLockIn();

	// Avanza le fasi fino a tornare a Planning; il movimento si applica nella fase Move.
	do
	{
		Phase = URTTurnRules::NextPhase(Phase);
		if (Phase == ERTMatchPhase::Prep)
		{
			ResolvePrep(); // abilita' di supporto (buff su se stessi)
		}
		else if (Phase == ERTMatchPhase::Dash)
		{
			ResolveDash(); // scatti: riposizionamento rapido PRIMA del Blast
		}
		else if (Phase == ERTMatchPhase::Blast)
		{
			ResolveCombat(); // gli attacchi usano la posizione PRIMA del movimento
		}
		else if (Phase == ERTMatchPhase::Move)
		{
			ResolveMovement();
		}
	} while (Phase != ERTMatchPhase::Planning);

	// TurnLog: ordinamento deterministico (fase -> categoria -> cella di partenza); GetAllActorsOfClass
	// non e' ordinato, quindi l'ordine di inserimento non e' affidabile (enum: confronto per valore intero).
	URTTurnLogLibrary::SortTurnLog(TurnLog); // ordine totale deterministico (libreria pura testabile)

	// Fase Cleanup, nell'ordine fissato da `spec-stati-temporanei-cp82.md` §4: revoca degli stati legati alla
	// cella -> scadenza delle durate -> energia/scudo/cooldown -> conteggio delle unita' vive.
	// La revoca precede il tick perche' le due nature non si sovrappongono: chi ha lasciato l'acqua si asciuga
	// in QUESTO Cleanup, senza aspettare un turno.
	ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* CleanupMap = MapActor ? MapActor->MapAsset : nullptr;

	// 0. Scadenza delle modifiche ambientali dei turni PRECEDENTI (CP 8.4). Precede le azioni di questo turno
	// per una ragione di durata: una cella incendiata adesso deve bruciare per i suoi due turni pieni: se il
	// tick venisse dopo, le mangerebbe subito uno.
	TickDynamicSurfaces(CleanupMap);

	// Stessa ragione di ordine delle superfici: un ponte creato ADESSO deve durare i suoi turni pieni, quindi
	// il tick precede le azioni di questo turno invece di mangiarne subito uno.
	TickDynamicArcs(CleanupMap);

	// Le coperture temporanee (CP 9.5) scalano invece GIA' in questo Cleanup, e non e' un'incoerenza con le due
	// righe qui sopra: un pannello nasce in **Prep**, cioe' prima del Blast che lo usa, quindi il turno in cui
	// e' stato eretto e' un turno in cui ha gia' riparato qualcuno. Saltarlo gliene regalerebbe uno.
	TickDynamicCovers(CleanupMap);

	// 1. Azioni ambientali (CP 8.3/8.4): la scarica elettrica e le modifiche del terreno precedono il danno di
	// `Burning`. Chi cade qui e' morto in QUESTO turno, come chi cade bruciato: il conteggio dei vivi arriva
	// dopo entrambi.
	ResolveEnvironment(CleanupMap);

	int32 Team0Alive = 0, Team1Alive = 0;
	// Presenze sull'OBIETTIVO contendibile (CP 10.2, #75): si contano insieme ai vivi e si valutano dopo il
	// loop, perche' la domanda e' la stessa — chi e' ancora in piedi, e su quale cella.
	int32 Team0OnObjective = 0, Team1OnObjective = 0;
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

		// 🔴 **Ordine STABILE, e da `#625` non e' piu' facoltativo.** `SortTurnLog` gira **prima** di questo
		// blocco (una volta sola, poco sopra): tutto cio' che viene appeso al TurnLog da qui in giu' resta
		// nell'ordine in cui e' stato inserito, e `GetAllActorsOfClass` non ne ha uno — lo dice il commento
		// di quel sort. Finche' questo loop non scriveva voci canoniche la cosa era invisibile; da quando
		// scrive quella del danno da `Burning`, **due unita' che bruciano nello stesso Cleanup possono
		// uscire in ordine diverso fra due esecuzioni della stessa partita**.
		// Non e' un dettaglio di stile: `RecordTurn` scrive `HashTurnLogOrdered` nel manifest — un hash che
		// per costruzione **non** e' invariante per permutazione — e `RTShowcaseScenarioTests` confronta due
		// run riga per riga. Uno scambio farebbe divergere l'archivio da se' stesso.
		// E' la stessa disciplina che `ResolveEnvironment` e `TickDynamicCovers` applicano gia', con lo
		// stesso comparatore. Trovato in code review.
		Actors.Sort([](const AActor& A, const AActor& B)
		{
			const ARTUnit* UA = Cast<ARTUnit>(&A);
			const ARTUnit* UB = Cast<ARTUnit>(&B);
			if (!UA || !UB) { return UA != nullptr; } // i non-unita' in coda, deterministicamente
			return URTHexLibrary::StableLess(UA->Cell, UB->Cell);
		});

		for (AActor* Actor : Actors)
		{
			ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive())
			{
				continue;
			}

			// 1. Effetti ambientali PRIMA dei KO (ADR-0003 §3): chi muore bruciato muore in questo turno.
			// Passa dalla contabilita' del danno di gioco (ApplyDamage + ApplyCombatState), quindi erode
			// prima lo scudo TEMPORANEO — che infatti scade solo piu' sotto, non prima.
			if (Unit->HasStatus(TAG_Status_Burning))
			{
				const int32 HpPrima = Unit->Health; // serve DOPO, per classificare l'esito
				const FRTDamageResult Burn = URTCombatLibrary::ApplyDamage(
					URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
					Unit->Shield, Unit->GetTemporaryShield(), Unit->Health);
				Unit->ApplyCombatState(Burn.Health, Burn.Shield);

				// ➕ **La voce canonica del danno da hazard** (`#625`). Fino al 2026-08-16 questo danno
				// esisteva **solo** in `AddLogEvent`, cioe' in un `UE_LOG` piu' un buffer circolare troncato:
				// chi riproduceva la partita vedeva gli HP scendere — o un'unita' sparire — senza un evento
				// che lo spiegasse, e `DescribeFirstDivergence` non poteva nominare quel punto. E' il difetto
				// che il gate `replay_representable` ha trovato, e che il commento di `ERTReactionDecision`
				// cita per nome in `RTTurnLog.h`.
				//
				// ⚠️ **Categoria `Combat` e non `Environment`, ed e' una scelta di modello.** La cella agisce,
				// ma la domanda a cui questa voce risponde e' *«quanti punti vita ha cambiato, e a chi»* — la
				// stessa per cui `Healed` sta fra gli esiti di combattimento invece di avere una categoria
				// propria. `ERTEnvironmentOutcome` parla di **superfici e coperture**: aggiungergli valori sul
				// danno alle unita' creerebbe due enum che rispondono alla stessa domanda sotto due categorie,
				// che e' precisamente il difetto argomentato in `RTTurnLog.h` §`ERTReactionDecision`.
				// La **causa** la porta `ActionId`, dove un danno da attacco porta l'identita' dell'azione: e'
				// li' che si distingue un colpo dalle fiamme, non nella categoria.
				//
				// ⚠️ `AppendLogEntry(Entry, Unit)` — l'unita' che **subisce**, non chi colpisce, ed e'
				// l'opposto della voce dell'attacco due funzioni piu' sotto. Non e' un'incoerenza: `UnitId` e'
				// «chi ha agito», e in un danno da hazard **non c'e' un attaccante**. Lasciarlo a `0` direbbe
				// «nessuna unita' dichiarata» su un evento che ha un soggetto solo e ovvio.
				FRTTurnLogEntry Burning;
				Burning.Phase = ERTMatchPhase::Cleanup;
				Burning.Category = ERTLogCategory::Combat;
				Burning.ActionId = TAG_Status_Burning.GetTag().GetTagName();
				// ⚠️ **Le due celle sono DOVE SI TROVA chi brucia, non la causa del danno**, e la prima
				// stesura di questa riga diceva il contrario («la cella agisce su chi ci sta sopra»).
				// E' falso per la maggioranza dei tick: `Fire` concede `Burning` con durata **2**, non con
				// il sentinella «finche' sulla cella», quindi un'unita' che esce dal fuoco continua a
				// bruciare — e prende gli 8 danni stando su un pavimento qualunque. La causa la porta
				// `ActionId`; queste due dicono solo dov'era. Trovato in code review.
				Burning.SrcCell = Unit->Cell;
				Burning.TgtCell = Unit->Cell;
				// ⚠️ `Amount` e' il danno NOMINALE del catalogo, non gli HP effettivamente persi: e' la
				// convenzione delle altre voci di danno (`Entry.Amount = Hit.Damage`), e cambiarla qui sola
				// renderebbe due voci di `Combat` non confrontabili. Quanto sia arrivato agli HP lo dice
				// `Outcome`: `ShieldAbsorbed` = zero.
				Burning.Amount = URTCombatLibrary::BurningCleanupDamage;
				// 🔴 **La libreria, non un ternario scritto a mano.** La prima stesura copiava quello della
				// voce d'attacco — che a sua volta scavalca la libreria — e nel copiarlo ha ereditato il suo
				// difetto: confrontava con `MaxHealth` invece che con la salute PRIMA del colpo, quindi
				// un'unita' gia' ferita il cui scudo assorbiva tutto veniva scritta come `Hit` per 8 danni
				// che non aveva preso. `ClassifyCombatOutcome` e' il posto dichiarato di quella priorita',
				// ed e' pinnata da `RTCombatLibraryTests`. Trovato in code review.
				Burning.Outcome = static_cast<uint8>(
					URTCombatLibrary::ClassifyCombatOutcome(HpPrima, Burn.Health, /*AttackerDmgBonus*/ 0));
				// ⚠️ Il soggetto e' chi SUBISCE, ed e' prescritto dal DoD di `#625` — «l'unita' che la subisce
				// in `UnitId` (non `0`: c'e' un soggetto)». **Inverte** la convenzione di `AppendLogEntry`
				// («chi ha AGITO»), che la voce d'attacco rispetta passando l'attaccante.
				//
				// ✅ **Chiusa da `#1150`, e le due righe che c'erano qui sono scadute.** Dicevano «oggi
				// nessuno lo fa» — falso: `URTTurnLogLibrary::IsDamageInflictedByActor` e' il consumatore, e
				// questa causa e' una di quelle che `IsEnvironmentalDamage` riconosce. E indicavano come
				// rimedio «un esito dedicato»: misurato, non c'e' posto — `Outcome` e' un solo `uint8` e
				// porta gia' la GRAVITA', quindi un valore «ambientale» toglierebbe `Lethal` a chi muore
				// bruciato. La convenzione e' dichiarata su `FRTTurnLogEntry::UnitId`, dove chi legge il
				// campo la trova.
				AppendLogEntry(Burning, Unit);

				// ⚠️ `AddLogEvent` **resta**, e non e' ridondanza: e' la vista leggibile a schermo, il TurnLog
				// e' la traccia. Il DoD di `#625` lo chiede esplicitamente — «non si sostituisce, si affianca».
				AddLogEvent(FString::Printf(TEXT("%s: %d danni da Status.Burning (q=%d,r=%d,L%d)"),
					*Unit->GetName(), URTCombatLibrary::BurningCleanupDamage,
					Unit->Cell.X, Unit->Cell.Y, Unit->Cell.Layer), FRTLogSubject::Unit(Unit));

				if (!Unit->IsAlive())
				{
					// L'eliminazione da hazard non ha un beat di playback (la timeline e' gia' chiusa):
					// la nasconde il catch-all di ConcludeTurn, che esiste proprio per questo caso.
					//
					// ⚠️ **La morte la porta l'`Outcome` della voce sopra, non una seconda voce.** Il DoD
					// chiede che l'eliminazione sia «distinta dal danno che non uccide», e `Lethal` la
					// distingue gia': una voce in piu' direbbe due volte lo stesso fatto, e il replay dovrebbe
					// decidere quale delle due e' il colpo — che e' lo stesso motivo per cui l'attacco letale,
					// due funzioni piu' sotto, non ne scrive una seconda.
					AddLogEvent(FString::Printf(TEXT("%s eliminato dalle fiamme"), *Unit->GetName()), FRTLogSubject::World());
					continue; // morto adesso: non guadagna energia, non conta fra i vivi
				}
			}

			// 2. Senza mappa autorevole non si revoca nulla: cancellare gli stati sarebbe inventare che la
			// cella non li sostiene, quando in realta' non la si e' potuta leggere.
			if (CleanupMap)
			{
				const FRTHexCellData* CellData = CleanupMap->FindCell(Unit->Cell);
				// #1077: la REVOCA e' una morte con una causa — l'unita' ha lasciato la cella che lo
				// sosteneva — ed e' diversa dalla scadenza qui sotto. Un replay che le confondesse non
				// saprebbe dire se il giocatore ha fatto qualcosa o se e' solo passato il tempo.
				// ⚠️ I tag arrivano gia' ordinati per nome: l'ordine di `CellBoundStatuses` e' quello di una
				// `TSet`, e farci dipendere l'ordine delle voci renderebbe l'hash del turno instabile.
				for (const FGameplayTag& Revocato : Unit->RevokeCellBoundStatusesNotIn(CellData
					? URTTerrainLibrary::CellBoundStatusesFor(CellData->Surface)
					: TSet<FGameplayTag>()))
				{
					// ⚠️ **Solo se lo stato e' finito DAVVERO.** `HasStatus` e' vero se il tag sta nei turni a
					// termine **oppure** fra quelli legati alla cella: un'unita' bagnata dall'acqua bassa e
					// anche da `PressureJet` conserva il `Wet` a termine quando lascia la pozza, e annunciarne
					// la revoca sarebbe una morte mai avvenuta — la stessa specie di bugia che #1077 esiste
					// per togliere. Trovato in code review.
					if (!Unit->HasStatus(Revocato))
					{
						FRTTurnLogEntry Morto = MakeStatusDeathEntry(Revocato, Unit->Cell, ERTStatusOutcome::Revoked);
						AppendLogEntry(Morto, Unit);
					}
				}
			}

			Unit->Energy = URTCombatLibrary::GainEnergy(Unit->Energy, Unit->EnergyPerTurn, Unit->MaxEnergy);
			Unit->ExpireTemporaryShield(); // la protezione delle abilita' di supporto vale un turno solo
			// [D-224]: subito DOPO la scadenza, cosi' l'invariante «a fine turno ogni unita' viva ha
			// esattamente lo scudo base e zero temporaneo» si verifica in un punto solo.
			Unit->RechargeBaseShield();
			// #1077: la SCADENZA. Nessuno ha fatto niente, e' finito il conteggio — e anche qui i tag
			// arrivano ordinati, perche' vengono da una `TMap`.
			for (const FGameplayTag& Scaduto : Unit->TickStatuses())
			{
				// Simmetrico della revoca: il conteggio e' finito, ma se la cella lo sostiene ancora lo stato
				// c'e' e non e' scaduto niente di osservabile.
				if (!Unit->HasStatus(Scaduto))
				{
					FRTTurnLogEntry Morto = MakeStatusDeathEntry(Scaduto, Unit->Cell, ERTStatusOutcome::Expired);
					AppendLogEntry(Morto, Unit);
				}
			}
			Unit->TickCooldowns();

			// Il piano di REAZIONE si azzera QUI, e non piu' nel pass che lo legge (`#505`). Con D-092 i punti
			// di valutazione sono piu' d'uno e distribuiti su fasi diverse: se il primo che passa consumasse il
			// piano, il secondo non troverebbe niente da valutare — e i trigger che nascono dopo il Blast non
			// scatterebbero mai, restando dati senza consumatore. Questa e' la fine del turno dell'unita', ed e'
			// dove un piano smette di valere. Pinnato da `Turn.PlansDoNotSurviveTheTurn`, che cade se questa
			// riga sparisce (verifica di mutazione, 2026-08-12).
			//
			// Azzera lo slot **e la sua condizione** ([D-109]): sono una cosa sola, e separarle rimetterebbe in
			// gioco una condizione orfana che il prossimo armamento erediterebbe.
			Unit->ClearReactionPlan();
			// E con lui il contatore delle attivazioni ([D-092]): «una per TURNO» ha bisogno di sapere quando
			// il turno finisce, ed e' qui — lo stesso punto in cui il piano smette di valere.
			Unit->ReactionActivationsThisTurn = 0;
			// La presenza sull'obiettivo si legge QUI, nello stesso passaggio: un secondo giro sulle unita'
			// sarebbe un secondo momento, e fra i due qualcosa potrebbe muoversi senza che nessuno lo veda.
			if (CleanupMap)
			{
				const FRTHexCellData* StandingOn = CleanupMap->FindCell(Unit->Cell);
				if (StandingOn && StandingOn->bIsObjective)
				{
					(Unit->TeamId == 0 ? Team0OnObjective : Team1OnObjective)++;
				}
			}
			(Unit->TeamId == 0 ? Team0Alive : Team1Alive)++;
		}
	}

	// L'OBIETTIVO contendibile (CP 10.2, #75), valutato QUI e non altrove: la DoD chiede il controllo «nel
	// Cleanup, dopo gli effetti ambientali e i KO», e questo e' l'unico punto che li ha entrambi alle spalle.
	// Le presenze contate sopra sono di unita' ancora VIVE: chi e' stato eliminato sull'obiettivo — dalle
	// fiamme, dall'acqua o da un colpo — non lo tiene, ed e' precisamente cio' che «dopo i KO» significa.
	//
	// ⚠️ **Sta PRIMA di `EvaluateMatchEnd`, e l'ordine e' la regola**: un punto segnato in questo Cleanup deve
	// poter chiudere la partita nello stesso Cleanup. Scritto dopo, il verdetto leggerebbe il punteggio del
	// turno precedente e la vittoria per obiettivo arriverebbe sempre con un turno di ritardo.
	if (CleanupMap && CleanupMap->HasObjectiveCell())
	{
		const ERTObjectiveOutcome Control = URTTurnRules::ResolveObjectiveControl(Team0OnObjective, Team1OnObjective);

		// UN punto per Cleanup controllato. Non e' un numero di bilanciamento ma la GRANULARITA' della
		// misura — «un turno di controllo vale un progresso» — ed e' intero come la DoD chiede: un float
		// renderebbe il punteggio dipendente dall'ordine delle somme, che e' esattamente cio' che il
		// determinismo del TurnLog non ammette. Quanti punti servano per vincere e' un'altra domanda, e vive
		// in `FRTMatchRules::ScoreToWin` — oggi ZERO, cioe' via disattivata.
		const int32 Points = 1;
		if (Control == ERTObjectiveOutcome::Team0Scores) { AddTeamScore(0, Points); }
		else if (Control == ERTObjectiveOutcome::Team1Scores) { AddTeamScore(1, Points); }

		FRTTurnLogEntry Objective;
		Objective.Phase = ERTMatchPhase::Cleanup;
		Objective.Category = ERTLogCategory::Objective;
		Objective.Outcome = static_cast<uint8>(Control);
		Objective.ActionId = FName(TEXT("Objective.Control"));
		// La cella e' quella dell'obiettivo quando ce n'e' UNO solo, che e' il caso della v0.1; con piu'
		// obiettivi (CP 31.1, post-v0.1) questa voce diventera' una per obiettivo. Finche' ce n'e' uno, le
		// due celle sono la stessa: la voce non descrive uno spostamento.
		Objective.SrcCell = CleanupMap->FirstObjectiveCell();
		Objective.TgtCell = Objective.SrcCell;
		// `Amount` porta i punti EFFETTIVAMENTE assegnati: zero quando l'obiettivo e' conteso o di nessuno.
		// Un lettore che somma questa colonna ottiene il punteggio, senza dover reinterpretare l'esito.
		Objective.Amount = (Control == ERTObjectiveOutcome::Team0Scores || Control == ERTObjectiveOutcome::Team1Scores) ? Points : 0;
		// `nullptr`: il punto lo fa la SQUADRA, non un'unita' ([D-063] — `UnitId = 0` e' «nessuna unita'»).
		// Nominare chi ci stava sopra sarebbe inventare un soggetto che la regola non ha.
		AppendLogEntry(Objective, nullptr);
	}

	// Fine partita a tre vie (CP 10.3), valutata QUI: nel Cleanup, dopo gli effetti ambientali e i KO, e
	// fuori dai resolver puri — nessuno di loro consulta il formato, e passarglielo sarebbe un parametro che
	// nessuna funzione legge (spec §16.3).
	FRTMatchState MatchState;
	MatchState.Team0Alive = Team0Alive;
	MatchState.Team1Alive = Team1Alive;
	MatchState.Team0Score = Team0Score;
	MatchState.Team1Score = Team1Score;
	MatchState.RoundNumber = TurnNumber;
	PendingResult = URTTurnRules::EvaluateMatchEnd(MatchState, MatchRules);
	// Lo stato su cui il verdetto e' stato dato viaggia con lui: `OnMatchEnded` lo annuncia da
	// `ConcludeTurn`, e ricostruirlo la' sarebbe un secondo calcolo che puo' divergere da questo.
	PendingState = MatchState;

	// Il playback di QUESTO turno parte da zero anche se non verra' riprodotto: senza, il ramo senza
	// playback lascerebbe il valore del turno precedente e la misura leggerebbe una durata mai avvenuta.
	PlaybackElapsedTotal = 0.f;

	// Se c'e' qualcosa da mostrare (movimenti/attacchi) e il playback e' attivo, riproduci la risoluzione
	// nel tempo; altrimenti concludi subito il turno (comportamento istantaneo: es. headless/senza eventi).
	if (bEnablePlayback && ResolvedTimeline.Num() > 0)
	{
		BeginPlayback();
		return;
	}
	ConcludeTurn();
}

void ARTTurnManager::ApplyForcedDisplacement(ARTUnit* Unit, const FRTCellId& NewCell,
	const FRTCellId& FacingSource, const TMap<ARTUnit*, FRTDisplacementCause>& CauseByTarget,
	const TCHAR* LogVerb, const URTHexMapAsset* Map, ERTMatchPhase InPhase)
{
	if (!IsValid(Unit))
	{
		return;
	}
	const FRTCellId OldCell = Unit->Cell;

	// 1. Riga di combat log: e' per l'HUD e NON finisce nel file — la traccia e' la voce di TurnLog al passo 3.
	AddLogEvent(FString::Printf(TEXT("%s: %s -> (q=%d,r=%d,L%d)"),
		LogVerb, *Unit->GetName(), NewCell.X, NewCell.Y, NewCell.Layer), FRTLogSubject::Unit(Unit));

	// 2. Celle attraversate: la linea esagonale fra le due, i cui passi sono adiacenti per costruzione. Serve
	// al playback E agli hazard — «lo spostamento forzato ignora il costo VOLONTARIO del terreno, non la
	// geometria e non gli hazard» (spec-tassonomia-movimento §3).
	const TArray<FRTCellId> Path = URTHexLibrary::HexLine(OldCell, NewCell);

	// 3. La voce di TurnLog CON LA CAUSA (#307). Prima lo spostamento esisteva solo come riga di combat log:
	// il replay registrava il danno e taceva il movimento, e chi rileggeva il file vedeva l'unita' altrove
	// senza nulla che lo spiegasse.
	AppendDisplacementEntry(Unit, OldCell, NewCell, Path.Num() - 1, CauseByTarget);

	// 4. Evento per il playback: lo spostamento scivola OldCell -> NewCell nella fase Blast.
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Move;
		Ev.SourceStableUnitId = Unit->StableUnitId;
		Ev.Path = Path;
		ResolvedTimeline.Add(Ev);
	}

	// 5. La cella nuova.
	Unit->Cell = NewCell;

	// 6. Il FACING verso la sorgente (CP 16.1, ADR-0005 §3): chi subisce uno spostamento si gira verso chi
	// l'ha causato — chi spinge, chi tira, o chi ha innescato la fuga ([D-104]). Si applica con la cella
	// GIA' aggiornata, cosi' la voce di log porta la posizione in cui l'unita' e' finita.
	//
	// Un Move volontario successivo VINCE: risolve dopo, nella sua fase, e riscrive. Non serve un flag che
	// ricordi «e' stata spostata» — e' l'ORDINE delle fasi a deciderlo, ed e' il motivo per cui
	// `URTFacingLibrary` non tiene stato.
	{
		FRTHexSimUnit Moved(0, NewCell, /*InMoveBudget=*/ 0);
		Moved.Facing = Unit->Facing;
		const ERTHexDirection Turned = URTFacingLibrary::FacingAfterDisplacement(
			NewCell, FacingSource, ERTDisplacementCause::Forced, Moved.Facing);
		RecordFacingChange(Moved, Turned,
			ERTFacingOutcome::TurnedToDisplacementSource, ERTMatchPhase::Blast, Unit);
		Unit->Facing = Moved.Facing;
	}

	// 7. La posizione visiva. Il contesto esagonale se lo chiede la primitiva invece di riceverlo: cosi' e'
	// autonoma, e chi la chiamera' da una fase dove quelle locali non sono in scope — il pass del Cleanup di
	// `SelfReposition`, D-092 — non deve procurarsele per poterla usare.
	{
		FVector VisualOrigin; float VisualSize; float VisualLayerH;
		GetHexContext(VisualOrigin, VisualSize, VisualLayerH);
		Unit->SetVisualLocation(Unit->WorldForCell(NewCell, VisualOrigin, VisualSize, VisualLayerH));
	}

	// 8. Gli hazard di OGNI cella attraversata, non solo di quella d'arrivo (#308). Chi viene spostato
	// attraverso `asciutto -> fuoco -> fuoco -> asciutto` ha attraversato quelle due celle di fuoco e ne
	// subisce le conseguenze, pur non avendo speso un solo punto movimento: il costo e' cio' che si paga per
	// SCEGLIERE di passare, la geometria e' cio' che c'e'.
	ApplyTerrainOnEnterEffects(Map, Unit, CellsEnteredAlong(Path), InPhase);

	// 9-10. Il piano segue l'unita' invece di riportarla indietro: la path composita dalla vecchia cella non
	// e' piu' valida, e se non c'era un Move pianificato la destinazione diventa quella nuova — altrimenti
	// l'unita' tornerebbe sui suoi passi nella fase Move, annullando lo spostamento.
	Unit->PlannedPath.Reset();
	Unit->PlannedWaypoints.Reset();
	if (Unit->PlannedCell == OldCell) { Unit->PlannedCell = NewCell; }
}

void ARTTurnManager::AppendDisplacementEntry(const ARTUnit* Target, const FRTCellId& From, const FRTCellId& To,
	int32 Steps, const TMap<ARTUnit*, FRTDisplacementCause>& CauseByTarget)
{
	FRTTurnLogEntry Entry;
	// Fase `Blast`: lo spostamento forzato avviene dove avviene il colpo che lo produce, non nella fase Move.
	// E' quello che rende leggibile «sono stato spostato PRIMA di potermi muovere».
	Entry.Phase = ERTMatchPhase::Blast;
	Entry.Category = ERTLogCategory::Move;
	Entry.Outcome = static_cast<uint8>(ERTMoveOutcome::Displaced);
	Entry.SrcCell = From;
	Entry.TgtCell = To;
	Entry.Amount = FMath::Max(0, Steps);

	// La chiave delle due mappe e' il puntatore non-const del bersaglio: qui si arriva con un const, e
	// `const_cast` sul solo scopo di CERCARE non muta nulla — la mappa e' const anch'essa.
	ARTUnit* Key = const_cast<ARTUnit*>(Target);
	if (const FRTDisplacementCause* Cause = CauseByTarget.Find(Key))
	{
		Entry.ActionId = Cause->ActionId;
		Entry.BaseActionId = Cause->BaseActionId;
		Entry.Priority = Cause->Priority;
	}

	// Voce di categoria Move: l'unita' e' quella che SI SPOSTA, come per ogni altra voce di movimento — le
	// celle della voce sono le sue. Chi ha spinto non e' ricostruibile qui: `FRTDisplacementCause` porta
	// l'azione, non l'attore.
	AppendLogEntry(Entry, Target);
}

void ARTTurnManager::AppendDisplacementResistedEntry(const ARTUnit* Target, ERTDisplacementBlockReason Reason,
	const TMap<ARTUnit*, FRTDisplacementCause>* CauseByTarget)
{
	if (!Target) { return; }

	FRTTurnLogEntry Entry;
	// Stessa fase e stessa categoria della voce positiva (#307): lo spostamento MANCATO appartiene al Blast
	// che l'avrebbe prodotto, non alla fase Move. Un lettore che filtra il Blast li trova insieme, ed e' la
	// condizione perche' «spinto e spostato» e «spinto e fermo» si confrontino sulla stessa riga di tempo.
	Entry.Phase = ERTMatchPhase::Blast;
	Entry.Category = ERTLogCategory::Move;
	Entry.Outcome = static_cast<uint8>(ERTMoveOutcome::DisplacementResisted);
	Entry.SrcCell = Target->Cell;
	Entry.TgtCell = Target->Cell; // non si e' mossa: dirlo, non lasciarlo dedurre dall'assenza di una voce
	Entry.Amount = static_cast<int32>(Reason);

	ARTUnit* Key = const_cast<ARTUnit*>(Target);
	if (CauseByTarget)
	{
		if (const FRTDisplacementCause* Cause = CauseByTarget->Find(Key))
		{
			Entry.ActionId = Cause->ActionId;
			Entry.BaseActionId = Cause->BaseActionId;
			Entry.Priority = Cause->Priority;
		}
	}

	AppendLogEntry(Entry, Target);
}

void ARTTurnManager::ApplyPlannedHeals(const TArray<ARTUnit*>& Targets, const TArray<int32>& Amounts,
	const TArray<FRTCellId>& Sources, const TArray<ARTUnit*>& Healers,
	const TArray<FRTActionDef>& Defs)
{
	// Tre regole del catalogo, tutte verificabili: non supera la salute massima · non rimuove stati (si tocca
	// solo `Health`) · **non resuscita** chi e' caduto in questo turno — una cura che riportasse in piedi
	// un'unita' a zero renderebbe il KO reversibile, che e' una regola diversa e non dichiarata da nessuna parte.
	for (int32 h = 0; h < Targets.Num(); ++h)
	{
		ARTUnit* HealTarget = Targets[h];
		if (!HealTarget || !HealTarget->IsAlive())
		{
			// 🔴 **Una cura su chi e' caduto nel frattempo non sparisce in silenzio** ([D-196], `#1447`).
			// Gli attacchi risolvono a priorita' 50-65 e le cure a 70: l'alleato puo' morire NELLO STESSO
			// Blast in cui qualcuno lo stava curando. `CollectHealActions` ha gia' accettato il piano e
			// ANNOTATO l'azione come partita, quindi senza questa voce il replay mostrerebbe un curatore con
			// l'abilita' in ricarica — la scrive `SpendStartedAbilities` a fase finita — e nessuna azione
			// registrata. ⚠️ Fino a `#1451` questa riga diceva «bruciato `ConsumeAbility`»: era vero quando
			// il consumo stava dentro la raccolta, e qui si arriva PRIMA del pagamento, non dopo.
			//
			// E' la terza faccia della stessa asimmetria che D-196 ha chiuso per `OutOfRange` e `NoEffect`.
			// Il motivo `TargetDead` esiste gia' nell'enum: e' esattamente questo.
			// 🔴 **Anche un curatore caduto scrive** (`#1473`, [D-219]), e fino al 2026-08-27 non lo faceva.
			// La riga qui diceva *«un curatore morto non scrive … la voce entrerebbe nell'hash del replay»*,
			// e si smentiva da sola quaranta righe piu' sotto: il percorso di **successo** scrive
			// `AppendLogEntry(Entry, Healers[h])` **senza nessuna guardia di vita**. Stessa funzione, stesso
			// predicato, due risposte — e quella che taceva era la sola a perdere informazione.
			//
			// ⚠️ **La morte del curatore non cambia se la cura e' avvenuta.** Se e' avvenuta, la traccia deve
			// dirlo: e' la classe che [D-196] ha chiuso quattro volte e [D-203] una quinta — *non una riga di
			// troppo, una che non c'e'*.
			//
			// ⚠️ **E «entra nell'hash» non era un argomento contro scriverla**: era un argomento contro
			// scriverla in modo NON DETERMINISTICO. La simultaneita' del Blast e' ordinata, e queste voci
			// escono nell'ordine di raccolta di `CollectHealActions`.
			//
			// ⚠️ **Non e' la stessa regola di `CollectHealActions` e `ResolveCleanseActions`**, che rifiutano
			// un cadavere e continuano a farlo: li' si decide se un'unita' **agisce**, e un morto non agisce.
			// Qui si decide se si **registra** qualcosa che e' gia' successo, e la morte non lo disfa.
			ARTUnit* Curatore = Healers.IsValidIndex(h) ? Healers[h] : nullptr;
			if (Curatore)
			{
				// ⚠️ Il motivo distingue i due casi che la condizione qui sopra mette insieme: `TargetGone`
				// se il puntatore non c'e' piu' — l'Actor e' stato distrutto fra raccolta e applicazione — e
				// `TargetDead` se l'unita' c'e' ed e' caduta. L'enum li separa, e `Amount` entra nell'hash.
				const ERTActionInvalidReason Motivo = HealTarget
					? ERTActionInvalidReason::TargetDead : ERTActionInvalidReason::TargetGone;

				// ⚠️ Se la def manca, la voce si scrive lo stesso: il percorso di SUCCESSO qui sotto tollera
				// entrambe le degradazioni, e una via di fallimento piu' silenziosa di quella di successo
				// riporterebbe il silenzio che questa voce esiste per togliere.
				FRTTurnLogEntry Mancata;
				Mancata.Phase = ERTMatchPhase::Blast;
				Mancata.Category = ERTLogCategory::Fallback;
				Mancata.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
				Mancata.SrcCell = Sources.IsValidIndex(h) ? Sources[h] : Curatore->Cell;
				Mancata.TgtCell = HealTarget ? HealTarget->Cell : Mancata.SrcCell;
				Mancata.Amount = static_cast<int32>(Motivo);
				if (Defs.IsValidIndex(h))
				{
					Mancata.ActionId = Defs[h].ActionId;
					Mancata.BaseActionId = Defs[h].BaseActionId;
					Mancata.Priority = Defs[h].Priority;
				}
				AppendLogEntry(Mancata, Curatore);
			}
			continue;
		}

		const int32 Before = HealTarget->Health;
		HealTarget->ApplyCombatState(FMath::Min(HealTarget->MaxHealth, Before + Amounts[h]), HealTarget->Shield);
		const int32 Restored = HealTarget->Health - Before;

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Combat;
		Entry.Outcome = static_cast<uint8>(ERTCombatOutcome::Healed);
		// QUALE azione ha curato, non l'azione generica ([D-195], `#1443`): una cura da `Gadget.Medkit` si
		// legge come tale, e i percorsi di fallimento della stessa cura la nominano gia' cosi'. Scriverlo a
		// mano dava allo stesso gadget due nomi a seconda che avesse funzionato — e `ActionId` entra
		// nell'hash, quindi la traccia archiviata era autoritativa e sbagliata.
		if (Defs.IsValidIndex(h))
		{
			Entry.ActionId = Defs[h].ActionId;
			Entry.BaseActionId = Defs[h].BaseActionId;
			Entry.Priority = Defs[h].Priority;
		}
		Entry.SrcCell = Sources.IsValidIndex(h) ? Sources[h] : HealTarget->Cell;
		Entry.TgtCell = HealTarget->Cell;
		Entry.Amount = Restored; // quanto e' stato curato DAVVERO: a salute piena la voce dice zero
		// L'attore e' chi CURA, non chi viene curato: la voce dice chi ha agito ([D-063]).
		AppendLogEntry(Entry, Healers.IsValidIndex(h) ? Healers[h] : nullptr);
		AddLogEvent(FString::Printf(TEXT("%s: +%d salute"), *HealTarget->GetName(), Restored), FRTLogSubject::Unit(HealTarget));
	}
}

bool ARTTurnManager::ApplyDynamicSurface(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexSurface NewSurface,
	int32 Turns, const FName& CauseActionId, const ARTUnit* Cause)
{
	const FRTHexCellData* Existing = Map ? Map->FindCell(Cell) : nullptr;
	if (!Existing || Turns <= 0)
	{
		return false; // fuori mappa o durata non positiva: non si modifica il campo per sbaglio
	}
	if (Existing->Surface == NewSurface)
	{
		return false; // gia' cosi': nessun cambiamento da registrare
	}

	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Cleanup;
	Entry.Category = ERTLogCategory::Environment;
	Entry.ActionId = CauseActionId;
	Entry.SrcCell = Cell;
	Entry.TgtCell = Cell;

	// Il fuoco non attecchisce su cio' che non brucia (catalogo terreni §2: «non incendia automaticamente
	// acqua o metallo»). E' una proprieta' della superficie di DESTINAZIONE, letta dal catalogo — non un
	// elenco di eccezioni scritto qui.
	if (NewSurface == ERTHexSurface::Fire && !URTTerrainLibrary::FindTerrainDef(Existing->Surface).bIsFlammable)
	{
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::SurfaceRejected);
		Entry.Amount = 0;
		AppendLogEntry(Entry, Cause); // la trasformazione e' ambientale, ma qualcuno l'ha tentata
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): non prende fuoco"), Cell.X, Cell.Y, Cell.Layer), FRTLogSubject::World());
		return false;
	}

	// L'acqua SPEGNE il fuoco: e' la stessa trasformazione, ma il TurnLog la distingue perche' per chi legge
	// il replay «la cella si allaga» e «la cella si spegne» non sono lo stesso evento.
	const bool bExtinguishes = (Existing->Surface == ERTHexSurface::Fire
		&& NewSurface == ERTHexSurface::ShallowWater);

	// L'ORIGINALE si registra una volta sola: una cella allagata e poi incendiata deve tornare al pavimento,
	// non all'acqua che c'era un turno prima.
	FRTDynamicSurface& State = DynamicSurfaces.FindOrAdd(Cell);
	if (State.TurnsRemaining <= 0)
	{
		State.Original = Existing->Surface;
	}
	State.TurnsRemaining = Turns;

	// La superficie corrente va nella MAPPA, che e' cio' che tutti leggono: il costo di movimento segue il
	// catalogo della nuova superficie, altrimenti una pozza costerebbe ancora quanto il pavimento.
	FRTHexCellData Updated = *Existing;
	Updated.Surface = NewSurface;
	Updated.MoveCost = URTTerrainLibrary::FindTerrainDef(NewSurface).MoveCost;
	Map->AddOrUpdateCell(Updated);

	Entry.Outcome = static_cast<uint8>(
		bExtinguishes ? ERTEnvironmentOutcome::SurfaceExtinguished : ERTEnvironmentOutcome::SurfaceChanged);
	Entry.Amount = Turns;
	AppendLogEntry(Entry, Cause);
	AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): %s (%d turni)"), Cell.X, Cell.Y, Cell.Layer,
		bExtinguishes ? TEXT("il fuoco si spegne") : TEXT("la superficie cambia"), Turns), FRTLogSubject::World());
	return true;
}

void ARTTurnManager::TickDynamicArcs(URTHexMapAsset* Map)
{
	if (!Map || DynamicArcs.Num() == 0) { return; }

	// Ordine STABILE: da qui escono voci di TurnLog, e due esecuzioni della stessa partita devono scriverle
	// nella stessa sequenza (la stessa disciplina di `TickDynamicSurfaces`).
	DynamicArcs.Sort([](const FRTDynamicArc& A, const FRTDynamicArc& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});

	for (int32 I = DynamicArcs.Num() - 1; I >= 0; --I)
	{
		FRTDynamicArc& Arc = DynamicArcs[I];
		if (Arc.CreatedOnTurn == TurnNumber)
		{
			continue; // nato in questo turno: i suoi turni cominciano dal prossimo, non da adesso
		}
		if (--Arc.TurnsRemaining > 0)
		{
			continue; // il ponte regge ancora
		}

		// Scaduto: l'arco sparisce dalla mappa. I due layer tornano irraggiungibili e il percorso FALLISCE —
		// il Move del turno successivo lo scopre da `GraphNeighbors`, non da un secondo controllo qui.
		const FRTCellId From = Arc.From;
		const FRTCellId To = Arc.To;
		DynamicArcs.RemoveAt(I);
		if (!Map->RemoveTransition(From, To, /*bBothDirections*/ true))
		{
			continue; // gia' tolto da un `ModifyArc`: niente da registrare
		}
		// NOTA (#302): questo ramo NON e' la rete di sicurezza per i ponti distrutti in combattimento, e
		// crederlo e' costato un difetto. `DamageArc` non rimuove l'arco — lo marca `Destroyed` e lo lascia
		// sulla mappa — quindi su un ponte crollato `RemoveTransition` RIUSCIREBBE, e da qui uscirebbe un
		// `BridgeRemoved` per un evento mai avvenuto. Un ponte abbattuto non arriva piu' fin qui: la sua entry
		// e' tolta da `DynamicArcs` nel momento del crollo.

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Cleanup;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::BridgeRemoved);
		// ⚠️ **Resta l'azione generica, e stavolta e' corretto**: la scadenza non ha un autore. `FRTDynamicArc`
		// porta arco e durata, non l'identita' di chi l'ha creato, quindi la voce direbbe il falso attribuendo
		// la rimozione a un'azione che nessuno ha eseguito in questo turno.
		//
		// ⚠️ Conseguenza da conoscere ([D-197], `#1447`): un ponte creato da un GADGET si legge col nome del
		// pezzo alla creazione e con `Action.ModifyArc` alla scadenza. Non e' lo stesso difetto — la seconda
		// voce non ha un autore da nominare — ma un consumatore che accoppia le due per `ActionId` non le
		// trova. Chiuderlo vuol dire portare l'identita' dentro `FRTDynamicArc`, cioe' toccare cio' che il
		// formato archivia: aperto a parte.
		Entry.ActionId = FName(TEXT("Action.ModifyArc"));
		Entry.SrcCell = From;
		Entry.TgtCell = To;
		Entry.Amount = 0;
		// Scadenza nel Cleanup: nessuno l'ha fatta, il turno e' passato. E' il caso per cui `UnitId = 0`
		// significa davvero «nessuna unita'» e non «non lo sappiamo» ([D-063]).
		AppendLogEntry(Entry, nullptr);
		AddLogEvent(FString::Printf(TEXT("Il ponte (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d) e' scaduto"),
			From.X, From.Y, From.Layer, To.X, To.Y, To.Layer), FRTLogSubject::World());
	}
}

void ARTTurnManager::TickDynamicCovers(URTHexMapAsset* Map)
{
	if (!Map || DynamicCovers.Num() == 0) { return; }

	// Ordine STABILE: da qui escono voci di TurnLog, e due esecuzioni della stessa partita devono scriverle
	// nella stessa sequenza (la stessa disciplina di `TickDynamicArcs`).
	DynamicCovers.Sort([](const FRTDynamicCover& A, const FRTDynamicCover& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
	});

	for (int32 I = DynamicCovers.Num() - 1; I >= 0; --I)
	{
		FRTDynamicCover& Cover = DynamicCovers[I];
		if (Cover.TurnsRemaining <= 0)
		{
			continue; // non scade da sola (pannello adattivo): resta finche' qualcuno non l'abbatte
		}
		if (--Cover.TurnsRemaining > 0)
		{
			continue; // il pannello regge ancora
		}

		const FRTCellId Cell = Cover.Cell;
		const ERTHexDirection Edge = Cover.Edge;
		DynamicCovers.RemoveAt(I);

		// La cella OLTRE il bordo: il TurnLog identifica una struttura con la coppia di celle, come per
		// coperture danneggiate, porte e ponti — nessun campo direzione nel log.
		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Cell);
		const int32 EdgeIndex = static_cast<int32>(Edge);
		const FRTCellId Toward = Ring.IsValidIndex(EdgeIndex) ? Ring[EdgeIndex] : Cell;

		if (!URTHexCoverLibrary::RemoveCover(Map, Cell, Edge))
		{
			continue; // gia' sparita per altra via (abbattuta in combattimento): niente da registrare
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Cleanup;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverExpired);
		Entry.ActionId = FName(TEXT("Action.CreateCover"));
		Entry.SrcCell = Cell;
		Entry.TgtCell = Toward;
		Entry.Amount = 0;
		AppendLogEntry(Entry, nullptr); // scadenza: nessun attore
		AddLogEvent(FString::Printf(TEXT("La copertura (q=%d,r=%d,L%d) e' scaduta"),
			Cell.X, Cell.Y, Cell.Layer), FRTLogSubject::World());
	}
}

void ARTTurnManager::TickDynamicSurfaces(URTHexMapAsset* Map)
{
	if (!Map || DynamicSurfaces.Num() == 0) { return; }

	// Ordine STABILE: `TMap` non ha un ordine garantito, e da qui escono voci di TurnLog. Senza questo, due
	// esecuzioni della stessa partita produrrebbero lo stesso insieme di voci in ordine diverso — l'hash e'
	// permutazione-invariante e non se ne accorgerebbe, ma il combat log letto da un umano si', e il giorno in
	// cui qualcosa dipendesse dall'ordine il difetto sarebbe gia' dentro.
	TArray<FRTCellId> Cells;
	DynamicSurfaces.GetKeys(Cells);
	Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });

	for (const FRTCellId& Cell : Cells)
	{
		FRTDynamicSurface& State = DynamicSurfaces[Cell];
		if (--State.TurnsRemaining > 0)
		{
			continue;
		}

		if (const FRTHexCellData* Current = Map->FindCell(Cell))
		{
			FRTHexCellData Restored = *Current;
			Restored.Surface = State.Original;
			Restored.MoveCost = URTTerrainLibrary::FindTerrainDef(State.Original).MoveCost;
			Map->AddOrUpdateCell(Restored);

			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Cleanup;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::SurfaceRestored);
			Entry.SrcCell = Cell;
			Entry.TgtCell = Cell;
			Entry.Amount = 0;
			AppendLogEntry(Entry, nullptr); // scadenza: nessun attore
			AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): la superficie torna com'era"),
				Cell.X, Cell.Y, Cell.Layer), FRTLogSubject::World());
		}
		DynamicSurfaces.Remove(Cell);
	}
}

namespace
{
	/**
	 * Ordine TOTALE del roster di partita, sullo stato iniziale delle unita'.
	 *
	 * Non si usa l'ordine di `GetAllActorsOfClass`: quello e' l'ordine in cui il livello tiene gli Actor, che
	 * non e' un dato di gioco. Se decidesse l'identita', due esecuzioni della stessa partita produrrebbero due
	 * tracce diverse — la classe di difetto che `InstanceLess` e `EntryLess` esistono per impedire.
	 *
	 * L'ultimo confronto e' il NOME dell'Actor, e serve solo in un caso che il gioco non produce: due unita'
	 * della stessa squadra sulla stessa cella all'inizio. Sta li' perche' un pareggio lo risolverebbe
	 * altrimenti `TArray::Sort`, che non e' stabile — la stessa trappola gia' pagata con `EntryLess` (D-067).
	 */
	bool MatchRosterLess(const ARTUnit& A, const ARTUnit& B)
	{
		if (A.TeamId != B.TeamId)       { return A.TeamId < B.TeamId; }
		if (A.Cell.X != B.Cell.X)       { return A.Cell.X < B.Cell.X; }
		if (A.Cell.Y != B.Cell.Y)       { return A.Cell.Y < B.Cell.Y; }
		if (A.Cell.Layer != B.Cell.Layer) { return A.Cell.Layer < B.Cell.Layer; }
		return A.GetName().Compare(B.GetName()) < 0;
	}
}

void ARTTurnManager::EnsureMatchRoster()
{
	if (bMatchRosterBuilt)
	{
		return; // l'identita' si assegna una volta: riassegnarla sarebbe il difetto che il campo deve evitare
	}

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Roster;
	Roster.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			// NON si filtra sui vivi, ed e' l'intero punto: filtrare e' cio' che fa scalare l'indice dello
			// snapshot. Alla prima risoluzione sono comunque tutti vivi — il filtro sarebbe inutile adesso e
			// dannoso come precedente.
			Roster.Add(Unit);
		}
	}
	if (Roster.Num() == 0)
	{
		// Nessuna unita' nel mondo: non si «costruisce» un roster vuoto, perche' congelarlo adesso darebbe
		// identita' a nessuno e le negherebbe a chi arriva dopo. Si riprova alla risoluzione successiva.
		return;
	}

	Roster.Sort([](const ARTUnit& A, const ARTUnit& B) { return MatchRosterLess(A, B); });

	MatchRoster.Reset(Roster.Num());
	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		// `+ 1`: lo `0` resta libero e significa «nessuna unita' dichiarata» ([D-063]), che e' cio' che dice
		// una voce ambientale del TurnLog.
		Roster[i]->StableUnitId = i + 1;

		// L'indice inverso, riempito **con la stessa lista gia' ordinata**: e' il motivo per cui la porta
		// `UnitByStableId` non costa un secondo `GetAllActorsOfClass`. Cio' che prima si buttava via qui
		// e' cio' che alla presentazione serve per tornare all'Actor da un fatto che porta il solo id.
		MatchRoster.Add(Roster[i]);
	}
	bMatchRosterBuilt = true;
}

int32 ARTTurnManager::RosterIndexForStableId(int32 StableUnitId, int32 RosterNum)
{
	// Lo `0` non e' un id ([D-063]) e non ha un indice. Un id oltre il roster e' di un'unita' arrivata
	// DOPO il congelamento: non ne aveva uno allora e non ne acquista uno adesso.
	if (StableUnitId <= 0 || StableUnitId > RosterNum)
	{
		return INDEX_NONE;
	}
	return StableUnitId - 1;
}

ARTUnit* ARTTurnManager::UnitByStableId(int32 StableUnitId) const
{
	const int32 Index = RosterIndexForStableId(StableUnitId, MatchRoster.Num());
	if (Index == INDEX_NONE)
	{
		return nullptr;
	}
	// `.Get()` e non `.IsValid()` + deref: una entry scaduta risponde `nullptr`, che per chi anima significa
	// «quell'unita' non c'e' piu'» — lo stesso `nullptr` del `TWeakObjectPtr` che stava dentro l'evento.
	return MatchRoster[Index].Get();
}

int32 ARTTurnManager::CurrentGraphRevision() const
{
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		if (const URTHexMapAsset* Asset = HexMap->MapAsset)
		{
			return Asset->Revision;
		}
	}
	// Nessuna mappa autorevole: `0` dice «non dichiarata», che e' vero, invece di un numero inventato.
	return 0;
}

void ARTTurnManager::AppendLogEntry(FRTTurnLogEntry& Entry, const ARTUnit* Actor)
{
	Entry.TurnNumber = TurnNumber;
	// Letta ADESSO e non a inizio turno: una porta che si apre o un ponte che crolla la fanno salire in mezzo
	// alla risoluzione, ed e' esattamente la ragione per cui il campo sta nella voce e non nell'header.
	Entry.GraphRevision = CurrentGraphRevision();
	// Chi ha AGITO, che e' la ragione per cui `UnitId` esiste ([D-063]): la cella non lo identifica — le voci
	// ambientali non hanno un'unita', l'interposizione scrive in `SrcCell` la cella del PROTETTO, e dopo un
	// Dash la cella dell'attore in fase Blast non e' piu' quella di partenza. Per questo l'attore arriva come
	// parametro e non si deduce.
	//
	// ⚠️ **Non per TUTTE le voci**, e chi aggiunge un produttore deve saperlo prima di scegliere cosa
	// passare: alcune famiglie invertono e mettono qui CHI SUBISCE. L'elenco e la ragione di ognuna stanno
	// nel commento di `FRTTurnLogEntry::UnitId`; la domanda si fa a
	// `URTTurnLogLibrary::IsSubjectTheSufferer`, che porta la tassonomia in un posto solo invece di lasciarla
	// a chi si ricorda di aver letto la prosa.
	//
	// `nullptr` -> `0`, cioe' «nessuna unita' dichiarata». Il parametro e' OBBLIGATORIO di proposito: reso
	// opzionale, un sito nuovo erediterebbe lo zero in silenzio e la voce direbbe «nessuno» invece di tacere.
	Entry.UnitId = Actor ? Actor->StableUnitId : 0;

	// 🔴 **Il verdetto di [D-223] si congela QUI, ed e' l'unico punto in cui e' corretto farlo.** Questa
	// funzione gira DURANTE la fase che produce il fatto: la conoscenza di squadra e la cella dell'attore
	// appartengono allo stesso istante. Il canale che deriva il combat log da queste voci gira invece a fine
	// turno (`ConcludeTurn`), quando la conoscenza e' quella del Blast e le celle sono gia' post-Move: li'
	// i due ingressi verrebbero da momenti diversi, e `AwarenessOfUnit` — che confronta la cella con
	// `VisibleCells` — restituirebbe un verdetto che non e' quello del fatto.
	//
	// Una voce senza attore e' un fatto di MONDO e lo dichiara: una superficie che scade, un ponte che
	// crolla. Non e' l'assenza di una decisione.
	Entry.Verdict = Actor ? FreezeVerdictFor(FRTLogSubject::Unit(Actor))
	                      : FRTKnowledgeVerdict::Everyone();

	// L'UNICO `TurnLog.Add` del file: ogni altro sito passa da qui.
	TurnLog.Add(Entry);
}

void ARTTurnManager::RecordFacingChange(FRTHexSimUnit& Unit, ERTHexDirection NewFacing, ERTFacingOutcome Reason,
	ERTMatchPhase LogPhase, const ARTUnit* Actor)
{
	// ⚠️ Un array LOCALE, non `TurnLog`: e' la riga che fa la differenza. La libreria scrive qui, e le voci
	// entrano nel TurnLog solo attraverso `AppendLogEntry`, che ci stampa turno, revisione del grafo e unita'.
	TArray<FRTTurnLogEntry> Prodotte;
	URTFacingLibrary::RecordFacingChange(Unit, NewFacing, Reason, LogPhase, Prodotte);
	for (FRTTurnLogEntry& Voce : Prodotte)
	{
		AppendLogEntry(Voce, Actor);
	}
}

void ARTTurnManager::ValidatePlansAtLockIn()
{
	// Le unita' vengono dal punto UNICO che le raccoglie e le ordina, non da un giro a mano: `Sort` a parte,
	// l'ordine di spawn deciderebbe in che sequenza compaiono le righe di rifiuto.
	//
	// ⚠️ **Qui non serve la MAPPA**, che e' la parte cara di `MakeCurrentSnapshot`: la vista di occupazione,
	// l'hash del terreno e una copia di `TeamKnowledgeState`, tutto costruito a ogni commit di turno e
	// scartato. Serve lo STATO DELL'UNITA', e quello lo costruisce `MakeSimUnit` — lo stesso helper che
	// riempie lo snapshot, quindi senza campi dimenticati.
	//
	// 🔴 **Non un `FRTHexSimUnit()` di default**, che sarebbe stato piu' corto e sbagliato: [D-190] tiene
	// `Unit` nella firma di `ValidatePlan` perche' *«il bot e CP 38.3 lo useranno»*, e il giorno in cui
	// qualcuno tornasse a leggerlo ogni piano verrebbe giudicato contro un'unita' che non esiste — cella
	// `(0,0,0)`, budget `0` — senza che niente diventi rosso. Passare lo stato vero non costa nulla e toglie
	// la trappola.
	TArray<ARTUnit*> Units;
	CollectLivingUnits(Units);

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		// `CollectLivingUnits` aggiunge solo puntatori che hanno passato `Unit && Unit->IsAlive()`:
		// un controllo di nullita' qui difenderebbe da niente.
		ARTUnit* Unit = Units[i];

		const TArray<FRTPlannedAction> Plan = URTPlanValidationLibrary::MakePlanFor(Unit);
		if (Plan.Num() == 0)
		{
			continue; // nessuna voce: niente da giudicare
		}

		const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(MakeSimUnit(i, Unit), Plan);
		if (Verdict.bLegal)
		{
			continue;
		}

		// 🔴 **Nessuna voce nel TurnLog da qui, ed e' una scelta.** Il TurnLog e' un formato serializzato,
		// ordinato canonicamente e riprodotto: una voce scritta al lock-in porterebbe una `Phase` che nessun
		// consumatore del replay ha mai visto, e dovrebbe nominare un'azione «colpevole» che l'ordine
		// canonico del validatore sceglie in modo diverso da come il resolver scarta. Cio' che il turno
		// scarta davvero lo dice `ResolveDash`, dove lo scarta — con `ERTMoveOutcome::SupersededByDash`.
		//
		// Qui resta il **combat log**, che e' cio' che serve: un piano incoerente ha un posto in cui
		// comparire mentre lo si compone, senza entrare nel formato che i replay confrontano.
		// 🔴 **Entrambe le azioni, mai una sola.** `OffendingActionId` e' l'azione che l'ordine canonico del
		// validatore incontra per seconda, e per il caso canonico scatto + movimento e' la MOBILITA', che
		// invece esegue. Nominare lei significa mandare il giocatore a correggere l'azione sbagliata.
		// Dire che due azioni occupano lo stesso slot e' vero comunque il resolver decida.
		const FString Dettaglio = Verdict.HolderActionId.IsNone()
			? FString::Printf(TEXT("%s: %s"),
				*Verdict.OffendingActionId.ToString(),
				*URTTurnLogLibrary::DescribeInvalidReason(Verdict.Reason))
			: FString::Printf(TEXT("%s e %s occupano lo stesso slot"),
				*Verdict.OffendingActionId.ToString(),
				*Verdict.HolderActionId.ToString());

		AddLogEvent(FString::Printf(TEXT("%s: piano non valido al lock-in (%s)"),
			*Unit->GetName(), *Dettaglio), FRTLogSubject::Unit(Unit));
	}
}

void ARTTurnManager::ResolveEnvironment(URTHexMapAsset* Map)
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	Units.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}
	if (Units.Num() == 0) { return; }
	// Stesso ordine stabile per cella del resto del turno: da qui dipendono gli indici passati alla libreria
	// e l'ordine in cui due scariche dello stesso turno si applicano.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	// Celle la cui SUPERFICIE nasce in questo Cleanup (`#570`). Si raccolgono qui e i loro effetti si
	// applicano in fondo, a tutte le trasformazioni decise: e' lo stesso "raccogli poi applica" del resto del
	// motore, e serve a un secondo scopo — lasciare un punto in cui una reazione puo' inserirsi PRIMA che gli
	// effetti tocchino l'unita' (`Reaction.HazardEscape`, `#505`). Applicandoli dentro il ciclo, una fuga
	// arriverebbe sempre tardi: l'unita' porterebbe `Burning` con se' e il danno del Cleanup la
	// raggiungerebbe comunque.
	TArray<FRTCellId> BornSurfaceCells;

	// Snapshot delle unita' PRIMA di applicare qualunque danno: "raccogli poi applica" (invariante #3). Due
	// scariche nello stesso Cleanup vedono lo stesso campo, quindi il loro esito non dipende dall'ordine.
	TArray<FRTHexCombatUnit> HexUnits;
	HexUnits.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		FRTHexCombatUnit HexUnit;
		HexUnit.UnitId = i;
		HexUnit.TeamId = Units[i]->TeamId;
		HexUnit.Cell = Units[i]->Cell;
		HexUnit.bAlive = Units[i]->IsAlive();
		HexUnits.Add(HexUnit);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Caster = Units[i];
		const int32 AbilityIndex = Caster->PlannedAbilityIndex;
		const URTActionData* Ability = Caster->GetAbility(AbilityIndex);
		if (!Ability
			|| URTCatalogLibrary::MapResolutionPhase(Ability->Def.ResolutionPhase) != ERTMatchPhase::Cleanup)
		{
			continue; // nessuna azione ambientale pianificata da questa unita'
		}

		ARTUnit* Target = Caster->PlannedAttackTarget;
		Caster->PlannedAbilityIndex = INDEX_NONE; // consumato: attivata o no, il piano non sopravvive al turno
		Caster->PlannedAttackTarget = nullptr;
		if (!Caster->CanUseAbility(AbilityIndex)) { continue; }

		// Il fallback dichiarato di `Action.Electrify` e' `Cancel`: senza bersaglio valido non succede nulla,
		// e non si sceglie un bersaglio di ripiego (il catalogo vieta le scelte implicite).
		if (!Target || !Target->IsAlive())
		{
			AddLogEvent(FString::Printf(TEXT("%s: %s annullata (nessun bersaglio)"),
				*Caster->GetName(), *Ability->Def.ActionId.ToString()), FRTLogSubject::Unit(Caster));
			continue;
		}

		// Azioni che modificano la MAPPA (CP 8.4). Quale superficie creano lo dice l'ActionId, ed e' l'unico
		// punto in cui questo orchestratore lo guarda: la coppia azione->superficie non e' esprimibile come
		// `FRTActionEffectSpec` (gli effetti agiscono su unita', non su celle) e inventare un campo
		// «SurfaceCreated» nel catalogo per due sole azioni sarebbe un dato che nessun'altra azione useria.
		// Quando le azioni ambientali saranno molte (CP 8.5), il posto giusto e' quel campo.
		{
			// Dal DATO, non dal nome (D-046, #282). Il confronto per ActionId funzionava finche' le uniche
			// azioni ambientali erano quelle del catalogo core; non appena un eroe ne possiede una — e
			// `Phase.FluidTrail` non puo' chiamarsi `Action.CreateWater` — un `if` sul nome smette di poter
			// esprimere «e' quell'azione». Stessa strada di `PropagationLimit`, che infatti gia' funzionava
			// per l'eroe che aveva ereditato `Action.Electrify`.
			const bool bCreatesSurface = Ability->Def.bCreatesSurface;
			const ERTHexSurface Created = Ability->Def.SurfaceCreated;
			// La CAUSA registrata nel TurnLog resta l'ActionId dell'abilita' usata: con un eroe owner e'
			// `Phase.FluidTrail`, non `Action.CreateWater`. Chi legge il replay deve vedere CHI ha allagato,
			// non la primitiva che c'e' sotto — l'identita' dell'eroe e' meta' del valore del log.
			const FName EnvActionId = Ability->Def.ActionId;

			if (bCreatesSurface)
			{
				Caster->ConsumeAbility(AbilityIndex);
				// Durata 2 turni per entrambe (catalogo terreni §2 per il fuoco, catalogo azioni §6 per l'acqua).
				// La cella e' quella del bersaglio, come per la scarica: stesso limite dichiarato sul
				// targeting per cella.
				//
				// Il raggio e' dell'AZIONE, non della superficie. Era `(Created == ShallowWater) ? 1 : 0`, cioe'
				// il ramo che il commento qui sopra dichiara di voler evitare: con tre produttori — acqua 1,
				// fuoco 0, fumo 1 (`Phase.MistVeil`, #353) — quel ramo dovrebbe indovinare quale superficie
				// vuole quale area, e sbaglierebbe alla prima azione che allaga una cella sola.
				const int32 Radius = FMath::Max(0, Ability->Def.SurfaceRadius);
				// Ordine STABILE delle celle: `HexArea` restituisce gia' un'area ordinata, quindi le voci di
				// TurnLog escono sempre nella stessa sequenza (#4).
				for (const FRTCellId& Cell : URTHexLibrary::HexArea(Target->Cell, Radius))
				{
					if (!ApplyDynamicSurface(Map, Cell, Created, /*Turns*/ 2, EnvActionId, Caster))
					{
						continue; // cella fuori mappa, gia' cosi', o che non ammette la trasformazione
					}
					// La cella ha cambiato superficie: chi ci si trova sopra ne subira' gli effetti, e si
					// RACCOGLIE soltanto (vedi in fondo alla funzione, `#570`).
					//
					// `AddUnique` e non `Add`: due azioni ambientali possono trasformare la STESSA cella nello
					// stesso Cleanup, e la cella comparirebbe due volte. Gli effetti si leggono dalla mappa al
					// momento dell'applicazione, quindi sarebbero quelli della superficie FINALE applicati due
					// volte.
					//
					// ⚠️ Col catalogo di oggi la differenza **non e' osservabile**, e vale la pena dirlo invece
					// di lasciar credere che questa riga stia salvando una partita: la stessa superficie due
					// volte non arriva qui (`ApplyDynamicSurface` scarta il caso «gia' cosi'»), il fuoco e'
					// rifiutato su cio' che non brucia, e l'unica doppia trasformazione raggiungibile —
					// fuoco poi acqua — riapplica `Wet`, che e' idempotente. Diventa un difetto vero il giorno
					// in cui una superficie con `Damage` sia raggiungibile due volte nello stesso Cleanup:
					// `AddUnique` costa zero e toglie la classe di errore a monte, invece di aspettare quel
					// giorno.
					BornSurfaceCells.AddUnique(Cell);
				}
				continue;
			}

			// `Action.ModifyArc` NON passa piu' di qui: dal CP 9.4 risolve nel **Blast** insieme a porte e
			// strutture, perche' la topologia deve cambiare tutta nello stesso momento e il Move che segue
			// deve vederla. Il suo ramo vive in `ResolveCombat`.
		}

		// La SORGENTE e' la cella del bersaglio, non l'unita': l'elettricita' entra nel terreno e da li' si
		// propaga. **Limite dichiarato (CP 8.3)**: il catalogo prevede anche «colpisce una cella conduttiva»
		// senza unita' sopra, ma la pianificazione non ha ancora un bersaglio-cella per le azioni
		// (`PlannedAttackTarget` e' un'unita'); arrivera' col targeting per cella dell'HUD (E11).
		const int32 InitialDamage = URTCatalogLibrary::FirstDamage(Ability->Def);
		const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
			Map, Target->Cell, Ability->Def.PropagationLimit, InitialDamage,
			URTCombatLibrary::PropagatedElectricDamage, HexUnits);

		Caster->ConsumeAbility(AbilityIndex);
		if (Hits.Num() == 0)
		{
			// Nessun colpo: senza mappa autorevole (fail-closed) o con il bersaglio ormai fuori dallo snapshot.
			AddLogEvent(FString::Printf(TEXT("%s: %s senza effetto"),
				*Caster->GetName(), *Ability->Def.ActionId.ToString()), FRTLogSubject::Unit(Caster));
			continue;
		}

		// APPLICA nell'ordine dichiarato dalla libreria (distanza -> cella -> unita'), che e' anche l'ordine
		// in cui le voci finiscono nel TurnLog: il replay racconta la scarica come si e' propagata.
		for (const FRTPropagationHit& Hit : Hits)
		{
			if (!Units.IsValidIndex(Hit.UnitId) || !Units[Hit.UnitId] || !Units[Hit.UnitId]->IsAlive())
			{
				continue;
			}
			ARTUnit* Victim = Units[Hit.UnitId];
			// `Environmental`: e' la propagazione elettrica, che nessuno ha mirato. Salta lo scudo base.
			const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Hit.Damage,
				ERTDamageSource::Environmental, Victim->Shield, Victim->GetTemporaryShield(), Victim->Health);
			Victim->ApplyCombatState(Result.Health, Result.Shield);

			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Cleanup;
			Entry.Category = ERTLogCategory::Combat;
			Entry.ActionId = Ability->Def.ActionId; // identita' dell'azione: un danno senza causa e' inspiegabile
			Entry.BaseActionId = Ability->Def.BaseActionId; // di quale generica e' un profilo (D-033, #354)
			Entry.SrcCell = Caster->Cell;
			Entry.TgtCell = Hit.Cell;
			Entry.Amount = Hit.Damage;
			Entry.Outcome = static_cast<uint8>(
				!Victim->IsAlive() ? ERTCombatOutcome::Lethal
				: (Result.Health == Victim->MaxHealth || Hit.Damage <= 0) ? ERTCombatOutcome::ShieldAbsorbed
				: ERTCombatOutcome::Hit);
			AppendLogEntry(Entry, Caster); // chi ha colpito, non chi e' stato colpito

			AddLogEvent(FString::Printf(TEXT("%s: %d danni da %s (%d %s)"),
				*Victim->GetName(), Hit.Damage, *Ability->Def.ActionId.ToString(),
				Hit.Steps, Hit.Steps == 0 ? TEXT("colpo diretto") : TEXT("celle di propagazione")), FRTLogSubject::Unit(Victim));
			if (!Victim->IsAlive())
			{
				AddLogEvent(FString::Printf(TEXT("%s eliminato dalla scarica"), *Victim->GetName()), FRTLogSubject::World());
			}
		}
	}

	// Le superfici NATE in questo Cleanup fanno effetto a chi ci si trova sopra (`#570`).
	//
	// Fino a qui la regola esisteva per l'acqua sola, con un `if (Created == ShallowWater)` scritto a mano e
	// un commento che dichiarava il problema nella sua forma generale: «gli `OnEnterEffects` valgono per chi
	// ENTRA, e aspettare che escano e rientrino sarebbe una regola che nessuno capirebbe guardando il campo».
	// Vero per l'acqua e per tutte le altre: un'unita' ferma su cui viene acceso un incendio **non prendeva
	// fuoco**. Ora la regola e' una sola e legge il catalogo, quindi la prossima superficie non deve
	// ricordarsi di entrare in un elenco.
	//
	// Si riusa `ApplyTerrainOnEnterEffects`, cioe' la stessa funzione che serve chi entra: `Damage` passa da
	// `ApplyCombatState` (l'unica contabilita' che erode anche lo scudo temporaneo) e `Status` conserva la
	// sentinella `PersistentWhileOnCell` per le durate 0. Riscriverla qui avrebbe prodotto una seconda
	// versione capace di divergere — ed e' esattamente com'era nata l'asimmetria.
	//
	// ⚠️ Conseguenza dichiarata: creare fuoco sotto un bersaglio fermo passa da 0 a **10 danni + `Burning 2`**
	// (catalogo terreni). E' un'apertura offensiva nuova, non un effetto collaterale.
	//
	// L'ordine e' quello di raccolta (`HexArea` gia' ordinata) incrociato con `Units`, ordinate per cella piu'
	// sopra: due unita' sulla stessa trasformazione ricevono gli effetti sempre nella stessa sequenza.
	// --- Fuga dagli hazard (CP 7.5, `#505`): il punto di valutazione del CLEANUP -----------------------
	// Le superfici sono nate e i loro effetti non hanno ancora toccato nessuno: e' l'unico istante in cui
	// `Reaction.HazardEscape` salva davvero. Un istante prima non c'e' niente di pericoloso; uno dopo l'unita'
	// ha gia' `Burning` addosso, e fuggire non glielo toglie — sarebbe teatro.
	//
	// `NoStates`: fuori dal Blast non esiste uno snapshot di combattimento da aggiornare. Il pass lo usa solo
	// per gli scudi, che qui non avrebbero colpi da fermare.
	TArray<FRTUnitCombatState> NoStates;
	FRTReactionPassResult HazardReactions;
	RunReactionPass(ERTReactionPassPoint::CleanupSurfaceBirth,
		[&Units, &BornSurfaceCells, Map](int32 SelfId, ERTReactionTrigger)
		{
			ARTUnit* Self = Units.IsValidIndex(SelfId) ? Units[SelfId] : nullptr;
			if (!Self || !Self->IsAlive() || !BornSurfaceCells.Contains(Self->Cell))
			{
				return FRTReactionTriggerHit{ false, INDEX_NONE };
			}
			// «Diventata pericolosa», non «cambiata»: allagare la cella sotto un'unita' non le fa scattare la
			// fuga. Il criterio lo dichiara il catalogo (`IsHazardousSurface`), non un elenco qui.
			const FRTHexCellData* Data = Map ? Map->FindCell(Self->Cell) : nullptr;
			const bool bDanger = Data != nullptr && URTTerrainLibrary::IsHazardousSurface(Data->Surface);
			// Nessun `TriggeredBy`: si fugge da una CELLA, e una cella non e' un'unita' ([D-063]).
			return FRTReactionTriggerHit{ bDanger, INDEX_NONE };
		},
		Units, NoStates, Map, HazardReactions);

	// Le destinazioni si calcolano TUTTE prima di muovere chiunque (D-094): due unita' che fuggono nello
	// stesso Cleanup non devono vedersi a vicenda gia' spostate, o l'esito dipenderebbe dall'ordine.
	if (HazardReactions.HazardFlees.Num() > 0)
	{
		TArray<FRTCellId> Occupied;
		for (ARTUnit* U : Units) { if (IsValid(U) && U->IsAlive()) { Occupied.Add(U->Cell); } }

		TArray<FRTCellId> Dest;
		Dest.Reserve(HazardReactions.HazardFlees.Num());
		for (const int32 FleeingId : HazardReactions.HazardFlees)
		{
			ARTUnit* Fleeing = Units.IsValidIndex(FleeingId) ? Units[FleeingId] : nullptr;
			Dest.Add((Fleeing && Fleeing->IsAlive())
				? URTTerrainLibrary::FindEscapeCell(Map, Fleeing->Cell, Fleeing->Facing, Occupied)
				: FRTCellId());
		}

		for (int32 f = 0; f < HazardReactions.HazardFlees.Num(); ++f)
		{
			ARTUnit* Fleeing = Units.IsValidIndex(HazardReactions.HazardFlees[f])
				? Units[HazardReactions.HazardFlees[f]] : nullptr;
			if (!Fleeing || !Fleeing->IsAlive()) { continue; }

			if (Dest[f] == Fleeing->Cell)
			{
				// Circondato: la reazione e' scattata e ha speso la sua attivazione senza salvare nessuno.
				// E' un esito e va detto, come per `EmergencyDash` quando non ha dove andare.
				AddLogEvent(FString::Printf(TEXT("%s: nessuna cella sicura dove fuggire"), *Fleeing->GetName()), FRTLogSubject::Unit(Fleeing));
				continue;
			}

			TMap<ARTUnit*, FRTDisplacementCause> FleeCause;
			FleeCause.Add(Fleeing, FRTDisplacementCause{ FName(TEXT("Reaction.HazardEscape")), NAME_None, 0 });
			// La sorgente del facing e' la cella d'ARRIVO, e non e' un trucco: con sorgente e arrivo
			// coincidenti `FacingAfterDisplacement` lascia l'orientamento invariato, che e' esattamente la
			// regola qui — non c'e' nessuno verso cui girarsi. E' il precedente gia' pinnato da
			// `Facing.EnvironmentalDisplacementKeepsFacing` (scivolare sul ghiaccio non ruota). [D-104] vale
			// per la fuga da un ATTACCANTE, che ha una minaccia da tenere davanti.
			ApplyForcedDisplacement(Fleeing, Dest[f], Dest[f], FleeCause, TEXT("Fuga"), Map,
				ERTMatchPhase::Cleanup);
		}
	}

	for (const FRTCellId& Cell : BornSurfaceCells)
	{
		for (ARTUnit* Occupant : Units)
		{
			// I morti no: una scarica elettrica di questo stesso Cleanup puo' averli appena eliminati, e
			// bruciare un caduto scriverebbe una riga di combat log per un evento che non esiste.
			//
			// E chi e' FUGGITO nemmeno: `Occupant->Cell` e' gia' quella nuova, quindi non combacia piu' con la
			// cella in fiamme. Non serve una lista di esclusi — la posizione aggiornata basta, ed e' anche il
			// motivo per cui le fughe si applicano prima di questo ciclo e non dopo.
			if (Occupant && Occupant->IsAlive() && Occupant->Cell == Cell)
			{
				ApplyTerrainOnEnterEffects(Map, Occupant, { Cell }, ERTMatchPhase::Cleanup);
			}
		}
	}
}

void ARTTurnManager::ConcludeTurn()
{
	// CP 11.3 (#79): il log leggibile si DERIVA dal TurnLog, che e' l'autorita'. Qui e non in
	// `LockInAndResolve`, perche' quella esce da due rami — playback o conclusione immediata — e solo questo
	// punto li attraversa entrambi: emettere di la' significherebbe nessuna riga nelle partite con playback.
	//
	// ⚠️ Prima le righe nascevano da 59 `AddLogEvent` sparse nella risoluzione, e il TurnLog nasceva altrove:
	// due produttori indipendenti coincidono per abitudine, non per costruzione. Misurato su una partita di
	// dodici turni: zero righe su cosa il bot avesse deciso, zero su chi avesse colpito chi.
	//
	// 🔴 Con il SOGGETTO, non solo col testo. Questa derivazione e' il canale PRIMARIO del combat log: senza
	// soggetto ogni riga arrivava qui come «riga di mondo» (`INDEX_NONE`) e passava il filtro di conoscenza
	// sempre — comprese le voci `Move`, che `DescribeEntry` stampa con `SrcCell` **e** `TgtCell` di un
	// nemico che la squadra puo' non vedere. Convertire i siti sparsi senza convertire questo lasciava il
	// canale piu' grosso scoperto.
	for (const FRTDescribedLine& Line : URTTurnLogLibrary::DescribeTurnLogWithSubjects(TurnLog))
	{
		// Il verdetto viene dalla voce, che lo ha congelato nella fase in cui il fatto e' accaduto.
		// Ricalcolarlo qui userebbe la conoscenza del Blast e le celle post-Move: due istanti diversi.
		AddLogEvent(Line.Text, FRTLogSubject::Frozen(Line.SubjectStableUnitId, Line.Verdict));
	}
	// PRIMA di tutto il resto: a partita finita questa funzione esce anticipatamente, e il turno che
	// decide la partita e' proprio quello che non verrebbe mai misurato.
	ClosePacingSample();

	// La traccia del turno appena risolto va su disco ADESSO, non a fine partita: un archivio serve
	// soprattutto quando la partita non arriva alla fine. `TurnNumber` e' ancora quello del turno chiuso —
	// l'incremento avviene piu' sotto, e invertire i due ordini scriverebbe ogni traccia col numero
	// sbagliato.
	RecordTurnToReplay();

	// ⚠️ Il checksum di fine partita si cattura QUI, **prima** che le unita' morte vengano distrutte: dopo,
	// una caduta non esisterebbe piu' e `bAlive = false` non comparirebbe mai in una partita vera — due
	// finali diversi per chi e' rimasto in piedi darebbero lo stesso hash. E' la decisione presa con
	// [D-084], insieme all'identita' che il digest usa.
	CaptureFinalStateHash();

	// Morte visiva differita: ora che il playback ha mostrato le eliminazioni, rimuovi gli Actor morti
	// (prima del prossimo turno, cosi' non figurano piu' come bersagli/ostacoli).
	DestroyDefeatedUnits();

	if (PendingResult.Outcome != ERTMatchOutcome::InProgress)
	{
		Phase = ERTMatchPhase::MatchEnded;

		// Esito E via: "vince il team 0" e' la stessa frase per un'eliminazione e per un punto di vantaggio
		// allo scadere dei round, e senza la via il log non permetterebbe di distinguerle (DoD di CP 10.3).
		AddLogEvent(FString::Printf(TEXT("Partita finita: %s - %s (round %d/%d, obiettivo %d-%d, formato %s)"),
			*URTTurnRules::DescribeOutcome(PendingResult.Outcome),
			*URTTurnRules::DescribeEndReason(PendingResult.Reason),
			TurnNumber,
			MatchRules.RoundLimit,
			Team0Score,
			Team1Score,
			*MatchRules.FormatId.ToString()), FRTLogSubject::World());

		// La partita e' finita: l'archivio si chiude, e da qui in poi dichiara di essere completo. Se
		// questa riga non venisse mai eseguita — crash, uscita — il manifest resterebbe non chiuso, ed e'
		// esattamente cosi' che un archivio parziale si riconosce.
		CloseReplayArchive();

		// 🔴 **L'annuncio di fine partita, ed e' l'unica cosa che collega il turno alla schermata di
		// Result.** Senza, `URTFrontendNavigator::ShowResult` restava senza chiamanti: esisteva, era
		// testata, e a fine partita non la invocava nessuno — il DoD di `#939` chiede «dopo la partita si
		// arriva a CP 46.5, non al desktop».
		//
		// ⚠️ **Qui la simulazione non conosce il frontend**, e non deve: annuncia il verdetto che ha gia'
		// dato e chi ascolta decide. `ARTGameMode` apre il Result; uno scenario headless non ascolta, e
		// per lui non cambia niente.
		//
		// ⚠️ **Dopo `CloseReplayArchive`**: chi ascolta puo' voler leggere la traccia, e un manifest
		// ancora aperto la descriverebbe come una partita in corso.
		OnMatchEnded.Broadcast(PendingResult, PendingState);

		return; // niente nuovo turno
	}

	++TurnNumber;

	// Riavvia la pianificazione del nuovo turno.
	StartPlanningTimer();
}

int32 ARTTurnManager::GetTeamScore(int32 TeamId) const
{
	if (TeamId == 0) { return Team0Score; }
	if (TeamId == 1) { return Team1Score; }
	return 0;
}

void ARTTurnManager::AddTeamScore(int32 TeamId, int32 Points)
{
	if (TeamId == 0)
	{
		Team0Score += Points;
	}
	else if (TeamId == 1)
	{
		Team1Score += Points;
	}
	else
	{
		// Il 2v2 ha due squadre: un punto assegnato a una terza sparirebbe senza che nessuno lo noti, e
		// l'esito della partita dipende da questi numeri.
		UE_LOG(LogRT, Warning, TEXT("[RT] Punteggio %d assegnato alla squadra %d, che non esiste: ignorato"),
			Points, TeamId);
		return;
	}

	AddLogEvent(FString::Printf(TEXT("Obiettivo: team %d +%d (ora %d-%d)"),
		TeamId, Points, Team0Score, Team1Score), FRTLogSubject::World());
}

void ARTTurnManager::CaptureFinalStateHash()
{
	if (!bRecordReplay || !ReplayManifest.MatchId.IsValid() || ReplayManifest.bClosed)
	{
		return;
	}

	// Si cattura a ogni turno e non solo all'ultimo: l'ultimo non si sa quale sia finche' non e' passato, e
	// un turno che chiude la partita per eliminazione lo scopre solo dopo aver risolto. In cambio il valore
	// c'e' sempre, anche quando la partita finisce in un ramo che non avevamo previsto.
	//
	// ⚠️ **Il KPI di pacing non misura questo costo**, ed e' bene saperlo prima che diventi un problema:
	// `Perf.TurnResolverMedian` spawna un TurnManager senza chiamare `BeginReplayRecording`, quindi la
	// guardia qui sopra manda la funzione a vuoto e il tempo per turno che quel test pubblica **esclude**
	// l'hash. Oggi la spesa e' trascurabile — un hash su mappa e unita' di una partita 2v2 — ma se la mappa
	// crescesse, la rete che dovrebbe accorgersene non copre questo percorso.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	Units.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}

	FVector Origin; float CellSize = 0.f; float LayerH = 0.f;
	const URTHexMapAsset* Map = GetHexContext(Origin, CellSize, LayerH);

	TArray<int32> TeamScores;
	TeamScores.Add(GetTeamScore(0));
	TeamScores.Add(GetTeamScore(1));

	PendingFinalStateHash = static_cast<int64>(URTMatchStateHashLibrary::HashMatchState(
		Map, URTMatchStateHashLibrary::BuildUnitDigests(Units), TeamScores));
}

void ARTTurnManager::BeginReplayRecording()
{
	if (!bRecordReplay)
	{
		return;
	}

	// Senza un formato risolto non si registra. `SetupHexMatch` esce anticipatamente quando
	// `ApplyMatchFormat` fallisce — la mappa resta a schermo col motivo nel log, e nessuna partita viene
	// allestita — ma il chiamante e' fuori da quella funzione e non lo sa. Un archivio che dichiara
	// `FormatId = None` non e' confrontabile con niente (`CompareSerializedTraces` distingue proprio il
	// `FormatMismatch`), e sarebbe la registrazione di una partita che non e' mai cominciata.
	if (MatchRules.FormatId.IsNone())
	{
		return;
	}

	ReplayManifest = FRTReplayManifest();
	ReplayManifest.MatchId = FGuid::NewGuid();
	// Il formato si legge ADESSO e non a `BeginPlay`: il GameMode spawna il TurnManager prima di risolvere
	// il formato di partita (`ApplyMatchFormat`), quindi a quel punto `MatchRules.FormatId` non e' ancora
	// quello vero. Un manifest che dichiara il formato sbagliato e' peggio di uno che non lo dichiara.
	ReplayManifest.FormatId = MatchRules.FormatId;
	ReplayManifest.bHexTopology = true; // un solo substrato: `FRTCellId` e' esagonale (ADR-0002)

	// L'UNICO tempo reale che tocca l'archivio: da qui esce la durata nel manifest e la data nell'indice.
	// Nessuno dei due entra in un hash.
	ReplayStartRealSeconds = FPlatformTime::Seconds();
	ReplayStartedUtc = FDateTime::UtcNow();

	// La partita entra nella lista ADESSO e non alla fine (`#416`): se entrasse alla fine, una partita
	// interrotta non comparirebbe da nessuna parte pur avendo lasciato un archivio riproducibile su disco.
	// La riga si completa alla chiusura — `AppendOrUpdate` aggiorna la stessa, non ne accoda una seconda.
	URTMatchHistoryLibrary::AppendOrUpdate(ResolveReplaysRoot(),
		URTMatchHistoryLibrary::EntryFromManifest(ReplayManifest, ReplayStartedUtc));
}

FString ARTTurnManager::ResolveReplaysRoot() const
{
	// 🔴 **Il default lo CHIEDE, non lo ricostruisce** (`#1050`). Fino al 2026-08-16 questa funzione
	// ripeteva qui `FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Replays"))`, e la stessa espressione
	// viveva in altri due punti: chi avesse spostato gli archivi ne avrebbe cambiato uno, e il lettore
	// avrebbe elencato una cartella vuota su una macchina piena di registrazioni — indistinguibile da
	// «non hai ancora giocato». Ne' il compilatore ne' un test se ne accorgono: sono funzioni corrette che
	// rispondono alla stessa domanda.
	//
	// ⚠️ Il posto e' il **produttore**: chi scrive possiede la disposizione su disco — cartella per partita,
	// manifest, una traccia per turno — e la radice ne e' il primo livello. Chi legge la chiede.
	// La terza copia era in `URTReplayViewerSubsystem`, tolta con `#999`/#1005; questa e' rimasta indietro
	// perche' allora `RTTurnManager.cpp` non era nel `writable` di nessuna track (`D-139`).
	return ReplaysRootOverride.IsEmpty()
		? URTReplayRecorderLibrary::DefaultReplaysRoot()
		: ReplaysRootOverride;
}

void ARTTurnManager::RecordTurnToReplay()
{
	if (!bRecordReplay || !ReplayManifest.MatchId.IsValid())
	{
		return;
	}

	// Il TurnManager non sa scrivere: consegna la traccia a chi lo fa. Non riordina e non serializza —
	// `SortTurnLog` ha gia' fissato l'ordine canonico, e la serializzazione e' della libreria del TurnLog.
	if (!URTReplayRecorderLibrary::RecordTurn(ResolveReplaysRoot(), ReplayManifest, TurnNumber, TurnLog))
	{
		// Non e' un errore di gioco: la partita continua anche se il disco no. Ma va DETTO, o un archivio
		// che non c'e' si scopre solo quando qualcuno prova a riaprirlo.
		AddLogEvent(FString::Printf(TEXT("Replay: il turno %d non e' stato registrato"), TurnNumber), FRTLogSubject::World());
	}
}

void ARTTurnManager::CloseReplayArchive()
{
	if (!bRecordReplay || !ReplayManifest.MatchId.IsValid() || ReplayManifest.bClosed)
	{
		return;
	}

	// Il checksum e' quello catturato in `CaptureFinalStateHash`, PRIMA che le unita' morte sparissero: qui
	// non si puo' piu' calcolare, perche' `DestroyDefeatedUnits` e' gia' passato.
	// La DURATA si misura adesso: nasce in `BeginReplayRecording` e finisce qui. E' l'unico tempo reale che
	// l'archivio porta, e vive in un campo che non entra in nessun hash.
	const float WallClock = static_cast<float>(FPlatformTime::Seconds() - ReplayStartRealSeconds);
	if (!URTReplayRecorderLibrary::CloseMatch(ResolveReplaysRoot(), ReplayManifest,
		PendingResult.Outcome, PendingFinalStateHash, WallClock))
	{
		AddLogEvent(TEXT("Replay: l'archivio non e' stato chiuso"), FRTLogSubject::World());
	}

	// La riga della lista si completa con quello che il manifest dice ADESSO: esito, turni, durata e la
	// disponibilita' del replay, che e' la chiusura stessa. Se la chiusura e' fallita, `bClosed` e' rimasto
	// `false` e l'indice lo riporta — la lista non promette una partita intera che il disco non ha.
	URTMatchHistoryLibrary::AppendOrUpdate(ResolveReplaysRoot(),
		URTMatchHistoryLibrary::EntryFromManifest(ReplayManifest, ReplayStartedUtc));
}

void ARTTurnManager::DestroyDefeatedUnits()
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (Unit && !Unit->IsAlive())
		{
			Unit->Destroy();
		}
	}
}

int32 ARTTurnManager::ResolveCoverStructures(const TArray<ARTUnit*>& Units)
{
	ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* Map = MapActor ? MapActor->MapAsset : nullptr;

	// Che cosa si e' chiesto alla mappa, gia' validato contro il PIANO (bordo dichiarato, bersaglio, portata).
	// Si raccoglie tutto prima di scrivere, come per `PendingArcOps` nel Blast: due pannelli sullo stesso bordo
	// nello stesso turno non possono coesistere, e chi vince deve dipendere da un ordine stabile e non
	// dall'ordine in cui `GetAllActorsOfClass` ha restituito gli attori.
	struct FRTPendingCoverOp
	{
		FRTCellId Cell;
		ERTHexDirection Edge = ERTHexDirection::E;
		int32 Integrity = 0;
		int32 Turns = 0;
		int32 FreeRotations = 0;
		FName ActionId;
		/**
		 * Chi ha chiesto la copertura (#405). Viaggia CON l'operazione e non si ricava dopo: `Pending` viene
		 * riordinato per cella/bordo, quindi l'indice qui non e' piu' quello dell'unita' che l'ha richiesta.
		 */
		ARTUnit* Actor = nullptr;
	};
	TArray<FRTPendingCoverOp> Pending;

	// Gli SPOSTAMENTI (`Riktor.Reconfigure`). Portano con se' chi li ha chiesti, perche' il cooldown si decide
	// solo in applicazione: una rotazione gratuita non lo spende, e quale copertura si sposta — quindi se una
	// rotazione gratuita c'e' — si sa solo guardando il campo.
	struct FRTPendingMoveOp
	{
		FRTCellId Cell;
		ERTHexDirection ToEdge = ERTHexDirection::E;
		FName ActionId;
		ARTUnit* Mover = nullptr;
		int32 AbilityIndex = INDEX_NONE;
	};
	TArray<FRTPendingMoveOp> Moves;

	// Le voci di rifiuto decise gia' in raccolta: escono in coda agli esiti, cosi' il replay racconta prima
	// cosa e' successo al campo e poi cosa non e' successo a chi ci ha provato.
	//
	// La voce viaggia CON il suo attore (#405): sono emesse in blocco molto piu' tardi, e a quel punto «chi ci
	// ha provato» non e' piu' ricostruibile — la cella nella voce e' quella del bordo, non dell'unita'.
	struct FRTCoverRejection
	{
		FRTTurnLogEntry Entry;
		const ARTUnit* Actor = nullptr;
	};
	TArray<FRTCoverRejection> Rejections;

	auto Reject = [this, &Rejections](const FRTCellId& From, const FRTCellId& Toward, const FName& ActionId,
		const TCHAR* Why, const ARTUnit* Who)
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Prep;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected);
		Entry.ActionId = ActionId;
		Entry.SrcCell = From;
		Entry.TgtCell = Toward;
		Entry.Amount = 0;
		Rejections.Add({ Entry, Who });
		AddLogEvent(FString::Printf(TEXT("%s: %s annullata (%s)"),
			Who ? *Who->GetName() : TEXT("?"), *ActionId.ToString(), Why), FRTLogSubject::Unit(Who));
	};

	for (ARTUnit* Unit : Units) // gia' ordinati per cella dal chiamante
	{
		if (!Unit || !Unit->IsAlive()) { continue; }

		const int32 Index = Unit->PlannedAbilityIndex;
		const URTActionData* Ability = Unit->GetAbility(Index);
		if (!Ability || Ability->Def.StructureOp == ERTStructureOp::None)
		{
			continue; // non tocca le strutture di bordo: la vede il motore azioni, o nessuno
		}

		// 🔴 **Le PORTE non passano da qui** ([D-148]). Questo loop e' delle COPERTURE: piu' sotto tratta
		// `MoveCover` in un ramo proprio e manda **tutto il resto** al ramo che erige una copertura dal
		// catalogo terreni. Senza questa riga `Action.Interact` — che dichiara `StructureOp` per farsi
		// puntare su un bordo — costruirebbe un muro invece di aprire una porta, **e la compilazione non lo
		// direbbe**: nessuno `switch` su questo enum e' esaustivo.
		//
		// Il percorso vero e' un altro e resta intatto: `RTHexCombatLibrary` raccoglie l'operazione su
		// `FirstDoorEdge` durante il Blast e la applica `URTHexDoorLibrary::SetDoorState`, che e' l'unico
		// ingresso di mutazione. Qui non si consuma nemmeno l'ability: la consuma quel percorso.
		if (Ability->Def.StructureOp == ERTStructureOp::SetDoorState)
		{
			continue;
		}

		// Il piano si consuma nel turno, attivata o no: e' la stessa disciplina di `ModifyArc` nel Blast.
		const FRTActionDef Def = Ability->Def;
		const bool bHasEdge = Unit->bHasPlannedCoverEdge;
		const ERTHexDirection Edge = Unit->PlannedCoverEdge;
		const bool bTargetsCell = Unit->bAttackTargetsCell;
		const FRTCellId TargetCell = bTargetsCell ? Unit->PlannedAttackCell
			: (Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget->Cell : Unit->Cell);
		const bool bHasTarget = bTargetsCell || Unit->PlannedAttackTarget != nullptr;

		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedAttackTarget = nullptr;
		Unit->bHasPlannedCoverEdge = false;
		if (!Unit->CanUseAbility(Index)) { continue; }

		if (!bHasTarget)
		{
			// Nessuna scelta implicita: il catalogo la vieta, e «la propria cella» sarebbe una scelta implicita
			// per quanto comoda. Chi vuole ripararsi davanti dichiara la propria cella.
			Reject(Unit->Cell, Unit->Cell, Def.ActionId, TEXT("nessun bersaglio"), Unit);
			continue;
		}
		if (!bHasEdge)
		{
			Reject(Unit->Cell, TargetCell, Def.ActionId, TEXT("nessun bordo dichiarato"), Unit);
			continue;
		}
		// La portata si valida QUI, prima di toccare la mappa. `ModifyArc` non lo fa e la issue #206 lo
		// registra come difetto: un'azione che dichiara `Range 3` e opera a dieci celle non e' un'azione a
		// portata, e' un'azione senza portata.
		if (URTHexLibrary::HexDistance(Unit->Cell, TargetCell) > Def.RangeCells)
		{
			Reject(Unit->Cell, TargetCell, Def.ActionId, TEXT("fuori portata"), Unit);
			continue;
		}

		// Lo SPOSTAMENTO non consuma qui: la rotazione gratuita del pannello adattivo si scopre solo guardando
		// quale copertura verra' mossa, e quello succede in applicazione.
		if (Def.StructureOp == ERTStructureOp::MoveCover)
		{
			Moves.Add({ TargetCell, Edge, Def.ActionId, Unit, Index });
			continue;
		}

		Unit->ConsumeAbility(Index);

		// Integrita' e durata del catalogo terreni (`Structure.KineticPanel`: 30 punti struttura) e del
		// catalogo azioni (2 turni, come ogni altra modifica temporanea del campo).
		int32 Integrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low);
		int32 Turns = 2;
		int32 FreeRotations = 0;

		// La VARIANTE attiva sostituisce i due numeri, se li dichiara. E' il primo punto in cui i `Parameters`
		// di una variante decidono qualcosa: il pannello rinforzato compra integrita' con la durata (45 per un
		// turno solo), l'adattivo compra flessibilita' con l'integrita' (25, e `DurationTurns` 0 — non scade
		// da sola). Non c'e' un ramo per eroe: si legge il dato di quell'abilita', qualunque sia.
		if (const FRTAbilityVariant* Variant = Ability->FindVariant(Unit->ActiveVariantId))
		{
			if (const int32* Declared = Variant->Parameters.Find(TEXT("Integrity")))
			{
				Integrity = *Declared;
			}
			if (const int32* Declared = Variant->Parameters.Find(TEXT("DurationTurns")))
			{
				Turns = *Declared;
			}
			if (const int32* Declared = Variant->Parameters.Find(TEXT("FreeRotations")))
			{
				FreeRotations = *Declared;
			}
		}

		Pending.Add({ TargetCell, Edge, Integrity, Turns, FreeRotations, Def.ActionId, Unit });
	}

	if (Pending.Num() == 0 && Moves.Num() == 0 && Rejections.Num() == 0)
	{
		return 0;
	}

	Pending.Sort([](const FRTPendingCoverOp& A, const FRTPendingCoverOp& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
	});

	int32 Applied = 0;
	for (const FRTPendingCoverOp& Op : Pending)
	{
		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Op.Cell);
		const int32 EdgeIndex = static_cast<int32>(Op.Edge);
		const FRTCellId Toward = Ring.IsValidIndex(EdgeIndex) ? Ring[EdgeIndex] : Op.Cell;

		if (!URTHexCoverLibrary::AddCover(Map, Op.Cell, Op.Edge, ERTHexCoverType::Low, Op.Integrity))
		{
			// Bordo gia' riparato o cella fuori mappa. Il cooldown resta speso: l'azione e' stata usata, ed e'
			// il `Cancel` del catalogo — non un'azione che non e' mai partita.
			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Prep;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected);
			Entry.ActionId = Op.ActionId;
			Entry.SrcCell = Op.Cell;
			Entry.TgtCell = Toward;
			Entry.Amount = 0;
			Rejections.Add({ Entry, Op.Actor });
			AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): il bordo e' gia' riparato"),
				Op.Cell.X, Op.Cell.Y, Op.Cell.Layer), FRTLogSubject::World());
			continue;
		}

		// Entra nella lista anche se non scade (`Turns` 0, pannello adattivo): la lista dice quali coperture
		// sono «di partita», e serve a `Reconfigure` per portarsi dietro le rotazioni gratuite. E' il tick a
		// saltare le permanenti, non l'inserimento a escluderle.
		DynamicCovers.Add({ Op.Cell, Op.Edge, Op.Turns, Op.FreeRotations });

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Prep;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated);
		Entry.ActionId = Op.ActionId;
		Entry.SrcCell = Op.Cell;
		Entry.TgtCell = Toward;
		Entry.Amount = Op.Turns;
		AppendLogEntry(Entry, Op.Actor);
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): copertura eretta (%d turni)"),
			Op.Cell.X, Op.Cell.Y, Op.Cell.Layer, Op.Turns), FRTLogSubject::World());
		++Applied;
	}

	// Gli SPOSTAMENTI dopo le creazioni: agiscono su un campo gia' aggiornato, e l'ordine fra i due gruppi e'
	// dichiarato invece che emergente dall'ordine degli attori.
	Moves.Sort([](const FRTPendingMoveOp& A, const FRTPendingMoveOp& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.ToEdge) < static_cast<uint8>(B.ToEdge);
	});

	for (const FRTPendingMoveOp& Move : Moves)
	{
		ARTUnit* Mover = Move.Mover;
		auto RejectMove = [&](const TCHAR* Why)
		{
			// Un tentativo fallito spende comunque l'azione: e' il `Cancel` del catalogo, non un turno gratis.
			if (Mover) { Mover->ConsumeAbility(Move.AbilityIndex); }
			Reject(Move.Cell, Move.Cell, Move.ActionId, Why, Mover);
		};

		const FRTHexCellData* Source = Map ? Map->FindCell(Move.Cell) : nullptr;
		if (Source == nullptr)
		{
			RejectMove(TEXT("cella fuori mappa"));
			continue;
		}

		// QUALE copertura si sposta: quella della cella indicata. Se ce n'e' piu' d'una la scelta sarebbe
		// implicita — «la prima dell'array» e' un ordine che il giocatore non vede — quindi si rifiuta. Un
		// rifiuto leggibile e' meglio di una regola indovinabile solo leggendo il codice.
		int32 Found = 0;
		ERTHexDirection FromEdge = ERTHexDirection::E;
		for (const FRTHexCover& Cover : Source->Covers)
		{
			if (Cover.Type == ERTHexCoverType::None) { continue; }
			++Found;
			FromEdge = Cover.Edge;
		}
		if (Found == 0)
		{
			RejectMove(TEXT("nessuna copertura da spostare"));
			continue;
		}
		if (Found > 1)
		{
			RejectMove(TEXT("piu' coperture sulla cella: quale spostare non e' dichiarabile"));
			continue;
		}
		if (FromEdge == Move.ToEdge)
		{
			RejectMove(TEXT("e' gia' su quel bordo"));
			continue;
		}

		const FRTHexCover* Entry = Source->CoverEntryOn(FromEdge);
		const ERTHexCoverType MovedType = Entry ? Entry->Type : ERTHexCoverType::Low;
		const int32 MovedIntegrity = Entry ? Entry->Integrity : FRTHexCover::DefaultIntegrity(MovedType);

		// Si toglie e si rimette: se la destinazione rifiuta (bordo gia' riparato), si RIMETTE dov'era. Una
		// copertura che sparisce perche' lo spostamento non era possibile sarebbe la peggiore delle due.
		URTHexCoverLibrary::RemoveCover(Map, Move.Cell, FromEdge);
		if (!URTHexCoverLibrary::AddCover(Map, Move.Cell, Move.ToEdge, MovedType, MovedIntegrity))
		{
			URTHexCoverLibrary::AddCover(Map, Move.Cell, FromEdge, MovedType, MovedIntegrity);
			RejectMove(TEXT("il bordo di destinazione e' gia' riparato"));
			continue;
		}

		// La voce di partita segue la copertura: durata residua conservata (spostare non ringiovanisce un
		// pannello) e una rotazione gratuita in meno, se ne aveva.
		bool bWasFree = false;
		for (FRTDynamicCover& Tracked : DynamicCovers)
		{
			if (Tracked.Cell == Move.Cell && Tracked.Edge == FromEdge)
			{
				Tracked.Edge = Move.ToEdge;
				if (Tracked.FreeRotations > 0)
				{
					--Tracked.FreeRotations;
					bWasFree = true;
				}
				break;
			}
		}

		// La rotazione gratuita non spende il cooldown: e' cio' che il pannello adattivo compra con i 5 punti
		// struttura in meno. Le successive si pagano come tutte le altre.
		if (!bWasFree && Mover)
		{
			Mover->ConsumeAbility(Move.AbilityIndex);
		}

		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Move.Cell);
		const int32 ToIndex = static_cast<int32>(Move.ToEdge);
		const FRTCellId Toward = Ring.IsValidIndex(ToIndex) ? Ring[ToIndex] : Move.Cell;

		FRTTurnLogEntry Entry2;
		Entry2.Phase = ERTMatchPhase::Prep;
		Entry2.Category = ERTLogCategory::Environment;
		Entry2.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverMoved);
		Entry2.ActionId = Move.ActionId;
		Entry2.SrcCell = Move.Cell;
		Entry2.TgtCell = Toward;
		Entry2.Amount = MovedIntegrity; // l'integrita' viaggia con la copertura: spostarla non la ripara
		AppendLogEntry(Entry2, Mover); // chi ha spostato la copertura
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): copertura spostata%s"),
			Move.Cell.X, Move.Cell.Y, Move.Cell.Layer, bWasFree ? TEXT(" (rotazione gratuita)") : TEXT("")), FRTLogSubject::World());
		// Anche uno SPOSTAMENTO e' successo qualcosa. Contando solo le erezioni, un turno in cui l'unico
		// evento e' una `Reconfigure` non accendeva il beat di Prep nel playback: il TurnLog lo registrava e
		// lo spettatore non vedeva niente. E' la presentazione a tacere, non la regola — ma tacere resta il
		// difetto che questo checkpoint dice di non voler commettere.
		++Applied;
	}

	// In blocco, ma una per una: `Append` bypasserebbe il contesto della v6, ed e' la seconda porta
	// d'ingresso al TurnLog che l'helper deve presidiare quanto la prima.
	for (FRTCoverRejection& Rejected : Rejections) { AppendLogEntry(Rejected.Entry, Rejected.Actor); }
	return Applied;
}

void ARTTurnManager::ResolvePrep()
{
	// Prima fase che passa dal MOTORE AZIONI (epic E4): raccogli -> ordina -> traduci in EVENTI -> applica.
	// Questo orchestratore non sa piu' che cosa faccia un'abilita' di supporto: applica un evento `Shield`.
	// Aggiungere un'azione di Prep con un altro effetto (una cura, uno stato) non richiede di toccarlo.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}
	Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	// 0. Le strutture di BORDO (CP 9.5) prima del motore azioni, e fuori da esso: il loro esito e' una modifica
	// della mappa, non un evento verso un'unita'. Passando dalla raccolta, un'azione senza `Effects`
	// ripiegherebbe sul campo legacy `Power` e un pannello si metterebbe a fare danno — e' lo stesso motivo per
	// cui `ModifyArc` si intercetta prima degli intenti nel Blast.
	if (ResolveCoverStructures(Units) > 0)
	{
		bPrepActiveThisTurn = true; // c'e' un beat di Prep da mostrare nel playback
	}

	// 1. RACCOGLI: un'istanza per ogni azione di Prep pianificata e utilizzabile.
	TArray<FRTActionInstance> Instances;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		if (!Unit->IsAlive()) { continue; }

		const int32 Index = Unit->PlannedAbilityIndex;
		const URTActionData* Ability = Unit->GetAbility(Index);
		if (!Ability || !Unit->CanUseAbility(Index)) { continue; }
		if (URTCatalogLibrary::MapResolutionPhase(Ability->Def.ResolutionPhase) != ERTMatchPhase::Prep) { continue; }

		// PREDITTIVA (E18 CP 18.2): si ARMA qui e risolve al boundary del Move, quindi NON entra fra le
		// istanze che producono eventi adesso. Tenercela dentro le farebbe tradurre il proprio `Damage` in un
		// evento verso se stessa — oggi innocuo perche' la Prep ignora il danno, ma sarebbe un difetto latente
		// in attesa che qualcuno aggiunga quel `case`. Meglio che l'esclusione sia dichiarata.
		if (Ability->Def.PredictiveTargeting != ERTPredictiveTargeting::None)
		{
			// Senza una cella dichiarata non c'e' previsione: il piano e' incompleto e l'azione non si arma.
			// Non e' un errore da segnalare qui — la validazione del piano sta a monte — ma non si finge
			// nemmeno che una previsione senza bersaglio sia stata fatta.
			if (Unit->bAttackTargetsCell)
			{
				FRTArmedPrediction Armed;
				Armed.Shooter = Unit;
				Armed.LockedCell = Unit->PlannedAttackCell;
				Armed.ActionId = Ability->Def.ActionId;
				Armed.BaseActionId = Ability->Def.BaseActionId;
				// Il danno viene dagli EFFECTS del catalogo, non da un numero scritto qui: cambiare quanto
				// fa `InterceptShot` deve restare una modifica ai dati.
				for (const FRTActionEffectSpec& Effect : Ability->Def.Effects)
				{
					if (Effect.Effect == ERTActionEffect::Damage) { Armed.Damage += Effect.Amount; }
				}
				ArmedPredictions.Add(Armed);
				bPrepActiveThisTurn = true; // armare e' un beat di Prep osservabile
			}

			// L'abilita' e' comunque SPESA: chi ha scommesso ha pagato il cooldown, che la previsione sia
			// giusta o no. E' la meta' del costo che rende il whiff una scelta e non un tentativo gratuito.
			Unit->ConsumeAbility(Index);
			Unit->PlannedAbilityIndex = INDEX_NONE;
			Unit->PlannedAttackTarget = nullptr;
			continue;
		}

		// OVERWATCH (CP 14.5): si ARMA qui e reagisce ai micro-step del Move, quindi — come la predittiva —
		// non entra fra le istanze che producono eventi adesso. La ragione e' anche piu' semplice: non ha
		// `Effects` da tradurre, perche' cosa scatta lo dice il PROFILO e non l'azione.
		//
		// ⚠️ Il confronto e' sull'`ActionId`, non su un campo del `Def`, e va detto perche' il repository
		// preferisce i discriminanti di dato: qui e' legittimo perche' `Action.Overwatch` e' un'azione
		// GENERICA e universale, non l'abilita' di un eroe. E' lo stesso trattamento che il resolver riserva
		// gia' a `Action.Cleanse`, `Action.Heal`, `Action.Interrupt` e `Action.ModifyArc`. La regola che
		// `AGENTS.md`/`CLAUDE.md` pongono vieta i branch **per eroe**, ed e' un'altra cosa: quella nasce
		// perche' il roster cresce, mentre le generiche sono sette e chiuse da D-025.
		if (Ability->Def.ActionId == FName(TEXT("Action.Overwatch")))
		{
			FRTArmedOverwatch Armed;
			Armed.Owner = Unit;
			Armed.Facing = Unit->Facing; // il cono E' il facing (ADR-0005 §4c), dichiarato in Planning
			Armed.ActionId = Ability->Def.ActionId;
			Armed.BaseActionId = Ability->Def.BaseActionId;

			// La condizione dichiarata ([D-109]). ⚠️ **Limite dichiarato**: il suo unico produttore in
			// partita — `rt.Reaction.Condition` — passa da `SetPlannedReactionCondition`, che pretende un
			// `PlannedReactionAbility` armato, cioe' lo **slot reazione**. L'Overwatch costa l'azione
			// PRINCIPALE (catalogo §1: «armare l'Overwatch costa l'azione principale; lo slot reazione
			// preparato e' un'altra cosa»), quindi oggi un piano di solo-Overwatch non riesce a dichiararne
			// una. Il campo si legge lo stesso — e' quello che [D-109] definisce, e `BuildOverwatchTriggers`
			// gia' lo consuma — ma finche' i due slot non sono riconciliati resta vuoto nel caso tipico.
			// E' la meta' di `#583` che questo checkpoint sblocca senza chiudere.
			Armed.Condition = Unit->PlannedReactionCondition;

			// Portata e danno vengono dall'ARMA dell'eroe. L'attacco base si trova per `BaseActionId` e non
			// per indice: «l'attacco base e' l'indice 0 del kit» e' vero oggi ma e' una convenzione, mentre
			// `BaseActionId` e' una dichiarazione che il roster deve rispettare — e
			// `Heroes.BasicAttackDeclaresItsBaseAction` la fa valere. Se un giorno l'indice 0 cambiasse, con
			// la convenzione l'Overwatch avrebbe silenziosamente la portata di un'altra azione.
			for (int32 A = 0; A < Unit->NumAbilities(); ++A)
			{
				const URTActionData* Weapon = Unit->GetAbility(A);
				if (!Weapon || Weapon->Def.BaseActionId != FName(TEXT("Action.BasicAttack"))) { continue; }

				Armed.RangeCells = FMath::Max(1, Weapon->Def.RangeCells);
				for (const FRTActionEffectSpec& Effect : Weapon->Def.Effects)
				{
					if (Effect.Effect == ERTActionEffect::Damage) { Armed.Damage += Effect.Amount; }
				}
				break;
			}

			ArmedOverwatches.Add(Armed);
			bPrepActiveThisTurn = true; // armare e' un beat di Prep osservabile, come la previsione

			// L'azione principale e' SPESA nel momento in cui si arma, non quando si spara: e' il
			// costo-opportunita' di D-012, ed e' cio' che rende la scommessa una scommessa. «Se nessun
			// trigger avviene, l'investimento e' perso» (`brief-azioni-generiche-overwatch.md` §6).
			// Da non confondere con la CHARGE, che `bCharged` tiene e che solo un `FIRE` consuma.
			Unit->ConsumeAbility(Index);
			Unit->PlannedAbilityIndex = INDEX_NONE;
			Unit->PlannedAttackTarget = nullptr;
			continue;
		}

		FRTActionInstance Instance;
		Instance.Def = Ability->Def;
		Instance.SourceUnitId = i;
		Instance.TargetUnitId = i;   // le azioni di Prep del vertical slice agiscono su chi le usa
		Instance.TargetCell = Unit->Cell;
		Instance.EventSequence = Instances.Num();
		Instances.Add(Instance);
	}
	if (Instances.Num() == 0) { return; }

	// 2. ORDINA con la regola unica (priorita' intera intra-fase, tie-break assoluto).
	URTActionQueueLibrary::SortActionInstances(Instances);

	// 3. TRADUCI in eventi e 4. APPLICA: si lavora sugli EVENTI, non sulle abilita'.
	for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEventsForAll(Instances))
	{
		if (!Units.IsValidIndex(Event.TargetUnitId)) { continue; }
		ARTUnit* Target = Units[Event.TargetUnitId];
		switch (Event.Kind)
		{
		case ERTActionEffect::Shield:
			Target->AddTemporaryShield(Event.Amount); // temporaneo: scade nel Cleanup (issue #96)
			AddLogEvent(FString::Printf(TEXT("%s: +%d scudo"), *Target->GetName(), Event.Amount), FRTLogSubject::Unit(Target));
			break;
		case ERTActionEffect::Heal:
			Target->ApplyCombatState(FMath::Min(Target->MaxHealth, Target->Health + Event.Amount), Target->Shield);
			AddLogEvent(FString::Printf(TEXT("%s: +%d salute"), *Target->GetName(), Event.Amount), FRTLogSubject::Unit(Target));
			break;
		case ERTActionEffect::Status:
			ApplyStatusLogged(Target, Event.StatusTag, Event.Amount);
			// #1077: nascita da AZIONE — l'esito la distingue dal terreno.
			// ⚠️ Solo se lo stato e' stato DAVVERO applicato: `ApplyStatus` esce senza toccare niente con
			// `Turns <= 0`, e una voce scritta comunque farebbe ricostruire a un replay uno stato che la
			// simulazione non ha mai concesso. Trovato in code review.
			if (Event.Amount == ARTUnit::PersistentWhileOnCell || Event.Amount > 0)
			{
				FRTTurnLogEntry Nato = MakeStatusBirthEntry(Phase, Event.StatusTag, Target->Cell, Event.Amount,
					/*bFromTerrain=*/ false);
				AppendLogEntry(Nato, Target);
			}
			AddLogEvent(FString::Printf(TEXT("%s: stato applicato"), *Target->GetName()), FRTLogSubject::Unit(Target));
			break;
		default:
			// Danno e spinta non appartengono alla Prep: risolvono nel Blast, dove l'ordine conta insieme
			// agli altri attacchi. Dichiararli qui sarebbe un errore di catalogo, non un caso da gestire.
			break;
		}
		bPrepActiveThisTurn = true; // c'e' un beat di Prep da mostrare nel playback
	}

	// 5. Consuma le abilita' usate e libera i piani.
	for (const FRTActionInstance& Instance : Instances)
	{
		ARTUnit* Unit = Units[Instance.SourceUnitId];
		Unit->ConsumeAbility(Unit->PlannedAbilityIndex);
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumato in Prep
		Unit->PlannedAttackTarget = nullptr;
	}
}

void ARTTurnManager::ResolveDash()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	FVector Origin; float CellSize; float LayerH;
	GetHexContext(Origin, CellSize, LayerH);

	// Stesso strato puro esagonale del movimento normale (CP 6.2): lo scatto e' un movimento con un altro
	// budget, non un secondo sistema di pathfinding. Lo snapshot congela mappa e occupazione a inizio fase.
	TArray<ARTUnit*> Units;
	FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units);

	// Fresco per il turno: chi corre a perdifiato (Action.Sprint) non para (CP 5.1), e questo e' l'unico
	// posto dove si sa CON CERTEZZA cosa ha davvero usato lo slot di scatto.
	ReactionBlockedThisTurn.Reset();

	// Indice (in Units) dell'attaccante per ogni impatto accodato in QUESTO scatto: serve a scartare l'impatto,
	// dopo la risoluzione simultanea, se la collisione ha bloccato il caricatore prima del contatto (CP 4.8).
	TArray<int32> PendingImpactAttackerIdx;

	// Percorsi: uno per ogni unita' (le non-scattanti restano ferme, ma occupano e bloccano come le altre).
	// Priorita' e stile lineare sono per la collisione simultanea (CP 4.8): due mobilita' del catalogo diverse
	// (`Action.Charge` priorita' 35, `Action.Dodge` 30, ecc.) possono coesistere nella STESSA fase Dash.
	TArray<TArray<FRTCellId>> Paths;
	TArray<int32> DashAbilityIdx; // parallelo a Units: INDEX_NONE = non scatta
	TArray<int32> Priorities;     // parallelo a Units: FRTActionDef::Priority dell'azione, 0 per chi non scatta
	TArray<bool> bLinearMovers;   // parallelo a Units: vero se la mobilita' e' lineare (URTMovementActionLibrary::IsLinear)
	TArray<bool> bPassThrough;    // parallelo a Units: vero per chi ATTRAVERSA le unita' ferme (LinearPass)
	Paths.Reserve(Units.Num());
	DashAbilityIdx.Init(INDEX_NONE, Units.Num());
	Priorities.Init(0, Units.Num());
	bLinearMovers.Init(false, Units.Num());
	bPassThrough.Init(false, Units.Num());
	int32 DasherCount = 0;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		Paths.Add({ Unit->Cell }); // default: fermo

		const int32 DashIdx = Unit->PlannedDashAbility;
		Unit->PlannedDashAbility = INDEX_NONE; // consumato per questo turno (valido o no)
		const URTActionData* Dash = Unit->GetAbility(DashIdx);

		// Mobilita' rapida: lo dichiara il CATALOGO (fase FastMovement -> macro-fase Dash) e nient'altro. Il
		// flag legacy `bDash` non esiste piu' (#142): era la seconda risposta alla stessa domanda, e chi
		// leggeva l'una non vedeva le azioni dichiarate solo con l'altra.
		const bool bFastMovement = Dash != nullptr && URTCatalogLibrary::IsFastMovement(Dash->Def);
		if (!bFastMovement || !Unit->CanUseAbility(DashIdx) || Unit->PlannedDashCell == Unit->Cell)
		{
			continue;
		}

		// Budget della fase Dash: la portata dichiarata dall'azione (con gli status), non il movimento del
		// turno. Per un'azione catalogata la verita' e' il `Def`: `Action.Sprint` vale 8 MP e quel numero sta
		// nel catalogo, non sul campo legacy dell'asset.
		const int32 DeclaredRange = Dash->Def.ActionId.IsNone() ? Dash->RangeCells : Dash->Def.RangeCells;
		const int32 EffectiveRange = Unit->GetEffectiveDashRange(DeclaredRange);

		TArray<FRTCellId> Path;
		bool bChargedIntoTarget = false;
		if (URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle))
		{
			// Mobilita' LINEARE (catalogo §2): una direzione fra le sei, e cio' che sta sulla traiettoria la
			// ferma. Non e' il pathfinding del movimento normale — con quello un muro davanti non fermerebbe
			// nulla, lo si girerebbe intorno, e `Dash.BlockedArc` non avrebbe nulla da verificare.
			TSet<int32> Hostiles;
			for (int32 u = 0; u < Units.Num(); ++u)
			{
				if (Units[u] && Units[u]->IsAlive() && Units[u]->TeamId != Unit->TeamId) { Hostiles.Add(u); }
			}

			const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
				Snapshot.Map, Unit->Cell, Unit->PlannedDashCell, EffectiveRange,
				Dash->Def.MovementStyle, Snapshot.Occupancy, Hostiles);

			// L'impatto della carica NON si applica qui: il catalogo le da' codice 20/30, cioe' movimento in
			// fase Dash e impatto fra i controlli, che risolvono per priorita' dentro il Blast.
			if (Linear.Stop == ERTLinearStop::Impact && Units.IsValidIndex(Linear.ImpactUnitId))
			{
				FRTChargeImpact Impact;
				Impact.Attacker = Unit;
				Impact.Target = Units[Linear.ImpactUnitId];
				Impact.Def = Dash->Def;
				PendingChargeImpacts.Add(Impact);
				PendingImpactAttackerIdx.Add(i);
				bChargedIntoTarget = true;
			}

			// Chi e' stato ATTRAVERSATO paga come chi viene urtato: stessa coda, stessa fase, stessi effetti
			// dichiarati dall'azione. Non un secondo percorso di danno — un secondo percorso divergerebbe dal
			// primo alla prima modifica, ed e' esattamente il difetto che la issue #140 ha chiuso altrove.
			for (const int32 CrossedId : Linear.PassedThroughUnitIds)
			{
				if (!Units.IsValidIndex(CrossedId) || !Units[CrossedId] || !Units[CrossedId]->IsAlive())
				{
					continue;
				}
				FRTChargeImpact Crossed;
				Crossed.Attacker = Unit;
				Crossed.Target = Units[CrossedId];
				Crossed.Def = Dash->Def;
				PendingChargeImpacts.Add(Crossed);
				PendingImpactAttackerIdx.Add(i);
			}

			Path.Add(Unit->Cell);
			Path.Append(Linear.Entered);
		}
		else
		{
			Snapshot.Units[i].MoveBudget = EffectiveRange;
			Path = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ i, Unit->PlannedDashCell).Path;
		}

		// Una carica che si ferma subito ha comunque colpito: l'impatto e' gia' registrato qui sopra.
		if (Path.Num() < 2 && !bChargedIntoTarget)
		{
			continue; // destinazione non allineata, fuori portata, bloccata o occupata
		}
		if (Path.Num() < 2)
		{
			Path = { Unit->Cell };
		}
		Paths[i] = Path;
		DashAbilityIdx[i] = DashIdx;
		Priorities[i] = Dash->Def.Priority;
		bLinearMovers[i] = URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle);
		// `ResolveLinearMove` ha gia' deciso CHI viene attraversato, ma il percorso passa ancora dal microstep
		// simultaneo, che ferma chiunque davanti a un'unita' immobile. Senza questo flag la lama si fermerebbe
		// davanti al primo bersaglio nonostante la traiettoria dica il contrario — ed e' esattamente cosi' che
		// il difetto si presentava: libreria corretta, partita no.
		bPassThrough[i] = (Dash->Def.MovementStyle == ERTMovementStyle::LinearPass);
		++DasherCount;
	}

	if (DasherCount == 0) { return; }

	// Scatti simultanei, ordine-indipendenti (stesso resolver a microstep del movimento, con priorita' e
	// scontro frontale fra mobilita' lineari — CP 4.8).
	const TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinearMovers, bPassThrough);

	// Un impatto era stato previsto sul percorso GIA' troncato dal solo `ResolveLinearMove` (occupazione
	// congelata a inizio fase): non sa se la collisione simultanea fermera' il caricatore PRIMA del contatto
	// (due cariche opposte, CP 4.8 — senza questo filtro entrambe infliggerebbero comunque danno, pur non
	// essendosi mai davvero scontrate). Vale solo se il caricatore ha completato il percorso GIA' troncato
	// esattamente com'era: qualunque scarto (priorita' persa o scontro frontale) invalida l'impatto previsto.
	for (int32 k = PendingChargeImpacts.Num() - 1; k >= 0; --k)
	{
		const int32 AttackerIdx = PendingImpactAttackerIdx[k];
		if (!Resolved.IsValidIndex(AttackerIdx) || Resolved[AttackerIdx].Outcome != ERTMoveOutcome::Moved)
		{
			PendingChargeImpacts.RemoveAt(k);
		}
	}

	// Eventi per il playback (Move-type, fase Dash) + traccia post-lock. Catturati PRIMA del placement,
	// osservatori inclusi: le celle da cui si guarda sono quelle di inizio fase ([D-223]).
	const TArray<FRTRouteObserverTeam> ObserverTeams = BuildRouteObserverTeams(Units);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] != INDEX_NONE && Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Route;
			Route.Add(Units[i]->Cell);
			Route.Append(Resolved[i].Entered);

			// L'identita' si prende da `Units[i]`, lo stesso indice da cui la prende `Ev.SourceStableUnitId` due righe
			// sotto. L'indice di `LastMoveRoutes` non la porta: l'`Add` e' condizionale (`#1497`).
			FRTMoveRoute& Tracked = LastMoveRoutes.AddDefaulted_GetRef();
			Tracked.StableUnitId = Units[i]->StableUnitId;
			Tracked.Cells = Route;

			// Il verdetto per cella di [D-223], come nel sito del Move.
			//
			// ⚠️ **Queste rotte non arrivano a schermo**, e il verdetto si calcola lo stesso: `ResolveMovement`
			// gira SEMPRE dopo `ResolveDash` nella stessa risoluzione e apre con `LastMoveRoutes.Reset()`.
			// Una voce senza verdetto qui sarebbe pero' una voce fail-closed pronta a diventare visibile il
			// giorno in cui quell'ordine cambia — cioe' una traccia che sparisce senza che nessuno capisca
			// perche'. Il costo e' una risoluzione di percezione per cella, una volta per turno.
			FreezeRouteVerdicts(Snapshot.Map, ObserverTeams, Units[i]->TeamId, Route, Tracked.CellVerdicts);

			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Dash;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.SourceStableUnitId = Units[i]->StableUnitId;
			Ev.Path = Route;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni e consuma l'abilita'. Lo scatto SPENDE il movimento del turno (D-028): chi ha
	// scattato non prosegue col Move, e questo e' il punto in cui la regola diventa vera — nel resolver, che
	// e' cio' che decide l'esito (invariante #1), non in un controllo di pianificazione che il bot potrebbe
	// aggirare. Prima di D-028 il move normale sopravviveva allo scatto: era il «movimento doppio» di
	// `docs/gameplay/spec-dash.md`, vigente e implementato, ora superato.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] == INDEX_NONE) { continue; }

		ARTUnit* Unit = Units[i];
		const FRTCellId Final = Resolved[i].Final;
		AddLogEvent(FString::Printf(TEXT("Scatto: %s -> (q=%d,r=%d,L%d)"), *Unit->GetName(), Final.X, Final.Y, Final.Layer), FRTLogSubject::Unit(Unit));

		// Lo SCATTO nel TurnLog (#307). Fino a qui la fase Dash non lasciava nessuna voce di movimento: il
		// replay vedeva un'unita' comparire altrove fra un turno e l'altro, e chi leggeva la traccia non
		// poteva distinguere uno scatto da un Move — perche' il Move c'era e lo scatto no.
		//
		// La voce e' la stessa del movimento volontario e si distingue per due campi che ci sono gia':
		// `Phase = Dash` (quando) e `ActionId` (con quale mobilita'). Nessun esito nuovo: `ERTMoveOutcome`
		// descrive gia' cosa e' successo alla rotta, e uno scatto bloccato da una collisione e' bloccato
		// esattamente come un passo.
		if (const URTActionData* DashDef = Unit->GetAbility(DashAbilityIdx[i]))
		{
			FRTTurnLogEntry DashEntry;
			DashEntry.Phase = ERTMatchPhase::Dash;
			DashEntry.Category = ERTLogCategory::Move;
			DashEntry.Outcome = static_cast<uint8>(Resolved[i].Outcome);
			DashEntry.SrcCell = Unit->Cell; // cella di PARTENZA: `Unit->Cell` non e' ancora stata riscritta
			DashEntry.TgtCell = Final;
			DashEntry.Amount = Resolved[i].Entered.Num();
			DashEntry.ActionId = DashDef->Def.ActionId;
			DashEntry.BaseActionId = DashDef->Def.BaseActionId;
			DashEntry.Priority = DashDef->Def.Priority;
			AppendLogEntry(DashEntry, Unit);
		}

		Unit->ConsumeAbility(DashAbilityIdx[i]);

		// `FacingAfterDash` (CP 16.1, D-020): lo scatto orienta, e il Blast che segue leggera' QUESTO valore.
		// Derivato dalla rotta effettiva, prima di spostare l'unita', cosi' la partenza e' ancora quella vera.
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Walked;
			Walked.Reserve(Resolved[i].Entered.Num() + 1);
			Walked.Add(Unit->Cell);
			Walked.Append(Resolved[i].Entered);

			FRTHexSimUnit Dashed(i, Unit->Cell, /*InMoveBudget=*/ 0);
			Dashed.Facing = Unit->Facing;
			RecordFacingChange(Dashed, URTFacingLibrary::FacingFromPath(Walked, Dashed.Facing),
				ERTFacingOutcome::DerivedFromDash, ERTMatchPhase::Dash, Unit);
			Unit->Facing = Dashed.Facing;

			// Traccia per la rotazione dichiarata (#291): dopo uno scatto LINEARE la sola direzione legale e'
			// quella del movimento, e a fine turno non sarebbe piu' deducibile — chi ha scattato arriva al Move
			// con `PlannedCell` uguale alla cella attuale, indistinguibile da chi non si e' mosso.
			if (const URTActionData* DashUsed = Unit->GetAbility(DashAbilityIdx[i]))
			{
				Unit->MovementStyleThisTurn = DashUsed->Def.MovementStyle;
				Unit->WalkedThisTurn = Walked;
			}
		}

		// 🔴 **Il predicato si legge PRIMA della riga qui sotto**, che riscrive `Unit->Cell` con la cella
		// d'arrivo. Letto dopo, «la destinazione pianificata e' diversa dalla posizione attuale» e' vero per
		// OGNI scatto che ha spostato l'unita' — anche per chi non aveva pianificato nessun movimento: il bot
		// parte da `PlannedCell = Cell` con `PlannedPath` vuoto (`PlanBots`), e il giocatore che sceglie una
		// mobilita' e clicca senza posare waypoint sta nello stesso caso. Ogni scatto del gioco dichiarava
		// uno scarto inesistente dentro un formato serializzato e riprodotto: misurato in code review il
		// 2026-08-26, difeso da `PlayerInteraction.NoSupersededEntryOnADashWithoutAPlannedMove`.
		const bool bHadNormalMove = Unit->HasPlannedNormalMove();
		const FRTCellId PreDashCell = Unit->Cell;

		Unit->Cell = Final;
		Unit->SetVisualLocation(Unit->WorldForCell(Final, Origin, CellSize, LayerH));
		ApplyTerrainOnEnterEffects(Snapshot.Map, Unit, Resolved[i].Entered, ERTMatchPhase::Dash);
		// Il movimento del turno e' finito qui: si scarta il percorso pianificato e la destinazione DIVENTA
		// la cella d'arrivo dello scatto. Senza l'assegnazione il resolver del Move vedrebbe una `PlannedCell`
		// diversa dalla posizione attuale e proverebbe comunque ad avvicinarcisi.
		//
		// 🔴 **E se c'era davvero un percorso, lo si DICE** (CP 38.2): fino al 2026-08-26 questo scarto era
		// muto, e un giocatore che aveva composto scatto + movimento vedeva la propria rotta sparire senza
		// che niente la nominasse. La voce si scrive QUI e non al lock-in perche' qui si sa **che cosa** viene
		// scartato: al commit il piano si contraddice e basta, e indovinare il perdente dall'ordine canonico
		// del validatore darebbe la risposta sbagliata.
		//
		// ⚠️ **Solo per chi e' ancora vivo**: `ApplyTerrainOnEnterEffects` qui sopra puo' aver ucciso l'unita'
		// sulla cella d'arrivo, e una traccia che adjudica il piano di un morto nella stessa fase in cui c'e'
		// la sua eliminazione dice due cose sullo stesso attore. Stesso filtro che `MakeCurrentSnapshot`
		// applica ovunque.
		if (bHadNormalMove && Unit->IsAlive())
		{
			FRTTurnLogEntry Superseded;
			Superseded.Phase = ERTMatchPhase::Dash;
			Superseded.Category = ERTLogCategory::Move;
			Superseded.Outcome = static_cast<uint8>(ERTMoveOutcome::SupersededByDash);
			// L'identita' dell'azione viene dal CATALOGO, come in `ResolveMovement`: `Priority` non e' un
			// campo decorativo, e' una chiave dell'ordine canonico (`EntryLess`) ed e' serializzata in v7.
			// Lasciata a `0` mentre l'altro produttore di `Action.Move` legge `50` dal catalogo, due voci
			// con lo stesso `ActionId` si ordinerebbero come se venissero da azioni diverse.
			//
			// `static const`: `FindCoreAction` COSTRUISCE il catalogo a ogni invocazione, e questo blocco sta
			// dentro il loop delle unita' che scattano. Stessa correzione gia' applicata in
			// `RTPlanValidationLibrary.cpp` dopo una misura in code review.
			static const FRTActionDef MoveDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
			Superseded.ActionId = TEXT("Action.Move"); // cio' che NON si esegue, non lo scatto che invece esegue
			Superseded.Priority = MoveDef.Priority;
			// 🔴 **`SrcCell` e' la cella di PARTENZA**, non quella d'arrivo: `BuildMoveLog` la dichiara
			// «chiave stabile dell'unita' nel turno», e `FilterTracesByEmitter` ci filtra sopra con
			// `ExcludedSources.Contains(Entry.SrcCell)` per confrontare le varianti a informazione nascosta.
			// Con la cella d'arrivo la traccia di una variante sopravvive al filtro e manda rosso un
			// confronto PERCHE' la variante ha funzionato. E la coppia (SrcCell, TgtCell) qui descrive una
			// ROTTA: quella che non si e' percorsa.
			Superseded.SrcCell = PreDashCell;       // da dove il movimento sarebbe partito
			Superseded.TgtCell = Unit->PlannedCell; // la destinazione dichiarata e mai raggiunta
			// `Amount` conta le celle del percorso RISOLTO SCARTATO — le stesse tre parole di `RTTurnLog.h`
			// e `spec-turnlog.md`, e «scartato» non e' decorativo: quelle celle NON sono state percorse, e'
			// il percorso che il pathfinder aveva espanso dai waypoint e che lo scatto ha annullato. Due
			// formulazioni per lo stesso campo sono la premessa di un difetto gia' pagato: un'asserzione che
			// confrontava `Amount` con i CLICK invece che col percorso passava solo perche' in quello
			// scenario i due numeri coincidevano. Un piano che dichiara solo una destinazione — il bot, che
			// «pianifica destinazioni, non percorsi a waypoint» — porta `0`, e la destinazione resta
			// leggibile in `TgtCell`. Non si stima dalla distanza: sarebbe un numero che nessuno ha
			// percorso, in un formato che finisce nell'hash del replay.
			Superseded.Amount = FMath::Max(0, Unit->PlannedPath.Num() - 1);
			AppendLogEntry(Superseded, Unit);
		}

		Unit->PlannedPath.Reset();
		Unit->PlannedWaypoints.Reset();
		Unit->PlannedCell = Final;

		const URTActionData* Used = Unit->GetAbility(DashAbilityIdx[i]);
		if (!Used) { continue; }

		// Chi ha usato un'azione che nega la reazione (CP 5.1: `Action.Sprint`) non ne tiene pronta una in
		// questo turno, comunque sia pianificata — vale QUI, non dove lo scatto e' stato solo pianificato,
		// perche' qui e' l'unico punto in cui l'azione risulta EFFETTIVAMENTE usata (non su cooldown, non
		// scartata dal fallback).
		if (!Used->Def.bAllowsReaction)
		{
			ReactionBlockedThisTurn.Add(Unit);
		}

		// SLOT consumati: lo dice il catalogo, non l'ActionId. Il movimento e' gia' speso qui sopra per OGNI
		// mobilita' rapida (D-028); resta da spendere la principale per chi dichiara `MovementAndMain`.
		//
		// Dopo D-028 nessuna azione di serie usa quello slot — `Action.Sprint`, che era l'unica, e' passata a
		// `Movement`. Il ramo resta perche' e' il meccanismo con cui un kit dichiara l'eccezione: «questa
		// mobilita' costa tutto il turno» si esprime in DATI, senza un `if` sull'ActionId qui dentro. E resta
		// verificato — `Actions.KitCanDeclareAMobilityThatCostsBothSlots` costruisce proprio quel caso, perche'
		// un ramo che nessun dato attraversa e' un ramo che nessun test difende.
		if (Used->Def.Slot == ERTActionSlot::MovementAndMain)
		{
			// 🔴 **Anche questo scarto si DICHIARA.** Fino ad ora la principale spariva in silenzio: stessa
			// forma del movimento scartato qui sopra, stessa ragione. Famiglia `Fallback`/`Cancelled` e non
			// `Move`/`SupersededByDash` perche' qui l'azione non avviene AFFATTO — mentre il movimento, dopo
			// lo scatto, l'unita' l'ha comunque compiuto. Il motivo viaggia in `Amount`, come per ogni voce
			// di Fallback (`RTTurnManager_Blast.cpp`).
			if (Unit->PlannedAbilityIndex != INDEX_NONE && Unit->IsAlive())
			{
				const URTActionData* Dropped = Unit->GetAbility(Unit->PlannedAbilityIndex);
				if (Dropped)
				{
					FRTTurnLogEntry Discarded;
					Discarded.Phase = ERTMatchPhase::Dash;
					Discarded.Category = ERTLogCategory::Fallback;
					Discarded.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
					Discarded.ActionId = Dropped->Def.ActionId;
					Discarded.BaseActionId = Dropped->Def.BaseActionId; // D-033: la traccia si spiega da sola
					Discarded.SrcCell = PreDashCell;   // chiave stabile dell'unita' nel turno
					Discarded.TgtCell = PreDashCell;   // = SrcCell: qui non c'e' una destinazione
					// L'identita' COMPLETA dell'azione scartata: `ActionId` c'era gia', gli altri due no ([D-196]).
					//
					// ⚠️ Fino al 2026-08-26 qui c'era scritto che «nessun produttore `Fallback` imposta
					// `Priority` dal catalogo». Da [D-196] la impostano tutti quelli del Blast, e lasciarla a
					// zero qui avrebbe fatto ordinare la stessa famiglia su due chiavi diverse a seconda di
					// chi ha scritto la voce — `Priority` e' un discriminante di `EntryLess`.
					Discarded.BaseActionId = Dropped->Def.BaseActionId;
					Discarded.Priority = Dropped->Def.Priority;
					Discarded.Amount = static_cast<int32>(ERTActionInvalidReason::SlotOccupied);
					AppendLogEntry(Discarded, Unit);
					// Niente `AddLogEvent` qui: la riga arriva al combat log attraverso `ConcludeTurn`, che
					// deriva l'intero log dal TurnLog (`DescribeTurnLog`) — come ogni altra voce.
				}
			}

			Unit->PlannedAbilityIndex = INDEX_NONE; // lo slot principale e' speso
			Unit->PlannedAttackTarget = nullptr;
		}

		// Effetti DICHIARATI dall'azione (Sprint applica `Status.Exposed`): stesso registry di Prep e Blast.
		// L'orchestratore non sa quale stato sia ne' perche': aggiungerne un altro non lo tocca.
		FRTActionInstance Instance;
		Instance.Def = Used->Def;
		Instance.SourceUnitId = i;
		Instance.TargetUnitId = i;   // le mobilita' del vertical slice applicano i propri effetti a chi le usa
		Instance.TargetCell = Final;
		Instance.EventSequence = i;
		for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEvents(Instance))
		{
			if (Event.Kind == ERTActionEffect::Status)
			{
				ApplyStatusLogged(Unit, Event.StatusTag, Event.Amount);
				// #1077: nascita da AZIONE, sul percorso del movimento lineare. Stessa guardia del sito
				// del Prep: nessuna voce se `ApplyStatus` non ha applicato niente.
				if (Event.Amount == ARTUnit::PersistentWhileOnCell || Event.Amount > 0)
				{
					FRTTurnLogEntry Nato = MakeStatusBirthEntry(Phase, Event.StatusTag, Unit->Cell, Event.Amount,
						/*bFromTerrain=*/ false);
					AppendLogEntry(Nato, Unit);
				}
				AddLogEvent(FString::Printf(TEXT("%s: %s per %d turno/i"),
					*Unit->GetName(), *Event.StatusTag.ToString(), Event.Amount), FRTLogSubject::Unit(Unit));
			}
			// Danno e spinta della Carica (CP 4.5) non si applicano qui: hanno per bersaglio il primo nemico
			// sulla linea, non chi scatta, e vanno risolti con gli altri colpi.
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Dash: %d scatti"), DasherCount);
}

void ARTTurnManager::RunReactionPass(ERTReactionPassPoint Point,
	TFunctionRef<FRTReactionTriggerHit(int32, ERTReactionTrigger)> Evaluate,
	const TArray<ARTUnit*>& Units, TArray<FRTUnitCombatState>& States,
	const URTHexMapAsset* Map, FRTReactionPassResult& Out)
{
	// Reazioni (CP 5.1): valutate su cio' che la fase ha gia' raccolto o deciso — lo snapshot congelato, non
	// un evento a cui reagire mentre il turno gira (invariante #3). Cosa sia quello snapshot dipende dal
	// punto: i colpi di `Plan.Hits` dopo il filtro di Interrupt, o gli spostamenti decisi e non applicati.
	// Un'unita' con piu' trigger validi nello stesso Blast si ferma comunque a UNA attivazione:
	// `FindTriggeringAttacker` restituisce il primo colpo che soddisfa il trigger, non li conta.
	// L'attivazione — o la non-attivazione, col motivo — finisce SEMPRE nel TurnLog, mai in silenzio.
	//
	// Gli EFFETTI delle reazioni attivate (CP 5.2) non si applicano qui: si raccolgono e si applicano insieme
	// agli altri colpi, piu' sotto. E' "raccogli poi applica" (invariante #3) — una reazione che modificasse
	// subito il danno lo farebbe su un totale ancora incompleto, e l'esito dipenderebbe dall'ordine delle unita'.
	// Le FUGHE di chi reagisce (`SelfReposition`, D-093) si raccolgono qui e si applicano DOPO il ciclo
	// (D-094). Spostarle dentro cambierebbe la posizione che le reazioni valutate dopo vedrebbero, e
	// `Action.Intercept` chiede l'alleato entro 2 celle: l'esito dipenderebbe dall'ordine di `Units`.
	TArray<ARTUnit*> FleeUnits;
	TArray<FRTCellId> FleeFrom;   // da CHI ci si allontana: e' anche la sorgente del facing (D-104)
	TArray<int32> FleeDist;
	Out.DeflectDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 ReactionIdx = Unit->PlannedReactionAbility;

		const URTActionData* Reaction = Unit->GetAbility(ReactionIdx);
		if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction)
		{
			continue; // nessuna reazione pianificata: niente da registrare
		}

		// Questo trigger si valuta ALTROVE: non e' affare di questo punto, e passarci sopra in silenzio e'
		// cio' che tiene una sola voce di TurnLog per reazione. Ci finisce anche l'interposizione, che ha il
		// suo ciclo dedicato piu' sopra — fino a `#505` a tenerla fuori bastava l'azzeramento del piano che
		// quel ciclo faceva, ma ora il piano deve sopravvivere alla fase (D-092) e l'esclusione va DETTA.
		if (URTReactionLibrary::PassPointFor(Reaction->Def.ReactionTrigger) != Point)
		{
			continue;
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Reaction;
		Entry.SrcCell = Unit->Cell;
		Entry.TgtCell = Unit->Cell;
		Entry.ActionId = Reaction->Def.ActionId; // identita': `Wraith.Deflection` non e' `Action.Deflect` (CP 5.5)
		Entry.BaseActionId = Reaction->Def.BaseActionId; // vuoto finche' le reazioni non dichiarano un profilo

		// CHI sa se il trigger e' scattato e' il chiamante: cambia con il punto, e il pass non ha modo di
		// saperlo senza conoscere i dati di ciascuna fase — cioe' senza tornare a essere quattro pass.
		const FRTReactionTriggerHit Hit = Evaluate(i, Reaction->Def.ReactionTrigger);
		const int32 TriggeredBy = Hit.TriggeredBy;

		// `ReactionActivationsThisTurn` e' la garanzia di [D-092] resa esplicita: una attivazione per turno, e
		// il contatore attraversa le fasi. Senza, la regola resterebbe vera solo come CONSEGUENZA del fatto
		// che `PassPointFor` da' a ogni trigger un punto solo — e cadrebbe in silenzio il giorno in cui un
		// trigger ne avesse due, senza che nessun test la stesse guardando.
		if (!Unit->CanUseAbility(ReactionIdx) || ReactionBlockedThisTurn.Contains(Unit)
			|| Unit->ReactionActivationsThisTurn > 0)
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Unavailable);
		}
		else if (Hit.bTriggered)
		{
			Unit->ConsumeAbility(ReactionIdx);
			++Unit->ReactionActivationsThisTurn;
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Activated);

			// TUTTI gli effetti che la reazione DICHIARA, non il primo che questo orchestratore riconosce
			// (CP 5.5). `URTReactionLibrary::BuildReactionEvents` decide anche CHI li subisce, per tipo di
			// effetto: offensivi a chi ha innescato, difensivi a chi reagisce. Qui non si guarda mai
			// l'`ActionId`: e' cio' che permette a una reazione d'eroe di riusare la semantica di
			// `Action.Deflect`/`Action.Counter` con numeri propri senza un ramo per eroe.
			for (const FRTActionEvent& Event : URTReactionLibrary::BuildReactionEvents(Reaction->Def, i, TriggeredBy))
			{
				if (!Units.IsValidIndex(Event.TargetUnitId) || !Units[Event.TargetUnitId]) { continue; }
				ARTUnit* EffectTarget = Units[Event.TargetUnitId];
				switch (Event.Kind)
				{
				case ERTActionEffect::Damage:
					// Il colpo di ritorno entra fra gli attacchi normali, quindi risolve sullo stato iniziale
					// come tutti gli altri: chi cade in questo stesso Blast contrattacca comunque, che e' la
					// regola gia' dichiarata da URTCombatResolver ("un'unita' colpita a morte infligge
					// comunque il proprio danno").
					Out.CounterAttacks.Add(FRTAttack(Event.TargetUnitId, Event.Amount));
					Out.CounterAttackSrc.Add(Unit->Cell);
					Out.CounterActionId.Add(Reaction->Def.ActionId);
					Out.CounterBaseActionId.Add(Reaction->Def.BaseActionId);
					Out.CounterPriority.Add(Reaction->Def.Priority);
					Out.CounterAttackActors.Add(Unit);
					AddLogEvent(FString::Printf(TEXT("%s: contrattacco su %s (%d)"),
						*Unit->GetName(), *EffectTarget->GetName(), Event.Amount), FRTLogSubject::Unit(Unit));
					break;

				case ERTActionEffect::DamageReduction:
					// Vale sul colpo che ha innescato la reazione: si attiva una volta sola, quindi entra fra
					// i delta del PRIMO danno diretto, come il -15 di Guard.
					Out.DeflectDelta[Event.TargetUnitId] -= Event.Amount;
					break;

				case ERTActionEffect::Shield:
					// Prima che i colpi vengano risolti, e con lo stesso aggiornamento sullo snapshot `States`
					// da cui il resolver legge: uno scudo che arrivasse dopo scadrebbe nel Cleanup dello
					// stesso turno senza aver protetto da niente.
					EffectTarget->AddTemporaryShield(Event.Amount);
					// `IsValidIndex` e non un accesso diretto: i punti di valutazione fuori dal Blast non hanno
					// uno snapshot di combattimento da aggiornare e passano un array vuoto. Uno scudo li' non
					// avrebbe comunque colpi da fermare — ma senza questa guardia sarebbe un accesso fuori
					// range, cioe' un crash invece di un effetto che non serve.
					if (States.IsValidIndex(Event.TargetUnitId))
					{
						States[Event.TargetUnitId].Shield = EffectTarget->Shield;
						// [D-224] Anche la quota temporanea: lo scudo appena concesso E' temporaneo, e
						// aggiornare solo il totale lo farebbe passare per BASE agli occhi del resolver.
						States[Event.TargetUnitId].TemporaryShield = EffectTarget->GetTemporaryShield();
					}
					AddLogEvent(FString::Printf(TEXT("%s: +%d scudo dalla reazione"),
						*EffectTarget->GetName(), Event.Amount), FRTLogSubject::Unit(EffectTarget));
					break;

				case ERTActionEffect::SelfReposition:
					// D-093: chi reagisce si allontana da chi l'ha innescato. Si RACCOGLIE soltanto — vedi
					// il commento sugli array, D-094.
					if (TriggeredBy != INDEX_NONE && Units.IsValidIndex(TriggeredBy) && Units[TriggeredBy])
					{
						FleeUnits.Add(EffectTarget);
						FleeFrom.Add(Units[TriggeredBy]->Cell);
						FleeDist.Add(Event.Amount);
					}
					else
					{
						// Nessuna sorgente: e' una fuga da una CELLA, non da qualcuno (`Reaction.HazardEscape`).
						// Non ha una direzione da cui allontanarsi, quindi la geometria della spinta non si
						// applica e la destinazione la sceglie il chiamante. Prima di `#505` questo ramo non
						// esisteva e l'effetto veniva scartato in silenzio: un modulo che dichiarava
						// `SelfReposition` senza attaccante non si muoveva, e nessun test lo diceva.
						Out.HazardFlees.Add(Event.TargetUnitId);
						Out.HazardFleeDistance.Add(Event.Amount);
					}
					break;

				case ERTActionEffect::CancelStatus:
					// Come sotto: si raccoglie e basta. QUALE controllo salti lo decide chi applica gli stati,
					// che ha davanti la lista completa di quelli in arrivo e sceglie il piu' grave.
					Out.CancelledControls.Add(Event.TargetUnitId);
					break;

				case ERTActionEffect::CancelDisplacement:
					// Si RACCOGLIE soltanto, come tutto il resto: chi muove e' piu' sotto e consultera' questo
					// insieme prima di spostare. Annullare QUI non avrebbe niente da annullare — la
					// destinazione della spinta non e' ancora stata calcolata.
					Out.CancelledDisplacements.Add(Event.TargetUnitId);
					break;

				default:
					// `Heal`, `Push`, `Pull`, `Status`: nessuna reazione del catalogo v0.1 li dichiara, e
					// applicarli qui richiederebbe cio' che il pass non ha (una direzione per la spinta, il
					// consumo degli stati insieme agli altri colpi). Il posto dove aggiungerli e' questo.
					break;
				}
			}
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::NotTriggered);
		}

		// Chi REAGISCE. Nell'interposizione `SrcCell` e' la cella del protetto, non la sua: dedurre
		// l'unita' dalla voce darebbe l'unita' sbagliata ([D-063]).
		AppendLogEntry(Entry, Unit);
		// Stesso soggetto della voce, e non per simmetria estetica: questa riga rieccheggia **verbatim**
		// cio' che `ConcludeTurn` deriva dalla voce qui sopra, `DescribeEntry` comprese le coordinate.
		// Senza soggetto la copia derivata sarebbe filtrata e questa no, e le coordinate soppresse
		// arriverebbero comunque a schermo dalla seconda porta.
		AddLogEvent(FString::Printf(TEXT("%s: %s"), *Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Entry)), FRTLogSubject::Unit(Unit));
	}

	// Le FUGHE raccolte sopra si applicano ora, con tutte le reazioni gia' valutate sullo snapshot congelato
	// (D-094). Prima si calcolano tutte le destinazioni e poi si muove: due unita' che fuggono nello stesso
	// Blast non devono vedersi a vicenda gia' spostate, altrimenti la seconda troverebbe libera una cella che
	// la prima sta lasciando e l'esito dipenderebbe dall'ordine.
	if (FleeUnits.Num() > 0)
	{
		TArray<FRTCellId> FleeBlockers;
		for (ARTUnit* U : Units) { if (IsValid(U) && U->IsAlive()) { FleeBlockers.Add(U->Cell); } }

		TArray<FRTCellId> FleeDest;
		FleeDest.Reserve(FleeUnits.Num());
		for (int32 f = 0; f < FleeUnits.Num(); ++f)
		{
			ARTUnit* Fleeing = FleeUnits[f];
			// Stessa geometria della spinta: ci si allontana dalla sorgente e ci si ferma davanti agli stessi
			// ostacoli. Una fuga non attraversa muri che una spinta non attraversa.
			FleeDest.Add((IsValid(Fleeing) && Fleeing->IsAlive())
				? URTHexCombatLibrary::HexKnockbackDestination(
					FleeFrom[f], Fleeing->Cell, FleeDist[f], Map, FleeBlockers)
				: FRTCellId());
		}

		for (int32 f = 0; f < FleeUnits.Num(); ++f)
		{
			ARTUnit* Fleeing = FleeUnits[f];
			if (!IsValid(Fleeing) || !Fleeing->IsAlive()) { continue; }
			if (FleeDest[f] == Fleeing->Cell)
			{
				// Non c'e' dove andare: la reazione e' scattata e ha speso la sua attivazione. E' un esito, e
				// va detto — altrimenti nel log resta una reazione senza conseguenze e sembra un difetto.
				AddLogEvent(FString::Printf(TEXT("%s: nessuna cella libera per la fuga"), *Fleeing->GetName()), FRTLogSubject::Unit(Fleeing));
				continue;
			}
			// Gli stessi dieci passi della spinta (#541): traccia con causa, hazard attraversati, facing verso
			// la minaccia (D-104), piano che segue. Una riga, perche' la primitiva esiste.
			TMap<ARTUnit*, FRTDisplacementCause> FleeCause;
			FleeCause.Add(Fleeing, FRTDisplacementCause{ FName(TEXT("Reaction.EmergencyDash")), NAME_None, 0 });
			ApplyForcedDisplacement(Fleeing, FleeDest[f], FleeFrom[f], FleeCause, TEXT("Fuga"), Map,
				ERTMatchPhase::Dash);
		}
	}
}

void ARTTurnManager::ResolveCombat()
{
	// 🔴 **Il pagamento sta FUORI dalla sequenza, e non e' una preferenza di stile** (`#1451` punto 3).
	//
	// `ResolveCombatPasses` ha un'uscita anticipata — «nessun colpo» — e finche' il consumo e' stato dentro
	// la sequenza, quell'uscita ha DOVUTO lasciare che ogni azione pagasse per conto proprio: e' esattamente
	// cio' che il commento di quel `return` dichiarava, ed e' la ragione per cui i punti di consumo erano
	// cinque. MISURATO il 2026-08-27: con la passata unica in coda alla sequenza, una cura fuori da uno
	// scontro non pagava piu' — quattro test rossi, e la causa era il `return`, non la passata.
	//
	// Tenendolo qui l'invariante diventa STRUTTURALE: qualunque uscita futura della sequenza passa comunque
	// da `SpendStartedAbilities`, e un `return` in piu' non puo' far dimenticare il cooldown a nessuno.
	FRTBlastContext Ctx;
	ResolveCombatPasses(Ctx);
	SpendStartedAbilities(Ctx);
}

void ARTTurnManager::ResolveCombatPasses(FRTBlastContext& Ctx)
{
	// La fase Blast e' una SEQUENZA: chi ordina sta qui, chi decide sta nei pass. L'ordine non e' un dettaglio
	// di implementazione — il catalogo assegna alle azioni un codice (20 movimento, 30 controllo, 40 attacco,
	// 70 supporto) e questa funzione lo rispetta. Spostare una chiamata cambia il gioco.
	GatherBlastUnits(Ctx);
	RefreshTeamKnowledgeForBlast(Ctx);
	ResolveCleanseActions(Ctx);
	CollectHealActions(Ctx);
	CollectAttackIntents(Ctx);
	AppendChargeImpactIntents(Ctx);

	// La geometria decide quali intenti diventano colpi: portata, linea di tiro e celle raggiunte dall'area.
	Ctx.Plan = URTHexCombatLibrary::CollectHexAttacks(Ctx.HexUnits, Ctx.Intents, Ctx.Map);

	// Alias sui campi del contesto: il resto della fase non e' ancora estratto e continua a leggerli con i
	// nomi che avevano da variabili locali. Sono RIFERIMENTI, non copie: nessun cambio di semantica. Ogni
	// alias sparisce quando il pass che lo usa diventa un metodo, e quando non ne resta nessuno questa
	// funzione e' solo la sequenza qui sopra.
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TMap<ARTUnit*, int32>& IndexOf = Ctx.IndexOf;
	TArray<FRTUnitCombatState>& States = Ctx.States;
	TArray<FRTHexCombatUnit>& HexUnits = Ctx.HexUnits;
	TArray<ARTUnit*>& HealTargets = Ctx.HealTargets;
	TArray<int32>& HealAmounts = Ctx.HealAmounts;
	TArray<FRTCellId>& HealSources = Ctx.HealSources;
	TArray<ARTUnit*>& HealActors = Ctx.HealActors;
	const TArray<FRTActionDef>& HealDefs = Ctx.HealDefs;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	TArray<int32>& IntentAbilityIndex = Ctx.IntentAbilityIndex;
	TArray<const URTActionData*>& IntentAbility = Ctx.IntentAbility;
	TArray<FRTActionDef>& IntentDefs = Ctx.IntentDefs;
	TArray<FRTPendingArcOp>& PendingArcOps = Ctx.PendingArcOps;
	FRTHexBlastPlan& Plan = Ctx.Plan;
	FRTReactionPassResult& Reactions = Ctx.Reactions;

	// Interrupt e Intercept riscrivono il piano PRIMA che i colpi diventino danno: il primo toglie cio' che
	// non deve partire, il secondo cambia chi lo incassa. Entrambi decidono su colpi congelati e applicano
	// dopo — l'ordine fra loro e' quello del catalogo, non una comodita'.
	ApplyInterrupts(Ctx);
	ResolveInterceptions(Ctx);

	RunBlastReactions(Ctx);
	LogBlockedIntents(Ctx);
	ApplyEnvironmentChanges(Ctx);


	// Intenti NON VALUTABILI (nessuna mappa autorevole): non finiscono nel TurnLog come «nessuna linea di
	// tiro» — sarebbe un esito di gioco al posto di un difetto di configurazione del livello. Restano un
	// warning, perche' e' il livello a dover essere corretto, non la posizione dell'unita'.
	if (Plan.UnverifiableIntents.Num() > 0)
	{
		AddLogEvent(FString::Printf(
			TEXT("Nessuna mappa esagonale: %d attacchi non validabili, nessun colpo applicato"),
			Plan.UnverifiableIntents.Num()), FRTLogSubject::World());
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Blast senza mappa autorevole: %d intenti non valutabili (livello senza ARTHexMapActor con celle)"),
			Plan.UnverifiableIntents.Num());
	}

	// Colpi a segno -> attacchi da applicare, con gli effetti collaterali dell'abilita' che li ha prodotti.
	// `Status.Marked` (CP 8.2, chiude la issue #137): pass a PRIORITA' dentro il Blast. Il catalogo da' a
	// `Action.MarkTarget` la priorita' 40 «la piu' bassa delle offensive, perche' un marchio che arrivasse
	// dopo i colpi non servirebbe a nulla»: il marchio deve quindi valere per i colpi dello stesso Blast a
	// priorita' piu' alta. Applicarlo insieme agli altri status (piu' sotto, dopo il danno) lo renderebbe
	// inutilizzabile in ogni turno — con durata 1 scade nel Cleanup dello stesso turno in cui e' applicato.
	//
	// Resta «raccogli poi applica»: si aggiustano i Power di colpi GIA' congelati, non si ricalcolano
	// bersagli o traiettorie. Precedente nel codice: `Action.Interrupt` (priorita' 20) filtra i colpi del
	// piano prima che diventino danno.
	TArray<int32> IncomingMarkPriority; // per bersaglio: priorita' del marchio applicato in QUESTO Blast
	IncomingMarkPriority.Init(MAX_int32, Units.Num());
	TArray<bool> bMarkedBeforeBlast;    // marchio ereditato da un turno precedente: vale per qualunque colpo
	bMarkedBeforeBlast.Init(false, Units.Num());
	// Stessa disciplina per `Wet`, e per la stessa ragione: `Phase.PressureJet` bagna DENTRO il Blast, quindi
	// un bonus che leggesse solo `HasStatus` non lo vedrebbe mai — e con durata 1 il bagnato scade nel Cleanup
	// dello stesso turno, quindi non lo vedrebbe nemmeno il turno dopo. La combo acqua+elettricita' era
	// documentata, aveva un test verde sull'aritmetica, e non era eseguibile in partita (#242).
	// Differenza dal marchio: il bagnato NON si consuma. Vale su ogni colpo finche' il bersaglio e' bagnato,
	// quindi qui si raccoglie soltanto la priorita' — non c'e' un `WetSpentOn`.
	TArray<int32> IncomingWetPriority;  // per bersaglio: priorita' dell'azione che lo bagna in QUESTO Blast
	IncomingWetPriority.Init(MAX_int32, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		bMarkedBeforeBlast[i] = Units[i] && Units[i]->HasStatus(TAG_Status_Marked);
	}
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex) || !Units.IsValidIndex(Hit.TargetId)
			|| !Units.IsValidIndex(Hit.AttackerId) || !Units[Hit.TargetId] || !Units[Hit.AttackerId])
		{
			continue;
		}
		const FRTActionDef& Def = IntentDefs[Hit.IntentIndex];
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Marked)
			{
				Units[Hit.TargetId]->ApplyMarkedBy(Units[Hit.AttackerId]->TeamId, Spec.StatusDuration);
				// #1077: anche questa e' una nascita, e senza la sua voce il Cleanup avrebbe registrato la
				// scadenza di un tag che il log non aveva mai visto comparire. Trovato in code review.
				if (Spec.StatusDuration > 0)
				{
					FRTTurnLogEntry Nato = MakeStatusBirthEntry(Phase, TAG_Status_Marked.GetTag(),
						Units[Hit.TargetId]->Cell, Spec.StatusDuration, /*bFromTerrain=*/ false);
					AppendLogEntry(Nato, Units[Hit.TargetId]);
				}
				IncomingMarkPriority[Hit.TargetId] = FMath::Min(IncomingMarkPriority[Hit.TargetId], Def.Priority);
			}
			// Il bagnato si REGISTRA e basta: applicarlo qui lo farebbe scadere nel Cleanup come prima, e
			// soprattutto lo renderebbe visibile a colpi che risolvono PRIMA di chi bagna. Lo applica il pass
			// degli effetti, dopo il danno, come per ogni altro status.
			else if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Wet)
			{
				IncomingWetPriority[Hit.TargetId] = FMath::Min(IncomingWetPriority[Hit.TargetId], Def.Priority);
			}
		}
	}

	// Ordine di spesa del marchio: priorita' crescente, poi ActionId -> AttackerId -> IntentIndex. E' l'ordine
	// canonico dell'ADR-0003 §3, e serve perche' «il PROSSIMO attacco alleato» sia una domanda con una sola
	// risposta anche quando due alleati colpiscono lo stesso bersaglio nello stesso turno.
	TArray<int32> HitOrder;
	HitOrder.Reserve(Plan.Hits.Num());
	for (int32 h = 0; h < Plan.Hits.Num(); ++h) { HitOrder.Add(h); }
	HitOrder.Sort([&Plan, &IntentDefs](int32 A, int32 B)
	{
		const FRTHexAttackHit& HA = Plan.Hits[A];
		const FRTHexAttackHit& HB = Plan.Hits[B];
		const int32 PA = IntentDefs.IsValidIndex(HA.IntentIndex) ? IntentDefs[HA.IntentIndex].Priority : MAX_int32;
		const int32 PB = IntentDefs.IsValidIndex(HB.IntentIndex) ? IntentDefs[HB.IntentIndex].Priority : MAX_int32;
		if (PA != PB) { return PA < PB; }
		const FName IdA = IntentDefs.IsValidIndex(HA.IntentIndex) ? IntentDefs[HA.IntentIndex].ActionId : NAME_None;
		const FName IdB = IntentDefs.IsValidIndex(HB.IntentIndex) ? IntentDefs[HB.IntentIndex].ActionId : NAME_None;
		if (IdA != IdB) { return IdA.LexicalLess(IdB); }
		if (HA.AttackerId != HB.AttackerId) { return HA.AttackerId < HB.AttackerId; }
		return HA.IntentIndex < HB.IntentIndex; // ordine TOTALE
	});

	TSet<int32> MarkSpentOn;
	for (int32 h : HitOrder)
	{
		FRTHexAttackHit& Hit = Plan.Hits[h];
		if (Hit.Power <= 0 || MarkSpentOn.Contains(Hit.TargetId)) { continue; } // "attacco": un colpo che fa danno
		if (!Units.IsValidIndex(Hit.TargetId) || !Units.IsValidIndex(Hit.AttackerId)
			|| !Units[Hit.TargetId] || !Units[Hit.AttackerId])
		{
			continue;
		}
		ARTUnit* Target = Units[Hit.TargetId];
		if (!Target->HasStatus(TAG_Status_Marked)) { continue; }
		if (Units[Hit.AttackerId]->TeamId != Target->GetMarkedByTeam()) { continue; } // solo la squadra del marcatore
		if (!bMarkedBeforeBlast[Hit.TargetId])
		{
			// Marchio nato in questo Blast: vale solo per i colpi a priorita' PIU' ALTA, cioe' risolti dopo.
			const int32 HitPriority = IntentDefs.IsValidIndex(Hit.IntentIndex)
				? IntentDefs[Hit.IntentIndex].Priority : MAX_int32;
			if (HitPriority <= IncomingMarkPriority[Hit.TargetId]) { continue; }
		}
		Hit.Power += URTCombatLibrary::MarkedFirstHitBonus;
		MarkSpentOn.Add(Hit.TargetId);
	}
	// Consumo dopo il pass, non dentro: durante il pass `HasStatus` deve rispondere sullo stato congelato.
	for (int32 TargetId : MarkSpentOn)
	{
		// Il marchio non scade e non viene tolto: viene **incassato**. `Spent` lo separa da `Expired` per la
		// stessa ragione per cui `Revoked` lo e' gia' — un replay deve poter dire che lo stato ha fatto il suo
		// lavoro invece che essere finito il tempo (`#1314`).
		if (Units[TargetId]->RemoveStatus(TAG_Status_Marked))
		{
			FRTTurnLogEntry Incassato = MakeStatusDeathEntry(
				TAG_Status_Marked, Units[TargetId]->Cell, ERTStatusOutcome::Spent);
			AppendLogEntry(Incassato, Units[TargetId]);
		}
	}

	// Bonus condizionati alla COPPIA (chi colpisce, chi subisce): si applicano sui colpi del piano, dove
	// attaccante e bersaglio sono ancora entrambi noti — `FRTAttack` conserva solo il bersaglio, e dopo
	// `ToAttacks` l'informazione non esiste piu'. Dopo l'Intercept, perche' il bersaglio puo' essere cambiato:
	// il bonus lo decide chi il colpo lo incassa davvero.
	//
	// `Gadget.LinearDischarge` +8 contro bersaglio `Status.Wet` (catalogo eroi §1). Non e' nella lista `Effects`
	// perche' non e' un danno fisso, e vale su OGNI colpo dell'azione finche' il bersaglio e' bagnato — non
	// solo sul primo, quindi non passa dai delta qui sotto.
	//
	// Due sorgenti di bagnato, e contano entrambe:
	//   - GIA' bagnato quando il Blast comincia (acqua bassa attraversata nel Dash, o turno precedente):
	//     `HasStatus` risponde di si', e vale per qualunque colpo;
	//   - bagnato IN QUESTO Blast (`Phase.PressureJet`, priorita' 50): vale solo per i colpi a priorita' piu'
	//     ALTA, cioe' risolti dopo. `LinearDischarge` ha priorita' 55, quindi la coordinazione funziona.
	// La seconda meta' mancava, ed e' il motivo per cui la combo firma della v0.1 non era eseguibile (#242).
	// L'ordine e' quello canonico di ADR-0003 §3, lo stesso del marchio: non ne nasce un secondo.
	//
	// LIMITE DICHIARATO (CP 8.2): il confronto e' su un `ActionId` scritto qui. E' l'unico bonus condizionale
	// del catalogo v0.1; quando ce ne sara' un secondo, la forma giusta e' un campo del catalogo azioni
	// («bonus X contro stato Y»), non un secondo `if`.
	for (FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex)
			|| IntentDefs[Hit.IntentIndex].ActionId != FName(TEXT("Hero.Gadget.LinearDischarge")))
		{
			continue;
		}
		if (!Units.IsValidIndex(Hit.TargetId) || !Units[Hit.TargetId])
		{
			continue;
		}
		const bool bWetBeforeBlast = Units[Hit.TargetId]->HasStatus(TAG_Status_Wet);
		const bool bWetFromThisBlast =
			IntentDefs[Hit.IntentIndex].Priority > IncomingWetPriority[Hit.TargetId];
		if (bWetBeforeBlast || bWetFromThisBlast)
		{
			Hit.Power = URTCombatLibrary::EffectiveAttackPower(Hit.Power, URTCombatLibrary::GadgetWetDischargeBonus);
		}
	}

	// Gli stati che valgono sul PRIMO danno diretto entrano qui come un delta per bersaglio: `Status.Exposed`
	// (chi ha scattato allo scoperto) somma +5. Vale una volta sola, quindi il totale non dipende da quale
	// colpo se lo prenda: e' un delta POSITIVO, e il clamp a zero di `ApplyFirstHitDelta` non lo tocca mai.
	//
	// ⚠️ `Status.Guarded` NON sta piu' qui ([D-292]). Con un delta NEGATIVO piu' grande del colpo che lo
	// riceve, la riduzione che avanza si perdeva nel clamp, e quanta se ne perdesse dipendeva da quale colpo
	// era primo: un bersaglio in Guardia colpito da 10 e da 30 incassava 30 o 25 a seconda dell'ordine
	// dell'array. La Guardia e' ora un POOL, piu' sotto.
	TArray<int32> FirstHitDelta;
	FirstHitDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (!Units[i]) { continue; }
		if (Units[i]->HasStatus(TAG_Status_Exposed))
		{
			FirstHitDelta[i] += URTCombatLibrary::ExposedFirstHitBonus;
		}
		// Il ramo `Status.Guarded` non e' piu' qui: la Guardia e' un pool, costruito dopo questo ciclo.
	}

	// `Status.Guarded` ([D-292]): la Guardia e' un POOL di 15 danni assorbibili, e solo i colpi dell'arco
	// FRONTALE lo consumano — l'emisfero posteriore resta scoperto ([D-206]). Cio' che un colpo non consuma
	// resta per i successivi, quindi il totale non dipende piu' da quale colpo arriva per primo.
	//
	// La maschera e' PER-COLPO, che e' il controllo direzionale che [D-206] ha deciso e che nessuno poteva
	// implementare prima di [D-212]: `FRTAttack` non portava l'attaccante, e «questo colpo e' frontale» non
	// era esprimibile dentro il resolver.
	// ⚠️ `bFrontalHit` e' indicizzato come `Plan.Hits`, ed e' lecito perche' `ToAttacks` mappa **1:1** e le
	// due funzioni che stanno in mezzo (`ApplyFirstHitDelta`, `ApplyDamageDelta`) copiano l'array e toccano
	// solo `Power`: nessuna aggiunge, toglie o riordina. Se un giorno una di loro cambiasse la cardinalita',
	// questa maschera punterebbe ai colpi sbagliati **in silenzio** — e' l'assunzione da rompere per prima
	// se il pool assorbisse dal lato sbagliato.
	TArray<int32> GuardPool;
	GuardPool.Init(0, Units.Num());
	TArray<bool> bFrontalHit;
	bFrontalHit.Init(false, Plan.Hits.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (!Units[i] || !Units[i]->HasStatus(TAG_Status_Guarded)) { continue; }
		GuardPool[i] = URTCombatLibrary::GuardFirstHitReduction;

		for (int32 h = 0; h < Plan.Hits.Num(); ++h)
		{
			const FRTHexAttackHit& Hit = Plan.Hits[h];
			if (Hit.TargetId != i) { continue; }

			// Attaccante non risolvibile: il colpo NON e' eleggibile. Un dato mancante non deve concedere una
			// protezione che nessuno ha dichiarato — fail-closed, come il resto del combattimento.
			const bool bFrontal = HexUnits.IsValidIndex(Hit.AttackerId)
				&& URTHexCombatLibrary::IsInFrontalArc(
					HexUnits[i].Cell, HexUnits[i].Facing, HexUnits[Hit.AttackerId].Cell);
			bFrontalHit[h] = bFrontal;

			if (!bFrontal && HexUnits.IsValidIndex(Hit.AttackerId))
			{
				FRTTurnLogEntry Bypassed;
				Bypassed.Phase = ERTMatchPhase::Blast;
				Bypassed.Category = ERTLogCategory::Facing;
				// `RearHitBypassedGuard` e non `...Cover` (`#1430`, [D-199]): questo ramo racconta la GUARDIA
				// scavalcata e mette in `Amount` la DIREZIONE del difensore, mentre il ramo della copertura ci
				// mette i punti di riduzione. Erano lo stesso esito con due payload incompatibili.
				Bypassed.Outcome = static_cast<uint8>(ERTFacingOutcome::RearHitBypassedGuard);
				Bypassed.SrcCell = HexUnits[Hit.AttackerId].Cell;
				Bypassed.TgtCell = HexUnits[i].Cell;
				Bypassed.Amount = static_cast<int32>(HexUnits[i].Facing);
				// 🔴 `UnitId` porta CHI SUBISCE, non chi ha colpito, e non e' una scelta arbitraria: e' cio'
				// che questa voce DESCRIVE. `Amount` porta il `Facing` del difensore, `TgtCell` la sua cella,
				// e la categoria e' `Facing` — l'evento e' «l'orientamento del difensore non ha retto». Le
				// altre voci `Facing` seguono la stessa regola: `MakeFacingEntry` mette cella e direzione
				// dell'unita' il cui orientamento sta raccontando.
				//
				// Fino a `#1418` qui arrivava l'ATTACCANTE mentre la riga leggibile due righe sotto nominava
				// il difensore: un consumatore che aggrega per `UnitId` e un umano che legge il log
				// rispondevano diversamente alla domanda «chi l'ha fatto». La riga leggibile aveva ragione.
				//
				// ⚠️ `UnitId` non entra nell'hash (D-063), quindi questa correzione non tocca l'identita'
				// delle tracce archiviate: cambia chi la voce dichiara, non quale traccia e'.
				AppendLogEntry(Bypassed, Units[i]);
				// ⚠️ Il SOGGETTO e' lo stesso della voce qui sopra, e la regola non e' «e' l'attaccante»:
				// e' «e' chi la voce dichiara». `ConcludeTurn` deriva da quella voce una riga identica a
				// questa, e due soggetti diversi sulla stessa frase significherebbero che una delle due
				// copie passa il filtro quando l'altra non passa — cioe' il leak che il filtro sopprime.
				//
				// 🔴 Prima del merge di `#1418` questa riga passava l'ATTACCANTE, ed era coerente **con la
				// voce di allora**: `AppendLogEntry` riceveva l'attaccante. Corretto il dato, il soggetto lo
				// segue — se fosse restato l'attaccante, la coerenza che il vincolo protegge sarebbe stata
				// rotta proprio dalla correzione che la rendeva possibile.
				AddLogEvent(FString::Printf(TEXT("%s: %s"), *Units[i]->GetName(),
					*URTTurnLogLibrary::DescribeEntry(Bypassed)), FRTLogSubject::Unit(Units[i]));
			}
		}
	}

	// Riduzione dichiarata dalle reazioni attivate (`Action.Deflect` e le reazioni d'eroe che ne riusano la
	// semantica): una reazione si attiva UNA volta, quindi vale sul colpo che l'ha innescata.
	//
	// ⚠️ RESTA un delta di primo colpo, e quindi porta ancora il difetto che [D-292] ha tolto alla Guardia:
	// `Deflect` e' -20, e su un colpo piu' piccolo l'avanzo si perde. Non e' stato spostato qui perche' e'
	// una REAZIONE — si attiva una volta, sul colpo che l'ha innescata — e trasformarla in un pool e' una
	// decisione sul suo significato, non una correzione. Dichiarato in `#1909`, da scorporare.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i]) { FirstHitDelta[i] += Reactions.DeflectDelta[i]; }
	}

	// `Action.Brace` (CP 5.2): -10 su OGNI danno diretto, non solo sul primo. E' un secondo passaggio con una
	// funzione diversa (`ApplyDamageDelta`, senza il gate "una volta sola"): applicarlo con `ApplyFirstHitDelta`
	// insieme agli altri delta ridurrebbe un colpo e lascerebbe passare interi tutti i successivi.
	TArray<int32> EveryHitDelta;
	EveryHitDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i] && Units[i]->HasStatus(TAG_Status_Braced))
		{
			EveryHitDelta[i] -= URTCombatLibrary::BraceDamageReduction;
		}
	}

	// L'assorbimento della Guardia viene per ULTIMO, dopo i modificatori del danno ([D-292]): il pool copre
	// cio' che resta, non il danno nominale. E' la lettura coerente con «quanto danno la guardia regge»: se
	// assorbisse per primo, `Status.Exposed` ne mangerebbe una parte prima che il difensore la usi.
	TArray<FRTAttack> Attacks = URTCombatResolver::ApplyAbsorptionPool(
		URTCombatResolver::ApplyDamageDelta(
			URTCombatResolver::ApplyFirstHitDelta(URTHexCombatLibrary::ToAttacks(Plan), FirstHitDelta),
			EveryHitDelta),
		GuardPool, bFrontalHit);

	TArray<FRTCellId> AttackSrc;  // cella dell'attaccante per ogni FRTAttack (TurnLog)
	// Parallelo ad `AttackSrc`, e non ridondante con lui: la cella dice DA DOVE, non CHI — e dopo un Dash le
	// due cose divergono (#405). `Attackers` non serve allo scopo: e' deduplicata, quindi non e' parallela.
	TArray<ARTUnit*> AttackActors;
	TArray<ARTUnit*>& Attackers = Ctx.Attackers;
	TArray<int32>& UsedAbilityIndex = Ctx.UsedAbilityIndex;
	// Status inflitti dalle abilita' (bersaglio + tag + durata, in array paralleli).
	TArray<ARTUnit*>& StatusTargets = Ctx.StatusTargets;
	TArray<FGameplayTag>& StatusTags = Ctx.StatusTags;
	TArray<int32>& StatusDurations = Ctx.StatusDurations;
	// Knockback: per ogni bersaglio colpito da un'abilita' con spinta, la cella dell'attaccante, la distanza
	// e quanti attaccanti lo spingono (2+ = forze contraddittorie -> nessuna spinta, deterministico).
	TMap<ARTUnit*, FRTCellId>& KnockFrom = Ctx.KnockFrom;
	TMap<ARTUnit*, int32>& KnockDist = Ctx.KnockDist;
	TMap<ARTUnit*, int32>& KnockCount = Ctx.KnockCount;
	// Quali attaccanti spingono ciascun bersaglio (D-085). Serve perche' `KnockCount` deve contare gli
	// ATTACCANTI e non gli eventi: dal CP 7.1 una sola azione puo' dichiarare due spinte (`Weapon.Impact` su
	// `Phase.PressureJet`), e contarle come due attaccanti attivava «forze contraddittorie» su un duello.
	TMap<ARTUnit*, TSet<int32>> KnockAttackers;
	// Trazione (`Action.Pull`, CP 4.7): stessa disciplina della spinta, array paralleli propri — una
	// direzione INVERTITA (verso chi tira, non lontano da lui) non e' la stessa spinta con un segno cambiato
	// nel dato che la applica.
	TMap<ARTUnit*, FRTCellId>& PullToward = Ctx.PullToward;
	TMap<ARTUnit*, int32>& PullDist = Ctx.PullDist;
	TMap<ARTUnit*, int32>& PullCount = Ctx.PullCount;
	// CON QUALE azione ogni bersaglio e' stato spostato (#307). **Due mappe, non una condivisa**, e per la
	// stessa ragione per cui `KnockFrom`/`KnockDist` e `PullToward`/`PullDist` sono gia' separate qui sopra.
	//
	// Una mappa sola sembrava bastare perche' «un bersaglio spinto da 2+ attaccanti non si sposta affatto»,
	// ma quel filtro conta le spinte e le trazioni SEPARATAMENTE: un bersaglio con una spinta da A e una
	// trazione da B ha `KnockCount == 1` e `PullCount == 1`, passa entrambi i filtri e **si sposta due volte**,
	// scrivendo due voci. Con una chiave sola la seconda `Add` sovrascrive la prima, e la voce della spinta
	// finirebbe per dichiarare l'azione di chi ha TIRATO. Su una PR che esiste per rendere il TurnLog
	// attribuibile sarebbe stato il difetto peggiore possibile: una causa scritta, precisa e falsa.
	TMap<ARTUnit*, FRTDisplacementCause>& PushCause = Ctx.PushCause;
	TMap<ARTUnit*, FRTDisplacementCause>& PullCause = Ctx.PullCause;
	AttackSrc.Reserve(Plan.Hits.Num());
	// IDENTITA' dell'azione per ogni colpo, paralleli ad `Attacks`/`AttackSrc` (CP 11.3, #79). Fino a qui le
	// voci di combattimento uscivano ANONIME: `ERTCombatOutcome` diceva «22 danni, eliminata» senza dire da
	// cosa. Con due attaccanti nello stesso Blast il replay non poteva attribuire il colpo, e la coppia
	// «azione base + profilo» di D-033 restava scritta nel catalogo e assente dalla traccia.
	//
	// Non e' un campo nuovo: `ActionId` e `BaseActionId` esistono gia' nella voce, sono gia' serializzati
	// (formato v3 e v5) e il primo entra gia' nell'hash. Riempirli non muove la versione del formato.
	TArray<FName> AttackActionId;
	TArray<FName> AttackBaseActionId;
	// PRIORITA' intra-fase del colpo (CP 11.3, `#79`, formato v7): il numero che ha deciso in che ORDINE le
	// azioni della stessa fase hanno risolto. Viene dal catalogo, non e' ricalcolato qui.
	TArray<int32> AttackPriority;
	AttackActionId.Reserve(Plan.Hits.Num());
	AttackBaseActionId.Reserve(Plan.Hits.Num());
	AttackPriority.Reserve(Plan.Hits.Num());
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		ARTUnit* Attacker = Units[Hit.AttackerId];
		ARTUnit* Victim = Units[Hit.TargetId];
		const URTActionData* Ability = IntentAbility.IsValidIndex(Hit.IntentIndex) ? IntentAbility[Hit.IntentIndex] : nullptr;
		const bool bHasDef = IntentDefs.IsValidIndex(Hit.IntentIndex);
		AttackSrc.Add(HexUnits[Hit.AttackerId].Cell);
		// `NAME_None` quando l'intento non ha una definizione: e' la stessa convenzione del campo, «non
		// dichiarata», e vale meno di un nome inventato qui.
		AttackActionId.Add(bHasDef ? IntentDefs[Hit.IntentIndex].ActionId : NAME_None);
		AttackBaseActionId.Add(bHasDef ? IntentDefs[Hit.IntentIndex].BaseActionId : NAME_None);
		AttackPriority.Add(bHasDef ? IntentDefs[Hit.IntentIndex].Priority : 0);
		AttackActors.Add(Attacker);

		// `#649` — la DIREZIONE ha annullato una copertura, e adesso la traccia lo dice.
		//
		// Finora `RearHitBypassedCover` — che nel nome porta proprio «Cover» — era emesso **solo** dal ramo
		// della Guard: la copertura scavalcata spariva dentro `EffectiveCoverReduction`, che e' pura. Dal
		// TurnLog un colpo pieno su un bersaglio riparato era indistinguibile da un colpo pieno su un
		// bersaglio scoperto, e il bonus che il bot conta in pianificazione (CP 13.5) non era verificabile.
		//
		// ⚠️ **`Amount` diverge dall'uso che ne fa la voce della Guard**, che ci mette la DIREZIONE del
		// difensore: qui porta i **punti di riduzione scavalcati**, che sono l'unico numero utile a misurare
		// il realizzo. La divergenza e' preesistente — due significati per lo stesso esito — ed e' nominata
		// qui invece di essere risolta: cambiare la voce della Guard e' toccare una traccia gia' spedita, con
		// i suoi test e il suo posto nel corpus golden.
		//
		// Un colpo che scavalca ENTRAMBE le protezioni produce due voci, ed e' corretto: sono due
		// annullamenti distinti dello stesso colpo.
		// `Victim` si guarda, come ogni loop gemello di questa funzione (`:3937`, `:3803`): senza,
		// `AppendLogEntry` scriverebbe `UnitId = 0`, che il suo commento definisce «nessuna unita'
		// dichiarata» — la voce direbbe che il colpo alle spalle e' arrivato a nessuno, mentre l'altro
		// produttore nomina sempre qualcuno. Due voci della stessa `(Category, Outcome)` di nuovo in
		// disaccordo, cioe' cio' che `#1418` esiste per togliere.
		if (Hit.CoverBypassedByFacing > 0 && HexUnits.IsValidIndex(Hit.AttackerId)
			&& HexUnits.IsValidIndex(Hit.TargetId) && Victim)
		{
			FRTTurnLogEntry BypassedCover;
			BypassedCover.Phase = ERTMatchPhase::Blast;
			BypassedCover.Category = ERTLogCategory::Facing;
			BypassedCover.Outcome = static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
			BypassedCover.SrcCell = HexUnits[Hit.AttackerId].Cell;
			BypassedCover.TgtCell = HexUnits[Hit.TargetId].Cell;
			BypassedCover.Amount = Hit.CoverBypassedByFacing;
			// CHI SUBISCE, come l'altro produttore di questo stesso esito (`#1418`). I due erano d'accordo
			// nell'accreditare l'attaccante e sono stati corretti insieme: due voci con la stessa
			// `(Category, Outcome)` che dichiarano unita' di ruolo diverso sono peggio di una sbagliata,
			// perche' chi aggrega non ha modo di sapere quale ha in mano.
			AppendLogEntry(BypassedCover, Victim);
		}

		// Effetti COLLATERALI del colpo (stato, spinta) dagli EVENTI dichiarati dall'azione, non da flag
		// letti qui: e' il motore azioni (epic E4). Il danno resta separato perche' segue una regola sua —
		// si somma per bersaglio e si applica in blocco, cosi' l'ordine dei colpi non cambia l'esito.
		if (bHasDef)
		{
			FRTActionInstance Instance;
			Instance.Def = IntentDefs[Hit.IntentIndex];
			Instance.SourceUnitId = Hit.AttackerId;
			Instance.TargetUnitId = Hit.TargetId;
			Instance.TargetCell = HexUnits[Hit.TargetId].Cell;
			Instance.EventSequence = Plan.Hits.Num();

			for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEvents(Instance))
			{
				switch (Event.Kind)
				{
				case ERTActionEffect::Status:
					// `Status.Marked` NO: l'ha gia' applicato il pass a priorita' qui sopra, che ne conosce
					// anche la squadra. Riapplicarlo ora rimetterebbe in piedi un marchio appena speso —
					// e senza provenienza, quindi inutilizzabile ma visibile nell'HUD.
					if (Event.StatusTag == TAG_Status_Marked) { break; }
					StatusTargets.Add(Victim);
					StatusTags.Add(Event.StatusTag);
					StatusDurations.Add(Event.Amount);
					break;

				case ERTActionEffect::Push:
					// `PushResistance` e' una SOGLIA, non una sottrazione (D-038): chi la possiede regge le
					// spinte fino a quel valore e cede a quelle piu' forti, per intero. La forma da `Action.Guard`,
					// che dal CP 5.2 fa gia' esattamente questo con `GuardResistedPushDistance` — due
					// resistenze alla spinta nello stesso combat con due semantiche diverse sarebbero state la
					// prima cosa da spiegare a chi bilancia (#241).
					//
					// La regola sta QUI, nel punto in cui la distanza viene registrata, e non nei sette
					// produttori di spinta del catalogo: il settimo nascerebbe gia' rotto.
					//
					// Il `Pull` tre casi piu' sotto **non** la applica, ed e' deliberato: il catalogo v0.1 §1
					// riserva la resistenza di Guard alla spinta, e una trazione non viene resistita da chi si
					// e' piantato. Estenderla qui avrebbe reso `PushResistance` piu' forte di `Guard` senza che
					// nessuno lo avesse deciso.
					if (Victim && Event.Amount <= Victim->PushResistance)
					{
						// Spinta assorbita: non si registra, quindi non c'e' niente da risolvere.
						//
						// ⚠️ E' il SESTO modo di non muoversi, e l'unico che `#420` ha lasciato senza voce di
						// TurnLog: `PushResistance` vale `0` su tutto il roster dopo D-075, quindi un
						// `AppendDisplacementResistedEntry` qui sarebbe codice che nessuna partita attraversa e
						// un valore di `ERTDisplacementBlockReason` che nessun test puo' coprire. Se la
						// meccanica si risveglia, la voce va scritta QUI, con un valore nuovo in coda
						// all'enum — non riusando `NoDestination`, che dice una cosa diversa.
						break;
					}
					// D-085 — le spinte si SOMMANO, e il contatore conta gli ATTACCANTI, non gli eventi.
					//
					// Fino a CP 7.1 le due cose coincidevano: nessuna azione del catalogo dichiarava piu' di un
					// `Push`, quindi un evento era un attaccante. `Weapon.Impact` rompe l'equivalenza — accoda un
					// secondo `Push 1` all'attacco base di Phase, che ne ha gia' uno — e con il conteggio per
					// evento il bersaglio finiva nel ramo «forze contraddittorie» qui sotto: **fermo**, con
					// `OpposingForces` nel TurnLog e un solo attaccante in campo. Una causa scritta, precisa e
					// falsa, che e' il difetto peggiore per una traccia che deve essere attribuibile.
					//
					// `KnockDist` accumula invece di sovrascrivere: `TMap::Add` teneva l'ultimo valore, quindi due
					// spinte da 1 ne producevano una da 1. Il catalogo ne vuole **una da 2**, che attraversa la
					// cella intermedia — non due da 1, che valutano la collisione due volte.
					{
						TSet<int32>& PushersOfVictim = KnockAttackers.FindOrAdd(Victim);
						bool bAlreadyPushing = false;
						PushersOfVictim.Add(Hit.AttackerId, &bAlreadyPushing);
						if (!bAlreadyPushing) { KnockCount.FindOrAdd(Victim)++; }
					}
					KnockFrom.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					KnockDist.FindOrAdd(Victim) += Event.Amount;
					PushCause.Add(Victim, bHasDef
						? FRTDisplacementCause{ IntentDefs[Hit.IntentIndex].ActionId,
							IntentDefs[Hit.IntentIndex].BaseActionId, IntentDefs[Hit.IntentIndex].Priority }
						: FRTDisplacementCause{});
					break;

				case ERTActionEffect::Pull:
					PullToward.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					PullDist.Add(Victim, Event.Amount);
					PullCount.FindOrAdd(Victim)++;
					PullCause.Add(Victim, bHasDef
						? FRTDisplacementCause{ IntentDefs[Hit.IntentIndex].ActionId,
							IntentDefs[Hit.IntentIndex].BaseActionId, IntentDefs[Hit.IntentIndex].Priority }
						: FRTDisplacementCause{});
					break;

				default:
					// Il danno e' gia' nel piano dei colpi (Hit.Power); cure e scudi non appartengono a un
					// colpo a segno. Nessun altro effetto ha senso qui: dichiararlo sarebbe un errore di
					// catalogo, non un caso da gestire.
					break;
				}
			}
		}

		// Evento per il playback: colpo Attacker -> Victim (mostrato nel Blast).
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Attack;
		Ev.SourceStableUnitId = Attacker ? Attacker->StableUnitId : 0;
		Ev.TargetStableUnitId = Victim ? Victim->StableUnitId : 0;
		Ev.Amount = Hit.Power;
		ResolvedTimeline.Add(Ev);

		// L'abilita' si consuma una volta per attaccante, anche se il colpo prende piu' bersagli.
		//
		// 🔴 **Il primo colpo con un'ABILITA' da consumare, non il primo colpo** (`#1449`). Un'unita' puo'
		// possedere piu' intenti nello stesso turno, e l'impatto di una carica ne porta uno con
		// `IntentAbilityIndex == INDEX_NONE` — non c'e' niente da consumare, lo scatto l'ha gia' fatto.
		// `Plan.Hits` e' ordinato per `AttackerId` e poi `TargetId`, quindi bastava che la vittima della
		// carica avesse indice minore del bersaglio dell'attacco perche' l'impatto entrasse per primo:
		// `UsedAbilityIndex` riceveva `INDEX_NONE`, il `Contains` impediva di registrare l'attacco vero, e
		// `ConsumeAbility(INDEX_NONE)` usciva subito. L'azione principale non pagava ne' cooldown ne'
		// energia, ed era riutilizzabile ogni turno.
		const int32 AbilityIdx = IntentAbilityIndex.IsValidIndex(Hit.IntentIndex)
			? IntentAbilityIndex[Hit.IntentIndex] : INDEX_NONE;
		const int32 Registrato = Attackers.IndexOfByKey(Attacker);
		if (Registrato == INDEX_NONE)
		{
			Attackers.Add(Attacker);
			UsedAbilityIndex.Add(AbilityIdx);
		}
		else if (UsedAbilityIndex.IsValidIndex(Registrato)
			&& UsedAbilityIndex[Registrato] == INDEX_NONE && AbilityIdx != INDEX_NONE)
		{
			// Registrato prima con un colpo che non consuma nulla: l'abilita' vera prende il suo posto.
			UsedAbilityIndex[Registrato] = AbilityIdx;
		}
	}

	// Contrattacchi (`Action.Counter`, CP 5.2): accodati QUI, dopo che `AttackSrc` e' completo — i due array
	// sono paralleli e il TurnLog li legge per indice, quindi vanno estesi insieme o le voci si disallineano.
	//
	// In coda, non in mezzo: il catalogo descrive il contrattacco come un attacco eseguito "dopo l'attacco
	// ricevuto". `ResolveAttacks` somma comunque per bersaglio sullo stato iniziale, quindi la posizione non
	// cambia il totale; cambia quale colpo conta come "primo" per Guard/Exposed/Deflect, ed e' giusto che sia
	// l'attacco pianificato a consumare quei delta, non un contrattacco arrivato di rimbalzo.
	Attacks.Append(Reactions.CounterAttacks);
	AttackSrc.Append(Reactions.CounterAttackSrc);
	AttackActionId.Append(Reactions.CounterActionId);
	AttackBaseActionId.Append(Reactions.CounterBaseActionId);
	AttackPriority.Append(Reactions.CounterPriority);
	AttackActors.Append(Reactions.CounterAttackActors);

	if (Attacks.Num() == 0)
	{
		// Nessun colpo, ma le cure vanno applicate lo stesso: un supporto che cura fuori da uno scontro e' il
		// caso NORMALE, non un'eccezione. (Difetto trovato da `Actions.Heal.RestoresWithoutExceedingMax`: la
		// prima stesura usciva di qui e la cura spariva in silenzio.)
		ApplyPlannedHeals(HealTargets, HealAmounts, HealSources, HealActors, HealDefs);

		// ⚠️ Qui non si consuma nulla, e non serve piu' che qualcuno lo faccia al posto proprio: `ResolveCombat`
		// chiama `SpendStartedAbilities` **fuori** da questa funzione, quindi anche questa uscita paga cio' che
		// i pass hanno annotato (`#1451` punto 3). Fino al 2026-08-27 questa riga diceva l'opposto — «la paga
		// dove quell'azione vive» — ed era la ragione per cui i punti di consumo erano cinque.
		return;
	}

	const TArray<FRTUnitCombatState> Resolved = URTCombatResolver::ResolveAttacks(States, Attacks);
	AddLogEvent(FString::Printf(TEXT("Blast: %d attacchi"), Attacks.Num()), FRTLogSubject::World());

	// Chi muore in questo Blast (viva PRIMA, morta DOPO): evento Defeated per la morte visiva differita
	// (il playback mostra prima il colpo, poi l'eliminazione).
	TArray<int32> BeforeHP, AfterHP;
	BeforeHP.Reserve(Units.Num());
	AfterHP.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		BeforeHP.Add(States[i].Health);
		AfterHP.Add(Resolved[i].Health);
	}
	for (const int32 Idx : URTCombatLibrary::NewlyDefeated(BeforeHP, AfterHP))
	{
		// 🔴 **La morte e' PUBBLICA**: `World()`, non il soggetto ([D-223], decisione d'autore del
		// 2026-08-28). Un'eliminazione la leggono tutte le squadre, anche chi non vedeva la vittima cadere.
		//
		// ⚠️ **Non e' il vecchio default fail-open che ricompare.** Fino a `#1499` queste righe passavano
		// perche' nessuno aveva dichiarato un soggetto; adesso passano perche' qualcuno ha deciso che
		// devono. La differenza non si vede nell'output e si vede nel codice, ed e' il punto della issue.
		//
		// ⚠️ **Cosa si rivela, e cosa no.** QUESTO testo porta nome e squadra, mai una cella: chi uccide con
		// un'AoE un nemico mai visto scopre che esisteva e che e' caduto, non dove fosse. E dopo la morte
		// non c'e' piu' una posizione da proteggere. Le altre quattro righe di morte seguono la stessa
		// regola: fiamme, scarica e i due annunci del playback.
		//
		// 🔴 **Ma «pubblica» vale per l'ANNUNCIO, non per il racconto del colpo, e c'e' una SESTA riga di
		// morte che resta filtrata.** Il canale derivato produce, per un esito `Lethal`,
		// *«(q,r,L) -> (q,r,L): N danni, eliminata»* (`URTTurnLogLibrary::DescribeEntry`): quella porta DUE
		// celle — attaccante e vittima — e passa dal verdetto congelato della propria voce. Il criterio non
		// e' «di cosa parla la riga» ma «cosa rivela»: e' pubblico CHE un'unita' sia caduta, non COME ne'
		// DA DOVE.
		//
		// ⚠️ E il soggetto di quella voce non e' sempre lo stesso: e' l'ATTACCANTE per il Blast e per la
		// scarica, la VITTIMA per `Status.Burning` e per il danno da terreno. Chi ci lavora sopra lo
		// verifichi invece di dedurlo — sono due convenzioni opposte nello stesso formato di riga.
		AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *Units[Idx]->GetName(), Units[Idx]->TeamId), FRTLogSubject::World());
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Defeated;
		Ev.SourceStableUnitId = Units[Idx]->StableUnitId;
		ResolvedTimeline.Add(Ev);
	}

	// TurnLog: esito di ogni attacco applicato (classificato da stato pre/post).
	// Il bonus di elevazione e' 0 finche' l'ambiente esagonale non lo reintroduce (epic E8/E9): senza dato
	// reale, dichiararlo 0 e' preferibile a leggerlo da un terreno quadrato che non e' piu' nella partita.
	for (int32 a = 0; a < Attacks.Num(); ++a)
	{
		const int32 Idx = Attacks[a].TargetIndex;
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Combat;
		E.Outcome = static_cast<uint8>(URTCombatLibrary::ClassifyCombatOutcome(BeforeHP[Idx], AfterHP[Idx], /*AttackerDmgBonus=*/ 0));
		E.SrcCell = AttackSrc[a];
		E.TgtCell = Units[Idx]->Cell;
		E.Amount = BeforeHP[Idx] - AfterHP[Idx];
		// CHI ha colpito, e di quale azione generica e' un profilo (CP 11.3 · D-033). Gli array sono paralleli
		// ad `Attacks` per costruzione: `AttackSrc` lo era gia', questi due sono stati riempiti e accodati
		// negli stessi due punti. `IsValidIndex` non e' difesa contro un bug ipotetico — e' che `Attacks`
		// passa da `ApplyDamageDelta`, e un giorno un delta potrebbe non conservare la cardinalita'.
		if (AttackActionId.IsValidIndex(a))     { E.ActionId = AttackActionId[a]; }
		if (AttackBaseActionId.IsValidIndex(a)) { E.BaseActionId = AttackBaseActionId[a]; }
		if (AttackPriority.IsValidIndex(a))     { E.Priority = AttackPriority[a]; }
		AppendLogEntry(E, AttackActors.IsValidIndex(a) ? AttackActors[a] : nullptr);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->ApplyCombatState(Resolved[i].Health, Resolved[i].Shield); // solo logico: rimozione visiva differita
	}

	ApplyPlannedHeals(HealTargets, HealAmounts, HealSources, HealActors, HealDefs);

	// Coda della fase: cio' che si applica quando il danno e' risolto e si sa chi e' rimasto in piedi.
	ApplyDisplacements(Ctx);
	MarkAttackerAbilitiesSpent(Ctx);
	ApplyControlStatuses(Ctx);
}


const URTHexMapAsset* ARTTurnManager::GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		return HexMap->GetHexContext(OutOrigin, OutHexSize, OutLayerHeight);
	}

	// Nessuna mappa nel livello: valori neutri. Le unita' restano dove sono, la scala non ha effetto.
	OutOrigin = FVector::ZeroVector;
	OutHexSize = 150.f;
	OutLayerHeight = 250.f;
	return nullptr;
}

FRTTeamKnowledge ARTTurnManager::KnowledgeForTeam(int32 TeamId) const
{
	for (const FRTTeamKnowledge& K : TeamKnowledgeState)
	{
		if (K.TeamId == TeamId) { return K; }
	}
	// Nessuna conoscenza ancora: una vuota, di versione CORRENTE. Non e' un dettaglio — una struttura a
	// versione 0 verrebbe scartata da `Observe` come illeggibile, e la squadra ricomincerebbe da zero a ogni
	// turno senza che nulla lo dichiari.
	FRTTeamKnowledge Empty;
	Empty.TeamId = TeamId;
	return Empty;
}

FRTHexSimUnit ARTTurnManager::MakeSimUnit(int32 Index, const ARTUnit* Unit) const
{
	FRTHexSimUnit SimUnit(Index, Unit->Cell, Unit->GetEffectiveMoveRange(), /*bAlive=*/ true);
	// `Action.Slow` (CP 4.7): +1 al costo di ogni cella, letto FRESCO a ogni costruzione — cosi' uno Slow
	// applicato nel Blast (stesso turno) si riflette gia' sulla fase Move che segue, senza bisogno di
	// ricordare "quando" e' stato applicato.
	SimUnit.MoveCostModifier = Unit->HasStatus(TAG_Status_Slow) ? 1 : 0;
	// Orientamento (CP 16.1): lo si porta perche' e' stato di gioco, e perche' il facing di fine round e'
	// quello di inizio del round dopo senza nessun travaso esplicito.
	SimUnit.Facing = Unit->Facing;
	return SimUnit;
}

void ARTTurnManager::CollectLivingUnits(TArray<ARTUnit*>& OutUnits) const
{
	OutUnits.Reset();

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(const_cast<ARTTurnManager*>(this), ARTUnit::StaticClass(), Actors);
	OutUnits.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (Unit && Unit->IsAlive())
		{
			OutUnits.Add(Unit); // i morti (es. nel Blast) non si muovono e non bloccano
		}
	}

	// ORDINE STABILE PER CELLA, e non e' una rifinitura: senza, l'ORDINE DI SPAWN decide la partita (#990).
	//
	// `GetAllActorsOfClass` restituisce gli Actor nell'ordine in cui il livello li tiene, che non e' un dato
	// di gioco. Da questo array nasce l'identita' delle unita' nello snapshot — l'indice, si veda il commento
	// qui sotto — e `PlanBots` itera proprio questi indici per far decidere i bot. Finche' le decisioni sono
	// indipendenti non si nota niente; appena due bot interagiscono, chi decide per primo cambia l'esito.
	//
	// MISURATO, non temuto (CP 47.5): la stessa partita 2v2 bot-contro-bot, con le stesse unita' sulle stesse
	// celle e inserite in ordine diverso, divergeva al **turno 2** — in un ordine `Riktor.Interposition` si
	// attivava, nell'altro non trovava trigger. Il turno 1 era identico byte per byte, che e' il modo in cui
	// questa classe di difetto passa inosservata: si manifesta quando gli agenti cominciano a interagire.
	//
	// E' lo stesso `Sort` con lo stesso comparatore che `ResolveCombat` applica al proprio array, dove la
	// regola era gia' scritta — *«GetAllActorsOfClass non e' ordinato, e da questo ordine dipendono gli
	// indici»*. Erano due gemelli, e uno solo dei due la rispettava.
	//
	// CADE `RefactorTactics.Match.Autobattle.DeterminismSurvivesUnitPermutation` se questa riga sparisce:
	// verificato per mutazione, non dedotto.
	OutUnits.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });
}

FRTHexSnapshot ARTTurnManager::MakeCurrentSnapshot(TArray<ARTUnit*>& OutUnits) const
{
	FVector Origin; float HexSize; float LayerH;
	const URTHexMapAsset* Map = GetHexContext(Origin, HexSize, LayerH);

	// Le unita' vive in ordine stabile: la raccolta e il sort vivono in `CollectLivingUnits`, perche'
	// `ValidatePlansAtLockIn` ha bisogno delle STESSE unita' nello STESSO ordine e non ha bisogno di niente
	// altro di questo snapshot. Duplicare il sort la' sarebbe stato il modo di farlo divergere.
	CollectLivingUnits(OutUnits);

	// L'identita' e' l'INDICE dell'unita' in OutUnits, un intero stabile — mai un pointer (stessa
	// disciplina del TurnLog). Il chiamante ritrova la propria unita' con OutUnits.IndexOfByKey.
	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Reserve(OutUnits.Num());
	for (int32 i = 0; i < OutUnits.Num(); ++i)
	{
		SimUnits.Add(MakeSimUnit(i, OutUnits[i]));
	}
	FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, SimUnits);
	// La conoscenza di squadra viaggia con la fotografia (CP 13.2), cosi' i consumatori puri — il bot di
	// CP 13.5, la HUD — la leggono dallo snapshot invece di chiederla al TurnManager. E' una COPIA: la
	// memoria che attraversa i turni resta una sola, e sta nel TurnManager.
	//
	// I contatti sono chiavati su `ARTUnit::StableUnitId` e NON sull'indice di questo array: le due
	// numerazioni sono diverse — questo snapshot scarta i morti, quello del Blast no, ed entrambi si
	// riordinano per cella a ogni movimento. Un consumatore che volesse risalire all'Actor deve cercare per
	// `StableUnitId`, mai indicizzare `Snapshot.Units` con `Contacts[].StableUnitId`.
	Snapshot.TeamKnowledge = TeamKnowledgeState;
	return Snapshot;
}

namespace
{
	/** La riduzione che la copertura applica a un colpo deciso a un decision boundary.
	 *
	 *  🔴 **Un colpo di boundary e' un TIRO NORMALE** ([#888], 2026-08-25): usa lo stesso
	 *  `EffectiveCoverReduction` del Blast, quindi eredita copertura **e** facing — chi viene preso fuori
	 *  dall'arco frontale perde il beneficio del muro. Il brief dice che chi arma *«spara con la propria
	 *  arma»*: se l'arma e' la stessa lo sono anche le regole del tiro, e il counterplay del difensore
	 *  resta la **rotta**.
	 *
	 *  ⚠️ La cella del bersaglio e' quella del **micro-step corrente**, ed e' deterministica: il resolver
	 *  non muove in continuo, quindi non esiste l'ambiguita' «cella lasciata o raggiunta» che rendeva la
	 *  domanda difficile finche' la si guardava a parole.
	 *
	 *  `Shape::Single`: un colpo di boundary ha un bersaglio solo. */
	int32 BoundaryCoverReduction(const URTHexMapAsset* Map, const ARTUnit* Attacker,
		const ARTUnit* Target, const FRTCellId& TargetCell)
	{
		if (Map == nullptr || Attacker == nullptr || Target == nullptr) { return 0; }

		FRTHexCombatUnit A;
		A.UnitId = 0;
		A.TeamId = Attacker->TeamId;
		A.Cell = Attacker->Cell;
		A.bAlive = Attacker->IsAlive();
		A.Facing = Attacker->Facing;

		FRTHexCombatUnit T;
		T.UnitId = 1;
		T.TeamId = Target->TeamId;
		// La cella su cui il colpo e' deciso, passata dal chiamante: la predittiva usa la cella
		// BLOCCATA (`Armed.LockedCell`, quella su cui si e' scommesso) perche' al momento del danno
		// il troncamento del movimento non e' ancora avvenuto e `Target->Cell` e' quella di partenza.
		// L'Overwatch passa la cella corrente, che al suo micro-step e' gia' quella giusta.
		T.Cell = TargetCell;
		T.bAlive = Target->IsAlive();
		T.Facing = Target->Facing;

		return URTHexCombatLibrary::EffectiveCoverReduction(Map, A, T, ERTAbilityShape::Single);
	}
}

void ARTTurnManager::ResolvePredictiveBoundary(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units, TArray<FRTHexMoveResult>& Resolved)
{
	if (ArmedPredictions.Num() == 0)
	{
		return;
	}

	// I colpi armati diventano dati puri: il pointer del tiratore ridiventa un INDICE, e l'ostilita' — che
	// e' l'unica cosa che il mondo sa e lo strato puro no — si risolve qui, una volta sola.
	TArray<FRTPredictiveShot> Shots;
	TArray<int32> ArmedIndexForShot; // per ritrovare l'armamento a cui ogni esito appartiene
	for (int32 a = 0; a < ArmedPredictions.Num(); ++a)
	{
		const FRTArmedPrediction& Armed = ArmedPredictions[a];
		ARTUnit* Shooter = Armed.Shooter.Get();
		if (!IsValid(Shooter) || !Shooter->IsAlive())
		{
			continue; // chi e' caduto nel Blast non spara: il colpo muore con lui, in silenzio
		}

		const int32 ShooterIdx = Units.IndexOfByKey(Shooter);
		if (ShooterIdx == INDEX_NONE)
		{
			continue;
		}

		FRTPredictiveShot Shot;
		Shot.SourceUnitId = ShooterIdx;
		Shot.LockedCell = Armed.LockedCell;
		Shot.ActionId = Armed.ActionId;
		for (int32 u = 0; u < Units.Num(); ++u)
		{
			if (IsValid(Units[u]) && Units[u]->TeamId != Shooter->TeamId)
			{
				Shot.Hostiles.Add(u);
			}
		}
		ArmedIndexForShot.Add(a);
		Shots.Add(Shot);
	}

	// Ripulisce SEMPRE, anche quando nessun colpo e' sopravvissuto: una previsione non vale due turni.
	const TArray<FRTArmedPrediction> Snapshot = ArmedPredictions;
	ArmedPredictions.Reset();

	if (Shots.Num() == 0)
	{
		return;
	}

	const TArray<FRTPredictiveResolution> Outcomes = URTPredictiveLibrary::ResolvePredictions(Shots, Resolved);
	URTPredictiveLibrary::ApplyPredictionsToMoves(Outcomes, Resolved);

	for (const FRTPredictiveResolution& Outcome : Outcomes)
	{
		if (!ArmedIndexForShot.IsValidIndex(Outcome.ShotIndex)) { continue; }
		const FRTArmedPrediction& Armed = Snapshot[ArmedIndexForShot[Outcome.ShotIndex]];
		ARTUnit* Shooter = Armed.Shooter.Get();
		if (!IsValid(Shooter)) { continue; }

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Move;
		Entry.Category = ERTLogCategory::Predictive;
		Entry.ActionId = Armed.ActionId;
		Entry.BaseActionId = Armed.BaseActionId;
		Entry.SrcCell = Shooter->Cell;
		// La cella BLOCCATA anche sul whiff: e' li' che si e' scommesso, ed e' l'informazione che serve per
		// capire il turno. Una voce che dicesse solo «a vuoto» non insegnerebbe niente a chi legge il replay.
		Entry.TgtCell = Armed.LockedCell;

		if (Outcome.bMatched && Units.IsValidIndex(Outcome.VictimUnitId))
		{
			ARTUnit* Victim = Units[Outcome.VictimUnitId];
			const int32 Reduction = BoundaryCoverReduction(Map, Shooter, Victim, Armed.LockedCell);
			const int32 Dealt = FMath::Max(0, Armed.Damage - Reduction);
			// `Direct`: e' un colpo al decision boundary, e passa dallo scudo base come ogni colpo.
			const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Dealt, ERTDamageSource::Direct,
				Victim->Shield, Victim->GetTemporaryShield(), Victim->Health);
			Victim->ApplyCombatState(Result.Health, Result.Shield);

			Entry.Outcome = static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched);
			// Il danno EFFETTIVO, non quello dichiarato: con la copertura i due divergono (#888), e scrivere
			// `Armed.Damage` qui rimetterebbe nel TurnLog autorevole un danno mai inflitto — il difetto che
			// il commento qui sopra registra come gia' corretto una volta.
			Entry.Amount = Dealt;
			AppendLogEntry(Entry, Shooter);

			AddLogEvent(FString::Printf(TEXT("%s: previsione azzeccata, %d danni a %s"),
				*Shooter->GetName(), Armed.Damage, *Victim->GetName()), FRTLogSubject::Unit(Shooter));
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed);
			Entry.Amount = 0;
			AppendLogEntry(Entry, Shooter);

			// Il whiff si SENTE: e' il `Misplay / Failure State` di D-032, e tacerlo lo renderebbe
			// indistinguibile da un turno in cui nessuno ha dichiarato niente.
			AddLogEvent(FString::Printf(TEXT("%s: previsione a vuoto, nessuno e' entrato"), *Shooter->GetName()), FRTLogSubject::Unit(Shooter));
		}
	}
}

FRTReactionWindowView ARTTurnManager::MakeReactionWindowView(const FRTReactionOpportunity& Opportunity,
	int32 OwnerTeamId, int32 ObserverTeamId) const
{
	// L'unico contenuto di questo metodo e' il valore che una funzione pura non puo' avere: la durata
	// autorevole. Tutto il resto — privacy, cardinalita', forma delle opzioni — resta nella libreria, che
	// per questo si verifica senza mondo.
	return URTReactionWindowLibrary::FilterWindowForTeam(ObserverTeamId, OwnerTeamId, Opportunity,
		FastReactionDuration);
}

FRTReactionDecision ARTTurnManager::AskReactionDecision(const FRTReactionOpportunity& Opportunity,
	int32 OwnerUnitId, bool bOwnerIsBot) const
{
	// Cardinalita' <= 1: nessuna finestra si apre e non si chiede niente a nessuno (ADR-0004 §2). Il caso
	// arriva davvero — una condizione dichiarata che filtri via tutti i bersagli lascia il solo `HOLD` — ed e'
	// cosi' che il regime *Conditional* emerge dai dati invece che da un enum di policy parallelo.
	//
	// ⚠️ **I sei ripieghi di questa funzione applicano `SafeResponse(Opportunity)` e non piu' la costante
	// `HoldResponse()`** (2026-08-19, fetta 3 di E14.7). Per l'Overwatch **non cambia un esito**: `HOLD` e'
	// sempre fra le sue risposte legali e `SafeResponse` la preferisce a qualunque altra. Cambia per il
	// `Brace` di [D-047], che chiama la propria scelta sicura `Hold Ground`: con la costante, ognuno dei sei
	// ripieghi avrebbe applicato una risposta **fuori** dalle sue `AllowedResponses` — cioe' un
	// `IsResponseAllowed` falso su una decisione presa dal resolver stesso.
	if (!URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity))
	{
		// **Perche' non c'era scelta**, non solo che non c'era (#583, [D-109]).
		//
		// Un'opportunity dell'Overwatch nasce **solo** dove qualcuno e' entrato nella zona, e
		// `BuildOverwatchTriggers` le da' un `FireResponse` per ogni bersaglio che SODDISFA la condizione, piu'
		// `HOLD` sempre. Quindi una finestra dell'Overwatch rimasta col solo `HOLD` significa una cosa sola:
		// c'erano bersagli e la condizione dichiarata li ha esclusi tutti.
		//
		// ⚠️ **Si deduce da `AllowedResponses`, non dai bersagli**: `FRTReactionOpportunity` porta la chiave e
		// le risposte, mentre `TargetUnitIds` vive sul trigger, che qui non arriva. Ed e' meglio cosi' — la
		// deduzione usa quello che il decisore vede davvero.
		//
		// ⚠️ **Non e' il caso di una finestra che nasce con una risposta sola per costruzione**: il profilo base
		// del `Brace` porta la propria risposta sicura dal catalogo (`Hold Ground`), non il `HOLD`
		// dell'Overwatch, e resta `HoldImmediate`. Il confronto e' con `HoldResponse()`, cioe' con la funzione
		// che quel valore lo SCRIVE — non con una costante riscritta qui.
		const bool bCollassoDaCondizione =
			Opportunity.AllowedResponses.Num() == 1
			&& Opportunity.AllowedResponses[0] == URTReactionOpportunityLibrary::HoldResponse();
		return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
			bCollassoDaCondizione
				? ERTReactionDecisionOutcome::CollapsedByCondition
				: ERTReactionDecisionOutcome::Immediate);
	}

	// --- La TRACCIA, se questa e' una ri-simulazione (`#886`) --------------------------------------------
	//
	// Sta QUI e non una riga piu' su: sopra c'e' il collasso di [D-109], che e' una funzione pura dello stato
	// e si RICALCOLA. Per quelle finestre la traccia contiene `HoldImmediate`, che non e' la scelta di
	// nessuno — e' la constatazione che non c'era scelta — e rileggerlo farebbe dipendere una regola del gioco
	// dalla traccia. Sta qui e non una riga piu' giu' perche' una risposta REGISTRATA batte il decisore
	// corrente: sostituisce *chi decide*, non *se c'e' da decidere*.
	if (RecordedDecisions.Num() > 0)
	{
		const FString Key = URTReactionOpportunityLibrary::DeriveOpportunityId(Opportunity.Key);
		if (const FRTReactionDecision* Recorded = RecordedDecisions.Find(Key))
		{
			// Rifiutata anche se registrata, e non e' diffidenza verso la traccia: se una risposta che allora
			// era legale oggi non lo e', le due esecuzioni hanno smesso di combaciare. Trattarla come un
			// normale `HoldRejected` di gioco mascherebbe un difetto del resolver da esito di partita.
			if (!URTReactionOpportunityLibrary::IsResponseAllowed(Opportunity, Recorded->Response))
			{
				ConsumedDecisionKeys.Add(Key);
				VerificationDivergences.Add(FString::Printf(
					TEXT("risposta registrata illegale nella ri-simulazione: '%s' non e' fra le AllowedResponses della finestra %s"),
					*Recorded->Response, *Key));
				return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
					ERTReactionDecisionOutcome::Rejected);
			}

			ConsumedDecisionKeys.Add(Key);
			return *Recorded;
		}

		// La ri-simulazione apre una finestra che la traccia non copre. L'esito applicato resta un `HOLD` —
		// una finestra va pur risolta, e sparare qui spenderebbe una carica su un disaccordo — ma il fatto
		// che ci sia stato un disaccordo NON si perde: e' [D-170], il giudizio sulla verifica esce da un
		// canale che non e' il TurnLog. Senza questa riga il caso sarebbe indistinguibile da «nessuno ha
		// risposto», cioe' da un successo.
		VerificationDivergences.Add(FString::Printf(
			TEXT("finestra non coperta dalla traccia: nessuna risposta registrata per %s"), *Key));
		return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
			ERTReactionDecisionOutcome::NoDecider);
	}

	// Il decisore INIETTATO ha la precedenza su tutto, bot compreso: e' il punto di sostituzione, e un test
	// che ne collega uno deve poter scriptare anche le risposte di un'unita' del bot.
	if (!ReactionDecider.IsBound())
	{
		// Il bot decide da se', con la sola opportunity: la firma di `DecideReactionResponse` non gli lascia
		// altro. Non passa dal delegate perche' non e' una configurazione — e' cio' che un'unita' del bot
		// **e'**, e collegarlo a BeginPlay lo renderebbe scollegabile per sbaglio.
		if (bOwnerIsBot)
		{
			const FString BotResponse = URTHexBotLibrary::DecideReactionResponse(Opportunity);
			// Passa dalla stessa validazione di una risposta esterna. Non e' diffidenza verso il bot: e' che
			// la legalita' di una risposta dev'essere decisa in UN posto, o due politiche diverse potrebbero
			// applicare risposte che l'altra rifiuta.
			if (!URTReactionOpportunityLibrary::IsResponseAllowed(Opportunity, BotResponse))
			{
				return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
					ERTReactionDecisionOutcome::Rejected);
			}
			// Stessa ragione del decisore iniettato — `Chosen` — e la risposta viaggia accanto (`#1118`).
			// Qui viveva la seconda chiamata a `ClassifyChosenResponse`, e il suo commento diceva che una
			// funzione sola evitava due regole divergenti: aveva ragione, e la separazione la rende
			// superflua. Non c'e' piu' niente da classificare, quindi non c'e' piu' niente che diverga.
			return FRTReactionDecision(BotResponse, ERTReactionDecisionOutcome::Chosen);
		}

		// Un'unita' umana senza UI: la finestra esiste e nessuno puo' rispondere. Fail-closed nel verso
		// giusto — senza decisore la charge non si spende. Il contrario, sparare per default, spenderebbe una
		// risorsa irreversibile per una configurazione mancante. La UI e' CP 14.6 (`#166`).
		return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
			ERTReactionDecisionOutcome::NoDecider);
	}

	// ⚠️ **Qui non si aspetta.** L'unica cosa che questa riga fa e' chiedere e ricevere: nessun `Sleep`,
	// nessun `Delay`, nessun timer, nessuna Timeline. Chi decide in fretta — il bot, un decisore di test —
	// risponde subito; una UI umana (CP 14.6) rispondera' invece con una stringa vuota fino a che il
	// giocatore non ha scelto, e sara' l'orchestratore a richiamare. Il risultato LOGICO non dipende dal
	// tempo reale in nessuno dei due casi, ed e' precisamente cio' che `Reactions.NoResolverWait` protegge.
	const FString Response = ReactionDecider.Execute(Opportunity, OwnerUnitId);

	// Vuota = «non ho risposto». Non e' un errore: e' la scadenza, e cosa valga allo scadere lo dice una
	// funzione pura, non questo `if`.
	if (Response.IsEmpty())
	{
		return URTReactionOpportunityLibrary::DecisionOnTimeout(Opportunity);
	}

	// Risposta STALE o inventata: rifiutata, e sostituita dal default. Non si applica «quello che voleva
	// dire» — una risposta che nomina un bersaglio non piu' offerto e' una decisione presa su un mondo che
	// non c'e' piu'.
	if (!URTReactionOpportunityLibrary::IsResponseAllowed(Opportunity, Response))
	{
		return FRTReactionDecision(URTReactionOpportunityLibrary::SafeResponse(Opportunity),
			ERTReactionDecisionOutcome::Rejected);
	}

	// La RAGIONE e' una sola — ha scelto — e **quale** risposta sia lo dice `Response`, che viaggia nella
	// decisione e finisce in `FRTTurnLogEntry::ReactionResponse` (`#1118`).
	//
	// 🔴 **Qui viveva `ClassifyChosenResponse`, che restituiva tre esiti diversi** — `FireChosen`,
	// `HoldChosen`, `ResponseChosen` — leggendo la risposta per **dedurne** un valore d'enum. Era la
	// conflazione in forma di funzione: la risposta c'era gia', e veniva compressa in un `uint8` che poi
	// tre consumatori decomprimevano ciascuno a modo suo. Chi vuole sapere se si e' sparato chiede
	// `FireResponseTarget(Response)`, che e' la stessa domanda senza il giro.
	return FRTReactionDecision(Response, ERTReactionDecisionOutcome::Chosen);
}

void ARTTurnManager::ArmRecordedReactionDecisions(const TArray<FRTTurnLogEntry>& TraceEntries)
{
	RecordedDecisions.Reset();
	ConsumedDecisionKeys.Reset();
	VerificationDivergences.Reset();

	for (const FRTTurnLogEntry& Entry : TraceEntries)
	{
		if (Entry.Category != ERTLogCategory::ReactionDecision || Entry.OpportunityId.IsEmpty())
		{
			continue;
		}

		const ERTReactionDecisionOutcome Outcome = static_cast<ERTReactionDecisionOutcome>(Entry.Outcome);

		// ⛔ Il collasso NON entra nella mappa. Quelle finestre non arrivano mai fin qui — le precede il gate
		// di cardinalita' — quindi una voce di collasso caricata resterebbe non consumata a fine corsa e
		// verrebbe segnalata come orfana su una ri-simulazione riuscita. Scartarla in ingresso e' anche cio'
		// che la rende inapplicabile per errore a una finestra che non e' la sua.
		//
		// ⚠️ **Sono DUE esiti dal 2026-08-23** (#583): `HoldCollapsedByCondition` per il collasso da condizione
		// dichiarata, `HoldImmediate` per la finestra che nasce con una risposta sola. Entrambi si ricalcolano
		// come funzione pura dello stato, quindi entrambi si scartano qui — e dimenticarne uno non produce un
		// errore di compilazione ma una **risposta orfana**, che e' come il test `CollapsedWindowIgnoresTheTrace`
		// l'ha trovato: «nessuna finestra ha reclamato T2|P4|M0|U1|action.overwatch|S0».
		if (Outcome == ERTReactionDecisionOutcome::Immediate
			|| Outcome == ERTReactionDecisionOutcome::CollapsedByCondition)
		{
			continue;
		}

		// La risposta si LEGGE se la voce la porta, e si ricostruisce altrimenti.
		//
		// 🔴 **La ricostruzione da sola ha smesso di bastare con [D-047], e non era incompleta: era
		// SBAGLIATA.** Regge finche' il vocabolario e' chiuso — l'Overwatch offre `FIRE:<id>` e `HOLD`, quindi
		// l'esito basta a dire quale delle due sia stata applicata. Il `Brace` apre finestre le cui risposte
		// vengono dal **catalogo** (`Hold Ground`, `SIDESTEP`, e quelle che i profili aggiungeranno): per
		// quelle la ricostruzione produce `HOLD`, che in una finestra di `Brace` **non e' nemmeno una risposta
		// legale**. Il ramo di sopra la rifiuterebbe come «registrata illegale», cioe' accuserebbe la traccia
		// di un difetto del lettore — e il replay divergerebbe applicando la scelta sicura al posto dello
		// scarto.
		//
		// ⚠️ **Il campo vuoto NON e' un dato mancante**: e' la dichiarazione che la risposta e' derivabile, ed
		// e' cosi' che ogni traccia scritta prima della v10 continua a significare esattamente cio' che
		// significava. L'Overwatch non scrive il token e non cambia di una riga.
		// ⚠️ **Dalla v11 il campo e' SEMPRE pieno** (`#1118`): la risposta non si deduce piu' dall'esito,
		// perche' l'esito ha smesso di nominarla. Il ripiego qui sotto serve alle tracce v2..v10 che
		// `MigrateReactionDecisionToV11` non ha potuto riempire — e produce esattamente cio' che quei byte
		// dicevano, perche' fino alla v10 il vocabolario era chiuso e la risposta ERA derivabile.
		const FString Response = !Entry.ReactionResponse.IsEmpty()
			? Entry.ReactionResponse
			: FString(URTReactionOpportunityLibrary::HoldResponse());

		// ⚠️ Due voci con la STESSA chiave: `Add` sovrascriverebbe, e una risposta andrebbe persa senza che
		// nessuno lo dica — cioe' il difetto che questa issue esiste per prevenire, spostato di un anello.
		// Il caso non e' teorico: `FRTReactionOpportunityKey` non porta l'istanza della reaction, e
		// `ResolveReactionBoundary` lo dichiara — due Overwatch della stessa unita' nello stesso micro-step
		// ricadrebbero sulla stessa chiave. Oggi non si produce (un'unita' pianifica una sola abilita' per
		// turno); il giorno in cui si producesse, la traccia diventerebbe ambigua e va **detto**, non
		// arrotondato. La PRIMA vince: l'ordine della traccia e' canonico (`EntryLess`), quindi «la prima»
		// e' una regola e non un caso.
		if (const FRTReactionDecision* Existing = RecordedDecisions.Find(Entry.OpportunityId))
		{
			VerificationDivergences.Add(FString::Printf(
				TEXT("traccia ambigua: due risposte registrate per %s ('%s' tenuta, '%s' scartata)"),
				*Entry.OpportunityId, *Existing->Response, *Response));
			continue;
		}

		RecordedDecisions.Add(Entry.OpportunityId, FRTReactionDecision(Response, Outcome));
	}
}

void ARTTurnManager::ReportOrphanRecordedDecisions()
{
	// Ordinato per chiave prima di riportare: `TMap` non garantisce l'ordine di iterazione, e un verdetto che
	// cambia riga a ogni esecuzione sarebbe un non-determinismo introdotto proprio dal codice che verifica il
	// determinismo.
	TArray<FString> Orphans;
	for (const TPair<FString, FRTReactionDecision>& Pair : RecordedDecisions)
	{
		if (!ConsumedDecisionKeys.Contains(Pair.Key))
		{
			Orphans.Add(Pair.Key);
		}
	}
	Orphans.Sort();

	for (const FString& Key : Orphans)
	{
		VerificationDivergences.Add(FString::Printf(
			TEXT("risposta registrata orfana: nessuna finestra ha reclamato %s"), *Key));
	}
}

void ARTTurnManager::ApplyReactionDecision(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units, FRTMovementResolutionState& State,
	const FRTReactionOpportunity& Opportunity, const FRTReactionDecision& Decision, int32 ArmedIndex)
{
	if (!ArmedOverwatches.IsValidIndex(ArmedIndex))
	{
		return;
	}
	FRTArmedOverwatch& Armed = ArmedOverwatches[ArmedIndex];
	ARTUnit* WatchOwner = Armed.Owner.Get();
	const int32 OwnerIdx = Opportunity.Key.OwnerId;
	if (!IsValid(WatchOwner) || !State.Pos.IsValidIndex(OwnerIdx))
	{
		return;
	}

	// La voce e' COMUNE ai sei esiti, e non solo al `FIRE`: un `HOLD` che non lascia traccia renderebbe
	// indistinguibile «ha scelto di non sparare» da «la finestra non si e' mai aperta», che sono la lettura
	// riuscita e il difetto. E' la stessa ragione per cui `ERTReactionOutcome::NotTriggered` esiste dal CP 5.1.
	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Move;
	Entry.Category = ERTLogCategory::ReactionDecision;
	Entry.Outcome = static_cast<uint8>(Decision.Outcome);
	// 🔴 **Il token si scrive SEMPRE, dalla v11** (`#1118`). Fino alla v10 lo scriveva solo il `Brace`, e
	// per l'Overwatch la risposta si deduceva dall'esito: `FireChosen` -> `FIRE:<bersaglio>`, ogni altro
	// -> `HOLD`. Con l'esito che non nomina piu' la risposta quella deduzione non e' piu' possibile — e non
	// e' una perdita, e' la ragione del cambio: una risposta dedotta e' una risposta che il formato non
	// porta, e `SIDESTEP:<Cella>` non si deduce da nessun `uint8`.
	Entry.ReactionResponse = Decision.Response;
	Entry.ActionId = Armed.ActionId;
	Entry.BaseActionId = Armed.BaseActionId;
	Entry.OpportunityId = URTReactionOpportunityLibrary::DeriveOpportunityId(Opportunity.Key);
	Entry.ReactionInstanceId = ArmedIndex;
	Entry.SrcCell = State.Pos[OwnerIdx];
	Entry.TgtCell = Entry.SrcCell;
	Entry.Priority = URTCatalogLibrary::FindCoreAction(Armed.ActionId).Priority;

	const int32 TargetIdx = URTReactionOpportunityLibrary::FireResponseTarget(Decision.Response);

	// 🔴 **`IsAlive()` e non solo `IsValid()`** (#1158). `ARTUnit::ApplyCombatState` NON distrugge l'Actor alla
	// morte e lo dichiara — «la rimozione VISIVA e la distruzione sono differite … cosi' il colpo mortale resta
	// osservabile» — quindi `IsValid` resta **vero** su un'unita' a 0 HP. Senza questo guard, due Overwatch
	// armati sullo stesso mover producevano due `FIRE`: il secondo colpiva un bersaglio gia' abbattuto dal
	// primo e scriveva `Entry.Amount = Armed.Damage` nel TurnLog **autorevole**, cioe' un danno mai inflitto.
	//
	// I `Triggers` si costruiscono UNA volta prima del ciclo per-opportunity, e l'unico guard per iterazione e'
	// `bCharged` — che e' per **watcher**, non per bersaglio: nessuno dei due impediva il caso.
	//
	// ⚠️ Il predicato e' quello che questo file usa gia' ovunque (`:182` con tanto di commento «un cadavere non
	// vede e non si nasconde»), non un secondo criterio nuovo: la sua assenza qui era un'incoerenza, e nessun
	// commento la difendeva.
	const bool bTargetStanding = Units.IsValidIndex(TargetIdx) && IsValid(Units[TargetIdx])
		&& Units[TargetIdx]->IsAlive();
	// ⚠️ **Si legge dalla RISPOSTA e non dall'esito** (`#1118`): `Chosen` dice che qualcuno ha deciso, non
	// che abbia sparato. `TargetIdx` viene gia' da `FireResponseTarget(Decision.Response)` dieci righe sopra,
	// quindi la domanda «e' un FIRE?» e' la stessa che ha prodotto quell'indice — e la si fa una volta sola.
	const bool bFireResponse = TargetIdx != INDEX_NONE;
	const bool bFire = bFireResponse && bTargetStanding && State.Pos.IsValidIndex(TargetIdx);

	if (!bFire)
	{
		// ⚠️ **Qui finiscono DUE casi diversi, e vanno distinti perche' la charge si comporta diversamente.**
		//
		// (1) `HOLD`, in tutte e cinque le sue forme: si perde l'OPPORTUNITY, non la reaction. `bCharged` resta
		// vero, quindi un micro-step successivo puo' ancora aprire una finestra nuova — ed e' precisamente
		// cio' che rende possibile il bait: lascio passare il tank perche' penso che dietro arrivi di meglio.
		//
		// (2) `FIRE` scelto su un bersaglio **gia' abbattuto** (#1158). Non e' un `HOLD`: il giocatore ha
		// premuto, e il log deve continuare a dirlo — `Entry.Outcome` resta `FireChosen`. Cio' che non deve
		// dire e' un danno mai inflitto, quindi `Entry.Amount` resta a zero e non si tronca nessun movimento
		// (il bersaglio non si muove piu' comunque).
		//
		// 🔴 **La charge in questo secondo caso si spende, ed e' STATUS QUO — non una decisione presa qui.**
		// Prima di #1158 il ramo `FIRE` girava per intero anche sul bersaglio a terra, e `Armed.bCharged = false`
		// con esso. Se il watcher debba spenderla o conservarla e' una domanda di **regola**, dichiarata fuori
		// scope dalla issue e da decidere dall'owner di ADR-0004: le due letture — *ha sparato* contro *non
		// c'era piu' niente da colpire* — sono entrambe difendibili. Conservarla qui sarebbe rispondere di
		// iniziativa, e per giunta cambiando il comportamento osservabile insieme alla correzione del log.
		if (bFireResponse && !bTargetStanding)
		{
			Armed.bCharged = false;

			// 🔴 **Il bersaglio va nominato anche quando non lo si colpisce**, e ometterlo rompeva il
			// round-trip della traccia consegnato da `#886`. `ArmRecordedReactionDecisions` RICOSTRUISCE la
			// risposta dal TurnLog — `FireResponse(Entry.SelectedTargetUnitId)` per ogni `FireChosen` — quindi
			// una voce senza bersaglio produce `"FIRE:-1"`, che `IsResponseAllowed` rifiuta: la
			// ri-simulazione registrerebbe una divergenza spuria, tornerebbe `HoldRejected` e cambierebbe
			// l'hash del turno rispetto alla partita originale.
			// ⚠️ Prima di #1158 il caso non esisteva perche' il ramo `FIRE` girava per intero e assegnava
			// questo campo; separando i due rami il campo va assegnato **due volte**, non una. Trovato da una
			// code review, non da un test: nessuno rieseguiva come Verifier una partita con due `FIRE`.
			Entry.SelectedTargetUnitId = TargetIdx;
		}
		AppendLogEntry(Entry, WatchOwner);
		return;
	}

	ARTUnit* Target = Units[TargetIdx];
	const int32 Reduction = BoundaryCoverReduction(Map, WatchOwner, Target, Target->Cell);
	const int32 Dealt = FMath::Max(0, Armed.Damage - Reduction);
	// `Direct`: colpo di Overwatch. Stessa natura del boundary shot qui sopra.
	const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Dealt, ERTDamageSource::Direct,
		Target->Shield, Target->GetTemporaryShield(), Target->Health);
	Target->ApplyCombatState(Result.Health, Result.Shield);

	// La charge si spende QUI e in nessun altro punto: `Charges = 1` (ADR-0004 §8). Da questo momento il
	// watcher non entra piu' fra quelli costruiti al micro-step successivo — `bArmed` falso e' il
	// `ReactionStillArmed` della condizione di trigger.
	Armed.bCharged = false;

	// E il movimento residuo si TRONCA, dentro il calcolo. E' la meta' del `FIRE` che il DoD chiede per nome:
	// il bersaglio resta nella cella raggiunta, e le collisioni dei micro-step successivi cambiano di
	// conseguenza perche' quell'unita' non e' piu' dove sarebbe arrivata.
	URTHexSimLibrary::StopUnitInPlace(State, TargetIdx, ERTMoveOutcome::StoppedByOverwatch);

	Entry.Amount = Dealt;  // il danno EFFETTIVO: con la copertura diverge da quello dichiarato (#888)
	Entry.SelectedTargetUnitId = TargetIdx;
	Entry.TgtCell = State.Pos[TargetIdx];
	AppendLogEntry(Entry, WatchOwner);

	AddLogEvent(FString::Printf(TEXT("%s: overwatch su %s, %d danni e movimento troncato"),
		*WatchOwner->GetName(), *Target->GetName(), Armed.Damage), FRTLogSubject::Unit(WatchOwner));
}

void ARTTurnManager::ResolveReactionBoundary(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units,
	FRTMovementResolutionState& State, const TArray<int32>& MovedUnitIds, int32 MicroStepIndex)
{
	// Fail-closed su tutti e tre: senza mappa non c'e' LOS (quindi nessun trigger), senza Overwatch armati non
	// c'e' chi reagisce, e senza nessuno che si sia mosso non c'e' l'ingresso in una cella controllata — che
	// e' l'evento, non la presenza.
	if (!Map || ArmedOverwatches.Num() == 0 || MovedUnitIds.Num() == 0)
	{
		return;
	}

	// --- 1. I WATCHER, derivati dallo stato CORRENTE -------------------------------------------------------
	//
	// Si ricostruiscono a ogni micro-step invece di essere tenuti: la cella del proprietario cambia se anche
	// lui si sta muovendo, e la conoscenza di squadra cambia perche' il bersaglio si sta avvicinando. Un
	// watcher costruito una volta nel Prep avrebbe la LOS di tre celle fa.
	TArray<FRTOverwatchWatcher> Watchers;
	TArray<int32> ArmedIndexForWatcher; // per ritrovare l'armamento a cui ogni trigger appartiene
	for (int32 a = 0; a < ArmedOverwatches.Num(); ++a)
	{
		const FRTArmedOverwatch& Armed = ArmedOverwatches[a];
		ARTUnit* WatchOwner = Armed.Owner.Get();

		// `bCharged` E' il `ReactionStillArmed` della condizione di trigger (ADR-0004 §6): una reaction gia'
		// spesa non ne apre altre. Chi e' caduto nel Blast non spara, in silenzio, come per la predittiva.
		if (!Armed.bCharged || !IsValid(WatchOwner) || !WatchOwner->IsAlive())
		{
			continue;
		}

		// Cap dei prompt (ADR-0004 §8). Sta QUI, prima di costruire il watcher, e non a valle della
		// decisione: una reaction che ha esaurito le proprie domande non deve nemmeno comparire fra quelle
		// che il resolver valuta — altrimenti il lavoro si farebbe comunque, e il cap sarebbe una tenda
		// davanti a un calcolo gia' avvenuto.
		if (Armed.PromptsUsed >= URTReactionOpportunityLibrary::MaxPromptsPerReaction())
		{
			continue;
		}
		const int32 OwnerIdx = Units.IndexOfByKey(WatchOwner);
		if (OwnerIdx == INDEX_NONE || !State.Pos.IsValidIndex(OwnerIdx))
		{
			continue;
		}

		// La cella CORRENTE nella risoluzione, non `WatchOwner->Cell`: durante il Move le posizioni vere stanno in
		// `State.Pos` — `PlaceOnCell` le scrive sull'attore solo alla fine. Leggere l'attore darebbe la cella
		// di partenza del turno, e un Overwatch che si e' spostato guarderebbe da dove non e' piu'.
		const FRTCellId OwnerCell = State.Pos[OwnerIdx];

		FRTOverwatchWatcher W;
		W.Zone = URTOffensiveActionLibrary::MakeSuppressiveZone(Map, OwnerIdx, WatchOwner->TeamId, OwnerCell,
			URTHexLibrary::Neighbor(OwnerCell, Armed.Facing), Armed.RangeCells, Armed.Damage);
		W.OwnerCell = OwnerCell;
		W.ReactionDefId = Armed.ActionId;
		W.DeclaredCondition = Armed.Condition;
		W.bArmed = true;

		// I cinque tie-break di ADR-0004 §4. `UnitInitiative` resta 0 perche' un'iniziativa per unita' non
		// esiste in v0.1: non e' un valore inventato, e' un criterio che non discrimina — l'ordine totale lo
		// garantiscono comunque i due successivi. `ReactionInstanceId` e' l'indice nell'armamento, che e'
		// stabile dentro il turno ed e' cio' che distingue due Overwatch della **stessa** unita'.
		W.ReactionPriority = URTCatalogLibrary::FindCoreAction(Armed.ActionId).Priority;
		W.AbilityPriority = W.ReactionPriority;
		W.UnitInitiative = 0;
		W.StableUnitId = WatchOwner->StableUnitId;
		W.ReactionInstanceId = a;

		// Quanto sa la SQUADRA del proprietario di ciascun bersaglio in movimento (E13). Si calcola sulle
		// posizioni correnti degli osservatori, per la stessa ragione di `OwnerCell`. Una chiave assente vale
		// `Hidden`, quindi qui entrano solo i nemici: un alleato non e' un bersaglio e non serve dichiararlo.
		TArray<FRTPerceiver> Observers;
		for (int32 u = 0; u < Units.Num(); ++u)
		{
			if (!IsValid(Units[u]) || !Units[u]->IsAlive() || Units[u]->TeamId != WatchOwner->TeamId) { continue; }
			FRTPerceiver P;
			P.Cell = State.Pos.IsValidIndex(u) ? State.Pos[u] : Units[u]->Cell;
			P.Facing = Units[u]->Facing;
			P.VisionRange = Units[u]->VisionRange;
			Observers.Add(P);
		}
		for (int32 TargetIdx : MovedUnitIds)
		{
			if (!Units.IsValidIndex(TargetIdx) || Units[TargetIdx]->TeamId == WatchOwner->TeamId) { continue; }
			W.TeamAwareness.Add(TargetIdx,
				URTPerceptionLibrary::TeamAwarenessOfCell(Map, Observers, State.Pos[TargetIdx]));
		}

		Watchers.Add(MoveTemp(W));
		ArmedIndexForWatcher.Add(a);
	}
	if (Watchers.Num() == 0)
	{
		return;
	}

	// --- 2. I MOVER: il solo passo APPENA compiuto ---------------------------------------------------------
	//
	// Una cella per mover, non il percorso: passare i percorsi interi farebbe calcolare i trigger di micro-step
	// non ancora avvenuti, e un `FIRE` qui **cambia** quel futuro. L'indice vero del passo viaggia accanto.
	TArray<FRTSuppressionMover> Movers;
	TMap<int32, FRTTargetVitals> Vitals;
	for (int32 TargetIdx : MovedUnitIds)
	{
		if (!Units.IsValidIndex(TargetIdx) || !IsValid(Units[TargetIdx])) { continue; }
		FRTSuppressionMover M;
		M.UnitId = TargetIdx;
		M.TeamId = Units[TargetIdx]->TeamId;
		M.Path = { State.Pos[TargetIdx] };
		Movers.Add(MoveTemp(M));

		// Le vitals servono solo alle condizioni dichiarate ([D-109]), e sono fail-closed: senza il dato, un
		// bersaglio sotto condizione non diventa una risposta legale.
		Vitals.Add(TargetIdx, FRTTargetVitals(Units[TargetIdx]->Health, Units[TargetIdx]->MaxHealth));
	}

	const TArray<FRTOverwatchTrigger> Triggers = URTReactionOpportunityLibrary::BuildOverwatchTriggers(
		Map, TurnNumber, Watchers, Movers, Vitals, MicroStepIndex);

	// --- 3. Per ogni opportunity: finestra, decisione, commit ----------------------------------------------
	for (const FRTOverwatchTrigger& Trigger : Triggers)
	{
		const FRTReactionOpportunity& Opportunity = Trigger.Opportunity;

		// L'armamento a cui questa opportunity appartiene, cercato sul WATCHER e non sull'unita': `OwnerId` e'
		// un indice di unita', e cio' che va ritrovato e' l'armamento.
		//
		// ⚠️ Il confronto NON distingue due Overwatch della stessa unita', e va detto invece che promesso:
		// `ReactionDefId` e' `Action.Overwatch` per entrambi, e `FRTReactionOpportunityKey` non porta
		// l'istanza (i suoi sei campi sono turno, macro-fase, micro-step, proprietario, reaction e `Seq`). Il
		// secondo armamento ricadrebbe sull'indice del primo e verrebbe saltato in silenzio alla riga sotto,
		// trovando `bCharged` gia' falso.
		// Oggi il caso NON si produce — un'unita' pianifica una sola abilita' per turno, quindi
		// `ArmedOverwatches` non ne contiene due dello stesso proprietario. Quando servira', a disambiguare
		// non basta un confronto in piu': serve che `FRTOverwatchTrigger` porti l'indice del watcher da cui
		// nasce, che oggi non ha (i suoi campi sono `Opportunity` e `TargetUnitIds`).
		int32 ArmedIndex = INDEX_NONE;
		for (int32 w = 0; w < Watchers.Num(); ++w)
		{
			if (Watchers[w].Zone.OwnerUnitId == Opportunity.Key.OwnerId
				&& Watchers[w].ReactionDefId == Opportunity.Key.ReactionDefId)
			{
				ArmedIndex = ArmedIndexForWatcher[w];
				break;
			}
		}
		if (!ArmedOverwatches.IsValidIndex(ArmedIndex) || !ArmedOverwatches[ArmedIndex].bCharged)
		{
			// Gia' spesa da un'opportunity precedente **dello stesso micro-step**: piu' watcher possono
			// scattare insieme, e l'ordine totale di ADR-0004 §4 dice quale arriva prima. Il secondo non trova
			// piu' la charge, ed e' corretto — `Charges = 1`.
			continue;
		}

		// Un PROMPT e' una finestra che chiede davvero: si conta prima di chiedere, e solo se c'e' una scelta.
		// Un'opportunity a cardinalita' <= 1 si committa da sola senza interrompere nessuno, e spenderle
		// contro il budget significherebbe far pagare al giocatore una domanda che non gli e' stata posta.
		if (URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity))
		{
			++ArmedOverwatches[ArmedIndex].PromptsUsed;
		}

		const ARTUnit* DecidingOwner = ArmedOverwatches[ArmedIndex].Owner.Get();
		const FRTReactionDecision Decision = AskReactionDecision(Opportunity, Opportunity.Key.OwnerId,
			IsValid(DecidingOwner) && DecidingOwner->bIsBotControlled);
		ApplyReactionDecision(Map, Units, State, Opportunity, Decision, ArmedIndex);
	}
}

void ARTTurnManager::ResolveMovement()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Origin; float HexSize; float LayerH;
	GetHexContext(Origin, HexSize, LayerH);

	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units);

	TArray<TArray<FRTCellId>> Paths;
	Paths.Reserve(Units.Num());
	// Chi e' stato accorciato dalla TOPOLOGIA: il resolver non puo' saperlo (il taglio avviene prima che lui
	// veda il percorso) e classificherebbe `Moved`, vero sul percorso troncato ma falso su cio' che l'unita'
	// aveva pianificato. Lo sa questo ciclo, e lo scrive lui nel log.
	TArray<bool> bStoppedByTopology;
	bStoppedByTopology.Init(false, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];

		// Path del turno: percorso composito (waypoint) se presente e coerente, altrimenti rotta calcolata
		// verso la destinazione singola. La validazione e' AUTOREVOLE e passa dallo strato puro esagonale:
		// FindPathForUnit rispetta costi, blocchi, occupazione e budget, a prescindere da cosa arriva dal client.
		TArray<FRTCellId> Path;
		if (Unit->PlannedPath.Num() >= 2 && Unit->PlannedPath[0] == Unit->Cell)
		{
			Path = Unit->PlannedPath;
		}
		else if (Unit->PlannedCell != Unit->Cell)
		{
			Path = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ i, Unit->PlannedCell).Path;
		}

		// Il percorso e' stato calcolato al momento del click (o impostato direttamente), PRIMA che il Blast
		// di QUESTO turno potesse radicare o rallentare l'unita' (`Action.Root`/`Action.Slow`, CP 4.7). Si
		// TRONCA qui contro il budget FRESCO — non si ricalcola da zero: un ostacolo POSIZIONALE (un'altra
		// unita' che occupa una cella a meta' strada) resta compito di `ResolveHexPaths` sotto, che cammina il
		// percorso passo per passo; qui si intercetta solo "il budget e' cambiato da quando il piano e' stato
		// scritto". Se non e' cambiato, il troncamento non taglia nulla — il percorso `FindPathForUnit` gia'
		// rispettava lo snapshot fresco, quindi qui e' un no-op per costruzione.
		Path = URTHexSimLibrary::TruncatePathToBudget(Snapshot, /*UnitId=*/ i, Path);

		if (Path.Num() < 2)
		{
			Path = { Unit->Cell }; // fermo
		}
		// Ghiaccio: chi finisce il Move su Ice con budget residuo scivola di una cella oltre. La cella extra
		// e' aggiunta al PATH, non alla posizione finale: cosi' occupazione e collisioni simultanee restano
		// affare del microstep di ResolveHexPaths, che le risolve gia' in modo indipendente dall'ordine.
		// Solo il Move normale: le mobilita' lineari (ResolveLinearMove) non passano da qui — §5.2 di
		// spec-terreni-e8.md: senza il microstep condiviso lo scivolamento non avrebbe la stessa garanzia
		// sotto collisione simultanea.
		Path = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ i, Path);

		// TOPOLOGIA (CP 9.3): il percorso e' stato validato quando la mappa era un'altra — una porta chiusa
		// nel Blast di QUESTO turno, un muro caduto — e `TruncatePathToBudget` non se ne accorge, perche'
		// guarda il budget. Senza questo taglio un percorso gia' pianificato attraverserebbe un varco che nel
		// frattempo si e' chiuso: il «path fantasma». Il movimento si FERMA all'ultima cella valida
		// (`Fallback.Stop`), non si annulla.
		const int32 PlannedLength = Path.Num();
		Path = URTHexSimLibrary::TruncatePathToTopology(Snapshot, Path);
		bStoppedByTopology[i] = Path.Num() < PlannedLength;

		Paths.Add(Path);
	}

	// RISOLUZIONE SEGMENTATA (CP 14.5). Fino a qui questa riga era `ResolveHexPaths(Paths)`, cioe' un colpo
	// solo. Non lo e' piu' perche' una finestra di reazione deve poter aprirsi **dentro** il calcolo: se si
	// aprisse a movimento concluso, i micro-step successivi sarebbero gia' stati risolti con un'unita' nelle
	// celle che il colpo le ha appena impedito di raggiungere, e il prompt mostrerebbe una scelta che non
	// cambia piu' niente. E' il motivo per cui `FRTMovementResolutionState` esiste (CP 14.2), scritto nel suo
	// stesso commento: «un Overwatch interattivo deve poter fermare il movimento dentro il calcolo».
	//
	// La via a passi e quella in blocco sono LO STESSO codice — `ResolveHexPaths` e' esattamente questo ciclo
	// — quindi il comportamento senza Overwatch armati e' invariato per costruzione, non per verifica.
	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
	{
		// CHI si e' mosso in questo micro-step, misurato e non dedotto: `Entered` cresce di una cella per ogni
		// unita' che ha davvero avanzato, quindi il confronto col valore precedente e' l'unica lettura che
		// distingue «ha fatto un passo» da «era ferma». Serve perche' un mover fermo non deve poter armare un
		// trigger: l'Overwatch scatta su chi ENTRA nella cella controllata, non su chi ci sta.
		TArray<int32> EnteredBefore;
		EnteredBefore.Init(0, Paths.Num());

		int32 MicroStepIndex = 0;
		while (URTHexSimLibrary::ResolveNextHexMicroStep(State))
		{
			TArray<int32> MovedUnitIds;
			for (int32 i = 0; i < State.Num(); ++i)
			{
				const int32 EnteredNow = State.Results[i].Entered.Num();
				if (EnteredNow > EnteredBefore[i])
				{
					MovedUnitIds.Add(i);
				}
				EnteredBefore[i] = EnteredNow;
			}

			// IL DECISION BOUNDARY. La «sospensione globale» di ADR-0004 §5 e' il fatto che questa chiamata
			// stia fra due micro-step e debba ritornare prima del successivo: nessuna unita' avanza mentre una
			// finestra e' aperta, e non perche' qualcuno le fermi — perche' il ciclo non gira.
			ResolveReactionBoundary(Snapshot.Map, Units, State, MovedUnitIds, MicroStepIndex);
			++MicroStepIndex;
		}
	}
	// Non esegue nulla: il ciclo qui sopra e' uscito perche' `ResolveNextHexMicroStep` ha restituito falso,
	// cioe' quando lo stato era gia' finito e gli `Outcome` gia' scritti. Resta perche' e' la funzione che
	// **dichiara** dove finisce la risoluzione — e perche' se un giorno il ciclo dovesse uscire prima, per un
	// cap sul numero di finestre, questa riga eviterebbe di consegnare risultati a meta'.
	TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::FinishHexMovement(State);

	// Ripulisce SEMPRE, come per le previsioni e per la stessa ragione: un Overwatch non vale due turni. Chi
	// non ha mai sparato ha perso l'investimento — e' il costo-opportunita' che rende la scommessa una
	// scommessa (`brief-azioni-generiche-overwatch.md` §6) — e chi ha sparato ha gia' speso la charge.
	ArmedOverwatches.Reset();

	// BOUNDARY DELLA PREDICTIVE ACTION (E18 CP 18.2). Sta QUI — dopo che le rotte sono calcolate, prima che
	// il log sia costruito e le posizioni applicate — perche' e' l'unico punto in cui esistono entrambe le
	// informazioni che servono: chi ha ATTRAVERSATO una cella e chi ci si e' fermato. Un controllo sulla sola
	// posizione finale mancherebbe chi ci passa sopra e prosegue, che e' il caso normale.
	//
	// Il troncamento avviene prima di `BuildMoveLog` proprio perche' il log dica la verita': una voce Move
	// costruita sulla rotta piena racconterebbe un movimento che non e' avvenuto.
	ResolvePredictiveBoundary(Snapshot.Map, Units, Resolved);

	// TurnLog dagli esiti: la chiave e' la cella di PARTENZA (Paths[i][0]), stabile perche' Cell cambia
	// dopo PlaceOnCell. BuildMoveLog produce una voce per unita' nell'ordine dell'input.
	// Causa dichiarata (#307): questo e' il movimento VOLONTARIO della fase Move. Scatto e spostamento
	// forzato hanno altri produttori e dichiareranno la propria.
	TArray<FRTTurnLogEntry> MoveLog = URTHexSimLibrary::BuildMoveLog(Paths, Resolved, TEXT("Action.Move"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority);
	for (int32 i = 0; i < MoveLog.Num(); ++i)
	{
		// Il reason code della topologia sostituisce quello del resolver solo se l'unita' ha davvero percorso
		// tutto cio' che le restava: se si e' fermata anche per un'unita' o per una cella contesa, quel motivo
		// e' avvenuto DOPO il taglio ed e' la spiegazione piu' vicina a cio' che il giocatore ha visto.
		if (bStoppedByTopology.IsValidIndex(i) && bStoppedByTopology[i]
			&& Resolved[i].Outcome == ERTMoveOutcome::Moved)
		{
			MoveLog[i].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByTopology);
		}
	}
	// In blocco, ma una per una: `Append` bypasserebbe il contesto della v6, ed e' la seconda porta
	// d'ingresso al TurnLog che l'helper deve presidiare quanto la prima.
	// Una voce per unita', nell'ordine dell'input (vedi `BuildMoveLog`): l'indice E' il legame, e per questo
	// il ciclo e' per indice e non per riferimento.
	for (int32 i = 0; i < MoveLog.Num(); ++i)
	{
		AppendLogEntry(MoveLog[i], Units.IsValidIndex(i) ? Units[i] : nullptr);
	}
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		// Il combat log mostra il REASON CODE del TurnLog, con le coordinate assiali: cosi' quel che il
		// giocatore legge e quel che il replay registra sono la stessa cosa, non due descrizioni parallele.
		if (MoveLog.IsValidIndex(i)
			&& (Resolved[i].Outcome == ERTMoveOutcome::BlockedContested || Resolved[i].Outcome == ERTMoveOutcome::BlockedByUnit))
		{
			// Il soggetto e' `Units[i]`, lo stesso che `AppendLogEntry` ha appena scritto nella voce: la
			// riga porta `SrcCell` e `TgtCell` di quella mossa, cioe' esattamente la posizione che una
			// squadra che non lo vede non deve leggere.
			AddLogEvent(FString::Printf(TEXT("%s: %s"),
				*Units[i]->GetName(), *URTTurnLogLibrary::DescribeEntry(MoveLog[i])), FRTLogSubject::Unit(Units[i]));
		}
	}

	// Traccia post-lock: rotte effettivamente percorse (viz del percorso risolto). Catturate PRIMA
	// del placement, cosi' includono la cella di partenza reale.
	//
	// ⚠️ **«Prima del placement» vale anche per gli OSSERVATORI, ed e' il campione dichiarato da [D-223]**:
	// costruiti qui, guardano dalle celle di inizio fase. Un campione per micro-step non esiste — la
	// conoscenza di squadra ha due sole assegnazioni per turno, entrambe per fase — e questo e' il limite
	// scritto nella decisione, non un difetto da riparare qui.
	const TArray<FRTRouteObserverTeam> ObserverTeams = BuildRouteObserverTeams(Units);
	LastMoveRoutes.Reset();
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Route;
			Route.Add(Units[i]->Cell);
			Route.Append(Resolved[i].Entered);

			// Come nel sito del Dash: l'identita' viene da `Units[i]`, mai dall'indice di `LastMoveRoutes`,
			// che salta chi non si e' mosso (`#1497`).
			FRTMoveRoute& Tracked = LastMoveRoutes.AddDefaulted_GetRef();
			Tracked.StableUnitId = Units[i]->StableUnitId;
			Tracked.Cells = Route;

			// Il verdetto di [D-223], una cella alla volta: la traccia porta il tratto OSSERVATO e si
			// tronca dove l'osservatore ha perso il soggetto. Congelato QUI e non letto a valle, perche' al
			// prossimo Planning la conoscenza sara' un'altra e il soggetto potrebbe non esistere piu'.
			FreezeRouteVerdicts(Snapshot.Map, ObserverTeams, Units[i]->TeamId, Route, Tracked.CellVerdicts);

			// Evento per il playback: rotta percorsa (start + celle attraversate) da animare.
			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Move;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.SourceStableUnitId = Units[i]->StableUnitId;
			Ev.Path = Route;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni finali e gli effetti delle celle ATTRAVERSATE (non solo di quella finale).
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i].Final, Origin, HexSize, LayerH);
		ApplyTerrainOnEnterEffects(Snapshot.Map, Units[i], Resolved[i].Entered, ERTMatchPhase::Move);
	}

	// Orientamento di fine Move (CP 16.1, `FacingFinalAfterMove` di D-020). Si deriva dalla rotta EFFETTIVA —
	// partenza piu' celle davvero attraversate — non dal percorso pianificato: un'unita' fermata a meta' strada
	// da una cella contesa guarda dove e' arrivata, non dove voleva andare.
	//
	// Dopo `PlaceOnCell`, quindi la voce di log porta la cella finale come chiave. E' l'ultima scrittura del
	// round: il Move risolve per ultimo, e questo valore persiste nel round successivo.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() == 0)
		{
			continue; // chi non si e' mosso non deriva nessun orientamento
		}

		TArray<FRTCellId> Walked;
		Walked.Reserve(Resolved[i].Entered.Num() + 1);
		Walked.Add(Paths[i].Num() > 0 ? Paths[i][0] : Units[i]->Cell);
		Walked.Append(Resolved[i].Entered);

		FRTHexSimUnit Moved(i, Units[i]->Cell, /*InMoveBudget=*/ 0);
		Moved.Facing = Units[i]->Facing;
		const ERTHexDirection Derived = URTFacingLibrary::FacingFromPath(Walked, Moved.Facing);
		RecordFacingChange(Moved, Derived, ERTFacingOutcome::DerivedFromMove,
			ERTMatchPhase::Move, Units[i]);
		Units[i]->Facing = Moved.Facing;

		// Il Move e' a BUDGET: le rotazioni legali saranno tre (l'ultimo passo e le due adiacenti). Sovrascrive
		// l'eventuale traccia dello scatto perche' il Move risolve dopo ed e' l'ultimo movimento del round.
		Units[i]->MovementStyleThisTurn = ERTMovementStyle::Budget;
		Units[i]->WalkedThisTurn = Walked;
	}

	// Rotazione DICHIARATA in pianificazione (D-020, #291). Ultimo passo del round, DOPO l'orientamento
	// derivato: e' quello il `Current` su cui si misura la legalita' — «una accanto» vuol dire accanto a dove
	// si e' arrivati, non a dove si era partiti. Chi non si e' mosso ha stile `None` e ruota libero: e' il caso
	// «resto fermo e mi giro», che prima di questo passaggio non era esprimibile.
	//
	// Una dichiarazione ILLEGALE viene rifiutata, non corretta verso la legale piu' vicina: in una fase
	// simultanea il giocatore non potrebbe accorgersi della correzione, e si ritroverebbe a subire un
	// orientamento che non ha scelto. Il rifiuto lascia una voce nel TurnLog proprio perche' NON cambia nulla:
	// senza, sarebbe indistinguibile da una dichiarazione mai fatta.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		if (!IsValid(Unit)) { continue; }
		if (!Unit->bDeclaresPlannedFacing || !Unit->IsAlive())
		{
			Unit->ClearDeclaredFacing();
			continue;
		}

		ERTHexDirection Applied = Unit->Facing;
		const bool bLegal = URTFacingLibrary::TryApplyDeclaredFacing(Unit->MovementStyleThisTurn,
			Unit->WalkedThisTurn, Unit->Facing, Unit->PlannedFacing, Applied);

		FRTHexSimUnit Declaring(i, Unit->Cell, /*InMoveBudget=*/ 0);
		Declaring.Facing = Unit->Facing;
		RecordFacingChange(Declaring, Applied,
			bLegal ? ERTFacingOutcome::DeclaredInPlanning : ERTFacingOutcome::DeclarationRejected,
			ERTMatchPhase::Move, Unit);
		Unit->Facing = Declaring.Facing;

		AddLogEvent(FString::Printf(TEXT("%s: rotazione dichiarata %s"), *Unit->GetName(),
			bLegal ? TEXT("applicata") : TEXT("RIFIUTATA (illegale per lo stile di movimento)")), FRTLogSubject::Unit(Unit));
		Unit->ClearDeclaredFacing();
	}

	// NOTA (CP 8.1): il cross-damage delle celle attraversate esiste di nuovo, ma solo per i terreni che
	// DICHIARANO effetti nel catalogo v0.1 — oggi Fire (10 danni + Burning), ShallowWater (Wet) e Smoke
	// (Obscured). Gli altri cinque non hanno OnEnterEffects: attraversarli non fa nulla, per scelta del
	// catalogo e non per un buco del resolver. Gli hazard di FINE turno (danno periodico a chi sosta) restano
	// fuori: li porta il CP 8.x dedicato, non questo.

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());
}

// ===================== Playback della risoluzione (presentazione) =============================

void ARTTurnManager::BeginPlayback()
{
	// Cache della trasformazione della mappa per convertire celle -> mondo durante il playback.
	// Stessa fonte usata da ResolveMovement: risoluzione e playback non possono divergere di scala.
	GetHexContext(PBOrigin, PBCellSize, PBLayerHeight);

	// Deriva le animazioni di movimento e la lista attacchi dagli eventi risolti.
	MoveAnims.Reset();
	PlaybackAttacks.Reset();
	PlaybackDefeated.Reset();
	TSet<ARTUnit*> StartPositioned; // per posizionare il cilindro all'inizio della sua PRIMA fase (Dash prima di Move)
	for (const FRTResolvedEvent& Ev : ResolvedTimeline)
	{
		// L'UNICO punto in cui l'id torna a essere un Actor, ed e' qui perche' qui si comincia ad animare
		// (#1800). `nullptr` significa «non c'e' piu' nessuno da muovere» e cade nei rami che gia'
		// esistevano per `TWeakObjectPtr` scaduto.
		ARTUnit* const Src = UnitByStableId(Ev.SourceStableUnitId);

		if (Ev.Type == ERTResolvedEventType::Move && Src && Ev.Path.Num() >= 2)
		{
			FRTMoveAnim Anim;
			Anim.Unit = Src;
			Anim.Phase = Ev.Phase; // Dash o Move
			Anim.World.Reserve(Ev.Path.Num());
			for (const FRTCellId& C : Ev.Path)
			{
				Anim.World.Add(Src->WorldForCell(C, PBOrigin, PBCellSize, PBLayerHeight));
			}
			// Metti il cilindro all'inizio della sua PRIMA anim (Dash precede Move nella timeline):
			// niente flash sulla cella finale. Un'anim successiva della stessa unita' non ne sposta lo start.
			if (!StartPositioned.Contains(Src))
			{
				Src->SetVisualLocation(Anim.World[0]);
				StartPositioned.Add(Src);
			}
			MoveAnims.Add(MoveTemp(Anim));
		}
		else if (Ev.Type == ERTResolvedEventType::Attack)
		{
			PlaybackAttacks.Add(Ev);
		}
		else if (Ev.Type == ERTResolvedEventType::Defeated)
		{
			PlaybackDefeated.Add(Ev);
		}
	}

	// Fasi attive, in ordine canonico (Prep -> Dash -> Blast -> Move). Cleanup: gia' applicato, nessun beat.
	bool bHasDash = false, bHasMove = false, bHasBlastMove = false;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == ERTMatchPhase::Dash) { bHasDash = true; }
		else if (A.Phase == ERTMatchPhase::Blast) { bHasBlastMove = true; } // spinta (knockback)
		else { bHasMove = true; }
	}
	PlaybackPhases.Reset();
	if (bPrepActiveThisTurn) { PlaybackPhases.Add(ERTMatchPhase::Prep); }
	if (bHasDash) { PlaybackPhases.Add(ERTMatchPhase::Dash); }
	if (PlaybackAttacks.Num() > 0 || bHasBlastMove) { PlaybackPhases.Add(ERTMatchPhase::Blast); }
	if (bHasMove) { PlaybackPhases.Add(ERTMatchPhase::Move); }

	if (PlaybackPhases.Num() == 0)
	{
		ConcludeTurn(); // niente da mostrare
		return;
	}

	// Durata reale = somma delle durate delle fasi effettivamente riprodotte (progress bar coerente).
	// Poi accelerazione per rientrare nel tetto (SpeedMultiplierForCap, logica pura testata).
	float RawTotal = 0.f;
	for (const ERTMatchPhase Ph : PlaybackPhases)
	{
		RawTotal += DurationForPlaybackPhase(Ph);
	}
	PlaybackSpeed = URTPlaybackLibrary::SpeedMultiplierForCap(RawTotal, MaxPlaybackSeconds);
	// La stima mostrata tiene conto anche della velocita' scelta da chi guarda. ⚠️ E' una stima ALL'AVVIO:
	// se la velocita' cambia a risoluzione in corso, la barra resta tarata su quella di partenza. La
	// riproduzione invece segue subito (TickPlayback ricompone a ogni tick) — l'unica cosa che diverge e'
	// il numero mostrato, non il ritmo, e non c'e' nulla di logico che vi dipenda.
	const float StartSpeed = URTPlaybackLibrary::EffectivePlaybackSpeed(ViewerPlaybackSpeed, PlaybackSpeed);
	PlaybackTotalSeconds = (StartSpeed > 0.f) ? (RawTotal / StartSpeed) : RawTotal;
	PlaybackElapsedTotal = 0.f;

	PlaybackPhaseIdx = 0;
	bIsResolving = true;
	SetActorTickEnabled(true);
	AddLogEvent(FString::Printf(TEXT("Risoluzione: %d fasi, ~%.1fs (x%.2f)"),
		PlaybackPhases.Num(), PlaybackTotalSeconds, StartSpeed), FRTLogSubject::World());
	EnterPlaybackPhase();
}

void ARTTurnManager::EnterPlaybackPhase()
{
	PlaybackPhaseElapsed = 0.f;
	AttacksShown = 0;
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	AddLogEvent(FString::Printf(TEXT("Playback fase: %s"), *GetPlaybackPhaseName()), FRTLogSubject::World());
	OnPhasePlaybackStarted.Broadcast(Ph);
	// In corsa deve risultare SOLO chi si muove in QUESTA fase. Senza l'azzeramento, un'unita' che ha scattato
	// nel Dash resta con il flag alzato per tutto il Blast (e il Move) e l'AnimBP la tiene in corsa sul posto:
	// il flag lo spegneva solo FinishPlayback, a risoluzione conclusa.
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Unit.IsValid()) { A.Unit->bIsMovingVisually = false; }
	}

	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid()) { A.Unit->bIsMovingVisually = true; OnUnitMoveStarted.Broadcast(A.Unit.Get()); }
		}
	}
}

void ARTTurnManager::TickPlayback(float DeltaSeconds)
{
	// Composizione RILETTA a ogni tick, non congelata in BeginPlayback: e' cio' che rende la velocita'
	// scelta applicabile DURANTE la risoluzione, e non solo dal turno successivo (CP 47.2, #955).
	// PlaybackSpeed resta il solo termine di cap; ViewerPlaybackSpeed e' la preferenza di chi guarda.
	const float Dt = DeltaSeconds * URTPlaybackLibrary::EffectivePlaybackSpeed(ViewerPlaybackSpeed, PlaybackSpeed);
	PlaybackPhaseElapsed += Dt;
	PlaybackElapsedTotal += Dt;

	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	const float PhaseDur = DurationForPlaybackPhase(Ph);

	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		// Movimento in PARALLELO: i cilindri di QUESTA fase (Dash o Move) scorrono con lo stesso Alpha.
		const float Alpha = (PhaseDur > 0.f) ? FMath::Clamp(PlaybackPhaseElapsed / PhaseDur, 0.f, 1.f) : 1.f;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid())
			{
				A.Unit->SetVisualLocation(URTPlaybackLibrary::InterpolateAlongPath(A.World, Alpha));
			}
		}
	}
	// ⚠️ `if`, NON `else if`: il Blast fa DUE cose insieme — scivolare (knockback, sopra) e rivelare i
	// colpi. Con l'else il secondo ramo era irraggiungibile, perche' il primo cattura gia' Blast, e
	// AttackShowSeconds non aveva alcun effetto: i colpi uscivano tutti nello stesso frame dal blocco
	// di finalizzazione (#911). E' la stessa struttura a due `if` che quel blocco usa piu' sotto.
	if (Ph == ERTMatchPhase::Blast)
	{
		// Rivela i colpi in serie (uno ogni AttackShowSeconds) per leggibilita' del danno.
		const int32 ShouldShow = URTPlaybackLibrary::AttacksToShow(
			PlaybackAttacks.Num(), PlaybackPhaseElapsed, AttackShowSeconds);
		while (AttacksShown < ShouldShow)
		{
			const FRTResolvedEvent& Atk = PlaybackAttacks[AttacksShown];
			ARTUnit* const AtkSrc = UnitByStableId(Atk.SourceStableUnitId);
			ARTUnit* const AtkTgt = UnitByStableId(Atk.TargetStableUnitId);
			AddLogEvent(FString::Printf(TEXT("Colpo: %s -> %s (%d)"),
				AtkSrc ? *AtkSrc->GetName() : TEXT("?"),
				AtkTgt ? *AtkTgt->GetName() : TEXT("(eliminato)"),
				// `FRTLogSubject::Unit` vuole l'Actor e non l'id, e lo dichiara: da un id soltanto il
				// verdetto di [D-223] non si calcola — servono anche squadra e cella.
				Atk.Amount), FRTLogSubject::Unit(AtkSrc));
			if (AtkSrc) { AtkSrc->PlayAttackMontage(); }
			if (AtkTgt) { AtkTgt->PlayHitMontage(); }
			OnAttackResolved.Broadcast(AtkSrc, AtkTgt, Atk.Amount);
			++AttacksShown;
		}
	}

	if (PlaybackPhaseElapsed >= PhaseDur)
	{
		// Finalizza la fase corrente.
		if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
		{
			for (const FRTMoveAnim& A : MoveAnims)
			{
				if (A.Phase == Ph && A.Unit.IsValid() && A.World.Num() > 0)
				{
					A.Unit->SetVisualLocation(A.World.Last());
				}
			}
		}
		if (Ph == ERTMatchPhase::Blast)
		{
			while (AttacksShown < PlaybackAttacks.Num())
			{
				const FRTResolvedEvent& Atk = PlaybackAttacks[AttacksShown];
				ARTUnit* const AtkSrc = UnitByStableId(Atk.SourceStableUnitId);
				ARTUnit* const AtkTgt = UnitByStableId(Atk.TargetStableUnitId);
				if (AtkSrc) { AtkSrc->PlayAttackMontage(); }
				if (AtkTgt) { AtkTgt->PlayHitMontage(); }
				OnAttackResolved.Broadcast(AtkSrc, AtkTgt, Atk.Amount);
				++AttacksShown;
			}
		}

		// Morte visiva differita: le unita' eliminate IN QUESTA fase spariscono ora, dopo che il colpo
		// (Blast) o l'attraversamento (Move) e' stato mostrato. Idempotente (guardia IsHidden).
		for (const FRTResolvedEvent& D : PlaybackDefeated)
		{
			ARTUnit* const DefU = UnitByStableId(D.SourceStableUnitId);
			if (D.Phase == Ph && DefU && !DefU->IsHidden())
			{
				AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *DefU->GetName()), FRTLogSubject::World());
				DefU->HideForDefeat();
				DefU->PlayDefeatMontage();
				OnUnitDefeated.Broadcast(DefU);
			}
		}

		++PlaybackPhaseIdx;
		if (PlaybackPhaseIdx >= PlaybackPhases.Num())
		{
			FinishPlayback();
			return;
		}
		EnterPlaybackPhase();
	}
}

void ARTTurnManager::FinishPlayback()
{
	bIsResolving = false;
	SetActorTickEnabled(false);

	// Snap di sicurezza alle posizioni finali (la cella logica e' gia' quella finale).
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Unit.IsValid())
		{
			A.Unit->SetVisualLocation(A.Unit->WorldForCell(A.Unit->Cell, PBOrigin, PBCellSize, PBLayerHeight));
		}
	}
	// La mesh ATTERRA sul facing logico (CP 16.1). Durante il playback lo yaw interpola verso la direzione di
	// marcia, che e' presentazione; a fine risoluzione deve coincidere con il valore che le REGOLE useranno nel
	// turno dopo — altrimenti il giocatore legge un orientamento e ne subisce un altro quando arriva la difesa
	// direzionale (CP 16.2).
	//
	// Qui e non alla fine dell'animazione: `SkipPlayback` passa da questa funzione, quindi saltare la
	// risoluzione non deve lasciare la figura girata dalla parte sbagliata.
	{
		TArray<AActor*> AllUnitsForFacing;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), AllUnitsForFacing);
		for (AActor* UnitActor : AllUnitsForFacing)
		{
			ARTUnit* U = Cast<ARTUnit>(UnitActor);
			if (!U || !U->IsAlive()) { continue; }

			const FVector Here = U->WorldForCell(U->Cell, PBOrigin, PBCellSize, PBLayerHeight);
			const FRTCellId Ahead = URTHexLibrary::Neighbor(U->Cell, U->Facing);
			const FVector There = U->WorldForCell(Ahead, PBOrigin, PBCellSize, PBLayerHeight);
			U->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Here, There), 0.f));
		}
	}

	// Catch-all: nasconde eventuali eliminati non ancora mostrati (hazard di Cleanup, oppure skip del playback).
	for (const FRTResolvedEvent& D : PlaybackDefeated)
	{
		ARTUnit* const DefU = UnitByStableId(D.SourceStableUnitId);
		if (DefU && !DefU->IsHidden())
		{
			AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *DefU->GetName()), FRTLogSubject::World());
			DefU->HideForDefeat();
			DefU->PlayDefeatMontage();
			OnUnitDefeated.Broadcast(DefU);
		}
	}

	MoveAnims.Reset();
	PlaybackAttacks.Reset();
	PlaybackDefeated.Reset();
	PlaybackPhases.Reset();

	AddLogEvent(FString::Printf(TEXT("Risoluzione completata (%.1fs)"), PlaybackElapsedTotal), FRTLogSubject::World());
	// Fine playback (anche via Skip, che passa di qui): nessuna unita' e' piu' in movimento -> l'AnimBP torna idle.
	{
		TArray<AActor*> AllUnits;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), AllUnits);
		for (AActor* UnitActor : AllUnits)
		{
			if (ARTUnit* U = Cast<ARTUnit>(UnitActor)) { U->bIsMovingVisually = false; }
		}
	}
	OnResolvePlaybackFinished.Broadcast();
	ConcludeTurn();
}

void ARTTurnManager::SkipPlayback()
{
	if (!bIsResolving)
	{
		return;
	}
	PacingCurrent.bPlaybackSkipped = true;
	AddLogEvent(TEXT("Risoluzione: salto"), FRTLogSubject::World());
	FinishPlayback();
}

float ARTTurnManager::DurationForPlaybackPhase(ERTMatchPhase InPhase) const
{
	// Le unita' si muovono in parallelo: alla durata serve il percorso PIU' LUNGO fra quelli riprodotti in
	// questa fase, non la loro somma. E' l'unico dato che il TurnManager possiede e la library no.
	int32 MaxSeg = 0;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == InPhase) { MaxSeg = FMath::Max(MaxSeg, A.World.Num() - 1); }
	}

	// La formula sta in `URTPlaybackLibrary::PhaseDuration`, dove si esercita senza mondo e senza Actor
	// (#1817). Qui resta la sola raccolta degli ingressi.
	return URTPlaybackLibrary::PhaseDuration(InPhase, MaxSeg, PlaybackAttacks.Num(),
		PlaybackCellsPerSecond, AttackShowSeconds, PhaseBeatSeconds);
}

FString ARTTurnManager::GetPlaybackPhaseName() const
{
	if (!bIsResolving || !PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return FString();
	}
	switch (PlaybackPhases[PlaybackPhaseIdx])
	{
	case ERTMatchPhase::Prep:    return TEXT("Prep");
	case ERTMatchPhase::Dash:    return TEXT("Dash");
	case ERTMatchPhase::Blast:   return TEXT("Blast");
	case ERTMatchPhase::Move:    return TEXT("Move");
	case ERTMatchPhase::Cleanup: return TEXT("Cleanup");
	default:                     return TEXT("Risoluzione");
	}
}

float ARTTurnManager::GetPlaybackProgress01() const
{
	if (!bIsResolving || PlaybackTotalSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(PlaybackElapsedTotal / PlaybackTotalSeconds, 0.f, 1.f);
}

// --- Sonda di pacing --------------------------------------------------------------------------
// TELEMETRIA. Nessun valore prodotto qui rientra in una decisione di gioco, nel TurnLog o nel suo hash:
// e' l'unica ragione per cui questo canale puo' permettersi di non essere deterministico.
// Spec: docs/gameplay/spec-pacing-turno.md

void ARTTurnManager::BeginPacingSample()
{
	PacingCurrent = FRTPacingSample();
	PacingCurrent.TurnNumber = TurnNumber;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}
		(Unit->TeamId == 0 ? PacingCurrent.UnitsAliveTeam0 : PacingCurrent.UnitsAliveTeam1)++;

		if (Unit->TeamId != PacingTeamId)
		{
			continue; // ActionsAvailable misura lo spazio di decisione di CHI decide, non di tutti
		}
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			if (Unit->CanUseAbility(I))
			{
				++PacingCurrent.ActionsAvailable;
			}
		}
	}

	PacingPlanningStart = FPlatformTime::Seconds();
	PacingLastInput = PacingPlanningStart;
	bPacingHadInput = false;
	bPacingSampleOpen = true; // da qui l'origine esiste, e i tempi si possono misurare
}

void ARTTurnManager::RecordPlanningInput(ERTPlanningInput Kind)
{
	if (Phase != ERTMatchPhase::Planning)
	{
		return; // un input fuori dalla pianificazione non e' una decisione di turno
	}

	// 🔴 Il SECONDO percorso dello stesso difetto, e il piu' facile da non vedere: `Phase` vale `Planning`
	// per default, quindi la guardia qui sopra NON ferma un TurnManager che non e' mai passato da
	// `BeginPlay`, e `(Now - 0.0) * 1000.0` sfora l'`int32` esattamente come nel lock-in. I contatori di
	// composizione — selezioni, ordini, annullamenti — restano validi: sono conteggi, non tempi.
	const double Now = FPlatformTime::Seconds();
	if (bPacingSampleOpen && !bPacingHadInput)
	{
		bPacingHadInput = true;
		PacingCurrent.MsToFirstInput = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
	}
	PacingLastInput = Now;

	switch (Kind)
	{
	case ERTPlanningInput::Selection: ++PacingCurrent.SelectionCount; break;
	case ERTPlanningInput::Order:     ++PacingCurrent.OrderCount;     break;
	case ERTPlanningInput::Undo:      ++PacingCurrent.UndoCount;      break;
	case ERTPlanningInput::Click:
	default:
		break; // attivita' generica: aggiorna solo i tempi
	}
}

void ARTTurnManager::ClosePacingSample()
{
	PacingCurrent.MsPlayback = FMath::RoundToInt(PlaybackElapsedTotal * 1000.f);

	// Quante finestre di reazione hanno occupato la squadra misurata in QUESTO turno (CP 14.6, `#166`).
	//
	// Si DERIVA dal TurnLog invece di essere contato durante la risoluzione: l'apertura di una finestra e' gia'
	// un fatto registrato, e un contatore parallelo nel resolver sarebbe una seconda verita' che diverge al
	// primo esito nuovo — con l'aggravante di essere uno stato mutabile su un percorso che deve restare
	// deterministico. Qui la misura e' telemetria pura: legge, non decide.
	{
		TSet<int32> Responders;
		TArray<AActor*> UnitActors;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), UnitActors);
		for (AActor* Actor : UnitActors)
		{
			// ⚠️ **Nessun filtro su `IsAlive()`**, a differenza di `BeginPacingSample`: un'unita' caduta
			// DURANTE il turno ha comunque potuto aprire finestre prima di cadere, e scartarla farebbe
			// sparire proprio le attese dei turni piu' concitati — cioe' quelle che tarano il bank.
			if (const ARTUnit* Unit = Cast<ARTUnit>(Actor))
			{
				// 🔴 **Lo `0` non entra**: [D-063] lo riserva a «nessuna unita' dichiarata», e
				// `EnsureMatchRoster` assegna gli id da 1 lasciandolo libero apposta. Un'unita' spawnata
				// DOPO il congelamento del roster lo conserva — e con `0` nel set, una voce di log senza
				// soggetto, o un'evocazione avversaria nella stessa condizione, finirebbe nel bank del
				// giocatore misurato: esattamente la confusione fra squadre che il filtro per responder
				// esiste per impedire ([D-167]).
				if (Unit->TeamId == PacingTeamId && Unit->StableUnitId != 0)
				{
					Responders.Add(Unit->StableUnitId);
				}
			}
		}

		// ⚠️ Nessun filtro per turno, e non e' una dimenticanza: `LockInAndResolve` fa `TurnLog.Reset()`
		// prima di ogni risoluzione, quindi il log contiene **gia' e solo** il turno corrente. Il filtro che
		// stava qui copiava l'intero array — ogni voce con le sue `FString` — per un predicato sempre vero.
		PacingCurrent.ReactionWindowsOpened =
			URTPacingLibrary::CountOpenedReactionWindows(TurnLog, Responders);
	}

	PacingSamples.Add(PacingCurrent);
	if (bRecordPacing)
	{
		AppendPacingRow(PacingCurrent);
	}
	PacingCurrent = FRTPacingSample();
	// Il campione e' chiuso: il prossimo turno misura solo se qualcuno lo riapre. Senza questo, un turno
	// aperto dal timer e uno successivo raggiunto da un altro percorso si misurerebbero entrambi
	// dall'origine del primo.
	bPacingSampleOpen = false;
}

void ARTTurnManager::AppendPacingRow(const FRTPacingSample& Sample)
{
	if (PacingFilePath.IsEmpty())
	{
		// Un file per esecuzione. Si scrive UNA RIGA PER TURNO: un riavvio della partita non perde nulla.
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RT"));
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/ true);
		PacingFilePath = FPaths::Combine(Dir,
			FString::Printf(TEXT("pacing_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"))));
		FFileHelper::SaveStringToFile(URTPacingLibrary::CsvHeader() + LINE_TERMINATOR, *PacingFilePath);
	}
	FFileHelper::SaveStringToFile(URTPacingLibrary::CsvRow(Sample) + LINE_TERMINATOR, *PacingFilePath,
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}
