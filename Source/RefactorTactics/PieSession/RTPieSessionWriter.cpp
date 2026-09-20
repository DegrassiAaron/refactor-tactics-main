#include "PieSession/RTPieSessionWriter.h"

#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const TCHAR* LexToString(ERTPieVerdict Verdict)
{
	switch (Verdict)
	{
	case ERTPieVerdict::Pass:         return TEXT("PASS");
	case ERTPieVerdict::Fail:         return TEXT("FAIL");
	case ERTPieVerdict::NotJudgeable: return TEXT("NOT_JUDGEABLE");
	case ERTPieVerdict::Blocked:      return TEXT("BLOCKED");
	case ERTPieVerdict::NotRun:       return TEXT("NOT_RUN");
	case ERTPieVerdict::Pending:      return TEXT("PENDING");
	}
	return TEXT("PENDING");
}

namespace
{
	/**
	 * I tre verdetti che non vengono da una persona devono portare un motivo. Se manca, il file lo dice:
	 * un `reason` vuoto su un `BLOCKED` sarebbe un buco silenzioso, e chi legge il file dopo non avrebbe
	 * modo di sapere se il motivo non c'era o se nessuno l'ha scritto.
	 */
	bool RTPieVerdettoRichiedeMotivo(ERTPieVerdict Verdict)
	{
		return Verdict == ERTPieVerdict::Blocked
			|| Verdict == ERTPieVerdict::NotRun
			|| Verdict == ERTPieVerdict::NotJudgeable;
	}
}

FString URTPieSessionWriter::MakeSessionId(const FDateTime& Now)
{
	return Now.ToString(TEXT("%Y%m%d-%H%M%S"));
}

FString URTPieSessionWriter::ReadHeadCommit()
{
	// Il `.git` sta accanto al `.uproject`: il progetto Unreal E' la radice del repository.
	const FString GitDir = FPaths::Combine(FPaths::ProjectDir(), TEXT(".git"));

	FString Head;
	if (!FFileHelper::LoadFileToString(Head, *FPaths::Combine(GitDir, TEXT("HEAD"))))
	{
		return FString();
	}
	Head.TrimStartAndEndInline();

	// Due forme: `ref: refs/heads/<branch>` su un branch, oppure gia' uno SHA in detached HEAD.
	const FString Prefisso = TEXT("ref: ");
	if (!Head.StartsWith(Prefisso))
	{
		return Head;
	}

	const FString RefPath = Head.RightChop(Prefisso.Len()).TrimStartAndEnd();
	FString Sha;
	if (FFileHelper::LoadFileToString(Sha, *FPaths::Combine(GitDir, RefPath)))
	{
		return Sha.TrimStartAndEnd();
	}

	// Il ref puo' essere impacchettato in `packed-refs` invece che avere un file suo.
	FString Packed;
	if (FFileHelper::LoadFileToString(Packed, *FPaths::Combine(GitDir, TEXT("packed-refs"))))
	{
		TArray<FString> Righe;
		Packed.ParseIntoArrayLines(Righe, /*CullEmpty=*/ true);
		for (const FString& Riga : Righe)
		{
			if (Riga.EndsWith(RefPath))
			{
				FString Sinistra, Destra;
				if (Riga.Split(TEXT(" "), &Sinistra, &Destra))
				{
					return Sinistra.TrimStartAndEnd();
				}
			}
		}
	}

	return FString();
}

FString URTPieSessionWriter::ToJson(const FString& SessionId, const FString& Commit,
	const TArray<FRTPieSessionStep>& Steps)
{
	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("sessionId"), SessionId);

	if (Commit.IsEmpty()) { Writer->WriteNull(TEXT("commit")); }
	else { Writer->WriteValue(TEXT("commit"), Commit); }

	Writer->WriteValue(TEXT("buildVersion"), FApp::GetBuildVersion());

	Writer->WriteArrayStart(TEXT("steps"));
	for (const FRTPieSessionStep& Step : Steps)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("pieItem"), Step.PieItem);

		if (Step.Criterion == INDEX_NONE) { Writer->WriteNull(TEXT("criterion")); }
		else { Writer->WriteValue(TEXT("criterion"), Step.Criterion); }

		Writer->WriteValue(TEXT("scenarioId"), Step.ScenarioId);
		Writer->WriteValue(TEXT("runId"), Step.RunId);
		Writer->WriteValue(TEXT("reportDir"), Step.ReportDir);

		// 🔑 Due campi, e nessuno dei due si deduce dall'altro: `expect` rosse con «a schermo si
		// capisce» e' una coppia che il progetto produce davvero.
		Writer->WriteValue(TEXT("machineOutcome"), Step.MachineOutcome);
		Writer->WriteValue(TEXT("verdict"), LexToString(Step.Verdict));

		if (RTPieVerdettoRichiedeMotivo(Step.Verdict))
		{
			Writer->WriteValue(TEXT("reason"),
				Step.Reason.IsEmpty() ? TEXT("(motivo non registrato)") : *Step.Reason);
		}
		else if (Step.Reason.IsEmpty()) { Writer->WriteNull(TEXT("reason")); }
		else { Writer->WriteValue(TEXT("reason"), Step.Reason); }

		Writer->WriteValue(TEXT("at"), Step.At.ToIso8601());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	Writer->WriteObjectEnd();
	Writer->Close();
	return Out;
}

bool URTPieSessionWriter::Write(const FString& SessionId, const FString& Commit,
	const TArray<FRTPieSessionStep>& Steps, FString& OutPath, FString& OutError)
{
	OutPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RTPieSessions"), SessionId,
		TEXT("session.json"));

	if (!FFileHelper::SaveStringToFile(ToJson(SessionId, Commit, Steps), *OutPath))
	{
		OutError = FString::Printf(TEXT("non si scrive %s"), *OutPath);
		OutPath.Reset();
		return false;
	}
	return true;
}
