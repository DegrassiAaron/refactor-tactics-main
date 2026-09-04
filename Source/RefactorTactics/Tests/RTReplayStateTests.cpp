#include "Misc/AutomationTest.h"
#include "Replay/RTReplayStateLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Lo stato delle unita' ricostruito **dalla traccia** (`#1625`, criterio 3 per la parte misurabile).
 *
 * 🔴 **Il guardrail centrale di quella issue e' un'assenza**: *«nessun resolver, targeting, LOS o
 * pathfinding nella UI di playback»*, *«nessun `SetActorLocation` come esito: la posizione viene dalla
 * traccia»*. Questi test misurano che la posizione ricostruita sia **quella che la traccia dichiara** — non
 * che sia «giusta» secondo una regola, perche' una regola qui sarebbe gia' il secondo simulatore.
 *
 * ⚠️ **Il ViewModel dichiarava gia' l'invariante e un test la misura da `#2095`**
 * (`Playback.ViewModelDoesNotDependOnTheResolver`). Questo file non la ripete: aggiunge il livello sopra —
 * che i fatti ricostruiti siano i fatti scritti.
 */
namespace
{
	FRTTracedUnitState Unita(int32 UnitId, int32 Q, int32 R, ERTHexDirection Facing = ERTHexDirection::E)
	{
		FRTTracedUnitState U;
		U.UnitId = UnitId;
		U.Cell = FRTCellId(Q, R);
		U.Facing = Facing;
		return U;
	}

	FRTTurnLogEntry Mossa(int32 Turno, int32 UnitId, const FRTCellId& Da, const FRTCellId& A,
		ERTMoveOutcome Esito = ERTMoveOutcome::Moved)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = ERTMatchPhase::Move;
		E.Category = ERTLogCategory::Move;
		E.UnitId = UnitId;
		E.SrcCell = Da;
		E.TgtCell = A;
		E.Outcome = static_cast<uint8>(Esito);
		return E;
	}

	FRTTurnLogEntry Orientamento(int32 Turno, int32 UnitId, ERTHexDirection Verso)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = ERTMatchPhase::Prep;
		E.Category = ERTLogCategory::Facing;
		E.UnitId = UnitId;
		// 🔴 La direzione viaggia in `Amount`, non in `Outcome`: quello porta la CAUSA del riorientamento.
		E.Amount = static_cast<int32>(Verso);
		E.Outcome = static_cast<uint8>(ERTFacingOutcome::DeclaredInPlanning);
		return E;
	}

	const FRTTracedUnitState* Trova(const TArray<FRTTracedUnitState>& Stato, int32 UnitId)
	{
		for (const FRTTracedUnitState& U : Stato) { if (U.UnitId == UnitId) { return &U; } }
		return nullptr;
	}
}

/**
 * **La posizione ricostruita e' quella che la traccia dichiara, e si ferma dove ci si ferma.**
 *
 * ⛔ **ANTI-VACUITA': lo stesso stato iniziale e la stessa traccia, letti a due punti diversi, devono dare
 * risposte diverse.** Una ricostruzione che ignorasse il punto — restituendo sempre l'inizio o sempre la
 * fine — passerebbe qualunque asserzione su un solo punto: e' il modo in cui questo tipo di funzione si
 * rompe davvero, ed e' il motivo per cui il test ne interroga **tre**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStatePositionTest,
	"RefactorTactics.Replay.State.PositionComesFromTheTraceAtThatPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStatePositionTest::RunTest(const FString&)
{
	const TArray<FRTTracedUnitState> Inizio = { Unita(1, 0, 0), Unita(2, 5, 0) };

	TArray<FRTTurnLogEntry> Traccia;
	Traccia.Add(Mossa(1, /*UnitId*/ 1, FRTCellId(0, 0), FRTCellId(1, 0)));
	Traccia.Add(Mossa(2, /*UnitId*/ 1, FRTCellId(1, 0), FRTCellId(2, 0)));
	Traccia.Add(Mossa(3, /*UnitId*/ 1, FRTCellId(2, 0), FRTCellId(3, 0)));
	URTTurnLogLibrary::SortTurnLog(Traccia);

	// --- Prima di muoversi: lo schieramento di partenza ------------------------------------------------
	const TArray<FRTTracedUnitState> T0 =
		URTReplayStateLibrary::UnitsAtPosition(Traccia, Inizio, /*Turno*/ 0, ERTMatchPhase::Cleanup);
	if (const FRTTracedUnitState* U = Trova(T0, 1))
	{
		TestEqual(TEXT("al turno 0 l'unita' e' dove e' stata schierata"), U->Cell, FRTCellId(0, 0));
	}
	else { AddError(TEXT("l'unita' 1 manca dallo stato al turno 0")); }

	// --- A meta': dove la traccia l'ha portata FINO LI' -----------------------------------------------
	const TArray<FRTTracedUnitState> T2 =
		URTReplayStateLibrary::UnitsAtPosition(Traccia, Inizio, /*Turno*/ 2, ERTMatchPhase::Cleanup);
	if (const FRTTracedUnitState* U = Trova(T2, 1))
	{
		TestEqual(TEXT("al turno 2 e' alla seconda mossa, non alla terza"), U->Cell, FRTCellId(2, 0));
	}
	else { AddError(TEXT("l'unita' 1 manca dallo stato al turno 2")); }

	// --- Alla fine ------------------------------------------------------------------------------------
	const TArray<FRTTracedUnitState> TFine = URTReplayStateLibrary::UnitsAtEnd(Traccia, Inizio);
	if (const FRTTracedUnitState* U = Trova(TFine, 1))
	{
		TestEqual(TEXT("alla fine e' all'ultima cella dichiarata"), U->Cell, FRTCellId(3, 0));
	}
	else { AddError(TEXT("l'unita' 1 manca dallo stato finale")); }

	// --- Chi non si muove resta dov'era, e non sparisce ------------------------------------------------
	if (const FRTTracedUnitState* U = Trova(TFine, 2))
	{
		TestEqual(TEXT("chi la traccia non nomina resta dove era schierato"), U->Cell, FRTCellId(5, 0));
		TestTrue(TEXT("ed e' ancora in piedi"), U->bAlive);
	}
	else { AddError(TEXT("l'unita' 2 e' sparita: la traccia non l'ha mai nominata")); }

	return true;
}

/**
 * **Una mobilita' BLOCCATA sposta comunque**, e chi filtrasse sul solo `Moved` lascerebbe l'unita' disegnata
 * dove non e'.
 *
 * 🔴 L'enum lo dichiara riga per riga: `BlockedByUnit` e i suoi fratelli sono *«fermata (o parziale)»*.
 * `TgtCell` e' dove l'unita' e' **davvero arrivata**, non dove voleva andare — quindi una mobilita' parziale
 * e' un movimento a tutti gli effetti per chi disegna.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStateBlockedMoveTest,
	"RefactorTactics.Replay.State.ABlockedMoveStillMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStateBlockedMoveTest::RunTest(const FString&)
{
	const TArray<FRTTracedUnitState> Inizio = { Unita(1, 0, 0) };

	TArray<FRTTurnLogEntry> Traccia;
	// Voleva andare a (3,0), si e' fermata a (2,0) per una cella occupata: la traccia dichiara l'arrivo VERO.
	Traccia.Add(Mossa(1, 1, FRTCellId(0, 0), FRTCellId(2, 0), ERTMoveOutcome::BlockedByUnit));

	const TArray<FRTTracedUnitState> Fine = URTReplayStateLibrary::UnitsAtEnd(Traccia, Inizio);
	if (const FRTTracedUnitState* U = Trova(Fine, 1))
	{
		TestEqual(TEXT("una mobilita' parziale porta comunque dove `TgtCell` dice"), U->Cell, FRTCellId(2, 0));
	}
	else { AddError(TEXT("l'unita' 1 manca")); }

	// E `Stayed` non muove: e' l'altro verso, e senza di lui il test sopra passerebbe anche su un
	// «applica sempre `TgtCell`» che ignora del tutto l'esito.
	TArray<FRTTurnLogEntry> Ferma;
	Ferma.Add(Mossa(1, 1, FRTCellId(0, 0), FRTCellId(0, 0), ERTMoveOutcome::Stayed));
	const TArray<FRTTracedUnitState> Restata = URTReplayStateLibrary::UnitsAtEnd(Ferma, Inizio);
	if (const FRTTracedUnitState* U = Trova(Restata, 1))
	{
		TestEqual(TEXT("chi non pianificava movimento resta dov'era"), U->Cell, FRTCellId(0, 0));
	}

	return true;
}

/**
 * **Il facing si legge da `Amount`, non da `Outcome`** — e un consumatore che sbagliasse campo prenderebbe
 * la **causa** del riorientamento (`DerivedFromMove`, `DeclaredInPlanning`) al posto della direzione.
 *
 * ⛔ **ANTI-VACUITA': la voce dichiara una direzione il cui valore numerico NON coincide con la causa.**
 * `SW` vale `4`; `DeclaredInPlanning` vale `2`. Se il codice leggesse `Outcome` otterrebbe `NW`, e il test
 * lo vedrebbe. Con una direzione scelta a caso i due campi potrebbero coincidere e il difetto passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStateFacingTest,
	"RefactorTactics.Replay.State.FacingComesFromAmountNotOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStateFacingTest::RunTest(const FString&)
{
	const TArray<FRTTracedUnitState> Inizio = { Unita(1, 0, 0, ERTHexDirection::E) };

	TArray<FRTTurnLogEntry> Traccia;
	Traccia.Add(Orientamento(1, /*UnitId*/ 1, ERTHexDirection::SW));

	// La premessa dell'anti-vacuita', resa esplicita: se coincidessero, questo test non proverebbe niente.
	TestNotEqual(TEXT("la direzione e la causa hanno valori numerici DIVERSI: il test puo' distinguerle"),
		static_cast<int32>(ERTHexDirection::SW),
		static_cast<int32>(ERTFacingOutcome::DeclaredInPlanning));

	const TArray<FRTTracedUnitState> Fine = URTReplayStateLibrary::UnitsAtEnd(Traccia, Inizio);
	if (const FRTTracedUnitState* U = Trova(Fine, 1))
	{
		TestEqual(TEXT("il facing e' quello dichiarato in `Amount`"), U->Facing, ERTHexDirection::SW);
	}
	else { AddError(TEXT("l'unita' 1 manca")); }

	return true;
}

/**
 * **L'elenco di cio' che viene ricostruito e' quello DICHIARATO**, non quello che il codice fa per caso.
 *
 * 🔴 Il criterio 3 di `#1625` ammette che una categoria sia *«dichiarata non resa con la sua ragione»*, e
 * l'header lo fa: tre famiglie ricostruite — posizione, facing, KO — e nove no, perche' sono **eventi e non
 * stato**. Senza questo test quella dichiarazione sarebbe prosa: qualcuno aggiungerebbe un ramo e l'elenco
 * scritto smetterebbe di descrivere il codice, in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStateDeclaredCoverageTest,
	"RefactorTactics.Replay.State.RenderedCategoriesAreTheDeclaredOnes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStateDeclaredCoverageTest::RunTest(const FString&)
{
	auto Voce = [](ERTLogCategory C, uint8 Outcome = 0)
	{
		FRTTurnLogEntry E;
		E.Category = C;
		E.Outcome = Outcome;
		return E;
	};

	// --- Le tre che cambiano lo stato -----------------------------------------------------------------
	TestTrue(TEXT("Move cambia lo stato"), URTReplayStateLibrary::EntryChangesUnitState(Voce(ERTLogCategory::Move)));
	TestTrue(TEXT("Facing cambia lo stato"), URTReplayStateLibrary::EntryChangesUnitState(Voce(ERTLogCategory::Facing)));
	TestTrue(TEXT("un colpo LETALE cambia lo stato"),
		URTReplayStateLibrary::EntryChangesUnitState(
			Voce(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Lethal))));

	// --- E il danno NON letale no: e' un evento, si disegna leggendo la voce ---------------------------
	TestFalse(TEXT("un colpo non letale non cambia lo stato ricostruito"),
		URTReplayStateLibrary::EntryChangesUnitState(
			Voce(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit))));

	// --- Le altre categorie sono dichiarate non rese ---------------------------------------------------
	// ⚠️ **Si itera l'ENUM e non un elenco scritto a mano**: una categoria aggiunta domani finisce qui da
	// sola, e chi la rendesse senza aggiornare la dichiarazione dell'header trova questo rosso.
	const TArray<ERTLogCategory> Rese = { ERTLogCategory::Move, ERTLogCategory::Facing, ERTLogCategory::Combat };
	const UEnum* Categorie = StaticEnum<ERTLogCategory>();
	if (!TestNotNull(TEXT("l'enum delle categorie e' riflesso"), Categorie)) { return false; }

	int32 NonRese = 0;
	for (int32 i = 0; i < Categorie->NumEnums() - 1; ++i) // -1: salta `_MAX`
	{
		const ERTLogCategory C = static_cast<ERTLogCategory>(Categorie->GetValueByIndex(i));
		if (Rese.Contains(C)) { continue; }

		++NonRese;
		TestFalse(*FString::Printf(TEXT("'%s' e' dichiarata non resa, e non ricostruisce stato"),
			*Categorie->GetNameStringByIndex(i)),
			URTReplayStateLibrary::EntryChangesUnitState(Voce(C)));
	}

	// Anti-vacuita': senza questa riga il ciclo passerebbe su un enum vuoto o mal riflesso.
	TestTrue(*FString::Printf(TEXT("ci sono categorie non rese da verificare: %d"), NonRese), NonRese >= 5);
	return true;
}

namespace
{
	/**
	 * Una fase `Move` con tre boundary e un arrivo finale, come la scrive il resolver da `#2260`.
	 *
	 * Ogni barriera porta l'unita' una cella piu' avanti, e l'arrivo di `BuildMoveLog` la porta oltre: le
	 * quattro posizioni sono **tutte diverse**, quindi un taglio sbagliato non puo' passare inosservato.
	 * ⛔ Se i boundary fossero indistinguibili, questi test passerebbero anche su un filtro che non filtra.
	 */
	TArray<FRTTurnLogEntry> TracciaConTreBoundary()
	{
		TArray<FRTTurnLogEntry> T;

		FRTTurnLogEntry B0 = Mossa(1, /*UnitId*/ 1, FRTCellId(0, 0), FRTCellId(1, 0));
		B0.MicroStepIndex = 0;
		T.Add(B0);

		FRTTurnLogEntry B1 = Mossa(1, /*UnitId*/ 1, FRTCellId(1, 0), FRTCellId(2, 0));
		B1.MicroStepIndex = 1;
		T.Add(B1);

		FRTTurnLogEntry B2 = Mossa(1, /*UnitId*/ 1, FRTCellId(2, 0), FRTCellId(3, 0));
		B2.MicroStepIndex = 2;
		T.Add(B2);

		// 🔴 L'arrivo aggregato: `BuildMoveLog` produce UNA voce per unita', dopo `FinishHexMovement`, e
		// quella voce non appartiene a nessuna barriera.
		FRTTurnLogEntry Arrivo = Mossa(1, /*UnitId*/ 1, FRTCellId(0, 0), FRTCellId(5, 0));
		Arrivo.MicroStepIndex = INDEX_NONE;
		T.Add(Arrivo);

		return T;
	}
}

/**
 * **Lo stato si indirizza al boundary, e i boundary sono distinguibili** (`#2272`).
 *
 * 🔴 **L'asserzione che porta il peso e' l'ultima**: lo stato a un boundary intermedio dev'essere
 * **diverso** da quello a fine fase. Un'implementazione che ignorasse la terza coordinata restituirebbe
 * ovunque lo stato finale, e ogni altra asserzione di questo file passerebbe lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStateAtBoundaryTest,
	"RefactorTactics.Replay.State.BoundaryIsAddressable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStateAtBoundaryTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Traccia = TracciaConTreBoundary();
	const TArray<FRTTracedUnitState> Iniziale = { Unita(1, 0, 0) };

	auto CellaAl = [&](int32 MicroStep) -> FRTCellId
	{
		const TArray<FRTTracedUnitState> S = URTReplayStateLibrary::UnitsAtBoundary(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move, MicroStep);
		const FRTTracedUnitState* U = Trova(S, 1);
		return U ? U->Cell : FRTCellId(-99, -99);
	};

	TestTrue(TEXT("al boundary 0 l'unita' ha fatto un passo"), CellaAl(0) == FRTCellId(1, 0));
	TestTrue(TEXT("al boundary 1 ne ha fatti due"), CellaAl(1) == FRTCellId(2, 0));
	TestTrue(TEXT("al boundary 2 ne ha fatti tre"), CellaAl(2) == FRTCellId(3, 0));

	// --- ⛔ ANTI-VACUITA': la fase intera e' un'ALTRA cosa ----------------------------------------------
	const TArray<FRTTracedUnitState> Fine = URTReplayStateLibrary::UnitsAtPosition(
		Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move);
	const FRTTracedUnitState* AFine = Trova(Fine, 1);
	if (TestNotNull(TEXT("l'unita' c'e' anche a fine fase"), AFine))
	{
		TestTrue(TEXT("a fine fase l'unita' e' all'arrivo aggregato"), AFine->Cell == FRTCellId(5, 0));
		TestTrue(TEXT("⛔ che NON e' lo stato del boundary 2: il taglio fine fa qualcosa"),
			AFine->Cell != CellaAl(2));
	}

	return true;
}

/**
 * **`playback ≡ seek`** — il criterio che `#1880` ha lasciato aperto, e che `#2260` non poteva chiudere.
 *
 * Riprodurre la traccia **barriera per barriera** fino a `N` e saltare direttamente a `N` devono produrre
 * lo **stesso stato**. E' la proprieta' che rende il seek una scorciatoia invece che una seconda verita':
 * se le due letture divergessero, il viewer mostrerebbe un mondo che dipende da **come** ci si e' arrivati,
 * e quello e' un secondo simulatore travestito da ottimizzazione.
 *
 * ⚠️ Il playback qui e' incrementale davvero: ogni passo riparte dallo stato prodotto dal precedente, non
 * da `Iniziale`. Un test che ripartisse ogni volta dall'inizio confronterebbe la funzione con se stessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPlaybackEqualsSeekTest,
	"RefactorTactics.Replay.State.PlaybackToBoundaryEqualsSeek",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPlaybackEqualsSeekTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Traccia = TracciaConTreBoundary();
	const TArray<FRTTracedUnitState> Iniziale = { Unita(1, 0, 0) };

	for (int32 Bersaglio = 0; Bersaglio <= 2; ++Bersaglio)
	{
		// --- SEEK: si salta direttamente ----------------------------------------------------------------
		const TArray<FRTTracedUnitState> Seek = URTReplayStateLibrary::UnitsAtBoundary(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move, Bersaglio);

		// --- PLAYBACK: si attraversa una barriera alla volta ---------------------------------------------
		TArray<FRTTracedUnitState> Playback = Iniziale;
		for (int32 Passo = 0; Passo <= Bersaglio; ++Passo)
		{
			Playback = URTReplayStateLibrary::UnitsAtBoundary(
				Traccia, Playback, /*Turno*/ 1, ERTMatchPhase::Move, Passo);
		}

		if (!TestEqual(FString::Printf(TEXT("boundary %d: stesso numero di unita'"), Bersaglio),
			Playback.Num(), Seek.Num()))
		{
			continue;
		}

		for (const FRTTracedUnitState& U : Seek)
		{
			const FRTTracedUnitState* P = Trova(Playback, U.UnitId);
			if (!TestNotNull(FString::Printf(TEXT("boundary %d: l'unita' %d c'e' in entrambi"),
				Bersaglio, U.UnitId), P))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("boundary %d: unita' %d, stessa cella"), Bersaglio, U.UnitId),
				P->Cell == U.Cell);
			TestEqual(FString::Printf(TEXT("boundary %d: unita' %d, stesso facing"), Bersaglio, U.UnitId),
				static_cast<int32>(P->Facing), static_cast<int32>(U.Facing));
			TestEqual(FString::Printf(TEXT("boundary %d: unita' %d, stesso stato vitale"), Bersaglio, U.UnitId),
				P->bAlive, U.bAlive);
		}
	}

	return true;
}

/**
 * ⚠️ **Una voce che non appartiene a un ciclo sta DOPO ogni boundary** (`#2272`, decisione d'autore).
 *
 * 🔴 Il difetto che questo test impedisce ha un'aria ragionevole: `INDEX_NONE` vale `-1`, e un confronto
 * numerico lo mette **prima** dello `0`. Seguirlo applicherebbe l'arrivo dell'unita' **prima** delle
 * barriere che ha attraversato per arrivarci — uno stato che la partita non ha mai attraversato.
 *
 * ⚠️ Il test misura anche il rovescio, che e' cio' che rende la regola coerente invece che comoda: le voci
 * `INDEX_NONE` delle fasi **precedenti** entrano eccome. Una fase conclusa non ha barriere ancora da
 * attraversare, e tagliarla sarebbe perdere fatti gia' avvenuti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStateBoundaryExcludesCyclelessTest,
	"RefactorTactics.Replay.State.CyclelessEntriesComeAfterEveryBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStateBoundaryExcludesCyclelessTest::RunTest(const FString&)
{
	// --- la voce senza ciclo della fase RICHIESTA resta fuori -------------------------------------------
	{
		const TArray<FRTTurnLogEntry> Traccia = TracciaConTreBoundary();
		const TArray<FRTTracedUnitState> Iniziale = { Unita(1, 0, 0) };

		const TArray<FRTTracedUnitState> AlBoundary = URTReplayStateLibrary::UnitsAtBoundary(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move, /*MicroStep*/ 2);
		const FRTTracedUnitState* U = Trova(AlBoundary, 1);
		if (TestNotNull(TEXT("l'unita' c'e'"), U))
		{
			TestTrue(TEXT("⛔ l'arrivo aggregato NON e' stato applicato"), U->Cell != FRTCellId(5, 0));
			TestTrue(TEXT("e lo stato e' quello dell'ultima barriera attraversata"), U->Cell == FRTCellId(3, 0));
		}
	}

	// --- ma quella di una fase PRECEDENTE entra --------------------------------------------------------
	{
		TArray<FRTTurnLogEntry> Traccia;

		// `Prep` viene prima di `Move` nell'ordine cronologico dell'enum, e la sua voce non ha micro-step.
		FRTTurnLogEntry Prima = Orientamento(/*Turno*/ 1, /*UnitId*/ 1, ERTHexDirection::NE);
		Prima.MicroStepIndex = INDEX_NONE;
		Traccia.Add(Prima);

		FRTTurnLogEntry B0 = Mossa(1, /*UnitId*/ 1, FRTCellId(0, 0), FRTCellId(1, 0));
		B0.MicroStepIndex = 0;
		Traccia.Add(B0);

		const TArray<FRTTracedUnitState> Iniziale = { Unita(1, 0, 0, ERTHexDirection::E) };
		const TArray<FRTTracedUnitState> AlBoundary = URTReplayStateLibrary::UnitsAtBoundary(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move, /*MicroStep*/ 0);

		const FRTTracedUnitState* U = Trova(AlBoundary, 1);
		if (TestNotNull(TEXT("l'unita' c'e'"), U))
		{
			TestEqual(TEXT("l'orientamento della fase precedente e' stato applicato"),
				static_cast<int32>(U->Facing), static_cast<int32>(ERTHexDirection::NE));
			TestTrue(TEXT("e anche il boundary 0 di questa fase"), U->Cell == FRTCellId(1, 0));
		}
	}

	// --- e `INDEX_NONE` come RICHIESTA significa la fase intera: il comportamento storico -----------------
	{
		const TArray<FRTTurnLogEntry> Traccia = TracciaConTreBoundary();
		const TArray<FRTTracedUnitState> Iniziale = { Unita(1, 0, 0) };

		const TArray<FRTTracedUnitState> Fine = URTReplayStateLibrary::UnitsAtBoundary(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move, INDEX_NONE);
		const TArray<FRTTracedUnitState> Storico = URTReplayStateLibrary::UnitsAtPosition(
			Traccia, Iniziale, /*Turno*/ 1, ERTMatchPhase::Move);

		const FRTTracedUnitState* A = Trova(Fine, 1);
		const FRTTracedUnitState* B = Trova(Storico, 1);
		if (TestNotNull(TEXT("c'e' in entrambe"), A) && TestNotNull(TEXT("e nell'altra"), B))
		{
			TestTrue(TEXT("chiedere INDEX_NONE == chiedere la fase intera"), A->Cell == B->Cell);
			TestTrue(TEXT("e include l'arrivo aggregato"), A->Cell == FRTCellId(5, 0));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
