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

	// CP 19.3 (D-155): quante unita' comanda una PERSONA. Tre rifiuti, e ognuno dice una cosa diversa —
	// messaggi distinti perche' chi legge un allestimento fallito deve sapere quale numero correggere.
	if (Rules.UnitsPerPlayer <= 0)
	{
		Errors.Add(FString::Printf(
			TEXT("UnitsPerPlayer %d: il formato non dichiara quante unita' comanda una persona"),
			Rules.UnitsPerPlayer));
	}
	else if (Rules.UnitsPerTeam > 0)
	{
		// I due controlli seguenti hanno senso solo con entrambi i numeri positivi: applicarli a un
		// `UnitsPerTeam` gia' rifiutato aggiungerebbe due righe che ripetono lo stesso difetto.
		if (Rules.UnitsPerPlayer > Rules.UnitsPerTeam)
		{
			Errors.Add(FString::Printf(
				TEXT("UnitsPerPlayer %d oltre UnitsPerTeam %d: una persona non puo' comandare piu' unita' di quante la squadra ne schieri"),
				Rules.UnitsPerPlayer, Rules.UnitsPerTeam));
		}
		else if (Rules.UnitsPerTeam % Rules.UnitsPerPlayer != 0)
		{
			// La ripartizione e' UNIFORME per decisione (D-155): un formato che non si divide descriverebbe
			// gruppi di controllo di dimensione diversa, e il campo — che e' UN numero — non saprebbe dirlo.
			// Rifiutare qui e' l'alternativa a un dato che mente.
			Errors.Add(FString::Printf(
				TEXT("UnitsPerTeam %d non si divide in gruppi da UnitsPerPlayer %d: la ripartizione del controllo e' uniforme, e un resto lascerebbe un gruppo di dimensione diversa che questo campo non sa esprimere"),
				Rules.UnitsPerTeam, Rules.UnitsPerPlayer));
		}
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
	Rules.UnitsPerPlayer = Format->UnitsPerPlayer;
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
	OutRules.UnitsPerPlayer = Format->UnitsPerPlayer;
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
	// ⛔ **E resta 12 anche dopo la correzione dello stato assorbente del 2026-08-22 (#1088), per decisione.**
	// [D-184] vieta esattamente questa mossa: alzare `RoundLimit` per accomodare una durata bot-contro-bot e'
	// l'inferenza «l'eroe e' debole» al posto di «il bot non sa giocarla» con un altro cappello, e [D-102] la
	// dichiara inammissibile finche' il bot non e' certificato. In piu' riaprirebbe D-010 e muoverebbe
	// `InitialBank` via [D-056].
	//
	// ⚠️ **E non basterebbe nemmeno**: la misura corrente e' **round 15**, annotata accanto a `ExpectedRounds`
	// insieme al 21 che sostituisce. 15 e' oltre anche il massimo della banda 2v2, quindi il 14 che una
	// misura headless suggeriva era giusto per il banco e falso per il gioco — quel percorso e' piu' corto
	// di quello reale. Il numero vive in un posto solo, sotto: qui c'e' il rimando.
	//
	// ---
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
	//
	// 🔴 **Il 10 e' una misura del 2026-08-06, e la realta' di oggi e' 15.** Dopo la correzione del deadlock
	// di #1088 (PR #1213) una partita reale sulla configurazione spedita si decideva al round **21**
	// (registrata in #149); dopo la correzione dello stato assorbente della stessa issue, rimisurata sullo
	// stesso percorso, si decide al round **15** (`Vince il team 0 (blu) - per eliminazione (round 15/40)`,
	// 2026-08-22). Il 21 resta scritto perche' e' il numero che D-184 cita. Con `RoundLimit` 12 la
	// via normale di chiusura e' quindi tornata a essere il **pareggio allo scadere** — l'esito che il
	// passaggio da 5 a 12 aveva tolto di mezzo, ricomparso da un'altra porta.
	//
	// ⚠️ **Il 21 NON si scrive qui, e non e' pigrizia.** Primo: renderebbe il formato **invalido**, perche'
	// `ValidateFormat` rifiuta `ExpectedRounds > RoundLimit` — e la regola avrebbe ragione, dato che il
	// formato dichiarerebbe una durata che non puo' raggiungere. Secondo, e piu' importante: **D-102**
	// dichiara che un risultato bot-contro-bot non e' evidenza di bilanciamento finche' il bot non e'
	// certificato sulle capability che lo producono. Il 21 dice cosa succede, non se sia giusto.
	//
	// ∴ **D-184** decide che il pareggio allo scadere e' un esito legittimo della v0.1 invece di ritarare
	// un numero su un dato inammissibile. Il `10` resta il target di design; la misura corrente (**15**, e
	// il 21 da cui viene) resta qui accanto perche' chi arriva dopo non debba ri-misurarla — ed e' anche
	// la ragione per cui il gate di
	// `ValidateFormat` oggi tace: e' cieco su una misura scaduta, non su una regola sbagliata.
	//
	// ⚠️ **Nulla di cio' che D-184 decide si muove col 15**: e' comunque oltre il limite di 12, quindi sul
	// default la partita finisce pari allo scadere e il pareggio resta l'esito legittimo che quella voce
	// dichiara. Il 15 eredita lo statuto del 21: **non ammissibile** come evidenza di bilanciamento per
	// D-102, quindi non diventa `ExpectedRounds` — che a 15 renderebbe anche il formato invalido.
	//
	// ---
	//
	// Soglia obiettivo ZERO, e non e' pigrizia: **nessuno assegna punti**. `ARTTurnManager::AddTeamScore`
	// esiste e funziona, ma il suo unico chiamante in tutto il repository e' un test — nel runtime non ci sono
	// obiettivi che producano punteggio. Una soglia > 0 dichiarerebbe una via di vittoria IRRAGGIUNGIBILE, cioe'
	// il difetto ricorrente del progetto nella sua forma opposta: non un dato che nessuno legge, ma una soglia
	// che nessuno alimenta. Lo zero dice il vero — in v0.1 si vince per eliminazione o al limite di round — e
	// diventa un numero il giorno in cui un obiettivo chiama `AddTeamScore`.
	Format->ExpectedRounds = 10;
	Format->ScoreToWin = 0;
	Format->UnitsPerTeam = 2;
	// Un umano solo, che comanda la squadra intera: il 2v2 offline contro bot **e' gia'** il caso
	// multi-unita', e i due numeri coincidono. E' precisamente cio' che rende invisibile un percorso che
	// legga l'uno al posto dell'altro (CP 19.3, D-155).
	Format->UnitsPerPlayer = 2;
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
	// Stessa ragione, e lo stesso vincolo: un ripiego con `UnitsPerPlayer` non dichiarato fallirebbe la
	// propria validazione, e la (D1) «la partita si avvia comunque» smetterebbe di valere.
	Rules.UnitsPerPlayer = 2;
	Rules.MapClass = ERTMapClass::Skirmish;
	return Rules;
}
