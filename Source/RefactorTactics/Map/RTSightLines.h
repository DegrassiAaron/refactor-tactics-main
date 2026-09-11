#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexVisionLibrary.h"
#include "RTSightLines.generated.h"

class URTHexMapAsset;

/**
 * Un bersaglio come lo vede CHI GUARDA — `#2742`.
 *
 * 🔑 **Porta il flag di conoscenza, e non la unita'.** Il produttore delle linee non deve poter leggere
 * niente altro di un bersaglio: gli arrivano una cella e un booleano che il velo ha **gia' deciso**
 * (`ARTUnit::IsKnownToObserver`), esattamente come `URTCombatLibrary::RefusalForObserver` riceve il
 * verdetto gia' calcolato. E' cio' che rende il canary di privacy scrivibile senza allestire un mondo.
 */
USTRUCT(BlueprintType)
struct FRTObservedTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Sight")
	FRTCellId Cell;

	/** Cio' che il velo ha deciso su questo bersaglio. ⛔ Non si ricalcola qui. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Sight")
	bool bKnownToObserver = false;

	FRTObservedTarget() = default;
	FRTObservedTarget(const FRTCellId& InCell, bool bInKnown)
		: Cell(InCell), bKnownToObserver(bInKnown) {}
};

/**
 * Una linea di tiro pronta per lo schermo: da dove parte, dove arriva, e dove si interrompe.
 *
 * ⚠️ **Non aggiunge un solo dato a `FRTLineOfSightResult`**: lo trasporta insieme ai due estremi, che il
 * verdetto grezzo non porta perche' non ne ha bisogno. Chi disegna sa **quale** linea sta guardando solo
 * se gli estremi viaggiano col verdetto.
 */
USTRUCT(BlueprintType)
struct FRTSightLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Sight")
	FRTCellId From;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Sight")
	FRTCellId To;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Sight")
	FRTLineOfSightResult Sight;

	bool IsClear() const { return Sight.IsClear(); }
};

/**
 * LE LINEE DI TIRO CHE UN OSSERVATORE PUO' VEDERE — `#2742`.
 *
 * ## Cosa possiede, e cosa NON possiede
 *
 * Possiede la **linea**: la traiettoria verso un bersaglio e il punto in cui si interrompe.
 *
 * ⛔ **Non possiede l'AREA** — *«quali celle vedo»* — che e' di [#1944] e ha un'altra resa, un altro costo
 * e un'altra grammatica. Il confine e' stato deciso il 2026-09-10 ed e' scritto in entrambe le issue: due
 * rese della stessa sorgente, non due sorgenti.
 *
 * ## Perche' esiste, invece di chiamare `DescribeLineOfSight` dal renderer
 *
 * 🔴 **Perche' la conoscenza deve stare in UNA firma.** Un renderer che chiamasse la geometria e poi
 * scartasse i bersagli ignoti sarebbe un **secondo contratto di conoscenza**, e ogni consumatore futuro
 * dovrebbe ricordarselo. E' il difetto che `ARTHUD::ComputeBlockerMarks` dichiara di evitare non
 * rifiltrando: la difesa sta nella **sorgente**.
 *
 * ∴ qui il filtro non e' una clausola: e' la ragione per cui la funzione ha questo nome.
 *
 * ⚠️ **La geometria non si ricalcola.** `DescribeLineOfSight` resta il produttore unico e questa libreria
 * lo chiama — non riscrive il cammino sulla linea, che diverrebbe la seconda autorita' che l'invariante #1
 * vieta.
 */
UCLASS()
class REFACTORTACTICS_API URTSightLineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Le linee che l'osservatore puo' vedere, dai bersagli che gli sono gia' stati autorizzati.
	 *
	 * 🔑 **Un bersaglio ignoto non produce una linea, e non produce nemmeno una linea vuota**: sparisce
	 * dal risultato. Una voce «linea assente» sarebbe distinguibile da «nessuna voce», e la differenza fra
	 * le due sarebbe essa stessa il canale che [D-225] chiude.
	 *
	 * ⚠️ **Accetta N bersagli benche' il consumatore ne passi UNO.** Non e' generalita' speculativa: e' la
	 * forma che rende scrivibile il canary — *«due stati nascosti diversi, stessa conoscenza autorizzata,
	 * stesso insieme di linee»* si asserisce su un INSIEME, e con un solo bersaglio l'asserzione
	 * degenererebbe. La cardinalita' 1 e' una scelta del CHIAMANTE (`#2742`: una linea, quella verso il
	 * bersaglio puntato), non di questa firma.
	 *
	 * @param Map      la mappa autorevole. Senza, nessuna linea: fail-closed come `ClassifyHexTargeting`.
	 * @param From     la cella del tiratore
	 * @param Targets  i bersagli col flag che il velo ha gia' deciso
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Sight")
	static TArray<FRTSightLine> AuthorizedSightLines(const URTHexMapAsset* Map, const FRTCellId& From,
		const TArray<FRTObservedTarget>& Targets);
};
