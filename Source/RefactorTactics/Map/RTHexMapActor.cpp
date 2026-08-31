#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTArenaCriteriaLibrary.h"
#include "Map/RTGeometryGrammar.h" // ToPolyline: i muri interni si disegnano dal loro segmento (#712)
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h" // anteprima di pianificazione (presentazione, non logica)
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
#if WITH_EDITOR
#include "ScopedTransaction.h"
#include "Components/LineBatchComponent.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"
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
 * l'altra non e' piu' possibile. Presidiato da `RefactorTactics.HexMapActor.ProceduralMeshSectionsHaveAMaterialSlot`.
 */
static const FName RTProceduralMeshSlotName(TEXT("Default"));

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
	constexpr float RTLiftGlyph = RTCellTopZ + 0.3f;    // glifo di superficie (#956): inciso nella faccia,
	                                                    // sotto il contorno, sopra il disco
	// Le coordinate incise (#1920): sopra superficie/griglia/glifo (leggibili), sotto marker e anteprima —
	// un ausilio d'autoraggio non deve coprire un canale di lettura della partita. Vive qui e non come
	// `constexpr` locale in `RebuildCoordinateLabels`, oltre mille righe piu' in basso, per lo stesso motivo per
	// cui le altre quattro quote vivono qui: un numero ricopiato lontano dagli altri e' quello che nessun
	// compilatore rilega quando la gerarchia cambia (vedi `RTMapVisuals.h`).
	constexpr float RTLiftCoordinateLabel = RTCellTopZ + 1.0f;
	constexpr float RTLiftMarker  = RTCellTopZ + 1.5f;  // blocca-movimento / blocca-vista
	constexpr float RTLiftPreview = RTCellTopZ + 2.5f;  // anteprima di pianificazione (sopra a tutto)

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
	const float UseLayerH = Map->LayerHeight;

	// La mesh nasce alta `2 · RTCellPrismRadius`: la scala Z che le da' `Fraction · H` e' quel rapporto.
	// Scritta cosi' invece che con un letterale, resta vera se una mappa cambia `LayerHeight`.
	const float PlanarScale = UseHexSize / RTCellPrismRadius * 0.95f;

	for (const FRTHexCellData& Cell : Map->Cells)
	{
		const bool bVisible = Visible.Contains(Cell.Id);
		const bool bKnown = bVisible || Explored.Contains(Cell.Id);
		const int32 Which = bVisible
			? RTKnowledgeVolumeLit
			: (bKnown ? RTKnowledgeVolumeRemembered : RTKnowledgeVolumeHidden);

		const float Fraction = RTKnowledgeVolumeFractions[Which];
		const float FlatScale = Fraction * UseLayerH / (2.f * RTCellPrismRadius);

		FVector World = URTHexLibrary::AxialToWorld(Cell.Id, GetActorLocation(), UseHexSize, UseLayerH);
		World.Z += static_cast<double>(Cell.Height);
		KnowledgeVolumes->AddInstance(
			FTransform(FRotator::ZeroRotator, World, FVector(PlanarScale, PlanarScale, FlatScale)),
			/*bWorldSpace=*/ true);
	}
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
	const float UseLayerH = Map->LayerHeight;
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
		else if (Vicino(RTKnowledgeVolumeHidden))      { ++OutHidden; }
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
	Cells->NumCustomDataFloats = 3;

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

	// Volumi delle regole di blocco: stessa disciplina del rilievo — nessuna collisione, nessuna ombra.
	Blockers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Blockers"));
	Blockers->SetupAttachment(Cells);
	Blockers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blockers->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blockers->CastShadow = false;
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
		SurfaceGlyphs[Ring]->NumCustomDataFloats = 3;
	}

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
	CellBorders->NumCustomDataFloats = 3;

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

void ARTHexMapActor::BindToMapAsset()
{
	if (BoundAsset.Get() == MapAsset)
	{
		return; // gia' iscritti all'asset giusto
	}
	UnbindFromMapAsset();
	if (MapAsset)
	{
		MapChangedHandle = MapAsset->OnMapChanged.AddUObject(this, &ARTHexMapActor::RebuildInstances);
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
	PreviewPath = Path;
	SetActorTickEnabled(HasAnythingToDraw());
}

bool ARTHexMapActor::HasAnythingToDraw() const
{
	return bCellOverlay
		|| bHoveredValid
		|| PreviewPath.Num() > 0
		|| PreviewHitCells.Num() > 0
		|| PreviewReachable.Num() > 0;
}

void ARTHexMapActor::SetPreviewHitCells(const TArray<FRTCellId>& HitCells, const TArray<FRTCellId>& AllyCells)
{
	// Si copia e basta: le celle arrivano gia' calcolate da URTHexCombatLibrary::HexHitCells. Rifiltrarle qui
	// sarebbe un secondo calcolo, e due calcoli della stessa cosa prima o poi divergono (invariante #1).
	PreviewHitCells = HitCells;
	PreviewAllyHitCells = AllyCells;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewReachableCells(const TArray<FRTCellId>& ReachableCells)
{
	PreviewReachable = ReachableCells;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCellOverlay)
	{
		DrawCellOverlay(); // prima: l'anteprima di pianificazione deve restare leggibile SOPRA
	}
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
			DrawDebugLine(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color,
				/*bPersistentLines=*/ false, /*LifeTime=*/ -1.f, Depth, /*Thickness=*/ 3.f);
		}
	};

	// Ordine di disegno: dal meno al piu' urgente, cosi' l'informazione critica resta leggibile sopra.
	// 1) dove POSSO andare  2) dove VADO  3) chi COLPISCO  4) cosa sto indicando.

	// Celle raggiungibili: contorno piccolo e tenue. Fa vedere il budget mordere (il fango accorcia il raggio)
	// senza coprire il resto: e' contesto, non una decisione presa.
	for (const FRTCellId& Cell : PreviewReachable)
	{
		DrawCellOutline(Cell, FColor(60, 110, 90), 0.52f);
	}

	// Traccia del percorso: contorno ciano su ogni cella + segmento fra i centri consecutivi.
	for (int32 I = 0; I < PreviewPath.Num(); ++I)
	{
		DrawCellOutline(PreviewPath[I], FColor(40, 220, 220), 0.72f);
		if (I > 0)
		{
			const FVector A = URTHexLibrary::AxialToWorld(PreviewPath[I - 1], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPath[I - 1]) + RTLiftPreview + 1.5f);
			const FVector B = URTHexLibrary::AxialToWorld(PreviewPath[I], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPath[I]) + RTLiftPreview + 1.5f);
			DrawDebugLine(World, A, B, FColor(40, 220, 220), false, -1.f, 0, 4.f);
		}
	}

	// Zona colpita dall'attacco pianificato. Rosso = minaccia; ARANCIONE = c'e' un alleato dentro, e va visto
	// PRIMA del lock-in: che il fuoco amico faccia danno e' gia' verificato dai test, che il giocatore lo sappia
	// mentre puo' ancora cambiare idea no — quella meta' esiste solo qui.
	for (const FRTCellId& Cell : PreviewHitCells)
	{
		const bool bAlly = PreviewAllyHitCells.Contains(Cell);
		DrawCellOutline(Cell, bAlly ? FColor(255, 150, 30) : FColor(230, 60, 50), bAlly ? 0.80f : 0.68f,
			/*bThroughUnits=*/ true);
	}

	// Cella sotto il cursore: disegnata per ultima e piu' larga, cosi' resta leggibile sopra la traccia.
	// Anche questa attraversa le unita': si punta un bersaglio molto piu' spesso di una cella vuota, e
	// l'evidenziazione che sparisce proprio quando indichi qualcuno e' peggio che non averla.
	if (bHoveredValid)
	{
		DrawCellOutline(HoveredCell, FColor::Yellow, 0.88f, /*bThroughUnits=*/ true);
	}
}

const URTHexMapAsset* ARTHexMapActor::GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	// Dimensioni dall'asset AUTOREVOLE; se manca valgono quelle dell'actor (graybox demo).
	const URTHexMapAsset* Map = MapAsset;
	OutOrigin = GetActorLocation();
	OutHexSize = Map ? Map->HexSize : HexSize;
	OutLayerHeight = Map ? Map->LayerHeight : LayerHeight;
	return Map;
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

void ARTHexMapActor::RebuildInstances()
{
	if (!Cells)
	{
		return;
	}

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

	Cells->ClearInstances();
	InstanceCells.Reset();
	InstanceBaseScale.Reset();
	// Lo stato del velo si azzera con gli indici a cui si riferisce: sopravvivergli significherebbe saltare
	// istanze che nel frattempo sono diventate altre celle.
	LastVeilState.Reset();
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		GlyphCells[Ring].Reset();
		GlyphBaseScale[Ring].Reset();
		LastGlyphVeilState[Ring].Reset();
	}
	ReliefCells.Reset();
	ReliefBaseScale.Reset();
	LastReliefVeilState.Reset();
	BlockerCells.Reset();
	BlockerBaseScale.Reset();
	LastBlockerVeilState.Reset();
	EdgeFeatureCells.Reset();
	EdgeFeatureBaseScale.Reset();
	LastEdgeFeatureVeilState.Reset();
	BorderCells.Reset();
	BorderBaseScale.Reset();
	LastBorderVeilState.Reset();

	// I glifi si ricostruiscono con le celle: `RebuildInstances` gira a ogni pennellata, e istanze vecchie
	// resterebbero appese a superfici che nel frattempo sono cambiate.
	for (int32 Ring = 0; Ring < RTGlyphMaxRings; ++Ring)
	{
		if (!SurfaceGlyphs[Ring]) { continue; }
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
	if (Relief)
	{
		if (CellShape != nullptr) { Relief->SetStaticMesh(CellShape); }
		Relief->ClearInstances();
	}
	if (Blockers)
	{
		if (CellShape != nullptr) { Blockers->SetStaticMesh(CellShape); }
		Blockers->ClearInstances();
	}
	if (EdgeFeatures)
	{
		// La mesh dei bordi NON segue `CellMesh`: quella e' la mesh della cella, e un pannello non e' una cella.
		EdgeFeatures->ClearInstances();
	}

	// La griglia: mesh propria e generata, come i glifi. NON segue `CellMesh` — quella e' il pieno della
	// cella, e un contorno non e' un pieno.
	if (CellBorders)
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

	// Sorgente celle: l'asset se popolato, altrimenti un graybox demo (esagono pieno di raggio DemoRadius).
	const float UseHexSize = MapAsset ? MapAsset->HexSize : HexSize;
	const float UseLayerH = MapAsset ? MapAsset->LayerHeight : LayerHeight;

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
		if (CellBorders)
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
			const FLinearColor BorderColor = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
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
		if (Rings > 0 && Rings <= RTGlyphMaxRings && SurfaceGlyphs[Rings - 1])
		{
			// La scala NON include il fattore 0.95 di `PlanarScale`: la mesh se lo porta gia' dentro
			// (`RTGlyphOuterScale`), e applicarlo due volte stringerebbe il segno del 10% senza che nessun
			// test lo veda — resterebbe proporzionato, solo piu' piccolo del previsto.
			const float GlyphScale = UseHexSize / 50.f;
			FVector GlyphCenter = World;
			GlyphCenter.Z += RTLiftGlyph;
			const FTransform GlyphXf(FRotator::ZeroRotator, GlyphCenter,
				FVector(GlyphScale, GlyphScale, 1.f));
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
		if (Relief && ReliefHeight > 0.f)
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

		// Volumi delle due regole. Sono INDIPENDENTI: una cella puo' averne una, l'altra o entrambe, e in
		// quest'ultimo caso si vedono tutte e due — la lastra larga e bassa attorno alla colonna stretta e
		// alta. Confonderle e' l'errore piu' frequente su questa mappa: una cella che blocca la vista si
		// ATTRAVERSA, ed e' cio' che serve a una rotta coperta ma percorribile.
		if (Blockers)
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
		if (EdgeFeatures && EdgeSources[I])
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
	if (EdgeFeatures && MapAsset)
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
			EdgeFeatureBaseScale.Add(WallXf.GetScale3D());
		}
	}

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
	// Stima per il reserve, non un limite: tre run, poche cifre ciascuna nel caso comune (non il caso
	// peggiore da dieci cifre che `NothingLeavesTheHexagonAtTheTrueWorstCase` misura), e ogni carattere del
	// set chiuso costa da 1 segmento (virgola) a 7 ('8'). Serve solo a risparmiare le prime riallocazioni
	// quando si trascina il pennello (RebuildInstances gira a ogni cella toccata) — se la stima e' bassa
	// `TArray` continua comunque a crescere.
	Lines.Reserve(MapAsset->Cells.Num() * 90);
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
// ricalcolo ottiene gia'. `NumCustomDataFloats` resta **3**.

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

int32 ARTHexMapActor::VeilInstances(UInstancedStaticMeshComponent* Component,
	const TArray<FRTCellId>& CellsOfInstance, const TArray<FVector>& BaseScale, TArray<uint8>& LastState,
	const TSet<FRTCellId>& Visible, const TSet<FRTCellId>& Explored,
	TFunctionRef<bool(const FRTCellId&, FLinearColor&)> BaseColor)
{
	if (!Component || CellsOfInstance.Num() == 0)
	{
		return 0;
	}

	// La precondizione DICHIARATA: la mappatura e' stata DERIVATA da `RebuildInstances`, e una ricostruzione
	// passata di mezzo lascia indici stantii. L'esito non e' un crash ma celle velate SBAGLIATE — il difetto
	// che si legge come «problema grafico» per settimane, finche' qualcuno non lo misura.
	if (!ensureMsgf(Component->GetInstanceCount() == CellsOfInstance.Num(),
		TEXT("ApplyKnowledgeVeil: %s ha %d istanze contro %d celle mappate — RebuildInstances e' passato di mezzo"),
		*Component->GetName(), Component->GetInstanceCount(), CellsOfInstance.Num()))
	{
		return 0;
	}

	// Lo stato precedente, per saltare cio' che non cambia. La prima volta e' tutto `Unwritten`, quindi il
	// primo velo tocca ogni istanza; dal secondo in poi tocca solo il bordo del cono.
	if (LastState.Num() != CellsOfInstance.Num())
	{
		LastState.Init(RTVeilUnwritten, CellsOfInstance.Num());
	}

	int32 Toccate = 0;
	for (int32 I = 0; I < CellsOfInstance.Num(); ++I)
	{
		const FRTCellId& Cell = CellsOfInstance[I];
		const bool bVisible = Visible.Contains(Cell);
		const bool bKnown = bVisible || Explored.Contains(Cell);
		const uint8 State = bVisible ? RTVeilLit : (bKnown ? RTVeilRemembered : RTVeilHidden);

		// 🔴 Il salto. `UpdateInstanceTransform` e `SetCustomDataValue` costano anche quando riscrivono lo
		// stesso valore, ed e' li' che finiva la misura del velo ingenuo — 2,2 s su arena piena.
		if (LastState[I] == State)
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
		Xf.SetScale3D(bKnown ? Full : FVector::ZeroVector);
		Component->UpdateInstanceTransform(I, Xf, /*bWorldSpace=*/ true, /*bMarkRenderStateDirty=*/ false);

		LastState[I] = State;
		++Toccate;

		FLinearColor Base;
		if (!bKnown || !BaseColor(Cell, Base))
		{
			// Niente colore da scrivere: su cio' che non si disegna, e sulle famiglie che non hanno un canale
			// colore per istanza — dove ricordato e osservato restano indistinguibili.
			continue;
		}
		const float Factor = bVisible ? 1.f : RTVeilExploredFactor;
		Component->SetCustomDataValue(I, 0, Base.R * Factor);
		Component->SetCustomDataValue(I, 1, Base.G * Factor);
		Component->SetCustomDataValue(I, 2, Base.B * Factor, /*bMarkRenderStateDirty=*/ false);
	}

	// Una volta sola, in coda, e SOLO se qualcosa e' cambiato: marcarlo a ogni canale ricostruirebbe il
	// buffer 3N volte, e marcarlo a vuoto ricostruirebbe tutto per niente.
	if (Toccate > 0)
	{
		Component->MarkRenderStateDirty();
	}
	return Toccate;
}

void ARTHexMapActor::ApplyKnowledgeVeil(const FRTTeamKnowledge& Knowledge)
{
	// ⚠️ Si azzera SUBITO: su un'uscita anticipata un contatore lasciato al valore precedente dichiarerebbe
	// lavoro svolto proprio nel caso in cui non se n'e' fatto nessuno, ed e' la misura su cui
	// `Veil.FullScanCostIsMeasured` asserisce.
	LastVeilTouchedCells = 0;
	if (!Cells)
	{
		return;
	}

	// Appartenenza puntuale, ripetuta una volta per istanza: `TSet` e non `TArray::Contains`, che su 7 651
	// celle sarebbe quadratico. Nessuno dei due insiemi viene ITERATO — il loro ordine dipenderebbe
	// dall'hash, e qui l'ordine e' quello delle istanze.
	const TSet<FRTCellId> Visible(Knowledge.VisibleCells);
	const TSet<FRTCellId> Explored(Knowledge.ExploredCells);

	// Il disco: l'unica famiglia con un colore PROPRIO per cella, riletto dall'asset a ogni velo invece che
	// memorizzato — un colore cachato sarebbe la seconda verita' sulla superficie.
	LastVeilTouchedCells = VeilInstances(Cells, InstanceCells, InstanceBaseScale, LastVeilState,
		Visible, Explored,
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
			Visible, Explored,
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
	const FLinearColor BorderBase = FLinearColor::FromSRGBColor(FColor(25, 25, 25));
	VeilInstances(CellBorders, BorderCells, BorderBaseScale, LastBorderVeilState, Visible, Explored,
		[&BorderBase](const FRTCellId&, FLinearColor& Out) { Out = BorderBase; return true; });

	auto SenzaColore = [](const FRTCellId&, FLinearColor&) { return false; };
	VeilInstances(Relief, ReliefCells, ReliefBaseScale, LastReliefVeilState, Visible, Explored, SenzaColore);
	VeilInstances(Blockers, BlockerCells, BlockerBaseScale, LastBlockerVeilState, Visible, Explored, SenzaColore);
	VeilInstances(EdgeFeatures, EdgeFeatureCells, EdgeFeatureBaseScale, LastEdgeFeatureVeilState,
		Visible, Explored, SenzaColore);
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
			++OutHidden;
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
	Count(Blockers, BlockerCells.Num());
	Count(EdgeFeatures, EdgeFeatureCells.Num());
}
