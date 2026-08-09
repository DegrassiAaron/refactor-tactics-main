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

	// Indice 4 — ReactiveCapacitor (CP 6.7). REAZIONE cablata sulla semantica di `Action.Counter`: stesso
	// trigger («sono stato colpito da un attacco diretto») e stesso modo di raggiungere chi ha colpito. Gli
	// EFFETTI sono di Flux e sono DUE — scudo 15 a se' **e** 10 danni all'attaccante: e' la reazione per cui
	// CP 5.5 ha reso il motore componibile, e prima di allora ne sarebbe arrivata solo meta'.
	// Cooldown 3, dal catalogo eroi: piu' lungo dei 2 di `Action.Counter`, perche' fa anche da scudo.
	Flux->Actions.Add(MakeHeroReactionFromCoreAction(TEXT("Flux.ReactiveCapacitor"), TEXT("Action.Counter"),
		/*Cooldown*/ 3,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 15),
		  FRTActionEffectSpec(ERTActionEffect::Damage, 10) }));

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

URTHeroData* URTHeroCatalogLibrary::MakeRiva()
{
	URTHeroData* Riva = NewObject<URTHeroData>();
	Riva->HeroId = TEXT("Hero.Riva");
	Riva->DisplayName = FText::FromString(TEXT("Riva"));
	Riva->MaxHealth = 95;
	Riva->MovePoints = 5;
	Riva->VisionRange = 5;
	Riva->PushResistance = 0;
	Riva->Affinity = TEXT("Affinity.Water");
	// Simmetrica a Flux (Affinity.Water e' gia' la sua debolezza): la rivalita' fra i due eroi legati dalla
	// combo Wet e' un solo identificatore condiviso in entrambe le direzioni, non due nomi da sincronizzare.
	Riva->Weakness = TEXT("Affinity.Electricity");

	// Indice 0 — PressureJet, attacco base. 16 danni non corrisponde a NESSUNA fascia di
	// `BasicAttackDamageForRange` (28/25/22/20): a differenza di `Flux.ArcPulse`, l'attacco base di Riva e'
	// TEMATICO (linea, Wet, spinta), non generico per portata. Non si forza `MakeBasicAttack` su un numero
	// che non gli appartiene. Range 5: stessa portata di `Flux.LinearDischarge` (stessa forma, stesso riuso).
	Riva->Actions.Add(MakeHeroAction(TEXT("Riva.PressureJet"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 5, /*Cooldown*/ 0, ERTActionFallback::AttackCell,
		{
			FRTActionEffectSpec(ERTActionEffect::Damage, 16),
			FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, /*Turni*/ 1),
			FRTActionEffectSpec(ERTActionEffect::Push, 1),
		}, ERTAbilityShape::Line));

	// Indice 1 — CircularTide. Cura 18 agli alleati, Wet ai nemici: DUE effetti dichiarati nella stessa
	// lista, ma NESSUN resolver oggi applica un effetto diverso ad alleati e nemici della stessa area
	// (`bFriendlyFire` decide solo SE colpire un alleato, non CON QUALE effetto — vedi limiti dichiarati
	// sulla dichiarazione della funzione). Portata 4 e raggio 1: stessi numeri di `Flux.Overload` (portata
	// decisa con l'utente, raggio riusato da `Action.CircularAoE`).
	Riva->Actions.Add(MakeHeroAction(TEXT("Riva.CircularTide"), ERTResolutionPhase::Attack, /*Priority*/ 60,
		/*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
		{
			FRTActionEffectSpec(ERTActionEffect::Heal, 18),
			FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, /*Turni*/ 1),
		}, ERTAbilityShape::Area, /*AreaRadius*/ 1));

	// Indice 2 — FluidTrail. `Dash 3` che crea acqua lungo il percorso: la mobilita' e' rappresentabile
	// (fase Dash, stile lineare — stessa famiglia di `Action.Dash`), la creazione di terreno no (nessun
	// effetto di cella dinamica esiste: E8/E9). Nessun Effects dichiarato: il movimento non passa da li', e
	// l'acqua lasciata dietro non ha un modello da consumare.
	// Lo slot va dichiarato: `MakeHeroAction` mette `Main` di default, e per una mobilita' che non fa danno
	// sarebbe quello sbagliato (D-028). Chi lascia la scia si e' mosso, ma puo' ancora agire.
	URTActionData* FluidTrail = MakeHeroAction(TEXT("Riva.FluidTrail"), ERTResolutionPhase::FastMovement,
		/*Priority*/ 30, /*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop, {},
		ERTAbilityShape::Single, /*AreaRadius*/ 0, ERTActionSlot::Movement);
	// Lo stile va DICHIARATO, non lasciato al default: `ERTMovementStyle::None` non e' "non specificato", e'
	// "non si muove", e la fase Dash lo instraderebbe sul pathfinding normale — cioe' una scia d'acqua che
	// aggira gli ostacoli. Stessa provenienza di `Bastion.Ram`: lo stile viene dall'azione omologa (#142).
	FluidTrail->Def.MovementStyle = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash")).MovementStyle;
	Riva->Actions.Add(FluidTrail);

	// Indice 3 — MistVeil. "Crea fumo raggio 1": nessun modello di cella (vision-blocking dinamico, E8/E9).
	// Range 0 come `Flux.ConductiveNode`: segnaposto dichiarato, non un numero di bilanciamento, perche'
	// l'abilita' non ha ancora un effetto da mirare davvero.
	Riva->Actions.Add(MakeHeroAction(TEXT("Riva.MistVeil"), ERTResolutionPhase::Preparation, /*Priority*/ 35,
		/*Range*/ 0, /*Cooldown*/ 3, ERTActionFallback::Cancel, {}, ERTAbilityShape::Area, /*AreaRadius*/ 1));

	// Indice 4 — FlowReaction. `Reposition 1` dopo un attacco subito: e' una reazione che produce MOVIMENTO
	// dentro un boundary di risoluzione, quindi il suo aggancio e' **E14** (ADR-0004, finestre di reazione),
	// non E5 — il motore di E5 valuta i trigger sullo snapshot dei colpi e non sposta nessuno.
	// Il rinvio e' dichiarato come DATO: slot `None` e nessun trigger. Darle lo slot `Reaction` la farebbe
	// raccogliere dal pass delle reazioni, che la registrerebbe come attivata senza che accada nulla — un
	// esito falso nel TurnLog e' peggio di un'abilita' dichiaratamente incompleta.
	Riva->Actions.Add(MakeHeroAction(TEXT("Riva.FlowReaction"), ERTResolutionPhase::Preparation, /*Priority*/ 36,
		/*Range*/ 0, /*Cooldown*/ 3, ERTActionFallback::Cancel, {}, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		ERTActionSlot::None, /*bInterruptible*/ false));

	// Variante di CircularTide (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// Curativa: cura 24 (invece di 18), MA non applica Wet ai nemici — rinuncia al setup della combo con
	// Flux per curare di piu'.
	FRTAbilityVariant Healing;
	Healing.VariantId = TEXT("Riva.CircularTide.Healing");
	Healing.DisplayName = FText::FromString(TEXT("Marea curativa"));
	Healing.Tradeoff = FText::FromString(TEXT("cura 24 invece di 18, ma non applica Wet ai nemici"));
	Healing.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Heal, 24));

	// Urto: cura 10 (meno della base) MA applica Push 1 ai nemici — meno supporto, piu' controllo.
	FRTAbilityVariant Impact;
	Impact.VariantId = TEXT("Riva.CircularTide.Impact");
	Impact.DisplayName = FText::FromString(TEXT("Marea d'urto"));
	Impact.Tradeoff = FText::FromString(TEXT("cura solo 10, ma applica Push 1 ai nemici"));
	Impact.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Heal, 10));
	Impact.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Push, 1));

	Riva->Actions[1]->Variants.Add(Healing);
	Riva->Actions[1]->Variants.Add(Impact);

	return Riva;
}

URTHeroData* URTHeroCatalogLibrary::MakeBastion()
{
	URTHeroData* Bastion = NewObject<URTHeroData>();
	Bastion->HeroId = TEXT("Hero.Bastion");
	Bastion->DisplayName = FText::FromString(TEXT("Bastion"));
	Bastion->MaxHealth = 120;
	Bastion->MovePoints = 4;
	Bastion->VisionRange = 5;
	Bastion->PushResistance = 1; // l'unico del roster: compra HP e stabilita' con movimento e vista
	Bastion->Affinity = TEXT("Affinity.Structures");
	// Simmetrica a Vektor (CP 6.5), come Flux/Riva fra loro: il roster chiude in due coppie. Il piu' lento
	// del roster e' vulnerabile a chi il movimento lo fa di mestiere.
	Bastion->Weakness = TEXT("Affinity.Movement");

	// Indice 0 — ImpactShot, attacco base. 24 danni / range 3 NON e' una fascia di
	// `BasicAttackDamageForRange` (a range 3 la fascia da' 25): come `Riva.PressureJet`, l'attacco base e'
	// dell'eroe, non della tabella generica. Un punto in meno del corto raggio standard, in cambio della
	// stazza.
	Bastion->Actions.Add(MakeHeroAction(TEXT("Bastion.ImpactShot"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 3, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 24) }));

	// Indice 1 — KineticPanel (CP 9.5). E' `Action.CreateCover` con un nome d'eroe: stesso riuso che `Ram` fa
	// di `Action.Charge` e `Interposition` di `Action.Intercept`. Fase, priorita', portata, fallback e
	// soprattutto `StructureOp` vengono dal CORE — cablarli a mano qui significherebbe due cataloghi da tenere
	// allineati, e il primo a cambiare romperebbe il secondo in silenzio.
	//
	// La fase resta `Preparation`, ed e' il core ad essersi allineato a questa (D-a, 2026-08-09): si costruisce
	// prima che i colpi partano, altrimenti la copertura arriverebbe dopo aver incassato.
	//
	// **Portata 3, non piu' 1**: il numero e' quello che il catalogo azioni dichiara per `CreateCover`. Il
	// prezzo e' un dato in piu' nel piano — a portata 1 il bordo si deduceva dalla coppia di celle, a 3 no —
	// ed e' `PlannedCoverEdge`.
	{
		const FRTActionDef CoverDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover"));
		URTActionData* Panel = MakeHeroAction(TEXT("Bastion.KineticPanel"), CoverDef.ResolutionPhase,
			CoverDef.Priority, CoverDef.RangeCells, /*Cooldown*/ 2, CoverDef.Fallback, CoverDef.Effects);
		Panel->Def.StructureOp = CoverDef.StructureOp; // erige: e' il dato che il resolver legge
		Bastion->Actions.Add(Panel);
	}

	// Indice 2 — Reconfigure. Sposta o ruota una copertura ESISTENTE: dipende dallo stesso sistema mancante
	// di KineticPanel, piu' un bersaglio (la copertura) che non e' ne' un'unita' ne' una cella nel modello
	// attuale. Fase Preparation per lo stesso motivo del pannello.
	Bastion->Actions.Add(MakeHeroAction(TEXT("Bastion.Reconfigure"), ERTResolutionPhase::Preparation,
		/*Priority*/ 31, /*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// Indice 3 — Ram. E' una CARICA: 20 danni + Push 1, gli stessi numeri di `Action.Charge` del catalogo
	// azioni v0.1 §2 — non una coincidenza, e' la stessa azione con un nome d'eroe. Riuso identico di fase,
	// stile di movimento e portata: l'impatto risolve nel Blast (codice 20/30), come per ogni carica.
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	URTActionData* Ram = MakeHeroAction(TEXT("Bastion.Ram"), ChargeDef.ResolutionPhase, ChargeDef.Priority,
		ChargeDef.RangeCells, /*Cooldown*/ 2, ChargeDef.Fallback, ChargeDef.Effects);
	Ram->Def.MovementStyle = ChargeDef.MovementStyle; // LinearCharge: si ferma ADDOSSO al primo nemico
	Bastion->Actions.Add(Ram);

	// Indice 4 — Interposition (CP 6.7). REAZIONE cablata sulla semantica di `Action.Intercept`: Bastion
	// DIVENTA il bersaglio di un colpo diretto a un alleato entro 2 celle. Nessun effetto proprio, ed e'
	// corretto: interporsi non aggiunge danno ne' stati, cambia CHI subisce un colpo altrui — cosa che
	// `FRTActionEffectSpec` non sa esprimere e che il pass delle reazioni fa gia' per l'azione core.
	// La portata (2) e la priorita' (la piu' bassa fra le reazioni, cosi' la redirezione precede le altre)
	// vengono dal core; il cooldown 3 dal catalogo eroi. L'eroe piu' resistente del roster e' quello che si
	// mette in mezzo: e' la sua identita', non un numero in piu'.
	Bastion->Actions.Add(MakeHeroReactionFromCoreAction(TEXT("Bastion.Interposition"), TEXT("Action.Intercept"),
		/*Cooldown*/ 3));

	// Variante di KineticPanel (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// I due compromessi sono fatti di INTEGRITA' e DURATA, non di effetti, e vivono in `Parameters`.
	// **Da CP 9.5 qualcuno li legge**: `ResolveCoverStructures` prende integrita' e durata dalla variante
	// attiva dell'unita' (`ARTUnit::ActiveVariantId`) invece che dai valori base. Erano stati scritti qui
	// dichiarando che sarebbero serviti a E9: e' successo.
	FRTAbilityVariant Reinforced;
	Reinforced.VariantId = TEXT("Bastion.KineticPanel.Reinforced");
	Reinforced.DisplayName = FText::FromString(TEXT("Pannello rinforzato"));
	Reinforced.Tradeoff = FText::FromString(TEXT("integrita' 45 invece di 30, ma dura un solo turno"));
	Reinforced.Parameters.Add(TEXT("Integrity"), 45);
	Reinforced.Parameters.Add(TEXT("DurationTurns"), 1);
	Reinforced.Parameters.Add(TEXT("FreeRotations"), 0);

	FRTAbilityVariant Adaptive;
	Adaptive.VariantId = TEXT("Bastion.KineticPanel.Adaptive");
	Adaptive.DisplayName = FText::FromString(TEXT("Pannello adattivo"));
	Adaptive.Tradeoff = FText::FromString(TEXT("integrita' 25 invece di 30, ma una rotazione gratuita"));
	Adaptive.Parameters.Add(TEXT("Integrity"), 25);
	// Durata 0 = "non scade da sola" (il pannello base del catalogo terreni non dichiara una durata): il
	// rinforzato la compra con la fragilita' del tempo, l'adattivo con quella dell'integrita'.
	Adaptive.Parameters.Add(TEXT("DurationTurns"), 0);
	Adaptive.Parameters.Add(TEXT("FreeRotations"), 1);

	Bastion->Actions[1]->Variants.Add(Reinforced);
	Bastion->Actions[1]->Variants.Add(Adaptive);

	return Bastion;
}

URTHeroData* URTHeroCatalogLibrary::MakeVektor()
{
	URTHeroData* Vektor = NewObject<URTHeroData>();
	Vektor->HeroId = TEXT("Hero.Vektor");
	Vektor->DisplayName = FText::FromString(TEXT("Vektor"));
	Vektor->MaxHealth = 100;
	Vektor->MovePoints = 6; // il piu' mobile del roster: e' cio' che compra con l'assenza di difese
	Vektor->VisionRange = 6;
	Vektor->PushResistance = 0;
	Vektor->Affinity = TEXT("Affinity.Movement");
	// Simmetrica a Bastion: chi si muove di mestiere e' neutralizzato da chi gli chiude le traiettorie.
	// Il roster chiude in due coppie — Flux↔Riva sull'acqua, Bastion↔Vektor sullo spazio.
	Vektor->Weakness = TEXT("Affinity.Structures");

	// Indice 0 — PulseShot, attacco base. 21 danni / range 4: come per Riva e Bastion, non e' la fascia
	// generica (a range 4 darebbe 22). Un punto in meno del medio raggio, pagato in mobilita'.
	Vektor->Actions.Add(MakeHeroAction(TEXT("Vektor.PulseShot"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 4, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 21) }));

	// Indice 1 — InterceptShot. 16 danni + stop del movimento a chi ENTRA nella cella controllata: il trigger
	// e' d'ingresso su movimento, non «sono stato colpito», quindi appartiene a **E14** (ADR-0004) e non a E5.
	// Cablarla sul motore di E5 duplicherebbe `FRTSuppressiveZone` (CP 4.6), che quel trigger ce l'ha gia'.
	// Il rinvio e' dichiarato come DATO: slot `None`, nessun trigger — il pass delle reazioni non la raccoglie
	// e non puo' registrare un'attivazione che non produce nulla.
	Vektor->Actions.Add(MakeHeroAction(TEXT("Vektor.InterceptShot"), ERTResolutionPhase::Preparation,
		/*Priority*/ 30, /*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) }, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		ERTActionSlot::None, /*bInterruptible*/ false));

	// Indice 2 — PassingBlade. `Dash 3` che colpisce per 20 le unita' ATTRAVERSATE: l'unica abilita'
	// non-base di Vektor interamente rappresentabile. Stile `LinearDash` e non `LinearCharge`: la carica si
	// FERMA sul primo nemico, questa gli passa attraverso — e' la differenza che `ERTMovementStyle` esiste
	// per rendere un dato invece che un `if` sull'ActionId (CP 4.5).
	Vektor->Actions.Add(MakeHeroAction(TEXT("Vektor.PassingBlade"), ERTResolutionPhase::FastMovement,
		/*Priority*/ 30, /*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 20) }));
	// `LinearPass` e non `LinearDash`: fino al 2026-08-08 questo commento diceva «passa attraverso» e il
	// codice si fermava sul primo nemico come uno scatto qualsiasi, quindi i 20 danni dichiarati sopra non
	// avevano un momento in cui applicarsi. Lo stile e' un DATO proprio per questo (CP 4.5): la differenza
	// fra fermarsi, fermarsi addosso, scavalcare e attraversare non e' un `if` sull'ActionId.
	Vektor->Actions[2]->Def.MovementStyle = ERTMovementStyle::LinearPass;

	// Indice 3 — Deflection (CP 6.7). REAZIONE cablata sulla semantica di `Action.Deflect`: -20 sul colpo
	// diretto che l'ha innescata. La riduzione arriva dagli effetti del core (`ERTActionEffect::DamageReduction`,
	// CP 5.5) e resta distinta dallo scudo: uno scudo ASSORBE e si consuma, questa toglie punti al colpo.
	// Stessa famiglia di `Action.Guard` (-15 al primo colpo) ma con un trigger invece di una stance.
	// Cooldown 2, uguale al core: il catalogo eroi non ne dichiara uno diverso.
	Vektor->Actions.Add(MakeHeroReactionFromCoreAction(TEXT("Vektor.Deflection"), TEXT("Action.Deflect"),
		/*Cooldown*/ 2));

	// Indice 4 — Feint. Marca una CELLA e concede un `Reposition`: nessuna delle due meta' e' dichiarabile.
	// `Status` si applica alle UNITA', non alle celle; il movimento passa da `ERTMovementStyle`, non da
	// Effects. Fase Control (codice 30 del catalogo, macro-fase Blast): la finta precede il danno per
	// priorita', com'e' giusto per un'azione che sposta e disorienta.
	//
	// La durata della marcatura e' **1 turno**, decisa esplicitamente perche' il PDF non la dava. Non sta nei
	// `Parameters` perche' quelli sono della VARIANTE (dove il catalogo differenzia due configurazioni), e
	// qui non c'e' niente da differenziare: e' un numero dell'azione base, e finche' non esiste un effetto
	// che lo porti resta dichiarato nel catalogo eroi, non simulato in un campo che nessuno leggerebbe.
	Vektor->Actions.Add(MakeHeroAction(TEXT("Vektor.Feint"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// Variante di InterceptShot (vincolo v0.1: una sola abilita' fondamentale con variante per eroe).
	// Preciso: 20 danni ma controlla UNA cella. Esteso: 14 danni ma una LINEA di 3 celle. Il danno e' un
	// effetto; il numero di celle controllate non lo e' (non esiste il concetto di "zona di controllo" fuori
	// da `FRTSuppressiveZone`, che nessuno collega ancora a un eroe): sta nei `Parameters`, come l'integrita'
	// del pannello di Bastion.
	FRTAbilityVariant Precise;
	Precise.VariantId = TEXT("Vektor.InterceptShot.Precise");
	Precise.DisplayName = FText::FromString(TEXT("Intercetto preciso"));
	Precise.Tradeoff = FText::FromString(TEXT("20 danni invece di 16, ma controlla una sola cella"));
	Precise.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 20));
	Precise.Parameters.Add(TEXT("ControlledCells"), 1);

	FRTAbilityVariant Extended;
	Extended.VariantId = TEXT("Vektor.InterceptShot.Extended");
	Extended.DisplayName = FText::FromString(TEXT("Intercetto esteso"));
	Extended.Tradeoff = FText::FromString(TEXT("controlla una linea di 3 celle, ma solo 14 danni"));
	Extended.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 14));
	Extended.Parameters.Add(TEXT("ControlledCells"), 3);

	Vektor->Actions[1]->Variants.Add(Precise);
	Vektor->Actions[1]->Variants.Add(Extended);

	return Vektor;
}

TArray<URTHeroData*> URTHeroCatalogLibrary::GetHeroRoster()
{
	return { MakeFlux(), MakeRiva(), MakeBastion(), MakeVektor() };
}

URTActionData* URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(const FName& HeroActionId,
	const FName& CoreActionId, int32 CooldownTurns, const TArray<FRTActionEffectSpec>& Effects,
	int32 RangeCells)
{
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(CoreActionId);
	if (Core.ActionId != CoreActionId || Core.Slot != ERTActionSlot::Reaction)
	{
		// Fail-closed: un'azione core assente, o che reazione non e', darebbe un'abilita' che il pass delle
		// reazioni non guarda — inerte in partita e silenziosa nel TurnLog.
		return nullptr;
	}

	URTActionData* Action = MakeHeroAction(HeroActionId, Core.ResolutionPhase, Core.Priority,
		RangeCells >= 0 ? RangeCells : Core.RangeCells, CooldownTurns, Core.Fallback,
		Effects.Num() > 0 ? Effects : Core.Effects, ERTAbilityShape::Single, /*AreaRadius*/ 0,
		Core.Slot, Core.bCanBeInterrupted);

	// Il trigger viene dalla semantica core: e' la domanda a cui la reazione risponde («sono stato colpito?»,
	// «un alleato e' stato colpito?»), non un numero di bilanciamento dell'eroe.
	Action->Def.ReactionTrigger = Core.ReactionTrigger;
	return Action;
}
