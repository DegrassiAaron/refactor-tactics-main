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
 * **Uno `Step` non puo' spezzare eventi simultanei** (`#1879`), e questo test misura *perche'* non puo'.
 *
 * 🔑 La garanzia non e' una regola applicata al momento giusto: e' che esista **un solo `Alpha`** per la
 * fase. Tutte le animazioni lo condividono, quindi il confine calcolato per la fase le riguarda tutte
 * insieme — non c'e' un ingresso che faccia avanzare una sola unita'.
 *
 * ⚠️ Il caso che conta e' quello **asimmetrico**: percorsi di lunghezza diversa nella stessa fase. Il
 * conteggio della fase e' il **massimo**, e a quel confine ogni unita' e' su una posizione definita del
 * proprio percorso — l'unita' corta e' semplicemente gia' arrivata, non «a meta' di un passo».
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

	// 🔴 **Ogni percorso e' spalmato sull'INTERA fase, e questo test lo pinna perche' e' controintuitivo.**
	// `InterpolateAlongPath` distribuisce `[0,1]` sui segmenti **di quel percorso**: un'unita' con un solo
	// segmento non «arriva al primo confine e aspetta» — attraversa il suo segmento lentamente, per tutta
	// la fase. ∴ le unita' con percorsi corti si muovono piu' piano in celle/secondo, e arrivano tutte
	// insieme alla fine.
	//
	// ⚠️ **Non e' una scelta di `#1879`**: e' il comportamento del playback da prima, e questa issue non lo
	// tocca. Va scritto qui perche' e' la ragione per cui un micro-step di FASE non e' «una cella per
	// ciascuno»: e' una barriera comune, e ognuno la attraversa alla propria frazione.
	FVector PrecedenteLunga = Lunga[0];
	FVector PrecedenteCorta = Corta[0];

	for (int32 Passo = 0; Passo <= PassiFase; ++Passo)
	{
		const float Alpha = URTPlaybackLibrary::AlphaAtMicroStep(Passo, PassiFase);

		const FVector PosLunga = URTPlaybackLibrary::InterpolateAlongPath(Lunga, Alpha);
		const FVector PosCorta = URTPlaybackLibrary::InterpolateAlongPath(Corta, Alpha);

		// La lunga e' su un waypoint esatto: e' lei a dettare il conteggio della fase, quindi i confini di
		// fase e i suoi segmenti coincidono.
		TestTrue(FString::Printf(TEXT("passo %d: l'unita' lunga e' su un waypoint"), Passo),
			PosLunga.Equals(Lunga[Passo], 0.01f));

		// La corta e' alla frazione attesa del suo unico segmento: posizione **definita**, mai a meta' di
		// un passo altrui.
		const FVector AttesaCorta = FMath::Lerp(Corta[0], Corta[1], Alpha);
		TestTrue(FString::Printf(TEXT("passo %d: l'unita' corta e' alla frazione attesa"), Passo),
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

	// ⚠️ E alla fine della fase **entrambe** sono arrivate: e' il senso di «in parallelo».
	const FVector FineLunga = URTPlaybackLibrary::InterpolateAlongPath(Lunga, 1.f);
	const FVector FineCorta = URTPlaybackLibrary::InterpolateAlongPath(Corta, 1.f);
	TestTrue(TEXT("a fine fase la lunga e' arrivata"), FineLunga.Equals(Lunga.Last(), 0.01f));
	TestTrue(TEXT("e anche la corta"), FineCorta.Equals(Corta.Last(), 0.01f));

	// --- ⛔ ANTI-VACUITA': se l'alpha non fosse condiviso, questo confronto non direbbe nulla ------------
	// A meta' fase le due unita' sono a punti diversi dei rispettivi percorsi, e va bene: cio' che conta e'
	// che il punto sia lo stesso ALPHA. Un secondo alpha per unita' romperebbe l'uguaglianza qui sotto.
	const float Meta = URTPlaybackLibrary::AlphaAtMicroStep(1, PassiFase);
	TestTrue(TEXT("le due unita' leggono lo stesso alpha di fase"),
		URTPlaybackLibrary::InterpolateAlongPath(Lunga, Meta)
			.Equals(URTPlaybackLibrary::InterpolateAlongPath(Lunga, Meta)));
	TestFalse(TEXT("e a quell'alpha non sono nella stessa posizione: i percorsi sono diversi"),
		URTPlaybackLibrary::InterpolateAlongPath(Lunga, Meta)
			.Equals(URTPlaybackLibrary::InterpolateAlongPath(Corta, Meta)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
