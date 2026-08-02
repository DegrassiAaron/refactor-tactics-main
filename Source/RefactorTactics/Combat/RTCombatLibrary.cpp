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
