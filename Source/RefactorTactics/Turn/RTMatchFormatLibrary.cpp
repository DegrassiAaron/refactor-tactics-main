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

	// CP 19.2: senza composizione dichiarata il formato non descrive una partita. Zero non e' «nessun limite»
	// come per `ScoreToWin` — e' una squadra vuota.
	if (Rules.UnitsPerTeam <= 0)
	{
		Errors.Add(FString::Printf(
			TEXT("UnitsPerTeam %d: il formato non dichiara quante unita' schiera una squadra"),
			Rules.UnitsPerTeam));
	}

	return Errors;
}

TArray<FString> URTMatchFormatLibrary::ValidateAgainstMap(const FRTMatchRules& Rules, const URTHexMapAsset* Map)
{
	if (!Map)
	{
		return { TEXT("mappa assente: l'accoppiata formato/mappa non e' verificabile") };
	}

	if (Map->MapClass != Rules.MapClass)
	{
		const UEnum* ClassEnum = StaticEnum<ERTMapClass>();
		const FString Wanted = ClassEnum ? ClassEnum->GetNameStringByValue(static_cast<int64>(Rules.MapClass)) : FString();
		const FString Found = ClassEnum ? ClassEnum->GetNameStringByValue(static_cast<int64>(Map->MapClass)) : FString();

		// Il messaggio nomina entrambe le classi: chi legge un log di allestimento fallito deve sapere cosa
		// cambiare, e «classe incompatibile» non lo dice.
		return { FString::Printf(
			TEXT("il formato '%s' richiede una mappa %s, ma la mappa dichiara %s"),
			*Rules.FormatId.ToString(), *Wanted, *Found) };
	}

	return {};
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
	Rules.UnitsPerTeam = Format->UnitsPerTeam;
	Rules.MapClass = Format->MapClass;

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
	OutRules.UnitsPerTeam = Format->UnitsPerTeam;
	OutRules.MapClass = Format->MapClass;
	return true;
}

FRTMatchRules URTMatchFormatLibrary::MakeFallbackRules()
{
	FRTMatchRules Rules;
	Rules.FormatId = FallbackFormatId;
	Rules.RoundLimit = 12;
	Rules.ScoreToWin = 0;
	// Il ripiego copre l'ASSENZA del formato, e deve produrre la partita del vertical slice: 2v2 su mappa
	// Skirmish. Un ripiego che non dichiarasse la composizione fallirebbe la propria validazione, e la (D1)
	// «la partita si avvia comunque» smetterebbe di valere.
	Rules.UnitsPerTeam = 2;
	Rules.MapClass = ERTMapClass::Skirmish;
	return Rules;
}
