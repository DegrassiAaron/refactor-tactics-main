// Risoluzione del MOVIMENTO: i momenti che la rendono riprendibile (`#2679`, [D-355]).
//
// 🔑 **Perche' un file.** [#1818] misura `ARTTurnManager` a dodici responsabilita', e la DoD di `#2679`
// vincola `RTTurnManager.cpp` + `.h` a **11.000 righe**: dopo la fetta 1 il margine era di **44**, che la
// fetta 2 mangia per intero. Spostare qui e' la scelta che quella DoD chiedeva di dichiarare, e va fatta
// PRIMA di aggiungere la finestra — o smette di essere una scelta e diventa debito scoperto.
//
// ⛔ **`FinishMovementResolution` e `ResolveReactionBoundary` NON sono qui, ed e' misurato e non una
// svista**: dipendono da helper del namespace anonimo di `RTTurnManager.cpp` che TRE fasi condividono —
// `BuildRouteObserverTeams` e `FreezeRouteVerdicts` (anche `ResolveDash`, `:4586` e `:4608`),
// `BoundaryFacing` (anche `:6274` e `:6871`). Portarli via richiederebbe un header interno che espone
// helper di fasi che questa issue non tocca: un cantiere che non le appartiene. Restano li' finche'
// qualcuno non lo apra per la ragione giusta.
//
// ⛔ **Nessuna riga e' cambiata nello spostamento.** Il commit che introduce comportamento e' un altro:
// tenerli insieme renderebbe impossibile leggere l'uno senza rileggere l'altro.

#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnManagerInternal.h" // gli helper che piu' fasi condividono (#2679)
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Map/RTHexVisionLibrary.h" // DescribeLineOfSight: la RAGIONE del blocco, non una seconda LOS (#2534)
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
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexLedgeLibrary.h" // IsEdgeOpen/FindLandingCell: il vocabolario del bordo (#2401), consumato qui
#include "Pathfinding/RTHexPathLibrary.h" // GraphNeighbors: le celle RAGGIUNGIBILI, non i sei vicini geometrici
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTFacingLibrary.h"
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

/**
 * La risoluzione del movimento in tre momenti (`#2679` fetta 1, [D-355]).
 *
 * 🔑 **Il taglio non introduce comportamento: lo SPOSTA.** Le nove locali che il ciclo dei micro-step
 * attraversa vivevano sullo stack di `ResolveMovement` e ora vivono in `PendingMovement`, che sopravvive
 * al ritorno della funzione. `FRTMovementResolutionState` era gia' sospendibile dal CP 14.2 e lo dichiara;
 * cio' che mancava era solo un posto dove tenerla fra una chiamata e l'altra.
 *
 * ⛔ **Nessuna di queste tre sospende ancora nulla.** Il punto di sospensione — `Suspended` in coda a
 * `ERTMovementAdvanceResult` — arriva con la fetta 2. Qui `ResolveMovement` resta, come composizione delle
 * tre, ed e' il GATE che tiene il comportamento invariante: finche' esiste ed e' l'unico chiamante di
 * produzione, lo split non puo' aver cambiato un esito senza che la suite se ne accorga.
 */
void ARTTurnManager::BeginMovementResolution()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PendingMovement = MakeUnique<FRTMovementResolutionContext>();
	FRTMovementResolutionContext& Ctx = *PendingMovement;
	Ctx.bActive = true;

	GetHexContext(Ctx.Origin, Ctx.HexSize, Ctx.LayerHeight);

	TArray<ARTUnit*> Units;
	Ctx.Snapshot = MakeCurrentSnapshot(Units);

	// Come nel Dash: la fase autoritativa dice cio' che lo snapshot ha registrato (#1970). Una condizione
	// gia' segnalata in questo turno non si ripete — la deduplica sta in `ReportSnapshotOverlaps`.
	ReportSnapshotOverlaps(Ctx.Snapshot);

	Ctx.Paths.Reserve(Units.Num());
	// Chi e' stato accorciato dalla TOPOLOGIA: il resolver non puo' saperlo (il taglio avviene prima che lui
	// veda il percorso) e classificherebbe `Moved`, vero sul percorso troncato ma falso su cio' che l'unita'
	// aveva pianificato. Lo sa questo ciclo, e lo scrive lui nel log.
	Ctx.bStoppedByTopology.Init(false, Units.Num());
	// Chi ha DICHIARATO una destinazione e se l'e' vista negare perche' occupata (#79). Come la topologia
	// qui sopra, e' un fatto che il resolver non puo' vedere, con una differenza: la topologia taglia un
	// percorso che esiste, questa lo azzera, e il resolver riceve `{ Cell }` — indistinguibile da chi non ha
	// mai pianificato. Si popola nel ramo di collasso qui sotto, che e' il punto in cui i tre produttori
	// (player, Scenario Harness, bot) diventano lo stesso caso, e si scrive nel log piu' in basso.
	Ctx.bDeniedByOccupant.Init(false, Units.Num());
	// La destinazione richiesta e negata, per indice. Viaggia accanto al flag e non dentro `Ctx.Paths`: in
	// `Ctx.Paths` sarebbe una cella che qualcuno potrebbe percorrere, e nessuno l'ha percorsa.
	Ctx.DeniedDestination.Init(FRTCellId(), Units.Num());
	// Quanto di ogni percorso il GIOCATORE ha chiesto, e se il terreno voleva portare l'unita' oltre
	// (`#2314`). E' l'informazione che il resolver non puo' ricostruire — riceve un percorso gia' esteso e
	// non sa che l'ultima cella non era pianificata — e che solo questo ciclo possiede, perche' e' qui che
	// lo scivolamento viene applicato. Gliela si passa invece di correggere il suo esito a valle: la
	// correzione nel chiamante era l'approccio di `#2290`, e ne sono usciti tre difetti di correttezza.
	TArray<FRTPlannedMovement> PlannedMoves;
	PlannedMoves.Init(FRTPlannedMovement(), Units.Num());
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
			Path = URTHexSimLibrary::FindPathForUnit(Ctx.Snapshot, /*UnitId=*/ i, Unit->PlannedCell).Path;
		}

		// Il percorso e' stato calcolato al momento del click (o impostato direttamente), PRIMA che il Blast
		// di QUESTO turno potesse radicare o rallentare l'unita' (`Action.Root`/`Action.Slow`, CP 4.7). Si
		// TRONCA qui contro il budget FRESCO — non si ricalcola da zero: un ostacolo POSIZIONALE (un'altra
		// unita' che occupa una cella a meta' strada) resta compito di `ResolveHexPaths` sotto, che cammina il
		// percorso passo per passo; qui si intercetta solo "il budget e' cambiato da quando il piano e' stato
		// scritto". Se non e' cambiato, il troncamento non taglia nulla — il percorso `FindPathForUnit` gia'
		// rispettava lo snapshot fresco, quindi qui e' un no-op per costruzione.
		Path = URTHexSimLibrary::TruncatePathToBudget(Ctx.Snapshot, /*UnitId=*/ i, Path);

		if (Path.Num() < 2)
		{
			// 🔑 **Il punto UNICO in cui i tre produttori collassano, ed e' per questo che la domanda di #79
			// si pone qui e in nessun altro posto.** Player, Scenario Harness e bot arrivano a questa riga
			// con lo stesso stato — nessun percorso percorribile — e da qui in giu' sono indistinguibili;
			// `FinalizeHexMovementOutcomes` li chiamera' tutti `Stayed`, cioe' «non pianificava movimento».
			// Su chi aveva dichiarato una destinazione occupata quella parola e' falsa.
			//
			// ⚠️ **Le due fonti non sono simmetriche, e nessuna delle due copre l'altra.**
			// - Player e harness rifiutano il piano IN PIANIFICAZIONE: il `Pop()` cancella la destinazione,
			//   `PlannedCell` torna a `Cell` e qui non resterebbe niente da leggere. Serve il dato che
			//   l'unita' ha portato fin qui.
			// - Il bot non ha un ramo di rifiuto: dichiara solo `PlannedCell` e il suo percorso fallisce
			//   QUI, dentro la fase Move. La sua destinazione richiesta e' ancora leggibile, e non serve
			//   nessuno stato che sopravviva.
			//
			// «Aveva dichiarato?» ha gia' una sede unica — `HasPlannedNormalMove()` — e non se ne scrive una
			// seconda. Il motivo lo classifica `ClassifyWaypointCell`: nessun secondo vocabolario, e budget,
			// cella bloccata e fuori mappa restano `Stayed` per scope dichiarato della #79.
			if (Unit->bMovePlanRejectedByOccupant)
			{
				Ctx.bDeniedByOccupant[i] = true;
				Ctx.DeniedDestination[i] = Unit->RejectedMoveDestination;
			}
			else if (Unit->HasPlannedNormalMove()
				&& URTHexSimLibrary::ClassifyWaypointCell(Ctx.Snapshot, /*UnitId=*/ i, Unit->PlannedCell)
					== ERTHexWaypointReason::Occupied)
			{
				Ctx.bDeniedByOccupant[i] = true;
				Ctx.DeniedDestination[i] = Unit->PlannedCell;
			}

			Path = { Unit->Cell }; // fermo
		}
		// Ghiaccio: chi finisce il Move su Ice con budget residuo scivola di una cella oltre. La cella extra
		// e' aggiunta al PATH, non alla posizione finale: cosi' occupazione e collisioni simultanee restano
		// affare del microstep di ResolveHexPaths, che le risolve gia' in modo indipendente dall'ordine.
		// Solo il Move normale: le mobilita' lineari (ResolveLinearMove) non passano da qui — §5.2 di
		// spec-terreni-e8.md: senza il microstep condiviso lo scivolamento non avrebbe la stessa garanzia
		// sotto collisione simultanea.
		const int32 LengthBeforeSlide = Path.Num();
		const FRTIceSlideResult Slide = URTHexSimLibrary::ApplyIceSliding(Ctx.Snapshot, /*UnitId=*/ i, Path);
		Path = Slide.Path;

		// TOPOLOGIA (CP 9.3): il percorso e' stato validato quando la mappa era un'altra — una porta chiusa
		// nel Blast di QUESTO turno, un muro caduto — e `TruncatePathToBudget` non se ne accorge, perche'
		// guarda il budget. Senza questo taglio un percorso gia' pianificato attraverserebbe un varco che nel
		// frattempo si e' chiuso: il «path fantasma». Il movimento si FERMA all'ultima cella valida
		// (`Fallback.Stop`), non si annulla.
		// ⚠️ **Si confronta contro la lunghezza PRIMA dello scivolamento, non contro `Path.Num()` di adesso**
		// (#2253). La cella di slide non e' pianificata dal giocatore: se la topologia toglie SOLO quella —
		// basta un bordo non attraversabile accanto al ghiaccio, e `ApplyIceSliding` guarda `bBlocksMovement`
		// della cella, non la percorribilita' del passo — l'unita' arriva esattamente dove aveva chiesto, e
		// dirle «fermo: varco chiuso» sarebbe falso. Con il vecchio confronto lo diceva, e non serviva nemmeno
		// un cambio di topologia a meta' turno perche' accadesse.
		Path = URTHexSimLibrary::TruncatePathToTopology(Ctx.Snapshot, Path);
		Ctx.bStoppedByTopology[i] = Path.Num() < LengthBeforeSlide;

		// Il piano del giocatore e' cio' che resta del percorso PRIMA dello scivolamento, dopo il taglio
		// della topologia: `Min` perche' quel taglio puo' aver accorciato anche la parte pianificata.
		PlannedMoves[i].PlannedLength = FMath::Min(LengthBeforeSlide, Path.Num());
		// ⚠️ **Se la topologia ha tagliato DENTRO il piano, lo scivolamento non e' piu' la domanda.** L'unita'
		// non completa nemmeno cio' che aveva chiesto, la precedenza va al taglio, e il ramo qui sotto scrive
		// `BlockedByTopology`. Senza questa condizione il resolver direbbe «arrivata, scivolamento impedito»
		// a chi si e' fermata davanti a un varco chiuso — vero sul percorso troncato, falso su cio' che
		// l'unita' aveva pianificato: esattamente il difetto che `TruncatePathToTopology` esiste per evitare.
		PlannedMoves[i].bSlideRequested = Slide.bSlideRequested && !Ctx.bStoppedByTopology[i];

		Ctx.Paths.Add(Path);
	}

	// RISOLUZIONE SEGMENTATA (CP 14.5). Fino a qui questa riga era `ResolveHexPaths(Ctx.Paths)`, cioe' un colpo
	// solo. Non lo e' piu' perche' una finestra di reazione deve poter aprirsi **dentro** il calcolo: se si
	// aprisse a movimento concluso, i micro-step successivi sarebbero gia' stati risolti con un'unita' nelle
	// celle che il colpo le ha appena impedito di raggiungere, e il prompt mostrerebbe una scelta che non
	// cambia piu' niente. E' il motivo per cui `FRTMovementResolutionState` esiste (CP 14.2), scritto nel suo
	// stesso commento: «un Overwatch interattivo deve poter fermare il movimento dentro il calcolo».
	//
	// La via a passi e quella in blocco sono LO STESSO codice — `ResolveHexPaths` e' esattamente questo ciclo
	// — quindi il comportamento senza Overwatch armati e' invariato per costruzione, non per verifica.
	Ctx.State = URTHexSimLibrary::BeginHexMovement(Ctx.Paths, TArray<int32>(),
		TArray<bool>(), TArray<bool>(), PlannedMoves);

	// Le unita' passano nel contesto come riferimenti DEBOLI: fra due micro-step, in prospettiva, passa una
	// finestra di reazione. Gli indici di `Ctx.State` sono indici di QUESTO array.
	Ctx.Units.Reserve(Units.Num());
	for (ARTUnit* Unit : Units)
	{
		Ctx.Units.Add(Unit);
	}

	// Nasce qui e non nel ciclo: da `#2679` in poi il ciclo puo' uscire e rientrare, e un contatore
	// dichiarato la' dentro ripartirebbe da zero a ogni rientro.
	Ctx.EnteredBefore.Init(0, Ctx.Paths.Num());
}

ERTMovementAdvanceResult ARTTurnManager::AdvanceMovementResolution()
{
	if (!PendingMovement.IsValid() || !PendingMovement->bActive)
	{
		return ERTMovementAdvanceResult::Finished; // niente da far avanzare: non e' un errore, e' la fine
	}
	FRTMovementResolutionContext& Ctx = *PendingMovement;

	// La vista con puntatori nudi che le firme esistenti richiedono. Ricostruita a ogni chiamata e mai
	// memorizzata: e' proprio la vita di questi puntatori che il contesto esiste per non dare per scontata.
	TArray<ARTUnit*> Units;
	Units.Reserve(Ctx.Units.Num());
	for (const TWeakObjectPtr<ARTUnit>& WeakUnit : Ctx.Units)
	{
		Units.Add(WeakUnit.Get());
	}

	// 🔑 **Si semina dallo STATO, non da zero.** Oggi le due cose coincidono perche' questa funzione gira
	// una volta sola; con la fetta 2 non coincideranno piu', e ripartire da zero rinumererebbe i boundary —
	// cioe' le chiavi di `FRTReactionOpportunityKey`, che e' il difetto che rompe il replay.
	CurrentMicroStepIndex = Ctx.State.MicroStepIndex;
	ON_SCOPE_EXIT{ CurrentMicroStepIndex = INDEX_NONE; };

	// CHI si e' mosso in questo micro-step, misurato e non dedotto: `Entered` cresce di una cella per ogni
	// unita' che ha davvero avanzato, quindi il confronto col valore precedente e' l'unica lettura che
	// distingue «ha fatto un passo» da «era ferma». Serve perche' un mover fermo non deve poter armare un
	// trigger: l'Overwatch scatta su chi ENTRA nella cella controllata, non su chi ci sta.

	// ⚠️ Il contatore e' UNO SOLO e vive sul manager (`#2260`): `AppendLogEntry` lo legge per stampare il
	// boundary sulle voci che nascono qui dentro. Una copia locale — com'era prima — tornerebbe a essere
	// invisibile da li', e l'unico modo di riallinearle sarebbe un secondo contatore da tenere d'accordo
	// col primo. Lo stesso valore alimenta la voce del log e `FRTReactionOpportunityKey`.
	// Il ripristino e' STRUTTURALE, non affidato alla disciplina: senza, ogni voce emessa dopo la
	// risoluzione del movimento erediterebbe l'indice dell'ultima barriera e direbbe di appartenere a un
	// ciclo gia' finito. Un `break` uscirebbe comunque di qui; un `return` futuro, no.
	if (!URTHexSimLibrary::ResolveNextHexMicroStep(Ctx.State))
	{
		return ERTMovementAdvanceResult::Finished;
	}

	{
		TArray<int32> MovedUnitIds;
		for (int32 i = 0; i < Ctx.State.Num(); ++i)
		{
			const int32 EnteredNow = Ctx.State.Results[i].Entered.Num();
			if (EnteredNow > Ctx.EnteredBefore[i])
			{
				MovedUnitIds.Add(i);
			}
			Ctx.EnteredBefore[i] = EnteredNow;
		}

		// IL DECISION BOUNDARY. La «sospensione globale» di ADR-0004 §5 e' il fatto che questa chiamata
		// stia fra due micro-step e debba ritornare prima del successivo: nessuna unita' avanza mentre una
		// finestra e' aperta, e non perche' qualcuno le fermi — perche' il ciclo non gira.
		// 🔑 **Il boundary puo' non tornare.** Se ha aperto una finestra, la resolution attende:
		// nessuna unita' avanza finche' non si chiude, e non perche' qualcuno le fermi — perche'
		// questa funzione ritorna senza risolvere il micro-step successivo. E' la sospensione globale
		// di ADR-0004 §5, che la fetta 1 aveva gratis dalla chiamata sincrona e che qui va conservata
		// di proposito.
		if (ResolveReactionBoundary(Ctx.Snapshot.Map, Units, Ctx.State, MovedUnitIds,
			CurrentMicroStepIndex) == ERTMovementAdvanceResult::Suspended)
		{
			return ERTMovementAdvanceResult::Suspended;
		}

	}

	return ERTMovementAdvanceResult::Advanced;
}

void ARTTurnManager::ResolveMovement()
{
	BeginMovementResolution();

	// ⚠️ **La guardia non e' difensiva: e' il cap che impedisce a un difetto del resolver di appendere
	// l'Editor invece di far fallire un test.** Un micro-step non supera la lunghezza del percorso piu'
	// lungo, e `256` sta due ordini di grandezza sopra qualunque percorso di una mappa 2v2.
	int32 Guard = 0;
	ERTMovementAdvanceResult Step = ERTMovementAdvanceResult::Advanced;
	while (Step == ERTMovementAdvanceResult::Advanced && Guard < 256)
	{
		Step = AdvanceMovementResolution();
		++Guard;
	}
	ensureMsgf(Guard < 256, TEXT("risoluzione del movimento non terminata in 256 micro-step"));

	// ⛔ **Questa e' la via SINCRONA, e una sospensione qui non puo' arrivare**: si sospende solo con
	// `OnReactionWindowOpened` legato, cioe' con una UI che attende — e chi ha una UI non chiama
	// `ResolveMovement`, guida i tre momenti. Se accadesse, concludere applicherebbe una risoluzione a
	// meta': meglio dirlo forte che scoprirlo dal TurnLog.
	if (Step == ERTMovementAdvanceResult::Suspended)
	{
		// ⚠️ **Warning e non `ensure`, e la ragione e' che questo non e' un difetto di runtime**: e' un
		// chiamante configurato male — ha legato `OnReactionWindowOpened` e poi ha chiesto la via sincrona.
		// Un `ensure` qui produce un callstack che l'automation conta come errore, e renderebbe rosso
		// qualunque test che la sospensione la voglia ESERCITARE.
		UE_LOG(LogRT, Warning,
			TEXT("ResolveMovement ha incontrato una finestra aperta: la via sincrona non puo' attenderla. ")
			TEXT("Chi lega OnReactionWindowOpened deve guidare Begin/Advance/Finish."));
	}

	FinishMovementResolution();
}

using namespace RTTurnManagerInternal;

ERTMovementAdvanceResult ARTTurnManager::ResolveReactionBoundary(const URTHexMapAsset* Map,
	const TArray<ARTUnit*>& Units, FRTMovementResolutionState& State, const TArray<int32>& MovedUnitIds,
	int32 MicroStepIndex)
{
	// Fail-closed su tutti e tre: senza mappa non c'e' LOS (quindi nessun trigger), senza Overwatch armati non
	// c'e' chi reagisce, e senza nessuno che si sia mosso non c'e' l'ingresso in una cella controllata — che
	// e' l'evento, non la presenza.
	if (!Map || ArmedOverwatches.Num() == 0 || MovedUnitIds.Num() == 0)
	{
		return ERTMovementAdvanceResult::Advanced;
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
			// **ADR-0008 §2**: un osservatore che sta camminando guarda dove ha appena messo il piede, non
			// dove guardava a inizio fase. La cella era gia' quella del micro-step; il facing no, e i due
			// insieme sono l'arco frontale da cui `TeamAwarenessOfCell` decide se il bersaglio e' `Detected`
			// — cioe' la seconda condizione di trigger di ADR-0004 §6.
			P.Facing = BoundaryFacing(State, u, Units[u]->Facing);
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
		return ERTMovementAdvanceResult::Advanced;
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
	// 🔑 **Appaiare prima, consumare poi** (`#2679` fetta 2). Il ciclo qui sotto non risolve piu' nulla:
	// costruisce le coppie `(opportunity, armamento)` e le deposita nel contesto. A risolverle e'
	// `PumpReactionTriggers`, che puo' fermarsi in mezzo — ed e' il motivo per cui l'appaiamento avviene
	// **tutto adesso**: la lista dei `Watchers` e' costruita dallo stato corrente, e dopo una sospensione
	// sarebbe diversa.
	FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	if (!Ctx)
	{
		return ERTMovementAdvanceResult::Advanced; // fuori da una risoluzione riprendibile non c'e' dove depositare: e' un difetto del chiamante
	}
	Ctx->PendingTriggers.Reset();
	Ctx->NextTrigger = 0;
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
		// ⚠️ **La charge NON si controlla qui, e non piu' da `#2679`.** La verifica sta nel pump, cioe' al
		// momento del consumo: fra l'appaiamento e il turno di questo trigger puo' essersi aperta e chiusa
		// una finestra che ha speso la charge, e un controllo fatto adesso direbbe di uno stato che non e'
		// piu' quello in cui la decisione verra' presa. L'esito e' identico finche' nessuno sospende, ed e'
		// quello che la suite verifica.
		FRTPendingReactionTrigger Pending;
		Pending.Opportunity = Opportunity;
		Pending.ArmedIndex = ArmedIndex;
		Ctx->PendingTriggers.Add(MoveTemp(Pending));
	}

	return PumpReactionTriggers(Map, Units, State);
}

/**
 * Consuma i trigger appaiati da `ResolveReactionBoundary`, uno alla volta, dal punto in cui era rimasto.
 *
 * 🔑 **Esiste perche' il consumo possa FERMARSI.** Oggi non si ferma mai — nessuno apre una finestra che
 * duri — e questa funzione e' il ciclo che stava dentro `ResolveReactionBoundary`, spostato dove potra'
 * uscire e rientrare. Il punto di sospensione si innesta qui, e in nessun altro posto.
 *
 * ⚠️ **La charge si verifica QUI**, non all'appaiamento: piu' watcher possono scattare nello stesso
 * micro-step, e l'ordine totale di ADR-0004 §4 dice quale arriva prima. Il secondo non trova piu' la
 * charge, ed e' corretto — `Charges = 1`.
 */
FString ARTTurnManager::GetOpenReactionWindowId() const
{
	const FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	return Ctx ? Ctx->OpenWindowOpportunityId : FString();
}

void ARTTurnManager::SubmitReactionResponse(const FString& OpportunityId, const FString& Response)
{
	FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	if (!Ctx || Ctx->OpenWindowOpportunityId.IsEmpty())
	{
		return; // nessuna finestra attende: una risposta senza domanda non e' un errore, e' un ritardo
	}

	// ⚠️ **Il gate dell'identita'.** Una risposta che nomina una finestra diversa da quella aperta e'
	// arrivata tardi — la sua e' scaduta, e un'altra si e' aperta nel frattempo. Applicarla significherebbe
	// far decidere il giocatore su un mondo che non c'e' piu': lo stesso difetto che `IsResponseAllowed`
	// rifiuta per le risposte stale, qui su un canale che quella funzione non vede.
	if (Ctx->OpenWindowOpportunityId != OpportunityId)
	{
		return;
	}

	CloseReactionWindow(Response);
}

void ARTTurnManager::ExpireReactionWindow()
{
	FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	if (!Ctx || Ctx->OpenWindowOpportunityId.IsEmpty())
	{
		return;
	}

	// Vuota = «non ho risposto». Cosa valga allo scadere lo dice una funzione pura, non questa: il ramo
	// e' lo stesso che `AskReactionDecision` prende da sempre per il decisore che tace.
	CloseReactionWindow(FString());
}

/**
 * Chiude la finestra aperta applicando `Response` e riprende il consumo dei trigger.
 *
 * 🔑 **La risposta non viene giudicata qui.** Passa da `AskReactionDecision` come ogni altra — legata al
 * decisore per la durata di questa chiamata — perche' la legalita' di una risposta si decide in **un**
 * posto: una seconda politica qui applicherebbe risposte che quella rifiuta.
 *
 * ⛔ **Riprende il MICRO-STEP, non la risoluzione.** I trigger rimasti in questo boundary vengono
 * consumati; far avanzare i micro-step successivi e' di chi chiama `AdvanceMovementResolution`, ed e' la
 * fetta 3. Qui la resolution torna disponibile, non riparte da sola.
 */
void ARTTurnManager::CloseReactionWindow(const FString& Response)
{
	FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	if (!Ctx || !Ctx->PendingTriggers.IsValidIndex(Ctx->NextTrigger - 1))
	{
		return;
	}

	const FRTPendingReactionTrigger Pending = Ctx->PendingTriggers[Ctx->NextTrigger - 1];
	Ctx->OpenWindowOpportunityId.Reset();
	Ctx->OpenWindowElapsed = 0.f;

	TArray<ARTUnit*> Units;
	Units.Reserve(Ctx->Units.Num());
	for (const TWeakObjectPtr<ARTUnit>& WeakUnit : Ctx->Units)
	{
		Units.Add(WeakUnit.Get());
	}

	// Il decisore viene legato per la durata di UNA domanda e poi rimesso com'era. E' il modo di far
	// passare la risposta umana dallo stesso imbuto del bot senza aggiungere un secondo ramo dentro
	// `AskReactionDecision`, che e' `const` e deve restarlo.
	FRTReactionDeciderSignature Previous = ReactionDecider;
	ReactionDecider.BindLambda(
		[Response](const FRTReactionOpportunity&, int32) -> FString { return Response; });

	const FRTReactionDecision Decision = AskReactionDecision(Pending.Opportunity,
		Pending.Opportunity.Key.OwnerId, /*bOwnerIsBot=*/ false);

	ReactionDecider = Previous;

	if (ArmedOverwatches.IsValidIndex(Pending.ArmedIndex))
	{
		ApplyReactionDecision(Ctx->Snapshot.Map, Units, Ctx->State, Pending.Opportunity, Decision,
			Pending.ArmedIndex);
	}

	// I trigger rimasti in questo boundary: possono aprirne un'altra, e allora si torna ad attendere.
	PumpReactionTriggers(Ctx->Snapshot.Map, Units, Ctx->State);
}

ERTMovementAdvanceResult ARTTurnManager::PumpReactionTriggers(const URTHexMapAsset* Map,
	const TArray<ARTUnit*>& Units, FRTMovementResolutionState& State)
{
	FRTMovementResolutionContext* Ctx = PendingMovement.Get();
	if (!Ctx)
	{
		return ERTMovementAdvanceResult::Advanced;
	}

	while (Ctx->PendingTriggers.IsValidIndex(Ctx->NextTrigger))
	{
		// Copiato e non referenziato: `ApplyReactionDecision` puo' toccare lo stato, e da `#2679` in poi
		// fra due giri di questo ciclo puo' passare un frame intero.
		const FRTPendingReactionTrigger Pending = Ctx->PendingTriggers[Ctx->NextTrigger];
		++Ctx->NextTrigger;

		const int32 ArmedIndex = Pending.ArmedIndex;
		if (!ArmedOverwatches.IsValidIndex(ArmedIndex) || !ArmedOverwatches[ArmedIndex].bCharged)
		{
			continue;
		}

		const FRTReactionOpportunity& Opportunity = Pending.Opportunity;

		// Un PROMPT e' una finestra che chiede davvero: si conta prima di chiedere, e solo se c'e' una scelta.
		// Un'opportunity a cardinalita' <= 1 si committa da sola senza interrompere nessuno, e spenderle
		// contro il budget significherebbe far pagare al giocatore una domanda che non gli e' stata posta.
		if (URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity))
		{
			++ArmedOverwatches[ArmedIndex].PromptsUsed;
		}

		const ARTUnit* DecidingOwner = ArmedOverwatches[ArmedIndex].Owner.Get();
		const bool bOwnerIsBot = IsValid(DecidingOwner) && DecidingOwner->bIsBotControlled;

		// --- IL PUNTO DI SOSPENSIONE (`#2679` fetta 2, [D-355]) -----------------------------------------
		//
		// 🔑 **Quattro condizioni, e nessuna e' una politica**: sono i fatti che rendono una finestra
		// interattiva possibile. Manca una qualsiasi e si prende la strada sincrona, che resta identica a
		// se stessa — e' cosi' che bot, test e Verifier non hanno bisogno di un ramo che li nomini.
		//
		// ⛔ **`RecordedDecisions` vuota e' la condizione che protegge il replay**, e sta qui e non altrove:
		// in ri-simulazione il Verifier non ha un decisore, e una finestra aperta non verrebbe chiusa da
		// nessuno. `Replay.Verifier.ResimulationIsDeterministic` si bloccherebbe invece di diventare rosso,
		// che e' il modo peggiore in cui un gate puo' fallire.
		if (OnReactionWindowOpened.IsBound()
			&& !bOwnerIsBot
			&& RecordedDecisions.Num() == 0
			&& URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity))
		{
			const int32 OwnerTeamId = IsValid(DecidingOwner) ? DecidingOwner->TeamId : INDEX_NONE;
			Ctx->OpenWindowOpportunityId = URTReactionOpportunityLibrary::DeriveOpportunityId(Opportunity.Key);
			Ctx->OpenWindowElapsed = 0.f;

			// Il DTO e' sanitizzato per la squadra di chi decide: `MakeReactionWindowView` acquista qui il
			// chiamante di produzione che `RTTurnManager.h:627` dichiarava mancante.
			OnReactionWindowOpened.Execute(
				MakeReactionWindowView(Opportunity, OwnerTeamId, OwnerTeamId), Opportunity.Key.OwnerId);

			return ERTMovementAdvanceResult::Suspended;
		}

		const FRTReactionDecision Decision = AskReactionDecision(Opportunity, Opportunity.Key.OwnerId,
			bOwnerIsBot);
		ApplyReactionDecision(Map, Units, State, Opportunity, Decision, ArmedIndex);
	}

	// Consumati tutti: la lista si svuota, e il boundary successivo riparte da zero.
	Ctx->PendingTriggers.Reset();
	Ctx->NextTrigger = 0;
	return ERTMovementAdvanceResult::Advanced;
}

void ARTTurnManager::FinishMovementResolution()
{
	if (!PendingMovement.IsValid() || !PendingMovement->bActive)
	{
		return; // fail-closed: proseguire una risoluzione che non esiste non e' un no-op da inventare
	}
	FRTMovementResolutionContext& Ctx = *PendingMovement;

	// La vista con puntatori nudi che le firme esistenti richiedono. Ricostruita a ogni chiamata e mai
	// memorizzata: e' proprio la vita di questi puntatori che il contesto esiste per non dare per scontata.
	TArray<ARTUnit*> Units;
	Units.Reserve(Ctx.Units.Num());
	for (const TWeakObjectPtr<ARTUnit>& WeakUnit : Ctx.Units)
	{
		Units.Add(WeakUnit.Get());
	}

	// Non esegue nulla: il ciclo qui sopra e' uscito perche' `ResolveNextHexMicroStep` ha restituito falso,
	// cioe' quando lo stato era gia' finito e gli `Outcome` gia' scritti. Resta perche' e' la funzione che
	// **dichiara** dove finisce la risoluzione — e perche' se un giorno il ciclo dovesse uscire prima, per un
	// cap sul numero di finestre, questa riga eviterebbe di consegnare risultati a meta'.
	TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::FinishHexMovement(Ctx.State);

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
	ResolvePredictiveBoundary(Ctx.Snapshot.Map, Units, Resolved);

	// TurnLog dagli esiti: la chiave e' la cella di PARTENZA (Ctx.Paths[i][0]), stabile perche' Cell cambia
	// dopo PlaceOnCell. BuildMoveLog produce una voce per unita' nell'ordine dell'input.
	// Causa dichiarata (#307): questo e' il movimento VOLONTARIO della fase Move. Scatto e spostamento
	// forzato hanno altri produttori e dichiareranno la propria.
	TArray<FRTTurnLogEntry> MoveLog = URTHexSimLibrary::BuildMoveLog(Ctx.Paths, Resolved, TEXT("Action.Move"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority);

	// 🔑 **Chi e' SCIVOLATO DAVVERO, e non chi lo ha solo chiesto** (`#2253`). Il predicato non e'
	// `bSlideRequested[i]`: fra la richiesta e la fine del Move l'unita' puo' essere fermata dal microstep,
	// perdere la cella contesa per priorita', o essere interrotta da `StoppedByOverwatch` /
	// `StoppedByPrediction`. E' esattamente la distinzione che `#2258` ha gia' dovuto fare per l'esito del
	// log, e le due cose devono restare la STESSA cosa — da cui la LETTURA dell'esito finale qui sotto,
	// invece di una seconda condizione scritta a mano che divergerebbe alla prima modifica. Da `#2314` chi
	// decide e' `FinalizeHexMovementOutcomes`, e questo predicato ne e' un lettore, non un secondo giudice.
	//
	// L'invariante che ne discende e' falsificabile e pinnata da `Status.UnbalancedIffSlid`:
	// *«`Status.Unbalanced` c'e' se e solo se il TurnLog di quel movimento dice `Slid`»*.
	TArray<bool> bSlidThisMove;
	bSlidThisMove.Init(false, MoveLog.Num());

	for (int32 i = 0; i < MoveLog.Num(); ++i)
	{
		// Il reason code della topologia sostituisce quello del resolver solo se l'unita' ha davvero percorso
		// tutto cio' che le restava: se si e' fermata anche per un'unita' o per una cella contesa, quel motivo
		// e' avvenuto DOPO il taglio ed e' la spiegazione piu' vicina a cio' che il giocatore ha visto.
		if (Ctx.bStoppedByTopology.IsValidIndex(i) && Ctx.bStoppedByTopology[i]
			&& Resolved[i].Outcome == ERTMoveOutcome::Moved)
		{
			MoveLog[i].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByTopology);
		}

		// 🔑 **Il movimento negato in PIANIFICAZIONE, che senza questo ramo non lascia traccia** (#79).
		// `Stayed` e' dichiarato «non pianificava movimento (path < 2 celle)»: su chi aveva dichiarato una
		// destinazione e se l'e' vista negare, quella voce non e' incompleta, e' FALSA — ed e' la stessa
		// stringa che legge chi non ha dichiarato niente. La voce SOSTITUISCE `Stayed`, non si aggiunge:
		// `BuildMoveLog` emette una voce per unita' per la fase Move, e due voci `Move` per la stessa unita'
		// nella stessa fase non hanno precedente nel formato.
		//
		// ⚠️ **La guardia e' `Stayed`, e non e' cosmetica.** Un'unita' che si e' mossa (`Moved`), o che e'
		// stata fermata dal resolver per una cella contesa (`BlockedContested`) o dalla topologia, ha un
		// esito avvenuto DOPO la pianificazione, ed e' la spiegazione piu' vicina a cio' che il giocatore ha
		// visto — la stessa precedenza che il ramo qui sopra applica alla topologia. Il rifiuto di
		// pianificazione parla solo quando il turno non ha nient'altro da dire.
		// Ramo indipendente e non un `else`: la guardia `Stayed` lo rende gia' disgiunto da quello della
		// topologia, che chiede `Moved`, e i due si leggono uno per volta invece che come una catena.
		if (Ctx.bDeniedByOccupant.IsValidIndex(i) && Ctx.bDeniedByOccupant[i]
			&& Resolved[i].Outcome == ERTMoveOutcome::Stayed)
		{
			MoveLog[i].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByUnit);
			// La destinazione RICHIESTA, non `Results[i].Final`: quella e' la cella di partenza, e con essa
			// la coppia `SrcCell -> TgtCell` descriverebbe una rotta lunga zero invece di quella negata. E'
			// la stessa scelta di `SupersededByDash`, «la destinazione dichiarata e mai raggiunta».
			MoveLog[i].TgtCell = Ctx.DeniedDestination[i];
			// `Amount` resta quello che `BuildMoveLog` ha scritto: `Entered.Num()`, cioe' `0` per costruzione
			// su chi non si e' mosso. Non si riassegna a mano — un `0` scritto due volte da due posti e' un
			// `0` che qualcuno dovra' tenere d'accordo — ma va detto che qui vale `0` PERCHE' nessuna cella e'
			// stata percorsa, non perche' il campo non si applichi. `ActionId` e `Priority` sono gia' quelli
			// del catalogo, letti da `BuildMoveLog`: nessun valore cablato entra in questo ramo.
		}
		// ⛔ **Il GHIACCIO non ha piu' un ramo qui, ed e' il punto di `#2314`.** C'era, e riscriveva `Moved`
		// in `Slid` confrontando `Resolved[i].Final` con la cella di scivolamento: una guardia POSIZIONALE,
		// che un percorso capace di rivisitare una cella (`{A, B, C, B}`) soddisfa anche a un terzo di
		// strada. E soprattutto non poteva vedere il caso opposto — arrivata e NON scivolata — perche' li'
		// `Resolved[i].Outcome` non e' `Moved` ma il reason del blocco, e la riscrittura non scattava.
		//
		// Ora `Slid` e `SlideBlocked` li scrive `FinalizeHexMovementOutcomes` sul progresso reale dentro
		// `FRTPlannedMovement::PlannedLength`, che e' il solo posto dove l'informazione e' completa. La
		// precedenza fra topologia e scivolamento resta dichiarata e va alla topologia: `bSlideRequested`
		// sopra e' gia' spento quando il taglio ha accorciato il piano.

		// 🔑 **`Status.Unbalanced` si legge dalla voce FINALE, ed e' l'invariante alla lettera.**
		// `Status.UnbalancedIffSlid` dice *«c'e' se e solo se il TurnLog di quel movimento dice `Slid`»*:
		// leggerlo qui — dopo la riscrittura della topologia, sull'esito che verra' davvero scritto — rende
		// la condizione la STESSA cosa che il replay racconta, invece di una seconda condizione da tenere
		// d'accordo con la prima. Prima di `#2314` era il ramo del ghiaccio a fissarlo, e quel ramo non
		// esiste piu'.
		bSlidThisMove[i] = static_cast<ERTMoveOutcome>(MoveLog[i].Outcome) == ERTMoveOutcome::Slid;
	}
	// In blocco, ma una per una: `Append` bypasserebbe il contesto della v6, ed e' la seconda porta
	// d'ingresso al TurnLog che l'helper deve presidiare quanto la prima.
	// Una voce per unita', nell'ordine dell'input (vedi `BuildMoveLog`): l'indice E' il legame, e per questo
	// il ciclo e' per indice e non per riferimento.
	for (int32 i = 0; i < MoveLog.Num(); ++i)
	{
		// ⚠️ **Anche questa gira prima di `PlaceOnCell`, e il suo verdetto si congela sulla cella di
		// PARTENZA** — ma qui, al contrario dei tre siti corretti da `#2142`, non e' evidente che sia
		// sbagliato: `SrcCell` di una voce `Move` **e'** per dichiarazione la cella di partenza, quindi
		// soggetto e voce si accordano. «Chi puo' leggere che quest'unita' si e' mossa» ammette due risposte
		// difendibili e nessuna decisione la sceglie; la traccia della rotta ne ha una terza ancora, un
		// verdetto **per cella** (`FreezeRouteVerdicts`). Aperta in `#2148` invece che risolta qui.
		AppendLogEntry(MoveLog[i], Units.IsValidIndex(i) ? Units[i] : nullptr);
	}
	// ⛔ **Niente `AddLogEvent` per le mosse bloccate**, e la riga che c'era qui non era di troppo fin
	// dall'inizio: e' diventata un duplicato con `#1932`.
	//
	// L'intento — *«il combat log mostra il REASON CODE del TurnLog, con le coordinate assiali, cosi'
	// quel che il giocatore legge e quel che il replay registra sono la stessa cosa»* — resta, e ora lo
	// realizza la derivazione: `ConcludeTurn` rende una riga per voce, e da `#1932` le voci `Move`
	// portano **anche il soggetto**, perche' `Move` e' l'unica categoria in cui `UnitId` e' pure il
	// soggetto grammaticale.
	//
	// 🔴 **E le due copie non erano nemmeno identiche**: questa nominava l'unita' con
	// `Units[i]->GetName()` — l'Actor, cioe' `RTUnit_0` — mentre la derivata usa il nome risolto da
	// `SubjectNamesForLog()`, cioe' `Wraith`. Lo stesso evento arrivava al giocatore due volte,
	// attribuito a due entita' che sembravano diverse. Misurato da
	// `RefactorTactics.UI.BlockedMoveLineIsNotRepeated`, che senza questa rimozione conta `2` (`#1412`).

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
			FreezeRouteVerdicts(Ctx.Snapshot.Map, ObserverTeams, Units[i]->TeamId, Route, Tracked.CellVerdicts);

			// Evento per il playback: rotta percorsa (start + celle attraversate) da animare.
			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Move;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.SourceStableUnitId = Units[i]->StableUnitId;
			Ev.Path = Route;
			// 🔴 Lo STESSO verdetto congelato una riga sopra per la traccia (`#1525`): il modello e la
			// polilinea che lo racconta si troncano allo stesso punto, perche' leggono lo stesso dato.
			Ev.CellVerdicts = Tracked.CellVerdicts;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni finali e gli effetti delle celle ATTRAVERSATE (non solo di quella finale).
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i].Final, Ctx.Origin, Ctx.HexSize, Ctx.LayerHeight);
		ApplyTerrainOnEnterEffects(Ctx.Snapshot.Map, Units[i], Resolved[i].Entered, ERTMatchPhase::Move);
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
		Walked.Add(Ctx.Paths[i].Num() > 0 ? Ctx.Paths[i][0] : Units[i]->Cell);
		Walked.Append(Resolved[i].Entered);

		FRTHexSimUnit Moved(i, Units[i]->Cell, /*InMoveBudget=*/ 0);
		Moved.Facing = Units[i]->Facing;

		// 🔑 **Il canale `Environmental`, che finora era un tipo senza produttore** (`#2253`).
		// `ERTDisplacementCause::Environmental` esisteva da `#726` con il commento che lo motiva — *«una
		// spinta ha una sorgente verso cui girarsi, uno scivolamento no»* — e compariva **solo nei test**:
		// l'unico produttore vivo (`ApplyForcedDisplacement`) scrive `Forced` costante. Lo stesso vale per
		// il suo gemello nel log, `ERTFacingOutcome::KeptOnEnvironmentalDisplacement`, dichiarato e senza
		// un solo sito che lo scrivesse.
		//
		// ⛔ **Perche' l'ultimo passo non puo' derivare l'orientamento.** `FacingFromPath` risponde alla
		// domanda *«dove stavo andando?»*, e per una cella di scivolamento quella domanda non ha soggetto:
		// il passo non e' stato scelto. Derivare da li' significherebbe girare l'unita' verso una
		// direzione che nessuno ha voluto, e per giunta farlo comparire nel replay come `DerivedFromMove`,
		// cioe' attribuendo al giocatore una rotazione del terreno.
		//
		// Le due celle coincidono di proposito: `FacingAfterDisplacement` con causa ambientale ignora la
		// sorgente e lascia l'orientamento invariato — pinnato da
		// `RefactorTactics.Facing.EnvironmentalDisplacementKeepsFacing`, che esisteva prima di questo sito.
		const bool bScivolato = bSlidThisMove.IsValidIndex(i) && bSlidThisMove[i];
		const ERTHexDirection Derived = bScivolato
			? URTFacingLibrary::FacingAfterDisplacement(Units[i]->Cell, Units[i]->Cell,
				ERTDisplacementCause::Environmental, Moved.Facing)
			: URTFacingLibrary::FacingFromPath(Walked, Moved.Facing);
		RecordFacingChange(Moved, Derived,
			bScivolato ? ERTFacingOutcome::KeptOnEnvironmentalDisplacement : ERTFacingOutcome::DerivedFromMove,
			ERTMatchPhase::Move, Units[i]);
		Units[i]->Facing = Moved.Facing;

		// Il Move e' a BUDGET: le rotazioni legali saranno tre (l'ultimo passo e le due adiacenti). Sovrascrive
		// l'eventuale traccia dello scatto perche' il Move risolve dopo ed e' l'ultimo movimento del round.
		Units[i]->MovementStyleThisTurn = ERTMovementStyle::Budget;
		Units[i]->WalkedThisTurn = Walked;
	}

	// STANDUP: chi era `Prone` e si e' MOSSO ha pagato il punto movimento, e si rialza ([D-319]).
	//
	// 🔑 **Il prezzo e' gia' stato pagato prima di qui, e non si sottrae due volte.**
	// `ARTUnit::GetEffectiveMoveRange()` lo toglie dal budget con cui questo Move e' stato pianificato e
	// risolto: questo ciclo non scala niente, registra che e' avvenuto e toglie lo stato.
	//
	// ⚠️ **La condizione e' «si e' mosso», non «e' `Prone`»**: chi resta fermo non paga e resta a terra —
	// e' l'altra meta' della regola, quella che rende `Prone` un costo invece che una tassa. `Entered` e'
	// la misura giusta perche' conta le celle DAVVERO percorse: un'unita' bloccata al primo micro-step non
	// ha speso niente e non si rialza.
	//
	// ⚠️ **Fase `Move`, non `Cleanup`**: e' qui che accade. Vedi la nota su `MakeStatusDeathEntry`, dove il
	// default resta `Cleanup` per non toccare le voci gia' serializzate.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (!IsValid(Units[i]) || !Resolved.IsValidIndex(i) || Resolved[i].Entered.Num() == 0)
		{
			continue;
		}
		if (!Units[i]->HasStatus(TAG_Status_Prone))
		{
			continue;
		}
		Units[i]->RemoveStatus(TAG_Status_Prone);
		FRTTurnLogEntry Rialzato = MakeStatusDeathEntry(TAG_Status_Prone, Units[i]->Cell,
			ERTStatusOutcome::ShakenOff, ERTMatchPhase::Move,
			/*Amount=*/ URTCombatLibrary::StandUpMovePointCost);
		AppendLogEntry(Rialzato, Units[i]);
	}

	// `Status.Unbalanced` a chi e' scivolato ([D-319], `brief-stati-unbalanced-prone.md` §2). Ciclo
	// PROPRIO e non un ramo di quello sopra: quello esce presto su chi non si e' mosso (`continue`), e
	// appendere qui una regola che dipende da un'altra condizione la renderebbe muta il giorno in cui la
	// prima cambia. L'ordine per indice tiene le voci deterministiche.
	//
	// ⚠️ **`AppliedByTerrain` e non `AppliedByAction`**: la sorgente e' il ghiaccio — `FRTTerrainDef` —
	// non un'azione pianificata da qualcuno. E' la stessa lettura per cui la causa dello spostamento e'
	// `Environmental` due righe piu' su: se qui si scrivesse `AppliedByAction`, il replay direbbe due cose
	// opposte sullo stesso evento.
	//
	// ⏱️ **Durata `2` e non `1`, ed e' misurato**: `TickStatuses()` decrementa nel Cleanup e questo e' il
	// Move, la fase immediatamente precedente. Con `1` lo stato nascerebbe e morirebbe senza che nessuna
	// fase interposta possa leggerlo — vedi il commento del tag.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (!bSlidThisMove.IsValidIndex(i) || !bSlidThisMove[i] || !IsValid(Units[i]))
		{
			continue;
		}
		FRTTurnLogEntry Nato = MakeStatusBirthEntry(ERTMatchPhase::Move, TAG_Status_Unbalanced,
			Units[i]->Cell, URTCombatLibrary::UnbalancedDurationTurns, /*bFromTerrain=*/ true);
		ApplyStatusLogged(Units[i], TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
		AppendLogEntry(Nato, Units[i]);
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
		// #79: il rifiuto di pianificazione e' stato letto e scritto nel log piu' sopra, e qui si CONSUMA —
		// «una dichiarazione vale per il turno in cui e' stata fatta», la stessa regola della rotazione
		// dichiarata che questo ciclo applica sotto. Sta prima dei due rami perche' vale per tutti: chi si e'
		// mosso (dove `PlaceOnCell` lo ha gia' azzerato), chi e' rimasto fermo, e chi e' MORTO in questo
		// turno — senza, un'unita' negata e poi uccisa porterebbe il rifiuto oltre il proprio turno, e una
		// rianimata lo troverebbe ancora acceso. `IsValid` sopra esclude solo chi e' gia' stato distrutto,
		// che non ha stato da azzerare.
		Unit->ClearMovePlanRejection();
		if (!Unit->bDeclaresPlannedFacing || !Unit->IsAlive())
		{
			Unit->ClearDeclaredFacing();
			continue;
		}

		ERTHexDirection Applied = Unit->Facing;
		// Il budget e' quello dell'EROE di questa unita' (ADR-0008 §1): qui si emette il verdetto, e il
		// verdetto non puo' usare un budget diverso da quello che la UI ha usato per proporre.
		const bool bLegal = URTFacingLibrary::TryApplyDeclaredFacing(Unit->MovementStyleThisTurn,
			Unit->WalkedThisTurn, Unit->Facing, Unit->PlannedFacing, Unit->PivotBudget(), Applied);

		FRTHexSimUnit Declaring(i, Unit->Cell, /*InMoveBudget=*/ 0);
		Declaring.Facing = Unit->Facing;
		RecordFacingChange(Declaring, Applied,
			bLegal ? ERTFacingOutcome::DeclaredInPlanning : ERTFacingOutcome::DeclarationRejected,
			ERTMatchPhase::Move, Unit);
		Unit->Facing = Declaring.Facing;

		AddLogEvent(FString::Printf(TEXT("%s: rotazione dichiarata %s"), *Unit->GetName(),
			bLegal ? TEXT("applicata") : TEXT("RIFIUTATA (fuori dal budget di pivot dell'eroe)")), FRTLogSubject::Unit(Unit));
		Unit->ClearDeclaredFacing();
	}

	// NOTA (CP 8.1): il cross-damage delle celle attraversate esiste di nuovo, ma solo per i terreni che
	// DICHIARANO effetti nel catalogo v0.1 — oggi Fire (10 danni + Burning), ShallowWater (Wet) e Smoke
	// (Obscured). Gli altri cinque non hanno OnEnterEffects: attraversarli non fa nulla, per scelta del
	// catalogo e non per un buco del resolver. Gli hazard di FINE turno (danno periodico a chi sosta) restano
	// fuori: li porta il CP 8.x dedicato, non questo.

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());

	// Il contesto muore QUI e non prima: ogni riga sopra lo legge.
	PendingMovement.Reset();
}
