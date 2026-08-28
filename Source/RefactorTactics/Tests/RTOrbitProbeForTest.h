#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

/**
 * Il RITORNO DI PERIODO DUE, per unita': `Cell[t] == Cell[t-2]` con `Cell[t] != Cell[t-1]`.
 *
 * E' la firma dello stato assorbente che i contatori di immobilita' NON vedono. Un'unita' che alterna
 * `A -> B -> A -> B` cambia cella a ogni turno, quindi ogni «turni consecutivi fermo» resta a zero e
 * l'oracolo passa verde su una partita che non decide nulla. Misurato su `L_HexArena` il 2026-08-23:
 * Riktor alterna fra `(1,-1,L0)` e la piattaforma `(3,-3,L1)` otto volte in dodici turni in partita, e
 * trentasette in quaranta nello scenario `AutoBattle.ArenaV01`.
 *
 * ⚠️ **E' la forma MINIMA che distingue un'alternanza da un percorso che ripassa.** Contare le celle
 * ripetute punirebbe un bot che aggira un ostacolo, cioe' esattamente il comportamento che #1287 e' andato
 * a comprare. Per questo si tengono solo le ultime due celle e non una storia intera: una storia inviterebbe
 * a cercarci dentro cicli piu' lunghi, che sono un'altra domanda e un'altra soglia.
 *
 * 🔴 **LIMITE DICHIARATO: vede il periodo DUE, non il tre.** Un'orbita `A -> B -> C -> A` lascia questo
 * contatore a zero. Non e' un'omissione da riparare di lato: il periodo tre chiede una storia per unita' e
 * una soglia propria, e nessun difetto misurato l'ha ancora prodotto. Chi ne misura uno estende qui, con la
 * propria verifica di mutazione.
 *
 * ⚠️ **La chiave e' del chiamante.** `NobodyOscillatesOnTheAuthoredMap` usa `GetUniqueID()`,
 * `EngagesOnTheGeneratedTestArena` usa `StableUnitId` — che vale 0 finche' `EnsureMatchRoster()` non lo
 * assegna, e li' il test ha la propria guardia sulla distinzione delle chiavi. Due unita' che ne
 * condividessero una si sovrascriverebbero, e l'oscillante non farebbe crescere nessun contatore: chi passa
 * una chiave deve sapere che e' distinta, perche' questa sonda non puo' verificarlo per lui.
 *
 * ⚠️ **Si campiona a FINE TURNO, una volta per turno.** Chiamarla a turno mezzo risolto misura posizioni che
 * non sono mai state uno stato di fine turno — e' il difetto trovato in code review su #1296, dove il tetto
 * di tick si esauriva in silenzio.
 */
struct FRTOrbitProbe
{
	/** Registra la cella di fine turno di una unita'. Da chiamare una volta per unita' viva, per turno. */
	void Observe(int32 Key, const FRTCellId& Cell)
	{
		const FRTCellId* Prev = Ultima.Find(Key);
		const FRTCellId* Prev2 = Penultima.Find(Key);
		if (Prev2 && Prev && *Prev2 == Cell && *Prev != Cell)
		{
			Ritorni.FindOrAdd(Key) += 1;
		}
		if (Prev) { Penultima.FindOrAdd(Key) = *Prev; }
		Ultima.FindOrAdd(Key) = Cell;
	}

	/** Quanti ritorni di periodo due ha accumulato l'unita' che ne ha di piu'. */
	int32 WorstReturns() const
	{
		int32 Peggiore = 0;
		for (const TPair<int32, int32>& P : Ritorni) { Peggiore = FMath::Max(Peggiore, P.Value); }
		return Peggiore;
	}

	/**
	 * La soglia si deriva dai turni GIOCATI, non da un tetto costante.
	 *
	 * 🔴 Con `MaxTurni / 3` il limite restava 4 anche quando la partita finiva prima, e un ritorno di
	 * periodo due e' osservabile solo dal TERZO turno: una partita decisa al quinto ne poteva produrre al
	 * massimo tre e passava **per aritmetica**. Poiche' lo scopo dei fix di #1287/#1296 e' proprio far
	 * decidere prima la partita, quella forma avrebbe reso vacuo il proprio test. Trovato in code review.
	 */
	static int32 LimitForTurns(int32 TurnsPlayed)
	{
		return FMath::Max(1, TurnsPlayed / 3);
	}

	/**
	 * Sotto questa durata l'oracolo non puo' cadere, e un verde non direbbe niente: il primo ritorno e'
	 * osservabile dal terzo turno, quindi con meno di sei turni la soglia non e' esercitabile. La premessa
	 * va ASSERITA dal chiamante, non sperata.
	 */
	static constexpr int32 MinTurnsToFalsify = 6;

private:
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, FRTCellId> Penultima;
	TMap<int32, int32> Ritorni;
};
