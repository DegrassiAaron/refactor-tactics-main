// I piani rispetto a quello attivo (#3679, [D-501]): sopra traslucidi, sotto spenti, il bordo dell'attivo marcato.
//
// 🔑 **Questi test misurano i DATI che la board scrive, non cio' che si vede.** Il trattamento sta nei canali 3..5 dei
// custom data per istanza, e lo applica `M_HexCell`: che a schermo si capisca quale piano e' attivo resta della voce
// PIE di #3679. Qui si misura che i canali giusti arrivino alle istanze giuste, che il controller sia l'ingresso, e
// che il velo e la collisione non ne sappiano niente.
//
// ⚠️ **«I dati arrivano» non ha mai voluto dire «il materiale li usa»**, e la differenza e' costata un giro intero:
// da #3679 la maschera di `M_HexCell` era collegata e ignorata (`UseConstant` rimasto acceso sul pin), e ogni test di
// questo file era verde con i piani sopra opachi. `CellMaterialCompilesTheInputsItHasConnected`, in fondo, chiude
// quella causa sull'asset; il resto — che si VEDA — lo chiude solo la PIE.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Materials/Material.h"
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	UInstancedStaticMeshComponent* LayerTreatmentIsm(const ARTHexMapActor* Board, const TCHAR* Name)
	{
		TArray<UInstancedStaticMeshComponent*> Isms;
		Board->GetComponents(Isms);
		for (UInstancedStaticMeshComponent* Ism : Isms)
		{
			if (Ism && Ism->GetName() == Name) { return Ism; }
		}
		return nullptr;
	}

	float LayerTreatmentChannel(const UInstancedStaticMeshComponent* Ism, int32 Instance, int32 Channel)
	{
		const int32 Indice = Instance * Ism->NumCustomDataFloats + Channel;
		return Ism->PerInstanceSMCustomData.IsValidIndex(Indice) ? Ism->PerInstanceSMCustomData[Indice] : -999.f;
	}

	/** La board della fixture a tre piani (#3678): 0 di raggio 4, 1 di raggio 2, 2 di raggio 1, impilati. */
	ARTHexMapActor* SpawnLayerTreatmentBoard(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Board = World->SpawnActorDeferred<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (Board)
		{
			Board->MapAsset = Asset;
			Board->FinishSpawning(FTransform::Identity); // `OnConstruction` ricostruisce le istanze
		}
		return Board;
	}

	int32 FirstInstanceOnLayer(const ARTHexMapActor* Board, int32 Layer)
	{
		for (int32 I = 0; I < Board->NumInstanceCells(); ++I)
		{
			if (Board->CellForInstance(I).Layer == Layer) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * **Il trattamento si ricava da (piano, piano attivo)** — la funzione pura della DoD di #3679.
 *
 * ⚠️ Il controllo «il secondo piano sotto e' piu' spento del primo» e' quello che rende il trattamento CRESCENTE con la
 * distanza, come chiede la issue: un trattamento uguale per tutti i piani sotto passerebbe gli altri controlli.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentPureTest,
	"RefactorTactics.HexMap.LayerTreatmentFollowsTheActiveLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentPureTest::RunTest(const FString&)
{
	const FRTLayerPresentationTuning T;

	TestTrue(TEXT("una mappa a un piano non ha trattamento"),
		RTLayerPresentation::Treatment(0, 0, /*bMultiLayer=*/ false, T).IsNeutral());
	TestTrue(TEXT("nemmeno su un piano che non e' l'attivo"),
		RTLayerPresentation::Treatment(1, 0, /*bMultiLayer=*/ false, T).IsNeutral());

	const FRTLayerTreatment Attivo = RTLayerPresentation::Treatment(1, 1, true, T);
	TestTrue(TEXT("l'attivo si vede come prima: opaco, colori pieni, la sua luce"), Attivo.IsNeutral());

	const FRTLayerTreatment Sopra = RTLayerPresentation::Treatment(2, 1, true, T);
	TestTrue(TEXT("sopra e' traslucido"), FMath::IsNearlyEqual(Sopra.Transparency, 1.f - T.UpperOpacity));
	TestTrue(TEXT("sopra non e' spento: si deve vedere attraverso, non leggerlo come lontano"),
		Sopra.Desaturation == 0.f && Sopra.ExposureStops >= 0.f);
	TestTrue(TEXT("sopra ha la luce che la taratura gli da'"), FMath::IsNearlyEqual(Sopra.ExposureStops, T.UpperExposureStops));

	// #3674: lo schiarimento del velo, con un valore DIVERSO dal default: una `Treatment` che ignorasse la taratura cade qui.
	// Tocca SOLO i piani sopra, non cambia l'opacita', e non scurisce mai.
	{
		FRTLayerPresentationTuning Chiaro;
		Chiaro.UpperExposureStops = 2.5f;
		// #3723: 1,5, il valore di prova di prima, e' diventato il default, e questo blocco non misurava piu' niente.
		TestFalse(TEXT("premessa: il valore di prova non e' il default"),
			FMath::IsNearlyEqual(Chiaro.UpperExposureStops, T.UpperExposureStops));
		const FRTLayerTreatment SopraChiaro = RTLayerPresentation::Treatment(2, 1, true, Chiaro);
		TestTrue(TEXT("schiarito: il piano sopra prende gli stop della leva"), FMath::IsNearlyEqual(SopraChiaro.ExposureStops, 2.5f));
		TestTrue(TEXT("schiarito: l'opacita' e' la stessa"), FMath::IsNearlyEqual(SopraChiaro.Transparency, Sopra.Transparency));
		TestTrue(TEXT("schiarito: l'attivo resta com'e'"), RTLayerPresentation::Treatment(1, 1, true, Chiaro).IsNeutral());
		TestTrue(TEXT("schiarito: un piano sotto resta com'e'"),
			FMath::IsNearlyEqual(RTLayerPresentation::Treatment(0, 1, true, Chiaro).ExposureStops,
				RTLayerPresentation::Treatment(0, 1, true, T).ExposureStops));
		TestTrue(TEXT("schiarito: una mappa a un piano resta senza trattamento"),
			RTLayerPresentation::Treatment(1, 0, /*bMultiLayer=*/ false, Chiaro).IsNeutral());

		Chiaro.UpperExposureStops = -2.f;
		TestEqual(TEXT("una leva negativa non scurisce il piano sopra"),
			RTLayerPresentation::Treatment(2, 1, true, Chiaro).ExposureStops, 0.f);
		Chiaro.UpperExposureStops = 99.f;
		TestEqual(TEXT("e una enorme si ferma al tetto"),
			RTLayerPresentation::Treatment(2, 1, true, Chiaro).ExposureStops, RTLayerPresentation::MaxUpperExposureStops);
	}

	const FRTLayerTreatment Sotto1 = RTLayerPresentation::Treatment(1, 2, true, T);
	const FRTLayerTreatment Sotto2 = RTLayerPresentation::Treatment(0, 2, true, T);
	TestEqual(TEXT("sotto e' opaco"), Sotto1.Transparency, 0.f);
	TestTrue(TEXT("un piano sotto: desaturato"), FMath::IsNearlyEqual(Sotto1.Desaturation, T.LowerDesaturationPerLayer));
	TestTrue(TEXT("un piano sotto: scurito"),
		FMath::IsNearlyEqual(Sotto1.ExposureStops, FMath::Log2(1.f - T.LowerDarkeningPerLayer), 1.e-4f));
	TestTrue(TEXT("due piani sotto: non meno desaturato di uno"), Sotto2.Desaturation >= Sotto1.Desaturation);
	TestTrue(TEXT("due piani sotto: piu' scuro di uno"), Sotto2.ExposureStops < Sotto1.ExposureStops);

	// #3723: al default, 1, un piano sotto e' gia' senza colore, e «di piu' con la distanza» li' non si puo' misurare.
	// La regola cumulata di D-501 si misura con una taratura a meta': una `Treatment` che non cumulasse cade qui.
	{
		FRTLayerPresentationTuning Graduale;
		Graduale.LowerDesaturationPerLayer = 0.5f;
		TestTrue(TEXT("graduale: un piano sotto perde meta' del colore"),
			FMath::IsNearlyEqual(RTLayerPresentation::Treatment(1, 2, true, Graduale).Desaturation, 0.5f));
		TestTrue(TEXT("graduale: due piani sotto ne perdono tre quarti"),
			FMath::IsNearlyEqual(RTLayerPresentation::Treatment(0, 2, true, Graduale).Desaturation, 0.75f));
	}

	// #3723: i valori dei piani li ha scelti l'autore il 2026-10-13, a esposizione fissa, sulle tavole della seduta U85.
	// Un default cambiato per sbaglio cade qui.
	TestTrue(TEXT("default sotto: desaturazione 1 per piano (U85)"), FMath::IsNearlyEqual(T.LowerDesaturationPerLayer, 1.f));
	TestTrue(TEXT("default sotto: scurimento 0,4 per piano (U85)"), FMath::IsNearlyEqual(T.LowerDarkeningPerLayer, 0.4f));
	// L'opacita' «sopra» resta quella della seduta U84, scelta sull'asset riparato: prima la maschera era inerte (#3704).
	TestTrue(TEXT("default sopra: opacita' 0,35 (U84, su piani traslucidi)"), FMath::IsNearlyEqual(T.UpperOpacity, 0.35f));
	// #3674 vuole il velo schiarito: a zero la piattaforma di `MultiLevel` non si distingueva. Quanto, e' di #3723.
	TestTrue(TEXT("default sopra: schiarimento 1,5 stop (U85)"), FMath::IsNearlyEqual(T.UpperExposureStops, 1.5f));
	TestTrue(TEXT("e il default e' uno schiarimento, sotto il tetto della leva"),
		T.UpperExposureStops > 0.f && T.UpperExposureStops < RTLayerPresentation::MaxUpperExposureStops);
	return true;
}

/**
 * **Il bordo dice il piano: nero l'attivo, bianco sopra, viola sotto** — la funzione pura della DoD di #3704 ([D-508]).
 *
 * ⚠️ Il controllo in scala di grigi e' §47-bis.1: il colore non porta il significato da solo, e i tre bordi devono
 * restare scuro, chiaro e medio anche senza tinta. Un viola troppo scuro o troppo chiaro cadrebbe qui, non in PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerBorderColorPureTest,
	"RefactorTactics.HexMap.BorderColorSaysTheLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerBorderColorPureTest::RunTest(const FString&)
{
	TestTrue(TEXT("su una mappa a un piano ogni piano e' l'attivo"),
		RTLayerPresentation::Relation(3, 0, /*bMultiLayer=*/ false) == ERTLayerRelation::Active);
	TestTrue(TEXT("il piano attivo"), RTLayerPresentation::Relation(1, 1, true) == ERTLayerRelation::Active);
	TestTrue(TEXT("un piano sopra"), RTLayerPresentation::Relation(2, 1, true) == ERTLayerRelation::Above);
	TestTrue(TEXT("due piani sopra: lo stesso"), RTLayerPresentation::Relation(3, 1, true) == ERTLayerRelation::Above);
	TestTrue(TEXT("un piano sotto"), RTLayerPresentation::Relation(0, 1, true) == ERTLayerRelation::Below);

	const FColor Nero = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Active);
	const FColor Bianco = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Above);
	const FColor Viola = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Below);
	TestTrue(TEXT("l'attivo ha il bordo di sempre"), Nero == URTHexLibrary::CellBorderColor());
	TestTrue(TEXT("sopra e' bianco"), Bianco == FColor(255, 255, 255));
	TestTrue(TEXT("sotto e' il viola di §32 (#7C5CFF)"), Viola == FColor(0x7C, 0x5C, 0xFF));

	// Luma sRGB (Rec. 601), cioe' come si legge in scala di grigi.
	auto Luma = [](const FColor& C) { return 0.299f * C.R + 0.587f * C.G + 0.114f * C.B; };
	TestTrue(*FString::Printf(TEXT("in grigio: nero %.0f < viola %.0f < bianco %.0f, con almeno 60 fra l'uno e l'altro"),
		Luma(Nero), Luma(Viola), Luma(Bianco)),
		Luma(Viola) - Luma(Nero) >= 60.f && Luma(Bianco) - Luma(Viola) >= 60.f);
	return true;
}

/**
 * 🔴 **Il piano attivo del CONTROLLER arriva alla board, e `ARTHexMapActor::ActiveLayer` resta com'e'** (#3679, scope 1).
 *
 * ⛔ Quel campo e' lo stato di authoring dell'editor mode (D-255): se il trattamento lo leggesse, o lo scrivesse, i due
 * scrittori su un campo solo sarebbero tornati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentFromControllerTest,
	"RefactorTactics.HexMap.ActiveLayerReachesTheBoardAsPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentFromControllerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("la board"), Board) || !TestNotNull(TEXT("il controller"), PC)) { return false; }
	UInstancedStaticMeshComponent* Cells = LayerTreatmentIsm(Board, TEXT("Cells"));
	UInstancedStaticMeshComponent* Bordi = LayerTreatmentIsm(Board, TEXT("CellBorders"));
	if (!TestNotNull(TEXT("le celle"), Cells) || !TestNotNull(TEXT("la griglia"), Bordi)) { return false; }

	const int32 Su0 = FirstInstanceOnLayer(Board, 0);
	const int32 Su1 = FirstInstanceOnLayer(Board, 1);
	const int32 Su2 = FirstInstanceOnLayer(Board, 2);
	if (!TestTrue(TEXT("premessa: istanze su tutti e tre i piani"), Su0 != INDEX_NONE && Su1 != INDEX_NONE && Su2 != INDEX_NONE)
		|| !TestEqual(TEXT("premessa: la griglia e' una per cella, nell'ordine delle celle"),
			Bordi->GetInstanceCount(), Cells->GetInstanceCount()))
	{
		return false;
	}
	const int32 Ch = RTLayerPresentation::FirstChannel;

	// Prima che il controller dica qualcosa la board e' neutra: e' la board di prima di #3679.
	TestEqual(TEXT("premessa: nessuno ha ancora detto il piano"), Board->GetPresentedActiveLayer(), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("premessa: il piano 2 nasce opaco"), LayerTreatmentChannel(Cells, Su2, Ch), 0.f);

	const int32 AuthoringPrima = Board->ActiveLayer;
	TestTrue(TEXT("il controller sale al piano 1"), PC->StepActiveLayer(+1));
	TestEqual(TEXT("la board presenta il piano 1"), Board->GetPresentedActiveLayer(), 1);
	TestEqual(TEXT("e il suo ActiveLayer d'authoring non e' cambiato"), Board->ActiveLayer, AuthoringPrima);

	const FRTLayerPresentationTuning T;
	TestTrue(TEXT("piano 2, sopra: traslucido"),
		FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su2, Ch), 1.f - T.UpperOpacity));
	TestTrue(TEXT("piano 2, sopra: schiarito degli stop della taratura (#3674)"),
		T.UpperExposureStops > 0.f && FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su2, Ch + 2), T.UpperExposureStops));
	TestEqual(TEXT("piano 1, attivo: opaco"), LayerTreatmentChannel(Cells, Su1, Ch), 0.f);
	TestEqual(TEXT("piano 1, attivo: la sua luce (#3674: lo schiarimento e' solo di chi sta sopra)"),
		LayerTreatmentChannel(Cells, Su1, Ch + 2), 0.f);
	TestEqual(TEXT("piano 1, attivo: colori pieni"), LayerTreatmentChannel(Cells, Su1, Ch + 1), 0.f);
	TestTrue(TEXT("piano 0, sotto: desaturato"),
		FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su0, Ch + 1), T.LowerDesaturationPerLayer));
	TestTrue(TEXT("piano 0, sotto: scurito"), LayerTreatmentChannel(Cells, Su0, Ch + 2) < 0.f);

	// #3704: i bordi dicono il piano col colore e restano opachi. La board non e' velata, quindi il fattore e' 1.
	auto ColoreBordo = [&](int32 Istanza)
	{
		return FLinearColor(LayerTreatmentChannel(Bordi, Istanza, 0), LayerTreatmentChannel(Bordi, Istanza, 1),
			LayerTreatmentChannel(Bordi, Istanza, 2));
	};
	auto Atteso = [](ERTLayerRelation R) { return FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(R)); };
	TestTrue(TEXT("bordo del piano 1, attivo: nero"), ColoreBordo(Su1).Equals(Atteso(ERTLayerRelation::Active), 1.e-4f));
	TestTrue(TEXT("bordo del piano 2, sopra: bianco"), ColoreBordo(Su2).Equals(Atteso(ERTLayerRelation::Above), 1.e-4f));
	TestTrue(TEXT("bordo del piano 0, sotto: viola"), ColoreBordo(Su0).Equals(Atteso(ERTLayerRelation::Below), 1.e-4f));
	for (int32 C = Ch; C < RTLayerPresentation::NumCustomDataFloats; ++C)
	{
		TestEqual(*FString::Printf(TEXT("il bordo sopra non ha trattamento: canale %d a zero (opaco)"), C),
			LayerTreatmentChannel(Bordi, Su2, C), 0.f);
		TestEqual(*FString::Printf(TEXT("il bordo sotto non ha trattamento: canale %d a zero (tiene il viola)"), C),
			LayerTreatmentChannel(Bordi, Su0, C), 0.f);
	}

	// Il cambio successivo sposta il trattamento: sceso al piano 0, il piano 1 e' sopra.
	TestTrue(TEXT("il controller scende al piano 0"), PC->StepActiveLayer(-1));
	TestEqual(TEXT("il piano 0 torna pieno"), LayerTreatmentChannel(Cells, Su0, Ch + 1), 0.f);
	TestTrue(TEXT("e il piano 1 e' traslucido"), LayerTreatmentChannel(Cells, Su1, Ch) > 0.f);
	TestTrue(TEXT("e schiarito, ora che sta sopra (#3674)"),
		FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su1, Ch + 2), T.UpperExposureStops));
	TestEqual(TEXT("mentre il piano 0, tornato attivo, riprende la sua luce"), LayerTreatmentChannel(Cells, Su0, Ch + 2), 0.f);
	TestTrue(TEXT("il bordo del piano 1 diventa bianco"), ColoreBordo(Su1).Equals(Atteso(ERTLayerRelation::Above), 1.e-4f));
	TestTrue(TEXT("e quello del piano 0 nero"), ColoreBordo(Su0).Equals(Atteso(ERTLayerRelation::Active), 1.e-4f));
	return true;
}

/**
 * 🔴 **Il velo viene prima: una cella mai vista resta non disegnata su qualunque piano** (#3679, vincolo 2).
 *
 * Il trattamento scrive i canali 3..5 e non la scala, e il velo nasconde azzerando la scala ([D-225]): una cella del
 * piano 2 mai vista resta a scala zero anche quando il piano 2 diventa traslucido. E i canali del colore, che sono del
 * velo, non cambiano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentKeepsTheVeilTest,
	"RefactorTactics.HexMap.LayerTreatmentLeavesTheVeilAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentKeepsTheVeilTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	// La squadra conosce i piani 0 e 1, non il 2.
	FRTTeamKnowledge Conosce;
	for (int32 I = 0; I < Board->NumInstanceCells(); ++I)
	{
		const FRTCellId Cella = Board->CellForInstance(I);
		if (Cella.Layer < 2)
		{
			Conosce.VisibleCells.Add(Cella);
			Conosce.ExploredCells.Add(Cella);
		}
	}
	Board->ApplyKnowledgeVeil(Conosce);

	const int32 Mai = FirstInstanceOnLayer(Board, 2);
	const int32 Vista = FirstInstanceOnLayer(Board, 0);
	FTransform Xf;
	if (!TestTrue(TEXT("premessa: la cella mai vista del piano 2 e' a scala zero"),
		Mai != INDEX_NONE && Cells->GetInstanceTransform(Mai, Xf, true) && Xf.GetScale3D().IsNearlyZero()))
	{
		return false;
	}
	float Colore[3];
	for (int32 C = 0; C < 3; ++C) { Colore[C] = LayerTreatmentChannel(Cells, Vista, C); }

	// Il piano attivo e' 0: il 2 sta sopra, cioe' traslucido.
	Board->SyncActiveLayerPresentation(0);
	TestTrue(TEXT("il trattamento e' arrivato al piano 2"), LayerTreatmentChannel(Cells, Mai, RTLayerPresentation::FirstChannel) > 0.f);
	TestTrue(TEXT("ma la cella mai vista resta a scala zero"),
		Cells->GetInstanceTransform(Mai, Xf, true) && Xf.GetScale3D().IsNearlyZero());
	for (int32 C = 0; C < 3; ++C)
	{
		TestEqual(*FString::Printf(TEXT("il canale %d del colore di una cella vista non cambia"), C),
			LayerTreatmentChannel(Cells, Vista, C), Colore[C]);
	}

	// #3704: anche il BORDO della cella mai vista resta non disegnato, benche' il suo colore dica «sopra». La griglia e'
	// una per cella e nell'ordine delle celle, quindi l'indice e' lo stesso.
	UInstancedStaticMeshComponent* Bordi = LayerTreatmentIsm(Board, TEXT("CellBorders"));
	if (TestNotNull(TEXT("la griglia"), Bordi)
		&& TestEqual(TEXT("premessa: la griglia e' una per cella"), Bordi->GetInstanceCount(), Cells->GetInstanceCount()))
	{
		TestTrue(TEXT("il bordo della cella mai vista resta a scala zero"),
			Bordi->GetInstanceTransform(Mai, Xf, true) && Xf.GetScale3D().IsNearlyZero());
		// Controllo positivo: il bordo di una cella vista del piano 1, sopra l'attivo, si disegna ed e' bianco.
		const int32 Sopra = FirstInstanceOnLayer(Board, 1);
		const FLinearColor Bianco = FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(ERTLayerRelation::Above));
		const FLinearColor Letto(LayerTreatmentChannel(Bordi, Sopra, 0), LayerTreatmentChannel(Bordi, Sopra, 1),
			LayerTreatmentChannel(Bordi, Sopra, 2));
		TestTrue(TEXT("controllo positivo: il bordo di una cella vista sopra l'attivo e' disegnato"),
			Bordi->GetInstanceTransform(Sopra, Xf, true) && !Xf.GetScale3D().IsNearlyZero());
		TestTrue(*FString::Printf(TEXT("ed e' bianco (letto %s)"), *Letto.ToString()), Letto.Equals(Bianco, 1.e-3f));
	}

	// E nell'altro verso: un velo dopo il trattamento non lo cancella. 🔴 Si guarda una cella che il velo RISCRIVE: una del
	// piano 1, sopra l'attivo, che passa da osservata a ricordata (rilievo della review). La cella mai vista non serviva: il
	// velo la salta, quindi qualunque velo avrebbe lasciato i suoi canali dov'erano.
	const int32 Ricordata = FirstInstanceOnLayer(Board, 1);
	if (!TestTrue(TEXT("premessa: c'e' una cella sul piano 1"), Ricordata != INDEX_NONE)) { return false; }
	float Trattamento[3];
	for (int32 C = 0; C < 3; ++C)
	{
		Trattamento[C] = LayerTreatmentChannel(Cells, Ricordata, RTLayerPresentation::FirstChannel + C);
	}
	TestTrue(TEXT("premessa: il piano 1 sta sopra l'attivo, quindi e' traslucido"), Trattamento[0] > 0.f);
	Conosce.VisibleCells.Reset();
	Board->ApplyKnowledgeVeil(Conosce);
	TestTrue(TEXT("premessa: il secondo velo ha toccato delle istanze"), Board->GetLastVeilTouchedCells() > 0);
	// #3760, seduta `U89`: dall'osservato al ricordo il colore sfuma. Si lascia finire la dissolvenza, e si legge il ricordo.
	static_cast<AActor*>(Board)->Tick(10.f);
	for (int32 C = 0; C < 3; ++C)
	{
		TestEqual(*FString::Printf(TEXT("il velo riscritto lascia il canale %d del trattamento dov'era"), RTLayerPresentation::FirstChannel + C),
			LayerTreatmentChannel(Cells, Ricordata, RTLayerPresentation::FirstChannel + C), Trattamento[C]);
	}

	// #3704 e #3760 (D-520): quando il velo riscrive il bordo di quella cella, ora RICORDATA, lo riscrive col colore della
	// pianta, a piena luminosita', su qualunque piano. Il trattamento del piano resta nei suoi canali, misurati qui sopra:
	// il colore e' del ricordo, la traslucenza del piano. Con il grigio scuro costante di prima di #3704, il bordo
	// tornerebbe scuro al primo velo.
	if (Bordi)
	{
		const FLinearColor Pianta = ARTHexMapActor::PlanColor();
		const FLinearColor Letto(LayerTreatmentChannel(Bordi, Ricordata, 0), LayerTreatmentChannel(Bordi, Ricordata, 1),
			LayerTreatmentChannel(Bordi, Ricordata, 2));
		TestTrue(*FString::Printf(TEXT("il velo riscrive il bordo del ricordo col colore della pianta (letto %s)"),
			*Letto.ToString()), Letto.Equals(Pianta, 1.e-3f));
	}
	return true;
}

/**
 * **I glifi ricevono il trattamento, anche quando un cambio di terreno li rimonta in partita** (#3679, rilievo della review).
 *
 * 🔴 `RepaintCells` toglie e rimette i glifi senza una ricostruzione, e un glifo appena montato nasce coi canali a zero,
 * cioe' pieno. Senza la riscrittura in coda a `RepaintCells`, il glifo nuovo di una cella su un piano traslucido resterebbe
 * opaco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentGlyphsTest,
	"RefactorTactics.HexMap.GlyphsTakeTheLayerTreatmentAlsoAfterARepaint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentGlyphsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// La cima ha due anelli (rilievo): e' l'unico glifo della mappa, quindi la sua istanza e' la sola del secondo anello.
	// ⚠️ Non il fumo: e' l'unica superficie con un volume, e `RepaintCells` rifiuta i cambi che muovono il volume, quindi il
	// velo ricostruirebbe invece di riallineare, e questo test non eserciterebbe il percorso per cella.
	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeLayerStackArena(World);
	const FRTCellId Cima(2, -2, 2);
	const FRTHexCellData* Dato = Asset ? Asset->FindCell(Cima) : nullptr;
	if (!TestNotNull(TEXT("premessa: la cima esiste"), Dato)) { return false; }
	FRTHexCellData Rilievo = *Dato;
	Rilievo.Surface = ERTHexSurface::HighGround;
	Asset->AddOrUpdateCell(Rilievo);
	Asset->SortCells();

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, Asset);
	UInstancedStaticMeshComponent* Anello1 = Board ? LayerTreatmentIsm(Board, TEXT("SurfaceGlyph2")) : nullptr;
	UInstancedStaticMeshComponent* Anello4 = Board ? LayerTreatmentIsm(Board, TEXT("SurfaceGlyph4")) : nullptr;
	if (!TestNotNull(TEXT("il secondo anello"), Anello1) || !TestNotNull(TEXT("il quarto anello"), Anello4)) { return false; }
	if (!TestEqual(TEXT("premessa: un solo glifo a due anelli, quello della cima"), Anello1->GetInstanceCount(), 1)
		|| !TestEqual(TEXT("premessa: nessun glifo a quattro anelli"), Anello4->GetInstanceCount(), 0))
	{
		return false;
	}

	// Tutto noto: il riallineamento per cella passa dal blocco di sincronizzazione del velo, che e' l'unico chiamante di
	// `RepaintCells` (privata), come in `HexMapActor.IncrementalRepaintEqualsFullRebuild`.
	FRTTeamKnowledge Tutto;
	for (const FRTHexCellData& C : Asset->Cells)
	{
		Tutto.VisibleCells.Add(C.Id);
		Tutto.ExploredCells.Add(C.Id);
	}
	Board->ApplyKnowledgeVeil(Tutto);
	Board->SyncActiveLayerPresentation(0); // la cima sta due piani sopra
	TestTrue(TEXT("il glifo della cima e' traslucido"), LayerTreatmentChannel(Anello1, 0, RTLayerPresentation::FirstChannel) > 0.f);

	// Il terreno cambia in partita: ghiaccio, quattro anelli. Il glifo passa al quarto anello senza ricostruzione.
	FRTHexCellData Ghiaccio = *Asset->FindCell(Cima);
	Ghiaccio.Surface = ERTHexSurface::Ice;
	Asset->AddOrUpdateCell(Ghiaccio);
	Board->ApplyKnowledgeVeil(Tutto);
	if (!TestTrue(TEXT("premessa: e' passato dal riallineamento per cella, non da una ricostruzione"),
			Board->LastRepaintTouchedInstances() > 0)
		|| !TestEqual(TEXT("premessa: il glifo e' passato al quarto anello"), Anello4->GetInstanceCount(), 1))
	{
		return false;
	}
	TestTrue(TEXT("il glifo rimontato e' traslucido come la sua cella"),
		LayerTreatmentChannel(Anello4, 0, RTLayerPresentation::FirstChannel) > 0.f);
	return true;
}

/**
 * **Una ricostruzione non riporta la board piatta** (#3679). Le istanze rinascono coi canali a zero, e la board le
 * riscrive col piano che aveva: la vista strategica, per esempio, ricostruisce tutto a ogni cambio di separazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSurvivesRebuildTest,
	"RefactorTactics.HexMap.RebuildKeepsTheLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSurvivesRebuildTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	Board->SyncActiveLayerPresentation(1);
	Board->RebuildInstances();

	const int32 Su2 = FirstInstanceOnLayer(Board, 2);
	const int32 Su0 = FirstInstanceOnLayer(Board, 0);
	TestTrue(TEXT("dopo la ricostruzione il piano 2 e' ancora traslucido"),
		LayerTreatmentChannel(Cells, Su2, RTLayerPresentation::FirstChannel) > 0.f);
	TestTrue(TEXT("e il piano 0 ancora desaturato"),
		LayerTreatmentChannel(Cells, Su0, RTLayerPresentation::FirstChannel + 1) > 0.f);

	// #3704: la ricostruzione riscrive la griglia col grigio di sempre; il bordo del piano 2 deve tornare bianco.
	if (UInstancedStaticMeshComponent* Bordi = LayerTreatmentIsm(Board, TEXT("CellBorders")))
	{
		const FLinearColor Bianco = FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(ERTLayerRelation::Above));
		TestTrue(TEXT("e il bordo del piano 2 e' ancora bianco"),
			FLinearColor(LayerTreatmentChannel(Bordi, Su2, 0), LayerTreatmentChannel(Bordi, Su2, 1),
				LayerTreatmentChannel(Bordi, Su2, 2)).Equals(Bianco, 1.e-4f));
	}
	return true;
}

/** **Una mappa a un piano si vede come prima di #3679**: nessun canale scritto, nemmeno l'enfasi del bordo (scope 6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSingleLayerTest,
	"RefactorTactics.HexMap.SingleLayerMapHasNoLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSingleLayerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(World);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius=*/ 2))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Asset->SortCells();
	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, Asset);
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	UInstancedStaticMeshComponent* Bordi = Board ? LayerTreatmentIsm(Board, TEXT("CellBorders")) : nullptr;
	if (!TestNotNull(TEXT("le celle"), Cells) || !TestNotNull(TEXT("la griglia"), Bordi)) { return false; }

	TestTrue(TEXT("la board accetta il piano"), Board->SyncActiveLayerPresentation(0));
	int32 Scritti = 0;
	for (const UInstancedStaticMeshComponent* Ism : { Cells, Bordi })
	{
		for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
		{
			for (int32 C = RTLayerPresentation::FirstChannel; C < RTLayerPresentation::NumCustomDataFloats; ++C)
			{
				Scritti += LayerTreatmentChannel(Ism, I, C) != 0.f ? 1 : 0;
			}
		}
	}
	TestEqual(TEXT("nessun canale del trattamento diverso da zero"), Scritti, 0);

	// #3704: i bordi sono quelli di sempre, e i fantasmi non hanno contorno.
	const FLinearColor Sempre = FLinearColor::FromSRGBColor(URTHexLibrary::CellBorderColor());
	int32 Diversi = 0;
	for (int32 I = 0; I < Bordi->GetInstanceCount(); ++I)
	{
		const FLinearColor Letto(LayerTreatmentChannel(Bordi, I, 0), LayerTreatmentChannel(Bordi, I, 1), LayerTreatmentChannel(Bordi, I, 2));
		Diversi += Letto.Equals(Sempre, 1.e-4f) ? 0 : 1;
	}
	TestEqual(TEXT("ogni bordo ha il grigio scuro di sempre"), Diversi, 0);

	FRTPlanPreview Piano;
	FRTPhasePreviewEntry Fase;
	Fase.PreviewDestination = FRTCellId(1, 0, 0);
	Fase.Certainty = ERTIntentCertainty::Confirmed;
	Piano.Phases.Add(Fase);
	Board->SetPlanPreview(Piano);
	if (UInstancedStaticMeshComponent* Contorni = LayerTreatmentIsm(Board, TEXT("PlanGhostBorders")))
	{
		TestFalse(TEXT("su un piano solo il fantasma non ha contorno, come prima di #3704"), Contorni->IsVisible());
	}
	else
	{
		AddError(TEXT("il componente dei contorni dei fantasmi non c'e'"));
	}
	return true;
}

/**
 * **Il contorno di un fantasma dice il piano della sua cella, e il riempimento resta la certezza** (#3704, [D-508]).
 *
 * Tre fantasmi su tre piani di `LayerStack`. Il controllo positivo e' il cambio di piano attivo: lo stesso fantasma cambia
 * contorno, e il suo riempimento no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerGhostBordersTest,
	"RefactorTactics.HexMap.PlanGhostBordersSayTheLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerGhostBordersTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	UInstancedStaticMeshComponent* Fantasmi = Board ? LayerTreatmentIsm(Board, TEXT("PlanGhosts")) : nullptr;
	UInstancedStaticMeshComponent* Contorni = Board ? LayerTreatmentIsm(Board, TEXT("PlanGhostBorders")) : nullptr;
	if (!TestNotNull(TEXT("i fantasmi"), Fantasmi) || !TestNotNull(TEXT("i loro contorni"), Contorni)) { return false; }

	// Un fantasma per piano, sulle tre colonne della pila: (0,0) a terra, (1,-1) sul mezzanino, (2,-2) in cima.
	FRTPlanPreview Piano;
	const FRTCellId Celle[] = { FRTCellId(0, 0, 0), FRTCellId(1, -1, 1), FRTCellId(2, -2, 2) };
	for (const FRTCellId& Cella : Celle)
	{
		FRTPhasePreviewEntry Fase;
		Fase.PreviewDestination = Cella;
		Fase.Certainty = ERTIntentCertainty::Confirmed;
		Piano.Phases.Add(Fase);
	}
	Board->SyncActiveLayerPresentation(1);
	Board->SetPlanPreview(Piano);
	if (!TestEqual(TEXT("un contorno per fantasma"), Contorni->GetInstanceCount(), Fantasmi->GetInstanceCount())
		|| !TestEqual(TEXT("premessa: tre fantasmi"), Fantasmi->GetInstanceCount(), 3))
	{
		return false;
	}
	TestTrue(TEXT("su piu' piani il contorno si vede"), Contorni->IsVisible());

	auto Colore = [](const UInstancedStaticMeshComponent* C, int32 I)
	{
		return FLinearColor(LayerTreatmentChannel(C, I, 0), LayerTreatmentChannel(C, I, 1), LayerTreatmentChannel(C, I, 2));
	};
	auto Atteso = [](ERTLayerRelation R) { return FLinearColor::FromSRGBColor(RTLayerPresentation::BorderColorFor(R)); };
	TestTrue(TEXT("fantasma a terra, sotto l'attivo: contorno viola"), Colore(Contorni, 0).Equals(Atteso(ERTLayerRelation::Below), 1.e-4f));
	TestTrue(TEXT("fantasma sul mezzanino, l'attivo: contorno nero"), Colore(Contorni, 1).Equals(Atteso(ERTLayerRelation::Active), 1.e-4f));
	TestTrue(TEXT("fantasma in cima, sopra l'attivo: contorno bianco"), Colore(Contorni, 2).Equals(Atteso(ERTLayerRelation::Above), 1.e-4f));

	// Il cambio di piano riscrive i contorni e non tocca i riempimenti.
	const FLinearColor Riempimento = Colore(Fantasmi, 1);
	Board->SyncActiveLayerPresentation(0);
	TestTrue(TEXT("sceso al piano 0, il fantasma del mezzanino ha il contorno bianco"),
		Colore(Contorni, 1).Equals(Atteso(ERTLayerRelation::Above), 1.e-4f));
	TestTrue(TEXT("e il suo riempimento e' ancora quello della certezza"), Colore(Fantasmi, 1).Equals(Riempimento, 1.e-6f));
	return true;
}

/**
 * 🔴 **Un raggio che attraversa un piano sopra quello attivo risolve sulla cella del piano attivo** (#3679, D-255).
 *
 * Il trattamento e' solo presentazione: la collisione delle celle non cambia, quindi il raggio colpisce ancora il
 * piano traslucido. E' il picking di D-255 a scartare quel colpo, perche' non e' sul piano attivo, e a ripiegare sul
 * piano attivo lungo lo stesso raggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentRayTest,
	"RefactorTactics.PlayerInput.ARayThroughATranslucentLayerResolvesOnTheActiveLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentRayTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeLayerStackArena(World);
	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, Asset);
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	const ECollisionEnabled::Type Prima = Cells->GetCollisionEnabled();
	Board->SyncActiveLayerPresentation(0);
	TestEqual(TEXT("il trattamento non tocca la collisione delle celle"),
		static_cast<int32>(Cells->GetCollisionEnabled()), static_cast<int32>(Prima));

	// Un raggio verticale sulla cima, (2,-2) al piano 2: il primo colpo e' li', due piani sopra l'attivo.
	FVector Origin; float HexSize = 0.f; float LayerH = 0.f;
	Board->GetHexContext(Origin, HexSize, LayerH);
	const FRTCellId Cima(2, -2, 2);
	const FVector Colpo = URTHexLibrary::AxialToWorld(Cima, Origin, HexSize, LayerH);
	const int32 PianoDelColpo = URTHexLibrary::WorldToLayer(Colpo.Z, Origin.Z, LayerH);
	TestEqual(TEXT("premessa: il colpo e' sul piano 2"), PianoDelColpo, 2);

	const bool bColpoValido = PianoDelColpo == 0; // la regola di `ResolveCellUnderCursor`: vale solo sul piano attivo
	const FRTCellId Risolta = URTHexLibrary::ResolveRayToCellOnLayer(Colpo + FVector(0, 0, 1000), FVector(0, 0, -1),
		Origin, HexSize, LayerH, /*ActiveLayer=*/ 0, bColpoValido, Colpo);
	TestEqual(TEXT("si risolve sul piano attivo"), Risolta.Layer, 0);
	TestTrue(TEXT("sotto la cima, non sulla cima"), Risolta.X == Cima.X && Risolta.Y == Cima.Y);
	TestTrue(TEXT("e la cella esiste sul piano attivo"), Asset->ContainsCell(Risolta));
	return true;
}

/**
 * **Le unita' non ricevono il trattamento** (#3679, scope 5): anello, targa e modello restano pieni su qualunque
 * piano. Il trattamento scrive solo le famiglie della board; qui si guarda che i componenti di un'unita' sul piano 2
 * — sopra l'attivo, quindi traslucido — escano dalla scrittura come ci sono entrati. E i fantasmi del piano, che usano
 * lo stesso materiale delle celle, non hanno i canali del trattamento: restano pieni come le unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSparesUnitsTest,
	"RefactorTactics.HexMap.UnitsDoNotTakeTheLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSparesUnitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }
	FVector Origin; float HexSize = 0.f; float LayerH = 0.f;
	Board->GetHexContext(Origin, HexSize, LayerH);

	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("l'unita'"), Unit)) { return false; }
	Unit->TeamId = 0;
	Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
	UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
	Unit->DispatchBeginPlay();
	Unit->PlaceOnCell(FRTCellId(2, -2, 2), Origin, HexSize, LayerH);

	// Lo stato di ogni componente visivo dell'unita': dati primitivi per istanza e materiali.
	struct FStato { TArray<float> Dati; TArray<UMaterialInterface*> Materiali; };
	auto Fotografa = [Unit]()
	{
		TMap<const UPrimitiveComponent*, FStato> Out;
		TArray<UPrimitiveComponent*> Primitivi;
		Unit->GetComponents(Primitivi);
		for (const UPrimitiveComponent* P : Primitivi)
		{
			FStato S;
			S.Dati = P->GetCustomPrimitiveData().Data;
			for (int32 M = 0; M < P->GetNumMaterials(); ++M) { S.Materiali.Add(P->GetMaterial(M)); }
			Out.Add(P, S);
		}
		return Out;
	};
	const TMap<const UPrimitiveComponent*, FStato> Prima = Fotografa();
	if (!TestTrue(TEXT("premessa: l'unita' ha componenti visivi"), Prima.Num() > 0)) { return false; }

	Board->SyncActiveLayerPresentation(0); // l'unita' sta due piani sopra l'attivo
	const TMap<const UPrimitiveComponent*, FStato> Dopo = Fotografa();
	for (const TPair<const UPrimitiveComponent*, FStato>& P : Prima)
	{
		const FStato* D = Dopo.Find(P.Key);
		TestTrue(*FString::Printf(TEXT("%s: dati e materiali invariati"), *P.Key->GetName()),
			D != nullptr && D->Dati == P.Value.Dati && D->Materiali == P.Value.Materiali);
	}

	const UInstancedStaticMeshComponent* Fantasmi = LayerTreatmentIsm(Board, TEXT("PlanGhosts"));
	if (TestNotNull(TEXT("i fantasmi del piano"), Fantasmi))
	{
		TestTrue(TEXT("i fantasmi non hanno i canali del trattamento"),
			Fantasmi->NumCustomDataFloats <= RTLayerPresentation::FirstChannel);
	}
	return true;
}

#if WITH_EDITOR
/**
 * 🔴 **Un ingresso COLLEGATO che il traduttore ignora** (#3704). E' il difetto che ha tenuto opachi i piani sopra
 * da #3679 in poi, con tutti i test dei canali verdi.
 *
 * Ogni ingresso del nodo radice porta due cose: l'espressione collegata e `UseConstant`, la costante in linea che
 * il Material Editor accende sui pin SCOLLEGATI. `CompileWithDefault` guarda `UseConstant` PER PRIMO: se e' acceso
 * compila la costante, e l'espressione collegata non viene nemmeno letta. Il Material Editor lo spegne quando si
 * tira il filo; `UMaterialEditingLibrary::ConnectMaterialProperty` — cioe' `connect_to_output` del bridge — no.
 * Risultato misurato il 2026-10-12 sul codice generato: `PixelMaterialInputs.OpacityMask = 1.0` con il nodo
 * `DitherTemporalAA` collegato, quindi nessuno shader di profondita' mascherato e nessun taglio.
 *
 * ⚠️ **Qui si misura l'asset, non lo shader**: la suite gira in `-NullRHI` e la mappa degli shader non c'e'. Che a
 * schermo un piano sopra lasci vedere sotto resta della voce PIE, col suo controllo positivo (a opacita' 0 il
 * piano sopra SPARISCE): questo test toglie soltanto la causa che la faceva fallire in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellMaterialCompilesItsInputsTest,
	"RefactorTactics.HexMap.CellMaterialCompilesTheInputsItHasConnected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCellMaterialCompilesItsInputsTest::RunTest(const FString&)
{
	UMaterialInterface* Interfaccia = GetDefault<ARTHexMapActor>()->CellMaterial.LoadSynchronous();
	UMaterial* Materiale = Interfaccia ? Interfaccia->GetMaterial() : nullptr;
	if (!TestNotNull(TEXT("premessa: il materiale delle celle si carica"), Materiale)) { return false; }

	TestTrue(TEXT("il materiale delle celle e' Masked: e' la maschera che fa vedere sotto un piano sopra"),
		Materiale->GetBlendMode() == BLEND_Masked);

	int32 Collegati = 0;
	for (int32 P = 0; P < MP_MAX; ++P)
	{
		FMaterialInputDescription Ingresso;
		if (!Materiale->GetExpressionInputDescription(EMaterialProperty(P), Ingresso)
			|| Ingresso.Input == nullptr || Ingresso.Input->Expression == nullptr)
		{
			continue;
		}
		++Collegati;
		TestFalse(*FString::Printf(TEXT("%s ha un nodo collegato, quindi la costante in linea e' spenta (altrimenti il nodo non si compila)"),
			*UEnum::GetValueAsString(EMaterialProperty(P))), Ingresso.bUseConstant);
	}

	// I due ingressi su cui il trattamento dei piani si regge, per nome: senza, il ciclo sopra passerebbe a vuoto.
	for (const EMaterialProperty Atteso : { MP_EmissiveColor, MP_OpacityMask })
	{
		FMaterialInputDescription Ingresso;
		const bool bCollegato = Materiale->GetExpressionInputDescription(Atteso, Ingresso)
			&& Ingresso.Input != nullptr && Ingresso.Input->Expression != nullptr;
		TestTrue(*FString::Printf(TEXT("%s ha un nodo collegato"), *UEnum::GetValueAsString(Atteso)), bCollegato);
	}
	TestTrue(TEXT("premessa: il ciclo ha guardato almeno i due ingressi attesi"), Collegati >= 2);
	return true;
}
#endif // WITH_EDITOR

#endif // WITH_DEV_AUTOMATION_TESTS
