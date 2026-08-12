// Le viste dello Screen HUD (§4.1, CP 11.7): cosa i widget leggono, e cosa NON possono leggere.
//
// Il valore di queste funzioni non e' che risparmiano righe al widget — e' che gli tolgono la possibilita' di
// sbagliare. `BuildTeamRoster` non ha un parametro «mostra anche gli avversari», e `BuildMatchHeader` non ha
// un posto dove scrivere `12`: le due regole che il DoD di CP 11.1 e la spec §4.1 impongono diventano
// proprieta' della firma invece che disciplina.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeHudVmWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHudVmWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnHudVmUnit(UWorld* World, FName HeroId, int32 TeamId)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H && H->HeroId == HeroId) { Hero = H; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (!Unit) { return nullptr; }
		Unit->ConfigureFromHeroData(Hero);
		Unit->TeamId = TeamId;
		return Unit;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Il roster e' della PROPRIA squadra, e le morte restano
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmRosterIsOwnTeamTest,
	"RefactorTactics.HudViewModel.RosterIsOwnTeamOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmRosterIsOwnTeamTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Flux = SpawnHudVmUnit(World, TEXT("Hero.Flux"), /*TeamId*/ 0);
	ARTUnit* Riva = SpawnHudVmUnit(World, TEXT("Hero.Riva"), /*TeamId*/ 0);
	ARTUnit* Bastion = SpawnHudVmUnit(World, TEXT("Hero.Bastion"), /*TeamId*/ 1);
	ARTUnit* Vektor = SpawnHudVmUnit(World, TEXT("Hero.Vektor"), /*TeamId*/ 1);

	if (!TestNotNull(TEXT("Flux"), Flux) || !TestNotNull(TEXT("Riva"), Riva)
		|| !TestNotNull(TEXT("Bastion"), Bastion) || !TestNotNull(TEXT("Vektor"), Vektor))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	const TArray<ARTUnit*> All = { Flux, Riva, Bastion, Vektor };

	const TArray<FRTUnitCardView> Mine = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("il roster ha le due unita' della mia squadra"), Mine.Num(), 2);
	for (const FRTUnitCardView& Card : Mine)
	{
		TestTrue(*FString::Printf(TEXT("%s e' un alleato"), *Card.HeroId.ToString()), Card.bIsAlly);
		TestFalse(TEXT("nessun avversario nel roster"),
			Card.HeroId == TEXT("Hero.Bastion") || Card.HeroId == TEXT("Hero.Vektor"));
	}

	// Simmetrico: cambiando squadra cambia il roster, e la funzione non ha altri parametri con cui sbagliare.
	const TArray<FRTUnitCardView> Theirs = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 1);
	TestEqual(TEXT("dall'altra parte se ne vedono due"), Theirs.Num(), 2);

	// Una morta NON sparisce: il conto della squadra deve restare leggibile.
	Flux->Health = 0;
	const TArray<FRTUnitCardView> AfterDeath = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("il roster resta di due voci anche con una unita' morta"), AfterDeath.Num(), 2);

	int32 Dead = 0;
	for (const FRTUnitCardView& Card : AfterDeath) { if (!Card.bAlive) { ++Dead; } }
	TestEqual(TEXT("e una e' marcata come non viva"), Dead, 1);

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La carta riflette il simulatore, non una copia
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmCardMirrorsUnitTest,
	"RefactorTactics.HudViewModel.CardMirrorsSimulator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmCardMirrorsUnitTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Bastion = SpawnHudVmUnit(World, TEXT("Hero.Bastion"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("Bastion"), Bastion)) { DestroyHudVmWorld(World); return false; }

	Bastion->Health = 42;
	Bastion->Shield = 7;
	Bastion->Energy = 13;

	const FRTUnitCardView Card = URTHudViewModel::BuildUnitCard(Bastion, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("salute"), Card.Health, 42);
	TestEqual(TEXT("scudo"), Card.Shield, 7);
	TestEqual(TEXT("energia"), Card.Energy, 13);
	TestEqual(TEXT("salute massima dal catalogo eroi"), Card.MaxHealth, Bastion->MaxHealth);
	TestEqual(TEXT("identita'"), Card.HeroId, Bastion->HeroId);
	TestTrue(TEXT("alleato"), Card.bIsAlly);
	TestTrue(TEXT("vivo"), Card.bAlive);

	// La stessa unita' vista dall'altra squadra e' la stessa unita': cambia la RELAZIONE, non i numeri.
	const FRTUnitCardView Enemy = URTHudViewModel::BuildUnitCard(Bastion, /*PlayerTeamId*/ 1);
	TestFalse(TEXT("vista da squadra 1 non e' alleata"), Enemy.bIsAlly);
	TestEqual(TEXT("ma la salute e' la stessa"), Enemy.Health, 42);

	// Nulla non e' «unita' a zero»: e' assenza. `bAlive` falso lo distingue da un'unita' morta con un nome.
	const FRTUnitCardView None = URTHudViewModel::BuildUnitCard(nullptr, /*PlayerTeamId*/ 0);
	TestFalse(TEXT("una carta senza unita' non e' viva"), None.bAlive);
	TestTrue(TEXT("e non ha identita'"), None.HeroId.IsNone());

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'intestazione senza manager e' neutra, non «zero»
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmHeaderWithoutManagerTest,
	"RefactorTactics.HudViewModel.HeaderWithoutManagerIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmHeaderWithoutManagerTest::RunTest(const FString&)
{
	const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(nullptr);

	TestEqual(TEXT("nessun round"), View.Round, 0);

	// ⚠️ La voce che conta: `RoundLimit` a 0 significa «non dichiarato», e un widget deve poterlo distinguere
	// da un limite vero. E' la stessa ragione per cui `ARTHUD` oggi stampa «Turno n» senza «/0».
	TestEqual(TEXT("nessun limite dichiarato"), View.RoundLimit, 0);

	// Negativo, non zero: «la domanda non si applica» non e' «il tempo e' scaduto».
	TestTrue(TEXT("il timer non si applica"), View.PlanningSecondsRemaining < 0.f);
	TestFalse(TEXT("non si sta risolvendo"), View.bResolving);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Un Planning SENZA orologio non e' un Planning scaduto
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmUntimedPlanningTest,
	"RefactorTactics.HudViewModel.UntimedPlanningIsNotExpired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmUntimedPlanningTest::RunTest(const FString&)
{
	// ⚠️ Questo test esiste per un difetto trovato in code review, non per completezza.
	//
	// `GetPlanningTimeRemaining()` clampa a `0.f`, e restituisce quello stesso `0.f` in DUE situazioni che il
	// giocatore vede in modo opposto: il tempo e' finito, oppure non c'e' mai stato un tempo. Il timer si
	// imposta solo `if (PlanningSeconds > 0.f)`, e `RTScenarioSession` usa `SetPlanningSeconds(0.f)` per le
	// run headless — quindi il secondo caso e' un percorso reale, non un'ipotesi.
	//
	// Il test precedente non poteva accorgersene: passando `nullptr` riceveva i default della struct senza
	// mai attraversare il ramo del timer.
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	// Nessun orologio: e' cio' che fa l'harness headless.
	TM->SetPlanningSeconds(0.f);

	const FRTMatchHeaderView Untimed = URTHudViewModel::BuildMatchHeader(TM);
	TestEqual(TEXT("siamo in Planning"), Untimed.Phase, ERTMatchPhase::Planning);
	TestTrue(TEXT("senza orologio il timer NON si applica, non e' scaduto"),
		Untimed.PlanningSecondsRemaining < 0.f);

	// Con un orologio vero la domanda si applica, e la risposta non e' negativa.
	TM->SetPlanningSeconds(30.f);
	const FRTMatchHeaderView Timed = URTHudViewModel::BuildMatchHeader(TM);
	TestTrue(TEXT("con un orologio il tempo residuo e' un numero utilizzabile"),
		Timed.PlanningSecondsRemaining >= 0.f);

	DestroyHudVmWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
