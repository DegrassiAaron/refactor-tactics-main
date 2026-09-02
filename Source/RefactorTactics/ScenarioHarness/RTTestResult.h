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


/**
 * UN CAMPO CHE E' CAMBIATO fra l'ingresso e l'uscita di una run — `#1630`.
 *
 * 🔑 **Porta il valore prima e dopo, non la differenza**: per `Cell` e `Statuses` una sottrazione non
 * esiste, e un diff che mostrasse solo «cambiato» costringerebbe a rileggere la traccia — cioè la cosa che
 * questa slice esiste per evitare.
 */
struct FRTUnitFieldChange
{
	/** Il nome del campo, come si chiama in `FRTUnitStateDigest`: `Health`, `Cell`, `Facing`, … */
	FName Field;

	FString Before;

	FString After;

	FRTUnitFieldChange() = default;
	FRTUnitFieldChange(FName InField, const FString& InBefore, const FString& InAfter)
		: Field(InField), Before(InBefore), After(InAfter) {}
};

/** Come un'unita' compare nel diff: presente in entrambi gli stati, o solo in uno. */
enum class ERTUnitDiffPresence : uint8
{
	/** C'era prima e c'e' dopo: i campi cambiati sono in `Changes`. */
	Present,
	/** Non c'era all'inizio ed e' comparsa. */
	Appeared,
	/** C'era all'inizio e non c'e' piu'. */
	Disappeared
};

/**
 * IL DIFF DI UN'UNITA': solo i campi cambiati, e nient'altro — `#1630`.
 *
 * ⚠️ **Una comparsa o una sparizione NON si rendono come campi cambiati.** Un'unita' che sparisce non e'
 * un `Health` che va a zero: e' un'assenza, e dirla come un campo costringerebbe chi legge a distinguere
 * due cose diverse dallo stesso segno.
 */
struct FRTUnitStateDiff
{
	int32 UnitId = 0;

	ERTUnitDiffPresence Presence = ERTUnitDiffPresence::Present;

	/** Vuoto se niente e' cambiato: un'unita' immobile e intatta non ha righe. */
	TArray<FRTUnitFieldChange> Changes;
};

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

	/**
	 * Tempo **simulato**: gli step di risoluzione eseguiti, moltiplicati per il passo fisso del runner.
	 *
	 * E' **deterministico** — stesso scenario, stesso numero di step, stesso valore — e questa e' l'unica
	 * ragione per cui vale la pena scriverlo accanto a `WallClockSeconds`: da solo direbbe poco, in coppia
	 * distingue due cause che oggi il referto confonde. Un `SimulationSeconds` fermo con un
	 * `WallClockSeconds` che raddoppia dice «la macchina e' carica»; il contrario dice «il gioco ha cambiato
	 * comportamento», ed e' l'unico dei due casi che riguardi il codice.
	 *
	 * 🔴 **Non entra in `StateHash`, nel TurnLog, ne' in alcuna decisione della simulazione.** E' misura, non
	 * ingresso: il giorno in cui una durata decidesse un esito, la suite smetterebbe di essere riproducibile.
	 */
	float SimulationSeconds = 0.f;

	/**
	 * Tempo di **parete**, quello dell'orologio da muro. **Non e' deterministico e non lo diventera'**:
	 * dipende dalla macchina, dal carico e da quante altre sessioni stanno compilando — su questo repository
	 * piu' sessioni condividono la working directory ([D-222](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * ⚠️ Percio' si **registra** ma non si asserisce per uguaglianza: un test che pretendesse due wall-clock
	 * identici sarebbe rosso a giorni alterni per una ragione che non e' il gioco. L'unica delle due durate
	 * su cui si possa scrivere un `TestEqual` e' `SimulationSeconds`.
	 */
	float WallClockSeconds = 0.f;

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

	/**
	 * COSA E' CAMBIATO fra l'ingresso e l'uscita, per unita' — `#1630`.
	 *
	 * 🔑 **Letto, non ricostruito.** I due stati arrivano da `URTMatchStateHashLibrary::BuildUnitDigests`,
	 * la stessa funzione che alimenta il checksum: il diff NON somma gli eventi del TurnLog. La differenza
	 * si vede in un caso preciso — uno stato che cambia senza che una voce di log lo nomini compare
	 * comunque qui, ed e' l'esperimento che il criterio d'accettazione porta con se'.
	 *
	 * ⚠️ Ordinato per `UnitId`: gli stati nascono da una `TMap`, la cui iterazione non e' garantita.
	 * L'hash se ne salva perche' ordina prima di mescolare; un elenco esposto no.
	 *
	 * ⛔ Non entra in `StateHash` ne' nel TurnLog: e' una lettura, e una lettura che cambiasse un esito
	 * sarebbe un secondo calcolo.
	 */
	TArray<FRTUnitStateDiff> StateDiff;

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
