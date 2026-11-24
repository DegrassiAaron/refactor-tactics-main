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
	//
	// 🔑 **Dal #4189 la somma ha due parti** ([D-634] (5)): lo scudo base ferma solo il `Direct`, e un colpo elementale lo
	// attraversa. `DamageByTarget` resta il totale (lo stadio `TargetSum`), `ElementalByTarget` ne e' la parte con
	// elemento; le mappe si leggono per chiave, l'ordine resta `Order`.
	TArray<int32> Order;
	TMap<int32, int32> DamageByTarget;
	TMap<int32, int32> ElementalByTarget;
	for (const FRTAttack& Attack : Attacks)
	{
		if (Units.IsValidIndex(Attack.TargetIndex))
		{
			if (!DamageByTarget.Contains(Attack.TargetIndex))
			{
				Order.Add(Attack.TargetIndex);
			}
			DamageByTarget.FindOrAdd(Attack.TargetIndex) += Attack.Power;
			if (!Attack.ElementId.IsNone())
			{
				ElementalByTarget.FindOrAdd(Attack.TargetIndex) += Attack.Power;
			}
			OutByTarget.FindOrAdd(Attack.TargetIndex).Stages.Append(Attack.Breakdown);
		}
	}

	TArray<FRTUnitCombatState> Result = Units;
	for (const int32 TargetIndex : Order)
	{
		const int32 Total = DamageByTarget[TargetIndex];
		const int32 Elemental = ElementalByTarget.FindRef(TargetIndex);
		const FRTUnitCombatState& Initial = Units[TargetIndex];
		// Le due parti in una chiamata sola, sulla somma per bersaglio: il temporaneo consuma prima l'elementale, la base
		// assorbe solo cio' che resta del `Direct` (`ApplyDamageParts`). Senza colpi elementali la parte elementale e' 0, e
		// l'esito e' quello di `ApplyDamage(Total, Direct)` di prima del #4189. Il danno ambientale non passa di qui — chiama
		// `ApplyDamage` direttamente dal TurnManager.
		// ⏱️ *Fino al #4189 qui c'era `ApplyDamage(Total, ERTDamageSource::Direct, ...)`, con `Direct` come costante: un campo
		// su `FRTAttack` che avrebbe avuto sempre lo stesso valore sarebbe stato speculazione. Con l'elemento non lo e' piu'.*
		const FRTDamageResult Damaged = URTCombatLibrary::ApplyDamageParts(Total - Elemental, Elemental,
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
		// intenzioni e diventa la stessa cosa che il registro della partita dichiara. `Operand` e' cio' che temporaneo e base
		// hanno assorbito insieme sulle due parti: per un colpo elementale da solo la base non vi contribuisce (#4189).
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

TArray<FRTAttack> URTCombatResolver::ApplyEligibleHitDelta(const TArray<FRTAttack>& Attacks,
	const TArray<int32>& DeltaByTarget, const TArray<bool>& bEligible, FName SourceId)
{
	TArray<FRTAttack> Result = Attacks;

	for (int32 i = 0; i < Result.Num(); ++i)
	{
		FRTAttack& Attack = Result[i];
		if (!DeltaByTarget.IsValidIndex(Attack.TargetIndex)) { continue; }

		// Fuori dalla maschera = fuori dall'arco frontale ([D-206]): il colpo passa intero. Una maschera
		// piu' corta dell'array vale «non eleggibile», non «eleggibile per default» — stessa disciplina di
		// `ApplyAbsorptionPool`, e per la stessa ragione.
		if (!bEligible.IsValidIndex(i) || !bEligible[i]) { continue; }

		const int32 Delta = DeltaByTarget[Attack.TargetIndex];
		if (Delta == 0) { continue; }

		const int32 Before = Attack.Power;
		Attack.Power = FMath::Max(0, Attack.Power + Delta);

		// 🔴 **Uno stadio che non ha cambiato niente NON compare** (`#1951`), ed e' la regola che
		// `ApplyAbsorptionPool` applica due funzioni piu' giu' con `if (Absorbed > 0)`.
		//
		// ⚠️ **Il caso e' RAGGIUNGIBILE in produzione, non teorico**: [D-312] fa passare il `Deflect`
		// PRIMA della Guardia, quindi un colpo frontale gia' azzerato dal pool della reazione arriva qui
		// con `Power == 0` — e senza questa riga il registro porterebbe una voce che dichiara una
		// riduzione di 15 su un colpo da cui non ha tolto niente. Un colpo puo' valere 0 anche senza
		// `Deflect`: `Hit.Power` e' la potenza d'intento meno la copertura.
		//
		// ⏱️ **E' una regressione che [D-408] avrebbe introdotto in silenzio**: finche' la Guardia passava
		// da `ApplyAbsorptionPool` la guardia c'era, e migrandola su un delta l'avrebbe persa. Trovata da
		// una code review. ⛔ `ApplyDamageDelta` (il `Brace`) NON ha ancora questa riga: e' un difetto
		// gemello e preesistente, nominato e non corretto qui.
		if (Attack.Power == Before) { continue; }

		// ⚠️ Lo stadio e' `EveryHitDelta` e non uno nuovo, e la provenienza la distingue il `SourceId`: e'
		// la stessa disciplina con cui `#2213` ha separato i due pool senza coniare un secondo stadio. Un
		// valore nuovo nell'enum toccherebbe il formato che un replay rilegge, e qui non serve.
		//
		// ⚠️ `Operand` e' il delta DICHIARATO, non quanto ne e' stato applicato: e' la convenzione della
		// famiglia dei delta (`ApplyDamageDelta`, `ApplyFirstHitDelta`). Quanto abbia morso davvero lo
		// dicono `Before` e `After`, e `SubtractClamped` dichiara che il clamp e' intervenuto.
		Attack.Breakdown.Emplace(ERTDamageStage::EveryHitDelta, SourceId,
			Delta >= 0 ? ERTDamageOp::Add : ERTDamageOp::SubtractClamped,
			FMath::Abs(Delta), Before, Attack.Power);
	}

	return Result;
}

TArray<FRTAttack> URTCombatResolver::ApplyAbsorptionPool(const TArray<FRTAttack>& Attacks,
	const TArray<int32>& PoolByTarget, const TArray<bool>& bEligible, FName SourceId)
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
			//
			// 🔑 La provenienza arriva dal CHIAMANTE e non e' piu' un letterale qui: questa funzione non sa quale pool la stia
			// usando (`#2213`; erano due, `Deflect` e `Guard`, fino a [D-408]).
			Attack.Breakdown.Emplace(ERTDamageStage::AbsorptionPool,
				SourceId, ERTDamageOp::Pool,
				Absorbed, Attack.Power, Attack.Power - Absorbed);
		}
		Attack.Power -= Absorbed;
		Budget -= Absorbed;
	}

	return Result;
}
