#include "Replay/RTReplayStateLibrary.h"

#include "Turn/RTTurnLogLibrary.h"

namespace
{
	/**
	 * L'indice dell'unita' in `Out`, creandola se la traccia la nomina per la prima volta.
	 *
	 * ⚠️ **Si aggiunge invece di scartare**, ed e' la scelta che il commento dell'header dichiara: un
	 * `UnitId` che compare solo nella traccia sarebbe altrimenti invisibile **senza che nulla lo dica**.
	 * Nasce sulla cella che la voce nomina, che e' l'unica informazione disponibile su di lei.
	 */
	int32 IndiceDi(TArray<FRTTracedUnitState>& Out, int32 UnitId, const FRTCellId& CellaSeNuova)
	{
		for (int32 i = 0; i < Out.Num(); ++i)
		{
			if (Out[i].UnitId == UnitId) { return i; }
		}

		FRTTracedUnitState Nuova;
		Nuova.UnitId = UnitId;
		Nuova.Cell = CellaSeNuova;
		return Out.Add(Nuova);
	}

	/**
	 * `true` se la voce e' a `(Turno, Fase, MicroStep)` o prima, nell'ordine in cui la partita le ha
	 * attraversate.
	 *
	 * `MicroStep == INDEX_NONE` significa **la fase intera**, che e' il comportamento storico e resta il
	 * default: in quel caso la terza coordinata non filtra nulla e ogni voce della fase entra.
	 */
	bool NonOltre(const FRTTurnLogEntry& E, int32 TurnNumber, ERTMatchPhase Phase, int32 MicroStep)
	{
		if (E.TurnNumber != TurnNumber)
		{
			return E.TurnNumber < TurnNumber;
		}
		// ⚠️ **`ERTMatchPhase` e' CRONOLOGICO**, ed e' una proprieta' dichiarata del tipo — non un'ipotesi
		// di questo file: `RTReplayPlayerLibrary` si appoggia alla stessa cosa per dire «la prossima fase».
		if (static_cast<uint8>(E.Phase) != static_cast<uint8>(Phase))
		{
			// Le fasi PRECEDENTI entrano per intero, micro-step compreso: sono finite, e una fase conclusa
			// non ha barriere ancora da attraversare. Il taglio fine vale solo dentro la fase richiesta.
			return static_cast<uint8>(E.Phase) < static_cast<uint8>(Phase);
		}

		// Da qui in giu' si e' DENTRO la fase richiesta.
		if (MicroStep == INDEX_NONE)
		{
			return true; // fase intera: nessun taglio fine
		}

		// 🔴 **Le voci `INDEX_NONE` stanno DOPO ogni boundary, e qui restano fuori** (`#2272`).
		//
		// Sono le voci che dichiarano di non appartenere a un ciclo di micro-step, e la piu' importante e'
		// `Action.Move`: `BuildMoveLog` gira dopo `FinishHexMovement`, quindi l'arrivo di un'unita' e'
		// posteriore a **ogni** barriera che ha attraversato per arrivarci.
		//
		// ⛔ Il numero direbbe il contrario — `-1 < 0` — e seguirlo applicherebbe l'arrivo prima delle
		// barriere: uno stato che la partita non ha mai attraversato, che e' esattamente cio' che l'header
		// di `UnitsAtPosition` vieta a proposito del riordino. Il segno qui non e' un ordine, e' una
		// categoria.
		if (E.MicroStepIndex == INDEX_NONE)
		{
			return false;
		}

		return E.MicroStepIndex <= MicroStep;
	}
}

bool URTReplayStateLibrary::EntryChangesUnitState(const FRTTurnLogEntry& Entry)
{
	switch (Entry.Category)
	{
	case ERTLogCategory::Move:
		// ⚠️ **Anche gli esiti BLOCCATI contano.** `BlockedByUnit` e i suoi fratelli descrivono una
		// mobilita' **parziale** — il commento dell'enum dice «fermata (o parziale)» — quindi `TgtCell` e'
		// dove l'unita' e' davvero arrivata, non dove voleva andare. Filtrare sul solo `Moved` lascerebbe
		// le unita' bloccate disegnate dove non sono.
		return true;

	case ERTLogCategory::Facing:
		return true;

	case ERTLogCategory::Combat:
		// Solo il colpo che abbatte cambia lo STATO ricostruito: il danno non letale e' un evento, e si
		// disegna leggendo la voce — non serve ricostruirlo.
		return Entry.Outcome == static_cast<uint8>(ERTCombatOutcome::Lethal);

	default:
		// Le nove famiglie dichiarate non rese: sono eventi, non stato. Vedi il commento dell'header.
		return false;
	}
}

TArray<FRTTracedUnitState> URTReplayStateLibrary::UnitsAtPosition(const TArray<FRTTurnLogEntry>& Entries,
	const TArray<FRTTracedUnitState>& Initial, int32 TurnNumber, ERTMatchPhase Phase)
{
	// La fase intera: `INDEX_NONE` come terza coordinata dice «non tagliare fine». Delegare invece di
	// duplicare il ciclo e' cio' che tiene le due letture d'accordo per costruzione.
	return UnitsAtBoundary(Entries, Initial, TurnNumber, Phase, INDEX_NONE);
}

TArray<FRTTracedUnitState> URTReplayStateLibrary::UnitsAtBoundary(const TArray<FRTTurnLogEntry>& Entries,
	const TArray<FRTTracedUnitState>& Initial, int32 TurnNumber, ERTMatchPhase Phase, int32 MicroStepIndex)
{
	TArray<FRTTracedUnitState> Out = Initial;

	for (const FRTTurnLogEntry& E : Entries)
	{
		if (!NonOltre(E, TurnNumber, Phase, MicroStepIndex) || !EntryChangesUnitState(E))
		{
			continue;
		}

		switch (E.Category)
		{
		case ERTLogCategory::Move:
		{
			// `UnitId` in una voce `Move` e' chi si e' mosso, e `TgtCell` dove e' arrivato.
			const int32 i = IndiceDi(Out, E.UnitId, E.TgtCell);
			Out[i].Cell = E.TgtCell;
			break;
		}

		case ERTLogCategory::Facing:
		{
			// 🔴 **La direzione viaggia in `Amount`**, e lo dichiara il campo stesso: *«danno effettivo
			// (Combat), numero di celle percorse (Move) o direzione come `ERTHexDirection` (Facing)»*. Un
			// consumatore che leggesse `Outcome` prenderebbe la CAUSA del riorientamento — `DerivedFromMove`,
			// `DeclaredInPlanning` — invece della direzione.
			//
			// ⚠️ Si valida l'intervallo: `Amount` e' un `int32` e l'enum ha sei valori. Una voce malformata
			// non deve produrre un facing fuori scala che poi qualcuno disegna.
			if (E.Amount >= 0 && E.Amount <= static_cast<int32>(ERTHexDirection::SE))
			{
				const int32 i = IndiceDi(Out, E.UnitId, E.SrcCell);
				Out[i].Facing = static_cast<ERTHexDirection>(E.Amount);
			}
			break;
		}

		case ERTLogCategory::Combat:
		{
			// 🔴 **Chi cade NON e' sempre `UnitId`**, ed e' la trappola di questa categoria: nelle voci
			// `Combat` `UnitId` porta **chi ha agito**, tranne negli hazard dove non c'e' un agente e porta
			// chi subisce. La tassonomia vive in `URTTurnLogLibrary::IsSubjectTheSufferer` — si chiede a
			// lei invece di ricordarsi la regola, che e' esattamente perche' quella funzione esiste.
			const bool bSoggettoSubisce = URTTurnLogLibrary::IsSubjectTheSufferer(E);
			const int32 Caduto = bSoggettoSubisce ? E.UnitId : E.SelectedTargetUnitId;

			// ⚠️ `SelectedTargetUnitId` vale `INDEX_NONE` quando la voce non nomina un bersaglio: allora non
			// si sa **chi** e' caduto, e non si abbatte nessuno. Indovinare qui toglierebbe dal campo
			// un'unita' che la traccia non ha mai dichiarato abbattuta.
			if (Caduto != INDEX_NONE && Caduto != 0)
			{
				const int32 i = IndiceDi(Out, Caduto, E.TgtCell);
				Out[i].bAlive = false;
			}
			break;
		}

		default:
			break;
		}
	}

	return Out;
}

TArray<FRTTracedUnitState> URTReplayStateLibrary::UnitsAtEnd(const TArray<FRTTurnLogEntry>& Entries,
	const TArray<FRTTracedUnitState>& Initial)
{
	// L'ultimo turno e l'ultima fase che la traccia contiene davvero. ⚠️ **Non si assume `MatchEnded`**:
	// `ERTMatchPhase` dichiara sette valori e il resolver ne emette cinque, quindi una costante scritta qui
	// sarebbe un limite superiore che vale per fortuna. Si legge cio' che c'e'.
	int32 UltimoTurno = 0;
	ERTMatchPhase UltimaFase = ERTMatchPhase::Planning;
	for (const FRTTurnLogEntry& E : Entries)
	{
		if (E.TurnNumber > UltimoTurno
			|| (E.TurnNumber == UltimoTurno && static_cast<uint8>(E.Phase) > static_cast<uint8>(UltimaFase)))
		{
			UltimoTurno = E.TurnNumber;
			UltimaFase = E.Phase;
		}
	}

	return UnitsAtPosition(Entries, Initial, UltimoTurno, UltimaFase);
}
