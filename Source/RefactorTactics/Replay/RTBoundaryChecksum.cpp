#include "Replay/RTBoundaryChecksum.h"

#include "Turn/RTMatchStateHash.h"
#include "Turn/RTTurnLogLibrary.h"

FString FRTBoundaryChecksum::ToString() const
{
	const UEnum* Tipo = StaticEnum<ERTMatchPhase>();
	const FString Fase = Tipo ? Tipo->GetNameStringByValue(static_cast<int64>(Phase)) : TEXT("?");
	return FString::Printf(TEXT("T%d|%s"), TurnNumber, *Fase);
}

TArray<FRTBoundaryChecksum> URTBoundaryChecksumLibrary::ChecksumsAlongTrace(const URTHexMapAsset* Map,
	const TArray<FRTTurnLogEntry>& Entries, const TArray<FRTTracedUnitState>& Initial)
{
	TArray<FRTBoundaryChecksum> Out;

	// --- i boundary che la traccia attraversa, senza ripetizioni e nell'ordine in cui compaiono --------
	//
	// ⚠️ **L'ordine e' quello CANONICO della traccia, non quello di emissione**, e va bene perche' la
	// domanda e' *«a quale barriera i due divergono»*: entrambe le sequenze si costruiscono con la stessa
	// regola, quindi si confrontano posizione per posizione. Pretendere l'ordine di emissione sarebbe
	// pretendere un dato che la traccia non porta — vedi `#1880`.
	TArray<TPair<int32, ERTMatchPhase>> Boundaries;
	for (const FRTTurnLogEntry& E : Entries)
	{
		const TPair<int32, ERTMatchPhase> Chiave(E.TurnNumber, E.Phase);
		if (!Boundaries.Contains(Chiave))
		{
			Boundaries.Add(Chiave);
		}
	}

	Out.Reserve(Boundaries.Num());
	for (const TPair<int32, ERTMatchPhase>& B : Boundaries)
	{
		// Lo stato a quel punto, ricostruito dalla traccia: nessuna riesecuzione, nessun resolver.
		const TArray<FRTTracedUnitState> Stati =
			URTReplayStateLibrary::UnitsAtPosition(Entries, Initial, B.Key, B.Value);

		// --- la traduzione verso il digest, e cio' che resta al default -------------------------------
		//
		// 🔴 `Health`, `Shield`, `Energy` e `Statuses` **non si popolano**: la traccia non li dichiara, e
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
		C.TurnNumber = B.Key;
		C.Phase = B.Value;
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
		// ⚠️ Si confrontano anche turno e fase, non solo l'hash: due sequenze che attraversassero boundary
		// DIVERSI con lo stesso hash sono divergenti lo stesso — e sarebbe la divergenza piu' insidiosa,
		// perche' i numeri coinciderebbero.
		if (A[i].TurnNumber != B[i].TurnNumber || A[i].Phase != B[i].Phase || A[i].Hash != B[i].Hash)
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

	if (A[i].TurnNumber != B[i].TurnNumber || A[i].Phase != B[i].Phase)
	{
		return FString::Printf(TEXT("le due esecuzioni attraversano boundary diversi: %s contro %s"),
			*A[i].ToString(), *B[i].ToString());
	}

	return FString::Printf(TEXT("divergono al boundary %s: 0x%08X contro 0x%08X"),
		*A[i].ToString(), static_cast<uint32>(A[i].Hash), static_cast<uint32>(B[i].Hash));
}
