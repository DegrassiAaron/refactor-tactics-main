#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Map/RTHexLibrary.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
