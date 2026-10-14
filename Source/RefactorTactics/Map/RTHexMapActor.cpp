#include "Map/RTHexMapActor.h"
#include "HAL/IConsoleManager.h"
extern TAutoConsoleVariable<float> CVarRTMapStrategicLayerSeparation; // #3632: la leva della separazione in PIE
#include "Map/RTStructuralBodyLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTPlaybackLibrary.h" // #2454: `TracerSegment`, la geometria del tracer — pura, non la riscrive il disegno
#include "Map/RTArenaCriteriaLibrary.h"
#include "Map/RTGeometryGrammar.h" // ToPolyline: i muri interni si disegnano dal loro segmento (#712)
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h" // #3706: il colore proprio dei pannelli di bordo
#include "HAL/IConsoleManager.h" // #2761: la CVar che cambia la semantica degli indici di RemoveInstance
#include "DrawDebugHelpers.h" // il solo contorno di DEBUG delle celle (`DrawCellOverlay`): l'anteprima no (#3508)
#include "Components/LineBatchComponent.h" // #3508: l'anteprima di pianificazione si disegna anche in Shipping
#include "EngineUtils.h" // TActorIterator
#include "UObject/ConstructorHelpers.h"
#include "RefactorTactics.h"
// Prisma esagonale generato (`GetCellPrismMesh`).
// 🔴 `MeshDescription` e `StaticMeshDescription` sono dipendenze DIRETTE in `RefactorTactics.Build.cs`, e la
// riga qui prima diceva il contrario: «sono in `PublicDependencyModuleNames` di `Engine.Build.cs`, quindi
// arrivano per transitivita'». La premessa e' vera — ci sono davvero, righe 105-106 — e la conclusione e'
// falsa: quell'elenco propaga gli **include path**, non risolve i simboli di un modulo a valle. La build lo
// ha detto in nove `LNK2019`. Verificare l'elenco sbagliato assomiglia molto a verificare.
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/StrongObjectPtr.h"
#include "Map/RTMapVisuals.h" // #983: le misure del disco stanno scritte una volta sola
#include "Map/RTOverlayPalette.h" // #1941: colore, scala e profondita' di un significato, in una sede sola
#include "Perception/RTVisibilityBorder.h" // #3761: i lati esposti, da cui la linea di confine della vista
#if WITH_EDITOR
#include "ScopedTransaction.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"
// Map Check (`CheckForErrors`): le regole di allestimento del livello. Solo Editor — `CheckForErrors`
// stesso e' dichiarato dentro `WITH_EDITOR` in `Actor.h`. Il Message Log lo scrive
// `URTMapTemplateLibrary::EmitIssueToMapCheck`, che e' l'unico posto che ne conosce la forma.
#include "Map/RTMapTemplateLibrary.h"
#endif

#define LOCTEXT_NAMESPACE "RTHexMap"

/**
 * Il nome che lega la sezione della mesh al suo slot materiale, e va scritto UNA VOLTA SOLA.
 *
 * 🔴 **`BuildFromMeshDescriptions` accoppia sezione e slot PER NOME**, confrontando il
 * `PolygonGroupMaterialSlotName` del `MeshDescription` col `MaterialSlotName` di `FStaticMaterial`. Se non
 * combaciano, la sezione esce con `MaterialIndex = -1`: geometria completa, nessun materiale da usare, e
 * **niente a schermo**. Misurato il 2026-08-30 nel pacchetto (#1665) — `tri=20 idx=60 matIdx=-1/1` sul
 * prisma, contro `matIdx=0/1` del Cube d'engine nella stessa run, che si vedeva.
 *
 * ⚠️ Le due estremita' del legame stanno **200 righe lontane** l'una dall'altra in tre coppie diverse, ed e'
 * il motivo per cui il disallineamento e' sopravvissuto: qui c'e' una costante, cosi' cambiarne una senza
 * l'altra non e' piu' possibile — ed e' l'unica difesa che il legame ha.
 *
 * ⌫ **Qui si leggeva *«Presidiato da
 * `RefactorTactics.HexMapActor.ProceduralMeshSectionsHaveAMaterialSlot`»*, e quel test non esiste:
 * tolto il 2026-10-03.** `git grep -c <nome> -- Source/` rispondeva **1**, il commento stesso.
 * 🔴 **E non c'e' un sostituto da citare al suo posto**, misurato: nessun test nomina
 * `RTProceduralMeshSlotName`, che in produzione compare in **otto** punti di questo file.
 * `HexMapActor.ProceduralMeshesAreRenderable` verifica una cosa VICINA e non questa — asserisce
 * *«ha render data con vertici»* — e ripuntare la riga la' avrebbe rimesso un presidio falso
 * con un nome che esiste, che e' peggio di uno con un nome che non esiste.
 * ⚠️ Il legame resta difeso dalla sola **costante condivisa** qui sotto: chi la tocca rompe
 * entrambe le estremita' insieme, ed e' per questo che esiste. Un gate automatico non c'e'.
 */
static const FName RTProceduralMeshSlotName(TEXT("Default"));

/** Il colore delle linee della pianta (#3760), scelto dall'autore sulla tavola P2: il ciano chiaro. */
static const FColor RTPlanLineColor(110, 205, 230);

namespace
{
	/**
	 * Geometria del disco che rappresenta una cella. Il prisma di `GetCellPrismMesh` ha mezza-altezza
	 * `RTCellPrismRadius` ed e' CENTRATO sull'origine — le stesse convenzioni del cilindro engine che ha
	 * sostituito, deliberatamente: con `RTCellFlatScale` la sua faccia superiore sta a `RTCellTopZ` sopra il
	 * centro della cella.
	 *
	 * 🔴 **Le tre costanti sono uscite da questo namespace anonimo con #983**, e ora vivono in
	 * `Map/RTMapVisuals.h`: qui non le vedeva nessun altro modulo, quindi chi doveva posarci sopra qualcosa
	 * ricopiava il numero. Il commento che spiegava perche' sono condivise — le linee di debug disegnate
	 * SOTTO `RTCellTopZ` finiscono dentro il disco e diventano invisibili, ed **e' successo davvero** — e'
	 * andato con loro, che e' il posto in cui serve a chi le include.
	 *
	 * Legare i lift a `RTCellTopZ` continua a fare si' che cambiare lo spessore del disco non riapra il
	 * difetto: le tre quote qui sotto non cambiano di una unita'.
	 */

	/**
	 * Il glifo di superficie (`#956`, `D-183`), in FRAZIONI DEL RAGGIO e mai in uu: con `#1155`
	 * (`HexSize` -> 150) le proporzioni si conservano, mentre scritte assolute il segno passerebbe dal 5,3%
	 * al 3,5% del raggio — il difetto che `D-163` registra per le altezze.
	 *
	 * `0,95` e' il bordo del disco, ricavato dal CODICE e non dal commento della scala annidata: quello
	 * dichiara 0,85 per il contorno di superficie, che a runtime sta invece a 0,90 (`DrawRing`).
	 */
	constexpr float RTGlyphOuterScale = 0.95f;
	constexpr float RTGlyphThickness = 0.0526f;
	constexpr float RTGlyphGap = 0.0421f;
	constexpr int32 RTGlyphMaxRings = 4;
	constexpr float RTCellPrismHalfHeight = 50.f;

	/**
	 * Geometria dei volumi che mostrano le regole, in frazione del raggio della cella.
	 *
	 * Sono ANNIDATI e in ordine: contorno di superficie 0.85 (linea) > lastra della vista 0.75 > rilievo del
	 * costo 0.60 > colonna del blocco 0.40. Una cella che dice tre cose insieme le mostra tutte e tre, invece
	 * di nasconderne due sotto la terza — ed e' il criterio che tiene, non l'estetica.
	 */
	constexpr float RTSightSlabScale = 0.75f;
	constexpr float RTBlockColumnScale = 0.40f;

	/**
	 * Altezze (uu). La lastra della vista e' BASSA di proposito: comunica «ci si passa sopra», che e' la cosa
	 * piu' fraintesa della mappa. La colonna del blocco e' alta abbastanza da leggersi dall'alto, che e' la
	 * vista di lavoro, e resta due ordini di grandezza sotto `LayerHeight` (250) per non confondersi con un
	 * piano — stesso vincolo del rilievo di costo.
	 *
	 * ⚠️ **La lastra deve restare sotto `URTHexLibrary::ReliefUnitHeight` (15 uu)**, e non e' una
	 * preferenza estetica. Lastra e rilievo sono CONCENTRICI e partono dalla stessa quota, e la lastra e'
	 * la piu' larga: se la supera anche in altezza, la inghiotte. Il costo massimo del catalogo v0.1 e'
	 * `2`, quindi il rilievo piu' alto che una mappa possa produrre e' esattamente `ReliefUnitHeight` —
	 * il che rende il caso peggiore l'unico caso, e ogni cella costosa che blocca la vista smetterebbe di
	 * dire quanto costa. A 16 uu succedeva; `HexMapActor.CostReliefSurvivesTheSightSlab` lo misura.
	 */
	constexpr float RTSightSlabHeight = 10.f;
	constexpr float RTBlockColumnHeight = 55.f;

	/**
	 * Pannelli di bordo (cubo engine: 100 uu per lato, centrato). Lo spessore e' sottile perche' un bordo non
	 * ha profondita': quello che deve comunicare e' DOVE sta e QUANTO e' alto.
	 *
	 * Le altezze seguono la semantica, non l'estetica:
	 * - copertura BASSA ripara e lascia passare tutto -> un muretto che si scavalca con lo sguardo;
	 * - copertura ALTA nega vista, passo e proiettili -> alta quanto la colonna di blocco, perche' fa la
	 *   stessa cosa su un lato invece che su una cella;
	 * - porta CHIUSA nega passo e vista -> piena, ed e' la piu' alta: e' una barriera, non un riparo;
	 * - porta APERTA lascia passare -> una soglia bassa, che dice «qui c'e' una porta» senza dire «e' chiusa».
	 *   Non disegnarla affatto nasconderebbe l'informazione piu' utile: che quel passaggio puo' chiudersi.
	 */
	constexpr float RTEdgePanelThickness = 0.10f;
	constexpr float RTEdgePanelWidth = 0.92f;
	constexpr float RTCoverLowHeight = 22.f;
	constexpr float RTCoverHighHeight = 55.f;
	constexpr float RTDoorClosedHeight = 70.f;
	constexpr float RTDoorOpenHeight = 5.f;

	/** Quote di disegno, tutte sopra la faccia del disco e in ordine di priorita' di lettura. */
	constexpr float RTLiftSurface = RTCellTopZ + 0.5f;  // contorno della superficie (contesto)
	// Il passo con cui due ghost sulla STESSA cella si separano in quota. Vedi `SetPlanPreview`.
	constexpr float RTGhostStackStep = 6.f;
	constexpr float RTLiftGlyph = RTCellTopZ + 0.3f;    // glifo di superficie (#956): inciso nella faccia,
	                                                    // sotto il contorno, sopra il disco
	// Le coordinate incise (#1920): sopra superficie/griglia/glifo (leggibili), sotto marker e anteprima —
	// un ausilio d'autoraggio non deve coprire un canale di lettura della partita. Vive qui e non come
	// `constexpr` locale in `RebuildCoordinateLabels`, oltre mille righe piu' in basso, per lo stesso motivo per
	// cui le altre quattro quote vivono qui: un numero ricopiato lontano dagli altri e' quello che nessun
	// compilatore rilega quando la gerarchia cambia (vedi `RTMapVisuals.h`).
	constexpr float RTLiftCoordinateLabel = RTCellTopZ + 1.0f;
	constexpr float RTLiftMarker  = RTCellTopZ + 1.5f;  // blocca-movimento / blocca-vista
	// La linea di confine della vista (#3761): sopra i marcatori, SOTTO l'anteprima. Un contorno d'anteprima su una
	// cella di bordo corre lungo lo stesso lato, e alla stessa quota i due colori si contenderebbero il pixel.
	constexpr float RTLiftVisionBorder = RTCellTopZ + 2.0f;
	// Lo spessore della linea di confine (#3761): quello con cui l'autore l'ha vista sulle tavole. Piu' dei 3 dei
	// contorni d'anteprima, perche' la linea deve leggersi anche sotto un'anteprima accesa.
	constexpr float RTVisionBorderThickness = 4.f;
	// Lo spessore del contorno dell'arco di un nemico (#3762): quello con cui l'autore l'ha visto sulle tavole, un
	// punto sotto la linea di confine, perche' dove si sovrappongono si legga ancora la linea della propria vista.
	constexpr float RTEnemyArcThickness = 3.f;
	constexpr float RTLiftPreview = RTCellTopZ + 2.5f;  // anteprima di pianificazione (sopra a tutto)

	// 🔴 **La ribbon di perimetro (#1942) NON entra in questa pila: la attraversa.** Il vincolo della issue
	// chiede che una parete alta sia *decisa* rispetto a queste quote e non aggiunta sopra per inerzia — ed e'
	// una decisione che il compilatore puo' tenere. `RTBoundaryRibbonHeight` vale un ordine di grandezza piu'
	// dell'intera pila: se qualcuno la abbassasse fin dentro il vicinato dei lift smetterebbe di leggersi come
	// parete e diventerebbe un sesto contorno, che e' precisamente il difetto che OVL-02 chiude.
	//
	// ⚠️ Il confronto e' con lo **spessore** della pila (`RTLiftPreview - RTCellTopZ`, cioe' `2.5`), non con
	// la sua quota assoluta: una prima stesura di questo assert confrontava `base + altezza` con
	// `RTLiftPreview * 4` ed e' uscita **falsa per zero** — `40.0 > 40.0`. Un fattore scelto a occhio su una
	// quota assoluta non dice nulla; su uno spessore dice quello che si intende.
	static_assert(RTBoundaryRibbonHeight > (RTLiftPreview - RTCellTopZ) * 4.f,
		"La ribbon di perimetro deve TORREGGIARE sulla pila dei lift, non infilarcisi dentro (#1942).");

	// Tracer del playback (`#2454`): altezza sopra la cella, lunghezza del dardo in frazioni di `HexSize`,
	// spessori. ⚠️ Valori di GRAYBOX, tarati in PIE (`PIE-V01-TRACER`): il getto e' piu' spesso del proiettile
	// perche' le due forme devono separarsi anche in un fotogramma fermo.
	constexpr float RTTracerHeight = 60.f;
	constexpr float RTTracerDashFraction = 0.35f;
	constexpr float RTTracerProjectileThickness = 4.f;
	constexpr float RTTracerJetThickness = 7.f;

	// Profilo FX (#3578, spec «il profilo FX» §2.1): spessori per tipo e sollevamento sopra la cella. ⚠️ Graybox,
	// tarati in PIE (`PIE-FX-ABILITA`); le scale vivono in `URTPlaybackLibrary::CueSegments`.
	constexpr float RTTracerZigzagThickness = 5.f;
	constexpr float RTCueLift = 6.f;
	constexpr float RTCueThickness = 3.f;
	constexpr float RTCueThickThickness = 4.f; // `Flash` e `ConeSweep`

	/**
	 * 🔴 **Il tetto vero dello spessore del tile, e NON e' lo `static_assert` degli anelli.**
	 *
	 * Il tile e' CENTRATO sul centro cella, quindi ispessirlo lo fa scendere anche sotto di meta' spessore.
	 * Due celle adiacenti che differiscono di UN gradino di rilievo distano `ReliefUnitHeight`: se il tile e'
	 * piu' spesso di quel gradino, la faccia inferiore della cella alta entra dentro la cella bassa e le due
	 * si compenetrano invece di gradinare. A schermo si legge come un errore di modellazione, non come una
	 * quota sbagliata, ed e' il motivo per cui vale la pena asserirlo invece di ricordarlo.
	 *
	 * ⚠️ **Sta QUI e non in `RTMapVisuals.h`**: quell'header e' deliberatamente senza dipendenze — «il punto
	 * in cui i numeri condivisi stanno scritti una volta sola», non un sistema di layering — e includerci
	 * `RTHexLibrary.h` per una sola costante lo trasformerebbe in altro. Questo file include gia' entrambi ed
	 * e' l'unico che posa sia il tile sia il rilievo, cioe' l'unico che puo' violare il vincolo.
	 *
	 * ⚠️ **`ReliefUnitHeight` e' il limite superiore, non un obiettivo**: l'uguaglianza e' ammessa perche' a
	 * spessore == gradino le due facce si toccano senza compenetrare.
	 */
	static_assert(RTCellThickness <= URTHexLibrary::ReliefUnitHeight,
		"Il tile della cella e' piu' spesso di un gradino di rilievo: due celle a quota adiacente si "
		"compenetrerebbero invece di gradinare (Map/RTMapVisuals.h, RTCellThicknessInH).");
}

UStaticMesh* ARTHexMapActor::GetCellGlyphMesh(int32 RingCount)
{
	// Mono-canale per SCELTA (criterio 1 di #956): cinque superfici su nove non ricevono un segno, e
	// `nullptr` lo dice meglio di una mesh vuota — chi la montasse pagherebbe un ISM per zero pixel.
	if (RingCount <= 0 || RingCount > RTGlyphMaxRings)
	{
		return nullptr;
	}

	// Una cache PER CONTEGGIO, con la disciplina di `GetCellPrismMesh`: `TStrongObjectPtr` tiene le mesh
	// fuori dalla portata del GC senza `AddToRoot` a mano.
	static TStrongObjectPtr<UStaticMesh> Cached[RTGlyphMaxRings];
	if (Cached[RingCount - 1].IsValid())
	{
		return Cached[RingCount - 1].Get();
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();

	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = RTProceduralMeshSlotName;

	// Gli anelli crescono VERSO L'INTERNO dal bordo del disco: l'anello `i` occupa
	// `[Outer - i*(Thickness+Gap) - Thickness, Outer - i*(Thickness+Gap)]`. Con le costanti di `D-183` i
	// raggi interni cadono a 0,90 / 0,80 / 0,71 / 0,61 — i numeri della sua tabella.
	for (int32 Ring = 0; Ring < RingCount; ++Ring)
	{
		const float OuterScale = RTGlyphOuterScale - Ring * (RTGlyphThickness + RTGlyphGap);
		const float InnerScale = OuterScale - RTGlyphThickness;

		// ⚠️ I vertici vengono da `HexCorners`, NON da un secondo `cos(60k-30)` scritto qui: e' il vincolo
		// che `#712` ha pagato a schermo, dove il bordo era un esagono e il pieno un cerchio.
		const TArray<FVector> OuterCorners = URTHexLibrary::HexCorners(FVector::ZeroVector, RTCellPrismRadius * OuterScale);
		const TArray<FVector> InnerCorners = URTHexLibrary::HexCorners(FVector::ZeroVector, RTCellPrismRadius * InnerScale);
		if (OuterCorners.Num() != 6 || InnerCorners.Num() != 6)
		{
			return nullptr;
		}

		TArray<FVertexID> OuterIds;
		TArray<FVertexID> InnerIds;
		for (int32 Corner = 0; Corner < 6; ++Corner)
		{
			const FVertexID O = Description.CreateVertex();
			Positions[O] = FVector3f(static_cast<float>(OuterCorners[Corner].X),
				static_cast<float>(OuterCorners[Corner].Y), 0.f);
			OuterIds.Add(O);

			const FVertexID I = Description.CreateVertex();
			Positions[I] = FVector3f(static_cast<float>(InnerCorners[Corner].X),
				static_cast<float>(InnerCorners[Corner].Y), 0.f);
			InnerIds.Add(I);
		}

		// La corona: sei quad fra i due esagoni. Piatta — il glifo e' INCISO nella faccia, non un volume:
		// a picco si legge come area, ed e' l'unico canale che la seduta U18 ha misurato leggibile dall'alto.
		for (int32 Edge = 0; Edge < 6; ++Edge)
		{
			const int32 Next = (Edge + 1) % 6;
			TArray<FVertexInstanceID> Instances;
			for (const FVertexID V : { InnerIds[Edge], InnerIds[Next], OuterIds[Next], OuterIds[Edge] })
			{
				Instances.Add(Description.CreateVertexInstance(V));
			}
			Description.CreatePolygon(Group, Instances);
		}
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(),
		*FString::Printf(TEXT("RT_CellGlyph_%d"), RingCount), RF_Transient);

	// Slot inizializzato, per la ragione scritta per esteso in `GetCellPrismMesh` (#1665): un
	// `FStaticMaterial()` nudo lascia `UVChannelData.bInitialized = false`, e fuori dall'Editor nessuno lo
	// ripara.
	FStaticMaterial GlyphSlot;
	GlyphSlot.MaterialSlotName = RTProceduralMeshSlotName;   // deve combaciare col PolygonGroup (#1665)
	GlyphSlot.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(GlyphSlot);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bBuildSimpleCollision = false;
	Params.bFastBuild = true;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);

	Cached[RingCount - 1].Reset(Mesh);
	return Mesh;
}

UStaticMesh* ARTHexMapActor::GetCellBorderMesh()
{
	// Il legame fra le due larghezze lo fa il COMPILATORE e non chi legge: `RTCellBorderThickness` vive in
	// `RTMapVisuals.h`, dove `RTGlyphThickness` non arriva, quindi lo `static_assert` di la' confronta un
	// letterale. Qui sono entrambe visibili, ed e' l'unico posto in cui l'asserzione dice la verita'.
	static_assert(RTCellBorderThickness < RTGlyphThickness,
		"Il bordo deve restare piu' sottile dell'anello del glifo: sopra di esso, un bordo piu' largo ne cancella il conteggio (D-183).");

	// Una sola per processo, con la disciplina di `GetCellPrismMesh`: `TStrongObjectPtr` tiene la mesh fuori
	// dalla portata del GC senza `AddToRoot` a mano.
	static TStrongObjectPtr<UStaticMesh> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();

	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = RTProceduralMeshSlotName;

	// ⚠️ I vertici vengono da `HexCorners`, NON da un secondo `cos(60k-30)` scritto qui: e' il vincolo che
	// `#712` ha pagato a schermo, dove il bordo era un esagono e il pieno un cerchio. Un confine che non
	// segue la stessa convenzione della cella che delimita e' peggio di nessun confine.
	const TArray<FVector> OuterCorners = URTHexLibrary::HexCorners(
		FVector::ZeroVector, RTCellPrismRadius * RTCellBorderOuterScale);
	const TArray<FVector> InnerCorners = URTHexLibrary::HexCorners(
		FVector::ZeroVector, RTCellPrismRadius * (RTCellBorderOuterScale - RTCellBorderThickness));
	if (OuterCorners.Num() != 6 || InnerCorners.Num() != 6)
	{
		return nullptr;
	}

	TArray<FVertexID> OuterIds;
	TArray<FVertexID> InnerIds;
	for (int32 Corner = 0; Corner < 6; ++Corner)
	{
		const FVertexID O = Description.CreateVertex();
		Positions[O] = FVector3f(static_cast<float>(OuterCorners[Corner].X),
			static_cast<float>(OuterCorners[Corner].Y), 0.f);
		OuterIds.Add(O);

		const FVertexID I = Description.CreateVertex();
		Positions[I] = FVector3f(static_cast<float>(InnerCorners[Corner].X),
			static_cast<float>(InnerCorners[Corner].Y), 0.f);
		InnerIds.Add(I);
	}

	// Sei quad fra i due esagoni. Piatta come il glifo: la griglia si legge a picco come contorno d'area, e
	// un bordo VOLUMETRICO proietterebbe un fianco che a camera tattica diventa una seconda linea sfalsata.
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const int32 Next = (Edge + 1) % 6;
		TArray<FVertexInstanceID> Instances;
		for (const FVertexID V : { InnerIds[Edge], InnerIds[Next], OuterIds[Next], OuterIds[Edge] })
		{
			Instances.Add(Description.CreateVertexInstance(V));
		}
		Description.CreatePolygon(Group, Instances);
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("RT_CellBorder"), RF_Transient);

	// Slot inizializzato, per la ragione scritta per esteso in `GetCellPrismMesh` (#1665): un
	// `FStaticMaterial()` nudo lascia `UVChannelData.bInitialized = false`, e fuori dall'Editor nessuno lo ripara.
	FStaticMaterial BorderSlot;
	BorderSlot.MaterialSlotName = RTProceduralMeshSlotName;   // deve combaciare col PolygonGroup (#1665)
	BorderSlot.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(BorderSlot);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bBuildSimpleCollision = false;
	Params.bFastBuild = true;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);

	Cached.Reset(Mesh);
	return Mesh;
}

UStaticMesh* ARTHexMapActor::GetCellPrismMesh()
{
	// Una sola mesh per l'intero processo: la costruzione tocca il disco zero volte e il risultato non dipende
	// da chi chiama. `TStrongObjectPtr` la tiene fuori dalla portata del GC senza `AddToRoot` a mano, che
	// nessuno ricorderebbe di bilanciare.
	static TStrongObjectPtr<UStaticMesh> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	// ⚠️ I vertici vengono da `HexCorners`, NON da un secondo `cos(60k-30)` scritto qui. E' l'intero punto
	// della correzione: il pieno e il contorno evidenziato devono nascere dalla stessa funzione, o torneranno
	// a divergere come nel difetto che questa mesh chiude.
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(FVector::ZeroVector, RTCellPrismRadius);
	if (Corners.Num() != 6)
	{
		return nullptr;
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();

	TArray<FVertexID> Top;
	TArray<FVertexID> Bottom;
	Top.Reserve(6);
	Bottom.Reserve(6);
	for (int32 Corner = 0; Corner < 6; ++Corner)
	{
		const FVertexID TopId = Description.CreateVertex();
		Positions[TopId] = FVector3f(
			static_cast<float>(Corners[Corner].X), static_cast<float>(Corners[Corner].Y), RTCellPrismHalfHeight);
		Top.Add(TopId);

		const FVertexID BottomId = Description.CreateVertex();
		Positions[BottomId] = FVector3f(
			static_cast<float>(Corners[Corner].X), static_cast<float>(Corners[Corner].Y), -RTCellPrismHalfHeight);
		Bottom.Add(BottomId);
	}

	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = RTProceduralMeshSlotName;

	auto AddFace = [&Description, Group](const TArray<FVertexID>& Ring)
	{
		TArray<FVertexInstanceID> Instances;
		Instances.Reserve(Ring.Num());
		for (const FVertexID Vertex : Ring)
		{
			Instances.Add(Description.CreateVertexInstance(Vertex));
		}
		Description.CreatePolygon(Group, Instances);
	};

	// Faccia superiore nell'ordine di `HexCorners`, inferiore rovesciata, e sei fianchi che chiudono il solido.
	AddFace(Top);

	TArray<FVertexID> BottomReversed = Bottom;
	Algo::Reverse(BottomReversed);
	AddFace(BottomReversed);

	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const int32 Next = (Edge + 1) % 6;
		AddFace(TArray<FVertexID>{ Bottom[Edge], Bottom[Next], Top[Next], Top[Edge] });
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("RT_CellHexPrism"), RF_Transient);

	// 🔴 **Lo slot si INIZIALIZZA, e un `FStaticMaterial()` nudo non lo e'** (#1665). Il default lascia
	// `MaterialSlotName = NAME_None` e `UVChannelData.bInitialized = false`; in Editor la pipeline di build
	// lo sistema da se', ma quel codice e' `WITH_EDITOR` e nel cotto non gira nessuno. Misurato il
	// 2026-08-30 su un pacchetto Development: un secondo dopo il caricamento parte
	// `Ensure condition failed: GetStaticMaterials()[MaterialIndex].UVChannelData.bInitialized`, da
	// `UInstancedStaticMeshComponent::GetMaterialStreamingData()`. Le celle avevano istanze, mesh,
	// materiale, bounds e custom data tutti corretti — e non arrivavano a schermo.
	//
	// `FMeshUVChannelInfo(1.f)` e' l'unico costruttore che mette `bInitialized = true` (densita' 1 su tutti
	// i canali): gli altri due azzerano o non inizializzano. Vale per tutte e tre le mesh procedurali di
	// questo file, che passano per la stessa `BuildFromMeshDescriptions` con `bFastBuild`.
	// 🔑 **E il nome DEVE essere quello del `PolygonGroup`** (riga 294: `"Default"`), perche' e' cosi' che
	// `BuildFromMeshDescriptions` lega la sezione allo slot. Se non combaciano, la sezione esce con
	// `MaterialIndex = -1` — misurato il 2026-08-30 nel pacchetto: `tri=20 idx=60 matIdx=-1/1`, cioe'
	// geometria completa e nessun materiale da usare. Il Cube d'engine, nella stessa run, dava `matIdx=0/1`
	// e si vedeva. E' l'unica differenza rimasta fra i due, dopo che bounds, custom data, materiale,
	// `ScreenSize` e visibilita' erano risultati identici.
	FStaticMaterial CellSlot;
	CellSlot.MaterialSlotName = RTProceduralMeshSlotName;
	CellSlot.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(CellSlot);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;          // obbligatorio fuori dall'Editor: senza, la build a runtime non gira
	Params.bMarkPackageDirty = false;  // transiente: non c'e' package da sporcare
	Params.bCommitMeshDescription = false;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);

	// COLLISIONE ESPLICITA, e non e' rifinitura: il pick della cella e' un `GetHitResultUnderCursor` con
	// `bTraceComplex = false`, e la cella si ricava dall'INDICE DELL'ISTANZA colpita. Con una collisione
	// approssimata un colpo vicino allo spigolo restituirebbe la cella sbagliata — un difetto che si vede
	// solo cliccando, cioe' il piu' caro da trovare. Un prisma esagonale e' convesso, quindi il suo scafo
	// convesso e' la forma ESATTA: qui la collisione e' piu' precisa di quella del cilindro sostituito.
	Mesh->CreateBodySetup();
	if (UBodySetup* Body = Mesh->GetBodySetup())
	{
		Body->AggGeom.ConvexElems.Reset();

		FKConvexElem Hull;
		Hull.VertexData.Reserve(12);
		for (const FVector& Corner : Corners)
		{
			Hull.VertexData.Add(FVector(Corner.X, Corner.Y, +RTCellPrismHalfHeight));
			Hull.VertexData.Add(FVector(Corner.X, Corner.Y, -RTCellPrismHalfHeight));
		}
		Hull.UpdateElemBox();
		Body->AggGeom.ConvexElems.Add(Hull);

		Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
	}

	Cached.Reset(Mesh);
	return Mesh;
}

#if !UE_BUILD_SHIPPING
UStaticMesh* ARTHexMapActor::GetKnowledgeVolumeMesh()
{
	// Stessa disciplina di `GetCellPrismMesh`: una sola per processo, tenuta fuori dal GC senza `AddToRoot`.
	static TStrongObjectPtr<UStaticMesh> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	// ⚠️ I vertici vengono da `HexCorners`, come per il disco e per la stessa ragione: due esagoni nati da
	// due trigonometrie divergono, e qui divergerebbero DALLA CELLA che stanno descrivendo.
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(FVector::ZeroVector, RTCellPrismRadius);
	if (Corners.Num() != 6)
	{
		return nullptr;
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();

	// 🔴 **La differenza da `GetCellPrismMesh` e' tutta qui**: `Z` va da `0` a `2R` invece che da `-R` a
	// `+R`. Il pivot sta alla BASE, quindi scalare la mesh la fa crescere verso l'alto restando appoggiata,
	// mentre un pivot centrato la farebbe crescere anche verso il basso — e quattro volumi che affondano di
	// quantita' diverse non si confrontano a vista.
	TArray<FVertexID> Top;
	TArray<FVertexID> Bottom;
	Top.Reserve(6);
	Bottom.Reserve(6);
	for (int32 Corner = 0; Corner < 6; ++Corner)
	{
		const FVertexID TopId = Description.CreateVertex();
		Positions[TopId] = FVector3f(
			static_cast<float>(Corners[Corner].X), static_cast<float>(Corners[Corner].Y),
			RTCellPrismRadius * 2.f);
		Top.Add(TopId);

		const FVertexID BottomId = Description.CreateVertex();
		Positions[BottomId] = FVector3f(
			static_cast<float>(Corners[Corner].X), static_cast<float>(Corners[Corner].Y), 0.f);
		Bottom.Add(BottomId);
	}

	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = RTProceduralMeshSlotName;

	auto AddFace = [&Description, Group](const TArray<FVertexID>& Ring)
	{
		TArray<FVertexInstanceID> Instances;
		Instances.Reserve(Ring.Num());
		for (const FVertexID Vertex : Ring)
		{
			Instances.Add(Description.CreateVertexInstance(Vertex));
		}
		Description.CreatePolygon(Group, Instances);
	};

	AddFace(Top);

	TArray<FVertexID> BottomReversed = Bottom;
	Algo::Reverse(BottomReversed);
	AddFace(BottomReversed);

	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const int32 Next = (Edge + 1) % 6;
		AddFace(TArray<FVertexID>{ Bottom[Edge], Bottom[Next], Top[Next], Top[Edge] });
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("RT_KnowledgeVolume"), RF_Transient);

	// Slot inizializzato, come sopra (#1665).
	FStaticMaterial VolumeSlot;
	VolumeSlot.MaterialSlotName = RTProceduralMeshSlotName;  // deve combaciare col PolygonGroup (#1665)
	VolumeSlot.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(VolumeSlot);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bMarkPackageDirty = false;
	Params.bCommitMeshDescription = false;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);

	// ⚠️ **Nessuna collisione, a differenza del disco.** Il prisma della cella ne ha una perche' e' il proxy
	// di click della selezione; questo si guarda e basta, e una collisione qui intercetterebbe i colpi
	// destinati alle celle sotto — cioe' romperebbe la selezione ogni volta che il debug e' acceso.

	Cached.Reset(Mesh);
	return Mesh;
}

void ARTHexMapActor::SetKnowledgeDebugEnabled(bool bEnabled, const FRTTeamKnowledge& Knowledge)
{
	bKnowledgeDebug = bEnabled;
	if (!KnowledgeVolumes)
	{
		return;
	}

	// 🔴 **Si ricostruisce da ZERO, sempre.** Non c'e' un aggiornamento incrementale da sbagliare e non c'e'
	// un array parallelo da tenere allineato: e' la ragione per cui questo componente non puo' finire nello
	// stato di indici stantii che `ApplyKnowledgeVeil` deve invece sorvegliare con un `ensure`.
	KnowledgeVolumes->ClearInstances();
	KnowledgeVolumes->SetVisibility(bEnabled);
	if (!bEnabled)
	{
		return;
	}

	const URTHexMapAsset* Map = MapAsset;
	if (!Map || Map->NumCells() == 0)
	{
		return;
	}

	if (UStaticMesh* Volume = GetKnowledgeVolumeMesh())
	{
		KnowledgeVolumes->SetStaticMesh(Volume);
	}

	const TSet<FRTCellId> Visible(Knowledge.VisibleCells);
	const TSet<FRTCellId> Explored(Knowledge.ExploredCells);

	const float UseHexSize = Map->HexSize;
	const float UseLayerH = GetPosedLayerHeight(); // #3632

	// La mesh nasce alta `2 · RTCellPrismRadius`: la scala Z che le da' `Fraction · H` e' quel rapporto.
	// Scritta cosi' invece che con un letterale, resta vera se una mappa cambia `LayerHeight`.
	const float PlanarScale = UseHexSize / RTCellPrismRadius * 0.95f;

	for (const FRTHexCellData& Cell : Map->Cells)
	{
		const bool bVisible = Visible.Contains(Cell.Id);
		const bool bKnown = bVisible || Explored.Contains(Cell.Id);
		// 🔴 **Le celle MAI VISTE non si disegnano** (`#2250`), ed e' il difetto che questo overlay
		// aveva: il velo le toglie — `VeilInstances` posa `FVector::ZeroVector`, il vuoto che [D-225]
		// prescrive — e questo le rimetteva, un prisma per ognuna. Misurato in PIE il 2026-09-04: **267 su
		// 331**, l'81% dei prismi. Acceso il debug, la partizione che si voleva ispezionare spariva.
		//
		// ⚠️ **Non e' una scelta di leggibilita', e' una contraddizione tolta**: uno strumento
		// d'ispezione che ridisegna cio' che il suo oggetto nasconde non puo' misurarlo. E' la regola che
		// `RTDebugConsole.cpp:13` gia' dichiara — *«uno strumento d'ispezione che muove cio' che ispeziona
		// produce sessioni di debug che non si possono confrontare fra loro»*.
		//
		// ➕ Il conteggio delle mai viste NON si perde: `GetKnowledgeDebugCounts` lo ricava come
		// complemento, e resta il numero che `rt.Debug.Knowledge` stampa.
		if (!bKnown)
		{
			continue;
		}

		const int32 Which = bVisible ? RTKnowledgeVolumeLit : RTKnowledgeVolumeRemembered;

		const float Fraction = RTKnowledgeVolumeFractions[Which];
		const float FlatScale = Fraction * UseLayerH / (2.f * RTCellPrismRadius);

		FVector World = URTHexLibrary::AxialToWorld(Cell.Id, GetActorLocation(), UseHexSize, UseLayerH);
		World.Z += static_cast<double>(Cell.Height);
		KnowledgeVolumes->AddInstance(
			FTransform(FRotator::ZeroRotator, World, FVector(PlanarScale, PlanarScale, FlatScale)),
			/*bWorldSpace=*/ true);
	}
}

int32 ARTHexMapActor::KnowledgeVolumeInstanceCount() const
{
	return KnowledgeVolumes ? KnowledgeVolumes->GetInstanceCount() : 0;
}

void ARTHexMapActor::GetKnowledgeDebugCounts(int32& OutHidden, int32& OutRemembered, int32& OutLit) const
{
	OutHidden = 0;
	OutRemembered = 0;
	OutLit = 0;
	const URTHexMapAsset* Map = MapAsset;
	if (!KnowledgeVolumes || !Map)
	{
		return;
	}

	// Si legge la scala REALE e si ricava la frazione, invece di fidarsi di un contatore: la stessa scelta
	// che `GetVeilCounts` motiva — un contatore proverebbe che la funzione sa contare, non che ha disegnato.
	//
	// 🔑 **Le MAI VISTE fanno eccezione, e per costruzione**: da `#2250` non si disegnano piu', quindi
	// non c'e' un'istanza da leggere. Si ricavano per COMPLEMENTO — le celle dell'asset meno quelle davvero
	// posate — che tiene la promessa del commento qui sopra invece di romperla: se il disegno smettesse di
	// funzionare, le due famiglie lette calerebbero e il complemento si gonfierebbe, cioe' il difetto si
	// vedrebbe nei numeri. Un contatore memorizzato all'accensione, no.
	const float UseLayerH = GetPosedLayerHeight(); // #3632
	if (UseLayerH <= 0.f)
	{
		return;
	}

	for (int32 I = 0; I < KnowledgeVolumes->GetInstanceCount(); ++I)
	{
		FTransform Xf;
		if (!KnowledgeVolumes->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
		{
			continue;
		}
		const double Fraction = Xf.GetScale3D().Z * 2.0 * static_cast<double>(RTCellPrismRadius) / UseLayerH;

		// L'indice della frazione piu' vicina. Tolleranza generosa perche' le quattro frazioni distano
		// almeno 1/6 fra loro: qui non si distinguono valori vicini, si classifica in quattro cassetti.
		auto Vicino = [Fraction](int32 Which)
		{
			return FMath::Abs(Fraction - static_cast<double>(RTKnowledgeVolumeFractions[Which])) < 0.05;
		};
		if (Vicino(RTKnowledgeVolumeLit))              { ++OutLit; }
		else if (Vicino(RTKnowledgeVolumeRemembered))  { ++OutRemembered; }
	}

	// Il COMPLEMENTO: cio' che l'asset ha e che non e' stato disegnato. Solo a debug acceso — spento non
	// c'e' niente di posato, e dire «tutte mai viste» sarebbe falso quanto dire zero.
	if (bKnowledgeDebug)
	{
		OutHidden = FMath::Max(0, Map->NumCells() - OutLit - OutRemembered);
	}
}
#endif // !UE_BUILD_SHIPPING

FTransform ARTHexMapActor::InteriorWallPanel(const FVector2D& LocalA, const FVector2D& LocalB,
	const FVector& CellCentreWorld, float PanelHeight, float PanelThickness)
{
	const FVector2D Along = LocalB - LocalA;
	const double Length = Along.Size();
	if (Length <= UE_KINDA_SMALL_NUMBER)
	{
		return FTransform::Identity;
	}

	const FVector2D Mid = (LocalA + LocalB) * 0.5;
	const FVector PanelCentre(
		CellCentreWorld.X + Mid.X,
		CellCentreWorld.Y + Mid.Y,
		CellCentreWorld.Z + RTCellTopZ + PanelHeight * 0.5);

	// 🔴 **+90 gradi, e togliendoli il muro esce di traverso.** Il cubo engine e' 100 uu per lato e la
	// convenzione dei pannelli — quella che `EdgeRotation` gia' segue — mette lo SPESSORE sulla X e la
	// LUNGHEZZA sulla Y. Lo yaw deve quindi puntare la X **perpendicolare** al muro, non lungo di esso.
	// La prima stesura usava l'angolo del muro e basta: a schermo i muri interni comparivano ruotati di un
	// angolo retto rispetto al gesto, ed e' cosi' che l'autore se n'e' accorto.
	const double AlongDegrees = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
	const FRotator Rotation(0.0, AlongDegrees + 90.0, 0.0);

	// Y porta la lunghezza (il cubo e' 100 uu, quindi la scala e' `Length / 100`), X lo spessore, Z l'altezza.
	return FTransform(Rotation, PanelCentre,
		FVector(PanelThickness, Length / 100.0, static_cast<double>(PanelHeight) / 100.0));
}

ARTHexMapActor::ARTHexMapActor()
{
	// Tick abilitabile ma SPENTO all'avvio: si accende solo quando c'e' un'anteprima da disegnare
	// (SetHoveredCell/SetPreviewPath), cosi' la mappa resta inerte fuori dalla pianificazione.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Cells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cells"));
	SetRootComponent(Cells);
	Cells->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // per il raycast di selezione (H2)
	Cells->SetCollisionResponseToAllChannels(ECR_Block);

	// Tre float per istanza: il colore della superficie, che `RebuildInstances` scrive per ogni cella.
	// ⚠️ Da soli non si vedono. Servono a un materiale che legga `PerInstanceCustomData`; in `M_HexCell` il
	// canale e' l'**Emissive**, non il Base Color — verificato nel grafo del materiale il 2026-08-23 (#956).
	// La differenza si vede a schermo: un emissive e' auto-illuminato, quindi la board si legge uguale con
	// qualunque luce di scena, ed e' il motivo per cui `PIE-DEBUG-CELLS` non dipende dall'illuminazione;
	// col materiale di default della mesh engine le celle restano grigie e questi float sono inerti.
	// Sta qui e non in `RebuildInstances` perche' `AddInstance` alloca i float alla creazione dell'istanza:
	// impostarlo dopo lascerebbe le istanze gia' aggiunte senza spazio dove scrivere.
	// ➕ **Sei e non piu' tre, da #3679**: i canali 3..5 sono il trattamento del piano rispetto a quello attivo
	// (`RTLayerPresentation`), neutro a zero. Glifi e griglia li portano anche loro; i fantasmi del piano no.
	Cells->NumCustomDataFloats = RTLayerPresentation::NumCustomDataFloats;

	// Segnaposto di COSTRUZIONE, sostituito dal prisma esagonale al primo `RebuildInstances` — che
	// `OnConstruction` chiama sempre, quindi a schermo questo cilindro non arriva mai.
	// ⚠️ Resta perche' `GetCellPrismMesh()` NON puo' essere chiamata da un costruttore: crea una `UObject` e
	// costruisce collisione, cioe' lavoro che nel costruttore del CDO non si fa. Il fallback vero e' li'.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Cells->SetStaticMesh(CylinderMesh.Object);
	}

	// Rilievo del costo: stessa mesh, ma SENZA collisione. E' la prima delle due difese — la seconda e'
	// `IsPickOnSelectableCell`, che scarta i colpi su qualunque componente diverso da `Cells`. Serve entrambe:
	// questa evita lavoro inutile al raycast, quella impedisce che una geometria futura rubi i click perche'
	// qualcuno si e' dimenticato di questa riga.
	Relief = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Relief"));
	Relief->SetupAttachment(Cells);
	Relief->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Relief->SetCollisionResponseToAllChannels(ECR_Ignore);
	Relief->CastShadow = false; // e' uno strumento di lettura, non scenografia: le ombre confonderebbero il profilo
	if (CylinderMesh.Succeeded())
	{
		Relief->SetStaticMesh(CylinderMesh.Object);
	}

	// Volume della SUPERFICIE (`#2936`): stessa disciplina del rilievo — nessuna collisione, nessuna ombra.
	//
	// ⚠️ **Nessuna collisione e' un requisito, non un'ottimizzazione**: un volume di fumo che intercettasse il
	// raycast del puntatore ruberebbe il click alla cella sotto di se', e il giocatore non potrebbe piu'
	// bersagliare la cella che il fumo occupa — cioe' proprio il caso che `#2870` esiste per aprire.
	SurfaceVolumes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SurfaceVolumes"));
	SurfaceVolumes->SetupAttachment(Cells);
	SurfaceVolumes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurfaceVolumes->SetCollisionResponseToAllChannels(ECR_Ignore);
	SurfaceVolumes->CastShadow = false;
	// #3706: tinta del piano e trattamento di D-501, per istanza. Nel costruttore per la ragione scritta per `Cells`.
	SurfaceVolumes->NumCustomDataFloats = RTLayerPresentation::VolumeNumCustomDataFloats;
	if (CylinderMesh.Succeeded())
	{
		SurfaceVolumes->SetStaticMesh(CylinderMesh.Object);
	}

	// Volumi delle regole di blocco: stessa disciplina del rilievo — nessuna collisione, nessuna ombra.
	// Il corpo strutturale (#1865): sotto le superfici, senza collisione — non si calpesta e non si clicca.
	StructuralBodies = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StructuralBodies"));
	StructuralBodies->SetupAttachment(Cells);
	StructuralBodies->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StructuralBodies->SetCollisionResponseToAllChannels(ECR_Ignore);
	StructuralBodies->CastShadow = false;
	StructuralBodies->NumCustomDataFloats = RTLayerPresentation::VolumeNumCustomDataFloats; // #3706

	Blockers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Blockers"));
	Blockers->SetupAttachment(Cells);
	Blockers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blockers->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blockers->CastShadow = false;
	Blockers->NumCustomDataFloats = RTLayerPresentation::VolumeNumCustomDataFloats; // #3706
	if (CylinderMesh.Succeeded())
	{
		Blockers->SetStaticMesh(CylinderMesh.Object);
	}

	// Pannelli di bordo: mesh PROPRIA. Un pannello non e' un cilindro schiacciato, e nessuna scala trasforma
	// l'uno nell'altro — e' l'unico caso in cui un secondo componente aggiunge qualcosa.
	EdgeFeatures = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EdgeFeatures"));
	EdgeFeatures->SetupAttachment(Cells);
	EdgeFeatures->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EdgeFeatures->SetCollisionResponseToAllChannels(ECR_Ignore);
	EdgeFeatures->CastShadow = false;
	EdgeFeatures->NumCustomDataFloats = RTLayerPresentation::VolumeNumCustomDataFloats; // #3706
	// #3706: il percorso ha un owner, ed e' lo stesso che il commandlet scrive. Soft: si carica alla ricostruzione.
	VolumeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s.%s"),
		RTLayerPresentation::VolumeMaterialPackage, RTLayerPresentation::VolumeMaterialName)));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		EdgeFeatures->SetStaticMesh(CubeMesh.Object);
	}

	// I quattro glifi di superficie (#956, `D-183`). Stessa disciplina degli altri tre — nessuna collisione,
	// nessuna ombra — piu' i custom data, che qui servono: il colore del segno arriva per istanza come per
	// `Cells`, ed e' l'unico altro componente a portarli.
	//
	// ⚠️ La MESH non si assegna qui: `GetCellGlyphMesh` la costruisce a runtime, e chiamarla nel costruttore
	// del CDO creerebbe oggetti transitori durante la fase di caricamento delle classi.
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		SurfaceGlyphs[Ring] = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
			*FString::Printf(TEXT("SurfaceGlyph%d"), Ring + 1));
		SurfaceGlyphs[Ring]->SetupAttachment(Cells);
		SurfaceGlyphs[Ring]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SurfaceGlyphs[Ring]->SetCollisionResponseToAllChannels(ECR_Ignore);
		SurfaceGlyphs[Ring]->CastShadow = false;
		SurfaceGlyphs[Ring]->NumCustomDataFloats = RTLayerPresentation::NumCustomDataFloats; // colore + trattamento (#3679)
	}

	// 🔑 **I GHOST della timeline di pianificazione** (`CP 11.5`, `#172`): uno per fase del piano.
	//
	// ⚠️ **Un ISM e non un Actor per ghost, e non `DrawDebugLine`**, ed e' la voce «budget di presentazione»
	// della DoD: *«pooling di mesh/decal, nessun Actor persistente per preview, aggiornamento a frequenza
	// limitata (non ogni Tick)»*. Le tre cose cadono da questa scelta invece di richiedere disciplina:
	//
	//  - il **pooling** e' il componente stesso — le istanze si aggiungono e si tolgono da un oggetto solo;
	//  - **nessun Actor**: un ISM e' un componente di questa board, non un attore che nasce e muore;
	//  - **non ogni Tick**: un'istanza posata RESTA posata. `DrawDebugLine` va riemessa a ogni fotogramma,
	//    ed e' la ragione per cui l'anteprima esistente tiene acceso il `Tick` finche' c'e' qualcosa da
	//    mostrare. Qui si scrive quando la timeline cambia, e poi non si paga piu' niente.
	PlanGhosts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PlanGhosts"));
	PlanGhosts->SetupAttachment(Cells);
	PlanGhosts->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanGhosts->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlanGhosts->CastShadow = false;
	// Il colore per istanza porta il grado di CERTEZZA, che e' l'informazione che distingue un ghost
	// confermato da uno previsto: senza custom data servirebbe un componente per livello.
	// ⚠️ **Tre e non sei** (#3679): i fantasmi del piano non seguono il trattamento dei piani, come le unita'.
	// `M_HexCell` legge i canali 3..5 col default a zero, quindi qui restano pieni.
	PlanGhosts->NumCustomDataFloats = 3;

	// ➕ **Il contorno dei fantasmi** ([D-508], #3704): il riempimento dice la certezza, il contorno il piano della cella
	// del fantasma (nero attivo, bianco sopra, viola sotto). E' un componente a se' con la mesh della griglia, perche'
	// il colore del riempimento e' gia' occupato. Tre float, il colore: nessun trattamento, il contorno resta pieno.
	PlanGhostBorders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PlanGhostBorders"));
	PlanGhostBorders->SetupAttachment(Cells);
	PlanGhostBorders->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanGhostBorders->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlanGhostBorders->CastShadow = false;
	PlanGhostBorders->NumCustomDataFloats = 3;

	// I volumi di conoscenza (debug). Stessa disciplina degli altri — nessuna collisione, nessuna ombra — e
	// nascosto per default: si accende da `rt.Debug.Knowledge`, e una board che lo mostrasse all'avvio
	// rivelerebbe cio' che la squadra non sa a chiunque apra il livello.
	//
	// ⚠️ **Si crea anche in Shipping, dove nessuno lo puo' riempire.** Il `CreateDefaultSubobject` NON e'
	// condizionale, di proposito: un CDO con un insieme di sottooggetti diverso fra due build e' una
	// differenza di layout che si paga alla deserializzazione, e un ISM senza mesh e senza istanze non
	// disegna nulla. Cio' che sparisce in Shipping e' `SetKnowledgeDebugEnabled`, cioe' il solo modo di
	// popolarlo.
	//
	// ⚠️ La MESH non si assegna qui, per la stessa ragione dei glifi: `GetKnowledgeVolumeMesh` costruisce una
	// `UObject` e il costruttore del CDO non e' il posto.
	// 🔑 La griglia (#1758). Stessa disciplina delle altre famiglie di lettura — nessuna collisione, nessuna
	// ombra — piu' i custom data, che qui servono: il velo deve poterla ATTENUARE e non solo nasconderla, o
	// ricordo e osservazione diventerebbero indistinguibili sul confine ([D-227]).
	//
	// ⚠️ Visibile per DEFAULT, al contrario di `KnowledgeVolumes`: il confine fra celle e' cio' con cui si
	// conta il movimento, e il DoD di #1758 lo pretende acceso senza che nessuno lo chieda.
	//
	// ⚠️ La MESH non si assegna qui, per la stessa ragione dei glifi: `GetCellBorderMesh` costruisce una
	// `UObject` e il costruttore del CDO non e' il posto.
	CellBorders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CellBorders"));
	CellBorders->SetupAttachment(Cells);
	CellBorders->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CellBorders->SetCollisionResponseToAllChannels(ECR_Ignore);
	CellBorders->CastShadow = false;
	CellBorders->NumCustomDataFloats = RTLayerPresentation::NumCustomDataFloats; // colore + trattamento (#3679)

	KnowledgeVolumes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("KnowledgeVolumes"));
	KnowledgeVolumes->SetupAttachment(Cells);
	KnowledgeVolumes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KnowledgeVolumes->SetCollisionResponseToAllChannels(ECR_Ignore);
	KnowledgeVolumes->CastShadow = false;
	KnowledgeVolumes->SetVisibility(false);

#if WITH_EDITOR
	// `CreateEditorOnlyDefaultSubobject`, non `CreateDefaultSubobject`: quest'ultimo non imposta
	// `bIsEditorOnly` e il componente verrebbe serializzato nel `.umap` e COTTO nel pacchetto — dove la
	// classe non dichiara piu' ne' la proprieta' ne' il subobject (vedi `WITH_EDITORONLY_DATA` sopra). La
	// guardia `if (!CoordinateLabels) return;` in `RebuildCoordinateLabels` copre gia' il `nullptr` fuori
	// editor, quindi il costruttore non deve fare altro.
	CoordinateLabels = CreateEditorOnlyDefaultSubobject<ULineBatchComponent>(TEXT("CoordinateLabels"));
	if (CoordinateLabels)
	{
		CoordinateLabels->SetupAttachment(RootComponent);
		// Come le altre famiglie di lettura: non e' scenografia e non deve intercettare il raycast di
		// selezione, che valida il componente colpito.
		CoordinateLabels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CoordinateLabels->SetCastShadow(false);
	}
#endif
}

void ARTHexMapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// L'asset e' la fonte autorevole; le istanze ISM ne sono solo la VISTA. Ricostruirla a ogni costruzione
	// (apertura del livello, spostamento dell'actor, undo, spawn in gioco) tiene le due cose allineate senza
	// dover premere RebuildInstances a mano. Copre anche il caso del gioco: SpawnActor chiama OnConstruction.
	RebuildInstances();

#if WITH_EDITOR
	BindToMapAsset();
#endif
}

#if WITH_EDITOR
void ARTHexMapActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Cambiare MapAsset, ActiveLayer, LayerView, DemoRadius, HexSize/LayerHeight o CellMesh cambia cosa si deve
	// vedere: si ricostruisce sempre (l'actor ha poche proprieta' e la ricostruzione e' idempotente).
	BindToMapAsset(); // se e' cambiato l'asset, si seguono le notifiche di quello nuovo
	RebuildInstances();
}

void ARTHexMapActor::CheckForErrors()
{
	Super::CheckForErrors();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 MapActorCount = 0;
	const URTHexMapAsset* Asset = nullptr;
	TArray<FRTSpawnPlacement> Spawns;
	URTMapTemplateLibrary::CollectFromWorld(World, MapActorCount, Asset, Spawns);

	TArray<FRTMapTemplateIssue> Issues;
	URTMapTemplateLibrary::ValidateTemplate(MapActorCount, Asset, Spawns, Issues);

	for (const FRTMapTemplateIssue& Issue : Issues)
	{
		// Solo le segnalazioni di LIVELLO, che sono quelle senza un marker a cui appartenere. Quelle con
		// un'etichetta le emette il proprio `ARTSpawnPoint`, cosi' ogni messaggio del Map Check porta
		// all'attore che si deve aprire.
		if (!Issue.Label.IsEmpty())
		{
			continue;
		}

		URTMapTemplateLibrary::EmitIssueToMapCheck(Issue, this);
	}
}

void ARTHexMapActor::BindToMapAsset()
{
	if (BoundAsset.Get() == MapAsset)
	{
		return; // gia' iscritti all'asset giusto
	}
	UnbindFromMapAsset();
	if (MapAsset)
	{
		MapChangedHandle = MapAsset->OnMapChanged.AddUObject(this, &ARTHexMapActor::RebuildAllForAssetChange);
		BoundAsset = MapAsset;
	}
}

void ARTHexMapActor::UnbindFromMapAsset()
{
	if (URTHexMapAsset* Previous = BoundAsset.Get())
	{
		Previous->OnMapChanged.Remove(MapChangedHandle);
	}
	MapChangedHandle.Reset();
	BoundAsset = nullptr;
}

void ARTHexMapActor::BeginDestroy()
{
	UnbindFromMapAsset();
	Super::BeginDestroy();
}
#endif

bool ARTHexMapActor::IsPickOnSelectableCell(const UPrimitiveComponent* HitComponent, int32 InstanceIndex) const
{
	// L'indice di istanza appartiene al componente COLPITO: va quindi validato contro quel componente, non
	// contro l'actor che lo contiene. Le due condizioni sono una regola sola.
	return HitComponent != nullptr
		&& HitComponent == Cells.Get()
		&& InstanceCells.IsValidIndex(InstanceIndex);
}

FRTCellId ARTHexMapActor::CellForInstance(int32 InstanceIndex) const
{
	return InstanceCells.IsValidIndex(InstanceIndex) ? InstanceCells[InstanceIndex] : FRTCellId();
}

const TArray<FRTCellId>& ARTHexMapActor::GetUnreachableCells() const
{
	if (!bUnreachableDirty)
	{
		return UnreachableCells;
	}
	bUnreachableDirty = false;
	UnreachableCells.Reset();

	// Gli spawn si chiedono alla stessa funzione che allestisce la partita: misurare la raggiungibilita' da
	// una cella qualunque risponderebbe di una partita che non si gioca.
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(MapAsset, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
		if (Start.Num() > 0)
		{
			UnreachableCells = URTArenaCriteriaLibrary::FindUnreachableCells(MapAsset, Start[0]);
		}
	}
	return UnreachableCells;
}

ARTHexMapActor* ARTHexMapActor::FindInWorld(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARTHexMapActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ARTHexMapActor::SetHoveredCell(const FRTCellId& Cell, bool bValid)
{
	HoveredCell = Cell;
	bHoveredValid = bValid;
	// Il tick serve solo mentre c'e' qualcosa da disegnare: fuori dalla pianificazione l'actor resta inerte.
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewPath(const TArray<FRTCellId>& Path)
{
	PreviewPathArea.Cells = Path;
	PreviewPathArea.Meaning = ERTOverlayMeaning::PathTrace;
	SetActorTickEnabled(HasAnythingToDraw());
}

bool ARTHexMapActor::HasAnythingToDraw() const
{
	return bCellOverlay
		|| bHoveredValid
		|| PreviewPathArea.Cells.Num() > 0
		|| PreviewHitArea.Cells.Num() > 0
		|| PreviewReachableArea.Cells.Num() > 0
		|| PreviewRangeArea.Cells.Num() > 0
		|| bPreviewAttackValid
		|| bHasPreviewSightBlock
		|| PlaybackFootprintCells.Num() > 0
		|| PlaybackStructureHits.Num() > 0
		|| PlaybackTracers.Num() > 0
		|| PlaybackCues.Num() > 0
		// Una dissolvenza del velo in volo e' lavoro da fare per fotogramma quanto un'anteprima (`#2875`).
		|| VeilCellsInTransition > 0
		// La linea di confine della vista (#3761) si ridisegna a ogni fotogramma, come l'anteprima.
		|| VisionBorder.Num() > 0
		// E gli archi dei nemici in vista che il giocatore chiede (#3762).
		|| HasEnemyArcToDraw()
		// #3760: le linee dei muri ricordati si ridisegnano a ogni fotogramma.
		|| bRememberedWallsToDraw;
}

void ARTHexMapActor::SetPreviewHitCells(const TArray<FRTCellId>& HitCells, const TArray<FRTCellId>& AllyCells)
{
	// Si copia e basta: le celle arrivano gia' calcolate da URTHexCombatLibrary::HexHitCells. Rifiltrarle qui
	// sarebbe un secondo calcolo, e due calcoli della stessa cosa prima o poi divergono (invariante #1).
	PreviewHitArea.Cells = HitCells;
	PreviewHitArea.Meaning = ERTOverlayMeaning::Attack;
	PreviewAllyHitArea.Cells = AllyCells;
	PreviewAllyHitArea.Meaning = ERTOverlayMeaning::FriendlyFire;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewReachableCells(const TArray<FRTCellId>& ReachableCells)
{
	PreviewReachableArea.Cells = ReachableCells;
	PreviewReachableArea.Meaning = ERTOverlayMeaning::Movement;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewRangeCells(const TArray<FRTCellId>& RangeCells)
{
	PreviewRangeArea.Cells = RangeCells;
	PreviewRangeArea.Meaning = ERTOverlayMeaning::AbilityRange;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewAttack(const FRTCellId& OriginCell, const FRTCellId& AimCell, bool bValid,
	bool bOriginPredicted)
{
	// Si copia e basta, come le celle colpite: l'origine la deriva `URTHexCombatLibrary::BlastOriginCell`,
	// che e' la stessa risposta che usera' il Blast. Ricalcolarla qui sarebbe la seconda origine.
	PreviewAttackOrigin = OriginCell;
	PreviewAttackAim = AimCell;
	bPreviewAttackValid = bValid;
	bPreviewOriginPredicted = bOriginPredicted;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewSightBlock(bool bBlocked, const FRTCellId& From, const FRTCellId& BlockedAt)
{
	// Si copia e basta, come le celle colpite e l'origine dell'attacco: `BlockedAt` lo produce
	// `DescribeLineOfSight` e ricalcolarlo qui sarebbe la seconda risposta alla stessa domanda.
	bHasPreviewSightBlock = bBlocked;
	PreviewSightFrom = From;
	PreviewSightBlockedAt = BlockedAt;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::AddPlaybackFootprint(const TArray<FRTCellId>& FootprintCells)
{
	// Si copia e basta, come le celle colpite dell'anteprima: arrivano da `FRTResolvedEvent::HitCells`, che
	// il resolver ha gia' prodotto. ⛔ Nessun filtro e nessun ricalcolo qui — sarebbero una seconda risposta
	// alla stessa domanda, ed e' l'invariante #1.
	//
	// ⚠️ `Append` e non assegnazione: `un evento -> un segnale`, e due impronte nello stesso Blast sono due
	// fatti distinti. ⛔ Nessuna deduplicazione: due aree che si sovrappongono si sovrappongono davvero, e
	// toglierne una sarebbe l'aggregazione furba che la v0.1 esclude (`#2453`).
	// ⚠️ `FootprintCells` e non `Cells`: `Cells` e' gia' un membro di questo actor — le istanze della
	// griglia — e il parametro lo nasconderebbe (`C4458`, che qui e' un errore, non un warning).
	PlaybackFootprintCells.Append(FootprintCells);
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackFootprint()
{
	PlaybackFootprintCells.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPlaybackTracers(const TArray<FRTPlaybackTracer>& Tracers)
{
	PlaybackTracers = Tracers;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackTracers()
{
	PlaybackTracers.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPlaybackCues(const TArray<FRTPlaybackCue>& Cues)
{
	PlaybackCues = Cues;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackCues()
{
	PlaybackCues.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::AddPlaybackStructureHit(const FRTCellId& Cell, const FRTCellId& Toward, bool bDestroyed)
{
	// Si copia e basta, come l'impronta qui sopra: i tre valori arrivano da `FRTResolvedEvent`, che li porta
	// dal resolver. Nessun ricalcolo del bordo e nessuna interrogazione della mappa — sarebbero la seconda
	// risposta a una domanda gia' chiusa (invariante #1), ed e' il divieto esplicito di `#2828`.
	//
	// ⚠️ `Emplace` e non assegnazione: `un evento -> un segnale`, e due muri colpiti nello stesso Blast
	// sono due fatti distinti. ⛔ Nessuna deduplicazione, nemmeno sullo stesso bordo: due colpi su una
	// stessa barriera sono due colpi, e fonderli sarebbe l'aggregazione furba che la v0.1 esclude (`#2453`).
	PlaybackStructureHits.Emplace(Cell, Toward, bDestroyed);
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackStructureHits()
{
	PlaybackStructureHits.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 🔑 **La CONVERGENZA del velo segue il tempo; il suo CONTENUTO no** (`#2875`). I target li decide
	// `ApplyKnowledgeVeil` ai due punti di refresh della conoscenza, e questa riga non li tocca: fa avanzare
	// il valore disegnato verso un bersaglio gia' scelto. `Veil.FollowsRefreshPoints` resta verde per
	// costruzione — misura le emissioni di `OnTeamKnowledgeRefreshed`, che questa funzione non produce.
	//
	// ⚠️ **A regime non costa niente**: `VeilCellsInTransition` va a zero quando tutto e' converso, e
	// `HasAnythingToDraw` spegne il `Tick`.
	if (VeilCellsInTransition > 0)
	{
		AdvanceVeilTransition(DeltaSeconds);
		if (VeilCellsInTransition == 0)
		{
			SetActorTickEnabled(HasAnythingToDraw());
		}
	}

	if (bCellOverlay)
	{
		DrawCellOverlay(); // prima: l'anteprima di pianificazione deve restare leggibile SOPRA
	}
	DrawVisionBorder();
	// #3762: ogni arco nel colore della squadra del suo nemico, e sotto la linea di confine per spessore.
	for (const FRTEnemyArcView& Arco : GetShownEnemyArcs())
	{
		DrawEdgeLines(Arco.Edges, Arco.Color, RTEnemyArcThickness);
	}
	DrawRememberedWallsAsLines();
	DrawPlanningPreview();
}

void ARTHexMapActor::SetCellOverlayEnabled(bool bEnabled)
{
	bCellOverlay = bEnabled;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetCellBordersVisible(bool bVisible)
{
	bCellBordersVisible = bVisible;
	if (CellBorders)
	{
		// 🔴 **Visibilita', non ricostruzione**, ed e' il requisito del DoD invece di un'ottimizzazione: un
		// `RebuildInstances` qui rifarebbe l'intera board (7 651 celle su arena piena) e — peggio —
		// azzererebbe `LastVeilState` e sorelle insieme agli indici a cui si riferiscono, costringendo il
		// velo successivo a ridipingere tutto. Un toggle di presentazione che invalida il velo non e' sola
		// presentazione.
		//
		// ⛔ Nessuna mutazione di `FRTMapState`, graph revision, path cache, snapshot, TurnLog o stato di
		// rete: il componente e' una vista DERIVATA dall'asset, e spegnerlo non toglie una cella dal grafo.
		CellBorders->SetVisibility(bVisible);
	}
}

void ARTHexMapActor::DrawCellOverlay() const
{
	const UWorld* World = GetWorld();
	if (!World || !MapAsset)
	{
		return; // senza mappa d'autore non c'e' nulla di informativo da mostrare
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);

	const auto DrawRing = [World, &Origin, Size, LayerH](const FRTCellId& Cell, const FColor& Color, float Scale,
		float Lift, float Thickness)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, Size, LayerH) + FVector(0, 0, Lift);
		const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Size * Scale);
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			DrawDebugLine(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color,
				/*bPersistentLines=*/ false, /*LifeTime=*/ -1.f, /*DepthPriority=*/ 0, Thickness);
		}
	};

	for (const FRTHexCellData& Cell : MapAsset->Cells)
	{
		// `Height` alza l'ISTANZA (RebuildInstances), quindi deve alzare anche le linee: senza, su una cella
		// rialzata il contorno resterebbe sepolto di `Height` unita' dentro il disco.
		const float CellLift = static_cast<float>(Cell.Height);

		// Contorno esterno: che superficie e'. Il costo di traversata si legge dalla superficie, non da un numero.
		// La scala e' 0.90 (non 0.86) per stare appena FUORI dal disco, che copre 0.95 del raggio: cosi' il
		// contorno non lotta con la faccia superiore per lo stesso pixel.
		DrawRing(Cell.Id, URTHexLibrary::SurfaceColor(Cell.Surface), 0.90f, CellLift + RTLiftSurface, /*Thickness=*/ 2.0f);

		// Due marcatori DISTINTI, perche' sono due regole diverse: dove non si passa e dove non si vede.
		if (Cell.bBlocksMovement)
		{
			DrawRing(Cell.Id, URTHexLibrary::BlockedCellColor(), 0.45f, CellLift + RTLiftMarker, 2.5f);
		}
		if (Cell.bBlocksLineOfSight)
		{
			DrawRing(Cell.Id, URTHexLibrary::SightBlockerColor(), 0.64f, CellLift + RTLiftMarker, 2.0f);
		}
	}
}

void ARTHexMapActor::SetPlanPreview(const FRTPlanPreview& Preview)
{
	// Prima della guardia sul componente: la timeline e' un fatto del piano, non dei ghost che la disegnano.
	LastPlanPreview = Preview;

	if (!PlanGhosts)
	{
		return;
	}

	// ⛔ **Si azzera sempre, anche per una timeline vuota**, ed e' il caso dell'annullamento: un piano
	// cancellato deve togliere i suoi ghost, e `Preview.Phases.Num() == 0` e' precisamente come arriva.
	PlanGhosts->ClearInstances();
	PlanGhostCells.Reset();
	if (PlanGhostBorders)
	{
		PlanGhostBorders->ClearInstances(); // #3704: i contorni seguono i fantasmi, uno a uno
	}

	if (Preview.Phases.Num() == 0)
	{
		return;
	}

	// La mesh si assegna qui e non nel costruttore, come per i glifi: `GetCellPrismMesh` la costruisce a
	// runtime, e chiamarla sul CDO creerebbe oggetti transitori al caricamento delle classi.
	if (UStaticMesh* Shape = GetCellPrismMesh())
	{
		PlanGhosts->SetStaticMesh(Shape);
	}
	if (UMaterialInterface* Mat = CellMaterial.LoadSynchronous())
	{
		PlanGhosts->SetMaterial(0, Mat);
		if (PlanGhostBorders)
		{
			PlanGhostBorders->SetMaterial(0, Mat);
		}
	}
	if (PlanGhostBorders)
	{
		if (UStaticMesh* Anello = GetCellBorderMesh())
		{
			PlanGhostBorders->SetStaticMesh(Anello);
		}
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);
	const float PlanarScale = Size / 50.f * 0.95f;

	for (const FRTPhasePreviewEntry& Fase : Preview.Phases)
	{
		// Il ghost sta dove l'unita' SARA' a fase conclusa: e' la domanda a cui questa timeline risponde.
		FVector World = URTHexLibrary::AxialToWorld(Fase.PreviewDestination, Origin, Size, LayerH);
		if (MapAsset)
		{
			if (const FRTHexCellData* Data = MapAsset->FindCell(Fase.PreviewDestination))
			{
				World.Z += static_cast<double>(Data->Height);
			}
		}
		World.Z += RTLiftPreview;

		// 🔴 **I ghost coincidenti si IMPILANO invece di sovrapporsi.** Due fasi possono finire sulla
		// stessa cella per costruzione — il Prep non sposta, il Blast non sposta chi lo esegue, un Move con
		// percorso rifiutato torna alla propria origine — e due istanze alla stessa posa producono
		// z-fighting con un colore che ne occlude un altro a caso. Il colore È l'informazione che questo
		// canale porta (la certezza), quindi perderlo cosi' sarebbe perdere l'unica cosa che i ghost dicono
		// oltre alla posizione.
		int32 GiaSuQuestaCella = 0;
		for (const FRTCellId& Posata : PlanGhostCells)
		{
			if (Posata == Fase.PreviewDestination)
			{
				++GiaSuQuestaCella;
			}
		}
		World.Z += static_cast<double>(GiaSuQuestaCella) * RTGhostStackStep;

		const FTransform Xf(FRotator::ZeroRotator, World,
			FVector(PlanarScale * 0.7f, PlanarScale * 0.7f, RTCellFlatScale));
		const int32 Index = PlanGhosts->AddInstance(Xf, /*bWorldSpace=*/ true);
		PlanGhostCells.Add(Fase.PreviewDestination);

		// 🔑 **Il colore porta la CERTEZZA, non la fase.** Le fasi si distinguono gia' per posizione e per
		// ordine; cio' che il giocatore non puo' dedurre guardando e' quanto ciascuna sia sicura.
		const FLinearColor Colore = GhostColorForCertainty(Fase.Certainty);
		PlanGhosts->SetCustomDataValue(Index, 0, Colore.R);
		PlanGhosts->SetCustomDataValue(Index, 1, Colore.G);
		PlanGhosts->SetCustomDataValue(Index, 2, Colore.B, /*bMarkRenderStateDirty=*/ false);

		// #3704: il contorno, alla stessa posa e alla stessa scala del fantasma, un filo sopra. Il colore lo scrive
		// `ApplyPlanGhostBorderColors` qui sotto, ed e' la stessa funzione che lo riscrive quando cambia il piano attivo.
		if (PlanGhostBorders)
		{
			// La stessa scala planare del fantasma, come la griglia ha la stessa del disco: l'anello esterno della mesh
			// del bordo sta a 1,0, quindi cade sul perimetro del prisma (vedi `RebuildInstances`, la griglia).
			FTransform BordoXf = Xf;
			BordoXf.SetScale3D(FVector(PlanarScale * 0.7f, PlanarScale * 0.7f, 1.f));
			BordoXf.AddToTranslation(FVector(0.0, 0.0, RTLiftCellBorder));
			PlanGhostBorders->AddInstance(BordoXf, /*bWorldSpace=*/ true);
		}
	}

	// Una volta sola, in coda: la stessa disciplina di `RebuildInstances` e di `VeilInstances`.
	PlanGhosts->MarkRenderStateDirty();
	ApplyPlanGhostBorderColors();
}

FLinearColor ARTHexMapActor::GhostColorForCertainty(ERTIntentCertainty Certainty)
{
	// ⚠️ **Tre livelli e non quattro**: `Unknown` non ha una resa, e il catalogo icone lo dichiara —
	// *«le chiavi sono i livelli DISEGNABILI, non i valori dell'enum»*. Un `Unknown` che arriva qui e' un
	// difetto a monte, e si disegna come il piu' debole invece di inventargli un aspetto proprio.
	switch (Certainty)
	{
	case ERTIntentCertainty::Confirmed: return FLinearColor::FromSRGBColor(FColor(90, 220, 120));
	case ERTIntentCertainty::Predicted: return FLinearColor::FromSRGBColor(FColor(220, 200, 90));
	default:                            return FLinearColor::FromSRGBColor(FColor(190, 120, 90));
	}
}

namespace
{
	/**
	 * 🔑 **Una linea dell'anteprima di pianificazione, nel line batcher del MONDO** (`#3508`).
	 *
	 * Fino a `#3508` l'anteprima usava `DrawDebugLine`, che in Shipping e' una funzione vuota
	 * (`UE_ENABLE_DEBUG_DRAWING` vale 0). Misurato sul pacchetto il 2026-10-06: il Development mostrava
	 * ventaglio, portata e percorso, lo Shipping nessuno dei tre. Il line batcher del mondo esiste in ogni
	 * build che non sia un server dedicato (`UWorld::UpdateWorldComponents`), e in Shipping scrive sul PDI
	 * della scena invece che su quello di debug — quindi colore e spessore li conferma solo il pacchetto.
	 *
	 * ⚠️ **Le regole sono quelle di `DrawDebugLine`, ricopiate apposta**, perche' in Development l'anteprima
	 * resti identica: batcher `Foreground` per `SDPG_Foreground` e `World` altrimenti, durata di UN
	 * fotogramma (`DefaultLifeTime`), niente su un server dedicato.
	 *
	 * ⛔ **Non guarda `r.EnableDrawDebugHelpers`**: l'anteprima e' gioco, non debug. Ed e' su quella CVar che
	 * si appoggia il test che la protegge: spenta, `DrawDebugLine` non disegna — come in Shipping — e queste
	 * linee si'.
	 */
	void DisegnaLineaAnteprima(const UWorld* World, const FVector& Da, const FVector& A, const FColor& Colore,
		uint8 Profondita, float Spessore)
	{
		if (!World || World->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		ULineBatchComponent* Batcher = World->GetLineBatcher(Profondita == SDPG_Foreground
			? UWorld::ELineBatcherType::Foreground : UWorld::ELineBatcherType::World);
		if (Batcher)
		{
			Batcher->DrawLine(Da, A, Colore, Profondita, Spessore, Batcher->DefaultLifeTime);
		}
	}
}

void ARTHexMapActor::SetVisionBorder(const TArray<FRTCellId>& VisibleCells)
{
	// Si calcola QUI, dove sta la mappa, e si conservano soltanto i lati: l'actor non tiene la conoscenza di
	// squadra ([D-242]), come per il velo.
	//
	// 🔑 **Solo i lati verso una cella della mappa**, per scelta dell'autore sulla domanda L3 di #3761: la linea segna
	// dove finisce la vista, non dove finisce il pavimento. Sul bordo della mappa e sul perimetro di una piattaforma —
	// dove la vista scende al piano di sotto ([D-511]) — non c'e' linea.
	VisionBorder = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(MapAsset, VisibleCells);
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetEnemyArcs(const TMap<FRTCellId, FRTEnemyArcView>& ArcsByEnemyCell)
{
	EnemyArcs = ArcsByEnemyCell;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetEnemyArcFocus(bool bAll, const FRTCellId& HoveredCell, bool bHasHover)
{
	if (bAll == bEnemyArcsAll && bHasHover == bEnemyArcHover && (!bHasHover || HoveredCell == EnemyArcHoveredCell))
	{
		return;
	}
	bEnemyArcsAll = bAll;
	bEnemyArcHover = bHasHover;
	EnemyArcHoveredCell = HoveredCell;
	SetActorTickEnabled(HasAnythingToDraw());
}

bool ARTHexMapActor::HasEnemyArcToDraw() const
{
	if (EnemyArcs.Num() == 0)
	{
		return false;
	}
	return bEnemyArcsAll || (bEnemyArcHover && EnemyArcs.Contains(EnemyArcHoveredCell));
}

TArray<FRTEnemyArcView> ARTHexMapActor::GetShownEnemyArcs() const
{
	TArray<FRTEnemyArcView> Out;
	if (bEnemyArcsAll)
	{
		// L'ordine di un `TMap` dipende dall'hash: si ordina per cella, perche' la diagnostica non cambi fra due giri.
		TArray<FRTCellId> Chiavi;
		EnemyArcs.GetKeys(Chiavi);
		Chiavi.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		for (const FRTCellId& K : Chiavi)
		{
			Out.Add(EnemyArcs[K]);
		}
	}
	else if (bEnemyArcHover)
	{
		if (const FRTEnemyArcView* Arco = EnemyArcs.Find(EnemyArcHoveredCell))
		{
			Out.Add(*Arco);
		}
	}
	return Out;
}

void ARTHexMapActor::DrawVisionBorder() const
{
	// 🔑 **Il colore e' quello della VISTA nella palette** (`#32ADE6`, [D-368]): l'autore l'ha scelto sulla tavola L1
	// di #3761, fra il bianco caldo e il blu della squadra. Si chiede alla palette e non si ricopia: se il colore
	// della vista cambia, la linea lo segue.
	//
	// ⚠️ **Si disegna anche in vista strategica**, per scelta dell'autore sulla tavola L2: nessun ramo su
	// `IsStrategicPresentation()` qui.
	DrawEdgeLines(VisionBorder, URTOverlayPalette::ColorFor(ERTOverlayMeaning::Vision), RTVisionBorderThickness);
}

void ARTHexMapActor::DrawEdgeLines(const TArray<FRTExposedEdge>& Lati, const FColor& Colore, float Spessore) const
{
	const UWorld* World = GetWorld();
	if (!World || Lati.Num() == 0)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);

	for (const FRTExposedEdge& Lato : Lati)
	{
		// Il tratto e' il LATO dell'esagono: meta' lunghezza per parte, a partire dal punto medio che
		// `EdgeMidpointWorld` deriva dai due centri, e perpendicolare alla direzione del vicino. Con un
		// circumraggio `Size` il lato e' lungo `Size`, quindi gli estremi cadono sugli spigoli di `HexCorners`.
		const FRTHexCellData* Dati = MapAsset ? MapAsset->FindCell(Lato.Cell) : nullptr;
		const FVector Su(0.f, 0.f, (Dati ? static_cast<float>(Dati->Height) : 0.f) + RTLiftVisionBorder);
		const FVector Centro = URTHexLibrary::AxialToWorld(Lato.Cell, Origin, Size, LayerH);
		const FVector Medio = URTHexLibrary::EdgeMidpointWorld(Lato.Cell, Lato.Direction, Origin, Size, LayerH);
		const FVector Verso = FVector(Medio.X - Centro.X, Medio.Y - Centro.Y, 0.f).GetSafeNormal();
		const FVector Lungo = FVector::CrossProduct(Verso, FVector::UpVector) * (Size * 0.5f);
		DisegnaLineaAnteprima(World, Medio - Lungo + Su, Medio + Lungo + Su, Colore, SDPG_World, Spessore);
	}
}

ARTHexMapActor::ERTVeilMemory ARTHexMapActor::MemoryFor(const UInstancedStaticMeshComponent* Component) const
{
	if (Component == CellBorders)
	{
		return ERTVeilMemory::Plan; // l'anello della griglia E' la pianta
	}
	// Tutto il resto nel ricordo non si disegna: disco, glifi, rilievo e volumi di superficie sono il «nessun colore di
	// superficie» della scelta dell'autore; muri, coperture, porte e corpi li ridisegna a linee
	// `DrawRememberedWallsAsLines`, per la seconda scelta dell'autore (le linee della pianta).
	return ERTVeilMemory::Hide;
}

FLinearColor ARTHexMapActor::PlanColor()
{
	// Il ciano chiaro, «quasi olografico»: l'autore l'ha scelto sulla tavola P2 di #3760, fra il bianco e l'ambra.
	return FLinearColor::FromSRGBColor(RTPlanLineColor);
}

void ARTHexMapActor::DrawRememberedWallsAsLines() const
{
	if (!bRememberedWallsToDraw)
	{
		return;
	}
	const UWorld* World = GetWorld();
	if (!World || !MapAsset)
	{
		return;
	}
	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);
	const FColor Colore = PlanColor().ToFColor(/*bSRGB=*/ true);
	const auto Quota = [this](const FRTCellId& Cella)
	{
		const FRTHexCellData* Dati = MapAsset->FindCell(Cella);
		return (Dati ? static_cast<float>(Dati->Height) : 0.f) + RTLiftPreview;
	};

	// I muri dei pannelli di bordo — coperture, porte, muri interni — come un tratto sul pavimento lungo il pannello.
	// La lunghezza viene dall'istanza, cosi' un muro interno corto resta corto.
	if (EdgeFeatures && EdgeFeatures->GetStaticMesh() && LastEdgeFeatureVeilState.Num() == EdgeFeatureCells.Num())
	{
		const float MezzaY = EdgeFeatures->GetStaticMesh()->GetBounds().BoxExtent.Y;
		for (int32 I = 0; I < EdgeFeatureCells.Num(); ++I)
		{
			if (LastEdgeFeatureVeilState[I] != RTVeilRemembered)
			{
				continue;
			}
			FTransform Xf;
			if (!EdgeFeatures->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
			{
				continue;
			}
			// La scala dell'istanza nascosta e' zero: la lunghezza si prende dalla scala piena.
			const float ScalaY = EdgeFeatureBaseScale.IsValidIndex(I) ? EdgeFeatureBaseScale[I].Y : 1.f;
			const FVector Lungo = Xf.GetRotation().GetAxisY() * (MezzaY * ScalaY);
			const FVector Centro = URTHexLibrary::AxialToWorld(EdgeFeatureCells[I], Origin, Size, LayerH);
			const FVector Punto(Xf.GetLocation().X, Xf.GetLocation().Y, Centro.Z + Quota(EdgeFeatureCells[I]));
			DisegnaLineaAnteprima(World, Punto - Lungo, Punto + Lungo, Colore, SDPG_World, 4.f);
		}
	}

	// Le celle che bloccano — colonne, lastre — come un esagono piu' piccolo dentro la cella.
	if (LastBlockerVeilState.Num() == BlockerCells.Num())
	{
		for (int32 I = 0; I < BlockerCells.Num(); ++I)
		{
			if (LastBlockerVeilState[I] != RTVeilRemembered)
			{
				continue;
			}
			const FVector Centro = URTHexLibrary::AxialToWorld(BlockerCells[I], Origin, Size, LayerH)
				+ FVector(0.f, 0.f, Quota(BlockerCells[I]));
			const TArray<FVector> Spigoli = URTHexLibrary::HexCorners(Centro, Size * 0.6f);
			for (int32 S = 0; S < Spigoli.Num(); ++S)
			{
				DisegnaLineaAnteprima(World, Spigoli[S], Spigoli[(S + 1) % Spigoli.Num()], Colore, SDPG_World, 3.f);
			}
		}
	}
}

void ARTHexMapActor::DrawPlanningPreview() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);

	// Quota di una cella: `Height` alza l'istanza, quindi deve alzare anche le linee (senza, l'anteprima
	// sprofonda dentro il disco delle celle rialzate).
	const URTHexMapAsset* Map = MapAsset;
	const auto CellLift = [Map](const FRTCellId& Cell) -> float
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data ? static_cast<float>(Data->Height) : 0.f;
	};

	// Contorno di una cella, dai vertici condivisi con il marker dell'editor (stesso orientamento).
	// `bThroughUnits`: disegna il contorno in FOREGROUND, cioe' senza test di profondita'.
	//
	// Serve alle celle che possono essere OCCUPATE. Il lift di 2.5 unita' mette l'anteprima sopra il terreno,
	// non sopra un'unita': da camera dall'alto il cilindro di chi sta sulla cella copre il contorno per intero.
	// L'arancione del fuoco amico ne era la vittima sistematica — esiste solo dove c'e' un alleato, quindi
	// finiva SEMPRE sotto un cilindro, e l'avviso che deve arrivare prima del lock-in non arrivava mai.
	// Osservato in PIE il 2026-08-08 (`PIE-PREVIEW-AREA`).
	//
	// Non si applica a tutto: celle raggiungibili e traccia del percorso stanno per definizione su celle
	// VUOTE — niente le copre, e metterle in foreground le farebbe vedere attraverso il terreno senza che
	// nessuno l'abbia chiesto.
	const auto DrawCellOutline = [World, &Origin, Size, LayerH, &CellLift](const FRTCellId& Cell, const FColor& Color, float Scale, bool bThroughUnits = false)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(Cell) + RTLiftPreview);
		const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Size * Scale);
		const uint8 Depth = bThroughUnits ? SDPG_Foreground : SDPG_World;
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			DisegnaLineaAnteprima(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color, Depth, /*Spessore=*/ 3.f);
		}
	};

	// Un significato porta con se' colore, scala e profondita': si passa QUELLO, e la grammatica la decide
	// `URTOverlayPalette` invece di tre argomenti ricopiati a ogni chiamata (#1941).
	const auto DrawMeaning = [&DrawCellOutline](const FRTCellId& Cell, const ERTOverlayMeaning Meaning)
	{
		DrawCellOutline(Cell, URTOverlayPalette::ColorFor(Meaning), URTOverlayPalette::ScaleFor(Meaning),
			URTOverlayPalette::DrawsThroughUnits(Meaning));
	};

	// Ordine di disegno: dal meno al piu' urgente, cosi' l'informazione critica resta leggibile sopra.
	// L'ordine non e' piu' soltanto questo commento: e' `URTOverlayPalette::PriorityFor`, e un test lo fissa.

	// Celle raggiungibili: contorno piccolo e tenue. Fa vedere il budget mordere (il fango accorcia il raggio)
	// senza coprire il resto: e' contesto, non una decisione presa.
	for (const FRTCellId& Cell : PreviewReachableArea.Cells)
	{
		DrawMeaning(Cell, ERTOverlayMeaning::Movement);
	}

	// La portata dell'azione armata (`#3507`): dove posso mirare. In targeting prende il posto del ventaglio, e sta
	// sotto l'area colpita, che dice chi colpisco.
	for (const FRTCellId& Cell : PreviewRangeArea.Cells)
	{
		DrawMeaning(Cell, ERTOverlayMeaning::AbilityRange);
	}

	// Traccia del percorso: contorno ciano su ogni cella + segmento fra i centri consecutivi.
	for (int32 I = 0; I < PreviewPathArea.Cells.Num(); ++I)
	{
		DrawMeaning(PreviewPathArea.Cells[I], ERTOverlayMeaning::PathTrace);
		if (I > 0)
		{
			const FVector A = URTHexLibrary::AxialToWorld(PreviewPathArea.Cells[I - 1], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPathArea.Cells[I - 1]) + RTLiftPreview + 1.5f);
			const FVector B = URTHexLibrary::AxialToWorld(PreviewPathArea.Cells[I], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPathArea.Cells[I]) + RTLiftPreview + 1.5f);
			DisegnaLineaAnteprima(World, A, B, URTOverlayPalette::ColorFor(ERTOverlayMeaning::PathTrace), SDPG_World, 4.f);
		}
	}

	// ORIGINE e MIRA dell'attacco pianificato, disegnate PRIMA dell'area: rispondono a una domanda diversa —
	// «da dove parte» e «verso cosa» — e senza di loro una singola zona rossa deve significare tutto.
	//
	// 🔴 **L'origine non e' sempre la cella dove sta l'unita' adesso.** La fase Dash precede il Blast, quindi
	// chi ha pianificato una carica sparera' da dov'e' arrivato: la cella la deriva
	// `URTHexCombatLibrary::BlastOriginCell`, che e' la stessa risposta che usera' il resolver.
	//
	// ⚠️ **Il tratteggio non e' decorazione**: distingue un'origine CONFERMATA (cella corrente) da una
	// PREVISTA (cella dello scatto, che la collisione simultanea di CP 4.8 puo' accorciare). E' un canale
	// non cromatico, come chiede la grammatica della certezza: chi non distingue i colori legge comunque
	// la differenza fra continuo e spezzato.
	if (bPreviewAttackValid)
	{
		DrawMeaning(PreviewAttackOrigin, ERTOverlayMeaning::AttackOriginAim);

		const FVector A = URTHexLibrary::AxialToWorld(PreviewAttackOrigin, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(PreviewAttackOrigin) + RTLiftPreview + 3.f);
		const FVector B = URTHexLibrary::AxialToWorld(PreviewAttackAim, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(PreviewAttackAim) + RTLiftPreview + 3.f);
		const FColor AimColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::AttackOriginAim);
		if (bPreviewOriginPredicted)
		{
			// Spezzata: otto tratti, quattro disegnati. Il numero e' fisso e non dipende dalla distanza —
			// un passo costante allungherebbe il ciclo su una linea lunga senza aggiungere informazione.
			constexpr int32 Segments = 8;
			for (int32 S = 0; S < Segments; S += 2)
			{
				const FVector P0 = FMath::Lerp(A, B, S / static_cast<float>(Segments));
				const FVector P1 = FMath::Lerp(A, B, (S + 1) / static_cast<float>(Segments));
				DisegnaLineaAnteprima(World, P0, P1, AimColor, SDPG_Foreground, /*Spessore=*/ 3.f);
			}
		}
		else
		{
			DisegnaLineaAnteprima(World, A, B, AimColor, SDPG_Foreground, /*Spessore=*/ 3.f);
		}
	}

	// ── LA LINEA CHE NON PASSA, e dove si ferma (`#2742`).
	//
	// 🔴 **Mutuamente esclusiva con la mira qui sopra, per costruzione e non per una guardia.** Quella si
	// disegna quando il piano ESISTE (`OnSelect`, ramo `bReady && Reason == Ok`); questa quando il piano
	// non e' nato perche' la traiettoria e' interrotta. I due stati non possono essere accesi insieme, e
	// per questo non c'e' un `else`: non ci sarebbe niente da mettere in `else`.
	//
	// ⚠️ **Troncata, non intera.** `BlockedAt` e' dove il raggio si e' fermato: disegnarla fino al
	// bersaglio direbbe che la traiettoria arriva, che e' l'opposto dell'informazione. Il segmento finisce
	// dove l'ostacolo comincia, e chi guarda legge la distanza che gli resta.
	//
	// ⛔ Nessun `FColor` letterale: il colore e' `Vision` nella palette di [D-368], come il D010 chiede.
	if (bHasPreviewSightBlock)
	{
		const FVector From = URTHexLibrary::AxialToWorld(PreviewSightFrom, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(PreviewSightFrom) + RTLiftPreview + 3.f);
		const FVector Stop = URTHexLibrary::AxialToWorld(PreviewSightBlockedAt, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(PreviewSightBlockedAt) + RTLiftPreview + 3.f);

		const FColor SightColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Vision);
		DisegnaLineaAnteprima(World, From, Stop, SightColor, SDPG_Foreground, /*Spessore=*/ 3.f);

		// L'ostacolo si marca sulla cella che ha fermato il raggio: la linea dice DOVE si e' fermata, il
		// contorno dice SU COSA. E' la stessa coppia — segmento piu' cella — che `#2697` usa per gli
		// ostacoli gia' incontrati in risoluzione.
		DrawCellOutline(PreviewSightBlockedAt, SightColor,
			URTOverlayPalette::ScaleFor(ERTOverlayMeaning::Vision),
			URTOverlayPalette::DrawsThroughUnits(ERTOverlayMeaning::Vision));
	}

	// Zona colpita dall'attacco pianificato. Rosso = minaccia; ARANCIONE = c'e' un alleato dentro, e va visto
	// PRIMA del lock-in: che il fuoco amico faccia danno e' gia' verificato dai test, che il giocatore lo sappia
	// mentre puo' ancora cambiare idea no — quella meta' esiste solo qui.
	for (const FRTCellId& Cell : PreviewHitArea.Cells)
	{
		const bool bAlly = PreviewAllyHitArea.Cells.Contains(Cell);
		DrawMeaning(Cell, bAlly ? ERTOverlayMeaning::FriendlyFire : ERTOverlayMeaning::Attack);
	}

	// Impronta dei colpi gia' RISOLTI, durante il playback (`#2454`).
	//
	// 🔑 **Stesso significato dell'area in anteprima, quindi stesso colore**: `ERTOverlayMeaning::Attack`
	// da `URTOverlayPalette`. ⛔ Nessun `FColor` letterale nuovo — un sesto letterale in questa funzione e'
	// precisamente il difetto che `#1941` esiste per chiudere, e che `RTHexLosConsole.cpp` ha gia' rifiutato
	// per iscritto.
	//
	// ⚠️ Vive nella stessa funzione dell'anteprima e non in una gemella: `DrawMeaning` e' una lambda locale,
	// ed estrarla sarebbe un refactor che questa issue non ha ragione di fare. Cio' che resta separato e' lo
	// **stato** — array proprio, spento da `FinishPlayback` invece che dal lock-in.
	for (const FRTCellId& Cell : PlaybackFootprintCells)
	{
		DrawMeaning(Cell, ERTOverlayMeaning::Attack);
	}

	// Colpi alle STRUTTURE gia' risolti, durante il playback (`#2828`).
	//
	// 🔑 **Un SEGMENTO sul bordo, non un esagono**: il soggetto e' un lato, e colorare le due facce
	// direbbe che sono state colpite loro. E' la stessa distinzione — dove finisce il fatto, su cosa cade —
	// che il blocco della linea di tiro qui sopra fa con segmento piu' contorno.
	//
	// ⛔ **Nessun `FColor` letterale e nessun ottavo `ERTOverlayMeaning`**: un colpo a un muro significa
	// quel che significa un colpo, quindi `Attack`, come l'impronta appena sopra. Un significato nuovo per
	// una variante di resa e' precisamente il difetto che `#1941` esiste per chiudere.
	//
	// ⚠️ **La distinzione caduta/danneggiata sta nello SPESSORE, ed e' graybox di proposito**: il VFX
	// vero — detriti, crollo — e' `#1848`, v0.2. Qui serve che il CAMBIAMENTO si veda, che e' cio' che
	// `#2453` isola come mancante nella v0.1: oggi si vede lo stato dopo, non il cambiamento.
	if (PlaybackStructureHits.Num() > 0)
	{
		const FColor StructureColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack);
		for (const FRTPlaybackStructureHit& Colpo : PlaybackStructureHits)
		{
			// ⚠️ **L'alzata e' quella della cella che PORTA la copertura, per entrambi gli estremi.**
			// Prenderla da ciascuna delle due inclinerebbe il segno quando le celle hanno altezze diverse, e
			// lo farebbe sprofondare del tutto sul bordo esterno dell'arena, dove `Toward` non e' nella mappa
			// e `CellLift` risponde `0`. Il muro appartiene alla faccia che lo dichiara: e' la sua quota.
			const float Alzata = CellLift(Colpo.Cell) + RTLiftPreview + 3.f;
			const FVector CentroA = URTHexLibrary::AxialToWorld(Colpo.Cell, Origin, Size, LayerH)
				+ FVector(0, 0, Alzata);
			const FVector CentroB = URTHexLibrary::AxialToWorld(Colpo.Toward, Origin, Size, LayerH)
				+ FVector(0, 0, Alzata);

			// 🔴 **PERPENDICOLARE all'asse fra i due centri, non lungo di esso.** Il bordo condiviso e' il
			// lato che i due esagoni hanno in comune: sta a meta' strada e giace di traverso. Un segmento
			// tracciato SULL'asse — anche corto, anche centrato — punta da una cella verso l'altra e si legge
			// come una traiettoria, che e' un'altra frase.
			// ⏱️ *La prima stesura faceva esattamente quello, e il commento accanto dichiarava di evitarlo.*
			//
			// La mezza lunghezza e' `d / (2√3)` perche' in una griglia esagonale il lato vale `d/√3`, con `d`
			// la distanza fra centri adiacenti: cosi' il segno copre il lato intero e non lo sborda.
			const FVector Asse = CentroB - CentroA;
			const FVector Meta = (CentroA + CentroB) * 0.5f;
			const FVector MezzoLato = FVector(-Asse.Y, Asse.X, 0.f) * (0.5f / FMath::Sqrt(3.f));
			DisegnaLineaAnteprima(World, Meta - MezzoLato, Meta + MezzoLato, StructureColor, SDPG_Foreground,
				Colpo.bDestroyed ? 6.f : 3.f);
		}
	}

	// Tracer degli attacchi base IN VOLO, durante il playback (`#2454`).
	//
	// 🔑 **Stesso significato di un colpo, quindi stesso colore**: `ERTOverlayMeaning::Attack`, come l'impronta e i
	// muri qui sopra. ⛔ Nessun `FColor` letterale e nessun significato nuovo (`#1941`): proiettile e getto si
	// separano per GEOMETRIA, che e' `URTPlaybackLibrary::TracerSegment`.
	// ⚠️ **Foreground**: il tracer attraversa le unita' come la linea di mira, o sparirebbe dentro chi spara.
	if (PlaybackTracers.Num() > 0)
	{
		const FColor TracerColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack);
		for (const FRTPlaybackTracer& T : PlaybackTracers)
		{
			const FVector Da = URTHexLibrary::AxialToWorld(T.From, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.From) + RTTracerHeight);
			const FVector A = URTHexLibrary::AxialToWorld(T.To, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.To) + RTTracerHeight);
			if (T.Style == ERTTracerStyle::Zigzag)
			{
				// #3578: lo zigzag e' una polilinea pura (`TracerPolyline`), un prefisso che cresce.
				TArray<FVector> Punti;
				URTPlaybackLibrary::TracerPolyline(T.Style, Da, A, T.Alpha, Size, Punti);
				for (int32 P = 1; P < Punti.Num(); ++P)
				{
					DisegnaLineaAnteprima(World, Punti[P - 1], Punti[P], TracerColor, SDPG_Foreground, RTTracerZigzagThickness);
				}
				continue;
			}
			FVector Inizio, Fine;
			URTPlaybackLibrary::TracerSegment(T.Style, Da, A, T.Alpha, Size * RTTracerDashFraction, Inizio, Fine);
			DisegnaLineaAnteprima(World, Inizio, Fine, TracerColor, SDPG_Foreground,
				T.Style == ERTTracerStyle::Jet ? RTTracerJetThickness : RTTracerProjectileThickness);
		}
	}

	// Le cue del profilo FX, durante il playback (#3578, spec «il profilo FX» §2.1, §2.4).
	//
	// 🔑 **Stesso colore del colpo** (R1): `ERTOverlayMeaning::Attack`, nessun significato nuovo (#1941). Gli stili si
	// separano per GEOMETRIA (`CueSegments`), mai per colore. ⚠️ Foreground, come il tracer: un anello sotto chi agisce
	// sparirebbe dentro la sua mesh. ⛔ Nessun `DrawDebug*` (D-467): `DisegnaLineaAnteprima` tace sul server dedicato.
	if (PlaybackCues.Num() > 0)
	{
		const FColor CueColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack);
		for (const FRTPlaybackCue& C : PlaybackCues)
		{
			const FVector Ancora = URTHexLibrary::AxialToWorld(C.At, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(C.At) + RTCueLift);
			const FVector Verso = URTHexLibrary::AxialToWorld(C.Toward, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(C.Toward) + RTCueLift);
			TArray<FVector> Inizi, Fini;
			URTPlaybackLibrary::CueSegments(C.Kind, Ancora, Verso, Size, C.Alpha, Inizi, Fini);
			const float Spessore = (C.Kind == ERTPlaybackCueKind::Flash || C.Kind == ERTPlaybackCueKind::ConeSweep)
				? RTCueThickThickness : RTCueThickness;
			for (int32 I = 0; I < Inizi.Num(); ++I)
			{
				DisegnaLineaAnteprima(World, Inizi[I], Fini[I], CueColor, SDPG_Foreground, Spessore);
			}
		}
	}

	// Cella sotto il cursore: disegnata per ultima e piu' larga, cosi' resta leggibile sopra la traccia.
	// Anche questa attraversa le unita': si punta un bersaglio molto piu' spesso di una cella vuota, e
	// l'evidenziazione che sparisce proprio quando indichi qualcuno e' peggio che non averla.
	if (bHoveredValid)
	{
		DrawMeaning(HoveredCell, ERTOverlayMeaning::Hover);
	}
}

const URTHexMapAsset* ARTHexMapActor::GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	// Dimensioni dall'asset AUTOREVOLE; se manca valgono quelle dell'actor (graybox demo).
	const URTHexMapAsset* Map = MapAsset;
	OutOrigin = GetActorLocation();
	OutHexSize = Map ? Map->HexSize : HexSize;
	OutLayerHeight = GetPosedLayerHeight(); // #3632: la quota della board POSATA, separata in vista strategica
	return Map;
}

const URTHexMapAsset* ARTHexMapActor::GetAuthoringHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	const URTHexMapAsset* Map = MapAsset;
	OutOrigin = GetActorLocation();
	OutHexSize = Map ? Map->HexSize : HexSize;
	OutLayerHeight = Map ? Map->LayerHeight : LayerHeight;
	return Map;
}

float ARTHexMapActor::GetPresentationSeparation() const
{
	if (!bStrategicPresentation)
	{
		return 1.f;
	}
	const float DaConsole = CVarRTMapStrategicLayerSeparation.GetValueOnGameThread();
	const float Fattore = DaConsole > 0.f ? DaConsole : StrategicLayerSeparation;
	return FMath::Clamp(Fattore, 1.f, 10.f);
}

float ARTHexMapActor::GetPresentationLayerHeight() const
{
	return (MapAsset ? MapAsset->LayerHeight : LayerHeight) * GetPresentationSeparation();
}

float ARTHexMapActor::GetPosedLayerHeight() const
{
	// La BASE e' quella corrente — asset, o actor se l'asset manca — come prima di #3632: si conserva solo il
	// fattore con cui la board e' stata posata, cioe' la scelta di presentazione, non una quota che invecchia.
	const float Fattore = AppliedLayerSeparation > 0.f ? AppliedLayerSeparation : GetPresentationSeparation();
	return (MapAsset ? MapAsset->LayerHeight : LayerHeight) * Fattore;
}

bool ARTHexMapActor::SyncStrategicPresentation(bool bStrategic, float* OutPreviousLayerHeight)
{
	bStrategicPresentation = bStrategic;
	if (AppliedLayerSeparation < 0.f)
	{
		// Mai posate: la prima ricostruzione la fara' chi allestisce la board, alla quota che la vista chiede.
		return false;
	}

	// Chi posa le unita' le ha posate alla quota dell'ultimo giro; prima del primo, a quella della board.
	const float Precedente = LastSyncedLayerHeight > 0.f ? LastSyncedLayerHeight : GetPosedLayerHeight();

	if (!FMath::IsNearlyEqual(GetPresentationSeparation(), AppliedLayerSeparation))
	{
		// 🔴 **Il velo che la board MOSTRA, letto prima di ricostruirla.** La ricostruzione riporta ogni istanza
		// alla scala piena, e la prima scrittura successiva e' uno snap: si riscrive nello stesso fotogramma
		// lo stato che il velo stesso aveva scritto sul disco, cella per cella, e nessuna cella mai vista
		// compare nemmeno per un frame.
		//
		// ⛔ **Non dalla conoscenza di squadra**, che questo actor non conserva: e' la regola di `D-242` che
		// `RebuildInstances` dichiara per `KnowledgeVolumes`. Qui si rilegge cio' che e' gia' a schermo — due
		// insiemi di celle, nessun contatto e nessun intento — e lo si ridisegna.
		FRTTeamKnowledge Disegnata;
		bool bVelata = false;
		for (int32 I = 0; I < InstanceCells.Num() && I < LastVeilState.Num(); ++I)
		{
			const uint8 Stato = LastVeilState[I];
			if (Stato == RTVeilUnwritten)
			{
				continue;
			}
			bVelata = true;
			if (Stato == RTVeilLit)
			{
				Disegnata.VisibleCells.Add(InstanceCells[I]);
			}
			if (Stato == RTVeilLit || Stato == RTVeilRemembered)
			{
				Disegnata.ExploredCells.Add(InstanceCells[I]);
			}
		}

		RebuildInstances(ERTRebuildFamily::All);
		// Una board mai velata (editor, prove senza percezione) resta com'e': un velo vuoto la nasconderebbe tutta.
		if (bVelata)
		{
			ApplyKnowledgeVeil(Disegnata);
		}
		SetPlanPreview(LastPlanPreview);
	}

	const float Posata = GetPosedLayerHeight();
	LastSyncedLayerHeight = Posata;
	if (FMath::IsNearlyEqual(Precedente, Posata))
	{
		return false;
	}
	if (OutPreviousLayerHeight)
	{
		*OutPreviousLayerHeight = Precedente;
	}
	return true;
}

bool ARTHexMapActor::SyncActiveLayerPresentation(int32 InActiveLayer)
{
	const FRTLayerPresentationTuning Leve = FRTLayerPresentationTuning::FromConsole();
	if (bLayerPresentationApplied && PresentedActiveLayer == InActiveLayer && AppliedLayerTuning == Leve)
	{
		return false; // il caso di ogni fotogramma: niente da riscrivere
	}
	PresentedActiveLayer = InActiveLayer;
	AppliedLayerTuning = Leve;
	bLayerPresentationApplied = true;
	ApplyLayerPresentation();
	return true;
}

void ARTHexMapActor::ApplyLayerPresentation()
{
	// «A un piano» si misura sulle istanze disegnate, non sull'asset: con `ActiveOnly` l'editor ne disegna uno solo, e
	// trattarlo rispetto a piani che non si vedono non avrebbe senso.
	int32 MinLayer = TNumericLimits<int32>::Max();
	int32 MaxLayer = TNumericLimits<int32>::Lowest();
	for (const FRTCellId& Cell : InstanceCells)
	{
		MinLayer = FMath::Min(MinLayer, Cell.Layer);
		MaxLayer = FMath::Max(MaxLayer, Cell.Layer);
	}
	const bool bMultiLayer = InstanceCells.Num() > 0 && MaxLayer > MinLayer;
	bPresentationMultiLayer = bMultiLayer; // lo leggono il velo e i contorni dei fantasmi, via `BorderBaseColorFor`

	// Una famiglia: per ogni istanza, il trattamento del piano della sua cella.
	auto Scrivi = [this, bMultiLayer](UInstancedStaticMeshComponent* Famiglia, const TArray<FRTCellId>& CelleDelleIstanze)
	{
		if (Famiglia == nullptr || Famiglia->NumCustomDataFloats < RTLayerPresentation::NumCustomDataFloats
			|| Famiglia->GetInstanceCount() != CelleDelleIstanze.Num())
		{
			return; // senza i canali, o con una mappatura stantia: il velo ha la stessa guardia
		}
		const int32 Primo = RTLayerPresentation::FirstChannel;
		for (int32 I = 0; I < CelleDelleIstanze.Num(); ++I)
		{
			const FRTLayerTreatment T = RTLayerPresentation::Treatment(CelleDelleIstanze[I].Layer, PresentedActiveLayer,
				bMultiLayer, AppliedLayerTuning);
			Famiglia->SetCustomDataValue(I, Primo + 0, T.Transparency, /*bMarkRenderStateDirty=*/ false);
			Famiglia->SetCustomDataValue(I, Primo + 1, T.Desaturation, /*bMarkRenderStateDirty=*/ false);
			Famiglia->SetCustomDataValue(I, Primo + 2, T.ExposureStops, /*bMarkRenderStateDirty=*/ false);
		}
		// Una volta sola per famiglia, come il velo: marcarlo per canale ricostruirebbe il buffer 3N volte.
		if (CelleDelleIstanze.Num() > 0)
		{
			Famiglia->MarkRenderStateDirty();
		}
	};

	Scrivi(Cells, InstanceCells);
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		Scrivi(SurfaceGlyphs[Ring], GlyphCells[Ring]); // i glifi seguono la cella (D-508)
	}

	// #3706: i VOLUMI dicono il piano con la tinta ([D-508]) e ricevono il trattamento di D-501. La tinta sta nei canali
	// 0..2 e la sua forza nel 6; i canali 3..5 sono quelli delle celle, quindi il trattamento lo scrive `Scrivi`.
	// ⛔ Il velo non li tocca: su queste famiglie nasconde con la scala e non scrive colore.
	for (const FRTVolumeFamily& Volume : GetVolumeFamilies())
	{
		UInstancedStaticMeshComponent* Famiglia = Volume.Component;
		if (Famiglia == nullptr || Famiglia->NumCustomDataFloats < RTLayerPresentation::VolumeNumCustomDataFloats
			|| Famiglia->GetInstanceCount() != Volume.Cells->Num())
		{
			continue; // senza i canali, o con una mappatura stantia: la stessa guardia di `Scrivi`
		}
		for (int32 I = 0; I < Volume.Cells->Num(); ++I)
		{
			const FRTLayerVolumeTint Tinta = RTLayerPresentation::VolumeTint((*Volume.Cells)[I].Layer, PresentedActiveLayer,
				bMultiLayer, AppliedLayerTuning);
			Famiglia->SetCustomDataValue(I, 0, Tinta.Color.R, /*bMarkRenderStateDirty=*/ false);
			Famiglia->SetCustomDataValue(I, 1, Tinta.Color.G, /*bMarkRenderStateDirty=*/ false);
			Famiglia->SetCustomDataValue(I, 2, Tinta.Color.B, /*bMarkRenderStateDirty=*/ false);
			Famiglia->SetCustomDataValue(I, RTLayerPresentation::VolumeTintStrengthChannel, Tinta.Strength,
				/*bMarkRenderStateDirty=*/ false);
		}
		Scrivi(Famiglia, *Volume.Cells); // i canali 3..5, e marca lo stato di render una volta sola
	}

	// 🔑 **I bordi dicono il piano col COLORE, e non ricevono il trattamento** ([D-508], #3704). I canali 3..5 restano a
	// zero: un bordo sopra e' opaco, cioe' un contorno bianco netto attorno a un riempimento traslucido, e un bordo sotto
	// tiene il suo viola invece di diventare grigio.
	//
	// ⚠️ **Il colore si riscrive col fattore del velo CORRENTE**, letto dagli array che il velo stesso mantiene: un bordo
	// ricordato resta attenuato, e uno che sta sfumando prosegue dal punto in cui e' (`AdvanceVeilFamily` lo porta avanti
	// con lo stesso `BorderBaseColorFor`). Un bordo mai visto non si tocca: e' a scala zero, e il velo lo riscrive quando
	// lo rivela.
	if (CellBorders && CellBorders->NumCustomDataFloats >= RTLayerPresentation::NumCustomDataFloats
		&& CellBorders->GetInstanceCount() == BorderCells.Num())
	{
		const bool bVelata = LastBorderVeilState.Num() == BorderCells.Num();
		for (int32 I = 0; I < BorderCells.Num(); ++I)
		{
			float Fattore = 1.f; // board mai velata: editor, prove senza percezione
			if (bVelata)
			{
				const uint8 Stato = LastBorderVeilState[I];
				if (Stato == RTVeilHidden)
				{
					continue;
				}
				if (Stato == RTVeilLit || Stato == RTVeilRemembered)
				{
					const float Target = (Stato == RTVeilLit) ? 1.f : RTVeilExploredFactor;
					Fattore = (BorderDisplayFactor.IsValidIndex(I) && BorderDisplayFactor[I] >= 0.f) ? BorderDisplayFactor[I] : Target;
				}
			}
			const FLinearColor Colore = BorderBaseColorFor(BorderCells[I]) * Fattore;
			CellBorders->SetCustomDataValue(I, 0, Colore.R, /*bMarkRenderStateDirty=*/ false);
			CellBorders->SetCustomDataValue(I, 1, Colore.G, /*bMarkRenderStateDirty=*/ false);
			CellBorders->SetCustomDataValue(I, 2, Colore.B, /*bMarkRenderStateDirty=*/ false);
			for (int32 C = RTLayerPresentation::FirstChannel; C < RTLayerPresentation::NumCustomDataFloats; ++C)
			{
				CellBorders->SetCustomDataValue(I, C, 0.f, /*bMarkRenderStateDirty=*/ false);
			}
		}
		if (BorderCells.Num() > 0)
		{
			CellBorders->MarkRenderStateDirty();
		}
	}

	ApplyPlanGhostBorderColors();
}

TArray<ARTHexMapActor::FRTVolumeFamily> ARTHexMapActor::GetVolumeFamilies() const
{
	// ⛔ `Relief` non c'e', ed e' voluto: vedi `VolumeMaterial`.
	TArray<FRTVolumeFamily> Famiglie;
	Famiglie.Add({ EdgeFeatures, &EdgeFeatureCells, TEXT("EdgeFeatures") });
	Famiglie.Add({ Blockers, &BlockerCells, TEXT("Blockers") });
	Famiglie.Add({ StructuralBodies, &BodyCells, TEXT("StructuralBodies") });
	Famiglie.Add({ SurfaceVolumes, &SurfaceVolumeCells, TEXT("SurfaceVolumes") });
	return Famiglie;
}

void ARTHexMapActor::ApplyVolumeMaterial()
{
	UMaterialInterface* Mat = VolumeMaterial.LoadSynchronous();
	if (Mat == nullptr)
	{
		return; // opzionale come `CellMaterial`: senza, i volumi tengono il materiale della mesh
	}
	// I pannelli di bordo hanno il loro colore proprio: una istanza dinamica, creata una volta sola.
	if (EdgePanelVolumeMaterial == nullptr || EdgePanelVolumeMaterial->Parent != Mat)
	{
		EdgePanelVolumeMaterial = UMaterialInstanceDynamic::Create(Mat, this);
		if (EdgePanelVolumeMaterial != nullptr)
		{
			EdgePanelVolumeMaterial->SetVectorParameterValue(RTLayerPresentation::VolumeParamOwnColor,
				RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ true));
		}
	}
	for (const FRTVolumeFamily& Volume : GetVolumeFamilies())
	{
		UMaterialInterface* Voluto = (Volume.Component == EdgeFeatures && EdgePanelVolumeMaterial != nullptr)
			? static_cast<UMaterialInterface*>(EdgePanelVolumeMaterial) : Mat;
		if (Volume.Component != nullptr && Volume.Component->GetMaterial(0) != Voluto)
		{
			Volume.Component->SetMaterial(0, Voluto);
		}
	}
}

FLinearColor ARTHexMapActor::BorderBaseColorFor(const FRTCellId& Cell) const
{
	// Prima che chi guarda dica il suo piano, e su una mappa a un piano, il bordo e' quello di sempre.
	const ERTLayerRelation Relazione = bLayerPresentationApplied
		? RTLayerPresentation::Relation(Cell.Layer, PresentedActiveLayer, bPresentationMultiLayer)
		: ERTLayerRelation::Active;
	return FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(Relazione));
}

void ARTHexMapActor::ApplyPlanGhostBorderColors()
{
	if (!PlanGhostBorders)
	{
		return;
	}
	// Su una mappa a un piano, e prima che chi guarda dica il suo piano, i fantasmi si vedono come prima di #3704: senza
	// contorno. Il contorno serve a dire il piano, e su un piano solo non c'e' niente da dire.
	PlanGhostBorders->SetVisibility(bLayerPresentationApplied && bPresentationMultiLayer);
	if (PlanGhostBorders->GetInstanceCount() != PlanGhostCells.Num())
	{
		return;
	}
	for (int32 I = 0; I < PlanGhostCells.Num(); ++I)
	{
		const FLinearColor Colore = BorderBaseColorFor(PlanGhostCells[I]);
		PlanGhostBorders->SetCustomDataValue(I, 0, Colore.R, /*bMarkRenderStateDirty=*/ false);
		PlanGhostBorders->SetCustomDataValue(I, 1, Colore.G, /*bMarkRenderStateDirty=*/ false);
		PlanGhostBorders->SetCustomDataValue(I, 2, Colore.B, /*bMarkRenderStateDirty=*/ false);
	}
	if (PlanGhostCells.Num() > 0)
	{
		PlanGhostBorders->MarkRenderStateDirty();
	}
}

bool ARTHexMapActor::PassesLayerFilter(int32 Layer) const
{
	// Filtro layer (H4): solo AllLayers impila i piani; ActiveOnly e Focus tengono le ISTANZE (e le terne
	// incise) sul solo layer attivo. La differenza fra ActiveOnly e Focus e' di sola presentazione — Focus
	// disegna gli altri piani a contorno (`RTHexEditor::DrawSurfaceOverlay`) — e sta fuori di qui apposta: i
	// piani di contesto non devono diventare istanze, o tornerebbero ad avere collisione e a intercettare il
	// click del pennello.
	return LayerView == ERTLayerViewMode::AllLayers || Layer == ActiveLayer;
}

void ARTHexMapActor::RebuildInstances(ERTRebuildFamily Families)
{
	if (!Cells)
	{
		return;
	}

	// Cio' che non e' nella maschera non viene ne' pulito ne' riempito: i suoi array paralleli restano
	// coerenti perche' nessuno li tocca. E' la ragione per cui la granularita' e' la famiglia (`#1865`).
	const bool bDoCells   = EnumHasAnyFlags(Families, ERTRebuildFamily::Cells);
	const bool bDoGlyphs  = EnumHasAnyFlags(Families, ERTRebuildFamily::Glyphs);
	const bool bDoRelief  = EnumHasAnyFlags(Families, ERTRebuildFamily::Relief);
	const bool bDoBlock   = EnumHasAnyFlags(Families, ERTRebuildFamily::Blockers);
	const bool bDoEdges   = EnumHasAnyFlags(Families, ERTRebuildFamily::EdgeFeatures);
	const bool bDoBorders = EnumHasAnyFlags(Families, ERTRebuildFamily::Borders);
	const bool bDoBodies  = EnumHasAnyFlags(Families, ERTRebuildFamily::Bodies);
	const bool bDoSurfaceVolumes = EnumHasAnyFlags(Families, ERTRebuildFamily::SurfaceVolumes);

	// Mesh configurabile con fallback. Il fallback e' il prisma esagonale generato, NON piu' il cilindro
	// engine: quello restava un disco, ed e' il difetto che `U22` ha visto a schermo. `CellMesh` continua a
	// vincere se qualcuno l'ha assegnata — la configurabilita' non si perde, cambia solo cosa succede quando
	// nessuno configura niente, che e' il caso di ogni livello esistente.
	UStaticMesh* CellShape = CellMesh.LoadSynchronous();
	if (CellShape == nullptr)
	{
		CellShape = GetCellPrismMesh();
	}
	if (CellShape != nullptr)
	{
		Cells->SetStaticMesh(CellShape);
	}

	// Dopo `SetStaticMesh`, che riporta gli slot ai materiali della mesh: invertire l'ordine perderebbe
	// l'override senza dirlo. Solo `Cells` — gli altri tre ISM non portano custom data, e tingerli col
	// colore della superficie direbbe una cosa falsa (il rilievo non e' terreno, e' il costo).
	//
	// ⚠️ Conseguenza VISTA a schermo e accettata il 2026-08-15, non un effetto collaterale non notato:
	// in Unlit il materiale di default rende i blocchi di `Relief` **la cosa piu' luminosa della scena**,
	// piu' delle celle colorate che stanno sotto. Le alternative erano tingerli come la cella (piu' scuri)
	// o dargli un grigio neutro; si e' scelto di lasciarli cosi', perche' il sovrapprezzo di movimento
	// DEVE saltare all'occhio. Se un giorno sembrera' un difetto, e' una decisione di leggibilita' e si
	// riapre in M8 / U9 — non si "corregge" qui.
	if (UMaterialInterface* Mat = CellMaterial.LoadSynchronous())
	{
		Cells->SetMaterial(0, Mat);
	}

	// ⚠️ **Ogni azzeramento sta con la sua famiglia**, e non piu' tutti insieme: una famiglia fuori dalla
	// maschera conserva istanze E array paralleli, che restano allineati fra loro perche' nessuno li tocca.
	// Separarli e' cio' che rende sicura la ricostruzione parziale.
	if (bDoCells)
	{
		Cells->ClearInstances();
		InstanceCells.Reset();
		InstanceBaseScale.Reset();
		// Lo stato del velo si azzera con gli indici a cui si riferisce: sopravvivergli significherebbe saltare
		// istanze che nel frattempo sono diventate altre celle.
		LastVeilState.Reset();
		// Il fattore disegnato segue lo stato, per la stessa ragione: e' parallelo agli stessi indici, e una
		// transizione sopravvissuta a un rebuild attenuerebbe una cella che nel frattempo e' un'altra.
		VeilDisplayFactor.Reset();
		VeilCellsInTransition = 0;
	}
	if (bDoGlyphs)
	{
		for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
		{
			GlyphCells[Ring].Reset();
			GlyphBaseScale[Ring].Reset();
			LastGlyphVeilState[Ring].Reset();
			GlyphDisplayFactor[Ring].Reset();
		}
		VeilCellsInTransition = 0;
	}
	if (bDoRelief)
	{
		ReliefCells.Reset();
		ReliefBaseScale.Reset();
		LastReliefVeilState.Reset();
	}
	if (bDoSurfaceVolumes && SurfaceVolumes)
	{
		SurfaceVolumes->ClearInstances();
		SurfaceVolumeCells.Reset();
		SurfaceVolumeBaseScale.Reset();
		// Lo stato del velo si azzera con gli indici a cui si riferisce, come per ogni altra famiglia.
		LastSurfaceVolumeVeilState.Reset();
	}
	if (bDoBlock)
	{
		BlockerCells.Reset();
		BlockerBaseScale.Reset();
		LastBlockerVeilState.Reset();
	}
	if (bDoEdges)
	{
		EdgeFeatureCells.Reset();
		EdgeFeatureOtherCells.Reset();
		EdgeFeatureBaseScale.Reset();
		LastEdgeFeatureVeilState.Reset();
		// `#3687`: cio' che questa ricostruzione disegna, per sapere a fine turno se e' ancora vero.
		CollectEdgeLayout(EdgeLayoutAtLastBuild);
	}
	if (bDoBorders)
	{
		BorderCells.Reset();
		BorderBaseScale.Reset();
		LastBorderVeilState.Reset();
		BorderDisplayFactor.Reset();
		VeilCellsInTransition = 0;
	}

	// I glifi si ricostruiscono con le celle: `RebuildInstances` gira a ogni pennellata, e istanze vecchie
	// resterebbero appese a superfici che nel frattempo sono cambiate.
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		if (!bDoGlyphs || !SurfaceGlyphs[Ring]) { continue; }
		SurfaceGlyphs[Ring]->ClearInstances();
		if (UStaticMesh* GlyphMesh = GetCellGlyphMesh(Ring + 1))
		{
			SurfaceGlyphs[Ring]->SetStaticMesh(GlyphMesh);
		}
		// Dopo `SetStaticMesh`, che riporta gli slot ai materiali della mesh — stesso ordine di `Cells`.
		if (UMaterialInterface* GlyphMat = CellMaterial.LoadSynchronous())
		{
			SurfaceGlyphs[Ring]->SetMaterial(0, GlyphMat);
		}
	}

	// Celle isolate: si INVALIDA soltanto, il calcolo lo fa `GetUnreachableCells` alla prima richiesta.
	// RebuildInstances viene chiamata a ogni OnClickDrag del pennello — molte volte al secondo mentre si
	// trascina — e una BFS sull'intero grafo a ogni cella dipinta sarebbe lavoro sprecato per un dato che
	// serve solo a chi guarda l'overlay, e solo se e' acceso.
	bUnreachableDirty = true;
	// Rilievo e blocchi seguono la stessa forma delle celle: sono volumi annidati DENTRO l'esagono
	// (`RTVolumeRelief`, `RTVolumeBlocker`), e un cilindro dentro un prisma si vedrebbe sporgere agli spigoli.
	if (bDoRelief && Relief)
	{
		if (CellShape != nullptr) { Relief->SetStaticMesh(CellShape); }
		Relief->ClearInstances();
	}
	if (bDoSurfaceVolumes && SurfaceVolumes)
	{
		if (CellShape != nullptr) { SurfaceVolumes->SetStaticMesh(CellShape); }
	}
	if (bDoBlock && Blockers)
	{
		if (CellShape != nullptr) { Blockers->SetStaticMesh(CellShape); }
		Blockers->ClearInstances();
	}
	if (bDoBodies && StructuralBodies)
	{
		// La NONA famiglia. Sta nell'elenco perche' l'ottava — `KnowledgeVolumes` — ci e' finita solo dopo
		// aver lasciato prismi appesi su una board che non esisteva piu' (`#2222`): un componente che non si
		// azzera qui sopravvive alla mappa che descrive.
		if (CellShape != nullptr) { StructuralBodies->SetStaticMesh(CellShape); }
		StructuralBodies->ClearInstances();
		// La mappatura cella->istanza e lo stato del velo si azzerano con gli indici a cui si riferiscono,
		// come per le altre otto: sopravvivergli significherebbe velare istanze diventate altre celle.
		BodyCells.Reset();
		BodyBaseScale.Reset();
		LastBodyVeilState.Reset();
	}
	if (bDoEdges && EdgeFeatures)
	{
		// La mesh dei bordi NON segue `CellMesh`: quella e' la mesh della cella, e un pannello non e' una cella.
		EdgeFeatures->ClearInstances();
	}

	// La griglia: mesh propria e generata, come i glifi. NON segue `CellMesh` — quella e' il pieno della
	// cella, e un contorno non e' un pieno.
	if (bDoBorders && CellBorders)
	{
		CellBorders->ClearInstances();
		if (UStaticMesh* BorderMesh = GetCellBorderMesh())
		{
			CellBorders->SetStaticMesh(BorderMesh);
		}
		// Dopo `SetStaticMesh`, che riporta gli slot ai materiali della mesh — stesso ordine di `Cells` e dei
		// glifi. Invertirlo perderebbe l'override senza dirlo.
		if (UMaterialInterface* BorderMat = CellMaterial.LoadSynchronous())
		{
			CellBorders->SetMaterial(0, BorderMat);
		}
		// ⚠️ La visibilita' si RIAFFERMA a ogni ricostruzione: `RebuildInstances` gira a ogni pennellata e a
		// ogni `OnConstruction`, e un componente ricreato tornerebbe al default ignorando un toggle spento.
		CellBorders->SetVisibility(bCellBordersVisible);
	}

	// 🔴 **L'OTTAVA famiglia, ed era l'unica che sopravviveva a questa funzione** (`#2222`).
	//
	// `KnowledgeVolumes` e' la sola vista che NON deriva dall'asset: la posa `SetKnowledgeDebugEnabled` a
	// partire da una conoscenza che questo actor non conserva. Finche' non veniva azzerata qui, ricostruire
	// la board lasciava appesi i prismi della board PRECEDENTE, sopra celle che nel frattempo erano diventate
	// altre celle — e in PIE si vedeva come geometria che si accumula a ogni scenario.
	//
	// ⚠️ **Si spegne anche il flag, e non e' pignoleria.** `bKnowledgeDebug` rimasto `true` con il
	// componente vuoto farebbe divergere lo stato interno da cio' che si vede, **in silenzio**. E' la stessa
	// superficie che ha gia' prodotto una diagnosi di velo rotto su un difetto inesistente: il registro PIE
	// annota che `rt.Debug.Knowledge` *«posa una fotografia e non la aggiorna»*, e che i suoi sintomi sono
	// «indistinguibili da un velo rotto». Un secondo modo di mentire sulla stessa superficie non si aggiunge.
	//
	// ⛔ **Non si RICOSTRUISCONO, e la scelta e' dichiarata**: l'actor non ha la conoscenza da cui rifarli,
	// e conservarne una copia farebbe di un componente di presentazione una seconda sede della conoscenza di
	// squadra — cio' che [D-242] esiste per impedire. Si rilancia il comando, che e' gia' la sua semantica.
	// ⚠️ **Questo ramo NON porta un `#if`, ed e' una scelta** (`#2395`). `bKnowledgeDebug` era dichiarato
	// sotto `#if !UE_BUILD_SHIPPING` mentre **due** righe di questo blocco lo toccano fuori da quella
	// guardia — la condizione qui sotto lo **legge**, il corpo lo **scrive** (`bKnowledgeDebug = false`) —
	// e stanno nel percorso di ricostruzione condiviso: la Shipping non compilava (`C2065` su ENTRAMBE, e a
	// cascata `C2088` sull'`&&`, perche' con un operando inesistente l'espressione non si forma).
	// ⛔ **Due, non una**: un audit che cercasse le sole letture mancherebbe la scrittura, che e' la riga da
	// cui il compilatore ha riportato il primo errore.
	//
	// 🔑 **La guardia e' stata tolta dalla DICHIARAZIONE, non aggiunta qui.** Il confine di questo strumento
	// *«passa dall'API, non dal componente»* — lo dichiara l'intestazione di `KnowledgeVolumes` — e cio' che
	// in Shipping non esiste e' il modo di **accenderlo**, non il byte che ne ricorda lo stato. E' la stessa
	// forma di `LastRebuildCreated`: campo nudo, accessore guardato. Un `#if` qui avrebbe spezzato in due il
	// rendering per un `bool` sempre `false`, e — non ovvio — avrebbe tolto alla Shipping l'**unico** punto
	// che azzera `KnowledgeVolumes`, che e' un default subobject i cui dati d'istanza si serializzano col
	// livello: una `.umap` salvata con istanze non avrebbe piu' nulla che le pulisca.
	if (KnowledgeVolumes && (bKnowledgeDebug || KnowledgeVolumes->GetInstanceCount() > 0))
	{
		KnowledgeVolumes->ClearInstances();
		KnowledgeVolumes->SetVisibility(false);
		bKnowledgeDebug = false;
		// Una riga per ACCENSIONE, non per pennellata: `RebuildInstances` gira a ogni cella dipinta, ma dal
		// giro successivo la condizione qui sopra e' falsa e questo ramo non si ripete.
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Board ricostruita: i volumi di rt.Debug.Knowledge sono stati "
			"rimossi e il debug e' spento. Rilancia il comando per rileggere la conoscenza."));
	}

	// Sorgente celle: l'asset se popolato, altrimenti un graybox demo (esagono pieno di raggio DemoRadius).
	const float UseHexSize = MapAsset ? MapAsset->HexSize : HexSize;
	// #3632: la quota POSATA cambia solo con una ricostruzione intera. Una parziale — il velo che insegue una
	// revisione dell'asset — ripone le sue famiglie alla quota delle altre: scrivendo la quota nuova, lascerebbe
	// la board mista, e `SyncStrategicPresentation` la crederebbe gia' allineata.
	if (Families == ERTRebuildFamily::All || AppliedLayerSeparation <= 0.f)
	{
		AppliedLayerSeparation = GetPresentationSeparation();
	}
	const float UseLayerH = GetPosedLayerHeight();

	// Filtro layer (H4): `PassesLayerFilter` (sopra) e' l'unica regola, condivisa con
	// `RebuildCoordinateLabels` — non piu' una lambda locale, per non avere due formule dello stesso filtro.
	TArray<FRTCellId> CellIds;
	TArray<int32> Heights;
	TArray<int32> MoveCosts;
	TArray<bool> BlocksMove;
	TArray<bool> BlocksSight;
	// Puntatori alle celle d'asset per leggerne coperture e porte: sparsi, quindi la stragrande maggioranza
	// delle celle non produce nulla. Nel ramo demo restano nulli — un graybox non ha bordi d'autore.
	TArray<const FRTHexCellData*> EdgeSources;
	// Superficie per cella: alimenta il colore per istanza. Sta in un array parallelo come gli altri e non si
	// legge da `EdgeSources`, che nel ramo demo e' tutto nullo.
	TArray<ERTHexSurface> Surfaces;
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		CellIds.Reserve(MapAsset->NumCells());
		Heights.Reserve(MapAsset->NumCells());
		MoveCosts.Reserve(MapAsset->NumCells());
		BlocksMove.Reserve(MapAsset->NumCells());
		BlocksSight.Reserve(MapAsset->NumCells());
		for (const FRTHexCellData& C : MapAsset->Cells)
		{
			if (!PassesLayerFilter(C.Id.Layer))
			{
				continue;
			}
			CellIds.Add(C.Id);
			Heights.Add(C.Height);
			MoveCosts.Add(C.TotalMoveCost()); // il rilievo mostra il costo VERO: una cella stretta si alza
			BlocksMove.Add(C.bBlocksMovement);
			BlocksSight.Add(C.bBlocksLineOfSight);
			EdgeSources.Add(&C);
			Surfaces.Add(C.Surface);
		}
	}
	else if (DemoRadius > 0)
	{
		// Demo graybox sul layer attivo (visibile sia in AllLayers sia in ActiveOnly).
		CellIds = URTHexLibrary::HexArea(FRTCellId(0, 0, ActiveLayer), DemoRadius);
		Heights.Init(0, CellIds.Num());
		MoveCosts.Init(1, CellIds.Num()); // il graybox non ha terreni: tutto pavimento, quindi piatto
		BlocksMove.Init(false, CellIds.Num());
		BlocksSight.Init(false, CellIds.Num());
		EdgeSources.Init(nullptr, CellIds.Num());
		Surfaces.Init(ERTHexSurface::Floor, CellIds.Num()); // coerente con MoveCosts a 1: il graybox e' pavimento
	}

	// Cilindro engine: raggio 50 uu, mezza-altezza 50 uu. Scala X,Y per coprire ~l'esagono, Z sottile (disco).
	// Lo spessore vive in `RTCellFlatScale` perche' le quote di disegno delle debug-line ci si appoggiano.
	const float PlanarScale = UseHexSize / 50.f * 0.95f;
	const float FlatScale = RTCellFlatScale;

	for (int32 I = 0; I < CellIds.Num(); ++I)
	{
		FVector World = URTHexLibrary::AxialToWorld(CellIds[I], GetActorLocation(), UseHexSize, UseLayerH);
		World.Z += static_cast<double>(Heights[I]);
		const FTransform Xf(FRotator::ZeroRotator, World, FVector(PlanarScale, PlanarScale, FlatScale));
		// Il pavimento si posa solo se la sua famiglia e' richiesta: `InstanceIndex` e le custom data
		// vivono qui dentro, e senza la posa non avrebbero un'istanza a cui riferirsi.
		if (bDoCells)
		{
			const int32 InstanceIndex = Cells->AddInstance(Xf, /*bWorldSpace=*/ true);
			InstanceCells.Add(CellIds[I]);
			// La scala piena si registra QUI, dove la si conosce: il velo nasconde azzerandola, e da uno zero non
			// si torna indietro ([D-227]).
			InstanceBaseScale.Add(Xf.GetScale3D());

			// Colore della superficie, per ISTANZA. La tavolozza e' `URTHexLibrary::SurfaceColor` — la stessa che
			// disegna l'anello dell'overlay e il marker dell'editor: una cella non puo' avere due colori a seconda
			// di chi la guarda.
			// ⚠️ `SurfaceColor` restituisce un `FColor` sRGB a 8 bit, il materiale legge **lineare** — sul canale
			// **Emissive** di `M_HexCell`, non sul Base Color come questo commento ha dichiarato fino al 2026-08-23.
			// `FromSRGBColor` fa la conversione; dividere per 255 darebbe tinte slavate, e lo sbaglio si vedrebbe
			// solo mettendo la mesh accanto al proprio anello.
			const FLinearColor CellColor = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(Surfaces[I]));
			Cells->SetCustomDataValue(InstanceIndex, 0, CellColor.R);
			Cells->SetCustomDataValue(InstanceIndex, 1, CellColor.G);
			// Il render state si marca una volta sola, sull'ultima istanza: farlo a ogni canale ricostruirebbe il
			// buffer 3N volte, e `RebuildInstances` gira a ogni OnClickDrag del pennello.
			Cells->SetCustomDataValue(InstanceIndex, 2, CellColor.B,
				/*bMarkRenderStateDirty=*/ I == CellIds.Num() - 1);

			// ── La GRIGLIA (#1758): il canale che dice DOVE FINISCE la cella ───────────────────────────────
			//
			// Una per cella, sempre: a differenza del glifo — che cinque superfici su nove non ricevono — il
			// confine non dipende dal terreno. Una griglia che saltasse le celle senza segno lascerebbe buchi
			// proprio sul pavimento, che e' la superficie piu' diffusa della board.
		}

		if (bDoBorders && CellBorders)
		{
			// ⚠️ La scala e' `PlanarScale` e NON `UseHexSize / 50.f`, ed e' l'opposto della scelta fatta per
			// il glifo tre righe piu' sotto: quello porta il `0.95` DENTRO la propria mesh, questo lo vuole
			// FUORI perche' il suo anello esterno sta a `1.0`. Solo cosi' il bordo cade esattamente sul
			// perimetro del prisma invece che il 5% oltre — dove sconfinerebbe nella cella vicina.
			FVector BorderCenter = World;
			BorderCenter.Z += RTLiftCellBorder;
			const FTransform BorderXf(FRotator::ZeroRotator, BorderCenter,
				FVector(PlanarScale, PlanarScale, 1.f));
			const int32 BorderIndex = CellBorders->AddInstance(BorderXf, /*bWorldSpace=*/ true);
			// Indicizzazione propria, come per i glifi: oggi e' uno-a-uno con `InstanceCells`, e il giorno in
			// cui non lo fosse piu' il velo colpirebbe la cella sbagliata senza che nessun conteggio se ne accorga.
			BorderCells.Add(CellIds[I]);
			BorderBaseScale.Add(BorderXf.GetScale3D());

			// La stessa costante scura del glifo ([D-183]): il bordo appartiene al registro «segno inciso»,
			// non alla tavolozza delle superfici. Tingerlo col colore del terreno raddoppierebbe il canale
			// che esiste gia' invece di aggiungerne uno — ed e' esattamente il difetto che #1758 chiude.
			const FLinearColor BorderColor = FLinearColor::FromSRGBColor(URTHexLibrary::CellBorderColor());
			CellBorders->SetCustomDataValue(BorderIndex, 0, BorderColor.R);
			CellBorders->SetCustomDataValue(BorderIndex, 1, BorderColor.G);
			CellBorders->SetCustomDataValue(BorderIndex, 2, BorderColor.B,
				/*bMarkRenderStateDirty=*/ I == CellIds.Num() - 1);
		}

		// ── Il GLIFO di superficie (#956): il secondo canale ────────────────────────────────────────
		//
		// Cinque superfici su nove non ne ricevono uno, ed e' una scelta dichiarata: `SurfaceRingCount`
		// restituisce zero e qui non si monta niente.
		const int32 Rings = URTHexLibrary::SurfaceRingCount(Surfaces[I]);
		if (bDoGlyphs && Rings > 0 && Rings <= RTGlyphMaxRings && SurfaceGlyphs[Rings - 1])
		{
			// ➕ La posa viene da `GlyphTransformForCell`, la sede condivisa con il riallineamento per-cella
			// (`#2761`): due copie della stessa formula divergerebbero al primo che ne cambia una.
			const FTransform GlyphXf = GlyphTransformForCell(CellIds[I], Heights[I], UseHexSize, UseLayerH);
			const int32 GlyphIndex = SurfaceGlyphs[Rings - 1]->AddInstance(GlyphXf, /*bWorldSpace=*/ true);
			// Indicizzazione PROPRIA del componente: senza queste due righe il velo della corona colpirebbe
			// la cella sbagliata, e il conteggio delle velate tornerebbe comunque giusto.
			GlyphCells[Rings - 1].Add(CellIds[I]);
			GlyphBaseScale[Rings - 1].Add(GlyphXf.GetScale3D());

			// ⚠️ La conversione sRGB->lineare si RIPETE qui, non si eredita: e' lo stesso contratto di `Cells`
			// — il materiale legge lineare sul canale Emissive — e un secondo sito che se la dimenticasse
			// darebbe un grigio slavato che si nota solo mettendo il glifo accanto alla propria cella.
			//
			// La costante scura viene da `D-183`, che l'ha scelta misurando: contrasto **160 / 165** sul
			// quartetto e distanza **60 / 65** dalla faccia piu' vicina, mentre bianco e derivato-per-0,5
			// cadevano DENTRO la gamma delle superfici. E' un placeholder, e `PIE-V01-BOARD` e' il posto in
			// cui verra' rimessa in discussione.
			const FLinearColor GlyphColor = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
			SurfaceGlyphs[Rings - 1]->SetCustomDataValue(GlyphIndex, 0, GlyphColor.R);
			SurfaceGlyphs[Rings - 1]->SetCustomDataValue(GlyphIndex, 1, GlyphColor.G);
			SurfaceGlyphs[Rings - 1]->SetCustomDataValue(GlyphIndex, 2, GlyphColor.B,
				/*bMarkRenderStateDirty=*/ I == CellIds.Num() - 1);
		}

		// Rilievo del costo: un blocco alto quanto il SOVRAPPREZZO della cella. Il pavimento non ne produce
		// nessuno — una mappa senza terreni costosi resta piatta, ed e' giusto: non c'e' niente da segnalare.
		const float ReliefHeight = URTHexLibrary::ReliefHeightForCost(MoveCosts[I]);
		if (bDoRelief && Relief && ReliefHeight > 0.f)
		{
			// Poggia sulla faccia del disco e cresce verso l'alto; il cilindro engine e' CENTRATO, quindi il
			// suo centro sta a meta' altezza. Piu' stretto della cella (0.6) per non coprire il contorno
			// colorato della superficie, che resta il canale del *tipo* di terreno.
			FVector ReliefCenter = World;
			ReliefCenter.Z += RTCellTopZ + ReliefHeight * 0.5;
			const FTransform ReliefXf(FRotator::ZeroRotator, ReliefCenter,
				FVector(PlanarScale * 0.6f, PlanarScale * 0.6f, ReliefHeight / 100.f));
			Relief->AddInstance(ReliefXf, /*bWorldSpace=*/ true);
			// La cella si registra per ISTANZA, come per i glifi: il pavimento non produce rilievo, quindi il
			// rilievo `N` non e' la cella `N` ([D-227]).
			ReliefCells.Add(CellIds[I]);
			ReliefBaseScale.Add(ReliefXf.GetScale3D());
		}

		// ➕ **IL VOLUME DELLA SUPERFICIE** (`#2936`): il terzo canale, dopo il colore del disco e il glifo
		// inciso. La forma la decide `URTHexLibrary::SurfaceVolumeFor`, che e' l'unica autorita': qui si
		// traduce in una trasformata, non si sceglie.
		//
		// ⚠️ **Legge `Surfaces[I]` e non il costo o i flag**: e' cio' che tiene questa famiglia indipendente da
		// `Relief` (costo) e da `Blockers` (regole di blocco). Il fumo costa 1 e non blocca la vista: nessuna
		// delle altre due lo disegnerebbe mai.
		const FVector2D SurfaceVolume = URTHexLibrary::SurfaceVolumeFor(Surfaces[I]);
		if (bDoSurfaceVolumes && SurfaceVolumes && SurfaceVolume.Y > 0.0)
		{
			// Poggia sulla faccia del disco e cresce verso l'alto, come il rilievo: il cilindro engine e'
			// CENTRATO, quindi il suo centro sta a meta' altezza.
			FVector VolumeCenter = World;
			VolumeCenter.Z += RTCellTopZ + SurfaceVolume.Y * 0.5;
			const FTransform VolumeXf(FRotator::ZeroRotator, VolumeCenter,
				FVector(PlanarScale * SurfaceVolume.X, PlanarScale * SurfaceVolume.X, SurfaceVolume.Y / 100.f));
			SurfaceVolumes->AddInstance(VolumeXf, /*bWorldSpace=*/ true);
			// Per ISTANZA e non per cella, come rilievo e glifi: otto superfici su nove non producono volume,
			// quindi il volume `N` non e' la cella `N`.
			SurfaceVolumeCells.Add(CellIds[I]);
			SurfaceVolumeBaseScale.Add(VolumeXf.GetScale3D());
		}

		// Volumi delle due regole. Sono INDIPENDENTI: una cella puo' averne una, l'altra o entrambe, e in
		// quest'ultimo caso si vedono tutte e due — la lastra larga e bassa attorno alla colonna stretta e
		// alta. Confonderle e' l'errore piu' frequente su questa mappa: una cella che blocca la vista si
		// ATTRAVERSA, ed e' cio' che serve a una rotta coperta ma percorribile.
		if (bDoBlock && Blockers)
		{
			auto AddVolume = [&](float PlanarFraction, float VolumeHeight)
			{
				FVector Center = World;
				Center.Z += RTCellTopZ + VolumeHeight * 0.5;
				const FTransform VolumeXf(FRotator::ZeroRotator, Center,
					FVector(PlanarScale * PlanarFraction, PlanarScale * PlanarFraction, VolumeHeight / 100.f));
				Blockers->AddInstance(VolumeXf, /*bWorldSpace=*/ true);
				// Una cella puo' passare di qui DUE volte — lastra e colonna insieme — e le due istanze
				// vanno velate entrambe: la mappatura e' per istanza, non per cella ([D-227]).
				BlockerCells.Add(CellIds[I]);
				BlockerBaseScale.Add(VolumeXf.GetScale3D());
			};

			if (BlocksSight[I]) { AddVolume(RTSightSlabScale, RTSightSlabHeight); }
			if (BlocksMove[I])  { AddVolume(RTBlockColumnScale, RTBlockColumnHeight); }
		}

		// Pannelli di BORDO: coperture e porte. Il punto e l'orientamento si CHIEDONO alla libreria
		// (`EdgeMidpointWorld`, `EdgeRotation`), che li deriva dai due centri di cella: se la convenzione dei
		// sei lati cambiasse, la geometria seguirebbe invece di mentire.
		//
		// 🔴 **`bDoEdges` mancava da questa guardia, e la ricostruzione PARZIALE le duplicava** (`#2894`).
		// Il `ClearInstances` di questa famiglia e' condizionato al flag (`if (bDoEdges)` piu' su), questo
		// ciclo non lo era: una `RebuildInstances(Cells | Glyphs)` non le puliva e le **riaggiungeva**.
		// Misurato dal rosso di `Veil.CoversEveryInstanceFamily`: le ausiliarie passavano da **148** a
		// **222**, cioe' esattamente i 74 pannelli di bordo contati due volte.
		//
		// ⚠️ **Il difetto era latente, non teorico**: fino a `#2894` `RebuildInstances` aveva **un solo**
		// chiamante non-`All` — nessuno — quindi la famiglia si ripuliva sempre insieme alle altre e il buco
		// non poteva manifestarsi. Il primo uso parziale l'ha trovato al primo test.
		if (bDoEdges && EdgeFeatures && EdgeSources[I])
		{
			const FRTHexCellData& Data = *EdgeSources[I];
			auto AddEdgePanel = [&](ERTHexDirection Edge, float PanelHeight)
			{
				FVector Center = URTHexLibrary::EdgeMidpointWorld(CellIds[I], Edge, GetActorLocation(),
					UseHexSize, UseLayerH);
				Center.Z += static_cast<double>(Heights[I]) + RTCellTopZ + PanelHeight * 0.5;
				// Il cubo engine e' 100 uu per lato: X sottile (spessore), Y lungo il bordo, Z l'altezza.
				const FTransform PanelXf(URTHexLibrary::EdgeRotation(CellIds[I], Edge), Center,
					FVector(RTEdgePanelThickness,
						UseHexSize / 100.f * RTEdgePanelWidth,
						PanelHeight / 100.f));
				EdgeFeatures->AddInstance(PanelXf, /*bWorldSpace=*/ true);
				// Una per copertura e una per porta, quindi piu' istanze sulla stessa cella ([D-227]).
				EdgeFeatureCells.Add(CellIds[I]);
				// `#3691`: e la cella dall'altra parte del bordo, che il velo guarda insieme alla prima.
				EdgeFeatureOtherCells.Add(URTHexLibrary::Neighbor(CellIds[I], Edge));
				EdgeFeatureBaseScale.Add(PanelXf.GetScale3D());
			};

			for (const FRTHexCover& Cover : Data.Covers)
			{
				if (Cover.Type == ERTHexCoverType::None) { continue; }
				AddEdgePanel(Cover.Edge,
					Cover.Type == ERTHexCoverType::High ? RTCoverHighHeight : RTCoverLowHeight);
			}
			for (const FRTHexDoor& Door : Data.Doors)
			{
				// `Destroyed` e' terminale e non si richiude: si mostra come aperta, perche' e' cio' che e'.
				const bool bBlocking = (Door.State == ERTHexDoorState::Closed
					|| Door.State == ERTHexDoorState::Locked);
				AddEdgePanel(Door.Edge, bBlocking ? RTDoorClosedHeight : RTDoorOpenHeight);
			}
		}
	}

	// MURI INTERNI (formato v10, #712): non stanno su un bordo, quindi non passano da `AddEdgePanel`.
	//
	// ⚠️ Stanno sull'ASSET e non nella cella, quindi il ciclo e' qui fuori e non dentro quello sopra: un
	// muro interno non appartiene alla cella nel modo in cui ci appartengono coperture e porte — la cella
	// non lo conosce.
	// ⚠️ Il pannello NON e' orientato con `EdgeRotation`, che deriva l'angolo dai due centri di cella:
	// qui non c'e' nessun vicino da guardare, la giacitura e' quella del segmento e basta.
	//
	// 🔴 **`bDoEdges` mancava anche qui** (`#2894`), ed e' il SECONDO blocco che scrive in `EdgeFeatures`:
	// il primo sono coperture e porte dentro il ciclo delle celle, questo sono i muri interni, che leggono
	// l'asset direttamente. Entrambi condividono un solo `ClearInstances`, quindi **entrambi** devono
	// condividerne la condizione: correggerne uno solo avrebbe lasciato la duplicazione a meta', cioe' un
	// difetto piu' difficile da vedere di quello di partenza.
	if (bDoEdges && EdgeFeatures && MapAsset)
	{
		for (const FRTHexInteriorWall& Wall : MapAsset->InteriorWalls)
		{
			// 🔴 **Il filtro dei layer va applicato anche qui.** Celle, coperture e porte lo ereditano dal
			// ciclo principale — le ultime due tramite `EdgeSources`, popolato solo per le celle filtrate —
			// ma questo ciclo legge l'asset direttamente, quindi non eredita niente. Senza, in `ActiveOnly`
			// o `Focus` un muro interno di un altro piano comparirebbe come pannello **fluttuante**, senza
			// la cella che lo contiene: un'anteprima che mostra un piano e ci mette dentro la geometria di
			// un altro.
			if (!PassesLayerFilter(Wall.Cell.Layer))
			{
				continue;
			}

			const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, UseHexSize);
			if (Line.Points.Num() < 2)
			{
				continue; // segmento fuori grammatica: non produce geometria, quindi non se ne disegna
			}

			const FRTHexCellData* Data = MapAsset->FindCell(Wall.Cell);
			const double CellTop = Data ? static_cast<double>(Data->Height) : 0.0;
			const FVector Centre = URTHexLibrary::AxialToWorld(Wall.Cell, GetActorLocation(), UseHexSize, UseLayerH);

			const FVector2D A = Line.Points[0];
			const FVector2D B = Line.Points[1];
			const double Length = FVector2D::Distance(A, B);
			if (Length <= UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float PanelHeight = (Wall.Segment.WallType == ERTHexCoverType::High)
				? RTCoverHighHeight : RTCoverLowHeight;

			const FVector CellBase(Centre.X, Centre.Y, Centre.Z + CellTop);
			const FTransform WallXf = InteriorWallPanel(A, B, CellBase, PanelHeight, RTEdgePanelThickness);
			EdgeFeatures->AddInstance(WallXf, /*bWorldSpace=*/ true);
			// Il SECONDO sito che monta pannelli di bordo: i muri interni. Dimenticarlo qui lascerebbe
			// disallineati gli indici di TUTTI i pannelli, non solo dei suoi ([D-227]).
			EdgeFeatureCells.Add(Wall.Cell);
			EdgeFeatureOtherCells.Add(Wall.Cell); // `#3691`: un muro interno non ha un altro lato
			EdgeFeatureBaseScale.Add(WallXf.GetScale3D());
		}
	}

	// ➕ **IL CORPO STRUTTURALE** (`#1865`): il volume che si vede SOTTO una superficie.
	//
	// 🔑 **Il derivatore decide, questo posa.** Le quote arrivano gia' conciliate da
	// `URTStructuralBodyLibrary::DeriveBodies` — frazione d'autore contro cio' che c'e' sotto — e qui non si
	// ricalcola niente: duplicare quella regola nel rendering sarebbe la seconda verita' che #1865 vieta.
	//
	// ⚠️ Le quote del derivatore sono in **spazio mappa** (origine a `Z = 0`), quindi ci si somma l'origine
	// dell'actor. La X e la Y vengono da `AxialToWorld` come per ogni altra famiglia; la sua Z **no**, perche'
	// quella e' la quota del CENTRO CELLA e il corpo sta piu' in basso.
	//
	// ⛔ **Rispetta il filtro dei piani** come le celle: un corpo che restasse visibile in `ActiveOnly`
	// mostrerebbe il volume di un piano che l'editor ha nascosto.
	if (bDoBodies && StructuralBodies && MapAsset != nullptr && CellShape != nullptr)
	{
		const FVector ActorOrigin = GetActorLocation();
		for (const FRTStructuralBody& Body : URTStructuralBodyLibrary::DeriveBodies(MapAsset, GetPosedLayerHeight()))
		{
			if (!PassesLayerFilter(Body.Cell.Layer))
			{
				continue;
			}
			const FVector Centro = URTHexLibrary::AxialToWorld(Body.Cell, ActorOrigin, UseHexSize, UseLayerH);
			// La mesh nasce alta `2 * RTCellPrismRadius` col pivot al centro: la scala Z e' quel rapporto, e
			// il centro del corpo sta a meta' fra le sue due facce.
			const float AltezzaZ = Body.Height() / (2.f * RTCellPrismRadius);
			const FVector Posizione(Centro.X, Centro.Y,
				ActorOrigin.Z + static_cast<double>(Body.BottomZ + Body.TopZ) * 0.5);
			const FTransform CorpoXf(FRotator::ZeroRotator, Posizione,
				FVector(PlanarScale, PlanarScale, AltezzaZ));
			StructuralBodies->AddInstance(CorpoXf, /*bWorldSpace=*/ true);

			// 🔴 **La registrazione che mancava, ed e' cio' che rendeva questa famiglia INVELABILE** (`#2731`).
			// Ogni altra famiglia registra `<Famiglia>Cells` / `<Famiglia>BaseScale` al proprio sito di
			// `AddInstance`; questo non registrava niente, quindi `ApplyKnowledgeVeil` non aveva indici da
			// velare e il volume solido restava visibile **sotto celle mai osservate** — cioe' esattamente
			// la geometria che [D-225] dichiara di non disegnare.
			//
			// ⚠️ **Una cella produce al piu' un corpo**, a differenza dei `Blockers` che possono passare due
			// volte (lastra e colonna): la mappatura resta comunque **per istanza** e non per cella, perche'
			// e' l'indice dell'istanza che il velo usa.
			BodyCells.Add(Body.Cell);
			BodyBaseScale.Add(CorpoXf.GetScale3D());
		}
	}

	// ➕ **IL COSTO DELLA RICOSTRUZIONE, reso leggibile** (`#1865`, punto 3).
	//
	// 🔑 Oggi «create» e «totali» coincidono, perche' questa funzione azzera ogni famiglia e le rifa' da
	// capo: e' precisamente il difetto che l'AC descrive — *«il numero di istanze ricreate deve essere
	// proporzionale alla modifica, non alla mappa»* — e senza questo numero non era falsificabile.
	//
	// ⚠️ **Chi rendera' incrementale la ricostruzione dovra' cambiare COME si conta**, non solo dove: un
	// rebuild parziale crea poche istanze e ne aggiorna molte, e sommare i totali continuerebbe a
	// rispondere «tutta la board» nascondendo l'ottimizzazione appena fatta. La somma qui sotto vale
	// finche' il rebuild e' totale, ed e' vera oggi.
	{
		// 🔑 **Si contano SOLO le famiglie richieste**, ed e' il cambiamento che il commento precedente
		// annunciava: *«chi rendera' incrementale la ricostruzione dovra' cambiare COME si conta»*. Sommare
		// i totali di tutte le famiglie continuerebbe a rispondere «tutta la board» anche dopo una
		// ricostruzione parziale, nascondendo l'ottimizzazione appena fatta.
		auto Conta = [](const UInstancedStaticMeshComponent* C) { return C ? C->GetInstanceCount() : 0; };
		LastRebuildCreated = 0;
		if (bDoCells)   { LastRebuildCreated += Conta(Cells); }
		if (bDoRelief)  { LastRebuildCreated += Conta(Relief); }
		if (bDoBlock)   { LastRebuildCreated += Conta(Blockers); }
		if (bDoEdges)   { LastRebuildCreated += Conta(EdgeFeatures); }
		if (bDoBorders) { LastRebuildCreated += Conta(CellBorders); }
		if (bDoBodies)  { LastRebuildCreated += Conta(StructuralBodies); }
		if (bDoGlyphs)
		{
			for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring) { LastRebuildCreated += Conta(SurfaceGlyphs[Ring]); }
		}
	}

	// #3706: dopo tutti i `SetStaticMesh` di qui sopra, che riportano gli slot ai materiali della mesh.
	ApplyVolumeMaterial();

	// #3679: le istanze nuove nascono coi canali del trattamento a zero, cioe' neutre. Se chi guarda aveva gia' un piano
	// attivo, il trattamento si riscrive nello stesso fotogramma: la board non torna piatta per una ricostruzione.
	if (bLayerPresentationApplied)
	{
		ApplyLayerPresentation();
	}

	// ➕ **La mappatura cella→istanza si e' mossa** (`#2761`): il guardiano di identita' la riverifica al
	// prossimo velo. Sta FUORI dal blocco diagnostico qui sopra — quello e' guardato da `WITH_EDITOR`, questo
	// e' il presidio di un invariante e vale ovunque.
	++VeilMappingRevision;

#if WITH_EDITOR
	// Le coordinate seguono la mappa: stesso innesco delle istanze, quindi nessuna regola di
	// invalidazione nuova da tenere allineata.
	RebuildCoordinateLabels();
#endif
}

#if WITH_EDITOR
void ARTHexMapActor::RebuildCoordinateLabels()
{
	if (!CoordinateLabels)
	{
		return;
	}
	CoordinateLabels->Flush();

	// La spec vuole «solo editor, mai in partita», ma `WITH_EDITOR` non lo garantisce da solo: in un
	// binario d'editor la macro vale 1 ANCHE dentro il mondo PIE, e questa funzione e' raggiunta dal
	// percorso di gioco (RebuildInstances e' chiamata da RTMatchBootstrapper e da RTScenarioSession
	// durante una partita). Il motore non le nasconde da solo — `FPrimitiveSceneProxy::IsShown` salta
	// il primitive in game mode solo se l'ACTOR e' editor-only, e `ARTHexMapActor` non lo e' — quindi la
	// guardia va messa qui a mano, con lo stesso principio gia' in uso per `bCellOverlay` (spento salvo
	// accensione esplicita dell'editor mode) e per `KnowledgeVolumes` (nato con `SetVisibility(false)`).
	// La guardia sta DOPO il `Flush()` sopra apposta: entrando in PIE le linee gia' posate in editor
	// vanno comunque cancellate, non lasciate appese.
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		return;
	}

	if (!MapAsset)
	{
		return; // nessuna mappa, nessuna coordinata: non si inventa una griglia
	}

	FVector MapOrigin = FVector::ZeroVector;
	// `MapHexSize`/`MapLayerHeight` e non `HexSize`/`LayerHeight`: quei nomi nascondono i membri
	// `ARTHexMapActor::HexSize`/`LayerHeight` (C4458), lo stesso motivo per cui `DrawCellOverlay` qui
	// sopra chiama i suoi locali `Size`/`LayerH` invece del nome del parametro `GetHexContext`.
	float MapHexSize = 0.f;
	float MapLayerHeight = 0.f;
	GetHexContext(MapOrigin, MapHexSize, MapLayerHeight);

	// Tinta scura provvisoria per il segno inciso — NON e' lo stesso valore lineare del registro dei
	// glifi/bordi ([D-183], che usa `FLinearColor::FromSRGBColor(FColor(25,25,25))`, ≈0,0097 lineare):
	// qui e' un letterale lineare diretto, circa otto volte piu' chiaro. Il mezzo e' diverso (linee di
	// debug contro custom data di un materiale) e il numero puo' legittimamente differire, ma resta una
	// scelta non tarata a schermo — vedi NOT RUN nel report del Task 3.
	// ⚠️ `FLinearColor`, non `FColor`: e' il tipo che `FBatchedLine` prende. Un `FColor` compilerebbe per
	// conversione implicita e passerebbe per lo spazio sbagliato.
	const FLinearColor Ink(0.08f, 0.08f, 0.08f, 1.f);
	constexpr float Thickness = 1.0f;

	TArray<FBatchedLine> Lines;
	// Stima per il reserve, non un limite: UNA run, poche cifre nel caso comune (non il caso
	// peggiore da dieci cifre che `NothingLeavesTheHexagonAtTheTrueWorstCase` misura), e ogni carattere del
	// set chiuso costa da 1 segmento (virgola) a 7 ('8'). Serve solo a risparmiare le prime riallocazioni
	// quando si trascina il pennello (RebuildInstances gira a ogni cella toccata) — se la stima e' bassa
	// `TArray` continua comunque a crescere.
	// ⌫ Era `* 90` quando le run erano tre: da una sola, un terzo. La stima resta larga di proposito —
	// costa memoria una volta, e sbagliarla in difetto costa riallocazioni a ogni cella del pennello.
	Lines.Reserve(MapAsset->Cells.Num() * 30);
	for (const FRTHexCellData& Cell : MapAsset->Cells)
	{
		// Stesso filtro layer di RebuildInstances: senza, in ActiveOnly/Focus le terne dei piani nascosti
		// cadrebbero sopra il disco del piano isolato — l'opposto di cio' per cui quelle modalita' esistono.
		if (!PassesLayerFilter(Cell.Id.Layer))
		{
			continue;
		}

		// `Height` alza l'ISTANZA (RebuildInstances, `Heights[I]`) e le altre due famiglie di linee di
		// questo file (`DrawCellOverlay::CellLift`, `DrawPlanningPreview::CellLift` sopra) la seguono: senza
		// qui, su una cella rialzata le terne restano sepolte di `Height` unita' dentro il disco.
		// `BuildCellLabel` non la conosce — prende `FRTCellId`, non `FRTHexCellData` — quindi va sommata qui.
		const float CellLift = static_cast<float>(Cell.Height) + RTLiftCoordinateLabel;

		const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell.Id, MapOrigin, MapHexSize, MapLayerHeight);
		for (const FRTLabelGlyph& Glyph : Label.Glyphs)
		{
			for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(Glyph.Character))
			{
				const FVector A = Glyph.Origin + Glyph.Right * S.From.X + Glyph.Up * S.From.Y + FVector(0, 0, CellLift);
				const FVector B = Glyph.Origin + Glyph.Right * S.To.X   + Glyph.Up * S.To.Y   + FVector(0, 0, CellLift);
				// Firma: (Start, End, FLinearColor, LifeTime, Thickness, DepthPriority).
				// `LifeTime` negativo = la linea resta finche' non si chiama `Flush()`, che e' il punto:
				// si posa quando la mappa cambia, non a ogni frame.
				Lines.Emplace(A, B, Ink, /*LifeTime*/ -1.f, Thickness, /*DepthPriority*/ uint8(SDPG_World));
			}
		}
	}
	CoordinateLabels->DrawLines(Lines);
}
#endif

void ARTHexMapActor::GenerateIntoAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato: assegnalo prima di generare."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexGenerate", "Hex: Generate Area"));
#endif
	MapAsset->Modify();
	const TArray<FRTCellId> Ids = URTHexLibrary::HexArea(FRTCellId(0, 0, ActiveLayer), FMath::Max(0, DemoRadius));
	for (const FRTCellId& Id : Ids)
	{
		FRTHexCellData Cell(Id);
		Cell.Surface = DemoSurface;
		MapAsset->AddOrUpdateCell(Cell);
	}
	MapAsset->SortCells();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Generate: %d celle nell'asset (raggio %d, layer %d)."), Ids.Num(), DemoRadius, ActiveLayer);
}

void ARTHexMapActor::GenerateArenaV01IntoAsset()
{
	// Scorciatoia per la fixture piu' usata: la seduta U1 la nomina, e cambiarle nome costringerebbe a
	// riscrivere una guida per un pulsante che fa gia' la cosa giusta.
	const FString Previous = FixtureId;
	FixtureId = TEXT("ArenaV01");
	GenerateFixtureIntoAsset();
	FixtureId = Previous;
}

void ARTHexMapActor::GenerateFixtureIntoAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato: assegnalo prima di generare."));
		return;
	}

	const URTHexMapAsset* Source = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), FixtureId);
	if (!Source)
	{
		// Nome sconosciuto: non si tocca nulla. Svuotare l'asset per un refuso sarebbe il danno peggiore, e
		// il messaggio dice QUALE nome non esiste invece di lamentarsi in astratto.
		// ⚠️ I nomi si CHIEDONO, non si riscrivono (`#1459`): questa riga ne elencava sei, uno dei quali
		// (`DemoArena`) non aveva un ramo nel dispatcher e un altro (`ArenaV01`) c'era senza essere elencato
		// dalla doc. Un messaggio d'errore che nomina fra i validi proprio quello che l'utente ha appena
		// chiesto e' peggio di uno generico.
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Fixture '%s' sconosciuta: asset invariato. Nomi validi: %s."),
			*FixtureId, *FString::Join(URTMatchSetupLibrary::KnownFixtureIds(), TEXT(", ")));
		return;
	}

#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexGenerateFixture", "Hex: Generate Fixture"));
#endif
	MapAsset->Modify();
	// Sostituisce, non fonde: mescolare una fixture con quello che c'era darebbe una mappa che non e' ne'
	// l'una ne' l'altra, e i criteri smetterebbero di dire qualcosa su cio' che si ha davanti.
	//
	// In UNA revisione (#905): scrivere il ciclo a mano ne produceva una per cella — 98 per l'arena della
	// v0.1 — e nessuna per le transizioni, che venivano assegnate direttamente. Generare una fixture e'
	// un evento solo, ed e' la regola che `UpdateCells` gia' enuncia.
	MapAsset->ReplaceContent(Source->Cells, Source->Transitions);
	MapAsset->SortCells();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Fixture '%s' scritta nell'asset: %d celle, %d transizioni."),
		*FixtureId, MapAsset->NumCells(), MapAsset->Transitions.Num());
}

void ARTHexMapActor::ClearAsset()
{
	if (!MapAsset)
	{
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexClear", "Hex: Clear"));
#endif
	MapAsset->Modify();
	// Il reset passa dall'asset, come ogni altra modifica: e' lui a possedere `Revision`. Scrivere sui due
	// array da qui la lasciava ferma, ed era l'unica modifica strutturale a farlo (#902).
	MapAsset->ClearAll();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Asset svuotato."));
}

void ARTHexMapActor::ValidateAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
	const TArray<FString> Errors = MapAsset->ValidateMap();
	if (Errors.Num() == 0)
	{
		UE_LOG(LogRT, Log, TEXT("[HexMap] Validazione OK: nessun errore (%d celle)."), MapAsset->NumCells());
	}
	for (const FString& E : Errors)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] %s"), *E);
	}
}

void ARTHexMapActor::PaintTargetCell()
{
	PaintCellData(PaintCellTarget, PaintSurface, PaintMoveCost, bPaintBlocksMovement);
}

void ARTHexMapActor::PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexPaint", "Hex: Paint Cell"));
#endif
	MapAsset->BeginStroke();
	MapAsset->PaintCellInStroke(Id, Surface, MoveCost, bBlocksMovement);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Paint su %s (superficie %d, costo %d, blocca=%d)."),
		*Id.ToString(), static_cast<int32>(Surface), MoveCost, bBlocksMovement ? 1 : 0);
}

bool ARTHexMapActor::EraseCell(const FRTCellId& Id)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
	if (!MapAsset->ContainsCell(Id))
	{
		// Niente da cancellare: nessuna transazione no-op sullo stack di Undo.
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexErase", "Hex: Erase Cell"));
#endif
	MapAsset->BeginStroke();
	const bool bRemoved = MapAsset->EraseCellInStroke(Id);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Erase %s: %s."), *Id.ToString(), bRemoved ? TEXT("rimossa") : TEXT("assente"));
	return bRemoved;
}

void ARTHexMapActor::AddVerticalTransition()
{
	AddTransitionData(TransitionFrom, TransitionTo, FMath::Max(0, TransitionCost), TransitionKind, bTransitionBidirectional);
}

void ARTHexMapActor::AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
	ERTHexTransitionKind Kind, bool bBidirectional)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
	if (!MapAsset->ContainsCell(From) || !MapAsset->ContainsCell(To))
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Transizione %s -> %s: una delle due celle non esiste nell'asset."),
			*From.ToString(), *To.ToString());
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexAddTransition", "Hex: Add Vertical Transition"));
#endif
	MapAsset->Modify();
	MapAsset->AddTransition(From, To, FMath::Max(0, Cost), Kind, bBidirectional);
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Transizione aggiunta %s -> %s (tipo %d, costo %d, bidirezionale=%d)."),
		*From.ToString(), *To.ToString(), static_cast<int32>(Kind), Cost, bBidirectional ? 1 : 0);
}

void ARTHexMapActor::RemoveVerticalTransition()
{
	RemoveTransitionData(TransitionFrom, TransitionTo, bTransitionBidirectional);
}

bool ARTHexMapActor::RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexRemoveTransition", "Hex: Remove Vertical Transition"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveTransition(From, To, bBothDirections);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Rimozione transizione %s -> %s: %s."),
		*From.ToString(), *To.ToString(), bRemoved ? TEXT("rimossa") : TEXT("non trovata"));
	return bRemoved;
}

#undef LOCTEXT_NAMESPACE

// --- Il velo della fog of war ([D-225], [D-227]) --------------------------------------------------------

// Il velo MOLTIPLICA l'RGB lineare invece di aggiungere un quarto float: il quarto float richiederebbe di
// toccare `M_HexCell.uasset` — un binario, human-first, un lavoro per volta — per un risultato che il
// ricalcolo ottiene gia'.
// ➕ Da #3679 i float sono sei, ma i canali 0..2 restano del velo e del colore: il trattamento dei piani sta nei
// canali 3..5 (`SyncActiveLayerPresentation`), e il materiale lo applica DOPO di loro. Le due scritture non si
// toccano, quindi il velo non sa niente del piano attivo e il trattamento niente del velo.

ERTHexSurface ARTHexMapActor::SurfaceForCell(const FRTCellId& Cell) const
{
	// Si rilegge dall'asset invece di essere memorizzata: `SurfaceColor` resta l'unica verita' sul colore, e
	// una copia per istanza sarebbe la seconda — divergenti al primo colpo di pennello.
	if (MapAsset)
	{
		if (const FRTHexCellData* Data = MapAsset->FindCell(Cell))
		{
			return Data->Surface;
		}
	}
	// Il graybox demo non ha terreni: `RebuildInstances` lo costruisce tutto `Floor`, e qui si resta coerenti.
	return ERTHexSurface::Floor;
}

void ARTHexMapActor::CollectEdgeLayout(TArray<int32>& Out) const
{
	Out.Reset();
	if (!MapAsset)
	{
		return; // il ramo demo non ha bordi d'autore
	}

	// L'ordine e' quello dell'asset, che e' anche quello in cui `RebuildInstances` posa i pannelli: due
	// letture dello stesso asset danno la stessa sequenza. Un marcatore di tipo davanti a ogni voce, cosi'
	// una porta e una copertura con gli stessi numeri non si confondono.
	for (const FRTHexCellData& C : MapAsset->Cells)
	{
		if (C.Covers.Num() == 0 && C.Doors.Num() == 0)
		{
			continue;
		}
		// L'altezza della cella entra perche' il pannello si posa sulla sua quota (`AddEdgePanel`).
		Out.Append({ 0, C.Id.X, C.Id.Y, C.Id.Layer, C.Height });
		for (const FRTHexCover& Cover : C.Covers)
		{
			Out.Append({ 1, static_cast<int32>(Cover.Edge), static_cast<int32>(Cover.Type) });
		}
		for (const FRTHexDoor& Door : C.Doors)
		{
			Out.Append({ 2, static_cast<int32>(Door.Edge), static_cast<int32>(Door.State) });
		}
	}
	// I muri interni non cambiano in partita oggi, ma stanno nella stessa famiglia: lasciarli fuori
	// renderebbe questo confronto cieco al primo produttore che li toccasse. Entra tutto cio' che il disegno
	// legge: il segmento intero (`ToPolyline`), il tipo e la quota della cella.
	for (const FRTHexInteriorWall& Wall : MapAsset->InteriorWalls)
	{
		const FRTHexCellData* Dati = MapAsset->FindCell(Wall.Cell);
		Out.Append({ 3, Wall.Cell.X, Wall.Cell.Y, Wall.Cell.Layer, Dati ? Dati->Height : 0,
			static_cast<int32>(Wall.Segment.WallType), static_cast<int32>(Wall.Segment.Axis), Wall.Segment.Offset,
			Wall.Segment.AlongStart, Wall.Segment.AlongEnd, Wall.Segment.Layer });
	}
}

int32 ARTHexMapActor::VeilInstances(UInstancedStaticMeshComponent* Component,
	const TArray<FRTCellId>& CellsOfInstance, const TArray<FVector>& BaseScale, TArray<uint8>& LastState,
	TArray<float>* DisplayFactor,
	const TSet<FRTCellId>& Visible, const TSet<FRTCellId>& Explored,
	TFunctionRef<bool(const FRTCellId&, FLinearColor&)> BaseColor,
	const TArray<FRTCellId>* OtherCellOfInstance)
{
	if (!Component || CellsOfInstance.Num() == 0)
	{
		return 0;
	}

	// #3760: che cosa fa questa famiglia nel ricordo. Si chiede una volta per chiamata, non per istanza.
	const ERTVeilMemory Ricordo = MemoryFor(Component);

	// La precondizione DICHIARATA: la mappatura e' stata DERIVATA da `RebuildInstances`, e una ricostruzione
	// passata di mezzo lascia indici stantii. L'esito non e' un crash ma celle velate SBAGLIATE — il difetto
	// che si legge come «problema grafico» per settimane, finche' qualcuno non lo misura.
	if (!ensureMsgf(Component->GetInstanceCount() == CellsOfInstance.Num(),
		TEXT("ApplyKnowledgeVeil: %s ha %d istanze contro %d celle mappate — RebuildInstances e' passato di mezzo"),
		*Component->GetName(), Component->GetInstanceCount(), CellsOfInstance.Num()))
	{
		return 0;
	}

	// ➕ **E il conteggio NON basta** (`#2761`): passa su due array della stessa lunghezza con le celle
	// scambiate. Il guardiano che coglie una permutazione — `VerifyMappingIdentity` — non sta qui ma in
	// `ApplyKnowledgeVeil`, prima di tutte le famiglie: costa `O(N)` per famiglia, e va pagato una volta per
	// mappatura nuova invece che una volta per famiglia per velo.

	// Lo stato precedente, per saltare cio' che non cambia. La prima volta e' tutto `Unwritten`, quindi il
	// primo velo tocca ogni istanza; dal secondo in poi tocca solo il bordo del cono.
	if (LastState.Num() != CellsOfInstance.Num())
	{
		LastState.Init(RTVeilUnwritten, CellsOfInstance.Num());
	}

	// Il fattore disegnato, parallelo allo stato. Nasce a `-1`: un valore che nessun target puo' avere, quindi
	// la prima scrittura di ogni istanza e' sempre uno **snap** e non una dissolvenza — la board non si
	// accende sfumando alla prima inquadratura.
	if (DisplayFactor && DisplayFactor->Num() != CellsOfInstance.Num())
	{
		DisplayFactor->Init(-1.f, CellsOfInstance.Num());
	}

	int32 Toccate = 0;
	for (int32 I = 0; I < CellsOfInstance.Num(); ++I)
	{
		const FRTCellId& Cell = CellsOfInstance[I];
		// `#3691`: per un pannello di bordo conta il lato piu' visibile dei due. ⚠️ Una mappatura parallela
		// disallineata ricade sulla sola cella mappata, cioe' sul velo di prima: mai piu' visibile di cosi'.
		const FRTCellId* Altra = (OtherCellOfInstance && OtherCellOfInstance->Num() == CellsOfInstance.Num())
			? &(*OtherCellOfInstance)[I] : nullptr;
		const bool bVisible = Visible.Contains(Cell) || (Altra && Visible.Contains(*Altra));
		const bool bKnown = bVisible || Explored.Contains(Cell) || (Altra && Explored.Contains(*Altra));
		const uint8 State = bVisible ? RTVeilLit : (bKnown ? RTVeilRemembered : RTVeilHidden);
		const uint8 Precedente = LastState[I];

		// 🔴 Il salto. `UpdateInstanceTransform` e `SetCustomDataValue` costano anche quando riscrivono lo
		// stesso valore, ed e' li' che finiva la misura del velo ingenuo — 2,2 s su arena piena.
		//
		// ⚠️ **Resta corretto col filtro di `#2875`**: se lo stato non cambia, non cambia nemmeno il target, e
		// una transizione gia' in volo la porta avanti `AdvanceVeilTransition` per conto suo. Questo salto
		// decide chi **riparte**, non chi si muove.
		if (Precedente == State)
		{
			continue;
		}

		// Mai vista: non si disegna ([D-225]). Non e' un velo opaco steso SOPRA un terreno noto — quella
		// sarebbe la «mappa nera» che §25 dell'HUD vieta — ma l'assenza del disegno.
		FTransform Xf;
		if (!Component->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
		{
			// ⚠️ Lo stato NON si segna: marcarlo qui direbbe «applicato» a una scrittura che non e' avvenuta,
			// e ogni chiamata successiva salterebbe l'istanza — la cella resterebbe alla scala sbagliata
			// finche' non passa un `RebuildInstances`.
			continue;
		}
		const FVector Full = BaseScale.IsValidIndex(I) ? BaseScale[I] : FVector::OneVector;
		// #3760: nel ricordo una famiglia `Hide` non si disegna, come una cella mai vista.
		const bool bDrawn = bVisible || (bKnown && Ricordo != ERTVeilMemory::Hide);
		Xf.SetScale3D(bDrawn ? Full : FVector::ZeroVector);
		Component->UpdateInstanceTransform(I, Xf, /*bWorldSpace=*/ true, /*bMarkRenderStateDirty=*/ false);

		LastState[I] = State;
		++Toccate;

		FLinearColor Base;
		if (!bDrawn || !BaseColor(Cell, Base))
		{
			// Niente colore da scrivere: su cio' che non si disegna, e sulle famiglie che non hanno un canale
			// colore per istanza — dove ricordato e osservato restano indistinguibili.
			continue;
		}
		// #3760: la pianta ha un colore suo, a piena luminosita'. Con target 1 sia osservata sia ricordata, il
		// fattore non si muove: il passaggio fra le due e' un cambio di colore secco, non una dissolvenza.
		const bool bPianta = !bVisible && Ricordo == ERTVeilMemory::Plan;
		if (bPianta)
		{
			Base = PlanColor();
		}
		const float Target = (bVisible || bPianta) ? 1.f : RTVeilExploredFactor;

		// 🔴 **Si ATTENUA solo fra due stati DISEGNATI, e il reveal da `Hidden` resta uno scalino.**
		// E' la decisione (i) di `#2875`, e la ragione e' il multilivello: `LayerView` vale `AllLayers` di
		// default — i piani si impilano a `RTCellLayerHeightRef` di distanza — quindi una cella di layer 1
		// disegnata a luminanza quasi nulla **starebbe sopra** quella di layer 0 e la coprirebbe. Sarebbe
		// «un disegno che copre», cioe' la mappa nera che §25 dell'HUD vieta e da cui [D-225] si difende con
		// quelle stesse parole.
		//
		// ⚠️ **Anche la prima scrittura e' uno snap** (`Precedente == RTVeilUnwritten`): senza, la board si
		// accenderebbe sfumando alla prima inquadratura, e il primo fotogramma e' quello che nessun test
		// tardivo prende.
		const bool bAttenua = DisplayFactor
			&& (Precedente == RTVeilLit || Precedente == RTVeilRemembered)
			&& (State == RTVeilLit || State == RTVeilRemembered);

		float Fattore = Target;
		if (DisplayFactor)
		{
			if (!bAttenua)
			{
				(*DisplayFactor)[I] = Target;
			}
			Fattore = (*DisplayFactor)[I];
			if (Fattore != Target)
			{
				++VeilCellsInTransition;
			}
		}

		Component->SetCustomDataValue(I, 0, Base.R * Fattore);
		Component->SetCustomDataValue(I, 1, Base.G * Fattore);
		Component->SetCustomDataValue(I, 2, Base.B * Fattore, /*bMarkRenderStateDirty=*/ false);
	}

	// Una volta sola, in coda, e SOLO se qualcosa e' cambiato: marcarlo a ogni canale ricostruirebbe il
	// buffer 3N volte, e marcarlo a vuoto ricostruirebbe tutto per niente.
	if (Toccate > 0)
	{
		Component->MarkRenderStateDirty();
	}
	return Toccate;
}

bool ARTHexMapActor::VerifyMappingIdentity(const UInstancedStaticMeshComponent* Component,
	const TArray<FRTCellId>& CellsOfInstance, bool bCompareHeight) const
{
	if (!Component || CellsOfInstance.Num() == 0)
	{
		return true;
	}
	// Se i conteggi gia' non tornano, il disallineamento e' quello che l'`ensureMsgf` di `VeilInstances`
	// riporta con parole sue: qui non si aggiunge un secondo allarme sullo stesso fatto.
	if (Component->GetInstanceCount() != CellsOfInstance.Num())
	{
		return true;
	}

	const float UseHexSize = MapAsset ? MapAsset->HexSize : HexSize;
	const float UseLayerH = GetPosedLayerHeight(); // #3632
	// Un quarto del passo fra due celle: molto sotto la distanza fra due centri vicini — quindi una
	// permutazione la supera sempre — e molto sopra il rumore di un `float` in spazio mondo.
	const double Tolleranza = FMath::Max(1.0, static_cast<double>(UseHexSize) * 0.25);

	// La quota ATTESA di una cella: il piano la porta gia', e ci si somma l'altezza dell'asset. Il
	// sollevamento di FAMIGLIA non compare perche' e' costante, e sparisce nella sottrazione qui sotto.
	auto QuotaAttesa = [this](const FRTCellId& Cell, const FVector& Piano) -> double
	{
		double Z = Piano.Z;
		if (MapAsset)
		{
			if (const FRTHexCellData* Data = MapAsset->FindCell(Cell))
			{
				Z += static_cast<double>(Data->Height);
			}
		}
		return Z;
	};

	// L'origine del confronto: lo scarto della PRIMA istanza. Vedi l'header — le istanze vivono in spazio
	// mondo e non seguono l'actor, quindi cio' che si confronta e' la forma della mappatura, non la sua
	// posizione assoluta.
	FTransform Primo;
	if (!Component->GetInstanceTransform(0, Primo, /*bWorldSpace=*/ true))
	{
		return true;
	}
	const FVector PianoPrimo = URTHexLibrary::AxialToWorld(CellsOfInstance[0], GetActorLocation(),
		UseHexSize, UseLayerH);
	const FVector2D Scarto(Primo.GetLocation().X - PianoPrimo.X, Primo.GetLocation().Y - PianoPrimo.Y);
	const double ScartoZ = Primo.GetLocation().Z - QuotaAttesa(CellsOfInstance[0], PianoPrimo);

	for (int32 I = 1; I < CellsOfInstance.Num(); ++I)
	{
		FTransform Xf;
		if (!Component->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
		{
			continue;
		}
		const FVector Piano = URTHexLibrary::AxialToWorld(CellsOfInstance[I], GetActorLocation(),
			UseHexSize, UseLayerH);
		const FVector2D Trovato(Xf.GetLocation().X, Xf.GetLocation().Y);
		const FVector2D Previsto(Piano.X + Scarto.X, Piano.Y + Scarto.Y);
		if (!ensureMsgf(FVector2D::Distance(Trovato, Previsto) <= Tolleranza,
			TEXT("ApplyKnowledgeVeil: %s ha l'istanza %d su %s, che sta a %.1f uu da dove la mappatura la ")
			TEXT("vuole — gli INDICI sono rimescolati, e i conteggi tornano lo stesso"),
			*Component->GetName(), I, *CellsOfInstance[I].ToString(),
			FVector2D::Distance(Trovato, Previsto)))
		{
			return false;
		}

		// 🔴 **La quota, dove e' confrontabile — ed e' l'unico modo di vedere una permutazione fra celle
		// IMPILATE.** `FRTCellId` e' `(X, Y, Layer)` e `AxialToWorld` mette il layer tutto nella `Z`: due
		// celle sopra la stessa colonna hanno esattamente lo stesso piano, e scambiarle sarebbe invisibile a
		// un confronto planare. La mappa e' multilivello per costruzione — `LayerView` vale `AllLayers` di
		// default — quindi non e' un caso di scuola.
		if (!bCompareHeight)
		{
			continue;
		}
		const double PrevistaZ = QuotaAttesa(CellsOfInstance[I], Piano) + ScartoZ;
		if (!ensureMsgf(FMath::Abs(Xf.GetLocation().Z - PrevistaZ) <= Tolleranza,
			TEXT("ApplyKnowledgeVeil: %s ha l'istanza %d su %s alla quota %.1f invece di %.1f — due celle ")
			TEXT("IMPILATE sono state scambiate, e sul piano non si vedrebbe"),
			*Component->GetName(), I, *CellsOfInstance[I].ToString(), Xf.GetLocation().Z, PrevistaZ))
		{
			return false;
		}
	}
	return true;
}

bool ARTHexMapActor::VerifyAllMappings() const
{
	// Le famiglie con un sollevamento COSTANTE per istanza — disco, corone, griglia — ammettono anche il
	// confronto sulla quota, che e' l'unico modo di distinguere due celle impilate.
	bool bOk = VerifyMappingIdentity(Cells, InstanceCells, /*bCompareHeight=*/ true);
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		bOk = VerifyMappingIdentity(SurfaceGlyphs[Ring], GlyphCells[Ring], /*bCompareHeight=*/ true) && bOk;
	}
	bOk = VerifyMappingIdentity(CellBorders, BorderCells, /*bCompareHeight=*/ true) && bOk;

	// ⚠️ **Queste tre portano un sollevamento che dipende dalla CELLA e non solo dalla famiglia** — il
	// rilievo cresce col sovrapprezzo di movimento, i volumi hanno due altezze diverse, il corpo strutturale
	// segue il proprio riempimento — quindi la quota non e' confrontabile senza ricopiarne le formule qui.
	// Restano sul solo piano, che coglie ogni permutazione tranne quella fra celle impilate.
	bOk = VerifyMappingIdentity(Relief, ReliefCells, /*bCompareHeight=*/ false) && bOk;
	bOk = VerifyMappingIdentity(Blockers, BlockerCells, /*bCompareHeight=*/ false) && bOk;
	bOk = VerifyMappingIdentity(StructuralBodies, BodyCells, /*bCompareHeight=*/ false) && bOk;

	// ⛔ `EdgeFeatures` non compare, ed e' dichiarato: i pannelli di bordo stanno sul punto medio di un LATO
	// (`EdgeMidpointWorld`), non sul centro della cella mappata, quindi nemmeno il piano e' confrontabile
	// senza sapere quale dei sei bordi ha prodotto l'istanza. Resta col solo conteggio.
	//
	// ⚠️ **`&& bOk` a destra e non a sinistra, in tutte le righe**: con l'ordine opposto il corto circuito
	// salterebbe le famiglie successive alla prima rotta, e il log direbbe un guasto solo dove ce ne sono
	// due — cioe' manderebbe a cercare la causa nel posto sbagliato.
	return bOk;
}

bool ARTHexMapActor::UsesRemoveAtSwap(const UInstancedStaticMeshComponent* Component)
{
	if (!Component)
	{
		return false;
	}
	// 🔑 **Si CHIEDE al componente, e la domanda e' una sola.** `SupportsRemoveSwap()` e' il canale che il
	// motore dichiara nel proprio header — *«l'implementazione e' libera di ignorare `bSupportRemoveAtSwap`,
	// ma deve onorare cio' che `SupportsRemoveSwap()` risponde»* — e in `5.8` copre **anche** la CVar:
	//
	//     bool UInstancedStaticMeshComponent::SupportsRemoveSwap() const
	//     { return bSupportRemoveAtSwap || CVarISMForceRemoveAtSwap.GetValueOnGameThread() != 0; }
	//
	// ⌫ **Una prima stesura leggeva la CVar da sé**, con accanto un commento che diceva *«e la CVar, che il
	// flag non copre»*. Era **falso**: il flag la copre, quel ramo era irraggiungibile, e la ricerca per nome
	// nel registro globale delle CVar girava a ogni rimozione di glifo per un valore che non poteva
	// differire. Rimosso il 2026-09-10.
	//
	// ⚠️ **Resta vero cio' che rendeva la CVar interessante**: la semantica degli indici e' modificabile
	// dalla console in qualunque momento, quindi si rilegge a ogni rimozione invece di memorizzarla.
	return Component->SupportsRemoveSwap();
}

bool ARTHexMapActor::RemoveInstanceMirrored(UInstancedStaticMeshComponent* Component, int32 Index,
	TArray<FRTCellId>& CellsOfInstance, TArray<FVector>& BaseScale, TArray<uint8>* LastState,
	TArray<float>* DisplayFactor)
{
	if (!Component || !CellsOfInstance.IsValidIndex(Index))
	{
		return false;
	}

	// 🔴 **La validita' si controlla contro il COMPONENTE, e `RemoveInstance` non la riporta.**
	// `RemoveInstanceInternal` decide con `bValidInstance` se rimuovere, poi cade su un `return true`
	// **incondizionato**: su un indice fuori dalle sue istanze non rimuove niente e risponde comunque `true`.
	//
	// ⌫ Una prima stesura si fidava di quel valore di ritorno. L'esito sarebbe stato il peggiore possibile:
	// con `CellsOfInstance` piu' lungo del componente — cioe' proprio il disallineamento contro cui tutto
	// questo codice esiste — la chiamata al motore sarebbe stata un nulla di fatto e lo specchio avrebbe
	// accorciato **solo** gli array paralleli. I conteggi sarebbero tornati a combaciare, l'`ensureMsgf` di
	// `VeilInstances` sarebbe passato, e la mappatura sarebbe rimasta traslata per sempre: questa funzione
	// avrebbe FABBRICATO la corruzione che ha il compito di impedire.
	if (!Component->IsValidInstance(Index))
	{
		return false;
	}

	// ⚠️ **Si CHIEDE prima di rimuovere.** Dopo, il componente ha gia' applicato la sua semantica e la
	// domanda non avrebbe piu' un momento in cui essere fatta senza aver gia' scelto una risposta.
	const bool bSwap = UsesRemoveAtSwap(Component);
	Component->RemoveInstance(Index);

	// Gli array paralleli seguono la STESSA rimozione, non una equivalente: `RemoveAt` fa scalare gli indici
	// successivi, `RemoveAtSwap` porta l'ultimo nel posto liberato, e i due lasciano ordini diversi.
	auto Specchia = [bSwap, Index](auto& Array)
	{
		if (!Array.IsValidIndex(Index))
		{
			return;
		}
		if (bSwap)
		{
			Array.RemoveAtSwap(Index);
		}
		else
		{
			Array.RemoveAt(Index);
		}
	};
	Specchia(CellsOfInstance);
	Specchia(BaseScale);
	if (LastState)
	{
		Specchia(*LastState);
	}
	if (DisplayFactor)
	{
		Specchia(*DisplayFactor);
	}
	return true;
}

bool ARTHexMapActor::RepaintCells(const TArray<FRTCellId>& Ids)
{
	LastRepaintTouched = 0;
	if (!Cells || !MapAsset || MapAsset->NumCells() == 0)
	{
		// Ramo demo o board non costruita: non c'e' un asset da cui rileggere la superficie, quindi non c'e'
		// niente da riallineare — e dire di averlo fatto sarebbe la bugia peggiore delle due.
		return false;
	}

	// 🔴 **Un elenco VUOTO non e' «niente da fare», e' «non so cosa fare».** Il chiamante arriva qui perche'
	// la revisione dell'asset si e' mossa: se il registro non nomina nessuna cella, la modifica e' di quelle
	// che non passano per le celle — una transizione aggiunta o tolta, per esempio, che muove `Revision` e
	// non tocca il registro. Rispondere `true` farebbe saltare la ricostruzione che avveniva prima di questa
	// issue, in silenzio e senza che nessun conteggio se ne accorgesse.
	if (Ids.Num() == 0)
	{
		return false;
	}

	// ⚠️ **Si VALIDA tutto prima di toccare qualunque cosa**, e non e' pignoleria: uscendo a meta' si
	// lascerebbe la board in uno stato ibrido e il contatore direbbe un lavoro che il chiamante sta per
	// buttare via ricostruendo. Con due passaggi, `false` significa «non ho toccato niente».
	//
	// ⚠️ **La mappa cella→indice si costruisce una volta**, non una scansione lineare per cella: con il
	// registro pieno e una board grande sarebbero `celle-modificate × celle-della-board` confronti, cioe' il
	// costo che segue la mappa rientrato dalla finestra. `InstanceCells` e' un array e resta tale — questa
	// e' una vista locale alla chiamata, quindi non un'altra sede da tenere allineata.
	TMap<FRTCellId, int32> IndicePerCella;
	IndicePerCella.Reserve(InstanceCells.Num());
	for (int32 I = 0; I < InstanceCells.Num(); ++I)
	{
		IndicePerCella.Add(InstanceCells[I], I);
	}

	const float UseHexSize = MapAsset->HexSize;
	const float UseLayerH = GetPosedLayerHeight(); // #3632

	TArray<int32> DiscIndices;
	DiscIndices.Reserve(Ids.Num());
	for (const FRTCellId& Id : Ids)
	{
		const int32* Trovato = IndicePerCella.Find(Id);
		const FRTHexCellData* Data = MapAsset->FindCell(Id);
		if (!Trovato || !Data)
		{
			// Cella NUOVA (non ha ancora un'istanza) o SPARITA (l'istanza va tolta): nessuno dei due e' un
			// riallineamento. Il secondo caso in particolare rimuoverebbe dal disco, e il disco e' la
			// famiglia su cui si appoggia la mappatura di tutte le altre.
			return false;
		}

		// 🔴 **E la GEOMETRIA deve essere rimasta quella, non solo la cella.** `AddOrUpdateCell` alza
		// `Revision` per QUALUNQUE campo, altezza compresa: questo percorso riscrive il colore e la presenza
		// del glifo, non le pose. Una cella che si alzasse resterebbe disegnata alla quota vecchia, e —
		// peggio — un glifo montato da `AddGlyphInstanceForCell` rileggerebbe l'altezza NUOVA, lasciando i
		// due canali della stessa cella a due quote diverse. Qui si confronta la posa attesa con quella che
		// c'e': se non coincidono, non e' un riallineamento e si ricostruisce.
		FTransform Attuale;
		if (!Cells->GetInstanceTransform(*Trovato, Attuale, /*bWorldSpace=*/ true))
		{
			return false;
		}
		FVector Atteso = URTHexLibrary::AxialToWorld(Id, GetActorLocation(), UseHexSize, UseLayerH);
		Atteso.Z += static_cast<double>(Data->Height);
		if (!Attuale.GetLocation().Equals(Atteso, 1.0))
		{
			return false;
		}

		// ➕ **E il VOLUME della superficie dev'essere rimasto lo stesso** (`#2936`).
		//
		// 🔴 **Questo percorso riscrive colore e glifo, non i volumi.** Una cella che diventasse fumo passando
		// di qui avrebbe due canali su tre: tinta e anello nuovi, nessun volume. ∴ quando la presenza del volume
		// cambia non e' un riallineamento, ed e' il fallback a doverla costruire.
		//
		// ⚠️ **Si confronta la PRESENZA, non la forma**: due superfici con volumi diversi non esistono ancora —
		// il fumo e' l'unica ad averne uno — e confrontare le dimensioni qui sarebbe un ramo che nessun dato
		// puo' percorrere. Il giorno in cui una seconda superficie dichiarera' un volume, questo confronto va
		// esteso alla forma, e `SurfaceVolumeFor` e' gia' il posto da cui leggerla.
		//
		// ⚠️ `Contains` su `SurfaceVolumeCells` e' lineare, e va bene: quell'array conta le sole celle CON un
		// volume — zero su una board senza fumo — non le 7 651 della mappa. E' la stessa ragione per cui il
		// guardiano di `#2761` puo' permettersi di girare per cella.
		const bool bVolumeOra = SurfaceVolumeCells.Contains(Id);
		const bool bVolumeDopo = URTHexLibrary::SurfaceVolumeFor(Data->Surface).Y > 0.0;
		if (bVolumeOra != bVolumeDopo)
		{
			return false;
		}

		DiscIndices.Add(*Trovato);
	}

	bool bDiscDirty = false;
	bool bMappaturaMossa = false;
	bool bAnelloToccato[RTGlyphMaxRings] = { false, false, false, false };
	for (int32 K = 0; K < Ids.Num(); ++K)
	{
		const FRTCellId& Id = Ids[K];
		const int32 DiscIndex = DiscIndices[K];
		const ERTHexSurface Surface = SurfaceForCell(Id);

		// ── Il DISCO: il colore ────────────────────────────────────────────────────────────────────────
		//
		// 🔴 **Si scrive il colore GIA' ATTENUATO, e non si tocca `LastVeilState`.** Una prima stesura
		// scriveva il colore pieno e marcava l'istanza `Unwritten` perche' il velo la riscrivesse. Funzionava,
		// e costava una cosa che non si vede in un conteggio: `Unwritten` non e' ne' `Lit` ne' `Remembered`,
		// quindi `VeilInstances` non attenua e **fa uno SNAP** — una dissolvenza di `#2875` gia' in volo
		// veniva buttata via, e la cella scattava al suo valore finale proprio nel momento in cui il terreno
		// cambia sotto lo sguardo, che e' quello per cui la transizione esiste.
		//
		// Moltiplicando qui per il fattore corrente, il velo puo' saltare l'istanza — il suo stato non e'
		// cambiato — e cio' che si vede resta coerente: colore nuovo, attenuazione di prima, dissolvenza viva.
		const float Fattore = VeilDisplayFactor.IsValidIndex(DiscIndex) && VeilDisplayFactor[DiscIndex] >= 0.f
			? VeilDisplayFactor[DiscIndex]
			: 1.f;
		const FLinearColor CellColor = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(Surface));
		Cells->SetCustomDataValue(DiscIndex, 0, CellColor.R * Fattore);
		Cells->SetCustomDataValue(DiscIndex, 1, CellColor.G * Fattore);
		Cells->SetCustomDataValue(DiscIndex, 2, CellColor.B * Fattore, /*bMarkRenderStateDirty=*/ false);
		bDiscDirty = true;
		++LastRepaintTouched;

		// ── La CORONA di glifi: la presenza ────────────────────────────────────────────────────────────
		//
		// Qui la superficie non decide un colore ma se l'istanza ESISTE, e su quale anello. E' l'unico punto
		// di questa classe in cui si rimuove un'istanza singola.
		const int32 Rings = URTHexLibrary::SurfaceRingCount(Surface);
		const int32 DesiredRing = (Rings > 0 && Rings <= RTGlyphMaxRings) ? Rings - 1 : INDEX_NONE;

		int32 CurrentRing = INDEX_NONE;
		int32 CurrentIndex = INDEX_NONE;
		for (int32 R = 0; R < RTGlyphMaxRings; ++R)
		{
			const int32 Found = GlyphCells[R].IndexOfByKey(Id);
			if (Found != INDEX_NONE)
			{
				CurrentRing = R;
				CurrentIndex = Found;
				break;
			}
		}

		if (DesiredRing == CurrentRing)
		{
			// Stesso anello: il glifo non ha un colore per superficie ([D-183] gli da' la costante scura),
			// quindi non c'e' niente da riscrivere. Cambiare terreno fra due superfici con lo stesso numero
			// di anelli non muove un'istanza, ed e' corretto.
			continue;
		}

		if (CurrentRing != INDEX_NONE)
		{
			if (RemoveInstanceMirrored(SurfaceGlyphs[CurrentRing], CurrentIndex, GlyphCells[CurrentRing],
					GlyphBaseScale[CurrentRing], &LastGlyphVeilState[CurrentRing],
					&GlyphDisplayFactor[CurrentRing]))
			{
				++LastRepaintTouched;
				bMappaturaMossa = true;
				bAnelloToccato[CurrentRing] = true;
			}
		}
		if (DesiredRing != INDEX_NONE && AddGlyphInstanceForCell(DesiredRing, Id))
		{
			++LastRepaintTouched;
			bMappaturaMossa = true;
			bAnelloToccato[DesiredRing] = true;
		}
	}

	// ➕ **Solo se la mappatura si e' DAVVERO mossa** (`#2761`). Riscrivere il colore di un disco non muove
	// nessun indice, e alzare la revisione a ogni pennellata farebbe pagare il guardiano `O(N)` per una
	// modifica che non puo' averlo invalidato — cioe' esattamente il costo che questa issue toglie.
	if (bMappaturaMossa)
	{
		++VeilMappingRevision;
	}

	// Una volta sola per componente, in coda: la stessa disciplina di `VeilInstances` e di `RebuildInstances`,
	// e per la stessa ragione — `MarkRenderStateDirty` distrugge e ricrea il proxy di scena, e farlo per
	// istanza lo ricostruirebbe N volte.
	//
	// ⚠️ **Gli anelli si marcano QUI e non dentro `AddGlyphInstanceForCell`**: montare K glifi sullo stesso
	// anello ricreava il proxy K volte, che e' esattamente cio' che le altre due funzioni evitano.
	if (bDiscDirty)
	{
		Cells->MarkRenderStateDirty();
	}
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		if (bAnelloToccato[Ring] && SurfaceGlyphs[Ring])
		{
			SurfaceGlyphs[Ring]->MarkRenderStateDirty();
		}
	}
	// #3679: un glifo montato qui nasce col trattamento a zero, cioe' pieno anche su un piano traslucido.
	if (bLayerPresentationApplied)
	{
		ApplyLayerPresentation();
	}
	return true;
}

FTransform ARTHexMapActor::GlyphTransformForCell(const FRTCellId& Id, int32 Height,
	float InHexSize, float InLayerHeight) const
{
	// 🔑 **La formula del glifo, in UNA sede.** La scala NON include il fattore `0.95` di `PlanarScale`:
	// la mesh se lo porta gia' dentro (`RTGlyphOuterScale`), e applicarlo due volte stringerebbe il segno del
	// 10% senza che nessun test lo veda — resterebbe proporzionato, solo piu' piccolo del previsto.
	FVector World = URTHexLibrary::AxialToWorld(Id, GetActorLocation(), InHexSize, InLayerHeight);
	World.Z += static_cast<double>(Height);
	World.Z += RTLiftGlyph;
	const float GlyphScale = InHexSize / 50.f;
	return FTransform(FRotator::ZeroRotator, World, FVector(GlyphScale, GlyphScale, 1.f));
}

bool ARTHexMapActor::AddGlyphInstanceForCell(int32 RingIndex, const FRTCellId& Id)
{
	if (RingIndex < 0 || RingIndex >= RTGlyphMaxRings || !SurfaceGlyphs[RingIndex] || !MapAsset)
	{
		return false;
	}
	const FRTHexCellData* Data = MapAsset->FindCell(Id);
	if (!Data)
	{
		return false;
	}

	// ➕ **La posa viene dalla sede condivisa**, la stessa che `RebuildInstances` usa: due siti che posano la
	// stessa istanza con due copie della formula divergono al primo che ne cambia una, e il confronto di
	// `IncrementalRepaintEqualsFullRebuild` se ne accorgerebbe solo sui canali che guarda.
	const FTransform GlyphXf = GlyphTransformForCell(Id, Data->Height, MapAsset->HexSize, GetPosedLayerHeight());

	const int32 GlyphIndex = SurfaceGlyphs[RingIndex]->AddInstance(GlyphXf, /*bWorldSpace=*/ true);
	GlyphCells[RingIndex].Add(Id);
	GlyphBaseScale[RingIndex].Add(GlyphXf.GetScale3D());

	// ⚠️ **Gli array del velo crescono con l'istanza, e SOLO se erano gia' allineati.** Se non lo erano —
	// il velo non e' ancora passato — restano vuoti, e `VeilInstances` li inizializza da solo al primo giro:
	// riempirli qui a meta' li farebbe sembrare allineati quando non lo sono.
	if (LastGlyphVeilState[RingIndex].Num() == GlyphCells[RingIndex].Num() - 1)
	{
		LastGlyphVeilState[RingIndex].Add(RTVeilUnwritten);
	}
	if (GlyphDisplayFactor[RingIndex].Num() == GlyphCells[RingIndex].Num() - 1)
	{
		// `-1` come alla costruzione: un valore che nessun target puo' avere, quindi la prima scrittura di
		// questa istanza e' uno **snap** e non una dissolvenza. Un glifo che comparisse sfumando sarebbe la
		// decisione (i) di `#2875` violata sul canale della forma invece che su quello del colore.
		GlyphDisplayFactor[RingIndex].Add(-1.f);
	}

	const FLinearColor GlyphColor = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
	SurfaceGlyphs[RingIndex]->SetCustomDataValue(GlyphIndex, 0, GlyphColor.R);
	SurfaceGlyphs[RingIndex]->SetCustomDataValue(GlyphIndex, 1, GlyphColor.G);
	// ⚠️ `false`: chi chiama marca il componente una volta sola in coda. Vedi `RepaintCells`.
	SurfaceGlyphs[RingIndex]->SetCustomDataValue(GlyphIndex, 2, GlyphColor.B,
		/*bMarkRenderStateDirty=*/ false);
	return true;
}

void ARTHexMapActor::ApplyKnowledgeVeil(const FRTTeamKnowledge& Knowledge)
{
	// ⚠️ Si azzera SUBITO: su un'uscita anticipata un contatore lasciato al valore precedente dichiarerebbe
	// lavoro svolto proprio nel caso in cui non se n'e' fatto nessuno, ed e' la misura su cui
	// `Veil.FullScanCostIsMeasured` asserisce.
	LastVeilTouchedCells = 0;
	// ➕ **E il contatore del riallineamento per-cella, per la stessa ragione** (`#2761`): da quando il
	// ridipingere puo' avvenire in due punti — il blocco di sincronizzazione qui sotto e `VeilInstances` —
	// un contatore che sopravvivesse a un velo in cui non e' successo niente dichiarerebbe lavoro svolto
	// proprio nel giro in cui non se n'e' fatto nessuno. Sono due meta' della stessa domanda, e si azzerano
	// insieme.
	LastRepaintTouched = 0;
	if (!Cells)
	{
		return;
	}

	// ➕ **LE ISTANZE SI ALLINEANO AL DATO PRIMA DI VELARLO** (`#2894`).
	//
	// 🔴 **Il difetto che chiude**: una superficie creata in partita non cambiava niente a schermo. Il colore
	// vive nel `CustomData` delle istanze e lo scrive `RebuildInstances`, che aveva **due chiamanti, entrambi
	// `OnConstruction`** — quindi `Action.Ignite`, `Action.CreateWater` e `Hero.Muiren.MistVeil` mutavano il
	// terreno e la board restava quella del primo fotogramma.
	//
	// 🔑 **Sta QUI e non in un canale nuovo, ed e' una conseguenza di cio' che questa funzione gia' dichiara
	// di sé**: `VeilInstances` porta un `ensureMsgf` che sorveglia gli indici stantii — sa di dipendere dalla
	// freschezza delle istanze. Velare istanze costruite su un dato vecchio non e' «un po' meno aggiornato»:
	// e' la stessa precondizione, vista dall'altro lato. E il momento e' quello giusto per costruzione — il
	// presenter chiama di qui a ogni `OnTeamKnowledgeRefreshed`, cioe' due volte per turno.
	//
	// ⛔ **Non e' una cache**: `SurfaceForCell` continua a rileggere dall'asset. Qui si confronta un numero di
	// versione — `Revision`, che `AddOrUpdateCell` incrementa gia' — e la copia del dato non esiste.
	//
	// ⚠️ **Due famiglie e non `All`**, perche' sono i due canali che [D-183] accoppia: `Cells` porta il
	// COLORE, `Glyphs` la FORMA (`SurfaceRingCount`: il fumo ha un anello). Ricostruirne una sola darebbe una
	// cella col colore nuovo e il segno inciso vecchio, cioe' due canali che si contraddicono su un criterio
	// — «colore **e** forma, mai solo il colore» (`#956`) — che esiste per non dipendere dal colore.
	// Le altre cinque famiglie non dipendono dalla superficie e non si toccano.
	//
	// ➕ **E dal 2026-09-10 si riallinea PER CELLA quando l'asset sa dire quali** (`#2761`).
	//
	// 🔴 **Il difetto che chiude, misurato**: una pennellata su UNA cella costava **14** istanze su una board
	// di raggio 1 (7 celle) e **122** su una di raggio 4 (61 celle) — il costo seguiva la MAPPA, non la
	// modifica, e questo blocco lo pagava due volte per turno su ogni cambio di terreno.
	// `HexMapActor.EditCostScalesWithTheEditNotTheMap` porta i numeri di oggi.
	//
	// ⚠️ **Il fallback resta e non e' un ripiego**: `GetCellsChangedSince` risponde `false` quando l'asset
	// non sa dire cosa e' cambiato — svuotamento, sostituzione integrale, traboccamento del registro, primo
	// giro dopo un caricamento — e `RepaintCells` risponde `false` quando la modifica non e' un
	// riallineamento ma una costruzione. In entrambi i casi si ricostruiscono le due famiglie, cioe' si fa
	// esattamente cio' che si faceva prima.
	//
	// ➕ **E i canali che dipendono dalla superficie sono TRE dal `#2936`**, non due: `Cells` porta il
	// COLORE, `Glyphs` la FORMA INCISA (`SurfaceRingCount`) e `SurfaceVolumes` il VOLUME
	// (`SurfaceVolumeFor`). Il fallback li ricostruisce insieme, perche' ricostruirne una parte darebbe una
	// cella con canali che si contraddicono — colore nuovo e forma vecchia — su un criterio, «colore **e**
	// forma, mai solo il colore» (`#956`), che esiste proprio per non dipendere da uno solo.
	//
	// 🔴 **E il percorso PER CELLA non conosce il volume, ed e' dichiarato qui invece che scoperto dopo.**
	// `RepaintCells` riscrive colore e presenza del glifo; il volume no. Una cella che diventa fumo per quella
	// strada avrebbe due canali su tre. E' per questo che `RepaintCells` **rifiuta** i cambi di superficie che
	// muovono il volume e lascia decidere al fallback: vedi la sua guardia `SurfaceVolumeFor`.
	//
	// 🔑 **Chi aggiunge un quarto canale derivato dalla superficie deve toccare ENTRAMBE le strade**, o quel
	// canale resterebbe indietro su una delle due — il difetto di `#2894`, ristretto a un percorso solo e
	// quindi piu' difficile da vedere di quanto lo fosse allora.
	if (MapAsset && MapAsset->Revision != LastSyncedMapRevision)
	{
		const int32 SinceRevision = LastSyncedMapRevision;
		LastSyncedMapRevision = MapAsset->Revision;

		TArray<FRTCellId> Cambiate;
		const bool bPerCella = MapAsset->GetCellsChangedSince(SinceRevision, Cambiate)
			&& RepaintCells(Cambiate);
		if (!bPerCella)
		{
			RebuildInstances(ERTRebuildFamily::Cells | ERTRebuildFamily::Glyphs
				| ERTRebuildFamily::SurfaceVolumes);
		}

		// 🔴 **E i BORDI, che fino a `#3687` questo blocco non guardava.** Un muro abbattuto nel Blast, una
		// copertura eretta in Prep, una porta aperta: la simulazione li applica alla copia di lavoro e muove
		// `Revision`, ma le tre famiglie qui sopra sono quelle della SUPERFICIE. I pannelli restavano quelli
		// della costruzione fino alla prossima ricostruzione completa, cioe' fino al `Tab`.
		//
		// ⚠️ **Solo se cio' che i bordi dichiarano e' cambiato**, non a ogni revisione: una pennellata di superficie
		// muove `Revision` e non tocca nessun bordo, e ricostruire `EdgeFeatures` per lei riporterebbe il
		// costo a seguire la mappa invece della modifica (`HexMapActor.EditCostScalesWithTheEditNotTheMap`).
		TArray<int32> Bordi;
		CollectEdgeLayout(Bordi);
		if (Bordi != EdgeLayoutAtLastBuild)
		{
			RebuildInstances(ERTRebuildFamily::EdgeFeatures);
		}
	}

	// Appartenenza puntuale, ripetuta una volta per istanza: `TSet` e non `TArray::Contains`, che su 7 651
	// celle sarebbe quadratico. Nessuno dei due insiemi viene ITERATO — il loro ordine dipenderebbe
	// dall'hash, e qui l'ordine e' quello delle istanze.
	const TSet<FRTCellId> Visible(Knowledge.VisibleCells);
	const TSet<FRTCellId> Explored(Knowledge.ExploredCells);

	// ➕ **Il guardiano caro si paga qui, e una volta per mappatura VERIFICATA** (`#2761`). Sta DOPO il blocco
	// di riallineamento qui sopra, che e' l'unico punto capace di muovere la mappatura durante un velo.
	//
	// 🔴 **La revisione si segna solo se il guardiano PASSA, e la prima stesura la segnava sempre.** Segnarla
	// prima trasformava un allarme permanente in un allarme che suona una volta sola: al giro successivo
	// `VeilMappingRevision == LastVerifiedMappingRevision`, il controllo veniva saltato, e la mappatura
	// rimescolata sarebbe stata velata per sempre — cioe' esattamente le «celle velate SBAGLIATE» che questo
	// guardiano esiste per rendere impossibili, con la spia spenta di suo pugno.
	//
	// ⚠️ **E se fallisce si ESCE, invece di velare le famiglie superstiti.** Le mappature derivano tutte dallo
	// stesso giro di costruzione: se una e' rimescolata, le altre non sono «probabilmente a posto», sono non
	// verificate. Il costo di ripetere il controllo a ogni velo finche' dura il guasto e' voluto — e' uno
	// stato rotto, e la board ferma e' il sintomo che lo rende visibile.
	if (VeilMappingRevision != LastVerifiedMappingRevision)
	{
		if (!VerifyAllMappings())
		{
			return;
		}
		LastVerifiedMappingRevision = VeilMappingRevision;
	}

	// Il disco: l'unica famiglia con un colore PROPRIO per cella, riletto dall'asset a ogni velo invece che
	// memorizzato — un colore cachato sarebbe la seconda verita' sulla superficie.
	LastVeilTouchedCells = VeilInstances(Cells, InstanceCells, InstanceBaseScale, LastVeilState,
		&VeilDisplayFactor, Visible, Explored,
		[this](const FRTCellId& Cell, FLinearColor& Out)
		{
			Out = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(SurfaceForCell(Cell)));
			return true;
		});

	// La corona segue il disco, e con lo STESSO fattore. Una prima stesura della spec la lasciava a piena
	// luminosita': una cella NON osservata sarebbe risultata piu' appariscente di una osservata. Lo stesso
	// moltiplicatore su entrambi i canali e' anche cio' che preserva il contrasto fra glifo e superficie.
	//
	// Stessa costante scura di `RebuildInstances` ([D-183]): il glifo non ha una tavolozza propria.
	const FLinearColor GlyphBase = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		VeilInstances(SurfaceGlyphs[Ring], GlyphCells[Ring], GlyphBaseScale[Ring], LastGlyphVeilState[Ring],
			&GlyphDisplayFactor[Ring], Visible, Explored,
			[&GlyphBase](const FRTCellId&, FLinearColor& Out) { Out = GlyphBase; return true; });
	}

	// 🔴 Le TRE famiglie che la prima stesura aveva lasciato fuori, ed e' il difetto piu' grave che questa
	// funzione poteva avere: velare il disco e lasciare in piedi rilievo, volumi di blocco e pannelli di
	// bordo fa leggere muri, coperture e porte dell'INTERA board prima di averla esplorata. Sulla graybox non
	// si vede — `MakeFlatArena` non ne produce nessuno — quindi nemmeno `GetVeilCounts` se ne accorgerebbe.
	//
	// Su queste il velo NASCONDE e basta: non portano custom data per istanza, quindi `false` e nessun colore.
	// La griglia segue il disco con lo STESSO fattore, e porta custom data apposta per poterlo fare: se il
	// bordo restasse a piena luminosita' sul ricordo, una cella NON osservata avrebbe il confine piu' marcato
	// di una osservata — lo stesso rovesciamento che la corona dei glifi aveva nella prima stesura della spec.
	// ➕ #3704: il colore di base del bordo dipende dal piano della cella rispetto all'attivo (D-508), quindi si chiede per
	// cella. Su una mappa a un piano, e prima che chi guarda dica il suo piano, e' il grigio scuro di sempre.
	VeilInstances(CellBorders, BorderCells, BorderBaseScale, LastBorderVeilState, &BorderDisplayFactor,
		Visible, Explored,
		[this](const FRTCellId& Cell, FLinearColor& Out) { Out = BorderBaseColorFor(Cell); return true; });

	// ⛔ `nullptr` invece di un array di fattori: queste tre famiglie non hanno un canale colore per istanza,
	// quindi non c'e' niente da attenuare — il velo le nasconde e basta, e la scala non si interpola.
	auto SenzaColore = [](const FRTCellId&, FLinearColor&) { return false; };
	VeilInstances(Relief, ReliefCells, ReliefBaseScale, LastReliefVeilState, nullptr, Visible, Explored,
		SenzaColore);

	// ⚠️ **Il volume della superficie passa di QUI, e non e' una formalita'** (`#2936`): `VeilInstances`
	// porta a scala ZERO cio' che sta su una cella mai vista ([D-225]). Senza questa riga il fumo si
	// vedrebbe **dove la squadra non guarda** — un canale di conoscenza aperto proprio dalla feature che
	// esiste per non aprirne (`#2870`), e visibile a colpo d'occhio invece che dedotto.
	VeilInstances(SurfaceVolumes, SurfaceVolumeCells, SurfaceVolumeBaseScale, LastSurfaceVolumeVeilState,
		nullptr, Visible, Explored, SenzaColore);
	VeilInstances(Blockers, BlockerCells, BlockerBaseScale, LastBlockerVeilState, nullptr, Visible, Explored,
		SenzaColore);
	// `#3691`, [D-502] (d): un pannello di bordo si vede da un lato qualunque.
	VeilInstances(EdgeFeatures, EdgeFeatureCells, EdgeFeatureBaseScale, LastEdgeFeatureVeilState,
		nullptr, Visible, Explored, SenzaColore, &EdgeFeatureOtherCells);

	// 🔴 **La NONA famiglia, che fino a `#2731` non passava di qui** — e non per una riga dimenticata: il
	// suo sito di `AddInstance` non registrava nessuna cella, quindi non c'era niente da velare. Su una
	// mappa con `BodyFill != None` il volume solido restava visibile sotto celle mai osservate.
	//
	// Come le altre tre senza canale colore: il velo NASCONDE e basta.
	VeilInstances(StructuralBodies, BodyCells, BodyBaseScale, LastBodyVeilState,
		nullptr, Visible, Explored, SenzaColore);

	// 🔑 **Se il velo ha aperto una transizione, il `Tick` si accende.** E' l'unico punto in cui puo'
	// nascerne una: i target li decide questa funzione, e nessun altro li muove.
	//
	// ⚠️ **Non si azzera `VeilCellsInTransition` qui.** Una transizione gia' in volo su un'istanza che questo
	// refresh non ha toccato e' ancora viva: azzerare direbbe «converso» a chi sta ancora sfumando, e
	// spegnerebbe il `Tick` a meta' dissolvenza. Il conteggio VERO lo ricalcola `AdvanceVeilTransition` a
	// ogni passo; questo lo alza soltanto, e la differenza dura un fotogramma.
	if (VeilCellsInTransition > 0)
	{
		SetActorTickEnabled(true);
	}

	// #3760: c'e' qualche muro, copertura o porta RICORDATO da disegnare a linee? Le linee si ridisegnano a ogni
	// fotogramma, quindi il `Tick` resta acceso finche' ce n'e' almeno uno.
	bRememberedWallsToDraw = LastEdgeFeatureVeilState.Contains(RTVeilRemembered)
		|| LastBlockerVeilState.Contains(RTVeilRemembered);
	SetActorTickEnabled(HasAnythingToDraw());
}

bool ARTHexMapActor::GetVeilWrittenColor(int32 InstanceIndex, FLinearColor& OutColor) const
{
	if (!Cells || Cells->NumCustomDataFloats < 3)
	{
		return false;
	}
	const int32 Base = InstanceIndex * Cells->NumCustomDataFloats;
	if (!Cells->PerInstanceSMCustomData.IsValidIndex(Base + 2))
	{
		return false;
	}
	OutColor = FLinearColor(Cells->PerInstanceSMCustomData[Base],
		Cells->PerInstanceSMCustomData[Base + 1],
		Cells->PerInstanceSMCustomData[Base + 2]);
	return true;
}

void ARTHexMapActor::AdvanceVeilTransition(float DeltaSeconds)
{
	// Il conteggio si RICALCOLA, non si decrementa: un contatore mantenuto per differenza diverge al primo
	// ramo che dimentica di aggiornarlo, e questo e' il numero che decide se il `Tick` resta acceso.
	int32 InMovimento = 0;

	InMovimento += AdvanceVeilFamily(Cells, InstanceCells, LastVeilState, VeilDisplayFactor, DeltaSeconds,
		[this](const FRTCellId& Cell, FLinearColor& Out)
		{
			// Riletto dall'asset, come in `ApplyKnowledgeVeil` e per la stessa ragione: un colore cachato
			// sarebbe la seconda verita' sulla superficie. Si paga sulla sola banda che si muove.
			Out = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(SurfaceForCell(Cell)));
			return true;
		});

	const FLinearColor GlyphBase = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		InMovimento += AdvanceVeilFamily(SurfaceGlyphs[Ring], GlyphCells[Ring], LastGlyphVeilState[Ring],
			GlyphDisplayFactor[Ring], DeltaSeconds,
			[&GlyphBase](const FRTCellId&, FLinearColor& Out) { Out = GlyphBase; return true; });
	}

	InMovimento += AdvanceVeilFamily(CellBorders, BorderCells, LastBorderVeilState, BorderDisplayFactor,
		DeltaSeconds,
		[this](const FRTCellId& Cell, FLinearColor& Out) { Out = BorderBaseColorFor(Cell); return true; }); // #3704

	VeilCellsInTransition = InMovimento;
}

int32 ARTHexMapActor::AdvanceVeilFamily(UInstancedStaticMeshComponent* Component,
	const TArray<FRTCellId>& CellsOfInstance, const TArray<uint8>& LastState, TArray<float>& DisplayFactor,
	float DeltaSeconds, TFunctionRef<bool(const FRTCellId&, FLinearColor&)> BaseColor)
{
	// ⚠️ Gli array possono essere disallineati fra un `RebuildInstances` e il velo successivo: si esce invece
	// di leggere fuori. La stessa precondizione che `VeilInstances` sorveglia con un `ensure`, qui senza —
	// questa funzione gira a ogni fotogramma, e un `ensure` per frame sarebbe rumore invece che segnale.
	if (!Component || DisplayFactor.Num() != CellsOfInstance.Num() || LastState.Num() != CellsOfInstance.Num())
	{
		return 0;
	}

	int32 InMovimento = 0;
	int32 Scritte = 0;
	const ERTVeilMemory Ricordo = MemoryFor(Component); // #3760
	for (int32 I = 0; I < CellsOfInstance.Num(); ++I)
	{
		const uint8 State = LastState[I];

		// Non disegnata (o mai scritta): non c'e' niente da attenuare. Il passaggio da e verso `Hidden` e'
		// uno **scalino** per decisione — vedi il commento in `VeilInstances`.
		if (State != RTVeilLit && State != RTVeilRemembered)
		{
			continue;
		}
		// #3760: un ricordo che non si disegna non ha niente da far convergere.
		if (State == RTVeilRemembered && Ricordo == ERTVeilMemory::Hide)
		{
			continue;
		}
		const bool bPianta = State == RTVeilRemembered && Ricordo == ERTVeilMemory::Plan;

		const float Target = (State == RTVeilLit || bPianta) ? 1.f : RTVeilExploredFactor;
		const float Corrente = DisplayFactor[I];
		if (Corrente == Target)
		{
			continue; // gia' arrivata: e' il caso della stragrande maggioranza delle istanze
		}

		const float Prossimo = URTVeilTransitionLibrary::Advance(Corrente, Target, DeltaSeconds, VeilTransition);
		DisplayFactor[I] = Prossimo;

		FLinearColor Base;
		if (BaseColor(CellsOfInstance[I], Base))
		{
			if (bPianta)
			{
				Base = PlanColor();
			}
			Component->SetCustomDataValue(I, 0, Base.R * Prossimo);
			Component->SetCustomDataValue(I, 1, Base.G * Prossimo);
			Component->SetCustomDataValue(I, 2, Base.B * Prossimo, /*bMarkRenderStateDirty=*/ false);
			++Scritte;
		}

		if (Prossimo != Target)
		{
			++InMovimento;
		}
	}

	// Una volta sola, in coda, e solo se qualcosa si e' mosso: la stessa disciplina di `VeilInstances`.
	if (Scritte > 0)
	{
		Component->MarkRenderStateDirty();
	}
	return InMovimento;
}

void ARTHexMapActor::GetVeilCounts(int32& OutVisible, int32& OutExplored, int32& OutHidden) const
{
	OutVisible = 0;
	OutExplored = 0;
	OutHidden = 0;
	if (!Cells)
	{
		return;
	}

	// Si legge lo stato REALE delle istanze, non un contatore scritto da `ApplyKnowledgeVeil`: un contatore
	// proverebbe che la funzione sa contare, non che ha disegnato.
	for (int32 I = 0; I < InstanceCells.Num(); ++I)
	{
		FTransform Xf;
		if (!Cells->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
		{
			continue;
		}
		if (Xf.GetScale3D().IsNearlyZero())
		{
			// ➕ **#3760: col disco spento la cella puo' essere un RICORDO**, perche' nella pianta il ricordo non ha
			// disco: lo dice l'anello della griglia, disegnato nel colore della pianta. Si legge l'anello, cioe' cio'
			// che si vede, con la stessa disciplina di qui sopra. ⚠️ L'anello di indice `I` e' quello della stessa
			// cella — `BorderCells` e `InstanceCells` sono uno-a-uno — e lo si verifica invece di assumerlo: una
			// mappatura disallineata ricade su «mai vista», che e' la lettura piu' prudente.
			bool bPianta = false;
			FTransform Anello;
			if (CellBorders && BorderCells.IsValidIndex(I) && BorderCells[I] == InstanceCells[I]
				&& CellBorders->GetInstanceTransform(I, Anello, /*bWorldSpace=*/ true)
				&& !Anello.GetScale3D().IsNearlyZero())
			{
				const int32 BaseAnello = I * CellBorders->NumCustomDataFloats;
				const FLinearColor Pianta = PlanColor();
				bPianta = CellBorders->PerInstanceSMCustomData.IsValidIndex(BaseAnello + 2)
					&& FMath::IsNearlyEqual(CellBorders->PerInstanceSMCustomData[BaseAnello], Pianta.R, 1e-3f)
					&& FMath::IsNearlyEqual(CellBorders->PerInstanceSMCustomData[BaseAnello + 1], Pianta.G, 1e-3f)
					&& FMath::IsNearlyEqual(CellBorders->PerInstanceSMCustomData[BaseAnello + 2], Pianta.B, 1e-3f);
			}
			++(bPianta ? OutExplored : OutHidden);
			continue;
		}
		// Fra accesa e ricordata distingue solo il COLORE: entrambe sono disegnate a scala piena.
		const FLinearColor Full = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(SurfaceForCell(InstanceCells[I])));
		const int32 Base = I * Cells->NumCustomDataFloats;
		const float Written = Cells->PerInstanceSMCustomData.IsValidIndex(Base)
			? Cells->PerInstanceSMCustomData[Base] : Full.R;
		// Soglia a meta' strada fra pieno e velato: qualunque valore sotto e' un ricordo, e la distanza fra i
		// due stati e' grande abbastanza da non dipendere dalla precisione del float.
		const float Midpoint = Full.R * (1.f + ARTHexMapActor::RTVeilExploredFactor) * 0.5f;
		if (Full.R > KINDA_SMALL_NUMBER && Written < Midpoint)
		{
			++OutExplored;
		}
		else
		{
			++OutVisible;
		}
	}
}


void ARTHexMapActor::GetAuxiliaryVeilCounts(int32& OutDrawn, int32& OutHidden) const
{
	OutDrawn = 0;
	OutHidden = 0;

	// Si legge lo stato REALE delle istanze, non un contatore scritto dal velo: un contatore proverebbe che
	// la funzione sa contare, non che ha nascosto.
	auto Count = [&OutDrawn, &OutHidden](const UInstancedStaticMeshComponent* Component, int32 Mapped)
	{
		if (!Component)
		{
			return;
		}
		const int32 Num = FMath::Min(Component->GetInstanceCount(), Mapped);
		for (int32 I = 0; I < Num; ++I)
		{
			FTransform Xf;
			if (!Component->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true))
			{
				continue;
			}
			if (Xf.GetScale3D().IsNearlyZero()) { ++OutHidden; } else { ++OutDrawn; }
		}
	};

	Count(Relief, ReliefCells.Num());
	Count(SurfaceVolumes, SurfaceVolumeCells.Num());
	Count(Blockers, BlockerCells.Num());
	Count(EdgeFeatures, EdgeFeatureCells.Num());
	// ➕ Il corpo strutturale entra qui da `#2731`: prima non compariva in **nessun** oracolo, ed e' il
	// motivo per cui il leak e' vissuto fino a una code review invece che fino al primo test rosso.
	Count(StructuralBodies, BodyCells.Num());
}

int32 ARTHexMapActor::PlanGhostInstanceCount() const
{
	return PlanGhosts ? PlanGhosts->GetInstanceCount() : 0;
}
