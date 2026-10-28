#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceRedirector.h" // GLog->AddOutputDevice: il log di fine pennellata si legge dalla riga che lo nomina
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include "Tools/RTHexPaintTool.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il CICLO del pennello in Erase, provato sul tool e non sulla sede ([#3916](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3916)).
 *
 * I test di #3322 (`RefactorTactics.MapEdit.Brush*`) chiamano `URTHexMapAsset::EraseCellInStroke` direttamente: provano
 * la regola, non che `URTHexPaintTool::ApplyBrushAt` la chiami. Un ritorno a `RemoveCell` nel solo tool, un contatore
 * scritto `=` invece di `+=`, o un log che stampa il numero sbagliato non facevano cadere niente.
 *
 * Qui si guida il tool a mano (`BeginStrokeForTest`, `ApplyBrushAtForTest`, `EndStrokeForTest`), senza viewport:
 * `ApplyBrushAt` seguito da `EndStrokeIfActive`. Non provano la risoluzione del raggio in una cella e nemmeno la
 * ricostruzione con maschera (`BrushRebuildMask`) che `OnClickPress` e `OnClickDrag` fanno dopo ogni cella: quella
 * resta fuori da questo file (la maschera ha il suo test, `BrushRebuildMask` e' statica). Fuori scopo restano anche
 * il dedup `PaintedThisStroke`, la transazione lazy e il valore di ritorno cumulativo di `ApplyBrushAt`.
 */
namespace
{
	/** Un mondo di gioco minimo, distrutto all'uscita da QUALUNQUE ramo. Nome prefissato: unity build. */
	struct FRTBrushWiringWorld
	{
		UWorld* World = nullptr;

		FRTBrushWiringWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
			if (World && GEngine)
			{
				FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
				Ctx.SetCurrentWorld(World);
			}
		}

		~FRTBrushWiringWorld()
		{
			if (World)
			{
				if (GEngine) { GEngine->DestroyWorldContext(World); }
				World->DestroyWorld(false);
			}
		}
	};

	/** Raccoglie le righe di log mentre vive: il log di fine pennellata e' l'unica uscita che il tool ha. */
	struct FRTBrushLogCapture : public FOutputDevice
	{
		TArray<FString> Lines;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			Lines.Add(V);
		}

		int32 CountContaining(const TCHAR* Needle) const
		{
			int32 Count = 0;
			for (const FString& L : Lines)
			{
				if (L.Contains(Needle))
				{
					++Count;
				}
			}
			return Count;
		}
	};

	/** La porta con nome pubblico sul bordo indicato. */
	void BrushWiringPutDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, int32 DoorId, FName StableId)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		FRTHexDoor Door(Edge, ERTHexDoorState::Closed, DoorId);
		Door.StableId = StableId;
		Data.Doors.Add(Door);
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Una corda per due vertici opposti, ruotata di `Sector` sesti di giro. */
	bool BrushWiringChord(int32 Sector, FRTGeometrySegment& Out)
	{
		const double Angle = (PI / 6.0) + (Sector * PI / 3.0);
		const FVector2D A(100.0 * FMath::Cos(Angle), 100.0 * FMath::Sin(Angle));
		if (!URTGeometryGrammarLibrary::SnapToGrammar(A, FVector2D(-A.X, -A.Y), 100.f, Out))
		{
			return false;
		}
		Out.WallType = ERTHexCoverType::High;
		return true;
	}

	/**
	 * Una regione VALIDA attorno a (0,0,0), pennello di raggio 1: muri su tre celle dell'area e uno fuori, un arco che
	 * esce dall'area e uno che entra, e un binding la cui sorgente cade dentro e il cui bersaglio fuori. E' la regione
	 * di `MapEdit.BrushAreaLeavesNoOrphans`, scritta qui perche' il test vive in un altro modulo.
	 */
	URTHexMapAsset* BrushWiringMakeRegion()
	{
		FRTGeometrySegment Chord0, Chord1, Chord2, OutsideChord;
		if (!BrushWiringChord(0, Chord0) || !BrushWiringChord(1, Chord1) || !BrushWiringChord(2, Chord2)
			|| !BrushWiringChord(0, OutsideChord))
		{
			return nullptr;
		}

		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
		Map->HexSize = 100.f;
		Map->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(0, 0, 0), Chord0));
		Map->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(1, 0, 0), Chord1));
		Map->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(-1, 1, 0), Chord2));
		Map->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(2, 0, 0), OutsideChord)); // fuori dall'area: deve restare
		Map->Transitions.Add(FRTHexEdge(FRTCellId(1, 0, 0), FRTCellId(3, 0, 0), 1));   // dall'area verso fuori
		Map->Transitions.Add(FRTHexEdge(FRTCellId(-3, 0, 0), FRTCellId(-1, 0, 0), 1)); // da fuori verso l'area
		BrushWiringPutDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, /*DoorId*/ 1, TEXT("S1"));
		BrushWiringPutDoor(Map, FRTCellId(2, 0, 0), ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
		Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
		return Map;
	}

	/** L'actor con l'asset gia' assegnato, cosi' `OnConstruction` lo vede come al caricamento del livello. */
	ARTHexMapActor* BrushWiringSpawnActor(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Actor = World->SpawnActorDeferred<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (Actor)
		{
			Actor->MapAsset = Asset;
			Actor->FinishSpawning(FTransform::Identity);
		}
		return Actor;
	}

	FString BrushWiringIssues(const URTHexMapAsset* Map)
	{
		const TArray<FString> Errors = Map->ValidateMap();
		return Errors.Num() > 0 ? FString::Join(Errors, TEXT(" | ")) : FString(TEXT("nessuna"));
	}
}

/**
 * **Il pennello in `Erase` porta via la cascata, e il log di fine pennellata dice quanto.**
 *
 * Una pennellata di raggio 1 attorno a un centro con geometria: sette celle, tre muri, due archi e un binding. La
 * controprova e' la premessa (l'allestimento porta davvero tutto e parte valido); poi si guida `ApplyBrushAt` e si
 * chiude la pennellata. Cadono: il tool che toglie la sola cella (restano orfani e il conteggio e' zero), un contatore
 * che non somma, un log che stampa un altro numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBrushWiringEraseTest,
	"RefactorTactics.Editor.Paint.BrushEraseWiresTheCascade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBrushWiringEraseTest::RunTest(const FString&)
{
	const FRTCellId Center(0, 0, 0);
	const int32 BrushRadius = 1;

	FRTBrushWiringWorld Mondo;
	if (!TestNotNull(TEXT("il mondo si crea"), Mondo.World))
	{
		return false;
	}
	URTHexMapAsset* Asset = BrushWiringMakeRegion();
	if (!TestNotNull(TEXT("la regione si costruisce"), Asset))
	{
		return false;
	}
	ARTHexMapActor* Actor = BrushWiringSpawnActor(Mondo.World, Asset);
	if (!TestNotNull(TEXT("l'actor si spawna"), Actor))
	{
		return false;
	}

	// --- Premessa: l'allestimento e' valido e porta tutto ----------------------------------------------
	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(Center, BrushRadius);
	TestTrue(TEXT("premessa: l'area copre piu' celle di un click singolo"), Area.Num() > 1);
	TestEqual(*FString::Printf(TEXT("premessa: la regione parte valida (segnalazioni: %s)"), *BrushWiringIssues(Asset)),
		Asset->ValidateMap().Num(), 0);
	TestEqual(TEXT("premessa: quattro muri"), Asset->InteriorWalls.Num(), 4);
	TestEqual(TEXT("premessa: due archi"), Asset->Transitions.Num(), 2);
	TestEqual(TEXT("premessa: un binding"), Asset->InteractionBindings.Num(), 1);
	const int32 CellsBefore = Asset->NumCells();

	FRTBrushLogCapture Log;
	GLog->AddOutputDevice(&Log);
	ON_SCOPE_EXIT { GLog->RemoveOutputDevice(&Log); };

	// --- La pennellata -----------------------------------------------------------------------------------
	URTHexPaintTool* Tool = NewObject<URTHexPaintTool>(GetTransientPackage());
	URTHexPaintToolProperties* Props = Tool->BeginStrokeForTest(Actor);
	if (!TestNotNull(TEXT("il tool ha le sue proprieta'"), Props))
	{
		return false;
	}
	Props->Operation = ERTHexPaintOp::Erase;
	Props->BrushRadius = BrushRadius;

	TestTrue(TEXT("la pennellata ha cambiato qualcosa"), Tool->ApplyBrushAtForTest(Center));
	Tool->EndStrokeForTest();
	GLog->Flush();

	TestEqual(TEXT("sono sparite le celle dell'area, e solo quelle"), Asset->NumCells(), CellsBefore - Area.Num());
	TestEqual(*FString::Printf(TEXT("nessun orfano (segnalazioni: %s)"), *BrushWiringIssues(Asset)),
		Asset->ValidateMap().Num(), 0);
	TestEqual(TEXT("resta il muro fuori dall'area"), Asset->InteriorWalls.Num(), 1);
	TestEqual(TEXT("gli archi sono andati"), Asset->Transitions.Num(), 0);
	TestEqual(TEXT("e il binding"), Asset->InteractionBindings.Num(), 0);

	// --- I contatori e il log --------------------------------------------------------------------------------
	TestEqual(TEXT("il tool ha contato le celle cancellate"), Tool->StrokeCellsErasedForTest(), Area.Num());
	TestEqual(TEXT("e somma la cascata di ogni cella: tre muri, due archi, un binding"),
		Tool->StrokeCascadeRemovedForTest(), 6);
	TestEqual(TEXT("il log di fine pennellata c'e', una volta, con i numeri giusti"),
		Log.CountContaining(*FString::Printf(
			TEXT("[HexMode] Erase: %d celle cancellate, 6 elementi portati via con esse"), Area.Num())), 1);

	// --- Una seconda pennellata sulla stessa area: i contatori ripartono da zero -----------------------------
	Tool->BeginStrokeForTest(Actor);
	TestFalse(TEXT("la seconda pennellata non trova niente da cancellare"), Tool->ApplyBrushAtForTest(Center));
	Tool->EndStrokeForTest();
	GLog->Flush();
	TestEqual(TEXT("i contatori della pennellata precedente non sopravvivono"), Tool->StrokeCellsErasedForTest(), 0);
	TestEqual(TEXT("nemmeno quello della cascata"), Tool->StrokeCascadeRemovedForTest(), 0);
	TestEqual(TEXT("e non c'e' un secondo log"), Log.CountContaining(TEXT("[HexMode] Erase:")), 1);
	return true;
}

/**
 * **Il pennello in `Paint` non incrementa i contatori dell'erase, e non lascia un log di erase.**
 *
 * Il gemello negativo: la stessa regione, la stessa area, ma si dipinge. Un contatore incrementato fuori dal ramo
 * dell'erase, o un log scritto anche per il paint, cadrebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBrushWiringPaintTest,
	"RefactorTactics.Editor.Paint.BrushPaintDoesNotCountAsErase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBrushWiringPaintTest::RunTest(const FString&)
{
	const FRTCellId Center(0, 0, 0);

	FRTBrushWiringWorld Mondo;
	if (!TestNotNull(TEXT("il mondo si crea"), Mondo.World))
	{
		return false;
	}
	URTHexMapAsset* Asset = BrushWiringMakeRegion();
	if (!TestNotNull(TEXT("la regione si costruisce"), Asset))
	{
		return false;
	}
	ARTHexMapActor* Actor = BrushWiringSpawnActor(Mondo.World, Asset);
	if (!TestNotNull(TEXT("l'actor si spawna"), Actor))
	{
		return false;
	}
	const int32 CellsBefore = Asset->NumCells();

	FRTBrushLogCapture Log;
	GLog->AddOutputDevice(&Log);
	ON_SCOPE_EXIT { GLog->RemoveOutputDevice(&Log); };

	URTHexPaintTool* Tool = NewObject<URTHexPaintTool>(GetTransientPackage());
	URTHexPaintToolProperties* Props = Tool->BeginStrokeForTest(Actor);
	if (!TestNotNull(TEXT("il tool ha le sue proprieta'"), Props))
	{
		return false;
	}
	Props->Operation = ERTHexPaintOp::Paint;
	Props->BrushRadius = 1;

	TestTrue(TEXT("dipingere cambia sempre qualcosa"), Tool->ApplyBrushAtForTest(Center));
	Tool->EndStrokeForTest();
	GLog->Flush();

	TestEqual(TEXT("dipingere non toglie celle"), Asset->NumCells(), CellsBefore);
	TestEqual(TEXT("e non porta via la geometria"), Asset->InteriorWalls.Num(), 4);
	TestEqual(TEXT("nessuna cella cancellata contata"), Tool->StrokeCellsErasedForTest(), 0);
	TestEqual(TEXT("nessuna cascata contata"), Tool->StrokeCascadeRemovedForTest(), 0);
	TestEqual(TEXT("e nessun log di erase"), Log.CountContaining(TEXT("[HexMode] Erase:")), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
