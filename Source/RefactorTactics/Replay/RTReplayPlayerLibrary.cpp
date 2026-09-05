#include "Replay/RTReplayPlayerLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Misc/Paths.h"

ERTReplayOpenResult URTReplayPlayerLibrary::OpenArchive(const FString& ReplaysRoot, const FGuid& MatchId,
	FRTReplaySession& OutSession, int32 ObserverTeamId)
{
	// Si lavora su una sessione TEMPORANEA e la si adotta solo a lettura riuscita: su un rifiuto a meta'
	// apertura, chi chiama deve ritrovare la sessione che aveva — non una mezza partita caricata.
	FRTReplaySession Aperta;

	if (!URTReplayRecorderLibrary::LoadManifest(ReplaysRoot, MatchId, Aperta.Manifest))
	{
		// Copre l'assenza del file e la versione sconosciuta: `ManifestFromJson` e' gia' fail-closed, e
		// rifiutare qui e' precisamente cio' che ADR-0009 §4 chiede al Player in apertura.
		return ERTReplayOpenResult::ManifestUnreadable;
	}

	if (!Aperta.Manifest.bHexTopology)
	{
		// Le celle di una traccia quadrata non significano la stessa cosa: riprodurle su un substrato
		// esagonale disegnerebbe una partita che non e' mai avvenuta. Il substrato quadrato non esiste piu'
		// nel progetto, quindi un archivio che lo dichiara viene da un altro mondo.
		return ERTReplayOpenResult::TopologyMismatch;
	}

	const FString Dir = URTReplayRecorderLibrary::MatchDirectory(ReplaysRoot, MatchId);

	// Quale PRODOTTO si apre ([D-316], `#2098`): la traccia per osservatore se l'archivio ne porta una per
	// questa squadra, altrimenti la canonica.
	//
	// 🔴 **Si guarda il MANIFEST e non il disco**, per la stessa ragione per cui il conteggio dei turni viene
	// di li': un file che c'e' senza essere dichiarato non e' parte dell'archivio, e uno dichiarato che manca
	// e' un archivio rotto — non un caso da compensare in silenzio ricadendo sulla canonica, che mostrerebbe
	// a una squadra i fatti dell'altra.
	//
	// ⚠️ **Una squadra non elencata NON e' un errore**: e' uno spettatore che chiede una vista che questo
	// archivio non ha — ogni `v1`, e ogni partita registrata prima di [D-316]. Riceve la canonica, cioe' la
	// stessa cosa che riceveva prima, e la differenza e' osservabile da `Manifest.ObserverTeamIds` invece di
	// essere taciuta.
	const bool bVistaFiltrata = ObserverTeamId >= 0
		&& Aperta.Manifest.ObserverTeamIds.Contains(ObserverTeamId);

	// Il manifest DICHIARA quanti turni ci sono: si caricano quelli, non quelli che si trovano in cartella.
	// Fidarsi dell'elenco dei file renderebbe una traccia lasciata li' per sbaglio parte della partita.
	Aperta.Traces.Reserve(Aperta.Manifest.TurnCount);
	for (int32 Turno = 1; Turno <= Aperta.Manifest.TurnCount; ++Turno)
	{
		const FString Percorso = FPaths::Combine(Dir, bVistaFiltrata
			? URTReplayRecorderLibrary::TurnFileNameForObserver(Turno, ObserverTeamId)
			: URTReplayRecorderLibrary::TurnFileName(Turno));

		TArray<FRTTurnLogEntry> Traccia;
		ERTLogTopology Topologia = ERTLogTopology::Square;
		if (!URTTurnLogLibrary::LoadTurnLogFromFile(Percorso, Traccia, &Topologia))
		{
			// `LoadTurnLogFromFile` e' gia' fail-closed su magic, versione, troncamento e checksum: qui non si
			// aggiunge una seconda validazione, si riporta la sua.
			return ERTReplayOpenResult::TraceUnreadable;
		}
		if (Topologia != ERTLogTopology::Hex)
		{
			// Una traccia quadrata dentro un archivio che si dichiara hex: il manifest e il payload non
			// raccontano la stessa partita, e riprodurla sarebbe interpretare byte arbitrari.
			return ERTReplayOpenResult::TopologyMismatch;
		}
		Aperta.Traces.Add(MoveTemp(Traccia));
	}

	Aperta.Cursor = FRTReplayCursor();
	Aperta.bComplete = Aperta.Manifest.bClosed;

	OutSession = MoveTemp(Aperta);
	return ERTReplayOpenResult::Opened;
}

bool URTReplayPlayerLibrary::AdvancePhase(FRTReplaySession& Session, ERTMatchPhase& OutPhase,
	TArray<FRTTurnLogEntry>& OutEntries)
{
	// Salta le tracce gia' esaurite (e quelle vuote: un turno puo' non aver prodotto voci).
	while (Session.Traces.IsValidIndex(Session.Cursor.TraceIndex)
		&& Session.Cursor.EntryIndex >= Session.Traces[Session.Cursor.TraceIndex].Num())
	{
		++Session.Cursor.TraceIndex;
		Session.Cursor.EntryIndex = 0;
	}

	if (!Session.Traces.IsValidIndex(Session.Cursor.TraceIndex))
	{
		return false; // sequenza finita: il cursore resta dov'e'
	}

	const TArray<FRTTurnLogEntry>& Traccia = Session.Traces[Session.Cursor.TraceIndex];
	const ERTMatchPhase Fase = Traccia[Session.Cursor.EntryIndex].Phase;

	// Le voci di una fase sono CONTIGUE perche' la traccia e' in forma canonica e `Phase` e' la prima chiave
	// di `EntryLess`: non serve filtrare tutta la traccia, basta avanzare finche' la fase non cambia. E' la
	// stessa proprieta' su cui poggia `SeekToPhase`, e va detta perche' e' l'unica cosa che rende questo
	// codice corretto senza ricalcolare nulla.
	TArray<FRTTurnLogEntry> DellaFase;
	int32 i = Session.Cursor.EntryIndex;
	for (; i < Traccia.Num() && Traccia[i].Phase == Fase; ++i)
	{
		DellaFase.Add(Traccia[i]);
	}

	Session.Cursor.EntryIndex = i;
	OutPhase = Fase;
	OutEntries = MoveTemp(DellaFase);
	return true;
}

void URTReplayPlayerLibrary::Rewind(FRTReplaySession& Session)
{
	Session.Cursor = FRTReplayCursor();
}

ERTReplaySeekResult URTReplayPlayerLibrary::SeekToTurn(FRTReplaySession& Session, int32 TurnNumber)
{
	// Il seek di `#415`, non un secondo meccanismo: su un `TurnNotFound` lascia il cursore dov'era, che e'
	// gia' il comportamento giusto qui — una riproduzione non salta a caso perche' il bersaglio non c'e'.
	return URTReplaySeekLibrary::SeekToTurn(Session.Traces, TurnNumber, Session.Cursor);
}
