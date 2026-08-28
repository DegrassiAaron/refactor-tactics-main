#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeView: il combat log filtra da qui, non da un secondo canale
#include "Bot/RTHexBotLibrary.h" // i pesi del bot hanno UNA sorgente: i default della struct
#include "Turn/RTTurnRules.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "Replay/RTReplayManifest.h"
#include "Ability/RTActionDef.h" // FRTActionDef: l'impatto della carica porta con se' la definizione
#include "Turn/RTHexSim.h" // FRTHexSnapshot: restituito per valore da MakeCurrentSnapshot
#include "Turn/RTPacing.h" // FRTPacingSample: telemetria, canale separato dal TurnLog
#include "Map/RTHexCellData.h" // ERTHexSurface: il terreno dinamico ricorda la superficie originale (CP 8.4)
#include "Combat/RTCombatResolver.h" // FRTAttack, FRTUnitCombatState: il pass reazioni raccoglie i primi e aggiorna i secondi
#include "Combat/RTHexCombatLibrary.h" // FRTHexAttackHit/FRTHexAttackIntent: cio' su cui il pass reazioni valuta i trigger
#include "Turn/RTReactionLibrary.h" // ERTReactionPassPoint/FRTReactionTriggerHit: la firma del pass reazioni li usa
#include "Turn/RTDeclaredCondition.h" // FRTDeclaredCondition: l'Overwatch armato porta con se' la condizione dichiarata
#include "Turn/RTReactionOpportunityTypes.h" // FRTReactionOpportunity/FRTReactionDecision: le firme del boundary li usano
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
 * La partita è finita, con il verdetto e lo stato che lo motiva (CP 46.5, `#940`).
 *
 * ⚠️ **È un annuncio, non un comando**, ed è la ragione per cui esiste invece di far chiamare al
 * `TurnManager` la schermata di fine partita: la simulazione non deve conoscere il frontend. Chi ascolta
 * decide cosa farne — `ARTGameMode` apre il Result, uno scenario headless non ascolta e non cambia nulla.
 * È la stessa forma di `URTFrontendNavigator::OnMatchRequested`: chi sa annuncia, chi può agire consuma.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTOnMatchEndedSignature, const FRTMatchResult&, Result, const FRTMatchState&, State);

/**
 * Una riga di combat log, col SOGGETTO accanto al testo.
 *
 * Il soggetto e' `ARTUnit::StableUnitId` — l'identita' che attraversa fasi e turni — oppure `INDEX_NONE`
 * per le righe che parlano del MONDO e non di un'unita' («Turno 3 - pianificazione», una superficie che
 * scade). Senza questo campo il filtro dovrebbe cercare coordinate dentro una stringa gia' formattata.
 */
/**
 * Di CHI parla una riga di log: un'unita', oppure il mondo. Mai «non ci ho pensato».
 *
 * 🔴 **E' un tipo e non un `int32` perche' il gate diventi il compilatore** (`#1499`). Il vecchio parametro
 * aveva un default `INDEX_NONE` fail-open: un sito nuovo che nominava un nemico passava il filtro di
 * conoscenza per omissione, e l'omissione non fa rumore. Ora non esiste conversione implicita da `int32`:
 * `AddLogEvent(Testo, INDEX_NONE)` non compila, e un sito senza soggetto deve **dichiararsi** `World()`.
 *
 * ⚠️ **`World()` non e' il vecchio default con un altro nome.** Dice «questa riga riguarda tutti» — una
 * superficie che scade, il marker di turno, la fine partita — ed e' una scelta che si legge. Il default
 * diceva soltanto che nessuno aveva deciso.
 */
struct FRTLogSubject
{
	/** Un'unita' viva, con tutto cio' che serve a congelarne il verdetto ([D-223]). */
	static FRTLogSubject Unit(const ARTUnit* InUnit);

	/**
	 * Un soggetto che porta GIA' la propria risposta, congelata altrove e prima.
	 *
	 * 🔴 **E' la forma del canale derivato dal TurnLog**, ed esiste perche' a fine turno il verdetto non e'
	 * piu' calcolabile correttamente: la conoscenza disponibile e' quella del Blast, le celle sono gia'
	 * post-Move, e `AwarenessOfUnit` confronta proprio quei due. La voce lo ha calcolato quando e' nata
	 * (`AppendLogEntry`), e qui si trasporta.
	 *
	 * ⚠️ **Non esiste una forma che prenda il solo `StableUnitId`**, ed e' deliberato: da un id soltanto il
	 * verdetto non si calcola — `ClassifyTarget` vuole anche squadra e cella — e una forma del genere
	 * inviterebbe a produrre righe fail-closed senza accorgersene.
	 */
	static FRTLogSubject Frozen(int32 InStableUnitId, const FRTKnowledgeVerdict& InVerdict);

	/** Un fatto che riguarda tutti: nessun soggetto da conoscere, nessuna ragione per nasconderlo. */
	static FRTLogSubject World();

	bool IsWorld() const { return bWorld; }
	int32 GetStableUnitId() const { return StableUnitId; }
	const ARTUnit* GetUnit() const { return Unit_; }

	/** Vero se il verdetto viaggia col soggetto: chi lo consuma non deve ricalcolarlo. */
	bool HasFrozenVerdict() const { return bFrozen; }
	const FRTKnowledgeVerdict& GetFrozenVerdict() const { return FrozenVerdict; }

private:
	FRTLogSubject() = default;

	bool bWorld = false;
	bool bFrozen = false;
	int32 StableUnitId = INDEX_NONE;
	const ARTUnit* Unit_ = nullptr;
	FRTKnowledgeVerdict FrozenVerdict;
};

USTRUCT()
struct FRTCombatLogLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Text;

	/** Chi ha prodotto il fatto. Resta per diagnosi e per i test: il filtro NON lo usa piu'. */
	UPROPERTY()
	int32 SubjectStableUnitId = INDEX_NONE;

	/**
	 * Chi puo' leggere questa riga, deciso quando la riga e' nata ([D-223]).
	 *
	 * 🔴 **Il default nasconde**: una riga che arrivasse qui senza verdetto non si legge. E' l'opposto del
	 * default che `#1499` ha rimosso, ed e' la direzione giusta per un filtro di privacy — si perde una
	 * riga, non si regala una posizione.
	 */
	UPROPERTY()
	FRTKnowledgeVerdict Verdict;
};

/**
 * Una rotta percorsa nell'ultima risoluzione, col SOGGETTO accanto alle celle.
 *
 * Stessa forma di `FRTCombatLogLine` e per la stessa ragione: un dato destinato alla presentazione porta
 * l'identita' di CHI lo ha prodotto, perche' a valle non c'e' modo di ricostruirla.
 *
 * 🔴 **L'indice dell'array non e' mai stato un'identita', e non puo' diventarlo** (`#1497`): la raccolta e'
 * COMPATTATA — aggiunge una voce solo per chi si e' davvero mosso — quindi con le unita' 1 e 3 in movimento
 * e la 2 ferma le due rotte stanno agli indici `0` e `1`. Una stesura precedente di `rt.Debug.DrawPaths`
 * stampava «unita %d» su quell'indice e nominava un'unita' che non si era mossa.
 *
 * Senza questo campo la traccia non e' filtrabile contro la conoscenza di squadra, e il percorso di un
 * nemico che non si vede resta disegnato a schermo.
 */
USTRUCT()
struct FRTMoveRoute
{
	GENERATED_BODY()

	/** Chi ha percorso questa rotta. `ARTUnit::StableUnitId`, mai un indice di array. */
	UPROPERTY()
	int32 StableUnitId = INDEX_NONE;

	/** Cella di partenza seguita dalle celle attraversate, nell'ordine in cui sono state percorse. */
	UPROPERTY()
	TArray<FRTCellId> Cells;

	/**
	 * Chi puo' vedere disegnata CIASCUNA cella di `Cells`, deciso quando la rotta e' stata raccolta
	 * ([D-223], emendamento del 2026-08-28). Parallelo a `Cells`, stesso indice.
	 *
	 * 🔴 **Un verdetto per cella e non uno per rotta, perche' una rotta non e' un fatto puntuale.** Una riga
	 * di log ha un istante; una rotta e' una traiettoria che ne attraversa due — partenza e arrivo — e un
	 * verdetto congelato sulla partenza autorizzerebbe a disegnare l'arrivo. Sono i due errori speculari che
	 * [D-223] esiste per chiudere: il **leak** (la polilinea entra nella nebbia) e la **contraddizione** (la
	 * traccia nascosta mentre il modello e' disegnato).
	 *
	 * ⚠️ **E il caso piu' comune non e' agli estremi**: `VisibleCells` e' un insieme *bucato* — LOS, cono
	 * frontale e close range — non un raggio, quindi il mezzo di un percorso puo' sparire mentre partenza e
	 * arrivo si vedono. Un verdetto per rotta non lo copre in nessuna delle sue forme.
	 *
	 * 🔴 **Non si consuma leggendo questo array**: la regola vive in `VisibleTrailFor`, che tronca. Chi
	 * ciclasse qui saltando le celle non ammesse disegnerebbe un segmento fra due celle non adiacenti, cioe'
	 * una linea che attraversa proprio il tratto da nascondere.
	 */
	UPROPERTY()
	TArray<FRTKnowledgeVerdict> CellVerdicts;
};

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

	/**
	 * Le righe che un osservatore puo' leggere. Statica e PURA: la si interroga senza montare una partita.
	 *
	 * 🔴 Una riga il cui soggetto e' ignoto **sparisce intera**, non viene oscurata: una riga oscurata
	 * dice comunque che qualcosa e' successo, e quando e' successo.
	 * L'ORDINE di produzione si conserva: un combat log riordinato non e' un log.
	 *
	 * 🔴 **Ignoto significa «non visto ORA»**, non «senza voce»: un soggetto `Remembered` ha una voce, ma
	 * le coordinate stampate nella riga sono quelle attuali, cioe' cio' che la squadra ha smesso di sapere.
	 * Stessa regola di `ARTHUD::ShouldDrawUnitOverlay`, e per la stessa ragione.
	 */
	static TArray<FString> ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines, int32 ObserverTeamId);

	/**
	 * Il tratto di una rotta che un osservatore puo' vedere disegnato: il PREFISSO che il verdetto ammette.
	 *
	 * E' il gemello di `ComposeVisibleLogLines` per il secondo canale che [D-223] congela, ed e' statica e
	 * PURA per la stessa ragione di `ARTHUD::ShouldDrawUnitOverlay`: `DrawHUD` non ha copertura headless,
	 * quindi la regola vive dove la si puo' interrogare senza montare un HUD ne' una partita.
	 *
	 * 🔴 **Tronca, non salta.** La rotta porta il tratto OSSERVATO e si interrompe dove l'osservatore ha
	 * perso il soggetto: *«ho visto questa parte del suo movimento»* e' l'unica frase vera in ogni caso.
	 * Saltare le celle non ammesse e riprendere piu' avanti disegnerebbe un segmento fra due celle non
	 * adiacenti — una linea tesa attraverso il tratto che si voleva nascondere, cioe' il leak in forma
	 * peggiore.
	 *
	 * ⚠️ **Fail-closed due volte**: un verdetto assente non ammette nessuno (`FRTKnowledgeVerdict` nasce a
	 * maschera vuota), e una rotta i cui verdetti non siano allineati alle celle non si disegna affatto —
	 * senza quel controllo, un `Cells.Add` futuro senza il verdetto corrispondente leggerebbe fuori array.
	 *
	 * ⚠️ Un tratto di UNA cella non produce nessun segmento a schermo, ed e' corretto: la cella di partenza
	 * era gia' osservata: disegnarne il punto non aggiunge nulla che l'osservatore non sapesse.
	 */
	static TArray<FRTCellId> VisibleTrailFor(const FRTMoveRoute& Route, int32 ObserverTeamId);

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

	/** Identita' della registrazione in corso. Non valida finche' `BeginReplayRecording` non e' stata chiamata. */
	FGuid GetReplayMatchId() const { return ReplayManifest.MatchId; }

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

	/**
	 * Velocita' SCELTA da chi guarda: 1 / 2 / 4 (CP 47.2, #955). E' una preferenza di ritmo, non un tetto:
	 * si compone con l'accelerazione automatica via `URTPlaybackLibrary::EffectivePlaybackSpeed`, che
	 * prende il massimo dei due. Scriverla a risoluzione in corso vale dal tick successivo — `TickPlayback`
	 * la rilegge a ogni tick e non la congela in `BeginPlayback`.
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
	 * Il fattore di accelerazione AUTOMATICA in vigore: quanto il tetto sta gia' comprimendo il round.
	 *
	 * Esiste per un motivo solo, ed e' il criterio 2 di CP 47.7 (#1015): l'etichetta del controllo di
	 * velocita' deve dire la verita' anche quando `Max(Viewer, Cap)` sceglie il tetto — a `x1` sotto un
	 * tetto che morde `3x` una manopola che mostrasse la sola scelta direbbe `x1` mentre lo schermo scorre
	 * a `3x`.
	 *
	 * ⚠️ **E' un accessore, non un secondo produttore.** L'HUD lo legge e lo passa a
	 * `URTPlaybackLibrary::EffectivePlaybackSpeed` insieme alla scelta: la composizione resta una sola, qui.
	 * L'alternativa — far ricalcolare il tetto all'HUD da `MaxPlaybackSeconds` — e' la seconda verita' che
	 * il DoD vieta con le parole *«non ricalcola, non stima»*.
	 */
	float GetPlaybackCapSpeed() const { return PlaybackSpeed; }

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
	void ResolveCleanseActions(const FRTBlastContext& Ctx);

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

	/** Gli attaccanti sopravvissuti spendono l'abilita' (energia e cooldown); se gratuita, accumulano energia. */
	void ConsumeAttackerAbilities(FRTBlastContext& Ctx);

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
	 * Che cosa ha scelto chi ha risposto: `FireChosen`, `HoldChosen` (la scelta sicura) o `ResponseChosen`
	 * (una risposta attiva che non e' `FIRE`) — E14.7, [D-047].
	 *
	 * `static` e in un posto solo perche' i **due** produttori di decisione — il bot e il decisore iniettato
	 * — la classificavano ciascuno per conto proprio con un booleano. Finche' le classi erano due la
	 * duplicazione era invisibile; con la terza sarebbero divergiute al primo che qualcuno dimentica di
	 * aggiornare, e a divergere sarebbe stato l'esito che finisce nel TurnLog **autorevole**.
	 */
	static ERTReactionDecisionOutcome ClassifyChosenResponse(const FRTReactionOpportunity& Opportunity,
		const FString& Response);

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
		const TArray<FRTCellId>& Sources, const TArray<ARTUnit*>& Healers);

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

	/** TurnLog dell'ultimo turno risolto (osservabilita' autoritativa; ordinato in LockInAndResolve). */
	TArray<FRTTurnLogEntry> TurnLog;

	/** Stato della registrazione in corso: id, hash per turno, chiusura. Lo tiene il manifest stesso. */
	FRTReplayManifest ReplayManifest;

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
	FString ResolveReplaysRoot() const;

	/**
	 * Istante reale d'inizio registrazione, per la durata nel manifest. E' l'UNICO tempo reale che tocca
	 * l'archivio, e finisce in un campo che non entra in nessun hash.
	 */
	double ReplayStartRealSeconds = 0.0;

	/** Istante d'inizio in UTC, per la riga dell'indice (`#416`). Il manifest porta una durata, non un inizio. */
	FDateTime ReplayStartedUtc = FDateTime(0);

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
	float PlaybackSpeed = 1.f;              // fattore di accelerazione per rientrare nel tetto
	float PlaybackTotalSeconds = 0.f;       // durata stimata (per la progress bar)
	float PlaybackElapsedTotal = 0.f;
	int32 AttacksShown = 0;                 // colpi gia' rivelati nel Blast corrente

	// Trasformazione griglia in cache per convertire celle->mondo durante il playback.
	FVector PBOrigin = FVector::ZeroVector;
	float PBCellSize = 200.f;
	float PBLayerHeight = 0.f;
};
