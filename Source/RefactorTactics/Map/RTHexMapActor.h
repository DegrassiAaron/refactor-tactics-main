#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/RTCellId.h"
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
	 * `CoverYard`.
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

protected:
	virtual void Tick(float DeltaSeconds) override;

	/** Disegna l'overlay di leggibilita' (debug-line): superficie, blocco del movimento, blocco della vista. */
	void DrawCellOverlay() const;

	/** Overlay di leggibilita' attivo (acceso da console, spento per default). */
	bool bCellOverlay = false;

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
	 * Mapping instance index -> FRTCellId (per selezione/debug). Stato DERIVATO, non serializzato:
	 * viene rigenerato da RebuildInstances a ogni costruzione dell'actor.
	 */
	TArray<FRTCellId> InstanceCells;

	/** Stato DERIVATO, non serializzato: cache pigra, invalidata da `RebuildInstances`. */
	mutable TArray<FRTCellId> UnreachableCells;
	mutable bool bUnreachableDirty = true;
};
