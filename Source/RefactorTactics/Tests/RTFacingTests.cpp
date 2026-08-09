#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTFacingLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 16.1 — il facing e' STATO DI GIOCO, non yaw di presentazione. Questi test fissano la parte pura:
 * quale direzione deriva da un movimento, quali rotazioni sono legali per stile, e cosa succede quando
 * l'unita' viene spostata da qualcun altro invece che dalla propria volonta'.
 *
 * Riferimento: ADR-0005 (orientamento) emendato da D-020 (timeline del facing per round).
 */

/** Percorso di celle adiacenti a partire da Start, seguendo le direzioni indicate. Start incluso. */
static TArray<FRTCellId> MakePath(const FRTCellId& Start, const TArray<ERTHexDirection>& Steps)
{
	TArray<FRTCellId> Path;
	Path.Add(Start);
	FRTCellId Current = Start;
	for (const ERTHexDirection Step : Steps)
	{
		Current = URTHexLibrary::Neighbor(Current, Step);
		Path.Add(Current);
	}
	return Path;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingLinearMoveDerivesDirectionTest,
	"RefactorTactics.Facing.LinearMoveDerivesDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingLinearMoveDerivesDirectionTest::RunTest(const FString&)
{
	const FRTCellId Start(0, 0, 0);
	const TArray<FRTCellId> Path = MakePath(Start, { ERTHexDirection::E, ERTHexDirection::E });

	// Una mobilita' lineare non lascia scelta: la direzione E' il movimento, non un input.
	const TArray<ERTHexDirection> Legal =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::LinearDash, Path, ERTHexDirection::SW);
	TestEqual(TEXT("una sola direzione legale"), Legal.Num(), 1);
	TestTrue(TEXT("ed e' quella del movimento"), Legal.Num() == 1 && Legal[0] == ERTHexDirection::E);

	TestTrue(TEXT("derivata = ultimo passo"),
		URTFacingLibrary::FacingFromPath(Path, ERTHexDirection::SW) == ERTHexDirection::E);

	// Stessa regola per gli altri stili lineari: il vincolo e' dello stile, non della singola azione.
	for (const ERTMovementStyle Style : { ERTMovementStyle::LinearCharge, ERTMovementStyle::LinearLeap,
										  ERTMovementStyle::LinearPass })
	{
		TestEqual(TEXT("gli stili lineari hanno una sola direzione legale"),
			URTFacingLibrary::LegalFacings(Style, Path, ERTHexDirection::SW).Num(), 1);
	}

	// La primitiva geometrica non inventa direzioni fra celle non adiacenti.
	ERTHexDirection Unused = ERTHexDirection::E;
	TestFalse(TEXT("celle non adiacenti non hanno direzione"),
		URTHexLibrary::DirectionBetween(Start, FRTCellId(3, 0, 0), Unused));
	TestFalse(TEXT("layer diverso non e' adiacenza orizzontale"),
		URTHexLibrary::DirectionBetween(Start, FRTCellId(1, 0, 1), Unused));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingBudgetMoveAllowsLastStepPlusMinusOneTest,
	"RefactorTactics.Facing.BudgetMoveAllowsLastStepPlusMinusOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingBudgetMoveAllowsLastStepPlusMinusOneTest::RunTest(const FString&)
{
	// Ultimo passo NE (1): le legali sono NE e le due adiacenti nell'ordine ciclico, E (0) e NW (2).
	const TArray<FRTCellId> Path = MakePath(FRTCellId(0, 0, 0), { ERTHexDirection::E, ERTHexDirection::NE });

	const TArray<ERTHexDirection> Legal =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::Budget, Path, ERTHexDirection::SW);
	TestEqual(TEXT("tre direzioni legali"), Legal.Num(), 3);
	TestTrue(TEXT("contiene l'ultimo passo"), Legal.Contains(ERTHexDirection::NE));
	TestTrue(TEXT("contiene D-1"), Legal.Contains(ERTHexDirection::E));
	TestTrue(TEXT("contiene D+1"), Legal.Contains(ERTHexDirection::NW));

    // Ordine STABILE (per valore dell'enum): l'insieme non dipende dall'ordine di costruzione.
	TestTrue(TEXT("ordinate per valore di enum"),
		Legal.Num() == 3 && Legal[0] == ERTHexDirection::E && Legal[1] == ERTHexDirection::NE
			&& Legal[2] == ERTHexDirection::NW);

	// Il ciclo si chiude: da E (0) le adiacenti sono SE (5) e NE (1), non "-1" che non esiste.
	const TArray<FRTCellId> EastPath = MakePath(FRTCellId(0, 0, 0), { ERTHexDirection::E });
	const TArray<ERTHexDirection> LegalEast =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::Budget, EastPath, ERTHexDirection::W);
	TestTrue(TEXT("il vicino ciclico di E e' SE"), LegalEast.Contains(ERTHexDirection::SE));
	TestTrue(TEXT("e NE"), LegalEast.Contains(ERTHexDirection::NE));

	// Senza rotazione dichiarata resta la derivata dall'ultimo passo.
	TestTrue(TEXT("default = ultimo passo"),
		URTFacingLibrary::FacingFromPath(Path, ERTHexDirection::SW) == ERTHexDirection::NE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRejectsIllegalDeclaredRotationTest,
	"RefactorTactics.Facing.RejectsIllegalDeclaredRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRejectsIllegalDeclaredRotationTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Path = MakePath(FRTCellId(0, 0, 0), { ERTHexDirection::NE });

	// SW e' l'opposta dell'ultimo passo: fuori dall'insieme legale di un movimento a budget.
	ERTHexDirection Result = ERTHexDirection::E;
	const bool bAccepted = URTFacingLibrary::TryApplyDeclaredFacing(
		ERTMovementStyle::Budget, Path, ERTHexDirection::W, ERTHexDirection::SW, Result);

	TestFalse(TEXT("rotazione illegale rifiutata"), bAccepted);
	// RIFIUTATA, non corretta in silenzio: il facing resta quello di partenza, non diventa il piu' vicino legale.
	TestTrue(TEXT("il facing precedente resta"), Result == ERTHexDirection::W);

	// La legale passa e vince sulla derivata.
	ERTHexDirection Legal = ERTHexDirection::E;
	TestTrue(TEXT("rotazione legale accettata"),
		URTFacingLibrary::TryApplyDeclaredFacing(
			ERTMovementStyle::Budget, Path, ERTHexDirection::W, ERTHexDirection::NW, Legal));
	TestTrue(TEXT("applica la dichiarata"), Legal == ERTHexDirection::NW);

	// Su stile lineare NESSUNA dichiarazione diversa dal movimento e' legale.
	ERTHexDirection LinearResult = ERTHexDirection::E;
	TestFalse(TEXT("il lineare non accetta rotazioni dichiarate"),
		URTFacingLibrary::TryApplyDeclaredFacing(
			ERTMovementStyle::LinearDash, Path, ERTHexDirection::W, ERTHexDirection::NW, LinearResult));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingStationaryUnitRotatesFreelyTest,
	"RefactorTactics.Facing.StationaryUnitRotatesFreely",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingStationaryUnitRotatesFreelyTest::RunTest(const FString&)
{
	// Nessun movimento: sei direzioni legali, rotazione libera dichiarata.
	const TArray<FRTCellId> NoPath;
	const TArray<ERTHexDirection> Legal =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::None, NoPath, ERTHexDirection::E);
	TestEqual(TEXT("sei direzioni legali"), Legal.Num(), 6);

	for (uint8 Raw = 0; Raw < 6; ++Raw)
	{
		const ERTHexDirection Declared = static_cast<ERTHexDirection>(Raw);
		ERTHexDirection Result = ERTHexDirection::E;
		TestTrue(TEXT("ogni direzione e' accettata da fermo"),
			URTFacingLibrary::TryApplyDeclaredFacing(
				ERTMovementStyle::None, NoPath, ERTHexDirection::E, Declared, Result));
		TestTrue(TEXT("applica la dichiarata"), Result == Declared);
	}

	// Un percorso di una sola cella non e' un movimento: chi non si sposta non deriva nulla.
	const TArray<FRTCellId> SingleCell = { FRTCellId(0, 0, 0) };
	TestTrue(TEXT("percorso di una cella lascia il facing invariato"),
		URTFacingLibrary::FacingFromPath(SingleCell, ERTHexDirection::SE) == ERTHexDirection::SE);
	TestTrue(TEXT("percorso vuoto lascia il facing invariato"),
		URTFacingLibrary::FacingFromPath(NoPath, ERTHexDirection::SE) == ERTHexDirection::SE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingForcedMovementFacesSourceTest,
	"RefactorTactics.Facing.ForcedMovementFacesSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingForcedMovementFacesSourceTest::RunTest(const FString&)
{
	// Spinta da ovest: l'unita' finisce a est e si gira verso chi l'ha spinta.
	const FRTCellId Source(-1, 0, 0);
	const FRTCellId Landed(1, 0, 0);
	TestTrue(TEXT("guarda la sorgente della spinta"),
		URTFacingLibrary::FacingAfterDisplacement(
			Landed, Source, ERTDisplacementCause::Forced, ERTHexDirection::E) == ERTHexDirection::W);

	// Sorgente non adiacente (spinta di piu' celle): conta la direzione verso di essa, non la sua distanza.
	TestTrue(TEXT("sorgente lontana: stessa direzione"),
		URTFacingLibrary::FacingAfterDisplacement(
			FRTCellId(4, 0, 0), Source, ERTDisplacementCause::Forced, ERTHexDirection::E) == ERTHexDirection::W);

	// Sorgente coincidente con l'arrivo: non c'e' direzione da derivare, il facing resta.
	TestTrue(TEXT("sorgente sulla cella d'arrivo lascia invariato"),
		URTFacingLibrary::FacingAfterDisplacement(
			Landed, Landed, ERTDisplacementCause::Forced, ERTHexDirection::NE) == ERTHexDirection::NE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingEnvironmentalDisplacementKeepsFacingTest,
	"RefactorTactics.Facing.EnvironmentalDisplacementKeepsFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingEnvironmentalDisplacementKeepsFacingTest::RunTest(const FString&)
{
	// Scivolare sul ghiaccio non e' subire una spinta: non c'e' nessuno verso cui girarsi.
	const FRTCellId Slid(2, -1, 0);
	const FRTCellId From(0, 0, 0);
	TestTrue(TEXT("lo scivolamento non ruota l'unita'"),
		URTFacingLibrary::FacingAfterDisplacement(
			Slid, From, ERTDisplacementCause::Environmental, ERTHexDirection::SE) == ERTHexDirection::SE);

	// Vale per ogni facing di partenza: la causa ambientale non tocca l'orientamento, punto.
	for (uint8 Raw = 0; Raw < 6; ++Raw)
	{
		const ERTHexDirection Before = static_cast<ERTHexDirection>(Raw);
		TestTrue(TEXT("invariato per ogni direzione di partenza"),
			URTFacingLibrary::FacingAfterDisplacement(
				Slid, From, ERTDisplacementCause::Environmental, Before) == Before);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingVoluntaryMoveWinsOverForcedTest,
	"RefactorTactics.Facing.VoluntaryMoveWinsOverForced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingVoluntaryMoveWinsOverForcedTest::RunTest(const FString&)
{
	// Prima la spinta: l'unita' guarda verso la sorgente.
	const ERTHexDirection AfterPush = URTFacingLibrary::FacingAfterDisplacement(
		FRTCellId(1, 0, 0), FRTCellId(-1, 0, 0), ERTDisplacementCause::Forced, ERTHexDirection::E);
	TestTrue(TEXT("dopo la spinta guarda la sorgente"), AfterPush == ERTHexDirection::W);

	// Poi il Move volontario, che arriva DOPO nell'ordine delle fasi e sovrascrive.
	const TArray<FRTCellId> Path = MakePath(FRTCellId(1, 0, 0), { ERTHexDirection::SE, ERTHexDirection::SE });
	const ERTHexDirection AfterMove = URTFacingLibrary::FacingFromPath(Path, AfterPush);
	TestTrue(TEXT("il movimento volontario vince"), AfterMove == ERTHexDirection::SE);

	// L'ordine inverso non e' un caso reale, ma fissa che nessuna delle due funzioni "ricorda" la precedente:
	// e' la sequenza di applicazione a decidere, non un flag nascosto dentro la libreria.
	const ERTHexDirection PushAfterMove = URTFacingLibrary::FacingAfterDisplacement(
		FRTCellId(1, 0, 0), FRTCellId(-1, 0, 0), ERTDisplacementCause::Forced, AfterMove);
	TestTrue(TEXT("applicata dopo, la spinta scriverebbe la sua"), PushAfterMove == ERTHexDirection::W);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
