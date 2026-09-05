#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTStructuralBodyLibrary.generated.h"

class URTHexMapAsset;

/**
 * Un corpo da disegnare sotto una superficie: la cella che lo genera e le due quote che lo delimitano.
 *
 * ⚠️ Le quote sono in **spazio mappa** — origine a `Z = 0` — e non in spazio mondo: il derivatore e' puro e
 * non conosce la posizione dell'actor. Chi disegna somma `GetActorLocation()`, come gia' fa per le celle.
 */
USTRUCT(BlueprintType)
struct FRTStructuralBody
{
	GENERATED_BODY()

	/** La superficie che genera questo corpo. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId Cell;

	/** Faccia SUPERIORE del corpo: la faccia inferiore del tile della cella, non il suo centro. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	float TopZ = 0.f;

	/** Faccia INFERIORE. Sempre `< TopZ`: un corpo di spessore nullo non viene prodotto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	float BottomZ = 0.f;

	/**
	 * Vero se la base e' stata ALZATA dalla cella sottostante, cioe' se il corpo e' piu' corto di quanto
	 * `BodyFill` chiedeva.
	 *
	 * 🔑 Esiste perche' e' l'unica differenza osservabile fra «l'autore ha chiesto poco» e «l'autore ha
	 * chiesto molto e sotto c'era qualcosa»: senza, un log non puo' dire *perche'* quel corpo e' alto cosi'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	bool bTruncated = false;

	float Height() const { return TopZ - BottomZ; }
};

/**
 * IL CORPO STRUTTURALE — `#1865`: da `Cells` + `Height` + `Layer` + `BodyFill` ai volumi da disegnare.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────
 * 🔑 **Due ingressi, e nessuno dei due basta da solo.**
 *
 * **(1) L'autore dichiara QUANTO**, con `FRTHexCellData::BodyFill` in terzi. Non e' derivabile dal
 * contesto, ed e' stato misurato provando a farlo: un ponte e una collina hanno **entrambi** il vuoto
 * sotto di se', il primo deve restare attraversabile e la seconda no, e nessun segnale geometrico li
 * distingue. Una regola che deducesse il riempimento trasformerebbe ogni ponte in un muro.
 *
 * **(2) Il derivatore concilia con CIO' CHE C'E'**: se la frazione porterebbe la base dentro la cella
 * sottostante, la base si ferma sulla sua **faccia superiore** (`RTCellTopZ`). Le due geometrie si toccano
 * senza compenetrare — la stessa convenzione che lo `static_assert` sul rilievo difende in
 * `RTHexMapActor.cpp`, dove *«spessore == gradino, le due facce si toccano senza compenetrare»*.
 *
 * ∴ `BodyFill` e' un'**intenzione**, non una misura: dice quanto l'autore vuole, e il risultato lo dice
 * `FRTStructuralBody::Height()`. `bTruncated` distingue i due casi, che altrimenti sarebbero
 * indistinguibili a valle.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────
 * ⛔ **Il corpo NON e' dato di gioco**, ed e' il primo non-goal di `#1865`: non crea `FRTCellId`, non entra
 * nel grafo, non compare in `ComputeHash`. Due mappe che differiscono solo per `BodyFill` si giocano
 * identiche. Questa libreria e' pura e non tocca l'asset — la si puo' chiamare da un test headless senza
 * un mondo, che e' la ragione per cui vive nel runtime e non nell'Editor.
 */
UCLASS()
class REFACTORTACTICS_API URTStructuralBodyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** La frazione di volume-cella che ogni valore dichiara. `None` -> `0`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static float FillFraction(ERTHexBodyFill Fill);

	/**
	 * I corpi da disegnare, in ordine canonico di cella (`URTHexLibrary::StableLess`) — quindi
	 * **deterministico**: stesso asset, stessa lista, indipendente dall'ordine in cui le celle sono
	 * arrivate nell'array.
	 *
	 * `Map` nullo o senza celle -> lista vuota. Una cella con `BodyFill == None` non produce nulla, e
	 * nemmeno una il cui spazio disponibile sotto sia gia' esaurito da una cella immediatamente sottostante:
	 * un corpo di altezza zero non e' un corpo, e disegnarlo produrrebbe facce coincidenti.
	 */
	static TArray<FRTStructuralBody> DeriveBodies(const URTHexMapAsset* Map);
};
