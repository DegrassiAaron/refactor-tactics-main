#include "Terrain/RTTerrainLibrary.h"

int32 URTTerrainLibrary::CellMoveCost(const FRTTerrainProps& Props)
{
	if (Props.bBlocksMovement)
	{
		return RT_BLOCKED_COST; // il blocco prevale sul costo
	}
	return 1 + FMath::Max(0, Props.ExtraMoveCost);
}

bool URTTerrainLibrary::BlocksMovement(const FRTTerrainProps& Props)
{
	return Props.bBlocksMovement;
}

bool URTTerrainLibrary::BlocksVision(const FRTTerrainProps& Props)
{
	return Props.bBlocksVision;
}
