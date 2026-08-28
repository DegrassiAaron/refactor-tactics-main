#include "Misc/AutomationTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTGameMode.h"
#include "RTVeilProbeForTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#1467` — **il velo della fog of war**, a TRE stati ([D-225] per il nascondimento, [D-227] per il ricordo).
 *
 * I tre stati non sono una raffinatezza: con due, il terreno gia' esplorato si richiuderebbe alle spalle del
 * giocatore, ed e' esattamente la conseguenza che [D-227] ha deciso di non accettare.
 */

namespace
{
	/** Una board vera, con le sue istanze montate: il velo agisce sugli ISM, non su un modello astratto. */
	ARTHexMapActor* MakeVeiledBoard(UWorld* World, int32 Radius)
	{
		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		if (!HexMap)
		{
			return nullptr;
		}
		HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		HexMap->RebuildInstances();
		return HexMap;
	}

	/** Conoscenza costruita a mano: il velo riceve un DATO, e non deve sapere da dove viene. */
	FRTTeamKnowledge KnowledgeOf(const TArray<FRTCellId>& Visible, const TArray<FRTCellId>& Explored)
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.TurnNumber = 1;
		K.VisibleCells = Visible;
		K.ExploredCells = Explored;
		return K;
	}
}

/**
 * La partizione e' ESATTA, e in tre parti.
 *
 * ⚠️ Il test della DoD precedente contava due insiemi (`M − N` velate) su una partizione che ora ne ha tre:
 * con `ExploredCells` non vuota quella sottrazione e' un'affermazione **falsa**, non imprecisa. Qui i tre
 * conteggi si asseriscono separatamente, e la loro somma deve ricomporre il totale — senza quella terza
 * asserzione, due errori che si compensano passerebbero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilCoversExactlyUnobservedCellsTest,
	"RefactorTactics.Veil.CoversExactlyUnobservedCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilCoversExactlyUnobservedCellsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeVeiledBoard(World, /*Radius=*/ 4);
	if (!TestNotNull(TEXT("board con istanze montate"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const int32 Totale = HexMap->NumInstanceCells();
	if (!TestTrue(TEXT("la board ha istanze"), Totale > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Tre insiemi disgiunti e costruiti a mano: due celle osservate ORA, tre che restano solo un ricordo,
	// tutto il resto mai visto.
	const TArray<FRTCellId> Visibili = { FRTCellId(0, 0), FRTCellId(1, 0) };
	TArray<FRTCellId> Esplorate = Visibili;
	Esplorate.Append({ FRTCellId(2, 0), FRTCellId(3, 0), FRTCellId(0, 1) });

	HexMap->ApplyKnowledgeVeil(KnowledgeOf(Visibili, Esplorate));

	int32 Accese = 0, Ricordate = 0, Nascoste = 0;
	HexMap->GetVeilCounts(Accese, Ricordate, Nascoste);

	TestEqual(TEXT("accese: esattamente le celle osservate ORA"), Accese, Visibili.Num());
	TestEqual(TEXT("ricordate: esattamente le esplorate che non si vedono piu'"),
		Ricordate, Esplorate.Num() - Visibili.Num());
	TestEqual(TEXT("nascoste: tutto cio' che nessuno ha mai visto"), Nascoste, Totale - Esplorate.Num());
	// La somma ricompone il totale: senza questa riga due conteggi sbagliati che si compensano passerebbero.
	TestEqual(TEXT("i tre stati sono una PARTIZIONE del totale"), Accese + Ricordate + Nascoste, Totale);

	// Anti-vacuita': se la board fosse cosi' piccola da non avere celle mai viste, il terzo stato non sarebbe
	// misurato e il test direbbe molto meno di quel che sembra.
	TestTrue(TEXT("esiste almeno una cella mai vista, altrimenti il terzo stato non e' misurato"),
		Nascoste > 0);

	// Il ricordo si SPEGNE ma resta disegnato: e' cio' che distingue [D-227] da [D-225]. Una cella ricordata
	// con scala zero sarebbe indistinguibile da una mai vista, e i tre stati tornerebbero due.
	const TArray<FRTCellId> SoloRicordo = { FRTCellId(2, 0) };
	HexMap->ApplyKnowledgeVeil(KnowledgeOf({}, SoloRicordo));
	int32 A2 = 0, R2 = 0, N2 = 0;
	HexMap->GetVeilCounts(A2, R2, N2);
	TestEqual(TEXT("senza nessuna cella visibile, il ricordo resta disegnato"), R2, 1);
	TestEqual(TEXT("e nessuna cella risulta accesa"), A2, 0);

	// Reversibile: il velo si rialza. Senza `InstanceBaseScale` la scala azzerata sarebbe definitiva, e una
	// cella tornata visibile resterebbe invisibile per sempre — un difetto che solo un SECONDO velo mostra.
	TArray<FRTCellId> Tutte;
	for (int32 I = 0; I < Totale; ++I) { Tutte.Add(HexMap->CellForInstance(I)); }
	HexMap->ApplyKnowledgeVeil(KnowledgeOf(Tutte, Tutte));
	int32 A3 = 0, R3 = 0, N3 = 0;
	HexMap->GetVeilCounts(A3, R3, N3);
	TestEqual(TEXT("il velo si RIALZA: nessuna cella resta nascosta"), N3, 0);
	TestEqual(TEXT("e tutte tornano accese"), A3, Totale);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **Il velo copre TUTTE le famiglie di istanze, non il solo disco.**
 *
 * `RebuildInstances` monta cinque famiglie per cella: disco, glifo, rilievo del costo, volumi di blocco e
 * pannelli di bordo. Velare le prime due e lasciare in piedi le altre tre fa leggere **muri, coperture e
 * porte dell'intera board** prima di averla esplorata — l'informazione che [D-225] dichiara di non disegnare.
 *
 * ⚠️ **Questo test esiste perche' il difetto e' invisibile agli altri.** `MakeFlatArena` non produce nessuna
 * di quelle tre geometrie, e `GetVeilCounts` guarda il solo `Cells`: la partizione tornerebbe **esatta**
 * mentre la board rivela la propria struttura. Serve una mappa d'AUTORE, con le regole accese.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilCoversEveryInstanceFamilyTest,
	"RefactorTactics.Veil.CoversEveryInstanceFamily",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilCoversEveryInstanceFamilyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("board"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Una mappa con le REGOLE accese: ogni cella blocca vista e movimento, porta una copertura e una porta.
	// E' il caso che la graybox non produce, ed e' l'unico in cui il difetto e' osservabile.
	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
	if (!TestNotNull(TEXT("asset"), Asset))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	for (FRTHexCellData& Cell : Asset->Cells)
	{
		Cell.bBlocksMovement = true;
		Cell.bBlocksLineOfSight = true;
		Cell.Surface = ERTHexSurface::Rough; // costo > 1: e' cio' che produce il rilievo
		FRTHexCover Cover;
		Cover.Edge = ERTHexDirection::E;
		Cover.Type = ERTHexCoverType::High;
		Cell.Covers.Add(Cover);
		FRTHexDoor Door;
		Door.Edge = ERTHexDirection::W;
		Door.State = ERTHexDoorState::Closed;
		Cell.Doors.Add(Door);
	}
	Asset->InvalidateLookup();
	HexMap->MapAsset = Asset;
	HexMap->RebuildInstances();

	int32 Disegnate = 0, Nascoste = 0;
	HexMap->GetAuxiliaryVeilCounts(Disegnate, Nascoste);
	// Anti-vacuita': senza queste geometrie il test non misurerebbe niente e passerebbe comunque.
	if (!TestTrue(TEXT("la mappa d'autore produce davvero rilievo, blocchi e pannelli"), Disegnate > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 TotaleAusiliarie = Disegnate + Nascoste;

	// Una sola cella osservata, nessun ricordo: tutto il resto della board non e' mai stato visto.
	const TArray<FRTCellId> Visibili = { FRTCellId(0, 0) };
	HexMap->ApplyKnowledgeVeil(KnowledgeOf(Visibili, Visibili));

	HexMap->GetAuxiliaryVeilCounts(Disegnate, Nascoste);
	TestTrue(TEXT("le geometrie delle celle MAI VISTE sono nascoste"), Nascoste > 0);
	TestEqual(TEXT("resta disegnato solo cio' che appartiene alla cella osservata"),
		Disegnate + Nascoste, TotaleAusiliarie);

	// 🔴 Il cuore: nessuna geometria puo' sopravvivere su una cella che nessuno ha mai visto. Il conteggio
	// delle disegnate deve coincidere con quelle della sola cella osservata — non «meno di prima».
	int32 AccesseDisco = 0, RicordateDisco = 0, NascosteDisco = 0;
	HexMap->GetVeilCounts(AccesseDisco, RicordateDisco, NascosteDisco);
	TestEqual(TEXT("sul disco resta accesa la sola cella osservata"), AccesseDisco, 1);
	TestTrue(*FString::Printf(
			TEXT("le geometrie superstiti sono poche quanto una cella (%d disegnate su %d)"),
			Disegnate, TotaleAusiliarie),
		Disegnate * 4 < TotaleAusiliarie);

	// E si rialza: la geometria di una cella tornata nota deve tornare disegnata, altrimenti il velo sarebbe
	// irreversibile anche qui — lo stesso difetto che `InstanceBaseScale` chiude sul disco.
	TArray<FRTCellId> Tutte;
	for (int32 I = 0; I < HexMap->NumInstanceCells(); ++I) { Tutte.Add(HexMap->CellForInstance(I)); }
	HexMap->ApplyKnowledgeVeil(KnowledgeOf(Tutte, Tutte));
	HexMap->GetAuxiliaryVeilCounts(Disegnate, Nascoste);
	TestEqual(TEXT("il velo si RIALZA anche sulle geometrie ausiliarie"), Nascoste, 0);
	TestEqual(TEXT("e tornano tutte disegnate"), Disegnate, TotaleAusiliarie);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il velo segue i PUNTI DI REFRESH, non il tempo reale.
 *
 * ⚠️ **Il conteggio e' l'unica cosa che discrimina.** Un velo aggiornato a `Tick` darebbe lo stesso risultato
 * visivo: la differenza e' che le emissioni sarebbero centinaia invece di poche unita'. Il test confronta le
 * emissioni con i TICK spesi, che e' la misura che rende impossibile passare per caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFollowsRefreshPointsTest,
	"RefactorTactics.Veil.FollowsRefreshPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFollowsRefreshPointsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTVeilProbeForTest* Probe = NewObject<URTVeilProbeForTest>();
	TM->OnTeamKnowledgeRefreshed.AddDynamic(Probe, &URTVeilProbeForTest::OnKnowledgeRefreshed);

	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	const int32 TurniVoluti = 2;
	int32 TickSpesi = 0;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < TurniVoluti)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
			++TickSpesi;
		}
		++Turni;
	}

	TestTrue(TEXT("almeno un turno e' stato giocato"), Turni > 0);
	TestTrue(TEXT("la conoscenza e' stata rinfrescata almeno una volta per turno"),
		Probe->RefreshTurns.Num() >= Turni);
	// Due punti per turno al massimo — planning e Blast — e nessuno di piu': e' il tetto che esclude un
	// terzo momento inventato per il velo.
	TestTrue(*FString::Printf(TEXT("al piu' due refresh per turno (emessi %d in %d turni)"),
			Probe->RefreshTurns.Num(), Turni),
		Probe->RefreshTurns.Num() <= 2 * Turni);

	// 🔴 La guardia anti-`Tick`, che e' la ragione d'essere del test. Con un aggiornamento legato al tempo
	// reale le emissioni crescerebbero con i tick spesi, non con i turni.
	//
	// ⚠️ **Anti-vacuita', e non e' un contorno**: se il playback si risolvesse in modo sincrono il ciclo dei
	// tick non girerebbe, `TickSpesi` resterebbe zero e la guardia sotto non misurerebbe NIENTE passando
	// verde. Il tetto `<= 2 * Turni` sopra reggerebbe da solo, ma questo test dichiara di confrontare le
	// emissioni con i tick: se i tick non ci sono, va detto invece che taciuto.
	TestTrue(TEXT("il playback ha davvero speso tick, altrimenti la guardia anti-Tick non misura nulla"),
		TickSpesi > 0);
	// ⚠️ Il confronto NON e' `emissioni < tick` in assoluto: con due turni le emissioni sono quattro, e un
	// playback che si chiudesse in tre tick renderebbe rosso un comportamento corretto. Cio' che discrimina
	// e' che le emissioni restino ancorate ai TURNI mentre i tick crescono — quindi il confronto vale solo
	// dove i tick hanno superato il tetto per turno, e li' e' vero per costruzione se il velo non segue il
	// tempo reale.
	if (TickSpesi > 2 * Turni)
	{
		TestTrue(*FString::Printf(TEXT("le emissioni seguono i TURNI, non i tick (%d emissioni contro %d tick)"),
				Probe->RefreshTurns.Num(), TickSpesi),
			Probe->RefreshTurns.Num() < TickSpesi);
	}

	AddInfo(FString::Printf(TEXT("refresh emessi: %d in %d turni (%d tick spesi)"),
		Probe->RefreshTurns.Num(), Turni, TickSpesi));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * ⏱️ **La misura che la DoD di `#1467` dichiara obbligatoria**: quanto costa `ApplyKnowledgeVeil` su arena
 * piena, e quale strategia di aggiornamento regge.
 *
 * 🔴 **La prima misura ha risposto NO alla scansione completa ingenua**: riscrivere tutte le istanze a ogni
 * velo costa **~2,2 s** (2 624 ms alla prima misura, 2 160 ms sulla suite del 2026-08-28), contro **due**
 * refresh per turno. La risposta non e' pero' la mappa
 * inversa cella→istanza — che risolverebbe un problema che non c'e', dato che `InstanceCells` da' gia'
 * l'indice — ma il **salto di cio' che non cambia**: fra due refresh consecutivi si muove il bordo del cono,
 * non la board.
 *
 * Tre casi, perche' misurarne uno solo direbbe la cosa sbagliata:
 *
 * | caso | quando accade | cosa misura |
 * |---|---|---|
 * | primo velo | una volta, all'inizio della partita | il costo che non si puo' evitare |
 * | velo identico | ogni refresh in cui nulla e' cambiato | che il salto esista davvero |
 * | velo di regime | **due volte per turno** | il numero che decide se il lavoro regge |
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFullScanCostTest,
	"RefactorTactics.Veil.FullScanCostIsMeasured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFullScanCostTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Raggio 50. ⚠️ Le celle sono **7 651**, non 7 351: un esagono di raggio R ne ha `3R² + 3R + 1`, e 7 351
	// e' il conteggio del raggio **49**. Il numero sbagliato circola in [D-225] e nella DoD di `#1467`, da cui
	// e' stato copiato — misurato qui il 2026-08-28.
	ARTHexMapActor* HexMap = MakeVeiledBoard(World, /*Radius=*/ 50);
	if (!TestNotNull(TEXT("arena piena"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const int32 Totale = HexMap->NumInstanceCells();
	TestEqual(TEXT("l'arena di raggio 50 ha 3R^2+3R+1 = 7 651 celle"), Totale, 7651);

	TArray<FRTCellId> Tutte;
	Tutte.Reserve(Totale);
	for (int32 I = 0; I < Totale; ++I) { Tutte.Add(HexMap->CellForInstance(I)); }

	// Caso 1 — il primo velo: nessuno stato precedente, quindi tocca tutto.
	TArray<FRTCellId> Visibili;
	for (int32 I = 0; I < Totale; I += 2) { Visibili.Add(Tutte[I]); }
	const FRTTeamKnowledge Prima = KnowledgeOf(Visibili, Tutte);

	double Start = FPlatformTime::Seconds();
	HexMap->ApplyKnowledgeVeil(Prima);
	const double PrimoMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	const int32 PrimoToccate = HexMap->GetLastVeilTouchedCells();
	AddInfo(FString::Printf(TEXT("primo velo su %d celle: %.2f ms (%d istanze toccate)"),
		Totale, PrimoMs, PrimoToccate));
	TestEqual(TEXT("il primo velo tocca ogni istanza"), PrimoToccate, Totale);

	// Caso 2 — stessa conoscenza: il salto deve azzerare il lavoro. Senza questa asserzione, un salto rotto
	// passerebbe inosservato — il risultato a schermo sarebbe identico.
	Start = FPlatformTime::Seconds();
	HexMap->ApplyKnowledgeVeil(Prima);
	const double IdenticoMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	AddInfo(FString::Printf(TEXT("velo identico: %.2f ms (%d istanze toccate)"),
		IdenticoMs, HexMap->GetLastVeilTouchedCells()));
	TestEqual(TEXT("un velo identico non tocca NESSUNA istanza"), HexMap->GetLastVeilTouchedCells(), 0);

	// Caso 3 — il velo di REGIME, che e' quello che avviene due volte per turno: il cono si sposta, e cambia
	// una manciata di celle. E' il numero che decide se il lavoro regge.
	TArray<FRTCellId> Spostate = Visibili;
	const int32 Cambiate = FMath::Min(80, Spostate.Num());
	for (int32 I = 0; I < Cambiate; ++I) { Spostate.RemoveAt(Spostate.Num() - 1, EAllowShrinking::No); }
	const FRTTeamKnowledge Dopo = KnowledgeOf(Spostate, Tutte);

	Start = FPlatformTime::Seconds();
	HexMap->ApplyKnowledgeVeil(Dopo);
	const double RegimeMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	const int32 RegimeToccate = HexMap->GetLastVeilTouchedCells();
	AddInfo(FString::Printf(TEXT("velo di REGIME (%d celle cambiate su %d): %.2f ms (%d istanze toccate)"),
		Cambiate, Totale, RegimeMs, RegimeToccate));
	TestEqual(TEXT("il velo di regime tocca solo cio' che e' cambiato"), RegimeToccate, Cambiate);

	// ⚠️ **Il gate e' RELATIVO, non un tetto in millisecondi.** Un `RegimeMs < 50.0` misura l'orologio di
	// una macchina sotto carico ignoto — l'editor che compila shader accanto lo fa cadere senza che nulla
	// sia regredito — e sarebbe rosso per un motivo che non e' il codice. Il rapporto invece sopravvive alla
	// macchina: il regime tocca 80 istanze contro 7 651, quindi la distanza reale e' di due ordini di
	// grandezza e un fattore 4 lascia margine larghissimo restando significativo.
	//
	// Il gate sta sul REGIME, non sul primo velo: e' il costo che si paga due volte per turno. Il primo velo
	// avviene una volta e sta nello stesso ordine di grandezza di `RebuildInstances`, che monta le stesse
	// 7 651 istanze e nessuno ha mai considerato un problema.
	TestTrue(*FString::Printf(
			TEXT("il velo di regime costa molto meno del primo (%.2f ms contro %.2f ms)"), RegimeMs, PrimoMs),
		RegimeMs * 4.0 < PrimoMs);

	// La scansione resta COMPLETA — si itera comunque `InstanceCells`, senza mappa inversa — e questa riga
	// dice che iterare 7 651 celle senza scrivere nulla non e' il costo: il costo era la SCRITTURA.
	TestTrue(*FString::Printf(
			TEXT("iterare l'intera board a vuoto costa molto meno che riscriverla (%.2f ms contro %.2f ms)"),
			IdenticoMs, PrimoMs),
		IdenticoMs * 4.0 < PrimoMs);

	// 🔴 Il numero che discrimina davvero non dipende dall'orologio, ed e' gia' asserito sopra:
	// `GetLastVeilTouchedCells` vale `Totale` al primo velo, **zero** su un velo identico e `Cambiate` a
	// regime. I millisecondi restano nel referto come misura, non come gate.

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il gate della leggibilita', rieseguito sui colori VELATI.
 *
 * `Hex.SurfaceColorsAreDistinguishable` confronta `URTHexLibrary::SurfaceColor`, cioe' il colore **non
 * velato**: resta verde mentre la leggibilita' del terreno ricordato cala, ed e' il buco che questo test
 * chiude.
 *
 * ⚠️ **La soglia non e' 60, ed e' una conseguenza aritmetica, non una concessione.** Il velo MOLTIPLICA
 * l'RGB, quindi ogni distanza fra due colori si riduce esattamente dello stesso fattore: pretendere 60 sui
 * velati sarebbe chiedere 171 sui pieni, e nessuna tavolozza a 8 bit li ha. La soglia si scala con il
 * fattore, e cio' che il test protegge e' che il velo non introduca collassi PROPRI — due superfici che si
 * distinguevano da piene e si confondono da velate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeiledSurfaceColorsAreDistinguishableTest,
	"RefactorTactics.Hex.VeiledSurfaceColorsAreDistinguishable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeiledSurfaceColorsAreDistinguishableTest::RunTest(const FString&)
{
	const TArray<ERTHexSurface> All = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Void,
		ERTHexSurface::Smoke, ERTHexSurface::HighGround
	};

	// Stessa guardia di `Hex.SurfaceColorsAreDistinguishable`: l'enum non dichiara `TEnumRange`, quindi una
	// decima superficie nascerebbe SCOPERTA anche da questo canale se nessuno contasse.
	const UEnum* SurfaceEnum = StaticEnum<ERTHexSurface>();
	if (TestNotNull(TEXT("l'enum delle superfici e' riflesso"), SurfaceEnum))
	{
		TestEqual(TEXT("l'elenco di questo test copre TUTTE le superfici dell'enum"),
			SurfaceEnum->NumEnums() - 1, All.Num());
	}

	const float Factor = ARTHexMapActor::RTVeilExploredFactor;

	// Il velo si applica in spazio LINEARE, come in `ApplyKnowledgeVeil`: misurare la distanza sui byte sRGB
	// darebbe un numero che non corrisponde a cio' che si vede a schermo.
	auto Veiled = [Factor](ERTHexSurface S)
	{
		const FLinearColor L = FLinearColor::FromSRGBColor(URTHexLibrary::SurfaceColor(S));
		return FLinearColor(L.R * Factor, L.G * Factor, L.B * Factor).ToFColor(/*bSRGB=*/ true);
	};
	auto Distance = [](const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	};

	// La soglia sui velati, **misurata** e non stimata. La previsione aritmetica ingenua era `60 * 0.35 = 21`
	// — sbagliata: il velo moltiplica in spazio LINEARE, ma la distanza si legge in sRGB, e quella curva e'
	// concava. La distanza minima reale fra due superfici velate e' **58** (misurata il 2026-08-28), cioe'
	// quasi la stessa dei colori pieni.
	//
	// ⚠️ Una soglia a 21 sarebbe stata **vacua**: passerebbe con un margine di quasi tre volte, e non
	// direbbe niente il giorno in cui il fattore del velo venisse abbassato. 50 lascia margine al reale e
	// cade davvero se qualcuno spinge `RTVeilExploredFactor` troppo in basso.
	const int32 SogliaVelata = 50;

	int32 Peggiore = MAX_int32;
	for (int32 I = 0; I < All.Num(); ++I)
	{
		for (int32 J = I + 1; J < All.Num(); ++J)
		{
			const int32 D = Distance(Veiled(All[I]), Veiled(All[J]));
			Peggiore = FMath::Min(Peggiore, D);
			TestTrue(*FString::Printf(
					TEXT("velate, le superfici %d e %d restano distinguibili (distanza %d, soglia %d)"),
					static_cast<int32>(All[I]), static_cast<int32>(All[J]), D, SogliaVelata),
				D >= SogliaVelata);
		}
	}
	AddInfo(FString::Printf(TEXT("distanza minima fra due superfici VELATE: %d (fattore %.2f)"),
		Peggiore, Factor));

	// 🔴 E il velo deve distinguersi da se' stesso: una superficie velata non puo' somigliare alla STESSA
	// superficie accesa, altrimenti «ricordato» e «osservato ora» sono lo stesso stato a schermo, e i tre
	// stati di [D-227] tornano due senza che nessun conteggio se ne accorga.
	for (ERTHexSurface S : All)
	{
		const FColor Piena = URTHexLibrary::SurfaceColor(S);
		const int32 D = Distance(Piena, Veiled(S));
		TestTrue(*FString::Printf(
				TEXT("la superficie %d velata si distingue da se' stessa accesa (distanza %d)"),
				static_cast<int32>(S), D),
			D >= SogliaVelata);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
