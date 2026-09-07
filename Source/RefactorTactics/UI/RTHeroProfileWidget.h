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

	/**
	 * Il testo con cui la scheda dice «questo dato non c'e'».
	 *
	 * ⚠️ **Una costante e non un letterale sparso**: e' la forma visibile del fail-closed, e se ogni
	 * sezione scrivesse il proprio segnaposto la scheda mostrerebbe tre convenzioni diverse per la stessa
	 * assenza — e prima o poi una di esse sarebbe uno `0`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	static FText GetAbsentValueText();

protected:
	/**
	 * La vista corrente. `EditAnywhere` per consentire una **preview di design-time** dentro il WBP con
	 * valori di fixture; in gioco arriva sempre da `SetProfileView`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HeroProfile")
	FRTHeroProfileView ProfileView;

	// ------------------------------------------------------------------------------------------------
	// I widget del `WBP_RT_HeroProfile`, collegati PER NOME.
	//
	// 🔴 **`BindWidgetOptional` invece di binding authorati nel Blueprint, ed e' la scelta che rende
	// questa scheda verificabile.** Un binding disegnato dentro il `.uasset` vive in un
	// `FDelegateRuntimeBinding` serializzato che non si diffa, non si grep-pa e si rompe in silenzio
	// (#937 esiste per un modale che si armava e non compariva). Popolare i testi da C++ mette la stessa
	// regola in un posto che un Automation Test puo' leggere.
	//
	// ⚠️ **`Optional` e non obbligatorio**: un WBP che non dichiara uno di questi nomi resta valido e
	// compila. Sono una scheda graybox e le sue sezioni sono opzionali per contratto — pretenderli tutti
	// trasformerebbe ogni variante di layout in un errore di compilazione del Blueprint.
	//
	// 🔵 Questi puntatori sono a WIDGET, non a gameplay: il confine che
	// `NoGameplayPointersInView` difende riguarda la VIEW, che resta senza puntatori di qualunque tipo.
	// ------------------------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> HeroName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> RoleLine;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> StyleTagLine;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> AffinityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> WeaknessText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> RangeAndDifficulty;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> CombatIdentity;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> StrengthsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class UTextBlock> TradeoffsText;

	/** Il radar della scheda. La view gli passa gli assi; la scheda non ne disegna nessuno. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HeroProfile")
	TObjectPtr<class URTHeroRadarWidget> HeroRadar;

	/**
	 * Riversa `ProfileView` nei widget collegati. Chiamata da `SetProfileView` e da `NativeConstruct`,
	 * perche' la view puo' arrivare **prima** che i widget esistano — ed e' il caso normale quando la
	 * scheda viene costruita gia' popolata.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroProfile")
	void RefreshBoundWidgets();

	virtual void NativeConstruct() override;
};
