#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/RTTurnLog.h" // ERTLogCategory: un'assertion sul log parla il vocabolario del log
#include "RTTestScenario.generated.h"

/**
 * Modello dati di uno SCENARIO di test automatico (RT Scenario Harness).
 *
 * Uno scenario descrive: chi c'e' in campo, cosa ciascuno fa turno per turno, e cosa deve risultare vero
 * alla fine. Il runner lo esegue attraverso il **percorso di gioco reale** (piani sulle unita' ->
 * `LockInAndResolve` -> resolver -> TurnLog), mai con scorciatoie tipo `SetActorLocation`: un test che non
 * passa dal codice vero non prova niente sul codice vero.
 *
 * Formato: JSON versionabile in `Scenarios/<Categoria>/<Nome>.json`. Non sono `.uasset` perche' devono
 * essere leggibili e diffabili in una PR.
 */

/** Esito di una esecuzione. `Error` NON e' un `Fail`: distinguere i due e' cio' che impedisce a uno scenario rotto di travestirsi da regressione di gioco. */
UENUM()
enum class ERTTestOutcome : uint8
{
	/** Simulazione completata, tutte le assertion soddisfatte. */
	Pass,
	/** Simulazione completata, almeno una assertion non soddisfatta. E' un difetto del GIOCO. */
	Fail,
	/** Impossibile eseguire: scenario invalido, eroe sconosciuto, mappa mancante. E' un difetto del TEST. */
	Error,
	/**
	 * Lo scenario e' valido e ha girato fin dove poteva, poi ha incontrato un turno che richiede una
	 * capability **non ancora costruita**. Non e' un difetto di nessuno: e' il progetto che non c'e' ancora.
	 *
	 * Serve a poter versionare uno showcase **prima** che tutti i suoi sistemi esistano. Senza questo esito
	 * la scelta sarebbe fra tenere la partita dimostrativa in Markdown per settimane, o vederla rossa ogni
	 * giorno finche' non e' completa — e una suite che ha un rosso "normale" smette di essere letta.
	 * Aggiunto IN CODA: i valori precedenti non cambiano numero.
	 */
	Blocked
};

/** Tipo di assertion. Prima iterazione: solo cio' che serve a `Movement.Basic` (§ASSERTION SYSTEM: «non creare un mega-framework prematuramente»). */
UENUM()
enum class ERTAssertionKind : uint8
{
	/** L'unita' indicata si trova sulla cella attesa a fine scenario. */
	UnitAtCell,
	/** Sono stati completati almeno N turni senza che la partita si interrompesse. */
	TurnsCompleted,
	/** La salute dell'unita' e' esattamente il valore atteso: e' cosi' che si verifica un danno. */
	UnitHpEquals,
	/** L'unita' e' viva (`value` != 0) oppure abbattuta (`value` == 0). */
	UnitAlive,
	/**
	 * L'unita' guarda nella direzione attesa (`ERTHexDirection` per nome: `E`, `NE`, `NW`, `W`, `SW`, `SE`).
	 *
	 * Aggiunta IN CODA (CP 16.1): i valori precedenti non cambiano numero. Verifica il facing LOGICO — quello
	 * che decide le regole — non lo yaw della mesh, che e' presentazione e puo' essere ancora a meta'
	 * interpolazione quando lo scenario finisce.
	 */
	UnitFacing,
	/**
	 * Quante volte un EVENTO compare nel TurnLog dello scenario. `value` e' il conteggio atteso, quindi `0`
	 * asserisce l'ASSENZA — che e' meta' di cio' che serve alla segretezza: «prima del reveal non deve
	 * esserci nessuna voce che dica quale scelta e' stata fatta».
	 *
	 * L'evento si nomina per NOME (`"category": "Environment", "outcome": "BridgeRemoved"`), come la
	 * direzione di `UnitFacing` e per la stessa ragione: un indice non dice niente a chi legge lo scenario, e
	 * rinumerare un enum renderebbe verdi gli scenari sbagliati. I nomi si risolvono per RIFLESSIONE
	 * (`StaticEnum<>`), non da una tabella scritta a mano — quelle divergono, ed e' gia' successo in questo
	 * file per l'elenco delle chiavi note.
	 *
	 * Aggiunta IN CODA: i valori precedenti non cambiano numero, come per `UnitFacing` a CP 16.1.
	 */
	LogEventCount,
	/**
	 * ORDINE RELATIVO di due eventi nel TurnLog: la prima occorrenza di `category`/`outcome` precede la prima
	 * di `thenCategory`/`thenOutcome`.
	 *
	 * Legge il log nell'ordine in cui e' stato SCRITTO, che e' l'ordine di risoluzione. Non e' un dettaglio:
	 * la forma canonica del log — quella serializzata e quella che entra in `HashTurnLog` — e' **ordinata**
	 * (`URTTurnLogLibrary::SortTurnLog`), quindi l'hash e' invariante per permutazione e **non sa niente
	 * dell'ordine**. Chi volesse verificare una sequenza guardando il checksum verificherebbe un'altra cosa.
	 */
	LogEventOrder,
	/**
	 * VALORE di `Amount` della prima voce che corrisponde a `category`/`outcome`.
	 *
	 * E' la terza primitiva prevista da `#318`, ed e' rimasta indietro finche' non e' esistito un formato da
	 * leggere: `#361` lo ha deciso, e `spec-turnlog.md` §4.2 lo dichiara — il Decision Time Bank scrive
	 * `Decision/BankConsumed` e `Decision/BankAfter` con i millisecondi in `Amount`.
	 *
	 * Non e' un'assertion per il bank: `Amount` e' il payload numerico di OGNI categoria — danno per `Combat`,
	 * celle percorse per `Move`, direzione per `Facing` — quindi questa primitiva serve tutte allo stesso modo.
	 * Era gia' possibile verificare che un colpo fosse avvenuto (`LogEventCount`); ora si puo' verificare
	 * QUANTO ha tolto.
	 *
	 * La PRIMA voce, non la somma: sommarle mescolerebbe finestre diverse in un numero solo, e il residuo di
	 * una decisione non e' la somma dei residui. Chi vuole la seconda occorrenza usa un evento piu' specifico
	 * — e se un giorno servisse davvero, si aggiunge un indice, non si cambia il significato di questa.
	 *
	 * Un evento ASSENTE fallisce dicendo che e' assente, non confrontando uno zero: e' lo stesso trattamento
	 * che `LogEventOrder` riserva agli eventi mancanti, e per la stessa ragione — un difetto di produzione non
	 * va fatto sembrare un difetto di valore.
	 *
	 * Aggiunta IN CODA: i valori precedenti non cambiano numero, come `UnitFacing` a CP 16.1 e le due di `#318`.
	 */
	LogEventAmount
};

/**
 * Modifica di una cella dell'arena generata: ostacoli, muri, terreno costoso.
 *
 * Serve a scrivere scenari come `Movement.Blocked` senza versionare un `.umap`: l'arena resta generata da
 * codice e lo scenario ne cambia le poche celle che gli interessano. Le celle non elencate restano pavimento
 * a costo 1.
 */
USTRUCT()
struct FRTScenarioCell
{
	GENERATED_BODY()

	UPROPERTY()
	FRTCellId Cell;

	/** Ostacolo: non ci si passa. Il pathfinding la aggira o rifiuta il percorso. */
	UPROPERTY()
	bool bBlocksMovement = false;

	/** Muro: non ci si vede attraverso. Non impedisce il passaggio. */
	UPROPERTY()
	bool bBlocksLineOfSight = false;

	/** Costo di traversata. 0 = non specificato, resta il default della cella (1). */
	UPROPERTY()
	int32 MoveCost = 0;

	/**
	 * Sovrapprezzo di occupazione (#619): quanto costa in PIU' entrare, perche' la geometria stringe la cella.
	 * Si somma a `MoveCost`, non lo sostituisce.
	 *
	 * Esiste qui perche' altrimenti nessuno scenario saprebbe esprimere una cella STRETTA, e l'effetto di
	 * `Constrained` sarebbe verificabile solo da un test C++ — cioe' mai in una partita vera.
	 * `0` significa «cella larga», che e' anche il default di una cella non elencata.
	 */
	UPROPERTY()
	int32 OccupancySurcharge = 0;
};

/** Un'unita' schierata dallo scenario. */
USTRUCT()
struct FRTScenarioUnit
{
	GENERATED_BODY()

	/** ID locale allo scenario (`A1`), usato dagli intent e dalle assertion. Non e' l'ID di gioco. */
	UPROPERTY()
	FString Id;

	/** ID stabile dell'eroe dal catalogo: `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`. */
	UPROPERTY()
	FName HeroId;

	UPROPERTY()
	int32 TeamId = 0;

	UPROPERTY()
	FRTCellId Cell;

	/**
	 * Da che parte l'unita' guarda quando lo scenario comincia. Default `E`, che e' lo stesso default di
	 * `ARTUnit`: omettere il campo lascia il gioco com'era.
	 *
	 * ⚠️ **E' dato di PIAZZAMENTO, non una rotazione dichiarata.** La distinzione conta: la capability
	 * `DeclaredRotation` resta indisponibile (D-020, #291) perche' dichiarare una rotazione IN PIANIFICAZIONE
	 * e' una mossa che il giocatore ancora non ha modo di chiedere, e darla all'harness lo renderebbe piu'
	 * capace del gioco. Dire dove una figura guarda quando la si posa sul tavolo e' invece la stessa classe di
	 * `cell` — nessuno «dichiara» di stare in una cella, ci si sta e basta.
	 *
	 * Serve da CP 13.2: da quando il targeting consuma la conoscenza, l'orientamento decide COSA la squadra
	 * vede, quindi uno scenario che non potesse esprimerlo non potrebbe descrivere un tiratore che guarda il
	 * proprio bersaglio.
	 */
	UPROPERTY()
	ERTHexDirection Facing = ERTHexDirection::E;

	/**
	 * L'unita' e' guidata dal BOT: il suo intent non sta nel file, lo produce l'utility scoring.
	 *
	 * ⚠️ **Non e' l'asimmetria che tiene fuori `ReactionPlanning` e `DeclaredRotation`.** Quelle restano
	 * indisponibili perche' l'harness sarebbe il PRIMO produttore di un campo che nessuno scrive in partita.
	 * Qui il produttore esiste ed e' quello vero: `ARTTurnManager::PlanBots()`, chiamato in partita a ogni
	 * turno. L'harness non apre un canale nuovo, usa `PlanBotsForTest()` — l'appiglio che
	 * `URTScenarioRunner` dichiara da sempre come l'unico, e che i test d'integrazione usano gia'.
	 *
	 * ⚠️ **Cosa questo NON e'**: il seam dei `DecisionProvider` di [D-101] (#542, v0.2). Quello serve quando i
	 * modi di giocare uno scenario diventano tre (bot vs bot, replay, umano+bot) e ognuno vorrebbe il proprio
	 * appiglio. Qui il modo resta uno: le unita' non-bot prendono gli intent dal file, le bot dal pianificatore
	 * del gioco, come in una partita 2v2 offline — che e' precisamente la v0.1.
	 *
	 * Un'unita' bot **non puo' avere intent nel file**: `Validate()` lo rifiuta. Ammetterli significherebbe che
	 * uno dei due sovrascrive l'altro in silenzio, e lo scenario direbbe di misurare il bot mentre misura il
	 * file (o viceversa, a seconda dell'ordine — cioe' di un dettaglio d'implementazione).
	 */
	UPROPERTY()
	bool bBotControlled = false;

	/**
	 * Salute iniziale. `-1` = non dichiarata, resta quella del roster.
	 *
	 * ⚠️ **Il sentinella e' `-1` e non `0` deliberatamente**: `0` e' un valore legittimo per `shield`, e usarlo
	 * come «non dichiarato» renderebbe impossibile chiedere *scudo azzerato* — che e' esattamente cio' che
	 * serve a un'esca. Un campo il cui valore d'assenza e' anche una richiesta valida non sa distinguere le
	 * due cose.
	 *
	 * Serve agli scenari in cui la SCELTA di un'unita' dipende da quanto e' ferita l'altra: un bersaglio a HP
	 * pieni non e' un'esca, e uno scenario che volesse dimostrare «non lo bersaglia» resterebbe verde per la
	 * ragione sbagliata — perche' non era invitante, non perche' non lo conosce.
	 */
	UPROPERTY()
	int32 Health = -1;

	/** Scudo iniziale. `-1` = non dichiarato, resta quello del roster. Vedi `Health` per il perche' del sentinella. */
	UPROPERTY()
	int32 Shield = -1;

	/**
	 * Portata visiva. `-1` = non dichiarata, resta quella del roster.
	 *
	 * ⚠️ **E' condizione iniziale, non bilanciamento**: la scrive gia' `ConfigureFromHeroData` dai dati
	 * dell'eroe, esattamente come `Health`, e dichiararla nello scenario non apre un canale che il gioco non
	 * abbia.
	 *
	 * Serve perche' altrimenti la premessa di uno scenario sulla conoscenza dipenderebbe dai numeri del
	 * roster, **che cambiano**: `D-073` ha appena portato Flux da un valore all'altro, e con `AttackRange` a 5
	 * contro viste da 5 a 7 non esiste oggi una distanza in cui un nemico sia insieme fuori vista e sotto tiro
	 * — cioe' la sola configurazione in cui «non lo bersaglia» dimostri qualcosa. Il test C++ gemello
	 * (`HexBotPlay.PlansOnPartialKnowledge`) dichiara la vista nel test per la stessa ragione, e lo scrive.
	 */
	UPROPERTY()
	int32 VisionRange = -1;

	/**
	 * L'EQUIPAGGIAMENTO che l'unita' porta: `EquipmentId` dal catalogo §1/§2/§3 (`#602`). Vuoto = il loadout
	 * di default del suo eroe, quindi gli scenari scritti prima non cambiano comportamento.
	 *
	 * ⚠️ **E' configurazione, non un'azione del giocatore**, ed e' la ragione per cui questa chiave esiste
	 * mentre `reaction` in pianificazione resta fuori (vedi `RTScenarioSession.cpp`): equipaggiare appartiene
	 * alla stessa classe di `cell`, `hero` e `facing` — cose che si decidono posando le figure sul tavolo, non
	 * mosse che il giocatore chiede durante il turno. L'harness che sceglie un loadout non e' piu' capace del
	 * gioco: sta scegliendo fra configurazioni che il gioco gia' assegna.
	 *
	 * Serve perche' i loadout consigliati usano **quattro** dei sette moduli di reazione (DoD di `#63`): senza
	 * questa chiave, `Anchor`, `Cleanse` e `CounterShot` — piu' le varianti e i gadget non consigliati — non
	 * sono esprimibili in nessuno scenario, e meta' del catalogo equipaggiamento resta senza specifica
	 * eseguibile.
	 *
	 * Passa dalla **stessa validazione del gioco** (`URTCatalogLibrary::ValidateLoadout`): una configurazione
	 * illegale rifiuta lo scenario con un motivo. Senza, l'harness diventerebbe piu' PERMISSIVO del gioco, che
	 * e' l'altra meta' della stessa asimmetria.
	 */
	UPROPERTY()
	TArray<FName> Loadout;
};

/**
 * L'intento di una unita' in un turno: movimento, abilita', o entrambi.
 *
 * Movimento e abilita' convivono perche' convivono nel gioco — un'unita' ha uno slot movimento e uno slot
 * principale, e usarli insieme e' la norma, non un caso limite.
 */
USTRUCT()
struct FRTScenarioIntent
{
	GENERATED_BODY()

	UPROPERTY()
	FString UnitId;

	/** Waypoint del movimento, come li produrrebbe il giocatore cliccando. Vuoto = l'unita' non si sposta. */
	UPROPERTY()
	TArray<FRTCellId> Move;

	/**
	 * `ActionId` dell'abilita' da usare (`Flux.ArcPulse`). Vuoto = nessun attacco.
	 *
	 * Per **ID** e non per indice: l'indice di un'abilita' nel kit si sposta appena qualcuno ne aggiunge una,
	 * e lo scenario continuerebbe a passare verificando l'abilita' sbagliata — il tipo di test che mente.
	 */
	UPROPERTY()
	FName Ability;

	/** ID di scenario del bersaglio. Obbligatorio quando `Ability` e' valorizzata. */
	UPROPERTY()
	FString Target;

	/**
	 * `ActionId` della mobilita' RAPIDA (`Vektor.PassingBlade`, `Action.Dash`): risolve in fase Dash, prima
	 * del Blast. Vuoto = nessuno scatto.
	 *
	 * Campo separato da `Ability` e non un'alternativa, perche' dopo [D-028] occupano slot diversi: lo scatto
	 * prende il movimento, l'abilita' la principale, e *schivo e sparo* e' un turno legale che l'intent deve
	 * poter esprimere. Con un solo campo non sarebbe rappresentabile.
	 */
	UPROPERTY()
	FName Dash;

	/** Cella d'arrivo dello scatto. Obbligatoria quando `Dash` e' valorizzata. */
	UPROPERTY()
	FRTCellId DashCell;

	/**
	 * Cella bersagliata da `Ability`, in alternativa a `Target`: le aree si centrano su una CELLA, che puo'
	 * essere vuota. Valida solo con `bTargetsCell`.
	 *
	 * Dichiarare **entrambi** e' un `ERROR` di validazione, non una scelta fra i due: chi ha scritto lo
	 * scenario non sa cosa voleva, e sceglierne uno al posto suo produrrebbe un test verde su una premessa
	 * sbagliata — peggio di un rosso, perche' nessuno va a guardarlo.
	 */
	UPROPERTY()
	FRTCellId TargetCell;

	/** Vero se `Ability` mira a `TargetCell` invece che a un'unita'. Serve a distinguere «cella (0,0)» da «campo non dichiarato». */
	UPROPERTY()
	bool bTargetsCell = false;

	/**
	 * Bordo bersagliato dalle azioni che agiscono su una STRUTTURA di bordo (CP 9.5: erigere o spostare una
	 * copertura). Nel JSON e' il nome della direzione: `"edge": "E"` .. `"SE"`.
	 *
	 * Serve perche' una copertura sta su un BORDO e una cella ne ha sei: la cella da sola non basta a dire
	 * quale. Vale solo con `bHasCoverEdge`, per la stessa ragione per cui `TargetCell` ha il suo flag — `E` e'
	 * una direzione legittima e non puo' fare da «non dichiarato».
	 */
	UPROPERTY()
	ERTHexDirection CoverEdge = ERTHexDirection::E;

	UPROPERTY()
	bool bHasCoverEdge = false;

	/**
	 * `ActionId` della REAZIONE che l'unita' arma per questo turno (`Flux.ReactiveCapacitor`). Vuota = nessuna.
	 *
	 * Armare non e' agire: la reazione dichiara solo **cosa succedera' se** il trigger scatta durante la
	 * risoluzione. Non ha bersaglio — lo decide il trigger (chi ha colpito, quale alleato e' stato preso) —
	 * ed e' per questo che sta in un campo suo invece di riusare `Ability`/`Target`: sono due slot diversi
	 * dell'unita' (`PlannedReactionAbility` contro `PlannedAbilityIndex`), e la stessa unita' puo' attaccare
	 * e reagire nello stesso turno.
	 *
	 * Per ID, per la stessa ragione dell'abilita'.
	 */
	UPROPERTY()
	FName Reaction;

	/**
	 * Rotazione DICHIARATA in pianificazione (D-020): `"facing": "NE"`. Da non confondere con
	 * `FRTScenarioUnit::Facing`, che e' dato di **piazzamento** — quello dice come un'unita' comincia il
	 * turno, questo dice come chiede di finirlo.
	 *
	 * 🔴 **Questa chiave e' arrivata TARDI di proposito, ed e' la storia che spiega il flag.** Fino al
	 * 2026-08-13 non esisteva, e non per dimenticanza: `PlannedFacing` non aveva alcun produttore nel gioco,
	 * quindi darla agli scenari avrebbe reso l'harness il **primo** produttore del campo — verdi che dicevano
	 * che il giocatore poteva girarsi restando fermo, mentre non aveva alcun modo di chiederlo (#291). La
	 * capability `DeclaredRotation` e' rimasta fuori dall'elenco dei disponibili per tutto quel tempo.
	 * Con `#737` il produttore esiste (`ARTPlayerController::HandleFacingSector`), quindi la premessa e'
	 * caduta e la chiave puo' entrare: ora un verde dice qualcosa di vero sul giocatore.
	 *
	 * Vale solo con `bDeclaresFacing`, per la stessa ragione di `CoverEdge`: `E` e' una direzione legittima e
	 * non puo' fare da «campo non dichiarato».
	 */
	UPROPERTY()
	ERTHexDirection Facing = ERTHexDirection::E;

	UPROPERTY()
	bool bDeclaresFacing = false;
};

/** Una cella riscritta da una variante: la stessa unita', altrove. */
USTRUCT()
struct FRTScenarioVariantUnit
{
	GENERATED_BODY()

	/** ID di scenario dell'unita' da spostare. Dev'essere una di quelle schierate. */
	UPROPERTY()
	FString Id;

	/** Dove parte in questa variante, al posto della cella dichiarata in `units`. */
	UPROPERTY()
	FRTCellId Cell;
};

/**
 * Una variante dello stesso allestimento: tutto identico tranne le celle che dichiara.
 *
 * Solo le CELLE, e la limitazione e' deliberata: una variante che potesse cambiare eroi, squadre o condizione
 * iniziale non sarebbe piu' «lo stesso scenario con un ingresso diverso», e il confronto fra le sue tracce non
 * direbbe piu' quale ingresso ha prodotto la differenza. Il giorno in cui servisse variare altro, lo si
 * aggiunge sapendo che si sta allargando cio' che il canary puo' attribuire.
 */
USTRUCT()
struct FRTScenarioVariant
{
	GENERATED_BODY()

	/** Nome leggibile, unico nello scenario: compare nel report ed e' il modo di sapere QUALE variante e' rossa. */
	UPROPERTY()
	FString Name;

	UPROPERTY()
	TArray<FRTScenarioVariantUnit> Units;
};

/** Un turno dello scenario. */
USTRUCT()
struct FRTScenarioTurn
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRTScenarioIntent> Intents;

	/**
	 * Capability che questo turno richiede per essere eseguito (`PredictiveAction`, `Objective`, `Facing`,
	 * `DecisionBoundary`, `InterceptRevalidation`, ...). Vuoto = nessun requisito oltre al gioco corrente.
	 *
	 * Il turno NON viene giocato se una manca: il risultato e' `Blocked` con il nome della capability, non
	 * un `Fail`. E' cio' che permette allo showcase di esistere come dato mentre lo si costruisce, e di
	 * dichiarare da solo **quanto lontano arriva oggi**.
	 */
	UPROPERTY()
	TArray<FString> Requires;
};

/** Una condizione da verificare a fine scenario. */
USTRUCT()
struct FRTTestExpectation
{
	GENERATED_BODY()

	UPROPERTY()
	ERTAssertionKind Kind = ERTAssertionKind::UnitAtCell;

	/** Unita' coinvolta (`UnitAtCell`). */
	UPROPERTY()
	FString UnitId;

	/** Cella attesa (`UnitAtCell`). */
	UPROPERTY()
	FRTCellId Cell;

	/** Valore intero atteso (`TurnsCompleted`, `UnitHpEquals`, `UnitAlive`, `LogEventCount`, `LogEventAmount`). */
	UPROPERTY()
	int32 Value = 0;

	/**
	 * Evento del TurnLog: categoria e esito, gia' risolti dai nomi del JSON (`LogEventCount`, `LogEventOrder`).
	 *
	 * L'esito e' un `uint8` il cui significato dipende dalla categoria — `ERTMoveOutcome` se `Move`,
	 * `ERTEnvironmentOutcome` se `Environment`, e cosi' via: e' la stessa convenzione di `FRTTurnLogEntry`,
	 * non una scorciatoia dell'harness.
	 */
	UPROPERTY()
	ERTLogCategory LogCategory = ERTLogCategory::Move;

	UPROPERTY()
	uint8 LogOutcome = 0;

	/** Secondo evento, quello che deve venire DOPO (`LogEventOrder`). */
	UPROPERTY()
	ERTLogCategory ThenCategory = ERTLogCategory::Move;

	UPROPERTY()
	uint8 ThenOutcome = 0;
};

/** Scenario completo, come letto dal file. */
USTRUCT()
struct FRTTestScenario
{
	GENERATED_BODY()

	/** ID stabile e gerarchico: `Movement.Basic`. Da' il nome alla cartella di output. */
	UPROPERTY()
	FString ScenarioId;

	/** Versione del FORMATO dello scenario, non del contenuto. Un loader piu' nuovo deve poter rifiutare un formato che non conosce. */
	UPROPERTY()
	int32 Version = 1;

	/**
	 * ID di scenario dell'unita' da **selezionare** quando lo scenario parte con un giocatore presente (PIE).
	 * Vuoto = nessuna selezione, ed e' il caso normale.
	 *
	 * Serve a una cosa sola, e va detta perche' non e' ovvia: l'ANTEPRIMA di pianificazione — celle colpite in
	 * rosso, alleato nell'area in arancione — non la produce il resolver ma il **player controller**, a
	 * partire dal piano dell'unita' SELEZIONATA. Uno scenario allestisce il campo e risolve, quindi da solo
	 * non fa comparire nulla: si vede l'esito, mai l'intenzione.
	 *
	 * Con questo campo la sessione scrive il piano d'attacco del primo turno sull'unita' indicata e la
	 * seleziona **dalla stessa porta del giocatore** (`HandleClickOnUnit`), cosi' l'anteprima compare da sola
	 * e resta visibile per tutta la pausa prima del turno. E' presentazione: headless non c'e' nessun
	 * controller e il campo non fa niente, quindi l'esito logico non cambia di una virgola.
	 */
	UPROPERTY()
	FString PreviewUnit;

	/**
	 * Seed dichiarato ma **non consumato**: oggi il progetto non ha alcun RNG e il determinismo viene da
	 * coordinate intere e ordinamenti totali (`HexSim.ReplayDivergenceZero`). Il campo esiste perche' il
	 * giorno in cui un RNG entrera' nel resolver lo scenario debba gia' saperlo dichiarare — non perche'
	 * faccia qualcosa adesso. Impostarlo a un valore diverso da 0 e' lecito e viene registrato nel report.
	 */
	UPROPERTY()
	int32 Seed = 0;

	/**
	 * **Fixture di mappa riferita per nome** (`RelayBasin`, `RelayLite`, `TestArena`). Vuoto = si genera
	 * l'arena da `MapRadius`, come prima.
	 *
	 * Riferire invece di duplicare non e' economia di righe: la geometria canonica vive in UN posto solo,
	 * gia' protetto da un test (`ShowcaseRelay.BasinLayoutMatchesSpec`). Quarantacinque celle copiate in un
	 * JSON sarebbero una seconda verita' che nessuno confronta con la prima — e che deriverebbe in silenzio.
	 *
	 * Un nome sconosciuto e' `Error`, mai un'arena vuota: la partita girerebbe comunque, dando un `Fail`
	 * incomprensibile al posto di un messaggio che dice cosa manca.
	 */
	UPROPERTY()
	FString Fixture;

	/** Raggio dell'arena esagonale generata quando `Fixture` e' vuoto. Mappa da codice: nessun `.umap`. */
	UPROPERTY()
	int32 MapRadius = 3;

	/** Celle da modificare nell'arena generata (ostacoli, muri, terreno costoso). Vuoto = arena liscia. */
	UPROPERTY()
	TArray<FRTScenarioCell> Cells;

	UPROPERTY()
	TArray<FRTScenarioUnit> Units;

	UPROPERTY()
	TArray<FRTScenarioTurn> Turns;

	UPROPERTY()
	TArray<FRTTestExpectation> Expect;

	/**
	 * Varianti dello stesso allestimento, giocate una per una. Vuoto = una sola esecuzione, il caso normale.
	 *
	 * Ogni variante rigioca lo scenario cambiando **solo** le celle che dichiara, e deve superare le stesse
	 * `expect` di tutte le altre.
	 */
	UPROPERTY()
	TArray<FRTScenarioVariant> Variants;

	/**
	 * Le varianti devono produrre lo **stesso TurnLog**, byte per byte.
	 *
	 * ⚠️ **E' il solo modo di esprimere un canary d'indipendenza**, e nessuna `expect` normale lo sostituisce:
	 * quelle guardano lo stato finale di UNA partita, mentre qui la proprieta' da dimostrare e' che un
	 * ingresso *non ha avuto effetto*. Non e' osservabile in una partita sola — «il bot non ha usato
	 * un'informazione» ha la stessa forma osservabile di «il bot ha deciso cosi'», e le due si distinguono
	 * soltanto cambiando quell'informazione e guardando se l'esito si muove.
	 *
	 * Si confronta il TURNLOG e non lo stato finale (`StateHash`) per una ragione che non e' comodita': lo
	 * stato finale contiene la posizione dell'unita' nascosta, **che le varianti cambiano per costruzione** —
	 * un confronto su quello sarebbe rosso sempre, e per il motivo sbagliato. Il TurnLog registra cio' che e'
	 * SUCCESSO, e un'unita' che nessuno vede e che non agisce non vi scrive nulla.
	 *
	 * ⚠️ **Da solo non basta**, e chi scrive lo scenario deve saperlo: due partite in cui non succede niente
	 * hanno TurnLog identici. Serve accanto almeno una `expect` che dimostri che qualcosa e' successo davvero
	 * — e' la stessa premessa che il test C++ gemello scrive a mano
	 * (`HexBotPlay.HiddenEnemyFairness`: *«nelle due partite il bot ha davvero pianificato qualcosa»*).
	 */
	UPROPERTY()
	bool bExpectSameAcrossVariants = false;

	/** Trova un'unita' per ID di scenario. Nullptr se assente. */
	const FRTScenarioUnit* FindUnit(const FString& InId) const
	{
		return Units.FindByPredicate([&InId](const FRTScenarioUnit& U) { return U.Id == InId; });
	}
};
