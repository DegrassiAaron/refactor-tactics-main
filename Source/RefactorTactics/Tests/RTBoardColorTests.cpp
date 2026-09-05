// Copyright RefactorTactics. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTBoardColorTests
{
	/**
	 * Il componente delle celle si prende dall'API PUBBLICA degli attori, non allargando la visibilità di
	 * un membro protetto: questa issue aggiunge un oracolo, non cambia la superficie di `Map/`.
	 */
	UInstancedStaticMeshComponent* FindCells(AActor* Actor)
	{
		TArray<UInstancedStaticMeshComponent*> Found;
		Actor->GetComponents<UInstancedStaticMeshComponent>(Found);
		for (UInstancedStaticMeshComponent* C : Found)
		{
			if (C && C->GetName() == TEXT("Cells")) { return C; }
		}
		return nullptr;
	}
}

/**
 * DOPO `RebuildInstances` ALMENO UN'ISTANZA HA CUSTOM DATA DIVERSO DA ZERO — `#1665`.
 *
 * 🔑 **È l'oracolo che oggi non esiste, e la sua assenza è metà del difetto.** `M_HexCell` legge i tre
 * `PerInstanceCustomData` come **Emissive**: se restano a zero le celle sono **nere**, e nero non è
 * nessuno dei due esiti che il codice dichiara — materiale assente dà **grigio**, materiale presente con
 * dati validi dà **colorato**. Fino a qui la differenza si vedeva solo con gli occhi, e solo nel
 * pacchetto.
 *
 * ⚠️ **Questo test non riproduce il difetto del pacchetto**, e va detto: gira nell'editor, dove
 * `OnConstruction` chiama `RebuildInstances` per la strada che l'editor usa. Se domani quella strada si
 * rompe *qui*, il test lo dice; se si rompe **solo in cooked**, non lo dice — è la ragione per cui la
 * issue chiede anche uno screenshot dal pacchetto, e per cui quella metà resta separata.
 *
 * ⛔ Non allarga la visibilità di `Cells`: il componente si trova per nome dall'API pubblica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoardCellsHaveCustomDataTest,
	"RefactorTactics.Map.BoardCellsCarryColorCustomData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoardCellsHaveCustomDataTest::RunTest(const FString&)
{
	using namespace RTBoardColorTests;

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!MapActor)
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
	MapActor->RebuildInstances();

	UInstancedStaticMeshComponent* Cells = FindCells(MapActor);
	if (!TestNotNull(TEXT("il componente delle celle esiste"), Cells))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const int32 Instances = Cells->GetInstanceCount();
	if (!TestTrue(FString::Printf(TEXT("la board ha istanze (%d)"), Instances), Instances > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// ⚠️ Prima dei valori: lo SPAZIO dove scriverli. `AddInstance` alloca i float alla creazione
	// dell'istanza, e un `NumCustomDataFloats` a zero renderebbe ogni `SetCustomDataValue` un no-op
	// silenzioso — il difetto si presenterebbe come «tutti zero» senza che nessuna riga abbia sbagliato.
	TestEqual(TEXT("tre float per istanza, come il materiale si aspetta"),
		Cells->NumCustomDataFloats, 3);

	const TArray<float>& Data = Cells->PerInstanceSMCustomData;
	if (!TestEqual(TEXT("un float per istanza per canale"), Data.Num(), Instances * 3))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	int32 NonZero = 0;
	for (const float V : Data)
	{
		if (V != 0.f) { ++NonZero; }
	}

	// 🔑 L'asserzione della issue: **almeno uno**. Non «tutti», perché un colore può legittimamente avere
	// un canale a zero, e pretendere che nessuno lo sia renderebbe il test rosso su una palette valida.
	TestTrue(FString::Printf(TEXT("almeno un canale scritto (%d su %d non nulli)"), NonZero, Data.Num()),
		NonZero > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
