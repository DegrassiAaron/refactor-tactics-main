// LA POLITICA DEL CONFINE COL FRONTEND, provata senza passare da un GameMode.
//
// `RTMatchEndOpensResultTests` e `RTFrontendPauseTests` provano il CABLAGGIO — che una partita vera finisca
// aprendo il Result, e che il ritorno al menu non lasci stato vivo — e devono continuare a farlo passando
// dal ciclo di vita, perche' e' la lezione di `#939`: otto test verdi non videro che il consumatore non era
// collegato a niente, perche' lo collegavano tutti da se'.
//
// Qui si prova la sola POLITICA: che la richiesta si consumi invece di essere letta, che un annuncio senza
// richiesta pendente non apra niente e lo dica, e che l'assenza di frontend non sia un errore. Sono due
// lenti diverse sullo stesso confine, e nessuna sostituisce l'altra.

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Frontend/RTMatchFrontendBridge.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La richiesta si CONSUMA: la prima volta dice quale livello aprire, la seconda non ha piu' niente da dire.
 *
 * ⚠️ **La seconda chiamata e' meta' del test.** Una richiesta letta e non consumata resta li' e fa rifiutare
 * il `PLAY` successivo con «mai consumata» — un messaggio che punta il dito su chi non consuma invece che su
 * chi non si e' iscritto, e che e' gia' costato una diagnosi intera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchBridgeConsumesOnceTest,
	"RefactorTactics.Frontend.BridgeConsumesTheMatchRequestOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchBridgeConsumesOnceTest::RunTest(const FString&)
{
	UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
	if (!TestNotNull(TEXT("GameInstance"), GI)) { return false; }
	GI->AddToRoot();
	GI->Init();

	URTFrontendNavigator* Nav = FRTMatchFrontendBridge::FindNavigator(GI);
	if (!TestNotNull(TEXT("il bridge trova il navigatore"), Nav))
	{
		GI->RemoveFromRoot();
		return false;
	}

	Nav->InitializeFrontend(RTScreenIds::Main);

	// ⚠️ Il livello si dichiara QUI e non si eredita da `DefaultGame.ini`: un test che dipendesse dal file
	// di configurazione misurerebbe anche quello, e diventerebbe rosso il giorno in cui qualcuno rinomina
	// una mappa per una ragione che con questo confine non c'entra.
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Test/L_BridgeTest");
	Nav->StartMatch();

	const FString Primo = FRTMatchFrontendBridge::ConsumeMatchLevel(GI, TEXT("L_Annunciato"));
	TestEqual(TEXT("con una richiesta pendente si apre il livello CHIESTO, non quello annunciato"),
		Primo, Nav->MatchLevel);

	// Seconda chiamata: la richiesta e' gia' stata consumata, quindi non c'e' piu' niente da aprire — e il
	// caso va DICHIARATO, non taciuto.
	AddExpectedError(TEXT("senza richiesta pendente"), EAutomationExpectedErrorFlags::Contains, 1);
	const FString Secondo = FRTMatchFrontendBridge::ConsumeMatchLevel(GI, TEXT("L_Annunciato"));
	TestTrue(TEXT("la seconda volta non c'e' piu' niente da aprire"), Secondo.IsEmpty());

	GI->RemoveFromRoot();
	return true;
}

/**
 * Senza frontend la partita gira lo stesso, e **non e' un errore**.
 *
 * E' la differenza voluta con `ARTFrontendGameMode`: li' un navigatore assente significa un menu che non
 * aprirebbe niente; qui uno scenario headless o un test di simulazione girano senza frontend, e la partita
 * deve poter finire lo stesso. Chi ha bisogno del Result e' il gioco, non il resolver.
 *
 * ⚠️ **Nessun `AddExpectedError`, ed e' deliberato**: se questi percorsi urlassero, l'errore non atteso
 * farebbe cadere il test — che e' precisamente il modo in cui si vuole scoprirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchBridgeWithoutFrontendTest,
	"RefactorTactics.Frontend.BridgeWithoutFrontendIsNotAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchBridgeWithoutFrontendTest::RunTest(const FString&)
{
	TestNull(TEXT("senza GameInstance non c'e' navigatore"), FRTMatchFrontendBridge::FindNavigator(nullptr));

	// La fine partita senza frontend si dichiara in Verbose e si tira dritto: nessun Error, nessun crash.
	const FRTMatchResult Result;
	const FRTMatchState State;
	FRTMatchFrontendBridge::ShowResult(nullptr, Result, State);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
