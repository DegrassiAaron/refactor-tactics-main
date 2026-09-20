#include "PieSession/RTPieSessionPlaylist.h"

#include "Misc/FileHelper.h"
#include "RefactorTactics.h" // LogRT
#include "ScenarioHarness/RTScenarioIndex.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** Uno scenario del corpus, ridotto a cio' che serve per comporre: chi e', e cosa dichiara. */
	struct FRTPieDichiarante
	{
		FString ScenarioId;
		FString CosaGuardare;
		TArray<FString> Verifies;
	};

	/** Il `_nota_cosa_guardare` dello scenario, se c'e'. Non e' l'esito atteso: e' dove posare l'occhio. */
	FString RTPieLeggiCosaGuardare(const TSharedPtr<FJsonObject>& Root)
	{
		FString Nota;
		if (Root.IsValid()) { Root->TryGetStringField(TEXT("_nota_cosa_guardare"), Nota); }
		return Nota;
	}

	TArray<FRTPieDichiarante> RTPieLeggiCorpus()
	{
		TArray<FRTPieDichiarante> Fuori;

		TArray<FString> Problemi;
		for (const FRTScenarioEntry& Entry : URTScenarioIndex::Scan(Problemi))
		{
			FString Json;
			if (!FFileHelper::LoadFileToString(Json, *Entry.Path))
			{
				continue;
			}

			FRTPieDichiarante D;
			D.ScenarioId = Entry.ScenarioId;
			if (URTPieSessionPlaylist::ReadVerifies(Json, D.Verifies) && D.Verifies.Num() > 0)
			{
				TSharedPtr<FJsonObject> Root;
				const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
				if (FJsonSerializer::Deserialize(Reader, Root))
				{
					D.CosaGuardare = RTPieLeggiCosaGuardare(Root);
				}
				Fuori.Add(MoveTemp(D));
			}
		}

		// Ordine stabile: `Scan` non promette un ordine, e una coda che cambia fra due aperture
		// renderebbe irriproducibile una seduta.
		Fuori.Sort([](const FRTPieDichiarante& A, const FRTPieDichiarante& B)
			{ return A.ScenarioId < B.ScenarioId; });
		return Fuori;
	}

	FRTPieSessionStep RTPieFaiPasso(const FRTPieDichiarante& D, const FString& Voce)
	{
		FRTPieSessionStep S;
		S.PieItem = Voce;
		S.ScenarioId = D.ScenarioId;
		S.WhatToWatch = D.CosaGuardare;
		return S;
	}

	/** Un selettore che nomina voci comincia per `PIE-`; tutto il resto e' un prefisso di scenario. */
	bool RTPieSelettoreNominaVoci(const FString& Selector)
	{
		return Selector.StartsWith(TEXT("PIE-"), ESearchCase::IgnoreCase);
	}
}

bool URTPieSessionPlaylist::ReadVerifies(const FString& JsonText, TArray<FString>& OutVerifies)
{
	OutVerifies.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* VerifiesJson = nullptr;
	if (!Root->TryGetArrayField(TEXT("verifies"), VerifiesJson))
	{
		return true; // nessun campo: lecito, lo scenario semplicemente non dichiara voci
	}

	for (const TSharedPtr<FJsonValue>& Value : *VerifiesJson)
	{
		FString Voce;
		if (Value.IsValid() && Value->TryGetString(Voce) && !Voce.IsEmpty())
		{
			OutVerifies.Add(Voce);
		}
	}
	return true;
}

FRTPieSessionPlan URTPieSessionPlaylist::Compose(const FString& Selector)
{
	FRTPieSessionPlan Plan;
	const TArray<FRTPieDichiarante> Corpus = RTPieLeggiCorpus();

	if (RTPieSelettoreNominaVoci(Selector))
	{
		TArray<FString> Chieste;
		Selector.ParseIntoArray(Chieste, TEXT(","), /*CullEmpty=*/ true);

		for (FString& Voce : Chieste)
		{
			Voce.TrimStartAndEndInline();

			TArray<FString> Dichiaranti;
			for (const FRTPieDichiarante& D : Corpus)
			{
				if (D.Verifies.ContainsByPredicate([&Voce](const FString& V)
					{ return V.Equals(Voce, ESearchCase::IgnoreCase); }))
				{
					Dichiaranti.Add(D.ScenarioId);
				}
			}

			if (Dichiaranti.Num() == 0)
			{
				Plan.Excluded.Add(FString::Printf(TEXT("%s — nessuno scenario la dichiara"), *Voce));
			}
			else if (Dichiaranti.Num() > 1)
			{
				Plan.Ambiguous.Add(FString::Printf(TEXT("%s — la dichiarano %s"),
					*Voce, *FString::Join(Dichiaranti, TEXT(", "))));
			}
			else
			{
				const FRTPieDichiarante* D = Corpus.FindByPredicate(
					[&Dichiaranti](const FRTPieDichiarante& C) { return C.ScenarioId == Dichiaranti[0]; });
				if (D) { Plan.Steps.Add(RTPieFaiPasso(*D, Voce)); }
			}
		}
	}
	else
	{
		// Prefisso di scenario: `Visual.Perception.*` e `Visual.Perception` selezionano lo stesso.
		FString Prefisso = Selector;
		Prefisso.RemoveFromEnd(TEXT("*"));

		for (const FRTPieDichiarante& D : Corpus)
		{
			if (!D.ScenarioId.StartsWith(Prefisso, ESearchCase::IgnoreCase))
			{
				continue;
			}
			for (const FString& Voce : D.Verifies)
			{
				Plan.Steps.Add(RTPieFaiPasso(D, Voce));
			}
		}

		if (Plan.Steps.Num() == 0)
		{
			Plan.Excluded.Add(FString::Printf(
				TEXT("%s — nessuno scenario col prefisso dichiara voci PIE"), *Selector));
		}
	}

	// ⛔ Un'ambiguita' azzera la coda: vedi il commento di `FRTPieSessionPlan::Ambiguous`.
	if (Plan.Ambiguous.Num() > 0)
	{
		Plan.Steps.Reset();
	}
	return Plan;
}
