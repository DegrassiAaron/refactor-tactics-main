#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

namespace
{
	/** `SlideCells` in coda e con default: solo `Ice` lo valorizza, e li' e' scritto per nome. */
	FRTTerrainDef MakeTerrain(ERTHexSurface Surface, int32 MoveCost, bool bBlocksDashCharge,
		bool bBlocksLineOfSight, bool bConductsElectricity, int32 MaxTargetingRangeThrough,
		TArray<FRTActionEffectSpec> OnEnterEffects, int32 SlideCells = 0)
	{
		FRTTerrainDef Def;
		Def.Surface = Surface;
		Def.MoveCost = MoveCost;
		Def.bBlocksDashCharge = bBlocksDashCharge;
		Def.bBlocksLineOfSight = bBlocksLineOfSight;
		Def.bConductsElectricity = bConductsElectricity;
		Def.MaxTargetingRangeThrough = MaxTargetingRangeThrough;
		Def.OnEnterEffects = MoveTemp(OnEnterEffects);
		Def.SlideCells = SlideCells;
		return Def;
	}
}

TArray<FRTTerrainDef> URTTerrainLibrary::GetTerrainCatalog()
{
	TArray<FRTTerrainDef> Catalog;
	Catalog.Add(MakeTerrain(ERTHexSurface::Floor,        1, false, false, false, 0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::Rough,        2, true,  false, false, 0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::ShallowWater, 2, false, false, true,  0,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, 0) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Fire,         2, false, false, false, 0,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 10),
		  FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Burning, 2) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Conductive,   1, false, false, true,  0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::Smoke,        1, false, false, false, 2,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Obscured, 0) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Ice,          1, false, false, false, 0, {}, /*SlideCells*/ 1));
	Catalog.Add(MakeTerrain(ERTHexSurface::HighGround,   1, false, false, false, 0, {}));
	return Catalog;
}

TSet<FGameplayTag> URTTerrainLibrary::CellBoundStatusesFor(ERTHexSurface Surface)
{
	TSet<FGameplayTag> Sustained;
	for (const FRTActionEffectSpec& Effect : FindTerrainDef(Surface).OnEnterEffects)
	{
		// Durata 0 su un effetto di stato = "finche' sulla cella" (spec-terreni-e8.md §2.1). Le durate
		// esplicite (Burning 2) non sono sostenute dalla cella: scadono a tempo anche restando nel fuoco.
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusDuration == 0 && Effect.StatusTag.IsValid())
		{
			Sustained.Add(Effect.StatusTag);
		}
	}
	return Sustained;
}

FRTTerrainDef URTTerrainLibrary::FindTerrainDef(ERTHexSurface Surface)
{
	for (const FRTTerrainDef& Def : GetTerrainCatalog())
	{
		if (Def.Surface == Surface) { return Def; }
	}
	return FRTTerrainDef();
}

int32 URTTerrainLibrary::EffectiveTargetingRange(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, int32 RangeCells)
{
	if (Map == nullptr)
	{
		return RangeCells; // niente terreno da leggere: il fail-closed e' del chiamante
	}

	// Catalogo letto UNA volta e non per cella: FindTerrainDef ricostruisce le 8 righe a ogni chiamata (e ogni
	// riga alloca il suo TArray di effetti), e questa funzione sta nel ciclo interno di BuildCandidates —
	// celle raggiungibili x nemici x lunghezza della linea. Stesso risultato, senza il lavoro ripetuto.
	const TArray<FRTTerrainDef> Catalog = GetTerrainCatalog();

	// `HexLine` e' la stessa primitiva usata da HexHitCells per Shape::Line: la linea che il cap misura e'
	// quella su cui si spara davvero, non una seconda approssimazione.
	int32 Effective = RangeCells;
	for (const FRTCellId& LineCell : URTHexLibrary::HexLine(From, To))
	{
		const FRTHexCellData* CellData = Map->FindCell(LineCell);
		if (CellData == nullptr)
		{
			continue;
		}
		for (const FRTTerrainDef& Def : Catalog)
		{
			if (Def.Surface == CellData->Surface && Def.MaxTargetingRangeThrough > 0)
			{
				Effective = FMath::Min(Effective, Def.MaxTargetingRangeThrough);
				break;
			}
		}
	}
	return Effective;
}

TArray<FString> URTTerrainLibrary::ValidateTerrainCatalog()
{
	return ValidateCatalogEntries(GetTerrainCatalog());
}

TArray<FString> URTTerrainLibrary::ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog)
{
	TArray<FString> Errors;
	TSet<ERTHexSurface> Seen;
	for (const FRTTerrainDef& Def : Catalog)
	{
		bool bAlreadySeen = false;
		Seen.Add(Def.Surface, &bAlreadySeen);
		if (bAlreadySeen)
		{
			Errors.Add(FString::Printf(TEXT("Terreno duplicato: %d"), static_cast<int32>(Def.Surface)));
		}
		if (Def.MoveCost < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MoveCost negativo (%d)"), static_cast<int32>(Def.Surface), Def.MoveCost));
		}
		if (Def.MaxTargetingRangeThrough < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MaxTargetingRangeThrough negativo (%d)"), static_cast<int32>(Def.Surface), Def.MaxTargetingRangeThrough));
		}
	}
	return Errors;
}

TArray<FRTPropagationHit> URTTerrainLibrary::CollectElectricPropagation(const URTHexMapAsset* Map,
	const FRTCellId& SourceCell, int32 MaxSteps, int32 InitialDamage, int32 PropagatedDamage,
	const TArray<FRTHexCombatUnit>& Units)
{
	TArray<FRTPropagationHit> Hits;
	if (!Map)
	{
		return Hits; // fail-closed: senza mappa non si sa quali celle conducano
	}

	// Indice cella -> unita' vive che la occupano. Costruito una volta: il BFS non deve riscorrere l'array
	// delle unita' per ogni cella visitata, e l'ordine di questa mappa non influenza il risultato perche'
	// l'output viene ordinato alla fine con una chiave totale.
	TMultiMap<FRTCellId, int32> UnitsByCell;
	for (const FRTHexCombatUnit& Unit : Units)
	{
		if (Unit.bAlive)
		{
			UnitsByCell.Add(Unit.Cell, Unit.UnitId);
		}
	}

	auto Conducts = [Map](const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map->FindCell(Cell);
		return Data && FindTerrainDef(Data->Surface).bConductsElectricity;
	};

	auto AddHitsOn = [&Hits, &UnitsByCell](const FRTCellId& Cell, int32 Steps, int32 Damage)
	{
		TArray<int32> Occupants;
		UnitsByCell.MultiFind(Cell, Occupants);
		for (const int32 UnitId : Occupants)
		{
			FRTPropagationHit Hit;
			Hit.UnitId = UnitId;
			Hit.Steps = Steps;
			Hit.Damage = Damage;
			Hit.Cell = Cell;
			Hits.Add(Hit);
		}
	};

	// Passo 0: chi sta sulla cella sorgente incassa il colpo DIRETTO. Vale anche se la cella non conduce —
	// elettrificare un bersaglio all'asciutto lo colpisce comunque, semplicemente non propaga oltre.
	AddHitsOn(SourceCell, /*Steps*/ 0, InitialDamage);

	// Visita in AMPIEZZA sul grafo delle celle conduttive: ogni cella e' visitata una volta sola, quindi
	// ogni unita' compare una volta sola anche quando piu' cammini d'acqua la raggiungono (vincolo del
	// catalogo §2). Il BFS garantisce inoltre che la prima volta che si tocca una cella la si tocchi alla
	// distanza MINIMA: il numero di passi e' una proprieta' della cella, non dell'ordine di scoperta.
	if (MaxSteps > 0 && Conducts(SourceCell))
	{
		TSet<FRTCellId> Visited;
		Visited.Add(SourceCell);

		TArray<FRTCellId> Frontier;
		Frontier.Add(SourceCell);

		for (int32 Step = 1; Step <= MaxSteps && Frontier.Num() > 0; ++Step)
		{
			TArray<FRTCellId> Next;
			for (const FRTCellId& Cell : Frontier)
			{
				// Ordine dei vicini FISSO (`URTHexLibrary::Neighbors` restituisce le sei direzioni in ordine
				// di enum): il risultato non dipende dall'iterazione di un container non ordinato (#4).
				for (const FRTCellId& Neighbor : URTHexLibrary::Neighbors(Cell))
				{
					if (Visited.Contains(Neighbor) || !Conducts(Neighbor))
					{
						continue; // gia' raggiunta, oppure la catena si interrompe qui
					}
					Visited.Add(Neighbor);
					Next.Add(Neighbor);
					AddHitsOn(Neighbor, Step, PropagatedDamage);
				}
			}
			Frontier = MoveTemp(Next);
		}
	}

	// Ordine DICHIARATO dal catalogo: distanza -> cella -> unita'. E' totale (due unita' non possono
	// condividere cella e id), quindi la sequenza e' la stessa a ogni esecuzione.
	Hits.Sort([](const FRTPropagationHit& A, const FRTPropagationHit& B)
	{
		if (A.Steps != B.Steps) { return A.Steps < B.Steps; }
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return A.UnitId < B.UnitId;
	});
	return Hits;
}
