#include "Misc/AutomationTest.h"
#include "Turn/RTPlaybackLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Tolleranza per confronti float nei test del playback.
	constexpr float RTTol = 0.001f;
}

// --- InterpolateAlongPath -------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackInterpEndpointsTest,
	"RefactorTactics.Playback.InterpolateEndpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackInterpEndpointsTest::RunTest(const FString&)
{
	const TArray<FVector> Path = { FVector(0, 0, 0), FVector(100, 0, 0) };
	TestTrue(TEXT("Alpha 0 = primo waypoint"),
		URTPlaybackLibrary::InterpolateAlongPath(Path, 0.f).Equals(FVector(0, 0, 0), RTTol));
	TestTrue(TEXT("Alpha 1 = ultimo waypoint"),
		URTPlaybackLibrary::InterpolateAlongPath(Path, 1.f).Equals(FVector(100, 0, 0), RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackInterpMidpointTest,
	"RefactorTactics.Playback.InterpolateMidpoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackInterpMidpointTest::RunTest(const FString&)
{
	// Un solo segmento: Alpha 0.5 = punto medio.
	const TArray<FVector> One = { FVector(0, 0, 0), FVector(100, 0, 0) };
	TestTrue(TEXT("meta' di un segmento"),
		URTPlaybackLibrary::InterpolateAlongPath(One, 0.5f).Equals(FVector(50, 0, 0), RTTol));

	// Due segmenti (3 waypoint): Alpha 0.5 cade esattamente sul waypoint centrale.
	const TArray<FVector> Two = { FVector(0, 0, 0), FVector(100, 0, 0), FVector(100, 100, 0) };
	TestTrue(TEXT("meta' di due segmenti = waypoint centrale"),
		URTPlaybackLibrary::InterpolateAlongPath(Two, 0.5f).Equals(FVector(100, 0, 0), RTTol));
	// Alpha 0.25 = meta' del primo segmento.
	TestTrue(TEXT("un quarto = meta' primo segmento"),
		URTPlaybackLibrary::InterpolateAlongPath(Two, 0.25f).Equals(FVector(50, 0, 0), RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackInterpDegenerateTest,
	"RefactorTactics.Playback.InterpolateDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackInterpDegenerateTest::RunTest(const FString&)
{
	// Vuoto -> ZeroVector; un solo waypoint -> quello; Alpha fuori range -> clamp.
	TestTrue(TEXT("vuoto -> zero"),
		URTPlaybackLibrary::InterpolateAlongPath(TArray<FVector>(), 0.5f).Equals(FVector::ZeroVector, RTTol));
	const TArray<FVector> Single = { FVector(7, 8, 9) };
	TestTrue(TEXT("singolo -> se stesso"),
		URTPlaybackLibrary::InterpolateAlongPath(Single, 0.5f).Equals(FVector(7, 8, 9), RTTol));
	const TArray<FVector> Seg = { FVector(0, 0, 0), FVector(100, 0, 0) };
	TestTrue(TEXT("Alpha<0 clampa a 0"),
		URTPlaybackLibrary::InterpolateAlongPath(Seg, -1.f).Equals(FVector(0, 0, 0), RTTol));
	TestTrue(TEXT("Alpha>1 clampa a 1"),
		URTPlaybackLibrary::InterpolateAlongPath(Seg, 2.f).Equals(FVector(100, 0, 0), RTTol));
	return true;
}

// --- EstimatePlaybackSeconds ----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEstimateTest,
	"RefactorTactics.Playback.EstimateSeconds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEstimateTest::RunTest(const FString&)
{
	// 4 segmenti a 8 celle/s = 0.5s di movimento (parallelo).
	TestTrue(TEXT("solo movimento"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EstimatePlaybackSeconds(4, 0, 0, 8.f, 0.f, 0.f), 0.5f, RTTol));
	// + 2 attacchi * 0.5s = 1.0 -> 1.5
	TestTrue(TEXT("movimento + attacchi"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EstimatePlaybackSeconds(4, 2, 0, 8.f, 0.f, 0.5f), 1.5f, RTTol));
	// + 3 fasi * 0.2s beat = 0.6 -> 2.1
	TestTrue(TEXT("movimento + attacchi + beat"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EstimatePlaybackSeconds(4, 2, 3, 8.f, 0.2f, 0.5f), 2.1f, RTTol));
	// CellsPerSecond <= 0 -> movimento istantaneo (nessun contributo movimento).
	TestTrue(TEXT("velocita' nulla -> nessun tempo di movimento"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EstimatePlaybackSeconds(4, 0, 2, 0.f, 0.2f, 0.5f), 0.4f, RTTol));
	return true;
}

// --- SpeedMultiplierForCap ------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSpeedCapTest,
	"RefactorTactics.Playback.SpeedMultiplierForCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSpeedCapTest::RunTest(const FString&)
{
	TestTrue(TEXT("entro il cap -> 1x"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SpeedMultiplierForCap(1.0f, 2.0f), 1.0f, RTTol));
	TestTrue(TEXT("oltre il cap -> proporzionale"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SpeedMultiplierForCap(4.0f, 2.0f), 2.0f, RTTol));
	TestTrue(TEXT("cap non positivo -> nessun limite (1x)"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SpeedMultiplierForCap(3.0f, 0.0f), 1.0f, RTTol));
	return true;
}

// --- EffectivePlaybackSpeed -----------------------------------------------------------------
//
// La tabella E' la decisione di CP 47.2 (#955): ogni riga che segue la prima nomina la composizione
// alternativa che falsifica. Cambiare Max in un prodotto, o in una sostituzione, deve far cadere una
// riga precisa e non le altre — e' il modo in cui questo test dice PERCHE' la formula e' quella.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEffectiveSpeedTest,
	"RefactorTactics.Playback.EffectiveSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEffectiveSpeedTest::RunTest(const FString&)
{
	// Nessuno dei due morde: la presentazione scorre a tempo reale.
	TestTrue(TEXT("x1 con cap inattivo -> 1x"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(1.0f, 1.0f), 1.0f, RTTol));

	// ⚠️ Falsifica "il tetto ridefinito" (x2 -> Max/2): su un round gia' sotto il tetto quella
	// composizione non farebbe nulla, e la manopola sarebbe inerte nel caso comune.
	TestTrue(TEXT("x4 con cap inattivo -> 4x (la manopola morde sui round brevi)"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(4.0f, 1.0f), 4.0f, RTTol));

	// ⚠️ Falsifica "la sostituzione" (solo Viewer): a x1 il tetto continua a valere, altrimenti un round
	// patologico durerebbe quanto vuole e MaxPlaybackSeconds sarebbe un campo morto.
	TestTrue(TEXT("x1 con cap che morde -> vince il cap (3x)"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(1.0f, 3.0f), 3.0f, RTTol));

	// ⚠️ Falsifica "il prodotto" (Viewer*Cap): darebbe 12x, illeggibile.
	TestTrue(TEXT("x4 su un round gia' accelerato 3x -> 4x, NON 12x"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(4.0f, 3.0f), 4.0f, RTTol));

	// Chiedere MENO di quanto il tetto impone non rallenta: il tetto e' un vincolo, non una preferenza.
	TestTrue(TEXT("x2 sotto un cap da 3x -> vince il cap (3x)"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(2.0f, 3.0f), 3.0f, RTTol));

	// Guardie: un campo azzerato non deve fermare il playback (vale 1, non 0).
	TestTrue(TEXT("velocita' scelta nulla -> trattata come x1"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(0.0f, 2.0f), 2.0f, RTTol));
	TestTrue(TEXT("velocita' scelta negativa -> trattata come x1"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(-3.0f, 1.0f), 1.0f, RTTol));
	TestTrue(TEXT("cap nullo -> trattato come 1x, la scelta passa"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(2.0f, 0.0f), 2.0f, RTTol));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
