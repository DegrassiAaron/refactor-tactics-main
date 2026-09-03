#include "Misc/AutomationTest.h"
#include "Combat/RTHexCombatLibrary.h" // IsInFrontalArc: #726 misura la divergenza contro il cono, non la sostituisce
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

// ---------------------------------------------------------------------------------------------------------
// #726 — la relazione a SEI DIREZIONI RELATIVE di [D-126], regola a settore semiaperto confermata da [D-147].
//
// ⚠️ Questi test asseriscono sull'INDICE relativo `(spicchio - facing + 6) % 6`, non sui nomi
// `FrontLeft`/`FrontRight`: la mappatura nome<->indice e' l'oggetto della decisione presa in
// `ERTRelativeDirection`, quindi non puo' essere anche la premessa dei test che la verificano. L'unico test
// che nomina i lati e' `RelativeSideNamesFollowGeometry`, ed e' li' apposta.
// ---------------------------------------------------------------------------------------------------------

/** Tutte le celle a distanza `1..RMax` dall'origine, escluso il centro. */
static TArray<FRTCellId> CellsWithinRadius(int32 RMax)
{
	TArray<FRTCellId> Out;
	const FRTCellId Origin(0, 0, 0);
	for (int32 Q = -RMax; Q <= RMax; ++Q)
	{
		for (int32 R = -RMax; R <= RMax; ++R)
		{
			const FRTCellId Cell(Q, R, 0);
			const int32 D = URTHexLibrary::HexDistance(Cell, Origin);
			if (D >= 1 && D <= RMax)
			{
				Out.Add(Cell);
			}
		}
	}
	return Out;
}

/** L'indice relativo, o `INDEX_NONE` se la relazione non e' definita. */
static int32 RelativeIndex(const FRTCellId& Defender, ERTHexDirection Facing, const FRTCellId& Origin)
{
	ERTRelativeDirection Rel = ERTRelativeDirection::Front;
	if (!URTFacingLibrary::RelativeDirectionFrom(Defender, Facing, Origin, Rel))
	{
		return INDEX_NONE;
	}
	return static_cast<int32>(Rel);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRelativeSidesArePartitionedEvenlyTest,
	"RefactorTactics.Facing.RelativeSidesArePartitionedEvenly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRelativeSidesArePartitionedEvenlyTest::RunTest(const FString&)
{
	// 🔑 E' IL test che discrimina la regola adottata da quella scartata. La regola a LINEA — «primo passo
	// della linea difensore->origine», sostituita dallo spec panel del 2026-08-13 — distribuisce
	// `40/36/36/32/36/36`: il settore frontale varia da 32 a 40 celle secondo l'orientamento, cioe' muoversi
	// verso Est proteggerebbe piu' che muoversi verso Ovest. Con questo test rosso, quella regola non rientra
	// per deriva.
	const FRTCellId Defender(0, 0, 0);
	const TArray<FRTCellId> Cells = CellsWithinRadius(8);
	TestEqual(TEXT("216 celle a raggio 1..8"), Cells.Num(), 216);

	int32 PerSide[6] = { 0, 0, 0, 0, 0, 0 };
	int32 Unclassified = 0;
	for (const FRTCellId& Cell : Cells)
	{
		const int32 Rel = RelativeIndex(Defender, ERTHexDirection::E, Cell);
		if (Rel == INDEX_NONE) { ++Unclassified; continue; }
		++PerSide[Rel];
	}

	// Ogni cella cade in ESATTAMENTE uno spicchio: e' il semiaperto `a > 0, b >= 0` a garantirlo, e senza
	// questa riga l'equipartizione sotto potrebbe reggere su un conteggio che perde celle.
	TestEqual(TEXT("nessuna cella resta senza direzione"), Unclassified, 0);
	for (int32 Side = 0; Side < 6; ++Side)
	{
		TestEqual(*FString::Printf(TEXT("36 celle per lo spicchio relativo %d"), Side), PerSide[Side], 36);
	}

	// E l'equipartizione non e' una media che si compensa fra anelli: vale a OGNI raggio, dove il ring di
	// `6r` celle si divide in sei da `r`.
	for (int32 Radius = 1; Radius <= 8; ++Radius)
	{
		int32 PerSideAtRadius[6] = { 0, 0, 0, 0, 0, 0 };
		for (const FRTCellId& Cell : Cells)
		{
			if (URTHexLibrary::HexDistance(Cell, Defender) != Radius) { continue; }
			const int32 Rel = RelativeIndex(Defender, ERTHexDirection::E, Cell);
			if (Rel != INDEX_NONE) { ++PerSideAtRadius[Rel]; }
		}
		for (int32 Side = 0; Side < 6; ++Side)
		{
			TestEqual(*FString::Printf(TEXT("anello %d, spicchio %d"), Radius, Side),
				PerSideAtRadius[Side], Radius);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRelativeDirectionRotatesWithFacingTest,
	"RefactorTactics.Facing.RelativeDirectionRotatesWithFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRelativeDirectionRotatesWithFacingTest::RunTest(const FString&)
{
	const FRTCellId Defender(0, 0, 0);
	const TArray<FRTCellId> Cells = CellsWithinRadius(4);

	// Ruotare di `k` il facing ruota la relazione di `-k`: e' l'equivarianza, cioe' la ragione per cui la
	// relazione e' RELATIVA e non una seconda tabella di direzioni assolute.
	for (const FRTCellId& Cell : Cells)
	{
		const int32 Base = RelativeIndex(Defender, ERTHexDirection::E, Cell);
		TestTrue(TEXT("ogni cella ha una direzione relativa"), Base != INDEX_NONE);
		if (Base == INDEX_NONE) { continue; }

		for (int32 K = 1; K < 6; ++K)
		{
			const ERTHexDirection Rotated = static_cast<ERTHexDirection>(K);
			const int32 Expected = ((Base - K) % 6 + 6) % 6;
			TestEqual(TEXT("ruotando il facing di k la relazione ruota di -k"),
				RelativeIndex(Defender, Rotated, Cell), Expected);
		}
	}

	// `Rear` e' l'opposto di `Front`, e non per convenzione dei nomi: la cella dritto dietro cade a tre
	// spicchi da quella dritto davanti.
	const int32 Ahead = RelativeIndex(Defender, ERTHexDirection::E, FRTCellId(3, 0, 0));
	const int32 Behind = RelativeIndex(Defender, ERTHexDirection::E, FRTCellId(-3, 0, 0));
	TestEqual(TEXT("dritto davanti e' lo spicchio relativo 0"), Ahead, 0);
	TestEqual(TEXT("dritto dietro e' lo spicchio relativo 3"), Behind, 3);
	TestEqual(TEXT("e 3 e' l'opposto di 0 nel ciclo dei sei"), (Ahead + 3) % 6, Behind);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRelativeDirectionEdgeCasesTest,
	"RefactorTactics.Facing.RelativeDirectionEdgeCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRelativeDirectionEdgeCasesTest::RunTest(const FString&)
{
	const FRTCellId Defender(0, 0, 0);

	// Stessa cella: nessun lato d'ingresso esiste, e la funzione lo DICE invece di restituire un valore
	// plausibile. `IsInFrontalArc` alla stessa domanda risponde `true` per contratto — «non e' alle spalle» —
	// e le due risposte non sono in conflitto: quella dice SE sei coperto, questa DA DOVE.
	ERTRelativeDirection Unused = ERTRelativeDirection::Rear;
	TestFalse(TEXT("stessa cella: nessuna direzione"),
		URTFacingLibrary::RelativeDirectionFrom(Defender, ERTHexDirection::E, Defender, Unused));
	TestTrue(TEXT("mentre IsInFrontalArc risponde true per contratto"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, Defender));

	// Layer diverso: si proietta, come fa `IsInFrontalArc`. Ricalcolare diversamente farebbe divergere le due
	// funzioni su un caso che nessun altro test copre.
	const FRTCellId OriginSameLayer(2, -1, 0);
	const FRTCellId OriginOtherLayer(2, -1, 3);
	TestEqual(TEXT("layer diverso: stessa risposta della cella proiettata"),
		RelativeIndex(Defender, ERTHexDirection::E, OriginOtherLayer),
		RelativeIndex(Defender, ERTHexDirection::E, OriginSameLayer));

	// E anche la coincidenza IN PIANTA su layer diversi non ha un lato: sopra la testa non e' una direzione.
	TestFalse(TEXT("stessa cella in pianta su un altro layer: nessuna direzione"),
		URTFacingLibrary::RelativeDirectionFrom(Defender, ERTHexDirection::E, FRTCellId(0, 0, 2), Unused));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRelativeDirectionDivergesFromConeTest,
	"RefactorTactics.Facing.RelativeDirectionDivergesFromCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRelativeDirectionDivergesFromConeTest::RunTest(const FString&)
{
	// La cella `(1,-2)` e' l'UNICA a distanza 2 su cui il cono a 120 gradi e l'insieme dei tre lati frontali
	// divergono, per un difensore in `(0,0)` orientato a `E` ([D-147] corregge qui il corpo di #726, che
	// nominava anche `(-1,2)`: quella cade nello spicchio relativo 4 e non e' una divergenza).
	const FRTCellId Defender(0, 0, 0);
	const FRTCellId Diverging(1, -2, 0);

	TestEqual(TEXT("(1,-2) cade nello spicchio relativo 1"),
		RelativeIndex(Defender, ERTHexDirection::E, Diverging), 1);
	TestEqual(TEXT("(-1,2) cade nello spicchio relativo 4, e non e' una divergenza"),
		RelativeIndex(Defender, ERTHexDirection::E, FRTCellId(-1, 2, 0)), 4);

	// 🔴 E il cono NON cambia risposta su di essa. E' la riga che protegge il bilanciamento: il cono e'
	// strettamente contenuto nei tre lati frontali, quindi sostituirlo sarebbe un BUFF DIFENSIVO NETTO
	// travestito da rinomina. [D-126] tiene le due letture separate proprio per questo.
	TestFalse(TEXT("il cono risponde false su (1,-2): la relazione non l'ha spostato"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, Diverging));

	// Il contenimento e' STRETTO, e si misura: ogni cella dentro il cono sta anche nei tre lati frontali,
	// mai il contrario. Un solo verso di divergenza e' cio' che rende «buff» una previsione e non un'opinione.
	int32 InThreeSidesNotInCone = 0;
	int32 InConeNotInThreeSides = 0;
	for (const FRTCellId& Cell : CellsWithinRadius(10))
	{
		const int32 Rel = RelativeIndex(Defender, ERTHexDirection::E, Cell);
		const bool bThreeFrontal = (Rel == 0 || Rel == 1 || Rel == 5);
		const bool bCone = URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, Cell);
		if (bThreeFrontal && !bCone) { ++InThreeSidesNotInCone; }
		if (bCone && !bThreeFrontal) { ++InConeNotInThreeSides; }
	}
	TestEqual(TEXT("45 divergenze a raggio 1..10, tutte nello stesso verso"), InThreeSidesNotInCone, 45);
	TestEqual(TEXT("e zero nel verso opposto: il cono e' contenuto"), InConeNotInThreeSides, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingRelativeSideNamesFollowGeometryTest,
	"RefactorTactics.Facing.RelativeSideNamesFollowGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingRelativeSideNamesFollowGeometryTest::RunTest(const FString&)
{
	// 🔑 L'unico test che nomina i lati, ed e' quello che pinna la DECISIONE presa in `ERTRelativeDirection`:
	// i nomi seguono la GEOMETRIA e non l'ordine di enumerazione di [D-126]. Senza questa riga la mappatura
	// potrebbe essere invertita da chiunque senza che nulla diventi rosso — ed e' la domanda che [D-147]
	// lascia aperta assegnandola a #726.
	const FRTCellId Defender(0, 0, 0);
	const FVector Origin = FVector::ZeroVector;
	constexpr float HexSize = 100.0f;
	constexpr float LayerHeight = 100.0f;

	// Convenzione UE: `+X` avanti, `+Y` a destra. Il difensore guarda a `E`, che `AxialToWorld` manda su `+X`.
	const FVector Ahead = URTHexLibrary::AxialToWorld(URTHexLibrary::Neighbor(Defender, ERTHexDirection::E),
		Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il facing E guarda verso +X in world"), Ahead.X > 1.0);
	TestTrue(TEXT("e non ha componente laterale"), FMath::Abs(Ahead.Y) < 1.0);

	ERTRelativeDirection Side = ERTRelativeDirection::Front;

	// Lo spicchio relativo 1 e' quello del vicino `NE`, che in world sta a `-Y`, cioe' a SINISTRA di chi
	// guarda a `E`. Percio' si chiama `FrontLeft` e non `FrontRight`.
	const FRTCellId LeftNeighbour = URTHexLibrary::Neighbor(Defender, ERTHexDirection::NE);
	const FVector LeftWorld = URTHexLibrary::AxialToWorld(LeftNeighbour, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il vicino NE sta a -Y, cioe' a sinistra"), LeftWorld.Y < -1.0);
	TestTrue(TEXT("e la relazione lo chiama FrontLeft"),
		URTFacingLibrary::RelativeDirectionFrom(Defender, ERTHexDirection::E, LeftNeighbour, Side)
		&& Side == ERTRelativeDirection::FrontLeft);

	// Simmetricamente, `SE` sta a `+Y` ed e' `FrontRight`.
	const FRTCellId RightNeighbour = URTHexLibrary::Neighbor(Defender, ERTHexDirection::SE);
	const FVector RightWorld = URTHexLibrary::AxialToWorld(RightNeighbour, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il vicino SE sta a +Y, cioe' a destra"), RightWorld.Y > 1.0);
	TestTrue(TEXT("e la relazione lo chiama FrontRight"),
		URTFacingLibrary::RelativeDirectionFrom(Defender, ERTHexDirection::E, RightNeighbour, Side)
		&& Side == ERTRelativeDirection::FrontRight);

	// I sei nomi restano quelli di [D-126] e il ciclo e' completo: cambiare l'ordine dell'enum senza cambiare
	// questo test e' cio' che non deve poter succedere in silenzio.
	TestEqual(TEXT("Front e' 0"), static_cast<int32>(ERTRelativeDirection::Front), 0);
	TestEqual(TEXT("FrontLeft e' 1"), static_cast<int32>(ERTRelativeDirection::FrontLeft), 1);
	TestEqual(TEXT("RearLeft e' 2"), static_cast<int32>(ERTRelativeDirection::RearLeft), 2);
	TestEqual(TEXT("Rear e' 3"), static_cast<int32>(ERTRelativeDirection::Rear), 3);
	TestEqual(TEXT("RearRight e' 4"), static_cast<int32>(ERTRelativeDirection::RearRight), 4);
	TestEqual(TEXT("FrontRight e' 5"), static_cast<int32>(ERTRelativeDirection::FrontRight), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingHitCameFromSideEntryTest,
	"RefactorTactics.Facing.HitCameFromSideEntryIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingHitCameFromSideEntryTest::RunTest(const FString&)
{
	// 🔑 Il produttore vive dentro `ResolveCombatPasses` e non e' esercitabile senza un TurnManager, ma il
	// CONTRATTO della voce si', ed e' quello che qui si pinna: la tassonomia del soggetto e il rendering.
	// Senza, riordinare `ERTRelativeDirection` sfaserebbe la tabella di `DescribeEntry` in silenzio, e
	// rimettere l'attaccante in `SrcCell` — il difetto di privacy corretto in `#726` — non darebbe rosso.
	const FRTCellId Defender(4, -2, 1);

	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Blast;
	Entry.Category = ERTLogCategory::Facing;
	Entry.Outcome = static_cast<uint8>(ERTFacingOutcome::HitCameFromSide);
	Entry.SrcCell = Defender;
	Entry.TgtCell = Defender;
	Entry.UnitId = 7;

	// 1. Il soggetto e' CHI SUBISCE, e la tassonomia lo sa. Un produttore che inverte il soggetto e non
	//    compare in `IsSubjectTheSufferer` fa accreditare alla vittima di aver AGITO (`#1418`).
	Entry.Amount = static_cast<int32>(ERTRelativeDirection::Rear);
	TestTrue(TEXT("il soggetto della voce e' chi subisce"),
		URTTurnLogLibrary::IsSubjectTheSufferer(Entry));

	// 2. Ognuno dei sei lati si rende col proprio nome, e i nomi seguono l'ordine dell'enum.
	const TCHAR* Expected[6] = { TEXT("Front"), TEXT("FrontLeft"), TEXT("RearLeft"),
								 TEXT("Rear"), TEXT("RearRight"), TEXT("FrontRight") };
	for (int32 Side = 0; Side < 6; ++Side)
	{
		Entry.Amount = Side;
		const FString Text = URTTurnLogLibrary::DescribeEntry(Entry);
		TestTrue(*FString::Printf(TEXT("il lato %d si rende come '%s' (ottenuto: %s)"),
			Side, Expected[Side], *Text), Text.Contains(Expected[Side]));
	}

	// ⚠️ `Front` e' sottostringa di `FrontLeft` e `FrontRight`: senza questa riga il ciclo sopra passerebbe
	//    anche con una tabella che risponde sempre `FrontLeft`.
	Entry.Amount = static_cast<int32>(ERTRelativeDirection::Front);
	const FString FrontText = URTTurnLogLibrary::DescribeEntry(Entry);
	TestFalse(TEXT("il lato Front non si rende come FrontLeft"), FrontText.Contains(TEXT("FrontLeft")));
	TestFalse(TEXT("ne' come FrontRight"), FrontText.Contains(TEXT("FrontRight")));

	// 3. Un `Amount` fuori intervallo si DICHIARA non tradotto invece di diventare `Front`: una traccia
	//    corrotta non deve produrre una frase sicura e sbagliata.
	Entry.Amount = 9;
	const FString Untranslated = URTTurnLogLibrary::DescribeEntry(Entry);
	TestTrue(TEXT("un lato fuori intervallo si dichiara non tradotto"),
		Untranslated.Contains(TEXT("non tradotto")));
	TestFalse(TEXT("e non ripiega su Front"), Untranslated.Contains(TEXT("lato Front")));

	// 4. 🔴 PRIVACY: la voce non contiene la cella dell'ATTACCANTE. Il verdetto di visibilita' si congela su
	//    chi subisce, quindi una posizione dell'attaccante qui sarebbe pubblicata a chi vede il bersaglio —
	//    su ogni colpo risolto, non sui rari bypass. Entrambe le celle sono quelle del difensore.
	TestTrue(TEXT("SrcCell e TgtCell portano la stessa cella"), Entry.SrcCell == Entry.TgtCell);
	Entry.Amount = static_cast<int32>(ERTRelativeDirection::FrontLeft);
	const FString Rendered = URTTurnLogLibrary::DescribeEntry(Entry);
	TestFalse(TEXT("il rendering non mostra due celle"), Rendered.Contains(TEXT("->")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
