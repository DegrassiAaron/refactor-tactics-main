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
	const TCHAR* KindToString(ERTAssertionKind Kind)
	{
		switch (Kind)
		{
		case ERTAssertionKind::UnitAtCell:     return TEXT("UnitAtCell");
		case ERTAssertionKind::TurnsCompleted: return TEXT("TurnsCompleted");
		default:                               return TEXT("Unknown");
		}
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

	// Esadecimale, non decimale: un hash si confronta a occhio fra due report, e in esadecimale la differenza
	// si vede alla prima cifra invece che contando le posizioni.
	Root->SetStringField(TEXT("stateHash"), FString::Printf(TEXT("%08x"), Result.StateHash));

	if (!Result.ErrorMessage.IsEmpty())
	{
		// Presente SOLO negli ERROR: la sua presenza distingue «non ho potuto eseguire» da «ho eseguito e non torna».
		Root->SetStringField(TEXT("error"), Result.ErrorMessage);
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
	const FString ScenarioDir = FPaths::Combine(RunsRoot(), ScenarioId);
	TArray<FString> Runs;
	IFileManager::Get().FindFiles(Runs, *(ScenarioDir / TEXT("*")), /*Files=*/ false, /*Directories=*/ true);
	if (Runs.Num() == 0)
	{
		return FString();
	}
	Runs.Sort();
	return FPaths::Combine(ScenarioDir, Runs.Last());
}
