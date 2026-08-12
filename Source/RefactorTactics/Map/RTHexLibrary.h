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

	/** Distanza minima tra la semi-retta (RayOrigin + t*RayDir, t>=0) e il segmento A..B. Pura, per hit-test archi. */
	static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B);

	/**
	 * Posa del BORDO `Edge` della cella: posizione al centro del lato, rotazione che guarda VERSO il vicino
	 * oltre quel bordo. La scala la decide il chiamante — qui c'e' dove e verso dove, non quanto grande.
	 *
	 * **Derivata dai due centri di cella, mai dai vertici.** Il punto e' il medio fra `AxialToWorld(Cell)` e
	 * `AxialToWorld(Neighbor(Cell, Edge))`, l'orientamento e' la loro differenza. Sceglierlo invece da due
	 * indici di `HexCorners` richiederebbe di sapere quali due vertici delimitano quel lato: un accoppiamento
	 * inciso che, il giorno in cui la convenzione dei sei lati cambiasse, diventerebbe **silenziosamente il
	 * bordo sbagliato**. E' l'avvertimento che `MakeCoverYardArena` porta gia' scritto, applicato alla
	 * geometria invece che alla direzione.
	 *
	 * Il bordo `E` di una cella **e'** il bordo `W` del suo vicino: stesso punto, versi opposti. E' una
	 * proprieta' verificata (`Hex.EdgeTransformIsDerivedFromCellCenters`), non una coincidenza da cui
	 * dipendere per caso.
	 *
	 * Planare come il resto della famiglia: il bordo resta alla quota del centro cella, quindi appartiene al
	 * `Layer` della cella e non scivola verso il piano vicino.
	 */
	static FTransform EdgeTransform(const FRTCellId& Cell, ERTHexDirection Edge, const FVector& Origin,
		float HexSize, float LayerHeight);

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

	/**
	 * Frazione della larghezza di cella occupata dal rilievo del costo. Piu' stretta della cella per non
	 * coprire il contorno colorato della superficie, che resta il canale del *tipo* di terreno.
	 *
	 * Vive qui e non nell'actor perche' i volumi di blocco devono sapersi confrontare con essa: la colonna
	 * ci sta DENTRO, la lastra la circonda restando piu' bassa. Sono relazioni, e un numero inciso in due
	 * posti le romperebbe in silenzio.
	 */
	static constexpr float ReliefWidthScale = 0.6f;

	/**
	 * I due volumi con cui l'editor rende leggibili le regole di BLOCCO — «forma per la regola», §3b del
	 * brief `brief-editor-map-viz.md`.
	 *
	 * La distinzione che devono servire e' la piu' fraintesa della mappa: una cella che blocca la vista **si
	 * attraversa**, ed e' cio' che serve a una rotta coperta ma percorribile. Due anelli concentrici non la
	 * dicevano; due ingombri diversi si', anche guardando a picco.
	 *
	 * | Regola | Forma | Percio' |
	 * |---|---|---|
	 * | non si passa | stretta e alta — una colonna | il rilievo del costo resta visibile ATTORNO alla base |
	 * | non si vede attraverso | larga e bassa — una lastra | il rilievo del costo la SUPERA e resta visibile |
	 *
	 * Una cella con entrambi i flag riceve entrambe le istanze, concentriche: mostra le due regole che ha,
	 * invece di diventare una terza cosa ambigua.
	 *
	 * ⚠️ Con una sola mesh (`CellMesh`, condivisa con `Cells` e `Relief`) la differenza fra i due e' una
	 * **proporzione**: se in editor non bastasse, la strada e' un ISM per forma. I rapporti che tengono in
	 * piedi la lettura sono pinnati da `Hex.BlockerVolumesSeparateTheTwoRules`, non affidati all'occhio.
	 */
	static constexpr float MovementBlockerHeight = 90.f;
	static constexpr float MovementBlockerWidthScale = 0.42f;
	static constexpr float SightBlockerHeight = 10.f;
	static constexpr float SightBlockerWidthScale = 0.80f;

	/** Spessore del pannello di bordo, in frazione del lato: sottile, perche' un bordo non ha volume proprio. */
	static constexpr float EdgePanelThickness = 0.14f;

	/**
	 * Profilo del pannello con cui l'editor disegna una proprieta' di BORDO:
	 * `X` = spessore (moltiplicatore di `EdgePanelThickness`) · `Y` = quanto del lato occupa (frazione) ·
	 * `Z` = altezza in uu.
	 *
	 * Le coperture hanno due tipi e bastano due altezze. Le **porte hanno quattro stati**, e un solo canale
	 * non li distingue: servono due domande indipendenti, ed e' cosi' che sono state divise —
	 *
	 * | | ci si passa? | e chi la cambia? | profilo |
	 * |---|---|---|---|
	 * | `Open` | si' | riapribile/richiudibile | soglia bassa, lato pieno |
	 * | `Destroyed` | si', **per sempre** | nessuno: e' terminale | soglia bassa, lato **incompleto** — manca un pezzo |
	 * | `Closed` | no | si riapre | pannello pieno |
	 * | `Locked` | no | **non da sola** | pannello pieno e piu' **spesso** |
	 *
	 * L'altezza risponde alla prima domanda, l'ingombro alla seconda: «alto» = non passi, «monco» = rotto,
	 * «spesso» = non lo apri tu. ⚠️ La seconda lettura e' piu' debole della prima e va guardata in editor —
	 * e' la ragione per cui i quattro profili sono pinnati come **distinti a coppie**
	 * (`Hex.DoorProfilesTellTheFourStatesApart`) invece che affidati all'occhio.
	 */
	static FVector CoverPanelProfile(ERTHexCoverType Type);
	static FVector DoorPanelProfile(ERTHexDoorState State);

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

	static FVector CellsCentroidWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
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
