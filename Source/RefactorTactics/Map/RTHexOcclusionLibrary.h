#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "RTHexOcclusionLibrary.generated.h"

class URTHexMapAsset;

/**
 * L'UNITA' DI MISURA DELLA GEOMETRIA ESATTA: un quarantottesimo di `HexSize`.
 *
 * `48 = 4 * 12`. Il `4` viene dagli anchor — le loro coordinate locali sono multipli interi di `R/4` una
 * volta fattorizzata la radice di tre — e il `12` da `RT_GeometryQuanta`, il passo con cui la grammatica
 * misura `Along` e `Offset`. Con questo denominatore **ogni** punto di cui questa libreria ha bisogno cade
 * su interi, e non ne serve uno piu' fine.
 */
static constexpr int64 RT_OcclusionQuanta = 4 * RT_GeometryQuanta;

/**
 * UN PUNTO IN COORDINATE LOCALI DI CELLA, ESATTO, SENZA VIRGOLA MOBILE.
 *
 * ## 🔑 Perche' due interi e non una `FVector2D`
 *
 * La LoS entra negli esiti di combattimento e quindi nell'hash di stato: un confronto che dipende
 * dall'ordine delle operazioni in virgola mobile e' una divergenza di replay che aspetta la piattaforma
 * sbagliata. Qui non c'e' epsilon da tarare perche' non c'e' float.
 *
 * ## La forma, e perche' e' sufficiente
 *
 * Il punto vale `(R3 * radice-di-tre, One)` in unita' di `HexSize / RT_OcclusionQuanta`. **Tutti** i punti
 * che servono hanno questa forma:
 *
 * - i sei VERTICI valgono `(±2, ±2)`, `(0, ±4)` in unita' `R/4` — cioe' `R3 = ±2, One = ±2` e simili;
 * - i sei PUNTI MEDI di lato valgono `(±2, 0)`, `(±1, ±3)` nelle stesse unita';
 * - un punto della grammatica e' `Perp * Offset / 12 + Along * AlongQ / 12` su due di quei punti notevoli,
 *   quindi resta della stessa forma una volta scalato di `12`.
 *
 * ## Perche' la radice non si calcola mai
 *
 * Il prodotto vettoriale fra due vettori di questa forma vale
 *
 * ```text
 * cross( (A1*r3, B1), (A2*r3, B2) ) = r3 * (A1*B2 - A2*B1)
 * ```
 *
 * — un intero moltiplicato per una costante **positiva**. Per un test di orientamento conta solo il segno,
 * e il segno e' quello dell'intero: la radice si semplifica e non entra mai in un confronto.
 */
struct FRTLocalPointQ
{
	/** Coefficiente della radice di tre, in unita' `HexSize / RT_OcclusionQuanta`. */
	int64 R3 = 0;

	/** Coefficiente razionale, nelle stesse unita'. */
	int64 One = 0;

	FRTLocalPointQ() = default;
	FRTLocalPointQ(int64 InR3, int64 InOne) : R3(InR3), One(InOne) {}

	bool operator==(const FRTLocalPointQ& O) const { return R3 == O.R3 && One == O.One; }
	bool operator!=(const FRTLocalPointQ& O) const { return !(*this == O); }
};

/**
 * LA GEOMETRIA INTRA-CELLA CHE FERMA VISTA E PROIETTILI — `D-269`, `D-270`, `#1830`.
 *
 * ## 🔴 Perche' NON e' `URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal`
 *
 * La domanda sembra la stessa e non lo e'. Quella funzione risponde per **connettivita'**: due settori
 * stanno nella stessa regione libera se esiste un percorso fra loro *girando attorno* alla geometria — ed e'
 * la definizione giusta per la POSA, dove un'unita' che gira attorno a un raggio centro-vertice ci arriva
 * davvero.
 *
 * **La vista non gira attorno a niente.** Quello stesso raggio lascia i settori nella stessa regione
 * (`SameRegion`) e taglia in due la retta che passa per il centro della cella: riusare la connettivita'
 * renderebbe quel muro TRASPARENTE, che e' esattamente il caso per cui `D-269` esiste.
 *
 * ∴ Il modello unico che `D-270` chiede e' la **rappresentazione**, non il predicato: `InteriorWalls` e la
 * grammatica di `FRTGeometrySegment` restano l'unica autorita' sulla geometria intra-cella, interrogata da
 * un posto solo — questo — e sopra ci vivono due predicati perche' le domande sono due.
 *
 * ## ⛔ Pura e headless
 *
 * Nessuno stato, nessun Actor, nessun `UWorld`, nessuna `SphereOverlap`, nessuna autorita' da `NavMesh` o
 * dalla mesh di rendering. Chiamabile dai test e dal modulo Editor.
 */
UCLASS()
class REFACTORTACTICS_API URTHexOcclusionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * LA GEOMETRIA INTERNA DI `Cell` FERMA LA LINEA che la attraversa venendo da `Prev` e andando a `Next`?
	 *
	 * 🔑 **La convenzione degli estremi**: `Prev == Cell` significa *«la linea NASCE qui»* (il tiratore), e
	 * `Next == Cell` *«la linea FINISCE qui»* (il bersaglio). In entrambi i casi quel capo della corda e' il
	 * CENTRO della cella. E' il modo per dire «nessun vicino» senza un puntatore e senza un booleano in piu',
	 * e rende la firma chiamabile da Blueprint.
	 *
	 * Blocca solo un muro `High` (`D-271`: `Low` e' copertura direzionale parziale, non occlusione) sul layer
	 * della cella, e solo se la corda lo **incrocia propriamente**: sfiorarlo in un estremo o scorrergli
	 * accanto in collinearita' non e' attraversarlo.
	 *
	 * ⚠️ Mappa nulla, celle non adiacenti, corda degenere: **non blocca**. E' lo stesso verso *fail-open* con
	 * cui `DescribeLineOfSight` tratta una cella assente dall'asset — il vuoto non e' un muro.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool BlocksSight(const URTHexMapAsset* Map, const FRTCellId& Prev, const FRTCellId& Cell,
		const FRTCellId& Next);

	/**
	 * LA CORDA che la linea percorre DENTRO una cella, come coppia di punti esatti.
	 *
	 * ```text
	 * cella INTERMEDIA   :  EdgeMid(lato d'ingresso)  ->  EdgeMid(lato d'uscita)
	 * cella del TIRATORE :  Center                    ->  EdgeMid(lato d'uscita)
	 * cella del BERSAGLIO:  EdgeMid(lato d'ingresso)  ->  Center
	 * ```
	 *
	 * 🔑 **Va dichiarata, perche' in una LoS cella-a-cella non esiste altrimenti un «in mezzo».**
	 * `URTHexLibrary::HexLine` produce una sequenza di CELLE, non una retta: senza questa definizione la
	 * frase «un segmento fra chi tira e chi e' mirato» non ha un luogo geometrico a cui riferirsi, e due
	 * implementazioni oneste sceglierebbero due corde diverse.
	 *
	 * ⚠️ **E' un'APPROSSIMAZIONE, ed e' dichiarata.** La retta euclidea fra i due centri non passa per i
	 * punti medi dei lati quando la linea «gira»; questa corda si'. E' la stessa classe di approssimazione
	 * che la LoS cella-a-cella accetta da sempre, ed e' coerente con essa: il modello discreto resta uno.
	 *
	 * ✅ **Simmetrica per costruzione**: scambiare `Prev` e `Next` scambia i due estremi e lascia lo stesso
	 * segmento, quindi la LoS resta indipendente dall'ordine — che e' un vincolo di `D-269`.
	 *
	 * Falso se i vicini non sono adiacenti alla cella, o se la corda degenera in un punto.
	 */
	static bool ChordThroughCell(const FRTCellId& Prev, const FRTCellId& Cell, const FRTCellId& Next,
		FRTLocalPointQ& OutA, FRTLocalPointQ& OutB);

	/**
	 * I DUE SEGMENTI SI INCROCIANO PROPRIAMENTE? Cioe' in un punto interno a entrambi.
	 *
	 * ⛔ **Tangenza e collinearita' NON contano**, ed e' una scelta, non una dimenticanza: toccare un muro in
	 * un suo estremo non e' attraversarlo, e guardare LUNGO un muro non e' guardarci attraverso. Il verso e'
	 * quello *fail-open* che la LoS gia' usa per la cella assente, e i due casi degeneri hanno un test loro.
	 *
	 * Esatto in aritmetica intera: nessun epsilon, nessuna dipendenza da `HexSize`, nessuna differenza fra
	 * piattaforme. Vedi `FRTLocalPointQ`.
	 */
	static bool SegmentsCrossProperly(const FRTLocalPointQ& P1, const FRTLocalPointQ& P2,
		const FRTLocalPointQ& Q1, const FRTLocalPointQ& Q2);

	/**
	 * I DUE ESTREMI di un segmento della grammatica, esatti.
	 *
	 * `Perp * Offset / 12 + Along * AlongQ / 12` sui due punti notevoli dell'asse, riportato su interi dal
	 * denominatore comune `RT_OcclusionQuanta`. Falso per un asse che non e' fra i sei.
	 */
	static bool SegmentEndpointsQ(const FRTGeometrySegment& Segment, FRTLocalPointQ& OutA,
		FRTLocalPointQ& OutB);

	/**
	 * IL PUNTO DI CONFINE DI SETTORE `Index`, esatto — l'equivalente intero di
	 * `URTHexOccupancyLibrary::SectorBoundaryPoints`.
	 *
	 * ⚠️ **E' una tabella, e una tabella puo' mentire**: e' l'unico punto di questa libreria che non deriva
	 * la convenzione da chi la possiede. Per questo esiste `Occlusion.BoundaryTableMatchesTheFloatOracle`,
	 * che la confronta punto per punto con `SectorBoundaryPoints` — se la convenzione dei dodici confini
	 * cambiasse, il test cade invece di lasciar mentire la tabella.
	 */
	static FRTLocalPointQ BoundaryPointQ(int32 BoundaryIndex);

	/** Il punto locale di un anchor (`ERTAnchorKind`), esatto. */
	static FRTLocalPointQ AnchorPointQ(ERTAnchorKind Kind, int32 Index);

	/** Il punto in coordinate locali reali. **Per DISEGNARE e per i test**, mai per decidere. */
	static FVector2D ToLocal(const FRTLocalPointQ& Point, float HexSize);
};
