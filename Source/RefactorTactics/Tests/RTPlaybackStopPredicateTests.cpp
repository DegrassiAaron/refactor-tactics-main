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
#include "RTAbilityFixtures.h"

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

// =========================================================================================================
// `#3292` — i CANALI del confine d'atto: `Next Action` si fermava solo sui colpi.
//
// 🔴 Il difetto aveva tre facce, e tutte e tre vivevano nello stesso tick: due canali non arrivavano al
// predicato, la regola era scritta due volte (di cui una morta), e l'atto in corso si leggeva da un canale
// solo.
// =========================================================================================================

namespace
{
	/**
	 * Un turno in cui **nessun `Attack` esiste**: un muro ALTO incassa, e il colpo non raggiunge nessuno.
	 *
	 * 🔑 **E' il caso che discrimina le due cause di `#3281`.** Con un colpo in scena, un `Next Action` che
	 * si ferma non direbbe **su cosa** si e' fermato: qui di `Attack` non ce n'e' nemmeno uno, quindi una
	 * fermata puo' essere venuta solo da un altro canale.
	 *
	 * Il muro alto sul bordo W di `(1,0)` toglie la linea di tiro (`LineOfSightPolicy::Required`), quindi
	 * l'intento finisce in `BlockedIntents` — ma il danno alla struttura si raccoglie **prima** di quel
	 * controllo, e la barriera incassa lo stesso. E' la stessa scena di `Cover.Destruction.LoggedInPlayedTurn`.
	 */
	ARTTurnManager* SetUpWallOnlyTurn(UWorld* World)
	{
		URTHexMapAsset* Mappa = SpawnStopPredicateMap(World);
		if (!Mappa) { return nullptr; }

		const FRTCellId Muraglia(1, 0);
		const FRTHexCellData* Esistente = Mappa->FindCell(Muraglia);
		if (!Esistente) { return nullptr; }
		FRTHexCellData ColMuro = *Esistente;
		ColMuro.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		Mappa->AddOrUpdateCell(ColMuro);
		Mappa->SortCells();

		ARTUnit* Sfondatore = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Dietro     = SpawnStopPredicateUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Sfondatore || !Dietro) { return nullptr; }
		if (!Sfondatore->Abilities.IsValidIndex(0) || !Sfondatore->Abilities[0]) { return nullptr; }

		// La capacita' di sfondare si dichiara sull'istanza: e' l'idioma di tutti gli scenari di struttura.
		Sfondatore->Abilities[0]->Def.Effects.Add(
			FRTActionEffectSpec(ERTActionEffect::DamageStructure, 10));
		Sfondatore->PlannedAbilityIndex = 0;
		Sfondatore->PlannedAttackTarget = Dietro;
		return TM;
	}

	/**
	 * Un turno in cui l'unico atto e' un'AREA su sole celle VUOTE: zero `Attack`, un'impronta.
	 *
	 * 🔑 **E' il caso per cui [D-301] ha creato `AttackFootprint`** — *«il "dopo" perde il caso che
	 * conta»* — e l'unico che esercita il canale dell'impronta **da solo**. Con un colpo in scena una
	 * fermata non direbbe da quale canale e' venuta; qui di colpi non ce n'e' nemmeno uno.
	 *
	 * ⚠️ **`Hero.Aevik.Overload` e' un'azione di CATALOGO ad area** (raggio 1, portata 3): la forma non e'
	 * riscritta a mano sull'istanza, quindi il test non misura come il resolver tratta una forma che
	 * nessuna azione dichiara.
	 */
	ARTTurnManager* SetUpEmptyAreaTurn(UWorld* World)
	{
		SpawnStopPredicateMap(World);
		ARTUnit* Lanciatore = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(2, 2));
		// ⚠️ Il secondo c'e' per non lasciare il turno senza avversari, ma sta LONTANO dall'area: se fosse
		// dentro, un `Attack` nascerebbe e il caso non sarebbe piu' quello delle celle vuote.
		ARTUnit* Lontano = SpawnStopPredicateUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(9, 9));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Lanciatore || !Lontano) { return nullptr; }

		// L'indice 3 di Aevik e' `Overload` (area, raggio 1). Si cerca per identita' invece che per indice:
		// un catalogo riordinato renderebbe il test verde su un'azione diversa.
		int32 Indice = INDEX_NONE;
		for (int32 I = 0; I < Lanciatore->Abilities.Num(); ++I)
		{
			if (Lanciatore->Abilities[I] && Lanciatore->Abilities[I]->Shape == ERTAbilityShape::Area)
			{
				Indice = I;
				break;
			}
		}
		if (Indice == INDEX_NONE) { return nullptr; }

		Lanciatore->PlannedAbilityIndex = Indice;
		Lanciatore->DeclareAttackOnCell(FRTCellId(4, 2)); // a portata, e intorno non c'e' nessuno
		Lanciatore->PlannedCell = FRTCellId(2, 2);
		return TM;
	}

	/** Quanti eventi di un tipo ci sono nella timeline riprodotta. */
	int32 CountStopPredicateEvents(const ARTTurnManager* TM, ERTResolvedEventType Type)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == Type) { ++N; }
		}
		return N;
	}
}

/**
 * `Next Action` si ferma su un atto che **non ha prodotto nessun `Attack`** — `#3292`.
 *
 * 🔴 **E' il difetto per intero, e non dipendeva dall'identita' dell'azione.**
 * `URTPlaybackLibrary::NextActionBoundary` — la funzione che *dichiara* il confine — non aveva un solo
 * chiamante non di test: il `Next Action` che il giocatore preme viveva **dentro il ciclo che rivela i
 * colpi**, e gli altri due canali del `Blast` — impronte e colpi a struttura — erano rivelati da funzioni
 * proprie, poche righe sopra e nello stesso tick, che non valutavano alcun predicato.
 *
 * ⚠️ **L'impronta lo dimostrava senza bisogno di nessuna decisione**: porta gia' un `ActionId` popolato
 * (`#2857`) e `Next Action` non ci si fermava lo stesso. ∴ non era l'identita' a mancare, era il canale a
 * non arrivare.
 *
 * 🔑 **Il caso non e' raro: e' quello che gli eventi esistono per rendere visibile.** Un muro che cade
 * senza vittime da' zero `Attack` e un `StructureHit`; un'area su sole celle vuote da' zero `Attack` e
 * un'impronta — il caso per cui [D-301] ha creato quell'evento. Entrambi sono a schermo: la fase li mostra,
 * li scagliona, riserva loro il tempo. `Next Action` li attraversava senza vederli.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionStopsOnAnActWithoutAttacksTest,
	"RefactorTactics.Playback.NextActionStopsOnAnActWithoutAttacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionStopsOnAnActWithoutAttacksTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpWallOnlyTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	// --- ⛔ LE PREMESSE, e sono ciò che rende il gate discriminante -----------------------------------
	//
	// Senza la prima, una fermata potrebbe venire da un colpo e il test misurerebbe il canale che
	// funzionava gia'. Senza la seconda, non ci sarebbe niente su cui fermarsi e il gate sarebbe verde per
	// assenza.
	if (!TestEqual(TEXT("⛔ premessa: NESSUN Attack in tutto il turno"),
		CountStopPredicateEvents(TM, ERTResolvedEventType::Attack), 0))
	{
		return false;
	}
	if (!TestTrue(TEXT("⛔ premessa: ma un colpo a struttura c'e'"),
		CountStopPredicateEvents(TM, ERTResolvedEventType::StructureHit) > 0))
	{
		return false;
	}

	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
	AdvanceUntilPausedOrDone(TM);

	// --- IL FATTO ------------------------------------------------------------------------------------
	TestTrue(TEXT("🔴 il playback si e' FERMATO su un atto che non ha prodotto nessun colpo"),
		TM->IsPlaybackPaused());
	TestTrue(TEXT("e non e' semplicemente arrivato in fondo"), TM->IsResolving());
	// ⛔ **NEL Blast, non al confine di fase.** `#2855` dichiara che un `Next Action` armato si ferma anche
	// al cambio di fase — *«un cambio di fase e' sempre anche un cambio d'atto»* — quindi senza questa riga
	// il gate sarebbe verde anche con il canale del muro ancora invisibile.
	TestEqual(TEXT("⛔ e si e' fermato NEL Blast: e' il muro, non il confine di fase"),
		TM->GetPlaybackPhaseName(), FString(TEXT("Blast")));
	TestEqual(TEXT("il predicato si e' consumato: la fermata e' sua"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	return true;
}

/**
 * E si ferma anche quando l'unico canale e' l'**IMPRONTA** — `#3292`.
 *
 * 🔴 **E' il caso che dimostra il difetto senza bisogno di nessuna decisione.** `AttackFootprint` porta
 * gia' un `ActionId` popolato (`#2857`), e `Next Action` **non ci si fermava lo stesso**: ∴ non era
 * l'identita' a mancare — quella e' la causa di `#3281` — era il canale a non arrivare al predicato.
 *
 * 🔑 **Un'area su sole celle vuote non produce nessun `Attack`**, ed e' precisamente il caso per cui
 * [D-301] ha creato questo evento: *«il "dopo" perde il caso che conta»*. La fase lo mostra, lo scagliona
 * e gli riserva il tempo; `Next Action` lo attraversava senza vederlo.
 *
 * ⚠️ **E il gemello sul muro non lo copre**: li' il canale e' `StructureHit`. Due canali, due gate — o una
 * mutazione che spegne l'impronta resterebbe verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionStopsOnAFootprintOnlyActTest,
	"RefactorTactics.Playback.NextActionStopsOnAFootprintOnlyAct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionStopsOnAFootprintOnlyActTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpEmptyAreaTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	// --- ⛔ LE PREMESSE: un'impronta, e NESSUN colpo -------------------------------------------------
	if (!TestEqual(TEXT("⛔ premessa: nessun Attack — l'area ha investito solo celle vuote"),
		CountStopPredicateEvents(TM, ERTResolvedEventType::Attack), 0))
	{
		return false;
	}
	if (!TestTrue(TEXT("⛔ premessa: ma un'impronta c'e'"),
		CountStopPredicateEvents(TM, ERTResolvedEventType::AttackFootprint) > 0))
	{
		return false;
	}
	// ⛔ E nemmeno un colpo a struttura: senza questa riga la fermata potrebbe venire dal canale del
	// gemello, e il gate misurerebbe due volte la stessa cosa.
	if (!TestEqual(TEXT("⛔ premessa: e nessun colpo a struttura: il canale e' l'impronta e basta"),
		CountStopPredicateEvents(TM, ERTResolvedEventType::StructureHit), 0))
	{
		return false;
	}

	TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
	AdvanceUntilPausedOrDone(TM);

	// --- IL FATTO ------------------------------------------------------------------------------------
	TestTrue(TEXT("🔴 il playback si e' FERMATO sull'impronta"), TM->IsPlaybackPaused());
	TestTrue(TEXT("e non e' arrivato in fondo"), TM->IsResolving());
	TestEqual(TEXT("⛔ e si e' fermato NEL Blast, non al confine di fase"),
		TM->GetPlaybackPhaseName(), FString(TEXT("Blast")));
	TestEqual(TEXT("il predicato si e' consumato: la fermata e' sua"),
		static_cast<int32>(TM->GetArmedPlaybackStop()), static_cast<int32>(ERTPlaybackStopAt::None));

	return true;
}

/**
 * E **non** si ferma due volte dentro lo stesso intento — `#3292`.
 *
 * 🔑 **E' la meta' che il primo gate non copre, e senza di essa la correzione sarebbe peggio del difetto.**
 * Un intento aggressivo produce un'**attivazione**, un'**impronta** e i **colpi** che ne derivano: piu' fatti,
 * un atto solo. Far passare ogni fatto dal predicato senza la regola giusta avrebbe fermato `Next Action` due
 * volte dentro un colpo solo — cioe' il difetto che `AttackFootprint` documenta gia' (*«una voce per INTENTO,
 * non per vittima»*). ⏱️ *Fino a #3549 erano canali paralleli del Blast; ora sono elementi di una sequenza.*
 *
 * ✅ Non succede, e la ragione sta nel criterio: attivazione, impronta e colpi nascono dallo stesso intento,
 * quindi portano la **stessa** coppia `(sorgente, azione)`, e `IsActBoundary` legge *«piu' eventi con la stessa
 * coppia sono UN atto»* (#3549). La premessa sulla sorgente comune e' asserita qui sotto, su un turno vero: le
 * sorgenti le scrivono tre produttori diversi (l'impronta da `Units[AttackerId]`, il colpo da `Attacker`,
 * l'attivazione dalla sorgente dell'intento), e una divergenza fermerebbe due volte.
 *
 * ⚠️ **E la terza faccia del difetto era proprio qui.** `RequestPlaybackStopAt` congelava l'atto in corso
 * leggendo `PlaybackAttacks[AttacksShown - 1]` — i soli colpi: dopo una fermata sull'impronta il paragone
 * tornava a un'azione **precedente**, e il colpo dello stesso intento sembrava aprire un atto nuovo. Il
 * gate se ne accorge perche' ri-arma il predicato dopo ogni fermata, che e' il modo in cui il comando si
 * usa davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionDoesNotStopTwiceWithinOneIntentTest,
	"RefactorTactics.Playback.NextActionDoesNotStopTwiceWithinOneIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionDoesNotStopTwiceWithinOneIntentTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpTwoPhaseTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	// --- ⛔ LE PREMESSE: un intento solo, e DUE canali che lo raccontano ------------------------------
	//
	// Senza la seconda riga il turno avrebbe un canale solo, e «non si ferma due volte» sarebbe vero per
	// costruzione — il gate verde che non puo' diventare rosso.
	const int32 Colpi = CountStopPredicateEvents(TM, ERTResolvedEventType::Attack);
	const int32 Impronte = CountStopPredicateEvents(TM, ERTResolvedEventType::AttackFootprint);
	if (!TestTrue(TEXT("⛔ premessa: c'e' almeno un colpo"), Colpi > 0)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: e almeno un'impronta, cioe' DUE canali per lo stesso intento"),
		Impronte > 0))
	{
		return false;
	}

	// Le identita' d'azione distinte fra colpi e impronte: e' il numero di ATTI che il Blast contiene.
	TSet<FName> AzioniDistinte;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if ((Ev.Type == ERTResolvedEventType::Attack || Ev.Type == ERTResolvedEventType::AttackFootprint)
			&& !Ev.ActionId.IsNone())
		{
			AzioniDistinte.Add(Ev.ActionId);
		}
	}
	if (!TestEqual(TEXT("⛔ premessa: un atto solo in scena"), AzioniDistinte.Num(), 1)) { return false; }

	// #3549: l'atto ora comincia con la sua ATTIVAZIONE, e resta un atto solo — attivazione, impronta e colpo
	// portano la stessa coppia (sorgente, azione).
	const bool bAttivazione = TM->ResolvedTimelineForTest().ContainsByPredicate([&AzioniDistinte](const FRTResolvedEvent& Ev)
		{ return Ev.Type == ERTResolvedEventType::AbilityActivated && AzioniDistinte.Contains(Ev.ActionId); });
	if (!TestTrue(TEXT("⛔ premessa: l'intento ha la sua attivazione"), bAttivazione)) { return false; }
	// ⛔ E l'attivazione e' IN SEQUENZA per chi guarda: senza, la fermata attesa verrebbe da impronta e colpo
	// soltanto, e la coppia dell'attivazione non sarebbe messa alla prova.
	if (!TestTrue(TEXT("⛔ premessa: l'attivazione e' nella sequenza di Blast"), TM->PlaybackActivationsQueuedForTest() > 0))
	{
		return false;
	}
	// Le sorgenti dei tre produttori coincidono su questo turno: una sola sorgente fra i fatti dell'atto.
	TSet<int32> SorgentiDellAtto;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if ((Ev.Type == ERTResolvedEventType::AbilityActivated || Ev.Type == ERTResolvedEventType::Attack
				|| Ev.Type == ERTResolvedEventType::AttackFootprint)
			&& Ev.Phase == ERTMatchPhase::Blast && AzioniDistinte.Contains(Ev.ActionId))
		{
			SorgentiDellAtto.Add(Ev.SourceStableUnitId);
		}
	}
	TestEqual(TEXT("🔴 attivazione, impronta e colpo dell'intento portano la STESSA sorgente"), SorgentiDellAtto.Num(), 1);

	// --- Si conta quante volte il playback si ferma, RI-ARMANDO ogni volta ---------------------------
	//
	// ⚠️ **Ri-armare e' il modo in cui il comando si usa davvero**, e senza di esso la terza faccia del
	// difetto — l'atto in corso letto da un canale solo — non si manifesterebbe: si vede alla SECONDA
	// pressione, non alla prima.
	// ⚠️ **Si contano SOLO le fermate dentro il `Blast`, e la distinzione non è una comodità.** Un `Next
	// Action` armato si ferma anche al confine di **fase**, ed è comportamento dichiarato e voluto di
	// `#2855`: *«un cambio di fase è sempre anche un cambio d'atto — la fase `Move` contiene il solo
	// `Action.Move`, e nessun atto attraversa due fasi»*. ⛔ Contare quella fermata qui misurerebbe una
	// regola diversa da quella in esame, e il gate sarebbe rosso su un comportamento corretto — come è
	// stato alla prima stesura, prima che la misura lo dicesse.
	int32 FermateNelBlast = 0;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		if (TM->IsPlaybackPaused())
		{
			if (TM->GetPlaybackPhaseName() == TEXT("Blast")) { ++FermateNelBlast; }
			TM->ResumePlayback();
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		else if (TM->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
		{
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		TM->Tick(0.05f);
	}

	// --- IL FATTO ------------------------------------------------------------------------------------
	//
	// 🔴 Un atto, **una** fermata. Due significherebbero che impronta e colpo sono stati letti come due
	// atti — il doppio conteggio che questa correzione deve escludere.
	TestEqual(TEXT("🔴 un intento solo: dentro il Blast il playback si ferma UNA volta"),
		FermateNelBlast, AzioniDistinte.Num());

	// --- #3549: due cure CONSECUTIVE da due unita' diverse sono DUE atti --------------------------------
	//
	// 🔴 La stessa azione generica (`Action.Heal`) da due sorgenti: con il confine sul solo `ActionId` il
	// playback si fermerebbe una volta, e `Next Action` salterebbe il secondo curatore.
	// ✅ Validato per mutazione: `IsActBoundary` sul solo `ActionId` fa cadere questo asserto.
	{
		UWorld* World2 = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("secondo mondo"), World2)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World2); };
		SpawnStopPredicateMap(World2);
		ARTUnit* C1 = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0));
		ARTUnit* Ferito = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(1, 0));
		ARTUnit* C2 = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0));
		ARTTurnManager* TM2 = World2->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM2 || !C1 || !Ferito || !C2) { return false; }
		Ferito->Health = FMath::Max(1, Ferito->Health - 30);
		for (ARTUnit* Curatore : { C1, C2 })
		{
			Curatore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Curatore, TEXT("Action.Heal"));
			Curatore->PlannedAttackTarget = Ferito;
			Curatore->PlannedCell = Curatore->Cell;
		}

		TM2->SetPlaybackControlsEnabled(true);
		TM2->LockInAndResolve();
		if (!TestTrue(TEXT("⛔ premessa: il turno delle cure si riproduce"), TM2->IsResolving())) { return false; }
		// ⛔ Premessa: le DUE attivazioni sono in sequenza per chi guarda, con lo stesso `ActionId` e sorgenti
		// diverse — e' esattamente la coppia che il confine sul solo `ActionId` confonderebbe.
		if (!TestEqual(TEXT("⛔ premessa: due attivazioni in coda"), TM2->PlaybackActivationsQueuedForTest(), 2))
		{
			return false;
		}
		{
			TSet<FName> AzioniCure;
			TSet<int32> SorgentiCure;
			for (const FRTResolvedEvent& Ev : TM2->ResolvedTimelineForTest())
			{
				if (Ev.Type == ERTResolvedEventType::AbilityActivated)
				{
					AzioniCure.Add(Ev.ActionId);
					SorgentiCure.Add(Ev.SourceStableUnitId);
				}
			}
			if (!TestEqual(TEXT("⛔ premessa: le attivazioni portano lo STESSO ActionId"), AzioniCure.Num(), 1)
				|| !TestEqual(TEXT("⛔ premessa: e due sorgenti diverse"), SorgentiCure.Num(), 2))
			{
				return false;
			}
		}

		int32 FermateCure = 0;
		for (int32 I = 0; I < 600 && TM2->IsResolving(); ++I)
		{
			if (TM2->IsPlaybackPaused())
			{
				if (TM2->GetPlaybackPhaseName() == TEXT("Blast")) { ++FermateCure; }
				TM2->ResumePlayback();
				TM2->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
			}
			else if (TM2->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
			{
				TM2->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
			}
			TM2->Tick(0.05f);
		}
		TestEqual(TEXT("🔴 due cure da due unita': DUE fermate"), FermateCure, 2);
	}

	return true;
}

/**
 * L'attivazione e il MURO dello stesso intento sono UN atto — #3549, review del Task 6 (copertura dei siti).
 *
 * 🔑 Il muro prende la sorgente da un produttore suo (`Entry.UnitId` della voce del TurnLog, in
 * `ARTTurnManager::AppendLogEntry`), l'attivazione dall'intento: due strade per lo stesso dato. Se divergessero, `Next Action` si
 * fermerebbe due volte dentro un intento solo — e passa dal percorso REALE (`RequestPlaybackStopAt` →
 * `TickPlayback`), non dalla libreria.
 * ⛔ Premesse: l'attivazione e' nella sequenza di chi guarda, e attivazione e muro portano la stessa coppia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationAndWallAreOneActTest,
	"RefactorTactics.Playback.ActivationAndWallOfOneIntentAreOneAct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationAndWallAreOneActTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpWallOnlyTurn(World);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno sta riproducendo qualcosa"), TM->IsResolving())) { return false; }

	const FRTResolvedEvent* Attivazione = TM->ResolvedTimelineForTest().FindByPredicate([](const FRTResolvedEvent& Ev)
		{ return Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == ERTMatchPhase::Blast; });
	const FRTResolvedEvent* Muro = TM->ResolvedTimelineForTest().FindByPredicate([](const FRTResolvedEvent& Ev)
		{ return Ev.Type == ERTResolvedEventType::StructureHit; });
	if (!TestNotNull(TEXT("⛔ premessa: l'intento si e' attivato nel Blast"), Attivazione)
		|| !TestNotNull(TEXT("⛔ premessa: e ha colpito il muro"), Muro))
	{
		return false;
	}
	if (!TestTrue(TEXT("⛔ premessa: l'attivazione e' nella sequenza di chi guarda"), TM->PlaybackActivationsQueuedForTest() > 0))
	{
		return false;
	}
	TestEqual(TEXT("🔴 il muro porta la sorgente dell'attivazione"), Muro->SourceStableUnitId, Attivazione->SourceStableUnitId);
	TestEqual(TEXT("🔴 e la sua azione"), Muro->ActionId, Attivazione->ActionId);

	int32 FermateNelBlast = 0;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		if (TM->IsPlaybackPaused())
		{
			if (TM->GetPlaybackPhaseName() == TEXT("Blast")) { ++FermateNelBlast; }
			TM->ResumePlayback();
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		else if (TM->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
		{
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		TM->Tick(0.05f);
	}
	TestEqual(TEXT("🔴 attivazione e muro di un intento: UNA fermata nel Blast"), FermateNelBlast, 1);
	return true;
}

/**
 * `Next Action` si ferma sulle attivazioni di Prep e di Dash — #3549, spec §2.4 (confini d'atto).
 *
 * 🔑 Prima di #3549 le due fasi non avevano un canale che passasse dal predicato: `Next Action` le attraversava
 * fermandosi solo al cambio di fase. Ora i rami Prep e Dash aggiornano l'atto in corso come il Blast.
 * 🔢 Le fermate attese: Prep e' la PRIMA fase (nessuna fermata d'ingresso) e ha un'attivazione → 1; nel Dash
 * si entra con una fermata di cambio fase, poi l'attivazione → 2.
 * ✅ Validato per mutazione: togliere `NotePlaybackActShown` da `RevealPlaybackActivations` porta le fermate a
 * 0 in Prep e 1 nel Dash.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionStopsOnPrepAndDashTest,
	"RefactorTactics.Playback.NextActionStopsOnPrepAndDashActivations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionStopsOnPrepAndDashTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnStopPredicateMap(World);

	ARTUnit* Scudo      = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 1));
	ARTUnit* Caricatore = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnStopPredicateUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Scudo || !Caricatore || !Bersaglio) { return false; }

	auto Indice = [](const ARTUnit* U, const TCHAR* Id)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			if (U->GetAbility(i) && U->GetAbility(i)->Def.ActionId == FName(Id)) { return i; }
		}
		return static_cast<int32>(INDEX_NONE);
	};
	Scudo->PlannedAbilityIndex = Indice(Scudo, TEXT("Hero.Muiren.TideGuard"));
	Scudo->PlannedCell = Scudo->Cell;
	Caricatore->PlannedDashAbility = Indice(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	// Viewer: senza controller `TeamIdOf` ripiega sulla squadra 0, che e' quella delle due sorgenti. Il refresh
	// e' la precondizione delle attivazioni di Prep e Dash (spec §2.5).
	TM->RefreshTeamKnowledgeNow();
	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ il turno si riproduce"), TM->IsResolving())) { return false; }

	int32 FermatePrep = 0, FermateDash = 0;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		if (TM->IsPlaybackPaused())
		{
			if (TM->GetPlaybackPhaseName() == TEXT("Prep")) { ++FermatePrep; }
			if (TM->GetPlaybackPhaseName() == TEXT("Dash")) { ++FermateDash; }
			TM->ResumePlayback();
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		else if (TM->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
		{
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		TM->Tick(0.05f);
	}
	TestEqual(TEXT("🔴 Prep: una fermata, sull'attivazione"), FermatePrep, 1);
	TestEqual(TEXT("🔴 Dash: il cambio di fase e l'attivazione"), FermateDash, 2);
	return true;
}

/**
 * La regola del confine d'atto ha **una sola** implementazione — `#3292`.
 *
 * 🔴 **Ne aveva due, e una non era eseguita.** `NextActionBoundary` la conteneva e non aveva un solo
 * chiamante non di test; il ciclo dei colpi di `TickPlayback` la **riscriveva** inline, ed era l'unico
 * sito che la valutasse davvero. Il commento accanto alla copia dichiarava *«la regola e' quella di
 * `NextActionBoundary`, non una seconda»*: vero sull'intenzione, falso sul codice.
 *
 * ⚠️ **Questo gate non conta i chiamanti: misura che le due FORME diano la stessa risposta.** Un `grep`
 * direbbe soltanto che qualcuno chiama qualcosa; qui si verifica che la vista-su-timeline
 * (`NextActionBoundary`) e il predicato per-evento (`IsActBoundary`) — cioe' cio' che il playback valuta a
 * ogni fatto rivelato — non possano divergere. Se qualcuno riscrivesse una delle due, questo diventa rosso.
 *
 * ⛔ **L'atto corrente lo calcola il test con la scansione all'indietro**, che e' logica di
 * `NextActionBoundary` e non della regola: replicarla qui e' cio' che rende il confronto possibile senza
 * chiedere alla funzione di raccontarsi da sola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActBoundaryRuleHasOneImplementationTest,
	"RefactorTactics.Playback.ActBoundaryRuleHasOneImplementation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActBoundaryRuleHasOneImplementationTest::RunTest(const FString&)
{
	auto Evento = [](ERTResolvedEventType Type, const TCHAR* Azione, int32 Sorgente = 0)
	{
		FRTResolvedEvent Ev;
		Ev.Type = Type;
		Ev.ActionId = (Azione != nullptr) ? FName(Azione) : NAME_None;
		Ev.SourceStableUnitId = Sorgente;
		return Ev;
	};

	// Una timeline che tocca tutti i rami: azione uguale, azione diversa, vuoto su un tipo ordinario, e
	// vuoto su `StructureHit` — dove il vuoto **e'** un confine ([D-437]).
	TArray<FRTResolvedEvent> Timeline;
	Timeline.Add(Evento(ERTResolvedEventType::AttackFootprint, TEXT("Action.A")));
	Timeline.Add(Evento(ERTResolvedEventType::Attack,          TEXT("Action.A"))); // stesso atto
	Timeline.Add(Evento(ERTResolvedEventType::Defeated,        nullptr));          // vuoto ordinario
	Timeline.Add(Evento(ERTResolvedEventType::StructureHit,    nullptr));          // vuoto = aggregato
	Timeline.Add(Evento(ERTResolvedEventType::Attack,          TEXT("Action.B"))); // atto nuovo
	Timeline.Add(Evento(ERTResolvedEventType::StructureHit,    TEXT("Action.B"))); // stesso atto
	Timeline.Add(Evento(ERTResolvedEventType::ArcHit,          nullptr));          // vuoto: NON un confine
	Timeline.Add(Evento(ERTResolvedEventType::AbilityActivated, TEXT("Action.Heal"), 1)); // atto nuovo
	Timeline.Add(Evento(ERTResolvedEventType::AbilityActivated, TEXT("Action.Heal"), 2)); // #3549: altra sorgente = confine

	// ⛔ ANTI-VACUITA': i due rami devono essere entrambi esercitati, o il confronto sarebbe fra due
	// risposte sempre uguali per costruzione.
	int32 Confini = 0;
	int32 NonConfini = 0;

	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		// L'atto in corso a `i-1`, con la stessa scansione all'indietro di `NextActionBoundary`.
		FName Corrente = NAME_None;
		int32 SorgenteCorrente = 0;
		for (int32 k = i - 1; k >= 0; --k)
		{
			if (!Timeline[k].ActionId.IsNone())
			{
				Corrente = Timeline[k].ActionId;
				SorgenteCorrente = Timeline[k].SourceStableUnitId;
				break;
			}
		}

		const bool bPredicato = URTPlaybackLibrary::IsActBoundary(Timeline[i], Corrente, SorgenteCorrente);
		const bool bVista = (URTPlaybackLibrary::NextActionBoundary(Timeline, i - 1) == i);
		if (bPredicato) { ++Confini; } else { ++NonConfini; }

		TestEqual(*FString::Printf(
			TEXT("🔴 evento %d: la vista-su-timeline e il predicato per-evento danno la STESSA risposta"), i),
			bVista, bPredicato);
	}

	TestTrue(TEXT("⛔ anti-vacuita': almeno un confine e' stato esercitato"), Confini > 0);
	TestTrue(TEXT("⛔ e almeno un NON-confine"), NonConfini > 0);

	// I due casi che la regola distingue e che nessun altro test di questo file tocca.
	TestTrue(TEXT("un `StructureHit` senza azione E' un confine ([D-437])"),
		URTPlaybackLibrary::IsActBoundary(
			Evento(ERTResolvedEventType::StructureHit, nullptr), FName(TEXT("Action.A"))));
	TestFalse(TEXT("⛔ ma un `ArcHit` senza azione NO: la sua identita' non e' decisa (#3280)"),
		URTPlaybackLibrary::IsActBoundary(
			Evento(ERTResolvedEventType::ArcHit, nullptr), FName(TEXT("Action.A"))));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
