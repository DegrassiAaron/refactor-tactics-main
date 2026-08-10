#include "Map/RTArenaCriteriaLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

namespace
{
	/** Le due estremita' non contano: sono gli spawn, e un criterio sul percorso parla di cio' che sta in mezzo. */
	TSet<FRTCellId> InteriorOf(const TArray<FRTCellId>& Path)
	{
		TSet<FRTCellId> Interior;
		for (int32 i = 1; i < Path.Num() - 1; ++i) { Interior.Add(Path[i]); }
		return Interior;
	}

	bool SpawnsUsable(const URTHexMapAsset* Map, const FRTCellId& A, const FRTCellId& B)
	{
		return Map && Map->ContainsCell(A) && Map->ContainsCell(B) && !(A == B);
	}
}

FRTArenaCriterionResult URTArenaCriteriaLibrary::CheckCoverage(const URTHexMapAsset* Map,
	const FRTCellId& SpawnA, const FRTCellId& SpawnB)
{
	if (!SpawnsUsable(Map, SpawnA, SpawnB))
	{
		return FRTArenaCriterionResult::NotEvaluable(TEXT("mappa nulla o spawn assenti/coincidenti"));
	}

	// Celle che bloccano la vista SUL segmento, estremi esclusi: gli estremi sono gli spawn, e la LOS non li
	// considera mai occlusori (nessuno si copre da solo).
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(SpawnA, SpawnB);
	int32 BlockersOnLine = 0;
	for (int32 i = 1; i < Line.Num() - 1; ++i)
	{
		const FRTHexCellData* Cell = Map->FindCell(Line[i]);
		if (Cell && Cell->bBlocksLineOfSight) { ++BlockersOnLine; }
	}

	// Il conteggio da solo misurerebbe l'intenzione: chiediamo anche che la vista sia davvero interrotta.
	const bool bSightBlocked = !URTHexVisionLibrary::HasLineOfSight(Map, SpawnA, SpawnB);

	const FString Detail = FString::Printf(
		TEXT("%d cell%s che bloccano la vista sul segmento (ne servono 2), linea di tiro %s"),
		BlockersOnLine, BlockersOnLine == 1 ? TEXT("a") : TEXT("e"),
		bSightBlocked ? TEXT("interrotta") : TEXT("LIBERA"));

	return (BlockersOnLine >= 2 && bSightBlocked)
		? FRTArenaCriterionResult::Pass(Detail)
		: FRTArenaCriterionResult::Fail(Detail);
}

FRTArenaCriterionResult URTArenaCriteriaLibrary::CheckCostBite(const URTHexMapAsset* Map,
	const FRTCellId& SpawnA, const FRTCellId& SpawnB, int32 MoveBudget)
{
	if (!SpawnsUsable(Map, SpawnA, SpawnB))
	{
		return FRTArenaCriterionResult::NotEvaluable(TEXT("mappa nulla o spawn assenti/coincidenti"));
	}

	const FRTHexPathResult Path = URTHexPathLibrary::FindPath(Map, SpawnA, SpawnB);
	if (Path.Status != ERTHexPathStatus::Success)
	{
		return FRTArenaCriterionResult::NotEvaluable(
			TEXT("nessun percorso fra gli spawn: la mappa e' scollegata, prima ancora che cara"));
	}

	// Una cella cara sul percorso OTTIMALE: se la zona si aggira senza rinunciare a nulla, l'A* la evita
	// e qui non compare — ed e' esattamente il caso che il criterio deve rifiutare.
	int32 MaxCellCost = 0;
	for (int32 i = 1; i < Path.Path.Num(); ++i) // il costo si paga entrando: la cella di partenza non si paga
	{
		if (const FRTHexCellData* Cell = Map->FindCell(Path.Path[i]))
		{
			MaxCellCost = FMath::Max(MaxCellCost, Cell->MoveCost);
		}
	}

	const bool bCrossesExpensive = MaxCellCost > 1;
	const bool bOverBudget = Path.TotalCost > MoveBudget;

	const FString Detail = FString::Printf(
		TEXT("percorso ottimale costo %d su budget %d (%s), cella piu' cara attraversata %d (%s)"),
		Path.TotalCost, MoveBudget, bOverBudget ? TEXT("oltre") : TEXT("dentro"),
		MaxCellCost, bCrossesExpensive ? TEXT("terreno costoso") : TEXT("tutto a costo 1"));

	return (bCrossesExpensive && bOverBudget)
		? FRTArenaCriterionResult::Pass(Detail)
		: FRTArenaCriterionResult::Fail(Detail);
}

float URTArenaCriteriaLibrary::ExposureRatio(const URTHexMapAsset* Map, const TArray<FRTCellId>& Path,
	const FRTCellId& Observer)
{
	int32 Exposed = 0;
	int32 Interior = 0;
	for (int32 i = 1; i < Path.Num() - 1; ++i) // estremi esclusi: sono gli spawn, e sono sempre visti da se stessi
	{
		++Interior;
		if (URTHexVisionLibrary::HasLineOfSight(Map, Observer, Path[i])) { ++Exposed; }
	}
	// Percorso senza celle interne (spawn adiacenti): niente da esporre, e nessun trade-off possibile.
	return Interior > 0 ? static_cast<float>(Exposed) / static_cast<float>(Interior) : 0.f;
}

FRTArenaCriterionResult URTArenaCriteriaLibrary::CheckTwoRoutes(const URTHexMapAsset* Map,
	const FRTCellId& SpawnA, const FRTCellId& SpawnB, float MaxCostRatio, float MinExposureGap)
{
	if (!SpawnsUsable(Map, SpawnA, SpawnB))
	{
		return FRTArenaCriterionResult::NotEvaluable(TEXT("mappa nulla o spawn assenti/coincidenti"));
	}

	const FRTHexPathResult First = URTHexPathLibrary::FindPath(Map, SpawnA, SpawnB);
	if (First.Status != ERTHexPathStatus::Success)
	{
		return FRTArenaCriterionResult::NotEvaluable(TEXT("nessun percorso fra gli spawn"));
	}

	// La seconda rotta si cerca vietando le celle INTERNE della prima: e' il modo di chiedere «esiste
	// un'alternativa che non ricalchi la strada principale». Gli estremi restano liberi, o non partirebbe.
	const TSet<FRTCellId> Blocked = InteriorOf(First.Path);
	const FRTHexPathResult Second = URTHexPathLibrary::FindPathAvoiding(Map, SpawnA, SpawnB, &Blocked);

	if (Second.Status != ERTHexPathStatus::Success)
	{
		// Verdetto sulla mappa, non impossibilita' di misurare: e' il difetto che il criterio cerca.
		return FRTArenaCriterionResult::Fail(FString::Printf(
			TEXT("una sola rotta (costo %d): nessuna alternativa disgiunta, il percorso e' obbligato"),
			First.TotalCost));
	}

	// Costo: la lunga non deve essere sproporzionata, o non e' una scelta ma una penitenza.
	const int32 Low = FMath::Min(First.TotalCost, Second.TotalCost);
	const int32 High = FMath::Max(First.TotalCost, Second.TotalCost);
	const bool bCostComparable = (Low > 0) && (static_cast<float>(High) <= static_cast<float>(Low) * MaxCostRatio);

	// Esposizione: ciascuna rotta vista dallo spawn AVVERSARIO. In FRAZIONE, non in numero assoluto — la rotta
	// piu' lunga ha piu' celle e quindi quasi sempre un numero *diverso* di celle esposte: contarle misurerebbe
	// la lunghezza, non la copertura (su campo aperto e simmetrico il criterio passava).
	const float ExposureFirst = ExposureRatio(Map, First.Path, SpawnB);
	const float ExposureSecond = ExposureRatio(Map, Second.Path, SpawnB);

	// La direzione conta: chi paga di piu' in movimento deve comprare copertura. Costo uguale -> nessuna delle
	// due e' «la piu' lunga», e allora basta che le esposizioni differiscano.
	// Lo scarto minimo esiste perche' senza bastava UNA cella: `GeneratedTestArena` passava con 57% contro
	// 56%, che non e' una scelta ma rumore di arrotondamento.
	const bool bSameCost = (First.TotalCost == Second.TotalCost);
	const float ExpensiveExposure = (First.TotalCost >= Second.TotalCost) ? ExposureFirst : ExposureSecond;
	const float CheapExposure = (First.TotalCost >= Second.TotalCost) ? ExposureSecond : ExposureFirst;
	const bool bTradeOff = bSameCost
		? (FMath::Abs(ExposureFirst - ExposureSecond) >= MinExposureGap)
		: (CheapExposure - ExpensiveExposure >= MinExposureGap);

	const FString Detail = FString::Printf(
		TEXT("rotte disgiunte costo %d e %d (rapporto %.2f, max %.2f: %s), esposizione %.0f%% e %.0f%% ")
		TEXT("(scarto minimo %.0f%%: %s)"),
		First.TotalCost, Second.TotalCost,
		Low > 0 ? static_cast<float>(High) / static_cast<float>(Low) : 0.f, MaxCostRatio,
		bCostComparable ? TEXT("ok") : TEXT("SPROPORZIONATE"),
		ExposureFirst * 100.f, ExposureSecond * 100.f, MinExposureGap * 100.f,
		bTradeOff ? TEXT("trade-off reale")
			: (bSameCost ? TEXT("esposizione troppo simile: nessun trade-off")
			              : TEXT("la rotta piu' cara NON e' abbastanza piu' coperta: nessuno la sceglierebbe")));

	return (bCostComparable && bTradeOff)
		? FRTArenaCriterionResult::Pass(Detail)
		: FRTArenaCriterionResult::Fail(Detail);
}

FRTArenaCriteriaReport URTArenaCriteriaLibrary::Evaluate(const URTHexMapAsset* Map,
	const FRTCellId& SpawnA, const FRTCellId& SpawnB, int32 MoveBudget, float MaxCostRatio, float MinExposureGap)
{
	FRTArenaCriteriaReport Report;
	Report.SpawnA = SpawnA;
	Report.SpawnB = SpawnB;
	Report.Coverage = CheckCoverage(Map, SpawnA, SpawnB);
	Report.CostBite = CheckCostBite(Map, SpawnA, SpawnB, MoveBudget);
	Report.TwoRoutes = CheckTwoRoutes(Map, SpawnA, SpawnB, MaxCostRatio, MinExposureGap);
	return Report;
}

FRTArenaCriteriaReport URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(const URTHexMapAsset* Map,
	int32 NumPerTeam, int32 Layer, int32 MoveBudget, float MaxCostRatio, float MinExposureGap)
{
	// Gli spawn NON si inventano: si chiedono alla stessa funzione che allestisce la partita, o si misurerebbe
	// una partita che non si gioca.
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, NumPerTeam, Layer);
	if (Start.Num() < NumPerTeam * 2)
	{
		FRTArenaCriteriaReport Report;
		const FString Why = FString::Printf(
			TEXT("celle di partenza insufficienti: %d per un %dv%d"), Start.Num(), NumPerTeam, NumPerTeam);
		Report.Coverage = FRTArenaCriterionResult::NotEvaluable(Why);
		Report.CostBite = FRTArenaCriterionResult::NotEvaluable(Why);
		Report.TwoRoutes = FRTArenaCriterionResult::NotEvaluable(Why);
		return Report;
	}

	// Le prime NumPerTeam al team 0, le successive al team 1 (contratto di PickStartCells).
	return Evaluate(Map, Start[0], Start[NumPerTeam], MoveBudget, MaxCostRatio, MinExposureGap);
}

FString URTArenaCriteriaLibrary::FormatReport(const FRTArenaCriteriaReport& Report)
{
	auto Mark = [](const FRTArenaCriterionResult& R) -> const TCHAR*
	{
		if (R.bNotEvaluable) { return TEXT("[?]"); }
		return R.bPassed ? TEXT("[ok]") : TEXT("[no]");
	};

	return FString::Printf(
		TEXT("spawn (%d,%d,%d) <-> (%d,%d,%d)\n")
		TEXT("  %s copertura : %s\n")
		TEXT("  %s costo     : %s\n")
		TEXT("  %s rotte     : %s"),
		Report.SpawnA.X, Report.SpawnA.Y, Report.SpawnA.Layer,
		Report.SpawnB.X, Report.SpawnB.Y, Report.SpawnB.Layer,
		Mark(Report.Coverage), *Report.Coverage.Detail,
		Mark(Report.CostBite), *Report.CostBite.Detail,
		Mark(Report.TwoRoutes), *Report.TwoRoutes.Detail);
}
