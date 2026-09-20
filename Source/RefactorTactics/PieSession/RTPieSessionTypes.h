#pragma once

// I tipi del conduttore di seduta PIE (#3208).
//
// Il conduttore aggiunge UNA domanda a quelle che l'harness gia' risponde: `ARTGameMode` decide **se**
// questa sessione e' una run di scenario, `FRTScenarioCoordinator` decide **come** si esegue, e il
// conduttore decide **quale, e quando il prossimo**. Non ruba nessuna delle altre due.

#include "CoreMinimal.h"
#include "ScenarioHarness/RTScenarioCoordinator.h" // ERTScenarioStart

#include "RTPieSessionTypes.generated.h"

/**
 * Il verdetto di un passo di seduta.
 *
 * ⛔ **Nessuno dei cinque significa «verde perche' il gate era verde».** L'esito macchina di uno
 * scenario e il giudizio di chi guarda sono due cose diverse, e il progetto ne produce regolarmente la
 * coppia contraddittoria: nella seduta `U54` il feed passava a verde mentre il dock falliva. Se un
 * giorno qualcuno facesse scendere questo campo da `FRTTestResult`, il gate
 * `RefactorTactics.PieSession.VerdictIsNotDeducedFromMachineOutcome` diventa rosso.
 */
UENUM()
enum class ERTPieVerdict : uint8
{
	/** Non ancora giudicato: il passo e' in corso o aspetta chi guarda. */
	Pending,

	/** Lo da' una persona. */
	Pass,

	/** Lo da' una persona. */
	Fail,

	/** Una persona, oppure il conduttore quando la voce non era allestibile. `Reason` obbligatorio nel secondo caso. */
	NotJudgeable,

	/** Solo il conduttore: la sessione esisteva ma la scena non e' mai partita. `Reason` obbligatorio. */
	Blocked,

	/** Solo il conduttore: la seduta e' finita prima di arrivarci. `Reason` obbligatorio. */
	NotRun
};

/** Dove si trova la seduta. Il mondo in `AwaitingVerdict` e' fermo: e' dove il coordinator lascia la scena. */
UENUM()
enum class ERTPieSessionState : uint8
{
	Idle,
	Playing,
	AwaitingVerdict,
	Done
};

/** Un passo: una voce PIE, l'allestimento che la produce, e cosa ne e' venuto fuori. */
USTRUCT()
struct FRTPieSessionStep
{
	GENERATED_BODY()

	/** L'ID della voce nel registro — e' l'ancora con cui chi giudica la ritrova. */
	UPROPERTY()
	FString PieItem;

	/** `INDEX_NONE` = la voce si giudica intera; altrimenti il numero del criterio dichiarato nella cella. */
	UPROPERTY()
	int32 Criterion = INDEX_NONE;

	UPROPERTY()
	FString ScenarioId;

	/**
	 * Dove posare l'occhio, copiato dal `_nota_cosa_guardare` dello scenario. Puo' essere vuoto.
	 *
	 * ⛔ **Non e' l'esito atteso, ed e' la differenza che tiene in piedi la regola**: l'esito atteso vive
	 * in `test-manuali-pie.md` e non si duplica; questa e' un'indicazione dell'allestimento, e vive
	 * accanto all'allestimento — `AreaGuardFromImpactCenter.json` la usa gia' cosi'.
	 */
	UPROPERTY()
	FString WhatToWatch;

	UPROPERTY()
	FString RunId;

	UPROPERTY()
	FString ReportDir;

	/**
	 * L'esito della run, come verbale. Copiarlo non crea una copia che invecchia: una run e' immutabile
	 * e datata, quindi e' un fatto registrato e non uno stato da riallineare.
	 */
	UPROPERTY()
	FString MachineOutcome;

	UPROPERTY()
	ERTPieVerdict Verdict = ERTPieVerdict::Pending;

	/** Obbligatorio per `Blocked`, `NotRun` e per un `NotJudgeable` deciso dal conduttore. */
	UPROPERTY()
	FString Reason;

	UPROPERTY()
	FDateTime At;
};

/**
 * LE DUE PORTE, e perche' il conduttore non conosce il coordinator.
 *
 * `ARTGameMode` possiede `FRTScenarioCoordinator` per valore e privato. Un conduttore che lo chiamasse
 * direttamente dovrebbe romperne l'incapsulamento, e soprattutto renderebbe inscrivibili i propri gate:
 * `FRTScenarioCoordinator` e' una classe concreta, e un test non puo' passarne uno finto.
 *
 * 🔴 **Non sostituirle con un puntatore al GameMode «tanto e' la stessa cosa»**: con un puntatore, i
 * casi limite — scenario non caricabile, sessione in errore, `expect` rosse, interruzione — tornano a
 * richiedere scenari veri e un mondo, e la conduzione smette di avere un oracolo headless.
 */
struct FRTPieSessionPorts
{
	/** Avvia uno scenario. In gioco inoltra a `FRTScenarioCoordinator::Start`; nei test e' una lambda. */
	TFunction<ERTScenarioStart(const FString& ScenarioId)> Launch;

	/** Pulisce prima del passo successivo — sbindatura del decisore inclusa, vedi `RTScenarioSession.h`. */
	TFunction<void()> TearDown;

	bool IsValid() const { return static_cast<bool>(Launch) && static_cast<bool>(TearDown); }
};

/** Il nome del verdetto come finisce nel file di seduta. */
REFACTORTACTICS_API const TCHAR* LexToString(ERTPieVerdict Verdict);
