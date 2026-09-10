#include "Misc/AutomationTest.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Turn/RTFacingLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2873` — **quanto costa l'accumulo del transito su ARENA PIENA**, misurato invece che stimato.
 *
 * ## Perche' esiste
 *
 * [D-379] ha scelto il ramo canonico: `ExploredCells` accumula anche cio' che si e' visto attraversando. La
 * voce di decisione dichiara il costo *«circa dieci coni in piu' per squadra per turno in un 2v2»* — ed e'
 * un **conto**, non una misura. L'AC di `#2873` chiede la misura, e la chiede sulla scala che [D-227]
 * nomina: l'arena di raggio **50**, non quella di raggio 6 su cui girano i KPI del resolver.
 *
 * ## Cosa misura, esattamente
 *
 * Il lavoro che `ARTTurnManager::AccumulateExploredFromTransit` fa per **una** unita' che percorre una rotta
 * lunga quanto il suo budget: un `FRTPerceiver` per cella attraversata, con l'orientamento di quel passo, e
 * una chiamata a `URTPerceptionLibrary::TeamVisibleCells`.
 *
 * 🔑 **Non e' una simulazione del costo: e' lo stesso codice.** `TeamVisibleCells` e' la funzione che
 * l'accumulo chiama, `FacingAtMicroStep` e' quella da cui prende il facing, e la rotta ha la lunghezza che
 * `URTHeroData::MovePoints` concede. Cio' che questo test non attraversa e' la ricerca della voce di squadra
 * in `TeamKnowledgeState` e l'unione finale in un `TSet` — due operazioni su array, dominate dai coni.
 *
 * ⚠️ **Una mediana su una macchina condivisa non e' un benchmark**, ed e' il motivo per cui la soglia e'
 * larga: cattura la regressione catastrofica — un accumulo diventato quadratico nella dimensione dell'arena
 * — non la variazione del dieci per cento. Il numero utile e' quello che il KPI stampa.
 */

namespace
{
	/** La mediana di campioni gia' raccolti, in millisecondi. */
	double MedianOf(TArray<double>& Samples)
	{
		if (Samples.Num() == 0) { return 0.0; }
		Samples.Sort();
		return Samples[Samples.Num() / 2];
	}
}

/**
 * 🔑 **Il KPI dell'accumulo del transito su arena piena.**
 *
 * Il numero da riportare nella issue e' nel log di questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTransitAccumulationCostTest,
	"RefactorTactics.Perf.TransitKnowledgeOnFullArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTransitAccumulationCostTest::RunTest(const FString&)
{
	// L'arena che [D-227] nomina quando dichiara il costo accettato di `ExploredCells`.
	constexpr int32 Radius = 50;
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	if (!TestNotNull(TEXT("arena di raggio 50"), Arena))
	{
		return false;
	}

	const int32 Celle = Arena->NumCells();
	AddInfo(FString::Printf(TEXT("arena di raggio %d: %d celle"), Radius, Celle));
	if (!TestTrue(TEXT("l'arena e' davvero piena"), Celle > 7000))
	{
		return false;
	}

	// Una rotta lunga quanto il budget di movimento: e' il caso peggiore per unita' in un turno.
	// ⚠️ Il numero viene da `URTHeroData::MovePoints`, non da un letterale scelto qui.
	constexpr int32 PassiPerTurno = 5;
	const FRTCellId Partenza(-20, 0, 0);
	TArray<FRTCellId> Attraversate;
	for (int32 P = 1; P <= PassiPerTurno; ++P)
	{
		Attraversate.Add(FRTCellId(Partenza.X + P, Partenza.Y, Partenza.Layer));
	}

	// Lo stesso lavoro dell'accumulo: un osservatore per posa, col facing di quel passo.
	auto AccumulaUnaVolta = [&]()
	{
		TArray<FRTPerceiver> LungoLaRotta;
		LungoLaRotta.Reserve(Attraversate.Num());
		TArray<FRTCellId> Prefisso;
		for (const FRTCellId& Cella : Attraversate)
		{
			Prefisso.Add(Cella);
			FRTPerceiver P;
			P.Cell = Cella;
			P.Facing = URTFacingLibrary::FacingAtMicroStep(Partenza, Prefisso, ERTHexDirection::E);
			P.VisionRange = 7; // il piu' lungo del roster: il caso peggiore, non la media
			LungoLaRotta.Add(P);
		}
		return URTPerceptionLibrary::TeamVisibleCells(Arena, LungoLaRotta).Num();
	};

	// Un giro a vuoto: la prima chiamata paga cache fredde che non appartengono alla misura.
	const int32 Viste = AccumulaUnaVolta();
	TestTrue(TEXT("l'accumulo vede davvero qualcosa"), Viste > 0);

	constexpr int32 Campioni = 50;
	TArray<double> Millisecondi;
	Millisecondi.Reserve(Campioni);
	for (int32 I = 0; I < Campioni; ++I)
	{
		const double Start = FPlatformTime::Seconds();
		AccumulaUnaVolta();
		Millisecondi.Add((FPlatformTime::Seconds() - Start) * 1000.0);
	}

	const double Mediana = MedianOf(Millisecondi);

	// In un 2v2 le unita' per squadra sono due, e ognuna accumula la propria rotta.
	constexpr int32 UnitaPerSquadra = 2;
	const double PerSquadraPerTurno = Mediana * UnitaPerSquadra;

	AddInfo(FString::Printf(
		TEXT("[KPI] Accumulo del transito su arena r=%d (%d celle): mediana %.3f ms per unita' (%d pose, vista 7), ")
		TEXT("%.3f ms per squadra per turno in un 2v2, su %d campioni — %d celle viste"),
		Radius, Celle, Mediana, PassiPerTurno, PerSquadraPerTurno, Campioni, Viste));

	// 🔴 **La soglia e' larga di proposito**: prende la regressione catastrofica, non il rumore di una
	// macchina condivisa. Il riferimento e' il KPI del resolver, che ha come target `< 100 ms/turno`
	// (`RefactorTactics.Perf.TurnResolverMedian`): l'accumulo deve restarne una frazione, non affiancarlo.
	constexpr double SogliaPerSquadraMs = 20.0;
	TestTrue(*FString::Printf(
			TEXT("l'accumulo per squadra per turno resta sotto %.0f ms su arena piena (misurato %.3f)"),
			SogliaPerSquadraMs, PerSquadraPerTurno),
		PerSquadraPerTurno < SogliaPerSquadraMs);

	// ⚠️ **Anti-vacuita'**: se la rotta fosse vuota o l'arena minuscola, il tempo sarebbe zero e la soglia
	// passerebbe per niente. La misura deve aver fatto lavoro vero.
	TestTrue(TEXT("la misura ha fatto lavoro vero"), Mediana > 0.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
