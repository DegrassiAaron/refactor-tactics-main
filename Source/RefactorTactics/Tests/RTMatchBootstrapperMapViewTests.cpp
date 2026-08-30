#include "Misc/AutomationTest.h"

#include "Engine/World.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapSource.h"
#include "RTGameMode.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Dopo l'allestimento la VISTA della mappa segue l'asset, su tutti i rami di `ApplyMapSource`.
 *
 * **Perche' esiste.** #1665, primo dei due difetti. `ApplyMapSource` ha cinque rami che assegnano
 * `MapAsset`; quattro chiamavano `RebuildInstances()` subito dopo, **il quinto no** — quello della mappa
 * d'autore, che ne fa una COPIA di lavoro con `DuplicateObject` (CP 8.4). L'attore continuava a puntare la
 * vista al vecchio asset mentre `MapAsset` era ormai un altro oggetto. Nel pacchetto la sonda misurava
 * silenzio totale: **zero istanze di cella**, cioe' board assente.
 *
 * ⚠️ **Le due reti che coprirebbero il buco valgono solo in Editor**: `BindToMapAsset()` vive dentro
 * `#if WITH_EDITOR`, e `OnConstruction` dichiara di coprire il caso `SpawnActor` — l'attore **spawnato**,
 * non uno **piazzato** in un livello cotto.
 *
 * 🔑 **Perche' questo test morde anche in Editor**, dove le reti ci sono: l'asset viene assegnato
 * dall'esterno DOPO lo spawn, e assegnare `MapAsset` non emette `OnMapChanged` — quindi il delegate non
 * scatta. L'unica cosa che allinea la vista all'asset e' la chiamata dentro `ApplyMapSource`. ✅ **Verificato
 * per mutazione**: togliendo quella riga il test diventa rosso, con `61 istanze contro 19 attese`. E' il
 * contrario di quanto era accaduto con `HexMapActor.ProceduralMeshesAreRenderable`, dove la mutazione
 * restava verde perche' un fallback dell'Editor mascherava il difetto — la mutazione va fatta, non
 * supposta.
 *
 * **Cosa NON copre.** Che le istanze si VEDANO: quello dipende dal materiale e dallo slot della mesh, ed e'
 * il secondo difetto di #1665 — presidiato per la parte strutturale da
 * `HexMapActor.ProceduralMeshesAreRenderable`. Qui si prova solo che esistano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperRebuildsViewOnAuthoredMapTest,
	"RefactorTactics.Match.Bootstrapper.AuthoredMapRebuildsTheView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperRebuildsViewOnAuthoredMapTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Una mappa d'autore minima, assegnata DOPO lo spawn: e' il caso reale — l'attore e' piazzato nel
	// livello col suo asset, non lo costruisce lui.
	URTHexMapAsset* Authored = NewObject<URTHexMapAsset>(HexMap);
	const TArray<FRTCellId> Ids = URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 2);
	for (const FRTCellId& Id : Ids)
	{
		Authored->AddOrUpdateCell(FRTHexCellData(Id));
	}
	HexMap->MapAsset = Authored;

	const int32 CelleAttese = Authored->NumCells();
	if (!TestTrue(TEXT("la mappa d'autore ha celle"), CelleAttese > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->SetupHexMatch(HexMap);

	// 1. L'asset e' stato DUPLICATO: la copia di lavoro di CP 8.4, non l'originale. Se questo asserto
	//    cadesse, la partita starebbe modificando il contenuto del progetto.
	TestNotEqual(TEXT("l'allestimento lavora su una COPIA della mappa d'autore"),
		HexMap->MapAsset.Get(), Authored);
	TestEqual(TEXT("la copia ha le stesse celle dell'originale"),
		HexMap->MapAsset ? HexMap->MapAsset->NumCells() : -1, CelleAttese);

	// 2. E la VISTA segue la copia. E' l'asserto che #1665 ha pagato.
	//
	//    🔑 **Verificato per mutazione**: togliendo `RebuildInstances()` dopo la duplicazione, questo test
	//    misura **61 istanze contro 19 attese** — cioe' la vista di un'ALTRA mappa (61 = `HexArea(4)`, il
	//    `DemoArenaRadius` di default), non della mappa appena allestita. ⚠️ Non e' il sintomo che la sonda
	//    misurava nel pacchetto, dove le istanze erano **zero**: li' non c'era nulla da mostrare, qui c'e'
	//    qualcosa di sbagliato. Sono due facce dello stesso difetto — la vista non segue l'asset — e il
	//    numero cambia con cio' che la vista mostrava prima.
	//
	//    Si chiede a `NumInstanceCells()`, l'accessore pubblico che il proprio commento dichiara
	//    «diagnostica e test»: il componente `Cells` e' `protected`, e un test non deve forzare
	//    l'incapsulamento per misurare cio' che l'attore gia' espone.
	TestEqual(*FString::Printf(
		TEXT("un'istanza per cella dopo l'allestimento (attese %d, trovate %d)"),
		CelleAttese, HexMap->NumInstanceCells()),
		HexMap->NumInstanceCells(), CelleAttese);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
