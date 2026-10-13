// Il piano di muri, coperture, porte e blocchi si legge dalla tinta del volume (#3706, [D-508]).
//
// 🔑 **Questi test misurano i DATI che la board scrive sui volumi, non cio' che si vede.** La tinta sta nei canali
// 0..2 e 6 dei custom data per istanza, il trattamento di D-501 nei 3..5, e li applica `M_HexVolume`: che a schermo
// un muro dica il suo piano resta della voce PIE di #3706. Qui si misura che i valori giusti arrivino alle istanze
// giuste, su tutte e quattro le famiglie, e che il velo e una ricostruzione parziale non li perdano.
//
// ⚠️ **«I dati arrivano» non vuol dire «il materiale li usa»**: e' la lezione di `M_HexCell` (#3704). L'altra meta'
// sta nel modulo Editor, `HexVolume.MaterialMatchesTheRecipe`, che guarda l'asset.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTHexMapActor* LvtSpawnBoard(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Board = World->SpawnActorDeferred<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (Board)
		{
			Board->MapAsset = Asset;
			Board->FinishSpawning(FTransform::Identity); // `OnConstruction` ricostruisce le istanze
		}
		return Board;
	}

	float LvtChannel(const UInstancedStaticMeshComponent* Ism, int32 Instance, int32 Channel)
	{
		const int32 Indice = Instance * Ism->NumCustomDataFloats + Channel;
		return Ism->PerInstanceSMCustomData.IsValidIndex(Indice) ? Ism->PerInstanceSMCustomData[Indice] : -999.f;
	}

	/** Le celle di `Map` sul piano `Layer`, come coppie (q,r). */
	TSet<FIntPoint> LvtFootprint(const URTHexMapAsset* Map, int32 Layer)
	{
		TSet<FIntPoint> Impronta;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (Cell.Id.Layer == Layer) { Impronta.Add(FIntPoint(Cell.Id.X, Cell.Id.Y)); }
		}
		return Impronta;
	}

	/**
	 * Confronta OGNI istanza di ogni famiglia dei volumi con la tinta e il trattamento attesi per il piano attivo
	 * `Attivo`. Ritorna quante istanze ha guardato, per relazione: chi chiama asserisce che non sia zero.
	 */
	void LvtCheckFamilies(FAutomationTestBase& Test, const ARTHexMapActor* Board, int32 Attivo, bool bMultiLayer,
		const TCHAR* Quando, int32 OutPerRelazione[3])
	{
		const FRTLayerPresentationTuning T;
		OutPerRelazione[0] = OutPerRelazione[1] = OutPerRelazione[2] = 0;
		for (const ARTHexMapActor::FRTVolumeFamily& Volume : Board->GetVolumeFamilies())
		{
			const UInstancedStaticMeshComponent* Ism = Volume.Component;
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: la famiglia %s esiste"), Quando, Volume.Name), Ism)) { continue; }
			if (!Test.TestEqual(*FString::Printf(TEXT("%s: %s ha una cella per istanza"), Quando, Volume.Name),
				Ism->GetInstanceCount(), Volume.Cells->Num()))
			{
				continue;
			}
			int32 Sbagliate = 0;
			for (int32 I = 0; I < Volume.Cells->Num(); ++I)
			{
				const int32 Layer = (*Volume.Cells)[I].Layer;
				const FRTLayerVolumeTint Tinta = RTLayerPresentation::VolumeTint(Layer, Attivo, bMultiLayer, T);
				const FRTLayerTreatment Tr = RTLayerPresentation::Treatment(Layer, Attivo, bMultiLayer, T);
				const bool bUguale =
					FMath::IsNearlyEqual(LvtChannel(Ism, I, 0), Tinta.Color.R, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, 1), Tinta.Color.G, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, 2), Tinta.Color.B, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, RTLayerPresentation::VolumeTintStrengthChannel), Tinta.Strength, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, RTLayerPresentation::FirstChannel + 0), Tr.Transparency, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, RTLayerPresentation::FirstChannel + 1), Tr.Desaturation, 1.e-4f)
					&& FMath::IsNearlyEqual(LvtChannel(Ism, I, RTLayerPresentation::FirstChannel + 2), Tr.ExposureStops, 1.e-4f);
				Sbagliate += bUguale ? 0 : 1;
				++OutPerRelazione[static_cast<int32>(RTLayerPresentation::Relation(Layer, Attivo, bMultiLayer))];
			}
			Test.TestEqual(*FString::Printf(TEXT("%s: ogni istanza di %s porta la tinta e il trattamento del suo piano"), Quando, Volume.Name),
				Sbagliate, 0);
		}
	}
}

/**
 * **La tinta del volume si ricava da (piano, piano attivo, mappa a piu' piani)** — la funzione pura della DoD di #3706.
 *
 * 🔑 Il colore NON e' riscritto: e' `BorderColorFor` della relazione, in lineare. Se i bordi delle celle cambiassero
 * colore, i volumi li seguono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerVolumeTintPureTest,
	"RefactorTactics.HexMap.VolumeTintFollowsTheLayerRelation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerVolumeTintPureTest::RunTest(const FString&)
{
	const FRTLayerPresentationTuning T;

	TestTrue(TEXT("una mappa a un piano non ha tinta (scope 3)"),
		RTLayerPresentation::VolumeTint(0, 0, /*bMultiLayer=*/ false, T).IsNone());
	TestTrue(TEXT("nemmeno su un piano che non e' l'attivo"),
		RTLayerPresentation::VolumeTint(2, 0, /*bMultiLayer=*/ false, T).IsNone());

	const TPair<int32, ERTLayerRelation> Casi[] = {
		{ 1, ERTLayerRelation::Active }, { 2, ERTLayerRelation::Above }, { 3, ERTLayerRelation::Above },
		{ 0, ERTLayerRelation::Below } };
	for (const TPair<int32, ERTLayerRelation>& Caso : Casi)
	{
		const FRTLayerVolumeTint Tinta = RTLayerPresentation::VolumeTint(Caso.Key, /*ActiveLayer=*/ 1, true, T);
		TestTrue(*FString::Printf(TEXT("piano %d: il colore e' quello del bordo della sua relazione, in lineare"), Caso.Key),
			Tinta.Color.Equals(FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(Caso.Value)), 1.e-4f));
		TestTrue(*FString::Printf(TEXT("piano %d: la forza e' quella della taratura, e non e' zero"), Caso.Key),
			FMath::IsNearlyEqual(Tinta.Strength, T.VolumeTintStrength) && !Tinta.IsNone());
	}

	// La leva: con un valore DIVERSO dal default, cosi' una `VolumeTint` che ignorasse la taratura cade qui.
	FRTLayerPresentationTuning Leva;
	Leva.VolumeTintStrength = 0.6f;
	TestTrue(TEXT("la forza segue la leva"), FMath::IsNearlyEqual(RTLayerPresentation::VolumeTint(2, 1, true, Leva).Strength, 0.6f));
	Leva.VolumeTintStrength = -3.f;
	TestEqual(TEXT("una leva negativa non da' una tinta negativa"), RTLayerPresentation::VolumeTint(2, 1, true, Leva).Strength, 0.f);
	Leva.VolumeTintStrength = 9.f;
	TestEqual(TEXT("e una enorme si ferma a uno"), RTLayerPresentation::VolumeTint(2, 1, true, Leva).Strength, 1.f);
	Leva.VolumeTintStrength = 0.f;
	TestTrue(TEXT("a zero la tinta e' spenta anche su una mappa a piu' piani"), RTLayerPresentation::VolumeTint(2, 1, true, Leva).IsNone());

	// Il default l'ha scelto l'autore (seduta `U86`, 2026-10-13). Un cambio per sbaglio cade qui.
	TestTrue(TEXT("default: la tinta e' leggera, 0,25 (scelta dall'autore, #3706)"), FMath::IsNearlyEqual(T.VolumeTintStrength, 0.25f));

	// La disposizione dei canali: il trattamento sta dove sta sulle celle, la forza della tinta e' l'ultimo.
	TestEqual(TEXT("i volumi portano un canale in piu' delle celle"),
		RTLayerPresentation::VolumeNumCustomDataFloats, RTLayerPresentation::NumCustomDataFloats + 1);
	TestEqual(TEXT("la forza della tinta sta nell'ultimo canale"),
		RTLayerPresentation::VolumeTintStrengthChannel, RTLayerPresentation::VolumeNumCustomDataFloats - 1);
	TestTrue(TEXT("e non si sovrappone al trattamento"),
		RTLayerPresentation::VolumeTintStrengthChannel >= RTLayerPresentation::FirstChannel + 3);

	// Il colore proprio e' neutro (`R == G == B`): la tinta e' l'unico canale cromatico di un volume.
	for (const bool bPannello : { false, true })
	{
		const FLinearColor Proprio = RTLayerPresentation::VolumeOwnColor(bPannello);
		TestTrue(TEXT("il colore proprio di un volume e' un grigio neutro"), Proprio.R == Proprio.G && Proprio.G == Proprio.B);
	}
	return true;
}

/**
 * `LayerYard` — la fixture su cui si giudica la tinta dei volumi: i tre piani di `LayerStack`, e ogni famiglia dei
 * volumi su **almeno due piani**. Una famiglia su un piano solo non mostrerebbe la differenza fra la tinta dell'attivo
 * e quella di un altro piano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerYardFixtureTest,
	"RefactorTactics.HexMap.LayerYardCarriesVolumesOnSeveralLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerYardFixtureTest::RunTest(const FString&)
{
	TestTrue(TEXT("`LayerYard` e' fra i nomi noti"), URTMatchSetupLibrary::KnownFixtureIds().Contains(TEXT("LayerYard")));
	URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("LayerYard"));
	URTHexMapAsset* Stack = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("LayerStack"));
	if (!TestNotNull(TEXT("la fixture si costruisce per nome"), Yard) || !TestNotNull(TEXT("e anche LayerStack"), Stack))
	{
		return false;
	}

	// Stesse impronte e stessi archi di `LayerStack`: le due fixture sono la stessa scena, con e senza volumi.
	for (int32 Piano = 0; Piano <= 2; ++Piano)
	{
		const TSet<FIntPoint> A = LvtFootprint(Yard, Piano);
		const TSet<FIntPoint> B = LvtFootprint(Stack, Piano);
		TestTrue(*FString::Printf(TEXT("il piano %d ha l'impronta di LayerStack (%d celle)"), Piano, B.Num()),
			A.Num() == B.Num() && A.Num() > 0 && A.Difference(B).Num() == 0);
	}
	TestEqual(TEXT("e gli stessi archi fra i piani"), Yard->Transitions.Num(), Stack->Transitions.Num());

	// Da terra si sale fino in cima anche coi volumi: un blocco sulla strada renderebbe i piani alti scenografia.
	const FRTHexSnapshot Snap =
		URTHexSimLibrary::MakeSnapshotOmniscient(Yard, { FRTHexSimUnit(0, FRTCellId(-1, 3, 0), /*Budget=*/ 99) });
	TSet<int32> PianiRaggiunti;
	for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snap, 0))
	{
		PianiRaggiunti.Add(R.Cell.Layer);
	}
	TestTrue(TEXT("da terra si raggiungono il piano 1 e il piano 2"), PianiRaggiunti.Contains(1) && PianiRaggiunti.Contains(2));

	// Sulla board: ogni famiglia dei volumi ha istanze su almeno due piani.
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	ARTHexMapActor* Board = LvtSpawnBoard(World, URTMatchSetupLibrary::MakeLayerYardArena(World));
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }
	for (const ARTHexMapActor::FRTVolumeFamily& Volume : Board->GetVolumeFamilies())
	{
		TSet<int32> Piani;
		for (const FRTCellId& Cell : *Volume.Cells) { Piani.Add(Cell.Layer); }
		AddInfo(FString::Printf(TEXT("%s: %d istanze su %d piani"), Volume.Name, Volume.Cells->Num(), Piani.Num()));
		TestTrue(*FString::Printf(TEXT("%s ha istanze su almeno due piani"), Volume.Name), Piani.Num() >= 2);
	}
	return true;
}

/**
 * **Le quattro famiglie portano i dati per istanza, e la board ci scrive la tinta e il trattamento del loro piano.**
 *
 * 🔴 Si guarda OGNI istanza di ogni famiglia, e si conta quante per relazione: una procedura che saltasse una
 * famiglia (la guardia sulla mappatura stantia lo fa in silenzio) lascerebbe i suoi canali a zero, e il confronto
 * cadrebbe. I tre contatori dicono che la scena contiene davvero volumi sull'attivo, sopra e sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerVolumeTintBoardTest,
	"RefactorTactics.HexMap.VolumesCarryTheTintOfTheirLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerVolumeTintBoardTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	ARTHexMapActor* Board = LvtSpawnBoard(World, URTMatchSetupLibrary::MakeLayerYardArena(World));
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }

	// Le famiglie, i loro canali, il loro materiale.
	UMaterialInterface* Materiale = Board->VolumeMaterial.LoadSynchronous();
	TestNotNull(TEXT("il materiale dei volumi si carica (lo scrive il commandlet RTBuildHexVolumeMaterial)"), Materiale);
	for (const ARTHexMapActor::FRTVolumeFamily& Volume : Board->GetVolumeFamilies())
	{
		if (!TestNotNull(*FString::Printf(TEXT("la famiglia %s"), Volume.Name), Volume.Component)) { return false; }
		TestEqual(*FString::Printf(TEXT("%s porta i sette dati per istanza"), Volume.Name),
			Volume.Component->NumCustomDataFloats, RTLayerPresentation::VolumeNumCustomDataFloats);
		const UMaterialInterface* Montato = Volume.Component->GetMaterial(0);
		TestTrue(*FString::Printf(TEXT("%s monta il materiale dei volumi, o una sua istanza"), Volume.Name),
			Materiale != nullptr && Montato != nullptr && Montato->GetMaterial() == Materiale->GetMaterial());
	}
	// I pannelli di bordo hanno il loro colore proprio, in una istanza dinamica dello stesso materiale.
	if (UMaterialInstanceDynamic* Pannelli = Cast<UMaterialInstanceDynamic>(Board->GetVolumeFamilies()[0].Component->GetMaterial(0)))
	{
		TestTrue(TEXT("i pannelli di bordo hanno il colore proprio dei pannelli"),
			Pannelli->K2_GetVectorParameterValue(RTLayerPresentation::VolumeParamOwnColor)
				.Equals(RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ true), 1.e-4f));
	}
	else
	{
		AddError(TEXT("EdgeFeatures non monta una istanza dinamica: i pannelli avrebbero il colore dei prismi"));
	}

	// ⛔ `Relief` resta fuori: e' il rilievo del costo, e tiene il materiale di default (vedi `VolumeMaterial`).
	TArray<UInstancedStaticMeshComponent*> Isms;
	Board->GetComponents(Isms);
	for (const UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism && Ism->GetName() == TEXT("Relief"))
		{
			TestEqual(TEXT("Relief non porta dati per istanza: non riceve ne' tinta ne' trattamento"), Ism->NumCustomDataFloats, 0);
		}
	}

	// Prima che chi guarda dica il piano la board e' neutra: nessuna tinta, nessun trattamento.
	int32 PerRelazione[3];
	TestEqual(TEXT("premessa: nessuno ha ancora detto il piano"), Board->GetPresentedActiveLayer(), static_cast<int32>(INDEX_NONE));
	for (const ARTHexMapActor::FRTVolumeFamily& Volume : Board->GetVolumeFamilies())
	{
		int32 NonZero = 0;
		for (int32 I = 0; I < Volume.Cells->Num(); ++I)
		{
			for (int32 C = 0; C < RTLayerPresentation::VolumeNumCustomDataFloats; ++C)
			{
				NonZero += LvtChannel(Volume.Component, I, C) != 0.f ? 1 : 0;
			}
		}
		TestEqual(*FString::Printf(TEXT("premessa: %s nasce neutra, tutti i canali a zero"), Volume.Name), NonZero, 0);
	}

	// Il mezzanino attivo: sulla scena ci sono volumi sull'attivo, sopra e sotto.
	TestTrue(TEXT("la board presenta il piano 1"), Board->SyncActiveLayerPresentation(1));
	LvtCheckFamilies(*this, Board, /*Attivo=*/ 1, /*bMultiLayer=*/ true, TEXT("piano attivo 1"), PerRelazione);
	TestTrue(*FString::Printf(TEXT("controllo positivo: volumi sull'attivo (%d), sopra (%d) e sotto (%d)"),
		PerRelazione[0], PerRelazione[1], PerRelazione[2]), PerRelazione[0] > 0 && PerRelazione[1] > 0 && PerRelazione[2] > 0);

	// Cambia il piano attivo: la tinta si sposta.
	TestTrue(TEXT("la board presenta il piano 0"), Board->SyncActiveLayerPresentation(0));
	LvtCheckFamilies(*this, Board, /*Attivo=*/ 0, true, TEXT("piano attivo 0"), PerRelazione);
	TestEqual(TEXT("col piano terra attivo nessun volume sta sotto"), PerRelazione[static_cast<int32>(ERTLayerRelation::Below)], 0);
	return true;
}

/**
 * **Il velo lascia stare la tinta, e una ricostruzione parziale dei pannelli di bordo non la perde** (DoD di #3706).
 *
 * I pannelli seguono la mappa in partita (#3687): quando un bordo cambia, `EdgeFeatures` si ricostruisce da sola, e
 * le sue istanze nuove nascono coi canali a zero. Senza la riscrittura a fine ricostruzione tornerebbero neutre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerVolumeTintSurvivesTest,
	"RefactorTactics.HexMap.VolumeTintSurvivesTheVeilAndAPartialRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerVolumeTintSurvivesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeLayerYardArena(World);
	ARTHexMapActor* Board = LvtSpawnBoard(World, Asset);
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }

	int32 PerRelazione[3];
	Board->SyncActiveLayerPresentation(1);
	LvtCheckFamilies(*this, Board, 1, true, TEXT("premessa"), PerRelazione);

	// Il velo: tutta la mappa vista, poi tutta ricordata. Su queste famiglie nasconde con la scala e non scrive colore.
	FRTTeamKnowledge Conosce;
	for (const FRTHexCellData& Cell : Asset->Cells)
	{
		Conosce.VisibleCells.Add(Cell.Id);
		Conosce.ExploredCells.Add(Cell.Id);
	}
	Board->ApplyKnowledgeVeil(Conosce);
	LvtCheckFamilies(*this, Board, 1, true, TEXT("dopo il velo (tutto visto)"), PerRelazione);
	Conosce.VisibleCells.Reset();
	Board->ApplyKnowledgeVeil(Conosce);
	TestTrue(TEXT("premessa: il secondo velo ha toccato delle istanze"), Board->GetLastVeilTouchedCells() > 0);
	LvtCheckFamilies(*this, Board, 1, true, TEXT("dopo il velo (tutto ricordato)"), PerRelazione);

	// La ricostruzione PARZIALE dei soli pannelli di bordo.
	const int32 PannelliPrima = Board->GetVolumeFamilies()[0].Component->GetInstanceCount();
	Board->RebuildInstances(ERTRebuildFamily::EdgeFeatures);
	const ARTHexMapActor::FRTVolumeFamily Pannelli = Board->GetVolumeFamilies()[0];
	TestTrue(TEXT("premessa: i pannelli ci sono ancora, quanti prima"),
		Pannelli.Component->GetInstanceCount() == PannelliPrima && PannelliPrima > 0);
	LvtCheckFamilies(*this, Board, 1, true, TEXT("dopo la ricostruzione parziale dei pannelli"), PerRelazione);
	// Anti-vacuita': almeno un pannello ha davvero una tinta (forza non nulla), cioe' il confronto non era zero == zero.
	int32 ConTinta = 0;
	for (int32 I = 0; I < Pannelli.Cells->Num(); ++I)
	{
		ConTinta += LvtChannel(Pannelli.Component, I, RTLayerPresentation::VolumeTintStrengthChannel) > 0.f ? 1 : 0;
	}
	TestEqual(TEXT("dopo la ricostruzione ogni pannello ha ancora la sua tinta"), ConTinta, Pannelli.Cells->Num());
	return true;
}

/**
 * **Una mappa a un piano non ha tinta**: i pannelli di `CoverYard` restano del colore loro (#3706, scope 3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerVolumeTintSingleLayerTest,
	"RefactorTactics.HexMap.SingleLayerMapHasNoVolumeTint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerVolumeTintSingleLayerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	ARTHexMapActor* Board = LvtSpawnBoard(World, URTMatchSetupLibrary::MakeCoverYardArena(World));
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }

	TestTrue(TEXT("la board accetta il piano"), Board->SyncActiveLayerPresentation(0));
	TestFalse(TEXT("premessa: la presentazione e' a un piano"), Board->IsPresentationMultiLayer());
	const ARTHexMapActor::FRTVolumeFamily Pannelli = Board->GetVolumeFamilies()[0];
	if (!TestTrue(TEXT("premessa: `CoverYard` ha pannelli di bordo"), Pannelli.Component != nullptr && Pannelli.Cells->Num() > 0))
	{
		return false;
	}
	int32 Scritti = 0;
	for (int32 I = 0; I < Pannelli.Cells->Num(); ++I)
	{
		for (int32 C = 0; C < RTLayerPresentation::VolumeNumCustomDataFloats; ++C)
		{
			Scritti += LvtChannel(Pannelli.Component, I, C) != 0.f ? 1 : 0;
		}
	}
	TestEqual(TEXT("a un piano nessun canale e' scritto: ne' tinta ne' trattamento"), Scritti, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
