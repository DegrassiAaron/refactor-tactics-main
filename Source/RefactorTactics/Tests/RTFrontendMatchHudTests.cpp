// CP 11.7 (#613) — il layer HUD: dove compare, quando se ne va, e perche' non e' una schermata.
//
// ⚠️ **Qui non si prova un layout.** L'ingombro del §4.1, la leggibilita' delle barre e il centro libero
// sono `PIE-V01-HUD` e la seduta d'editor U15. Cio' che si prova senza editor e' il **ciclo di vita**: un
// HUD che compare con la partita, sopravvive alla pausa, resta inerte sotto un modale, e se ne va quando
// la partita finisce.
//
// 🔴 **Il difetto che questo file esiste per impedire.** `SyncPresentation` smonta ogni widget di
// `LiveWidgets` che non sia la cima dello stack o un modale (`RTFrontendNavigator.cpp`). Un HUD registrato
// come schermata sparirebbe all'apertura della pausa — e il sintomo sarebbe la partita nuda sotto un menu,
// che si legge come un difetto del menu invece che del layer. Il HUD vive quindi in un campo suo, fuori
// dalla mappa, con `ZOrder` negativo.

#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
// Le `UUserWidget` concrete da registrare: `UUserWidget` e' `Abstract` e `CreateWidget` la rifiuta.
#include "UI/RTScreenHudWidgets.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTMatchHudTestsLocal
{
	URTFrontendNavigator* MakeNavigator(UGameInstance*& OutGI)
	{
		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (!OutGI)
		{
			return nullptr;
		}
		OutGI->AddToRoot();
		OutGI->Init();
		return OutGI->GetSubsystem<URTFrontendNavigator>();
	}

	void ReleaseNavigator(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}

	FRTScreenBinding MakeBinding(FName ScreenId)
	{
		FRTScreenBinding Binding;
		Binding.ScreenId = ScreenId;
		Binding.WidgetClass = TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass());
		return Binding;
	}

	/**
	 * Un navigatore avviato sul menu, con la pausa registrata e il HUD dichiarato.
	 *
	 * ⚠️ **Non legge il `.ini`**, per la stessa ragione di `RTFrontendPauseTests`: `StartFrontendFrom` e
	 * `SetMatchHudWidgetClassForTest` esistono apposta, e un test che dipendesse dalla configurazione
	 * misurerebbe il `.ini` invece del proprio soggetto.
	 */
	URTFrontendNavigator* MakeStartedNavigator(UGameInstance*& OutGI)
	{
		URTFrontendNavigator* Nav = MakeNavigator(OutGI);
		if (!Nav)
		{
			return nullptr;
		}

		TArray<FRTScreenBinding> Screens;
		Screens.Add(MakeBinding(RTScreenIds::Main));
		Screens.Add(MakeBinding(RTScreenIds::Pause));
		Nav->StartFrontendFrom(Screens);

		// ⚠️ **Una classe DIVERSA da quella delle schermate**, e non e' un dettaglio di allestimento: con la
		// stessa classe, `MatchHudSurvivesThePause` resterebbe verde anche se il HUD finisse in
		// `LiveWidgets` — due widget indistinguibili, e nessuna asserzione capace di separarli.
		Nav->SetMatchHudWidgetClassForTest(TSoftClassPtr<UUserWidget>(URTTacticalHUDWidget::StaticClass()));
		return Nav;
	}
}

// ─── Il ciclo di vita ────────────────────────────────────────────────────────────────────────────────

/**
 * Il HUD compare con la partita, e **non prima**.
 *
 * Nel menu non deve esistere: un HUD costruito all'avvio del frontend leggerebbe un `TurnManager` che non
 * c'e', e mostrerebbe la vista neutra sopra un Main Menu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudAppearsWithTheMatchTest,
	"RefactorTactics.Frontend.MatchHudAppearsWithTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudAppearsWithTheMatchTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	TestEqual(TEXT("si parte dal menu"), Nav->GetCurrentScreen(), RTScreenIds::Main);
	TestNull(TEXT("e nel menu non c'e' HUD di partita"), Nav->GetMatchHudWidget());

	TestEqual(TEXT("EnterMatch"), Nav->EnterMatch(), ERTNavResult::Ok);
	TestNotNull(TEXT("con la partita, il HUD c'e'"), Nav->GetMatchHudWidget());

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * 🔴 **Il test che vale il layer: il HUD non e' una schermata.**
 *
 * `SyncPresentation` smonta ogni widget di `LiveWidgets` che non sia la cima o un modale. Se il HUD ci
 * stesse dentro, la pausa lo farebbe sparire. Le due asserzioni sono complementari e servono entrambe:
 * l'identita' del widget prova che non e' stato ricostruito, l'assenza da `LiveWidgets` prova **perche'**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudSurvivesThePauseTest,
	"RefactorTactics.Frontend.MatchHudSurvivesThePause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudSurvivesThePauseTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	Nav->EnterMatch();
	UUserWidget* Hud = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), Hud))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	Nav->ShowPause();
	TestEqual(TEXT("sotto la pausa il HUD e' lo STESSO widget"), Nav->GetMatchHudWidget(), Hud);
	TestNull(TEXT("e non e' registrato come schermata"), Nav->FindLiveWidget(RTScreenIds::Match));

	Nav->ResumeMatch();
	TestEqual(TEXT("e il RESUME non lo ricrea"), Nav->GetMatchHudWidget(), Hud);

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Sotto un modale il HUD resta **visibile e inerte** — non nascosto.
 *
 * E' la meta' di `Modal/Reaction > HUD > world` che questo checkpoint puo' consegnare: un modale che
 * oscura il contesto che sta interrompendo e' peggio di nessun modale, ma un HUD **cliccabile** sotto una
 * pausa e' un secondo ruleset. L'altra meta' — il click che attraversa il HUD e arriva al mondo — e' di
 * CP 11.8 (#705), e questo test non la tocca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudIsInertUnderThePauseTest,
	"RefactorTactics.Frontend.MatchHudIsInertUnderThePause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudIsInertUnderThePauseTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	Nav->EnterMatch();
	UUserWidget* Hud = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), Hud))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	TestTrue(TEXT("in partita il HUD riceve input"), Hud->GetIsEnabled());

	Nav->ShowPause();
	TestFalse(TEXT("sotto la pausa e' inerte"), Hud->GetIsEnabled());

	Nav->ResumeMatch();
	TestTrue(TEXT("e il RESUME glielo ridà"), Hud->GetIsEnabled());

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Tornando al menu il HUD se ne va, e l'istanza **non si riusa**.
 *
 * ⚠️ L'azzeramento del puntatore e' la lezione di PR #1264, gia' pagata una volta su `LiveWidgets`: i
 * widget appartengono al **mondo in cui sono stati costruiti**, e riusarne uno dopo un cambio di livello
 * chiama `AddToViewport` su un mondo smontato. Il test lo misura confrontando le due istanze, non
 * fidandosi del fatto che il puntatore sia stato azzerato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudLeavesWithTheMatchTest,
	"RefactorTactics.Frontend.MatchHudLeavesWithTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudLeavesWithTheMatchTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	Nav->EnterMatch();
	UUserWidget* First = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), First))
	{
		RTMatchHudTestsLocal::ReleaseNavigator(GI);
		return false;
	}

	Nav->InitializeFrontend(RTScreenIds::Main);
	TestNull(TEXT("tornando al menu il HUD se ne va"), Nav->GetMatchHudWidget());

	Nav->EnterMatch();
	UUserWidget* Second = Nav->GetMatchHudWidget();
	TestNotNull(TEXT("la partita dopo ne ha uno"), Second);
	TestNotEqual(TEXT("e non e' l'istanza del mondo smontato"), Second, First);

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
