#include "Combat/RTCombatLibrary.h"

FRTDamageResult URTCombatLibrary::ApplyDamage(int32 Damage, int32 Shield, int32 Health)
{
	const int32 SafeDamage = FMath::Max(0, Damage);
	const int32 SafeShield = FMath::Max(0, Shield);

	const int32 AbsorbedByShield = FMath::Min(SafeDamage, SafeShield);
	const int32 NewShield = SafeShield - AbsorbedByShield;
	const int32 DamageToHealth = SafeDamage - AbsorbedByShield;
	const int32 NewHealth = FMath::Max(0, Health - DamageToHealth);

	return FRTDamageResult(NewHealth, NewShield);
}

int32 URTCombatLibrary::GainEnergy(int32 Current, int32 Gain, int32 Max)
{
	return FMath::Clamp(Current + Gain, 0, Max);
}

bool URTCombatLibrary::IsUltimateReady(int32 Energy, int32 Max)
{
	return Max > 0 && Energy >= Max;
}

int32 URTCombatLibrary::EffectiveMoveRange(int32 BaseRange, bool bRooted, bool bSlowed)
{
	if (bRooted)
	{
		return 0;
	}
	return bSlowed ? BaseRange / 2 : BaseRange;
}

bool URTCombatLibrary::IsAbilityUsable(int32 CooldownRemaining, int32 Energy, int32 EnergyCost)
{
	return CooldownRemaining <= 0 && Energy >= EnergyCost;
}

bool URTCombatLibrary::IsIntentVisibleTo(int32 ObserverTeamId, int32 OwnerTeamId, bool bOwnerRevealed)
{
	// Alleati: sempre. Avversari: solo se il proprietario e' rivelato.
	return ObserverTeamId == OwnerTeamId || bOwnerRevealed;
}

int32 URTCombatLibrary::EffectiveAttackPower(int32 BasePower, int32 OccupantDamageBonus)
{
	return FMath::Max(0, BasePower + OccupantDamageBonus);
}

FRTGridCoord URTCombatLibrary::KnockbackDestination(const FRTGridCoord& Attacker, const FRTGridCoord& Target,
	int32 Distance, const TArray<FRTGridCoord>& Blocked, int32 Width, int32 Height)
{
	const int32 DX = Target.X - Attacker.X;
	const int32 DY = Target.Y - Attacker.Y;
	if (Distance <= 0 || (DX == 0 && DY == 0))
	{
		return Target; // nessuna spinta / nessuna direzione
	}

	// Direzione cardinale di allontanamento: l'asse col delta maggiore (a parità, l'asse X).
	int32 StepX = 0, StepY = 0;
	if (FMath::Abs(DX) >= FMath::Abs(DY)) { StepX = (DX > 0) ? 1 : -1; }
	else { StepY = (DY > 0) ? 1 : -1; }

	FRTGridCoord Current = Target;
	for (int32 i = 0; i < Distance; ++i)
	{
		const FRTGridCoord Next(Current.X + StepX, Current.Y + StepY, Target.Layer); // stesso layer del bersaglio
		if (Next.X < 0 || Next.X >= Width || Next.Y < 0 || Next.Y >= Height)
		{
			break; // bordo della griglia
		}
		if (Blocked.Contains(Next))
		{
			break; // ostacolo/occupato: si ferma sulla cella libera precedente
		}
		Current = Next;
	}
	return Current;
}

TArray<int32> URTCombatLibrary::NewlyDefeated(const TArray<int32>& HealthBefore, const TArray<int32>& HealthAfter)
{
	TArray<int32> Result;
	const int32 Count = FMath::Min(HealthBefore.Num(), HealthAfter.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		if (HealthBefore[i] > 0 && HealthAfter[i] <= 0)
		{
			Result.Add(i); // viva prima, morta ora
		}
	}
	return Result;
}
