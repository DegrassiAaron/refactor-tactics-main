#pragma once

// Il conduttore di seduta PIE: possiede la coda, il passo corrente e i verdetti (#3208).
//
// Non sa come si allestisce uno scenario ne' cosa c'e' a schermo. Parla con due porte, e questo e' cio'
// che rende la conduzione verificabile senza aprire l'Editor — in un progetto dove le verifiche PIE
// costano una persona a schermo, la parte che le automatizza non puo' essere a sua volta giudicabile
// solo a schermo.

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PieSession/RTPieSessionTypes.h"

#include "RTPieSessionSubsystem.generated.h"

struct FRTTestResult;

UCLASS()
class REFACTORTACTICS_API URTPieSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Chiude una seduta rimasta aperta quando la GameInstance muore — lo Stop di PIE, tipicamente. */
	virtual void Deinitialize() override;

	/**
	 * Apre una seduta e avvia il primo passo che si riesce ad allestire.
	 *
	 * I passi che non si allestiscono non fermano la seduta: si chiudono `NotJudgeable` col motivo e si
	 * prosegue. Fermarsi al primo buco costringerebbe a riaprire l'Editor per le voci a valle, che e'
	 * esattamente il costo che questo sottosistema esiste per togliere.
	 */
	void Begin(TArray<FRTPieSessionStep> InSteps, FRTPieSessionPorts InPorts);

	/**
	 * Installa le porte senza aprire una seduta.
	 *
	 * Serve perche' chi POSSIEDE le porte (il GameMode, che ha il coordinator) e chi APRE la seduta (il
	 * comando console) sono due soggetti diversi: il primo le installa al `BeginPlay`, il secondo chiama
	 * `Begin` senza doverle conoscere. Le riscrive a ogni partita, perche' puntano a un Actor che non
	 * sopravvive al cambio di livello mentre questo subsystem si'.
	 */
	void SetPorts(FRTPieSessionPorts InPorts) { Ports = MoveTemp(InPorts); }

	/** Apre una seduta con le porte gia' installate. Non fa nulla se non ce ne sono. */
	void Begin(TArray<FRTPieSessionStep> InSteps);

	bool HasPorts() const { return Ports.IsValid(); }

	/** Lo scenario del passo corrente e' finito: da qui si chiede il verdetto, o lo si scrive da soli. */
	void OnScenarioFinished(const FRTTestResult& Result, const FString& ReportDir);

	/** Il verdetto di chi guarda. Chiude il passo, pulisce, e avvia il successivo. */
	void SubmitVerdict(ERTPieVerdict Verdict, const FString& Reason);

	/** Interrompe: i verdetti gia' dati restano, il resto diventa `NotRun`. Non si perde nulla di giudicato. */
	void Abort();

	ERTPieSessionState State() const { return SessionState; }
	const TArray<FRTPieSessionStep>& Steps() const { return SessionSteps; }
	const FString& SessionId() const { return CurrentSessionId; }

	const FRTPieSessionStep* CurrentStep() const
	{
		return SessionSteps.IsValidIndex(Cursor) ? &SessionSteps[Cursor] : nullptr;
	}

	/**
	 * Vero mentre una seduta e' in corso. E' la **quarta sorgente** di `ARTGameMode::ResolveScenarioToRun`,
	 * e vince sulle altre tre: una CVar `rt.Test.Scenario` rimasta impostata da una prova precedente
	 * dirotterebbe in silenzio ogni passo, e chi guarda crederebbe di giudicare la voce annunciata.
	 */
	bool IsConducting() const
	{
		return SessionState != ERTPieSessionState::Idle && SessionState != ERTPieSessionState::Done;
	}

	/** Dove e' finito il file di seduta, una volta chiusa. Vuoto se non e' stato scritto. */
	const FString& WrittenSessionFile() const { return SessionFilePath; }

	/**
	 * Lo scenario che la seduta impone a `ARTGameMode::ResolveScenarioToRun`, o **vuoto** se non ne
	 * impone nessuno — conduttore assente, seduta mai aperta, seduta finita.
	 *
	 * 🔑 Statica e tollerante al `nullptr` perche' la precedenza abbia un gate **senza** costruire una
	 * `UGameInstance`: il mondo dei test non ne ha una, e un `GetSubsystem` li' dentro restituirebbe
	 * sempre null — un test che non puo' distinguere «nessuna seduta» da «non ho potuto chiedere» non
	 * verificherebbe la precedenza, la assumerebbe.
	 */
	static FString ScenarioImposedBy(const URTPieSessionSubsystem* Conduttore);

private:
	/** Avvia il passo corrente, saltando e chiudendo quelli che non si allestiscono. */
	void LaunchCurrent();

	/** Chiude la seduta e scrive il file. Idempotente. */
	void Finish();

	TArray<FRTPieSessionStep> SessionSteps;
	FRTPieSessionPorts Ports;
	FString CurrentSessionId;
	FString SessionFilePath;
	int32 Cursor = INDEX_NONE;
	ERTPieSessionState SessionState = ERTPieSessionState::Idle;
};
