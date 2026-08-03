#include "Turn/RTPlaybackLibrary.h"

FVector URTPlaybackLibrary::InterpolateAlongPath(const TArray<FVector>& Waypoints, float Alpha)
{
	const int32 N = Waypoints.Num();
	if (N == 0)
	{
		return FVector::ZeroVector;
	}
	if (N == 1)
	{
		return Waypoints[0];
	}

	const int32 SegCount = N - 1;
	const float T = FMath::Clamp(Alpha, 0.f, 1.f) * SegCount; // posizione continua in [0, SegCount]
	int32 Seg = FMath::FloorToInt(T);
	float Frac;
	if (Seg >= SegCount)
	{
		Seg = SegCount - 1; // Alpha == 1: ultimo segmento, completo
		Frac = 1.f;
	}
	else
	{
		Frac = T - Seg;
	}
	return FMath::Lerp(Waypoints[Seg], Waypoints[Seg + 1], Frac);
}

float URTPlaybackLibrary::EstimatePlaybackSeconds(int32 MaxMoveSegments, int32 NumAttacks, int32 NumActivePhases,
	float CellsPerSecond, float PhaseBeatSeconds, float AttackShowSeconds)
{
	// Movimento in parallelo: domina il percorso piu' lungo. Velocita' non positiva = istantaneo.
	const float MoveSeconds = (CellsPerSecond > 0.f) ? (MaxMoveSegments / CellsPerSecond) : 0.f;
	const float AttackSeconds = NumAttacks * AttackShowSeconds;
	const float BeatSeconds = NumActivePhases * PhaseBeatSeconds;
	return MoveSeconds + AttackSeconds + BeatSeconds;
}

float URTPlaybackLibrary::SpeedMultiplierForCap(float EstimatedSeconds, float MaxSeconds)
{
	if (MaxSeconds <= 0.f || EstimatedSeconds <= MaxSeconds)
	{
		return 1.f;
	}
	return EstimatedSeconds / MaxSeconds;
}
