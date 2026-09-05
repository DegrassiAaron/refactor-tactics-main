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

/**
 * **Il micro-step del playback e' un segmento, e il conteggio lo dice** (`#1879`).
 *
 * 🔑 Non e' una granularita' nuova: `InterpolateAlongPath` divide gia' `[0,1]` in frazioni uguali, una per
 * segmento, e lo dichiara — *«1 passo logico = 1 segmento»*. Questo test pinna che il conteggio sia
 * l'inverso esatto di quella divisione, invece di una seconda convenzione che le somiglia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackMicroStepCountTest,
	"RefactorTactics.Playback.MicroStepCountMatchesSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackMicroStepCountTest::RunTest(const FString&)
{
	TestEqual(TEXT("percorso vuoto: nessun micro-step"),
		URTPlaybackLibrary::MicroStepsInPath({}), 0);

	// ⚠️ Un'unita' ferma ha UN waypoint — la sua cella — e zero barriere da attraversare.
	TestEqual(TEXT("un solo waypoint: nessun micro-step"),
		URTPlaybackLibrary::MicroStepsInPath({ FVector::ZeroVector }), 0);

	TestEqual(TEXT("due waypoint: un segmento"),
		URTPlaybackLibrary::MicroStepsInPath({ FVector(0,0,0), FVector(100,0,0) }), 1);

	TestEqual(TEXT("quattro waypoint: tre segmenti"),
		URTPlaybackLibrary::MicroStepsInPath(
			{ FVector(0,0,0), FVector(100,0,0), FVector(200,0,0), FVector(300,0,0) }), 3);

	// --- l'alpha dei confini divide `[0,1]` in parti uguali --------------------------------------------
	TestEqual(TEXT("il confine 0 e' l'inizio"), URTPlaybackLibrary::AlphaAtMicroStep(0, 4), 0.f);
	TestEqual(TEXT("il confine 2 di 4 e' meta'"), URTPlaybackLibrary::AlphaAtMicroStep(2, 4), 0.5f);
	TestEqual(TEXT("il confine 4 di 4 e' la fine"), URTPlaybackLibrary::AlphaAtMicroStep(4, 4), 1.f);

	// ⚠️ Nessun segmento = fase gia' conclusa. `1.f` e non `0.f`, o resterebbe in attesa per sempre.
	TestEqual(TEXT("senza segmenti l'alpha e' la fine, non l'inizio"),
		URTPlaybackLibrary::AlphaAtMicroStep(0, 0), 1.f);

	return true;
}

/**
 * **Uno `Step` esegue un micro-step INTERO e si ferma su un confine** (`#1879`).
 *
 * 🔴 **Le due asserzioni che portano il peso sono la ripetizione e la tolleranza.**
 *
 * *Ripetizione*: da un `Alpha` gia' esattamente su un confine si deve avanzare al **successivo**. Un `>=`
 * al posto del `>` lascerebbe `Step` fermo sul posto ogni volta che lo si preme due volte di fila — che e'
 * esattamente l'uso per cui il comando esiste.
 *
 * *Tolleranza*: `Alpha` arriva da un'accumulazione in virgola mobile, e `1/3` vale `0.333333343`. Senza la
 * tolleranza prima del floor, un confine raggiunto per accumulo cadrebbe nel segmento successivo e una
 * pressione sola ne salterebbe **due**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStepWholeMicroStepTest,
	"RefactorTactics.Playback.StepExecutesWholeMicroStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStepWholeMicroStepTest::RunTest(const FString&)
{
	const int32 Passi = 4; // confini a 0, 0.25, 0.5, 0.75, 1

	TestEqual(TEXT("da 0 si va al primo confine"),
		URTPlaybackLibrary::NextMicroStepBoundary(0.f, Passi), 0.25f);

	// --- 🔴 da un confine si va al SUCCESSIVO, non si resta fermi --------------------------------------
	TestEqual(TEXT("da un confine esatto si avanza al successivo"),
		URTPlaybackLibrary::NextMicroStepBoundary(0.25f, Passi), 0.5f);

	// --- ⛔ mai un punto intermedio: da meta' segmento si arriva al confine, non oltre -----------------
	TestEqual(TEXT("da meta' segmento si arriva al confine di quel segmento"),
		URTPlaybackLibrary::NextMicroStepBoundary(0.3f, Passi), 0.5f);

	// --- non si supera mai la fine ---------------------------------------------------------------------
	TestEqual(TEXT("dall'ultimo confine si resta alla fine"),
		URTPlaybackLibrary::NextMicroStepBoundary(1.f, Passi), 1.f);
	TestEqual(TEXT("e oltre la fine non si va"),
		URTPlaybackLibrary::NextMicroStepBoundary(0.99f, Passi), 1.f);

	// --- 🔴 la catena completa: N pressioni portano ESATTAMENTE alla fine, non prima e non oltre --------
	// E' l'asserzione che fallirebbe se la tolleranza mancasse: gli errori di accumulo si sommerebbero e
	// la catena arriverebbe a fine fase in meno di N passi.
	float Alpha = 0.f;
	for (int32 i = 0; i < Passi; ++i)
	{
		Alpha = URTPlaybackLibrary::NextMicroStepBoundary(Alpha, Passi);
	}
	TestEqual(TEXT("quattro pressioni su quattro segmenti arrivano a fine fase"), Alpha, 1.f);

	// ⚠️ E la penultima NON ci arriva: se ci arrivasse, la catena starebbe saltando un confine.
	float Parziale = 0.f;
	for (int32 i = 0; i < Passi - 1; ++i)
	{
		Parziale = URTPlaybackLibrary::NextMicroStepBoundary(Parziale, Passi);
	}
	TestTrue(TEXT("⛔ e tre pressioni su quattro segmenti NON arrivano alla fine"), Parziale < 1.f);

	// --- una fase senza segmenti e' gia' conclusa ------------------------------------------------------
	TestEqual(TEXT("senza segmenti si e' gia' alla fine"),
		URTPlaybackLibrary::NextMicroStepBoundary(0.f, 0), 1.f);

	return true;
}

/**
 * **La velocita' visuale per cella non dipende dal percorso di nessun altro** (`#2370`).
 *
 * 🔑 E' `PhaseDuration` letta per-unita': la fase dura `MaxSeg / rate`, e a quello stesso rate ciascuno
 * percorre le **proprie** celle. Il percorso piu' lungo arriva a `1` esattamente a fine fase — le due
 * formule sono la stessa divisione, e non possono divergere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackRouteAlphaTest,
	"RefactorTactics.Playback.RouteAlphaIsPerRouteNotPerPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackRouteAlphaTest::RunTest(const FString&)
{
	const float Rate = 2.f;

	// --- il rate e' quello dichiarato: un secondo, due celle -------------------------------------------
	TestEqual(TEXT("a t=0 si e' alla partenza"), URTPlaybackLibrary::RouteAlpha(10, 0.f, Rate), 0.f);
	TestEqual(TEXT("dopo 1 s un percorso di 10 celle ne ha fatte 2"),
		URTPlaybackLibrary::RouteAlpha(10, 1.f, Rate) * 10.f, 2.f);
	TestEqual(TEXT("e uno di 2 celle e' arrivato"), URTPlaybackLibrary::RouteAlpha(2, 1.f, Rate), 1.f);

	// --- 🔴 il caso di `#2370`: 2 celle e 10 celle nella stessa fase -----------------------------------
	// La fase dura `10 / rate = 5 s`. A 1 s la corta e' arrivata e la lunga e' a un quinto.
	const float DurataFase = 10.f / Rate;
	TestEqual(TEXT("la fase dura quanto le 10 celle al rate base"),
		URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Move, 10, 0, Rate, 0.f, 0.f), DurataFase);

	TestEqual(TEXT("⛔ a 1 s la corta e' arrivata"), URTPlaybackLibrary::RouteAlpha(2, 1.f, Rate), 1.f);
	TestTrue(TEXT("⛔ e la lunga NO"), URTPlaybackLibrary::RouteAlpha(10, 1.f, Rate) < 1.f);

	// ⚠️ Il difetto misurato, scritto come numero: con l'`Alpha` di fase la corta avrebbe fatto
	// `1/5` del suo percorso — cioe' 0,4 celle in un secondo invece di 2. Cinque volte piu' lento.
	const float AlphaDiFaseA1s = 1.f / DurataFase;
	TestTrue(TEXT("⛔ l'alpha di fase avrebbe tenuto indietro la corta"),
		AlphaDiFaseA1s < URTPlaybackLibrary::RouteAlpha(2, 1.f, Rate) - RTTol);

	// --- il percorso piu' lungo chiude ESATTAMENTE con la fase -----------------------------------------
	TestEqual(TEXT("a fine fase la lunga e' arrivata, non oltre"),
		URTPlaybackLibrary::RouteAlpha(10, DurataFase, Rate), 1.f);
	TestTrue(TEXT("e un istante prima non lo era"),
		URTPlaybackLibrary::RouteAlpha(10, DurataFase - 0.01f, Rate) < 1.f);

	// --- degeneri: nessuna divisione per zero, e nessuna attesa infinita -------------------------------
	// ⚠️ `1.f` e non `0.f`, come `AlphaAtMicroStep(0, 0)`: senza segmenti la fase e' gia' conclusa.
	TestEqual(TEXT("senza segmenti si e' gia' alla fine"), URTPlaybackLibrary::RouteAlpha(0, 0.f, Rate), 1.f);
	TestEqual(TEXT("segmenti negativi: idem"), URTPlaybackLibrary::RouteAlpha(-3, 0.f, Rate), 1.f);
	TestEqual(TEXT("rate nullo = movimento istantaneo, non una divisione per zero"),
		URTPlaybackLibrary::RouteAlpha(10, 0.f, 0.f), 1.f);
	TestEqual(TEXT("rate negativo: idem"), URTPlaybackLibrary::RouteAlpha(10, 0.f, -1.f), 1.f);

	// --- mai fuori da [0,1] ----------------------------------------------------------------------------
	TestEqual(TEXT("oltre la fine si resta alla fine"), URTPlaybackLibrary::RouteAlpha(2, 999.f, Rate), 1.f);
	TestEqual(TEXT("un elapsed negativo resta alla partenza"),
		URTPlaybackLibrary::RouteAlpha(2, -5.f, Rate), 0.f);

	// --- 🔑 invarianza rispetto alla velocita' del viewer ----------------------------------------------
	// `ViewerPlaybackSpeed` moltiplica `Dt`, quindi entra in `PhaseElapsed` per TUTTI allo stesso modo:
	// l'ordine di arrivo non cambia, cambia solo l'orologio.
	const float Doppio = 2.f;
	TestEqual(TEXT("a velocita' doppia si e' allo stesso punto in meta' tempo (corta)"),
		URTPlaybackLibrary::RouteAlpha(2, 1.f * Doppio, Rate), URTPlaybackLibrary::RouteAlpha(2, 1.f, Rate * Doppio));
	TestEqual(TEXT("e lo stesso vale per la lunga"),
		URTPlaybackLibrary::RouteAlpha(10, 1.f * Doppio, Rate), URTPlaybackLibrary::RouteAlpha(10, 1.f, Rate * Doppio));

	return true;
}

/**
 * **Uno `Step` non puo' spezzare eventi simultanei** (`#1879`), e questo test misura *perche'* non puo'.
 *
 * 🔑 La garanzia non e' una regola applicata al momento giusto: e' che esista **un solo orologio di fase**.
 * `PlaybackPhaseElapsed` avanza una volta per tick e il confine di `StepMicroStep` e' un istante in
 * SECONDI (`PlaybackStepTargetElapsed`), non una frazione di percorso: quell'istante riguarda tutte le
 * animazioni insieme, e non esiste un ingresso che faccia avanzare una sola unita'.
 *
 * ⚠️ **La garanzia NON era «un solo `Alpha` condiviso», ed e' importante saperlo**: fino al 2026-09-05 lo
 * era anche, e questo test lo pinnava. `#2370` ha dato a ogni percorso il proprio `Alpha` — le sue celle al
 * rate base — e la simultaneita' e' rimasta intatta, perche' non e' mai dipesa da quello. Cio' che l'ha
 * sempre garantita e' l'orologio unico.
 *
 * ⚠️ Il caso che conta e' quello **asimmetrico**: percorsi di lunghezza diversa nella stessa fase. Il
 * conteggio della fase e' il **massimo**, e a quel confine ogni unita' e' su una posizione definita del
 * proprio percorso — l'unita' corta e' gia' arrivata, non «a meta' di un passo».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStepKeepsSimultaneityTest,
	"RefactorTactics.Playback.StepDoesNotSplitSimultaneousEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStepKeepsSimultaneityTest::RunTest(const FString&)
{
	// Due unita' nella stessa fase: una percorre tre segmenti, l'altra uno solo.
	const TArray<FVector> Lunga = { FVector(0,0,0), FVector(100,0,0), FVector(200,0,0), FVector(300,0,0) };
	const TArray<FVector> Corta = { FVector(0,500,0), FVector(100,500,0) };

	const int32 PassiFase = FMath::Max(
		URTPlaybackLibrary::MicroStepsInPath(Lunga),
		URTPlaybackLibrary::MicroStepsInPath(Corta));
	TestEqual(TEXT("la fase dura quanto il percorso PIU' LUNGO"), PassiFase, 3);

	// L'orologio della fase, nei termini in cui il playback lo tiene: un rate base, e una durata che vale
	// il percorso piu' lungo diviso quel rate. Sono gli stessi ingressi di `PhaseDuration`.
	const float CellePerSecondo = 2.f;
	const float Durata = static_cast<float>(PassiFase) / CellePerSecondo; // 1,5 s
	TestEqual(TEXT("la durata di fase e' il percorso piu' lungo al rate base"),
		URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Move, PassiFase, 0, CellePerSecondo, 0.f, 0.f), Durata);

	// 🔴 **Ogni percorso ha il PROPRIO `Alpha`, e questo test lo pinna perche' e' il cuore di `#2370`.**
	// `InterpolateAlongPath` distribuisce `[0,1]` sui segmenti **di quel percorso**: se gli si desse
	// l'`Alpha` di fase, un'unita' con un solo segmento lo attraverserebbe lentamente per tutta la fase e
	// arriverebbe insieme a quella da tre. Con `RouteAlpha` attraversa la sua cella al rate base, **arriva
	// al primo confine e aspetta**.
	//
	// ⚠️ **Ed e' questo che rende un micro-step di fase «una cella per ciascuno»**: il confine `k` cade a
	// `k / CellePerSecondo` secondi, quindi ogni unita' e' esattamente sulla propria cella `k` — o e' gia'
	// arrivata. Con l'`Alpha` condiviso era a `k/PassiFase` del proprio percorso, cioe' in mezzo a un
	// segmento, e l'ingresso-cella visibile non coincideva con nessun confine canonico.
	FVector PrecedenteLunga = Lunga[0];
	FVector PrecedenteCorta = Corta[0];

	for (int32 Passo = 0; Passo <= PassiFase; ++Passo)
	{
		// Il confine, in secondi: e' UNO, ed e' lo stesso per entrambe. La simultaneita' sta qui.
		const float Elapsed = URTPlaybackLibrary::AlphaAtMicroStep(Passo, PassiFase) * Durata;
		TestEqual(FString::Printf(TEXT("passo %d: il confine cade a k/rate secondi"), Passo),
			Elapsed, static_cast<float>(Passo) / CellePerSecondo);

		const FVector PosLunga = URTPlaybackLibrary::InterpolateAlongPath(
			Lunga, URTPlaybackLibrary::RouteAlpha(PassiFase, Elapsed, CellePerSecondo));
		const FVector PosCorta = URTPlaybackLibrary::InterpolateAlongPath(
			Corta, URTPlaybackLibrary::RouteAlpha(1, Elapsed, CellePerSecondo));

		// La lunga e' su un waypoint esatto: e' lei a dettare il conteggio della fase, quindi i confini di
		// fase e i suoi segmenti coincidono.
		TestTrue(FString::Printf(TEXT("passo %d: l'unita' lunga e' su un waypoint"), Passo),
			PosLunga.Equals(Lunga[Passo], 0.01f));

		// 🔴 La corta e' sulla propria cella `Passo`, e il suo percorso ne ha una sola: al confine 0 e'
		// alla partenza, da 1 in poi e' **arrivata**. Mai a meta' di un passo altrui.
		const FVector AttesaCorta = Corta[FMath::Min(Passo, 1)];
		TestTrue(FString::Printf(TEXT("passo %d: l'unita' corta e' sulla propria cella"), Passo),
			PosCorta.Equals(AttesaCorta, 0.01f));

		// ⛔ **Monotonia**: nessuna delle due torna indietro fra un confine e il successivo. E' cio' che
		// rende `Step` un avanzamento e non un salto.
		if (Passo > 0)
		{
			TestTrue(FString::Printf(TEXT("passo %d: la lunga non torna indietro"), Passo),
				FVector::DistSquared(Lunga[0], PosLunga) >= FVector::DistSquared(Lunga[0], PrecedenteLunga) - 0.01f);
			TestTrue(FString::Printf(TEXT("passo %d: la corta non torna indietro"), Passo),
				FVector::DistSquared(Corta[0], PosCorta) >= FVector::DistSquared(Corta[0], PrecedenteCorta) - 0.01f);
		}
		PrecedenteLunga = PosLunga;
		PrecedenteCorta = PosCorta;
	}

	// ⚠️ E alla fine della fase **entrambe** sono arrivate: e' il senso di «in parallelo». La corta ci era
	// arrivata prima e ha aspettato; la fase finisce comunque quando finisce l'ultima.
	const FVector FineLunga = URTPlaybackLibrary::InterpolateAlongPath(
		Lunga, URTPlaybackLibrary::RouteAlpha(PassiFase, Durata, CellePerSecondo));
	const FVector FineCorta = URTPlaybackLibrary::InterpolateAlongPath(
		Corta, URTPlaybackLibrary::RouteAlpha(1, Durata, CellePerSecondo));
	TestTrue(TEXT("a fine fase la lunga e' arrivata"), FineLunga.Equals(Lunga.Last(), 0.01f));
	TestTrue(TEXT("e anche la corta"), FineCorta.Equals(Corta.Last(), 0.01f));

	// --- ⛔ ANTI-VACUITA': la corta arriva PRIMA, e senza questa riga il test non lo direbbe -------------
	// E' l'asserzione che l'`Alpha` condiviso faceva fallire: con `Elapsed / Durata` la corta sarebbe stata
	// a un terzo del suo unico segmento — non alla fine — e le due sarebbero arrivate insieme.
	const float MetaSecondi = Durata / static_cast<float>(PassiFase); // il primo confine: 0,5 s
	const FVector CortaAlPrimoConfine = URTPlaybackLibrary::InterpolateAlongPath(
		Corta, URTPlaybackLibrary::RouteAlpha(1, MetaSecondi, CellePerSecondo));
	const FVector LungaAlPrimoConfine = URTPlaybackLibrary::InterpolateAlongPath(
		Lunga, URTPlaybackLibrary::RouteAlpha(PassiFase, MetaSecondi, CellePerSecondo));
	TestTrue(TEXT("⛔ al primo confine la corta e' GIA' arrivata"),
		CortaAlPrimoConfine.Equals(Corta.Last(), 0.01f));
	TestFalse(TEXT("⛔ mentre la lunga e' ancora per strada"),
		LungaAlPrimoConfine.Equals(Lunga.Last(), 0.01f));

	// ⛔ E la velocita' per cella e' la STESSA per entrambe: e' il difetto di `#2370` letto in positivo.
	// Un terzo della fase percorre una cella, chiunque la percorra.
	TestEqual(TEXT("la lunga ha percorso una cella nel primo confine"),
		URTPlaybackLibrary::RouteAlpha(PassiFase, MetaSecondi, CellePerSecondo) * PassiFase, 1.f);
	TestEqual(TEXT("e la corta ne ha percorsa una anch'essa (il suo percorso finisce li')"),
		URTPlaybackLibrary::RouteAlpha(1, MetaSecondi, CellePerSecondo) * 1.f, 1.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
