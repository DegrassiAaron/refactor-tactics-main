// #1287 — sulla mappa d'autore i bot si fermano a due celle e non sparano.
//
// Il test di ingaggio esistente, `Match.Autobattle.EngagesOnTheShippedMapSource`, imposta
// `MapSource = GeneratedTestArena`: misura l'arena generata, non la mappa che la partita carica. Dopo
// #1069 il `MapSource` spedito e' `LevelAsset`, quindi quel test guarda una configurazione che non e'
// piu' quella spedita — ed e' il buco da cui questo difetto e' passato.
//
// Misurato prima del fix, 12 turni su `L_HexArena`: 42 voci `Stayed` su 48, zero `Combat`, unita' ferme
// nove e dieci turni sulla stessa cella. La mappa ha un ostacolo al centro che blocca vista e passo, e il
// punteggio del bot misura la distanza in LINEA D'ARIA: a due celle era gia' al minimo, e aggirare
// l'ostacolo avrebbe solo peggiorato il punteggio.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTAuthoredEngagement
{
	TArray<ARTUnit*> LiveUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);
		TArray<ARTUnit*> Units;
		for (AActor* A : Found)
		{
			ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->IsAlive()) { Units.Add(U); }
		}
		return Units;
	}
}

/**
 * **Sulla mappa che si gioca, nessuno si parcheggia.**
 *
 * Stesso oracolo di #1088 e di `D-184` — nessuna unita' viva ferma sulla stessa cella oltre
 * `RoundLimit / 3` — applicato alla mappa d'autore invece che all'arena generata. Il pareggio allo
 * scadere resta un esito legittimo: cio' che non lo e' e' stare fermi.
 *
 * ⚠️ **E' anche il rilevatore di OSCILLAZIONE.** Un bot che alternasse fra «cerca» e «avvicinati»
 * tornerebbe sulle stesse celle: la sequenza per unita' lo mostra, e una singola chiamata a `PlanUnit`
 * no. E' la ragione per cui questo test fa girare una partita invece di interrogare la libreria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapNobodyParksTest,
	"RefactorTactics.Match.Autobattle.NobodyParksOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapNobodyParksTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored = Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// La mappa D'AUTORE, non una generata: e' il punto del test.
	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	TArray<ARTUnit*> Units = RTAuthoredEngagement::LiveUnits(World);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Sequenza di celle per unita': la memoria che rende visibile sia il parcheggio sia l'oscillazione.
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, int32> Ferma;
	TMap<int32, int32> PiuLunga;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			const int32 Id = U->GetUniqueID();
			const FRTCellId* Prev = Ultima.Find(Id);
			if (Prev && *Prev == U->Cell) { Ferma.FindOrAdd(Id) += 1; }
			else { Ferma.FindOrAdd(Id) = 0; }
			int32& Record = PiuLunga.FindOrAdd(Id);
			Record = FMath::Max(Record, Ferma[Id]);
			Ultima.FindOrAdd(Id) = U->Cell;
		}
	}

	int32 Peggiore = 0;
	for (const TPair<int32, int32>& P : PiuLunga) { Peggiore = FMath::Max(Peggiore, P.Value); }

	// `RoundLimit / 3`: la soglia di D-184, la stessa che l'oracolo di #1088 usa sull'arena di prova.
	const int32 Limite = FMath::Max(1, MaxTurni / 3);
	AddInfo(FString::Printf(TEXT("turni giocati: %d  |  piu' lunga sequenza ferma: %d (limite %d)"),
		Turni, Peggiore, Limite));

	TestTrue(*FString::Printf(
		TEXT("nessuna unita' si parcheggia sulla mappa d'autore: piu' lunga sequenza ferma %d turni (limite %d)"),
		Peggiore, Limite), Peggiore <= Limite);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
