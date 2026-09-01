#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"

TArray<FRTUnitCombatState> URTCombatResolver::ResolveAttacks(const TArray<FRTUnitCombatState>& Units, const TArray<FRTAttack>& Attacks)
{
	TMap<int32, FRTDamageBreakdown> Ignored;
	return ResolveAttacksWithBreakdown(Units, Attacks, Ignored);
}

TArray<FRTUnitCombatState> URTCombatResolver::ResolveAttacksWithBreakdown(
	const TArray<FRTUnitCombatState>& Units, const TArray<FRTAttack>& Attacks,
	TMap<int32, FRTDamageBreakdown>& OutByTarget)
{
	OutByTarget.Reset();

	// Somma i danni per bersaglio (raccogli), poi applica sullo stato iniziale (applica).
	// L'ordine degli attacchi non influisce sul risultato.
	//
	// 🔴 **L'ordine dei BERSAGLI e' quello di prima apparizione, e non quello della `TMap`.** Per gli
	// stati e' indifferente — ognuno si risolve dal proprio stato iniziale — ma il registro degli stadi
	// raccolto nell'ordine di una mappa cambierebbe fra due esecuzioni, e un elenco che cambia ordine non
	// e' verificabile (`#1951`).
	TArray<int32> Order;
	TMap<int32, int32> DamageByTarget;
	for (const FRTAttack& Attack : Attacks)
	{
		if (Units.IsValidIndex(Attack.TargetIndex))
		{
			if (!DamageByTarget.Contains(Attack.TargetIndex))
			{
				Order.Add(Attack.TargetIndex);
			}
			DamageByTarget.FindOrAdd(Attack.TargetIndex) += Attack.Power;
			OutByTarget.FindOrAdd(Attack.TargetIndex).Stages.Append(Attack.Breakdown);
		}
	}

	TArray<FRTUnitCombatState> Result = Units;
	for (const int32 TargetIndex : Order)
	{
		const int32 Total = DamageByTarget[TargetIndex];
		const FRTUnitCombatState& Initial = Units[TargetIndex];
		// `Direct` come costante e non come campo di `FRTAttack`: ogni colpo che attraversa questo resolver
		// viene dal Blast, e un campo che avrebbe sempre lo stesso valore sarebbe speculazione. Il danno
		// ambientale non passa di qui — chiama `ApplyDamage` direttamente dal TurnManager.
		const FRTDamageResult Damaged = URTCombatLibrary::ApplyDamage(Total, ERTDamageSource::Direct,
			Initial.Shield, Initial.TemporaryShield, Initial.Health);
		Result[TargetIndex] = FRTUnitCombatState(Damaged.Health, Damaged.Shield, Damaged.TemporaryShield);

		FRTDamageBreakdown& Breakdown = OutByTarget.FindOrAdd(TargetIndex);

		// STADIO 8 — la somma per bersaglio. Compare **solo con piu' di un colpo**: con uno solo la somma
		// non trasforma niente, e uno stadio che non si applica non deve comparire con operando zero.
		int32 Hits = 0;
		for (const FRTAttack& A : Attacks)
		{
			if (A.TargetIndex == TargetIndex) { ++Hits; }
		}
		if (Hits > 1)
		{
			Breakdown.Stages.Emplace(ERTDamageStage::TargetSum, FName(TEXT("invariante #3")),
				ERTDamageOp::Sum, Hits, Total, Total);
		}

		// STADIO 9 — temporaneo → base → HP. `After` e' cio' che gli HP hanno DAVVERO perso, cioe'
		// l'`Amount` che il TurnLog registra: e' il punto in cui il breakdown smette di essere una somma di
		// intenzioni e diventa la stessa cosa che il registro della partita dichiara.
		const int32 HpLost = Initial.Health - Damaged.Health;
		Breakdown.Stages.Emplace(ERTDamageStage::ShieldAbsorption, FName(TEXT("D-224 · scudo base")),
			ERTDamageOp::TwoLayerAbsorb, Total - HpLost, Total, HpLost);
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

		const int32 Before = Attack.Power;
		Attack.Power = FMath::Max(0, Attack.Power + Delta);
		// ⚠️ Il registro si scrive DOPO il clamp e porta `Before`/`After` reali: con un delta negativo
		// piu' grande del colpo la riduzione che avanza si PERDE, e `After - Before` non e' `Delta`. E'
		// esattamente cio' che il breakdown deve mostrare invece di far dedurre (`D-292`).
		Attack.Breakdown.Emplace(ERTDamageStage::FirstHitDelta,
			FName(TEXT("D-292 · primo colpo")),
			Delta >= 0 ? ERTDamageOp::Add : ERTDamageOp::SubtractClamped,
			FMath::Abs(Delta), Before, Attack.Power);
		AlreadyApplied.Add(Attack.TargetIndex);
	}

	return Result;
}

TArray<FRTAttack> URTCombatResolver::ApplyDamageDelta(const TArray<FRTAttack>& Attacks,
	const TArray<int32>& DeltaByTarget)
{
	TArray<FRTAttack> Result = Attacks;

	// Nessun set "gia' applicato": il delta vale per OGNI colpo di quel bersaglio. E' l'unica differenza da
	// ApplyFirstHitDelta, ed e' la ragione per cui esistono due funzioni invece di un flag.
	for (FRTAttack& Attack : Result)
	{
		if (!DeltaByTarget.IsValidIndex(Attack.TargetIndex)) { continue; }

		const int32 Delta = DeltaByTarget[Attack.TargetIndex];
		if (Delta == 0) { continue; }

		const int32 Before = Attack.Power;
		Attack.Power = FMath::Max(0, Attack.Power + Delta);
		// Stesso clamp e stessa ragione di `ApplyFirstHitDelta`, senza il gate «una volta sola» (CP 5.2).
		Attack.Breakdown.Emplace(ERTDamageStage::EveryHitDelta,
			FName(TEXT("CP 5.2 · ogni colpo")),
			Delta >= 0 ? ERTDamageOp::Add : ERTDamageOp::SubtractClamped,
			FMath::Abs(Delta), Before, Attack.Power);
	}

	return Result;
}

TArray<FRTAttack> URTCombatResolver::ApplyAbsorptionPool(const TArray<FRTAttack>& Attacks,
	const TArray<int32>& PoolByTarget, const TArray<bool>& bEligible)
{
	TArray<FRTAttack> Result = Attacks;

	// Copia locale del budget: si consuma mentre si scorre, e l'ingresso resta const. Un `TArray` e non una
	// `TMap`, per la stessa ragione di `ApplyFirstHitDelta`: l'ordine di iterazione non deve decidere nulla.
	TArray<int32> Remaining = PoolByTarget;

	for (int32 i = 0; i < Result.Num(); ++i)
	{
		FRTAttack& Attack = Result[i];
		if (!Remaining.IsValidIndex(Attack.TargetIndex)) { continue; }

		// Fuori dalla maschera = fuori dall'arco frontale (D-206): il colpo passa intero e NON tocca il pool.
		// Un `bEligible` piu' corto dell'array vale «non eleggibile», non «eleggibile per default»: un dato
		// mancante non deve concedere una protezione che nessuno ha dichiarato.
		if (!bEligible.IsValidIndex(i) || !bEligible[i]) { continue; }

		int32& Budget = Remaining[Attack.TargetIndex];
		if (Budget <= 0) { continue; }

		// `Min` e non una sottrazione con clamp: cio' che il colpo non consuma resta nel budget per i colpi
		// successivi. E' l'intera differenza con `ApplyFirstHitDelta`, dove l'avanzo si perdeva.
		const int32 Absorbed = FMath::Min(Budget, Attack.Power);
		if (Absorbed > 0)
		{
			// ⚠️ Solo se ha morso: uno stadio che non si applica NON compare, perche' un elenco che
			// contiene tutto non spiega niente (`#1951`).
			Attack.Breakdown.Emplace(ERTDamageStage::AbsorptionPool,
				FName(TEXT("D-292 · Status.Guarded")), ERTDamageOp::Pool,
				Absorbed, Attack.Power, Attack.Power - Absorbed);
		}
		Attack.Power -= Absorbed;
		Budget -= Absorbed;
	}

	return Result;
}
