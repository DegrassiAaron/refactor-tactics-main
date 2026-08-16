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
 * Traccia serializzata di UN turno (CP 12.6, #178).
 *
 * Un array per turno e non un buffer unico per la partita: il corpus golden deve poter dire «turno 3», e il
 * `FRTTurnLogEntry` non porta il numero di turno — il TurnLog e' per turno, e aggiungerglielo sarebbe una
 * migrazione di formato per un dato che il chiamante gia' conosce.
 *
 * I byte sono quelli di `URTTurnLogLibrary::SerializeTurnLog`, checksum in coda incluso: e' il «TurnLog **e**
 * checksum» che il DoD chiede di confrontare, e `CompareSerializedTraces` verifica entrambi.
 *
 * Struct C++ semplice come `FRTTestResult` che la contiene: questo header non passa da UHT.
 */
struct FRTTurnTrace
{
	TArray<uint8> Bytes;
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

	/**
	 * Perche' lo scenario si e' fermato, valorizzato solo quando `Outcome == Blocked`: nomina la **capability
	 * mancante** e il turno che la chiedeva. Senza il nome, un BLOCKED direbbe solo «non tutto e' pronto», che
	 * e' esattamente cio' che si sapeva gia'.
	 */
	FString BlockedReason;

	int32 TurnsPlayed = 0;

	/** Seed dichiarato dallo scenario. Registrato nel report anche se oggi nessun RNG lo consuma. */
	int32 Seed = 0;

	/**
	 * Digest dello stato finale: posizione, salute, scudo, energia e stati di ogni unità — **anche di quelle
	 * cadute**, che entrano con `bAlive = false`. Non è un dettaglio: senza, «tre vivi e un caduto» e «tre
	 * vivi e basta» darebbero lo stesso hash ([D-084](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * Serve al gate di determinismo (CP 12.1): stesso scenario ⇒ stesso hash, su qualunque numero di
	 * ripetizioni. È **permutazione-invariante** per costruzione — le unità si ordinano prima di mescolare —
	 * quindi cambiare l'ordine degli intent nello scenario non deve cambiarlo. Se cambia, l'ordine dell'array
	 * sta decidendo l'esito, che è ciò che l'invariante #3 vieta.
	 *
	 * 0 quando lo scenario non è stato eseguito (`Error`): un hash su nessuno stato sarebbe un numero finto.
	 */
	uint32 StateHash = 0;

	/**
	 * Traccia serializzata di ogni turno giocato, nell'ordine in cui sono stati giocati (CP 12.6, #178).
	 *
	 * Vive qui e non in un meccanismo separato perche' il DoD chiede **un solo sistema**: il golden replay
	 * della showcase (#170, CP 15.4) usa questo stesso, non un secondo che gli somiglia.
	 */
	TArray<FRTTurnTrace> TurnTraces;

	/**
	 * Cose accadute durante l'esecuzione che non sono ne' assertion ne' errori, ma senza le quali un
	 * risultato resterebbe muto: un intent che non e' partito perche' il bersaglio era gia' a terra, un
	 * percorso rifiutato dal budget.
	 *
	 * La distinzione con `ErrorMessage` e' chi ha sbagliato: un `Error` dice «lo scenario e' scritto male e
	 * non si e' potuto eseguire», una nota dice «e' andata cosi', ed ecco perche'». Confonderle rimanderebbe
	 * a cercare una regressione dove c'e' solo una partita andata diversamente da come la si immaginava.
	 */
	TArray<FString> Notes;

	/** Quante decisioni scriptate sono state effettivamente APPLICATE a una finestra. */
	int32 ScriptedDecisionsApplied = 0;

	/**
	 * Quante sono state dichiarate e non hanno mai trovato una finestra. ⚠️ Diverso da zero e' un
	 * FALLIMENTO (task 6), non un avanzo: descrive qualcosa che non e' successo.
	 */
	int32 ScriptedDecisionsUnused = 0;

	/**
	 * L'ultimo token restituito dal decisore scriptato, per come il gioco lo ha ricevuto — `HOLD` o
	 * `FIRE:<StableUnitId>`. Serve a verificare la **traduzione**, che e' l'unica parte che lo scenario non
	 * poteva esprimere: senza, si potrebbe solo osservare che «qualcosa e' successo».
	 */
	FString LastScriptedResponse;

	/**
	 * Chi ha risposto alle finestre: `scenario`, `test-override`, `none`.
	 *
	 * ⚠️ Sta QUI e non nel TurnLog, ed e' una scelta: al replay serve **quale** decisione — quella e' stato
	 * di gioco e c'e' gia' — non **chi** l'ha fornita, che e' diagnostica. Un campo nuovo in
	 * `FRTTurnLogEntry` muoverebbe i golden per un dato che il replay non legge.
	 */
	FString DecisionSource = TEXT("none");

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
		case ERTTestOutcome::Pass:    return TEXT("PASS");
		case ERTTestOutcome::Fail:    return TEXT("FAIL");
		case ERTTestOutcome::Blocked: return TEXT("BLOCKED");
		default:                      return TEXT("ERROR");
		}
	}
};
