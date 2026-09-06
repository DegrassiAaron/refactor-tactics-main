// Hero Lab (#2600) — caricare un eroe canonico e provare il suo kit reale.
//
// ## Il rapporto con #2599, che e' l'unica cosa che questa libreria non negozia
//
// Hero Lab **non esegue niente da solo**: costruisce la fixture chiamando
// `URTAbilityLabLibrary::BuildFixture`. Il non-goal *«un secondo Ability Runner»* non e' una promessa da
// mantenere con la disciplina — e' una proprieta' del codice, perche' qui non esiste un percorso alternativo.
//
// Cio' che Hero Lab aggiunge e' l'unica domanda che #2599 non puo' porre: *«questa ability e' DI questo
// eroe?»*. Per l'Ability Lab ogni ability canonica e' eseguibile; per l'Hero Lab conta anche di chi sia.
//
// ## Cosa NON e'
//
// Non e' un roster editor, non autora eroi ne' abilita', non introduce progression, talenti o loadout.
// Legge `URTHeroCatalogLibrary` e lo mostra.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTAbilityLab.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "RTHeroLab.generated.h"

/**
 * L'identita' di un eroe canonico, piu' le stat e i token che il catalogo dichiara davvero.
 *
 * ⚠️ **Niente `FGameplayTag`, ed e' una constatazione, non una scelta di design.** `URTHeroData` non
 * dichiara nessun campo tag: `Affinity`, `Weakness` e `ReactionProfileId` sono `FName` a **forma** di tag
 * (`Affinity.Electricity`, `Profile.Grounding`). Introdurre veri Gameplay Tag qui vorrebbe dire inventare un
 * asse che il gioco non ha, e la 0.1 di uno strumento non e' il posto dove nasce un asse di gameplay.
 */
USTRUCT(BlueprintType)
struct FRTHeroLabEntry
{
	GENERATED_BODY()

	/** `Hero.Gadget`, `Hero.Phase`, `Hero.Branth`, `Hero.Wraith`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 MaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 MovePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 VisionRange = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 HearingThreshold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 PushResistance = 0;

	/** Token a forma di tag, non `FGameplayTag`. Vedi la nota sulla struct. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	FName Affinity;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	FName Weakness;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	FName ReactionProfileId;

	/** Quante voci il kit dichiara. L'elenco si chiede a `ListHeroKit`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HeroLab")
	int32 DeclaredAbilityCount = 0;
};

UCLASS()
class REFACTORTACTICS_API URTHeroLabLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I quattro eroi canonici, nell'ordine del catalogo: Gadget, Phase, Branth, Wraith.
	 *
	 * ⚠️ `Hero.Riktor` **non esiste**: `D-334` ha rinominato l'identita' in `Hero.Branth`. Gli **asset**
	 * conservano il nome vecchio (`/Game/RT/Characters/Riktor/`), e `RTGameMode.cpp` dichiara che non e' un
	 * refuso. Questa libreria indicizza per identita', mai per path d'asset.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroLab")
	static TArray<FRTHeroLabEntry> ListCanonicalHeroes();

	/** La voce di `HeroId`, se canonico. `false` senza toccare `OutEntry` quando non lo e'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroLab")
	static bool FindHero(const FName& HeroId, FRTHeroLabEntry& OutEntry);

	/**
	 * Il kit dichiarato dall'eroe, nelle stesse voci che l'Ability Lab usa.
	 *
	 * Il tipo di ritorno e' quello di #2599 di proposito: un `FRTHeroLabAbility` parallelo sarebbe una
	 * seconda descrizione della stessa ability, e divergerebbe al primo campo aggiunto a uno solo dei due.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroLab")
	static TArray<FRTAbilityLabEntry> ListHeroKit(const FName& HeroId);

	/**
	 * La fixture che fa eseguire `AbilityId` **a quell'eroe**.
	 *
	 * Verifica l'appartenenza — l'unica cosa che #2599 non puo' controllare — e poi **delega**:
	 * `URTAbilityLabLibrary::BuildFixture` costruisce, e questa funzione non tocca il risultato. Se un
	 * giorno costruisse anche un solo campo per conto proprio, il secondo runner sarebbe nato qui.
	 *
	 * ⛔ Non e' una `UFUNCTION`, per la stessa ragione di #2599: `FRTTestScenario` e' `USTRUCT()` e non
	 * `BlueprintType`, e il gate `Scenario.AuthoringContractIsReachableFromBlueprint` verifica entrambi i versi.
	 */
	static bool BuildHeroFixture(const FName& HeroId, const FName& AbilityId,
		const FRTAbilityLabFixtureSpec& Spec, FRTTestScenario& OutScenario, FString& OutError);
};
