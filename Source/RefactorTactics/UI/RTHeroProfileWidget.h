#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHeroProfileView.h"
#include "RTHeroProfileWidget.generated.h"

/**
 * La base del Profilo Tattico: riceve una vista gia' risolta e la espone al Blueprint.
 *
 * ⚠️ **Qui non c'e' layout**, come in `RTScreenHudWidgets.h`: il `.uasset` `WBP_RT_HeroProfile` decide
 * aspetto e disposizione, questa classe decide **cosa il widget puo' leggere**.
 *
 * 🔴 **Non acquisisce contesto di partita, e non e' una dimenticanza.** In questo file non compare
 * `ARTTurnManager`, `ARTUnit`, `APlayerController` ne' un catalogo: il profilo deve poter vivere in
 * Character Select, in una preview di Wiki o in un inspector, cioe' dove una partita non esiste. Un
 * accessor al match lo renderebbe riusabile solo dentro il match — l'esatto contrario dell'outcome.
 *
 * 🔵 **Le domande `Has*` esistono per il fail-closed.** Senza di esse ogni WBP dovrebbe scrivere da solo
 * la condizione «mostra oppure no», e prima o poi qualcuno disegnerebbe un asse mancante come zero o
 * dedurrebbe una proficiency dall'affinita'. Con esse la regola sta in un posto solo.
 */
UCLASS(BlueprintType, Blueprintable)
class REFACTORTACTICS_API URTHeroProfileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Sostituisce la vista e notifica il Blueprint.
	 *
	 * ⚠️ **Il widget non valida e non corregge.** Chi fornisce la view e' responsabile della sua coerenza
	 * (`URTHeroProfileLibrary::ValidateProfileView` dice se lo e'); sanare qui in silenzio nasconderebbe
	 * un dato rotto invece di mostrarlo a chi puo' ripararlo.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroProfile")
	void SetProfileView(const FRTHeroProfileView& InProfileView);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	const FRTHeroProfileView& GetProfileView() const { return ProfileView; }

	/** Chiamato dopo ogni `SetProfileView`: e' il posto in cui il Blueprint riallinea i suoi binding. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HeroProfile")
	void OnProfileViewChanged();

	// ------------------------------------------------------------------------------------------------
	// Fail-closed: cosa si puo' mostrare. Ogni `false` = sezione collassata o «—», mai un valore dedotto.
	// ------------------------------------------------------------------------------------------------

	/** Vero se c'e' almeno un asse e tutti sono rappresentabili. Vedi `URTHeroRadarWidget`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasRadar() const;

	/** Vero se esiste un ruolo primario. ⚠️ Un profilo senza ruolo e' lo stato NORMALE finche' nessun
	 *  owner runtime pubblica la tassonomia: la scheda lo tollera invece di inventare un'etichetta. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasPrimaryRole() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasSecondaryRole() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasAffinity() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasWeakness() const;

	/** ⚠️ Indipendente da `HasAffinity()`: un'affinita' dichiarata non implica nessuna proficiency. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasElementalProficiencies() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasElementRelations() const;

	/** Vero quando almeno una delle due difficolta' e' dichiarata (`> 0`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	bool HasDifficultyRatings() const;

protected:
	/**
	 * La vista corrente. `EditAnywhere` per consentire una **preview di design-time** dentro il WBP con
	 * valori di fixture; in gioco arriva sempre da `SetProfileView`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HeroProfile")
	FRTHeroProfileView ProfileView;
};
