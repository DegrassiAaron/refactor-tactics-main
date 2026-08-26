// Validazione di stress 4v4 (epic E17, CP 17.1 e CP 17.2).
//
// L'epic e' **misura, non produzione**: serve a sapere dove il sistema si rompe con otto unita', non a
// dichiarare il 4v4 un formato. Il formato competitivo finale non e' deciso — 3v3 e' la baseline di studio,
// 4v4 lo stress test — e nessuno di questi test va letto come una decisione in merito.
//
// Il difetto che l'epic cerca e' un `if (Num == 2)` da qualche parte: un resolver che sa quante unita' ci
// sono. Se esiste, va corretto e non aggirato, ed e' la ragione per cui
// `Stress.NoTeamSizeAssumptionInResolver` interroga QUATTRO dimensioni di squadra invece di confrontare 4v4
// col 2v2: due dimensioni non distinguono «funziona per ogni N» da «funziona per i due N che ho provato».

#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h" // MakeFlatArena: un solo builder di arena piatta
#include "RTWorldFixtures.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti per file: la unity build condivide la translation unit.

	// Il mondo di prova vive in `RTWorldFixtures.h`: era dichiarato qui con un nome tutto suo perche' la
	// unity build fonde le translation unit e due namespace anonimi con la stessa funzione collidono. Un
	// namespace NOMINATO non ha quel problema. Le chiamate restano qualificate: in unity build questo
	// namespace anonimo e' lo stesso di ogni altro file di test.

	/**
	 * L'arena dello stress: piu' ampia della showcase (r=5) e con TRE direttrici.
	 *
	 * Le tre rotte nascono da due barriere parziali, non da un disegno a mano: con otto unita' su un campo
	 * aperto il primo contatto avviene ovunque contemporaneamente, e cio' che si misura e' una mischia, non
	 * un formato. ⚠️ **Limite dichiarato**: la direttrice «centro/obiettivo» del checkpoint e' qui solo
	 * geometrica — un obiettivo contendibile e' CP 10.2, che non e' atterrato. Va letta come corridoio
	 * centrale, non come punto di controllo.
	 */
	URTHexMapAsset* MakeStressArena(UWorld* World, int32 Radius = 8)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		// Due barriere verticali che lasciano tre varchi: nord, centro, sud. Celle che bloccano passo e vista,
		// cioe' il dato che il progetto usa gia' per la copertura alta — nessuna geometria nuova.
		auto Wall = [M](int32 q, int32 r)
		{
			const FRTCellId Id(q, r, 0);
			if (!M->ContainsCell(Id)) { return; }
			FRTHexCellData Data = M->FindCell(Id) ? *M->FindCell(Id) : FRTHexCellData(Id);
			Data.bBlocksMovement = true;
			Data.bBlocksLineOfSight = true;
			M->AddOrUpdateCell(Data);
		};
		for (int32 r = -6; r <= -3; ++r) { Wall(-2, r); }
		for (int32 r = 2; r <= 5; ++r)   { Wall(-2, r); }
		for (int32 r = -6; r <= -3; ++r) { Wall(2, r); }
		for (int32 r = 2; r <= 5; ++r)   { Wall(2, r); }

		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnStressUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true; // bot contro bot: la partita si gioca da sola, nessuna mano umana
		// Senza `BeginPlay` i cooldown non sono inizializzati e ogni abilita' risulta sempre pronta: si
		// misurerebbe un gioco che non esiste.
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Il roster CORE, nell'ordine del canone: Gadget · Phase · Riktor · Wraith.
	 *
	 * Quattro eroi per squadra e non «quattro copie di due»: e' il roster della v0.1, ed e' cio' che rende il
	 * 4v4 uno stress del sistema invece che dello stesso eroe moltiplicato.
	 */
	TArray<URTHeroData*> StressCoreRoster(int32 Count)
	{
		TArray<URTHeroData*> Roster;
		const TArray<URTHeroData*> All = {
			URTHeroCatalogLibrary::MakeGadget(),
			URTHeroCatalogLibrary::MakePhase(),
			URTHeroCatalogLibrary::MakeRiktor(),
			URTHeroCatalogLibrary::MakeWraith(),
		};
		for (int32 i = 0; i < Count; ++i)
		{
			Roster.Add(All[i % All.Num()]);
		}
		return Roster;
	}

	/** Celle di partenza di una squadra: `Count` celle contigue sul proprio lato dell'arena. */
	TArray<FRTCellId> StressStartCells(int32 TeamId, int32 Count)
	{
		TArray<FRTCellId> Cells;
		const int32 Sign = (TeamId == 0) ? -1 : 1;
		for (int32 i = 0; i < Count && i < 4; ++i)
		{
			// Colonna sul proprio bordo (|q| = 7): quattro celle contigue, tutte a distanza esagonale 7 dal
			// centro — dentro il raggio 8 — e lontane dalle barriere, che stanno a |q| = 2.
			Cells.Add(FRTCellId(Sign * 7, -Sign * (3 + i), 0));
		}
		return Cells;
	}

	/** Un turno completo: pianificazione dei bot, risoluzione e playback fino in fondo. */
	void PlayStressTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Le unita' vive del mondo, in ordine di `StableUnitId` (mai l'ordine di `GetAllActorsOfClass`). */
	TArray<ARTUnit*> StressUnits(UWorld* World, bool bOnlyAlive)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
		TArray<ARTUnit*> Units;
		for (AActor* Actor : Actors)
		{
			ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (Unit && (!bOnlyAlive || Unit->IsAlive())) { Units.Add(Unit); }
		}
		// `GetAllActorsOfClass` non promette un ordine: ordinare qui evita che un test dipenda da come
		// l'engine ha allocato gli attori (invariante #3).
		Units.Sort([](const ARTUnit& A, const ARTUnit& B)
		{
			if (A.TeamId != B.TeamId) { return A.TeamId < B.TeamId; }
			return A.StableUnitId < B.StableUnitId;
		});
		return Units;
	}
}

/**
 * CP 17.1 — il roster core a QUATTRO per squadra gioca una partita intera.
 *
 * La proprieta' e' che nessuna riga del resolver e' stata toccata per farlo: si cambia solo la composizione,
 * che dal CP 19.2 e' un dato del formato (`FRTMatchRules::UnitsPerTeam`) e non una proprieta' del `GameMode`.
 *
 * Le invarianti per turno sono le stesse di `HexMatch.PlaysToCompletion`, e questo e' voluto: se con otto
 * unita' cadono, cade una proprieta' che il 2v2 gia' garantiva — che e' esattamente cio' che uno stress test
 * deve poter dire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStressCoreRoster4v4CompletesTest,
	"RefactorTactics.Stress.CoreRoster4v4Completes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStressCoreRoster4v4CompletesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	URTHexMapAsset* Map = MakeStressArena(World);

	const TArray<URTHeroData*> Roster = StressCoreRoster(4);
	for (int32 Team = 0; Team < 2; ++Team)
	{
		const TArray<FRTCellId> Cells = StressStartCells(Team, 4);
		for (int32 i = 0; i < 4; ++i)
		{
			SpawnStressUnit(World, Team, Roster[i], Cells[i]);
		}
	}

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM) { RTWorldFixtures::DestroyWorld(World); return false; }

	// Otto unita' distinte, ognuna sulla propria cella: se l'allestimento gia' ne perdesse una, il resto del
	// test misurerebbe un 4v3 chiamandolo 4v4.
	{
		const TArray<ARTUnit*> Start = StressUnits(World, /*bOnlyAlive=*/ true);
		TestEqual(TEXT("otto unita' vive all'avvio"), Start.Num(), 8);
		TSet<FRTCellId> StartCells;
		for (const ARTUnit* U : Start) { StartCells.Add(U->Cell); }
		TestEqual(TEXT("otto celle di partenza distinte"), StartCells.Num(), Start.Num());
	}

	// Tetto di sicurezza, non una regola: serve a fallire invece di girare all'infinito. Piu' largo del 2v2
	// perche' con otto unita' la partita puo' legittimamente durare di piu' — il DATO e' quanto dura, e viene
	// registrato sotto.
	const int32 MaxTurns = 60;
	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < MaxTurns)
	{
		PlayStressTurn(TM);
		++TurnsPlayed;

		TSet<FRTCellId> Occupied;
		for (const ARTUnit* Unit : StressUnits(World, /*bOnlyAlive=*/ true))
		{
			TestTrue(FString::Printf(TEXT("turno %d: %s e' su una cella della mappa"), TurnsPlayed, *Unit->GetName()),
				Map->ContainsCell(Unit->Cell));
			TestFalse(FString::Printf(TEXT("turno %d: nessuna sovrapposizione su %s"), TurnsPlayed, *Unit->Cell.ToString()),
				Occupied.Contains(Unit->Cell));
			Occupied.Add(Unit->Cell);
		}
	}

	TestTrue(TEXT("la partita 4v4 si e' decisa entro il tetto di sicurezza"),
		TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestTrue(TEXT("sono stati giocati piu' turni (non si e' decisa al primo)"), TurnsPlayed > 1);

	// La misura, che e' il punto dell'epic. Si registra anche se fuori dalla banda 10-14 del 2v2: serve a
	// sapere dove siamo, non a superare un esame.
	AddInfo(FString::Printf(TEXT("[E17] partita 4v4 decisa in %d turni (banda 2v2 di riferimento: 10-14)"),
		TurnsPlayed));
	UE_LOG(LogTemp, Display, TEXT("[E17] 4v4 su arena r=8, tre direttrici: decisa in %d turni"), TurnsPlayed);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * CP 17.1 — il resolver non sa quante unita' ci sono per squadra.
 *
 * Quattro dimensioni e non due: con il solo confronto 2v2 / 4v4 passerebbe un resolver che tratta «2» e
 * «diverso da 2» come due casi, che e' il difetto scritto in forma diversa. L'oracolo e' la COPERTURA del
 * TurnLog — ogni unita' viva deve comparire almeno una volta dopo un turno — perche' e' l'unico segnale che
 * distingue «l'unita' ha deciso di non fare niente» da «il resolver non l'ha vista».
 *
 * ⚠️ Limite dichiarato: la copertura si misura su `FRTTurnLogEntry::UnitId`, che vale `0` per le voci
 * ambientali (D-063) e viene popolato solo dalla prima risoluzione in poi. Il test gioca quindi un turno
 * intero prima di leggere, e ignora lo `0`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStressNoTeamSizeAssumptionTest,
	"RefactorTactics.Stress.NoTeamSizeAssumptionInResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStressNoTeamSizeAssumptionTest::RunTest(const FString&)
{
	for (int32 PerTeam = 1; PerTeam <= 4; ++PerTeam)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		MakeStressArena(World);

		const TArray<URTHeroData*> Roster = StressCoreRoster(PerTeam);
		for (int32 Team = 0; Team < 2; ++Team)
		{
			const TArray<FRTCellId> Cells = StressStartCells(Team, PerTeam);
			for (int32 i = 0; i < PerTeam; ++i)
			{
				SpawnStressUnit(World, Team, Roster[i], Cells[i]);
			}
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { RTWorldFixtures::DestroyWorld(World); return false; }

		const int32 Expected = PerTeam * 2;
		TestEqual(FString::Printf(TEXT("%dv%d: allestite %d unita'"), PerTeam, PerTeam, Expected),
			StressUnits(World, /*bOnlyAlive=*/ true).Num(), Expected);

		PlayStressTurn(TM);

		// Gli id stabili sono assegnati dalla prima risoluzione in poi: raccoglierli PRIMA darebbe tutti `0`.
		TSet<int32> AliveIds;
		for (const ARTUnit* Unit : StressUnits(World, /*bOnlyAlive=*/ true))
		{
			AliveIds.Add(Unit->StableUnitId);
		}

		TSet<int32> InLog;
		for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
		{
			if (Entry.UnitId != 0) { InLog.Add(Entry.UnitId); }
		}

		// Ogni unita' viva ha lasciato traccia: e' cio' che un `if (Num == 2)` romperebbe per prima cosa,
		// lasciando fuori dal turno le unita' oltre la seconda.
		for (int32 Id : AliveIds)
		{
			TestTrue(FString::Printf(TEXT("%dv%d: l'unita' %d compare nel TurnLog del primo turno"),
				PerTeam, PerTeam, Id), InLog.Contains(Id));
		}

		// Il numero di identita' distinte cresce con la dimensione: senza questo, un TurnLog che nomina
		// sempre le stesse due unita' passerebbe il controllo qui sopra ogni volta che le altre sono morte.
		TestEqual(FString::Printf(TEXT("%dv%d: le identita' vive sono %d"), PerTeam, PerTeam, Expected),
			AliveIds.Num(), Expected);

		RTWorldFixtures::DestroyWorld(World);
	}

	return true;
}

/**
 * CP 17.2 — lo stesso scenario 4v4, ripetuto, da' lo stesso `LogHash` e lo stesso `StateHash`.
 *
 * Due esecuzioni e non una: e' la definizione di divergenza zero. Il confronto e' per TURNO e non solo alla
 * fine, perche' un hash finale uguale non esclude che due partite siano passate per stati diversi — e la
 * diagnosi «diverge dal turno N» e' l'unica utile a chi deve ripararla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStressReplayDivergenceZeroAt4v4Test,
	"RefactorTactics.Stress.ReplayDivergenceZeroAt4v4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStressReplayDivergenceZeroAt4v4Test::RunTest(const FString&)
{
	// Una esecuzione: gli hash del TurnLog turno per turno, e lo stato finale.
	struct FRun
	{
		TArray<uint32> PerTurnLogHash;
		uint32 FinalStateHash = 0;
		int32 Turns = 0;
	};

	auto PlayOnce = [this]() -> FRun
	{
		FRun Run;
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return Run; }
		URTHexMapAsset* Map = MakeStressArena(World);

		const TArray<URTHeroData*> Roster = StressCoreRoster(4);
		for (int32 Team = 0; Team < 2; ++Team)
		{
			const TArray<FRTCellId> Cells = StressStartCells(Team, 4);
			for (int32 i = 0; i < 4; ++i)
			{
				SpawnStressUnit(World, Team, Roster[i], Cells[i]);
			}
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { RTWorldFixtures::DestroyWorld(World); return Run; }

		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Run.Turns < 60)
		{
			PlayStressTurn(TM);
			++Run.Turns;
			// Il TurnLog e' per turno (`TurnLog.Reset()` a inizio risoluzione): l'hash si prende subito, o
			// il turno successivo lo cancella.
			Run.PerTurnLogHash.Add(URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog()));
		}

		// Stato finale: le unita' SOPRAVVISSUTE — le cadute sono gia' state distrutte dal `TurnManager`, e
		// `BuildUnitDigests` le includerebbe se ci fossero ancora. E' un limite di questo punto di misura,
		// non della funzione: due partite che finiscono con gli stessi vivi e caduti diversi qui non si
		// distinguerebbero. La sequenza degli hash per turno lo copre.
		const TArray<ARTUnit*> Survivors = StressUnits(World, /*bOnlyAlive=*/ false);
		const TArray<int32> Scores = { TM->GetTeamScore(0), TM->GetTeamScore(1) };
		Run.FinalStateHash = URTMatchStateHashLibrary::HashMatchState(
			Map, URTMatchStateHashLibrary::BuildUnitDigests(Survivors), Scores);

		RTWorldFixtures::DestroyWorld(World);
		return Run;
	};

	const FRun A = PlayOnce();
	const FRun B = PlayOnce();

	TestTrue(TEXT("la prima esecuzione ha giocato dei turni"), A.Turns > 1);
	TestEqual(TEXT("le due esecuzioni giocano lo stesso numero di turni"), B.Turns, A.Turns);

	// Diagnosi al PRIMO turno divergente: un solo `TestEqual` sull'ultimo hash direbbe «diverge» senza dire
	// dove, ed e' precisamente il dato che serve.
	const int32 Common = FMath::Min(A.PerTurnLogHash.Num(), B.PerTurnLogHash.Num());
	int32 FirstDivergentTurn = INDEX_NONE;
	for (int32 T = 0; T < Common; ++T)
	{
		if (A.PerTurnLogHash[T] != B.PerTurnLogHash[T]) { FirstDivergentTurn = T + 1; break; }
	}
	TestEqual(TEXT("nessun turno diverge fra due esecuzioni identiche"), FirstDivergentTurn, INDEX_NONE);
	TestEqual(TEXT("lo stato finale ha lo stesso checksum"), B.FinalStateHash, A.FinalStateHash);

	AddInfo(FString::Printf(TEXT("[E17] divergenza replay 4v4: 0 su %d turni confrontati"), Common));
	return true;
}

/**
 * CP 17.2 — le metriche del resolver con otto unita', registrate.
 *
 * Non c'e' una soglia da superare: il DoD chiede di ALLEGARE le misure, «anche se fuori target». La sola
 * assertion e' un tetto larghissimo, che cattura la regressione catastrofica e non il rumore — la stessa
 * disciplina di `Perf.TurnResolverMedian`.
 *
 * ⚠️ **Due delle cinque metriche richieste non sono misurabili qui, e vanno dichiarate invece che stimate**:
 * «opportunity di reazione per turno» e «prompt manuali per turno» richiedono che le opportunity siano
 * prodotte dal `TurnManager` durante la risoluzione, cioe' **CP 14.5**. CP 14.4 ha la funzione che le
 * costruisce (`BuildOverwatchTriggers`) ma nessuno la chiama ancora in partita: un numero riportato oggi
 * sarebbe `0` per assenza di produttore, non per assenza di reazioni, e le due cose si leggono uguali.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStressResolverAt4v4Test,
	"RefactorTactics.Perf.ResolverAt4v4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStressResolverAt4v4Test::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	MakeStressArena(World);

	const TArray<URTHeroData*> Roster = StressCoreRoster(4);
	for (int32 Team = 0; Team < 2; ++Team)
	{
		const TArray<FRTCellId> Cells = StressStartCells(Team, 4);
		for (int32 i = 0; i < 4; ++i)
		{
			SpawnStressUnit(World, Team, Roster[i], Cells[i]);
		}
	}

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM) { RTWorldFixtures::DestroyWorld(World); return false; }
	// Playback DISATTIVATO: qui si misura la risoluzione autoritativa, non la sua riproduzione nel tempo,
	// che e' presentazione (invariante #1).
	TM->bEnablePlayback = false;

	TArray<double> ResolveMs;
	TArray<int32> EventsPerTurn;
	TArray<int32> LogBytesPerTurn;

	for (int32 Turn = 0; Turn < 12 && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
	{
		TM->PlanBotsForTest();
		const double Start = FPlatformTime::Seconds();
		TM->LockInAndResolve();
		ResolveMs.Add((FPlatformTime::Seconds() - Start) * 1000.0);

		const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
		EventsPerTurn.Add(Log.Num());
		// Dimensione del TurnLog come NUMERO DI VOCI per la struttura serializzata: la dimensione in byte
		// dipende dal formato su file, che ha un owner suo (`spec-turnlog-serialize.md`). Qui si registra
		// cio' che questo punto di misura conosce davvero.
		LogBytesPerTurn.Add(Log.Num() * static_cast<int32>(sizeof(FRTTurnLogEntry)));
	}

	if (!TestTrue(TEXT("sono stati misurati dei turni"), ResolveMs.Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	auto Median = [](TArray<double> V)
	{
		V.Sort();
		return V.Num() % 2 ? V[V.Num() / 2] : 0.5 * (V[V.Num() / 2 - 1] + V[V.Num() / 2]);
	};
	auto MeanInt = [](const TArray<int32>& V)
	{
		if (V.Num() == 0) { return 0.0; }
		int64 Sum = 0;
		for (int32 X : V) { Sum += X; }
		return static_cast<double>(Sum) / V.Num();
	};

	const double MedianMs = Median(ResolveMs);
	const FString Report = FString::Printf(
		TEXT("[E17] 4v4, arena r=8, playback off, %d turni: resolver mediana %.3f ms/turno · ")
		TEXT("voci TurnLog %.1f/turno · TurnLog %.0f byte/turno · opportunity di reazione: NON MISURABILE ")
		TEXT("(nessun produttore in partita finche' CP 14.5 non atterra) · prompt manuali: NON MISURABILE (idem)"),
		ResolveMs.Num(), MedianMs, MeanInt(EventsPerTurn), MeanInt(LogBytesPerTurn));
	AddInfo(Report);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Report);

	// Tetto larghissimo, come per il 2v2: il target PDR e' < 100 ms/turno e questa assertion non lo
	// sostituisce — cattura la regressione di ordine di grandezza, non lo scostamento.
	TestTrue(FString::Printf(TEXT("il resolver 4v4 resta nell'ordine di grandezza atteso (%.3f ms)"), MedianMs),
		MedianMs < 1000.0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
