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

/**
 * DOVE si posa il pannello del verdetto.
 *
 * 🔴 E' un dato e non quattro numeri sparsi nel layout, per una ragione misurata: il default —
 * nessun allineamento, quindi in alto a sinistra — lo posava sopra `URTTeamRosterWidget`, cioe' sopra
 * i nomi degli eroi, e alla prima seduta reale il prompt era illeggibile (#3242). Un posizionamento
 * che vive dentro `RebuildWidget` non ha modo di essere verificato senza uno schermo; questo si'.
 */
struct FRTPieOverlayPlacement
{
	EHorizontalAlignment Horizontal = HAlign_Left;
	EVerticalAlignment Vertical = VAlign_Center;

	/** Distanza dal bordo sinistro, in pixel di Slate. */
	float LeftMargin = 24.f;

	/** ⛔ Il tetto esiste perche' il CENTRO resta libero: la board non si copre, per contratto. */
	float MaxWidth = 420.f;
};

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

	/**
	 * Il posizionamento che `RebuildWidget` applica — la sola parte del layout che un gate headless puo'
	 * guardare.
	 *
	 * ⚠️ Dice dove il pannello **chiede** di stare, non che a schermo si legga: quella meta' resta di chi
	 * guarda, ed e' il residuo dichiarato in spec §4.5.
	 */
	static FRTPieOverlayPlacement Placement();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** Rilegge dal conduttore cosa mostrare. Vuoto quando nessun passo aspetta un verdetto. */
	FText CurrentPrompt() const;

	TSharedPtr<class STextBlock> PromptText;
};
