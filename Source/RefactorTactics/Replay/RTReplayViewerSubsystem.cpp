#include "Replay/RTReplayViewerSubsystem.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Misc/Paths.h"

FString URTReplayViewerSubsystem::GetReplaysRoot() const
{
	// 🔴 **Il default lo CHIEDE al produttore, non lo ricostruisce.** La prima stesura ripeteva qui il
	// ternario di `ARTTurnManager::ResolveReplaysRoot` — stesso letterale, stesso campo — motivandolo con
	// l'intenzione di «evitare che i due estremi divergano». Due owner indipendenti della stessa costante
	// **sono** la divergenza. Trovato in code review.
	return ReplaysRootOverride.IsEmpty()
		? URTReplayRecorderLibrary::DefaultReplaysRoot()
		: ReplaysRootOverride;
}

void URTReplayViewerSubsystem::SetReplaysRoot(const FString& Root)
{
	if (Root == ReplaysRootOverride)
	{
		return;
	}

	// ⚠️ **Cambiare la radice chiude l'archivio aperto**, e non e' zelo: senza, il subsystem
	// descriverebbe una partita di una cartella mentre ne elenca un'altra — `GetManifest()` e
	// `GetPosition()` dal vecchio percorso, `LoadMatchList()` dal nuovo. Una schermata che mostra la lista
	// accanto a «in riproduzione» direbbe due partite senza rapporto. Trovato in code review.
	ReplaysRootOverride = Root;
	Close();
}

void URTReplayViewerSubsystem::Close()
{
	// Rilascia le tracce. ⚠️ Serve perche' `FRTReplaySession::Traces` porta il TurnLog **di ogni turno**
	// registrato, e questo e' un `UGameInstanceSubsystem`: sopravvive a ogni caricamento di livello. Senza,
	// un replay da sessanta turni aperto una volta resta in memoria per tutta la vita del processo, anche
	// dopo che il giocatore e' tornato al menu e ha cominciato una partita. Trovato in code review —
	// `Rewind()` non lo fa di proposito («senza rileggere il disco»), quindi serviva l'altro verso.
	ViewModel = FRTReplayViewModel();
}

void URTReplayViewerSubsystem::Deinitialize()
{
	Close();
	Super::Deinitialize();
}

bool URTReplayViewerSubsystem::LoadMatchList(bool bNewestFirst, TArray<FRTMatchHistoryEntry>& OutMatches) const
{
	if (!URTMatchHistoryLibrary::LoadIndex(GetReplaysRoot(), OutMatches))
	{
		return false;
	}

	// ⚠️ **L'ordinamento sta qui perche' Blueprint non puo' farlo.** L'indice e' in ordine di apparizione,
	// e `AppendOrUpdate` scrive la riga quando la partita COMINCIA e la aggiorna in luogo: l'array e'
	// quindi permanentemente dal piu' vecchio. Non esiste un nodo Blueprint che ordini un
	// `TArray<FRTMatchHistoryEntry>` per `FDateTime`, quindi senza questo la schermata dovrebbe scendere in
	// C++ — che e' esattamente cio' che il DoD di `#999` esclude. Trovato in code review.
	if (bNewestFirst)
	{
		OutMatches.StableSort([](const FRTMatchHistoryEntry& A, const FRTMatchHistoryEntry& B)
		{
			return A.StartedUtc > B.StartedUtc;
		});
	}
	return true;
}

ERTReplayOpenResult URTReplayViewerSubsystem::OpenMatch(const FGuid& MatchId)
{
	return ViewModel.Open(GetReplaysRoot(), MatchId);
}

FText URTReplayViewerSubsystem::GetTurnLabel() const
{
	// ⚠️ Il trattino non e' un ripiego: e' la risposta giusta quando **non c'e' un turno corrente**. Copre
	// quattro stati diversi — nessun archivio, prima dell'inizio, sequenza finita, traccia pre-v6 — e la
	// prima stesura di questo commento ne descriveva uno solo, dicendo «quando il turno non e'
	// dichiarabile». Trovato in code review: a fine replay il contatore va a `—`, e su un archivio sano
	// somiglia a un difetto dei dati.
	//
	// ⚠️ **Distinguere i quattro casi e' della schermata, non di qui**: `GetPosition().State` li separa gia'
	// tutti e quattro, e sceglierne uno qui — «Fine» invece di `—` — sarebbe comporre il testo al posto di
	// chi disegna. Cio' che questa funzione garantisce e' l'unica cosa che **non** e' stile: che un turno
	// non dichiarato non si stampi mai come numero.
	const FRTReplayPosition Posizione = ViewModel.Position();
	if (!Posizione.HasTurn())
	{
		return NSLOCTEXT("RefactorTactics", "ReplayTurnUnknown", "—");
	}

	// 🔴 `FText::AsNumber` **raggruppa**: il turno 1234 diventerebbe «1.234», cioe' una quantita' invece di
	// un identificatore — e chi lo ridigitasse in un campo di ricerca otterrebbe `TurnNotFound`. Trovato in
	// code review. `URTTurnHeaderWidget::GetRoundCounterText` usa `Printf` per la stessa ragione.
	return FText::FromString(FString::Printf(TEXT("%d"), Posizione.TurnNumber));
}

FText URTReplayViewerSubsystem::GetPhaseLabel() const
{
	// 🔴 **Simmetrico a `GetTurnLabel`, e mancava.** `FRTReplayPosition::Phase` vale `Planning` negli stati
	// senza fase — scelto proprio perche' non e' mai osservabile — ma `Planning` resta un membro vero
	// dell'enum: uno `Switch on ERTMatchPhase` in UMG imboccherebbe un ramo `Planning` prima dell'inizio e
	// di nuovo dopo la fine, disegnando una fase che il replay dichiara impossibile. E' la stessa classe di
	// difetto del «Turno 0», lasciata scoperta sull'altro campo. Trovato in code review.
	const FRTReplayPosition Posizione = ViewModel.Position();
	if (!Posizione.HasPhase())
	{
		return NSLOCTEXT("RefactorTactics", "ReplayPhaseNone", "—");
	}
	return FText::FromString(StaticEnum<ERTMatchPhase>()->GetNameStringByValue((int64)Posizione.Phase));
}

void URTReplayViewerSubsystem::SetSecondsPerPhase(float Seconds)
{
	// ⚠️ **Il clamp esiste perche' questa e' una porta Blueprint.** `Tick` avanza quando
	// `SecondsPerPhase <= 0` — la guardia `> 0.f` e' falsa anche per `NaN` — quindi uno slider di velocita'
	// che tocchi lo zero, o un lerp che sottopassi, farebbe scorrere l'intera partita a una fase per
	// chiamata senza che nulla lo dica. Quella guardia nel view model e' un ripiego difensivo, non una
	// funzione di avanzamento rapido. Trovato in code review.
	ViewModel.SecondsPerPhase = FMath::IsFinite(Seconds)
		? FMath::Max(KINDA_SMALL_NUMBER, Seconds)
		: 1.5f;
}
