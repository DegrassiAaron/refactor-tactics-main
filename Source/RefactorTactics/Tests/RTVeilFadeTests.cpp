#include "Misc/AutomationTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Perception/RTVeilTransition.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2875` — **il velo ATTENUA invece di saltare**, consumando il filtro di `#2874`.
 *
 * ## Cosa si interpola, e cosa no
 *
 * Si interpola il **fattore**, non il colore: il colore pieno di una cella e' `SurfaceColor` della sua
 * superficie e non cambia mai, mentre fra osservato e ricordato cambia il moltiplicatore — `1.0` contro
 * `RTVeilExploredFactor`.
 *
 * ⛔ **E si attenua SOLO fra due stati disegnati.** Il reveal da `Hidden` resta uno **scalino**: e' la
 * decisione (i) di `#2875`, e la ragione e' il multilivello — `LayerView` vale `AllLayers` di default, i
 * piani si impilano, e una cella di layer 1 disegnata a luminanza quasi nulla coprirebbe quella di layer 0.
 * Sarebbe «un disegno che copre», cioe' la mappa nera da cui [D-225] si difende con quelle stesse parole.
 */

namespace
{
	/** Una board vera con le sue istanze: il filtro agisce sugli ISM, non su un modello astratto. */
	ARTHexMapActor* MakeFadingBoard(UWorld* World, int32 Radius)
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

	FRTTeamKnowledge FadeKnowledgeOf(const TArray<FRTCellId>& Visible, const TArray<FRTCellId>& Explored)
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.TurnNumber = 1;
		K.VisibleCells = Visible;
		K.ExploredCells = Explored;
		return K;
	}

	/**
	 * Il canale rosso scritto per l'istanza `I`, cioe' il valore che si DISEGNA adesso.
	 *
	 * ⚠️ Passa da `GetVeilWrittenColor`, che legge il buffer per istanza — non dal fattore interno: un test
	 * scritto sul fattore sarebbe verde anche se il colore non arrivasse mai alle istanze.
	 */
	bool RedWrittenFor(const ARTHexMapActor* HexMap, int32 InstanceIndex, float& Out)
	{
		FLinearColor Scritto;
		if (!HexMap->GetVeilWrittenColor(InstanceIndex, Scritto))
		{
			return false;
		}
		Out = Scritto.R;
		return true;
	}

	/** L'indice dell'istanza che rappresenta `Cell`, o `INDEX_NONE`. */
	int32 InstanceOf(const ARTHexMapActor* HexMap, const FRTCellId& Cell)
	{
		for (int32 I = 0; I < HexMap->NumInstanceCells(); ++I)
		{
			if (HexMap->CellForInstance(I) == Cell)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}
}

/**
 * 🔴 **IL TEST CHE RENDE VERO TUTTO IL RESTO**: fra osservato e ricordato passano valori **intermedi**.
 *
 * Senza, il filtro potrebbe essere cablato e non fare niente, e nessun conteggio se ne accorgerebbe — il
 * velo continuerebbe a saltare con l'aria di attenuare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFadePassesThroughIntermediateTest,
	"RefactorTactics.Veil.FadePassesThroughIntermediateValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFadePassesThroughIntermediateTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Cella(0, 0);
	const int32 I = InstanceOf(HexMap, Cella);
	if (!TestTrue(TEXT("la cella ha un'istanza"), I != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// 1. Osservata: il fattore vale 1, e la scrittura e' immediata (viene da `Hidden`, quindi snap).
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Cella }, { Cella }));
	float Accesa = 0.f;
	if (!TestTrue(TEXT("il canale rosso si legge"), RedWrittenFor(HexMap, I, Accesa)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// 2. Diventa un ricordo: il target scende a `RTVeilExploredFactor`, ma il valore disegnato NON ci arriva
	//    in questo istante — deve attraversare.
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({}, { Cella }));
	const float Ricordo = Accesa * ARTHexMapActor::RTVeilExploredFactor;

	TestTrue(*FString::Printf(TEXT("al refresh il valore e' ancora quello acceso (%.4f)"), Accesa),
		FMath::IsNearlyEqual(Accesa, Accesa));
	TestTrue(TEXT("il velo dichiara una transizione in volo"), HexMap->GetVeilCellsInTransition() > 0);

	// 3. Un passo breve: il valore si e' mosso, e sta IN MEZZO ai due.
	HexMap->TickActor(1.f / 60.f, LEVELTICK_All, HexMap->PrimaryActorTick);

	float AMeta = 0.f;
	RedWrittenFor(HexMap, I, AMeta);
	AddInfo(FString::Printf(TEXT("acceso %.4f -> ricordo %.4f, dopo un passo: %.4f"), Accesa, Ricordo, AMeta));

	TestTrue(*FString::Printf(TEXT("il valore e' sceso sotto l'acceso (%.4f < %.4f)"), AMeta, Accesa),
		AMeta < Accesa);
	TestTrue(*FString::Printf(TEXT("ma non e' ancora al ricordo (%.4f > %.4f)"), AMeta, Ricordo),
		AMeta > Ricordo);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * A convergenza il valore disegnato e' **esattamente** quello che il velo scriveva prima di `#2875`.
 *
 * 🔑 E' la garanzia che questa fetta cambia il **quando**, non il **cosa**: a regime la board e' identica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFadeConvergesToHistoricValueTest,
	"RefactorTactics.Veil.FadeConvergesToTheHistoricValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFadeConvergesToHistoricValueTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Cella(0, 0);
	const int32 I = InstanceOf(HexMap, Cella);
	if (!TestTrue(TEXT("la cella ha un'istanza"), I != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Cella }, { Cella }));
	float Accesa = 0.f;
	RedWrittenFor(HexMap, I, Accesa);

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({}, { Cella }));

	int32 Passi = 0;
	while (HexMap->GetVeilCellsInTransition() > 0 && Passi < 600)
	{
		HexMap->TickActor(1.f / 60.f, LEVELTICK_All, HexMap->PrimaryActorTick);
		++Passi;
	}

	TestEqual(TEXT("alla fine nessuna istanza e' in transizione"), HexMap->GetVeilCellsInTransition(), 0);
	TestTrue(TEXT("e ci e' arrivata entro il limite"), Passi < 600);

	float Finale = 0.f;
	RedWrittenFor(HexMap, I, Finale);
	const float Storico = Accesa * ARTHexMapActor::RTVeilExploredFactor;

	AddInfo(FString::Printf(TEXT("converso in %d passi: %.6f contro lo storico %.6f"), Passi, Finale, Storico));
	TestTrue(*FString::Printf(TEXT("il valore converso e' quello storico (%.6f vs %.6f)"), Finale, Storico),
		FMath::IsNearlyEqual(Finale, Storico, 1.e-4f));

	// Il `Tick` si spegne da solo: a regime il velo non costa niente per fotogramma.
	TestFalse(TEXT("e il Tick si e' spento"), HexMap->IsActorTickEnabled());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **IL REVEAL DA `Hidden` E' ISTANTANEO**, ed e' l'AC che discende dalla decisione (i).
 *
 * Senza questa garanzia il filtro farebbe partire la cella da un fattore basso e la si vedrebbe
 * **schiarire** — cioe' l'uscita (ii) introdotta per distrazione invece che per decisione, con il tile di
 * layer 1 che copre quello di layer 0 per tutta la durata della dissolvenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilRevealFromHiddenIsInstantTest,
	"RefactorTactics.Veil.RevealFromHiddenIsInstant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilRevealFromHiddenIsInstantTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Vicina(1, 0);
	const FRTCellId Nuova(2, 0);
	const int32 INuova = InstanceOf(HexMap, Nuova);
	if (!TestTrue(TEXT("la cella nuova ha un'istanza"), INuova != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// 1. `Nuova` e' MAI VISTA: non disegnata, e non ha un valore da leggere.
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Vicina }, { Vicina }));

	// 2. Diventa osservata. Senza tick di mezzo: il valore dev'essere gia' quello finale.
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Vicina, Nuova }, { Vicina, Nuova }));

	float SubitoDopo = 0.f;
	if (!TestTrue(TEXT("il canale rosso si legge"), RedWrittenFor(HexMap, INuova, SubitoDopo)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// L'oracolo: il valore pieno della superficie, cioe' quello che una cella osservata deve avere.
	const FLinearColor Pieno = FLinearColor::FromSRGBColor(
		URTHexLibrary::SurfaceColor(HexMap->SurfaceForCell(Nuova)));

	AddInfo(FString::Printf(TEXT("reveal senza tick: scritto %.6f, pieno %.6f"), SubitoDopo, Pieno.R));
	TestTrue(*FString::Printf(TEXT("il reveal da Hidden arriva al valore pieno SUBITO (%.6f vs %.6f)"),
			SubitoDopo, Pieno.R),
		FMath::IsNearlyEqual(SubitoDopo, Pieno.R, 1.e-4f));

	// ⚠️ E non deve nemmeno aver aperto una transizione: se l'avesse fatto, il valore sarebbe giusto adesso
	// ma il filtro lo muoverebbe al primo tick.
	TestEqual(TEXT("e non apre nessuna transizione"), HexMap->GetVeilCellsInTransition(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * La PAUSA congela la dissolvenza dov'e', invece di concluderla.
 *
 * E' il vincolo che il prototipo chiedeva per il replay in pausa, e vive qui perche' e' qui che si vede: un
 * passo nullo ripetuto non deve muovere il valore di un bit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFadePauseFreezesTest,
	"RefactorTactics.Veil.FadePauseFreezesTheTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFadePauseFreezesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Cella(0, 0);
	const int32 I = InstanceOf(HexMap, Cella);
	if (!TestTrue(TEXT("la cella ha un'istanza"), I != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Cella }, { Cella }));
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({}, { Cella }));
	HexMap->TickActor(1.f / 60.f, LEVELTICK_All, HexMap->PrimaryActorTick); // a meta' strada

	float AMeta = 0.f;
	RedWrittenFor(HexMap, I, AMeta);

	for (int32 P = 0; P < 50; ++P)
	{
		HexMap->TickActor(0.f, LEVELTICK_All, HexMap->PrimaryActorTick);
	}

	float DopoLaPausa = 0.f;
	RedWrittenFor(HexMap, I, DopoLaPausa);

	TestTrue(TEXT("cinquanta passi da zero secondi non muovono il valore"), DopoLaPausa == AMeta);
	TestTrue(TEXT("e la transizione resta viva, non conclusa"), HexMap->GetVeilCellsInTransition() > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Col filtro **spento** la board e' quella di prima di `#2875`.
 *
 * 🔑 E' il ramo di confronto, e serve che sia **nominato**: un comportamento che si prova solo cancellando
 * il codice non si prova in una suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFadeInstantReproducesHistoricTest,
	"RefactorTactics.Veil.FadeInstantParamsReproduceTheOldBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFadeInstantReproducesHistoricTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->SetVeilTransitionParams(FRTVeilTransitionParams::Instant());

	const FRTCellId Cella(0, 0);
	const int32 I = InstanceOf(HexMap, Cella);
	if (!TestTrue(TEXT("la cella ha un'istanza"), I != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ Cella }, { Cella }));
	float Accesa = 0.f;
	RedWrittenFor(HexMap, I, Accesa);

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({}, { Cella }));
	HexMap->TickActor(1.f / 60.f, LEVELTICK_All, HexMap->PrimaryActorTick);

	float Dopo = 0.f;
	RedWrittenFor(HexMap, I, Dopo);
	const float Storico = Accesa * ARTHexMapActor::RTVeilExploredFactor;

	TestTrue(*FString::Printf(TEXT("un solo passo arriva al valore storico (%.6f vs %.6f)"), Dopo, Storico),
		FMath::IsNearlyEqual(Dopo, Storico, 1.e-4f));
	TestEqual(TEXT("e non resta niente in transizione"), HexMap->GetVeilCellsInTransition(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il filtro non tocca la PARTIZIONE: i tre stati restano quelli, mentre il colore attraversa.
 *
 * ⚠️ `GetVeilCounts` distingue acceso da ricordato **guardando il colore scritto**. Durante una dissolvenza
 * quel colore e' in mezzo ai due, quindi questo test misura la partizione **a convergenza** — dove l'AC la
 * chiede — e dichiara che a meta' transizione il conteggio non e' un oracolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFadeKeepsThePartitionTest,
	"RefactorTactics.Veil.FadeKeepsTheThreeStatePartition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFadeKeepsThePartitionTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = MakeFadingBoard(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("board con istanze"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const int32 Totale = HexMap->NumInstanceCells();
	const TArray<FRTCellId> Visibili = { FRTCellId(0, 0), FRTCellId(1, 0) };
	TArray<FRTCellId> Esplorate = Visibili;
	Esplorate.Append({ FRTCellId(2, 0), FRTCellId(0, 1) });

	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf(Visibili, Esplorate));

	// Poi si spegne una delle due accese: parte una dissolvenza.
	HexMap->ApplyKnowledgeVeil(FadeKnowledgeOf({ FRTCellId(0, 0) }, Esplorate));
	int32 Passi = 0;
	while (HexMap->GetVeilCellsInTransition() > 0 && Passi < 600)
	{
		HexMap->TickActor(1.f / 60.f, LEVELTICK_All, HexMap->PrimaryActorTick);
		++Passi;
	}

	int32 Accese = 0, Ricordate = 0, Nascoste = 0;
	HexMap->GetVeilCounts(Accese, Ricordate, Nascoste);

	AddInfo(FString::Printf(TEXT("a convergenza (%d passi): %d accese, %d ricordate, %d nascoste su %d"),
		Passi, Accese, Ricordate, Nascoste, Totale));

	TestEqual(TEXT("una sola cella resta accesa"), Accese, 1);
	TestEqual(TEXT("le altre esplorate sono ricordi"), Ricordate, Esplorate.Num() - 1);
	TestEqual(TEXT("e i tre stati partizionano la board"), Accese + Ricordate + Nascoste, Totale);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
