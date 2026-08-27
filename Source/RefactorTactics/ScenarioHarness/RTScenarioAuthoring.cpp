// La porta Blueprint dello Scenario Harness. Vedi `RTScenarioAuthoring.h` e ADR-0010.
//
// Ogni metodo qui dentro e' una traduzione: prende cio' che Blueprint sa dire, lo gira al draft, e riporta
// l'esito. Se un giorno una di queste funzioni contenesse un `if` che decide un esito di gioco, sarebbe da
// spostare nel runtime — questo file non e' un posto dove una regola puo' vivere.

#include "ScenarioHarness/RTScenarioAuthoring.h"

#include "Ability/RTHeroCatalogLibrary.h" // il roster per la tendina: gli id, non gli eroi costruiti
#include "ScenarioHarness/RTScenarioIndex.h"

#define LOCTEXT_NAMESPACE "RTScenarioAuthoring"

URTScenarioAuthoring* URTScenarioAuthoring::CreateScenarioDraft(UObject* Outer)
{
	// Senza un Outer esplicito l'oggetto finisce nel transient package: e' il default sensato per uno
	// strumento d'authoring, che non deve essere salvato con nulla.
	UObject* Owner = Outer != nullptr ? Outer : GetTransientPackage();
	return NewObject<URTScenarioAuthoring>(Owner);
}

void URTScenarioAuthoring::NewScenario(const FString& ScenarioId, int32 MapRadius)
{
	Draft.NewScenario(ScenarioId, MapRadius);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::OpenById(const FString& ScenarioId, FString& OutError)
{
	return Draft.OpenById(ScenarioId, OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::OpenFromFile(const FString& FilePath, FString& OutError)
{
	return Draft.OpenFromFile(FilePath, OutError);
}

void URTScenarioAuthoring::Close()
{
	Draft.Close();
}

ERTScenarioAuthoringResult URTScenarioAuthoring::Validate(FString& OutError) const
{
	return Draft.Validate(OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::SaveToFile(const FString& FilePath, FString& OutError)
{
	return Draft.SaveToFile(FilePath, OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::SaveInPlace(FString& OutError)
{
	return Draft.SaveInPlace(OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::AddUnit(const FString& UnitId, FName HeroId, int32 TeamId,
	FRTCellId Cell, ERTHexDirection Facing, FString& OutError)
{
	if (!Draft.IsOpen())
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	// `AddUnit` del draft risponde `bool` perche' li' l'unico modo di fallire e' il piazzamento; qui serve un
	// esito che la UI possa ramificare, e «rifiutato dalle regole» e' `Invalid`.
	return Draft.AddUnit(UnitId, HeroId, TeamId, Cell, Facing, OutError)
		? ERTScenarioAuthoringResult::Success
		: ERTScenarioAuthoringResult::Invalid;
}

ERTScenarioAuthoringResult URTScenarioAuthoring::MoveUnit(const FString& UnitId, FRTCellId Cell,
	FString& OutError)
{
	return Draft.MoveUnit(UnitId, Cell, OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::RemoveUnit(const FString& UnitId, FString& OutError)
{
	return Draft.RemoveUnit(UnitId, OutError);
}

ERTScenarioAuthoringResult URTScenarioAuthoring::SetUnitFacing(const FString& UnitId, ERTHexDirection Facing,
	FString& OutError)
{
	return Draft.SetUnitFacing(UnitId, Facing, OutError);
}

TArray<FName> URTScenarioAuthoring::ListHeroIds()
{
	return URTHeroCatalogLibrary::GetHeroIds();
}

TArray<FString> URTScenarioAuthoring::ListScenarioIds(const FString& FilterTagA, const FString& FilterTagB)
{
	return URTScenarioIndex::ListIds(FilterTagA, FilterTagB);
}

TArray<FString> URTScenarioAuthoring::ListScenarioTags()
{
	return URTScenarioIndex::ListTags();
}

FText URTScenarioAuthoring::DescribeResult(ERTScenarioAuthoringResult Result)
{
	// Frasi brevi e non accusatorie verso l'utente: dicono cos'e' successo, e il messaggio d'errore che le
	// accompagna dice quale campo. Due registri diversi, e servono entrambi.
	switch (Result)
	{
	case ERTScenarioAuthoringResult::Success:
		return LOCTEXT("Success", "Fatto");
	case ERTScenarioAuthoringResult::NotFound:
		return LOCTEXT("NotFound", "Scenario non trovato");
	case ERTScenarioAuthoringResult::Invalid:
		return LOCTEXT("Invalid", "Scenario non valido: non e' stato salvato");
	case ERTScenarioAuthoringResult::WriteFailed:
		return LOCTEXT("WriteFailed", "Scrittura fallita");
	case ERTScenarioAuthoringResult::NoScenarioOpen:
		return LOCTEXT("NoScenarioOpen", "Nessuno scenario aperto");
	}

	return LOCTEXT("Unknown", "Esito sconosciuto");
}

#undef LOCTEXT_NAMESPACE
