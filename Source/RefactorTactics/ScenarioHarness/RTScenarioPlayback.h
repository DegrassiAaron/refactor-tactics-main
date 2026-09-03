#pragma once

#include "CoreMinimal.h"
#include "Replay/RTReplayStateLibrary.h" // FRTTracedUnitState
#include "ScenarioHarness/RTScenarioDraft.h" // FRTScenarioUnitView

/**
 * Il ponte fra **la traccia** e **le viste d'authoring**: dove disegnare le unita' a un punto del playback
 * (`#1625`).
 *
 * 🔴 **Sta qui e non nel core replay**, ed e' deliberato: `URTReplayStateLibrary` ricostruisce lo stato in
 * `FRTTracedUnitState` — `UnitId`, cella, facing, vivo — e **non conosce** `FRTScenarioUnitView`, che e' un
 * tipo dello Scenario Harness. Farglielo conoscere legherebbe il replay all'authoring per una conversione
 * che serve a un consumatore solo.
 *
 * ⚠️ **Ed e' l'unico posto in cui i DUE SPAZI DI ID si incontrano.** Uno scenario nomina le unita' con una
 * `FString`; il TurnLog con un `int32` che `EnsureMatchRoster` assegna **dopo** aver ordinato il roster.
 * La corrispondenza non si deduce e non si ricalcola: arriva da `FRTScenarioDraft::GetLastRunScenarioIds()`,
 * che la trasporta dall'esecuzione dove e' l'unica cosa che la conosce.
 */
namespace RTScenarioPlayback
{
	/**
	 * Le viste **spostate** dove la traccia dice che le unita' erano, a quel punto.
	 *
	 * `Views` sono le unita' dello scenario com'e' stato scritto — la posa di partenza. `States` sono quelle
	 * stesse unita' dopo che la traccia e' stata applicata fino a un turno e una fase. `ScenarioIdByUnitId`
	 * lega gli id dei secondi alle identita' delle prime.
	 *
	 * ⛔ **Un'unita' abbattuta ESCE dall'elenco**, e qui e' la scelta giusta anche se il core la tiene: la
	 * traccia deve poter dire *«e' caduta»* — e infatti `FRTTracedUnitState::bAlive` esiste — ma una **vista**
	 * e' un marcatore da disegnare, e disegnare un morto sul campo racconterebbe una partita diversa da
	 * quella che si e' giocata. I due tipi rispondono a due domande, e la conversione e' dove la seconda
	 * prende il posto della prima.
	 *
	 * ⚠️ **Cio' che la traccia non nomina resta dov'e'.** Un'unita' che non si e' mai mossa non compare in
	 * nessuna voce, e la sua posa di partenza e' gia' la risposta giusta: saltarla non e' una lacuna.
	 *
	 * 🔴 **Uno stato senza identita' nota viene SCARTATO, non indovinato.** Se `ScenarioIdByUnitId` non
	 * traduce un `UnitId` — mappa vuota perche' non si e' ancora corso, o traccia di un'altra corsa — quella
	 * unita' non si sposta. E' fail-closed: muovere il marcatore sbagliato e' peggio che non muoverne
	 * nessuno, perche' il primo si vede e sembra vero.
	 */
	REFACTORTACTICS_API TArray<FRTScenarioUnitView> ViewsAtTracedStates(
		const TArray<FRTScenarioUnitView>& Views,
		const TArray<FRTTracedUnitState>& States,
		const TMap<int32, FString>& ScenarioIdByUnitId);

	/**
	 * Lo stato **iniziale** da cui far partire la ricostruzione, ricavato dalle viste d'authoring.
	 *
	 * `URTReplayStateLibrary::UnitsAtPosition` ha bisogno di uno schieramento di partenza — la traccia
	 * dichiara i cambiamenti, non le posizioni iniziali — e questa e' la traduzione nell'altro verso.
	 *
	 * ⚠️ **Le viste che la mappa non nomina restano fuori.** Non e' una perdita: un'unita' senza
	 * `StableUnitId` non comparira' mai nella traccia, quindi darle uno stato iniziale non servirebbe a
	 * nulla — e le assegnerebbe un id inventato, che e' il modo in cui i due spazi tornano a confondersi.
	 */
	REFACTORTACTICS_API TArray<FRTTracedUnitState> InitialStatesFromViews(
		const TArray<FRTScenarioUnitView>& Views,
		const TMap<int32, FString>& ScenarioIdByUnitId);
}
