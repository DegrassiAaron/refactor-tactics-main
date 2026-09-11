// L'INGRESSO DEI CONTROLLI DI PLAYBACK (`#2858`).
//
// 🔴 **Cosa mancava, e perche' un test sul manager non lo copriva.** `#1879` ha consegnato
// `PausePlayback`, `ResumePlayback` e `StepMicroStep` con un default fail-closed, e li ha coperti di
// test. Cio' che nessuno copriva e' che qualcuno potesse **premerli**: il gate v0.1 di `#1881` — *«Pause e
// Step si fermano su un safe boundary, in VsBot e Debug»* — era verde nei test e non dimostrabile in
// partita, perche' in partita quei comandi non si accendevano.
//
// 🔑 **Per questo i test del gesto passano dal `ARTPlayerController` e non dal manager.** Chiamare
// `TM->PausePlayback()` dimostra che il manager funziona, cosa che `#1879` ha gia' dimostrato. Passare da
// `OnTogglePlaybackPauseForTest()` percorre la strada del tasto `K`, che e' cio' che a questa issue
// mancava.
//
// ⛔ **La console variable non e' esercitata da qui.** `rt.Debug.PlaybackControls` la legge
// `ARTGameMode` al proprio allestimento, che questi test non montano: l'AC che la riguarda si verifica
// con il `grep` che la issue stessa detta, e l'ergonomia in PIE. Dichiarato invece che simulato.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	URTHexMapAsset* SpawnEntryPointMap(UWorld* World, int32 Radius = 12)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnEntryPointUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Un turno con un colpo e un movimento: due fasi riprodotte, quindi qualcosa da guardare scorrere. */
	ARTTurnManager* SetUpEntryPointTurn(UWorld* World)
	{
		SpawnEntryPointMap(World);
		ARTUnit* Attaccante = SpawnEntryPointUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
		ARTUnit* Bersaglio  = SpawnEntryPointUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Attaccante || !Bersaglio) { return nullptr; }

		Attaccante->PlannedAbilityIndex = 0;
		Attaccante->PlannedAttackTarget = Bersaglio;
		Attaccante->PlannedCell = FRTCellId(2, 3);
		return TM;
	}

	/** Il TurnLog reso confrontabile voce per voce — non un hash: l'ordine e' parte del contratto. */
	TArray<FString> EntryPointTurnLogFingerprint(const ARTTurnManager* TM)
	{
		TArray<FString> Out;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			Out.Add(FString::Printf(TEXT("%d|%d|%d|%d|%d|(%d,%d,%d)->(%d,%d,%d)|%s"),
				static_cast<int32>(E.Phase), static_cast<int32>(E.Category), E.Outcome, E.Amount, E.UnitId,
				E.SrcCell.X, E.SrcCell.Y, E.SrcCell.Layer,
				E.TgtCell.X, E.TgtCell.Y, E.TgtCell.Layer,
				*E.ActionId.ToString()));
		}
		return Out;
	}
}

/**
 * 🔑 **Il criterio centrale della issue: i comandi hanno un chiamante che non e' un test del manager.**
 *
 * Il gesto passa dal controller — la stessa strada del tasto `K` — e ferma il playback. Poi `L` avanza di
 * un micro-step e torna fermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackControlsReachableFromControllerTest,
	"RefactorTactics.Playback.ControlsAreReachableFromTheController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackControlsReachableFromControllerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpEntryPointTurn(World);
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }
	if (!TestNotNull(TEXT("player controller"), PC)) { return false; }

	// --- ⛔ Senza accensione il gesto non fa nulla ----------------------------------------------------
	// E' il fail-closed di `#1879` osservato da FUORI: il tasto esiste, il comando no.
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	PC->OnTogglePlaybackPauseForTest();
	TestFalse(TEXT("⛔ a controlli spenti il gesto non ferma niente"), TM->IsPlaybackPaused());

	// --- ✅ Accesi, lo stesso gesto ferma ------------------------------------------------------------
	TM->SetPlaybackControlsEnabled(true);
	PC->OnTogglePlaybackPauseForTest();
	TestTrue(TEXT("✅ acceso, il gesto K ferma il playback"), TM->IsPlaybackPaused());

	// Il toggle interroga lo stato: premuto di nuovo, riprende.
	PC->OnTogglePlaybackPauseForTest();
	TestFalse(TEXT("✅ e premuto di nuovo riprende"), TM->IsPlaybackPaused());

	// --- ✅ `L` avanza di un micro-step e torna fermo -------------------------------------------------
	PC->OnStepPlaybackMicroStepForTest();
	// ⚠️ `StepMicroStep` riparte fino al confine: il ritorno alla pausa avviene nel tick che lo raggiunge.
	for (int32 I = 0; I < 200 && TM->IsResolving() && !TM->IsPlaybackPaused(); ++I)
	{
		TM->Tick(0.05f);
	}
	TestTrue(TEXT("✅ dopo lo Step il playback e' di nuovo fermo"),
		TM->IsPlaybackPaused() || !TM->IsResolving());

	return true;
}

/**
 * ⚠️ **Partire in pausa e' subordinato ai controlli.** Senza il comando per riprendere, partire fermi
 * sarebbe una partita bloccata da un flag — e la richiesta non va nemmeno messa in coda: si applicherebbe
 * a un'accensione successiva che nessuno ha collegato a questa richiesta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStartPausedRequiresControlsTest,
	"RefactorTactics.Playback.StartPausedRequiresControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStartPausedRequiresControlsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	TestFalse(TEXT("nessuno parte in pausa per default"), TM->DoesPlaybackStartPaused());

	TM->SetStartPlaybackPaused(true);
	TestFalse(TEXT("⛔ a controlli spenti la richiesta e' inerte"), TM->DoesPlaybackStartPaused());

	// ⛔ ANTI-VACUITA': e non e' stata messa in coda. Accendere i controlli DOPO non deve far riemergere
	// una richiesta che era stata rifiutata.
	TM->SetPlaybackControlsEnabled(true);
	TestFalse(TEXT("⛔ e non riemerge quando i controlli si accendono"), TM->DoesPlaybackStartPaused());

	// --- ⛔ ASSERZIONE DI CONTROLLO: con i controlli accesi la richiesta vale ------------------------
	TM->SetStartPlaybackPaused(true);
	TestTrue(TEXT("✅ a controlli accesi la richiesta vale"), TM->DoesPlaybackStartPaused());

	// E revocare i controlli la spegne: e' la terza via alla partita bloccata da un flag.
	TM->SetPlaybackControlsEnabled(false);
	TestFalse(TEXT("revocare i controlli spegne anche la politica"), TM->DoesPlaybackStartPaused());

	return true;
}

/**
 * 🔑 **Con *start paused*, il primo confine osservabile e' il primo.** Il playback nasce fermo e non
 * scorre finche' non si riprende.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStartPausedHoldsFirstFrameTest,
	"RefactorTactics.Playback.StartPausedHoldsTheFirstFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStartPausedHoldsFirstFrameTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpEntryPointTurn(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->SetStartPlaybackPaused(true);
	TM->LockInAndResolve();

	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }
	TestTrue(TEXT("il playback nasce FERMO"), TM->IsPlaybackPaused());

	// ⛔ ANTI-VACUITA': e resta fermo. Senza questo, «nasce fermo» sarebbe vero anche per un flag che il
	// primo tick spegne da solo — cioe' per una pausa che non ferma niente.
	for (int32 I = 0; I < 100; ++I) { TM->Tick(0.05f); }
	TestTrue(TEXT("⛔ e cento tick dopo e' ancora fermo, e il turno non e' concluso"),
		TM->IsPlaybackPaused() && TM->IsResolving());

	// ✅ E riprende quando glielo si chiede: la pausa non e' un blocco.
	TM->ResumePlayback();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	TestFalse(TEXT("✅ ripreso, il turno arriva in fondo"), TM->IsResolving());

	return true;
}

/**
 * 🔴 **`SkipPlayback` da uno stato di pausa non deve lasciare il turno SEGUENTE fermo.**
 *
 * E' il rischio che `#2858` registra, ed era un difetto vero: `TickPlayback` esce prima di toccare
 * qualunque cosa quando e' in pausa, quindi un playback fermo non puo' finire da solo — l'unica strada
 * per arrivare a `FinishPlayback` con il flag acceso e' `SkipPlayback()`. Misurato su `00b751ce`: ne'
 * `FinishPlayback` ne' `BeginPlayback` azzeravano `bPlaybackPaused`.
 *
 * ⚠️ **Era latente finche' i comandi non avevano chiamanti**: nessuno poteva mettere in pausa, quindi
 * nessuno poteva saltare da fermo. Questa issue apre la via, e la chiude nello stesso commit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSkipFromPauseTest,
	"RefactorTactics.Playback.SkipFromPauseDoesNotLeaveTheNextTurnPaused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSkipFromPauseTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpEntryPointTurn(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	TM->PausePlayback();
	if (!TestTrue(TEXT("premessa: fermo"), TM->IsPlaybackPaused())) { return false; }

	// Il salto da fermo: e' la via che apre il difetto.
	TM->SkipPlayback();

	TestFalse(TEXT("🔴 saltare da fermo non lascia la pausa accesa"), TM->IsPlaybackPaused());

	return true;
}

/**
 * 🔴 **L'ASSERZIONE DI CONTROLLO della issue**: senza l'ingresso, il comportamento di partita e'
 * **identico**. Stesso turno in due mondi — uno con i controlli accesi e mai usati, uno senza — e TurnLog
 * confrontato voce per voce.
 *
 * ⛔ Non e' un hash: due tracce con lo stesso stato finale e un ORDINE diverso darebbero lo stesso digest,
 * e l'ordine e' parte del contratto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackWithoutEntryPointNothingChangesTest,
	"RefactorTactics.Playback.WithoutTheEntryPointNothingChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackWithoutEntryPointNothingChangesTest::RunTest(const FString&)
{
	TArray<FString> SenzaIngresso;
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo A"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTTurnManager* TM = SetUpEntryPointTurn(World);
		if (!TestNotNull(TEXT("turno A"), TM)) { return false; }

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		SenzaIngresso = EntryPointTurnLogFingerprint(TM);
	}

	TArray<FString> ConIngresso;
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo B"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTTurnManager* TM = SetUpEntryPointTurn(World);
		if (!TestNotNull(TEXT("turno B"), TM)) { return false; }

		// I controlli accesi ma non premuti: e' il caso che chi lancia con la console variable ottiene
		// prima di toccare qualsiasi tasto, ed e' quello che non deve cambiare la partita.
		TM->SetPlaybackControlsEnabled(true);
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		ConIngresso = EntryPointTurnLogFingerprint(TM);
	}

	// ⛔ ANTI-VACUITA': due TurnLog vuoti sono identici e non misurerebbero niente.
	if (!TestTrue(TEXT("⛔ il turno ha prodotto voci di TurnLog"), SenzaIngresso.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("stesso NUMERO di voci con e senza ingresso"),
		ConIngresso.Num(), SenzaIngresso.Num());

	const int32 N = FMath::Min(ConIngresso.Num(), SenzaIngresso.Num());
	for (int32 I = 0; I < N; ++I)
	{
		TestEqual(*FString::Printf(TEXT("voce %d identica"), I), ConIngresso[I], SenzaIngresso[I]);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
