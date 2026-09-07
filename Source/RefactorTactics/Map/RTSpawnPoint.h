#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/RTCellId.h"

#include "RTSpawnPoint.generated.h"

class ARTHexMapActor;
class UBillboardComponent;
class URTHexMapAsset;

/**
 * DOVE una squadra parte, dichiarato da un autore invece che dedotto dalla geometria.
 *
 * 🔑 **Perche' esiste.** Oggi le celle di partenza non sono un dato: `URTMatchSetupLibrary::PickStartCells`
 * le DERIVA dalle celle percorribili in ordine stabile, prendendo le due estremita' dell'ordine. E'
 * riproducibile e non dipende da RNG, ma ha una conseguenza che `URTHexLibrary::SpawnTeam0Color` dichiara
 * per iscritto: le partenze «si spostano da sole appena si aggiunge o toglie una cella». Su una mappa
 * d'autore duplicata da un template quella non e' una proprieta' desiderabile — chi disegna l'arena decide
 * da dove si parte, e non vuole che una cella dipinta in un angolo sposti lo schieramento.
 *
 * ⚠️ **Non sostituisce `PickStartCells`, e questa e' la parte da non fraintendere.** La derivazione resta il
 * comportamento di una mappa che non dichiara nulla, cioe' di tutte quelle che esistono oggi. Questo attore
 * e' il PRODUTTORE del dato autorato; il consumatore — la precedenza «spawn autorati, altrimenti derivati»
 * dentro l'allestimento — e' Fase B e non e' scritto qui. Finche' non lo e', questi attori sono authoring
 * che nessuna partita legge: dichiarato, non nascosto.
 *
 * ## 🔴 Una sola fonte di verita': la POSIZIONE
 *
 * ⛔ **Non esiste un `FRTCellId` editabile su questo attore.** Sarebbe la seconda verita' sullo stesso fatto:
 * un autore trascina il marker nel viewport e la cella scritta a mano resta indietro, oppure edita la cella e
 * il marker resta dov'era — e nessuno dei due casi ha un modo di accorgersene. La cella si CHIEDE, con
 * `ResolveCell`, e passa dallo stesso `GetHexContext` che usano risoluzione, playback e input del giocatore.
 *
 * ∴ spostare il marker cambia la cella, e non c'e' niente da tenere sincronizzato.
 *
 * ## Confini
 *
 * ⛔ Nessuna regola di gioco: non spawna niente, non legge snapshot, non tocca `TurnLog` ne' `StateHash`.
 * ⛔ Nessuna presentazione: non porta luci, mesh di scena ne' materiali. Il billboard e' editor-only.
 * ⛔ Non e' un `APlayerStart`: quello colloca un pawn in coordinate mondo, questo NOMINA uno slot tattico.
 */
UCLASS(Blueprintable, meta = (DisplayName = "RT Tactical Spawn Point"))
class REFACTORTACTICS_API ARTSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ARTSpawnPoint();

	/**
	 * La squadra che parte da qui. Intero e non enum: `FRTShowcaseSpawn::TeamId` e `ARTUnit` usano gia'
	 * `int32`, e introdurre qui un vocabolario diverso creerebbe due modi di dire la stessa squadra.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Spawn", meta = (ClampMin = "0"))
	int32 TeamId = 0;

	/**
	 * QUALE dei posti di quella squadra. La coppia `(TeamId, SlotIndex)` e' la chiave: due marker che la
	 * condividono descrivono due posti per la stessa unita', e la validazione li rifiuta.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Spawn", meta = (ClampMin = "0"))
	int32 SlotIndex = 0;

	/**
	 * La cella su cui questo marker cade, DERIVATA dalla posizione dell'attore attraverso il contesto
	 * geometrico della mappa. Falso quando la mappa non c'e', non porta un asset, o la cella risultante non
	 * esiste nell'asset — cioe' quando il marker e' fuori dal tabellone.
	 *
	 * ⚠️ **Non ha un `UWorld` per argomento**: prende la mappa che il chiamante ha gia' risolto, cosi' un
	 * test la puo' esercitare su un attore mappa costruito a mano.
	 */
	bool ResolveCell(const ARTHexMapActor* MapActor, FRTCellId& OutCell) const;

#if WITH_EDITOR
	/** Map Check: la cella non risolve, oppure un altro marker occupa la stessa coppia `(Team, Slot)`. */
	virtual void CheckForErrors() override;
#endif

private:
#if WITH_EDITORONLY_DATA
	/** L'icona con cui lo si vede e lo si seleziona nel viewport. Editor-only: non esiste nel packaged. */
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif
};
