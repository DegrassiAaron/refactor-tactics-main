#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Turn/RTPacingConsole.h"

// La guardia: senza, questi test finiscono nel binario Shipping. Vedi `#923`.
#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	ERTRecordPacingRequest ParseOne(const FString& Arg)
	{
		TArray<FString> Args;
		Args.Add(Arg);
		return URTPacingConsoleLibrary::ParseArgs(Args);
	}

	FString Joined(const TArray<FString>& Lines) { return FString::Join(Lines, TEXT(" | ")); }
}

/**
 * UN ARGOMENTO ILLEGGIBILE E' UN RIFIUTO, NON UNO SPENTO — `#3398`.
 *
 * 🔑 **E' l'unico modo in cui questo comando puo' far perdere una misura.** Chi esegue `PIE-PACING-1`
 * digita `rt.Debug.RecordPacing 1` e poi gioca un turno intero; se un refuso venisse letto come «spento»
 * — o peggio come «acceso» — se ne accorgerebbe solo dopo, cercando righe in un CSV che non esiste o
 * fidandosi di uno stato che non e' quello. Il turno non si rigioca uguale.
 *
 * ⚠️ **`Invalid` e `Disable` sono asseriti separatamente e non «non-Enable»**: un predicato che li
 * confonde passerebbe anche con l'implementazione sbagliata, che e' precisamente quella che questo test
 * esiste per escludere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingConsoleRejectsGarbageTest,
	"RefactorTactics.Debug.PacingConsoleRejectsGarbage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPacingConsoleRejectsGarbageTest::RunTest(const FString& /*Parameters*/)
{
	// Nessun argomento: e' una domanda, non un comando.
	TestTrue(TEXT("nessun argomento -> Query"),
		URTPacingConsoleLibrary::ParseArgs(TArray<FString>()) == ERTRecordPacingRequest::Query);

	for (const TCHAR* Yes : { TEXT("1"), TEXT("true"), TEXT("on"), TEXT("si"), TEXT("TRUE"), TEXT(" 1 ") })
	{
		TestTrue(FString::Printf(TEXT("'%s' -> Enable"), Yes),
			ParseOne(Yes) == ERTRecordPacingRequest::Enable);
	}
	for (const TCHAR* No : { TEXT("0"), TEXT("false"), TEXT("off"), TEXT("no"), TEXT("Off") })
	{
		TestTrue(FString::Printf(TEXT("'%s' -> Disable"), No),
			ParseOne(No) == ERTRecordPacingRequest::Disable);
	}

	// ⚠️ `tru` e `2` sono i due refusi plausibili: uno e' un prefisso di `true`, l'altro un numero che
	// non e' ne' 0 ne' 1. Un parser scritto con `FCString::Atoi` leggerebbe `tru` come 0 — cioe' come
	// «spegni» — senza dire niente.
	for (const TCHAR* Bad : { TEXT("tru"), TEXT("2"), TEXT("-1"), TEXT(""), TEXT("csv"), TEXT("yes") })
	{
		TestTrue(FString::Printf(TEXT("'%s' -> Invalid"), Bad),
			ParseOne(Bad) == ERTRecordPacingRequest::Invalid);
	}

	// Due argomenti: nessuna lettura ovvia, quindi si rifiuta.
	{
		TArray<FString> Due;
		Due.Add(TEXT("1"));
		Due.Add(TEXT("0"));
		TestTrue(TEXT("due argomenti -> Invalid"),
			URTPacingConsoleLibrary::ParseArgs(Due) == ERTRecordPacingRequest::Invalid);
	}

	return true;
}

/**
 * LA RESA DICE SEMPRE CHE I TURNI GIA' CHIUSI NON ENTRANO NEL CSV — `#3398`.
 *
 * 🔑 **Perche' il flag e' letto da `FRTPacingRecorder::Close`, non all'apertura del campione.** Armare a
 * partita iniziata produce un CSV **piu' corto della partita**, e `PIE-PACING-1` chiede *«una riga per
 * turno giocato»*: senza questa riga chi esegue confronterebbe due numeri diversi credendoli lo stesso.
 *
 * ⚠️ **E un rifiuto non deve nominare il CSV.** Se `Describe` stampasse il percorso anche su `Invalid`,
 * un refuso avrebbe l'aspetto di un successo — ed e' il difetto peggiore qui, perche' la riga di conferma
 * e' l'unica cosa che chi esegue guarda prima di iniziare a giocare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingConsoleDescribesTheGapTest,
	"RefactorTactics.Debug.PacingConsoleDescribesTheGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPacingConsoleDescribesTheGapTest::RunTest(const FString& /*Parameters*/)
{
	const FString Dir = TEXT("D:/x/Saved/RT");

	// Armato da spento: deve dire dove nascera' il file E che i turni gia' chiusi restano fuori.
	{
		const FString T = Joined(URTPacingConsoleLibrary::Describe(
			ERTRecordPacingRequest::Enable, /*bWasEnabled=*/ false, Dir, FString()));
		TestTrue(TEXT("armando, nomina la cartella"), T.Contains(Dir));
		TestTrue(TEXT("armando, avverte sui turni gia' chiusi"), T.Contains(TEXT("gia' conclusi")));
		TestTrue(TEXT("armando, dichiara il passaggio di stato"), T.Contains(TEXT("ATTIVA")));
	}

	// Se il file esiste gia', si nomina QUELLO: e' cio' che si allega come evidenza.
	{
		const FString Path = TEXT("D:/x/Saved/RT/pacing_20260928-101500.csv");
		const FString T = Joined(URTPacingConsoleLibrary::Describe(
			ERTRecordPacingRequest::Query, /*bWasEnabled=*/ true, Dir, Path));
		TestTrue(TEXT("con file esistente, lo nomina"), T.Contains(Path));
	}

	// Spento: il comando deve dire che i campioni NON si perdono, altrimenti sembra che disarmare
	// spenga la telemetria — e non e' vero, `rt.Debug.Pacing` continua a rispondere.
	{
		const FString T = Joined(URTPacingConsoleLibrary::Describe(
			ERTRecordPacingRequest::Disable, /*bWasEnabled=*/ true, Dir, FString()));
		TestTrue(TEXT("disarmando, rimanda a rt.Debug.Pacing"), T.Contains(TEXT("rt.Debug.Pacing")));
	}

	// Rifiuto: nessun percorso, nessuna parola che somigli a una conferma.
	{
		const FString T = Joined(URTPacingConsoleLibrary::Describe(
			ERTRecordPacingRequest::Invalid, /*bWasEnabled=*/ false, Dir, FString()));
		TestFalse(TEXT("il rifiuto non nomina la cartella"), T.Contains(Dir));
		TestTrue(TEXT("il rifiuto dichiara che nulla e' cambiato"), T.Contains(TEXT("nulla e' stato cambiato")));
		TestTrue(TEXT("il rifiuto mostra l'uso"), T.Contains(TEXT("rt.Debug.RecordPacing [1|0]")));
	}

	return true;
}

/**
 * IL COMANDO E' REGISTRATO COL NOME CHE LA DOCUMENTAZIONE PROMETTE — `#3398`.
 *
 * ⚠️ **Il controllo positivo sta accanto**: `rt.Debug.Pacing` deve essere trovato dalla stessa chiamata.
 * Senza, un `FindConsoleObject` che restituisse `nullptr` per un guasto del registro renderebbe questo
 * test verde-per-assenza invece che rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingConsoleIsRegisteredTest,
	"RefactorTactics.Debug.PacingConsoleIsRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPacingConsoleIsRegisteredTest::RunTest(const FString& /*Parameters*/)
{
	TestNotNull(TEXT("rt.Debug.RecordPacing e' registrato"),
		IConsoleManager::Get().FindConsoleObject(TEXT("rt.Debug.RecordPacing")));
	TestNotNull(TEXT("controllo positivo: rt.Debug.Pacing e' registrato"),
		IConsoleManager::Get().FindConsoleObject(TEXT("rt.Debug.Pacing")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
