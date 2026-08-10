#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "Ability/RTActionDef.h" // FRTActionDef: l'impatto della carica porta con se' la definizione
#include "Turn/RTHexSim.h" // FRTHexSnapshot: restituito per valore da MakeCurrentSnapshot
#include "Turn/RTPacing.h" // FRTPacingSample: telemetria, canale separato dal TurnLog
#include "Map/RTHexCellData.h" // ERTHexSurface: il terreno dinamico ricorda la superficie originale (CP 8.4)
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

// Delegate per la presentazione in Blueprint (camera/VFX/SFX): il playback riproduce eventi gia' risolti.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTPhasePlaybackSignature, ERTMatchPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTUnitPlaybackSignature, ARTUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTAttackPlaybackSignature, ARTUnit*, Source, ARTUnit*, Target, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTPlaybackFinishedSignature);

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

	/** Ultimi eventi (combat log) per la HUD, dal piu' vecchio al piu' recente. */
	const TArray<FString>& GetRecentEvents() const { return RecentEvents; }

	/** Esiti autoritativi dell'ultimo turno risolto (Movimento + Combat), ordinati deterministicamente. */
	const TArray<FRTTurnLogEntry>& GetTurnLog() const { return TurnLog; }

	/** Rotte effettivamente percorse nell'ultima risoluzione (viz post-lock del percorso eseguito). */
	const TArray<TArray<FRTCellId>>& GetLastMoveRoutes() const { return LastMoveRoutes; }

	// --- Sonda di pacing (TELEMETRIA: nessun ritorno verso il gameplay) --------------------------
	/**
	 * Registra un input di pianificazione. Chiamata dal PlayerController, che NON cronometra: tutto il
	 * tempo vive qui, in un posto solo. Ignorata fuori dalla fase di pianificazione.
	 */
	void RecordPlanningInput(ERTPlanningInput Kind);

	/** Campioni di pacing della sessione corrente (sola lettura; telemetria, non stato di gioco). */
	const TArray<FRTPacingSample>& GetPacingSamples() const { return PacingSamples; }

	/** Se vero, ogni turno appende una riga in Saved/RT/pacing_<sessione>.csv. L'accumulo in memoria e' sempre attivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	bool bRecordPacing = false;

	/** Squadra il cui spazio di decisione si misura in ActionsAvailable (il giocatore umano e' il team 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	int32 PacingTeamId = 0;

	// --- Presentazione (Blueprint) -------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTPhasePlaybackSignature OnPhasePlaybackStarted;

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

	/** Velocita' di scorrimento dei cilindri nel Move (celle al secondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PlaybackCellsPerSecond = 6.5f;

	/** Pausa tra una fase e la successiva (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PhaseBeatSeconds = 0.30f;

	/** Durata di visualizzazione di ogni colpo nel Blast (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float AttackShowSeconds = 0.50f;

	/** Tetto di durata del playback: oltre, si accelera automaticamente (0 = nessun tetto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float MaxPlaybackSeconds = 12.f;

	// --- Tuning del bot (utility scoring, editabile in editor senza ricompilare) -----------------
	// Pesi interi iniettati nel FRTBotContext di PlanBots (invariante #4: niente float). I default
	// coincidono con quelli della struct: a parita' di valori il comportamento e' invariato.
	/** Bonus se l'attacco pianificato UCCIDE il bersaglio: domina la scelta (focus-fire letale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKill = 10000;

	/** Peso del danno inflitto dall'attacco (focus-fire: a parita' d'altro, piu' danno = meglio). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WDamage = 10;

	/** Penalita' per ogni nemico che puo' colpire la cella scelta (evita di esporsi al tiro). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WThreat = 100;

	/** Penalita' (kiter) proporzionale a quanto si sta SOTTO la distanza di sicurezza (standoff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKiteViolation = 50;

	/** Penalita' (mischia) proporzionale alla distanza dal nemico: chiudere la distanza e' meglio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WApproach = 10;

	/** Bonus per la quota (Layer) della cella: premia l'alta quota (tiro oltre coperture basse, +danno). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WElevation = 20;

	/**
	 * Snapshot dello stato corrente della partita (unita' VIVE del livello + mappa autorevole).
	 * `OutUnits[i]` e' l'unita' con `UnitId == i`: e' la chiave con cui rileggere gli esiti.
	 *
	 * Il TurnManager e' l'autorita' (invariante #5): il controller del giocatore chiede QUESTO snapshot per
	 * calcolare le sue anteprime, invece di ricostruirsi uno stato parallelo che potrebbe divergere.
	 */
	FRTHexSnapshot MakeCurrentSnapshot(TArray<ARTUnit*>& OutUnits) const;

protected:
	virtual void BeginPlay() override;

	void PlanBots();
	void ResolvePrep();
	void ResolveDash();
	void ResolveCombat();
	void ResolveMovement();

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
	 * Colpi predittivi armati nella Prep di QUESTO turno, consumati al boundary del Move (E18).
	 *
	 * Vive sul TurnManager e non sull'unita' perche' la Prep azzera `PlannedAbilityIndex` appena consuma
	 * l'abilita': dopo quel punto il piano non esiste piu', e la previsione dev'essere gia' stata catturata.
	 * Si svuota a ogni Move, quindi un colpo non sopravvive al proprio turno.
	 */
	UPROPERTY(Transient)
	TArray<FRTArmedPrediction> ArmedPredictions;

	/**
	 * Risolve i colpi predittivi armati contro le rotte appena calcolate, e TRONCA il movimento di chi viene
	 * colto (E18 CP 18.2). Modifica `Resolved` in luogo: chiamata prima che il TurnLog sia costruito.
	 *
	 * La decisione sta nello strato PURO (`URTPredictiveLibrary`); qui restano solo le tre cose che il mondo
	 * possiede — chi e' ostile a chi, il danno sulle unita' vere, e le voci di log. Tenerle separate e' cio'
	 * che permette a `Predictive.PermutationInvariant` di esistere come test senza un `UWorld`.
	 */
	void ResolvePredictiveBoundary(const TArray<ARTUnit*>& Units, TArray<FRTHexMoveResult>& Resolved);

	/**
	 * Applica le cure raccolte da `ResolveCombat` (CP 8.5). Chiamata da DUE punti — dopo i danni, e nel ramo
	 * "nessun colpo in questo turno" — perche' una cura fuori da uno scontro e' il caso normale di un
	 * supporto, non un'eccezione: con un solo call site la cura sparirebbe in silenzio quando nessuno attacca.
	 */
	void ApplyPlannedHeals(const TArray<ARTUnit*>& Targets, const TArray<int32>& Amounts,
		const TArray<FRTCellId>& Sources);

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
		const FName& CauseActionId);

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
	float DurationForPlaybackPhase(ERTMatchPhase InPhase) const;

	// --- Sonda di pacing ------------------------------------------------------------------------
	void BeginPacingSample();
	void ClosePacingSample();
	void AppendPacingRow(const FRTPacingSample& Sample);

	TArray<FRTPacingSample> PacingSamples;
	FRTPacingSample PacingCurrent;
	double PacingPlanningStart = 0.0;  // FPlatformTime::Seconds() all'apertura della pianificazione
	double PacingLastInput = 0.0;
	bool bPacingHadInput = false;
	FString PacingFilePath;            // vuoto finche' non si scrive la prima riga

	/** Registra un evento: lo scrive nel log LogRT e lo accoda al combat log della HUD. */
	void AddLogEvent(const FString& Message);

	/**
	 * Applica gli OnEnterEffects (URTTerrainLibrary) di ogni cella in Entered a Unit: Damage via
	 * URTCombatLibrary::ApplyDamage, Status via Unit->ApplyStatus. Usata da ResolveDash e ResolveMovement
	 * sulle celle FRTHexMoveResult::Entered di ciascuna unita' (CP 8.1).
	 */
	void ApplyTerrainOnEnterEffects(const URTHexMapAsset* Map, ARTUnit* Unit, const TArray<FRTCellId>& Entered);

	/** Le celle ENTRATE lungo un percorso: tutte tranne la partenza, dove l'unita' stava gia'. */
	static TArray<FRTCellId> CellsEnteredAlong(const TArray<FRTCellId>& Path);

	UPROPERTY()
	TArray<FString> RecentEvents;

	/** TurnLog dell'ultimo turno risolto (osservabilita' autoritativa; ordinato in LockInAndResolve). */
	TArray<FRTTurnLogEntry> TurnLog;

	/** Rotte (celle) percorse da ogni unita' che si e' mossa nell'ultima risoluzione. */
	TArray<TArray<FRTCellId>> LastMoveRoutes;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Turn")
	int32 MaxLogLines = 6;

	/** Durata della fase di pianificazione; allo scadere scatta il lock-in automatico. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	float PlanningSeconds = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 TurnNumber = 1;

	FTimerHandle PlanningTimerHandle;

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

	/** Regole di formato in vigore. Default: nessun limite di round, nessuna soglia (solo eliminazione). */
	FRTMatchRules MatchRules;

	/** Progresso obiettivo per squadra. Alimentato da AddTeamScore, letto dalla regola di fine partita. */
	int32 Team0Score = 0;
	int32 Team1Score = 0;

	TArray<FRTMoveAnim> MoveAnims;          // derivati dagli eventi Move
	TArray<FRTResolvedEvent> PlaybackAttacks; // eventi Attack, mostrati in serie nel Blast
	TArray<FRTResolvedEvent> PlaybackDefeated; // eventi Defeated, mostrati a fine della loro fase
	TArray<ERTMatchPhase> PlaybackPhases;   // fasi attive, in ordine
	int32 PlaybackPhaseIdx = 0;
	float PlaybackPhaseElapsed = 0.f;
	float PlaybackSpeed = 1.f;              // fattore di accelerazione per rientrare nel tetto
	float PlaybackTotalSeconds = 0.f;       // durata stimata (per la progress bar)
	float PlaybackElapsedTotal = 0.f;
	int32 AttacksShown = 0;                 // colpi gia' rivelati nel Blast corrente

	// Trasformazione griglia in cache per convertire celle->mondo durante il playback.
	FVector PBOrigin = FVector::ZeroVector;
	float PBCellSize = 200.f;
	float PBLayerHeight = 0.f;
};
