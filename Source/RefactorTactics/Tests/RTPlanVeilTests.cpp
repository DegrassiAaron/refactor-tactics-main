// LA PIANTA: il terreno ricordato si disegna come pianta, non attenuato (#3760).
//
// L'autore ha scelto «Pianta» (2026-10-14): *«la cella ricordata diventa un disegno a linee monocromo, quasi
// olografico. Si capisce la forma, non che e' il presente.»* Sulle tavole ha scelto poi il ciano chiaro. Nella seduta
// `U89` (2026-10-15) ha respinto la prima resa, e ha scelto: un fondo tenue color pianta, perche' il solo anello sembrava
// «mai visto»; la dissolvenza fra osservato e ricordato, perche' il cambio secco disturbava; i muri ricordati come
// pannelli traslucidi; e il rilievo del costo piu' chiaro, perche' le celle viste che lo portano sembravano nere. Questi
// test fissano cio' che quelle scelte dicono:
//
//  - una cella ricordata ha l'anello nel colore della pianta e il fondo scuro della pianta, e i tre stati restano tre;
//  - il colore della pianta non si confonde con cio' che si vede adesso;
//  - fra osservato e ricordato il colore sfuma, in tutti e due i versi, e il glifo sfuma nel fondo e poi sparisce;
//  - un muro ricordato resta un pannello, tinto e semitrasparente, e torna pieno quando lo si rivede;
//  - il rilievo del costo e' chiaro.

#include "Misc/AutomationTest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Perception/RTTeamKnowledge.h"
#include "Perception/RTVeilTransition.h"
#include "RTConsoleVariableGuardForTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: in unity build i test condividono la TU. */
	ARTHexMapActor* MakePiantaBoard(UWorld* World, URTHexMapAsset* Map)
	{
		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		if (!HexMap)
		{
			return nullptr;
		}
		HexMap->MapAsset = Map;
		HexMap->RebuildInstances();
		return HexMap;
	}

	FRTTeamKnowledge ConoscenzaPianta(const TArray<FRTCellId>& Visibili, const TArray<FRTCellId>& Esplorate)
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.TurnNumber = 1;
		K.VisibleCells = Visibili;
		K.ExploredCells = Esplorate;
		return K;
	}

	const UInstancedStaticMeshComponent* IsmPianta(const ARTHexMapActor* HexMap, const FString& Nome)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Tutte;
		HexMap->GetComponents(Tutte);
		for (const UInstancedStaticMeshComponent* C : Tutte)
		{
			if (C && C->GetName() == Nome)
			{
				return C;
			}
		}
		return nullptr;
	}

	/** L'istanza del disco di una cella. Disco e anello sono uno-a-uno: lo stesso indice vale per l'anello. */
	int32 IstanzaPianta(const ARTHexMapActor* HexMap, const FRTCellId& Cella)
	{
		for (int32 I = 0; I < HexMap->NumInstanceCells(); ++I)
		{
			if (HexMap->CellForInstance(I) == Cella)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	bool DisegnataPianta(const UInstancedStaticMeshComponent* C, int32 I)
	{
		FTransform Xf;
		return C && C->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true) && !Xf.GetScale3D().IsNearlyZero();
	}

	float CanalePianta(const UInstancedStaticMeshComponent* C, int32 I, int32 Canale)
	{
		const int32 Indice = I * C->NumCustomDataFloats + Canale;
		return C->PerInstanceSMCustomData.IsValidIndex(Indice) ? C->PerInstanceSMCustomData[Indice] : -1.f;
	}

	FLinearColor ColorePianta(const UInstancedStaticMeshComponent* C, int32 I)
	{
		return FLinearColor(CanalePianta(C, I, 0), CanalePianta(C, I, 1), CanalePianta(C, I, 2));
	}

	/** Vero se `X` sta strettamente fra `A` e `B` su ciascun canale in cui i due differiscono. */
	bool FraPianta(const FLinearColor& X, const FLinearColor& A, const FLinearColor& B)
	{
		const float Xs[3] = { X.R, X.G, X.B };
		const float As[3] = { A.R, A.G, A.B };
		const float Bs[3] = { B.R, B.G, B.B };
		bool bAlmenoUno = false;
		for (int32 K = 0; K < 3; ++K)
		{
			if (FMath::IsNearlyEqual(As[K], Bs[K], 1e-3f))
			{
				continue;
			}
			bAlmenoUno = true;
			const float Lo = FMath::Min(As[K], Bs[K]);
			const float Hi = FMath::Max(As[K], Bs[K]);
			if (!(Xs[K] > Lo + 1e-4f && Xs[K] < Hi - 1e-4f))
			{
				return false;
			}
		}
		return bAlmenoUno;
	}

	/** Il colore del disco di una cella osservata di superficie `Floor`: quello di `MakeFlatArena`. */
	FLinearColor PavimentoOsservato()
	{
		return FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(ERTHexSurface::Floor));
	}
}

/**
 * 🔑 **Una cella ricordata e' una pianta: l'anello nel colore della pianta, e il fondo scuro della pianta.** E i tre stati
 * restano tre: osservata (disco e anello della griglia), ricordata (anello ciano e fondo ciano scuro), mai vista
 * (niente). Il fondo c'e' per la scelta dell'autore nella seduta `U89`: col solo anello il ricordo sembrava «mai visto».
 *
 * ⚠️ L'ultima meta' misura `GetVeilCounts`, che legge cio' che e' disegnato e trova il ricordo nell'anello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRememberedCellIsAPlanTest,
	"RefactorTactics.Veil.RememberedCellIsAPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRememberedCellIsAPlanTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = MakePiantaBoard(World, URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3));
	const UInstancedStaticMeshComponent* Dischi = HexMap ? IsmPianta(HexMap, TEXT("Cells")) : nullptr;
	const UInstancedStaticMeshComponent* Anelli = HexMap ? IsmPianta(HexMap, TEXT("CellBorders")) : nullptr;
	if (!TestTrue(TEXT("board con dischi e anelli"), Dischi != nullptr && Anelli != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Vista(0, 0, 0);
	const FRTCellId Ricordata(1, 0, 0);
	const FRTCellId MaiVista(3, 0, 0);
	// La prima stesura e' uno scalino, quindi la ricordata e' gia' al suo colore pieno di ricordo.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Vista }, { Vista, Ricordata }));

	const int32 IV = IstanzaPianta(HexMap, Vista);
	const int32 IR = IstanzaPianta(HexMap, Ricordata);
	const int32 IM = IstanzaPianta(HexMap, MaiVista);
	if (!TestTrue(TEXT("premessa: le tre celle hanno un'istanza"), IV != INDEX_NONE && IR != INDEX_NONE && IM != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FLinearColor Pianta = ARTHexMapActor::PlanColor();
	const FLinearColor Fondo = ARTHexMapActor::PlanFloorColor();
	TestTrue(TEXT("osservata: il disco c'e'"), DisegnataPianta(Dischi, IV));
	TestTrue(TEXT("osservata: il disco ha il colore della superficie"), ColorePianta(Dischi, IV).Equals(PavimentoOsservato(), 1e-3f));
	TestTrue(TEXT("osservata: l'anello c'e'"), DisegnataPianta(Anelli, IV));
	TestFalse(TEXT("osservata: l'anello NON ha il colore della pianta"), ColorePianta(Anelli, IV).Equals(Pianta, 1e-3f));

	TestTrue(TEXT("ricordata: il disco c'e', come fondo"), DisegnataPianta(Dischi, IR));
	TestTrue(TEXT("ricordata: il fondo ha il colore scuro della pianta"), ColorePianta(Dischi, IR).Equals(Fondo, 1e-3f));
	// Il fondo non e' il nero del «mai visto», e non e' il colore di un pavimento osservato: e' un terzo stato.
	TestTrue(*FString::Printf(TEXT("ricordata: il fondo non e' nero (luminanza %.3f)"), Fondo.GetLuminance()),
		Fondo.GetLuminance() > 0.02f);
	TestTrue(TEXT("ricordata: il fondo e' piu' scuro del pavimento osservato"),
		Fondo.GetLuminance() < PavimentoOsservato().GetLuminance());
	TestTrue(TEXT("ricordata: il fondo e' tinto di ciano, non grigio"), Fondo.B > Fondo.R && Fondo.G > Fondo.R);
	TestTrue(TEXT("ricordata: l'anello c'e'"), DisegnataPianta(Anelli, IR));
	TestTrue(TEXT("ricordata: l'anello ha il colore della pianta, a piena luminosita'"),
		ColorePianta(Anelli, IR).Equals(Pianta, 1e-3f));

	TestFalse(TEXT("mai vista: niente disco"), DisegnataPianta(Dischi, IM));
	TestFalse(TEXT("mai vista: niente anello"), DisegnataPianta(Anelli, IM));

	int32 Accese = 0, Ricordate = 0, Nascoste = 0;
	HexMap->GetVeilCounts(Accese, Ricordate, Nascoste);
	TestEqual(TEXT("il conteggio trova UNA cella osservata"), Accese, 1);
	TestEqual(TEXT("e UNA ricordata"), Ricordate, 1);
	TestEqual(TEXT("e tutte le altre mai viste"), Nascoste, HexMap->NumInstanceCells() - 2);

	// E si torna indietro: rivista, e conclusa la dissolvenza, la cella ritrova il disco e l'anello della griglia.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Vista, Ricordata }, { Vista, Ricordata }));
	static_cast<AActor*>(HexMap)->Tick(10.f);
	TestTrue(TEXT("rivista: il disco ha di nuovo il colore della superficie"),
		ColorePianta(Dischi, IR).Equals(PavimentoOsservato(), 1e-3f));
	TestFalse(TEXT("rivista: l'anello lascia il colore della pianta"), ColorePianta(Anelli, IR).Equals(Pianta, 1e-3f));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il ricordo non si deve confondere col presente. Ripensa `Hex.VeiledSurfaceColorsAreDistinguishable`, che misurava i
 * colori di superficie attenuati: con la pianta il ricordo non ha piu' un colore di superficie, e il canale che dice
 * «ricordato» e' l'anello. Quindi il colore della pianta si confronta con l'anello delle celle osservate, su tutti i
 * piani.
 *
 * ⚠️ La soglia e' quella del test che ripensa, 50 sulla somma delle differenze in sRGB.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanColorStandsApartTest,
	"RefactorTactics.Hex.PlanColorStandsApartFromThePresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanColorStandsApartTest::RunTest(const FString&)
{
	const FColor Pianta = ARTHexMapActor::PlanColor().ToFColor(/*bSRGB=*/ true);
	const auto Distanza = [](const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	};
	const int32 Soglia = 50;
	for (const ERTLayerRelation R : { ERTLayerRelation::Active, ERTLayerRelation::Above, ERTLayerRelation::Below })
	{
		const int32 D = Distanza(Pianta, RTLayerPresentation::BorderColorFor(R));
		TestTrue(*FString::Printf(TEXT("la pianta si distingue dall'anello osservato del piano %d (distanza %d)"),
			static_cast<int32>(R), D), D >= Soglia);
	}
	return true;
}

/**
 * 🔑 **Fra osservato e ricordato il colore SFUMA, in tutti e due i versi.** Nella seduta `U89` l'autore ha scelto «Il
 * cambio secco disturba»: la prima stesura della pianta passava al ricordo in un fotogramma.
 *
 * Si misura il disco, l'anello e il glifo di una cella di ghiaccio, che ne ha uno: appena la cella esce dalla vista hanno
 * ancora il colore vero; a meta' strada stanno fra i due; alla fine hanno quello della pianta, e il glifo, che ha sfumato
 * nel fondo, non si disegna piu'. Poi al contrario.
 *
 * ⚠️ Il controllo positivo e' il primo: senza dissolvenza il colore sarebbe gia' quello della pianta subito dopo il
 * velo, ed e' esattamente cio' che la prima stesura faceva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanDissolvesTest,
	"RefactorTactics.Veil.PlanDissolvesBetweenObservedAndRemembered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanDissolvesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
	const FRTCellId Cella(1, 0, 0);
	{
		FRTHexCellData Dati = *Map->FindCell(Cella);
		Dati.Surface = ERTHexSurface::Ice;
		Map->AddOrUpdateCell(Dati);
	}
	ARTHexMapActor* HexMap = MakePiantaBoard(World, Map);
	const UInstancedStaticMeshComponent* Dischi = HexMap ? IsmPianta(HexMap, TEXT("Cells")) : nullptr;
	const UInstancedStaticMeshComponent* Anelli = HexMap ? IsmPianta(HexMap, TEXT("CellBorders")) : nullptr;
	const int32 IC = HexMap ? IstanzaPianta(HexMap, Cella) : INDEX_NONE;
	// Il glifo del ghiaccio: la famiglia di glifi che ha un'istanza, l'unica su questa board.
	const UInstancedStaticMeshComponent* Glifi = nullptr;
	if (HexMap)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Tutte;
		HexMap->GetComponents(Tutte);
		for (const UInstancedStaticMeshComponent* C : Tutte)
		{
			if (C && C->GetName().StartsWith(TEXT("SurfaceGlyph")) && C->GetInstanceCount() == 1)
			{
				Glifi = C;
			}
		}
	}
	if (!TestTrue(TEXT("premessa: dischi, anelli, l'istanza della cella e il glifo del ghiaccio"),
		Dischi != nullptr && Anelli != nullptr && IC != INDEX_NONE && Glifi != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	FRTVeilTransitionParams Tempi;
	Tempi.RevealSeconds = 0.4f;
	Tempi.HideSeconds = 0.4f;
	HexMap->SetVeilTransitionParams(Tempi);

	const FLinearColor Vero = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(ERTHexSurface::Ice));
	const FLinearColor Fondo = ARTHexMapActor::PlanFloorColor();
	const FLinearColor Pianta = ARTHexMapActor::PlanColor();
	const FLinearColor AnelloVero = ColorePianta(Anelli, IC); // prima del velo: quello della griglia
	const FRTCellId Altra(0, 0, 0);

	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Cella, Altra }, { Cella, Altra }));
	const FLinearColor AnelloOsservato = ColorePianta(Anelli, IC);
	TestTrue(TEXT("osservata: il disco ha il colore del ghiaccio"), ColorePianta(Dischi, IC).Equals(Vero, 1e-3f));
	TestTrue(TEXT("osservata: il glifo si disegna"), DisegnataPianta(Glifi, 0));

	// Esce dalla vista.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Altra }, { Cella, Altra }));
	TestTrue(TEXT("si apre una dissolvenza"), HexMap->GetVeilCellsInTransition() > 0);
	TestTrue(TEXT("subito dopo il velo il disco ha ancora il colore del ghiaccio"), ColorePianta(Dischi, IC).Equals(Vero, 1e-3f));
	TestTrue(TEXT("e il glifo si disegna ancora"), DisegnataPianta(Glifi, 0));

	static_cast<AActor*>(HexMap)->Tick(0.1f);
	TestTrue(TEXT("a meta' strada il disco sta fra il ghiaccio e il fondo della pianta"),
		FraPianta(ColorePianta(Dischi, IC), Vero, Fondo));
	TestTrue(TEXT("e l'anello fra il colore della griglia e quello della pianta"),
		FraPianta(ColorePianta(Anelli, IC), AnelloOsservato, Pianta));

	static_cast<AActor*>(HexMap)->Tick(10.f);
	TestEqual(TEXT("alla fine nessuna istanza e' piu' in movimento"), HexMap->GetVeilCellsInTransition(), 0);
	TestTrue(TEXT("alla fine il disco e' il fondo della pianta"), ColorePianta(Dischi, IC).Equals(Fondo, 1e-3f));
	TestTrue(TEXT("e l'anello e' la pianta"), ColorePianta(Anelli, IC).Equals(Pianta, 1e-3f));
	TestFalse(TEXT("e il glifo, sfumato nel fondo, non si disegna piu'"), DisegnataPianta(Glifi, 0));

	// E al contrario: rivista, il colore torna sfumando.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Cella, Altra }, { Cella, Altra }));
	TestTrue(TEXT("rivista: il glifo si disegna subito"), DisegnataPianta(Glifi, 0));
	TestTrue(TEXT("rivista: subito dopo il velo il disco e' ancora il fondo"), ColorePianta(Dischi, IC).Equals(Fondo, 1e-3f));
	static_cast<AActor*>(HexMap)->Tick(0.1f);
	TestTrue(TEXT("rivista, a meta' strada: il disco sta fra il fondo e il ghiaccio"),
		FraPianta(ColorePianta(Dischi, IC), Fondo, Vero));
	static_cast<AActor*>(HexMap)->Tick(10.f);
	TestTrue(TEXT("rivista, alla fine: il disco e' di nuovo ghiaccio"), ColorePianta(Dischi, IC).Equals(Vero, 1e-3f));
	TestTrue(TEXT("e l'anello e' di nuovo quello della griglia"), ColorePianta(Anelli, IC).Equals(AnelloOsservato, 1e-3f));
	AddInfo(FString::Printf(TEXT("anello prima del velo %s, osservato %s"), *AnelloVero.ToString(), *AnelloOsservato.ToString()));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔑 **Un muro ricordato resta un PANNELLO, tinto col colore della pianta e semitrasparente, e torna pieno quando lo si
 * rivede.** La scelta dell'autore nella seduta `U89`, «Pannello traslucido»: *«Il muro ricordato resta un volume,
 * attenuato o semitrasparente, invece di una linea»*. La prima stesura lo ridisegnava a linee, e le linee non ci sono
 * piu': il controllo e' sul batcher del mondo, col disegno di debug spento come in Shipping.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRememberedWallIsATranslucentPanelTest,
	"RefactorTactics.Veil.RememberedWallIsATranslucentPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRememberedWallIsATranslucentPanelTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Una copertura alta sul lato est della cella (1,0): dall'altra parte (2,0), che la squadra non ha mai visto.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	const FRTCellId ConMuro(1, 0, 0);
	FRTHexCellData Cella = *Map->FindCell(ConMuro);
	FRTHexCover Copertura;
	Copertura.Edge = ERTHexDirection::E;
	Copertura.Type = ERTHexCoverType::High;
	Cella.Covers = { Copertura };
	Map->AddOrUpdateCell(Cella);

	ARTHexMapActor* HexMap = MakePiantaBoard(World, Map);
	const UInstancedStaticMeshComponent* Pannelli = HexMap ? IsmPianta(HexMap, TEXT("EdgeFeatures")) : nullptr;
	ULineBatchComponent* Mondo = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: board, pannelli, batcher e CVar del debug"),
		HexMap != nullptr && Pannelli != nullptr && Mondo != nullptr && Debug != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("premessa: la copertura ha UN pannello"), Pannelli->GetInstanceCount(), 1)
		|| !TestTrue(TEXT("premessa: i pannelli portano i canali dei volumi"),
			Pannelli->NumCustomDataFloats >= RTLayerPresentation::VolumeNumCustomDataFloats))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	const FLinearColor Pianta = ARTHexMapActor::PlanColor();
	const FLinearColor PiantaSRGB(Pianta.ToFColor(/*bSRGB=*/ true));
	const int32 Trasparenza = RTLayerPresentation::FirstChannel;
	const int32 Forza = RTLayerPresentation::VolumeTintStrengthChannel;
	const auto LineeDellaPianta = [&]()
	{
		Mondo->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		int32 N = 0;
		for (const FBatchedLine& L : Mondo->BatchedLines)
		{
			N += L.Color.Equals(PiantaSRGB) ? 1 : 0;
		}
		return N;
	};

	// Osservata: il pannello e' pieno, senza tinta e opaco, come prima della pianta su una mappa a un piano.
	const FRTCellId Davanti(0, 0, 0);
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Davanti, ConMuro }, { Davanti, ConMuro }));
	TestTrue(TEXT("visto: il pannello si disegna"), DisegnataPianta(Pannelli, 0));
	TestTrue(TEXT("visto: nessuna tinta"), FMath::IsNearlyZero(CanalePianta(Pannelli, 0, Forza)));
	TestTrue(TEXT("visto: opaco"), FMath::IsNearlyZero(CanalePianta(Pannelli, 0, Trasparenza)));

	// Ricordato: resta, tinto e semitrasparente, dopo la dissolvenza.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Davanti }, { Davanti, ConMuro }));
	static_cast<AActor*>(HexMap)->Tick(10.f);
	TestTrue(TEXT("ricordato: il pannello si disegna ancora"), DisegnataPianta(Pannelli, 0));
	TestTrue(TEXT("ricordato: la tinta e' piena"), FMath::IsNearlyEqual(CanalePianta(Pannelli, 0, Forza), 1.f));
	TestTrue(TEXT("ricordato: nel colore della pianta"), ColorePianta(Pannelli, 0).Equals(Pianta, 1e-3f));
	const float T = CanalePianta(Pannelli, 0, Trasparenza);
	TestTrue(*FString::Printf(TEXT("ricordato: semitrasparente (%.2f), ne' opaco ne' sparito"), T), T > 0.1f && T < 0.9f);
	TestEqual(TEXT("ricordato: nessuna linea della pianta al suo posto"), LineeDellaPianta(), 0);

	// 🔴 **E riapplicare la presentazione dei piani non lo rimette pieno.** La presentazione e il velo scrivono gli stessi
	// canali, e `WriteVolumeAppearance` e' il loro scrittore comune: scritti dalla sola presentazione, coi valori del
	// piano, i muri ricordati tornerebbero opachi al primo cambio di piano attivo (mutazione `M9` della verifica del
	// 2026-10-15, sopravvissuta prima di questo controllo).
	TestTrue(TEXT("premessa: la presentazione dei piani si riapplica"), HexMap->SyncActiveLayerPresentation(0));
	TestTrue(TEXT("riapplicati i piani, il pannello ricordato ha ancora la tinta piena"),
		FMath::IsNearlyEqual(CanalePianta(Pannelli, 0, Forza), 1.f));
	TestTrue(TEXT("ed e' ancora semitrasparente"), CanalePianta(Pannelli, 0, Trasparenza) > 0.1f);

	// Rivisto: il pannello torna pieno.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Davanti, ConMuro }, { Davanti, ConMuro }));
	static_cast<AActor*>(HexMap)->Tick(10.f);
	TestTrue(TEXT("rivisto: nessuna tinta"), FMath::IsNearlyZero(CanalePianta(Pannelli, 0, Forza)));
	TestTrue(TEXT("rivisto: opaco"), FMath::IsNearlyZero(CanalePianta(Pannelli, 0, Trasparenza)));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il rilievo del costo e' CHIARO (#3760, seduta `U89`). Teneva il materiale di default della mesh, che in luce piena e'
 * un grigio scuro, e l'autore ha letto come «nere» le celle viste che lo portano. Ora ha l'istanza del materiale dei
 * volumi col suo colore proprio, piu' chiaro di quello dei volumi.
 *
 * ⚠️ Senza `VolumeMaterial` caricabile il rilievo tiene il materiale della mesh, e il test lo dice invece di passare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReliefIsLightTest,
	"RefactorTactics.HexMapActor.CostReliefIsLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReliefIsLightTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = MakePiantaBoard(World, URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 1));
	const UInstancedStaticMeshComponent* Rilievo = HexMap ? IsmPianta(HexMap, TEXT("Relief")) : nullptr;
	if (!TestNotNull(TEXT("premessa: il rilievo"), Rilievo))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Rilievo->GetMaterial(0));
	if (TestNotNull(TEXT("il rilievo ha un'istanza del materiale dei volumi"), Mid))
	{
		const FLinearColor Proprio = Mid->K2_GetVectorParameterValue(RTLayerPresentation::VolumeParamOwnColor);
		const FLinearColor DeiVolumi = RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ false);
		TestTrue(*FString::Printf(TEXT("il suo colore proprio (%.2f) e' piu' chiaro di quello dei volumi (%.2f)"),
			Proprio.GetLuminance(), DeiVolumi.GetLuminance()), Proprio.GetLuminance() > 2.f * DeiVolumi.GetLuminance());
		TestEqual(TEXT("e non porta dati per istanza: niente tinta del piano"), Rilievo->NumCustomDataFloats, 0);
	}
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il volume del FUMO ha il colore della sua superficie (#3760, seduta `U89`). Con il grigio scuro dei prismi una cella di
 * fumo restava «nera» nella zona vista, accanto alle celle col rilievo appena schiarito. Su `RelayBasin`, che il fumo lo
 * porta, come nella tavola `V3`.
 *
 * 🔑 La seconda meta' e' la guardia del colore unico: la famiglia ha UNA istanza del materiale, quindi il colore vale
 * finche' il fumo e' l'unica superficie con un volume. Una seconda forma fa cadere il test, e dovra' decidere il suo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSmokeVolumeIsLightTest,
	"RefactorTactics.HexMapActor.SmokeVolumeIsLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSmokeVolumeIsLightTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = MakePiantaBoard(World, URTMatchSetupLibrary::MakeShowcaseRelayBasinArena(GetTransientPackage()));
	const UInstancedStaticMeshComponent* Fumo = HexMap ? IsmPianta(HexMap, TEXT("SurfaceVolumes")) : nullptr;
	if (!TestNotNull(TEXT("premessa: i volumi di superficie"), Fumo) || !TestTrue(TEXT("premessa: la mappa ha del fumo"), Fumo->GetInstanceCount() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Fumo->GetMaterial(0));
	if (TestNotNull(TEXT("il volume del fumo ha un'istanza del materiale dei volumi"), Mid))
	{
		const FLinearColor Proprio = Mid->K2_GetVectorParameterValue(RTLayerPresentation::VolumeParamOwnColor);
		const FLinearColor DelFumo = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(ERTHexSurface::Smoke));
		TestTrue(TEXT("il suo colore proprio e' quello della superficie del fumo"), Proprio.Equals(DelFumo, 1.e-4f));
		const FLinearColor DeiVolumi = RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ false);
		TestTrue(*FString::Printf(TEXT("ed e' piu' chiaro di quello dei volumi (%.2f contro %.2f)"),
			Proprio.GetLuminance(), DeiVolumi.GetLuminance()), Proprio.GetLuminance() > 2.f * DeiVolumi.GetLuminance());
	}

	const UEnum* SurfaceEnum = StaticEnum<ERTHexSurface>();
	if (TestNotNull(TEXT("l'enum delle superfici e' riflesso"), SurfaceEnum))
	{
		for (int32 I = 0; I < SurfaceEnum->NumEnums() - 1; ++I)
		{
			const ERTHexSurface S = static_cast<ERTHexSurface>(SurfaceEnum->GetValueByIndex(I));
			if (S != ERTHexSurface::Smoke)
			{
				TestFalse(*FString::Printf(TEXT("%s non ha un volume: il colore della famiglia e' quello del fumo"),
					*SurfaceEnum->GetNameStringByIndex(I)), URTHexLibrary::SurfaceVolumeFor(S).Y > 0.0);
			}
		}
	}
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
