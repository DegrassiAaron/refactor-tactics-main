#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // MapResolutionPhase: un'azione di Prep non ha un bersaglio da dichiarare
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
#include "Player/RTPlayerController.h"
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
			TEXT("CreateCover"),       // E9 CP 9.5: coperture erette in partita, temporanee, spostabili
			// D-046 (#282): un EROE possiede davvero un'azione ambientale. Non basta che il resolver la sappia
			// risolvere — per mesi la sapeva, e nessuna unita' poteva innescarla. Oggi `Flux.ConductiveNode` e'
			// `Action.Electrify` e `Riva.FluidTrail` e' `Action.CreateWater`.
			//
			// NON copre `Action.Ignite` ne' `Action.ModifyArc`: nessun eroe del roster le possiede, e D-046 ha
			// deciso che restino senza owner in v0.1 (nessuna affinita' col fuoco; i ponti non appartengono a
			// nessun kit). Uno scenario che chieda di accenderle deve restare BLOCKED, ed e' il motivo per cui
			// questa capability non si chiama «Environment»: quella c'e' gia' e dice un'altra cosa.
			TEXT("EnvironmentalActionOwner"),
		};
		// NON disponibile, e la riga che manca vale quanto quelle che ci sono:
		//
		//   `ReactionPlanning` — dichiarare una reazione IN PIANIFICAZIONE. `PlannedReactionAbility` esiste,
		//   il resolver lo legge in due punti e l'HUD pure, ma in tutto il progetto lo SCRIVONO solo i test:
		//   ne' il controller ne' il bot. Dare agli scenari uno slot `reaction` renderebbe l'harness il primo
		//   produttore di quel campo — cioe' piu' CAPACE del gioco, e i suoi verdi direbbero che il giocatore
		//   puo' preparare una parata quando non puo'. E' il rovescio esatto del caso `ValidateActionSlots`,
		//   dove l'harness rischiava di essere piu' SEVERO del gioco. Entrambe le asimmetrie mentono.
		//
		// Il produttore nasce con le finestre di reazione (E14/S5-1). Fino ad allora un turno che chiede
		// `ReactionPlanning` e' BLOCKED, che e' la verita' e costa una riga.
		//
		//   `DeclaredRotation` — dichiarare una ROTAZIONE in pianificazione (D-020). Dopo #291 la catena esiste
		//   quasi tutta: il campo sta su `ARTUnit`, entra in `FRTPlannedIntent`, passa da `FilterForTeam`, e il
		//   `TurnManager` lo consuma a fine Move producendo `DeclaredInPlanning` o `DeclarationRejected`. Manca
		//   il solo anello che non e' una regola: l'INPUT. Nessun comando lo dichiara e il bot non lo usa, quindi
		//   dare agli scenari una chiave `facing` renderebbe l'harness il primo produttore del campo — la stessa
		//   asimmetria di `ReactionPlanning`, con lo stesso esito: verdi che dicono che il giocatore puo' girarsi
		//   restando fermo, mentre non ha alcun modo di chiederlo. L'input e' lavoro di E11, e senza l'insieme
		//   legale mostrato a schermo il giocatore non saprebbe nemmeno quali tre direzioni gli restano.
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

	/**
	 * Posizione della PRIMA occorrenza di un evento nel log, o `INDEX_NONE`.
	 *
	 * «Prima occorrenza» e non «una qualsiasi»: `LogEventOrder` chiede se una cosa e' successa prima di
	 * un'altra, e con eventi ripetuti l'unica formulazione che non dipende da quale coppia si sceglie e'
	 * confrontare i due esordi.
	 */
	int32 IndexOfScenarioLogEvent(const TArray<FRTTurnLogEntry>& Log, ERTLogCategory Category, uint8 Outcome)
	{
		for (int32 I = 0; I < Log.Num(); ++I)
		{
			if (Log[I].Category == Category && Log[I].Outcome == Outcome) { return I; }
		}
		return INDEX_NONE;
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
	ApplyPreviewSelection();
	return true;
}

void FRTScenarioSession::ApplyPreviewSelection()
{
	if (Scenario.PreviewUnit.IsEmpty() || !Scenario.Turns.IsValidIndex(0))
	{
		return;
	}

	UWorld* W = World.Get();
	// Nessun controller = headless. E' il ramo che garantisce che questo campo non possa spostare un esito:
	// dove si misura, non gira.
	//
	// `GetActorOfClass` e non `GetFirstPlayerController()`: quest'ultima legge la lista di controller del
	// mondo, che si popola solo attraverso il login del GameMode — in un mondo di test un controller
	// spawnato con `SpawnActor` non ci finisce, e il ramo restava non esercitabile. Cercare l'ATTORE e' anche
	// il modo in cui il resto del progetto trova turn manager, mappa e camera.
	ARTPlayerController* PC = W
		? Cast<ARTPlayerController>(UGameplayStatics::GetActorOfClass(W, ARTPlayerController::StaticClass()))
		: nullptr;
	if (!PC)
	{
		return;
	}

	TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Scenario.PreviewUnit);
	ARTUnit* Unit = Found ? Found->Get() : nullptr;
	if (!Unit)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: previewUnit '%s' non esiste, nessuna selezione"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit);
		return;
	}

	// I TRE GESTI del giocatore, nell'ordine in cui li farebbe: scegli l'abilita', seleziona la tua unita',
	// clicca il nemico. Passando dalle stesse funzioni, e non scrivendo lo stato a mano: un'anteprima
	// costruita per scorciatoia assomiglierebbe a quella del gioco senza esserlo, ed e' proprio quella la
	// bugia che questo scenario dovrebbe smascherare.
	//
	// Il primo tentativo chiamava il solo `HandleClickOnUnit(Unit)` credendo che selezionasse. Non seleziona:
	// presuppone una selezione e tratta l'argomento come BERSAGLIO, quindi usciva subito e a schermo non
	// compariva niente — mentre il log qui sotto dichiarava successo. Da li' il `Verify` in fondo.
	ARTUnit* Target = nullptr;
	int32 AbilityIndex = INDEX_NONE;
	for (const FRTScenarioIntent& Intent : Scenario.Turns[0].Intents)
	{
		if (Intent.UnitId != Scenario.PreviewUnit || Intent.Ability.IsNone() || Intent.bTargetsCell)
		{
			continue;
		}
		TWeakObjectPtr<ARTUnit>* FoundTarget = UnitsById.Find(Intent.Target);
		Target = FoundTarget ? FoundTarget->Get() : nullptr;
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* Ability = Unit->GetAbility(I);
			if (Ability && Ability->Def.ActionId == Intent.Ability)
			{
				AbilityIndex = I;
				break;
			}
		}
		break;
	}

	if (!Target || AbilityIndex == INDEX_NONE)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: '%s' non ha un attacco su bersaglio al turno 1, nessuna anteprima"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit);
		return;
	}

	Unit->SelectedAbilityIndex = AbilityIndex;                  // come premere il tasto dell'abilita'
	PC->SelectUnit(Unit, /*bRecordAsPlayerInput=*/ false);      // come cliccare la propria unita'
	PC->HandleClickOnUnit(Target);                              // come cliccare il nemico

	// VERIFICA invece di dichiarare. La riga precedente stampava «selezionata» subito dopo la chiamata, e ha
	// raccontato per un'intera sessione un successo che non c'era: se il piano non e' atterrato, il log deve
	// dirlo qui, dove si sa, invece di lasciarlo scoprire a chi guarda uno schermo che non cambia.
	if (Unit->PlannedAttackTarget == Target)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: '%s' punta '%s' — anteprima attiva, guardala prima che risolva"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit, *Target->GetName());
	}
	else
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: l'anteprima NON e' partita — il bersaglio e' stato rifiutato "
			"(portata, linea di tiro o abilita' non pronta). Le righe [RT] qui sopra dicono quale"),
			*Scenario.ScenarioId);
	}
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
			U->bAttackTargetsCell = false;
			// E il BORDO dichiarato: senza, un pannello eretto al turno 1 lascerebbe il lato scritto nel piano
			// e un'azione di struttura del turno 3 lo troverebbe gia' pronto senza averlo chiesto.
			U->bHasPlannedCoverEdge = false;
			// Lo SCATTO non si azzera qui, ed e' deliberato: `RTTurnManager` lo consuma a ogni risoluzione
			// («consumato per questo turno, valido o no»), quindi un reset in questo punto sarebbe una seconda
			// copia della stessa regola — quella che smette di essere aggiornata quando la prima cambia.
			//
			// Verificato per mutazione: azzerarlo qui non fa cadere alcun test, perche' non c'e' niente da
			// azzerare. E' il modo in cui questa riga, scritta d'istinto insieme alle due sopra, e' stata tolta.
			// E la reazione armata: il turn manager la consuma da solo, ma solo se il trigger scatta. Senza
			// questo azzeramento una reazione mai scattata resterebbe armata per tutto lo scenario, e un
			// turno successivo la vedrebbe partire senza che nessun intent l'abbia chiesta.
			U->PlannedReactionAbility = INDEX_NONE;
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
		// Bersaglio a UNITA': il caso a cella e' il ramo dopo. La condizione porta `!bTargetsCell` perche'
		// altrimenti un intent a cella entrerebbe QUI, non troverebbe l'unita' (`Target` e' vuoto per
		// costruzione) e finirebbe nel ramo del bersaglio abbattuto: una nota al posto di un attacco.
		if (!Intent.Ability.IsNone() && !Intent.bTargetsCell)
		{
			TWeakObjectPtr<ARTUnit>* FoundTarget = UnitsById.Find(Intent.Target);
			ARTUnit* Target = FoundTarget ? FoundTarget->Get() : nullptr;

			const int32 AbilityIndex = FindAbilityIndex(Intent.Ability);

			// Un'azione di Prep agisce su chi la usa, quindi non ha un bersaglio da dichiarare. La domanda si
			// pone al CATALOGO — `MapResolutionPhase` — invece di elencare gli ActionId self: un'azione di Prep
			// aggiunta domani si comporta bene senza che nessuno si ricordi di questa riga.
			const URTActionData* PlannedAbility = Unit->GetAbility(AbilityIndex);
			const bool bResolvesOnSelf = PlannedAbility
				&& URTCatalogLibrary::MapResolutionPhase(PlannedAbility->Def.ResolutionPhase) == ERTMatchPhase::Prep;

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
			else if (bResolvesOnSelf)
			{
				// Azione che risolve su CHI LA USA (`Action.Guard`, `Action.Brace`, e ogni azione di Prep del
				// vertical slice): il `TurnManager` si bersaglia da solo — `Instance.TargetUnitId = i`, e il
				// `PlannedAttackTarget` non lo guarda nemmeno. Pretendere un bersaglio qui sarebbe una regola
				// dell'HARNESS che il gioco non ha, e costringerebbe a scrivere «Bastion si mette in guardia
				// bersagliando se stesso» per ottenere quel che il gioco chiama semplicemente mettersi in guardia.
				Unit->PlannedAbilityIndex = AbilityIndex;
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
		else if (!Intent.Ability.IsNone() && Intent.bTargetsCell)
		{
			// Bersaglio a CELLA: nessun controllo sull'esistenza di un'unita' li' sopra, ed e' il punto —
			// un'area si centra dove si vuole, anche su un varco vuoto. Che poi colpisca qualcuno lo decide
			// il raggio, in fase Blast.
			const int32 AbilityIndex = FindAbilityIndex(Intent.Ability);
			if (AbilityIndex == INDEX_NONE)
			{
				ErroredBy = FString::Printf(TEXT("'%s' non possiede l'abilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Ability.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else
			{
				Unit->PlannedAbilityIndex = AbilityIndex;
				Unit->PlannedAttackTarget = nullptr;
				Unit->PlannedAttackCell = Intent.TargetCell;
				Unit->bAttackTargetsCell = true;

				// Il BORDO, per le azioni che agiscono su una struttura di bordo (CP 9.5). Si scrive solo se
				// lo scenario lo dichiara: senza flag, il resolver rifiuta con `CoverRejected` invece di
				// scegliere un lato — ed e' cio' che deve succedere se il file lo dimentica.
				if (Intent.bHasCoverEdge)
				{
					Unit->PlannedCoverEdge = Intent.CoverEdge;
					Unit->bHasPlannedCoverEdge = true;
				}
			}
		}

		// --- reazione -------------------------------------------------------------------------------------
		// Slot diverso dall'abilita': la stessa unita' puo' attaccare E tenere armata una reazione nello
		// stesso turno. Nessun bersaglio da impostare — chi subira' la reazione lo decide il trigger durante
		// la risoluzione, non lo scenario.
		if (!Intent.Reaction.IsNone())
		{
			for (int32 I = 0; I < Unit->NumAbilities(); ++I)
			{
				const URTActionData* Action = Unit->GetAbility(I);
				if (Action && Action->Def.ActionId == Intent.Reaction)
				{
					Unit->PlannedReactionAbility = I;
					break;
				}
			}
			// Nessun ramo d'errore: che l'eroe possieda la reazione e che sia davvero una reazione lo ha gia'
			// verificato il loader, che rifiuta lo scenario con un motivo invece di lasciarlo girare a vuoto.
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
			// Il TurnLog di QUESTO turno, prima che il prossimo `LockInAndResolve` lo azzeri. E' l'unico
			// istante in cui e' completo e ancora esistente: leggerlo a scenario finito darebbe l'ultimo turno
			// soltanto, e un'assertion su tre turni sarebbe verde per il motivo sbagliato.
			ScenarioLog.Append(TM->GetTurnLog());

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
			// E la reazione armata: il turn manager la consuma da solo, ma solo se il trigger scatta. Senza
			// questo azzeramento una reazione mai scattata resterebbe armata per tutto lo scenario, e un
			// turno successivo la vedrebbe partire senza che nessun intent l'abbia chiesta.
			U->PlannedReactionAbility = INDEX_NONE;
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
		case ERTAssertionKind::UnitFacing:
		{
			static const TCHAR* DirectionNames[6] = { TEXT("E"), TEXT("NE"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("SE") };
			const int32 WantIndex = FMath::Clamp(Exp.Value, 0, 5);
			A.Description = FString::Printf(TEXT("UnitFacing(%s)"), *Exp.UnitId);
			A.Expected = DirectionNames[WantIndex];

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				A.Actual = TEXT("unita' assente");
				A.bPassed = false;
			}
			else
			{
				// Il facing LOGICO, non lo yaw dell'attore: e' il valore che le regole leggono, e l'unico che
				// abbia senso confrontare quando il playback puo' essere ancora a meta' interpolazione.
				const int32 ActualIndex = FMath::Clamp(static_cast<int32>(Unit->Facing), 0, 5);
				A.Actual = DirectionNames[ActualIndex];
				A.bPassed = (ActualIndex == WantIndex);
			}
			break;
		}
		case ERTAssertionKind::LogEventCount:
		{
			const FString EventName = URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome);
			A.Description = FString::Printf(TEXT("LogEventCount(%s)"), *EventName);
			A.Expected = FString::FromInt(Exp.Value);

			int32 Found = 0;
			for (const FRTTurnLogEntry& Entry : ScenarioLog)
			{
				if (Entry.Category == Exp.LogCategory && Entry.Outcome == Exp.LogOutcome) { ++Found; }
			}
			A.Actual = FString::FromInt(Found);
			A.bPassed = (Found == Exp.Value);
			break;
		}
		case ERTAssertionKind::LogEventOrder:
		{
			const FString FirstName = URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome);
			const FString ThenName = URTScenarioLoader::DescribeLogEvent(Exp.ThenCategory, Exp.ThenOutcome);
			A.Description = FString::Printf(TEXT("LogEventOrder(%s prima di %s)"), *FirstName, *ThenName);
			A.Expected = FString::Printf(TEXT("%s prima di %s"), *FirstName, *ThenName);

			const int32 FirstAt = IndexOfScenarioLogEvent(ScenarioLog, Exp.LogCategory, Exp.LogOutcome);
			const int32 ThenAt = IndexOfScenarioLogEvent(ScenarioLog, Exp.ThenCategory, Exp.ThenOutcome);

			// Un evento ASSENTE non e' «fuori ordine»: e' un altro difetto, e dirlo cosi' evita di mandare a
			// cercare un problema di sequenza dove il problema e' che l'evento non e' mai stato prodotto.
			if (FirstAt == INDEX_NONE || ThenAt == INDEX_NONE)
			{
				A.Actual = FString::Printf(TEXT("%s%s%s"),
					FirstAt == INDEX_NONE ? *FString::Printf(TEXT("%s assente"), *FirstName) : TEXT(""),
					(FirstAt == INDEX_NONE && ThenAt == INDEX_NONE) ? TEXT(", ") : TEXT(""),
					ThenAt == INDEX_NONE ? *FString::Printf(TEXT("%s assente"), *ThenName) : TEXT(""));
				A.bPassed = false;
			}
			else
			{
				A.Actual = FString::Printf(TEXT("posizioni %d e %d su %d voci"), FirstAt, ThenAt, ScenarioLog.Num());
				A.bPassed = (FirstAt < ThenAt);
			}
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
