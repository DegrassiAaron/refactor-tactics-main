#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTFrontendGameMode.generated.h"

/**
 * Il GameMode della **mappa del frontend** (CP 46.3, `#938`): l'unica cosa che apre il menu.
 *
 * ## Perche' esiste
 *
 * Fino a `18b3f105` `URTFrontendNavigator::InitializeFrontend` non era chiamata da nessuna parte fuori dai
 * test. Il navigatore c'era, le sue tre schermate base c'erano, i cinque `WBP_RT_*` erano versionati — e
 * niente li portava a schermo. Questa classe e' quel niente.
 *
 * ## Perche' un GameMode, e non la `GameInstance`
 *
 * ⚠️ E' la domanda che si ripresenta ogni volta che si guarda questo file, quindi sta scritta qui.
 * `URTFrontendNavigator` e' un `UGameInstanceSubsystem` **perche' deve sopravvivere al cambio di livello**:
 * `Main Menu -> partita -> Main Menu` e' il ciclo di E46, e un owner del flow che morisse col livello
 * perderebbe lo stack proprio quando serve a tornare indietro.
 *
 * Ma la stessa proprieta' gli impedisce di aprirsi da solo: nascendo con la `GameInstance`, cioe' su
 * **ogni** mappa, un auto-avvio metterebbe il Main Menu sopra una partita in corso. Il GameMode invece
 * appartiene alla mappa, quindi il frontend si apre esattamente dove quella mappa viene caricata — e si
 * riapre al ritorno dal match, che e' cio' che serve a CP 46.5.
 *
 * ## Cosa NON fa
 *
 * - **Non allestisce una partita.** Non e' `ARTGameMode` e non ne condivide niente: qui non ci sono unita',
 *   scenari, resolver o TurnLog. La mappa del frontend non e' un campo di battaglia.
 * - **Non decide quale schermata aprire.** La radice e' `RTScreenIds::Main` e la sceglie
 *   `URTFrontendNavigator::StartFrontend`, che e' l'unico owner del flow (invariante 1 di CP 46.1).
 */
UCLASS()
class REFACTORTACTICS_API ARTFrontendGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/**
	 * Avvia il frontend per questa partita. `true` se il menu si e' aperto.
	 *
	 * ⚠️ **Restituisce `bool` e non `ERTNavResult`, ed e' una scelta.** I due modi in cui questa funzione
	 * fallisce — nessuna `GameInstance`, nessun navigatore — non sono transizioni di navigazione: non c'e'
	 * il navigatore che le produrrebbe. Riusare quell'enum costringerebbe a scegliere un valore che parla
	 * d'altro (`InvalidScreen` descrive un nome vuoto), e un motivo sbagliato e' peggio di nessun motivo.
	 * La causa va in `LogRT` come errore, che e' il canale giusto per un guasto d'ambiente.
	 *
	 * ⚠️ **Ed e' pubblica invece di vivere dentro `BeginPlay`** perche' altrimenti sarebbe verificabile solo
	 * in PIE. Cosi' un test costruisce un mondo, ci attacca una `GameInstance` e chiede a questa funzione
	 * cio' che il DoD chiede alla mappa. Quello che resta non coperto e' la sola riga di dispatch — che
	 * `BeginPlay` la chiami — e resta di `PIE-V01-FRONTEND-MAIN`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	bool StartFrontendForThisGame();

protected:
	virtual void BeginPlay() override;
};
