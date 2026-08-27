// Lo scenario in lavorazione. Vedi `RTScenarioDraft.h` per il perche' della separazione dalla facade.
//
// Ogni operazione qui dentro delega a chi possiede gia' la regola: `URTScenarioLoader` per interpretare,
// validare e scrivere, `URTScenarioIndex` per sapere dove vive uno scenario. Questo file non decide niente
// che qualcun altro decida gia' — se un giorno si trovasse a farlo, la decisione sarebbe nel posto sbagliato.

#include "ScenarioHarness/RTScenarioDraft.h"

#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h" // ValidateUnitPlacement: la regola sta li', non qui

#include "Ability/RTHeroCatalogLibrary.h"     // MovePoints: il budget viene dall'eroe, non da una stima
#include "Ability/RTHeroData.h"
#include "ScenarioHarness/RTScenarioArena.h"  // la stessa arena su cui girera' la partita
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"             // ReachableCells: il pathfinding sta li', e non qui

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

int32 FRTScenarioDraft::IndexOfUnit(const FString& UnitId) const
{
	return Scenario.Units.IndexOfByPredicate(
		[&UnitId](const FRTScenarioUnit& U) { return U.Id == UnitId; });
}

ERTScenarioAuthoringResult FRTScenarioDraft::AddUnit(const FString& UnitId, FName HeroId, int32 TeamId,
	const FRTCellId& Cell, ERTHexDirection Facing, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	FRTScenarioUnit Candidate;
	Candidate.Id = UnitId;
	Candidate.HeroId = HeroId;
	Candidate.TeamId = TeamId;
	Candidate.Cell = Cell;
	Candidate.Facing = Facing;

	// `INDEX_NONE`: l'unita' non e' ancora nell'array, quindi non c'e' niente da escludere dai confronti.
	// La regola e' quella del gioco, non una copia — vedi la nota in testa alla sezione nell'header.
	if (!URTScenarioLoader::ValidateUnitPlacement(Scenario, Candidate, INDEX_NONE, OutError))
	{
		return ERTScenarioAuthoringResult::Invalid;
	}

	Scenario.Units.Add(MoveTemp(Candidate));
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::MoveUnit(const FString& UnitId, const FRTCellId& Cell,
	FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	const int32 Index = IndexOfUnit(UnitId);
	if (Index == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}

	// Si valida una COPIA con la cella nuova, e si applica solo se passa: mutare prima e disfare dopo
	// lascerebbe lo scenario modificato in ogni percorso d'errore che dimenticasse il ripristino.
	FRTScenarioUnit Moved = Scenario.Units[Index];
	Moved.Cell = Cell;

	// `Index` da ignorare: senza, l'unita' collidere' con la propria posizione attuale e nessuno potrebbe
	// mai muoversi — e il messaggio direbbe «due unita' partono dalla stessa cella» parlando di una sola.
	if (!URTScenarioLoader::ValidateUnitPlacement(Scenario, Moved, Index, OutError))
	{
		return ERTScenarioAuthoringResult::Invalid;
	}

	Scenario.Units[Index] = MoveTemp(Moved);
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::RemoveUnit(const FString& UnitId, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	const int32 Index = IndexOfUnit(UnitId);
	if (Index == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}

	// ⚠️ Togliere una unita' puo' lasciare intent, decisioni e assertion che la NOMINANO: restano li', e
	// `Validate` li rifiutera' al salvataggio dicendo quale. Ripulirli qui cancellerebbe in silenzio il lavoro
	// di chi ha scritto quel turno, per un click che voleva togliere una pedina.
	//
	// 🔴 **Ma tacerlo e' un vicolo cieco, e va detto adesso invece che al salvataggio.** L'authoring dei turni
	// e' `#1116` e non esiste ancora: chi ritira una unita' nominata da un intent non ha, oggi, nessun modo di
	// riparare lo scenario da questa API — l'unica uscita sarebbe chiudere e perdere il lavoro. Contarli qui
	// non risolve il vicolo, ma lo rende visibile nel momento in cui si crea.
	int32 DanglingIntents = 0;
	int32 DanglingDecisions = 0;
	for (const FRTScenarioTurn& Turn : Scenario.Turns)
	{
		for (const FRTScenarioIntent& Intent : Turn.Intents)
		{
			if (Intent.UnitId == UnitId || Intent.Target == UnitId) { ++DanglingIntents; }
		}
		for (const FRTScenarioDecision& Decision : Turn.Decisions)
		{
			if (Decision.Unit == UnitId || Decision.Target == UnitId) { ++DanglingDecisions; }
		}
	}
	int32 DanglingExpectations = 0;
	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		if (Exp.UnitId == UnitId) { ++DanglingExpectations; }
	}

	Scenario.Units.RemoveAt(Index);

	if (DanglingIntents + DanglingDecisions + DanglingExpectations > 0)
	{
		// L'esito resta `Success` — la rimozione e' avvenuta — ma il messaggio dice cosa e' rimasto indietro.
		OutError = FString::Printf(
			TEXT("'%s' ritirata, ma restano %d intent, %d decisioni e %d assertion che la nominano: lo ")
			TEXT("scenario non si salvera' finche' non spariscono"),
			*UnitId, DanglingIntents, DanglingDecisions, DanglingExpectations);
	}
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::SetUnitFacing(const FString& UnitId, ERTHexDirection Facing,
	FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	const int32 Index = IndexOfUnit(UnitId);
	if (Index == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}

	// Nessuna validazione di piazzamento: ruotare non cambia la cella, e `ERTHexDirection` non ha valori
	// illegali da rifiutare — e' l'enum del gioco, non una stringa che qualcuno potrebbe scrivere storta.
	Scenario.Units[Index].Facing = Facing;
	return ERTScenarioAuthoringResult::Success;
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

// --- authoring dei turni (#1116) -------------------------------------------------------------------------

namespace
{
	/** L'intent di quell'unita' in quel turno, o `INDEX_NONE`. */
	int32 IndexOfIntent(const FRTScenarioTurn& Turn, const FString& UnitId)
	{
		return Turn.Intents.IndexOfByPredicate(
			[&UnitId](const FRTScenarioIntent& I) { return I.UnitId == UnitId; });
	}
}

int32 FRTScenarioDraft::AddTurn()
{
	if (!bOpen)
	{
		return INDEX_NONE;
	}
	return Scenario.Turns.Add(FRTScenarioTurn());
}

ERTScenarioAuthoringResult FRTScenarioDraft::SetMoveIntent(int32 TurnIndex, const FString& UnitId,
	const TArray<FRTCellId>& Path, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (!Scenario.Turns.IsValidIndex(TurnIndex))
	{
		OutError = FString::Printf(TEXT("turno %d inesistente (ce ne sono %d)"), TurnIndex, Scenario.Turns.Num());
		return ERTScenarioAuthoringResult::NotFound;
	}
	if (IndexOfUnit(UnitId) == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}
	if (Path.Num() == 0)
	{
		// Un `Move` senza destinazione non e' un'attesa: e' un piano che non dice dove. Chi vuole l'attesa
		// chiede `SetWaitIntent`, che la scrive nella forma che il formato ha.
		OutError = FString::Printf(
			TEXT("il Move di '%s' non dichiara nessuna cella: per non muoversi si usa Wait"), *UnitId);
		return ERTScenarioAuthoringResult::Invalid;
	}

	FRTScenarioTurn& Turn = Scenario.Turns[TurnIndex];
	const int32 Existing = IndexOfIntent(Turn, UnitId);

	FRTScenarioIntent Intent;
	Intent.UnitId = UnitId;
	Intent.Move = Path;

	// Sostituisce invece di accumulare: un editor in cui la stessa unita' porta due piani nello stesso turno
	// e' un editor che mente, e il resolver ne applicherebbe uno solo senza dire quale.
	if (Existing != INDEX_NONE)
	{
		Turn.Intents[Existing] = MoveTemp(Intent);
	}
	else
	{
		Turn.Intents.Add(MoveTemp(Intent));
	}
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::SetWaitIntent(int32 TurnIndex, const FString& UnitId,
	FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (!Scenario.Turns.IsValidIndex(TurnIndex))
	{
		OutError = FString::Printf(TEXT("turno %d inesistente (ce ne sono %d)"), TurnIndex, Scenario.Turns.Num());
		return ERTScenarioAuthoringResult::NotFound;
	}
	if (IndexOfUnit(UnitId) == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}

	FRTScenarioTurn& Turn = Scenario.Turns[TurnIndex];
	const int32 Existing = IndexOfIntent(Turn, UnitId);

	// Un intent che nomina l'unita' e nient'altro: e' cosi' che il formato dice «questa unita' non fa nulla».
	// Vedi la nota nell'header sul perche' NON e' `ability: "Action.Wait"`.
	FRTScenarioIntent Intent;
	Intent.UnitId = UnitId;

	if (Existing != INDEX_NONE)
	{
		Turn.Intents[Existing] = MoveTemp(Intent);
	}
	else
	{
		Turn.Intents.Add(MoveTemp(Intent));
	}
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::RemoveIntent(int32 TurnIndex, const FString& UnitId,
	FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (!Scenario.Turns.IsValidIndex(TurnIndex))
	{
		OutError = FString::Printf(TEXT("turno %d inesistente (ce ne sono %d)"), TurnIndex, Scenario.Turns.Num());
		return ERTScenarioAuthoringResult::NotFound;
	}

	FRTScenarioTurn& Turn = Scenario.Turns[TurnIndex];
	const int32 Existing = IndexOfIntent(Turn, UnitId);
	if (Existing == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("'%s' non ha un intent nel turno %d"), *UnitId, TurnIndex);
		return ERTScenarioAuthoringResult::NotFound;
	}

	Turn.Intents.RemoveAt(Existing);
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::AddExpectationUnitAtCell(const FString& UnitId,
	const FRTCellId& Cell, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (IndexOfUnit(UnitId) == INDEX_NONE)
	{
		// `Validate` rifiuterebbe comunque un'assertion su una unita' non schierata, ma dirlo QUI evita di
		// scrivere una assertion che rende lo scenario insalvabile e di scoprirlo al salvataggio.
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}

	FRTTestExpectation Expectation;
	Expectation.Kind = ERTAssertionKind::UnitAtCell;
	Expectation.UnitId = UnitId;
	Expectation.Cell = Cell;
	Scenario.Expect.Add(MoveTemp(Expectation));
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::AddExpectationLogEventCount(ERTLogCategory Category, uint8 Outcome,
	int32 Count, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (Count < 0)
	{
		OutError = FString::Printf(TEXT("LogEventCount: un conteggio non puo' essere negativo (era %d)"), Count);
		return ERTScenarioAuthoringResult::Invalid;
	}

	// `OutcomeEnumForCategory` e' la stessa funzione che il loader consulta: una categoria senza enum di esiti
	// viene rifiutata qui come la', e per la stessa ragione.
	if (URTScenarioLoader::OutcomeEnumForCategory(Category) == nullptr)
	{
		const UEnum* CategoryEnum = StaticEnum<ERTLogCategory>();
		OutError = FString::Printf(TEXT("la categoria %s non e' asseribile: non ha un enum di esiti"),
			CategoryEnum ? *CategoryEnum->GetNameStringByValue(static_cast<int64>(Category)) : TEXT("?"));
		return ERTScenarioAuthoringResult::Invalid;
	}

	FRTTestExpectation Expectation;
	Expectation.Kind = ERTAssertionKind::LogEventCount;
	Expectation.LogCategory = Category;
	Expectation.LogOutcome = Outcome;
	Expectation.Value = Count;
	Scenario.Expect.Add(MoveTemp(Expectation));
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::RemoveExpectation(int32 ExpectationIndex, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (!Scenario.Expect.IsValidIndex(ExpectationIndex))
	{
		OutError = FString::Printf(TEXT("assertion %d inesistente (ce ne sono %d)"),
			ExpectationIndex, Scenario.Expect.Num());
		return ERTScenarioAuthoringResult::NotFound;
	}

	Scenario.Expect.RemoveAt(ExpectationIndex);
	return ERTScenarioAuthoringResult::Success;
}

// --- preview (#1116) -------------------------------------------------------------------------------------

TArray<FRTCellId> FRTScenarioDraft::GetReachableCells(const FString& UnitId, UObject* Outer,
	FString& OutError) const
{
	OutError.Reset();
	TArray<FRTCellId> Result;

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return Result;
	}

	const int32 UnitIndex = IndexOfUnit(UnitId);
	if (UnitIndex == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return Result;
	}

	// La stessa arena che costruira' il runner: se ne esistessero due versioni, la preview mostrerebbe celle
	// che poi il resolver non concede, e nessuno dei due direbbe di sbagliare.
	URTHexMapAsset* Map = URTScenarioArenaLibrary::BuildArena(Scenario, Outer);
	if (!Map)
	{
		OutError = Scenario.Fixture.IsEmpty()
			? FString::Printf(TEXT("arena di raggio %d non costruibile"), Scenario.MapRadius)
			: FString::Printf(TEXT("fixture di mappa sconosciuta: '%s'"), *Scenario.Fixture);
		return Result;
	}

	// Il roster UNA volta, fuori dal ciclo: `GetHeroRoster()` costruisce quattro `URTHeroData` con tutte le
	// loro abilita' a ogni chiamata, e chiamarlo per unita' e' il costo che la review di `#1115` ha gia'
	// trovato una volta su questo stesso percorso.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();

	// Lo snapshot vuole TUTTE le unita', non solo quella interrogata: le altre occupano celle, e una preview
	// che le ignorasse offrirebbe destinazioni gia' prese.
	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Reserve(Scenario.Units.Num());
	for (const FRTScenarioUnit& Unit : Scenario.Units)
	{
		FRTHexSimUnit Sim;
		Sim.UnitId = SimUnits.Num(); // indice denso: e' l'id con cui `ReachableCells` vuole essere chiamata
		Sim.Cell = Unit.Cell;
		Sim.bAlive = true;
		Sim.Facing = Unit.Facing;

		// Il budget viene dall'eroe, dalla stessa fonte da cui lo prende `ARTUnit` (`MoveRange =
		// Hero->MovePoints`): non e' una stima, e' il valore. Un eroe che il catalogo non conosce non arriva
		// fin qui — `Validate` lo rifiuta — ma se ci arrivasse, budget zero e' l'assunzione che non inventa
		// movimento.
		URTHeroData* const* Found = Roster.FindByPredicate(
			[&Unit](const URTHeroData* H) { return H && H->HeroId == Unit.HeroId; });
		Sim.MoveBudget = (Found && *Found) ? (*Found)->MovePoints : 0;

		SimUnits.Add(MoveTemp(Sim));
	}

	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, SimUnits);

	// ⚠️ Qui non c'e' un algoritmo, c'e' una **domanda**. Budget, blocchi, occupanti e archi li ha gia'
	// applicati il servizio runtime — che e' il punto di `#1116`: nessun secondo pathfinder nell'editor.
	const TArray<FRTHexReachableCell> Reachable = URTHexSimLibrary::ReachableCells(Snapshot, UnitIndex);

	Result.Reserve(Reachable.Num());
	for (const FRTHexReachableCell& Cell : Reachable)
	{
		Result.Add(Cell.Cell);
	}
	return Result;
}
