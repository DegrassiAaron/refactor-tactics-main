#include "Ability/RTWorkbenchVariant.h"

#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTActionReadout.h"

namespace
{
	/** L'azione del kit con quell'`ActionId`, o `nullptr`. Il kit e' piccolo: nessun indice da costruire. */
	URTActionData* TrovaAzione(const TArray<URTActionData*>& Abilities, const FName& ActionId)
	{
		for (URTActionData* A : Abilities)
		{
			if (A != nullptr && A->Def.ActionId == ActionId) { return A; }
		}
		return nullptr;
	}

	/**
	 * L'indice dell'effetto `Damage` da toccare, o `INDEX_NONE` se la richiesta non e' soddisfacibile.
	 * `EffectIndex == INDEX_NONE` in ingresso significa «il primo», che e' anche l'unico che lo specchio
	 * proietta.
	 */
	int32 IndiceDannoRichiesto(const FRTActionDef& Def, int32 EffectIndex)
	{
		if (EffectIndex == INDEX_NONE)
		{
			for (int32 i = 0; i < Def.Effects.Num(); ++i)
			{
				if (Def.Effects[i].Effect == ERTActionEffect::Damage) { return i; }
			}
			return INDEX_NONE;
		}

		if (!Def.Effects.IsValidIndex(EffectIndex)) { return INDEX_NONE; }
		return Def.Effects[EffectIndex].Effect == ERTActionEffect::Damage ? EffectIndex : INDEX_NONE;
	}

	/** L'indice del PRIMO effetto `Damage`: e' quello, e solo quello, che `URTActionData::Power` proietta. */
	int32 IndicePrimoDanno(const FRTActionDef& Def)
	{
		for (int32 i = 0; i < Def.Effects.Num(); ++i)
		{
			if (Def.Effects[i].Effect == ERTActionEffect::Damage) { return i; }
		}
		return INDEX_NONE;
	}
}

ERTVariantApplyResult URTWorkbenchVariantLibrary::Validate(const FRTWorkbenchVariant& Variant,
	const TArray<URTActionData*>& Abilities)
{
	if (Variant.VariantId.IsNone())
	{
		return ERTVariantApplyResult::MissingVariantId;
	}

	for (const FRTAbilityParameterOverride& Ov : Variant.Overrides)
	{
		const URTActionData* Azione = TrovaAzione(Abilities, Ov.ActionId);
		if (Azione == nullptr)
		{
			return ERTVariantApplyResult::UnknownAction;
		}

		if (Ov.ParameterKey == RTActionParameterKeys::RangeCells()
			|| Ov.ParameterKey == RTActionParameterKeys::CooldownTurns())
		{
			continue;
		}

		if (Ov.ParameterKey == RTActionParameterKeys::Damage())
		{
			if (IndiceDannoRichiesto(Azione->Def, Ov.EffectIndex) == INDEX_NONE)
			{
				return ERTVariantApplyResult::InvalidEffectIndex;
			}
			continue;
		}

		return ERTVariantApplyResult::UnknownParameter;
	}

	return ERTVariantApplyResult::Ok;
}

ERTVariantApplyResult URTWorkbenchVariantLibrary::Apply(const FRTWorkbenchVariant& Variant,
	const TArray<URTActionData*>& Abilities, FRTWorkbenchVariant& OutRestore)
{
	OutRestore = FRTWorkbenchVariant();

	// ⚠️ Valida TUTTO prima di scrivere il primo numero. Un fail a meta' lascerebbe una variante che il
	// designer non ha configurato, e il risultato verrebbe attribuito all'esperimento sbagliato.
	const ERTVariantApplyResult Esito = Validate(Variant, Abilities);
	if (Esito != ERTVariantApplyResult::Ok)
	{
		return Esito;
	}

	OutRestore.VariantId = FName(*(Variant.VariantId.ToString() + TEXT(".Restore")));

	for (const FRTAbilityParameterOverride& Ov : Variant.Overrides)
	{
		URTActionData* Azione = TrovaAzione(Abilities, Ov.ActionId);
		// `Validate` ha gia' escluso il nullo; la guardia resta perche' un ritorno anticipato qui sarebbe
		// una scrittura parziale, cioe' proprio cio' che la validazione in due tempi esiste per impedire.
		if (Azione == nullptr) { continue; }

		FRTAbilityParameterOverride Inverso = Ov;

		if (Ov.ParameterKey == RTActionParameterKeys::RangeCells())
		{
			Inverso.Value = Azione->Def.RangeCells;
			// ENTRAMBE le case: il `Def` per chi applica il ternario, lo specchio per il bot.
			Azione->Def.RangeCells = Ov.Value;
			Azione->RangeCells = Ov.Value;
		}
		else if (Ov.ParameterKey == RTActionParameterKeys::CooldownTurns())
		{
			Inverso.Value = Azione->Def.CooldownTurns;
			// Lo specchio qui non e' un di piu': `ARTUnit::ConsumeAbility` legge SOLO quello.
			Azione->Def.CooldownTurns = Ov.Value;
			Azione->CooldownTurns = Ov.Value;
		}
		else // Damage
		{
			const int32 Indice = IndiceDannoRichiesto(Azione->Def, Ov.EffectIndex);
			if (Indice == INDEX_NONE) { continue; } // gia' escluso da `Validate`

			Inverso.Value = Azione->Def.Effects[Indice].Amount;
			Inverso.EffectIndex = Indice;
			Azione->Def.Effects[Indice].Amount = Ov.Value;

			// Lo specchio porta SOLO il primo `Damage`: sovrascrivere il secondo non lo tocca, ed e'
			// corretto — `Power` non lo rappresenta affatto. Scriverlo comunque farebbe dire allo specchio
			// un numero che appartiene a un altro colpo.
			if (Indice == IndicePrimoDanno(Azione->Def))
			{
				Azione->Power = Ov.Value;
			}
		}

		OutRestore.Overrides.Add(Inverso);
	}

	return ERTVariantApplyResult::Ok;
}
