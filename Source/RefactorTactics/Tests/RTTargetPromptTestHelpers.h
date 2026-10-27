// #3869 — i testi del prompt di targeting, per i test che li guardano tutti.
//
// 🔑 **Enumerati dagli `UENUM`, non scritti a mano.** Il prompt e' una funzione di `ERTPointerContext` e `ERTPointerTargetKind`
// (`URTHudViewModel::BuildTargetPrompt`): una lista di valori copiata in due test resta com'e' quando un enum ne guadagna uno,
// e il test continua a dire «tutti i testi» su un insieme che non lo e' piu'. Qui si cammina `StaticEnum` con `NumEnums() - 1`
// (l'ultimo e' il `_MAX` che UHT aggiunge), come fanno gli altri test che coprono un enum.
//
// ⚠️ **Namespace NOMINATO e funzioni `inline`**, per la stessa ragione di `RTWidgetAssetTestHelpers.h`: un helper anonimo in un header
// e' una copia per unita' di traduzione, e una funzione non `inline` in un header e' una doppia definizione nello stesso blob.

#pragma once

#include "CoreMinimal.h"
#include "UI/RTHudViewModel.h"
#include "Player/RTPointerInteraction.h"

namespace RTTargetPromptTest
{
	/**
	 * Ogni testo diverso e non vuoto che `BuildTargetPrompt` produce, su tutte le coppie (contesto, forma di bersaglio).
	 *
	 * Distinti perche' le coppie sono molte e i testi pochi: chi li misura o li legge una volta per testo non rifa' lo stesso
	 * lavoro per ogni coppia che porta alla stessa frase.
	 */
	inline TArray<FText> AllTexts()
	{
		TArray<FText> Testi;
		TArray<FString> Visti;
		const UEnum* Contesti = StaticEnum<ERTPointerContext>();
		const UEnum* Forme = StaticEnum<ERTPointerTargetKind>();
		for (int32 c = 0; c < Contesti->NumEnums() - 1; ++c)
		{
			for (int32 f = 0; f < Forme->NumEnums() - 1; ++f)
			{
				const FText Testo = URTHudViewModel::BuildTargetPrompt(
					static_cast<ERTPointerContext>(Contesti->GetValueByIndex(c)),
					static_cast<ERTPointerTargetKind>(Forme->GetValueByIndex(f))).Text;
				const FString Lettura = Testo.ToString();
				if (!Lettura.IsEmpty() && !Visti.Contains(Lettura))
				{
					Visti.Add(Lettura);
					Testi.Add(Testo);
				}
			}
		}
		return Testi;
	}
}
