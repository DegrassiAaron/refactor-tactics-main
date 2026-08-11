#include "Misc/AutomationTest.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Match History (`#416`): la lista delle partite non apre gli archivi.
 *
 * Il criterio della issue e' misurabile e non dichiarativo, e qui e' verificato nel modo piu' severo
 * disponibile: si CANCELLANO gli archivi e la lista deve leggersi lo stesso. Un'implementazione che
 * scorresse le cartelle o aprisse i manifest fallirebbe — un conteggio di letture, invece, si puo' sempre
 * discutere.
 */
namespace
{
	FString HistoryRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("MatchHistory"), Nome);
	}

	void PuliscIStoria(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	FRTTurnLogEntry VoceStoria(int32 Turno, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = ERTMatchPhase::Move;
		E.Category = ERTLogCategory::Move;
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, Turno);
		E.TgtCell = FRTCellId(Amount + 1, Turno);
		return E;
	}

	/** Scrive un archivio vero e restituisce il manifest chiuso: la lista deve poterne fare a meno. */
	FRTReplayManifest ScriviArchivio(const FString& Root, const FGuid& MatchId, int32 Turni, bool bChiudi)
	{
		FRTReplayManifest M;
		M.MatchId = MatchId;
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;

		for (int32 T = 1; T <= Turni; ++T)
		{
			TArray<FRTTurnLogEntry> Voci;
			Voci.Add(VoceStoria(T, T * 10));
			URTTurnLogLibrary::SortTurnLog(Voci);
			URTReplayRecorderLibrary::RecordTurn(Root, M, T, Voci);
		}
		if (bChiudi)
		{
			URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 4242, 90.5f);
		}
		return M;
	}
}

/**
 * Aprire la lista non legge nessun payload di archivio.
 *
 * Si registrano due partite, si cancellano **tutti** gli archivi, e la lista resta leggibile e completa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryNoPayloadTest,
	"RefactorTactics.Replay.History.ListDoesNotOpenArchives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryNoPayloadTest::RunTest(const FString&)
{
	const FString Root = HistoryRoot(TEXT("NoPayload"));
	PuliscIStoria(Root);

	const FGuid IdA(0xAAAAAAAA, 1, 2, 3);
	const FGuid IdB(0xBBBBBBBB, 4, 5, 6);
	const FDateTime Quando(2026, 8, 10, 12, 0, 0);

	const FRTReplayManifest A = ScriviArchivio(Root, IdA, /*Turni=*/ 3, /*bChiudi=*/ true);
	const FRTReplayManifest B = ScriviArchivio(Root, IdB, /*Turni=*/ 1, /*bChiudi=*/ false);

	TestTrue(TEXT("la prima partita entra nell'indice"),
		URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(A, Quando)));
	TestTrue(TEXT("la seconda partita entra nell'indice"),
		URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(B, Quando)));

	// ⚠️ IL PUNTO DEL TEST: gli archivi spariscono. Quello che resta e' solo `history.rtindex`.
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.DeleteDirectoryRecursively(*URTReplayRecorderLibrary::MatchDirectory(Root, IdA));
	PF.DeleteDirectoryRecursively(*URTReplayRecorderLibrary::MatchDirectory(Root, IdB));

	TArray<FRTMatchHistoryEntry> Lista;
	if (!TestTrue(TEXT("la lista si legge senza archivi"), URTMatchHistoryLibrary::LoadIndex(Root, Lista)))
	{
		PuliscIStoria(Root);
		return false;
	}
	TestEqual(TEXT("due partite in lista"), Lista.Num(), 2);

	const FRTMatchHistoryEntry* RigaA = Lista.FindByPredicate(
		[&IdA](const FRTMatchHistoryEntry& E) { return E.MatchId == IdA; });
	const FRTMatchHistoryEntry* RigaB = Lista.FindByPredicate(
		[&IdB](const FRTMatchHistoryEntry& E) { return E.MatchId == IdB; });

	if (!TestNotNull(TEXT("la prima partita e' in lista"), RigaA)
		|| !TestNotNull(TEXT("la seconda partita e' in lista"), RigaB))
	{
		PuliscIStoria(Root);
		return false;
	}

	// I metadati arrivano dall'indice, non dagli archivi che non esistono piu'.
	TestEqual(TEXT("modo"), RigaA->FormatId, FName(TEXT("Format.Skirmish2v2")));
	TestEqual(TEXT("turni"), RigaA->TurnCount, 3);
	TestTrue(TEXT("esito registrato"), RigaA->Outcome == ERTMatchOutcome::Team0Wins);
	TestEqual(TEXT("durata"), RigaA->WallClockSeconds, 90.5f);
	TestEqual(TEXT("data d'inizio"), RigaA->StartedUtc, Quando);

	// DISPONIBILITA' DEL REPLAY: la partita chiusa e' completa, quella interrotta no. Non e' «assente»:
	// e' parziale, e la lista lo dice invece di lasciarlo scoprire a chi clicca.
	TestTrue(TEXT("la partita chiusa ha il replay completo"), RigaA->bReplayComplete);
	TestFalse(TEXT("la partita interrotta ha il replay parziale"), RigaB->bReplayComplete);
	TestTrue(TEXT("l'esito della partita interrotta resta 'in corso'"),
		RigaB->Outcome == ERTMatchOutcome::InProgress);

	PuliscIStoria(Root);
	return true;
}

/**
 * La stessa partita occupa UNA riga, non due.
 *
 * Una partita entra nella lista quando comincia — cosi' compare anche se il gioco muore — e la stessa riga si
 * completa alla fine. Accodare darebbe due righe, e la seconda non renderebbe falsa la prima: la lista
 * mostrerebbe due partite dove ce n'e' una.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryUpdatesRowTest,
	"RefactorTactics.Replay.History.SameMatchKeepsOneRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryUpdatesRowTest::RunTest(const FString&)
{
	const FString Root = HistoryRoot(TEXT("OneRow"));
	PuliscIStoria(Root);

	const FGuid Id(0xC0FFEE00, 7, 8, 9);
	const FDateTime Quando(2026, 8, 10, 9, 30, 0);

	// 1. La partita comincia: riga aperta, nessun esito, replay non completo.
	FRTReplayManifest M;
	M.MatchId = Id;
	M.FormatId = FName(TEXT("Format.Skirmish2v2"));
	URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(M, Quando));

	TArray<FRTMatchHistoryEntry> Aperta;
	URTMatchHistoryLibrary::LoadIndex(Root, Aperta);
	TestEqual(TEXT("una riga all'inizio"), Aperta.Num(), 1);
	TestFalse(TEXT("il replay non e' ancora completo"), Aperta[0].bReplayComplete);

	// 2. La partita finisce: la STESSA riga si completa.
	M.TurnCount = 5;
	M.OrderedHashPerTurn.SetNum(5);
	M.Outcome = ERTMatchOutcome::Team1Wins;
	M.WallClockSeconds = 120.f;
	M.bClosed = true;
	URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(M, Quando));

	TArray<FRTMatchHistoryEntry> Chiusa;
	URTMatchHistoryLibrary::LoadIndex(Root, Chiusa);
	TestEqual(TEXT("ancora una riga sola"), Chiusa.Num(), 1);
	TestTrue(TEXT("l'esito e' arrivato"), Chiusa[0].Outcome == ERTMatchOutcome::Team1Wins);
	TestEqual(TEXT("i turni sono arrivati"), Chiusa[0].TurnCount, 5);
	TestTrue(TEXT("il replay e' completo"), Chiusa[0].bReplayComplete);

	// 3. Un indice inesistente non e' una lista vuota: e' un'assenza, e si dice.
	TArray<FRTMatchHistoryEntry> Altrove;
	Altrove.Add(FRTMatchHistoryEntry());
	TestFalse(TEXT("una radice senza indice non si legge"),
		URTMatchHistoryLibrary::LoadIndex(HistoryRoot(TEXT("MaiScritto")), Altrove));
	TestEqual(TEXT("e l'uscita non e' stata toccata"), Altrove.Num(), 1);

	PuliscIStoria(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
