#include "Turn/RTCombatLog.h"

#include "Unit/RTUnit.h" // ARTUnit: `StableUnitId` e `Cell`, i due campi che il soggetto congela

FRTLogSubject FRTLogSubject::Unit(const ARTUnit* InUnit)
{
	FRTLogSubject S;
	S.Unit_ = InUnit;
	S.StableUnitId = InUnit ? InUnit->StableUnitId : INDEX_NONE;
	return S;
}

FRTLogSubject FRTLogSubject::UnitAt(const ARTUnit* InUnit, const FRTCellId& InFactCell)
{
	FRTLogSubject S = Unit(InUnit);
	S.bFactCell = true;
	S.FactCell = InFactCell;
	return S;
}

FRTCellId FRTLogSubject::GetFactCell() const
{
	if (bFactCell)
	{
		return FactCell;
	}
	// Fuori dalla finestra di `ResolveMovement` l'Actor E' la cella del fatto, ed e' il caso della grande
	// maggioranza dei produttori: il default non e' un ripiego.
	return Unit_ ? Unit_->Cell : FRTCellId();
}

FRTLogSubject FRTLogSubject::Frozen(int32 InStableUnitId, const FRTKnowledgeVerdict& InVerdict)
{
	FRTLogSubject S;
	S.bFrozen = true;
	S.StableUnitId = InStableUnitId;
	S.FrozenVerdict = InVerdict;
	return S;
}

FRTLogSubject FRTLogSubject::World()
{
	FRTLogSubject S;
	S.bWorld = true;
	return S;
}

TArray<FString> URTCombatLogLibrary::ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines,
	int32 ObserverTeamId)
{
	TArray<FString> Out;
	Out.Reserve(Lines.Num());
	for (const FRTCombatLogLine& L : Lines)
	{
		// 🔴 **Nessuna vista costruita qui, ed e' il punto di [D-223].** La domanda «puo' leggerla?» ha gia'
		// una risposta, decisa quando la riga e' nata: interrogare la conoscenza di ADESSO risponderebbe a
		// una domanda diversa, e per un soggetto nel frattempo distrutto non risponderebbe affatto.
		//
		// ⚠️ Il verdetto e' fail-closed di default: una riga che arrivasse qui senza verdetto non si legge.
		if (L.Verdict.AllowsTeam(ObserverTeamId))
		{
			Out.Add(L.Text);
		}
	}
	return Out; // ordine di produzione, mai riordinato
}
