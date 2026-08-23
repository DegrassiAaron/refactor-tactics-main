// #1069 criterio 4 — un test che passa dalla CONFIGURAZIONE SPEDITA.
//
// E' l'anello che mancava a tutta la storia di questa issue. `Heroes.SpawnFromData` e
// `HexMatch.PlaysToCompletionWithoutInput` sono verdi perche' costruiscono il roster **in codice** e non
// passano mai dal `BP_GameMode`: il commit `2874cf3b` ha potuto infilare due override nel Blueprint
// spedito — un `MatchFormat` verso un asset mai esistito in git e un `MapSource` che scavalca la mappa
// del livello — senza far cadere niente.
//
// ⚠️ **Cio' che questo file NON puo' vedere, e va detto**: il riferimento penzolante. Un puntatore che
// non risolve diventa `nullptr`, e il GameMode ripiega sul formato spedito senza distinguere «non
// assegnato» da «assegnato a un fantasma». Quella meta' la copre `tools/asset-refs/check.ts`, che guarda
// i byte del package invece del valore risolto — ed e' la ragione per cui il criterio 5 esiste separato.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Frontend/RTStartupReport.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTShippedGameMode
{
	const TCHAR* const ClassPath = TEXT("/Game/RT/Core/Framework/BP_GameMode.BP_GameMode_C");

	/** Una mappa esagonale piena: abbastanza celle percorribili per le quattro posizioni di partenza. */
	ARTHexMapActor* SpawnMap(UWorld* World)
	{
		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
		{
			Asset->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Asset->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = Asset;
		return Actor;
	}
}

/**
 * **Il `BP_GameMode` spedito allestisce la partita che dichiara.**
 *
 * Non «una partita»: *quella* partita. Le tre asserzioni sono le tre cose che un giocatore ottiene
 * premendo Play, e ognuna e' gia' stata falsa almeno una volta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShippedGameModeSetsUpAdvertisedMatchTest,
	"RefactorTactics.Startup.ShippedGameModeSetsUpTheAdvertisedMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShippedGameModeSetsUpAdvertisedMatchTest::RunTest(const FString&)
{
	// ⚠️ **`-1` = «ignora», e la scelta ha un precedente costoso.** Il Blueprint spedito porta oggi un
	// riferimento a un package assente, e il caricamento lo dichiara. Asserire la PRESENZA di quel rumore
	// legherebbe il test al difetto invece che alla regola, e lo farebbe cadere il giorno in cui l'asset
	// viene ripulito: e' esattamente l'errore che `ExpectMissingFrontendAssets` sta facendo adesso, dove
	// tre test pretendono un warning che il merge dei `.uasset` ha reso impossibile.
	AddExpectedMessage(TEXT("Failed to find object"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ -1);
	AddExpectedMessage(TEXT("was not available"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ -1);

	UClass* Shipped = LoadClass<ARTGameMode>(nullptr, RTShippedGameMode::ClassPath);
	if (!TestNotNull(*FString::Printf(TEXT("la classe spedita '%s' si carica"), RTShippedGameMode::ClassPath),
		Shipped))
	{
		return false;
	}

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = RTShippedGameMode::SpawnMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>(Shipped);
	if (!TestNotNull(TEXT("il GameMode spedito"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->SetupHexMatch(HexMap);

	// 1. **Le formazioni spedite risolvono tutte.** E' la meta' che il criterio 1 della issue ha gia'
	//    consuntivato sul log («4 eroi»), qui pinnata: un `Team*Heroes` che tornasse a nominare un eroe
	//    uscito col rename di D-130 fermerebbe l'allestimento, e questo test lo direbbe prima del PIE.
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);
	TestEqual(TEXT("quattro unita' in campo dalla configurazione spedita"), Found.Num(), 4);

	const FRTStartupReport& Report = GameMode->GetStartupReport();

	// 2. **Nessun fatale, e l'allestimento arriva in fondo.** Su una macchina che HA il formato invalido
	//    sul disco questa riga cade con `FormatAssetInvalid` — cioe' il test riproduce il «works on my
	//    machine» rovesciato che la issue descrive, invece di dipendere da chi lo esegue.
	TestEqual(TEXT("nessun esito fatale"),
		URTStartupReportLibrary::FindFatal(Report), ERTStartupOutcome::Ok);
	TestEqual(TEXT("l'allestimento arriva a Ready"), Report.Phase, ERTLoadPhase::Ready);

	// 3. **La mappa del livello non viene scavalcata da un'arena generata.** E' l'override che #1069
	//    contesta: `rt.Arena.Check` misura che `GeneratedTestArena` soddisfa **1 dei 3** criteri di U1
	//    (una cella bloccante invece di due, rotte a esposizione 57%/56%), e G13 dichiara quella riserva.
	bool bArenaGenerata = false;
	for (const FRTStartupNote& Note : Report.Notes)
	{
		if (Note.Outcome == ERTStartupOutcome::UsingTestArena
			|| Note.Outcome == ERTStartupOutcome::UsingDemoArena)
		{
			bArenaGenerata = true;
		}
	}
	TestFalse(TEXT("il GameMode spedito non sostituisce la mappa del livello con un'arena generata"),
		bArenaGenerata);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
