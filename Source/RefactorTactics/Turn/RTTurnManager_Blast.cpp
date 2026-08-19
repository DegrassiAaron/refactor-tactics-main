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
		Ctx.States.Add(FRTUnitCombatState(Unit->Health, Unit->Shield));

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
}

void ARTTurnManager::ResolveCleanseActions(const FRTBlastContext& Ctx)
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
		if (!Cleanse || Cleanse->Def.ActionId != FName(TEXT("Action.Cleanse")) || !Unit->CanUseAbility(CleanseIdx))
		{
			continue;
		}

		FGameplayTag Removed;
		for (const FGameplayTag& Candidate : Unit->PlannedCleansePriority)
		{
			if (Unit->RemoveStatus(Candidate))
			{
				Removed = Candidate;
				break; // UNO solo: e' il vincolo del catalogo, non un'ottimizzazione
			}
		}

		Unit->ConsumeAbility(CleanseIdx);
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumata qui: non deve diventare anche un intento d'attacco
		Unit->PlannedAttackTarget = nullptr;

		AddLogEvent(Removed.IsValid()
			? FString::Printf(TEXT("%s: purificato %s"), *Unit->GetName(), *Removed.ToString())
			: FString::Printf(TEXT("%s: nessuno stato da purificare"), *Unit->GetName()));
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
		if (!Heal || Heal->Def.ActionId != FName(TEXT("Action.Heal")) || !Unit->CanUseAbility(HealIdx))
		{
			continue;
		}

		// Bersaglio: chi e' stato scelto in pianificazione, oppure SE STESSI se non c'e' nessuno — il catalogo
		// dichiara che la cura «puo' bersagliare se stessi», e curare a vuoto non e' un'alternativa sensata.
		ARTUnit* HealTarget = Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget.Get() : Unit;
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedAttackTarget = nullptr;
		Unit->ConsumeAbility(HealIdx);

		// Portata dal catalogo, misurata come per ogni altra azione: una cura a distanza infinita sarebbe una
		// regola diversa da quella scritta.
		if (URTHexLibrary::HexDistance(Unit->Cell, HealTarget->Cell) > Heal->Def.RangeCells)
		{
			AddLogEvent(FString::Printf(TEXT("%s: cura fuori portata"), *Unit->GetName()));
			continue;
		}

		int32 Amount = 0;
		for (const FRTActionEffectSpec& Spec : Heal->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Heal) { Amount = Spec.Amount; break; }
		}
		if (Amount <= 0) { continue; }

		// Chi cura, accanto a da-dove: la cella del curatore non identifica un'unita' ([D-063]), e il TurnLog
		// deve dire chi ha agito (#405). `AddHeal` tiene allineati i quattro array paralleli.
		Ctx.AddHeal(Unit, HealTarget, Amount, Unit->Cell);
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
		if (PlannedNow && PlannedNow->Def.ActionId == FName(TEXT("Action.ModifyArc")))
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
					ArcRejected.ActionId = PlannedNow->Def.ActionId;
					ArcRejected.SrcCell = Unit->Cell;
					ArcRejected.TgtCell = ArcTarget->Cell;
					ArcRejected.Amount = static_cast<int32>(ERTActionInvalidReason::OutOfRange);
					AppendLogEntry(ArcRejected, Unit);
					AddLogEvent(FString::Printf(TEXT("%s: %s"),
						*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(ArcRejected)));

					// L'abilita' NON si consuma: il piano e' gia' stato azzerato sopra (si spende nel turno,
					// attivata o no), ma il cooldown paga solo cio' che ha davvero toccato la mappa.
					continue;
				}

				Unit->ConsumeAbility(ArcAbilityIndex);
				PendingArcOps.Add({ Unit->Cell, ArcTarget->Cell, Unit });
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
				URTFacingLibrary::RecordFacingChange(Attacker, TowardsTarget, ERTFacingOutcome::TargetingReoriented,
					ERTMatchPhase::Blast, TurnLog);
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
			AppendLogEntry(FallbackEntry, Unit);
			AddLogEvent(FString::Printf(TEXT("%s: %s"),
				*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(FallbackEntry)));

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
	TSet<int32> InterruptedAttackerIds;
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex)
			|| IntentDefs[Hit.IntentIndex].ActionId != FName(TEXT("Action.Interrupt")))
		{
			continue;
		}
		if (!Units.IsValidIndex(Hit.TargetId)) { continue; }

		// L'azione pianificata dal BERSAGLIO non si legge da `Unit->PlannedAbilityIndex`: il ciclo che ha
		// costruito `Intents`, qualche riga sopra, l'ha gia' CONSUMATA (azzerata) per ogni unita', bersaglio
		// compreso — e' cosi' che il turno evita di rieseguire due volte la stessa azione. Va cercata fra gli
		// `Intents` gia' catturati, nell'entrata che il bersaglio ha prodotto per SE STESSO (AttackerId ==
		// l'indice del bersaglio dell'Interrupt): e' li' che la definizione originale sopravvive al reset.
		int32 VictimIntentIdx = INDEX_NONE;
		for (int32 k = 0; k < Intents.Num(); ++k)
		{
			if (Intents[k].AttackerId == Hit.TargetId) { VictimIntentIdx = k; break; }
		}

		// Solo se il bersaglio ha DAVVERO pianificato un'azione interrompibile: un Interrupt su chi non ha
		// pianificato nulla (o ha pianificato Guard, non interrompibile) non ha niente da cancellare.
		if (VictimIntentIdx != INDEX_NONE && IntentDefs.IsValidIndex(VictimIntentIdx)
			&& IntentDefs[VictimIntentIdx].bCanBeInterrupted)
		{
			InterruptedAttackerIds.Add(Hit.TargetId);
			AddLogEvent(FString::Printf(TEXT("%s: interrotto da %s"),
				*Units[Hit.TargetId]->GetName(), *Units[Hit.AttackerId]->GetName()));
		}
	}
	// Il colpo dell'Interrupt STESSO non deve mai diventare un `FRTAttack`: non fa danno (`Effects` vuoto),
	// ma un colpo a Power 0 nell'array conterebbe comunque come "primo colpo" per `ApplyFirstHitDelta` —
	// consumando il bonus/malus di Guard/Exposed/Marked su un colpo fantasma invece che sull'attacco vero
	// che dovrebbe riceverlo. Si toglie insieme ai colpi degli interrotti, nello stesso filtro.
	Plan.Hits.RemoveAll([&InterruptedAttackerIds, &IntentDefs](const FRTHexAttackHit& Hit)
	{
		if (InterruptedAttackerIds.Contains(Hit.AttackerId)) { return true; }
		return IntentDefs.IsValidIndex(Hit.IntentIndex)
			&& IntentDefs[Hit.IntentIndex].ActionId == FName(TEXT("Action.Interrupt"));
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
				*Unit->GetName(), *Units[OriginalTarget]->GetName()));
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::NotTriggered);
		}

		// Chi REAGISCE. Nell'interposizione `SrcCell` e' la cella del protetto, non la sua: dedurre
		// l'unita' dalla voce darebbe l'unita' sbagliata ([D-063]).
		AppendLogEntry(Entry, Unit);
		AddLogEvent(FString::Printf(TEXT("%s: %s"), *Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Entry)));
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
		AddLogEvent(FString::Printf(TEXT("%s (%s -> %s)"), *URTTurnLogLibrary::DescribeEntry(NoLos),
			*Units[Blocked.AttackerId]->GetName(),
			bTargetsUnit ? *Units[Blocked.TargetId]->GetName() : TEXT("cella")));
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
			Change.bDestroyed ? TEXT("abbattuta") : TEXT("danneggiata"), Change.RemainingIntegrity));

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
				Change.bBroken ? TEXT("crollato") : TEXT("danneggiato"), Change.RemainingIntegrity));

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
	PendingArcOps.Sort([](const FRTPendingArcOp& A, const FRTPendingArcOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
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
		Entry.ActionId = FName(TEXT("Action.ModifyArc"));
		Entry.SrcCell = Op.From;
		Entry.TgtCell = Op.To;
		Entry.Amount = bRemoved ? 0 : 2; // turni di durata del ponte creato
		AppendLogEntry(Entry, Op.Actor);
		AddLogEvent(FString::Printf(TEXT("Collegamento %s: (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d)"),
			bRemoved ? TEXT("rimosso") : TEXT("creato"),
			Op.From.X, Op.From.Y, Op.From.Layer, Op.To.X, Op.To.Y, Op.To.Layer));
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
			Change.bBlocking ? TEXT("chiusa") : TEXT("aperta")));
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
				AddLogEvent(FString::Printf(TEXT("%s: ancorato, la spinta non lo sposta"), *T->GetName()));
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
				AddLogEvent(FString::Printf(TEXT("%s: in guardia, resiste alla spinta"), *T->GetName()));
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
				BraceOpportunity.Key.OwnerId = Units.IndexOfByKey(T);
				BraceOpportunity.Key.ReactionDefId = FName(TEXT("Action.Brace"));

				// Le ESEGUIBILI, non le dichiarate: `Profile.Grounding` e `Profile.Glance` sono contenuto
				// deciso da [D-132] e non hanno ancora effetti — offrirle qui aprirebbe una finestra su una
				// scelta che il resolver non sa applicare, cioe' un prompt che ferma la resolution per non
				// fare niente. La distanza fra i due elenchi e' misurata da un test, non lasciata implicita.
				BraceOpportunity.AllowedResponses =
					URTCatalogLibrary::BraceExecutableResponses(T->ReactionProfileId);

				const FRTReactionDecision BraceDecision = AskReactionDecision(
					BraceOpportunity, BraceOpportunity.Key.OwnerId, T->bIsBotControlled);

				// La risposta si traduce in primitive del catalogo effetti (`spec-reaction-clash-e14.md` §5) e
				// non in un ramo per token: un `if (Response == "SIDESTEP")` qui sarebbe il branch per eroe che
				// [D-047] esiste per togliere, scritto una riga sotto la funzione che lo evita.
				int32 EscapeSteps = 0;
				for (const FRTActionEffectSpec& Effect :
					URTCatalogLibrary::BraceResponseEffects(T->ReactionProfileId, BraceDecision.Response))
				{
					if (Effect.Effect == ERTActionEffect::SelfReposition)
					{
						EscapeSteps = Effect.Amount;
					}
				}

				if (EscapeSteps > 0)
				{
					// 🔴 **Lo scarto NON si applica qui: entra in `KTargets` come una spinta qualunque.**
					// Dentro questo ciclo le destinazioni degli altri bersagli si calcolano ancora sulle celle
					// dello snapshot, e muovere un'unita' adesso le farebbe dipendere dall'ordine di
					// iterazione — l'invariante #4. Passando di la' eredita anche il controllo di destinazione
					// contesa, che dev'essere la stessa regola per tutti quelli che si muovono in questo Blast.
					const FRTCellId Escape = URTHexCombatLibrary::HexKnockbackDestination(
						KnockFrom[T], T->Cell, EscapeSteps, Map, KOccupied);
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
						TEXT("%s: nessuna cella per scartare, tiene la posizione"), *T->GetName()));
				}

				AddLogEvent(FString::Printf(TEXT("%s: irrigidito, la spinta non lo sposta"), *T->GetName()));
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
				AddLogEvent(FString::Printf(TEXT("%s: ancorato, la trazione non lo sposta"), *T->GetName()));
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

void ARTTurnManager::ConsumeAttackerAbilities(FRTBlastContext& Ctx)
{
	TArray<ARTUnit*>& Attackers = Ctx.Attackers;
	TArray<int32>& UsedAbilityIndex = Ctx.UsedAbilityIndex;

	// Attaccanti sopravvissuti: consuma l'abilita' (energia+cooldown); se gratuita, accumula energia.
	for (int32 i = 0; i < Attackers.Num(); ++i)
	{
		ARTUnit* Attacker = Attackers[i];
		if (!IsValid(Attacker) || !Attacker->IsAlive())
		{
			continue;
		}
		const URTActionData* Ability = Attacker->GetAbility(UsedAbilityIndex[i]);
		Attacker->ConsumeAbility(UsedAbilityIndex[i]);
		if (Ability && Ability->EnergyCost > 0)
		{
			AddLogEvent(FString::Printf(TEXT("Ultimate! %s"), *Attacker->GetName()));
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
				*Canceller->GetName(), *StatusTags[BestIdx].ToString()));
		}
	}

	// L'ultimate applica il proprio status ai bersagli sopravvissuti.
	for (int32 i = 0; i < StatusTargets.Num(); ++i)
	{
		if (CancelledStatusIdx.Contains(i)) { continue; } // annullato prima di esistere (CP 7.5)
		ARTUnit* Slowed = StatusTargets[i];
		if (IsValid(Slowed) && Slowed->IsAlive())
		{
			Slowed->ApplyStatus(StatusTags[i], StatusDurations[i]);
			AddLogEvent(FString::Printf(TEXT("Status: %s"), *Slowed->GetName()));
		}
	}
}
