#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTStructureIdentityLibrary.generated.h"

class URTHexMapAsset;

/**
 * Un bordo risolto per nome: la cella che PORTA la voce di porta e la direzione occupata.
 *
 * E' la stessa convenzione con cui `FRTHexDoor` dichiara il proprio bordo — visto DALLA cella — perche' un
 * bordo scritto su una faccia sola resti citabile senza dover indovinare quale delle due l'ha ricevuto.
 */
USTRUCT(BlueprintType)
struct FRTStructureEdgeRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	ERTHexDirection Edge = ERTHexDirection::E;

	FRTStructureEdgeRef() = default;
	FRTStructureEdgeRef(const FRTCellId& InCell, ERTHexDirection InEdge) : Cell(InCell), Edge(InEdge) {}

	bool operator==(const FRTStructureEdgeRef& Other) const
	{
		return Cell == Other.Cell && Edge == Other.Edge;
	}
};

/**
 * Un arco risolto per nome. Direzionale come `FRTHexEdge`: un ponte bidirezionale ne produce DUE, che
 * portano lo stesso nome perche' sono una struttura sola.
 */
USTRUCT(BlueprintType)
struct FRTStructureArcRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId From;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId To;

	FRTStructureArcRef() = default;
	FRTStructureArcRef(const FRTCellId& InFrom, const FRTCellId& InTo) : From(InFrom), To(InTo) {}

	bool operator==(const FRTStructureArcRef& Other) const
	{
		return From == Other.From && To == Other.To;
	}
};

/**
 * L'identita' stabile delle strutture di mappa (CP 23.3, #832): UNICO punto di lettura da nome a struttura.
 *
 * Sta accanto a `URTHexDoorLibrary` e non dentro di essa per la stessa ragione per cui quella sta accanto a
 * `URTHexCoverLibrary`: risponde a una domanda diversa — «quale struttura si chiama cosi'?» invece di
 * «questo varco e' aperto?» — e soprattutto perche' il nome copre anche gli ARCHI, che porte non sono.
 *
 * ⚠️ Nessuna seconda authority: chi deve MUTARE una porta continua a passare da
 * `URTHexDoorLibrary::SetDoorState`. Questa libreria risolve un nome, non cambia stato.
 */
UCLASS()
class REFACTORTACTICS_API URTStructureIdentityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bordi della porta che porta quel nome, in ordine canonico di cella.
	 *
	 * Vuoto se il nome non esiste, se e' `NAME_None` o se `Map` e' nullo: un nome che non risolve non e' un
	 * errore di lettura — e' il caso su cui poggia la validazione dei riferimenti pendenti.
	 */
	static TArray<FRTStructureEdgeRef> FindDoorEdges(const URTHexMapAsset* Map, FName StableId);

	/**
	 * Archi che portano quel nome, nell'ordine in cui stanno in `Transitions`.
	 *
	 * Vuoto alle stesse condizioni di `FindDoorEdges`. Un ponte bidirezionale ne restituisce due: sono i
	 * due versi della stessa struttura, non due strutture.
	 */
	static TArray<FRTStructureArcRef> FindArcs(const URTHexMapAsset* Map, FName StableId);

	/**
	 * Errori per i riferimenti che non risolvono: un bersaglio fantasma non deve arrivare a runtime.
	 *
	 * Un riferimento e' valido se il nome risolve una porta OPPURE un arco. `NAME_None` non e' un
	 * riferimento: e' un campo lasciato indietro, e va segnalato invece di risolversi in silenzio.
	 *
	 * ⚠️ In #832 non esiste ancora un dato di produzione che citi uno `StableId` — l'unico consumatore
	 * previsto e' l'interaction graph di CP 23.4 (#833). Questa funzione esiste perche' la regola viva in
	 * UN posto quando quel consumatore arrivera', invece di nascere dentro di lui.
	 */
	static TArray<FString> ValidateReferences(const URTHexMapAsset* Map, const TArray<FName>& References);
};
