#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Core/RTTypes.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Tests/RTDefeatPresentationProbeForTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo per questi test (nomi distinti per file: in unity build i test condividono la translation unit). */
	UWorld* MakeDefeatPresentationWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyDefeatPresentationWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnDefeatPresentationMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnDefeatPresentationUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	void RunDefeatPresentationTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Come sopra, ma **contando i tick** e comunicandoli alla sonda prima di ognuno.
	 *
	 * Restituisce il tick in cui la risoluzione si e' chiusa. Il numero da solo non dice niente: cio' che
	 * conta e' la DISTANZA da `FirstAnnouncementTick`, cioe' quanto il playback e' continuato dopo
	 * l'annuncio della morte.
	 */
	int32 RunCountingTicks(ARTTurnManager* TM, URTDefeatPresentationProbeForTest* Sonda)
	{
		TM->LockInAndResolve();
		int32 I = 0;
		for (; I < 400 && TM->IsResolving(); ++I)
		{
			Sonda->CurrentTick = I;
			TM->Tick(0.05f);
		}
		return I;
	}
}

/**
 * **L'annuncio di morte arriva su un'unita' ancora disegnata** — la precondizione senza la quale il
 * montaggio `Death` non puo' essere visto (#2452, DoD di #288).
 *
 * 🔴 **Il difetto che questo test copre.** Fino al 2026-09-05 il playback faceva, in quest'ordine:
 *
 * ```cpp
 * DefU->HideForDefeat();      // -> SetActorHiddenInGame(true), propaga alla skeletal
 * DefU->PlayDefeatMontage();  // parte su un attore GIA' nascosto
 * ```
 *
 * Il montaggio partiva e non veniva **mai** disegnato. La terza clausola di `PIE-AS4b` — *«morte →
 * `Death`»* — non era raggiungibile a nessuna qualita' degli asset, e il difetto era **latente**: inerte
 * finche' i dodici `AM_<Pack>_*` non esistono (#2450), attivo il giorno in cui atterrano.
 *
 * 🔑 **Perche' l'oracolo e' la visibilita' e non il conteggio.** Il numero di annunci era **1** anche con il
 * difetto: era l'hide stesso a fare da guardia di idempotenza (`!DefU->IsHidden()`). Contarli non avrebbe
 * distinto le due versioni. Cio' che le distingue e' *su che cosa* arriva l'annuncio.
 *
 * ⚠️ **E il conteggio serve lo stesso, come controllo positivo**: senza `Announcements == 1` l'asserzione
 * su `AnnouncedWhileHidden == 0` sarebbe vera anche se nessuna morte fosse mai avvenuta — verde per
 * assenza invece che per correttezza.
 *
 * ⛔ **Non prova che il montaggio si veda**: quello e' `PIE-AS4b` e l'oracolo e' una persona. Negli scenari
 * headless nessun `AnimInstance` viene istanziato, quindi qui non c'e' animazione da guardare — solo la sua
 * precondizione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefeatAnnouncedWhileStillVisibleTest,
	"RefactorTactics.Playback.DefeatAnnouncedWhileStillVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefeatAnnouncedWhileStillVisibleTest::RunTest(const FString&)
{
	UWorld* World = MakeDefeatPresentationWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefeatPresentationMap(World);

	// Una sola morte, cosi' il conteggio atteso e' esattamente 1 e un doppio annuncio si vede.
	ARTUnit* Vittima = SpawnDefeatPresentationUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Carnefice = SpawnDefeatPresentationUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Vittima || !Carnefice) { DestroyDefeatPresentationWorld(World); return false; }

	Vittima->Health = 1;
	Vittima->Shield = 0;
	Carnefice->PlannedAbilityIndex = 0;
	Carnefice->PlannedAttackTarget = Vittima;

	URTDefeatPresentationProbeForTest* Sonda = NewObject<URTDefeatPresentationProbeForTest>();
	TM->OnUnitDefeated.AddDynamic(Sonda, &URTDefeatPresentationProbeForTest::OnUnitDefeated);

	RunDefeatPresentationTurn(TM);

	// --- Controllo positivo: la morte e' AVVENUTA e l'annuncio e' arrivato --------------------------
	// Senza questo, l'asserzione sulla visibilita' sarebbe vera per assenza.
	if (!TestEqual(TEXT("controllo positivo: esattamente un annuncio di morte"), Sonda->Announcements, 1))
	{
		DestroyDefeatPresentationWorld(World);
		return false;
	}

	// --- L'oracolo del difetto ----------------------------------------------------------------------
	TestEqual(TEXT("l'annuncio arriva su un'unita' ANCORA DISEGNATA (0 annunci su attore nascosto)"),
		Sonda->AnnouncedWhileHidden, 0);

	// --- L'invariante #1: la presentazione non ha spostato la vita logica ---------------------------
	// La distruzione resta in `ConcludeTurn`, cioe' dopo il playback: a turno concluso l'Actor non c'e' piu'.
	TestFalse(TEXT("la vita logica non e' cambiata: l'eliminata e' distrutta a fine turno"),
		IsValid(Vittima));
	TestTrue(TEXT("e chi ha colpito e' ancora in campo"), IsValid(Carnefice) && Carnefice->IsAlive());

	DestroyDefeatPresentationWorld(World);
	return true;
}

/**
 * **Una sola unita' eliminata produce un solo annuncio, anche passando dal catch-all di `FinishPlayback`.**
 *
 * 🔴 **La regressione che questo test impedisce, e che la correzione di #2452 poteva introdurre.**
 * L'idempotenza dell'annuncio veniva da `!DefU->IsHidden()`: era l'hide a marcare «gia' mostrato». Spostando
 * l'hide a `FinishPlayback`, il catch-all avrebbe ritrovato l'unita' **non nascosta** e avrebbe rifatto
 * partire il montaggio e ribroadcastato `OnUnitDefeated` — due volte sulla stessa unita', un `Death` che
 * riparte da capo e ogni cue Blueprint agganciata al delegate eseguita in doppio.
 *
 * Il marcatore esplicito `PlaybackDefeatShown` esiste per questo, e questo test e' la sua ragione: senza di
 * esso `Announcements` vale **2** e `AnnouncedNames` contiene due volte lo stesso nome.
 *
 * ⚠️ **`AnnouncedNames` e non solo il conteggio**: due unita' morte darebbero anche loro `2`, e sarebbe
 * corretto. Cio' che deve non accadere e' lo **stesso nome due volte**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefeatAnnouncedExactlyOnceTest,
	"RefactorTactics.Playback.DefeatAnnouncedExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefeatAnnouncedExactlyOnceTest::RunTest(const FString&)
{
	UWorld* World = MakeDefeatPresentationWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefeatPresentationMap(World);

	ARTUnit* Vittima = SpawnDefeatPresentationUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Carnefice = SpawnDefeatPresentationUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Vittima || !Carnefice) { DestroyDefeatPresentationWorld(World); return false; }

	Vittima->Health = 1;
	Vittima->Shield = 0;
	Carnefice->PlannedAbilityIndex = 0;
	Carnefice->PlannedAttackTarget = Vittima;

	const FString NomeVittima = Vittima->GetName();

	URTDefeatPresentationProbeForTest* Sonda = NewObject<URTDefeatPresentationProbeForTest>();
	TM->OnUnitDefeated.AddDynamic(Sonda, &URTDefeatPresentationProbeForTest::OnUnitDefeated);

	RunDefeatPresentationTurn(TM);

	if (!TestTrue(TEXT("controllo positivo: l'annuncio e' arrivato almeno una volta"),
		Sonda->Announcements >= 1))
	{
		DestroyDefeatPresentationWorld(World);
		return false;
	}

	int32 QuanteVolteLaVittima = 0;
	for (const FString& N : Sonda->AnnouncedNames)
	{
		if (N == NomeVittima) { ++QuanteVolteLaVittima; }
	}

	TestEqual(*FString::Printf(TEXT("la stessa unita' e' annunciata UNA sola volta (annunci in tutto: %d)"),
		Sonda->Announcements), QuanteVolteLaVittima, 1);

	DestroyDefeatPresentationWorld(World);
	return true;
}

/**
 * **Quando la morte cade nell'ULTIMA fase riprodotta, il playback non finisce nello stesso istante**: tiene
 * ancora `DefeatBeatSeconds`, cosi' il montaggio `Death` ha una finestra invece di zero (#2452).
 *
 * 🔴 **Il caso non e' di laboratorio, ed e' il peggiore che ci sia**: e' la forma di
 * `Scenarios/Visual/Combat/Defeat.json`, il banco di `PIE-VIS-KO` che #288 raccomanda per giudicare la
 * sequenza colpo → barra → rimozione. Li' **nessuno si muove**: tutti gli intent sono attacchi, quindi
 * `PlaybackPhases` finisce con `Blast` e l'eliminazione avviene proprio nel `Blast`. Spostare l'hide a
 * `FinishPlayback` — la prima meta' di #2452 — non basta a rendere `Death` visibile su quel banco, perche'
 * `FinishPlayback` segue immediatamente.
 *
 * 🔑 **Il controllo positivo e' dentro il test**: lo stesso scenario viene eseguito due volte, una con
 * `DefeatBeatSeconds = 0` e una col valore reale. Senza il confronto, «il playback e' durato N tick» non
 * direbbe nulla — N dipende dalla durata delle fasi, non dalla coda. Cio' che si misura e' la **differenza**
 * fra i due mondi, e la distanza fra annuncio e fine dentro ciascuno.
 *
 * ⛔ **Non prova che il montaggio si veda**, e non potrebbe: negli scenari headless nessun `AnimInstance`
 * viene istanziato. Prova che la finestra ESISTE. Che dentro quella finestra si veda `Death` e' `PIE-AS4b`,
 * e il suo oracolo e' una persona.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefeatInLastPhaseGetsABeatTest,
	"RefactorTactics.Playback.DefeatInLastPhaseGetsABeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefeatInLastPhaseGetsABeatTest::RunTest(const FString&)
{
	// --- Mondo 1: la coda DISATTIVATA, che e' il comportamento senza questa correzione ----------------
	int32 DistanzaSenzaCoda = -1;
	{
		UWorld* World = MakeDefeatPresentationWorld();
		if (!TestNotNull(TEXT("world di controllo"), World)) { return false; }
		SpawnDefeatPresentationMap(World);

		ARTUnit* Vittima = SpawnDefeatPresentationUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Carnefice = SpawnDefeatPresentationUnit(World, 1, FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Vittima || !Carnefice) { DestroyDefeatPresentationWorld(World); return false; }

		// Nessuno si muove: solo un attacco. `PlaybackPhases` finisce con `Blast`, ed e' li' che si muore.
		Vittima->Health = 1;
		Vittima->Shield = 0;
		Carnefice->PlannedAbilityIndex = 0;
		Carnefice->PlannedAttackTarget = Vittima;

		TM->DefeatBeatSeconds = 0.f; // il controllo: coda spenta

		URTDefeatPresentationProbeForTest* Sonda = NewObject<URTDefeatPresentationProbeForTest>();
		TM->OnUnitDefeated.AddDynamic(Sonda, &URTDefeatPresentationProbeForTest::OnUnitDefeated);

		const int32 TickFine = RunCountingTicks(TM, Sonda);

		if (!TestTrue(TEXT("controllo positivo: nel mondo di controllo la morte e' avvenuta"),
			Sonda->FirstAnnouncementTick >= 0))
		{
			DestroyDefeatPresentationWorld(World);
			return false;
		}
		DistanzaSenzaCoda = TickFine - Sonda->FirstAnnouncementTick;
		DestroyDefeatPresentationWorld(World);
	}

	// Senza coda il playback si chiude praticamente addosso all'annuncio: e' precisamente la finestra zero.
	TestTrue(*FString::Printf(
		TEXT("premessa: senza coda la risoluzione finisce subito dopo l'annuncio (distanza %d tick)"),
		DistanzaSenzaCoda), DistanzaSenzaCoda <= 1);

	// --- Mondo 2: la coda ATTIVA -----------------------------------------------------------------------
	int32 DistanzaConCoda = -1;
	{
		UWorld* World = MakeDefeatPresentationWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnDefeatPresentationMap(World);

		ARTUnit* Vittima = SpawnDefeatPresentationUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Carnefice = SpawnDefeatPresentationUnit(World, 1, FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Vittima || !Carnefice) { DestroyDefeatPresentationWorld(World); return false; }

		Vittima->Health = 1;
		Vittima->Shield = 0;
		Carnefice->PlannedAbilityIndex = 0;
		Carnefice->PlannedAttackTarget = Vittima;

		// Il default del CDO, non un valore scritto qui: se qualcuno lo azzerasse, questo test lo direbbe.
		if (!TestTrue(TEXT("premessa: la coda e' configurata a un valore positivo"), TM->DefeatBeatSeconds > 0.f))
		{
			DestroyDefeatPresentationWorld(World);
			return false;
		}

		URTDefeatPresentationProbeForTest* Sonda = NewObject<URTDefeatPresentationProbeForTest>();
		TM->OnUnitDefeated.AddDynamic(Sonda, &URTDefeatPresentationProbeForTest::OnUnitDefeated);

		const int32 TickFine = RunCountingTicks(TM, Sonda);

		if (!TestTrue(TEXT("controllo positivo: la morte e' avvenuta anche qui"),
			Sonda->FirstAnnouncementTick >= 0))
		{
			DestroyDefeatPresentationWorld(World);
			return false;
		}
		DistanzaConCoda = TickFine - Sonda->FirstAnnouncementTick;

		// La vita logica non e' cambiata: la distruzione resta in `ConcludeTurn`.
		TestFalse(TEXT("la coda non ha tenuto in vita l'eliminata"), IsValid(Vittima));

		DestroyDefeatPresentationWorld(World);
	}

	// --- L'oracolo -------------------------------------------------------------------------------------
	TestTrue(*FString::Printf(
		TEXT("con la coda il playback continua DOPO l'annuncio (%d tick contro %d senza)"),
		DistanzaConCoda, DistanzaSenzaCoda), DistanzaConCoda > DistanzaSenzaCoda + 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
