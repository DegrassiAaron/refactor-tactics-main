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

	// `IsLinearStyle` viveva qui e distingueva `Linear*` da `Budget` per dare a uno UNA direzione e all'altro
	// tre — la tabella per stile di ADR-0005 §1. Con ADR-0008 quella domanda non si pone piu': lo stile dice
	// solo QUALE budget leggere, e a rispondere e' `FamilyForStyle`, che e' pubblica e testabile.

	/** Il pivot da fermo e' universale a 3 (ADR-0008 §1): non e' un campo dell'eroe, e non lo diventa. */
	constexpr int32 StationaryPivotMaxSteps = 3;

	constexpr int32 MaxPivotSteps = 3;
}

ERTMovementFamily URTFacingLibrary::FamilyForStyle(ERTMovementStyle Style)
{
	switch (Style)
	{
	case ERTMovementStyle::None:
		return ERTMovementFamily::Stationary;
	case ERTMovementStyle::Budget:
		return ERTMovementFamily::Move;
	case ERTMovementStyle::LinearDash:
	case ERTMovementStyle::LinearCharge:
	case ERTMovementStyle::LinearLeap:
	case ERTMovementStyle::LinearPass:
		return ERTMovementFamily::Dash;
	}

	// ⚠️ NIENTE `default:` sopra, ed e' la ragione per cui questa riga esiste: con un `default` uno stile
	// aggiunto in coda a `ERTMovementStyle` ricadrebbe in silenzio su una famiglia, e un budget sbagliato
	// non fa crash — fa un insieme di rotazioni diverso da quello che l'ADR prescrive. Senza, il compilatore
	// avverte sull'enumeratore non gestito e la scelta torna a chi aggiunge lo stile.
	checkNoEntry();
	return ERTMovementFamily::Dash;
}

int32 URTFacingLibrary::PivotStepsForStyle(const FRTPivotBudget& Budget, ERTMovementStyle Style)
{
	switch (FamilyForStyle(Style))
	{
	case ERTMovementFamily::Stationary:
		return StationaryPivotMaxSteps;
	case ERTMovementFamily::Move:
		return FMath::Clamp(Budget.MoveEndMaxSteps, 0, MaxPivotSteps);
	case ERTMovementFamily::Dash:
		return FMath::Clamp(Budget.DashEndMaxSteps, 0, MaxPivotSteps);
	}

	checkNoEntry();
	return 0;
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
	ERTHexDirection Current, const FRTPivotBudget& Budget)
{
	TArray<ERTHexDirection> Legal;

	const int32 Steps = PivotStepsForStyle(Budget, Style);

	// Tre step da un esagono coprono l'intero cerchio: enumerare `D-3 .. D+3` produrrebbe la direzione
	// opposta due volte. Il ramo esiste per questo, non perche' lo stazionario sia un caso speciale — con
	// `Budget` o `Dash` a 3 si arriva qui allo stesso insieme.
	if (Steps >= 3)
	{
		Legal.Reserve(6);
		for (int32 I = 0; I < 6; ++I)
		{
			Legal.Add(static_cast<ERTHexDirection>(I));
		}
		return Legal; // gia' in ordine di enum
	}

	// Da fermo non si e' percorso nulla, quindi `FacingFromPath` restituisce `Current` e il cono si apre
	// attorno all'orientamento attuale: e' la stessa formula, non un secondo ramo.
	const ERTHexDirection Moved = FacingFromPath(Path, Current);

	Legal.Reserve(2 * Steps + 1);
	for (int32 Offset = -Steps; Offset <= Steps; ++Offset)
	{
		Legal.AddUnique(RotateDirection(Moved, Offset));
	}

	// Ordine stabile: per valore dell'enum, cosi' l'insieme non dipende da come e' stato costruito.
	// `CycleDeclaredFacing` ci si appoggia per essere ripetibile sotto la stessa sequenza di tasti.
	Legal.Sort([](const ERTHexDirection& A, const ERTHexDirection& B)
	{
		return static_cast<uint8>(A) < static_cast<uint8>(B);
	});
	return Legal;
}

bool URTFacingLibrary::TryApplyDeclaredFacing(ERTMovementStyle Style, const TArray<FRTCellId>& Path,
	ERTHexDirection Current, ERTHexDirection Declared, const FRTPivotBudget& Budget,
	ERTHexDirection& OutFacing)
{
	if (!LegalFacings(Style, Path, Current, Budget).Contains(Declared))
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
	// La GEOMETRIA sta in `DirectionWedgeTowards` e non e' duplicata qui: quella funzione e' l'unico produttore
	// di «in quale spicchio cade questa cella», ed e' anche l'unico posto in cui il semiaperto `a > 0, b >= 0`
	// e' scritto. Qui c'e' solo la parte che il facing aggiunge, cioe' rendere l'indice RELATIVO.
	// Anche il caso «stessa cella» e' suo: risponde `false`, e questa lo propaga invece di ricontrollarlo.
	int32 Sector = INDEX_NONE;
	if (!URTHexLibrary::DirectionWedgeTowards(DefenderCell, OriginCell, Sector))
	{
		return false;
	}

	constexpr int32 NumDirections = 6;
	const int32 Relative = ((Sector - static_cast<int32>(Facing)) % NumDirections + NumDirections) % NumDirections;
	OutDirection = static_cast<ERTRelativeDirection>(Relative);
	return true;
}

bool URTFacingLibrary::MakeHitCameFromSideEntry(const FRTCellId& DefenderCell, ERTHexDirection DefenderFacing,
	const FRTCellId& OriginCell, ERTMatchPhase Phase, FRTTurnLogEntry& OutEntry)
{
	// Il lato lo calcola `RelativeDirectionFrom` e non c'e' una seconda copia della regola: quella funzione e'
	// l'unica che sa rendere relativo l'indice, e il suo `false` — celle coincidenti in pianta — si propaga
	// invece di essere ricontrollato. `OutEntry` resta intatta, come `OutDirection` la' dentro.
	ERTRelativeDirection Side = ERTRelativeDirection::Front;
	if (!RelativeDirectionFrom(DefenderCell, DefenderFacing, OriginCell, Side))
	{
		return false;
	}

	OutEntry.Phase = Phase;
	OutEntry.Category = ERTLogCategory::Facing;
	OutEntry.Outcome = static_cast<uint8>(ERTFacingOutcome::HitCameFromSide);
	// 🔴 Il DIFENSORE in entrambi i campi. `OriginCell` e' un ingresso di questa funzione e non ne esce: e' il
	// punto in cui la privacy della voce smette di dipendere dalla disciplina di chi la costruisce.
	OutEntry.SrcCell = DefenderCell;
	OutEntry.TgtCell = DefenderCell;
	OutEntry.Amount = static_cast<int32>(Side);
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
