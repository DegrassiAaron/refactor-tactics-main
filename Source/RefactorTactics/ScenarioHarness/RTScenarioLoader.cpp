#include "ScenarioHarness/RTScenarioLoader.h"
#include "Turn/RTTurnLogLibrary.h" // la mappa categoria -> enum degli esiti vive li' (#1427)
#include "ScenarioHarness/RTScenarioRunner.h" // MaxTurnsHardCap: il tetto assoluto che un file non puo' superare
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // un'azione di Prep risolve su se' e non dichiara un bersaglio
#include "Turn/RTTurnRules.h"
#include "Turn/RTReactionOpportunityTypes.h" // IsDeclaredConditionAllowed: il validator della condizione sta nel gioco
#include "Map/RTHexLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/**
	 * Una direzione esagonale dal suo nome (`E`, `NE`, `NW`, `W`, `SW`, `SE`), maiuscole indifferenti.
	 *
	 * Il nome sconosciuto e' un errore dichiarato, non un ripiego: un ripiego darebbe allo scenario un
	 * orientamento diverso da quello scritto nel file, e nessuno se ne accorgerebbe.
	 */
	bool ParseDirection(const FString& Text, ERTHexDirection& Out, FString& OutError, const TCHAR* Where)
	{
		static const TMap<FString, ERTHexDirection> ByName = {
			{ TEXT("E"),  ERTHexDirection::E },
			{ TEXT("NE"), ERTHexDirection::NE },
			{ TEXT("NW"), ERTHexDirection::NW },
			{ TEXT("W"),  ERTHexDirection::W },
			{ TEXT("SW"), ERTHexDirection::SW },
			{ TEXT("SE"), ERTHexDirection::SE },
		};
		const ERTHexDirection* Found = ByName.Find(Text.ToUpper());
		if (Found == nullptr)
		{
			OutError = FString::Printf(
				TEXT("%s: direzione '%s' sconosciuta (attese E, NE, NW, W, SW, SE)"), Where, *Text);
			return false;
		}
		Out = *Found;
		return true;
	}

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

	/** I nomi di un enum, ordinati, per un messaggio d'errore che dica cosa era previsto. */
	FString EnumNameList(const UEnum* Enum)
	{
		if (!Enum) { return FString(); }
		TArray<FString> Names;
		// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge, e non e' un valore scrivibile.
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
		{
			Names.Add(Enum->GetNameStringByIndex(I));
		}
		Names.Sort();
		return FString::Join(Names, TEXT(", "));
	}

	/**
	 * Categoria ed esito di un evento del TurnLog, scritti per NOME nello scenario.
	 *
	 * I nomi si risolvono per RIFLESSIONE (`StaticEnum<>`), non da una tabella scritta a mano: le tabelle
	 * scritte a mano divergono dall'enum che descrivono, ed e' gia' costato due test rossi in questo stesso
	 * file (l'elenco delle chiavi note contro il parser che leggeva `edge`). Cosi' aggiungere un esito al
	 * TurnLog lo rende scrivibile negli scenari senza toccare il loader.
	 */
	bool ParseScenarioLogEvent(const TSharedPtr<FJsonObject>& Obj, const TCHAR* CategoryField,
		const TCHAR* OutcomeField, ERTLogCategory& OutCategory, uint8& OutOutcome, FString& OutError)
	{
		FString CategoryText;
		if (!Obj->TryGetStringField(CategoryField, CategoryText))
		{
			OutError = FString::Printf(TEXT("assertion sul TurnLog: manca il campo %s"), CategoryField);
			return false;
		}

		const UEnum* CategoryEnum = StaticEnum<ERTLogCategory>();
		const int64 CategoryValue = CategoryEnum ? CategoryEnum->GetValueByNameString(CategoryText) : INDEX_NONE;
		if (CategoryValue == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("assertion sul TurnLog: categoria '%s' sconosciuta (previste: %s)"),
				*CategoryText, *EnumNameList(CategoryEnum));
			return false;
		}
		OutCategory = static_cast<ERTLogCategory>(CategoryValue);

		FString OutcomeText;
		if (!Obj->TryGetStringField(OutcomeField, OutcomeText))
		{
			OutError = FString::Printf(TEXT("assertion sul TurnLog: manca il campo %s"), OutcomeField);
			return false;
		}

		const UEnum* OutcomeEnum = URTScenarioLoader::OutcomeEnumForCategory(OutCategory);

		// ⚠️ **Una categoria SENZA enum non e' «esito sbagliato»: e' «categoria non asseribile», e finche'
		// i due casi non si distinguevano il messaggio diceva `(previsti: )` — una lista vuota che non
		// spiega niente.** E' costato un ciclo: uno scenario che chiedeva `PredictionWhiffed` falliva il
		// caricamento con un errore che sembrava un refuso nel nome dell'esito, mentre il difetto era che
		// `Predictive` non aveva un caso in `OutcomeEnumForCategory`. Chi aggiunge una categoria nuova al
		// TurnLog e dimentica la riga la' dentro riceve ora una frase che gliela indica.
		if (OutcomeEnum == nullptr)
		{
			OutError = FString::Printf(
				TEXT("assertion sul TurnLog: la categoria %s non e' asseribile — non ha un enum di esiti in ")
				TEXT("`URTScenarioLoader::OutcomeEnumForCategory`. Non e' un errore dello scenario: manca un ")
				TEXT("caso nel loader, e va aggiunto li'."),
				*CategoryText);
			return false;
		}

		const int64 OutcomeValue = OutcomeEnum->GetValueByNameString(OutcomeText);
		if (OutcomeValue == INDEX_NONE)
		{
			// Il messaggio nomina la CATEGORIA: `BridgeRemoved` e' un esito legittimo, ma non di `Facing`, e
			// senza quel dettaglio l'autore dello scenario cerca l'errore nel posto sbagliato.
			OutError = FString::Printf(
				TEXT("assertion sul TurnLog: esito '%s' sconosciuto per la categoria %s (previsti: %s)"),
				*OutcomeText, *CategoryText, *EnumNameList(OutcomeEnum));
			return false;
		}
		OutOutcome = static_cast<uint8>(OutcomeValue);
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

const UEnum* URTScenarioLoader::OutcomeEnumForCategory(ERTLogCategory Category)
{
	// ⚠️ **La mappa non vive piu' qui** (`#1427`, 2026-08-27): la corrispondenza categoria -> enum degli
	// esiti e' una proprieta' del TurnLog, e tenerla nello harness la rendeva irraggiungibile a chi il
	// TurnLog lo scrive — `ScenarioHarness` dipende da `Turn`, non viceversa. Il report della divergenza
	// golden rendeva `Outcome` come intero nudo per questo.
	//
	// Resta questo passacarte perche' il contratto d'errore del caricatore la nomina per nome, e chi legge
	// quel messaggio deve trovare la funzione dove il messaggio dice.
	return URTTurnLogLibrary::OutcomeEnumForCategory(Category);
}

FString URTScenarioLoader::DescribeLogEvent(ERTLogCategory Category, uint8 Outcome)
{
	const UEnum* CategoryEnum = StaticEnum<ERTLogCategory>();
	const FString CategoryName = CategoryEnum
		? CategoryEnum->GetNameStringByValue(static_cast<int64>(Category))
		: FString::FromInt(static_cast<int32>(Category));

	const UEnum* OutcomeEnum = OutcomeEnumForCategory(Category);
	const FString OutcomeName = OutcomeEnum
		? OutcomeEnum->GetNameStringByValue(static_cast<int64>(Outcome))
		: FString();

	// Un esito fuori dall'enum non e' impossibile: il campo e' un `uint8` e i log serializzati vecchi possono
	// portarne uno che questa build non conosce piu'. Mostrarlo GREZZO e' l'unica risposta onesta.
	return FString::Printf(TEXT("%s.%s"), *CategoryName,
		OutcomeName.IsEmpty() ? *FString::Printf(TEXT("esito %d"), static_cast<int32>(Outcome)) : *OutcomeName);
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

namespace
{
	/**
	 * Le regole di FORMA di una `decisions`: vocabolario di `respond`, presenza/assenza di `target`, e la
	 * versione che quella forma richiede.
	 *
	 * 🔴 **Esiste perche' erano DUE copie, e sono divergite alla prima aggiunta.** Il parser
	 * (`LoadFromString`) e il gate (`Validate`) portavano lo stesso controllo scritto due volte: aggiungendo
	 * le risposte di profilo (E14.7 fetta 4) ne e' stata aggiornata **una sola**, e il corpus e' diventato
	 * rosso con il messaggio della copia vecchia — «previste: FIRE, HOLD» — mentre il codice nuovo diceva
	 * altro a dieci righe di distanza. Una funzione sola e' l'unica forma in cui non possono piu' dire cose
	 * diverse. Il commento sotto `ValidateScenarioTurns` spiega perche' servono entrambi i chiamanti.
	 */
	bool ValidateDecisionForm(const FRTScenarioDecision& Decision, int32 ScenarioVersion, FString& OutError)
	{
		const bool bFire = Decision.Respond.Equals(TEXT("FIRE"), ESearchCase::CaseSensitive);
		const bool bHold = Decision.Respond.Equals(TEXT("HOLD"), ESearchCase::CaseSensitive);

		// Il vocabolario si CHIEDE al catalogo, non si riscrive qui: le risposte di un Reaction Profile
		// (`Hold Ground`, `SIDESTEP`, …) sono legali quanto `FIRE`/`HOLD`, e l'elenco che le conosce e' uno
		// solo — `URTCatalogLibrary`. Una seconda lista in questo file divergerebbe al primo profilo
		// aggiunto, e a divergere sarebbe il GATE, cioe' il pezzo il cui mestiere e' accorgersene.
		const bool bProfileResponse = !bFire && !bHold
			&& URTCatalogLibrary::IsKnownReactionProfileResponse(Decision.Respond);

		if (!bFire && !bHold && !bProfileResponse)
		{
			// L'elenco atteso si GENERA e si ORDINA, come per le chiavi di turno: scriverlo a mano
			// significherebbe che il messaggio d'errore e il controllo possono dire cose diverse — ed e' il
			// messaggio a essere letto quando qualcosa non torna.
			TArray<FString> Expected = { TEXT("FIRE"), TEXT("HOLD") };
			Expected.Append(URTCatalogLibrary::AllReactionProfileResponses());
			Expected.Sort();
			OutError = FString::Printf(TEXT("decisions: risposta '%s' sconosciuta (previste: %s)"),
				*Decision.Respond, *FString::Join(Expected, TEXT(", ")));
			return false;
		}

		// 🔴 **Anche questa forma deve DICHIARARE la versione che la ammette**, per la stessa ragione per cui
		// `decisions` richiede la `2`: su una build a `SupportedVersion = 2` un file `version: 2` con
		// `respond: "SIDESTEP"` passa il gate di versione e viene poi rifiutato con «risposta sconosciuta»,
		// che accusa il FILE mentre il difetto e' la build. Il gate qui fa dire al messaggio la cosa giusta.
		if (bProfileResponse && ScenarioVersion < 3)
		{
			OutError = FString::Printf(
				TEXT("decisions: la risposta di profilo '%s' richiede \"version\": 3 (dichiarata: %d)"),
				*Decision.Respond, ScenarioVersion);
			return false;
		}

		// `target` obbligatorio con FIRE e VIETATO con tutto il resto. Il secondo divieto e' la meta' che
		// conta: un bersaglio ignorato fa dichiarare allo scenario una cosa che non verifica. ⚠️ La condizione
		// e' `!bFire` e non `bHold`: scritta sul solo `HOLD` avrebbe lasciato passare `SIDESTEP` con un
		// `target`, cioe' avrebbe riaperto in silenzio proprio il caso che questo divieto chiude.
		if (bFire && Decision.Target.IsEmpty())
		{
			OutError = FString::Printf(TEXT("decisions: 'FIRE' di '%s' richiede 'target'"), *Decision.Unit);
			return false;
		}
		if (!bFire && !Decision.Target.IsEmpty())
		{
			OutError = FString::Printf(TEXT("decisions: '%s' non ammette 'target' (dichiarato '%s')"),
				*Decision.Respond, *Decision.Target);
			return false;
		}
		return true;
	}

	// --- Sezioni del formato scenario ------------------------------------------------------------
	//
	// Una funzione per sezione del JSON, nell'ordine in cui il documento le dichiara. Ciascuna torna
	// `false` avendo scritto in `OutError` il motivo: un errore di scrittura dello scenario dev'essere
	// un messaggio, mai un turno che gira a meta'. Le sezioni opzionali che mancano tornano `true`
	// senza toccare nulla — «assente» e «vuota» restano cose diverse dove il formato le distingue.

	/** `cells` (opzionale): celle il cui terreno lo scenario sovrascrive rispetto alla mappa di partenza. */
	bool ParseScenarioCells(const TSharedPtr<FJsonObject>& Root, FRTTestScenario& OutScenario, FString& OutError)
	{
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
				Obj->TryGetNumberField(TEXT("occupancySurcharge"), Cell.OccupancySurcharge);
				OutScenario.Cells.Add(Cell);
			}
		}
		return true;
	}

	/** `units` (obbligatoria): chi scende in campo. Uno scenario senza unita' non e' uno scenario. */
	bool ParseScenarioUnits(const TSharedPtr<FJsonObject>& Root, FRTTestScenario& OutScenario, FString& OutError)
	{
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

			// `facing` opzionale: assente = `E`, lo stesso default di `ARTUnit`. Un nome sconosciuto e' un ERRORE
			// e non un ripiego silenzioso su `E` — uno scenario che scrivesse `"North"` otterrebbe altrimenti un
			// orientamento diverso da quello che ha chiesto, e il suo verde direbbe la cosa sbagliata.
			FString FacingText;
			if (Obj->TryGetStringField(TEXT("facing"), FacingText) && !FacingText.IsEmpty())
			{
				if (!ParseDirection(FacingText, Unit.Facing, OutError,
					*FString::Printf(TEXT("unita' '%s'"), *Unit.Id)))
				{
					return false;
				}
			}

			// `bot` opzionale: assente = unita' guidata dal file, che resta il caso normale.
			Obj->TryGetBoolField(TEXT("bot"), Unit.bBotControlled);

			// `health`/`shield` opzionali: assenti = i valori del roster. Il campo si legge solo se PRESENTE, cosi'
			// il sentinella `-1` resta distinguibile da uno `0` chiesto davvero (vedi `FRTScenarioUnit::Health`).
			if (Obj->HasField(TEXT("health")))
			{
				Obj->TryGetNumberField(TEXT("health"), Unit.Health);
			}
			if (Obj->HasField(TEXT("shield")))
			{
				Obj->TryGetNumberField(TEXT("shield"), Unit.Shield);
			}
			if (Obj->HasField(TEXT("visionRange")))
			{
				Obj->TryGetNumberField(TEXT("visionRange"), Unit.VisionRange);
			}
			// `loadout` opzionale (`#602`): assente = il default dell'eroe, quindi gli scenari gia' scritti non
			// cambiano. Qui si LEGGE soltanto; che i pezzi esistano e che l'insieme sia legale lo verifica la
			// validazione piu' sotto, insieme al resto — un errore di scrittura dello scenario dev'essere un
			// motivo, non un turno che gira a meta'.
			const TArray<TSharedPtr<FJsonValue>>* LoadoutArr = nullptr;
			if (Obj->TryGetArrayField(TEXT("loadout"), LoadoutArr))
			{
				// La PRESENZA della chiave, registrata a parte dal contenuto: `"loadout": []` significa «entra
				// spoglia» e `loadout` assente significa «monta il default dell'eroe», ma entrambe danno zero
				// pezzi. Senza questo flag le due forme sarebbero indistinguibili, e quattro scenari che tengono
				// ferma la spinta a 1 tornerebbero a misurare la cosa sbagliata **restando verdi**.
				Unit.bLoadoutDeclared = true;

				for (const TSharedPtr<FJsonValue>& Piece : *LoadoutArr)
				{
					FString PieceId;
					if (!Piece->TryGetString(PieceId) || PieceId.IsEmpty())
					{
						OutError = FString::Printf(
							TEXT("unita' '%s': loadout deve essere una lista di EquipmentId"), *Unit.Id);
						return false;
					}
					Unit.Loadout.Add(FName(*PieceId));
				}
			}

			OutScenario.Units.Add(Unit);
		}
		return true;
	}

	/**
	 * `turns` (opzionale): gli intenti dichiarati turno per turno. E' la sezione piu' grande del formato,
	 * perche' un intento porta con se' azione, bersaglio, facing, percorso e parametri.
	 */
	bool ParseScenarioTurns(const TSharedPtr<FJsonObject>& Root, FRTTestScenario& OutScenario, FString& OutError)
	{
		// --- turni -------------------------------------------------------------------------------------------
		const TArray<TSharedPtr<FJsonValue>>* TurnsJson = nullptr;
		if (Root->TryGetArrayField(TEXT("turns"), TurnsJson))
		{
			for (const TSharedPtr<FJsonValue>& TurnValue : *TurnsJson)
			{
				const TSharedPtr<FJsonObject> TurnObj = TurnValue->AsObject();
				if (!TurnObj.IsValid()) { OutError = TEXT("turns: voce non valida"); return false; }

				FRTScenarioTurn Turn;

				// Le chiavi di turno ammesse. Misurato sul corpus il 2026-08-16 (77 file): sono quattro —
				// `intents` 113, `requires` 36, `_turno` 64, `_nota` 3 — e le due con `_` sono la convenzione
				// dei commenti. `decisions` si e' aggiunta qui con la fase A di `#512`, quando nel corpus non
				// compariva ancora; dalla **fase B** compare, in due file versionati
				// (`RT_Showcase_Relay_v01` T4 e `Spec/Overwatch/HoldThenFire` T2), che e' il motivo per cui
				// questa riga non dice piu' «non compare ancora»: chi rimisura il corpus contro un commento
				// scaduto trova una contraddizione e non sa quale delle due credere.
				// Senza questo controllo un refuso — `desicions` per `decisions` — viene
				// ignorato e il turno cade su `HoldNoDecider`, che e' indistinguibile da «nessuno ha
				// risposto»: verde per il motivo sbagliato.
				{
					static const TSet<FString> KnownTurnKeys = {
						TEXT("intents"), TEXT("requires"), TEXT("decisions")
					};
					TArray<FString> UnknownTurnKeys;
					for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : TurnObj->Values)
					{
						if (Field.Key.StartsWith(TEXT("_"))) { continue; }
						if (!KnownTurnKeys.Contains(Field.Key)) { UnknownTurnKeys.Add(Field.Key); }
					}
					if (UnknownTurnKeys.Num() > 0)
					{
						UnknownTurnKeys.Sort();
						TArray<FString> Expected = KnownTurnKeys.Array();
						Expected.Sort();
						OutError = FString::Printf(TEXT("turns: chiave sconosciuta '%s' (previste: %s)"),
							*UnknownTurnKeys[0], *FString::Join(Expected, TEXT(", ")));
						return false;
					}
				}

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

				// `decisions`: le risposte scriptate ai decision boundary di questo turno (CP 15.3 meta' B, #512).
				// Si legge PRIMA degli intent per una ragione di leggibilita' del messaggio d'errore: un turno con
				// un refuso nelle decisioni fallisce nominando le decisioni, non il primo intent che incontra.
				const TArray<TSharedPtr<FJsonValue>>* DecisionsJson = nullptr;
				const bool bHasDecisionsKey = TurnObj->HasField(TEXT("decisions"));
				const bool bDecisionsIsArray = TurnObj->TryGetArrayField(TEXT("decisions"), DecisionsJson);
				// 🔴 **La chiave c'e' ma non e' un array: e' un errore, non un'assenza.** `"decisions": {}` o
				// `"decisions": null` supererebbero il controllo sulle chiavi di turno — la chiave E' nota — e poi
				// `TryGetArrayField` fallirebbe in silenzio, saltando il blocco intero: il turno girerebbe con la
				// coda vuota, ogni finestra cadrebbe su un timeout e lo scenario sarebbe verde. E' lo stesso buco
				// che il controllo sulle chiavi chiude un livello piu' su.
				if (bHasDecisionsKey && !bDecisionsIsArray)
				{
					OutError = TEXT("turns: 'decisions' deve essere un array");
					return false;
				}
				// 🔴 **E il formato deve DICHIARARE la versione che le ammette.** Senza questo, la `version` 2
				// non gaterebbe nulla: un file `version: 1` con `decisions` verrebbe accettato qui e — su una
				// build vecchia, che non conosce ne' la chiave ne' la versione — ignorato in silenzio, giocando
				// il turno non scriptato. E' esattamente il fallimento che il bump esiste per impedire.
				if (bDecisionsIsArray && DecisionsJson->Num() > 0 && OutScenario.Version < 2)
				{
					OutError = FString::Printf(
						TEXT("turns: 'decisions' richiede \"version\": 2 (dichiarata: %d)"), OutScenario.Version);
					return false;
				}
				if (bDecisionsIsArray)
				{
					for (const TSharedPtr<FJsonValue>& DecisionValue : *DecisionsJson)
					{
						const TSharedPtr<FJsonObject> DecisionObj = DecisionValue->AsObject();
						if (!DecisionObj.IsValid())
						{
							OutError = TEXT("decisions: voce non valida");
							return false;
						}

						FRTScenarioDecision Decision;
						DecisionObj->TryGetStringField(TEXT("unit"), Decision.Unit);
						DecisionObj->TryGetStringField(TEXT("respond"), Decision.Respond);
						DecisionObj->TryGetStringField(TEXT("target"), Decision.Target);

						// L'elenco delle chiavi attese si GENERA dal set e si ORDINA: le due copie sono divergite
						// alla prima aggiunta (`edge`, poco piu' sotto), e un `TSet` non ha ordine — un messaggio che
						// cambia testo fra due esecuzioni identiche fa dubitare del file invece che di se' stesso.
						static const TSet<FString> KnownDecisionKeys = {
							TEXT("unit"), TEXT("respond"), TEXT("target")
						};
						TArray<FString> UnknownDecisionKeys;
						for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : DecisionObj->Values)
						{
							if (Field.Key.StartsWith(TEXT("_")))
							{
								continue; // `_nota` e simili: commenti, come altrove nel formato
							}
							if (!KnownDecisionKeys.Contains(Field.Key))
							{
								UnknownDecisionKeys.Add(Field.Key);
							}
						}
						if (UnknownDecisionKeys.Num() > 0)
						{
							UnknownDecisionKeys.Sort();
							TArray<FString> ExpectedKeys = KnownDecisionKeys.Array();
							ExpectedKeys.Sort();
							OutError = FString::Printf(
								TEXT("decisions: chiave sconosciuta '%s' (previste: %s)"),
								*UnknownDecisionKeys[0], *FString::Join(ExpectedKeys, TEXT(", ")));
							return false;
						}

						// La FORMA — vocabolario di `respond`, `target`, versione richiesta — sta in
						// `ValidateDecisionForm`, condivisa con `Validate`. Erano due copie e sono divergite:
						// vedi il commento di quella funzione.
						if (!ValidateDecisionForm(Decision, OutScenario.Version, OutError))
						{
							return false;
						}
						const bool bFire = Decision.Respond.Equals(TEXT("FIRE"), ESearchCase::CaseSensitive);
						// I nomi si risolvono QUI: un'unita' che non esiste e' uno scenario scritto male, non una
						// capability mancante — e `Blocked` direbbe la seconda cosa. `units` e' letto sopra
						// (riga ~247), quindi `FindUnit` ha gia' il roster.
						if (!OutScenario.FindUnit(Decision.Unit))
						{
							OutError = FString::Printf(
								TEXT("decisions: unita' '%s' non schierata"), *Decision.Unit);
							return false;
						}
						if (bFire && !OutScenario.FindUnit(Decision.Target))
						{
							OutError = FString::Printf(
								TEXT("decisions: bersaglio '%s' non schierato"), *Decision.Target);
							return false;
						}

						Turn.Decisions.Add(Decision);
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
								TEXT("edge"),
								// `facing` — rotazione DICHIARATA in pianificazione (D-020, #291). Aggiunta qui e nel
								// parser **nello stesso commit**, che e' la lezione lasciata da `edge`: le due copie
								// vivono a trecento righe di distanza e nessun gate le confronta.
								TEXT("facing"),
								// `condition` — la condizione dichiarata sulla reazione armata ([D-109], #583).
								// Terza voce aggiunta qui e nel parser nello STESSO commit, e la lezione lasciata da
								// `edge` ha smesso di essere teorica: senza questa riga lo scenario nuovo veniva
								// rifiutato con «chiave sconosciuta», cioe' la meta' mancante del loader si sarebbe
								// presentata come un errore di scrittura del file invece che come una lacuna del parser.
								TEXT("condition")
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
								// pretenderlo qui costringerebbe a scrivere «Riktor si mette in guardia bersagliando
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

						// Rotazione DICHIARATA (D-020, #291): come si chiede di FINIRE girati, non come si comincia.
						// Usa `ParseDirection`, l'helper che gia' serve il facing di PIAZZAMENTO poche righe piu' su:
						// il blocco di `edge` qui sopra si porta dietro una copia della mappa delle direzioni, e due
						// copie divergono alla prima direzione aggiunta.
						FString IntentFacingText;
						if (IntentObj->TryGetStringField(TEXT("facing"), IntentFacingText) && !IntentFacingText.IsEmpty())
						{
							const FString Where = FString::Printf(
								TEXT("intent di '%s': rotazione dichiarata"), *Intent.UnitId);
							if (!ParseDirection(IntentFacingText, Intent.Facing, OutError, *Where))
							{
								return false;
							}
							Intent.bDeclaresFacing = true;
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

						// La condizione dichiarata sulla reazione armata ([D-109]). Oggetto e non stringa: `id` e
						// `param` sono due cose diverse — quale condizione, e con quale soglia — e una stringa sola
						// costringerebbe a inventare una sintassi (`"TargetHealth<=10"`) che nessuno ha deciso e che
						// il validator del gioco non parla.
						// 🔴 **Si guarda la PRESENZA prima del tipo, e non e' pignoleria.** `TryGetObjectField`
						// restituisce `false` per QUALUNQUE valore che non sia un oggetto — una stringa, un numero,
						// `null` — quindi un `"condition": "TargetHealthAtOrBelowPercent"` cadeva fuori dall'`if`
						// senza un errore. E il gate delle chiavi sconosciute non lo vedeva, perche' `condition` e'
						// una chiave NOTA: lo scenario girava senza condizione, l'opportunity non collassava, e
						// l'autore leggeva solo `LogEventCount(HoldImmediate) atteso 2, ottenuto 0`. La forma con la
						// stringa e' anche la prima che verrebbe in mente, visto che ogni altro campo dell'intent
						// (`unit`, `ability`, `target`, `reaction`, `edge`, `facing`) e' una stringa.
						if (IntentObj->HasField(TEXT("condition")))
						{
							const TSharedPtr<FJsonObject>* ConditionObj = nullptr;
							if (!IntentObj->TryGetObjectField(TEXT("condition"), ConditionObj))
							{
								OutError = FString::Printf(
									TEXT("intent di '%s': condition deve essere un oggetto { \"id\": ..., \"param\": N }"),
									*Intent.UnitId);
								return false;
							}
							FString ConditionId;
							if (!(*ConditionObj)->TryGetStringField(TEXT("id"), ConditionId) || ConditionId.IsEmpty())
							{
								OutError = FString::Printf(
									TEXT("intent di '%s': condition senza 'id'"), *Intent.UnitId);
								return false;
							}

							// Il parametro e' INTERO e va chiesto esplicitamente: un default silenzioso qui
							// significherebbe «soglia 0», cioe' una condizione che non e' mai vera, e lo scenario
							// direbbe di aver ristretto il fuoco mentre lo ha spento.
							double ParamNumber = 0.0;
							if (!(*ConditionObj)->TryGetNumberField(TEXT("param"), ParamNumber))
							{
								OutError = FString::Printf(
									TEXT("intent di '%s': condition '%s' senza 'param'"), *Intent.UnitId, *ConditionId);
								return false;
							}
							// Il RANGE si controlla PRIMA del cast, e non e' pedanteria: `static_cast<int32>` di un
							// double che non ci sta e' undefined behavior. `"param": 1e20` supererebbe il controllo
							// di interezza qui sotto — e' gia' intero — e atterrerebbe su `IsDeclaredConditionAllowed`
							// come un valore INDEFINITO: se cadesse per caso dentro `0..100` lo scenario verrebbe
							// accettato con una soglia che non e' quella scritta nel file. Il limite dichiarato del
							// validator e' `0..100`, quindi qui basta la finestra di `int32` per rendere il cast sicuro
							// e lasciare a lui l'ultima parola sul dominio vero.
							if (ParamNumber < static_cast<double>(TNumericLimits<int32>::Min())
								|| ParamNumber > static_cast<double>(TNumericLimits<int32>::Max()))
							{
								OutError = FString::Printf(
									TEXT("intent di '%s': condition '%s' ha un 'param' fuori scala (%f)"),
									*Intent.UnitId, *ConditionId, ParamNumber);
								return false;
							}
							// Confronto ESATTO, non `IsNearlyEqual`: la tolleranza di default e' `UE_KINDA_SMALL_NUMBER`
							// (1e-4), quindi un `10.00005` passerebbe e verrebbe arrotondato a 10 — cioe' lo scenario
							// girerebbe una soglia che il file non dichiara, che e' precisamente cio' che il gate `G7`
							// vieta. Una regola sull'esattezza si verifica esattamente.
							if (ParamNumber != FMath::RoundToDouble(ParamNumber))
							{
								// Gate `G7`: niente float in soglie e priorita'. Il confronto del gioco e' in
								// aritmetica intera (`Health * 100 <= MaxHealth * Param`), quindi un `50.5` verrebbe
								// troncato in silenzio e lo scenario descriverebbe una soglia che non e' la sua.
								OutError = FString::Printf(
									TEXT("intent di '%s': condition '%s' vuole un 'param' INTERO, non %f"),
									*Intent.UnitId, *ConditionId, ParamNumber);
								return false;
							}

							Intent.Condition = FRTDeclaredCondition(
								FName(*ConditionId), static_cast<int32>(FMath::RoundToDouble(ParamNumber)));
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
		return true;
	}

	/** `expect` (opzionale): le assertion. E' qui che uno scenario dichiara che cosa sta verificando. */
	bool ParseScenarioExpectations(const TSharedPtr<FJsonObject>& Root, FRTTestScenario& OutScenario, FString& OutError)
	{
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
				// I due capi di un REDIRECT (#1060). `unit` obbligatorio in entrambe: senza, l'assertion
				// confronterebbe un id vuoto e passerebbe o fallirebbe per un motivo che non e' quello scritto —
				// lo stesso argomento con cui `UnitHpEquals` rifiuta un `value` mancante invece di indovinare 0.
				else if (Type == TEXT("OriginalTargetEquals") || Type == TEXT("EffectiveTargetEquals"))
				{
					Exp.Kind = (Type == TEXT("OriginalTargetEquals"))
						? ERTAssertionKind::OriginalTargetEquals
						: ERTAssertionKind::EffectiveTargetEquals;
					if (!Obj->TryGetStringField(TEXT("unit"), Exp.UnitId) || Exp.UnitId.IsEmpty())
					{
						OutError = FString::Printf(TEXT("assertion %s: manca il campo unit"), *Type);
						return false;
					}
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
				else if (Type == TEXT("LogEventCount"))
				{
					Exp.Kind = ERTAssertionKind::LogEventCount;
					if (!ParseScenarioLogEvent(Obj, TEXT("category"), TEXT("outcome"), Exp.LogCategory, Exp.LogOutcome, OutError))
					{
						return false;
					}
					// Conteggio ATTESO, e `0` e' un valore legittimo — anzi e' quello che serve per asserire
					// un'assenza. Assente del tutto = 1, cioe' «l'evento c'e'»: e' il caso piu' comune e scriverlo
					// ogni volta sarebbe rumore.
					int32 Count = 1;
					Obj->TryGetNumberField(TEXT("value"), Count);
					if (Count < 0)
					{
						OutError = TEXT("assertion LogEventCount: value non puo' essere negativo");
						return false;
					}
					Exp.Value = Count;
				}
				else if (Type == TEXT("LogEventOrder"))
				{
					Exp.Kind = ERTAssertionKind::LogEventOrder;
					if (!ParseScenarioLogEvent(Obj, TEXT("category"), TEXT("outcome"), Exp.LogCategory, Exp.LogOutcome, OutError))
					{
						return false;
					}
					if (!ParseScenarioLogEvent(Obj, TEXT("thenCategory"), TEXT("thenOutcome"), Exp.ThenCategory, Exp.ThenOutcome, OutError))
					{
						return false;
					}
				}
				else if (Type == TEXT("LogEventAmount"))
				{
					Exp.Kind = ERTAssertionKind::LogEventAmount;
					if (!ParseScenarioLogEvent(Obj, TEXT("category"), TEXT("outcome"), Exp.LogCategory, Exp.LogOutcome, OutError))
					{
						return false;
					}
					// `value` e' OBBLIGATORIO, al contrario di `LogEventCount`: li' l'omissione ha un default
					// sensato («l'evento c'e'»), qui non ce n'e' uno — un valore atteso implicito sarebbe un
					// numero inventato dal parser, e uno scenario passerebbe senza dire cosa si aspettava.
					int32 Amount = 0;
					if (!Obj->TryGetNumberField(TEXT("value"), Amount))
					{
						OutError = TEXT("assertion LogEventAmount: manca 'value', il valore atteso di Amount");
						return false;
					}
					Exp.Value = Amount;
				}
				else
				{
					// Meglio rifiutare che ignorare: una assertion scritta male che venisse saltata in silenzio
					// farebbe passare un test che non verifica nulla.
					// ⚠️ L'elenco va tenuto allineato all'enum: chi scrive `OriginalTargetEqual` (senza la `s`)
					// legge questa riga per capire cosa esiste, e un elenco stantio gli fa concludere che il
					// vocabolario non c'e'. La v9 l'aveva dimenticato — trovato da una code review.
					OutError = FString::Printf(TEXT("assertion sconosciuta: '%s' (previste: UnitAtCell, TurnsCompleted, UnitHpEquals, UnitAlive, UnitFacing, LogEventCount, LogEventOrder, LogEventAmount, OriginalTargetEquals, EffectiveTargetEquals)"), *Type);
					return false;
				}
				OutScenario.Expect.Add(Exp);
			}
		}
		return true;
	}

	/** `variants` (opzionale): schieramenti alternativi che devono dare lo stesso esito. */
	bool ParseScenarioVariants(const TSharedPtr<FJsonObject>& Root, FRTTestScenario& OutScenario, FString& OutError)
	{
		// --- varianti (opzionale) -------------------------------------------------------------------------------
		const TArray<TSharedPtr<FJsonValue>>* VariantsJson = nullptr;
		if (Root->TryGetArrayField(TEXT("variants"), VariantsJson))
		{
			for (const TSharedPtr<FJsonValue>& Value : *VariantsJson)
			{
				const TSharedPtr<FJsonObject> Obj = Value->AsObject();
				if (!Obj.IsValid()) { OutError = TEXT("variants: voce non valida"); return false; }

				FRTScenarioVariant Variant;
				Obj->TryGetStringField(TEXT("name"), Variant.Name);

				const TArray<TSharedPtr<FJsonValue>>* VariantUnits = nullptr;
				if (Obj->TryGetArrayField(TEXT("units"), VariantUnits))
				{
					for (const TSharedPtr<FJsonValue>& UnitValue : *VariantUnits)
					{
						const TSharedPtr<FJsonObject> UnitObj = UnitValue->AsObject();
						if (!UnitObj.IsValid()) { OutError = TEXT("variants: unita' non valida"); return false; }

						FRTScenarioVariantUnit VariantUnit;
						UnitObj->TryGetStringField(TEXT("id"), VariantUnit.Id);
						const TArray<TSharedPtr<FJsonValue>>* CellArr = nullptr;
						UnitObj->TryGetArrayField(TEXT("cell"), CellArr);
						if (!ParseCell(CellArr, VariantUnit.Cell, OutError,
							*FString::Printf(TEXT("variante '%s', unita' '%s'"), *Variant.Name, *VariantUnit.Id)))
						{
							return false;
						}
						Variant.Units.Add(VariantUnit);
					}
				}
				OutScenario.Variants.Add(Variant);
			}
		}
		return true;
	}
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

	// Le sezioni si leggono nell'ordine in cui il formato le dichiara, e ognuna si ferma al primo errore:
	// uno scenario mezzo caricato sarebbe peggio di uno rifiutato, perche' girerebbe.
	if (!ParseScenarioCells(Root, OutScenario, OutError)) { return false; }
	if (!ParseScenarioUnits(Root, OutScenario, OutError)) { return false; }
	if (!ParseScenarioTurns(Root, OutScenario, OutError)) { return false; }
	if (!ParseScenarioExpectations(Root, OutScenario, OutError)) { return false; }
	if (!ParseScenarioVariants(Root, OutScenario, OutError)) { return false; }

	Root->TryGetBoolField(TEXT("expectSameAcrossVariants"), OutScenario.bExpectSameAcrossVariants);

	// Free-run (CP 47.4). Nessun default silenzioso su `maxTurns`: la sua assenza resta 0 e `Validate` la
	// rifiuta con un motivo, invece di far girare la partita sotto un tetto che il file non dichiara.
	//
	// 🔴 **Ogni chiave si legge in DUE tempi — «c'e'?» e «e' del tipo giusto?» — e la seconda domanda e' quella
	// che conta.** Un `TryGet*` da solo restituisce `false` sia per una chiave assente sia per una chiave del
	// tipo sbagliato, e le due cose qui hanno esiti opposti: `"requires": "Objective"` (stringa invece di
	// array) lascerebbe `Requires` vuoto, e `AutoBattle.Objective` — che senza quel blocco gioca fino
	// all'eliminazione — uscirebbe **`Pass` in 10 turni** dichiarando di misurare l'obiettivo. E' il verde
	// falso che questa chiave esiste per impedire, prodotto da un refuso di battitura.
	//
	// Stessa disciplina che `turns`, `decisions` e `intents` applicano gia' ai propri oggetti: le chiavi
	// sconosciute sono un errore, e un tipo sbagliato non e' un'assenza.
	auto LeggiCampoTipizzato = [&Root, &OutError](const TCHAR* Key, auto&& Reader, const TCHAR* Expected) -> bool
	{
		if (!Root->HasField(Key))
		{
			return true;
		}
		if (!Reader())
		{
			OutError = FString::Printf(TEXT("%s: il valore dev'essere %s"), Key, Expected);
			return false;
		}
		return true;
	};

	if (!LeggiCampoTipizzato(TEXT("freeRun"),
		[&] { return Root->TryGetBoolField(TEXT("freeRun"), OutScenario.bFreeRun); },
		TEXT("un booleano"))) { return false; }
	if (!LeggiCampoTipizzato(TEXT("maxTurns"),
		[&] { return Root->TryGetNumberField(TEXT("maxTurns"), OutScenario.MaxTurns); },
		TEXT("un intero"))) { return false; }
	if (!LeggiCampoTipizzato(TEXT("repeatCount"),
		[&] { return Root->TryGetNumberField(TEXT("repeatCount"), OutScenario.RepeatCount); },
		TEXT("un intero"))) { return false; }

	const TArray<TSharedPtr<FJsonValue>>* RequiresJson = nullptr;
	if (!LeggiCampoTipizzato(TEXT("requires"),
		[&] { return Root->TryGetArrayField(TEXT("requires"), RequiresJson); },
		TEXT("un array di nomi di capability"))) { return false; }
	if (RequiresJson)
	{
		for (const TSharedPtr<FJsonValue>& Value : *RequiresJson)
		{
			FString Capability;
			if (!Value.IsValid() || !Value->TryGetString(Capability) || Capability.IsEmpty())
			{
				OutError = TEXT("requires: ogni voce dev'essere il nome non vuoto di una capability");
				return false;
			}
			OutScenario.Requires.Add(Capability);
		}
	}

	// 🔴 **E il formato deve DICHIARARE la versione che le ammette**, come `decisions` con la `2` e le risposte
	// di profilo con la `3`. Il bump di `SupportedVersion` guarda nel verso «build vecchia, file nuovo»; questo
	// controllo guarda nell'altro — un file che usa la semantica della v4 dichiarandosi v1 mentirebbe a ogni
	// lettore e a ogni strumento, e su una build vecchia verrebbe letto come uno scenario con `turns` vuoto,
	// cioe' verde senza aver giocato.
	const bool bUsaChiaviV4 = OutScenario.bFreeRun || OutScenario.MaxTurns != 0
		|| OutScenario.RepeatCount != 1 || OutScenario.Requires.Num() > 0;
	if (bUsaChiaviV4 && OutScenario.Version < 4)
	{
		OutError = FString::Printf(
			TEXT("'freeRun'/'maxTurns'/'repeatCount'/'requires' richiedono \"version\": 4 (dichiarata: %d)"),
			OutScenario.Version);
		return false;
	}

	return Validate(OutScenario, OutError);
}

namespace
{
	// --- Validazione, sezione per sezione --------------------------------------------------------
	//
	// Stessa disciplina del parser: una funzione per sezione, `false` col motivo in `OutError`. Cio' che
	// attraversa piu' sezioni viaggia come PARAMETRO e non come variabile di una funzione lunga —
	// `SeenIds` nasce validando le unita' e serve a turni, assertion e varianti per dire «questo id non
	// esiste», e vederlo nella firma e' l'unico modo di sapere che quel legame c'e'.

	/** `cells`: nessuna cella fuori mappa, nessun costo negativo. */
	bool ValidateScenarioCells(const FRTTestScenario& Scenario, bool bUsesFixture, FString& OutError)
	{
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
			if (Cell.OccupancySurcharge < 0)
			{
				OutError = FString::Printf(TEXT("cella %s: occupancySurcharge negativo (%d)"),
					*Cell.Cell.ToString(), Cell.OccupancySurcharge);
				return false;
			}
		}
		return true;
	}

	/**
	 * `units`: eroi esistenti, id unici, celle libere e legali.
	 * PRODUCE `SeenIds` e `BotIds`, che le sezioni successive consumano.
	 */
	bool ValidateScenarioUnits(const FRTTestScenario& Scenario, bool bUsesFixture,
		TSet<FString>& SeenIds, TSet<FString>& BotIds, FString& OutError)
	{
		const TSet<FName> Heroes = KnownHeroIds();
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

			// `loadout` (`#602`): i pezzi devono esistere nel catalogo e l'insieme dev'essere LEGALE secondo la
			// stessa regola del gioco (`ValidateLoadout`, 1+1+1 — CP 7.4). Uno scenario che monti due gadget va
			// rifiutato con un motivo, esattamente come lo sarebbe una configurazione illegale in partita:
			// altrimenti l'harness diventa piu' PERMISSIVO del gioco, che e' l'altra meta' dell'asimmetria per cui
			// `ReactionPlanning` resta fuori.
			if (Unit.Loadout.Num() > 0)
			{
				TArray<const URTEquipmentData*> Pieces;
				for (const FName& PieceId : Unit.Loadout)
				{
					const URTEquipmentData* Found = URTCatalogLibrary::FindEquipment(PieceId);
					if (Found == nullptr)
					{
						OutError = FString::Printf(TEXT("unita' '%s': equipaggiamento sconosciuto '%s'"),
							*Unit.Id, *PieceId.ToString());
						return false;
					}
					Pieces.Add(Found);
				}

				const TArray<FString> LoadoutErrors = URTCatalogLibrary::ValidateLoadout(Pieces);
				if (LoadoutErrors.Num() > 0)
				{
					OutError = FString::Printf(TEXT("unita' '%s': loadout illegale — %s"),
						*Unit.Id, *LoadoutErrors[0]);
					return false;
				}
			}

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

			// Una salute dichiarata a zero schiererebbe un cadavere: il gioco non lo farebbe mai, e ogni assertion
			// successiva misurerebbe una partita che non puo' esistere. Il tetto (`MaxHealth`) lo verifica la
			// sessione, che e' l'unico posto dove il valore del roster e' davvero disponibile.
			if (Unit.Health != -1 && Unit.Health <= 0)
			{
				OutError = FString::Printf(TEXT("unita' '%s': health dichiarata a %d (dev'essere > 0, oppure omessa)"),
					*Unit.Id, Unit.Health);
				return false;
			}
			if (Unit.Shield != -1 && Unit.Shield < 0)
			{
				OutError = FString::Printf(TEXT("unita' '%s': shield dichiarato a %d (dev'essere >= 0, oppure omesso)"),
					*Unit.Id, Unit.Shield);
				return false;
			}
			// `0` e' legittimo — un'unita' cieca e' una premessa scrivibile — e il catalogo eroi applica lo stesso
			// vincolo («range visivo negativo»), quindi qui si rifiuta la stessa cosa che rifiuterebbe il gioco.
			if (Unit.VisionRange != -1 && Unit.VisionRange < 0)
			{
				OutError = FString::Printf(TEXT("unita' '%s': visionRange dichiarato a %d (dev'essere >= 0, oppure omesso)"),
					*Unit.Id, Unit.VisionRange);
				return false;
			}

			if (Unit.bBotControlled)
			{
				BotIds.Add(Unit.Id);
			}
		}
		return true;
	}


	/** `turns`: gli intenti nominano unita' che esistono e non pilotano a mano quelle affidate al bot. */
	bool ValidateScenarioTurns(const FRTTestScenario& Scenario,
		const TSet<FString>& SeenIds, const TSet<FString>& BotIds, FString& OutError)
	{
		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			// 🔴 **Le `decisions` si validano QUI e non solo nel parser**, ed e' la meta' che mancava: `Validate`
			// e' il gate che ogni strada attraversa — `FRTScenarioSession::Start` lo chiama — mentre i controlli
			// scritti dentro `LoadFromString` li vede solo chi arriva da JSON. Ogni scenario costruito in
			// memoria (tutti i test di questa fase, e ogni chiamante di `RunScenarioIsolated`) saltava
			// «unita' schierata», «FIRE richiede target» e «HOLD non ammette target»: un `D.Unit` scritto male
			// non produceva un errore, restava non consumato e riemergeva piu' tardi come un residuo che parla
			// d'altro.
			for (const FRTScenarioDecision& Decision : Turn.Decisions)
			{
				// La FORMA sta in `ValidateDecisionForm`, condivisa col parser: vedi il commento di quella
				// funzione per la ragione — erano due copie e sono divergite.
				if (!ValidateDecisionForm(Decision, Scenario.Version, OutError))
				{
					return false;
				}
				const bool bFire = Decision.Respond.Equals(TEXT("FIRE"), ESearchCase::CaseSensitive);
				if (!SeenIds.Contains(Decision.Unit))
				{
					OutError = FString::Printf(TEXT("decisions: unita' '%s' non schierata"), *Decision.Unit);
					return false;
				}
				if (bFire && !SeenIds.Contains(Decision.Target))
				{
					OutError = FString::Printf(TEXT("decisions: bersaglio '%s' non schierato"), *Decision.Target);
					return false;
				}
			}

			for (const FRTScenarioIntent& Intent : Turn.Intents)
			{
				if (!SeenIds.Contains(Intent.UnitId))
				{
					OutError = FString::Printf(TEXT("intent per un'unita' non schierata: '%s'"), *Intent.UnitId);
					return false;
				}
				// Un'unita' bot decide da sola: un intent scritto nel file verrebbe SOVRASCRITTO da `PlanBots`, che
				// azzera il piano di ogni unita' che guida. Accettarlo silenziosamente darebbe uno scenario che
				// dichiara una mossa, ne gioca un'altra, e resta verde se le due combaciano per caso.
				if (BotIds.Contains(Intent.UnitId))
				{
					OutError = FString::Printf(
						TEXT("intent dichiarato per l'unita' bot '%s': il suo piano lo produce l'utility scoring, non il file"),
						*Intent.UnitId);
					return false;
				}
				// ⚠️ **Reazione e condizione si validano PRIMA del blocco dell'abilita', e l'ordine non e' stilistico.**
				// Quel blocco fa `continue` per le azioni di Prep che risolvono su chi le usa — `Action.Overwatch` e' una
				// di quelle — e un `continue` salta il RESTO del corpo del ciclo, non solo i controlli sul bersaglio che
				// lo motivano. Con le due validazioni piu' in basso, un intent che arma l'Overwatch NON veniva validato:
				// ne' la sua condizione (#583) ne' la sua `reaction`, che poteva essere un nome inesistente e passare in
				// silenzio. Trovato dal test `LoaderRejectsMalformedCondition`, che falliva sul solo caso con `ability`.
				// La reazione si valida QUI e non a runtime perche' il suo modo di fallire e' silenzioso: armare
				// una reazione inesistente non produce nessun effetto e nessun errore, e chi legge vedrebbe solo
				// un'assertion sui danni che non torna, senza un indizio su dove guardare.
				if (!Intent.Reaction.IsNone())
				{
					FName HeroId;
					TArray<FName> UnitLoadout;
					for (const FRTScenarioUnit& U : Scenario.Units)
					{
						if (U.Id == Intent.UnitId) { HeroId = U.HeroId; UnitLoadout = U.Loadout; break; }
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

					// E anche fra i pezzi che lo SCENARIO equipaggia (`#602`): il kit dell'eroe non e' piu' l'unica
					// sorgente di azioni, e senza questa riga i tre moduli fuori dai loadout consigliati resterebbero
					// irraggiungibili proprio quando la chiave `loadout` esiste per raggiungerli. Il confronto e'
					// sull'`EquipmentId`, che e' anche il nome con cui il modulo compare nel TurnLog.
					if (Armed == nullptr && UnitLoadout.Contains(Intent.Reaction))
					{
						if (const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Intent.Reaction))
						{
							Armed = URTCatalogLibrary::MakeEquipmentAction(Piece, nullptr);
						}
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

				// La condizione dichiarata ([D-109]) si valida QUI per la stessa ragione della reazione: il suo modo
				// di fallire e' SILENZIOSO. `ARTUnit::SetPlannedReactionCondition` restituisce `false` e non scrive
				// niente — nessun log, nessun errore — quindi uno scenario con una condizione rifiutata girerebbe
				// **senza** condizione, l'opportunity non collasserebbe, e chi legge vedrebbe solo un'assertion sugli
				// HP che non torna. E' precisamente il difetto che #583 esiste per intercettare.
				if (Intent.Condition.IsDeclared())
				{
					// Una condizione senza reazione a cui applicarsi resta orfana nel piano: e' il primo dei due
					// rifiuti di `SetPlannedReactionCondition`, riprodotto qui perche' produca un motivo invece di un
					// campo vuoto.
					//
					// ⚠️ **Ed e' il vincolo che morde davvero oggi**, non un caso di scuola: l'Overwatch costa
					// l'azione PRINCIPALE, non lo slot reazione, quindi un intent di solo `"ability":
					// "Action.Overwatch"` con una `condition` finisce qui. Chi scrive lo scenario deve armare ANCHE
					// una reazione — la mancata riconciliazione dei due slot e' dichiarata in `RTTurnManager.cpp`,
					// ramo `Action.Overwatch`, e non si scioglie da questo lato.
					if (Intent.Reaction.IsNone())
					{
						OutError = FString::Printf(
							TEXT("intent di '%s': condition '%s' senza una reaction a cui applicarsi ")
							TEXT("(SetPlannedReactionCondition la rifiuterebbe in silenzio)"),
							*Intent.UnitId, *Intent.Condition.Id.ToString());
						return false;
					}

					// L'elenco delle condizioni ammesse e il range del parametro vivono nel GIOCO, non qui: questa
					// riga chiama il validator vero (`IsDeclaredConditionAllowed`) invece di ricopiarne le regole,
					// cosi' una condizione nuova non deve essere aggiunta in due posti — e non puo' essere ammessa
					// dall'harness e rifiutata dal gioco.
					if (!URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(Intent.Condition))
					{
						OutError = FString::Printf(
							TEXT("intent di '%s': condition '%s' param %d non e' ammessa dalla v0.1 ")
							TEXT("(l'unica e' 'TargetHealthAtOrBelowPercent', param 0..100)"),
							*Intent.UnitId, *Intent.Condition.Id.ToString(), Intent.Condition.Param);
						return false;
					}
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

			}
		}
		return true;
	}

	/** `expect`: ogni assertion punta a un'unita' schierata. */
	bool ValidateScenarioExpectations(const FRTTestScenario& Scenario,
		const TSet<FString>& SeenIds, FString& OutError)
	{
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

	/** `variants`: nomi unici, e ogni variante riposiziona unita' che esistono. */
	bool ValidateScenarioVariants(const FRTTestScenario& Scenario, bool bUsesFixture,
		const TSet<FString>& SeenIds, FString& OutError)
	{
		// --- varianti ------------------------------------------------------------------------------------------
		// Un confronto fra UNA cosa e' vuoto, e sarebbe verde per sempre: e' il modo piu' facile in cui un canary
		// smette di verificare senza che nessuno se ne accorga.
		if (Scenario.bExpectSameAcrossVariants && Scenario.Variants.Num() < 2)
		{
			OutError = FString::Printf(
				TEXT("expectSameAcrossVariants con %d varianti: senza almeno due il confronto non esiste"),
				Scenario.Variants.Num());
			return false;
		}
		if (Scenario.Variants.Num() == 1)
		{
			OutError = TEXT("una sola variante: o sono due o piu', oppure e' l'allestimento normale (campo units)");
			return false;
		}

		TSet<FString> SeenVariantNames;
		for (const FRTScenarioVariant& Variant : Scenario.Variants)
		{
			if (Variant.Name.IsEmpty())
			{
				OutError = TEXT("una variante non ha nome: il report non saprebbe dire QUALE e' rossa");
				return false;
			}
			if (SeenVariantNames.Contains(Variant.Name))
			{
				OutError = FString::Printf(TEXT("due varianti si chiamano '%s'"), *Variant.Name);
				return false;
			}
			SeenVariantNames.Add(Variant.Name);

			if (Variant.Units.Num() == 0)
			{
				// Una variante che non cambia niente e' una ripetizione: il suo TurnLog coincide per costruzione, e
				// farebbe passare `expectSameAcrossVariants` senza aver messo alla prova nessun ingresso.
				OutError = FString::Printf(TEXT("la variante '%s' non sposta nessuna unita'"), *Variant.Name);
				return false;
			}

			// Le celle della variante sostituiscono quelle dichiarate: la sovrapposizione si verifica sull'insieme
			// EFFETTIVO, non su quello di partenza — altrimenti due unita' potrebbero finire sulla stessa cella
			// proprio nella variante, che e' l'unico posto in cui nessuno guarderebbe.
			TMap<FString, FRTCellId> CellsInVariant;
			for (const FRTScenarioUnit& Unit : Scenario.Units)
			{
				CellsInVariant.Add(Unit.Id, Unit.Cell);
			}
			for (const FRTScenarioVariantUnit& Moved : Variant.Units)
			{
				if (!SeenIds.Contains(Moved.Id))
				{
					OutError = FString::Printf(TEXT("la variante '%s' sposta un'unita' non schierata: '%s'"),
						*Variant.Name, *Moved.Id);
					return false;
				}
				if (!bUsesFixture
					&& URTHexLibrary::HexDistance(Moved.Cell, FRTCellId(0, 0, Moved.Cell.Layer)) > Scenario.MapRadius)
				{
					OutError = FString::Printf(TEXT("la variante '%s': cella %s di '%s' fuori dall'arena di raggio %d"),
						*Variant.Name, *Moved.Cell.ToString(), *Moved.Id, Scenario.MapRadius);
					return false;
				}
				CellsInVariant.Add(Moved.Id, Moved.Cell);
			}

			TSet<FRTCellId> Occupied;
			for (const TPair<FString, FRTCellId>& Pair : CellsInVariant)
			{
				if (Occupied.Contains(Pair.Value))
				{
					OutError = FString::Printf(TEXT("la variante '%s' mette due unita' sulla stessa cella %s"),
						*Variant.Name, *Pair.Value.ToString());
					return false;
				}
				Occupied.Add(Pair.Value);
			}
		}
		return true;
	}

	/**
	 * Le regole del **free-run** (CP 47.4), e le tre che valgono al contrario: un campo del free-run scritto
	 * in uno scenario a turni e' un campo che non conta, e un campo che c'e' e non conta e' il modo in cui uno
	 * scenario dice una cosa e ne verifica un'altra.
	 */
	bool ValidateScenarioFreeRun(const FRTTestScenario& Scenario, const TSet<FString>& BotIds, FString& OutError)
	{
		if (Scenario.RepeatCount < 1)
		{
			OutError = FString::Printf(TEXT("repeatCount dev'essere almeno 1 (era %d)"), Scenario.RepeatCount);
			return false;
		}
		// E un tetto anche di sopra, per la stessa ragione per cui `maxTurns` ne ha uno: centomila ripetizioni
		// passerebbero la validazione e bloccherebbero il corpus senza una diagnostica.
		if (Scenario.RepeatCount > URTScenarioRunner::MaxRepeatCount)
		{
			OutError = FString::Printf(
				TEXT("repeatCount %d oltre il tetto del runner (%d): un corpus di determinismo si misura con ")
				TEXT("poche ripetizioni, non con una suite che non finisce"),
				Scenario.RepeatCount, URTScenarioRunner::MaxRepeatCount);
			return false;
		}
		if (Scenario.RepeatCount > 1 && Scenario.Variants.Num() > 0)
		{
			OutError = TEXT("repeatCount e variants insieme: le varianti cambiano un ingresso, repeatCount non ")
				TEXT("cambia niente — mescolarli darebbe tracce che differiscono per costruzione e un ")
				TEXT("confronto che non risponde a nessuna delle due domande");
			return false;
		}

		if (!Scenario.bFreeRun)
		{
			// I due campi che il free-run porta con se' non hanno significato senza di lui: rifiutarli qui e'
			// cio' che impedisce a un file di dichiarare una guardia che nessuno applichera'.
			if (Scenario.MaxTurns != 0)
			{
				OutError = FString::Printf(
					TEXT("maxTurns (%d) senza freeRun: il numero di turni lo dice gia' `turns`, e una guardia ")
					TEXT("che nessuno applica e' peggio di una guardia assente"), Scenario.MaxTurns);
				return false;
			}
			if (Scenario.Requires.Num() > 0)
			{
				OutError = TEXT("requires di scenario senza freeRun: li' il posto del requisito e' il turno ")
					TEXT("(`turns[].requires`), e due posti per la stessa dichiarazione divergono al primo edit");
				return false;
			}
			return true;
		}

		if (Scenario.Turns.Num() > 0)
		{
			OutError = FString::Printf(
				TEXT("freeRun con %d turni enumerati: o la partita decide quando finire, o lo dice il file — ")
				TEXT("insieme dicono due cose e ne eseguono una"), Scenario.Turns.Num());
			return false;
		}
		if (Scenario.MaxTurns <= 0)
		{
			OutError = TEXT("freeRun senza maxTurns: il tetto e' una guardia di sicurezza e si dichiara nel ")
				TEXT("file — un tetto invisibile e' un tetto che nessuno rivede");
			return false;
		}
		// Il tetto del FILE non puo' scavalcare quello del RUNNER. `MaxTurnsHardCap` esisteva dal primo giorno
		// dell'harness — «uno scenario non deve poter girare all'infinito» — ed era una costante **dichiarata e
		// mai letta**: nessun chiamante in tutto `Source/`. Il free-run e' il primo caso in cui un file puo'
		// davvero chiedere una partita senza fine, quindi e' anche il primo che le da' un consumatore.
		if (Scenario.MaxTurns > URTScenarioRunner::MaxTurnsHardCap)
		{
			OutError = FString::Printf(
				TEXT("maxTurns %d oltre il tetto del runner (%d): un tetto che il runner non sa applicare non ")
				TEXT("e' una guardia"), Scenario.MaxTurns, URTScenarioRunner::MaxTurnsHardCap);
			return false;
		}

		// `previewUnit` e' inerte in free-run e va rifiutata invece di ignorata: `ApplyPreviewSelection` scrive
		// il piano d'attacco del **turno 1** per farne comparire l'anteprima, e un free-run non ha un turno 1 da
		// cui leggerlo. Accettarla darebbe un campo che c'e' e non fa niente, e l'unico segnale sarebbe
		// l'ASSENZA dell'anteprima — indistinguibile da un'anteprima che non funziona.
		if (!Scenario.PreviewUnit.IsEmpty())
		{
			OutError = TEXT("previewUnit con freeRun: l'anteprima si costruisce dal piano del primo turno, e un ")
				TEXT("free-run non ne dichiara nessuno");
			return false;
		}

		// Ogni unita' al bot. Un'unita' non-bot in free-run non ha nessuno che le scriva l'intent — non c'e'
		// un turno da cui leggerlo — e resterebbe ferma per tutta la partita: uno scenario che dichiara un 2v2
		// e ne gioca meta' produrrebbe un esito vero su una premessa falsa.
		for (const FRTScenarioUnit& Unit : Scenario.Units)
		{
			if (!BotIds.Contains(Unit.Id))
			{
				OutError = FString::Printf(
					TEXT("freeRun con l'unita' '%s' non guidata dal bot: in un free-run gli intent non li ")
					TEXT("scrive nessuno, e quell'unita' resterebbe ferma senza dirlo"), *Unit.Id);
				return false;
			}
		}
		return true;
	}
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
	// Gli identificatori raccolti validando le unita' servono a tutte le sezioni successive: nascono qui,
	// dove si vede che il legame esiste, invece che dentro la sezione che li produce.
	TSet<FString> SeenIds;
	TSet<FString> BotIds;

	if (!ValidateScenarioCells(Scenario, bUsesFixture, OutError)) { return false; }
	if (!ValidateScenarioUnits(Scenario, bUsesFixture, SeenIds, BotIds, OutError)) { return false; }
	if (!ValidateScenarioTurns(Scenario, SeenIds, BotIds, OutError)) { return false; }
	if (!ValidateScenarioExpectations(Scenario, SeenIds, OutError)) { return false; }
	if (!ValidateScenarioVariants(Scenario, bUsesFixture, SeenIds, OutError)) { return false; }
	// Dopo le unita' perche' legge `BotIds`, e dopo le varianti perche' una delle sue regole le nomina.
	if (!ValidateScenarioFreeRun(Scenario, BotIds, OutError)) { return false; }

	return true;
}
