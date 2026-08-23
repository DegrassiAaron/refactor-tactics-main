#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnRules.h"
#include "RTMatchResultViewModel.generated.h"

/**
 * Cio' che la schermata di fine partita mostra (CP 46.5, #940).
 *
 * ⛔ **LEGGE il risultato canonico, non lo ricalcola.** E' il vincolo centrale del DoD: «un secondo calcolo
 * nel widget sarebbe una seconda autorita' che puo' divergere». La condizione di fine partita e' di E10 e
 * il TurnLog ne e' il registro; qui si traduce soltanto — da `ERTMatchOutcome` a «chi ha vinto», che e' la
 * domanda che il widget pone.
 *
 * ⚠️ **`bHasWinner` e `WinningTeamId` non sono la stessa informazione**, e tenerli separati e' il punto:
 * un `INDEX_NONE` che scivolasse a `0` sarebbe indistinguibile da «vince il team 0». Il pareggio e' un
 * esito legittimo della v0.1 ([D-184]) e la schermata deve saperlo dire.
 *
 * ⚠️ **E `bIsFinished` non e' `!bHasWinner`**: una partita in corso e un pareggio hanno entrambi zero
 * vincitori, e confonderli e' il modo in cui una schermata di fine partita compare a meta' turno.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTMatchResultViewModel
{
	GENERATED_BODY()

	/** L'esito canonico, trasportato senza interpretazione. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;

	/** La via per cui e' finita: distingue un'eliminazione da uno scadere dei round. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	ERTMatchEndReason Reason = ERTMatchEndReason::None;

	/** Round giocati, dallo stato di fine. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	int32 RoundNumber = 0;

	/** `true` se la partita e' conclusa, pareggio compreso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	bool bIsFinished = false;

	/** `true` solo se una squadra ha vinto. Un pareggio e' finito ma senza vincitore. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	bool bHasWinner = false;

	/** Squadra vincitrice, o `INDEX_NONE` se non ce n'e' una. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Frontend")
	int32 WinningTeamId = INDEX_NONE;

	/**
	 * Costruisce la vista dal risultato canonico e dallo stato di fine.
	 *
	 * ⚠️ Prende `FRTMatchState` **solo** per il numero di round: le unita' vive e i punteggi non entrano
	 * nel verdetto, perche' il verdetto e' gia' stato dato. Usarli qui sarebbe il ricalcolo che il DoD vieta.
	 */
	static FRTMatchResultViewModel From(const FRTMatchResult& Result, const FRTMatchState& State)
	{
		FRTMatchResultViewModel VM;
		VM.Outcome = Result.Outcome;
		VM.Reason = Result.Reason;
		VM.RoundNumber = State.RoundNumber;
		VM.bIsFinished = (Result.Outcome != ERTMatchOutcome::InProgress);
		VM.bHasWinner = (Result.Outcome == ERTMatchOutcome::Team0Wins)
			|| (Result.Outcome == ERTMatchOutcome::Team1Wins);
		VM.WinningTeamId = (Result.Outcome == ERTMatchOutcome::Team0Wins) ? 0
			: (Result.Outcome == ERTMatchOutcome::Team1Wins) ? 1
			: INDEX_NONE;
		return VM;
	}
};
