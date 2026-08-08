#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Map/RTCellId.h"
#include "RTMovementActionLibrary.generated.h"

class URTHexMapAsset;

/** Perche' una mobilita' lineare si e' fermata dove si e' fermata. E' il reason code del TurnLog. */
UENUM(BlueprintType)
enum class ERTLinearStop : uint8
{
	/** Arrivata alla cella richiesta. */
	Completed,
	/** Il bersaglio non e' su una delle sei direzioni, o e' oltre la portata: l'azione non parte. */
	NotAligned,
	/** Fermata davanti a un muro, a una cella che blocca il movimento o al bordo della mappa. */
	BlockedByTerrain,
	/** Fermata davanti a un'unita' (o perche' la cella d'arrivo era occupata). */
	BlockedByUnit,
	/** Fermata SUL primo nemico incontrato: e' l'impatto della carica. */
	Impact
};

/** Esito di una mobilita' lineare: dove si arriva, cosa si attraversa, chi si colpisce e perche' ci si ferma. */
USTRUCT(BlueprintType)
struct FRTLinearMoveResult
{
	GENERATED_BODY()

	/** Cella finale (= cella di partenza se l'azione non parte). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTCellId Final;

	/** Celle attraversate, partenza esclusa. Vuoto se non ci si muove. Per il salto contiene solo l'atterraggio. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	TArray<FRTCellId> Entered;

	/** Unita' urtata dalla carica (INDEX_NONE se nessuna). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	int32 ImpactUnitId = INDEX_NONE;

	/**
	 * Unita' ATTRAVERSATE lungo la traiettoria (`LinearPass`), nell'ordine in cui sono state incontrate.
	 * Vuoto per ogni altro stile.
	 *
	 * Lista e non singolo id, a differenza di `ImpactUnitId`: la carica si ferma sul primo bersaglio, quindi
	 * ne ha uno per definizione; la lama ne incontra quanti ne stanno sulla linea.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	TArray<int32> PassedThroughUnitIds;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	ERTLinearStop Stop = ERTLinearStop::NotAligned;

	FRTLinearMoveResult() = default;
};

/**
 * Le mobilita' LINEARI del catalogo v0.1 (`Dash`, `Charge`, `Leap`, `Reposition`): una linea retta su una
 * delle sei direzioni, non un percorso.
 *
 * Sono cosa diversa dal movimento a budget (`Action.Move`, `Action.Sprint`), che aggira gli ostacoli in
 * pianificazione: qui la traiettoria e' dichiarata e cio' che la interrompe FERMA lo scatto. Tenerle separate
 * e' il motivo per cui `Dash.BlockedArc` e' verificabile: con il pathfinding, un muro davanti non fermerebbe
 * nulla — lo si girerebbe intorno.
 *
 * Funzioni pure: nessun Actor, nessun UWorld.
 */
UCLASS()
class REFACTORTACTICS_API URTMovementActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Vero se lo stile dichiarato si muove in linea retta (e quindi passa da `ResolveLinearMove`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Movement")
	static bool IsLinear(ERTMovementStyle Style)
	{
		return Style == ERTMovementStyle::LinearDash
			|| Style == ERTMovementStyle::LinearCharge
			|| Style == ERTMovementStyle::LinearLeap
			|| Style == ERTMovementStyle::LinearPass;
	}

	/**
	 * Vero se `To` sta su una delle sei direzioni esagonali a partire da `From` (stesso layer), con la
	 * distanza in celle. Il salto obliquo di un cavallo degli scacchi non e' una direzione esagonale.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Movement")
	static bool IsStraightLine(const FRTCellId& From, const FRTCellId& To, int32& OutDistance);

	/**
	 * Risolve una mobilita' lineare secondo lo stile dichiarato dall'azione.
	 *
	 * - `LinearDash` — avanza finche' la cella successiva e' percorribile e libera; si ferma PRIMA di un muro
	 *   o di un'unita'. Non termina mai in una cella occupata.
	 * - `LinearCharge` — come sopra, ma se la cella successiva ospita un NEMICO si ferma li' davanti e lo
	 *   segnala come impatto: e' la carica che si arresta addosso al bersaglio.
	 * - `LinearLeap` — ignora unita' e celle intermedie: conta solo l'atterraggio, che deve essere una cella
	 *   percorribile e libera. Non cambia layer.
	 *
	 * `Occupancy` mappa cella -> UnitId; `Hostiles` sono gli UnitId che la carica considera bersagli.
	 * Senza mappa non si muove nessuno (FAIL-CLOSED): non si sa cosa ci sia davanti.
	 */
	static FRTLinearMoveResult ResolveLinearMove(const URTHexMapAsset* Map, const FRTCellId& From,
		const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
		const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles);

	/**
	 * Vero se una mobilita' lineare con questi parametri **arriva davvero** su `Target`.
	 *
	 * Serve a chi PROPONE uno scatto (il bot genera le candidate col grafo, che non conosce la linearita')
	 * per non offrire mosse che la risoluzione rifiuterebbe: un'abilita' spesa senza effetto e senza
	 * spiegazione e' peggio di una mossa non fatta. E' un semplice predicato su `ResolveLinearMove`, non una
	 * seconda implementazione — e' proprio la divergenza fra le due che la issue #140 ha chiuso.
	 *
	 * `From == Target` e' vero: «resto dove sono» non e' uno scatto illegale, e chi filtra un insieme di
	 * candidate non deve perdere quella di partenza.
	 *
	 * Nota sulla CARICA: qui conta l'arrivo sulla cella, quindi una carica che si ferma ADDOSSO al nemico
	 * (esito `Impact`, cella precedente) risulta non raggiungibile. E' voluto: chi filtra candidate di
	 * riposizionamento vuole sapere dove si ATTERRA, e l'impatto e' un intento del Blast, non un arrivo.
	 */
	static bool IsLinearReachable(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& Target,
		int32 MaxCells, ERTMovementStyle Style, const TMap<FRTCellId, int32>& Occupancy,
		const TSet<int32>& Hostiles);
};
