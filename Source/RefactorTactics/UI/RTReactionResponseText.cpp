#include "UI/RTReactionResponseText.h"

#include "Turn/RTReactionOpportunityTypes.h" // FireResponseTarget: il formato `FIRE:<i>` ha un solo parser
#include "Turn/RTReactionWindowView.h" // la vista che porta i nomi gia' risolti (#3667)

namespace
{
	struct FRTResponseText
	{
		const TCHAR* Key;
		const TCHAR* Name;
		const TCHAR* Description;
	};

	/**
	 * Le voci. `FIRE` copre ogni `FIRE:<indice>`: la chiave la ricava `KeyFor`, non un confronto sul prefisso.
	 * `HOLD` e' `URTReactionOpportunityLibrary::HoldResponse()`; `Hold Ground` e le manovre sono le risposte dei
	 * profili (`URTCatalogLibrary::BraceExecutableResponses`).
	 *
	 * ⚠️ **Il nome e' specifico di proposito** (#3632): in un namespace anonimo vive in tutto il blob unity, e
	 * si chiamava `Voci`, nome comune per una variabile locale. Quando la ripartizione dei blob ha messo questo
	 * file accanto a `RTScreenHudWidgets.cpp`, il parametro `Voci` di una lambda lo ha nascosto e la build si e'
	 * fermata su C4459.
	 */
	const FRTResponseText TestiDelleRisposte[] = {
		{ TEXT("FIRE"),        TEXT("FIRE"),        TEXT("Spari al bersaglio.") },
		{ TEXT("HOLD"),        TEXT("HOLD"),        TEXT("Non spari: lasci passare il bersaglio.") },
		{ TEXT("Hold Ground"), TEXT("HOLD GROUND"), TEXT("Resti nella cella: attutisci i colpi e resisti alla prima spinta.") },
		{ TEXT("SIDESTEP"),    TEXT("SIDESTEP"),    TEXT("Scarti di una cella, fuori dalla linea della spinta.") },
	};

	FString KeyFor(const FString& Response)
	{
		return URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE ? FString(TEXT("FIRE")) : Response;
	}

	const FRTResponseText* Find(const FString& Response)
	{
		const FString Chiave = KeyFor(Response);
		for (const FRTResponseText& Voce : TestiDelleRisposte)
		{
			if (Chiave == Voce.Key)
			{
				return &Voce;
			}
		}
		return nullptr;
	}
}

FText RTReactionResponseText::NameFor(const FString& Response)
{
	const FRTResponseText* Voce = Find(Response);
	return FText::FromString(Voce ? FString(Voce->Name) : Response);
}

FText RTReactionResponseText::DescriptionFor(const FString& Response)
{
	const FRTResponseText* Voce = Find(Response);
	return Voce ? FText::FromString(Voce->Description) : FText::GetEmpty();
}

FText RTReactionResponseText::DescriptionFor(const FRTReactionWindowOptionView& Option)
{
	// Il nome c'e' solo dove il core lo ha scritto: un `FIRE` il cui bersaglio e' stato risolto alla
	// costruzione della vista. Senza, la frase generica della tabella — mai un indice, mai un nome ricavato qui.
	if (URTReactionOpportunityLibrary::FireResponseTarget(Option.Response) != INDEX_NONE)
	{
		return Option.TargetName.IsEmptyOrWhitespace()
			? DescriptionFor(Option.Response)
			: FText::Format(NSLOCTEXT("RTReactionResponseText", "FireAtTarget", "Attacca {0}"), Option.TargetName);
	}
	// `HOLD` e' la risposta dell'Overwatch, e la tavola E la scrive in coppia con «Attacca …». Il confronto e'
	// con la funzione che quel valore lo SCRIVE, non con una stringa ripetuta qui.
	if (Option.Response == FString(URTReactionOpportunityLibrary::HoldResponse()))
	{
		return NSLOCTEXT("RTReactionResponseText", "HoldFire", "Non attaccare");
	}
	return DescriptionFor(Option.Response);
}

FText RTReactionResponseText::DescriptionFor(const FRTReactionWindowOptionView& Option, const FRTReactionWindowView& Window)
{
	if (!Window.bOpen || Option.Response != FString(URTReactionOpportunityLibrary::HoldResponse()))
	{
		return DescriptionFor(Option);
	}

	// [D-411] (`#3788`): chi questa finestra offre come bersaglio e' stato chiesto, e dopo un `HOLD` — o allo scadere —
	// non torna per il resto del turno. «Offerto» lo dice il core, con la funzione con cui il pompaggio segna chi ha
	// chiesto (review della PR #3791): qui non c'e' una seconda copia della regola. I nomi stanno sulle opzioni `FIRE`.
	TArray<FString> Risposte;
	for (const FRTReactionWindowOptionView& Altra : Window.Options)
	{
		Risposte.Add(Altra.Response);
	}
	const TArray<int32> Offerti = URTReactionOpportunityLibrary::OfferedFireTargets(Risposte);
	TArray<FText> Nomi;
	for (const FRTReactionWindowOptionView& Altra : Window.Options)
	{
		const int32 Bersaglio = URTReactionOpportunityLibrary::FireResponseTarget(Altra.Response);
		if (Bersaglio != INDEX_NONE && Offerti.Contains(Bersaglio) && !Altra.TargetName.IsEmptyOrWhitespace())
		{
			Nomi.Add(Altra.TargetName);
		}
	}
	const bool bTuttiNominati = Offerti.Num() > 0 && Nomi.Num() == Offerti.Num();

	if (Offerti.Num() == 1)
	{
		return bTuttiNominati
			? FText::Format(NSLOCTEXT("RTReactionResponseText", "LetThrough", "Lascia passare {0}"), Nomi[0])
			: DescriptionFor(Option);
	}
	if (Offerti.Num() > 1)
	{
		// ⚠️ **«Tutti» solo se nessuno e' entrato senza essere offerto** (review della PR #3791): il sottotitolo nomina chi
		// e' entrato, e chi la condizione dichiarata ha escluso non e' stato chiesto — puo' tornare. Allora si nominano
		// quelli che si lasciano passare, o, senza i loro nomi, la frase resta quella di prima.
		if (Window.TriggerNames.Num() <= Offerti.Num())
		{
			return NSLOCTEXT("RTReactionResponseText", "LetAllThrough", "Lascia passare tutti");
		}
		if (bTuttiNominati)
		{
			FText Primi = Nomi[0];
			for (int32 i = 1; i < Nomi.Num() - 1; ++i)
			{
				Primi = FText::Format(NSLOCTEXT("RTReactionResponseText", "ListComma", "{0}, {1}"), Primi, Nomi[i]);
			}
			return FText::Format(NSLOCTEXT("RTReactionResponseText", "LetSomeThrough", "Lascia passare {0} e {1}"),
				Primi, Nomi.Last());
		}
	}
	// Nessun bersaglio, o nessun nome da dire: la frase non nomina nessuno, e resta quella di prima.
	return DescriptionFor(Option);
}

FText RTReactionResponseText::TitleFor(const FRTReactionWindowView& Window)
{
	if (!Window.bOpen)
	{
		return FText::GetEmpty();
	}
	const bool bReazione = !Window.ReactionName.IsEmptyOrWhitespace();
	const bool bUnita = !Window.ReactorName.IsEmptyOrWhitespace();
	const FText Reazione = bReazione ? Window.ReactionName : NSLOCTEXT("RTReactionResponseText", "NeutralTitle", "Reazione");
	return bUnita
		? FText::Format(NSLOCTEXT("RTReactionResponseText", "TitleWithReactor", "{0} — {1}"), Reazione, Window.ReactorName)
		: Reazione;
}

FText RTReactionResponseText::SubtitleFor(const FRTReactionWindowView& Window)
{
	if (!Window.bOpen)
	{
		return FText::GetEmpty();
	}

	// Solo i nomi leggibili: un nome vuoto in mezzo a una frase e' una frase rotta.
	TArray<FText> Nomi;
	for (const FText& Nome : Window.TriggerNames)
	{
		if (!Nome.IsEmptyOrWhitespace()) { Nomi.Add(Nome); }
	}

	// 🔑 **La frase la sceglie cio' che chi apre la finestra ha dichiarato**, non l'id della reazione: il
	// produttore sa se qualcuno e' entrato nella sua area o lo sta spingendo, e lo scrive in `TriggerKind`.
	switch (Window.TriggerKind)
	{
	case ERTReactionTriggerKind::Pushed:
		// ⚠️ Senza un nome la frase e' NEUTRA, e non e' un ripiego di comodo: e' cio' che il giocatore ha
		// diritto di sapere quando chi lo spinge sta in una cella che la sua squadra non rileva.
		return Nomi.Num() > 0
			? FText::Format(NSLOCTEXT("RTReactionResponseText", "PushedBy", "{0} ti spinge"), Nomi[0])
			: NSLOCTEXT("RTReactionResponseText", "PushedByUnknown", "Un nemico ti spinge");

	case ERTReactionTriggerKind::EnteredControlledArea:
	{
		if (Nomi.Num() == 0)
		{
			return FText::GetEmpty();
		}
		if (Nomi.Num() == 1)
		{
			return FText::Format(
				NSLOCTEXT("RTReactionResponseText", "OneEntered", "{0} è entrato nell'area controllata"), Nomi[0]);
		}
		// «A, B e C»: tutti tranne l'ultimo con la virgola, l'ultimo con la congiunzione.
		FText Primi = Nomi[0];
		for (int32 i = 1; i < Nomi.Num() - 1; ++i)
		{
			Primi = FText::Format(NSLOCTEXT("RTReactionResponseText", "ListComma", "{0}, {1}"), Primi, Nomi[i]);
		}
		return FText::Format(
			NSLOCTEXT("RTReactionResponseText", "ManyEntered", "{0} e {1} sono entrati nell'area controllata"),
			Primi, Nomi.Last());
	}

	case ERTReactionTriggerKind::None:
	default:
		break;
	}
	return FText::GetEmpty();
}
