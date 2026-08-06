#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTPlaybackLibrary.generated.h"

/**
 * Matematica pura del playback della risoluzione (posizione-nel-tempo + durata).
 * Non tocca Actor ne' World: e' testabile in automation. L'animazione degli Actor la usa
 * per interpolare i cilindri e stimare/limitare la durata del round (invariante #1: presentazione,
 * non decisione).
 */
UCLASS()
class REFACTORTACTICS_API URTPlaybackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Yaw (gradi) per orientare un attore da From verso To sul piano XY (facing planare, Z ignorata).
	 * Convenzione UE: +X = 0, +Y = 90, -X = +/-180, -Y = -90. Direzione nulla -> 0.
	 *
	 * Vive qui, con il resto della presentazione del turno: lavora su coordinate MONDO e non sa nulla della
	 * topologia della griglia. Stava in `URTGridLibrary` per ragioni storiche ed e' stata spostata con la
	 * rimozione del substrato quadrato (CP 7.2).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float DirectionYaw(const FVector& From, const FVector& To);

	/**
	 * Posizione lungo una polilinea di waypoint a velocita' costante per segmento.
	 * Alpha in [0,1] copre l'intero percorso: 0 = primo waypoint, 1 = ultimo.
	 * Ogni segmento occupa la stessa frazione di Alpha (1/(N-1)), indipendentemente dalla lunghezza
	 * in mondo (una rampa piu' lunga non rallenta: 1 passo logico = 1 segmento).
	 * Vuoto -> ZeroVector; un solo waypoint -> quello.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static FVector InterpolateAlongPath(const TArray<FVector>& Waypoints, float Alpha);

	/**
	 * Durata stimata del playback (secondi): movimento in PARALLELO (il segmento piu' lungo domina),
	 * attacchi in serie, piu' un beat per fase attiva.
	 * = MaxMoveSegments/CellsPerSecond + NumAttacks*AttackShowSeconds + NumActivePhases*PhaseBeatSeconds.
	 * CellsPerSecond <= 0 e' trattato come "istantaneo" per la parte movimento.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float EstimatePlaybackSeconds(int32 MaxMoveSegments, int32 NumAttacks, int32 NumActivePhases,
		float CellsPerSecond, float PhaseBeatSeconds, float AttackShowSeconds);

	/**
	 * Fattore di accelerazione per rientrare nel tetto di durata: 1 se gia' entro il cap
	 * (o se il cap non e' positivo), altrimenti EstimatedSeconds/MaxSeconds (>= 1).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float SpeedMultiplierForCap(float EstimatedSeconds, float MaxSeconds);
};
