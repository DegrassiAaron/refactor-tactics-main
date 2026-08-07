#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTCellId.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Fixture della showcase «Il Relè» — CP 15.2 (issue #168).
 *
 * NON e' lo scenario degli 8 turni (CP 15.4) e NON scripta gli intenti (CP 15.3): qui si verifica che la
 * fixture esista, sia documentata e sia STABILE. Una fixture che cambia layout fra due generazioni rende
 * inutile ogni golden replay costruito sopra, e il difetto non si vedrebbe mai in un test di gioco.
 */
namespace
{
	/** Le celle non-Floor attese, con la superficie che la fixture dichiara. Ordine irrilevante. */
	struct FRTShowcaseExpectedSurface
	{
		FRTCellId Cell;
		ERTHexSurface Surface;
	};

	TArray<FRTShowcaseExpectedSurface> ShowcaseExpectedSurfaces()
	{
		return {
			// Acqua al centro: la spina dorsale della combo acqua+elettricita' (il payoff arriva a CP 8.3).
			{ FRTCellId(0,  0, 0), ERTHexSurface::ShallowWater },
			{ FRTCellId(0, -1, 0), ERTHexSurface::ShallowWater },
			{ FRTCellId(0,  1, 0), ERTHexSurface::ShallowWater },
			// Conduttivo a contatto con l'acqua, in coppia simmetrica.
			{ FRTCellId( 1, -1, 0), ERTHexSurface::Conductive },
			{ FRTCellId(-1,  1, 0), ERTHexSurface::Conductive },
			// Rough: nega Dash/Charge lungo una delle due vie d'avvicinamento.
			{ FRTCellId(-2, -1, 0), ERTHexSurface::Rough },
			{ FRTCellId( 2,  1, 0), ERTHexSurface::Rough },
			// Ice: scivolamento involontario di chi TERMINA il Move qui.
			{ FRTCellId(-2,  2, 0), ERTHexSurface::Ice },
			{ FRTCellId( 2, -2, 0), ERTHexSurface::Ice },
			// Fire: danno on-enter dal catalogo terreni.
			{ FRTCellId(0, -2, 0), ERTHexSurface::Fire },
			{ FRTCellId(0,  2, 0), ERTHexSurface::Fire },
			// Smoke: cap del targeting a 2 celle.
			{ FRTCellId(-1, -2, 0), ERTHexSurface::Smoke },
			{ FRTCellId( 1,  2, 0), ERTHexSurface::Smoke },
		};
	}

	// --- Esecuzione dello scenario Lite -----------------------------------------------------------
	// Gli helper portano il prefisso `Showcase`: nella unity build questo file finisce in un'unita' di
	// traduzione condivisa e un nome generico collide con quello di un altro file di test (gotcha noto).

	UWorld* MakeShowcaseWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyShowcaseWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	const URTHeroData* FindShowcaseHero(FName HeroId)
	{
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId) { return Hero; }
		}
		return nullptr;
	}

	/** Unita' della showcase: eroe dal CATALOGO (non `ConfigureAsArchetype`, che e' ormai solo test-legacy). */
	ARTUnit* SpawnShowcaseHero(UWorld* World, const FRTShowcaseSpawn& Spawn)
	{
		const URTHeroData* Hero = FindShowcaseHero(Spawn.HeroId);
		if (!World || !Hero) { return nullptr; }

		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = Spawn.TeamId;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = true; // nessuna mano umana: gli input sono un dato (lo scripting vero e' CP 15.3)
		// Senza BeginPlay i cooldown non esistono e ogni abilita' risulta sempre pronta: si misurerebbe
		// un gioco che non esiste.
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Spawn.Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return Unit;
	}

	void PlayOneShowcaseTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Cio' che una partita della fixture produce di osservabile: l'arena, il log, il suo hash, lo stato finale. */
	struct FRTShowcaseRun
	{
		bool bValid = false;
		/**
		 * Hash dell'arena su cui la partita e' stata giocata. Sta qui perche' senza di esso il test e' piu'
		 * debole di quanto sembri: una fixture che deriva su una cella che nessuno calpesta produce log e
		 * stato finale identici, e la deriva passerebbe inosservata proprio nel test che deve fermarla.
		 * (Verificato con una mutazione: senza questo campo il test restava verde.)
		 */
		uint32 ArenaHash = 0;
		TArray<uint32> TurnLogHashes;
		TArray<FString> LogLines;
		TArray<FString> FinalState;
	};

	FRTShowcaseRun RunShowcaseLite(int32 NumTurns)
	{
		FRTShowcaseRun Run;

		UWorld* World = MakeShowcaseWorld();
		if (!World) { return Run; }

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(World);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (!Arena || !MapActor) { DestroyShowcaseWorld(World); return Run; }
		MapActor->MapAsset = Arena;
		Run.ArenaHash = Arena->ComputeHash();

		for (const FRTShowcaseSpawn& Spawn : URTMatchSetupLibrary::GetShowcaseRelayLiteSpawns())
		{
			if (SpawnShowcaseHero(World, Spawn) == nullptr) { DestroyShowcaseWorld(World); return Run; }
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { DestroyShowcaseWorld(World); return Run; }

		for (int32 Turn = 1; Turn <= NumTurns && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
		{
			PlayOneShowcaseTurn(TM);

			const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
			Run.TurnLogHashes.Add(URTTurnLogLibrary::HashTurnLog(Log));
			for (const FRTTurnLogEntry& Entry : Log)
			{
				Run.LogLines.Add(FString::Printf(TEXT("T%d %s"), Turn, *URTTurnLogLibrary::DescribeEntry(Entry)));
			}
		}

		// Lo stato finale, non solo il racconto: due partite possono loggare uguale e finire diverse.
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
		for (const AActor* Actor : Actors)
		{
			const ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit) { continue; }
			Run.FinalState.Add(FString::Printf(TEXT("%s team=%d hp=%d shield=%d cell=%s alive=%d"),
				*Unit->HeroId.ToString(), Unit->TeamId, Unit->Health, Unit->Shield,
				*Unit->Cell.ToString(), Unit->IsAlive() ? 1 : 0));
		}
		Run.FinalState.Sort(); // l'ordine degli Actor non e' materia di questo test: lo e' il contenuto

		Run.bValid = true;
		DestroyShowcaseWorld(World);
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseFixtureLayoutTest,
	"RefactorTactics.ShowcaseRelay.FixtureLayoutIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseFixtureLayoutTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(GetTransientPackage());
	if (!TestNotNull(TEXT("l'arena della showcase esiste"), Arena))
	{
		return false;
	}

	// Esagono pieno di raggio 5 sul layer 0: 3*R*(R+1)+1 = 91 celle.
	TestEqual(TEXT("numero di celle dell'arena"), Arena->NumCells(), 91);
	TestEqual(TEXT("un solo layer"), Arena->GetLayers().Num(), 1);

	// --- Superfici dichiarate ---------------------------------------------------------------------
	for (const FRTShowcaseExpectedSurface& Expected : ShowcaseExpectedSurfaces())
	{
		const FRTHexCellData* Data = Arena->FindCell(Expected.Cell);
		if (!TestNotNull(*FString::Printf(TEXT("la cella %s esiste"), *Expected.Cell.ToString()), Data))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("superficie di %s"), *Expected.Cell.ToString()),
			static_cast<int32>(Data->Surface), static_cast<int32>(Expected.Surface));
	}

	// --- I costi vengono dal CATALOGO, non da numeri scritti a mano nella fixture -------------------
	// Se la fixture incidesse i costi, cambiare il catalogo terreni lascerebbe la showcase su valori morti.
	for (const FRTCellId& Id : Arena->CellsInLayer(0))
	{
		const FRTHexCellData* Data = Arena->FindCell(Id);
		if (!Data) { continue; }
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Data->Surface);
		TestEqual(*FString::Printf(TEXT("costo di %s coerente col catalogo"), *Id.ToString()),
			Data->MoveCost, Def.MoveCost);
	}

	// --- Simmetria puntuale: nessun lato e' avvantaggiato ------------------------------------------
	// (q,r) e (-q,-r) devono avere la stessa superficie. E' il vincolo che rende leggibile un esito:
	// se una squadra vince, non e' perche' la meta' campo era piu' comoda.
	for (const FRTCellId& Id : Arena->CellsInLayer(0))
	{
		const FRTHexCellData* Data = Arena->FindCell(Id);
		const FRTHexCellData* Mirror = Arena->FindCell(FRTCellId(-Id.X, -Id.Y, Id.Layer));
		if (!TestNotNull(*FString::Printf(TEXT("la cella speculare di %s esiste"), *Id.ToString()), Mirror))
		{
			continue;
		}
		if (Data)
		{
			TestEqual(*FString::Printf(TEXT("superficie speculare di %s"), *Id.ToString()),
				static_cast<int32>(Data->Surface), static_cast<int32>(Mirror->Surface));
		}
	}

	// --- Spawn canonico ----------------------------------------------------------------------------
	const TArray<FRTShowcaseSpawn> Spawns = URTMatchSetupLibrary::GetShowcaseRelayLiteSpawns();
	TestEqual(TEXT("quattro unita' in campo (2v2)"), Spawns.Num(), 4);

	TSet<FRTCellId> Cells;
	TSet<FName> Heroes;
	int32 Team0 = 0;
	for (const FRTShowcaseSpawn& Spawn : Spawns)
	{
		Cells.Add(Spawn.Cell);
		Heroes.Add(Spawn.HeroId);
		Team0 += (Spawn.TeamId == 0) ? 1 : 0;

		const FRTHexCellData* Data = Arena->FindCell(Spawn.Cell);
		if (TestNotNull(*FString::Printf(TEXT("la cella di partenza %s e' nell'arena"), *Spawn.Cell.ToString()), Data))
		{
			TestFalse(*FString::Printf(TEXT("la cella di partenza %s e' percorribile"), *Spawn.Cell.ToString()),
				Data->bBlocksMovement);
			TestEqual(*FString::Printf(TEXT("si parte su terreno neutro in %s"), *Spawn.Cell.ToString()),
				static_cast<int32>(Data->Surface), static_cast<int32>(ERTHexSurface::Floor));
		}
	}
	TestEqual(TEXT("nessuna sovrapposizione di partenza"), Cells.Num(), 4);
	TestEqual(TEXT("quattro eroi distinti"), Heroes.Num(), 4);
	TestEqual(TEXT("due unita' per squadra"), Team0, 2);

	// Il roster canonico della v0.1, con gli ID del catalogo eroi: non gli archetipi legacy ne' i nomi
	// storici (Aegis/Nyx/Drift/Vex). Sono gli stessi ID che `URTHeroCatalogLibrary` usa come chiave stabile.
	for (const FName HeroId : { FName("Hero.Flux"), FName("Hero.Riva"), FName("Hero.Bastion"), FName("Hero.Vektor") })
	{
		TestTrue(*FString::Printf(TEXT("%s e' in campo"), *HeroId.ToString()), Heroes.Contains(HeroId));
	}

	// --- Stabilita' fra due generazioni ------------------------------------------------------------
	// Due arene generate dalla stessa funzione devono avere lo STESSO hash: e' la premessa del golden replay.
	URTHexMapAsset* Again = URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(GetTransientPackage());
	if (TestNotNull(TEXT("seconda generazione dell'arena"), Again))
	{
		TestEqual(TEXT("hash della mappa stabile fra due generazioni"),
			Again->ComputeHash(), Arena->ComputeHash());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseLiteDeterminismTest,
	"RefactorTactics.ShowcaseRelay.LiteScenarioIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseLiteDeterminismTest::RunTest(const FString&)
{
	// Quattro turni: abbastanza perche' le unita' si avvicinino, si colpiscano e il log abbia sostanza;
	// gli 8 turni scriptati sono CP 15.4, qui interessa che la fixture NON DERIVI.
	const int32 NumTurns = 4;
	const FRTShowcaseRun First = RunShowcaseLite(NumTurns);
	const FRTShowcaseRun Second = RunShowcaseLite(NumTurns);

	if (!TestTrue(TEXT("prima esecuzione dello scenario allestita"), First.bValid)
		|| !TestTrue(TEXT("seconda esecuzione dello scenario allestita"), Second.bValid))
	{
		return false;
	}

	// Guardia contro il confronto a vuoto: due partite mute sarebbero "identiche" senza dimostrare nulla.
	TestTrue(TEXT("lo scenario ha prodotto eventi di log"), First.LogLines.Num() > 0);
	TestEqual(TEXT("lo scenario ha giocato i turni richiesti"), First.TurnLogHashes.Num(), NumTurns);
	// Non si pretendono quattro unita': le sconfitte vengono DISTRUTTE dal TurnManager, e in quattro turni
	// lo scontro puo' gia' aver prodotto un KO. Il test verifica che le due partite finiscano UGUALI,
	// non che finiscano in un modo particolare — quello sara' il golden replay di CP 15.4.
	TestTrue(TEXT("lo scenario lascia unita' in campo"), First.FinalState.Num() > 0);

	// --- L'arena e' la stessa in entrambe le esecuzioni ---------------------------------------------
	// Prima del log: se la fixture deriva, il resto del confronto sta gia' misurando due partite diverse.
	TestEqual(TEXT("stessa arena nelle due esecuzioni"), Second.ArenaHash, First.ArenaHash);

	// --- Hash del TurnLog, turno per turno ---------------------------------------------------------
	if (TestEqual(TEXT("stesso numero di turni giocati"), Second.TurnLogHashes.Num(), First.TurnLogHashes.Num()))
	{
		for (int32 Turn = 0; Turn < First.TurnLogHashes.Num(); ++Turn)
		{
			TestEqual(*FString::Printf(TEXT("hash del TurnLog al turno %d"), Turn + 1),
				Second.TurnLogHashes[Turn], First.TurnLogHashes[Turn]);
		}
	}

	// --- Contenuto del log: l'hash dice CHE e' cambiato, la riga dice COSA ---------------------------
	if (TestEqual(TEXT("stesso numero di eventi di log"), Second.LogLines.Num(), First.LogLines.Num()))
	{
		for (int32 I = 0; I < First.LogLines.Num(); ++I)
		{
			TestEqual(*FString::Printf(TEXT("evento di log %d"), I), Second.LogLines[I], First.LogLines[I]);
		}
	}

	// --- Stato finale --------------------------------------------------------------------------------
	if (TestEqual(TEXT("stesso numero di unita' a fine partita"), Second.FinalState.Num(), First.FinalState.Num()))
	{
		for (int32 I = 0; I < First.FinalState.Num(); ++I)
		{
			TestEqual(TEXT("stato finale dell'unita'"), Second.FinalState[I], First.FinalState[I]);
		}
	}

	return true;
}
