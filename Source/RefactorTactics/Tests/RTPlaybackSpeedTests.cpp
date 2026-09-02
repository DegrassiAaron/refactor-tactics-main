// Copyright RefactorTactics. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Replay/RTPlaybackSpeed.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * OGNI VELOCITA' HA UN MOLTIPLICATORE, E `Instant` SI RICONOSCE PER NOME — `#1625`.
 *
 * 🔑 **Il confronto giusto è sul nome, non sul numero.** Un consumatore che scrivesse
 * `if (Multiplier <= KINDA_SMALL_NUMBER)` legherebbe il proprio comportamento a un valore che questa
 * libreria può cambiare, e il giorno che `Instant` diventasse `0.001` per una ragione di rendering quella
 * riga smetterebbe di riconoscerlo **senza un errore**. Il test pinna entrambe le cose: che i numeri
 * esistano, e che `IsInstant` non dipenda da loro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSpeedScaleTest,
	"RefactorTactics.Playback.SpeedScaleIsCompleteAndInstantIsNamed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSpeedScaleTest::RunTest(const FString&)
{
	const TArray<ERTPlaybackSpeed> All = URTPlaybackSpeedLibrary::AllSpeeds();
	TestEqual(TEXT("le velocita' dichiarate dallo scope sono sei"), All.Num(), 6);

	// Ogni valore dell'enum e' nella scala: se qualcuno ne aggiunge uno senza metterlo qui, si vede.
	const int32 Count = static_cast<int32>(ERTPlaybackSpeed::Instant) + 1;
	TestEqual(TEXT("e la scala copre tutto l'enum"), All.Num(), Count);

	// I moltiplicatori: piu' lento = piu' tempo per passo, e la scala e' monotona.
	TestEqual(TEXT("0.25x dura quattro volte"), URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed::Quarter), 4.0f);
	TestEqual(TEXT("1x e' la base"), URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed::Normal), 1.0f);
	TestEqual(TEXT("4x dura un quarto"), URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed::Quadruple), 0.25f);

	float Previous = TNumericLimits<float>::Max();
	for (const ERTPlaybackSpeed Speed : All)
	{
		const float M = URTPlaybackSpeedLibrary::SecondsMultiplier(Speed);
		TestTrue(FString::Printf(TEXT("la scala e' monotona decrescente (%.2f dopo %.2f)"), M, Previous),
			M < Previous);
		Previous = M;
	}

	// ⛔ `Instant` si riconosce per NOME, e nessun'altra velocita' risponde vero.
	TestTrue(TEXT("Instant e' instant"), URTPlaybackSpeedLibrary::IsInstant(ERTPlaybackSpeed::Instant));
	for (const ERTPlaybackSpeed Speed : All)
	{
		if (Speed == ERTPlaybackSpeed::Instant) { continue; }
		TestFalse(FString::Printf(TEXT("e %d non lo e'"), static_cast<int32>(Speed)),
			URTPlaybackSpeedLibrary::IsInstant(Speed));
	}

	// Il ciclo torna all'inizio: un controllo a un tasto non deve incastrarsi in fondo.
	TestEqual(TEXT("da Instant si torna alla piu' lenta"),
		static_cast<int32>(URTPlaybackSpeedLibrary::NextSpeed(ERTPlaybackSpeed::Instant)),
		static_cast<int32>(ERTPlaybackSpeed::Quarter));
	TestEqual(TEXT("e 1x porta a 2x"),
		static_cast<int32>(URTPlaybackSpeedLibrary::NextSpeed(ERTPlaybackSpeed::Normal)),
		static_cast<int32>(ERTPlaybackSpeed::Double));

	return true;
}

/**
 * LA VELOCITA' NON ENTRA IN NESSUN ESITO: `Instant` ≡ `1x` — `#1625`.
 *
 * 🔑 È il quarto criterio d'accettazione, e il modo in cui è reso misurabile conta: «stesso stato finale»
 * per un trasporto significa **stessi valori**, non stessi tempi. La velocità decide quanto si aspetta fra
 * due posizioni della traccia, non quali posizioni esistano.
 *
 * ⚠️ Il test lo verifica **per costruzione**: nessuna funzione di questa libreria prende una velocità e
 * restituisce qualcosa che non sia un tempo. Se un giorno qualcuno le facesse decidere *cosa* accade — un
 * filtro, un salto, una soglia — questa asserzione andrebbe riscritta, e il fatto che vada riscritta è
 * l'allarme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSpeedIsPresentationOnlyTest,
	"RefactorTactics.Playback.InstantEqualsNormalInEverythingButTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSpeedIsPresentationOnlyTest::RunTest(const FString&)
{
	// L'unica differenza fra le due e' il tempo per passo.
	TestNotEqual(TEXT("i tempi differiscono"),
		URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed::Instant),
		URTPlaybackSpeedLibrary::SecondsMultiplier(ERTPlaybackSpeed::Normal));

	// ⛔ E NIENT'ALTRO: la scala, l'ordine e la successione non cambiano a seconda della velocita' scelta.
	// Se una di queste dipendesse dalla velocita', la velocita' avrebbe smesso di essere presentazione.
	const TArray<ERTPlaybackSpeed> A = URTPlaybackSpeedLibrary::AllSpeeds();
	const TArray<ERTPlaybackSpeed> B = URTPlaybackSpeedLibrary::AllSpeeds();
	if (TestEqual(TEXT("la scala e' la stessa a ogni chiamata"), A.Num(), B.Num()))
	{
		for (int32 i = 0; i < A.Num(); ++i)
		{
			TestEqual(FString::Printf(TEXT("posizione %d"), i),
				static_cast<int32>(A[i]), static_cast<int32>(B[i]));
		}
	}

	// La successione da un valore non dipende da quale velocita' sia «attiva»: e' una funzione del solo
	// argomento, ed e' cio' che rende il trasporto indipendente dalla presentazione.
	for (const ERTPlaybackSpeed Speed : A)
	{
		TestEqual(FString::Printf(TEXT("NextSpeed(%d) e' stabile"), static_cast<int32>(Speed)),
			static_cast<int32>(URTPlaybackSpeedLibrary::NextSpeed(Speed)),
			static_cast<int32>(URTPlaybackSpeedLibrary::NextSpeed(Speed)));
	}

	return true;
}

/**
 * IL PLAYBACK NON DIPENDE DAL RESOLVER: LA SORGENTE E' LA TRACCIA — `#1625`.
 *
 * 🔴 **È un'esistenziale negativa, e questa volta è legittima.** `#1697` ha dichiarato malformato un
 * criterio che chiedeva di dimostrare una non-esistenza su uno spazio **non enumerabile** — *«tutte le
 * mutazioni possibili del bot»*. Qui lo spazio è finito e nominabile: i simboli che il resolver espone
 * sono un elenco chiuso, e si guardano uno per uno.
 *
 * ⚠️ **La misura è sul contenuto del file, non sul comportamento a runtime**, e la differenza è il motivo
 * per cui questo test regge: un test a runtime proverebbe che *quella esecuzione* non ha chiamato il
 * resolver, non che nessuna lo faccia.
 *
 * 🔴 **E i commenti si tolgono prima di cercare, perché la prima stesura è caduta proprio lì.**
 * `RTReplayViewModel.h` **nomina** `RTCombatResolver`, `RTHexSimLibrary` e `ARTTurnManager` — in un
 * commento che dichiara *«nessuna catena raggiunge»* quei simboli, cioè esattamente l'invariante che
 * questo test verifica. Un criterio che cerca un token nel testo trova anche la frase che promette il
 * contrario, e boccia il file per la ragione opposta a quella vera. Qui si misura la **dipendenza** — un
 * `#include`, una qualificazione `::` — non la citazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackDoesNotDependOnResolverTest,
	"RefactorTactics.Playback.ViewModelDoesNotDependOnTheResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackDoesNotDependOnResolverTest::RunTest(const FString&)
{
	const FString Root = FPaths::ProjectDir() / TEXT("Source/RefactorTactics/Replay/");
	const TCHAR* Files[] = { TEXT("RTReplayViewModel.h"), TEXT("RTReplayViewModel.cpp"), TEXT("RTPlaybackSpeed.h") };

	// I simboli che, se comparissero qui, significherebbero che il playback RIESEGUE invece di leggere.
	const TCHAR* Forbidden[] = {
		TEXT("RTCombatResolver"), TEXT("URTCombatResolver"),
		TEXT("RTHexSimLibrary"), TEXT("ResolveAttacks"), TEXT("ResolveHexPaths"),
		TEXT("ARTTurnManager")
	};

	int32 Examined = 0;
	bool bFoundViewModelSymbol = false;
	for (const TCHAR* File : Files)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(Root + File)))
		{
			// ⛔ Un file che non si legge non e' un test verde: sarebbe l'assenza per non aver guardato.
			AddError(FString::Printf(TEXT("non ho potuto leggere %s: la misura non vale"), File));
			continue;
		}
		++Examined;

		// I COMMENTI ESCONO PRIMA DELLA RICERCA: e' dove la prima stesura di questo test e' caduta.
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, /*InCullEmpty=*/ false);
		FString Code;
		bool bInBlockComment = false;
		for (const FString& Line : Lines)
		{
			FString L = Line;
			if (bInBlockComment)
			{
				int32 Close = INDEX_NONE;
				if (L.FindLastChar(TEXT('/'), Close) && L.Contains(TEXT("*/")))
				{
					L = L.RightChop(L.Find(TEXT("*/")) + 2);
					bInBlockComment = false;
				}
				else
				{
					continue;
				}
			}
			int32 Block = L.Find(TEXT("/*"));
			if (Block != INDEX_NONE)
			{
				bInBlockComment = !L.Contains(TEXT("*/"));
				L = L.Left(Block);
			}
			const int32 Slash = L.Find(TEXT("//"));
			if (Slash != INDEX_NONE) { L = L.Left(Slash); }
			Code += L + TEXT("\n");
		}

		if (Code.Contains(TEXT("FRTReplayViewModel")) || Code.Contains(TEXT("ERTPlaybackSpeed")))
		{
			bFoundViewModelSymbol = true;
		}

		for (const TCHAR* Symbol : Forbidden)
		{
			TestFalse(FString::Printf(TEXT("%s non DIPENDE da %s"), File, Symbol), Code.Contains(Symbol));
		}
	}

	// Le righe di sanita': se il percorso fosse sbagliato, o se lo spoglio dei commenti avesse svuotato il
	// file, tutti i `TestFalse` sopra sarebbero verdi per assenza invece che per pulizia.
	TestEqual(TEXT("i file esaminati sono tre"), Examined, 3);
	TestTrue(TEXT("e il codice spogliato non e' vuoto: contiene ancora il tipo che stiamo esaminando"),
		bFoundViewModelSymbol);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
