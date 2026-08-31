#include "Turn/RTTurnManager.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTPredictiveLibrary.h" // boundary della Predictive Action (E18): la decisione sta nel puro
#include "Turn/RTReactionOpportunityTypes.h" // Decision Boundary dell'Overwatch (CP 14.5): opportunity e decisione
#include "Combat/RTOffensiveActionLibrary.h" // MakeSuppressiveZone: la zona dell'Overwatch E' quella della soppressione
#include "Perception/RTPerceptionLibrary.h" // TeamAwarenessOfCell: il trigger richiede `Rilevato` (ADR-0004 §6)
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTFacingLibrary.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Replay/RTMatchHistoryLibrary.h" // indice delle partite: una riga per partita, fuori dagli archivi (#416)
#include "Replay/RTReplayRecorderLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

// =================================================================================================
// I pass della fase BLAST di `ARTTurnManager`.
//
// Vivono in un'unita' di traduzione propria e non in `RTTurnManager.cpp` per una ragione misurata: la
// fase Blast e' la piu' grande del turno, e tenerla nello stesso file del resto dell'orchestrazione
// rendeva `ResolveCombat` una funzione da millesettecento righe dentro un file da seimila. Sono la
// STESSA classe — `ARTTurnManager` — e nessuna firma pubblica cambia: cambia dove il codice sta.
//
// L'ordine delle definizioni qui sotto e' quello in cui `ResolveCombat` le chiama, e non e' cosmetico:
// il catalogo assegna alle azioni un codice (20 movimento, 30 controllo, 40 attacco, 70 supporto) e la
// sequenza lo rispetta. Chi legge questo file dall'alto legge la fase nell'ordine in cui accade.
//
// Lo stato che i pass si passano sta in `FRTBlastContext` (`Turn/RTBlastContext.h`), dichiarato invece
// che implicito: prima erano cinquantotto variabili locali di una funzione sola, e nessuno poteva sapere
// quali un pass leggesse senza leggerli tutti.
// =================================================================================================

namespace
{
	/**
	 * L'azione E' quella core indicata, direttamente o per derivazione?
	 *
	 * 🔴 **`ActionId` da solo non basta, e non e' un dettaglio** (`#1443`). `MakeEquipmentAction` riscrive
	 * `ActionId` con l'id del PEZZO — «nel TurnLog si legge il gadget, non l'azione generica» — e conserva
	 * la provenienza in `DerivedFromActionId` ([D-195]); le abilita' d'eroe costruite da un'azione core
	 * fanno lo stesso. Chi confronta il solo `ActionId` letterale non riconosce ne' l'una ne' l'altra.
	 *
	 * L'effetto misurato: `Gadget.Medkit` — che il catalogo dichiara «la versione portatile di
	 * `Action.Heal`» — non passava dal percorso delle cure.
	 *
	 * ⚠️ **Non e' nel loadout di default**, contro quanto diceva la prima stesura di questo commento:
	 * `DefaultGadgetFor` assegna Insulator, Sprinkler, PortableCover e Sensor, e il medkit non e' fra
	 * quelli. Ci arriva uno scenario che lo dichiara. L'impatto e' quindi piu' piccolo di come l'avevo
	 * scritto — ma un gadget spedito che non fa cio' che dichiara resta un difetto, non una svista di
	 * priorita'. Finiva fra gli intenti d'attacco, `ValidateInstance` rispondeva `TargetFriendly`
	 * sull'alleato e il fallback lo annullava: **il medkit curava zero**, e la riga di log dava la colpa al
	 * bersaglio.
	 *
	 * ⚠️ Si guarda la derivazione, NON `BaseActionId`: quello dice di quale delle SETTE generiche un'azione
	 * e' il profilo ([D-033]), e `Heal`/`Cleanse`/`Interrupt` fra le sette non ci sono.
	 */
	bool IsCoreAction(const FRTActionDef& Def, const FName& Core)
	{
		return Def.ActionId == Core || Def.DerivedFromActionId == Core;
	}

	// ⚠️ Costruite UNA volta per il processo: `FName(TEXT("..."))` fa un hash case-insensitive piu' una
	// ricerca nella tabella globale sotto lock, e questo helper viene chiamato anche dentro il filtro
	// per-colpo di `ApplyInterrupts`.
	const FName ActionHeal(TEXT("Action.Heal"));
	const FName ActionCleanse(TEXT("Action.Cleanse"));
	const FName ActionInterrupt(TEXT("Action.Interrupt"));
	const FName ActionModifyArc(TEXT("Action.ModifyArc"));

	/**
	 * La voce `Fallback` di un'azione di supporto che non avviene: cambia solo il MOTIVO.
	 *
	 * Un builder invece di tre copie da nove campi: questo file porta gia' diversi costruttori quasi
	 * identici della stessa famiglia, e un campo aggiunto domani andrebbe ricordato in tutti. Stessa
	 * ragione per cui `#1415` ha collassato 49 builder di arena in uno.
	 */
	FRTTurnLogEntry MakeSupportFallback(const ARTUnit* Autore, const ARTUnit* Bersaglio,
		const FRTActionDef& Def, ERTActionInvalidReason Motivo)
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Fallback;
		Entry.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
		Entry.SrcCell = Autore ? Autore->Cell : FRTCellId();
		Entry.TgtCell = Bersaglio ? Bersaglio->Cell : Entry.SrcCell;
		Entry.Amount = static_cast<int32>(Motivo);
		Entry.ActionId = Def.ActionId;
		Entry.BaseActionId = Def.BaseActionId;
		Entry.Priority = Def.Priority;
		return Entry;
	}
}

void ARTTurnManager::GatherBlastUnits(FRTBlastContext& Ctx) const
{
	// Mappa ESAGONALE autorevole: portata (distanza esagonale) e linea di tiro si valutano qui.
	// Il terreno quadrato (Altura, incendio) non entra piu' nel Blast: l'ambiente attivo su hex e' l'epic E8.
	Ctx.Map = GetHexContext(Ctx.HexOrigin, Ctx.HexSize, Ctx.HexLayerH);

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	Ctx.Units.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Ctx.Units.Add(Unit);
		}
	}
	// Ordine STABILE per cella: GetAllActorsOfClass non e' ordinato, e da questo ordine dipendono gli indici
	// del piano, il TurnLog e la sequenza del playback. Una cella ospita al piu' un'unita' -> ordine totale.
	Ctx.Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	Ctx.States.Reserve(Ctx.Units.Num());
	Ctx.HexUnits.Reserve(Ctx.Units.Num());
	for (int32 i = 0; i < Ctx.Units.Num(); ++i)
	{
		ARTUnit* Unit = Ctx.Units[i];
		Ctx.IndexOf.Add(Unit, i);
		// [D-224] La quota TEMPORANEA viaggia con lo snapshot: senza, il resolver leggerebbe `TemporaryShield`
		// a zero e tratterebbe come BASE tutto lo scudo — cioe' sbaglierebbe l'unica domanda per cui il campo
		// esiste, «quanta protezione deve saltare il danno ambientale».
		Ctx.States.Add(FRTUnitCombatState(Unit->Health, Unit->Shield, Unit->GetTemporaryShield()));

		FRTHexCombatUnit HexUnit;
		HexUnit.UnitId = i; // identita' = indice (come FRTHexSnapshot::Units)
		HexUnit.TeamId = Unit->TeamId;
		HexUnit.Cell = Unit->Cell;
		HexUnit.bAlive = Unit->IsAlive();
		HexUnit.Facing = Unit->Facing; // CP 16.2: da che lato e' scoperta
		Ctx.HexUnits.Add(HexUnit);
	}
}

void ARTTurnManager::RefreshTeamKnowledgeForBlast(const FRTBlastContext& Ctx)
{
	// CONOSCENZA DI SQUADRA (CP 13.2), rinfrescata QUI e non a inizio turno: la posizione autorevole per il
	// Blast e' quella post-Dash, e osservare prima dello scatto darebbe una fotografia che nessuna fase usa.
	// Chi ha caricato in mezzo al campo si e' esposto, e la squadra avversaria deve saperlo prima di sparare.
	TSet<int32> Teams;
	for (const FRTHexCombatUnit& HU : Ctx.HexUnits) { Teams.Add(HU.TeamId); }
	TArray<int32> SortedTeams = Teams.Array();
	SortedTeams.Sort(); // l'ordine di un TSet dipende dall'hash: qui si itera, quindi si ordina

	TArray<FRTTeamKnowledge> Refreshed;
	for (int32 TeamId : SortedTeams)
	{
		TArray<FRTPerceiver> Observers;
		TArray<FRTLastKnownContact> EnemiesNow;
		for (int32 u = 0; u < Ctx.HexUnits.Num(); ++u)
		{
			if (!Ctx.HexUnits[u].bAlive) { continue; } // un cadavere non vede e non si nasconde
			if (Ctx.HexUnits[u].TeamId == TeamId)
			{
				FRTPerceiver P;
				P.Cell = Ctx.HexUnits[u].Cell;
				P.Facing = Ctx.HexUnits[u].Facing;
				P.VisionRange = Ctx.Units[u]->VisionRange;
				Observers.Add(P);
			}
			else
			{
				// Identita' STABILE, non l'indice `u`: questo array e' ordinato per cella e si rinumera
				// appena qualcuno si muove. `TurnNumber` in ingresso ignorato — lo scrive `Observe`, che
				// e' l'unica a sapere QUANDO l'avvistamento avviene.
				EnemiesNow.Add(FRTLastKnownContact(Ctx.Units[u]->StableUnitId, Ctx.HexUnits[u].Cell, /*ignorato*/ 0));
			}
		}
		Refreshed.Add(URTTeamKnowledgeLibrary::Observe(Ctx.Map, TeamId, TurnNumber, Observers, EnemiesNow,
			KnowledgeForTeam(TeamId)));
	}
	TeamKnowledgeState = MoveTemp(Refreshed);
	// Il secondo punto: una cella rivelata da un'esplosione compare a META' playback ([D-227]).
	OnTeamKnowledgeRefreshed.Broadcast(TurnNumber);
}

void ARTTurnManager::ResolveCleanseActions(FRTBlastContext& Ctx)
{
	// `Action.Cleanse` (CP 5.2): azione PRINCIPALE, non una reazione, e l'unica del Blast che agisce su CHI LA
	// USA invece che su un bersaglio. Risolve PRIMA del ciclo degli intenti, per due motivi indipendenti:
	//
	// 1. quel ciclo CONSUMA `PlannedAbilityIndex` (lo azzera appena letto, per ogni unita'): un pass successivo
	//    non troverebbe piu' nulla da leggere — e' lo stesso tranello gia' incontrato con `Action.Interrupt`
	//    al CP 4.7;
	// 2. purificarsi da `Exposed` DOPO aver incassato il +5 che quello stato comporta non servirebbe a niente.
	//    Il catalogo le da' infatti codice 30 (controllo), non 40 (attacco): il controllo viene prima del danno.
	//
	// QUALE stato togliere lo dice il PIANO (`PlannedCleansePriority`), mai il resolver: si scorre la lista
	// dichiarata e si rimuove il primo stato effettivamente presente, uno solo. Lista vuota -> nessuna
	// rimozione (fail-closed): indovinare per conto del giocatore e' esattamente cio' che il catalogo vieta.
	//
	// Limite noto: non puo' togliere uno stato applicato da un'azione di controllo dello STESSO Blast (quelli
	// si applicano a fine fase, insieme agli altri effetti dei colpi). Purifica cio' che c'era a inizio turno.
	for (int32 i = 0; i < Ctx.Units.Num(); ++i)
	{
		ARTUnit* Unit = Ctx.Units[i];
		const int32 CleanseIdx = Unit->PlannedAbilityIndex;
		const URTActionData* Cleanse = Unit->GetAbility(CleanseIdx);
		// Stessa guardia della cura, e per la stessa ragione: un cadavere non purifica, e soprattutto non
		// lascia una voce nell'hash del replay.
		if (!Unit->IsAlive())
		{
			continue;
		}

		if (!Cleanse || !IsCoreAction(Cleanse->Def, ActionCleanse) || !Unit->CanUseAbility(CleanseIdx))
		{
			continue;
		}

		FGameplayTag Removed;
		for (const FGameplayTag& Candidate : Unit->PlannedCleansePriority)
		{
			if (Unit->RemoveStatus(Candidate))
			{
				Removed = Candidate;

				// `Cleansed` e non `Extinguished`: qualcuno ha **speso un'azione** per togliere questo stato, e
				// il replay deve poter distinguere una purificazione voluta da un fuoco spento dall'acqua in cui
				// si e' finiti (`#1314`).
				FRTTurnLogEntry Purificato =
					MakeStatusDeathEntry(Candidate, Unit->Cell, ERTStatusOutcome::Cleansed);
				AppendLogEntry(Purificato, Unit);
				break; // UNO solo: e' il vincolo del catalogo, non un'ottimizzazione
			}
		}

		Ctx.MarkAbilitySpent(Unit, CleanseIdx); // parte qui, si paga in `SpendStartedAbilities` (`#1451`)
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumata qui: non deve diventare anche un intento d'attacco
		Unit->PlannedAttackTarget = nullptr;

		// 🔴 Una purificazione che non purifica **non sparisce in silenzio** ([D-196], `#1437`): e' lo stesso
		// difetto della cura senza effetti sessanta righe piu' sotto, nella stessa funzione — l'azione e' gia'
		// stata ANNOTATA come partita, quindi il cooldown verra' scritto comunque, e il record autoritativo
		// non conteneva niente. Chiuderne uno e lasciare il gemello lascerebbe la classe mezza aperta.
		//
		// ⚠️ Fino a `#1451` questa riga diceva «ci si arriva dopo `ConsumeAbility`, quindi il cooldown e' gia'
		// bruciato»: vero allora, falso da quando il pagamento e' in `SpendStartedAbilities`. Il MOTIVO per cui
		// la voce serve non cambia — l'abilita' risultera' comunque spesa — ma la premessa si', e questo file
		// tratta i commenti come specifica.
		if (!Removed.IsValid())
		{
			FRTTurnLogEntry PurgaVuota = MakeSupportFallback(
				Unit, Unit, Cleanse->Def, ERTActionInvalidReason::NoEffect);
			AppendLogEntry(PurgaVuota, Unit);
		}

		AddLogEvent(Removed.IsValid()
			? FString::Printf(TEXT("%s: purificato %s"), *Unit->GetName(), *Removed.ToString())
			: FString::Printf(TEXT("%s: nessuno stato da purificare"), *Unit->GetName()), FRTLogSubject::Unit(Unit));
	}
}

void ARTTurnManager::CollectHealActions(FRTBlastContext& Ctx)
{
	// `Action.Heal` (CP 8.5): azione di SUPPORTO, non un colpo. Si raccoglie qui, prima del ciclo degli
	// intenti — che consuma `PlannedAbilityIndex` e costruirebbe un intento d'attacco su un alleato — e si
	// applica DOPO i danni, piu' sotto: la priorita' 70 del catalogo la mette dopo gli attacchi (50-65),
	// quindi cura le ferite di questo turno, non quelle del turno prima.
	for (int32 i = 0; i < Ctx.Units.Num(); ++i)
	{
		ARTUnit* Unit = Ctx.Units[i];
		const int32 HealIdx = Unit->PlannedAbilityIndex;
		const URTActionData* Heal = Unit->GetAbility(HealIdx);
		// ⚠️ Un'unita' UCCISA nella fase Dash arriva qui col piano ancora addosso — `GatherBlastUnits` non
		// filtra i morti — e senza questa guardia un cadavere consuma l'abilita' e lascia le sue voci nel
		// TurnLog: deterministiche, ma rumore che entra nell'hash del replay. E' la stessa ragione per cui
		// `CollectAttackIntents` controlla `IsAlive()`.
		//
		// ⚠️ **Prima di tutto**, non a meta' funzione: la prima stesura di `#1437` la metteva dopo il
		// controllo di portata, che e' gia' un punto che scrive una voce. Trovato in code review.
		if (!Unit->IsAlive())
		{
			continue;
		}

		if (!Heal || !IsCoreAction(Heal->Def, ActionHeal) || !Unit->CanUseAbility(HealIdx))
		{
			continue;
		}

		// Bersaglio: chi e' stato scelto in pianificazione, oppure SE STESSI se non c'e' nessuno — il catalogo
		// dichiara che la cura «puo' bersagliare se stessi», e curare a vuoto non e' un'alternativa sensata.
		ARTUnit* HealTarget = Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget.Get() : Unit;
		// Il PIANO si azzera qui, il COOLDOWN piu' sotto (`#1445`, [D-200]): l'unita' ha speso il suo turno —
		// non puo' riagire — ma l'abilita' si paga solo se l'azione e' PARTITA. Azzerare il piano piu' in
		// basso lascerebbe il ciclo degli intenti costruire un attacco su un alleato, che e' la ragione per
		// cui questa raccolta viene prima.
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedAttackTarget = nullptr;

		// Portata dal catalogo, misurata come per ogni altra azione: una cura a distanza infinita sarebbe una
		// regola diversa da quella scritta.
		if (URTHexLibrary::HexDistance(Unit->Cell, HealTarget->Cell) > Heal->Def.RangeCells)
		{
			// 🔴 **L'asimmetria INVERSA** ([D-196], `#1412` punto 4): fino a qui questa cura mancata viveva
			// SOLO nel combat log. Il record autoritativo non la conteneva, quindi un replay non poteva
			// riprodurla e un rapporto di divergenza non poteva spiegare perche' l'alleato non fosse stato
			// curato. La superficie leggibile sapeva qualcosa che la traccia non registrava — il verso
			// opposto dei duplicati, e quello piu' difficile da vedere: non c'e' una riga di troppo, ce n'e'
			// una che non c'e'.
			// 🔴 **E il cooldown NON si paga** (`#1445`, [D-200]). Fino al 2026-08-27 si arrivava qui con
			// l'abilita' gia' bruciata, mentre `ModifyArc` — l'altra azione di supporto di questo stesso file —
			// dichiarava la regola opposta a centoventi righe di distanza: «il cooldown paga solo cio' che ha
			// davvero toccato la mappa». Due azioni di supporto, due regole, nessuna delle due dichiarata.
			//
			// ⚠️ **La portata decide se l'azione PARTE**, e non e' una sfumatura: e' l'unico dei quattro modi
			// di fallire che si conosce in pianificazione. Il bersaglio caduto nella simultaneita'
			// (`TargetDead`, [D-197]) e la def senza effetto utile (`NoEffect`) sono ESITI di un'azione
			// partita, e restano a carico. Un esito si paga; una mira impossibile no.
			FRTTurnLogEntry CuraMancata = MakeSupportFallback(
				Unit, HealTarget, Heal->Def, ERTActionInvalidReason::OutOfRange);
			AppendLogEntry(CuraMancata, Unit);
			// ⛔ **Niente `AddLogEvent`**: `ConcludeTurn` deriva una riga per ogni voce di TurnLog, quindi
			// tenerla avrebbe creato un duplicato nuovo. La riga derivata porta azione, motivo e celle; il
			// nome dell'unita' e' il debito noto di `#1412` punto 2.
			continue;
		}

		// 🔴 **Qui l'azione e' PARTITA** ([D-200]): il bersaglio e' raggiungibile, e da questo punto in poi
		// qualunque cosa vada storta e' un esito. Il cooldown si paga, e resta pagato anche se la
		// simultaneita' disfa la cura piu' tardi — il bersaglio che cade nello stesso Blast, [D-197].
		Ctx.MarkAbilitySpent(Unit, HealIdx); // parte qui, si paga in `SpendStartedAbilities` (`#1451`)

		int32 Amount = 0;
		for (const FRTActionEffectSpec& Spec : Heal->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Heal) { Amount = Spec.Amount; break; }
		}
		// 🔴 Una cura che non cura **non sparisce in silenzio** (`#1437`). Ci si arriva DOPO l'ANNOTAZIONE
		// — il cooldown lo scrivera' `SpendStartedAbilities` a fase finita (`#1451`; fino ad allora questa
		// riga diceva «gia' bruciato», che era vero e non lo e' piu') — e dopo che il piano e' azzerato: senza
		// questa voce non restava niente, ne' nel TurnLog ne' nel combat log, e un replay non poteva
		// spiegare perche' il turno del curatore non avesse prodotto nulla e perche' l'abilita' fosse in
		// ricarica. Strettamente peggio del caso «fuori portata» qui sopra, che almeno una riga ce l'aveva.
		//
		// Ci si arriva con un'abilita' il cui `ActionId` e' letteralmente `Action.Heal` e i cui `Effects` non
		// portano un `Heal` utile: un data asset scritto male, o un catalogo modificato.
		//
		// ⚠️ Da `#1443` ci arriva **anche** un equipaggiamento: il filtro guarda `DerivedFromActionId`, non
		// il solo `ActionId`. Un gadget i cui `GrantedEffects` sostituissero la cura senza metterne una —
		// come `Gadget.BreachCharge` fa con `Action.HeavyAttack` — finirebbe qui.
		//
		// ⚠️ Il motivo e' `NoEffect`, aggiunto per questo: `None` significa «l'azione e' eseguibile», e la
		// resa generica direbbe «non eseguibile» — falso in tutti e due i versi. L'azione era valida e non
		// aveva niente da applicare.
		//
		// ⚠️ **E il cooldown resta pagato**, a differenza del caso «fuori portata» ([D-200]): l'azione era
		// valida, e' partita e ha raggiunto il bersaglio. Che la def non portasse un `Heal` utile e' un
		// difetto del DATO, non una mira impossibile — e un catalogo rotto non e' una leva di bilanciamento
		// su cui valga la pena costruire un'eccezione.
		if (Amount <= 0)
		{
			FRTTurnLogEntry CuraVuota = MakeSupportFallback(
				Unit, HealTarget, Heal->Def, ERTActionInvalidReason::NoEffect);
			AppendLogEntry(CuraVuota, Unit);
			continue;
		}

		// Chi cura, accanto a da-dove: la cella del curatore non identifica un'unita' ([D-063]), e il TurnLog
		// deve dire chi ha agito (#405). `AddHeal` tiene allineati i quattro array paralleli.
		Ctx.AddHeal(Unit, HealTarget, Amount, Unit->Cell, Heal->Def);
	}
}

void ARTTurnManager::CollectAttackIntents(FRTBlastContext& Ctx)
{
	// Il corpo lavora sui campi del contesto con i nomi che avevano da variabili locali: sono RIFERIMENTI,
	// non copie, e tenerli evita di riscrivere duecento righe di regole per cambiarne il confine.
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TMap<ARTUnit*, int32>& IndexOf = Ctx.IndexOf;
	TArray<FRTHexCombatUnit>& HexUnits = Ctx.HexUnits;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	TArray<int32>& IntentAbilityIndex = Ctx.IntentAbilityIndex;
	TArray<const URTActionData*>& IntentAbility = Ctx.IntentAbility;
	TArray<FRTActionDef>& IntentDefs = Ctx.IntentDefs;
	TArray<FRTPendingArcOp>& PendingArcOps = Ctx.PendingArcOps;

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];

		// Le azioni AMBIENTALI (fase `Environment`) risolvono nel Cleanup, non qui: il loro piano deve
		// sopravvivere a questo ciclo. E' lo stesso tranello gia' incontrato con `Action.Interrupt` (CP 4.7)
		// e `Action.Cleanse` (CP 5.2) — questo ciclo AZZERA `PlannedAbilityIndex` per ogni unita', quindi un
		// pass successivo non troverebbe piu' nulla da leggere.
		const URTActionData* PlannedNow = Unit->GetAbility(Unit->PlannedAbilityIndex);
		if (PlannedNow
			&& URTCatalogLibrary::MapResolutionPhase(PlannedNow->Def.ResolutionPhase) == ERTMatchPhase::Cleanup)
		{
			continue; // il piano resta: lo consuma `ResolveEnvironment`
		}

		// `Action.ModifyArc` (CP 9.4) risolve QUI, ma non e' un intento d'attacco: si intercetta PRIMA della
		// raccolta perche' il percorso normale leggerebbe il danno dagli effetti e, non trovandone, ripiegherebbe
		// sul campo legacy `Ability->Power` — un'azione che cambia la topologia si metterebbe a fare danno.
		//
		// L'arco e' identificato dalla COPPIA (chi la usa, il bersaglio): la pianificazione non ha un
		// bersaglio-arco, e questo resta un limite dichiarato finche' l'HUD di E11 non ne porta uno.
		if (PlannedNow && IsCoreAction(PlannedNow->Def, ActionModifyArc))
		{
			ARTUnit* ArcTarget = Unit->PlannedAttackTarget;
			const int32 ArcAbilityIndex = Unit->PlannedAbilityIndex;
			Unit->PlannedAttackTarget = nullptr;
			Unit->PlannedAbilityIndex = INDEX_NONE; // consumato nel turno, attivata o no
			if (Unit->CanUseAbility(ArcAbilityIndex) && ArcTarget && ArcTarget->IsAlive())
			{
				// La PORTATA dichiarata dal catalogo si valida qui, prima di toccare la topologia (#206).
				// `ModifyArc` non passa da `ValidateInstance` — si intercetta prima della raccolta degli
				// intenti, due righe sopra — quindi il controllo che ogni altra azione del Blast riceve
				// gratis va scritto: senza, un'azione che dichiara `Range 3` opera dall'altra parte della
				// mappa, e non e' un'azione a portata, e' un'azione senza portata.
				if (URTHexLibrary::HexDistance(Unit->Cell, ArcTarget->Cell) > PlannedNow->Def.RangeCells)
				{
					// Il fallback dichiarato a catalogo e' `Cancel`: nessun effetto, ma VISIBILE. Un'azione
					// che sparisce in silenzio e' indistinguibile da un difetto — la stessa ragione per cui
					// esiste `CoverRejected`. Il motivo viaggia in `Amount` come per ogni altro fallback.
					FRTTurnLogEntry ArcRejected;
					ArcRejected.Phase = ERTMatchPhase::Blast;
					ArcRejected.Category = ERTLogCategory::Fallback;
					ArcRejected.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
					// La tripla completa, come gli altri produttori `Fallback` di questo file ([D-196]).
					ArcRejected.ActionId = PlannedNow->Def.ActionId;
					ArcRejected.BaseActionId = PlannedNow->Def.BaseActionId;
					ArcRejected.Priority = PlannedNow->Def.Priority;
					ArcRejected.SrcCell = Unit->Cell;
					ArcRejected.TgtCell = ArcTarget->Cell;
					ArcRejected.Amount = static_cast<int32>(ERTActionInvalidReason::OutOfRange);
					AppendLogEntry(ArcRejected, Unit);
					// Stesso soggetto della voce: `ConcludeTurn` ne deriva una riga identica a questa, e
					// due soggetti diversi sulla stessa frase farebbero passare una copia e non l'altra.
					AddLogEvent(FString::Printf(TEXT("%s: %s"),
						*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(ArcRejected)), FRTLogSubject::Unit(Unit));

					// L'abilita' NON si consuma: il piano e' gia' stato azzerato sopra (si spende nel turno,
					// attivata o no), ma il cooldown paga solo cio' che ha davvero toccato la mappa.
					continue;
				}

				Ctx.MarkAbilitySpent(Unit, ArcAbilityIndex); // parte qui, si paga in `SpendStartedAbilities`
				PendingArcOps.Add({ Unit->Cell, ArcTarget->Cell, Unit, PlannedNow->Def });
			}
			continue;
		}

		ARTUnit* Target = Unit->PlannedAttackTarget;
		const int32 AbilityIndex = Unit->PlannedAbilityIndex;
		Unit->PlannedAttackTarget = nullptr; // consumati nel turno
		Unit->PlannedAbilityIndex = INDEX_NONE;

		const URTActionData* Ability = Unit->GetAbility(AbilityIndex);
		if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || !Unit->CanUseAbility(AbilityIndex))
		{
			continue; // nessuna azione di Blast pianificata: non c'e' un'azione da far fallire
		}

		// Chi usa un'azione principale che nega la reazione (CP 5.1: nessuna oggi, ma il dato e' generico)
		// non ne tiene pronta una in questo turno. `Action.Sprint` (l'unico caso reale) passa dallo scatto,
		// non da qui: e' `ResolveDash` a registrarlo, perche' risolve prima e consuma lo slot principale.
		if (!Ability->Def.bAllowsReaction)
		{
			ReactionBlockedThisTurn.Add(Unit);
		}

		// D-020: un'azione con BERSAGLIO orienta chi la usa PRIMA che risolva. Non e' una proprieta' dichiarata
		// per azione — e' la regola: si guarda cio' che si colpisce. Qui, e non nel Prep, perche' le azioni di
		// Prep del vertical slice agiscono su chi le usa (`Instance.TargetUnitId = i`) e non hanno un bersaglio
		// esterno verso cui girarsi.
		//
		// Conta la cella DICHIARATA nel piano, non il primo bersaglio che l'area colpira': l'orientamento e' una
		// scelta del giocatore, e un'area che prende tre unita' non deve farlo dipendere dall'ordine di calcolo.
		// `Unit->IsAlive()` e non solo il bersaglio: questo ciclo non filtra gli attaccanti morti (chi cade nel
		// Dash ci arriva col piano ancora addosso) e il loro colpo viene scartato piu' avanti, da
		// `CollectHexAttacks`, che salta le unita' non vive. Senza il guard un cadavere si girerebbe verso il
		// bersaglio e lascerebbe la sua voce nel TurnLog: deterministica, ma rumore che entra nell'hash del replay.
		if (Unit->IsAlive() && Target && Target->IsAlive() && Target != Unit)
		{
			ERTHexDirection TowardsTarget = Unit->Facing;
			if (URTHexLibrary::DirectionTowards(Unit->Cell, Target->Cell, TowardsTarget))
			{
				FRTHexSimUnit Attacker(i, Unit->Cell, /*InMoveBudget=*/ 0);
				Attacker.Facing = Unit->Facing;
				RecordFacingChange(Attacker, TowardsTarget, ERTFacingOutcome::TargetingReoriented,
					ERTMatchPhase::Blast, Unit);
				Unit->Facing = Attacker.Facing;

				// Anche nella copia del Blast: `HexUnits` e' stato costruito PRIMA di questo ciclo, e la difesa
				// direzionale (CP 16.2) legge di li'. Senza questa riga, due unita' che si attaccano a vicenda
				// nella stessa fase si girerebbero l'una verso l'altra e verrebbero comunque colpite alle spalle.
				if (HexUnits.IsValidIndex(i))
				{
					HexUnits[i].Facing = Unit->Facing;
				}
			}
		}

		// Da qui si ragiona su ISTANZE, non su puntatori: e' l'istanza che si valida e su cui si applica il
		// fallback DICHIARATO dall'azione, in un punto solo (spec E4 §D5).
		FRTActionInstance Instance;
		Instance.Def = Ability->Def;
		// Portata dell'istanza: dal catalogo se la dichiara, altrimenti dal portatore. `RangeCells <= 0` NON
		// vuol dire "nessuna portata" — e' la convenzione del catalogo per «portata dell'arma»
		// (`Action.PrecisionAttack`, «range dell'arma +1»; `Action.MarkTarget`).
		//
		// Prima il ponte valeva solo per le azioni SENZA `ActionId`, quindi quelle due si validavano con
		// portata 0 e degradavano sempre al proprio fallback, mentre `Intent.RangeCells` leggeva il campo
		// dell'asset: due verita' sulla stessa portata, e l'azione non arrivava mai a bersaglio.
		// Difetto trovato al CP 8.2 (issue #65) mentre si cablava `Status.Marked`.
		if (Instance.Def.ActionId.IsNone() || Instance.Def.RangeCells <= 0)
		{
			Instance.Def.RangeCells = Ability->RangeCells;
		}
		Instance.SourceUnitId = i;
		// Bersaglio a CELLA: nessuna unita' mirata per costruzione, non una che si e' persa. La distinzione
		// conta subito qui sotto, dove `TargetUnitId == INDEX_NONE` significa `TargetGone` e degraderebbe al
		// fallback un'azione che invece sta facendo esattamente cio' che le e' stato chiesto.
		const bool bTargetsCell = Unit->bAttackTargetsCell;
		Instance.TargetUnitId = (!bTargetsCell && Target && IndexOf.Contains(Target)) ? IndexOf[Target] : INDEX_NONE;
		Instance.TargetCell = bTargetsCell ? Unit->PlannedAttackCell : (Target ? Target->Cell : Unit->Cell);
		Instance.EventSequence = Intents.Num();

		// Un'azione di Blast senza bersaglio non e' un'azione «che non ne ha uno» (quelle sono il movimento e il
		// supporto su se stessi, e risolvono altrove): e' un'azione che il bersaglio l'ha PERSO — eliminato e
		// rimosso dal livello, o mai valido. Senza questa distinzione l'istanza risulterebbe valida e l'unita'
		// finirebbe per puntare la propria cella.
		ERTActionInvalidReason Reason = (Instance.TargetUnitId == INDEX_NONE && !bTargetsCell)
			? ERTActionInvalidReason::TargetGone
			: URTActionFallbackLibrary::ValidateInstance(Instance, HexUnits, Map);

		// CP 13.2 — IL TARGETING CONSUMA LA CONOSCENZA. Si valuta DOPO la geometria e solo se la geometria
		// regge, per la stessa ragione per cui `ValidateInstance` mette la portata prima della copertura: il
		// motivo scritto nel log dev'essere quello che chi gioca deve correggere. «Non lo vedi» detto a chi
		// era comunque fuori portata sposterebbe l'attenzione sul difetto sbagliato.
		if (Reason == ERTActionInvalidReason::None && Instance.TargetUnitId != INDEX_NONE
			&& HexUnits.IsValidIndex(Instance.TargetUnitId))
		{
			const FRTTeamKnowledge Knowledge = KnowledgeForTeam(Unit->TeamId);
			const int32 TargetId = Instance.TargetUnitId;          // indice di fase: serve solo qui e ora
			const int32 TargetStable = Units[TargetId]->StableUnitId; // identita' che la memoria usa
			switch (URTTeamKnowledgeLibrary::ClassifyTarget(Knowledge, TargetStable,
				HexUnits[TargetId].TeamId, HexUnits[TargetId].Cell))
			{
			case ERTTargetKnowledge::Allowed:
				break; // la squadra lo vede: si mira all'unita', come sempre

			case ERTTargetKnowledge::CellOnly:
			{
				// Contatto INCERTO: si colpisce la CELLA dell'ultimo contatto, mai l'unita'. La differenza non
				// e' formale — mirando all'unita' il colpo la seguirebbe dove si e' spostata, cioe' userebbe
				// una posizione che la squadra non conosce. Qui invece il colpo resta dov'era il ricordo, e se
				// il bersaglio si e' mosso trova terra battuta. E' il costo di sparare a memoria.
				FRTCellId Remembered;
				if (URTTeamKnowledgeLibrary::LastKnownCell(Knowledge, TargetStable, Remembered))
				{
					Instance.TargetUnitId = INDEX_NONE;
					Instance.TargetCell = Remembered;
					// Nessuna rivalidazione qui: `ValidateInstance` su un bersaglio-cella risponde sempre
					// `None` per costruzione. Portata e linea di tiro sulla cella ricordata le verifica
					// `CollectHexAttacks`, che e' l'owner della geometria dei colpi a cella.
				}
				else
				{
					Reason = ERTActionInvalidReason::TargetUnknown; // incerto senza ricordo: non e' un bersaglio
				}
				break;
			}

			default:
				Reason = ERTActionInvalidReason::TargetUnknown;
				break;
			}
		}

		// La copertura NON passa di qui: la registra il piano del Blast col suo reason code (NoLineOfSight), e
		// con la traiettoria bloccata nemmeno un'area potrebbe partire — applicarle `AttackCell` significherebbe
		// colpire attraverso il muro. Stesso discorso per la mappa assente, che e' un difetto del livello.
		const bool bHandledByPlan = Reason == ERTActionInvalidReason::NoLineOfSight
			|| Reason == ERTActionInvalidReason::NoMap;

		if (Reason != ERTActionInvalidReason::None && !bHandledByPlan)
		{
			const FRTFallbackResult Fallback = URTActionFallbackLibrary::ApplyFallback(Instance, Reason);

			// L'azione fallita non sparisce piu' in silenzio: cosa e' stato applicato e PERCHE' finiscono nel
			// TurnLog (categoria Fallback, motivo in Amount) e nel combat log.
			FRTTurnLogEntry FallbackEntry;
			FallbackEntry.Phase = ERTMatchPhase::Blast;
			FallbackEntry.Category = ERTLogCategory::Fallback;
			FallbackEntry.Outcome = static_cast<uint8>(URTActionFallbackLibrary::ToLogOutcome(Fallback.Applied));
			FallbackEntry.SrcCell = Unit->Cell;
			FallbackEntry.TgtCell = Instance.TargetCell;
			FallbackEntry.Amount = static_cast<int32>(Reason);
			// QUALE azione e' fallita ([D-196], `#1412`). Stesso modello della voce `NoLos` 350 righe piu'
			// sotto, e per la stessa ragione: un'azione che non avviene lascia SOLO questa voce, e senza
			// l'identita' non dice se a mancare sia stata l'ultimate o l'attacco base. Due azioni annullate
			// dalla stessa unita' nello stesso turno producevano righe identiche byte a byte.
			//
			// ⚠️ `Instance.Def` e' ancora quella ORIGINALE: `Instance` viene riassegnata all'istanza del
			// fallback solo dopo questo blocco. L'azione da nominare e' quella che e' fallita, non il suo
			// ripiego.
			//
			// ⚠️ `ActionId` **entra nell'hash**: e' un cambio d'identita' delle tracce archiviate, dichiarato
			// in [D-196] e pagato con la rigenerazione del corpus nella stessa PR.
			FallbackEntry.ActionId = Instance.Def.ActionId;
			FallbackEntry.BaseActionId = Instance.Def.BaseActionId;
			FallbackEntry.Priority = Instance.Def.Priority;
			AppendLogEntry(FallbackEntry, Unit);
			// Stesso soggetto della voce: vedi `ArcRejected` poco sopra — la copia derivata da
			// `ConcludeTurn` e questa devono passare o cadere insieme.
			AddLogEvent(FString::Printf(TEXT("%s: %s"),
				*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(FallbackEntry)), FRTLogSubject::Unit(Unit));

			if (!Fallback.bProducesEffects)
			{
				continue; // Cancel (e cio' che vi degrada): l'azione non avviene
			}
			Instance = Fallback.Instance; // AttackCell: si perde il bersaglio, resta la cella
		}

		FRTHexAttackIntent Intent;
		Intent.AttackerId = i;
		Intent.TargetId = Instance.TargetUnitId;
		Intent.TargetCell = Instance.TargetCell;
		Intent.Shape = Ability->Shape;
		Intent.RangeCells = Ability->RangeCells;
		Intent.AreaRadius = Ability->AreaRadius;
		// La friendly fire policy la DICHIARA l'azione (`Action.CircularAoE` la ha true): senza questa riga
		// l'intento nasceva sempre a false e nessuna area colpiva un alleato in partita, benche' il dato
		// esistesse nel catalogo e il resolver puro lo rispettasse. Difetto trovato al CP 8.2 e corretto qui.
		Intent.bFriendlyFire = Instance.Def.bFriendlyFire;
		// Stessa storia, stesso rimedio ([`INT-8`]): senza questa riga l'intento nascerebbe sempre a `false` e
		// NESSUN attacco produrrebbe un colpo, benche' il catalogo lo dichiari.
		Intent.bCountsAsAttack = Instance.Def.bCountsAsAttack;
		// Danno DICHIARATO dagli effetti dell'azione: e' il catalogo a dirlo. Il campo legacy `Power` resta
		// come ripiego per le abilita' non ancora catalogate (quelle generiche di EnsureDefaultAbilities):
		// finche' esistono, toglierlo del tutto trasformerebbe i loro colpi in danno zero.
		int32 DeclaredDamage = 0;
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage)
			{
				DeclaredDamage = Spec.Amount;
				break;
			}
		}
		Intent.Power = URTCombatLibrary::EffectiveAttackPower(
			DeclaredDamage > 0 ? DeclaredDamage : Ability->Power, /*OccupantDamageBonus=*/ 0);

		// Danno alle STRUTTURE (CP 9.2): scala distinta, dichiarata dall'azione. Nessun ripiego sul `Power`
		// legacy — un'abilita' che non lo dichiara non sfonda, e va bene cosi': sfondare e' una capacita' che
		// il catalogo concede, non un effetto collaterale di essere forti.
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::DamageStructure)
			{
				Intent.StructurePower = FMath::Max(0, Spec.Amount);
				break;
			}
		}

		// PORTE (CP 9.3): lo stato viaggia in `Amount` (interi soltanto, come la durata di uno stato). Un
		// valore fuori intervallo non produce nessun ordine — meglio nessuna operazione che una porta portata
		// a uno stato che non esiste.
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect != ERTActionEffect::SetDoorState)
			{
				continue;
			}
			if (Spec.Amount >= 0 && Spec.Amount <= static_cast<int32>(ERTHexDoorState::Destroyed))
			{
				Intent.bChangesDoor = true;
				Intent.DoorState = static_cast<ERTHexDoorState>(Spec.Amount);

				// Il bordo che il giocatore ha CLICCATO, se l'ha dichiarato (CP 10.1, `#74`). Lo stesso campo
				// del piano che il ramo delle coperture legge da sempre: qui non arrivava, e l'operazione
				// finiva su un bordo dedotto dalla traiettoria — un bordo che nessuno aveva scelto ([D-149]).
				Intent.bHasDeclaredDoorEdge = Unit->bHasPlannedCoverEdge;
				Intent.DeclaredDoorEdge = Unit->PlannedCoverEdge;
			}
			break;
		}
		Intents.Add(Intent);
		IntentAbilityIndex.Add(AbilityIndex);
		IntentAbility.Add(Ability);
		IntentDefs.Add(Instance.Def);
	}
}

void ARTTurnManager::AppendChargeImpactIntents(FRTBlastContext& Ctx)
{
	TMap<ARTUnit*, int32>& IndexOf = Ctx.IndexOf;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	TArray<int32>& IntentAbilityIndex = Ctx.IntentAbilityIndex;
	TArray<const URTActionData*>& IntentAbility = Ctx.IntentAbility;
	TArray<FRTActionDef>& IntentDefs = Ctx.IntentDefs;

	// Impatti delle cariche risolte nella fase Dash: entrano nel Blast come intenti a portata 1, cioe' addosso
	// al bersaglio. E' il codice 20/30 del catalogo — il movimento e' avvenuto prima, il colpo risolve qui, con
	// gli altri, per priorita'. Applicarlo dentro la fase Dash lo avrebbe messo fuori dall'ordine.
	for (const FRTChargeImpact& Impact : PendingChargeImpacts)
	{
		ARTUnit* Attacker = Impact.Attacker.Get();
		ARTUnit* Victim = Impact.Target.Get();
		if (!Attacker || !Victim || !IndexOf.Contains(Attacker) || !IndexOf.Contains(Victim)) { continue; }

		FRTHexAttackIntent Intent;
		Intent.AttackerId = IndexOf[Attacker];
		Intent.TargetId = IndexOf[Victim];
		Intent.TargetCell = Victim->Cell;
		Intent.Shape = ERTAbilityShape::Single;
		Intent.RangeCells = 1; // dopo l'impatto si e' adiacenti: e' questa la portata del colpo
		Intent.AreaRadius = 0;
		// [`INT-8`]: anche qui il colpo si dichiara, e si legge dall'azione che l'ha prodotto invece di darlo
		// per scontato. E' il SECONDO punto in cui nasce un intento -- l'impatto della carica non passa dal
		// ciclo dei piani -- quindi la propagazione va ripetuta, o gli impatti smetterebbero di colpire mentre
		// tutto il resto funziona.
		Intent.bCountsAsAttack = Impact.Def.bCountsAsAttack;

		const int32 ImpactDamage = URTCatalogLibrary::FirstDamage(Impact.Def);
		Intent.Power = URTCombatLibrary::EffectiveAttackPower(ImpactDamage, /*OccupantDamageBonus=*/ 0);

		Intents.Add(Intent);
		IntentAbilityIndex.Add(INDEX_NONE); // nessuna abilita' da consumare: lo scatto l'ha gia' fatto
		IntentAbility.Add(nullptr);
		IntentDefs.Add(Impact.Def);
	}
	PendingChargeImpacts.Reset();
}

void ARTTurnManager::ApplyInterrupts(FRTBlastContext& Ctx)
{
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	TArray<FRTActionDef>& IntentDefs = Ctx.IntentDefs;
	FRTHexBlastPlan& Plan = Ctx.Plan;

	// `Action.Interrupt` (CP 4.7): cancella l'INTERA azione di un'altra unita', non un effetto su un
	// bersaglio — per questo si filtra QUI, sui colpi gia' raccolti, invece di passare dal registry
	// (`URTActionEffectLibrary::ProduceEvents`), che sa tradurre effetti su un bersaglio ma non "annulla
	// l'azione X". Filtrare `Plan.Hits` prima che diventino danno o eventi cancella ENTRAMBI in un colpo solo
	// — anche per un'abilita' ad area che avrebbe prodotto piu' Hit dallo stesso attaccante.
	//
	// L'Interrupt stesso passa dalla normale validazione di bersaglio/portata/linea di tiro di
	// CollectHexAttacks (e' un intento come un altro, portata 1): un Interrupt senza linea di tiro sul
	// bersaglio non produce un Hit, quindi non cancella nulla, esattamente come un attacco bloccato dalla
	// copertura.
	// 🔴 **Si tiene traccia degli INTENTI cancellati, non delle unita'** (`#1437`). Un'unita' puo' possedere
	// piu' di un intento nello stesso turno — `AppendChargeImpactIntents` ne aggiunge uno per l'impatto di
	// una carica — e la versione precedente ne cercava UNO solo, col `break` al primo trovato, per poi
	// cancellare col filtro **tutti** i colpi di quell'attaccante. Ne seguivano due esiti sbagliati e
	// simmetrici:
	//
	// - primo intento non interrompibile e secondo si': la guardia falliva sul primo e **non si interrompeva
	//   niente**, mentre un'azione interrompibile c'era;
	// - primo interrompibile e secondo no: si cancellavano **entrambi**, incluso quello che dichiara di non
	//   poter essere interrotto, e la traccia ne nominava uno solo.
	//
	// Con gli indici degli intenti la domanda si fa per azione, che e' l'unita' su cui `InterruptPolicy`
	// e' dichiarato — e la deduplicazione viene gratis: due Interrupt sulla stessa vittima aggiungono lo
	// stesso indice al `TSet` e producono una voce sola.
	// 🔴 **PASSATA 1 di 3 — si costruisce il GRAFO, non si cancella ancora niente** (`#1451`, [D-202]).
	//
	// La versione precedente decideva e applicava nello stesso ciclo, e la guardia «un Interrupt gia'
	// cancellato non interrompe» (`#1437`) funzionava quindi **solo in una delle due direzioni**. `Plan.Hits`
	// e' ordinato per `AttackerId`, quindi con A che interrompe B e B che interrompe C:
	//
	// - indice di A **minore** di quello di B: si vede `Hit(A→B)` per primo, l'intento di B entra
	//   nell'insieme, e `Hit(B→C)` viene saltato. L'azione di C sopravvive. ✅
	// - indice **maggiore**: si vede `Hit(B→C)` per primo — l'azione di C e' gia' cancellata — e solo dopo
	//   si scopre che B era a sua volta interrotto. ❌
	//
	// «E' come se non fosse mai partita» valeva o non valeva a seconda dell'ORDINE DI SPAWN delle unita'.
	// E da `#1449` c'era un secondo verso: B non paga (e' cancellato), quindi C perdeva la propria azione
	// per un Interrupt annullato **e** gratuito.
	//
	// Gli archi si tengono in due array PARALLELI e non in una `TMap`: iterare una `TMap` non ha ordine
	// dichiarato, e qui l'ordine decide cosa la traccia archiviata contiene.
	TArray<int32> Interruttori;            // intenti Interrupt che cancellerebbero qualcosa, crescenti
	TArray<TArray<int32>> Cancellerebbe;   // parallelo: gli intenti che ciascuno annullerebbe

	auto ArcoDa = [&Interruttori, &Cancellerebbe](int32 IntentoInterrupt) -> TArray<int32>&
	{
		const int32 Trovato = Interruttori.Find(IntentoInterrupt);
		if (Trovato != INDEX_NONE) { return Cancellerebbe[Trovato]; }
		Interruttori.Add(IntentoInterrupt);
		return Cancellerebbe.AddDefaulted_GetRef();
	};

	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex)
			|| !IsCoreAction(IntentDefs[Hit.IntentIndex], ActionInterrupt))
		{
			continue;
		}
		if (!Units.IsValidIndex(Hit.TargetId) || !Units[Hit.TargetId]) { continue; }

		// Le azioni pianificate dal BERSAGLIO non si leggono da `Unit->PlannedAbilityIndex`: il ciclo che ha
		// costruito `Intents`, qualche riga sopra, le ha gia' CONSUMATE (azzerate) per ogni unita', bersaglio
		// compreso — e' cosi' che il turno evita di rieseguire due volte la stessa azione. Vanno cercate fra
		// gli `Intents` gia' catturati, nelle entrate che il bersaglio ha prodotto per SE STESSO
		// (`AttackerId` == l'indice del bersaglio dell'Interrupt): e' li' che le definizioni originali
		// sopravvivono al reset.
		for (int32 k = 0; k < Intents.Num(); ++k)
		{
			if (Intents[k].AttackerId != Hit.TargetId) { continue; }

			// 🔴 **L'impatto di una carica E' raggiungibile dall'Interrupt, e quel giorno e' arrivato**
			// ([D-300], `#1955`). Fino a qui era immune, con una ragione scritta e buona: *«cancellarlo
			// annullerebbe a posteriori la coda di un'azione risolta a meta', lasciando l'unita' dove la
			// carica l'ha portata e togliendole il colpo»*, e il commento chiudeva con *«se un giorno si
			// vorra' [...] e' una scelta di bilanciamento da dichiarare, non l'effetto di un ciclo»*.
			//
			// D-300 e' quella scelta, e **scioglie l'obiezione invece di ignorarla**: `Action.Charge`
			// dichiara `ERTInterruptPolicy::SuppressSecondary`, quindi l'impatto non viene **cancellato** ma
			// **degradato** — il colpo resta, cade la spinta. La coda di un'azione risolta a meta' non
			// sparisce piu' a posteriori: perde la sua parte accessoria.
			//
			// Gli impatti si riconoscono ancora da `IntentAbilityIndex == INDEX_NONE`, che
			// `AppendChargeImpactIntents` scrive perche' non c'e' un'abilita' da consumare. Il valore serve
			// ancora, ma **piu' avanti** (il pagamento del cooldown, che un impatto non deve): qui non filtra
			// piu' niente.

			// Solo cio' che DICHIARA di poter essere interrotto: un Interrupt su chi ha pianificato Guard
			// (`ERTInterruptPolicy::None`) non ha niente da cancellare.
			if (!IntentDefs.IsValidIndex(k) || !IntentDefs[k].CanBeInterrupted()) { continue; }

			// L'arco: questo Interrupt cancellerebbe questa azione. **Se** sara' efficace lo decide la
			// passata 2 — qui non si cancella niente, e i colpi restano tutti al loro posto.
			ArcoDa(Hit.IntentIndex).AddUnique(k);
		}
	}

	// 🔴 **PASSATA 2 di 3 — chi cancella davvero**, a punto fisso ([D-202]).
	//
	// Un Interrupt e' **efficace** se nessun Interrupt efficace lo cancella. Si stratifica dalla radice: chi
	// non e' bersaglio di nessun Interrupt e' efficace subito, e da li' si propaga. La catena A→B→C si
	// risolve allo stesso modo qualunque sia l'ordine degli indici — A e' alla radice, quindi B cade e C
	// sopravvive, sempre.
	//
	// ⚠️ **Un CICLO non ha radice, e resta indeciso**: due unita' adiacenti che si interrompono a vicenda
	// — `Action.Interrupt` ha portata 1, quindi e' del tutto ordinario — non raggiungono mai un livello.
	// Restano **inefficaci**: nessuno dei due cancella, ed entrambe le azioni originali procedono. E' la
	// lettura letterale di `#1437` («un Interrupt cancellato non interrompe») applicata a entrambi
	// insieme, invece che al primo che l'ordine degli indici incontrava.
	//
	// ⚠️ **Pagano lo stesso**: hanno prodotto un colpo, quindi la regola di `#1449` li raggiunge piu'
	// sotto. Si sono neutralizzati, non hanno rinunciato.
	enum class EStato : uint8 { Indeciso, Efficace, Cancellato };
	TArray<EStato> Stato;
	Stato.Init(EStato::Indeciso, Interruttori.Num());

	// Gli Interrupt che minacciano un dato intento: si cerca fra gli archi, senza `TMap`.
	auto MinacceContro = [&Interruttori, &Cancellerebbe](int32 Intento, TArray<int32>& Out)
	{
		Out.Reset();
		for (int32 i = 0; i < Interruttori.Num(); ++i)
		{
			if (Cancellerebbe[i].Contains(Intento)) { Out.Add(i); }
		}
	};

	TArray<int32> Minacce;
	bool bProgresso = true;
	while (bProgresso)
	{
		bProgresso = false;
		for (int32 i = 0; i < Interruttori.Num(); ++i)
		{
			if (Stato[i] != EStato::Indeciso) { continue; }

			MinacceContro(Interruttori[i], Minacce);
			bool bCancellatoDaEfficace = false;
			bool bQualcunoIndeciso = false;
			for (int32 j : Minacce)
			{
				if (Stato[j] == EStato::Efficace) { bCancellatoDaEfficace = true; break; }
				if (Stato[j] == EStato::Indeciso) { bQualcunoIndeciso = true; }
			}

			if (bCancellatoDaEfficace)
			{
				Stato[i] = EStato::Cancellato;
				bProgresso = true;
			}
			else if (!bQualcunoIndeciso)
			{
				// Nessuna minaccia viva: o non ne aveva, o tutte sono state cancellate a loro volta.
				Stato[i] = EStato::Efficace;
				bProgresso = true;
			}
		}
	}

	// Cio' che gli Interrupt EFFICACI tolgono. Gli indecisi — i cicli — non contribuiscono.
	//
	// 🔴 **Due insiemi e non uno, da [D-300]**: `SuppressSecondary` non cancella l'azione, ne toglie gli
	// effetti oltre il primo. Tenerli separati non e' pulizia — e' cio' che rende corretto tutto il resto
	// del pass, perche' `InterruptedIntents` alimenta `RemoveAll` (il colpo sparisce) e la voce `Cancelled`
	// del TurnLog (l'azione e' annullata). Un intento degradato non deve entrare in nessuna delle due:
	// il suo colpo resta nel piano e la sua azione e' avvenuta.
	//
	// ⚠️ **E la propagazione a punto fisso non cambia, perche' degradare non e' cancellare**: [D-202]
	// definisce efficace come *«nessun Interrupt efficace lo cancella»*, quindi un Interrupt che viene
	// soltanto degradato resta efficace e continua a togliere ai propri bersagli. Per questo la lettura di
	// `Stato[]` qui sopra e' invariata: la distinzione nasce **dopo** che l'efficacia e' decisa.
	TSet<int32> InterruptedIntents;   // cancellati: il colpo sparisce, l'azione e' annullata
	TSet<int32> DegradedIntents;      // degradati: il colpo resta, cadono gli effetti oltre il primo
	for (int32 i = 0; i < Interruttori.Num(); ++i)
	{
		if (Stato[i] != EStato::Efficace) { continue; }
		for (int32 Bersaglio : Cancellerebbe[i])
		{
			// La policy la dichiara la VITTIMA, come ogni altra proprieta' di interrompibilita': un
			// `Action.Interrupt` non porta con se' alcun flag.
			const bool bDegrada = IntentDefs.IsValidIndex(Bersaglio)
				&& IntentDefs[Bersaglio].InterruptPolicy == ERTInterruptPolicy::SuppressSecondary;
			if (bDegrada) { DegradedIntents.Add(Bersaglio); }
			else { InterruptedIntents.Add(Bersaglio); }
		}
	}
	// Il contesto lo porta fino a dove `FRTActionInstance` si costruisce: e' li' che `bInterrupted` diventa
	// vero e `ProduceEvents` taglia la lista. Senza questo trasporto la policy sarebbe dichiarata e mai
	// applicata ([D-207]).
	Ctx.DegradedIntents = DegradedIntents;

	// 🔴 **Chi si e' neutralizzato lascia traccia** (`#1460`, [D-203]).
	//
	// Un Interrupt rimasto `Indeciso` e' in un ciclo: ha speso l'azione, ha raggiunto il bersaglio e non ha
	// cancellato niente, perche' cio' che stava annullando stava annullando lui. Senza questa voce quel turno
	// non lasciava **nessuna** traccia autoritativa — due unita' che pagano un cooldown e producono zero
	// voci — ed e' la classe che [D-196] ha chiuso quattro volte: *non una riga di troppo, una che non c'e'*.
	//
	// ⚠️ Si scrive PRIMA delle cancellazioni e in ordine d'indice, come quelle: l'ordine di emissione non
	// conta per l'hash — il TurnLog si ordina canonicamente — ma conta per chi legge il log a schermo.
	for (int32 i = 0; i < Interruttori.Num(); ++i)
	{
		if (Stato[i] != EStato::Indeciso) { continue; }

		const int32 k = Interruttori[i];
		if (!Intents.IsValidIndex(k) || !IntentDefs.IsValidIndex(k)) { continue; }
		ARTUnit* Speso = Units.IsValidIndex(Intents[k].AttackerId) ? Units[Intents[k].AttackerId] : nullptr;
		if (!Speso) { continue; }

		// Qui il soggetto e' chi ha SPESO l'Interrupt, non chi lo subiva: e' la sua azione che non ha
		// ottenuto niente. E' il verso opposto della voce `Cancelled` qui sotto, dove il soggetto e' la
		// vittima — e i due sono coerenti perche' entrambe nominano l'unita' di cui raccontano l'azione.
		const int32 BersaglioId = Intents[k].TargetId;
		const FRTCellId CellaMirata = Units.IsValidIndex(BersaglioId) && Units[BersaglioId]
			? Units[BersaglioId]->Cell
			: Intents[k].TargetCell;
		FRTTurnLogEntry Neutralizzata;
		Neutralizzata.Phase = ERTMatchPhase::Blast;
		Neutralizzata.Category = ERTLogCategory::Fallback;
		Neutralizzata.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
		Neutralizzata.SrcCell = Speso->Cell;
		Neutralizzata.TgtCell = CellaMirata;
		Neutralizzata.Amount = static_cast<int32>(ERTActionInvalidReason::Neutralised);
		Neutralizzata.ActionId = IntentDefs[k].ActionId;
		Neutralizzata.BaseActionId = IntentDefs[k].BaseActionId;
		Neutralizzata.Priority = IntentDefs[k].Priority;
		AppendLogEntry(Neutralizzata, Speso);
	}

	// 🔴 **PASSATA 3 di 3 — gli effetti**, sull'insieme ormai deciso.
	//
	// Una voce per AZIONE cancellata, non per colpo: `InterruptedIntents` porta intenti, quindi due Interrupt
	// sulla stessa vittima producono una voce sola. Si scorre in ordine crescente d'indice perche' l'ordine
	// di `TSet` non e' dichiarato e queste voci entrano nella traccia archiviata.
	TArray<int32> Cancellati = InterruptedIntents.Array();
	Cancellati.Sort();
	for (int32 k : Cancellati)
	{
		if (!Intents.IsValidIndex(k) || !IntentDefs.IsValidIndex(k)) { continue; }
		ARTUnit* Vittima = Units.IsValidIndex(Intents[k].AttackerId) ? Units[Intents[k].AttackerId] : nullptr;
		if (!Vittima) { continue; }

		// 🔴 **L'asimmetria INVERSA**, secondo sito ([D-196], `#1412` punto 4): un'azione cancellata da
		// un'altra unita' non lasciava nessuna traccia autoritativa. Il piano della vittima sparisce dal
		// turno e il replay non sa perche'.
		//
		// `SrcCell` e' la cella di chi SUBISCE l'interruzione — e' la sua azione a essere annullata,
		// quindi e' lei il soggetto della voce — e `UnitId` la segue, come il combat log (`#1418`).
		//
		// ⚠️ `TgtCell` porta **dove puntava l'azione cancellata**, come in ogni altra voce `Fallback`
		// della famiglia (`CuraMancata`, `FallbackEntry`, `ArcRejected`, `SlotOccupied`). Metterci la
		// cella di chi ha interrotto faceva leggere «la vittima attaccava l'interruttore» — preciso e
		// falso, e il campo entra nell'hash.
		//
		// ⚠️ **Chi ha interrotto non entra nella voce**: `UnitId` e' uno solo e lo prende il soggetto.
		// Stesso costo di `#1430` per `RearHitBypassedGuard`/`RearHitBypassedCover`.
		//
		// ⚠️ Un intento puo' puntare a una CELLA e non a un'unita': dopo un fallback `AttackCell`, o su
		// un colpo a memoria di CP 13.2, `TargetId` e' `INDEX_NONE` e il punto di mira sta in `TargetCell`.
		// Ripiegare sulla cella della vittima scriverebbe «si e' attaccata da sola» — preciso e falso, e
		// `TgtCell` entra nell'hash.
		const int32 BersaglioId = Intents[k].TargetId;
		const FRTCellId CellaMirata = Units.IsValidIndex(BersaglioId) && Units[BersaglioId]
			? Units[BersaglioId]->Cell
			: Intents[k].TargetCell;
		FRTTurnLogEntry Interrotta;
		Interrotta.Phase = ERTMatchPhase::Blast;
		Interrotta.Category = ERTLogCategory::Fallback;
		Interrotta.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
		Interrotta.SrcCell = Vittima->Cell;
		Interrotta.TgtCell = CellaMirata;
		Interrotta.Amount = static_cast<int32>(ERTActionInvalidReason::Interrupted);
		Interrotta.ActionId = IntentDefs[k].ActionId;
		Interrotta.BaseActionId = IntentDefs[k].BaseActionId;
		Interrotta.Priority = IntentDefs[k].Priority;
		AppendLogEntry(Interrotta, Vittima);

		// ⛔ **Niente `AddLogEvent` qui**: la riga arriva al combat log attraverso `ConcludeTurn`, che
		// deriva una riga per ogni voce di TurnLog. Tenerla creerebbe un duplicato — la stessa
		// informazione in due formati — che e' il debito noto di `#1412` punto 2.
	}

	// Quali intenti hanno prodotto almeno un colpo: e' la differenza fra «l'azione e' avvenuta» e «e' stata
	// dichiarata e basta», e serve subito sotto.
	TSet<int32> IntentiConColpo;
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		IntentiConColpo.Add(Hit.IntentIndex);
	}

	// 🔴 **Chi ha interrotto paga l'azione che ha speso** (`#1444`, `#1449`).
	//
	// Si scorrono gli INTENTI e non i colpi perche' un Interrupt puo' non produrne nessuno pur essendo stato
	// speso: mirato a una CELLA — direttamente, o su una cella ricordata di CP 13.2 da cui il bersaglio si
	// e' spostato — l'intento e' valido e non c'e' nessuno da colpire.
	//
	// ⚠️ **Ma non paga TUTTO cio' che non ha colpito**, e la prima stesura di `#1449` lo faceva: un
	// Interrupt fuori portata, senza linea di tiro, senza mappa autorevole o su un alleato non produce colpi
	// per le stesse ragioni per cui non ne produce un attacco qualunque, e quelli restano gratuiti. Farli
	// pagare avrebbe dato all'Interrupt una regola sua, contraddicendo il commento venticinque righe piu'
	// su — «un Interrupt senza linea di tiro non cancella nulla, esattamente come un attacco bloccato dalla
	// copertura». Si paga il colpo che c'e' stato, oppure l'intento che mirava a una cella.
	for (int32 k = 0; k < Intents.Num(); ++k)
	{
		if (!IntentDefs.IsValidIndex(k) || !IsCoreAction(IntentDefs[k], ActionInterrupt)
			|| InterruptedIntents.Contains(k))
		{
			continue; // non e' un Interrupt, oppure e' stato annullato a sua volta: non paga
		}
		const bool bMirataACella = Intents[k].TargetId == INDEX_NONE;
		if (!IntentiConColpo.Contains(k) && !bMirataACella)
		{
			continue; // niente colpo e nessuna cella mirata: e' un'azione non avvenuta come le altre
		}
		if (!Ctx.IntentAbilityIndex.IsValidIndex(k) || Ctx.IntentAbilityIndex[k] == INDEX_NONE)
		{
			continue;
		}
		ARTUnit* Interruttore = Units.IsValidIndex(Intents[k].AttackerId)
			? Units[Intents[k].AttackerId] : nullptr;
		// ⚠️ `IsAlive()`: un'unita' uccisa in Prep o nel Dash arriva al Blast col piano ancora addosso —
		// `CollectAttackIntents` non filtra i morti, e lo dichiara — quindi senza questa guardia un cadavere
		// pagherebbe un'azione che non ha mai eseguito.
		//
		// ⚠️ **NON e' la stessa regola di `MarkAttackerAbilitiesSpent`**, e fino al 2026-08-27 questa riga
		// diceva che lo fosse. Qui si gira PRIMA del danno, quindi `IsAlive()` vuol dire «vivo quando
		// annota»; li' si gira DOPO, e vuol dire «sopravvissuto alla fase». Sono i due lati dell'asimmetria
		// di [D-209], e scambiarli cambia il gioco — lo fanno diventare rosse le due righe
		// `curatore che cade nel Blast` e `attaccante che colpisce e cade`.
		if (Interruttore && Interruttore->IsAlive())
		{
			Ctx.MarkAbilitySpent(Interruttore, Ctx.IntentAbilityIndex[k]);
		}
	}

	// Il colpo dell'Interrupt STESSO non deve mai diventare un `FRTAttack`: non fa danno (`Effects` vuoto),
	// ma un colpo a Power 0 nell'array conterebbe comunque come "primo colpo" per `ApplyFirstHitDelta` —
	// consumando il bonus/malus di Guard/Exposed/Marked su un colpo fantasma invece che sull'attacco vero
	// che dovrebbe riceverlo. Si toglie insieme ai colpi degli intenti interrotti, nello stesso filtro.
	//
	// ⚠️ Il filtro guarda l'INTENTO, non l'attaccante: cosi' un'azione non interrompibile della stessa unita'
	// sopravvive, che e' cio' che `InterruptPolicy` dichiara (`#1437`).
	Plan.Hits.RemoveAll([&InterruptedIntents, &IntentDefs](const FRTHexAttackHit& Hit)
	{
		if (InterruptedIntents.Contains(Hit.IntentIndex)) { return true; }
		return IntentDefs.IsValidIndex(Hit.IntentIndex)
			&& IsCoreAction(IntentDefs[Hit.IntentIndex], ActionInterrupt);
	});

	// 🔴 **E lo stesso vale per i rifiuti di CP 10.1**, che la raccolta ha gia' registrato: un'`Interact` su
	// un bordo senza porta, interrotta nello stesso Blast, lascerebbe nel TurnLog sia `Interrupted` sia
	// `Fallback/Cancelled` con `NoEffect` — la traccia direbbe che l'azione e' stata annullata da un
	// avversario e, nella riga accanto, che non aveva niente su cui agire. Un'azione ha un motivo, e quando
	// due sono veri vince quello che l'ha fermata per primo: l'Interrupt cancella l'azione, il rifiuto
	// descriveva cosa avrebbe fatto se fosse arrivata a valutarlo.
	Plan.DoorlessIntents.RemoveAll([&InterruptedIntents](int32 IntentIdx)
	{
		return InterruptedIntents.Contains(IntentIdx);
	});

	// 🔴 **E l'impronta dell'area sparisce con l'azione che l'avrebbe prodotta** ([D-301]). Un colpo
	// interrotto non ha investito nessuna cella: lasciarne il footprint mostrerebbe a schermo un'area che
	// il resolver ha annullato — cioe' una presentazione che contraddice l'esito, che e' il difetto
	// opposto e simmetrico a quello per cui il footprint esiste.
	//
	// ⚠️ Si filtra QUI e non altrove per la stessa ragione degli altri due canali: la coerenza fra colpi
	// cancellati e impronta e' una proprieta' del punto in cui si cancella, non una disciplina da ricordare
	// in un secondo posto. Stesso criterio — l'INTENTO, non l'attaccante.
	Plan.Footprints.RemoveAll([&InterruptedIntents, &IntentDefs](const FRTAttackFootprint& Footprint)
	{
		if (InterruptedIntents.Contains(Footprint.IntentIndex)) { return true; }
		return IntentDefs.IsValidIndex(Footprint.IntentIndex)
			&& IsCoreAction(IntentDefs[Footprint.IntentIndex], ActionInterrupt);
	});
}

void ARTTurnManager::ResolveInterceptions(FRTBlastContext& Ctx)
{
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTHexCombatUnit>& HexUnits = Ctx.HexUnits;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	FRTHexBlastPlan& Plan = Ctx.Plan;

	// `Action.Intercept` (CP 5.3): la reazione piu' delicata, perche' cambia il bersaglio di un attacco ALTRUI.
	// Ha un pass tutto suo, PRIMA delle altre reazioni, e non e' una comodita': il catalogo le da' priorita' 10,
	// la piu' bassa fra le reazioni (Deflect 15, Counter 20). Se risolvesse insieme alle altre, il bersaglio
	// originale valuterebbe il proprio Counter su un colpo che non riceve piu' — contrattaccando per una ferita
	// che ha preso qualcun altro.
	//
	// "Raccogli poi applica" anche qui: si decide TUTTO sui colpi congelati, poi si riscrivono i bersagli.
	// `ExcludedHits` impedisce a due intercettori di reclamare lo stesso colpo; con due unita' per squadra la
	// contesa non e' raggiungibile (chi e' colpito non puo' intercettare per se stesso), ma il meccanismo non
	// dipende da quel numero.
	TSet<int32> ClaimedHits;
	TArray<int32> RedirectHit;      // parallelo alle claim: indice del colpo in Plan.Hits
	TArray<int32> RedirectTo;       // chi lo intercetta (UnitId = indice in Units)
	TArray<int32> RedirectFrom;     // bersaglio ORIGINALE, per il TurnLog
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 ReactionIdx = Unit->PlannedReactionAbility;
		const URTActionData* Reaction = Unit->GetAbility(ReactionIdx);
		if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction
			|| Reaction->Def.ReactionTrigger != ERTReactionTrigger::AllyHitByDirectAttack)
		{
			continue; // non e' un'interposizione: la valuta il ciclo generale piu' sotto
		}

		// Da qui l'azione e' NOSTRA: il ciclo generale non deve rivalutarla ne' registrarla una seconda volta.
		// A garantirlo era l'azzeramento del piano; da `#505` lo garantisce il filtro sul trigger dentro
		// `RunReactionPass`, perche' il piano deve restare leggibile anche dai pass delle fasi successive.

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Reaction;
		Entry.SrcCell = Unit->Cell;
		Entry.TgtCell = Unit->Cell;
		Entry.ActionId = Reaction->Def.ActionId; // `Riktor.Interposition` non e' `Action.Intercept` (CP 5.5)
		Entry.BaseActionId = Reaction->Def.BaseActionId; // vuoto finche' le reazioni non dichiarano un profilo

		// Il contatore di [D-092] vale anche qui: l'interposizione E' un'attivazione, e lasciarla fuori
		// avrebbe reso il limite «una per turno» vero per tre reazioni su quattro.
		const bool bCanReact = Unit->CanUseAbility(ReactionIdx) && !ReactionBlockedThisTurn.Contains(Unit)
			&& Unit->ReactionActivationsThisTurn == 0;
		const int32 HitIdx = bCanReact
			? URTReactionLibrary::FindInterceptableHit(i, Reaction->Def.RangeCells,
				Plan.Hits, Intents, HexUnits, Map, ClaimedHits)
			: INDEX_NONE;

		if (!bCanReact)
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Unavailable);
		}
		else if (HitIdx != INDEX_NONE)
		{
			const int32 OriginalTarget = Plan.Hits[HitIdx].TargetId;
			ClaimedHits.Add(HitIdx);
			RedirectHit.Add(HitIdx);
			RedirectTo.Add(i);
			RedirectFrom.Add(OriginalTarget);

			Unit->ConsumeAbility(ReactionIdx);
			++Unit->ReactionActivationsThisTurn; // [D-092]: anche l'interposizione spende l'attivazione
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Activated);
			// Bersaglio ORIGINALE -> bersaglio FINALE: il TurnLog deve dire da chi a chi e' passato il colpo,
			// altrimenti un danno comparso su un'unita' mai bersagliata risulterebbe inspiegabile nel replay.
			Entry.SrcCell = Units[OriginalTarget]->Cell;
			Entry.TgtCell = Unit->Cell;
			// ➕ **E lo dice per IDENTITA', non solo per cella** (`#1060`, formato v9). Le due celle bastano a
			// discriminare due partite — ed e' per questo che il campo resta fuori dall'hash — ma non a farsi
			// leggere: chi rilegge la voce dovrebbe risolvere `SrcCell` in un'unita', che e' l'inferenza che
			// [D-063] vieta. Senza questo campo `OriginalTargetEquals` non aveva un dato su cui poggiare, e la
			// feature dell'interposizione (`#200`, in `main` da giorni) restava non verificabile da uno scenario.
			// ⚠️ `UnitId` porta gia' l'altro capo — chi INCASSA e' l'unita' che reagisce — quindi la voce ora
			// nomina entrambi i capi del trasferimento.
			//
			// 🔴 **`StableUnitId` e NON l'indice in `Units`**, ed e' la differenza che rende il campo usabile.
			// `OriginalTarget` qui e' un indice di risoluzione (`Plan.Hits[].TargetId`); `AppendLogEntry` scrive
			// invece `Entry.UnitId = Actor->StableUnitId`. Scrivere l'indice farebbe nominare alla **stessa
			// voce** i due capi del trasferimento in **due spazi di identificatori diversi** — e chi la legge
			// non ha modo di accorgersene, perche' sono entrambi `int32` e su un'arena piccola i valori
			// coincidono per caso.
			// ⚠️ Nota per chi estende: `SelectedTargetUnitId` (v8) porta invece l'INDICE. I due spazi convivono
			// gia' in questa struct, e questo campo sceglie quello stabile perche' e' l'unico che regge fuori
			// dalla singola risoluzione — che e' esattamente cio' che un'assertion di scenario deve confrontare.
			Entry.OriginalTargetUnitId = Units[OriginalTarget]->StableUnitId;
			AddLogEvent(FString::Printf(TEXT("%s: si interpone per %s"),
				*Unit->GetName(), *Units[OriginalTarget]->GetName()), FRTLogSubject::Unit(Unit));
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::NotTriggered);
		}

		// Chi REAGISCE. Nell'interposizione `SrcCell` e' la cella del protetto, non la sua: dedurre
		// l'unita' dalla voce darebbe l'unita' sbagliata ([D-063]).
		AppendLogEntry(Entry, Unit);
		// Stesso soggetto della voce: la copia che `ConcludeTurn` deriva e questa raccontano lo stesso
		// evento con le stesse coordinate, e devono passare o cadere insieme.
		AddLogEvent(FString::Printf(TEXT("%s: %s"), *Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Entry)), FRTLogSubject::Unit(Unit));
	}
	// APPLICA: i bersagli si riscrivono solo ora, quando ogni decisione e' stata presa sullo stesso snapshot.
	//
	// Riscrivere il solo `TargetId` non basta (D-017): la copertura e' gia' dentro il `Power`, calcolata sul
	// bordo davanti a chi era il bersaglio quando i colpi sono stati raccolti. Il colpo arriverebbe a chi si
	// interpone protetto dal muretto di qualcun altro — e il combat log mostrerebbe una copertura che davanti
	// a lui non esiste. `RedirectHitTo` rivalida la geometria target-dependent su chi il colpo lo incassa
	// davvero, ed e' l'unico posto dove farlo: qui il redirect e' deciso e nessuna reazione e' ancora stata
	// valutata sui colpi riscritti, quindi la rivalidazione non puo' aprire una seconda opportunity.
	//
	// E' la stessa disciplina dei bonus di coppia piu' sotto (`Gadget.LinearDischarge` contro `Status.Wet`):
	// cio' che dipende da CHI subisce si decide dopo l'Intercept, non prima.
	for (int32 r = 0; r < RedirectHit.Num(); ++r)
	{
		Plan.Hits[RedirectHit[r]] = URTHexCombatLibrary::RedirectHitTo(RedirectTo[r],
			Plan.Hits[RedirectHit[r]], HexUnits, Intents, Map);
	}
}

void ARTTurnManager::RunBlastReactions(FRTBlastContext& Ctx)
{
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTUnitCombatState>& States = Ctx.States;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	FRTHexBlastPlan& Plan = Ctx.Plan;
	FRTReactionPassResult& Reactions = Ctx.Reactions;

	// Reazioni (CP 5.1). I pass sono DUE, uno per fase (D-092): cosa raccoglie ciascuno, e perche' le fughe
	// si applicano al suo interno, sta scritto su `FRTReactionPassResult` e su `RunReactionPass`.
	// L'esito NON e' locale a questa funzione: vive nel contesto perche' il danno lo consuma molto piu' tardi.
	RunReactionPass(ERTReactionPassPoint::BlastHits,
		[&Plan, &Intents](int32 SelfId, ERTReactionTrigger Trigger)
		{
			// Qui «scattato» e «chi l'ha innescato» coincidono: un colpo ha sempre un attaccante, ed e' la
			// convenzione con cui `FindTriggeringAttacker` e' nato. Smettono di coincidere negli altri punti.
			const int32 By = URTReactionLibrary::FindTriggeringAttacker(Trigger, SelfId, Plan.Hits, Intents);
			return FRTReactionTriggerHit{ By != INDEX_NONE, By };
		},
		Units, States, Map, Reactions);
}

void ARTTurnManager::LogBlockedIntents(const FRTBlastContext& Ctx)
{
	const TArray<ARTUnit*>& Units = Ctx.Units;
	const TArray<FRTHexCombatUnit>& HexUnits = Ctx.HexUnits;
	const TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	const TArray<FRTActionDef>& IntentDefs = Ctx.IntentDefs;
	const FRTHexBlastPlan& Plan = Ctx.Plan;

	// Intenti fermati dalla copertura: l'attacco non avviene e il TurnLog ne registra il motivo.
	for (const int32 BlockedIdx : Plan.BlockedIntents)
	{
		if (!Intents.IsValidIndex(BlockedIdx)) { continue; }
		const FRTHexAttackIntent& Blocked = Intents[BlockedIdx];
		// Il bersaglio puo' essere una CELLA e non un'unita' (`Fallback.AttackCell`, e da CP 9.2 anche il
		// modo in cui si punta una struttura): con `TargetId == INDEX_NONE` indicizzare `HexUnits` uscirebbe
		// dall'array. La cella mirata e' comunque cio' che il log deve dire.
		const bool bTargetsUnit = HexUnits.IsValidIndex(Blocked.TargetId);
		FRTTurnLogEntry NoLos;
		NoLos.Phase = ERTMatchPhase::Blast;
		NoLos.Category = ERTLogCategory::Combat;
		NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
		NoLos.SrcCell = HexUnits[Blocked.AttackerId].Cell;
		NoLos.TgtCell = bTargetsUnit ? HexUnits[Blocked.TargetId].Cell : Blocked.TargetCell;
		NoLos.Amount = 0;
		// QUALE azione e' stata fermata (CP 11.3, #79). `Plan.BlockedIntents` indicizza `Intents`, e
		// `IntentDefs` gli e' parallelo per costruzione: entrambi crescono negli stessi due punti, quelli
		// degli intenti pianificati e degli impatti di carica.
		//
		// Serve piu' qui che altrove: un attacco che non avviene lascia SOLO questa voce, e senza il nome
		// «nessuna linea di tiro» non dice se a mancare il colpo e' stata l'ultimate o l'attacco base.
		if (IntentDefs.IsValidIndex(BlockedIdx))
		{
			NoLos.ActionId = IntentDefs[BlockedIdx].ActionId;
			NoLos.BaseActionId = IntentDefs[BlockedIdx].BaseActionId;
			NoLos.Priority = IntentDefs[BlockedIdx].Priority;
		}
		AppendLogEntry(NoLos, Units.IsValidIndex(Blocked.AttackerId) ? Units[Blocked.AttackerId] : nullptr);
		// ⚠️ Il soggetto e' l'ATTACCANTE anche se la frase nomina entrambi i capi: e' quello che
		// `AppendLogEntry` scrive nella voce, e `ConcludeTurn` ne deriva una riga gemella di questa.
		// Scegliere il bersaglio qui farebbe filtrare le due copie con criteri diversi.
		AddLogEvent(FString::Printf(TEXT("%s (%s -> %s)"),
			*URTTurnLogLibrary::DescribeEntry(NoLos),
			*Units[Blocked.AttackerId]->GetName(),
			bTargetsUnit ? *Units[Blocked.TargetId]->GetName() : TEXT("cella")),
			FRTLogSubject::Unit(Units.IsValidIndex(Blocked.AttackerId) ? Units[Blocked.AttackerId] : nullptr));
	}

	// Interazioni dichiarate su un bordo dove non c'e' nessuna porta (CP 10.1, `#74`).
	//
	// ⛔ **Prima non lasciavano traccia.** L'operazione non veniva raccolta, il turno passava, e il giocatore
	// vedeva la propria azione sparire senza che il TurnLog avesse niente da spiegare. La voce e' `Fallback`
	// con `NoEffect` e non `Combat` con `NoLineOfSight`: non c'e' nessuna copertura di mezzo, e dirlo
	// scriverebbe una cosa falsa su una partita vera — la stessa ragione per cui `UnverifiableIntents` e'
	// tenuto separato da `BlockedIntents`.
	for (const int32 DoorlessIdx : Plan.DoorlessIntents)
	{
		if (!Intents.IsValidIndex(DoorlessIdx)) { continue; }
		const FRTHexAttackIntent& Doorless = Intents[DoorlessIdx];
		if (!HexUnits.IsValidIndex(Doorless.AttackerId)) { continue; }

		// 🔴 **La coppia di celle E' IL BORDO**, come in ogni altra voce di questo sottosistema — le
		// coperture e le porte la scrivono cosi'. La prima stesura ci metteva attaccante -> cella mirata:
		// due `Interact` rifiutate dalla stessa unita' verso la stessa cella ma su bordi DIVERSI producevano
		// righe identiche byte a byte, ed e' il difetto che [D-196] ha gia' pagato per le voci `Fallback`.
		// Quale dei sei bordi il giocatore avesse dichiarato e' l'unica informazione diagnostica del rifiuto.
		const FRTCellId RefusedCell = HexUnits.IsValidIndex(Doorless.TargetId)
			? HexUnits[Doorless.TargetId].Cell : Doorless.TargetCell;

		FRTTurnLogEntry NoDoor;
		NoDoor.Phase = ERTMatchPhase::Blast;
		NoDoor.Category = ERTLogCategory::Fallback;
		NoDoor.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
		NoDoor.SrcCell = RefusedCell;
		NoDoor.TgtCell = Doorless.bHasDeclaredDoorEdge
			? URTHexLibrary::Neighbor(RefusedCell, Doorless.DeclaredDoorEdge)
			: RefusedCell;
		// Il PERCHE' viaggia in `Amount`, come ogni voce `Fallback`: `NoEffect` — l'azione e' stata
		// dichiarata, e' stata valutata, e non aveva niente su cui agire.
		NoDoor.Amount = static_cast<int32>(ERTActionInvalidReason::NoEffect);
		if (IntentDefs.IsValidIndex(DoorlessIdx))
		{
			NoDoor.ActionId = IntentDefs[DoorlessIdx].ActionId;
			NoDoor.BaseActionId = IntentDefs[DoorlessIdx].BaseActionId;
			NoDoor.Priority = IntentDefs[DoorlessIdx].Priority;
		}

		ARTUnit* Actor = Units.IsValidIndex(Doorless.AttackerId) ? Units[Doorless.AttackerId] : nullptr;
		AppendLogEntry(NoDoor, Actor);
		// ⚠️ Due testi, perche' i casi sono due: `DoorlessIntents` raccoglie ANCHE gli intenti senza bordo
		// dichiarato — il ramo di CP 9.3, dove l'operazione nasce da una traiettoria. Dire «sul bordo
		// dichiarato» a chi non ne ha dichiarato uno indirizza verso una causa che non esiste.
		AddLogEvent(FString::Printf(TEXT("%s: %s"),
			Actor ? *Actor->GetName() : TEXT("unita'"),
			Doorless.bHasDeclaredDoorEdge
				? TEXT("nessuna porta sul bordo dichiarato")
				: TEXT("nessuna porta sulla traiettoria")),
			FRTLogSubject::Unit(Actor));
	}
}

void ARTTurnManager::ApplyEnvironmentChanges(FRTBlastContext& Ctx)
{
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTHexCombatUnit>& HexUnits = Ctx.HexUnits;
	TArray<FRTHexAttackIntent>& Intents = Ctx.Intents;
	TArray<FRTPendingArcOp>& PendingArcOps = Ctx.PendingArcOps;
	FRTHexBlastPlan& Plan = Ctx.Plan;

	// STRUTTURE (CP 9.2): il danno raccolto contro le barriere si applica ORA, a colpi risolti — non durante
	// la raccolta. Chi ha sparato in questo Blast non guadagna la linea perche' il muro e' caduto: la vista e
	// il grafo si riaprono dalla fase successiva, e l'ordine dei colpi non cambia l'esito (invariante #3).
	//
	// La mappa si prende dall'actor, non dal `Map` const di questa funzione: la partita gira su una COPIA di
	// lavoro (`ARTGameMode` la duplica all'avvio), quindi abbattere un muro non tocca l'asset su disco. E' lo
	// stesso puntatore che il Cleanup usa per le superfici dinamiche.
	ARTHexMapActor* StructureMapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* MutableMap = StructureMapActor ? StructureMapActor->MapAsset : nullptr;
	for (const FRTCoverDamageResult& Change :
		URTHexCoverLibrary::ApplyStructureDamage(MutableMap, Plan.StructureHits))
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			Change.bDestroyed ? ERTEnvironmentOutcome::CoverDestroyed : ERTEnvironmentOutcome::CoverDamaged);
		// La coppia di celle E' il bordo: nessun campo nuovo nel TurnLog per dire "quale lato".
		Entry.SrcCell = Change.Cell;
		Entry.TgtCell = Change.Toward;
		Entry.Amount = Change.RemainingIntegrity;
		// L'attaccante arriva col risultato: `FRTStructureHit` lo dichiarava gia' — «serve al TurnLog, non al
		// calcolo» — e ora `FRTCoverDamageResult` lo propaga. Resta un indice fino a qui, quindi lo strato di
		// mappa non ha mai visto un Actor.
		AppendLogEntry(Entry, Units.IsValidIndex(Change.AttackerId) ? Units[Change.AttackerId] : nullptr);
		AddLogEvent(FString::Printf(TEXT("Copertura (q=%d,r=%d,L%d) verso (q=%d,r=%d): %s (integrita' %d)"),
			Change.Cell.X, Change.Cell.Y, Change.Cell.Layer, Change.Toward.X, Change.Toward.Y,
			Change.bDestroyed ? TEXT("abbattuta") : TEXT("danneggiata"), Change.RemainingIntegrity), FRTLogSubject::World());

		// Una copertura abbattuta smette di essere anche una copertura TEMPORANEA (CP 9.5). Senza questa
		// riga l'entry sopravvive al proprio riparo, e siccome identifica la barriera con la sola coppia
		// (cella, bordo), alla scadenza del suo timer porterebbe via **quello che trova su quel bordo**: se
		// nel frattempo qualcuno ha riparato lo stesso varco, gli distrugge il pannello in anticipo e scrive
		// nel TurnLog una scadenza mai avvenuta. Due Riktor sullo stesso choke point bastano — i cooldown
		// sono per unita', quindi il secondo non aspetta il primo.
		if (Change.bDestroyed)
		{
			ERTHexDirection DestroyedEdge = ERTHexDirection::E;
			if (URTHexCoverLibrary::EdgeDirection(Change.Cell, Change.Toward, DestroyedEdge))
			{
				const FRTCellId DestroyedCell = Change.Cell;
				DynamicCovers.RemoveAll([&DestroyedCell, DestroyedEdge](const FRTDynamicCover& Tracked)
				{
					return Tracked.Cell == DestroyedCell && Tracked.Edge == DestroyedEdge;
				});
			}
		}
	}

	// ARCHI (CP 9.4). Due pezzi, entrambi a fase conclusa:
	//
	// 1. il DANNO. Un'azione che dichiara `DamageStructure` e mira a un'unita' agli estremi di un ponte lo
	//    scalfisce. La coppia (attaccante, bersaglio) identifica l'arco per la stessa ragione di `ModifyArc`;
	//    se fra le due celle non c'e' un arco, `DamageArc` non fa nulla e non tocca la revisione.
	for (int32 IntentIdx = 0; IntentIdx < Intents.Num(); ++IntentIdx)
	{
		const FRTHexAttackIntent& Intent = Intents[IntentIdx];
		if (Intent.StructurePower <= 0 || !HexUnits.IsValidIndex(Intent.AttackerId))
		{
			continue;
		}
		// Il bersaglio puo' essere una CELLA (`TargetId == INDEX_NONE`): l'arco si cerca comunque, verso la
		// cella mirata — indicizzare `HexUnits[TargetId]` uscirebbe dall'array.
		const FRTCellId ArcTo = HexUnits.IsValidIndex(Intent.TargetId)
			? HexUnits[Intent.TargetId].Cell : Intent.TargetCell;

		for (const FRTArcChange& Change :
			URTHexArcLibrary::DamageArc(MutableMap, HexUnits[Intent.AttackerId].Cell, ArcTo, Intent.StructurePower))
		{
			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Blast;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(Change.bBroken
				? ERTEnvironmentOutcome::BridgeDestroyed : ERTEnvironmentOutcome::BridgeDamaged);
			Entry.SrcCell = Change.From;
			Entry.TgtCell = Change.To;
			Entry.Amount = Change.RemainingIntegrity;
			AppendLogEntry(Entry, Units.IsValidIndex(Intent.AttackerId) ? Units[Intent.AttackerId] : nullptr);
			AddLogEvent(FString::Printf(TEXT("Ponte (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d): %s (integrita' %d)"),
				Change.From.X, Change.From.Y, Change.From.Layer,
				Change.To.X, Change.To.Y, Change.To.Layer,
				Change.bBroken ? TEXT("crollato") : TEXT("danneggiato"), Change.RemainingIntegrity), FRTLogSubject::World());

			// Un ponte ABBATTUTO smette di essere anche un ponte TEMPORANEO. Senza questa riga l'entry
			// sopravvive al proprio ponte, e non in modo innocuo: `DamageArc` non RIMUOVE l'arco, lo marca
			// `Destroyed` e lo lascia sulla mappa, quindi alla scadenza del timer `TickDynamicArcs` chiama
			// `RemoveTransition` e ci RIESCE — scrivendo un `BridgeRemoved` per un crollo avvenuto turni prima
			// e portandosi via le macerie. Le macerie devono restare: e' su di esse che `SetArcState` esercita
			// il proprio «terminale» (un ponte abbattuto non si riattiva).
			//
			// Stessa disciplina delle coperture (#301, `ApplyStructureDamage`): l'entry muore quando muore la
			// struttura che rappresenta. `DamageArc` restituisce un `Change` per verso, quindi qui si passa due
			// volte sulla stessa coppia: `RemoveAll` e' idempotente e la seconda non trova piu' nulla.
			if (Change.State == ERTHexArcState::Destroyed)
			{
				const FRTCellId BrokenFrom = Change.From;
				const FRTCellId BrokenTo = Change.To;
				DynamicArcs.RemoveAll([&BrokenFrom, &BrokenTo](const FRTDynamicArc& Tracked)
				{
					return (Tracked.From == BrokenFrom && Tracked.To == BrokenTo)
						|| (Tracked.From == BrokenTo && Tracked.To == BrokenFrom);
				});
			}
		}
	}

	// 2. `ModifyArc`: se il collegamento c'e' lo TOGLIE, altrimenti lo CREA — «apri o chiudi» e' la stessa
	//    azione vista dai due lati. Il ponte creato e' TEMPORANEO (2 turni, come le altre modifiche ambientali
	//    del catalogo) e CONDUTTIVO: la scarica lo risale, quindi e' un rischio oltre che una scorciatoia.
	//    Ordine canonico prima di applicare: l'ordine delle unita' non deve decidere l'esito.
	// ⚠️ L'ordinamento deve essere TOTALE: da [D-197] la voce porta anche `ActionId`, che entra nell'hash,
	// quindi due operazioni sullo stesso arco non possono piu' avere un ordine indeterminato — deciderebbero
	// quale azione la traccia archiviata nomina. `TArray::Sort` non e' stabile.
	PendingArcOps.Sort([](const FRTPendingArcOp& A, const FRTPendingArcOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		if (!(A.To == B.To)) { return URTHexLibrary::StableLess(A.To, B.To); }
		// Spareggio sull'unita' e poi sull'azione: due operazioni sullo STESSO arco avevano un ordine
		// indeterminato — `TArray::Sort` non e' stabile — e da [D-197] quell'ordine decide quale `ActionId`
		// la voce archivia. `StableUnitId` e' l'identita' che non dipende dall'ordine di spawn.
		const int32 UnitA = A.Actor ? A.Actor->StableUnitId : 0;
		const int32 UnitB = B.Actor ? B.Actor->StableUnitId : 0;
		if (UnitA != UnitB) { return UnitA < UnitB; }
		return A.Def.ActionId.LexicalLess(B.Def.ActionId);
	});
	for (const FRTPendingArcOp& Op : PendingArcOps)
	{
		if (!MutableMap) { break; }

		const bool bRemoved = MutableMap->RemoveTransition(Op.From, Op.To, /*bBothDirections*/ true);
		if (bRemoved)
		{
			DynamicArcs.RemoveAll([&Op](const FRTDynamicArc& D)
			{
				return (D.From == Op.From && D.To == Op.To) || (D.From == Op.To && D.To == Op.From);
			});
		}
		else
		{
			// Creazione: i due versi in UNA sola revisione (un ponte e' un evento, non due archi).
			TArray<FRTHexEdge> Built;
			FRTHexEdge Forward(Op.From, Op.To, /*Cost*/ 1, ERTHexTransitionKind::Bridge);
			Forward.bConductsElectricity = true;
			FRTHexEdge Backward(Op.To, Op.From, /*Cost*/ 1, ERTHexTransitionKind::Bridge);
			Backward.bConductsElectricity = true;
			Built.Add(Forward);
			Built.Add(Backward);
			MutableMap->UpdateTransitions(Built);
			// Durata 2 turni, come le altre modifiche ambientali del catalogo (`Ignite`, `CreateWater`).
			DynamicArcs.Add({ Op.From, Op.To, /*TurnsRemaining*/ 2, /*CreatedOnTurn*/ TurnNumber });
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			bRemoved ? ERTEnvironmentOutcome::BridgeRemoved : ERTEnvironmentOutcome::BridgeCreated);
		// QUALE azione, non l'azione generica ([D-195], [D-197], `#1447`): il percorso di FALLIMENTO
		// (`ArcRejected`) la nomina gia' cosi', e i due devono dire la stessa cosa.
		//
		// ⚠️ Se la def manca si degrada **insieme**: nome generico, e gli altri due campi a zero. Prendere
		// `Op.Def.Priority` da una def costruita per default scriverebbe **50** — il valore di partenza del
		// campo — mentre il catalogo dichiara 75 per `Action.ModifyArc`, e `Priority` discrimina in
		// `EntryLess`: la voce affermerebbe una precedenza che il resolver non ha usato.
		if (Op.Def.ActionId.IsNone())
		{
			Entry.ActionId = ActionModifyArc;
		}
		else
		{
			Entry.ActionId = Op.Def.ActionId;
			Entry.BaseActionId = Op.Def.BaseActionId;
			Entry.Priority = Op.Def.Priority;
		}
		Entry.SrcCell = Op.From;
		Entry.TgtCell = Op.To;
		Entry.Amount = bRemoved ? 0 : 2; // turni di durata del ponte creato
		AppendLogEntry(Entry, Op.Actor);
		AddLogEvent(FString::Printf(TEXT("Collegamento %s: (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d)"),
			bRemoved ? TEXT("rimosso") : TEXT("creato"),
			Op.From.X, Op.From.Y, Op.From.Layer, Op.To.X, Op.To.Y, Op.To.Layer), FRTLogSubject::World());
	}

	// PORTE (CP 9.3): stessa disciplina delle strutture — gli ordini si applicano ORA, a colpi risolti, non
	// durante la raccolta. Chi ha sparato in questo Blast non perde la linea perche' una porta si e' chiusa: la
	// topologia cambia dalla fase SUCCESSIVA, che e' il Move — ed e' li' che il percorso gia' pianificato deve
	// accorgersene.
	for (const FRTDoorChange& Change : URTHexDoorLibrary::ApplyDoorOps(MutableMap, Plan.DoorOps))
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			Change.bBlocking ? ERTEnvironmentOutcome::DoorClosed : ERTEnvironmentOutcome::DoorOpened);
		// La coppia di celle E' il bordo, come per le coperture: nessun campo nuovo nel TurnLog.
		Entry.SrcCell = Change.Cell;
		Entry.TgtCell = Change.Toward;
		Entry.Amount = static_cast<int32>(Change.State);
		// Come per le coperture, e per la stessa via: l'intento conosce chi agisce, `FRTDoorOp` lo porta e il
		// cambio lo restituisce. Aprire una porta e' un'azione deliberata: se restasse a `0` sarebbe l'unica
		// del turno a dichiarare «nessuna unita'».
		AppendLogEntry(Entry, Units.IsValidIndex(Change.ActorId) ? Units[Change.ActorId] : nullptr);
		AddLogEvent(FString::Printf(TEXT("Porta (q=%d,r=%d,L%d) verso (q=%d,r=%d): %s"),
			Change.Cell.X, Change.Cell.Y, Change.Cell.Layer, Change.Toward.X, Change.Toward.Y,
			Change.bBlocking ? TEXT("chiusa") : TEXT("aperta")), FRTLogSubject::World());
	}
}

void ARTTurnManager::ApplyDisplacements(FRTBlastContext& Ctx)
{
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTUnitCombatState>& States = Ctx.States;
	TMap<ARTUnit*, FRTCellId>& KnockFrom = Ctx.KnockFrom;
	TMap<ARTUnit*, int32>& KnockDist = Ctx.KnockDist;
	TMap<ARTUnit*, int32>& KnockCount = Ctx.KnockCount;
	TMap<ARTUnit*, FRTCellId>& PullToward = Ctx.PullToward;
	TMap<ARTUnit*, int32>& PullDist = Ctx.PullDist;
	TMap<ARTUnit*, int32>& PullCount = Ctx.PullCount;
	TMap<ARTUnit*, FRTDisplacementCause>& PushCause = Ctx.PushCause;
	TMap<ARTUnit*, FRTDisplacementCause>& PullCause = Ctx.PullCause;

	// --- Ancoraggio (CP 7.5, `#505`): il punto di valutazione degli SPOSTAMENTI ----------------------
	// Spinte e trazioni sono decise — raccolte in `KnockCount`/`PullCount` — e non ancora applicate. E' il
	// solo momento in cui `Reaction.Anchor` puo' annullarle: dopo, annullare vorrebbe dire rimettere indietro
	// un'unita' gia' mossa, con due voci di TurnLog che si contraddicono sullo stesso passo.
	//
	// UNA chiamata per spinta e trazione insieme, non due: un'unita' spinta **e** tirata nello stesso Blast
	// deve reagire una volta sola, e «una attivazione per turno» e' una garanzia del pass — chiamandolo due
	// volte la si perderebbe qui invece che nel catalogo.
	//
	// Risultato in una struct PROPRIA: `Reactions` e' gia' stata consumata piu' sopra (deflect nei delta del
	// primo danno, contrattacchi negli attacchi), e riusarla farebbe ripartire `DeflectDelta` da zero.
	// Di questo punto si consuma `CancelledDisplacements` e basta: un contrattacco dichiarato da una reazione
	// allo spostamento arriverebbe a colpi gia' risolti, quindi il catalogo non lo prevede (`Reaction.Anchor`
	// dichiara solo `CancelDisplacement`).
	FRTReactionPassResult DisplacementReactions;
	RunReactionPass(ERTReactionPassPoint::BlastDisplacement,
		[&Units, &KnockCount, &PullCount](int32 SelfId, ERTReactionTrigger)
		{
			ARTUnit* Self = Units.IsValidIndex(SelfId) ? Units[SelfId] : nullptr;
			const int32* Pushes = Self ? KnockCount.Find(Self) : nullptr;
			const int32* Pulls  = Self ? PullCount.Find(Self) : nullptr;

			// **Esattamente UNA**, non «almeno una»: due spinte sullo stesso bersaglio si annullano da sole
			// per geometria (`OpposingForces`, `#420`) e il bersaglio resta fermo comunque. Con `Contains`
			// l'ancora si spendeva per fermare uno spostamento che non sarebbe avvenuto, e il TurnLog
			// scriveva pure un'altra causa — trovato in code review.
			//
			// E' la stessa regola di `Reaction.Cleanse`, che non si attiva quando annullare non cambierebbe
			// nulla: due moduli dello stesso checkpoint non possono avere filosofie opposte su quando
			// spendere l'unica attivazione del turno.
			const bool bAboutToMove = (Pushes && *Pushes == 1) || (Pulls && *Pulls == 1);

			// Nessun `TriggeredBy`: di uno spostamento si conosce la cella di provenienza
			// (`KnockFrom`/`PullToward`), non un'unita' — e con due attaccanti non ce ne sarebbe una sola.
			// Gli effetti difensivi non ne hanno bisogno, e `BuildReactionEvents` scarta gli offensivi.
			return FRTReactionTriggerHit{ bAboutToMove, INDEX_NONE };
		},
		Units, States, Map, DisplacementReactions);

	// --- Spinta (knockback): dopo il danno, sulle posizioni snapshot del Blast -----------------------
	// Direzione ESAGONALE (una delle sei), non piu' cardinale: la spinta segue la linea attaccante->bersaglio
	// e si ferma su bordo mappa, ostacolo o unita'. Spinte multiple sullo stesso bersaglio si annullano.
	if (KnockCount.Num() > 0)
	{
		// Bloccanti: le celle di tutte le unita' (non si spinge dentro un'altra unita').
		TArray<FRTCellId> KOccupied;
		for (ARTUnit* U : Units) { KOccupied.Add(U->Cell); }

		// Destinazioni dallo snapshot: solo bersagli vivi spinti da ESATTAMENTE un attaccante.
		// Si itera su Units (ordine stabile per cella): l'ordine di iterazione di una TMap non e' garantito
		// e da qui dipendono la sequenza del playback e quella del combat log.
		TArray<ARTUnit*> KTargets;
		TArray<FRTCellId> KFinal;

		// Chi si e' spostato per SCELTA e non per la spinta ([D-047]): serve al solo verbo del log, che
		// altrimenti racconterebbe «spinto» un'unita' che ha deciso di scartare. Il TurnLog esiste per dire
		// QUALE difesa ha retto e quale no — un verbo sbagliato e' la stessa lacuna di `#420`, un livello sopra.
		TSet<const ARTUnit*> Sidestepped;

		// Lo spazio di id alive-only in cui vive `Key.OwnerId`, costruito **al piu' una volta per Blast** e
		// solo se una finestra si apre davvero.
		//
		// 🔴 **Prima stava DENTRO il ciclo**, e una code review ha misurato il costo: `MakeCurrentSnapshot`
		// fa un `GetAllActorsOfClass` sul livello, costruisce un `FRTHexSnapshot` che qui viene **buttato
		// via**, e ordina — tutto questo per **ogni** unita' in `Brace`. Il caso di gran lunga piu' comune e'
		// il profilo base (Riktor), dove `AskReactionDecision` risponde `HoldImmediate` senza mai leggere
		// `OwnerId`: si pagava un giro completo per un valore che nessuno guardava.
		TArray<ARTUnit*> BlastAliveUnits;

		for (ARTUnit* T : Units)
		{
			const int32* Pushes = KnockCount.Find(T);

			// FORZE CONTRADDITTORIE (#420): spinto da due o piu' attaccanti, resta fermo. Non e' una difesa —
			// e' la geometria del turno — e fino a qui non lasciava traccia da nessuna parte, ne' nel combat
			// log ne' nel file. La voce si scrive PRIMA del `continue` che scarta il bersaglio.
			if (Pushes && *Pushes > 1 && IsValid(T) && T->IsAlive())
			{
				// Nessuna causa: `PushCause` conserva UN attaccante su due, e nominarlo direbbe che a fermare
				// l'unita' e' stata quella azione. Ne sono servite due, ed e' proprio il punto dell'esito.
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::OpposingForces, nullptr);
			}

			if (!Pushes || *Pushes != 1 || !IsValid(T) || !T->IsAlive()) { continue; }

			// `Reaction.Anchor` (CP 7.5, `#505`) annulla lo spostamento a QUALUNQUE distanza: «impedisce una
			// spinta» e' un conteggio, non una soglia (D-094), e «una» la garantisce l'attivazione unica del
			// pass.
			//
			// Sta PRIMA di `Guarded` e `Braced`, e non e' un dettaglio d'ordine: quando si arriva qui l'ancora
			// e' gia' stata attivata e consumata dal pass, che gira prima di sapere se una difesa passiva
			// avrebbe retto. Metterla dopo scriverebbe nel TurnLog «spinta retta: in guardia» per un turno in
			// cui il giocatore ha speso la reazione — un log che nomina la difesa sbagliata. Una reazione
			// dichiarata ha la precedenza su uno stato che non si consuma.
			if (DisplacementReactions.CancelledDisplacements.Contains(Units.IndexOfByKey(T)))
			{
				AddLogEvent(FString::Printf(TEXT("%s: ancorato, la spinta non lo sposta"), *T->GetName()), FRTLogSubject::Unit(T));
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::Anchored, &PushCause);
				continue;
			}

			// `Action.Guard` regge una spinta di UNA cella: chi si e' piantato non arretra di un passo, ma una
			// spinta piu' forte lo sposta comunque (la guardia non e' un'ancora, catalogo v0.1 §1).
			//
			// ⚠️ **Dal 2026-08-11 una spinta piu' forte ESISTE** (D-085): `Weapon.Impact` su
			// `Phase.PressureJet`, che spinge gia' di 1, produce una spinta di **2** — ed e' il loadout di
			// DEFAULT di Phase (D-089). Fino a CP 7.1 questo ramo assorbiva ogni spinta del gioco e il commento
			// diceva cosi'; ora cede, e il ramo `Braced` sotto **aggiunge copertura davvero**.
			// Pinnato da `Equipment.PushTwoSeparatesGuardFromBrace`.
			if (T->HasStatus(TAG_Status_Guarded) && KnockDist[T] <= URTCombatLibrary::GuardResistedPushDistance)
			{
				AddLogEvent(FString::Printf(TEXT("%s: in guardia, resiste alla spinta"), *T->GetName()), FRTLogSubject::Unit(T));
				// La stringa sopra e' per l'HUD e non finisce nel file (#420): la voce di TurnLog e' questa, ed
				// e' cio' che permette a un replay di dire QUALE difesa ha retto invece del solo «non si e' mosso».
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::Guarded, &PushCause);
				continue;
			}

			// `Action.Brace` (CP 5.2) "impedisce la PRIMA spinta". "Prima" e non "tutte" e' rispettato per
			// costruzione, non da un contatore: tutte le spinte del Blast si risolvono in questo unico
			// passaggio, e un bersaglio spinto da 2+ attaccanti e' gia' escluso sopra (`*Pushes != 1`,
			// forze contraddittorie).
			//
			// Il ramo non guarda `KnockDist`, quindi regge una spinta di qualunque distanza.
			//
			// ⚠️ **Questo commento e' stato riscritto due volte, e la seconda inverte la prima.** Fino al
			// 2026-08-10 diceva che la distanza distingueva `Brace` da `Guard`; D-074 lo corresse, perche' con
			// un solo valore di spinta (`1`) il ramo `Guarded` sopra intercettava gia' tutto e questo non
			// vedeva mai un caso proprio. **Dal 2026-08-11 la premessa di D-074 e' caduta**: `Weapon.Impact`
			// su un attacco che spinge gia' produce una spinta di **2** (D-085), quindi `Guard` cede e questo
			// ramo regge. La distanza torna a essere un asse che separa le due difese, e non per una v0.2:
			// oggi, con il loadout di default di Phase.
			//
			// Resta vero che le due differiscono anche nel danno (-15 sul primo colpo contro -10 su ogni
			// colpo), pinnato da `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit`.
			// Il caso della spinta di 2 e' pinnato da `Equipment.PushTwoSeparatesGuardFromBrace`.
			//
			// ⚠️ Ha una conseguenza su `BAL-1` ([#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403)):
			// l'opzione «`Guard` solo danno, `Brace` solo spostamento» era stata **preclusa** perche' senza
			// una spinta >= 2 avrebbe lasciato `Brace` senza mestiere. Quel mestiere ora esiste.
			if (T->HasStatus(TAG_Status_Braced))
			{
				// ➕ **[D-047], fetta 3 di E14.7: da qui il `Brace` non decide piu' da solo.** Le risposte
				// legali vengono dal Reaction Profile che l'unita' porta, e la loro CARDINALITA' dice se si
				// apre una finestra — con la regola che ADR-0004 §2 ha gia' (`AllowedResponses >= 2`), non con
				// una nuova. E' il punto in cui `URTCatalogLibrary::BraceAllowedResponses` smette di essere un
				// dato che leggono solo i test.
				//
				// ⚠️ **Col profilo base non cambia NIENTE, ed e' una proprieta' e non una fortuna**: l'elenco
				// ha la sola `Hold Ground`, `RequiresDecisionBoundary` e' falso, `AskReactionDecision`
				// restituisce subito `HoldImmediate` senza chiedere a nessuno, e si finisce nelle due righe di
				// sempre. Nessun prompt e nessuna sospensione per chi si copre e basta — la voce di DoD «col
				// profilo base nessun boundary si apre» e' vera per COSTRUZIONE, non per un `if` che la
				// protegge.
				FRTReactionOpportunity BraceOpportunity;
				BraceOpportunity.Key.TurnNumber = TurnNumber;
				BraceOpportunity.Key.MacroPhase = ERTMatchPhase::Blast; // la spinta si risolve qui, e la
				                                                        // chiave dice la fase del TURNO
				// 🔴 **`OwnerId` vive nello spazio di id di `MakeCurrentSnapshot`, NON in quello del Blast**, e
				// la differenza non e' teorica: `GatherBlastUnits` aggiunge **ogni** `ARTUnit` senza filtrare
				// (`Ctx.Units`), mentre `MakeCurrentSnapshot` scarta i morti — il suo commento lo dichiara,
				// «i morti (es. nel Blast) non si muovono e non bloccano». Entrambi ordinano per cella, quindi
				// **un solo caduto che ordina prima di questa unita' sposta di uno tutti gli indici a valle**.
				//
				// ⚠️ Ogni consumatore di `Key.OwnerId` assume lo spazio alive-only: `DecideScriptedResponse`
				// risolve `RuntimeUnits[OwnerUnitId]` su un array preso da `MakeCurrentSnapshot`. Con l'indice
				// del Blast, una partita in cui qualcuno e' gia' caduto risolverebbe l'unita' SBAGLIATA — la
				// decisione scriptata non verrebbe riconosciuta e la finestra scadrebbe in `Hold Ground`, con
				// l'harness che segnala «finestra scoperta» invece del difetto vero.
				//
				// L'Overwatch e' immune per costruzione — `ResolveMovement` costruisce il proprio `Units`
				// **da** `MakeCurrentSnapshot` — e questo ramo era l'unico produttore nell'altro spazio.
				// ⛔ Nessun test lo vedeva: gli scenari hanno tutte le unita' vive, e con zero morti i due
				// spazi coincidono. Trovato da una code review, non dalla suite.
				BraceOpportunity.Key.ReactionDefId = FName(TEXT("Action.Brace"));

				// Le ESEGUIBILI, non le dichiarate: `Profile.Grounding` e `Profile.Glance` sono contenuto
				// deciso da [D-132] e non hanno ancora effetti — offrirle qui aprirebbe una finestra su una
				// scelta che il resolver non sa applicare, cioe' un prompt che ferma la resolution per non
				// fare niente. La distanza fra i due elenchi e' misurata da un test, non lasciata implicita.
				BraceOpportunity.AllowedResponses =
					URTCatalogLibrary::BraceExecutableResponses(T->ReactionProfileId);

				// L'`OwnerId` si calcola **solo se una finestra si apre**: sotto la soglia di ADR-0004 §2
				// `AskReactionDecision` risponde `HoldImmediate` senza leggerlo, e costruire lo snapshot per
				// quel caso e' lavoro speso per un valore che nessuno guarda. Le due domande sono in
				// quest'ordine perche' la seconda dipende dalla prima.
				if (URTReactionOpportunityLibrary::RequiresDecisionBoundary(BraceOpportunity))
				{
					if (BlastAliveUnits.Num() == 0)
					{
						MakeCurrentSnapshot(BlastAliveUnits);
					}
					BraceOpportunity.Key.OwnerId = BlastAliveUnits.IndexOfByKey(T);
				}

				const FRTReactionDecision BraceDecision = AskReactionDecision(
					BraceOpportunity, BraceOpportunity.Key.OwnerId, T->bIsBotControlled);

				// ➕ **La decisione entra nel TurnLog** (v10, [D-047]), e non e' una rifinitura di
				// diagnostica: senza questa voce la ri-simulazione **perde lo scarto**.
				// `ArmRecordedReactionDecisions` non troverebbe la chiave, segnalerebbe «finestra non coperta
				// dalla traccia» e applicherebbe la scelta sicura — quindi l'unita' che aveva scartato
				// resterebbe ferma nel replay, e il Verifier accuserebbe la traccia di un difetto dello
				// **scrittore**.
				//
				// ⚠️ **Solo se una finestra si e' aperta davvero.** Il caso degenere — profilo base,
				// `HoldImmediate` — non produce voce, e non per risparmiare: `ArmRecordedReactionDecisions`
				// **scarta in ingresso** gli `HoldImmediate`, quindi una voce del genere resterebbe non
				// consumata a fine corsa e verrebbe riportata come **orfana** su una ri-simulazione riuscita.
				// Scriverla sarebbe un falso allarme costruito qui e diagnosticato altrove.
				//
				// 🔴 **E NON si scrive un `HoldNoDecider` prodotto DALLA ri-simulazione**, che e' un caso
				// diverso e piu' insidioso. Quell'esito ha due significati: in partita «un'unita' umana senza
				// UI», che e' un fatto da registrare; in replay «la traccia non copriva questa finestra», che
				// e' la **diagnosi di una lacuna**. Scriverlo nel TurnLog del replay farebbe apparire coperta
				// una finestra che non lo era: ridando quel log al Verifier, `ArmRecordedReactionDecisions`
				// troverebbe la chiave, `IsResponseAllowed` passerebbe, e una traccia **nota come incompleta**
				// si presenterebbe pulita. Una lacuna che sparisce dopo un giro e' peggio di una lacuna.
				// ⚠️ `RecordedDecisions.Num() > 0` e' cio' che distingue i due significati, e non c'e' un altro
				// modo: l'esito da solo non lo dice. Trovato da una code review.
				const bool bResimulating = RecordedDecisions.Num() > 0;
				const bool bLacunaDelReplay = bResimulating
					&& BraceDecision.Outcome == ERTReactionDecisionOutcome::NoDecider;

				if (BraceDecision.Outcome != ERTReactionDecisionOutcome::Immediate && !bLacunaDelReplay)
				{
					FRTTurnLogEntry BraceEntry;
					BraceEntry.Phase = ERTMatchPhase::Blast; // dove la finestra si e' aperta, non dove le
					                                         // finestre si aprono di solito
					BraceEntry.Category = ERTLogCategory::ReactionDecision;
					BraceEntry.Outcome = static_cast<uint8>(BraceDecision.Outcome);
					// ⚠️ **Dalla CHIAVE, non da un secondo letterale.** La prima stesura riscriveva qui
					// `FName(TEXT("Action.Brace"))`, mentre l'`OpportunityId` lo deriva da
					// `Key.ReactionDefId` — una sola identita' con due sorgenti indipendenti. Il giorno in cui
					// la chiave cambiasse (un `Brace` profilato, un rename di catalogo) la voce dichiarerebbe
					// un'azione e l'id ne nominerebbe un'altra: niente smetterebbe di compilare, il replay
					// continuerebbe a funzionare — aggancia solo l'`OpportunityId` — e a mentire sarebbe il
					// solo testo del referto.
					BraceEntry.ActionId = BraceOpportunity.Key.ReactionDefId;
					BraceEntry.OpportunityId =
						URTReactionOpportunityLibrary::DeriveOpportunityId(BraceOpportunity.Key);
					BraceEntry.SrcCell = T->Cell;
					BraceEntry.TgtCell = T->Cell;
					BraceEntry.Priority = URTCatalogLibrary::FindCoreAction(BraceEntry.ActionId).Priority;

					// 🔴 **Il TOKEN, che e' il motivo per cui la v10 esiste.** L'Overwatch lo lascia vuoto
					// perche' la sua risposta si deduce dall'esito; qui no — `Hold Ground` e `SIDESTEP`
					// condividerebbero `HoldChosen`/`ResponseChosen` senza dire QUALE, e il replay
					// ricostruirebbe `HOLD`, che in questa finestra non e' nemmeno legale.
					BraceEntry.ReactionResponse = BraceDecision.Response;

					AppendLogEntry(BraceEntry, T);
				}

				// La risposta si traduce in primitive del catalogo effetti (`spec-reaction-clash-e14.md` §5) e
				// non in un ramo per token: un `if (Response == "SIDESTEP")` qui sarebbe il branch per eroe che
				// [D-047] esiste per togliere, scritto una riga sotto la funzione che lo evita.
				// ⚠️ **Si ACCUMULA e non si assegna**, e i due `Max(0, …)` non sono prudenza generica: una
				// risposta che dichiarasse due `SelfReposition` con un `=` avrebbe applicato solo l'ultima,
				// silenziosamente e in un ordine deciso dal catalogo. Un `Amount` negativo — che nessun profilo
				// scrive oggi — invertirebbe la direzione della fuga trasformando uno scarto in un avvicinamento.
				// Nessuno dei due casi esiste nel catalogo attuale, ed e' proprio per questo che vanno chiusi
				// qui: il giorno in cui esistessero, non lo direbbe nessun test.
				int32 EscapeSteps = 0;
				for (const FRTActionEffectSpec& Effect :
					URTCatalogLibrary::BraceResponseEffects(T->ReactionProfileId, BraceDecision.Response))
				{
					if (Effect.Effect == ERTActionEffect::SelfReposition)
					{
						EscapeSteps += FMath::Max(0, Effect.Amount);
					}
				}

				if (EscapeSteps > 0)
				{
					// 🔴 **Lo scarto NON si applica qui: entra in `KTargets` come una spinta qualunque.**
					// Dentro questo ciclo le destinazioni degli altri bersagli si calcolano ancora sulle celle
					// dello snapshot, e muovere un'unita' adesso le farebbe dipendere dall'ordine di
					// iterazione — l'invariante #4. Passando di la' eredita anche il controllo di destinazione
					// contesa, che dev'essere la stessa regola per tutti quelli che si muovono in questo Blast.
					// 🔴 **FUORI dalla linea, non lungo di essa** — la correzione del 2026-08-19.
					// `HexKnockbackDestination` allontana dall'attaccante, cioe' manda l'unita' **dove la
					// spinta voleva**: e siccome il ramo `Braced` blocca gia' la spinta a qualunque distanza,
					// `SIDESTEP` cedeva una cella per ottenere cio' che `Hold Ground` dava gratis. Una risposta
					// strettamente dominata non e' una scelta, e un boundary che ne offre una costa un prompt
					// senza comprare niente. `FindSidestepCell` esce dalla linea; le due sotto-decisioni —
					// quale cella, e cosa vale se non ce n'e' nessuna — seguono il precedente di
					// `Reaction.HazardEscape` invece di aprirne uno secondo.
					//
					// ⚠️ `EscapeSteps` non entra piu' nella geometria: uno scarto e' **di una cella** per
					// definizione — «esci dalla linea» non ha un multiplo. Il valore resta letto perche' e' cio'
					// che DISTINGUE una risposta che sposta da una che non sposta, ed e' l'unico modo in cui il
					// catalogo puo' dirlo senza un ramo per token.
					const FRTCellId Escape = URTReactionLibrary::FindSidestepCell(
						Map, T->Cell, KnockFrom[T], T->Facing, KOccupied);
					if (Escape != T->Cell)
					{
						KTargets.Add(T);
						KFinal.Add(Escape);
						Sidestepped.Add(T);
						continue;
					}

					// Nessuna cella dove scartare — bordo, ostacolo, unita' dietro. Si ripiega sul
					// comportamento base invece di sprecare la scelta, ed e' l'opposto di
					// `Reaction.EmergencyDash` («se non c'e' dove andare si spreca»): li' una reazione si
					// consuma, qui `Hold Ground` non e' una risorsa. Chi sceglie di scartare non deve finire
					// meno protetto di chi non ha scelto affatto.
					AddLogEvent(FString::Printf(
						TEXT("%s: nessuna cella per scartare, tiene la posizione"), *T->GetName()), FRTLogSubject::Unit(T));
				}

				AddLogEvent(FString::Printf(TEXT("%s: irrigidito, la spinta non lo sposta"), *T->GetName()), FRTLogSubject::Unit(T));
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::Braced, &PushCause);
				continue;
			}

			const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
				KnockFrom[T], T->Cell, KnockDist[T], Map, KOccupied);
			if (Dest != T->Cell) { KTargets.Add(T); KFinal.Add(Dest); }
			else
			{
				// DESTINAZIONE IMPOSSIBILE (#420): bordo mappa, ostacolo o unita' subito dietro. La spinta e'
				// stata risolta e non ha dove andare — non e' una difesa, e finora era muta come le altre due
				// cause geometriche.
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::NoDestination, &PushCause);
			}
		}
		for (int32 a = 0; a < KTargets.Num(); ++a)
		{
			// Destinazione contesa (2+ verso la stessa cella): quei bersagli restano (ordine-indipendente).
			bool bContested = false;
			for (int32 b = 0; b < KTargets.Num(); ++b)
			{
				if (a != b && KFinal[a] == KFinal[b]) { bContested = true; break; }
			}
			if (bContested)
			{
				// Il SESTO modo di non muoversi, che `#420` non contava: due bersagli spinti verso la stessa
				// cella restano entrambi fermi. Era il piu' muto dei sei — nemmeno una riga di combat log.
				AppendDisplacementResistedEntry(KTargets[a], ERTDisplacementBlockReason::ContestedDestination,
					&PushCause);
				continue;
			}

			ARTUnit* T = KTargets[a];
			ApplyForcedDisplacement(T, KFinal[a], KnockFrom[T], PushCause,
				Sidestepped.Contains(T) ? TEXT("Scarto") : TEXT("Spinta"), Map, ERTMatchPhase::Blast);
		}
	}

	// --- Trazione (`Action.Pull`, CP 4.7): stessa disciplina della spinta, direzione opposta -------------
	// Il catalogo v0.1 §1 riserva la resistenza di `Action.Guard` alla "spinta": una trazione non viene
	// resistita da chi si e' messo in guardia — non e' un'estensione implicita, e' cio' che il testo dice.
	if (PullCount.Num() > 0)
	{
		TArray<FRTCellId> POccupied;
		for (ARTUnit* U : Units) { POccupied.Add(U->Cell); }

		TArray<ARTUnit*> PTargets;
		TArray<FRTCellId> PFinal;
		for (ARTUnit* T : Units)
		{
			const int32* Pulls = PullCount.Find(T);
			if (!Pulls || *Pulls != 1 || !IsValid(T) || !T->IsAlive()) { continue; }

			// `Reaction.Anchor` vale anche qui: il catalogo dice «ricevi `Push`/`Pull`», e l'ancora e' l'unica
			// delle tre difese che la trazione incontra — `Guard` il testo lo riserva alla spinta, e la riga
			// sopra lo dichiara. L'insieme e' lo stesso del ramo della spinta, riempito da UNA sola attivazione:
			// chi e' spinto e tirato nello stesso Blast non paga due reazioni.
			if (DisplacementReactions.CancelledDisplacements.Contains(Units.IndexOfByKey(T)))
			{
				AddLogEvent(FString::Printf(TEXT("%s: ancorato, la trazione non lo sposta"), *T->GetName()), FRTLogSubject::Unit(T));
				AppendDisplacementResistedEntry(T, ERTDisplacementBlockReason::Anchored, &PullCause);
				continue;
			}

			const FRTCellId Dest = URTHexCombatLibrary::HexPullDestination(
				PullToward[T], T->Cell, PullDist[T], Map, POccupied);
			if (Dest != T->Cell) { PTargets.Add(T); PFinal.Add(Dest); }
		}
		for (int32 a = 0; a < PTargets.Num(); ++a)
		{
			bool bContested = false;
			for (int32 b = 0; b < PTargets.Num(); ++b)
			{
				if (a != b && PFinal[a] == PFinal[b]) { bContested = true; break; }
			}
			if (bContested) { continue; }

			ARTUnit* T = PTargets[a];
			ApplyForcedDisplacement(T, PFinal[a], PullToward[T], PullCause, TEXT("Trazione"), Map,
				ERTMatchPhase::Blast);
		}
	}
}

void ARTTurnManager::MarkAttackerAbilitiesSpent(FRTBlastContext& Ctx)
{
	TArray<ARTUnit*>& Attackers = Ctx.Attackers;
	TArray<int32>& UsedAbilityIndex = Ctx.UsedAbilityIndex;

	// Attaccanti SOPRAVVISSUTI: qui si decide CHI paga e si assegna l'energia — due cose, e il nome dice
	// la prima. A scrivere il cooldown e' `SpendStartedAbilities`, l'unico punto che lo fa (`#1451`).
	//
	// ⚠️ **La guardia `IsAlive()` resta QUI e non si sposta**: per un attaccante «spesa» significa
	// *sopravvissuto alla fase*, non *partita* — e' l'unico dei cinque punti in cui il criterio si conosce
	// solo a danno risolto. Portarla nella passata unica farebbe smettere di pagare anche il curatore che
	// cade a meta' Blast, che oggi paga: un cambio di gioco, non una pulizia.
	for (int32 i = 0; i < Attackers.Num(); ++i)
	{
		ARTUnit* Attacker = Attackers[i];
		if (!IsValid(Attacker) || !Attacker->IsAlive())
		{
			continue;
		}
		const URTActionData* Ability = Attacker->GetAbility(UsedAbilityIndex[i]);
		Ctx.MarkAbilitySpent(Attacker, UsedAbilityIndex[i]);
		if (Ability && Ability->EnergyCost > 0)
		{
			AddLogEvent(FString::Printf(TEXT("Ultimate! %s"), *Attacker->GetName()), FRTLogSubject::Unit(Attacker));
		}
		else
		{
			Attacker->Energy = URTCombatLibrary::GainEnergy(Attacker->Energy, Attacker->EnergyOnHit, Attacker->MaxEnergy);
		}
	}
}

void ARTTurnManager::ApplyControlStatuses(FRTBlastContext& Ctx)
{
	const URTHexMapAsset* Map = Ctx.Map;
	TArray<ARTUnit*>& Units = Ctx.Units;
	TArray<FRTUnitCombatState>& States = Ctx.States;
	TArray<ARTUnit*>& StatusTargets = Ctx.StatusTargets;
	TArray<FGameplayTag>& StatusTags = Ctx.StatusTags;
	TArray<int32>& StatusDurations = Ctx.StatusDurations;

	// --- Purificazione (CP 7.5, `#505`): il punto di valutazione degli STATI --------------------------
	// Gli stati sono RACCOLTI e non ancora applicati: e' il solo momento in cui `Reaction.Cleanse` puo'
	// annullarne uno. Dopo sarebbe una rimozione — osservabilmente simile, ma il TurnLog racconterebbe due
	// volte lo stesso turno («applicato», poi «tolto») per un evento che non e' mai avvenuto.
	//
	// La Prep non passa di qui: applica i suoi stati direttamente. Quindi il `Status.Root` che `Action.Brace`
	// mette a se stesso non e' intercettabile, ed e' corretto — una reazione anti-controllo che annullasse la
	// propria preparazione sarebbe un difetto.
	FRTReactionPassResult ControlReactions;
	RunReactionPass(ERTReactionPassPoint::BlastStatus,
		[&Units, &StatusTargets, &StatusTags](int32 SelfId, ERTReactionTrigger)
		{
			ARTUnit* Self = Units.IsValidIndex(SelfId) ? Units[SelfId] : nullptr;
			if (!Self) { return FRTReactionTriggerHit{ false, INDEX_NONE }; }

			for (int32 s = 0; s < StatusTargets.Num(); ++s)
			{
				if (StatusTargets[s] != Self) { continue; }
				if (URTReactionLibrary::ControlSeverityRank(StatusTags[s]) == INDEX_NONE) { continue; }
				// Scatta solo per un controllo che CAMBIA qualcosa: chi e' gia' sotto quello stato non spende
				// l'attivazione per un rinnovo (deciso il 2026-08-12). Il limite che ne segue e' dichiarato:
				// il PROLUNGAMENTO di un controllo gia' attivo non e' intercettabile.
				if (Self->HasStatus(StatusTags[s])) { continue; }
				// Nessun `TriggeredBy`: chi ha applicato lo stato non serve a un effetto difensivo, e con due
				// attaccanti non ce ne sarebbe uno solo.
				return FRTReactionTriggerHit{ true, INDEX_NONE };
			}
			return FRTReactionTriggerHit{ false, INDEX_NONE };
		},
		Units, States, Map, ControlReactions);

	// QUALE controllo salta si decide qui, dove la lista in arrivo e' completa: **il piu' grave**, non il
	// primo raccolto. L'ordine di raccolta segue i colpi — cioe' CHI colpisce — e sprecherebbe l'attivazione
	// su uno `Slow` da attacco base (`Weapon.Suppressive`, l'ultimate) lasciando passare un `Root`.
	//
	// Si itera su `Units` e non sul `TSet`: l'insieme risponde a `Contains`, ma iterarlo darebbe un ordine
	// non deterministico (invariante #3). Qui non cambierebbe l'esito — un indice appartiene a una sola
	// unita' — ma la regola del progetto e' non dipenderne mai, non «non dipenderne quando si vede».
	TSet<int32> CancelledStatusIdx;
	for (int32 u = 0; u < Units.Num(); ++u)
	{
		if (!ControlReactions.CancelledControls.Contains(u)) { continue; }
		ARTUnit* Canceller = Units[u];
		if (!Canceller) { continue; }

		int32 BestIdx = INDEX_NONE;
		int32 BestRank = MAX_int32;
		for (int32 s = 0; s < StatusTargets.Num(); ++s)
		{
			if (StatusTargets[s] != Canceller || CancelledStatusIdx.Contains(s)) { continue; }
			const int32 Rank = URTReactionLibrary::ControlSeverityRank(StatusTags[s]);
			if (Rank == INDEX_NONE || Canceller->HasStatus(StatusTags[s])) { continue; }
			// `<` e non `<=`: a parita' di gravita' vince il PRIMO in ordine di raccolta, che e' l'ordine
			// canonico dei colpi.
			if (Rank < BestRank) { BestRank = Rank; BestIdx = s; }
		}

		if (BestIdx != INDEX_NONE)
		{
			CancelledStatusIdx.Add(BestIdx);
			AddLogEvent(FString::Printf(TEXT("%s: %s annullato dalla purificazione"),
				*Canceller->GetName(), *StatusTags[BestIdx].ToString()), FRTLogSubject::Unit(Canceller));
		}
	}

	// L'ultimate applica il proprio status ai bersagli sopravvissuti.
	for (int32 i = 0; i < StatusTargets.Num(); ++i)
	{
		if (CancelledStatusIdx.Contains(i)) { continue; } // annullato prima di esistere (CP 7.5)
		ARTUnit* Slowed = StatusTargets[i];
		if (IsValid(Slowed) && Slowed->IsAlive())
		{
			ApplyStatusLogged(Slowed, StatusTags[i], StatusDurations[i]);
			// #1077: la nascita entra nel TurnLog anche da qui. Senza, il Cleanup avrebbe registrato la
			// scadenza di uno stato che il log non aveva mai visto nascere — un replay che legge una morte
			// senza nascita e' l'asimmetria che quell'issue esiste per chiudere. Trovato in code review.
			if (StatusDurations[i] == ARTUnit::PersistentWhileOnCell || StatusDurations[i] > 0)
			{
				FRTTurnLogEntry Nato = MakeStatusBirthEntry(Phase, StatusTags[i], Slowed->Cell,
					StatusDurations[i], /*bFromTerrain=*/ false);
				AppendLogEntry(Nato, Slowed);
			}
			AddLogEvent(FString::Printf(TEXT("Status: %s"), *Slowed->GetName()), FRTLogSubject::Unit(Slowed));
		}
	}
}

void ARTTurnManager::SpendStartedAbilities(const FRTBlastContext& Ctx)
{
	// 🔴 **L'UNICO posto in cui un'azione pianificata del Blast paga il proprio cooldown** (`#1451` punto 3).
	//
	// Prima erano cinque, con cinque criteri: `ResolveCleanseActions` subito dopo le guardie di validita',
	// `CollectHealActions` dopo la portata ([D-200]), `ModifyArc` solo se l'op finiva in coda,
	// `ApplyInterrupts` dagli INTENTI con le eccezioni di `#1449`, `MarkAttackerAbilitiesSpent` dai colpi
	// sopravvissuti. Ogni azione nuova che potesse validarsi senza colpire ne voleva un sesto — ed e' la
	// ragione per cui il difetto e' stato trovato cinque volte in quarantotto ore (#1437, #1443, #1444,
	// #1445, #1449).
	//
	// ⚠️ **Quel che si e' unificato e' il GESTO, non il criterio**, e la distinzione e' la sostanza della
	// correzione. «L'azione e' partita?» resta a chi raccoglie, perche' e' l'unico che sa cosa puo' sapere in
	// quel momento: la portata si conosce in pianificazione, un colpo prodotto no, e la sopravvivenza di chi
	// attacca si sa solo a danno risolto. Pretendere un criterio solo qui avrebbe voluto dire scrivere un
	// `switch` sull'azione — cioe' rifare le cinque regole con un nome nuovo.
	//
	// ⚠️ **E per la stessa ragione qui NON si riguarda `IsAlive()`**: chi cura o purifica e' vivo all'inizio
	// del Blast, chi attacca deve esserlo alla fine. Un controllo unico in questo punto farebbe smettere di
	// pagare il curatore che cade a meta' fase — un cambio di comportamento travestito da pulizia.
	//
	// ⚠️ **Fuori di qui, e dichiarato**: le REAZIONI (`ResolveInterceptions`, `RunReactionPass`) non sono
	// azioni pianificate ma inneschi condizionali, e a governarle e' [D-092] col proprio contatore di
	// attivazioni; le altre fasi (Prep, Move, Dash, Environment) hanno tempistiche proprie, e ricondurle a
	// [D-200] e' una decisione che non e' stata presa.
	//
	// ⚠️ **Un accoppiamento latente, dichiarato perche' oggi e' irraggiungibile e domani forse no.**
	//
	// `CanUseAbility` e' `IsAbilityUsable(GetAbilityCooldown(Index), Energy, EnergyCost)`: legge **il
	// cooldown E l'energia**, cioe' entrambe le cose che questa passata ha differito. Quattro punti la
	// chiamano dopo che qualcuno ha annotato — `ResolveInterceptions`, `RunReactionPass(BlastHits)`,
	// `RunReactionPass(BlastDisplacement)` e `RunReactionPass(BlastStatus)`, quest'ultimo anche dopo
	// `MarkAttackerAbilitiesSpent`.
	//
	// MISURATO il 2026-08-27, e sono due condizioni indipendenti che oggi non si verificano:
	// - **energia**: nessuna delle sei reazioni spedite dichiara `EnergyCost` (default 0), quindi
	//   l'ultimate differita non puo' rendere attivabile una reazione che prima non lo era;
	// - **cooldown**: perche' si vedesse, un'unita' dovrebbe avere lo STESSO indice come azione principale
	//   e come reazione. `CollectAttackIntents` non filtra su `Def.Slot`, quindi il caso e' costruibile —
	//   ma i due percorsi di produzione che armano una reazione (`ARTPlayerController` e il bot) scrivono
	//   `PlannedReactionAbility` e RITORNANO, senza mai toccare `PlannedAbilityIndex`. Ci arriva solo chi
	//   scrive il piano a mano: test e scenari.
	//
	// Chi spedira' una reazione con un costo, o rendera' pianificabile come principale un'abilita' di slot
	// reazione, guardi qui prima.
	//
	// L'ordine e' quello di annotazione, che e' l'ordine dei pass: `ConsumeAbility` scrive su unita' diverse,
	// quindi non c'e' esito che dipenda dall'ordine — ma un array, e non una `TMap`, perche' la regola del
	// progetto e' non dipendere mai dall'ordine di iterazione, non «non dipenderne quando si vede».
	// I due array si riempiono in una sola istruzione (`MarkAbilitySpent`), che ne e' l'unico scrittore:
	// la cardinalita' e' un invariante del tipo, non una condizione da tollerare. Dichiararlo qui evita di
	// suggerire al prossimo lettore che possano divergere — e quindi di «aggiustare» lo sbilanciamento
	// invece di preservare l'accoppiata.
	//
	// ⚠️ **`check` e non `checkSlow`**: `checkSlow` e' attivo solo sotto `DO_GUARD_SLOW`, cioe' in Debug —
	// ne' l'Editor Development su cui gira l'automation ne' la Shipping lo compilano. Un invariante
	// dichiarato con `checkSlow` non e' verificato da nessuna build che questo progetto produce davvero.
	check(Ctx.SpentActors.Num() == Ctx.SpentAbilityIndex.Num());

	for (int32 i = 0; i < Ctx.SpentActors.Num(); ++i)
	{
		ARTUnit* Attore = Ctx.SpentActors[i];
		// ⚠️ Nessuna unita' viene DISTRUTTA dentro il Blast — `DestroyDefeatedUnits` gira in `ConcludeTurn`,
		// dopo — quindi questa guardia oggi non scatta mai. Resta perche' il contesto tiene puntatori grezzi,
		// come `Attackers`: se un giorno un pass distruggesse un attore, saltare e' meglio che dereferenziare.
		if (!IsValid(Attore))
		{
			continue;
		}
		Attore->ConsumeAbility(Ctx.SpentAbilityIndex[i]);
	}
}
