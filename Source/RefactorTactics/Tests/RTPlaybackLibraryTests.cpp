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

// --- SlackScaleForBudget --------------------------------------------------------------------
//
// ⚠️ Ha sostituito `RefactorTactics.Playback.SpeedMultiplierForCap` il 2026-09-02 (#1878). Quel test
// pinnava un fattore `>= 1` che `TickPlayback` moltiplicava dentro `Dt`: il tetto accelerava i cilindri,
// ed era cio' che il product owner ha escluso. Non e' stato aggiunto accanto — sarebbe rimasta una
// verita' verde che afferma il contrario di quella viva.
//
// La riga che porta il peso e' l'ultima: e' quella che distingue un budget SOFT da uno HARD.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSlackScaleTest,
	"RefactorTactics.Playback.SlackScaleForBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSlackScaleTest::RunTest(const FString&)
{
	// Locomozione 2 s + attese 1 s = 3 s, budget 5 s: ci si sta, niente da comprimere.
	TestTrue(TEXT("gia' dentro il budget -> nessuna compressione"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(2.0f, 1.0f, 5.0f), 1.0f, RTTol));

	// 2 s + 4 s = 6 s contro 4 s: restano 2 s per 4 s di attese -> ne sopravvive meta'.
	TestTrue(TEXT("oltre il budget -> lo slack si comprime della frazione che serve"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(2.0f, 4.0f, 4.0f), 0.5f, RTTol));

	// Il taglio esatto: 2 s + 2 s = 4 s con budget 4 s e' il confine, e il confine sta DENTRO.
	TestTrue(TEXT("esattamente sul budget -> nessuna compressione"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(2.0f, 2.0f, 4.0f), 1.0f, RTTol));

	TestTrue(TEXT("budget non positivo -> nessun limite"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(9.0f, 3.0f, 0.0f), 1.0f, RTTol));

	// Nessuno slack: la risposta e' 1 e non 0. Sul tempo non cambia nulla (zero per qualunque scala e'
	// zero), ma il valore e' anche telemetria, e uno 0 direbbe che il budget ha agito senza avere su cosa.
	TestTrue(TEXT("niente da comprimere -> 1, non 0"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(9.0f, 0.0f, 4.0f), 1.0f, RTTol));

	// 🔑 **La riga che definisce «soft».** La sola locomozione (10 s) eccede gia' il budget (4 s): si toglie
	// tutto il comprimibile e non basta. La risposta e' 0 — «ho tolto tutto» — e NON un numero negativo,
	// che significherebbe togliere tempo al movimento, cioe' accelerarlo. La durata sfora, e sforare e'
	// la risposta giusta: e' la decisione del PO applicata al caso peggiore.
	TestTrue(TEXT("la sola locomozione eccede il budget -> slack a 0, e la durata sfora"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::SlackScaleForBudget(10.0f, 2.0f, 4.0f), 0.0f, RTTol));

	return true;
}

// --- EffectivePlaybackSpeed -----------------------------------------------------------------
//
// ⚠️ **Aveva otto righe e ne ha tre**, dal 2026-09-02 (#1878). Le cinque cadute pinnavano la composizione
// `Max(Viewer, Cap)` di CP 47.2 (#955) — e sono cadute perche' il secondo argomento non ha piu' un
// produttore, non perche' quella decisione sia stata rovesciata. Il tetto e' vivo: agisce su
// `SlackScaleForBudget`. Le alternative che #955 aveva scartato restano registrate nel docstring della
// funzione, dove tornerebbero utili se un secondo fattore di velocita' rinascesse.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEffectiveSpeedTest,
	"RefactorTactics.Playback.EffectiveSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEffectiveSpeedTest::RunTest(const FString&)
{
	TestTrue(TEXT("la scelta passa cosi' com'e'"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(4.0f), 4.0f, RTTol));

	// Guardie: un campo azzerato non deve fermare il playback (vale 1, non 0).
	TestTrue(TEXT("velocita' scelta nulla -> trattata come x1"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(0.0f), 1.0f, RTTol));
	TestTrue(TEXT("velocita' scelta negativa -> trattata come x1"),
		FMath::IsNearlyEqual(URTPlaybackLibrary::EffectivePlaybackSpeed(-3.0f), 1.0f, RTTol));

	return true;
}

// --- PhaseTime: i due termini ---------------------------------------------------------------
//
// 🔑 **Cio' che questo test protegge e' quali fasi il budget puo' toccare**, e la risposta e' «solo quelle
// che non mostrano nulla». Ogni riga sul `Blast` esiste per un difetto misurato in review: la prima
// stesura classificava comprimibile l'eccedenza dei colpi sulla spinta, e con lo slack a zero la fase
// durava zero — i colpi uscivano tutti in un frame e la spinta scivolava al rate base invece che sulla
// finestra dei colpi. Se qualcuno rimettesse dello slack nel `Blast`, cadono le due righe che lo negano.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseTimeSplitTest,
	"RefactorTactics.Playback.PhaseTimeSplitsShownFromSlack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseTimeSplitTest::RunTest(const FString&)
{
	// Move: 4 celle a 2 celle/s = 2 s, tutto mostrato. Non c'e' attesa da togliere, e toglierla sarebbe
	// accelerare i cilindri — che e' esattamente cio' che #1878 vieta.
	{
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Move, /*MaxSeg*/ 4, /*Attacks*/ 0, /*CellsPerSec*/ 2.f, 0.5f, 0.3f);
		TestTrue(TEXT("Move: 2 s mostrati"), FMath::IsNearlyEqual(T.Shown, 2.0f, RTTol));
		TestTrue(TEXT("Move: nessuno slack"), FMath::IsNearlyEqual(T.Slack, 0.0f, RTTol));
	}

	// Prep: un beat, e non mostra nulla. E' l'unica attesa comprimibile del sistema.
	{
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Prep, 0, 0, 2.f, 0.5f, /*Beat*/ 0.3f);
		TestTrue(TEXT("Prep: non mostra nulla"), FMath::IsNearlyEqual(T.Shown, 0.0f, RTTol));
		TestTrue(TEXT("Prep: il beat e' tutto slack"), FMath::IsNearlyEqual(T.Slack, 0.3f, RTTol));
	}

	// 🔴 Blast dominato dai COLPI: 4 colpi x 0,5 s = 2 s contro 0,5 s di spinta. Tutto mostrato, zero
	// slack. La riga che cade se qualcuno rende comprimibile il tempo di lettura: con slack a 1,5 s e la
	// scala a zero, questa fase durerebbe 0,5 s e tre colpi su quattro uscirebbero nello stesso frame.
	{
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Blast, /*MaxSeg*/ 1, /*Attacks*/ 4, /*CellsPerSec*/ 2.f, 0.5f, 0.3f);
		TestTrue(TEXT("Blast: il tempo dei colpi e' mostrato, non atteso"),
			FMath::IsNearlyEqual(T.Shown, 2.0f, RTTol));
		TestTrue(TEXT("Blast: nessuno slack, nemmeno l'eccedenza dei colpi sulla spinta"),
			FMath::IsNearlyEqual(T.Slack, 0.0f, RTTol));
		TestTrue(TEXT("Blast: il totale resta Max(colpi, spinta)"),
			FMath::IsNearlyEqual(T.Total(), 2.0f, RTTol));
	}

	// Blast dominato dalla SPINTA: 6 celle a 2 celle/s = 3 s contro 1 colpo da 0,5 s.
	{
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Blast, /*MaxSeg*/ 6, /*Attacks*/ 1, /*CellsPerSec*/ 2.f, 0.5f, 0.3f);
		TestTrue(TEXT("Blast: spinta dominante -> 3 s mostrati"),
			FMath::IsNearlyEqual(T.Shown, 3.0f, RTTol));
		TestTrue(TEXT("Blast: spinta dominante -> nessuno slack"),
			FMath::IsNearlyEqual(T.Slack, 0.0f, RTTol));
	}

	// ⚠️ **L'invariante di compatibilita' vuole NUMERI, non un confronto.** La prima stesura confrontava
	// `T.Total()` con `PhaseDuration(...)`: da quando `PhaseDuration` E' `PhaseTime(...).Total()`, quel
	// confronto e' una tautologia e resta verde anche se ogni durata si dimezza. I valori qui sotto sono
	// quelli che `PhaseDuration` restituiva PRIMA della separazione, calcolati a mano dalla formula
	// originale su `MaxSeg=3, Attacks=2, CellsPerSec=2, AttackShow=0,5, Beat=0,3`.
	{
		// Dash/Move: 3 celle / 2 celle-al-secondo.
		TestTrue(TEXT("Dash valeva 1,5 s e vale 1,5 s"), FMath::IsNearlyEqual(
			URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Dash, 3, 2, 2.f, 0.5f, 0.3f), 1.5f, RTTol));
		TestTrue(TEXT("Move valeva 1,5 s e vale 1,5 s"), FMath::IsNearlyEqual(
			URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Move, 3, 2, 2.f, 0.5f, 0.3f), 1.5f, RTTol));
		// Blast: Max(2 colpi x 0,5 = 1,0 ; spinta 1,5) = 1,5.
		TestTrue(TEXT("Blast valeva 1,5 s e vale 1,5 s"), FMath::IsNearlyEqual(
			URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Blast, 3, 2, 2.f, 0.5f, 0.3f), 1.5f, RTTol));
		// Prep/Cleanup: un beat.
		TestTrue(TEXT("Prep valeva 0,3 s e vale 0,3 s"), FMath::IsNearlyEqual(
			URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Prep, 3, 2, 2.f, 0.5f, 0.3f), 0.3f, RTTol));
		TestTrue(TEXT("Cleanup valeva 0,3 s e vale 0,3 s"), FMath::IsNearlyEqual(
			URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Cleanup, 3, 2, 2.f, 0.5f, 0.3f), 0.3f, RTTol));
	}

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
