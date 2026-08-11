#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTNoiseTypes.generated.h"

/**
 * Che COSA ha fatto rumore. Non e' una categoria decorativa: la HUD e il TurnLog devono poter dire «uno
 * scatto» invece di «intensita' 6», e il tipo e' l'unica informazione che sopravvive all'attenuazione.
 *
 * Volutamente pochi valori. Ogni voce in piu' e' una promessa di distinguerli, e la v0.1 non ne ha bisogno:
 * il documento sorgente ne elenca molti di piu' (decoy, passi fantasma, eco), tutti dichiarati **fuori
 * perimetro** dal brief §12.
 */
UENUM(BlueprintType)
enum class ERTNoiseType : uint8
{
	/** Passi: movimento normale e scatto. */
	Movement,
	/** Colpo andato a segno o a vuoto: un'arma che parte si sente. */
	Combat,
	/** Ambiente: esplosioni, strutture che cedono, porte forzate. */
	Environment
};

/**
 * Un evento sonoro: il **fatto** che ha prodotto rumore, prima di qualunque propagazione (CP 13.3).
 *
 * Sei campi e non uno di piu': la DoD chiede *«nessun campo non consumato»*, ed e' la regola che tiene
 * onesta una struttura nuova — un campo che nessuno legge diventa, appena serializzato, un valore neutro
 * indistinguibile da un dato vero (vedi `UnitId`/`GraphRevision`, D-063/D-067).
 *
 * ⚠️ `OriginCell` e' la cella della SORGENTE, ed e' informazione da server: non e' cio' che un ascoltatore
 * riceve. Chi sente ottiene un'AREA e un livello `Uncertain` (D13 del brief), mai la cella esatta — un
 * rumore che localizza con precisione e' solo un secondo raggio di rilevamento.
 */
USTRUCT(BlueprintType)
struct FRTNoiseEvent
{
	GENERATED_BODY()

	/** Chi l'ha prodotto: `ARTUnit::StableUnitId`, l'identita' che attraversa fasi e turni. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	int32 SourceUnitId = INDEX_NONE;

	/** Dove e' nato. Server-side: non raggiunge un ascoltatore avversario. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	FRTCellId OriginCell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	ERTNoiseType NoiseType = ERTNoiseType::Movement;

	/**
	 * Quanto forte, sulla scala `0-10` di D-041 — la stessa di `HearingThreshold`, ed e' cio' che rende
	 * confrontabili le due grandezze: `Wait 0 · Sprint 5 · Dash 6 · esplosione 10`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	int32 Intensity = 0;

	/** Il turno in cui e' avvenuto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	int32 TurnIndex = 0;

	/**
	 * Il micro-step della fase. Serve perche' due unita' che si muovono nello stesso turno fanno rumore in
	 * momenti diversi, e l'ordine e' cio' che rende la propagazione riproducibile in un replay: senza, due
	 * eventi dello stesso turno sarebbero indistinguibili e il loro ordine dipenderebbe dall'array.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Noise")
	int32 MicroStepIndex = 0;

	FRTNoiseEvent() = default;
};

/** Quanto rumore e' arrivato in una cella. L'unita' di risposta del flood fill. */
USTRUCT(BlueprintType)
struct FRTNoiseReception
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Noise")
	FRTCellId Cell;

	/** `Intensity` meno il costo acustico speso per arrivare fin qui. Sempre `> 0`: a zero non si sente. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Noise")
	int32 ReceivedNoise = 0;

	FRTNoiseReception() = default;
	FRTNoiseReception(const FRTCellId& InCell, int32 InReceived) : Cell(InCell), ReceivedNoise(InReceived) {}
};
