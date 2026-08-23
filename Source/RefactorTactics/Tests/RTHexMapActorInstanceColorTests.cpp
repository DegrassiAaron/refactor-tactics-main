#include "Misc/AutomationTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/Material.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il colore della superficie arriva alla singola ISTANZA, non al componente.
 *
 * Cosa NON verifica, e va detto: che a schermo si veda qualcosa. I custom data sono inerti finche' il
 * materiale della mesh non legge `PerInstanceCustomData` — in `M_HexCell` sull'**Emissive**, non sul Base
 * Color come questa riga ha dichiarato fino al 2026-08-23 (#956) — e col materiale di default del
 * motore le celle restano grigie comunque. Quel pezzo e' un `.uasset` e si verifica a occhio.
 * Qui si prova l'unica cosa che il codice puo' sbagliare da solo: quale colore finisce su quale istanza.
 */
namespace
{
	// Nome distinto: i namespace anonimi delle unity build si fondono, e due helper omonime sarebbero
	// una ridefinizione.
	UWorld* MakeInstanceColorWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyInstanceColorWorld(UWorld* World)
	{
		if (!World) { return; }
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	}

	FRTHexCellData MakeCell(int32 Q, int32 R, ERTHexSurface Surface)
	{
		FRTHexCellData C;
		C.Id = FRTCellId(Q, R, 0);
		C.Surface = Surface;
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorInstanceColorTest,
	"RefactorTactics.HexMap.InstanceColorFollowsSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorInstanceColorTest::RunTest(const FString&)
{
	UWorld* World = MakeInstanceColorWorld();
	if (!TestNotNull(TEXT("mondo di test creato"), World))
	{
		return false;
	}

	// Tre superfici deliberatamente diverse fra loro: se il colore fosse preso una volta sola e riusato,
	// tre celle uguali non lo direbbero.
	const TArray<ERTHexSurface> Expected = {
		ERTHexSurface::Floor, ERTHexSurface::Rough, ERTHexSurface::Ice
	};

	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (int32 I = 0; I < Expected.Num(); ++I)
	{
		Map->AddOrUpdateCell(MakeCell(I, 0, Expected[I]));
	}

	ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("actor mappa creato"), Actor))
	{
		DestroyInstanceColorWorld(World);
		return false;
	}
	Actor->MapAsset = Map;
	Actor->RebuildInstances();

	// `Cells` e' il root component (lo dichiara il costruttore con SetRootComponent), quindi non serve
	// esporre il membro per leggerlo.
	UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Actor->GetRootComponent());
	if (!TestNotNull(TEXT("il root e' l'ISM delle celle"), ISM))
	{
		DestroyInstanceColorWorld(World);
		return false;
	}

	TestEqual(TEXT("tre float per istanza (RGB)"), ISM->NumCustomDataFloats, 3);
	TestEqual(TEXT("un'istanza per cella"), ISM->GetInstanceCount(), Expected.Num());
	TestEqual(TEXT("i custom data sono 3 per istanza"),
		ISM->PerInstanceSMCustomData.Num(), Expected.Num() * 3);

	if (ISM->PerInstanceSMCustomData.Num() != Expected.Num() * 3)
	{
		DestroyInstanceColorWorld(World);
		return false;
	}

	for (int32 I = 0; I < Expected.Num(); ++I)
	{
		// La conversione e' parte del contratto: `SurfaceColor` e' sRGB a 8 bit, il canale del materiale e'
		// lineare. (Il canale e' l'Emissive: verificato nel grafo di `M_HexCell` il 2026-08-23.)
		// Un `/255.f` al posto di `FromSRGBColor` passerebbe un test scritto sullo stesso errore, quindi
		// l'atteso si costruisce dalla tavolozza + la conversione dichiarata, non dai numeri.
		const FLinearColor Want = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(Expected[I]));
		const float R = ISM->PerInstanceSMCustomData[I * 3 + 0];
		const float G = ISM->PerInstanceSMCustomData[I * 3 + 1];
		const float B = ISM->PerInstanceSMCustomData[I * 3 + 2];

		const FString Where = FString::Printf(TEXT("istanza %d"), I);
		TestNearlyEqual(*(Where + TEXT(" R")), R, Want.R, KINDA_SMALL_NUMBER);
		TestNearlyEqual(*(Where + TEXT(" G")), G, Want.G, KINDA_SMALL_NUMBER);
		TestNearlyEqual(*(Where + TEXT(" B")), B, Want.B, KINDA_SMALL_NUMBER);
	}

	// Due superfici diverse devono produrre due colori diversi *sulle istanze*: senza questo, un bug che
	// scrivesse sempre la prima cella passerebbe i confronti qui sopra solo se la tavolozza fosse piatta.
	const float FirstR = ISM->PerInstanceSMCustomData[0];
	const float SecondR = ISM->PerInstanceSMCustomData[3];
	TestNotEqual(TEXT("Floor e Rough non finiscono con lo stesso rosso"), FirstR, SecondR);

	DestroyInstanceColorWorld(World);
	return true;
}

/**
 * Il materiale delle celle si applica dal CODICE, non dal livello.
 *
 * Perche' e' questo il comportamento da fissare: `MapSource = GeneratedTestArena` costruisce la mappa a
 * runtime, quindi non esiste nessun livello dove assegnare il materiale. Un'assegnazione fatta solo
 * sull'actor di `L_HexArena` funzionerebbe li' e lascerebbe grigie le sessioni che usano l'arena generata —
 * cioe' la maggior parte dei playtest.
 *
 * Il test non usa `M_HexCell`: userebbe l'asset reale per provare il cablaggio, e fallirebbe per un motivo
 * sbagliato ovunque quell'asset non ci sia. Basta un materiale qualunque.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorCellMaterialTest,
	"RefactorTactics.HexMap.CellMaterialAppliedFromCode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorCellMaterialTest::RunTest(const FString&)
{
	UWorld* World = MakeInstanceColorWorld();
	if (!TestNotNull(TEXT("mondo di test creato"), World))
	{
		return false;
	}

	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(MakeCell(0, 0, ERTHexSurface::Floor));

	ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("actor mappa creato"), Actor))
	{
		DestroyInstanceColorWorld(World);
		return false;
	}

	UMaterial* Probe = UMaterial::GetDefaultMaterial(MD_Surface);
	Actor->MapAsset = Map;
	Actor->CellMaterial = Probe;
	Actor->RebuildInstances();

	UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Actor->GetRootComponent());
	if (!TestNotNull(TEXT("il root e' l'ISM delle celle"), ISM))
	{
		DestroyInstanceColorWorld(World);
		return false;
	}

	TestEqual(TEXT("RebuildInstances applica CellMaterial allo slot 0"), ISM->GetMaterial(0), (UMaterialInterface*)Probe);

	DestroyInstanceColorWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
