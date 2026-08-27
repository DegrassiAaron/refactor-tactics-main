// Lo scenario in lavorazione. Vedi `RTScenarioDraft.h` per il perche' della separazione dalla facade.
//
// Ogni operazione qui dentro delega a chi possiede gia' la regola: `URTScenarioLoader` per interpretare,
// validare e scrivere, `URTScenarioIndex` per sapere dove vive uno scenario. Questo file non decide niente
// che qualcun altro decida gia' — se un giorno si trovasse a farlo, la decisione sarebbe nel posto sbagliato.

#include "ScenarioHarness/RTScenarioDraft.h"

#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"

void FRTScenarioDraft::NewScenario(const FString& ScenarioId, int32 MapRadius)
{
	Scenario = FRTTestScenario();
	Scenario.ScenarioId = ScenarioId;
	Scenario.MapRadius = MapRadius;
	SourcePath.Reset();
	bOpen = true;

	// ⚠️ Niente `Validate` qui, ed e' voluto: uno scenario appena creato NON e' valido — non ha unita' e non
	// ha assertion. Rifiutarlo alla nascita renderebbe impossibile crearne uno, che e' il punto di `#1115`.
	// La validita' e' una domanda che si fa al salvataggio, non alla prima riga.
}

ERTScenarioAuthoringResult FRTScenarioDraft::OpenById(const FString& ScenarioId, FString& OutError)
{
	OutError.Reset();

	// L'ID non si traduce in percorso con una regola di composizione: lo chiede all'indice, perche' le
	// cartelle sono libere e l'identita' e' dichiarata dal file (`RTScenarioIndex.h`).
	const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, OutError);
	if (Path.IsEmpty())
	{
		return ERTScenarioAuthoringResult::NotFound;
	}

	return OpenFromFile(Path, OutError);
}

ERTScenarioAuthoringResult FRTScenarioDraft::OpenFromFile(const FString& FilePath, FString& OutError)
{
	OutError.Reset();

	FRTTestScenario Loaded;
	if (!URTScenarioLoader::LoadFromFile(FilePath, Loaded, OutError))
	{
		// `LoadFromFile` non distingue «file assente» da «file illeggibile»: entrambi sono, per chi apre, lo
		// scenario che non c'e'. La frase che accompagna il codice dice quale dei due.
		return ERTScenarioAuthoringResult::NotFound;
	}

	Scenario = MoveTemp(Loaded);
	SourcePath = FilePath;
	bOpen = true;
	return ERTScenarioAuthoringResult::Success;
}

void FRTScenarioDraft::Close()
{
	Scenario = FRTTestScenario();
	SourcePath.Reset();
	bOpen = false;
}

ERTScenarioAuthoringResult FRTScenarioDraft::Validate(FString& OutError) const
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	return URTScenarioLoader::Validate(Scenario, OutError)
		? ERTScenarioAuthoringResult::Success
		: ERTScenarioAuthoringResult::Invalid;
}

ERTScenarioAuthoringResult FRTScenarioDraft::SaveToFile(const FString& FilePath, FString& OutError) const
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	// `SaveToFile` valida gia' per conto suo e non tocca il disco se lo scenario non passa. La validazione
	// esplicita che segue serve a **distinguere i due esiti**: `Invalid` accusa lo scenario, `WriteFailed`
	// accusa il disco, e una UI che li confondesse manderebbe a cercare il difetto nel posto sbagliato.
	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		OutError = ValidationError;
		return ERTScenarioAuthoringResult::Invalid;
	}

	if (!URTScenarioLoader::SaveToFile(Scenario, FilePath, OutError))
	{
		return ERTScenarioAuthoringResult::WriteFailed;
	}

	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::SaveInPlace(FString& OutError) const
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	// Il percorso da cui si e' aperto viene prima dell'indice: un file aperto per percorso esplicito — un
	// file di test, una cartella nuova — si risalva dov'era, e l'indice potrebbe non conoscerlo affatto.
	FString Target = SourcePath;
	if (Target.IsEmpty())
	{
		Target = URTScenarioIndex::ResolvePath(Scenario.ScenarioId, OutError);
		if (Target.IsEmpty())
		{
			return ERTScenarioAuthoringResult::NotFound;
		}
	}

	return SaveToFile(Target, OutError);
}

FRTScenarioSummary FRTScenarioDraft::GetSummary() const
{
	FRTScenarioSummary Summary;
	if (!bOpen)
	{
		return Summary;
	}

	Summary.ScenarioId = Scenario.ScenarioId;
	Summary.Version = Scenario.Version;
	Summary.Tags = Scenario.Tags;
	Summary.Fixture = Scenario.Fixture;
	Summary.MapRadius = Scenario.MapRadius;
	Summary.UnitCount = Scenario.Units.Num();
	Summary.TurnCount = Scenario.Turns.Num();
	Summary.ExpectationCount = Scenario.Expect.Num();
	Summary.VariantCount = Scenario.Variants.Num();
	return Summary;
}

TArray<FRTScenarioUnitView> FRTScenarioDraft::ListUnits() const
{
	TArray<FRTScenarioUnitView> Views;
	if (!bOpen)
	{
		return Views;
	}

	Views.Reserve(Scenario.Units.Num());
	for (const FRTScenarioUnit& Unit : Scenario.Units)
	{
		FRTScenarioUnitView& View = Views.AddDefaulted_GetRef();
		View.Id = Unit.Id;
		View.HeroId = Unit.HeroId;
		View.TeamId = Unit.TeamId;
		View.Cell = Unit.Cell;
		View.Facing = Unit.Facing;
		View.bBotControlled = Unit.bBotControlled;
	}

	// L'ordine e' quello del file, non un ordinamento: le unita' si nominano per Stable Unit ID e riordinarle
	// qui farebbe divergere questa vista dal `units` che l'autore legge nel JSON.
	return Views;
}
