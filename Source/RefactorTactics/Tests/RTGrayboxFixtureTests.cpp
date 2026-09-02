#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Map/RTHexLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Unit/RTUnit.h"
#include "World/RTGrayboxUnitFacingFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Stesso idioma di `RTBotAllyTests`: un mondo di prova, creato e distrutto dal test. */
	UWorld* MakeGrayboxFixtureWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyGrayboxFixtureWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * 🔴 **Il marker del fixture VIENE dalla libreria, e questo test lo prova sull'attore vero.**
 *
 * 🔑 **E' il buco che la spec di #1992 dichiarava incolmabile.** Con un Blueprint puro la sua sezione
 * Automation ammetteva: *«sei test coprono la formula, non che il Blueprint la chiami; un fixture che
 * calcolasse l'origine per conto proprio li lascerebbe tutti verdi»*. Da quando la geometria sta in
 * `ARTGrayboxUnitFacingFixture`, l'attore si **spawna** e il componente posato si confronta col valore
 * vero: un fixture che si calcolasse l'origine per conto proprio adesso fallisce qui.
 *
 * ⚠️ L'atteso non e' un letterale: si ricostruisce chiamando `URTHexLibrary`, cioe' per la stessa strada
 * che il fixture deve seguire. Se la convenzione dei sei lati cambiasse, cambierebbero insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureMarkerTest,
	"RefactorTactics.Graybox.FixtureMarkerComesFromTheLibrary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureMarkerTest::RunTest(const FString&)
{
	UWorld* World = MakeGrayboxFixtureWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTGrayboxUnitFacingFixture* Fixture =
		World->SpawnActor<ARTGrayboxUnitFacingFixture>(FVector::ZeroVector, FRotator::ZeroRotator);
	// ⚠️ `.Get()`: `TestNotNull` prende un puntatore nudo, e un `TObjectPtr` non ha overload.
	if (!TestNotNull(TEXT("il fixture si posa"), Fixture) || !TestNotNull(TEXT("ha il marker"), Fixture->FacingMarker.Get()))
	{
		DestroyGrayboxFixtureWorld(World);
		return false;
	}

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		Fixture->Facing = Dir;
		Fixture->RerunConstructionScripts();

		const FVector  Origin   = URTHexLibrary::FacingMarkerOrigin(Dir, FVector::ZeroVector,
			Fixture->BodyRadius, Fixture->FaceHeight);
		const FRotator Rotation = URTHexLibrary::FacingRotation(Dir);
		const FVector  Expected = Origin + Rotation.Vector() * (static_cast<double>(Fixture->MarkerLength) * 0.5);

		TestTrue(*FString::Printf(TEXT("direzione %d: il marker sta dove dice la libreria"), D),
			Fixture->FacingMarker->GetRelativeLocation().Equals(Expected, 0.01));
		TestTrue(*FString::Printf(TEXT("direzione %d: il marker guarda dove dice la libreria"), D),
			Fixture->FacingMarker->GetRelativeRotation().Equals(Rotation, 0.01f));
	}

	// ⚠️ `MarkerLength` NON entra nell'origine: cambiandola il marker si allunga e il punto in cui
	// COMINCIA non si muove. E' la proprieta' che rende la lunghezza una misura invece di una somma.
	Fixture->Facing = ERTHexDirection::E;
	Fixture->MarkerLength = 40.f;
	Fixture->RerunConstructionScripts();
	const FVector ShortStart = Fixture->FacingMarker->GetRelativeLocation()
		- Fixture->FacingMarker->GetRelativeRotation().Vector() * 20.0;

	Fixture->MarkerLength = 120.f;
	Fixture->RerunConstructionScripts();
	const FVector LongStart = Fixture->FacingMarker->GetRelativeLocation()
		- Fixture->FacingMarker->GetRelativeRotation().Vector() * 60.0;

	TestTrue(TEXT("allungando il marker l'origine non si sposta"), ShortStart.Equals(LongStart, 0.01));

	DestroyGrayboxFixtureWorld(World);
	return true;
}

/**
 * ⛔ **Il root e' neutro.** E' #593, il difetto che *«non fallisce: deforma in silenzio tutto cio' che gli
 * si attacca sotto»* — una Skeletal Mesh stirata di `1.5x` senza che niente suoni.
 *
 * ⚠️ Si guarda la scala del ROOT COMPONENT, non quella dell'attore: sono cose diverse, e posare l'attore
 * scalato e' una scelta legittima di chi allestisce. Cio' che non deve accadere e' che la deformazione sia
 * incisa nel fixture.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureRootTest,
	"RefactorTactics.Graybox.FixtureRootIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureRootTest::RunTest(const FString&)
{
	const ARTGrayboxUnitFacingFixture* CDO = GetDefault<ARTGrayboxUnitFacingFixture>();
	if (!TestNotNull(TEXT("il CDO esiste"), CDO) || !TestNotNull(TEXT("ha un root"), CDO->SceneRoot.Get()))
	{
		return false;
	}

	const FVector Scale = CDO->SceneRoot->GetRelativeScale3D();
	TestTrue(TEXT("la scala del root e' unitaria"), Scale.Equals(FVector::OneVector, 1.e-4));

	// ⛔ E non e' un `ARTUnit`: la spec lo vieta esplicitamente, perche' un'unita' vera passa da altro.
	TestFalse(TEXT("il fixture non e' un'unita'"),
		ARTGrayboxUnitFacingFixture::StaticClass()->IsChildOf(ARTUnit::StaticClass()));
	return true;
}

/**
 * Il corpo POGGIA sul piano del root invece di starci a meta' dentro.
 *
 * Il cilindro engine e' centrato sulla propria altezza: chi lo scala e lo lascia all'origine ottiene un
 * corpo sepolto a meta', e il facing sembra partire dal suolo. Il difetto non fallisce da nessuna parte —
 * si vede soltanto, e a quel punto sembra un problema del marker.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureBodyTest,
	"RefactorTactics.Graybox.FixtureBodySitsOnThePlane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureBodyTest::RunTest(const FString&)
{
	constexpr float Radius = 60.f;
	constexpr float Height = 180.f;
	const FTransform Body = ARTGrayboxUnitFacingFixture::BodyTransform(Radius, Height);

	// La primitiva engine e' alta 100 a scala 1: la mezza-altezza vale `100 * ScaleZ / 2`.
	const double HalfHeight = 100.0 * Body.GetScale3D().Z * 0.5;
	TestTrue(TEXT("la base del corpo tocca il piano"),
		FMath::IsNearlyEqual(Body.GetLocation().Z - HalfHeight, 0.0, 0.01));
	TestTrue(TEXT("la sommita' del corpo e' l'altezza dichiarata"),
		FMath::IsNearlyEqual(Body.GetLocation().Z + HalfHeight, static_cast<double>(Height), 0.01));
	TestTrue(TEXT("la pianta e' circolare: X e Y hanno la stessa scala"),
		FMath::IsNearlyEqual(Body.GetScale3D().X, Body.GetScale3D().Y, 1.e-6));
	return true;
}

/**
 * 🔴 **Il fixture INDOSSA i suoi materiali, e corpo e marker non sono lo stesso.**
 *
 * Nasce dal difetto visto a schermo: senza materiale le primitive engine prendono il grigio di default —
 * quello del pavimento — e il fixture spariva. ⚠️ *«I materiali esistono»* non e' *«il fixture li porta»*:
 * la separazione dei valori e' verificata altrove (`FixtureMaterialsSeparateBodyAndMarker`), qui si
 * verifica il legame, che e' la parte che si era rotta.
 *
 * ⚠️ **La risoluzione avviene in `OnConstruction`, non nel costruttore**, e questo test copre proprio
 * quella scelta: con un `ConstructorHelpers::FObjectFinder` il CDO si costruisce prima che il commandlet
 * crei gli asset, i tre path fallivano — *«Failed to find MI_Graybox_Fixture_Body»* — e il fixture nasceva
 * grigio. Misurato eseguendo, non previsto leggendo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureMaterialsWornTest,
	"RefactorTactics.Graybox.FixtureWearsItsMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureMaterialsWornTest::RunTest(const FString&)
{
	UWorld* World = MakeGrayboxFixtureWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTGrayboxUnitFacingFixture* Fixture =
		World->SpawnActor<ARTGrayboxUnitFacingFixture>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("il fixture si posa"), Fixture))
	{
		DestroyGrayboxFixtureWorld(World);
		return false;
	}
	Fixture->RerunConstructionScripts();

	// 🔴 **Si confronta con QUALE materiale, non con «ce n'e' uno».**
	//
	// ⚠️ La prima stesura asseriva `GetMaterial(0) != nullptr` ed era **vacua**: senza override quel metodo
	// restituisce il materiale di DEFAULT DELLA MESH, che non e' mai nullo. Misurato con una mutazione —
	// tolta l'assegnazione al corpo, il test restava verde. Cadeva anche la seconda asserzione, «corpo e
	// marker diversi»: il corpo prendeva il default della mesh e il marker il suo, quindi differivano
	// comunque. Due asserzioni, zero copertura.
	UMaterialInterface* ExpectedBody   = Fixture->BodyMaterial.LoadSynchronous();
	UMaterialInterface* ExpectedMarker = Fixture->MarkerMaterial.LoadSynchronous();
	if (!TestNotNull(TEXT("il materiale del corpo si risolve dal path"), ExpectedBody)
		|| !TestNotNull(TEXT("il materiale del marker si risolve dal path"), ExpectedMarker))
	{
		DestroyGrayboxFixtureWorld(World);
		return false;
	}

	TestEqual(TEXT("il corpo porta il PROPRIO materiale, non il default della mesh"),
		Fixture->UnitBody ? Fixture->UnitBody->GetMaterial(0) : nullptr, ExpectedBody);
	TestEqual(TEXT("il marker porta il PROPRIO materiale"),
		Fixture->FacingMarker ? Fixture->FacingMarker->GetMaterial(0) : nullptr, ExpectedMarker);

	// ⛔ E i due non sono lo stesso: a schermo il marker sparirebbe dentro il corpo invece che dentro il
	// pavimento — lo stesso difetto, spostato di un componente.
	TestTrue(TEXT("corpo e marker non portano lo stesso materiale"), ExpectedBody != ExpectedMarker);

	DestroyGrayboxFixtureWorld(World);
	return true;
}

/**
 * 🔴 **Il marker sta in ALTO sul corpo, non alla base.**
 *
 * Segnalato guardandolo: *«posizionandolo vicino la base lo rende poco leggibile»*. La quota era `24` su
 * un corpo alto `180`, cioe' schiacciata fra il disco a terra e il pavimento — dove nessun contrasto
 * salva un marker.
 *
 * ⚠️ **Il difetto non era il numero, era la sua provenienza**: `24` veniva da `WedgeLocalZ` di
 * `RTScenarioPreviewActor`, dove pero' il cuneo sta a `WedgeForward = 78` — FUORI dal corpo, in
 * un'anteprima con un'altra camera. Un default «derivato» da un contesto diverso non e' derivato: e'
 * copiato, e questo test e' il guardiano contro il prossimo che lo ricopia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureMarkerHeightTest,
	"RefactorTactics.Graybox.FixtureMarkerSitsHighOnTheBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureMarkerHeightTest::RunTest(const FString&)
{
	const ARTGrayboxUnitFacingFixture* CDO = GetDefault<ARTGrayboxUnitFacingFixture>();
	if (!TestNotNull(TEXT("il CDO esiste"), CDO))
	{
		return false;
	}

	// La meta' alta del corpo: sotto, il marker compete col disco a terra e col pavimento.
	TestTrue(*FString::Printf(TEXT("il marker sta sopra meta' corpo (%.0f su %.0f)"), CDO->FaceHeight, CDO->BodyHeight),
		CDO->FaceHeight > CDO->BodyHeight * 0.5f);

	// ⛔ E non esce dalla sommita': un marker che levita sopra il corpo non ne indica piu' il facing.
	TestTrue(TEXT("il marker resta sul corpo, non sopra"), CDO->FaceHeight < CDO->BodyHeight);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
