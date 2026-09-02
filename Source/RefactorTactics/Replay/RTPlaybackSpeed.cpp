// Copyright RefactorTactics. All Rights Reserved.

#include "Replay/RTPlaybackSpeed.h"

float URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed Speed)
{
	switch (Speed)
	{
	case ERTPlaybackSpeed::Quarter:   return 4.0f;
	case ERTPlaybackSpeed::Half:      return 2.0f;
	case ERTPlaybackSpeed::Normal:    return 1.0f;
	case ERTPlaybackSpeed::Double:    return 0.5f;
	case ERTPlaybackSpeed::Quadruple: return 0.25f;
	case ERTPlaybackSpeed::Instant:   return 0.0f;
	}

	// ⚠️ Nessun `default:` sopra, ed è deliberato: un valore nuovo nell'enum fa scattare l'avviso del
	// compilatore sullo switch invece di cadere in un ramo che «funziona» dando 1x. Qui si arriva solo
	// con un cast da un intero fuori scala.
	return 1.0f;
}

bool URTPlaybackSpeedLibrary::IsInstant(ERTPlaybackSpeed Speed)
{
	return Speed == ERTPlaybackSpeed::Instant;
}

TArray<ERTPlaybackSpeed> URTPlaybackSpeedLibrary::AllSpeeds()
{
	return {
		ERTPlaybackSpeed::Quarter,
		ERTPlaybackSpeed::Half,
		ERTPlaybackSpeed::Normal,
		ERTPlaybackSpeed::Double,
		ERTPlaybackSpeed::Quadruple,
		ERTPlaybackSpeed::Instant
	};
}

ERTPlaybackSpeed URTPlaybackSpeedLibrary::NextSpeed(ERTPlaybackSpeed Speed)
{
	const TArray<ERTPlaybackSpeed> Scale = AllSpeeds();
	const int32 Index = Scale.IndexOfByKey(Speed);
	if (Index == INDEX_NONE)
	{
		return ERTPlaybackSpeed::Normal;
	}
	return Scale[(Index + 1) % Scale.Num()];
}
