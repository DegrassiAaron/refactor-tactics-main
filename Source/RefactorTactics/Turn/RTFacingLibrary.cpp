#include "Turn/RTFacingLibrary.h"

#include "Map/RTHexLibrary.h"

namespace
{
	/** Le sei direzioni sono un ciclo: da E (0) si torna a SE (5), non a "-1". */
	ERTHexDirection RotateDirection(ERTHexDirection Dir, int32 Steps)
	{
		constexpr int32 NumDirections = 6;
		const int32 Rotated = (static_cast<int32>(Dir) + Steps) % NumDirections;
		return static_cast<ERTHexDirection>((Rotated + NumDirections) % NumDirections);
	}

	bool IsLinearStyle(ERTMovementStyle Style)
	{
		return Style == ERTMovementStyle::LinearDash || Style == ERTMovementStyle::LinearCharge
			|| Style == ERTMovementStyle::LinearLeap || Style == ERTMovementStyle::LinearPass;
	}
}

ERTHexDirection URTFacingLibrary::FacingFromPath(const TArray<FRTCellId>& Path, ERTHexDirection Current)
{
	if (Path.Num() < 2)
	{
		return Current; // nessuno spostamento: non c'e' niente da derivare
	}

	const FRTCellId& Last = Path.Last();
	const FRTCellId& Previous = Path[Path.Num() - 2];

	ERTHexDirection Derived = Current;
	if (URTHexLibrary::DirectionBetween(Previous, Last, Derived))
	{
		return Derived;
	}

	// Passi non adiacenti (salto): vale la direzione verso l'arrivo. Se nemmeno quella esiste — arrivo e
	// partenza coincidono nel piano, per esempio un salto fra layer sulla stessa colonna — il facing resta.
	if (URTHexLibrary::DirectionTowards(Previous, Last, Derived))
	{
		return Derived;
	}
	return Current;
}

TArray<ERTHexDirection> URTFacingLibrary::LegalFacings(ERTMovementStyle Style, const TArray<FRTCellId>& Path,
	ERTHexDirection Current)
{
	TArray<ERTHexDirection> Legal;

	if (Style == ERTMovementStyle::None)
	{
		Legal.Reserve(6);
		for (int32 I = 0; I < 6; ++I)
		{
			Legal.Add(static_cast<ERTHexDirection>(I));
		}
		return Legal; // gia' in ordine di enum
	}

	const ERTHexDirection Moved = FacingFromPath(Path, Current);

	if (IsLinearStyle(Style))
	{
		Legal.Add(Moved);
		return Legal;
	}

	// Budget: l'ultimo passo e le due adiacenti nel ciclo.
	Legal.Reserve(3);
	Legal.Add(RotateDirection(Moved, -1));
	Legal.Add(Moved);
	Legal.Add(RotateDirection(Moved, +1));

	// Ordine stabile: per valore dell'enum, cosi' l'insieme non dipende da come e' stato costruito.
	Legal.Sort([](const ERTHexDirection& A, const ERTHexDirection& B)
	{
		return static_cast<uint8>(A) < static_cast<uint8>(B);
	});
	return Legal;
}

bool URTFacingLibrary::TryApplyDeclaredFacing(ERTMovementStyle Style, const TArray<FRTCellId>& Path,
	ERTHexDirection Current, ERTHexDirection Declared, ERTHexDirection& OutFacing)
{
	if (!LegalFacings(Style, Path, Current).Contains(Declared))
	{
		OutFacing = Current;
		return false;
	}

	OutFacing = Declared;
	return true;
}

ERTHexDirection URTFacingLibrary::FacingAfterDisplacement(const FRTCellId& LandedCell, const FRTCellId& SourceCell,
	ERTDisplacementCause Cause, ERTHexDirection Current)
{
	if (Cause != ERTDisplacementCause::Forced)
	{
		return Current;
	}

	ERTHexDirection Towards = Current;
	if (URTHexLibrary::DirectionTowards(LandedCell, SourceCell, Towards))
	{
		return Towards;
	}
	return Current;
}

bool URTFacingLibrary::RelativeDirectionFrom(const FRTCellId& DefenderCell, ERTHexDirection Facing,
	const FRTCellId& OriginCell, ERTRelativeDirection& OutDirection)
{
	// La GEOMETRIA sta in `SectorIndexTowards` e non e' duplicata qui: quella funzione e' l'unico produttore
	// di «in quale spicchio cade questa cella», ed e' anche l'unico posto in cui il semiaperto `a > 0, b >= 0`
	// e' scritto. Qui c'e' solo la parte che il facing aggiunge, cioe' rendere l'indice RELATIVO.
	// Anche il caso «stessa cella» e' suo: risponde `false`, e questa lo propaga invece di ricontrollarlo.
	int32 Sector = INDEX_NONE;
	if (!URTHexLibrary::SectorIndexTowards(DefenderCell, OriginCell, Sector))
	{
		return false;
	}

	constexpr int32 NumDirections = 6;
	const int32 Relative = ((Sector - static_cast<int32>(Facing)) % NumDirections + NumDirections) % NumDirections;
	OutDirection = static_cast<ERTRelativeDirection>(Relative);
	return true;
}

namespace
{
	/** Voce di TurnLog per l'orientamento: la direzione viaggia in `Amount`, la cella dell'unita' e' la chiave. */
	FRTTurnLogEntry MakeFacingEntry(const FRTCellId& UnitCell, ERTHexDirection Direction, ERTFacingOutcome Reason,
		ERTMatchPhase Phase)
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = Phase;
		Entry.Category = ERTLogCategory::Facing;
		Entry.Outcome = static_cast<uint8>(Reason);
		Entry.SrcCell = UnitCell;
		Entry.TgtCell = UnitCell; // non applicabile: per convenzione del formato vale SrcCell
		Entry.Amount = static_cast<int32>(Direction);
		return Entry;
	}
}

void URTFacingLibrary::RecordFacingChange(FRTHexSimUnit& Unit, ERTHexDirection NewFacing, ERTFacingOutcome Reason,
	ERTMatchPhase Phase, TArray<FRTTurnLogEntry>& Log)
{
	const bool bRejection = (Reason == ERTFacingOutcome::DeclarationRejected);
	if (Unit.Facing == NewFacing && !bRejection)
	{
		return; // non e' successo niente: non c'e' niente da raccontare
	}

	// Il rifiuto si registra col facing CONSERVATO, non con quello chiesto: il log dice cosa vale, non cosa
	// era stato domandato — e cio' che vale, dopo un rifiuto, e' l'orientamento di prima.
	const ERTHexDirection Recorded = bRejection ? Unit.Facing : NewFacing;
	if (!bRejection)
	{
		Unit.Facing = NewFacing;
	}
	Log.Add(MakeFacingEntry(Unit.Cell, Recorded, Reason, Phase));
}

ERTHexDirection URTFacingLibrary::ReadFacingForConsumer(const FRTHexSimUnit& Unit, ERTFacingOutcome Consumer,
	ERTMatchPhase Phase, TArray<FRTTurnLogEntry>& Log)
{
	Log.Add(MakeFacingEntry(Unit.Cell, Unit.Facing, Consumer, Phase));
	return Unit.Facing;
}
