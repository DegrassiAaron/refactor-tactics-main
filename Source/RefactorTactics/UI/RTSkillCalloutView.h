#pragma once

#include "CoreMinimal.h"
#include "RTSkillCalloutView.generated.h"

/**
 * Il fumetto che compare sopra un'unita' quando il playback rivela l'attivazione di una skill di firma (`#4025`).
 *
 * 🔴 **Solo presentazione: nessun hash.** Questa struttura non entra in `MapState`, nello snapshot, nel TurnLog
 * ne' in `StateHash`: e' costruita da un fatto gia' risolto (l'attivazione della skill). Un golden non puo'
 * cambiare per colpa sua, e se cambia il difetto e' altrove.
 *
 * 🔑 **Porta le cose che il widget non deve ricalcolare**: chi grida, che parola compare e da quando. Il tempo
 * di vita NON e' un campo: lo detta `ARTTurnManager::PlaybackElapsedTotal` meno `StartAt`, mai un `DeltaTime`
 * locale, cosi' il fumetto segue il tempo del playback e non quello del frame.
 *
 * Costruttore e curve del tempo: `URTHudViewModel::BuildSkillCallout`, `SkillCalloutAlpha`, `SkillCalloutScale`.
 */
USTRUCT(BlueprintType)
struct FRTSkillCalloutView
{
	GENERATED_BODY()

	/**
	 * Chi GRIDA, come `ARTUnit::StableUnitId`.
	 *
	 * ⚠️ **`0` non e' l'unita' numero zero: e' «nessuno»** ([D-063]). `EnsureMatchRoster` assegna gli id a
	 * partire da `1` e lascia lo zero libero apposta: un fumetto attribuito allo zero non combacia con nessuna
	 * unita' e quindi non compare, che e' l'effetto voluto.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 SourceStableUnitId = 0;

	/** L'azione che grida. `None` = nessun fumetto registrato: la vista di default non dice niente. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName ActionId;

	/** Il testo gia' composto. Il maiuscolo lo applica il widget: e' presentazione, non contenuto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FString Label;

	/**
	 * Vero quando il kit non ha dato un nome alla skill e il testo e' il ripiego (l'ultimo segmento dell'id).
	 * 🔑 **E' un campo e non un confronto fra `Label` e `ActionId`**: chi vuole segnalare un nome mancante lo
	 * legge qui, senza dedurlo da una stringa.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bFromFallback = false;

	/** `PlaybackElapsedTotal` al momento della comparsa: l'origine del tempo di vita del fumetto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float StartAt = 0.f;
};
