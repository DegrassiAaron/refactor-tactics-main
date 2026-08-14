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

	// ⚠️ Qui c'era il controllo su `FormatVersion <= 0`, rimosso con il campo (**D-141**, #844). Era una
	// guardia quasi impossibile da far scattare: il default e' 1, `FindShippedFormat` non lo tocca, e un
	// asset che non modifica il campo non lo porta nemmeno nei byte — restava il solo caso di un designer
	// che scrivesse `0` a mano nell'editor. Proteggeva dall'unico valore che nessuno avrebbe messo.

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

const FName URTMatchFormatLibrary::Skirmish2v2FormatId = FName(TEXT("Format.Skirmish2v2"));

URTMatchFormatData* URTMatchFormatLibrary::FindShippedFormat(FName FormatId)
{
	if (FormatId != Skirmish2v2FormatId)
	{
		return nullptr; // id sconosciuto: decide il chiamante, qui non si inventa un formato
	}

	URTMatchFormatData* Format = NewObject<URTMatchFormatData>();
	Format->FormatId = Skirmish2v2FormatId;
	// RoundLimit 12 — allineato a **D-010**, che consolida `RoundLimit` **10-14 in 2v2** (16-20 in 3v3): 12 e'
	// il centro dell'intervallo del formato che questo catalogo descrive. Portato qui da 5 il 2026-08-10.
	//
	// Il 5 precedente era un valore da test, e si dichiarava «una scelta di ritmo, non un ripiego»: era una
	// motivazione scritta senza confrontarla con D-010, che diceva gia' il contrario. A falsificarla e' stato
	// il primo playtest al PIE — partita 2v2 su `GeneratedTestArena` finita
	// `Pareggio - allo scadere dei round (round 5/5)` con una squadra in vantaggio **2 contro 1** e il bot
	// che gia' puntava l'ultimo superstite. Con 5 la via NORMALE di chiusura era il pareggio a vantaggio
	// netto: un esito che nessuno vuole dichiarare come regola.
	//
	// ⚠️ `RoundLimit` non e' solo la fine della partita: **D-056** ne deriva `InitialBank`
	// (`RoundLimit x (MaxWindow - Grace)`), quindi questo numero muove anche il time bank. La formula si tara
	// a CP 14.6 e li' va riletta con 12, non con 5.
	Format->RoundLimit = 12;
	// `ExpectedRounds` non lo legge nessun codice di gioco: e' un target di design, e il suo unico lettore
	// e' il validator, che rifiuta un formato in cui i round attesi superano il limite.
	//
	// Vale **10**, e la scelta e' l'inversa di quella che c'era con RoundLimit 5. Li' i round attesi erano
	// il limite stesso, perche' il limite ERA la fine attesa; con 12 la fine attesa torna a essere
	// l'**eliminazione**, e il limite la rete di sicurezza dietro di essa. Il 10 e' il dato misurato
	// headless il 2026-08-06 (bot contro bot: la partita si decide al turno 10), non un numero scelto a
	// tavolino — ed e' lo stesso valore che `PIE-HEXPLAY-10` porta come dato di riferimento.
	// Tenerlo uguale a 12 avrebbe dichiarato di nuovo che ci si aspetta di arrivare allo scadere.
	// Soglia obiettivo ZERO, e non e' pigrizia: **nessuno assegna punti**. `ARTTurnManager::AddTeamScore`
	// esiste e funziona, ma il suo unico chiamante in tutto il repository e' un test — nel runtime non ci sono
	// obiettivi che producano punteggio. Una soglia > 0 dichiarerebbe una via di vittoria IRRAGGIUNGIBILE, cioe'
	// il difetto ricorrente del progetto nella sua forma opposta: non un dato che nessuno legge, ma una soglia
	// che nessuno alimenta. Lo zero dice il vero — in v0.1 si vince per eliminazione o al limite di round — e
	// diventa un numero il giorno in cui un obiettivo chiama `AddTeamScore`.
	Format->ExpectedRounds = 10;
	Format->ScoreToWin = 0;
	Format->UnitsPerTeam = 2;
	Format->MapClass = ERTMapClass::Skirmish;
	return Format;
}

TArray<FName> URTMatchFormatLibrary::ShippedFormatIds()
{
	return { Skirmish2v2FormatId };
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
