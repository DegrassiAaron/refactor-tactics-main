#include "Misc/AutomationTest.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Tests/RTReplayTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il ponte Blueprint del replay (`#999`).
 *
 * ⚠️ **Questi test non ripetono quelli del view model, e la differenza e' il punto.** Posizione, bordi,
 * fasi osservabili e ritmo sono gia' provati in `RTReplayViewModelTests.cpp` senza mondo. Qui si prova
 * cio' che **solo** il ponte puo' sbagliare: che ogni inoltro arrivi al metodo giusto, che la radice
 * degli archivi sia quella del produttore, e le poche cose che questo tipo possiede davvero.
 *
 * 🔴 La prima stesura provava tre cose e lasciava scoperta **meta' dei forwarder** — fra cui `Tick`,
 * `Rewind`, i due `Step*Backward` e i due seek. La tesi «ogni funzione e' un inoltro» era **asserita, non
 * misurata**: uno scambio da copia-incolla (`StepTurnBackward` che chiama `StepPhaseBackward`) compila,
 * inoltra a un metodo vero e sarebbe passato. Trovato in code review.
 */
using namespace RTReplayFixtures;

/**
 * `Replay.ViewerSubsystem.BlueprintCanDriveTheWholeViewer` — il criterio della issue, in un test.
 *
 * *«Un widget Blueprint puo', senza C++: elencare le partite, aprirne una, leggere turno e fase correnti,
 * muoversi di fase e di turno, e sapere quali comandi sono abilitati.»* Il test percorre esattamente
 * quella frase, e ogni chiamata che fa e' una `UFUNCTION` — cioe' qualcosa che un graph node puo' fare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemDrivesTest,
	"RefactorTactics.Replay.ViewerSubsystem.BlueprintCanDriveTheWholeViewer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemDrivesTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("Guida"));
	Pulisci(R);
	const FGuid Id = ArchivioDueTurni(R, /*bChiudi=*/true, /*bIndicizza=*/true);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); Pulisci(R); return false; }

	V->SetReplaysRoot(R);

	// 1. elencare le partite
	TArray<FRTMatchHistoryEntry> Partite;
	TestTrue(TEXT("la lista si legge"), V->LoadMatchList(/*bNewestFirst=*/true, Partite));
	TestEqual(TEXT("una partita"), Partite.Num(), 1);
	if (Partite.Num() == 1)
	{
		TestEqual(TEXT("ed e' quella scritta"), Partite[0].MatchId, Id);
		TestTrue(TEXT("dichiarata completa"), Partite[0].bReplayComplete);
	}

	// 2. aprirne una
	TestEqual(TEXT("si apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
	TestTrue(TEXT("ed e' aperta"), V->IsOpen());
	TestTrue(TEXT("qualcuno ha provato ad aprire"), V->HasAttemptedOpen());
	TestEqual(TEXT("con esito Opened"), V->GetLastOpenResult(), ERTReplayOpenResult::Opened);
	TestTrue(TEXT("e completa"), V->IsArchiveComplete());
	TestEqual(TEXT("il manifest arriva fino a Blueprint"), V->GetManifest().FinalStateHash, (int64)4242);

	// 3. leggere turno e fase correnti
	TestEqual(TEXT("prima dell'inizio"), V->GetPosition().State, ERTReplayPositionState::BeforeStart);
	TestFalse(TEXT("senza fase"), V->PositionHasPhase());
	TestFalse(TEXT("senza turno"), V->PositionHasTurn());

	// 4. muoversi di fase
	TestTrue(TEXT("prima fase"), V->StepPhaseForward());
	TestTrue(TEXT("ora c'e' una fase"), V->PositionHasPhase());
	TestEqual(TEXT("Blast"), V->GetPosition().Phase, ERTMatchPhase::Blast);
	TestEqual(TEXT("turno 1"), V->GetPosition().TurnNumber, 1);
	TestEqual(TEXT("e c'e' una voce da disegnare"), V->GetCurrentPhaseEntries().Num(), 1);

	// 5. muoversi di turno
	TestTrue(TEXT("turno successivo"), V->StepTurnForward());
	TestEqual(TEXT("turno 2"), V->GetPosition().TurnNumber, 2);

	// 6. sapere quali comandi sono abilitati
	TestFalse(TEXT("dall'ultimo turno non si va avanti"), V->CanStepTurnForward());
	TestTrue(TEXT("ma indietro si"), V->CanStepTurnBackward());
	TestTrue(TEXT("e di fase si avanza ancora"), V->CanStepPhaseForward());
	TestTrue(TEXT("e si torna indietro"), V->CanStepPhaseBackward());

	// La barra dei salti legge le fasi presenti, non l'enum.
	const TArray<ERTMatchPhase> Fasi = V->GetPhasesInCurrentTurn();
	TestEqual(TEXT("due fasi nel turno"), Fasi.Num(), 2);
	if (Fasi.Num() == 2)
	{
		TestEqual(TEXT("Blast"), Fasi[0], ERTMatchPhase::Blast);
		TestEqual(TEXT("poi Move"), Fasi[1], ERTMatchPhase::Move);
	}
	TestEqual(TEXT("e le osservabili sono cinque"),
		URTReplayViewerSubsystem::GetObservablePhases().Num(), 5);

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * `Replay.ViewerSubsystem.EveryForwardReachesItsOwnMethod` — la tesi del ponte, **misurata**.
 *
 * Ogni inoltro viene chiamato e se ne osserva un effetto che **solo quel metodo** produce. E' il test che
 * cade se qualcuno scambia due forward per copia-incolla — l'unico modo in cui questo tipo puo' rompersi,
 * per sua stessa ammissione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemForwardsTest,
	"RefactorTactics.Replay.ViewerSubsystem.EveryForwardReachesItsOwnMethod",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemForwardsTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("Inoltri"));
	Pulisci(R);

	// Turni 3 e 7: la numerazione non contigua distingue «turno adiacente» da «N±1», e distingue i salti di
	// turno da quelli di fase, che su turni 1 e 2 potrebbero coincidere per caso.
	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(Traccia(3, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	Tracce.Add(Traccia(7, { ERTMatchPhase::Prep, ERTMatchPhase::Cleanup }));
	const FGuid Id = ScriviArchivio(R, Tracce, /*bChiudi=*/true);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); Pulisci(R); return false; }

	V->SetReplaysRoot(R);
	TestEqual(TEXT("apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);

	// StepPhaseForward → prima fase del primo turno
	TestTrue(TEXT("StepPhaseForward"), V->StepPhaseForward());
	TestEqual(TEXT("turno 3, Blast"), V->GetPosition().TurnNumber, 3);
	TestEqual(TEXT("Blast"), V->GetPosition().Phase, ERTMatchPhase::Blast);

	// StepTurnForward → turno 7 (adiacente), NON 4
	TestTrue(TEXT("StepTurnForward"), V->StepTurnForward());
	TestEqual(TEXT("turno 7, non 4"), V->GetPosition().TurnNumber, 7);
	TestEqual(TEXT("alla sua prima fase"), V->GetPosition().Phase, ERTMatchPhase::Prep);

	// StepTurnBackward → torna a 3. ⚠️ Se inoltrasse a `StepPhaseBackward` finirebbe su Cleanup del 3,
	// non sulla sua PRIMA fase: e' il punto in cui i due si distinguono.
	TestTrue(TEXT("StepTurnBackward"), V->StepTurnBackward());
	TestEqual(TEXT("turno 3"), V->GetPosition().TurnNumber, 3);
	TestEqual(TEXT("alla PRIMA fase del turno, non all'ultima"), V->GetPosition().Phase,
		ERTMatchPhase::Blast);

	// StepPhaseBackward → esce dalla sequenza dal davanti
	TestTrue(TEXT("StepPhaseBackward"), V->StepPhaseBackward());
	TestEqual(TEXT("prima dell'inizio"), V->GetPosition().State, ERTReplayPositionState::BeforeStart);

	// SeekToTurn → fail-closed su un turno che non esiste, poi trovato su uno che esiste
	TestEqual(TEXT("SeekToTurn su un turno assente"), V->SeekToTurn(4), ERTReplaySeekResult::TurnNotFound);
	TestEqual(TEXT("non si e' mosso"), V->GetPosition().State, ERTReplayPositionState::BeforeStart);
	TestEqual(TEXT("SeekToTurn su uno presente"), V->SeekToTurn(7), ERTReplaySeekResult::Found);
	TestEqual(TEXT("ci siamo"), V->GetPosition().TurnNumber, 7);

	// SeekToPhaseInCurrentTurn → dentro il turno corrente, e rifiuta le non osservabili
	TestEqual(TEXT("SeekToPhase su Cleanup"), V->SeekToPhaseInCurrentTurn(ERTMatchPhase::Cleanup),
		ERTReplaySeekResult::Found);
	TestEqual(TEXT("ci siamo"), V->GetPosition().Phase, ERTMatchPhase::Cleanup);
	TestEqual(TEXT("e Planning e' rifiutata"), V->SeekToPhaseInCurrentTurn(ERTMatchPhase::Planning),
		ERTReplaySeekResult::PhaseNotFound);
	TestEqual(TEXT("senza muoversi"), V->GetPosition().Phase, ERTMatchPhase::Cleanup);

	// Tick → avanza solo in riproduzione, e solo al battito
	V->SetSecondsPerPhase(1.f);
	TestEqual(TEXT("SetSecondsPerPhase/GetSecondsPerPhase"), V->GetSecondsPerPhase(), 1.f);
	TestFalse(TEXT("in pausa il tick non muove"), V->Tick(10.f));
	V->Rewind();
	TestEqual(TEXT("Rewind riporta prima dell'inizio"), V->GetPosition().State,
		ERTReplayPositionState::BeforeStart);
	TestTrue(TEXT("ma l'archivio resta aperto"), V->IsOpen());

	V->Play();
	TestTrue(TEXT("Play"), V->IsPlaying());
	TestEqual(TEXT("e mostra subito la prima fase"), V->GetPosition().Phase, ERTMatchPhase::Blast);
	TestFalse(TEXT("mezzo battito non avanza"), V->Tick(0.4f));
	TestTrue(TEXT("il battito completo si"), V->Tick(0.7f));
	TestEqual(TEXT("una fase avanti"), V->GetPosition().Phase, ERTMatchPhase::Move);
	V->Pause();
	TestFalse(TEXT("Pause"), V->IsPlaying());

	// Close → rilascia, e da qui in poi non c'e' piu' un archivio
	V->Close();
	TestFalse(TEXT("Close chiude"), V->IsOpen());
	TestFalse(TEXT("e non e' navigabile"), V->CanStepPhaseForward());
	TestEqual(TEXT("ne' ha voci"), V->GetCurrentPhaseEntries().Num(), 0);

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * `Replay.ViewerSubsystem.LabelsNeverPrintASentinel` — le due sole formattazioni del ponte.
 *
 * Stanno qui e non nel widget perche' sono **regole**, non stile: una traccia pre-v6 dichiara `0` e la
 * fase vale `Planning` fuori dalla sequenza. Entrambi sono sentinella, ed entrambi, stampati, direbbero al
 * giocatore una cosa falsa su una partita che per il resto si guarda benissimo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemLabelsTest,
	"RefactorTactics.Replay.ViewerSubsystem.LabelsNeverPrintASentinel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemLabelsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); return false; }

	const FString Trattino(TEXT("—"));

	// Senza archivio: nessun turno, nessuna fase.
	TestEqual(TEXT("turno senza archivio"), V->GetTurnLabel().ToString(), Trattino);
	TestEqual(TEXT("fase senza archivio"), V->GetPhaseLabel().ToString(), Trattino);

	// Un turno vero si stampa come numero, e una fase vera col suo nome.
	{
		const FString R = TransientRoot(TEXT("Etichette"));
		Pulisci(R);
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(Traccia(7, { ERTMatchPhase::Blast }));
		const FGuid Id = ScriviArchivio(R, Tracce, true);
		V->SetReplaysRoot(R);
		TestEqual(TEXT("apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);

		TestEqual(TEXT("prima di cominciare: trattino"), V->GetTurnLabel().ToString(), Trattino);
		TestEqual(TEXT("e nessuna fase"), V->GetPhaseLabel().ToString(), Trattino);

		TestTrue(TEXT("prima fase"), V->StepPhaseForward());
		TestEqual(TEXT("il turno dichiarato e' 7"), V->GetTurnLabel().ToString(), FString(TEXT("7")));
		TestEqual(TEXT("e la fase e' Blast"), V->GetPhaseLabel().ToString(), FString(TEXT("Blast")));

		// A fine sequenza tornano entrambe al trattino: non c'e' una posizione corrente.
		while (V->StepPhaseForward()) {}
		TestEqual(TEXT("finita"), V->GetPosition().State, ERTReplayPositionState::Ended);
		TestEqual(TEXT("turno a fine sequenza"), V->GetTurnLabel().ToString(), Trattino);
		TestEqual(TEXT("fase a fine sequenza"), V->GetPhaseLabel().ToString(), Trattino);
		Pulisci(R);
	}

	// 🔴 Un turno grande **non si raggruppa**: `FText::AsNumber` avrebbe reso `1234` come «1.234», cioe'
	// una quantita' invece di un identificatore — e chi lo ridigitasse in un campo di ricerca otterrebbe
	// `TurnNotFound`. Trovato in code review, e il test precedente pinnava `7`, che non ci arrivava.
	{
		const FString R = TransientRoot(TEXT("TurnoGrande"));
		Pulisci(R);
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(Traccia(1234, { ERTMatchPhase::Blast }));
		const FGuid Id = ScriviArchivio(R, Tracce, true);
		V->SetReplaysRoot(R);
		TestEqual(TEXT("apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
		TestTrue(TEXT("prima fase"), V->StepPhaseForward());
		TestEqual(TEXT("nessun separatore di migliaia"), V->GetTurnLabel().ToString(),
			FString(TEXT("1234")));
		Pulisci(R);
	}

	// Una traccia pre-v6, che dichiara `0`: si guarda, ma il turno non e' dichiarabile.
	{
		const FString R = TransientRoot(TEXT("PreV6"));
		Pulisci(R);
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(Traccia(0, { ERTMatchPhase::Blast }));
		const FGuid Id = ScriviArchivio(R, Tracce, true);
		V->SetReplaysRoot(R);
		TestEqual(TEXT("si apre lo stesso"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
		TestTrue(TEXT("e si guarda"), V->StepPhaseForward());

		TestTrue(TEXT("la fase c'e'"), V->PositionHasPhase());
		TestEqual(TEXT("ed e' Blast"), V->GetPhaseLabel().ToString(), FString(TEXT("Blast")));
		TestFalse(TEXT("il turno no"), V->PositionHasTurn());
		TestEqual(TEXT("e l'etichetta NON dice 0"), V->GetTurnLabel().ToString(), Trattino);
		Pulisci(R);
	}

	ReleaseSubsystemHost(GI);
	return true;
}

/**
 * `Replay.ViewerSubsystem.ReadsWhereTheRecorderWrites` — i due capi della catena, **davvero**.
 *
 * 🔴 La prima stesura di questo test era tautologica: ricopiava l'espressione dell'implementazione come
 * valore atteso, quindi non poteva cadere sulla divergenza che il suo nome promette. Trovato in code
 * review. Qui si **scrive** un archivio con la radice di default del recorder e lo si **rilegge** dal
 * subsystem senza override: se i due capi si separassero, la lista sarebbe vuota.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemRootTest,
	"RefactorTactics.Replay.ViewerSubsystem.ReadsWhereTheRecorderWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemRootTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); return false; }

	// Il default del lettore E' quello del produttore: una sola sorgente, non due espressioni uguali.
	TestEqual(TEXT("il default viene dal recorder"), V->GetReplaysRoot(),
		URTReplayRecorderLibrary::DefaultReplaysRoot());

	// E il giro completo: si scrive DOVE scrive il recorder, si legge SENZA override.
	const FString Predefinita = URTReplayRecorderLibrary::DefaultReplaysRoot();
	const FGuid Id = ScriviArchivio(Predefinita,
		{ Traccia(1, { ERTMatchPhase::Blast }) }, /*bChiudi=*/true, /*bIndicizza=*/true);

	TArray<FRTMatchHistoryEntry> Partite;
	TestTrue(TEXT("la lista si legge dalla radice di default"),
		V->LoadMatchList(/*bNewestFirst=*/true, Partite));
	TestTrue(TEXT("e contiene la partita appena scritta"),
		Partite.ContainsByPredicate([&Id](const FRTMatchHistoryEntry& E) { return E.MatchId == Id; }));
	TestEqual(TEXT("che si apre senza override"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);

	// ⚠️ Si ripulisce solo la propria partita: `Saved/Replays` e' la cartella vera, e potrebbe contenere
	// registrazioni di chi sta usando il progetto.
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	V->Close();
	PF.DeleteDirectoryRecursively(*URTReplayRecorderLibrary::MatchDirectory(Predefinita, Id));

	// L'override vince finche' non lo si toglie.
	const FString Altrove = TransientRoot(TEXT("Altrove"));
	V->SetReplaysRoot(Altrove);
	TestEqual(TEXT("l'override vince"), V->GetReplaysRoot(), Altrove);
	V->SetReplaysRoot(FString());
	TestEqual(TEXT("e svuotandolo si torna al default"), V->GetReplaysRoot(), Predefinita);

	ReleaseSubsystemHost(GI);
	return true;
}

/**
 * `Replay.ViewerSubsystem.ChangingRootClosesTheOpenArchive` — e il resto di cio' che il ponte possiede.
 *
 * Due difetti trovati in code review, entrambi invisibili finche' non li si nomina: cambiare la radice
 * lasciava aperto un archivio dell'**altra** cartella, e `SetSecondsPerPhase(0)` faceva scorrere l'intera
 * partita a una fase per `Tick`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemGuardsTest,
	"RefactorTactics.Replay.ViewerSubsystem.ChangingRootClosesTheOpenArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemGuardsTest::RunTest(const FString&)
{
	const FString R1 = TransientRoot(TEXT("Radice1"));
	const FString R2 = TransientRoot(TEXT("Radice2"));
	Pulisci(R1);
	Pulisci(R2);
	const FGuid Id = ArchivioDueTurni(R1, true, /*bIndicizza=*/true);
	ArchivioDueTurni(R2, true, /*bIndicizza=*/true);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V))
	{
		ReleaseSubsystemHost(GI); Pulisci(R1); Pulisci(R2); return false;
	}

	V->SetReplaysRoot(R1);
	TestEqual(TEXT("apre dalla prima radice"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
	TestTrue(TEXT("prima fase"), V->StepPhaseForward());
	TestTrue(TEXT("aperta"), V->IsOpen());

	// Cambiare radice chiude: altrimenti il subsystem descriverebbe una partita di R1 elencando R2.
	V->SetReplaysRoot(R2);
	TestFalse(TEXT("cambiare radice chiude l'archivio"), V->IsOpen());
	TestEqual(TEXT("e riporta prima dell'inizio"), V->GetPosition().State,
		ERTReplayPositionState::BeforeStart);
	TArray<FRTMatchHistoryEntry> Partite;
	TestTrue(TEXT("la lista ora e' dell'altra radice"), V->LoadMatchList(true, Partite));
	TestEqual(TEXT("una partita"), Partite.Num(), 1);
	if (Partite.Num() == 1)
	{
		TestNotEqual(TEXT("e non e' quella di prima"), Partite[0].MatchId, Id);
	}

	// Impostare la STESSA radice non chiude nulla: sarebbe un reset a sorpresa.
	V->SetReplaysRoot(R2);
	TestEqual(TEXT("radice invariata"), V->GetReplaysRoot(), R2);

	// 🔴 Il clamp del ritmo. Con `0` la guardia `SecondsPerPhase > 0.f` del view model e' falsa e ogni
	// `Tick` avanzerebbe di una fase, qualunque delta.
	V->SetSecondsPerPhase(0.f);
	TestTrue(TEXT("zero viene clampato a un valore positivo"), V->GetSecondsPerPhase() > 0.f);
	V->SetSecondsPerPhase(-5.f);
	TestTrue(TEXT("e cosi' un negativo"), V->GetSecondsPerPhase() > 0.f);
	V->SetSecondsPerPhase(FMath::Sqrt(-1.f)); // NaN: `> 0.f` e' falso anche per lui
	TestTrue(TEXT("e un NaN non passa"), FMath::IsFinite(V->GetSecondsPerPhase()));
	TestTrue(TEXT("restando positivo"), V->GetSecondsPerPhase() > 0.f);
	V->SetSecondsPerPhase(2.f);
	TestEqual(TEXT("un valore valido passa intatto"), V->GetSecondsPerPhase(), 2.f);

	ReleaseSubsystemHost(GI);
	Pulisci(R1);
	Pulisci(R2);
	return true;
}

/**
 * `Replay.ViewerSubsystem.NewestFirstIsSomethingBlueprintCannotDo` — l'ordinamento.
 *
 * Non e' comodita': l'indice e' permanentemente dal piu' vecchio (la riga si scrive quando la partita
 * **comincia**), e non esiste un nodo Blueprint che ordini un array di struct per `FDateTime`. Senza
 * questo, una schermata di cronologia dovrebbe scendere in C++ — cioe' violare il DoD di `#999`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemOrderTest,
	"RefactorTactics.Replay.ViewerSubsystem.NewestFirstIsSomethingBlueprintCannotDo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemOrderTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("Ordine"));
	Pulisci(R);

	// Scritte dalla piu' VECCHIA alla piu' recente, che e' l'ordine in cui l'indice le tiene.
	const FGuid Vecchia = ScriviArchivio(R, { Traccia(1, { ERTMatchPhase::Blast }) }, true, true,
		FDateTime(2026, 8, 10, 9, 0, 0));
	const FGuid Media = ScriviArchivio(R, { Traccia(1, { ERTMatchPhase::Blast }) }, true, true,
		FDateTime(2026, 8, 12, 9, 0, 0));
	const FGuid Recente = ScriviArchivio(R, { Traccia(1, { ERTMatchPhase::Blast }) }, true, true,
		FDateTime(2026, 8, 16, 9, 0, 0));

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); Pulisci(R); return false; }
	V->SetReplaysRoot(R);

	TArray<FRTMatchHistoryEntry> Cronologia;
	TestTrue(TEXT("si legge"), V->LoadMatchList(/*bNewestFirst=*/true, Cronologia));
	if (TestEqual(TEXT("tre partite"), Cronologia.Num(), 3))
	{
		TestEqual(TEXT("prima la piu' recente"), Cronologia[0].MatchId, Recente);
		TestEqual(TEXT("poi quella di mezzo"), Cronologia[1].MatchId, Media);
		TestEqual(TEXT("infine la piu' vecchia"), Cronologia[2].MatchId, Vecchia);
	}

	// Senza ordinamento resta l'ordine dell'indice, che e' quello di scrittura.
	TArray<FRTMatchHistoryEntry> Grezza;
	TestTrue(TEXT("si legge grezza"), V->LoadMatchList(/*bNewestFirst=*/false, Grezza));
	if (TestEqual(TEXT("tre partite"), Grezza.Num(), 3))
	{
		TestEqual(TEXT("nell'ordine dell'indice: la piu' vecchia per prima"), Grezza[0].MatchId, Vecchia);
	}

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
