#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTFacingLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"

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

// ---------------------------------------------------------------------------------------------------------
// Timeline di D-020: il facing cambia PIU' VOLTE per round e ogni consumatore legge il valore piu' recente.
// Lo stato e' uno solo (sull'unita'); la storia sta nel TurnLog, ed e' quella a rendere il round ricostruibile.
// ---------------------------------------------------------------------------------------------------------

/** Le voci di orientamento del log, in ordine. */
static TArray<FRTTurnLogEntry> FacingEntries(const TArray<FRTTurnLogEntry>& Log)
{
	TArray<FRTTurnLogEntry> Out;
	for (const FRTTurnLogEntry& Entry : Log)
	{
		if (Entry.Category == ERTLogCategory::Facing)
		{
			Out.Add(Entry);
		}
	}
	return Out;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingDashThenBlastUsesLatestValueTest,
	"RefactorTactics.Facing.DashThenBlastUsesLatestValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingDashThenBlastUsesLatestValueTest::RunTest(const FString&)
{
	FRTHexSimUnit Unit(1, FRTCellId(0, 0, 0), 5);
	Unit.Facing = ERTHexDirection::W;
	TArray<FRTTurnLogEntry> Log;

	// Lo scatto scrive il proprio orientamento nella fase Dash...
	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::NE, ERTFacingOutcome::DerivedFromDash,
		ERTMatchPhase::Dash, Log);

	// ...e il Blast, che risolve DOPO, deve leggere quello, non il facing di inizio round.
	const ERTHexDirection Used = URTFacingLibrary::ReadFacingForConsumer(Unit, ERTFacingOutcome::UsedByBlast,
		ERTMatchPhase::Blast, Log);
	TestTrue(TEXT("il Blast usa il facing scritto dal Dash"), Used == ERTHexDirection::NE);
	TestTrue(TEXT("e non quello di inizio round"), Used != ERTHexDirection::W);

	// Il log racconta la sequenza: prima la scrittura, poi la lettura, con la fase di ciascuna.
	const TArray<FRTTurnLogEntry> Entries = FacingEntries(Log);
	TestEqual(TEXT("due voci di orientamento"), Entries.Num(), 2);
	if (Entries.Num() == 2)
	{
		TestTrue(TEXT("la prima e' la scrittura dello scatto"),
			static_cast<ERTFacingOutcome>(Entries[0].Outcome) == ERTFacingOutcome::DerivedFromDash
				&& Entries[0].Phase == ERTMatchPhase::Dash);
		TestTrue(TEXT("la seconda e' la lettura del Blast"),
			static_cast<ERTFacingOutcome>(Entries[1].Outcome) == ERTFacingOutcome::UsedByBlast
				&& Entries[1].Phase == ERTMatchPhase::Blast);
		TestEqual(TEXT("entrambe portano la direzione usata"),
			Entries[1].Amount, static_cast<int32>(ERTHexDirection::NE));
	}

	// Poi il Move, ultimo, sovrascrive: e' il FacingFinalAfterMove.
	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::SE, ERTFacingOutcome::DerivedFromMove,
		ERTMatchPhase::Move, Log);
	TestTrue(TEXT("il Move fissa l'orientamento finale"), Unit.Facing == ERTHexDirection::SE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingTargetChangeWithinRoundReorientsTest,
	"RefactorTactics.Facing.TargetChangeWithinRoundReorients",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingTargetChangeWithinRoundReorientsTest::RunTest(const FString&)
{
	FRTHexSimUnit Unit(1, FRTCellId(0, 0, 0), 5);
	Unit.Facing = ERTHexDirection::E;
	TArray<FRTTurnLogEntry> Log;

	// Due bersagli nello stesso round, in due fasi diverse: due orientamenti, non uno solo (D-020).
	const FRTCellId FirstTarget(0, -2, 0);  // a NW
	const FRTCellId SecondTarget(-2, 0, 0); // a W
	ERTHexDirection Towards = ERTHexDirection::E;

	TestTrue(TEXT("direzione del primo bersaglio"),
		URTHexLibrary::DirectionTowards(Unit.Cell, FirstTarget, Towards));
	URTFacingLibrary::RecordFacingChange(Unit, Towards, ERTFacingOutcome::TargetingReoriented,
		ERTMatchPhase::Prep, Log);
	TestTrue(TEXT("orientata sul primo bersaglio"), Unit.Facing == ERTHexDirection::NW);

	TestTrue(TEXT("direzione del secondo bersaglio"),
		URTHexLibrary::DirectionTowards(Unit.Cell, SecondTarget, Towards));
	URTFacingLibrary::RecordFacingChange(Unit, Towards, ERTFacingOutcome::TargetingReoriented,
		ERTMatchPhase::Blast, Log);
	TestTrue(TEXT("riorientata sul secondo bersaglio"), Unit.Facing == ERTHexDirection::W);

	// Il round ha prodotto DUE voci: un campo per turno non basterebbe a ricostruirlo.
	const TArray<FRTTurnLogEntry> Entries = FacingEntries(Log);
	TestEqual(TEXT("due riorientamenti nello stesso round"), Entries.Num(), 2);
	if (Entries.Num() == 2)
	{
		TestNotEqual(TEXT("con direzioni diverse"), Entries[0].Amount, Entries[1].Amount);
		TestTrue(TEXT("e fasi diverse"), Entries[0].Phase != Entries[1].Phase);
	}

	// Riscrivere lo STESSO valore non e' un evento e non sporca il log.
	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::W, ERTFacingOutcome::TargetingReoriented,
		ERTMatchPhase::Blast, Log);
	TestEqual(TEXT("nessuna voce per un non-cambiamento"), FacingEntries(Log).Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRoundInheritsFinalFacingTest,
	"RefactorTactics.Facing.RoundInheritsFinalFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRoundInheritsFinalFacingTest::RunTest(const FString&)
{
	FRTHexSimUnit Unit(1, FRTCellId(0, 0, 0), 5);
	Unit.Facing = ERTHexDirection::E;
	TArray<FRTTurnLogEntry> Log;

	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::SW, ERTFacingOutcome::DerivedFromMove,
		ERTMatchPhase::Move, Log);

	// Il round dopo comincia dallo snapshot delle unita' come sono rimaste: il facing finale e' quello iniziale
	// del round successivo, senza nessun passaggio di travaso.
	const FRTHexSnapshot Next = URTHexSimLibrary::MakeSnapshot(nullptr, { Unit });
	TestEqual(TEXT("l'unita' e' nello snapshot"), Next.Units.Num(), 1);
	if (Next.Units.Num() == 1)
	{
		TestTrue(TEXT("il facing finale diventa quello di inizio round"),
			Next.Units[0].Facing == ERTHexDirection::SW);
	}

	// E non e' un default che coincide per caso: un facing diverso viaggia altrettanto.
	FRTHexSimUnit Other(2, FRTCellId(3, 0, 0), 5);
	Other.Facing = ERTHexDirection::NW;
	const FRTHexSnapshot Two = URTHexSimLibrary::MakeSnapshot(nullptr, { Unit, Other });
	TestEqual(TEXT("due unita' nello snapshot"), Two.Units.Num(), 2);
	if (Two.Units.Num() == 2)
	{
		TestTrue(TEXT("ognuna porta il proprio orientamento"),
			Two.Units[0].Facing == ERTHexDirection::SW && Two.Units[1].Facing == ERTHexDirection::NW);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingTurnLogNamesConsumerAndReasonTest,
	"RefactorTactics.Facing.TurnLogNamesConsumerAndReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingTurnLogNamesConsumerAndReasonTest::RunTest(const FString&)
{
	FRTHexSimUnit Unit(1, FRTCellId(2, -1, 0), 5);
	Unit.Facing = ERTHexDirection::E;
	TArray<FRTTurnLogEntry> Log;

	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::NE, ERTFacingOutcome::DerivedFromDash,
		ERTMatchPhase::Dash, Log);
	URTFacingLibrary::ReadFacingForConsumer(Unit, ERTFacingOutcome::UsedByBlast, ERTMatchPhase::Blast, Log);

	// Una rotazione rifiutata e' un esito osservabile proprio perche' NON cambia niente: se non finisse nel log,
	// il replay non distinguerebbe "ha chiesto e le e' stato negato" da "non ha chiesto".
	URTFacingLibrary::RecordFacingChange(Unit, ERTHexDirection::SW, ERTFacingOutcome::DeclarationRejected,
		ERTMatchPhase::Planning, Log);
	TestTrue(TEXT("il rifiuto non cambia il facing"), Unit.Facing == ERTHexDirection::NE);

	const TArray<FRTTurnLogEntry> Entries = FacingEntries(Log);
	TestEqual(TEXT("tre voci"), Entries.Num(), 3);
	if (Entries.Num() == 3)
	{
		// Il rifiuto registra il facing CONSERVATO, non quello chiesto.
		TestEqual(TEXT("il rifiuto porta la direzione conservata"),
			Entries[2].Amount, static_cast<int32>(ERTHexDirection::NE));
		TestTrue(TEXT("la chiave e' la cella dell'unita'"), Entries[2].SrcCell == Unit.Cell);
	}

	// Ogni voce si descrive: il combat log non deve mostrare una riga vuota per una categoria che non conosce.
	for (const FRTTurnLogEntry& Entry : Entries)
	{
		const FString Text = URTTurnLogLibrary::DescribeEntry(Entry);
		TestTrue(TEXT("la voce ha una descrizione"), !Text.IsEmpty());
		TestTrue(TEXT("e nomina la cella"), Text.Contains(TEXT("2")));
	}

	// L'hash distingue consumatori diversi: se non lo facesse, il TurnLog direbbe *che* si e' letto ma non *chi*,
	// e due round diversi collasserebbero sullo stesso checksum.
	TArray<FRTTurnLogEntry> Blast = { Entries[1] };
	TArray<FRTTurnLogEntry> Overwatch = Blast;
	Overwatch[0].Outcome = static_cast<uint8>(ERTFacingOutcome::UsedByOverwatch);
	TestNotEqual(TEXT("consumatori diversi -> hash diversi"),
		URTTurnLogLibrary::HashTurnLog(Blast), URTTurnLogLibrary::HashTurnLog(Overwatch));

	// E distingue le direzioni: il facing entra nell'hash del replay attraverso `Amount`.
	TArray<FRTTurnLogEntry> OtherDirection = Blast;
	OtherDirection[0].Amount = static_cast<int32>(ERTHexDirection::SW);
	TestNotEqual(TEXT("direzioni diverse -> hash diversi"),
		URTTurnLogLibrary::HashTurnLog(Blast), URTTurnLogLibrary::HashTurnLog(OtherDirection));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingPermutationInvariantTest,
	"RefactorTactics.Facing.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingPermutationInvariantTest::RunTest(const FString&)
{
	// Due unita' che si orientano nello stesso round: l'ordine in cui il resolver le visita non deve cambiare
	// ne' il risultato ne' l'hash del log. E' l'invariante #3 applicata all'orientamento.
	auto RunInOrder = [](bool bFirstUnitFirst, TArray<FRTTurnLogEntry>& OutLog)
	{
		FRTHexSimUnit A(1, FRTCellId(0, 0, 0), 5);
		FRTHexSimUnit B(2, FRTCellId(4, 0, 0), 5);
		A.Facing = ERTHexDirection::E;
		B.Facing = ERTHexDirection::W;

		if (bFirstUnitFirst)
		{
			URTFacingLibrary::RecordFacingChange(A, ERTHexDirection::NE, ERTFacingOutcome::DerivedFromMove,
				ERTMatchPhase::Move, OutLog);
			URTFacingLibrary::RecordFacingChange(B, ERTHexDirection::SW, ERTFacingOutcome::DerivedFromMove,
				ERTMatchPhase::Move, OutLog);
		}
		else
		{
			URTFacingLibrary::RecordFacingChange(B, ERTHexDirection::SW, ERTFacingOutcome::DerivedFromMove,
				ERTMatchPhase::Move, OutLog);
			URTFacingLibrary::RecordFacingChange(A, ERTHexDirection::NE, ERTFacingOutcome::DerivedFromMove,
				ERTMatchPhase::Move, OutLog);
		}
		return TPair<ERTHexDirection, ERTHexDirection>(A.Facing, B.Facing);
	};

	TArray<FRTTurnLogEntry> LogForward;
	TArray<FRTTurnLogEntry> LogReversed;
	const TPair<ERTHexDirection, ERTHexDirection> Forward = RunInOrder(true, LogForward);
	const TPair<ERTHexDirection, ERTHexDirection> Reversed = RunInOrder(false, LogReversed);

	TestTrue(TEXT("stesso orientamento per A"), Forward.Key == Reversed.Key);
	TestTrue(TEXT("stesso orientamento per B"), Forward.Value == Reversed.Value);
	TestEqual(TEXT("stesso hash del log"),
		URTTurnLogLibrary::HashTurnLog(LogForward), URTTurnLogLibrary::HashTurnLog(LogReversed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingIntentIsTeamFilteredTest,
	"RefactorTactics.Facing.IntentIsTeamFiltered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingIntentIsTeamFilteredTest::RunTest(const FString&)
{
	// Un avversario RIVELATO: vede la posa (si vede guardando), non la rotazione dichiarata (e' un piano).
	FRTPlannedIntent Enemy;
	Enemy.OwnerCell = FRTCellId(3, 0, 0);
	Enemy.TeamId = 1;
	Enemy.bRevealed = true;
	Enemy.Facing = ERTHexDirection::NW;
	Enemy.bDeclaresRotation = true;
	Enemy.DeclaredFacing = ERTHexDirection::SE;

	FRTPlannedIntent Ally = Enemy;
	Ally.OwnerCell = FRTCellId(0, 0, 0);
	Ally.TeamId = 0;
	Ally.bRevealed = false;

	const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(0, { Ally, Enemy });
	TestEqual(TEXT("due viste: l'alleato e il nemico rivelato"), Views.Num(), 2);
	if (Views.Num() != 2)
	{
		return false;
	}

	const FRTIntentView& AllyView = Views[0];
	const FRTIntentView& EnemyView = Views[1];

	TestTrue(TEXT("l'alleato riceve la rotazione dichiarata"), AllyView.bDeclaresRotation);
	TestTrue(TEXT("con la direzione giusta"), AllyView.DeclaredFacing == ERTHexDirection::SE);

	// Il campo non e' spedito e nascosto: non e' proprio valorizzato (invariante #6).
	TestFalse(TEXT("l'avversario NON riceve la rotazione dichiarata"), EnemyView.bDeclaresRotation);
	TestTrue(TEXT("e nemmeno la direzione dichiarata"), EnemyView.DeclaredFacing != ERTHexDirection::SE);

	// La posa attuale invece si vede: negarla nasconderebbe cio' che si ha davanti agli occhi.
	TestTrue(TEXT("la posa attuale e' pubblica"), EnemyView.Facing == ERTHexDirection::NW);
	TestTrue(TEXT("e vale anche per l'alleato"), AllyView.Facing == ERTHexDirection::NW);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
