#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "RTScenarioAuthoring.generated.h"

/**
 * **La porta Blueprint dello Scenario Harness.** L'unica.
 *
 * `URTScenarioLoader`, `URTScenarioRunner` e `FRTScenarioSession` restano C++ e non sono raggiungibili da
 * Blueprint: questa classe li chiama per conto della UI e ne traduce gli esiti. Decisione registrata in
 * [ADR-0010](../../../docs/decisions/adr-0010-esposizione-blueprint-scenario-harness.md).
 *
 * ⚠️ **Cosa Blueprint NON puo' fare, per costruzione e non per disciplina:**
 *
 * - costruire un `FRTTestScenario`: le nove `USTRUCT` del formato non sono `BlueprintType`, quindi non
 *   esistono come pin;
 * - scrivere JSON: la serializzazione passa da `URTScenarioLoader::SaveToFile` e da nessun altro posto;
 * - decidere se uno scenario e' valido: lo decide `Validate`, e questa classe riporta la risposta;
 * - modificare il modello attraverso una vista: `FRTScenarioSummary` e `FRTScenarioUnitView` sono
 *   fotografie, e cambiarle non cambia niente.
 *
 * ⚠️ **E cosa questa classe non deve mai diventare**: un posto dove vive una regola di gioco. Traduce. Se si
 * trovasse a decidere un esito, un costo o una raggiungibilita', quella decisione sarebbe del runtime e
 * andrebbe spostata — e' il §3 di `spec-tactical-designer.md`.
 *
 * **Perche' un `UObject` e non un subsystem**: `#1115` colloca il Composer nel viewport dell'Editor, dove non
 * esiste una `GameInstance`. Un oggetto creato da factory funziona in Editor, in PIE e headless nei test, e
 * il widget lo tiene come variabile. Vedi ADR-0010 §4.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTScenarioAuthoring : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Crea un draft vuoto. Il chiamante ne diventa proprietario: un widget lo tiene come variabile, e piu'
	 * draft possono convivere senza che nulla sia globale.
	 *
	 * `Outer` esplicito perche' un oggetto d'authoring creato in Editor deve poter appartenere a chi lo usa;
	 * `nullptr` lo mette sotto il transient package, che e' il default sensato per uno strumento.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	static URTScenarioAuthoring* CreateScenarioDraft(UObject* Outer);

	/** Uno scenario nuovo. Non e' valido finche' non ha unita' e assertion, ed e' corretto cosi'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	void NewScenario(const FString& ScenarioId, int32 MapRadius = 3);

	/**
	 * Apre uno scenario per **ID**, chiedendo all'indice dove vive. L'identita' e' dichiarata dal file: il
	 * percorso e' un dettaglio di storage e non si compone a mano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult OpenById(const FString& ScenarioId, FString& OutError);

	/** Apre da un percorso esplicito, per i file che l'indice non copre. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult OpenFromFile(const FString& FilePath, FString& OutError);

	/** Chiude senza salvare. Le operazioni successive rispondono `NoScenarioOpen`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	void Close();

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	bool IsOpen() const { return Draft.IsOpen(); }

	/** L'unico giudice della validita' e' `URTScenarioLoader::Validate`: qui se ne riporta la risposta. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult Validate(FString& OutError) const;

	/**
	 * Salva nel percorso dato. **Esplicito**: nessuna scrittura implicita a ogni modifica.
	 *
	 * Uno scenario invalido non tocca il disco, e i due esiti restano distinti — `Invalid` accusa lo
	 * scenario, `WriteFailed` accusa il disco.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SaveToFile(const FString& FilePath, FString& OutError);

	/** Salva dove lo scenario gia' viveva. `NotFound` se non e' mai stato su disco. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SaveInPlace(FString& OutError);

	/** Fotografia dell'intestazione. Modificarla non modifica lo scenario. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	FRTScenarioSummary GetSummary() const { return Draft.GetSummary(); }

	/** Le unita' schierate, nell'ordine del file. Fotografie: vedi `GetSummary`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	TArray<FRTScenarioUnitView> ListUnits() const { return Draft.ListUnits(); }

	/** Gli ID che l'indice conosce, filtrabili per tag. Vuoti = elenco completo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static TArray<FString> ListScenarioIds(const FString& FilterTagA, const FString& FilterTagB);

	/** I tag esistenti nel corpus, per costruire un filtro senza scriverli a mano. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static TArray<FString> ListScenarioTags();

	/** Frase leggibile per un esito, da mostrare accanto all'icona. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static FText DescribeResult(ERTScenarioAuthoringResult Result);

	/** Il draft C++ sottostante. Per il codice C++ e i test: **non** e' esposto a Blueprint. */
	FRTScenarioDraft& GetDraft() { return Draft; }
	const FRTScenarioDraft& GetDraft() const { return Draft; }

private:
	/**
	 * La logica sta qui dentro, non in questa classe. La facade traduce e basta — e' la separazione che
	 * rende i test headless possibili senza costruire un `UObject`, la stessa di `FRTReplayViewModel`.
	 */
	FRTScenarioDraft Draft;
};
