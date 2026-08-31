// LE MISURE CHE #1902 e #1896 CHIEDONO, prese headless invece che a mano in PIE.
//
// ⛔ **Non sono test di regressione, e la differenza va detta.** Un test di regressione fissa un
// comportamento e diventa rosso se cambia. Questi due producono un NUMERO che serve a decidere: le loro
// asserzioni sono volutamente deboli — verificano che la misura sia stata presa su una partita vera, non
// che valga un valore particolare. Il valore si legge nel log ed entra nella issue.
//
// ∴ se domani il bot cambia e i numeri cambiano, questi test **devono restare verdi**: il loro compito e'
// misurare, non pinnare. Chi volesse pinnare una soglia lo faccia in un test suo, dopo che la decisione
// di #1902 e' stata presa — oggi non esiste una soglia da difendere.
//
// ⚠️ **Il baseline «Phase umana» NON e' ottenibile headless**, ed e' il limite di questa misura: senza
// input il compagno resta fermo. Il confronto e' quindi *Phase ferma* contro *Phase pianificata dal bot*,
// che risponde alla domanda «il bot contribuisce alla conoscenza piu' di un'unita' immobile?» — non alla
// domanda «contribuisce quanto una persona?». Quest'ultima resta una seduta PIE.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTGameMode.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definite in RTGameMode.cpp: le sorgenti «di adesso» che scavalcano la proprieta' del GameMode. */
extern TAutoConsoleVariable<int32> CVarRTBotAllies;
extern TAutoConsoleVariable<int32> CVarRTAutobattle;

namespace
{
	// ⚠️ Nomi prefissati `Recon`: la unity build condivide la translation unit con gli altri file di test.

	/** I tre volumi che descrivono cosa una squadra sa, presi dallo stato LOGICO e non dal velo. */
	struct FRTReconVolumi
	{
		int32 Visibili = 0;
		int32 Esplorate = 0;
		int32 Contatti = 0;
	};

	struct FRTScopedReconState
	{
		FString SavedCommandLine;
		int32 SavedBotAllies;
		int32 SavedAutobattle;

		FRTScopedReconState()
			: SavedCommandLine(FCommandLine::Get())
			, SavedBotAllies(CVarRTBotAllies.GetValueOnGameThread())
			, SavedAutobattle(CVarRTAutobattle.GetValueOnGameThread())
		{
			// La configurazione la decide la proprieta' del GameMode: le sorgenti «di adesso» a riposo,
			// altrimenti una CVar lasciata accesa da un altro test deciderebbe al posto della misura.
			CVarRTBotAllies->Set(-1, ECVF_SetByCode);
			CVarRTAutobattle->Set(-1, ECVF_SetByCode);
		}

		~FRTScopedReconState()
		{
			FCommandLine::Set(*SavedCommandLine);
			CVarRTBotAllies->Set(SavedBotAllies, ECVF_SetByCode);
			CVarRTAutobattle->Set(SavedAutobattle, ECVF_SetByCode);
		}
	};

	UWorld* MakeReconWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyReconWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Gioca `Turni` turni con `BotAllyCount` compagni al bot e restituisce cosa sa la squadra 0.
	 *
	 * Passa da `SetupHexMatch` — il percorso vero — e non da un allestimento a mano: una fixture costruita
	 * qui resterebbe verde anche se il GameMode smettesse di assegnare il flag, cioe' misurerebbe la propria
	 * copia. E' la stessa disciplina di `RTHeroSpawnTests`.
	 */
	bool ReconGioca(int32 BotAllyCount, int32 Turni, FRTReconVolumi& OutVolumi, int32& OutUnitaVive)
	{
		UWorld* World = MakeReconWorld();
		if (!World) { return false; }

		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !TM || !GameMode) { DestroyReconWorld(World); return false; }

		HexMap->MapAsset = Map;
		GameMode->BotAllyCount = BotAllyCount;
		GameMode->SetupHexMatch(HexMap);

		// I turni li chiude `LockInAndResolve`, non il timer: qui non c'e' nessuno che prema Ready e il
		// tempo di gioco non scorre. Ogni risoluzione si lascia finire prima della successiva, altrimenti
		// si misurerebbe uno stato a meta' fase.
		for (int32 T = 0; T < Turni; ++T)
		{
			TM->LockInAndResolve();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
			{
				TM->Tick(0.05f);
			}
		}

		// Lo stato LOGICO, non il velo: `GetKnowledgeDebugCounts` conta le istanze disegnate e headless
		// non ne esistono. `KnowledgeForTeamPublic` e' la conoscenza che la simulazione ha prodotto, ed e'
		// la stessa che il presenter mostrerebbe.
		const FRTTeamKnowledge K = TM->KnowledgeForTeamPublic(0);
		OutVolumi.Visibili = K.VisibleCells.Num();
		OutVolumi.Esplorate = K.ExploredCells.Num();
		OutVolumi.Contatti = K.Contacts.Num();

		// Quante unita' della squadra 0 sono ancora in campo: un confronto fra due partite in cui una ha
		// perso un'unita' e l'altra no non parla di ricognizione, parla di sopravvivenza.
		OutUnitaVive = 0;
		TArray<AActor*> Trovate;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Trovate);
		for (const AActor* A : Trovate)
		{
			const ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->TeamId == 0 && U->IsAlive()) { ++OutUnitaVive; }
		}

		DestroyReconWorld(World);
		return true;
	}
}

/**
 * #1902 — IL BASELINE: quanto sa la squadra del giocatore, con il compagno fermo e con il compagno al bot.
 *
 * Il numero da riportare nella issue e' nel log di questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReconBaselineTest,
	"RefactorTactics.Bot.Ally.ReconBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReconBaselineTest::RunTest(const FString&)
{
	FRTScopedReconState Guard;

	static constexpr int32 Turni = 5;

	FRTReconVolumi Ferma, Bot;
	int32 ViveFerma = 0, ViveBot = 0;
	if (!TestTrue(TEXT("la partita col compagno fermo e' stata giocata"),
			ReconGioca(/*BotAllyCount=*/ 0, Turni, Ferma, ViveFerma))
		|| !TestTrue(TEXT("la partita col compagno al bot e' stata giocata"),
			ReconGioca(/*BotAllyCount=*/ 1, Turni, Bot, ViveBot)))
	{
		return false;
	}

	AddInfo(FString::Printf(
		TEXT("[#1902 BASELINE dopo %d turni] compagno FERMO: %d visibili · %d esplorate · %d contatti "
			 "(%d unita' vive) || compagno al BOT: %d visibili · %d esplorate · %d contatti (%d vive)"),
		Turni, Ferma.Visibili, Ferma.Esplorate, Ferma.Contatti, ViveFerma,
		Bot.Visibili, Bot.Esplorate, Bot.Contatti, ViveBot));

	// PREMESSA: la misura viene da una partita vera. Senza, due zeri identici sarebbero «nessuna
	// differenza» — la conclusione piu' comoda e la piu' falsa.
	TestTrue(TEXT("premessa: la squadra 0 conosce qualcosa in entrambe le partite"),
		Ferma.Visibili > 0 && Bot.Visibili > 0);
	TestTrue(TEXT("premessa: la squadra 0 ha unita' in campo in entrambe"), ViveFerma > 0 && ViveBot > 0);

	// L'UNICA asserzione di merito, e volutamente debole: un compagno che si muove non puo' far sapere
	// alla squadra MENO di un compagno immobile. Se questa cadesse non sarebbe una questione di pesi:
	// sarebbe un difetto della conoscenza, e andrebbe indagato prima di qualunque discorso sullo score.
	TestTrue(TEXT("il compagno al bot non riduce cio' che la squadra ha esplorato"),
		Bot.Esplorate >= Ferma.Esplorate);

	return true;
}

/**
 * #1896 — LA PREMESSA STRUTTURALE della diagnosi, verificabile senza aprire l'editor.
 *
 * Il warning nasce in un nodo Blueprint e headless non e' riproducibile. Ma la diagnosi dice: il
 * denominatore e' `CooldownTurns`, il cui default e' `0`, quindi il difetto **non dipende dall'eroe**. Se
 * ogni eroe del roster porta almeno un'azione a ricarica zero, quella predizione e' strutturalmente
 * attesa e la seduta PIE si riduce a una conferma.
 *
 * ⛔ Questo test **non prova** che il divide-by-zero avvenga: prova che la condizione che lo spiegherebbe
 * e' presente in tutti e quattro i kit. La distinzione e' il motivo per cui la issue resta aperta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTZeroCooldownIsUbiquitousTest,
	"RefactorTactics.Hud.ZeroCooldownAbilitiesExistForEveryHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTZeroCooldownIsUbiquitousTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("premessa: il catalogo eroi non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }
		const FString Chi = Hero->HeroId.ToString();

		int32 ARicaricaZero = 0;
		for (const URTActionData* Azione : Hero->Actions)
		{
			if (Azione && Azione->Def.CooldownTurns == 0) { ++ARicaricaZero; }
		}

		AddInfo(FString::Printf(TEXT("[#1896] %s: %d azioni su %d hanno CooldownTurns == 0"),
			*Chi, ARicaricaZero, Hero->Actions.Num()));

		// Se questa cade per un eroe, la diagnosi di #1896 e' sbagliata e va ripresa la pista
		// dell'equipaggiamento: sarebbe l'unico modo perche' il difetto dipenda da CHI e' selezionato.
        TestTrue(FString::Printf(TEXT("%s porta almeno un'azione senza ricarica"), *Chi),
			ARicaricaZero > 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
