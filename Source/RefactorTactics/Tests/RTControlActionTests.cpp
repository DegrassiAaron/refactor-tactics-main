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
		// 🔴 **`PlannedCell` si allinea alla cella di posa, e non e' una comodita'.** Il default
		// `FRTCellId()` e' `(0,0,0)` — una cella VERA — e `HasPlannedNormalMove()` e' `PlannedCell != Cell`:
		// un'unita' spawnata e lasciata senza piano pianifica quindi un movimento verso l'origine, con
		// traversate spurie e voci di TurnLog che nessun test ha chiesto. Neutralizzarlo qui vale per tutti
		// gli spawn del file — l'alternativa era ripetere l'assegnazione a ogni chiamata e dimenticarla,
		// che e' esattamente cio' che succedeva.
		U->PlannedCell = Cell;
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
	/** Abbastanza da sopravvivere al decremento del Cleanup, e da restare leggibile. */
	constexpr int32 MinCooldownTurns = 3;

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

	/**
	 * Mette l'azione nello **slot 0**, sostituendo l'abilita' che ci stava, e allinea lo specchio legacy del
	 * cooldown. Torna sempre `0`.
	 *
	 * ⚠️ **Serve per poter MISURARE il cooldown**, e non e' un vezzo: `ConsumeAbility` scrive in
	 * `AbilityCooldowns` solo se l'indice e' valido, e `ConfigureFromHeroData` dimensiona quell'array sulle
	 * abilita' dell'eroe. Un'abilita' APPESA in coda — quel che fa `AddControlAbility` — sta a un indice che
	 * l'array non copre, quindi il cooldown non viene mai scritto e `CanUseAbility` risponde sempre `true`.
	 * Un test che asserisse «non ha pagato» su un'abilita' appesa sarebbe verde **anche col difetto**.
	 *
	 * ⚠️ `ConsumeAbility` legge `URTActionData::CooldownTurns`, non `Def.CooldownTurns`: uno specchio
	 * legacy che `AddControlAbility` non copia, come non copia `RangeCells` e `Power`.
	 *
	 * ⚠️ **E il cooldown si alza a `MinCooldownTurns`**, perche' il segnale deve sopravvivere al turno:
	 * il Cleanup dello stesso turno decrementa, quindi un cooldown da **1** e' gia' tornato a zero quando il
	 * test guarda — ed e' il valore che `Action.Heal` dichiara. MISURATO: entrambi i casi riportavano
	 * «cooldown residuo: 0», cioe' un'asserzione «non ha pagato» verde **anche col difetto**. Il test
	 * misura SE si paga, non quanto: il valore del catalogo e' pinnato altrove.
	 */
	int32 UseControlAbilityInSlot0(ARTUnit* Unit, const TCHAR* ActionId)
	{
		const int32 Appesa = AddControlAbility(Unit, ActionId);
		if (Appesa == INDEX_NONE || !Unit->Abilities.IsValidIndex(0)) { return INDEX_NONE; }

		URTActionData* Azione = Unit->Abilities[Appesa];
		Unit->Abilities.RemoveAt(Appesa);
		Unit->Abilities[0] = Azione;
		Azione->CooldownTurns = FMath::Max(MinCooldownTurns, Azione->Def.CooldownTurns);
		return 0;
	}


	/**
	 * Lo scenario d'interruzione: chi interrompe, chi viene interrotto, chi avrebbe incassato il colpo.
	 *
	 * ⚠️ Le distanze sono di UNA cella: `Action.Interrupt` ha portata 1, e una versione precedente di questo
	 * montaggio le aveva messe a due — l'interruzione non avveniva e il test cadeva su una premessa rotta,
	 * con un messaggio che sembrava parlare della traccia.
	 *
	 * ⚠️ Nessuno sull'origine: `FRTCellId()` di default e' `(0,0,0)`, che e' una cella vera, e un'asserzione
	 * su una cella che coincide col default passerebbe anche senza assegnazione.
	 */
	struct FInterruptScenario
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Interrupter = nullptr;
		ARTUnit* Attacker = nullptr;   // chi SUBISCE l'interruzione: la sua azione viene cancellata
		ARTUnit* Victim = nullptr;     // chi avrebbe incassato il colpo cancellato
		int32 InterruptIdx = INDEX_NONE;
	};

	bool BuildInterruptScenario(FAutomationTestBase& Test, FInterruptScenario& Out)
	{
		Out.World = MakeControlWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), Out.World)) { return false; }
		SpawnControlMap(Out.World, 6);

		Out.Interrupter = SpawnControlUnit(Out.World, 0, FRTCellId(3, 0));
		Out.Attacker = SpawnControlUnit(Out.World, 1, FRTCellId(4, 0));
		Out.Victim = SpawnControlUnit(Out.World, 0, FRTCellId(5, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(TEXT("Interrupter"), Out.Interrupter)
			|| !Test.TestNotNull(TEXT("Attacker"), Out.Attacker)
			|| !Test.TestNotNull(TEXT("Victim"), Out.Victim)
			|| !Test.TestNotNull(TEXT("TM"), Out.TM))
		{
			return false;
		}

		Out.InterruptIdx = AddControlAbility(Out.Interrupter, TEXT("Action.Interrupt"));
		Out.Interrupter->PlannedAbilityIndex = Out.InterruptIdx;
		Out.Interrupter->PlannedAttackTarget = Out.Attacker;
		Out.Interrupter->PlannedCell = Out.Interrupter->Cell;

		Out.Attacker->PlannedAbilityIndex = 0; // attacco base, interrompibile
		Out.Attacker->PlannedAttackTarget = Out.Victim;
		Out.Attacker->PlannedCell = Out.Attacker->Cell;
		return true;
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

	const int32 HealIdx = UseControlAbilityInSlot0(Curatore, TEXT("Action.Heal"));
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

	// 🔴 **E il cooldown NON e' stato pagato** (`#1445`, [D-200]): la portata e' l'unico dei modi di fallire
	// che si conosce in pianificazione, quindi l'azione non e' mai PARTITA. E' la stessa regola che
	// `ModifyArc` seguiva gia' — «il cooldown paga solo cio' che ha davvero toccato la mappa» — e che la cura
	// contraddiceva nello stesso file.
	//
	// ⚠️ La premessa serve: senza cooldown dichiarato l'asserzione passerebbe per il motivo sbagliato.
	if (TestTrue(TEXT("premessa: il catalogo dichiara un cooldown"),
		Curatore->Abilities[HealIdx]->CooldownTurns > 0))
	{
		AddInfo(FString::Printf(TEXT("cooldown residuo: %d"), Curatore->GetAbilityCooldown(HealIdx)));
		TestTrue(TEXT("la cura fuori portata non brucia il cooldown"), Curatore->CanUseAbility(HealIdx));
	}

	DestroyControlWorld(World);
	return true;
}

/**
 * **Una cura che non ha niente da applicare lascia una traccia.**
 *
 * Ci si arriva DOPO l'annotazione — il cooldown lo scrive `SpendStartedAbilities` a fase finita (`#1451`;
 * fino ad allora questa riga diceva «gia' bruciato», che era vero e non lo e' piu') — e dopo che il piano e'
 * stato azzerato: prima di [D-196] non restava niente, ne' nel TurnLog ne' nel combat log, e un replay non
 * poteva spiegare perche' il turno del curatore non avesse prodotto nulla e perche' l'abilita' finisse in
 * ricarica.
 *
 * ⚠️ Il motivo e' `NoEffect` e non `None`: quello significa «l'azione e' eseguibile», e la resa generica
 * direbbe «non eseguibile» — falso in tutti e due i versi. L'azione era valida e non aveva niente da
 * applicare.
 *
 * ⚠️ Lo stato si costruisce a mano perche' nessun dato spedito ci arriva: `Action.Heal` dichiara `Heal 20`.
 * E' il caso del data asset scritto male — e senza questo test il valore d'enum sarebbe uno slot
 * permanente comprato senza evidenza che il ramo si raggiunga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHealWithoutEffectIsTracedTest,
	"RefactorTactics.Actions.Heal.NoEffectIsTraced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHealWithoutEffectIsTracedTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Curatore = SpawnControlUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* Ferito = SpawnControlUnit(World, 0, FRTCellId(3, 0)); // adiacente: la portata non c'entra
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Curatore"), Curatore) || !TestNotNull(TEXT("Ferito"), Ferito)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	// Una cura senza effetti: `ActionId` giusto, `Effects` vuoto.
	//
	// ⚠️ Nello **slot 0**, e col cooldown alzato, per le stesse due ragioni di `UseControlAbilityInSlot0`:
	// `AbilityCooldowns` non copre gli indici appesi, e un cooldown da 1 e' gia' scaduto quando il test
	// guarda. Costruita a mano invece di passare dall'helper perche' serve una `Def` MUTILATA, che l'helper
	// non sa produrre.
	URTActionData* Rotta = NewObject<URTActionData>(Curatore);
	Rotta->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Heal"));
	Rotta->Def.Effects.Empty();
	Rotta->RangeCells = Rotta->Def.RangeCells;
	Rotta->CooldownTurns = MinCooldownTurns;
	Curatore->Abilities[0] = Rotta;
	const int32 HealIdx = 0;

	Ferito->Health = FMath::Max(1, Ferito->Health - 30);
	const int32 SaluteFerito = Ferito->Health;

	Curatore->PlannedAbilityIndex = HealIdx;
	Curatore->PlannedAttackTarget = Ferito;
	Curatore->PlannedCell = Curatore->Cell;

	RunControlTurn(TM);

	TestEqual(TEXT("premessa: nessuna cura e' avvenuta"), Ferito->Health, SaluteFerito);

	// 🔴 **E il cooldown E' stato pagato** (`#1445`, [D-200]). E' la terza faccia del confine, insieme a
	// `OutOfRangeIsTraced` (non paga) e `DeadTargetIsTraced` (paga): l'azione era valida, e' partita e ha
	// raggiunto il bersaglio: che la def non portasse un `Heal` utile e' un difetto del DATO, non una mira
	// impossibile. Un catalogo rotto non e' una leva di bilanciamento su cui costruire un'eccezione.
	//
	// ⚠️ Prima di `#1445` questo cooldown non si asseriva affatto, e la ragione dichiarata era che il
	// Cleanup lo azzera nello stesso turno — vera per il valore del catalogo, 1. Il test lo alza a
	// `MinCooldownTurns` perche' il segnale sopravviva: misura SE si paga, non quanto.
	AddInfo(FString::Printf(TEXT("cooldown residuo: %d"), Curatore->GetAbilityCooldown(HealIdx)));
	TestFalse(TEXT("una cura senza effetti resta pagata"), Curatore->CanUseAbility(HealIdx));

	const FRTTurnLogEntry* Vuota = TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect);
	});
	if (TestNotNull(TEXT("il turno speso a vuoto e' nel record autoritativo"), Vuota))
	{
		TestEqual(TEXT("accredita chi ha provato a curare"), Vuota->UnitId, Curatore->StableUnitId);
		TestEqual(TEXT("e nomina l'azione"), Vuota->ActionId, FName(TEXT("Action.Heal")));
	}

	DestroyControlWorld(World);
	return true;
}

/**
 * **Una cura su chi e' caduto nel frattempo lascia una traccia.**
 *
 * Gli attacchi risolvono a priorita' 50-65 e le cure a 70: l'alleato puo' morire **nello stesso Blast** in
 * cui qualcuno lo sta curando. `CollectHealActions` ha gia' accettato il piano e ANNOTATO l'azione come
 * partita (`#1451`: fino ad allora la bruciava sul posto, e questa riga diceva cosi'),
 * quindi prima di `#1447` il replay mostrava un curatore con l'abilita' in ricarica e nessuna azione
 * registrata — la terza faccia dell'asimmetria che [D-196] ha chiuso per `OutOfRange` e `NoEffect`.
 *
 * ⚠️ Il bersaglio si abbatte a mano invece di farlo uccidere da un attacco nello stesso turno: cio' che si
 * misura e' la voce, non l'ordine delle priorita' — che ha i suoi test — e farla dipendere da un secondo
 * attaccante avrebbe legato questo test a un bilanciamento che puo' cambiare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHealOnDeadAllyIsTracedTest,
	"RefactorTactics.Actions.Heal.DeadTargetIsTraced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHealOnDeadAllyIsTracedTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Curatore = SpawnControlUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* Ferito = SpawnControlUnit(World, 0, FRTCellId(3, 0)); // adiacente: la portata non c'entra
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Curatore"), Curatore) || !TestNotNull(TEXT("Ferito"), Ferito)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 HealIdx = UseControlAbilityInSlot0(Curatore, TEXT("Action.Heal"));
	Curatore->PlannedAbilityIndex = HealIdx;
	Curatore->PlannedAttackTarget = Ferito;
	Curatore->PlannedCell = Curatore->Cell;

	// L'alleato cade PRIMA che la cura risolva. La cura e' gia' stata pianificata e validata.
	Ferito->ApplyCombatState(0, 0);
	if (!TestFalse(TEXT("premessa: l'alleato e' caduto"), Ferito->IsAlive()))
	{
		DestroyControlWorld(World);
		return false;
	}

	RunControlTurn(TM);

	const FRTTurnLogEntry* Mancata = TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::TargetDead);
	});
	if (TestNotNull(TEXT("il turno speso su un morto e' nel record autoritativo"), Mancata))
	{
		TestEqual(TEXT("accredita chi ha provato a curare"), Mancata->UnitId, Curatore->StableUnitId);
		TestEqual(TEXT("e nomina l'azione"), Mancata->ActionId, FName(TEXT("Action.Heal")));
	}

	// 🔴 **E il cooldown E' stato pagato** ([D-200]), che e' l'altra meta' del confine: il bersaglio era vivo
	// e in portata quando la cura e' partita, ed e' la SIMULTANEITA' ad averla disfatta. Un esito si paga.
	// Senza questa asserzione, «la cura fuori portata non paga» si potrebbe soddisfare togliendo il costo a
	// tutti i modi di fallire, e la regola diventerebbe un'altra.
	if (TestTrue(TEXT("premessa: il catalogo dichiara un cooldown"),
		Curatore->Abilities[HealIdx]->CooldownTurns > 0))
	{
		AddInfo(FString::Printf(TEXT("cooldown residuo: %d"), Curatore->GetAbilityCooldown(HealIdx)));
		TestFalse(TEXT("la cura su un bersaglio caduto nella simultaneita' resta pagata"),
			Curatore->CanUseAbility(HealIdx));
	}

	// ⚠️ NON si rilegge `Ferito` dopo il turno: `ConcludeTurn` chiama `DestroyDefeatedUnits`, che distrugge
	// l'Actor di chi e' caduto — «nemmeno il pointer sopravvive alla partita», dice `RTUnit.h`. Leggerlo
	// funzionerebbe finche' la memoria resta allocata, e diventerebbe una lettura di spazzatura al primo GC.
	// Che la cura non resusciti lo pinnano gia' i test della cura; qui si misura la TRACCIA.

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
 * **Due interruttori sull'INTERROTTO cancellano UNA azione, e la traccia lo dice una volta.**
 *
 * ⚠️ Chi subisce l'interruzione qui e' `Attacker` — la sua e' l'azione cancellata — mentre `Victim` e' chi
 * avrebbe incassato il colpo. Due ruoli, due nomi: la prima stesura di questo commento li chiamava
 * entrambi «vittima», e chi debuggasse un fallimento cercherebbe voci su `Victim`, che non ne produce.
 *
 * La traccia ne portava due prima della guardia: l'effetto di gioco era deduplicato da un `TSet` di
 * unita', ma la voce veniva scritta una volta per **colpo** di Interrupt.
 *
 * Ora si tiene traccia degli INTENTI cancellati, non delle unita': due Interrupt aggiungono lo stesso
 * indice al set e la seconda passata non scrive niente. La deduplicazione non e' una guardia in piu', e'
 * una conseguenza di aver scelto la chiave giusta.
 */
/**
 * **L'interruttore paga il cooldown che il catalogo gli da'.**
 *
 * `Action.Interrupt` dichiara cooldown 2 e non lo pagava mai: si poteva interrompere ogni singolo turno
 * (`#1444`). I suoi colpi escono da `Plan.Hits` col filtro di `ApplyInterrupts` — devono, o un colpo a
 * Power 0 conterebbe come «primo colpo» per `ApplyFirstHitDelta` — e uscivano PRIMA che `ResolveCombat`
 * costruisse `Attackers` dai colpi sopravvissuti, che e' cio' da cui `MarkAttackerAbilitiesSpent` legge.
 * `Action.Interrupt` risolve in fase `Control`, quindi nemmeno la consumazione del Prep lo copriva.
 *
 * ⚠️ **La fixture SOSTITUISCE un'abilita' invece di aggiungerne una**, e non e' un dettaglio:
 * `ConsumeAbility` scrive in `AbilityCooldowns` solo se l'indice e' valido, e `ConfigureFromHeroData`
 * dimensiona quell'array sulle abilita' dell'eroe. Un'abilita' appesa in coda sta a un indice che l'array
 * non copre, quindi il cooldown non viene mai scritto e un test che lo guardasse misurerebbe la fixture
 * invece del resolver — verde con la correzione tolta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterrupterPaysCooldownTest,
	"RefactorTactics.Actions.Interrupt.InterrupterPaysCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterrupterPaysCooldownTest::RunTest(const FString&)
{
	FInterruptScenario Sc;
	if (!BuildInterruptScenario(*this, Sc))
	{
		DestroyControlWorld(Sc.World);
		return false;
	}

	// L'Interrupt prende lo SLOT di un'abilita' esistente: cosi' `AbilityCooldowns` ce l'ha gia'.
	URTActionData* Interrupt = Sc.Interrupter->Abilities[Sc.InterruptIdx];
	Sc.Interrupter->Abilities.RemoveAt(Sc.InterruptIdx);
	Sc.Interrupter->Abilities[0] = Interrupt;
	Sc.Interrupter->PlannedAbilityIndex = 0;

	// `ConsumeAbility` legge `URTActionData::CooldownTurns`, non `Def.CooldownTurns`: uno specchio legacy
	// che l'helper non copia, come non copia `RangeCells` e `Power`.
	Interrupt->CooldownTurns = Interrupt->Def.CooldownTurns;
	if (!TestTrue(TEXT("premessa: il catalogo dichiara un cooldown"), Interrupt->CooldownTurns > 0))
	{
		DestroyControlWorld(Sc.World);
		return false;
	}

	const int32 SaluteVittima = Sc.Victim->Health;
	RunControlTurn(Sc.TM);

	TestEqual(TEXT("premessa: l'interruzione e' avvenuta"), Sc.Victim->Health, SaluteVittima);

	// Il cooldown e' stato pagato. Si guarda `CanUseAbility` e non il numero: il Cleanup dello stesso turno
	// lo decrementa gia' una volta, e pinnare il valore esatto legherebbe il test alla lunghezza dichiarata
	// invece che alla regola.
	AddInfo(FString::Printf(TEXT("cooldown residuo dopo il turno: %d"),
		Sc.Interrupter->GetAbilityCooldown(0)));
	TestFalse(TEXT("l'interruttore non puo' interrompere di nuovo subito"),
		Sc.Interrupter->CanUseAbility(0));

	DestroyControlWorld(Sc.World);
	return true;
}

/**
 * **Un Interrupt che non trova nessuno paga lo stesso.**
 *
 * `#1444` aveva agganciato il costo dell'Interrupt ai suoi `FRTHexAttackHit`, e un Interrupt puo' non
 * produrne nessuno: basta che il bersaglio dichiarato non sia piu' li'. L'azione era stata pianificata e
 * validata — l'intento esiste — quindi e' stata spesa, e restava gratuita (`#1449`).
 *
 * Qui l'Interrupt si dichiara su una CELLA VUOTA: l'intento nasce e viene validato — una cella e' un
 * bersaglio legittimo — ma non c'e' nessuno da colpire, quindi nessun `FRTHexAttackHit`.
 *
 * ⚠️ **Non** si uccide il bersaglio per ottenere lo stesso effetto: li' `ValidateInstance` risponde
 * `TargetDead` e l'intento non nasce affatto — interviene il fallback, che e' un'altra regola. La prima
 * stesura di questo test lo faceva e misurava il caso sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptWithoutHitStillPaysTest,
	"RefactorTactics.Actions.Interrupt.MissedInterruptStillPays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptWithoutHitStillPaysTest::RunTest(const FString&)
{
	FInterruptScenario Sc;
	if (!BuildInterruptScenario(*this, Sc))
	{
		DestroyControlWorld(Sc.World);
		return false;
	}

	// Stessa disciplina di `InterrupterPaysCooldown`: l'Interrupt prende lo SLOT di un'abilita' esistente,
	// cosi' `AbilityCooldowns` lo copre, e lo specchio legacy del cooldown viene riempito a mano.
	URTActionData* Interrupt = Sc.Interrupter->Abilities[Sc.InterruptIdx];
	Sc.Interrupter->Abilities.RemoveAt(Sc.InterruptIdx);
	Sc.Interrupter->Abilities[0] = Interrupt;
	Sc.Interrupter->PlannedAbilityIndex = 0;
	Interrupt->CooldownTurns = Interrupt->Def.CooldownTurns;

	// 🔴 L'Interrupt si dichiara su una CELLA VUOTA: l'intento nasce e viene validato — una cella e' un
	// bersaglio legittimo — ma `CollectHexAttacks` non trova nessuno da colpire, quindi nessun
	// `FRTHexAttackHit`.
	//
	// ⚠️ NON si uccide il bersaglio per ottenere lo stesso effetto: in quel caso `ValidateInstance`
	// risponde `TargetDead` e l'intento non nasce affatto — l'azione viene annullata dal fallback, che e'
	// un'altra regola. La prima stesura di questo test lo faceva e misurava il caso sbagliato.
	Sc.Interrupter->PlannedAttackTarget = nullptr;
	Sc.Interrupter->bAttackTargetsCell = true;
	Sc.Interrupter->PlannedAttackCell = FRTCellId(2, 0); // adiacente, e vuota

	if (!TestTrue(TEXT("premessa: il catalogo dichiara un cooldown"), Interrupt->CooldownTurns > 0))
	{
		DestroyControlWorld(Sc.World);
		return false;
	}
	const int32 SaluteVittima = Sc.Victim->Health;

	RunControlTurn(Sc.TM);

	// 🔴 La premessa che rende il test quello che dice di essere: **nessun colpo**, quindi nessuna
	// interruzione — l'attacco arriva a destinazione. Senza, il giorno in cui l'Interrupt su cella tornasse
	// a colpire qualcuno il test resterebbe verde misurando il percorso vecchio.
	TestTrue(TEXT("premessa: nessun colpo, quindi l'attacco NON e' stato interrotto"),
		Sc.Victim->Health < SaluteVittima);

	AddInfo(FString::Printf(TEXT("cooldown residuo dopo il turno: %d"),
		Sc.Interrupter->GetAbilityCooldown(0)));
	TestFalse(TEXT("l'azione e' stata spesa comunque: non e' riutilizzabile subito"),
		Sc.Interrupter->CanUseAbility(0));

	DestroyControlWorld(Sc.World);
	return true;
}

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

	// ⚠️ Questa parte non e' nuova di `#1437`: la guardia esisteva gia' dopo `#1434`, con una chiave per
	// unita' invece che per intento. Il test la tiene ferma; cio' che `#1437` cambia — un impatto di carica
	// che sopravvive all'Interrupt — sta in `Actions.Charge.ImpactSurvivesInterrupt`.

	DestroyControlWorld(World);
	return true;
}

/**
 * **Chi carica e usa un'ultimate nello stesso turno la PAGA, esattamente come chi non ha caricato**
 * (`#1451` punto 2).
 *
 * `ResolveCombat` registra un attaccante la prima volta che ne incontra un colpo, e da quella
 * registrazione `MarkAttackerAbilitiesSpent` legge quale abilita' spendere. Un'unita' puo' pero' possedere
 * **due intenti** nello stesso turno: `AppendChargeImpactIntents` aggiunge quello dell'impatto, con
 * `IntentAbilityIndex == INDEX_NONE` — lo scatto l'ha gia' fatto, non c'e' niente da consumare.
 *
 * `Plan.Hits` e' ordinato per `AttackerId` e poi `TargetId`: bastava che la vittima della CARICA avesse
 * indice minore del bersaglio dell'attacco perche' l'impatto entrasse per primo. `UsedAbilityIndex`
 * riceveva `INDEX_NONE`, `ConsumeAbility(INDEX_NONE)` usciva subito, e il ramo `EnergyCost > 0` non
 * scattava: l'unita' **guadagnava** `EnergyOnHit` invece di pagare l'ultimate, che restava riutilizzabile
 * ogni turno. `#1449` l'ha corretto — si registra il primo colpo **con un'abilita' da consumare** — e questo
 * test e' la copertura che quella correzione non aveva.
 *
 * ⚠️ **L'oracolo e' un CONFRONTO, non un valore**, e la prima stesura sbagliava proprio qui: asseriva che
 * l'energia calasse, e MISURAVA 5 → 27 con l'ultimate regolarmente pagata. Nello stesso turno l'energia
 * riceve anche `EnergyPerTurn` dal Cleanup, quindi il saldo finale somma flussi diversi e non dice se
 * l'ultimate e' stata pagata. Girando lo stesso turno **con e senza la carica** quei flussi si cancellano:
 * cio' che resta e' il costo dell'ultimate, che deve essere lo stesso.
 *
 * ⚠️ **La carica avviene sul percorso reale**, non spingendo un impatto in `PendingChargeImpacts`: quel
 * campo lo popola `ResolveDashPhase` quando una mobilita' lineare si ferma addosso a qualcuno, e montarlo a
 * mano proverebbe che `AppendChargeImpactIntents` legge un array — non che il turno ci arriva.
 *
 * ⚠️ **Le celle non sono decorative**: l'attaccante finisce a `(2,0)`, la vittima della carica sta a `(3,0)`
 * e quella dell'ultimate a `(4,0)`. `Ctx.Units` e' ordinato per cella (`StableLess` su `X`, cioe' `q`),
 * quindi il bersaglio della carica ha indice MINORE — precisamente la condizione in cui il difetto si
 * manifestava. Invertendole il test resterebbe verde senza dire niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeDoesNotRefundUltimateTest,
	"RefactorTactics.Actions.Charge.ChargeDoesNotMakeTheUltimateFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeDoesNotRefundUltimateTest::RunTest(const FString&)
{
	// Lo stesso turno, con e senza la carica. `OutEnergia` e' il saldo finale del caricatore.
	auto GiraIlTurno = [this](bool bConCarica, int32& OutEnergia, bool& bOutColpito,
		bool& bOutUltimateInRicarica) -> bool
	{
		UWorld* World = MakeControlWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnControlMap(World, 8);

		// Senza carica parte gia' dove la carica lo porterebbe: cosi' l'unica differenza fra i due giri e'
		// la carica stessa, e non la distanza da cui l'ultimate viene tirata.
		ARTUnit* Caricatore = SpawnControlUnit(World, 0, bConCarica ? FRTCellId(0, 0) : FRTCellId(2, 0));
		ARTUnit* Investito = SpawnControlUnit(World, 1, FRTCellId(3, 0));
		ARTUnit* Bersaglio = SpawnControlUnit(World, 1, FRTCellId(4, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(TEXT("Caricatore"), Caricatore) || !TestNotNull(TEXT("Investito"), Investito)
			|| !TestNotNull(TEXT("Bersaglio"), Bersaglio) || !TestNotNull(TEXT("TM"), TM))
		{
			DestroyControlWorld(World);
			return false;
		}

		// L'ultimate: costa energia, e sta nello SLOT 0 perche' `AbilityCooldowns` non copre gli indici
		// appesi. ⚠️ La portata si scrive in TUTTI E DUE i posti — `Def.RangeCells` e' quella che il resolver
		// misura, `RangeCells` sull'oggetto e' lo specchio legacy — o l'ultimate non colpisce.
		URTActionData* Ultimate = NewObject<URTActionData>(Caricatore);
		Ultimate->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack"));
		Ultimate->DisplayName = FText::FromString(TEXT("Ultimate di prova"));
		Ultimate->Def.RangeCells = 3;
		Ultimate->RangeCells = 3;
		Ultimate->Power = 15;
		Ultimate->CooldownTurns = MinCooldownTurns;
		Ultimate->EnergyCost = 3;
		Caricatore->Abilities[0] = Ultimate;

		Caricatore->Energy = FMath::Clamp(Ultimate->EnergyCost + 2, 0, Caricatore->MaxEnergy);

		if (bConCarica)
		{
			const int32 ChargeIdx = AddControlAbility(Caricatore, TEXT("Action.Charge"));
			if (!TestTrue(TEXT("premessa: il catalogo ha una carica"), ChargeIdx != INDEX_NONE))
			{
				DestroyControlWorld(World);
				return false;
			}
			// Carica sul MOVIMENTO, ultimate sull'azione principale: due slot diversi ([D-191]).
			Caricatore->PlannedDashAbility = ChargeIdx;
			Caricatore->PlannedDashCell = Investito->Cell;
		}
		Caricatore->PlannedAbilityIndex = 0;
		Caricatore->PlannedAttackTarget = Bersaglio;
		Caricatore->PlannedCell = Caricatore->Cell;

		RunControlTurn(TM);

		OutEnergia = Caricatore->Energy;
		bOutColpito = Bersaglio->Health < Bersaglio->MaxHealth;
		bOutUltimateInRicarica = !Caricatore->CanUseAbility(0);

		AddInfo(FString::Printf(TEXT("%s: energia finale %d, ultimate in ricarica %s, cella %s"),
			bConCarica ? TEXT("con carica") : TEXT("senza carica"), OutEnergia,
			bOutUltimateInRicarica ? TEXT("si") : TEXT("NO"), *Caricatore->Cell.ToString()));

		// La premessa che rende il giro con la carica un caso: la carica DEVE essere avvenuta, o l'unita'
		// avrebbe un solo intento e il difetto non avrebbe occasione di manifestarsi.
		const bool bCaricaAvvenuta = !bConCarica
			|| URTHexLibrary::HexDistance(Caricatore->Cell, Investito->Cell) <= 1;
		if (!TestTrue(TEXT("premessa: la carica ha portato l'unita' addosso al bersaglio"), bCaricaAvvenuta)
			|| !TestTrue(TEXT("premessa: l'impatto precede l'attacco nell'ordine per cella"),
				URTHexLibrary::StableLess(Investito->Cell, Bersaglio->Cell)))
		{
			DestroyControlWorld(World);
			return false;
		}

		DestroyControlWorld(World);
		return true;
	};

	int32 EnergiaConCarica = 0, EnergiaSenza = 0;
	bool bColpitoCon = false, bColpitoSenza = false;
	bool bRicaricaCon = false, bRicaricaSenza = false;
	if (!GiraIlTurno(/*bConCarica=*/ true,  EnergiaConCarica, bColpitoCon, bRicaricaCon))  { return false; }
	if (!GiraIlTurno(/*bConCarica=*/ false, EnergiaSenza,     bColpitoSenza, bRicaricaSenza)) { return false; }

	// Le premesse: in tutti e due i giri l'ultimate deve aver COLPITO, o l'unico colpo sarebbe quello
	// dell'impatto e l'energia salirebbe per una premessa rotta invece che per il resolver.
	if (!TestTrue(TEXT("premessa: con la carica l'ultimate ha colpito"), bColpitoCon)
		|| !TestTrue(TEXT("premessa: senza la carica l'ultimate ha colpito"), bColpitoSenza))
	{
		return false;
	}

	// 🔴 L'invariante: la carica non cambia quanto costa l'ultimate. Col difetto, il giro CON la carica
	// guadagnava `EnergyOnHit` invece di pagare, e i due saldi divergevano.
	TestEqual(FString::Printf(TEXT("l'ultimate costa uguale: %d con la carica, %d senza"),
		EnergiaConCarica, EnergiaSenza), EnergiaConCarica, EnergiaSenza);

	// E l'altra meta': il cooldown parte in entrambi i casi.
	TestTrue(TEXT("l'ultimate e' in ricarica anche dopo una carica"), bRicaricaCon);
	TestTrue(TEXT("e anche senza"), bRicaricaSenza);

	return true;
}

/**
 * **La catena A→B→C si risolve allo stesso modo nei due ordini di spawn** (`#1451`, [D-202]).
 *
 * `ApplyInterrupts` scorreva `Plan.Hits`, che `CollectHexAttacks` ordina per `AttackerId`, e decideva
 * mentre scopriva. La guardia di `#1437` — «un Interrupt gia' cancellato non interrompe» — funzionava
 * quindi **solo in una delle due direzioni**:
 *
 * - indice di A **minore** di quello di B: si vede `Hit(A→B)` per primo, l'intento di B entra
 *   nell'insieme, e `Hit(B→C)` viene saltato. L'azione di C sopravvive. ✅
 * - indice **maggiore**: si vede `Hit(B→C)` per primo — l'azione di C e' gia' cancellata — e solo dopo
 *   si scopre che B era a sua volta interrotto. ❌
 *
 * E da `#1449` c'era un secondo verso: B non paga (e' cancellato), quindi C perdeva la propria azione per
 * un Interrupt annullato **e** gratuito.
 *
 * ⚠️ **Il test costruisce la catena DUE volte, invertendo l'ordine di spawn**, ed e' l'unica forma che
 * cade sul difetto: una sola direzione sarebbe passata anche prima. `StableUnitId` si assegna per ordine
 * di spawn (`Roster[i]->StableUnitId = i + 1`), quindi invertire gli spawn inverte gli indici.
 *
 * ⚠️ L'oracolo e' la SALUTE della vittima finale, non una voce di traccia: e' l'effetto che il
 * difetto produceva o non produceva a seconda dell'ordine. Le distanze sono di una cella — `Action.Interrupt`
 * ha portata 1 — e la vittima di C sta a portata dell'attacco base.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptChainOrderIndependentTest,
	"RefactorTactics.Actions.Interrupt.ChainDoesNotDependOnSpawnOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptChainOrderIndependentTest::RunTest(const FString&)
{
	// ⚠️ **Si SPECCHIA la fila**, e ci sono voluti due tentativi per arrivarci. L'indice che decide l'ordine
	// di `Plan.Hits` e' quello in `Ctx.Units`, che `GatherBlastUnits` ordina **per cella**
	// (`URTHexLibrary::StableLess`) — non per ordine di spawn, e nemmeno per `StableUnitId`, che
	// `MatchRosterLess` costruisce su `(TeamId, cella, nome)`. Le prime due stesure di questo test invertivano
	// prima gli `SpawnControlUnit` e poi i team, e in tutti e due i casi giravano **due volte lo stesso
	// scenario**: misurato, non supposto.
	auto GiraLaCatena = [this](bool bSpecchiata, int32& OutDannoSubito, int32& OutInterrupted,
		FRTCellId& OutCellaA, FRTCellId& OutCellaB) -> bool
	{
		UWorld* World = MakeControlWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnControlMap(World, 6);

		// A interrompe B; B interrompe C; C attacca V. Tutti adiacenti in fila, e i team alternati perche'
		// ogni anello della catena punta a un nemico.
		//
		// Specchiata, la stessa catena percorre le celle al contrario: A finisce DOPO B nell'ordine per cella,
		// che e' l'unica cosa che questo test vuole cambiare fra i due giri.
		const FRTCellId CellaA = bSpecchiata ? FRTCellId(3, 0) : FRTCellId(1, 0);
		const FRTCellId CellaB = FRTCellId(2, 0);
		const FRTCellId CellaC = bSpecchiata ? FRTCellId(1, 0) : FRTCellId(3, 0);
		const FRTCellId CellaV = bSpecchiata ? FRTCellId(0, 0) : FRTCellId(4, 0);
		ARTUnit* A = SpawnControlUnit(World, 0, CellaA);
		ARTUnit* B = SpawnControlUnit(World, 1, CellaB);
		ARTUnit* C = SpawnControlUnit(World, 0, CellaC);
		ARTUnit* V = SpawnControlUnit(World, 1, CellaV);
		OutCellaA = CellaA;
		OutCellaB = CellaB;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("C"), C)
			|| !TestNotNull(TEXT("V"), V) || !TestNotNull(TEXT("TM"), TM))
		{
			DestroyControlWorld(World);
			return false;
		}

		const int32 IdxA = AddControlAbility(A, TEXT("Action.Interrupt"));
		A->PlannedAbilityIndex = IdxA;
		A->PlannedAttackTarget = B;
		A->PlannedCell = A->Cell;

		const int32 IdxB = AddControlAbility(B, TEXT("Action.Interrupt"));
		B->PlannedAbilityIndex = IdxB;
		B->PlannedAttackTarget = C;
		B->PlannedCell = B->Cell;

		C->PlannedAbilityIndex = 0; // attacco base, interrompibile
		C->PlannedAttackTarget = V;
		C->PlannedCell = C->Cell;

		const int32 SaluteIniziale = V->Health;
		RunControlTurn(TM);
		OutDannoSubito = SaluteIniziale - V->Health;

		OutInterrupted = TM->GetTurnLog().FilterByPredicate([](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
		}).Num();

		AddInfo(FString::Printf(TEXT("%s: A in %s, B in %s -> danno %d, cancellate %d"),
			bSpecchiata ? TEXT("specchiata") : TEXT("dritta"),
			*CellaA.ToString(), *CellaB.ToString(), OutDannoSubito, OutInterrupted));

		DestroyControlWorld(World);
		return true;
	};

	int32 DannoDritta = 0, InterruptedDritta = 0;
	int32 DannoSpecchiata = 0, InterruptedSpecchiata = 0;
	FRTCellId A1, B1, A2, B2;
	if (!GiraLaCatena(/*bSpecchiata=*/ false, DannoDritta, InterruptedDritta, A1, B1)) { return false; }
	if (!GiraLaCatena(/*bSpecchiata=*/ true,  DannoSpecchiata, InterruptedSpecchiata, A2, B2)) { return false; }

	// 🔴 **La premessa che rende il test un test**: i due giri devono avere ordini OPPOSTI. Si confronta con
	// lo stesso comparatore che `GatherBlastUnits` usa — `URTHexLibrary::StableLess` sulle celle — perche' e'
	// quello a decidere `AttackerId` e quindi l'ordine di `Plan.Hits`. Senza questa coppia di asserzioni il
	// test girerebbe due volte lo stesso scenario e concorderebbe sempre: e' successo due volte scrivendolo.
	if (!TestTrue(TEXT("premessa: dritta, A viene prima di B nell'ordine per cella"),
			URTHexLibrary::StableLess(A1, B1))
		|| !TestTrue(TEXT("premessa: specchiata, A viene DOPO B"),
			URTHexLibrary::StableLess(B2, A2)))
	{
		return false;
	}

	AddInfo(FString::Printf(TEXT("danno a V: %d contro %d; voci Interrupted: %d contro %d"),
		DannoDritta, DannoSpecchiata, InterruptedDritta, InterruptedSpecchiata));

	// La premessa: senza questa, «i due ordini concordano» sarebbe vero anche se in entrambi non succedesse
	// niente. A interrompe B, quindi B non cancella C, quindi C colpisce.
	if (!TestTrue(TEXT("premessa: nella catena dritta l'attacco di C arriva a V"), DannoDritta > 0))
	{
		return false;
	}

	// 🔴 L'invariante: l'ordine di spawn non decide chi resta cancellato.
	TestEqual(TEXT("il danno a V non dipende dall'ordine per cella"), DannoSpecchiata, DannoDritta);
	TestEqual(TEXT("e nemmeno quante azioni risultano interrotte"), InterruptedSpecchiata, InterruptedDritta);

	// Una sola: quella di B. L'attacco di C non e' cancellato, perche' l'Interrupt di B e' caduto.
	TestEqual(TEXT("una sola azione cancellata: quella di B"), InterruptedDritta, 1);

	return true;
}

/**
 * **Due Interrupt reciproci si neutralizzano: nessuno dei due cancella** (`#1451`, [D-202]).
 *
 * E' il caso che il punto fisso non puo' stratificare — un ciclo non ha radice — e la semantica va scelta.
 * La scelta e' la lettura letterale di `#1437` applicata a **entrambi insieme** invece che al primo che
 * l'ordine degli indici incontrava: ciascuno e' cancellato dall'altro, quindi nessuno dei due interrompe,
 * e le due azioni originali procedono.
 *
 * ⚠️ **Pagano lo stesso**: hanno prodotto un colpo, quindi la regola di `#1449` li raggiunge. Si sono
 * neutralizzati, non hanno rinunciato — e questo test lo pinna, perche' e' la meta' che si perderebbe
 * leggendo solo «nessuno dei due cancella».
 *
 * ⚠️ Due unita' adiacenti che si interrompono a vicenda non sono un caso di laboratorio:
 * `Action.Interrupt` ha portata 1, quindi e' la configurazione ordinaria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptCycleNeutralisesTest,
	"RefactorTactics.Actions.Interrupt.MutualInterruptsNeutralise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptCycleNeutralisesTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	// A e B si interrompono a vicenda. Entrambi useranno lo SLOT 0, cosi' il cooldown e' osservabile.
	ARTUnit* A = SpawnControlUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* B = SpawnControlUnit(World, 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 IdxA = UseControlAbilityInSlot0(A, TEXT("Action.Interrupt"));
	A->PlannedAbilityIndex = IdxA;
	A->PlannedAttackTarget = B;
	A->PlannedCell = A->Cell;

	const int32 IdxB = UseControlAbilityInSlot0(B, TEXT("Action.Interrupt"));
	B->PlannedAbilityIndex = IdxB;
	B->PlannedAttackTarget = A;
	B->PlannedCell = B->Cell;

	RunControlTurn(TM);

	// 🔴 Nessuna azione cancellata: i due si sono neutralizzati.
	const int32 Cancellate = TM->GetTurnLog().FilterByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
	}).Num();
	AddInfo(FString::Printf(TEXT("azioni cancellate: %d; cooldown A=%d B=%d"),
		Cancellate, A->GetAbilityCooldown(IdxA), B->GetAbilityCooldown(IdxB)));
	TestEqual(TEXT("due Interrupt reciproci non cancellano niente"), Cancellate, 0);

	// 🔴 E l'altra meta': hanno speso l'azione, quindi la pagano.
	if (TestTrue(TEXT("premessa: il catalogo dichiara un cooldown"), A->Abilities[IdxA]->CooldownTurns > 0))
	{
		TestFalse(TEXT("A ha pagato l'Interrupt che si e' neutralizzato"), A->CanUseAbility(IdxA));
		TestFalse(TEXT("e B lo stesso"), B->CanUseAbility(IdxB));
	}

	// 🔴 **E il turno lascia traccia** (`#1460`, [D-203]). Fino al 2026-08-27 due unita' che si
	// neutralizzavano pagavano un cooldown e producevano **zero** voci: il replay non poteva spiegare perche'
	// nessuna delle due interruzioni avesse avuto effetto. E' la classe che [D-196] ha chiuso quattro volte.
	const TArray<FRTTurnLogEntry> Neutralizzate = TM->GetTurnLog().FilterByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Neutralised);
	});
	AddInfo(FString::Printf(TEXT("voci di neutralizzazione: %d"), Neutralizzate.Num()));
	if (TestEqual(TEXT("due voci: una per ciascun Interrupt neutralizzato"), Neutralizzate.Num(), 2))
	{
		// Ognuna nomina CHI ha speso l'azione — e' la sua che non ha ottenuto niente — e QUALE azione era.
		TSet<int32> Soggetti;
		for (const FRTTurnLogEntry& E : Neutralizzate)
		{
			Soggetti.Add(E.UnitId);
			TestEqual(TEXT("e nomina l'azione"), E.ActionId, FName(TEXT("Action.Interrupt")));

			// ⚠️ **`TgtCell` porta la cella dell'ALTRO**, non la propria. Una voce che dicesse «puntavo a me
			// stesso» sarebbe precisa e falsa — la stessa forma che il commento della voce `Cancelled`
			// condanna — e `TgtCell` entra nell'hash. Qui `SrcCell` e `TgtCell` sono le due celle in gioco,
			// scambiate fra le due voci.
			const FRTCellId Attesa = (E.UnitId == A->StableUnitId) ? B->Cell : A->Cell;
			TestEqual(TEXT("e punta alla cella dell'altro, non alla propria"), E.TgtCell, Attesa);
			TestNotEqual(TEXT("che non e' la sua"), E.TgtCell, E.SrcCell);
		}
		TestTrue(TEXT("le due voci accreditano le due unita' diverse"),
			Soggetti.Contains(A->StableUnitId) && Soggetti.Contains(B->StableUnitId));
	}

	// ⚠️ E **nessuna** voce di cancellazione: le due cose non si confondono. `Interrupted` dice «la tua
	// azione e' stata annullata», `Neutralised` dice «la tua azione non ha annullato niente» — versi opposti,
	// e riusare lo stesso motivo rifarebbe la coppia con due significati che `#1430` ha appena separato.
	TestEqual(TEXT("e nessuna voce di azione interrotta"),
		TM->GetTurnLog().FilterByPredicate([](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
		}).Num(), 0);

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

/**
 * **Un'azione pianificata si paga se e' PARTITA, e non altrimenti — misurato in un posto solo.**
 *
 * `#1451` punto 3: «un'azione spesa si paga» era implementata nel Blast in punti diversi con criteri
 * diversi, e ogni azione nuova ne voleva uno in piu'. [D-200] ha scritto la regola —
 *
 * > *La PORTATA decide se un'azione parte; tutto il resto e' un esito, e un esito si paga.*
 *
 * — ma la scriveva per una sola azione, la cura. Questa tabella la misura su **tutti** i punti di consumo
 * delle azioni pianificate del Blast, con un oracolo solo.
 *
 * ⚠️ **A tabella e non un test per riga, e la forma e' la lezione di [D-201]**: `MakeFlatArena` era stata
 * corretta da sola, e i sei builder rimasti avevano ereditato il difetto perche' «uno solo e' coperto»
 * bastava a far passare la suite. `HexMap.EveryArenaBuilderIsOneRevision` scorre una tabella per questo, e
 * qui vale identico: alcune righe duplicano un test che esiste gia' (`Heal.OutOfRangeIsTraced`,
 * `Interrupt.MissedInterruptStillPays`), e la duplicazione **e' il punto** — quei test misurano ognuno la
 * propria azione, questo misura la REGOLA.
 *
 * ⚠️ **Il cooldown si legge con `CanUseAbility` e a `MinCooldownTurns`**, non col numero: il Cleanup dello
 * stesso turno decrementa, quindi un cooldown da 1 e' gia' tornato a zero quando il test guarda — e
 * un'asserzione «non ha pagato» sarebbe verde anche col difetto. E' la trappola gia' pagata da `#1445`.
 *
 * ⚠️ **Si misurano ENTRAMBE le meta' di `ConsumeAbility`** — cooldown e energia — perche' il refactor le
 * ha spostate tutte e due, e un oracolo sul solo cooldown resterebbe verde togliendo l'altra.
 *
 * ⚠️ **Due righe pinnano l'ASIMMETRIA di [D-209]**, e senza di esse il rimedio «ovvio» — portare
 * `IsAlive()` dentro `SpendStartedAbilities` per «finire la pulizia» — lascerebbe la tabella verde mentre
 * cambia il gioco: `curatore che cade nel Blast` (annota da vivo, paga comunque) e `attaccante che
 * colpisce e cade` (per un attaccante «spesa» vuol dire sopravvissuto, e non paga).
 *
 * ⚠️ **Fuori da questa tabella, e dichiarato**: le REAZIONI (`ResolveInterceptions`, `RunReactionPass`) non
 * sono azioni pianificate ma inneschi condizionali, e a governarle e' [D-092] col proprio contatore di
 * attivazioni; le altre fasi (Prep, Move, Dash, Environment) hanno tempistiche proprie, e ricondurle a
 * [D-200] e' una decisione che non e' stata presa.
 */
namespace
{
	/** Che cosa mette in campo la riga. Cambia il bersaglio e la sua squadra, mai l'oracolo. */
	enum class ESpesaMontaggio : uint8
	{
		SuSeStessi,        //< `Action.Cleanse`: bersaglio implicito, la distanza non conta
		Alleato,           //< `Action.Heal`, `Action.ModifyArc`
		NemicoCheAttacca,  //< `Action.Interrupt`: serve anche chi incassa il colpo cancellato
		NemicoDaColpire,   //< l'attacco base dell'eroe, cioe' `MarkAttackerAbilitiesSpent`
	};

	/**
	 * L'energia e' l'ALTRA meta' di `ConsumeAbility` (`RTUnit.cpp`: sconta `EnergyCost` **e** scrive il
	 * cooldown), e il refactor di `#1451` ha spostato tutte e due. Un oracolo sul solo cooldown resterebbe
	 * verde togliendo la riga dell'energia — il difetto che la tabella esiste per non lasciar passare.
	 *
	 * ⚠️ `EnergyPerTurn` si azzera nella riga: il Cleanup accredita reddito a ogni unita' (`RTTurnManager`),
	 * e qui e' rumore su una misura che parla di quanto si SPENDE. Non si aggira nessuna regola: si toglie
	 * un'entrata estranea, come si alza il cooldown perche' sopravviva al decremento.
	 */
	constexpr int32 CostoEnergiaProva = 5;
	constexpr int32 EnergiaIniziale = 50;

	struct FSpesaCase
	{
		const TCHAR* Nome;
		const TCHAR* ActionId;      //< `nullptr` = si usa lo slot 0 dell'eroe (attacco base)
		ESpesaMontaggio Montaggio;
		int32 Distanza;             //< celle fra chi agisce e il bersaglio
		bool bAttoreVivo;            //< falso = arriva al Blast gia' caduto (Prep o Dash)
		bool bMuoreNelBlast;         //< vero = un nemico adiacente lo uccide NELLA stessa fase
		bool bPurificaUnoStatoPosseduto; //< solo Cleanse: dichiara la priorita' su uno stato che HA
		bool bAbilitaACosto;         //< falso = `EnergyCost` 0, cioe' il ramo `GainEnergy` degli attaccanti
		bool bDevePagare;
		const TCHAR* Perche;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedActionPaysOnlyIfItStartedTest,
	"RefactorTactics.Actions.Blast.PlannedActionPaysOnlyIfItStarted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedActionPaysOnlyIfItStartedTest::RunTest(const FString&)
{
	const FSpesaCase Casi[] = {
		{ TEXT("purificazione efficace"), TEXT("Action.Cleanse"), ESpesaMontaggio::SuSeStessi,
		  0, true, false, true, true, true,
		  TEXT("l'azione e' partita e ha tolto lo stato") },
		{ TEXT("purificazione a vuoto"), TEXT("Action.Cleanse"), ESpesaMontaggio::SuSeStessi,
		  0, true, false, false, true, true,
		  TEXT("[D-200]: ha guardato e non c'era niente, che e' un ESITO — e il costo impedisce la purificazione assicurativa ogni turno") },
		{ TEXT("cura in portata"), TEXT("Action.Heal"), ESpesaMontaggio::Alleato,
		  1, true, false, false, true, true,
		  TEXT("bersaglio raggiungibile: l'azione parte") },
		{ TEXT("cura fuori portata"), TEXT("Action.Heal"), ESpesaMontaggio::Alleato,
		  5, true, false, false, true, false,
		  TEXT("[D-200]: la portata e' l'unico modo di fallire noto in PIANIFICAZIONE — non e' mai partita") },
		{ TEXT("arco in portata"), TEXT("Action.ModifyArc"), ESpesaMontaggio::Alleato,
		  1, true, false, false, true, true,
		  TEXT("la topologia e' stata toccata") },
		{ TEXT("arco fuori portata"), TEXT("Action.ModifyArc"), ESpesaMontaggio::Alleato,
		  5, true, false, false, true, false,
		  TEXT("stessa regola della cura, e ModifyArc la seguiva gia' con parole sue") },
		{ TEXT("interruzione a segno"), TEXT("Action.Interrupt"), ESpesaMontaggio::NemicoCheAttacca,
		  1, true, false, false, true, true,
		  TEXT("#1444: ha prodotto un colpo") },
		{ TEXT("interruzione fuori portata"), TEXT("Action.Interrupt"), ESpesaMontaggio::NemicoCheAttacca,
		  3, true, false, false, true, false,
		  TEXT("#1449: niente colpo e nessuna cella mirata — e' un'azione non avvenuta come le altre") },
		{ TEXT("attacco che colpisce"), nullptr, ESpesaMontaggio::NemicoDaColpire,
		  1, true, false, false, true, true,
		  TEXT("il colpo e' arrivato: MarkAttackerAbilitiesSpent lo raccoglie") },
		{ TEXT("pianificatore gia' caduto"), TEXT("Action.Heal"), ESpesaMontaggio::Alleato,
		  1, false, false, false, true, false,
		  TEXT("un cadavere non paga un'azione che non ha mai eseguito: arriva al Blast col piano addosso, e GatherBlastUnits non filtra i morti") },
		// 🔴 Le DUE righe che pinnano l'asimmetria di [D-209], e senza le quali il rimedio «ovvio» —
		// portare `IsAlive()` dentro `SpendStartedAbilities` — lascerebbe la tabella verde.
		{ TEXT("curatore che cade nel Blast"), TEXT("Action.Heal"), ESpesaMontaggio::Alleato,
		  1, true, true, false, true, true,
		  TEXT("[D-209]: chi cura e' vivo quando ANNOTA, e paga anche se cade piu' tardi nella stessa fase") },
		{ TEXT("attaccante che colpisce e cade"), nullptr, ESpesaMontaggio::NemicoDaColpire,
		  1, true, true, false, true, false,
		  TEXT("[D-209]: per un attaccante «spesa» vuol dire SOPRAVVISSUTO alla fase, e questo non lo e'") },
		// L'altro ramo di `MarkAttackerAbilitiesSpent`: senza costo in energia non si logga `Ultimate!`, si
		// accredita `EnergyOnHit`. Senza questa riga, cancellare quel ramo — cioe' l'accumulo di energia di
		// ogni attaccante del gioco — non farebbe cadere niente.
		{ TEXT("attacco base senza costo"), nullptr, ESpesaMontaggio::NemicoDaColpire,
		  1, true, false, false, false, true,
		  TEXT("il colpo e' arrivato e l'abilita' e' gratuita: si paga il cooldown e si ACCUMULA energia") },
	};

	for (const FSpesaCase& C : Casi)
	{
		UWorld* World = MakeControlWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnControlMap(World, 6);

		ARTUnit* Attore = SpawnControlUnit(World, 0, FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(*FString::Printf(TEXT("%s: attore"), C.Nome), Attore)
			|| !TestNotNull(*FString::Printf(TEXT("%s: TM"), C.Nome), TM))
		{
			DestroyControlWorld(World);
			return false;
		}

		// L'abilita' va nello SLOT 0 e con un cooldown che sopravvive al Cleanup, o il cooldown non e'
		// osservabile affatto: `AbilityCooldowns` non copre gli indici appesi in coda.
		const int32 Idx = C.ActionId ? UseControlAbilityInSlot0(Attore, C.ActionId) : 0;
		if (!TestTrue(*FString::Printf(TEXT("%s: l'abilita' e' nello slot 0"), C.Nome),
			Idx == 0 && Attore->Abilities.IsValidIndex(0)))
		{
			DestroyControlWorld(World);
			return false;
		}

		URTActionData* Azione = Attore->Abilities[0];
		if (!C.ActionId)
		{
			Azione->CooldownTurns = MinCooldownTurns; // l'attacco base dell'eroe non ne dichiara uno
		}
		// 🔴 **Il costo in energia e' una proprieta' della RIGA, non della tabella.**
		// `MarkAttackerAbilitiesSpent` si biforca su `EnergyCost > 0`: con un costo logga `Ultimate!`, senza
		// accredita `EnergyOnHit`. Mettendo un costo ovunque — come faceva la prima stesura di questa
		// misura — il ramo `GainEnergy` smetteva di essere coperto da qualunque riga, e cancellarlo del
		// tutto (cioe' togliere l'accumulo di energia a OGNI attaccante del gioco) lasciava la tabella
		// verde. La riga `attacco base senza costo` esiste per tenerlo rosso.
		Azione->EnergyCost = C.bAbilitaACosto ? CostoEnergiaProva : 0;
		Attore->Energy = EnergiaIniziale;
		Attore->EnergyPerTurn = 0; // il reddito del Cleanup e' rumore su una misura che parla di spesa

		if (!TestTrue(*FString::Printf(TEXT("%s: premessa: il cooldown e' osservabile"), C.Nome),
			Azione->CooldownTurns >= MinCooldownTurns))
		{
			DestroyControlWorld(World);
			return false;
		}

		switch (C.Montaggio)
		{
		case ESpesaMontaggio::SuSeStessi:
			// Quale stato togliere lo dice il PIANO, mai il resolver: la lista dichiarata e' l'unica
			// differenza fra la riga efficace e quella a vuoto.
			if (C.bPurificaUnoStatoPosseduto)
			{
				Attore->ApplyStatus(TAG_Status_Root, 2);
				Attore->PlannedCleansePriority = { TAG_Status_Root };
			}
			else
			{
				Attore->PlannedCleansePriority = { TAG_Status_Exposed }; // dichiarato e non posseduto
			}
			Attore->PlannedAbilityIndex = Idx;
			Attore->PlannedCell = Attore->Cell;
			break;

		case ESpesaMontaggio::Alleato:
		{
			ARTUnit* Alleato = SpawnControlUnit(World, 0, FRTCellId(C.Distanza, 0));
			if (!TestNotNull(*FString::Printf(TEXT("%s: alleato"), C.Nome), Alleato))
			{
				DestroyControlWorld(World);
				return false;
			}
			Alleato->Health = FMath::Max(1, Alleato->Health - 30);
			Attore->PlannedAbilityIndex = Idx;
			Attore->PlannedAttackTarget = Alleato;
			Attore->PlannedCell = Attore->Cell;
			break;
		}

		case ESpesaMontaggio::NemicoCheAttacca:
		{
			ARTUnit* Attaccante = SpawnControlUnit(World, 1, FRTCellId(C.Distanza, 0));
			ARTUnit* Vittima = SpawnControlUnit(World, 0, FRTCellId(C.Distanza + 1, 0));
			if (!TestNotNull(*FString::Printf(TEXT("%s: attaccante"), C.Nome), Attaccante)
				|| !TestNotNull(*FString::Printf(TEXT("%s: vittima"), C.Nome), Vittima))
			{
				DestroyControlWorld(World);
				return false;
			}
			Attore->PlannedAbilityIndex = Idx;
			Attore->PlannedAttackTarget = Attaccante;
			Attore->PlannedCell = Attore->Cell;

			Attaccante->PlannedAbilityIndex = 0; // attacco base, interrompibile
			Attaccante->PlannedAttackTarget = Vittima;
			Attaccante->PlannedCell = Attaccante->Cell;
			break;
		}

		case ESpesaMontaggio::NemicoDaColpire:
		{
			ARTUnit* Nemico = SpawnControlUnit(World, 1, FRTCellId(C.Distanza, 0));
			if (!TestNotNull(*FString::Printf(TEXT("%s: nemico"), C.Nome), Nemico))
			{
				DestroyControlWorld(World);
				return false;
			}
			Attore->PlannedAbilityIndex = Idx;
			Attore->PlannedAttackTarget = Nemico;
			Attore->PlannedCell = Attore->Cell;
			break;
		}
		}

		// 🔴 **Chi uccide l'attore DENTRO il Blast**, per le due righe che pinnano l'asimmetria di [D-209].
		// Un nemico adiacente su una cella che nessun montaggio usa, e l'attore a 1 punto vita: il colpo
		// arriva nella simultaneita' del Blast, quindi l'attore e' vivo quando ANNOTA e morto quando
		// `MarkAttackerAbilitiesSpent` guarda chi e' rimasto in piedi. Le due righe leggono i due lati.
		if (C.bMuoreNelBlast)
		{
			// ⚠️ La cella e' FUORI dall'asse `(x, 0)` che tutti i montaggi usano, e che sia libera si
			// verifica invece di darlo per scontato: `PlaceOnCell` non rifiuta una cella occupata, quindi
			// una riga futura con un montaggio fuori asse fallirebbe su un sintomo scollegato — movimento
			// bloccato, bersaglio sbagliato — con un messaggio che parla di cooldown.
			const FRTCellId CellaCarnefice(0, 1);
			TArray<AActor*> GiaInCampo;
			UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), GiaInCampo);
			bool bLibera = true;
			for (const AActor* A : GiaInCampo)
			{
				const ARTUnit* U = Cast<ARTUnit>(A);
				if (U && U->Cell == CellaCarnefice) { bLibera = false; break; }
			}
			if (!TestTrue(*FString::Printf(TEXT("%s: premessa: la cella del carnefice e' libera"), C.Nome),
				bLibera))
			{
				DestroyControlWorld(World);
				return false;
			}

			ARTUnit* Carnefice = SpawnControlUnit(World, 1, CellaCarnefice);
			if (!TestNotNull(*FString::Printf(TEXT("%s: carnefice"), C.Nome), Carnefice))
			{
				DestroyControlWorld(World);
				return false;
			}
			Carnefice->PlannedAbilityIndex = 0; // attacco base dell'eroe
			Carnefice->PlannedAttackTarget = Attore;
			Carnefice->PlannedCell = Carnefice->Cell;
			Attore->Health = 1;
		}

		// La riga del cadavere: l'unita' arriva al Blast morta e col piano ancora addosso, che e' cio' che
		// succede a chi cade in Prep o nel Dash. Si scrive lo STATO, non si aggira una regola: sotto esame
		// c'e' la guardia del resolver, non il modo in cui l'unita' e' caduta.
		if (!C.bAttoreVivo)
		{
			Attore->Health = 0;
		}

		const TWeakObjectPtr<ARTUnit> AttoreDebole(Attore);
		RunControlTurn(TM);

		// 🔴 **Il cadavere si legge anche da distrutto, e non e' una scorciatoia**: `ConcludeTurn` chiama
		// `DestroyDefeatedUnits`, che distrugge chi ha `Health <= 0` — quindi a turno finito la riga del
		// pianificatore caduto non avrebbe **nessuna** abilita' da interrogare, e la sua asserzione
		// passerebbe per assenza di misura invece che per la regola. MISURATO: `IsValid()` risponde falso.
		//
		// ⚠️ L'oggetto e' `PendingKill`, non raccolto: nessun GC gira dentro un `RunControlTurn`, e
		// `AbilityCooldowns` e' un `TArray<int32>` sull'attore. Il campionamento durante i tick non e'
		// un'alternativa: `LockInAndResolve()` risolve il turno **per intero e in modo sincrono**, quindi il
		// ciclo di tick non ha nessuna finestra in cui guardare (misurato: zero campioni su otto righe).
		ARTUnit* AttoreDopo = AttoreDebole.Get(/*bEvenIfPendingKill*/ true);
		if (!TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), C.Nome), AttoreDopo))
		{
			DestroyControlWorld(World);
			continue; // senza l'attore non c'e' niente da leggere, e un `Success` muto sarebbe peggio
		}

		// 🔴 **`GetAbilityCooldown` e non `CanUseAbility`**: il secondo e' falso anche per mancanza di
		// ENERGIA (`IsAbilityUsable(cooldown, Energy, EnergyCost)`), quindi su un'ultimate a pagamento
		// direbbe «ha pagato» per il motivo sbagliato. Qui si misura il COOLDOWN, che e' cio' che
		// `ConsumeAbility` scrive.
		// 🔴 **La premessa della riga «muore nel Blast» si verifica, o la riga si degrada in silenzio.**
		// L'oracolo di `curatore che cade nel Blast` e' identico a quello di `cura in portata`: se il
		// carnefice smettesse di uccidere — un ribilanciamento del danno base, una copertura fra le due
		// celle — la riga resterebbe verde senza piu' misurare niente, e la prova mutante che la giustifica
		// evaporerebbe. E' il modo di fallire che l'intestazione di questo test cita due volte.
		if (C.bMuoreNelBlast)
		{
			TestFalse(*FString::Printf(TEXT("%s: premessa: l'attore e' davvero caduto nel Blast"), C.Nome),
				AttoreDopo->IsAlive());
		}

		const bool bHaPagato = AttoreDopo->GetAbilityCooldown(0) > 0;
		AddInfo(FString::Printf(TEXT("%s -> cooldown residuo %d, energia %d (%s)"),
			C.Nome, AttoreDopo->GetAbilityCooldown(0), AttoreDopo->Energy, C.Perche));
		TestEqual(*FString::Printf(TEXT("%s: %s"), C.Nome, C.Perche), bHaPagato, C.bDevePagare);

		// 🔴 **E l'energia, che e' l'altra meta' di `ConsumeAbility`.** Con `EnergyCost > 0` il ramo
		// `Ultimate!` di `MarkAttackerAbilitiesSpent` non accredita `EnergyOnHit`, e `EnergyPerTurn` e'
		// azzerato: l'unico movimento possibile e' la spesa. Senza questa riga, togliere lo scalo
		// dell'energia da `ConsumeAbility` lascerebbe tutte e dodici le righe verdi.
		int32 EnergiaAttesa = EnergiaIniziale;
		if (C.bDevePagare)
		{
			// Con un costo si spende; senza, l'unico ramo che tocca l'energia e' il `GainEnergy` di
			// `MarkAttackerAbilitiesSpent`, che accredita `EnergyOnHit` all'attaccante sopravvissuto.
			EnergiaAttesa = C.bAbilitaACosto
				? EnergiaIniziale - CostoEnergiaProva
				: FMath::Min(AttoreDopo->MaxEnergy, EnergiaIniziale + AttoreDopo->EnergyOnHit);
		}
		TestEqual(*FString::Printf(TEXT("%s: energia"), C.Nome), AttoreDopo->Energy, EnergiaAttesa);

		DestroyControlWorld(World);
	}

	return true;
}

/**
 * **Anche un curatore caduto lascia la voce della cura mancata.**
 *
 * `ApplyPlannedHeals` decideva con **due predicati opposti** se scrivere: il ramo di fallimento guardava
 * `Curatore->IsAlive()`, quello di successo — quaranta righe piu' sotto, nella stessa funzione — scriveva
 * senza nessuna guardia. Un curatore che cadeva mentre anche il bersaglio cadeva non lasciava **niente**:
 * un turno speso, un cooldown pagato e il replay senza una riga che lo spiegasse (`#1473`, [D-219]).
 *
 * 🔴 **L'oracolo e' la VOCE, non lo stato del mondo**: la cura non avviene in nessuno dei due casi, quindi
 * salute e cooldown sono identici col difetto e senza. Solo la traccia distingue.
 *
 * ⚠️ **Il curatore si legge da distrutto**: `ConcludeTurn` chiama `DestroyDefeatedUnits`, quindi a turno
 * finito l'attore non c'e' piu'. `StableUnitId` si cattura PRIMA — e' un dato, non un puntatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallenHealerStillLeavesTheEntryTest,
	"RefactorTactics.Actions.Heal.FallenHealerStillLeavesTheEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallenHealerStillLeavesTheEntryTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	ARTUnit* Curatore = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Ferito   = SpawnControlUnit(World, 0, FRTCellId(1, 0));
	ARTUnit* Boia1    = SpawnControlUnit(World, 1, FRTCellId(0, 1)); // adiacente al curatore
	ARTUnit* Boia2    = SpawnControlUnit(World, 1, FRTCellId(2, 0)); // adiacente al ferito
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Curatore"), Curatore) || !TestNotNull(TEXT("Ferito"), Ferito)
		|| !TestNotNull(TEXT("Boia1"), Boia1) || !TestNotNull(TEXT("Boia2"), Boia2)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	const int32 HealIdx = UseControlAbilityInSlot0(Curatore, TEXT("Action.Heal"));
	Curatore->PlannedAbilityIndex = HealIdx;
	Curatore->PlannedAttackTarget = Ferito;

	// Entrambi cadono NELLO STESSO Blast in cui la cura era stata accettata: e' il caso che la voce
	// `TargetDead` esiste per registrare, piu' la morte del curatore che la faceva sparire.
	Curatore->Health = 1;
	Ferito->Health = 1;
	Boia1->PlannedAbilityIndex = 0; // attacco base dell'eroe
	Boia1->PlannedAttackTarget = Curatore;
	Boia2->PlannedAbilityIndex = 0;
	Boia2->PlannedAttackTarget = Ferito;

	// ⚠️ **`StableUnitId` si legge DOPO il turno, non allo spawn**: lo assegna l'allestimento al lock-in, e
	// prima vale **zero** per tutti. MISURATO: catturandolo qui il predicato cercava `UnitId == 0` e non
	// trovava la voce, che porta `1`. L'attore non sopravvive a `ConcludeTurn`, quindi si rilegge da
	// `PendingKill` — l'oggetto non e' raccolto, e `StableUnitId` e' un `int32` sull'attore.
	const TWeakObjectPtr<ARTUnit> CuratoreDebole(Curatore);

	RunControlTurn(TM);

	ARTUnit* CuratoreDopo = CuratoreDebole.Get(/*bEvenIfPendingKill*/ true);
	if (!TestNotNull(TEXT("il curatore e' ancora leggibile"), CuratoreDopo))
	{
		DestroyControlWorld(World);
		return false;
	}
	if (!TestFalse(TEXT("premessa: il curatore e' davvero caduto nel Blast"), CuratoreDopo->IsAlive()))
	{
		DestroyControlWorld(World);
		return false;
	}
	const int32 IdCuratore = CuratoreDopo->StableUnitId;

	const FRTTurnLogEntry* Mancata = TM->GetTurnLog().FindByPredicate([IdCuratore](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::TargetDead)
			&& E.UnitId == IdCuratore;
	});

	if (TestNotNull(TEXT("la cura mancata e' nel record autoritativo anche se chi curava e' caduto"), Mancata))
	{
		TestEqual(TEXT("e nomina l'azione, non la generica"), Mancata->ActionId, FName(TEXT("Action.Heal")));
	}

	DestroyControlWorld(World);
	return true;
}

/**
 * **Il bot arma il modulo di loadout, non la reazione di kit.**
 *
 * Fino a [D-220] il bot scorreva le abilità e armava la **prima** di slot reazione, poi `break`: con due
 * reazioni la scelta la faceva l'**ordine degli indici**. `EquipLoadout` fa `Abilities.Add`, quindi il
 * modulo sta in fondo e il kit davanti — e il modulo non veniva armato **mai**. In v0.1 (2v2 contro bot)
 * è metà del campo: la composizione esisteva solo per il giocatore umano (`#1485`).
 *
 * 🔴 **L'oracolo è QUALE reazione risulta armata**, non che ne risulti armata una: col difetto
 * `PlannedReactionAbility` puntava comunque a un'abilità valida, e un test che avesse solo controllato
 * `!= INDEX_NONE` sarebbe stato verde.
 *
 * ⚠️ La riga non dice che il modulo sia più forte del kit — quella è una domanda di E15, aperta come
 * `BOT-REACT-1`. Dice che un loadout che il bot non usa non esiste per metà delle unità in campo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotArmsTheLoadoutModuleTest,
	"RefactorTactics.HexBotPlay.BotArmsTheLoadoutModuleNotTheKitReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotArmsTheLoadoutModuleTest::RunTest(const FString&)
{
	UWorld* World = MakeControlWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnControlMap(World, 6);

	// Riktor con il proprio loadout di default: kit `Interposition` (indice 4) + modulo `Reaction.Cleanse`.
	ARTUnit* Bot = SpawnControlUnit(World, 1, FRTCellId(2, 0));
	ARTUnit* Umano = SpawnControlUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bot"), Bot) || !TestNotNull(TEXT("Umano"), Umano) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyControlWorld(World);
		return false;
	}

	Bot->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeRiktor());
	Bot->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(FName(TEXT("Hero.Riktor"))));
	Bot->bIsBotControlled = true;

	// Premessa: l'unità porta DUE reazioni, o il test non misura niente — è la condizione che rende la
	// scelta possibile, e senza di essa il difetto non sarebbe nemmeno esprimibile.
	int32 Reazioni = 0;
	for (int32 R = 0; R < Bot->NumAbilities(); ++R)
	{
		const URTActionData* A = Bot->GetAbility(R);
		if (A && A->Def.Slot == ERTActionSlot::Reaction) { ++Reazioni; }
	}
	if (!TestEqual(TEXT("premessa: Riktor porta due reazioni (kit + modulo)"), Reazioni, 2))
	{
		DestroyControlWorld(World);
		return false;
	}

	RunControlTurn(TM);

	const URTActionData* Armata = Bot->GetAbility(Bot->PlannedReactionAbility);
	if (!TestNotNull(TEXT("il bot ha armato una reazione"), Armata))
	{
		DestroyControlWorld(World);
		return false;
	}

	AddInfo(FString::Printf(TEXT("armata: %s (indice %d di %d)"),
		*Armata->Def.ActionId.ToString(), Bot->PlannedReactionAbility, Bot->NumAbilities()));

	// L'oracolo: viene dal LOADOUT, non dal kit. Si chiede al catalogo invece di guardare l'indice, per la
	// stessa ragione per cui lo fa il resolver.
	TestNotNull(TEXT("il bot arma il modulo di loadout, non la reazione di kit"),
		URTCatalogLibrary::FindEquipment(Armata->Def.ActionId));

	DestroyControlWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
