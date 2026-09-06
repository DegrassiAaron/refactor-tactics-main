// Ability Lab (#2599) — eseguire UNA ability canonica in una fixture deterministica.
//
// ## Perche' e' una facade e non un motore
//
// Lo Scenario Harness esprime gia' tutto cio' che serve: `FRTScenarioUnit` posa caster e bersaglio,
// `FRTScenarioIntent::Ability` dichiara *quale* ability eseguire, `FRTTestScenario::Seed` fissa il
// determinismo, e `URTScenarioRunner::Run` esegue il percorso reale. Il before/after esiste come
// `FRTTestResult::UnitStateDiff`; il TurnLog esiste come `FRTTestResult::TurnTraces`.
//
// ∴ questo file **compone**, non decide. L'invariante che #2599 chiede — *«mai un mini-resolver
// separato»* — qui non e' una convenzione da rispettare: e' strutturale, perche' questa libreria non ha
// un altro modo di far succedere qualcosa. Non esiste una funzione che applichi un effetto, calcoli un
// danno o legga una portata per deciderne l'esito.
//
// ⚠️ **Nessuna formula di gameplay sotto questo header.** Se un giorno ne comparisse una, il difetto non
// sarebbe l'aritmetica: sarebbe la seconda risposta alla stessa domanda.
//
// ## Cosa NON e'
//
// Non e' lo Skill Workbench (`TD 0.3`, #1950, `out_of_release_scope`): quello **autora una variante**
// senza toccare il dato di produzione. Questo **esegue il dato di produzione** e non lo scrive mai.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionReadout.h"
#include "Map/RTCellId.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "RTAbilityLab.generated.h"

/**
 * Una voce del selettore: l'identita' di una ability canonica piu' il minimo che serve a sceglierla.
 *
 * Non porta valori di bilanciamento oltre a forma, portata e raggio — quelli si leggono da
 * `URTActionReadoutLibrary::DescribeActionParameters`, che e' la loro unica casa e sa dire da **dove**
 * ogni numero viene. Duplicarli qui creerebbe la quarta casa che `#1953` ha misurato.
 */
USTRUCT(BlueprintType)
struct FRTAbilityLabEntry
{
	GENERATED_BODY()

	/** `Hero.Gadget.LinearDischarge`, `Action.Move`, ... */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FName AbilityId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FText DisplayName;

	/** L'eroe nel cui kit vive. `NAME_None` per un'azione core, che non e' di nessuno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FName OwnerHeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	bool bIsCoreAction = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	int32 RangeCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	int32 AreaRadius = 0;
};

/**
 * Come si vuole la fixture. Tutti i campi hanno un default utilizzabile: il caso normale e' non
 * dichiarare niente e ottenere una posa a due unita' con un seed fisso.
 */
USTRUCT(BlueprintType)
struct FRTAbilityLabFixtureSpec
{
	GENERATED_BODY()

	/** Fissa il determinismo. Due `BuildFixture` con lo stesso seed producono lo stesso scenario. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	int32 Seed = 1;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	int32 MapRadius = 3;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FRTCellId CasterCell = FRTCellId(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FRTCellId TargetCell = FRTCellId(2, 0);

	/**
	 * L'eroe che subisce. Deve essere diverso dal caster, altrimenti la posa mette due unita' della
	 * stessa identita' in due squadre — leggibile per il motore, confondente per chi guarda.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FName TargetHeroId = FName(TEXT("Hero.Wraith"));
};

UCLASS()
class REFACTORTACTICS_API URTAbilityLabLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'insieme canonico delle ability: l'**unione** dei kit d'eroe (`URTHeroCatalogLibrary::GetHeroRoster`
	 * → `URTHeroData::Actions`) e del catalogo core (`URTCatalogLibrary::GetCoreActionCatalog`).
	 *
	 * ⚠️ Sono due sorgenti e non una: il corpo di #2599 diceva *«via `URTCatalogLibrary`»*, e quella
	 * libreria conosce solo le azioni core. Un selettore costruito su di essa non mostrerebbe **nessuna**
	 * ability d'eroe — cioe' esattamente cio' che l'Ability Lab esiste per eseguire.
	 *
	 * Ordine stabile: prima i kit nell'ordine del roster, poi le core nell'ordine del catalogo. Nessun
	 * `Sort` su `FName`, che varia con l'ordine di creazione dei nomi.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static TArray<FRTAbilityLabEntry> ListCanonicalAbilities();

	/** La voce di `AbilityId`, se canonica. `false` senza toccare `OutEntry` quando non lo e'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static bool FindAbility(const FName& AbilityId, FRTAbilityLabEntry& OutEntry);

	/**
	 * Il readout dei parametri, delegato per intero a `URTActionReadoutLibrary`.
	 *
	 * Non riassume e non sceglie: il designer vede **entrambe** le case di ogni numero — cio' che il
	 * catalogo dichiara e cio' che il consumatore legge — perche' uno strumento che ne scegliesse una
	 * mostrerebbe un valore che il gioco puo' non usare.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static ERTActionReadoutResult DescribeAbility(const FName& AbilityId,
		TArray<FRTActionParameterView>& OutParameters);

	/**
	 * Costruisce lo scenario che esegue `AbilityId` una volta sola.
	 *
	 * **Fail closed**: se `AbilityId` non e' canonica, ritorna `false`, scrive `OutError` e lascia
	 * `OutScenario` intatto. Non produce una fixture parziale — una fixture a meta' verrebbe eseguita, e
	 * il suo esito sarebbe indistinguibile da quello di un'ability che semplicemente non fa nulla.
	 *
	 * Lo scenario porta sempre almeno una `Expect`: l'harness **rifiuta** uno scenario senza assertion,
	 * perche' passerebbe sempre. L'assertion di default e' `TurnsCompleted == 1` — dice che il turno e'
	 * stato giocato, e non pretende un esito di gameplay che il Lab non ha il compito di decidere.
	 */
	// ⛔ **Deliberatamente NON una `UFUNCTION`.** `FRTTestScenario` e' `USTRUCT()` e **non**
	// `BlueprintType`: ADR-0010 tiene le nove struct del formato fuori dalla portata di Blueprint, e il
	// gate `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint` verifica **entrambi i
	// versi**. Esporre questa firma renderebbe rosso quel gate — cioe' il Lab comprerebbe una comodita'
	// d'editor al prezzo dell'invariante che lo Scenario Harness esiste per difendere.
	static bool BuildFixture(const FName& AbilityId, const FRTAbilityLabFixtureSpec& Spec,
		FRTTestScenario& OutScenario, FString& OutError);

	/**
	 * Il TurnLog della run, in righe leggibili.
	 *
	 * Percorso: `FRTTestResult::TurnTraces` (byte canonici) → `URTTurnLogLibrary::DeserializeTurnLog` →
	 * `DescribeTurnLog`. Nessuna riga viene composta qui: le stringhe sono quelle che il TurnLog produce
	 * per chiunque altro.
	 */
	// ⛔ **Deliberatamente NON una `UFUNCTION`.** `RTTestResult.h` non dichiara **nessuna** `USTRUCT`:
	// `FRTTestResult` e `FRTTurnTrace` sono struct C++ nude, invisibili alla riflessione. UHT lo dice in
	// otto secondi — *«Unable to find 'class', 'delegate', 'enum', or 'struct' with name
	// 'FRTTestResult'»* — e ha ragione: il risultato di una run e' un oggetto di dominio, non un DTO.
	static TArray<FString> DescribeRunTurnLog(const FRTTestResult& Result);
};
