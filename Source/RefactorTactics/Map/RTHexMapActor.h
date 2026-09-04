#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge: l'ingresso del velo ([D-227])
#include "Map/RTHexCellData.h"
#include "RTHexMapActor.generated.h"

class UInstancedStaticMeshComponent;
class UPrimitiveComponent;
class UStaticMesh;
class URTHexMapAsset;

/**
 * Modalita' di visualizzazione dei layer (H4): tutti i piani impilati, solo il layer attivo, o il layer attivo
 * in primo piano con gli altri a contorno.
 *
 * I valori vanno aggiunti IN CODA: l'enum e' serializzato negli asset e sui livelli, e inserirne uno in mezzo
 * rimapperebbe le mappe gia' salvate su una modalita' diversa da quella scelta.
 */
UENUM(BlueprintType)
enum class ERTLayerViewMode : uint8
{
	AllLayers,  // mostra tutte le celle di tutti i layer (impilate per quota)
	ActiveOnly, // mostra solo le celle del layer attivo (isola il piano)
	/**
	 * Mesh sul SOLO layer attivo (come ActiveOnly), gli altri piani disegnati a contorno dall'editor mode
	 * (`RTHexEditor::DrawSurfaceOverlay`): si vede cosa c'e' sopra e sotto senza perdere di vista il piano
	 * su cui si sta lavorando.
	 *
	 * I piani di contesto non producono istanze, quindi non hanno collisione: il raycast del click non puo'
	 * colpirli e il pennello non puo' finire sul piano sbagliato.
	 */
	Focus
};

/**
 * Visualizzatore della mappa esagonale: genera un'ISTANZA per cella (ISM), NON un Actor per cella. Nessuna
 * autorita' sui dati: legge le celle da URTHexMapAsset (o genera un graybox demo se l'asset e' assente).
 * La logica (coordinate/pathfinding) resta separata dal rendering.
 */
UCLASS()
class REFACTORTACTICS_API ARTHexMapActor : public AActor
{
	GENERATED_BODY()

public:
	ARTHexMapActor();

	/** Sorgente autorevole delle celle (se assente/vuota, si usa il graybox demo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	TObjectPtr<URTHexMapAsset> MapAsset;

	/** Mesh della cella. Se assente si usa il prisma esagonale di `GetCellPrismMesh`. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	TSoftObjectPtr<UStaticMesh> CellMesh;

	/**
	 * Il prisma esagonale con cui si disegna una cella: mesh **generata in codice**, condivisa da tutte le
	 * istanze e costruita una volta sola.
	 *
	 * ⚠️ Nasce da un difetto visto a schermo nella seduta `U22`: le celle si vedevano come **dischi**, perche'
	 * erano istanze di `/Engine/BasicShapes/Cylinder` mentre il contorno evidenziato veniva da `HexCorners`.
	 * Due percorsi diversi per la stessa forma, quindi il bordo era un esagono e il pieno un cerchio. Qui i
	 * vertici arrivano da `URTHexLibrary::HexCorners`: **la stessa funzione**, non una seconda copia della
	 * convenzione pointy-top, ed e' cio' che impedisce ai due disegni di divergere di nuovo.
	 *
	 * Convenzioni ereditate dal cilindro che sostituisce, per non spostare niente di quanto gia' tarato:
	 * circumraggio **`RTCellPrismRadius`** (`PlanarScale` divide per quello) e Z **centrato**, cioe' le
	 * costanti di [`Map/RTMapVisuals.h`](RTMapVisuals.h) — dove sono uscite dal namespace anonimo con #983.
	 * Cambiarle muoverebbe ogni lift di debug-line insieme al disco.
	 *
	 * 🔵 Generata invece che autorata come `.uasset`: un binario in piu' sarebbe un path da leasare, e due
	 * `.uasset` non si fondono. Questa si diffa e si testa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap",
		meta = (ToolTip = "Il prisma esagonale della cella: circumraggio RTCellPrismRadius (50 uu), mezza-altezza 50 uu, centrato sull'origine. Chi lo posa lo scala."))
	static UStaticMesh* GetCellPrismMesh();

	/**
	 * Mesh del GLIFO di superficie (`#956`, `D-183`): `RingCount` corone esagonali concentriche, piatte.
	 *
	 * E' il secondo canale della board — colore E forma — e si legge a picco come **contrasto d'area**: 9,7% /
	 * 18,4% / 26,2% / 32,9% della faccia per uno / due / tre / quattro anelli. E' l'unico criterio che la
	 * seduta U18 ha misurato leggibile dall'alto (`PIE-HEX-VIZ-COSTO` ✅ contro `PIE-HEX-VIZ-BLOCCHI` ❌,
	 * dove «la differenza e' una proporzione che la vista di lavoro azzera»).
	 *
	 * Gli anelli crescono VERSO L'INTERNO dal bordo del disco (`0,95` del raggio), con spessore e gap come
	 * **frazioni del raggio** e non in uu: con `#1155` (`HexSize` -> 150) le proporzioni si conservano, mentre
	 * scritti assoluti il glifo passerebbe dal 5,3% al 3,5% del raggio — il difetto che `D-163` registra per
	 * le altezze.
	 *
	 * `RingCount <= 0` restituisce `nullptr`: cinque superfici su nove sono mono-canale per scelta.
	 * Una cache per conteggio, come `GetCellPrismMesh`, e i vertici chiesti a `URTHexLibrary::HexCorners` —
	 * mai a un secondo `cos(60k-30)` scritto qui, che e' il difetto di `#712`.
	 */
	static UStaticMesh* GetCellGlyphMesh(int32 RingCount);

	/**
	 * L'anello di BORDO della cella (#1758): una corona esagonale piatta sul perimetro, che dice dove una
	 * cella finisce e comincia la vicina.
	 *
	 * 🔴 **Non e' il contorno di `rt.Debug.DrawCells`, ed e' la ragione per cui questa mesh esiste.** Quello
	 * e' una debug-line disegnata nel Tick dietro un comando console, e un giocatore non digita un comando
	 * per sapere dove finisce una cella: questa e' geometria istanziata, accesa per default, che vive nella
	 * board come i glifi.
	 *
	 * Stessa disciplina di `GetCellGlyphMesh`: una sola per processo, `TStrongObjectPtr` invece di
	 * `AddToRoot`, e i vertici chiesti a `URTHexLibrary::HexCorners` — mai un secondo `cos(60k-30)` scritto
	 * qui, che e' il difetto che `#712` ha pagato a schermo con un bordo esagonale su un pieno circolare.
	 */
	static UStaticMesh* GetCellBorderMesh();

	/**
	 * La trasformazione del pannello di un muro interno, dai due estremi del segmento in coordinate LOCALI
	 * alla cella.
	 *
	 * 🔴 Sta qui, pura e statica, perche' la prima stesura era dentro `RebuildInstances` e sbagliava di
	 * **90 gradi**: metteva lo yaw LUNGO il muro, mentre nella convenzione dei pannelli (cubo engine da
	 * 100 uu) la X e' lo SPESSORE e la Y la lunghezza — quindi il muro veniva disegnato di traverso rispetto
	 * al gesto. L'errore e' della stessa famiglia di tutti gli altri di `#712`: due convenzioni che devono
	 * accordarsi e nessuna asserzione che le tenga insieme.
	 *
	 * `EdgeRotation` fa la stessa cosa per i bordi e la fa gia' giusta — il suo asse X punta al VICINO, cioe'
	 * perpendicolare al bordo — ma non e' riusabile qui: deriva l'angolo dai due centri di cella, e un muro
	 * interno non ha nessun vicino da guardare.
	 *
	 * `RefactorTactics.HexMap.InteriorWallPanelFollowsTheSegment` lo verifica.
	 */
	static FTransform InteriorWallPanel(const FVector2D& LocalA, const FVector2D& LocalB,
		const FVector& CellCentreWorld, float PanelHeight, float PanelThickness);

	/**
	 * Materiale delle celle: legge i tre `PerInstanceCustomData` che `RebuildInstances` scrive e li usa come
	 * colore, cosi' ogni cella si legge per superficie. Se assente, l'ISM tiene il materiale della mesh e le
	 * celle restano grigie — degrado silenzioso, non un errore.
	 *
	 * ⚠️ Sta QUI e non nel livello, benche' il materiale si possa assegnare anche al componente in editor:
	 * `MapSource = GeneratedTestArena` costruisce la mappa a runtime e non ha nessun livello dove assegnare
	 * niente. Un'assegnazione fatta solo su `L_HexArena` lascerebbe grigie proprio le sessioni U2..U6, che
	 * sull'arena d'autore non girano.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	TSoftObjectPtr<UMaterialInterface> CellMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Core/Grid/M_HexCell.M_HexCell")));

	/** Dimensione esagono (cm) usata se MapAsset e' assente. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	float HexSize = 150.f;

	/** Quota tra layer (cm) usata se MapAsset e' assente. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	float LayerHeight = 250.f;

	/** [H4] Layer attivo: usato per il filtro di visualizzazione e come layer di generazione/painting. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Layer")
	int32 ActiveLayer = 0;

	/** [H4] Come mostrare i layer: tutti impilati, solo quello attivo, o l'attivo con gli altri a contorno. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Layer")
	ERTLayerViewMode LayerView = ERTLayerViewMode::AllLayers;

	/**
	 * [Focus] Quanti piani sopra e sotto disegnare come contorno di contesto (0 = nessuno, come ActiveOnly).
	 * Oltre questa distanza dal layer attivo il piano non viene disegnato: su una mappa alta il contesto
	 * completo e' rumore, e quello che serve mentre si dipinge sono i piani vicini.
	 *
	 * Non ha effetto fuori da `Focus`, e non tocca le istanze: e' un parametro di sola presentazione.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Layer", meta = (ClampMin = "0", ClampMax = "8"))
	int32 GhostLayerRange = 2;

	/** Se MapAsset e' assente/vuoto, genera un esagono pieno di questo raggio (0 = niente demo). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	int32 DemoRadius = 4;

	/** Superficie assegnata dal generatore editor (GenerateIntoAsset). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	ERTHexSurface DemoSurface = ERTHexSurface::Floor;

	/** [Editor] Genera nell'asset assegnato un esagono pieno (DemoRadius, DemoSurface), marca dirty e ridisegna. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void GenerateIntoAsset();

	/** [Editor] Svuota le celle e le transizioni dell'asset, marca dirty e ridisegna. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void ClearAsset();

	/**
	 * [Editor] Scrive nell'asset l'**arena della v0.1**: il layout che soddisfa i tre criteri del `done_when`
	 * di U1, verificati da `RefactorTactics.Arena.ArenaV01MeetsAllThreeCriteria` e non a occhio.
	 *
	 * **Sostituisce** il contenuto dell'asset (svuota prima): serve a partire da un layout corretto, non a
	 * fondersi con quello che c'era. Dopo, `rt.Arena.Check` deve dare tre `[ok]`.
	 */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void GenerateArenaV01IntoAsset();

	/**
	 * Nome della fixture da scrivere nell'asset: `ArenaV01`, `RelayBasin`, `RelayLite`, `TestArena`,
	 * `CoverYard`, `BlockYard`, `GrayKitYard`, `VisionSplit`, `ProbeYard`.
	 *
	 * ⚠️ **Completato il 2026-08-31, e ne mancavano tre**: `GrayKitYard`, `VisionSplit` e `ProbeYard` erano
	 * nel dispatcher e non qui. Il test qui sotto verifica **una direzione sola** — che ogni nome elencato
	 * costruisca — quindi un nome che esiste e non e' elencato resta invisibile: non e' rotto, e' solo
	 * introvabile da chi legge il tooltip invece del codice.
	 *
	 * ⚠️ **L'autorita' e' `URTMatchSetupLibrary::KnownFixtureIds()`**, non questa riga: e' un tooltip di
	 * `UPROPERTY`, quindi un literal che UHT deve poter leggere a compile time e che non puo' chiamare una
	 * funzione. Resta percio' un elenco a mano — l'ultimo — e nominava `DemoArena`, che non ha un ramo nel
	 * dispatcher (`#1459`). A tenerlo allineato ci pensa
	 * `RefactorTactics.HexMap.EveryListedFixtureNameBuilds`.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	FString FixtureId = TEXT("ArenaV01");

	/**
	 * [Editor] Scrive nell'asset la fixture indicata da `FixtureId`, **sostituendo** il contenuto.
	 *
	 * Esiste perche' le fixture vivevano solo in codice e nessuno poteva aprirle in editor: `CoverYard` e'
	 * l'unica mappa con una copertura ALTA e `RelayBasin` l'unica con una porta, quindi senza questo pulsante
	 * quei due casi non erano guardabili — e cio' che non si guarda non si verifica.
	 *
	 * Nome sconosciuto -> non tocca nulla e lo dice: meglio un asset invariato che uno svuotato per un refuso.
	 */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void GenerateFixtureIntoAsset();

	/** [Editor] Esegue il validator sull'asset e logga gli errori. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void ValidateAsset();

	/** [Editor] Cella bersaglio del painting per-cella. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	FRTCellId PaintCellTarget;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	ERTHexSurface PaintSurface = ERTHexSurface::Floor;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	int32 PaintMoveCost = 1;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	bool bPaintBlocksMovement = false;

	/** [Editor] Applica superficie/costo/blocco a PaintCellTarget (la crea se assente). Annullabile (Undo/Redo). */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Paint")
	void PaintTargetCell();

	/** Scrive Surface/MoveCost/bBlocksMovement sulla cella Id (la crea se assente, preserva Height/LOS). Annullabile. */
	void PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);

	/** Rimuove la cella Id dall'asset. Vero se esisteva. Annullabile. */
	bool EraseCell(const FRTCellId& Id);

	/** [H4] Cella di partenza della transizione verticale/speciale (bridge/tunnel/scale/ascensore). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	FRTCellId TransitionFrom;

	/** [H4] Cella di arrivo della transizione (tipicamente su un altro layer). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	FRTCellId TransitionTo;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	int32 TransitionCost = 2;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	ERTHexTransitionKind TransitionKind = ERTHexTransitionKind::Stair;

	/** Se vero, crea/rimuove anche l'arco inverso (transizione percorribile nei due sensi). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	bool bTransitionBidirectional = true;

	/** [Editor] Aggiunge la transizione TransitionFrom->TransitionTo (e l'inversa se bidirezionale). Annullabile. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Transition")
	void AddVerticalTransition();

	/** Aggiunge la transizione From->To (e l'inversa se bidirezionale) se entrambe le celle esistono. Annullabile. */
	void AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
		ERTHexTransitionKind Kind, bool bBidirectional);

	/** [Editor] Rimuove la transizione TransitionFrom->TransitionTo (e l'inversa se bidirezionale). Annullabile. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Transition")
	void RemoveVerticalTransition();

	/** Rimuove la transizione From->To (e l'inversa se bBothDirections) dall'asset. Vero se ha rimosso. Annullabile. */
	bool RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections);

	/**
	 * Celle percorribili che nessuno raggiunge dagli spawn. Calcolate **pigramente**: la visita del grafo
	 * avviene alla prima richiesta dopo un cambiamento, non a ogni ricostruzione.
	 *
	 * La pigrizia non e' un'ottimizzazione preventiva. `RebuildInstances` **non** e' il punto in cui la mappa
	 * ha finito di cambiare: il tool Paint la chiama a ogni `OnClickDrag`, cioe' molte volte al secondo mentre
	 * si trascina il pennello. Calcolare li' avrebbe fatto una BFS sull'intero grafo per ogni cella dipinta,
	 * per un dato che serve solo a chi disegna l'overlay — e solo se l'overlay e' acceso.
	 *
	 * Invalidare e' O(1); il calcolo lo paga chi lo guarda.
	 */
	const TArray<FRTCellId>& GetUnreachableCells() const;

	/** Ricostruisce tutte le istanze dalle celle (asset o demo). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RefactorTactics|HexMap")
	void RebuildInstances();

	/**
	 * Cella corrispondente a un'istanza.
	 *
	 * ⚠️ Fuori range risponde `FRTCellId()`, cioe' `(0,0,0)` — una cella **valida**, non un sentinella: chi
	 * chiama non puo' distinguere «origine della mappa» da «indice inesistente», e deve validare l'indice
	 * PRIMA (`URTHexLibrary::PickTargetsSelectableCells` lo fa per il raycast di selezione).
	 */
	FRTCellId CellForInstance(int32 InstanceIndex) const;

	/** Numero di celle attualmente rappresentate (istanze ISM). Diagnostica e test. */
	int32 NumInstanceCells() const { return InstanceCells.Num(); }

	/**
	 * Stende il velo della fog of war sulla board, secondo cio' che UNA squadra sa ([D-225], [D-227]).
	 *
	 * Tre stati e non due, ed e' la conseguenza diretta di [D-227]:
	 *
	 * | stato della cella | resa |
	 * |---|---|
	 * | in `VisibleCells` — osservata ORA | piena luminosita' |
	 * | in `ExploredCells` ma non visibile — **ricordo** | RGB moltiplicato per `RTVeilExploredFactor` |
	 * | in nessuna delle due — mai vista | **non disegnata** ([D-225]) |
	 *
	 * 🔴 **Non e' un costruttore.** `RebuildInstances` resta l'unico, e questa funzione **ricalcola** il colore
	 * da `URTHexLibrary::SurfaceColor` invece di memorizzarlo: un colore cachato sarebbe la seconda verita'
	 * sulla superficie, e le due divergerebbero al primo colpo di pennello.
	 *
	 * ⚠️ **Precondizione dichiarata e verificata.** `InstanceCells` e' stato DERIVATO: un `RebuildInstances`
	 * fra il calcolo del velo e la sua applicazione lascia indici stantii, e l'esito non e' un crash ma
	 * **celle velate sbagliate** — un difetto che si legge come «problema grafico» per settimane. L'`ensure`
	 * costa nulla e lo rende rumoroso.
	 *
	 * ⚠️ **Chi sia il viewer NON lo decide questa firma**: riceve una conoscenza gia' scelta dal chiamante.
	 * In 2v2 offline contro bot e' il team del giocatore; spettatore e replay riporteranno la domanda.
	 *
	 * 🔴 **Copre TUTTE E CINQUE le famiglie di istanze che `RebuildInstances` monta**, non il solo disco:
	 * `Cells`, `SurfaceGlyphs`, `Relief`, `Blockers` ed `EdgeFeatures`. Velare il disco e lasciare in piedi
	 * colonne, lastre e pannelli farebbe leggere muri, coperture e porte dell'INTERA board prima di averla
	 * esplorata — cioe' proprio l'informazione che [D-225] dichiara di non disegnare — e il difetto sarebbe
	 * invisibile a `GetVeilCounts`, che guarda il solo `Cells`.
	 *
	 * ⚠️ **Su quelle tre il velo NASCONDE e basta, non attenua.** Non portano custom data per istanza — nessun
	 * `SetCustomDataValue` in `RebuildInstances` — quindi un ricordo si distingue da un'osservazione solo sul
	 * disco sottostante. Attenuarle richiederebbe un canale nei loro materiali, che questa fetta non apre.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	void ApplyKnowledgeVeil(const FRTTeamKnowledge& Knowledge);

	/**
	 * Quanto resta acceso il terreno RICORDATO ma non osservato. Non e' una preferenza estetica: sotto una
	 * certa soglia il ricordo diventa indistinguibile dal nascosto, e i tre stati tornano due.
	 */
	static constexpr float RTVeilExploredFactor = 0.35f;

#if !UE_BUILD_SHIPPING
	/**
	 * 🔑 **Le quattro frazioni del volume-cella, il vocabolario di forma del debug della conoscenza.**
	 *
	 * Sono frazioni di `H` — l'altezza del volume-cella — come ogni altezza di questo progetto da [D-168].
	 * Con `LayerHeight` al suo default valgono `83 · 125 · 167 · 250` uu.
	 *
	 * ⚠️ **Sono quattro e gli stati sono TRE, e la quarta non ha oggi un significato da dire.** Il mapping
	 * assegnato e' `3/3` osservata · `2/3` ricordata · `1/3` mai vista; `1/2` esiste nel vocabolario perche'
	 * e' stata richiesta, non perche' qualcosa la usi. Inventarle uno stato — «ricordo scaduto», «parziale» —
	 * significherebbe creare un quarto valore che il modello di conoscenza **non ha**: `FRTTeamKnowledge`
	 * porta `VisibleCells` ed `ExploredCells`, e il terzo stato e' l'assenza da entrambe. E' il difetto che
	 * [D-146] registra per le cinque voci che non corrispondevano al modello reale.
	 *
	 * 🔴 **Una sola mesh, quattro scale — non quattro mesh.** I quattro volumi differiscono per la sola
	 * altezza, quindi quattro `FMeshDescription` sarebbero quattro copie della stessa geometria che si
	 * possono desincronizzare. E' la disciplina che il disco della cella gia' segue: `GetCellPrismMesh` e'
	 * una sola, e `RTCellFlatScale` la schiaccia.
	 */
	static constexpr float RTKnowledgeVolumeFractions[4] = { 1.f / 3.f, 1.f / 2.f, 2.f / 3.f, 1.f };

	/** Indici in `RTKnowledgeVolumeFractions`, per non scrivere `0` e `2` dove si intende uno STATO. */
	static constexpr int32 RTKnowledgeVolumeHidden = 0;      // 1/3 — mai vista
	static constexpr int32 RTKnowledgeVolumeRemembered = 2;  // 2/3 — ricordo
	static constexpr int32 RTKnowledgeVolumeLit = 3;         // 3/3 — osservata ORA

	/**
	 * Il prisma esagonale del debug: circumraggio `RTCellPrismRadius`, alto `2 · RTCellPrismRadius` e con il
	 * **PIVOT ALLA BASE** — `Z` in `[0, 2R]`, non centrato come `GetCellPrismMesh`.
	 *
	 * 🔴 **Il pivot e' la meta' del valore dello strumento.** Con un pivot centrato un volume a `1/3`
	 * sporgerebbe di `1/6 H` — 42 uu — SOTTO il pavimento della cella, dentro il layer inferiore: quattro
	 * volumi che non appoggiano sullo stesso piano non sono confrontabili a vista, ed e' l'unica cosa che
	 * questo strumento deve permettere. E' il «pivot contract» di §4 del contratto graybox, applicato qui.
	 */
	static UStaticMesh* GetKnowledgeVolumeMesh();

	/**
	 * Accende o spegne i volumi di conoscenza, ricostruendoli da zero secondo cio' che UNA squadra sa.
	 *
	 * ⚠️ **Non tocca il velo.** `ApplyKnowledgeVeil` continua a decidere la board; questo aggiunge un canale
	 * sopra, e i due si leggono insieme. Spegnendolo la board resta come il velo l'ha lasciata.
	 */
	void SetKnowledgeDebugEnabled(bool bEnabled, const FRTTeamKnowledge& Knowledge);

	/** Se il debug della conoscenza e' acceso. */
	bool IsKnowledgeDebugEnabled() const { return bKnowledgeDebug; }

	/**
	 * Quanti volumi per stato il debug ha posato: mai viste, ricordate, osservate. Per i test.
	 *
	 * ⚠️ **Legge lo stato REALE delle istanze, non un contatore**, ed e' la stessa disciplina di
	 * `GetVeilCounts`: *«un contatore proverebbe che la funzione sa contare, non che ha disegnato»*. La
	 * frazione si ricava dalla scala Z dell'istanza contro `LayerHeight` corrente — non da un letterale,
	 * cosi' il conteggio resta vero anche se la quota fra i piani cambia.
	 */
	void GetKnowledgeDebugCounts(int32& OutHidden, int32& OutRemembered, int32& OutLit) const;

	/**
	 * Quante istanze l'overlay ha DAVVERO posato — diagnostica, e serve a una domanda che i tre conteggi non
	 * possono piu' rispondere (`#2250`).
	 *
	 * 🔑 `GetKnowledgeDebugCounts` risponde `Hidden > 0` per **complemento**, quindi resta vero anche se
	 * l'overlay disegnasse tutto: un test scritto sui suoi numeri non potrebbe accorgersi della regressione.
	 * Questo conta gli oggetti in scena, ed e' l'unico modo di provare che le mai viste non si disegnano.
	 */
	int32 KnowledgeVolumeInstanceCount() const;
#endif

	/** Quante istanze il velo ha lasciato accese, ricordate e nascoste. Diagnostica e test. */
	void GetVeilCounts(int32& OutVisible, int32& OutExplored, int32& OutHidden) const;

	/**
	 * Lo stesso conteggio per le famiglie che NON sono il disco: rilievo del costo, volumi di blocco e
	 * pannelli di bordo. Disegnate contro nascoste, e non tre stati — su queste il velo non attenua.
	 *
	 * ⚠️ Esiste perche' `GetVeilCounts` guarda il solo `Cells`: un velo che nascondesse il disco e lasciasse
	 * in piedi colonne, lastre e pannelli tornerebbe **un conteggio perfetto** mentre rivela muri, coperture
	 * e porte dell'intera board. Sulla graybox il difetto e' invisibile — `MakeFlatArena` non produce nessuna
	 * di queste geometrie — quindi serve un canale che le guardi.
	 */
	void GetAuxiliaryVeilCounts(int32& OutDrawn, int32& OutHidden) const;

	/**
	 * Quante istanze l'ultimo `ApplyKnowledgeVeil` ha davvero TOCCATO.
	 *
	 * ⚠️ E' la misura che rende visibile il salto: senza, un velo che riscrive tutto e uno che riscrive il
	 * bordo del cono sono indistinguibili — stesso risultato a schermo, due ordini di grandezza di
	 * differenza nel costo.
	 */
	int32 GetLastVeilTouchedCells() const { return LastVeilTouchedCells; }

	/** Stati che il velo scrive per istanza. `Unwritten` distingue «mai velata» da «velata e nascosta». */
	static constexpr uint8 RTVeilUnwritten = 0xFF;
	static constexpr uint8 RTVeilHidden = 0;
	static constexpr uint8 RTVeilRemembered = 1;
	static constexpr uint8 RTVeilLit = 2;

	/**
	 * La superficie di una cella, riletta dall'asset. Fuori dall'asset — o sul graybox demo, che non ha
	 * terreni — risponde `Floor`, che e' cio' che `RebuildInstances` disegna nello stesso caso.
	 */
	ERTHexSurface SurfaceForCell(const FRTCellId& Cell) const;

	/**
	 * Filtro layer (H4): solo `AllLayers` impila i piani; `ActiveOnly` e `Focus` tengono solo il layer
	 * attivo. Un'UNICA regola, condivisa da `RebuildInstances` (le istanze) e `RebuildCoordinateLabels`
	 * (le terne incise): due formule per lo stesso filtro sono il difetto che questo file evita gia' per la
	 * geometria dell'esagono (`URTHexLibrary::CellCorners`) — qui vale lo stesso principio.
	 */
	bool PassesLayerFilter(int32 Layer) const;

	/**
	 * Il colpo di un raycast di selezione cade su una cella selezionabile di QUESTO actor?
	 *
	 * Vero solo se e' stato colpito **proprio** il componente delle celle — non un altro componente dello
	 * stesso actor, come il rilievo del costo, che e' geometria di lettura — e se l'indice di istanza che
	 * accompagna il colpo appartiene davvero a quel componente.
	 *
	 * Le due condizioni sono una regola sola: `Result.Item` si riferisce al componente COLPITO, quindi
	 * risolverlo contro un altro componente restituisce una cella valida e sbagliata. Verificare l'ACTOR non
	 * basta, e il difetto non si manifesterebbe come errore ma come pennello che dipinge altrove.
	 */
	bool IsPickOnSelectableCell(const UPrimitiveComponent* HitComponent, int32 InstanceIndex) const;

	/** La mappa esagonale del livello (la prima trovata), oppure nullptr se il livello non ne ha. */
	static ARTHexMapActor* FindInWorld(const UWorld* World);

	/**
	 * Contesto geometrico della mappa: origine = posizione dell'ACTOR, scala (HexSize/LayerHeight) dall'ASSET
	 * autorevole o, se manca, dall'actor stesso (graybox demo). Ritorna l'asset (nullptr se assente).
	 *
	 * UNICO punto da cui passano le conversioni cella<->mondo: risoluzione, playback e input del giocatore
	 * devono condividere la stessa scala, altrimenti l'anteprima e l'esito divergono.
	 */
	const URTHexMapAsset* GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const;

	// --- Anteprima di pianificazione (SOLA PRESENTAZIONE) -------------------------------------------------
	// Invariante #1: questi metodi non decidono nulla. Ricevono cio' che il simulatore ha gia' deciso e lo
	// disegnano. Il disegno e' a debug-line: la presentazione curata (mesh + materiale) e' M8.

	/** Cella sotto il cursore: `bValid` false (o cella non nella mappa) = nessuna evidenziazione. */
	void SetHoveredCell(const FRTCellId& Cell, bool bValid);

	/** Percorso pianificato da evidenziare (vuoto = nessuna traccia). Celle consecutive, partenza inclusa. */
	void SetPreviewPath(const TArray<FRTCellId>& Path);

	/**
	 * Celle colpite dall'attacco pianificato (vuoto = nessuna anteprima). `AllyCells` e' il sottoinsieme
	 * occupato da UNITA' ALLEATE: si disegna in arancione perche' il fuoco amico va visto **prima** del
	 * lock-in, non dedotto dai danni dopo.
	 *
	 * Le celle arrivano gia' calcolate da `URTHexCombatLibrary::HexHitCells` (funzione pura, gia' testata):
	 * qui non si ricalcola nulla, altrimenti anteprima ed esito potrebbero divergere (invariante #1).
	 */
	void SetPreviewHitCells(const TArray<FRTCellId>& HitCells, const TArray<FRTCellId>& AllyCells);

	/**
	 * Celle raggiungibili dall'unita' selezionata col budget corrente (vuoto = nessuna anteprima).
	 * Vengono da `URTHexSimLibrary::ReachableCells`: budget, blocchi, occupanti e archi sono gia' applicati.
	 */
	void SetPreviewReachableCells(const TArray<FRTCellId>& ReachableCells);

	/**
	 * Origine e mira dell'attacco pianificato: da DOVE parte il colpo e verso cosa. `bValid = false` spegne
	 * entrambe.
	 *
	 * 🔴 **L'origine non e' sempre la cella in cui l'unita' si trova ora.** Il ciclo risolve
	 * `Prep -> Dash -> Blast -> Move`: uno scatto pianificato si applica PRIMA degli attacchi, quindi sposta
	 * l'origine di questo turno. La cella la deriva `URTHexCombatLibrary::BlastOriginCell` — qui non si
	 * calcola, si riceve.
	 *
	 * `bOriginPredicted` distingue un'origine confermata (cella corrente) da una prevista (cella dello
	 * scatto, che la collisione simultanea puo' accorciare): il disegno la rende con un tratteggio, cioe' un
	 * canale non cromatico.
	 */
	void SetPreviewAttack(const FRTCellId& OriginCell, const FRTCellId& AimCell, bool bValid,
		bool bOriginPredicted);

	/** Conteggi dell'anteprima (diagnostica e test headless: il disegno non e' verificabile senza schermo). */
	int32 NumPreviewHitCells() const { return PreviewHitCells.Num(); }
	int32 NumPreviewAllyHitCells() const { return PreviewAllyHitCells.Num(); }
	int32 NumPreviewReachableCells() const { return PreviewReachable.Num(); }

	/** Vero se la cella e' fra quelle colpite dall'anteprima corrente (test). */
	bool IsPreviewHitCell(const FRTCellId& Cell) const { return PreviewHitCells.Contains(Cell); }
	/** Vero se la cella e' fra quelle colpite **e** occupata da un alleato (test del fuoco amico). */
	bool IsPreviewAllyHitCell(const FRTCellId& Cell) const { return PreviewAllyHitCells.Contains(Cell); }
	/** Vero se la cella e' fra quelle raggiungibili nell'anteprima corrente (test). */
	bool IsPreviewReachableCell(const FRTCellId& Cell) const { return PreviewReachable.Contains(Cell); }

	/** Cella attualmente evidenziata e sua validita' (diagnostica e test). */
	FRTCellId GetHoveredCell() const { return HoveredCell; }
	bool IsHoveredCellValid() const { return bHoveredValid; }

	/** Numero di celle nella traccia di anteprima (diagnostica e test). */
	int32 NumPreviewPathCells() const { return PreviewPath.Num(); }

	/**
	 * Overlay di LEGGIBILITA': disegna ogni cella col colore della sua superficie, con marcatori distinti per
	 * «blocca il movimento» e «blocca la vista». Serve perche' in partita le celle sono tutte cilindri identici:
	 * fango, ostacoli e muri sono invisibili, e una mappa che non comunica le proprie regole contraddice il
	 * pilastro «leggibilita' tattica».
	 *
	 * E' uno **strumento di sviluppo** (si accende da `rt.Debug.DrawCells`), non la presentazione definitiva:
	 * quella richiede materiali per superficie ed e' M8/M9. Sola lettura: non decide nulla (invariante #1).
	 */
	void SetCellOverlayEnabled(bool bEnabled);
	bool IsCellOverlayEnabled() const { return bCellOverlay; }

	/**
	 * 🔑 **Il toggle della griglia: presentazione, non debug** (#1758).
	 *
	 * ⚠️ **Non e' `SetCellOverlayEnabled` e non gli somiglia**, ed e' l'unica cosa che questa coppia di
	 * funzioni deve dire chiaramente: quello accende un overlay di **debug-line** dietro
	 * `rt.Debug.DrawCells`, disegnato nel `Tick`, spento per default e dichiarato «non la presentazione
	 * definitiva»; questo accende o spegne la **geometria istanziata** che il giocatore vede sempre.
	 * `rt.Debug.DrawCells` resta, e resta uno strumento di sviluppo.
	 *
	 * 🔴 **Non ricostruisce niente, e il DoD lo pretende con un test invece che con un'ispezione**: commuta la
	 * VISIBILITA' del componente, che e' `O(1)`. Un `RebuildInstances` qui rifarebbe l'intera board a ogni
	 * pressione — su arena piena sono 7 651 celle — e soprattutto **azzererebbe lo stato del velo**
	 * (`LastVeilState` e sorelle si resettano con gli indici), quindi il primo velo successivo ridipingerebbe
	 * tutto. Un toggle di presentazione che invalida il velo non e' sola presentazione.
	 *
	 * ⛔ **Non muta `FRTMapState`, graph revision, path cache, snapshot, TurnLog o stato di rete.** Il
	 * componente e' una vista DERIVATA: spegnerlo non toglie una cella dal grafo, esattamente come nasconderla
	 * col velo non la rende intraversabile.
	 */
	void SetCellBordersVisible(bool bVisible);
	bool AreCellBordersVisible() const { return bCellBordersVisible; }

protected:
	virtual void Tick(float DeltaSeconds) override;

	/** Disegna l'overlay di leggibilita' (debug-line): superficie, blocco del movimento, blocco della vista. */
	void DrawCellOverlay() const;

	/** Overlay di leggibilita' attivo (acceso da console, spento per default). */
	bool bCellOverlay = false;

	/**
	 * Griglia attiva. 🔑 **`true`, e il default e' un requisito del DoD di #1758, non una preferenza**: il
	 * confine fra celle e' cio' con cui si conta il movimento, quindi la board lo mostra senza che nessuno
	 * lo chieda. E' il contrario esatto di `bCellOverlay`, che nasce spento perche' e' un debug.
	 */
	bool bCellBordersVisible = true;

#if !UE_BUILD_SHIPPING
	/** Debug dei volumi di conoscenza attivo (acceso da `rt.Debug.Knowledge`, spento per default). */
	bool bKnowledgeDebug = false;
#endif

	/** Disegna evidenziazione e traccia (debug-line): nessun effetto sulla logica. */
	void DrawPlanningPreview() const;

	/** Cella sotto il cursore, impostata dal controller. */
	FRTCellId HoveredCell;

	/** Falso = niente evidenziazione (cursore fuori dalla mappa). */
	bool bHoveredValid = false;

	/** Traccia del percorso pianificato, impostata dal controller. */
	TArray<FRTCellId> PreviewPath;

	/** Celle colpite dall'attacco pianificato, impostate dal controller (sola presentazione). */
	TArray<FRTCellId> PreviewHitCells;
	/** Sottoinsieme di `PreviewHitCells` occupato da alleati: fuoco amico, disegnato in arancione. */
	TArray<FRTCellId> PreviewAllyHitCells;
	/** Celle raggiungibili dall'unita' selezionata, impostate dal controller (sola presentazione). */
	TArray<FRTCellId> PreviewReachable;

	/** Cella da cui parte l'attacco pianificato — post-scatto quando lo scatto si applica. */
	FRTCellId PreviewAttackOrigin;
	/** Cella verso cui punta la mira (bersaglio dichiarato o cella mirata). */
	FRTCellId PreviewAttackAim;
	/** Falso = nessun attacco pianificato: origine e linea di mira restano spente. */
	bool bPreviewAttackValid = false;
	/** Vero = l'origine viene dallo scatto pianificato: prevista, non confermata (linea tratteggiata). */
	bool bPreviewOriginPredicted = false;

	/** Vero se qualcosa va disegnato: evita di tenere il Tick acceso su un actor che non mostra nulla. */
	bool HasAnythingToDraw() const;
	/**
	 * Ricostruisce la vista a ogni costruzione dell'actor: apertura del livello, spostamento, undo, spawn.
	 * Le istanze ISM sono una vista DERIVATA dall'asset (autorevole): senza questo, riaprendo il livello la
	 * griglia non viene ridisegnata e InstanceCells resta vuoto (il click non saprebbe piu' a quale cella
	 * corrisponde un'istanza).
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	/** Aggiorna la vista quando si cambia asset, layer, dimensioni o mesh dal pannello Details. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Disiscrive dall'asset prima della distruzione (nessun delegate pendente su un actor morto). */
	virtual void BeginDestroy() override;

	/**
	 * Si iscrive alle notifiche dell'asset mostrato (undo/redo): l'actor non fa parte di quelle transazioni,
	 * quindi senza notifica continuerebbe a mostrare lo stato precedente finche' qualcos'altro non lo forza.
	 * Idempotente: rifa' il bind solo se MapAsset e' cambiato.
	 */
	void BindToMapAsset();
	void UnbindFromMapAsset();

	/** Asset a cui siamo attualmente iscritti (puo' cambiare quando si riassegna MapAsset). */
	TWeakObjectPtr<URTHexMapAsset> BoundAsset;
	FDelegateHandle MapChangedHandle;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> Cells;

	/**
	 * Rilievo che mostra il COSTO di attraversamento: un blocco alto quanto il sovrapprezzo della cella.
	 *
	 * **Collisione disabilitata, e non e' un dettaglio.** Il raycast di selezione valida il COMPONENTE
	 * (`IsPickOnSelectableCell`, dal 2026-08-12 — prima confrontava l'actor, e un colpo qui produceva una
	 * cella valida e SBAGLIATA: era il difetto di `#588`). Oggi un colpo su questa geometria non sbaglia
	 * cella, viene **scartato**: la risoluzione ripiega sul piano del layer e si perde la precisione del
	 * colpo sulla mesh delle celle. Non e' piu' «dipinge dove non ho cliccato», ma resta una degradazione
	 * silenziosa, e per una geometria di sola lettura non c'e' niente da guadagnarci.
	 *
	 * E' la stessa ragione per cui i piani di contesto di `Focus` non diventano istanze.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> Relief;

	/**
	 * Volumi delle due regole di blocco: dove non si passa, e dove non si vede attraverso.
	 *
	 * **Un solo componente per due forme**, e non e' un compromesso: un ISM porta una sola `StaticMesh`, ma le
	 * ISTANZE hanno scale indipendenti — e qui la forma *e'* la scala. Un cilindro stretto e alto legge come
	 * colonna, uno largo e basso come lastra. Due componenti avrebbero dato due mesh diverse al prezzo di due
	 * cicli di vita da tenere allineati, per una differenza che la scala gia' produce.
	 *
	 * Le due forme sono **concentriche e annidate** con i canali gia' presenti — contorno di superficie 0.85,
	 * lastra della vista 0.75, rilievo del costo 0.60, colonna del blocco 0.40 — cosi' una cella che dice tre
	 * cose le mostra tutte e tre invece di sovrapporle.
	 *
	 * `NoCollision` per la stessa ragione di `Relief`: il raycast valida il componente, quindi un colpo qui
	 * verrebbe scartato e la risoluzione ripiegherebbe sul piano — nessun errore, ma precisione persa.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> Blockers;

	/**
	 * Pannelli su BORDO: coperture e porte. Sono l'unica cosa della mappa che non appartiene a una cella ma a
	 * un **lato**, e l'overlay a cerchi centrati non poteva dirle — da che lato si e' riparati e' cio' che
	 * decide se una posizione e' buona.
	 *
	 * Ha una **mesh propria** (cubo) e non condivide quella dei blocchi, ed e' l'unico caso in cui un secondo
	 * componente e' giustificato: un pannello non e' un cilindro schiacciato, e nessuna scala trasforma l'uno
	 * nell'altro. Dove la scala bastava — colonna contro lastra — `Blockers` resta un componente solo.
	 *
	 * `NoCollision` come gli altri: il raycast valida il componente, e un colpo qui verrebbe scartato.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> EdgeFeatures;

	/**
	 * Il GLIFO di superficie (`#956`, `D-183`): il secondo canale della board, colore E forma.
	 *
	 * **Quattro componenti e non uno**, perche' un ISM porta una sola mesh e i quattro segni sono quattro
	 * mesh — una per conteggio di anelli. L'indice e' `RingCount - 1`.
	 *
	 * Portano custom data come `Cells`, e sono gli unici due a farlo: il colore del glifo e' una costante
	 * scura, mentre rilievo, blocchi e bordi restano col materiale di default per la ragione gia' scritta
	 * sopra — tingerli direbbe una cosa falsa.
	 *
	 * `NoCollision` e `CastShadow = false` come gli altri tre: sono strumenti di lettura, non scenografia.
	 */
	// ⚠️ Niente `BlueprintReadOnly`: UHT rifiuta un array statico esposto a Blueprint. Resta `VisibleAnywhere`,
	// che e' cio' che serve — questi componenti si guardano nel dettaglio dell'attore, non si leggono da BP.
	UPROPERTY(VisibleAnywhere, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> SurfaceGlyphs[4];

	/**
	 * 🔑 **La GRIGLIA: il confine fra due celle, in partita e senza comando console** (#1758).
	 *
	 * 🔴 **Il difetto che chiude non e' «manca un colore».** Il colore di superficie in partita esisteva gia'
	 * — `Cells` lo porta per istanza dal 2026-08-23 — ma due celle adiacenti della STESSA superficie restano
	 * due prismi dello stesso colore appoggiati l'uno all'altro: il colore non dice **dove finisce una**. Su
	 * un gioco in cui il costo si conta in celle, chi non vede il confine non puo' contare il movimento.
	 * Superficie e confine sono due canali diversi, e la board ne aveva uno solo.
	 *
	 * ⚠️ **Porta custom data come `Cells` e `SurfaceGlyphs`, e per la stessa ragione**: senza, il velo
	 * potrebbe solo NASCONDERE il bordo, e ricordo e osservazione diventerebbero indistinguibili sulla
	 * griglia — lo stesso buco che `Relief`, `Blockers` ed `EdgeFeatures` hanno per costruzione.
	 *
	 * ⚠️ **Il colore e' la costante scura del glifo, non la tavolozza delle superfici** ([D-183]): il bordo
	 * appartiene al registro «segno inciso», e tingerlo come il terreno raddoppierebbe il canale che esiste
	 * gia' invece di aggiungerne uno.
	 *
	 * `NoCollision` e `CastShadow = false` come le altre famiglie di lettura: il raycast di selezione valida
	 * il COMPONENTE (`IsPickOnSelectableCell`), quindi un colpo qui verrebbe scartato — e un bordo che
	 * proiettasse ombra disegnerebbe una seconda griglia sfalsata sulla board.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> CellBorders;

	/**
	 * 🔴 **I volumi di conoscenza: uno strumento di ISPEZIONE, e l'unico componente della board che disegna
	 * cio' che la squadra NON sa.**
	 *
	 * Rende leggibili in ALTEZZA i tre stati che `ApplyKnowledgeVeil` rende in colore e presenza —
	 * `3/3` osservata, `2/3` ricordata, `1/3` mai vista. E' il canale FORMA che [D-146] chiede accanto al
	 * colore, applicato alla conoscenza invece che alla superficie.
	 *
	 * ⛔ **Perche' non contraddice [D-225], che dichiara «mai vista: non si disegna».** Quella decisione ha
	 * per attore il GIOCATORE, e vieta la «mappa nera» a schermo in partita. Qui l'attore e' chi sviluppa, e
	 * vedere cio' che il giocatore non vede e' il mestiere di un debug.
	 *
	 * ⚠️ **Il confine passa dall'API, non dal componente, e la ragione e' un vincolo di UHT**: una
	 * `UPROPERTY` non puo' stare dentro `#if !UE_BUILD_SHIPPING` — *«must not be inside preprocessor blocks,
	 * except for WITH_EDITORONLY_DATA»* — e `WITH_EDITORONLY_DATA` sarebbe sbagliato, perche' questo
	 * strumento deve funzionare anche in una build Development cooked, dove vale `0`. Quindi il componente
	 * esiste sempre, **vuoto e invisibile**, mentre `SetKnowledgeDebugEnabled` — l'unica cosa che lo popola —
	 * non esiste in Shipping. Un ISM senza mesh e senza istanze non disegna niente e non costa niente: cio'
	 * che va tolto e' il modo di riempirlo.
	 *
	 * ⚠️ **E il confinamento lo verifica la BUILD, non un test**, perche' nessun test puo' misurarlo: una
	 * suite gira in Development, dove il simbolo c'e' per definizione, e un test che ne constatasse la
	 * presenza non direbbe nulla su Shipping. L'oracolo e' il gate `G1` del DoD v0.1 — compilare il target
	 * Shipping, dove anche `WITH_DEV_AUTOMATION_TESTS` vale `0`.
	 *
	 * ✅ **Fatto il 2026-08-29, e con l'oracolo giusto**: `Result: Succeeded`, e poi le stringhe cercate nei
	 * due binari — `rt.Debug.Knowledge` e `RT_KnowledgeVolume` **assenti** dallo Shipping, presenti nel DLL
	 * di Development. Una build che passa proverebbe solo che il codice compila; e' l'assenza del simbolo a
	 * provare che non e' stato spedito. Il dettaglio, con il suo controllo di sanita', sta in
	 * `Map/RTKnowledgeDebugConsole.cpp`.
	 *
	 * ⚠️ **NON partecipa a `RebuildInstances`, ed e' una scelta contro un difetto noto.** Le altre cinque
	 * famiglie hanno array paralleli (`InstanceCells`, `…BaseScale`, `Last…VeilState`) che un
	 * `RebuildInstances` di mezzo lascia stantii — l'header di `ApplyKnowledgeVeil` lo dichiara: *«celle
	 * velate SBAGLIATE, un difetto che si legge come "problema grafico" per settimane»*. Questo componente si
	 * **ricostruisce da zero** a ogni chiamata, quindi non ha indici da tenere allineati e non puo' entrare
	 * in quello stato. Costa piu' CPU di un aggiornamento incrementale: e' debug, e la correttezza vale piu'
	 * della frequenza.
	 *
	 * ⚠️ **Il buco che colma, e che oggi non ha altro rimedio.** Su `Relief`, `Blockers` ed `EdgeFeatures` il
	 * velo *«NASCONDE e basta, non attenua»* perche' non portano custom data: ricordo e osservazione sono
	 * **indistinguibili**. L'altezza non passa dal materiale, quindi li distingue dove il colore non arriva.
	 */
	UPROPERTY(VisibleAnywhere, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> KnowledgeVolumes;

#if WITH_EDITORONLY_DATA
	/**
	 * Le coordinate incise sul pavimento (#1920). **Solo editor**, e non e' un ottavo ISM.
	 *
	 * 🔑 **Perche' un `ULineBatchComponent` e non un disegno per frame.** La GEOMETRIA (i punti di ogni
	 * stroke) si calcola una volta, quando la mappa cambia, invece che a ogni frame come farebbe un
	 * `DrawDebugLine` posato da `Tick` — il ricalcolo per evento che #711 ha gia' pagato, un costo che
	 * nessun test misura e che si scopre solo quando il viewport smette di seguire il mouse.
	 *
	 * ⚠️ **Non e' pero' un vantaggio assoluto sul costo PER FRAME.** `ULineBatchComponent::TickComponent`
	 * scorre comunque tutte le linee registrate a ogni frame, e la sua scene proxy le riemette al PDI per
	 * ogni vista: disegnare `N` linee resta `O(N)` a ogni frame, qui come con `DrawDebugLine`. Cio' che si
	 * evita e' il costo di RICOSTRUIRE quell'`N` — l'iterazione su celle/glifi/stroke di
	 * `RebuildCoordinateLabels` — a ogni frame invece che a ogni cambiamento della mappa.
	 *
	 * ⚠️ Le sette famiglie di ISM sono canali di lettura DELLA PARTITA. Questo no: in una build di gioco
	 * il componente non esiste, e si verifica per assenza.
	 *
	 * ⚠️ **`WITH_EDITORONLY_DATA` e non `WITH_EDITOR`, per la `UPROPERTY`**: UHT rifiuta una `UPROPERTY`
	 * dentro `WITH_EDITOR` — *«UProperties should not be wrapped by WITH_EDITOR, use WITH_EDITORONLY_DATA
	 * instead»* — mentre la funzione sotto, che non e' una `UPROPERTY`, resta sotto `WITH_EDITOR` come nel
	 * resto del file. Le due macro coincidono per i target di questo progetto (Editor vs Game), quindi il
	 * confine «solo editor» e' lo stesso.
	 */
	UPROPERTY(VisibleAnywhere, Category = "RefactorTactics|HexMap")
	TObjectPtr<class ULineBatchComponent> CoordinateLabels;
#endif

#if WITH_EDITOR
	/** Ridisegna le terne di tutte le celle. Nessuna decisione: consuma `BuildCellLabel`. */
	void RebuildCoordinateLabels();
#endif

	/**
	 * Mapping instance index -> FRTCellId (per selezione/debug). Stato DERIVATO, non serializzato:
	 * viene rigenerato da RebuildInstances a ogni costruzione dell'actor.
	 */
	TArray<FRTCellId> InstanceCells;

	/**
	 * La scala PIENA di ogni istanza di `Cells`, per indice. Stato DERIVATO come `InstanceCells`.
	 *
	 * ⚠️ Serve perche' il velo nasconde portando la scala a zero, e da una scala **gia'** a zero non si
	 * risale a quella piena: senza questo array il primo `ApplyKnowledgeVeil` sarebbe irreversibile, e la
	 * cella tornata visibile resterebbe invisibile per sempre.
	 */
	TArray<FVector> InstanceBaseScale;

	/**
	 * Mapping instance index -> `FRTCellId` per i QUATTRO componenti dei glifi, e le loro scale piene.
	 * Stato DERIVATO, rigenerato da `RebuildInstances`.
	 *
	 * ⚠️ Esistono perche' i glifi hanno un'indicizzazione PROPRIA: il glifo `N` non e' la cella `N`, dato che
	 * cinque superfici su nove non ne ricevono nessuno (`SurfaceRingCount` restituisce zero). Velare la corona
	 * riusando gli indici di `InstanceCells` colpirebbe la cella sbagliata — e il difetto sarebbe silenzioso,
	 * perche' il conteggio delle istanze velate tornerebbe giusto.
	 */
	TArray<FRTCellId> GlyphCells[4];
	TArray<FVector> GlyphBaseScale[4];

	/**
	 * Gli stessi array per la griglia. **Un'istanza per cella, uno-a-uno con `InstanceCells`** — a differenza
	 * di `Relief`/`Blockers`/`EdgeFeatures`, che saltano celle o ne aggiungono due.
	 *
	 * ⚠️ **Restano array propri e non si riusa `InstanceCells`**, nonostante l'uno-a-uno: il giorno in cui il
	 * bordo saltasse una cella — un layer filtrato, una superficie senza griglia — gli indici divergerebbero
	 * e il velo colpirebbe la cella sbagliata **senza che nessun conteggio se ne accorga**. E' precisamente
	 * il difetto che l'header di `GlyphCells` documenta per i glifi.
	 */
	TArray<FRTCellId> BorderCells;
	TArray<FVector> BorderBaseScale;
	TArray<uint8> LastBorderVeilState;

	/**
	 * Gli stessi due array per le TRE famiglie di geometria che non sono ne' il disco ne' il glifo: il rilievo
	 * del costo, i volumi di blocco vista/movimento e i pannelli di bordo (coperture e porte).
	 *
	 * ⚠️ La loro indicizzazione non e' quella delle celle **e non e' nemmeno uno-a-uno**: `Relief` salta il
	 * pavimento, `Blockers` puo' aggiungere DUE istanze sulla stessa cella — lastra e colonna insieme — ed
	 * `EdgeFeatures` una per copertura e una per porta. Ecco perche' la cella si registra per ISTANZA e non
	 * si ricava: senza, il velo colpirebbe la geometria di un'altra cella.
	 */
	TArray<FRTCellId> ReliefCells;
	TArray<FVector> ReliefBaseScale;
	TArray<uint8> LastReliefVeilState;
	TArray<FRTCellId> BlockerCells;
	TArray<FVector> BlockerBaseScale;
	TArray<uint8> LastBlockerVeilState;
	TArray<FRTCellId> EdgeFeatureCells;
	TArray<FVector> EdgeFeatureBaseScale;
	TArray<uint8> LastEdgeFeatureVeilState;

	/**
	 * Lo stato che il velo ha SCRITTO per ultimo su ogni istanza: `0` nascosta, `1` ricordata, `2` accesa —
	 * `0xFF` quando non e' mai stato scritto. Stato DERIVATO, azzerato da `RebuildInstances` insieme agli
	 * indici a cui si riferisce.
	 *
	 * ⚠️ **Non e' un'ottimizzazione preventiva: e' la risposta a una misura.** Riscrivere tutte le istanze a
	 * ogni velo costa **~2,2 s** su arena piena (7 651 celle; 2 624 ms alla prima misura, 2 160 ms sulla suite
	 * del 2026-08-28 — la macchina sposta il numero, non l'ordine di grandezza), contro un budget di
	 * due refresh per turno. Fra due refresh consecutivi pero' cambia solo il bordo del cono, quindi il
	 * lavoro utile e' una frazione minima del totale: si tocca cio' che cambia, e il resto si salta.
	 */
	TArray<uint8> LastVeilState;
	TArray<uint8> LastGlyphVeilState[4];

	/** Quante istanze l'ultimo velo ha toccato. Diagnostica: vedi `GetLastVeilTouchedCells`. */
	int32 LastVeilTouchedCells = 0;

	/**
	 * Il velo su UNA famiglia di istanze. Esiste perche' le famiglie sono cinque e la regola e' una sola:
	 * scritta cinque volte, la sesta famiglia nascerebbe scoperta e nessun conteggio se ne accorgerebbe — che
	 * e' esattamente come `Relief`, `Blockers` ed `EdgeFeatures` erano rimasti fuori dalla prima stesura.
	 *
	 * `BaseColor` riempie il colore PIENO della cella e risponde `false` se quella famiglia non ha un canale
	 * colore per istanza: in quel caso il velo agisce sulla sola scala, e ricordato e osservato restano
	 * indistinguibili su quella geometria.
	 *
	 * @return quante istanze sono state davvero toccate.
	 */
	int32 VeilInstances(UInstancedStaticMeshComponent* Component, const TArray<FRTCellId>& CellsOfInstance,
		const TArray<FVector>& BaseScale, TArray<uint8>& LastState,
		const TSet<FRTCellId>& Visible, const TSet<FRTCellId>& Explored,
		TFunctionRef<bool(const FRTCellId&, FLinearColor&)> BaseColor);

	/** Stato DERIVATO, non serializzato: cache pigra, invalidata da `RebuildInstances`. */
	mutable TArray<FRTCellId> UnreachableCells;
	mutable bool bUnreachableDirty = true;
};
