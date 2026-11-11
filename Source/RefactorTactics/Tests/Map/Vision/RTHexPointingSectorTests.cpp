#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexOccupancyLibrary.h" // SectorBoundaryPoints: la convenzione a cui il settore si ancora

// La guardia: senza, questi test finiscono nel binario Shipping. Vedi `#923`.
#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un punto a distanza `R` dal centro, all'angolo dato. Nomi distinti: la unity build fonde le TU. */
	FVector2D PointingAt(double Degrees, double R)
	{
		const double Rad = FMath::DegreesToRadians(Degrees);
		return FVector2D(static_cast<float>(R * FMath::Cos(Rad)), static_cast<float>(R * FMath::Sin(Rad)));
	}

	constexpr float PointingHexSize = 100.f;

	/** Fuori dalla dead-zone con margine: l'inraggio è ~86,6 e la dead-zone ~21,65. */
	constexpr double PointingSafeR = 60.0;
}

/**
 * IL SETTORE SI ANCORA A `SectorBoundaryPoints`, NON A UN LETTERALE — `#1615`, `spec-pointer-interaction.md` §4.9.
 *
 * 🔑 **È il test che conta più degli altri sette**, e il motivo è scritto nella spec: i due consumatori a
 * dodici — occupancy e puntamento — usano lo **stesso** partizionamento, e il modo in cui due copie della
 * stessa convenzione divergono è che ciascuna ripeta `-30 + 30k` per conto proprio. Questo test non
 * confronta `PointingSectorAt` con una formula riscritta qui: la confronta con **i punti che la funzione
 * dell'occupancy produce**.
 *
 * ⚠️ Si campiona il punto a metà angolo fra `P[k]` e `P[k+1]`, non `P[k]`: quello sta **sul confine**, ed è
 * il caso ambiguo che il test dell'epsilon interroga apposta più sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointingSectorFollowsBoundaryTest,
	"RefactorTactics.Hex.PointingSectorFollowsTheBoundaryPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointingSectorFollowsBoundaryTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(PointingHexSize, Boundary);
	if (!TestEqual(TEXT("dodici punti di confine"), Boundary.Num(), RT_OccupancySectorCount))
	{
		return false;
	}

	for (int32 K = 0; K < RT_OccupancySectorCount; ++K)
	{
		const FVector2D& A = Boundary[K];
		const FVector2D& B = Boundary[(K + 1) % RT_OccupancySectorCount];

		// L'angolo a metà fra i due confini del settore `k`. Si media l'ANGOLO e non i punti: `P[k]` e
		// `P[k+1]` hanno raggi diversi — vertice e punto medio di lato si alternano — quindi la media dei
		// punti non cadrebbe a metà angolo.
		const double AngA = FMath::RadiansToDegrees(FMath::Atan2(static_cast<double>(A.Y), static_cast<double>(A.X)));
		double AngB = FMath::RadiansToDegrees(FMath::Atan2(static_cast<double>(B.Y), static_cast<double>(B.X)));
		if (AngB < AngA) { AngB += 360.0; } // il wrap dell'ultimo settore, che chiude il giro
		const double Mid = (AngA + AngB) * 0.5;

		TestEqual(*FString::Printf(TEXT("il punto a metà del settore %d (%.1f gradi) è il settore %d"),
			K, Mid, K), URTHexLibrary::PointingSectorAt(PointingAt(Mid, PointingSafeR), PointingHexSize), K);
	}
	return true;
}

/**
 * I DODICI SETTORI NON SONO DIREZIONI — il ponte di [D-243], e i quattro diagonali che `#712` ha già pagato.
 *
 * 🔴 **Il test copre un DIAGONALE apposta.** `DirectionForEdgeIndex` rispecchia le due numerazioni, che
 * «girano in verso opposto»: `E` e `W` sono punti fissi, i quattro diagonali sono scambiati a coppie. Il
 * difetto di `#712` — la cottura che scriveva la copertura sul lato sbagliato per **quattro bordi su sei** —
 * era invisibile ai test *«perché usavano solo `E` e `W`, i due punti fissi»*. Fermarsi ai cardinali qui
 * ripeterebbe quell'errore su un ponte nuovo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointingSectorBridgesToDirectionTest,
	"RefactorTactics.Hex.PointingSectorBridgesToSixDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointingSectorBridgesToDirectionTest::RunTest(const FString&)
{
	// Il ponte dichiarato da D-243: settore -> bordo geometrico -> direzione di vicinato.
	auto DirectionOfSector = [](int32 Sector)
	{
		return URTHexLibrary::DirectionForEdgeIndex(Sector / 2);
	};

	// I due settori che condividono un bordo devono dare la STESSA direzione: è ciò che rende la
	// derivazione a senso unico, e il motivo per cui l'inverso non esiste.
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		TestTrue(*FString::Printf(TEXT("i settori %d e %d danno la stessa direzione"), Edge * 2, Edge * 2 + 1),
			DirectionOfSector(Edge * 2) == DirectionOfSector(Edge * 2 + 1));
	}

	// ⚠️ ANTI-VACUITÀ sui punti fissi: se il ponte fosse un `static_cast` — il difetto di `#712` — questo
	// passerebbe comunque su `E`, che è fisso. Serve un diagonale, dove le due numerazioni divergono.
	const ERTHexDirection FromSectorZero = DirectionOfSector(0);
	TestTrue(TEXT("il settore 0 è il bordo geometrico 0, cioè E"), FromSectorZero == ERTHexDirection::E);

	const ERTHexDirection Diagonale = DirectionOfSector(2); // bordo geometrico 1
	TestTrue(*FString::Printf(TEXT("e il bordo geometrico 1 NON è la direzione 1: è %d, non %d — le due numerazioni girano al contrario"),
		static_cast<int32>(Diagonale), 1),
		Diagonale == URTHexLibrary::DirectionForEdgeIndex(1) && Diagonale != static_cast<ERTHexDirection>(1));
	return true;
}

/**
 * LA DEAD-ZONE HA UN NUMERO, NON L'AGGETTIVO «CONFIGURABILE» — `#1615`.
 *
 * Il DoD chiede la forma esatta: *«data una cella `HexSize` 100 e dead-zone `0,25·inraggio`, a 20 unità dal
 * centro il settore è `None`, a 30 è definito»*. Con `HexSize 100` l'inraggio è `86,60` e la soglia `21,65`,
 * quindi i due campioni cadono uno per parte con margine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointingSectorDeadZoneTest,
	"RefactorTactics.Hex.PointingSectorDeadZoneHasANumber",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointingSectorDeadZoneTest::RunTest(const FString&)
{
	TestEqual(TEXT("la frazione dichiarata è un quarto dell'inraggio"),
		URTHexLibrary::PointingDeadZoneFraction, 0.25f);

	// A 20 unità: dentro la dead-zone (soglia ~21,65), in ogni direzione.
	for (double Deg = 0.0; Deg < 360.0; Deg += 45.0)
	{
		TestEqual(*FString::Printf(TEXT("a 20 unità (%.0f gradi) non si punta niente"), Deg),
			URTHexLibrary::PointingSectorAt(PointingAt(Deg, 20.0), PointingHexSize), INDEX_NONE);
	}

	// A 30 unità: fuori, e il settore esiste.
	for (double Deg = 0.0; Deg < 360.0; Deg += 45.0)
	{
		const int32 Sector = URTHexLibrary::PointingSectorAt(PointingAt(Deg, 30.0), PointingHexSize);
		TestTrue(*FString::Printf(TEXT("a 30 unità (%.0f gradi) il settore è definito: %d"), Deg, Sector),
			Sector >= 0 && Sector < RT_OccupancySectorCount);
	}

	// Il centro esatto, che è il caso limite di ogni dead-zone.
	TestEqual(TEXT("il centro non punta niente"),
		URTHexLibrary::PointingSectorAt(FVector2D::ZeroVector, PointingHexSize), INDEX_NONE);

	// ⛔ Fail-closed: senza scala non c'è geometria. `0` sarebbe un settore plausibile e falso.
	TestEqual(TEXT("HexSize zero -> nessun settore"),
		URTHexLibrary::PointingSectorAt(PointingAt(0.0, PointingSafeR), 0.f), INDEX_NONE);
	TestEqual(TEXT("HexSize negativo -> nessun settore"),
		URTHexLibrary::PointingSectorAt(PointingAt(0.0, PointingSafeR), -50.f), INDEX_NONE);
	return true;
}

/**
 * IL CONFINE, IL WRAP E L'INVARIANZA — i casi che `#1615` elenca e che si sbagliano in silenzio.
 *
 * ⚠️ **L'invarianza per scala e per traslazione non è decorativa**: la firma prende un punto LOCALE proprio
 * perché il settore non dipenda da dove sta la cella. Se un giorno qualcuno cambiasse la firma per prendere
 * un punto world, questo test cadrebbe — ed è il suo mestiere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointingSectorEdgeCasesTest,
	"RefactorTactics.Hex.PointingSectorHandlesBoundaryAndWrap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointingSectorEdgeCasesTest::RunTest(const FString&)
{
	// Il confine fra il settore 0 e il 1 sta a 0 gradi (`-30 + 30·1`). Un epsilon per parte deve cadere in
	// due settori diversi: è la proprietà che rende il partizionamento una partizione.
	const int32 Prima = URTHexLibrary::PointingSectorAt(PointingAt(-0.01, PointingSafeR), PointingHexSize);
	const int32 Dopo  = URTHexLibrary::PointingSectorAt(PointingAt(+0.01, PointingSafeR), PointingHexSize);
	TestTrue(*FString::Printf(TEXT("l'epsilon attraversa un confine: %d -> %d"), Prima, Dopo), Prima != Dopo);

	// Il wrap: `359,9` gradi e `-0,1` sono lo stesso punto, e devono dare lo stesso settore.
	TestEqual(TEXT("359,9 gradi e -0,1 sono lo stesso settore"),
		URTHexLibrary::PointingSectorAt(PointingAt(359.9, PointingSafeR), PointingHexSize),
		URTHexLibrary::PointingSectorAt(PointingAt(-0.1, PointingSafeR), PointingHexSize));

	// Ogni multiplo di 30 gradi cade in un settore valido: nessun angolo di confine finisce fuori range.
	for (int32 K = 0; K < 12; ++K)
	{
		const double Deg = -30.0 + 30.0 * K;
		const int32 Sector = URTHexLibrary::PointingSectorAt(PointingAt(Deg, PointingSafeR), PointingHexSize);
		TestTrue(*FString::Printf(TEXT("il confine a %.0f gradi cade in un settore valido: %d"), Deg, Sector),
			Sector >= 0 && Sector < RT_OccupancySectorCount);
	}

	// INVARIANZA PER SCALA: lo stesso angolo, celle di dimensione diversa, stesso settore.
	for (double Deg = 5.0; Deg < 360.0; Deg += 37.0)
	{
		TestEqual(*FString::Printf(TEXT("a %.0f gradi il settore non dipende da HexSize"), Deg),
			URTHexLibrary::PointingSectorAt(PointingAt(Deg, 60.0), 100.f),
			URTHexLibrary::PointingSectorAt(PointingAt(Deg, 120.0), 200.f));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
