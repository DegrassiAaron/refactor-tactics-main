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
