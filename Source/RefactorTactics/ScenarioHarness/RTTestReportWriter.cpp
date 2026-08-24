#include "ScenarioHarness/RTTestReportWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/EngineVersion.h"

namespace
{
	/**
	 * Nome dell'assertion nel report, preso dall'ENUM invece che da uno switch scritto a mano.
	 *
	 * Lo switch precedente elencava due casi su cinque e restituiva `Unknown` per gli altri tre: un report che
	 * diceva «Unknown» per ogni `UnitHpEquals` caduto. Una tabella scritta a mano diverge dall'enum che
	 * descrive appena qualcuno aggiunge un valore — ed e' successo — quindi qui non ce n'e' piu' una.
	 */
	FString KindToString(ERTAssertionKind Kind)
	{
		const UEnum* Enum = StaticEnum<ERTAssertionKind>();
		const FString Name = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Kind)) : FString();
		return Name.IsEmpty() ? FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(Kind)) : Name;
	}
}

FString URTTestReportWriter::RunsRoot()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RTTests"));
}

FString URTTestReportWriter::ToJson(const FRTTestResult& Result, const FString& RunId)
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	Root->SetNumberField(TEXT("schemaVersion"), SchemaVersion);
	Root->SetStringField(TEXT("scenario"), Result.ScenarioId);
	Root->SetStringField(TEXT("runId"), RunId);
	Root->SetStringField(TEXT("result"), Result.OutcomeString());
	Root->SetStringField(TEXT("engineVersion"),
		FEngineVersion::Current().ToString(EVersionComponent::Patch));

	// Il seed viaggia nel report anche se oggi nessun RNG lo consuma: quando un RNG entrera' nel resolver,
	// i report vecchi diranno gia' con quale seed erano stati prodotti.
	Root->SetNumberField(TEXT("seed"), Result.Seed);
	Root->SetNumberField(TEXT("turnsPlayed"), Result.TurnsPlayed);

	// Le finestre di reazione (#512): CHI ha risposto, e quante decisioni scriptate sono state applicate o
	// sono rimaste inutilizzate. La provenienza sta nel referto e non nel TurnLog perche' al replay serve
	// **quale** decisione, non chi l'ha fornita — un campo nuovo in `FRTTurnLogEntry` muoverebbe i golden
	// per un dato che il replay non legge.
	//
	// ⚠️ `SchemaVersion` NON viene alzata, e il piano diceva di alzarla: la regola scritta accanto alla
	// costante e' «a ogni cambio **non retrocompatibile**», e tre campi aggiunti sono additivi — un lettore
	// vecchio li ignora e continua a leggere tutto il resto. Alzarla qui renderebbe la costante un contatore
	// di modifiche invece del segnale di rottura che dichiara di essere.
	Root->SetStringField(TEXT("decisionSource"), Result.DecisionSource);
	Root->SetNumberField(TEXT("scriptedDecisionsApplied"), Result.ScriptedDecisionsApplied);
	Root->SetNumberField(TEXT("scriptedDecisionsUnused"), Result.ScriptedDecisionsUnused);

	// Esadecimale, non decimale: un hash si confronta a occhio fra due report, e in esadecimale la differenza
	// si vede alla prima cifra invece che contando le posizioni.
	Root->SetStringField(TEXT("stateHash"), FString::Printf(TEXT("%08x"), Result.StateHash));

	if (!Result.ErrorMessage.IsEmpty())
	{
		// Presente SOLO negli ERROR: la sua presenza distingue «non ho potuto eseguire» da «ho eseguito e non torna».
		Root->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	if (!Result.BlockedReason.IsEmpty())
	{
		// Sta accanto a `error` e non al suo posto: chi legge il report deve poter distinguere «il test e'
		// rotto» da «la feature non c'e' ancora» senza interpretare l'esito.
		Root->SetStringField(TEXT("blockedReason"), Result.BlockedReason);
	}

	if (Result.Notes.Num() > 0)
	{
		// Ne' `error` ne' `blockedReason`: le note non dicono chi ha sbagliato, dicono cosa e' successo. Un
		// intent che non e' partito perche' il bersaglio era gia' a terra spiega l'assertion che cade dopo —
		// senza, resta un «non ha attaccato» senza motivo, e il motivo si cerca nel log del motore.
		TArray<TSharedPtr<FJsonValue>> NoteValues;
		for (const FString& Note : Result.Notes)
		{
			NoteValues.Add(MakeShared<FJsonValueString>(Note));
		}
		Root->SetArrayField(TEXT("notes"), NoteValues);
	}

	const TSharedRef<FJsonObject> Counts = MakeShared<FJsonObject>();
	Counts->SetNumberField(TEXT("passed"), Result.PassedCount());
	Counts->SetNumberField(TEXT("failed"), Result.FailedCount());
	Root->SetObjectField(TEXT("assertions"), Counts);

	// Le assertion FALLITE per prime e per intero: sono il motivo per cui si apre questo file.
	TArray<TSharedPtr<FJsonValue>> Failures;
	TArray<TSharedPtr<FJsonValue>> All;
	for (const FRTAssertionResult& A : Result.Assertions)
	{
		const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("assertion"), KindToString(A.Kind));
		Obj->SetStringField(TEXT("description"), A.Description);
		Obj->SetBoolField(TEXT("passed"), A.bPassed);
		Obj->SetStringField(TEXT("expected"), A.Expected);
		Obj->SetStringField(TEXT("actual"), A.Actual);
		Obj->SetNumberField(TEXT("turn"), A.Turn);

		All.Add(MakeShared<FJsonValueObject>(Obj));
		if (!A.bPassed)
		{
			Failures.Add(MakeShared<FJsonValueObject>(Obj));
		}
	}
	Root->SetArrayField(TEXT("failures"), Failures);
	Root->SetArrayField(TEXT("details"), All);

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool URTTestReportWriter::Write(const FRTTestResult& Result, const FString& RunId,
	FString& OutDirectory, FString& OutError)
{
	OutError.Reset();

	// Il RunId e' cronologico per costruzione, cosi' «l'ultima run» e' l'ultima in ordine alfabetico:
	// nessun indice da mantenere, nessun file di stato che puo' disallinearsi.
	const FString EffectiveRunId = RunId.IsEmpty()
		? FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"))
		: RunId;

	OutDirectory = FPaths::Combine(RunsRoot(), Result.ScenarioId, EffectiveRunId);
	if (!IFileManager::Get().MakeDirectory(*OutDirectory, /*Tree=*/ true))
	{
		OutError = FString::Printf(TEXT("impossibile creare la cartella del report: %s"), *OutDirectory);
		return false;
	}

	const FString Path = FPaths::Combine(OutDirectory, TEXT("result.json"));
	if (!FFileHelper::SaveStringToFile(ToJson(Result, EffectiveRunId), *Path))
	{
		OutError = FString::Printf(TEXT("impossibile scrivere %s"), *Path);
		return false;
	}
	return true;
}

FString URTTestReportWriter::FindLatestRunDirectory(const FString& ScenarioId)
{
	// 🔴 **Ordinava per NOME e prendeva l'ultimo, e non e' la stessa cosa che «la piu' recente»** (#1154).
	// Fra soli nomi `YYYYMMDD-hhmmss` i due ordini coincidono — ed e' per questo che il difetto e' vissuto
	// invisibile — ma basta UNA cartella con un nome diverso per romperli: in ASCII `s` (0x73) viene dopo
	// `2` (0x32), quindi `selftest` era l'ultima per costruzione e lo sarebbe rimasta per qualunque
	// timestamp futuro. Osservato in PIE il 2026-08-17: `rt.Test.Run` scriveva in `20260817-172722` e
	// `rt.Test.DumpResult` stampava `selftest`, un report di due giorni prima.
	//
	// ⚠️ **Non e' diagnostica assente: e' diagnostica che MENTE.** Il contenuto di `selftest` era identico a
	// quello del run appena eseguito, quindi chi confrontava il JSON concludeva che il comando funzionasse;
	// al secondo run della stessa sessione avrebbe mostrato un valore diverso da quello reale, spacciandolo
	// per l'ultimo.
	//
	// ⛔ **Cancellare `selftest` non era il rimedio**: e' il primo che viene in mente e nasconde il difetto
	// lasciandolo intatto — la prossima cartella non-timestamp (`latest`, `baseline`, un nome a mano) lo
	// farebbe riemergere identico, e senza nessuno che se ne accorga.
	const FString ScenarioDir = FPaths::Combine(RunsRoot(), ScenarioId);
	TArray<FString> Runs;
	IFileManager::Get().FindFiles(Runs, *(ScenarioDir / TEXT("*")), /*Files=*/ false, /*Directories=*/ true);

	// La recenza si misura sul `result.json`, non sulla cartella: e' il file che la run produce, ed e' quello
	// che `DumpResult` andra' a leggere. Una cartella senza report non e' una run leggibile e resta fuori —
	// stampare un percorso che non contiene niente sarebbe la stessa diagnostica bugiarda in un'altra forma.
	FString Migliore;
	FDateTime QuandoMigliore = FDateTime::MinValue();
	for (const FString& Run : Runs)
	{
		const FString Report = FPaths::Combine(ScenarioDir, Run, TEXT("result.json"));
		const FDateTime Quando = IFileManager::Get().GetTimeStamp(*Report);
		if (Quando == FDateTime::MinValue())
		{
			continue; // niente report: non e' una run che si possa stampare
		}
		// ⚠️ A parita' di istante vince il nome piu' alto, e la riga serve: la granularita' del timestamp del
		// filesystem non e' infinita, e due run nello stesso secondo lascerebbero l'esito all'ordine di
		// enumerazione di `FindFiles` — cioe' non deterministico.
		if (Migliore.IsEmpty() || Quando > QuandoMigliore || (Quando == QuandoMigliore && Run > Migliore))
		{
			Migliore = Run;
			QuandoMigliore = Quando;
		}
	}
	return Migliore.IsEmpty() ? FString() : FPaths::Combine(ScenarioDir, Migliore);
}
