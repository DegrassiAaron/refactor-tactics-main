// L'ARCO DEL NEMICO, calcolato solo sul terreno che la squadra conosce (#3762).
//
// La regola e' dell'autore (2026-10-14): l'arco non deve rivelare muri mai visti. Ogni test qui sotto ha il suo
// controllo sulla mappa VERA, che dice che cosa si sarebbe rivelato senza la regola: senza quel controllo un test
// verde non distinguerebbe «la regola tiene» da «nella scena non c'era niente da rivelare».

#include "Misc/AutomationTest.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTEnemyArcLibrary.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Map/RTHexMapActor.h"
#include "Perception/RTTeamKnowledge.h"
#include "Perception/RTVisibilityBorder.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"
#include "RTConsoleVariableGuardForTest.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Arena piatta con un muro alla vista in `Muro` (nomi distinti per file: unity build). */
	URTHexMapAsset* MakeArenaArco(int32 Raggio, const FRTCellId& Muro)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Raggio);
		FRTHexCellData Cella = Map->FindCell(Muro) ? *Map->FindCell(Muro) : FRTHexCellData(Muro);
		Cella.Id = Muro;
		Cella.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Cella);
		Map->SortCells();
		return Map;
	}

	TArray<FRTCellId> TutteTranne(const URTHexMapAsset* Map, const TArray<FRTCellId>& Escluse)
	{
		TArray<FRTCellId> Out;
		for (const FRTHexCellData& C : Map->Cells)
		{
			if (!Escluse.Contains(C.Id))
			{
				Out.Add(C.Id);
			}
		}
		return Out;
	}

	FRTPerceiver NemicoArco(const FRTCellId& Cella, ERTHexDirection Verso, int32 Portata)
	{
		FRTPerceiver P;
		P.Cell = Cella;
		P.Facing = Verso;
		P.VisionRange = Portata;
		return P;
	}
}

/**
 * 🔴 **Il caso che la regola esiste per chiudere**: un muro che la squadra non ha mai visto, fra il nemico e celle
 * che la squadra conosce. Sulla mappa vera l'arco avrebbe un buco dietro il muro, e il buco lo rivelerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcUnseenWallTest,
	"RefactorTactics.EnemyArc.UnseenWallDoesNotCarveTheArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcUnseenWallTest::RunTest(const FString&)
{
	const FRTCellId Nemico(0, 0, 0);
	const FRTCellId Muro(2, 0, 0);
	const FRTCellId Dietro(3, 0, 0);
	URTHexMapAsset* Map = MakeArenaArco(5, Muro);
	const FRTPerceiver P = NemicoArco(Nemico, ERTHexDirection::E, 5);

	// Controllo: sulla mappa vera il muro toglie la cella dietro di lui.
	if (!TestFalse(TEXT("controllo: sulla mappa vera la cella dietro il muro non si vede"),
		URTPerceptionLibrary::VisibleCells(Map, P).Contains(Dietro)))
	{
		return false;
	}

	// La squadra conosce tutto tranne la cella del muro.
	const TArray<FRTCellId> Note = TutteTranne(Map, { Muro });
	URTHexMapAsset* Nota = URTEnemyArcLibrary::KnownTerrainMap(GetTransientPackage(), Map, Note);
	if (!TestNotNull(TEXT("la copia della mappa"), Nota)) { return false; }

	const TArray<FRTCellId> Arco = URTEnemyArcLibrary::SightOnKnownTerrain(Nota, P, Note);
	TestTrue(TEXT("sul terreno noto la cella dietro il muro mai visto resta nell'arco"), Arco.Contains(Dietro));
	TestFalse(TEXT("e la cella del muro, mai vista, non c'e'"), Arco.Contains(Muro));

	// La mappa vera non si tocca: la copia e' un altro oggetto.
	TestTrue(TEXT("la mappa vera ha ancora il suo muro"), Map->FindCell(Muro) && Map->FindCell(Muro)->bBlocksLineOfSight);
	TestTrue(TEXT("la copia e' un oggetto diverso"), Nota != Map);
	return true;
}

/**
 * Un muro che la squadra HA visto ferma l'arco come sempre: la regola toglie cio' che non si sa, non cio' che si sa.
 * E con tutto il terreno noto l'arco e' esattamente quello di `VisibleCells`, la percezione e non una seconda geometria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcKnownWallTest,
	"RefactorTactics.EnemyArc.KnownTerrainIsThePerceptionArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcKnownWallTest::RunTest(const FString&)
{
	const FRTCellId Nemico(0, 0, 0);
	const FRTCellId Muro(2, 0, 0);
	URTHexMapAsset* Map = MakeArenaArco(5, Muro);
	const FRTPerceiver P = NemicoArco(Nemico, ERTHexDirection::E, 5);

	const TArray<FRTCellId> Tutte = TutteTranne(Map, {});
	URTHexMapAsset* Nota = URTEnemyArcLibrary::KnownTerrainMap(GetTransientPackage(), Map, Tutte);
	const TArray<FRTCellId> Arco = URTEnemyArcLibrary::SightOnKnownTerrain(Nota, P, Tutte);
	TestTrue(TEXT("con tutto il terreno noto, l'arco e' quello della percezione"), Arco == URTPerceptionLibrary::VisibleCells(Map, P));
	TestFalse(TEXT("e il muro noto ferma l'arco"), Arco.Contains(FRTCellId(3, 0, 0)));
	return true;
}

/** L'arco non esce mai dal terreno noto: nessuna cella mai vista, tranne quella del nemico, che si vede. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcStaysOnKnownCellsTest,
	"RefactorTactics.EnemyArc.NeverShowsAnUnseenCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcStaysOnKnownCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 5);
	const FRTCellId Nemico(1, 0, 0);
	const FRTPerceiver P = NemicoArco(Nemico, ERTHexDirection::E, 5);

	// La squadra conosce soltanto le celle entro 2 dall'origine; il nemico sta al margine.
	TArray<FRTCellId> Note;
	for (const FRTHexCellData& C : Map->Cells)
	{
		if (URTHexLibrary::HexDistance(FRTCellId(-1, 0, 0), C.Id) <= 2)
		{
			Note.Add(C.Id);
		}
	}
	Note.Remove(Nemico); // il caso limite: anche la cella del nemico, se non fosse nota, resta

	URTHexMapAsset* Nota = URTEnemyArcLibrary::KnownTerrainMap(GetTransientPackage(), Map, Note);
	const TArray<FRTCellId> Arco = URTEnemyArcLibrary::SightOnKnownTerrain(Nota, P, Note);

	// Controllo: sulla mappa intera il nemico vede ben oltre il terreno noto.
	int32 FuoriDalNoto = 0;
	for (const FRTCellId& C : URTPerceptionLibrary::VisibleCells(Map, P))
	{
		FuoriDalNoto += Note.Contains(C) ? 0 : 1;
	}
	TestTrue(TEXT("controllo: senza il filtro l'arco uscirebbe dal terreno noto"), FuoriDalNoto > 1);

	bool bTuttoNoto = true;
	for (const FRTCellId& C : Arco)
	{
		bTuttoNoto &= (C == Nemico) || Note.Contains(C);
	}
	TestTrue(TEXT("ogni cella dell'arco e' nota, o e' quella del nemico"), bTuttoNoto);
	TestTrue(TEXT("la cella del nemico c'e'"), Arco.Contains(Nemico));
	return true;
}

/**
 * Coperture e porte: si tolgono dai lati fra due celle mai viste, restano su un lato che confina con una cella nota
 * — quel lato il velo lo disegna gia' ([D-502] (d)).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcEdgesTest,
	"RefactorTactics.EnemyArc.EdgesFacingKnownCellsStay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcEdgesTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	const FRTCellId Ignota(2, 0, 0);
	FRTHexCellData Cella = *Map->FindCell(Ignota);
	FRTHexCover VersoIgnota;
	VersoIgnota.Edge = ERTHexDirection::E; // verso (3,0): mai vista anche lei
	FRTHexCover VersoNota;
	VersoNota.Edge = ERTHexDirection::W;   // verso (1,0): nota
	Cella.Covers = { VersoIgnota, VersoNota };
	Cella.Doors = { FRTHexDoor(ERTHexDirection::NE), FRTHexDoor(ERTHexDirection::NW) };
	Map->AddOrUpdateCell(Cella);

	TArray<FRTCellId> Note = TutteTranne(Map, { Ignota, URTHexLibrary::Neighbor(Ignota, ERTHexDirection::E),
		URTHexLibrary::Neighbor(Ignota, ERTHexDirection::NE) });
	if (!TestTrue(TEXT("premessa: il vicino a ovest e' noto, quello a nord-ovest anche"),
		Note.Contains(URTHexLibrary::Neighbor(Ignota, ERTHexDirection::W))
		&& Note.Contains(URTHexLibrary::Neighbor(Ignota, ERTHexDirection::NW))))
	{
		return false;
	}

	URTHexMapAsset* Nota = URTEnemyArcLibrary::KnownTerrainMap(GetTransientPackage(), Map, Note);
	const FRTHexCellData* Copiata = Nota ? Nota->FindCell(Ignota) : nullptr;
	if (!TestNotNull(TEXT("la cella nella copia"), Copiata)) { return false; }
	TestEqual(TEXT("resta una copertura sola"), Copiata->Covers.Num(), 1);
	TestTrue(TEXT("ed e' quella sul lato verso la cella nota"),
		Copiata->Covers.Num() == 1 && Copiata->Covers[0].Edge == ERTHexDirection::W);
	TestEqual(TEXT("resta una porta sola"), Copiata->Doors.Num(), 1);
	TestTrue(TEXT("ed e' quella verso la cella nota"),
		Copiata->Doors.Num() == 1 && Copiata->Doors[0].Edge == ERTHexDirection::NW);
	TestEqual(TEXT("la mappa vera tiene le sue due coperture"), Map->FindCell(Ignota)->Covers.Num(), 2);
	return true;
}

/**
 * 🔑 **Solo i nemici IN VISTA hanno un arco**, e ogni arco e' quello della percezione sul terreno noto, nel colore del
 * suo nemico. Un alleato non ne ha; un nemico su una cella che la squadra non vede adesso nemmeno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcOnlyInViewTest,
	"RefactorTactics.EnemyArc.OnlyEnemiesInViewHaveAnArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcOnlyInViewTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 5);
	FRTTeamKnowledge Viewer;
	Viewer.TeamId = 0;
	for (const FRTHexCellData& C : Map->Cells)
	{
		if (URTHexLibrary::HexDistance(FRTCellId(0, 0, 0), C.Id) <= 3)
		{
			Viewer.ExploredCells.Add(C.Id);
			if (URTHexLibrary::HexDistance(FRTCellId(0, 0, 0), C.Id) <= 2)
			{
				Viewer.VisibleCells.Add(C.Id);
			}
		}
	}

	FRTEnemyForArc Alleato;
	Alleato.Cell = FRTCellId(0, 0, 0);
	Alleato.TeamId = 0;
	FRTEnemyForArc InVista;
	InVista.Cell = FRTCellId(2, 0, 0);
	InVista.Facing = ERTHexDirection::W;
	InVista.TeamId = 1;
	InVista.Color = FColor(255, 51, 38);
	FRTEnemyForArc FuoriVista;
	FuoriVista.Cell = FRTCellId(-4, 0, 0);
	FuoriVista.TeamId = 1;
	if (!TestFalse(TEXT("premessa: il nemico fuori vista sta su una cella che la squadra non vede"),
		Viewer.VisibleCells.Contains(FuoriVista.Cell)))
	{
		return false;
	}

	const TMap<FRTCellId, FRTEnemyArcView> Archi =
		URTEnemyArcLibrary::ArcsOfEnemiesInView(Map, Viewer, { Alleato, InVista, FuoriVista });
	TestEqual(TEXT("un solo arco"), Archi.Num(), 1);
	const FRTEnemyArcView* Arco = Archi.Find(InVista.Cell);
	if (!TestNotNull(TEXT("ed e' quello del nemico in vista, chiave la sua cella"), Arco))
	{
		return false;
	}
	TestEqual(TEXT("nel colore del suo nemico"), Arco->Color, InVista.Color);

	// L'arco e' la percezione sul terreno noto, chiusa dai lati verso celle della mappa.
	TArray<FRTCellId> Note = Viewer.ExploredCells;
	FRTPerceiver P;
	P.Cell = InVista.Cell;
	P.Facing = InVista.Facing;
	P.VisionRange = InVista.VisionRange;
	const TArray<FRTExposedEdge> Atteso = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(Map,
		URTEnemyArcLibrary::SightOnKnownTerrain(URTEnemyArcLibrary::KnownTerrainMap(GetTransientPackage(), Map, Note), P, Note));
	TestTrue(TEXT("i lati sono quelli della percezione sul terreno noto"), Arco->Edges == Atteso);
	TestTrue(TEXT("e non sono vuoti"), Arco->Edges.Num() > 0);
	return true;
}

/**
 * La mappa disegna UN arco, quello che il giocatore chiede ([D-521], seduta `U89`): nessuno di base, quello del nemico
 * sotto il mouse, altrimenti quello del nemico ispezionato, e il mouse vince sull'ispezione. In primo piano, davanti
 * alla geometria, e col debug spento come in Shipping. Con l'arco si disegnano i sei lati della cella del suo nemico,
 * nello stesso rosso: dove stanno lo misura `EnemyArc.OwnerCellIsOutlinedInTheArcColor`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcFocusTest,
	"RefactorTactics.EnemyArc.FocusChoosesWhichArcsAreDrawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcFocusTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ULineBatchComponent* Mondo = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	ULineBatchComponent* PrimoPiano = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: mappa, batcher e CVar del debug"), HexMap && Mondo && PrimoPiano && Debug))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	const FRTCellId A(2, 0, 0);
	const FRTCellId B(-2, 0, 0);
	const FRTCellId Vuota(0, 1, 0);
	FRTEnemyArcView ArcoA;
	ArcoA.Edges = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, { A });
	ArcoA.Color = FColor(255, 51, 38);
	FRTEnemyArcView ArcoB;
	ArcoB.Edges = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, { B, URTHexLibrary::Neighbor(B, ERTHexDirection::W) });
	ArcoB.Color = FColor(255, 51, 38);
	TMap<FRTCellId, FRTEnemyArcView> Archi;
	Archi.Add(A, ArcoA);
	Archi.Add(B, ArcoB);
	HexMap->SetEnemyArcs(Archi);
	// Senza lati diversi, «l'arco di A» e «l'arco di B» darebbero lo stesso conteggio e il test non li distinguerebbe.
	if (!TestTrue(TEXT("premessa: i due archi hanno un numero di lati diverso"), ArcoA.Edges.Num() != ArcoB.Edges.Num()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FLinearColor Rosso(ArcoA.Color);
	constexpr int32 LatiDelNemico = 6; // il contorno della cella del nemico che possiede l'arco
	const auto ContaRosse = [&](const ULineBatchComponent* Batcher)
	{
		int32 N = 0;
		for (const FBatchedLine& L : Batcher->BatchedLines)
		{
			N += L.Color.Equals(Rosso) ? 1 : 0;
		}
		return N;
	};
	// Le linee rosse di un fotogramma: quelle in primo piano, e in `NelMondo` quelle nel batcher del mondo, dove la
	// geometria le coprirebbe.
	int32 NelMondo = 0;
	const auto LineeRosse = [&]()
	{
		Mondo->Flush();
		PrimoPiano->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		NelMondo = ContaRosse(Mondo);
		return ContaRosse(PrimoPiano);
	};

	HexMap->SetEnemyArcFocus(FRTCellId(), /*bHasHover=*/ false, FRTCellId(), /*bHasInspected=*/ false);
	TestEqual(TEXT("senza mouse ne' ispezione non si disegna nessun arco"), HexMap->GetShownEnemyArcs().Num(), 0);
	TestEqual(TEXT("e nessuna linea"), LineeRosse(), 0);

	HexMap->SetEnemyArcFocus(A, true, FRTCellId(), false);
	TestEqual(TEXT("col mouse sul nemico A, il suo arco"), HexMap->GetShownEnemyArcs().Num(), 1);
	TestEqual(TEXT("e solo le sue linee, in primo piano"), LineeRosse(), ArcoA.Edges.Num() + LatiDelNemico);
	TestEqual(TEXT("e nessuna nel batcher del mondo"), NelMondo, 0);

	HexMap->SetEnemyArcFocus(Vuota, true, FRTCellId(), false);
	TestEqual(TEXT("col mouse su una cella senza nemico, niente"), HexMap->GetShownEnemyArcs().Num(), 0);

	HexMap->SetEnemyArcFocus(Vuota, true, B, true);
	TestEqual(TEXT("col mouse su una cella vuota e B ispezionato, l'arco di B"), LineeRosse(), ArcoB.Edges.Num() + LatiDelNemico);

	HexMap->SetEnemyArcFocus(FRTCellId(), false, B, true);
	TestEqual(TEXT("senza mouse e B ispezionato, l'arco di B"), LineeRosse(), ArcoB.Edges.Num() + LatiDelNemico);

	HexMap->SetEnemyArcFocus(A, true, B, true);
	TestEqual(TEXT("col mouse su A e B ispezionato, un arco solo"), HexMap->GetShownEnemyArcs().Num(), 1);
	TestEqual(TEXT("ed e' quello di A: il mouse vince"), LineeRosse(), ArcoA.Edges.Num() + LatiDelNemico);

	HexMap->SetEnemyArcFocus(FRTCellId(), false, Vuota, true);
	TestEqual(TEXT("un'unita' ispezionata senza arco non ne mostra"), HexMap->GetShownEnemyArcs().Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * L'arco dice DI CHI e': la cella del nemico che lo possiede ha i suoi sei lati nello stesso rosso, in primo piano
 * ([D-521] (6)). Nella seconda tornata di `U89` l'autore leggeva l'area ma non il nemico, e ha scelto «Nemico
 * evidenziato».
 *
 * 🔑 L'arco del test e' lontano dal suo nemico, quindi i lati trovati sulla cella del nemico non possono essere suoi.
 * E si guarda anche il nemico ISPEZIONATO col mouse su una cella vuota: e' il ramo in cui la cella del padrone non e'
 * quella sotto il cursore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcOwnerOutlineTest,
	"RefactorTactics.EnemyArc.OwnerCellIsOutlinedInTheArcColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcOwnerOutlineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ULineBatchComponent* PrimoPiano = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	if (!TestTrue(TEXT("premessa: mappa e batcher"), HexMap && PrimoPiano))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	const FRTCellId A(2, 0, 0);
	const FRTCellId B(0, 2, 0);
	const FRTCellId Vuota(0, 0, 0);
	FRTEnemyArcView Arco;
	Arco.Edges = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, { FRTCellId(-3, 0, 0) });
	Arco.Color = FColor(255, 51, 38);
	TMap<FRTCellId, FRTEnemyArcView> Archi;
	Archi.Add(A, Arco);
	Archi.Add(B, Arco);
	HexMap->SetEnemyArcs(Archi);

	FVector Origine;
	float Lato = 0.f;
	float Piano = 0.f;
	HexMap->GetHexContext(Origine, Lato, Piano);
	const FLinearColor Rosso(Arco.Color);
	// Quanti dei sei lati di `Cella` hanno una linea rossa in primo piano, misurando il punto medio sul piano.
	const auto LatiRossiDi = [&](const FRTCellId& Cella)
	{
		PrimoPiano->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		int32 N = 0;
		for (int32 D = 0; D < 6; ++D)
		{
			const FVector Medio = URTHexLibrary::EdgeMidpointWorld(Cella, static_cast<ERTHexDirection>(D), Origine, Lato, Piano);
			for (const FBatchedLine& L : PrimoPiano->BatchedLines)
			{
				const FVector M = (L.Start + L.End) * 0.5f;
				if (L.Color.Equals(Rosso) && FVector2D(M.X - Medio.X, M.Y - Medio.Y).Size() < 1.f)
				{
					++N;
					break;
				}
			}
		}
		return N;
	};

	HexMap->SetEnemyArcFocus(A, /*bHasHover=*/ true, FRTCellId(), /*bHasInspected=*/ false);
	TestEqual(TEXT("col mouse sul nemico A, i sei lati della sua cella sono rossi"), LatiRossiDi(A), 6);
	TestEqual(TEXT("e quelli di B no"), LatiRossiDi(B), 0);

	HexMap->SetEnemyArcFocus(Vuota, true, B, true);
	TestEqual(TEXT("col mouse su una cella vuota e B ispezionato, i sei lati di B"), LatiRossiDi(B), 6);
	TestEqual(TEXT("e nessuno della cella sotto il cursore"), LatiRossiDi(Vuota), 0);

	HexMap->SetEnemyArcFocus(FRTCellId(), false, FRTCellId(), false);
	TestEqual(TEXT("senza arco, nessun contorno"), LatiRossiDi(A) + LatiRossiDi(B), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il controller passa alla mappa la cella del nemico ISPEZIONATO (#3762): senza mouse sulla mappa, l'arco che si vede
 * e' il suo. Il click che ispeziona ha i suoi test nella matrice del puntatore; qui l'ispezione si imposta diretta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcFollowsTheInspectedEnemyTest,
	"RefactorTactics.EnemyArc.ControllerShowsTheInspectedEnemyArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcFollowsTheInspectedEnemyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTUnit* Nemico = World->SpawnActor<ARTUnit>();
	if (!TestTrue(TEXT("premessa: mappa, controller e unita'"), HexMap && PC && Nemico))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	Nemico->Cell = FRTCellId(2, 0, 0);

	FRTEnemyArcView Arco;
	Arco.Edges = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, { Nemico->Cell });
	Arco.Color = FColor(255, 51, 38);
	TMap<FRTCellId, FRTEnemyArcView> Archi;
	Archi.Add(Nemico->Cell, Arco);
	HexMap->SetEnemyArcs(Archi);
	HexMap->SetHoveredCell(FRTCellId(), false);

	PC->UpdateEnemyArcFocusForTest(HexMap);
	TestEqual(TEXT("senza mouse ne' ispezione, nessun arco"), HexMap->GetShownEnemyArcs().Num(), 0);

	PC->SetInspectedUnitForTest(Nemico);
	PC->UpdateEnemyArcFocusForTest(HexMap);
	const TArray<FRTEnemyArcView> Mostrati = HexMap->GetShownEnemyArcs();
	TestEqual(TEXT("col nemico ispezionato, un arco"), Mostrati.Num(), 1);
	TestTrue(TEXT("ed e' il suo"), Mostrati.Num() == 1 && Mostrati[0].Edges == Arco.Edges);

	PC->SetInspectedUnitForTest(nullptr);
	PC->UpdateEnemyArcFocusForTest(HexMap);
	TestEqual(TEXT("chiusa l'ispezione, l'arco sparisce"), HexMap->GetShownEnemyArcs().Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Nessun tasto mostra gli archi di tutti i nemici in vista (#3762, seduta `U89`): l'autore ha chiesto che se ne veda
 * uno solo, e il tasto `H` che li accendeva tutti e' stato tolto con la sua azione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyArcNoKeyShowsThemAllTest,
	"RefactorTactics.PlayerInput.NoKeyShowsEveryEnemyArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyArcNoKeyShowsThemAllTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const UInputMappingContext* Imc = PC ? PC->BuildAndGetMappingContextForTest() : nullptr;
	if (TestNotNull(TEXT("mapping context costruito"), Imc))
	{
		// Senza mappature il conteggio qui sotto sarebbe zero per costruzione.
		TestTrue(TEXT("premessa: il contesto ha delle mappature"), Imc->GetMappings().Num() > 0);
		int32 Archi = 0;
		int32 SuH = 0;
		for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
		{
			Archi += (M.Action && M.Action->GetName() == TEXT("IA_ShowEnemyArcs")) ? 1 : 0;
			SuH += (M.Key == EKeys::H) ? 1 : 0;
		}
		TestEqual(TEXT("nessuna azione mostra gli archi di tutti"), Archi, 0);
		TestEqual(TEXT("e H non e' mappato"), SuH, 0);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
