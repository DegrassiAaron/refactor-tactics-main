#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexDoorLibrary.generated.h"

class URTHexMapAsset;

/**
 * Ordine di una porta raccolto durante una fase, gia' riferito al BORDO: la coppia di celle lo identifica
 * senza una direzione, la stessa convenzione con cui il TurnLog lo scrive in `SrcCell`/`TgtCell`.
 */
USTRUCT(BlueprintType)
struct FRTDoorOp
{
	GENERATED_BODY()

	/** Cella dal lato di chi agisce. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId From;

	/** Cella oltre il bordo. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId To;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexDoorState State = ERTHexDoorState::Closed;

	/** Chi apre o chiude, come indice (#405). Il TurnLog lo legge; il calcolo della porta lo ignora. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 ActorId = INDEX_NONE;

	FRTDoorOp() = default;
	FRTDoorOp(const FRTCellId& InFrom, const FRTCellId& InTo, ERTHexDoorState InState, int32 InActorId = INDEX_NONE)
		: From(InFrom), To(InTo), State(InState), ActorId(InActorId) {}
};

/**
 * UN ORDINE DATO A UNA SORGENTE, non a un bordo — `#833`, `CP 23.4`.
 *
 * 🔑 **Esiste perche' un bordo non basta a dire chi comanda.** `FRTDoorOp` nomina la porta con le due celle
 * che la contengono: e' esatto per la porta che si apre stando adiacenti, e muto per una leva che ne comanda
 * altre. Il grafo di interazione lega **nomi** (`FRTInteractionBinding::SourceId`), e i bersagli li risolve
 * `URTHexDoorLibrary::ApplyInteraction` a fase conclusa.
 *
 * ⚠️ **Il client non sceglie i bersagli, e questo tipo lo rende impossibile invece di vietarlo**: qui c'e' il
 * nome della SORGENTE e nient'altro. Chi ha pianificato ha puntato un bordo che vedeva; quale porta si apra
 * dall'altra parte della mappa lo decide l'autorita', leggendo l'asset.
 */
USTRUCT(BlueprintType)
struct FRTInteractionOp
{
	GENERATED_BODY()

	/** Il nome della struttura su cui si agisce, `StableId` di `CP 23.3`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FName SourceId;

	/**
	 * Lo stato richiesto ai bersagli.
	 *
	 * ⚠️ `Action.Interact` puo' chiedere solo `Open` ([D-151]) e la commutazione e' `INT-7`: il campo esiste
	 * perche' `ApplyInteraction` lo prende, non perche' qui ci sia una scelta.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexDoorState State = ERTHexDoorState::Open;

	/** Chi ha dato l'ordine, come indice (#405). Il TurnLog lo legge. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 ActorId = INDEX_NONE;

	FRTInteractionOp() = default;
	FRTInteractionOp(FName InSourceId, ERTHexDoorState InState, int32 InActorId = INDEX_NONE)
		: SourceId(InSourceId), State(InState), ActorId(InActorId) {}
};

/** Cosa e' cambiato su un bordo dopo un ordine: le voci che il chiamante scrive nel TurnLog. */
USTRUCT(BlueprintType)
struct FRTDoorChange
{
	GENERATED_BODY()

	/** Cella che PORTA la voce di porta (puo' essere una delle due facce del bordo). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId Cell;

	/** Cella oltre il bordo: con `Cell` identifica la porta in modo leggibile nel log. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId Toward;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexDoorState State = ERTHexDoorState::Closed;

	/** Vero se il bordo, dopo il cambio, nega passo e vista. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	bool bBlocking = false;

	/** Chi ha agito, propagato da `FRTDoorOp` (#405). Indice, non Actor: lo strato resta puro. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 ActorId = INDEX_NONE;
};

/**
 * Le porte: stato di un BORDO che puo' cambiare a partita in corso.
 *
 * Sta accanto a `URTHexCoverLibrary` e non dentro di essa perche' risponde a una domanda diversa — «questo
 * varco e' aperto?» invece di «questo bordo ripara?» — ma la LETTURA e' unificata da `BlocksTraversal`, che
 * somma muro e porta: e' quella l'unica funzione che vista, pathfinding e combat interrogano, quindi non
 * possono divergere. Vedi docs/gameplay/spec-porte-cp93.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexDoorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Stato della porta sul bordo fra due celle ADIACENTI, visto dai due lati: vale il piu' RESTRITTIVO delle
	 * due facce, con la stessa ragione delle coperture — la barriera e' fisica, e la regola non deve dipendere
	 * da quale delle due celle l'ha ricevuta.
	 *
	 * `Open` se le celle non sono adiacenti, se il bordo non ha porte, se una cella manca o se `Map` e' nullo:
	 * l'assenza di una porta non e' una porta aperta che qualcuno puo' chiudere, ma per la traversata sono
	 * indistinguibili — chi deve saperlo usa `FindDoor`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTHexDoorState DoorBetween(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/** Vero se sul bordo c'e' una porta che NEGA passo e vista (`Closed` o `Locked`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool BlocksBetween(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/** Vero se quello stato nega passo e vista. Regola in un posto solo: la usano lettura e log. */
	static bool StateBlocks(ERTHexDoorState State)
	{
		return State == ERTHexDoorState::Closed || State == ERTHexDoorState::Locked;
	}

	/**
	 * UNICO ingresso di mutazione. Porta il bordo — e ogni altro bordo del suo GRUPPO (`DoorId`) — allo stato
	 * richiesto, scrivendo su ENTRAMBE le facce dichiarate, e incrementa la revisione UNA sola volta.
	 *
	 * Le regole di transizione stanno qui perche' questo e' l'unico posto che puo' garantirle: chi le
	 * bypassasse produrrebbe stati impossibili.
	 * - `Destroyed` e' TERMINALE: nessuna transizione ne esce;
	 * - `Locked` -> `Open` e' RIFIUTATA (serve l'apertura autorizzata di CP 10.1); le altre uscite sono ammesse.
	 *
	 * Ritorna solo cio' che e' cambiato DAVVERO, in ordine canonico, cosi' il chiamante ha esattamente le voci
	 * da scrivere nel TurnLog. Un bordo senza porta, o gia' nello stato richiesto, non produce nulla e non
	 * tocca ne' hash ne' revisione.
	 */
	static TArray<FRTDoorChange> SetDoorState(URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		ERTHexDoorState State);

	/**
	 * Applica gli ordini raccolti in una fase, a fase CONCLUSA (invariante #3: l'ordine degli ordini non
	 * cambia l'esito). Sullo stesso bordo vince lo stato piu' RESTRITTIVO: due unita' che nello stesso turno
	 * una apre e una chiude danno lo stesso esito in qualunque ordine.
	 */
	static TArray<FRTDoorChange> ApplyDoorOps(URTHexMapAsset* Map, const TArray<FRTDoorOp>& Ops);

	/**
	 * Applica un'interazione REMOTA: risolve i bersagli comandati da `SourceId` nel grafo di CP 23.4 (#833) e
	 * li porta a `State`, con **una sola revisione** qualunque sia il numero di bersagli.
	 *
	 * ⚠️ **Non e' un secondo ingresso di mutazione**: passa dalla stessa commutazione di `SetDoorState` — stesse
	 * regole di transizione, stesso trattamento del gruppo `DoorId`, stesse voci — e si limita a commettere una
	 * volta in fondo invece che una per bersaglio. Le regole restano dove erano.
	 *
	 * ⚠️ **L'operazione NON e' atomica** ([D-150]): applica i bersagli applicabili e riporta gli altri in
	 * `OutRefusals` con un reason code che nomina la struttura, il suo stato e quello richiesto. E' il
	 * comportamento che il motore ha gia' un livello sotto, non una scelta nuova — e toglie di mezzo la
	 * pre-validazione che «tutto o niente» richiederebbe, dato che la commutazione non sa tornare indietro.
	 *
	 * L'ordine di applicazione e' quello dichiarato in `FRTInteractionBinding::TargetIds`, che e' dato d'asset
	 * e autorevole; dentro ogni bersaglio vale l'ordine canonico di cella.
	 *
	 * Sorgente sconosciuta, senza binding o con un bersaglio conteso (`INT-5`): nessun cambio. Il terzo caso si
	 * distingue dagli altri due perche' arriva con un reason code, non perche' la lista e' vuota.
	 */
	static TArray<FRTDoorChange> ApplyInteraction(URTHexMapAsset* Map, FName SourceId, ERTHexDoorState State,
		int32 ActorId = INDEX_NONE, TArray<FString>* OutRefusals = nullptr);
};
