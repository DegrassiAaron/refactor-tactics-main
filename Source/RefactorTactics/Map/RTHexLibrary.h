#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h" // ERTHexSurface: serve ai colori dell'overlay di leggibilita'
#include "RTHexLibrary.generated.h"

/**
 * Matematica pura della griglia esagonale pointy-top (assiale/cubica). Deterministica: le coordinate restano
 * intere; il float compare solo nelle conversioni verso/da lo spazio-mondo (rendering/input), col risultato
 * assiale sempre arrotondato a intero (arrotondamento cubico). Nessuna dipendenza da Actor/NavMesh.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Vettore assiale (dq,dr) della direzione esagonale (pointy-top), ordine stabile 0..5. */
	static FIntPoint AxialDirection(ERTHexDirection Dir);

	/** Cella adiacente nella direzione data (stesso layer). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTCellId Neighbor(const FRTCellId& Cell, ERTHexDirection Dir);

	/** I sei vicini orizzontali (stesso layer), in ordine di direzione E..SE. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> Neighbors(const FRTCellId& Cell);

	/** Distanza esagonale (cubica) tra due celle. Ignora il Layer (i piani si collegano con archi espliciti). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 HexDistance(const FRTCellId& A, const FRTCellId& B);

	/**
	 * Direzione da From alla cella ADIACENTE To, sullo stesso layer. `false` (e OutDirection invariata) se le
	 * due celle non sono adiacenti o stanno su layer diversi.
	 *
	 * Rigorosa di proposito: chi deriva un orientamento da due passi consecutivi di un percorso deve
	 * accorgersi se quei passi non sono adiacenti, non ricevere la direzione approssimata piu' vicina.
	 * Per la direzione verso una cella qualunque c'e' `DirectionTowards`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool DirectionBetween(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection);

	/**
	 * Direzione da From verso una cella QUALUNQUE: il primo passo della linea From->To, cioe' la direzione
	 * in cui ci si incamminerebbe. `false` se le due celle coincidono nel piano (distanza 0): non esiste
	 * una direzione, e restituirne una arbitraria sarebbe un dato inventato.
	 * Planare come `HexLine` e `HexDistance`: il Layer non entra nel calcolo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool DirectionTowards(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection);

	/** Centro-mondo della cella (pointy-top): X,Y dal piano assiale, Z = Origin.Z + Layer*LayerHeight. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FVector AxialToWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight);

	/** Cella che contiene il punto-mondo (arrotondamento cubico), sul Layer indicato (layer attivo). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTCellId WorldToAxial(const FVector& World, const FVector& Origin, float HexSize, int32 Layer);

	/** Layer (intero) corrispondente a una quota-mondo Z. LayerHeight<=0 -> 0. RoundToInt = floor(x+0.5). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 WorldToLayer(double WorldZ, double OriginZ, float LayerHeight);

	/**
	 * Cella COMPLETA che contiene il punto-mondo: ricava il Layer dalla quota e poi la coppia assiale su quel
	 * piano. Unico punto da cui passano raycast dell'input e hit-test dell'editor, cosi' la sequenza
	 * "layer poi assiale" non viene ricomposta a mano in ogni chiamante.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTCellId WorldToCellId(const FVector& World, const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * Cella puntata da un raggio **su un piano dichiarato**, con o senza un colpo gia' validato dal chiamante.
	 *
	 * E' la parte calcolabile di `RTHexEditor::ResolveClickedCell`, estratta qui perche' li' non era
	 * verificabile: `Source/RefactorTacticsEditor/` non contiene nessun test, e un modulo Editor non si
	 * esercita headless. Cio' che resta al chiamante e' la parte che ha bisogno del mondo — sparare il
	 * raycast e decidere se il colpo VALE — e cio' che arriva qui e' solo geometria.
	 *
	 * `bHasValidHit` e' la risposta del chiamante a due domande che questa funzione non puo' porsi:
	 * il colpo e' sul componente selezionabile? ed e' sul piano attivo? Se una delle due e' no, il punto
	 * si prende dall'intersezione col **piano del layer**, che e' la geometria giusta per «dove sto
	 * puntando su QUESTO piano» — proiettare un colpo di un altro piano lo sposterebbe in orizzontale di
	 * circa `LayerHeight`, cioe' di alcune celle con la camera obliqua del viewport.
	 *
	 * Pura: stessi argomenti, stessa cella. Nessun `UWorld`, nessun actor, nessuna collisione.
	 */
	static FRTCellId ResolveRayToCellOnLayer(const FVector& RayOrigin, const FVector& RayDirection,
		const FVector& Origin, float HexSize, float LayerHeight, int32 ActiveLayer,
		bool bHasValidHit, const FVector& HitPoint);

	/**
	 * Centro-mondo del **BORDO** fra la cella e il suo vicino nella direzione data: il punto medio fra i due
	 * centri di cella.
	 *
	 * Esiste perche' coperture e porte stanno sui bordi (`FRTHexCover::Edge`, `FRTHexDoor::Edge`) e finora
	 * nessuno sapeva dire *dove* sia un bordo nel mondo. Chi doveva disegnarci qualcosa se lo sarebbe
	 * ricalcolato a modo suo, e la convenzione dei sei lati avrebbe smesso di essere una convenzione.
	 *
	 * **Derivato, non inciso**: il punto viene dai due centri, quindi se `AxialDirection` cambiasse la
	 * geometria seguirebbe invece di mentire. Stessa disciplina che `MakeCoverYardArena` gia' impone ai dati.
	 *
	 * Lo stesso bordo fisico ha lo stesso centro visto dalle DUE celle che lo condividono: `EdgeMidpointWorld(A,
	 * E)` coincide con `EdgeMidpointWorld(vicino a est di A, W)`. E' cio' che impedisce a una copertura di
	 * apparire in due posti diversi a seconda di chi la dichiara.
	 */
	static FVector EdgeMidpointWorld(const FRTCellId& Cell, ERTHexDirection Dir, const FVector& Origin,
		float HexSize, float LayerHeight);

	/**
	 * Rotazione di un pannello posato su quel bordo: l'asse X guarda **verso il vicino**, quindi la larghezza
	 * del pannello corre lungo il bordo.
	 *
	 * Visto dalle due celle che condividono il bordo, lo yaw differisce di 180 gradi — il pannello e' lo
	 * stesso, cambia il verso da cui lo si guarda.
	 */
	static FRotator EdgeRotation(const FRTCellId& Cell, ERTHexDirection Dir);

	/** La direzione opposta (E<->W, NE<->SW, NW<->SE): il bordo condiviso, visto dall'altra cella. */
	static ERTHexDirection OppositeDirection(ERTHexDirection Dir);

	/** Distanza minima tra la semi-retta (RayOrigin + t*RayDir, t>=0) e il segmento A..B. Pura, per hit-test archi. */
	static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B);

	/**
	 * I 6 vertici dell'esagono attorno a Center (pointy-top, primo vertice a -30 gradi), complanari al centro.
	 * Il chiamante chiude il contorno collegando l'ultimo al primo. Condivisa da marker dell'editor e anteprima
	 * in gioco: un solo orientamento, cosi' i due disegni non divergono.
	 */
	static TArray<FVector> HexCorners(const FVector& Center, float Radius);

	/**
	 * Media dei centri-mondo delle celle indicate: il punto da inquadrare per un gruppo (es. la squadra del
	 * giocatore all'avvio). Insieme vuoto -> `Origin`. Indipendente dall'ordine dell'input.
	 */
	/**
	 * Colore con cui l'overlay di leggibilita' disegna una superficie. UNICA definizione: la usano sia il
	 * marker dell'editor sia l'overlay in partita, cosi' la stessa cella non cambia colore fra i due.
	 * Vincolo verificato da test: superfici diverse hanno colori distinguibili fra loro e dal marcatore di blocco.
	 */
	static FColor SurfaceColor(ERTHexSurface Surface);

	/**
	 * Altezza (uu) del RILIEVO con cui l'editor mostra quanto costa attraversare una cella: il profilo della
	 * mappa racconta dove si rallenta, senza aprire un pannello.
	 *
	 * Il costo di riferimento e' `1` — il pavimento — e sta a quota **zero**: il rilievo misura il
	 * *sovrapprezzo*, non il costo assoluto. Una mappa tutta pavimento resta piatta, ed e' giusto: non c'e'
	 * niente da segnalare.
	 *
	 * `ReliefUnitHeight` e' scelto **due ordini di grandezza sotto** `LayerHeight` (250 uu di default): un
	 * rilievo non deve mai poter essere scambiato per un piano. E' il vincolo che tiene separati i due
	 * significati della quota in questa vista.
	 *
	 * Il numero da cui parte non si incide qui: arriva dal catalogo terreni via `FindTerrainDef`, cosi'
	 * ribilanciare `Rough` cambia la mappa da sola invece di lasciarla su un valore morto.
	 */
	static constexpr float ReliefUnitHeight = 15.f;

	static float ReliefHeightForCost(int32 MoveCost);

	/** Colore del marcatore delle celle che BLOCCANO il movimento (esagono interno). */
	static FColor BlockedCellColor();

	/** Colore del marcatore delle celle che bloccano la LINEA DI VISTA (esagono interno, distinto dal blocco). */
	static FColor SightBlockerColor();

	/**
	 * Colori delle celle di PARTENZA delle due squadre (anello esterno, piu' largo del contorno di superficie).
	 *
	 * Non sono una proprieta' della mappa: `URTMatchSetupLibrary::PickStartCells` le DERIVA dalle celle
	 * percorribili, quindi si spostano da sole appena si aggiunge o toglie una cella. Mostrarle e' l'unico modo
	 * di accorgersene mentre si dipinge, invece che dal log a lavoro finito.
	 */
	static FColor SpawnTeam0Color();
	static FColor SpawnTeam1Color();

	/**
	 * Colore delle celle percorribili che **nessuno raggiunge**: una zona irraggiungibile non e' una scelta di
	 * design, e' una mappa rotta — la si vede e non ci si arriva.
	 *
	 * Distinto da tutto il resto perche' dice una cosa di natura diversa: gli altri marcatori descrivono cosa
	 * la cella *e'*, questo che la cella e' **staccata dal resto**.
	 */
	static FColor UnreachableCellColor();

	static FVector CellsCentroidWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
		float LayerHeight);

	/**
	 * Ingombro-mondo di un insieme di celle: il box che le contiene tutte, su tutti i layer.
	 *
	 * Serve a rispondere «fammi vedere TUTTO» (`#623`). Distinto da `CellsCentroidWorld`, che dice *dove
	 * guardare* e non *quanto largo*: inquadrare il centroide a distanza fissa taglia le mappe grandi e
	 * spreca schermo su quelle piccole.
	 *
	 * ⚠️ E' l'**ingombro**, non i centri. Ogni cella contribuisce con l'esagono intero — semi-estensione
	 * `HexSize·√3/2` in X e `HexSize` in Y, che sono i vertici di un pointy-top di circumraggio `HexSize`
	 * (vedi `HexCorners`). Prendere i soli centri taglierebbe mezza cella su ogni bordo della mappa.
	 *
	 * ⚠️ In Z il box copre i **centri** dei layer estremi, non il volume disegnato: blocchi e rilievo hanno
	 * un'altezza che questa funzione non conosce, ed e' un dato di presentazione. Chi inquadra puo'
	 * espandere il box se gli serve margine.
	 *
	 * Insieme vuoto -> box **non valido** (`IsValid == 0`), non un box degenere sull'origine: una mappa
	 * senza celle non ha un'inquadratura, e restituirne una plausibile e' il difetto che `rt.Arena.Check`
	 * esiste per denunciare. Il chiamante deve controllare `IsValid` prima di usarlo.
	 */
	static FBox CellsBoundsWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
		float LayerHeight);

	/** Ordinamento stabile deterministico: Layer, poi X, poi Y. */
	static bool StableLess(const FRTCellId& A, const FRTCellId& B);

	/** Esagono PIENO di raggio N attorno al centro (tutte le celle a distanza <= N, stesso layer). 3N(N+1)+1 celle. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> HexArea(const FRTCellId& Center, int32 Radius);

	/**
	 * Celle attraversate dalla linea A->B, ESTREMI INCLUSI, sul layer di A (linea planare: come HexDistance,
	 * il Layer non entra nel calcolo). Lunghezza = HexDistance(A,B)+1 e celle consecutive sempre adiacenti.
	 * Interpolazione in ARITMETICA INTERA (lerp razionale + arrotondamento cubico sui resti): niente float
	 * nella logica di gioco (invariante #4) e nessuna oscillazione sulle linee che passano sul confine tra
	 * due celle. Tie-break dell'arrotondamento in ordine fisso q -> r -> s (come CubeRound).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> HexLine(const FRTCellId& A, const FRTCellId& B);

	/**
	 * Ventaglio di 120 gradi da From verso Target, profondo Range celle: unione dei due settori esagonali a 60
	 * gradi adiacenti alla direzione principale (il primo passo della linea From->Target). From e' ESCLUSO;
	 * output ordinato con StableLess (deterministico). Target == From o Range <= 0 -> vuoto.
	 * Copertura: 3 celle a distanza 1, 5 a distanza 2, ecc.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> HexCone(const FRTCellId& From, const FRTCellId& Target, int32 Range);

};
