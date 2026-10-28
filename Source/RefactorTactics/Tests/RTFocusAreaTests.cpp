// L'area di movimento di chi e' a fuoco (`#3750`, fetta F5, D-552): il potenziale di un nemico osservato, mai il suo
// piano, calcolato su cio' che la squadra vede adesso. Fetta A: il calcolo puro, senza cablaggio.
//
// Ogni divieto ha il suo gemello. Un test che mostra solo la forma giusta passerebbe anche con un'area che dipende dal
// piano o dalla geometria nascosta: il difetto che D-552 esiste per impedire non cambia la forma, cambia da cosa dipende.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTMovementProfileLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTFocusArea.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	constexpr int32 FocRadius = 8;

	URTHexMapAsset* FocArena()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), FocRadius);
	}

	FRTTeamKnowledge FocKnowledge(const URTHexMapAsset* Map, TFunctionRef<bool(const FRTCellId&)> IsVisible)
	{
		FRTTeamKnowledge Knowledge;
		Knowledge.TeamId = 0;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (IsVisible(Cell.Id)) { Knowledge.VisibleCells.Add(Cell.Id); }
		}
		return Knowledge;
	}

	FRTTeamKnowledge FocAllVisible(const URTHexMapAsset* Map)
	{
		return FocKnowledge(Map, [](const FRTCellId&) { return true; });
	}

	/** Le celle dell'arena entro `Radius` dal centro. L'oracolo e' la distanza esagonale, non il codice sotto prova. */
	TArray<FRTCellId> FocWithin(const URTHexMapAsset* Map, const FRTCellId& Center, int32 Radius)
	{
		TArray<FRTCellId> Cells;
		for (const FRTCellId& Cell : URTHexLibrary::HexArea(Center, Radius))
		{
			if (Map->ContainsCell(Cell)) { Cells.Add(Cell); }
		}
		Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		return Cells;
	}

	FRTFocusMoveSubject FocSubject(int32 Base, int32 SlowCost = 0, bool bRunDenied = false)
	{
		FRTFocusMoveSubject Subject;
		Subject.Cell = FRTCellId(0, 0, 0);
		Subject.BaseMoveRange = Base;
		Subject.MoveCostModifier = SlowCost;
		Subject.bRunDenied = bRunDenied;
		return Subject;
	}

	bool FocContainsAll(const TArray<FRTCellId>& Whole, const TArray<FRTCellId>& Part)
	{
		for (const FRTCellId& Cell : Part) { if (!Whole.Contains(Cell)) { return false; } }
		return true;
	}

	ARTUnit* FocSpawn(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (Unit == nullptr) { return nullptr; }
		Unit->TeamId = TeamId;
		// Un eroe NUOVO per unita': le azioni sono oggetti del `URTHeroData`, e condividerlo le legherebbe fra loro.
		Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return Unit;
	}
}

// Le due bande sono l'`1x` e il `2x` del movimento base, e la normale sta dentro la corsa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaBandsTest,
	"RefactorTactics.FocusArea.TwoBandsAreOneAndTwoTimesTheBaseRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaBandsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTCellId Center(0, 0, 0);

	const FRTFocusMoveAreas Areas = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map), FocSubject(3));

	TestTrue(TEXT("normale: tutte e sole le celle entro 3 passi"), Areas.Normal == FocWithin(Map, Center, 3));
	TestTrue(TEXT("corsa: tutte e sole le celle entro 6 passi"), Areas.Run == FocWithin(Map, Center, 6));
	TestFalse(TEXT("la corsa non e' negata"), Areas.bRunDenied);

	// Anti-vacuita': le due aree sono davvero diverse, e la normale sta dentro la corsa.
	TestTrue(TEXT("la corsa e' piu' grande della normale"), Areas.Run.Num() > Areas.Normal.Num() && Areas.Normal.Num() > 1);
	TestTrue(TEXT("la normale sta dentro la corsa"), FocContainsAll(Areas.Run, Areas.Normal));

	// Le liste sono ordinate in modo stabile: due chiamate danno le stesse liste.
	const FRTFocusMoveAreas Again = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map), FocSubject(3));
	TestTrue(TEXT("stessi ingressi, stesse liste"), Again.Normal == Areas.Normal && Again.Run == Areas.Run);
	return true;
}

// Con la corsa negata da uno stato pubblico resta la sola area normale, e non cambia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaRunDeniedTest,
	"RefactorTactics.FocusArea.ADeniedRunLeavesOnlyTheNormalArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaRunDeniedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTCellId Center(0, 0, 0);

	const FRTFocusMoveAreas Allowed = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map), FocSubject(3));
	const FRTFocusMoveAreas Denied = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map),
		FocSubject(3, /*SlowCost=*/ 0, /*bRunDenied=*/ true));

	TestTrue(TEXT("corsa negata: la dichiarazione c'e'"), Denied.bRunDenied);
	TestEqual(TEXT("corsa negata: nessuna area di corsa"), Denied.Run.Num(), 0);
	TestTrue(TEXT("corsa negata: l'area normale e' la stessa di prima"), Denied.Normal == Allowed.Normal && Denied.Normal == FocWithin(Map, Center, 3));
	TestTrue(TEXT("controllo positivo: senza il divieto la corsa c'e'"), Allowed.Run.Num() > 0);
	return true;
}

// `Slow` aggiunge un costo a ogni cella, e il calcolo lo riporta: `GetEffectiveMoveRange` non lo legge.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaSlowTest,
	"RefactorTactics.FocusArea.SlowAddsOneToEveryCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaSlowTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTCellId Center(0, 0, 0);

	// Base 3: budget 3 (normale) e 6 (corsa), a 2 per cella: 1 passo e 3 passi.
	const FRTFocusMoveAreas Slowed = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map),
		FocSubject(3, /*SlowCost=*/ 1));
	const FRTFocusMoveAreas Plain = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map), FocSubject(3));

	TestTrue(TEXT("Slow: la normale arriva a 1 passo"), Slowed.Normal == FocWithin(Map, Center, 1));
	TestTrue(TEXT("Slow: la corsa arriva a 3 passi"), Slowed.Run == FocWithin(Map, Center, 3));
	TestTrue(TEXT("controllo positivo: senza Slow le aree sono piu' grandi"),
		Plain.Normal.Num() > Slowed.Normal.Num() && Plain.Run.Num() > Slowed.Run.Num());
	return true;
}

// 🔴 LE CELLE NON IN VISTA SONO MURI NEL CALCOLO, NON UN RITAGLIO DEL RISULTATO. Una cintura di celle non in vista a
// distanza 2: le celle oltre (in vista, entro il budget) si raggiungerebbero solo attraversandola.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaHiddenCellsAreWallsTest,
	"RefactorTactics.FocusArea.HiddenCellsAreWallsInTheComputationNotAClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaHiddenCellsAreWallsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTCellId Center(0, 0, 0);

	const FRTTeamKnowledge Ringed = FocKnowledge(Map, [&Center](const FRTCellId& Cell) { return URTHexLibrary::HexDistance(Center, Cell) != 2; });
	const FRTTeamKnowledge Open = FocAllVisible(Map);
	const FRTFocusMoveSubject Subject = FocSubject(4); // normale 4, corsa 8

	const FRTFocusMoveAreas Free = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, Open, Subject);
	const FRTFocusMoveAreas Walled = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, Ringed, Subject);

	// Controllo positivo: a cintura visibile le celle a 3 e 4 passi si raggiungono.
	TestTrue(TEXT("senza cintura, la cella a 3 passi e' nell'area"), Free.Normal.Contains(FRTCellId(3, 0, 0)));
	TestTrue(TEXT("senza cintura, la cella a 4 passi e' nell'area"), Free.Normal.Contains(FRTCellId(4, 0, 0)));

	// Con la cintura il soggetto e' chiuso dentro: arriva solo alle celle a 1 passo, in entrambe le bande.
	TestTrue(TEXT("con la cintura, la normale si ferma a 1 passo"), Walled.Normal == FocWithin(Map, Center, 1));
	TestTrue(TEXT("con la cintura, anche la corsa"), Walled.Run == FocWithin(Map, Center, 1));

	// La prova che il muro sta NEL calcolo: la cella a 3 passi e' IN VISTA ed entro il budget, quindi un ritaglio del
	// risultato (calcolare sulla mappa intera e togliere le celle non in vista) la lascerebbe dentro.
	TestTrue(TEXT("premessa: la cella a 3 passi e' in vista"), Ringed.VisibleCells.Contains(FRTCellId(3, 0, 0)));
	TestFalse(TEXT("ma e' fuori dall'area: raggiungibile solo attraverso celle non in vista"), Walled.Normal.Contains(FRTCellId(3, 0, 0)));
	TestFalse(TEXT("anche per la corsa"), Walled.Run.Contains(FRTCellId(3, 0, 0)));
	return true;
}

// MONDI GEMELLI (P3). Due mappe che differiscono SOLO in cio' che la squadra non vede adesso danno la stessa area: le
// celle mai viste e le celle esplorate e non piu' in vista. E il controllo positivo: una differenza in una cella in vista
// la cambia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaTwinWorldsTest,
	"RefactorTactics.FocusArea.AreaIsTheSameWhenOnlyWhatIsHiddenDiffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaTwinWorldsTest::RunTest(const FString&)
{
	URTHexMapAsset* WorldA = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), WorldA)) { return false; }
	URTHexMapAsset* WorldB = DuplicateObject<URTHexMapAsset>(WorldA, GetTransientPackage());
	URTHexMapAsset* WorldC = DuplicateObject<URTHexMapAsset>(WorldA, GetTransientPackage());
	if (!TestTrue(TEXT("le gemelle esistono"), WorldB && WorldC)) { return false; }

	// Il buio: tutto cio' che e' a est (q >= 3) o a sud (r >= 3). Le celle a sud sono ESPLORATE ma non piu' in vista; quelle a
	// est sono mai viste. Entrambe le regioni stanno dentro la corsa (budget 6) del soggetto in (0,0).
	auto IsDark = [](const FRTCellId& Cell) { return Cell.X >= 3 || Cell.Y >= 3; };
	FRTTeamKnowledge Knowledge = FocKnowledge(WorldA, [&IsDark](const FRTCellId& Cell) { return !IsDark(Cell); });
	for (const FRTHexCellData& Cell : WorldA->Cells)
	{
		if (Cell.Id.Y >= 3) { Knowledge.ExploredCells.Add(Cell.Id); }
	}

	// B differisce da A solo nel buio: li' le celle sono muri, in A sono aperte.
	for (FRTHexCellData& Cell : WorldB->Cells)
	{
		if (IsDark(Cell.Id)) { Cell.bBlocksMovement = true; }
	}
	// C differisce da A in una cella IN VISTA.
	for (FRTHexCellData& Cell : WorldC->Cells)
	{
		if (Cell.Id == FRTCellId(1, 0, 0)) { Cell.bBlocksMovement = true; }
	}

	const FRTFocusMoveSubject Subject = FocSubject(3);
	const FRTFocusMoveAreas InA = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), WorldA, Knowledge, Subject);
	const FRTFocusMoveAreas InB = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), WorldB, Knowledge, Subject);
	const FRTFocusMoveAreas InC = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), WorldC, Knowledge, Subject);

	TestTrue(TEXT("la geometria mai vista e quella esplorata e non piu' in vista non cambiano l'area normale"), InA.Normal == InB.Normal);
	TestTrue(TEXT("ne' la corsa"), InA.Run == InB.Run);

	// Controllo positivo 1: le due mappe SI DISTINGUONO se le si guardasse per intero.
	const FRTFocusMoveAreas SeenByAll = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), WorldA, FocAllVisible(WorldA), Subject);
	const FRTFocusMoveAreas SeenByAllB = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), WorldB, FocAllVisible(WorldB), Subject);
	TestTrue(TEXT("premessa: guardando tutto, A e B darebbero aree diverse"), SeenByAll.Run != SeenByAllB.Run);
	TestTrue(TEXT("premessa: il buio tocca la corsa del soggetto"), SeenByAll.Run.Num() > InA.Run.Num());

	// Controllo positivo 2: una differenza in una cella in vista cambia l'area.
	TestTrue(TEXT("una cella in vista cambiata cambia l'area"), InA.Normal != InC.Normal);
	return true;
}

// Un soggetto la cui cella la squadra non vede adesso non ha area.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaUnseenSubjectTest,
	"RefactorTactics.FocusArea.ASubjectOnAnUnseenCellHasNoArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaUnseenSubjectTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = FocArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTCellId Center(0, 0, 0);

	FRTTeamKnowledge Blind = FocKnowledge(Map, [&Center](const FRTCellId& Cell) { return !(Cell == Center); });
	// La cella del soggetto e' ESPLORATA ma non piu' in vista: dice dove sta l'unita' quanto una cella mai vista, e non
	// deve bastare (D-225, D-227). Senza questa riga una mutazione nel solo gate del soggetto passerebbe.
	Blind.ExploredCells.Add(Center);
	const FRTFocusMoveAreas Hidden = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, Blind, FocSubject(3));
	TestEqual(TEXT("cella del soggetto non in vista: nessuna area normale"), Hidden.Normal.Num(), 0);
	TestEqual(TEXT("ne' di corsa"), Hidden.Run.Num(), 0);

	const FRTFocusMoveAreas Seen = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, FocAllVisible(Map), FocSubject(3));
	TestTrue(TEXT("controllo positivo: in vista, le aree ci sono"), Seen.Normal.Num() > 0 && Seen.Run.Num() > 0);
	return true;
}

// 🔴 MAI IL PIANO. Due unita' identiche che differiscono solo nel piano danno lo stesso soggetto e la stessa area; gli stati
// pubblici, invece, lo cambiano. Il piano nemico piu' pericoloso e' quello che dichiara la corsa: con il budget del piano
// l'area crescerebbe, e a forma invariata nessun altro test se ne accorgerebbe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaNeverReadsThePlanTest,
	"RefactorTactics.FocusArea.TheSubjectIsTheSameWhateverTheEnemyPlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaNeverReadsThePlanTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	const FRTCellId Cell(0, 0, 0);
	ARTUnit* Quiet = FocSpawn(World, 1, Cell);
	ARTUnit* Planner = FocSpawn(World, 1, Cell);
	ARTUnit* Slowed = FocSpawn(World, 1, Cell);
	ARTUnit* Unbalanced = FocSpawn(World, 1, Cell);
	ARTUnit* Rooted = FocSpawn(World, 1, Cell);
	if (!TestTrue(TEXT("le unita' esistono"), Quiet && Planner && Slowed && Unbalanced && Rooted))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Il piano: waypoint lontani, un'azione pianificata e il profilo di corsa dichiarato.
	Planner->PlannedWaypoints.Add(FRTCellId(1, 0, 0));
	Planner->PlannedWaypoints.Add(FRTCellId(2, 0, 0));
	Planner->PlannedWaypoints.Add(FRTCellId(3, 0, 0));
	Planner->PlannedAbilityIndex = 0;
	Planner->PlannedDashAbility = 1;
	Planner->PlannedMovementProfileId = URTMovementProfileLibrary::ProfileSprint;

	const FRTFocusMoveSubject A = URTFocusAreaLibrary::SubjectFor(Quiet);
	const FRTFocusMoveSubject B = URTFocusAreaLibrary::SubjectFor(Planner);
	TestTrue(TEXT("stessa cella"), A.Cell == B.Cell);
	TestEqual(TEXT("stesso movimento base"), B.BaseMoveRange, A.BaseMoveRange);
	TestEqual(TEXT("stesso costo per cella"), B.MoveCostModifier, A.MoveCostModifier);
	TestEqual(TEXT("stessa corsa"), B.bRunDenied, A.bRunDenied);
	TestTrue(TEXT("premessa: il movimento base e' un numero vero"), A.BaseMoveRange > 0);

	URTHexMapAsset* Map = FocArena();
	const FRTTeamKnowledge Knowledge = FocAllVisible(Map);
	const FRTFocusMoveAreas InA = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, Knowledge, A);
	const FRTFocusMoveAreas InB = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Map, Knowledge, B);
	TestTrue(TEXT("due piani diversi, la stessa area normale"), InA.Normal == InB.Normal);
	TestTrue(TEXT("e la stessa area di corsa"), InA.Run == InB.Run);

	// Controllo positivo: gli stati PUBBLICI cambiano il soggetto, ciascuno per la sua parte.
	Slowed->ApplyStatus(TAG_Status_Slow, 2);
	Unbalanced->ApplyStatus(TAG_Status_Unbalanced, 2);
	Rooted->ApplyStatus(TAG_Status_Root, 2);
	TestEqual(TEXT("Slow: +1 al costo di ogni cella"), URTFocusAreaLibrary::SubjectFor(Slowed).MoveCostModifier, 1);
	TestTrue(TEXT("Unbalanced: la corsa e' negata"), URTFocusAreaLibrary::SubjectFor(Unbalanced).bRunDenied);
	TestEqual(TEXT("Root: nessun movimento"), URTFocusAreaLibrary::SubjectFor(Rooted).BaseMoveRange, 0);

	// Nessuna unita': il soggetto di default non muove.
	const FRTFocusMoveSubject None = URTFocusAreaLibrary::SubjectFor(nullptr);
	TestEqual(TEXT("nessuna unita': movimento base 0"), None.BaseMoveRange, 0);
	TestEqual(TEXT("e nessuna area su una mappa nulla"), URTFocusAreaLibrary::MoveAreas(nullptr, A).Normal.Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// 🔴 UN ARCO VERSO UNA CELLA NON IN VISTA NON PORTA NELL'AREA. Tutti gli altri test girano su un piano piatto, senza
// transizioni: se il pathfinder smettesse di saltare l'arrivo di un arco che blocca il movimento, nessuno se ne
// accorgerebbe, e un ponte o una scala farebbero entrare nell'area il piano di sopra, cioe' geometria mai vista.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusAreaHiddenArcTargetTest,
	"RefactorTactics.FocusArea.AnArcToAHiddenCellNeverEntersTheArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusAreaHiddenArcTargetTest::RunTest(const FString&)
{
	// Un piano sopra: due celle al layer 1, collegate da una scala alla cella di partenza.
	auto BuildWithUpperFloor = [](int32 UpperMoveCost)
	{
		URTHexMapAsset* Map = FocArena();
		FRTHexCellData First; First.Id = FRTCellId(0, 0, 1); First.MoveCost = UpperMoveCost;
		FRTHexCellData Second; Second.Id = FRTCellId(1, 0, 1); Second.MoveCost = UpperMoveCost;
		Map->AddOrUpdateCell(First);
		Map->AddOrUpdateCell(Second);
		Map->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), /*Cost=*/ 1, ERTHexTransitionKind::Stair, /*bBidirectional=*/ true);
		return Map;
	};
	URTHexMapAsset* Plain = BuildWithUpperFloor(1);
	URTHexMapAsset* Costly = BuildWithUpperFloor(9); // stesso piano di sopra, ma con un'altra geometria nascosta
	if (!TestTrue(TEXT("le mappe esistono"), Plain && Costly && Plain->ContainsCell(FRTCellId(0, 0, 1)))) { return false; }

	const auto OnlyGround = [](const FRTCellId& Cell) { return Cell.Layer == 0; };
	const FRTTeamKnowledge UpperHidden = FocKnowledge(Plain, OnlyGround);
	const FRTTeamKnowledge AllSeen = FocAllVisible(Plain);
	const FRTFocusMoveSubject Subject = FocSubject(3);

	// Controllo positivo: se il piano di sopra si vede, la scala porta fin li' (1 per la scala, 1 per la cella accanto).
	const FRTFocusMoveAreas Seen = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Plain, AllSeen, Subject);
	TestTrue(TEXT("piano di sopra in vista: la scala porta alla prima cella"), Seen.Normal.Contains(FRTCellId(0, 0, 1)));
	TestTrue(TEXT("e alla seconda"), Seen.Normal.Contains(FRTCellId(1, 0, 1)));

	// Il piano di sopra NON si vede: nessuna cella del layer 1 entra nell'area, per nessuna delle due bande.
	const FRTFocusMoveAreas Hidden = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Plain, UpperHidden, Subject);
	int32 UpperCells = 0;
	for (const FRTCellId& Cell : Hidden.Normal) { UpperCells += Cell.Layer == 1 ? 1 : 0; }
	for (const FRTCellId& Cell : Hidden.Run) { UpperCells += Cell.Layer == 1 ? 1 : 0; }
	TestEqual(TEXT("piano di sopra non in vista: nessuna sua cella nell'area"), UpperCells, 0);
	TestTrue(TEXT("premessa: il piano di terra c'e' ancora"), Hidden.Normal.Num() > 1 && Hidden.Run.Num() > 1);

	// Gemelli: lo stesso piano di sopra con un'altra geometria (costo diverso) non cambia l'area.
	const FRTFocusMoveAreas HiddenCostly = URTFocusAreaLibrary::MoveAreasFor(GetTransientPackage(), Costly, UpperHidden, Subject);
	TestTrue(TEXT("un'altra geometria al piano di sopra non cambia l'area"), Hidden.Normal == HiddenCostly.Normal && Hidden.Run == HiddenCostly.Run);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
