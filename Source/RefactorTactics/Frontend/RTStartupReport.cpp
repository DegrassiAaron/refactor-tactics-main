#include "Frontend/RTStartupReport.h"

#define LOCTEXT_NAMESPACE "RTStartup"

bool URTStartupReportLibrary::IsFatal(ERTStartupOutcome Outcome)
{
	// ⚠️ `switch` esplicito e senza `default`: quando qualcuno aggiungera' un nono esito, il compilatore
	// chiedera' **di che tipo e'** invece di classificarlo in silenzio come non fatale. Un `default` qui
	// renderebbe ogni caso nuovo un ripiego, che e' il lato sbagliato in cui sbagliare.
	switch (Outcome)
	{
	case ERTStartupOutcome::FormatAssetInvalid:
	case ERTStartupOutcome::ShippedFormatInvalid:
	case ERTStartupOutcome::FormatMapMismatch:
	case ERTStartupOutcome::RosterHeroMissing:
	// I due di CP 46.4: senza livello non c'e' partita da avviare, e senza consumatore non parte comunque.
	// A differenza di `LevelMapMissing` non hanno ripiego, quindi sono fatali.
	case ERTStartupOutcome::MatchLevelUnset:
	case ERTStartupOutcome::MatchRequestNotConsumed:
	// I due di CP 46.6, gemelli dei precedenti dall'altro capo del ciclo: senza livello del frontend non
	// c'e' dove tornare, e senza consumatore il ritorno non avviene. Nessuno dei due ha un ripiego.
	case ERTStartupOutcome::FrontendLevelUnset:
	case ERTStartupOutcome::FrontendReturnNotConsumed:
		return true;

	case ERTStartupOutcome::Ok:
	case ERTStartupOutcome::UsingTestArena:
	case ERTStartupOutcome::UsingDemoArena:
	case ERTStartupOutcome::LevelMapMissing:
	// Gemello del precedente e degradato per la stessa ragione: l'arena demo sostituisce anche la mappa
	// vuota. Cambia cio' che si dice, non cio' che si fa.
	case ERTStartupOutcome::LevelMapEmpty:
	case ERTStartupOutcome::UsingFallbackFormat:
	case ERTStartupOutcome::NoTurnManager:
		return false;
	}

	return false;
}

bool URTStartupReportLibrary::IsDegraded(ERTStartupOutcome Outcome)
{
	// Degradato = non `Ok` e non fatale. Definirlo per differenza invece che con un secondo elenco evita
	// che i due elenchi divergano: un esito nuovo appartiene a uno dei due per costruzione.
	return Outcome != ERTStartupOutcome::Ok && !IsFatal(Outcome);
}

ERTStartupOutcome URTStartupReportLibrary::FindFatal(const FRTStartupReport& Report)
{
	for (const FRTStartupNote& Note : Report.Notes)
	{
		if (IsFatal(Note.Outcome))
		{
			return Note.Outcome;
		}
	}
	return ERTStartupOutcome::Ok;
}

bool URTStartupReportLibrary::HasDegradation(const FRTStartupReport& Report)
{
	for (const FRTStartupNote& Note : Report.Notes)
	{
		if (IsDegraded(Note.Outcome))
		{
			return true;
		}
	}
	return false;
}

FText URTStartupReportLibrary::DescribeOutcome(ERTStartupOutcome Outcome)
{
	// Il testo nasce QUI e non nel widget: e' la stessa ragione per cui esiste l'enum. Un widget che
	// componesse la riga sarebbe libero di dire una cosa diversa dal log a parita' di causa.
	switch (Outcome)
	{
	case ERTStartupOutcome::Ok:
		return FText::GetEmpty();

	case ERTStartupOutcome::FormatAssetInvalid:
		return LOCTEXT("FormatAssetInvalid", "Il formato di partita assegnato non e' valido.");
	case ERTStartupOutcome::ShippedFormatInvalid:
		return LOCTEXT("ShippedFormatInvalid", "Il formato spedito col gioco non e' valido.");
	case ERTStartupOutcome::FormatMapMismatch:
		return LOCTEXT("FormatMapMismatch", "Formato e mappa non combaciano.");
	case ERTStartupOutcome::RosterHeroMissing:
		return LOCTEXT("RosterHeroMissing", "Un eroe della formazione non e' nel catalogo: partita non allestita.");

	case ERTStartupOutcome::MatchLevelUnset:
		return LOCTEXT("MatchLevelUnset",
			"Nessun livello di partita configurato: controlla MatchLevel in DefaultGame.ini.");
	case ERTStartupOutcome::MatchRequestNotConsumed:
		return LOCTEXT("MatchRequestNotConsumed",
			"La richiesta di partita precedente non e' stata raccolta: l'avvio non e' collegato.");

	case ERTStartupOutcome::UsingTestArena:
		return LOCTEXT("UsingTestArena", "Arena di PROVA generata: non e' una mappa di gioco.");
	case ERTStartupOutcome::UsingDemoArena:
		return LOCTEXT("UsingDemoArena", "Arena di ripiego generata.");
	case ERTStartupOutcome::LevelMapMissing:
		return LOCTEXT("LevelMapMissing", "Il livello non porta una mappa esagonale: arena di ripiego.");
	// La correzione e' l'opposto di quella del fratello: li' manca l'actor, qui c'e' e la sua mappa e'
	// vuota. Mandare a posare un actor gia' posato e' il difetto che #1921 chiude.
	case ERTStartupOutcome::LevelMapEmpty:
		return LOCTEXT("LevelMapEmpty", "La mappa del livello e' VUOTA (zero celle): arena di ripiego.");
	case ERTStartupOutcome::UsingFallbackFormat:
		return LOCTEXT("UsingFallbackFormat", "Formato di RIPIEGO: nessun formato dichiarato in vigore.");
	case ERTStartupOutcome::NoTurnManager:
		return LOCTEXT("NoTurnManager", "Nessun TurnManager: il formato non e' stato applicato.");

	case ERTStartupOutcome::FrontendLevelUnset:
		return LOCTEXT("FrontendLevelUnset",
			"Nessun livello di menu configurato: controlla FrontendLevel in DefaultGame.ini.");
	case ERTStartupOutcome::FrontendReturnNotConsumed:
		return LOCTEXT("FrontendReturnNotConsumed",
			"Il ritorno al menu precedente non e' stato raccolto: lo smontaggio non e' collegato.");
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
