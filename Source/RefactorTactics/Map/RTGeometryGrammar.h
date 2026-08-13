#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexCellData.h"
#include "RTGeometryGrammar.generated.h"

/**
 * Quante suddivisioni per PUNTO NOTEVOLE della direzione. E' il passo della griglia d'authoring.
 *
 * ⚠️ **Non e' una frazione di `HexSize`, ed e' la scelta che fa funzionare tutto il resto.** Un quanto
 * lineare uniforme non puo' esprimere insieme i due punti notevoli dell'esagono: l'apotema vale
 * `HexSize * sqrt(3)/2`, e nessun numero di suddivisioni di `HexSize` la rende intera — con `HexSize = 100`
 * l'apotema e' `86.602540...`, irrazionale per costruzione. Un muro appoggiato al lato dell'esagono avrebbe
 * quindi un offset non rappresentabile, e la TERZA famiglia della grammatica — «segmenti sui lati /
 * perimetro» — sarebbe inesprimibile in interi.
 *
 * Qui il quanto e' RELATIVO al punto di confine di quella direzione: `Q` quanti lungo una direzione
 * significano «esattamente il punto notevole di quella direzione», che sia un vertice (raggio pieno) o un
 * punto medio di lato (apotema). Le due lunghezze diverse diventano lo stesso intero, ed e' esatto:
 * il muro sul lato E ha `Offset = Q` e estremi `-Q/2 .. +Q/2`, che ricostruiscono i due vertici a meno
 * dell'epsilon di macchina.
 *
 * `12` perche' e' pari — serve a `Q/2`, il mezzo lato — e divisibile per 2, 3, 4 e 6, cioe' le frazioni
 * che un designer usa davvero. Non ha relazione con i dodici settori di `#619`: quelli sono un righello
 * angolare, questo e' un passo radiale.
 */
static constexpr int32 RT_GeometryQuanta = 12;

/**
 * Limite di sanita' sugli interi della grammatica: nessuna coordinata oltre quattro punti notevoli dal
 * centro della cella.
 *
 * Non e' una regola di gameplay ed e' volutamente largo: la geometria vive in coordinate LOCALI di cella e
 * un muro lungo attraversa piu' celle, quindi in ciascuna i suoi estremi cadono legittimamente fuori. Serve
 * a impedire che un dato corrotto o una deserializzazione futura portino interi assurdi dentro una
 * conversione che poi moltiplica per `HexSize`.
 */
static constexpr int32 RT_GeometryMaxQuanta = 4 * RT_GeometryQuanta;

/**
 * I sei assi tattici, non orientati, a 30 gradi l'uno dall'altro.
 *
 * ⚠️ **Gli angoli NON sono incisi qui**: ogni asse e' l'indice di un punto di confine di
 * `URTHexOccupancyLibrary::SectorBoundaryPoints`, che li ancora al primo vertice dell'esagono — a `-30`
 * gradi — nello stesso verso in cui `URTHexLibrary` enumera il perimetro. L'asse `k` e' il punto di
 * confine `k + 1`. Se la convenzione dei sei lati cambiasse, gli assi la seguirebbero; un angolo scritto a
 * mano mentirebbe in silenzio, che e' esattamente il difetto che `#588` ha pagato.
 *
 * Tre assi puntano al PUNTO MEDIO di un lato (`Deg0`, `Deg60`, `Deg120`) e tre a un VERTICE (`Deg30`,
 * `Deg90`, `Deg150`): sono le «direttrici principali derivate dall'esagono» e le loro ortogonali.
 */
UENUM(BlueprintType)
enum class ERTTacticalAxis : uint8
{
	/** Verso il punto medio del lato `E`. Punto di confine 1. */
	Deg0,
	/** Verso il vertice a 30 gradi. Punto di confine 2. */
	Deg30,
	/** Verso il punto medio del lato `NE`. Punto di confine 3. */
	Deg60,
	/** Verso il vertice a 90 gradi. Punto di confine 4. */
	Deg90,
	/** Verso il punto medio del lato `NW`. Punto di confine 5. */
	Deg120,
	/** Verso il vertice a 150 gradi. Punto di confine 6. */
	Deg150
};

/** Quanti assi tattici esistono. Chi itera usa questo, non un `6` scritto a mano. */
static constexpr int32 RT_TacticalAxisCount = 6;

/**
 * Perche' un segmento non appartiene alla grammatica.
 *
 * E' un REASON CODE, non una stringa, e la ragione e' verificabile invece che stilistica: la verifica di
 * mutazione richiesta dal DoD di `#620` — «allentata una regola per volta, cade ESATTAMENTE il test che la
 * protegge» — non e' asseribile se due regole diverse producono messaggi di testo simili. Il precedente e'
 * consolidato in `ERTActionInvalidReason`, `ERTHexTargetReason`, `ERTHexWaypointReason`,
 * `ERTDisplacementBlockReason` e `ERTMatchEndReason`.
 */
UENUM(BlueprintType)
enum class ERTGeometryViolation : uint8
{
	/** Il segmento appartiene alla grammatica. */
	None,

	/**
	 * L'asse non e' uno dei sei. Nella rappresentazione discreta il fuori-asse non si disegna: si
	 * DESERIALIZZA — un asset scritto da una versione futura, o un dato corrotto, porta un valore d'enum
	 * che non esiste. E' l'unico modo in cui «geometria fuori asse» puo' ancora entrare, ed e' il motivo
	 * per cui il controllo resta.
	 */
	OffAxis,

	/** Gli estremi coincidono: un segmento che non copre nulla, e che nessuna cottura saprebbe orientare. */
	ZeroLength,

	/** Lo STESSO segmento e' gia' presente nella collezione: due muri sovrapposti sono un muro. */
	DuplicateSegment,

	/** Layer negativo: i piani della mappa partono da zero. */
	InvalidLayer,

	/** Una coordinata supera `RT_GeometryMaxQuanta` dal centro della cella. */
	OutsideEditableBounds
};

/**
 * UN SEGMENTO DI GEOMETRIA D'AUTHORING, e l'authority della grammatica — `D-127`.
 *
 * Interamente DISCRETO: un enum e tre interi. Non contiene, e non deve contenere, alcun estremo in virgola
 * mobile: `FRTOccupancyPolyline` — che di float ne ha — e' il DERIVATO di calcolo, prodotto da `ToPolyline`
 * e passato a `ComputeMask`. Il float esiste per DISEGNARE, non per decidere, ed e' la §11 di
 * `docs/technical/spec-hex-geometry-authoring.md`.
 *
 * `Along` e `Offset` sono in quanti (`RT_GeometryQuanta`), misurati rispetto ai punti notevoli delle due
 * direzioni: `Along` lungo l'asse, `Offset` lungo la sua perpendicolare ruotata di `-90` gradi.
 */
USTRUCT(BlueprintType)
struct FRTGeometrySegment
{
	GENERATED_BODY()

	/** La giacitura. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTTacticalAxis Axis = ERTTacticalAxis::Deg0;

	/** Spostamento perpendicolare all'asse, in quanti. Zero = il segmento passa per il centro della cella. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Offset = 0;

	/** Primo estremo, in quanti lungo l'asse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 AlongStart = 0;

	/** Secondo estremo, in quanti lungo l'asse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 AlongEnd = 0;

	/** Il piano su cui il segmento vive. Stessa semantica di `FRTCellId::Layer`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Layer = 0;

	/**
	 * CHE COSA e' questo muro, non dove sta: `Low` e' il muretto, `High` il muro pieno.
	 *
	 * Aggiunto da `#621`, e la divisione e' voluta: `#620` ha fissato la GRAMMATICA — dove un segmento puo'
	 * stare — mentre il tipo e' la SEMANTICA che la cottura consuma per scegliere fra `FRTHexCover{Low}` e
	 * `{High}`. Senza, il bake dovrebbe indovinare.
	 *
	 * Riusa `ERTHexCoverType` invece di introdurre un `ERTWallKind` parallelo: i due valori canonici esistono
	 * gia', con le definizioni scritte in `RTHexCellData.h` e l'integrita' di catalogo (30 / 50). Un secondo
	 * enum con gli stessi due valori sarebbe la «seconda rappresentazione dello stesso oggetto» che il corpo
	 * di `#621` vieta esplicitamente.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexCoverType WallType = ERTHexCoverType::High;

	bool operator==(const FRTGeometrySegment& Other) const
	{
		return Axis == Other.Axis
			&& Offset == Other.Offset
			&& Layer == Other.Layer
			// Un segmento e' lo STESSO segmento anche percorso al contrario: gli estremi sono una coppia
			// non ordinata, e senza questo `DuplicateSegment` sarebbe aggirabile scambiandoli.
			&& FMath::Min(AlongStart, AlongEnd) == FMath::Min(Other.AlongStart, Other.AlongEnd)
			&& FMath::Max(AlongStart, AlongEnd) == FMath::Max(Other.AlongStart, Other.AlongEnd);
	}
};

/** Dove sta una violazione, in una collezione. Lo strato che SEGNALA restituisce questi. */
USTRUCT(BlueprintType)
struct FRTGeometryIssue
{
	GENERATED_BODY()

	/** Indice del segmento nella collezione validata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 SegmentIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTGeometryViolation Violation = ERTGeometryViolation::None;
};

/**
 * La grammatica quantizzata delle direttrici, e il validator che la rende una regola — `#620`.
 *
 * Vive nel modulo RUNTIME e non nell'editor: in `Source/RefactorTacticsEditor/` non esiste alcun test, e
 * una regola che nessun test puo' far cadere non e' una regola. L'editor la CHIAMA. Stessa ragione per cui
 * `URTHexOccupancyLibrary` sta qui pur servendo soprattutto l'authoring.
 *
 * Il rifiuto e' a DUE STRATI, come il precedente del repository: `ValidateSegment` RIFIUTA — e' quello che
 * un tool chiama prima di accettare il gesto — mentre `Validate` SEGNALA l'elenco completo su una
 * collezione, come fa `URTHexMapAsset::ValidateMap`, senza bloccare.
 */
UCLASS()
class REFACTORTACTICS_API URTGeometryGrammarLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'indice del punto di confine che DA' la direzione dell'asse: `Axis + 1`.
	 *
	 * Esiste come funzione, e non come costante, perche' e' il solo posto in cui la corrispondenza fra i sei
	 * assi e i dodici confini e' scritta.
	 */
	static int32 AxisBoundaryIndex(ERTTacticalAxis Axis);

	/**
	 * La direzione dell'asse in coordinate locali di cella: il punto di confine corrispondente, alla
	 * distanza del suo punto notevole — vertice o punto medio di lato.
	 *
	 * ⚠️ **Non normalizzata, ed e' deliberato**: la sua lunghezza E' il quanto di quella direzione, ed e'
	 * cio' che rende esatto un muro perimetrale. Normalizzarla reintrodurrebbe `sqrt(3)`.
	 */
	static FVector2D AxisPoint(ERTTacticalAxis Axis, float HexSize);

	/**
	 * La perpendicolare all'asse, ruotata di `-90` gradi: il punto di confine `Axis + 1 + 9` modulo dodici.
	 *
	 * Nove passi da 30 gradi sono `-90`. Il verso e' scelto perche' rende positivo l'offset del caso
	 * principale — il muro sul lato `E`, asse `Deg90`, offset verso `0` gradi.
	 */
	static FVector2D AxisPerpendicularPoint(ERTTacticalAxis Axis, float HexSize);

	/** L'asse e' uno dei sei valori dichiarati. Falso su un enum arrivato da una deserializzazione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool IsKnownAxis(ERTTacticalAxis Axis);

	/**
	 * STRATO CHE RIFIUTA: la prima violazione di un segmento preso da solo, `None` se appartiene alla
	 * grammatica. Non puo' rilevare `DuplicateSegment`, che e' una proprieta' della collezione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTGeometryViolation ValidateSegment(const FRTGeometrySegment& Segment);

	/**
	 * STRATO CHE SEGNALA: tutte le violazioni di una collezione, duplicati inclusi, in ordine di indice.
	 * Non blocca e non modifica nulla — l'equivalente di `URTHexMapAsset::ValidateMap` per la grammatica.
	 */
	static void Validate(const TArray<FRTGeometrySegment>& Segments, TArray<FRTGeometryIssue>& OutIssues);

	/**
	 * IL DERIVATO: da segmento discreto a polilinea di calcolo, l'ingresso di
	 * `URTHexOccupancyLibrary::ComputeMask`.
	 *
	 * E' il punto in cui il float nasce, e nasce a VALLE dell'authority. Un segmento non valido produce una
	 * polilinea vuota invece di coordinate arbitrarie: chi non ha validato non ottiene geometria.
	 */
	static FRTOccupancyPolyline ToPolyline(const FRTGeometrySegment& Segment, float HexSize);

	/**
	 * LO SNAP: dai due punti che il mouse ha tracciato al segmento della grammatica piu' vicino.
	 *
	 * E' il cuore del gesto d'authoring (`#712`), e vive QUI e non nell'editor per la ragione di sempre: in
	 * `Source/RefactorTacticsEditor/` non esiste alcun test, e uno snap che nessun test puo' far cadere non e'
	 * una regola ma un'abitudine. Il tool la CHIAMA, e resta sottile.
	 *
	 * Prova tutti e sei gli assi, proietta i due punti sulla base ortogonale (asse, perpendicolare) di
	 * ciascuno — le due direzioni hanno lunghezze diverse, vertice e apotema, quindi la proiezione divide per
	 * il quadrato di ciascuna — arrotonda a quanti interi e tiene l'asse che sbaglia meno.
	 *
	 * Restituisce `false` se il gesto non produce un segmento legale: due punti troppo vicini collassano in
	 * lunghezza zero, e un gesto lontano dalla cella esce dai bordi editabili. In quel caso `OutSegment` non
	 * e' definito, e chi disegna deve mostrare un ghost invalido invece di committare.
	 *
	 * ⚠️ **`Layer` non si deduce dalla geometria**: e' contesto d'editor — il layer attivo — e resta al
	 * chiamante. Qui vale sempre zero.
	 */
	static bool SnapToGrammar(const FVector2D& LocalA, const FVector2D& LocalB, float HexSize,
		FRTGeometrySegment& OutSegment);
};
