#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchStateHash.generated.h"

class URTHexMapAsset;

/**
 * Stato di un'unità al termine della partita, nella forma che entra nel checksum.
 *
 * È una struttura di DATI e non `ARTUnit` perché il checksum non deve dipendere da un Actor: così si calcola
 * headless, si verifica con un test diretto, e chi lo legge vede esattamente cosa ci finisce dentro.
 */
USTRUCT()
struct FRTUnitStateDigest
{
	GENERATED_BODY()

	/** Identità stabile dell'unità nello scenario: viene dal file, non dall'ordine di spawn. */
	UPROPERTY() FString Id;

	UPROPERTY() FRTCellId Cell;
	UPROPERTY() int32 Health = 0;
	UPROPERTY() int32 Shield = 0;
	UPROPERTY() int32 Energy = 0;
	UPROPERTY() bool bAlive = true;

	/**
	 * Stati attivi (tag). Arrivano da `TMap`/`TSet`, la cui iterazione non è deterministica: `HashMatchState`
	 * li **ordina** prima di mescolarli, quindi il chiamante può passarli come li trova.
	 */
	UPROPERTY() TArray<FName> Statuses;
};

/**
 * Checksum dello stato di fine partita (CP 12.1, issue #81).
 *
 * Copre ciò che il DoD chiede: unità, **stati**, **terreni modificati**, **strutture** e **progresso degli
 * obiettivi**. Prima copriva le sole unità, e due finali che differivano solo per il campo — una cella in
 * fiamme, una copertura eretta, un obiettivo conquistato — davano lo stesso hash.
 *
 * Perché conta oltre al DoD: il corpus golden di CP 12.6 (#178) confronta TurnLog **e** checksum. Un checksum
 * cieco all'ambiente avrebbe delegato tutta la copertura ambientale al solo TurnLog, senza dirlo.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchStateHashLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Digest FNV-1a dello stato finale: stesso idioma di `URTTurnLogLibrary::HashTurnLog`, e come quello
	 * mescola **solo interi** (invariante #4).
	 *
	 * Ordine totale e stabile: le unità per `Id`, le celle nell'ordine canonico dell'asset (`SortCells`), gli
	 * stati ordinati per nome. Permutare gli ingressi non cambia il risultato — è la proprietà che rende il
	 * checksum una prova di determinismo invece di una sua vittima.
	 *
	 * @param Map        mappa a fine partita: superfici, coperture, bordi, archi e revisione.
	 * @param Units      stato delle unità (l'ordine dell'array è irrilevante).
	 * @param TeamScores progresso obiettivo per squadra, indicizzato per `TeamId`.
	 */
	static uint32 HashMatchState(const URTHexMapAsset* Map, const TArray<FRTUnitStateDigest>& Units,
		const TArray<int32>& TeamScores);
};
