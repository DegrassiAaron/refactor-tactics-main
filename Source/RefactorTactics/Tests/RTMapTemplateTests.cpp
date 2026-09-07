// Le regole di ALLESTIMENTO di un livello tattico — `URTMapTemplateLibrary`.
//
// 🔑 **Ogni test ha la sua controprova**, con la disciplina di `RTHexMapValidationTests.cpp`: per ogni caso
// invalido c'e' il caso valido piu' vicino, che deve restare in silenzio. Un validator che segnala sempre
// passerebbe meta' di questi test, ed e' il difetto piu' facile da scrivere in questo dominio.
//
// ⚠️ **Nessun `UWorld`, nessun attore, nessuna presentazione.** Le regole ricevono un conteggio e dei marker
// gia' risolti: e' cio' che le rende esercitabili headless. L'unico pezzo che tocca il mondo —
// `CollectFromWorld` — non decide niente, e quindi non ha regole da verificare.

#include "Misc/AutomationTest.h"

#include "Algo/Reverse.h"
#include "Engine/World.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapTemplateLibrary.h"
#include "Map/RTSpawnPoint.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Il prefisso di dominio non e' stile: e' cio' che tiene in piedi l'unity build. Due funzioni omonime
	// in namespace anonimi diversi sopravvivono al linker ma non al compilatore quando il raggruppamento le
	// mette nella stessa unita' (`#2271`).
	constexpr float MapTemplateHexSize = 100.f;
	constexpr float MapTemplateLayerHeight = 250.f;
	const FVector MapTemplateOrigin = FVector::ZeroVector;

	/** Un'arena piatta di raggio 3 sul layer 0, con la scala fissata. */
	URTHexMapAsset* MapTemplateMakeArena()
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
		if (Map)
		{
			Map->HexSize = MapTemplateHexSize;
			Map->LayerHeight = MapTemplateLayerHeight;
		}
		return Map;
	}

	/** Un marker gia' risolto, per i test che non passano dalla geometria. */
	FRTSpawnPlacement MapTemplateSpawn(int32 TeamId, int32 SlotIndex, const TCHAR* Label, bool bResolved = true)
	{
		FRTSpawnPlacement Spawn;
		Spawn.TeamId = TeamId;
		Spawn.SlotIndex = SlotIndex;
		Spawn.Label = Label;
		Spawn.bResolved = bResolved;
		return Spawn;
	}

	/** Quante segnalazioni di quel tipo. */
	int32 MapTemplateCount(const TArray<FRTMapTemplateIssue>& Issues, ERTMapTemplateIssue Reason)
	{
		int32 Count = 0;
		for (const FRTMapTemplateIssue& Issue : Issues)
		{
			if (Issue.Reason == Reason)
			{
				++Count;
			}
		}
		return Count;
	}

	/** I quattro marker del template: due squadre, due posti ciascuna. */
	TArray<FRTSpawnPlacement> MapTemplateFourSpawns()
	{
		return {
			MapTemplateSpawn(0, 0, TEXT("Spawn_T0_S0")),
			MapTemplateSpawn(0, 1, TEXT("Spawn_T0_S1")),
			MapTemplateSpawn(1, 0, TEXT("Spawn_T1_S0")),
			MapTemplateSpawn(1, 1, TEXT("Spawn_T1_S1")),
		};
	}
}

/**
 * LA CONTROPROVA GENERALE: un template allestito bene non dice niente.
 *
 * Senza questa, ogni altro test qui sotto passerebbe anche con un validator che segnala sempre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateValidIsSilentTest,
	"RefactorTactics.MapTemplate.ValidTemplateIsSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateValidIsSilentTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapTemplateMakeArena();
	TestNotNull(TEXT("L'arena di prova esiste"), Map);

	TArray<FRTMapTemplateIssue> Issues;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, MapTemplateFourSpawns(), Issues);

	TestEqual(TEXT("Un template completo non produce segnalazioni"), Issues.Num(), 0);
	return true;
}

/**
 * Due attori mappa: quale origine valga lo deciderebbe l'ordine di iterazione, cioe' nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateDuplicateMapActorTest,
	"RefactorTactics.MapTemplate.DuplicateMapActorIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateDuplicateMapActorTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapTemplateMakeArena();

	TArray<FRTMapTemplateIssue> Duplicated;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 2, Map, MapTemplateFourSpawns(), Duplicated);
	TestEqual(TEXT("Due attori mappa producono una segnalazione"),
		MapTemplateCount(Duplicated, ERTMapTemplateIssue::DuplicateMapActor), 1);

	// La controprova: uno solo tace.
	TArray<FRTMapTemplateIssue> Single;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, MapTemplateFourSpawns(), Single);
	TestEqual(TEXT("Un solo attore mappa non produce la segnalazione"),
		MapTemplateCount(Single, ERTMapTemplateIssue::DuplicateMapActor), 0);

	return true;
}

/**
 * L'asset assente si segnala UNA volta, e non si somma all'attore assente.
 *
 * ⚠️ La seconda meta' e' la parte che vale: senza attore mappa l'asset manca per conseguenza, e due
 * segnalazioni per una causa sola manderebbero chi legge a cercare due difetti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateMissingAssetTest,
	"RefactorTactics.MapTemplate.MissingMapAssetIsReportedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateMissingAssetTest::RunTest(const FString&)
{
	TArray<FRTMapTemplateIssue> WithActor;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, nullptr, MapTemplateFourSpawns(), WithActor);
	TestEqual(TEXT("L'attore senza asset si segnala"),
		MapTemplateCount(WithActor, ERTMapTemplateIssue::MissingMapAsset), 1);

	TArray<FRTMapTemplateIssue> WithoutActor;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 0, nullptr, MapTemplateFourSpawns(), WithoutActor);
	TestEqual(TEXT("Senza attore mappa la segnalazione e' quella dell'attore"),
		MapTemplateCount(WithoutActor, ERTMapTemplateIssue::MissingMapActor), 1);
	TestEqual(TEXT("...e l'asset assente NON si somma come secondo difetto"),
		MapTemplateCount(WithoutActor, ERTMapTemplateIssue::MissingMapAsset), 0);

	// La controprova: con attore e asset, nessuna delle due.
	URTHexMapAsset* Map = MapTemplateMakeArena();
	TArray<FRTMapTemplateIssue> Complete;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, MapTemplateFourSpawns(), Complete);
	TestEqual(TEXT("Attore e asset presenti non producono segnalazioni"), Complete.Num(), 0);

	return true;
}

/**
 * Due marker sulla stessa coppia `(Team, Slot)`: due posti per la stessa unita'.
 *
 * ⚠️ **Le segnalazioni sono DUE, una per marker**, e non una per gruppo: chi legge il Map Check clicca
 * sull'attore, e un solo messaggio lascerebbe muto uno dei due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateDuplicateSlotTest,
	"RefactorTactics.MapTemplate.DuplicateSpawnSlotIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateDuplicateSlotTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapTemplateMakeArena();

	TArray<FRTSpawnPlacement> Clashing = {
		MapTemplateSpawn(0, 0, TEXT("Spawn_A")),
		MapTemplateSpawn(0, 0, TEXT("Spawn_B")),
		MapTemplateSpawn(1, 0, TEXT("Spawn_C")),
	};

	TArray<FRTMapTemplateIssue> Issues;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, Clashing, Issues);
	TestEqual(TEXT("Il gruppo duplicato segnala entrambi i marker"),
		MapTemplateCount(Issues, ERTMapTemplateIssue::DuplicateSpawnSlot), 2);

	// La controprova PIU' VICINA: stessa squadra, slot diverso. Un validator che confrontasse solo il team
	// segnalerebbe anche questo.
	TArray<FRTSpawnPlacement> SameTeam = {
		MapTemplateSpawn(0, 0, TEXT("Spawn_A")),
		MapTemplateSpawn(0, 1, TEXT("Spawn_B")),
	};
	TArray<FRTMapTemplateIssue> SameTeamIssues;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, SameTeam, SameTeamIssues);
	TestEqual(TEXT("Stessa squadra su slot diversi non e' un duplicato"), SameTeamIssues.Num(), 0);

	// E la simmetrica: stesso slot, squadra diversa.
	TArray<FRTSpawnPlacement> SameSlot = {
		MapTemplateSpawn(0, 0, TEXT("Spawn_A")),
		MapTemplateSpawn(1, 0, TEXT("Spawn_B")),
	};
	TArray<FRTMapTemplateIssue> SameSlotIssues;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, SameSlot, SameSlotIssues);
	TestEqual(TEXT("Stesso slot in squadre diverse non e' un duplicato"), SameSlotIssues.Num(), 0);

	return true;
}

/**
 * LA DERIVAZIONE: la cella si ricava dalla POSIZIONE, e una posizione fuori dal tabellone non ne ha una.
 *
 * 🔑 **E' il test che tiene in piedi la scelta di non avere un `FRTCellId` editabile su `ARTSpawnPoint`.**
 * Il round-trip verifica che il marker posato sul centro di una cella risolva a QUELLA cella — cioe' che la
 * conversione passi davvero da `URTHexLibrary` e non da un arrotondamento riscritto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateSpawnOffMapTest,
	"RefactorTactics.MapTemplate.SpawnResolvesToTheCellItStandsOn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateSpawnOffMapTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapTemplateMakeArena();
	TestNotNull(TEXT("L'arena di prova esiste"), Map);

	// Il centro-mondo di una cella che l'arena contiene di sicuro (raggio 3).
	const FRTCellId Inside(2, -1, 0);
	TestNotNull(TEXT("La cella scelta appartiene all'arena"), Map->FindCell(Inside));

	const FVector InsideWorld = URTHexLibrary::AxialToWorld(Inside, MapTemplateOrigin,
		MapTemplateHexSize, MapTemplateLayerHeight);

	FRTCellId Resolved;
	const bool bInside = URTMapTemplateLibrary::ResolveWorldToCell(Map, MapTemplateOrigin,
		MapTemplateHexSize, MapTemplateLayerHeight, InsideWorld, Resolved);
	TestTrue(TEXT("Un marker sul centro di una cella risolve"), bInside);
	TestTrue(TEXT("...e risolve proprio a QUELLA cella"), Resolved == Inside);

	// Fuori dal tabellone: la coppia assiale si calcola comunque — l'arrotondamento cubico non fallisce mai —
	// ma l'asset non contiene quella cella. E' la distinzione che questo caso esiste per fissare.
	const FRTCellId Outside(12, -6, 0);
	TestNull(TEXT("La cella lontana NON appartiene all'arena"), Map->FindCell(Outside));
	const FVector OutsideWorld = URTHexLibrary::AxialToWorld(Outside, MapTemplateOrigin,
		MapTemplateHexSize, MapTemplateLayerHeight);

	FRTCellId Unresolved;
	const bool bOutside = URTMapTemplateLibrary::ResolveWorldToCell(Map, MapTemplateOrigin,
		MapTemplateHexSize, MapTemplateLayerHeight, OutsideWorld, Unresolved);
	TestFalse(TEXT("Un marker fuori dal tabellone non risolve"), bOutside);

	// E la regola che ne consegue.
	TArray<FRTSpawnPlacement> Spawns = {
		MapTemplateSpawn(0, 0, TEXT("Spawn_Dentro"), /*bResolved=*/ true),
		MapTemplateSpawn(0, 1, TEXT("Spawn_Fuori"), /*bResolved=*/ false),
	};
	TArray<FRTMapTemplateIssue> Issues;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 1, Map, Spawns, Issues);
	TestEqual(TEXT("Solo il marker fuori mappa si segnala"),
		MapTemplateCount(Issues, ERTMapTemplateIssue::SpawnOffMap), 1);
	TestEqual(TEXT("...ed e' quello"), Issues[0].Label, FString(TEXT("Spawn_Fuori")));

	return true;
}

/**
 * L'elenco non dipende dall'ordine in cui i marker arrivano.
 *
 * ⚠️ **Non e' pedanteria**: i marker arrivano da `TActorIterator`, che non garantisce nessun ordine. Senza
 * l'ordinamento, due esecuzioni sullo stesso livello elencherebbero gli stessi difetti in ordine diverso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateDeterministicTest,
	"RefactorTactics.MapTemplate.IssueListIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateDeterministicTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapTemplateMakeArena();

	TArray<FRTSpawnPlacement> Forward = {
		MapTemplateSpawn(1, 1, TEXT("Spawn_D"), /*bResolved=*/ false),
		MapTemplateSpawn(0, 0, TEXT("Spawn_A")),
		MapTemplateSpawn(0, 0, TEXT("Spawn_B")),
		MapTemplateSpawn(1, 0, TEXT("Spawn_C"), /*bResolved=*/ false),
	};
	TArray<FRTSpawnPlacement> Reversed = Forward;
	Algo::Reverse(Reversed);

	TArray<FRTMapTemplateIssue> FromForward;
	TArray<FRTMapTemplateIssue> FromReversed;
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 2, Map, Forward, FromForward);
	URTMapTemplateLibrary::ValidateTemplate(/*MapActorCount=*/ 2, Map, Reversed, FromReversed);

	TestEqual(TEXT("Lo stesso livello produce lo stesso numero di segnalazioni"),
		FromForward.Num(), FromReversed.Num());

	// ⚠️ Il confronto e' sulla SEQUENZA, non sull'insieme: e' l'ordine il fatto che questo test difende.
	bool bIdentical = FromForward.Num() == FromReversed.Num();
	for (int32 I = 0; bIdentical && I < FromForward.Num(); ++I)
	{
		bIdentical = FromForward[I].Reason == FromReversed[I].Reason
			&& FromForward[I].Label == FromReversed[I].Label
			&& FromForward[I].TeamId == FromReversed[I].TeamId
			&& FromForward[I].SlotIndex == FromReversed[I].SlotIndex;
	}
	TestTrue(TEXT("L'ordine delle segnalazioni non dipende dall'ordine d'ingresso"), bIdentical);

	// Controprova che l'elenco non sia vuoto: un elenco vuoto sarebbe identico a se' stesso e questo test
	// passerebbe senza aver misurato niente.
	TestTrue(TEXT("Il caso di prova produce davvero delle segnalazioni"), FromForward.Num() > 0);

	return true;
}

/**
 * LA RACCOLTA DAL MONDO: `CollectFromWorld` legge davvero gli attori, e li ordina.
 *
 * 🔑 **E' il pezzo che i test qui sopra NON coprivano.** Quelli esercitano le regole su placement
 * costruiti a mano; questo verifica il gradino prima — che dal livello escano i placement giusti — ed e'
 * l'unico posto dove `ARTSpawnPoint::ResolveCell` e `GetHexContext` vengono davvero attraversati.
 *
 * ⚠️ **L'ordine e' la meta' che vale.** `TActorIterator` non garantisce nessun ordine, quindi la sola
 * cosa che rende ripetibile l'elenco e' l'ordinamento esplicito: qui i marker si posano di proposito in
 * ordine inverso a quello atteso, cosi' un ordinamento assente si vede invece di passare per fortuna.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateCollectFromWorldTest,
	"RefactorTactics.MapTemplate.CollectFromWorldReadsAndOrdersTheLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateCollectFromWorldTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!MapActor)
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
	Map->HexSize = MapTemplateHexSize;
	Map->LayerHeight = MapTemplateLayerHeight;
	MapActor->MapAsset = Map;
	MapActor->SetActorLocation(FVector::ZeroVector);

	const FVector Origin = MapActor->GetActorLocation();
	auto PlaceSpawn = [&](int32 TeamId, int32 SlotIndex, const FRTCellId& Cell) -> ARTSpawnPoint*
	{
		const FVector Where = URTHexLibrary::AxialToWorld(Cell, Origin, MapTemplateHexSize, MapTemplateLayerHeight);
		ARTSpawnPoint* Spawn = World->SpawnActor<ARTSpawnPoint>(ARTSpawnPoint::StaticClass(), FTransform(Where));
		if (Spawn)
		{
			Spawn->TeamId = TeamId;
			Spawn->SlotIndex = SlotIndex;
		}
		return Spawn;
	};

	// Posati AL CONTRARIO dell'ordine atteso: (1,0) prima di (0,0).
	PlaceSpawn(/*TeamId=*/ 1, /*SlotIndex=*/ 0, FRTCellId(2, 0, 0));
	PlaceSpawn(/*TeamId=*/ 0, /*SlotIndex=*/ 1, FRTCellId(-2, 1, 0));
	PlaceSpawn(/*TeamId=*/ 0, /*SlotIndex=*/ 0, FRTCellId(-2, 0, 0));
	// Fuori dal tabellone: la coppia assiale si calcola, ma l'arena di raggio 3 non contiene la cella.
	PlaceSpawn(/*TeamId=*/ 1, /*SlotIndex=*/ 1, FRTCellId(9, 0, 0));

	int32 MapActorCount = 0;
	const URTHexMapAsset* Collected = nullptr;
	TArray<FRTSpawnPlacement> Spawns;
	URTMapTemplateLibrary::CollectFromWorld(World, MapActorCount, Collected, Spawns);

	TestEqual(TEXT("Un solo attore mappa nel livello"), MapActorCount, 1);
	TestTrue(TEXT("L'asset raccolto e' quello assegnato"), Collected == Map);
	TestEqual(TEXT("Quattro marker raccolti"), Spawns.Num(), 4);

	if (Spawns.Num() == 4)
	{
		// L'ordine atteso e' (TeamId, SlotIndex), non quello di posa.
		TestEqual(TEXT("Primo: Team 0 Slot 0"), Spawns[0].TeamId * 10 + Spawns[0].SlotIndex, 0);
		TestEqual(TEXT("Secondo: Team 0 Slot 1"), Spawns[1].TeamId * 10 + Spawns[1].SlotIndex, 1);
		TestEqual(TEXT("Terzo: Team 1 Slot 0"), Spawns[2].TeamId * 10 + Spawns[2].SlotIndex, 10);
		TestEqual(TEXT("Quarto: Team 1 Slot 1"), Spawns[3].TeamId * 10 + Spawns[3].SlotIndex, 11);

		// La cella DERIVATA dalla posizione, non un valore scritto a mano da qualche parte.
		TestTrue(TEXT("Il marker su (-2,0) risolve"), Spawns[0].bResolved);
		TestTrue(TEXT("...e risolve proprio a quella cella"), Spawns[0].Cell == FRTCellId(-2, 0, 0));
		TestTrue(TEXT("Il marker su (2,0) risolve"), Spawns[2].bResolved);
		TestTrue(TEXT("...e risolve proprio a quella cella"), Spawns[2].Cell == FRTCellId(2, 0, 0));

		// E il marker fuori mappa non risolve: e' la distinzione fra «coordinata calcolabile» e «cella
		// che esiste», la stessa che `SpawnResolvesToTheCellItStandsOn` fissa senza mondo.
		TestFalse(TEXT("Il marker fuori dal tabellone non risolve"), Spawns[3].bResolved);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
