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
	const bool bRecorded = URTReplayRecorderLibrary::RecordTurn(Root, Manifest, TurnNumber, TurnLog);

	// La riga della lista segue i turni mentre la partita gira (`#3871`). `Begin` la scrive con 0 turni e `Close` la
	// completa (`#3859`), ma una partita che alla chiusura non arriva — un processo terminato, una cattura che si ferma
	// a un numero di frame — restava per sempre «0 turni» mentre il manifest, riscritto a ogni turno, ne contava tre o
	// quattro. `AppendOrUpdate` aggiorna la riga in luogo e `EntryFromManifest` legge cio' che il manifest dice ADESSO:
	// esito ancora «in corso» e replay non completo, perche' i turni non fingono una chiusura.
	//
	// Nessun ramo sull'esito del turno: se non e' stato registrato il manifest non si e' mosso e la riga si riscrive
	// identica, e un ramo che nessun test puo' distinguere dal suo assente e' codice da non scrivere. L'esito
	// dell'indice non cambia quello del turno: se l'indice non si scrive, l'archivio e' comunque a posto.
	URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(Manifest, StartedUtc));
	return bRecorded;
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

	const bool bClosed = URTReplayRecorderLibrary::CloseMatch(Root, Manifest, Outcome, FinalStateHash, WallClock);

	// La riga della lista si completa con quello che il manifest dice ADESSO: esito, turni, durata e la disponibilita'
	// del replay, che e' la chiusura stessa. `AppendOrUpdate` aggiorna la riga scritta da `Begin`, non ne accoda una
	// seconda. ⚠️ Si fa **anche se la chiusura e' fallita**: `bClosed` e' rimasto `false` e l'indice lo riporta, cosi' la
	// lista non promette una partita intera che il disco non ha. (`#3859`: la chiamata c'era in `ARTTurnManager` e si e'
	// persa nell'estrazione di `0e636014f`, `#2286`; da allora ogni riga restava «in corso, 0 turni».)
	URTMatchHistoryLibrary::AppendOrUpdate(Root, URTMatchHistoryLibrary::EntryFromManifest(Manifest, StartedUtc));

	return bClosed;
}
