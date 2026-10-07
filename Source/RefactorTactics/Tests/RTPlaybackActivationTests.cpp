// IL BEAT DI ATTIVAZIONE NEL PLAYBACK (#3549, spec «il momento» §2.4-§2.5).
//
// 🔑 **Il meccanismo, non il disegno**: headless nessun `AnimInstance` esiste, quindi si conta la cue CHIAMATA
// (`ARTUnit::CastCuesPlayedForTest`) e la riga `Attiva:` del feed filtrato per squadra.
// ⚠️ Il viewer e' `RTWorldFixtures::MakePlayerOnTeam`: uno `SpawnActor<ARTPlayerController>` nudo non ha un
// `ARTPlayerState` e `TeamIdOf` ripiega su 0 (`Player/RTPlayerState.cpp:5-12`).
// 🔴 **E la fixture non basta da sola**: senza `InitializeActorsForPlay` il controller non entra nella lista del
// mondo e `GetPlayerController(this, 0)` — la porta di `BeginPlayback` — non lo trova (misurato: zero controller
// nel mondo). Dove il viewer e' scelto dal test il mondo e' inizializzato e una premessa misura il viewer letto;
// dove basta la squadra 0 il test non crea nessun controller, e il viewer e' il ripiego — la squadra delle sorgenti.
// ⚠️ La squadra «che non vede» dei primi due test e' una squadra SENZA unita' in campo: fuori da
// `TeamKnowledgeState`, quindi fail-closed. Esercita il FILTRO; la percezione vera (una squadra presente, un
// muro alto) e' `EnemyBehindHighCoverHasNoActivationBeat`.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Unit/RTUnitAnimInstance.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Perception/RTTeamKnowledge.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	struct FRTBeatDiProva
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Scudo = nullptr;    // Prep: TideGuard
		ARTUnit* Tiratore = nullptr; // Blast: ImpactShot
	};

	ARTUnit* SpawnBeatUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	/** Due attivazioni della squadra 0 — una di Prep, una di Blast — e un viewer della squadra data. */
	bool CostruisciBeat(FAutomationTestBase& Test, int32 SquadraDelViewer, FRTBeatDiProva& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		// 🔴 **Senza, il viewer non esiste per il TurnManager.** Misurato: a mondo non inizializzato
		// `PostInitializeComponents` non gira, il controller non entra in `PlayerControllerList`
		// (`GetNumPlayerControllers() == 0`) e `GetPlayerController(this, 0)` — la porta di `BeginPlayback` —
		// risponde `nullptr`: `TeamIdOf` ripiega su 0 qualunque squadra la fixture abbia assegnato.
		Out.World->InitializeActorsForPlay(FURL());
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		Out.Scudo    = SpawnBeatUnit(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		Out.Tiratore = SpawnBeatUnit(Out.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnBeatUnit(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Scudo || !Out.Tiratore || !Bersaglio) { return false; }
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer);
		if (!Test.TestNotNull(TEXT("viewer"), Viewer)) { return false; }
		// ⛔ Premessa: il viewer che il TurnManager legge (la stessa porta di `BeginPlayback`) e' QUESTO, della
		// squadra data. Senza, ogni asserto sotto misurerebbe il ripiego sulla squadra 0.
		if (!Test.TestTrue(TEXT("⛔ premessa: il TurnManager vede il viewer della fixture"),
				UGameplayStatics::GetPlayerController(Out.Scudo, 0) == Viewer)
			|| !Test.TestEqual(TEXT("⛔ premessa: e la sua squadra e' quella data"),
				ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Out.Scudo, 0)), SquadraDelViewer))
		{
			return false;
		}

		for (int32 i = 0; i < Out.Scudo->NumAbilities(); ++i)
		{
			if (Out.Scudo->GetAbility(i) && Out.Scudo->GetAbility(i)->Def.ActionId == FName(TEXT("Hero.Muiren.TideGuard")))
			{
				Out.Scudo->PlannedAbilityIndex = i;
			}
		}
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		Out.Tiratore->PlannedAbilityIndex = 0;
		Out.Tiratore->PlannedAttackTarget = Bersaglio;
		Out.Tiratore->PlannedCell = Out.Tiratore->Cell;

		Out.TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep (spec §2.5)
		return true;
	}

	void FinoAllaFineDelPlayback(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	bool HaRigaAttiva(const ARTTurnManager* TM, int32 Squadra)
	{
		return TM->GetRecentEventsForTeam(Squadra).ContainsByPredicate(
			[](const FString& Riga) { return Riga.StartsWith(TEXT("Attiva:")); });
	}

	// --- La clip per abilita' (#3563) ------------------------------------------------------------------------

	/** Path sintetici: non esistono nei pack, quindi non si confondono con una clip vera del default. */
	const TCHAR* ClipAzioneScudo    = TEXT("/Game/Prova/AzioneTideGuard.AzioneTideGuard");
	const TCHAR* ClipAzioneTiratore = TEXT("/Game/Prova/AzioneImpactShot.AzioneImpactShot");
	const TCHAR* ClipCaricaLancio   = TEXT("/Game/Prova/AzioneRamCast.AzioneRamCast");
	const TCHAR* ClipCaricaImpatto  = TEXT("/Game/Prova/AzioneRamImpatto.AzioneRamImpatto");

	/** Una variante attiva in `PerAction[ActionId][Ruolo]` dell'eroe, nel CDO di `URTUnitAnimInstance`. */
	void IniettaClipAzioneBeat(const FName& HeroId, const FName& ActionId, ERTPresentationRole Ruolo, const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_ProvaBeat")), FName(TEXT("A")),
			TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_ProvaBeat")));
		GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero.FindOrAdd(HeroId)
			.PerAction.FindOrAdd(ActionId).PerRole.Add(Ruolo, Pool);
	}
}

/**
 * Un'attivazione visibile SUONA la cue `Cast` sulla sorgente e scrive la riga `Attiva:` — spec §5.1, I8.
 * ⛔ Con la sorgente non osservata dal viewer: zero cue, nessuna riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPlaysTheCastCueTest,
	"RefactorTactics.Playback.ActivationPlaysTheCastCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPlaysTheCastCueTest::RunTest(const FString&)
{
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		FinoAllaFineDelPlayback(B.TM);
		TestEqual(TEXT("🔴 la cue Cast e' suonata sul tiratore, una volta"), B.Tiratore->CastCuesPlayedForTest(), 1);
		TestEqual(TEXT("e sullo scudo, nella Prep"), B.Scudo->CastCuesPlayedForTest(), 1);
		TestTrue(TEXT("la riga Attiva: e' nel feed della squadra"), HaRigaAttiva(B.TM, 0));
	}
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 7, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		FinoAllaFineDelPlayback(B.TM);
		TestEqual(TEXT("⛔ sorgente non osservata: nessuna cue"), B.Tiratore->CastCuesPlayedForTest(), 0);
		TestEqual(TEXT("⛔ e nemmeno sullo scudo, nella Prep"), B.Scudo->CastCuesPlayedForTest(), 0);
		// ⚠️ L'assenza della riga e' doppiamente coperta: la nasconde gia' il filtro [D-223] del feed, sul verdetto
		// congelato. Il filtro D6 delle CODE di playback lo pinna `HiddenSourceHasNoActivationBeat`.
		TestFalse(TEXT("⛔ e nessuna riga Attiva: per quella squadra"), HaRigaAttiva(B.TM, 7));
	}
	return true;
}

/**
 * Una sorgente non osservata non entra nelle code di playback — D6, spec §2.5. Tutto o niente.
 * ⛔ Controllo positivo: osservata, entrano entrambe (Prep e Blast).
 * ✅ Validato per mutazione: togliere il filtro in `BeginPlayback` (Prep/Dash) o in `BuildBlastSequence`
 * (Blast) fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackHiddenSourceHasNoBeatTest,
	"RefactorTactics.Playback.HiddenSourceHasNoActivationBeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackHiddenSourceHasNoBeatTest::RunTest(const FString&)
{
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 7, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		TestEqual(TEXT("🔴 non osservata: nessuna attivazione in coda"), B.TM->PlaybackActivationsQueuedForTest(), 0);
		// Il colpo resta: il velo sul bersaglio e' di [D-223], non di questa feature.
		TestTrue(TEXT("e la timeline canonica le conserva"), B.TM->ResolvedEventCountOfTypeForTest(
			ERTResolvedEventType::AbilityActivated) >= 2);
	}
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		TestEqual(TEXT("✅ osservata: Prep e Blast in coda"), B.TM->PlaybackActivationsQueuedForTest(), 2);
	}
	return true;
}

namespace
{
	/** L'indice nel kit dell'abilita' con questo `ActionId`. Nome distinto per l'unity build. */
	int32 BeatIndiceAbilita(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			if (U->GetAbility(i) && U->GetAbility(i)->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}

	URTHexMapAsset* BeatMappa(UWorld* World, int32 Raggio)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Raggio);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;
		return M;
	}
}

/**
 * Il prefisso gia' mostrato della sequenza sopravvive a un Blast SOSPESO davvero — D-355 (Review Focus 3).
 *
 * 🔑 Il montaggio e' quello di `Reactions.Brace.WindowSuspendsBlast` (`Tests/RTDefensiveReactionTests.cpp`): un
 * `Brace` con `Profile.Sidestep` contro un `Push` apre la finestra, la resolution si ferma a meta' Blast e il
 * playback parte sulla timeline PARZIALE; `ExpireReactionWindow` la completa e `BeginPlayback(true)` ricostruisce
 * la sequenza. Il test puro (`BlastSequencePrefixIsStableUnderExtension`) prova la funzione; questo prova che il
 * TurnManager le passa il prefisso giusto.
 * ⚠️ **La finestra si apre solo con un decisore legato** (`OnReactionWindowOpened.IsBound()`, la prima delle
 * condizioni del ramo del `Brace` in `RTTurnManager_Blast.cpp`): senza, la reazione si decide subito e non c'e'
 * nessuna sospensione da attraversare. Il montaggio di riferimento lo lega per misurare; qui serve ad aprirla.
 * ⚠️ **Limite del montaggio, misurato: la ripresa NON aggiunge elementi alla sequenza di Blast.** Il test scrive la
 * crescita nel proprio log (`AddInfo`, «crescita della sequenza»): su questo turno e' ZERO, mentre la timeline
 * cresce — la ripresa aggiunge la spinta e i suoi esiti, nessuno di un tipo che entri in sequenza. La ragione sta nel
 * resolver — attivazioni, impronte, colpi e muri del Blast escono tutti PRIMA della sospensione: l'unico
 * produttore di `Attack` e' il ciclo del danno, le attivazioni escono dopo `ApplyEnvironmentChanges`, e l'unico
 * produttore di `StructureHit` e' `ApplyEnvironmentChanges` (danno raccolto dal piano dei colpi,
 * `Plan.StructureHits`). La spinta, che e' cio' che la finestra sospende, in `ApplyDisplacements` non colpisce
 * strutture: nessuno `StructureHit` con la coppia (Pusher, `Action.Push`) puo' nascere dopo la ripresa.
 * ∴ la mutazione «ricostruisci da zero anche estendendo» (`Previous` vuoto, prefisso 0 in `BeginPlayback`) resta
 * VERDE per IDEMPOTENZA — `Build(T, vuoto, 0) == Build(T, S, k)` quando la parte di Blast della timeline non
 * cresce — e non perche' gli eventi nuovi cadano dopo: di eventi nuovi in sequenza non ce ne sono.
 * Questo test prova che il prefisso SOPRAVVIVE a una sospensione vera (finestra aperta, playback parziale,
 * ripresa); che senza il `Previous` sarebbe stato rotto lo pinna il test puro
 * `BlastSequencePrefixIsStableUnderExtension`, e nessun turno giocato oggi lo puo' pinnare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPrefixSurvivesSuspensionTest,
	"RefactorTactics.Playback.ActivationPrefixSurvivesASuspendedBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPrefixSurvivesSuspensionTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 6);

	ARTUnit* Bracer = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// Il viewer e' il ripiego a 0 (`TeamIdOf` senza controller nel mondo), la squadra del Bracer: un
	// `MakePlayerOnTeam(World, 0)` qui sarebbe inerte, perche' il mondo non e' inizializzato.
	if (!TM || !Bracer || !Pusher) { return false; }

	Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
	Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
	Bracer->PlannedCell = Bracer->Cell;
	Pusher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3);
	Pusher->PlannedAttackTarget = Bracer;
	Pusher->PlannedCell = Pusher->Cell;

	int32 FinestreAperte = 0;
	TM->OnReactionWindowOpened.BindLambda([&FinestreAperte](const FRTReactionWindowView&, int32) { ++FinestreAperte; });
	ON_SCOPE_EXIT{ TM->OnReactionWindowOpened.Unbind(); };

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: la resolution e' sospesa sulla finestra del Brace"), TM->IsResolutionSuspended())
		|| !TestEqual(TEXT("⛔ premessa: la sospensione e' la finestra del Brace, aperta una volta"), FinestreAperte, 1))
	{
		return false;
	}

	// ⛔ **Premessa esplicita: durante la sospensione il playback e' IN CORSO su una timeline PARZIALE**, e la
	// sua sequenza di Blast non e' vuota — e' il prefisso che l'estensione deve conservare. ⚠️ Se questa
	// premessa cade, il MONTAGGIO va rifatto prima di toccare il codice (per esempio il playback non parte
	// durante la finestra, o il Push non e' visibile al viewer): l'invariante resta coperta dal test puro del
	// Task 5, `BlastSequencePrefixIsStableUnderExtension`, e questo test non deve essere allentato per passare.
	if (!TestTrue(TEXT("⛔ premessa: il playback e' in corso durante la sospensione"), TM->IsResolving())
		|| !TestTrue(TEXT("⛔ premessa: la sequenza di Blast della timeline parziale non e' vuota"),
			TM->PlaybackBlastSequenceIndicesForTest().Num() > 0))
	{
		return false;
	}

	// Il playback parziale scorre fino a mostrare almeno un elemento del Blast: e' il prefisso da congelare.
	for (int32 I = 0; I < 200 && TM->IsResolving() && TM->PlaybackBlastShownForTest() == 0; ++I)
	{
		TM->Tick(0.05f);
	}
	const int32 Mostrati = TM->PlaybackBlastShownForTest();
	if (!TestTrue(TEXT("⛔ premessa: un prefisso del Blast e' stato mostrato prima della ripresa"), Mostrati > 0))
	{
		return false;
	}
	const TArray<int32> Prima = TM->PlaybackBlastSequenceIndicesForTest();
	const int32 TimelinePrima = TM->ResolvedTimelineCountForTest();

	for (int32 Scadenze = 0; Scadenze < 8 && TM->IsResolutionSuspended(); ++Scadenze)
	{
		TM->ExpireReactionWindow();
	}
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), TM->IsResolutionSuspended())) { return false; }
	TM->Tick(0.01f); // l'estensione e' gia' avvenuta; un tick breve non tocca il prefisso, che e' gia' mostrato

	const TArray<int32> Dopo = TM->PlaybackBlastSequenceIndicesForTest();
	// La misura che il commento di testa cita: quanto la ripresa ha aggiunto alla timeline e alla sequenza.
	AddInfo(FString::Printf(TEXT("crescita della sequenza di Blast: %d (prima %d, dopo %d, mostrati %d); crescita della timeline: %d"),
		Dopo.Num() - Prima.Num(), Prima.Num(), Dopo.Num(), Mostrati, TM->ResolvedTimelineCountForTest() - TimelinePrima));
	if (!TestTrue(TEXT("la sequenza estesa non e' piu' corta del prefisso"), Dopo.Num() >= Mostrati)) { return false; }
	for (int32 i = 0; i < Mostrati; ++i)
	{
		TestEqual(*FString::Printf(TEXT("🔴 l'elemento %d del prefisso mostrato e' lo stesso dopo l'estensione"), i),
			Dopo[i], Prima[i]);
	}
	return true;
}

/**
 * Nel Dash lo `Step` atterra sulle celle DOPO le attivazioni, e durante l'anticipo lo scattatore NON corre
 * sul posto (Review Focus 4; la classe di difetti di #3519).
 *
 * 🔑 Il playback parte in pausa all'inizio del Dash: l'anticipo dura un `AttackShowSeconds` (una attivazione).
 * ✅ Validato per mutazione: con `Anticipo = 0.f` in `StepMicroStep` il primo `Step` cade dentro l'anticipo e
 * l'indice di cella resta 0; togliendo la condizione `bInAnticipo` da `bInCorsa` il flag di corsa e' acceso
 * durante l'anticipo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackDashStepAfterActivationsTest,
	"RefactorTactics.Playback.DashStepLandsOnCellsAfterTheActivations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackDashStepAfterActivationsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 8);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// Il viewer e' il ripiego a 0 (`TeamIdOf` senza controller nel mondo), la squadra della sorgente: un
	// `MakePlayerOnTeam(World, 0)` qui sarebbe inerte, perche' il mondo non e' inizializzato.
	if (!TM || !Caricatore || !Bersaglio) { return false; }

	Caricatore->PlannedDashAbility = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;
	if (!TestTrue(TEXT("premessa: Branth ha Ram"), Caricatore->PlannedDashAbility != INDEX_NONE)) { return false; }

	TM->RefreshTeamKnowledgeNow();
	TM->SetPlaybackControlsEnabled(true);
	TM->SetStartPlaybackPaused(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' fermo all'inizio del Dash"),
		TM->IsResolving() && TM->IsPlaybackPaused() && TM->GetPlaybackPhaseName() == TEXT("Dash")))
	{
		return false;
	}

	// --- Durante l'anticipo: il cast suona, il cilindro e' fermo e NON corre ---------------------------
	TestFalse(TEXT("🔴 all'ingresso del Dash con un'attivazione la corsa non e' accesa"), Caricatore->bIsMovingVisually);
	// ⛔ Premessa del tick qui sotto: 0,1 s cade DENTRO l'anticipo solo se la cadenza e' piu' lunga. Il valore
	// e' la `UPROPERTY` `ARTTurnManager::AttackShowSeconds` (default 0,50).
	if (!TestTrue(TEXT("AttackShowSeconds > 0.1"), TM->AttackShowSeconds > 0.1f)) { return false; }
	TM->ResumePlayback();
	TM->Tick(0.1f); // dentro l'anticipo: 0,1 s contro un AttackShowSeconds
	TestFalse(TEXT("🔴 durante l'anticipo lo scattatore non corre sul posto"), Caricatore->bIsMovingVisually);
	TestEqual(TEXT("e resta sulla cella di partenza"), TM->PlaybackAnimCellIndexForTest(Caricatore), 0);
	TestEqual(TEXT("il cast e' gia' suonato"), Caricatore->CastCuesPlayedForTest(), 1);

	// --- Il primo Step dopo l'anticipo: cella 1, e ora corre ---------------------------------------------
	TM->PausePlayback();
	TM->StepMicroStep();
	for (int32 I = 0; I < 200 && TM->IsResolving() && !TM->IsPlaybackPaused(); ++I)
	{
		TM->Tick(0.02f);
	}
	TestEqual(TEXT("🔴 il primo Step atterra sulla PRIMA cella della rotta"), TM->PlaybackAnimCellIndexForTest(Caricatore), 1);
	TestTrue(TEXT("e a rotta iniziata la corsa e' accesa"), Caricatore->bIsMovingVisually);
	return true;
}

/**
 * Una sorgente nemica DAVVERO fuori vista — dietro un muro alto, a distanza 3 — non ha beat (Review Focus 5).
 *
 * 🔑 I test con la «squadra senza unita'» esercitano il FILTRO; questo esercita la PERCEZIONE: la squadra del
 * viewer e' in campo, ma il muro le nega la linea di vista sulla sorgente e la consapevolezza a 360° vale entro
 * 2 celle. Il tiro e' bloccato e SI attiva (`Ruling`), ma il viewer non lo vede.
 * ⛔ Premessa: la squadra del viewer E' in `TeamKnowledgeState` (`RefreshTeamKnowledgeNow`, e vede la cella
 * della propria unita') — senza, il verdetto sarebbe il fail-closed della squadra assente, cioe' il filtro.
 * ⛔ Controllo positivo: lo stesso turno visto dalla squadra della sorgente ha il beat.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEnemyBehindHighCoverTest,
	"RefactorTactics.Playback.EnemyBehindHighCoverHasNoActivationBeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEnemyBehindHighCoverTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](int32 SquadraDelViewer, int32& OutCue, bool& OutVisibile) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		World->InitializeActorsForPlay(FURL()); // il viewer deve esistere per il TurnManager: vedi `CostruisciBeat`
		URTHexMapAsset* Map = BeatMappa(World, 6);

		ARTUnit* Tiratore  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* Viewer = RTWorldFixtures::MakePlayerOnTeam(World, SquadraDelViewer);
		if (!TM || !Map || !Tiratore || !Bersaglio || !Viewer)
		{
			return false;
		}
		if (!TestEqual(TEXT("⛔ premessa: il TurnManager legge la squadra del viewer della fixture"),
			ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(TM, 0)), SquadraDelViewer))
		{
			return false;
		}
		// Il muro alto sul bordo fra (0,0) e (1,0): la convenzione `Ring[EdgeIndex]` di `ResolveCoverStructures`.
		const int32 Bordo = URTHexLibrary::Neighbors(FRTCellId(0, 0)).IndexOfByKey(FRTCellId(1, 0));
		if (!TestTrue(TEXT("premessa: (1,0) e' un vicino di (0,0)"), Bordo != INDEX_NONE)) { return false; }
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 0), static_cast<ERTHexDirection>(Bordo), ERTHexCoverType::High, 100);

		Tiratore->PlannedAbilityIndex = BeatIndiceAbilita(Tiratore, TEXT("Hero.Ivrin.PulseShot"));
		Tiratore->PlannedAttackTarget = Bersaglio;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->RefreshTeamKnowledgeNow();
		// ⛔ Controllo positivo della premessa: la squadra 0 e' PRESENTE nella conoscenza — vede la propria cella.
		if (!TestTrue(TEXT("⛔ premessa: la squadra 0 e' in TeamKnowledgeState (vede la cella della propria unita')"),
			TM->KnowledgeForTeamPublic(0).VisibleCells.Contains(Bersaglio->Cell)))
		{
			return false;
		}

		TM->LockInAndResolve();
		// ⚠️ Misurata DOPO la risoluzione, mentre il verdetto dell'attivazione si congela all'emissione, nel Blast:
		// le due letture coincidono perche' in questo turno nessuno si muove (nessun piano di movimento, nessuna
		// spinta), e la premessa qui sotto misura che il muro nega ancora la vista a fine turno: un muro non si
		// ricostruisce dentro il turno, quindi con le stesse posizioni la negava anche all'emissione.
		OutVisibile = TM->KnowledgeForTeamPublic(0).VisibleCells.Contains(Tiratore->Cell);
		FinoAllaFineDelPlayback(TM);
		OutCue = Tiratore->CastCuesPlayedForTest();
		return true;
	};

	int32 Cue = 0;
	bool bVisibile = true;
	if (!GiraIlTurno(/*Viewer*/ 0, Cue, bVisibile)) { return false; }
	if (!TestFalse(TEXT("⛔ premessa: la squadra 0 non vede la cella del tiratore"), bVisibile)) { return false; }
	TestEqual(TEXT("🔴 sorgente nemica fuori vista: nessun beat"), Cue, 0);

	if (!GiraIlTurno(/*Viewer*/ 1, Cue, bVisibile)) { return false; }
	TestEqual(TEXT("✅ visto dalla squadra della sorgente: il beat c'e'"), Cue, 1);
	return true;
}

/**
 * Il cancello del Dash si apre per le sole attivazioni — spec §2.4 C3. Una carica su un bersaglio gia'
 * adiacente non entra in nessuna cella: nessun `MoveAnim` di fase Dash, e la fase deve nascere lo stesso.
 * ✅ Validato per mutazione: riportare il cancello a `if (bHasDash)` fa cadere entrambi gli asserti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackDashOpensForActivationsOnlyTest,
	"RefactorTactics.Playback.DashPhaseOpensForActivationsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackDashOpensForActivationsOnlyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 6);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// Il viewer e' il ripiego a 0 (`TeamIdOf` senza controller nel mondo), la squadra della sorgente: un
	// `MakePlayerOnTeam(World, 0)` qui sarebbe inerte, perche' il mondo non e' inizializzato.
	if (!TM || !Caricatore || !Bersaglio) { return false; }
	Caricatore->PlannedDashAbility = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	const bool bMoveDash = TM->ResolvedTimelineForTest().ContainsByPredicate([](const FRTResolvedEvent& E)
		{ return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash; });
	if (!TestFalse(TEXT("⛔ premessa: nessun Move di fase Dash"), bMoveDash)) { return false; }

	bool bFaseDash = false;
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		bFaseDash |= (TM->GetPlaybackPhaseName() == TEXT("Dash"));
		TM->Tick(0.05f);
	}
	TestTrue(TEXT("🔴 la fase Dash e' nata dalla sola attivazione"), bFaseDash);
	TestEqual(TEXT("🔴 e il cast dello scatto e' suonato"), Caricatore->CastCuesPlayedForTest(), 1);
	return true;
}

/**
 * Il beat conosce l'AZIONE: il cast dello Scudo suona la clip di TideGuard, l'attacco del Tiratore quella di
 * ImpactShot — spec «la clip per abilita'» §2.2, D3.
 *
 * 🔴 **Prima misura che il beat `Attack` PARTA** (`L0` nella traccia dei battiti): nessun altro test su questa
 * fixture asserisce un attacco — contano le cue `Cast` — e senza la premessa la mutazione (6) sarebbe vacua.
 * 🔑 Il `Cast` del Tiratore e' il ripiego (nessuna voce per `ImpactShot`/`Cast`, nessuna generica): e' il
 * controllo positivo dello stesso turno.
 * ✅ Validato per mutazione: (3) `ShowActivation` a un argomento → cade sullo Scudo; (6) `LaunchPlaybackAttack` a
 * un argomento → cade sul Tiratore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPlaysTheActionClipTest,
	"RefactorTactics.Playback.ActivationPlaysTheActionClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPlaysTheActionClipTest::RunTest(const FString&)
{
	const TMap<FName, FRTHeroPresentationClips> Salvato = GetDefault<URTUnitAnimInstance>()->ClipsPerHero;
	ON_SCOPE_EXIT{ GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero = Salvato; };
	IniettaClipAzioneBeat(FName(TEXT("Hero.Muiren")), FName(TEXT("Hero.Muiren.TideGuard")),
		ERTPresentationRole::Cast, ClipAzioneScudo);
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), FName(TEXT("Hero.Branth.ImpactShot")),
		ERTPresentationRole::Attack, ClipAzioneTiratore);

	FRTBeatDiProva B;
	const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }
	B.TM->bRecordAttackBeatsForTest = true;
	B.TM->LockInAndResolve();
	FinoAllaFineDelPlayback(B.TM);

	// ⛔ Le premesse: i due beat sono PARTITI. Senza, ogni asserto sotto confronterebbe un path vuoto.
	if (!TestEqual(TEXT("⛔ premessa: il cast dello scudo e' suonato"), B.Scudo->CastCuesPlayedForTest(), 1)
		|| !TestTrue(TEXT("⛔ premessa: LaunchPlaybackAttack e' partito (L0 nella traccia dei battiti)"),
			B.TM->AttackBeatTraceForTest().Contains(TEXT("L0"))))
	{
		return false;
	}

	TestEqual(TEXT("🔴 il cast dello scudo suona la clip d'azione di TideGuard"),
		B.Scudo->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(), FString(ClipAzioneScudo));
	TestEqual(TEXT("🔴 l'attacco del tiratore suona la clip d'azione di ImpactShot"),
		B.Tiratore->LastResolvedClipPathForTest(ERTPresentationRole::Attack).ToString(), FString(ClipAzioneTiratore));
	TestEqual(TEXT("controllo positivo: il cast del tiratore non ha voce, e' il ruolo"),
		B.Tiratore->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(),
		GetDefault<URTUnitAnimInstance>()->ActiveClipFor(FName(TEXT("Hero.Branth")), ERTPresentationRole::Cast)
			.ToSoftObjectPath().ToString());
	return true;
}

/**
 * Review Focus (e): l'impatto di una carica porta l'`ActionId` dello SCATTO (`Impact.Def = Dash->Def`,
 * `RTTurnManager.cpp:4746`), quindi suona la clip `Attack` dello scatto — spec §2.6, riga di `Hero.Branth.Ram`.
 *
 * ⚠️ Il caricatore e' della squadra 0 e il test non crea nessun controller: il viewer e' il ripiego sulla squadra 0
 * (`RTPlaybackActivationTests.cpp:9-10`), cioe' la squadra delle sorgenti. E' la fixture di
 * `Playback.DashStepLandsOnCellsAfterTheActivations` (qui sopra: `BeatMappa`, mondo non inizializzato, viewer di
 * ripiego), con il bersaglio adiacente alla cella d'arrivo come in `Turn.ChargeActivatesInDashNotInBlast`
 * (`Tests/RTAbilityActivatedTests.cpp`); qui si asserisce il PATH suonato.
 * ✅ Validato per mutazione (6).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChargeImpactPlaysTheDashAttackClipTest,
	"RefactorTactics.Playback.ChargeImpactPlaysTheDashAttackClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChargeImpactPlaysTheDashAttackClipTest::RunTest(const FString&)
{
	const TMap<FName, FRTHeroPresentationClips> Salvato = GetDefault<URTUnitAnimInstance>()->ClipsPerHero;
	ON_SCOPE_EXIT{ GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero = Salvato; };
	const FName Ram(TEXT("Hero.Branth.Ram"));
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), Ram, ERTPresentationRole::Cast, ClipCaricaLancio);
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), Ram, ERTPresentationRole::Attack, ClipCaricaImpatto);

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 8);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Caricatore || !Bersaglio) { return false; }
	const int32 IndiceRam = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	if (!TestTrue(TEXT("⛔ premessa: Branth ha Ram"), IndiceRam != INDEX_NONE)) { return false; }
	Caricatore->PlannedDashAbility = IndiceRam;
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->bRecordAttackBeatsForTest = true;
	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	bool bImpattoConLaChiaveDelloScatto = false;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.ActionId == Ram
			&& Ev.SourceStableUnitId == Caricatore->StableUnitId && Ev.Phase == ERTMatchPhase::Blast)
		{
			bImpattoConLaChiaveDelloScatto = true;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: l'impatto e' un Attack di Blast con la chiave dello scatto"),
			bImpattoConLaChiaveDelloScatto))
	{
		return false;
	}

	FinoAllaFineDelPlayback(TM);
	if (!TestTrue(TEXT("⛔ premessa: LaunchPlaybackAttack e' partito per l'impatto"),
			TM->AttackBeatTraceForTest().Contains(TEXT("L0"))))
	{
		return false;
	}
	TestEqual(TEXT("🔴 l'impatto della carica suona la clip Attack dello SCATTO"),
		Caricatore->LastResolvedClipPathForTest(ERTPresentationRole::Attack).ToString(), FString(ClipCaricaImpatto));
	TestEqual(TEXT("e il cast della carica, nel Dash, la clip Cast dello scatto"),
		Caricatore->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(), FString(ClipCaricaLancio));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
