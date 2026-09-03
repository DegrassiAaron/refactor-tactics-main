#include "Misc/AutomationTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator: contare le unita' vive distingue «velo rotto» da «partita finita»
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapVisuals.h" // RTCellPrismRadius: la convenzione della mesh che il volume di debug scala
#include "Engine/StaticMesh.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTGameMode.h"
#include "RTVeilProbeForTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTTurnManager.h"
#include "Perception/RTKnowledgeVeilPresenter.h"
#include "Unit/RTUnit.h" // ARTUnit: TActorIterator ne pretende la definizione, non basta la forward

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
 * 🔴 **`E13.8` — il velo ha un CONSUMATORE, e la board non nasce interamente visibile.**
 *
 * ⚠️ **Questo test misura il CABLAGGIO, non il meccanismo**, ed e' la distinzione che `#1467` ha pagato:
 * quel checkpoint ha costruito `ApplyKnowledgeVeil` e l'ha coperta con cinque test — tutti verdi, tutti
 * che la chiamavano **a mano** — mentre in partita non la chiamava nessuno. Un meccanismo coperto al 100%
 * e mai invocato e' indistinguibile, dalla suite, da uno che funziona.
 *
 * Qui il velo si stende perche' `ARTGameMode::HookKnowledgeVeil` lo aggancia, che e' lo **stesso** codice
 * che `BeginPlay` esegue in partita.
 *
 * ⚠️ **La prima inquadratura e' meta' del test.** Il velo deve valere PRIMA del primo refresh: senza,
 * la board nasce tutta visibile e si vela dopo, e il primo fotogramma e' quello che rivela l'intera mappa —
 * il difetto che nessuno vedrebbe, perche' dura un frame ed e' quello che nessun test tardivo prende.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilHasAConsumerTest,
	"RefactorTactics.Veil.GameModeAppliesTheVeilForTheViewerTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilHasAConsumerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// 🔴 **Senza questa riga il consumatore non riceve NIENTE, ed e' la lezione di `#939`**:
	// `AActor::ProcessEvent` scarta ogni evento se `GetWorld()->AreActorsInitialized()` e' falso, e
	// `OnTeamKnowledgeRefreshed` e' un delegate dinamico che invoca proprio da li'. `ARTGameMode` e' un
	// ACTOR, quindi senza inizializzare il mondo il suo handler non gira — mentre `URTVeilProbeForTest`,
	// che e' un `UObject` e non un attore, riceve regolarmente.
	//
	// ⚠️ **I sintomi sono quelli di un cablaggio rotto, e non lo e'**: iscrizione registrata (il GameMode
	// compare in `GetAllObjects()` prima E dopo il turno), delegate che emette, handler mai invocato. Misurato
	// il 2026-08-29 spendendoci undici cicli di diagnosi, prima di trovare che `#939` lo aveva gia' registrato
	// in `RTMatchEndOpensResultTests.cpp`. La differenza fra le due famiglie — attore contro `UObject` — e'
	// l'unica cosa che spiega perche' la sonda riceve e il consumatore no.
	World->InitializeActorsForPlay(FURL());

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// La sonda gia' usata da `FollowsRefreshPoints`: dice se i refresh sono stati EMESSI, cosa che i tre
	// conteggi del velo non distinguono da «emessi e ignorati».
	URTVeilProbeForTest* Probe = NewObject<URTVeilProbeForTest>();
	TM->OnTeamKnowledgeRefreshed.AddDynamic(Probe, &URTVeilProbeForTest::OnKnowledgeRefreshed);

	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	const int32 Totale = HexMap->NumInstanceCells();
	if (!TestTrue(TEXT("la board ha istanze"), Totale > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1. Prima dell'aggancio la board e' interamente disegnata ------------------------------------
	// Non e' il comportamento voluto: e' lo stato di partenza, ed e' cio' che rende non vacuo il punto 2.
	int32 A0 = 0, R0 = 0, N0 = 0;
	HexMap->GetVeilCounts(A0, R0, N0);
	TestEqual(TEXT("prima dell'aggancio nessuna cella e' nascosta (stato di partenza)"), N0, 0);

	// --- 2. LA PRIMA INQUADRATURA: l'aggancio vela SUBITO, non al primo refresh ----------------------
	GameMode->HookKnowledgeVeil();

	// 🔑 **L'ANELLO**: l'aggancio deve aver steso il velo ESATTAMENTE una volta.
	// ⚠️ Il conteggio si legge dal PRESENTER e non piu' dal GameMode (`E-SOLID` fetta 4): il viewer e'
	// del client, e in questo mondo senza `ARTPlayerController` il presenter e' quello senza proprietario.
	URTKnowledgeVeilPresenter* Presenter = GameMode->GetKnowledgeVeilPresenter();
	if (!TestNotNull(TEXT("il GameMode risolve un presenter"), Presenter))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestEqual(TEXT("l'aggancio stende il velo una volta"), Presenter->GetApplications(), 1);

	int32 A1 = 0, R1 = 0, N1 = 0;
	HexMap->GetVeilCounts(A1, R1, N1);
	TestTrue(*FString::Printf(
			TEXT("subito dopo l'aggancio la board NON e' interamente visibile (%d/%d nascoste)"), N1, Totale),
		N1 > 0);
	TestEqual(TEXT("e i tre stati restano una partizione del totale"), A1 + R1 + N1, Totale);
	AddInfo(FString::Printf(TEXT("velo alla prima inquadratura: %d accese, %d ricordate, %d nascoste su %d"),
		A1, R1, N1, Totale));

	// --- 3. Il PERCORSO VERO: un turno giocato ridipinge il velo dal delegate ------------------------
	// Non si chiama `ApplyKnowledgeVeil`: si gioca, e il velo si aggiorna perche' qualcuno lo ascolta.
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < 1)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++Turni;
	}
	TestTrue(TEXT("un turno e' stato giocato"), Turni > 0);

	// ⚠️ Diagnostica prima delle asserzioni: se la partita e' FINITA, non restano unita' vive, e
	// `RefreshTeamKnowledgeForPlanning` produce una conoscenza vuota per ogni squadra — quindi il velo
	// nasconde tutto ed e' il comportamento CORRETTO, non un difetto del cablaggio. Il conteggio delle vive
	// distingue i due casi, che dai soli tre numeri del velo sono indistinguibili.
	int32 Vive = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		if (It->IsAlive()) { ++Vive; }
	}
	const FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(Presenter->ViewerTeamId());
	AddInfo(FString::Printf(
		TEXT("dopo il turno: fase=%d, vive=%d, refresh emessi=%d, VisibleCells=%d, ExploredCells=%d"),
		static_cast<int32>(TM->GetPhase()), Vive, Probe->RefreshTurns.Num(),
		Conoscenza.VisibleCells.Num(), Conoscenza.ExploredCells.Num()));

	// 🔴 **L'ANELLO, ed e' il cuore di questo test.** I refresh sono stati EMESSI (`Probe`); se il velo non
	// e' stato ridipinto altrettante volte, il consumatore non e' agganciato — ed e' precisamente la
	// distanza fra «il meccanismo esiste» e «qualcuno lo chiama» che `#1467` ha pagato.
	//
	// ⚠️ **Si asserisce sul CONTEGGIO e non sui tre numeri del velo**, perche' quelli non discriminano: una
	// board tutta nascosta e' compatibile sia con «mai ridipinta» sia con «ridipinta con conoscenza vuota»,
	// e i due difetti hanno fix opposti. Questa riga separa i due casi prima che qualcuno debba indovinare.
	TestEqual(*FString::Printf(
			TEXT("il velo e' stato ridipinto a ogni refresh: %d applicazioni contro %d refresh emessi"),
			Presenter->GetApplications(), Probe->RefreshTurns.Num()),
		Presenter->GetApplications(), 1 + Probe->RefreshTurns.Num());


	int32 A2 = 0, R2 = 0, N2 = 0;
	HexMap->GetVeilCounts(A2, R2, N2);
	TestEqual(TEXT("dopo il turno i tre stati sono ancora una partizione"), A2 + R2 + N2, Totale);
	TestTrue(TEXT("e la board resta velata secondo cio' che la squadra vede"), N2 > 0);

	// --- 4. La vista e' di UNA squadra, e non di tutte ------------------------------------------------
	// ⚠️ Senza questa riga il test passerebbe anche con un velo che nasconde tutto o che mostra tutto: cio'
	// che discrimina e' che esista almeno una cella ACCESA, cioe' che qualcuno stia guardando davvero.
	TestTrue(*FString::Printf(TEXT("almeno una cella e' osservata dal viewer (%d accese)"), A2), A2 > 0);

	// --- 5. `ViewerTeamId` senza controller ripiega su 0, dichiaratamente ---------------------------
	TestEqual(TEXT("senza PlayerController il viewer e' la squadra 0, come FrameOwnTeam"),
		Presenter->ViewerTeamId(), 0);

	AddInfo(FString::Printf(TEXT("velo dopo un turno: %d accese, %d ricordate, %d nascoste su %d"),
		A2, R2, N2, Totale));

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

#if !UE_BUILD_SHIPPING

/**
 * 🔴 **I volumi di conoscenza dicono in ALTEZZA cio' che il velo dice in colore e presenza.**
 *
 * La partizione e' la stessa di `VeilCoversExactlyUnobservedCells` — osservata / ricordata / mai vista — ma
 * misurata su un canale diverso, ed e' il punto: il velo rende «mai vista» **non disegnando** ([D-225]),
 * quindi sul suo canale quello stato e' indistinguibile da una cella che non esiste. Qui ha un'altezza, e
 * un'altezza si conta.
 *
 * ⚠️ **I conteggi si leggono dalle ISTANZE, non da un contatore**: `GetKnowledgeDebugCounts` ricava la
 * frazione dalla scala Z reale, come `GetVeilCounts` legge la scala reale del disco. Un contatore
 * proverebbe che la funzione sa contare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVolumesPartitionTest,
	"RefactorTactics.Veil.KnowledgeVolumesPartitionTheBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVolumesPartitionTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeVeiledBoard(World, /*Radius=*/ 2);
	if (!TestNotNull(TEXT("board"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const int32 Totale = HexMap->MapAsset->NumCells();

	// Una cella osservata, una ricordata, tutte le altre mai viste. `ExploredCells` e' un SOVRAINSIEME di
	// `VisibleCells` ([D-227]), quindi la osservata compare in entrambe: e' il dato reale, non una scorciatoia.
	const FRTCellId Osservata = HexMap->MapAsset->Cells[0].Id;
	const FRTCellId Ricordata = HexMap->MapAsset->Cells[1].Id;
	HexMap->SetKnowledgeDebugEnabled(true, KnowledgeOf({ Osservata }, { Osservata, Ricordata }));

	int32 Hidden = 0;
	int32 Remembered = 0;
	int32 Lit = 0;
	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);

	TestEqual(TEXT("una sola cella osservata porta il volume 3/3"), Lit, 1);
	TestEqual(TEXT("una sola cella ricordata porta il volume 2/3"), Remembered, 1);
	TestEqual(TEXT("tutte le altre sono mai viste, e portano il volume 1/3"), Hidden, Totale - 2);

	// La somma ricompone il totale: senza, due errori che si compensano passerebbero — la stessa ragione per
	// cui il test del velo asserisce anche il totale invece dei soli tre conteggi.
	TestEqual(TEXT("i tre stati partizionano la board, senza celle perse"), Hidden + Remembered + Lit, Totale);

	// E spegnendolo non resta niente: il componente si svuota, non si limita a nascondersi.
	HexMap->SetKnowledgeDebugEnabled(false, FRTTeamKnowledge());
	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);
	TestEqual(TEXT("spento, nessun volume resta posato"), Hidden + Remembered + Lit, 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **I volumi non sopravvivono alla board che descrivono** (`#2222`).
 *
 * Era l'unica delle otto famiglie di `ARTHexMapActor` che `RebuildInstances` non azzerava: le celle si
 * rifacevano e i prismi restavano, sopra celle che nel frattempo erano diventate altre celle. In PIE si
 * vedeva come geometria che si accumula a ogni scenario.
 *
 * 🔑 **Si asserisce anche il FLAG, e non e' un di piu'.** Il difetto peggiore non era il prisma di troppo:
 * era `bKnowledgeDebug` rimasto `true` con il componente pieno di roba vecchia — stato interno e schermo
 * che divergono **in silenzio**, sulla stessa superficie che ha gia' prodotto una diagnosi di velo rotto su
 * un difetto inesistente. Un test che contasse solo le istanze lascerebbe passare proprio quella meta'.
 *
 * ➕ **Il controllo positivo e' obbligatorio**: senza `Prima > 0`, un difetto che non posasse alcun volume
 * renderebbe questo test verde per la ragione opposta a quella che interessa.
 *
 * ⛔ **Cio' che NON afferma**: che i volumi si RICOSTRUISCANO. Non devono — l'actor non conserva la
 * conoscenza da cui rifarli, e [D-242] vieta di dargliene una copia. Si rilancia `rt.Debug.Knowledge`, che
 * e' gia' la sua semantica: una fotografia, da rifare dopo ogni refresh.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVolumesDoNotSurviveARebuildTest,
	"RefactorTactics.Veil.KnowledgeVolumesDoNotSurviveARebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVolumesDoNotSurviveARebuildTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeVeiledBoard(World, /*Radius=*/ 2);
	if (!TestNotNull(TEXT("board"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Osservata = HexMap->MapAsset->Cells[0].Id;
	HexMap->SetKnowledgeDebugEnabled(true, KnowledgeOf({ Osservata }, { Osservata }));

	int32 Hidden = 0;
	int32 Remembered = 0;
	int32 Lit = 0;
	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);
	const int32 Prima = Hidden + Remembered + Lit;

	// ➕ CONTROLLO POSITIVO: c'e' davvero qualcosa da rimuovere, e il debug risulta acceso.
	if (!TestTrue(TEXT("acceso, i volumi sono posati"), Prima > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestTrue(TEXT("e il flag lo dichiara"), HexMap->IsKnowledgeDebugEnabled());

	// La board si ricostruisce: e' cio' che accade a ogni scenario, a ogni `rt.Map.Fixture` e a ogni
	// pennellata dell'editor.
	HexMap->RebuildInstances();

	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);
	TestEqual(TEXT("dopo la ricostruzione non resta posato nessun volume"), Hidden + Remembered + Lit, 0);

	// 🔑 La meta' che conta: lo stato non sopravvive a cio' che descriveva.
	TestFalse(TEXT("e il flag non dice piu' acceso"), HexMap->IsKnowledgeDebugEnabled());

	// Riaccenderlo funziona: la pulizia non ha rotto il percorso normale, l'ha solo reso ripetibile.
	HexMap->SetKnowledgeDebugEnabled(true, KnowledgeOf({ Osservata }, { Osservata }));
	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);
	TestEqual(TEXT("riacceso, i volumi tornano tutti"), Hidden + Remembered + Lit, Prima);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **Il pivot del volume sta alla BASE, e non e' una rifinitura.**
 *
 * Con il pivot centrato — la convenzione di `GetCellPrismMesh` — un volume a `1/3` affonderebbe di `1/6 H`
 * sotto il pavimento della cella, uno a `3/3` di `1/2 H`: quattro volumi che affondano di quantita' diverse
 * non si confrontano a vista, ed e' l'unica cosa che questo strumento deve permettere.
 *
 * Il test misura la MESH, non una scala: la sua bounding box deve partire da `Z = 0` e salire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVolumeMeshPivotTest,
	"RefactorTactics.Veil.KnowledgeVolumeMeshHasBasePivot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVolumeMeshPivotTest::RunTest(const FString&)
{
	UStaticMesh* Mesh = ARTHexMapActor::GetKnowledgeVolumeMesh();
	if (!TestNotNull(TEXT("la mesh del volume di conoscenza"), Mesh)) { return false; }

	const FBox Box = Mesh->GetBoundingBox();

	TestTrue(*FString::Printf(TEXT("la base sta a Z=0 (misurato %.2f)"), Box.Min.Z),
		FMath::IsNearlyEqual(Box.Min.Z, 0.0, 0.01));
	TestTrue(*FString::Printf(TEXT("il tetto sta a 2R (misurato %.2f)"), Box.Max.Z),
		FMath::IsNearlyEqual(Box.Max.Z, static_cast<double>(RTCellPrismRadius) * 2.0, 0.01));

	// E la pianta e' quella del disco: i due esagoni nascono entrambi da `HexCorners`, quindi un volume di
	// debug si sovrappone ESATTAMENTE alla cella che descrive invece di sbordare.
	//
	// 🔴 **Il circumraggio si misura su Y, non su X, e la prima stesura di questo test lo cercava su X.**
	// L'esagono e' POINTY-TOP: `HexCorners` parte da -30 gradi, quindi due vertici stanno in alto e in basso
	// — `Max.Y = R` — mentre lungo X il bordo piu' lontano e' il centro di un LATO, cioe' l'apotema
	// `R·√3/2 = 43.30`. Il test chiedeva `50` su X e trovava `43.30`: era l'asserzione a sbagliarsi, non la
	// mesh. Asserirli entrambi e' anche piu' forte del solo raggio — insieme dicono che l'esagono e'
	// pointy-top, e un flat-top li scambierebbe facendo cadere il test.
	TestTrue(*FString::Printf(TEXT("il circumraggio, su Y, e' RTCellPrismRadius (misurato %.2f)"), Box.Max.Y),
		FMath::IsNearlyEqual(Box.Max.Y, static_cast<double>(RTCellPrismRadius), 0.01));
	TestTrue(*FString::Printf(TEXT("l'apotema, su X, e' R*sqrt(3)/2 (misurato %.2f)"), Box.Max.X),
		FMath::IsNearlyEqual(Box.Max.X, static_cast<double>(RTCellPrismRadius) * FMath::Sqrt(3.0) / 2.0, 0.01));

	return true;
}

/**
 * ⚠️ **Le quattro frazioni sono un vocabolario, e tre di loro hanno uno stato.**
 *
 * Questo test pinna il mapping — `1/3` mai vista, `2/3` ricordo, `3/3` osservata — e soprattutto pinna che
 * `1/2` **non e' usata da nessuno dei tre**. Il giorno in cui qualcuno le assegnasse un significato, il
 * modello di conoscenza dovrebbe avere un quarto stato che oggi non ha: `FRTTeamKnowledge` porta due insiemi
 * e il terzo stato e' l'assenza da entrambi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVolumeFractionsTest,
	"RefactorTactics.Veil.KnowledgeVolumeFractionsAreThreeOfFour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVolumeFractionsTest::RunTest(const FString&)
{
	TestEqual(TEXT("mai vista -> 1/3"),
		ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeHidden], 1.f / 3.f);
	TestEqual(TEXT("ricordo -> 2/3"),
		ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeRemembered], 2.f / 3.f);
	TestEqual(TEXT("osservata -> 3/3"),
		ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeLit], 1.f);

	// L'indice 1 e' `1/2`, e nessuno dei tre stati lo nomina: e' il vocabolario disponibile, non un quarto
	// significato. Se questa riga cade, e' perche' qualcuno ha assegnato uno stato che il modello non ha.
	TestTrue(TEXT("la quarta frazione (1/2) non e' assegnata a nessuno stato"),
		ARTHexMapActor::RTKnowledgeVolumeHidden != 1
		&& ARTHexMapActor::RTKnowledgeVolumeRemembered != 1
		&& ARTHexMapActor::RTKnowledgeVolumeLit != 1);
	TestEqual(TEXT("e vale comunque 1/2, perche' e' stata chiesta"),
		ARTHexMapActor::RTKnowledgeVolumeFractions[1], 1.f / 2.f);

	// L'ordine e' crescente: piu' si sa, piu' il volume e' alto. Invertirlo passerebbe i tre `TestEqual` qui
	// sopra e renderebbe lo strumento illeggibile — «basso» smetterebbe di voler dire «se ne sa poco».
	TestTrue(TEXT("piu' si sa, piu' il volume e' alto"),
		ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeHidden]
			< ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeRemembered]
		&& ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeRemembered]
			< ARTHexMapActor::RTKnowledgeVolumeFractions[ARTHexMapActor::RTKnowledgeVolumeLit]);

	return true;
}

#endif // !UE_BUILD_SHIPPING



/**
 * 🔴 **Il difetto di `#1762`, preso alla RADICE invece che al sintomo.**
 *
 * Il sintomo era: al primo turno i click non muovono. La radice e' che all'allestimento la conoscenza di
 * squadra e' **vuota**, perche' l'unico produttore — `RefreshTeamKnowledgeForPlanning` dentro `PlanBots`
 * dentro `StartPlanningTimer` — gira nel `BeginPlay` del TurnManager, cioe' PRIMA che
 * `SetupHexMatch` spawni le unita'.
 *
 * 🔑 **L'asserzione e' sulle celle ACCESE, e la scelta e' il punto.** `FirstFrameVeilsImmediately`
 * asserisce gia' che dopo l'aggancio qualcosa sia nascosto (`N > 0`), e quel test restava **verde** col
 * difetto attivo: una board interamente nascosta soddisfa `N > 0` benissimo. Il complemento — che almeno
 * una cella resti accesa — e' cio' che nessuno verificava, ed e' esattamente cio' che il giocatore puo'
 * cliccare: una cella non disegnata non ha collisione, e il click e' un raycast.
 *
 * ⚠️ Non asserisce un NUMERO di celle accese: dipende da mappa, roster e `VisionRange`, e pinnarlo
 * renderebbe il test fragile a un cambio di bilanciamento che non c'entra. La soglia che conta e' **zero**.
 *
 * 🔴 **E il CABLAGGIO si asserisce, non si suppone.** Una prima stesura chiamava
 * `RefreshTeamKnowledgeNow()` a mano e si fermava li': **verifica di mutazione FALLITA** il 2026-08-30 —
 * rimuovendo la chiamata da `ARTGameMode::BeginPlay` il test restava **verde**, perche' non passava dal
 * sito che il difetto riguarda. Provava che il metodo funziona, non che qualcuno lo chiama: la stessa
 * distanza fra «il meccanismo esiste» e «qualcuno lo invoca» che `#1467` ha pagato col velo mai chiamato.
 * ✅ Ora il test misura **prima e dopo**: la finestra vuota esiste davvero, e il refresh la chiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFirstApplyHasNonEmptyKnowledgeTest,
	"RefactorTactics.Veil.FirstApplyHasNonEmptyKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFirstApplyHasNonEmptyKnowledgeTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Come `FirstFrameVeilsImmediately`: senza, il delegate dinamico del GameMode non viene mai invocato
	// (`AActor::ProcessEvent` scarta gli eventi con gli attori non inizializzati — la lezione di `#939`).
	World->InitializeActorsForPlay(FURL());

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	const int32 Totale = HexMap->NumInstanceCells();
	if (!TestTrue(TEXT("la board ha istanze"), Totale > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1a. LA FINESTRA: dopo lo spawn del roster, e PRIMA del cablaggio, la conoscenza e' VUOTA ----
	// 🔴 E' il difetto di `#1762` colto sul fatto: l'unico refresh finora avvenuto e' quello dentro il
	// `BeginPlay` del TurnManager, uscito con zero unita' perche' `SetupHexMatch` non le aveva ancora
	// spawnate. Senza questa asserzione il punto 1b sarebbe vacuo: proverebbe che la conoscenza e'
	// popolata, non che qualcosa l'ha popolata.
	const FRTTeamKnowledge Prima = TM->KnowledgeForTeamPublic(/*TeamId=*/ 0);
	TestEqual(TEXT("prima del cablaggio la conoscenza della squadra 0 e' vuota (la finestra di #1762)"),
		Prima.VisibleCells.Num(), 0);

	// --- 1b. IL CABLAGGIO, e si attraversa il percorso VERO ------------------------------------------
	//
	// 🔴 **`DispatchBeginPlay()` e non `RefreshTeamKnowledgeNow()` a mano, ed e' il punto di tutto il
	// test.** Due stesure precedenti chiamavano il metodo direttamente e la **verifica di mutazione le ha
	// bocciate entrambe** (2026-08-30): togliendo la chiamata da `ARTGameMode::BeginPlay` il test restava
	// **verde**, perche' non passava dal sito che il difetto riguarda. Provava che il metodo funziona, non
	// che qualcuno lo chiama — la stessa distanza fra «il meccanismo esiste» e «qualcuno lo invoca» che
	// `#1467` ha pagato col velo mai chiamato.
	//
	// ⚠️ `SetupHexMatch` e' gia' stata chiamata sopra, e `BeginPlay` la richiama: e' idempotente sul
	// roster (`EnsureMatchRoster` non ricostruisce cio' che c'e'), e cio' che conta qui e' che la riga
	// `TurnManager->RefreshTeamKnowledgeNow()` venga eseguita DAL codice di produzione.
	GameMode->DispatchBeginPlay();

	const FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(/*TeamId=*/ 0);
	TestTrue(*FString::Printf(TEXT("la squadra 0 vede almeno una cella (VisibleCells=%d)"),
			Conoscenza.VisibleCells.Num()),
		Conoscenza.VisibleCells.Num() > 0);

	// --- 2. Il sintomo: dopo l'aggancio la board NON e' interamente nascosta ------------------------
	// ⚠️ L'aggancio l'ha gia' fatto `BeginPlay` sopra: qui non si richiama nulla, si MISURA.

	int32 Accese = 0, Ricordate = 0, Nascoste = 0;
	HexMap->GetVeilCounts(Accese, Ricordate, Nascoste);

	// 🔑 L'asserzione che il difetto rompe. Col bug: `Accese == 0`, board intera senza collisione.
	TestTrue(*FString::Printf(
			TEXT("dopo l'aggancio almeno una cella resta ACCESA e quindi cliccabile (%d accese, %d ricordate, "
				"%d nascoste, su %d)"), Accese, Ricordate, Nascoste, Totale),
		Accese > 0);

	// ⚠️ E il velo continua a fare il suo mestiere: curare il click non deve scoprire la mappa. Senza
	// questa riga la correzione piu' semplice — non velare affatto — passerebbe il test qui sopra.
	TestTrue(*FString::Printf(TEXT("il velo nasconde ancora cio' che nessuno ha visto (%d nascoste)"),
			Nascoste),
		Nascoste > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}


/**
 * 🔴 **La mappa NON si richiude alle spalle del giocatore**, giocando davvero i turni.
 *
 * E' il **criterio 4** di `PIE-HEX-VIZ-VELO`, quello *per cui quella voce esiste*: e' la conseguenza che
 * [D-227] ha deciso di non accettare — un velo senza memoria farebbe tornare invisibile la cella appena
 * attraversata appena esce dal cono, **due volte per turno**, ai due punti di refresh.
 *
 * ⚠️ **Il dato e' gia' coperto, la PRESENTAZIONE no.** `Perception.KnowledgeRemembersExploredCells`
 * verifica che `ExploredCells` sopravviva oltre `ContactLifetimeTurns`, ma lo fa sulle funzioni pure, con
 * `Observe` chiamata a mano. Nessun test del velo muoveva un'unita' e ricontrollava la **board**: il difetto
 * poteva rientrare dalla finestra della presentazione senza che nulla diventasse rosso.
 *
 * 🔑 **L'invariante e' la MONOTONIA, non un conteggio.** Le celle nascoste possono solo **diminuire**:
 * `ExploredCells` cresce in modo monotono e non scade, quindi cio' che e' stato visto una volta resta almeno
 * *ricordato* per sempre. Asserire numeri esatti legherebbe il test a mappa, roster e `VisionRange` — fragile
 * a ogni cambio di bilanciamento; asserire che l'insieme nascosto non cresca e' la stessa garanzia, e non
 * invecchia.
 *
 * ⚠️ Vale **anche** se un'unita' muore o la partita finisce: in quel caso la conoscenza si svuota e il
 * velo nasconde tutto, il che farebbe **crescere** le nascoste — comportamento corretto e non regressione.
 * Per questo il test conta le unita' vive e, se ne perde, dichiara il caso e non asserisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilBoardDoesNotCloseBehindThePlayerTest,
	"RefactorTactics.Veil.BoardDoesNotCloseBehindThePlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilBoardDoesNotCloseBehindThePlayerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	World->InitializeActorsForPlay(FURL()); // senza, il delegate dinamico del GameMode non arriva (#939)

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->bAutobattle = true; // i bot muovono da soli: e' il movimento che deve lasciare il ricordo
	GameMode->DispatchBeginPlay(); // il percorso VERO: allestisce, rinfresca la conoscenza e aggancia il velo

	const int32 Totale = HexMap->NumInstanceCells();
	if (!TestTrue(TEXT("la board ha istanze"), Totale > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	auto Conta = [&](int32& A, int32& R, int32& N) { HexMap->GetVeilCounts(A, R, N); };
	auto Vive = [&]()
	{
		int32 V = 0;
		for (TActorIterator<ARTUnit> It(World); It; ++It) { if (It->IsAlive()) { ++V; } }
		return V;
	};

	int32 A0 = 0, R0 = 0, N0 = 0;
	Conta(A0, R0, N0);
	const int32 Vive0 = Vive();
	AddInfo(FString::Printf(TEXT("turno iniziale: %d accese, %d ricordate, %d nascoste su %d (%d vive)"),
		A0, R0, N0, Totale, Vive0));

	// Serve che ci sia qualcosa da richiudere: se nulla e' nascosto, il test non falsificherebbe niente.
	if (!TestTrue(TEXT("all'inizio esiste terreno mai visto (altrimenti il test sarebbe vacuo)"), N0 > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- Si GIOCA: i bot si muovono, ed e' il movimento che porta celle dentro e fuori dal cono ------
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < 3)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		++Turni;

		const int32 ViveOra = Vive();
		int32 A = 0, R = 0, N = 0;
		Conta(A, R, N);
		AddInfo(FString::Printf(TEXT("dopo il turno %d: %d accese, %d ricordate, %d nascoste (%d vive)"),
			Turni, A, R, N, ViveOra));

		// ⚠️ Se la squadra ha perso unita' la conoscenza si assottiglia per una ragione LEGITTIMA: un
		// cadavere non vede. Dichiarare il caso e non asserire e' piu' onesto che asserire una cosa diversa.
		if (ViveOra < Vive0)
		{
			AddInfo(TEXT("unita' perse: la monotonia non e' piu' attesa, il test si ferma qui"));
			break;
		}

		// 🔑 L'INVARIANTE: cio' che era noto resta almeno ricordato. Le nascoste non crescono MAI.
		TestTrue(*FString::Printf(
				TEXT("turno %d: la mappa non si richiude (nascoste %d, non piu' delle %d iniziali)"),
				Turni, N, N0),
			N <= N0);

		// E i tre stati restano una partizione: nessuna istanza si perde per strada.
		TestEqual(*FString::Printf(TEXT("turno %d: i tre stati partizionano la board"), Turni),
			A + R + N, Totale);
	}

	TestTrue(TEXT("almeno un turno e' stato giocato"), Turni > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
