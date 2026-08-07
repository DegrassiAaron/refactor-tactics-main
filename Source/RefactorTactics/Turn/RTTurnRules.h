#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTMatchFormatData.h"
#include "RTTurnRules.generated.h"

/** Fasi di un turno (risoluzione a fasi con priorita' fissa). */
UENUM(BlueprintType)
enum class ERTMatchPhase : uint8
{
	Planning,
	Prep,
	Dash,
	Blast,
	Move,
	Cleanup,
	MatchEnded
};

/** Esito della partita in base alle unita' vive per squadra. */
UENUM(BlueprintType)
enum class ERTMatchOutcome : uint8
{
	InProgress,
	Team0Wins,
	Team1Wins,
	Draw
};

/**
 * VIA per cui la partita e' finita. Separata dall'esito perche' l'esito dice CHI ha vinto e la via dice
 * PERCHE': "vince il team 0" e' la stessa parola per un'eliminazione e per un vantaggio di un punto allo
 * scadere, e senza la via ne' il log ne' l'HUD potrebbero distinguerle (DoD di CP 10.3).
 */
UENUM(BlueprintType)
enum class ERTMatchEndReason : uint8
{
	/** La partita non e' finita. */
	None,
	/** Una squadra (o entrambe) e' rimasta senza unita' vive. */
	Elimination,
	/** Una squadra ha raggiunto la soglia di punteggio del formato. */
	Objective,
	/** Il `RoundLimit` del formato e' stato raggiunto: decide il confronto dei punteggi. */
	RoundLimit
};

/**
 * Fotografia dello stato di partita al momento della valutazione (fine Cleanup). Solo interi: nessun Actor,
 * nessun puntatore, nessun tempo di parete — cosi' la regola resta pura e riproducibile in un test.
 */
USTRUCT(BlueprintType)
struct FRTMatchState
{
	GENERATED_BODY()

	/** Unita' vive della squadra 0 dopo il Cleanup. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 Team0Alive = 0;

	/** Unita' vive della squadra 1 dopo il Cleanup. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 Team1Alive = 0;

	/** Progresso obiettivo della squadra 0. Intero, mai un float (DoD di CP 10.2). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 Team0Score = 0;

	/** Progresso obiettivo della squadra 1. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 Team1Score = 0;

	/** Round in corso, 1-based (nel codice storico si chiama "turno": glossario della spec §0). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 RoundNumber = 1;

	FRTMatchState() = default;
};

/** Esito della partita e via che l'ha determinato. `InProgress` implica `None` e viceversa. */
USTRUCT(BlueprintType)
struct FRTMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	ERTMatchEndReason Reason = ERTMatchEndReason::None;

	FRTMatchResult() = default;
	FRTMatchResult(ERTMatchOutcome InOutcome, ERTMatchEndReason InReason)
		: Outcome(InOutcome), Reason(InReason) {}
};

UCLASS()
class REFACTORTACTICS_API URTTurnRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Fase successiva nel ciclo Planning -> Prep -> Dash -> Blast -> Move -> Cleanup -> Planning.
	 * MatchEnded e' assorbente (resta MatchEnded).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	static ERTMatchPhase NextPhase(ERTMatchPhase Phase);

	/**
	 * Esito dato il numero di unita' vive per squadra.
	 * Una squadra senza unita' vive perde; se entrambe sono a zero e' pareggio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	static ERTMatchOutcome EvaluateOutcome(int32 Team0Alive, int32 Team1Alive);

	/**
	 * Fine partita a TRE VIE, valutata nel Cleanup (CP 10.3). Pura e deterministica: stesso stato + stesse
	 * regole = stesso esito, sempre.
	 *
	 * Precedenza fissata da `spec-durata-partita-e-scala-mappe.md` §12:
	 *   1. **eliminazione** — la victory condition si valuta per prima;
	 *   2. **obiettivo** — soglia `ScoreToWin` raggiunta (0 = via disattivata);
	 *   3. **`RoundLimit`** — confronto dei punteggi; **parita' = pareggio dichiarato**, mai un vincitore
	 *      scelto per posizione, e nessun overtime (fuori scope v0.1).
	 *
	 * L'ordine non e' un dettaglio: senza una precedenza fissa, una squadra azzerata nello stesso Cleanup in
	 * cui l'altra tocca la soglia darebbe un esito dipendente dall'ordine dei controlli.
	 */
	static FRTMatchResult EvaluateMatchEnd(const FRTMatchState& State, const FRTMatchRules& Rules);

	/**
	 * Testo leggibile dell'esito ("Vince il team 0 (blu)"). UNICA fonte per il combat log e per l'HUD: due
	 * formulazioni diverse per lo stesso esito sarebbero due verita' per chi legge.
	 */
	static FString DescribeOutcome(ERTMatchOutcome Outcome);

	/** Testo leggibile della via che ha chiuso la partita ("per eliminazione"). */
	static FString DescribeEndReason(ERTMatchEndReason Reason);
};
