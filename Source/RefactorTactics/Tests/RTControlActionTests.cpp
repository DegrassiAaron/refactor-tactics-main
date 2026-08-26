#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Le cinque azioni di controllo (catalogo v0.1 §5): CP 4.7. Push/Root/Slow passano dalla stessa pipeline
 * di ResolveCombat che gia' applica gli effetti di Guardian.Sweep/Ranger.Burst; Interrupt e' l'eccezione,
 * cancella l'intera azione di un'altra unita' filtrando i colpi raccolti prima che diventino danno o eventi.
 */
namespace
{
	/**
	 * Una spinta di DUE celle, dichiarata qui e non pescata dal catalogo.
	 *
	 * Serviva `Guardian.Sweep`, l'unica azione del progetto con `Push 2`, ed e' sparita con gli archetipi
	 * legacy il 2026-08-10: il roster v0.1 arriva a `Push 1`. Ma la regola sotto esame e' del RESOLVER —
	 * cosa attraversa un bersaglio spinto oltre una cella, e cosa succede se quella intermedia brucia — e
	 * con una spinta di una cella sola non esisterebbe **nessuna cella intermedia** da attraversare.
	 *
	 * Dichiararla nel test e' piu' onesto che tenere in vita un'azione di gioco per sostenere una verifica:
	 * il catalogo dice cosa il gioco spedisce, questo dice cosa il motore deve saper fare.
	 */
	FRTActionDef MakePush2Def()
	{
		FRTActionDef Def;
		Def.ActionId = TEXT("Test.Push2");
		Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Def.Priority = 55;
		Def.RangeCells = 3;
		Def.CostMP = 0;
		Def.CooldownTurns = 0;
		Def.Fallback = ERTActionFallback::AttackCell;
		Def.bCanBeInterrupted = true;
		Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 30));
		Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Push, 2));
		return Def;
	}

	UWorld* MakeControlWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyControlWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnControlMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnControlUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith()); // stats/portata base qualunque: i test guardano il controllo
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Aggiunge un'azione di CONTROLLO generica all'unita' (dal catalogo core) e ne restituisce l'indice.
	 *
	 * `Power` va azzerato esplicitamente: `URTActionData::Power` ha un default legacy di **30** (eredita'
	 * dell'MVP quadrato, prima del catalogo), e `ResolveCombat` vi ricade quando l'azione non dichiara un
	 * effetto Damage (`DeclaredDamage>0 ? DeclaredDamage : Ability->Power`). Senza questa riga un'azione di
	 * controllo senza danno infliggerebbe comunque 30 danni fantasma — e' la stessa disciplina che
	 * `MakeHeroAction` (Ability/RTHeroCatalogLibrary.cpp, CP 6.2) gia' applica per gli eroi.
	 */
	int32 AddControlAbility(ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Ability = NewObject<URTActionData>(Unit);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Ability->RangeCells = Ability->Def.RangeCells; // specchio legacy: alcuni percorsi lo leggono ancora
		Ability->Power = 0;
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Ability->Power = Spec.Amount; break; }
		}
		Unit->Abilities.Add(Ability);
		return Unit->Abilities.Num() - 1;
	}

	void RunControlTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlActionsMatchCatalogTest,
	"RefactorTactics.Actions.ControlActionsMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlActionsMatchCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §5 del catalogo azioni v0.1. La tabella non aveva colonna Range (unica sezione del
	// catalogo senza): il numero e' stato deciso in CP 4.7, 1 per quattro azioni su cinque. Pull e' l'unica
	// eccezione (2): a range 1 il bersaglio, gia' adiacente, finirebbe sempre sulla cella di chi tira —
	// sempre occupata — e la trazione si annullerebbe per costruzione, in ogni caso.
	struct FExpected { const TCHAR* Id; int32 Priority; int32 Cooldown; int32 Range; };
	const FExpected Expected[] = {
		{ TEXT("Action.Interrupt"), 20, 2, 1 },
		{ TEXT("Action.Root"),      25, 2, 1 },
		{ TEXT("Action.Push"),      40, 1, 1 },
		{ TEXT("Action.Pull"),      40, 1, 2 },
		{ TEXT("Action.Slow"),      50, 1, 1 },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(E.Id));
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id))) { continue; }
		TestEqual(FString::Printf(TEXT("%s: priorita'"), E.Id), Def.Priority, E.Priority);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestEqual(FString::Printf(TEXT("%s: portata"), E.Id), Def.RangeCells, E.Range);
		TestTrue(FString::Printf(TEXT("%s: risolve nel Blast"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Blast);
	}

	// La priorita' e' cio' che mette il controllo PRIMA del danno: Interrupt e Root risolvono prima della
	// piu' bassa offensiva (MarkTarget, 40), Push/Pull la eguagliano, Slow la segue di poco.
	TestTrue(TEXT("Interrupt precede ogni offensiva"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Interrupt")).Priority
		< URTCatalogLibrary::FindCoreAction(TEXT("Action.MarkTarget")).Priority);
	TestTrue(TEXT("Root precede ogni offensiva"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Root")).Priority
		< URTCatalogLibrary::FindCoreAction(TEXT("Action.MarkTarget")).Priority);

	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("le azioni generiche restano valide"), Errors.Num(), 0);
	return true;
}

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushInvalidDestinationTest,
	"RefactorTactics.Actions.Push.InvalidDestination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTPushInvalidDestinationTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 §5. "Destinazione bloccata = spostamento annullato": se la prima
	// cella nella direzione della spinta e' un ostacolo o un'unita', il bersaglio NON si sposta affatto —
	// non e' un errore, e' l'esito dichiarato.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	// Muro subito dietro il bersaglio, sulla linea di spinta.
	FRTHexCellData Wall(FRTCellId(2, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	const TArray<FRTCellId> NoOccupants;
	const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, 0), /*Distance*/ 1, Map, NoOccupants);
	TestTrue(TEXT("destinazione bloccata: nessuno spostamento"), Dest == FRTCellId(1, 0));

	// Stesso esito se la cella e' occupata da un'unita' invece che da un muro.
	TArray<FRTCellId> Occupied;
	Occupied.Add(FRTCellId(2, 0));
	const FRTCellId DestOccupied = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, 0), 1, Map, Occupied);
	TestTrue(TEXT("cella occupata: nessuno spostamento"), DestOccupied == FRTCellId(1, 0));

	// In partita: `Action.Push` contro un bersaglio senza via di fuga non produce un crash ne' una posizione
	// invalida — il bersaglio resta esattamente dov'era.
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 4);
	// Sovrascrive la mappa spawnata con quella che ha il muro: stesso layout di sopra.
	ARTHexMapActor* HexMap = Cast<ARTHexMapActor>(UGameplayStatics::GetActorOfClass(World, ARTHexMapActor::StaticClass()));
	if (HexMap) { HexMap->MapAsset = Map; }

	ARTUnit* Pusher = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnControlUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("Victim"), Victim) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 PushIdx = AddControlAbility(Pusher, TEXT("Action.Push"));
	Pusher->PlannedAbilityIndex = PushIdx;
	Pusher->PlannedAttackTarget = Victim;
	Victim->PlannedAbilityIndex = INDEX_NONE;

	RunControlTurn(TM);

	TestTrue(TEXT("il bersaglio resta dov'era: destinazione bloccata"), Victim->Cell == FRTCellId(1, 0));
	DestroyControlWorld(World);
	return true;
}

/**
 * `PushResistance` e' una **soglia**: regge le spinte fino al proprio valore, cede a quelle piu' forti — e
 * quando cede la spinta passa **intera**, non ridotta. E' la forma di `Action.Guard` (CP 5.2), non una
 * sottrazione: due resistenze alla spinta con due semantiche diverse nello stesso combat sarebbero state la
 * prima cosa da spiegare a chi bilancia (#241, D-038).
 *
 * Il caso che nessuno scenario puo' esprimere e' proprio quello che separa le due semantiche: **`Push 2`**
 * esiste solo su `Guardian.Sweep`, un'azione d'archetipo fuori dal roster v0.1. Con la sola spinta da 1 dei
 * kit, soglia e sottrazione darebbero lo stesso risultato e questo test sarebbe verde comunque.
 *
 * `Action.Pull` **non** e' resistito, ed e' deliberato: il catalogo riserva la resistenza di Guard alla
 * spinta. Verificarlo qui impedisce che qualcuno lo "aggiusti" per simmetria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushResistanceIsAThresholdTest,
	"RefactorTactics.Actions.PushResistanceIsAThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushResistanceIsAThresholdTest::RunTest(const FString&)
{
	struct FCase
	{
		const TCHAR* ActionId;   // azione che produce la spinta
		int32 Resistance;        // resistenza del bersaglio
		int32 VictimQ;           // distanza del bersaglio sulla riga r=0
		bool bShouldMove;        // ci si aspetta che si sposti?
		const TCHAR* What;
	};
	// La trazione vuole il bersaglio a DUE celle: tirato da una sola, la cella verso cui andrebbe e' quella
	// del puller ed e' occupata — non si sposterebbe per geografia invece che per resistenza, e il caso
	// direbbe il contrario di quel che sembra.
	static const FCase Cases[] = {
		{ TEXT("Action.Push"),      1, 1, false, TEXT("Push 1 contro resistenza 1: assorbita") },
		{ TEXT("Action.Push"),      0, 1, true,  TEXT("Push 1 contro resistenza 0: arretra") },
		{ TEXT("Guardian.Sweep"),   1, 1, true,  TEXT("Push 2 contro resistenza 1: la soglia cede, e cede INTERA") },
		{ TEXT("Action.Pull"),      1, 2, true,  TEXT("Pull 1 contro resistenza 1: la trazione non e' resistita") },
	};

	for (const FCase& C : Cases)
	{
		UWorld* World = MakeControlWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnControlMap(World, 4);

		ARTUnit* Mover = SpawnControlUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Victim = SpawnControlUnit(World, 1, FRTCellId(C.VictimQ, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Mover || !Victim || !TM) { DestroyControlWorld(World); return false; }

		Victim->PushResistance = C.Resistance;
		const FRTCellId Before = Victim->Cell;

		// I casi con `Push 2` non hanno un'azione di gioco che li produca: il roster arriva a `Push 1`, e
		// `Guardian.Sweep` e' sparita con gli archetipi legacy. La definizione la dichiara il test.
		URTActionData* Ability = NewObject<URTActionData>(Mover);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(C.ActionId));
		if (Ability->Def.ActionId.IsNone())
		{
			Ability->Def = MakePush2Def();
		}
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		Mover->Abilities.Add(Ability);

		Mover->PlannedAbilityIndex = Mover->Abilities.Num() - 1;
		Mover->PlannedAttackTarget = Victim;
		Victim->PlannedAbilityIndex = INDEX_NONE;

		RunControlTurn(TM);

		const bool bMoved = !(Victim->Cell == Before);
		TestEqual(C.What, bMoved, C.bShouldMove);
		DestroyControlWorld(World);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPullTest,
	"RefactorTactics.Actions.Pull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPullTest::RunTest(const FString&)
{
	// Simmetrica a Push: la funzione pura, poi la stessa cosa in partita.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	const TArray<FRTCellId> NoOccupants;

	// Puller in (0,0), bersaglio in (2,0): tirato di 1 cella si avvicina a (1,0), non finisce mai su (0,0).
	const FRTCellId Dest = URTHexCombatLibrary::HexPullDestination(FRTCellId(0, 0), FRTCellId(2, 0), 1, Map, NoOccupants);
	TestTrue(TEXT("si avvicina di una cella"), Dest == FRTCellId(1, 0));

	// La cella del puller e' SEMPRE fra le occupate (ci sta un'unita' viva): la trazione non finisce mai li'.
	TArray<FRTCellId> PullerOccupies;
	PullerOccupies.Add(FRTCellId(0, 0));
	const FRTCellId DestNextToPuller =
		URTHexCombatLibrary::HexPullDestination(FRTCellId(0, 0), FRTCellId(1, 0), 1, Map, PullerOccupies);
	TestTrue(TEXT("non finisce mai addosso a chi tira"), DestNextToPuller == FRTCellId(1, 0));

	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 4);
	ARTUnit* Puller = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnControlUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Puller"), Puller) || !TestNotNull(TEXT("Victim"), Victim) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 PullIdx = AddControlAbility(Puller, TEXT("Action.Pull"));
	Puller->PlannedAbilityIndex = PullIdx;
	Puller->PlannedAttackTarget = Victim;

	RunControlTurn(TM);

	TestTrue(TEXT("in partita: il bersaglio si avvicina di 1"), Victim->Cell == FRTCellId(1, 0));
	DestroyControlWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRootCancelsRemainingStepsTest,
	"RefactorTactics.Actions.Root.CancelsRemainingSteps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRootCancelsRemainingStepsTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il bersaglio ha gia' un percorso composito pianificato (waypoint) quando
	// viene radicato NELLO STESSO turno: il Root risolve nel Blast, il Move segue — i micro-step non ancora
	// risolti si cancellano, non si eseguono col piano di prima del radicamento.
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	// Adiacente alla posizione INIZIALE del bersaglio: Root ha portata 1, e per il targeting conta la cella
	// prima del Move (che risolve dopo il Blast) — il bersaglio non si e' ancora mosso quando il colpo parte.
	ARTUnit* Rooter = SpawnControlUnit(World, 0, FRTCellId(-1, 0));
	ARTUnit* Mover = SpawnControlUnit(World, 1, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Rooter"), Rooter) || !TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 RootIdx = AddControlAbility(Rooter, TEXT("Action.Root"));
	Rooter->PlannedAbilityIndex = RootIdx;
	Rooter->PlannedAttackTarget = Mover;
	Rooter->PlannedCell = Rooter->Cell; // fermo: solo il Root

	// Mover pianifica un percorso a waypoint di 3 celle, come farebbe un giocatore che clicca la destinazione.
	Mover->PlannedWaypoints = { FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) };
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) };
	Mover->PlannedCell = FRTCellId(3, 0);
	Mover->PlannedAbilityIndex = INDEX_NONE;

	RunControlTurn(TM);

	TestTrue(TEXT("il radicato non si muove affatto"), Mover->Cell == FRTCellId(0, 0));
	// Non si controlla `HasStatus` DOPO il turno: `Status.Root` dura 1 turno, e a questo punto
	// `TickStatuses` (chiamato a fine risoluzione) lo ha gia' scaduto — correttamente. L'esito
	// OSSERVABILE (il radicato non si e' mosso, sopra) e' cio' che il meccanismo doveva produrre.
	DestroyControlWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRootAllowsAttackTest,
	"RefactorTactics.Actions.Root.AllowsAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRootAllowsAttackTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Radicato NON vuol dire disarmato: il colpo pianificato dello stesso turno in
	// cui si viene radicati risolve normalmente — Root passa dal budget di movimento, non tocca gli attacchi.
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Rooter = SpawnControlUnit(World, 0, FRTCellId(-1, 0)); // adiacente: Root ha portata 1
	ARTUnit* RootedAttacker = SpawnControlUnit(World, 1, FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnControlUnit(World, 0, FRTCellId(1, 0)); // bersaglio del colpo del radicato
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Rooter"), Rooter) || !TestNotNull(TEXT("RootedAttacker"), RootedAttacker)
		|| !TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 RootIdx = AddControlAbility(Rooter, TEXT("Action.Root"));
	Rooter->PlannedAbilityIndex = RootIdx;
	Rooter->PlannedAttackTarget = RootedAttacker;
	Rooter->PlannedCell = Rooter->Cell;

	const int32 HealthBefore = Enemy->Health;
	RootedAttacker->PlannedAbilityIndex = 0; // attacco base dell'archetipo
	RootedAttacker->PlannedAttackTarget = Enemy;
	RootedAttacker->PlannedCell = RootedAttacker->Cell; // niente movimento: solo l'attacco

	RunControlTurn(TM);

	// Non si controlla `HasStatus` dopo il turno per lo stesso motivo del test precedente: `Status.Root`
	// dura 1 turno ed e' gia' scaduto a questo punto. L'esito che conta e' che il colpo sia arrivato.
	TestTrue(TEXT("il colpo del radicato e' arrivato comunque"), Enemy->Health < HealthBefore);
	DestroyControlWorld(World);
	return true;
}

/**
 * **Una cura che non avviene lascia una traccia.**
 *
 * `Action.Heal` ha portata 3. Oltre, il Blast saltava la cura scrivendo una riga di combat log e basta: il
 * record autoritativo non conteneva NIENTE, quindi un replay non poteva riprodurre il turno e un rapporto
 * di divergenza non poteva spiegare perche' l'alleato fosse rimasto ferito. E' l'asimmetria INVERSA di
 * `#1412` — non una riga di troppo, una che non c'e' — chiusa con [D-196].
 *
 * ⚠️ Il test sta fra i test di CONTROLLO e non fra quelli di cura perche' qui si misura la TRACCIA, non la
 * regola di portata: quella e' gia' pinnata altrove, e duplicarla sposterebbe il punto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHealOutOfRangeIsTracedTest,
	"RefactorTactics.Actions.Heal.OutOfRangeIsTraced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHealOutOfRangeIsTracedTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 8);

	ARTUnit* Curatore = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Ferito = SpawnControlUnit(World, 0, FRTCellId(6, 0)); // ben oltre la portata 3 di Action.Heal
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Curatore"), Curatore) || !TestNotNull(TEXT("Ferito"), Ferito)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 HealIdx = AddControlAbility(Curatore, TEXT("Action.Heal"));
	if (!TestTrue(TEXT("premessa: la cura e' oltre la portata dichiarata"),
		URTHexLibrary::HexDistance(Curatore->Cell, Ferito->Cell)
			> Curatore->Abilities[HealIdx]->Def.RangeCells))
	{
		DestroyControlWorld(World);
		return false;
	}

	Ferito->Health = FMath::Max(1, Ferito->Health - 30);
	const int32 SaluteFerito = Ferito->Health;

	Curatore->PlannedAbilityIndex = HealIdx;
	Curatore->PlannedAttackTarget = Ferito;
	Curatore->PlannedCell = Curatore->Cell;

	RunControlTurn(TM);

	TestEqual(TEXT("premessa: la cura non e' avvenuta"), Ferito->Health, SaluteFerito);

	const FRTTurnLogEntry* Mancata = TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange)
			&& E.ActionId == FName(TEXT("Action.Heal"));
	});
	if (TestNotNull(TEXT("la cura mancata e' nel record autoritativo"), Mancata))
	{
		TestEqual(TEXT("accredita chi ha provato a curare"), Mancata->UnitId, Curatore->StableUnitId);
		TestEqual(TEXT("e dice chi doveva essere curato"), Mancata->TgtCell, Ferito->Cell);
	}

	DestroyControlWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptOnlyInterruptibleTest,
	"RefactorTactics.Actions.Interrupt.OnlyInterruptible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptOnlyInterruptibleTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Interrupt cancella un attacco interrompibile per intero (non solo i suoi
	// effetti collaterali: anche il danno, che oggi si calcola PRIMA di sapere se l'azione sara' interrotta —
	// per questo si filtra sui colpi raccolti, non sul singolo evento). Su un'azione che dichiara
	// `bCanBeInterrupted = false` (Guard), Interrupt non ha nulla da cancellare.
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	// ⚠️ NESSUNO sull'origine: `FRTCellId()` di default e' `(0,0,0)`, che e' una cella VERA — un'asserzione
	// su una cella che coincide col default passerebbe anche se il produttore non la assegnasse affatto.
	//
	// ⚠️ E le distanze restano di UNA cella: `Action.Interrupt` ha portata 1 (catalogo). La prima versione
	// di questo spostamento le aveva messe a due, e il test cadeva sull'interruzione che non avveniva — un
	// rosso che sembrava un difetto della traccia ed era la premessa rotta.
	ARTUnit* Interrupter = SpawnControlUnit(World, 0, FRTCellId(3, 0));
	ARTUnit* Attacker = SpawnControlUnit(World, 1, FRTCellId(4, 0));
	ARTUnit* Victim = SpawnControlUnit(World, 0, FRTCellId(5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Interrupter"), Interrupter) || !TestNotNull(TEXT("Attacker"), Attacker)
		|| !TestNotNull(TEXT("Victim"), Victim) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 InterruptIdx = AddControlAbility(Interrupter, TEXT("Action.Interrupt"));
	TestTrue(TEXT("premessa: l'interruttore e' in portata di chi attacca"),
		URTHexLibrary::HexDistance(Interrupter->Cell, Attacker->Cell)
			<= Interrupter->Abilities[InterruptIdx]->Def.RangeCells);
	Interrupter->PlannedAbilityIndex = InterruptIdx;
	Interrupter->PlannedAttackTarget = Attacker;
	Interrupter->PlannedCell = Interrupter->Cell;

	const int32 HealthBefore = Victim->Health;
	Attacker->PlannedAbilityIndex = 0; // attacco base (Ranger.Shot): bCanBeInterrupted = true
	Attacker->PlannedAttackTarget = Victim;
	Attacker->PlannedCell = Attacker->Cell;

	RunControlTurn(TM);

	TestEqual(TEXT("l'attacco interrotto non fa danno"), Victim->Health, HealthBefore);

	// L'abilita' interrotta NON si consuma: e' come se non fosse mai partita, non come se avesse fallito.
	TestFalse(TEXT("il cooldown dell'attacco base non e' scattato"), Attacker->GetAbilityCooldown(0) > 0);

	// 🔴 **E l'interruzione esiste nel record autoritativo** ([D-196], `#1412` punto 4). Fino al 2026-08-26
	// viveva SOLO nel combat log: il piano dell'attaccante spariva dal turno e nessuna traccia diceva
	// perche', quindi un replay non poteva riprodurlo ne' un rapporto di divergenza spiegarlo.
	const FRTTurnLogEntry* Interrotta = TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
	});
	if (TestNotNull(TEXT("il TurnLog registra l'interruzione"), Interrotta))
	{
		// Il soggetto e' chi SUBISCE l'interruzione: e' la sua azione a essere annullata (`#1418`).
		TestEqual(TEXT("accredita chi e' stato interrotto"), Interrotta->UnitId, Attacker->StableUnitId);

		// `SrcCell` -> `TgtCell` e' «da dove a dove puntava l'azione cancellata», come in ogni altra voce
		// `Fallback`. Non la cella di chi ha interrotto: `DescribeEntry` rende tutte le voci della famiglia
		// con lo stesso «src -> tgt», e metterci l'interruttore faceva leggere «la vittima attaccava
		// l'interruttore» — preciso e falso.
		TestEqual(TEXT("parte da chi e' stato interrotto"), Interrotta->SrcCell, Attacker->Cell);
		TestEqual(TEXT("e punta dove puntava l'azione cancellata"), Interrotta->TgtCell, Victim->Cell);
		TestNotEqual(TEXT("non la cella di chi ha interrotto"), Interrotta->TgtCell, Interrupter->Cell);

		TestFalse(TEXT("e dice QUALE azione e' stata cancellata"), Interrotta->ActionId.IsNone());
	}

	DestroyControlWorld(World);
	return true;
}

/**
 * **Due interruttori sulla stessa vittima cancellano UNA azione, e la traccia lo dice una volta.**
 *
 * Banale in 2v2, e prima di `#1437` la traccia ne portava due: l'effetto di gioco era deduplicato da un
 * `TSet` di unita', ma la voce di TurnLog veniva scritta una volta per **colpo** di Interrupt. Il record
 * autoritativo diceva che l'unica azione della vittima era stata cancellata due volte — la riga doppia che
 * `#1412` esiste per togliere.
 *
 * Ora si tiene traccia degli INTENTI cancellati, non delle unita': due Interrupt aggiungono lo stesso
 * indice al set e la seconda passata non scrive niente. La deduplicazione non e' una guardia in piu', e'
 * una conseguenza di aver scelto la chiave giusta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptTwiceTracesOnceTest,
	"RefactorTactics.Actions.Interrupt.TwoInterruptersTraceOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptTwiceTracesOnceTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	// Due interruttori adiacenti alla vittima: `Action.Interrupt` ha portata 1.
	ARTUnit* PrimoInterrupter = SpawnControlUnit(World, 0, FRTCellId(3, 0));
	ARTUnit* SecondoInterrupter = SpawnControlUnit(World, 0, FRTCellId(4, -1));
	ARTUnit* Attacker = SpawnControlUnit(World, 1, FRTCellId(4, 0));
	ARTUnit* Victim = SpawnControlUnit(World, 0, FRTCellId(5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("primo interrupter"), PrimoInterrupter)
		|| !TestNotNull(TEXT("secondo interrupter"), SecondoInterrupter)
		|| !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	for (ARTUnit* Interrupter : { PrimoInterrupter, SecondoInterrupter })
	{
		const int32 Idx = AddControlAbility(Interrupter, TEXT("Action.Interrupt"));
		TestTrue(TEXT("premessa: l'interruttore e' in portata di chi attacca"),
			URTHexLibrary::HexDistance(Interrupter->Cell, Attacker->Cell)
				<= Interrupter->Abilities[Idx]->Def.RangeCells);
		Interrupter->PlannedAbilityIndex = Idx;
		Interrupter->PlannedAttackTarget = Attacker;
		Interrupter->PlannedCell = Interrupter->Cell;
	}

	const int32 HealthBefore = Victim->Health;
	Attacker->PlannedAbilityIndex = 0; // attacco base, interrompibile
	Attacker->PlannedAttackTarget = Victim;
	Attacker->PlannedCell = Attacker->Cell;

	RunControlTurn(TM);

	TestEqual(TEXT("l'attacco interrotto non fa danno"), Victim->Health, HealthBefore);

	int32 Voci = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted))
		{
			++Voci;
		}
	}
	TestEqual(TEXT("una azione cancellata, una voce"), Voci, 1);

	DestroyControlWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptSkipsNonInterruptibleTest,
	"RefactorTactics.Actions.Interrupt.SkipsNonInterruptible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptSkipsNonInterruptibleTest::RunTest(const FString&)
{
	// Meta' mancante del test precedente: Guard dichiara `bCanBeInterrupted = false` (CP 4.4). Un Interrupt
	// contro chi si sta mettendo in guardia non ha nulla da cancellare — la guardia vale comunque.
	//
	// Non si verifica con `HasStatus` DOPO il turno (scadrebbe comunque, come nei test di Root sopra): si
	// aggiunge un terzo attaccante che colpisce il Guarder NELLO STESSO turno, e si osserva la riduzione di
	// 15 sul primo danno diretto (`URTCombatLibrary::GuardFirstHitReduction`) — l'esito che la guardia
	// promette, non lo stato che la produce.
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Interrupter = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Guarder = SpawnControlUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* Assailant = SpawnControlUnit(World, 0, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Interrupter"), Interrupter) || !TestNotNull(TEXT("Guarder"), Guarder)
		|| !TestNotNull(TEXT("Assailant"), Assailant) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 InterruptIdx = AddControlAbility(Interrupter, TEXT("Action.Interrupt"));
	Interrupter->PlannedAbilityIndex = InterruptIdx;
	Interrupter->PlannedAttackTarget = Guarder;
	Interrupter->PlannedCell = Interrupter->Cell;

	const int32 GuardIdx = AddControlAbility(Guarder, TEXT("Action.Guard"));
	Guarder->PlannedAbilityIndex = GuardIdx;
	Guarder->PlannedCell = Guarder->Cell;

	const int32 HealthBefore = Guarder->Health;
	Assailant->PlannedAbilityIndex = 0; // attacco base dell'eroe (indice 0, catalogo v0.1)
	Assailant->PlannedAttackTarget = Guarder;
	Assailant->PlannedCell = Assailant->Cell;

	RunControlTurn(TM);

	// Il colpo pieno lo dichiara l'attacco base di chi colpisce: la proprieta' e' «Guard non e'
	// interrompibile, quindi la sua riduzione si applica comunque», non «il colpo fa 25».
	const int32 DamageTaken = HealthBefore - Guarder->Health;
	TestEqual(TEXT("Guard non interrompibile: la riduzione al primo danno si applica comunque"),
		DamageTaken, Assailant->AttackPower - URTCombatLibrary::GuardFirstHitReduction);
	DestroyControlWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlowExtraCostPerCellTest,
	"RefactorTactics.Actions.Slow.ExtraCostPerCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlowExtraCostPerCellTest::RunTest(const FString&)
{
	// Il meccanismo che sostituisce il dimezzamento pre-CP4.2: +1 al costo di OGNI cella, non una riduzione
	// flat del budget. Verificato al livello puro (Dijkstra/A*), prima ancora di passare da un'unita' viva.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 5);

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.MapHash = Map->ComputeHash();
	Snapshot.Revision = Map->Revision;

	FRTHexSimUnit Normal(0, FRTCellId(0, 0), /*MoveBudget*/ 5);
	FRTHexSimUnit Slowed(1, FRTCellId(0, 0), /*MoveBudget*/ 5);
	Slowed.MoveCostModifier = 1;
	Snapshot.Units = { Normal, Slowed };

	// Budget 5, celle a costo 1: un'unita' normale raggiunge 5 celle di distanza (linea retta, costo 5).
	const FRTHexPathResult NormalPath =
		URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 0, FRTCellId(5, 0));
	TestTrue(TEXT("normale: raggiunge 5 celle di distanza"), NormalPath.Status == ERTHexPathStatus::Success);

	// Rallentata: ogni cella costa 2 (1 base + 1 di Slow). Con budget 5 arriva al massimo a 2 celle (costo 4),
	// non a 5 (costerebbe 10).
	const FRTHexPathResult SlowedFar =
		URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 1, FRTCellId(5, 0));
	TestTrue(TEXT("rallentata: 5 celle sono fuori portata"), SlowedFar.Status != ERTHexPathStatus::Success);

	const FRTHexPathResult SlowedNear =
		URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 1, FRTCellId(2, 0));
	TestTrue(TEXT("rallentata: 2 celle (costo 4) restano affrontabili"), SlowedNear.Status == ERTHexPathStatus::Success);

	const FRTHexPathResult SlowedTooFar =
		URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 1, FRTCellId(3, 0));
	TestTrue(TEXT("rallentata: 3 celle (costerebbero 6) sono fuori budget"),
		SlowedTooFar.Status != ERTHexPathStatus::Success);

	// Le celle raggiungibili riflettono lo stesso sovrapprezzo.
	const TArray<FRTHexReachableCell> NormalReach = URTHexSimLibrary::ReachableCells(Snapshot, 0);
	const TArray<FRTHexReachableCell> SlowedReach = URTHexSimLibrary::ReachableCells(Snapshot, 1);
	TestTrue(TEXT("la rallentata raggiunge MENO celle a parita' di budget"), SlowedReach.Num() < NormalReach.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlowAppliesInLiveTurnTest,
	"RefactorTactics.Actions.Slow.AppliesInLiveTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlowAppliesInLiveTurnTest::RunTest(const FString&)
{
	// Il test precedente verifica la FORMULA (pura); questo verifica che il turno VIVO la applichi davvero —
	// `ARTTurnManager::MakeCurrentSnapshot` deve leggere `Status.Slow` e passarlo come `MoveCostModifier`
	// nello snapshot della fase Move. Senza questo wiring, il meccanismo esisterebbe solo sulla carta: nessun
	// altro test lo scopre (e' esattamente cosi' che la mutazione l'ha trovato in fase di verifica).
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 8);

	ARTUnit* Slower = SpawnControlUnit(World, 0, FRTCellId(-1, 0)); // adiacente: Slow ha portata 1
	ARTUnit* Mover = SpawnControlUnit(World, 1, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Slower"), Slower) || !TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 SlowIdx = AddControlAbility(Slower, TEXT("Action.Slow"));
	Slower->PlannedAbilityIndex = SlowIdx;
	Slower->PlannedAttackTarget = Mover;
	Slower->PlannedCell = Slower->Cell;

	// Rallentato: ogni cella costa 2 invece di 1, quindi con budget B si arriva a B/2 celle invece che a B.
	// Il budget si LEGGE dall'unita': era scritto 5, il valore del Ranger legacy, e la proprieta' sotto
	// esame — «Slow raddoppia il costo per cella» — non dipende da quale eroe cammina.
	const int32 Budget = Mover->GetEffectiveMoveRange();
	const int32 MaxCells = Budget / 2;
	TestTrue(TEXT("il budget basta a distinguere rallentato da non rallentato"), MaxCells >= 1 && MaxCells < Budget);

	Mover->PlannedWaypoints.Reset();
	Mover->PlannedPath.Reset();
	Mover->PlannedPath.Add(FRTCellId(0, 0));
	for (int32 Q = 1; Q <= Budget; ++Q)
	{
		Mover->PlannedWaypoints.Add(FRTCellId(Q, 0));
		Mover->PlannedPath.Add(FRTCellId(Q, 0));
	}
	Mover->PlannedCell = FRTCellId(Budget, 0);
	Mover->PlannedAbilityIndex = INDEX_NONE;

	RunControlTurn(TM);

	TestTrue(FString::Printf(TEXT("rallentato: si ferma a %d celle, non arriva a %d"), MaxCells, Budget),
		Mover->Cell == FRTCellId(MaxCells, 0));
	DestroyControlWorld(World);
	return true;
}


/**
 * Una spinta ATTRAVERSO il fuoco brucia (#308).
 *
 * `spec-tassonomia-movimento.md` §3: «lo spostamento forzato ignora il costo VOLONTARIO del terreno. Non
 * ignora la geometria, e non ignora gli hazard». Chi viene spinto attraverso `asciutto -> fuoco -> asciutto`
 * ha attraversato quella cella di fuoco, e ne subisce le conseguenze pur non avendo speso un solo punto
 * movimento: il costo e' cio' che si paga per SCEGLIERE di passare, la geometria e' cio' che c'e'.
 *
 * Non lo faceva. `KPath` — la linea percorsa dalla spinta — era calcolata «per l'animazione», e un commento
 * rimandava il danno da attraversamento «all'ambiente attivo (epic E8)». E8 e' atterrata (sei checkpoint
 * `INTEGRATED`) e questo punto non e' stato ripassato: un rinvio scaduto, non una decisione.
 *
 * MISURA DIFFERENZIALE, non un numero assoluto: la stessa scena due volte, identica tranne la superficie
 * della cella di mezzo. Cosi' il danno dell'azione che spinge (`Guardian.Sweep` ne dichiara 30) esce dal
 * conto da solo, e resta esattamente cio' che il fuoco aggiunge. Un test scritto sul totale mentirebbe alla
 * prima volta che qualcuno ribilancia l'azione.
 *
 * La cella d'ARRIVO non brucia: e' deliberato. Con l'arrivo in fiamme il test passerebbe anche applicando i
 * soli effetti della destinazione — cioe' anche col difetto meta' corretto — e la domanda dell'issue era
 * proprio sulle celle INTERMEDIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushThroughFireTest,
	"RefactorTactics.Actions.Push.CrossesHazardsOfEveryCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushThroughFireTest::RunTest(const FString&)
{
	// Restituisce la salute della vittima a fine turno, e dove e' finita. `bBurningMidCell` accende la sola
	// cella di mezzo.
	auto RunPush = [this](bool bBurningMidCell, FRTCellId& OutCell, bool& bOutBurning) -> int32
	{
		UWorld* World = MakeControlWorld();
		if (!World) { return -1; }

		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 6))
		{
			FRTHexCellData Data(Id);
			if (bBurningMidCell && Id == FRTCellId(2, 0))
			{
				Data.Surface = ERTHexSurface::Fire;
				// Il costo del catalogo per il fuoco e' 2, e va messo: se la spinta lo leggesse, il test
				// direbbe «non brucia» quando il difetto vero sarebbe «la spinta paga il terreno».
				Data.MoveCost = 2;
			}
			M->AddOrUpdateCell(Data);
		}
		M->SortCells();
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		ARTUnit* Mover = SpawnControlUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Victim = SpawnControlUnit(World, 1, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Mover || !Victim || !TM) { DestroyControlWorld(World); return -1; }

		Victim->PushResistance = 0;

		// Spinta di DUE celle: con una cella sola non esisterebbe una cella INTERMEDIA da attraversare,
		// che e' precisamente cio' che questo test guarda.
		URTActionData* Ability = NewObject<URTActionData>(Mover);
		Ability->Def = MakePush2Def();
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		Mover->Abilities.Add(Ability);
		Mover->PlannedAbilityIndex = Mover->Abilities.Num() - 1;
		Mover->PlannedAttackTarget = Victim;
		Victim->PlannedAbilityIndex = INDEX_NONE;
		Victim->PlannedCell = Victim->Cell;

		RunControlTurn(TM);

		OutCell = Victim->Cell;
		bOutBurning = Victim->HasStatus(TAG_Status_Burning);
		const int32 Health = Victim->Health;
		DestroyControlWorld(World);
		return Health;
	};

	FRTCellId ClearCell, BurningCell;
	bool bClearBurning = false, bFireBurning = false;
	const int32 HealthOverClearGround = RunPush(/*bBurningMidCell*/ false, ClearCell, bClearBurning);
	const int32 HealthThroughFire = RunPush(/*bBurningMidCell*/ true, BurningCell, bFireBurning);

	if (!TestTrue(TEXT("entrambe le esecuzioni sono andate a buon fine"),
		HealthOverClearGround >= 0 && HealthThroughFire >= 0))
	{
		return false;
	}

	// Precondizione: la spinta e' avvenuta, di due celle, e in ENTRAMBI i casi finisce nello stesso posto.
	// Senza, la differenza di salute potrebbe venire da una geometria diversa invece che dal fuoco.
	TestTrue(TEXT("spinta di due celle su terreno libero"), ClearCell == FRTCellId(3, 0));
	TestTrue(TEXT("e la stessa spinta attraverso il fuoco arriva alla stessa cella"),
		BurningCell == FRTCellId(3, 0));

	// Il catalogo terreni dichiara `Fire = 10 danni + Status.Burning 2 turni`, e `Burning` batte **8** nel
	// Cleanup (`URTCombatLibrary::BurningCleanupDamage`) — anche in quello del turno stesso, che gira dopo il
	// Blast. Il conto atteso e' quindi la somma dei due, scritta come somma e non come `18`: un numero unico
	// non direbbe quale dei due manca il giorno che il test diventa rosso.
	const int32 ExpectedFireCost = 10 + URTCombatLibrary::BurningCleanupDamage;
	TestEqual(TEXT("attraversare il fuoco costa l'ingresso piu' il Burning del Cleanup, e non li paga chi non lo attraversa"),
		HealthOverClearGround - HealthThroughFire, ExpectedFireCost);
	TestFalse(TEXT("su terreno libero nessuno brucia"), bClearBurning);
	TestTrue(TEXT("chi e' stato spinto nel fuoco ne esce in fiamme: gli effetti sono TUTTI quelli della cella, non il solo danno"),
		bFireBurning);

	return true;
}

/**
 * Essere spinti non consuma il movimento della vittima (#308, terzo bullet).
 *
 * La regola sta nella stessa §3: lo spostamento forzato «non consuma il `MoveBudget` della vittima, ne' la
 * sua azione, ne' il suo Dash». Non e' osservabile su un campo — il budget vive nello snapshot del turno, non
 * sull'unita' — ma lo e' sul risultato: una vittima spinta nel Blast **si muove comunque** nel Move dello
 * stesso turno, che risolve dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushDoesNotSpendVictimMoveTest,
	"RefactorTactics.Actions.Push.DoesNotSpendTheVictimMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushDoesNotSpendVictimMoveTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Mover = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnControlUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Mover || !Victim || !TM) { DestroyControlWorld(World); return false; }

	Victim->PushResistance = 0;

	URTActionData* Ability = NewObject<URTActionData>(Mover);
	Ability->Def = MakePush2Def();
	Ability->RangeCells = Ability->Def.RangeCells;
	Ability->Power = 0;
	Mover->Abilities.Add(Ability);
	Mover->PlannedAbilityIndex = Mover->Abilities.Num() - 1;
	Mover->PlannedAttackTarget = Victim;

	// La vittima pianifica di andare a NORD-EST. Verra' spinta a est nel Blast, e il Move — che risolve dopo —
	// deve comunque portarla dove aveva deciso.
	const FRTCellId Goal(3, -2);
	Victim->PlannedAbilityIndex = INDEX_NONE;
	Victim->PlannedCell = Goal;

	RunControlTurn(TM);

	TestTrue(TEXT("la vittima e' stata spinta E si e' comunque mossa dove voleva"), Victim->Cell == Goal);

	DestroyControlWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
