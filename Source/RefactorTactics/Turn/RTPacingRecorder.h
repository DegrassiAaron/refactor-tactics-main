#pragma once

#include "CoreMinimal.h"
#include "Turn/RTPacing.h"   // FRTPacingSample, FRTPacingUnitFacts, ERTPlanningInput, ERTLockInSource
#include "Turn/RTTurnLog.h"  // FRTTurnLogEntry
#include "RTPacingRecorder.generated.h"

/**
 * Lo **stato** di un campione di pacing, e il suo ciclo di vita (`#1818`, ottava fetta).
 *
 * 🔑 **Perche' esce da `ARTTurnManager`.** La telemetria di pacing e' il secondo carico che la §Scope di
 * `#1818` indica come estraibile, e la sua ragione e' scritta li': *«non decide nulla, e' l'unico altro
 * carico completamente inerte all'esito»*. Il TurnManager teneva quattro metodi e dieci membri per una cosa
 * che nessuna regola legge — e che, restando dentro, aveva bisogno di un mondo per essere esercitata.
 *
 * ⛔ **Non decide niente, e la forma lo rende vero invece che promesso**: qui non si raggiunge un `ARTUnit`,
 * un `UWorld` o il TurnManager. I fatti sulle unita' arrivano **gia' raccolti** (`FRTPacingUnitFacts`) e la
 * matematica sta in `URTPacingLibrary`, che era gia' pura. Cio' che resta e' la sequenza — apri, annota,
 * chiudi — e il poco stato che la tiene insieme.
 *
 * ⚠️ **La raccolta dei fatti resta nel TurnManager**, ed e' deliberato: `GetAllActorsOfClass` interroga il
 * mondo, e il mondo e' cio' che l'orchestratore possiede. Spostarla qui avrebbe portato dentro la
 * dipendenza che questa fetta esiste per togliere.
 */
USTRUCT()
struct REFACTORTACTICS_API FRTPacingRecorder
{
	GENERATED_BODY()

	/**
	 * Apre il campione del turno. Da qui in poi i tempi hanno un'origine.
	 *
	 * 🔴 **L'origine esiste solo dopo questa chiamata**, ed e' il difetto che `RTPacingIntegrationTests`
	 * documenta: prima, `PlanningStart` vale `0.0` e un tempo calcolato su di essa e' `Now * 1000`, che
	 * sfora l'`int32`. `IsOpen()` e' la guardia, e i chiamanti la interrogano invece di assumerla.
	 */
	void Begin(int32 InTurnNumber, const TArray<FRTPacingUnitFacts>& Facts, int32 PacingTeamId);

	/**
	 * Annota un input di pianificazione: i tempi si aggiornano, i contatori si incrementano.
	 *
	 * ⚠️ **I contatori valgono anche a campione chiuso, i TEMPI no.** Selezioni, ordini e annullamenti sono
	 * conteggi e restano validi comunque; il tempo al primo input ha bisogno di un'origine, e senza campione
	 * aperto non c'e'. La distinzione e' nel corpo, non nel chiamante.
	 */
	void NoteInput(ERTPlanningInput Kind);

	/**
	 * Chiude il campione, lo accoda, e — se `bWriteCsv` — lo scrive in coda al file di sessione.
	 *
	 * `PlaybackSeconds` e `TurnLog` sono gli ultimi due fatti che solo l'orchestratore conosce, e arrivano
	 * come parametri per la stessa ragione dei `Facts`: entrare a prenderli significherebbe conoscerlo.
	 */
	void Close(float PlaybackSeconds, const TArray<FRTPacingUnitFacts>& Facts, int32 PacingTeamId,
		const TArray<FRTTurnLogEntry>& TurnLog, bool bWriteCsv);

	/**
	 * Annota il lock-in: apre il campione se nessuno l'aveva aperto, poi scrive i tre tempi.
	 *
	 * 🔴 **Se l'origine non c'era, i tempi valgono `Unmeasured` e non un numero.** Un turno concluso da
	 * un timeout, o una sessione non presidiata, non ha un umano che ha deciso: misurare `Now - 0.0`
	 * darebbe milioni di millisecondi, e sarebbe un dato **peggiore** dell'assenza — perche' entrerebbe nelle
	 * statistiche. La regola vive qui, col suo stato, invece che in ogni chiamante.
	 */
	void NoteLockIn(bool bUnattended, int32 InTurnNumber, const TArray<FRTPacingUnitFacts>& Facts,
		int32 PacingTeamId);

	/** `true` fra `Begin` e `Close`: i tempi sono misurabili solo qui dentro. */
	bool IsOpen() const { return bOpen; }

	/** I campioni chiusi, in ordine di turno. */
	const TArray<FRTPacingSample>& GetSamples() const { return Samples; }

	/** Il campione in composizione: chi lo annota scrive qui, e solo campi che non decidono nulla. */
	FRTPacingSample& Current() { return CurrentSample; }
	const FRTPacingSample& Current() const { return CurrentSample; }

	/** Il file scritto in questa sessione, vuoto finche' non se ne scrive uno. */
	const FString& GetFilePath() const { return FilePath; }

private:
	/** Accoda una riga al CSV, creando il file e l'intestazione alla prima scrittura. */
	void AppendRow(const FRTPacingSample& Sample);

	UPROPERTY()
	TArray<FRTPacingSample> Samples;

	UPROPERTY()
	FRTPacingSample CurrentSample;

	bool bOpen = false;
	double PlanningStart = 0.0;
	double LastInput = 0.0;
	bool bHadInput = false;
	FString FilePath;
};
