#include "RTLabViewModel.h"

#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

void FRTLabViewModel::SetHeroFilter(const FName& InHeroId)
{
	if (HeroFilter == InHeroId)
	{
		return;
	}

	HeroFilter = InHeroId;
	// Il filtro e' cambiato davvero (il ritorno anticipato sopra lascia l'Id dov'e'): e' un gesto.
	ClearLaunchStatus();

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

FRTLabViewModel::EFilterState FRTLabViewModel::DescribeFilterState(FRTHeroLabEntry& OutHero) const
{
	if (!HasHeroFilter())
	{
		return EFilterState::NoFilter;
	}
	if (!URTHeroLabLibrary::FindHero(HeroFilter, OutHero))
	{
		return EFilterState::UnknownHeroId;
	}
	// ⚠️ Il conteggio si chiede a `VisibleAbilities()`, che e' cio' che il selettore mostra davvero --
	// non a `OutHero.DeclaredAbilityCount`, che e' un campo del catalogo. I due possono divergere, e la
	// domanda di questa funzione e' **perche' la lista e' vuota**, non quante voci il catalogo dichiari.
	return VisibleAbilities().Num() == 0
		? EFilterState::HeroWithEmptyKit
		: EFilterState::HeroWithKit;
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
	ClearLaunchStatus();
	return true;
}

void FRTLabViewModel::ClearLaunchStatus()
{
	LaunchedId.Reset();
	bLaunchFinishedOnce = false;
}

void FRTLabViewModel::NoteLaunched(const FString& Id)
{
	LaunchedId = Id;
	bLaunchFinishedOnce = false;
}

void FRTLabViewModel::NoteLaunchFinished()
{
	LaunchedId.Reset();
	bLaunchFinishedOnce = true;
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
	// Una run e' un gesto: la riga di stato mostra l'esito di questa, non il lancio PIE di prima.
	ClearLaunchStatus();

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

bool FRTLabViewModel::PrepareForPie(FString& OutScenarioId, FString& OutError)
{
	OutScenarioId.Reset();
	OutError.Reset();

	FRTTestScenario Scenario;
	if (!BuildScenario(Scenario, OutError))
	{
		return false;
	}

	// 🔴 Radice vuota = sotto automation, senza override: non esiste e non si crea. Scrivere comunque
	// `MakeDirectory("")` + un percorso relativo sporcherebbe la cartella corrente.
	const FString Root = URTScenarioLoader::LabScenariosRoot();
	if (Root.IsEmpty())
	{
		OutError = TEXT("la radice del Lab non e' disponibile sotto automation senza override: niente da scrivere");
		return false;
	}
	IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
	const FString Percorso = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(Root, Scenario.ScenarioId + TEXT(".json")));

	// `SaveToFile` valida prima di toccare il disco: una fixture invalida non lascia un file a meta'.
	if (!URTScenarioLoader::SaveToFile(Scenario, Percorso, OutError))
	{
		return false;
	}

	// 🔑 Lanciabile = l'indice risolve l'Id a QUESTO file. Un doppione altrove rende l'Id ambiguo, e il
	// GameMode lo rifiuterebbe a schermo senza che il pannello potesse dirlo prima.
	FString ErroreIndice;
	const FString Risolto = URTScenarioIndex::ResolvePath(Scenario.ScenarioId, ErroreIndice);
	if (Risolto.IsEmpty())
	{
		// Un file che rende ambiguo un Id avvelenerebbe la console e il GameMode a ogni clic: si toglie.
		IFileManager::Get().Delete(*Percorso);
		OutError = FString::Printf(TEXT("fixture scritta in '%s' ma non lanciabile: %s; il file appena scritto e' stato rimosso"),
			*Percorso, *ErroreIndice);
		return false;
	}
	if (!FPaths::IsSamePath(Risolto, Percorso))
	{
		// Stessa ragione del ramo sopra: un file che non e' quello a cui l'Id risolve non deve restare.
		IFileManager::Get().Delete(*Percorso);
		OutError = FString::Printf(TEXT("'%s' risolve a '%s', non al file appena scritto '%s'; il file appena scritto e' stato rimosso"),
			*Scenario.ScenarioId, *Risolto, *Percorso);
		return false;
	}

	OutScenarioId = Scenario.ScenarioId;
	return true;
}
