#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"

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

ERTCombatOutcome URTCombatLibrary::ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus)
{
	if (HealthBefore > 0 && HealthAfter <= 0) { return ERTCombatOutcome::Lethal; }
	if (HealthAfter == HealthBefore)          { return ERTCombatOutcome::ShieldAbsorbed; }
	if (AttackerDmgBonus > 0)                 { return ERTCombatOutcome::TerrainBonus; }
	return ERTCombatOutcome::Hit;
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

bool URTCombatLibrary::CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId)
{
	// Nessuna eccezione: si comanda solo la propria squadra (niente "possessione" di unita' avversarie).
	return UnitTeamId == PlayerTeamId;
}

ERTHexTargetReason URTCombatLibrary::ClassifyHexTargeting(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, int32 RangeCells)
{
	if (!Map)
	{
		return ERTHexTargetReason::NoMap; // FAIL-CLOSED: senza mappa la linea di tiro non e' verificabile
	}
	if (URTHexLibrary::HexDistance(From, To) > FMath::Max(0, RangeCells))
	{
		return ERTHexTargetReason::OutOfRange; // la portata si valuta PRIMA: e' un difetto diverso da "coperto"
	}
	return URTHexVisionLibrary::HasLineOfSight(Map, From, To)
		? ERTHexTargetReason::Ok : ERTHexTargetReason::NoLineOfSight;
}

bool URTCombatLibrary::CanTargetHexCell(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
	int32 RangeCells)
{
	return ClassifyHexTargeting(Map, From, To, RangeCells) == ERTHexTargetReason::Ok;
}

int32 URTCombatLibrary::EffectiveAttackPower(int32 BasePower, int32 OccupantDamageBonus)
{
	return FMath::Max(0, BasePower + OccupantDamageBonus);
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
