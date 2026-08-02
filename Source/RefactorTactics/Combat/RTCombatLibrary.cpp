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
