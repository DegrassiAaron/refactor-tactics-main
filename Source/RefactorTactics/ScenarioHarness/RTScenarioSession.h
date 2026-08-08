#pragma once

#include "CoreMinimal.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"

class UWorld;
class ARTUnit;
class ARTTurnManager;
class URTHexMapAsset;

/**
 * Esecuzione di uno scenario come **macchina a stati**, avanzabile un passo alla volta.
 *
 * Esiste per una ragione misurata: risolvendo tutti i turni dentro `BeginPlay`, lo scenario finiva *prima
 * del primo fotogramma* e non si vedeva giocare. Con la sessione, chi guarda puo' farla avanzare **un passo
 * per frame** e vedere il playback scorrere; chi verifica in automatico la fa girare in un ciclo stretto e
 * ottiene lo stesso identico esito.
 *
 * Le due strade guidano la **stessa** sessione, non due copie della stessa logica: se divergessero, un test
 * verde non direbbe piu' niente su quel che si vede a schermo.
 *
 * Il turn manager e il resolver restano ignari: la sessione entra dagli stessi ingressi del giocatore
 * (piani sulle unita', `LockInAndResolve`).
 */
class REFACTORTACTICS_API FRTScenarioSession
{
public:
	/**
	 * Pausa in secondi **prima** di risolvere ogni turno: il tempo per guardare dove sono le unita' prima che
	 * si muovano. 0 = nessuna pausa (e' il valore delle esecuzioni headless, dove non c'e' nessuno a guardare).
	 */
	float TurnPauseSeconds = 0.f;

	/**
	 * Allestisce il mondo: arena, unita' dal catalogo, turn manager.
	 *
	 * @return false se lo scenario non e' eseguibile — l'esito e' gia' un `Error` con il motivo in
	 *         `GetResult()`. Non lancia mai: un harness che crasha non sa dire perche'.
	 */
	bool Start(UWorld* World, const FRTTestScenario& Scenario);

	/**
	 * Avanza di un passo.
	 *
	 * @param DeltaSeconds tempo trascorso: scandisce le pause e, in modalita' pompata, il playback.
	 * @param bPumpTurnManager **true** quando nessuno sta ticcando il mondo (test headless: il turn manager va
	 *        fatto avanzare a mano); **false** in gioco, dove il mondo lo ticca gia' e pomparlo lo farebbe
	 *        correre al doppio della velocita'.
	 */
	void Step(float DeltaSeconds, bool bPumpTurnManager);

	/** Vero quando non c'e' piu' niente da fare: assertion valutate ed esito definitivo. */
	bool IsFinished() const { return State == EState::Finished; }

	const FRTTestResult& GetResult() const { return Result; }

	/** Turno corrente (1-based) mentre gira, per la diagnostica a schermo. 0 = non ancora partito. */
	int32 GetCurrentTurn() const { return TurnIndex + 1; }

private:
	enum class EState : uint8
	{
		NotStarted,
		/** In attesa prima di risolvere il turno: e' la finestra in cui si guarda il campo. */
		PauseBeforeTurn,
		/** Il turn manager sta risolvendo: qui scorre il playback, ed e' cio' che si vede. */
		Resolving,
		Finished
	};

	/** Scrive i piani del turno sulle unita', come farebbe il giocatore, e chiude la pianificazione. */
	void BeginTurn();

	/** Azzera i piani, calcola l'hash dello stato e valuta le assertion. */
	void Finish();

	EState State = EState::NotStarted;
	FRTTestScenario Scenario;
	FRTTestResult Result;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<ARTTurnManager> TurnManager;
	TObjectPtr<URTHexMapAsset> Map;
	TMap<FString, TWeakObjectPtr<ARTUnit>> UnitsById;

	int32 TurnIndex = 0;
	float PauseElapsed = 0.f;

	/**
	 * Perche' la sessione si e' fermata prima della fine, se si e' fermata: nomina la **capability mancante**
	 * e il turno che la chiedeva. Vuoto = lo scenario e' arrivato in fondo.
	 */
	FString BlockedBy;

	/**
	 * Errore di SCRITTURA dello scenario scoperto durante l'esecuzione — tipicamente un'abilita' che l'eroe
	 * non possiede. Vuoto = nessuno.
	 *
	 * Esiste separato da `BlockedBy` perche' ha precedenza su tutto: uno scenario scritto male non produce un
	 * verdetto sul gioco. Se cadesse come `Fail`, il report direbbe «regressione» per un errore di battitura.
	 */
	FString ErroredBy;

	/** Vedi `FRTTestResult::Notes`: cio' che e' successo e che spiega un risultato altrimenti muto. */
	TArray<FString> Notes;

	/** Tetto di sicurezza sulla risoluzione di UN turno: fallire e' meglio che girare all'infinito. */
	int32 ResolveTicks = 0;
};
