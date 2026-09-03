// **ADR-0008 §2 al decision boundary, in partita** (`#2131`).
//
// I tre test puri di `RTFacingTests.cpp` pinnano la FORMULA — `FacingAt(k) = FacingFromPath(Path[0..k],
// FacingAtMoveStart)` — e sono caratterizzazione dichiarata: `FacingFromPath` esisteva gia', e applicarla a
// un prefisso non cambia niente da sola. Questi due misurano l'altra meta', che e' l'unica che il canone non
// aveva: **chi legge quel valore mentre il ciclo dei micro-step gira**.
//
// 🔴 **Fino al 2026-09-03 i consumatori del boundary leggevano `ARTUnit::Facing`, cioe' l'orientamento
// d'INGRESSO nella fase**: dentro il ciclo l'attore non e' ancora stato ne' spostato ne' riorientato — la
// `RecordFacingChange(DerivedFromMove)` scrive dopo l'uscita. Il commento del sito di fuoco lo dichiarava
// *«non e' un ripiego»*, e la §2 dice il contrario dal 2026-08-10: al boundary vale l'ultimo passo compiuto.
// E' la stessa forma del difetto che `#2142` ha corretto sulle CELLE, un anello piu' in la'.
//
// ⚠️ **Perche' passano dal ciclo VERO** (`LockInAndResolve`) e non da `FacingAtMicroStep` chiamata a mano:
// la funzione pura era verde anche col difetto vivo, perche' il difetto non stava nel calcolo ma in chi lo
// chiamava. E' la lezione di [D-312], la stessa che governa `RTReactionMicroStepCellTests.cpp`.
//
// Prefissi `MsFacing*` negli helper: la unity build fonde i namespace anonimi fra file.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Turn/RTFacingLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeMsFacingWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMsFacingWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnMsFacingMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnMsFacingUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing,
		const URTHeroData* Hero)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		U->Facing = Facing;
		return U;
	}

	/** Arma `Action.Overwatch` come AZIONE PRINCIPALE, che e' cio' che costa (catalogo §1). */
	bool ArmMsFacingOverwatch(ARTUnit* Watcher, int32 SlotIndex = 3)
	{
		const int32 Index = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), SlotIndex);
		if (Index == INDEX_NONE) { return false; }
		Watcher->PlannedAbilityIndex = Index;
		return true;
	}

	/** Un decisore che risponde `FIRE` a qualunque finestra glielo consenta. */
	void BindMsFacingFireDecider(ARTTurnManager* TM)
	{
		TM->ReactionDecider.BindLambda(
			[](const FRTReactionOpportunity& Opportunity, int32 /*OwnerUnitId*/) -> FString
			{
				for (const FString& Response : Opportunity.AllowedResponses)
				{
					if (URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE)
					{
						return Response;
					}
				}
				return FString();
			});
	}

	void RunMsFacingTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** La voce `ReactionDecision` che dichiara un `FIRE`, o `nullptr`. */
	const FRTTurnLogEntry* FindMsFacingFireEntry(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::ReactionDecision) { continue; }
			if (URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) == INDEX_NONE) { continue; }
			return &E;
		}
		return nullptr;
	}

	/** La voce direzionale del colpo subito (`Facing` / `HitCameFromSide`), o `nullptr`. */
	const FRTTurnLogEntry* FindMsFacingHitSideEntry(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Facing
				&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::HitCameFromSide))
			{
				return &E;
			}
		}
		return nullptr;
	}
}

/**
 * **`Facing.FinalPivotIsNotRetroactive`** — ADR-0008 §2, ultimo capoverso: *«il pivot finale si applica dopo
 * l'ultimo micro-step, e non retroattivamente: i boundary che sono gia' passati hanno letto il facing
 * derivato, e nessuna rotazione successiva li rilegge»*.
 *
 * 🔑 **E' la regola che [`D-295`] §(2) dichiarava «pinnata per nome» da un test che non esisteva**, misura
 * del referto `movement-microsteps-facing-pivot-spec-panel-2026-08-31.md` §6.3 e riga corretta il
 * 2026-09-03. Da qui il nome: chi legge il registro deve trovare cio' che il registro promette.
 *
 * Il colpo di Overwatch e' il solo consumatore che risolve DENTRO il ciclo e lascia una traccia leggibile —
 * la voce direzionale di `#2128` — quindi e' l'unico punto in cui «prima» e «dopo» il pivot sono due valori
 * osservabili nella stessa partita.
 *
 * ⚠️ **Senza la rotazione dichiarata il test non direbbe nulla**: il `FIRE` tronca il movimento nella cella
 * raggiunta, quindi il boundary del colpo E' l'ultimo passo e il `FacingFinalAfterMove` deriva dallo stesso
 * passo — i due valori COINCIDEREBBERO, e un'implementazione retroattiva passerebbe. Il pivot e' cio' che li
 * separa, ed e' la ragione per cui il mover e' `Hero.Phase` (`MoveEndPivotMaxSteps = 2`) e non il Wraith.
 *
 * *Mutazione che lo rende rosso*: spostare la `RecordFacingChange(DeclaredInPlanning)` prima del ciclo dei
 * micro-step, o rileggere `Target->Facing` dopo il pivot invece del `TargetBoundaryFacing` del boundary.
 * Cade anche `Spec.Facing.OverwatchHitCameFromSide`, che e' il secondo punto di lettura chiesto dal DoD.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingFinalPivotIsNotRetroactiveTest,
	"RefactorTactics.Facing.FinalPivotIsNotRetroactive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingFinalPivotIsNotRetroactiveTest::RunTest(const FString&)
{
	UWorld* World = MakeMsFacingWorld();
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnMsFacingMap(World, /*Radius=*/ 6);

	// Geometria di `Spec/Overwatch/HoldThenFire.json`: la zona dell'Overwatch e' una LINEA lungo il facing
	// letto in Prep. V guarda a W da (2,-1) e controlla (1,-1) (0,-1) (-1,-1) (-2,-1) — la portata dell'arma
	// del Wraith.
	const FRTCellId WatcherCell(2, -1, 0);
	const FRTCellId MoverStart(-3, 0, 0);
	const FRTCellId MoverEntry(-2, -1, 0);   // ci si entra percorrendo NE: e' la prima cella controllata
	const FRTCellId MoverBeyond(-1, -1, 0);  // il secondo passo, che il FIRE impedisce

	ARTUnit* Watcher = SpawnMsFacingUnit(World, /*Team*/ 1, WatcherCell, ERTHexDirection::W,
		URTHeroCatalogLibrary::MakeWraith());
	// `Hero.Phase` per il BUDGET: ADR-0008 §1 gli da' `MoveEndPivotMaxSteps = 2`, cioe' esattamente i due
	// step che separano `NE` da `W`. Con un budget minore la dichiarazione sarebbe RIFIUTATA e il test
	// misurerebbe il rifiuto invece della non retroattivita'.
	ARTUnit* Mover = SpawnMsFacingUnit(World, /*Team*/ 0, MoverStart, ERTHexDirection::W,
		URTHeroCatalogLibrary::MakePhase());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestTrue(TEXT("la scena si monta"), MapActor && MapActor->MapAsset && Watcher && Mover && TM))
	{
		DestroyMsFacingWorld(World);
		return false;
	}

	Mover->PlannedPath = { MoverStart, MoverEntry, MoverBeyond };
	Mover->PlannedCell = MoverBeyond;
	Mover->bDeclaresPlannedFacing = true;
	Mover->PlannedFacing = ERTHexDirection::W;

	if (!TestTrue(TEXT("l'Overwatch si arma"), ArmMsFacingOverwatch(Watcher)))
	{
		DestroyMsFacingWorld(World);
		return false;
	}
	BindMsFacingFireDecider(TM);
	RunMsFacingTurn(TM);

	// --- PREMESSE: il colpo e' avvenuto dentro il ciclo, e ha troncato il movimento ---------------------
	TestNotNull(TEXT("una finestra si e' aperta e il watcher ha sparato"), FindMsFacingFireEntry(TM));
	TestTrue(TEXT("il movimento e' troncato nella cella d'ingresso"), Mover->Cell == MoverEntry);

	// --- LA MISURA -------------------------------------------------------------------------------------
	//
	// Il lato si legge sul facing del MICRO-STEP: il mover ha percorso `NE`, il watcher sta nello spicchio
	// assoluto `E` (indice 0) rispetto alla cella raggiunta, quindi l'indice relativo e' (0-1+6)%6 = 5.
	const FRTTurnLogEntry* Side = FindMsFacingHitSideEntry(TM);
	if (!TestNotNull(TEXT("il colpo ha lasciato la sua voce direzionale"), Side))
	{
		DestroyMsFacingWorld(World);
		return false;
	}
	TestEqual(TEXT("il lato e' quello del passo appena compiuto (FrontRight)"), Side->Amount,
		static_cast<int32>(ERTRelativeDirection::FrontRight));

	// Il pivot E' avvenuto, e ha portato il facing altrove: senza questa riga il confronto sopra sarebbe
	// vero anche in un mondo in cui nessuna rotazione e' stata applicata.
	TestTrue(TEXT("il pivot dichiarato e' stato applicato"), Mover->Facing == ERTHexDirection::W);

	// 🔴 **E il valore post-pivot NON e' quello registrato.** Con facing `W` (indice 3) lo stesso colpo
	// darebbe `Rear` (3): e' il numero che la voce porterebbe se il pivot fosse retroattivo. La riga esiste
	// perche' «5 e' giusto» e «3 e' sbagliato» sono due affermazioni diverse, e la seconda e' quella che il
	// test deve fare.
	TestTrue(TEXT("e non e' il lato che il facing FINALE darebbe"),
		Side->Amount != static_cast<int32>(ERTRelativeDirection::Rear));

	DestroyMsFacingWorld(World);
	return true;
}

/**
 * **`Overwatch.TriggerReadsMicroStepFacing`** — ADR-0008 §Verifica: *«il trigger valuta l'arco sul facing
 * del boundary, non su quello iniziale ne' su quello finale»*.
 *
 * 🔑 **Quale arco, e dove entra nel trigger.** Non il cono del watcher: `FRTArmedOverwatch::Facing` e' il
 * facing DICHIARATO in Planning e il suo docstring vieta esplicitamente di rileggerlo a ogni micro-step —
 * *«lo renderebbe una direzione che cambia dopo l'impegno»*. L'arco che il trigger valuta a ogni boundary e'
 * quello degli OSSERVATORI della squadra del watcher: `ResolveReactionBoundary` costruisce i `FRTPerceiver`
 * dello schieramento e ne ricava `TeamAwareness`, che e' la **seconda condizione di trigger** di ADR-0004 §6
 * (*«`TargetDetected` — livello Rilevato e non visibile»*). Un osservatore che sta CAMMINANDO guarda dove ha
 * appena messo il piede, e questo cambia chi la squadra vede.
 *
 * ⚠️ **La misura corregge una lettura precedente.** Lo spec panel del 2026-09-03 aveva concluso che questo
 * test *«non ha un soggetto»*, misurando `facing` in `RTReactionOpportunityTypes.cpp` (0 occorrenze) e i
 * chiamanti non-test di `ReadFacingForConsumer` (0). Entrambe le misure sono esatte e il file era il file
 * sbagliato: il facing entra nel trigger **dal chiamante**, come `TeamAwareness` gia' calcolata, non dentro
 * il costruttore delle opportunity. `#1933` resta confinante — registrare cio' che il trigger ha letto e'
 * un'altra cosa dal leggerlo — e non e' bloccante.
 *
 * 🔑 **Due scene, stessa cella finale, esito opposto.** L'alleato osservatore finisce in (3,3) in entrambe:
 * nella prima ci arriva camminando, nella seconda ci era gia'. Cambia solo il facing che il boundary legge,
 * e con esso il fatto che il bersaglio sia `Detected`. Un'asserzione sulla sola scena positiva sarebbe
 * soddisfatta da un trigger che spara sempre.
 *
 * *Mutazione che lo rende rosso*: rimettere `P.Facing = Units[u]->Facing` in `ResolveReactionBoundary`. La
 * prima scena perde il fuoco, perche' l'alleato tornerebbe a guardare `E`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchTriggerReadsMicroStepFacingTest,
	"RefactorTactics.Overwatch.TriggerReadsMicroStepFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchTriggerReadsMicroStepFacingTest::RunTest(const FString&)
{
	const FRTCellId WatcherCell(0, 0, 0);
	const FRTCellId TargetStart(3, 1, 0);
	const FRTCellId TargetEntry(3, 0, 0);  // terza cella della linea guardata, a 3 da (0,0)
	const FRTCellId AllyStart(3, 4, 0);
	const FRTCellId AllySeat(3, 3, 0);     // dove l'alleato ARRIVA nella prima scena, e DOVE STA nella seconda

	// `bAllyWalks` e' l'unica variabile fra le due scene.
	auto RunScene = [&](bool bAllyWalks, ARTUnit*& OutTarget, ARTTurnManager*& OutTM, UWorld*& OutWorld) -> bool
	{
		OutWorld = MakeMsFacingWorld();
		if (!OutWorld) { return false; }
		// Raggio 8: (3,4) dista 7 dall'origine, e un'arena piu' stretta lo lascerebbe fuori mappa.
		ARTHexMapActor* MapActor = SpawnMsFacingMap(OutWorld, /*Radius=*/ 8);
		if (!MapActor || !MapActor->MapAsset) { return false; }

		ARTUnit* Watcher = SpawnMsFacingUnit(OutWorld, /*Team*/ 1, WatcherCell, ERTHexDirection::E,
			URTHeroCatalogLibrary::MakeWraith());
		ARTUnit* Ally = SpawnMsFacingUnit(OutWorld, /*Team*/ 1, bAllyWalks ? AllyStart : AllySeat,
			ERTHexDirection::E, URTHeroCatalogLibrary::MakeWraith());
		OutTarget = SpawnMsFacingUnit(OutWorld, /*Team*/ 0, TargetStart, ERTHexDirection::W,
			URTHeroCatalogLibrary::MakeWraith());
		OutTM = OutWorld->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Watcher || !Ally || !OutTarget || !OutTM) { return false; }

		// 🔴 **La vista del watcher si accorcia a 1, ed e' il perno della scena.** Il cono dell'Overwatch E'
		// il facing (ADR-0005 §4c), quindi un watcher con la vista di serie vedrebbe da solo tutta la propria
		// linea e `TeamAwareness` sarebbe `Detected` comunque: l'arco dell'alleato non discriminerebbe nulla.
		// `Reach` resta `max(VisionRange, CloseAwarenessRange) = 2`, e il bersaglio entra a distanza 3.
		Watcher->VisionRange = 1;

		if (bAllyWalks)
		{
			// (3,4) -> (3,3) e' un passo `NW`: al boundary l'alleato guarda lungo la colonna e (3,0) gli
			// cade davanti a distanza 3. Con il facing di PIAZZAMENTO (`E`) non ci cadrebbe.
			Ally->PlannedPath = { AllyStart, AllySeat };
			Ally->PlannedCell = AllySeat;
		}

		OutTarget->PlannedPath = { TargetStart, TargetEntry };
		OutTarget->PlannedCell = TargetEntry;

		if (!ArmMsFacingOverwatch(Watcher)) { return false; }
		BindMsFacingFireDecider(OutTM);
		RunMsFacingTurn(OutTM);
		return true;
	};

	// --- SCENA 1: l'alleato cammina, e girandosi porta il bersaglio nella conoscenza di squadra ----------
	{
		ARTUnit* Target = nullptr; ARTTurnManager* TM = nullptr; UWorld* World = nullptr;
		const bool bBuilt = RunScene(/*bAllyWalks=*/ true, Target, TM, World);
		if (!TestTrue(TEXT("la scena 'alleato in cammino' si monta"), bBuilt))
		{
			DestroyMsFacingWorld(World);
			return false;
		}
		TestTrue(TEXT("il bersaglio e' entrato nella cella controllata"), Target->Cell == TargetEntry);
		TestNotNull(TEXT("il trigger si e' aperto e il watcher ha sparato"), FindMsFacingFireEntry(TM));
		TestTrue(TEXT("e il bersaglio ha incassato"), Target->Health < Target->MaxHealth);
		DestroyMsFacingWorld(World);
	}

	// --- SCENA 2: stessa cella, stesso facing dichiarato, ma l'alleato non ha camminato ------------------
	{
		ARTUnit* Target = nullptr; ARTTurnManager* TM = nullptr; UWorld* World = nullptr;
		const bool bBuilt = RunScene(/*bAllyWalks=*/ false, Target, TM, World);
		if (!TestTrue(TEXT("la scena 'alleato fermo' si monta"), bBuilt))
		{
			DestroyMsFacingWorld(World);
			return false;
		}
		TestTrue(TEXT("il bersaglio e' entrato nella stessa cella controllata"), Target->Cell == TargetEntry);
		TestNull(TEXT("nessun trigger: la squadra non lo vede"), FindMsFacingFireEntry(TM));
		TestTrue(TEXT("e il bersaglio e' intatto"), Target->Health == Target->MaxHealth);
		DestroyMsFacingWorld(World);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
