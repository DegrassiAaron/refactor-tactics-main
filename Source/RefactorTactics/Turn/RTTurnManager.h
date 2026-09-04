#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeView: il combat log filtra da qui, non da un secondo canale
#include "Bot/RTHexBotLibrary.h" // i pesi del bot hanno UNA sorgente: i default della struct
#include "Turn/RTTurnRules.h"
#include "Turn/RTResolvedEvent.h"
#include "Replay/RTReplayAuditLibrary.h" // FRTAuditBotDecision: il quarto record di D-313
#include "Turn/RTMoveRoute.h" // FRTMoveRoute + URTMoveRouteLibrary: la rotta e il suo filtro
#include "Turn/RTCombatLog.h" // FRTLogSubject, FRTCombatLogLine: i tipi del combat log
#include "Turn/RTTurnLog.h"
#include "Turn/RTReplayRecording.h" // FRTReplayRecording: l'archivio in scrittura vive fuori (#2286)
#include "Replay/RTReplayManifest.h"
#include "Ability/RTActionDef.h" // FRTActionDef: l'impatto della carica porta con se' la definizione
#include "Turn/RTHexSim.h" // FRTHexSnapshot: restituito per valore da MakeCurrentSnapshot
#include "Turn/RTPacingRecorder.h" // FRTPacingRecorder: la telemetria vive fuori (#1818)
#include "Turn/RTPacing.h" // FRTPacingSample: telemetria, canale separato dal TurnLog
#include "Turn/RTPlaybackLibrary.h" // FRTPhaseTime: la fase ha due termini, e il budget ne tocca uno solo
#include "Map/RTHexCellData.h" // ERTHexSurface: il terreno dinamico ricorda la superficie originale (CP 8.4)
#include "Combat/RTCombatResolver.h" // FRTAttack, FRTUnitCombatState: il pass reazioni raccoglie i primi e aggiorna i secondi
#include "Combat/RTHexCombatLibrary.h" // FRTHexAttackHit/FRTHexAttackIntent: cio' su cui il pass reazioni valuta i trigger
#include "Turn/RTReactionLibrary.h" // ERTReactionPassPoint/FRTReactionTriggerHit: la firma del pass reazioni li usa
#include "Turn/RTDeclaredCondition.h" // FRTDeclaredCondition: l'Overwatch armato porta con se' la condizione dichiarata
#include "Turn/RTReactionOpportunityTypes.h" // FRTReactionOpportunity/FRTReactionDecision: le firme del boundary li usano
#include "Turn/RTReactionWindowView.h" // FRTReactionWindowView: il DTO che questo manager consegna alla presentazione
#include "Turn/RTReactionPassResult.h" // FRTReactionPassResult/FRTDisplacementCause: cio' che il pass reazioni raccoglie
#include "Turn/RTBlastContext.h" // FRTBlastContext: lo stato che i pass del Blast si passano l'un l'altro
#include "RTTurnManager.generated.h"

class ARTUnit;
class URTHexMapAsset;

/**
 * Un colpo PREDITTIVO armato in Prep e in attesa del boundary del Move (E18 CP 18.2).
 *
 * Porta il TIRATORE come pointer e non come indice, ed e' l'unica deroga alla disciplina «l'identita' e' un
 * intero» che vale dentro una fase: fra la Prep e il Move c'e' lo scatto, e le unita' si spostano. Un indice
 * catturato nella Prep punterebbe a un'altra unita' al momento di risolvere, perche' gli array del Move sono
 * ricostruiti da zero. Al boundary il pointer torna un indice con `IndexOfByKey`, ed e' li' che ridiventa la
 * chiave stabile che il TurnLog richiede.
 *
 * La CELLA invece e' catturata in pianificazione e non si tocca piu': e' il dato che rende la previsione una
 * scommessa invece di un ordine.
 */

USTRUCT()
struct FRTArmedPrediction
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ARTUnit> Shooter = nullptr;

	/** La cella dichiarata in Planning. Non viene rivalutata al boundary. */
	UPROPERTY()
	FRTCellId LockedCell;

	/** Identita' dell'azione, per il TurnLog: fra due previsioni cambia l'abilita' spesa, non solo l'esito. */
	UPROPERTY()
	FName ActionId;

	/** L'azione generica di cui `ActionId` e' un profilo (D-033), quando ne ha una. */
	UPROPERTY()
	FName BaseActionId;

	/** Danno da applicare a chi viene colto. Viene dagli `Effects` del catalogo, non da un numero qui. */
	UPROPERTY()
	int32 Damage = 0;

	FRTArmedPrediction() = default;
};

/**
 * Un `Action.Overwatch` ARMATO nel Prep, in attesa dei micro-step del Move (CP 14.5).
 *
 * Tiene i fatti DUREVOLI dell'armamento — chi, con che portata, con che danno, con quale condizione — e non
 * la zona: `FRTOverwatchWatcher` si **deriva** da qui a ogni micro-step, quando la cella del proprietario e la
 * conoscenza di squadra sono quelle correnti.
 *
 * ⚠️ La separazione non e' un gusto architetturale, evita un difetto misurato: `ResolvePrep` costruisce il
 * proprio array di unita' ordinandolo per cella (`StableLess`), `ResolveMovement` usa quello dello snapshot.
 * I due ordini NON coincidono, quindi un indice catturato nel Prep indicherebbe un'altra unita' nel Move —
 * e `FRTOverwatchWatcher::TeamAwareness` e `FRTSuppressionMover::UnitId` sono indici. Tenere il PUNTATORE e
 * risolverlo al momento dell'uso e' esattamente cio' che `FRTArmedPrediction` fa qui sopra, e per la stessa
 * ragione (`ResolvePredictiveBoundary` chiama `Units.IndexOfByKey(Shooter)`).
 */
USTRUCT()
struct FRTArmedOverwatch
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ARTUnit> Owner = nullptr;

	/**
	 * Il facing DICHIARATO quando l'Overwatch e' stato armato, non quello corrente.
	 *
	 * Il cono e' il facing (ADR-0005 §4c) e si dichiara in Planning «insieme a settore e facing»
	 * (`RT_ActionCatalog_v0.1.md` §2). Rileggerlo a ogni micro-step lo renderebbe una direzione che cambia
	 * dopo l'impegno, cioe' il contrario di una scommessa.
	 */
	UPROPERTY()
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Identita' dell'azione, per il TurnLog. */
	UPROPERTY()
	FName ActionId;

	/** L'azione generica di cui `ActionId` e' un profilo (D-033), quando ne ha una. */
	UPROPERTY()
	FName BaseActionId;

	/**
	 * Portata e danno vengono dall'ARMA dell'eroe, catturate al momento di armare.
	 *
	 * Non sono numeri scelti qui, e non sono nel catalogo: `Action.Overwatch` dichiara solo la parte
	 * universale, perche' area, raggio ed effetto sono il **profilo** e i profili dei quattro eroi della v0.1
	 * sono ancora una domanda aperta (`brief-azioni-generiche-overwatch.md` §8). Derivarli dall'attacco base
	 * riusa dati gia' decisi per eroe invece di inventarne di nuovi, differenzia gli eroi da solo — che e' il
	 * gate anti-omogeneizzazione del §7 — e corrisponde alla semantica dell'azione: si trattiene il colpo e
	 * si spara con la propria arma quando qualcuno passa.
	 */
	UPROPERTY()
	int32 RangeCells = 1;

	UPROPERTY()
	int32 Damage = 0;

	/** La condizione dichiarata in pianificazione ([D-109]). Vuota = nessuna. */
	UPROPERTY()
	FRTDeclaredCondition Condition;

	/**
	 * La charge: `Charges = 1` in v0.1 (ADR-0004 §8). `FIRE` la consuma, `HOLD` **no**.
	 *
	 * E' il campo che rende `Overwatch.HoldKeepsArmed` una proprieta' e non una speranza: finche' e' vero, un
	 * micro-step successivo puo' ancora aprire una nuova opportunity.
	 */
	UPROPERTY()
	bool bCharged = true;

	/**
	 * Quante finestre questa reaction ha gia' APERTO in questo turno (ADR-0004 §8, cap a
	 * `MaxPromptsPerReaction`).
	 *
	 * Separato da `bCharged` perche' contano cose diverse, e la differenza si vede subito: la charge limita i
	 * `FIRE` — uno solo — mentre questo limita le DOMANDE, che un `HOLD` non consuma. Senza, un bersaglio che
	 * percorre cinque celle dentro la zona verrebbe chiesto cinque volte con una sola Overwatch: e' il numero
	 * che la misura di overhead di questo checkpoint ha registrato prima che il cap esistesse.
	 */
	UPROPERTY()
	int32 PromptsUsed = 0;

	FRTArmedOverwatch() = default;
};

// Delegate per la presentazione in Blueprint (camera/VFX/SFX): il playback riproduce eventi gia' risolti.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTPhasePlaybackSignature, ERTMatchPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTUnitPlaybackSignature, ARTUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTAttackPlaybackSignature, ARTUnit*, Source, ARTUnit*, Target, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTPlaybackFinishedSignature);

/**
 * La conoscenza di squadra e' stata rinfrescata: chi la DISEGNA puo' rileggerla ([D-227], `#1467`).
 *
 * ⚠️ Non e' un momento nuovo inventato per il velo: sono i **due** punti che gia' rinfrescano la conoscenza —
 * `RefreshTeamKnowledgeForPlanning` a inizio turno e `RefreshTeamKnowledgeForBlast` **dentro la risoluzione**.
 * Un aggiornamento a `Tick` darebbe lo stesso risultato visivo e sarebbe sbagliato: il velo seguirebbe il
 * tempo reale invece dello stato del turno, ed e' cio' che `Veil.FollowsRefreshPoints` esiste per impedire.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTTeamKnowledgeRefreshedSignature, int32, TurnNumber);

/**
 * La partita è finita, con il verdetto e lo stato che lo motiva (CP 46.5, `#940`).
 *
 * ⚠️ **È un annuncio, non un comando**, ed è la ragione per cui esiste invece di far chiamare al
 * `TurnManager` la schermata di fine partita: la simulazione non deve conoscere il frontend. Chi ascolta
 * decide cosa farne — `ARTGameMode` apre il Result, uno scenario headless non ascolta e non cambia nulla.
 * È la stessa forma di `URTFrontendNavigator::OnMatchRequested`: chi sa annuncia, chi può agire consuma.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTOnMatchEndedSignature, const FRTMatchResult&, Result, const FRTMatchState&, State);

// `FRTLogSubject` e `FRTCombatLogLine` vivono in `Turn/RTCombatLog.h` da `#1818`: chiunque volesse una
// riga di log doveva includere QUESTO header per una struct di tre campi.

// `FRTMoveRoute` vive in `Turn/RTMoveRoute.h` da `#1818`, col filtro `URTMoveRouteLibrary`.

/**
 * Orchestratore del turno: tiene fase e numero di turno e, al lock-in, risolve il turno (logica sincrona,
 * autoritativa) e poi ne RIPRODUCE nel tempo la risoluzione (playback) per rendere il round osservabile.
 * L'animazione legge eventi gia' risolti: non decide nulla (invariante #1).
 */
UCLASS()
class REFACTORTACTICS_API ARTTurnManager : public AActor
{
	GENERATED_BODY()

public:
	ARTTurnManager();

	virtual void Tick(float DeltaSeconds) override;

	/** Chiude la pianificazione e risolve il turno; il movimento si applica nella fase Move. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void LockInAndResolve();

	/** Hook per i test d'integrazione headless: invoca la pianificazione dei bot senza timer/playback. */
	void PlanBotsForTest() { PlanBots(); }

	/**
	 * Hook per i test: quanti verdetti porta l'evento di movimento di questa unita' nella timeline risolta.
	 *
	 * 🔴 **Esiste perche' il predicato puro non basta a dimostrare il fix di `#1525`.**
	 * `ObservedPrefixLength` e' coperto da cinque test, ma resterebbe verde anche se nessuno gli passasse
	 * i verdetti: il difetto non era la regola — era che `FRTResolvedEvent` portava la rotta **senza** il
	 * verdetto che la traccia aveva gia' congelato due righe sopra. Questo accessore rende osservabile
	 * proprio quel collegamento, che e' la meta' del difetto che un test puro non puo' vedere.
	 *
	 * @return il numero di verdetti, o `INDEX_NONE` se l'unita' non ha un evento di movimento.
	 */
	int32 ResolvedMoveVerdictCountForTest(int32 StableUnitId) const;

	/**
	 * Hook per i test: quante reazioni RISOLTE ha emesso l'unita' data in questo turno (#2191).
	 *
	 * 🔴 Esiste perche' `ResolvedTimeline` e' privato e la presentazione e' l'unico consumatore: senza
	 * questo accessore l'emissione sarebbe verificabile solo a schermo, cioe' proprio la meta' che le voci
	 * `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE` esistono per giudicare — e un test che non puo' vedere il
	 * fatto non lo sorveglia.
	 *
	 * ⚠️ Conta gli eventi, non gli effetti: una reazione che apre uno scudo e una che contrattacca
	 * valgono **uno** ciascuna. Quanti danni siano passati lo dicono gli `Attack`, che restano loro.
	 *
	 * @return il numero di eventi `ReactionResolved` con quel soggetto; `0` se non ha reagito.
	 */
	int32 ResolvedReactionCountForTest(int32 ReactorStableUnitId) const;

	/**
	 * Hook per i test: gli eventi `StatusChanged` emessi in questo turno (`#2245`).
	 *
	 * 🔴 Stessa ragione dell'accessore qui sopra — `ResolvedTimeline` e' privato e la presentazione e'
	 * l'unico consumatore — con una in piu': l'emissione avviene in `AppendLogEntry`, cioe' in un punto
	 * che **ogni** voce di log attraversa. Senza poter leggere la timeline, la differenza fra «emesso per
	 * gli stati» ed «emesso per tutto» non sarebbe osservabile da un test.
	 *
	 * ⚠️ Restituisce gli eventi INTERI e non un conteggio: cio' che va sorvegliato qui e' il **contenuto** —
	 * quale tag, quale causa, quanti turni — e un numero non lo direbbe. Il precedente conta perche' li'
	 * la domanda era *«ha reagito?»*; qui e' *«che cosa e' successo a quale stato, e perche'»*.
	 *
	 * @return copia degli eventi `StatusChanged` nell'ordine di emissione, che e' quello delle voci di log.
	 */
	TArray<FRTResolvedEvent> ResolvedStatusEventsForTest() const;

	/**
	 * Hook per i test: applica una modifica temporanea di superficie dichiarandone l'autore.
	 *
	 * Serve perche' la scadenza ambientale — la voce che deve restare senza attore (#405) — nessuno scenario
	 * del corpus la produce: le azioni ambientali richiedono un eroe owner e le durate superano la lunghezza
	 * degli scenari. Senza questo hook il criterio resterebbe scritto e mai eseguito.
	 */
	bool ApplyDynamicSurfaceForTest(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexSurface NewSurface,
		int32 Turns, const FName& CauseActionId, const ARTUnit* Cause)
	{
		return ApplyDynamicSurface(Map, Cell, NewSurface, Turns, CauseActionId, Cause);
	}

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	ERTMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	int32 GetTurnNumber() const { return TurnNumber; }

	/**
	 * Regole di formato IN VIGORE: `RoundLimit`, soglia obiettivo, identita' del formato (CP 10.3).
	 * Non sono una costante di questo orchestratore: le risolve `ARTGameMode` dall'asset del formato.
	 */
	const FRTMatchRules& GetMatchRules() const { return MatchRules; }

	/**
	 * Assegna le regole di formato. La chiama `ARTGameMode` dopo averle risolte dall'asset — o dopo aver
	 * scelto di ripiegare, che e' una decisione sua e non di questa classe (issue #185).
	 * Non assegnarle lascia `RoundLimit` a 0, cioe' una partita che finisce solo per eliminazione: e' il
	 * comportamento dei test che allestiscono un TurnManager senza GameMode.
	 */
	void SetMatchRules(const FRTMatchRules& InRules) { MatchRules = InRules; }

	/** Esito della partita e via che l'ha determinato; `InProgress`/`None` finche' e' in corso. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	FRTMatchResult GetMatchResult() const { return PendingResult; }

	/**
	 * Cambia la durata della pianificazione e RIAVVIA il timer se e' in corso, cosi' il nuovo valore vale
	 * subito invece che dal turno dopo. Valori negativi vengono portati a 0 (= nessuna scadenza).
	 *
	 * Serve all'allestimento (`ARTGameMode`) per accorciare la pianificazione quando gira uno scenario di
	 * test: la partita normale continua a usare i suoi 30 secondi. Non e' una regola di gioco — e' ritmo di
	 * presentazione, e resta fuori dal resolver.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void SetPlanningSeconds(float NewSeconds);

	/** Durata corrente della pianificazione (diagnostica e test). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlanningSeconds() const { return PlanningSeconds; }

	/**
	 * Durata della finestra Fast Reaction (ADR-0004 §8). E' cio' che alimenta il countdown del DTO di
	 * CP 14.6: la presentazione la LEGGE da qui, non ne tiene una copia.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetFastReactionDuration() const { return FastReactionDuration; }

	/**
	 * Cambia la durata della finestra, clampando alla FONTE come fa `SetPlanningSeconds`.
	 *
	 * `ClampMin` sulla `UPROPERTY` copre l'authoring in Editor e nient'altro: un valore negativo assegnato da
	 * codice o da un Blueprint lo attraverserebbe, e un countdown negativo e' un timer che non scatta mai. Il
	 * clamp del DTO protegge una copia sola — questo protegge il campo.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void SetFastReactionDuration(float NewSeconds) { FastReactionDuration = FMath::Max(0.f, NewSeconds); }

	/**
	 * La finestra `Opportunity` come la riceve `ObserverTeamId`, con il countdown autorevole di questo
	 * manager (CP 14.6, `#166`).
	 *
	 * E' il **punto d'ingresso unico** fra il core e la presentazione della finestra, e la ragione per cui
	 * esiste invece di lasciare che ciascun chiamante componga la coppia (filtro, durata): sono due decisioni
	 * che devono restare insieme. La regola di sanitizzazione vive in
	 * `URTReactionWindowLibrary::FilterWindowForTeam`, che resta pura e testabile senza mondo; qui si aggiunge
	 * la sola cosa che una funzione pura non puo' avere — il valore autorevole della durata.
	 *
	 * ⚠️ **Non e' una `UFUNCTION`**: prende `FRTReactionOpportunity`, e per esporla ai Blueprint bisognerebbe
	 * rendere `BlueprintType` l'opportunity autorevole. Il risultato invece lo e' per intero.
	 *
	 * ⛔ **Nessun chiamante di produzione, oggi**, ed e' una conseguenza e non una svista: `AskReactionDecision`
	 * e' sincrono e non esiste una finestra persistente da interrogare. Il chiamante nasce con il decisore
	 * umano asincrono di DIR-A. Cio' che questo metodo garantisce da subito e' che quel chiamante non debba
	 * scegliere da dove prende i 3,0 s — la seconda sorgente che ADR-0005 §4c vieta.
	 */
	FRTReactionWindowView MakeReactionWindowView(const FRTReactionOpportunity& Opportunity,
		int32 OwnerTeamId, int32 ObserverTeamId) const;

	/** Progresso obiettivo di una squadra (intero, mai un float). Squadra sconosciuta -> 0. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	int32 GetTeamScore(int32 TeamId) const;

	/**
	 * Aggiunge progresso obiettivo a una squadra. E' l'ingresso che il sistema Objective (CP 10.2) usera'
	 * nel Cleanup: qui vive il GIUDICE della fine partita, non la fonte del punteggio.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void AddTeamScore(int32 TeamId, int32 Points);

	/** Secondi rimanenti alla pianificazione (0 se scaduto/assente). Utile per una futura HUD. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlanningTimeRemaining() const;

	/** Vero mentre e' in corso il playback della risoluzione (pianificazione bloccata). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	bool IsResolving() const { return bIsResolving; }

	/** Nome leggibile della fase in riproduzione (per la HUD). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	FString GetPlaybackPhaseName() const;

	/** Avanzamento del playback in [0,1] (0 se non in risoluzione). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlaybackProgress01() const;

	/** Salta il resto del playback e conclude subito il turno (snap alle posizioni finali). */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void SkipPlayback();

	/**
	 * Ultimi eventi (combat log), dal piu' vecchio al piu' recente. NON filtrati per squadra: e' la vista
	 * completa, la stessa forma che aveva prima che RecentEvents portasse un soggetto. Chi disegna per un
	 * giocatore deve chiamare `GetRecentEventsForTeam`, non questa.
	 */
	TArray<FString> GetRecentEvents() const;



	/** Le righe recenti gia' filtrate per una squadra. E' cio' che l'HUD deve chiamare. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FString> GetRecentEventsForTeam(int32 ObserverTeamId) const;

	/** Esiti autoritativi dell'ultimo turno risolto (Movimento + Combat), ordinati deterministicamente. */
	const TArray<FRTTurnLogEntry>& GetTurnLog() const { return TurnLog; }

	/**
	 * Registrazione del replay (`#469`). Per il replay il TurnManager **non scrive**: passa il TurnLog a
	 * `URTReplayRecorderLibrary` e non tocca il disco.
	 *
	 * ⚠️ La frase vale per il replay e non per la classe: `AppendPacingRow` scrive gia' i CSV di pacing in
	 * `Saved/RT/`, e lo fa da `ClosePacingSample`, **una riga sopra** la registrazione dentro `ConcludeTurn`.
	 * Dirlo in assoluto sarebbe falso a una riga di distanza.
	 *
	 * Non contraddice [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3: quel confine
	 * dice che chi **riproduce** non chiama il resolver. Qui e' il contrario — e' il resolver che consegna a
	 * chi scrive, e scrivere non e' riprodurre.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Replay")
	bool bRecordReplay = true;

	/**
	 * Radice degli archivi. Vuota = `Saved/Replays`. E' **configurazione**, non un ramo «se test»: un test
	 * che deve scrivere altrove imposta un parametro, non attiva un percorso di codice diverso da quello
	 * che gira in partita.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Replay")
	FString ReplaysRootOverride;

	/**
	 * Avvia la registrazione: genera il `MatchId` e fissa il formato. **Va chiamata da chi allestisce una
	 * partita vera** — il GameMode — e non da `BeginPlay`.
	 *
	 * Due ragioni, entrambe misurate: `BeginPlay` gira anche per i 27 file di test e per lo
	 * `ScenarioHarness` che spawnano un TurnManager, e registrare li' significherebbe far scrivere su disco
	 * centinaia di test che non l'hanno chiesto; e il GameMode spawna il TurnManager **prima** di risolvere
	 * il formato (`ApplyMatchFormat`), quindi a `BeginPlay` `MatchRules.FormatId` non e' ancora quello vero.
	 *
	 * Finche' non viene chiamata, `RecordTurnToReplay` e `CloseReplayArchive` non fanno nulla: la
	 * registrazione e' spenta per assenza di identita', non per un flag da ricordarsi.
	 */
	void BeginReplayRecording();

	/**
	 * L'ALLESTIMENTO RIVENDICA L'APERTURA DEL TURNO 1 — `#2102`, [D-314].
	 *
	 * Dopo questa chiamata `BeginPlay` **non** apre piu' il turno da solo: lo aprira'
	 * `OpenFirstTurnAfterSetup()`, quando la board esiste davvero.
	 *
	 * 🔑 **Stesso schema di `BeginReplayRecording`, e per la stessa ragione**: `BeginPlay` gira anche per i
	 * test headless e per lo `ScenarioHarness`, che spawnano un TurnManager a mano e non hanno
	 * bootstrapper. Un'attesa **incondizionata** li fermerebbe tutti. Chi non rivendica apre come sempre.
	 *
	 * ⛔ **Il criterio NON e' «siamo in PIE».** Il progetto ha gia' deciso questa domanda in
	 * `ARTPlayerController::IsPlanningInputInert`: *«un predicato che rispondesse solo in PIE non sarebbe
	 * verificabile headless, e questo DEVE esserlo»*. Qui il criterio e' una rivendicazione esplicita, che
	 * un test puo' fare e non fare.
	 *
	 * ⚠️ **Non annulla un turno gia' aperto**, e lo dichiara nel log invece di tacere: chiuderlo e
	 * riaprirlo significherebbe richiudere il campione di pacing, cioe' toccare `MsToLockIn`. Idempotente.
	 */
	void ClaimFirstTurnForMatchSetup();

	/**
	 * APRE il turno 1 rivendicato: e' il punto in cui l'allestimento dichiara di aver finito.
	 *
	 * Non fa nulla se nessuno ha rivendicato (il turno l'ha gia' aperto `BeginPlay`) o se e' gia' stato
	 * aperto da questa funzione. **Idempotente**: due chiamate producono una sola voce `Turno 1`, e
	 * l'idempotenza non e' cortesia — il GameMode la chiama da piu' cammini d'uscita, perche' un turno mai
	 * aperto e' peggio di un turno aperto presto.
	 */
	void OpenFirstTurnAfterSetup();

	/** Vero se qualcuno ha rivendicato l'apertura del turno 1. Per i test dell'ordine (`#2102`). */
	bool IsFirstTurnClaimedBySetup() const { return bFirstTurnClaimedBySetup; }

	/** Vero se il turno 1 e' stato aperto — da `BeginPlay` o dall'allestimento. Per i test dell'ordine. */
	bool HasOpenedFirstTurn() const { return bFirstTurnOpened; }

	/**
	 * Ricalcola SUBITO la conoscenza di squadra, con le unita' che esistono adesso.
	 *
	 * 🔴 **Esiste per una finestra misurata, non per simmetria** ([#1762]). L'unico produttore di
	 * conoscenza e' `RefreshTeamKnowledgeForPlanning`, che gira dentro `PlanBots`, che gira dentro
	 * `StartPlanningTimer`, che gira nel **`BeginPlay` di questo attore** — e a quel punto le unita' NON
	 * esistono ancora: le spawna `ARTGameMode::SetupHexMatch`, piu' tardi. Il primo refresh esce quindi con
	 * `Live` vuoto e produce una conoscenza **vuota per ogni squadra**, che nessun altro rinfresca fino al
	 * turno successivo.
	 *
	 * ⚠️ **Era gia' dichiarato e ritenuto innocuo**: il commento sopra lo spawn del TurnManager avverte che
	 * li' non c'e' nessuna `ARTUnit` e che *«le due funzioni sono scritte per sopportarlo»*. «Sopportarlo»
	 * significava non andare in crash — non produrre una conoscenza sensata. Quando il velo e' arrivato
	 * ([D-242]) ha letto quella conoscenza vuota, ha marcato ogni cella come mai vista e, poiche' una cella
	 * mai vista non si disegna ([D-225]), le ha tolto anche la **collisione**: al primo turno il click, che e'
	 * un raycast, non colpiva piu' niente.
	 *
	 * ✅ Chiamarla dopo lo spawn del roster e' l'unica uscita che non muove l'architettura: non chiede alla
	 * presentazione di ricalcolare lo stato (lo fa chi allestisce la partita, che e' il suo mestiere) e non
	 * rinuncia alla stesura del velo alla prima inquadratura, che [D-242] chiede per non rivelare la mappa.
	 *
	 * ⚠️ **Non e' economica** — passa da `MakeCurrentSnapshot`, che fa `GetAllActorsOfClass` e due `Sort` —
	 * ed e' accettabile perche' si paga **una volta per partita**, all'allestimento. Non va messa in un `Tick`
	 * ne' chiamata a ogni fase: i due punti di refresh restano quelli, e `Veil.FollowsRefreshPoints` lo pinna.
	 */
	void RefreshTeamKnowledgeNow();

	/** Identita' della registrazione in corso. Non valida finche' `BeginReplayRecording` non e' stata chiamata. */
	FGuid GetReplayMatchId() const { return ReplayRecording.GetMatchId(); }

	/** Ultimo checksum di stato catturato. `0` = mai calcolato (registrazione spenta, o nessun turno risolto). */
	int64 GetPendingFinalStateHash() const { return PendingFinalStateHash; }

	/**
	 * Rotte effettivamente percorse nell'ultima risoluzione (viz post-lock del percorso eseguito).
	 *
	 * ⚠️ **Non e' filtrata per conoscenza**: porta le rotte di ENTRAMBE le squadre, e ogni consumatore che
	 * la disegna o la stampa deve filtrarla per conto proprio. Il campo `StableUnitId` esiste perche' quel
	 * filtro sia possibile (`#1497`); la regola con cui filtrare e' la domanda aperta di `#1496`.
	 */
	const TArray<FRTMoveRoute>& GetLastMoveRoutes() const { return LastMoveRoutes; }

	/**
	 * La conoscenza di UNA squadra, per la presentazione. Copia piccola: NON e' `MakeCurrentSnapshot`, che
	 * fa `GetAllActorsOfClass` e due `Sort` ed e' proibitiva a ogni frame.
	 *
	 * 🔴 **NON e' una `UFUNCTION`, ed e' deliberato.** Una prima stesura la esponeva come `BlueprintPure`, e
	 * sarebbe stata il PRIMO canale Blueprint verso la conoscenza NON filtrata di una squadra qualunque:
	 * `MakeCurrentSnapshot` non e' esposta, quindi finora nessun Blueprint poteva ottenere un
	 * `FRTTeamKnowledge`, e le `UFUNCTION` di `URTTeamKnowledgeLibrary` sono gia' li' per interrogarlo.
	 * Un widget avrebbe potuto chiamare `KnowledgeForTeamPublic(1)` e leggere `VisibleCells` e `Contacts`
	 * dell'avversario — aprire un canale non filtrato nello stesso commit che ne chiude uno.
	 * Chi ha bisogno della conoscenza in Blueprint passa da `FRTKnowledgeView`, che e' la porta.
	 */
	FRTTeamKnowledge KnowledgeForTeamPublic(int32 TeamId) const { return KnowledgeForTeam(TeamId); }

	// --- Sonda di pacing (TELEMETRIA: nessun ritorno verso il gameplay) --------------------------
	/**
	 * Registra un input di pianificazione. Chiamata dal PlayerController, che NON cronometra: tutto il
	 * tempo vive qui, in un posto solo. Ignorata fuori dalla fase di pianificazione.
	 */
	void RecordPlanningInput(ERTPlanningInput Kind);

	/**
	 * Emesso dai due punti di refresh, con il turno a cui la fotografia si riferisce. Il consumatore naturale
	 * e' la presentazione — `ARTHexMapActor::ApplyKnowledgeVeil` — che sceglie DI CHI e' la conoscenza da
	 * disegnare: questo delegate non lo decide, e non deve.
	 */
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Perception")
	FRTTeamKnowledgeRefreshedSignature OnTeamKnowledgeRefreshed;

	/** Campioni di pacing della sessione corrente (sola lettura; telemetria, non stato di gioco). */
	const TArray<FRTPacingSample>& GetPacingSamples() const { return Pacing.GetSamples(); }

	/** Se vero, ogni turno appende una riga in Saved/RT/pacing_<sessione>.csv. L'accumulo in memoria e' sempre attivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	bool bRecordPacing = false;

	/**
	 * **La sessione non e' presidiata**: nessuna mano umana pianifica, quindi non c'e' un ritmo umano da
	 * cronometrare (#971).
	 *
	 * ⛔ **Si viene INFORMATI, non si chiede.** `ARTGameMode::SetupHexMatch` latcha la modalita' e la spinge
	 * qui, nello stesso blocco in cui spinge `SetPlanningSeconds` e per la stessa ragione: e' configurazione
	 * del turno, e quel blocco sta **prima** del ritorno anticipato, quindi vale anche su un livello che
	 * porta gia' le proprie unita'. Interrogare il GameMode da qui romperebbe la riga che questo file
	 * dichiara di sua mano — *«qui la simulazione non conosce il frontend, e non deve»* — e aggiungerebbe
	 * una seconda autorita' sulla stessa domanda.
	 *
	 * ⚠️ **Non tocca il resolver**: e' telemetria. L'unico effetto e' che i tre TEMPI del campione di pacing
	 * si dichiarano `Unmeasured` invece di riportare una pianificazione che nessuno ha fatto.
	 */
	void SetUnattendedSession(bool bUnattended) { bUnattendedSession = bUnattended; }

	/** Vedi `SetUnattendedSession`. */
	bool IsUnattendedSession() const { return bUnattendedSession; }

	/** Squadra il cui spazio di decisione si misura in ActionsAvailable (il giocatore umano e' il team 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	int32 PacingTeamId = 0;

	// --- Presentazione (Blueprint) -------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTPhasePlaybackSignature OnPhasePlaybackStarted;

	/**
	 * Annuncio di fine partita. Emesso una volta sola, quando la fase diventa `MatchEnded`.
	 *
	 * ⚠️ **Dopo che l'archivio del replay è chiuso**, non prima: chi ascolta può voler leggere la traccia,
	 * e un manifest ancora aperto la descriverebbe come una partita in corso.
	 */
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Match")
	FRTOnMatchEndedSignature OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTUnitPlaybackSignature OnUnitMoveStarted;

	/** Un'unita' viene eliminata VISIVAMENTE nel playback (per VFX/SFX di morte in Blueprint). */
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTUnitPlaybackSignature OnUnitDefeated;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTAttackPlaybackSignature OnAttackResolved;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTPlaybackFinishedSignature OnResolvePlaybackFinished;

	// --- Tuning del pacing (editabile in editor, tuning live senza ricompilare) -----------------
	/** Se falso, la risoluzione resta istantanea (nessun playback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	bool bEnablePlayback = true;

	/**
	 * Velocita' di scorrimento dei modelli nelle fasi che muovono (celle al secondo).
	 *
	 * 🔑 **`1.44` NON e' un gusto: e' la velocita' a cui il piede non scivola** (`#1878`, 2026-09-03), e la
	 * si ricava da due valori che il repository gia' dichiarava:
	 *
	 *     passo di una cella = HexSize * sqrt(3) = 150 * 1,732 = 259,8 cm      (`URTHexLibrary::AxialToWorld`)
	 *     la clip di corsa dichiara                        = 375 cm/s          (`ARTUnit::VisualRunSpeed`)
	 *     ∴ velocita' senza scivolamento = 375 / 259,8     = 1,443 celle/s
	 *
	 * A `1.44` il residuo e' **-0,2%**, sotto qualunque soglia percettiva.
	 *
	 * 📐 **Il pattinamento agli altri valori, per capire cosa si sta comprando**: `6.5` (il default fino al
	 * 2026-09-02) traslava a 1688 cm/s contro i 375 dichiarati, cioe' **+350%** — i personaggi correvano
	 * quattro volte e mezzo piu' della loro animazione, ed e' la causa vera della segnalazione *«sembrano
	 * andare in fast-forward»*. `2.0` sarebbe **+39%**, `1.65` **+14%**.
	 *
	 * ⚠️ **Il product owner aveva scelto `2.0` a schermo, e ha cambiato idea davanti a questo calcolo**: la
	 * scelta percettiva e quella geometrica non coincidevano, e ha prevalso la seconda perche' `1.44` sta
	 * fra i due valori giudicati «lenti» (`1.35` e `1.65`) — cioe' dentro un intervallo gia' esplorato.
	 *
	 * 🔴 **Vale finche' `HexSize` vale 150.** Il numero senza scivolamento e' una funzione del passo, non
	 * una costante: una mappa autorata con `HexSize` diverso rimette i piedi a pattinare, e nessun errore lo
	 * segnala. Lo pinna `RefactorTactics.Playback.DefaultRateMatchesTheRunClip`, che ricalcola la relazione
	 * invece di ripetere il numero — se qualcuno cambia questo default, `VisualRunSpeed` o `HexSize`, cade.
	 *
	 * ⚠️ **Il numero scritto e' il numero che si osserva, a `ViewerPlaybackSpeed` = 1.** La manopola del
	 * viewer (`x1/x2/x4`) moltiplica l'orologio del playback: a `x4` si vedono `5,8` celle/s. Cio' che non
	 * accade piu' e' che il tetto di durata acceleri da se' — lo pinna
	 * `RefactorTactics.Playback.BudgetDoesNotSpeedUpLocomotion`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PlaybackCellsPerSecond = 1.44f;

	/** Pausa tra una fase e la successiva (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PhaseBeatSeconds = 0.30f;

	/** Durata di visualizzazione di ogni colpo nel Blast (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float AttackShowSeconds = 0.50f;

	/**
	 * Budget SOFT di durata del playback: oltre, si comprimono le ATTESE (0 = nessun budget).
	 *
	 * ⚠️ **Non accelera piu' la locomozione, e la parola «soft» e' quella differenza** (`#1878`,
	 * 2026-09-02). Prima moltiplicava `Dt` — l'unico orologio, che governa anche l'interpolazione del
	 * movimento — quindi il tetto faceva correre i cilindri per far stare il turno nel numero. Il product
	 * owner ha escluso quel comportamento: *«la durata target della Resolution non deve determinare la
	 * velocita' visuale base della locomozione»*. Ora entra in
	 * `URTPlaybackLibrary::SlackScaleForBudget`, che comprime `FRTPhaseTime::Slack` e non tocca
	 * `Locomotion`. Quando il comprimibile finisce, **la durata sfora**: e' la definizione di soft.
	 *
	 * 📐 Misurato il 2026-09-02 su 125.780 risoluzioni nei log: il tetto **non era mai intervenuto** —
	 * durata raw massima 4,4 s contro 12. Il difetto era latente, e si sarebbe risvegliato appena abbassata
	 * la velocita' base, annullando proprio la correzione che #1878 chiede.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float MaxPlaybackSeconds = 12.f;

	/**
	 * Velocita' SCELTA da chi guarda: 1 / 2 / 4 (CP 47.2, #955). E' una preferenza di ritmo, ed e' l'UNICA
	 * cosa che accelera la riproduzione: dal 2026-09-02 non si compone piu' con un fattore del tetto,
	 * perche' il tetto non ne produce piu' uno (`#1878`). Scriverla a risoluzione in corso vale dal tick
	 * successivo — `TickPlayback` la rilegge a ogni tick e non la congela in `BeginPlayback`.
	 *
	 * ⚠️ Presentazione, mai decisione (invariante #1): non entra nel TurnLog, non ne tocca l'hash, non
	 * cambia l'ordine di risoluzione. Il gate che lo verifica e' in
	 * `RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback`.
	 * ⚠️ Da non confondere con `rt.Match.PlanningSeconds`: quello e' quanto dura la DECISIONE, questo
	 * quanto dura il MOSTRARLA. Confonderli renderebbe la misura di pacing non confrontabile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float ViewerPlaybackSpeed = 1.f;

	/**
	 * Quanto il budget sta comprimendo le attese di questo round: `1` = nessuna compressione, `0` = tolto
	 * tutto il comprimibile e la durata sfora comunque.
	 *
	 * ⚠️ **Non e' un fattore di velocita' e non va mostrato come tale.** Ha sostituito
	 * `GetPlaybackCapSpeed()` il 2026-09-02 (`#1878`): quello esponeva un moltiplicatore `>= 1` che
	 * accelerava la riproduzione, e l'etichetta della manopola lo componeva con la scelta del viewer per
	 * dire la verita' su uno schermo che scorreva piu' in fretta. Ora lo schermo non scorre piu' in fretta
	 * da se': l'etichetta mostra la sola scelta, ed e' vera perche' non c'e' un secondo fattore.
	 *
	 * Resta esposto come **telemetria di pacing** — dice se il budget ha morso, cosa che il criterio 2 di
	 * CP 47.7 (`#1015`) chiedeva di non nascondere. ⚠️ Presentazione: fuori da `StateHash` e dal TurnLog.
	 */
	float GetPlaybackSlackScale() const { return PlaybackSlackScale; }

	// --- Tuning del bot (utility scoring, editabile in editor senza ricompilare) -----------------
	// Pesi interi iniettati nel FRTBotContext di PlanBots (invariante #4: niente float). I default
	// coincidono con quelli della struct: a parita' di valori il comportamento e' invariato.
	/** Bonus se l'attacco pianificato UCCIDE il bersaglio: domina la scelta (focus-fire letale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKill = FRTHexBotContext{}.WKill;

	/** Peso del danno inflitto dall'attacco (focus-fire: a parita' d'altro, piu' danno = meglio). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WDamage = FRTHexBotContext{}.WDamage;

	/** Penalita' per ogni nemico che puo' colpire la cella scelta (evita di esporsi al tiro). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WThreat = FRTHexBotContext{}.WThreat;

	/** Penalita' (kiter) proporzionale a quanto si sta SOTTO la distanza di sicurezza (standoff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKiteViolation = FRTHexBotContext{}.WKiteViolation;

	/**
	 * Penalita' proporzionale alla distanza: dal nemico per la MISCHIA, dalla propria portata per il
	 * KITER (dentro la banda utile e' indifferente). ⚠️ Il tooltip diceva «(mischia)» e non era piu'
	 * vero dal 2026-08-23: questo peso governa l'avvicinamento di ogni unita', kiter compresi.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WApproach = FRTHexBotContext{}.WApproach;

	/**
	 * Bonus per la quota (`Layer`) della cella di destinazione: premia l'alta quota — tiro oltre coperture
	 * basse, piu' danno (`URTHexBotLibrary::ScorePlan`).
	 *
	 * 🔴 **E' il termine in cui si e' formato lo stato assorbente di #1088**, perche' compete con
	 * l'avvicinamento: restare in alto continua a incassarlo, quindi sopra una certa soglia batte muoversi.
	 * ⛔ Renderlo relativo alla cella di partenza NON aiuta e non va riprovato — `Origin` e' fisso per
	 * l'intera scelta, quindi sposta ogni candidata della stessa costante: scritto, misurato e tolto il
	 * 2026-08-22.
	 *
	 * **Vale 4 e non 5 perche' l'invariante si misura sul caso peggiore**: le arene generate usano due layer
	 * (`MaxLayer` 1) e li' anche 5 reggeva, ma con tre layer `5 x 2 = 10` pareggia `WApproach` e il tie-break
	 * «mossa minima» riapre il parcheggio. Misurato: `-40` contro `-40`, esattamente pari.
	 *
	 * ⚠️ **INVARIANTE: `WElevation * MaxLayer < WApproach`** — l'unica difesa reale, perche' nessuna forma
	 * rende il difetto impossibile. Alzarlo da qui in editor lo riapre. Questa e' la sorgente che vince in
	 * partita:
	 * `PlanBots` copia questi valori nel `FRTHexBotContext`, quindi cambiare il default della struct
	 * senza cambiare questo non muove nulla di cio' che il giocatore vede.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WElevation = FRTHexBotContext{}.WElevation;

	/**
	 * Bonus per una cella da cui si VEDE un contatto noto, sui piani senza attacco (#1300, D-185): il
	 * termine che risponde a «da qui posso ingaggiare».
	 *
	 * ⚠️ **Non si alza senza alzare anche il decadimento.** Da solo e' un bonus posizionale, e sopra
	 * `WApproach - WElevation * MaxLayer` riapre lo stato assorbente di #1088 su una cella che vede — con i
	 * default, gia' da `7`. E' `WEngageDecay` a renderlo sostenibile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WEngage = FRTHexBotContext{}.WEngage;

	/**
	 * Quanto `WEngage` cala per ogni turno consecutivo senza ingaggiare. **Zero disattiva la memoria e
	 * riporta il termine alla forma che non passa gli oracoli**: la coppia si tara insieme, e l'esito lo
	 * pinna `HexBot.EngageBonusFadesWithIdleTurns`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WEngageDecay = FRTHexBotContext{}.WEngageDecay;

	/**
	 * Quanto vale CONTROLLARE la cella obiettivo, cioe' terminare il piano sopra di essa (`#2269`).
	 *
	 * E' la categoria `Objective` di `spec-bot-tattico.md` §5, e prima di questa riga il punteggio del bot
	 * non la nominava affatto: su `L_HexArena` — che un obiettivo ce l'ha, a `(0,-3,L0)` — una partita 2v2
	 * si e' decisa `obiettivo 0-3` senza che nessuno dei due bot lo stesse giocando.
	 *
	 * ⚠️ **Zero spegne il termine**, e sul roster spedito non c'e' nessun'altra manopola che lo faccia: e'
	 * l'interruttore con cui la verifica di mutazione misura che il termine stia davvero decidendo qualcosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WObjective = FRTHexBotContext{}.WObjective;

	/**
	 * Quanto `WObjective` cala per ogni passo che manca all'obiettivo piu' vicino.
	 *
	 * 🔴 **INVARIANTE: `WObjectiveFalloff > WApproach`.** Sotto quella soglia il gradiente verso l'obiettivo
	 * pareggia quello verso il nemico, il tie-break «a parita' vince la mossa minima» fa restare, e il bot
	 * non raggiunge l'obiettivo nemmeno quando gli e' accanto. Pinnata da
	 * `HexBot.ObjectivePullBeatsClosingOneCell` sull'ESITO di `ChooseBestPlan`.
	 * ⛔ Abbassarlo da qui in editor riapre quel difetto, esattamente come alzare `WElevation` riapre `#1088`:
	 * questa e' la sorgente che vince in partita, perche' `PlanBots` copia queste UPROPERTY nel contesto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WObjectiveFalloff = FRTHexBotContext{}.WObjectiveFalloff;

	/**
	 * Snapshot dello stato corrente della partita (unita' VIVE del livello + mappa autorevole).
	 * `OutUnits[i]` e' l'unita' con `UnitId == i`: e' la chiave con cui rileggere gli esiti.
	 *
	 * Il TurnManager e' l'autorita' (invariante #5): il controller del giocatore chiede QUESTO snapshot per
	 * calcolare le sue anteprime, invece di ricostruirsi uno stato parallelo che potrebbe divergere.
	 */
	FRTHexSnapshot MakeCurrentSnapshot(TArray<ARTUnit*>& OutUnits) const;

	/**
	 * Le unita' VIVE del livello, in ordine stabile per cella.
	 *
	 * E' la prima meta' di `MakeCurrentSnapshot`, estratta perche' chi ha bisogno delle unita' ma NON dello
	 * snapshot non paghi la seconda: `ValidatePlansAtLockIn` iterava un `FRTHexSnapshot` completo — un
	 * `GetAllActorsOfClass` sull'intero livello, un `FRTHexSimUnit` per unita', la vista di mappa e
	 * occupazione, una copia di `TeamKnowledgeState` — per passarne un elemento a `URTPlanValidationLibrary`,
	 * che dopo [D-190] non lo legge affatto.
	 *
	 * 🔴 **Il `Sort` non e' una rifinitura**: senza, l'ordine di spawn decide la partita (#990), e cade
	 * `Match.Autobattle.DeterminismSurvivesUnitPermutation` — verificato per mutazione.
	 *
	 * ⚠️ Questo NON e' l'unico `StableLess` su unita' del progetto: `ResolveEnvironment` e `ResolvePrep`
	 * ordinano array propri con lo stesso comparatore, e `ResolveCombat` pure. Questo helper e' la sorgente
	 * unica per **chi vuole le unita' vive del livello**, non un consolidamento di tutti gli ordinamenti:
	 * cambiare il comparatore qui non lo cambia la'.
	 */
	/**
	 * Lo stato di simulazione di UNA unita', con tutti i campi che lo snapshot le darebbe.
	 *
	 * Esiste perche' chi ha bisogno dello stato di un'unita' — `ValidatePlansAtLockIn` — non debba
	 * costruirselo a mano: e' cosi' che e' nato un difetto trovato in code review, con `MoveCostModifier` e
	 * `Facing` dimenticati. `MakeCurrentSnapshot` chiama questo stesso helper nel proprio loop, quindi i due
	 * non possono divergere.
	 *
	 * ⚠️ `Index` e' l'identita' NELLO SNAPSHOT, non `StableUnitId`: due numerazioni diverse (si veda il
	 * commento sui contatti in `MakeCurrentSnapshot`).
	 */
	FRTHexSimUnit MakeSimUnit(int32 Index, const ARTUnit* Unit) const;

	void CollectLivingUnits(TArray<ARTUnit*>& OutUnits) const;

protected:
	virtual void BeginPlay() override;

	void PlanBots();
	void ResolvePrep();
	void ResolveDash();
	void ResolveCombat();
	void ResolveMovement();

	// --- Pass della fase Blast -------------------------------------------------------------------
	//
	// `ResolveCombat` ordina; questi decidono. Ogni pass riceve il contesto della fase (`FRTBlastContext`)
	// e vi lascia cio' che i successivi leggeranno: la sequenza e' quella di `ResolveCombat`, e cambiarla
	// cambia il gioco — l'ordine fra controllo, danno e spostamento e' una regola del catalogo, non un
	// dettaglio di implementazione. Sono metodi e non funzioni libere perche' scrivono nei membri che
	// devono sopravvivere alla fase: `TurnLog`, `TeamKnowledgeState`, `ReactionBlockedThisTurn`.

	/** Raccoglie le unita' del livello, le ordina per cella e costruisce identita', stati e copia posizionale. */
	void GatherBlastUnits(FRTBlastContext& Ctx) const;

	/**
	 * Rinfresca la conoscenza di squadra (CP 13.2) sulle posizioni POST-Dash.
	 * Qui e non a inizio turno: chi ha caricato in mezzo al campo si e' esposto, e l'avversario deve saperlo
	 * prima di sparare. Osservare prima dello scatto darebbe una fotografia che nessuna fase usa.
	 */
	void RefreshTeamKnowledgeForBlast(const FRTBlastContext& Ctx);

	/**
	 * `Action.Cleanse` (CP 5.2): risolve PRIMA del ciclo degli intenti, che consuma `PlannedAbilityIndex`.
	 * Il controllo (codice 30) viene prima del danno (40): purificarsi da `Exposed` dopo averne incassato il
	 * malus non servirebbe a niente.
	 */
	void ResolveCleanseActions(FRTBlastContext& Ctx);

	/**
	 * `Action.Heal` (CP 8.5): si RACCOGLIE qui, prima che il ciclo degli intenti azzeri i piani, e si applica
	 * dopo i danni — la priorita' 70 del catalogo la mette dopo gli attacchi (50-65), quindi cura le ferite di
	 * questo turno, non quelle del turno prima.
	 */
	void CollectHealActions(FRTBlastContext& Ctx);

	/**
	 * Traduce i piani delle unita' in intenti d'attacco: valida l'ABILITA' (esiste, non e' uno scatto, e'
	 * utilizzabile), orienta chi ha un bersaglio ([D-020]), consuma la conoscenza di squadra sul targeting
	 * (CP 13.2) e applica il fallback DICHIARATO quando l'istanza non regge. La GEOMETRIA — portata, linea di
	 * tiro, celle colpite — non si valuta qui: la valida `URTHexCombatLibrary` sul piano.
	 *
	 * Intercetta anche le azioni che NON sono intenti d'attacco ma risolvono in questa fase
	 * (`Action.ModifyArc`) o in una successiva (fase `Environment` -> Cleanup), perche' questo ciclo azzera
	 * `PlannedAbilityIndex` per ogni unita': un pass successivo non troverebbe piu' nulla da leggere.
	 */
	void CollectAttackIntents(FRTBlastContext& Ctx);

	/**
	 * Aggiunge come intenti gli impatti delle cariche risolte nella fase Dash, e svuota la coda.
	 * Il movimento e' avvenuto prima, il colpo risolve qui per priorita': applicarlo dentro il Dash lo
	 * avrebbe messo fuori dall'ordine del catalogo.
	 */
	void AppendChargeImpactIntents(FRTBlastContext& Ctx);

	/**
	 * `Action.Interrupt` (CP 4.7): toglie dal piano i colpi di chi e' stato interrotto, e il colpo
	 * dell'Interrupt stesso. Filtra `Plan.Hits` PRIMA che diventino danno o eventi, cosi' un'abilita' ad area
	 * interrotta sparisce in un colpo solo — cosa che il registry degli effetti non saprebbe fare, perche' sa
	 * tradurre effetti su un bersaglio ma non «annulla l'azione X».
	 */
	void ApplyInterrupts(FRTBlastContext& Ctx);

	/**
	 * `Action.Intercept` (CP 5.3): riscrive il bersaglio di un attacco altrui. Ha un pass tutto suo, PRIMA
	 * delle altre reazioni, perche' il catalogo le da' la priorita' piu' bassa fra le reazioni: se risolvesse
	 * insieme alle altre, il bersaglio originale valuterebbe il proprio Counter su un colpo che non riceve
	 * piu'. Decide su colpi congelati, poi applica — e la rivalidazione della geometria sul nuovo bersaglio
	 * avviene qui, dove nessuna reazione e' ancora stata valutata sui colpi riscritti.
	 */
	void ResolveInterceptions(FRTBlastContext& Ctx);

	/**
	 * Pass delle reazioni sui colpi del Blast (CP 5.1). Raccoglie in `Ctx.Reactions` cio' che il chiamante
	 * applica piu' tardi — riduzione del danno e contrattacchi — mentre le fughe le applica al proprio
	 * interno, dopo aver valutato tutte le reazioni sullo snapshot congelato ([D-094]).
	 */
	void RunBlastReactions(FRTBlastContext& Ctx);

	/** Registra nel TurnLog gli intenti che la copertura ha fermato: l'attacco non avviene, e si dice perche'. */
	void LogBlockedIntents(const FRTBlastContext& Ctx);

	/**
	 * Applica alla mappa cio' che il Blast ha deciso: danno alle strutture (CP 9.2), operazioni sugli archi
	 * (CP 9.4) e ordini alle porte (CP 9.3).
	 *
	 * Tutto ORA, a colpi risolti, e non durante la raccolta: chi ha sparato in questo Blast non guadagna la
	 * linea perche' il muro e' caduto — la vista e il grafo si riaprono dalla fase successiva, e l'ordine dei
	 * colpi non cambia l'esito (invariante #3). La mappa che scrive e' la COPIA di lavoro dell'actor, non
	 * l'asset su disco.
	 */
	void ApplyEnvironmentChanges(FRTBlastContext& Ctx);

	/**
	 * Spinta e trazione (CP 4.7), applicate DOPO il danno sulle posizioni dello snapshot del Blast.
	 *
	 * Include il punto di valutazione di `Reaction.Anchor` (CP 7.5): gli spostamenti sono decisi e non
	 * ancora applicati, ed e' l'unico momento in cui annullarli e' possibile — dopo, vorrebbe dire rimettere
	 * indietro un'unita' gia' mossa, con due voci di TurnLog che si contraddicono sullo stesso passo.
	 * UNA chiamata per spinta e trazione insieme: chi e' spinto **e** tirato reagisce una volta sola.
	 */
	void ApplyDisplacements(FRTBlastContext& Ctx);

	/**
	 * Decide QUALI attaccanti pagano — i sopravvissuti — e assegna energia o la voce `Ultimate!`.
	 *
	 * ⚠️ Il cooldown NON lo scrive: annota con `MarkAbilitySpent`, e a pagare e' `SpendStartedAbilities`
	 * (`#1451` punto 3). Si chiamava `ConsumeAttackerAbilities`, e il nome e' stato cambiato perche' dopo
	 * quel refactor non consumava piu' niente: chi cercasse `Consume` per capire dove nasce un cooldown
	 * sarebbe atterrato qui senza trovare nessuna scrittura.
	 */
	void MarkAttackerAbilitiesSpent(FRTBlastContext& Ctx);

	/**
	 * L'UNICO punto in cui un'azione pianificata del Blast paga il proprio cooldown (`#1451` punto 3).
	 *
	 * Consuma cio' che i pass hanno annotato con `FRTBlastContext::MarkAbilitySpent`. Il criterio
	 * «l'azione e' PARTITA» resta a chi annota — [D-200] lo scrive per la portata — e qui non si
	 * ridecide niente, nemmeno `IsAlive()`: le guardie di vita valgono al momento dell'annotazione.
	 * Il contesto e' `const` perche' questa passata non ha niente da aggiungergli. ⚠️ **Non e' il
	 * compilatore a difendere l'asimmetria di [D-209]**: `IsAlive()` e' un metodo const e `SpentActors[i]`
	 * restituisce un puntatore a non-const, quindi infilare qui `if (!Attore->IsAlive()) continue;`
	 * compila benissimo. A renderlo rosso sono le due righe di `PlannedActionPaysOnlyIfItStarted`.
	 */
	void SpendStartedAbilities(const FRTBlastContext& Ctx);

	/** La sequenza dei pass del Blast. Ha un'uscita anticipata: il pagamento sta in `ResolveCombat`. */
	void ResolveCombatPasses(FRTBlastContext& Ctx);

	/**
	 * Applica ai bersagli sopravvissuti gli stati dichiarati dai colpi, consultando prima chi ha annullato il
	 * controllo con una reazione. Quale controllo salti lo decide QUI chi applica, che ha davanti la lista
	 * completa e sceglie il piu' grave: deciderlo nel pass sarebbe sceglierlo due volte, in due posti che
	 * possono divergere.
	 */
	void ApplyControlStatuses(FRTBlastContext& Ctx);

	/**
	 * Azioni AMBIENTALI pianificate (fase `Environment`, codice 50 del catalogo): risolvono nel **Cleanup**,
	 * dopo il Move, cosi' colpiscono anche chi e' appena entrato nella cella. Oggi: la propagazione elettrica
	 * di `Action.Electrify` (CP 8.3).
	 *
	 * Precede il danno di `Status.Burning` nell'ordine del Cleanup: la scarica e' un evento istantaneo
	 * dell'ambiente, il bruciore e' un danno a tempo che matura a fine turno. L'ordine e' osservabile — chi
	 * muore prende un reason code diverso nel TurnLog — quindi e' dichiarato, non lasciato al caso.
	 */
	void ResolveEnvironment(URTHexMapAsset* Map);

	/**
	 * Il pass delle REAZIONI (CP 5.1): per ogni unita' con una reazione pianificata valuta il trigger sui colpi
	 * gia' raccolti, ne registra l'esito nel TurnLog — sempre, anche la non-attivazione — e traduce gli effetti
	 * della reazione attivata in cio' che il chiamante applichera' insieme al resto della fase.
	 *
	 * **Una funzione e non un blocco dentro `ResolveCombat` perche' con `D-092` i punti di valutazione sono
	 * piu' d'uno**: i trigger che non nascono da un colpo si valutano dove il loro evento e' deciso.
	 *
	 * `Point` dice QUALE punto sta girando: il pass guarda solo le unita' il cui trigger appartiene a questo
	 * punto (`URTReactionLibrary::PassPointFor`) e ignora le altre — senza il filtro, la stessa reazione
	 * prenderebbe una voce `NotTriggered` in ogni punto del turno invece che nel suo.
	 *
	 * `Evaluate` decide **se** il trigger scatta e **chi** l'ha innescato: e' il chiamante a saperlo, perche'
	 * cambia con il punto (i colpi raccolti, gli spostamenti decisi, la cella sotto i piedi). Il pass non
	 * conosce nessuna di queste cose, e cosi' resta uno solo.
	 *
	 * Le fughe di chi reagisce (`SelfReposition`) si applicano DENTRO, alla fine: e' il punto di `D-094` — tutte
	 * le reazioni valutate sullo snapshot congelato, poi si muove. Vedi `FRTReactionPassResult` per cosa esce.
	 */
	void RunReactionPass(ERTReactionPassPoint Point,
		TFunctionRef<FRTReactionTriggerHit(int32 /*SelfId*/, ERTReactionTrigger)> Evaluate,
		const TArray<ARTUnit*>& Units, TArray<FRTUnitCombatState>& States,
		const URTHexMapAsset* Map, FRTReactionPassResult& Out);

	/**
	 * Colpi predittivi armati nella Prep di QUESTO turno, consumati al boundary del Move (E18).
	 *
	 * Vive sul TurnManager e non sull'unita' perche' la Prep azzera `PlannedAbilityIndex` appena consuma
	 * l'abilita': dopo quel punto il piano non esiste piu', e la previsione dev'essere gia' stata catturata.
	 * Si svuota a ogni Move, quindi un colpo non sopravvive al proprio turno.
	 */
	UPROPERTY(Transient)
	TArray<FRTArmedPrediction> ArmedPredictions;

	/**
	 * Overwatch armati nella Prep di QUESTO turno, consumati durante i micro-step del Move (CP 14.5).
	 *
	 * Stesse due ragioni del gemello qui sopra — la Prep azzera `PlannedAbilityIndex` appena consuma
	 * l'abilita', e un armamento non sopravvive al proprio turno — piu' una terza che e' solo sua: a
	 * differenza della previsione, che si valuta UNA volta al boundary, questo si rilegge a **ogni**
	 * micro-step, e fra un passo e l'altro il suo stato cambia (`bCharged` cade al primo `FIRE`).
	 */
	UPROPERTY(Transient)
	TArray<FRTArmedOverwatch> ArmedOverwatches;

	/**
	 * Risolve i colpi predittivi armati contro le rotte appena calcolate, e TRONCA il movimento di chi viene
	 * colto (E18 CP 18.2). Modifica `Resolved` in luogo: chiamata prima che il TurnLog sia costruito.
	 *
	 * La decisione sta nello strato PURO (`URTPredictiveLibrary`); qui restano solo le tre cose che il mondo
	 * possiede — chi e' ostile a chi, il danno sulle unita' vere, e le voci di log. Tenerle separate e' cio'
	 * che permette a `Predictive.PermutationInvariant` di esistere come test senza un `UWorld`.
	 */
	void ResolvePredictiveBoundary(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units, TArray<FRTHexMoveResult>& Resolved);

	/**
	 * Apre le finestre di reazione del micro-step appena eseguito, raccoglie le decisioni e le applica
	 * (CP 14.5). Chiamata FRA un `ResolveNextHexMicroStep` e il successivo.
	 *
	 * E' l'orchestratore del Decision Boundary: costruisce i watcher dallo stato corrente, chiede allo strato
	 * puro quali opportunity esistono, e per quelle con `AllowedResponses >= 2` interroga il decisore. Il
	 * resolver non attende mai: non c'e' `Sleep`, `Delay`, timer ne' Timeline qui dentro ne' sotto: la
	 * sospensione e' il fatto che questa funzione **ritorni** prima del micro-step successivo.
	 *
	 * `MicroStepIndex` e' l'indice del passo nel TURNO, e viaggia fino dentro `FRTReactionOpportunityKey`:
	 * senza, due passi dello stesso turno avrebbero lo stesso `OpportunityId`.
	 */
	void ResolveReactionBoundary(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units,
		FRTMovementResolutionState& State, const TArray<int32>& MovedUnitIds, int32 MicroStepIndex);

	/**
	 * Apre UNA finestra e ne restituisce l'esito (CP 14.5). Non applica nulla: decide soltanto.
	 *
	 * Separata da `ApplyReactionDecision` perche' sono due responsabilita' che falliscono in modi diversi —
	 * qui si sbaglia a raccogliere una risposta, li' a tradurla in effetti — e perche' e' la separazione che
	 * rende il replay possibile: rieseguire un turno significa **saltare** questa e chiamare solo quella, con
	 * le risposte lette dal TurnLog invece che chieste a qualcuno.
	 *
	 * Cardinalita' <= 1 non apre nessuna finestra e non interroga nessuno (ADR-0004 §2).
	 */
	FRTReactionDecision AskReactionDecision(const FRTReactionOpportunity& Opportunity, int32 OwnerUnitId,
		bool bOwnerIsBot) const;

	/**
	 * Applica l'esito di una finestra: `FIRE` colpisce, spende la charge e TRONCA il movimento residuo del
	 * bersaglio; `HOLD` non fa nulla e lascia la reaction armata (CP 14.5).
	 *
	 * `ArmedIndex` indicizza `ArmedOverwatches`: e' l'armamento la cui charge si consuma, e tenerlo esplicito
	 * evita di ricercarlo una seconda volta con una regola che potrebbe non coincidere con la prima.
	 */
	void ApplyReactionDecision(const URTHexMapAsset* Map, const TArray<ARTUnit*>& Units, FRTMovementResolutionState& State,
		const FRTReactionOpportunity& Opportunity, const FRTReactionDecision& Decision, int32 ArmedIndex);

	/**
	 * Chi risponde per l'unita' `OwnerUnitId` a questa opportunity. Restituisce una delle `AllowedResponses`,
	 * oppure una stringa vuota per «non ho risposto» — che l'orchestratore tratta come scadenza.
	 *
	 * ⚠️ **Non e' il seam dei `DecisionProvider` di [D-101]** (`#542`, v0.2), ne' il `DecisionProvider`
	 * iniettabile di CP 15.3 meta' B (`#512`). Sono tre cose in tre release, e la issue di questo checkpoint
	 * avverte per nome di non confonderle: questa e' la piu' semplice delle tre — un solo punto di
	 * sostituzione, nessuna politica, nessun contratto sugli esiti. Quando D-101 arrivera', questo delegate e'
	 * cio' che verra' sostituito, non un concorrente da riconciliare.
	 *
	 * Non legato: default di sistema, cioe' `Timeout -> HOLD`. E' fail-closed nel verso giusto — senza un
	 * decisore la charge non si spende.
	 */
	DECLARE_DELEGATE_RetVal_TwoParams(FString, FRTReactionDeciderSignature,
		const FRTReactionOpportunity& /*Opportunity*/, int32 /*OwnerUnitId*/);

	/**
	 * Il decisore corrente. Sostituibile dai test per scriptare le risposte senza un timer reale, che e'
	 * l'unico modo in cui `Overwatch.DecisionIsReplayable` puo' essere un test e non una sessione manuale.
	 *
	 * Pubblico perche' iniettarlo E' il suo scopo; `Transient` perche' un delegate non si serializza.
	 */
public:
	FRTReactionDeciderSignature ReactionDecider;

	/**
	 * Arma il manager con le decisioni di reazione GIA' PRESE, lette da una traccia (`#886`).
	 *
	 * E' la meta' che mancava al DoD di CP 14.5: *«la decisione entra nel TurnLog e il replay la riproduce
	 * senza reinterrogare nessuno»*. Da qui in poi `AskReactionDecision` consulta questa mappa **al posto**
	 * dei rami «bot» e «decisore legato» — non prima del gate di cardinalita', che resta calcolato ([D-109]).
	 *
	 * Chiama chi **ri-simula** (il Verifier: test, corpus golden, diagnosi), mai il gioco. Con la mappa vuota
	 * — cioe' sempre, in partita — il comportamento e' identico a quello di prima: il costo per il percorso
	 * normale e' un `Num() == 0`.
	 *
	 * ⚠️ Le voci `HoldImmediate` **si scartano** invece di essere caricate. Quelle finestre non vengono mai
	 * consultate (le precede il gate di cardinalita'), quindi tenerle qui le farebbe risultare **orfane a fine
	 * corsa su una ri-simulazione perfettamente riuscita** — un falso positivo sistematico — e le renderebbe
	 * applicabili per errore a una finestra che non e' la loro.
	 *
	 * ⚠️ La risposta si **ricostruisce** da `Outcome` e `SelectedTargetUnitId`: il TurnLog non porta la
	 * stringa. `FireChosen` da' `FIRE:<id>`, ogni altro esito da' `HOLD` — che e' cio' che quegli esiti hanno
	 * applicato, per quanto diverso sia il modo in cui ci sono arrivati.
	 */
	void ArmRecordedReactionDecisions(const TArray<FRTTurnLogEntry>& TraceEntries);

	/**
	 * I disaccordi fra traccia e ri-simulazione, in ordine di rilevazione ([D-170]).
	 *
	 * Vuoto = la ri-simulazione ha reclamato ogni risposta registrata e nessuna finestra e' rimasta scoperta.
	 * **Non e' un canale di gioco**: un fatto della partita entra nella traccia, un giudizio sulla verifica
	 * no — e un esito nuovo su `ERTReactionDecisionOutcome` avrebbe fatto differire la traccia ri-simulata
	 * dall'originale proprio quando il confronto degli hash deve dire qualcosa.
	 *
	 * ⚠️ Non sostituisce `DescribeFirstDivergence`, che confronta due tracce **a valle**: questo nasce
	 * **durante** la ri-simulazione, sulla singola chiave, e dice *quale* chiave — che un hash diverso non sa.
	 */
	const TArray<FString>& GetVerificationDivergences() const { return VerificationDivergences; }

	/**
	 * Registra come divergenza ogni risposta registrata che nessuna finestra ha reclamato (`#886`, voce 4).
	 *
	 * Va chiamata **a fine ri-simulazione**, quando «non ancora consumata» e «mai consumata» smettono di
	 * essere la stessa cosa. Una chiave orfana e' il sintomo che le chiavi hanno smesso di combaciare —
	 * `MicroStepIndex` e `Seq` sono funzione dello svolgimento — e ignorarla in silenzio e' precisamente il
	 * difetto che `#886` esiste per prevenire: una divergenza indistinguibile dal successo.
	 */
	void ReportOrphanRecordedDecisions();

protected:

	/**
	 * Le risposte registrate, per `OpportunityId`. Vuota fuori dalla ri-simulazione.
	 *
	 * ⚠️ **Una mappa e non un array**: l'`OpportunityId` e' un'IDENTITA', non un indice. Indicizzare per
	 * ordine di comparsa sembrerebbe equivalente e non lo e' — applicare una risposta cambia i micro-step
	 * successivi, quindi cambia le chiavi delle finestre successive, e la prima finestra che non si riapre
	 * farebbe scorrere tutte le altre di uno.
	 */
	TMap<FString, FRTReactionDecision> RecordedDecisions;

	/**
	 * Le chiavi gia' consumate, per distinguere l'orfana dalla non-ancora-vista.
	 *
	 * `mutable` insieme al gemello qui sotto, e la ragione va detta perche' un `mutable` e' quasi sempre un
	 * odore: `AskReactionDecision` e' `const` per dichiarare che **non tocca lo stato di gioco**, e quella
	 * garanzia resta intatta. Questi due campi sono contabilita' della VERIFICA — chi ha reclamato cosa, e
	 * cosa non tornava — cioe' esattamente la categoria che [D-170] tiene fuori dal mondo di gioco.
	 */
	mutable TSet<FString> ConsumedDecisionKeys;

	/** Il verdetto in costruzione. Vedi `GetVerificationDivergences`. */
	mutable TArray<FString> VerificationDivergences;

	/**
	 * Applica le cure raccolte da `ResolveCombat` (CP 8.5). Chiamata da DUE punti — dopo i danni, e nel ramo
	 * "nessun colpo in questo turno" — perche' una cura fuori da uno scontro e' il caso normale di un
	 * supporto, non un'eccezione: con un solo call site la cura sparirebbe in silenzio quando nessuno attacca.
	 *
	 * `Healers` e' parallelo a `Targets`: chi CURA, che il TurnLog deve poter nominare (#405). Non si deduce
	 * da `Sources` — quella e' una cella, e una cella non e' un'unita' ([D-063]).
	 */
	void ApplyPlannedHeals(const TArray<ARTUnit*>& Targets, const TArray<int32>& Amounts,
		const TArray<FRTCellId>& Sources, const TArray<ARTUnit*>& Healers,
		const TArray<FRTActionDef>& Defs);

	/**
	 * Voce di TurnLog per uno spostamento SUBITO — spinta o trazione (#307). Chiamata dai due punti che
	 * spostano un'unita' contro la sua volonta', che scrivono la stessa voce: l'esito e' `Displaced` per
	 * entrambi, e a distinguerli sono le celle (allontanarsi dalla sorgente o avvicinarsi).
	 *
	 * Una funzione e non due blocchi copiati perche' la differenza fra i due call site e' zero: un secondo
	 * blocco identico e' esattamente il posto dove, fra sei mesi, una correzione viene applicata a uno solo.
	 *
	 * `Steps` e' quante celle sono state attraversate — la lunghezza della linea meno la partenza — e finisce
	 * in `Amount`, dove le voci di movimento portano gia' quel numero.
	 */
	void AppendDisplacementEntry(const ARTUnit* Target, const FRTCellId& From, const FRTCellId& To, int32 Steps,
		const TMap<ARTUnit*, FRTDisplacementCause>& CauseByTarget);

	/**
	 * Applica uno spostamento FORZATO gia' deciso: i dieci passi che devono avvenire tutti, in un posto solo
	 * (`#541`).
	 *
	 * Non calcola **dove** — quello lo fanno `HexKnockbackDestination` e la sua gemella per la trazione, e
	 * restano separate perche' la direzione e' l'unica cosa che davvero distingue una spinta da un tiro.
	 * Questa applica **come**, ed e' identica per entrambe: log, percorso, voce di TurnLog con la causa
	 * (`#307`), evento di playback, cella nuova, facing verso la sorgente (CP 16.1), posizione visiva, hazard
	 * di **ogni** cella attraversata (`#308`), e il piano che segue l'unita' invece di riportarla indietro.
	 *
	 * ⚠️ **Esisteva gia' due volte**, per `Push` e per `Pull`, riga per riga uguale tranne il verbo del log,
	 * la mappa delle cause e la sorgente del facing. La terza copia sarebbe arrivata con `SelfReposition`
	 * (D-093), e la terza e' quella che trasforma una duplicazione in un pattern da imitare: chi aggiunge il
	 * quarto produttore di spostamento copia da una delle tre, e prima o poi ne copia una a cui manca un
	 * passo. Ogni passo omesso e' un difetto gia' pagato — `#307` la causa, `#308` gli hazard, il piano che
	 * non segue riporta l'unita' indietro nel Move.
	 *
	 * `FacingSource` e' la cella **verso cui** l'unita' si gira: chi spinge per la spinta, chi tira per la
	 * trazione, chi ha innescato per una fuga ([D-104](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * ⚠️ `InPhase` viaggia fino agli hazard attraversati (passo 8): uno spostamento forzato nasce in fasi
	 * diverse — spinta e trazione nel `Blast`, fuga in `Dash` o in `Cleanup` — e il danno da terreno che ne
	 * consegue deve dichiarare **quella**, non una fissa (`#1067`).
	 */
	void ApplyForcedDisplacement(ARTUnit* Unit, const FRTCellId& NewCell, const FRTCellId& FacingSource,
		const TMap<ARTUnit*, FRTDisplacementCause>& CauseByTarget, const TCHAR* LogVerb,
		const URTHexMapAsset* Map, ERTMatchPhase InPhase);

	/**
	 * Voce di TurnLog per uno spostamento forzato ANNULLATO (#420): la spinta e' stata registrata, risolta, e
	 * l'unita' e' rimasta dov'era. Il gemello negativo di `AppendDisplacementEntry`, e con la stessa forma —
	 * fase `Blast`, categoria `Move`, causa presa dalla stessa mappa.
	 *
	 * Le celle sono entrambe quella dell'unita': non si e' spostata, e la voce lo dice invece di lasciarlo
	 * dedurre. Il PERCHE' viaggia in `Amount` come `ERTDisplacementBlockReason` — vedi il commento dell'enum.
	 *
	 * Chiamata da cinque punti del ciclo di knockback, che sono i cinque modi RAGGIUNGIBILI di non muoversi.
	 * Il sesto (`PushResistance`) e' dormiente per D-075 e non ha produttore: aggiungerlo darebbe un valore di
	 * enum che nessun test puo' coprire.
	 *
	 * `CauseByTarget` e' un PUNTATORE e non un riferimento perche' un call site su cinque non ha una causa
	 * sola da nominare: con `OpposingForces` gli attaccanti sono due o piu', e la mappa ne conserva uno solo —
	 * l'ultimo scritto. Scriverlo direbbe a un replay che a fermare l'unita' e' stata QUELLA azione, che e'
	 * falso. `nullptr` lascia `ActionId` vuoto, che e' la verita' disponibile.
	 */
	void AppendDisplacementResistedEntry(const ARTUnit* Target, ERTDisplacementBlockReason Reason,
		const TMap<ARTUnit*, FRTDisplacementCause>* CauseByTarget);

	/**
	 * Modifiche TEMPORANEE alla mappa (CP 8.4): fuoco acceso, acqua creata. Il terreno dinamico vive in due
	 * pezzi, e la divisione non e' casuale:
	 * - la superficie **corrente** sta nella mappa, perche' e' cio' che tutti leggono gia' (costi, Dash,
	 *   ghiaccio, targeting, on-enter, conduttivita'): un secondo posto da consultare sarebbe un secondo
	 *   modello di verita', e prima o poi qualcuno leggerebbe solo uno dei due;
	 * - la superficie **originale** e i turni rimanenti stanno qui, perche' sono stato di PARTITA e non dato
	 *   di mappa: due partite sulla stessa arena non devono ereditarsi il fuoco a vicenda.
	 */
	struct FRTDynamicSurface
	{
		ERTHexSurface Original = ERTHexSurface::Floor;
		int32 TurnsRemaining = 0;
	};
	TMap<FRTCellId, FRTDynamicSurface> DynamicSurfaces;

	/**
	 * Ponti TEMPORANEI creati da `Action.ModifyArc` (CP 9.4). Stessa divisione di `DynamicSurfaces`: l'arco
	 * corrente sta nella mappa, perche' e' cio' che il grafo legge gia'; la scadenza sta qui, perche' e' stato
	 * di PARTITA — due partite sulla stessa arena non devono ereditarsi i ponti a vicenda.
	 */
	struct FRTDynamicArc
	{
		FRTCellId From;
		FRTCellId To;
		int32 TurnsRemaining = 0;
		/**
		 * Turno in cui il ponte e' nato. Serve al tick: `ModifyArc` risolve nel **Blast** e la scadenza gira
		 * nel **Cleanup dello stesso turno**, quindi senza questo dato un ponte da 2 turni ne perderebbe uno
		 * prima ancora che qualcuno possa attraversarlo. Le superfici dinamiche non hanno il problema perche'
		 * nascono nel Cleanup, dopo il proprio tick.
		 */
		int32 CreatedOnTurn = 0;
	};
	TArray<FRTDynamicArc> DynamicArcs;

	/** Scadenza dei ponti temporanei, nel Cleanup: a zero turni l'arco sparisce, e si registra. */
	void TickDynamicArcs(URTHexMapAsset* Map);

	/**
	 * Coperture TEMPORANEE erette in partita da `Action.CreateCover` (CP 9.5). Stessa divisione di
	 * `DynamicSurfaces` e `DynamicArcs`: la copertura corrente sta nella mappa, perche' e' cio' che combat,
	 * vista e grafo leggono gia'; la scadenza sta qui, perche' e' stato di PARTITA.
	 *
	 * A differenza del ponte NON serve il turno di nascita, e la ragione e' la fase: `CreateCover` risolve in
	 * **Prep**, cioe' PRIMA del Blast che la usera'. Un pannello da 2 turni eretto nel turno N deve riparare il
	 * Blast di N e quello di N+1, quindi il tick del Cleanup di N deve gia' scalare — mentre il ponte, che
	 * nasce nel Blast, salta il proprio turno perche' altrimenti perderebbe l'unica fase Move per cui e' stato
	 * costruito. Le due regole divergono per la fase in cui nascono, non per svista.
	 */
	struct FRTDynamicCover
	{
		FRTCellId Cell;
		ERTHexDirection Edge = ERTHexDirection::E;
		/** Turni che restano. **0 = non scade da sola** (pannello adattivo): il tick la salta. */
		int32 TurnsRemaining = 0;
		/**
		 * Rotazioni ancora gratuite (pannello adattivo: 1). Una `Reconfigure` che ne consuma una non spende il
		 * cooldown: e' il compromesso che il catalogo eroi dichiara in cambio di 25 punti struttura invece di 30.
		 */
		int32 FreeRotations = 0;
	};
	/**
	 * Tutte le coperture erette IN PARTITA, anche quelle che non scadono: la lista non serve solo alla
	 * scadenza, ma a sapere quali coperture sono «di partita» e quali erano gia' sulla mappa — e a portarsi
	 * dietro le rotazioni gratuite quando una viene spostata.
	 */
	TArray<FRTDynamicCover> DynamicCovers;

	/** Scadenza delle coperture temporanee, nel Cleanup: a zero turni il bordo torna scoperto, e si registra. */
	void TickDynamicCovers(URTHexMapAsset* Map);

	/**
	 * Pass delle strutture di BORDO nella fase Prep (CP 9.5): raccoglie le richieste di `Action.CreateCover`,
	 * le ordina e le applica a fase conclusa. Ritorna quante operazioni hanno cambiato il campo davvero —
	 * erezioni **e** spostamenti: e' il numero che accende il beat di Prep nel playback, e un turno in cui
	 * l'unico evento e' una `Reconfigure` non deve restare muto per lo spettatore.
	 *
	 * Sta fuori da `ResolvePrep` (il motore azioni, che lavora su eventi verso UNITA') per la stessa ragione
	 * per cui `ModifyArc` sta fuori dalla raccolta degli intenti del Blast: il suo esito e' una modifica della
	 * mappa, e passare dagli effetti la farebbe ripiegare sul campo legacy `Power`.
	 */
	int32 ResolveCoverStructures(const TArray<ARTUnit*>& Units);

	/**
	 * Cambia la superficie di una cella per `Turns` turni, registrandolo nel TurnLog. Ritorna falso (e non
	 * cambia nulla) se la destinazione non ammette la trasformazione — l'acqua e il metallo non prendono
	 * fuoco. Un secondo cambio sulla stessa cella **non** perde l'originale: si sovrascrive la superficie
	 * corrente e si tiene la prima, altrimenti una cella allagata e poi incendiata non tornerebbe mai
	 * com'era.
	 */
	bool ApplyDynamicSurface(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexSurface NewSurface, int32 Turns,
		const FName& CauseActionId, const ARTUnit* Cause);

	/** Scadenza delle modifiche temporanee, nel Cleanup: a zero turni la cella torna com'era, e si registra. */
	void TickDynamicSurfaces(URTHexMapAsset* Map);
	void StartPlanningTimer();
	void OnPlanningTimeout();

	/**
	 * Contesto geometrico della mappa esagonale del livello: origine dell'actor e dimensioni prese dall'asset
	 * autorevole (o dall'actor, se l'asset manca). Unico punto da cui passano tutte le conversioni cella->mondo,
	 * cosi' risoluzione e playback non possono divergere di scala. Ritorna l'asset (nullptr se non c'e' mappa).
	 */
	const URTHexMapAsset* GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const;

	/** Termina il turno: se la partita e' decisa la chiude, altrimenti riapre la pianificazione. */
	void ConcludeTurn();

	/** Distrugge definitivamente le unita' morte (morte visiva differita): chiamato a fine turno. */
	void DestroyDefeatedUnits();

	/** Avvia il playback della risoluzione (movimento in parallelo, fasi a beat). */
	void BeginPlayback();
	void EnterPlaybackPhase();
	void TickPlayback(float DeltaSeconds);
	void FinishPlayback();
	/**
	 * I due termini della fase — movimento e attesa — prima che il budget tocchi il secondo.
	 * Raccoglie gli ingressi che solo il TurnManager possiede e delega la formula a
	 * `URTPlaybackLibrary::PhaseTime`.
	 */
	FRTPhaseTime PhaseTimeForPlaybackPhase(ERTMatchPhase InPhase) const;

	/** La durata della fase come sara' riprodotta: locomozione intatta, attese scalate dal budget. */
	float DurationForPlaybackPhase(ERTMatchPhase InPhase) const;

	// --- Sonda di pacing ------------------------------------------------------------------------
	/**
	 * Fotografa le unita' del mondo nei quattro interi che il pacing osserva.
	 *
	 * E' l'UNICO punto in cui la sonda guarda gli Actor: da qui in poi le regole di conteggio sono funzioni
	 * pure di `URTPacingLibrary`, provate headless (#1818). Prima esistevano due `GetAllActorsOfClass` in
	 * punti diversi, con due filtri **volutamente diversi** fra loro e la differenza spiegata solo da un
	 * commento accanto a ciascuno.
	 */
	TArray<FRTPacingUnitFacts> CollectPacingUnitFacts() const;

	/**
	 * La telemetria di pacing, uscita da questa classe con l'ottava fetta di `#1818`.
	 *
	 * 🔑 Qui restava **stato** — dieci membri — per una cosa che nessuna regola legge. Ora l'orchestratore
	 * possiede il registratore e gli passa i fatti; la sequenza e i tempi vivono in `FRTPacingRecorder`, che
	 * si esercita **senza un mondo**.
	 */
	FRTPacingRecorder Pacing;

	/**
	 * Vero quando nessun umano sta pianificando: lo dichiara chi allestisce la sessione.
	 *
	 * ⚠️ **Non e' stato di pacing**, benche' il pacing sia l'unico a leggerlo: e' un fatto della SESSIONE, e i
	 * due accessori pubblici qui sopra lo espongono. E' rimasto qui quando il registratore e' uscito, e la
	 * prima stesura di quella fetta se l'era portato via — il compilatore l'ha detto subito, ed e' la ragione
	 * per cui un taglio si verifica compilando invece che rileggendo.
	 */
	bool bUnattendedSession = false;


	/**
	 * Registra un evento: lo scrive nel log LogRT (completo, diagnosi per sviluppatore) e lo accoda al
	 * combat log della HUD col suo SOGGETTO.
	 *
	 * 🔴 **Il soggetto e' OBBLIGATORIO, e non c'e' un valore che significhi «non ci ho pensato»** (`#1499`).
	 * Fino al 2026-08-28 il parametro era un `int32` con default `INDEX_NONE`, fail-open: un sito nuovo che
	 * nominava un nemico passava il filtro di conoscenza per omissione, e l'omissione non faceva rumore.
	 * Ora il tipo non converte da `int32`, quindi il compilatore enumera chi dimentica.
	 *
	 * Le due forme sono entrambe una dichiarazione, e si leggono:
	 * - `FRTLogSubject::Unit(U)` — la riga parla di quell'unita', e il verdetto di [D-223] si congela qui;
	 * - `FRTLogSubject::World()` — la riga riguarda tutti, e lo dice.
	 *
	 * ⚠️ **`World()` non e' il vecchio default con un nome nuovo**: il default diceva soltanto che nessuno
	 * aveva deciso, e copriva per sbaglio anche righe che nominavano un'unita'.
	 *
	 * Il caso che rende la distinzione concreta sono le **righe di morte**: prima passavano per omissione,
	 * adesso portano `World()` perche' [D-223] ha deciso che un'eliminazione e' pubblica. Stesso esito a
	 * schermo, ragione opposta — e la ragione e' cio' che il prossimo autore puo' cambiare sapendo cosa fa.
	 *
	 * Il censimento — quanti siti, su quali file — si rimisura sul branch: un numero letterale in un
	 * commento invecchia al primo sito aggiunto e nessuno lo rilegge.
	 */
	void AddLogEvent(const FString& Message, FRTLogSubject Subject);

	/**
	 * Il verdetto di [D-223] per un soggetto, calcolato ADESSO sulla conoscenza corrente di tutte le squadre.
	 *
	 * ⚠️ **Fail-closed quando non e' calcolabile**: un soggetto che porta il solo `StableUnitId` non ha
	 * squadra ne' cella, e `ClassifyTarget` le vuole entrambe. Restituisce `NoOne()` — una riga che non si
	 * legge, mai una che si legge per sbaglio.
	 */
	FRTKnowledgeVerdict FreezeVerdictFor(const FRTLogSubject& Subject) const;

	/**
	 * Applica gli OnEnterEffects (URTTerrainLibrary) di ogni cella in Entered a Unit: Damage via
	 * URTCombatLibrary::ApplyDamage, Status via Unit->ApplyStatus. Usata da ResolveDash e ResolveMovement
	 * sulle celle FRTHexMoveResult::Entered di ciascuna unita' (CP 8.1).
	 *
	 * ⚠️ **`InPhase` e' un parametro e non si legge da `Phase`**, ed e' il risultato di una verifica, non
	 * una preferenza di stile (`#1067`). Due ragioni, entrambe misurate:
	 * · il ciclo delle fasi in `LockInAndResolve` esce **quando `Phase == Planning`**, e la Cleanup gira
	 *   dopo — quindi durante tutta la Cleanup il membro vale `Planning`, che e' una fase che il replay non
	 *   osserva mai. E' il motivo per cui ogni voce di quella fase la scrive letteralmente;
	 * · questa funzione e' chiamata da **quattro** siti in **tre** fasi diverse — `ResolveDash` (`Dash`),
	 *   `ResolveMovement` (`Move`), `ResolveEnvironment` (`Cleanup`) e `ApplyForcedDisplacement`, che a sua
	 *   volta arriva da tre punti in fasi diverse. Nessun valore fisso sarebbe giusto per tutti.
	 */
	void ApplyTerrainOnEnterEffects(const URTHexMapAsset* Map, ARTUnit* Unit, const TArray<FRTCellId>& Entered,
		ERTMatchPhase InPhase);

	/** Le celle ENTRATE lungo un percorso: tutte tranne la partenza, dove l'unita' stava gia'. */
	static TArray<FRTCellId> CellsEnteredAlong(const TArray<FRTCellId>& Path);

	UPROPERTY()
	TArray<FRTCombatLogLine> RecentEvents;

	/**
	 * Le due istantanee di conoscenza del turno, per l'artefatto d'audit di [D-313] (`#2074`).
	 *
	 * ⚠️ **Due e non una**, e non e' ridondanza: `TeamKnowledgeState` ha esattamente due assegnazioni per
	 * turno — il refresh di Planning e quello di Blast — e le due rispondono a domande d'audit diverse. La
	 * prima e' cio' su cui il bot ha deciso, la seconda quella contro cui i verdetti sono stati congelati.
	 * Registrarne una sola renderebbe una delle due verifiche impossibile.
	 *
	 * ⛔ **Copie, non riferimenti, e non entrano in nessun hash**: registrare non deve poter cambiare cio'
	 * che si registra.
	 */
	TArray<FRTTeamKnowledge> PlanningKnowledgeForAudit;
	TArray<FRTTeamKnowledge> BlastKnowledgeForAudit;

	/**
	 * Le SCELTE dei bot del turno, catturate a pianificazione chiusa ([D-313], emendamento del 2026-09-02).
	 *
	 * 🔴 **E' il quarto record, e senza di lui l'equita' non e' verificabile da nessun archivio**: la scelta
	 * del bot non sopravvive alla pianificazione, e gli EFFETTI non la contengono — `TgtCell` su una voce di
	 * combattimento e' la cella della vittima, non quella a cui si e' mirato.
	 */
	TArray<FRTAuditBotDecision> BotDecisionsForAudit;

	/**
	 * Il turno a cui `BotDecisionsForAudit` appartiene, e serve per la stessa ragione per cui le due
	 * conoscenze portano il proprio `TurnNumber`: **le scelte di un altro turno non sono evidenza, sono un
	 * errore che sembra una prova**. `FRTAuditBotDecision` un numero di turno non ce l'ha — e' un record di
	 * scelta, non un'istantanea — quindi il timbro sta qui.
	 */
	int32 BotDecisionsTurnForAudit = INDEX_NONE;

	/** TurnLog dell'ultimo turno risolto (osservabilita' autoritativa; ordinato in LockInAndResolve). */
	TArray<FRTTurnLogEntry> TurnLog;

	/** Stato della registrazione in corso: id, hash per turno, chiusura. Lo tiene il manifest stesso. */
	/**
	 * L'archivio replay in scrittura, uscito da questa classe con la nona fetta di `#2286`.
	 *
	 * 🔑 Qui restavano il manifest e due timestamp per un carico **inerte all'esito**: registra cio' che
	 * il resolver ha gia' deciso, e nessuna regola lo rilegge. L'orchestratore raccoglie i fatti e li passa;
	 * la sequenza e lo stato vivono in `FRTReplayRecording`, che non conosce ne' il mondo ne' il TurnLog.
	 */
	FRTReplayRecording ReplayRecording;

	/** Scrive la traccia del turno appena risolto. Silenziosa se la registrazione e' spenta. */
	void RecordTurnToReplay();

	/** Chiude l'archivio a partita finita. Silenziosa se la registrazione e' spenta o non e' mai partita. */
	void CloseReplayArchive();

	/**
	 * Calcola il checksum dello stato e lo conserva, per la chiusura dell'archivio.
	 *
	 * Va chiamata **prima** di `DestroyDefeatedUnits` ([D-084]): dopo, le unita' cadute non esistono piu' e
	 * il digest non potrebbe piu' distinguere «tre vivi e un caduto» da «tre vivi e basta».
	 */
	void CaptureFinalStateHash();

	/** L'ultimo checksum catturato, usato alla chiusura. `0` = mai calcolato. */
	int64 PendingFinalStateHash = 0;

	/** La radice effettiva: l'override se c'e', altrimenti `Saved/Replays`. */

	/**
	 * Istante reale d'inizio registrazione, per la durata nel manifest. E' l'UNICO tempo reale che tocca
	 * l'archivio, e finisce in un campo che non entra in nessun hash.
	 */

	/** Istante d'inizio in UTC, per la riga dell'indice (`#416`). Il manifest porta una durata, non un inizio. */

	/** Rotte percorse da ogni unita' che si e' mossa nell'ultima risoluzione, ciascuna col proprio soggetto. */
	TArray<FRTMoveRoute> LastMoveRoutes;

	/**
	 * Quante righe di log il manager conserva.
	 *
	 * ⚠️ **Sei non bastano da CP 11.3 (#79)**, e il numero non e' estetico: da quando il log leggibile si
	 * DERIVA dal TurnLog (`ConcludeTurn`), un turno solo ne produce quante sono le sue voci — movimenti,
	 * colpi, reazioni, effetti d'ambiente. Con la finestra a sei, le righe del turno appena risolto
	 * spingevano fuori quelle emesse durante la risoluzione, e il DoD di #79 — «ogni esito deve essere
	 * spiegabile leggendo il log» — diventava insoddisfacibile per costruzione. Trovato da un test che
	 * cercava «Status.Wet da terreno» e non lo trovava piu' (`Terrain.Status.LogMatchesState`).
	 *
	 * Questa e' la memoria del MANAGER, non quanto ne mostra il widget: quante righe stanno a schermo lo
	 * decide l'HUD (CP 11.1), che puo' mostrarne meno senza che il log le perda.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Turn")
	int32 MaxLogLines = 60;

	/** Durata della fase di pianificazione; allo scadere scatta il lock-in automatico. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	float PlanningSeconds = 30.f;

	/**
	 * Durata della finestra Fast Reaction, in secondi (ADR-0004 §8, CP 14.6): **3,0 s**.
	 *
	 * Sta qui e non in `FRTMatchRules` per la ragione che quel file dichiara di se': e' un tempo di PARETE,
	 * non un parametro di regola da cui l'esito dipende. L'esito allo scadere e' `SafeResponse` — mai `FIRE`
	 * — e non cambia con la durata: quello che cambia e' quanto si ha per decidere.
	 *
	 * ⚠️ **Nasce con un lettore, ed e' il motivo per cui nasce adesso.** CP 14.5 rinvio' questo valore
	 * proprio perche' nessuno lo leggeva (`roadmap-v0.1.md`: *«spostata a CP 14.6: qui non aveva un
	 * lettore»*), e fino a oggi `FastReactionDuration` esisteva solo dentro due commenti. Il lettore e'
	 * `MakeReactionWindowView`, che lo consegna alla presentazione come countdown del DTO.
	 *
	 * 🔴 **La prima stesura diceva che il lettore era `FilterWindowForTeam`, e non era vero**: quella
	 * funzione riceve la durata come PARAMETRO e non ha mai letto questo campo. Il commento affermava un
	 * cablaggio inesistente — cioe' esattamente il difetto che descriveva — e lo ha trovato una code review.
	 *
	 * `ClampMin` sulla FONTE e non solo nel consumatore: un valore negativo qui produrrebbe un timer che non
	 * scatta mai per ogni lettore futuro, e un clamp che vive solo a valle protegge una copia sola.
	 *
	 * Server-authoritative: un client lento non allunga la finestra. In v0.1 offline la distinzione non e'
	 * osservabile, e il campo esiste al singolare apposta — due sorgenti sarebbero due verita' (ADR-0005 §4c).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn", meta = (ClampMin = "0.0"))
	float FastReactionDuration = 3.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 TurnNumber = 1;

	/**
	 * Cosa sa ogni squadra, e che SOPRAVVIVE al turno (CP 13.2). Ordinata per `TeamId`.
	 *
	 * Vive qui e non nello snapshot perche' lo snapshot e' una fotografia che nasce e muore dentro una fase,
	 * mentre la memoria del contatto e' esattamente cio' che deve attraversare il confine fra due turni — e'
	 * la sua unica ragione di esistere. Lo snapshot ne riceve una COPIA (`FRTHexSnapshot::TeamKnowledge`),
	 * cosi' i consumatori puri la leggono senza conoscere il TurnManager.
	 */
	TArray<FRTTeamKnowledge> TeamKnowledgeState;

	/** La conoscenza della squadra, o una vuota e di versione corrente se la squadra non ne ha ancora. */
	FRTTeamKnowledge KnowledgeForTeam(int32 TeamId) const;

	/**
	 * Rinfresca `TeamKnowledgeState` dalle posizioni ATTUALI delle unita' vive (CP 13.5).
	 *
	 * Serve a inizio pianificazione, dove `ResolveCombat` non e' ancora passato: senza, al primo turno la
	 * conoscenza e' **vuota** e un bot che pianifica su di essa sarebbe cieco invece che parziale — cioe' il
	 * filtro di percezione sembrerebbe funzionare mentre produce un bot che non fa niente.
	 *
	 * NON sostituisce il rinfresco dentro `ResolveCombat`, che resta dov'e' per la ragione scritta li': la
	 * posizione autorevole per il Blast e' quella POST-Dash, e chi ha caricato in mezzo al campo deve essere
	 * visto prima che si spari. Questo e' un secondo campione, allo stato di inizio turno.
	 *
	 * Idempotente rispetto alla scadenza dei ricordi: `Observe` scrive `TurnNumber` corrente sui contatti
	 * freschi, quindi chiamarla due volte nello stesso turno non allunga la memoria di nessuno.
	 */
	void RefreshTeamKnowledgeForPlanning(const TArray<ARTUnit*>& Live);

	/**
	 * Il boundary IN CORSO, che `AppendLogEntry` stampa su ogni voce come terza coordinata dopo turno e fase
	 * (`#2260`). Vive qui, e non come variabile locale del ciclo, per una ragione sola: il punto che CONTA i
	 * micro-step e il punto che SCRIVE la traccia sono funzioni diverse, e l'unico modo di tenerli d'accordo
	 * senza duplicare il contatore e' che ne leggano uno solo.
	 *
	 * ⚠️ **`INDEX_NONE` non e' «non inizializzato»: e' un valore con un significato**, ed e' quello che vale
	 * per la maggior parte della partita. Una voce nasce fuori da un ciclo di micro-step ogni volta che la
	 * sua fase non ne ha — Blast, status, hazard, objective, i rifiuti in Planning — e li' `INDEX_NONE` dice
	 * esattamente questo: *nessun ciclo qui*. Cosi' `0` dentro una traccia `WithMicroStep` torna a
	 * significare **una cosa sola**, il PRIMO boundary, invece di due.
	 *
	 * 🔑 **Lo ripristina chi lo alza, e per costruzione**: il ciclo di movimento lo porta a `0` quando
	 * comincia e un `ON_SCOPE_EXIT` lo rimette a `INDEX_NONE` quando esce. Senza quel ripristino ogni voce
	 * emessa DOPO il movimento erediterebbe in silenzio l'indice dell'ultima barriera, e direbbe di
	 * appartenere a un ciclo gia' finito. Affidarlo a un `return` che non dimentichi sarebbe affidarlo alla
	 * disciplina, che e' precisamente cio' che questo campo esiste per non dover chiedere.
	 */
	int32 CurrentMicroStepIndex = INDEX_NONE;

	/**
	 * Aggiunge una voce al TurnLog stampandoci i campi di CONTESTO della v6 (#405): turno, revisione del grafo
	 * e identita' dell'attore. Ogni emissione passa di qui — se un sito chiamasse `TurnLog.Add` direttamente,
	 * la sua voce nascerebbe senza contesto e nessun test se ne accorgerebbe.
	 *
	 * `Actor` e' chi ha AGITO, e va passato esplicitamente perche' dalla voce non si deduce: l'interposizione
	 * scrive in `SrcCell` la cella del protetto, e dopo un Dash la cella dell'attore non e' piu' quella di
	 * partenza. `nullptr` per le voci ambientali, che un'unita' non ce l'hanno — ed e' un valore da scegliere,
	 * non da subire: il parametro non ha default apposta.
	 */
	void AppendLogEntry(FRTTurnLogEntry& Entry, const ARTUnit* Actor);

	/**
	 * La stessa emissione, ma col SOGGETTO invece del solo attore.
	 *
	 * 🔴 **Serve a chi scrive prima di `PlaceOnCell`** (`#2142`): li' `Actor->Cell` e' la cella di partenza
	 * del turno, e sia il verdetto di [D-223] sia il soggetto d'audit di [D-313] verrebbero congelati su una
	 * posizione che l'unita' ha gia' lasciato. `FRTLogSubject::UnitAt` porta la cella dell'impatto, e questa
	 * e' la porta che la fa arrivare a entrambe le scritture — leggendola **una volta sola**, cosi' che non
	 * possano divergere.
	 *
	 * L'overload con l'`ARTUnit*` resta la forma corta per tutti gli altri produttori e delega a questa.
	 */
	void AppendLogEntry(FRTTurnLogEntry& Entry, const FRTLogSubject& Subject);

	/**
	 * Registra un cambio d'orientamento e ne appende le voci **passando da `AppendLogEntry`**.
	 *
	 * ⚠️ `URTFacingLibrary` lavora su `FRTHexSimUnit`, che porta l'INDICE della simulazione e non
	 * `StableUnitId`: la libreria non puo' riempire da sola i tre campi di contesto, e finche' i chiamanti le
	 * passavano `TurnLog` per riferimento ogni voce `Facing` derivata nasceva con **turno 0, revisione 0 e
	 * nessuna unita'** (`#1429`). Il commento di `AppendLogEntry` prometteva che ogni emissione passasse di
	 * li'; era vero *del file*, non del TurnLog.
	 *
	 * L'attore arriva come parametro per la stessa ragione per cui ce l'ha `AppendLogEntry`: dalla voce non si
	 * deduce, e dedurlo dall'indice della simulazione legherebbe la traccia a una corrispondenza
	 * (`StableUnitId == FRTHexSimUnit::UnitId + 1`) che nessuno ha dichiarato.
	 */
	// `LogPhase` e non `Phase`: il manager ha un membro con quel nome, e ombreggiarlo e' un warning trattato
	// come errore.
	void RecordFacingChange(FRTHexSimUnit& Unit, ERTHexDirection NewFacing, ERTFacingOutcome Reason,
		ERTMatchPhase LogPhase, const ARTUnit* Actor);

	/**
	 * Valida il piano di ogni unita' viva al COMMIT, e registra nel COMBAT LOG quello che non torna (CP 38.2).
	 *
	 * Il lock-in e' l'ultimo istante in cui un piano e' ancora un piano: dopo, e' una risoluzione. E' qui
	 * che la DoD chiede *«un punto solo che risponde LEGALE / ILLEGALE + reason code prima del commit»*,
	 * e questo e' il commit.
	 *
	 * 🔴 **Registra e non BLOCCA, ed e' una scelta con una ragione misurata.** Un rifiuto al momento del
	 * click era la prima versione, ed e' stata ritirata in code review: un piano illegale nasce quasi sempre
	 * da uno scatto piu' un movimento, e uno scatto pianificato **non e' annullabile** — `ERTPointerBackStep`
	 * elenca waypoint, targeting, inspector e focus, non il dash, e gli unici a togliere
	 * `PlannedDashAbility` sono il resolver e il ri-pianificatore del bot. Rifiutare l'input avrebbe chiuso
	 * il giocatore in un turno senza uscita: non poteva ne' muoversi ne' disfare. Un piano incoerente che si
	 * risolve come il resolver decide e' meno grave di un turno che non si puo' correggere.
	 *
	 * ⚠️ Cio' che cambia rispetto a prima non e' l'esito del turno, e' la sua **osservabilita'**: quello
	 * che il resolver assorbiva in silenzio ora lascia una voce con il motivo. Il giorno in cui il dash sara'
	 * annullabile, questa funzione e' il punto da cui far partire un rifiuto vero.
	 */
	void ValidatePlansAtLockIn();

	/** Applica uno status e registra la voce se cosi' facendo ha SPENTO un `Burning` (#1314). */
	void ApplyStatusLogged(ARTUnit* Unit, FGameplayTag Tag, int32 Turns);

	/** L'invariante dei pesi si verifica una volta per partita, sull'istanza viva (#1276). */
	bool bBotWeightInvariantChecked = false;

	/**
	 * Le due voci di ciclo di vita di uno stato (#1077), costruite in UN posto solo.
	 *
	 * 🔴 **Erano copiate in cinque siti, e i cinque sono derivati esattamente nei campi che non
	 * condividevano** — la fase, la cella e la regola del sentinella. Trovato in code review, ed e' la
	 * causa comune di tre rilievi distinti: un helper qui non e' eleganza, e' il punto in cui quelle regole
	 * stanno scritte una volta.
	 *
	 * ⚠️ **Statiche e dichiarate qui** perche' servono anche a `RTTurnManager_Blast.cpp`, che e' un'altra
	 * unita' di traduzione dello stesso `ARTTurnManager`: un helper in un namespace anonimo del `.cpp` non
	 * ci arriverebbe, ed e' il motivo per cui una delle nascite era rimasta senza voce.
	 */
	static FRTTurnLogEntry MakeStatusBirthEntry(ERTMatchPhase InPhase, FGameplayTag Tag, const FRTCellId& Cell,
		int32 RequestedTurns, bool bFromTerrain);
	static FRTTurnLogEntry MakeStatusDeathEntry(FGameplayTag Tag, const FRTCellId& Cell,
		ERTStatusOutcome Outcome);

	/**
	 * La voce di uno stato ISTANTANEO: `Status.Electrified`, che e' l'etichetta di un evento e non uno
	 * stato che dura (`#1324`, [D-315]).
	 *
	 * 🔴 **Terza funzione e non un terzo ramo di `MakeStatusBirthEntry`, deliberatamente.** Quella mappa una
	 * DURATA su un esito, e la sua sentinella e' `PersistentWhileOnCell`. Inferire l'istantaneita' da
	 * `RequestedTurns == 0` la renderebbe raggiungibile da **tutti e cinque** i suoi chiamanti: chiunque
	 * passasse una durata nulla per un'altra ragione scriverebbe `AppliedInstantly` senza averlo chiesto, e
	 * `0` e' un valore che si ottiene per errore molto piu' facilmente di `-1`. Qui l'istantaneita' e' nel
	 * NOME della funzione, quindi non ci si finisce per sbaglio.
	 *
	 * ⚠️ Non nasce e non muore: nessuna `MakeStatusDeathEntry` seguira' mai una di queste voci.
	 */
	static FRTTurnLogEntry MakeStatusInstantEntry(ERTMatchPhase InPhase, FGameplayTag Tag,
		const FRTCellId& Cell);

	/** Revisione del grafo di mappa ADESSO: sale durante la risoluzione, quindi si legge a ogni emissione. */
	int32 CurrentGraphRevision() const;

	/**
	 * Assegna a ogni unita' la sua identita' STABILE di partita, una volta sola (#405, [D-063]).
	 *
	 * Il progetto conosceva l'unita' solo come indice in `MakeCurrentSnapshot`: quello filtra i vivi e si
	 * ricostruisce a ogni fase, quindi scala appena qualcuno muore — e `DestroyDefeatedUnits` distrugge pure
	 * l'Actor, cosi' nemmeno il pointer sopravvive. Per una traccia che si rilegge a partita finita serve un
	 * intero che non si muova: `ARTUnit::StableUnitId`, assegnato qui e mai piu' toccato.
	 *
	 * Idempotente: la seconda chiamata non fa niente. Un roster VUOTO non conta come costruito — congelarlo
	 * darebbe identita' a nessuno e la negherebbe a chi arriva dopo.
	 */
	void EnsureMatchRoster();

	/** Il roster di partita e' stato costruito: l'identita' delle unita' non si riassegna piu'. */
	bool bMatchRosterBuilt = false;

	/**
	 * Il roster congelato, in ordine di `StableUnitId`: l'indice `i` porta l'unita' di id `i + 1`.
	 *
	 * 🔑 **Non e' una seconda sorgente di identita'**: l'identita' resta `ARTUnit::StableUnitId`, e questo
	 * array e' solo l'indice inverso — l'unico modo di tornare all'Actor partendo da un fatto che ne porta
	 * il solo id. Lo riempie `EnsureMatchRoster` con la lista che gia' ordinava e poi buttava via, quindi
	 * **non costa un secondo `GetAllActorsOfClass`**: era gia' li'.
	 *
	 * ⚠️ **Weak, e deve restarlo**: `DestroyDefeatedUnits` distrugge gli Actor delle unita' eliminate mentre
	 * il roster resta congelato per tutta la partita. Una entry scaduta e' la risposta giusta — «quell'unita'
	 * non c'e' piu'» — non un buco da riparare.
	 */
	TArray<TWeakObjectPtr<ARTUnit>> MatchRoster;

	/**
	 * Da `StableUnitId` all'Actor, o `nullptr`.
	 *
	 * 🔴 **E' la porta del confine simulazione -> presentazione** (#1800): i fatti risolti
	 * (`FRTResolvedEvent`) portano id, e solo chi deve davvero animare passa di qui. Chiamarla per
	 * *decidere* qualcosa rimetterebbe la lifetime di un Actor dentro un esito, che e' esattamente il
	 * difetto che gli id tolgono.
	 *
	 * Risponde `nullptr` per tre cause, e tutte e tre significano «nessun Actor da animare»: id `0`
	 * ([D-063], nessuna unita' dichiarata), id fuori dal roster congelato (unita' spawnata dopo), unita'
	 * gia' distrutta. E' lo stesso `nullptr` che rispondeva `TWeakObjectPtr::Get()` prima di questa fetta.
	 */
	ARTUnit* UnitByStableId(int32 StableUnitId) const;

	/**
	 * Dice le sovrapposizioni che lo snapshot ha registrato, **una volta per turno ciascuna** (`#1970`).
	 *
	 * 🔴 **La segnalazione sta QUI e non in `MakeSnapshot`**, che e' una funzione pura e non sa se sta
	 * servendo una risoluzione autoritativa o l'anteprima sotto il cursore: `ARTPlayerController` la chiama
	 * a ogni interazione di pianificazione, e un log la' dentro produrrebbe centinaia di righe identiche al
	 * secondo per un difetto solo. Rilevare e segnalare sono due mestieri; questo e' il secondo.
	 *
	 * ⚠️ La deduplica e' una LISTA e non un set con hash: nel caso normale e' vuota, quindi il confronto
	 * lineare non costa niente, e non c'e' una collisione che possa far sparire in silenzio proprio il log
	 * che questa funzione esiste per emettere.
	 */
	void ReportSnapshotOverlaps(const FRTHexSnapshot& Snapshot);

	/**
	 * Le sovrapposizioni gia' segnalate nel turno corrente. Azzerata quando il turno avanza: la stessa
	 * condizione che sopravvive a due turni va detta due volte — e' un fatto nuovo ogni turno.
	 */
	TArray<FRTHexOverlap> ReportedOverlapsThisTurn;

public:
	/**
	 * `StableUnitId` -> nome leggibile, per le righe del combat log derivate dal TurnLog (`#1932`).
	 *
	 * 🔴 **Esiste per non avere due produttori del testo.** Le righe che il giocatore legge nascono da
	 * `URTTurnLogLibrary::DescribeTurnLogWithSubjects`, a cui questa mappa viene passata: chi vuole
	 * RIDERIVARE le stesse righe — un test che confronta cio' che e' stato emesso con cio' che il TurnLog
	 * dice — deve poter usare la stessa risoluzione, altrimenti confronta `Gadget: resta` con `u3: resta` e
	 * fallisce su una differenza che non e' un difetto. E' `public` per questo: e' il modo di verificare che
	 * di produttori ce ne sia uno solo.
	 *
	 * Legge `MatchRoster`, non le sole unita' vive: una voce puo' riguardare chi e' stato eliminato nel turno
	 * appena chiuso, e `DestroyDefeatedUnits` passa dopo. Una entry scaduta non contribuisce, e la riga esce
	 * con `u<id>`.
	 */
	TMap<int32, FString> SubjectNamesForLog() const;

	/**
	 * L'indice nel roster congelato per un `StableUnitId`, o `INDEX_NONE`.
	 *
	 * E' la **meta' inversa** di `EnsureMatchRoster`, che assegna `Roster[i]->StableUnitId = i + 1`. Le due
	 * stanno vicine apposta: sono una convenzione sola, e separarle e' il modo in cui un `+ 1` diventa un
	 * `off-by-one` che nessuno rilegge.
	 *
	 * `static` e senza stato **per poterla provare senza un mondo** (#1818): le due regole che porta — lo
	 * `0` non e' un id ([D-063]) e un id oltre il roster e' di un'unita' arrivata dopo il congelamento —
	 * erano verificabili solo spawnando degli Actor.
	 */
	static int32 RosterIndexForStableId(int32 StableUnitId, int32 RosterNum);

protected:

	FTimerHandle PlanningTimerHandle;

	/**
	 * L'allestimento ha rivendicato l'apertura del turno 1 (`#2102`, [D-314]).
	 *
	 * ⚠️ **Stato di AVVIO, non stato di partita**: non entra in snapshot, TurnLog o hash. Dice chi apre il
	 * primo turno, non cosa succede dentro.
	 */
	bool bFirstTurnClaimedBySetup = false;

	/** Il turno 1 e' stato aperto — da `BeginPlay` o dall'allestimento. Rende idempotente l'apertura. */
	bool bFirstTurnOpened = false;

protected:
	/**
	 * Impatto di una carica: chi ha caricato, chi ha urtato e con quale definizione d'azione.
	 *
	 * Il movimento della carica risolve nella fase Dash, ma il colpo NO: il catalogo le assegna il codice
	 * 20/30, e il 30 (controllo) risolve per priorita' dentro il Blast. L'impatto aspetta qui in mezzo.
	 */
	struct FRTChargeImpact
	{
		TWeakObjectPtr<ARTUnit> Attacker;
		TWeakObjectPtr<ARTUnit> Target;
		FRTActionDef Def;
	};

	/** Impatti raccolti nella fase Dash del turno corrente, consumati dal Blast. */
	TArray<FRTChargeImpact> PendingChargeImpacts;

	/**
	 * Unita' a cui, in QUESTO turno, un'altra azione gia' risolta ha negato la reazione (`Action.Sprint`:
	 * `FRTActionDef::bAllowsReaction = false`). Popolato da `ResolveDash` (l'unica fase, oggi, con un'azione
	 * del genere) e riletto da `ResolveCombat` — la reazione risolve nel Blast, DOPO il Dash, quindi il divieto
	 * deve sopravvivere al cambio di fase. Svuotato a inizio `ResolveDash`: e' sempre fresco per il turno.
	 */
	TSet<TWeakObjectPtr<ARTUnit>> ReactionBlockedThisTurn;

private:
	/** Animazione di movimento di una singola unita': waypoint gia' convertiti in mondo + fase (Dash/Move). */
	struct FRTMoveAnim
	{
		TWeakObjectPtr<ARTUnit> Unit;
		TArray<FVector> World; // start + celle attraversate, in coordinate mondo
		ERTMatchPhase Phase = ERTMatchPhase::Move; // fase in cui va riprodotta (Dash o Move)
	};

	/** Eventi risolti nel turno corrente (movimenti, attacchi) da riprodurre. */
	TArray<FRTResolvedEvent> ResolvedTimeline;

	// Stato runtime del playback.
	bool bIsResolving = false;
	bool bPrepActiveThisTurn = false;

	/** Esito + via calcolati nel Cleanup e consumati da ConcludeTurn (che chiude o apre il round dopo). */
	FRTMatchResult PendingResult;

	/**
	 * Lo stato su cui il verdetto è stato dato, conservato accanto ad esso.
	 *
	 * ⚠️ Serve perché `OnMatchEnded` lo annuncia da `ConcludeTurn`, mentre il calcolo avviene nel Cleanup:
	 * senza, il conteggio dei round andrebbe ricostruito al momento dell'annuncio, e sarebbe un secondo
	 * calcolo che può divergere da quello che ha deciso l'esito.
	 */
	FRTMatchState PendingState;

	/** Regole di formato in vigore. Default: nessun limite di round, nessuna soglia (solo eliminazione). */
	FRTMatchRules MatchRules;

	/** Progresso obiettivo per squadra. Alimentato da AddTeamScore, letto dalla regola di fine partita. */
	int32 Team0Score = 0;
	int32 Team1Score = 0;

	/**
	 * La memoria per unita' del termine di ingaggio (#1300, D-185), per `StableUnitId`.
	 *
	 * `BotIdleTurns` conta da quanti turni consecutivi il piano scelto per quell'unita' non contiene un
	 * attacco; `BotIdleRound` ricorda in quale round il conteggio e' gia' stato aggiornato.
	 *
	 * ⚠️ **La guardia sul round serve davvero**: `PlanBotsForTest()` seguito da `LockInAndResolve()`
	 * pianifica **due volte lo stesso round**, e senza di lei il contatore correrebbe al doppio della
	 * velocita' nei test che usano il primo e non negli altri — cioe' il decadimento misurerebbe una
	 * partita diversa a seconda di chi la guarda.
	 *
	 * ⚠️ **Vive qui e non in una `static`**: l'automation gira molte partite nello stesso processo, e uno
	 * stato statico le farebbe ereditare la memoria l'una dall'altra. Vive qui e non su `ARTUnit` perche'
	 * e' bookkeeping del bot, come il kiting: un'unita' che muove il giocatore non lo consulta mai.
	 * ⚠️ **Non se ne itera mai l'ordine** (solo `FindRef`/`FindOrAdd`): l'invariante «niente dipendenza
	 * dall'ordine di `TMap`» resta intatta.
	 */
	TMap<int32, int32> BotIdleTurns;
	TMap<int32, int32> BotIdleRound;

	TArray<FRTMoveAnim> MoveAnims;          // derivati dagli eventi Move
	TArray<FRTResolvedEvent> PlaybackAttacks; // eventi Attack, mostrati in serie nel Blast
	TArray<FRTResolvedEvent> PlaybackDefeated; // eventi Defeated, mostrati a fine della loro fase
	TArray<ERTMatchPhase> PlaybackPhases;   // fasi attive, in ordine
	int32 PlaybackPhaseIdx = 0;
	float PlaybackPhaseElapsed = 0.f;
	float PlaybackSlackScale = 1.f;         // quanto il budget comprime le ATTESE (1 = nessuna, 0 = tutto)
	float PlaybackTotalSeconds = 0.f;       // durata stimata (per la progress bar)
	float PlaybackElapsedTotal = 0.f;
	int32 AttacksShown = 0;                 // colpi gia' rivelati nel Blast corrente

	// Trasformazione griglia in cache per convertire celle->mondo durante il playback.
	FVector PBOrigin = FVector::ZeroVector;
	float PBCellSize = 200.f;
	float PBLayerHeight = 0.f;
};
