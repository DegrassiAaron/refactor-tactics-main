#include "Misc/AutomationTest.h"

#include "RTDevToolset.h"

#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "ToolsetRegistry/ToolCallExceptionHandler.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mappa esagonale piena di raggio N su layer 0, tutte le celle a costo 1 (stesso stampo di RTHexPathTests). */
	URTHexMapAsset* MakeMap(int32 Radius)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Map->SortCells();
		return Map;
	}

	/**
	 * Esegue `Func` dentro uno stack frame in cui `RaiseScriptError` e' osservabile, e restituisce l'errore
	 * sollevato (vuoto se nessuno).
	 *
	 * ⚠️ Senza questo, chiamare un tool da C++ e verificare l'errore misurerebbe il silenzio: fuori da un
	 * frame Blueprint `RaiseScriptError` non fa nulla — lo dice il docstring di `FToolCallExceptionHandler` —
	 * e un test scritto senza saperlo passerebbe qualunque cosa faccia il tool.
	 */
	FString ErrorRaisedBy(TFunction<void()>&& Func)
	{
		UE::ToolsetRegistry::FToolCallExceptionHandler Handler;
		Handler.CaptureErrorsIn(MoveTemp(Func));
		return Handler.GetException();
	}

	/** Confronto esatto di due sequenze di celle: stessa lunghezza e stesso ordine. */
	bool SamePath(const TArray<FRTCellId>& A, const TArray<FRTCellId>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 I = 0; I < A.Num(); ++I)
		{
			if (!(A[I] == B[I])) { return false; }
		}
		return true;
	}
}

// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetCellLookupTest,
	"RefactorTactics.DevToolset.CellLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetCellLookupTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(3);

	// Una cella con valori DISTINTI campo per campo: se il report leggesse la sorgente sbagliata — il costo
	// al posto del sovrapprezzo, o viceversa — con numeri uguali non si vedrebbe.
	FRTHexCellData Authored(FRTCellId(1, 0, 0));
	Authored.Surface = ERTHexSurface::Ice;
	Authored.MoveCost = 2;
	Authored.OccupancySurcharge = 3;
	Authored.Height = 7;
	Authored.bBlocksLineOfSight = true;
	Authored.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low));
	// ⚠️ La porta sta sul bordo E, che guarda LONTANO dal centro. Una porta `Closed` e' SOTTRATTIVA:
	// nega l'adiacenza del PROPRIO bordo. Su W — il bordo verso (0,0,0) — toglierebbe questa cella dai
	// vicini del centro, ed e' esattamente cio' che il blocco in fondo al test verifica.
	Authored.Doors.Add(FRTHexDoor(ERTHexDirection::E, ERTHexDoorState::Closed));
	Map->AddOrUpdateCell(Authored);
	Map->SortCells();

	const FRTDevCellReport Report = URTDevToolset::DumpCellOnMap(Map, FRTCellId(1, 0, 0));

	TestTrue(TEXT("la cella esiste"), Report.bExists);
	TestTrue(TEXT("coordinata riportata invariata"), Report.Cell == FRTCellId(1, 0, 0));
	TestEqual(TEXT("superficie tradotta in nome d'enum"), Report.Surface, FString(TEXT("Ice")));
	TestEqual(TEXT("costo base"), Report.MoveCost, 2);
	TestEqual(TEXT("sovrapprezzo di geometria"), Report.OccupancySurcharge, 3);
	TestEqual(TEXT("costo totale = base + sovrapprezzo"), Report.TotalMoveCost, 5);
	TestEqual(TEXT("quota"), Report.Height, 7);
	TestTrue(TEXT("blocca la vista"), Report.bBlocksLineOfSight);
	TestFalse(TEXT("non blocca il passo"), Report.bBlocksMovement);
	TestEqual(TEXT("una copertura"), Report.NumCovers, 1);
	TestEqual(TEXT("una porta"), Report.NumDoors, 1);
	TestEqual(TEXT("revisione del grafo"), Report.GraphRevision, Map->Revision);

	// I sei vicini planari di una cella interna, con il costo di ENTRATA nella destinazione: la cella a
	// costo totale 5 costruita sopra deve comparire con 5 fra i vicini del centro, non con 1.
	const FRTDevCellReport Center = URTDevToolset::DumpCellOnMap(Map, FRTCellId(0, 0, 0));
	TestEqual(TEXT("sei vicini planari al centro"), Center.Neighbors.Num(), 6);

	const FRTDevNeighbor* Expensive = Center.Neighbors.FindByPredicate(
		[](const FRTDevNeighbor& N) { return N.Cell == FRTCellId(1, 0, 0); });
	TestNotNull(TEXT("il vicino (1,0,0) e' fra i raggiungibili"), Expensive);
	if (Expensive)
	{
		TestEqual(TEXT("costo dell'arco = costo totale della destinazione"), Expensive->Cost, 5);
	}

	// La regola delle porte, verificata invece che assunta: la STESSA porta spostata sul bordo W — quello
	// che guarda il centro — toglie la cella dai vicini raggiungibili senza rimuoverla dalla mappa. E' la
	// distinzione fra «non esiste» e «non ci si passa», e un report che le confondesse mentirebbe due volte.
	FRTHexCellData Sealed = Authored;
	Sealed.Doors.Empty();
	Sealed.Doors.Add(FRTHexDoor(ERTHexDirection::W, ERTHexDoorState::Closed));
	Map->AddOrUpdateCell(Sealed);
	Map->SortCells();

	const FRTDevCellReport AfterSeal = URTDevToolset::DumpCellOnMap(Map, FRTCellId(0, 0, 0));
	TestEqual(TEXT("una porta chiusa verso il centro toglie un vicino"), AfterSeal.Neighbors.Num(), 5);
	TestTrue(TEXT("ma la cella e' ancora nella mappa"),
		URTDevToolset::DumpCellOnMap(Map, FRTCellId(1, 0, 0)).bExists);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetMissingCellTest,
	"RefactorTactics.DevToolset.MissingCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetMissingCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(2);

	FRTDevCellReport Report;
	const FString Error = ErrorRaisedBy([&Report, Map]()
	{
		Report = URTDevToolset::DumpCellOnMap(Map, FRTCellId(99, 99, 0));
	});

	TestFalse(TEXT("la cella non esiste"), Report.bExists);
	TestFalse(TEXT("un errore e' stato sollevato"), Error.IsEmpty());
	TestTrue(TEXT("l'errore nomina la cella mancante"), Error.Contains(TEXT("does not exist")));

	// La coordinata chiesta torna comunque indietro: chi legge la risposta deve poter capire DI COSA si
	// parla senza reggere lo stato della richiesta.
	TestTrue(TEXT("coordinata riportata anche in errore"), Report.Cell == FRTCellId(99, 99, 0));

	// ⚠️ Controprova del meccanismo di cattura: la stessa sonda su una cella VALIDA non deve produrre errori.
	// Senza di essa, un `CaptureErrorsIn` che catturasse sempre qualcosa renderebbe il test sopra vacuo.
	FRTDevCellReport Good;
	const FString NoError = ErrorRaisedBy([&Good, Map]()
	{
		Good = URTDevToolset::DumpCellOnMap(Map, FRTCellId(0, 0, 0));
	});
	TestTrue(TEXT("nessun errore su una cella valida"), NoError.IsEmpty());
	TestTrue(TEXT("e la cella valida esiste"), Good.bExists);

	// Mappa assente: il tool non deve rispondere con un report vuoto che sembra una misura.
	FRTDevCellReport NoMap;
	const FString MapError = ErrorRaisedBy([&NoMap]()
	{
		NoMap = URTDevToolset::DumpCellOnMap(nullptr, FRTCellId(0, 0, 0));
	});
	TestFalse(TEXT("mappa assente -> errore esplicito"), MapError.IsEmpty());
	TestFalse(TEXT("mappa assente -> nessuna cella"), NoMap.bExists);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetPathQueryTest,
	"RefactorTactics.DevToolset.PathQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetPathQueryTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(3);

	const FRTDevPathReport Report = URTDevToolset::FindPathOnMap(Map, FRTCellId(-2, 0, 0), FRTCellId(2, 0, 0));

	TestTrue(TEXT("successo"), Report.bSuccess);
	TestEqual(TEXT("stato riportato dal pathfinder"), Report.Status, FString(TEXT("Success")));
	TestEqual(TEXT("costo intero"), Report.TotalCost, 4);
	TestTrue(TEXT("nodi espansi riportati"), Report.NodesVisited > 0);
	TestEqual(TEXT("revisione del grafo"), Report.GraphRevision, Map->Revision);
	TestTrue(TEXT("durata del pathfinder misurata"), Report.PathfinderMilliseconds >= 0.0);

	// ⚠️ ORDINE ESATTO, non «arriva a destinazione»: su una retta assiale il percorso minimo e' unico, quindi
	// una sequenza diversa e' un difetto e non una scelta alternativa dell'A*.
	const TArray<FRTCellId> Expected = {
		FRTCellId(-2, 0, 0), FRTCellId(-1, 0, 0), FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0)
	};
	TestTrue(TEXT("sequenza esatta di celle"), SamePath(Report.Path, Expected));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetUnreachableGoalTest,
	"RefactorTactics.DevToolset.UnreachableGoal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetUnreachableGoalTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(2);

	// Una destinazione fuori mappa e' una RISPOSTA del pathfinder, non un guasto: deve tornare con il proprio
	// stato e senza sollevare errori di script.
	FRTDevPathReport Report;
	const FString Error = ErrorRaisedBy([&Report, Map]()
	{
		Report = URTDevToolset::FindPathOnMap(Map, FRTCellId(0, 0, 0), FRTCellId(99, 99, 0));
	});

	TestFalse(TEXT("nessun successo"), Report.bSuccess);
	TestEqual(TEXT("stato GoalInvalid"), Report.Status, FString(TEXT("GoalInvalid")));
	TestEqual(TEXT("percorso vuoto"), Report.Path.Num(), 0);
	TestTrue(TEXT("nessun errore sollevato: e' un esito, non un guasto"), Error.IsEmpty());

	const FRTDevPathReport BadStart =
		URTDevToolset::FindPathOnMap(Map, FRTCellId(99, 99, 0), FRTCellId(0, 0, 0));
	TestEqual(TEXT("stato StartInvalid"), BadStart.Status, FString(TEXT("StartInvalid")));

	// Layer diverso senza transizione esplicita: raggiungibile solo per archi, che qui non esistono.
	const FRTDevPathReport OtherLayer =
		URTDevToolset::FindPathOnMap(Map, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1));
	TestFalse(TEXT("un altro layer non e' adiacente"), OtherLayer.bSuccess);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetDeterminismTest,
	"RefactorTactics.DevToolset.PathDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetDeterminismTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(4);

	// Un ostacolo, perche' su una retta libera qualunque implementazione sembra deterministica: qui l'A' deve
	// SCEGLIERE da che parte aggirare, ed e' quella scelta che il tie-break stabile deve ripetere.
	FRTHexCellData Blocked(FRTCellId(0, 0, 0));
	Blocked.bBlocksMovement = true;
	Map->AddOrUpdateCell(Blocked);
	Map->SortCells();

	const FRTDevPathReport First = URTDevToolset::FindPathOnMap(Map, FRTCellId(-3, 0, 0), FRTCellId(3, 0, 0));
	TestTrue(TEXT("il percorso esiste"), First.bSuccess);
	TestTrue(TEXT("aggira l'ostacolo"), !First.Path.Contains(FRTCellId(0, 0, 0)));

	// ⚠️ Gli ESTREMI, e non e' pedanteria: il confronto run-contro-run non vede un errore SISTEMATICO.
	// Un percorso invertito e' identico a se stesso a ogni esecuzione e resta pure contiguo — misurato con una
	// mutazione: senza queste due righe, invertire `Report.Path` nel facade lasciava questo test VERDE.
	TestTrue(TEXT("parte dalla cella di partenza"),
		First.Path.Num() > 0 && First.Path[0] == FRTCellId(-3, 0, 0));
	TestTrue(TEXT("arriva alla destinazione"),
		First.Path.Num() > 0 && First.Path.Last() == FRTCellId(3, 0, 0));

	// Stesso input, stessa revisione del grafo => stessa sequenza ORDINATA e stesso costo.
	for (int32 Run = 1; Run <= 4; ++Run)
	{
		const FRTDevPathReport Again =
			URTDevToolset::FindPathOnMap(Map, FRTCellId(-3, 0, 0), FRTCellId(3, 0, 0));

		TestEqual(FString::Printf(TEXT("run %d: stessa revisione"), Run), Again.GraphRevision, First.GraphRevision);
		TestEqual(FString::Printf(TEXT("run %d: stesso costo"), Run), Again.TotalCost, First.TotalCost);
		TestEqual(FString::Printf(TEXT("run %d: stessi nodi espansi"), Run), Again.NodesVisited, First.NodesVisited);
		TestTrue(FString::Printf(TEXT("run %d: stessa sequenza esatta"), Run), SamePath(Again.Path, First.Path));
	}

	// Il percorso resta contiguo: ogni passo e' un vicino del precedente. Cattura un report che perdesse o
	// riordinasse celle senza cambiare ne' costo ne' estremi — cio' che il confronto run-contro-run da solo
	// non vedrebbe, perche' sarebbe sbagliato allo stesso modo tutte le volte.
	bool bContiguous = true;
	for (int32 I = 1; I < First.Path.Num(); ++I)
	{
		bContiguous &= (URTHexLibrary::HexDistance(First.Path[I - 1], First.Path[I]) == 1);
	}
	TestTrue(TEXT("percorso contiguo passo per passo"), bContiguous);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevToolsetMapSummaryTest,
	"RefactorTactics.DevToolset.MapSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevToolsetMapSummaryTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMap(2);
	const int32 Expected = Map->NumCells();

	const FRTDevMapReport Report = URTDevToolset::DescribeMap(Map, TEXT("L_Test"));

	TestTrue(TEXT("mappa caricata"), Report.bLoaded);
	TestEqual(TEXT("numero di celle"), Report.NumCells, Expected);
	TestEqual(TEXT("nome del livello riportato"), Report.LevelName, FString(TEXT("L_Test")));
	TestEqual(TEXT("nessuna transizione su una mappa piana"), Report.NumTransitions, 0);
	TestEqual(TEXT("un solo layer popolato"), Report.Layers.Num(), 1);
	TestEqual(TEXT("versione di formato dell'asset"), Report.FormatVersion, URTHexMapAsset::CurrentFormatVersion);
	TestTrue(TEXT("hash del contenuto"), Report.ContentHash == static_cast<int64>(Map->ComputeHash()));

	// La validazione passa dal validator del progetto: una mappa generata cosi' non ha problemi da segnalare.
	const FRTDevValidationReport Validation = URTDevToolset::ValidateMapAsset(Map, TArray<FRTCellId>());
	TestTrue(TEXT("validazione eseguita"), Validation.bLoaded);
	TestEqual(TEXT("nessun problema"), Validation.Issues.Num(), 0);
	TestTrue(TEXT("mappa valida"), Validation.bValid);

	// Mappa assente: `bLoaded` false e un errore esplicito, invece di un «valida» che nessuno ha verificato.
	FRTDevValidationReport Missing;
	const FString Error = ErrorRaisedBy([&Missing]()
	{
		Missing = URTDevToolset::ValidateMapAsset(nullptr, TArray<FRTCellId>());
	});
	TestFalse(TEXT("mappa assente -> errore"), Error.IsEmpty());
	TestFalse(TEXT("mappa assente -> non caricata"), Missing.bLoaded);
	TestFalse(TEXT("mappa assente -> non dichiarata valida"), Missing.bValid);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
