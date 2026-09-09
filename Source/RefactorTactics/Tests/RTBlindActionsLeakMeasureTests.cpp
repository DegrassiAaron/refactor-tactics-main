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
// `URTHexSimLibrary::BlockedCellsFor` (`Turn/RTHexSimLibrary.cpp`) costruisce le celle impercorribili
// scorrendo `Snapshot.Occupancy` per intero, e l'unico criterio di esclusione e' «non sono io». Nessun
// filtro di conoscenza. Lo consumano `ReachableCells` e `FindPathForUnit`, e da li' l'anteprima arriva a
// schermo (`RTPlayerController.cpp`, `SetPreviewReachableCells`).
//
// I tre mondi differiscono SOLO per hidden occupancy; la `FRTTeamKnowledge` dell'osservatore e' la stessa
// oggetto, non una copia equivalente:
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
	/** Squadra dell'osservatore che pianifica. `FRTHexSimUnit` non porta il `TeamId`: vive qui. */
	constexpr int32 ObserverTeam = 0;
	/** Squadra dei nascosti. */
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

	/** Uno dei tre mondi: chi pianifica, piu' `HiddenCount` nascosti sulle celle dichiarate sopra. */
	FRTHexSnapshot WorldWith(const URTHexMapAsset* Map, int32 HiddenCount)
	{
		TArray<FRTHexSimUnit> Units;
		Units.Add(FRTHexSimUnit(PlannerId, PlannerCell, PlannerBudget));
		if (HiddenCount >= 1) { Units.Add(FRTHexSimUnit(2, HiddenCellA, /*MoveBudget*/ 0)); }
		if (HiddenCount >= 2) { Units.Add(FRTHexSimUnit(3, HiddenCellB, /*MoveBudget*/ 0)); }

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
		const bool bA = URTTeamKnowledgeLibrary::ClassifyTarget(K, 2, HiddenTeam, HiddenCellA)
			== ERTTargetKnowledge::Rejected;
		const bool bB = URTTeamKnowledgeLibrary::ClassifyTarget(K, 3, HiddenTeam, HiddenCellB)
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

	AddInfo(FString::Printf(TEXT("mondo A — nessun nascosto: %s"), *DescribePath(PathA)));
	AddInfo(FString::Printf(TEXT("mondo B — un nascosto:     %s"), *DescribePath(PathB)));

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

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
