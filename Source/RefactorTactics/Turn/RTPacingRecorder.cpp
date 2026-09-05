#include "Turn/RTPacingRecorder.h"

#include "Turn/RTPacingLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

void FRTPacingRecorder::Begin(int32 InTurnNumber, const TArray<FRTPacingUnitFacts>& Facts, int32 PacingTeamId)
{
	CurrentSample = FRTPacingSample();
	CurrentSample.TurnNumber = InTurnNumber;

	URTPacingLibrary::ApplyOpeningCounts(Facts, PacingTeamId, CurrentSample);

	PlanningStart = FPlatformTime::Seconds();
	LastInput = PlanningStart;
	bHadInput = false;
	bOpen = true; // da qui l'origine esiste, e i tempi si possono misurare
}

void FRTPacingRecorder::NoteInput(ERTPlanningInput Kind)
{
	const double Now = FPlatformTime::Seconds();

	// 🔴 Il tempo si misura **solo a campione aperto**, e la guardia sta qui e non nel chiamante: senza
	// origine, `(Now - 0.0) * 1000.0` sfora l'`int32`. E' il difetto che `RTPacingIntegrationTests`
	// documenta, e tenerlo dentro significa che nessun chiamante puo' dimenticarsene.
	if (bOpen && !bHadInput)
	{
		bHadInput = true;
		CurrentSample.MsToFirstInput = FMath::RoundToInt((Now - PlanningStart) * 1000.0);
	}
	LastInput = Now;

	// ⚠️ I CONTATORI valgono comunque: sono conteggi, non tempi, e non hanno bisogno di un'origine.
	switch (Kind)
	{
	case ERTPlanningInput::Selection: ++CurrentSample.SelectionCount; break;
	case ERTPlanningInput::Order:     ++CurrentSample.OrderCount;     break;
	case ERTPlanningInput::Undo:      ++CurrentSample.UndoCount;      break;
	case ERTPlanningInput::Click:
	default:
		break; // attivita' generica: aggiorna solo i tempi
	}
}

void FRTPacingRecorder::NoteLockIn(bool bUnattended, int32 InTurnNumber,
	const TArray<FRTPacingUnitFacts>& Facts, int32 PacingTeamId)
{
	// Un campione puo' non essere mai stato aperto: un turno concluso senza passare dalla pianificazione
	// presidiata esiste, e va comunque contato.
	const bool bWasOpen = bOpen;
	if (!bWasOpen)
	{
		Begin(InTurnNumber, Facts, PacingTeamId);
	}

	if (!bWasOpen || bUnattended)
	{
		// 🔴 Nessuna origine, o nessun umano: i tre tempi sono NON MISURATI, e dirlo e' meglio che
		// inventarli. `Now - 0.0` darebbe milioni di millisecondi, e finirebbe nelle statistiche.
		CurrentSample.MsToLockIn = FRTPacingSample::Unmeasured;
		CurrentSample.MsSinceLastInput = FRTPacingSample::Unmeasured;
		CurrentSample.MsToFirstInput = FRTPacingSample::Unmeasured;
		return;
	}

	const double Now = FPlatformTime::Seconds();
	CurrentSample.MsToLockIn = FMath::RoundToInt((Now - PlanningStart) * 1000.0);
	CurrentSample.MsSinceLastInput = bHadInput
		? FMath::RoundToInt((Now - LastInput) * 1000.0)
		: CurrentSample.MsToLockIn;
	if (!bHadInput)
	{
		CurrentSample.MsToFirstInput = CurrentSample.MsToLockIn;
	}
}

void FRTPacingRecorder::Close(float PlaybackSeconds, const TArray<FRTPacingUnitFacts>& Facts,
	int32 PacingTeamId, const TArray<FRTTurnLogEntry>& TurnLog, bool bWriteCsv)
{
	CurrentSample.MsPlayback = FMath::RoundToInt(PlaybackSeconds * 1000.f);

	{
		const TSet<int32> Responders(URTPacingLibrary::RespondersForPacing(Facts, PacingTeamId));

		CurrentSample.ReactionWindowsOpened =
			URTPacingLibrary::CountOpenedReactionWindows(TurnLog, Responders);
	}

	Samples.Add(CurrentSample);
	if (bWriteCsv)
	{
		AppendRow(CurrentSample);
	}
	CurrentSample = FRTPacingSample();
	bOpen = false;
}

void FRTPacingRecorder::AppendRow(const FRTPacingSample& Sample)
{
	if (FilePath.IsEmpty())
	{
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RT"));
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/ true);
		FilePath = FPaths::Combine(Dir,
			FString::Printf(TEXT("pacing_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"))));
		FFileHelper::SaveStringToFile(URTPacingLibrary::CsvHeader() + LINE_TERMINATOR, *FilePath);
	}
	FFileHelper::SaveStringToFile(URTPacingLibrary::CsvRow(Sample) + LINE_TERMINATOR, *FilePath,
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}
