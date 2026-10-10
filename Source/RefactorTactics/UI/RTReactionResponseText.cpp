#include "UI/RTReactionResponseText.h"

#include "Turn/RTReactionOpportunityTypes.h" // FireResponseTarget: il formato `FIRE:<i>` ha un solo parser

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
	 */
	const FRTResponseText Voci[] = {
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
		for (const FRTResponseText& Voce : Voci)
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
