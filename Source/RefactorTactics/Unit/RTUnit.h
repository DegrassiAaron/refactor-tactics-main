#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/RTTypes.h"
#include "Ability/RTActionDef.h" // ERTMovementStyle: le rotazioni legali sono una proprieta' dello STILE
#include "Turn/RTDeclaredCondition.h" // la condizione dichiarata vive nel piano dell'unita'
#include "Selection/RTSelectable.h"
#include "Map/RTMapVisuals.h" // #983: RTCellTopZ si include invece di ricopiarlo in un commento
#include "RTUnit.generated.h"

class UStaticMeshComponent;
class UArrowComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URTActionData;
class URTHeroData;

/**
 * Unita' segnaposto per il demo: una mesh su una cella, colorata per team e selezionabile.
 * E' un marker minimale (niente statistiche/abilita' qui): quelle arrivano in M2/M3.
 */
UCLASS()
class REFACTORTACTICS_API ARTUnit : public AActor, public IRTSelectable
{
	GENERATED_BODY()

public:
	ARTUnit();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 TeamId = 0;

	/**
	 * Il GRUPPO DI CONTROLLO dentro la squadra — `CP 19.3`, `#1124`: quale persona seduta in questa squadra
	 * comanda questa unita'.
	 *
	 * Lo assegna `ARTGameMode` all'allestimento con `URTCombatLibrary::ControlGroupForUnit`, dall'indice
	 * dell'unita' nella propria squadra. ⚠️ **Non e' un secondo `TeamId`**: due unita' di gruppi diversi
	 * restano compagne di squadra — non si colpiscono, condividono la conoscenza — e cambia solo **chi le
	 * muove**.
	 *
	 * Default `0`, che con `UnitsPerPlayer == UnitsPerTeam` — la v0.1 — e' anche l'unico gruppo esistente.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 ControlGroup = 0;

	/**
	 * Identita' STABILE di questa istanza per tutta la partita (#405, [D-063]). Parte da `1`: lo `0` resta
	 * libero e significa «nessuna unita' dichiarata», che e' cio' che dice una voce ambientale del TurnLog.
	 *
	 * Non e' l'identita' del SIMULATORE, ed e' la ragione per cui questo campo esiste: quella e' l'indice in
	 * `MakeCurrentSnapshot`, che filtra i vivi e viene ricostruito a ogni fase — quindi scala appena qualcuno
	 * muore. Peggio: `DestroyDefeatedUnits` distrugge l'Actor a fine turno, quindi nemmeno il pointer
	 * sopravvive alla partita. Una traccia che si rilegge a partita finita ha bisogno di un intero che non si
	 * muova, ed e' questo.
	 *
	 * La assegna `ARTTurnManager::EnsureMatchRoster()` una volta sola, alla prima risoluzione. Chi crea
	 * l'unita' non deve valorizzarla: sarebbe una seconda sorgente di identita', e diverrebbero due.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 StableUnitId = 0;

	/** Numero massimo di celle percorribili in un turno (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 MoveRange = 4;

	/**
	 * Se vero, l'unita' e' pianificata automaticamente dal bot.
	 *
	 * ⚠️ **Chi lo scrive, misurato il 2026-08-29.** Tre siti in produzione: `FRTMatchBootstrapper`
	 * quando spawna l'eroe, che legge la modalita' gia' risolta da chi ordina l'allestimento; il ramo dello
	 * stesso bootstrapper che passa al bot le unita' gia' posate nel livello — e scrive **solo `true`**, e solo con l'autobattle
	 * in vigore; e `FRTScenarioSession`. Gli altri commenti su questo campo rimandano qui invece di
	 * ripetere il conteggio: ripetuto in piu' posti invecchia in tutti e diverge in qualcuno.
	 *
	 * ⛔ **Un'unita' posata a mano nel livello non passa da nessuno dei tre**: entra in campo con il
	 * valore che porta dalla propria dichiarazione, ed e' la finestra che un conteggio dei writer non
	 * copre. Oggi quel valore e' il default qui sotto: nessuna mappa versionata contiene un `ARTUnit`, e
	 * `Content/RT/` non nomina questo campo, quindi nessun override lo sovrascrive.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Unit")
	bool bIsBotControlled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FRTCellId Cell;

	/**
	 * Orientamento AUTOREVOLE (CP 16.1): stato di gioco, non la rotazione della mesh.
	 *
	 * Convive con `bFaceMovementDirection`, che resta presentazione e continua a interpolare lo yaw: la
	 * differenza e' che questo campo lo leggono le REGOLE, e a fine playback la mesh deve atterrare qui. Se
	 * i due divergessero, il giocatore leggerebbe una cosa e ne subirebbe un'altra.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Unit")
	FRTCellId PlannedCell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxHealth = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	/** Portata dell'attacco base, in celle (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackRange = 5;

	/** Danno dell'attacco base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackPower = 30;

	/**
	 * Statistiche del catalogo eroi v0.1 senza ancora un consumatore in partita: la vista è LOS/FoW (non
	 * costruito), la resistenza push base è distinta da quella temporanea di `Status.Guarded`
	 * (`URTCombatLibrary::GuardResistedPushDistance`, l'unica oggi applicata). Esistono qui perché il DoD di
	 * CP 6.1 le vuole DATI sull'unità, non perché qualcosa le legga già.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 VisionRange = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 PushResistance = 0;

	/**
	 * Soglia d'udito dell'eroe (D-041): COMPENSA la vista invece di seguirla — chi vede lontano sente meno.
	 * Il valore canonico vive nel catalogo (`URTHeroData::HearingThreshold`) e arriva qui da
	 * `ConfigureFromHeroData`; il default 5 vale solo per un'unità che non è stata configurata da un eroe.
	 *
	 * Serve a `URTAcousticPropagationLibrary::IsAudible`, che prende la soglia come parametro e prima di
	 * questo campo non aveva modo di ottenerla da un'unità in partita.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 HearingThreshold = 5;

	/** Affinità e debolezza ambientale dell'eroe (identità per le combo fra eroi, es. Gadget su bersaglio Wet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName Affinity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName Weakness;

	/**
	 * Il **Reaction Profile** che il `Brace` di quest'unita' arma (E14.7, [D-047]). `NAME_None` = profilo base.
	 *
	 * ⚠️ **Trasportato e non risolto dall'`HeroId`**, benche' l'unita' porti anche quello: il resolver legge
	 * l'unita', non il catalogo eroi, e farlo risalire dall'eroe significherebbe che ogni sito che valuta una
	 * finestra deve conoscere `URTHeroCatalogLibrary` — cioe' una dipendenza in piu' per ottenere un dato che
	 * l'unita' puo' portarsi. E' la stessa scelta gia' fatta per `Affinity` e `Weakness`, che pure sarebbero
	 * deducibili dall'eroe.
	 *
	 * 🔵 Resta un **riferimento** al catalogo, non una copia delle risposte: `URTCatalogLibrary::FindReactionProfile`
	 * risolve l'id quando serve, quindi cambiare le risposte di un profilo non richiede di toccare le unita'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName ReactionProfileId;

	/**
	 * Eroe configurato via `ConfigureFromHeroData`. `NAME_None` = unità mai configurata da un eroe: oggi
	 * accade a un'unità piazzata a mano in livello o quando `ConfigureFromHeroData` riceve `nullptr` e
	 * ritorna fail-closed. ⚠️ Fino al 2026-08-13 questo commento diceva «unità legacy configurata via
	 * archetipo», e citava un percorso che non esiste più: `ConfigureAsArchetype` è stato rimosso.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName HeroId;

	/**
	 * Nome canonico/player-facing dell'eroe (D-120: Gadget · Phase · Riktor · Wraith), dichiarato dal
	 * catalogo e trasportato qui da `ConfigureFromHeroData`. `FText` perché è testo mostrato all'utente e
	 * deve restare localizzabile.
	 *
	 * ⚠️ **Non è lo Stable ID.** `HeroId` resta `Hero.Gadget` e non si rinomina: D-120 tiene i due piani
	 * separati, e la migrazione degli identificatori ha un blocker proprio ancora aperto (#716).
	 * Vuoto = nessun eroe l'ha dichiarato: la presentazione ricade su `ShortHeroName`, mai su stringa vuota.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FText HeroDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxEnergy = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Energy = 0;

	/** Energia guadagnata a ogni turno. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyPerTurn = 25;

	/** Energia guadagnata quando si porta a segno un attacco (non ultimate). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyOnHit = 15;

	/** Moltiplicatore di danno dell'ultimate (attacco a energia piena). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateMultiplier = 2;

	/** Raggio dell'area colpita dall'ultimate attorno al bersaglio (0 = singolo bersaglio). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateRadius = 1;

	/** Abilita' data-driven dell'unita' (se vuota, popolata con default in codice all'avvio). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	TArray<TObjectPtr<URTActionData>> Abilities;

	/**
	 * Abilita' ARMATA dal giocatore per la pianificazione. `INDEX_NONE` = **nessuna**, ed e' lo stato in cui
	 * un'unita' nasce e a cui torna dopo aver pianificato.
	 *
	 * 🔴 **Nasceva a `0`, e quello era il difetto** ([D-128](docs/decisions/RT_PDR_00_Decision_Log.md)). Con
	 * un default valido un'abilita' era **sempre** armata: non esisteva uno stato neutro, e cliccare un
	 * nemico pianificava sempre lo slot `0` — che il giocatore non aveva scelto, aveva trovato armato
	 * all'avvio. L'affordance mostrata prima del click non prediceva il click, che e' l'invariante di
	 * `docs/technical/systems/spec-pointer-interaction.md`.
	 *
	 * Ora il neutro esiste ed e' rappresentabile: `ERTPointerContext::Planning` con `INDEX_NONE`,
	 * `ERTPointerContext::Targeting` con un indice valido. Il giocatore arma con gli hotkey `1`-`4`, e
	 * l'HUD lo mostra — `RTHUD.cpp` evidenzia lo slot solo quando l'indice coincide, quindi nello stato
	 * neutro nessuno slot risulta acceso.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 SelectedAbilityIndex = INDEX_NONE;

	/** Abilita' pianificata per il turno (INDEX_NONE = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Ability")
	int32 PlannedAbilityIndex = INDEX_NONE;

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Combat")
	TObjectPtr<ARTUnit> PlannedAttackTarget = nullptr;

	/**
	 * Cella bersagliata dall'azione principale, in alternativa a `PlannedAttackTarget`: le aree si centrano
	 * su una CELLA, che puo' essere vuota (`Gadget.Overload` su un varco, una cella conduttiva senza nessuno
	 * sopra). Valida solo con `bAttackTargetsCell`.
	 *
	 * Chiude a meta' il limite dichiarato in `RTTurnManager` (CP 8.3): la pianificazione non aveva un
	 * bersaglio-cella. Resta scoperto il lato HUD — puntare una cella col mouse e' E11.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	FRTCellId PlannedAttackCell;

	/**
	 * Variante di abilita' attiva su questa unita' (es. `Riktor.KineticPanel.Reinforced`). `None` = i numeri
	 * del catalogo base.
	 *
	 * E' il **minimo** che rende consumabili i `Parameters` delle varianti, che fino a CP 9.5 nessuno leggeva:
	 * il catalogo dichiarava due compromessi (45 punti struttura per un turno solo, oppure 25 con una
	 * rotazione gratuita) senza che il gioco sapesse applicarli. CHI la sceglie, quando e con quale interfaccia
	 * resta l'epic E7: qui c'e' il campo e il suo consumatore, non il loadout.
	 *
	 * Il vincolo di catalogo — una sola abilita' fondamentale con variante, per eroe — e' la ragione per cui
	 * basta UN id per unita' invece di una mappa abilita -> variante.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Ability")
	FName ActiveVariantId;

	/**
	 * Bordo bersagliato dalle azioni che agiscono su una STRUTTURA di bordo (CP 9.5: `Action.CreateCover`,
	 * `Riktor.Reconfigure`). Valido solo con `bHasPlannedCoverEdge`.
	 *
	 * Serve un dato in piu' perche' una copertura sta su un BORDO, e una cella ne ha sei: con portata 3 la
	 * coppia (chi la erige, cella bersaglio) non basta a determinarlo — a portata 1 sarebbe bastata, ma il
	 * catalogo azioni dichiara 3. E' l'equivalente per i bordi di `PlannedAttackCell`, e come quello resta
	 * scoperto sul lato HUD: puntare un bordo col mouse e' E11.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	ERTHexDirection PlannedCoverEdge = ERTHexDirection::E;

	/**
	 * Vero se il piano dichiara `PlannedCoverEdge`. Serve un flag e non basta un valore di riposo: le sei
	 * direzioni sono tutte legittime, quindi nessuna puo' fare da «nessuna» senza diventare un caso speciale
	 * che il giocatore non puo' dedurre.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	bool bHasPlannedCoverEdge = false;

	/**
	 * Vero se l'azione principale mira a `PlannedAttackCell` invece che a un'unita'.
	 *
	 * Serve un flag e non basta «target nullo»: nel resolver `TargetUnitId == INDEX_NONE` significa gia'
	 * **bersaglio perso** (eliminato o mai valido), che degrada al fallback. Senza distinguerle, mirare a una
	 * cella verrebbe letto come un errore di pianificazione.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	bool bAttackTargetsCell = false;

	/**
	 * Percorso composito pianificato (waypoint risolti in celle, From = Cell incluso).
	 * Vuoto o < 2 celle = nessun movimento composito (si usa PlannedCell come destinazione singola).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FRTCellId> PlannedPath;

	/**
	 * Waypoint cliccati dal giocatore (esclusa la cella di partenza), da cui deriva PlannedPath.
	 * Vive sull'unita' cosi' la riselezione ne preserva l'editing (aggiungi/annulla step).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FRTCellId> PlannedWaypoints;

	/**
	 * Rotazione DICHIARATA in pianificazione (D-020, #291): «finito il movimento, guarda di la'».
	 *
	 * Il flag separato dal valore ha la stessa ragione di `bAttackTargetsCell`: `E` e' una direzione legittima
	 * e non puo' fare da «non dichiarato». Il resolver la consuma a fine Move — dopo l'orientamento derivato,
	 * che e' il `Current` su cui si misura la legalita' — e la azzera: una dichiarazione vale per il turno in
	 * cui e' stata fatta, come ogni altro pezzo del piano.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	bool bDeclaresPlannedFacing = false;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	ERTHexDirection PlannedFacing = ERTHexDirection::E;

	/**
	 * Movimento EFFETTIVAMENTE eseguito in questo turno: stile e rotta percorsa. Da questi due dipende
	 * l'insieme delle rotazioni legali (`URTFacingLibrary::LegalFacings`) — tre dopo un Move a budget, una
	 * sola dopo uno scatto lineare, sei da fermo — e nessuno dei due e' deducibile a fine turno: chi ha
	 * scattato ha `PlannedCell` uguale alla cella attuale esattamente come chi non si e' mosso.
	 *
	 * Scritti dalle fasi che muovono, letti solo dal consumo della rotazione dichiarata, azzerati con essa.
	 */
	UPROPERTY(Transient)
	ERTMovementStyle MovementStyleThisTurn = ERTMovementStyle::None;

	UPROPERTY(Transient)
	TArray<FRTCellId> WalkedThisTurn;

	/** Chiude il turno della rotazione: dichiarazione consumata e traccia del movimento scaricata. */
	void ClearDeclaredFacing()
	{
		bDeclaresPlannedFacing = false;
		MovementStyleThisTurn = ERTMovementStyle::None;
		WalkedThisTurn.Reset();
	}

	/** Abilita' di scatto pianificata per il turno (INDEX_NONE = nessuno scatto). Si risolve in fase Dash. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	int32 PlannedDashAbility = INDEX_NONE;

	/** Cella di destinazione dello scatto pianificato (valida solo se PlannedDashAbility e' impostata). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	FRTCellId PlannedDashCell;

	/**
	 * Reazione pianificata per il turno (INDEX_NONE = nessuna reazione pronta), slot indipendente da
	 * Movimento e Principale (CP 5.1, epic E5). Non ha una cella o un bersaglio: il trigger dichiarato
	 * dall'abilita' (`FRTActionDef::ReactionTrigger`) e' valutato sullo snapshot del Blast, non scelto qui.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	int32 PlannedReactionAbility = INDEX_NONE;

	/**
	 * La condizione dichiarata sulla reazione armata ([D-109]). Vuota = nessuna: si risponde a ogni trigger.
	 *
	 * Si scrive SOLO da `SetPlannedReactionCondition`, che la valida: un campo pubblico scrivibile a mano
	 * rimetterebbe in gioco proprio cio' che il validator esiste per impedire — una condizione che nessuna
	 * funzione sa valutare, scoperta al trigger invece che in pianificazione.
	 */
	UPROPERTY()
	FRTDeclaredCondition PlannedReactionCondition;

	/**
	 * Dichiara (o toglie) la condizione sulla reazione armata. Vero se il piano e' cambiato.
	 *
	 * Rifiuta senza toccare niente se non c'e' una reazione armata — una condizione orfana verrebbe ereditata
	 * dal prossimo armamento — o se il validator non ammette la condizione. Togliere la condizione e' sempre
	 * legittimo: e' il modo di tornare a «rispondi comunque».
	 */
	bool SetPlannedReactionCondition(const FRTDeclaredCondition& Condition);

	/**
	 * Azzera il piano di REAZIONE: lo slot e la sua condizione, insieme.
	 *
	 * Esiste per non poter dimenticare la seconda meta'. Azzerare lo slot e lasciare la condizione la
	 * renderebbe orfana, e il turno dopo il giocatore se la ritroverebbe addosso senza averla chiesta:
	 * `SetPlannedReactionCondition` rifiuta di crearne una all'ingresso, e senza questo metodo la stessa cosa
	 * rientrerebbe dalla porta di servizio del reset.
	 */
	void ClearReactionPlan();

	/**
	 * Quante REAZIONI l'unita' ha gia' attivato in questo turno ([D-092]). Si azzera a fine turno, insieme al
	 * piano.
	 *
	 * «Una attivazione per turno» sarebbe vera anche senza, per costruzione: `URTReactionLibrary::PassPointFor`
	 * assegna a ogni trigger **un solo** punto di valutazione, quindi nessuna unita' viene guardata due volte.
	 * Ma «vera per costruzione» significa che la garanzia vive in una proprieta' del mapping invece che in una
	 * regola scritta: il giorno in cui un trigger fosse valutato in due punti — o un punto girasse due volte —
	 * la regola cadrebbe **in silenzio**, senza che nessun test la stesse guardando.
	 *
	 * Questo contatore la rende esplicita e verificabile: e' la garanzia che D-092 prescriveva, e ora esiste
	 * come dato invece che come conseguenza. Pinnata da `Reactions.CounterBlocksASecondActivation`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 ReactionActivationsThisTurn = 0;

	/**
	 * Ordine di rimozione dichiarato per `Action.Cleanse` (CP 5.2): si purifica il PRIMO stato di questa lista
	 * che l'unita' possiede davvero, e uno solo.
	 *
	 * La scelta e' del giocatore ed e' fatta in PIANIFICAZIONE, mai a runtime: il catalogo lo pretende
	 * esplicitamente ("nessuna scelta implicita"), perche' un'euristica automatica ("togli il piu' dannoso")
	 * renderebbe l'esito impredicibile — e la predizione e' un pilastro di prodotto. Lista vuota = nessuna
	 * scelta dichiarata = nessuna rimozione (fail-closed): il Cleanse risolve senza effetto e lo registra,
	 * invece di indovinare per conto del giocatore.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FGameplayTag> PlannedCleansePriority;

	/**
	 * Imposta statistiche e azioni interamente da `URTHeroData` (CP 6.1, epic E6): nessun numero scritto qui.
	 *
	 * E' l'UNICA via per configurare un'unita'. Fino al 2026-08-10 ne esisteva una seconda,
	 * `ConfigureAsArchetype`, con le statistiche dei due archetipi scritte in C++: era gia' fuori da ogni
	 * partita — il GameMode schiera i quattro eroi da CP 6.6 — e sopravviveva solo perche' 24 file di test
	 * la usavano per costruire «un'unita' qualunque». Un secondo percorso di configurazione che nessuna
	 * partita esercita e' un motore parallelo con numeri propri: i test che lo usavano misuravano un gioco
	 * che non esisteva piu'.
	 *
	 * `Hero == nullptr` non configura nulla (fail-closed, come il resto del motore azioni: un'unita' senza
	 * dati non e' un'unita' con numeri a caso).
	 */
	void ConfigureFromHeroData(const URTHeroData* Hero);

	/**
	 * Applica un loadout a questa unità: le tre categorie, ognuna col **suo** verbo (`#1054`, CP 7.4).
	 *
	 * - **variante d'arma** → MODIFICA l'attacco base (indice 0). Non viene accodata: sarebbe un'azione in
	 *   più invece di un'arma diversa, ed è l'errore che l'harness faceva prima di `#63`.
	 * - **gadget** e **modulo di reazione** → CONCEDONO un'azione, che si accoda.
	 *
	 * Sta su `ARTUnit` e non su `URTCatalogLibrary` perché l'unità è il soggetto — e perché così la
	 * **partita e l'harness chiamano la stessa funzione**. Scriverla due volte le farebbe divergere, e
	 * uno scenario che verifica un equipaggiamento diverso da quello che il gioco monta non prova niente
	 * sul gioco: è tutto il valore che il corpus ha.
	 *
	 * ⚠️ **Non fa validazione**: che l'insieme sia 1+1+1 lo dichiara `ValidateLoadout`, e chi passa di qui
	 * lo ha già fatto — il loader per uno scenario, `DefaultLoadoutFor` per la partita (che restituisce
	 * *tutto o niente*, mai un insieme parziale). Un pezzo che non esiste viene saltato, non è un errore:
	 * fermarsi a metà lascerebbe l'unità equipaggiata in parte, che è peggio di non esserlo.
	 *
	 * ⚠️ **Idempotente per costruzione contro il difetto peggiore.** L'attacco base viene **duplicato**
	 * prima di essere modificato, perché `ConfigureFromHeroData` fa `Abilities = Hero->Actions`, cioè copia
	 * i *puntatori*: senza la duplicazione, due unità che condividessero lo stesso `URTHeroData` si
	 * applicherebbero la variante a vicenda — e `RangeCells` **accumula**, quindi la seconda perderebbe due
	 * celle invece di una. Oggi non succede (`GetHeroRoster()` ricostruisce a ogni chiamata), ma nulla lo
	 * pinna, e una cache del roster — ottimizzazione plausibile — lo introdurrebbe **in silenzio**.
	 */
	void EquipLoadout(const TArray<FName>& PieceIds);

	int32 NumAbilities() const { return Abilities.Num(); }
	URTActionData* GetAbility(int32 Index) const;

	/**
	 * Indice della prima azione di mobilita' RAPIDA (fase Dash del catalogo), o INDEX_NONE se l'unita' non ne
	 * ha. Il gate e' lo stesso di `ARTTurnManager::ResolveDash`: se qui e li' rispondessero in modo diverso, il
	 * bot pianificherebbe scatti che il resolver rifiuta — o non ne pianificherebbe affatto (#142).
	 */
	int32 FindDashAbilityIndex() const;

	/**
	 * Vero se questa unita' ha pianificato un MOVIMENTO NORMALE — una destinazione diversa dalla cella
	 * attuale, oppure un percorso con almeno un passo.
	 *
	 * 🔴 **Un solo posto, ed e' il motivo per cui esiste.** La stessa domanda si faceva in due file con gli
	 * operandi invertiti: `URTPlanValidationLibrary::MakePlanFor` decide se il piano CONTIENE una voce
	 * `Action.Move`, `ARTTurnManager::ResolveDash` decide se scrivere la voce che dichiara quel movimento
	 * SCARTATO. Sono la stessa regola: se una delle due guadagnasse un caso — un piano con soli
	 * `PlannedWaypoints`, un bot che cominciasse a scrivere `PlannedPath` — validatore e resolver
	 * risponderebbero diversamente sulla stessa unita' nello stesso turno, in silenzio.
	 *
	 * ⚠️ **Non e' la stessa condizione che `ResolveMovement` applica per PERCORRERE il piano**: quello esige
	 * anche che il percorso sia ancorato — `PlannedPath[0] == Cell` — e altrimenti ripiega su `PlannedCell`.
	 * Qui la domanda e' se un movimento sia stato DICHIARATO, non se sia percorribile. Oggi la differenza non
	 * e' raggiungibile (entrambi gli scrittori di `Cell` azzerano `PlannedPath`), ma il controllo d'ancoraggio
	 * esiste perche' quell'invariante non e' data per scontata: se un giorno cadesse, questo predicato
	 * direbbe «si muove» dove il resolver terrebbe l'unita' ferma.
	 *
	 * ⚠️ **Si legge PRIMA che `Cell` venga riscritta.** In `ResolveDash` la cella d'arrivo dello scatto
	 * sovrascrive `Cell`: chiamato dopo, il confronto con `PlannedCell` e' vero per OGNI scatto che ha
	 * spostato l'unita', anche per chi non aveva pianificato nulla. E' un difetto misurato il 2026-08-26,
	 * ed e' difeso da `PlayerInteraction.NoSupersededEntryOnADashWithoutAPlannedMove`.
	 */
	bool HasPlannedNormalMove() const { return PlannedCell != Cell || PlannedPath.Num() > 1; }

	/** Vero se l'abilita' e' pronta (non in ricarica) e c'e' energia sufficiente. */
	bool CanUseAbility(int32 Index) const;

	/**
	 * Lo scatto pianificato si applichera' davvero all'inizio della risoluzione?
	 *
	 * Tre condizioni, ed e' la stessa congiunzione che `ARTTurnManager::ResolveDash` valuta per decidere se
	 * muovere l'unita': mobilita' rapida dichiarata dal catalogo, azione utilizzabile (ricarica ed energia),
	 * destinazione diversa dalla cella corrente.
	 *
	 * 🔴 **Sta qui perche' ha due consumatori, e la seconda copia sarebbe divergibile.** Oltre al resolver la
	 * chiede l'ANTEPRIMA, che deve partire dalla cella post-scatto: la fase Dash precede il Blast, quindi chi
	 * carica e poi spara agisce da dove e' arrivato. Finche' la condizione stava solo dentro `ResolveDash`,
	 * l'anteprima non aveva modo di porre la domanda senza riscriverla.
	 *
	 * ⚠️ **Non promette la cella d'arrivo.** `ResolveDash` risolve la collisione simultanea (CP 4.8) e puo'
	 * fermare lo scatto prima della destinazione: questa risponde «lo scatto parte», non «lo scatto arriva».
	 */
	bool PlannedDashApplies() const;

	/** Cooldown residuo (turni) di un'abilita'. */
	int32 GetAbilityCooldown(int32 Index) const;

	/** Seleziona l'abilita' attiva del giocatore (se l'indice e' valido). */
	void SelectAbility(int32 Index);

	/** Avvia la ricarica dell'abilita' e ne consuma l'energia. */
	void ConsumeAbility(int32 Index);

	/** Decrementa i cooldown di tutte le abilita'. */
	void TickCooldowns();

	/**
	 * Applica lo stato di combattimento risolto (solo logico). Se HP<=0 l'unita' e' morta, ma NON viene
	 * distrutta subito: la rimozione visiva/distruzione e' differita (morte visiva differita, vedi TurnManager).
	 *
	 * Il danno assorbito dallo scudo erode PRIMA la parte temporanea (vedi AddTemporaryShield).
	 */
	void ApplyCombatState(int32 NewHealth, int32 NewShield);

	/**
	 * Aggiunge scudo TEMPORANEO: protegge per il turno corrente e scade nel Cleanup (ExpireTemporaryShield).
	 * E' la forma di protezione delle abilita' di supporto — il catalogo v0.1 dice «lo Scudo scade durante il
	 * Cleanup del turno», e senza scadenza si accumulava turno dopo turno rendendo i duelli interminabili
	 * (issue #96: una partita 2v2 durava 25 turni contro i 12 previsti).
	 */
	void AddTemporaryShield(int32 Amount);

	/** Rimuove la parte temporanea dello scudo (fine turno). Lo scudo BASE dell'unita' resta. */
	void ExpireTemporaryShield();

	/**
	 * Riporta lo scudo BASE al suo valore pieno ([D-224]): 5 punti che ogni unita' porta, non crescono e
	 * tornano interi a fine turno. Chiamata dal COSTRUTTORE — un'unita' esiste gia' protetta — e in coda al
	 * Cleanup, dove il temporaneo e' appena scaduto.
	 *
	 * ⚠️ **Non da `BeginPlay`, ed e' una correzione misurata**: i mondi di test costruiti con
	 * `UWorld::CreateWorld` non fanno partire `BeginPlay`, quindi lo scudo sarebbe esistito in partita e
	 * NON dove lo si verifica. Il costruttore gira su `NewObject` come su ogni `SpawnActor`.
	 *
	 * Non e' nemmeno il default del campo `Shield`: il valore lo dichiara `URTCombatLibrary::BaseShield`
	 * insieme alle altre costanti di combattimento, e un solo punto lo applica.
	 *
	 * La somma con `TemporaryShield` e' ridondante nella posizione attuale — li' vale sempre 0 — ma tiene
	 * l'invariante `Shield = base + temporaneo` vera se un giorno l'ordine delle due chiamate cambiasse.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Unit")
	void RechargeBaseShield();

	/** Quota dello scudo corrente che scadra' a fine turno (diagnostica/HUD). */
	int32 GetTemporaryShield() const { return TemporaryShield; }

	/** Nasconde la mesh e disabilita la collisione (morte visiva; la distruzione avviene a fine turno). */
	void HideForDefeat();

	bool IsAlive() const { return Health > 0; }

	/**
	 * Durata sentinella: lo stato NON scade da solo, vale finche' la cella sotto i piedi dell'unita' lo
	 * sostiene (CP 8.2, `spec-stati-temporanei-cp82.md` §3 D1). E' la forma di `Wet` sull'acqua bassa e di
	 * `Obscured` nel fumo: un contatore li farebbe scadere mentre l'unita' e' ancora nell'acqua.
	 * La revoca avviene nel Cleanup via RevokeCellBoundStatusesNotIn.
	 */
	static constexpr int32 PersistentWhileOnCell = -1;

	/**
	 * Applica uno status per Turns turni (non accorcia una durata gia' piu' lunga), oppure legato alla cella
	 * se Turns == PersistentWhileOnCell.
	 *
	 * Le due nature convivono senza sovrascriversi: chi e' bagnato dall'acqua E dall'abilita' di Phase resta
	 * bagnato per la durata di Phase anche dopo essere uscito dall'acqua.
	 *
	 * ⚠️ **Restituisce se ha SPENTO un `Burning`** (`#1314`). `ARTUnit` il TurnLog non ce l'ha — e' la
	 * stessa ragione per cui `TickStatuses` restituisce i tag scaduti invece di scriverli — quindi il
	 * momento in cui l'acqua spegne il fuoco puo' diventare una voce solo se questa funzione lo dichiara a
	 * chi la chiama. Prima era muto, e il replay non poteva dire perche' un'unita' avesse smesso di bruciare.
	 */
	bool ApplyStatus(FGameplayTag Tag, int32 Turns);

	/** Vero se lo status e' attivo (durata residua > 0). */
	bool HasStatus(FGameplayTag Tag) const;

	/**
	 * Stati attivi in forma ORDINATA e senza duplicati: quelli a durata (`StatusTurns`) piu' quelli legati
	 * alla cella (`CellBoundStatuses`), che sullo stesso tag possono coesistere.
	 *
	 * Serve al checksum di fine partita (CP 12.1): `HasStatus` risponde su un tag per volta, e un checksum
	 * non puo' indovinare quali chiedere. L'ordine e' parte del contratto — le due sorgenti sono `TMap` e
	 * `TSet`, la cui iterazione non e' deterministica (invariante #4), quindi enumerarle a valle non basta.
	 */
	TArray<FName> GetActiveStatusNames() const;

	/**
	 * Applica `Status.Marked` registrando la SQUADRA del marcatore (CP 8.2).
	 *
	 * Il catalogo promette "+6 al prossimo attacco **alleato**": senza sapere chi ha marcato, il marchio
	 * sarebbe una vulnerabilita' generica di cui approfitterebbe anche la squadra del bersaglio.
	 */
	void ApplyMarkedBy(int32 MarkerTeamId, int32 Turns);

	/** Squadra che ha piazzato il marchio, `INDEX_NONE` se l'unita' non e' marcata. */
	int32 GetMarkedByTeam() const { return MarkedByTeam; }

	/**
	 * Rimuove uno status PRIMA della sua scadenza naturale (`Action.Cleanse`, CP 5.2). Ritorna vero se lo
	 * status c'era davvero: il chiamante distingue "purificato" da "non c'era nulla da togliere" senza
	 * doverlo chiedere prima con `HasStatus`.
	 */
	bool RemoveStatus(FGameplayTag Tag);

	/**
	 * Decrementa la durata di tutti gli status A TERMINE; rimuove quelli scaduti. Non tocca i persistenti.
	 *
	 * **Restituisce i tag SCADUTI**, ordinati per nome (#1077). ⚠️ L'ordine e' esplicito e non quello di
	 * `StatusTurns`: e' una `TMap`, e far dipendere da lei l'ordine delle voci del TurnLog violerebbe
	 * l'invariante «niente dipendenza dall'ordine di `TMap`/`TSet`» — con la conseguenza concreta che
	 * l'hash del turno cambierebbe fra due esecuzioni identiche, che e' precisamente cio' che
	 * `HashTurnLogOrdered` esiste per rendere visibile.
	 *
	 * ⚠️ **Restituisce invece di scrivere sul log**, e non e' un dettaglio: `ARTUnit` il TurnLog non ce
	 * l'ha, ed e' la ragione per cui questo momento era muto. Chi lo registra e' il TurnManager.
	 */
	TArray<FGameplayTag> TickStatuses();

	/**
	 * Revoca gli stati legati alla cella che la cella corrente non sostiene piu' (Cleanup, CP 8.2).
	 * `Sustained` sono i tag dichiarati dal terreno su cui l'unita' ha TERMINATO il turno: chi e' uscito
	 * dall'acqua smette di essere bagnato subito, senza aspettare il turno successivo.
	 *
	 * Una durata esplicita in corso sullo stesso tag sopravvive: la revoca toglie solo la parte "finche'
	 * sulla cella".
	 */
	TArray<FGameplayTag> RevokeCellBoundStatusesNotIn(const TSet<FGameplayTag>& Sustained);

	/**
	 * Range di movimento tenendo conto degli status: **solo `Root`**, che lo azzera.
	 *
	 * `Slow` non passa piu' da qui da CP 4.7: e' un costo **per cella** nel pathfinding
	 * (`ARTTurnManager::MakeCurrentSnapshot` -> `MoveCostModifier`), non una riduzione flat del budget.
	 * Il corpo della funzione lo dice gia'.
	 */
	int32 GetEffectiveMoveRange() const;

	/**
	 * Portata effettiva dello SCATTO con gli status: **solo `Root`** la azzera.
	 *
	 * `Slow` NON la tocca, ed e' deliberato: vale «+1 al costo di OGNI cella attraversata» (catalogo v0.1
	 * §5), quindi morde il movimento a budget (`Action.Move`, `Action.Sprint`) e non le mobilita' lineari
	 * (`Dash`/`Charge`/`Leap`/`Reposition`), che un costo per cella non ce l'hanno. Il corpo della funzione
	 * lo dice gia'.
	 *
	 * ⚠️ Fino al 2026-08-10 questa riga diceva «Slow dimezza», ed era **falsa**: il dimezzamento era il
	 * meccanismo che `Ranger.Burst` applicava allo stesso stato **prima di CP 4.7**, sopravvissuto alla
	 * propria sostituzione. Il codice era corretto da allora; a mentire era il commento. Trovata dallo spec
	 * panel su CP 36.3, che da questa firma avrebbe dovuto DERIVARE la primitiva di `Slow`.
	 */
	int32 GetEffectiveDashRange(int32 BaseRange) const;

private:
	/** Quota dello `Shield` corrente che scade nel Cleanup: il resto e' scudo base dell'unita'. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat", meta = (AllowPrivateAccess = "true"))
	int32 TemporaryShield = 0;

	/** Status a termine: tag -> turni residui (sempre > 0; gli scaduti vengono rimossi). */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusTurns;

	/** Squadra che ha applicato `Status.Marked` (INDEX_NONE = nessun marchio attivo). */
	UPROPERTY()
	int32 MarkedByTeam = INDEX_NONE;

	/**
	 * Status legati alla cella (durata PersistentWhileOnCell). Contenitore separato e non una sentinella
	 * dentro StatusTurns: le due nature devono poter coesistere sullo STESSO tag senza che una cancelli
	 * l'altra (acqua bassa + `Phase.PressureJet` sono entrambe sorgenti di `Wet`).
	 */
	UPROPERTY()
	TSet<FGameplayTag> CellBoundStatuses;

	/** Cooldown residuo per abilita' (parallelo a Abilities). */
	UPROPERTY()
	TArray<int32> AbilityCooldowns;

	/**
	 * Riallinea `AbilityCooldowns` ad `Abilities`: va chiamata OVUNQUE il kit venga popolato o sostituito.
	 *
	 * Era dimensionato solo in `BeginPlay` e in `ConfigureFromHeroData`, quindi un'unita' configurata come
	 * archetipo in un world che non ha chiamato `World->BeginPlay()` — cioe' ogni world di test — teneva
	 * l'array VUOTO: `ConsumeAbility` trovava `IsValidIndex` falso e non scriveva, `GetAbilityCooldown`
	 * rispondeva sempre 0. Nessun test poteva accorgersene (#135).
	 *
	 * `SetNumZeroed` e non `Init`: azzera i nuovi slot senza cancellare i cooldown gia' scorrendo, cosi' la
	 * chiamata resta sicura anche se il kit cambia a partita iniziata.
	 */
	void SyncAbilityCooldowns();

	/** Popola Abilities con un set di default (attacco, colpo pesante, ultimate) se vuota. */
	void EnsureDefaultAbilities();

	/** Crea un'abilita' data-driven in codice. */
	URTActionData* MakeAbility(const FString& Name, int32 Range, int32 Power, int32 Area,
		int32 Cooldown, int32 EnergyCost, FGameplayTag Status, int32 StatusDur);

public:

	/** Semi-altezza della mesh (cilindro base ~100uu con scala Z 1.8 -> ~180, meta' = 90). */
	static constexpr float UnitHalfHeight = 90.f;

	/**
	 * Offset verticale del pivot rispetto al piano della cella (SOLO presentazione).
	 * Default = UnitHalfHeight (90) per il cilindro segnaposto col pivot al centro; impostare a 0 per
	 * personaggi skeletali col pivot ai piedi (via BP_Unit). Non tocca la logica: la griglia resta autoritativa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	float VisualZOffset = UnitHalfHeight;

	/**
	 * Se vero, durante il movimento visivo l'unita' si orienta verso la direzione di spostamento (solo yaw).
	 * Default false = comportamento invariato (il cilindro non ruota). I BP_Unit dei personaggi lo attivano
	 * cosi' la corsa (es. Jog_Fwd) punta dove vanno. Solo presentazione: non tocca la logica.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	bool bFaceMovementDirection = false;

	/**
	 * Vero mentre l'unita' e' in movimento nel playback del turno. Lo imposta il TurnManager; l'AnimBP lo legge
	 * (un solo Cast to RTUnit, uguale per ogni personaggio) per passare da idle a corsa, senza wiring per-unita'.
	 * Solo presentazione: non tocca la logica.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Unit")
	bool bIsMovingVisually = false;

	/**
	 * Velocita' che l'unita' DICHIARA mentre corre nel playback (cm/s). Non e' una velocita' simulata:
	 * l'unita' non ha un movement component e il playback la sposta per interpolazione, quindi la velocita'
	 * vera e' sempre zero. Serve agli AnimBP dei pack Paragon, che scelgono idle/corsa e la direzione del
	 * blendspace leggendo `GetVelocity()`. Solo presentazione: nessuna regola la legge.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	float VisualRunSpeed = 375.f;

	/**
	 * Classe di animazione applicata alla Skeletal Mesh che il Blueprint aggiunge, se ne ha una.
	 *
	 * 🔴 **Si assegna da qui e non nei `BP_Unit_*`, ed e' una scelta sul PESO del repository.** Gli
	 * AnimBlueprint dei pack Paragon pesano 650–735 KB l'uno: duplicarne quattro per ricablarne l'ingresso
	 * costerebbe ~2,8 MB contro gli 0,7 MB che pesa oggi tutto `Content/` versionato, e i `.uasset` non si
	 * comprimono per delta. `URTUnitAnimInstance` fa lo stesso lavoro in C++, e nessun binario cambia.
	 *
	 * ⚠️ **Non scavalca una scelta fatta in Blueprint**: se il componente porta gia' una `Anim Class`,
	 * quella vince e questa non viene applicata — provare un AnimBP a mano resta possibile senza
	 * ricompilare.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TSubclassOf<UAnimInstance> UnitAnimClass;

	/**
	 * 🔴 **Rotazione della MESH rispetto al forward dell'attore, in gradi di yaw.**
	 *
	 * Le skeletal di personaggio si modellano quasi sempre rivolte lungo **+Y**, mentre il forward di un
	 * attore Unreal e' **+X**. `ACharacter` compensa la differenza ruotando il proprio mesh component di
	 * `-90` — e' nel suo costruttore, e per questo la maggior parte dei progetti non incontra mai il
	 * problema. `ARTUnit` deriva da `AActor`: quella compensazione non c'e' mai stata, e il personaggio
	 * corre di traverso rispetto alla direzione di marcia.
	 *
	 * ⚠️ **E' un parametro perche' dipende dal pack**, non una costante: si applica al componente skeletal
	 * e si corregge guardando la freccia di `FacingArrow`, che punta sempre lungo il forward dell'attore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	float MeshYawOffset = -90.f;

	/**
	 * 🔴 **LOD fisso delle skeletal dell'unita'. `-1` = sceglie il motore.**
	 *
	 * Esiste per un difetto misurato, non per qualita' visiva: **le LOD dei pack Paragon rimuovono ossa**,
	 * e fra quelle rimosse ci sono le catene. Misurato sui quattro pack del roster, contando le ossa di
	 * catena nella lista di rimozione LOD di ciascuna mesh:
	 *
	 * | Pack | ossa di catena rimosse | effetto a schermo |
	 * |---|---|---|
	 * | Gadget | **0** | nessuno |
	 * | Wraith | **0** | nessuno |
	 * | Phase | **6** (`hip_chain_l/r_01..03`) | catenine ai fianchi, poco visibile |
	 * | Riktor | **13** (`l_hand_chain_01..04`, `chain_tip_r`, ...) | **le catene si stendono sullo schermo** |
	 *
	 * Quando il LOD cala quelle ossa spariscono dalle *required bones*, e i vertici che vi sono pesati si
	 * stirano. ⚠️ **Si vede solo da lontano** — a zoom massimo indietro e nella panoramica di inizio
	 * partita — ed e' la ragione per cui e' sopravvissuto: chi guarda l'unita' da vicino non lo incontra mai.
	 *
	 * ⛔ **Non e' un difetto dell'asset da correggere nell'asset**: i pack Paragon sono sorgente in sola
	 * lettura, e una modifica alle loro LOD andrebbe rifatta a ogni riscaricamento. Il prezzo di tenerli a
	 * LOD 0 e' trascurabile: in partita ci sono **quattro** personaggi.
	 *
	 * ⚠️ `-1` riapre il difetto ed e' deliberato: serve a misurare di nuovo, non a spedire.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 ForcedMeshLOD = 0;

	/**
	 * Freccia a terra che mostra **dove l'unita' e' rivolta**, cioe' il forward dell'attore.
	 *
	 * Serve a separare due difetti che a schermo si assomigliano: una mesh ruotata rispetto all'attore
	 * (`MeshYawOffset`) e un facing logico sbagliato. La freccia segue l'ATTORE — e' figlia del root —
	 * quindi se punta dove ci si aspetta ma il personaggio guarda altrove, il difetto e' nell'offset.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UArrowComponent> FacingArrow;

	/** La freccia si vede in partita. Spegnila quando la presentazione non ne ha piu' bisogno. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	bool bShowFacingArrow = true;

	/** Posiziona l'unita' al centro-mondo della cella esagonale, con la base appoggiata al piano. */
	void PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * Posizione-mondo che PlaceOnCell userebbe per InCell, senza modificare lo stato logico (per il playback).
	 * LayerHeight non ha default: su mappa multilivello e' un dato reale della mappa, non un extra opzionale.
	 */
	FVector WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const;

	/** Sposta solo la mesh (presentazione): NON cambia Cell ne' il piano. Usato dall'animazione del turno. */
	void SetVisualLocation(const FVector& World);

	/**
	 * Velocita' DICHIARATA, non simulata. `AActor::GetVelocity()` legge il movement component e questa unita'
	 * non ne ha: si muove per interpolazione di presentazione, quindi la velocita' vera sarebbe **sempre**
	 * zero e ogni AnimBP che la legge resterebbe fermo in idle senza un errore e senza un log.
	 *
	 * Qui la si ricava dallo stato che il TurnManager gia' scrive — `bIsMovingVisually` — e dalla direzione
	 * dell'ultimo spostamento visivo. E' l'ingresso che gli AnimBP dei pack Paragon leggono per scegliere
	 * idle/corsa e la direzione del blendspace.
	 *
	 * ⚠️ Solo presentazione: nessuna regola legge questo valore, e il risultato logico non ne dipende.
	 */
	virtual FVector GetVelocity() const override;

	// --- Eventi di presentazione del combattimento (montaggi): implementati nei BP_Unit, chiamati dal TurnManager.
	//     Se un BP non li implementa non succede nulla (nessun crash): la logica resta invariata (invariante #1).
	/** L'attaccante esegue il montaggio d'attacco (fase Blast). */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayAttackMontage();

	/** Il bersaglio reagisce al colpo subito. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayHitMontage();

	/** L'unita' eliminata esegue il montaggio di morte, prima della rimozione visiva. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayDefeatMontage();

	// IRTSelectable
	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	/** Colore del team (TeamId 0 -> Team0, altrimenti Team1). Pura: usata da ApplyTeamColor e testabile. */
	static FLinearColor TeamColorFor(int32 InTeamId, const FLinearColor& Team0, const FLinearColor& Team1);

	/**
	 * Ultimo segmento di uno Stable ID: `Hero.Gadget` -> `Gadget`.
	 *
	 * ⚠️ **Non e' piu' il nome mostrato a schermo, ed e' cambiato il 2026-08-13** (#715). Fino ad allora
	 * questa funzione ERA l'etichetta, e la sua descrizione lo diceva; oggi l'etichetta la sceglie
	 * `DisplayLabel`, che preferisce il nome canonico dichiarato dal catalogo (D-120) e chiama questa solo
	 * quando quel nome manca. Resta quindi il **ripiego**, non la regola.
	 *
	 * `NAME_None` -> `Fallback`, cosi' l'etichetta non sparisce mai. ⚠️ La descrizione precedente motivava
	 * quel caso con «unita' legacy da archetipo»: `ARTUnit::ConfigureAsArchetype` non esiste piu'. Il caso
	 * resta raggiungibile per altra via — un'unita' piazzata a mano in livello, o una che riceve
	 * `ConfigureFromHeroData(nullptr)` e viene lasciata dal `return` fail-closed.
	 *
	 * Pura e testabile: `RefactorTactics.Unit.ShortHeroNameFromStableId`.
	 */
	static FString ShortHeroName(FName InHeroId, const FString& Fallback);

	/**
	 * Etichetta da mostrare sopra l'unita': il nome CANONICO dichiarato dal catalogo (D-120), con ripiego
	 * sull'ID stabile quando non c'e'.
	 *
	 * Tre casi, e nessuno produce una stringa vuota — che e' il difetto che `ShortHeroName` esisteva per
	 * impedire e che un `FText` vuoto reintrodurrebbe in silenzio, perche' vuoto e' un valore legale:
	 *   1. `InDisplayName` valorizzato        -> il nome canonico (`Gadget`, `Phase`, ...);
	 *   2. `InDisplayName` vuoto              -> `ShortHeroName(InHeroId)`, cioe' l'ultimo segmento dell'ID;
	 *   3. anche `InHeroId` a `NAME_None`     -> `Fallback` (il nome dell'attore).
	 *
	 * Pura e testabile: la presentazione sceglie fra due fonti, non inventa.
	 */
	static FString DisplayLabel(const FText& InDisplayName, FName InHeroId, const FString& Fallback);

	/**
	 * Quota del CENTRO di un anello a terra sopra il piano della cella (unita' di mondo).
	 *
	 * 🔴 **Non e' un margine estetico: e' un vincolo geometrico, e sbagliarlo rende l'anello INVISIBILE.**
	 * Il tile della cella e' il prisma di `GetCellPrismMesh` schiacciato da `RTCellFlatScale`, quindi la sua
	 * faccia superiore sta a `RTCellTopZ` — meta' spessore — sopra il centro cella (`Map/RTMapVisuals.h`).
	 * L'anello e' un cilindro engine con scala Z `0.02`, cioe' semi-altezza `50 * 0.02 = 1.0`: perche' la
	 * sua faccia superiore emerga dal tile serve
	 *
	 *     Clearance + 1.0  >  7.5      ->      Clearance > 6.5
	 *
	 * `6.8` lascia **0.3** di margine, lo stesso che questa costante ha sempre tenuto.
	 *
	 * ⏱️ **Valeva `1.8` fino al 2026-08-28**, quando la faccia del tile stava a `2.5` e la soglia era
	 * `1.5`. Lo spessore della cella e' passato da `5` a `15` uu (`0.06 H`, `Map/RTMapVisuals.h`) e questa
	 * costante e' l'UNICA quota del progetto che ha dovuto seguirlo a mano: tutte le altre — i quattro lift
	 * di `RTHexMapActor.cpp`, `RTLastContactGhostZ`, i pannelli di muro, il rilievo — sono scritte come
	 * `RTCellTopZ + k` e sono salite da sole. ✅ **E non e' stato necessario accorgersene**: lo
	 * `static_assert` qui sotto ha rotto la build, che e' il motivo per cui esiste.
	 *
	 * 🔴 **Una stesura di #593 aveva messo `1.0`**, deducendolo dal `+1` della vecchia formula senza
	 * misurare il disco: l'anello sarebbe finito con la faccia a `2.0`, mezza unita' DENTRO il disco
	 * opaco, e identita' di squadra e anello di selezione sarebbero spariti a schermo. E' precisamente
	 * l'errore contro cui `RTHexMapActor.cpp` mette in guardia accanto a quelle costanti — *«le linee di
	 * debug disegnate SOTTO `RTCellTopZ` finiscono dentro il disco e diventano invisibili. E' successo
	 * davvero»* — trovato in code review, non a schermo.
	 *
	 * ✅ **Il legame con `RTCellTopZ` lo verifica ora il compilatore** (#983): quella costante e' uscita dal
	 * namespace anonimo di `RTHexMapActor.cpp` ed e' in `Map/RTMapVisuals.h`, quindi la disuguaglianza qui
	 * sopra si puo' asserire invece di raccontarla. Prima questa riga diceva che il legame *non* era
	 * verificato e rimandava a #983, e il numero `2.5` viveva in un commento su entrambi i lati.
	 */
	static constexpr float RingGroundClearance = 6.8f;

	/** Semi-altezza di un anello a terra: il prisma della cella (`RTCellPrismRadius`) per la sua scala Z. */
	static constexpr float RingGroundFlatScale = 0.02f;
	static constexpr float RingHalfHeight = RTCellPrismRadius * RingGroundFlatScale;

	/**
	 * 🔴 **L'invariante che rende visibile l'anello, verificata alla compilazione.**
	 *
	 * ⚠️ **Copre il margine, non la formula**: qui la quota-mondo del centro dell'anello si semplifica in
	 * `RingGroundClearance` — `RingLocalZ` restituisce `-VisualZOffset + RingGroundClearance`, e il pivot
	 * si somma — ma quella funzione vive nel `.cpp` e non e' `constexpr`, quindi non entra in uno
	 * `static_assert`. Se cambia LEI, questa riga resta verde e a cadere e'
	 * `RefactorTactics.Unit.RingClearsCellDisc`, che la chiama davvero per entrambi i pivot. Le due guardie
	 * servono a cose diverse e nessuna delle due sostituisce l'altra.
	 */
	static_assert(RingGroundClearance + RingHalfHeight > RTCellTopZ,
		"L'anello a terra finirebbe DENTRO il disco della cella e sparirebbe a schermo (#593, #983).");

	/**
	 * Offset Z LOCALE per portare un anello a terra (`TeamRing`/`SelectionRing`) al piano della cella.
	 *
	 * ⚠️ **Aveva un secondo parametro `ParentScaleZ` fino a #593**, e la sua sparizione e' il punto: gli
	 * anelli erano figli di `Mesh`, quindi la loro posizione relativa veniva moltiplicata per la scala Z
	 * del cilindro e andava divisa per riportarla al piano. Sotto un root **unitario** quel fattore non
	 * esiste, e un argomento che vale sempre `1` e' un dato che nessuno legge.
	 *
	 * ⚠️ Con lui e' sparita anche la guardia `ParentScaleZ == 0`: senza divisione non c'e' niente da
	 * proteggere. Pura, e pinnata da `RefactorTactics.Unit.RingLocalZ`.
	 */
	static float RingLocalZ(float VisualZOffset);

	/**
	 * 🔴 **Separazione fra i due anelli, e non e' cosmetica: senza, z-fightano.**
	 *
	 * `TeamRing` (raggio `1.6`) e `SelectionRing` (raggio `1.9`) sono due dischi CONCENTRICI, e fino al
	 * 2026-08-25 stavano alla stessa quota — «stessa quota, stesso valore», scritto per contratto. Con
	 * semi-altezza `RingHalfHeight` uguale, le loro facce superiori coincidevano **esattamente** nella
	 * corona interna, e a schermo l'unita' selezionata mostrava un lampeggio fra il colore di squadra e
	 * quello di selezione. Visto al PIE, non in code review.
	 *
	 * ⚠️ **Si separa ALZANDO il TeamRing, mai abbassando il SelectionRing.** Il margine sul disco della
	 * cella e' `RingGroundClearance + RingHalfHeight - RTCellTopZ`, cioe' **0.3**: scendere di un'unita'
	 * farebbe sprofondare l'anello dentro il disco, che e' esattamente il difetto contro cui lo
	 * `static_assert` qui sopra monta la guardia.
	 *
	 * ⚠️ **E le due quote restano derivate dalla STESSA sorgente** (`RingLocalZ`), che era la ragione del
	 * contratto originale: due chiamate indipendenti si desincronizzano al primo che cambia (code review
	 * di #593). Qui non si torna indietro su quello — si aggiunge un delta esplicito a una sola di esse.
	 */
	static constexpr float RingStackSeparation = 1.0f;

	static_assert(RingStackSeparation >= RingHalfHeight,
		"Separazione minore della semi-altezza: le facce dei due anelli tornerebbero a sovrapporsi.");

	/** Quota locale dell'anello di TEAM: sopra quello di selezione, cosi' il colore di squadra resta leggibile al centro. */
	static float TeamRingLocalZ(float VisualZOffset);

	/** Quota locale dell'anello di SELEZIONE: resta alla quota-terra di riferimento, e fa da cornice esterna. */
	static float SelectionRingLocalZ(float VisualZOffset);

	/**
	 * Se l'unita' va renderizzata, date le DUE variabili che lo decidono.
	 *
	 * 🔴 Pura, statica e in un posto solo perche' il difetto naturale e' calcolarla in due: un morto che
	 * «diventa noto» tornerebbe visibile. La morte vince sempre sulla conoscenza.
	 */
	static bool ShouldBeRendered(bool bAlive, bool bKnownToObserver);

	/**
	 * Se il CILINDRO SEGNAPOSTO va mostrato.
	 *
	 * 🔴 Il cilindro e' un segnaposto, e il posto non e' piu' vuoto quando l'eroe porta la propria skeletal:
	 * mostrarlo allora rimette un cilindro dentro il personaggio. Il predicato si CALCOLA da qui invece di
	 * rileggere cio' che il Blueprint ha impostato, cosi' il valore non dipende da quale delle due forme il
	 * `BP_Unit_*` abbia usato per nascondere il cilindro.
	 *
	 * ⚠️ **`SetVisibility` NON scavalca `bHiddenInGame`**, e una stesura precedente di questa riga diceva
	 * il contrario. Sono due flag distinti e un componente si disegna solo se `bVisible && !bHiddenInGame`:
	 * su un `BP_Unit_*` che nasconda il cilindro con `bHiddenInGame = true`, `SetVisibility(true)` non lo
	 * riporterebbe a schermo. Cio' che regge davvero e' piu' stretto: **sugli eroi il predicato calcola
	 * `false` in entrambi i casi** (`bHasHeroMesh` e' vero, quindi il cilindro va nascosto comunque), e le
	 * due forme coincidono per VERSO, non per una proprieta' di `SetVisibility`. Un'unita' **senza**
	 * skeletal il cui Blueprint usasse `bHiddenInGame = true` resterebbe invisibile mentre questo codice
	 * chiede di mostrarla: quel caso oggi non esiste nel repository, e se nascesse va risolto qui —
	 * togliendo il flag alla fonte o chiamando anche `SetHiddenInGame`, non fidandosi di `SetVisibility`.
	 *
	 * ⚠️ Nascosto NON significa scollegato: `Mesh` resta il proxy di click (QueryOnly + `ECR_Block`), e la
	 * collisione la decide `bRender`, non questo predicato.
	 */
	static bool ShouldShowPlaceholderMesh(bool bRender, bool bHasHeroMesh);

	/**
	 * Se l'ANELLO DI SELEZIONE va mostrato: serve che l'unita' si veda, che sia selezionata, e che un
	 * materiale di selezione esista (senza, il ripiego e' non mostrarlo affatto).
	 *
	 * 🔴 Prende `bSelected` come PARAMETRO e non legge `SelectionRing->IsVisible()`: dopo una refresh con
	 * `bRender == false` quella risponderebbe «no» anche su un'unita' selezionata, e la selezione si
	 * perderebbe al primo riavvistamento. Lo stato sta nel flag, la visibilita' e' la sua funzione.
	 */
	static bool ShouldShowSelectionRing(bool bRender, bool bSelected, bool bHasSelectionMaterial);

	/** Se l'ANELLO DI SQUADRA va mostrato: senza il suo materiale il ripiego e' il colore sul cilindro. */
	static bool ShouldShowTeamRing(bool bRender, bool bHasTeamRingMaterial);

	/**
	 * Dichiara se l'osservatore locale conosce questa unita'. REVERSIBILE, a differenza di `HideForDefeat`,
	 * che significa morte ed e' a senso unico per SEMANTICA.
	 */
	void SetKnownToObserver(bool bKnown);

	/**
	 * Opacita' della sagoma del ricordo. Pura: la dissolvenza e' PRESENTAZIONE e non ha effetti logici, ma
	 * la sua REGOLA e' testabile e va tenuta fuori dal Tick.
	 *
	 * `URTTeamKnowledgeLibrary::ContactLifetimeTurns` vale 1: il ricordo vive il turno successivo, poi basta.
	 * Un contatto con turno maggiore di quello corrente e' incoerente -> zero (fail-closed).
	 *
	 * 🔴 **Nessuna eta' produce `1.0`**: la sagoma e' SEMITRASPARENTE per specifica
	 * (`docs/technical/systems/conoscenza-parziale-visibile-spec.md` **S4**), e da quando l'unita' ignota
	 * sparisce e' l'unica cosa che il giocatore vede di quel nemico — nella cella dell'ULTIMO CONTATTO, non
	 * in quella vera. Fresca `0.75`, al turno dopo `0.45`, poi zero.
	 */
	static float GhostOpacityForContact(int32 ContactTurn, int32 CurrentTurn);

	/**
	 * Mostra/aggiorna/nasconde la sagoma dell'ultimo contatto (Task 6). `CellCenterWorld` e' il centro della
	 * cella del CONTATTO — lo stesso riferimento che darebbe `URTHexLibrary::AxialToWorld` per
	 * `FRTKnowledgeEntry::Cell` (Task 2) — mai la posizione ATTUALE di questo attore: `ContactGhost` porta
	 * `SetUsingAbsoluteLocation`/`Rotation`, quindi resta li' anche se l'unita' vera si e' spostata altrove.
	 *
	 * Fail-closed sull'eta' del contatto (`GhostOpacityForContact`): un `ContactTurn` scaduto o incoerente
	 * nasconde la sagoma invece di disegnarla a caso.
	 *
	 * La mesh/posa si copiano dalla skeletal VIVA del Blueprint (Step 6.1, `FindHeroSkeletal`): un'unita' col
	 * solo cilindro segnaposto non ha nulla da copiare, e la sagoma resta nascosta. Il materiale
	 * (`ContactGhostMaterial`) e' OPZIONALE: se `M_LastContactGhost` non risolve — non ancora creato, o
	 * rimosso — la sagoma resta comunque visibile col materiale di default della mesh: una sagoma non
	 * colorata, mai un crash.
	 */
	void UpdateContactGhost(const FVector& CellCenterWorld, int32 ContactTurn, int32 CurrentTurn);

	/**
	 * Spegne la sagoma dell'ultimo contatto, senza bisogno di un `ContactTurn` (Task 6b).
	 *
	 * ⚠️ Esiste perche' un chiamante che decide «non c'e' nulla da ricordare» — propria squadra, nemico
	 * VISTO ora, nemico senza voce nella vista (ricordo scaduto) — non ha un contatto vero da passare a
	 * `UpdateContactGhost`. Inventarne uno "incoerente" (es. dal futuro) solo per sfruttare il fail-closed di
	 * `GhostOpacityForContact` userebbe un CAMPO DATI come segnale di rendering: `ContactTurn` diventerebbe
	 * indistinguibile fra "contatto vero ma scaduto" e "spenta di proposito", ed e' esattamente il pattern
	 * che questo repository ha gia' pagato altrove. Qui lo spegnimento e' deciso dal rendering, non dal dato.
	 */
	void HideContactGhost();

protected:
	/** Vero finche' l'osservatore locale non dichiara il contrario: un'unita' nasce nota. */
	UPROPERTY()
	bool bKnownToObserver = true;

	/**
	 * Vero fra `OnSelected` e `OnDeselected`. E' lo STATO della selezione su questa unita', e nessuno lo
	 * deduce dai componenti: l'anello puo' essere nascosto perche' non si e' selezionati, perche' non si
	 * vede l'unita', o perche' manca il materiale, e le tre cose non si distinguono guardando un `bVisible`.
	 */
	UPROPERTY()
	bool bSelected = false;

	/**
	 * Ricalcola la visibilita' di TUTTI i componenti visivi dallo stato dell'unita'
	 * (`bKnownToObserver`, `bSelected`, `bShowFacingArrow`, i materiali, la vita).
	 *
	 * 🔴 **Deriva, non assegna.** Chi cambia stato — `SetKnownToObserver`, `OnSelected`, `OnDeselected`,
	 * `ApplyTeamColor` — muta il proprio flag e chiama questa: nessuno di loro scrive su un componente. Con
	 * W scrittori e F flag l'alternativa sono W×F congiunzioni sparse, e ognuna e' una che qualcuno
	 * dimentichera' — che e' esattamente come `SetKnownToObserver(true)` finiva per accendere l'anello di
	 * selezione su un nemico che nessuno aveva selezionato. Qui il posto che sa quali componenti esistono
	 * e' uno solo.
	 *
	 * 🔴 **Non usa `SetActorHiddenInGame`, ed e' il punto di questa funzione.** Quella propaga a TUTTI i
	 * componenti dell'actor, inclusa la sagoma dell'ultimo contatto (Task 6), che vive su questo stesso
	 * actor e deve vedersi **proprio quando l'unita' non si vede**. Nascondere l'actor renderebbe la sagoma
	 * inerte, e nessun test automatico lo prenderebbe: si vedrebbe solo in PIE.
	 */
	void RefreshComponentVisibility();

	/**
	 * La skeletal dell'EROE, aggiunta dal Blueprint `BP_Unit_*` (Step 6.1) — MAI `ContactGhost`, che e' un
	 * SECONDO `USkeletalMeshComponent`, nativo, per la sagoma del ricordo (Task 6). Un `FindComponentByClass`
	 * cieco potrebbe restituire l'uno o l'altro a seconda dell'ordine di registrazione dei componenti:
	 * l'esclusione qui e' esplicita ed e' per QUESTO che `RefreshComponentVisibility` non spegne mai la sagoma
	 * per sbaglio invece del personaggio vero (o viceversa).
	 */
	USkeletalMeshComponent* FindHeroSkeletal() const;

	virtual void BeginPlay() override;

	/**
	 * Applica `UnitAnimClass` alla Skeletal Mesh che il Blueprint ha aggiunto, se ne ha una e se non porta
	 * gia' una propria `Anim Class`. Un'unita' col solo cilindro segnaposto non ha niente da animare.
	 */
	void ApplyUnitAnimClass();

	/** Inchioda le skeletal dell'unita' a `ForcedMeshLOD` (vedi il perche' su quella proprieta'). */
	void ApplyUnitMeshLOD();

	/** Compensa l'orientamento con cui la skeletal e' modellata (vedi MeshYawOffset). */
	void ApplyMeshYawOffset();

	/** Visibilita' e quota della freccia di orientamento. */
	void ApplyFacingArrow();

	void ApplyTeamColor();

	/**
	 * Root NEUTRO (#593): non porta scala, e non deve portarne.
	 *
	 * ⚠️ **E' l'unico componente la cui scala viene ereditata da chi si attacca in Blueprint.** Finche' il
	 * root era `Mesh` — cilindro segnaposto, scala `(1.2, 1.2, 1.8)` — una Skeletal Mesh sotto veniva
	 * stirata di `1.8/1.2 = 1.5x`, e i `BP_Unit_*` lo compensavano a mano con «World/Absolute Scale»: un
	 * workaround da rifare su ogni BP nuovo, che nessun errore segnala se manca, e che comunque non
	 * fermava l'ingrandimento del 15% alla selezione.
	 *
	 * ⛔ **Non assegnargli una scala.** Se serve deformare il segnaposto, si deforma `Mesh`, che e' li'
	 * per quello. `RefactorTactics.Unit.RootIsNeutral` lo pinna.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/**
	 * Direzione (piana, normalizzata) dell'ultimo spostamento visivo. La aggiorna `SetVisualLocation` e la
	 * legge `GetVelocity`. Transient: e' stato di presentazione del playback corrente, non un dato salvato.
	 */
	UPROPERTY(Transient)
	FVector LastVisualDirection = FVector::ForwardVector;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

	/** Anello di team a terra: identita' di squadra visibile anche con personaggi skeletal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> TeamRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingDynMaterial;

	/** Materiale dell'anello (M_TeamRing, con parametro "Color"). Assente -> anello nascosto (fallback). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> TeamRingMaterial;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team0Color = FLinearColor(0.10f, 0.40f, 1.00f);

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team1Color = FLinearColor(1.00f, 0.20f, 0.15f);

	/** Anello di SELEZIONE a terra: riscontro visivo della selezione, visibile anche sui personaggi skeletal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> SelectionRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SelectionRingDynMaterial;

	/** Materiale dell'anello di selezione (parametro "Color"). Assente -> nessun anello di selezione (fallback). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> SelectionRingMaterial;

	/** Colore dell'anello di selezione (default giallo). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor SelectionColor = FLinearColor(1.00f, 0.85f, 0.10f);

	/** Scala base della mesh; l'evidenziazione di selezione la moltiplica, non la sostituisce. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FVector BaseMeshScale = FVector(1.2f, 1.2f, 1.8f);

	/**
	 * Materiale con un parametro vettoriale "Color" usato per il colore-team.
	 * Default: /Game/RT/Art/GlobalMaterials/M_Global_Tint (tint parametrico trasversale, usato anche
	 * dalla griglia). Se assente, l'unita' resta grigia.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> UnitMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Art/GlobalMaterials/M_Global_Tint.M_Global_Tint")));

	/**
	 * Sagoma dell'ultimo contatto (Task 6). Figlia di `SceneRoot` ma NON ne eredita il transform:
	 * `SetUsingAbsoluteLocation`/`Rotation` (costruttore) la sganciano, cosi' `UpdateContactGhost` puo'
	 * fissarla alla cella del RICORDO senza che seguisse l'attore vero altrove. `NoCollision` e
	 * `CastShadow = false`: e' presentazione pura, mai un proxy di click ne' un'ombra che tradisce la
	 * posizione vera attraverso un'illuminazione sbagliata.
	 *
	 * ⚠️ **Esclusa per identita' da `RefreshComponentVisibility`, `ApplyUnitAnimClass` e `ApplyMeshYawOffset`**
	 * (via `FindHeroSkeletal`, o perche' priva di mesh finche' `UpdateContactGhost` non gliene assegna una):
	 * deve vedersi PROPRIO QUANDO l'unita' vera non si vede, mai essere spenta dallo stesso percorso.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<USkeletalMeshComponent> ContactGhost;

	/**
	 * Materiale della sagoma (M_LastContactGhost: Translucent, Unlit, emissivo grigio monocromo, parametro
	 * scalare "GhostOpacity"). Assente -> `UpdateContactGhost` lascia il materiale di DEFAULT della mesh:
	 * la sagoma resta visibile, senza dissolvenza — degrada, non crasha.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> ContactGhostMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Characters/Shared/Materials/M_LastContactGhost.M_LastContactGhost")));

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ContactGhostDynMaterial;
};
