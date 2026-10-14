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

#endif // WITH_DEV_AUTOMATION_TESTS
