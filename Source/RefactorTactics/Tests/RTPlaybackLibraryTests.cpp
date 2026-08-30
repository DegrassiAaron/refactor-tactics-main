#include "Misc/AutomationTest.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnRules.h"

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

// --- AttacksToShow --------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackStaggerTest,
	"RefactorTactics.Playback.AttacksToShowStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackStaggerTest::RunTest(const FString&)
{
	// Quattro colpi, uno ogni mezzo secondo.
	TestEqual(TEXT("a fase appena iniziata il primo colpo e' gia' uscito"),
		URTPlaybackLibrary::AttacksToShow(4, 0.f, 0.5f), 1);
	TestEqual(TEXT("appena prima del secondo beat siamo ancora a uno"),
		URTPlaybackLibrary::AttacksToShow(4, 0.49f, 0.5f), 1);
	TestEqual(TEXT("sul beat esce il secondo"),
		URTPlaybackLibrary::AttacksToShow(4, 0.5f, 0.5f), 2);
	TestEqual(TEXT("dopo tre beat sono tre"),
		URTPlaybackLibrary::AttacksToShow(4, 1.0f, 0.5f), 3);
	TestEqual(TEXT("oltre la fine non se ne inventano altri"),
		URTPlaybackLibrary::AttacksToShow(4, 10.f, 0.5f), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackShowSecondsHasEffectTest,
	"RefactorTactics.Playback.AttackShowSecondsHasEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackShowSecondsHasEffectTest::RunTest(const FString&)
{
	// 🔴 E' il criterio che #911 chiedeva: con due valori diversi di AttackShowSeconds il numero di
	// colpi visibili NELLO STESSO ISTANTE deve differire. Finche' il ramo che scagliona era
	// irraggiungibile il campo non aveva alcun effetto, e questa asserzione sarebbe stata l'unica a
	// dirlo: gli altri test misurano la formula, questo misura che il parametro conti.
	const int32 Fast = URTPlaybackLibrary::AttacksToShow(4, 1.0f, 0.5f);
	const int32 Slow = URTPlaybackLibrary::AttacksToShow(4, 1.0f, 1.0f);
	TestEqual(TEXT("un secondo a mezzo secondo per colpo -> tre"), Fast, 3);
	TestEqual(TEXT("un secondo a un secondo per colpo -> due"), Slow, 2);
	TestTrue(TEXT("AttackShowSeconds cambia quanti colpi si vedono"), Fast > Slow);

	// ⚠️ N colpi occupano N-1 INTERVALLI, non N: il primo esce a fase appena iniziata. Con 4 colpi a
	// mezzo secondo l'ultimo compare a 1.5 s, mentre DurationForPlaybackPhase riserva al Blast
	// NumAttacks*AttackShowSeconds = 2.0 s. Il beat che avanza non e' uno spreco: e' il tempo in cui
	// l'ultimo colpo resta leggibile prima che la fase chiuda. Misurato, non dedotto: la prima
	// stesura di questo test asseriva che a 1.99 s ne mancasse ancora uno, ed era falso.
	TestEqual(TEXT("l'ultimo colpo esce dopo N-1 intervalli"),
		URTPlaybackLibrary::AttacksToShow(4, 3 * 0.5f, 0.5f), 4);
	TestEqual(TEXT("un istante prima ne manca uno"),
		URTPlaybackLibrary::AttacksToShow(4, 3 * 0.5f - 0.01f, 0.5f), 3);
	TestEqual(TEXT("la durata riservata al Blast lascia un beat di coda, e nessun colpo in sospeso"),
		URTPlaybackLibrary::AttacksToShow(4, 4 * 0.5f, 0.5f), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackStaggerDegenerateTest,
	"RefactorTactics.Playback.AttacksToShowDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackStaggerDegenerateTest::RunTest(const FString&)
{
	TestEqual(TEXT("nessun colpo -> niente da mostrare"),
		URTPlaybackLibrary::AttacksToShow(0, 1.f, 0.5f), 0);
	TestEqual(TEXT("scaglionamento disattivato (0) -> tutti insieme"),
		URTPlaybackLibrary::AttacksToShow(3, 0.f, 0.f), 3);
	TestEqual(TEXT("scaglionamento negativo -> tutti insieme, non un crash"),
		URTPlaybackLibrary::AttacksToShow(3, 0.f, -1.f), 3);
	TestEqual(TEXT("tempo negativo -> il primo colpo, non zero ne' un indice fuori range"),
		URTPlaybackLibrary::AttacksToShow(3, -5.f, 0.5f), 1);
	return true;
}

// --- PhaseDuration --------------------------------------------------------------------------
//
// La durata di UNA fase del playback. Stava in `ARTTurnManager::DurationForPlaybackPhase`, dove per
// esercitarla serviva un mondo e un Actor: qui e' una funzione pura, e i cinque casi sotto girano
// headless. E' la formula che il gioco usa davvero (#1817).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseDurationMotionTest,
	"RefactorTactics.Playback.PhaseDurationScalesWithTheLongestPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseDurationMotionTest::RunTest(const FString&)
{
	// Le unita' si muovono IN PARALLELO: comanda il percorso piu' lungo, non la somma.
	// 4 segmenti a 8 celle/s = 0.5s.
	TestTrue(TEXT("Move: segmenti / velocita'"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Move, 4, 0, 8.f, 0.5f, 0.2f), 0.5f, RTTol));
	TestTrue(TEXT("Dash usa la stessa regola di Move"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Dash, 4, 0, 8.f, 0.5f, 0.2f), 0.5f, RTTol));
	// Il numero di attacchi non entra nelle fasi di movimento.
	TestTrue(TEXT("gli attacchi non allungano il Move"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Move, 4, 9, 8.f, 0.5f, 0.2f), 0.5f, RTTol));
	// Nessun movimento in quella fase -> nessun tempo.
	TestTrue(TEXT("zero segmenti -> zero secondi"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Move, 0, 0, 8.f, 0.5f, 0.2f), 0.f, RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseDurationBlastTest,
	"RefactorTactics.Playback.PhaseDurationBlastTakesTheLongerOfShotsAndKnockback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseDurationBlastTest::RunTest(const FString&)
{
	// Nel Blast i colpi e lo scivolamento del knockback avvengono nella STESSA finestra: la fase dura
	// quanto il piu' lungo dei due, non quanto la loro somma. E' la ragione per cui esiste un `Max`.
	// 3 colpi a 0.5s = 1.5s ; 2 segmenti a 8 celle/s = 0.25s -> vincono i colpi.
	TestTrue(TEXT("comandano i colpi"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Blast, 2, 3, 8.f, 0.5f, 0.2f), 1.5f, RTTol));
	// 1 colpo a 0.5s = 0.5s ; 8 segmenti a 8 celle/s = 1.0s -> vince la spinta.
	TestTrue(TEXT("comanda il knockback"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Blast, 8, 1, 8.f, 0.5f, 0.2f), 1.f, RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseDurationBlastFloorTest,
	"RefactorTactics.Playback.PhaseDurationBlastKeepsOneShotOfTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseDurationBlastFloorTest::RunTest(const FString&)
{
	// Un Blast SENZA colpi esiste: e' la spinta pura (knockback senza danno). Il tempo riservato resta
	// quello di un colpo, e non zero — altrimenti una fase che si vede non avrebbe durata.
	TestTrue(TEXT("zero colpi vale comunque un colpo di tempo"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Blast, 0, 0, 8.f, 0.5f, 0.2f), 0.5f, RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseDurationBeatTest,
	"RefactorTactics.Playback.PhaseDurationIsOneBeatWithoutMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseDurationBeatTest::RunTest(const FString&)
{
	// Prep non muove e non colpisce: dura un beat, e i segmenti passati non lo cambiano.
	TestTrue(TEXT("Prep = un beat"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Prep, 5, 5, 8.f, 0.5f, 0.2f), 0.2f, RTTol));
	TestTrue(TEXT("Cleanup = un beat"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Cleanup, 0, 0, 8.f, 0.5f, 0.2f), 0.2f, RTTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseDurationDegenerateTest,
	"RefactorTactics.Playback.PhaseDurationDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseDurationDegenerateTest::RunTest(const FString&)
{
	// Velocita' nulla o negativa = movimento istantaneo, non una divisione per zero.
	TestTrue(TEXT("velocita' nulla -> movimento istantaneo"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Move, 4, 0, 0.f, 0.5f, 0.2f), 0.f, RTTol));
	TestTrue(TEXT("velocita' negativa -> movimento istantaneo"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Move, 4, 0, -8.f, 0.5f, 0.2f), 0.f, RTTol));
	// Nel Blast la velocita' nulla azzera SOLO la spinta: i colpi restano.
	TestTrue(TEXT("Blast a velocita' nulla conserva i colpi"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::PhaseDuration(
			ERTMatchPhase::Blast, 4, 2, 0.f, 0.5f, 0.2f), 1.f, RTTol));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
