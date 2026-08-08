#pragma once

#include "CoreMinimal.h"
#include "ScenarioHarness/RTTestScenario.h"


/**
 * Esito di una singola assertion. Porta **Expected e Actual**, non solo un booleano: un report che dice
 * «fallita» senza dire cosa si aspettava costringe a rieseguire il test per capire, e a quel punto tanto
 * varrebbe non averlo.
 */
struct FRTAssertionResult
{
	ERTAssertionKind Kind = ERTAssertionKind::UnitAtCell;

	/** Cosa verificava, in una riga: `UnitAtCell(A1)`. */
	FString Description;

	bool bPassed = false;

	/** Valore atteso e valore osservato, gia' formattati per la lettura. */
	FString Expected;
	FString Actual;

	/** Turno in cui l'assertion e' stata valutata (le assertion di fine scenario riportano l'ultimo turno). */
	int32 Turn = 0;
};

/**
 * Risultato completo di una esecuzione. E' cio' che finisce in `result.json` e che Claude Code legge per
 * dire PASS/FAIL e diagnosticare la causa senza aprire migliaia di righe di log Unreal.
 */
struct FRTTestResult
{
	FString ScenarioId;

	/**
	 * `Error` non e' un `Fail`. Fail = la simulazione e' andata a termine e un'aspettativa non e' stata
	 * soddisfatta (difetto del GIOCO). Error = non si e' potuto eseguire (difetto del TEST o dell'ambiente).
	 * Confonderli fa perdere tempo su una regressione che non esiste.
	 */
	ERTTestOutcome Outcome = ERTTestOutcome::Error;

	/** Motivo leggibile, valorizzato solo quando `Outcome == Error`. */
	FString ErrorMessage;

	int32 TurnsPlayed = 0;

	/** Seed dichiarato dallo scenario. Registrato nel report anche se oggi nessun RNG lo consuma. */
	int32 Seed = 0;

	/**
	 * Digest dello stato finale: posizione, salute, scudo ed energia di ogni unità viva.
	 *
	 * Serve al gate di determinismo (CP 12.1): stesso scenario ⇒ stesso hash, su qualunque numero di
	 * ripetizioni. È **permutazione-invariante** per costruzione — le unità si ordinano prima di mescolare —
	 * quindi cambiare l'ordine degli intent nello scenario non deve cambiarlo. Se cambia, l'ordine dell'array
	 * sta decidendo l'esito, che è ciò che l'invariante #3 vieta.
	 *
	 * 0 quando lo scenario non è stato eseguito (`Error`): un hash su nessuno stato sarebbe un numero finto.
	 */
	uint32 StateHash = 0;

	TArray<FRTAssertionResult> Assertions;

	int32 PassedCount() const
	{
		int32 N = 0;
		for (const FRTAssertionResult& A : Assertions) { N += A.bPassed ? 1 : 0; }
		return N;
	}

	int32 FailedCount() const { return Assertions.Num() - PassedCount(); }

	/** Testo breve dell'esito, per log e console. */
	FString OutcomeString() const
	{
		switch (Outcome)
		{
		case ERTTestOutcome::Pass:  return TEXT("PASS");
		case ERTTestOutcome::Fail:  return TEXT("FAIL");
		default:                    return TEXT("ERROR");
		}
	}
};
