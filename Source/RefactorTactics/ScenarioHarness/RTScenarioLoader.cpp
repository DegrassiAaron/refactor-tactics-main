#include "ScenarioHarness/RTScenarioLoader.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // un'azione di Prep risolve su se' e non dichiara un bersaglio
#include "Turn/RTTurnRules.h"
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
	// Solo presentazione: quale unita' selezionare in PIE per far comparire l'anteprima. Headless non fa nulla.
	Root->TryGetStringField(TEXT("previewUnit"), OutScenario.PreviewUnit);
	Root->TryGetStringField(TEXT("fixture"), OutScenario.Fixture);
	Root->TryGetNumberField(TEXT("mapRadius"), OutScenario.MapRadius);

	// --- celle modificate (opzionale) --------------------------------------------------------------------
	const TArray<TSharedPtr<FJsonValue>>* CellsJson = nullptr;
	if (Root->TryGetArrayField(TEXT("cells"), CellsJson))
	{
		for (const TSharedPtr<FJsonValue>& Value : *CellsJson)
		{
			const TSharedPtr<FJsonObject> Obj = Value->AsObject();
			if (!Obj.IsValid()) { OutError = TEXT("cells: voce non valida"); return false; }

			FRTScenarioCell Cell;
			const TArray<TSharedPtr<FJsonValue>>* CellArr = nullptr;
			Obj->TryGetArrayField(TEXT("cell"), CellArr);
			if (!ParseCell(CellArr, Cell.Cell, OutError, TEXT("cells")))
			{
				return false;
			}
			Obj->TryGetBoolField(TEXT("blocksMovement"), Cell.bBlocksMovement);
			Obj->TryGetBoolField(TEXT("blocksLineOfSight"), Cell.bBlocksLineOfSight);
			Obj->TryGetNumberField(TEXT("moveCost"), Cell.MoveCost);
			OutScenario.Cells.Add(Cell);
		}
	}

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

			// `requires`: cosa deve esistere nel gioco perche' questo turno sia giocabile. Il runner si ferma
			// qui con `Blocked` invece di fallire, e lo scenario puo' essere versionato prima dei suoi sistemi.
			const TArray<TSharedPtr<FJsonValue>>* RequiresJson = nullptr;
			if (TurnObj->TryGetArrayField(TEXT("requires"), RequiresJson))
			{
				for (const TSharedPtr<FJsonValue>& Req : *RequiresJson)
				{
					FString Capability;
					if (!Req->TryGetString(Capability) || Capability.IsEmpty())
					{
						OutError = TEXT("requires: ogni voce deve essere il nome di una capability");
						return false;
					}
					Turn.Requires.Add(Capability);
				}
			}

			const TArray<TSharedPtr<FJsonValue>>* IntentsJson = nullptr;
			if (TurnObj->TryGetArrayField(TEXT("intents"), IntentsJson))
			{
				for (const TSharedPtr<FJsonValue>& IntentValue : *IntentsJson)
				{
					const TSharedPtr<FJsonObject> IntentObj = IntentValue->AsObject();
					if (!IntentObj.IsValid()) { OutError = TEXT("intents: voce non valida"); return false; }

					FRTScenarioIntent Intent;
					IntentObj->TryGetStringField(TEXT("unit"), Intent.UnitId);

					// Chiavi SCONOSCIUTE: rifiutate, non ignorate (CP 16.1). Prima di questo controllo il loader
					// leggeva le chiavi note e passava oltre alle altre in silenzio: uno scenario che chiedeva
					// qualcosa che l'harness non sa fare — un orientamento dichiarato, per dire — girava verde
					// verificando tutto tranne cio' che gli premeva. Un test che passa senza verificare e' peggio
					// di un test assente, perche' occupa il posto di quello vero.
					{
						static const TSet<FString> KnownIntentKeys = {
							TEXT("unit"), TEXT("ability"), TEXT("target"), TEXT("targetCell"),
							TEXT("dash"), TEXT("dashTo"), TEXT("reaction"), TEXT("move"),
							// `edge` — bordo bersagliato dalle azioni su STRUTTURA (CP 9.5). Mancava, e la sua
							// assenza e' costata due test rossi su `main`: il parser che lo LEGGE e questo elenco
							// che lo RIFIUTAVA sono nati su rami diversi, e il merge testuale non poteva vederlo.
							TEXT("edge")
						};
						// Si RACCOGLIE e si ORDINA prima di riportare: `IntentObj->Values` e' una TMap, e con due
						// chiavi sbagliate nello stesso intent il messaggio nominerebbe l'una o l'altra a seconda
						// dell'ordine di hash. Un errore che cambia testo fra due esecuzioni identiche fa dubitare
						// del file invece che di se' stesso.
						TArray<FString> Unknown;
						for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : IntentObj->Values)
						{
							if (Field.Key.StartsWith(TEXT("_")))
							{
								continue; // `_nota` e simili: commenti, come altrove nel formato
							}
							if (!KnownIntentKeys.Contains(Field.Key))
							{
								Unknown.Add(Field.Key);
							}
						}
						if (Unknown.Num() > 0)
						{
							Unknown.Sort();
							// L'elenco delle chiavi attese si GENERA dal set, non si riscrive a mano: le due copie
							// sono divergite alla prima aggiunta (`edge`), e un messaggio che elenca chiavi diverse
							// da quelle accettate manda a cercare il difetto nel file invece che qui. Ordinato,
							// perche' un `TSet` non ha ordine e il testo non deve cambiare fra due esecuzioni.
							TArray<FString> Expected = KnownIntentKeys.Array();
							Expected.Sort();
							OutError = FString::Printf(
								TEXT("intent di '%s': chiave sconosciuta '%s' (previste: %s)"),
								*Intent.UnitId, *Unknown[0], *FString::Join(Expected, TEXT(", ")));
							return false;
						}
					}

					FString AbilityText;
					if (IntentObj->TryGetStringField(TEXT("ability"), AbilityText) && !AbilityText.IsEmpty())
					{
						Intent.Ability = FName(*AbilityText);

						// Bersaglio a CELLA: alternativa a `target`, per le aree che si centrano su una cella
						// anche vuota. La coesistenza dei due la rifiuta `Validate`, non questo punto: qui si
						// legge il file, li' si giudica se ha senso.
						const TArray<TSharedPtr<FJsonValue>>* TargetCellArr = nullptr;
						if (IntentObj->TryGetArrayField(TEXT("targetCell"), TargetCellArr) && TargetCellArr->Num() >= 2)
						{
							Intent.TargetCell = FRTCellId(
								static_cast<int32>((*TargetCellArr)[0]->AsNumber()),
								static_cast<int32>((*TargetCellArr)[1]->AsNumber()),
								TargetCellArr->Num() >= 3 ? static_cast<int32>((*TargetCellArr)[2]->AsNumber()) : 0);
							Intent.bTargetsCell = true;
							IntentObj->TryGetStringField(TEXT("target"), Intent.Target); // solo per diagnosticare l'ambiguita'
						}
						else if (!IntentObj->TryGetStringField(TEXT("target"), Intent.Target) || Intent.Target.IsEmpty())
						{
							// Un'azione che risolve su CHI LA USA non ha un bersaglio da dichiarare: il
							// `TurnManager` si bersaglia da solo in fase Prep (`Instance.TargetUnitId = i`), e
							// pretenderlo qui costringerebbe a scrivere «Bastion si mette in guardia bersagliando
							// se stesso». La domanda si pone al CATALOGO invece di elencare gli ActionId self,
							// cosi' un'azione di Prep aggiunta domani non deve ricordarsi di questa riga.
							//
							// Se l'ID non e' nel catalogo core la Def torna vuota e la fase e' quella di default:
							// non e' Prep, quindi il bersaglio resta obbligatorio. Le azioni d'eroe passano di
							// qui, ed e' il comportamento che avevano prima.
							const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(Intent.Ability);
							const bool bResolvesOnSelf = !Def.ActionId.IsNone()
								&& URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Prep;
							if (!bResolvesOnSelf)
							{
								// Per tutte le altre, un'abilita' senza bersaglio non e' un'omissione innocua: lo
								// scenario girerebbe senza attaccare nessuno e l'assertion sui danni fallirebbe
								// per il motivo sbagliato.
								OutError = FString::Printf(
									TEXT("intent di '%s': l'abilita' '%s' non dichiara un bersaglio (campo target)"),
									*Intent.UnitId, *AbilityText);
								return false;
							}
						}
					}

					// Bordo bersagliato (CP 9.5): il nome della direzione, non il suo indice. Un numero nel JSON
					// costringerebbe chi scrive lo scenario a ricordare che 0 e' E, e un errore di conteggio
					// produrrebbe un pannello sul lato sbagliato con lo scenario comunque verde.
					FString EdgeText;
					if (IntentObj->TryGetStringField(TEXT("edge"), EdgeText) && !EdgeText.IsEmpty())
					{
						static const TMap<FString, ERTHexDirection> ByName = {
							{ TEXT("E"),  ERTHexDirection::E },
							{ TEXT("NE"), ERTHexDirection::NE },
							{ TEXT("NW"), ERTHexDirection::NW },
							{ TEXT("W"),  ERTHexDirection::W },
							{ TEXT("SW"), ERTHexDirection::SW },
							{ TEXT("SE"), ERTHexDirection::SE },
						};
						const ERTHexDirection* Found = ByName.Find(EdgeText.ToUpper());
						if (Found == nullptr)
						{
							OutError = FString::Printf(
								TEXT("intent di '%s': bordo '%s' sconosciuto (attesi E, NE, NW, W, SW, SE)"),
								*Intent.UnitId, *EdgeText);
							return false;
						}
						Intent.CoverEdge = *Found;
						Intent.bHasCoverEdge = true;
					}

					FString DashText;
					if (IntentObj->TryGetStringField(TEXT("dash"), DashText) && !DashText.IsEmpty())
					{
						Intent.Dash = FName(*DashText);

						// La destinazione e' obbligatoria per lo stesso motivo per cui lo e' il bersaglio di
						// un'abilita': senza, lo scatto non partirebbe e l'assertion cadrebbe su un fatto
						// diverso da quello che lo scenario voleva verificare.
						const TArray<TSharedPtr<FJsonValue>>* DashCellArr = nullptr;
						if (!IntentObj->TryGetArrayField(TEXT("dashTo"), DashCellArr) || DashCellArr->Num() < 2)
						{
							OutError = FString::Printf(
								TEXT("intent di '%s': la mobilita' '%s' non dichiara una destinazione (campo dashTo)"),
								*Intent.UnitId, *DashText);
							return false;
						}
						Intent.DashCell = FRTCellId(
							static_cast<int32>((*DashCellArr)[0]->AsNumber()),
							static_cast<int32>((*DashCellArr)[1]->AsNumber()),
							DashCellArr->Num() >= 3 ? static_cast<int32>((*DashCellArr)[2]->AsNumber()) : 0);
					}

					FString ReactionText;
					if (IntentObj->TryGetStringField(TEXT("reaction"), ReactionText) && !ReactionText.IsEmpty())
					{
						// Nessun bersaglio da pretendere qui: una reazione non lo dichiara, lo riceve dal trigger.
						Intent.Reaction = FName(*ReactionText);
					}

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
			else if (Type == TEXT("UnitHpEquals"))
			{
				Exp.Kind = ERTAssertionKind::UnitHpEquals;
				Obj->TryGetStringField(TEXT("unit"), Exp.UnitId);
				if (!Obj->TryGetNumberField(TEXT("value"), Exp.Value))
				{
					// Senza `value` verificherebbe «HP == 0», cioe' una cosa diversa da quella che chi l'ha
					// scritta intendeva. Meglio rifiutare che indovinare.
					OutError = FString::Printf(TEXT("assertion UnitHpEquals su '%s': manca il campo value"), *Exp.UnitId);
					return false;
				}
			}
			else if (Type == TEXT("UnitAlive"))
			{
				Exp.Kind = ERTAssertionKind::UnitAlive;
				Obj->TryGetStringField(TEXT("unit"), Exp.UnitId);
				bool bAlive = true;
				Obj->TryGetBoolField(TEXT("value"), bAlive);
				Exp.Value = bAlive ? 1 : 0;
			}
			else if (Type == TEXT("UnitFacing"))
			{
				Exp.Kind = ERTAssertionKind::UnitFacing;
				Obj->TryGetStringField(TEXT("unit"), Exp.UnitId);

				// La direzione si scrive per NOME, non come indice: `3` non dice niente a chi legge lo scenario,
				// e rinumerare l'enum renderebbe verdi gli scenari sbagliati.
				FString DirectionText;
				if (!Obj->TryGetStringField(TEXT("value"), DirectionText))
				{
					OutError = FString::Printf(TEXT("assertion UnitFacing su '%s': manca il campo value"), *Exp.UnitId);
					return false;
				}

				static const TMap<FString, ERTHexDirection> ByName = {
					{ TEXT("E"),  ERTHexDirection::E  }, { TEXT("NE"), ERTHexDirection::NE },
					{ TEXT("NW"), ERTHexDirection::NW }, { TEXT("W"),  ERTHexDirection::W  },
					{ TEXT("SW"), ERTHexDirection::SW }, { TEXT("SE"), ERTHexDirection::SE }
				};
				const ERTHexDirection* Found = ByName.Find(DirectionText.ToUpper());
				if (!Found)
				{
					OutError = FString::Printf(
						TEXT("assertion UnitFacing su '%s': direzione '%s' sconosciuta (previste: E, NE, NW, W, SW, SE)"),
						*Exp.UnitId, *DirectionText);
					return false;
				}
				Exp.Value = static_cast<int32>(*Found);
			}
			else
			{
				// Meglio rifiutare che ignorare: una assertion scritta male che venisse saltata in silenzio
				// farebbe passare un test che non verifica nulla.
				OutError = FString::Printf(TEXT("assertion sconosciuta: '%s' (previste: UnitAtCell, TurnsCompleted, UnitHpEquals, UnitAlive, UnitFacing)"), *Type);
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
	// `mapRadius` conta solo quando l'arena si GENERA. Con una fixture riferita per nome la forma la decide
	// la fixture, e un raggio non ha nulla da dire: pretenderlo qui rifiuterebbe scenari perfettamente validi.
	const bool bUsesFixture = !Scenario.Fixture.IsEmpty();
	if (!bUsesFixture && Scenario.MapRadius <= 0)
	{
		OutError = FString::Printf(TEXT("mapRadius deve essere positivo (era %d)"), Scenario.MapRadius);
		return false;
	}
	if (Scenario.Units.Num() == 0)
	{
		OutError = TEXT("nessuna unita' schierata");
		return false;
	}

	// Le celle modificate devono stare nell'arena: un ostacolo fuori mappa non blocca niente e lo scenario
	// verificherebbe una condizione che non ha mai creato.
	for (const FRTScenarioCell& Cell : Scenario.Cells)
	{
		if (!bUsesFixture && URTHexLibrary::HexDistance(Cell.Cell, FRTCellId(0, 0, Cell.Cell.Layer)) > Scenario.MapRadius)
		{
			OutError = FString::Printf(TEXT("cella modificata %s fuori dall'arena di raggio %d"),
				*Cell.Cell.ToString(), Scenario.MapRadius);
			return false;
		}
		if (Cell.MoveCost < 0)
		{
			OutError = FString::Printf(TEXT("cella %s: moveCost negativo (%d)"), *Cell.Cell.ToString(), Cell.MoveCost);
			return false;
		}
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
		// Con una fixture la forma non e' un raggio: che la cella esista lo verifica il RUNNER sulla mappa
		// vera (vedi `Run`), che e' l'unico posto dove la geometria reale e' disponibile.
		if (!bUsesFixture && URTHexLibrary::HexDistance(Unit.Cell, FRTCellId(0, 0, Unit.Cell.Layer)) > Scenario.MapRadius)
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

		// Un'unita' che parte dentro un ostacolo e' uno scenario impossibile: il gioco non la piazzerebbe mai
		// li', e ogni assertion successiva misurerebbe una situazione che non puo' esistere.
		const FRTScenarioCell* OnBlocked = Scenario.Cells.FindByPredicate(
			[&Unit](const FRTScenarioCell& C) { return C.Cell == Unit.Cell && C.bBlocksMovement; });
		if (OnBlocked)
		{
			OutError = FString::Printf(TEXT("unita' '%s' parte su una cella che blocca il movimento %s"),
				*Unit.Id, *Unit.Cell.ToString());
			return false;
		}
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
			if (!Intent.Ability.IsNone())
			{
				// Bersaglio doppio: `ERROR`, non una scelta fra i due. Sceglierne uno al posto di chi ha
				// scritto lo scenario produrrebbe un test verde su una premessa sbagliata — e quello nessuno
				// va a riaprirlo.
				if (Intent.bTargetsCell && !Intent.Target.IsEmpty())
				{
					OutError = FString::Printf(
						TEXT("intent di '%s': dichiara sia il bersaglio '%s' sia una cella (target e targetCell insieme)"),
						*Intent.UnitId, *Intent.Target);
					return false;
				}
				if (Intent.bTargetsCell)
				{
					// Una cella bersaglio segue le stesse regole di ogni altra cella dello scenario: fuori
					// dall'arena e' un errore di scrittura, non un tiro che manca.
					continue;
				}
				// Azione che risolve su chi la usa: il bersaglio vuoto e' la forma CORRETTA, non un'omissione.
				// La lettura piu' sopra l'ha gia' ammessa; qui si evita che il controllo «e' schierato?» la
				// respinga cercando un'unita' di nome "".
				if (Intent.Target.IsEmpty())
				{
					const FRTActionDef SelfDef = URTCatalogLibrary::FindCoreAction(Intent.Ability);
					if (!SelfDef.ActionId.IsNone()
						&& URTCatalogLibrary::MapResolutionPhase(SelfDef.ResolutionPhase) == ERTMatchPhase::Prep)
					{
						continue;
					}
				}
				if (!SeenIds.Contains(Intent.Target))
				{
					OutError = FString::Printf(TEXT("intent di '%s': bersaglio '%s' non schierato"),
						*Intent.UnitId, *Intent.Target);
					return false;
				}
				if (Intent.Target == Intent.UnitId)
				{
					// Nessuna abilita' del roster v0.1 bersaglia se stessa: se un giorno esistesse, questo
					// controllo va rilassato di PROPOSITO, non lasciato cadere per distrazione.
					OutError = FString::Printf(TEXT("intent di '%s': l'unita' bersaglia se stessa"), *Intent.UnitId);
					return false;
				}
			}

			// La reazione si valida QUI e non a runtime perche' il suo modo di fallire e' silenzioso: armare
			// una reazione inesistente non produce nessun effetto e nessun errore, e chi legge vedrebbe solo
			// un'assertion sui danni che non torna, senza un indizio su dove guardare.
			if (!Intent.Reaction.IsNone())
			{
				FName HeroId;
				for (const FRTScenarioUnit& U : Scenario.Units)
				{
					if (U.Id == Intent.UnitId) { HeroId = U.HeroId; break; }
				}

				const URTActionData* Armed = nullptr;
				for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
				{
					if (Hero == nullptr || Hero->HeroId != HeroId) { continue; }
					for (const URTActionData* Action : Hero->Actions)
					{
						if (Action && Action->Def.ActionId == Intent.Reaction) { Armed = Action; break; }
					}
					break;
				}

				if (Armed == nullptr)
				{
					OutError = FString::Printf(TEXT("intent di '%s' (%s): non possiede la reazione '%s'"),
						*Intent.UnitId, *HeroId.ToString(), *Intent.Reaction.ToString());
					return false;
				}
				if (Armed->Def.Slot != ERTActionSlot::Reaction)
				{
					// Un'azione normale armata come reazione non scatta mai: resterebbe li' a non fare niente,
					// e lo scenario descriverebbe una regola che non ha mai messo alla prova.
					OutError = FString::Printf(
						TEXT("intent di '%s': '%s' non e' una reazione (non occupa lo slot Reaction)"),
						*Intent.UnitId, *Intent.Reaction.ToString());
					return false;
				}
			}
		}
	}

	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		const bool bNeedsUnit = (Exp.Kind == ERTAssertionKind::UnitAtCell)
			|| (Exp.Kind == ERTAssertionKind::UnitHpEquals)
			|| (Exp.Kind == ERTAssertionKind::UnitAlive);
		if (bNeedsUnit && !SeenIds.Contains(Exp.UnitId))
		{
			OutError = FString::Printf(TEXT("assertion su un'unita' non schierata: '%s'"), *Exp.UnitId);
			return false;
		}
		if (Exp.Kind == ERTAssertionKind::UnitHpEquals && Exp.Value < 0)
		{
			OutError = FString::Printf(TEXT("assertion UnitHpEquals su '%s': gli HP attesi non possono essere negativi (%d)"),
				*Exp.UnitId, Exp.Value);
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
