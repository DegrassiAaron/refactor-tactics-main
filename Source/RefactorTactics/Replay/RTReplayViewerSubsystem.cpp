#include "Replay/RTReplayViewerSubsystem.h"
#include "Misc/Paths.h"

FString URTReplayViewerSubsystem::GetReplaysRoot() const
{
	// `Saved/Replays` e' dove il recorder scrive (`#469`): il default non e' una convenzione di questo
	// file, e' la stessa cartella dall'altro capo della catena. Ricavarla da `FPaths` invece di ripeterla
	// come stringa evita che i due estremi divergano.
	return ReplaysRootOverride.IsEmpty()
		? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Replays"))
		: ReplaysRootOverride;
}

bool URTReplayViewerSubsystem::LoadMatchList(TArray<FRTMatchHistoryEntry>& OutMatches) const
{
	return URTMatchHistoryLibrary::LoadIndex(GetReplaysRoot(), OutMatches);
}

ERTReplayOpenResult URTReplayViewerSubsystem::OpenMatch(const FGuid& MatchId)
{
	return ViewModel.Open(GetReplaysRoot(), MatchId);
}

FText URTReplayViewerSubsystem::GetTurnLabel() const
{
	// ⚠️ Il trattino non e' un ripiego: e' la risposta giusta. Una traccia scritta prima del formato v6
	// dichiara `0`, che e' il sentinella «non dichiarato» e non il turno zero — stamparlo come numero
	// direbbe al giocatore una cosa falsa su una partita che per il resto si guarda benissimo.
	const FRTReplayPosition Posizione = ViewModel.Position();
	if (!Posizione.HasTurn())
	{
		return NSLOCTEXT("RefactorTactics", "ReplayTurnUnknown", "—");
	}
	return FText::AsNumber(Posizione.TurnNumber);
}
