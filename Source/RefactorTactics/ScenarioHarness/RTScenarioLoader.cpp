#include "ScenarioHarness/RTScenarioLoader.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** Cella da array `[q, r, layer]`. Il layer e' opzionale (default 0), come nella maggior parte degli scenari piani. */
	bool ParseCell(const TArray<TSharedPtr<FJsonValue>>* Arr, FRTCellId& Out, FString& OutError, const TCHAR* Where)
	{
		if (!Arr || Arr->Num() < 2)
		{
			OutError = FString::Printf(TEXT("%s: la cella deve essere [q, r] oppure [q, r, layer]"), Where);
			return false;
		}
		Out = FRTCellId(
			static_cast<int32>((*Arr)[0]->AsNumber()),
			static_cast<int32>((*Arr)[1]->AsNumber()),
			Arr->Num() >= 3 ? static_cast<int32>((*Arr)[2]->AsNumber()) : 0);
		return true;
	}

	/** Gli HeroId che il catalogo conosce davvero. Nessun elenco scritto a mano: se il roster cambia, questa segue. */
	TSet<FName> KnownHeroIds()
	{
		TSet<FName> Ids;
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero)
			{
				Ids.Add(Hero->HeroId);
			}
		}
		return Ids;
	}
}

FString URTScenarioLoader::ScenariosRoot()
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("Scenarios"));
}

FString URTScenarioLoader::PathForScenarioId(const FString& ScenarioId)
{
	// `Movement.Basic` -> `Scenarios/Movement/Basic.json`. L'ID gerarchico E' il percorso: un solo posto
	// dove sta la verita' su dove vive uno scenario.
	FString Path = ScenariosRoot();
	TArray<FString> Parts;
	ScenarioId.ParseIntoArray(Parts, TEXT("."), /*InCullEmpty=*/ true);
	for (int32 I = 0; I < Parts.Num(); ++I)
	{
		Path = FPaths::Combine(Path, I == Parts.Num() - 1 ? Parts[I] + TEXT(".json") : Parts[I]);
	}
	return Path;
}

bool URTScenarioLoader::LoadFromFile(const FString& FilePath, FRTTestScenario& OutScenario, FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutError = FString::Printf(TEXT("scenario non leggibile: %s"), *FilePath);
		return false;
	}
	return LoadFromString(Text, OutScenario, OutError);
}

bool URTScenarioLoader::LoadFromString(const FString& JsonText, FRTTestScenario& OutScenario, FString& OutError)
{
	OutScenario = FRTTestScenario();
	OutError.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("JSON non interpretabile");
		return false;
	}

	if (!Root->TryGetStringField(TEXT("scenarioId"), OutScenario.ScenarioId) || OutScenario.ScenarioId.IsEmpty())
	{
		OutError = TEXT("campo obbligatorio mancante: scenarioId");
		return false;
	}

	Root->TryGetNumberField(TEXT("version"), OutScenario.Version);
	if (OutScenario.Version > SupportedVersion)
	{
		OutError = FString::Printf(TEXT("formato versione %d non supportato (massimo %d): aggiorna il loader"),
			OutScenario.Version, SupportedVersion);
		return false;
	}
	Root->TryGetNumberField(TEXT("seed"), OutScenario.Seed);
	Root->TryGetNumberField(TEXT("mapRadius"), OutScenario.MapRadius);

	// --- unita' ------------------------------------------------------------------------------------------
	const TArray<TSharedPtr<FJsonValue>>* UnitsJson = nullptr;
	if (!Root->TryGetArrayField(TEXT("units"), UnitsJson) || UnitsJson->Num() == 0)
	{
		OutError = TEXT("uno scenario deve schierare almeno una unita' (campo units)");
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *UnitsJson)
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid()) { OutError = TEXT("units: voce non valida"); return false; }

		FRTScenarioUnit Unit;
		Obj->TryGetStringField(TEXT("id"), Unit.Id);
		FString HeroText;
		Obj->TryGetStringField(TEXT("hero"), HeroText);
		Unit.HeroId = FName(*HeroText);
		Obj->TryGetNumberField(TEXT("team"), Unit.TeamId);

		const TArray<TSharedPtr<FJsonValue>>* CellArr = nullptr;
		Obj->TryGetArrayField(TEXT("cell"), CellArr);
		if (!ParseCell(CellArr, Unit.Cell, OutError, *FString::Printf(TEXT("unita' '%s'"), *Unit.Id)))
		{
			return false;
		}
		OutScenario.Units.Add(Unit);
	}

	// --- turni -------------------------------------------------------------------------------------------
	const TArray<TSharedPtr<FJsonValue>>* TurnsJson = nullptr;
	if (Root->TryGetArrayField(TEXT("turns"), TurnsJson))
	{
		for (const TSharedPtr<FJsonValue>& TurnValue : *TurnsJson)
		{
			const TSharedPtr<FJsonObject> TurnObj = TurnValue->AsObject();
			if (!TurnObj.IsValid()) { OutError = TEXT("turns: voce non valida"); return false; }

			FRTScenarioTurn Turn;
			const TArray<TSharedPtr<FJsonValue>>* IntentsJson = nullptr;
			if (TurnObj->TryGetArrayField(TEXT("intents"), IntentsJson))
			{
				for (const TSharedPtr<FJsonValue>& IntentValue : *IntentsJson)
				{
					const TSharedPtr<FJsonObject> IntentObj = IntentValue->AsObject();
					if (!IntentObj.IsValid()) { OutError = TEXT("intents: voce non valida"); return false; }

					FRTScenarioIntent Intent;
					IntentObj->TryGetStringField(TEXT("unit"), Intent.UnitId);

					const TArray<TSharedPtr<FJsonValue>>* MoveArr = nullptr;
					if (IntentObj->TryGetArrayField(TEXT("move"), MoveArr))
					{
						for (const TSharedPtr<FJsonValue>& Step : *MoveArr)
						{
							const TArray<TSharedPtr<FJsonValue>>* StepArr = nullptr;
							if (!Step->TryGetArray(StepArr))
							{
								OutError = FString::Printf(TEXT("intent di '%s': move deve essere una lista di celle"), *Intent.UnitId);
								return false;
							}
							FRTCellId Cell;
							if (!ParseCell(StepArr, Cell, OutError, *FString::Printf(TEXT("move di '%s'"), *Intent.UnitId)))
							{
								return false;
							}
							Intent.Move.Add(Cell);
						}
					}
					Turn.Intents.Add(Intent);
				}
			}
			OutScenario.Turns.Add(Turn);
		}
	}

	// --- assertion ---------------------------------------------------------------------------------------
	const TArray<TSharedPtr<FJsonValue>>* ExpectJson = nullptr;
	if (Root->TryGetArrayField(TEXT("expect"), ExpectJson))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ExpectJson)
		{
			const TSharedPtr<FJsonObject> Obj = Value->AsObject();
			if (!Obj.IsValid()) { OutError = TEXT("expect: voce non valida"); return false; }

			FString Type;
			Obj->TryGetStringField(TEXT("type"), Type);

			FRTTestExpectation Exp;
			if (Type == TEXT("UnitAtCell"))
			{
				Exp.Kind = ERTAssertionKind::UnitAtCell;
				Obj->TryGetStringField(TEXT("unit"), Exp.UnitId);
				const TArray<TSharedPtr<FJsonValue>>* CellArr = nullptr;
				Obj->TryGetArrayField(TEXT("cell"), CellArr);
				if (!ParseCell(CellArr, Exp.Cell, OutError, TEXT("expect UnitAtCell")))
				{
					return false;
				}
			}
			else if (Type == TEXT("TurnsCompleted"))
			{
				Exp.Kind = ERTAssertionKind::TurnsCompleted;
				Obj->TryGetNumberField(TEXT("value"), Exp.Value);
			}
			else
			{
				// Meglio rifiutare che ignorare: una assertion scritta male che venisse saltata in silenzio
				// farebbe passare un test che non verifica nulla.
				OutError = FString::Printf(TEXT("assertion sconosciuta: '%s' (previste: UnitAtCell, TurnsCompleted)"), *Type);
				return false;
			}
			OutScenario.Expect.Add(Exp);
		}
	}

	return Validate(OutScenario, OutError);
}

bool URTScenarioLoader::Validate(const FRTTestScenario& Scenario, FString& OutError)
{
	OutError.Reset();

	if (Scenario.ScenarioId.IsEmpty())
	{
		OutError = TEXT("scenarioId vuoto");
		return false;
	}
	if (Scenario.MapRadius <= 0)
	{
		OutError = FString::Printf(TEXT("mapRadius deve essere positivo (era %d)"), Scenario.MapRadius);
		return false;
	}
	if (Scenario.Units.Num() == 0)
	{
		OutError = TEXT("nessuna unita' schierata");
		return false;
	}

	const TSet<FName> Heroes = KnownHeroIds();
	TSet<FString> SeenIds;
	TSet<FRTCellId> SeenCells;
	for (const FRTScenarioUnit& Unit : Scenario.Units)
	{
		if (Unit.Id.IsEmpty())
		{
			OutError = TEXT("un'unita' non ha id");
			return false;
		}
		if (SeenIds.Contains(Unit.Id))
		{
			OutError = FString::Printf(TEXT("id unita' duplicato: '%s'"), *Unit.Id);
			return false;
		}
		SeenIds.Add(Unit.Id);

		if (!Heroes.Contains(Unit.HeroId))
		{
			// Il caso che il documento di specifica sbagliava per primo, citando eroi inesistenti.
			OutError = FString::Printf(TEXT("unita' '%s': eroe sconosciuto '%s' (attesi %d eroi dal catalogo)"),
				*Unit.Id, *Unit.HeroId.ToString(), Heroes.Num());
			return false;
		}
		if (URTHexLibrary::HexDistance(Unit.Cell, FRTCellId(0, 0, Unit.Cell.Layer)) > Scenario.MapRadius)
		{
			OutError = FString::Printf(TEXT("unita' '%s': cella %s fuori dall'arena di raggio %d"),
				*Unit.Id, *Unit.Cell.ToString(), Scenario.MapRadius);
			return false;
		}
		if (SeenCells.Contains(Unit.Cell))
		{
			OutError = FString::Printf(TEXT("due unita' partono dalla stessa cella %s"), *Unit.Cell.ToString());
			return false;
		}
		SeenCells.Add(Unit.Cell);
	}

	for (const FRTScenarioTurn& Turn : Scenario.Turns)
	{
		for (const FRTScenarioIntent& Intent : Turn.Intents)
		{
			if (!SeenIds.Contains(Intent.UnitId))
			{
				OutError = FString::Printf(TEXT("intent per un'unita' non schierata: '%s'"), *Intent.UnitId);
				return false;
			}
		}
	}

	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		if (Exp.Kind == ERTAssertionKind::UnitAtCell && !SeenIds.Contains(Exp.UnitId))
		{
			OutError = FString::Printf(TEXT("assertion UnitAtCell su un'unita' non schierata: '%s'"), *Exp.UnitId);
			return false;
		}
	}

	if (Scenario.Expect.Num() == 0)
	{
		// Uno scenario senza assertion passerebbe SEMPRE: sarebbe un test che non testa.
		OutError = TEXT("nessuna assertion dichiarata (campo expect): lo scenario passerebbe sempre");
		return false;
	}

	return true;
}
