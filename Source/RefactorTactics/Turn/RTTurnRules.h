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

/**
 * Cosa e' successo all'obiettivo contendibile alla fine di un Cleanup (CP 10.2, #75).
 *
 * ⚠️ **Enum PROPRIO e categoria propria nel TurnLog** ([D-162]): `FRTTurnLogEntry::Outcome` e' un `uint8` il
 * cui significato lo decide la categoria, e nessuno degli enum esistenti sa dire «contestato». `Combat` dice
 * `Hit`/`Lethal`, `Environment` dice `CoverDestroyed`/`DoorOpened`: un punto d'obiettivo non e' ne' l'uno ne'
 * l'altro. La categoria si aggiunge **in coda**, quindi la versione del formato del TurnLog non cambia.
 *
 * ⚠️ **La squadra sta qui e non in `UnitId`**, e non e' una scorciatoia: il punto lo fa la SQUADRA, non
 * un'unita' — chi occupa l'obiettivo puo' essere una unita' diversa a ogni turno, e sceglierne una sarebbe
 * inventare un soggetto. `UnitId` resta `0` come vuole [D-063] per le voci senza unita'.
 */
UENUM(BlueprintType)
enum class ERTObjectiveOutcome : uint8
{
	/** Nessuno era presente: l'obiettivo non e' di nessuno e nessun punteggio si muove. */
	Unclaimed,
	/**
	 * Presenza PARITARIA: nessun progresso per nessuno.
	 *
	 * ⚠️ E' una voce che si scrive anche quando non succede niente, ed e' voluto: senza, un turno conteso e
	 * un turno vuoto sarebbero indistinguibili nel log — e la contesa e' precisamente cio' che questo
	 * checkpoint aggiunge alla partita.
	 */
	Contested,
	/** La squadra 0 controllava l'obiettivo da sola: `Amount` porta i punti assegnati. */
	Team0Scores,
	/** La squadra 1 controllava l'obiettivo da sola: `Amount` porta i punti assegnati. */
	Team1Scores
};

UCLASS()
class REFACTORTACTICS_API URTTurnRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * **Quante squadre esistono in una partita: DUE, e non e' una costante di comodo.**
	 *
	 * E' il numero che questa stessa libreria ha gia' cablato nelle proprie firme — `EvaluateOutcome(int32
	 * Team0Alive, int32 Team1Alive)`, `ResolveObjectiveControl(int32 Team0Present, int32 Team1Present)` — e
	 * che `ERTMatchOutcome` enumera con `Team0Wins`/`Team1Wins`. Chi decide chi vince conosce due squadre:
	 * questa riga lo DICHIARA invece di lasciarlo dedurre da un nome di parametro.
	 *
	 * 🔴 **Perche' esiste** (#1515): una terza squadra non produce un errore, produce un **esito che nessuno
	 * puo' riprodurre**. `ARTTurnManager::GetTeamScore` risponde `0` a ogni `TeamId` diverso da 0 e 1, e
	 * l'hash di stato si costruisce da `GetTeamScore(0)` e `GetTeamScore(1)` soltanto: uno scenario con
	 * `"team": 7` gira, dichiara un vincitore, e il punteggio di quella squadra resta **fuori dall'hash di
	 * determinismo** e fuori dal giudizio di fine partita.
	 *
	 * ⛔ **Non e' `FRTKnowledgeVerdict::MaxTeamId`**, che vale 31 ed e' la capacita' di una **bitmask di
	 * percezione**: quanti indici quella maschera sa rappresentare, non quante squadre giocano. Derivare il
	 * vincolo da li' ammetterebbe `"team": 7` — cioe' proprio il caso rotto.
	 *
	 * ⚠️ **Non e' `FRTMatchRules::UnitsPerTeam`**: quello dice quante unita' schiera una squadra, non quante
	 * squadre ci sono. Il formato competitivo aperto (3v3 vs 4v4, `OPEN_DECISIONS`) muove le UNITA', non il
	 * numero di squadre — che resta due in ogni formato oggi dichiarato.
	 *
	 * 🔔 Il giorno in cui una partita a tre squadre diventi reale, questa costante e' il punto da cui
	 * partire, e i suoi consumatori si trovano cercando questo simbolo: `GetTeamScore`, `EvaluateOutcome`,
	 * `CompareScores` e l'hash di stato vanno rifatti insieme, non uno per volta.
	 */
	static constexpr int32 NumTeams = 2;

	/**
	 * Se `TeamId` e' una squadra che questa partita conosce. Vedi `NumTeams` per il perche' del vincolo.
	 *
	 * Non e' `UFUNCTION`: e' una funzione pura su un intero, usata dal validator dello Scenario Harness e
	 * dai test, e non ha bisogno di attraversare la reflection.
	 */
	static bool IsValidTeamId(int32 TeamId) { return TeamId >= 0 && TeamId < NumTeams; }

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

	/**
	 * Chi controlla l'obiettivo contendibile, dato quante unita' VIVE di ciascuna squadra lo occupano
	 * (CP 10.2). Pura: nessun Actor, nessun World, nessun tempo — si verifica headless.
	 *
	 * La regola in una riga: **controlla chi e' presente da solo**. Presenza paritaria — inclusi due zeri —
	 * non muove niente, ed e' la meta' della DoD che rende l'obiettivo *contendibile* invece che *occupabile*:
	 * arrivare in tempo per pareggiare la presenza basta a negare il punto, senza sparare un colpo.
	 *
	 * ⚠️ **Conta le unita', non le azioni**: un'unita' che ha speso il turno in `Action.Wait` contende
	 * esattamente come una che ha attaccato. E' la prima riga della DoD di #75, ed e' anche la ragione per cui
	 * questa funzione non vede ne' intenti ne' piani.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	static ERTObjectiveOutcome ResolveObjectiveControl(int32 Team0Present, int32 Team1Present);
};
