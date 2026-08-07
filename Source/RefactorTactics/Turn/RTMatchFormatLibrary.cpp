#include "Turn/RTMatchFormatLibrary.h"

const FName URTMatchFormatLibrary::FallbackFormatId = FName(TEXT("Format.Fallback"));

TArray<FString> URTMatchFormatLibrary::ValidateRules(const FRTMatchRules& Rules)
{
	TArray<FString> Errors;

	if (Rules.FormatId.IsNone())
	{
		Errors.Add(TEXT("FormatId assente: la traccia non potrebbe dichiarare con quale formato e' stata prodotta"));
	}

	if (Rules.RoundLimit <= 0)
	{
		Errors.Add(FString::Printf(
			TEXT("RoundLimit %d: senza un limite positivo la partita non puo' finire per scadenza dei round"),
			Rules.RoundLimit));
	}

	if (Rules.ScoreToWin < 0)
	{
		Errors.Add(FString::Printf(
			TEXT("ScoreToWin %d: la soglia di punteggio non puo' essere negativa (0 = via disattivata)"),
			Rules.ScoreToWin));
	}

	return Errors;
}

TArray<FString> URTMatchFormatLibrary::ValidateFormat(const URTMatchFormatData* Format)
{
	if (!Format)
	{
		return { TEXT("formato di partita assente") };
	}

	FRTMatchRules Rules;
	Rules.FormatId = Format->FormatId;
	Rules.RoundLimit = Format->RoundLimit;
	Rules.ScoreToWin = Format->ScoreToWin;

	TArray<FString> Errors = ValidateRules(Rules);

	if (Format->FormatVersion <= 0)
	{
		Errors.Add(FString::Printf(TEXT("FormatVersion %d: versione non dichiarata"), Format->FormatVersion));
	}

	// Solo con un limite valido il confronto ha senso: sommarlo a un RoundLimit gia' rifiutato direbbe due
	// volte lo stesso difetto.
	if (Format->RoundLimit > 0 && Format->ExpectedRounds > Format->RoundLimit)
	{
		Errors.Add(FString::Printf(
			TEXT("ExpectedRounds %d oltre RoundLimit %d: il formato dichiara una durata che non puo' raggiungere"),
			Format->ExpectedRounds, Format->RoundLimit));
	}

	return Errors;
}

bool URTMatchFormatLibrary::AreRulesUsable(const FRTMatchRules& Rules, FString& OutReason)
{
	const TArray<FString> Errors = ValidateRules(Rules);
	if (Errors.Num() == 0)
	{
		return true;
	}

	OutReason = FString::Join(Errors, TEXT("; "));
	return false;
}

bool URTMatchFormatLibrary::ResolveRules(const URTMatchFormatData* Format, FRTMatchRules& OutRules,
	FString& OutReason)
{
	const TArray<FString> Errors = ValidateFormat(Format);
	if (Errors.Num() > 0)
	{
		// Fail-closed: niente ripiego e niente scrittura parziale. Chi ha chiamato decide che farne, con in
		// mano il motivo.
		OutReason = FString::Join(Errors, TEXT("; "));
		return false;
	}

	OutRules.FormatId = Format->FormatId;
	OutRules.RoundLimit = Format->RoundLimit;
	OutRules.ScoreToWin = Format->ScoreToWin;
	return true;
}

FRTMatchRules URTMatchFormatLibrary::MakeFallbackRules()
{
	FRTMatchRules Rules;
	Rules.FormatId = FallbackFormatId;
	Rules.RoundLimit = 12;
	Rules.ScoreToWin = 0;
	return Rules;
}
