#include "Replay/RTBoundaryChecksum.h"

#include "Turn/RTMatchStateHash.h"
#include "Turn/RTTurnLogLibrary.h"

FString FRTBoundaryChecksum::ToString() const
{
	const UEnum* Tipo = StaticEnum<ERTMatchPhase>();
	const FString Fase = Tipo ? Tipo->GetNameStringByValue(static_cast<int64>(Phase)) : TEXT("?");

	// ⚠️ Il `#N` solo quando il boundary e' di ciclo: `INDEX_NONE` E' la fase intera, e stampare `#-1`
	// direbbe che esiste un micro-step negativo invece che nessun micro-step.
	if (MicroStepIndex == INDEX_NONE)
	{
		return FString::Printf(TEXT("T%d|%s"), TurnNumber, *Fase);
	}
	return FString::Printf(TEXT("T%d|%s#%d"), TurnNumber, *Fase, MicroStepIndex);
}

namespace
{
	/**
	 * La chiave di un boundary: la terna, e nient'altro.
	 *
	 * ⚠️ Non e' `FRTBoundaryChecksum` senza l'hash: quello e' il **risultato**, questa e' l'identita' su cui
	 * si deduplica e si ordina prima che un hash esista. Tenerli separati evita di confrontare per errore
	 * due boundary sul loro contenuto invece che sul loro luogo.
	 */
	struct FRTBoundaryKey
	{
		int32 TurnNumber = 0;
		ERTMatchPhase Phase = ERTMatchPhase::Planning;
		int32 MicroStepIndex = INDEX_NONE;

		FRTBoundaryKey() = default;
		FRTBoundaryKey(int32 InTurn, ERTMatchPhase InPhase, int32 InMicroStep)
			: TurnNumber(InTurn), Phase(InPhase), MicroStepIndex(InMicroStep) {}

		bool operator==(const FRTBoundaryKey& Other) const
		{
			return TurnNumber == Other.TurnNumber
				&& Phase == Other.Phase
				&& MicroStepIndex == Other.MicroStepIndex;
		}
	};
}

TArray<FRTBoundaryChecksum> URTBoundaryChecksumLibrary::ChecksumsAlongTrace(const URTHexMapAsset* Map,
	const TArray<FRTTurnLogEntry>& Entries, const TArray<FRTTracedUnitState>& Initial,
	ERTTurnLogFormatVersion Version)
{
	TArray<FRTBoundaryChecksum> Out;

	// 🔑 Sotto `WithMicroStep` il campo non esiste nella traccia e la deserializzazione lo lascia a `0` su
	// OGNI voce. Chiavare su quello direbbe `T2|Move#0` dove nessun micro-step e' mai stato scritto: qui il
	// taglio fine si spegne, e ogni boundary vale la fase intera. E' il comportamento storico, dichiarato.
	const bool bTracciaPortaIlMicroStep =
		static_cast<uint16>(Version) >= static_cast<uint16>(ERTTurnLogFormatVersion::WithMicroStep);

	// --- i boundary che la traccia attraversa, senza ripetizioni ---------------------------------------
	//
	// 🔴 **L'ordine di APPARIZIONE non basta piu', e prima bastava.** `EntryLess` ordina per turno e fase —
	// quindi finche' la chiave era `(Turno, Fase)` leggere le voci in sequenza le produceva gia' ordinate.
	// **`MicroStepIndex` non e' nel comparatore**: due voci della stessa fase con micro-step `2` e `0`
	// compaiono nell'ordine deciso da `Category` e dalle celle, non dal loro indice. Si ordina qui, e la
	// regola e' esplicita invece che ereditata da un invariante che non copre questa coordinata.
	TArray<FRTBoundaryKey> Boundaries;
	for (const FRTTurnLogEntry& E : Entries)
	{
		const FRTBoundaryKey Chiave(E.TurnNumber, E.Phase,
			bTracciaPortaIlMicroStep ? E.MicroStepIndex : INDEX_NONE);
		if (!Boundaries.Contains(Chiave))
		{
			Boundaries.Add(Chiave);
		}
	}

	// ⛔ `INDEX_NONE` va ULTIMO nella sua fase, malgrado `-1 < 0`: e' la stessa regola che `NonOltre`
	// applica in `URTReplayStateLibrary`, e le due DEVONO concordare — la sequenza che si costruisce qui
	// e' l'indice con cui si ricostruisce li'. Ordinarlo per numero applicherebbe l'arrivo di un'unita'
	// prima delle barriere che ha attraversato per arrivarci: uno stato che la partita non ha attraversato.
	Boundaries.Sort([](const FRTBoundaryKey& A, const FRTBoundaryKey& B)
	{
		if (A.TurnNumber != B.TurnNumber) { return A.TurnNumber < B.TurnNumber; }
		if (A.Phase != B.Phase)           { return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase); }
		if (A.MicroStepIndex == B.MicroStepIndex) { return false; }
		if (A.MicroStepIndex == INDEX_NONE) { return false; } // la fase intera chiude
		if (B.MicroStepIndex == INDEX_NONE) { return true; }
		return A.MicroStepIndex < B.MicroStepIndex;
	});

	Out.Reserve(Boundaries.Num());
	for (const FRTBoundaryKey& B : Boundaries)
	{
		// Lo stato a quel punto, ricostruito dalla traccia: nessuna riesecuzione, nessun resolver.
		// `UnitsAtBoundary` con `INDEX_NONE` E' `UnitsAtPosition` — quella delega qui — quindi il caso
		// phase-only non prende una strada diversa: prende la stessa strada con la terza coordinata spenta.
		const TArray<FRTTracedUnitState> Stati =
			URTReplayStateLibrary::UnitsAtBoundary(Entries, Initial, B.TurnNumber, B.Phase, B.MicroStepIndex);

		// --- la traduzione verso il digest, e cio' che resta al default -------------------------------
		//
		// 🔴 `Health`, `Shield` e `Statuses` **non si popolano**: la traccia non li dichiara, e
		// dedurli sommando le voci di danno sarebbe ricostruire uno stato che nessuno ha scritto. Restano
		// al default, uguali ovunque, quindi non discriminano — ed e' dichiarato nell'header invece che
		// scoperto da chi legge un checksum che non distingue due stati diversi.
		TArray<FRTUnitStateDigest> Digest;
		Digest.Reserve(Stati.Num());
		for (const FRTTracedUnitState& S : Stati)
		{
			// ⛔ Chi e' caduto NON entra: un'unita' morta non ha uno stato da confrontare, e lasciarla
			// dentro con la sua ultima cella direbbe che e' ancora li'.
			if (!S.bAlive)
			{
				continue;
			}

			FRTUnitStateDigest D;
			D.UnitId = S.UnitId;
			D.Cell = S.Cell;
			D.Facing = S.Facing;
			Digest.Add(D);
		}

		FRTBoundaryChecksum C;
		C.TurnNumber = B.TurnNumber;
		C.Phase = B.Phase;
		C.MicroStepIndex = B.MicroStepIndex;
		// 🔑 Il calcolo passa da qui e da nessun'altra parte: nessuna seconda aritmetica di hash.
		// `TeamScores` vuoto: il progresso obiettivo non e' nella traccia, e vale per il checksum di
		// boundary la stessa regola dei quattro campi sopra — non si inventa cio' che non e' dichiarato.
		C.Hash = static_cast<int64>(URTMatchStateHashLibrary::HashMatchState(Map, Digest, TArray<int32>()));
		Out.Add(C);
	}

	return Out;
}

int32 URTBoundaryChecksumLibrary::FirstDivergence(const TArray<FRTBoundaryChecksum>& A,
	const TArray<FRTBoundaryChecksum>& B)
{
	const int32 Comune = FMath::Min(A.Num(), B.Num());
	for (int32 i = 0; i < Comune; ++i)
	{
		// ⚠️ Si confronta il **luogo** oltre all'hash: due sequenze che attraversassero boundary DIVERSI con
		// lo stesso hash sono divergenti lo stesso — e sarebbe la divergenza piu' insidiosa, perche' i
		// numeri coinciderebbero.
		//
		// 🔴 **`MicroStepIndex` fa parte del luogo** (`#2374`). Ometterlo qui lascerebbe passare due
		// esecuzioni che attraversano un numero diverso di barriere dentro la stessa fase ogni volta che gli
		// hash pareggiano — e pareggiano spesso, perche' un micro-step che non muove nessuno lascia lo stato
		// identico al precedente. La chiave si e' allargata: il confronto si allarga con lei.
		if (A[i].TurnNumber != B[i].TurnNumber
			|| A[i].Phase != B[i].Phase
			|| A[i].MicroStepIndex != B[i].MicroStepIndex
			|| A[i].Hash != B[i].Hash)
		{
			return i;
		}
	}

	// Lunghezze diverse: la prima posizione che una sola delle due possiede.
	return (A.Num() == B.Num()) ? INDEX_NONE : Comune;
}

FString URTBoundaryChecksumLibrary::DescribeDivergence(const TArray<FRTBoundaryChecksum>& A,
	const TArray<FRTBoundaryChecksum>& B)
{
	const int32 i = FirstDivergence(A, B);
	if (i == INDEX_NONE)
	{
		return FString();
	}

	// Una delle due puo' non avere quella posizione: si dice, invece di stampare un boundary inventato.
	const bool bInA = A.IsValidIndex(i);
	const bool bInB = B.IsValidIndex(i);

	if (!bInA || !bInB)
	{
		const FRTBoundaryChecksum& Esistente = bInA ? A[i] : B[i];
		return FString::Printf(
			TEXT("le due esecuzioni hanno un numero diverso di boundary (%d contro %d): la prima a mancare e' %s"),
			A.Num(), B.Num(), *Esistente.ToString());
	}

	// 🔴 Il **luogo** e' la terna, non la coppia (`#2374`): due esecuzioni che alla stessa posizione hanno
	// micro-step diversi attraversano boundary diversi, ed e' quello che il messaggio deve dire. Senza
	// `MicroStepIndex` qui il caso cadrebbe nel ramo sotto, che stamperebbe *«divergono al boundary
	// T1|Move#0: 0x1234 contro 0x1234»* — due hash identici come spiegazione di una divergenza.
	if (A[i].TurnNumber != B[i].TurnNumber
		|| A[i].Phase != B[i].Phase
		|| A[i].MicroStepIndex != B[i].MicroStepIndex)
	{
		return FString::Printf(TEXT("le due esecuzioni attraversano boundary diversi: %s contro %s"),
			*A[i].ToString(), *B[i].ToString());
	}

	return FString::Printf(TEXT("divergono al boundary %s: 0x%08X contro 0x%08X"),
		*A[i].ToString(), static_cast<uint32>(A[i].Hash), static_cast<uint32>(B[i].Hash));
}
