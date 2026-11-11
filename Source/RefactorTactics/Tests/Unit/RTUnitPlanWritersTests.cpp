// Il piano dell'unita' si scrive da una porta sola (#4068, E50 fetta 2, tappa 1).
//
// Il piano per-turno (`Planned*`, `bTurnPlanDeclared` e affini) sta in campi pubblici di `ARTUnit`. Fino a #4068
// lo scrivevano a mano il controller, l'harness degli scenari, la dichiarazione del verso, il TurnManager e il Blast.
// Ora passano tutti dai mutatori di `ARTUnit` (`DeclarePlannedMainAction`, `ClearPlannedMainAction`,
// `SetPlannedRoute`, ...), e questo cancello lo tiene vero finche' lo struct `FRTUnitPlan` non rende i campi privati.
//
// ⚠️ **Cosa NON copre, dichiarato.**
//   - Le scritture attraverso un RIFERIMENTO non costante (un campo del piano passato a una funzione che lo modifica):
//     un rilevatore testuale non le vede. #4068 ha tolto le due che c'erano (`TruncateWaypointsToBudget` ora lavora
//     su una copia).
//   - Una scrittura spezzata su piu' righe, con l'operatore a capo dopo il nome del campo.
//   - Le scritture con `.` su un `ARTUnit&`: in produzione il piano si raggiunge sempre per puntatore, e con `.` lo
//     scrivono altri tipi che hanno campi con lo stesso nome (`FRTBotPlanDecision`, `FRTPlanPreviewInput`).
//   - I test, che scrivono i campi a mano di proposito: sono fuori dal perimetro.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** I campi del piano che non cominciano per `Planned`. */
	const TCHAR* const PlanWritersAltriCampi[] = {
		TEXT("bAttackTargetsCell"), TEXT("bHasPlannedCoverEdge"), TEXT("bDeclaresPlannedFacing"), TEXT("bHasPlannedAim"),
		TEXT("bTurnPlanDeclared"), TEXT("WaypointsAllaDichiarazione"), TEXT("WaypointsAllaReazione"),
	};

	/** Le mutazioni di un `TArray` del piano che contano come scrittura. */
	const TCHAR* const PlanWritersMutazioni[] = {
		TEXT("Add"), TEXT("AddUnique"), TEXT("AddDefaulted"), TEXT("AddZeroed"), TEXT("Emplace"), TEXT("EmplaceAt"),
		TEXT("Push"), TEXT("Pop"), TEXT("Insert"), TEXT("Append"), TEXT("Reset"), TEXT("Empty"), TEXT("Init"),
		TEXT("SetNum"), TEXT("SetNumZeroed"), TEXT("Remove"), TEXT("RemoveAt"), TEXT("RemoveAtSwap"), TEXT("RemoveAll"),
		TEXT("RemoveSingle"), TEXT("Sort"), TEXT("StableSort"), TEXT("Swap"), TEXT("Shrink"),
	};

	/** Gli operatori d'assegnazione composti: tutti scrivono il campo che hanno a sinistra. */
	const TCHAR* const PlanWritersComposti[] = {
		TEXT("+="), TEXT("-="), TEXT("*="), TEXT("/="), TEXT("%="), TEXT("&="), TEXT("|="), TEXT("^="), TEXT("<<="), TEXT(">>="),
	};

	bool PlanWritersIsIdentChar(TCHAR C) { return FChar::IsAlnum(C) || C == TEXT('_'); }

	bool PlanWritersIsPlanField(const FString& Nome)
	{
		if (Nome.StartsWith(TEXT("Planned"), ESearchCase::CaseSensitive))
		{
			return true;
		}
		for (const TCHAR* Campo : PlanWritersAltriCampi)
		{
			if (Nome.Equals(Campo, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	/**
	 * Vero se la coda dopo un campo del piano lo SCRIVE: un'assegnazione semplice o composta, un incremento, o
	 * (sull'array) una mutazione. Un confronto (`==`, `!=`, `<=`, `>=`) non conta, ne' una lettura.
	 */
	bool PlanWritersTailWrites(const FString& Coda)
	{
		if (Coda.StartsWith(TEXT("++")) || Coda.StartsWith(TEXT("--"))) { return true; }
		for (const TCHAR* Op : PlanWritersComposti)
		{
			if (Coda.StartsWith(Op)) { return true; }
		}
		return Coda.StartsWith(TEXT("=")) && !Coda.StartsWith(TEXT("=="));
	}

	/**
	 * Una riga di commento, con la convenzione del gemello del Blast (`RTEnvironmentActionTests`): `* ` con lo SPAZIO,
	 * perche' una dereferenziazione a inizio riga (`*Unit->PlannedCell`) e' codice e non va saltata.
	 */
	bool PlanWritersIsComment(const FString& Riga)
	{
		const FString Pulita = Riga.TrimStart();
		return Pulita.StartsWith(TEXT("//")) || Pulita.StartsWith(TEXT("/*")) || Pulita.StartsWith(TEXT("* "))
			|| Pulita.StartsWith(TEXT("*/")) || Pulita.Equals(TEXT("*"));
	}

	/**
	 * Vero se nella riga c'e' `->CampoDelPiano` seguito da una SCRITTURA del campo (`PlanWritersTailWrites`), di un suo
	 * elemento (`[i] =`), di un suo membro (`.X =`), o da una mutazione dell'array (`.Add(`, `.Pop(`, ...). Le righe di
	 * commento si saltano.
	 */
	bool PlanWritersLineWrites(const FString& Riga)
	{
		if (PlanWritersIsComment(Riga))
		{
			return false;
		}
		int32 Da = 0;
		while (true)
		{
			const int32 Freccia = Riga.Find(TEXT("->"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Da);
			if (Freccia == INDEX_NONE) { return false; }
			int32 Fine = Freccia + 2;
			while (Fine < Riga.Len() && PlanWritersIsIdentChar(Riga[Fine])) { ++Fine; }
			const FString Nome = Riga.Mid(Freccia + 2, Fine - Freccia - 2);
			Da = Fine;
			if (!PlanWritersIsPlanField(Nome)) { continue; }

			int32 K = Fine;
			while (K < Riga.Len() && FChar::IsWhitespace(Riga[K])) { ++K; }
			const FString Coda = Riga.Mid(K);
			if (PlanWritersTailWrites(Coda)) { return true; }
			// Un elemento: `->PlannedPath[0] = X`.
			if (Coda.StartsWith(TEXT("[")))
			{
				const int32 Chiusa = Coda.Find(TEXT("]"));
				if (Chiusa != INDEX_NONE && PlanWritersTailWrites(Coda.Mid(Chiusa + 1).TrimStart())) { return true; }
				continue;
			}
			if (Coda.StartsWith(TEXT(".")))
			{
				for (const TCHAR* Metodo : PlanWritersMutazioni)
				{
					if (Coda.StartsWith(FString::Printf(TEXT(".%s("), Metodo), ESearchCase::CaseSensitive)) { return true; }
				}
				// Un membro: `->PlannedCell.X = 3`.
				int32 M = 1;
				while (M < Coda.Len() && PlanWritersIsIdentChar(Coda[M])) { ++M; }
				if (M > 1 && PlanWritersTailWrites(Coda.Mid(M).TrimStart())) { return true; }
			}
		}
	}

	/** Le righe (1-based) del testo che scrivono il piano. */
	TArray<int32> PlanWritersFind(const FString& Testo)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
		TArray<int32> Out;
		for (int32 I = 0; I < Righe.Num(); ++I)
		{
			if (PlanWritersLineWrites(Righe[I])) { Out.Add(I + 1); }
		}
		return Out;
	}
}

/**
 * **Il codice di gioco scrive il piano solo attraverso `ARTUnit`.** Perimetro: il modulo di gioco
 * (`Source/RefactorTactics`), meno i test e meno `Unit/RTUnit.h`/`.cpp`, dove stanno i mutatori.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWritesGoThroughTheUnitTest,
	"RefactorTactics.Meta.PlanFieldsAreWrittenOnlyThroughTheUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWritesGoThroughTheUnitTest::RunTest(const FString&)
{
	const FString Radice = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("RefactorTactics"));
	TArray<FString> Files;
	for (const TCHAR* Ext : { TEXT("*.cpp"), TEXT("*.h") })
	{
		TArray<FString> Trovati;
		IFileManager::Get().FindFilesRecursive(Trovati, *Radice, Ext, /*Files=*/ true, /*Dirs=*/ false, /*bClearFileNames=*/ false);
		Files.Append(Trovati);
	}
	// ⛔ Zero file letti vuol dire un percorso sbagliato, non un modulo pulito.
	if (!TestTrue(*FString::Printf(TEXT("lo sweep ha trovato i sorgenti (%s)"), *Radice), Files.Num() > 0)) { return false; }

	int32 Letti = 0;
	int32 LettureDelPiano = 0;
	bool bVistoIlController = false;
	bool bVistoIlTurnManager = false;
	for (FString F : Files)
	{
		FPaths::NormalizeFilename(F);
		if (F.Contains(TEXT("/Tests/")) || F.EndsWith(TEXT("/Unit/RTUnit.h")) || F.EndsWith(TEXT("/Unit/RTUnit.cpp")))
		{
			continue;
		}
		const FString Nome = FPaths::GetCleanFilename(F);
		bVistoIlController |= Nome == TEXT("RTPlayerController.cpp");
		bVistoIlTurnManager |= Nome == TEXT("RTTurnManager.cpp");
		FString Testo;
		if (!TestTrue(*FString::Printf(TEXT("il sorgente si legge: %s"), *Nome), FFileHelper::LoadFileToString(Testo, *F)))
		{
			return false;
		}
		++Letti;
		LettureDelPiano += Testo.Contains(TEXT("->PlannedAbilityIndex"), ESearchCase::CaseSensitive) ? 1 : 0;
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
		for (const int32 Riga : PlanWritersFind(Testo))
		{
			AddError(FString::Printf(
				TEXT("%s:%d scrive un campo del piano a mano: `%s`. Il piano si scrive dai mutatori di `ARTUnit` ")
				TEXT("(`DeclarePlannedMainAction`, `ClearPlannedMainAction`, `SetPlannedRoute`, ... in `Unit/RTUnit.h`): se ")
				TEXT("l'operazione non c'e', aggiungila li', accanto alle altre. Vedi #4068."),
				*Nome, Riga, *Righe[Riga - 1].TrimStartAndEnd()));
		}
	}
	// ⛔ Anti-vacuita': il perimetro contiene i file che il piano lo scrivevano, e in produzione il piano si LEGGE ancora
	// con `->` in piu' file: il rilevatore guarda testo vivo, non un insieme vuoto.
	TestTrue(TEXT("il controller e' dentro il perimetro"), bVistoIlController);
	TestTrue(TEXT("il TurnManager e' dentro il perimetro"), bVistoIlTurnManager);
	TestTrue(*FString::Printf(TEXT("lo sweep ha letto file di gioco (%d)"), Letti), Letti > 50);
	TestTrue(*FString::Printf(TEXT("e il piano vi si legge ancora con `->` (%d file)"), LettureDelPiano), LettureDelPiano > 1);
	return true;
}

/**
 * **Il controllo positivo del cancello**: lo STESSO rilevatore, su un testo piantato, vede ogni forma di scrittura e
 * nessuna lettura. Senza, lo sweep verde non distinguerebbe «nessuno scrive a mano» da «rilevatore cieco».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWritesGuardSeesPlantedWritesTest,
	"RefactorTactics.Meta.PlanWritesGuardSeesPlantedWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWritesGuardSeesPlantedWritesTest::RunTest(const FString&)
{
	struct FRiga { const TCHAR* Testo; bool bScrive; };
	const FRiga Righe[] = {
		{ TEXT("\tUnit->PlannedCell = Unit->Cell;"), true },
		{ TEXT("\tUnit->PlannedAbilityIndex=INDEX_NONE;"), true },
		{ TEXT("\tUnit->PlannedWaypoints.Pop();"), true },
		{ TEXT("\tSelectedUnit->PlannedWaypoints.Add(Cell);"), true },
		{ TEXT("\tUnit->bHasPlannedCoverEdge = false;"), true },
		{ TEXT("\tUnit->WaypointsAllaDichiarazione = Unit->PlannedWaypoints.Num();"), true },
		{ TEXT("\tUnit->PlannedReactionAbility += 1;"), true },
		{ TEXT("\tUnit->bHasPlannedCoverEdge |= bBordo;"), true },
		{ TEXT("\tUnit->bAttackTargetsCell &= bCella;"), true },
		{ TEXT("\tUnit->PlannedWaypoints.Emplace(Cell);"), true },
		{ TEXT("\tUnit->PlannedPath[0] = Cell;"), true },
		{ TEXT("\tUnit->PlannedCell.X = 3;"), true },
		{ TEXT("\t*Unit->PlannedCell = Other; // dereferenziazione a inizio riga: codice, non commento"), true },
		{ TEXT("\t*Unit->PlannedDashAbility == 0;"), false },
		{ TEXT("\tif (Unit->PlannedPath[0] == Cell && Unit->PlannedCell.X >= 0) {}"), false },
		{ TEXT("\tif (Unit->PlannedCell == OldCell) { Unit->SetPlannedCell(NewCell); }"), false },
		{ TEXT("\tif (Unit->PlannedAbilityIndex != INDEX_NONE && Unit->PlannedDashAbility <= 0) {}"), false },
		{ TEXT("\tconst int32 N = Unit->PlannedWaypoints.Num();"), false },
		{ TEXT("\tPiano.PlannedCell = Bot.Cell;"), false },
		{ TEXT("\t// Unit->PlannedCell = Unit->Cell;"), false },
		{ TEXT("\t * `Unit->PlannedAbilityIndex = INDEX_NONE` lo faceva a mano"), false },
		{ TEXT("\tUnit->ClearPlannedMainAction();"), false },
		{ TEXT("\tPlannedAbilityIndex = INDEX_NONE; // dentro ARTUnit"), false },
	};
	FString Testo;
	TArray<int32> Attese;
	for (int32 I = 0; I < UE_ARRAY_COUNT(Righe); ++I)
	{
		Testo += Righe[I].Testo;
		Testo += TEXT("\n");
		if (Righe[I].bScrive) { Attese.Add(I + 1); }
	}
	const TArray<int32> Trovate = PlanWritersFind(Testo);
	TestEqual(TEXT("le scritture piantate sono tutte viste"), Trovate.Num(), Attese.Num());
	for (int32 I = 0; I < UE_ARRAY_COUNT(Righe); ++I)
	{
		TestEqual(FString::Printf(TEXT("riga %d: `%s`"), I + 1, Righe[I].Testo), Trovate.Contains(I + 1), Righe[I].bScrive);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
