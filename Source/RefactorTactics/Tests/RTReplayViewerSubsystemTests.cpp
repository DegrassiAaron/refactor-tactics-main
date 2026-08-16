#include "Misc/AutomationTest.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il ponte Blueprint del replay (`#999`).
 *
 * ⚠️ **Questi test non ripetono quelli del view model, e la differenza e' il punto.** Posizione, bordi,
 * fasi osservabili e ritmo sono gia' provati in `RTReplayViewModelTests.cpp` senza mondo. Qui si prova
 * cio' che **solo** il ponte puo' sbagliare: che l'inoltro arrivi davvero al view model, che la radice
 * degli archivi sia quella dove il recorder scrive, e l'unica formattazione che questo tipo possiede.
 *
 * ⚠️ Serve un `UGameInstance` — non un mondo, non un viewport: e' la stessa costruzione dei test del
 * navigatore, e per la stessa ragione.
 */
namespace
{
	FString SubRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayViewerSub"), Nome);
	}

	void PulisciSub(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	FRTTurnLogEntry VoceSub(int32 Turno, ERTMatchPhase Fase, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.ActionId = FName(TEXT("Action.Move"));
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, 0);
		E.TgtCell = FRTCellId(Amount + 1, 0);
		return E;
	}

	TArray<FRTTurnLogEntry> TracciaSub(int32 TurnoDichiarato)
	{
		TArray<FRTTurnLogEntry> Voci;
		Voci.Add(VoceSub(TurnoDichiarato, ERTMatchPhase::Blast, 1));
		Voci.Add(VoceSub(TurnoDichiarato, ERTMatchPhase::Move, 2));
		URTTurnLogLibrary::SortTurnLog(Voci);
		return Voci;
	}

	/** Un archivio di `Turni` turni con l'indice aggiornato, come lo lascerebbe una partita vera. */
	FGuid ScriviArchivioSub(const FString& Root, int32 Turni, int32 PrimoTurno = 1)
	{
		FRTReplayManifest M;
		M.MatchId = FGuid::NewGuid();
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;

		for (int32 i = 0; i < Turni; ++i)
		{
			URTReplayRecorderLibrary::RecordTurn(Root, M, i + 1, TracciaSub(PrimoTurno + i));
		}
		URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 4242, 30.f);

		// L'indice e' un file separato: il recorder non lo scrive, lo scrive chi registra la partita.
		URTMatchHistoryLibrary::AppendOrUpdate(Root,
			URTMatchHistoryLibrary::EntryFromManifest(M, FDateTime(2026, 8, 16, 9, 0, 0)));
		return M.MatchId;
	}

	URTReplayViewerSubsystem* MakeViewer(UGameInstance*& OutGI)
	{
		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (!OutGI)
		{
			return nullptr;
		}
		OutGI->AddToRoot();
		OutGI->Init();
		return OutGI->GetSubsystem<URTReplayViewerSubsystem>();
	}

	void ReleaseViewer(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}
}

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
	const FString Root = SubRoot(TEXT("Guida"));
	PulisciSub(Root);
	const FGuid Id = ScriviArchivioSub(Root, 2);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeViewer(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseViewer(GI); PulisciSub(Root); return false; }

	V->SetReplaysRoot(Root);

	// 1. elencare le partite
	TArray<FRTMatchHistoryEntry> Partite;
	TestTrue(TEXT("la lista si legge"), V->LoadMatchList(Partite));
	TestEqual(TEXT("una partita"), Partite.Num(), 1);
	if (Partite.Num() == 1)
	{
		TestEqual(TEXT("ed e' quella scritta"), Partite[0].MatchId, Id);
		TestTrue(TEXT("dichiarata completa"), Partite[0].bReplayComplete);
	}

	// 2. aprirne una
	TestEqual(TEXT("si apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
	TestTrue(TEXT("ed e' aperta"), V->IsOpen());
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

	// La barra dei salti legge le fasi presenti, non l'enum.
	TestEqual(TEXT("due fasi nel turno"), V->GetPhasesInCurrentTurn().Num(), 2);
	TestEqual(TEXT("e le osservabili sono cinque"),
		URTReplayViewerSubsystem::GetObservablePhases().Num(), 5);

	// La riproduzione, dal ponte.
	V->SetSecondsPerPhase(1.f);
	V->Play();
	TestTrue(TEXT("riproduce"), V->IsPlaying());
	V->Pause();
	TestFalse(TEXT("e si mette in pausa"), V->IsPlaying());

	ReleaseViewer(GI);
	PulisciSub(Root);
	return true;
}

/**
 * `Replay.ViewerSubsystem.TurnLabelSaysDashWhenTheTurnIsNotDeclared` — l'unica formattazione del ponte.
 *
 * E' qui e non nel widget perche' e' una **regola**, non uno stile: una traccia pre-v6 dichiara `0`, che
 * e' un sentinella e non un turno, e stamparlo come numero direbbe una cosa falsa. Lasciarla al widget
 * significherebbe riscriverla in ogni schermata che mostra una posizione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemTurnLabelTest,
	"RefactorTactics.Replay.ViewerSubsystem.TurnLabelSaysDashWhenTheTurnIsNotDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemTurnLabelTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeViewer(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseViewer(GI); return false; }

	// Prima di aprire qualsiasi cosa: nessun turno, quindi il trattino.
	TestEqual(TEXT("senza archivio: trattino"), V->GetTurnLabel().ToString(), FString(TEXT("—")));

	// Un turno vero si stampa come numero.
	{
		const FString Root = SubRoot(TEXT("Etichetta"));
		PulisciSub(Root);
		const FGuid Id = ScriviArchivioSub(Root, 1, /*PrimoTurno=*/7);
		V->SetReplaysRoot(Root);
		TestEqual(TEXT("apre"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);

		TestEqual(TEXT("prima di cominciare: trattino"), V->GetTurnLabel().ToString(), FString(TEXT("—")));
		TestTrue(TEXT("prima fase"), V->StepPhaseForward());
		TestEqual(TEXT("il turno dichiarato e' 7"), V->GetTurnLabel().ToString(), FString(TEXT("7")));
		PulisciSub(Root);
	}

	// Una traccia pre-v6, che dichiara `0`: si guarda, ma il turno non e' dichiarabile.
	{
		const FString Root = SubRoot(TEXT("EtichettaPreV6"));
		PulisciSub(Root);
		const FGuid Id = ScriviArchivioSub(Root, 1, /*PrimoTurno=*/0);
		V->SetReplaysRoot(Root);
		TestEqual(TEXT("si apre lo stesso"), V->OpenMatch(Id), ERTReplayOpenResult::Opened);
		TestTrue(TEXT("e si guarda"), V->StepPhaseForward());

		TestEqual(TEXT("c'e' una fase"), V->GetPosition().Phase, ERTMatchPhase::Blast);
		TestTrue(TEXT("e Blueprint lo sa"), V->PositionHasPhase());
		TestFalse(TEXT("ma il turno no"), V->PositionHasTurn());
		TestEqual(TEXT("e l'etichetta NON dice 0"), V->GetTurnLabel().ToString(), FString(TEXT("—")));
		PulisciSub(Root);
	}

	ReleaseViewer(GI);
	return true;
}

/**
 * `Replay.ViewerSubsystem.DefaultRootIsWhereTheRecorderWrites` — i due capi della catena.
 *
 * ⚠️ Il default non e' una convenzione di comodo: e' la **stessa** cartella in cui il recorder scrive
 * (`#469`). Se i due estremi divergessero, la lista sarebbe vuota su una macchina che ha registrato
 * partite — e sarebbe indistinguibile da «non hai ancora giocato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerSubsystemRootTest,
	"RefactorTactics.Replay.ViewerSubsystem.DefaultRootIsWhereTheRecorderWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTReplayViewerSubsystemRootTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeViewer(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseViewer(GI); return false; }

	const FString Atteso = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Replays"));
	TestEqual(TEXT("il default e' Saved/Replays"), V->GetReplaysRoot(), Atteso);

	// L'override serve ai test e a un'eventuale cartella d'importazione, e vale finche' non lo si toglie.
	const FString Altrove = SubRoot(TEXT("Altrove"));
	V->SetReplaysRoot(Altrove);
	TestEqual(TEXT("l'override vince"), V->GetReplaysRoot(), Altrove);
	V->SetReplaysRoot(FString());
	TestEqual(TEXT("e svuotandolo si torna al default"), V->GetReplaysRoot(), Atteso);

	ReleaseViewer(GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
