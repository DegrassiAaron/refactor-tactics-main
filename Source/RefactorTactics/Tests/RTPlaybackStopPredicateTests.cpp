// IL PREDICATO DI PAUSA UNA TANTUM: `Next Phase` E `Next Action` (`#2855`).
//
// 🔑 **Cosa misura questo file, e cosa NON misura.** Misura che il playback si fermi dove il predicato
// dice, che il predicato si consumi, e che non sopravviva al turno. ⛔ **Non** misura l'equivalenza fra
// avanzamento comandato e continuo: quello e' il gate di `#2859`, che confronta due esecuzioni e non una.
//
// 🔴 **L'asserzione che pesa di piu' e' quella di CONTROLLO**: armare il predicato non deve cambiare il
// TurnLog. Un playback che decidesse qualcosa violerebbe l'invariante #1 di `#1881` — *«il playback puo'
// cambiare COME e QUANTO VELOCEMENTE si mostra un risultato gia' deciso, non il risultato»* — e senza
// questa verifica il resto del file sarebbe verde su una regressione di autorita'.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTPlaybackLibrary.h"
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

	URTHexMapAsset* SpawnStopPredicateMap(UWorld* World, int32 Radius = 12)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnStopPredicateUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	/**
	 * Il turno di prova: un colpo (fase `Blast`) e un movimento (fase `Move`), cosi' il playback ha almeno
	 * DUE fasi e un confine da attraversare. Senza due fasi ogni asserzione su `Next Phase` sarebbe vera
	 * per assenza di confini.
	 */
	ARTTurnManager* SetUpTwoPhaseTurn(UWorld* World)
	{
		SpawnStopPredicateMap(World);
		ARTUnit* Attaccante = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
		ARTUnit* Bersaglio  = SpawnStopPredicateUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Attaccante || !Bersaglio) { return nullptr; }

		Attaccante->PlannedAbilityIndex = 0;
		Attaccante->PlannedAttackTarget = Bersaglio;
		Attaccante->PlannedCell = FRTCellId(2, 3);
		return TM;
	}

	/** Avanza il playback di al piu' `MaxTick` tick, fermandosi appena il playback si mette in pausa. */
	void AdvanceUntilPausedOrDone(ARTTurnManager* TM, int32 MaxTick = 400)
	{
		for (int32 I = 0; I < MaxTick && TM->IsResolving() && !TM->IsPlaybackPaused(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Il TurnLog reso confrontabile voce per voce.
	 *
	 * ⛔ **Non un hash.** Due tracce con lo stesso stato finale e un ORDINE diverso darebbero lo stesso
	 * digest, e l'ordine degli eventi e' parte del contratto: `EntryLess` esiste per questo. E' lo stesso
	 * criterio che `#2859` fissa per il proprio gate.
	 */
	TArray<FString> TurnLogFingerprint(const ARTTurnManager* TM)
	{
		TArray<FString> Out;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			Out.Add(FString::Printf(TEXT("%d|%d|%d|%d|%d|(%d,%d,%d)->(%d,%d,%d)|%s|%s"),
				static_cast<int32>(E.Phase), static_cast<int32>(E.Category), E.Outcome, E.Amount, E.UnitId,
				E.SrcCell.X, E.SrcCell.Y, E.SrcCell.Layer,
				E.TgtCell.X, E.TgtCell.Y, E.TgtCell.Layer,
				*E.ActionId.ToString(), *E.BaseActionId.ToString()));
		}
		return Out;
	}
}

/**
 * ⛔ **Senza controlli abilitati il predicato e' inerte** — fail-closed, la stessa forma di `#1879`.
 *
 * E l'asserzione di CONTROLLO e' la seconda meta': dopo l'abilitazione l'armamento deve leggersi. Senza,
 * un `RequestPlaybackStopAt` implementato come corpo vuoto passerebbe la prima meta' e il criterio sarebbe
 * verde su un comando che non esiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPredicateInertWithoutControlsTest,
	"RefactorTactics.Playback.PredicateIsInertWithoutControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPredicateInertWithoutControlsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	TestEqual(TEXT("nessun predicato armato all'inizio"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextPhase);
	TestEqual(TEXT("⛔ armare e' inerte senza controlli"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));
	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
	TestEqual(TEXT("⛔ vale per entrambi i confini"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	// --- ⛔ ASSERZIONE DI CONTROLLO -------------------------------------------------------------------
	TM->SetPlaybackControlsEnabled(true);
	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextPhase);
	TestEqual(TEXT("✅ abilitati, l'armamento si legge"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::NextPhase));

	// Armare due volte di fila non accumula: e' lo stesso stato riscritto, non due confini in coda.
	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextPhase);
	TestEqual(TEXT("armarlo due volte resta UN predicato"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::NextPhase));

	// ⚠️ Togliere i controlli disarma: un predicato armato in una sessione senza il comando per riprendere
	// fermerebbe il playback e nessuno potrebbe piu' farlo ripartire.
	TM->SetPlaybackControlsEnabled(false);
	TestEqual(TEXT("revocare i controlli disarma il predicato"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	return true;
}

/**
 * 🔑 **Il criterio principale**: armato `Next Phase`, l'immagine scorre e si ferma all'INIZIO della fase
 * successiva — `IsPlaybackPaused()` vero, e `GetPlaybackPhaseName()` che nomina la fase NUOVA.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextPhaseStopsAtBoundaryTest,
	"RefactorTactics.Playback.NextPhaseStopsAtThePhaseBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextPhaseStopsAtBoundaryTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();

	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving()))
	{
		return false;
	}
	const FString FaseIniziale = TM->GetPlaybackPhaseName();

	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextPhase);
	AdvanceUntilPausedOrDone(TM);

	// ⛔ ANTI-VACUITA': se il turno avesse una sola fase non ci sarebbe nessun confine da attraversare, e
	// «si e' fermato» sarebbe indistinguibile da «e' finito». Il setup ne prevede due.
	if (!TestTrue(TEXT("⛔ il playback e' ancora in corso: si e' fermato, non concluso"), TM->IsResolving()))
	{
		return false;
	}
	TestTrue(TEXT("si e' fermato al confine"), TM->IsPlaybackPaused());
	TestNotEqual(TEXT("e la fase nominata e' quella NUOVA"), TM->GetPlaybackPhaseName(), FaseIniziale);

	// Il predicato si e' CONSUMATO: e' una tantum, non una modalita'.
	TestEqual(TEXT("il predicato si e' consumato"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	// ⛔ E senza riarmarlo il playback non si ferma piu' da solo: e' la riga che tiene il comando dalla
	// parte giusta di [D-355] — una fase non acquisisce una ragione di fermarsi «per simmetria».
	TM->ResumePlayback();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	TestFalse(TEXT("⛔ senza predicato armato il playback arriva in fondo"), TM->IsResolving());

	return true;
}

/**
 * ⚠️ **Armato durante l'ULTIMA fase riprodotta, il playback arriva alla fine del turno e non resta
 * armato.** Un predicato armato e mai soddisfatto bloccherebbe l'osservazione del turno seguente senza
 * dirlo — il rischio che `#2855` registra.
 *
 * ⚠️ `PlaybackPhases` non contiene mai `Cleanup`: un `Next Phase` dall'ultima fase non porta al Cleanup,
 * porta alla fine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPredicateDoesNotSurviveTheTurnTest,
	"RefactorTactics.Playback.ArmedPredicateDoesNotSurviveTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPredicateDoesNotSurviveTheTurnTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();

	// Si arma e si lascia scorrere fino alla fine: attraversato il primo confine il predicato si consuma,
	// e riarmandolo sull'ultima fase non trovera' piu' nulla da consumare.
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextPhase);
		TM->ResumePlayback();
		TM->Tick(0.05f);
	}

	TestFalse(TEXT("il turno e' arrivato in fondo"), TM->IsResolving());
	TestEqual(TEXT("⛔ e nessun predicato e' sopravvissuto al turno"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	return true;
}

/**
 * 🔑 **`Next Action` si ferma sul primo colpo di un'azione diversa** (`#2857` porta l'`ActionId`, questa
 * issue lo consuma).
 *
 * ⚠️ Su un turno con un solo atto il confine di azione coincide con quello di fase, e la fermata avviene
 * comunque: il test asserisce che ci si fermi **prima della fine**, non su quale dei due sia scattato.
 * Distinguerli richiederebbe un turno con due azioni diverse nello stesso `Blast`, che il corpus di questo
 * file non costruisce — ed e' dichiarato invece di essere simulato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionStopsBeforeTheEndTest,
	"RefactorTactics.Playback.NextActionStopsAtTheActionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionStopsBeforeTheEndTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
	TestEqual(TEXT("premessa: armato"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::NextAction));

	AdvanceUntilPausedOrDone(TM);

	TestTrue(TEXT("il playback si e' fermato su un atto, non e' arrivato in fondo"), TM->IsResolving());
	TestTrue(TEXT("ed e' fermo"), TM->IsPlaybackPaused());
	TestEqual(TEXT("il predicato si e' consumato"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	return true;
}

/**
 * 🔴 **L'ASSERZIONE DI CONTROLLO, e la ragione per cui questo file esiste oltre ai suoi criteri.**
 *
 * Lo stesso turno, negli stessi due mondi, risolto una volta con il predicato armato ripetutamente e una
 * volta senza: il TurnLog deve essere **identico voce per voce**. Un playback che decidesse qualcosa
 * violerebbe l'invariante #1 di `#1881`, e sarebbe la regressione piu' grave che questo comando puo'
 * introdurre — un trasporto che cambia cio' che trasporta.
 *
 * ⛔ **Non e' il gate di `#2859`.** Quello confronta avanzamento COMANDATO e continuo su tutta la
 * risoluzione, con i checksum di boundary e `FirstDivergence` a nominare il punto. Qui si verifica solo
 * che *armare un predicato* sia neutro. Il gate resta da scrivere, e ha altre dipendenze.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPredicateDoesNotChangeTheTurnLogTest,
	"RefactorTactics.Playback.PredicateDoesNotChangeTheTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPredicateDoesNotChangeTheTurnLogTest::RunTest(const FString&)
{
	TArray<FString> SenzaPredicato;
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo A"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
		if (!TestNotNull(TEXT("turno A"), TM)) { return false; }

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		SenzaPredicato = TurnLogFingerprint(TM);
	}

	TArray<FString> ConPredicato;
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo B"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
		if (!TestNotNull(TEXT("turno B"), TM)) { return false; }

		TM->SetPlaybackControlsEnabled(true);
		TM->LockInAndResolve();
		// Si arma a ogni tick: il predicato viene consumato e riarmato per tutta la risoluzione, cioe' il
		// caso piu' sfavorevole — il playback si ferma a ogni confine che incontra.
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
			TM->ResumePlayback();
			TM->Tick(0.05f);
		}
		ConPredicato = TurnLogFingerprint(TM);
	}

	// ⛔ ANTI-VACUITA': due TurnLog vuoti sono identici, e direbbero che il test non ha misurato niente.
	if (!TestTrue(TEXT("⛔ il turno ha prodotto voci di TurnLog"), SenzaPredicato.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("stesso NUMERO di voci con e senza predicato"),
		ConPredicato.Num(), SenzaPredicato.Num());

	const int32 N = FMath::Min(ConPredicato.Num(), SenzaPredicato.Num());
	for (int32 I = 0; I < N; ++I)
	{
		// Voce per voce E nello stesso ordine: l'ordine degli eventi e' parte del contratto, e un confronto
		// insensibile alla posizione lascerebbe passare una permutazione.
		TestEqual(*FString::Printf(TEXT("voce %d identica"), I), ConPredicato[I], SenzaPredicato[I]);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
