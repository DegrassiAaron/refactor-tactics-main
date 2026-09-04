// I CONTROLLI DI PLAYBACK, E IL LORO DIVIETO PER DEFAULT (#1879).
//
// La matrice delle modalita' di #1879 vieta Pause e Step nel PvP live competitivo: l'autorita' non puo'
// essere fermata localmente da un client, e la deducibilita' del ritmo ha gia' un proprietario (#759).
//
// ⚠️ **Quella modalita' non esiste nel codice** — `ERTMatchMode`, `bCompetitive`, `bIsPvP`: zero occorrenze,
// coerente con l'assenza di replica di rete misurata su #1805 (`DOREPLIFETIME` -> zero). ⛔ Inventarla per
// poterla negare sarebbe fabbricare una dipendenza.
//
// 🔑 Il divieto si ottiene **per costruzione**: i comandi sono inerti finche' qualcuno non li abilita. Questi
// test misurano quel default, che e' l'unica parte del criterio che oggi ha un soggetto.

#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "RTWorldFixtures.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔴 **Pause e Step non fanno nulla finche' non sono abilitati** — e il test lo misura sul MANAGER, non
 * sulla libreria: la libreria non ha modalita', il divieto vive dove vive lo stato.
 *
 * ⛔ **L'asserzione di controllo e' la seconda meta'**: dopo l'abilitazione gli stessi comandi devono
 * funzionare. Senza, un `PausePlayback()` implementato come corpo vuoto passerebbe la prima meta' del test
 * e il criterio sarebbe verde su una funzione che non esiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackControlsDeniedByDefaultTest,
	"RefactorTactics.Playback.ControlsAreDeniedByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackControlsDeniedByDefaultTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	// --- il default e' il divieto ----------------------------------------------------------------------
	TestFalse(TEXT("i controlli nascono NEGATI"), TM->ArePlaybackControlsEnabled());
	TestFalse(TEXT("e il playback non nasce in pausa"), TM->IsPlaybackPaused());

	TM->PausePlayback();
	TestFalse(TEXT("⛔ Pause e' inerte senza abilitazione"), TM->IsPlaybackPaused());

	TM->StepMicroStep();
	TestFalse(TEXT("⛔ e anche Step lo e'"), TM->IsPlaybackPaused());

	// --- ⛔ ASSERZIONE DI CONTROLLO: abilitati, funzionano ---------------------------------------------
	// Senza questa meta', un comando implementato come corpo vuoto passerebbe tutto il test qui sopra.
	TM->SetPlaybackControlsEnabled(true);
	TestTrue(TEXT("l'abilitazione si legge"), TM->ArePlaybackControlsEnabled());

	TM->PausePlayback();
	TestTrue(TEXT("✅ ora Pause ferma davvero"), TM->IsPlaybackPaused());

	TM->ResumePlayback();
	TestFalse(TEXT("✅ e Resume riprende"), TM->IsPlaybackPaused());

	// --- ⚠️ togliere i controlli non lascia la partita ferma -------------------------------------------
	TM->PausePlayback();
	TestTrue(TEXT("premessa: fermo"), TM->IsPlaybackPaused());
	TM->SetPlaybackControlsEnabled(false);
	TestFalse(TEXT("disabilitare mentre e' fermo lo fa RIPARTIRE"), TM->IsPlaybackPaused());

	return true;
}

/**
 * ⚠️ **Una fase senza segmenti non si puo' attraversare, e `Step` non ci prova** (`#1879`).
 *
 * Il manager appena spawnato non ha un playback in corso: nessuna fase, nessuna animazione, durata zero.
 * `StepMicroStep` deve fermarsi invece di dividere per zero o di inventare un avanzamento — ed e' il caso
 * degenere che un test scritto solo sul caso felice non tocca mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStepOnEmptyPhaseTest,
	"RefactorTactics.Playback.StepOnAnEmptyPhaseDoesNotAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStepOnEmptyPhaseTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);

	TestEqual(TEXT("senza playback in corso non ci sono micro-step"),
		TM->MicroStepsInCurrentPlaybackPhase(), 0);

	TM->StepMicroStep();
	TestTrue(TEXT("Step su una fase vuota lascia fermi, non avanza"), TM->IsPlaybackPaused());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
