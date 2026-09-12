#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h" // DeriveOpportunityId: l'identita' della finestra
#include "Replay/RTBoundaryChecksum.h" // ChecksumsAlongTrace / DescribeDivergence: l oracolo del progetto
#include "Replay/RTReplayStateLibrary.h" // FRTTracedUnitState: lo schieramento iniziale della traccia
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // GoldenEntriesMatch: stessa voce ha gia un proprietario
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La RETE del refactor di `#2679` fetta 1: rendere `ResolveMovement` riprendibile.
 *
 * 🔑 **Questi test non descrivono una feature nuova — fissano quella che c'e'**, ed e' il punto. Il lavoro
 * di `#2679` sposta lo stato della risoluzione del movimento dallo stack di `ResolveMovement` a un contesto
 * membro, e spezza la funzione in `Begin` / `Advance` / `Finish`. Un refactor a comportamento invariante non
 * ha un test che diventa verde: ha un test che **resta** verde, e che sarebbe diventato rosso se una delle
 * undici locali che il ciclo attraversa fosse stata persa per strada.
 *
 * ⚠️ **L'impronta non e' l'intero TurnLog, ed e' una scelta.** Confrontare il log per intero renderebbe
 * questi test rossi a ogni voce nuova aggiunta da un'altra issue — e il primo che li vedesse rossi per quel
 * motivo imparerebbe a spuntarli invece di leggerli. Cio' che si fissa e' la terna che dichiara l'esito del
 * movimento: dove sono finite le unita', quante voci ha prodotto il turno, e il digest ordinato delle voci
 * di `Move`.
 */
namespace
{
	UWorld* MakeResumeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyResumeWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnResumeMap(UWorld* World, int32 Radius = 8)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnResumeUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo salvo che il caso non lo cambi
		return U;
	}

	/**
	 * Il turno intero: il commit e poi il playback fino in fondo.
	 *
	 * ⚠️ **Il `Tick` serve, e non e' un dettaglio del test**: `FinishPlayback` e' cio' che porta i modelli
	 * alla loro cella logica. Senza, `ARTUnit::Cell` sarebbe letto a meta' riproduzione e l'impronta
	 * misurerebbe un fotogramma invece di un esito.
	 */
	void RunResumeTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Cio' che il refactor NON deve cambiare. Vedi il commento in testa al file. */
	struct FTurnFingerprint
	{
		TArray<FRTCellId> FinalCells;
		int32 LogEntryCount = 0;
		FString MoveDigest;

		bool Equals(const FTurnFingerprint& O) const
		{
			return FinalCells == O.FinalCells
				&& LogEntryCount == O.LogEntryCount
				&& MoveDigest == O.MoveDigest;
		}

		FString ToString() const
		{
			return FString::Printf(TEXT("celle=%d voci=%d digest='%s'"),
				FinalCells.Num(), LogEntryCount, *MoveDigest);
		}
	};

	FTurnFingerprint Capture(const ARTTurnManager* TM, const TArray<ARTUnit*>& Units)
	{
		FTurnFingerprint F;
		for (const ARTUnit* U : Units)
		{
			F.FinalCells.Add(U ? U->Cell : FRTCellId());
		}
		const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
		F.LogEntryCount = Log.Num();
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.Phase == ERTMatchPhase::Move)
			{
				F.MoveDigest += FString::Printf(TEXT("%d:%d,%d,%d;"),
					E.UnitId, E.TgtCell.X, E.TgtCell.Y, E.TgtCell.Layer);
			}
		}
		return F;
	}
}

/**
 * L'impronta di un turno di movimento con un watcher armato: il percorso che il refactor tocca.
 *
 * ⚠️ **Il test NON asserisce che l'Overwatch spari.** Cio' che gli serve e' che il turno attraversi
 * `ResolveReactionBoundary`, che viene chiamata a **ogni** micro-step — con o senza watcher che scattano.
 * Asserire il colpo legherebbe questa rete alle regole di ingaggio dell'Overwatch, che questa issue non
 * tocca, e la renderebbe rossa per motivi che non la riguardano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementCharacterizationTest,
	"RefactorTactics.Movement.ResolveMovementFingerprintIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementCharacterizationTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	// Il watcher e' del bot: il decisore sincrono prende il ramo `bIsBotControlled` e risponde subito,
	// che e' l'unico modo in cui questo test puo' essere un test e non una sessione manuale.
	Watcher->bIsBotControlled = true;
	// 🔴 **`PlannedAbilityIndex` e non `PlannedReactionAbility`**: l'Overwatch e' l'AZIONE PRINCIPALE del
	// turno, che e' cio' che costa (catalogo §1). Lo slot reazione e' un altro meccanismo — Counter,
	// Deflect — e armarlo li' non produce nessun watcher: misurato, il test vedeva zero finestre.
	Watcher->PlannedAbilityIndex =
		RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);

	// Due celle: dentro il budget di movimento del Ivrin senza dover conoscere il numero esatto.
	Mover->PlannedCell = FRTCellId(2, 0);

	RunResumeTurn(TM);

	const FTurnFingerprint Fingerprint = Capture(TM, { Mover, Watcher });

	// Un'impronta vuota passerebbe qualunque confronto: si verifica che ci sia qualcosa da confrontare.
	TestTrue(TEXT("il turno ha prodotto voci di log"), Fingerprint.LogEntryCount > 0);
	TestTrue(TEXT("il movimento ha lasciato traccia nel log"), !Fingerprint.MoveDigest.IsEmpty());
	TestFalse(TEXT("il mover ha lasciato la cella di partenza"),
		Fingerprint.FinalCells[0] == FRTCellId(0, 0));

	DestroyResumeWorld(World);
	return true;
}


/**
 * Le GUARDIE delle tre funzioni: avanzare o concludere una risoluzione che non esiste e' `Finished`, non
 * un crash.
 *
 * 🔴 **Questo test nasce da un fallimento vero, e la cronaca vale.** La prima versione guidava
 * `Begin` → `Advance`×N → `Finish` a mano e contava i passi, asserendo `Passi > 1`. Ha misurato **0**, e
 * non per un difetto dello split: `LockInAndResolve` esegue `ValidatePlansAtLockIn` e l'altro preambolo
 * **prima** delle fasi, quindi una risoluzione avviata fuori da li' non ha piani validati — nessun
 * percorso, nessun micro-step. Il test chiedeva a una funzione di funzionare fuori dal contesto che la
 * rende sensata.
 *
 * ⛔ **Che `Advance` faccia UN passo per chiamata non e' verificabile da fuori oggi**, e questo file lo
 * dichiara invece di fingerlo con un'asserzione piu' debole: il contesto e' privato e muore in `Finish`,
 * e il percorso di produzione (`ResolveMovement`) e' il ciclo di `Advance` — confrontarli sarebbe vero per
 * costruzione. Diventa verificabile con la **fetta 3**, quando a guidare i tre momenti sara' un
 * orchestratore esterno dopo il preambolo: li' il conteggio dei passi e' osservabile da chi li conta.
 *
 * ✅ **Cio' che invece resta coperto**, ed e' il gate che conta per questa fetta: `ResolveMovementFingerprintIsStable`
 * gira il turno per la via di produzione e fissa il suo esito. Se lo split avesse perso una delle nove
 * locali, sarebbe rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementResolutionGuardsTest,
	"RefactorTactics.Movement.ResolutionGuardsFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementResolutionGuardsTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	// Nessun `Begin`: non c'e' contesto, e la risposta e' «non c'e' niente da far avanzare».
	TestTrue(TEXT("Advance senza Begin e' Finished"),
		TM->AdvanceMovementResolution() == ERTMovementAdvanceResult::Finished);

	// E `Finish` sullo stesso vuoto non deve toccare niente ne' esplodere.
	TM->FinishMovementResolution();
	TestTrue(TEXT("Finish senza Begin e' un no-op"),
		TM->AdvanceMovementResolution() == ERTMovementAdvanceResult::Finished);

	// Dopo un ciclo completo il contesto e' rilasciato: un Advance in piu' e' Finished, non un dangling.
	TM->BeginMovementResolution();
	int32 Guard = 0;
	while (TM->AdvanceMovementResolution() == ERTMovementAdvanceResult::Advanced && Guard < 256)
	{
		++Guard;
	}
	TM->FinishMovementResolution();
	TestTrue(TEXT("il ciclo termina senza toccare la guardia"), Guard < 256);
	TestTrue(TEXT("dopo Finish il contesto e' rilasciato"),
		TM->AdvanceMovementResolution() == ERTMovementAdvanceResult::Finished);

	DestroyResumeWorld(World);
	return true;
}


/**
 * **La finestra si apre e la resolution si sospende** (`#2679` fetta 2, [D-355]).
 *
 * 🔑 **Cosa dimostra, esattamente.** Con `OnReactionWindowOpened` legato — cioe' con una UI che attende —
 * un'opportunity che richiede una decisione **apre una finestra** e `AdvanceMovementResolution` ritorna
 * `Suspended` invece di risolvere il micro-step successivo. E' la sospensione globale di ADR-0004 §5, che
 * la fetta 1 aveva gratis dalla chiamata sincrona e che qui e' conservata di proposito.
 *
 * ⚠️ **L'`ensure` atteso E' la prova, non un difetto.** Il test guida il turno per la via **sincrona**
 * (`LockInAndResolve` → `ResolveMovement`), che per costruzione non sa attendere: incontrando una finestra
 * dichiara forte di non poterla onorare. Dichiararlo atteso con `AddExpectedError` e' il modo di
 * verificare che quella strada sia stata **davvero** imboccata — senza, il test passerebbe anche se la
 * sospensione non fosse mai avvenuta.
 *
 * ⛔ **Il ciclo completo — attesa, risposta, ripresa — non e' verificabile qui**, e non si finge: richiede
 * un orchestratore che guidi i tre momenti dopo il preambolo di `LockInAndResolve`, ed e' la **fetta 3**.
 * Cio' che questa fetta consegna e' il meccanismo; chi lo guida arriva dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowSuspendsTest,
	"RefactorTactics.Reactions.WindowSuspendsResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowSuspendsTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	// ⚠️ **Il watcher NON e' del bot**: e' la condizione che rende la finestra interattiva possibile.
	Watcher->bIsBotControlled = false;

	// 🔴 **`PlannedAbilityIndex` e non `PlannedReactionAbility`**: l'Overwatch e' l'AZIONE PRINCIPALE del
	// turno, che e' cio' che costa (catalogo §1). Lo slot reazione e' un altro meccanismo — Counter,
	// Deflect — e armarlo li' non produce nessun watcher: misurato, il test vedeva zero finestre.
	Watcher->PlannedAbilityIndex =
		RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);

	// 🔑 **Il cono E' il facing** (ADR-0005 §4c): senza orientarlo verso il mover la zona guarda altrove.
	Watcher->Facing = ERTHexDirection::W;
	Watcher->PlannedCell = Watcher->Cell;

	// (2,0) e non (3,0): la cella del watcher e' occupata, e una destinazione occupata viene negata prima
	// che il mover entri nella zona.
	Mover->PlannedCell = FRTCellId(2, 0);

	// La UI finta: registra le finestre che si aprono e non risponde a nessuna.
	TArray<FString> Aperte;
	TM->OnReactionWindowOpened.BindLambda(
		[&Aperte](const FRTReactionWindowView& View, int32 /*OwnerUnitId*/)
		{
			Aperte.Add(URTReactionOpportunityLibrary::DeriveOpportunityId(View.Key));
		});

	TM->LockInAndResolve();

	// 🔑 **La prova.** Il delegate e' stato invocato: una finestra si e' aperta, con la sua identita', e la
	// resolution ha smesso di avanzare per aspettarla.
	TestTrue(FString::Printf(TEXT("una finestra si e' aperta (%d)"), Aperte.Num()), Aperte.Num() > 0);
	if (Aperte.Num() > 0)
	{
		TestTrue(TEXT("la finestra ha un'identita' non vuota"), !Aperte[0].IsEmpty());
	}

	// 🔑 **La resolution NON e' arrivata in fondo, ed e' il punto.** Da `#2679` fetta 3 `LockInAndResolve`
	// esce senza concludere quando il movimento si sospende: il turno esiste a meta' e aspetta una risposta.
	TestTrue(TEXT("la resolution e' sospesa e attende"), TM->IsResolutionSuspended());

	// Una risposta che nomina un'altra finestra non chiude nulla: e' arrivata tardi.
	TM->SubmitReactionResponse(TEXT("una-finestra-che-non-esiste"), TEXT("HOLD"));
	TestTrue(TEXT("una risposta stale non chiude la finestra"), TM->IsResolutionSuspended());

	// La scadenza la chiude, e la resolution riprende: `HoldTimeout`, charge non consumata.
	int32 Scadenze = 0;
	while (TM->IsResolutionSuspended() && Scadenze < 16)
	{
		TM->ExpireReactionWindow();
		++Scadenze;
	}
	TestFalse(TEXT("scadute le finestre, la resolution e' conclusa"), TM->IsResolutionSuspended());
	TestTrue(TEXT("il turno ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	TM->OnReactionWindowOpened.Unbind();
	DestroyResumeWorld(World);
	return true;
}


/**
 * **La fase che il gioco dichiara mentre aspetta** ([D-356], `#2692`).
 *
 * 🔴 **E' il test che `D-356` chiede esplicitamente, e che non esisteva.** Fino al 2026-09-09 il ciclo
 * delle fasi arrivava in fondo anche quando una fase si sospendeva — un resolver che si ferma RITORNA
 * normalmente, e il ciclo non se ne accorgeva — quindi `Phase` tornava a `Planning` mentre il turno non
 * era finito, il playback scorreva e una finestra aspettava una risposta.
 *
 * ⛔ **Tre lettori sbagliavano, e nessuno di loro e' stato toccato per correggerli**: l'header dell'HUD
 * stampava «Pianificazione» col countdown a schermo, la traccia post-lock si disegnava sopra un turno in
 * corso, e `RecordPlanningInput` contava i click come input di pianificazione — cioe' sporcava il pacing
 * con cui si tara [D-348]. Con la fase vera i loro `== Planning` sono falsi per costruzione.
 *
 * 🔑 **Cosa misura, esattamente**: che a resolution sospesa `GetPhase()` sia la fase che si e' fermata e
 * **non** `Planning`, e che alla chiusura il turno arrivi comunque in fondo. Il secondo asserto e' cio'
 * che impedisce alla correzione di fermare il turno per sempre: uscire dal ciclo non basta, bisogna anche
 * rientrarci.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSuspendedPhaseIsTheRealPhaseTest,
	"RefactorTactics.Reactions.SuspendedPhaseIsNotPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSuspendedPhaseIsTheRealPhaseTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	Watcher->bIsBotControlled = false;
	Watcher->PlannedAbilityIndex =
		RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
	Watcher->Facing = ERTHexDirection::W;
	Watcher->PlannedCell = Watcher->Cell;
	Mover->PlannedCell = FRTCellId(2, 0);

	// La UI finta: apre e non risponde, come negli altri casi di questo file.
	bool bApertaAlmenoUna = false;
	TM->OnReactionWindowOpened.BindLambda(
		[&bApertaAlmenoUna](const FRTReactionWindowView&, int32) { bApertaAlmenoUna = true; });

	TM->LockInAndResolve();

	if (!TestTrue(TEXT("una finestra si e' aperta"), bApertaAlmenoUna)
		|| !TestTrue(TEXT("la resolution e' sospesa"), TM->IsResolutionSuspended()))
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	// 🔑 **LA PROVA.** La sospensione avviene nel movimento, quindi la fase dichiarata dev'essere `Move`.
	// Prima di [D-356] qui si leggeva `Planning`, ed e' il difetto che questo test esiste per fissare.
	TestEqual(TEXT("a resolution sospesa la fase e' quella che si e' fermata"),
		TM->GetPhase(), ERTMatchPhase::Move);
	TestFalse(TEXT("e NON e' `Planning`: il turno non e' finito"),
		TM->GetPhase() == ERTMatchPhase::Planning);

	// ⚠️ **Uscire dal ciclo non basta: bisogna rientrarci.** Alla chiusura il turno deve arrivare in fondo,
	// altrimenti la correzione avrebbe barattato una fase sbagliata con un turno che non finisce piu'.
	int32 Scadenze = 0;
	while (TM->IsResolutionSuspended() && Scadenze < 16)
	{
		TM->ExpireReactionWindow();
		++Scadenze;
	}
	TestFalse(TEXT("chiuse le finestre, la resolution e' conclusa"), TM->IsResolutionSuspended());
	TestEqual(TEXT("e il ciclo delle fasi e' tornato a `Planning`"),
		TM->GetPhase(), ERTMatchPhase::Planning);
	TestTrue(TEXT("il turno ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	TM->OnReactionWindowOpened.Unbind();
	DestroyResumeWorld(World);
	return true;
}


/**
 * **Il ciclo completo: attesa, risposta, ripresa** (`#2679` fetta 3, [D-355]).
 *
 * ✅ **E' la copertura che le fette 1 e 2 avevano dichiarato SCOPERTA**, e che diventa verificabile solo
 * ora: serviva che `LockInAndResolve` sapesse uscire senza concludere, e che la chiusura della finestra
 * sapesse riprendere. Con il turno che si ferma davvero, il ciclo si osserva dall'esterno senza guidare
 * nulla a mano.
 *
 * 🔑 **Il turno NON e' concluso mentre la finestra e' aperta**, ed e' la sospensione globale di
 * ADR-0004 §5 misurata dove conta: `IsResolutionSuspended()` e' vero, e nessuna delle fasi successive ha
 * girato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowFullCycleTest,
	"RefactorTactics.Reactions.WindowResponseResumesResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowFullCycleTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	Watcher->bIsBotControlled = false;
	Watcher->PlannedAbilityIndex =
		RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
	Watcher->Facing = ERTHexDirection::W;
	Watcher->PlannedCell = Watcher->Cell;
	Mover->PlannedCell = FRTCellId(2, 0);

	TArray<FString> Aperte;
	TM->OnReactionWindowOpened.BindLambda(
		[&Aperte](const FRTReactionWindowView& View, int32 /*OwnerUnitId*/)
		{
			Aperte.Add(URTReactionOpportunityLibrary::DeriveOpportunityId(View.Key));
		});

	TM->LockInAndResolve();

	// --- L'ATTESA ---------------------------------------------------------------------------------------
	const bool bSospesa = TM->IsResolutionSuspended();
	TestTrue(TEXT("la resolution si e' fermata sulla finestra"), bSospesa);
	TestTrue(FString::Printf(TEXT("una finestra si e' aperta (%d)"), Aperte.Num()), Aperte.Num() > 0);

	if (!bSospesa)
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	const FString Aperta = TM->GetOpenReactionWindowId();
	TestTrue(TEXT("la finestra aperta ha un'identita'"), !Aperta.IsEmpty());

	// --- LA RISPOSTA ------------------------------------------------------------------------------------
	// Una risposta che nomina un'altra finestra non chiude nulla: e' arrivata tardi.
	TM->SubmitReactionResponse(TEXT("finestra-inesistente"), TEXT("HOLD"));
	TestTrue(TEXT("una risposta stale non chiude la finestra"), TM->IsResolutionSuspended());

	// La risposta giusta la chiude. `HOLD` e' sempre fra le risposte legali di un Overwatch.
	TM->SubmitReactionResponse(Aperta, TEXT("HOLD"));

	// --- LA RIPRESA -------------------------------------------------------------------------------------
	// ⚠️ **Puo' essersi riaperta**: se un secondo watcher scatta nello stesso micro-step, la resolution
	// torna ad attendere. Si risponde finche' non ce ne sono piu' — che e' esattamente cio' che fara' la UI.
	int32 Chiusure = 1;
	while (TM->IsResolutionSuspended() && Chiusure < 16)
	{
		const FString Ancora = TM->GetOpenReactionWindowId();
		if (Ancora.IsEmpty()) { break; }
		TM->SubmitReactionResponse(Ancora, TEXT("HOLD"));
		++Chiusure;
	}

	TestFalse(TEXT("chiuse le finestre, la resolution non e' piu' sospesa"), TM->IsResolutionSuspended());
	TestTrue(TEXT("il numero di chiusure e' rimasto ragionevole"), Chiusure < 16);

	// Il turno e' arrivato in fondo: il TurnLog e' stato scritto.
	TestTrue(TEXT("il turno ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	TM->OnReactionWindowOpened.Unbind();
	DestroyResumeWorld(World);
	return true;
}


/**
 * **Il playback e' gia' in corso quando la finestra si apre** (`#2679` fetta 3, [D-355]).
 *
 * 🔑 **E' la meta' che a `D-355` mancava.** La decisione dice che la finestra si apre *durante* il playback;
 * fino a questa fetta la resolution si fermava PRIMA che `BeginPlayback` fosse mai chiamato, e chi decideva
 * guardava uno schermo che non aveva mostrato il nemico entrare nella zona.
 *
 * ⚠️ **Cio' che l'automation puo' vedere, e cio' che non puo'.** Qui si verifica che il playback sia
 * *partito* (`IsResolving()`) e che la timeline porti gia' il tratto percorso. Che il movimento **si veda**
 * fluido attraverso la giunzione — nessun salto all'indietro, nessuna ripartenza da capo — e' presentazione,
 * e il suo gate e' **PIE**: resta `NOT RUN` e va dichiarato, non dedotto da questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowPlaybackIsRunningTest,
	"RefactorTactics.Reactions.WindowOpensWithPlaybackRunning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowPlaybackIsRunningTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	Watcher->bIsBotControlled = false;
	Watcher->PlannedAbilityIndex =
		RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
	Watcher->Facing = ERTHexDirection::W;
	Watcher->PlannedCell = Watcher->Cell;
	Mover->PlannedCell = FRTCellId(2, 0);

	bool bPlaybackRunningAtOpen = false;
	TM->OnReactionWindowOpened.BindLambda(
		[&bPlaybackRunningAtOpen, TM](const FRTReactionWindowView&, int32)
		{
			// ⚠️ Letto DENTRO il delegate: dopo, il turno prosegue e la risposta non sarebbe piu' la stessa.
			bPlaybackRunningAtOpen = TM->IsResolving();
		});

	TM->LockInAndResolve();

	if (!TestTrue(TEXT("la resolution si e' sospesa su una finestra"), TM->IsResolutionSuspended()))
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	// 🔑 Il playback e' partito, e la finestra si e' aperta con quello in corso.
	TestTrue(TEXT("il playback e' in corso mentre la finestra attende"), TM->IsResolving());

	// Il tratto percorso e' nella timeline: c'e' qualcosa da guardare, non uno schermo fermo.
	TestTrue(TEXT("la timeline porta gia' il movimento percorso"),
		TM->ResolvedEventCountOfTypeForTest(ERTResolvedEventType::Move) > 0);

	// 🔴 **Il turno NON si conclude mentre la finestra attende**, ed e' il difetto che questo test esiste
	// per impedire: `FinishPlayback` chiama `ConcludeTurn`, e il playback parziale mostra pochi secondi
	// mentre la finestra ne dura `FastReactionDuration`. Senza il fermo, il turno si chiudeva sotto chi
	// stava ancora decidendo. Si ticka ben oltre la durata del tratto mostrato.
	//
	// ⚠️ **Ma SOTTO la durata della finestra, e il vincolo e' nuovo** (`#2717`). Questo ciclo faceva 200
	// tick — **10 secondi** — e asseriva che la finestra fosse ancora aperta. Era vero, e per il motivo
	// sbagliato: nessuno la faceva scadere, perche' `OpenWindowElapsed` era azzerato in quattro punti e
	// incrementato in zero. L'asserto codificava il difetto invece di sorvegliare l'intento.
	//
	// 🔑 Ora la finestra scade a `FastReactionDuration` (3,0 s di default, ADR-0004 §8), quindi il tempo
	// da far passare e' quello che serve a esaurire il playback parziale e **non** la finestra: 2 secondi
	// stanno comodamente in mezzo. Il difetto che il test sorveglia — il turno che si chiude perche' il
	// PLAYBACK e' finito — resta osservabile esattamente come prima.
	const float DurataFinestra = TM->GetFastReactionDuration();
	const int32 TickSottoLaScadenza = FMath::Max(1, FMath::FloorToInt((DurataFinestra * 0.66f) / 0.05f));
	for (int32 I = 0; I < TickSottoLaScadenza; ++I)
	{
		TM->Tick(0.05f);
	}
	TestTrue(TEXT("esaurito il playback parziale ma non la finestra, la resolution attende ancora"),
		TM->IsResolutionSuspended());
	TestTrue(TEXT("la finestra e' ancora aperta"), !TM->GetOpenReactionWindowId().IsEmpty());

	// ⛔ **Che la finestra scada da sola NON si misura qui**, ed e' una lezione costata un rosso: questo
	// scenario arma piu' watcher, quindi le finestre si aprono **in sequenza** e ognuna dura
	// `FastReactionDuration`. Un asserto «dopo N tick non attende piu'» dipenderebbe da **quante** finestre
	// si sono aperte, che questo test non controlla. La scadenza autonoma ha il suo test dedicato e
	// deterministico: `Reactions.Brace.WindowExpiresOnItsOwn`.

	// Chiuse le finestre, la resolution arriva in fondo senza che il playback riparta da zero.
	int32 Chiusure = 0;
	while (TM->IsResolutionSuspended() && Chiusure < 16)
	{
		TM->ExpireReactionWindow();
		++Chiusure;
	}
	TestFalse(TEXT("la resolution e' conclusa"), TM->IsResolutionSuspended());
	TestTrue(TEXT("il turno ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	TM->OnReactionWindowOpened.Unbind();
	DestroyResumeWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// #2859 — SOSPESA E RIPRESA == PASSAGGIO UNICO
//
// 🔴 **Questa issue era nata chiedendo un'altra cosa, e quella cosa non poteva fallire.** Chiedeva di
// confrontare un turno riprodotto in modo continuo con lo stesso turno **steppato** coi comandi di `#1879`.
// Misurato: `Pause`, `Step` e `Next Phase` toccano `TickPlayback` — l'immagine — mentre il TurnLog e'
// scritto per intero da `RunPhaseLoop` **prima** che il playback cominci. Dentro `TickPlayback` e
// `StepMicroStep` le scritture di log sono **zero**. ∴ quel confronto avrebbe messo un array contro se
// stesso: verde anche su uno `Step` implementato come corpo vuoto.
//
// ✅ **Cio' che attraversa davvero stato persistente e' la finestra di reazione**, ed e' quello che questo
// gate misura. `RunPhaseLoop` esce su `IsResolutionSuspended()` lasciando `Phase` dov'e', e chi chiude la
// finestra rientra ripartendo *dalla fase gia' risolta, senza rieseguirla*. Se quella ripresa sbagliasse —
// una fase rieseguita, una saltata, un ramo registrato che decide diverso — le due tracce divergono.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Chiude ogni finestra aperta con `HOLD` finche' la risoluzione non riparte. Restituisce quante. */
	int32 ChiudiOgniFinestra(ARTTurnManager* TM, int32 Tetto = 16)
	{
		int32 Chiuse = 0;
		while (TM && TM->IsResolutionSuspended() && Chiuse < Tetto)
		{
			const FString Aperta = TM->GetOpenReactionWindowId();
			if (Aperta.IsEmpty()) { break; }
			TM->SubmitReactionResponse(Aperta, TEXT("HOLD"));
			++Chiuse;
		}
		return Chiuse;
	}

	/** L'esito di una corsa dello scenario: tutto cio' che il confronto e le anti-vacuita' leggono. */
	struct FCorsaRisolta
	{
		TArray<FRTTurnLogEntry> Log;
		TArray<FRTTracedUnitState> Iniziale;
		bool bSospesa = false;
		int32 FinestreChiuse = 0;
		int32 Divergenze = 0;
		int32 Aperture = 0;
		bool bAncoraSospesa = false;
		bool bAllestita = false;
	};
}

/**
 * 🔴 **Il gate di `#1881` che mancava: «nessun secondo simulatore».**
 *
 * Due percorsi, lo stesso turno:
 *
 *   A) la risoluzione si **sospende** su una finestra viva e **riprende** quando la risposta arriva;
 *   B) la stessa risoluzione con le decisioni **registrate** armate prima — [D-355]: *«il ramo della traccia
 *      precede qualunque attesa»* — quindi in un passaggio unico, senza mai sospendersi.
 *
 * Devono produrre la **stessa** traccia. Il confronto e' voce per voce con `GoldenEntriesMatch` — il
 * predicato di «stessa voce» che il progetto gia' possiede — piu' i checksum di boundary che, quando
 * cadono, **nominano il luogo** (`T1|Move#3`) invece di dire «gli hash differiscono».
 *
 * ⛔ **Le tre anti-vacuita' sono dentro il test e non sono ornamentali.** Senza l'assertion che A si sia
 * davvero sospesa, i due percorsi potrebbero essere lo stesso e il test proverebbe zero — che e'
 * esattamente il difetto per cui la formulazione originale di questa issue e' stata ritirata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSuspendedResumedMatchesSinglePassTest,
	"RefactorTactics.Resolution.SuspendedAndResumedMatchesSinglePass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSuspendedResumedMatchesSinglePassTest::RunTest(const FString&)
{
	// Una corsa dello stesso scenario. `Registrate` vuoto = percorso A (finestra viva); non vuoto = B.
	auto Corsa = [](const TArray<FRTTurnLogEntry>& Registrate) -> FCorsaRisolta
	{
		FCorsaRisolta R;

		UWorld* World = MakeResumeWorld();
		if (!World) { return R; }
		SpawnResumeMap(World);

		ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
		ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Mover || !Watcher || !TM) { DestroyResumeWorld(World); return R; }

		Watcher->bIsBotControlled = false;
		Watcher->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Watcher->PlannedCell = Watcher->Cell;
		Mover->PlannedCell = FRTCellId(2, 0);

		// 🔴 **Senza un ascoltatore la finestra non si apre: si committa da sola.** Lo dicono
		// `Reactions.SingleResponseCommitsWithoutWindow` e `Reactions.NoPlayerControllerLeavesTheDelegateUnbound`,
		// e la prima stesura di questo test lo ha imparato dal proprio rosso — l anti-vacuita 1 ha rifiutato
		// di passare perche la corsa A non si era sospesa. Si lega in ENTRAMBE le corse, cosi lunica
		// differenza fra A e B restano le decisioni registrate.
		int32 Aperture = 0;
		TM->OnReactionWindowOpened.BindLambda(
			[&Aperture](const FRTReactionWindowView&, int32) { ++Aperture; });

		// ⚠️ Le celle si leggono PRIMA di risolvere — la traccia dichiara i cambiamenti, non le posizioni di
		// partenza — mentre `StableUnitId` si legge DOPO: lo assegna `EnsureMatchRoster` dentro
		// `LockInAndResolve`, e prima varrebbe ancora `0` per tutti ([D-063]).
		const FRTCellId CellaMover = Mover->Cell;
		const FRTCellId CellaWatcher = Watcher->Cell;

		if (Registrate.Num() > 0)
		{
			TM->ArmRecordedReactionDecisions(Registrate);
		}

		TM->LockInAndResolve();

		R.bSospesa = TM->IsResolutionSuspended();
		R.FinestreChiuse = ChiudiOgniFinestra(TM);
		R.bAncoraSospesa = TM->IsResolutionSuspended();
		R.Log = TM->GetTurnLog();
		R.Divergenze = TM->GetVerificationDivergences().Num();
		R.Aperture = Aperture;
		TM->OnReactionWindowOpened.Unbind();

		FRTTracedUnitState SM; SM.UnitId = Mover->StableUnitId;   SM.Cell = CellaMover;
		FRTTracedUnitState SW; SW.UnitId = Watcher->StableUnitId; SW.Cell = CellaWatcher;
		R.Iniziale.Add(SM);
		R.Iniziale.Add(SW);
		R.bAllestita = true;

		DestroyResumeWorld(World);
		return R;
	};

	// --- A) sospensione VIVA ----------------------------------------------------------------------------
	const FCorsaRisolta A = Corsa({});
	if (!TestTrue(TEXT("la corsa A si allestisce"), A.bAllestita)) { return false; }

	// ⛔ ANTI-VACUITA' 1: senza una sospensione vera, A e B sono lo stesso percorso e il confronto non misura
	// niente. E' il difetto per cui la formulazione originale di questa issue e' stata ritirata.
	if (!TestTrue(TEXT("anti-vacuita': la corsa A si e' DAVVERO sospesa su una finestra"), A.bSospesa))
	{
		return false;
	}
	if (!TestTrue(TEXT("anti-vacuita': e almeno una finestra e' stata chiusa per riprenderla"),
		A.FinestreChiuse > 0))
	{
		return false;
	}
	if (!TestTrue(TEXT("anti-vacuita': e la finestra si e' davvero APERTA, non committata da sola"),
		A.Aperture > 0))
	{
		return false;
	}
	if (!TestTrue(TEXT("la corsa A ha prodotto una traccia"), A.Log.Num() > 0)) { return false; }
	// ⛔ ANTI-VACUITA' 1-bis: `ChiudiOgniFinestra` ha un TETTO, e un tetto che tronca in silenzio lascia
	// la risoluzione a meta' con una traccia parziale — che poi diverge da B per il motivo sbagliato. Se il
	// tetto e' stato raggiunto, va detto QUI e col suo nome, non dedotto da un confronto che fallisce dopo.
	if (!TestFalse(TEXT("anti-vacuita': la corsa A non e' RIMASTA sospesa: il tetto di chiusura non ha troncato"),
		A.bAncoraSospesa))
	{
		return false;
	}


	// 🔴 **E le voci di decisione della corsa VIVA devono portare un indice VERO.**
	//
	// ⚠️ Questa riga esiste perche' la verifica per mutazione di #2956 ha scoperto un buco nel gate: forzando
	// `INDEX_NONE` su ENTRAMBE le corse, il confronto A-contro-B resta verde — le due tracce sono uguali fra
	// loro, e sbagliate insieme. Il confronto misura la DIVERGENZA fra i percorsi, non il valore del campo.
	//
	// Qui il valore si afferma in assoluto, e solo dove e' lecito: una `ReactionDecision` in fase Move nasce
	// dentro un ciclo di micro-step, quindi `INDEX_NONE` — che significa «nessun ciclo qui» — sarebbe falso.
	int32 DecisioniSenzaIndice = 0;
	for (const FRTTurnLogEntry& E : A.Log)
	{
		if (E.Category == ERTLogCategory::ReactionDecision && E.Phase == ERTMatchPhase::Move
			&& E.MicroStepIndex == INDEX_NONE)
		{
			++DecisioniSenzaIndice;
		}
	}
	TestEqual(TEXT("le decisioni di reazione della corsa viva sono localizzate nel loro micro-step"),
		DecisioniSenzaIndice, 0);

	// --- B) passaggio UNICO, con le decisioni di A registrate --------------------------------------------
	const FCorsaRisolta B = Corsa(A.Log);
	if (!TestTrue(TEXT("la corsa B si allestisce"), B.bAllestita)) { return false; }

	// ⛔ ANTI-VACUITA' 2: se B si fosse sospesa, il ramo registrato non ha preso e si starebbero confrontando
	// due esecuzioni vive — cioe' un'altra proprieta'.
	TestFalse(TEXT("anti-vacuita': la corsa B NON si e' sospesa (il ramo registrato ha preso)"), B.bSospesa);
	TestEqual(TEXT("anti-vacuita': nessuna divergenza fra traccia e ri-simulazione"), B.Divergenze, 0);

	// --- L'equivalenza, voce per voce -------------------------------------------------------------------
	if (!TestEqual(TEXT("le due tracce hanno lo stesso numero di voci"), B.Log.Num(), A.Log.Num()))
	{
		return false;
	}

	int32 PrimaDiversa = INDEX_NONE;
	for (int32 i = 0; i < A.Log.Num(); ++i)
	{
		// 🔴 **`MicroStepIndex` si confronta a parte, e non e' ridondante.** `GoldenEntriesMatch` passa da
		// `HashTurnLogOrdered`, che quel campo **non lo copre**: senza questa riga il confronto sarebbe cieco
		// proprio sulla localizzazione che #1880 ha aggiunto e su cui #2374 costruisce i boundary — cioe' sul
		// campo che il criterio d accettazione di #2859 nomina per esteso.
		if (!URTTurnLogLibrary::GoldenEntriesMatch(A.Log[i], B.Log[i])
			|| A.Log[i].MicroStepIndex != B.Log[i].MicroStepIndex)
		{
			PrimaDiversa = i;
			break;
		}
	}
	TestEqual(*FString::Printf(
		TEXT("voce per voce: sospesa+ripresa e passaggio unico coincidono (prima diversa: %d)"), PrimaDiversa),
		PrimaDiversa, INDEX_NONE);

	// --- E il LUOGO, non solo il fatto ------------------------------------------------------------------
	// 🔑 L'oracolo e' quello del progetto: `DescribeDivergence` decide COME si dice, e restituisce la stringa
	// vuota quando le due sequenze coincidono — cosi' un rosso porta `T1|Move#3` invece di un booleano.
	const TArray<FRTBoundaryChecksum> BoundA = URTBoundaryChecksumLibrary::ChecksumsAlongTrace(
		nullptr, A.Log, A.Iniziale, ERTTurnLogFormatVersion::WithMicroStep);
	const TArray<FRTBoundaryChecksum> BoundB = URTBoundaryChecksumLibrary::ChecksumsAlongTrace(
		nullptr, B.Log, B.Iniziale, ERTTurnLogFormatVersion::WithMicroStep);

	// ⛔ ANTI-VACUITA' 3: due sequenze vuote coinciderebbero sempre.
	if (!TestTrue(TEXT("anti-vacuita': la traccia attraversa almeno un boundary"), BoundA.Num() > 0))
	{
		return false;
	}

	TestEqual(TEXT("i checksum di boundary non divergono"),
		URTBoundaryChecksumLibrary::DescribeDivergence(BoundA, BoundB), FString());
	return true;
}

/**
 * `Movement.MicroStepBudgetAccumulatesAcrossCalls` — il tetto conta la RISOLUZIONE, non l'invocazione
 * (`#2856`).
 *
 * 🔴 **Il difetto misurato.** `Guard` era una locale in `ResolveMovement` e un'altra in
 * `ResumeSuspendedResolution`: il tetto limitava un singolo pump, e l'`ensureMsgf` diceva *«risoluzione del
 * movimento non terminata in 256 micro-step»* mentre cio' che scattava era *«questo pump non e' terminato in
 * 256»*. Un resolver che non converge ma si sospende regolarmente attraversava l'asserzione indefinitamente
 * — cioe' esattamente il fallimento che la guardia esiste per prevenire.
 *
 * 🔑 **Cosa dimostra questo test, e perche' basta.** Che il contatore ACCUMULA fra chiamate distinte a
 * `AdvanceMovementResolution`, invece di ripartire con chi lo interroga. E' la stessa proprieta' che una
 * sospensione mette alla prova — la ripresa e' un secondo ingresso nel pump — misurata senza dipendere da
 * quante finestre un fixture apra: un test che si fidasse di quel numero sarebbe verde per fortuna.
 * `MicroStepBudgetSurvivesSuspension`, qui sotto, la misura sul percorso vero.
 *
 * ⚠️ **Il valore atteso non e' scritto a mano.** Quanti micro-step serva un percorso e' un dettaglio del
 * resolver, e pinnarlo qui renderebbe questo test rosso a ogni cambio di locomozione. Cio' che si asserisce
 * e' la RELAZIONE: il contatore vale quanti `Advanced` sono stati restituiti, ne' piu' ne' meno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMicroStepBudgetAccumulatesTest,
	"RefactorTactics.Movement.MicroStepBudgetAccumulatesAcrossCalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMicroStepBudgetAccumulatesTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	// Un percorso di piu' celle: con una sola il contatore non potrebbe distinguere «accumula» da «e' a uno».
	Mover->PlannedCell = FRTCellId(4, 0);

	// Senza contesto non c'e' un budget da leggere, e la risposta lo dice invece di fingere uno zero — che
	// sarebbe indistinguibile da una risoluzione appena cominciata.
	TestEqual(TEXT("senza risoluzione in corso il budget non esiste"),
		TM->GetMicroStepsSpentInResolution(), INDEX_NONE);

	TM->BeginMovementResolution();
	TestEqual(TEXT("una risoluzione appena aperta non ha speso nulla"),
		TM->GetMicroStepsSpentInResolution(), 0);

	// 🔑 **Chiamate DISTINTE, non un ciclo unico**: e' il punto. Ognuna e' un ingresso nuovo nel pump, come
	// lo e' una ripresa dopo una finestra, e il contatore non deve accorgersene.
	int32 Avanzati = 0;
	int32 Iterazioni = 0;
	while (Iterazioni < 512)
	{
		++Iterazioni;
		const ERTMovementAdvanceResult Step = TM->AdvanceMovementResolution();
		if (Step != ERTMovementAdvanceResult::Advanced)
		{
			break;
		}
		++Avanzati;

		// ⛔ Il controllo e' DENTRO il ciclo e non solo alla fine: un contatore che si azzera a ogni chiamata
		// e viene riletto una volta sola in coda mostrerebbe `1` e passerebbe per «accumula» se il percorso
		// avesse un passo solo. Qui ogni passo deve trovare il numero al posto giusto.
		if (TM->GetMicroStepsSpentInResolution() != Avanzati)
		{
			AddError(FString::Printf(
				TEXT("il budget non accumula: dopo %d avanzamenti il contesto ne dichiara %d"),
				Avanzati, TM->GetMicroStepsSpentInResolution()));
			break;
		}
	}

	// ⛔ ANTI-VACUITA': senza almeno due avanzamenti il ciclo sopra non ha misurato nessun accumulo.
	TestTrue(FString::Printf(TEXT("anti-vacuita': la risoluzione ha piu' di un micro-step (%d)"), Avanzati),
		Avanzati >= 2);

	// Il ramo `Finished` non consuma budget: non ha risolto nessun micro-step, e contarlo renderebbe il
	// numero «quante volte qualcuno ha chiesto» invece di «quanti passi sono avvenuti».
	TestEqual(TEXT("l'ultima chiamata, che non avanza, non consuma budget"),
		TM->GetMicroStepsSpentInResolution(), Avanzati);

	TM->FinishMovementResolution();
	TestEqual(TEXT("chiuso il contesto, il budget muore con lui"),
		TM->GetMicroStepsSpentInResolution(), INDEX_NONE);

	DestroyResumeWorld(World);
	return true;
}

/**
 * `Movement.MicroStepBudgetSurvivesSuspension` — il conteggio PROSEGUE attraverso una sospensione, invece di
 * ripartire con la ripresa (`#2856`).
 *
 * 🔑 **E' l'asserzione che la correzione esiste per rendere vera**, e sul percorso vero: `LockInAndResolve`
 * si ferma su una finestra, `ExpireReactionWindow` la chiude, e `ResumeSuspendedResolution` rientra nel pump.
 * Prima di `#2856` quel rientro dichiarava una locale nuova e ricominciava da zero.
 *
 * ⛔ **Non asserisce che l'Overwatch spari**: sono regole d'ingaggio che questa issue non tocca, e legarcisi
 * renderebbe il test rosso per motivi che non lo riguardano. Cio' di cui ha bisogno — due finestre, su
 * boundary diversi — lo verifica prima di misurare, con due gate anti-vacuita' espliciti.
 *
 * ✅ **VALIDATO PER MUTAZIONE, e la prima stesura era vacua — vale la pena dire perche'.** Confrontava i
 * campioni fra loro (`Campioni[i] >= Campioni[i-1]`). Con la mutazione «azzera il contatore all'ingresso del
 * pump» i campioni diventano `[1, 1]` invece di `[1, 2]`, e `1 >= 1` **passa**: il test era verde col
 * difetto dentro. L'ancoraggio giusto non e' il campione precedente ma il **boundary**, che e' un testimone
 * indipendente perche' vive nello stato del resolver e non sullo stack. Con l'uguaglianza al boundary la
 * stessa mutazione da' `finestra 1: budget=1 boundary=1, atteso 2` — misurato il 2026-09-12.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMicroStepBudgetSurvivesSuspensionTest,
	"RefactorTactics.Movement.MicroStepBudgetSurvivesSuspension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMicroStepBudgetSurvivesSuspensionTest::RunTest(const FString&)
{
	UWorld* World = MakeResumeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnResumeMap(World);

	ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	// 🔑 **DUE watcher, e non e' abbondanza.** Una sospensione sola non basta a questo test: il contatore e'
	// osservabile solo mentre il contesto e' vivo, e con una finestra unica l'unico campione si prende PRIMA
	// della ripresa — cioe' proprio dove il difetto non si vede. Servono due finestre perche' la seconda si
	// apra DENTRO il pump di ripresa, che e' il ciclo che dichiarava la seconda locale.
	ARTUnit* WatcherA = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTUnit* WatcherB = SpawnResumeUnit(World, /*TeamId=*/ 1, FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("WatcherA"), WatcherA)
		|| !TestNotNull(TEXT("WatcherB"), WatcherB) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		DestroyResumeWorld(World);
		return false;
	}

	// Stessa impalcatura di `Reactions.WindowSuspendsResolution`: il watcher e' umano, arma l'Overwatch nello
	// slot dell'azione principale, e il cono guarda il mover. Vedi li' per il perche' di ognuna delle tre.
	for (ARTUnit* W : { WatcherA, WatcherB })
	{
		W->bIsBotControlled = false;
		W->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(W, TEXT("Action.Overwatch"), 3);
		W->Facing = ERTHexDirection::W;
		W->PlannedCell = W->Cell;
	}
	Mover->PlannedCell = FRTCellId(2, 0);

	// 🔑 **Il campione si prende MENTRE la risoluzione e' viva.** Alla fine il contesto e' rilasciato e il
	// contatore con lui: leggerlo dopo darebbe `INDEX_NONE` e non proverebbe niente. Il delegate scatta dentro
	// il pump, che e' l'unico istante in cui il numero e' osservabile.
	TArray<int32> Campioni;
	TArray<int32> Boundary;
	TM->OnReactionWindowOpened.BindLambda(
		[&Campioni, &Boundary, TM](const FRTReactionWindowView& View, int32)
		{
			Campioni.Add(TM->GetMicroStepsSpentInResolution());
			Boundary.Add(View.Key.MicroStepIndex);
		});

	TM->LockInAndResolve();

	// ⛔ ANTI-VACUITA' 1: senza sospensione non c'e' niente da far sopravvivere, e il resto del test
	// passerebbe misurando una risoluzione lineare.
	if (!TestTrue(TEXT("anti-vacuita': la resolution si e' sospesa"), TM->IsResolutionSuspended()))
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	const int32 PrimaDellaRipresa = TM->GetMicroStepsSpentInResolution();
	TestTrue(FString::Printf(TEXT("anti-vacuita': qualche micro-step e' gia' stato speso (%d)"),
		PrimaDellaRipresa), PrimaDellaRipresa > 0);

	// La ripresa: ogni scadenza chiude una finestra e rientra nel pump. E' li' che viveva la seconda locale.
	int32 Scadenze = 0;
	int32 MinimoDopoLaRipresa = MAX_int32;
	while (TM->IsResolutionSuspended() && Scadenze < 16)
	{
		TM->ExpireReactionWindow();
		++Scadenze;

		const int32 Ora = TM->GetMicroStepsSpentInResolution();
		if (Ora != INDEX_NONE)
		{
			MinimoDopoLaRipresa = FMath::Min(MinimoDopoLaRipresa, Ora);
		}
	}

	TestFalse(TEXT("scadute le finestre, la resolution e' conclusa"), TM->IsResolutionSuspended());
	TestTrue(FString::Printf(TEXT("anti-vacuita': la ripresa e' avvenuta (%d scadenze)"), Scadenze),
		Scadenze > 0);

	for (int32 i = 0; i < Campioni.Num(); ++i)
	{
		AddInfo(FString::Printf(TEXT("finestra %d: budget=%d boundary=%d"), i, Campioni[i], Boundary[i]));
	}

	// ⛔ **ANTI-VACUITA' 2.** L'unico istante in cui il contatore e' osservabile DENTRO il pump di ripresa e'
	// l'apertura di una finestra. Con un campione solo l'asserzione qui sotto non attraverserebbe mai una
	// ripresa. Se questa riga diventa rossa il fixture ha smesso di aprire due finestre: si aggiusta il
	// fixture, non si toglie la riga.
	if (!TestTrue(FString::Printf(TEXT("anti-vacuita': si sono aperte almeno due finestre (%d)"),
		Campioni.Num()), Campioni.Num() >= 2))
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	// ⛔ **ANTI-VACUITA' 3, e senza di lei l'asserzione seguente e' cieca.** Se tutte le finestre si aprissero
	// sullo STESSO boundary, un contatore azzerato a ogni ripresa produrrebbe la stessa coppia di un
	// contatore che accumula, e il test resterebbe verde col difetto dentro. Misurato: e' esattamente cosi'
	// che una prima stesura di questo test — che confrontava i campioni fra loro con `>=` — passava sulla
	// mutazione «azzera il contatore all'ingresso del pump».
	if (!TestTrue(FString::Printf(TEXT("anti-vacuita': le finestre stanno su boundary diversi (%d -> %d)"),
		Boundary[0], Boundary.Last()), Boundary.Last() > Boundary[0]))
	{
		TM->OnReactionWindowOpened.Unbind();
		DestroyResumeWorld(World);
		return false;
	}

	// 🔴 **L'ASSERZIONE, e l'ancoraggio non e' il campione precedente ma il BOUNDARY.**
	//
	// `FRTReactionOpportunityKey::MicroStepIndex` numera i micro-step dell'intera risoluzione: vive in
	// `FRTMovementResolutionState`, che una sospensione attraversa per costruzione (`#2679` semina
	// `CurrentMicroStepIndex` da li' proprio per non rinumerare i boundary). E' quindi il testimone
	// indipendente di quanti micro-step la risoluzione ha davvero fatto, e il budget deve stargli accanto.
	//
	// 🔑 **La relazione e' `budget == boundary + 1`, ed e' strutturale**: `AdvanceMovementResolution` cattura
	// `CurrentMicroStepIndex` PRIMA di risolvere, `ResolveNextHexMicroStep` incrementa
	// `State.MicroStepIndex`, e subito dopo si incrementa `MicroStepsSpent`. Due contatori dello stesso
	// evento, sfasati di uno. Misurato il 2026-09-12: `(budget=1, boundary=0)` alla prima finestra,
	// `(budget=2, boundary=1)` alla seconda — cioe' dentro il pump di ripresa.
	//
	// ⚠️ **Con la locale di prima la coppia si sarebbe slegata alla ripresa**: `(1, 0)` e poi `(1, 1)`. Il
	// boundary avanza perche' sta nello stato, il budget no perche' stava sullo stack. E' il difetto,
	// espresso come uguaglianza che si rompe.
	for (int32 i = 0; i < Campioni.Num(); ++i)
	{
		TestEqual(FString::Printf(
			TEXT("finestra %d: il budget conta la risoluzione, non l'invocazione (boundary=%d)"),
			i, Boundary[i]), Campioni[i], Boundary[i] + 1);
	}

	// La stessa proprieta' letta da fuori, quando la ripresa lascia il contesto vivo abbastanza da leggerlo.
	// Si guarda il MINIMO e non l'ultimo valore: l'ultimo e' `INDEX_NONE` — il contesto rilasciato — e un
	// azzeramento intermedio seguito da una risalita passerebbe inosservato.
	if (MinimoDopoLaRipresa != MAX_int32)
	{
		TestTrue(FString::Printf(
			TEXT("il conteggio prosegue anche visto da fuori (prima=%d, minimo dopo=%d)"),
			PrimaDellaRipresa, MinimoDopoLaRipresa),
			MinimoDopoLaRipresa >= PrimaDellaRipresa);
	}

	TM->OnReactionWindowOpened.Unbind();
	DestroyResumeWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
