#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTHexOccupancyLibrary.generated.h"

/** Quanti settori angolari dividono un esagono. Uno ogni 30 gradi. */
static constexpr int32 RT_OccupancySectorCount = 12;

/**
 * Quanto una cella e' invasa da geometria solida.
 *
 * NON e' un terzo booleano accanto a `bBlocksMovement`/`bBlocksLineOfSight`: quelli dicono SE si passa,
 * questo dice QUANTO la cella e' stretta, ed e' la ragione per cui esiste un valore intermedio.
 * `Constrained` nasce con il suo consumatore (il costo): senza, sarebbe indistinguibile da `Free` per
 * chiunque legga, cioe' un campo che nessuno legge.
 */
UENUM(BlueprintType)
enum class ERTCellOccupancy : uint8
{
	/** Si attraversa senza pagare nulla in piu'. */
	Free,

	/** Si attraversa, ma la cella e' stretta: costa di piu' (vedi `URTHexOccupancyLibrary::Surcharge`). */
	Constrained,

	/** Non si attraversa. */
	Blocked
};

/**
 * Le soglie che trasformano un conteggio di settori in una classificazione.
 *
 * Vivono nel modulo RUNTIME e non in un `UInteractiveToolPropertySet`, per due ragioni e la seconda e' un KPI.
 * La prima: i property set stanno nel modulo Editor, dove non esiste alcun test, e una soglia che nessun test
 * puo' cambiare non e' una soglia ma una costante travestita. La seconda: il costo di cella entra nell'hash di
 * stato partita (`RTMatchStateHash`), quindi una soglia che fosse stato di tool PER UTENTE farebbe produrre a
 * due autori, dalla stessa geometria, due mappe con hash diverso.
 *
 * Il pannello d'editor le ESPONE; non le possiede.
 */
USTRUCT(BlueprintType)
struct FRTOccupancyThresholds
{
	GENERATED_BODY()

	/** Da questo numero di settori occupati in su la cella e' `Constrained`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 ConstrainedFrom = 4;

	/** Da questo numero di settori occupati in su la cella e' `Blocked`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 BlockedFrom = 6;

	/**
	 * Quanto costa in piu' attraversare una cella `Constrained`. E' il CONSUMATORE della classificazione:
	 * senza, `Constrained` e `Free` sarebbero indistinguibili per chiunque legga, cioe' un campo che nessuno
	 * legge — il difetto che questo repository ha gia' pagato quattro volte.
	 *
	 * Sta qui accanto alle soglie, e non altrove, perche' e' lo stesso dato d'autore: la coppia
	 * «quando una cella e' stretta» + «quanto costa esserlo» si legge e si registra insieme.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 ConstrainedSurcharge = 1;
};

/**
 * L'occupazione di una cella misurata sui dodici settori piu' il centro.
 *
 * ⚠️ **I dodici settori NON sono direzioni di movimento.** Le direzioni restano SEI, pointy-top
 * (`ERTHexDirection`). Questi sono la misura di QUANTO una cella e' invasa, e servono a rispondere a una
 * domanda che un booleano non sa esprimere: un muro che taglia un angolo a 90 gradi attraversa la cella senza
 * rispettarne i lati.
 */
USTRUCT(BlueprintType)
struct FRTOccupancyMask
{
	GENERATED_BODY()

	/**
	 * Dodici bit, uno per settore da 30 gradi.
	 *
	 * **Ancoraggio del settore 0**: parte dal PRIMO VERTICE dell'esagono, quello che `URTHexLibrary` usa per
	 * costruire il perimetro — a `-30` gradi — e ogni settore avanza di `+30` nello stesso verso in cui quella
	 * funzione enumera i vertici. Ne segue che ogni direzione esagonale copre esattamente DUE settori
	 * consecutivi (`E` = 0 e 1, `NE` = 2 e 3, ...), il che rende la corrispondenza fra i dodici settori e le
	 * sei direzioni esatta invece che approssimata.
	 *
	 * L'ancoraggio non e' cosmetico: senza, due implementazioni entrambe corrette producono maschere diverse
	 * dalla stessa geometria, «deterministica» diventa vera per costruzione, e la maschera e' un intero che
	 * prima o poi finisce in un hash.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Sectors = 0;

	/** Il CENTRO della cella e' occupato. Da solo basta a rendere la cella `Blocked`, comunque siano i settori. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bCoreBlocked = false;
};

/**
 * IL CONTRATTO D'INGRESSO: una polilinea di geometria architettonica, in coordinate LOCALI di cella —
 * origine nel centro della cella, stesse unita' di `HexSize`.
 *
 * Aperta = un MURO che taglia la cella. Chiusa = un FOOTPRINT, e il suo interno e' solido.
 * Le quattro fixture di §22 del sorgente si esprimono tutte qui: un segmento solido (aperta, due punti),
 * un angolo (aperta, tre punti), un footprint solido (chiusa, contiene il centro), un footprint void
 * (chiusa, non lo contiene).
 *
 * ⚠️ **Quali polilinee siano LEGALI non si decide qui**: la grammatica quantizzata delle direttrici e' di
 * `#620`, ed e' arrivata — `FRTGeometrySegment` in `RTGeometryGrammar.h`.
 *
 * 🔑 **E questo tipo NON e' l'authority** (`D-127`): e' il DERIVATO di calcolo. L'authority e' discreta —
 * un enum e tre interi — e questa polilinea si ottiene da li' con
 * `URTGeometryGrammarLibrary::ToPolyline`. Il float che vive in `Points` esiste per DISEGNARE, non per
 * decidere: nessun estremo in virgola mobile entra in cio' che si serializza e si hasha.
 */
USTRUCT(BlueprintType)
struct FRTOccupancyPolyline
{
	GENERATED_BODY()

	/** Vertici in coordinate locali di cella. Meno di due punti non occupa nulla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	TArray<FVector2D> Points;

	/** Chiusa: l'ultimo punto si ricongiunge al primo e l'interno e' SOLIDO. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bClosed = false;
};

/**
 * Occupazione di una cella da geometria solida: dodici settori, il centro, e la classificazione che ne esce.
 *
 * Interamente PURA e headless — geometria in ingresso, dodici bit e un enum in uscita — e vive nel modulo
 * runtime perche' in `Source/RefactorTacticsEditor/` non esiste alcun test: cio' che nasce dentro l'editor
 * nasce non verificabile. L'editor la CHIAMA.
 */
UCLASS()
class REFACTORTACTICS_API URTHexOccupancyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Quanti dei dodici settori sono occupati. Ignora i bit oltre il dodicesimo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 NumOccupiedSectors(const FRTOccupancyMask& Mask);

	/**
	 * Classifica una cella dal numero di settori occupati, per soglia.
	 *
	 * `bCoreBlocked` vince su tutto: una cella con il centro dentro un muro e' `Blocked` anche se i settori
	 * occupati fossero zero — che e' il caso di un footprint solido piu' grande della cella, i cui bordi
	 * cadono tutti fuori.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTCellOccupancy Classify(const FRTOccupancyMask& Mask, const FRTOccupancyThresholds& Thresholds);

	/**
	 * I DODICI PUNTI che delimitano i settori sul perimetro, in coordinate locali di cella.
	 *
	 * Alternano vertice dell'esagono e punto medio di lato: `P[0]` e' il primo vertice — lo stesso da cui
	 * `URTHexLibrary` costruisce il perimetro, a `-30` gradi — `P[1]` e' il punto medio del lato `E`, e cosi'
	 * via ogni `30` gradi. Il settore `k` e' il triangolo `(centro, P[k], P[k+1])`, e i dodici triangoli
	 * pavimentano l'esagono esattamente: fra un vertice e il punto medio di un lato adiacente il bordo
	 * dell'esagono E' un segmento dritto.
	 */
	static void SectorBoundaryPoints(float HexSize, TArray<FVector2D>& OutPoints);

	/**
	 * Misura l'occupazione della cella dalla geometria che la investe.
	 *
	 * DETERMINISTICA e indipendente dall'ordine: i settori si accendono con un OR, quindi la stessa geometria
	 * presentata in ordine diverso produce la stessa maschera per costruzione — e un test lo dimostra invece
	 * di dedurlo.
	 *
	 * Un settore e' occupato se la geometria lo INTERSECA. Per una polilinea chiusa conta anche il suo
	 * interno: un footprint che contiene il centro rende `bCoreBlocked` vero e occupa tutti i settori, e uno
	 * piu' grande dell'intera cella lo fa senza che il suo bordo tocchi un solo triangolo — che e' la ragione
	 * per cui `CoreBlocked` esiste e non e' deducibile dal conteggio.
	 */
	static FRTOccupancyMask ComputeMask(const TArray<FRTOccupancyPolyline>& Geometry, float HexSize);


	/**
	 * Quanto costa IN PIU' attraversare una cella con questa classificazione — la META' COSTO della cottura,
	 * che appartiene a #619. I BORDI (`FRTHexCover`, `bBlocksMovement`) sono di #621.
	 *
	 * `Blocked` non paga un sovrapprezzo perche' non si attraversa affatto: chi la rende impassabile e' il
	 * bordo, non il costo. Restituire un numero alto qui sarebbe un secondo modo di dire «non si passa», e due
	 * modi di dire la stessa cosa divergono.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 Surcharge(ERTCellOccupancy Occupancy, const FRTOccupancyThresholds& Thresholds);
};
