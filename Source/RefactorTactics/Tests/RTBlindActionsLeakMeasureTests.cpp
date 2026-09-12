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
		Units.Add(FRTHexSimUnit(PlannerId, PlannerCell, PlannerBudget));
		FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, Units);
		Snapshot.TeamKnowledge.Add(K);
		return Snapshot;
	}
}

/**
 * ⚠️ **VERDE PERCHE' IL CANALE C'E'**, come i due test sopra. Diventa rosso il giorno in cui `BLIND-4` viene
 * chiusa nel verso *(b)* — «la geometria mai osservata non e' pubblica» — e qualcuno lo implementa.
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
 * ⚠️ **VERDE PERCHE' IL CANALE C'E'.** Se `BLIND-4` chiudesse nel verso *(b)*, questo test diventa rosso.
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

#endif // WITH_DEV_AUTOMATION_TESTS
