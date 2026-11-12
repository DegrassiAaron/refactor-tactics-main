// Il piano dell'unita' si scrive da una porta sola (#4068 e #4076, E50 fetta 2, tappe 1 e 2).
//
// Il piano per-turno (`Planned*`, `bTurnPlanDeclared` e affini) sta in campi pubblici di `ARTUnit`. Fino a #4068
// lo scrivevano a mano il controller, l'harness degli scenari, la dichiarazione del verso, il TurnManager e il Blast;
// fino a #4076, i test. Ora passano tutti dai mutatori di `ARTUnit` (`DeclarePlannedMainAction`, `ClearPlannedMainAction`,
// `SetPlannedRoute`, ...), e i test, dove nessun mutatore produce lo stato che vogliono, da `FRTUnitPlanTestAccess`
// (`Tests/Shared/RTUnitPlanTestAccess.h`), l'unico file dei test fuori dal perimetro, che il codice di gioco non usa.
// Questo cancello lo tiene vero: quando lo struct `FRTUnitPlan` rendera' i campi privati ([D-608]), cambieranno
// `ARTUnit` e quell'accesso, non i test.
//
// I campi del piano sono un ELENCO, non un prefisso: `Planned*` prendeva anche il widget `PlannedCorner` della barra.
// `Meta.PlanFieldListMatchesTheUnit` tiene l'elenco uguale ai campi dichiarati in `Unit/RTUnit.h`.
//
// ⚠️ **Cosa NON copre, dichiarato.**
//   - Le scritture attraverso un RIFERIMENTO non costante (un campo del piano passato a una funzione che lo modifica):
//     un rilevatore testuale non le vede. #4068 ha tolto le due che c'erano (`TruncateWaypointsToBudget` ora lavora
//     su una copia).
//   - Una scrittura spezzata su piu' righe, con l'operatore a capo dopo il nome del campo.
//   - Le scritture con `.` su un `ARTUnit&`: il piano si raggiunge sempre per puntatore, e con `.` lo scrivono altri
//     tipi che hanno campi con lo stesso nome (`FRTBotPlanDecision`, `FRTPlanPreviewInput`).
//   - Una scrittura attraverso un metodo che rende un riferimento (`->PlannedPath.Last() = C`), e un pre-incremento
//     su un'espressione che non e' un nome semplice (`++(*U)->PlannedAbilityIndex`).
//   - Una riga dentro un commento `/* */` su piu' righe che non comincia con `*`.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * I campi del piano di `ARTUnit`. Ogni campo dell'unita' il cui nome contiene `Planned` deve stare qui
	 * (`Meta.PlanFieldListMatchesTheUnit`); gli ultimi quattro non lo contengono, e ci stanno per scelta.
	 */
	const TCHAR* const PlanWritersCampi[] = {
		TEXT("PlannedCell"), TEXT("PlannedAbilityIndex"), TEXT("PlannedAttackTarget"), TEXT("PlannedAttackCell"),
		TEXT("PlannedCoverEdge"), TEXT("bHasPlannedCoverEdge"), TEXT("PlannedAimCell"), TEXT("bHasPlannedAim"),
		TEXT("PlannedPath"), TEXT("PlannedWaypoints"), TEXT("PlannedMovementProfileId"), TEXT("bDeclaresPlannedFacing"),
		TEXT("PlannedFacing"), TEXT("PlannedDashAbility"), TEXT("PlannedDashCell"), TEXT("PlannedChargeRun"),
		TEXT("bHasPlannedChargeRun"), TEXT("PlannedReactionAbility"), TEXT("PlannedReactionCondition"),
		TEXT("PlannedCleansePriority"),
		TEXT("bAttackTargetsCell"), TEXT("bTurnPlanDeclared"), TEXT("WaypointsAllaDichiarazione"), TEXT("WaypointsAllaReazione"),
	};

	/** Il file dei test che scrive il piano per mestiere: l'accesso riservato, fuori dal perimetro. */
	const TCHAR* const PlanWritersAccessoDeiTest = TEXT("/Tests/Shared/RTUnitPlanTestAccess.h");

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
		for (const TCHAR* Campo : PlanWritersCampi)
		{
			if (Nome.Equals(Campo, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	/**
	 * La riga senza stringhe e senza commenti: il contenuto di `"..."`, di `'...'` e di un commento a blocco aperto e
	 * chiuso sulla riga diventa spazi, e da un `//`, o da un commento a blocco che la riga non chiude, in poi la riga
	 * finisce. Cosi' una scrittura scritta dentro un messaggio (`TEXT("U->PlannedCell = X")`) o citata in un commento non
	 * e' una scrittura.
	 */
	FString PlanWritersCode(const FString& Riga)
	{
		FString Out = Riga;
		TCHAR Apice = 0;
		for (int32 I = 0; I < Out.Len(); ++I)
		{
			const TCHAR C = Out[I];
			if (Apice != 0)
			{
				if (C == TEXT('\\'))
				{
					Out[I] = TEXT(' ');
					if (I + 1 < Out.Len()) { Out[++I] = TEXT(' '); }
				}
				else if (C == Apice) { Apice = 0; }
				else { Out[I] = TEXT(' '); }
			}
			else if (C == TEXT('"') || C == TEXT('\'')) { Apice = C; }
			else if (C == TEXT('/') && I + 1 < Out.Len() && Out[I + 1] == TEXT('/')) { return Out.Left(I); }
			else if (C == TEXT('/') && I + 1 < Out.Len() && Out[I + 1] == TEXT('*'))
			{
				const int32 Chiusa = Out.Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, I + 2);
				if (Chiusa == INDEX_NONE) { return Out.Left(I); }
				for (int32 K = I; K < Chiusa + 2; ++K) { Out[K] = TEXT(' '); }
				I = Chiusa + 1;
			}
		}
		return Out;
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
	 * perche' una dereferenziazione a inizio riga (`*Unit->PlannedCell`) e' codice e non va saltata. Una riga che apre
	 * o chiude un commento a blocco e' un commento solo se dopo la chiusura non c'e' codice: un commento a blocco
	 * seguito da `U->PlannedCell = C;` scrive.
	 */
	bool PlanWritersIsComment(const FString& Riga)
	{
		const FString Pulita = Riga.TrimStart();
		if (Pulita.StartsWith(TEXT("//")) || Pulita.StartsWith(TEXT("* ")) || Pulita.Equals(TEXT("*"))) { return true; }
		if (Pulita.StartsWith(TEXT("/*")) || Pulita.StartsWith(TEXT("*/")))
		{
			const int32 Da = Pulita.StartsWith(TEXT("/*")) ? 2 : 0;
			const int32 Chiusa = Pulita.Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Da);
			return Chiusa == INDEX_NONE || PlanWritersCode(Pulita.Mid(Chiusa + 2)).TrimStartAndEnd().IsEmpty();
		}
		return false;
	}

	/** Vero se `Coda` comincia con `.Membro` seguito da una scrittura: `.X = 3`. */
	bool PlanWritersMemberWrites(const FString& Coda)
	{
		if (!Coda.StartsWith(TEXT("."))) { return false; }
		int32 M = 1;
		while (M < Coda.Len() && PlanWritersIsIdentChar(Coda[M])) { ++M; }
		return M > 1 && PlanWritersTailWrites(Coda.Mid(M).TrimStart());
	}

	/** Vero se subito prima dell'oggetto che finisce a `Freccia` (un nome semplice) c'e' `++` o `--`. */
	bool PlanWritersPreIncrements(const FString& Riga, int32 Freccia)
	{
		int32 I = Freccia;
		while (I > 0 && PlanWritersIsIdentChar(Riga[I - 1])) { --I; }
		if (I == Freccia) { return false; }
		while (I > 0 && FChar::IsWhitespace(Riga[I - 1])) { --I; }
		return I >= 2 && ((Riga[I - 1] == TEXT('+') && Riga[I - 2] == TEXT('+')) || (Riga[I - 1] == TEXT('-') && Riga[I - 2] == TEXT('-')));
	}

	/**
	 * Vero se nella riga c'e' `->CampoDelPiano` seguito da una SCRITTURA del campo (`PlanWritersTailWrites`), di un suo
	 * elemento (`[i] =`), di un suo membro (`.X =`), o da una mutazione dell'array (`.Add(`, `.Pop(`, ...). Le righe di
	 * commento si saltano, e nelle altre si guarda solo il codice (`PlanWritersCode`).
	 */
	bool PlanWritersLineWrites(const FString& RigaIntera)
	{
		if (PlanWritersIsComment(RigaIntera))
		{
			return false;
		}
		const FString Riga = PlanWritersCode(RigaIntera);
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
			// Un pre-incremento: `++Unit->PlannedAbilityIndex`.
			if (PlanWritersPreIncrements(Riga, Freccia)) { return true; }

			int32 K = Fine;
			while (K < Riga.Len() && FChar::IsWhitespace(Riga[K])) { ++K; }
			const FString Coda = Riga.Mid(K);
			if (PlanWritersTailWrites(Coda)) { return true; }
			// Un elemento, anche con un indice annidato o con un suo membro: `->PlannedPath[A[0]] = X`, `->PlannedPath[0].X = 3`.
			if (Coda.StartsWith(TEXT("[")))
			{
				int32 Prof = 0;
				int32 Chiusa = INDEX_NONE;
				for (int32 J = 0; J < Coda.Len(); ++J)
				{
					if (Coda[J] == TEXT('[')) { ++Prof; }
					else if (Coda[J] == TEXT(']') && --Prof == 0) { Chiusa = J; break; }
				}
				if (Chiusa == INDEX_NONE) { continue; }
				const FString Resto = Coda.Mid(Chiusa + 1).TrimStart();
				if (PlanWritersTailWrites(Resto) || PlanWritersMemberWrites(Resto)) { return true; }
				continue;
			}
			if (Coda.StartsWith(TEXT(".")))
			{
				for (const TCHAR* Metodo : PlanWritersMutazioni)
				{
					if (Coda.StartsWith(FString::Printf(TEXT(".%s("), Metodo), ESearchCase::CaseSensitive)) { return true; }
				}
				// Un membro: `->PlannedCell.X = 3`.
				if (PlanWritersMemberWrites(Coda)) { return true; }
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

	/** Le righe (1-based) che USANO l'accesso riservato ai test: nel codice, non in un commento o in una stringa. */
	TArray<int32> PlanWritersFindTestAccess(const FString& Testo)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
		TArray<int32> Out;
		for (int32 I = 0; I < Righe.Num(); ++I)
		{
			if (!PlanWritersIsComment(Righe[I])
				&& PlanWritersCode(Righe[I]).Contains(TEXT("FRTUnitPlanTestAccess"), ESearchCase::CaseSensitive))
			{
				Out.Add(I + 1);
			}
		}
		return Out;
	}
}

/**
 * **Il modulo di gioco scrive il piano solo attraverso `ARTUnit`.** Perimetro: `Source/RefactorTactics`, test compresi
 * (#4076), meno `Unit/RTUnit.h`/`.cpp`, dove stanno i mutatori, e meno l'accesso riservato ai test. Fuori da `Tests/`
 * nessun file usa quell'accesso: altrimenti il gioco scriverebbe il piano a mano passando da li', e il rilevatore delle
 * scritture non lo vedrebbe.
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
	int32 LettiDeiTest = 0;
	int32 LettureDelPiano = 0;
	int32 TestCheUsanoLAccesso = 0;
	bool bVistoIlController = false;
	bool bVistoIlTurnManager = false;
	bool bVistoLAccesso = false;
	for (FString F : Files)
	{
		FPaths::NormalizeFilename(F);
		if (F.EndsWith(PlanWritersAccessoDeiTest))
		{
			bVistoLAccesso = true;
			continue;
		}
		if (F.EndsWith(TEXT("/Unit/RTUnit.h")) || F.EndsWith(TEXT("/Unit/RTUnit.cpp")))
		{
			continue;
		}
		const FString Nome = FPaths::GetCleanFilename(F);
		bVistoIlController |= Nome == TEXT("RTPlayerController.cpp");
		bVistoIlTurnManager |= Nome == TEXT("RTTurnManager.cpp");
		LettiDeiTest += F.Contains(TEXT("/Tests/")) ? 1 : 0;
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
				TEXT("l'operazione non c'e', aggiungila li', accanto alle altre. In un test, se nessun mutatore produce lo ")
				TEXT("stato che serve, `FRTUnitPlanTestAccess` (`Tests/Shared/RTUnitPlanTestAccess.h`). Vedi #4068 e #4076."),
				*Nome, Riga, *Righe[Riga - 1].TrimStartAndEnd()));
		}
		const TArray<int32> UsiDellAccesso = PlanWritersFindTestAccess(Testo);
		if (F.Contains(TEXT("/Tests/")))
		{
			TestCheUsanoLAccesso += UsiDellAccesso.Num() > 0 ? 1 : 0;
			continue;
		}
		for (const int32 Riga : UsiDellAccesso)
		{
			AddError(FString::Printf(
				TEXT("%s:%d usa `FRTUnitPlanTestAccess` fuori dai test: `%s`. L'accesso riservato e' dei test; il codice di ")
				TEXT("gioco scrive il piano dai mutatori di `ARTUnit` (`Unit/RTUnit.h`). Vedi #4076."),
				*Nome, Riga, *Righe[Riga - 1].TrimStartAndEnd()));
		}
	}
	// ⛔ Anti-vacuita': il perimetro contiene i file che il piano lo scrivevano, test compresi, e il piano vi si LEGGE
	// ancora con `->` in piu' file: il rilevatore guarda testo vivo, non un insieme vuoto. L'accesso riservato esiste
	// davvero: se sparisse, il cancello escluderebbe un file che non c'e' e nessuno se ne accorgerebbe.
	TestTrue(TEXT("il controller e' dentro il perimetro"), bVistoIlController);
	TestTrue(TEXT("il TurnManager e' dentro il perimetro"), bVistoIlTurnManager);
	TestTrue(TEXT("l'accesso riservato ai test esiste"), bVistoLAccesso);
	TestTrue(*FString::Printf(TEXT("lo sweep ha letto file di gioco (%d)"), Letti), Letti > 50);
	TestTrue(*FString::Printf(TEXT("e file di test (%d)"), LettiDeiTest), LettiDeiTest > 50);
	TestTrue(*FString::Printf(TEXT("e il piano vi si legge ancora con `->` (%d file)"), LettureDelPiano), LettureDelPiano > 1);
	// E l'accesso riservato i test lo usano davvero: il controllo che lo vieta al gioco guarda testo vivo.
	TestTrue(*FString::Printf(TEXT("e l'accesso riservato vi si usa (%d file di test)"), TestCheUsanoLAccesso), TestCheUsanoLAccesso > 1);
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
		// #4076: un campo dell'elenco che il prefisso non prendeva, e tre cose che non sono scritture del piano.
		{ TEXT("\tUnit->bHasPlannedChargeRun = true;"), true },
		{ TEXT("\tS->PlannedCorner = NewObject<UBorder>(S); // un widget della barra, non un campo dell'unita'"), false },
		{ TEXT("\tTestTrue(TEXT(\"U->PlannedCell = X\"), bOk); // dentro una stringa"), false },
		{ TEXT("\tFoo(); // era: Unit->PlannedCell = Unit->Cell;"), false },
		// #4076, review: le forme che il rilevatore non vedeva, e i commenti a blocco che scrivono o no.
		{ TEXT("\t++Unit->PlannedAbilityIndex;"), true },
		{ TEXT("\tUnit->PlannedPath[0].X = 3;"), true },
		{ TEXT("\tUnit->PlannedPath[Ordine[0]] = Cell;"), true },
		{ TEXT("\t/* nota */ Unit->PlannedCell = Cell;"), true },
		{ TEXT("\t*/ Unit->PlannedCell = Cell;"), true },
		{ TEXT("\t/* Unit->PlannedCell = Cell; */ Foo();"), false },
		{ TEXT("\tFoo(); /* Unit->PlannedCell = Cell; */"), false },
		{ TEXT("\t/* Unit->PlannedCell = Cell;"), false },
		{ TEXT("\tconst int32 Q = Unit->PlannedPath[0].X + Conta++;"), false },
		{ TEXT("\tif (Unit->PlannedPath[Ordine[0]] == Cell) {}"), false },
		{ TEXT("\tconst int32 R = Conta++ + Unit->PlannedAbilityIndex;"), false },
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

	// Ogni voce degli elenchi del rilevatore, piantata una per una: un refuso in una voce lo lascerebbe cieco su quella
	// forma, con tutto verde.
	for (const TCHAR* Metodo : PlanWritersMutazioni)
	{
		const FString Riga = FString::Printf(TEXT("\tUnit->PlannedWaypoints.%s(Cell);"), Metodo);
		TestTrue(*FString::Printf(TEXT("mutazione vista: `%s`"), *Riga.TrimStart()), PlanWritersLineWrites(Riga));
	}
	for (const TCHAR* Op : PlanWritersComposti)
	{
		const FString Riga = FString::Printf(TEXT("\tUnit->PlannedAbilityIndex %s 1;"), Op);
		TestTrue(*FString::Printf(TEXT("operatore visto: `%s`"), *Riga.TrimStart()), PlanWritersLineWrites(Riga));
	}
	for (const TCHAR* Op : { TEXT("++"), TEXT("--") })
	{
		const FString Dopo = FString::Printf(TEXT("\tUnit->PlannedAbilityIndex%s;"), Op);
		const FString Prima = FString::Printf(TEXT("\t%sUnit->PlannedAbilityIndex;"), Op);
		TestTrue(*FString::Printf(TEXT("incremento visto: `%s`"), *Dopo.TrimStart()), PlanWritersLineWrites(Dopo));
		TestTrue(*FString::Printf(TEXT("incremento visto: `%s`"), *Prima.TrimStart()), PlanWritersLineWrites(Prima));
	}

	// L'uso dell'accesso riservato: nel codice si', citato in un commento o in una stringa no.
	const TArray<int32> Usi = PlanWritersFindTestAccess(FString(
		TEXT("\tFRTUnitPlanTestAccess::SetPath(*U, X);\n")
		TEXT("\t// FRTUnitPlanTestAccess, citato\n")
		TEXT("\tTestTrue(TEXT(\"FRTUnitPlanTestAccess\"), bOk);\n")));
	TestTrue(*FString::Printf(TEXT("l'uso dell'accesso si vede solo nel codice (righe viste: %d)"), Usi.Num()),
		Usi.Num() == 1 && Usi[0] == 1);
	return true;
}

namespace
{
	/**
	 * Il nome dichiarato da una riga di `RTUnit.h`, se la riga dichiara un campo: `Tipo Nome;`, `Tipo Nome = valore;`,
	 * `Tipo Nome{};`, `uint8 bNome : 1;`, anche dopo un `UPROPERTY(...)` sulla stessa riga. La testa e' cio' che precede
	 * l'inizializzatore o la larghezza del bitfield: le parentesi dell'inizializzatore non dicono niente, quelle della
	 * testa dicono «funzione». Un'assegnazione dentro un mutatore (`PlannedCell = At;`) non ha un tipo davanti, e non conta.
	 */
	FString PlanWritersDeclaredField(const FString& RigaIntera)
	{
		if (PlanWritersIsComment(RigaIntera)) { return FString(); }
		FString Riga = PlanWritersCode(RigaIntera).TrimStartAndEnd();
		if (Riga.StartsWith(TEXT("UPROPERTY(")))
		{
			int32 Prof = 0;
			int32 Fine = INDEX_NONE;
			for (int32 I = 0; I < Riga.Len(); ++I)
			{
				if (Riga[I] == TEXT('(')) { ++Prof; }
				else if (Riga[I] == TEXT(')') && --Prof == 0) { Fine = I; break; }
			}
			if (Fine == INDEX_NONE) { return FString(); }
			Riga = Riga.Mid(Fine + 1).TrimStart();
		}
		if (!Riga.EndsWith(TEXT(";"))) { return FString(); }
		FString Testa = Riga.LeftChop(1);
		for (int32 I = 0; I < Testa.Len(); ++I)
		{
			const TCHAR C = Testa[I];
			const bool bDuePunti = C == TEXT(':') && !(I + 1 < Testa.Len() && Testa[I + 1] == TEXT(':'))
				&& !(I > 0 && Testa[I - 1] == TEXT(':'));
			if (C == TEXT('=') || C == TEXT('{') || bDuePunti)
			{
				Testa = Testa.Left(I);
				break;
			}
		}
		if (Testa.Contains(TEXT("(")) || Testa.Contains(TEXT("}"))) { return FString(); }
		TArray<FString> Parole;
		Testa.TrimStartAndEnd().ParseIntoArrayWS(Parole);
		if (Parole.Num() < 2 || Parole[0] == TEXT("return") || Parole[0] == TEXT("delete") || Parole[0] == TEXT("case"))
		{
			return FString();
		}
		return Parole.Last().Replace(TEXT("*"), TEXT("")).Replace(TEXT("&"), TEXT(""));
	}
}

/**
 * **L'elenco dei campi del piano e' quello dell'unita'.** Ogni campo di `ARTUnit` il cui nome contiene `Planned` sta
 * nell'elenco del cancello, e ogni voce dell'elenco e' un campo dichiarato in `Unit/RTUnit.h`. Senza, un campo nuovo del
 * piano sfuggirebbe al cancello, e una voce rinominata lo renderebbe cieco in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanFieldListMatchesTheUnitTest,
	"RefactorTactics.Meta.PlanFieldListMatchesTheUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanFieldListMatchesTheUnitTest::RunTest(const FString&)
{
	const FString Header = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("RefactorTactics"), TEXT("Unit"), TEXT("RTUnit.h"));
	FString Testo;
	if (!TestTrue(*FString::Printf(TEXT("l'header si legge (%s)"), *Header), FFileHelper::LoadFileToString(Testo, *Header))) { return false; }

	// ⛔ Il parser, su righe piantate: le forme di dichiarazione che un campo nuovo puo' prendere, e le righe che non
	// dichiarano niente. Un campo che sfugge al parser sfugge all'elenco, e quindi al cancello, in silenzio.
	struct FDichiarazione { const TCHAR* Riga; const TCHAR* Nome; };
	const FDichiarazione Piantate[] = {
		{ TEXT("\tFRTCellId PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tint32 PlannedFoo = INDEX_NONE;"), TEXT("PlannedFoo") },
		{ TEXT("\tFRTCellId PlannedFoo = FRTCellId(0, 0, 0);"), TEXT("PlannedFoo") },
		{ TEXT("\tFRTCellId PlannedFoo{};"), TEXT("PlannedFoo") },
		{ TEXT("\tUPROPERTY(VisibleAnywhere, Category = \"Piano\") int32 PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tuint8 bPlannedFoo : 1;"), TEXT("bPlannedFoo") },
		{ TEXT("\tTWeakObjectPtr<ARTUnit> PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tTMap<FName, int32> PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tvoid SetPlannedFoo(const FRTCellId& NewCell);"), TEXT("") },
		{ TEXT("\tvoid ClearPlannedFoo() { PlannedFoo = INDEX_NONE; }"), TEXT("") },
		{ TEXT("\tPlannedFoo = At;"), TEXT("") },
		{ TEXT("\treturn PlannedFoo;"), TEXT("") },
		{ TEXT("\tcase ERTPlannedFoo::A: return 1;"), TEXT("") },
		{ TEXT("\t// FRTCellId PlannedFoo;"), TEXT("") },
	};
	for (const FDichiarazione& P : Piantate)
	{
		TestEqual(FString::Printf(TEXT("parser: `%s`"), P.Riga), PlanWritersDeclaredField(P.Riga), FString(P.Nome));
	}

	TArray<FString> Righe;
	Testo.ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
	TSet<FString> Dichiarati;
	for (const FString& Riga : Righe)
	{
		const FString Nome = PlanWritersDeclaredField(Riga);
		if (!Nome.IsEmpty()) { Dichiarati.Add(Nome); }
	}
	// ⛔ Anti-vacuita': il parser trova davvero i campi, compresi uno con l'inizializzatore e uno senza.
	if (!TestTrue(TEXT("premessa: il parser vede `PlannedCell` (senza inizializzatore)"), Dichiarati.Contains(TEXT("PlannedCell")))
		|| !TestTrue(TEXT("premessa: e `PlannedAbilityIndex` (con)"), Dichiarati.Contains(TEXT("PlannedAbilityIndex"))))
	{
		return false;
	}

	TSet<FString> Elenco;
	for (const TCHAR* Campo : PlanWritersCampi) { Elenco.Add(Campo); }
	for (const FString& Nome : Dichiarati)
	{
		if (Nome.Contains(TEXT("Planned"), ESearchCase::CaseSensitive))
		{
			TestTrue(*FString::Printf(TEXT("`%s` e' un campo del piano di `ARTUnit`: sta nell'elenco del cancello"), *Nome),
				Elenco.Contains(Nome));
		}
	}
	for (const FString& Nome : Elenco)
	{
		TestTrue(*FString::Printf(TEXT("la voce `%s` dell'elenco e' un campo dichiarato in `Unit/RTUnit.h`"), *Nome),
			Dichiarati.Contains(Nome));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
