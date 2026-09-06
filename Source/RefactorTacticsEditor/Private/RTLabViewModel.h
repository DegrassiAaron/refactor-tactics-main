// Il modello del Lab: un pannello solo, e l'eroe e' un filtro (#2599 Fetta B, #2600).
//
// ## Perche' un modello separato dal widget
//
// 🔑 **Esiste separato per una ragione sola: cosi' resta qualcosa da misurare.** Slate su un editor vivo
// non lo vede nessun automation test — `RTDevSandboxLauncherTests.cpp` lo dichiara in testa per il proprio
// pannello, e `FRTAnimBrowserModel` esiste per lo stesso motivo. Se filtro, selezione ed esecuzione
// vivessero dentro `SRTLabPanel`, l'intera fetta sarebbe a carico dell'occhio umano.
//
// ⛔ **Non conosce Slate.** Nessun `SWidget`, nessun `FSlateBrush`: il pannello lo interroga, non il
// contrario.
//
// ## Perche' un pannello solo, e non due
//
// `URTHeroLabLibrary::ListHeroKit` **e'** un filtro su `URTAbilityLabLibrary::ListCanonicalAbilities`, e
// `BuildHeroFixture` verifica l'appartenenza e poi **delega** a `BuildFixture`. Due pannelli distinti
// ripeterebbero selettore, readout, Run, before/after e vista del TurnLog — l'80% della superficie, in due
// posti che divergono al primo campo aggiunto a uno solo dei due.
//
// ```text
// filtro eroe vuoto      -> l'elenco e' il catalogo canonico   -> #2599
// filtro eroe impostato  -> l'elenco e' il kit di quell'eroe   -> #2600
// ```
//
// ⛔ **Nessuna formula di gameplay sotto questo header.** Il modello compone e delega: non calcola un
// danno, non legge una portata per deciderne l'esito, non conosce la topologia esagonale.

#pragma once

#include "CoreMinimal.h"
#include "Ability/RTAbilityLab.h"
#include "Ability/RTHeroLab.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"

class UWorld;

/** L'esito dell'ultima esecuzione, nella forma che il pannello mostra. */
struct FRTLabRunResult
{
	/** `false` finche' non si e' eseguito nulla. Distingue «non ho ancora corso» da «ho corso e non e' uscito niente». */
	bool bHasRun = false;

	/** `PASS` · `FAIL` · `BLOCKED` · `ERROR`, come `FRTTestResult::OutcomeString`. */
	FString Outcome;

	/** Il motivo, quando la fixture non e' stata nemmeno costruita o la run e' andata in errore. */
	FString Error;

	int32 TurnsPlayed = 0;

	/** Il TurnLog canonico in righe leggibili — `URTAbilityLabLibrary::DescribeRunTurnLog`. */
	TArray<FString> TurnLogLines;

	/** Il before/after, cosi' come il runner lo produce. Nessun campo ricalcolato qui. */
	TArray<FRTUnitStateDiff> Diffs;
};

class FRTLabViewModel
{
public:
	// ── Filtro ──────────────────────────────────────────────────────────────────────────────────────

	/**
	 * `NAME_None` = tutto il catalogo canonico (#2599). Un `HeroId` = il kit di quell'eroe (#2600).
	 *
	 * ⚠️ **Cambiare filtro azzera una selezione che non gli appartiene.** Senza questo, il readout
	 * resterebbe quello dell'ability precedente mentre l'elenco mostra un altro kit — e il pannello
	 * direbbe numeri che non corrispondono a cio' che si sta guardando.
	 */
	void SetHeroFilter(const FName& InHeroId);

	FName GetHeroFilter() const { return HeroFilter; }
	bool HasHeroFilter() const { return !HeroFilter.IsNone(); }

	/** L'elenco che il selettore mostra: catalogo intero o kit, secondo il filtro. */
	TArray<FRTAbilityLabEntry> VisibleAbilities() const;

	/** L'identita' dell'eroe filtrato. `false` senza filtro: non c'e' un eroe da descrivere. */
	bool GetHeroReadout(FRTHeroLabEntry& OutHero) const;

	// ── Selezione ───────────────────────────────────────────────────────────────────────────────────

	/**
	 * Sceglie l'ability da eseguire.
	 *
	 * ⛔ **Rifiuta cio' che non e' nell'elenco visibile**, e la selezione precedente resta intatta. Con un
	 * filtro attivo questo e' il posto in cui la regola d'appartenenza di #2600 diventa visibile: non e'
	 * una convenzione della UI, e' la stessa domanda che `BuildHeroFixture` pone.
	 */
	bool SelectAbility(const FName& InAbilityId);

	FName GetSelectedAbility() const { return SelectedAbility; }

	/** Il readout dei parametri della selezione, delegato a `URTActionReadoutLibrary`. */
	ERTActionReadoutResult DescribeSelection(TArray<FRTActionParameterView>& OutParameters) const;

	// ── Esecuzione ──────────────────────────────────────────────────────────────────────────────────

	/**
	 * La fixture che verrebbe eseguita adesso.
	 *
	 * Con filtro passa da `URTHeroLabLibrary::BuildHeroFixture`, senza filtro da
	 * `URTAbilityLabLibrary::BuildFixture`. **E' pubblica perche' sia verificabile**: che il modello non
	 * costruisca una terza fixture propria si prova confrontando questa con quelle due, non leggendo il
	 * codice.
	 */
	bool BuildScenario(FRTTestScenario& OutScenario, FString& OutError) const;

	/**
	 * Esegue la fixture nel runtime reale.
	 *
	 * ⚠️ Il `UWorld` arriva dal **chiamante**: il pannello ne crea uno transitorio, i test il loro. Il
	 * modello non conosce `GEditor` — se lo cercasse, smetterebbe di essere provabile headless.
	 */
	bool Run(UWorld* World, FString& OutError);

	const FRTLabRunResult& LastRun() const { return Result; }

	/** Azzera l'esito. Il filtro e la selezione restano: non e' un reset del pannello. */
	void ClearRun() { Result = FRTLabRunResult(); }

	/** La spec della fixture, esposta perche' il pannello possa offrire seed e posa. */
	FRTAbilityLabFixtureSpec& MutableSpec() { return Spec; }
	const FRTAbilityLabFixtureSpec& GetSpec() const { return Spec; }

private:
	/** `true` se `AbilityId` compare fra le `VisibleAbilities()` correnti. */
	bool IsVisible(const FName& AbilityId) const;

	FName HeroFilter;
	FName SelectedAbility;
	FRTAbilityLabFixtureSpec Spec;
	FRTLabRunResult Result;
};
