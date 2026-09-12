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
#include "Turn/RTMatchSetupLibrary.h"
#include "Engine/World.h"
#include "Frontend/RTStartupReport.h"
#include "Kismet/GameplayStatics.h"
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
		URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

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

/**
 * **Il ritmo dell'auto-run resta corto NELLA CLASSE SPEDITA, non solo nel default di compilazione.**
 *
 * `Scenario.AutoRunPacingHasShortDefault` asserisce la stessa soglia su `SpawnActor<ARTGameMode>()`, cioe' sul
 * default C++ — che nessun override di Blueprint puo' muovere. Ma il gioco gira su `BP_GameMode_C`
 * (`Config/DefaultEngine.ini`, `GlobalDefaultGameMode`), e `ScenarioTurnPauseSeconds` e' `EditAnywhere`: un valore
 * scritto nei Class Defaults viveva fuori da entrambi i gate.
 *
 * ⚠️ **E non lo copre il test qui sopra**, che pure carica la classe giusta: le sue tre asserzioni misurano
 * l'ALLESTIMENTO — quattro unita', nessun fatale, la mappa non sostituita — e questa proprieta' e' un ritmo consumato
 * DOPO il setup. E' la ragione per cui era l'unica del gruppo invisibile a tutti e due.
 *
 * 🔑 **Perche' la soglia conta**: un ritmo alto fa credere di giudicare il gioco mentre si guarda la scena FRA due
 * risoluzioni, dove tutti i piani sono azzerati e nessuno sta pianificando. Il registro PIE ha gia' contato quattro
 * verdetti presi cosi'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShippedGameModePacingStaysShortTest,
	"RefactorTactics.Startup.ShippedGameModePacingStaysShort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShippedGameModePacingStaysShortTest::RunTest(const FString&)
{
	// Stessa ragione del test qui sopra: il Blueprint spedito porta un riferimento a un package assente, e il
	// caricamento lo dichiara. Asserire la presenza di quel rumore legherebbe il test al difetto.
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

	// 🔴 **La guardia che impedisce a questo test di diventare una copia vacua dell'altro.** Se `LoadClass`
	// ripiegasse sulla classe C++ — asset rinominato, path cambiato — ogni asserzione qui sotto misurerebbe il
	// default di compilazione e resterebbe verde per la ragione sbagliata, cioe' esattamente il difetto che
	// questo file esiste per chiudere.
	TestTrue(TEXT("la classe spedita e' una SOTTOCLASSE del GameMode C++, non il GameMode C++"),
		Shipped != ARTGameMode::StaticClass());

	// ⛔ Il CDO della classe CARICATA, non `ARTGameMode::StaticClass()`: e' la distinzione che questo test esiste
	// per fare.
	const ARTGameMode* Cdo = Shipped->GetDefaultObject<ARTGameMode>();
	if (!TestNotNull(TEXT("il CDO della classe spedita"), Cdo)) { return false; }

	TestTrue(TEXT("il ritmo spedito e' positivo"), Cdo->ScenarioTurnPauseSeconds >= 0.f);
	TestTrue(*FString::Printf(
		TEXT("il ritmo spedito e' molto piu' corto della pianificazione normale (30 s): vale %.2f"),
		Cdo->ScenarioTurnPauseSeconds),
		Cdo->ScenarioTurnPauseSeconds < 10.f);

	// ── **Le altre leve di prova non sono armate nel Blueprint spedito** (`#3067`, criterio 3).
	//
	// Sono le proprieta' `EditAnywhere` che NON rompono l'allestimento, quindi le tre asserzioni del test qui
	// sopra le attraversano senza vederle: una partita con `bAutobattle` acceso o un filtro di scenario armato
	// monta comunque quattro unita', non produce fatali e non sostituisce la mappa.
	//
	// ⚠️ `ScenarioToRun` e' l'eccezione e si legge lo stesso: un valore qui fa uscire `SetupHexMatch` nel ramo
	// dello scenario, quindi le unita' sarebbero ZERO e l'altro test cadrebbe. Asserirlo qui non e' ridondanza —
	// e' il messaggio che nomina la causa invece di far contare le unita' a chi legge.
	TestTrue(TEXT("il Blueprint spedito non arma uno scenario"), Cdo->ScenarioToRun.IsEmpty());
	TestTrue(TEXT("ne' il filtro di scenario A"), Cdo->ScenarioFilterA.IsEmpty());
	TestTrue(TEXT("ne' il filtro di scenario B"), Cdo->ScenarioFilterB.IsEmpty());
	TestFalse(TEXT("il Blueprint spedito non accende l'autobattle"), Cdo->bAutobattle);
	TestEqual(TEXT("ne' aggiunge alleati bot"), Cdo->BotAllyCount, 0);
	TestEqual(TEXT("ne' sovrascrive la finestra di pianificazione (-1 = usa il formato)"),
		Cdo->MatchPlanningSeconds, -1.f);

	// ── **`DemoArenaRadius`**, l'ultima delle dieci, asserita da `#3083`.
	//
	// 🔴 **Fino al 2026-09-12 il Blueprint spedito portava `12` contro i `4` dell'header**, ed era l'unica
	// delle dieci a divergere. Non era una scelta: il token era gia' nel `.uasset` **prima** della pulizia di
	// `#1069`, e `4eed5e0f` rimosse proprio il rig a cui serviva — l'override `GeneratedTestArena` di
	// `MapSource`. E' sopravvissuto perche' non rompe l'allestimento, che e' la ragione strutturale per cui
	// `#3067` esiste.
	//
	// 🔑 **Il reset e' stato misurato sui BYTE, non dedotto dal pannello.** In un CDO di Blueprint i valori
	// sono delta-serializzati contro il CDO padre: scrivere il valore del padre NON lascia «un override che
	// ripete il default» — la proprieta' smette di essere serializzata del tutto. Misurato: il token
	// `DemoArenaRadius` nel `.uasset` passa da **1 a 0** e il package da **21152 a 21099** byte.
	//
	// ⚠️ **Con questa riga i due CDO concordano su tutte e dieci, e quella era la leva che rendeva questo
	// test falsificabile dall'esterno**: la mutazione `M2` di `#3072` usava la divergenza per dimostrare che
	// il CDO letto e' quello dell'asset. Cio' che resta al suo posto non e' una convinzione:
	//
	//   1. la guardia strutturale qui sopra — la classe caricata dev'essere una SOTTOCLASSE, non
	//      `ARTGameMode::StaticClass()`;
	//   2. la validazione per mutazione sul `.uasset`, eseguita in seduta `U46` il 2026-09-12 su `ebba4106`:
	//      scritto `ScenarioTurnPauseSeconds = 15` nei Class Defaults, **solo** questo test diventa rosso e
	//      `Scenario.AutoRunPacingHasShortDefault` resta verde (`#3067`, criterio 2).
	//
	// ⛔ Si confrontano i **due CDO**, non il letterale `4`: un numero scritto a mano qui invecchierebbe il
	// giorno che il default C++ cambia, e direbbe meno di cio' che si vuole dire — che il Blueprint **non
	// sovrascrive**.
	TestEqual(TEXT("ne' il raggio dell'arena di ripiego (resta il default C++, nessun override)"),
		Cdo->DemoArenaRadius,
		ARTGameMode::StaticClass()->GetDefaultObject<ARTGameMode>()->DemoArenaRadius);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
