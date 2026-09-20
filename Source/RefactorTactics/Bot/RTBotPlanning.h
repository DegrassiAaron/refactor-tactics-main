// Copyright RefactorTactics. All Rights Reserved.
//
// I CONTRATTI della pianificazione dei bot: cosa il planner riceve, e cosa decide (#3013).
//
// 🔑 **Esistono perche' la decisione del bot possa essere provata senza spawnare un mondo.** Prima viveva
// dentro `ARTTurnManager::PlanBots` — 1 054 righe, la funzione piu' lunga del repository — e l'unico
// appiglio per provarla era `PlanBotsForTest()`, che richiede un `ARTTurnManager` vivo dentro un mondo.
// `CLAUDE.md` §11 dice dove appartiene: *«i bot generano intent, e usano gli stessi legal-action rules,
// canonical state, simulator e resolution rules dei player»*. L'orchestratore fa accadere quel pensiero;
// non lo ospita.
//
// ⚠️ **I fatti, non gli Actor.** `FRTBotUnitFacts` porta cio' che la decisione legge da `ARTUnit`, cosi'
// che il planner non abbia bisogno della classe. E' la stessa forma di `CollectPacingUnitFacts`, che la
// fetta precedente della stessa issue (#1818) aveva gia' scelto per la telemetria.
//
// ⛔ **Le abilita' restano `URTActionData*` e NON diventano fatti.** Sono asset: `NewObject` le costruisce
// senza mondo, quindi copiarne i campi qui sarebbe una seconda verita' del catalogo da tenere allineata —
// il difetto che `CLAUDE.md` §5 chiama «una seconda authority». Il planner le legge come le leggeva prima.

#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Replay/RTReplayAuditLibrary.h"
#include "RTBotPlanning.generated.h"

class URTActionData;

/**
 * Cio' che la decisione del bot legge da una `ARTUnit`, senza la `ARTUnit`.
 *
 * ⚠️ **`Index` e' l'indice nello snapshot, non `StableUnitId`**: e' la chiave con cui
 * `URTHexSimLibrary` indirizza le unita' (`Snapshot.Units[Index]`), e il planner la usa per i percorsi e
 * per l'occupazione. `StableUnitId` serve altrove — memoria d'inattivita' e audit del replay — ed e' per
 * questo che ci sono entrambi. Confonderli e' il difetto che [D-063] descrive.
 */
USTRUCT()
struct FRTBotUnitFacts
{
	GENERATED_BODY()

	/** Indice nello snapshot: `Snapshot.Units[Index]`. */
	UPROPERTY() int32 Index = INDEX_NONE;

	/** Identita' che sopravvive al turno: memoria d'inattivita' e audit ([D-063]). */
	UPROPERTY() int32 StableUnitId = INDEX_NONE;

	UPROPERTY() int32 TeamId = 0;
	UPROPERTY() bool bIsBotControlled = false;
	UPROPERTY() bool bAlive = true;

	/**
	 * Il nome con cui l'unita' compare nel combat log (`AActor::GetName`).
	 *
	 * ⚠️ E' **presentazione**, e sta qui solo perche' le righe di log lo contengono gia' oggi. Nessuna
	 * decisione lo legge, e nessuna deve cominciare: `CLAUDE.md` §11 tiene separato lo stato logico da
	 * quello di presentazione.
	 */
	UPROPERTY() FString DisplayName;

	UPROPERTY() FRTCellId Cell;
	UPROPERTY() ERTHexDirection Facing = ERTHexDirection::E;

	UPROPERTY() int32 Health = 0;
	UPROPERTY() int32 Shield = 0;
	UPROPERTY() int32 MaxHealth = 0;
	UPROPERTY() int32 AttackRange = 0;

	/** `HasStatus(TAG_Status_Unbalanced)` al momento della raccolta. */
	UPROPERTY() bool bUnbalanced = false;

	/** Le abilita' dell'unita', nell'ordine in cui `ARTUnit::GetAbility` le espone. Puo' contenere null. */
	UPROPERTY() TArray<TObjectPtr<URTActionData>> Abilities;

	/**
	 * `CanUseAbility(i)` per ogni abilita', raccolto **insieme** alle abilita'.
	 *
	 * 🔑 **Parallelo ad `Abilities` e non dedotto**: dipende da cooldown e risorse, cioe' da stato che
	 * vive sull'unita' e che il planner non deve conoscere per decidere.
	 */
	UPROPERTY() TArray<bool> bAbilityUsable;

	/** `FindDashAbilityIndex()`, oppure `INDEX_NONE`. */
	UPROPERTY() int32 DashAbilityIndex = INDEX_NONE;

	/** `GetEffectiveDashRange(Declared)` gia' applicato alla portata dichiarata dall'abilita' di scatto. */
	UPROPERTY() int32 EffectiveDashRange = 0;

	const URTActionData* GetAbility(int32 AbilityIndex) const
	{
		return Abilities.IsValidIndex(AbilityIndex) ? Abilities[AbilityIndex].Get() : nullptr;
	}

	bool CanUseAbility(int32 AbilityIndex) const
	{
		return bAbilityUsable.IsValidIndex(AbilityIndex) && bAbilityUsable[AbilityIndex];
	}

	int32 NumAbilities() const { return Abilities.Num(); }
};

/**
 * I pesi dell'utility scoring, copiati dalle `UPROPERTY` dell'orchestratore.
 *
 * ⚠️ **Sono interi, ed e' l'invariante #4**: niente float nella decisione competitiva. La sorgente che
 * vince in partita resta l'istanza di `ARTTurnManager` piazzata nel livello — un `.umap` serializza i
 * propri valori e batte i default C++ — e questa struct e' solo il loro trasporto.
 */
USTRUCT()
struct FRTBotWeights
{
	GENERATED_BODY()

	UPROPERTY() int32 WKill = 0;
	UPROPERTY() int32 WDamage = 0;
	UPROPERTY() int32 WThreat = 0;
	UPROPERTY() int32 WKiteViolation = 0;
	UPROPERTY() int32 WApproach = 0;
	UPROPERTY() int32 WElevation = 0;
	UPROPERTY() int32 WEngage = 0;
	UPROPERTY() int32 WEngageDecay = 0;
	UPROPERTY() int32 WObjective = 0;
	UPROPERTY() int32 WObjectiveFalloff = 0;
};

/**
 * Il piano deciso per UNA unita': esattamente i campi che `PlanBots` scriveva sull'`ARTUnit`.
 *
 * ⛔ **E' l'intera uscita, e va tenuto tale.** Se il planner cominciasse a scrivere un nono campo senza
 * passare di qui, l'orchestratore non lo applicherebbe e la decisione si perderebbe **in silenzio**.
 *
 * 🔴 **E NESSUN GATE LO IMPEDISCE, contrariamente a quanto questa riga ha dichiarato fino al 2026-09-20.**
 * Diceva *«`Bot.PlannerOutputCoversPlanFields` e' il gate che lo impedisce»*, e quel test non esiste:
 * `git grep -n "PlannerOutputCoversPlanFields" -- .` su tutto l'albero restituisce solo questo commento.
 * Il piu' vicino e' `Bot.PlanBotsWritesWhatTheValidatorReads`, che copre un'altra proprieta'.
 *
 * ⚠️ **Il nome NON e' stato sostituito con uno esistente**, ed e' deliberato: mettere qui un gate che
 * copre altro nasconderebbe un'assenza invece di dichiararla — e chi legge smetterebbe di cercarla. La
 * protezione oggi e' la lettura di chi tocca `PlanBots`, e questa riga dice che e' l'unica.
 */
USTRUCT()
struct FRTBotPlanDecision
{
	GENERATED_BODY()

	/** L'unita' a cui questo piano appartiene: indice nello snapshot. */
	UPROPERTY() int32 UnitIndex = INDEX_NONE;

	UPROPERTY() FRTCellId PlannedCell;
	UPROPERTY() TArray<FRTCellId> PlannedPath;
	UPROPERTY() TArray<FRTCellId> PlannedWaypoints;
	UPROPERTY() int32 PlannedAbilityIndex = INDEX_NONE;
	UPROPERTY() int32 PlannedDashAbility = INDEX_NONE;
	UPROPERTY() FRTCellId PlannedDashCell;
	UPROPERTY() int32 PlannedReactionAbility = INDEX_NONE;

	/**
	 * Il bersaglio dell'azione principale, come **indice nello snapshot**; `INDEX_NONE` = nessuno.
	 *
	 * 🔴 **Non e' un `ARTUnit*` e non lo diventa, ed e' una scelta obbligata**: `ARTUnit::PlannedAttackTarget`
	 * e `bAttackTargetsCell` sono **mutuamente esclusivi**, e l'esclusivita' vive in tre funzioni —
	 * `DeclareAttackOnUnit`, `DeclareAttackOnCell`, `ClearPlannedAttack` (`#2884`). L'header di `ARTUnit`
	 * vieta di scrivere quei campi a mano, *«finche' l'esclusivita' e' stata una convenzione invece che una
	 * funzione nessuno l'ha rispettata»*. ⛔ Chi applica questo piano **deve** passare da quelle funzioni.
	 *
	 * 🔴 **E anche qui il gate dichiarato non esiste**: la riga nominava `Bot.PlannerAppliesAttackThroughDeclare`,
	 * e `git grep` lo trova solo in questo commento. Due invarianti che il codice afferma sorvegliate e che
	 * nessuno misura, trovate insieme dalla ricognizione di `#543`. Il nome resta tolto e non sostituito,
	 * per la ragione scritta sopra `FRTBotPlanDecision`.
	 */
	UPROPERTY() int32 PlannedAttackTargetIndex = INDEX_NONE;
};

/**
 * Una riga di combat log prodotta dalla decisione, col suo soggetto.
 *
 * ⚠️ **Il soggetto e' un INDICE, non l'Actor.** `ARTTurnManager::AddLogEvent` vuole un `FRTLogSubject`,
 * che porta l'Actor; ma se il planner lo costruisse avrebbe di nuovo bisogno di `ARTUnit`, che e'
 * esattamente cio' da cui questa fetta lo libera. L'orchestratore lo ricompone quando emette.
 */
USTRUCT()
struct FRTBotLogLine
{
	GENERATED_BODY()

	UPROPERTY() FString Text;

	/** L'unita' a cui la riga si riferisce: indice nello snapshot, `INDEX_NONE` se nessuna. */
	UPROPERTY() int32 SubjectUnitIndex = INDEX_NONE;
};

/**
 * Quanto esce da una pianificazione: i piani, le righe di log, le voci d'audit.
 *
 * 🔑 **Le righe di log sono un'USCITA, non un effetto.** Prima il planner chiamava `AddLogEvent` otto
 * volte mentre decideva; ora le restituisce, e l'orchestratore le emette. E' la differenza fra una
 * funzione che si puo' provare senza mondo e una che no.
 */
USTRUCT()
struct FRTBotPlanningOutcome
{
	GENERATED_BODY()

	UPROPERTY() TArray<FRTBotPlanDecision> Decisions;

	/** Le righe destinate al combat log, nell'ordine in cui la decisione le ha prodotte. */
	UPROPERTY() TArray<FRTBotLogLine> LogLines;

	/**
	 * Le scelte di bersaglio, per il ricalcolo che l'audit del replay esegue.
	 *
	 * ⚠️ **Popolate solo se la pianificazione le ha richieste** (`bRecordReplay`): fuori dalla
	 * registrazione l'array resta vuoto, ed e' il comportamento di prima — l'audit non e' gratuito e non
	 * si paga in partita.
	 */
	UPROPERTY() TArray<FRTAuditBotDecision> AuditDecisions;

	/**
	 * Quanti punti di riduzione i piani SCELTI di questo turno si aspettano di scavalcare grazie alla
	 * direzione (`#649`, CP 16.2). Somma su tutti i bot che hanno deciso in questa chiamata.
	 *
	 * 🔑 **E' la meta' che mancava a un rapporto, e senza di lui l'altra non significa niente.** Le voci
	 * `Facing`/`RearHitBypassedCover` dicono quanti punti sono stati scavalcati DAVVERO; questo dice
	 * quanti il bot ne aveva contati decidendo. Il rapporto — realizzati su stimati — e' il *tasso di
	 * realizzo*, ed e' la seconda meta', rimasta sulla carta dal 2026-08-12, della decisione che ha
	 * introdotto il termine.
	 *
	 * ⛔ **Sta sull'ESITO e non su `FRTBotPlanDecision`, ed e' una separazione voluta.** Quella struct e'
	 * *«esattamente i campi che `PlanBots` scriveva sull'`ARTUnit`»*: un campo che l'orchestratore non
	 * applica la trasformerebbe in un misto di decisione e telemetria, e il gate che sorveglia la
	 * copertura dei campi non saprebbe piu' quale delle due sta contando.
	 *
	 * ⛔ **Solo i piani scelti, mai le candidate.** `ChooseBestPlan` valuta decine di candidate per unita'
	 * e ognuna porta la propria stima: sommarle tutte gonfierebbe il numeratore di un ordine di grandezza
	 * e il tasso direbbe che il bot sovrastima quando invece a sovrastimare sarebbe la misura.
	 *
	 * ⚠️ **Zero quando nessun piano scelto attacca attraverso una copertura** — cioe' su ogni board senza
	 * coperture di bordo, che oggi sono tutte quelle generate. Uno zero qui e' *«la condizione non si e'
	 * presentata»*, non *«il bot non sovrastima»*: chi misura deve distinguerli o non ha misurato niente.
	 */
	UPROPERTY() int32 PlannedCoverBypassedByFacing = 0;
};
