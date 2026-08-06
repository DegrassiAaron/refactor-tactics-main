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
