#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // GetGenericActionIds: il kit e' eroe + generiche (D-025)
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Lo spawn del roster attraverso `ARTGameMode::SetupHexMatch` — il percorso VERO di CP 6.6, non una sua
 * imitazione. Un test che ricostruisse a mano cio' che fa `SpawnHero` resterebbe verde anche togliendo la
 * guardia fail-closed dal GameMode: verificherebbe la propria copia, non il codice spedito. (E' successo:
 * la prima stesura di questo file faceva esattamente quell'errore, e la verifica di mutazione l'ha scoperto.)
 */
namespace
{
	UWorld* MakeRosterWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRosterWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale piena: abbastanza celle percorribili per le quattro posizioni di partenza. */
	ARTHexMapActor* SpawnRosterMap(UWorld* World, int32 Radius = 4)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	TArray<ARTUnit*> CollectRosterUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);

		TArray<ARTUnit*> Units;
		for (AActor* Actor : Found)
		{
			if (ARTUnit* Unit = Cast<ARTUnit>(Actor)) { Units.Add(Unit); }
		}
		return Units;
	}

	ARTUnit* FindByHeroId(const TArray<ARTUnit*>& Units, const TCHAR* HeroId)
	{
		for (ARTUnit* Unit : Units)
		{
			if (Unit->HeroId == FName(HeroId)) { return Unit; }
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroSpawnFromDataTest,
	"RefactorTactics.Heroes.SpawnFromData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroSpawnFromDataTest::RunTest(const FString&)
{
	// Nome vincolante della DoD (CP 6.6): il GameMode allestisce il 2v2 con i QUATTRO EROI del catalogo, non
	// piu' con due archetipi hard-coded.
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// I quattro eroi del catalogo, ognuno una volta sola: nessun duplicato involontario.
	TSet<FName> InPlay;
	for (const ARTUnit* Unit : Units) { InPlay.Add(Unit->HeroId); }
	TestEqual(TEXT("quattro eroi distinti"), InPlay.Num(), 4);
	TestTrue(TEXT("c'e' Flux"), InPlay.Contains(FName(TEXT("Hero.Flux"))));
	TestTrue(TEXT("c'e' Riva"), InPlay.Contains(FName(TEXT("Hero.Riva"))));
	TestTrue(TEXT("c'e' Bastion"), InPlay.Contains(FName(TEXT("Hero.Bastion"))));
	TestTrue(TEXT("c'e' Vektor"), InPlay.Contains(FName(TEXT("Hero.Vektor"))));

	// Ogni unita' porta le statistiche del SUO eroe, sopravvissute a BeginPlay.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const FString Who = Hero->HeroId.ToString();
		ARTUnit* Unit = FindByHeroId(Units, *Who);
		if (!TestNotNull(FString::Printf(TEXT("%s in campo"), *Who), Unit)) { continue; }

		TestEqual(FString::Printf(TEXT("%s: salute"), *Who), Unit->MaxHealth, Hero->MaxHealth);
		TestEqual(FString::Printf(TEXT("%s: salute corrente = massima"), *Who), Unit->Health, Hero->MaxHealth);
		TestEqual(FString::Printf(TEXT("%s: movimento"), *Who), Unit->MoveRange, Hero->MovePoints);
		TestEqual(FString::Printf(TEXT("%s: vista"), *Who), Unit->VisionRange, Hero->VisionRange);
		TestEqual(FString::Printf(TEXT("%s: resistenza push"), *Who), Unit->PushResistance, Hero->PushResistance);
		TestEqual(FString::Printf(TEXT("%s: affinita'"), *Who), Unit->Affinity, Hero->Affinity);
		// Cinque dell'eroe piu' le generiche di D-025: ogni unita' in campo puo' dichiarare `Guard` e `Brace`,
		// e i quattro rami che li consumano nel TurnManager smettono di essere irraggiungibili.
		TestEqual(FString::Printf(TEXT("%s: azioni dell'eroe piu' generiche"), *Who),
			Unit->NumAbilities(), 5 + URTCatalogLibrary::GetGenericActionIds().Num());
		TestEqual(FString::Printf(TEXT("%s: l'indice 0 resta l'attacco base"), *Who),
			Unit->GetAbility(0)->Def.ActionId, Hero->Actions[0]->Def.ActionId);
	}

	// Formazione di default: Flux+Riva (giocatore) contro Bastion+Vektor (bot).
	ARTUnit* Flux = FindByHeroId(Units, TEXT("Hero.Flux"));
	ARTUnit* Riva = FindByHeroId(Units, TEXT("Hero.Riva"));
	ARTUnit* Bastion = FindByHeroId(Units, TEXT("Hero.Bastion"));
	ARTUnit* Vektor = FindByHeroId(Units, TEXT("Hero.Vektor"));
	if (Flux && Riva && Bastion && Vektor)
	{
		TestEqual(TEXT("Flux e' del giocatore"), Flux->TeamId, 0);
		TestEqual(TEXT("Riva anche: la combo Wet e' giocabile"), Riva->TeamId, 0);
		TestEqual(TEXT("Bastion e' del bot"), Bastion->TeamId, 1);
		TestEqual(TEXT("Vektor anche"), Vektor->TeamId, 1);
		TestFalse(TEXT("il giocatore comanda i suoi"), Flux->bIsBotControlled);
		TestTrue(TEXT("il bot comanda i propri"), Bastion->bIsBotControlled);

		// **Il punto della DoD sul bot**: MP diversi arrivano davvero in campo. Il budget dello snapshot viene
		// da `MoveRange`, quindi il bot non puo' proporre a Bastion una mossa da 6 celle.
		TestEqual(TEXT("Bastion: 4 MP"), Bastion->MoveRange, 4);
		TestEqual(TEXT("Vektor: 6 MP"), Vektor->MoveRange, 6);
		TestEqual(TEXT("Flux colpisce a 4"), Flux->AttackRange, 4);
		TestEqual(TEXT("Bastion colpisce a 3"), Bastion->AttackRange, 3);

		// Ogni unita' ha le PROPRIE istanze d'azione: due eroi che condividessero un `URTActionData`
		// ricaricherebbero insieme.
		TestTrue(TEXT("azioni non condivise"), Flux->GetAbility(0) != Riva->GetAbility(0));

		// Le unita' stanno su celle distinte della mappa.
		TSet<FRTCellId> Cells;
		for (const ARTUnit* Unit : Units) { Cells.Add(Unit->Cell); }
		TestEqual(TEXT("quattro celle di partenza distinte"), Cells.Num(), 4);
	}

	DestroyRosterWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroSpawnFailClosedTest,
	"RefactorTactics.Heroes.SpawnFailsClosedWithoutData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroSpawnFailClosedTest::RunTest(const FString&)
{
	// Un HeroId che non esiste nel catalogo non produce un'unita' con statistiche di default: non produce
	// NIENTE. Un'unita' mancante si diagnostica; una coi numeri di default, che sembrano plausibili, no.
	UWorld* World = MakeRosterWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnRosterMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyRosterWorld(World);
		return false;
	}

	// Due modi diversi di sbagliare una formazione: un eroe che non esiste, e uno schierato DUE volte.
	// Nessuno dei due deve mettere in campo un'unita' senza dati o una copia che condivide le azioni.
	GameMode->Team0Heroes = { TEXT("Hero.Nonexistent"), TEXT("Hero.Flux") };
	GameMode->Team1Heroes = { TEXT("Hero.Flux"), TEXT("Hero.Vektor") };

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectRosterUnits(World);
	TestEqual(TEXT("solo i due eroi validi entrano in campo"), Units.Num(), 2);

	TSet<FName> InPlay;
	for (const ARTUnit* Unit : Units) { InPlay.Add(Unit->HeroId); }
	TestTrue(TEXT("Flux, una volta sola"), InPlay.Contains(FName(TEXT("Hero.Flux"))));
	TestTrue(TEXT("e Vektor"), InPlay.Contains(FName(TEXT("Hero.Vektor"))));
	TestEqual(TEXT("nessun duplicato in campo"), InPlay.Num(), Units.Num());

	// Nessuna unita' e' rimasta senza identita': se il fail-closed non funzionasse, qui ci sarebbe un'unita'
	// con `HeroId` vuoto e i valori di default di `ARTUnit`.
	for (const ARTUnit* Unit : Units)
	{
		TestFalse(TEXT("ogni unita' in campo ha un eroe"), Unit->HeroId.IsNone());
	}

	DestroyRosterWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
