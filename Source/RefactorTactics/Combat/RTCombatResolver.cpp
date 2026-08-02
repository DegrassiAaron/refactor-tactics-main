#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"

TArray<FRTUnitCombatState> URTCombatResolver::ResolveAttacks(const TArray<FRTUnitCombatState>& Units, const TArray<FRTAttack>& Attacks)
{
	// Somma i danni per bersaglio (raccogli), poi applica sullo stato iniziale (applica).
	// L'ordine degli attacchi non influisce sul risultato.
	TMap<int32, int32> DamageByTarget;
	for (const FRTAttack& Attack : Attacks)
	{
		if (Units.IsValidIndex(Attack.TargetIndex))
		{
			DamageByTarget.FindOrAdd(Attack.TargetIndex) += Attack.Power;
		}
	}

	TArray<FRTUnitCombatState> Result = Units;
	for (const TPair<int32, int32>& Pair : DamageByTarget)
	{
		const FRTUnitCombatState& Initial = Units[Pair.Key];
		const FRTDamageResult Damaged = URTCombatLibrary::ApplyDamage(Pair.Value, Initial.Shield, Initial.Health);
		Result[Pair.Key] = FRTUnitCombatState(Damaged.Health, Damaged.Shield);
	}
	return Result;
}
