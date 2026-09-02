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

	/**
	 * I sei vertici-mondo della cella, in ordine stabile a partire da -30 gradi e in senso antiorario,
	 * complanari al centro (stesso Z). Il raggio e' HexSize: la "dimensione" di un pointy-top e' il
	 * circumraggio, cioe' il lato.
	 *
	 * Esiste perche' il Cell Placement Volume (contratto graybox §5) dev'essere DERIVATO dalla cella
	 * logica e non ricalcolato altrove: due celle adiacenti condividono esattamente due di questi
	 * vertici, e una guida d'authoring che non tassella e' peggio di nessuna guida. Chi disegna il
	 * prisma chiama questa, non riscrive la trigonometria in Blueprint.
	 *
	 * Non contiene geometria propria: compone `AxialToWorld` e `HexCorners`, che restano gli unici due
	 * posti in cui la griglia e la convenzione pointy-top sono scritte.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FVector> CellCorners(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * Transform con cui posare il prisma di `ARTHexMapActor::GetCellPrismMesh` come volume della cella:
	 * posizione al centro, nessuna rotazione, scala che porta quella mesh — circumraggio e mezza-altezza
	 * `RTCellPrismRadius` — alle misure della cella.
	 *
	 * `PlanarFraction` vale `1.0` per il footprint esterno e meno per il safe footprint (`GBX-1`). Agisce
	 * **solo sulla pianta**: l'inset e' una frazione di `C` e misura il ritrarsi dai vicini, mentre
	 * l'altezza ha il proprio denominatore (`H`) — sono i due denominatori di §6, e confonderli e' il
	 * difetto che `D-168` ha corretto. Se l'inset toccasse anche Z i volumi smetterebbero di tassellare
	 * in verticale.
	 *
	 * Vive qui e non nel Blueprint perche' e' il punto in cui la convenzione della mesh incontra quella
	 * della griglia: due numeri che, sbagliati insieme, danno un volume plausibile e falso.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FTransform CellVolumeTransform(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight, float PlanarFraction);

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

	/**
	 * Orientamento-mondo di una delle sei direzioni: l'asse X guarda dove sta il vicino.
	 *
	 * E' `EdgeRotation` **senza la cella**, ed esiste per due ragioni misurate (#1992).
	 *
	 * 🔑 **La prima: `EdgeRotation` non e' chiamabile da Blueprint** — e' una `static` nuda. Un asset in
	 * `Content/` che debba orientare qualcosa secondo `ERTHexDirection` non ha altra strada che incidersi
	 * sei angoli, cioe' aprire una **seconda** convenzione dei sei lati. E' il difetto di `#712`, dove due
	 * numerazioni divergenti scrivevano la copertura sul lato sbagliato per quattro bordi su sei.
	 *
	 * ⚠️ **La seconda: scartare la cella e' lecito, e non e' ovvio.** Lo e' perche' `AxialToWorld` e'
	 * **affine** in `(q,r)`: lo spostamento fra due centri dipende solo dal delta assiale, mai da dove si
	 * parte. Se quella funzione smettesse di esserlo — un'origine per layer, una deformazione — questa
	 * scorciatoia mentirebbe in silenzio. Il guardiano e'
	 * `RefactorTactics.Hex.FacingRotationIsCellIndependent`, che lo **misura** su celle sparse invece di
	 * affermarlo.
	 *
	 * ⛔ Non e' una seconda risposta: **delega** a `EdgeRotation` e non ricalcola nessuna trigonometria.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRotator FacingRotation(ERTHexDirection Facing);

	/**
	 * Da dove **parte** il marker che mostra il facing di un corpo cilindrico: sulla SUPERFICIE del corpo,
	 * alla quota della faccia.
	 *
	 *     MarkerOrigin = UnitCenter + Forward(Facing) * BodyRadius + Up * FaceHeight
	 *
	 * 🔑 **Perche' non dal centro** (#1992). Un marker che parte dal centro attraversa il corpo: da vicino
	 * lo si vede spuntare da dentro, e a camera tattica la sua lunghezza apparente include il raggio del
	 * corpo — cioe' due corpi con la stessa lunghezza di marker ma raggio diverso sembrano guardare a
	 * distanze diverse. Con l'origine sulla superficie la lunghezza del marker torna a essere **una
	 * misura** invece della somma di due cose.
	 *
	 * ⚠️ **La lunghezza del marker NON e' un parametro, ed e' deliberato.** Tenerla fuori dalla firma rende
	 * vera *per costruzione* la proprieta' che l'authoring pretende — cambiando `BodyRadius` l'origine si
	 * sposta e la lunghezza non cambia. Passarla qui la degraderebbe a promessa da verificare.
	 *
	 * ⚠️ `Up` e' il verso del **mondo**, non del corpo: le sei origini stanno tutte alla stessa quota. Un
	 * offset ruotato in blocco darebbe lo stesso risultato finche' la rotazione e' solo yaw, e comincerebbe
	 * a mentire il giorno in cui non lo fosse — `RefactorTactics.Hex.FacingMarkerOriginKeepsWorldUp`.
	 *
	 * `BodyRadius = 0` degenera al centro, alla quota `FaceHeight`: e' il caso limite che distingue questa
	 * formula da una che le somiglia.
	 *
	 * ⛔ **Geometria di presentazione, non di gioco**: nessuna cella, nessun costo, nessuna occupancy.
	 * Vive qui — accanto a `CellVolumeTransform`, che fa lo stesso mestiere per i volumi d'authoring —
	 * perche' e' il punto in cui la convenzione dei sei lati incontra un corpo da disegnare, e perche' un
	 * asset in `Content/` deve poterla chiamare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FVector FacingMarkerOrigin(ERTHexDirection Facing, const FVector& UnitCenter,
		float BodyRadius, float FaceHeight);

	/**
	 * Quale dei sei bordi e' piu' vicino a un punto-mondo, visto DA quella cella (#1864).
	 *
	 * Serve a un click: il viewport da' un punto, e la selezione ragiona per `(Cella, Bordo)`.
	 *
	 * ⚠️ **Confronta le distanze dai sei `EdgeMidpointWorld`, e non ricava un angolo.** La convenzione dei
	 * sei lati vive gia' li' dentro; riscriverla come `Atan2` diviso in spicchi significherebbe averne due,
	 * e il giorno in cui una cambiasse mentirebbero in silenzio. E' lo stesso criterio per cui
	 * l'orientamento di un pannello viene da `EdgeRotation` invece che da un angolo inciso a mano.
	 *
	 * ⚠️ **La quota non conta**: il confronto e' in pianta. Un click arriva da una camera che guarda dall'alto
	 * e il punto sul terreno puo' stare a un'altezza qualsiasi rispetto al centro del bordo — includere `Z`
	 * farebbe dipendere il lato scelto dall'inclinazione della camera.
	 *
	 * Il centro esatto della cella non appartiene a nessun bordo piu' che a un altro: la risposta e'
	 * arbitraria ma **deterministica**, perche' un click al centro non deve far saltare la selezione fra
	 * lati diversi a ogni tentativo.
	 */
	static ERTHexDirection NearestEdgeDirection(const FRTCellId& Cell, const FVector& WorldPoint,
		const FVector& Origin, float HexSize, float LayerHeight);

	/** Distanza minima tra la semi-retta (RayOrigin + t*RayDir, t>=0) e il segmento A..B. Pura, per hit-test archi. */
	static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B);

	/**
	 * I 6 vertici dell'esagono attorno a Center (pointy-top, primo vertice a -30 gradi), complanari al centro.
	 * Il chiamante chiude il contorno collegando l'ultimo al primo. Condivisa da marker dell'editor e anteprima
	 * in gioco: un solo orientamento, cosi' i due disegni non divergono.
	 */
	static TArray<FVector> HexCorners(const FVector& Center, float Radius);

	/**
	 * PONTE FRA LE DUE NUMERAZIONI DEI BORDI, che nel progetto sono due e non coincidono.
	 *
	 * - **geometrica**: il bordo `k` va da `HexCorners[k]` a `HexCorners[k+1]`, punto medio a `60k` gradi.
	 *   La usano il perimetro, `SectorBoundaryPoints` e il ghost del Geometry tool.
	 * - **di vicinato**: `ERTHexDirection(j)` e' la direzione di `AxialDirection(j)`, cioe' dove sta il
	 *   vicino. La usano `Neighbor`, `NeighborAcross`, le porte, il combattimento e `FRTHexCover::Edge`.
	 *
	 * Girano in verso opposto: `E` e `W` coincidono, i quattro diagonali sono scambiati a coppie
	 * (`NE↔SE`, `NW↔SW`). Il difetto che ha reso necessarie queste due funzioni e' `#712`: la cottura
	 * faceva un `static_cast` da una all'altra e scriveva la copertura sul lato sbagliato per quattro
	 * bordi su sei — e i test non lo vedevano perche' usavano solo `E` e `W`, i due punti fissi.
	 *
	 * ⚠️ **Chi converte deve chiamare queste, non riscrivere `(6 - k) % 6`.** Due copie della stessa
	 * formula sono esattamente il modo in cui il difetto e' nato.
	 * `RefactorTactics.Hex.EdgeIndexMatchesNeighbourDirection` le verifica derivandole dal mondo.
	 */
	static ERTHexDirection DirectionForEdgeIndex(int32 EdgeIndex);

	/** L'inverso di `DirectionForEdgeIndex`. E' la stessa operazione: il rispecchiamento e' un'involuzione. */
	static int32 EdgeIndexForDirection(ERTHexDirection Dir);

	/**
	 * La frazione dell'INRAGGIO sotto la quale non si punta niente — la dead-zone al centro della cella.
	 *
	 * Ha un numero e non l'aggettivo *«configurabile»*, ed e' una richiesta esplicita di `#1615`: una
	 * dead-zone senza valore e' una decisione rimandata, e il primo che ne ha bisogno ne sceglie uno diverso.
	 * Sull'inraggio e non su `HexSize` perche' e' la distanza dal centro al lato piu' vicino: e' li' che la
	 * cella e' piu' stretta, ed e' quella la misura di *«quanto sono vicino al centro»*.
	 */
	static constexpr float PointingDeadZoneFraction = 0.25f;

	/**
	 * IL SETTORE DI PUNTAMENTO `0..11` sotto un punto, in coordinate LOCALI di cella. `INDEX_NONE` = dead-zone.
	 *
	 * 🔑 **Non e' una terza tassonomia di «settore»**: e' lo stesso partizionamento che
	 * `URTHexOccupancyLibrary::SectorBoundaryPoints` fissa — dodici triangoli `(centro, P[k], P[k+1])` con
	 * `P[k]` a `-30 + 30k` gradi — con una domanda diversa. L'occupancy chiede *quanto* di quel triangolo e'
	 * invaso da geometria; questa chiede *in quale* triangolo cade un punto. Vedi
	 * `spec-pointer-interaction.md` §4.9.
	 *
	 * ⚠️ **Locale e non world, ed e' la ragione per cui la firma e' questa**: il settore non deve dipendere
	 * da dove sta la cella. Con un punto world la stessa geometria darebbe risposte diverse a `(0,0)` e a
	 * `(5000, 3000)` per errore di arrotondamento, e il test di traslazione che `#1615` chiede non avrebbe
	 * nulla da ancorare. Chi ha un punto world sottrae `AxialToWorld` del centro.
	 *
	 * ⛔ **I dodici settori NON sono direzioni.** L'adiacenza resta a sei: il ponte e' `EdgeIndex =
	 * SectorIndex / 2` seguito da `DirectionForEdgeIndex`, fissato da [D-243]. Ed e' a **senso unico** — da
	 * una direzione non si torna a un settore, perche' due settori ne condividono una.
	 *
	 * `HexSize <= 0` -> `INDEX_NONE`: senza scala non c'e' geometria, e la dead-zone non e' calcolabile.
	 */
	static int32 PointingSectorAt(const FVector2D& LocalPoint, float HexSize);

	/**
	 * La porzione di un gesto che cade dentro UNA cella, nelle coordinate locali di quella cella.
	 *
	 * La grammatica dei muri e' definita per cella: `SnapToGrammar` ragiona sui punti notevoli di un
	 * esagono, e non sa niente dei suoi vicini. Un muro tracciato lungo tre celle e' quindi **tre**
	 * segmenti, uno per cella, e questo tipo e' come si dicono.
	 */
	struct FRTCellSegment
	{
		FRTCellId Cell;
		FVector2D LocalStart = FVector2D::ZeroVector;
		FVector2D LocalEnd = FVector2D::ZeroVector;
	};

	/**
	 * TAGLIA UN GESTO IN UN SEGMENTO PER OGNI CELLA CHE ATTRAVERSA.
	 *
	 * ⚠️ Nasce da un limite visto a schermo (`#712`, seduta `U22`): *«non si estende oltre il primo
	 * esagono»*. Il tool cuoceva soltanto la cella della pressione, quindi un muro lungo ne riempiva una e
	 * si fermava — e dopo l'aggancio ai punti notevoli anche la geometria restava confinata li', perche'
	 * quei punti sono di quella cella.
	 *
	 * Ogni porzione e' **ritagliata sull'esagono** della propria cella, quindi i suoi estremi cadono sul
	 * perimetro: sono gia' i punti che `SnapToGrammar` sa agganciare, ed e' cio' che rende la catena
	 * continua invece di lasciare buchi fra una cella e l'altra.
	 *
	 * Ordine di percorrenza dal primo estremo al secondo, e deterministico: chi cuoce non deve dipendere
	 * dall'ordine di iterazione di niente.
	 *
	 * Le porzioni piu' corte di `MinLength` sono scartate — un gesto che sfiora l'angolo di una cella non
	 * ci disegna un muro lungo zero.
	 */
	static void SplitSegmentAcrossCells(const FVector2D& WorldStart, const FVector2D& WorldEnd,
		const FVector& Origin, float HexSize, int32 Layer, float MinLength, TArray<FRTCellSegment>& Out);

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
	 * Il SECONDO canale di una superficie: quanti anelli concentrici incide il suo glifo (`D-183`).
	 *
	 * `0` significa «nessun glifo», non «glifo vuoto»: cinque superfici su nove restano mono-canale, ed e' una
	 * scelta dichiarata nel criterio 1 di `#956` — i quattro segni chiudono le collisioni misurate che le
	 * riguardano.
	 *
	 * ⚠️ **DERIVATO dalla superficie, mai memorizzato sulla cella.** E' cio' che tiene insieme il canale forma
	 * con il vincolo «nessun campo nuovo, nessuna migrazione di formato»: se diventasse un dato, l'hash della
	 * mappa cambierebbe e ogni `.uasset` andrebbe risalvato.
	 *
	 * Intero e non float perche' il gate lo asserisce: due superfici si separano se il colore le separa OPPURE
	 * se questo numero differisce, e un confronto fra interi non ha tolleranze da scegliere.
	 */
	static int32 SurfaceRingCount(ERTHexSurface Surface);

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

	/**
	 * Come sopra, ma tenendo conto della **quota d'autore** di ciascuna cella (`FRTHexCellData::Height`).
	 *
	 * Esiste perche' l'overload su `FRTCellId` e' corretto per cio' che riceve — un id non porta la quota —
	 * e chi inquadra invece la conosce. `ARTHexMapActor::RebuildInstances` disegna ogni cella a
	 * `World.Z + Height`: una mappa con celle alzate produrrebbe un box piatto sul piano del layer, e
	 * l'inquadratura taglierebbe proprio le celle piu' alte.
	 *
	 * ⚠️ `Height` e' **rendering** e non logica — lo dichiara il suo commento, e la logica usa `Layer` piu'
	 * archi. Sta qui lo stesso perche' la domanda «fammi vedere tutto» e' una domanda di rendering: il box
	 * deve contenere cio' che si VEDE, non cio' che il resolver considera.
	 *
	 * 🔵 **Al 2026-08-17 nessun produttore scrive quel campo**: misurato, l'unica assegnazione in `Source/`
	 * sta in `RTHexDoorTests.cpp`. Questo overload e' quindi una difesa contro un difetto **latente**, non
	 * la correzione di uno osservato — ed e' scritto ora perche' costa due righe adesso e un'inquadratura
	 * sbagliata il giorno in cui un pennello imparera' ad alzare una cella.
	 */
	static FBox CellsBoundsWorld(const TArray<FRTHexCellData>& Cells, const FVector& Origin, float HexSize,
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
