#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Combat/RTOffensiveActionLibrary.h" // FRTSuppressiveZone, FRTSuppressionMover: la geometria e' UNA
#include "Perception/RTPerceptionLibrary.h"  // ERTAwareness: il trigger richiede `Detected`, non «visibile»
#include "Turn/RTDeclaredCondition.h" // FRTDeclaredCondition: header leggero, lo usa anche ARTUnit
#include "Turn/RTTurnLog.h" // ERTReactionDecisionOutcome: l'esito di una finestra vive con gli altri esiti di log
#include "Turn/RTTurnRules.h"
#include "RTReactionOpportunityTypes.generated.h"

class URTHexMapAsset;

/**
 * I sei campi che individuano una `ReactionOpportunity` in una partita (CP 14.3).
 *
 * Esiste perche' l'id dell'opportunity dev'essere una FUNZIONE dello stato e non un valore generato: la
 * decisione presa in una finestra entra nel TurnLog, e il replay la ritrova solo se l'id si ricalcola identico
 * su una seconda esecuzione. Un `FGuid::NewGuid()` sarebbe piu' breve da scrivere e renderebbe il replay non
 * riproducibile senza che nessun test se ne accorga.
 */
USTRUCT()
struct FRTReactionOpportunityKey
{
	GENERATED_BODY()

	/** Turno in cui l'opportunity si apre. */
	UPROPERTY()
	int32 TurnNumber = 0;

	/**
	 * Macro-fase del turno in cui l'opportunity si apre: due opportunity dello stesso turno in fasi diverse
	 * sono distinte.
	 *
	 * E' `ERTMatchPhase` — `Planning · Prep · Dash · Blast · Move · Cleanup` — e **non** `ERTResolutionPhase`,
	 * che il suo stesso owner dichiara non essere una macro-fase: quelli sono i codici `0/10/20/…` del
	 * catalogo, cioe' la fase che un'AZIONE dichiara, con la conversione in
	 * `URTCatalogLibrary::MapResolutionPhase` (e il codice 20 che si sdoppia). Un'opportunity nasce in una
	 * fase del TURNO, non in una dichiarata da un'azione.
	 */
	UPROPERTY()
	ERTMatchPhase MacroPhase = ERTMatchPhase::Planning;

	/**
	 * Micro-step del movimento in cui l'opportunity nasce (CP 14.2).
	 *
	 * ⚠️ `FRTMovementResolutionState::MicroStepIndex` e' oggi documentato come «diagnostico: nessuna regola lo
	 * legge». Entrando qui smette di esserlo: diventa parte di un identificatore che finisce nell'hash del
	 * replay. Il commento alla sorgente va aggiornato quando questa chiave viene popolata dal resolver, o
	 * mentira' esattamente come i due commenti su `Slow` corretti in `ceb1b29`.
	 */
	UPROPERTY()
	int32 MicroStepIndex = 0;

	/** Unita' che possiede l'opportunity — chi puo' rispondere, non chi ha innescato. */
	UPROPERTY()
	int32 OwnerId = INDEX_NONE;

	/**
	 * La reaction che l'ha aperta (`Action.Counter`, `Action.Deflect`, ...).
	 *
	 * `FName` come `FRTActionDef::ActionId`, e non `FString`: il chiamante scrivera' `Def.ActionId` senza
	 * conversione. Con una `FString` il caller avrebbe dovuto passare `ActionId.ToString()`, che restituisce
	 * la casing dell'ISTANZA — e `FName` confronta senza distinguerla. Due punti del codice che l'engine
	 * considera la stessa azione avrebbero prodotto due id diversi: esattamente «un identificatore che non e'
	 * funzione del suo stato», il difetto che questo file esiste per impedire, spostato di un livello.
	 */
	UPROPERTY()
	FName ReactionDefId;

	/**
	 * Progressivo fra opportunity identiche in tutto il resto.
	 *
	 * Serve al caso che sembra impossibile finche' non capita: la stessa unita', la stessa reaction, lo stesso
	 * micro-step, due volte. Senza `Seq` le due collidono e il replay attribuisce a una la decisione dell'altra.
	 */
	UPROPERTY()
	int32 Seq = 0;
};

/**
 * Una opportunity di reazione: chi puo' rispondere, e **quali risposte gli sono legali** (ADR-0004 §2).
 *
 * La cardinalita' di `AllowedResponses` e' l'unico discriminante fra i due regimi, e non esiste un secondo
 * criterio: `≤ 1` e' il caso degenere — le reazioni E5 di oggi, che scattano o non scattano — e `≥ 2` apre il
 * decision boundary. Un modello solo, con E5 conservata dentro, invece di due da mantenere.
 */
USTRUCT()
struct FRTReactionOpportunity
{
	GENERATED_BODY()

	/** Identita' dell'opportunity: vedi `FRTReactionOpportunityKey`. */
	UPROPERTY()
	FRTReactionOpportunityKey Key;

	/**
	 * Le risposte legali per il possessore, gia' filtrate.
	 *
	 * ⚠️ Vuoto e singleton **non** sono lo stesso caso per chi legge — nessuna risposta legale contro una sola
	 * — ma per la regola del boundary lo sono: in entrambi non c'e' niente da scegliere. Il conteggio si fa
	 * qui e non sul catalogo: [D-047](../../../docs/decisions/RT_PDR_00_Decision_Log.md) precisa che la
	 * cardinalita' di `Brace` e' 1 **per il profilo base**, non per natura, e un profilo d'eroe che dichiari
	 * una seconda risposta apre il boundary con questa stessa regola invece di aggiungerne una nuova.
	 */
	UPROPERTY()
	TArray<FString> AllowedResponses;
};


/**
 * Le tre intenzioni della grammatica di playtest (`spec-reaction-clash-e14.md` §4, [D-049]).
 *
 * ⚠️ **`PROPOSED FOR PLAYTEST`, non canoniche**: la matrice `READ > STAND > SHIFT > READ` e' universale, i
 * payoff per eroe NO — quelli non entrano nei dati d'eroe finche' il playtest non li promuove.
 *
 * L'enum e' **chiuso a tre valori**, ed e' l'unico enum nuovo che E14.7 introduce (§11). Un quarto valore
 * sarebbe una quarta intenzione, cioe' una modifica alla grammatica: si decide, non si aggiunge.
 */
UENUM()
enum class ERTGrammarIntent : uint8
{
	/** Tenere la posizione. Il fallback di chi e' in `Brace`, e la scelta sicura di §9. */
	Stand,
	/** Leggere l'avversario e anticiparlo. Batte `Stand`, perde contro `Shift`. */
	Read,
	/** Spostarsi per uscire dalla linea. Batte `Read`, perde contro `Stand`. */
	Shift
};


/**
 * Un partecipante a un boundary contested: chi e', e **la sua** opportunity.
 *
 * 🔴 **L'opportunity e' PER PARTECIPANTE, e non e' un dettaglio di comodita': e' la privacy di §7.2.**
 * Quella tabella dice che in un Clash l'esistenza della finestra e' nota a entrambi, ma le *risposte legali
 * dell'altro* non sono **mai inviate** e la sua scelta non lo e' prima del reveal. Un unico
 * `FRTReactionOpportunity` che elencasse i partecipanti con le rispettive risposte sarebbe, per costruzione,
 * il DTO che le rivela — quindi non e' quello che viaggia.
 *
 * ∴ ogni partecipante riceve il DTO a due campi che ha sempre ricevuto (`Key` + le **sue**
 * `AllowedResponses`), e i partecipanti vivono ACCANTO. E' lo stesso motivo per cui `FRTOverwatchTrigger`
 * tiene i bersagli fuori dal DTO invece che dentro, dichiarato nel commento di quel tipo: l'elenco chiuso di
 * campi verificato da `Overwatch.OpportunityLeaksNoFuture` e' l'unica barriera che impedisce a un campo di
 * informazione altrui di entrare, e si allarga solo con una ragione — non per far stare comoda una feature.
 */
USTRUCT()
struct FRTReactionParticipant
{
	GENERATED_BODY()

	/** Chi risponde. Indice di unita', come `FRTReactionOpportunityKey::OwnerId`. */
	UPROPERTY()
	int32 UnitId = INDEX_NONE;

	/** Cio' che QUESTO partecipante vede: la sua chiave e le sue risposte legali, e niente dell'altro. */
	UPROPERTY()
	FRTReactionOpportunity Opportunity;

	FRTReactionParticipant() = default;
	FRTReactionParticipant(int32 InUnitId, const FRTReactionOpportunity& InOpportunity)
		: UnitId(InUnitId), Opportunity(InOpportunity) {}
};


/**
 * Un boundary con piu' di un partecipante (E14.7, [D-048]).
 *
 * ⚠️ **Non esiste un campo `Type = Clash`, e questo tipo non lo introduce**: `IsContested` e' una funzione
 * dei partecipanti e delle loro cardinalita', ricalcolata dove serve. Un campo sarebbe una seconda verita'
 * accanto alla cardinalita', e le due divergerebbero al primo profilo che cambia — e' il rischio (b) che
 * l'epic elenca e la ragione per cui §3.1 dice «contested e' derivato, non dichiarato».
 *
 * `Key` e' quella CONDIVISA — l'identita' del boundary, non di una delle due opportunity — cosi' che il
 * TurnLog possa registrare due lock contro un solo `OpportunityId` e il budget di §8 contarne **uno**.
 */
USTRUCT()
struct FRTContestedBoundary
{
	GENERATED_BODY()

	/** L'identita' del boundary, condivisa fra i partecipanti. */
	UPROPERTY()
	FRTReactionOpportunityKey Key;

	/**
	 * I partecipanti, in **ordine canonico** e non di arrivo (§7.3).
	 *
	 * L'ordine di arrivo sarebbe una dipendenza dal tempo reale, contro l'invariante #4: due esecuzioni
	 * della stessa partita registrerebbero i lock in ordini diversi e l'hash divergerebbe.
	 */
	UPROPERTY()
	TArray<FRTReactionParticipant> Participants;
};


/**
 * La scelta **bloccata** da un partecipante, non ancora rivelata (§7.1).
 *
 * "Lock" e non "risposta": la differenza e' che una risposta e' pubblica quando arriva, un lock no. Fra il
 * lock e il reveal la scelta esiste, e' vincolante, e **nessuno la vede** — nemmeno il fatto che sia
 * arrivata (§7.2: *«momento del lock dell'altro: non osservabile»*).
 */
USTRUCT()
struct FRTContestedLock
{
	GENERATED_BODY()

	UPROPERTY()
	int32 UnitId = INDEX_NONE;

	/** Una delle `AllowedResponses` di QUESTO partecipante. */
	UPROPERTY()
	FString Response;

	/** L'intenzione di grammatica che la risposta esprime ([D-049]). */
	UPROPERTY()
	ERTGrammarIntent Intent = ERTGrammarIntent::Stand;

	/**
	 * La maneuver scelta ha un **costo** (§9). Falso = gratuita, e allora non c'e' niente da consumare.
	 *
	 * ⚠️ **Nessuna risorsa nuova**: §9 dice di usare `Charges`, cooldown e stato del dispositivo che il
	 * progetto ha gia'. Questo flag dice *se* la maneuver costi, non *cosa*: il pagamento vero lo esegue chi
	 * possiede la risorsa, e questa struct non lo duplica.
	 */
	UPROPERTY()
	bool bManeuverHasCost = false;

	/**
	 * La **policy diversa** che §9 lascia dichiarare alla maneuver: se vero, il costo NON si consuma quando
	 * il Clash e' perso.
	 *
	 * ⚠️ Il default e' `false`, ed e' la regola: si paga **al lock valido, anche perdendo**. Senza,
	 * *«provo comunque, tanto se perdo non costa nulla»* svuota la scelta — che e' cio' che il Clash esiste
	 * per creare. L'eccezione esiste perche' la spec la prevede, ma va **dichiarata** dalla maneuver: un
	 * default che rimborsa avrebbe reso la regola l'eccezione.
	 */
	UPROPERTY()
	bool bRefundIfLost = false;

	FRTContestedLock() = default;
	FRTContestedLock(int32 InUnitId, const FString& InResponse, ERTGrammarIntent InIntent)
		: UnitId(InUnitId), Response(InResponse), Intent(InIntent) {}
};


/**
 * L'esito del confronto per un partecipante (§5).
 *
 * ⚠️ **Non e' successo/fallimento**: il confronto produce *Vantaggio A · Pari · Vantaggio B*, ed e' la
 * maneuver a dichiarare cosa significhi ciascuno dei tre. Un tank tollera il `Lose`, un duellante ha `Win`
 * enorme e `Lose` pesante — a parita' di matrice. E' questo che rende diversi i profili.
 */
UENUM()
enum class ERTClashOutcome : uint8
{
	Win,
	Tie,
	Lose
};


/**
 * Cio' che il **reveal** produce, e nient'altro (§7.1).
 *
 * 🔴 **`bRevealed` e' la scadenza fissa, resa una proprieta' dei DATI invece che del tempo.** La regola dice
 * che la finestra dura sempre `FastReactionDuration` e che il reveal avviene **alla scadenza, mai
 * all'arrivo del secondo lock** — perche' anticipare direbbe a ciascuno *quando* l'altro ha deciso, e la
 * latenza di decisione e' una lettura dell'avversario che il gioco non offre.
 *
 * In un resolver che non dipende dal tempo reale quella regola non puo' essere un timer: diventa
 * **«nessun esito e' osservabile finche' non hanno lockato tutti»**. Con lock parziali questa struct e'
 * vuota — non «parzialmente vera» — e non c'e' niente da cui dedurre chi abbia gia' scelto.
 *
 * ⚠️ Il **costo** della regola resta dichiarato e non sparisce: ogni Clash paga 3,0 s pieni di resolution
 * anche quando entrambi lockano subito, ed entra nel budget di §8. Quel costo e' presentazione (CP 14.6);
 * qui vive solo la parte logica, cioe' che l'esito non anticipi.
 */
USTRUCT()
struct FRTClashResolution
{
	GENERATED_BODY()

	/** Falso = non tutti hanno lockato: **niente** e' osservabile, e i due array sono vuoti. */
	UPROPERTY()
	bool bRevealed = false;

	/** I partecipanti in ordine canonico (§7.3), non di arrivo. Parallelo a `Outcomes`. */
	UPROPERTY()
	TArray<int32> UnitIds;

	/** L'esito di ciascuno, nello stesso ordine di `UnitIds`. */
	UPROPERTY()
	TArray<ERTClashOutcome> Outcomes;
};


/**
 * Salute di un bersaglio al micro-step: quanto basta a valutare una condizione dichiarata, e non un byte di
 * piu'.
 *
 * Viaggia in una mappa `UnitId -> vitals` passata al builder invece che dentro `FRTSuppressionMover`: quel
 * tipo descrive «il percorso di un'unita' come lo vede la soppressione», e la soppressione non guarda la
 * salute — aggiungercela lo renderebbe un dato non letto da meta' dei suoi consumatori, che e' la stessa
 * ragione per cui `OwnerCell` sta sul watcher e non nella zona.
 */
USTRUCT()
struct FRTTargetVitals
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Health = 0;

	UPROPERTY()
	int32 MaxHealth = 0;

	FRTTargetVitals() = default;
	FRTTargetVitals(int32 InHealth, int32 InMaxHealth) : Health(InHealth), MaxHealth(InMaxHealth) {}
};

/**
 * Un Overwatch ARMATO (CP 14.4): la zona che controlla, chi la controlla, e l'identita' che decide l'ordine
 * quando piu' reazioni scattano nello stesso micro-step.
 *
 * La zona e' una `FRTSuppressiveZone` — la STESSA che `Action.SuppressiveLine` prepara nel Prep — e non una
 * geometria propria. E' il vincolo esplicito del checkpoint, e la ragione e' che una seconda «Overwatch
 * cone/line» sarebbe subito divergente: chi si copre da un attacco lineare si copre anche dalla soppressione,
 * e dovrebbe coprirsi anche dall'Overwatch. Con due geometrie quella terza proprieta' smetterebbe di valere
 * senza che nessun test la interroghi.
 */
USTRUCT()
struct FRTOverwatchWatcher
{
	GENERATED_BODY()

	/** Le celle controllate, prodotte da `URTOffensiveActionLibrary::MakeSuppressiveZone`. */
	UPROPERTY()
	FRTSuppressiveZone Zone;

	/**
	 * Cella da cui si guarda, per la LOS.
	 *
	 * Sta QUI e non in `FRTSuppressiveZone` di proposito: la soppressione non ha bisogno della cella del
	 * proprietario — la sua linea e' gia' stata tracciata nel Prep e non si rivaluta — mentre l'Overwatch
	 * ricontrolla la vista a ogni micro-step. Aggiungere il campo alla zona lo renderebbe un dato non letto
	 * per meta' dei suoi consumatori.
	 */
	UPROPERTY()
	FRTCellId OwnerCell;

	/** La reaction che l'Overwatch armerebbe (`Action.Overwatch`, o il profilo d'eroe che la specializza). */
	UPROPERTY()
	FName ReactionDefId;

	/**
	 * Quanto sa la SQUADRA del proprietario di ciascun bersaglio (E13, CP 13.1).
	 *
	 * Mappa `UnitId -> ERTAwareness`. Una chiave assente vale `Hidden` (`FindRef` restituisce il valore zero
	 * dell'enum): il default e' fail-closed, cioe' un bersaglio che nessuno ha dichiarato non arma niente.
	 *
	 * E' una PROIEZIONE della conoscenza di squadra, non un secondo calcolo di percezione: l'Overwatch non
	 * deve poter vedere piu' di quanto la sua squadra sappia gia'.
	 */
	UPROPERTY()
	TMap<int32, ERTAwareness> TeamAwareness;

	/** I quattro tie-break di ADR-0004 §4, dopo `ReactionPriority`. Valore minore = risolve prima. */
	UPROPERTY()
	int32 ReactionPriority = 0;

	UPROPERTY()
	int32 AbilityPriority = 0;

	UPROPERTY()
	int32 UnitInitiative = 0;

	UPROPERTY()
	int32 StableUnitId = INDEX_NONE;

	UPROPERTY()
	int32 ReactionInstanceId = INDEX_NONE;

	/** `ReactionStillArmed` della condizione di trigger: una reaction gia' spesa non ne apre altre. */
	UPROPERTY()
	bool bArmed = true;

	/**
	 * La condizione dichiarata in pianificazione da chi ha armato l'Overwatch ([D-109]). Vuota = nessuna.
	 *
	 * Sta sul WATCHER e non nell'opportunity: e' un intento privato di chi arma, e il DTO che raggiunge il
	 * possessore ha un elenco chiuso di campi verificato da `Overwatch.OpportunityLeaksNoFuture`. Le risposte
	 * legali dicono gia' tutto cio' che serve sapere — la condizione ha gia' fatto il suo lavoro nel produrle.
	 */
	UPROPERTY()
	FRTDeclaredCondition DeclaredCondition;

	FRTOverwatchWatcher() = default;
};

/**
 * Cio' che un Overwatch produce a un micro-step: l'opportunity da presentare e i bersagli che la popolano.
 *
 * I bersagli viaggiano ACCANTO al DTO e non dentro: `FRTReactionOpportunity` ha un elenco chiuso di campi
 * (`RefactorTactics.Overwatch.OpportunityLeaksNoFuture`), e allargarlo per comodita' toglierebbe al progetto
 * l'unica barriera che impedisce a un campo di informazione futura di entrare nel DTO. Le risposte legali
 * dicono gia' tutto cio' che il possessore deve sapere.
 */
USTRUCT()
struct FRTOverwatchTrigger
{
	GENERATED_BODY()

	UPROPERTY()
	FRTReactionOpportunity Opportunity;

	/** Bersagli che hanno armato il trigger, in ordine crescente di `UnitId`: uno per ogni risposta `FIRE:`. */
	UPROPERTY()
	TArray<int32> TargetUnitIds;

	FRTOverwatchTrigger() = default;
};


/**
 * L'esito di una finestra: la risposta applicata, e l'esito che spiega da dove viene (CP 14.5).
 *
 * `Response` e' una stringa e non un enum perche' e' un elemento di `AllowedResponses`, che il profilo
 * costruisce: `FIRE:<UnitId>` porta con se' il bersaglio, e un enum costringerebbe a un secondo campo per
 * dirlo — cioe' a due dati che possono contraddirsi.
 *
 * `Outcome` non e' ridondante rispetto a `Response`: la risposta dice **cosa** si applica — ed e' l'unica
 * cosa da cui dipende il determinismo — mentre l'esito dice **come ci si e' arrivati**. `HOLD` scelto e `HOLD`
 * per scadenza applicano lo stesso effetto e raccontano due partite diverse.
 */
USTRUCT()
struct FRTReactionDecision
{
	GENERATED_BODY()

	UPROPERTY()
	FString Response;

	UPROPERTY()
	ERTReactionDecisionOutcome Outcome = ERTReactionDecisionOutcome::HoldTimeout;

	FRTReactionDecision() = default;
	FRTReactionDecision(const FString& InResponse, ERTReactionDecisionOutcome InOutcome)
		: Response(InResponse), Outcome(InOutcome) {}
};

/**
 * Derivazione dell'identita' di una opportunity. Funzioni pure: nessuno stato, nessun accesso al mondo.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionOpportunityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** L'unica condizione ammessa dalla v0.1 ([D-109]): «spara solo se il bersaglio e' a N% di salute o meno». */
	static FName TargetHealthAtOrBelowPercent();

	/**
	 * La condizione e' dichiarabile? Elenco CHIUSO, nel codice.
	 *
	 * Nel dato sarebbe piu' flessibile e sbagliato: dichiarare una condizione inesistente diventerebbe una
	 * modifica al JSON invece di un errore di validazione — la stessa ragione per cui `IsCapabilityAvailable`
	 * tiene il proprio elenco qui. Valida anche il PARAMETRO: una soglia oltre il 100% sarebbe sempre vera,
	 * cioe' una condizione che non condiziona, e il giocatore crederebbe di aver ristretto il fuoco.
	 */
	static bool IsDeclaredConditionAllowed(const FRTDeclaredCondition& Condition);

	/** La risposta che NON spende niente. `Timeout -> HOLD` (ADR-0004 §3) sceglie questa. */
	static const TCHAR* HoldResponse() { return TEXT("HOLD"); }

	/**
	 * Quante volte UNA reaction armata puo' aprire una finestra nello stesso turno (ADR-0004 §8: **3**).
	 *
	 * ⚠️ Serve, e non e' un limite teorico: senza, un bersaglio che percorre cinque celle dentro la zona apre
	 * **cinque** finestre con la stessa Overwatch, perche' `Charges = 1` limita i `FIRE` e non le domande. E'
	 * il numero che la misura di overhead di CP 14.5 ha registrato prima che questo cap esistesse.
	 *
	 * Limita i **prompt**, cioe' le finestre che chiedono davvero qualcosa: un'opportunity a cardinalita' <= 1
	 * si committa da sola senza interrompere nessuno, e contarla spenderebbe il budget del giocatore per una
	 * domanda che non gli e' stata posta.
	 *
	 * Da non confondere col cap AGGREGATO per turno, che ADR-0004 §8 lascia volutamente assente (D-20) e che
	 * [D-050] ha poi risolto altrove col Decision Time Bank — in tempo anziche' in prompt. I due non sono in
	 * contraddizione: questo limita una reaction, quello limita un giocatore.
	 */
	static constexpr int32 MaxPromptsPerReaction() { return 3; }

	/** La risposta che spende la reaction su un bersaglio: `FIRE:<UnitId>`. */
	static FString FireResponse(int32 TargetUnitId);

	/**
	 * Il bersaglio di una risposta `FIRE:<UnitId>`, o `INDEX_NONE` se la stringa non e' un `FIRE`.
	 *
	 * L'inversa di `FireResponse`, e sta accanto a lei di proposito: sono un formato, e un formato con il
	 * parser scritto altrove diverge al primo cambiamento. `HOLD` da' `INDEX_NONE`, che non e' un errore —
	 * e' il fatto che non ci sia un bersaglio.
	 */
	static int32 FireResponseTarget(const FString& Response);

	/**
	 * La risposta e' fra quelle legali per questa opportunity? Confronto esatto, elenco chiuso.
	 *
	 * E' la guardia contro la **risposta stale** che il DoD di E14 richiede: una decisione che arriva per una
	 * finestra gia' chiusa, o che nomina un bersaglio che nel frattempo non e' piu' un'opzione, non deve
	 * potersi applicare. Sta qui e non nell'orchestratore perche' «quali risposte sono legali» e' gia'
	 * responsabilita' di questo tipo — chiederlo altrove sarebbe una seconda regola.
	 */
	static bool IsResponseAllowed(const FRTReactionOpportunity& Opportunity, const FString& Response);

	/**
	 * La decisione allo scadere della finestra: **sempre** `HOLD` (ADR-0004 §3). Funzione PURA.
	 *
	 * Mai `FIRE`, e la ragione e' asimmetrica: `FIRE` consuma una risorsa irreversibile, e un input mancato
	 * non deve spenderla. Il valore non dipende dall'opportunity — il parametro c'e' perche' il chiamante non
	 * debba conoscere la stringa, non perche' serva a calcolarlo.
	 *
	 * ⚠️ Nessun timer qui dentro, e nessuno nel resolver: **quando** la finestra scada e' un fatto
	 * dell'orologio, che vive nell'orchestratore. Questa funzione dice solo *cosa* vale allo scadere, ed e'
	 * per questo che il risultato logico non dipende dal tempo reale.
	 */
	static FRTReactionDecision DecisionOnTimeout(const FRTReactionOpportunity& Opportunity);

	/**
	 * Le opportunity che gli Overwatch armati aprono lungo i micro-step della fase Move (CP 14.4).
	 *
	 * Condizione di trigger, per bersaglio e per micro-step, e sono tutte e quattro necessarie
	 * (ADR-0004 §6): `TargetInsideArea ∧ HasLineOfSight ∧ TargetDetected ∧ ReactionStillArmed`.
	 *
	 * Piu' bersagli nello stesso micro-step danno **una sola** opportunity con piu' risposte
	 * (`FIRE:a` / `FIRE:b` / `HOLD`), mai due prompt in sequenza: prompt sequenziali darebbero un vantaggio
	 * all'ordine di iterazione, che l'invariante #4 vieta (ADR-0004 §4).
	 *
	 * L'esito e' ordinato in modo TOTALE — micro-step, poi `ReactionPriority → AbilityPriority →
	 * UnitInitiative → StableUnitId → ReactionInstanceId` — e non dipende dall'ordine di `Watchers` ne' di
	 * `Movers`: permutarli non cambia il risultato.
	 *
	 * Pura e fail-closed: senza mappa autorevole non c'e' LOS, quindi nessun trigger.
	 */
	/**
	 * `TargetVitals` serve solo alle condizioni dichiarate, ed e' opzionale perche' la stragrande maggioranza
	 * dei watcher non ne ha nessuna. **Fail-closed**: se una condizione e' dichiarata e la salute del bersaglio
	 * non c'e', quel bersaglio non diventa una risposta legale — offrire `FIRE` su una condizione non
	 * verificabile significherebbe sparare a una regola che nessuno ha controllato. E' la stessa scelta che
	 * `TeamAwareness` fa per un bersaglio non dichiarato.
	 */
	/**
	 * `FirstMicroStepIndex` dice a quale micro-step del TURNO corrisponde il passo 0 dei `Movers` (CP 14.5).
	 *
	 * Serve a chi chiama questa funzione **dentro** il ciclo di risoluzione invece che su percorsi interi.
	 * `ARTTurnManager::ResolveMovement` non puo' passarle i percorsi completi: calcolerebbe in anticipo i
	 * trigger di micro-step che non sono ancora avvenuti, e un `FIRE` alla finestra corrente **cambia** il
	 * futuro di chi viene fermato — quei trigger sarebbero il futuro di un mondo che la decisione ha appena
	 * annullato. Passa quindi il solo passo appena compiuto, e con esso l'indice vero.
	 *
	 * Senza il parametro ogni chiamata produrrebbe `MicroStepIndex = 0`, e poiche' l'indice e' uno dei sei
	 * campi di `FRTReactionOpportunityKey`, due passi diversi dello stesso turno collasserebbero sullo stesso
	 * `OpportunityId`: il replay attribuirebbe a uno la decisione dell'altro. E' esattamente il difetto che
	 * `Seq` esiste per impedire, spostato di un campo.
	 *
	 * Il default `0` tiene invariati i chiamanti che passano percorsi interi — i test di CP 14.4 — per cui il
	 * passo 0 dei mover E' il micro-step 0.
	 */
	static TArray<FRTOverwatchTrigger> BuildOverwatchTriggers(const URTHexMapAsset* Map, int32 TurnNumber,
		const TArray<FRTOverwatchWatcher>& Watchers, const TArray<FRTSuppressionMover>& Movers,
		const TMap<int32, FRTTargetVitals>& TargetVitals = TMap<int32, FRTTargetVitals>(),
		int32 FirstMicroStepIndex = 0);

	/**
	 * La condizione e' soddisfatta per questo bersaglio? Funzione pura, valutata al trigger.
	 *
	 * Condizione non dichiarata -> vero: chi non pone condizioni non restringe nulla. Dato mancante o
	 * incoerente (`MaxHealth <= 0`) -> falso, per la ragione fail-closed di sopra.
	 */
	static bool IsConditionSatisfied(const FRTDeclaredCondition& Condition, const FRTTargetVitals* Vitals);

	/**
	 * Vero se questa opportunity richiede un decision boundary; falso se si committa immediatamente.
	 *
	 * E' la regola di ADR-0004 §2 e nient'altro: `AllowedResponses.Num() >= 2`. Sta in una funzione pura, e
	 * non dentro il resolver, perche' il resolver possa chiederla senza che nessuno possa risponderle
	 * diversamente altrove — due punti che decidono «serve una finestra?» sono due regole.
	 */
	static bool RequiresDecisionBoundary(const FRTReactionOpportunity& Opportunity);

	/**
	 * Id stabile e ispezionabile derivato dai sei campi di `Key`.
	 *
	 * Stabile: la stessa chiave da' lo stesso id in ogni esecuzione e in ogni processo — nessun hash di
	 * puntatore, nessun contatore globale, nessun GUID.
	 */
	static FString DeriveOpportunityId(const FRTReactionOpportunityKey& Key);

	// --- E14.7: Reaction Clash ([D-048]) ---------------------------------------------------------------

	/**
	 * Il boundary e' **contested**? Vero quando **due** partecipanti hanno ciascuno almeno due risposte
	 * legali (§3.1).
	 *
	 * ⚠️ **Derivato, mai dichiarato.** Non c'e' un campo da leggere: il criterio e' *quanti partecipanti
	 * hanno una scelta vera*, ed e' la stessa domanda che `RequiresDecisionBoundary` fa per uno solo. Due
	 * partecipanti di cui uno con una risposta obbligata **non** sono un Clash — §3.2 lo dice in negativo,
	 * e senza questo controllo un attacco base diventerebbe un Clash appena qualcuno arma un `Brace`.
	 */
	static bool IsContested(const FRTContestedBoundary& Boundary);

	/**
	 * Quanti partecipanti hanno una scelta vera. E' la misura su cui `IsContested` decide, esposta perche'
	 * un test possa distinguere «uno solo ce l'ha» da «nessuno», che il booleano confonde.
	 */
	static int32 CountParticipantsWithChoice(const FRTContestedBoundary& Boundary);

	/**
	 * L'ordine canonico dei partecipanti (§7.3): `ReactionPriority → AbilityPriority → UnitInitiative →
	 * StableUnitId → ReactionInstanceId`, cioe' l'ordine totale che ADR-0004 §4 ha gia'.
	 *
	 * ⚠️ **Ordina in luogo e NON e' l'ordine di arrivo**: il momento del lock e' precisamente cio' che §7.1
	 * tiene non osservabile. Qui i partecipanti portano solo `UnitId` e la propria opportunity, quindi
	 * l'ordine si riduce al primo criterio disponibile — `UnitId` crescente — che e' `StableUnitId` sotto un
	 * altro nome. Il giorno in cui un partecipante portera' la propria priorita', questa funzione e' il posto
	 * dove aggiungerla: un secondo ordinamento altrove sarebbe una seconda regola.
	 */
	static void SortParticipantsCanonically(FRTContestedBoundary& Boundary);

	/**
	 * La matrice della grammatica: `READ > STAND > SHIFT > READ` ([D-049], §4.1).
	 *
	 * Restituisce `+1` se `A` batte `B`, `-1` se perde, `0` per il pareggio (intenzioni uguali). E' un ciclo
	 * a tre: nessuna intenzione domina, ed e' la proprieta' che rende la scelta una lettura dell'avversario
	 * invece di un calcolo.
	 *
	 * ⚠️ `PROPOSED FOR PLAYTEST`: la **matrice** e' universale e vive qui; i **payoff** per eroe non entrano
	 * nei dati d'eroe finche' il playtest non li promuove (§4.3).
	 */
	static int32 CompareGrammarIntents(ERTGrammarIntent A, ERTGrammarIntent B);

	/**
	 * Il **reveal** di un boundary contested: confronta i lock e produce gli esiti (§7.1, §5).
	 *
	 * 🔴 **Rivela solo quando hanno lockato TUTTI.** Con lock parziali restituisce una risoluzione vuota
	 * (`bRevealed == false`) e non un esito provvisorio: e' cosi' che «il reveal avviene alla scadenza, mai
	 * all'arrivo del secondo lock» diventa una regola verificabile senza un orologio. Un chiamante che
	 * volesse anticipare non ha niente da leggere.
	 *
	 * ⚠️ **L'ordine dei lock in ingresso e' irrilevante per l'esito** — e' l'invariante #4: due esecuzioni
	 * della stessa partita in cui i due giocatori premono in ordine diverso devono dare la stessa traccia.
	 * L'uscita e' in ordine **canonico** (§7.3), mai di arrivo.
	 *
	 * ⛔ Un lock di chi non e' partecipante viene **ignorato**, e uno duplicato non conta due volte: la
	 * cardinalita' del reveal dipende dai partecipanti dichiarati, non da quanti messaggi arrivano.
	 */
	static FRTClashResolution ResolveContestedBoundary(const FRTContestedBoundary& Boundary,
		const TArray<FRTContestedLock>& Locks);

	/**
	 * Chi consuma il costo della propria maneuver, in ordine canonico (§9).
	 *
	 * 🔴 **Si paga al LOCK VALIDO, anche perdendo il Clash.** E' la regola che rende la scelta una scelta:
	 * senza, *«provo comunque, tanto se perdo non costa nulla»* la svuota. Chi perde paga come chi vince, e
	 * il `Lose` non e' un rimborso.
	 *
	 * L'unica eccezione e' quella che §9 lascia **dichiarare alla maneuver** — `bRefundIfLost` — e non e' un
	 * caso particolare del resolver: e' un dato del lock.
	 *
	 * ⚠️ **Solo dopo il reveal.** Con una risoluzione non rivelata restituisce vuoto: consumare prima
	 * direbbe che qualcuno ha lockato, e la scadenza fissa (§7.1) esiste per non dirlo. Il costo e' un
	 * effetto osservabile quanto un esito.
	 */
	static TArray<int32> UnitsConsumingCost(const TArray<FRTContestedLock>& Locks,
		const FRTClashResolution& Resolution);

	/**
	 * Le voci di TurnLog di un boundary contested, nell'ordine in cui vanno scritte (§10).
	 *
	 * Sei tipi di evento — `OpportunityCreated`, `ChoiceLocked` (uno per partecipante), `Revealed`,
	 * `Compared`, `OutcomeResolved` (uno per partecipante), `CostConsumed` — tutti di categoria
	 * `ReactionClash`, con `ERTClashLogEvent` in `Outcome`.
	 *
	 * 🔴 **Restituisce vuoto se il boundary non e' rivelato**, e non e' una scorciatoia: e' §10 che lo
	 * impone — *«in rete, nessun evento di scelta viene pubblicato prima del reveal»*. Se `ChoiceLocked`
	 * fosse scritto all'arrivo del lock, l'ORDINE delle voci direbbe chi ha deciso per primo, cioe' la
	 * latenza di decisione che §7.1 nasconde. Le voci nascono tutte insieme, al reveal, in ordine canonico.
	 *
	 * ⚠️ Il TurnLog canonico **non dipende da timestamp di presentazione**: qui non entra nessun tempo.
	 */
	static TArray<FRTTurnLogEntry> MakeClashLogEntries(const FRTContestedBoundary& Boundary,
		const TArray<FRTContestedLock>& Locks, const FRTClashResolution& Resolution);
};
