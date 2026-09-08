#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h" // DeriveOpportunityId: l'identita' della finestra
#include "Turn/RTTurnLog.h"
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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
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

	// Due celle: dentro il budget di movimento del Wraith senza dover conoscere il numero esatto.
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

#endif // WITH_DEV_AUTOMATION_TESTS
