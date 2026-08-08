#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace
{
	/**
	 * Arena esagonale piena di raggio N, piu' le celle modificate dallo scenario.
	 *
	 * RIUSA l'actor mappa gia' presente se c'e': lo stesso codice gira in un mondo vuoto (test) e in una PIE
	 * dove il GameMode ha gia' allestito. Spawnarne un secondo darebbe due griglie sovrapposte.
	 */
	/**
	 * Le capability che il gioco possiede **oggi**. Un turno che ne chiede una assente non viene giocato, e
	 * lo scenario si dichiara `Blocked` invece di fallire.
	 *
	 * L'elenco sta nel CODICE e non nei dati: se stesse nello scenario, dichiarare disponibile una capability
	 * inesistente sarebbe una modifica al JSON, e il primo scenario verde e bugiardo arriverebbe da li'.
	 */
	bool IsCapabilityAvailable(const FString& Capability)
	{
		static const TSet<FString> Available = {
			TEXT("FixtureReference"),  // lo scenario riferisce la geometria per nome
			TEXT("Reaction"),          // E5: reazioni componibili, automatiche (AllowedResponses <= 1)
			TEXT("Environment"),       // E8: superfici, stati, propagazione
			TEXT("Cover"),             // E9 CP 9.1/9.2: coperture bassa e alta, distruzione
			TEXT("Structures"),        // E9 CP 9.3: porte come bordo, revisione della mappa
		};
		return Available.Contains(Capability);
	}

	/**
	 * L'arena dello scenario: una **fixture riferita per nome** se dichiarata, altrimenti generata dal raggio.
	 *
	 * Riferire invece di duplicare tiene la geometria canonica in UN posto solo, gia' protetto da un test
	 * (`ShowcaseRelay.BasinLayoutMatchesSpec`). Gli override di cella restano validi e si applicano SOPRA la
	 * fixture: le due strade differiscono solo per come nasce la mappa.
	 */
	URTHexMapAsset* BuildScenarioArena(UWorld* World, const FString& Fixture, int32 Radius,
		const TArray<FRTScenarioCell>& Overrides, ARTHexMapActor*& OutActor)
	{
		URTHexMapAsset* Map = nullptr;
		if (!Fixture.IsEmpty())
		{
			// Nome sconosciuto -> nullptr, mai un'arena vuota: quella farebbe girare la partita e produrrebbe
			// un fallimento che parla di unita' fuori mappa invece che della fixture inesistente.
			Map = URTMatchSetupLibrary::MakeFixtureArena(World, Fixture);
			if (!Map)
			{
				return nullptr;
			}
		}
		else
		{
			Map = NewObject<URTHexMapAsset>();
			for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
			{
				Map->AddOrUpdateCell(FRTHexCellData(Id));
			}
		}

		// Le modifiche DOPO l'arena piena: una cella elencata due volte vince l'ultima, e l'esito non dipende
		// dall'ordine di generazione.
		for (const FRTScenarioCell& Spec : Overrides)
		{
			FRTHexCellData Cell(Spec.Cell);
			Cell.bBlocksMovement = Spec.bBlocksMovement;
			Cell.bBlocksLineOfSight = Spec.bBlocksLineOfSight;
			if (Spec.MoveCost > 0)
			{
				Cell.MoveCost = Spec.MoveCost;
			}
			Map->AddOrUpdateCell(Cell);
		}
		Map->SortCells();

		ARTHexMapActor* Actor = ARTHexMapActor::FindInWorld(World);
		if (!Actor)
		{
			Actor = World->SpawnActor<ARTHexMapActor>();
		}
		if (!Actor)
		{
			return nullptr;
		}
		Actor->MapAsset = Map;
		Actor->RebuildInstances(); // la vista ISM segue l'asset: senza, in PIE resterebbe la mappa precedente
		OutActor = Actor;
		return Map;
	}

	/** L'eroe del catalogo con quell'ID stabile, o nullptr. Il roster e' la fonte: nessun elenco duplicato. */
	URTHeroData* FindScenarioHero(FName HeroId)
	{
		for (URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId)
			{
				return Hero;
			}
		}
		return nullptr;
	}
}

bool FRTScenarioSession::Start(UWorld* InWorld, const FRTTestScenario& InScenario)
{
	Scenario = InScenario;
	Result = FRTTestResult();
	Result.ScenarioId = Scenario.ScenarioId;
	Result.Seed = Scenario.Seed;

	auto Fail = [this](const FString& Reason) -> bool
	{
		// Tutto cio' che va storto qui e' ERROR, non FAIL: non si e' potuto eseguire, quindi il difetto e' nel
		// test o nell'ambiente, non nel gioco. Confonderli fa cercare una regressione che non esiste.
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = Reason;
		State = EState::Finished;
		return false;
	};

	if (!InWorld)
	{
		return Fail(TEXT("nessun mondo in cui eseguire lo scenario"));
	}
	World = InWorld;

	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		return Fail(FString::Printf(TEXT("scenario non valido: %s"), *ValidationError));
	}

	ARTHexMapActor* MapActor = nullptr;
	Map = BuildScenarioArena(InWorld, Scenario.Fixture, Scenario.MapRadius, Scenario.Cells, MapActor);
	if (!Map)
	{
		// Il nome sbagliato si dice, non si aggira: e' un difetto dello SCENARIO, e va nominato.
		return Fail(Scenario.Fixture.IsEmpty()
			? TEXT("impossibile creare l'arena esagonale")
			: *FString::Printf(TEXT("fixture di mappa sconosciuta: '%s'"), *Scenario.Fixture));
	}

	// Origine e scala dall'ACTOR mappa, non dall'origine del mondo: e' la stessa fonte che usa `RebuildInstances`
	// per disegnare la griglia e che usa l'allestimento della partita normale. Prendendo `FVector::ZeroVector`,
	// con un actor mappa spostato nel livello le unita' finivano in un punto e la griglia in un altro — e la
	// camera, inquadrando le unita', lasciava la mappa fuori campo. Osservato in PIE su L_DevSandbox.
	FVector MapOrigin = FVector::ZeroVector;
	float MapHexSize = Map->HexSize;
	float MapLayerHeight = Map->LayerHeight;
	if (MapActor)
	{
		MapActor->GetHexContext(MapOrigin, MapHexSize, MapLayerHeight);
	}

	for (const FRTScenarioUnit& Spec : Scenario.Units)
	{
		URTHeroData* Hero = FindScenarioHero(Spec.HeroId);
		if (!Hero)
		{
			// Validate() lo esclude gia', ma la sessione non si fida di un invariante altrui.
			return Fail(FString::Printf(TEXT("eroe '%s' non nel catalogo"), *Spec.HeroId.ToString()));
		}

		ARTUnit* Unit = InWorld->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit)
		{
			return Fail(FString::Printf(TEXT("spawn fallito per l'unita' '%s'"), *Spec.Id));
		}
		Unit->TeamId = Spec.TeamId;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		// Le unita' dello scenario NON sono bot: gli intent li decide il file, non l'utility scoring.
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Spec.Cell, MapOrigin, MapHexSize, MapLayerHeight);

		UnitsById.Add(Spec.Id, Unit);
	}

	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(InWorld, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		TM = InWorld->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	}
	if (!TM)
	{
		return Fail(TEXT("impossibile creare il turn manager"));
	}
	TurnManager = TM;

	// Il timer del turn manager va SPENTO: e' la sessione a decidere quando si chiude la pianificazione. Se
	// scadesse per conto suo risolverebbe un turno che la sessione non ha preparato — ed e' esattamente il
	// difetto dei turni fantasma, visto in PIE prima che questa riga esistesse.
	TM->SetPlanningSeconds(0.f);

	if (Scenario.Turns.Num() == 0)
	{
		// Uno scenario senza turni e' legittimo: verifica solo lo stato iniziale.
		Finish();
		return true;
	}

	State = EState::PauseBeforeTurn;
	PauseElapsed = 0.f;
	TurnIndex = 0;
	return true;
}

void FRTScenarioSession::BeginTurn()
{
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM || !Scenario.Turns.IsValidIndex(TurnIndex))
	{
		Finish();
		return;
	}

	// Il turno chiede qualcosa che il gioco non sa ancora fare? Ci si ferma QUI, dichiarando cosa manca.
	// Non si gioca "quel che si puo'" del turno: un turno a meta' produrrebbe uno stato che non corrisponde
	// ne' al gioco di oggi ne' a quello di domani, e ogni assertion successiva mentirebbe.
	for (const FString& Required : Scenario.Turns[TurnIndex].Requires)
	{
		if (!IsCapabilityAvailable(Required))
		{
			BlockedBy = FString::Printf(TEXT("turno %d: manca la capability '%s'"), TurnIndex + 1, *Required);
			Finish();
			return;
		}
	}

	// Tutte ferme per default: un'unita' senza intent nel turno NON eredita il piano del turno prima.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
			// Anche il piano d'ATTACCO: senza, un'unita' che ha attaccato al turno 1 continuerebbe a farlo
			// nei turni successivi senza che lo scenario glielo chieda.
			U->PlannedAbilityIndex = INDEX_NONE;
			U->PlannedAttackTarget = nullptr;
			// Lo SCATTO non si azzera qui, ed e' deliberato: `RTTurnManager` lo consuma a ogni risoluzione
			// («consumato per questo turno, valido o no»), quindi un reset in questo punto sarebbe una seconda
			// copia della stessa regola — quella che smette di essere aggiornata quando la prima cambia.
			//
			// Verificato per mutazione: azzerarlo qui non fa cadere alcun test, perche' non c'e' niente da
			// azzerare. E' il modo in cui questa riga, scritta d'istinto insieme alle due sopra, e' stata tolta.
		}
	}

	for (const FRTScenarioIntent& Intent : Scenario.Turns[TurnIndex].Intents)
	{
		TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Intent.UnitId);
		ARTUnit* Unit = Found ? Found->Get() : nullptr;
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}

		// L'abilita' si cerca per ActionId: l'indice nel kit si sposta appena qualcuno ne aggiunge una, e uno
		// scenario che punta a un indice continuerebbe a passare verificando l'abilita' sbagliata.
		const auto FindAbilityIndex = [Unit](const FName& ActionId) -> int32
		{
			for (int32 I = 0; I < Unit->NumAbilities(); ++I)
			{
				const URTActionData* Ability = Unit->GetAbility(I);
				if (Ability && Ability->Def.ActionId == ActionId) { return I; }
			}
			return INDEX_NONE;
		};

		// --- scatto (fase Dash, PRIMA del Blast) ------------------------------------------------------------
		// Slot distinto dall'abilita' perche' dopo D-028 lo sono davvero: lo scatto prende il movimento, non la
		// principale. Un intent puo' dichiarare entrambi, ed e' *schivo e sparo*.
		if (!Intent.Dash.IsNone())
		{
			const int32 DashIndex = FindAbilityIndex(Intent.Dash);
			if (DashIndex == INDEX_NONE)
			{
				ErroredBy = FString::Printf(TEXT("'%s' non possiede la mobilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Dash.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else
			{
				// Si scrive il PIANO, non l'esito: traiettoria, ostacoli e chi viene attraversato li decide il
				// resolver in fase Dash. Anticiparli qui significherebbe verificare le regole della sessione
				// invece di quelle del gioco.
				Unit->PlannedDashAbility = DashIndex;
				Unit->PlannedDashCell = Intent.DashCell;
			}
		}

		// --- abilita' -------------------------------------------------------------------------------------
		// Stessa strada del controller: si scrivono `PlannedAbilityIndex` e `PlannedAttackTarget`, esattamente
		// come dopo un click sul nemico. Portata, LOS, cooldown ed energia li valuta il turn manager al momento
		// della risoluzione — la sessione non li anticipa, altrimenti verificherebbe le proprie regole invece
		// di quelle del gioco.
		if (!Intent.Ability.IsNone())
		{
			TWeakObjectPtr<ARTUnit>* FoundTarget = UnitsById.Find(Intent.Target);
			ARTUnit* Target = FoundTarget ? FoundTarget->Get() : nullptr;

			const int32 AbilityIndex = FindAbilityIndex(Intent.Ability);

			if (AbilityIndex == INDEX_NONE)
			{
				// Un'abilita' che l'eroe non possiede e' un errore di SCRITTURA dello scenario, non un esito di
				// gioco. Prima finiva in un log e l'attacco semplicemente non partiva: l'assertion sui danni
				// cadeva e il report diceva FAIL, cioe' mandava a cercare una regressione che non esisteva.
				//
				// Il validator non puo' prenderlo al caricamento: `Riva.CircularTide` ESISTE nel catalogo, non
				// e' nel kit di Flux — e il kit lo si conosce solo quando le unita' sono state costruite.
				ErroredBy = FString::Printf(TEXT("'%s' non possiede l'abilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Ability.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else if (!Target || !Target->IsAlive())
			{
				// Non e' un errore: e' una partita andata cosi'. Ma tacerlo lascia senza spiegazione l'assertion
				// che cadra' subito dopo — «perche' non ha attaccato?» e' la domanda che il report deve evitare
				// di far nascere.
				//
				// `!Target` copre il caso REALE, e non era ovvio: `DestroyDefeatedUnits` rimuove le unita'
				// abbattute a fine turno, quindi un morto non si osserva come `IsAlive() == false` ma come weak
				// pointer NULLO. Il controllo su `IsAlive()` da solo non scattava mai. L'id, invece, e' garantito
				// dichiarato: il loader rifiuta un intent che nomini un'unita' inesistente, quindi qui un
				// puntatore nullo significa «c'era e non c'e' piu'», non «non e' mai esistita».
				Notes.Add(FString::Printf(TEXT("turno %d: '%s' bersagliava '%s', gia' abbattuto: l'azione non parte"),
					TurnIndex + 1, *Intent.UnitId, *Intent.Target));
			}
			else
			{
				Unit->PlannedAbilityIndex = AbilityIndex;
				Unit->PlannedAttackTarget = Target;
			}
		}

		if (Intent.Move.Num() == 0)
		{
			continue;
		}

		// Stessa strada del controller: i waypoint diventano un percorso composito calcolato sullo snapshot
		// AUTOREVOLE. Percorso non valido (budget, blocchi, occupanti) -> l'unita' resta ferma e l'assertion
		// lo mostra: e' il comportamento del gioco, non un caso speciale del test.
		TArray<ARTUnit*> SnapshotUnits;
		const FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(SnapshotUnits);
		const int32 UnitId = SnapshotUnits.IndexOfByKey(Unit);
		if (UnitId == INDEX_NONE)
		{
			continue;
		}

		const FRTHexPathResult Path = URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, Intent.Move);
		if (Path.Path.Num() >= 2)
		{
			Unit->PlannedWaypoints = Intent.Move;
			Unit->PlannedPath = Path.Path;
			Unit->PlannedCell = Path.Path.Last();
		}
		else
		{
			Notes.Add(FString::Printf(TEXT("turno %d: percorso rifiutato per '%s': l'unita' resta ferma"),
				TurnIndex + 1, *Intent.UnitId));
			UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: percorso rifiutato per '%s' (l'unita' resta ferma)"),
				*Scenario.ScenarioId, *Intent.UnitId);
		}
	}

	// Uno scenario scritto male non si gioca: fermarsi qui evita di produrre uno stato che nessuna assertion
	// puo' interpretare, e soprattutto evita di riportarlo come se fosse un verdetto sul gioco.
	if (!ErroredBy.IsEmpty())
	{
		Finish();
		return;
	}

	TM->LockInAndResolve();
	State = EState::Resolving;
	ResolveTicks = 0;
}

void FRTScenarioSession::Step(float DeltaSeconds, bool bPumpTurnManager)
{
	ARTTurnManager* TM = TurnManager.Get();
	if (State == EState::Finished || !TM)
	{
		return;
	}

	switch (State)
	{
	case EState::PauseBeforeTurn:
	{
		// La partita puo' essersi decisa prima della fine dello scenario: i turni restanti non esistono.
		if (TM->GetPhase() == ERTMatchPhase::MatchEnded)
		{
			Finish();
			return;
		}
		PauseElapsed += DeltaSeconds;
		if (PauseElapsed >= TurnPauseSeconds)
		{
			BeginTurn();
		}
		break;
	}

	case EState::Resolving:
	{
		// In gioco e' il mondo a ticcare il turn manager: pomparlo anche qui lo farebbe correre al doppio
		// della velocita', e il playback che si vuole GUARDARE passerebbe in meta' del tempo.
		if (bPumpTurnManager)
		{
			TM->Tick(DeltaSeconds);
		}

		if (!TM->IsResolving())
		{
			++TurnIndex;
			Result.TurnsPlayed = TurnIndex;
			if (TurnIndex >= Scenario.Turns.Num())
			{
				Finish();
			}
			else
			{
				State = EState::PauseBeforeTurn;
				PauseElapsed = 0.f;
			}
			break;
		}

		// Tetto di sicurezza: una risoluzione che non finisce deve FALLIRE, non girare all'infinito. Senza,
		// un test appeso somiglierebbe a un test lento, e la differenza si scoprirebbe solo aspettando.
		if (++ResolveTicks > URTScenarioRunner::MaxResolveTicks)
		{
			Result.Outcome = ERTTestOutcome::Error;
			Result.ErrorMessage = FString::Printf(
				TEXT("il turno %d non ha finito di risolvere entro %d passi"),
				TurnIndex + 1, URTScenarioRunner::MaxResolveTicks);
			State = EState::Finished;
		}
		break;
	}

	default:
		break;
	}
}

void FRTScenarioSession::Finish()
{
	// Piani AZZERATI: se restassero appesi, qualunque cosa risolvesse un turno dopo lo scenario li
	// ri-eseguirebbe. E' successo davvero, e in PIE sembrava lo scenario stesso.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
			// Anche il piano d'ATTACCO: senza, un'unita' che ha attaccato al turno 1 continuerebbe a farlo
			// nei turni successivi senza che lo scenario glielo chieda.
			U->PlannedAbilityIndex = INDEX_NONE;
			U->PlannedAttackTarget = nullptr;
		}
	}

	// Digest dello stato finale (FNV-1a, stesso idioma di `URTTurnLogLibrary::HashTurnLog`). Le unita' si
	// ordinano per ID di scenario: l'ID viene dal file ed e' stabile, l'ordine di `TMap` no.
	{
		TArray<FString> Ids;
		UnitsById.GetKeys(Ids);
		Ids.Sort();

		uint32 Hash = 2166136261u;
		auto Mix = [&Hash](uint32 V) { Hash ^= V; Hash *= 16777619u; };
		for (const FString& Id : Ids)
		{
			const ARTUnit* Unit = UnitsById[Id].Get();
			if (!Unit)
			{
				continue;
			}
			for (const TCHAR Ch : Id)
			{
				Mix(static_cast<uint32>(Ch));
			}
			Mix(static_cast<uint32>(Unit->Cell.X));
			Mix(static_cast<uint32>(Unit->Cell.Y));
			Mix(static_cast<uint32>(Unit->Cell.Layer));
			Mix(static_cast<uint32>(Unit->Health));
			Mix(static_cast<uint32>(Unit->Shield));
			Mix(static_cast<uint32>(Unit->Energy));
			Mix(Unit->IsAlive() ? 1u : 0u);
		}
		Result.StateHash = Hash;
	}

	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		FRTAssertionResult A;
		A.Kind = Exp.Kind;
		A.Turn = Result.TurnsPlayed;

		switch (Exp.Kind)
		{
		case ERTAssertionKind::UnitAtCell:
		{
			A.Description = FString::Printf(TEXT("UnitAtCell(%s)"), *Exp.UnitId);
			A.Expected = Exp.Cell.ToString();

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				A.Actual = TEXT("unita' assente");
				A.bPassed = false;
			}
			else
			{
				A.Actual = Unit->Cell.ToString();
				A.bPassed = (Unit->Cell == Exp.Cell);
			}
			break;
		}
		case ERTAssertionKind::TurnsCompleted:
		{
			A.Description = TEXT("TurnsCompleted");
			A.Expected = FString::Printf(TEXT(">= %d"), Exp.Value);
			A.Actual = FString::FromInt(Result.TurnsPlayed);
			A.bPassed = (Result.TurnsPlayed >= Exp.Value);
			break;
		}
		case ERTAssertionKind::UnitHpEquals:
		{
			A.Description = FString::Printf(TEXT("UnitHpEquals(%s)"), *Exp.UnitId);
			A.Expected = FString::FromInt(Exp.Value);

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				// Un'unita' ABBATTUTA puo' essere stata distrutta: distinguerlo da «non esiste» conta, perche'
				// sono due difetti diversi — uno di gioco, uno di scenario.
				A.Actual = TEXT("unita' assente (abbattuta o mai creata)");
				A.bPassed = false;
			}
			else
			{
				// Lo SCUDO si dichiara separatamente: qui si guardano gli HP, e un danno assorbito dallo scudo
				// deve risultare come «HP invariati», non come «nessun danno».
				A.Actual = FString::Printf(TEXT("%d (scudo %d)"), Unit->Health, Unit->Shield);
				A.bPassed = (Unit->Health == Exp.Value);
			}
			break;
		}
		case ERTAssertionKind::UnitAlive:
		{
			const bool bWantAlive = (Exp.Value != 0);
			A.Description = FString::Printf(TEXT("UnitAlive(%s)"), *Exp.UnitId);
			A.Expected = bWantAlive ? TEXT("viva") : TEXT("abbattuta");

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			// Un'unita' rimossa dal mondo conta come abbattuta: e' il modo in cui il gioco toglie di mezzo chi
			// arriva a zero HP, e chiedere «e' viva?» a un puntatore nullo deve avere una risposta, non un crash.
			const bool bIsAlive = (Unit != nullptr && Unit->IsAlive());
			A.Actual = bIsAlive ? TEXT("viva") : TEXT("abbattuta");
			A.bPassed = (bIsAlive == bWantAlive);
			break;
		}
		default:
			A.Description = TEXT("assertion non implementata");
			A.bPassed = false;
			break;
		}

		Result.Assertions.Add(A);
	}

	Result.Notes = Notes;

	// Precedenza: ERROR > FAIL > BLOCKED > PASS.
	//
	// L'ERROR viene per primo perche' e' l'unico che parla di CHI HA SBAGLIATO invece che di cosa e'
	// successo: se lo scenario e' scritto male, ogni assertion che segue misura uno stato che non doveva
	// esistere, e chiamarla FAIL manderebbe a cercare una regressione inesistente.
	//
	// Poi un FAIL vero batte il BLOCKED: un'assertion caduta PRIMA del punto di blocco riguarda codice che
	// esiste ed e' rotto — nasconderla dietro "non e' ancora pronto" sarebbe il modo piu' comodo di perdere
	// una regressione.
	if (!ErroredBy.IsEmpty())
	{
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = ErroredBy;
	}
	else if (Result.FailedCount() > 0)
	{
		Result.Outcome = ERTTestOutcome::Fail;
	}
	else if (!BlockedBy.IsEmpty())
	{
		Result.Outcome = ERTTestOutcome::Blocked;
		Result.BlockedReason = BlockedBy;
	}
	else
	{
		Result.Outcome = ERTTestOutcome::Pass;
	}
	State = EState::Finished;

	UE_LOG(LogRT, Log, TEXT("[RT-Test] %s: %s (%d/%d assertion, %d turni)"),
		*Result.ScenarioId, *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed);
}
