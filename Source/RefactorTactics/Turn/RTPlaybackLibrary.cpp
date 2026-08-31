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

int32 URTPlaybackLibrary::AttacksToShow(int32 NumAttacks, float PhaseElapsed, float AttackShowSeconds)
{
	if (NumAttacks <= 0)
	{
		return 0;
	}
	if (AttackShowSeconds <= 0.f)
	{
		return NumAttacks; // nessuno scaglionamento richiesto: la fase li mostra tutti insieme
	}
	// Il primo colpo esce a fase appena iniziata (1 + ...): scaglionare non deve ritardare l'inizio,
	// altrimenti una fase con un colpo solo resterebbe muta per tutta AttackShowSeconds.
	const float Elapsed = FMath::Max(0.f, PhaseElapsed);
	return FMath::Min(NumAttacks, 1 + FMath::FloorToInt(Elapsed / AttackShowSeconds));
}

float URTPlaybackLibrary::PhaseDuration(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
	float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	// Il tempo di movimento e' lo stesso calcolo per tutte le fasi che muovono, Blast compreso: si scrive
	// una volta sola perche' due copie divergerebbero alla prima modifica di una delle due.
	const float MoveTime = (CellsPerSecond > 0.f)
		? (FMath::Max(0, MaxMoveSegments) / CellsPerSecond)
		: 0.f;

	switch (Phase)
	{
	case ERTMatchPhase::Dash:
	case ERTMatchPhase::Move:
		return MoveTime;

	case ERTMatchPhase::Blast:
	{
		// `Max(1, ...)`: un Blast di sola spinta non ha colpi, e una fase che si vede non puo' durare zero.
		const float AttackTime = FMath::Max(1, NumAttacks) * AttackShowSeconds;
		// `Max` e non somma: i colpi si vedono MENTRE il bersaglio scivola, non dopo.
		return FMath::Max(AttackTime, MoveTime);
	}

	default:
		return PhaseBeatSeconds; // Prep, Cleanup, Planning: un beat, senza moto da attendere
	}
}

float URTPlaybackLibrary::SpeedMultiplierForCap(float EstimatedSeconds, float MaxSeconds)
{
	if (MaxSeconds <= 0.f || EstimatedSeconds <= MaxSeconds)
	{
		return 1.f;
	}
	return EstimatedSeconds / MaxSeconds;
}

float URTPlaybackLibrary::EffectivePlaybackSpeed(float ViewerSpeed, float CapSpeed)
{
	// Non positivo = "non scelto" / "nessun limite": entrambi valgono 1, cosi' la composizione resta
	// definita anche su un campo azzerato (variabile Blueprint, Memzero) invece di fermare il playback.
	const float Viewer = (ViewerSpeed > 0.f) ? ViewerSpeed : 1.f;
	const float Cap = (CapSpeed > 0.f) ? CapSpeed : 1.f;
	return FMath::Max(Viewer, Cap);
}

float URTPlaybackLibrary::DirectionYaw(const FVector& From, const FVector& To)
{
	const FVector Dir = To - From;
	if (Dir.SizeSquared2D() <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f; // nessuna direzione planare -> nessun orientamento
	}
	// Atan2(Y,X): +X=0, +Y=90, -X=+/-180, -Y=-90 (convenzione yaw UE). Z ignorata (facing planare).
	return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
}
