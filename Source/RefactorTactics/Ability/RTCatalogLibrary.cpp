#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Ability/RTEquipmentData.h"
#include "Combat/RTCombatLibrary.h" // DeflectDamageReduction: il numero della riduzione resta uno solo

ERTMatchPhase URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase Phase)
{
	// Funzione TOTALE: un caso per ogni valore dell'enum, nessun `default` che nasconda una fase dimenticata
	// (aggiungerne una senza mapparla diventa un errore di compilazione, non un bug silenzioso a runtime).
	switch (Phase)
	{
	case ERTResolutionPhase::Snapshot:       return ERTMatchPhase::Planning; // congelamento a fine pianificazione
	case ERTResolutionPhase::Preparation:    return ERTMatchPhase::Prep;
	case ERTResolutionPhase::FastMovement:   return ERTMatchPhase::Dash;     // la mobilita' rapida precede il Blast
	case ERTResolutionPhase::NormalMovement: return ERTMatchPhase::Move;     // il percorso normale lo segue
	case ERTResolutionPhase::Control:        return ERTMatchPhase::Blast;    // il controllo non e' una macro-fase
	case ERTResolutionPhase::Attack:         return ERTMatchPhase::Blast;
	case ERTResolutionPhase::Environment:    return ERTMatchPhase::Cleanup;  // dopo il Move
	case ERTResolutionPhase::Cleanup:        return ERTMatchPhase::Cleanup;
	}
	return ERTMatchPhase::Cleanup;
}

bool URTCatalogLibrary::IsFastMovement(const FRTActionDef& Def)
{
	return MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash;
}

int32 URTCatalogLibrary::FirstDamage(const FRTActionDef& Def)
{
	for (const FRTActionEffectSpec& Spec : Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
	}
	return 0;
}

int32 URTCatalogLibrary::ResolutionPhaseCode(ERTResolutionPhase Phase)
{
	switch (Phase)
	{
	case ERTResolutionPhase::Snapshot:       return 0;
	case ERTResolutionPhase::Preparation:    return 10;
	case ERTResolutionPhase::FastMovement:   return 20; // stesso codice del movimento normale: il 20 si sdoppia
	case ERTResolutionPhase::NormalMovement: return 20;
	case ERTResolutionPhase::Control:        return 30;
	case ERTResolutionPhase::Attack:         return 40;
	case ERTResolutionPhase::Environment:    return 50;
	case ERTResolutionPhase::Cleanup:        return 60;
	}
	return 60;
}

TArray<FString> URTCatalogLibrary::ValidateActions(const TArray<FRTActionDef>& Actions)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		const FRTActionDef& Action = Actions[i];
		// Nome per i messaggi: l'ID se c'e', altrimenti la posizione (un errore che non dice DOVE e' inutile).
		const FString Where = Action.ActionId.IsNone()
			? FString::Printf(TEXT("azione #%d"), i)
			: Action.ActionId.ToString();

		if (Action.ActionId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: ActionId mancante (l'ID e' la chiave stabile del TurnLog)"), *Where));
		}
		else if (Seen.Contains(Action.ActionId))
		{
			Errors.Add(FString::Printf(TEXT("%s: ActionId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Action.ActionId);
		}

		if (Action.Priority < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: priorita' negativa (%d)"), *Where, Action.Priority));
		}
		if (Action.RangeCells < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: portata negativa (%d)"), *Where, Action.RangeCells));
		}
		if (Action.CostMP < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: costo negativo (%d)"), *Where, Action.CostMP));
		}
		if (Action.CooldownTurns < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: cooldown negativo (%d)"), *Where, Action.CooldownTurns));
		}

		if (Action.PropagationLimit < 0)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: propagazione senza limite (usa 0 per 'non propaga', N>0 per fermarsi a N celle)"), *Where));
		}

		if (Action.ResolutionPhase == ERTResolutionPhase::Snapshot)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: nessuna azione risolve nello Snapshot (fase di congelamento dello stato)"), *Where));
		}

		// Regola del vertical slice: un movimento bloccato si ferma nell'ultima cella valida. Un fallback
		// diverso (annullare, attaccare) renderebbe imprevedibile il movimento simultaneo.
		const bool bIsMovement = Action.ResolutionPhase == ERTResolutionPhase::FastMovement
			|| Action.ResolutionPhase == ERTResolutionPhase::NormalMovement;
		if (bIsMovement && Action.Fallback != ERTActionFallback::Stop)
		{
			Errors.Add(FString::Printf(TEXT("%s: azione di movimento con fallback diverso da Stop"), *Where));
		}
	}

	return Errors;
}

TArray<FString> URTCatalogLibrary::ValidateEquipment(const TArray<const URTEquipmentData*>& Equipment)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	for (int32 i = 0; i < Equipment.Num(); ++i)
	{
		const URTEquipmentData* Item = Equipment[i];
		if (Item == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("equipaggiamento #%d: riferimento nullo"), i));
			continue;
		}

		const FString Where = Item->EquipmentId.IsNone()
			? FString::Printf(TEXT("equipaggiamento #%d"), i)
			: Item->EquipmentId.ToString();

		if (Item->EquipmentId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: EquipmentId mancante"), *Where));
		}
		else if (Seen.Contains(Item->EquipmentId))
		{
			Errors.Add(FString::Printf(TEXT("%s: EquipmentId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Item->EquipmentId);
		}

		// Regola di prodotto: la scelta e' orizzontale. Un equipaggiamento senza svantaggio dichiarato e'
		// potere che si accumula, cioe' esattamente cio' che il canone esclude.
		if (Item->Drawback.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("%s: nessuno svantaggio dichiarato"), *Where));
		}
		if (Item->CooldownTurns < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: cooldown negativo (%d)"), *Where, Item->CooldownTurns));
		}
	}

	return Errors;
}

namespace
{
	FRTActionDef ShippedAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority, int32 Range,
		int32 Cooldown, ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects,
		bool bInterruptible = true, ERTActionSlot Slot = ERTActionSlot::Main,
		ERTMovementStyle Movement = ERTMovementStyle::None)
	{
		FRTActionDef Def;
		Def.Effects = Effects;
		Def.ActionId = Id;
		Def.ResolutionPhase = Phase;
		Def.Priority = Priority;
		Def.RangeCells = Range;
		Def.CooldownTurns = Cooldown;
		Def.Fallback = Fallback;
		Def.bCanBeInterrupted = bInterruptible;
		Def.Slot = Slot;
		Def.MovementStyle = Movement;
		// Chi corre a perdifiato non para: lo `Sprint` e' l'unica azione della v0.1 che nega la reazione.
		Def.bAllowsReaction = (Id != FName(TEXT("Action.Sprint")));
		return Def;
	}
}

TArray<FRTActionDef> URTCatalogLibrary::GetShippedActionCatalog()
{
	// Le azioni che il gioco assegna DAVVERO agli archetipi (ARTUnit::ConfigureAsArchetype). Sono la fonte
	// dei loro `Def`: una sola verita' fra codice e catalogo. Gli ID seguono la convenzione del catalogo v0.1
	// per le abilita' d'eroe (`<Eroe>.<Abilita>`), non quella delle azioni generiche (`Action.*`).
	TArray<FRTActionDef> Catalog;

	// Ranger — kiter a lunga gittata.
	Catalog.Add(ShippedAction(TEXT("Ranger.Shot"),        ERTResolutionPhase::Attack,       50, 6, 0, ERTActionFallback::Cancel,     { { ERTActionEffect::Damage, 25 } }));
	Catalog.Add(ShippedAction(TEXT("Ranger.PreciseShot"), ERTResolutionPhase::Attack,       60, 7, 2, ERTActionFallback::Cancel,     { { ERTActionEffect::Damage, 40 } }));
	Catalog.Add(ShippedAction(TEXT("Ranger.Burst"),       ERTResolutionPhase::Attack,       65, 6, 0, ERTActionFallback::AttackCell, { { ERTActionEffect::Damage, 50 }, { ERTActionEffect::Status, TAG_Status_Slow, 2 } }));
	// Lo scatto del Ranger e' un `Action.Dash` con la portata dell'archetipo: LINEARE, come ogni mobilita'
	// rapida del catalogo (§2). Senza lo stile dichiarato ricadrebbe sul pathfinding del movimento normale —
	// aggirerebbe gli ostacoli e attraverserebbe il terreno che nega lo scatto (#142).
	Catalog.Add(ShippedAction(TEXT("Ranger.Dash"),        ERTResolutionPhase::FastMovement, 30, 5, 2, ERTActionFallback::Stop,       {},
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearDash));

	// Guardian — mischia resistente.
	Catalog.Add(ShippedAction(TEXT("Guardian.Sweep"),   ERTResolutionPhase::Attack,       55, 3, 0, ERTActionFallback::AttackCell, { { ERTActionEffect::Damage, 30 }, { ERTActionEffect::Push, 2 } }));
	Catalog.Add(ShippedAction(TEXT("Guardian.Barrier"), ERTResolutionPhase::Preparation,  35, 0, 3, ERTActionFallback::Cancel,     { { ERTActionEffect::Shield, 40 } }, /*bInterruptible*/ false));
	Catalog.Add(ShippedAction(TEXT("Guardian.Quake"),   ERTResolutionPhase::Attack,       65, 3, 0, ERTActionFallback::AttackCell, { { ERTActionEffect::Damage, 40 }, { ERTActionEffect::Status, TAG_Status_Root, 2 } }));
	// La Carica del Guardian e' `Action.Charge` con la portata dell'archetipo: si ferma ADDOSSO al primo
	// nemico sulla traiettoria e lo colpisce. Gli effetti sono gli stessi della carica generica (20 danni piu'
	// una spinta di 1) perche' e' la stessa azione: una `LinearCharge` senza effetti sarebbe un contatto da
	// zero danni, cioe' una carica che non carica.
	Catalog.Add(ShippedAction(TEXT("Guardian.Charge"),  ERTResolutionPhase::FastMovement, 35, 4, 3, ERTActionFallback::Stop,
		{ { ERTActionEffect::Damage, 20 }, { ERTActionEffect::Push, 1 } },
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearCharge));

	return Catalog;
}

FRTActionDef URTCatalogLibrary::FindShippedAction(const FName& ActionId)
{
	for (const FRTActionDef& Def : GetShippedActionCatalog())
	{
		if (Def.ActionId == ActionId) { return Def; }
	}
	return FRTActionDef();
}

TArray<FRTActionDef> URTCatalogLibrary::GetCoreActionCatalog()
{
	TArray<FRTActionDef> Catalog;

	// `Action.Sprint` (catalogo v0.1 §2) — 8 MP, consuma movimento E azione principale, applica `Status.Exposed`
	// fino al Cleanup. Per le azioni di mobilita' rapida `RangeCells` e' il BUDGET in punti movimento, non un
	// numero di celle: su terreno difficile si arriva meno lontano (e' lo stesso budget del movimento normale,
	// con un'altra quantita').
	//
	// Lo svantaggio dello scatto lungo e' `Exposed`, dichiarato come EFFETTO: chi corre allo scoperto incassa
	// +5 dal primo colpo. Niente di tutto cio' e' scritto nell'orchestratore.
	Catalog.Add(ShippedAction(TEXT("Action.Sprint"), ERTResolutionPhase::FastMovement, /*Priority*/ 60,
		/*Range (MP)*/ 8, /*Cooldown*/ 0, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Exposed, /*Turni*/ 1) },
		/*bInterruptible*/ true, ERTActionSlot::MovementAndMain, ERTMovementStyle::Budget));

	// `Action.Wait` (catalogo v0.1 §1) — non fa nulla e risolve per ultima (priorita' 100). Serve gia' ora
	// perche' e' cio' in cui `Fallback.Wait` trasforma un'azione: senza, il fallback dovrebbe inventarsi in
	// codice un'azione vuota, cioe' una seconda definizione della stessa cosa.
	//
	// Il catalogo le da' fallback «—»: l'enum non ha un valore "nessuno", e per un'azione che non muove e non
	// colpisce `Stop` e `Cancel` sono lo stesso comportamento osservabile (niente). Si usa `Stop` perche' e'
	// quello che il validator richiede alle azioni di fase Move — un'eccezione in meno, non una regola nuova.
	Catalog.Add(ShippedAction(TEXT("Action.Wait"), ERTResolutionPhase::NormalMovement, /*Priority*/ 100,
		/*Range*/ 0, /*Cooldown*/ 0, ERTActionFallback::Stop, {},
		/*bInterruptible*/ false, ERTActionSlot::None));

	// `Action.Move` — il percorso normale, dopo il Blast (ADR-0003 §3). Nessun effetto dichiarato: a muovere
	// l'unita' e' il resolver dei percorsi, che avanza a micro-step sullo snapshot. Un effetto "MoveTo" qui
	// duplicherebbe quella decisione in un secondo posto.
	Catalog.Add(ShippedAction(TEXT("Action.Move"), ERTResolutionPhase::NormalMovement, /*Priority*/ 50,
		/*Range (MP)*/ 5, /*Cooldown*/ 0, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::Budget));

	// `Action.BasicAttack` — identita', fase, priorita' e fallback stanno qui; DANNO e PORTATA no, perche'
	// dipendono dall'eroe e dalla sua arma (catalogo §1, tabella delle fasce). Li applica MakeBasicAttack:
	// mettere qui un numero significherebbe sceglierne uno arbitrario per tutti.
	Catalog.Add(ShippedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 0, /*Cooldown*/ 0, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));

	// `Action.Guard` — si prepara nel Prep e vale per il turno: -15 al primo danno diretto, resiste a una
	// spinta di 1 cella, scade nel Cleanup. Non interrompibile (catalogo §1).
	Catalog.Add(ShippedAction(TEXT("Action.Guard"), ERTResolutionPhase::Preparation, /*Priority*/ 40,
		/*Range (self)*/ 0, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Guarded, /*Turni*/ 1) },
		/*bInterruptible*/ false, ERTActionSlot::Main));

	// `Action.Activate` e `Action.Interact` — portata 1: solo oggetti ADIACENTI. Nessun effetto dichiarato
	// finche' non esistono oggetti da attivare (porte, consolle, ponti, obiettivi): quelli sono E9/E10. Qui
	// entrano identita', fase, priorita' e il vincolo di adiacenza, che e' gia' una regola verificabile.
	Catalog.Add(ShippedAction(TEXT("Action.Activate"), ERTResolutionPhase::Attack, /*Priority*/ 70,
		/*Range*/ 1, /*Cooldown*/ 0, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));
	Catalog.Add(ShippedAction(TEXT("Action.Interact"), ERTResolutionPhase::Attack, /*Priority*/ 80,
		/*Range*/ 1, /*Cooldown*/ 0, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));

	// --- Mobilita' LINEARI (catalogo §2) ---------------------------------------------------------------
	// Tutte in macro-fase Dash: riposizionarsi in fretta e' cio' che permette di sparare da un'altra parte
	// nello stesso turno (ADR-0003 §3). Tutte con `Fallback.Stop`: se la traiettoria si chiude ci si ferma
	// nell'ultima cella valida, non si annulla e non si aggira.

	// `Dash` — 3 celle su una delle sei direzioni. Non consuma il percorso Move: scatto e movimento normale
	// convivono nello stesso turno (e' lo slot Principale a essere speso, non quello di movimento).
	Catalog.Add(ShippedAction(TEXT("Action.Dash"), ERTResolutionPhase::FastMovement, /*Priority*/ 30,
		/*Range*/ 3, /*Cooldown*/ 1, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearDash));

	// `Charge` — 3 celle, si ferma ADDOSSO al primo nemico e lo colpisce: 20 danni piu' una spinta di 1.
	// Gli effetti sono dichiarati qui, ma si applicano nel Blast (codice 20/30 del catalogo): il movimento e'
	// fase 20, l'impatto e' controllo, e il controllo risolve per priorita' dentro il Blast.
	Catalog.Add(ShippedAction(TEXT("Action.Charge"), ERTResolutionPhase::FastMovement, /*Priority*/ 35,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 20), FRTActionEffectSpec(ERTActionEffect::Push, 1) },
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearCharge));

	// `Leap` — 3 celle scavalcando cio' che sta in mezzo (unita', coperture basse). La cella d'atterraggio
	// invece la si subisce: dev'essere percorribile e libera.
	Catalog.Add(ShippedAction(TEXT("Action.Leap"), ERTResolutionPhase::FastMovement, /*Priority*/ 25,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearLeap));

	// `Reposition` — due celle e nient'altro: nessuno stato, nessuna traversata. E' lo scatto "tattico" che si
	// paga poco, e la differenza con `Sprint` sta tutta nei dati (2 celle in linea contro 8 MP piu' Exposed).
	Catalog.Add(ShippedAction(TEXT("Action.Reposition"), ERTResolutionPhase::FastMovement, /*Priority*/ 40,
		/*Range*/ 2, /*Cooldown*/ 1, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Main, ERTMovementStyle::LinearDash));

	// --- Azioni OFFENSIVE (catalogo §3) ----------------------------------------------------------------
	// Tutte nel Blast tranne la soppressione, che si PREPARA. La priorita' e' cio' che le distingue davvero:
	// dentro la stessa macro-fase risolvono nell'ordine 40 (marchio) → 55 (linea) → 60 (precisione) →
	// 65 (area) → 80 (pesante). Il marchio arriva per primo perche' il suo +6 deve poter valere sui colpi
	// dello stesso turno; il pesante per ultimo perche' e' cio' che il catalogo compra con i suoi 35 danni.

	// `PrecisionAttack` — 24 danni fissi, portata dell'arma +1 (la mette MakePrecisionAttack: qui, come per
	// `BasicAttack`, un numero sarebbe arbitrario). Non e' usabile dopo uno Sprint, ma questo NON e' scritto
	// qui: `Sprint` occupa gia' movimento e azione principale, e ValidateActionSlots ne fa un caso della
	// regola generale invece di un'eccezione sull'ActionId.
	Catalog.Add(ShippedAction(TEXT("Action.PrecisionAttack"), ERTResolutionPhase::Attack, /*Priority*/ 60,
		/*Range*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 24) }));

	// `HeavyAttack` — 35 danni e priorita' 80: risolve tardi, ed e' il prezzo che paga per essere il colpo
	// piu' duro. Interrompibile: se un `Action.Interrupt` la coglie prima del Blast non produce NULLA — non
	// mezzo danno, non un effetto parziale (lo garantisce URTActionEffectLibrary::ProduceEvents).
	// Contro le STRUTTURE vale 20, non 35 (DoD di CP 9.2): un colpo pesante scalfisce un muro meno di una
	// carica da sfondamento dedicata (`Gadget.BreachCharge`, 35 a struttura, epic E7 #61). Sono due scale
	// diverse, ed e' per questo che sono due effetti dichiarati e non un numero solo.
	Catalog.Add(ShippedAction(TEXT("Action.HeavyAttack"), ERTResolutionPhase::Attack, /*Priority*/ 80,
		/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 35),
		  FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20) }));

	// `LineAttack` — 22 danni al PRIMO bersaglio valido su una delle sei direzioni, portata 5. Non e' la
	// `Shape::Line` delle abilita' d'archetipo (che colpisce tutti quelli attraversati): la risolve
	// URTOffensiveActionLibrary::ResolveLineAttack, che si ferma sul primo che incontra.
	// `Fallback.AttackCell`: se il bersaglio si sposta, la linea parte comunque dov'era puntata.
	Catalog.Add(ShippedAction(TEXT("Action.LineAttack"), ERTResolutionPhase::Attack, /*Priority*/ 55,
		/*Range*/ 5, /*Cooldown*/ 1, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 22) }));

	// `CircularAoE` — 18 danni in un esagono di raggio 1, centro entro 4 celle. `RangeCells` e' la portata
	// del CENTRO, il raggio dell'area sta nell'intento (`FRTHexAttackIntent::AreaRadius`): sono due numeri
	// diversi e confonderli farebbe esplodere l'area a quattro celle di distanza.
	//
	// **Friendly fire attivo**: e' l'unica azione della v0.1 che colpisce anche i propri, e lo dichiara nei
	// DATI (`bFriendlyFire`). Non e' una dimenticanza del filtro di squadra, e non e' una decisione di chi
	// costruisce l'intento: chi assegnera' l'area a un eroe (E6) la copia da qui e basta.
	{
		FRTActionDef Area = ShippedAction(TEXT("Action.CircularAoE"), ERTResolutionPhase::Attack, /*Priority*/ 65,
			/*Range (centro)*/ 4, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
			{ FRTActionEffectSpec(ERTActionEffect::Damage, 18) });
		Area.bFriendlyFire = true;
		Catalog.Add(Area);
	}

	// `SuppressiveLine` — si PREPARA (fase 10, quindi macro-fase Prep) e si attiva su un trigger: il primo
	// nemico che entra in una cella controllata durante il Move prende 16 danni e si ferma li'. Una sola
	// attivazione per turno. Non interrompibile: una volta preparata la linea, c'e'.
	//
	// E' l'unica offensiva che non risolve nel Blast, e la ragione e' che il suo effetto non ha un bersaglio
	// al momento della pianificazione: ce l'ha chi ci cammina dentro.
	Catalog.Add(ShippedAction(TEXT("Action.SuppressiveLine"), ERTResolutionPhase::Preparation, /*Priority*/ 30,
		/*Range*/ 5, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) },
		/*bInterruptible*/ false));

	// `MarkTarget` — nessun danno proprio: applica `Status.Marked` per un turno, e il prossimo attacco
	// alleato contro quel bersaglio infligge +6 e consuma il marchio. Priorita' 40, la piu' bassa delle
	// offensive, perche' un marchio che arrivasse dopo i colpi non servirebbe a nulla.
	// `Range 0` = **portata del portatore**, come `Action.PrecisionAttack` («range dell'arma +1», catalogo §3):
	// non e' un'azione a portata nulla. La traduzione la fa `ARTTurnManager` sull'istanza (CP 8.2), che prima
	// copriva le sole azioni non catalogate e lasciava quindi queste due invalidabili per fuori portata.
	Catalog.Add(ShippedAction(TEXT("Action.MarkTarget"), ERTResolutionPhase::Attack, /*Priority*/ 40,
		/*Range*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Marked, /*Turni*/ 1) }));

	// --- Difensive e reazioni (catalogo §4) ---------------------------------------------------------------
	// ATTENZIONE alla riga «Slot» della tabella: solo `Counter`, `Intercept` e `Deflect` occupano lo slot
	// REAZIONE (0-1 per unita', indipendente da Movimento e Principale, trigger valutato sullo snapshot del
	// Blast — CP 5.1). `Brace`, `Shield` e `Cleanse` sono azioni PRINCIPALI: si dichiarano e basta, senza
	// trigger, e risolvono nella loro fase come qualunque altra azione. Trattarle tutte e cinque come
	// "reazioni" perche' stanno nella stessa sezione del catalogo sarebbe una lettura sbagliata della tabella.
	//
	// Le reazioni non dichiarano un `Fallback` vero (il catalogo lo dice esplicitamente): `Cancel` resta un
	// segnaposto inerte, non usato da nessun percorso.
	//
	// Range 0 per tutte: la tabella non dichiara una portata per questa sezione. Per le tre difensive su se
	// stessi 0 e' il valore giusto (`self`); per `Counter` significa che il contrattacco raggiunge chi ha
	// colpito, chiunque sia — la portata di un colpo di ritorno non e' un dato che il catalogo fornisce, e
	// inventarne uno cambierebbe quali attacchi si possono punire.

	// `Counter` — contrattacco da 16 danni contro chi ha colpito, DOPO il colpo ricevuto. Il danno e'
	// dichiarato qui come effetto: il resolver lo legge dal `Def`, non da una costante propria.
	{
		FRTActionDef Counter = ShippedAction(TEXT("Action.Counter"), ERTResolutionPhase::Control, /*Priority*/ 20,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) }, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Counter.ReactionTrigger = ERTReactionTrigger::HitByDirectAttack;
		Catalog.Add(Counter);
	}

	// `Deflect` — riduce di 20 il danno diretto che l'ha innescata, DICHIARANDOLO come effetto
	// (`ERTActionEffect::DamageReduction`, CP 5.5). Fino a CP 5.2 il numero viveva solo come
	// `URTCombatLibrary::DeflectDamageReduction` letta da un `if (ActionId == "Action.Deflect")` nel
	// `TurnManager`: la costante resta la fonte del valore, ma chi lo applica ora lo legge dai dati — cosi' una
	// reazione d'eroe puo' riusare la stessa semantica con un numero proprio senza un secondo ramo.
	// Resta diverso dal -15 di `Action.Guard`, che non e' una reazione: quello e' uno stato di Prep.
	{
		FRTActionDef Deflect = ShippedAction(TEXT("Action.Deflect"), ERTResolutionPhase::Control, /*Priority*/ 15,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::DamageReduction, URTCombatLibrary::DeflectDamageReduction) },
			/*bInterruptible*/ true, ERTActionSlot::Reaction);
		Deflect.ReactionTrigger = ERTReactionTrigger::HitByDirectAttack;
		Catalog.Add(Deflect);
	}

	// `Intercept` — l'intercettore DIVENTA il bersaglio di un colpo diretto a un alleato entro 2 celle.
	// Nessun effetto dichiarato: non aggiunge danno, ne' sposta, ne' applica stati — cambia CHI subisce un
	// colpo altrui, che non e' esprimibile come `FRTActionEffectSpec` (quelli agiscono su un bersaglio dato).
	//
	// Range **2**: qui il numero e' dichiarato dal catalogo ("un alleato entro 2 celle"), non deciso da noi.
	// Priorita' **10**, la piu' bassa fra le reazioni (Deflect 15, Counter 20), ed e' una regola, non un
	// dettaglio: cambiando il bersaglio dei colpi, Intercept deve risolvere PRIMA che le altre reazioni
	// valutino chi e' stato colpito — altrimenti il bersaglio originale contrattaccherebbe per un colpo che
	// non ha piu' ricevuto.
	{
		FRTActionDef Intercept = ShippedAction(TEXT("Action.Intercept"), ERTResolutionPhase::Control, /*Priority*/ 10,
			/*Range*/ 2, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Intercept.ReactionTrigger = ERTReactionTrigger::AllyHitByDirectAttack;
		Catalog.Add(Intercept);
	}

	// `Brace` — azione PRINCIPALE di Prep. Dichiara DUE stati: `Braced` (-10 a ogni danno diretto e blocca la
	// prima spinta) e `Root` (blocca il movimento volontario). Root e' riuso 1:1 di un meccanismo gia'
	// collaudato — azzera movimento e scatto, non tocca attacchi ne' spostamento subito, che e' esattamente
	// cio' che il catalogo chiede a chi si irrigidisce: si pianta per incassare, non smette di combattere.
	Catalog.Add(ShippedAction(TEXT("Action.Brace"), ERTResolutionPhase::Preparation, /*Priority*/ 30,
		/*Range (self)*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Braced, /*Turni*/ 1),
		  FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Root, /*Turni*/ 1) },
		/*bInterruptible*/ false, ERTActionSlot::Main));

	// `Shield` — azione PRINCIPALE di Prep: 25 punti di scudo TEMPORANEO, consumati prima della salute e
	// scaduti nel Cleanup. Stesso identico meccanismo di `Guardian.Barrier` (che ne da' 40): `ResolvePrep`
	// traduce l'effetto in `AddTemporaryShield` senza sapere quale azione l'abbia prodotto. Non protegge dal
	// controllo senza danno per costruzione — uno scudo assorbe danno, e Root/Slow non ne sono.
	Catalog.Add(ShippedAction(TEXT("Action.Shield"), ERTResolutionPhase::Preparation, /*Priority*/ 35,
		/*Range (self)*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 25) },
		/*bInterruptible*/ false, ERTActionSlot::Main));

	// `Cleanse` — azione PRINCIPALE, codice 30 (controllo) quindi risolve nel Blast PRIMA del danno: purificarsi
	// dopo aver incassato il colpo che lo stato ha aggravato non servirebbe a niente. Nessun effetto dichiarato:
	// "rimuovi uno stato a scelta" non e' esprimibile come `FRTActionEffectSpec` (che applica, non toglie), e
	// soprattutto QUALE stato lo decide il piano del giocatore (`ARTUnit::PlannedCleansePriority`), non il dato
	// dell'azione.
	Catalog.Add(ShippedAction(TEXT("Action.Cleanse"), ERTResolutionPhase::Control, /*Priority*/ 25,
		/*Range (self)*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));

	// --- Azioni di CONTROLLO (catalogo §5) -------------------------------------------------------------
	// Tutte risolvono nel Blast (fase dichiarata `Control`, codice 30) PRIMA del danno: la priorita' le mette
	// nell'ordine 20 (Interrupt) → 25 (Root) → 40 (Push/Pull) → 50 (Slow) — sotto la piu' bassa offensiva
	// (MarkTarget, anch'essa 40): un'interruzione o un radicamento devono valere prima che qualunque colpo
	// parta, non dopo. Range 1 per Push/Root/Interrupt/Slow, **2 per Pull**: la tabella del catalogo non
	// dichiarava una portata (unica sezione senza colonna Range), quindi il numero e' deciso qui — e per Pull
	// e' diverso dagli altri quattro per un motivo geometrico dichiarato sotto, non per svista.
	//
	// Push/Root/Slow riusano la STESSA pipeline di `ResolveCombat` che gia' applica gli effetti di
	// Guardian.Sweep/Ranger.Burst (un'azione senza danno e' comunque un "colpo" con Power 0: l'effetto
	// collaterale passa lo stesso). Interrupt e' l'eccezione: cancella l'INTERA azione di un'altra unita', non
	// un effetto su un bersaglio, e per questo non dichiara nessun `FRTActionEffectSpec` — la sua conseguenza
	// si applica filtrando `Plan.Hits` prima che diventino danno o eventi (`ARTTurnManager::ResolveCombat`).

	// `Push` — spinta di 1 cella, che allontana. Riusa lo stesso meccanismo di knockback di Guardian.Sweep.
	Catalog.Add(ShippedAction(TEXT("Action.Push"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 1, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Push, 1) }));

	// `Pull` — trazione di 1 cella, che avvicina: prima azione del catalogo a usare `ERTActionEffect::Pull`.
	// Range **2**, non 1 come le altre quattro: con targeting a 1 (adiacenza) e trazione di 1, il bersaglio
	// finirebbe SEMPRE sulla cella di chi tira — sempre occupata, quindi la trazione si annullerebbe per
	// costruzione, in ogni caso, senza eccezioni. E' l'unica delle cinque a deviare, e la ragione e'
	// geometrica: bisogna poter agganciare un bersaglio a 2 celle per tirarlo a 1 senza finirgli addosso.
	Catalog.Add(ShippedAction(TEXT("Action.Pull"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 2, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Pull, 1) }));

	// `Root` — blocca il movimento per 1 turno. Cancella i micro-step di movimento NON ANCORA risolti (fase
	// Move, dopo il Blast) tramite `GetEffectiveMoveRange`, che azzera il budget per chi e' radicato — non
	// impedisce attacchi, Guard o Activate, che non passano da quel budget.
	Catalog.Add(ShippedAction(TEXT("Action.Root"), ERTResolutionPhase::Control, /*Priority*/ 25,
		/*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Root, /*Turni*/ 1) }));

	// `Slow` — +1 al costo di OGNI cella per 1 turno (non dimezza il raggio: e' un meccanismo diverso da
	// quello che `Ranger.Burst` applicava allo stesso tag prima di questo checkpoint — vedi
	// `URTCombatLibrary::EffectiveMoveRange` e il modificatore di costo in `FRTHexSimUnit`). Non riduce la
	// portata delle mobilita' lineari (Dash/Charge/Leap/Reposition): quelle non hanno un costo per cella da
	// aumentare, e la v0.1 le dichiara fuori dall'effetto di Slow.
	Catalog.Add(ShippedAction(TEXT("Action.Slow"), ERTResolutionPhase::Control, /*Priority*/ 50,
		/*Range*/ 1, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Slow, /*Turni*/ 1) }));

	// `Interrupt` — nessun effetto dichiarabile: la sua conseguenza e' cancellare l'azione di un'altra unita',
	// non modificarne le statistiche. Agisce solo su chi dichiara `bCanBeInterrupted = true` — il controllo
	// e' fatto da `ARTTurnManager::ResolveCombat`, non da un flag che questa azione porterebbe con se'.
	Catalog.Add(ShippedAction(TEXT("Action.Interrupt"), ERTResolutionPhase::Control, /*Priority*/ 20,
		/*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// --- Azioni AMBIENTALI (catalogo §6) -----------------------------------------------------------------
	// `Electrify` — la combo firma del gioco (CP 8.3). Fase `Environment` (codice 50), quindi risolve nel
	// **Cleanup**, dopo il Move: cosi' colpisce anche chi e' appena entrato nell'acqua, che e' il punto
	// tattico dell'azione. Portata 4, cooldown 2, danno iniziale 20 (catalogo azioni §6).
	//
	// `PropagationLimit = 3` e' il primo valore non nullo di quel campo: fino a qui esisteva solo come regola
	// del validator («una propagazione illimitata rende il turno impredicibile», errore dichiarato §17). Il
	// danno PROPAGATO (12) non e' un secondo `Effects`, perche' non e' un effetto dell'azione sul bersaglio
	// ma il valore che l'ambiente porta oltre: vive come `URTCombatLibrary::PropagatedElectricDamage`,
	// accanto alle altre costanti di calcolo (Guard, Deflect, Brace, Burning).
	{
		FRTActionDef Electrify = ShippedAction(TEXT("Action.Electrify"), ERTResolutionPhase::Environment,
			/*Priority*/ 30, /*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Damage, 20) });
		Electrify.PropagationLimit = 3;
		Catalog.Add(Electrify);
	}

	// `Ignite` e `CreateWater` — le due azioni che CAMBIANO la mappa (CP 8.4). Il catalogo azioni le elenca
	// entrambe con priorita' 60 e portata 4; qui risolvono nella sola fase `Environment` (Cleanup), non nel
	// Blast: una cella che prende fuoco a meta' turno cambierebbe il costo di un percorso gia' calcolato.
	//
	// Nessuna delle due dichiara `Effects`: il loro esito non e' un effetto su un'UNITA' (danno, cura, stato)
	// ma una modifica della CELLA, che `FRTActionEffectSpec` non sa esprimere — gli effetti si applicano a
	// bersagli, non a terreno. La superficie che creano e la durata vivono nel resolver ambientale, che e'
	// l'unico posto in cui il terreno dinamico esiste.
	//
	// **Durata 2 turni** per entrambe, dal catalogo terreni §2 (fuoco) e dal catalogo azioni §6 (acqua).
	Catalog.Add(ShippedAction(TEXT("Action.Ignite"), ERTResolutionPhase::Environment, /*Priority*/ 60,
		/*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));
	Catalog.Add(ShippedAction(TEXT("Action.CreateWater"), ERTResolutionPhase::Environment, /*Priority*/ 60,
		/*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// `ModifyArc` — apre o chiude un COLLEGAMENTO fra celle (CP 8.5). Non tocca le superfici: cambia la
	// topologia, ed e' per questo che la DoD chiede che **incrementi la revisione** della mappa — il numero che
	// invalida le cache di percorso. Fase `Environment` come le altre ambientali: cambiare un arco a meta'
	// Blast renderebbe invalido un percorso gia' calcolato in questo stesso turno.
	Catalog.Add(ShippedAction(TEXT("Action.ModifyArc"), ERTResolutionPhase::Environment, /*Priority*/ 75,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// `Heal` — cura 20, portata 3, e **puo' bersagliare se stessi** (catalogo azioni §6). Priorita' 70: risolve
	// DOPO gli attacchi (50-65), quindi cura le ferite di questo turno e non quelle del turno prima.
	// A differenza delle ambientali risolve nel **Blast**: e' un'azione di supporto, non una modifica del campo.
	{
		FRTActionDef Heal = ShippedAction(TEXT("Action.Heal"), ERTResolutionPhase::Attack, /*Priority*/ 70,
			/*Range*/ 3, /*Cooldown*/ 1, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Heal, 20) });
		Catalog.Add(Heal);
	}

	return Catalog;
}

int32 URTCatalogLibrary::BasicAttackDamageForRange(int32 WeaponRangeCells)
{
	// Tabella delle fasce (catalogo v0.1 §1): corpo a corpo 28/r1 · corto 25/r3 · medio 22/r4 · lungo 20/r6.
	// Piu' lontano si colpisce, meno si fa male: e' la scelta orizzontale del catalogo, non una scala di potenza.
	// Le portate intermedie ricadono nella fascia il cui limite le contiene (r2 -> corto, r5 -> lungo).
	if (WeaponRangeCells <= 1) { return 28; }
	if (WeaponRangeCells <= 3) { return 25; }
	if (WeaponRangeCells <= 4) { return 22; }
	return 20;
}

FRTActionDef URTCatalogLibrary::MakeBasicAttack(int32 WeaponRangeCells)
{
	// L'attacco base di UN eroe: l'identita' viene dal catalogo, i due numeri che dipendono dall'arma li mette
	// la fascia. Cosi' `Action.BasicAttack` resta una sola azione con un solo ID, invece di quattro varianti.
	FRTActionDef Def = FindCoreAction(TEXT("Action.BasicAttack"));
	if (Def.ActionId.IsNone())
	{
		return Def; // catalogo incompleto: meglio una definizione vuota che una inventata qui
	}

	const int32 Range = FMath::Max(1, WeaponRangeCells);
	Def.RangeCells = Range;
	Def.Effects.Reset();
	Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, BasicAttackDamageForRange(Range)));
	return Def;
}

FRTActionDef URTCatalogLibrary::MakeWeaponAttack(const FName& ActionId, int32 WeaponRangeCells)
{
	// Il catalogo dichiara per queste azioni un targeting "bersaglio" senza numero: la portata e' quella
	// dell'arma dell'eroe. Metterne una qui significherebbe sceglierne una arbitraria per tutti.
	FRTActionDef Def = FindCoreAction(ActionId);
	if (Def.ActionId.IsNone())
	{
		return Def; // catalogo incompleto: meglio una definizione vuota che una inventata qui
	}

	Def.RangeCells = FMath::Max(1, WeaponRangeCells);
	return Def;
}

FRTActionDef URTCatalogLibrary::MakePrecisionAttack(int32 WeaponRangeCells)
{
	// Il **+1** e' l'identita' della precisione (catalogo §3): si colpisce una cella piu' lontano di quanto
	// arrivi l'arma. Sta in una funzione che porta il nome dell'azione, non in un `if` sull'ActionId dentro
	// MakeWeaponAttack — cosi' aggiungere un'altra azione con un bonus diverso non tocca nulla di questo.
	return MakeWeaponAttack(TEXT("Action.PrecisionAttack"), FMath::Max(1, WeaponRangeCells) + 1);
}

TArray<FString> URTCatalogLibrary::ValidateActionSlots(const TArray<FRTActionDef>& PlannedActions)
{
	TArray<FString> Errors;

	// Chi ha gia' preso quale slot: serve a NOMINARE il colpevole («la principale e' occupata da Sprint»),
	// perche' un errore che dice solo "slot pieno" costringe a ricostruire il piano a mano.
	FName MovementTakenBy;
	FName MainTakenBy;

	for (const FRTActionDef& Action : PlannedActions)
	{
		const bool bTakesMovement = Action.Slot == ERTActionSlot::Movement
			|| Action.Slot == ERTActionSlot::MovementAndMain;
		const bool bTakesMain = Action.Slot == ERTActionSlot::Main
			|| Action.Slot == ERTActionSlot::MovementAndMain;

		if (bTakesMovement)
		{
			if (!MovementTakenBy.IsNone())
			{
				Errors.Add(FString::Printf(TEXT("%s: slot movimento gia' occupato da %s"),
					*Action.ActionId.ToString(), *MovementTakenBy.ToString()));
			}
			else
			{
				MovementTakenBy = Action.ActionId;
			}
		}

		if (bTakesMain)
		{
			if (!MainTakenBy.IsNone())
			{
				Errors.Add(FString::Printf(TEXT("%s: azione principale gia' occupata da %s"),
					*Action.ActionId.ToString(), *MainTakenBy.ToString()));
			}
			else
			{
				MainTakenBy = Action.ActionId;
			}
		}
	}

	return Errors;
}

FRTActionDef URTCatalogLibrary::FindCoreAction(const FName& ActionId)
{
	for (const FRTActionDef& Def : GetCoreActionCatalog())
	{
		if (Def.ActionId == ActionId) { return Def; }
	}
	return FRTActionDef();
}
