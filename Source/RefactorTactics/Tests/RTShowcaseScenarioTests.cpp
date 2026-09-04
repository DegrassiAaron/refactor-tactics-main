#include "Misc/AutomationTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTReactionOpportunityTypes.h" // HoldResponse: il decisore che il test binda per primo
#include "Turn/RTReactionLibrary.h" // ERTReactionOutcome: «reazione» fra gli otto eventi chiave (#170)
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"          // HexDistance: l'adiacenza si chiede, non si deduce dalle coordinate
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
#include "ScenarioHarness/RTTestReportWriter.h" // l'ultimo anello: il campo deve arrivare in `result.json`
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"  // FindFiles: la cardinalita' del corpus golden si conta sul disco (#170)
#include "Algo/Reverse.h"     // la permutazione dell'ordine di spawn, turno per turno (#170)
#include "Templates/Function.h" // TFunction: gli otto eventi chiave sono PREDICATI, non categorie (#170)
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

// La guardia: senza, i test di questo file finiscono compilati DENTRO il binario Shipping che si
// distribuisce. Non e' una formalita' di build — e' cio' che tiene il codice di test fuori dal gioco.
// Aggiunta con #923, dopo che lo stesso difetto ha rotto la build Shipping due volte (2026-08-09 e
// 2026-08-15) senza che la suite, che gira sul target Editor, se ne accorgesse.
#if WITH_DEV_AUTOMATION_TESTS

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

	// Il mondo di prova vive in `RTWorldFixtures.h`: era dichiarato qui con un nome tutto suo perche' la
	// unity build fonde le translation unit e due namespace anonimi con la stessa funzione collidono. Un
	// namespace NOMINATO non ha quel problema. Le chiamate restano qualificate: in unity build questo
	// namespace anonimo e' lo stesso di ogni altro file di test.

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

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return Run; }

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(World);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (!Arena || !MapActor) { RTWorldFixtures::DestroyWorld(World); return Run; }
		MapActor->MapAsset = Arena;
		Run.ArenaHash = Arena->ComputeHash();

		for (const FRTShowcaseSpawn& Spawn : URTMatchSetupLibrary::GetShowcaseRelayLiteSpawns())
		{
			if (SpawnShowcaseHero(World, Spawn) == nullptr) { RTWorldFixtures::DestroyWorld(World); return Run; }
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { RTWorldFixtures::DestroyWorld(World); return Run; }

		for (int32 Turn = 1; Turn <= NumTurns && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
		{
			RTWorldFixtures::PlayOneTurn(TM);

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
		RTWorldFixtures::DestroyWorld(World);
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
	for (const FName HeroId : { FName("Hero.Gadget"), FName("Hero.Phase"), FName("Hero.Riktor"), FName("Hero.Wraith") })
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
	 *  richiesta dall'handoff §7, e qui e' una proprieta' del tipo, non un controllo da ricordare.
	 *
	 *  ⚠️ **Nove di queste celle hanno un secondo committente, e non e' lo showcase**: le due `Smoke`, le
	 *  due `Conductive`, le due `HighGround` e le tre `Ice` sono le quattro superfici che `D-146`/`D-183`
	 *  chiedono di rendere distinguibili per FORMA (CP 47.3, #956), e questa e' la sola fixture che le
	 *  contiene tutte — le altre rendono `Floor` piu' `Rough` e nient'altro (#1267).
	 *
	 *  🔴 **L'invariante e' «almeno una cella per superficie», non «tutte e nove»**, e la differenza conta
	 *  per chi modifica: togliere una delle tre `Ice` lascia la fixture utilizzabile, togliere una delle due
	 *  **`HighGround`** la avvicina all'inutilizzabile — sono le uniche che `RelayLite` non ha, quindi sono
	 *  cio' che rende Basin l'unica candidata. Perderle tutte lascia la verifica della grammatica visiva
	 *  senza un caso da guardare, e **nessun test lo direbbe**: questo file protegge la mappa dello scenario
	 *  a otto turni, e si e' trovato a proteggere anche quella per caso. */
	TArray<FRTShowcaseExpectedSurface> BasinExpectedSurfaces()
	{
		return {
			// Corridoio ovest: Gadget ci passa al turno 1; `MistVeil` ne aggiunge al turno 5.
			{ FRTCellId(-3, 0, 0), ERTHexSurface::Smoke },
			{ FRTCellId(-2, 0, 0), ERTHexSurface::Smoke },
			// Lane d'acqua di Phase: conduttiva, ed e' cio' che rende possibile il payoff del turno 7.
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
			// Fascia nord: Wraith la attraversa al turno 3 scendendo dalla cresta. Lontana dagli spawn.
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

	// --- Il Relay e' pavimento libero, ed E' l'obiettivo --------------------------------------------
	// L'obiettivo non deve stare su un terreno che lo difende o lo penalizza da solo: chi lo tiene deve
	// averlo tenuto, non esserci capitato sopra.
	const FRTHexCellData* Relay = Arena->FindCell(FRTCellId(0, 0, 0));
	if (TestNotNull(TEXT("la cella del Relay esiste"), Relay))
	{
		TestEqual(TEXT("il Relay e' su Floor"), static_cast<int32>(Relay->Surface),
			static_cast<int32>(ERTHexSurface::Floor));
		TestFalse(TEXT("il Relay non blocca il movimento"), Relay->bBlocksMovement);
		// 🔴 **La riga che mancava, e la sua assenza non produceva nessun rosso** (`#170`). La spec owner
		// dichiara il Relay `Objective.Relay, contendibile` (§2.2) dal primo giorno; la fixture non lo
		// marcava, e nessun test glielo chiedeva. Il T8 restava `Blocked` su una capability, e chi avesse
		// scoperto la capability senza questa riga avrebbe ottenuto un turno che gira, non segna e passa —
		// perche' su una mappa senza obiettivi il Cleanup tace di proposito
		// (`Objectives.SilentWithoutObjectiveCell`). L'assenza era invisibile in entrambi i versi.
		TestTrue(TEXT("il Relay E' un obiettivo contendibile: e' cio' che rende giocabile il T8"),
			Relay->bIsObjective);
	}

	// ⚠️ **UNO SOLO, e la cardinalita' e' parte dell'oracolo.** `FirstObjectiveCell()` restituisce il minimo
	// in ordine stabile (`URTHexLibrary::StableLess`), non «quello giusto»: con un secondo obiettivo su una
	// cella che precede il Relay, il T8 segnerebbe altrove e la riga qui sopra resterebbe **verde lo stesso**,
	// perche' il Relay sarebbe comunque marcato. Piu' obiettivi simultanei sono CP 31.1, post-v0.1 — qui la
	// loro assenza si misura invece di darla per scontata.
	int32 Obiettivi = 0;
	for (const FRTHexCellData& Cell : Arena->Cells)
	{
		if (Cell.bIsObjective)
		{
			++Obiettivi;
		}
	}
	TestEqual(TEXT("la mappa dichiara ESATTAMENTE un obiettivo"), Obiettivi, 1);

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

	// --- Copertura del T6: quella che rende DISCRIMINANTE l'interposizione (#1060) -------------------
	// La copertura sta sulla VITTIMA (Wraith) e non sull'intercettore (Riktor), ed e' cio' che rende il T6
	// discriminante: quando Riktor si interpone, la geometria si rivaluta su di LUI, che non ha riparo.
	// Un resolver che conservasse la copertura del bersaglio ORIGINALE gli farebbe 12 danni invece di 22 —
	// 113 punti vita invece di 103 — ed e' l'errore che D-017 vieta.
	TestEqual(TEXT("copertura bassa davanti a Wraith, sul bordo da cui il colpo lo raggiunge"),
		static_cast<int32>(URTHexCoverLibrary::CoverBetween(Arena, FRTCellId(1, -1, 0), FRTCellId(0, -1, 0))),
		static_cast<int32>(ERTHexCoverType::Low));

	// ⚠️ **E l'altra meta' dell'oracolo**: il bersaglio ORIGINALE non deve averne una sul proprio bordo
	// d'ingresso, altrimenti i due comportamenti — quello giusto e quello sbagliato — darebbero lo stesso
	// danno e il T6 sarebbe verde in entrambi i casi.
	TestEqual(TEXT("RIKTOR non ne ha una sul proprio bordo d'ingresso: e' l'altra meta' dell'oracolo"),
		static_cast<int32>(URTHexCoverLibrary::CoverBetween(Arena, FRTCellId(2, 0, 0), FRTCellId(1, 0, 0))),
		static_cast<int32>(ERTHexCoverType::None));

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

	TestEqual(TEXT("Gadget allo spawn dichiarato"),    ById.FindRef(TEXT("Hero.Gadget")),    FRTCellId(-4, 0, 0));
	TestEqual(TEXT("Phase allo spawn dichiarato"),    ById.FindRef(TEXT("Hero.Phase")),    FRTCellId(-4, 1, 0));
	TestEqual(TEXT("Riktor allo spawn dichiarato"), ById.FindRef(TEXT("Hero.Riktor")), FRTCellId( 4, 0, 0));
	TestEqual(TEXT("Wraith allo spawn dichiarato"),  ById.FindRef(TEXT("Hero.Wraith")),  FRTCellId( 4, 1, 0));

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
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Test.UnknownFixture");
	Scenario.Fixture = TEXT("NonEsiste");
	Scenario.Units.Add([]{ FRTScenarioUnit U; U.Id = TEXT("A1"); U.HeroId = TEXT("Hero.Gadget");
		U.TeamId = 0; U.Cell = FRTCellId(0, 0, 0); return U; }());
	// Uno scenario senza assertion viene rifiutato in validazione — giustamente: passerebbe sempre. Qui ne
	// serve una qualunque, perche' cio' che si verifica e' che si arrivi al controllo della fixture.
	Scenario.Expect.Add([]{ FRTTestExpectation E; E.Kind = ERTAssertionKind::TurnsCompleted; E.Value = 0; return E; }());

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	RTWorldFixtures::DestroyWorld(World);

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
	UWorld* World = RTWorldFixtures::MakeWorld();
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
	RTWorldFixtures::DestroyWorld(World);

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

// ⏱️ **Rinominato da `ShowcaseRelayV01RunsTurnOne` il 2026-09-03 (`#170`).** Il nome vecchio diceva «turno
// uno» e il test ne pinnava **sette**: era nato quando lo showcase si fermava al T2 e ha attraversato quattro
// sblocchi senza cambiare — l'ID restava fermo perche' era l'exit gate di una tranche di roadmap, e perche'
// la DOMANDA («quanto lontano arriva, e cosa lo ferma») non cambiava mai.
//
// ⚠️ **E il nome nuovo non porta un numero, di proposito.** `RunsEightTurns` sarebbe invecchiato come
// `RunsTurnOne`: il numero vive nell'assertion, dove cambiarlo e' un diff che si legge, non nel nome, dove
// resta vero per definizione. Cio' che questo test dice ora e' una PROPRIETA' — li gioca **tutti** — e la
// proprieta' e' l'unica formulazione che un nono turno non renderebbe falsa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioShowcaseRelayV01Test,
	"RefactorTactics.Scenario.ShowcaseRelayV01PlaysEveryTurn",
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

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// `RunById` e non `Run`: e' il percorso che scrive davvero `result.json`. Un test che salta il report
	// verificherebbe il resolver e lascerebbe scoperto proprio cio' che serve a diagnosticare da fuori.
	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("RT_Showcase_Relay_v01"), ReportDir);
	RTWorldFixtures::DestroyWorld(World);

	// ⚠️ **Il nome del test e' storico, il numero no.** Nasceva quando lo showcase arrivava a UN turno: il
	// T2 chiedeva la Predictive Action e li' si fermava. Con E18 CP 18.2 (2026-08-10) quella capability
	// esiste, il T2 gira, il T3 non chiedeva nulla — e lo showcase arriva a **tre**. L'ID resta invariato
	// perche' e' l'exit gate della tranche S2-1 nella roadmap, e perche' la DOMANDA non e' cambiata: quanto
	// lontano arriva oggi, e cosa lo ferma. E' la risposta che si muove, ed e' giusto che si muova — ogni
	// capability che atterra ne accende un altro pezzo. Il numero e' pinnato apposta: se scendesse, sarebbe
	// una regressione da vedere subito.
	//
	// ⏱️ **2026-08-16, `#512` fase B: da tre a CINQUE.** Il T4 chiedeva `DecisionBoundary`, che ora e' fra le
	// disponibili; il T5 non chiede nulla. Il numero si e' mosso per la terza volta e per la terza ragione
	// giusta — non e' il test ad essere stato aggiustato, e' il gioco ad essere arrivato piu' lontano.
	// ⏱️ **2026-09-02, `#1060`: da cinque a SETTE.** Il T6 ha il proprio contenuto e
	// `InterceptRevalidation` e' fra le disponibili; il T7 gioca **con lui**, perche' non dichiara `requires`
	// e nessuno lo teneva fuori se non il turno prima.
	//
	// ⏱️ **2026-09-03, `#170`: da sette a OTTO, e l'esito passa da `BLOCKED` a `PASS`.** E' la quinta volta
	// che questo numero si muove e la prima in cui arriva in fondo — non c'e' piu' un turno che aspetta.
	// 🔴 **E cio' che l'ha sbloccato non e' la chiusura di `#75`**, che era avvenuta il giorno prima
	// lasciando il T8 esattamente com'era: era una riga nella FIXTURE. `MakeShowcaseRelayBasinArena` non
	// marcava `(0,0,0)` come obiettivo — nessun costruttore di arena ne marcava uno — e su una mappa senza
	// obiettivi il Cleanup tace di proposito. La capability aveva il suo produttore e niente su cui girare.
	TestEqual(TEXT("li gioca TUTTI: nessun turno resta dietro una capability"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass));
	TestEqual(TEXT("otto turni giocati, quanti il file ne dichiara"), Result.TurnsPlayed, 8);
	// 🔴 **La meta' negativa, e qui vale doppio.** `PASS` con otto turni non basta a dire che nessuno e'
	// stato saltato: `BlockedReason` VUOTO e' l'affermazione che nessuna capability e' stata invocata come
	// scusa. Senza questa riga, un futuro turno 9 bloccato darebbe ancora otto turni giocati e un verde qui.
	TestTrue(TEXT("e nessuna capability e' rimasta a bloccare qualcosa"), Result.BlockedReason.IsEmpty());

	// Le assertion sono state valutate e sono passate — tutte e ventidue, non solo quelle dei primi turni.
	TestTrue(TEXT("le assertion di scenario sono state valutate"), Result.Assertions.Num() > 0);
	TestEqual(TEXT("nessuna assertion fallita"), Result.FailedCount(), 0);

	// Lo stato finale e' un dato, non un caso: senza hash il golden replay non esiste.
	TestNotEqual(TEXT("lo StateHash e' stato calcolato"), Result.StateHash, static_cast<uint32>(0));

	// --- Il report: diagnosticabile da fuori, senza aprire i log di Unreal --------------------------
	if (TestFalse(TEXT("il report ha una cartella"), ReportDir.IsEmpty()))
	{
		const FString ReportPath = FPaths::Combine(ReportDir, TEXT("result.json"));
		FString Json;
		if (TestTrue(TEXT("result.json e' stato scritto"), FFileHelper::LoadFileToString(Json, *ReportPath)))
		{
			// ⏱️ **`PASS` e non piu' `BLOCKED` dal 2026-09-03 (`#170`)**: il report diceva quale capability
			// mancava, e ora non ne manca nessuna. ⚠️ **La riga che nominava `Objective` e' stata TOLTA e non
			// invertita**, ed e' una scelta: un `TestFalse(Contains("Objective"))` sarebbe andato rosso per la
			// ragione sbagliata — il report ora nomina `Objective` eccome, nelle assertion del T8 che
			// contano le voci del TurnLog. Cercare l'assenza di una parola in un documento che ha
			// legittimamente cominciato a usarla e' il modo piu' veloce di scrivere un test fragile.
			TestTrue(TEXT("il report dichiara l'esito PASS"), Json.Contains(TEXT("PASS")));
			// 🔴 **E non porta NESSUNO dei due campi diagnostici**, che e' l'affermazione simmetrica di
			// quella che stava qui prima. Finche' l'esito era `BLOCKED` questa riga verificava che
			// `blockedReason` esistesse **accanto** a `error` — chi legge deve poter distinguere «il test e'
			// rotto» da «la feature non c'e' ancora». Ora che nessuna delle due condizioni vale, il report
			// deve tacere su entrambe: `RTTestReportWriter` scrive quei campi **solo se non vuoti**, quindi
			// trovarli in un PASS significherebbe che un residuo e' sopravvissuto all'esecuzione.
			TestFalse(TEXT("un PASS non porta blockedReason"), Json.Contains(TEXT("blockedReason")));
			TestFalse(TEXT("ne' error"), Json.Contains(TEXT("\"error\"")));
			// ➕ **Il T4 ha SPARATO, e il report lo dice in due numeri.** Senza questi, «cinque turni» sarebbe
			// compatibile con un T4 che si sblocca e non apre nessuna finestra — cioe' esattamente il verde
			// che la fase B esiste per non produrre. `scriptedDecisionsUnused` e' la meta' negativa: una
			// decisione dichiarata e mai consumata significa finestra scoperta, e l'harness la conta.
			// ⚠️ **La virgola finale fa parte dell'asserzione, e non e' pedanteria**: senza, `": 2"` e' un
			// prefisso e il `Contains` accetterebbe anche `": 21"` — il conteggio sbagliato dentro un verde.
			// E il caso non e' ipotetico: con `": 1"` questa riga sarebbe rimasta verde nel passaggio da UNA
			// decisione a DUE, perche' un prefisso non distingue «1» da «1 seguito da altro».
			//
			// ⏱️ **Da 1 a 2 con `#1038`**: lo showcase ha riavuto la propria coreografia. Wraith sale sulla
			// cresta al T1, predice da lassu' al T2, scende attraverso il fuoco al T3 e arma al T4 da una
			// riga — `r = -1` — la cui linea verso ovest non ha porte. Gadget entra per primo (`HOLD`),
			// Phase dopo (`FIRE`): DUE opportunity distinte, che e' cio' che `showcase-v0.1.md` §«Turno 4»
			// chiede. Il turno precedente ne apriva una sola perche' il T1 parcheggiava Wraith sulla lane
			// d'acqua, dietro la porta chiusa — e nessuna linea di Overwatch usciva da li'.
			TestTrue(TEXT("il T4 ha applicato ENTRAMBE le decisioni scriptate"),
				Json.Contains(TEXT("\"scriptedDecisionsApplied\": 2,")));
			TestTrue(TEXT("e non ne ha lasciata nessuna inutilizzata"),
				Json.Contains(TEXT("\"scriptedDecisionsUnused\": 0,")));
		}
	}

	return true;
}

// =====================================================================================================
// S2-3 — il turno 1 della showcase e' DETERMINISTICO.
//
// Che passi lo verifica gia' `ShowcaseRelayV01PlaysEveryTurn`. Questo verifica una cosa diversa e piu' fragile:
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
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return FRTTestResult(); }
		const FRTTestResult R = URTScenarioRunner::Run(World, Scenario);
		RTWorldFixtures::DestroyWorld(World);
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
	Scenario.Units.Add(Unita(TEXT("Gadget"),    TEXT("Hero.Gadget"),    0, FRTCellId(-4, 0, 0)));
	Scenario.Units.Add(Unita(TEXT("Phase"),    TEXT("Hero.Phase"),    0, FRTCellId(-4, 1, 0)));
	Scenario.Units.Add(Unita(TEXT("Riktor"), TEXT("Hero.Riktor"), 1, FRTCellId( 4, 0, 0)));
	Scenario.Units.Add(Unita(TEXT("Wraith"),  TEXT("Hero.Wraith"),  1, FRTCellId( 4, 1, 0)));

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
		T.Intents.Add(Movimento(TEXT("Gadget"),    FRTCellId(-3, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Phase"),    FRTCellId(-3, 1, 0)));
		T.Intents.Add(Movimento(TEXT("Riktor"), FRTCellId( 3, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Wraith"),  FRTCellId( 3, 1, 0)));
		Scenario.Turns.Add(T);
	}

	// T2 — AZIONE PRINCIPALE e movimento nello stesso turno, su unità diverse. Riktor erige un pannello
	// (slot principale) mentre gli altri continuano ad avvicinarsi (slot movimento): è la coesistenza dei due
	// slot che l'intent deve saper esprimere, e senza la quale «alimentare gli intenti» significherebbe solo
	// «muovere».
	{
		FRTScenarioTurn T;
		T.Requires.Add(TEXT("CreateCover"));

		FRTScenarioIntent Pannello;
		Pannello.UnitId = TEXT("Riktor");
		Pannello.Ability = FName(TEXT("Hero.Riktor.KineticPanel"));
		Pannello.TargetCell = FRTCellId(3, 0, 0);
		Pannello.bTargetsCell = true;
		Pannello.CoverEdge = ERTHexDirection::W; // verso chi arriva, che è l'unico verso che ha senso
		Pannello.bHasCoverEdge = true;
		T.Intents.Add(Pannello);

		T.Intents.Add(Movimento(TEXT("Gadget"), FRTCellId(-2, 0, 0)));
		T.Intents.Add(Movimento(TEXT("Phase"), FRTCellId(-2, 1, 0)));
		Scenario.Turns.Add(T);
	}

	// T3 — LO SCRIPT DELLE DECISIONI È VUOTO, e lo scenario resta valido. È l'ultima voce di DoD della metà A:
	// finché E14 non è atterrata non c'è nessuna finestra da alimentare, e un turno che non ne chiede resta un
	// turno legittimo. Un'unità sola si muove: gli altri restano fermi, che è un intento anche quello.
	{
		FRTScenarioTurn T;
		T.Intents.Add(Movimento(TEXT("Phase"), FRTCellId(-1, 1, 0)));
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
	Scenario.Expect.Add(Dove(TEXT("Gadget"),    FRTCellId(-2, 0, 0)));
	Scenario.Expect.Add(Dove(TEXT("Phase"),    FRTCellId(-1, 1, 0)));
	Scenario.Expect.Add(Dove(TEXT("Riktor"), FRTCellId( 3, 0, 0))); // ha eretto, non si è mosso
	Scenario.Expect.Add(Dove(TEXT("Wraith"),  FRTCellId( 3, 1, 0)));
	Scenario.Expect.Add([]{ FRTTestExpectation E; E.Kind = ERTAssertionKind::TurnsCompleted; E.Value = 3; return E; }());

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	RTWorldFixtures::DestroyWorld(World);

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
				{ return E.ActionId == FName(TEXT("Hero.Riktor.KineticPanel")); });
			TestTrue(TEXT("l'azione principale dichiarata compare nel TurnLog"), bPannelloNelLog);
		}
	}

	return true;
}

/**
 * Il test che `#512` nomina: un decisore INIETTATO risponde a una finestra vera, e la risposta viene dallo
 * scenario invece che da una persona.
 *
 * ⚠️ Lo scenario e' costruito qui e non caricato da file: `Scenarios/` e' `integration_only`, e in fase A
 * nessun JSON del corpus si tocca.
 *
 * ⚠️ **Nessun `Requires`, ed e' una correzione al piano.** Il piano dichiarava `Requires("DecisionBoundary")`
 * spiegando che «il turno e' `Blocked` per costruzione» ma che il decisore sarebbe stato comunque
 * interrogato: le due cose non stanno insieme. `FRTScenarioSession::BeginTurn` chiama `Finish()` e ritorna
 * appena una capability non e' disponibile — PRIMA di applicare gli intent — quindi con quel `requires`
 * nessun Overwatch si arma, nessuna finestra si apre e il decisore non viene chiamato mai.
 * `DecisionBoundary` e' un'etichetta del vocabolario degli scenari, non un interruttore del motore: le
 * finestre le apre gia' il CP 14.5. Scoprirla resta fase B, come dicono i vincoli globali.
 *
 * Geometria misurata, non indovinata: `Wraith` ha vista 6 e `Wraith.PulseShot` portata 4, quindi come
 * guardiano copre il varco. Il cono E' il facing (ADR-0005 §4c): da `(2,0,0)` guardando a `W` parte da
 * `(1,0,0)` e arriva a `(-2,0,0)`. `Gadget` lo attraversa. Un solo bersaglio basta per aprire la finestra
 * perche' `HOLD` e' sempre in coda ad `AllowedResponses`, quindi la cardinalita' e' 2 e
 * `RequiresDecisionBoundary` e' vera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionProviderTest,
	"RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionProviderTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.DecisionProviderIsInjectable");
	Scenario.MapRadius = 5;

	auto Unita = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& Cell,
		ERTHexDirection Facing)
	{
		FRTScenarioUnit U;
		U.Id = Id; U.HeroId = FName(Hero); U.TeamId = Team; U.Cell = Cell; U.Facing = Facing;
		return U;
	};
	// Il guardiano guarda a Ovest; il bersaglio attraversa il cono da Ovest verso il centro.
	Scenario.Units.Add(Unita(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
	Scenario.Units.Add(Unita(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));

	{
		FRTScenarioTurn T;

		// `Action.Overwatch` risolve in Prep, quindi si arma su chi la usa e NON dichiara un bersaglio:
		// `bResolvesOnSelf` in `RTScenarioSession.cpp` lo tratta gia' cosi'.
		FRTScenarioIntent Arma;
		Arma.UnitId = TEXT("Guardia");
		Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);

		FRTScenarioIntent Attraversa;
		Attraversa.UnitId = TEXT("Corsa");
		Attraversa.Move.Add(FRTCellId(-2, 0, 0));
		Attraversa.Move.Add(FRTCellId(-1, 0, 0));
		T.Intents.Add(Attraversa);

		FRTScenarioDecision D;
		D.Unit = TEXT("Guardia"); D.Respond = TEXT("FIRE"); D.Target = TEXT("Corsa");
		T.Decisions.Add(D);

		Scenario.Turns.Add(T);
	}

	// ⚠️ Serve un'assertion anche COSTRUENDO lo scenario in memoria, e i vincoli globali del piano dicevano
	// il contrario — «la guardia e' solo nel loader, gli scenari costruiti in memoria non la attraversano».
	// Misurato: `RunScenarioIsolated` valida la struct, e senza questa riga il referto torna con
	// «nessuna assertion dichiarata» e ZERO turni giocati. Non e' una formalita': senza turni non c'e'
	// finestra, e il test fallirebbe indicando il provider invece della propria costruzione.
	{
		FRTTestExpectation E;
		E.Kind = ERTAssertionKind::TurnsCompleted;
		E.Value = 1;
		Scenario.Expect.Add(E);
	}

	// `RunScenarioIsolated` crea il mondo, esegue e lo distrugge: e' l'API che `RTWorldFixtures.h` espone
	// proprio per questo, e usarla evita la coppia crea/distruggi da tenere allineata a mano.
	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	if (!TestEqual(TEXT("il decisore ha risposto una volta"), Result.ScriptedDecisionsApplied, 1))
	{
		// Se resta 0 la finestra non si e' aperta, e la causa e' la geometria o l'armamento — non il
		// provider. Si stampa lo stato invece di rilassare l'asserzione.
		//
		// ⚠️ Il piano diceva `URTTurnLogLibrary::ToText(Result.TurnLog)`: `FRTTestResult` non ha un campo
		// `TurnLog`, ha `TurnTraces`, e quelle portano i soli byte serializzati.
		AddInfo(FString::Printf(
			TEXT("turni giocati=%d tracce=%d blocked='%s' error='%s' ultimo token='%s' inutilizzate=%d"),
			Result.TurnsPlayed, Result.TurnTraces.Num(), *Result.BlockedReason, *Result.ErrorMessage,
			*Result.LastScriptedResponse, Result.ScriptedDecisionsUnused));
		for (const FString& Nota : Result.Notes) { AddInfo(Nota); }
		for (const FRTTurnTrace& Traccia : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Voci;
			if (URTTurnLogLibrary::DeserializeTurnLog(Traccia.Bytes, Voci))
			{
				for (const FRTTurnLogEntry& V : Voci) { AddInfo(URTTurnLogLibrary::DescribeEntry(V)); }
			}
		}
	}
	TestEqual(TEXT("nessuna decisione e' rimasta inutilizzata"), Result.ScriptedDecisionsUnused, 0);
	// La TRADUZIONE e' l'unica parte che il JSON non poteva esprimere: si verifica sul token, non
	// sull'esito. Lo `StableUnitId` di `Corsa` e' assegnato allo spawn, e lo scenario non poteva scriverlo.
	TestTrue(FString::Printf(TEXT("il token applicato e' un FIRE, non un HOLD (era: '%s')"),
		*Result.LastScriptedResponse),
		Result.LastScriptedResponse.StartsWith(TEXT("FIRE:")));
	return true;
}

/**
 * Il residuo e' un FALLIMENTO, non un avanzo. Senza questo controllo uno scenario puo' scriptare due
 * decisioni, vederne applicare una, e restare verde: e' il modo in cui un test smette di verificare senza
 * dirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionResidueTest,
	"RefactorTactics.ShowcaseRelay.UnusedScriptedDecisionFailsTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionResidueTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.UnusedScriptedDecision");
	Scenario.MapRadius = 4;

	FRTScenarioUnit Sola;
	Sola.Id = TEXT("Sola"); Sola.HeroId = FName(TEXT("Hero.Riktor")); Sola.TeamId = 0;
	Sola.Cell = FRTCellId(0, 0, 0);
	Scenario.Units.Add(Sola);

	// Nessun intent, quindi nessuna finestra: la decisione non puo' trovare nulla da cui essere consumata.
	FRTScenarioTurn T;
	FRTScenarioDecision D;
	D.Unit = TEXT("Sola"); D.Respond = TEXT("HOLD");
	T.Decisions.Add(D);
	Scenario.Turns.Add(T);

	// Anche in memoria serve un'assertion: `RunScenarioIsolated` valida la struct. Vedi il task 5.
	FRTTestExpectation E;
	E.Kind = ERTAssertionKind::TurnsCompleted;
	E.Value = 1;
	Scenario.Expect.Add(E);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	TestEqual(TEXT("la decisione resta inutilizzata"), Result.ScriptedDecisionsUnused, 1);
	// `Error` e non `Fail`: e' lo stesso verso che la session usa gia' quando lo SCENARIO e' scritto male,
	// distinto da un'aspettativa di gioco caduta.
	TestEqual(TEXT("e lo scenario e' in errore"), static_cast<int32>(Result.Outcome),
		static_cast<int32>(ERTTestOutcome::Error));
	TestTrue(FString::Printf(TEXT("il messaggio nomina l'unita' (era: '%s')"), *Result.ErrorMessage),
		Result.ErrorMessage.Contains(TEXT("Sola")));
	return true;
}

/**
 * Due decisioni per la stessa unita' si consumano in ordine di dichiarazione, e una finestra in piu' delle
 * decisioni dichiarate non e' un timeout: e' una finestra scoperta.
 *
 * ⚠️ Il numero di decisioni non e' dedotto dai micro-step, e' **contato**: con lo stesso allestimento del
 * task 5 il TurnLog mostra DUE finestre quando la prima risposta non spara. Un `FIRE` invece TRONCA il
 * movimento, ed e' il motivo per cui `DecisionProviderIsInjectable` — che dichiara un solo `FIRE` — ne apre
 * una sola e non lascia residuo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionQueueTest,
	"RefactorTactics.ShowcaseRelay.ScriptedDecisionsAreConsumedInOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionQueueTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.ScriptedDecisionsInOrder");
	Scenario.MapRadius = 5;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
		return X;
	};
	// Stessa geometria del task 5, che e' quella misurata: nessun `Requires`, o il turno sarebbe `Blocked`
	// prima ancora di applicare gli intent.
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));

	FRTScenarioTurn T;
	FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
	T.Intents.Add(Arma);
	FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
	Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
	T.Intents.Add(Corre);

	// Se la coda fosse posizionale invece che per unita', l'ordine cambierebbe col movimento.
	FRTScenarioDecision Prima; Prima.Unit = TEXT("Guardia"); Prima.Respond = TEXT("HOLD");
	FRTScenarioDecision Seconda; Seconda.Unit = TEXT("Guardia"); Seconda.Respond = TEXT("FIRE");
	Seconda.Target = TEXT("Corsa");
	T.Decisions.Add(Prima);
	T.Decisions.Add(Seconda);
	Scenario.Turns.Add(T);

	FRTTestExpectation E;
	E.Kind = ERTAssertionKind::TurnsCompleted;
	E.Value = 1;
	Scenario.Expect.Add(E);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	if (!TestEqual(TEXT("entrambe consumate"), Result.ScriptedDecisionsApplied, 2))
	{
		AddInfo(FString::Printf(TEXT("inutilizzate=%d ultimo token='%s' error='%s'"),
			Result.ScriptedDecisionsUnused, *Result.LastScriptedResponse, *Result.ErrorMessage));
		for (const FRTTurnTrace& Traccia : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Voci;
			if (URTTurnLogLibrary::DeserializeTurnLog(Traccia.Bytes, Voci))
			{
				for (const FRTTurnLogEntry& V : Voci) { AddInfo(URTTurnLogLibrary::DescribeEntry(V)); }
			}
		}
	}
	TestEqual(TEXT("nessun residuo"), Result.ScriptedDecisionsUnused, 0);
	// La SECONDA e' il `FIRE`: se l'ordine fosse invertito l'ultimo token sarebbe `HOLD`.
	TestTrue(FString::Printf(TEXT("l'ultima applicata e' il FIRE (era: '%s')"), *Result.LastScriptedResponse),
		Result.LastScriptedResponse.StartsWith(TEXT("FIRE:")));
	return true;
}

/**
 * L'altra meta' del residuo, e senza di lei il controllo sulla finestra SCOPERTA non sarebbe coperto da
 * nulla: il piano lo prescrive al passo 4 del task 6, ma i suoi due test non lo toccano — il primo non apre
 * finestre, il secondo le consuma tutte. Un controllo che nessun test fa cadere e' codice che puo' sparire
 * senza che la suite se ne accorga.
 *
 * Il caso: DUE finestre e UNA sola decisione. La prima risponde `HOLD`, che non spende la carica e lascia
 * proseguire il movimento; la seconda si apre e non ha nessuna decisione che la nomini. Non e' un timeout,
 * e' uno scenario che dice meno di quanto il turno chieda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseUncoveredWindowTest,
	"RefactorTactics.ShowcaseRelay.UncoveredReactionWindowFailsTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseUncoveredWindowTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.UncoveredReactionWindow");
	Scenario.MapRadius = 5;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
		return X;
	};
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));

	FRTScenarioTurn T;
	FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
	T.Intents.Add(Arma);
	FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
	Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
	T.Intents.Add(Corre);

	// UNA sola decisione per due finestre: la seconda resta scoperta.
	FRTScenarioDecision Unica; Unica.Unit = TEXT("Guardia"); Unica.Respond = TEXT("HOLD");
	T.Decisions.Add(Unica);
	Scenario.Turns.Add(T);

	FRTTestExpectation E;
	E.Kind = ERTAssertionKind::TurnsCompleted;
	E.Value = 1;
	Scenario.Expect.Add(E);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	// La decisione dichiarata E' stata consumata: il difetto non e' un residuo, e i due controlli non vanno
	// confusi — dicono cose diverse e mandano a cercare in posti diversi.
	TestEqual(TEXT("l'unica decisione e' stata consumata"), Result.ScriptedDecisionsApplied, 1);
	TestEqual(TEXT("e non e' rimasto un residuo"), Result.ScriptedDecisionsUnused, 0);
	TestEqual(TEXT("ma lo scenario e' in errore"), static_cast<int32>(Result.Outcome),
		static_cast<int32>(ERTTestOutcome::Error));
	TestTrue(FString::Printf(TEXT("il motivo dice 'finestra' e nomina l'unita' (era: '%s')"),
		*Result.ErrorMessage),
		Result.ErrorMessage.Contains(TEXT("finestra")) && Result.ErrorMessage.Contains(TEXT("Guardia")));
	return true;
}

/**
 * Due sorgenti per la stessa decisione, e la precedenza e' del test. Il prezzo e' che uno scenario con
 * `decisions` verrebbe ignorato in silenzio: percio' la provenienza si SCRIVE. Al replay serve **quale**
 * decisione, non chi l'ha fornita — ma a chi diagnostica una divergenza serve la seconda.
 *
 * ⚠️ Qui NON si usa `RunScenarioIsolated`: il mondo serve **prima** dello scenario, perche' il manager deve
 * esistere e il decisore deve essere bindato prima che la session parta. E' il caso che dimostra la
 * precedenza, e con l'API isolata non sarebbe esprimibile.
 *
 * ⚠️ La geometria e' quella del task 5 e non uno scenario inerte: il piano contava le interrogazioni in una
 * variabile che poi non asseriva mai. Senza una finestra vera il test direbbe soltanto «la session non ha
 * bindato», che e' meta' della precedenza — l'altra meta' e' che a rispondere sia stato il decisore del
 * test, e quella si vede solo contando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionSourceTest,
	"RefactorTactics.ShowcaseRelay.TestDeciderWinsAndIsRecorded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionSourceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!World) { AddError(TEXT("mondo non creabile")); return false; }

	// Il manager esiste prima della session: e' cosi' che un test binda PRIMA e vince. `SetUp` riusa il
	// manager gia' presente invece di spawnarne un altro, quindi trova lo slot occupato.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// `FRTScenarioSession::Start` tratta lo stesso spawn come fallibile («impossibile creare il turn
	// manager»): senza guardia qui il fallimento sarebbe un dereference nullo che abbatte la RUN, invece di
	// un test rosso con un motivo.
	if (!TM)
	{
		AddError(TEXT("turn manager non creabile"));
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	int32 Interrogato = 0;
	TM->ReactionDecider.BindLambda([&Interrogato](const FRTReactionOpportunity&, int32) -> FString
	{
		++Interrogato;
		return URTReactionOpportunityLibrary::HoldResponse();
	});

	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.TestDeciderWins");
	Scenario.MapRadius = 5;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
		return X;
	};
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));

	{
		FRTScenarioTurn T;
		FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);
		FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
		Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
		T.Intents.Add(Corre);
		// Nessuna `decisions`: lo scenario non ne dichiara, quindi non c'e' residuo e nessuna finestra
		// risulta scoperta — i due controlli del task 6 non entrano in gioco qui.
		Scenario.Turns.Add(T);
	}

	FRTTestExpectation E;
	E.Kind = ERTAssertionKind::TurnsCompleted;
	E.Value = 1;
	Scenario.Expect.Add(E);

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	RTWorldFixtures::DestroyWorld(World);

	TestEqual(TEXT("il referto nomina la sorgente"), Result.DecisionSource, FString(TEXT("test-override")));
	TestEqual(TEXT("la session non ha applicato nulla di suo"), Result.ScriptedDecisionsApplied, 0);
	TestTrue(FString::Printf(TEXT("e a rispondere e' stato il decisore del test (interrogato %d volte)"),
		Interrogato), Interrogato >= 1);

	// L'ULTIMO anello: il campo deve arrivare fino a `result.json`, che e' il file che qualcuno legge
	// davvero. Dichiarato nella struct e trasportato nel referto non basta — sono tre anelli, e il piano
	// scriveva il writer senza che nessun test lo leggesse.
	FString Dir, WriteError;
	if (TestTrue(TEXT("report scritto su disco"),
		URTTestReportWriter::Write(Result, TEXT("decisionsource"), Dir, WriteError)))
	{
		FString Json;
		if (TestTrue(TEXT("result.json rileggibile"),
			FFileHelper::LoadFileToString(Json, *FPaths::Combine(Dir, TEXT("result.json")))))
		{
			TestTrue(TEXT("il json dichiara la sorgente"), Json.Contains(TEXT("decisionSource")));
			TestTrue(TEXT("e il suo valore e' quello del referto"), Json.Contains(TEXT("test-override")));
			TestTrue(TEXT("il json porta i due contatori"),
				Json.Contains(TEXT("scriptedDecisionsApplied")) &&
				Json.Contains(TEXT("scriptedDecisionsUnused")));
		}
	}
	return true;
}

/**
 * Mutazione: lo stesso scenario, con `FIRE` e con `HOLD`, deve dare un esito DIVERSO. Se non lo desse, il
 * turno sarebbe verde comunque e la decisione non conterebbe — che e' precisamente cio' che il golden
 * replay di `#170` ha bisogno di escludere.
 *
 * Il DoD chiedeva «sostituendo il provider con uno che restituisce un esito, cada almeno uno scenario». La
 * firma del delegate e' `FString`, quindi un esito non e' nemmeno esprimibile: quel test non avrebbe una
 * premessa costruibile e sarebbe verde senza poter fallire. La verifica e' quindi COMPORTAMENTALE.
 *
 * ⚠️ **I due rami dichiarano un numero DIVERSO di decisioni, e l'asimmetria e' il punto.** Il piano ne
 * dichiarava una per ramo, ma e' stato scritto prima del task 6: un `FIRE` tronca il movimento e apre UNA
 * finestra, un `HOLD` non spende la carica e ne apre DUE. Col ramo `HOLD` a una sola decisione la seconda
 * finestra resterebbe scoperta e lo scenario andrebbe in `Error` — non per la mutazione, ma per il
 * controllo introdotto due task fa. Il conteggio delle finestre e' cosi' esso stesso un'evidenza, e piu'
 * leggibile dell'hash quando cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionMutationTest,
	"RefactorTactics.ShowcaseRelay.DecisionsChangeTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionMutationTest::RunTest(const FString&)
{
	// Due esecuzioni che differiscono per UNA cosa sola: cosa risponde `Guardia`. Geometria del task 5,
	// quella misurata — e nessun `Requires`, o il turno sarebbe `Blocked` prima degli intent.
	auto Costruisci = [](bool bScriptaFire)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("Internal.DecisionsChangeTheOutcome");
		S.MapRadius = 5;
		auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
		{
			FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
			return X;
		};
		S.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
		S.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));

		FRTScenarioTurn T;
		FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);
		FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
		Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
		T.Intents.Add(Corre);

		if (bScriptaFire)
		{
			// Il `FIRE` tronca: una finestra sola, quindi una decisione sola.
			FRTScenarioDecision D;
			D.Unit = TEXT("Guardia"); D.Respond = TEXT("FIRE"); D.Target = TEXT("Corsa");
			T.Decisions.Add(D);
		}
		else
		{
			// Il `HOLD` lascia proseguire: due finestre, e servono due decisioni o la seconda e' scoperta.
			for (int32 i = 0; i < 2; ++i)
			{
				FRTScenarioDecision D;
				D.Unit = TEXT("Guardia"); D.Respond = TEXT("HOLD");
				T.Decisions.Add(D);
			}
		}
		S.Turns.Add(T);

		FRTTestExpectation E;
		E.Kind = ERTAssertionKind::TurnsCompleted;
		E.Value = 1;
		S.Expect.Add(E);
		return S;
	};

	// Due mondi distinti e isolati: se condividessero il mondo, la seconda esecuzione partirebbe dallo stato
	// lasciato dalla prima e la differenza fra i due hash non direbbe piu' nulla sulla decisione.
	const FRTTestResult ConFire = RTWorldFixtures::RunScenarioIsolated(Costruisci(true));
	const FRTTestResult ConHold = RTWorldFixtures::RunScenarioIsolated(Costruisci(false));

	// Nessuno dei due deve essere in errore: un `Error` renderebbe la differenza fra gli hash priva di
	// significato, perche' i due turni non sarebbero stati giocati entrambi.
	TestEqual(FString::Printf(TEXT("il ramo FIRE ha giocato (error='%s')"), *ConFire.ErrorMessage),
		static_cast<int32>(ConFire.Outcome), static_cast<int32>(ERTTestOutcome::Pass));
	TestEqual(FString::Printf(TEXT("il ramo HOLD ha giocato (error='%s')"), *ConHold.ErrorMessage),
		static_cast<int32>(ConHold.Outcome), static_cast<int32>(ERTTestOutcome::Pass));

	// Il conteggio E' gia' un'evidenza: una finestra col `FIRE`, due col `HOLD`.
	TestEqual(TEXT("col FIRE si apre una sola finestra"), ConFire.ScriptedDecisionsApplied, 1);
	TestEqual(TEXT("col HOLD se ne aprono due"), ConHold.ScriptedDecisionsApplied, 2);
	TestEqual(TEXT("nessun residuo nel ramo FIRE"), ConFire.ScriptedDecisionsUnused, 0);
	TestEqual(TEXT("nessun residuo nel ramo HOLD"), ConHold.ScriptedDecisionsUnused, 0);

	// La mutazione vera: `FIRE` tronca il movimento residuo e fa danno, `HOLD` no. Se i due hash
	// coincidessero, la decisione non avrebbe cambiato niente e tutto il resto sarebbe verde a vuoto.
	TestNotEqual(TEXT("FIRE e HOLD producono stati diversi"),
		static_cast<int32>(ConFire.StateHash), static_cast<int32>(ConHold.StateHash));
	return true;
}

/**
 * Una risposta scriptata RIFIUTATA dal manager non deve passare per un `HOLD` qualunque: `HoldRejected` e
 * `HoldChosen` hanno lo stesso effetto sul gioco e significati opposti per chi legge il referto.
 *
 * `Alleato` e' della STESSA squadra del guardiano e non si muove, quindi `FIRE:<lui>` non e' fra le
 * `AllowedResponses` della finestra e il manager lo rifiuta. La legalita' resta decisa da
 * `IsResponseAllowed` in un posto solo: qui si legge l'esito, non si duplica la politica.
 *
 * ⚠️ **Due decisioni e non una, per la stessa ragione del task 8**: un rifiuto diventa `HoldRejected`, che
 * non spende la carica, quindi il movimento prosegue e si apre una SECONDA finestra. Con una sola decisione
 * quella resterebbe scoperta e lo scenario cadrebbe per il controllo del task 6 — con un messaggio che non
 * nomina il bersaglio rifiutato. Sarebbe verde per la ragione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionRejectedTest,
	"RefactorTactics.ShowcaseRelay.RejectedScriptedResponseFailsTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionRejectedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.RejectedScriptedResponse");
	Scenario.MapRadius = 5;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
		return X;
	};
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"),  1, FRTCellId( 2,  0, 0), ERTHexDirection::W));
	Scenario.Units.Add(U(TEXT("Alleato"), TEXT("Hero.Riktor"), 1, FRTCellId( 3, -1, 0), ERTHexDirection::W));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),    0, FRTCellId(-3,  0, 0), ERTHexDirection::E));

	FRTScenarioTurn T;
	FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
	T.Intents.Add(Arma);
	FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
	Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
	T.Intents.Add(Corre);

	FRTScenarioDecision Rifiutata;
	Rifiutata.Unit = TEXT("Guardia"); Rifiutata.Respond = TEXT("FIRE");
	Rifiutata.Target = TEXT("Alleato"); // schierato, quindi il loader lo accetta — ma non e' offerto
	T.Decisions.Add(Rifiutata);
	FRTScenarioDecision Copre;
	Copre.Unit = TEXT("Guardia"); Copre.Respond = TEXT("HOLD");
	T.Decisions.Add(Copre);
	Scenario.Turns.Add(T);

	FRTTestExpectation E;
	E.Kind = ERTAssertionKind::TurnsCompleted;
	E.Value = 1;
	Scenario.Expect.Add(E);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	TestEqual(TEXT("lo scenario e' in errore"), static_cast<int32>(Result.Outcome),
		static_cast<int32>(ERTTestOutcome::Error));
	// Il messaggio deve nominare il BERSAGLIO SCRIPTATO, non il token: `FIRE:<indice>` non contiene
	// «Alleato», e un referto che dice solo «una risposta e' stata rifiutata» manda a rileggere lo scenario
	// intero per capire quale.
	TestTrue(FString::Printf(TEXT("il messaggio nomina la decisione rifiutata (era: '%s')"),
		*Result.ErrorMessage), Result.ErrorMessage.Contains(TEXT("Alleato")));
	// E il rifiuto non deve essere confuso col residuo: entrambe le decisioni sono state consumate.
	TestEqual(TEXT("entrambe le decisioni consumate"), Result.ScriptedDecisionsApplied, 2);
	TestEqual(TEXT("nessun residuo"), Result.ScriptedDecisionsUnused, 0);
	return true;
}

/**
 * Uno scenario che non scripta nulla deve LASCIARE IL SEAM COM'ERA, e il delegate non deve sopravvivergli.
 *
 * Trovato in code review, ed erano due difetti nello stesso punto:
 *
 * 1. bindare sempre zittiva il bot. `AskReactionDecision` raggiunge `URTHexBotLibrary::DecideReactionResponse`
 *    **solo** se il delegate non e' legato — «il decisore iniettato ha la precedenza su tutto, bot compreso»
 *    — quindi un'unita' del bot con un Overwatch armato avrebbe smesso di reagire in ogni scenario. E per un
 *    proprietario umano la voce del TurnLog sarebbe passata da `HoldNoDecider` a `HoldTimeout`: una
 *    differenza di BYTE per il corpus golden;
 * 2. `BindRaw(this, ...)` lascia un puntatore grezzo in un delegate posseduto dall'ATTORE, che sopravvive
 *    alla sessione. `URTScenarioRunner::Run` usa `RunSingle(..., bTearDownAfter=false)`: la sessione e' una
 *    locale che muore al `return`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDeciderLifetimeTest,
	"RefactorTactics.ShowcaseRelay.UnscriptedScenarioLeavesTheSeamAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDeciderLifetimeTest::RunTest(const FString&)
{
	auto Costruisci = [](const TCHAR* Id, bool bConDecisioni)
	{
		FRTTestScenario S;
		S.ScenarioId = Id;
		S.MapRadius = 5;
		auto U = [](const TCHAR* UId, const TCHAR* Hero, int32 Team, const FRTCellId& C, ERTHexDirection F)
		{
			FRTScenarioUnit X; X.Id = UId; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; X.Facing = F;
			return X;
		};
		S.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Wraith"), 1, FRTCellId( 2, 0, 0), ERTHexDirection::W));
		S.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Gadget"),   0, FRTCellId(-3, 0, 0), ERTHexDirection::E));
		FRTScenarioTurn T;
		FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia");
		Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);
		FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
		Corre.Move.Add(FRTCellId(-2, 0, 0)); Corre.Move.Add(FRTCellId(-1, 0, 0));
		T.Intents.Add(Corre);
		if (bConDecisioni)
		{
			for (int32 i = 0; i < 2; ++i)
			{
				FRTScenarioDecision D; D.Unit = TEXT("Guardia"); D.Respond = TEXT("HOLD");
				T.Decisions.Add(D);
			}
		}
		S.Turns.Add(T);
		FRTTestExpectation E;
		E.Kind = ERTAssertionKind::TurnsCompleted;
		E.Value = 1;
		S.Expect.Add(E);
		return S;
	};

	// ⚠️ **Un mondo per esecuzione, e non e' pignoleria.** Senza `TearDown` — che `RunSingle` non chiama sul
	// percorso normale — le unita' del primo scenario restano nel mondo: un secondo `Run` ne troverebbe il
	// doppio e gli indici di risoluzione non tornerebbero. E' una proprieta' dell'harness che precede questa
	// feature; qui si evita usando due mondi, come fa `RunScenarioIsolated`.
	auto Esegui = [this](const FRTTestScenario& Scenario, bool& bOutSeamLegatoDopo)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { AddError(TEXT("mondo non creabile")); return FRTTestResult(); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM)
		{
			AddError(TEXT("turn manager non creabile"));
			RTWorldFixtures::DestroyWorld(World);
			return FRTTestResult();
		}
		const FRTTestResult R = URTScenarioRunner::Run(World, Scenario);
		// Letto PRIMA di distruggere il mondo: e' l'istante in cui un delegate sopravvissuto si vedrebbe.
		bOutSeamLegatoDopo = TM->ReactionDecider.IsBound();
		RTWorldFixtures::DestroyWorld(World);
		return R;
	};

	// Senza `decisions`: il seam non va toccato, o il bot smette di reagire e il TurnLog cambia byte.
	bool bLegatoDopoMuto = true;
	const FRTTestResult Muto = Esegui(Costruisci(TEXT("Internal.Unscripted"), false), bLegatoDopoMuto);
	TestEqual(TEXT("uno scenario senza decisioni non dichiara una sorgente"),
		Muto.DecisionSource, FString(TEXT("none")));
	TestFalse(TEXT("e non lascia il seam legato"), bLegatoDopoMuto);

	// Con `decisions`: la sessione binda, risponde, e a esecuzione finita NON resta legata — la sessione e'
	// una locale dentro `RunSingle`, e senza il distruttore il delegate punterebbe a memoria morta.
	bool bLegatoDopoScriptato = true;
	const FRTTestResult Scriptato = Esegui(Costruisci(TEXT("Internal.Scripted"), true), bLegatoDopoScriptato);
	TestEqual(TEXT("uno scenario con decisioni dichiara di aver risposto"),
		Scriptato.DecisionSource, FString(TEXT("scenario")));
	TestEqual(TEXT("e le ha applicate"), Scriptato.ScriptedDecisionsApplied, 2);
	TestFalse(TEXT("ma il delegate non sopravvive alla sessione"), bLegatoDopoScriptato);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.

/**
 * ── `GrayKitYard`: la scena della seduta `U25`+`U35` ────────────────────────────────────────────────
 *
 * 🔴 **Questi test non dicono che la scena si LEGGE — dicono che c'e'.** L'oracolo di «e' leggibile» non
 * esiste nell'harness e non va simulato: sta in `PIE-GBX-*` e `PIE-GRID-CONFINE`, e lo da' una persona.
 * Cio' che si puo' asserire headless e' che chi apre quella fixture trovi davvero le quattro cose che le
 * dieci verifiche devono guardare — e questo si', perche' e' un fatto sui DATI.
 *
 * Serve perche' l'allestimento a mano ha un modo noto di fallire in silenzio: le porte si scrivevano in
 * `Cells[i].Doors` dal Details, e senza forzare il ridisegno si guardava la geometria vecchia. Una voce
 * sembrava fallita mentre era solo non allestita. Una fixture testata non ha quel modo di sbagliare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayKitYardCarriesEverySubjectTest,
	"RefactorTactics.Scenario.GrayKitYardCarriesEverySubject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayKitYardCarriesEverySubjectTest::RunTest(const FString&)
{
	const URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("GrayKitYard"));
	if (!TestNotNull(TEXT("la fixture GrayKitYard si risolve per nome"), Yard))
	{
		return false;
	}

	// Stesso corpo di CoverYard: e' il requisito di confrontabilita' di `PIE-GBX-FIT`, non un dettaglio.
	TestEqual(TEXT("e' l'esagono di raggio 3, come CoverYard"), Yard->NumCells(), 37);

	int32 CoverHigh = 0, CoverLow = 0;
	int32 DoorOpen = 0, DoorClosed = 0, DoorLocked = 0, DoorDestroyed = 0;
	int32 Water = 0, Ice = 0;
	for (const FRTHexCellData& Cell : Yard->Cells)
	{
		for (const FRTHexCover& Cover : Cell.Covers)
		{
			if (Cover.Type == ERTHexCoverType::High) { ++CoverHigh; }
			else if (Cover.Type == ERTHexCoverType::Low) { ++CoverLow; }
		}
		for (const FRTHexDoor& Door : Cell.Doors)
		{
			switch (Door.State)
			{
			case ERTHexDoorState::Open:      ++DoorOpen;      break;
			case ERTHexDoorState::Closed:    ++DoorClosed;    break;
			case ERTHexDoorState::Locked:    ++DoorLocked;    break;
			case ERTHexDoorState::Destroyed: ++DoorDestroyed; break;
			default: break;
			}
		}
		if (Cell.Surface == ERTHexSurface::ShallowWater) { ++Water; }
		else if (Cell.Surface == ERTHexSurface::Ice)     { ++Ice; }
	}

	// `PIE-GBX-COVER`: le due coperture che si confrontano a una riga di distanza.
	TestEqual(TEXT("una copertura alta"), CoverHigh, 1);
	TestEqual(TEXT("una copertura bassa"), CoverLow, 1);

	// 🔑 `PIE-GBX-DOOR`: i QUATTRO stati insieme. Nessun'altra fixture li porta — `CoverYard` non ha porte
	// e `RelayBasin` ne ha una sola, chiusa. Escluderne uno coprirebbe tre stati su quattro.
	TestEqual(TEXT("porta Open"), DoorOpen, 1);
	TestEqual(TEXT("porta Closed"), DoorClosed, 1);
	TestEqual(TEXT("porta Locked, che D-171 vuole distinta da Closed"), DoorLocked, 1);
	TestEqual(TEXT("porta Destroyed, stato terminale"), DoorDestroyed, 1);

	// `PIE-GBX-SURFACE`: due superfici diverse. DUE celle ciascuna, non una — vedi il test accanto.
	TestEqual(TEXT("due celle d'acqua"), Water, 2);
	TestEqual(TEXT("due celle di ghiaccio"), Ice, 2);

	return true;
}

/**
 * 🔑 **Il test che protegge il caso piu' difficile della griglia, ed e' quello che si perde per primo.**
 *
 * `PIE-GBX-SURFACE` vuole due superfici **diverse** adiacenti; `PIE-GRID-CONFINE` vuole l'opposto — due celle
 * della **stessa** superficie, dove il colore non dice dove finisce una e comincia l'altra. Con una cella per
 * tipo la fixture soddisferebbe la prima e non la seconda, e il giudizio piu' difficile della griglia
 * resterebbe non guardabile **proprio nella scena costruita per guardarlo**.
 *
 * L'adiacenza si CHIEDE a `URTHexLibrary::Distance` invece di fidarsi delle coordinate scritte nel builder:
 * un refuso in una delle quattro celle darebbe una scena plausibile e sbagliata, e nessun conteggio se ne
 * accorgerebbe — i totali del test sopra tornerebbero comunque.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayKitYardHasSameSurfaceNeighboursTest,
	"RefactorTactics.Scenario.GrayKitYardHasSameSurfaceNeighbours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayKitYardHasSameSurfaceNeighboursTest::RunTest(const FString&)
{
	const URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("GrayKitYard"));
	if (!TestNotNull(TEXT("la fixture GrayKitYard si risolve"), Yard))
	{
		return false;
	}

	// Le celle per superficie, raccolte dai DATI e non riscritte qui.
	TArray<FRTCellId> WaterCells;
	TArray<FRTCellId> IceCells;
	for (const FRTHexCellData& Cell : Yard->Cells)
	{
		if (Cell.Surface == ERTHexSurface::ShallowWater) { WaterCells.Add(Cell.Id); }
		else if (Cell.Surface == ERTHexSurface::Ice)     { IceCells.Add(Cell.Id); }
	}

	auto HasAdjacentPair = [](const TArray<FRTCellId>& Cells)
	{
		for (int32 I = 0; I < Cells.Num(); ++I)
		{
			for (int32 J = I + 1; J < Cells.Num(); ++J)
			{
				if (URTHexLibrary::HexDistance(Cells[I], Cells[J]) == 1) { return true; }
			}
		}
		return false;
	};

	TestTrue(TEXT("due celle d'ACQUA sono adiacenti: il confine su superficie uguale si puo' guardare"),
		HasAdjacentPair(WaterCells));
	TestTrue(TEXT("due celle di GHIACCIO sono adiacenti, idem"),
		HasAdjacentPair(IceCells));

	// E il caso opposto, che `PIE-GBX-SURFACE` guarda: acqua e ghiaccio si toccano da qualche parte.
	bool bWaterTouchesIce = false;
	for (const FRTCellId& W : WaterCells)
	{
		for (const FRTCellId& I : IceCells)
		{
			if (URTHexLibrary::HexDistance(W, I) == 1) { bWaterTouchesIce = true; }
		}
	}
	TestTrue(TEXT("acqua e ghiaccio si toccano: il confronto fra superfici DIVERSE c'e'"), bWaterTouchesIce);

	return true;
}

/**
 * La fixture e' raggiungibile **per nome**, che e' il modo in cui la seduta la userà: `FixtureId` nel
 * Details dell'actor, o `"fixture"` in uno scenario JSON. Una fixture che esiste in codice e non e' in
 * `KnownFixtureIds()` non la trova nessuno — e `MakeFixtureArena` risponde `nullptr`, cioe' una partita che
 * fallisce parlando di unita' fuori mappa invece che del nome mancante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayKitYardIsAdvertisedTest,
	"RefactorTactics.Scenario.GrayKitYardIsAdvertisedByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayKitYardIsAdvertisedTest::RunTest(const FString&)
{
	TestTrue(TEXT("GrayKitYard e' fra le fixture note, quindi la tendina e gli scenari la vedono"),
		URTMatchSetupLibrary::KnownFixtureIds().Contains(TEXT("GrayKitYard")));
	return true;
}

/**
 * ── `BlockYard`: i tre casi di blocco, affiancati ───────────────────────────────────────────────────
 *
 * 🔴 **Come i test di `GrayKitYard`, questo NON dice che la scena si legge.** Quell'oracolo non esiste
 * nell'harness e non va simulato: sta in `PIE-HEX-VIZ-BLOCCHI` e lo da' una persona guardando dall'alto.
 * Qui si asserisce il fatto sui DATI che rende quel giudizio possibile — che i tre casi ci siano tutti e
 * tre, e che siano vicini abbastanza da confrontarsi.
 *
 * Serve perche' il caso **solo-movimento** non esisteva in nessuna delle otto fixture precedenti, ed era
 * sempre lui a mancare. Chi eseguiva la voce partiva da una fixture, ne trovava due su tre e dipingeva il
 * terzo a mano: fermarsi prima lasciava il confronto a due, ed e' il modo in cui la voce e' caduta il
 * 2026-08-20.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlockYardCarriesAllThreeBlockCasesTest,
	"RefactorTactics.Scenario.BlockYardCarriesAllThreeBlockCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlockYardCarriesAllThreeBlockCasesTest::RunTest(const FString&)
{
	const URTHexMapAsset* Yard = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("BlockYard"));
	if (!TestNotNull(TEXT("la fixture BlockYard si risolve per nome"), Yard))
	{
		return false;
	}

	// Stesso corpo di `CoverYard`: le due fixture di leggibilita' si guardano alla stessa distanza di camera.
	TestEqual(TEXT("e' l'esagono di raggio 3, come CoverYard"), Yard->NumCells(), 37);

	TArray<FRTCellId> SoloMovimento, SoloVista, Entrambi;
	int32 SuperficiDiverse = 0;
	for (const FRTHexCellData& Cell : Yard->Cells)
	{
		if (Cell.bBlocksMovement && !Cell.bBlocksLineOfSight)      { SoloMovimento.Add(Cell.Id); }
		else if (!Cell.bBlocksMovement && Cell.bBlocksLineOfSight) { SoloVista.Add(Cell.Id); }
		else if (Cell.bBlocksMovement && Cell.bBlocksLineOfSight)  { Entrambi.Add(Cell.Id); }

		if (Cell.Surface != ERTHexSurface::Floor) { ++SuperficiDiverse; }
	}

	// I tre casi. Sono l'unica ragione per cui questa fixture esiste, e si contano per ESATTEZZA: una in piu'
	// renderebbe ambiguo quale cella la voce sta giudicando.
	TestEqual(TEXT("una cella blocca SOLO il movimento — il caso che nessun'altra fixture porta"),
		SoloMovimento.Num(), 1);
	TestEqual(TEXT("una cella blocca SOLO la vista"), SoloVista.Num(), 1);
	TestEqual(TEXT("una cella blocca ENTRAMBI"), Entrambi.Num(), 1);

	// 🔑 **L'adiacenza e' parte del criterio, non estetica.** La voce chiede che la cella con entrambi mostri
	// «due volumi concentrici, non una terza forma ambigua»: e' un confronto, e un confronto si fa con il
	// vicino. Se un giorno qualcuno spostasse una delle tre dall'altra parte della mappa i conti tornerebbero
	// e il giudizio diventerebbe impossibile — questo test cadrebbe, ed e' il suo mestiere.
	if (SoloMovimento.Num() == 1 && SoloVista.Num() == 1 && Entrambi.Num() == 1)
	{
		const TArray<FRTCellId> ViciniDiEntrambi = URTHexLibrary::Neighbors(Entrambi[0]);
		TestTrue(TEXT("la cella ENTRAMBI e' adiacente a quella solo-movimento"),
			ViciniDiEntrambi.Contains(SoloMovimento[0]));
		TestTrue(TEXT("la cella ENTRAMBI e' adiacente a quella solo-vista"),
			ViciniDiEntrambi.Contains(SoloVista[0]));
	}

	// Stessa disciplina di `CoverYard`, e qui e' un'asserzione invece che un commento: una superficie diversa
	// darebbe una seconda spiegazione a «quella cella si legge diversa», e la voce chiede della FORMA.
	TestEqual(TEXT("nessuna superficie diversa dal pavimento: l'unica variabile sono i due flag"),
		SuperficiDiverse, 0);
	return true;
}

/**
 * Raggiungibile **per nome**, come la sorella: e' cosi' che la seduta la usera' — `FixtureId` nel Details
 * dell'actor, poi `GenerateFixtureIntoAsset`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlockYardIsAdvertisedTest,
	"RefactorTactics.Scenario.BlockYardIsAdvertisedByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlockYardIsAdvertisedTest::RunTest(const FString&)
{
	TestTrue(TEXT("BlockYard e' fra le fixture note, quindi la tendina e gli scenari la vedono"),
		URTMatchSetupLibrary::KnownFixtureIds().Contains(TEXT("BlockYard")));
	return true;
}

// =====================================================================================================
// CP 15.4 (#170) — il golden replay degli otto turni.
//
// ⚠️ **Quattro test, e nessuno dei quattro ripete `Simulation.GoldenCorpusMatches`.** Quello confronta le
// tracce di ogni corpus col proprio riferimento su disco, showcase compreso da questo checkpoint, ed e' la
// verifica di UGUAGLIANZA. Qui si verificano quattro cose che l'uguaglianza non copre:
//
//   · che il golden non sia **vacuo** — otto tracce identiche a otto tracce vuote combaciano benissimo;
//   · che la **diagnosi** di una divergenza nomini turno, fase e azione, cioe' che serva a qualcosa;
//   · che l'ordine di spawn non cambi lo stato **a ogni turno**, non solo alla fine;
//   · che gli eventi chiave siano **prodotti**, e non solo rappresentabili.
// =====================================================================================================

namespace
{
	/** Carica lo showcase versionato. Falso — con errore gia' registrato — se non si carica. */
	bool RTLoadShowcaseRelay(FAutomationTestBase& Test, FRTTestScenario& OutScenario)
	{
		FString LoadError;
		const FString Path = URTScenarioIndex::ResolvePath(TEXT("RT_Showcase_Relay_v01"), LoadError);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, OutScenario, LoadError))
		{
			Test.AddError(FString::Printf(TEXT("RT_Showcase_Relay_v01 non si carica: %s"), *LoadError));
			return false;
		}
		return true;
	}

	/** Tutte le voci di TurnLog della partita, in ordine di turno. Vuoto se una traccia non si rilegge. */
	TArray<FRTTurnLogEntry> RTShowcaseAllLogEntries(const FRTTestResult& Result, FAutomationTestBase& Test)
	{
		TArray<FRTTurnLogEntry> Tutte;
		for (int32 i = 0; i < Result.TurnTraces.Num(); ++i)
		{
			TArray<FRTTurnLogEntry> Voci;
			if (!URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[i].Bytes, Voci))
			{
				Test.AddError(FString::Printf(TEXT("la traccia del turno %d non si rilegge"), i + 1));
				return TArray<FRTTurnLogEntry>();
			}
			Tutte.Append(Voci);
		}
		return Tutte;
	}
}

/**
 * Il golden dell'intera partita **esiste, e' completo e non e' vuoto**.
 *
 * 🔴 **La parte che `GoldenCorpusMatches` non puo' dare, ed e' la ragione per cui questo test esiste**: un
 * confronto e' verde anche fra due cose vuote. Se lo showcase smettesse di produrre voci — un resolver che
 * non scrive piu' nel TurnLog, una serializzazione che tronca — le tracce fresche sarebbero vuote, e il
 * giorno in cui qualcuno rigenerasse il corpus lo sarebbero anche le sue. Da quel momento in poi il corpus
 * confermerebbe il nulla, e nessuno se ne accorgerebbe: `#1598` ha gia' documentato la forma simmetrica di
 * questo difetto (il file che resta e che nessuno confronta).
 *
 * ⚠️ Otto, non «almeno uno»: la CARDINALITA' e' parte dell'affermazione. Un corpus di sette file per uno
 * scenario che gioca sette turni sarebbe internamente coerente e direbbe una cosa falsa sullo showcase.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseEightTurnsGoldenTest,
	"RefactorTactics.ShowcaseRelay.EightTurnsGoldenMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseEightTurnsGoldenTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!RTLoadShowcaseRelay(*this, Scenario)) { return false; }

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);
	if (!TestEqual(TEXT("lo showcase gira fino in fondo"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito %s — %s%s"), *Result.OutcomeString(),
			*Result.BlockedReason, *Result.ErrorMessage));
		return false;
	}

	TestEqual(TEXT("otto tracce, una per turno"), Result.TurnTraces.Num(), 8);

	// 🔴 L'anti-vacuita', turno per turno. Non «il totale e' > 0»: un turno muto in mezzo a sette parlanti
	// sarebbe invisibile a una somma, e sarebbe precisamente il turno da guardare.
	for (int32 i = 0; i < Result.TurnTraces.Num(); ++i)
	{
		TArray<FRTTurnLogEntry> Voci;
		const bool bRiletta = URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[i].Bytes, Voci);
		if (TestTrue(FString::Printf(TEXT("la traccia del turno %d si rilegge"), i + 1), bRiletta))
		{
			TestTrue(FString::Printf(TEXT("il turno %d lascia almeno una voce nel TurnLog"), i + 1),
				Voci.Num() > 0);
		}
	}

	// E gli otto file sul disco, che sono l'altra meta': il confronto lo fa `GoldenCorpusMatches`, ma se un
	// file mancasse quel test lo direbbe **dentro un ciclo su sette corpus**, e la diagnosi arriverebbe
	// mescolata. Qui la cardinalita' del corpus dello showcase si legge da sola.
	const FString CorpusDir = FPaths::Combine(FPaths::ProjectDir(),
		TEXT("Source"), TEXT("RefactorTactics"), TEXT("Tests"), TEXT("Golden"), TEXT("RT_Showcase_Relay_v01"));
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(CorpusDir, TEXT("*.rttl")), true, false);
	TestEqual(TEXT("otto file `turn-NN.rttl` versionati per lo showcase"), Files.Num(), 8);

	return true;
}

/**
 * La DIAGNOSI di una divergenza, non l'uguaglianza.
 *
 * 🔴 **Un corpus che sa solo dire «diverso» finisce rigenerato invece che letto**, ed e' il difetto che
 * `DescribeFirstDivergence` esiste per impedire. Il test lo verifica sullo showcase e non su una traccia
 * costruita a mano: e' la partita piu' lunga del corpus — otto turni, quattro unita', reazioni e ambiente —
 * cioe' quella in cui «dove e' cambiato» ha un valore reale.
 *
 * ⚠️ Si guasta la traccia **del turno 8**, di proposito: e' l'ultimo, quindi un messaggio che nominasse il
 * turno per posizione anziche' per numero direbbe «turno 1» e il test lo vedrebbe. Con il primo turno i due
 * comportamenti sarebbero indistinguibili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDivergenceNamesTest,
	"RefactorTactics.ShowcaseRelay.DivergenceNamesTurnPhaseAndAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDivergenceNamesTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!RTLoadShowcaseRelay(*this, Scenario)) { return false; }

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);
	if (!TestEqual(TEXT("otto tracce da cui partire"), Result.TurnTraces.Num(), 8)) { return false; }

	TArray<FRTTurnLogEntry> Originale;
	if (!TestTrue(TEXT("la traccia del turno 8 si rilegge"),
		URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[7].Bytes, Originale)))
	{
		return false;
	}
	if (!TestTrue(TEXT("il turno 8 ha voci da guastare"), Originale.Num() > 0)) { return false; }

	// La voce dell'obiettivo e' quella che il T8 esiste per produrre: guastare LEI rende il messaggio
	// verificabile su un `ActionId` che il test conosce per nome, invece che su uno qualsiasi.
	int32 Indice = INDEX_NONE;
	for (int32 i = 0; i < Originale.Num(); ++i)
	{
		if (Originale[i].Category == ERTLogCategory::Objective) { Indice = i; break; }
	}
	if (!TestTrue(TEXT("il turno 8 porta la voce dell'obiettivo"), Indice != INDEX_NONE)) { return false; }

	TArray<FRTTurnLogEntry> Guasta = Originale;
	// `Amount` e non l'esito: un punto che vale zero invece di uno e' la divergenza piu' silenziosa che
	// questa voce possa avere — stessa categoria, stesso esito, stessa cella.
	Guasta[Indice].Amount = Originale[Indice].Amount + 7;

	const FString Messaggio = URTTurnLogLibrary::DescribeFirstDivergence(8, Originale, Guasta);
	AddInfo(FString::Printf(TEXT("diagnosi: %s"), *Messaggio));

	TestTrue(TEXT("la diagnosi nomina il TURNO"), Messaggio.Contains(TEXT("8")));
	TestTrue(TEXT("la diagnosi nomina la FASE"),
		Messaggio.Contains(TEXT("Cleanup")));
	TestTrue(TEXT("la diagnosi nomina l'AZIONE"),
		Messaggio.Contains(TEXT("Objective.Control")));

	// 🔴 **La controprova, senza la quale i tre `Contains` sopra sono compatibili con un messaggio
	// costante.** Due tracce identiche non devono produrre la stessa frase: se `DescribeFirstDivergence`
	// restituisse sempre lo stesso testo, questo test sarebbe verde e non misurerebbe niente.
	const FString Nessuna = URTTurnLogLibrary::DescribeFirstDivergence(8, Originale, Originale);
	TestNotEqual(TEXT("e su due tracce identiche dice un'altra cosa"), Nessuna, Messaggio);

	return true;
}

/**
 * L'ordine di spawn non cambia lo stato **a ogni turno**, non solo alla fine.
 *
 * 🔴 **`Simulation.ChecksumStableAcrossPermutations` confronta il solo hash FINALE**, e su una partita di
 * otto turni due divergenze che si compensano lo attraversano senza lasciare traccia. Qui si confronta
 * turno per turno: lo scenario viene troncato a K turni per K da 1 a 8, e ogni troncamento si esegue
 * diritto e permutato.
 *
 * ⚠️ **L'oracolo e' `StateHash`, NON il TurnLog**, ed e' una scelta obbligata: `TurnLog.OrderedHashIsNotThe
 * CanonicalOne` esiste perche' un digest che **ordina il proprio input** non puo' falsificare l'invarianza
 * d'ordine — sarebbe verde per costruzione, cioe' un test che dimostra la propria premessa.
 *
 * ⚠️ Le `Expect` della partita intera si SOSTITUISCONO nei troncamenti — non si azzerano. Parlano di otto
 * turni, e valutarle al turno 3 le farebbe cadere per la ragione sbagliata; ma un `expect` vuoto e' un
 * **errore di caricamento** dichiarato — *«nessuna assertion dichiarata: lo scenario passerebbe sempre»* —
 * e la prima stesura di questo test lo ha scoperto cadendo. Al loro posto va l'unica affermazione che vale
 * per ogni troncamento: che i K turni siano stati giocati tutti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseStateHashPerTurnTest,
	"RefactorTactics.ShowcaseRelay.StateHashStablePerTurnUnderPermutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseStateHashPerTurnTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!RTLoadShowcaseRelay(*this, Scenario)) { return false; }
	if (!TestEqual(TEXT("otto turni da percorrere"), Scenario.Turns.Num(), 8)) { return false; }

	// 🔴 La controprova, prima delle uguaglianze: se gli hash dei turni fossero tutti uguali fra loro, ogni
	// confronto qui sotto sarebbe verde e non direbbe niente. Si raccolgono e si conta quanti valori
	// DISTINTI producono.
	TSet<uint32> Distinti;
	TArray<uint32> PerTurno;

	for (int32 K = 1; K <= Scenario.Turns.Num(); ++K)
	{
		FRTTestScenario Troncato = Scenario;
		Troncato.Turns.SetNum(K);
		Troncato.Expect.Reset();
		FRTTestExpectation TurniGiocati;
		TurniGiocati.Kind = ERTAssertionKind::TurnsCompleted;
		TurniGiocati.Value = K;
		Troncato.Expect.Add(TurniGiocati);

		FRTTestScenario Permutato = Troncato;
		Algo::Reverse(Permutato.Units);
		for (FRTScenarioTurn& Turno : Permutato.Turns)
		{
			Algo::Reverse(Turno.Intents);
		}

		const FRTTestResult Diritto = RTWorldFixtures::RunScenarioIsolated(Troncato);
		const FRTTestResult Rovescio = RTWorldFixtures::RunScenarioIsolated(Permutato);

		if (Diritto.Outcome == ERTTestOutcome::Error || Rovescio.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("troncamento a %d turni non eseguibile: %s / %s"),
				K, *Diritto.ErrorMessage, *Rovescio.ErrorMessage));
			return false;
		}

		TestEqual(FString::Printf(TEXT("turno %d: stesso numero di turni giocati"), K),
			Rovescio.TurnsPlayed, Diritto.TurnsPlayed);
		TestEqual(FString::Printf(TEXT("turno %d: stesso StateHash (%08x vs %08x)"),
			K, Rovescio.StateHash, Diritto.StateHash), Rovescio.StateHash, Diritto.StateHash);

		AddInfo(FString::Printf(TEXT("turno %d: StateHash %08x"), K, Diritto.StateHash));
		Distinti.Add(Diritto.StateHash);
		PerTurno.Add(Diritto.StateHash);
	}

	// 🔴 **SETTE valori distinti su otto turni, e la collisione e' NOTA, misurata e attesa.**
	//
	// Questa riga asseriva `Distinti.Num() == Turns.Num()`, cioe' otto, e passava. ⚠️ **Passava per il motivo
	// sbagliato**: nel digest c'era `Energy`, che cresceva di `EnergyPerTurn` a ogni Cleanup di ogni unita'
	// viva. Un contatore monotono rende ogni turno distinto **qualunque cosa accada nella partita** — faceva
	// da marca temporale, non da stato. [D-324](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) l'ha
	// tolta dal gameplay, e la misura ha smesso di essere mascherata.
	//
	// **T1 e T2 producono lo stesso hash, e lo scenario dichiara perche'**: il T2 e' il *whiff* — Wraith
	// arma `Hero.Wraith.InterceptShot` su una cella che nessuno attraversa. Nessuno si muove, nessuno subisce
	// danno, nessuno cambia stato: dei sette campi del digest non ne cambia **nessuno**.
	//
	// 🔴 **Ma una cosa cambia, e il digest non la vede: il COOLDOWN.** `InterceptShot` dichiara `Cooldown 2`,
	// e il catalogo eroi scrive che *«chi scommette paga il cooldown anche quando sbaglia, ed e' la meta' del
	// costo che rende il whiff una scelta»*. Dopo il T2 Wraith **non puo' rifare** quell'azione per due turni:
	// e' stato di gioco, e due partite che differissero solo per quello hanno lo stesso `StateHash`. E' la
	// stessa classe di difetto che [D-261](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha corretto
	// per il `Facing`, e soddisfa il criterio di `D-284` (3) — *«un campo entra solo se due oggetti possono
	// differire solo per quello»*.
	//
	// ⛔ **Non e' corretto qui**: aggiungere un campo al digest e' una decisione, non un adattamento di test.
	// Owner: #2366.
	//
	// ✅ **Questa riga e' il segnalibro.** Quando il cooldown entrera' nel digest, T1 e T2 si separeranno e
	// questa assertion cadra' — e chi la legge trovera' scritto qui che la caduta e' la **conferma** attesa,
	// non una regressione: si porta il `7` a `8` e si cancella questo blocco.
	TestEqual(TEXT("sette distinti su otto: T1 e T2 collidono perche' il whiff non tocca nessun campo del "
	               "digest — il cooldown che paga non ne fa parte"),
		Distinti.Num(), 7);

	// La collisione e' ESATTAMENTE quella dichiarata, non una qualsiasi: se collidessero due altri turni il
	// conteggio resterebbe 7 e la riga sopra tacerebbe.
	//
	// ⚠️ Nessuna guardia `if (PerTurno.Num() >= 2)`: un salto silenzioso toglierebbe di mezzo proprio
	// l'assertion piu' stretta senza dirlo. Il loop gira su otto turni gia' verificati sopra — se ne
	// producesse meno, e' un errore da dichiarare, non da aggirare.
	if (!TestEqual(TEXT("premessa: un hash per turno"), PerTurno.Num(), Scenario.Turns.Num()))
	{
		return false;
	}
	TestEqual(TEXT("e la collisione e' T1 con T2, non un'altra coppia"), PerTurno[0], PerTurno[1]);

	return true;
}

/**
 * Gli otto eventi chiave sono **prodotti**, non solo rappresentabili.
 *
 * 🔴 **La differenza fra i due verbi e' l'intero valore di questo test.** Il TurnLog ha una categoria per
 * ciascuno di questi eventi da mesi: un test che verificasse che l'`enum` li contiene sarebbe verde su un
 * gioco che non ne emette nessuno. Qui si guarda cosa lo showcase ha davvero scritto.
 *
 * ⚠️ **Si asserisce l'IDENTITA' di cio' che manca, non il conteggio**, ed e' la stessa forma di
 * `GoldenCorpusCoversItsCategories`: un cambio che perdesse un evento guadagnandone un altro terrebbe il
 * numero fermo e questo test verde, mentre la promessa diventerebbe falsa senza segnale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseKeyEventsTest,
	"RefactorTactics.ShowcaseRelay.EveryKeyEventAppearsInTheLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseKeyEventsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!RTLoadShowcaseRelay(*this, Scenario)) { return false; }

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);
	const TArray<FRTTurnLogEntry> Voci = RTShowcaseAllLogEntries(Result, *this);
	if (!TestTrue(TEXT("la partita ha lasciato un TurnLog"), Voci.Num() > 0)) { return false; }
	AddInfo(FString::Printf(TEXT("il TurnLog dello showcase porta %d voci su %d turni"),
		Voci.Num(), Result.TurnTraces.Num()));

	// Gli otto eventi chiave del DoD, ciascuno con il predicato che lo riconosce. ⚠️ **Predicati e non
	// categorie**: «stato» e «KO» vivono entrambi sotto `Combat` — [D-162] mette la causa in `ActionId` e la
	// gravita' in `Outcome` — quindi una mappa evento→categoria li confonderebbe.
	struct FEventoChiave
	{
		const TCHAR* Nome;
		TFunction<bool(const FRTTurnLogEntry&)> Riconosce;
	};
	const FEventoChiave Chiave[] = {
		{ TEXT("azione dichiarata"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Move
				&& static_cast<ERTMoveOutcome>(E.Outcome) == ERTMoveOutcome::Moved; } },
		{ TEXT("fallback"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Fallback; } },
		{ TEXT("danno"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Combat
				&& static_cast<ERTCombatOutcome>(E.Outcome) == ERTCombatOutcome::Hit; } },
		{ TEXT("stato"), [](const FRTTurnLogEntry& E)
			{ return E.ActionId == FName(TEXT("Status.Burning")); } },
		{ TEXT("reazione"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Reaction
				&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Activated; } },
		{ TEXT("ambiente"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Environment; } },
		{ TEXT("obiettivo"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Objective
				&& static_cast<ERTObjectiveOutcome>(E.Outcome) == ERTObjectiveOutcome::Team0Scores; } },
		{ TEXT("KO"), [](const FRTTurnLogEntry& E)
			{ return E.Category == ERTLogCategory::Combat
				&& static_cast<ERTCombatOutcome>(E.Outcome) == ERTCombatOutcome::Lethal; } },
	};

	TArray<FString> Mancanti;
	for (const FEventoChiave& Evento : Chiave)
	{
		bool bProdotto = false;
		for (const FRTTurnLogEntry& E : Voci)
		{
			if (Evento.Riconosce(E)) { bProdotto = true; break; }
		}
		if (!bProdotto) { Mancanti.Add(Evento.Nome); }
	}
	Mancanti.Sort();

	// ⛔ **DUE degli otto non sono prodotti, e sono dichiarati invece che nascosti** (`#170`). La voce di DoD
	// chiedeva che tutti e otto lo fossero: misurato, sei lo sono. Le due che mancano hanno ragioni diverse,
	// ed entrambe sono di CONTENUTO — non di codice: il TurnLog sa scriverle entrambe, e altri scenari del
	// corpus le producono.
	//
	//   · **`KO`** — il §T8 della spec chiede che Gadget vada KO mentre Phase segna. Misurato: non c'e'
	//     nessuno che possa ucciderlo. Resta a (-3,-1,0) dal T4, le due unita' rosse sono a cinque celle e
	//     piu' dopo la scivolata del T7, e la sua cella e' `Floor` — nessun hazard. Scrivere l'intento non lo
	//     farebbe morire; asserire il KO renderebbe il turno rosso.
	//   · **`fallback`** — nessuna azione dello showcase ripiega. La categoria e' emessa da SETTE siti del
	//     resolver e uno scenario che la produce esiste (`Visual.Combat.FallbackTargetMoved`, il fallback
	//     `AttackCell` di un bersaglio uscito di portata): qui semplicemente non capita, perche' ogni
	//     bersaglio dichiarato e' ancora raggiungibile quando l'azione risolve. ⚠️ **Il whiff del T2 NON e'
	//     un fallback**: `PredictionWhiffed` e' `Predictive`, e la previsione mancata e' l'esito voluto di
	//     quel turno, non un ripiego.
	//
	// Owner di entrambe: `#2149`.
	//
	// ⚠️ **Questa riga cade il giorno in cui una delle due comparisse**, ed e' voluto: sarebbe la notizia che
	// lo showcase ha guadagnato un evento chiave, e va registrata qui invece di essere ereditata in silenzio.
	// E' la stessa disciplina che `GoldenCorpusCoversItsCategories` applica alle categorie scoperte —
	// l'IDENTITA' di cio' che manca, non il conteggio: perderne uno guadagnandone un altro terrebbe il
	// numero a sei e questo test verde.
	TestEqual(TEXT("e a mancare sono esattamente `fallback` e `KO` (contenuto, non codice — owner #2149)"),
		FString::Join(Mancanti, TEXT(", ")), FString(TEXT("fallback, KO")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
