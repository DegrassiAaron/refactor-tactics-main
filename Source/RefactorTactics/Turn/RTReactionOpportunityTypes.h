#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Combat/RTOffensiveActionLibrary.h" // FRTSuppressiveZone, FRTSuppressionMover: la geometria e' UNA
#include "Perception/RTPerceptionLibrary.h"  // ERTAwareness: il trigger richiede `Detected`, non «visibile»
#include "Turn/RTDeclaredCondition.h" // FRTDeclaredCondition: header leggero, lo usa anche ARTUnit
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

	/** La risposta che spende la reaction su un bersaglio: `FIRE:<UnitId>`. */
	static FString FireResponse(int32 TargetUnitId);

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
	static TArray<FRTOverwatchTrigger> BuildOverwatchTriggers(const URTHexMapAsset* Map, int32 TurnNumber,
		const TArray<FRTOverwatchWatcher>& Watchers, const TArray<FRTSuppressionMover>& Movers,
		const TMap<int32, FRTTargetVitals>& TargetVitals = TMap<int32, FRTTargetVitals>());

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
};
