#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTEquipmentData.h" // `Reaction.Anchor` arriva dal catalogo equipaggiamento (CP 7.5, #505)
#include "Turn/RTReactionLibrary.h"  // ERTReactionOutcome: la voce dell'ancora dev'essere `Activated`

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **Perche' un esito ha una causa** — CP 11.3 (`#79`) e `#307`, verificati sul percorso reale.
 *
 * Le due issue sono una domanda sola posta a due categorie del TurnLog. Prima di questo lavoro:
 *
 * - una voce `Combat` diceva «22 danni, eliminata» e **non diceva da cosa**. Con due attaccanti nello stesso
 *   Blast il replay non poteva attribuire il colpo, e la coppia «azione base + profilo» che [D-033] richiede
 *   restava nel catalogo invece che nella traccia;
 * - uno **spostamento forzato** non lasciava alcuna voce: solo una riga di combat log, che e' una stringa per
 *   l'HUD e non finisce nel file. Chi rileggeva una traccia vedeva l'unita' altrove senza nulla che lo
 *   spiegasse — e uno **scatto** non lasciava nemmeno quello, perche' la fase Dash non scriveva movimento.
 *
 * Tutti i test qui girano su un turno vero (`LockInAndResolve`), mai sulle librerie pure: la domanda e' se
 * la traccia che la PARTITA produce sia spiegabile, e una libreria chiamata a mano non risponde.
 */
namespace
{
	UWorld* MakeCauseWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCauseWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnCauseMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnCauseUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeVektor());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Azione generica dal catalogo core. `Power` azzerato: vedi la nota in RTControlActionTests.cpp. */
	int32 AddCoreAbility(ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Ability = NewObject<URTActionData>(Unit);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Ability->Power = Spec.Amount; break; }
		}
		Unit->Abilities.Add(Ability);
		return Unit->Abilities.Num() - 1;
	}

	void RunCauseTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Le voci di una categoria, nell'ordine in cui il turno le ha scritte. */
	TArray<FRTTurnLogEntry> EntriesOfCategory(const ARTTurnManager* TM, ERTLogCategory Category)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == Category) { Out.Add(E); }
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatEntryNamesItsActionTest,
	"RefactorTactics.TurnLog.CombatEntryNamesItsAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatEntryNamesItsActionTest::RunTest(const FString&)
{
	// CP 11.3 (`#79`), prima meta': **con che cosa** e' stato inflitto il danno.
	//
	// Il caso e' DUE attaccanti con due azioni diverse nello stesso Blast, non uno solo: e' la situazione che
	// l'issue descrive — con una voce anonima il replay sa che sono arrivati due colpi e non sa attribuirli.
	// Con un attaccante solo il test passerebbe anche se l'identita' fosse cablata a caso.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Pusher  = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Shooter = SpawnCauseUnit(World, 0, FRTCellId(0, 3));
	ARTUnit* PushVictim  = SpawnCauseUnit(World, 1, FRTCellId(1, 0)); // adiacente: `Action.Push` ha portata 1
	ARTUnit* ShotVictim  = SpawnCauseUnit(World, 1, FRTCellId(3, 3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("PushVictim"), PushVictim) || !TestNotNull(TEXT("ShotVictim"), ShotVictim)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const int32 PushIdx = AddCoreAbility(Pusher, TEXT("Action.Push"));
	Pusher->PlannedAbilityIndex = PushIdx;
	Pusher->PlannedAttackTarget = PushVictim;

	// Un'azione con identita' d'eroe, cosi' le due voci non possono essere confuse fra loro.
	URTActionData* Shot = NewObject<URTActionData>(Shooter);
	Shot->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack"));
	Shot->Def.ActionId = FName(TEXT("Hero.TestShot"));
	Shot->Def.RangeCells = 4;
	Shot->Def.Effects.Reset();
	Shot->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 12));
	Shot->RangeCells = 4;
	Shot->Power = 12;
	Shooter->Abilities.Add(Shot);
	Shooter->PlannedAbilityIndex = Shooter->Abilities.Num() - 1;
	Shooter->PlannedAttackTarget = ShotVictim;

	PushVictim->PlannedCell = PushVictim->Cell;
	ShotVictim->PlannedCell = ShotVictim->Cell;

	RunCauseTurn(TM);

	const TArray<FRTTurnLogEntry> Combat = EntriesOfCategory(TM, ERTLogCategory::Combat);
	if (!TestTrue(TEXT("il Blast ha lasciato almeno due voci di combattimento"), Combat.Num() >= 2))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// Le voci si cercano per CELLA del bersaglio, e la portata di questa affermazione va detta con
	// precisione: vale **dentro un singolo Blast**, dove le celle sono quelle dello stesso snapshot e una
	// cella ospita al piu' un'unita'.
	//
	// NON e' «la cella e' l'identita' dell'unita' nel turno»: [D-063] ha ritirato quella formulazione — non
	// regge per le voci ambientali, per l'interposizione (che scrive la cella del protetto) e dopo un Dash —
	// e ha introdotto `UnitId` proprio per questo. Qui la cella basta perche' il confronto non attraversa
	// nessuna fase; copiare questo pattern fuori da un singolo Blast sarebbe sbagliato.
	const FRTTurnLogEntry* Pushed = Combat.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.TgtCell == FRTCellId(1, 0);
	});
	const FRTTurnLogEntry* Shotted = Combat.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.TgtCell == FRTCellId(3, 3);
	});
	if (!TestTrue(TEXT("esiste la voce del colpo spinto"), Pushed != nullptr)
		|| !TestTrue(TEXT("esiste la voce del colpo sparato"), Shotted != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestEqual(TEXT("#79: la voce dello spintone NOMINA la propria azione"),
		Pushed->ActionId, FName(TEXT("Action.Push")));
	TestEqual(TEXT("#79: la voce del colpo NOMINA la propria, diversa"),
		Shotted->ActionId, FName(TEXT("Hero.TestShot")));
	TestTrue(TEXT("ognuna parte dalla cella del PROPRIO attaccante"),
		Pushed->SrcCell == FRTCellId(0, 0) && Shotted->SrcCell == FRTCellId(0, 3));

	// E la stringa che il giocatore legge dice la stessa cosa del campo: e' l'invariante di `#79` — «il log
	// e' coerente col TurnLog serializzato», cioe' una descrizione sola, non due parallele.
	const FString Described = URTTurnLogLibrary::DescribeEntry(*Shotted);
	TestTrue(FString::Printf(TEXT("il combat log riporta l'azione: %s"), *Described),
		Described.Contains(TEXT("Hero.TestShot")));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisplacementHasCauseAndSourceTest,
	"RefactorTactics.TurnLog.DisplacementHasCauseAndSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisplacementHasCauseAndSourceTest::RunTest(const FString&)
{
	// `#307`, il cuore: uno spostamento SUBITO deve dire **perche'** e **da chi**.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	Victim->PushResistance = 0; // la soglia deve cedere, o non c'e' spostamento da registrare
	const int32 PushIdx = AddCoreAbility(Pusher, TEXT("Action.Push"));
	Pusher->PlannedAbilityIndex = PushIdx;
	Pusher->PlannedAttackTarget = Victim;
	Victim->PlannedCell = Victim->Cell; // non si muove di sua volonta': cio' che accade e' solo la spinta

	RunCauseTurn(TM);

	const FRTCellId Pushed = Victim->Cell;
	if (!TestTrue(TEXT("il bersaglio e' stato davvero spostato"), Pushed != FRTCellId(1, 0)))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// --- 1. Lo spostamento ESISTE nel TurnLog, e si dichiara subito, non scelto -----------------------
	const TArray<FRTTurnLogEntry> Moves = EntriesOfCategory(TM, ERTLogCategory::Move);
	const FRTTurnLogEntry* Displaced = Moves.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return static_cast<ERTMoveOutcome>(E.Outcome) == ERTMoveOutcome::Displaced;
	});
	if (!TestTrue(TEXT("#307: lo spostamento forzato lascia una voce nel TurnLog"), Displaced != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestTrue(TEXT("parte da dove il bersaglio era"), Displaced->SrcCell == FRTCellId(1, 0));
	TestTrue(TEXT("arriva dove il bersaglio e' finito"), Displaced->TgtCell == Pushed);
	TestTrue(TEXT("e' registrato nel Blast, dove avviene il colpo che lo produce"),
		Displaced->Phase == ERTMatchPhase::Blast);

	// --- 2. LA CAUSA: con quale azione --------------------------------------------------------------
	TestEqual(TEXT("#307: la voce dice CON QUALE azione"), Displaced->ActionId, FName(TEXT("Action.Push")));

	// --- 3. LA SORGENTE: chi ha spinto, ricostruita dal log e non da un campo nuovo ------------------
	//
	// E' la parte di `#307` che chiedeva «chi ha spinto». La risposta non e' un campo aggiunto — sarebbe
	// costato una versione del formato — ma un GIUNTO fra due voci dello stesso Blast:
	//
	//     Combat.TgtCell == Displaced.SrcCell   &&   Combat.ActionId == Displaced.ActionId
	//     => Combat.SrcCell E' la cella di chi ha spinto
	//
	// Regge perche' una cella ospita al piu' un'unita': il bersaglio identifica il colpo in modo univoco,
	// anche con piu' attaccanti che usano la stessa azione nello stesso turno.
	const TArray<FRTTurnLogEntry> Combat = EntriesOfCategory(TM, ERTLogCategory::Combat);
	const FRTTurnLogEntry* Cause = Combat.FindByPredicate([Displaced](const FRTTurnLogEntry& E)
	{
		return E.TgtCell == Displaced->SrcCell && E.ActionId == Displaced->ActionId;
	});
	if (!TestTrue(TEXT("#307: il colpo che ha causato lo spostamento e' ritrovabile nel log"), Cause != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}
	TestTrue(TEXT("#307: e la SORGENTE e' la cella di chi ha spinto"), Cause->SrcCell == FRTCellId(0, 0));

	// La riga leggibile lo dice a parole: «spostata», non «si muove».
	const FString Described = URTTurnLogLibrary::DescribeEntry(*Displaced);
	TestTrue(FString::Printf(TEXT("il combat log distingue subito da scelto: %s"), *Described),
		Described.Contains(TEXT("spostata")) && Described.Contains(TEXT("Action.Push")));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushAndPullKeepTheirOwnCauseTest,
	"RefactorTactics.TurnLog.PushAndPullKeepTheirOwnCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushAndPullKeepTheirOwnCauseTest::RunTest(const FString&)
{
	// REGRESSIONE trovata in code review sulla PR di `#307`, non da questi test — che usavano un attaccante
	// solo per bersaglio e quindi non potevano vederla.
	//
	// Il caso: lo stesso bersaglio subisce una SPINTA da un attaccante e una TRAZIONE da un altro nello
	// stesso Blast. Il filtro «forze contraddittorie» conta le due cose SEPARATAMENTE (`KnockCount` e
	// `PullCount`), quindi entrambe passano e il bersaglio si sposta due volte, scrivendo due voci.
	//
	// La prima versione teneva le cause in **una mappa sola** chiavata sul bersaglio: la seconda `Add`
	// sovrascriveva la prima e una delle due voci dichiarava l'azione dell'attaccante sbagliato. Una causa
	// scritta, precisa e falsa — su un lavoro che esiste per rendere il TurnLog attribuibile e' il difetto
	// peggiore possibile, peggio dell'assenza di causa da cui si partiva.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	// Geometria: il bersaglio in mezzo, spingitore e trattore su lati opposti, entrambi adiacenti
	// (`Action.Push` ha portata 1, `Action.Pull` 2).
	ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* Puller = SpawnCauseUnit(World, 0, FRTCellId(1, -2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("Puller"), Puller) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	Victim->PushResistance = 0;

	const int32 PushIdx = AddCoreAbility(Pusher, TEXT("Action.Push"));
	Pusher->PlannedAbilityIndex = PushIdx;
	Pusher->PlannedAttackTarget = Victim;

	const int32 PullIdx = AddCoreAbility(Puller, TEXT("Action.Pull"));
	Puller->PlannedAbilityIndex = PullIdx;
	Puller->PlannedAttackTarget = Victim;

	Victim->PlannedCell = Victim->Cell;

	RunCauseTurn(TM);

	// Le voci di spostamento del turno. Se il gioco ne produce una sola (perche' uno dei due effetti non e'
	// arrivato) il test non ha niente da discriminare e lo dice, invece di passare per finta.
	TArray<FRTTurnLogEntry> Displacements;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Move
			&& static_cast<ERTMoveOutcome>(E.Outcome) == ERTMoveOutcome::Displaced)
		{
			Displacements.Add(E);
		}
	}

	if (!TestTrue(FString::Printf(TEXT("servono DUE spostamenti per discriminare, trovati %d"),
		Displacements.Num()), Displacements.Num() == 2))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// L'invariante: le due voci NON possono dichiarare la stessa causa. Con la mappa condivisa erano
	// identiche, ed e' questa riga che cadeva.
	TestTrue(TEXT("#307: spinta e trazione dichiarano cause DIVERSE, non la stessa sovrascritta"),
		Displacements[0].ActionId != Displacements[1].ActionId);

	// E le due cause sono esattamente quelle giocate, non due nomi qualsiasi.
	TSet<FName> Causes;
	for (const FRTTurnLogEntry& E : Displacements) { Causes.Add(E.ActionId); }
	TestTrue(TEXT("una voce e' della spinta"), Causes.Contains(FName(TEXT("Action.Push"))));
	TestTrue(TEXT("l'altra e' della trazione"), Causes.Contains(FName(TEXT("Action.Pull"))));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashLeavesAMoveEntryTest,
	"RefactorTactics.TurnLog.DashIsDistinguishableFromMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashLeavesAMoveEntryTest::RunTest(const FString&)
{
	// `#307`, terzo caso: «se e' scattata». Non basta che lo scatto lasci una voce — deve essere
	// DISTINGUIBILE da un passo, o il log risponde alla domanda sbagliata.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Runner = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Walker = SpawnCauseUnit(World, 0, FRTCellId(0, 3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Walker"), Walker)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// Uno scatta, l'altro cammina, nello STESSO turno: e' il caso discriminante. Con una sola unita' il test
	// direbbe «esiste una voce di movimento», che era vero anche prima.
	URTActionData* Dash = NewObject<URTActionData>(Runner);
	Dash->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Runner->Abilities.Add(Dash);
	Runner->PlannedDashAbility = Runner->Abilities.Num() - 1;
	Runner->PlannedDashCell = FRTCellId(3, 0);

	Walker->PlannedCell = FRTCellId(1, 3);

	RunCauseTurn(TM);

	const TArray<FRTTurnLogEntry> Moves = EntriesOfCategory(TM, ERTLogCategory::Move);

	const FRTTurnLogEntry* DashEntry = Moves.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Phase == ERTMatchPhase::Dash;
	});
	if (!TestTrue(TEXT("#307: lo scatto lascia una voce di movimento"), DashEntry != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestEqual(TEXT("#307: la voce dello scatto NOMINA la mobilita' usata"),
		DashEntry->ActionId, FName(TEXT("Action.Dash")));
	TestTrue(TEXT("parte da dove lo scattante era"), DashEntry->SrcCell == FRTCellId(0, 0));
	TestTrue(TEXT("arriva dove lo scatto l'ha portato"), DashEntry->TgtCell == Runner->Cell);

	// E il passo volontario resta distinguibile: stessa categoria, altra fase, altro ActionId. Sono le due
	// cose che rendono la causa RICOSTRUIBILE invece che indovinabile.
	const FRTTurnLogEntry* WalkEntry = Moves.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Phase == ERTMatchPhase::Move && E.SrcCell == FRTCellId(0, 3);
	});
	if (!TestTrue(TEXT("il passo volontario ha una voce sua"), WalkEntry != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}
	TestEqual(TEXT("#307: e dichiara Action.Move"), WalkEntry->ActionId, FName(TEXT("Action.Move")));
	TestTrue(TEXT("#307: scatto e passo non sono la stessa voce"),
		DashEntry->ActionId != WalkEntry->ActionId && DashEntry->Phase != WalkEntry->Phase);

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBasicAttackProfilePairInLogTest,
	"RefactorTactics.TurnLog.BasicAttackLogsBaseAndProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBasicAttackProfilePairInLogTest::RunTest(const FString&)
{
	// `#315`, gate `log_debug`. [D-033] chiede che una traccia sia leggibile come **azione base + profilo**.
	// Il campo `BaseActionId` esiste dal `#354` e il catalogo eroi lo riempie — ma nessuna voce di
	// COMBATTIMENTO lo portava, perche' le voci di combattimento non avevano identita' affatto. E' `#79` a
	// chiudere questo gate: qui si verifica che il giro completo funzioni su un colpo vero.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Attacker = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim   = SpawnCauseUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// Un attacco base d'eroe: identita' propria (`Hero.TestShot`) e generica dichiarata (`Action.BasicAttack`),
	// che e' la forma che `MakeHeroBasicAttack` produce per i quattro del roster.
	URTActionData* Shot = NewObject<URTActionData>(Attacker);
	Shot->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack"));
	Shot->Def.ActionId = FName(TEXT("Hero.TestShot"));
	Shot->Def.BaseActionId = FName(TEXT("Action.BasicAttack"));
	Shot->Def.RangeCells = 4;
	Shot->Def.Effects.Reset();
	Shot->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 12));
	Shot->RangeCells = 4;
	Shot->Power = 12;
	Attacker->Abilities.Add(Shot);

	Attacker->PlannedAbilityIndex = Attacker->Abilities.Num() - 1;
	Attacker->PlannedAttackTarget = Victim;
	Victim->PlannedCell = Victim->Cell;

	RunCauseTurn(TM);

	const TArray<FRTTurnLogEntry> Combat = EntriesOfCategory(TM, ERTLogCategory::Combat);
	const FRTTurnLogEntry* Hit = Combat.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.ActionId == FName(TEXT("Hero.TestShot"));
	});
	if (!TestTrue(TEXT("#315: il colpo dell'eroe e' nel log con la propria identita'"), Hit != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestEqual(TEXT("#315 / D-033: la voce dichiara anche l'azione GENERICA di cui e' un profilo"),
		Hit->BaseActionId, FName(TEXT("Action.BasicAttack")));

	// La forma leggibile e' quella che D-033 descrive: base **e** profilo, in una riga sola.
	const FString Described = URTTurnLogLibrary::DescribeEntry(*Hit);
	TestTrue(FString::Printf(TEXT("«azione base + profilo» nel combat log: %s"), *Described),
		Described.Contains(TEXT("Action.BasicAttack · Hero.TestShot")));

	DestroyCauseWorld(World);
	return true;
}

/**
 * **Lo spostamento MANCATO** — `#420`. `#307` ha spiegato la spinta che sposta e ha lasciato muta quella che
 * non sposta: cinque modi diversi di restare fermi, indistinguibili nel file, e due dei cinque senza nemmeno
 * una riga di combat log. Un sesto — la destinazione contesa — non lasciava proprio nulla.
 *
 * I quattro test qui sotto girano tutti su un turno vero e coprono i quattro modi RAGGIUNGIBILI dal gioco
 * come lo si gioca oggi. Il quinto (`PushResistance`) non e' testato **e non ha produttore**: D-075 l'ha
 * portata a `0` su tutto il roster, e un test che gliela rimettesse a mano verificherebbe un ramo che nessuna
 * partita attraversa — esattamente il verde che `RefactorTactics.Actions.PushResistanceIsAThreshold` gia'
 * produce, e che D-075 nota non essere diventato rosso quando il dato e' sparito.
 */
namespace
{
	/** Le voci `Move` con esito `DisplacementResisted`, nell'ordine in cui il turno le ha scritte. */
	TArray<FRTTurnLogEntry> ResistedEntries(const ARTTurnManager* TM)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : EntriesOfCategory(TM, ERTLogCategory::Move))
		{
			if (static_cast<ERTMoveOutcome>(E.Outcome) == ERTMoveOutcome::DisplacementResisted) { Out.Add(E); }
		}
		return Out;
	}

	/** Prepara una spinta: `Pusher` spinge `Victim`, nessuno dei due si muove di sua volonta'. */
	void PlanPushOn(ARTUnit* Pusher, ARTUnit* Victim)
	{
		if (!Pusher || !Victim) { return; }
		Pusher->PlannedAbilityIndex = AddCoreAbility(Pusher, TEXT("Action.Push"));
		Pusher->PlannedAttackTarget = Victim;
		Pusher->PlannedCell = Pusher->Cell;
		Victim->PushResistance = 0; // la soglia dorme dopo D-075: azzerarla e' dichiararlo, non aggirarlo
		Victim->PlannedCell = Victim->Cell;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpposingPushesLeaveATraceTest,
	"RefactorTactics.TurnLog.OpposingPushesLeaveATrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpposingPushesLeaveATraceTest::RunTest(const FString&)
{
	// CASO 4 di `#420`: due attaccanti spingono lo stesso bersaglio nello stesso Blast, le forze si annullano.
	// Non e' una difesa — il bersaglio non ha fatto nulla — ed era muto in entrambi i canali.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Left = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* Right = SpawnCauseUnit(World, 0, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Left"), Left) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("Right"), Right) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	PlanPushOn(Left, Victim);
	PlanPushOn(Right, Victim);

	RunCauseTurn(TM);

	if (!TestTrue(TEXT("premessa: le due spinte si annullano e il bersaglio resta"),
		Victim->Cell == FRTCellId(1, 0)))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Resisted = ResistedEntries(TM);
	if (!TestEqual(TEXT("#420: lo spostamento annullato lascia UNA voce nel TurnLog"), Resisted.Num(), 1))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestEqual(TEXT("#420: e dice PERCHE' — forze opposte, non una difesa"),
		static_cast<ERTDisplacementBlockReason>(Resisted[0].Amount),
		ERTDisplacementBlockReason::OpposingForces);
	TestTrue(TEXT("le due celle coincidono: e' la forma di «non si e' spostata»"),
		Resisted[0].SrcCell == FRTCellId(1, 0) && Resisted[0].TgtCell == FRTCellId(1, 0));
	TestTrue(TEXT("registrata nel Blast, dove sarebbe avvenuto lo spostamento"),
		Resisted[0].Phase == ERTMatchPhase::Blast);

	// L'azione resta VUOTA di proposito: gli attaccanti sono due, e nominarne uno direbbe che a fermare il
	// bersaglio e' stata quella spinta. Ne sono servite due, ed e' il contenuto stesso dell'esito.
	TestTrue(TEXT("#420: con due attaccanti la voce non ne nomina uno solo"), Resisted[0].ActionId.IsNone());

	const FString Described = URTTurnLogLibrary::DescribeEntry(Resisted[0]);
	TestTrue(FString::Printf(TEXT("la riga leggibile dice la causa: %s"), *Described),
		Described.Contains(TEXT("forze opposte")));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushWithoutDestinationLeavesATraceTest,
	"RefactorTactics.TurnLog.PushWithoutDestinationLeavesATrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushWithoutDestinationLeavesATraceTest::RunTest(const FString&)
{
	// CASO 5 di `#420`: la spinta arriva, e non ha dove mandare il bersaglio. Il bordo mappa lo dimostra
	// senza fabbricare dati — la mappa ha raggio 6, quindi (7,0) non esiste.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World, /*Radius=*/ 6);

	ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(5, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(6, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	PlanPushOn(Pusher, Victim);

	RunCauseTurn(TM);

	if (!TestTrue(TEXT("premessa: contro il bordo il bersaglio non si sposta"),
		Victim->Cell == FRTCellId(6, 0)))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Resisted = ResistedEntries(TM);
	if (!TestEqual(TEXT("#420: anche senza destinazione la spinta lascia una voce"), Resisted.Num(), 1))
	{
		DestroyCauseWorld(World);
		return false;
	}

	TestEqual(TEXT("#420: la causa e' geometrica, non difensiva"),
		static_cast<ERTDisplacementBlockReason>(Resisted[0].Amount),
		ERTDisplacementBlockReason::NoDestination);

	// Qui l'attaccante e' UNO: la voce puo' nominare l'azione, e lo fa. E' la differenza col caso 4.
	TestEqual(TEXT("#420: con un attaccante solo la voce dice con quale azione"),
		Resisted[0].ActionId, FName(TEXT("Action.Push")));

	const FString Described = URTTurnLogLibrary::DescribeEntry(Resisted[0]);
	TestTrue(FString::Printf(TEXT("la riga leggibile non dice «resta»: %s"), *Described),
		Described.Contains(TEXT("nessuna uscita")) && !Described.Contains(TEXT("celle")));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardAndBraceAreDistinguishableInTheLogTest,
	"RefactorTactics.TurnLog.GuardAndBraceAreDistinguishableInTheLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardAndBraceAreDistinguishableInTheLogTest::RunTest(const FString&)
{
	// CASI 1 e 2 di `#420`. Le due difese emettevano una riga di combat log — una stringa per l'HUD, che non
	// finisce nel file — quindi nel TurnLog erano la stessa identica assenza. Qui devono essere due voci
	// diverse, nello stesso turno: che e' l'unico modo di dimostrare che il file le distingue.
	//
	// ⚠️ NON e' il confine di bilanciamento `BAL-1` (#403): quello riguarda il danno, e D-074 ha stabilito che
	// ogni spinta del gioco vale 1. Qui si verifica solo che la TRACCIA dica quale difesa ha agito.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* PusherA = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Guarder = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* PusherB = SpawnCauseUnit(World, 0, FRTCellId(0, 2));
	ARTUnit* Bracer = SpawnCauseUnit(World, 1, FRTCellId(1, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("PusherA"), PusherA) || !TestNotNull(TEXT("Guarder"), Guarder)
		|| !TestNotNull(TEXT("PusherB"), PusherB) || !TestNotNull(TEXT("Bracer"), Bracer)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	PlanPushOn(PusherA, Guarder);
	PlanPushOn(PusherB, Bracer);

	// Le difese si pianificano, non si iniettano: `Action.Guard` e `Action.Brace` sono azioni di controllo che
	// risolvono prima del knockback, quindi lo stato c'e' quando la spinta lo interroga. `PlanPushOn` ha gia'
	// scritto `PlannedCell`; qui si aggiunge l'abilita' e la si sceglie.
	Guarder->PlannedAbilityIndex = AddCoreAbility(Guarder, TEXT("Action.Guard"));
	Bracer->PlannedAbilityIndex = AddCoreAbility(Bracer, TEXT("Action.Brace"));

	RunCauseTurn(TM);

	if (!TestTrue(TEXT("premessa: entrambe le difese reggono e nessuno dei due si sposta"),
		Guarder->Cell == FRTCellId(1, 0) && Bracer->Cell == FRTCellId(1, 2)))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Resisted = ResistedEntries(TM);
	if (!TestEqual(TEXT("#420: due difese, due voci"), Resisted.Num(), 2))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const FRTTurnLogEntry* GuardEntry = Resisted.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.SrcCell == FRTCellId(1, 0);
	});
	const FRTTurnLogEntry* BraceEntry = Resisted.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.SrcCell == FRTCellId(1, 2);
	});
	if (!TestTrue(TEXT("una voce per ciascun difensore, riconoscibile dalla propria cella"),
		GuardEntry != nullptr && BraceEntry != nullptr))
	{
		DestroyCauseWorld(World);
		return false;
	}

	// Il cuore: due reason code DIVERSI. Prima di `#420` erano la stessa assenza di voce.
	TestEqual(TEXT("#420: la guardia si dichiara"),
		static_cast<ERTDisplacementBlockReason>(GuardEntry->Amount), ERTDisplacementBlockReason::Guarded);
	TestEqual(TEXT("#420: l'irrigidimento si dichiara, e non e' lo stesso valore"),
		static_cast<ERTDisplacementBlockReason>(BraceEntry->Amount), ERTDisplacementBlockReason::Braced);

	const FString GuardText = URTTurnLogLibrary::DescribeEntry(*GuardEntry);
	const FString BraceText = URTTurnLogLibrary::DescribeEntry(*BraceEntry);
	TestTrue(FString::Printf(TEXT("e il giocatore legge due frasi diverse: «%s» / «%s»"),
		*GuardText, *BraceText),
		GuardText.Contains(TEXT("in guardia")) && BraceText.Contains(TEXT("irrigidito")));

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTContestedPushDestinationLeavesATraceTest,
	"RefactorTactics.TurnLog.ContestedPushDestinationLeavesATrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTContestedPushDestinationLeavesATraceTest::RunTest(const FString&)
{
	// Il SESTO modo di non muoversi, che `#420` non contava perche' vive nel secondo ciclo del knockback: due
	// bersagli spinti verso la STESSA cella restano entrambi fermi, perche' l'esito non deve dipendere da
	// quale dei due si risolve prima. Era il piu' muto dei sei — nemmeno una riga di combat log.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	// Geometria: (0,0) e' libera, e le due spinte puntano entrambe li' da lati opposti.
	ARTUnit* PusherA = SpawnCauseUnit(World, 0, FRTCellId(-2, 0));
	ARTUnit* VictimA = SpawnCauseUnit(World, 1, FRTCellId(-1, 0));
	ARTUnit* PusherB = SpawnCauseUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* VictimB = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("PusherA"), PusherA) || !TestNotNull(TEXT("VictimA"), VictimA)
		|| !TestNotNull(TEXT("PusherB"), PusherB) || !TestNotNull(TEXT("VictimB"), VictimB)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCauseWorld(World);
		return false;
	}

	PlanPushOn(PusherA, VictimA);
	PlanPushOn(PusherB, VictimB);

	RunCauseTurn(TM);

	if (!TestTrue(TEXT("premessa: la destinazione comune blocca entrambe le spinte"),
		VictimA->Cell == FRTCellId(-1, 0) && VictimB->Cell == FRTCellId(1, 0)))
	{
		DestroyCauseWorld(World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Resisted = ResistedEntries(TM);
	if (!TestEqual(TEXT("#420: due bersagli fermati, due voci"), Resisted.Num(), 2))
	{
		DestroyCauseWorld(World);
		return false;
	}

	for (const FRTTurnLogEntry& E : Resisted)
	{
		TestEqual(TEXT("#420: la causa e' la destinazione contesa"),
			static_cast<ERTDisplacementBlockReason>(E.Amount),
			ERTDisplacementBlockReason::ContestedDestination);
		TestTrue(TEXT("le due celle coincidono"), E.SrcCell == E.TgtCell);
	}

	DestroyCauseWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushedUnitFacesPusherInPlayTest,
	"RefactorTactics.TurnLog.PushedUnitFacesThePusherInPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushedUnitFacesPusherInPlayTest::RunTest(const FString&)
{
	// ⚠️ Nasce da un BUCO misurato (#548): estraendo la primitiva di spostamento (#541) ho mutato i suoi
	// dieci passi uno per volta, e togliere `Unit->Facing = Moved.Facing;` lasciava la suite **interamente
	// verde**. La regola era testata — `Facing.ForcedMovementFacesSource` — ma sulla FUNZIONE PURA: un test
	// che passa una costante a `FacingAfterDisplacement` non può vedere che in partita quel valore non
	// arriva. Prima di #541 quella riga era per giunta scritta due volte, e nessuno dei due posti era coperto.
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnCauseMap(World, 8);

	// Chi spinge è a OVEST di chi subisce, che quindi arretra verso EST e deve girarsi verso ovest.
	ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Pusher || !Victim || !TM) { DestroyCauseWorld(World); return false; }

	// ⚠️ Il facing di partenza è scelto perché DEBBA cambiare: la vittima guarda a est, cioè dalla parte
	// opposta a chi la spinge. Senza questa riga il test passerebbe anche con il passo mutato — è lo stesso
	// motivo per cui `Combat.BastionImpactShotSlows` ha un gemello di controllo.
	Victim->Facing = ERTHexDirection::E;

	PlanPushOn(Pusher, Victim);
	RunCauseTurn(TM);

	TestTrue(TEXT("la spinta ha spostato la vittima"), Victim->Cell != FRTCellId(1, 0));
	TestTrue(TEXT("e la vittima si e' girata VERSO chi l'ha spinta"),
		Victim->Facing == ERTHexDirection::W);
	TestTrue(TEXT("cioe' non guarda piu' dove guardava prima"), Victim->Facing != ERTHexDirection::E);

	// Chi spinge non si gira: la regola vale per chi SUBISCE lo spostamento.
	TestEqual(TEXT("chi spinge resta dov'era"), Pusher->Cell, FRTCellId(0, 0));

	DestroyCauseWorld(World);
	return true;
}

// =====================================================================================================
// CP 7.5 (`#505`) — `Reaction.Anchor` ANNULLA lo spostamento.
//
// E' il test che il DoD di `#62` nominava e che non era scrivibile: non mancava un dato, mancava il MOMENTO
// in cui valutarlo. «Sto per essere spinto» non si legge sui colpi raccolti — si sa solo dove la spinta e'
// gia' decisa e non ancora applicata, che e' il punto `BlastDisplacement` di `URTReactionLibrary::PassPointFor`.
//
// Sta in questo file e non fra gli `Equipment.*` perche' qui vive la famiglia a cui appartiene: le CAUSE per
// cui uno spostamento non avviene, con l'allestimento della spinta (`PlanPushOn`) e la lettura delle voci
// (`ResistedEntries`) gia' pronti. Duplicarli altrove avrebbe dato due allestimenti da tenere d'accordo.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorCancelsPushTest,
	"RefactorTactics.Equipment.Anchor.CancelsPush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorCancelsPushTest::RunTest(const FString&)
{
	const URTEquipmentData* Anchor = nullptr;
	for (const URTEquipmentData* M : URTCatalogLibrary::MakeReactionModules())
	{
		if (M && M->EquipmentId == FName(TEXT("Reaction.Anchor"))) { Anchor = M; break; }
	}
	if (!TestNotNull(TEXT("`Reaction.Anchor` e' nel catalogo dei moduli"), Anchor)) { return false; }

	// --- PREMESSA: la stessa spinta, SENZA il modulo, sposta davvero ---------------------------------
	// Senza questo gemello il test passerebbe anche se la spinta non fosse mai avvenuta — «l'unita' e' ferma»
	// e' l'esito atteso e anche quello di una scena che non fa niente.
	{
		UWorld* World = MakeCauseWorld();
		if (!TestNotNull(TEXT("World della premessa"), World)) { return false; }
		SpawnCauseMap(World);

		ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		if (!Pusher || !Victim || !TM) { DestroyCauseWorld(World); return false; }

		PlanPushOn(Pusher, Victim);
		RunCauseTurn(TM);

		const bool bMossa = (Victim->Cell != FRTCellId(1, 0));
		DestroyCauseWorld(World);
		if (!TestTrue(TEXT("premessa: senza ancora la spinta sposta il bersaglio"), bMossa))
		{
			return false; // senza una spinta che funziona, il caso sotto non proverebbe niente
		}
	}

	// --- IL CASO: con il modulo, la stessa spinta non lo sposta --------------------------------------
	UWorld* World = MakeCauseWorld();
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	SpawnCauseMap(World);

	ARTUnit* Pusher = SpawnCauseUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!Pusher || !Victim || !TM) { DestroyCauseWorld(World); return false; }

	URTActionData* Reazione = URTCatalogLibrary::MakeEquipmentAction(Anchor, Victim);
	if (!TestNotNull(TEXT("il modulo concede un'azione"), Reazione)) { DestroyCauseWorld(World); return false; }
	TestTrue(TEXT("ed e' una reazione con il trigger dello spostamento, ereditato da `Action.Anchor`"),
		Reazione->Def.Slot == ERTActionSlot::Reaction
		&& Reazione->Def.ReactionTrigger == ERTReactionTrigger::AboutToBeDisplaced);
	Victim->Abilities.Add(Reazione);
	Victim->PlannedReactionAbility = Victim->Abilities.Num() - 1;

	PlanPushOn(Pusher, Victim);
	RunCauseTurn(TM);

	// La prova che conta: la cella. Non un flag, non una voce di log — dove sta l'unita' a fine turno.
	TestEqual(TEXT("l'ancora ha annullato la spinta: il bersaglio e' rimasto dov'era"),
		Victim->Cell, FRTCellId(1, 0));

	const TArray<FRTTurnLogEntry> Resisted = ResistedEntries(TM);
	if (!TestEqual(TEXT("lo spostamento annullato lascia UNA voce nel TurnLog"), Resisted.Num(), 1))
	{
		DestroyCauseWorld(World);
		return false;
	}
	// E dice QUALE difesa ha retto: senza il reason code proprio, un replay non distinguerebbe l'ancora da una
	// guardia o da un bordo mappa — tre modi molto diversi di non essersi mossi.
	TestEqual(TEXT("e dice PERCHE': ancorato, non in guardia ne' senza uscita"),
		static_cast<ERTDisplacementBlockReason>(Resisted[0].Amount),
		ERTDisplacementBlockReason::Anchored);
	TestTrue(FString::Printf(TEXT("la riga leggibile lo dice: %s"),
		*URTTurnLogLibrary::DescribeEntry(Resisted[0])),
		URTTurnLogLibrary::DescribeEntry(Resisted[0]).Contains(TEXT("ancorato")));

	// La reazione si e' ATTIVATA, e la sua voce lo dichiara: senza, l'unita' potrebbe essere rimasta ferma per
	// un motivo qualunque e l'ancora essere ancora carica.
	bool bAttivata = false;
	for (const FRTTurnLogEntry& E : EntriesOfCategory(TM, ERTLogCategory::Reaction))
	{
		if (E.ActionId == FName(TEXT("Reaction.Anchor"))
			&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Activated)
		{
			bAttivata = true;
		}
	}
	TestTrue(TEXT("il TurnLog registra l'ancora come ATTIVATA, non come non scattata"), bAttivata);

	DestroyCauseWorld(World);

	// --- E non si spende per una spinta che la geometria ferma da sola ------------------------------
	// Due attaccanti sullo stesso bersaglio: le forze si annullano (`OpposingForces`, `#420`) e l'unita'
	// resta ferma comunque. L'ancora non deve attivarsi — trovato in code review, quando il valutatore
	// chiedeva «e' in arrivo una spinta?» invece di «e' in arrivo una spinta che mi sposterebbe?».
	// E' la stessa regola di `Reaction.Cleanse`, che resta carica se annullare non cambierebbe nulla.
	{
		UWorld* World2 = MakeCauseWorld();
		if (!TestNotNull(TEXT("World delle forze opposte"), World2)) { return false; }
		SpawnCauseMap(World2);
		ARTUnit* Left   = SpawnCauseUnit(World2, 0, FRTCellId(0, 0));
		ARTUnit* Victim2 = SpawnCauseUnit(World2, 1, FRTCellId(1, 0));
		ARTUnit* Right  = SpawnCauseUnit(World2, 0, FRTCellId(2, 0));
		ARTTurnManager* TM2 = World2->SpawnActor<ARTTurnManager>();
		if (!Left || !Victim2 || !Right || !TM2) { DestroyCauseWorld(World2); return false; }

		URTActionData* Reazione2 = URTCatalogLibrary::MakeEquipmentAction(Anchor, Victim2);
		Victim2->Abilities.Add(Reazione2);
		Victim2->PlannedReactionAbility = Victim2->Abilities.Num() - 1;

		PlanPushOn(Left, Victim2);
		PlanPushOn(Right, Victim2);
		RunCauseTurn(TM2);

		if (!TestTrue(TEXT("premessa: le due spinte si annullano e il bersaglio resta"),
			Victim2->Cell == FRTCellId(1, 0)))
		{
			DestroyCauseWorld(World2);
			return false;
		}

		bool bSprecata = false;
		for (const FRTTurnLogEntry& E : EntriesOfCategory(TM2, ERTLogCategory::Reaction))
		{
			if (E.ActionId == FName(TEXT("Reaction.Anchor"))
				&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Activated)
			{
				bSprecata = true;
			}
		}
		TestFalse(TEXT("l'ancora NON si spende: quella spinta non lo avrebbe spostato"), bSprecata);

		// E la causa registrata resta quella vera: le forze opposte, non un'ancora che non e' intervenuta.
		const TArray<FRTTurnLogEntry> Resisted2 = ResistedEntries(TM2);
		if (Resisted2.Num() == 1)
		{
			TestEqual(TEXT("il TurnLog dice ancora `OpposingForces`"),
				static_cast<ERTDisplacementBlockReason>(Resisted2[0].Amount),
				ERTDisplacementBlockReason::OpposingForces);
		}
		DestroyCauseWorld(World2);
	}

	return true;
}

// =====================================================================================================
// CP 7.5 (`#505`) — `Reaction.Cleanse` annulla il controllo che sta per arrivare.
//
// ⚠️ L'oracolo NON e' `HasStatus` a fine turno: `Status.Root` e `Status.Slow` durano 1 turno e il Cleanup
// dello stesso turno li ha gia' fatti scadere con `TickStatuses`. Un test scritto cosi' e' verde-e-inutile
// (la prima stesura lo era, e a tradirla e' stata la PREMESSA, non il caso). L'oracolo e' l'esito
// osservabile che il controllo doveva produrre: chi e' radicato **non si muove**, chi e' rallentato **si
// muove meno**. E' la stessa nota che `RTControlActionTests.cpp` porta accanto ai suoi test di Root.
//
// Il caso 2 e' quello per cui la specifica e' stata riscritta: con `Root` e `Slow` insieme, annullare «il
// primo raccolto» avrebbe sprecato l'attivazione sullo `Slow` — che arriva anche dall'attacco base con
// `Weapon.Suppressive` — lasciando passare il `Root`, l'unico dei due che fa perdere il turno.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCleanseCancelsControlTest,
	"RefactorTactics.Equipment.Cleanse.CancelsControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCleanseCancelsControlTest::RunTest(const FString&)
{
	const URTEquipmentData* Cleanse = nullptr;
	for (const URTEquipmentData* M : URTCatalogLibrary::MakeReactionModules())
	{
		if (M && M->EquipmentId == FName(TEXT("Reaction.Cleanse"))) { Cleanse = M; break; }
	}
	if (!TestNotNull(TEXT("`Reaction.Cleanse` e' nel catalogo dei moduli"), Cleanse)) { return false; }

	// Passa da `MakeEquipmentAction`, cioe' dal percorso che il gioco usa davvero: un `FRTActionDef` scritto
	// a mano non proverebbe che il modulo eredita il trigger dall'azione core.
	auto EquipCleanse = [&Cleanse](ARTUnit* Unit)
	{
		URTActionData* Reazione = URTCatalogLibrary::MakeEquipmentAction(Cleanse, Unit);
		Unit->Abilities.Add(Reazione);
		Unit->PlannedReactionAbility = Unit->Abilities.Num() - 1;
		return Reazione;
	};

	// Chi subisce il controllo pianifica una corsa lunga: e' cio' che rende osservabili sia il Root (non si
	// muove affatto) sia lo Slow (si muove meno).
	auto PlanLongRun = [](ARTUnit* Unit)
	{
		Unit->PlannedWaypoints = { FRTCellId(0, 1), FRTCellId(0, 2), FRTCellId(0, 3), FRTCellId(0, 4) };
		Unit->PlannedPath = { FRTCellId(0, 0), FRTCellId(0, 1), FRTCellId(0, 2), FRTCellId(0, 3), FRTCellId(0, 4) };
		Unit->PlannedCell = FRTCellId(0, 4);
		Unit->PlannedAbilityIndex = INDEX_NONE;
	};

	auto WasActivated = [](const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : EntriesOfCategory(TM, ERTLogCategory::Reaction))
		{
			if (E.ActionId == FName(TEXT("Reaction.Cleanse"))
				&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Activated)
			{
				return true;
			}
		}
		return false;
	};

	// --- PREMESSA: senza il modulo, il Root inchioda il bersaglio -----------------------------------
	{
		UWorld* World = MakeCauseWorld();
		if (!TestNotNull(TEXT("World della premessa"), World)) { return false; }
		SpawnCauseMap(World);
		ARTUnit* Rooter = SpawnCauseUnit(World, 0, FRTCellId(1, 0));
		ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		if (!Rooter || !Victim || !TM) { DestroyCauseWorld(World); return false; }

		Rooter->PlannedAbilityIndex = AddCoreAbility(Rooter, TEXT("Action.Root"));
		Rooter->PlannedAttackTarget = Victim;
		Rooter->PlannedCell = Rooter->Cell;
		PlanLongRun(Victim);
		RunCauseTurn(TM);

		const bool bFermo = (Victim->Cell == FRTCellId(0, 0));
		DestroyCauseWorld(World);
		if (!TestTrue(TEXT("premessa: senza modulo il Root arriva e inchioda il bersaglio"), bFermo))
		{
			return false; // senza un Root che funziona, i casi sotto non proverebbero niente
		}
	}

	// --- CASO 1: con il modulo, il Root e' annullato e il bersaglio corre ----------------------------
	int32 CelleSenzaSlow = 0;
	{
		UWorld* World = MakeCauseWorld();
		if (!TestNotNull(TEXT("World"), World)) { return false; }
		SpawnCauseMap(World);
		ARTUnit* Rooter = SpawnCauseUnit(World, 0, FRTCellId(1, 0));
		ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		if (!Rooter || !Victim || !TM) { DestroyCauseWorld(World); return false; }

		URTActionData* Reazione = EquipCleanse(Victim);
		if (!TestNotNull(TEXT("il modulo concede un'azione"), Reazione)) { DestroyCauseWorld(World); return false; }
		TestTrue(TEXT("ed e' una reazione col trigger del controllo, ereditato da `Action.Purge`"),
			Reazione->Def.Slot == ERTActionSlot::Reaction
			&& Reazione->Def.ReactionTrigger == ERTReactionTrigger::AboutToReceiveControl);

		Rooter->PlannedAbilityIndex = AddCoreAbility(Rooter, TEXT("Action.Root"));
		Rooter->PlannedAttackTarget = Victim;
		Rooter->PlannedCell = Rooter->Cell;
		PlanLongRun(Victim);
		RunCauseTurn(TM);

		CelleSenzaSlow = URTHexLibrary::HexDistance(FRTCellId(0, 0), Victim->Cell);
		TestTrue(TEXT("il Root e' stato annullato: il bersaglio si e' mosso"), CelleSenzaSlow > 0);
		TestTrue(TEXT("e il TurnLog registra la reazione come ATTIVATA"), WasActivated(TM));
		DestroyCauseWorld(World);
	}
	if (CelleSenzaSlow <= 1)
	{
		AddError(TEXT("il bersaglio deve percorrere piu' di una cella, altrimenti il caso 2 non distingue"));
		return false;
	}

	// --- CASO 2: Root + Slow insieme -> salta il PIU' GRAVE ------------------------------------------
	{
		UWorld* World = MakeCauseWorld();
		if (!TestNotNull(TEXT("World"), World)) { return false; }
		SpawnCauseMap(World);
		ARTUnit* Rooter = SpawnCauseUnit(World, 0, FRTCellId(1, 0));
		ARTUnit* Slower = SpawnCauseUnit(World, 0, FRTCellId(-1, 0));
		ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		if (!Rooter || !Slower || !Victim || !TM) { DestroyCauseWorld(World); return false; }

		EquipCleanse(Victim);
		Rooter->PlannedAbilityIndex = AddCoreAbility(Rooter, TEXT("Action.Root"));
		Rooter->PlannedAttackTarget = Victim;
		Rooter->PlannedCell = Rooter->Cell;
		Slower->PlannedAbilityIndex = AddCoreAbility(Slower, TEXT("Action.Slow"));
		Slower->PlannedAttackTarget = Victim;
		Slower->PlannedCell = Slower->Cell;
		PlanLongRun(Victim);
		RunCauseTurn(TM);

		const int32 CelleConSlow = URTHexLibrary::HexDistance(FRTCellId(0, 0), Victim->Cell);
		// Il cuore del test, in due assert di verso opposto: il Root e' stato annullato (si e' mosso) ma lo
		// Slow no (si e' mosso MENO che senza). Un solo assert non distinguerebbe «ha annullato il Root» da
		// «ha annullato tutto».
		TestTrue(TEXT("il Root, il piu' grave, e' stato annullato: il bersaglio si e' mosso"), CelleConSlow > 0);
		TestTrue(FString::Printf(TEXT("lo Slow invece e' arrivato: %d celle contro %d senza"),
			CelleConSlow, CelleSenzaSlow), CelleConSlow < CelleSenzaSlow);
		DestroyCauseWorld(World);
	}

	// --- CASO 3: gia' sotto quel controllo -> la reazione resta CARICA -------------------------------
	{
		UWorld* World = MakeCauseWorld();
		if (!TestNotNull(TEXT("World"), World)) { return false; }
		SpawnCauseMap(World);
		ARTUnit* Rooter = SpawnCauseUnit(World, 0, FRTCellId(1, 0));
		ARTUnit* Victim = SpawnCauseUnit(World, 1, FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		if (!Rooter || !Victim || !TM) { DestroyCauseWorld(World); return false; }

		EquipCleanse(Victim);
		// Precondizione, non un aggiramento: l'unita' arriva al turno gia' radicata da prima. Il gameplay
		// sotto test e' la REAZIONE, non il modo in cui lo stato ci e' finito.
		Victim->ApplyStatus(TAG_Status_Root, /*Turni*/ 3);

		Rooter->PlannedAbilityIndex = AddCoreAbility(Rooter, TEXT("Action.Root"));
		Rooter->PlannedAttackTarget = Victim;
		Rooter->PlannedCell = Rooter->Cell;
		PlanLongRun(Victim);
		RunCauseTurn(TM);

		// Annullare un rinnovo non cambierebbe nulla di osservabile — resta radicato comunque — e l'unica
		// attivazione del turno vale piu' di cosi'.
		TestFalse(TEXT("con il Root gia' addosso la reazione NON si attiva"), WasActivated(TM));
		TestTrue(TEXT("e infatti resta inchiodato"), Victim->Cell == FRTCellId(0, 0));
		DestroyCauseWorld(World);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
