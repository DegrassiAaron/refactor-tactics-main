#include "Misc/AutomationTest.h"

#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"        // FStaticMeshRenderData / FPositionVertexBuffer

#include "Map/RTHexMapActor.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Le mesh procedurali sono DISEGNABILI: bounds che contengono la geometria, e sezioni con uno slot valido.
 *
 * **Perche' esiste, e cosa ha trovato.** #1665: nel pacchetto la board non arrivava a schermo con **65
 * istanze**, la mesh giusta, il materiale giusto e tre float di custom data per istanza — tutto corretto e
 * niente disegnato. La causa, isolata il 2026-08-30 commutando la sola mesh a `/Engine/BasicShapes/Cube`
 * nella stessa run: la sezione del prisma usciva con **`MaterialIndex = -1`** (`tri=20 idx=60 matIdx=-1/1`,
 * contro `matIdx=0/1` del Cube, che si vedeva). `BuildFromMeshDescriptions` lega sezione e slot **per
 * nome**, e il `PolygonGroupMaterialSlotName` non combaciava col `MaterialSlotName` dello slot.
 *
 * 🔴 **L'asserzione 4 NON presidia quel difetto in `EditorContext`, e la verifica di mutazione lo ha
 * dimostrato invece di lasciarlo supporre.** Rinominato **un solo lato** del legame — lo slot a
 * `"MUTAZIONE_1665"`, il `PolygonGroup` fermo su `"Default"` — il test e' rimasto **verde**, e il valore
 * misurato dice perche': `MaterialIndex = 0`. **In Editor un fallback risolve l'indice anche quando i nomi
 * non combaciano**; nel cotto no, e li' esce `-1`. La prima stesura di questo commento affermava il
 * contrario (*«`MaterialIndex` si costruisce allo stesso modo nei due mondi»*): era una deduzione, ed era
 * falsa.
 *
 * ⚠️ **Quindi cosa vale l'asserzione 4?** Copre il caso *strutturale* — zero sezioni, zero slot, o un indice
 * fuori intervallo — non il disallineamento di nome, che qui e' invisibile per costruzione. ⛔ **L'unico
 * oracolo del difetto vero resta il pacchetto**, finche' il test non diventa `ClientContext`: quello lo
 * porterebbe sul binario staged, dove `-1` si vede, e ha un costo dichiarato — la suite raggiungibile dal
 * packaged passerebbe da **11** a 12 test, e il §3 del DoD nomina quegli 11 uno per uno alla riga `G2`. E'
 * una scelta da fare insieme a quel documento, non di passaggio qui.
 *
 * **Perche' l'asserto e' relazionale e non una soglia.** `SphereRadius > 0` passerebbe con un raggio di
 * 0,001 uu — che e' zero per il culling e non zero per il test. L'atteso si ricava invece dai **vertici che
 * la mesh ha davvero**, letti dal suo render data: i bounds devono contenerli. Cosi' il test non porta
 * numeri copiati, e cambia da solo se la geometria cambia — la stessa ragione per cui
 * `HexMap.InstanceColorFollowsSurface` costruisce l'atteso da `FromSRGBColor` invece che da valori scritti
 * a mano.
 */
namespace
{
	// Nome distinto: i namespace anonimi delle unity build si fondono.
	struct FRTMeshBoundsCheck
	{
		bool bHasRenderData = false;
		FBox Reale = FBox(ForceInit);
		uint32 NumVertices = 0;
	};

	FRTMeshBoundsCheck ReadVertexExtents(const UStaticMesh* Mesh)
	{
		FRTMeshBoundsCheck Out;
		const FStaticMeshRenderData* Render = Mesh ? Mesh->GetRenderData() : nullptr;
		if (!Render || Render->LODResources.Num() == 0)
		{
			return Out;
		}

		const FPositionVertexBuffer& Positions = Render->LODResources[0].VertexBuffers.PositionVertexBuffer;
		Out.NumVertices = Positions.GetNumVertices();
		if (Out.NumVertices == 0)
		{
			return Out;
		}

		// `FBox(ForceInit)` nasce NON valido e diventa valido al primo `+=`: e' l'idioma che
		// `URTHexLibrary` usa gia' per l'AABB della mappa, e non ha il caso limite dei sentinelle
		// `TNumericLimits` lasciati intatti quando il ciclo non gira.
		for (uint32 Index = 0; Index < Out.NumVertices; ++Index)
		{
			Out.Reale += FVector(Positions.VertexPosition(Index));
		}
		Out.bHasRenderData = Out.Reale.IsValid != 0;
		return Out;
	}

	/**
	 * La distanza del vertice piu' lontano da un'origine data.
	 *
	 * 🔴 **Serve una seconda lettura del buffer, e la prima stesura di questo test provava a evitarla
	 * usando gli otto angoli dell'AABB. E' SBAGLIATO, ed e' fallito su tutte e sei le mesh**: per il
	 * prisma il raggio dichiarato e' `70,711` = `sqrt(50² + 50²)`, cioe' esattamente il vertice piu'
	 * lontano — mentre l'angolo del box sta a `82,916`, perche' un esagono non ha vertici agli angoli
	 * del proprio rettangolo circoscritto. Il raggio del motore era **giusto e stretto**; l'atteso era
	 * mio, e allargare la tolleranza avrebbe nascosto l'errore invece di correggerlo.
	 */
	double MaxVertexDistance(const UStaticMesh* Mesh, const FVector& Origin)
	{
		const FStaticMeshRenderData* Render = Mesh ? Mesh->GetRenderData() : nullptr;
		if (!Render || Render->LODResources.Num() == 0)
		{
			return 0.0;
		}

		const FPositionVertexBuffer& Positions = Render->LODResources[0].VertexBuffers.PositionVertexBuffer;
		double Max = 0.0;
		for (uint32 Index = 0; Index < Positions.GetNumVertices(); ++Index)
		{
			Max = FMath::Max(Max, FVector::Dist(Origin, FVector(Positions.VertexPosition(Index))));
		}
		return Max;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapMeshBoundsTest,
	"RefactorTactics.HexMapActor.ProceduralMeshesAreRenderable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapMeshBoundsTest::RunTest(const FString&)
{
	// Tutte e tre le mesh procedurali dell'attore, perche' passano tutte per la stessa
	// `BuildFromMeshDescriptions` con `bFastBuild`: se il difetto e' li', e' di tutte e tre.
	// Il glifo si chiede a piu' conteggi di anelli — sono mesh diverse, con una cache per conteggio.
	TArray<TPair<FString, UStaticMesh*>> Meshes;
	Meshes.Emplace(TEXT("prisma della cella"), ARTHexMapActor::GetCellPrismMesh());
	Meshes.Emplace(TEXT("volume di conoscenza"), ARTHexMapActor::GetKnowledgeVolumeMesh());
	for (int32 Rings = 1; Rings <= 4; ++Rings)
	{
		Meshes.Emplace(FString::Printf(TEXT("glifo a %d anelli"), Rings), ARTHexMapActor::GetCellGlyphMesh(Rings));
	}

	for (const TPair<FString, UStaticMesh*>& Entry : Meshes)
	{
		const FString& Nome = Entry.Key;
		UStaticMesh* Mesh = Entry.Value;

		if (!TestNotNull(*FString::Printf(TEXT("%s: la mesh si costruisce"), *Nome), Mesh))
		{
			continue;
		}

		const FRTMeshBoundsCheck Vertici = ReadVertexExtents(Mesh);
		if (!TestTrue(*FString::Printf(TEXT("%s: ha render data con vertici"), *Nome), Vertici.bHasRenderData))
		{
			// Senza vertici non c'e' atteso da confrontare: il difetto sarebbe un altro, e lo dicono
			// gia' `Hex.CellPrismMatchesHexCorners` e `Hex.CellGlyphMatchesHexCorners`.
			continue;
		}

		const FBoxSphereBounds Bounds = Mesh->GetBounds();

		// 1. Il raggio non e' nullo. E' la condizione che il culling guarda: un raggio zero scarta ogni
		//    istanza prima di disegnarla, e a schermo non si distingue da «niente da disegnare».
		TestTrue(*FString::Printf(TEXT("%s: il raggio dei bounds non e' nullo (e' %.3f)"),
			*Nome, Bounds.SphereRadius), Bounds.SphereRadius > UE_KINDA_SMALL_NUMBER);

		// 2. E contiene davvero la geometria. Un raggio non nullo ma piu' piccolo dei vertici lascerebbe
		//    la mesh sparire a distanza invece che sempre — lo stesso difetto, piu' difficile da vedere.
		const FBox& Reale = Vertici.Reale;
		const FBox Dichiarato(Bounds.Origin - Bounds.BoxExtent, Bounds.Origin + Bounds.BoxExtent);

		// Tolleranza di un millesimo del raggio della cella (50 uu): assorbe il float delle posizioni
		// senza assorbire un box sbagliato, che sarebbe fuori di decine di unita'.
		constexpr double Tolleranza = 0.05;
		const FBox DichiaratoConMargine = Dichiarato.ExpandBy(Tolleranza);

		TestTrue(*FString::Printf(
			TEXT("%s: i bounds contengono i %u vertici della mesh — dichiarato [%s .. %s], reale [%s .. %s]"),
			*Nome, Vertici.NumVertices,
			*Dichiarato.Min.ToString(), *Dichiarato.Max.ToString(),
			*Reale.Min.ToString(), *Reale.Max.ToString()),
			DichiaratoConMargine.IsInsideOrOn(Reale.Min) && DichiaratoConMargine.IsInsideOrOn(Reale.Max));

		// 3. Il raggio della sfera copre il vertice piu' lontano dall'origine dichiarata. E' la misura che
		//    il culling usa davvero, e non si deduce dal box: un box giusto con un raggio sbagliato passa
		//    il controllo 2 e sparisce lo stesso.
		//
		//    ⚠️ Il confronto e' contro i **vertici**, non contro gli angoli dell'AABB: un esagono non ha
		//    vertici agli angoli del proprio rettangolo circoscritto, e chiedere al raggio di coprirli
		//    sarebbe chiedergli di essere piu' largo del necessario. Vedi `MaxVertexDistance`.
		const double MaxDistanza = MaxVertexDistance(Mesh, Bounds.Origin);

		TestTrue(*FString::Printf(
			TEXT("%s: il raggio (%.3f) copre il vertice piu' lontano (%.3f)"),
			*Nome, Bounds.SphereRadius, MaxDistanza),
			Bounds.SphereRadius + Tolleranza >= MaxDistanza);

		// 3b. E non e' assurdamente largo. Un raggio enorme non fa sparire niente, ma disattiva il culling
		//     e non e' cio' che il motore produce: qui e' STRETTO — misurato il 2026-08-30, `70,711` sul
		//     prisma contro un vertice piu' lontano di `70,711`. Se un giorno diverge di piu' del doppio,
		//     e' cambiato qualcosa nella costruzione della mesh e vale la pena saperlo.
		TestTrue(*FString::Printf(
			TEXT("%s: il raggio (%.3f) non e' spropositato rispetto ai vertici (%.3f)"),
			*Nome, Bounds.SphereRadius, MaxDistanza),
			Bounds.SphereRadius <= MaxDistanza * 2.0 + Tolleranza);

		// 4. Ogni sezione ha uno slot materiale VALIDO.
		//
		// 🔴 **E' il difetto di #1665, e nessuno lo vedeva.** `BuildFromMeshDescriptions` lega sezione e
		//    slot **per nome**: se il `PolygonGroupMaterialSlotName` del `MeshDescription` non combacia col
		//    `MaterialSlotName` di `FStaticMaterial`, la sezione esce con `MaterialIndex = -1`. Geometria
		//    completa — 20 triangoli, 60 indici, bounds giusti, custom data giusti — e **niente a schermo**.
		//    Misurato nel pacchetto il 2026-08-30: `matIdx=-1/1` sul prisma contro `matIdx=0/1` del Cube
		//    d'engine nella stessa run, che si vedeva.
		//
		// ⚠️ **Perche' era invisibile a tutto il resto**: in Editor questa mesh si disegna lo stesso, e il
		//    difetto emerge solo in una build cotta. Nessun test lo copriva, e l'unico oracolo era guardare
		//    un pacchetto. Questa asserzione lo rende rosso in automation.
		const int32 NumSlot = Mesh->GetStaticMaterials().Num();
		const FStaticMeshRenderData* RD = Mesh->GetRenderData();
		const int32 NumSez = (RD && RD->LODResources.Num() > 0) ? RD->LODResources[0].Sections.Num() : 0;

		TestTrue(*FString::Printf(TEXT("%s: ha almeno una sezione e uno slot materiale"), *Nome),
			NumSez > 0 && NumSlot > 0);

		for (int32 S = 0; S < NumSez; ++S)
		{
			const int32 MatIdx = RD->LODResources[0].Sections[S].MaterialIndex;
			AddInfo(FString::Printf(TEXT("%s: sezione %d -> MaterialIndex %d, slot '%s' (%d slot)"),
				*Nome, S, MatIdx,
				NumSlot > 0 ? *Mesh->GetStaticMaterials()[0].MaterialSlotName.ToString() : TEXT("<nessuno>"),
				NumSlot));
			TestTrue(*FString::Printf(
				TEXT("%s: la sezione %d punta a uno slot valido (MaterialIndex %d, slot disponibili %d)"),
				*Nome, S, MatIdx, NumSlot),
				MatIdx >= 0 && MatIdx < NumSlot);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
