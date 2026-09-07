#include "RTLabViewModel.h"

#include "ScenarioHarness/RTScenarioRunner.h"

void FRTLabViewModel::SetHeroFilter(const FName& InHeroId)
{
	if (HeroFilter == InHeroId)
	{
		return;
	}

	HeroFilter = InHeroId;

	// La selezione sopravvive al cambio di filtro **solo** se e' ancora visibile. Senza questa riga il
	// pannello mostrerebbe il readout di un'ability che non appartiene al kit elencato: numeri veri, che
	// pero' non rispondono a cio' che si sta guardando.
	if (!SelectedAbility.IsNone() && !IsVisible(SelectedAbility))
	{
		SelectedAbility = NAME_None;
		// Anche l'esito era di quell'ability: lasciarlo sarebbe un TurnLog orfano sotto un selettore vuoto.
		Result = FRTLabRunResult();
	}
}

TArray<FRTAbilityLabEntry> FRTLabViewModel::VisibleAbilities() const
{
	// Il filtro non e' una condizione dentro un elenco costruito qui: sono le due funzioni canoniche, e
	// la seconda e' gia' un filtro sulla prima.
	return HasHeroFilter()
		? URTHeroLabLibrary::ListHeroKit(HeroFilter)
		: URTAbilityLabLibrary::ListCanonicalAbilities();
}

bool FRTLabViewModel::GetHeroReadout(FRTHeroLabEntry& OutHero) const
{
	if (!HasHeroFilter())
	{
		return false;
	}
	return URTHeroLabLibrary::FindHero(HeroFilter, OutHero);
}

bool FRTLabViewModel::IsVisible(const FName& AbilityId) const
{
	if (AbilityId.IsNone())
	{
		return false;
	}
	for (const FRTAbilityLabEntry& Entry : VisibleAbilities())
	{
		if (Entry.AbilityId == AbilityId)
		{
			return true;
		}
	}
	return false;
}

bool FRTLabViewModel::SelectAbility(const FName& InAbilityId)
{
	// ⛔ Fuori dall'elenco visibile si rifiuta, e la selezione precedente **resta**. Un rifiuto che
	// azzerasse la selezione punirebbe il click sbagliato con la perdita di quello giusto di prima.
	if (!IsVisible(InAbilityId))
	{
		return false;
	}

	SelectedAbility = InAbilityId;
	Result = FRTLabRunResult();
	return true;
}

ERTActionReadoutResult FRTLabViewModel::DescribeSelection(TArray<FRTActionParameterView>& OutParameters) const
{
	// Delega intera: il readout ha una casa sola, e sa dire da dove ogni numero viene.
	return URTAbilityLabLibrary::DescribeAbility(SelectedAbility, OutParameters);
}

bool FRTLabViewModel::BuildScenario(FRTTestScenario& OutScenario, FString& OutError) const
{
	if (SelectedAbility.IsNone())
	{
		OutError = TEXT("Nessuna ability selezionata: non c'e' niente da eseguire.");
		return false;
	}

	// Le due sole strade, e nessuna terza. Con il filtro passa dalla verifica d'appartenenza di #2600,
	// che poi delega alla stessa `BuildFixture` dell'altro ramo.
	return HasHeroFilter()
		? URTHeroLabLibrary::BuildHeroFixture(HeroFilter, SelectedAbility, Spec, OutScenario, OutError)
		: URTAbilityLabLibrary::BuildFixture(SelectedAbility, Spec, OutScenario, OutError);
}

bool FRTLabViewModel::Run(UWorld* World, FString& OutError)
{
	Result = FRTLabRunResult();

	if (!World)
	{
		OutError = TEXT("Nessun mondo su cui eseguire la fixture.");
		Result.Error = OutError;
		return false;
	}

	FRTTestScenario Scenario;
	if (!BuildScenario(Scenario, OutError))
	{
		// Fail closed e **visibile**: l'errore finisce nell'esito, non solo nel valore di ritorno. Un
		// pannello che ignorasse il ritorno mostrerebbe altrimenti l'esito della run precedente.
		Result.Error = OutError;
		return false;
	}

	const FRTTestResult RunResult = URTScenarioRunner::Run(World, Scenario);

	Result.bHasRun = true;
	Result.Outcome = RunResult.OutcomeString();
	Result.Error = RunResult.ErrorMessage;
	Result.TurnsPlayed = RunResult.TurnsPlayed;
	// Il before/after e il TurnLog sono quelli che il runner produce: nessuna riga composta qui.
	Result.Diffs = RunResult.StateDiff;
	Result.TurnLogLines = URTAbilityLabLibrary::DescribeRunTurnLog(RunResult);

	if (RunResult.Outcome == ERTTestOutcome::Error)
	{
		OutError = RunResult.ErrorMessage;
		return false;
	}

	return true;
}
