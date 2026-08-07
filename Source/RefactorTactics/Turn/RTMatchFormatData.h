#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTMatchFormatData.generated.h"

/**
 * Le regole di formato RISOLTE, cioe' quelle realmente in vigore in questa partita.
 *
 * L'asset (`URTMatchFormatData`) e' la SORGENTE, questa struct e' cio' che il `TurnManager` legge: una sola
 * verita' per valore, mai un asset consultato in un punto e una `UPROPERTY` di override in un altro
 * (ADR-0005 §4c: "due sorgenti sarebbero due verita'").
 *
 * Contiene solo parametri di REGOLA (spec-durata-partita-e-scala-mappe.md §16.2): input deterministici da cui
 * l'esito dipende. I tempi di parete (`PlanningSeconds`, `MaxPlaybackSeconds`) NON stanno qui e restano
 * `UPROPERTY` sul `TurnManager` — spostarli senza un consumatore creerebbe la seconda verita' di cui sopra.
 */
USTRUCT(BlueprintType)
struct FRTMatchRules
{
	GENERATED_BODY()

	/**
	 * IDENTITA' stabile del formato in vigore. E' l'unico campo che raggiunge l'header del TurnLog: due
	 * esecuzioni dello stesso scenario con `RoundLimit` diverso producono tracce identiche fino al round in
	 * cui il limite morde, e senza questo marcatore la divergenza verrebbe attribuita al CODICE invece che
	 * alla CONFIGURAZIONE (stessa ragione per cui esiste `ERTLogTopology`).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	FName FormatId;

	/** Numero massimo di round: raggiunto il limite la partita finisce e si confrontano i punteggi. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 RoundLimit = 0;

	/** Punteggio che chiude la partita per obiettivo. **0 = via disattivata** (nessuna soglia in vigore). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ScoreToWin = 0;

	FRTMatchRules() = default;
};

/**
 * Formato di partita: il pacchetto di parametri che governa durata ed esito (2v2 Skirmish, 3v3 Standard...).
 * Asset versionato e confrontabile, come il resto dei dati di gioco (`URTActionData`, `URTHeroData`,
 * `URTHexMapAsset`) — non costanti sparse nell'orchestratore.
 *
 * Decisione di forma: issue #185, riportata in `docs/design/spec-durata-partita-e-scala-mappe.md` §16.1.
 * Entra subito **solo cio' che un checkpoint consuma** (D2): `RoundLimit` e `ScoreToWin`, entrambi letti da
 * CP 10.3. Gli altri parametri della spec (§16) migrano quando avranno un lettore, non prima.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTMatchFormatData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identita' stabile del formato (es. `Format.Skirmish2v2`). Chiave del confronto fra tracce. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	FName FormatId;

	/** Versione del formato dati dell'asset (per migrazioni future), come `URTHexMapAsset::FormatVersion`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 FormatVersion = 1;

	/** Numero massimo di round. Valore iniziale del 2v2 v0.1; banda 10-14, cap 14-16 (spec §6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 RoundLimit = 12;

	/**
	 * Round attesi per una partita di questo formato. **Nessun codice di gioco lo legge**: e' un target di
	 * design (spec §16.2, terza classe) e il suo unico lettore e' il validator, che rifiuta un formato in cui
	 * i round attesi superano il limite — una contraddizione che altrimenti resterebbe silenziosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ExpectedRounds = 12;

	/** Punteggio obiettivo che chiude la partita. 0 = nessuna vittoria per obiettivo in questo formato. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ScoreToWin = 0;
};
