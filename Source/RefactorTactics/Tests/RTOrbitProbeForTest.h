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
 * ⚠️ **La chiave e' del chiamante, e deve essere DISTINTA.** `NobodyOscillatesOnTheAuthoredMap` usa
 * `GetUniqueID()`, `EngagesOnTheGeneratedTestArena` usa `StableUnitId` — che vale 0 finche'
 * `EnsureMatchRoster()` non lo assegna, e li' il test ha la propria guardia.
 *
 * 🔴 **E la guardia serve contro un falso POSITIVO, non contro un falso negativo.** Questa nota diceva che
 * con una chiave condivisa «l'oscillante non farebbe crescere nessun contatore»: misurato, e' il contrario.
 * Due unita' FERME su celle diverse che scrivono la stessa chiave producono `A B A B ...` — un'oscillazione
 * perfetta in cui nessuno si e' mosso. Pinnato da `Meta.OrbitProbeKeepsUnitsApart`. Questa sonda non puo'
 * verificare la distinzione per conto del chiamante, e se non ce l'ha inventa un difetto.
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
	 * Sotto questa durata l'oracolo non puo' cadere, e un verde non direbbe niente.
	 *
	 * 🔴 **Il numero e' 4, e la prima stesura diceva 6 con un'aritmetica sbagliata.** Sosteneva che «il primo
	 * ritorno e' osservabile dal terzo turno, quindi con meno di sei turni la soglia non e' esercitabile»:
	 * il primo dato e' giusto, la conclusione no. Con `N` campioni il massimo dei ritorni osservabili e'
	 * `N - 2`, e il limite e' `max(1, N / 3)`:
	 *
	 *     N = 3   ->  max 1  contro limite 1   -> NON falsificabile
	 *     N = 4   ->  max 2  contro limite 1   -> falsificabile
	 *     N = 5   ->  max 3  contro limite 1   -> falsificabile
	 *
	 * ⚠️ **E il 6 non era prudenza gratuita, faceva danno.** Il chiamante che ASSERISCE la premessa —
	 * `NobodyOscillatesOnTheAuthoredMap` — va rosso quando la partita finisce prima, con un messaggio che
	 * dice «la partita non e' durata abbastanza». Cioe' un rosso per il bot che decide PRIMA, che e'
	 * esattamente il miglioramento che #1287/#1296 ed E47.1 esistono per produrre — lo stesso difetto che
	 * `RTMatchAutobattleTests.cpp` ha gia' documentato sulla propria soglia sorella. Pinnato da
	 * `Meta.OrbitProbeThresholdFollowsTheTurns`, che verifica anche la MINIMALITA': a 3 non e' esercitabile,
	 * a 4 si'. Verificato per mutazione: rimettendo 6, quel test cade.
	 *
	 * ⚠️ **La premessa e' del chiamante, e le due politiche legittime sono due.** Chi ha l'oscillazione come
	 * UNICA asserzione la assicura e si ferma — senza turni a sufficienza quel test non misura niente.
	 * Chi la affianca ad altre asserzioni (combattimento, primo colpo, parcheggio) avverte e prosegue,
	 * perche' fermarsi butterebbe via le misure che restano valide. Non e' una divergenza accidentale: e'
	 * la stessa domanda con due risposte giuste, e il chiamante deve sceglierne una in modo esplicito.
	 */
	static constexpr int32 MinTurnsToFalsify = 4;

private:
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, FRTCellId> Penultima;
	TMap<int32, int32> Ritorni;
};
