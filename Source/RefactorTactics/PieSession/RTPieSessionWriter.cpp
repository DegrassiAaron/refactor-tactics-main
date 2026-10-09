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
	bool RTPieVerdettoRichiedeMotivo(const FRTPieSessionStep& Step)
	{
		// ⛔ Il discrimine e' CHI l'ha emesso, non quale sia. Un `NOT_JUDGEABLE` premuto da una persona
		// e' una risposta completa; lo stesso verdetto scritto dal conduttore senza motivo e' un buco.
		// Prima il criterio era il valore, e il tasto `3` stampava «(motivo non registrato)» ogni volta:
		// l'unico tasto di giudizio che il design offre marchiava un difetto falso a ogni pressione.
		return Step.Source == ERTPieVerdictSource::Conductor;
	}
}

FString URTPieSessionWriter::MakeSessionId(const FDateTime& Now)
{
	// I MILLISECONDI ci sono perche' servono: al secondo, due sedute aperte nello stesso istante —
	// e due test che girano di fila lo sono — si scrivono nella stessa cartella, e la seconda
	// sovrascrive la prima senza dire niente.
	return FString::Printf(TEXT("%s-%03d"), *Now.ToString(TEXT("%Y%m%d-%H%M%S")), Now.GetMillisecond());
}

FString URTPieSessionWriter::ReadHeadCommit()
{
	// Il `.git` sta accanto al `.uproject`: il progetto Unreal E' la radice del repository.
	FString GitDir = FPaths::Combine(FPaths::ProjectDir(), TEXT(".git"));

	// 🔴 **In un WORKTREE `.git` non e' una cartella, e' un FILE** che contiene `gitdir: <percorso>`.
	// Questo progetto ne ha uno vivo (`D:/Repositories/rt-wt-2989-dock`), quindi non e' un caso teorico:
	// senza questo ramo, una seduta condotta da un worktree perde la propria attribuzione al commit —
	// in un progetto la cui regola e' che una misura vale solo su un commit dichiarato — e i test del
	// commit diventano rossi li' per una ragione ambientale. Trovato in code review il 2026-09-20.
	if (FPaths::FileExists(GitDir))
	{
		FString Puntatore;
		if (!FFileHelper::LoadFileToString(Puntatore, *GitDir))
		{
			return FString();
		}
		Puntatore.TrimStartAndEndInline();
		if (!Puntatore.StartsWith(TEXT("gitdir:")))
		{
			return FString();
		}
		GitDir = Puntatore.RightChop(7).TrimStartAndEnd();
	}

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

	// ⚠️ In un worktree `HEAD` e' suo, ma i REF sono condivisi e vivono nella **common dir**: risolverli
	// contro `gitdir` troverebbe un file che non c'e' e farebbe cadere tutto su `packed-refs`, che a sua
	// volta non c'e' li'. Il puntatore e' il file `commondir` dentro `gitdir`, di solito `../..`.
	FString RefRoot = GitDir;
	FString CommonRel;
	if (FFileHelper::LoadFileToString(CommonRel, *FPaths::Combine(GitDir, TEXT("commondir"))))
	{
		CommonRel.TrimStartAndEndInline();
		RefRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(GitDir, CommonRel));
	}

	FString Sha;
	if (FFileHelper::LoadFileToString(Sha, *FPaths::Combine(RefRoot, RefPath)))
	{
		return Sha.TrimStartAndEnd();
	}

	// Il ref puo' essere impacchettato in `packed-refs` invece che avere un file suo.
	FString Packed;
	if (FFileHelper::LoadFileToString(Packed, *FPaths::Combine(RefRoot, TEXT("packed-refs"))))
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

		Writer->WriteValue(TEXT("verdictSource"),
			Step.Source == ERTPieVerdictSource::Human ? TEXT("human")
				: Step.Source == ERTPieVerdictSource::Conductor ? TEXT("conductor") : TEXT("none"));

		if (RTPieVerdettoRichiedeMotivo(Step))
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

	// ⚠️ `ForceUTF8WithoutBOM` e non il default `AutoDetect`: un `reason` scritto a mano con un
	// accento farebbe salvare l'INTERO file in UTF-16LE, e chi lo legge come JSON UTF-8 — il passo di
	// propagazione — troverebbe byte nulli. Un carattere digitato cambierebbe il formato del file.
	if (!FFileHelper::SaveStringToFile(ToJson(SessionId, Commit, Steps), *OutPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("non si scrive %s"), *OutPath);
		OutPath.Reset();
		return false;
	}
	return true;
}
