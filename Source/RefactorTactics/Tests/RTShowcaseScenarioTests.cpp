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
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTCellId.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

	/** Unita' della showcase: eroe dal CATALOGO, l'unico percorso di configurazione. */
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

// =====================================================================================================
// Relay Basin — la mappa CANONICA della showcase (RT_Showcase_Relay_v01)
//
// Non sostituisce l'arena Lite: quella e' un esagono simmetrico di raggio 5 che serve al determinismo,
// questa e' la mappa degli 8 turni. Convivono perche' rispondono a due domande diverse — «l'esito e'
// riproducibile?» e «il gioco sa mostrare cio' che dice di essere?».
//
// Il layout e' AUTORATO (docs/product/showcase-v0.1.md §2): la spec di scenario dichiarata dall'handoff
// non esiste nel repository. Questo test e' cio' che impedisce alla mappa di derivare in silenzio.
// =====================================================================================================
namespace
{
	/** Le celle non-`Floor` del Relay Basin. Ogni cella compare UNA volta sola: e' la verifica di coerenza
	 *  richiesta dall'handoff §7, e qui e' una proprieta' del tipo, non un controllo da ricordare. */
	TArray<FRTShowcaseExpectedSurface> BasinExpectedSurfaces()
	{
		return {
			// Corridoio ovest: Flux ci passa al turno 1; `MistVeil` ne aggiunge al turno 5.
			{ FRTCellId(-3, 0, 0), ERTHexSurface::Smoke },
			{ FRTCellId(-2, 0, 0), ERTHexSurface::Smoke },
			// Lane d'acqua di Riva: conduttiva, ed e' cio' che rende possibile il payoff del turno 7.
			{ FRTCellId(-3, 1, 0), ERTHexSurface::ShallowWater },
			{ FRTCellId(-2, 1, 0), ERTHexSurface::ShallowWater },
			{ FRTCellId(-1, 1, 0), ERTHexSurface::ShallowWater },
			{ FRTCellId( 0, 1, 0), ERTHexSurface::ShallowWater },
			// Prosegue verso est: il `ConductiveNode` del turno 2 collega questa tratta all'acqua.
			{ FRTCellId( 1, 1, 0), ERTHexSurface::Conductive },
			{ FRTCellId( 2, 1, 0), ERTHexSurface::Conductive },
			// Sbarra la via diretta est->Relay ai movimenti lineari: invalida il `Ram` del turno 7.
			{ FRTCellId( 1, 0, 0), ERTHexSurface::Rough },
			{ FRTCellId( 2, 0, 0), ERTHexSurface::Rough },
			// Fascia nord: Vektor la attraversa al turno 3 scendendo dalla cresta. Lontana dagli spawn.
			{ FRTCellId( 2, -1, 0), ERTHexSurface::Fire },
			{ FRTCellId( 1, -1, 0), ERTHexSurface::Fire },
			// Cresta nord-est: vantaggio GEOMETRICO, nessun bonus numerico (D-024).
			{ FRTCellId( 2, -2, 0), ERTHexSurface::HighGround },
			{ FRTCellId( 3, -1, 0), ERTHexSurface::HighGround },
			// Ripiano sud: scivolata deterministica al turno 7.
			{ FRTCellId(-1, 2, 0), ERTHexSurface::Ice },
			{ FRTCellId( 0, 2, 0), ERTHexSurface::Ice },
			{ FRTCellId( 1, 2, 0), ERTHexSurface::Ice },
		};
	}

	/** Estensione di `q` per ogni riga `r`, dall'handoff §7. Somma = 45. */
	struct FRTBasinRow { int32 R; int32 MinQ; int32 MaxQ; };
	TArray<FRTBasinRow> BasinRows()
	{
		return { {-3,-1,1}, {-2,-2,2}, {-1,-3,3}, {0,-4,4}, {1,-4,4}, {2,-3,3}, {3,-2,2} };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseBasinLayoutTest,
	"RefactorTactics.ShowcaseRelay.BasinLayoutMatchesSpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseBasinLayoutTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeShowcaseRelayBasinArena(GetTransientPackage());
	if (!TestNotNull(TEXT("l'arena Relay Basin esiste"), Arena))
	{
		return false;
	}

	// --- Forma: 45 celle su un solo layer, riga per riga -------------------------------------------
	TestEqual(TEXT("numero di celle"), Arena->NumCells(), 45);
	TestEqual(TEXT("un solo layer"), Arena->GetLayers().Num(), 1);

	int32 Expected = 0;
	for (const FRTBasinRow& Row : BasinRows())
	{
		for (int32 Q = Row.MinQ; Q <= Row.MaxQ; ++Q)
		{
			++Expected;
			TestTrue(*FString::Printf(TEXT("la cella (%d,%d) appartiene alla mappa"), Q, Row.R),
				Arena->ContainsCell(FRTCellId(Q, Row.R, 0)));
		}
	}
	TestEqual(TEXT("la forma dichiarata somma a 45"), Expected, 45);

	// --- Superfici dichiarate ----------------------------------------------------------------------
	for (const FRTShowcaseExpectedSurface& Want : BasinExpectedSurfaces())
	{
		const FRTHexCellData* Data = Arena->FindCell(Want.Cell);
		if (!TestNotNull(*FString::Printf(TEXT("la cella %s esiste"), *Want.Cell.ToString()), Data))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("superficie di %s"), *Want.Cell.ToString()),
			static_cast<int32>(Data->Surface), static_cast<int32>(Want.Surface));
	}

	// --- I costi vengono dal CATALOGO ---------------------------------------------------------------
	// Se la fixture incidesse i propri numeri, cambiare il catalogo terreni lascerebbe la showcase su
	// valori morti — e nessun test se ne accorgerebbe.
	for (const FRTCellId& Id : Arena->CellsInLayer(0))
	{
		const FRTHexCellData* Data = Arena->FindCell(Id);
		if (!Data) { continue; }
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Data->Surface);
		TestEqual(*FString::Printf(TEXT("costo di %s coerente col catalogo"), *Id.ToString()),
			Data->MoveCost, Def.MoveCost);
	}

	// --- Il Relay e' pavimento libero ---------------------------------------------------------------
	// L'obiettivo non deve stare su un terreno che lo difende o lo penalizza da solo: chi lo tiene deve
	// averlo tenuto, non esserci capitato sopra.
	const FRTHexCellData* Relay = Arena->FindCell(FRTCellId(0, 0, 0));
	if (TestNotNull(TEXT("la cella del Relay esiste"), Relay))
	{
		TestEqual(TEXT("il Relay e' su Floor"), static_cast<int32>(Relay->Surface),
			static_cast<int32>(ERTHexSurface::Floor));
		TestFalse(TEXT("il Relay non blocca il movimento"), Relay->bBlocksMovement);
	}

	// --- Gate: una PORTA chiusa, non un meccanismo nuovo (CP 9.3) -----------------------------------
	TestEqual(TEXT("il gate e' chiuso all'inizio"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Arena, FRTCellId(0, 1, 0), FRTCellId(1, 1, 0))),
		static_cast<int32>(ERTHexDoorState::Closed));
	TestTrue(TEXT("il gate chiuso nega il passo"),
		URTHexDoorLibrary::BlocksBetween(Arena, FRTCellId(0, 1, 0), FRTCellId(1, 1, 0)));
	// Nei DUE versi: una porta e' un bordo, non una direzione.
	TestTrue(TEXT("il gate nega il passo anche in senso inverso"),
		URTHexDoorLibrary::BlocksBetween(Arena, FRTCellId(1, 1, 0), FRTCellId(0, 1, 0)));

	// --- Copertura bassa sull'approccio nord al Relay (CP 9.1) --------------------------------------
	TestEqual(TEXT("copertura bassa fra (0,-1) e il Relay"),
		static_cast<int32>(URTHexCoverLibrary::CoverBetween(Arena, FRTCellId(0, 0, 0), FRTCellId(0, -1, 0))),
		static_cast<int32>(ERTHexCoverType::Low));

	// --- Spawn canonici ------------------------------------------------------------------------------
	const TArray<FRTShowcaseSpawn> Spawns = URTMatchSetupLibrary::GetShowcaseRelayBasinSpawns();
	TestEqual(TEXT("quattro unita' in campo (2v2)"), Spawns.Num(), 4);

	TMap<FName, FRTCellId> ById;
	for (const FRTShowcaseSpawn& Spawn : Spawns)
	{
		ById.Add(Spawn.HeroId, Spawn.Cell);
		TestTrue(*FString::Printf(TEXT("lo spawn di %s e' dentro la mappa"), *Spawn.HeroId.ToString()),
			Arena->ContainsCell(Spawn.Cell));
		const FRTHexCellData* Data = Arena->FindCell(Spawn.Cell);
		if (Data)
		{
			TestEqual(*FString::Printf(TEXT("%s parte su Floor"), *Spawn.HeroId.ToString()),
				static_cast<int32>(Data->Surface), static_cast<int32>(ERTHexSurface::Floor));
		}
	}

	TestEqual(TEXT("Flux allo spawn dichiarato"),    ById.FindRef(TEXT("Hero.Flux")),    FRTCellId(-4, 0, 0));
	TestEqual(TEXT("Riva allo spawn dichiarato"),    ById.FindRef(TEXT("Hero.Riva")),    FRTCellId(-4, 1, 0));
	TestEqual(TEXT("Bastion allo spawn dichiarato"), ById.FindRef(TEXT("Hero.Bastion")), FRTCellId( 4, 0, 0));
	TestEqual(TEXT("Vektor allo spawn dichiarato"),  ById.FindRef(TEXT("Hero.Vektor")),  FRTCellId( 4, 1, 0));

	return true;
}

// =====================================================================================================
// S2-1 — lo scenario RIFERISCE la geometria, non la duplica
//
// Il punto non e' risparmiare righe di JSON: e' che la mappa canonica viva in UN posto solo, gia'
// protetto da BasinLayoutMatchesSpec. Quarantacinque celle copiate dentro uno scenario sarebbero una
// seconda verita' che nessun test confronta con la prima.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioFixtureFactoryTest,
	"RefactorTactics.Scenario.FixtureFactoryResolvesKnownNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioFixtureFactoryTest::RunTest(const FString&)
{
	UObject* Outer = GetTransientPackage();

	const URTHexMapAsset* Basin = URTMatchSetupLibrary::MakeFixtureArena(Outer, TEXT("RelayBasin"));
	if (TestNotNull(TEXT("la fixture RelayBasin si risolve"), Basin))
	{
		TestEqual(TEXT("RelayBasin ha le 45 celle della mappa canonica"), Basin->NumCells(), 45);
	}

	const URTHexMapAsset* Lite = URTMatchSetupLibrary::MakeFixtureArena(Outer, TEXT("RelayLite"));
	if (TestNotNull(TEXT("la fixture RelayLite si risolve"), Lite))
	{
		TestEqual(TEXT("RelayLite resta l'esagono di raggio 5"), Lite->NumCells(), 91);
	}

	const URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeFixtureArena(Outer, TEXT("CoverYard"));
	if (TestNotNull(TEXT("la fixture CoverYard si risolve"), Yard))
	{
		TestEqual(TEXT("CoverYard e' l'esagono di raggio 3"), Yard->NumCells(), 37);
	}

	// Un nome sconosciuto non produce un'arena vuota su cui la partita girerebbe comunque dando un FAIL
	// incomprensibile: produce NIENTE, e il chiamante decide.
	TestNull(TEXT("una fixture sconosciuta non si inventa"),
		URTMatchSetupLibrary::MakeFixtureArena(Outer, TEXT("NonEsiste")));
	TestNull(TEXT("una fixture senza nome non si inventa"),
		URTMatchSetupLibrary::MakeFixtureArena(Outer, FString()));

	return true;
}

/**
 * `CoverYard` porta i DUE tipi di copertura, e la differenza si legge dai dati prima ancora che da una
 * partita: la barriera **alta** nega l'attraversamento, la **bassa** lo lascia passare riducendo il danno.
 *
 * Il test guarda `CoverBetween`, non i campi della cella: e' la stessa funzione che interrogano vista, grafo
 * e combat (CP 9.2 §3), quindi verificarla qui vuol dire verificare cio' che il gioco chiedera' davvero. E
 * la chiede nei **due versi**: una barriera disegnata su una faccia sola che bloccasse in una direzione e non
 * nell'altra passerebbe un controllo sui campi e fallirebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverYardLayoutTest,
	"RefactorTactics.Scenario.CoverYardHasBothCoverTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverYardLayoutTest::RunTest(const FString&)
{
	const URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeCoverYardArena(GetTransientPackage());
	if (!TestNotNull(TEXT("il Cover Yard esiste"), Yard))
	{
		return false;
	}

	const FRTCellId HighA(0, 0, 0), HighB(1, 0, 0);
	const FRTCellId LowA(0, 1, 0),  LowB(1, 1, 0);

	TestEqual(TEXT("(0,0)->(1,0) e' una barriera ALTA"),
		URTHexCoverLibrary::CoverBetween(Yard, HighA, HighB), ERTHexCoverType::High);
	TestEqual(TEXT("e lo e' anche nel verso opposto"),
		URTHexCoverLibrary::CoverBetween(Yard, HighB, HighA), ERTHexCoverType::High);

	TestEqual(TEXT("(0,1)->(1,1) e' una copertura BASSA"),
		URTHexCoverLibrary::CoverBetween(Yard, LowA, LowB), ERTHexCoverType::Low);

	// Il resto del cortile e' sgombro: se un bordo qualunque risultasse coperto, ogni scenario scritto qui
	// misurerebbe una barriera che nessuno ha disegnato.
	TestEqual(TEXT("un bordo qualunque non ha coperture"),
		URTHexCoverLibrary::CoverBetween(Yard, FRTCellId(-1, 0, 0), FRTCellId(-2, 0, 0)),
		ERTHexCoverType::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioUnknownFixtureTest,
	"RefactorTactics.Scenario.UnknownFixtureIsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioUnknownFixtureTest::RunTest(const FString&)
{
	UWorld* World = MakeShowcaseWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Test.UnknownFixture");
	Scenario.Fixture = TEXT("NonEsiste");
	Scenario.Units.Add([]{ FRTScenarioUnit U; U.Id = TEXT("A1"); U.HeroId = TEXT("Hero.Flux");
		U.TeamId = 0; U.Cell = FRTCellId(0, 0, 0); return U; }());
	// Uno scenario senza assertion viene rifiutato in validazione — giustamente: passerebbe sempre. Qui ne
	// serve una qualunque, perche' cio' che si verifica e' che si arrivi al controllo della fixture.
	Scenario.Expect.Add([]{ FRTTestExpectation E; E.Kind = ERTAssertionKind::TurnsCompleted; E.Value = 0; return E; }());

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyShowcaseWorld(World);

	// ERROR, non FAIL: il gioco non ha sbagliato niente, lo scenario si'. Confonderli fa cercare nel
	// resolver un difetto che sta nel JSON.
	TestEqual(TEXT("fixture sconosciuta -> ERROR"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Error));
	TestTrue(TEXT("il messaggio nomina la fixture"), Result.ErrorMessage.Contains(TEXT("NonEsiste")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBlockedTurnTest,
	"RefactorTactics.Scenario.BlockedTurnIsNotAFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBlockedTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeShowcaseWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Test.BlockedTurn");
	Scenario.Fixture = TEXT("RelayBasin");
	for (const FRTShowcaseSpawn& Spawn : URTMatchSetupLibrary::GetShowcaseRelayBasinSpawns())
	{
		FRTScenarioUnit U;
		U.Id = Spawn.HeroId.ToString();
		U.HeroId = Spawn.HeroId;
		U.TeamId = Spawn.TeamId;
		U.Cell = Spawn.Cell;
		Scenario.Units.Add(U);
	}

	// Turno 1: eseguibile. Turno 2: chiede una capability che non esiste.
	//
	// Era `PredictiveAction` fino al 2026-08-10, quando E18 l'ha resa disponibile e questo test ha iniziato a
	// dire PASS — cioe' aveva smesso di verificare il BLOCKED. E' il rischio di ogni test che usi una
	// capability mancante come esempio: prima o poi quella capability arriva. `Perception` e' la scelta con
	// l'orizzonte piu' lungo (E13, `#151`, non iniziata), ma non e' eterna: quando atterrera', questa riga
	// andra' spostata di nuovo, non cancellata.
	Scenario.Turns.Add(FRTScenarioTurn());
	FRTScenarioTurn Blocked;
	Blocked.Requires.Add(TEXT("Perception"));
	Scenario.Turns.Add(Blocked);

	// L'assertion e' sul turno GIOCATO, non su quelli bloccati: e' cio' che rende l'esito un BLOCKED con
	// zero fallimenti invece di un FAIL mascherato.
	Scenario.Expect.Add([]{ FRTTestExpectation E; E.Kind = ERTAssertionKind::TurnsCompleted; E.Value = 1; return E; }());

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyShowcaseWorld(World);

	// Il valore di questo esito e' tutto qui: uno showcase parzialmente eseguibile deve dire QUANTO
	// arriva e COSA lo ferma, non fallire in blocco come se il gioco fosse rotto.
	TestEqual(TEXT("capability mancante -> BLOCKED"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Blocked));
	TestEqual(TEXT("il turno 1 e' stato giocato davvero"), Result.TurnsPlayed, 1);
	TestTrue(TEXT("il motivo nomina la capability mancante"),
		Result.BlockedReason.Contains(TEXT("Perception")));
	TestEqual(TEXT("BLOCKED si legge come tale nel report"), Result.OutcomeString(), FString(TEXT("BLOCKED")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioShowcaseRelayV01Test,
	"RefactorTactics.Scenario.ShowcaseRelayV01RunsTurnOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioShowcaseRelayV01Test::RunTest(const FString&)
{
	// Lo scenario VERSIONATO, non uno costruito nel test: e' cio' che rende questo un test dello showcase
	// e non dell'harness.
	FRTTestScenario Scenario;
	FString LoadError;
	// Il percorso si chiede all'INDICE: da `c53efc3` l'ID non e' piu' il percorso, e le cartelle sono libere.
	const FString ScenarioPath = URTScenarioIndex::ResolvePath(TEXT("RT_Showcase_Relay_v01"), LoadError);
	if (!TestTrue(TEXT("RT_Showcase_Relay_v01 si carica"),
		!ScenarioPath.IsEmpty() && URTScenarioLoader::LoadFromFile(ScenarioPath, Scenario, LoadError)))
	{
		AddError(FString::Printf(TEXT("caricamento fallito: %s"), *LoadError));
		return false;
	}

	TestEqual(TEXT("riferisce la fixture invece di duplicarla"), Scenario.Fixture, FString(TEXT("RelayBasin")));
	TestEqual(TEXT("la geometria non e' copiata nel JSON"), Scenario.Cells.Num(), 0);
	TestEqual(TEXT("otto turni dichiarati"), Scenario.Turns.Num(), 8);

	UWorld* World = MakeShowcaseWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// `RunById` e non `Run`: e' il percorso che scrive davvero `result.json`. Un test che salta il report
	// verificherebbe il resolver e lascerebbe scoperto proprio cio' che serve a diagnosticare da fuori.
	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("RT_Showcase_Relay_v01"), ReportDir);
	DestroyShowcaseWorld(World);

	// ⚠️ **Il nome del test e' storico, il numero no.** Nasceva quando lo showcase arrivava a UN turno: il
	// T2 chiedeva la Predictive Action e li' si fermava. Con E18 CP 18.2 (2026-08-10) quella capability
	// esiste, il T2 gira, il T3 non chiedeva nulla — e lo showcase arriva a **tre**. L'ID resta invariato
	// perche' e' l'exit gate della tranche S2-1 nella roadmap, e perche' la DOMANDA non e' cambiata: quanto
	// lontano arriva oggi, e cosa lo ferma. E' la risposta che si muove, ed e' giusto che si muova — ogni
	// capability che atterra ne accende un altro pezzo. Il numero e' pinnato apposta: se scendesse, sarebbe
	// una regressione da vedere subito.
	TestEqual(TEXT("si ferma sul primo turno non supportato"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Blocked));
	TestEqual(TEXT("arriva a tre turni: T1 e T3 non chiedono nulla, T2 lo apre E18"), Result.TurnsPlayed, 3);
	// Ora lo ferma il T4, che chiede la finestra di reazione: `DecisionBoundary` e' E14, non iniziata.
	TestTrue(TEXT("dichiara cosa lo blocca"), Result.BlockedReason.Contains(TEXT("DecisionBoundary")));

	// Le assertion del turno 1 sono state valutate e sono passate: un BLOCKED non le nasconde.
	TestTrue(TEXT("il turno 1 ha assertion valutate"), Result.Assertions.Num() > 0);
	TestEqual(TEXT("nessuna assertion del turno 1 fallita"), Result.FailedCount(), 0);

	// Lo stato finale e' un dato, non un caso: senza hash il golden replay non esiste.
	TestNotEqual(TEXT("lo StateHash e' stato calcolato"), Result.StateHash, static_cast<uint32>(0));

	// --- Il report: diagnosticabile da fuori, senza aprire i log di Unreal --------------------------
	if (TestFalse(TEXT("il report ha una cartella"), ReportDir.IsEmpty()))
	{
		const FString ReportPath = FPaths::Combine(ReportDir, TEXT("result.json"));
		FString Json;
		if (TestTrue(TEXT("result.json e' stato scritto"), FFileHelper::LoadFileToString(Json, *ReportPath)))
		{
			TestTrue(TEXT("il report dichiara l'esito BLOCKED"), Json.Contains(TEXT("BLOCKED")));
			// Il motivo sta accanto all'esito: «BLOCKED» da solo direbbe che non tutto e' pronto, che si
			// sapeva gia'. Il valore e' nel nome della capability.
			TestTrue(TEXT("il report nomina la capability mancante"), Json.Contains(TEXT("DecisionBoundary")));
			TestTrue(TEXT("il report distingue blockedReason da error"), Json.Contains(TEXT("blockedReason")));
		}
	}

	return true;
}

// =====================================================================================================
// S2-3 — il turno 1 della showcase e' DETERMINISTICO.
//
// Che passi lo verifica gia' `ShowcaseRelayV01RunsTurnOne`. Questo verifica una cosa diversa e piu' fragile:
// che passi SEMPRE ALLO STESSO MODO. Quattro unita' che si muovono nello stesso turno sono esattamente il
// caso in cui l'ordine dell'array puo' decidere l'esito senza che nessuno se ne accorga — e una showcase che
// cambia da una esecuzione all'altra non e' una dimostrazione, e' un aneddoto.
//
// Il gate generale (`Replay.Verifier.ResimulationIsDeterministic`, 100 ripetizioni) gira su
// `Movement.Collision`: due unita' su una mappa piccola. Questo gira sulla geometria canonica con il roster
// intero, che e' dove le interazioni non previste hanno spazio per manifestarsi.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseT1DeterministicTest,
	"RefactorTactics.Scenario.ShowcaseT1IsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseT1DeterministicTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString LoadError;
	const FString ScenarioPath = URTScenarioIndex::ResolvePath(TEXT("RT_Showcase_Relay_v01"), LoadError);
	if (!TestTrue(TEXT("lo showcase si trova nell'indice"), !ScenarioPath.IsEmpty())
		|| !TestTrue(TEXT("e si carica"), URTScenarioLoader::LoadFromFile(ScenarioPath, Scenario, LoadError)))
	{
		AddError(FString::Printf(TEXT("caricamento fallito: %s"), *LoadError));
		return false;
	}

	// Ogni ripetizione in un mondo NUOVO: riusarlo lascerebbe in giro gli actor della precedente, e due
	// esecuzioni identiche lo sarebbero per il motivo sbagliato — o divergerebbero per uno stato residuo che
	// non c'entra col determinismo del gioco.
	auto RunOnce = [this, &Scenario]() -> FRTTestResult
	{
		UWorld* World = MakeShowcaseWorld();
		if (!World) { return FRTTestResult(); }
		const FRTTestResult R = URTScenarioRunner::Run(World, Scenario);
		DestroyShowcaseWorld(World);
		return R;
	};

	const FRTTestResult First = RunOnce();
	if (First.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la prima esecuzione non e' partita: %s"), *First.ErrorMessage));
		return false;
	}
	// Un hash a zero significherebbe «nessuno stato»: confrontare zeri fra loro non proverebbe niente.
	TestNotEqual(TEXT("lo stato finale produce un hash reale"), First.StateHash, static_cast<uint32>(0));
	TestTrue(TEXT("almeno un turno e' stato giocato davvero"), First.TurnsPlayed >= 1);

	constexpr int32 Repetitions = 10;
	int32 Divergences = 0;
	for (int32 I = 1; I < Repetitions; ++I)
	{
		const FRTTestResult Again = RunOnce();
		// Non solo l'hash: anche ESITO e turni giocati. Uno scenario che si fermasse prima produrrebbe uno
		// stato diverso e quindi un hash diverso, ma uno che si ferma prima con lo stesso stato passerebbe
		// un confronto sul solo hash.
		if (Again.StateHash != First.StateHash
			|| Again.OutcomeString() != First.OutcomeString()
			|| Again.TurnsPlayed != First.TurnsPlayed)
		{
			++Divergences;
			AddError(FString::Printf(
				TEXT("ripetizione %d divergente: hash %08x vs %08x, esito %s vs %s, turni %d vs %d"),
				I, Again.StateHash, First.StateHash, *Again.OutcomeString(), *First.OutcomeString(),
				Again.TurnsPlayed, First.TurnsPlayed));
		}
	}
	TestEqual(FString::Printf(TEXT("%d ripetizioni identiche"), Repetitions), Divergences, 0);
	return true;
}

// =====================================================================================================
// CP 15.3 metà A (#169) — gli intenti di una partita sono un DATO, non un click.
//
// È la condizione perché il golden replay esista (CP 15.4) e perché la UI resti un consumer: se l'unico
// modo di far giocare un turno fosse cliccare, nessun test potrebbe descrivere una partita di otto turni,
// e la showcase sarebbe una cosa che una persona esegue a mano.
//
// ⚠️ **Metà A soltanto, e il confine è dichiarato.** Le DECISIONI di finestra (`Boundary → FIRE/HOLD`) e il
// `DecisionProvider` iniettabile sono la metà B e aspettano #163 (CP 14.3): `DecisionProvider` non esiste in
// `Source/`, e costruirlo prima che esistano finestre da cui iniettare darebbe un test verde e privo di
// contenuto — verde anche se E14 atterrasse male. Qui si verifica invece l'ultima voce di DoD che riguarda
// la metà A: **con lo script delle decisioni vuoto, lo scenario resta valido**.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseScriptedInputsTest,
	"RefactorTactics.ShowcaseRelay.ScriptedInputsDriveMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseScriptedInputsTest::RunTest(const FString&)
{
	// Lo scenario è costruito qui e non caricato da file: quello versionato dichiara otto turni e si ferma
	// sul primo non supportato, che è la sua domanda. Questa è un'altra — «gli intenti guidano davvero la
	// partita?» — e va posta su turni che il gioco di oggi sa giocare per intero, altrimenti la risposta
	// sarebbe BLOCKED e non direbbe nulla su ciò che si voleva sapere.
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.ScriptedInputsDriveMatch");
	Scenario.Fixture = TEXT("RelayBasin"); // la geometria canonica, riferita per nome e mai duplicata

	auto Unita = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& Cell)
	{
		FRTScenarioUnit U;
		U.Id = Id;
		U.HeroId = FName(Hero);
		U.TeamId = Team;
		U.Cell = Cell;
		return U;
	};
	Scenario.Units.Add(Unita(TEXT("Flux"),    TEXT("Hero.Flux"),    0, FRTCellId(-4, 0, 0)));
	Scenario.Units.Add(Unita(TEXT("Riva"),    TEXT("Hero.Riva"),    0, FRTCellId(-4, 1, 0)));
	Scenario.Units.Add(Unita(TEXT("Bastion"), TEXT("Hero.Bastion"), 1, FRTCellId( 4, 0, 0)));
	Scenario.Units.Add(Unita(TEXT("Vektor"),  TEXT("Hero.Vektor"),  1, FRTCellId( 4, 1, 0)));

	auto Movimento = [](const TCHAR* Id, const FRTCellId& Dove)
	{
		FRTScenarioIntent I;
		I.UnitId = Id;
		I.Move.Add(Dove);
		return I;
	};

	// T1 — SOLO MOVIMENTO, quattro unità nello stesso turno.
	{
		FRTScenarioTurn T;
		T.Intents.Add(Movimento(TEXT("Flux"),    FRTCellId(-3, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Riva"),    FRTCellId(-3, 1, 0)));
		T.Intents.Add(Movimento(TEXT("Bastion"), FRTCellId( 3, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Vektor"),  FRTCellId( 3, 1, 0)));
		Scenario.Turns.Add(T);
	}

	// T2 — AZIONE PRINCIPALE e movimento nello stesso turno, su unità diverse. Bastion erige un pannello
	// (slot principale) mentre gli altri continuano ad avvicinarsi (slot movimento): è la coesistenza dei due
	// slot che l'intent deve saper esprimere, e senza la quale «alimentare gli intenti» significherebbe solo
	// «muovere».
	{
		FRTScenarioTurn T;
		T.Requires.Add(TEXT("CreateCover"));

		FRTScenarioIntent Pannello;
		Pannello.UnitId = TEXT("Bastion");
		Pannello.Ability = FName(TEXT("Bastion.KineticPanel"));
		Pannello.TargetCell = FRTCellId(3, 0, 0);
		Pannello.bTargetsCell = true;
		Pannello.CoverEdge = ERTHexDirection::W; // verso chi arriva, che è l'unico verso che ha senso
		Pannello.bHasCoverEdge = true;
		T.Intents.Add(Pannello);

		T.Intents.Add(Movimento(TEXT("Flux"), FRTCellId(-2, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Riva"), FRTCellId(-2, 1, 0)));
		Scenario.Turns.Add(T);
	}

	// T3 — LO SCRIPT DELLE DECISIONI È VUOTO, e lo scenario resta valido. È l'ultima voce di DoD della metà A:
	// finché E14 non è atterrata non c'è nessuna finestra da alimentare, e un turno che non ne chiede resta un
	// turno legittimo. Un'unità sola si muove: gli altri restano fermi, che è un intento anche quello.
	{
		FRTScenarioTurn T;
		T.Intents.Add(Movimento(TEXT("Riva"), FRTCellId(-1, 1, 0)));
		Scenario.Turns.Add(T);
	}

	// Le assertion sono sulle POSIZIONI DICHIARATE: se il resolver ignorasse gli intenti e lasciasse decidere
	// il bot, le unità finirebbero altrove e questo test cadrebbe. È il punto — non «la partita gira», ma
	// «la partita fa quello che lo script dice».
	auto Dove = [](const TCHAR* Id, const FRTCellId& Cell)
	{
		FRTTestExpectation E;
		E.Kind = ERTAssertionKind::UnitAtCell;
		E.UnitId = Id;
		E.Cell = Cell;
		return E;
	};
	Scenario.Expect.Add(Dove(TEXT("Flux"),    FRTCellId(-2, 0, 0)));
	Scenario.Expect.Add(Dove(TEXT("Riva"),    FRTCellId(-1, 1, 0)));
	Scenario.Expect.Add(Dove(TEXT("Bastion"), FRTCellId( 3, 0, 0))); // ha eretto, non si è mosso
	Scenario.Expect.Add(Dove(TEXT("Vektor"),  FRTCellId( 3, 1, 0)));
	Scenario.Expect.Add([]{ FRTTestExpectation E; E.Kind = ERTAssertionKind::TurnsCompleted; E.Value = 3; return E; }());

	UWorld* World = MakeShowcaseWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyShowcaseWorld(World);

	if (!TestEqual(TEXT("i tre turni girano senza UI"),
			static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito %s — %s%s"), *Result.OutcomeString(),
			*Result.ErrorMessage, *Result.BlockedReason));
		return false;
	}

	TestEqual(TEXT("tre turni giocati"), Result.TurnsPlayed, 3);
	TestEqual(TEXT("nessuna assertion fallita"), Result.FailedCount(), 0);

	// Le tracce esistono per ogni turno: è la prova che i turni sono passati dal resolver reale e non da una
	// scorciatoia dell'harness — un TurnLog non si produce spostando un Actor.
	TestEqual(TEXT("una traccia per turno"), Result.TurnTraces.Num(), 3);

	// L'AZIONE PRINCIPALE dichiarata come dato è finita nel TurnLog del suo turno. Verificare solo le
	// posizioni lascerebbe scoperto proprio lo slot che il movimento non copre: uno scenario che alimentasse
	// i soli movimenti passerebbe tutte le assertion di cella e non direbbe niente sull'altro slot.
	if (Result.TurnTraces.IsValidIndex(1))
	{
		TArray<FRTTurnLogEntry> Voci;
		if (TestTrue(TEXT("la traccia del turno 2 si rilegge"),
				URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[1].Bytes, Voci)))
		{
			const bool bPannelloNelLog = Voci.ContainsByPredicate([](const FRTTurnLogEntry& E)
				{ return E.ActionId == FName(TEXT("Bastion.KineticPanel")); });
			TestTrue(TEXT("l'azione principale dichiarata compare nel TurnLog"), bPannelloNelLog);
		}
	}

	return true;
}
