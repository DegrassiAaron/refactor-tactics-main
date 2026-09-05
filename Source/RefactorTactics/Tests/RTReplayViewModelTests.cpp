#include "Misc/AutomationTest.h"
#include "Replay/RTReplayViewModel.h"
#include "Tests/RTReplayTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il view model del replay (`#472`) — la parte automatizzabile dei criteri di R6.
 *
 * ⚠️ **Guarda gli `#include`**: il view model e la fixture. Nessun `ARTTurnManager`, nessun
 * `URTHexSimLibrary`, nessun resolver, nessun `UWorld`, **nessun `UUserWidget`**. E' la forma negativa
 * che `#472` chiede al percorso della UI, la stessa che ADR-0009 §3 chiede al Player: se la posizione o
 * i comandi avessero bisogno del resolver o di un widget, questi test non potrebbero esistere in questa
 * forma. Un test si aggira con un `#include`; una dipendenza che non esiste no.
 *
 * 🔴 Gli helper vivevano qui e sono stati copiati — con un suffisso diverso e una funzione peggiorata —
 * nel file del ponte. Ora stanno in `RTReplayTestFixtures.h`, uno solo per entrambi. Trovato in code
 * review.
 */
using namespace RTReplayFixtures;


/**
 * `Replay.ViewModel.PositionIsNotTheCursor` — **il test centrale di #472**, e il difetto che ha fatto
 * riscrivere la issue.
 *
 * `AdvancePhase` emette le voci di una fase e lascia il cursore **oltre** di esse. Chi leggesse la
 * posizione dal cursore mostrerebbe quindi la fase SUCCESSIVA a quella che sta guardando, e — a fine
 * traccia — il turno successivo a quello in corso.
 *
 * Il test non si limita a controllare che il view model dica la cosa giusta: **misura anche cosa direbbe
 * il cursore**, sulla stessa sequenza e allo stesso punto. Senza quel secondo confronto sarebbe verde
 * anche su un'implementazione che legge dal cursore per caso in questo scenario, e il difetto che pretende
 * di pinnare tornerebbe senza far cadere niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelPositionTest,
	"RefactorTactics.Replay.ViewModel.PositionIsNotTheCursor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelPositionTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Posizione"));
	Pulisci(Root);
	const FGuid Id = ArchivioDueTurni(Root);

	FRTReplayViewModel VM;
	TestEqual(TEXT("apre"), VM.Open(Root, Id), ERTReplayOpenResult::Opened);

	// Prima di cominciare non si e' al turno 1: si e' PRIMA del turno 1, e sono due cose diverse.
	TestEqual(TEXT("stato iniziale"), VM.Position().State, ERTReplayPositionState::BeforeStart);
	TestFalse(TEXT("prima dell'inizio non c'e' un turno"), VM.Position().HasTurn());

	// Una fase avanti: si guarda Blast del turno 1.
	TestTrue(TEXT("prima fase"), VM.StepPhaseForward());
	TestEqual(TEXT("turno 1"), VM.Position().TurnNumber, 1);
	TestEqual(TEXT("fase Blast"), VM.Position().Phase, ERTMatchPhase::Blast);

	// Cosa direbbe il cursore nello stesso punto: la fase SUCCESSIVA, non quella mostrata.
	{
		FRTReplaySession Parallela;
		URTReplayPlayerLibrary::OpenArchive(Root, Id, Parallela);
		ERTMatchPhase Emessa = ERTMatchPhase::Planning;
		TArray<FRTTurnLogEntry> Voci;
		URTReplayPlayerLibrary::AdvancePhase(Parallela, Emessa, Voci);

		TestEqual(TEXT("il Player ha emesso Blast"), Emessa, ERTMatchPhase::Blast);
		const FRTTurnLogEntry& SottoIlCursore =
			Parallela.Traces[Parallela.Cursor.TraceIndex][Parallela.Cursor.EntryIndex];
		TestEqual(TEXT("ma il cursore punta gia' a Move"), SottoIlCursore.Phase, ERTMatchPhase::Move);
		TestNotEqual(TEXT("cursore != posizione mostrata"), SottoIlCursore.Phase, VM.Position().Phase);
	}

	// Ultima fase del turno 1. Qui il cursore esce dalla traccia: normalizzarlo — che e' cio' che una UI
	// farebbe per non leggere fuori range — lo porterebbe al turno 2 mentre si guarda ancora il turno 1.
	TestTrue(TEXT("seconda fase"), VM.StepPhaseForward());
	TestEqual(TEXT("ancora turno 1"), VM.Position().TurnNumber, 1);
	TestEqual(TEXT("fase Move"), VM.Position().Phase, ERTMatchPhase::Move);

	{
		FRTReplaySession Parallela;
		URTReplayPlayerLibrary::OpenArchive(Root, Id, Parallela);
		ERTMatchPhase Emessa = ERTMatchPhase::Planning;
		TArray<FRTTurnLogEntry> Voci;
		URTReplayPlayerLibrary::AdvancePhase(Parallela, Emessa, Voci);
		URTReplayPlayerLibrary::AdvancePhase(Parallela, Emessa, Voci);

		TestEqual(TEXT("il cursore ha esaurito la traccia"),
			Parallela.Cursor.EntryIndex, Parallela.Traces[Parallela.Cursor.TraceIndex].Num());

		// La normalizzazione che una UI scriverebbe per non leggere fuori range. ⚠️ E' la STESSA di
		// `AdvancePhase` — azzera l'indice a ogni salto di traccia — e va replicata cosi': una versione che
		// tenesse fermo `EntryIndex` scavalcherebbe anche la traccia successiva, che qui e' lunga uguale.
		int32 TracciaNormalizzata = Parallela.Cursor.TraceIndex;
		int32 VoceNormalizzata = Parallela.Cursor.EntryIndex;
		while (Parallela.Traces.IsValidIndex(TracciaNormalizzata)
			&& VoceNormalizzata >= Parallela.Traces[TracciaNormalizzata].Num())
		{
			++TracciaNormalizzata;
			VoceNormalizzata = 0;
		}

		if (TestTrue(TEXT("la normalizzazione approda su una traccia"),
			Parallela.Traces.IsValidIndex(TracciaNormalizzata)))
		{
			TestEqual(TEXT("e porta al turno 2"), Parallela.Traces[TracciaNormalizzata][0].TurnNumber, 2);
			TestNotEqual(TEXT("mentre si guarda ancora il turno 1"),
				Parallela.Traces[TracciaNormalizzata][0].TurnNumber, VM.Position().TurnNumber);
		}
	}

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.DeclaresPositionInAllFourStates` — il criterio «dichiara SEMPRE turno e fase».
 *
 * «Sempre» e' verificabile solo se gli stati sono enumerati: sono quattro, e questo test li visita tutti.
 * Il quarto — `Unaddressable` — usa una traccia che dichiara `TurnNumber == 0`, che e' il sentinella «non
 * dichiarato» delle tracce pre-v6 e non un turno: un archivio simile si apre e si guarda, ma non e'
 * indirizzabile per turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelFourStatesTest,
	"RefactorTactics.Replay.ViewModel.DeclaresPositionInAllFourStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelFourStatesTest::RunTest(const FString&)
{
	{
		const FString Root = TransientRoot(TEXT("QuattroStati"));
		Pulisci(Root);
		const FGuid Id = ArchivioDueTurni(Root);

		FRTReplayViewModel VM;
		VM.Open(Root, Id);

		// 1. BeforeStart
		TestEqual(TEXT("1/4 BeforeStart"), VM.Position().State, ERTReplayPositionState::BeforeStart);
		TestFalse(TEXT("senza fase"), VM.Position().HasPhase());
		TestFalse(TEXT("senza turno"), VM.Position().HasTurn());

		// 2. AtPhase
		VM.StepPhaseForward();
		TestEqual(TEXT("2/4 AtPhase"), VM.Position().State, ERTReplayPositionState::AtPhase);
		TestTrue(TEXT("con fase"), VM.Position().HasPhase());
		TestTrue(TEXT("con turno"), VM.Position().HasTurn());

		// 3. Ended — quattro fasi in tutto, quindi il quinto passo esce dalla sequenza.
		int32 Passi = 0;
		while (VM.StepPhaseForward() && Passi < 16) { ++Passi; }
		TestEqual(TEXT("3/4 Ended"), VM.Position().State, ERTReplayPositionState::Ended);
		TestFalse(TEXT("finita: nessuna fase corrente"), VM.Position().HasPhase());
		TestFalse(TEXT("finita: nessun turno corrente"), VM.Position().HasTurn());

		Pulisci(Root);
	}

	{
		// 4. Unaddressable — le voci dichiarano `0`, come una traccia scritta prima del formato v6.
		const FString Root = TransientRoot(TEXT("NonIndirizzabile"));
		Pulisci(Root);
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(Traccia(0, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		const FGuid Id = ScriviArchivio(Root, Tracce, true);

		FRTReplayViewModel VM;
		TestEqual(TEXT("un archivio pre-v6 si apre lo stesso"), VM.Open(Root, Id),
			ERTReplayOpenResult::Opened);
		TestTrue(TEXT("e si guarda"), VM.StepPhaseForward());

		TestEqual(TEXT("4/4 Unaddressable"), VM.Position().State, ERTReplayPositionState::Unaddressable);
		TestTrue(TEXT("la fase c'e'"), VM.Position().HasPhase());
		TestEqual(TEXT("ed e' Blast"), VM.Position().Phase, ERTMatchPhase::Blast);
		TestFalse(TEXT("il turno no"), VM.Position().HasTurn());
		TestEqual(TEXT("e non si spaccia per turno 0"), VM.Position().TurnNumber, 0);

		// Il seek al turno resta fail-closed: `0` non e' un bersaglio.
		TestEqual(TEXT("seek a 0 rifiutato"), VM.SeekToTurn(0), ERTReplaySeekResult::TurnNotFound);
		TestEqual(TEXT("la posizione non si e' mossa"), VM.Position().Phase, ERTMatchPhase::Blast);

		Pulisci(Root);
	}

	return true;
}

/**
 * `Replay.ViewModel.OnlyFiveObservablePhases` — `ERTMatchPhase` ne dichiara sette, il replay ne mostra
 * cinque.
 *
 * Nessun punto del resolver emette voci con `Planning` o `MatchEnded`: una UI che enumerasse l'enum
 * disegnerebbe due comandi che non possono funzionare. Il test pinna il **numero** e le due esclusioni,
 * cosi' che aggiungere una fase all'enum senza decidere se e' osservabile faccia cadere qualcosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelObservablePhasesTest,
	"RefactorTactics.Replay.ViewModel.OnlyFiveObservablePhases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelObservablePhasesTest::RunTest(const FString&)
{
	const TArray<ERTMatchPhase>& Fasi = FRTReplayViewModel::ObservablePhases();
	TestEqual(TEXT("cinque, non sette"), Fasi.Num(), 5);
	TestFalse(TEXT("Planning non e' osservabile"), Fasi.Contains(ERTMatchPhase::Planning));
	TestFalse(TEXT("MatchEnded non e' osservabile"), Fasi.Contains(ERTMatchPhase::MatchEnded));
	TestEqual(TEXT("in ordine cronologico: la prima e' Prep"), Fasi[0], ERTMatchPhase::Prep);
	TestEqual(TEXT("l'ultima e' Cleanup"), Fasi.Last(), ERTMatchPhase::Cleanup);

	const FString Root = TransientRoot(TEXT("FasiOsservabili"));
	Pulisci(Root);
	const FGuid Id = ArchivioDueTurni(Root);

	FRTReplayViewModel VM;
	VM.Open(Root, Id);
	VM.StepPhaseForward();

	// Una fase osservabile ma assente da QUESTA traccia: `PhaseNotFound`, ed e' l'esito corretto — il
	// turno c'e', quella fase non ha prodotto voci.
	TestEqual(TEXT("seek a Dash, assente dalla traccia"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Dash),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("non ha mosso la posizione"), VM.Position().Phase, ERTMatchPhase::Blast);

	TestEqual(TEXT("seek a Move riesce"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Move),
		ERTReplaySeekResult::Found);
	TestEqual(TEXT("e sposta"), VM.Position().Phase, ERTMatchPhase::Move);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.NonObservablePhaseIsRefusedNotFollowed` — la **guardia**, non la fixture.
 *
 * 🔴 Questo test esiste perche' la code review ha mostrato che le tre asserzioni su `Planning` e
 * `MatchEnded` nel test precedente non provavano niente: la traccia di prova non conteneva quelle voci,
 * quindi `PhaseNotFound` arrivava dal seek e sarebbe arrivato lo stesso **senza** alcuna guardia — come
 * dimostrava l'asserzione gemella su `Dash`, una fase osservabile che dava esito identico.
 *
 * Qui la traccia contiene **davvero** una voce `Planning`. Senza la guardia il seek la troverebbe, il
 * view model ci atterrerebbe, e da li' `PhasesInTrace` non ritroverebbe piu' la fase corrente: quattro
 * `Can*` a `false`, quattro `Step*` no-op, `Play` che non parte — il viewer bloccato senza uscita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelNonObservableGuardTest,
	"RefactorTactics.Replay.ViewModel.NonObservablePhaseIsRefusedNotFollowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelNonObservableGuardTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("GuardiaFasi"));
	Pulisci(Root);

	// ⚠️ Una traccia con voci `Planning` e `MatchEnded`. Il resolver oggi non ne emette — ma nessun punto
	// del codice lo impone, e il Player non lo verifica in apertura: e' una proprieta' documentata, non
	// garantita. Un archivio cosi' e' leggibile, quindi il view model deve reggerlo.
	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(Traccia(1, { ERTMatchPhase::Planning, ERTMatchPhase::Blast, ERTMatchPhase::MatchEnded }));
	const FGuid Id = ScriviArchivio(Root, Tracce, true);

	FRTReplayViewModel VM;
	TestEqual(TEXT("l'archivio si apre"), VM.Open(Root, Id), ERTReplayOpenResult::Opened);
	TestTrue(TEXT("e si comincia"), VM.StepPhaseForward());

	// La navigazione salta le due non osservabili: la prima fase raggiungibile e' Blast, non Planning.
	TestEqual(TEXT("la prima fase mostrata e' Blast"), VM.Position().Phase, ERTMatchPhase::Blast);
	TestEqual(TEXT("una sola fase osservabile nel turno"), VM.PhasesInCurrentTurn().Num(), 1);

	// Il seek esplicito le RIFIUTA, e la posizione non si muove.
	TestEqual(TEXT("seek a Planning rifiutato"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Planning),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("seek a MatchEnded rifiutato"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::MatchEnded),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("la posizione e' rimasta su Blast"), VM.Position().Phase, ERTMatchPhase::Blast);

	// E soprattutto: il view model e' ancora VIVO. E' questa l'asserzione che la guardia protegge — senza,
	// qui sarebbe tutto morto.
	TestTrue(TEXT("si puo' ancora andare avanti"), VM.CanStepPhaseForward());
	TestTrue(TEXT("e indietro"), VM.CanStepPhaseBackward());
	TestTrue(TEXT("e il passo avanti funziona"), VM.StepPhaseForward());
	TestEqual(TEXT("che porta a Ended, non a MatchEnded"), VM.Position().State,
		ERTReplayPositionState::Ended);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.CurrentPhaseEntriesMatchThePlayer` — cosa c'e' da disegnare.
 *
 * 🔴 L'accessor mancava: il view model diceva *dove* si e' senza dare nulla da mostrare, e `#472` chiede
 * di **guardare** una partita, non solo di sapere a che punto e'. Trovato in code review.
 *
 * Il test non si limita a contare le voci: le confronta con quelle che il Player emette per la stessa
 * fase. Se il view model ricavasse il contenuto per un'altra strada — filtrando la traccia a mano invece
 * di partire dal seek — i due insiemi divergerebbero al primo caso limite, e sarebbe quello a decidere
 * cosa il giocatore vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelEntriesTest,
	"RefactorTactics.Replay.ViewModel.CurrentPhaseEntriesMatchThePlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelEntriesTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Contenuto"));
	Pulisci(Root);

	// Due voci nella stessa fase: e' cio' che rende osservabile il raggruppamento.
	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(Traccia(1, { ERTMatchPhase::Blast, ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	const FGuid Id = ScriviArchivio(Root, Tracce, true);

	FRTReplayViewModel VM;
	TestEqual(TEXT("apre"), VM.Open(Root, Id), ERTReplayOpenResult::Opened);

	TestEqual(TEXT("prima di cominciare non c'e' niente da disegnare"), VM.CurrentPhaseEntries().Num(), 0);

	TestTrue(TEXT("prima fase"), VM.StepPhaseForward());
	const TArray<FRTTurnLogEntry> Mostrate = VM.CurrentPhaseEntries();
	TestEqual(TEXT("due voci nella fase Blast"), Mostrate.Num(), 2);

	// Le stesse che il Player emette per quella fase.
	{
		FRTReplaySession Parallela;
		URTReplayPlayerLibrary::OpenArchive(Root, Id, Parallela);
		ERTMatchPhase Emessa = ERTMatchPhase::Planning;
		TArray<FRTTurnLogEntry> DalPlayer;
		URTReplayPlayerLibrary::AdvancePhase(Parallela, Emessa, DalPlayer);

		TestEqual(TEXT("stessa fase del Player"), Emessa, VM.Position().Phase);
		if (TestEqual(TEXT("stesso numero di voci"), Mostrate.Num(), DalPlayer.Num()))
		{
			for (int32 i = 0; i < Mostrate.Num(); ++i)
			{
				TestEqual(TEXT("stessa voce"), Mostrate[i].Amount, DalPlayer[i].Amount);
				TestEqual(TEXT("stessa fase"), Mostrate[i].Phase, DalPlayer[i].Phase);
			}
		}
	}

	// La barra dei salti di fase legge le fasi presenti, non l'enum.
	const TArray<ERTMatchPhase> Fasi = VM.PhasesInCurrentTurn();
	TestEqual(TEXT("il turno ha due fasi, non cinque"), Fasi.Num(), 2);
	TestEqual(TEXT("Blast"), Fasi[0], ERTMatchPhase::Blast);
	TestEqual(TEXT("poi Move"), Fasi[1], ERTMatchPhase::Move);

	// A fine sequenza non c'e' piu' niente da disegnare, e non e' un errore: non c'e' una fase corrente.
	int32 Passi = 0;
	while (VM.StepPhaseForward() && Passi < 16) { ++Passi; }
	TestEqual(TEXT("finita"), VM.Position().State, ERTReplayPositionState::Ended);
	TestEqual(TEXT("niente da disegnare"), VM.CurrentPhaseEntries().Num(), 0);
	TestEqual(TEXT("e nessuna fase nel turno corrente"), VM.PhasesInCurrentTurn().Num(), 0);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.EdgesAreDisabledNotSilent` — i quattro bordi.
 *
 * Un comando che ai bordi non fa nulla **senza dirlo** e' indistinguibile da uno rotto. Il test verifica
 * che `Can*` e `Step*` rispondano alla stessa domanda: un `Can` che abilitasse un pulsante inerte sarebbe
 * il difetto nella sua forma piu' insidiosa, perche' a schermo somiglia a un blocco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelEdgesTest,
	"RefactorTactics.Replay.ViewModel.EdgesAreDisabledNotSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelEdgesTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Bordi"));
	Pulisci(Root);
	const FGuid Id = ArchivioDueTurni(Root);

	FRTReplayViewModel VM;
	VM.Open(Root, Id);

	// Bordo 1: indietro da prima dell'inizio.
	TestFalse(TEXT("indietro da BeforeStart e' disabilitato"), VM.CanStepPhaseBackward());
	TestFalse(TEXT("e non fa nulla"), VM.StepPhaseBackward());
	TestEqual(TEXT("posizione invariata"), VM.Position().State, ERTReplayPositionState::BeforeStart);

	// Bordo 2: turno precedente dal primo turno.
	VM.StepPhaseForward(); // turno 1
	TestEqual(TEXT("al turno 1"), VM.Position().TurnNumber, 1);
	TestFalse(TEXT("turno precedente dal primo e' disabilitato"), VM.CanStepTurnBackward());
	TestFalse(TEXT("e non fa nulla"), VM.StepTurnBackward());
	TestEqual(TEXT("ancora turno 1"), VM.Position().TurnNumber, 1);

	// Bordo 3: turno successivo dall'ultimo.
	TestTrue(TEXT("turno successivo dal primo e' abilitato"), VM.CanStepTurnForward());
	TestTrue(TEXT("e muove"), VM.StepTurnForward());
	TestEqual(TEXT("al turno 2"), VM.Position().TurnNumber, 2);
	TestFalse(TEXT("turno successivo dall'ultimo e' disabilitato"), VM.CanStepTurnForward());
	TestFalse(TEXT("e non fa nulla"), VM.StepTurnForward());
	TestEqual(TEXT("ancora turno 2"), VM.Position().TurnNumber, 2);

	// Bordo 4: avanti dalla fine.
	int32 Passi = 0;
	while (VM.StepPhaseForward() && Passi < 16) { ++Passi; }
	TestEqual(TEXT("finita"), VM.Position().State, ERTReplayPositionState::Ended);
	TestFalse(TEXT("avanti da Ended e' disabilitato"), VM.CanStepPhaseForward());
	TestFalse(TEXT("e non fa nulla"), VM.StepPhaseForward());

	// I due estremi restano raggiungibili all'indietro: `Ended` non e' un vicolo cieco.
	TestTrue(TEXT("indietro da Ended e' abilitato"), VM.CanStepPhaseBackward());
	TestTrue(TEXT("e riporta all'ultima fase"), VM.StepPhaseBackward());
	TestEqual(TEXT("che e' Move del turno 2"), VM.Position().Phase, ERTMatchPhase::Move);
	TestEqual(TEXT("turno 2"), VM.Position().TurnNumber, 2);

	// E il confine di turno all'indietro porta all'ULTIMA fase del turno precedente, non alla sua prima:
	// e' il verso in cui si sta guardando.
	TestTrue(TEXT("indietro"), VM.StepPhaseBackward()); // Blast turno 2
	TestTrue(TEXT("indietro, attraversa il confine"), VM.StepPhaseBackward());
	TestEqual(TEXT("ultima fase del turno 1"), VM.Position().Phase, ERTMatchPhase::Move);
	TestEqual(TEXT("turno 1"), VM.Position().TurnNumber, 1);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.TurnStepFollowsTheTraceNotArithmetic` — «turno precedente» non e' `N-1`.
 *
 * Le tracce dichiarano il proprio `TurnNumber` e una sequenza puo' iniziare da un turno qualsiasi
 * (`RTReplaySeekLibrary`). Un archivio con numerazione non contigua e' quindi riproducibile, e un salto
 * aritmetico bloccherebbe i pulsanti su una partita perfettamente valida. Il test usa i turni **3** e
 * **7**: con `+1`/`-1` entrambi i comandi fallirebbero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelTurnStepTest,
	"RefactorTactics.Replay.ViewModel.TurnStepFollowsTheTraceNotArithmetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelTurnStepTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("TurniNonContigui"));
	Pulisci(Root);

	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(Traccia(3, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	Tracce.Add(Traccia(7, { ERTMatchPhase::Prep, ERTMatchPhase::Cleanup }));
	const FGuid Id = ScriviArchivio(Root, Tracce, true);

	FRTReplayViewModel VM;
	TestEqual(TEXT("apre"), VM.Open(Root, Id), ERTReplayOpenResult::Opened);

	TestTrue(TEXT("prima fase"), VM.StepPhaseForward());
	TestEqual(TEXT("il primo turno dichiarato e' 3"), VM.Position().TurnNumber, 3);

	TestTrue(TEXT("turno successivo"), VM.StepTurnForward());
	TestEqual(TEXT("che e' 7, non 4"), VM.Position().TurnNumber, 7);
	TestEqual(TEXT("e ci si arriva alla sua prima fase"), VM.Position().Phase, ERTMatchPhase::Prep);

	TestTrue(TEXT("turno precedente"), VM.StepTurnBackward());
	TestEqual(TEXT("torna a 3, non a 6"), VM.Position().TurnNumber, 3);
	TestEqual(TEXT("alla PRIMA fase del turno, non all'ultima"), VM.Position().Phase, ERTMatchPhase::Blast);

	// Il seek diretto resta quello di #415, e vale sui numeri dichiarati.
	TestEqual(TEXT("seek al 7"), VM.SeekToTurn(7), ERTReplaySeekResult::Found);
	TestEqual(TEXT("ci siamo"), VM.Position().TurnNumber, 7);
	TestEqual(TEXT("seek al 4, che non esiste"), VM.SeekToTurn(4), ERTReplaySeekResult::TurnNotFound);
	TestEqual(TEXT("fail-closed: non si e' mosso"), VM.Position().TurnNumber, 7);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.OpenFailuresAreDistinctAndLeaveNothingUsable` — i quattro esiti di apertura.
 *
 * `#472` chiede che la UI li distingua: mandano chi diagnostica in tre posti diversi. E su un rifiuto il
 * view model non deve restare **navigabile a meta'** — che sarebbe la cosa peggiore da consegnare a una
 * schermata, perche' sembra funzionare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelOpenFailureTest,
	"RefactorTactics.Replay.ViewModel.OpenFailuresAreDistinctAndLeaveNothingUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelOpenFailureTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Aperture"));
	Pulisci(Root);

	// Nessun manifest sotto quel `MatchId`.
	{
		FRTReplayViewModel VM;
		TestEqual(TEXT("manifest assente"), VM.Open(Root, FGuid::NewGuid()),
			ERTReplayOpenResult::ManifestUnreadable);
		TestFalse(TEXT("non e' aperto"), VM.IsOpen());
		TestEqual(TEXT("l'esito resta interrogabile"), VM.LastOpenResult(),
			ERTReplayOpenResult::ManifestUnreadable);

		// Niente e' navigabile: e' il punto del fail-closed.
		TestFalse(TEXT("niente avanti"), VM.CanStepPhaseForward());
		TestFalse(TEXT("niente indietro"), VM.CanStepPhaseBackward());
		TestFalse(TEXT("nessun turno avanti"), VM.CanStepTurnForward());
		TestFalse(TEXT("nessun turno indietro"), VM.CanStepTurnBackward());
		TestFalse(TEXT("e non si muove"), VM.StepPhaseForward());
		TestEqual(TEXT("resta BeforeStart"), VM.Position().State, ERTReplayPositionState::BeforeStart);

		// Nemmeno il play parte: un pulsante che si accende senza che nulla si muova e' lo stesso difetto
		// dei bordi, in un altro posto.
		VM.Play();
		TestFalse(TEXT("il play non parte su un archivio non aperto"), VM.IsPlaying());
	}

	// Una traccia dichiarata dal manifest ma mancante dal disco.
	{
		const FGuid Id = ArchivioDueTurni(Root);
		const FString Dir = URTReplayRecorderLibrary::MatchDirectory(Root, Id);
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		const FString Traccia2 = FPaths::Combine(Dir, URTReplayRecorderLibrary::TurnFileName(2));
		TestTrue(TEXT("la traccia esiste prima"), PF.FileExists(*Traccia2));
		PF.DeleteFile(*Traccia2);

		FRTReplayViewModel VM;
		TestEqual(TEXT("traccia mancante"), VM.Open(Root, Id), ERTReplayOpenResult::TraceUnreadable);
		TestFalse(TEXT("non e' aperto"), VM.IsOpen());
		TestNotEqual(TEXT("ed e' un esito DIVERSO da manifest assente"), VM.LastOpenResult(),
			ERTReplayOpenResult::ManifestUnreadable);

		// ⚠️ Il «non lascia niente di navigabile» va verificato **per ogni** rifiuto, non solo per il
		// primo: e' meta' del nome di questo test, e la prima stesura lo asseriva su un caso su due.
		TestFalse(TEXT("niente avanti"), VM.CanStepPhaseForward());
		TestFalse(TEXT("e non si muove"), VM.StepPhaseForward());
		VM.Play();
		TestFalse(TEXT("e il play non parte"), VM.IsPlaying());
	}

	// Un manifest che dichiara una topologia che questo Player non riproduce.
	//
	// 🔴 Mancava, e la code review l'ha misurato: il test si chiamava «i quattro esiti sono distinti»
	// mentre ne esercitava **due**. `TopologyMismatch` e' l'esito che `OpenArchive` puo' produrre da due
	// punti diversi, ed e' quello che manderebbe chi diagnostica nel posto piu' lontano dagli altri.
	{
		FRTReplayManifest M;
		M.MatchId = FGuid::NewGuid();
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = false; // un archivio che viene da un altro mondo: il substrato quadrato non esiste piu'
		URTReplayRecorderLibrary::RecordTurn(Root, M, 1,
			Traccia(1, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 42, 3.f);

		FRTReplayViewModel VM;
		TestEqual(TEXT("topologia incompatibile"), VM.Open(Root, M.MatchId),
			ERTReplayOpenResult::TopologyMismatch);
		TestFalse(TEXT("non e' aperto"), VM.IsOpen());
		TestNotEqual(TEXT("diverso da manifest illeggibile"), VM.LastOpenResult(),
			ERTReplayOpenResult::ManifestUnreadable);
		TestNotEqual(TEXT("e diverso da traccia illeggibile"), VM.LastOpenResult(),
			ERTReplayOpenResult::TraceUnreadable);
		TestFalse(TEXT("e non e' navigabile"), VM.CanStepPhaseForward());

		// `IsComplete()` non deve spacciare «nessun archivio» per «archivio parziale»: il manifest di
		// questa partita e' CHIUSO, quindi un `bComplete` letto senza guardare `bOpen` sarebbe pure
		// arrivato dal posto sbagliato.
		TestFalse(TEXT("e non si dichiara completo"), VM.IsComplete());
	}

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.UnopenedModelDoesNotAnswerDomainQuestions` — il silenzio prima dell'apertura.
 *
 * 🔴 Trovato in code review: `IsComplete()` leggeva `Session.bComplete` senza guardare `bOpen`, quindi un
 * view model **mai aperto** rispondeva `false` — che una schermata rende come «archivio parziale», cioe'
 * una partita guardabile ma troncata, al posto di «non c'e' nessuna partita». E `LastOpenResult()` parte
 * da `ManifestUnreadable`, quindi dichiarava illeggibile un manifest che nessuno aveva mai cercato.
 *
 * E' la famiglia di difetti che questo codebase evita altrove per scelta esplicita — il `-1.f` di
 * `PlanningSecondsRemaining`, il `0` di `TurnNumber` — e che qui era rientrata da tre porte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelUnopenedTest,
	"RefactorTactics.Replay.ViewModel.UnopenedModelDoesNotAnswerDomainQuestions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelUnopenedTest::RunTest(const FString&)
{
	FRTReplayViewModel VM;

	TestFalse(TEXT("non e' aperto"), VM.IsOpen());
	TestFalse(TEXT("e nessuno ha mai provato ad aprirlo"), VM.HasAttemptedOpen());
	TestFalse(TEXT("non si dichiara completo"), VM.IsComplete());
	TestEqual(TEXT("non ha voci da mostrare"), VM.CurrentPhaseEntries().Num(), 0);
	TestEqual(TEXT("ne' fasi nel turno corrente"), VM.PhasesInCurrentTurn().Num(), 0);
	TestEqual(TEXT("la posizione e' prima dell'inizio"), VM.Position().State,
		ERTReplayPositionState::BeforeStart);
	TestFalse(TEXT("che non ha fase"), VM.Position().HasPhase());

	// Dopo un tentativo fallito la domanda «com'e' andata» diventa lecita, e la risposta e' quella vera.
	const FString Root = TransientRoot(TEXT("MaiAperto"));
	Pulisci(Root);
	TestEqual(TEXT("apertura fallita"), VM.Open(Root, FGuid::NewGuid()),
		ERTReplayOpenResult::ManifestUnreadable);
	TestTrue(TEXT("ora il tentativo c'e' stato"), VM.HasAttemptedOpen());
	TestFalse(TEXT("ma l'archivio no"), VM.IsOpen());
	TestFalse(TEXT("e non si dichiara completo"), VM.IsComplete());

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.PartialArchiveIsWatchableAndSaysSo` — un archivio parziale non e' un archivio rotto.
 *
 * Si apre, si guarda fino a dove arriva, e **lo dichiara prima** invece di lasciarlo scoprire quando la
 * riproduzione si ferma senza spiegazioni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelPartialTest,
	"RefactorTactics.Replay.ViewModel.PartialArchiveIsWatchableAndSaysSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelPartialTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Parziale"));
	Pulisci(Root);
	const FGuid Id = ArchivioDueTurni(Root, /*bChiudi=*/false);

	FRTReplayViewModel VM;
	TestEqual(TEXT("un parziale si apre"), VM.Open(Root, Id), ERTReplayOpenResult::Opened);
	TestFalse(TEXT("e dichiara di non essere completo"), VM.IsComplete());

	// ⚠️ Si contano le posizioni CON una fase, non i passi riusciti: l'ultimo passo riesce e porta a
	// `Ended`, che una fase non ce l'ha. Contare i `true` darebbe cinque per quattro fasi — ed e'
	// esattamente l'errore che la prima stesura di questo test conteneva.
	int32 Fasi = 0;
	int32 Passi = 0;
	while (VM.StepPhaseForward() && Passi < 16)
	{
		++Passi;
		if (VM.Position().HasPhase()) { ++Fasi; }
	}
	TestEqual(TEXT("si guarda fino in fondo: quattro fasi"), Fasi, 4);
	TestEqual(TEXT("in cinque passi, l'ultimo dei quali esce dalla sequenza"), Passi, 5);
	TestEqual(TEXT("e finisce"), VM.Position().State, ERTReplayPositionState::Ended);

	Pulisci(Root);
	return true;
}

/**
 * `Replay.ViewModel.PlayAdvancesOnePhasePerBeatAndStopsAtEnd` — lo stato di riproduzione, che nessuna
 * libreria portava.
 *
 * ⚠️ Il test pinna anche la regola del **delta grande**: un frame lungo avanza di UNA fase, non di quante
 * ne entrerebbero. Consumare l'arretrato farebbe dipendere dal frame rate quante fasi il giocatore vede,
 * ed e' il tipo di dipendenza dal tempo reale che il progetto tiene fuori dalle decisioni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelPlaybackTest,
	"RefactorTactics.Replay.ViewModel.PlayAdvancesOnePhasePerBeatAndStopsAtEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelPlaybackTest::RunTest(const FString&)
{
	const FString Root = TransientRoot(TEXT("Riproduzione"));
	Pulisci(Root);
	const FGuid Id = ArchivioDueTurni(Root);

	FRTReplayViewModel VM;
	VM.Open(Root, Id);
	VM.SecondsPerPhase = 1.f;

	// In pausa il tempo non muove niente: e' la pausa, non un rallentamento.
	TestFalse(TEXT("non parte da solo"), VM.IsPlaying());
	TestFalse(TEXT("in pausa il tick non avanza"), VM.Tick(10.f));
	TestEqual(TEXT("fermo prima dell'inizio"), VM.Position().State, ERTReplayPositionState::BeforeStart);

	VM.Play();
	TestTrue(TEXT("in riproduzione"), VM.IsPlaying());

	// 🔴 **Il primo passo e' immediato**, e la prima stesura di questo test asseriva il contrario —
	// «ancora fermo» dopo `Play`. Era il comportamento vero, ed era un difetto: `BeforeStart` non ha una
	// fase, quindi premere Play lasciava una schermata vuota per un `SecondsPerPhase` intero. Trovato in
	// code review.
	TestEqual(TEXT("Play mostra subito la prima fase"), VM.Position().Phase, ERTMatchPhase::Blast);
	TestEqual(TEXT("turno 1"), VM.Position().TurnNumber, 1);
	TestEqual(TEXT("e ha qualcosa da disegnare"), VM.CurrentPhaseEntries().Num(), 1);

	// Sotto la soglia non succede niente: la fase resta a schermo il tempo che le spetta.
	TestFalse(TEXT("mezzo battito non avanza"), VM.Tick(0.5f));
	TestEqual(TEXT("ancora sulla prima fase"), VM.Position().Phase, ERTMatchPhase::Blast);

	// Un delta enorme: **una** fase, non tre.
	TestTrue(TEXT("delta grande"), VM.Tick(100.f));
	TestEqual(TEXT("una sola fase avanti"), VM.Position().Phase, ERTMatchPhase::Move);
	TestEqual(TEXT("stesso turno"), VM.Position().TurnNumber, 1);

	// Fino in fondo: alla fine si mette in pausa da solo.
	int32 Battiti = 0;
	while (VM.IsPlaying() && Battiti < 16) { VM.Tick(2.f); ++Battiti; }
	TestEqual(TEXT("finita"), VM.Position().State, ERTReplayPositionState::Ended);
	TestFalse(TEXT("e la riproduzione si e' fermata da sola"), VM.IsPlaying());

	// `Play` su una sequenza finita non riparte: non c'e' dove andare.
	VM.Play();
	TestFalse(TEXT("play su Ended non parte"), VM.IsPlaying());

	// 🔴 **L'auto-pausa vale anche arrivando alla fine COI PULSANTI**, non solo col tick, e la prima
	// stesura la teneva solo in `Tick`: si poteva restare «in riproduzione» su un replay concluso per un
	// `SecondsPerPhase` intero, con il pulsante di pausa acceso su qualcosa che non riproduceva.
	// Trovato in code review.
	VM.Rewind();
	VM.Play();
	TestTrue(TEXT("riparte"), VM.IsPlaying());
	int32 Clic = 0;
	while (VM.StepPhaseForward() && Clic < 16) { ++Clic; }
	TestEqual(TEXT("si arriva alla fine a colpi di pulsante"), VM.Position().State,
		ERTReplayPositionState::Ended);
	TestFalse(TEXT("e la riproduzione si e' fermata lo stesso"), VM.IsPlaying());

	// Ma `Rewind` la rende di nuovo riproducibile, senza rileggere il disco.
	VM.Rewind();
	TestEqual(TEXT("torna prima dell'inizio"), VM.Position().State, ERTReplayPositionState::BeforeStart);
	VM.Play();
	TestTrue(TEXT("e riparte"), VM.IsPlaying());

	Pulisci(Root);
	return true;
}


/**
 * **`OpenFromTraces` e' la STESSA navigazione, non una seconda** (`#1625`).
 *
 * 🔑 Lo Scenario Playback riproduce l'ultima esecuzione di uno scenario, che vive in memoria e non e'
 * mai stata scritta su disco. Senza questa porta l'unico modo di riusare la navigazione sarebbe
 * riscriverla nell'editor — due implementazioni di «fase successiva» che possono divergere.
 *
 * ⛔ **Il test che conta e' l'EQUIVALENZA.** Le stesse tracce, aperte per le due vie, devono percorrersi
 * identiche: stessi bordi, stesse fasi, stesse posizioni a ogni passo. Un test che si limitasse a
 * verificare che la nuova porta «funziona» passerebbe anche su una navigazione leggermente diversa — ed
 * e' precisamente il difetto che avere due porte introduce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewModelFromTracesTest,
	"RefactorTactics.Replay.ViewModel.OpensFromTracesInMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewModelFromTracesTest::RunTest(const FString&)
{
	// --- Il rifiuto, per primo ------------------------------------------------------------------------
	{
		FRTReplayViewModel Vuoto;
		TestFalse(TEXT("tracce vuote: rifiuta"), Vuoto.OpenFromTraces({}));
		TestFalse(TEXT("e non risulta aperto"), Vuoto.IsOpen());

		// ⚠️ Un playback aperto su zero turni mostrerebbe un campo fermo — indistinguibile da una partita
		// in cui non e' successo niente, che e' un'affermazione diversa.
		TestFalse(TEXT("e non si puo' nemmeno avanzare"), Vuoto.CanStepPhaseForward());
	}

	// --- Le stesse tracce, per le due vie -------------------------------------------------------------
	const TArray<TArray<FRTTurnLogEntry>> Tracce = {
		Traccia(1, { ERTMatchPhase::Blast, ERTMatchPhase::Move }),
		Traccia(2, { ERTMatchPhase::Prep, ERTMatchPhase::Blast, ERTMatchPhase::Cleanup }),
	};

	const FString Root = TransientRoot(TEXT("DaMemoria"));
	Pulisci(Root);
	const FGuid Id = ScriviArchivio(Root, Tracce, /*bChiudi=*/ true);

	FRTReplayViewModel DaDisco;
	if (!TestEqual(TEXT("l'archivio si apre"), DaDisco.Open(Root, Id), ERTReplayOpenResult::Opened))
	{
		Pulisci(Root);
		return false;
	}

	FRTReplayViewModel DaMemoria;
	if (!TestTrue(TEXT("e le stesse tracce si aprono in memoria"), DaMemoria.OpenFromTraces(Tracce)))
	{
		Pulisci(Root);
		return false;
	}

	// --- Si percorrono identiche ----------------------------------------------------------------------
	// ⚠️ Si confrontano anche i `CanStep*`: sono cio' che accende e spegne i pulsanti, e due navigazioni
	// che finiscono nello stesso posto per strade diverse si distinguono proprio sui bordi.
	int32 Passi = 0;
	while (DaDisco.CanStepPhaseForward())
	{
		TestTrue(TEXT("il bordo e' lo stesso"), DaMemoria.CanStepPhaseForward());

		DaDisco.StepPhaseForward();
		DaMemoria.StepPhaseForward();

		TestEqual(TEXT("stesso turno"), DaMemoria.Position().TurnNumber, DaDisco.Position().TurnNumber);
		TestEqual(TEXT("stessa fase"), DaMemoria.Position().Phase, DaDisco.Position().Phase);
		TestEqual(TEXT("stesso stato"), DaMemoria.Position().State, DaDisco.Position().State);

		if (++Passi > 32) { break; } // guardia: una navigazione che non termina e' un difetto, non un ciclo
	}

	// ⛔ ANTI-VACUITA': se il ciclo non avesse fatto nemmeno un passo, tutte le uguaglianze sopra sarebbero
	// vere per assenza.
	//
	// ⚠️ **Sono SEI passi per cinque fasi**, e il numero va spiegato invece che tollerato: le tracce
	// dichiarano due fasi nel primo turno e tre nel secondo, ma l'ultimo passo non porta a una sesta fase —
	// porta a `Ended`, che e' uno stato del replay e non un istante della partita. La prima stesura si
	// aspettava cinque e cadeva qui: era il test a non conoscere la fine, non la navigazione a sbagliarla.
	TestEqual(TEXT("cinque fasi, piu' l'arrivo a fine partita"), Passi, 6);
	TestEqual(TEXT("e l'ultimo passo e' la FINE, non una sesta fase"),
		DaMemoria.Position().State, ERTReplayPositionState::Ended);
	TestEqual(TEXT("che le due vie raggiungono insieme"),
		DaMemoria.Position().State, DaDisco.Position().State);
	TestFalse(TEXT("ed entrambe sono in fondo"), DaMemoria.CanStepPhaseForward());

	// Le fasi del turno corrente vengono dalla traccia, e sono le stesse.
	TestEqual(TEXT("stesse fasi nel turno corrente"),
		DaMemoria.PhasesInCurrentTurn().Num(), DaDisco.PhasesInCurrentTurn().Num());

	Pulisci(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
