// Le linee di tiro che un osservatore puo' vedere — `#2742`.
//
// ## Cosa possiede questa famiglia di test
//
// La **linea**: la traiettoria verso un bersaglio e dove si interrompe. ⛔ Non l'**area** — *«quali celle
// vedo»* — che e' di `#1944`, con un'altra resa e un altro costo. Il confine e' stato deciso il 2026-09-10
// ed e' scritto in entrambe le issue.
//
// ## Perche' il produttore esiste, invece di chiamare la geometria dal renderer
//
// Perche' la conoscenza deve stare in UNA firma. Un consumatore che chiamasse `DescribeLineOfSight` e poi
// scartasse i bersagli ignoti sarebbe un secondo contratto di conoscenza, e ogni consumatore futuro
// dovrebbe ricordarsene. E' il difetto che `ARTHUD::ComputeBlockerMarks` evita non rifiltrando.

#include "Misc/AutomationTest.h"
#include "Map/RTSightLines.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	URTHexMapAsset* SlArena()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	}

	/** Un muro a meta' strada, sulla linea fra (0,0,0) e (3,0,0). */
	void SlPutWall(URTHexMapAsset* Map, const FRTCellId& Where)
	{
		FRTHexCellData Wall(Where);
		Wall.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Wall);
		Map->SortCells();
	}
}

/**
 * 🔴 **UN BERSAGLIO IGNOTO NON PRODUCE UNA LINEA, E NON PRODUCE NEMMENO UNA LINEA VUOTA.**
 *
 * ⚠️ **La differenza fra le due forme e' l'intera feature.** Una voce «linea assente» sarebbe
 * distinguibile da «nessuna voce» — e chi conta le voci saprebbe che li' c'e' qualcuno, che e'
 * precisamente cio' che [D-225] chiude. Il bersaglio ignoto **sparisce** dal risultato.
 *
 * ⚠️ **Anti-vacuita'**: si asserisce anche che il bersaglio NOTO una linea la produca. Senza, un
 * produttore che restituisse sempre l'insieme vuoto passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSightUnknownProducesNoLineTest,
	"RefactorTactics.Sight.UnknownTargetProducesNoLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSightUnknownProducesNoLineTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SlArena();
	if (!TestNotNull(TEXT("arena di prova"), Map)) { return false; }

	const FRTCellId From(0, 0, 0);
	const FRTCellId Target(3, 0, 0);

	// 1. CONTROLLO POSITIVO: noto -> una linea c'e'.
	const TArray<FRTSightLine> Noto = URTSightLineLibrary::AuthorizedSightLines(
		Map, From, { FRTObservedTarget(Target, /*bKnown=*/ true) });
	TestEqual(TEXT("un bersaglio noto produce una linea"), Noto.Num(), 1);

	// 2. 🔴 Il cuore: ignoto -> NESSUNA voce, non una voce vuota.
	const TArray<FRTSightLine> Ignoto = URTSightLineLibrary::AuthorizedSightLines(
		Map, From, { FRTObservedTarget(Target, /*bKnown=*/ false) });
	TestEqual(TEXT("un bersaglio ignoto non produce alcuna voce"), Ignoto.Num(), 0);

	// 3. Senza mappa autorevole non si afferma niente: fail-closed, come `ClassifyHexTargeting`.
	const TArray<FRTSightLine> SenzaMappa = URTSightLineLibrary::AuthorizedSightLines(
		nullptr, From, { FRTObservedTarget(Target, /*bKnown=*/ true) });
	TestEqual(TEXT("senza mappa: nessuna linea, non una linea libera"), SenzaMappa.Num(), 0);

	return true;
}

/**
 * ⛔ **IL CANARY DI PRIVACY, E SI ASSERISCE COME UGUAGLIANZA.**
 *
 * Due stati nascosti **diversi** — il nemico ignoto sta in due posti diversi, e in uno dei due dietro un
 * muro — con la **stessa** conoscenza autorizzata devono produrre lo **stesso** insieme di linee.
 *
 * 🔑 **Perche' uguaglianza e non «entrambi vuoti».** Due insiemi entrambi non vuoti ma **diversi**
 * riaprirebbero il canale, e un test che guardasse solo la vacuita' di ciascuno non lo vedrebbe. E' la
 * stessa forma di `RefactorTactics.Combat.RefusalHidesTheUnknownTarget`, e per la stessa ragione.
 *
 * ⚠️ **Il caso NOTO e' incluso di proposito in entrambi gli insiemi**: senza, l'uguaglianza sarebbe fra
 * due insiemi vuoti e passerebbe con un produttore che non produce mai niente.
 *
 * ⛔ **Verifica di mutazione**: togliere il filtro `bKnownToObserver` da `AuthorizedSightLines` deve far
 * cadere questo test — i due insiemi divergerebbero, perche' gli stati nascosti sono diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSightHiddenStatesAreIndistinguishableTest,
	"RefactorTactics.Sight.HiddenStatesProduceTheSameLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSightHiddenStatesAreIndistinguishableTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId Visto(0, 3, 0);      // lo stesso in entrambi gli stati: e' l'anti-vacuita'
	const FRTCellId NascostoA(3, 0, 0);  // stato A: l'ignoto sta qui, in vista libera
	const FRTCellId NascostoB(2, 1, 0);  // stato B: altrove, e dietro un muro

	// ── Stato A
	URTHexMapAsset* MapA = SlArena();
	if (!TestNotNull(TEXT("arena A"), MapA)) { return false; }
	const TArray<FRTSightLine> A = URTSightLineLibrary::AuthorizedSightLines(MapA, From, {
		FRTObservedTarget(Visto,     /*bKnown=*/ true),
		FRTObservedTarget(NascostoA, /*bKnown=*/ false) });

	// ── Stato B: l'ignoto e' in un'altra cella E dietro un ostacolo. Se il filtro non fosse nella firma,
	//    il muro cambierebbe il verdetto della sua linea e i due insiemi divergerebbero.
	URTHexMapAsset* MapB = SlArena();
	if (!TestNotNull(TEXT("arena B"), MapB)) { return false; }
	SlPutWall(MapB, FRTCellId(1, 1, 0));
	const TArray<FRTSightLine> B = URTSightLineLibrary::AuthorizedSightLines(MapB, From, {
		FRTObservedTarget(Visto,     /*bKnown=*/ true),
		FRTObservedTarget(NascostoB, /*bKnown=*/ false) });

	// ── ANTI-VACUITA': l'insieme non e' vuoto, altrimenti l'uguaglianza non direbbe niente.
	if (!TestEqual(TEXT("premessa: il bersaglio NOTO produce la sua linea in entrambi gli stati"),
			A.Num(), 1))
	{
		return false;
	}

	// ── 🔴 Il cuore: stessa conoscenza autorizzata, stesso insieme di linee.
	TestEqual(TEXT("due stati nascosti diversi producono lo STESSO numero di linee"), A.Num(), B.Num());
	TestTrue(TEXT("e la linea e' la stessa: stessa origine"), A[0].From == B[0].From);
	TestTrue(TEXT("e lo stesso bersaglio"), A[0].To == B[0].To);
	TestEqual(TEXT("e lo stesso verdetto di traiettoria"),
		static_cast<int32>(A[0].Sight.Block), static_cast<int32>(B[0].Sight.Block));

	return true;
}

/**
 * 🔴 **LA LINEA INTERROTTA PORTA IL PUNTO IN CUI SI FERMA, E NON LO RICALCOLA.**
 *
 * Il D010 chiede che la LOS **non venga ricalcolata**: `DescribeLineOfSight` e' il produttore unico. Qui
 * si asserisce proprio quello — il `BlockedAt` che il produttore delle linee restituisce e' **lo stesso**
 * che la geometria produce da sola.
 *
 * ⚠️ Un test che avesse solo verificato *«e' bloccata»* non avrebbe visto la differenza fra consumare e
 * riscrivere: due implementazioni possono concordare sul si/no e divergere su **dove**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSightBlockedCarriesItsStopTest,
	"RefactorTactics.Sight.BlockedLineCarriesWhereItStops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSightBlockedCarriesItsStopTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SlArena();
	if (!TestNotNull(TEXT("arena di prova"), Map)) { return false; }

	const FRTCellId From(0, 0, 0);
	const FRTCellId Target(3, 0, 0);
	const FRTCellId Muro(2, 0, 0);

	// Premessa: senza muro la linea passa. Senza questa riga il test non distinguerebbe «bloccata dal
	// muro» da «bloccata sempre».
	const TArray<FRTSightLine> Libera = URTSightLineLibrary::AuthorizedSightLines(
		Map, From, { FRTObservedTarget(Target, true) });
	if (!TestEqual(TEXT("premessa: una linea c'e'"), Libera.Num(), 1)) { return false; }
	TestTrue(TEXT("premessa: e senza ostacoli passa"), Libera[0].IsClear());

	SlPutWall(Map, Muro);

	const TArray<FRTSightLine> Bloccata = URTSightLineLibrary::AuthorizedSightLines(
		Map, From, { FRTObservedTarget(Target, true) });
	if (!TestEqual(TEXT("col muro la linea c'e' ancora"), Bloccata.Num(), 1)) { return false; }

	TestFalse(TEXT("ma non passa piu'"), Bloccata[0].IsClear());

	// 🔴 Il cuore: il punto d'arresto e' quello della geometria, non un secondo calcolo.
	const FRTLineOfSightResult Diretto = URTHexVisionLibrary::DescribeLineOfSight(Map, From, Target);
	TestTrue(TEXT("il punto d'arresto e' quello di DescribeLineOfSight, non uno ricalcolato"),
		Bloccata[0].Sight.BlockedAt == Diretto.BlockedAt);
	TestTrue(TEXT("e gli estremi viaggiano col verdetto"),
		Bloccata[0].From == From && Bloccata[0].To == Target);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
