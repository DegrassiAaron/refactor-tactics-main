// LA LINEA DI CONFINE DELLA VISTA, in partita (#3761).
//
// Tre domande, tre test, e nessuna delle tre e' gia' coperta dai test del modello (`RTVisibilityBorderTests.cpp`):
//
//  - quali lati: in partita la linea non corre lungo il vuoto, cioe' lungo il bordo della mappa e delle
//    piattaforme (`ExposedEdgesFacingMapCells`);
//  - come si disegna: nel line batcher del mondo anche col disegno di debug spento, come lo Shipping (#3508);
//  - da dove viene: dal presenter del velo, con la STESSA conoscenza del velo.

#include "Misc/AutomationTest.h"
#include "Components/LineBatchComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTOverlayPalette.h"
#include "Perception/RTKnowledgeVeilPresenter.h"
#include "Perception/RTVisibilityBorder.h"
#include "RTConsoleVariableGuardForTest.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un'arena piatta del raggio chiesto, centrata sull'origine (nomi distinti per file, unity build). */
	URTHexMapAsset* MakeArenaConfine(int32 Raggio)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Raggio);
	}

	/** Vero se ogni lato dell'elenco da' su una cella della mappa. */
	bool OgniLatoDaSuUnaCella(const URTHexMapAsset* Map, const TArray<FRTExposedEdge>& Lati)
	{
		for (const FRTExposedEdge& E : Lati)
		{
			if (!Map->ContainsCell(URTHexLibrary::Neighbor(E.Cell, E.Direction)))
			{
				return false;
			}
		}
		return true;
	}
}

/**
 * 🔑 **In partita la linea segna dove finisce la VISTA, non dove finisce il PAVIMENTO.**
 *
 * Ogni caso ha il suo controllo positivo su `ExposedEdges`, che il vuoto lo conta: senza, un filtro che togliesse
 * tutto passerebbe il caso dell'arena intera, e un filtro che non togliesse niente passerebbe quello della cella
 * centrale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionBorderSkipsTheVoidTest,
	"RefactorTactics.Visibility.BorderInMatchSkipsTheVoid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionBorderSkipsTheVoidTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeArenaConfine(2);
	if (!TestNotNull(TEXT("arena"), Map)) { return false; }

	TArray<FRTCellId> Tutte;
	for (const FRTHexCellData& Cella : Map->Cells)
	{
		Tutte.Add(Cella.Id);
	}

	// 1. L'arena vista per intero: tutti i lati esposti danno sul vuoto, e in partita non resta niente.
	TestTrue(TEXT("controllo: vista l'arena intera, il modello conta i lati sul bordo"),
		URTVisibilityBorderLibrary::ExposedEdges(Tutte).Num() > 0);
	TestEqual(TEXT("vista l'arena intera, in partita non c'e' nessuna linea"),
		URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(Map, Tutte).Num(), 0);

	// 2. La sola cella centrale: i sei vicini esistono, quindi i sei lati restano.
	const FRTCellId Centro(0, 0, 0);
	TestEqual(TEXT("la cella centrale ha sei lati verso celle della mappa"),
		URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(Map, { Centro }).Num(), 6);

	// 3. Una cella sul bordo: restano i lati verso le celle che esistono, e solo quelli.
	const FRTCellId Bordo(2, 0, 0);
	if (!TestTrue(TEXT("premessa: la cella di bordo sta nell'arena"), Map->ContainsCell(Bordo))) { return false; }
	const TArray<FRTExposedEdge> Modello = URTVisibilityBorderLibrary::ExposedEdges({ Bordo });
	const TArray<FRTExposedEdge> Partita = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(Map, { Bordo });
	int32 VerosoCelle = 0;
	for (const FRTExposedEdge& E : Modello)
	{
		VerosoCelle += Map->ContainsCell(URTHexLibrary::Neighbor(E.Cell, E.Direction)) ? 1 : 0;
	}
	TestEqual(TEXT("controllo: il modello da' sei lati alla cella di bordo"), Modello.Num(), 6);
	TestTrue(TEXT("premessa: la cella di bordo ha vicini fuori dall'arena"), VerosoCelle < 6);
	TestEqual(TEXT("in partita restano i lati verso celle che esistono"), Partita.Num(), VerosoCelle);
	TestTrue(TEXT("e ognuno da' su una cella della mappa"), OgniLatoDaSuUnaCella(Map, Partita));

	// 4. Una piattaforma su un piano di sopra, senza vicini sul suo piano: il suo perimetro e' vuoto, non vista
	// che finisce. E' il caso di [D-511], dove la vista scende al piano di sotto.
	const FRTCellId Piattaforma(0, 0, 1);
	Map->AddOrUpdateCell(FRTHexCellData(Piattaforma));
	Map->SortCells();
	TestEqual(TEXT("controllo: il modello da' sei lati alla piattaforma"),
		URTVisibilityBorderLibrary::ExposedEdges({ Piattaforma }).Num(), 6);
	TestEqual(TEXT("in partita il perimetro di una piattaforma isolata non ha linee"),
		URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(Map, { Piattaforma }).Num(), 0);

	// 5. Senza mappa non si sa dove sia il vuoto, e non si toglie niente.
	TestTrue(TEXT("senza mappa risponde come il modello"),
		URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(nullptr, { Bordo }) == Modello);

	return true;
}

/**
 * 🔑 **La linea si disegna anche col disegno di debug spento**, come in Shipping, ed e' la guardia contro
 * `DrawDebugLine` (#3508, D-467). Il controllo positivo e' lo stesso di `Preview.DrawsWithDebugDrawingOff`.
 *
 * ⚠️ **I conteggi sono esatti**, e gli estremi stanno sugli spigoli dell'esagono: un tratto lungo il doppio, o
 * ruotato di 90 gradi, avrebbe lo stesso numero di linee.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionBorderDrawsWithDebugDrawingOffTest,
	"RefactorTactics.Visibility.BorderLineDrawsWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionBorderDrawsWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ULineBatchComponent* Mondo = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: batcher World e Foreground, e la CVar del debug"),
		Mondo != nullptr && Primo != nullptr && Debug != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	Mondo->Flush();
	Primo->Flush();
	DrawDebugLine(World, FVector::ZeroVector, FVector(100.f, 0.f, 0.f), FColor::Red, false, -1.f, SDPG_World, 1.f);
	if (!TestEqual(TEXT("controllo: con il debug spento DrawDebugLine non disegna"), Mondo->BatchedLines.Num(), 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("mappa"), HexMap)) { RTWorldFixtures::DestroyWorld(World); return false; }
	HexMap->MapAsset = MakeArenaConfine(4);

	const FRTCellId Centro(0, 0, 0);
	HexMap->SetVisionBorder({ Centro });
	if (!TestEqual(TEXT("la mappa conserva i sei lati della cella centrale"), HexMap->GetVisionBorder().Num(), 6))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestTrue(TEXT("con una linea da disegnare la mappa ticka"), HexMap->IsActorTickEnabled());

	static_cast<AActor*>(HexMap)->Tick(0.f);
	TestEqual(TEXT("nel batcher World: un tratto per lato"), Mondo->BatchedLines.Num(), 6);
	TestEqual(TEXT("nel batcher Foreground: niente, la linea non attraversa le unita'"), Primo->BatchedLines.Num(), 0);

	// Gli estremi stanno sugli spigoli dell'esagono, in pianta.
	FVector Origin; float Size = 0.f; float LayerH = 0.f;
	HexMap->GetHexContext(Origin, Size, LayerH);
	const TArray<FVector> Spigoli = URTHexLibrary::HexCorners(URTHexLibrary::AxialToWorld(Centro, Origin, Size, LayerH), Size);
	const auto SuUnoSpigolo = [&Spigoli](const FVector& P)
	{
		for (const FVector& S : Spigoli)
		{
			if (FVector2D::Distance(FVector2D(P), FVector2D(S)) < 1.f)
			{
				return true;
			}
		}
		return false;
	};
	int32 SuGliSpigoli = 0;
	for (const FBatchedLine& Linea : Mondo->BatchedLines)
	{
		SuGliSpigoli += (SuUnoSpigolo(Linea.Start) && SuUnoSpigolo(Linea.End)) ? 1 : 0;
	}
	TestEqual(TEXT("ogni tratto va da uno spigolo all'altro dell'esagono"), SuGliSpigoli, 6);

	// Il colore e' quello della VISTA nella palette: la scelta dell'autore sulla tavola L1 di #3761.
	const FLinearColor Vista(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Vision));
	int32 ColoreDellaVista = 0;
	for (const FBatchedLine& Linea : Mondo->BatchedLines)
	{
		ColoreDellaVista += Linea.Color.Equals(Vista) ? 1 : 0;
	}
	TestEqual(TEXT("ogni tratto ha il colore della vista della palette"), ColoreDellaVista, 6);

	// Nessuna cella vista: nessuna linea, e la mappa smette di tickare per la linea.
	HexMap->SetVisionBorder({});
	Mondo->Flush();
	static_cast<AActor*>(HexMap)->Tick(0.f);
	TestEqual(TEXT("senza celle viste non si disegna niente"), Mondo->BatchedLines.Num(), 0);
	TestFalse(TEXT("e la mappa non ticka per niente"), HexMap->IsActorTickEnabled());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **La linea viene dal presenter del velo, e dalla STESSA conoscenza.** Misura il cablaggio, non il
 * meccanismo: e' la lezione di `#1467` che `Veil.GameModeAppliesTheVeilForTheViewerTeam` racconta, un
 * meccanismo testato a mano e mai chiamato in partita.
 *
 * ⚠️ Il confronto e' con le celle VISTE e non con quelle esplorate: un presenter che passasse `ExploredCells`
 * disegnerebbe il confine del ricordo, che [D-227] tiene distinto, e questo test lo vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionBorderFollowsTheVeilTest,
	"RefactorTactics.Veil.PresenterDrawsTheBorderOfTheSameKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionBorderFollowsTheVeilTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	// Senza, il GameMode — un attore — non riceve gli eventi dinamici: vedi `Veil.GameModeAppliesTheVeilForTheViewerTeam`.
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
	TestEqual(TEXT("prima dell'aggancio non c'e' nessuna linea"), HexMap->GetVisionBorder().Num(), 0);

	GameMode->HookKnowledgeVeil();
	URTKnowledgeVeilPresenter* Presenter = GameMode->GetKnowledgeVeilPresenter();
	if (!TestNotNull(TEXT("il GameMode risolve un presenter"), Presenter))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const auto Atteso = [&]()
	{
		return URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset,
			TM->KnowledgeForTeamPublic(Presenter->ViewerTeamId()).VisibleCells);
	};
	// ⚠️ All'aggancio la conoscenza e' ancora VUOTA: si riempie al primo refresh, quando si apre un turno. Qui il
	// confronto vale per costruzione — nessuna cella vista, nessuna linea — e il test vero e' dopo.
	TestTrue(TEXT("all'aggancio la linea e' il confine delle celle VISTE"), HexMap->GetVisionBorder() == Atteso());

	// DUE turni giocati: la linea si ridisegna perche' il presenter ascolta il refresh, non perche' la si chiama.
	// ⚠️ Due e non uno: il primo refresh e' la prima osservazione, e li' vista ed esplorata coincidono.
	for (int32 Turno = 0; Turno < 2 && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turno)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
	const FRTTeamKnowledge Dopo = TM->KnowledgeForTeamPublic(Presenter->ViewerTeamId());
	AddInfo(FString::Printf(TEXT("dopo due turni: VisibleCells=%d, ExploredCells=%d, lati della linea=%d"),
		Dopo.VisibleCells.Num(), Dopo.ExploredCells.Num(), HexMap->GetVisionBorder().Num()));
	TestTrue(TEXT("premessa: la squadra del viewer vede qualcosa"), Dopo.VisibleCells.Num() > 0);
	// Senza questa premessa il confronto non distinguerebbe le celle viste da quelle esplorate, e un presenter che
	// passasse `ExploredCells` resterebbe verde.
	TestTrue(TEXT("premessa: la squadra ricorda celle che non vede piu'"),
		URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, Dopo.ExploredCells)
			!= URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(HexMap->MapAsset, Dopo.VisibleCells));
	TestTrue(TEXT("la linea e' il confine delle celle viste ADESSO"), HexMap->GetVisionBorder() == Atteso());
	TestTrue(TEXT("e non e' vuota"), HexMap->GetVisionBorder().Num() > 0);

	// 🔴 **Ciascuno dei due percorsi, DA SOLO.** Dopo due turni la conoscenza del playback e quella dal vivo
	// coincidono, quindi la linea qui sopra l'ha potuta scrivere l'uno o l'altro: togliere UNA delle due chiamate a
	// `SetVisionBorder` lasciava il test verde (mutazioni `M2` e `M3` della verifica del 2026-10-15). Qui si svuota la
	// linea e si fa passare un percorso alla volta.
	HexMap->SetVisionBorder(TArray<FRTCellId>());
	TestEqual(TEXT("premessa: la linea svuotata"), HexMap->GetVisionBorder().Num(), 0);
	Presenter->Apply();
	TestTrue(TEXT("il refresh dal vivo, da solo, ridisegna la linea delle celle viste"),
		HexMap->GetVisionBorder().Num() > 0 && HexMap->GetVisionBorder() == Atteso());

	HexMap->SetVisionBorder(TArray<FRTCellId>());
	TM->OnPlaybackStepAdvanced.Broadcast();
	const TArray<FRTExposedEdge> AttesoPlayback = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(
		HexMap->MapAsset, TM->PlaybackKnowledgeForTeam(Presenter->ViewerTeamId()).VisibleCells);
	TestTrue(TEXT("premessa: nel playback la squadra vede qualcosa"), AttesoPlayback.Num() > 0);
	TestTrue(TEXT("il passo di playback, da solo, ridisegna la linea con la conoscenza del playback"),
		HexMap->GetVisionBorder().Num() > 0 && HexMap->GetVisionBorder() == AttesoPlayback);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
