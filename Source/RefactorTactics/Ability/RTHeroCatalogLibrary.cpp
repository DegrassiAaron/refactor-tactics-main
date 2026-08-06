#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"

namespace
{
	/**
	 * Un'azione d'eroe completa: popola SIA `Def` (modello E4, letto dai validator e dai test) SIA i campi
	 * legacy specchiati (`RangeCells`/`Power`/`Shape`/`AreaRadius`/`CooldownTurns`) che `ARTTurnManager` legge
	 * ancora oggi in partita (vedi il ponte a `RTTurnManager.cpp` riga ~793: si attiva SOLO se `Def.ActionId`
	 * e' vuoto, quindi per un'azione catalogata come questa il campo legacy dev'essere gia' coerente da solo).
	 * Stessa disciplina di `URTCatalogLibrary::ShippedAction`, estesa ai campi che l'oggetto porta in piu'.
	 */
	URTActionData* MakeHeroAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority, int32 Range,
		int32 Cooldown, ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects,
		ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0,
		ERTActionSlot Slot = ERTActionSlot::Main, bool bInterruptible = true)
	{
		URTActionData* Action = NewObject<URTActionData>();

		Action->Def.ActionId = Id;
		Action->Def.ResolutionPhase = Phase;
		Action->Def.Priority = Priority;
		Action->Def.RangeCells = Range;
		Action->Def.CooldownTurns = Cooldown;
		Action->Def.Fallback = Fallback;
		Action->Def.Effects = Effects;
		Action->Def.Slot = Slot;
		Action->Def.bCanBeInterrupted = bInterruptible;

		Action->RangeCells = Range;
		Action->CooldownTurns = Cooldown;
		Action->Shape = Shape;
		Action->AreaRadius = AreaRadius;
		Action->Power = 0;
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Action->Power = Spec.Amount; break; }
		}

		return Action;
	}
}

TArray<FString> URTHeroCatalogLibrary::ValidateHeroes(const TArray<const URTHeroData*>& Heroes)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	// Numero di azioni atteso: 1 attacco base + 4 abilita' fondamentali (catalogo v0.1 §"Struttura di un eroe").
	constexpr int32 ExpectedActionCount = 5;

	for (int32 i = 0; i < Heroes.Num(); ++i)
	{
		const URTHeroData* Hero = Heroes[i];
		if (Hero == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("eroe #%d: riferimento nullo"), i));
			continue;
		}

		const FString Where = Hero->HeroId.IsNone()
			? FString::Printf(TEXT("eroe #%d"), i)
			: Hero->HeroId.ToString();

		if (Hero->HeroId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId mancante (l'ID e' la chiave stabile dell'asset)"), *Where));
		}
		else if (Seen.Contains(Hero->HeroId))
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Hero->HeroId);
		}

		if (Hero->MaxHealth <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: salute non positiva (%d)"), *Where, Hero->MaxHealth));
		}
		if (Hero->MovePoints <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: movimento non positivo (%d)"), *Where, Hero->MovePoints));
		}
		if (Hero->VisionRange < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: range visivo negativo (%d)"), *Where, Hero->VisionRange));
		}
		if (Hero->PushResistance < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: resistenza push negativa (%d)"), *Where, Hero->PushResistance));
		}
		if (Hero->Affinity.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: affinita' non dichiarata"), *Where));
		}
		// La debolezza non si inventa (catalogo v0.1 §5): un eroe senza debolezza dichiarata resta
		// strutturalmente incompleto, e il validator lo dice prima che arrivi in partita.
		if (Hero->Weakness.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: debolezza non dichiarata"), *Where));
		}

		if (Hero->Actions.Num() != ExpectedActionCount)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: attese %d azioni (attacco base + quattro fondamentali), trovate %d"),
				*Where, ExpectedActionCount, Hero->Actions.Num()));
			continue; // struttura gia' rotta: contare le varianti su un array della forma sbagliata non aiuta
		}

		// Indice 0 = attacco base, escluso dal conteggio delle varianti: la variante e' di un'abilita'
		// FONDAMENTALE (catalogo v0.1 §6), mai dell'attacco base (quello lo modificano le varianti d'arma,
		// equipaggiamento E7).
		int32 VariantCount = 0;
		for (int32 a = 1; a < Hero->Actions.Num(); ++a)
		{
			if (Hero->Actions[a] && Hero->Actions[a]->Variants.Num() > 0)
			{
				++VariantCount;
			}
		}
		if (VariantCount != 1)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: %d abilita' fondamentali con variante (attesa esattamente 1)"), *Where, VariantCount));
		}
	}

	return Errors;
}

URTHeroData* URTHeroCatalogLibrary::MakeFlux()
{
	URTHeroData* Flux = NewObject<URTHeroData>();
	Flux->HeroId = TEXT("Hero.Flux");
	Flux->DisplayName = FText::FromString(TEXT("Flux"));
	Flux->MaxHealth = 90;
	Flux->MovePoints = 5;
	Flux->VisionRange = 6;
	Flux->PushResistance = 0;
	Flux->Affinity = TEXT("Affinity.Electricity");
	// Debolezza acqua: stesso identificatore che Riva (CP 6.3) usera' come sua affinita', cosi' la combo
	// "Flux su bersaglio Wet" e "l'affinita' di Riva e' l'acqua" restano lo stesso concetto, non due nomi.
	Flux->Weakness = TEXT("Affinity.Water");

	// Indice 0 — ArcPulse, attacco base. 22 danni / range 4 e' ESATTAMENTE la fascia "medio raggio" del
	// catalogo azioni v0.1 §1: non e' una coincidenza da verificare a mano, e' la stessa tabella.
	const FRTActionDef ArcPulseDef = URTCatalogLibrary::MakeBasicAttack(4);
	Flux->Actions.Add(MakeHeroAction(TEXT("Flux.ArcPulse"), ArcPulseDef.ResolutionPhase, ArcPulseDef.Priority,
		ArcPulseDef.RangeCells, ArcPulseDef.CooldownTurns, ArcPulseDef.Fallback, ArcPulseDef.Effects));

	// Indice 1 — LinearDischarge. 24 danni in linea, range 5 (stessa portata di `Action.LineAttack`: nessuna
	// azione lineare del catalogo ne dichiara una diversa). Il bonus "+8 su bersaglio Wet" NON e' nella lista
	// Effects: e' condizionale al bersaglio, non un danno fisso, e passa da `EffectiveAttackPower` +
	// `URTCombatLibrary::FluxWetDischargeBonus` (vedi `Heroes.Flux.WetBonus`) — un chiamante che applica
	// SOLO Effects vede 24, corretto finche' non controlla anche lo status del bersaglio.
	Flux->Actions.Add(MakeHeroAction(TEXT("Flux.LinearDischarge"), ERTResolutionPhase::Attack, /*Priority*/ 55,
		/*Range*/ 5, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 24) }, ERTAbilityShape::Line));

	// Indice 2 — ConductiveNode. "Rende conduttiva una cella per 2 turni": nessun modello di conduttivita' di
	// cella esiste (ne' in FRTHexCellData ne' in FRTActionEffectSpec, che applica stati solo alle UNITA').
	// Effects vuoto e' la dichiarazione onesta: l'identita', la fase e il cooldown sono dati veri, l'effetto
	// no. Range 0 (self) e' un segnaposto, non un numero di bilanciamento: arriva un range reale insieme
	// all'effetto, non prima.
	Flux->Actions.Add(MakeHeroAction(TEXT("Flux.ConductiveNode"), ERTResolutionPhase::Preparation, /*Priority*/ 35,
		/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// Indice 3 — Overload. AoE 18 danni, raggio 1 (riuso il raggio di `Action.CircularAoE`, non un numero
	// nuovo), portata 3. "Interrupt sui dispositivi" non e' rappresentabile: non esistono dispositivi/gadget
	// (E7). Solo il danno e' un effetto dichiarato.
	Flux->Actions.Add(MakeHeroAction(TEXT("Flux.Overload"), ERTResolutionPhase::Attack, /*Priority*/ 65,
		/*Range*/ 3, /*Cooldown*/ 3, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 18) }, ERTAbilityShape::Area, /*AreaRadius*/ 1));
	// La variante (vincolo v0.1: una sola abilita' fondamentale per eroe) sta su LinearDischarge, non qui:
	// vedi sotto.

	// Indice 4 — ReactiveCapacitor. Scudo 15 (rappresentabile: e' un effetto Shield) + "10 danni
	// all'attaccante" (NON rappresentabile: una reazione non ha ancora uno slot dedicato ne' un modo di
	// riferire "chi ha appena colpito" — quel bersaglio si conosce solo al trigger, l'istanza pianificata non
	// ce l'ha. Arriva con E5). Fase Preparation: il commento di ERTResolutionPhase la elenca esplicitamente
	// fra "scudi, stance, trappole, REAZIONI PREPARATE". Slot None: non esiste ancora uno slot Reazione, e
	// forzarla su Main le farebbe consumare l'azione principale che non le compete.
	Flux->Actions.Add(MakeHeroAction(TEXT("Flux.ReactiveCapacitor"), ERTResolutionPhase::Preparation,
		/*Priority*/ 35, /*Range*/ 0, /*Cooldown*/ 3, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 15) }, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		ERTActionSlot::None, /*bInterruptible*/ false));

	// Variante di LinearDischarge (catalogo v0.1 §6, vincolo: una sola abilita' fondamentale con variante).
	// Concentrata: +6 danni (30 totali), un solo bersaglio — "non si propaga" e' vero per costruzione, dato
	// che LinearDischarge base non propaga (nessun sistema di propagazione elettrica esiste, E8).
	FRTAbilityVariant Concentrated;
	Concentrated.VariantId = TEXT("Flux.LinearDischarge.Concentrated");
	Concentrated.DisplayName = FText::FromString(TEXT("Scarica concentrata"));
	Concentrated.Tradeoff = FText::FromString(TEXT("+6 danni (30 totali), ma non si propaga a un secondo bersaglio"));
	Concentrated.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 30));

	// Ramificata: -6 danni per bersaglio (18 ciascuno) MA un bersaglio aggiuntivo. Due eventi di danno
	// separati rappresentano i due colpi: QUALE secondo nemico venga colpito e' targeting non ancora cablato
	// (`ProduceEvents` legge oggi un solo `TargetUnitId` per istanza) — il numero c'e', il "come" arriva
	// quando la geometria multi-bersaglio delle azioni lineari lo richiedera' davvero.
	FRTAbilityVariant Branched;
	Branched.VariantId = TEXT("Flux.LinearDischarge.Branched");
	Branched.DisplayName = FText::FromString(TEXT("Scarica ramificata"));
	Branched.Tradeoff = FText::FromString(TEXT("un bersaglio aggiuntivo, ma -6 danni per bersaglio (18 ciascuno)"));
	Branched.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));
	Branched.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));

	Flux->Actions[1]->Variants.Add(Concentrated);
	Flux->Actions[1]->Variants.Add(Branched);

	return Flux;
}
