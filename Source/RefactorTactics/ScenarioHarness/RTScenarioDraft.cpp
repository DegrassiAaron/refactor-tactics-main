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
#include "ScenarioHarness/RTScenarioRunner.h"  // il percorso di gioco reale, e nessun altro
#include "Turn/RTTurnLogLibrary.h"             // DeserializeTurnLog: le tracce le decodifica chi le scrive

void FRTScenarioDraft::NewScenario(const FString& ScenarioId, int32 MapRadius)
{
	Scenario = FRTTestScenario();
	Scenario.ScenarioId = ScenarioId;
	Scenario.MapRadius = MapRadius;
	SourcePath.Reset();
	bOpen = true;
	ForgetLastRun();

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
	// Il report di un ALTRO scenario non deve sopravvivere qui: il pannello attribuirebbe a questo l'esito,
	// l'hash e il TurnLog di quello precedente, che non e' mai stato eseguito. Idem in `Close` e `NewScenario`.
	ForgetLastRun();
	return ERTScenarioAuthoringResult::Success;
}

void FRTScenarioDraft::Close()
{
	Scenario = FRTTestScenario();
	SourcePath.Reset();
	bOpen = false;
	ForgetLastRun();
}

void FRTScenarioDraft::ForgetLastRun()
{
	LastReport = FRTScenarioRunReport();
	LastTraces.Reset();
	LastLog.Reset();
	bLogDecoded = false;
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

ERTScenarioAuthoringResult FRTScenarioDraft::SaveToFile(const FString& FilePath, FString& OutError)
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

	// ⚠️ Da qui in poi lo scenario HA una fonte, e `Reset` sa dove tornare. Senza questa riga uno scenario
	// creato nell'editor non ne aveva mai una, e `Reset` era un no-op silenzioso per l'intero flusso di
	// `#1115`: crea, modifica, salva, modifica ancora, RESET — e non tornava niente.
	SourcePath = FilePath;
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::SaveInPlace(FString& OutError)
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
	const int32 First = Scenario.Units.IndexOfByPredicate(
		[&UnitId](const FRTScenarioUnit& U) { return U.Id == UnitId; });

	// 🔴 **Se l'id fosse ambiguo, questa funzione non lo direbbe a nessuno** (#1515). Restituire il PRIMO
	// match e' la scelta giusta solo perche' gli id sono unici; se non lo fossero, `MoveUnit` sposterebbe la
	// prima e lascerebbe la seconda, `RemoveUnit` ne toglierebbe una sola e `AddUnit` rifiuterebbe ogni
	// piazzamento per quell'id — un editor che **obbedisce a meta'** senza dire perche'.
	//
	// ⚠️ **Non e' una difesa strutturale, ed e' deliberato**: l'unicita' e' gia' garantita da DUE porte, e
	// aggiungere un terzo guardiano sarebbe una difesa senza consumatore.
	//   · `OpenFromFile` -> `LoadFromString`, che termina con `Validate` e rifiuta il duplicato nominandolo;
	//   · `AddUnit`, che chiama `ValidateUnitPlacement` sul candidato prima di inserirlo.
	// Le due porte sono asserite da `Scenario.DuplicateUnitIdIsRejectedAtBothDoors`, con verifica di
	// mutazione. Questo `ensure` copre la terza strada — uno scenario **costruito da codice** o prodotto da
	// uno strumento futuro — e la trasforma da silenzio in segnale in sviluppo, senza cambiare il
	// comportamento di chi chiama ne' fallire in produzione.
	ensureMsgf(First == INDEX_NONE
		|| Scenario.Units.FindLastByPredicate(
			[&UnitId](const FRTScenarioUnit& U) { return U.Id == UnitId; }) == First,
		TEXT("FRTScenarioDraft: id unita' '%s' ambiguo — piu' di una unita' lo porta, e le operazioni di ")
		TEXT("editing agirebbero sulla prima. Lo scenario non e' passato da Validate."), *UnitId);

	return First;
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

ERTScenarioAuthoringResult FRTScenarioDraft::AddTurn(int32& OutTurnIndex, FString& OutError)
{
	OutError.Reset();
	OutTurnIndex = INDEX_NONE;

	// L'esito tipizzato e non un `int32` con sentinella: e' la convenzione che l'header di questo file
	// dichiara come regola, e la prima stesura la violava proprio qui. Un `-1` non spiegato finiva dritto in
	// `SetMoveIntent(-1, ...)`, che rispondeva «turno -1 inesistente» — un errore sul problema sbagliato, a
	// una chiamata di distanza da quello vero.
	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	OutTurnIndex = Scenario.Turns.Add(FRTScenarioTurn());
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::RemoveTurn(int32 TurnIndex, FString& OutError)
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

	// Senza questa, un turno aggiunto per sbaglio non si toglieva piu': `Validate` accetta un turno vuoto,
	// quindi il file si salvava e il runner lo GIOCAVA — un turno che nessuno ha scritto. L'unica uscita era
	// modificare il JSON a mano, in uno strumento che esiste per non farlo.
	Scenario.Turns.RemoveAt(TurnIndex);
	return ERTScenarioAuthoringResult::Success;
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
	const int32 UnitIndex = IndexOfUnit(UnitId);
	if (UnitIndex == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}
	// `ValidateScenarioTurns` rifiuta un intent dichiarato per una unita' affidata al bot: il piano lo scrive
	// il bot, e scriverlo a mano vorrebbe dire due autori per lo stesso turno. Dirlo QUI invece che al
	// salvataggio e' la stessa ragione per cui `AddExpectationUnitAtCell` controlla la sua condizione: senza,
	// il designer scopre di avere uno scenario insalvabile molto dopo averlo reso tale.
	if (Scenario.Units[UnitIndex].bBotControlled)
	{
		OutError = FString::Printf(
			TEXT("'%s' e' guidata dal bot: il suo piano lo scrive il bot, non lo scenario"), *UnitId);
		return ERTScenarioAuthoringResult::Invalid;
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

	// 🔴 Si scrive **solo il campo del movimento**, non l'intero intent.
	//
	// La prima stesura costruiva un `FRTScenarioIntent` nuovo e lo assegnava sopra quello esistente: un
	// intent che portava anche `ability`, `target`, `reaction` o una `condition` li perdeva tutti, in
	// silenzio, per un click che voleva solo cambiare il percorso. E' la stessa cancellazione muta del lavoro
	// altrui che `RemoveUnit` si rifiuta di fare tre funzioni piu' su — trovata dalla review di `#1116`.
	//
	// Un intent per unita' per turno resta la regola: un editor in cui la stessa unita' porta due piani nello
	// stesso turno e' un editor che mente, e il resolver ne applicherebbe uno solo senza dire quale.
	if (Existing != INDEX_NONE)
	{
		Turn.Intents[Existing].Move = Path;
	}
	else
	{
		FRTScenarioIntent Intent;
		Intent.UnitId = UnitId;
		Intent.Move = Path;
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
	const int32 UnitIndex = IndexOfUnit(UnitId);
	if (UnitIndex == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("unita' '%s' non schierata"), *UnitId);
		return ERTScenarioAuthoringResult::NotFound;
	}
	// `ValidateScenarioTurns` rifiuta un intent dichiarato per una unita' affidata al bot: il piano lo scrive
	// il bot, e scriverlo a mano vorrebbe dire due autori per lo stesso turno. Dirlo QUI invece che al
	// salvataggio e' la stessa ragione per cui `AddExpectationUnitAtCell` controlla la sua condizione: senza,
	// il designer scopre di avere uno scenario insalvabile molto dopo averlo reso tale.
	if (Scenario.Units[UnitIndex].bBotControlled)
	{
		OutError = FString::Printf(
			TEXT("'%s' e' guidata dal bot: il suo piano lo scrive il bot, non lo scenario"), *UnitId);
		return ERTScenarioAuthoringResult::Invalid;
	}

	FRTScenarioTurn& Turn = Scenario.Turns[TurnIndex];
	const int32 Existing = IndexOfIntent(Turn, UnitId);

	// Un intent che nomina l'unita' e nient'altro: e' cosi' che il formato dice «questa unita' non fa nulla».
	// Vedi la nota nell'header sul perche' NON e' `ability: "Action.Wait"`.
	FRTScenarioIntent Intent;
	Intent.UnitId = UnitId;

	if (Existing != INDEX_NONE)
	{
		// ⚠️ Qui sostituire e' il SIGNIFICATO dell'operazione — «non fa nulla» vuol dire che non fa nemmeno
		// cio' che faceva prima — ma non deve essere muto: se c'era un piano, chi ha cliccato deve sapere che
		// l'ha tolto. Un editor che cancella in silenzio il lavoro di qualcun altro e' il difetto che la
		// review di `#1116` ha trovato su `SetMoveIntent`, e questa e' la stessa domanda con un'altra risposta.
		const FRTScenarioIntent& Previous = Turn.Intents[Existing];
		TArray<FString> Cleared;
		if (Previous.Move.Num() > 0) { Cleared.Add(TEXT("un movimento")); }
		if (!Previous.Ability.IsNone()) { Cleared.Add(FString::Printf(TEXT("l'abilita' '%s'"), *Previous.Ability.ToString())); }
		if (!Previous.Dash.IsNone()) { Cleared.Add(FString::Printf(TEXT("la mobilita' '%s'"), *Previous.Dash.ToString())); }
		if (!Previous.Reaction.IsNone()) { Cleared.Add(FString::Printf(TEXT("la reazione '%s'"), *Previous.Reaction.ToString())); }
		if (Previous.bDeclaresFacing) { Cleared.Add(TEXT("una rotazione dichiarata")); }
		if (Previous.Condition.IsDeclared()) { Cleared.Add(TEXT("una condizione")); }

		Turn.Intents[Existing] = MoveTemp(Intent);

		if (Cleared.Num() > 0)
		{
			// L'esito resta `Success` — l'attesa e' stata scritta — ma il messaggio dice cosa e' sparito.
			OutError = FString::Printf(TEXT("'%s' ora attende: tolti %s"), *UnitId,
				*FString::Join(Cleared, TEXT(", ")));
		}
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
	const UEnum* CategoryEnum = StaticEnum<ERTLogCategory>();
	const FString CategoryName = CategoryEnum
		? CategoryEnum->GetNameStringByValue(static_cast<int64>(Category)) : TEXT("?");

	const UEnum* OutcomeEnum = URTScenarioLoader::OutcomeEnumForCategory(Category);
	if (OutcomeEnum == nullptr)
	{
		OutError = FString::Printf(TEXT("la categoria %s non e' asseribile: non ha un enum di esiti"),
			*CategoryName);
		return ERTScenarioAuthoringResult::Invalid;
	}

	// 🔴 E l'esito dev'essere un valore CHE QUELL'ENUM SA NOMINARE.
	//
	// Manca a questo controllo, e la review di `#1116` lo ha trovato: `Outcome` arriva come `uint8` nudo — in
	// Blueprint e' un pin Byte senza tendina — quindi un numero sbagliato e' l'errore che ci si aspetta.
	// Senza il controllo passava `Validate`, e il writer lo serializzava con `GetNameStringByValue` che per un
	// valore fuori scala torna stringa vuota: `"outcome": ""`. Il file finiva su disco e il loader non lo
	// rileggeva piu' — **lo strumento d'authoring scriveva uno scenario che non sapeva riaprire.**
	if (OutcomeEnum->GetNameStringByValue(static_cast<int64>(Outcome)).IsEmpty())
	{
		TArray<FString> Known;
		for (int32 I = 0; I < OutcomeEnum->NumEnums() - 1; ++I)
		{
			Known.Add(OutcomeEnum->GetNameStringByIndex(I));
		}
		Known.Sort();
		OutError = FString::Printf(TEXT("esito %d sconosciuto per la categoria %s (previsti: %s)"),
			Outcome, *CategoryName, *FString::Join(Known, TEXT(", ")));
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

TArray<FRTScenarioIntentView> FRTScenarioDraft::ListIntents(int32 TurnIndex) const
{
	TArray<FRTScenarioIntentView> Views;
	if (!bOpen || !Scenario.Turns.IsValidIndex(TurnIndex))
	{
		return Views;
	}

	const FRTScenarioTurn& Turn = Scenario.Turns[TurnIndex];
	Views.Reserve(Turn.Intents.Num());
	for (const FRTScenarioIntent& Intent : Turn.Intents)
	{
		FRTScenarioIntentView& View = Views.AddDefaulted_GetRef();
		View.UnitId = Intent.UnitId;
		View.bHasMove = Intent.Move.Num() > 0;
		View.Move = Intent.Move;
		View.Ability = Intent.Ability;

		// La riga di lista dice cosa fa l'unita', nell'ordine in cui conta per chi guarda.
		TArray<FString> Parts;
		if (View.bHasMove) { Parts.Add(FString::Printf(TEXT("Move (%d celle)"), Intent.Move.Num())); }
		if (!Intent.Ability.IsNone()) { Parts.Add(Intent.Ability.ToString()); }
		if (!Intent.Dash.IsNone()) { Parts.Add(Intent.Dash.ToString()); }
		if (!Intent.Reaction.IsNone()) { Parts.Add(Intent.Reaction.ToString()); }
		if (Intent.bDeclaresFacing) { Parts.Add(TEXT("rotazione")); }
		// Un intent che non dichiara niente E' l'attesa: e' cosi' che il formato la scrive.
		View.Summary = Parts.Num() > 0 ? FString::Join(Parts, TEXT(" + ")) : TEXT("Wait");
	}
	return Views;
}

TArray<FRTScenarioExpectationView> FRTScenarioDraft::ListExpectations() const
{
	TArray<FRTScenarioExpectationView> Views;
	if (!bOpen)
	{
		return Views;
	}

	// I nomi dei tipi si ricavano per riflessione, come li scrive il writer: una tabella parallela
	// divergerebbe dall'enum al primo tipo aggiunto.
	const UEnum* KindEnum = StaticEnum<ERTAssertionKind>();

	Views.Reserve(Scenario.Expect.Num());
	for (int32 Index = 0; Index < Scenario.Expect.Num(); ++Index)
	{
		const FRTTestExpectation& Exp = Scenario.Expect[Index];
		FRTScenarioExpectationView& View = Views.AddDefaulted_GetRef();
		View.Index = Index;
		View.Type = KindEnum ? KindEnum->GetNameStringByValue(static_cast<int64>(Exp.Kind)) : FString();
		View.UnitId = Exp.UnitId;

		switch (Exp.Kind)
		{
		case ERTAssertionKind::UnitAtCell:
			View.Summary = FString::Printf(TEXT("%s in %s"), *Exp.UnitId, *Exp.Cell.ToString());
			break;
		case ERTAssertionKind::LogEventCount:
			View.Summary = FString::Printf(TEXT("%s x%d"),
				*URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome), Exp.Value);
			break;
		default:
			// Le altre assertion il formato le conosce e questa slice non le scrive: elencarle comunque e'
			// cio' che permette di vedere — e togliere — quelle di uno scenario aperto da file.
			View.Summary = Exp.UnitId.IsEmpty()
				? FString::Printf(TEXT("%s = %d"), *View.Type, Exp.Value)
				: FString::Printf(TEXT("%s su %s = %d"), *View.Type, *Exp.UnitId, Exp.Value);
			break;
		}
	}
	return Views;
}

// --- esecuzione (#1117) ----------------------------------------------------------------------------------

ERTScenarioAuthoringResult FRTScenarioDraft::Run(UWorld* World, FString& OutError)
{
	OutError.Reset();

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}
	if (World == nullptr)
	{
		// Detto qui e non lasciato esplodere dentro il runner: chi chiama da un editor senza partita deve
		// sapere che il mondo lo deve fornire, non che «lo scenario non gira».
		OutError = TEXT("nessun mondo in cui eseguire: il runner spawna unita' e turn manager");
		return ERTScenarioAuthoringResult::Invalid;
	}

	// `Validate` PRIMA di far girare qualcosa. Il runner la rifarebbe, ma qui serve a distinguere gli esiti:
	// uno scenario invalido e' `Invalid` con l'errore che nomina il campo, non un `ERROR` di esecuzione che
	// manderebbe a cercare il difetto nel gioco.
	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		OutError = ValidationError;
		return ERTScenarioAuthoringResult::Invalid;
	}

	// ⚠️ **Il percorso reale, e nient'altro.** `URTScenarioRunner::Run` porta lo scenario attraverso piani
	// sulle unita', resolver e TurnLog. Non c'e' un ramo alternativo, non c'e' un `if Editor`, e non c'e' una
	// scorciatoia: se ci fosse, il Tactical Designer sarebbe il secondo simulatore che il §3 vieta.
	//
	// Lo scenario passa per COPIA e il draft non viene toccato: e' cio' che rende `Reset` un ritorno
	// all'initial state invece che un undo.
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);

	// ⚠️ La cache del log si invalida QUI, non solo nei cambi di scenario. La prima stesura della cache lo
	// dimenticava: chi avesse chiesto il log PRIMA di correre — un pannello che si disegna vuoto all'apertura
	// — avrebbe fissato un risultato vuoto, e dopo `Run` avrebbe continuato a mostrarlo. Il test lo ha preso
	// al primo giro, ed e' la ragione per cui `nessun log prima di correre` sta prima di `Run` in quel test.
	LastTraces = Result.TurnTraces;
	LastLog.Reset();
	bLogDecoded = false;

	FRTScenarioRunReport Report;
	Report.bHasRun = true;
	Report.ScenarioId = Result.ScenarioId;
	Report.Outcome = Result.Outcome;
	Report.OutcomeText = Result.OutcomeString();
	Report.ErrorMessage = Result.ErrorMessage;
	Report.BlockedReason = Result.BlockedReason;
	Report.TurnsPlayed = Result.TurnsPlayed;
	Report.PassedCount = Result.PassedCount();
	Report.FailedCount = Result.FailedCount();
	Report.Notes = Result.Notes;

	// Esadecimale e non un intero: `uint32` non ha un pin Blueprint, e come `int32` meta' degli hash
	// comparirebbero negativi — irriconoscibili accanto a quelli che il report headless stampa.
	// ⚠️ Con le VARIANTI il runner aggrega piu' corse e non assegna ne' `StateHash` ne' `TurnTraces`
	// all'aggregato. Mostrare `00000000` sarebbe indistinguibile da un hash vero che vale zero, e un TurnLog
	// vuoto sembrerebbe una partita che non ha fatto niente: `bHasTrace` dice che non sono applicabili.
	Report.bHasTrace = Scenario.Variants.Num() == 0;
	Report.StateHash = Report.bHasTrace ? FString::Printf(TEXT("%08x"), Result.StateHash) : FString();

	Report.Assertions.Reserve(Result.Assertions.Num());
	for (const FRTAssertionResult& Assertion : Result.Assertions)
	{
		FRTScenarioAssertionView& View = Report.Assertions.AddDefaulted_GetRef();
		View.bPassed = Assertion.bPassed;
		View.Description = Assertion.Description;
		// I due campi restano separati: e' cio' che permette al pannello di incolonnarli.
		View.Expected = Assertion.Expected;
		View.Actual = Assertion.Actual;
		View.Turn = Assertion.Turn;
	}

	LastReport = MoveTemp(Report);

	// ⚠️ L'esito dell'operazione e' `Success` anche quando lo scenario FALLISCE: sono due domande diverse.
	// «L'esecuzione e' avvenuta» e «il gioco si e' comportato come atteso» non sono la stessa cosa, e
	// confonderle farebbe apparire un `FAIL` — che e' un difetto del gioco, l'informazione piu' preziosa che
	// questo strumento produce — come un guasto dello strumento.
	return ERTScenarioAuthoringResult::Success;
}

ERTScenarioAuthoringResult FRTScenarioDraft::Reset(bool& bOutDiscardedEdits, FString& OutError)
{
	OutError.Reset();
	bOutDiscardedEdits = false;

	if (!bOpen)
	{
		OutError = TEXT("nessuno scenario aperto");
		return ERTScenarioAuthoringResult::NoScenarioOpen;
	}

	if (SourcePath.IsEmpty())
	{
		// ⚠️ Il report si scarta QUI e non prima: se il reload piu' sotto fallisse, azzerarlo in anticipo
		// lascerebbe il draft mezzo resettato — scenario intatto ed evidenza della corsa persa, cioe' proprio
		// cio' che l'header di questo modulo dichiara di non fare mai.
		ForgetLastRun();
		// Uno scenario mai salvato non ha un «file dice» a cui tornare. Non e' un errore: il report e' stato
		// scartato, e l'initial state e' quello che l'authoring ha in mano — non c'e' altra fonte.
		return ERTScenarioAuthoringResult::Success;
	}

	// ⚠️ Si ricarica dalla FONTE, non si disfa la partita. `Run` non ha modificato lo scenario, quindi non
	// c'e' niente da annullare: quello che questa riga recupera sono le modifiche d'authoring non salvate, e
	// `#1117` chiede esattamente questo — che `RESET` riporti a cio' che il file DICHIARA, non allo stato
	// precedente.
	FRTTestScenario Reloaded;
	FString LoadError;
	if (!URTScenarioLoader::LoadFromFile(SourcePath, Reloaded, LoadError))
	{
		// Niente e' stato toccato: il report e' ancora consultabile, e chi ha appena eseguito puo' rileggere
		// l'hash della corsa che ha fatto invece di trovarsi con le mani vuote.
		OutError = FString::Printf(TEXT("il file di origine non si rilegge: %s"), *LoadError);
		return ERTScenarioAuthoringResult::NotFound;
	}

	// ⚠️ **Il confronto e' sul CONTENUTO, non sui conteggi.**
	//
	// La prima stesura confrontava `Units.Num()`, `Turns.Num()` ed `Expect.Num()`, e non vedeva la modifica
	// piu' comune di tutte: spostare una unita' cambia una cella e nessun conteggio. Il test l'ha preso
	// subito — ed era lo stesso difetto delle due review precedenti, un controllo che sembra funzionare
	// perche' nessuno gli ha mostrato il caso normale.
	//
	// La forma canonica del writer e' deterministica per costruzione (`#1114`), quindi due testi diversi
	// significano due scenari diversi. Se uno dei due non si serializza — puo' succedere a uno scenario in
	// lavorazione — si assume che ci siano modifiche: il costo di un avviso di troppo e' una frase, quello di
	// un avviso mancato e' lavoro perso senza sapere perche'.
	FString Current;
	FString OnDisk;
	FString Ignored;
	const bool bBothSerialize = URTScenarioLoader::SaveToString(Scenario, Current, Ignored)
		&& URTScenarioLoader::SaveToString(Reloaded, OnDisk, Ignored);
	const bool bHadUnsavedEdits = !bBothSerialize || Current != OnDisk;

	Scenario = MoveTemp(Reloaded);
	ForgetLastRun();

	// Perdere modifiche e' il significato dell'operazione, ma dirlo non lo e'. Lo dice un flag e NON
	// `OutError`: un avviso scritto nel canale degli errori non si distingue da un errore, e chi lo legge
	// dovrebbe confrontare stringhe per capire quale dei due ha ricevuto.
	bOutDiscardedEdits = bHadUnsavedEdits;
	return ERTScenarioAuthoringResult::Success;
}

TArray<FRTScenarioLogEntryView> FRTScenarioDraft::GetLastRunLog() const
{
	// ⚠️ **Decodificato una volta e messo in cache.**
	//
	// Le tracce restano byte finche' qualcuno non chiede il log — chi vuole solo sapere se e' PASS non paga la
	// decodifica — ma la prima stesura la rifaceva a OGNI chiamata. Un nodo Blueprint legato a una lista viene
	// rivalutato a ogni frame, quindi «una volta per richiesta» sarebbe diventato «una volta per frame per
	// widget»: `DeserializeTurnLog` su tutte le tracce piu' `DescribeLogEvent` per voce, sessanta volte al
	// secondo. La cache si azzera in `ForgetLastRun`, cioe' a ogni `Run`, `Reset`, apertura o chiusura.
	if (bLogDecoded)
	{
		return LastLog;
	}

	TArray<FRTScenarioLogEntryView> Views;

	for (int32 TurnIndex = 0; TurnIndex < LastTraces.Num(); ++TurnIndex)
	{
		TArray<FRTTurnLogEntry> Entries;
		if (!URTTurnLogLibrary::DeserializeTurnLog(LastTraces[TurnIndex].Bytes, Entries))
		{
			// Una traccia illeggibile non fa sparire le altre: il log e' uno strumento di diagnosi, e un
			// turno mancante e' meno peggio di un pannello vuoto che non dice perche'.
			continue;
		}

		for (const FRTTurnLogEntry& Entry : Entries)
		{
			FRTScenarioLogEntryView& View = Views.AddDefaulted_GetRef();
			View.Turn = TurnIndex + 1; // i turni si contano da 1 per chi legge, da 0 per l'array
			View.Category = Entry.Category;
			// Il nome leggibile lo compone il loader, che e' l'unico posto che sa quale enum di esiti
			// appartiene a quale categoria: una tabella qui divergerebbe da quella.
			View.Event = URTScenarioLoader::DescribeLogEvent(Entry.Category, Entry.Outcome);
			View.FromCell = Entry.SrcCell;
			View.ToCell = Entry.TgtCell;
			// Il numero e l'azione: senza, il pannello direbbe «Combat.Hit» senza dire quanto, e
			// `Riktor.Interposition` sarebbe indistinguibile da `Action.Intercept`.
			View.Amount = Entry.Amount;
			View.ActionId = Entry.ActionId;
		}
	}

	LastLog = MoveTemp(Views);
	bLogDecoded = true;
	return LastLog;
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
