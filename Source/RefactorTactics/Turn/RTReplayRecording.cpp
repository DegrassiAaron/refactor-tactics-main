#include "Turn/RTReplayRecording.h"

#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayAuditLibrary.h"
#include "Replay/RTMatchHistoryLibrary.h"

FString FRTReplayRecording::ResolveRoot(const FString& Override)
{
	return Override.IsEmpty()
		? URTReplayRecorderLibrary::DefaultReplaysRoot()
		: Override;
}

void FRTReplayRecording::Begin(FName FormatId, TArray<int32> ObserverTeamIds, int32 LocalObserverTeamId,
	const FString& Root)
{
	Manifest = FRTReplayManifest();
	Manifest.MatchId = FGuid::NewGuid();
	Manifest.FormatId = FormatId;
	Manifest.bHexTopology = true; // un solo substrato: `FRTCellId` e' esagonale (ADR-0002)

	// ⚠️ Gli osservatori arrivano **gia' raccolti e ordinati** da chi interroga il mondo: qui non si sa
	// cosa sia un `ARTUnit`, ed e' la proprieta' che rende questa struct esercitabile senza un mondo.
	Manifest.ObserverTeamIds = MoveTemp(ObserverTeamIds);
	Manifest.LocalObserverTeamId = LocalObserverTeamId;

	StartRealSeconds = FPlatformTime::Seconds();
	StartedUtc = FDateTime::UtcNow();

	URTMatchHistoryLibrary::AppendOrUpdate(Root,
		URTMatchHistoryLibrary::EntryFromManifest(Manifest, StartedUtc));
}

bool FRTReplayRecording::RecordTurn(const FString& Root, int32 TurnNumber,
	const TArray<FRTTurnLogEntry>& TurnLog)
{
	// ⛔ Nessun log qui: si risponde `false` e chi chiama decide cosa dirne. Vedi il contratto nell'header.
	return URTReplayRecorderLibrary::RecordTurn(Root, Manifest, TurnNumber, TurnLog);
}

bool FRTReplayRecording::RecordAudit(const FString& Root, const FRTTurnAudit& Audit) const
{
	return URTReplayAuditLibrary::RecordTurnAudit(Root, Audit);
}

uint32 FRTReplayRecording::LastOrderedHash() const
{
	return Manifest.OrderedHashPerTurn.Num() > 0
		? Manifest.OrderedHashPerTurn.Last()
		: 0;
}

bool FRTReplayRecording::Close(const FString& Root, ERTMatchOutcome Outcome, uint32 FinalStateHash)
{
	// La durata a orologio si misura da `Begin`, ed e' l'unico punto in cui il tempo reale entra: e'
	// telemetria dell'archivio, non un ingresso di nessuna regola.
	const float WallClock = static_cast<float>(FPlatformTime::Seconds() - StartRealSeconds);

	return URTReplayRecorderLibrary::CloseMatch(Root, Manifest, Outcome, FinalStateHash, WallClock);
}
