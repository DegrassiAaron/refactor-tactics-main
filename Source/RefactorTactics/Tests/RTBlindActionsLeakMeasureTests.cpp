// La SONDA del leak di occupazione nel Planning — `#2793`, capability `Blind / Unseen Spatial Actions`
// (`#2791`), contratto in attesa di decisione (`#2792`).
//
// ⛔ **Qui non si decide, e non si corregge.** Nessuna regola cambia, nessun filtro viene introdotto, nessuna
// soglia nasce. Questo file rende ESEGUIBILE una misura che oggi vive solo come lettura del sorgente nel
// corpo di `#2793`, e la lascia dov'e'.
//
// ## Perche' una sonda e non un canary
//
// `#2793` e' bloccata su `#2792`, che e' una decisione d'autore. La stessa scelta che questa sonda misura e'
// **dichiarata di proposito** altrove, nel docstring di `HexBotPlay.HiddenEnemyFairness`:
//
//     «l'OCCUPAZIONE resta legittimamente globale — `ReachableCells` e `DashHostiles` modellano cio' che il
//      resolver fara', non cio' che la squadra sa, e il giocatore umano ha lo stesso vincolo. Rendere il bot
//      piu' cieco dell'umano sarebbe sbagliato quanto renderlo onnisciente.»
//
// ∴ asserire qui `PlanningView(A) == PlanningView(B)` metterebbe un ROSSO PERMANENTE su `main` in nome di un
// invariante che nessuna decisione ha ancora adottato — ed e' il modo in cui un gate muore senza che nessuno
// lo spenga ([D-361], che lo ha appena misurato: cinque volte in una sola sessione a spiegare che i rossi
// erano attesi).
//
// ⌫ **«Nessuna decisione ha ancora adottato» e' scaduto dopo cinque ore: corretto il 2026-09-20.**
// [D-371] ha adottato quell'invariante il 2026-09-10 chiudendo `BLIND-1` con l'uscita *(c)* — «filtrata
// ovunque, bot compreso» — e dichiarando **ritirato** il principio del docstring citato qui sopra. Datazione:
// l'ultimo commit di questo file e' `fbc76dc5` (01:19), [D-371] e' entrata con `1b3dc92a` (06:30) dello
// stesso giorno.
//
// 🔴 **La forma della sonda resta giusta, e cambia solo cosa la tiene tale.** Non e' piu' «l'invariante non
// e' stato adottato», e' «l'invariante e' stato adottato e **non e' ancora implementato**»: `BlockedCellsFor`
// scorre `Snapshot.Occupancy` senza filtro, e il *dove* vive il filtro e' `BLIND-2`, aperta e dipendente da
// `OBS-1`. Un rosso qui resterebbe permanente lo stesso, per una ragione diversa — ed e' la ragione per cui
// il verso di conversione, sotto, e' ancora quello.
//
// ⚠️ **E la sede del principio ritirato e' TRIPLA, non doppia.** Oltre a questo file e a
// `HexBotPlay.HiddenEnemyFairness` (`Tests/RTHexBotIntegrationTests.cpp`, che appartiene a `#2793`), il testo
// vive verbatim in `Scenarios/Spec/Bot/HiddenEnemyFairness.json`, chiave `_nota_occupazione` — che **non e'
// un file inerte**: `docs/technical/architecture/capability-map.md` lo elenca fra gli scenari-gate di
// `RT-CAP-INTENT-PRIVACY`. Nessuna DoD lo nomina. Chi riscrive il canary per `#2793` deve passare anche di
// li', o il principio ritirato sopravvive in un artefatto di gate.
//
// ## 🔴 Cosa questo file asserisce, allora
//
// Il **comportamento corrente**, non quello desiderato. I test qui sotto sono VERDI PERCHE' IL DIFETTO C'E'.
//
// Diventeranno ROSSI il giorno in cui `#2793` filtra `BlockedCellsFor` per conoscenza — ed e' precisamente
// il loro scopo: chi implementa quel filtro deve **passare da qui** e convertire l'asserzione nel canary
// A/B/C, invece di scoprire mesi dopo che nessuno misurava piu' niente. Un difetto che sparisce in silenzio
// e uno che nasce in silenzio sono lo stesso difetto letto in due versi.
//
// ## Cosa misura, e su quale catena
//
// `URTHexSimLibrary::BlockedCellsFor` (`Turn/RTHexSimLibrary.cpp:60`, namespace anonimo) costruisce le celle
// impercorribili scorrendo `Snapshot.Occupancy` per intero (`:67`), e i soli criteri di esclusione sono «non
// sono io» e la compagna attraversabile che non sia destinazione ([D-396]). Nessun filtro di conoscenza. Lo
// consumano `ReachableCells` e `FindPathForUnit`, e da li' l'anteprima arriva a schermo
// (`RTPlayerController.cpp`, `SetPreviewReachableCells`).
//
// ⚠️ **Le sedi che leggono l'occupazione sono DUE, non una, e questo commento ne nominava una sola**
// (corretto il 2026-09-21, `#2793`). `ReachableCells` la legge a monte del Dijkstra via `BlockedCellsFor`
// (`RTHexSimLibrary.cpp:219`) **e di nuovo sul RISULTATO**, direttamente, per togliere le celle occupate da
// altri (`:345`, il filtro alleati di [D-396]) — e quel secondo ramo **non passa dall'imbuto**.
// 🔑 Chi implementa il filtro di conoscenza seguendo la sola riga di sopra chiuderebbe il **percorso** e
// lascerebbe aperto il **ventaglio**: `ClassifyWaypointCell` (`:626`) e' una terza sede, anch'essa diretta.
//
// I tre mondi differiscono SOLO per hidden occupancy. ⌫ **Questa riga diceva che la `FRTTeamKnowledge`
// dell'osservatore e' «la stessa oggetto, non una copia equivalente», ed era falso** — corretto il
// 2026-09-21 in code review: `WorldWith` chiama `ObserverKnowledge(Map)` da capo per ogni mondo. Cio' che
// regge la premessa non e' l'identita' dell'oggetto, e' un'**asserzione**: `ThreeWorldsAllOfferANonEmptyPlan`
// confronta le tre conoscenze campo per campo. Senza, un mondo che ricevesse una mappa diversa — la forma
// che la seconda meta' di questo file usa davvero, con `Walled->AddOrUpdateCell` — farebbe divergere le
// conoscenze autorizzate, e ogni «oggi differisce» resterebbe verde per la ragione sbagliata.
//
//     A — cella X vuota
//     B — un nemico che l'osservatore non conosce
//     C — due nemici che l'osservatore non conosce
//
// ⚠️ **La premessa e' asserita, non supposta** — la lezione di `HexBotPlay.HiddenEnemyFairness`, dove senza
// `bDidSomething` due «fermo» sarebbero stati identici e l'uguaglianza non avrebbe provato nulla. Qui la
// premessa e' doppia: (1) `ClassifyTarget` risponde `Rejected` su ogni nascosto, cioe' l'osservatore e'
// davvero ignorante; (2) il ventaglio di `A` CONTIENE le celle dei nascosti, cioe' il banco puo' vedere una
// differenza. Senza (2) un delta nullo non direbbe «nessun leak», direbbe «misura fuori portata».
//
// ## LA MISURA — 2026-09-09, clone `refactor-tactics-dev`, su `197a135b`, run dichiarata VALIDA
//
//     mondo   nascosti   celle nel ventaglio   celle perse rispetto ad A
//     A          0              61                        —
//     B          1              60               (2,0,L0)                 <- la cella del nascosto
//     C          2              59               (2,-2,L0) (2,0,L0)       <- le celle dei due nascosti
//
//     percorso (0,0) -> (4,0)
//     A   costo 4:  (0,0) (1,0) (2,0) (3,0) (4,0)
//     B   costo 5:  (0,0) (1,0) (1,1) (2,1) (3,0) (4,0)
//
// 🔴 **Il buco nel ventaglio E' UNA LETTURA DI POSIZIONE.** Non «il ventaglio e' un po' diverso»: le celle
// perse sono **esattamente** quelle dei nascosti, una per nascosto. Chi guarda l'anteprima non deduce che
// c'e' qualcuno: legge **dove**.
//
// 🔴 **E il costo cambia — 4 diventa 5.** Questa e' la meta' della misura che la lettura del sorgente non
// aveva anticipato: il canale non e' solo la FORMA del tracciato, e' il **numero** che il giocatore ha
// davanti. Un costo che sale di uno su una direttrice libera dice che qualcosa la ostruisce, e lo dice in
// una cifra sola, senza bisogno di guardare il disegno.
//
// ⚠️ Le celle e i costi qui sopra sono proprieta' di QUESTO banco (arena piatta di raggio 4, costo uniforme):
// non sono soglie e nessuna asserzione li confronta. Cio' che le asserzioni fissano e' il **verso** —
// il ventaglio cambia, il percorso cambia — non i valori.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"          // ERTAbilityShape
#include "Combat/RTHexCombatLibrary.h"     // HexHitCells, MakeBlastPreview — il footprint dell'AoE
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPath.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Squadra dell'osservatore che pianifica.
	 *
	 * ⌫ **Questa riga diceva «`FRTHexSimUnit` non porta il `TeamId`: vive qui», ed era FALSO** — corretto il
	 * 2026-09-21 (`#2793`). Il campo esiste, `int32 TeamId = INDEX_NONE;` a `Turn/RTHexSim.h:109`; e' il
	 * **costruttore** usato qui (`RTHexSim.h:112`) a non impostarlo.
	 *
	 * 🔴 **E la differenza non era cosmetica: il banco misurava per un accidente.** Lasciate a
	 * `INDEX_NONE`, tutte le unita' rispondevano `false` a `TeamsAreAllied` (`RTHexSimLibrary.cpp:42`:
	 * *«una squadra non dichiarata non e' alleata di nessuno, nemmeno di un'altra non dichiarata»*) — quindi
	 * i nascosti bloccavano la rotta, ma per **assenza di dato**, non perche' avversari. Il giorno in cui
	 * quel predicato cambiasse, la misura del percorso sarebbe morta in silenzio. Da qui in poi le squadre
	 * si **assegnano**, e `WorldWith` lo fa.
	 */
	constexpr int32 ObserverTeam = 0;
	/** Squadra dei nascosti. Avversaria: cio' che li rende ostacoli e' dichiarato, non dedotto. */
	constexpr int32 HiddenTeam = 1;

	/** Chi pianifica. Budget largo: il banco misura la CONOSCENZA, non il budget. */
	constexpr int32 PlannerId = 1;
	constexpr int32 PlannerBudget = 8;

	/** Il raggio dell'arena: piatta, costo uniforme, nessun ostacolo. Una regola alla volta. */
	constexpr int32 ArenaRadius = 4;

	const FRTCellId PlannerCell{ 0, 0, 0 };
	/** Le due celle dei nascosti. Fuori dalla vista dell'osservatore, dentro il suo raggio d'azione. */
	const FRTCellId HiddenCellA{ 2, 0, 0 };
	const FRTCellId HiddenCellB{ 2, -2, 0 };
	/** Il fondo verso cui si chiede un percorso: oltre il primo nascosto, sulla stessa direttrice. */
	const FRTCellId GoalCell{ 4, 0, 0 };

	/**
	 * La conoscenza dell'osservatore, IDENTICA nei tre mondi.
	 *
	 * 🔴 **Il terreno e' ricordato, le unita' no.** `ExploredCells` copre l'arena intera — e' il caso di
	 * [D-227], dove una cella gia' vista resta disegnata e raggiungibile — mentre `VisibleCells` si ferma
	 * agli immediati vicini di chi pianifica e `Contacts` e' vuota. E' la configurazione in cui il difetto
	 * si manifesta **senza toccare [D-249]**: nessuna cella «mai vista» entra in gioco.
	 */
	FRTTeamKnowledge ObserverKnowledge(const URTHexMapAsset* Map)
	{
		FRTTeamKnowledge K;
		K.TeamId = ObserverTeam;
		K.TurnNumber = 1;

		for (const FRTHexCellData& Cell : Map->Cells)
		{
			K.ExploredCells.Add(Cell.Id);
			if (URTHexLibrary::HexDistance(PlannerCell, Cell.Id) <= 1)
			{
				K.VisibleCells.Add(Cell.Id);
			}
		}
		K.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		K.VisibleCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		return K;
	}

	/** Identita' stabili dei due nascosti: nominate, perche' i reason code le confrontano per unita'. */
	constexpr int32 HiddenIdA = 2;
	constexpr int32 HiddenIdB = 3;

	/** Un'unita' del banco con la sua squadra DICHIARATA — vedi la nota su `ObserverTeam`. */
	FRTHexSimUnit UnitOn(int32 UnitId, const FRTCellId& Cell, int32 MoveBudget, int32 TeamId)
	{
		FRTHexSimUnit U(UnitId, Cell, MoveBudget);
		U.TeamId = TeamId;
		return U;
	}

	/** Uno dei tre mondi: chi pianifica, piu' `HiddenCount` nascosti sulle celle dichiarate sopra. */
	FRTHexSnapshot WorldWith(const URTHexMapAsset* Map, int32 HiddenCount)
	{
		TArray<FRTHexSimUnit> Units;
		Units.Add(UnitOn(PlannerId, PlannerCell, PlannerBudget, ObserverTeam));
		if (HiddenCount >= 1) { Units.Add(UnitOn(HiddenIdA, HiddenCellA, /*MoveBudget*/ 0, HiddenTeam)); }
		if (HiddenCount >= 2) { Units.Add(UnitOn(HiddenIdB, HiddenCellB, /*MoveBudget*/ 0, HiddenTeam)); }

		FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, Units);
		// La conoscenza entra nello snapshot come la mette il TurnManager in partita. `MakeSnapshot` non la
		// costruisce: e' esattamente il punto: la porta esiste, e chi calcola il ventaglio non la apre.
		Snapshot.TeamKnowledge.Add(ObserverKnowledge(Map));
		return Snapshot;
	}

	TSet<FRTCellId> FanCells(const FRTHexSnapshot& Snapshot)
	{
		TSet<FRTCellId> Out;
		for (const FRTHexReachableCell& Cell : URTHexSimLibrary::ReachableCells(Snapshot, PlannerId))
		{
			Out.Add(Cell.Cell);
		}
		return Out;
	}

	/**
	 * Il ventaglio **intero**, non il solo insieme delle celle — `#2793`, rilievo di code review del
	 * 2026-09-21.
	 *
	 * 🔴 **`FanCells` butta via due terzi di `FRTHexReachableCell`**, e con essi due canali. La struct porta
	 * `Cell`, `Cost` e `FromCell` (`Turn/RTHexSim.h:118-141`): il **costo per cella** e' il numero che il
	 * readout stampa sotto il cursore, e `FromCell` e' il predecessore da cui deriva il **facing** (CP 13.5).
	 * Un confronto sul solo insieme di celle dichiarerebbe «ventaglio identico» mentre entrambi cambiano —
	 * ed e' la stessa forma del difetto che la mutazione sui reason code ha misurato: il canale non si
	 * chiude, cambia nome.
	 */
	TMap<FRTCellId, FRTHexReachableCell> FanByCell(const FRTHexSnapshot& Snapshot)
	{
		TMap<FRTCellId, FRTHexReachableCell> Out;
		for (const FRTHexReachableCell& Cell : URTHexSimLibrary::ReachableCells(Snapshot, PlannerId))
		{
			Out.Add(Cell.Cell, Cell);
		}
		return Out;
	}

	/** Le celle che `A` offriva e questo mondo non offre piu': la forma del buco. */
	TArray<FRTCellId> CellsLost(const TSet<FRTCellId>& Baseline, const TSet<FRTCellId>& Other)
	{
		TArray<FRTCellId> Lost = Baseline.Difference(Other).Array();
		Lost.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		return Lost;
	}

	FString DescribeCells(const TArray<FRTCellId>& Cells)
	{
		if (Cells.Num() == 0) { return TEXT("nessuna"); }
		FString Out;
		for (const FRTCellId& Cell : Cells)
		{
			Out += FString::Printf(TEXT("(%d,%d,L%d) "), Cell.X, Cell.Y, Cell.Layer);
		}
		return Out.TrimEnd();
	}

	FString DescribePath(const FRTHexPathResult& Result)
	{
		if (Result.Status != ERTHexPathStatus::Success) { return TEXT("nessun percorso"); }
		FString Out = FString::Printf(TEXT("costo %d: "), Result.TotalCost);
		for (const FRTCellId& Cell : Result.Path)
		{
			Out += FString::Printf(TEXT("(%d,%d) "), Cell.X, Cell.Y);
		}
		return Out.TrimEnd();
	}

	/**
	 * La premessa, asserita in ogni test che usa il banco: l'osservatore **non conosce** i nascosti, e la
	 * regola che lo dice e' quella di produzione (`ClassifyTarget`, CP 13.2), non un predicato scritto qui.
	 */
	bool ObserverIsIgnorantOfHidden(FAutomationTestBase& Test, const URTHexMapAsset* Map)
	{
		const FRTTeamKnowledge K = ObserverKnowledge(Map);
		const bool bA = URTTeamKnowledgeLibrary::ClassifyTarget(K, HiddenIdA, HiddenTeam, HiddenCellA)
			== ERTTargetKnowledge::Rejected;
		const bool bB = URTTeamKnowledgeLibrary::ClassifyTarget(K, HiddenIdB, HiddenTeam, HiddenCellB)
			== ERTTargetKnowledge::Rejected;
		return Test.TestTrue(
			TEXT("premessa: per ClassifyTarget entrambi i nascosti sono Rejected, cioe' ignoti alla squadra"),
			bA && bB);
	}
}

// ---------------------------------------------------------------------------------------------------------
// Il ventaglio raggiungibile
// ---------------------------------------------------------------------------------------------------------

/**
 * ⚠️ **VERDE PERCHE' IL DIFETTO C'E'.** Diventa rosso quando `#2793` filtra `BlockedCellsFor` per
 * conoscenza, ed e' il segnale che l'asserzione va convertita nel canary `PlanningView(A) == PlanningView(B)`
 * invece di essere cancellata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsReachableFanLeaksTest,
	"RefactorTactics.BlindActions.ReachableFanLeaksHiddenOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsReachableFanLeaksTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}
	if (!ObserverIsIgnorantOfHidden(*this, Map))
	{
		return false;
	}

	const TSet<FRTCellId> FanA = FanCells(WorldWith(Map, 0));
	const TSet<FRTCellId> FanB = FanCells(WorldWith(Map, 1));
	const TSet<FRTCellId> FanC = FanCells(WorldWith(Map, 2));

	// 🔴 **Il controllo che rende la misura leggibile.** Senza, un delta nullo sarebbe ambiguo fra «nessun
	// leak» e «i nascosti stanno fuori dal ventaglio, quindi il banco non poteva vedere niente».
	if (!TestTrue(TEXT("premessa: nel mondo A entrambe le celle dei nascosti sono nel ventaglio"),
		FanA.Contains(HiddenCellA) && FanA.Contains(HiddenCellB)))
	{
		return false;
	}

	const TArray<FRTCellId> LostB = CellsLost(FanA, FanB);
	const TArray<FRTCellId> LostC = CellsLost(FanA, FanC);

	AddInfo(FString::Printf(TEXT("mondo A — nessun nascosto:  %d celle nel ventaglio"), FanA.Num()));
	AddInfo(FString::Printf(TEXT("mondo B — un nascosto:      %d celle, perse rispetto ad A: %s"),
		FanB.Num(), *DescribeCells(LostB)));
	AddInfo(FString::Printf(TEXT("mondo C — due nascosti:     %d celle, perse rispetto ad A: %s"),
		FanC.Num(), *DescribeCells(LostC)));

	// La misura, asserita come COMPORTAMENTO CORRENTE. Vedi l'avvertenza in testa al file.
	TestTrue(TEXT("oggi il ventaglio CAMBIA per un'occupazione che l'osservatore non conosce (B != A)"),
		LostB.Num() > 0);
	TestTrue(TEXT("oggi il ventaglio perde ancora piu' celle con due nascosti (C perde piu' di B)"),
		LostC.Num() > LostB.Num());

	// ➕ **Il ventaglio non e' un insieme di celle: e' un insieme di `FRTHexReachableCell`** — rilievo di
	// code review del 2026-09-21. Le celle che SOPRAVVIVONO in `B` possono comunque portare un costo e un
	// predecessore diversi, e sono due canali che il confronto sull'insieme non vedeva:
	//   - il **costo per cella** e' il numero che il readout mostra sotto il cursore;
	//   - `FromCell` e' il predecessore da cui deriva il **facing** (CP 13.5).
	// 🔑 Senza questa misura, un filtro messo nella sola sede del RISULTATO (`RTHexSimLibrary.cpp:345`)
	// renderebbe i tre insiemi identici e il canary direbbe «canale chiuso» mentre il Dijkstra gira ancora
	// intorno all'occupazione.
	const TMap<FRTCellId, FRTHexReachableCell> FullA = FanByCell(WorldWith(Map, 0));
	const TMap<FRTCellId, FRTHexReachableCell> FullB = FanByCell(WorldWith(Map, 1));

	TArray<FRTCellId> CostChanged;
	TArray<FRTCellId> PredecessorChanged;
	for (const TPair<FRTCellId, FRTHexReachableCell>& Entry : FullA)
	{
		const FRTHexReachableCell* InB = FullB.Find(Entry.Key);
		if (!InB) { continue; } // gia' contata fra le celle perse
		if (InB->Cost != Entry.Value.Cost) { CostChanged.Add(Entry.Key); }
		if (!(InB->FromCell == Entry.Value.FromCell)) { PredecessorChanged.Add(Entry.Key); }
	}
	CostChanged.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
	PredecessorChanged.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });

	AddInfo(FString::Printf(TEXT("celle presenti in A e in B col COSTO cambiato:        %s"), *DescribeCells(CostChanged)));
	AddInfo(FString::Printf(TEXT("celle presenti in A e in B col PREDECESSORE cambiato: %s"), *DescribeCells(PredecessorChanged)));

	// Il controllo: senza celle in comune il confronto sopra non avrebbe popolazione.
	if (!TestTrue(TEXT("premessa: A e B condividono delle celle — il confronto per costo ha popolazione"),
		FullA.Num() > LostB.Num()))
	{
		return false;
	}

	TestTrue(TEXT("oggi anche il COSTO di celle che restano nel ventaglio cambia per un'occupazione ignota"),
		CostChanged.Num() > 0);
	TestTrue(TEXT("oggi cambia anche il PREDECESSORE, da cui deriva il facing: il canale non e' solo l'insieme"),
		PredecessorChanged.Num() > 0);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'anteprima del percorso
// ---------------------------------------------------------------------------------------------------------

/**
 * ⚠️ **VERDE PERCHE' IL DIFETTO C'E'**, come sopra.
 *
 * Il percorso e' il canale piu' esplicito dei due: il ventaglio nasconde una cella, il tracciato **gira
 * intorno** a un ostacolo che il giocatore non ha nessun diritto di conoscere — che e' l'argomento con cui
 * [D-249] aveva vietato il movimento verso l'ignoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsPathLeaksTest,
	"RefactorTactics.BlindActions.PlannedPathLeaksHiddenOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsPathLeaksTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}
	if (!ObserverIsIgnorantOfHidden(*this, Map))
	{
		return false;
	}

	const FRTHexPathResult PathA = URTHexSimLibrary::FindPathForUnit(WorldWith(Map, 0), PlannerId, GoalCell);
	const FRTHexPathResult PathB = URTHexSimLibrary::FindPathForUnit(WorldWith(Map, 1), PlannerId, GoalCell);
	const FRTHexPathResult PathC = URTHexSimLibrary::FindPathForUnit(WorldWith(Map, 2), PlannerId, GoalCell);

	AddInfo(FString::Printf(TEXT("mondo A — nessun nascosto: %s"), *DescribePath(PathA)));
	AddInfo(FString::Printf(TEXT("mondo B — un nascosto:     %s"), *DescribePath(PathB)));
	AddInfo(FString::Printf(TEXT("mondo C — due nascosti:    %s"), *DescribePath(PathC)));

	// ℹ️ **Nota, non guardia** (`#2793`, 2026-09-21). `FindPathForUnit` con `MoveBudget <= 0` esce a
	// `RTHexSimLibrary.cpp:421` **senza mai guardare l'occupazione**. ⌫ **Una prima stesura ci metteva un
	// `TestTrue(PlannerBudget > 0)` e lo motivava con *«una regressione sul budget spegnerebbe questo canale
	// mostrando un verde»*: era falso, e trovato in code review.** `PlannerBudget` e' `constexpr`, quindi
	// quell'asserzione non poteva fallire a runtime; e il verde che prometteva di prevenire non e'
	// raggiungibile — con budget zero l'uscita anticipata restituisce `NoPath`, la premessa qui sotto cade e
	// il test e' **rosso**. La copertura c'era gia', ed era strutturale.

	// Il controllo: senza un percorso in A che passa dalla cella del nascosto, il confronto non misura nulla.
	if (!TestTrue(TEXT("premessa: nel mondo A il percorso verso il fondo passa dalla cella del nascosto"),
		PathA.Status == ERTHexPathStatus::Success && PathA.Path.Contains(HiddenCellA)))
	{
		return false;
	}

	// La misura, asserita come COMPORTAMENTO CORRENTE.
	TestTrue(TEXT("oggi il percorso EVITA una cella che l'osservatore non sa occupata"),
		PathB.Status != ERTHexPathStatus::Success || !PathB.Path.Contains(HiddenCellA));
	TestTrue(TEXT("oggi il tracciato mostrato al giocatore CAMBIA forma per un'occupazione ignota"),
		PathA.Path != PathB.Path);

	// 🔴 Il canale piu' stretto dei tre, e quello che la lettura del sorgente non aveva anticipato: non serve
	// guardare il disegno, basta la cifra. Un costo che sale su una direttrice libera dichiara un'ostruzione.
	TestTrue(TEXT("oggi anche il COSTO mostrato cambia per un'occupazione che l'osservatore non conosce"),
		PathA.TotalCost != PathB.TotalCost);

	// ➕ **Il terzo mondo, che la DoD di `#2793` chiede e questo test non aveva** (2026-09-21).
	//
	// ⚠️ **Cosa NON si asserisce, e perche'.** `HiddenCellB` sta a `(2,-2)`, **fuori** dalla direttrice
	// `(0,0) → (4,0)`: chiedere che il tracciato di `C` non la contenga sarebbe vero comunque, anche a
	// filtro implementato, e sarebbe un verde per costruzione. Cio' che si misura e' l'unica cosa che la
	// DoD chiede — `PlanningView(A) == PlanningView(C)` — e oggi e' **falsa**, per il primo nascosto.
	if (!TestTrue(TEXT("premessa: nel mondo C un percorso ESISTE — altrimenti si misurerebbe un'assenza"),
		PathC.Status == ERTHexPathStatus::Success))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("B e C mostrano lo stesso tracciato: %s — il secondo nascosto e' fuori direttrice"),
		PathB.Path == PathC.Path ? TEXT("si'") : TEXT("no")));

	// La misura, asserita come COMPORTAMENTO CORRENTE, sul terzo mondo.
	TestTrue(TEXT("oggi il tracciato di C differisce da A: l'uguaglianza a tre mondi della DoD non tiene"),
		PathA.Path != PathC.Path);
	TestTrue(TEXT("oggi anche il COSTO di C differisce da quello di A"),
		PathA.TotalCost != PathC.TotalCost);

	return true;
}

// =========================================================================================================
// `BLIND-4` — la GEOMETRIA mai osservata, non l'occupazione
// =========================================================================================================
//
// ⛔ **Stessa disciplina dei due test sopra: qui non si decide e non si corregge.** `BLIND-4` chiede se la
// geometria mai osservata sia informazione pubblica, ed e' una decisione d'autore aperta in
// `docs/OPEN_DECISIONS.md`.
//
// 🔴 **Perche' questa sonda esiste, e perche' la sua prima stesura non fu scritta.** `BLIND-4` era stata
// registrata con la riga *«chi la chiude deve produrre l'esempio falsificabile insieme alla risposta»* —
// cioe' con la conclusione che l'esempio non fosse producibile prima. E' lo stesso errore gia' fatto e gia'
// corretto su `BLIND-1`: **l'esempio non misura la risposta, misura il CANALE**. La domanda instrumentabile
// oggi, senza decidere nulla, e':
//
//     due mondi con conoscenza autorizzata IDENTICA, che differiscono solo per la geometria MAI OSSERVATA.
//     Se un output osservabile del Planning differisce, quella geometria e' GIA' pubblica per quel canale.
//
// ## I due canali, che non si equivalgono
//
// Il velo E' cablato — `URTKnowledgeVeilPresenter::Apply` chiama
// `ARTHexMapActor::ApplyKnowledgeVeil(KnowledgeForTeamPublic(ViewerTeamId()))` — quindi una cella mai
// osservata **non si disegna**. Da qui la separazione:
//
//   - **diretto**  — il ventaglio OFFRE celle che il velo nasconde. Misurato da
//                    `ReachableFanOffersNeverObservedCells`.
//   - **indiretto** — un muro NEL BUIO fa piegare la parte di percorso che sta NELLA LUCE. Il giocatore non
//                    vede il muro: vede che la strada gira. Misurato da `NeverObservedWallBendsTheLitPath`,
//                    ed e' il piu' insidioso dei due perche' non ha un rimedio ovvio — per non piegare, il
//                    percorso dovrebbe ignorare il muro, cioe' servirebbe la **mappa ottimistica** che
//                    [D-249] aveva contato fra i costi da evitare.
//
// ⚠️ **La destinazione e' RICORDATA, non mai vista, ed e' la condizione che rende il caso legale.** Per
// [D-227] una cella gia' vista resta raggiungibile; per [D-249] una mai vista non e' bersaglio di movimento.
// Un banco che puntasse al buio misurerebbe un percorso che il giocatore non puo' chiedere.
//
// ## LA MISURA — 2026-09-10, clone `refactor-tactics-dev`, base `67a0d164` piu' le correzioni di questo
// commit, run dichiarata VALIDA
//
//     CANALE INDIRETTO — un muro in (2,0), MAI OSSERVATA, contro la stessa cella libera
//       mondo APERTO   costo 4:  (0,0) (1,0) (2,0) (3,0) (4,0)
//       mondo MURO     costo 5:  (0,0) (1,0) (1,1) (2,1) (3,0) (4,0)
//       differiscono E L'OSSERVATORE LE VEDE:   (1,1)
//       differiscono ma restano NEL BUIO:       (2,0) (2,1)
//
//     CANALE DIRETTO — ricordo fermo a distanza 2
//       ventaglio 61 celle, di cui MAI OSSERVATE: 42
//
// 🔴 **Il canale indiretto e' REALE, e la misura che conta e' la separazione fra le due righe.** Non «i due
// tracciati differiscono» — quello sarebbe ovvio e innocuo se la differenza stesse tutta nel buio. La
// differenza si SPACCA: due celle restano invisibili — `(2,0)`, il muro, e `(2,1)` sulla deviazione — e
// **una resta illuminata**, `(1,1)`. Il giocatore guarda la strada girare in piena luce, e da quella curva
// deduce che al buio c'e' un muro. Il muro non si vede; la sua **conseguenza** si'.
//
// ⚠️ **La prima stesura di questo banco aveva UNA sola cella al buio, e con essa la misura era tautologica**
// — trovato in code review. Se l'unica cella non illuminata e' quella del muro, e il muro non puo' comparire
// nel percorso deviato, allora *qualunque* differenza e' per forza illuminata: `LitDifference > 0`
// coincideva con `PathOpen.Path != PathWalled.Path`, cioe' con la riga che questo commento dichiara
// insufficiente. La seconda cella al buio e' cio' che rende la separazione una misura invece che
// un'identita'.
//
// 🔴 **E il canale diretto non e' un caso limite: e' la maggioranza del ventaglio.** Con un ricordo fermo a
// distanza 2, **42 celle su 61** che la portata offre sono celle che il velo non disegna. La portata conosce
// il buio molto piu' di quanto il buio si lasci guardare.
//
// ∴ **la geometria mai osservata e' GIA' pubblica per entrambi i canali.** `BLIND-4` non e' quindi la
// domanda *«la rendiamo pubblica?»* — lo e' gia' — ma *«la dichiariamo tale, oppure paghiamo la mappa
// ottimistica che [D-249] aveva contato fra i costi da evitare?»*. E' una domanda molto piu' stretta di
// quella con cui la voce era stata registrata.

namespace
{
	/** La cella BUIA che nel mondo `Muro` ospita il muro: mai osservata in entrambi i mondi. */
	const FRTCellId DarkCell{ 2, 0, 0 };

	/**
	 * La SECONDA cella al buio, e non e' un ornamento: sta **sulla deviazione** che il muro provoca.
	 *
	 * 🔴 **Senza, l'asserzione di testa sarebbe TAUTOLOGICA** — trovato in code review. Con una sola cella
	 * non illuminata sull'intera board, e con quella cella occupata dal muro (quindi mai presente nel
	 * percorso deviato), **qualunque** differenza fra i due tracciati e' per forza illuminata: la misura
	 * `LitDifference > 0` collassa su `PathOpen.Path != PathWalled.Path` e non dice niente di piu'.
	 * Ma la riga in testa a questo blocco distingue proprio le due cose. Con `(2,1)` al buio la deviazione
	 * attraversa il buio, il filtro «illuminato» ha qualcosa da togliere, e la misura torna a discriminare.
	 */
	const FRTCellId DarkOnDetour{ 2, 1, 0 };
	/** Destinazione RICORDATA — legale per [D-227] — che sta oltre la cella buia sulla stessa direttrice. */
	const FRTCellId LitGoal{ 4, 0, 0 };

	/** Un'arena piatta nuova: i due mondi non possono condividere la mappa, perche' uno la muta. */
	URTHexMapAsset* MakeArena()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	}

	/**
	 * Conoscenza in cui tutto e' ricordato tranne una **tasca buia** di due celle: `DarkCell`, che nel mondo
	 * `Muro` porta il muro, e `DarkOnDetour`, che sta sulla deviazione.
	 *
	 * 🔑 **Due e non una, ed e' una correzione da code review.** Con una sola cella al buio l'asserzione di
	 * testa non poteva distinguere «la deviazione si vede» da «i tracciati differiscono»: vedi il commento
	 * su `DarkOnDetour`.
	 */
	FRTTeamKnowledge KnowledgeWithDarkPocket(const URTHexMapAsset* Map)
	{
		FRTTeamKnowledge K;
		K.TeamId = ObserverTeam;
		K.TurnNumber = 1;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			// Mai osservate: non entrano nemmeno in `Explored`.
			if (Cell.Id == DarkCell || Cell.Id == DarkOnDetour) { continue; }
			K.ExploredCells.Add(Cell.Id);
			if (URTHexLibrary::HexDistance(PlannerCell, Cell.Id) <= 1) { K.VisibleCells.Add(Cell.Id); }
		}
		K.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		K.VisibleCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		return K;
	}

	/** Conoscenza in cui il ricordo si ferma a distanza 2: oltre, la board e' buia. Per il canale DIRETTO. */
	FRTTeamKnowledge KnowledgeExploredWithinTwo(const URTHexMapAsset* Map)
	{
		FRTTeamKnowledge K;
		K.TeamId = ObserverTeam;
		K.TurnNumber = 1;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			const int32 D = URTHexLibrary::HexDistance(PlannerCell, Cell.Id);
			if (D <= 2) { K.ExploredCells.Add(Cell.Id); }
			if (D <= 1) { K.VisibleCells.Add(Cell.Id); }
		}
		K.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		K.VisibleCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		return K;
	}

	/** Cio' che l'osservatore ha diritto di VEDERE disegnato: il visibile piu' il ricordato. */
	bool IsLit(const FRTTeamKnowledge& K, const FRTCellId& Cell)
	{
		return K.VisibleCells.Contains(Cell) || K.ExploredCells.Contains(Cell);
	}

	/** Solo chi pianifica, su questa mappa. Nessun nascosto: qui la variabile e' il TERRENO. */
	FRTHexSnapshot LonePlannerOn(const URTHexMapAsset* Map, const FRTTeamKnowledge& K)
	{
		TArray<FRTHexSimUnit> Units;
		Units.Add(UnitOn(PlannerId, PlannerCell, PlannerBudget, ObserverTeam));
		FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, Units);
		Snapshot.TeamKnowledge.Add(K);
		return Snapshot;
	}
}

/**
 * ⚠️ **VERDE PERCHE' IL CANALE C'E'**, come i due test sopra.
 *
 * ⌫ **L'innesco dichiarato qui era irraggiungibile: corretto il 2026-09-20.** Questa riga diceva *«diventa
 * rosso il giorno in cui `BLIND-4` viene chiusa nel verso (b) — "la geometria mai osservata non e'
 * pubblica"»*. [D-373] ha chiuso `BLIND-4` nel verso **opposto** il 2026-09-10: *«la forma e' pubblica»*, e
 * ha **scartato** esplicitamente *«la forma non e' pubblica»*. Quel rosso non puo' piu' accadere.
 *
 * 🔑 **L'innesco vero e' un altro, e [D-373] lo nomina**: l'implementazione della **mappa ottimistica** di
 * [D-372], che appartiene a [#2794]. Sotto quella regola *«per una cella ignota il Planning non legge piu'
 * `bBlocksMovement`, `TotalMoveCost()` ne' `BlocksTraversal`»* — cioe' il muro mai osservato smette di
 * piegare il percorso, e questo test diventa rosso. 🔴 **Chi lavora #2794 lo leggera' come l'esito atteso,
 * non come un difetto proprio** — che e' l'intero scopo di questa riga, e il motivo per cui lasciarla
 * sbagliata era peggio che non averla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsDarkWallBendsPathTest,
	"RefactorTactics.BlindActions.NeverObservedWallBendsTheLitPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsDarkWallBendsPathTest::RunTest(const FString&)
{
	URTHexMapAsset* Open = MakeArena();
	URTHexMapAsset* Walled = MakeArena();
	if (!TestTrue(TEXT("premessa: le due arene esistono e sono distinte"), Open != nullptr && Walled != nullptr && Open != Walled))
	{
		return false;
	}

	// L'UNICA differenza fra i due mondi: un muro nella cella che l'osservatore non ha mai visto.
	FRTHexCellData Wall(DarkCell);
	Wall.bBlocksMovement = true;
	Walled->AddOrUpdateCell(Wall);
	Walled->SortCells();

	const FRTTeamKnowledge K = KnowledgeWithDarkPocket(Open);

	// 🔴 Le premesse che rendono la misura leggibile, e senza le quali un delta non direbbe niente.
	if (!TestTrue(TEXT("premessa: le due celle della tasca sono MAI OSSERVATE (ne' viste ne' ricordate)"),
		!IsLit(K, DarkCell) && !IsLit(K, DarkOnDetour)))
	{
		return false;
	}
	if (!TestTrue(TEXT("premessa: la destinazione e' RICORDATA, quindi legale per [D-227]"), IsLit(K, LitGoal)))
	{
		return false;
	}

	const FRTHexPathResult PathOpen   = URTHexSimLibrary::FindPathForUnit(LonePlannerOn(Open, K), PlannerId, LitGoal);
	const FRTHexPathResult PathWalled = URTHexSimLibrary::FindPathForUnit(LonePlannerOn(Walled, K), PlannerId, LitGoal);

	AddInfo(FString::Printf(TEXT("mondo APERTO — nessun muro al buio: %s"), *DescribePath(PathOpen)));
	AddInfo(FString::Printf(TEXT("mondo MURO   — muro in (%d,%d) mai osservata: %s"),
		DarkCell.X, DarkCell.Y, *DescribePath(PathWalled)));

	// Il controllo: senza un percorso che passi dalla cella buia, il muro non poteva contare.
	if (!TestTrue(TEXT("premessa: nel mondo APERTO il percorso passa dalla cella buia"),
		PathOpen.Status == ERTHexPathStatus::Success && PathOpen.Path.Contains(DarkCell)))
	{
		return false;
	}

	// 🔴 **E il mondo MURO deve avere un percorso, non l'assenza di uno** — trovato in code review.
	// `FRTHexPathResult` nasce `NoPath` con `Path` vuoto e `TotalCost` a zero: se il muro facesse ABORTIRE
	// la ricerca invece di farla deviare, `LitDifference` raccoglierebbe l'intero tracciato aperto e i costi
	// differirebbero (4 contro 0). Entrambe le misure sotto passerebbero **a vuoto**, stampando «il percorso
	// piega» dove non c'e' percorso da piegare. Il test gemello di sopra questa guardia ce l'ha gia'.
	if (!TestTrue(TEXT("premessa: nel mondo MURO un percorso ESISTE — altrimenti non c'e' deviazione, c'e' assenza"),
		PathWalled.Status == ERTHexPathStatus::Success))
	{
		return false;
	}

	// 🔴 **LA MISURA CHE CONTA**: non «i percorsi differiscono», ma «differiscono DOVE IL GIOCATORE GUARDA».
	// Una differenza confinata alla cella buia non sarebbe un leak: quella cella non si disegna.
	TArray<FRTCellId> LitDifference;
	for (const FRTCellId& Cell : PathWalled.Path)
	{
		if (!PathOpen.Path.Contains(Cell) && IsLit(K, Cell)) { LitDifference.AddUnique(Cell); }
	}
	for (const FRTCellId& Cell : PathOpen.Path)
	{
		if (!PathWalled.Path.Contains(Cell) && IsLit(K, Cell)) { LitDifference.AddUnique(Cell); }
	}
	LitDifference.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });

	// Le celle in cui i tracciati differiscono e che stanno NEL BUIO: non sono un leak, e servono a provare
	// che il filtro «illuminato» ha davvero qualcosa da togliere.
	TArray<FRTCellId> DarkDifference;
	for (const FRTCellId& Cell : PathWalled.Path)
	{
		if (!PathOpen.Path.Contains(Cell) && !IsLit(K, Cell)) { DarkDifference.AddUnique(Cell); }
	}
	for (const FRTCellId& Cell : PathOpen.Path)
	{
		if (!PathWalled.Path.Contains(Cell) && !IsLit(K, Cell)) { DarkDifference.AddUnique(Cell); }
	}
	DarkDifference.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });

	AddInfo(FString::Printf(TEXT("celle in cui i due tracciati differiscono E che l'osservatore VEDE: %s"),
		*DescribeCells(LitDifference)));
	AddInfo(FString::Printf(TEXT("celle in cui differiscono ma che restano NEL BUIO (non sono un leak):  %s"),
		*DescribeCells(DarkDifference)));

	// 🔴 **La premessa che toglie la tautologia**: se nessuna differenza cadesse nel buio, `LitDifference`
	// coinciderebbe con «i tracciati differiscono» e la misura non direbbe niente di piu'.
	if (!TestTrue(TEXT("premessa: la deviazione attraversa il buio, quindi il filtro ILLUMINATO discrimina"),
		DarkDifference.Num() > 0))
	{
		return false;
	}

	// La misura, asserita come COMPORTAMENTO CORRENTE.
	TestTrue(TEXT("oggi un muro MAI OSSERVATO fa piegare il percorso nella parte ILLUMINATA: il giocatore lo deduce senza vederlo"),
		LitDifference.Num() > 0);
	TestTrue(TEXT("oggi anche il COSTO cambia per una geometria che l'osservatore non ha mai visto"),
		PathOpen.TotalCost != PathWalled.TotalCost);

	return true;
}

/**
 * Il canale DIRETTO: il ventaglio offre celle che il velo nasconde.
 *
 * ⚠️ **VERDE PERCHE' IL CANALE C'E'.**
 *
 * ⌫ **Corretto il 2026-09-20, e DUE volte: l'innesco dichiarato era irraggiungibile, e la prima
 * riscrittura ne ha messo un altro che non scatta.**
 *
 * La riga diceva *«se `BLIND-4` chiudesse nel verso (b), questo test diventa rosso»*, e [D-373] l'ha chiusa
 * nel verso *(a)* — la forma della board **e'** pubblica. Fin qui come il test precedente.
 *
 * 🔴 **Ma l'innesco di QUEL test non vale per questo, e scriverlo lo stesso era un errore trovato in code
 * review.** La mappa ottimistica di [D-372] rende una cella ignota **passabile nel caso migliore**: puo'
 * solo **allargare** il ventaglio. L'asserzione qui e' `Dark.Num() > 0` su un'arena **piatta e senza
 * ostacoli** (`MakeFlatArena`, vedi `ArenaRadius`), dove non c'e' geometria da ottimizzare: il conteggio
 * delle celle buie offerte non si muove di una cella, e il test resta verde.
 *
 * 🔑 **L'innesco vero e' `BLIND-1` uscita *(c)*, cioe' [#2793]**: diventa rosso il giorno in cui il
 * ventaglio si filtra per conoscenza e smette di offrire cio' che il velo non disegna. E' lo stesso innesco
 * dei due test in testa al file — questo misura il canale **diretto**, non quello indiretto della
 * geometria, e condivide con loro la conversione nel canary A/B/C.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsFanOffersDarkCellsTest,
	"RefactorTactics.BlindActions.ReachableFanOffersNeverObservedCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsFanOffersDarkCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeArena();
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}

	const FRTTeamKnowledge K = KnowledgeExploredWithinTwo(Map);
	const TSet<FRTCellId> Fan = FanCells(LonePlannerOn(Map, K));

	TArray<FRTCellId> Dark;
	for (const FRTCellId& Cell : Fan)
	{
		if (!IsLit(K, Cell)) { Dark.Add(Cell); }
	}
	Dark.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });

	AddInfo(FString::Printf(TEXT("ricordo fino a distanza 2 — ventaglio: %d celle, di cui MAI OSSERVATE: %d"),
		Fan.Num(), Dark.Num()));

	// 🔴 **La premessa dice cio' che serve davvero, e la prima stesura no** — trovato in code review.
	// Diceva `Fan.Num() > ExploredCells.Num() / 2`, cioe' `61 > 9`: passava per un fattore sei, e avrebbe
	// continuato a passare proprio nel caso che esiste per escludere — un budget abbassato, o un
	// `ReachableCells` filtrato, che confinasse il ventaglio dentro il ricordo. La misura sotto sarebbe
	// allora fallita dicendo «il leak e' chiuso» quando la causa vera e' «misura fuori portata».
	//
	// ⚠️ E la premessa non puo' essere «una cella del ventaglio sta oltre la distanza 2»: con questa
	// conoscenza sarebbe **la misura stessa**, cioe' circolare. Deve parlare del BANCO, non dell'esito:
	// la board ha celle oltre il ricordo, e il budget basta a uscirne.
	int32 CellsBeyondMemory = 0;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (URTHexLibrary::HexDistance(PlannerCell, Cell.Id) > 2) { ++CellsBeyondMemory; }
	}
	if (!TestTrue(TEXT("premessa: la board HA celle oltre il ricordo, e il budget basta a uscirne"),
		CellsBeyondMemory > 0 && PlannerBudget > 2))
	{
		return false;
	}

	// La misura, asserita come COMPORTAMENTO CORRENTE.
	TestTrue(TEXT("oggi il ventaglio OFFRE celle che il velo non disegna: la portata conosce il buio"),
		Dark.Num() > 0);

	return true;
}

// =========================================================================================================
// IL CANARY A TRE MONDI — il footprint dell'AoE e il reason code
// =========================================================================================================
//
// La Definition of Done di `#2793` elenca i canali osservabili del Planning e **chiude l'elenco** (*«non un
// "almeno"»*). Sono, per nome: **ventaglio · percorso · costo · footprint dell'AoE · reason code · `CanTarget`
// e lo stato del cursore**. I primi tre li misurano i due test in testa al file, ora estesi al terzo mondo;
// i due qui sotto coprono footprint e reason code, sullo **stesso banco** e con le **stesse** premesse —
// arena piatta di raggio 4, conoscenza dell'osservatore identica nei tre mondi (asserita, vedi
// `ThreeWorldsAllOfferANonEmptyPlan`), nascosti `Rejected` per `ClassifyTarget`.
//
// ⛔ **`CanTarget` e lo stato del cursore NON sono misurati qui, e la ragione e' strutturale — non una
// dimenticanza** (rilievo di code review, 2026-09-21).
//
//   - **`CanTarget` non puo' differire, e asserirlo sarebbe una tautologia.**
//     `URTCombatLibrary::CanTargetHexCell` (`Combat/RTCombatLibrary.h:532`) e' una riga sola —
//     `return ClassifyHexTargeting(Map, From, To, RangeCells, Policy) == ERTHexTargetReason::Ok;`
//     (`RTCombatLibrary.cpp:250`) — e la sua firma non porta **nessuno** snapshot, **nessuna** unita',
//     **nessuna** occupazione. I tre mondi differiscono solo per `Snapshot.Occupancy`: un confronto a tre
//     mondi su quella chiamata passerebbe **anche a filtro rotto**, cioe' sarebbe precisamente il difetto
//     che questo file denuncia due volte (vedi `ReachableFanOffersNeverObservedCells`). Il punto in cui la
//     conoscenza entra nel targeting e' `RefusalForObserver`, che e' `ERTTargetRefusal` — trattato sotto.
//   - **Lo stato del cursore si misura nei suoi ingressi, non nella sua stringa.** Il readout compone
//     `ClassifyProbeCell` e il costo del percorso; entrambi sono asseriti a tre mondi qui e in testa al
//     file. La stringa in se' vive in `ARTPlayerController` e in `RefactorTacticsEditor`, fuori da un banco
//     puro.
//
// ∴ dei canali che la DoD chiude in elenco, questo banco ne misura quattro e ne dichiara due — uno perche'
// non puo' perdere, uno perche' e' gia' misurato a monte. Nessuno resta taciuto.
//
// ⛔ **Anche qui non si decide e non si corregge.** Vale l'avvertenza in testa al file: `BLIND-2` (*dove*
// vive il filtro) e `OBS-1` (*di chi* e' la vista) sono aperte, e `#2793` e' ferma su quelle. Questi test
// dicono **quali canali perdono e quali no**, con l'evidenza eseguibile accanto.
//
// ## 🔴 Quali enum entrano nel confronto dei reason code — la riga che la DoD chiede per SIMBOLO
//
// La DoD di `#2793` porta: *«nominato per SIMBOLO, non per categoria […] chi implementa dichiara **quali**
// enum entrano nel confronto»*. Questa e' la dichiarazione, e vale finche' qualcuno non la cambia qui.
//
//     ENTRANO
//       ERTHexWaypointReason   (`Turn/RTHexSimLibrary.h:18`)  — Ok · NotOnMap · BlocksMovement · Occupied
//       ERTHexProbeExclusion   (`Turn/RTHexSimLibrary.h:41`)  — Reachable · NotOnMap · BlocksMovement ·
//                                                               Occupied · OutOfBudget · NoRoute
//
//     NON ENTRANO, e il motivo e' diverso per ciascuno
//       ERTTargetRefusal       (`Combat/RTCombatLibrary.h:71`)
//       ERTMoveOutcome         (`Turn/RTTurnLog.h:448`)
//
// **Perche' `ERTHexWaypointReason` e `ERTHexProbeExclusion` entrano.** Sono i due reason code che il
// Planning **calcola sull'occupazione** e mostra a chi pianifica: il primo spiega il rifiuto di un
// waypoint, il secondo risponde a *«perche' quella cella no»* nella sonda di movimento (#711). Entrambi
// leggono `Snapshot.Occupancy` **direttamente** — `ClassifyWaypointCell` a `RTHexSimLibrary.cpp:626`,
// `ClassifyProbeCell` di nuovo tramite `BlockedCellsFor` a `:692` — e nessuno dei due vede
// `Snapshot.TeamKnowledge`. 🔑 **E' la forma piu' esplicita del leak di tutto il file**: non una cella che
// manca da un insieme, ma un rifiuto **nominato** — il gioco risponde `Occupied` a proposito di una cella
// di cui il giocatore non ha diritto di sapere che sia occupata.
//
// **Perche' `ERTTargetRefusal` NON entra, pur essendo un reason code di Planning.** E' il solo dei quattro
// gia' progettato per la privacy: `RefusalForObserver` (`Combat/RTCombatLibrary.h:580`) prende il verdetto
// interno **piu' il flag di conoscenza** e collassa su `Nothing`, che esiste apposta per non distinguere
// cella vuota da cella con ignoto. ⛔ Ma la coppia `ClassifyHexTargeting` + `RefusalForObserver` non e'
// composta da una funzione pura: la compone `ARTPlayerController` (`Player/RTPlayerController.cpp:324-328`),
// e `MakePlanPreview` il valore lo **copia** dall'ingresso (`Turn/RTPlanPreview.cpp:151`). Un banco headless
// potrebbe solo ricomporla qui — cioe' aprire una **seconda sede** della stessa regola, che e' il difetto
// che `#711` e [D-242] esistono per impedire. ∴ resta fuori **per costruzione del banco**, non perche' sia
// pulito, ed e' un `FOLLOW-UP CANDIDATE` di `#2793`: il suo canary vive dove la coppia si compone.
//
// **Perche' `ERTMoveOutcome` NON entra, e perche' la DoD lo nominava.** Il corpo di `#2793` lo elenca fra
// le *«almeno tre famiglie»* di reason code. Misurato: **non e' un canale di Planning**. E' l'esito del
// *resolver*, scritto dopo il commit del turno — `FinalizeHexMovementOutcomes`
// (`Turn/RTHexSimLibrary.cpp:1456`) e i siti di `ARTTurnManager` — e chi pianifica non lo ha davanti.
// Confrontarlo nel canary dell'anteprima misurerebbe un dato che il giocatore non vede. ⚠️ **Non e'
// «pulito»**: e' un canale **post-commit**, e se qualcuno vorra' sorvegliarlo servira' un banco proprio,
// che parte da un turno risolto e non da uno snapshot di Planning.
//
// ## LA MISURA — 2026-09-21, clone `refactor-tactics-dev`, base `781020a1` piu' le modifiche di questo
// commit, motore libero, run dichiarata VALIDA (`Found 7`, `Success=7`, `Fail=0`)
//
//     canale                                    A (0 nascosti)   B (1)         C (2)          identico?
//     ventaglio raggiungibile                   61 celle         60            59                 NO
//       celle perse rispetto ad A               —                (2,0)         (2,-2) (2,0)
//     percorso (0,0) -> (4,0)                   diretto          devia         devia              NO
//     costo mostrato                            4                5             5                  NO
//     footprint AoE (area r=2 su (2,0))         19 celle         19            19                SI'
//       origine mostrata                        (0,0,L0)         (0,0,L0)      (0,0,L0)          SI'
//       celle alleate investite                 0                0             0                 SI'
//     ERTHexWaypointReason  su (2,0)            Ok               Occupied      Occupied           NO
//     ERTHexProbeExclusion  su (2,0)            Reachable        Occupied      Occupied           NO
//     ERTHexWaypointReason  su (2,-2)           Ok               —             Occupied           NO
//
// 🔴 **Quattro canali su cinque perdono, e il quinto e' pulito per una ragione strutturale, non per
// fortuna.** Il footprint non si muove perche' `HexHitCells` non ha unita' nella firma — e' l'unico dei
// cinque in cui l'occupazione non puo' entrare nemmeno volendo.
//
// 🔑 **E il reason code e' il canale piu' stretto di tutti, piu' del costo.** Il costo che sale di uno va
// interpretato; `Occupied` no: e' il nome esatto di cio' che il giocatore non ha diritto di sapere, detto
// dal gioco in una parola.
//
// ## LA VERIFICA DI MUTAZIONE — stessa data, una mutazione per volta, con rebuild fra l'una e l'altra
//
// Due mutazioni, entrambe applicate al codice di **produzione**, misurate e **ripristinate**. Servono a
// dire che i due test nuovi non sono verdi per costruzione: `Found 7` in tutte e tre le run.
//
//     (1) footprint AoE — `MakeBlastPreview`, il filtro alleati `Other.TeamId != Attacker.TeamId`
//         rovesciato in `==` (`Combat/RTHexCombatLibrary.cpp:906`)
//         -> ROSSO, e SOLO `ThreeWorldsAgreeOnTheAoEFootprint` (`Success=6 Fail=1`)
//         -> `AllyCells`: 0 / 1 / 2 celle nei tre mondi — la stessa firma del ventaglio, una cella per
//            nascosto. L'asserzione caduta e' quella sulle celle alleate.
//
//     (2) reason code — `ClassifyWaypointCell` privata della lettura di `Snapshot.Occupancy`
//         (`Turn/RTHexSimLibrary.cpp:626`)
//         -> ROSSO, e SOLO `ThreeWorldsDisagreeOnTheRefusalSymbol` (`Success=6 Fail=1`)
//
// 🔴 **E la seconda mutazione ha misurato qualcosa che nessuno aveva chiesto, ma che chi implementa il
// filtro deve sapere prima di cominciare.** Con `ClassifyWaypointCell` cieca all'occupazione i simboli
// diventano:
//
//     cella (2,0,L0)   ERTHexWaypointReason    A=Ok         B=Ok        C=Ok          <- chiuso
//     cella (2,0,L0)   ERTHexProbeExclusion    A=Reachable  B=NoRoute   C=NoRoute     <- APERTO
//
// `ERTHexProbeExclusion` **continua a distinguere i tre mondi**: ha smesso di dire `Occupied` e ha
// cominciato a dire `NoRoute`, perche' il suo secondo ramo interroga `BlockedCellsFor` (`:692`) che
// l'occupazione la legge ancora. ∴ **un filtro messo in una sede sola non chiude il canale: gli cambia
// nome**, e il nome nuovo e' pure *falso* — non c'e' nessuna strada mancante, c'e' un'unita' che
// l'osservatore non conosce. E' la stessa lezione delle due sedi di `ReachableCells` in testa a questo
// file, misurata una seconda volta su un enum invece che su un insieme.

namespace
{
	/**
	 * Le stesse unita' dei tre mondi, nel tipo che il Blast usa.
	 *
	 * ⚠️ **Due tipi e non uno, ed e' del repository, non del banco**: il movimento gira su `FRTHexSimUnit`
	 * (`Turn/RTHexSim.h`) e il combattimento su `FRTHexCombatUnit` (`Combat/RTHexCombatLibrary.h:20`).
	 * `WorldWith` costruisce i primi; questo costruisce i secondi **dalle stesse celle e dalle stesse
	 * squadre**, o i due canali misurerebbero due allestimenti diversi.
	 */
	TArray<FRTHexCombatUnit> CombatWorldWith(int32 HiddenCount)
	{
		auto Make = [](int32 UnitId, const FRTCellId& Cell, int32 TeamId)
		{
			FRTHexCombatUnit U;
			U.UnitId = UnitId;
			U.TeamId = TeamId;
			U.Cell = Cell;
			U.bAlive = true;
			return U;
		};

		TArray<FRTHexCombatUnit> Units;
		Units.Add(Make(PlannerId, PlannerCell, ObserverTeam));
		if (HiddenCount >= 1) { Units.Add(Make(HiddenIdA, HiddenCellA, HiddenTeam)); }
		if (HiddenCount >= 2) { Units.Add(Make(HiddenIdB, HiddenCellB, HiddenTeam)); }
		return Units;
	}

	/**
	 * Il piano d'attacco identico nei tre mondi: un'area di raggio 2 puntata sulla cella `HiddenCellA`.
	 *
	 * 🔑 **Mirare a una CELLA e non a un'unita' e' cio' che rende il caso legale**, ed e' la stessa
	 * disciplina di `Fallback.AttackCell`: l'osservatore non conosce i nascosti, quindi non puo' sceglierli
	 * come bersaglio — ma la cella e' `Explored`, e per [D-227] puntarla e' un'azione che il Planning gli
	 * concede. E' esattamente il caso in cui un footprint che cambiasse direbbe *«li' c'e' qualcuno»*.
	 *
	 * `bFriendlyFire` acceso di proposito: accende anche il ramo `AllyCells`, che e' l'unico punto di
	 * `MakeBlastPreview` in cui le altre unita' entrano davvero (`RTHexCombatLibrary.cpp:906`). Spento,
	 * quel ramo non girerebbe e il canale resterebbe non misurato.
	 */
	constexpr int32 BlastAreaRadius = 2;
	constexpr int32 BlastRangeCells = 4;

	FRTBlastPreviewPlan BlastAtHiddenCell()
	{
		FRTBlastPreviewPlan Plan;
		Plan.AttackerId = 0;              // indice in `Units`, non `UnitId`: e' chi pianifica, sempre primo
		Plan.bHasAction = true;
		Plan.bTargetsCell = true;
		Plan.TargetCell = HiddenCellA;
		Plan.Shape = ERTAbilityShape::Area;
		Plan.RangeCells = BlastRangeCells;
		Plan.AreaRadius = BlastAreaRadius;
		Plan.bFriendlyFire = true;
		return Plan;
	}
}

/**
 * ✅ **VERDE PERCHE' IL CANALE E' PULITO**, ed e' l'unico test del file che lo sia.
 *
 * Il footprint dell'AoE **non cambia** per un'occupazione che l'osservatore non conosce, e non e' un caso:
 * `HexHitCells` (`Combat/RTHexCombatLibrary.h:585`) non ha nessuna unita' nella propria firma — e' geometria
 * su `Shape/From/Target/RangeCells/AreaRadius` — e `MakeBlastPreview` tocca le unita' in tre punti soli:
 * l'origine dell'attaccante, la cella del bersaglio quando `!bTargetsCell`, e `AllyCells`, che filtra sulla
 * squadra dell'attaccante (`cpp:906`). Un nemico **avversario** e **ignoto** non entra in nessuno dei tre.
 *
 * 🔴 **E per questo il test ha bisogno della sua anti-vacuita', piu' degli altri.** «Identico nei tre
 * mondi» e' vero *per costruzione*: senza la premessa che le celle dei nascosti **cadano dentro il
 * footprint**, un delta nullo non direbbe «nessun leak», direbbe «misura fuori portata» — lo stesso errore
 * che il test del ventaglio evita con `FanA.Contains(HiddenCellA)`.
 *
 * ⚠️ **Cosa NON prova.** Che il canale resti pulito: lo prova finche' `MakeBlastPreview` non impara a
 * leggere l'occupazione. Il giorno in cui un'anteprima volesse mostrare *«qui dentro c'e' qualcuno»* questo
 * test diventerebbe rosso, e sarebbe il posto giusto in cui accorgersene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsAoEFootprintIsCleanTest,
	"RefactorTactics.BlindActions.ThreeWorldsAgreeOnTheAoEFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsAoEFootprintIsCleanTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}
	if (!ObserverIsIgnorantOfHidden(*this, Map))
	{
		return false;
	}

	const FRTBlastPreviewPlan Plan = BlastAtHiddenCell();
	const FRTBlastPreview PrevA = URTHexCombatLibrary::MakeBlastPreview(Plan, CombatWorldWith(0));
	const FRTBlastPreview PrevB = URTHexCombatLibrary::MakeBlastPreview(Plan, CombatWorldWith(1));
	const FRTBlastPreview PrevC = URTHexCombatLibrary::MakeBlastPreview(Plan, CombatWorldWith(2));

	AddInfo(FString::Printf(TEXT("mondo A — nessun nascosto: origine (%d,%d,L%d), %d celle investite, %d alleate"),
		PrevA.Origin.X, PrevA.Origin.Y, PrevA.Origin.Layer, PrevA.HitCells.Num(), PrevA.AllyCells.Num()));
	AddInfo(FString::Printf(TEXT("mondo B — un nascosto:     origine (%d,%d,L%d), %d celle investite, %d alleate"),
		PrevB.Origin.X, PrevB.Origin.Y, PrevB.Origin.Layer, PrevB.HitCells.Num(), PrevB.AllyCells.Num()));
	AddInfo(FString::Printf(TEXT("mondo C — due nascosti:    origine (%d,%d,L%d), %d celle investite, %d alleate"),
		PrevC.Origin.X, PrevC.Origin.Y, PrevC.Origin.Layer, PrevC.HitCells.Num(), PrevC.AllyCells.Num()));

	// 🔴 **L'anti-vacuita', e senza questa il test non misura niente.** Il footprint deve COPRIRE le celle
	// dei due nascosti: solo allora «identico nei tre mondi» e' una proprieta' invece di una tautologia.
	if (!TestTrue(TEXT("premessa: il footprint di A copre ENTRAMBE le celle dei nascosti — il banco puo' vedere un delta"),
		PrevA.HitCells.Contains(HiddenCellA) && PrevA.HitCells.Contains(HiddenCellB)))
	{
		AddError(FString::Printf(TEXT("footprint di A: %s"), *DescribeCells(PrevA.HitCells)));
		return false;
	}
	// Seconda anti-vacuita': il ramo del fuoco amico deve essere ACCESO, o `AllyCells` sarebbe vuoto per
	// una ragione che non c'entra con la conoscenza.
	if (!TestTrue(TEXT("premessa: il piano dichiara fuoco amico, quindi il ramo AllyCells gira davvero"),
		Plan.bFriendlyFire))
	{
		return false;
	}

	// LA MISURA: `PlanningView(A) == PlanningView(B) == PlanningView(C)` sul canale del footprint.
	TestTrue(TEXT("il footprint dell'AoE e' IDENTICO nei tre mondi: nessuna cella investita cambia"),
		PrevA.HitCells == PrevB.HitCells && PrevB.HitCells == PrevC.HitCells);
	TestTrue(TEXT("l'origine mostrata e' IDENTICA nei tre mondi"),
		PrevA.Origin == PrevB.Origin && PrevB.Origin == PrevC.Origin);
	TestTrue(TEXT("l'elenco delle celle alleate investite e' IDENTICO nei tre mondi"),
		PrevA.AllyCells == PrevB.AllyCells && PrevB.AllyCells == PrevC.AllyCells);

	return true;
}

/**
 * ⚠️ **VERDE PERCHE' IL DIFETTO C'E'**, come i due test in testa al file, e diventa rosso allo stesso
 * innesco: il giorno in cui il Planning classifica per conoscenza invece che sull'occupazione autorevole.
 *
 * 🔴 **E' il canale piu' esplicito dei cinque.** Il ventaglio toglie una cella e il percorso gira intorno:
 * il giocatore **deduce**. Qui il gioco **lo dice**: alla domanda «perche' non posso mettere il waypoint
 * li'?» risponde, con un simbolo, `Occupied`. Non «fuori portata», non «non si passa»: *occupata*. Una cella
 * che l'osservatore ha esplorato e su cui, per quanto ne sa, non c'e' nessuno.
 *
 * Gli enum che entrano nel confronto sono dichiarati nel blocco in testa a questa sezione. In breve:
 * `ERTHexWaypointReason` e `ERTHexProbeExclusion` si'; `ERTTargetRefusal` e `ERTMoveOutcome` no, con la
 * ragione scritta accanto a ciascuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsRefusalSymbolLeaksTest,
	"RefactorTactics.BlindActions.ThreeWorldsDisagreeOnTheRefusalSymbol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsRefusalSymbolLeaksTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}
	if (!ObserverIsIgnorantOfHidden(*this, Map))
	{
		return false;
	}

	const UEnum* WaypointEnum = StaticEnum<ERTHexWaypointReason>();
	const UEnum* ProbeEnum    = StaticEnum<ERTHexProbeExclusion>();
	if (!TestTrue(TEXT("premessa: i due enum del confronto sono riflessi, quindi il referto ne stampa il SIMBOLO"),
		WaypointEnum != nullptr && ProbeEnum != nullptr))
	{
		return false;
	}

	const FRTHexSnapshot WorldA = WorldWith(Map, 0);
	const FRTHexSnapshot WorldB = WorldWith(Map, 1);
	const FRTHexSnapshot WorldC = WorldWith(Map, 2);

	// `ClassifyProbeCell` vuole il set gia' calcolato: e' il criterio portante di #711 — nessuna seconda
	// ricerca. Ogni mondo porta il proprio, come lo porterebbe la sonda in partita.
	const TArray<FRTHexReachableCell> ReachA = URTHexSimLibrary::ReachableCells(WorldA, PlannerId);
	const TArray<FRTHexReachableCell> ReachB = URTHexSimLibrary::ReachableCells(WorldB, PlannerId);
	const TArray<FRTHexReachableCell> ReachC = URTHexSimLibrary::ReachableCells(WorldC, PlannerId);

	const ERTHexWaypointReason WpA = URTHexSimLibrary::ClassifyWaypointCell(WorldA, PlannerId, HiddenCellA);
	const ERTHexWaypointReason WpB = URTHexSimLibrary::ClassifyWaypointCell(WorldB, PlannerId, HiddenCellA);
	const ERTHexWaypointReason WpC = URTHexSimLibrary::ClassifyWaypointCell(WorldC, PlannerId, HiddenCellA);

	const ERTHexProbeExclusion PrA = URTHexSimLibrary::ClassifyProbeCell(WorldA, PlannerId, ReachA, HiddenCellA);
	const ERTHexProbeExclusion PrB = URTHexSimLibrary::ClassifyProbeCell(WorldB, PlannerId, ReachB, HiddenCellA);
	const ERTHexProbeExclusion PrC = URTHexSimLibrary::ClassifyProbeCell(WorldC, PlannerId, ReachC, HiddenCellA);

	auto Sym = [](const UEnum* E, int64 V) { return E->GetNameStringByValue(V); };
	AddInfo(FString::Printf(TEXT("cella (%d,%d,L%d) — ERTHexWaypointReason:  A=%s  B=%s  C=%s"),
		HiddenCellA.X, HiddenCellA.Y, HiddenCellA.Layer,
		*Sym(WaypointEnum, (int64)WpA), *Sym(WaypointEnum, (int64)WpB), *Sym(WaypointEnum, (int64)WpC)));
	AddInfo(FString::Printf(TEXT("cella (%d,%d,L%d) — ERTHexProbeExclusion: A=%s  B=%s  C=%s"),
		HiddenCellA.X, HiddenCellA.Y, HiddenCellA.Layer,
		*Sym(ProbeEnum, (int64)PrA), *Sym(ProbeEnum, (int64)PrB), *Sym(ProbeEnum, (int64)PrC)));

	// 🔴 **La premessa che rende leggibile il confronto**: nel mondo senza nascosti la cella non ha NIENTE
	// che non vada. Senza, un delta potrebbe venire dalla mappa invece che dall'occupazione.
	if (!TestTrue(TEXT("premessa: nel mondo A la cella e' Ok e raggiungibile — il rifiuto non viene dal terreno"),
		WpA == ERTHexWaypointReason::Ok && PrA == ERTHexProbeExclusion::Reachable))
	{
		return false;
	}

	// LA MISURA, asserita come COMPORTAMENTO CORRENTE, e nominata per SIMBOLO.
	TestTrue(TEXT("oggi ERTHexWaypointReason risponde Occupied su una cella che l'osservatore non sa occupata (B)"),
		WpB == ERTHexWaypointReason::Occupied);
	TestTrue(TEXT("oggi ERTHexWaypointReason risponde Occupied anche nel mondo a due nascosti (C)"),
		WpC == ERTHexWaypointReason::Occupied);
	TestTrue(TEXT("oggi ERTHexProbeExclusion risponde Occupied dove l'osservatore non sa esserci nessuno (B)"),
		PrB == ERTHexProbeExclusion::Occupied);
	TestTrue(TEXT("oggi ERTHexProbeExclusion risponde Occupied anche nel mondo a due nascosti (C)"),
		PrC == ERTHexProbeExclusion::Occupied);

	// E il secondo nascosto porta lo stesso simbolo sulla propria cella: il canale non e' un caso isolato
	// della prima, ed e' cio' che distingue `C` da una ripetizione di `B`.
	const ERTHexWaypointReason WpC2 = URTHexSimLibrary::ClassifyWaypointCell(WorldC, PlannerId, HiddenCellB);
	const ERTHexWaypointReason WpA2 = URTHexSimLibrary::ClassifyWaypointCell(WorldA, PlannerId, HiddenCellB);
	AddInfo(FString::Printf(TEXT("cella (%d,%d,L%d) — ERTHexWaypointReason:  A=%s  C=%s"),
		HiddenCellB.X, HiddenCellB.Y, HiddenCellB.Layer,
		*Sym(WaypointEnum, (int64)WpA2), *Sym(WaypointEnum, (int64)WpC2)));
	TestTrue(TEXT("premessa: nel mondo A anche la seconda cella e' Ok"), WpA2 == ERTHexWaypointReason::Ok);
	TestTrue(TEXT("oggi il simbolo cambia anche sulla cella del SECONDO nascosto, solo nel mondo C"),
		WpC2 == ERTHexWaypointReason::Occupied);

	return true;
}

/**
 * Le **due** premesse fondanti del banco, che nessuno degli altri test poteva portare da solo.
 *
 * **(1) Nei tre mondi il piano non e' vuoto.** 🔑 Tre «non si puo' fare niente» sono identici, e
 * l'uguaglianza non proverebbe nulla — la lezione di `HexBotPlay.HiddenEnemyFairness`, dove senza
 * `bDidSomething` due «fermo» sarebbero passati per un canary verde. Vale per l'intero banco: ventaglio,
 * percorso, costo, footprint, reason code.
 *
 * **(2) I tre mondi sono identici per conoscenza autorizzata.** ➕ Aggiunta il 2026-09-21 in code review:
 * era la premessa che l'intera DoD di `#2793` presuppone — *«tre allestimenti identici per conoscenza
 * autorizzata»* — e nessun test la asseriva, mentre il commento in testa al file dichiarava una garanzia
 * che non esisteva. Se le tre conoscenze divergessero, ogni «oggi differisce» del banco resterebbe verde
 * per la ragione sbagliata, e dopo la conversione all'invariante diventerebbe rosso per una causa che non
 * e' il filtro.
 *
 * Vivono in un test proprio perche' un giorno qualcuno cambiera' `ArenaRadius`, `PlannerBudget` o
 * `ObserverKnowledge`, e questo diventera' rosso **prima** che gli altri diventino verdi per la ragione
 * sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindActionsPlanIsNotEmptyTest,
	"RefactorTactics.BlindActions.ThreeWorldsAllOfferANonEmptyPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindActionsPlanIsNotEmptyTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), ArenaRadius);
	if (!TestNotNull(TEXT("premessa: l'arena piatta esiste"), Map))
	{
		return false;
	}

	// 🔴 **La premessa fondante della DoD, che nessun test asseriva** — rilievo di code review del
	// 2026-09-21. *«Tre allestimenti IDENTICI per conoscenza autorizzata»* era **supposto**, e il commento in
	// testa al file dichiarava una garanzia che non c'era. Qui si misura: se le tre conoscenze divergessero,
	// ogni «oggi differisce» del banco resterebbe verde per la ragione sbagliata.
	const FRTHexSnapshot W0 = WorldWith(Map, 0);
	const FRTHexSnapshot W1 = WorldWith(Map, 1);
	const FRTHexSnapshot W2 = WorldWith(Map, 2);
	if (!TestTrue(TEXT("premessa: i tre mondi portano UNA conoscenza ciascuno, quella dell'osservatore"),
		W0.TeamKnowledge.Num() == 1 && W1.TeamKnowledge.Num() == 1 && W2.TeamKnowledge.Num() == 1))
	{
		return false;
	}
	auto StessaConoscenza = [](const FRTTeamKnowledge& L, const FRTTeamKnowledge& R)
	{
		return L.TeamId == R.TeamId
			&& L.TurnNumber == R.TurnNumber
			&& L.Version == R.Version
			&& L.VisibleCells == R.VisibleCells
			&& L.ExploredCells == R.ExploredCells
			&& L.Contacts.Num() == R.Contacts.Num();
	};
	AddInfo(FString::Printf(TEXT("conoscenza dell'osservatore: %d celle viste, %d esplorate, %d contatti"),
		W0.TeamKnowledge[0].VisibleCells.Num(), W0.TeamKnowledge[0].ExploredCells.Num(),
		W0.TeamKnowledge[0].Contacts.Num()));
	TestTrue(TEXT("i tre mondi sono IDENTICI per conoscenza autorizzata: differiscono solo per l'occupazione nascosta"),
		StessaConoscenza(W0.TeamKnowledge[0], W1.TeamKnowledge[0])
			&& StessaConoscenza(W1.TeamKnowledge[0], W2.TeamKnowledge[0]));

	for (int32 Hidden = 0; Hidden <= 2; ++Hidden)
	{
		const FRTHexSnapshot World = WorldWith(Map, Hidden);
		const TArray<FRTHexReachableCell> Reach = URTHexSimLibrary::ReachableCells(World, PlannerId);
		const FRTHexPathResult Path = URTHexSimLibrary::FindPathForUnit(World, PlannerId, GoalCell);
		const FRTBlastPreview Blast = URTHexCombatLibrary::MakeBlastPreview(BlastAtHiddenCell(), CombatWorldWith(Hidden));

		AddInfo(FString::Printf(TEXT("mondo con %d nascosti: %d celle nel ventaglio, percorso %s, %d celle investite"),
			Hidden, Reach.Num(), *DescribePath(Path), Blast.HitCells.Num()));

		TestTrue(*FString::Printf(TEXT("mondo %d: il ventaglio offre almeno una destinazione"), Hidden),
			Reach.Num() > 1);
		TestTrue(*FString::Printf(TEXT("mondo %d: un percorso verso il fondo ESISTE"), Hidden),
			Path.Status == ERTHexPathStatus::Success);
		TestTrue(*FString::Printf(TEXT("mondo %d: l'area dichiarata investe almeno una cella"), Hidden),
			Blast.HitCells.Num() > 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
