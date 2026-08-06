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

TArray<FRTAttack> URTCombatResolver::ApplyFirstHitDelta(const TArray<FRTAttack>& Attacks,
	const TArray<int32>& DeltaByTarget)
{
	TArray<FRTAttack> Result = Attacks;

	// Un bersaglio riceve il delta una volta sola: si itera l'array dei colpi (ordine gia' deterministico) e
	// si segna chi l'ha gia' avuto. Nessuna TMap in mezzo: l'ordine di iterazione non deve mai decidere nulla.
	TSet<int32> AlreadyApplied;
	for (FRTAttack& Attack : Result)
	{
		if (!DeltaByTarget.IsValidIndex(Attack.TargetIndex)) { continue; }

		const int32 Delta = DeltaByTarget[Attack.TargetIndex];
		if (Delta == 0 || AlreadyApplied.Contains(Attack.TargetIndex)) { continue; }

		Attack.Power = FMath::Max(0, Attack.Power + Delta);
		AlreadyApplied.Add(Attack.TargetIndex);
	}

	return Result;
}
