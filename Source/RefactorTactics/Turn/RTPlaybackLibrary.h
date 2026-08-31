#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnRules.h"
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
	 * Quanti colpi devono essere GIA' MOSTRATI dopo PhaseElapsed secondi di fase Blast: il primo esce
	 * subito, e ogni AttackShowSeconds successivi ne compare un altro, fino a NumAttacks.
	 * = Min(NumAttacks, 1 + Floor(PhaseElapsed/AttackShowSeconds)).
	 * AttackShowSeconds <= 0 significa "nessuno scaglionamento": escono tutti insieme.
	 *
	 * E' il contro-termine di `PhaseDuration`, che per il `Blast` riserva gia' il tempo dei colpi: la
	 * durata mostrata a chi guarda e il ritmo con cui i colpi compaiono devono venire dalla stessa
	 * formula, o la barra promette un tempo che la riproduzione non usa (#911).
	 * ⚠️ Poiche' il primo colpo esce a t=0, N colpi occupano N-1 intervalli: l'ultimo compare a
	 * (N-1)*AttackShowSeconds, un beat prima che la durata riservata alla fase finisca. Quel beat
	 * e' il tempo di lettura dell'ultimo colpo, non un residuo da recuperare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static int32 AttacksToShow(int32 NumAttacks, float PhaseElapsed, float AttackShowSeconds);

	/**
	 * Durata (secondi) di UNA fase del playback, prima di qualunque accelerazione.
	 *
	 * `MaxMoveSegments` e' il percorso PIU' LUNGO fra quelli riprodotti in questa fase, non la loro somma:
	 * le unita' si muovono in parallelo, quindi la fase finisce quando finisce l'ultima.
	 *
	 *  - `Dash` / `Move`  → `MaxMoveSegments / CellsPerSecond`. Gli attacchi non entrano.
	 *  - `Blast`          → `Max(colpi, spinta)`, **non** la somma: i colpi e lo scivolamento del knockback
	 *                       occupano la stessa finestra. Il tempo dei colpi ha un pavimento di uno anche
	 *                       quando non ce ne sono, perche' un Blast di sola spinta si vede e deve durare.
	 *  - ogni altra fase  → un beat (`PhaseBeatSeconds`).
	 *
	 * `CellsPerSecond <= 0` significa movimento istantaneo, non una divisione per zero.
	 *
	 * 🔑 **E' l'UNICA formula di durata del playback**, e il totale del round non ne ha una propria:
	 * `ARTTurnManager::BeginPlayback` somma questa su tutte le fasi attive (`RawTotal`).
	 *
	 * ⛔ **Non aggiungerne una aggregata.** Ne e' esistita una — `EstimatePlaybackSeconds`, rimossa il
	 * 2026-08-31 — che sommava movimento, colpi e beat sull'intero round: dava un numero **diverso** da
	 * questo, perche' qui il `Blast` prende `Max(colpi, spinta)` e non la somma. Era coperta da quattro
	 * asserzioni e chiamata da nessuno, cioe' una verita' verde e morta accanto a quella viva. Se serve il
	 * totale, si somma questa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float PhaseDuration(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
		float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds);

	/**
	 * Fattore di accelerazione per rientrare nel tetto di durata: 1 se gia' entro il cap
	 * (o se il cap non e' positivo), altrimenti EstimatedSeconds/MaxSeconds (>= 1).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float SpeedMultiplierForCap(float EstimatedSeconds, float MaxSeconds);

	/**
	 * Velocita' effettiva del playback: compone la velocita' SCELTA da chi guarda (x1/x2/x4) con il
	 * fattore di accelerazione che il tetto di durata impone da se'.
	 * = Max(ViewerSpeed, CapSpeed) — «almeno la velocita' che chiedi, almeno quella che serve a stare
	 * nel tetto». Valori non positivi sono trattati come 1 (nessuna scelta / nessun limite).
	 *
	 * Le altre tre composizioni sono state scartate su un caso concreto ciascuna (CP 47.2, #955):
	 *  - PRODOTTO (Viewer*Cap): un round gia' accelerato 3x dal tetto, visto a x4, va a 12x — illeggibile
	 *    proprio nei round che piu' avrebbero bisogno di essere letti;
	 *  - SOSTITUZIONE (solo Viewer): il tetto smette di valere e MaxPlaybackSeconds diventa un campo morto;
	 *  - TETTO RIDEFINITO (x2 -> Max/2): sotto il tetto il cap non si applica affatto, quindi su un round
	 *    da 4 s premere x2 non farebbe NULLA. E' il difetto che la uccide, ed e' il caso comune.
	 * Max e' monotona nei due argomenti e non produce mai il caso 12x.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float EffectivePlaybackSpeed(float ViewerSpeed, float CapSpeed);
};
