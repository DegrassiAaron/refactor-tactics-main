// Il piano dell'unita' si scrive da una porta sola (#4068 e #4076, E50 fetta 2, tappe 1 e 2).
//
// Il piano per-turno (`Planned*`, `bTurnPlanDeclared` e affini) sta in campi pubblici di `ARTUnit`. Fino a #4068
// lo scrivevano a mano il controller, l'harness degli scenari, la dichiarazione del verso, il TurnManager e il Blast;
// fino a #4076, i test. Ora passano tutti dai mutatori di `ARTUnit` (`DeclarePlannedMainAction`, `ClearPlannedMainAction`,
// `SetPlannedRoute`, ...), e i test, dove nessun mutatore produce lo stato che vogliono, da `FRTUnitPlanTestAccess`
// (`Tests/Shared/RTUnitPlanTestAccess.h`), l'unico file dei test fuori dal perimetro. Fuori dai test nessun file lo usa,
// e nessuno include un header dei test: ne' il modulo di gioco ne' quello dell'Editor, che sono entrambi nel perimetro.
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
//   - Un metodo di `TArray` che non e' nell'elenco (l'elenco segue `Containers/Array.h` di UE 5.8), e le funzioni libere
//     che ricevono l'array per riferimento (`Algo::Sort(Unit->PlannedPath)`): sono il primo caso di questo elenco.

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

	/**
	 * Le mutazioni di un `TArray` del piano che contano come scrittura: i metodi pubblici di `Containers/Array.h` (UE 5.8)
	 * che ne cambiano il contenuto o l'ordine. `Reserve` cambia solo la capacita', e non c'e'. Ogni voce ha la sua riga
	 * piantata, scritta a mano e non ricavata da qui, in `Meta.PlanWritesGuardSeesPlantedWrites`.
	 */
	const TCHAR* const PlanWritersMutazioni[] = {
		TEXT("Add"), TEXT("Add_GetRef"), TEXT("AddDefaulted"), TEXT("AddDefaulted_GetRef"), TEXT("AddUninitialized"),
		TEXT("AddUnique"), TEXT("AddZeroed"), TEXT("AddZeroed_GetRef"), TEXT("Append"), TEXT("Emplace"), TEXT("Emplace_GetRef"),
		TEXT("EmplaceAt"), TEXT("EmplaceAt_GetRef"), TEXT("Empty"), TEXT("Heapify"), TEXT("HeapPop"), TEXT("HeapPopDiscard"),
		TEXT("HeapPush"), TEXT("HeapRemoveAt"), TEXT("HeapSort"), TEXT("Init"), TEXT("Insert"), TEXT("Insert_GetRef"),
		TEXT("InsertDefaulted"), TEXT("InsertDefaulted_GetRef"), TEXT("InsertUninitialized"), TEXT("InsertZeroed"),
		TEXT("InsertZeroed_GetRef"), TEXT("Pop"), TEXT("Push"), TEXT("Remove"), TEXT("RemoveAll"), TEXT("RemoveAllSwap"),
		TEXT("RemoveAt"), TEXT("RemoveAtSwap"), TEXT("RemoveSingle"), TEXT("RemoveSingleSwap"), TEXT("RemoveSwap"), TEXT("Reset"),
		TEXT("SetNum"), TEXT("SetNumUninitialized"), TEXT("SetNumZeroed"), TEXT("Shrink"), TEXT("Sort"), TEXT("StableSort"),
		TEXT("Swap"), TEXT("SwapMemory"),
	};

	/** Gli operatori d'assegnazione composti: tutti scrivono il campo che hanno a sinistra. Righe piantate a mano, come sopra. */
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
	 * Il codice di una riga, per i rilevatori: vuoto se la riga e' un commento. Con la convenzione del gemello del Blast
	 * (`RTEnvironmentActionTests`), `* ` con lo SPAZIO apre una riga DENTRO un commento a blocco: una dereferenziazione
	 * a inizio riga (`*Unit->PlannedCell`) e' codice e non va saltata. Una riga dentro un commento a blocco, o che
	 * comincia chiudendolo, ha codice solo dopo la chiusura: `* nota`, la chiusura e poi `U->PlannedCell = C;` scrive.
	 * Il resto passa da `PlanWritersCode`, che toglie stringhe e commenti.
	 */
	FString PlanWritersLineCode(const FString& Riga)
	{
		const FString Pulita = Riga.TrimStart();
		if (Pulita.StartsWith(TEXT("//"))) { return FString(); }
		if (Pulita.StartsWith(TEXT("* ")) || Pulita.Equals(TEXT("*")) || Pulita.StartsWith(TEXT("*/")))
		{
			const int32 Chiusa = Pulita.Find(TEXT("*/"), ESearchCase::CaseSensitive);
			return Chiusa == INDEX_NONE ? FString() : PlanWritersCode(Pulita.Mid(Chiusa + 2));
		}
		return PlanWritersCode(Riga);
	}

	/** Una riga senza codice: un commento, o la parte di un commento a blocco. */
	bool PlanWritersIsComment(const FString& Riga) { return PlanWritersLineCode(Riga).TrimStartAndEnd().IsEmpty(); }

	bool PlanWritersIsMutation(const FString& Metodo)
	{
		for (const TCHAR* M : PlanWritersMutazioni)
		{
			if (Metodo.Equals(M, ESearchCase::CaseSensitive)) { return true; }
		}
		return false;
	}

	/** Vero se `Coda` comincia con `.Membro` seguito da una scrittura: `.X = 3`. */
	bool PlanWritersMemberWrites(const FString& Coda)
	{
		if (!Coda.StartsWith(TEXT("."))) { return false; }
		int32 M = 1;
		while (M < Coda.Len() && FChar::IsWhitespace(Coda[M])) { ++M; }
		const int32 Inizio = M;
		while (M < Coda.Len() && PlanWritersIsIdentChar(Coda[M])) { ++M; }
		return M > Inizio && PlanWritersTailWrites(Coda.Mid(M).TrimStart());
	}

	/** Vero se `Coda` comincia con la chiamata di una mutazione dell'array, anche con spazi: `.Add(`, `. Add(`, `.Add (`. */
	bool PlanWritersMutationCall(const FString& Coda)
	{
		if (!Coda.StartsWith(TEXT("."))) { return false; }
		int32 M = 1;
		while (M < Coda.Len() && FChar::IsWhitespace(Coda[M])) { ++M; }
		const int32 Inizio = M;
		while (M < Coda.Len() && PlanWritersIsIdentChar(Coda[M])) { ++M; }
		const FString Metodo = Coda.Mid(Inizio, M - Inizio);
		while (M < Coda.Len() && FChar::IsWhitespace(Coda[M])) { ++M; }
		return M < Coda.Len() && Coda[M] == TEXT('(') && PlanWritersIsMutation(Metodo);
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
	 * elemento (`[i] =`), di un suo membro (`.X =`), o da una mutazione dell'array (`.Add(`, `.Pop(`, ...). Si guarda solo
	 * il codice della riga (`PlanWritersLineCode`).
	 */
	bool PlanWritersLineWrites(const FString& RigaIntera)
	{
		const FString Riga = PlanWritersLineCode(RigaIntera);
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
			// Una mutazione dell'array, o un membro: `->PlannedWaypoints.Add(C)`, `->PlannedCell.X = 3`.
			if (PlanWritersMutationCall(Coda) || PlanWritersMemberWrites(Coda)) { return true; }
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
			if (PlanWritersLineCode(Righe[I]).Contains(TEXT("FRTUnitPlanTestAccess"), ESearchCase::CaseSensitive))
			{
				Out.Add(I + 1);
			}
		}
		return Out;
	}

	/**
	 * Le righe (1-based) che includono un header dei test (`#include "Tests/..."` o `".../Tests/..."`). Fuori dai test
	 * vietarlo chiude anche la via di una funzione dei test che chiami l'accesso riservato per conto del gioco.
	 */
	TArray<int32> PlanWritersFindTestsInclude(const FString& Testo)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
		TArray<int32> Out;
		for (int32 I = 0; I < Righe.Num(); ++I)
		{
			const FString Pulita = Righe[I].TrimStart();
			if (!Pulita.StartsWith(TEXT("#include"), ESearchCase::CaseSensitive)) { continue; }
			int32 Apre = INDEX_NONE;
			int32 Chiude = INDEX_NONE;
			if (!Pulita.FindChar(TEXT('"'), Apre)) { continue; }
			Chiude = Pulita.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Apre + 1);
			if (Chiude == INDEX_NONE) { continue; }
			const FString Header = Pulita.Mid(Apre + 1, Chiude - Apre - 1);
			if (Header.StartsWith(TEXT("Tests/"), ESearchCase::CaseSensitive) || Header.Contains(TEXT("/Tests/"), ESearchCase::CaseSensitive))
			{
				Out.Add(I + 1);
			}
		}
		return Out;
	}

	/** Un file dei test: lo dice il percorso RELATIVO alla radice del modulo, non quello assoluto. */
	bool PlanWritersIsTestFile(const FString& Relativo)
	{
		return Relativo.StartsWith(TEXT("Tests/"), ESearchCase::CaseSensitive) || Relativo.Contains(TEXT("/Tests/"), ESearchCase::CaseSensitive);
	}
}

/**
 * **Il gioco scrive il piano solo attraverso `ARTUnit`.** Perimetro: `Source/RefactorTactics` e `Source/RefactorTacticsEditor`,
 * test compresi (#4076), meno `Unit/RTUnit.h`/`.cpp`, dove stanno i mutatori, e meno l'accesso riservato ai test. Fuori dai
 * test nessun file usa quell'accesso ne' include un header dei test: altrimenti il gioco scriverebbe il piano a mano
 * passando da li', e il rilevatore delle scritture non lo vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWritesGoThroughTheUnitTest,
	"RefactorTactics.Meta.PlanFieldsAreWrittenOnlyThroughTheUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWritesGoThroughTheUnitTest::RunTest(const FString&)
{
	// (percorso, percorso relativo alla radice del suo modulo, modulo di gioco?)
	struct FSorgente { FString Percorso; FString Relativo; bool bGioco; };
	TArray<FSorgente> Files;
	int32 LettiDellEditor = 0;
	for (const TCHAR* Modulo : { TEXT("RefactorTactics"), TEXT("RefactorTacticsEditor") })
	{
		FString Radice = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), Modulo);
		FPaths::NormalizeDirectoryName(Radice);
		int32 DelModulo = 0;
		for (const TCHAR* Ext : { TEXT("*.cpp"), TEXT("*.h") })
		{
			TArray<FString> Trovati;
			IFileManager::Get().FindFilesRecursive(Trovati, *Radice, Ext, /*Files=*/ true, /*Dirs=*/ false, /*bClearFileNames=*/ false);
			for (FString F : Trovati)
			{
				FPaths::NormalizeFilename(F);
				if (!F.StartsWith(Radice + TEXT("/"), ESearchCase::CaseSensitive)) { continue; }
				Files.Add({ F, F.Mid(Radice.Len() + 1), FCString::Strcmp(Modulo, TEXT("RefactorTactics")) == 0 });
				++DelModulo;
			}
		}
		// ⛔ Zero file trovati vuol dire un percorso sbagliato, non un modulo pulito.
		if (!TestTrue(*FString::Printf(TEXT("lo sweep ha trovato i sorgenti di %s (%s)"), Modulo, *Radice), DelModulo > 0)) { return false; }
	}

	int32 Letti = 0;
	int32 LettiDeiTest = 0;
	int32 LettiFuoriDaiTest = 0;
	int32 LettureDelPiano = 0;
	int32 TestCheUsanoLAccesso = 0;
	int32 TestCheIncludonoITest = 0;
	bool bVistoIlController = false;
	bool bVistoIlTurnManager = false;
	bool bVistoLAccesso = false;
	for (const FSorgente& S : Files)
	{
		const FString& F = S.Percorso;
		if (S.bGioco && S.Relativo.Equals(TEXT("Tests/Shared/RTUnitPlanTestAccess.h"), ESearchCase::CaseSensitive))
		{
			bVistoLAccesso = true;
			continue;
		}
		if (S.bGioco && (S.Relativo.Equals(TEXT("Unit/RTUnit.h"), ESearchCase::CaseSensitive)
			|| S.Relativo.Equals(TEXT("Unit/RTUnit.cpp"), ESearchCase::CaseSensitive)))
		{
			continue;
		}
		const FString Nome = FPaths::GetCleanFilename(F);
		const bool bTest = PlanWritersIsTestFile(S.Relativo);
		bVistoIlController |= S.bGioco && S.Relativo.Equals(TEXT("Player/RTPlayerController.cpp"), ESearchCase::CaseSensitive);
		bVistoIlTurnManager |= S.bGioco && S.Relativo.Equals(TEXT("Turn/RTTurnManager.cpp"), ESearchCase::CaseSensitive);
		LettiDeiTest += bTest ? 1 : 0;
		LettiFuoriDaiTest += bTest ? 0 : 1;
		LettiDellEditor += S.bGioco ? 0 : 1;
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
		const TArray<int32> IncludeDeiTest = PlanWritersFindTestsInclude(Testo);
		if (bTest)
		{
			TestCheUsanoLAccesso += UsiDellAccesso.Num() > 0 ? 1 : 0;
			TestCheIncludonoITest += IncludeDeiTest.Num() > 0 ? 1 : 0;
			continue;
		}
		for (const int32 Riga : UsiDellAccesso)
		{
			AddError(FString::Printf(
				TEXT("%s:%d usa `FRTUnitPlanTestAccess` fuori dai test: `%s`. L'accesso riservato e' dei test; il codice di ")
				TEXT("gioco scrive il piano dai mutatori di `ARTUnit` (`Unit/RTUnit.h`). Vedi #4076."),
				*Nome, Riga, *Righe[Riga - 1].TrimStartAndEnd()));
		}
		for (const int32 Riga : IncludeDeiTest)
		{
			AddError(FString::Printf(
				TEXT("%s:%d include un header dei test fuori dai test: `%s`. Il codice di gioco e dell'Editor non dipende ")
				TEXT("dai test: e' anche la via per cui una funzione dei test scriverebbe il piano per conto del gioco. Vedi #4076."),
				*Nome, Riga, *Righe[Riga - 1].TrimStartAndEnd()));
		}
	}
	// ⛔ Anti-vacuita': il perimetro contiene i file che il piano lo scrivevano, test compresi, e il piano vi si LEGGE
	// ancora con `->` in piu' file: il rilevatore guarda testo vivo, non un insieme vuoto. L'accesso riservato esiste
	// davvero: se sparisse, il cancello escluderebbe un file che non c'e' e nessuno se ne accorgerebbe. I due controlli
	// riservati al codice non di test guardano una popolazione vera: se la classificazione mettesse tutto fra i test,
	// tacerebbero senza un rosso.
	TestTrue(TEXT("il controller e' dentro il perimetro"), bVistoIlController);
	TestTrue(TEXT("il TurnManager e' dentro il perimetro"), bVistoIlTurnManager);
	TestTrue(TEXT("l'accesso riservato ai test esiste"), bVistoLAccesso);
	TestTrue(*FString::Printf(TEXT("lo sweep ha letto file (%d)"), Letti), Letti > 50);
	TestTrue(*FString::Printf(TEXT("e file di test (%d)"), LettiDeiTest), LettiDeiTest > 50);
	TestTrue(*FString::Printf(TEXT("e file fuori dai test (%d)"), LettiFuoriDaiTest), LettiFuoriDaiTest > 50);
	TestTrue(*FString::Printf(TEXT("e file del modulo Editor (%d)"), LettiDellEditor), LettiDellEditor > 50);
	TestTrue(*FString::Printf(TEXT("e il piano vi si legge ancora con `->` (%d file)"), LettureDelPiano), LettureDelPiano > 1);
	// E l'accesso riservato e gli header dei test i test li usano davvero: i controlli che li vietano al resto guardano
	// testo vivo.
	TestTrue(*FString::Printf(TEXT("e l'accesso riservato vi si usa (%d file di test)"), TestCheUsanoLAccesso), TestCheUsanoLAccesso > 1);
	TestTrue(*FString::Printf(TEXT("e gli header dei test vi si includono (%d file di test)"), TestCheIncludonoITest), TestCheIncludonoITest > 1);
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
		// La seconda review: spazi intorno al nome del metodo, e il codice dopo la chiusura su una riga che comincia con `* `.
		{ TEXT("\tUnit->PlannedWaypoints.Add (Cell);"), true },
		{ TEXT("\tUnit->PlannedWaypoints. Add(Cell);"), true },
		{ TEXT("\t * nota */ Unit->PlannedCell = Cell;"), true },
		{ TEXT("\t * nota */"), false },
		{ TEXT("\tconst int32 N2 = Unit->PlannedWaypoints.Num ();"), false },
		{ TEXT("\tUnit->PlannedWaypoints.Reserve(8); // la capacita', non il contenuto"), false },
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

	// Ogni voce degli elenchi del rilevatore ha la sua riga piantata, SCRITTA A MANO: ricavarla dalla voce non
	// proverebbe niente, perche' un refuso nella voce starebbe anche nella riga. Con la riga scritta a parte, un refuso
	// nell'elenco lascia la riga non vista, e una voce nuova senza riga fa sbagliare il conteggio.
	const TCHAR* const MutazioniPiantate[] = {
		TEXT("\tUnit->PlannedWaypoints.Add(C);"), TEXT("\tUnit->PlannedWaypoints.Add_GetRef(C);"),
		TEXT("\tUnit->PlannedWaypoints.AddDefaulted();"), TEXT("\tUnit->PlannedWaypoints.AddDefaulted_GetRef();"),
		TEXT("\tUnit->PlannedWaypoints.AddUninitialized(1);"), TEXT("\tUnit->PlannedWaypoints.AddUnique(C);"),
		TEXT("\tUnit->PlannedWaypoints.AddZeroed(1);"), TEXT("\tUnit->PlannedWaypoints.AddZeroed_GetRef();"),
		TEXT("\tUnit->PlannedWaypoints.Append(Altri);"), TEXT("\tUnit->PlannedWaypoints.Emplace(C);"),
		TEXT("\tUnit->PlannedWaypoints.Emplace_GetRef(C);"), TEXT("\tUnit->PlannedWaypoints.EmplaceAt(0, C);"),
		TEXT("\tUnit->PlannedWaypoints.EmplaceAt_GetRef(0, C);"), TEXT("\tUnit->PlannedWaypoints.Empty();"),
		TEXT("\tUnit->PlannedWaypoints.Heapify();"), TEXT("\tUnit->PlannedWaypoints.HeapPop(C);"),
		TEXT("\tUnit->PlannedWaypoints.HeapPopDiscard();"), TEXT("\tUnit->PlannedWaypoints.HeapPush(C);"),
		TEXT("\tUnit->PlannedWaypoints.HeapRemoveAt(0);"), TEXT("\tUnit->PlannedWaypoints.HeapSort();"),
		TEXT("\tUnit->PlannedWaypoints.Init(C, 2);"), TEXT("\tUnit->PlannedWaypoints.Insert(C, 0);"),
		TEXT("\tUnit->PlannedWaypoints.Insert_GetRef(C, 0);"), TEXT("\tUnit->PlannedWaypoints.InsertDefaulted(0);"),
		TEXT("\tUnit->PlannedWaypoints.InsertDefaulted_GetRef(0);"), TEXT("\tUnit->PlannedWaypoints.InsertUninitialized(0);"),
		TEXT("\tUnit->PlannedWaypoints.InsertZeroed(0);"), TEXT("\tUnit->PlannedWaypoints.InsertZeroed_GetRef(0);"),
		TEXT("\tUnit->PlannedWaypoints.Pop();"), TEXT("\tUnit->PlannedWaypoints.Push(C);"),
		TEXT("\tUnit->PlannedWaypoints.Remove(C);"), TEXT("\tUnit->PlannedWaypoints.RemoveAll(Pred);"),
		TEXT("\tUnit->PlannedWaypoints.RemoveAllSwap(Pred);"), TEXT("\tUnit->PlannedWaypoints.RemoveAt(0);"),
		TEXT("\tUnit->PlannedWaypoints.RemoveAtSwap(0);"), TEXT("\tUnit->PlannedWaypoints.RemoveSingle(C);"),
		TEXT("\tUnit->PlannedWaypoints.RemoveSingleSwap(C);"), TEXT("\tUnit->PlannedWaypoints.RemoveSwap(C);"),
		TEXT("\tUnit->PlannedWaypoints.Reset();"), TEXT("\tUnit->PlannedWaypoints.SetNum(2);"),
		TEXT("\tUnit->PlannedWaypoints.SetNumUninitialized(2);"), TEXT("\tUnit->PlannedWaypoints.SetNumZeroed(2);"),
		TEXT("\tUnit->PlannedWaypoints.Shrink();"), TEXT("\tUnit->PlannedWaypoints.Sort();"),
		TEXT("\tUnit->PlannedWaypoints.StableSort();"), TEXT("\tUnit->PlannedWaypoints.Swap(0, 1);"),
		TEXT("\tUnit->PlannedWaypoints.SwapMemory(0, 1);"),
	};
	const TCHAR* const CompostiPiantati[] = {
		TEXT("\tUnit->PlannedAbilityIndex += 1;"), TEXT("\tUnit->PlannedAbilityIndex -= 1;"), TEXT("\tUnit->PlannedAbilityIndex *= 2;"),
		TEXT("\tUnit->PlannedAbilityIndex /= 2;"), TEXT("\tUnit->PlannedAbilityIndex %= 2;"), TEXT("\tUnit->PlannedAbilityIndex &= 1;"),
		TEXT("\tUnit->PlannedAbilityIndex |= 1;"), TEXT("\tUnit->PlannedAbilityIndex ^= 1;"), TEXT("\tUnit->PlannedAbilityIndex <<= 1;"),
		TEXT("\tUnit->PlannedAbilityIndex >>= 1;"),
	};
	const TCHAR* const IncrementiPiantati[] = {
		TEXT("\tUnit->PlannedAbilityIndex++;"), TEXT("\tUnit->PlannedAbilityIndex--;"),
		TEXT("\t++Unit->PlannedAbilityIndex;"), TEXT("\t--Unit->PlannedAbilityIndex;"),
	};
	TestEqual(TEXT("ogni mutazione dell'elenco ha la sua riga piantata"),
		static_cast<int32>(UE_ARRAY_COUNT(MutazioniPiantate)), static_cast<int32>(UE_ARRAY_COUNT(PlanWritersMutazioni)));
	TestEqual(TEXT("ogni operatore composto dell'elenco ha la sua riga piantata"),
		static_cast<int32>(UE_ARRAY_COUNT(CompostiPiantati)), static_cast<int32>(UE_ARRAY_COUNT(PlanWritersComposti)));
	for (const TCHAR* Riga : MutazioniPiantate)
	{
		TestTrue(*FString::Printf(TEXT("mutazione vista: `%s`"), *FString(Riga).TrimStart()), PlanWritersLineWrites(Riga));
	}
	for (const TCHAR* Riga : CompostiPiantati)
	{
		TestTrue(*FString::Printf(TEXT("operatore visto: `%s`"), *FString(Riga).TrimStart()), PlanWritersLineWrites(Riga));
	}
	for (const TCHAR* Riga : IncrementiPiantati)
	{
		TestTrue(*FString::Printf(TEXT("incremento visto: `%s`"), *FString(Riga).TrimStart()), PlanWritersLineWrites(Riga));
	}

	// L'uso dell'accesso riservato: nel codice si', citato in un commento o in una stringa no.
	const TArray<int32> Usi = PlanWritersFindTestAccess(FString(
		TEXT("\tFRTUnitPlanTestAccess::SetPath(*U, X);\n")
		TEXT("\t// FRTUnitPlanTestAccess, citato\n")
		TEXT("\tTestTrue(TEXT(\"FRTUnitPlanTestAccess\"), bOk);\n")));
	TestTrue(*FString::Printf(TEXT("l'uso dell'accesso si vede solo nel codice (righe viste: %d)"), Usi.Num()),
		Usi.Num() == 1 && Usi[0] == 1);

	// Gli header dei test inclusi: le righe 1 e 4 si', le altre no.
	const TArray<int32> Include = PlanWritersFindTestsInclude(FString(
		TEXT("#include \"Tests/Shared/RTUnitPlanTestAccess.h\"\n")
		TEXT("#include \"Unit/RTUnit.h\"\n")
		TEXT("// #include \"Tests/Shared/RTScenarioTestSupport.h\"\n")
		TEXT("\t#include \"Lab/Tests/RTLabFixture.h\"\n")
		TEXT("#include \"Turn/RTTurnTestsHelpers.h\"\n")));
	TestTrue(*FString::Printf(TEXT("gli include dei test si vedono, gli altri no (righe viste: %d)"), Include.Num()),
		Include.Num() == 2 && Include[0] == 1 && Include[1] == 4);

	// La classificazione dei file: sul percorso relativo al modulo.
	TestTrue(TEXT("`Tests/...` e' un file dei test"), PlanWritersIsTestFile(TEXT("Tests/Unit/RTUnitPlanWritersTests.cpp")));
	TestTrue(TEXT("`Private/Tests/...` e' un file dei test"), PlanWritersIsTestFile(TEXT("Private/Tests/Lab/RTLabViewModelTests.cpp")));
	TestFalse(TEXT("`Player/...` non e' un file dei test"), PlanWritersIsTestFile(TEXT("Player/RTPlayerController.cpp")));
	TestFalse(TEXT("`TestsHelpers/...` non e' una cartella dei test"), PlanWritersIsTestFile(TEXT("TestsHelpers/Foo.cpp")));
	return true;
}

namespace
{
	/**
	 * Il nome dichiarato da una riga di `RTUnit.h`, se la riga dichiara un campo: `Tipo Nome;`, `Tipo Nome = valore;`,
	 * `Tipo Nome{};`, `uint8 bNome : 1;`, `int32 Nome[3];`, anche dopo `public:` o una macro sulla stessa riga
	 * (`UPROPERTY(...)`, `UE_DEPRECATED(...)`). La testa e' cio' che precede l'inizializzatore, la larghezza del bitfield o
	 * la dimensione: le parentesi dell'inizializzatore non dicono niente, quelle della testa fuori dai `<...>` dicono
	 * «funzione» (`TFunction<void()> Nome;` e' un campo). Un'assegnazione dentro un mutatore (`PlannedCell = At;`) non ha
	 * un tipo davanti, e non conta.
	 */
	FString PlanWritersDeclaredField(const FString& RigaIntera)
	{
		FString Riga = PlanWritersLineCode(RigaIntera).TrimStartAndEnd();
		for (const TCHAR* Accesso : { TEXT("public:"), TEXT("protected:"), TEXT("private:") })
		{
			if (Riga.StartsWith(Accesso, ESearchCase::CaseSensitive)) { Riga = Riga.Mid(FCString::Strlen(Accesso)).TrimStart(); }
		}
		// Le macro in maiuscolo con i loro argomenti, anche con lo spazio prima della parentesi.
		while (true)
		{
			int32 M = 0;
			while (M < Riga.Len() && (FChar::IsUpper(Riga[M]) || (M > 0 && (FChar::IsDigit(Riga[M]) || Riga[M] == TEXT('_'))))) { ++M; }
			int32 P = M;
			while (P < Riga.Len() && FChar::IsWhitespace(Riga[P])) { ++P; }
			if (M < 2 || P >= Riga.Len() || Riga[P] != TEXT('(')) { break; }
			int32 Prof = 0;
			int32 Fine = INDEX_NONE;
			for (int32 I = P; I < Riga.Len(); ++I)
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
			if (C == TEXT('=') || C == TEXT('{') || C == TEXT('[') || bDuePunti)
			{
				Testa = Testa.Left(I);
				break;
			}
		}
		int32 Angoli = 0;
		for (const TCHAR C : Testa)
		{
			if (C == TEXT('<')) { ++Angoli; }
			else if (C == TEXT('>')) { --Angoli; }
			else if ((C == TEXT('(') || C == TEXT('}')) && Angoli <= 0) { return FString(); }
		}
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
		// La seconda review: forme che il parser perdeva, e due che non sono campi.
		{ TEXT("public: int32 PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tUE_DEPRECATED(5.8, \"usa altro\") int32 PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tUPROPERTY (VisibleAnywhere) int32 PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tTFunction<void()> PlannedFoo;"), TEXT("PlannedFoo") },
		{ TEXT("\tint32 PlannedFoo[3];"), TEXT("PlannedFoo") },
		{ TEXT("\tTFunction<void()> GetPlannedFoo() const;"), TEXT("") },
		{ TEXT("\tUFUNCTION(BlueprintCallable) void SetPlannedFoo(int32 X);"), TEXT("") },
		{ TEXT("\t * FRTCellId PlannedFoo;"), TEXT("") },
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
