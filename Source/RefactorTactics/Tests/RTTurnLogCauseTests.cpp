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
		U->ConfigureAsArchetype(ERTArchetype::Ranger);
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

#endif // WITH_DEV_AUTOMATION_TESTS
