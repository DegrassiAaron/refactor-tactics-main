#include "Misc/AutomationTest.h"
#include "Replay/RTReplayViewModel.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il view model del replay (`#472`) — la parte automatizzabile dei criteri di R6.
 *
 * ⚠️ **Guarda gli `#include`**: il view model, il recorder, il TurnLog. Nessun `ARTTurnManager`, nessun
 * `URTHexSimLibrary`, nessun resolver, nessun `UWorld`, **nessun `UUserWidget`**. E' la forma negativa che
 * `#472` chiede al percorso della UI, la stessa che
 * [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3 chiede al Player: se la
 * posizione o i comandi avessero bisogno del resolver o di un widget, questi test non potrebbero esistere
 * in questa forma. Un test si aggira con un `#include`; una dipendenza che non esiste no.
 */
namespace
{
	FString VMRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayViewModel"), Nome);
	}

	void PulisciVM(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	FRTTurnLogEntry VoceVM(int32 TurnoDichiarato, ERTMatchPhase Fase, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = TurnoDichiarato;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.ActionId = FName(TEXT("Action.Move"));
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, 0);
		E.TgtCell = FRTCellId(Amount + 1, 0);
		return E;
	}

	/**
	 * Una traccia che dichiara `TurnoDichiarato` e contiene una voce per ciascuna fase richiesta.
	 *
	 * ⚠️ Il turno **dichiarato dalle voci** e' un parametro separato dal numero di file, e non e' un
	 * artificio: `RTReplaySeekLibrary` documenta che una sequenza puo' iniziare da un turno qualsiasi, e
	 * `OpenArchive` carica i file per posizione. Sono due cose diverse, e i test che le confondono
	 * verificherebbero un archivio che il codice non promette.
	 */
	TArray<FRTTurnLogEntry> TracciaVM(int32 TurnoDichiarato, const TArray<ERTMatchPhase>& Fasi)
	{
		TArray<FRTTurnLogEntry> Voci;
		int32 N = 0;
		for (const ERTMatchPhase F : Fasi)
		{
			Voci.Add(VoceVM(TurnoDichiarato, F, ++N));
		}
		URTTurnLogLibrary::SortTurnLog(Voci); // forma canonica: precondizione del seek e del Player
		return Voci;
	}

	/** Scrive un archivio dalle tracce date, nell'ordine. `bChiudi = false` lo lascia parziale. */
	FGuid ScriviArchivioVM(const FString& Root, const TArray<TArray<FRTTurnLogEntry>>& Tracce, bool bChiudi)
	{
		FRTReplayManifest M;
		M.MatchId = FGuid::NewGuid();
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;

		for (int32 i = 0; i < Tracce.Num(); ++i)
		{
			URTReplayRecorderLibrary::RecordTurn(Root, M, i + 1, Tracce[i]);
		}
		if (bChiudi)
		{
			URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 999, 12.5f);
		}
		return M.MatchId;
	}

	/** Due turni pieni, con due fasi ciascuno. E' l'archivio di base della maggior parte dei test. */
	FGuid ArchivioDueTurni(const FString& Root, bool bChiudi = true)
	{
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(TracciaVM(1, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		Tracce.Add(TracciaVM(2, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		return ScriviArchivioVM(Root, Tracce, bChiudi);
	}
}

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
	const FString Root = VMRoot(TEXT("Posizione"));
	PulisciVM(Root);
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

	PulisciVM(Root);
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
		const FString Root = VMRoot(TEXT("QuattroStati"));
		PulisciVM(Root);
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

		PulisciVM(Root);
	}

	{
		// 4. Unaddressable — le voci dichiarano `0`, come una traccia scritta prima del formato v6.
		const FString Root = VMRoot(TEXT("NonIndirizzabile"));
		PulisciVM(Root);
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(TracciaVM(0, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		const FGuid Id = ScriviArchivioVM(Root, Tracce, true);

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

		PulisciVM(Root);
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

	const FString Root = VMRoot(TEXT("FasiOsservabili"));
	PulisciVM(Root);
	const FGuid Id = ArchivioDueTurni(Root);

	FRTReplayViewModel VM;
	VM.Open(Root, Id);
	VM.StepPhaseForward();

	// Le due non osservabili rispondono `PhaseNotFound` anche a un seek esplicito — e' l'esito corretto,
	// non un difetto da tappare: il turno c'e', quella fase non ha prodotto voci.
	TestEqual(TEXT("seek a Planning"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Planning),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("seek a MatchEnded"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::MatchEnded),
		ERTReplaySeekResult::PhaseNotFound);
	// Una fase osservabile ma assente da QUESTA traccia risponde allo stesso modo, e va bene cosi'.
	TestEqual(TEXT("seek a Dash, assente dalla traccia"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Dash),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("nessuno dei tre ha mosso la posizione"), VM.Position().Phase, ERTMatchPhase::Blast);

	TestEqual(TEXT("seek a Move riesce"), VM.SeekToPhaseInCurrentTurn(ERTMatchPhase::Move),
		ERTReplaySeekResult::Found);
	TestEqual(TEXT("e sposta"), VM.Position().Phase, ERTMatchPhase::Move);

	PulisciVM(Root);
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
	const FString Root = VMRoot(TEXT("Bordi"));
	PulisciVM(Root);
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

	PulisciVM(Root);
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
	const FString Root = VMRoot(TEXT("TurniNonContigui"));
	PulisciVM(Root);

	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(TracciaVM(3, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	Tracce.Add(TracciaVM(7, { ERTMatchPhase::Prep, ERTMatchPhase::Cleanup }));
	const FGuid Id = ScriviArchivioVM(Root, Tracce, true);

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

	PulisciVM(Root);
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
	const FString Root = VMRoot(TEXT("Aperture"));
	PulisciVM(Root);

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
	}

	PulisciVM(Root);
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
	const FString Root = VMRoot(TEXT("Parziale"));
	PulisciVM(Root);
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

	PulisciVM(Root);
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
	const FString Root = VMRoot(TEXT("Riproduzione"));
	PulisciVM(Root);
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

	// Sotto la soglia non succede niente: la fase resta a schermo il tempo che le spetta.
	TestFalse(TEXT("mezzo battito non avanza"), VM.Tick(0.5f));
	TestEqual(TEXT("ancora fermo"), VM.Position().State, ERTReplayPositionState::BeforeStart);

	TestTrue(TEXT("il battito completo avanza"), VM.Tick(0.6f));
	TestEqual(TEXT("prima fase"), VM.Position().Phase, ERTMatchPhase::Blast);
	TestEqual(TEXT("turno 1"), VM.Position().TurnNumber, 1);

	// Un delta enorme: **una** fase, non quattro.
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

	// Ma `Rewind` la rende di nuovo riproducibile, senza rileggere il disco.
	VM.Rewind();
	TestEqual(TEXT("torna prima dell'inizio"), VM.Position().State, ERTReplayPositionState::BeforeStart);
	VM.Play();
	TestTrue(TEXT("e riparte"), VM.IsPlaying());

	PulisciVM(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
