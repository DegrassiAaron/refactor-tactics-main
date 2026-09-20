#pragma once

// L'overlay che chiede il verdetto (#3208).
//
// ⛔ **Nessuna decisione vive qui.** Il widget mostra cosa il conduttore gli dice di mostrare e gli
// rimanda un tasto; se non c'e' — seduta headless, o overlay non ancora montato — il conduttore funziona
// lo stesso e i verdetti arrivano da `rt.Pie.Verdict`.
//
// 🔑 **In C++ e senza `.uasset`**, deliberatamente: l'authoring di un widget appartiene al clone
// principale e il binding dei dispatcher si cabla a mano nell'Editor, quindi una logica che vivesse nel
// Blueprint non avrebbe modo di essere rossa senza aprirlo. Chi vuole una resa piu' curata deriva un
// `WBP_` da questa classe: eredita il comportamento senza toccarlo.

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "InputCoreTypes.h" // FKey
#include "PieSession/RTPieSessionTypes.h"

#include "RTPieVerdictOverlay.generated.h"

UCLASS()
class REFACTORTACTICS_API URTPieVerdictOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Il testo del prompt: l'ID della voce, il criterio se ce n'e' uno, e dove posare l'occhio.
	 *
	 * ⛔ **Non riporta l'esito atteso**, che ha un owner solo (`test-manuali-pie.md`). L'ID e' l'ancora:
	 * chi giudica lo ritrova nel registro, dove il criterio pieno e' scritto per esteso.
	 *
	 * Statica e pura perche' sia verificabile senza costruire un widget: e' la meta' di questo file che
	 * un gate puo' guardare.
	 */
	static FText ComposePrompt(const FString& PieItem, int32 Criterion, const FString& WhatToWatch);

	/** Il verdetto di un tasto, o `Pending` se quel tasto non ne e' uno. Statica e pura. */
	static ERTPieVerdict VerdictForKey(const FKey& Key);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** Rilegge dal conduttore cosa mostrare. Vuoto quando nessun passo aspetta un verdetto. */
	FText CurrentPrompt() const;

	TSharedPtr<class STextBlock> PromptText;
};
