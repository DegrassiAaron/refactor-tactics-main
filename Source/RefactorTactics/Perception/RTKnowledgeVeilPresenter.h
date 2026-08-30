#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RTKnowledgeVeilPresenter.generated.h"

class ARTTurnManager;

/**
 * CHI STENDE IL VELO SULLA BOARD, e per conto di quale squadra.
 *
 * ## Perche' appartiene al CLIENT e non alla partita
 *
 * 🔑 **Il viewer e' del giocatore, non del match.** Fino a `E-SOLID` fetta 4 questa logica viveva in
 * `ARTGameMode`, cioe' nell'oggetto che in multiplayer e' **uno solo e sta sul server**: chiedere «di chi e'
 * la vista?» a un oggetto globale e' la domanda sbagliata non appena i client sono due. Il proprietario
 * naturale e' `ARTPlayerController`, che gia' porta `PlayerTeamId` — la stessa fonte che
 * `ARTCameraPawn::FrameOwnTeam` e `CanPlayerControlUnit` leggono. Il velo si aggiunge a quei lettori invece
 * di aprire una terza autorita'.
 *
 * ## Presentation-only, e la riga e' netta
 *
 * ⛔ **Non decide visibilita', non calcola conoscenza, non tocca stato autorevole, non costruisce una
 * seconda simulazione.** Applica alla mappa la vista **gia' prodotta** dal sistema autorevole. Se un giorno
 * il velo dovesse mostrare la vista di un altro, la riga da cambiare e' una e non e' qui.
 *
 * ## 🔴 PRIVACY DI RETE — questo NON e' il disegno finale
 *
 * Oggi il presenter legge `ARTTurnManager::KnowledgeForTeamPublic(ViewerTeamId())`: un oggetto **locale**
 * che, nel vertical slice offline, contiene la conoscenza canonica di **entrambe** le squadre. E'
 * accettabile finche' client e server sono lo stesso processo, e **non lo e' un minuto dopo**.
 *
 * Il disegno finale:
 *
 *     SERVER  conoscenza canonica
 *                │
 *                ├── sanitize squadra 0 ──┐
 *                └── sanitize squadra 1 ──┤
 *                                         ▼
 *                          DTO replicato / RPC al SOLO owner autorizzato
 *                                         ▼
 *                            ARTPlayerController (client)
 *                                         ▼
 *                            URTKnowledgeVeilPresenter
 *
 * ⛔ Il client avversario non deve **mai** ricevere la conoscenza canonica ne' gli intenti privati, e i dati
 * privati non vanno messi su un Actor globale replicato. Cio' che questa fetta compra e' il **posto giusto
 * dove attaccare quel canale**: il presenter e' gia' per-client, quindi la migrazione cambia da dove legge,
 * non chi legge.
 */
UCLASS()
class REFACTORTACTICS_API URTKnowledgeVeilPresenter : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Aggancia il velo al `TurnManager` e lo stende SUBITO.
	 *
	 * 🔴 **Subito, non al primo refresh.** Senza l'applicazione immediata la board nasce interamente
	 * visibile e si vela al primo `RefreshTeamKnowledgeForPlanning`: il primo fotogramma e' quello che
	 * rivela tutta la mappa, ed e' l'unico che nessun test tardivo potrebbe prendere.
	 *
	 * ⚠️ **Chiamarla due volte e' sicuro** — `AddUniqueDynamic` non duplica l'iscrizione — ed e' cio' che la
	 * rende usabile da chi non sa se il ciclo di vita sia gia' passato di qui.
	 */
	void Hook(ARTTurnManager* InTurnManager);

	/**
	 * Stende il velo sulla board secondo la conoscenza di `ViewerTeamId()`.
	 *
	 * Senza mappa o senza turn manager non fa nulla e **non conta**: il contatore deve dire quante volte il
	 * velo e' stato steso, non quante volte qualcuno ci ha provato.
	 */
	void Apply();

	/**
	 * 🔑 **DI CHI E' LA VISTA che il velo disegna: `ARTPlayerController::PlayerTeamId`.**
	 *
	 * Si RILEGGE dall'`Outer` a ogni applicazione e non si copia in un campo: copiarla farebbe di questo
	 * oggetto una seconda sede del valore, che e' esattamente cio' che questa funzione esiste per evitare.
	 *
	 * Ripiega su `0` quando l'`Outer` non e' un controller — headless, harness, test di simulazione — con la
	 * stessa regola di `ARTCameraPawn::FrameOwnTeam`, che il suo test pinna.
	 */
	int32 ViewerTeamId() const;

	/**
	 * 🔑 **Quante volte il velo e' stato steso. Esiste per rendere osservabile l'ANELLO che non lo era.**
	 *
	 * I tre conteggi di `ARTHexMapActor::GetVeilCounts` dicono **come** e' la board, non **quante volte** e'
	 * stata ridipinta — e i due difetti che contano qui producono lo stesso quadro:
	 *
	 *  - il velo steso una volta sola all'aggancio, con l'handler mai invocato;
	 *  - il velo ridipinto a ogni refresh, ma con una conoscenza vuota.
	 *
	 * Entrambi lasciano la board tutta nascosta. Senza un contatore, distinguerli richiede un log — e un log
	 * assente ha due letture, «non eseguito» e «non catturato», che portano a fix opposti.
	 *
	 * ⚠️ **NON e' un contatore di correttezza**: dice che `Apply` e' stata eseguita, non che abbia disegnato
	 * bene. Quella la misura `GetVeilCounts`, che legge le istanze reali. Servono entrambi.
	 */
	int32 GetApplications() const { return Applications; }

protected:
	/**
	 * Il subscriber di `OnTeamKnowledgeRefreshed`.
	 *
	 * ⚠️ **`UFUNCTION` perche' il delegate e' dinamico**, non per esposizione: nessuna categoria, nessun
	 * `BlueprintCallable`. Un Blueprint che potesse invocarlo stenderebbe il velo fuori dai punti di
	 * refresh, che e' precisamente cio' che `Veil.FollowsRefreshPoints` impedisce.
	 */
	UFUNCTION()
	void HandleTeamKnowledgeRefreshed(int32 TurnNumber);

private:
	/**
	 * Da chi arriva la conoscenza. **Weak** perche' il turn manager muore col mondo e il presenter puo'
	 * sopravvivergli di un istante: un puntatore forte lo terrebbe in vita oltre la partita.
	 */
	TWeakObjectPtr<ARTTurnManager> TurnManager;

	/** Vedi `GetApplications()`. */
	int32 Applications = 0;
};
