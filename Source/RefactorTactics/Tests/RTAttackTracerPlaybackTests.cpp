// Il Blast in due battiti per colpo, e il tracer che lo racconta (`#2454`, spec §2.3).
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAttackPlaybackProbeForTest.h" // `OnAttackResolved` col tick di risoluzione in cui scatta (#911)

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	ARTUnit* SpawnBattitoUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Il turno di prova: attaccanti della squadra 0 — quella di chi guarda in un mondo senza player controller
	 * (`ARTPlayerState::TeamIdOf(nullptr)` risponde 0) — contro Ivrin. Con `bDue`, Aevik spara anche lui. Con
	 * `bConMove`, Branth si sposta dopo il colpo: dopo il Blast c'e' un'altra fase, e a fine Blast il playback
	 * non finisce — quindi nessun `FinishPlayback` ripulisce al posto della fase.
	 */
	ARTTurnManager* SetUpBattitoTurn(UWorld* World, bool bDue, bool bConMove = false)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTUnit* Branth = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 2));
		ARTUnit* Bersaglio = SpawnBattitoUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth || !Bersaglio) { return nullptr; }
		Branth->PlannedAbilityIndex = 0;
		Branth->PlannedAttackTarget = Bersaglio;
		Branth->PlannedCell = bConMove ? FRTCellId(1, 3) : Branth->Cell;
		if (bDue)
		{
			ARTUnit* Aevik = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(3, 4));
			if (!Aevik) { return nullptr; }
			Aevik->PlannedAbilityIndex = 0;
			Aevik->PlannedAttackTarget = Bersaglio;
			Aevik->PlannedCell = Aevik->Cell;
		}
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}

	/**
	 * Il colpo ALLE SPALLE di `Privacy.UnseenAttackerIsOutOfTheOriginVerdict`: Branth in (2,2) spara a Ivrin in
	 * (5,2), entrambi a Est — a 3 celle, oltre la consapevolezza ravvicinata (2) e fuori dall'arco frontale di Ivrin.
	 * `TeamBranth` decide chi guarda: in un mondo senza player controller lo spettatore e' la squadra 0, quindi con
	 * `TeamBranth == 0` guarda chi spara, con `1` guarda chi e' colpito e non conosceva l'attaccante.
	 */
	ARTTurnManager* SetUpAlleSpalleTurn(UWorld* World, int32 TeamBranth)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTUnit* Branth = SpawnBattitoUnit(World, TeamBranth, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
		ARTUnit* Ivrin = SpawnBattitoUnit(World, 1 - TeamBranth, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(5, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth || !Ivrin) { return nullptr; }
		Branth->Facing = ERTHexDirection::E;
		Ivrin->Facing = ERTHexDirection::E;
		Branth->PlannedAbilityIndex = 0; // Hero.Branth.ImpactShot, Single, portata 3
		Branth->PlannedAttackTarget = Ivrin;
		Branth->PlannedCell = Branth->Cell;
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}

	/** Tick piccoli finche' il playback e' nel Blast; falso se la risoluzione finisce prima. */
	bool TickUntilBlast(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			if (TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast) { return true; }
			TM->Tick(0.02f);
		}
		return TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast;
	}

	/** L'indice del tick in cui un battito compare per la prima volta nella traccia, o -1. */
	int32 TickOfBeat(const TArray<TPair<int32, FString>>& Visti, const FString& Battito)
	{
		for (const TPair<int32, FString>& V : Visti) { if (V.Value == Battito) { return V.Key; } }
		return -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackHitArrivesAfterLaunchTest,
	"RefactorTactics.Playback.HitArrivesAfterTheLaunch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackHitArrivesAfterLaunchTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	TArray<TPair<int32, FString>> Visti;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		const int32 Prima = TM->AttackBeatTraceForTest().Num();
		TM->Tick(0.02f);
		for (int32 K = Prima; K < TM->AttackBeatTraceForTest().Num(); ++K)
		{
			Visti.Add(TPair<int32, FString>(I, TM->AttackBeatTraceForTest()[K]));
		}
	}

	const int32 Lancio = TickOfBeat(Visti, TEXT("L0"));
	const int32 Arrivo = TickOfBeat(Visti, TEXT("A0"));
	if (!TestTrue(TEXT("premessa: il colpo e' partito ed e' arrivato"), Lancio >= 0 && Arrivo >= 0)) { return false; }
	// 🔴 **La mutazione dichiarata**: con `TracerFlightSeconds = 0` lancio e arrivo cadono nello stesso tick.
	TestTrue(FString::Printf(TEXT("l'arrivo (tick %d) e' DOPO il lancio (tick %d)"), Arrivo, Lancio), Arrivo > Lancio);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackBeatsOrderedInOneTickTest,
	"RefactorTactics.Playback.AttackBeatsStayOrderedInOneTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackBeatsOrderedInOneTickTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ true);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il playback arriva al Blast"), TickUntilBlast(TM))) { return false; }

	// Un tick LUNGO, che copre tutti i battiti del Blast: velocita' x4, o un fotogramma lento.
	TM->Tick(5.f);

	// 🔴 **La mutazione dichiarata**: due cicli separati — prima i lanci, poi gli arrivi — danno
	// `L0, L1, A0, A1`, e `Next Action` fermerebbe sull'arrivo di 0 con 1 gia' in volo.
	const TArray<FString> Attesa = { TEXT("L0"), TEXT("A0"), TEXT("L1"), TEXT("A1") };
	TestEqual(TEXT("i battiti escono L0, A0, L1, A1"),
		FString::Join(TM->AttackBeatTraceForTest(), TEXT(",")), FString::Join(Attesa, TEXT(",")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerInFlightTest,
	"RefactorTactics.Playback.TracerIsInFlightBetweenLaunchAndArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerInFlightTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
	if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }

	// La sonda di `OnAttackResolved`: dice in quale TICK scatta il broadcast, che e' l'istante in cui compaiono
	// `Hit`, numero e barra. `AddToRoot` perche' un `UObject` locale senza riferimento lo porta via il GC.
	URTAttackPlaybackProbeForTest* Sonda = NewObject<URTAttackPlaybackProbeForTest>();
	Sonda->AddToRoot();
	ON_SCOPE_EXIT{ Sonda->RemoveFromRoot(); };
	TM->OnAttackResolved.AddDynamic(Sonda, &URTAttackPlaybackProbeForTest::OnAttackResolved);

	TM->LockInAndResolve();
	bool bVistoInVolo = false;
	bool bInVoloDopoArrivo = false;
	int32 TickLancio = -1;
	int32 TickArrivo = -1;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		Sonda->CurrentTick = I;
		TM->Tick(0.02f);
		const TArray<FString>& Traccia = TM->AttackBeatTraceForTest();
		const bool bLanciato = Traccia.Contains(TEXT("L0"));
		const bool bArrivato = Traccia.Contains(TEXT("A0"));
		if (bLanciato && TickLancio < 0) { TickLancio = I; }
		if (bArrivato && TickArrivo < 0) { TickArrivo = I; }
		if (bLanciato && !bArrivato && Mappa->NumPlaybackTracers() == 1)
		{
			bVistoInVolo = true;
			const FRTPlaybackTracer& T = Mappa->GetPlaybackTracers()[0];
			TestTrue(TEXT("ImpactShot e' un proiettile"), T.Style == ERTTracerStyle::Projectile);
			TestTrue(TEXT("parte dalla cella di Branth"), T.From == FRTCellId(1, 2));
			TestTrue(TEXT("e va su quella di Ivrin"), T.To == FRTCellId(3, 2));
		}
		bInVoloDopoArrivo |= (bArrivato && Mappa->NumPlaybackTracers() > 0);
	}
	// 🔴 **La mutazione dichiarata**: senza la consegna alla mappa nessun tracer e' mai in volo.
	TestTrue(TEXT("fra lancio e arrivo il tracer e' in volo"), bVistoInVolo);
	TestFalse(TEXT("dopo l'arrivo nessun tracer resta"), bInVoloDopoArrivo);

	// V1 — decisione dell'autore: «il numero compare all'arrivo». Il proiettile parte al lancio, ma `Hit`,
	// numero e `OnAttackResolved` scattano al tick dell'ARRIVO e mai a quello del lancio: se il broadcast
	// tornasse al lancio, il danno comparirebbe prima che il proiettile sia partito.
	// ⚠️ Si pinna il tick del broadcast e non quello del tracer: il secondo lo misura la meta' sopra.
	if (TestTrue(TEXT("premessa: il colpo e' partito ed e' arrivato"), TickLancio >= 0 && TickArrivo >= 0)
		&& TestEqual(TEXT("premessa: un solo colpo ha scatenato `OnAttackResolved`"), Sonda->AttackTicks.Num(), 1))
	{
		TestEqual(TEXT("il broadcast di `OnAttackResolved` cade al tick dell'arrivo"),
			Sonda->AttackTicks[0], TickArrivo);
		TestTrue(FString::Printf(TEXT("e dopo il tick del lancio (%d), non con esso (%d)"),
			TickLancio, Sonda->AttackTicks[0]), Sonda->AttackTicks[0] > TickLancio);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerChannelClearsTest,
	"RefactorTactics.Playback.TracerChannelClearsAtBlastEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerChannelClearsTest::RunTest(const FString&)
{
	// (a) `SkipPlayback` con un tracer in volo: il canale si spegne.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo A"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa A"), TM != nullptr && Mappa != nullptr)) { return false; }

		TM->LockInAndResolve();
		for (int32 I = 0; I < 600 && TM->IsResolving() && Mappa->NumPlaybackTracers() == 0; ++I) { TM->Tick(0.02f); }
		if (!TestEqual(TEXT("premessa: un tracer in volo"), Mappa->NumPlaybackTracers(), 1)) { return false; }
		TM->SkipPlayback();
		TestEqual(TEXT("dopo SkipPlayback il canale e' spento"), Mappa->NumPlaybackTracers(), 0);
	}
	// (b) Fine del Blast: nessun tracer sopravvive alla fase.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo B"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ true);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa B"), TM != nullptr && Mappa != nullptr)) { return false; }

		TM->LockInAndResolve();
		if (!TestTrue(TEXT("premessa: il playback arriva al Blast"), TickUntilBlast(TM))) { return false; }
		for (int32 I = 0; I < 600 && TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast; ++I)
		{
			TM->Tick(0.02f);
		}
		TestEqual(TEXT("uscito dal Blast, il canale e' spento"), Mappa->NumPlaybackTracers(), 0);
	}
	// (c) Una fase accorciata SOTTO il volo: l'unico caso in cui la rete di finalizzazione serve davvero, e
	// quindi l'unico in cui lo spegnimento a fine Blast non e' ridondante.
	//
	// 🔴 **(b) non lo pinna, ed e' misurato**: col ritmo di oggi `PhaseTime` dimensiona il Blast su
	// `Max(1, N) * AttackShowSeconds`, quindi all'ultimo tick della fase l'ultimo arrivo e' gia' passato e la
	// consegna in uscita ha gia' lasciato il canale vuoto. Tolta la riga di spegnimento, (a) e (b) restano
	// verdi. Qui `AttackShowSeconds` si abbassa A COLPO IN VOLO (e' una `UPROPERTY` scrivibile, riletta a ogni
	// tick da `PhaseTime`): la fase scade con il colpo lanciato e non arrivato, la consegna lascia il tracer
	// in canale, e a spegnerlo e' solo lo spegnimento a fine Blast.
	// ⚠️ Con un Move dopo il Blast, altrimenti `FinishPlayback` ripulisce comunque e la prova non distingue.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo C"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false, /*bConMove=*/ true);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa C"), TM != nullptr && Mappa != nullptr)) { return false; }

		URTAttackPlaybackProbeForTest* Sonda = NewObject<URTAttackPlaybackProbeForTest>();
		Sonda->AddToRoot();
		ON_SCOPE_EXIT{ Sonda->RemoveFromRoot(); };
		TM->OnAttackResolved.AddDynamic(Sonda, &URTAttackPlaybackProbeForTest::OnAttackResolved);

		TM->LockInAndResolve();
		for (int32 I = 0; I < 600 && TM->IsResolving() && Mappa->NumPlaybackTracers() == 0; ++I) { TM->Tick(0.02f); }
		if (!TestEqual(TEXT("premessa: un tracer in volo"), Mappa->NumPlaybackTracers(), 1)) { return false; }
		if (!TestTrue(TEXT("premessa: siamo nel Blast"), TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast)) { return false; }

		TM->AttackShowSeconds = 0.05f; // la fase scade prima dell'arrivo previsto a `TracerFlightSeconds`
		for (int32 I = 0; I < 50 && TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast; ++I)
		{
			TM->Tick(0.02f);
		}
		if (!TestTrue(TEXT("premessa: il playback e' passato alla fase dopo il Blast, senza finire"),
			TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() != ERTMatchPhase::Blast)) { return false; }
		TestEqual(TEXT("la rete ha arrivato il colpo lanciato e non arrivato"), Sonda->AttackTicks.Num(), 1);
		TestEqual(TEXT("e uscito dal Blast il canale e' spento"), Mappa->NumPlaybackTracers(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyUnseenAttackerTracerTest,
	"RefactorTactics.Privacy.UnseenAttackerTracerIsNotDelivered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyUnseenAttackerTracerTest::RunTest(const FString&)
{
	// 🔴 **Il filtro di privacy del tracer vive in UN punto**: `PushPlaybackTracers` non consegna un tracer il cui
	// stile, per chi guarda, e' `None`. La mappa disegna TUTTO cio' che riceve, quindi quella guardia e' l'intero
	// filtro che impedisce a una linea di partire dalla cella di un attaccante che lo spettatore non poteva vedere.
	// Finora ogni fixture di playback aveva l'attaccante nella squadra 0 — lo spettatore di un mondo di prova —
	// e togliere la guardia, o sostituire `TracerStyleFor(...)` con uno stile costante, lasciava tutto verde.
	//
	// Qui le squadre si scambiano: lo spettatore (squadra 0) e' la VITTIMA, colpita alle spalle a 3 celle da un
	// attaccante che non vedeva (`Privacy.UnseenAttackerIsOutOfTheOriginVerdict` misura il verdetto, questo misura
	// che il playback lo rispetti). La proprieta' e' su OGNI tick: nessun tracer in canale, mai.
	//
	// ⚠️ **Il controllo positivo e' la meta' che regge il test**: con `TracerStyleFor == None` anche una geometria
	// non risolta darebbe zero tracer, e la proprieta' sarebbe vera per un motivo che non e' la privacy. Lo stesso
	// colpo, con le squadre nell'altro verso, deve mostrare il tracer a chi spara.
	//
	// ⛔ **Limite residuo, dichiarato**: una mutazione che sostituisca `PlaybackViewerTeamId` con la costante 0 resta
	// INDIVIDUABILE solo a mano finche' lo spettatore del test e' fisso alla squadra 0 — in un mondo senza player
	// controller `ARTPlayerState::TeamIdOf(nullptr)` risponde 0, e la squadra dello spettatore non si puo' variare
	// da qui. Lo coprono la lettura del codice e la seduta PIE con due giocatori, non questo test.
	for (const bool bSpettatoreEColpito : { false, true })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTTurnManager* TM = SetUpAlleSpalleTurn(World, /*TeamBranth=*/ bSpettatoreEColpito ? 1 : 0);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }

		TM->LockInAndResolve();
		bool bLanciato = false;
		bool bArrivato = false;
		int32 TickConTracer = 0;
		for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.02f);
			bLanciato |= TM->AttackBeatTraceForTest().Contains(TEXT("L0"));
			bArrivato |= TM->AttackBeatTraceForTest().Contains(TEXT("A0"));
			if (Mappa->NumPlaybackTracers() > 0) { ++TickConTracer; }
		}
		if (!TestTrue(bSpettatoreEColpito
				? TEXT("premessa (colpito): il colpo alle spalle e' partito ed e' arrivato")
				: TEXT("premessa (controllo): il colpo e' partito ed e' arrivato"), bLanciato && bArrivato)) { return false; }

		if (bSpettatoreEColpito)
		{
			TestEqual(TEXT("la vittima non vedeva l'attaccante: nessun tracer in canale, su nessun tick"),
				TickConTracer, 0);
		}
		else
		{
			TestTrue(TEXT("controllo positivo: chi spara vede il tracer dello stesso colpo"), TickConTracer > 0);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
