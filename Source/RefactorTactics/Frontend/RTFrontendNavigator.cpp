#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendWidgets.h"   // URTErrorModalWidgetBase: il modale si ARMA, non si mostra e basta
#include "Frontend/RTStartupReport.h"    // DescribeOutcome: il testo del motivo vive li'

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
// Per `FWorldDelegates::OnWorldCleanup`: la cache dei widget muore col mondo che li ha costruiti.
#include "Engine/World.h"
// Per `RTScreenIds::Main`: la radice del frontend ha un nome canonico, e `StartFrontend` lo usa invece di
// riscrivere `TEXT("Main")` — un refuso qui non produrrebbe un errore ma una schermata che non disegna.
#include "Frontend/RTFrontendScreenIds.h"
// Per `LogRT`: `StartFrontend` avvisa quando la configurazione non ha prodotto nessuna schermata.
#include "RefactorTactics.h"

namespace
{
	/**
	 * Le schermate stanno sotto, i modali sopra. Due bande separate invece di un contatore crescente:
	 * un modale aperto su una schermata profonda deve restare sopra **qualunque** schermata, anche dopo un
	 * `ReturnMain`, e con uno Z progressivo finirebbe sotto alla prima navigazione.
	 */
	constexpr int32 ScreenZBase = 0;
	constexpr int32 ModalZBase = 1000;

	// Sotto le schermate, e di molto: il HUD e' cio' che sta **dietro** a tutto il frontend. Un margine
	// stretto renderebbe l'ordine una coincidenza invece di una dichiarazione.
	constexpr int32 MatchHudZ = -100;
}

ERTNavResult URTFrontendNavigator::InitializeFrontend(FName RootScreenId)
{
	if (RootScreenId.IsNone())
	{
		// Non si inizializza niente: uno stack con radice vuota accetterebbe transizioni che non
		// producono mai un widget, cioe' fallirebbe in silenzio a ogni chiamata successiva.
		return ERTNavResult::InvalidScreen;
	}

	// ⚠️ **I widget del frontend precedente si buttano, e non e' un'ottimizzazione mancata.**
	// Questo subsystem sopravvive al cambio di livello **apposta** — e' cio' che permette a `ReturnMain` di
	// funzionare dopo una partita — ma i widget dentro `LiveWidgets` appartengono al **mondo in cui sono
	// stati costruiti**. Al secondo ingresso nel frontend (`Main Menu -> partita -> Main Menu`, il ciclo di
	// CP 46.5) `PresentWidget` trovava l'istanza vecchia, saltava `CreateWidget`, e chiamava
	// `AddToViewport` su un widget il cui mondo era stato smontato. Trovato in code review su PR #1264.
	//
	// ⚠️ **Non contraddice la cache dichiarata da CP 46.1**: quella vale fra `PushScreen` e `PopScreen`,
	// cioe' **dentro** una sessione. `InitializeFrontend` non e' una navigazione, e' l'inizio di una
	// sessione nuova — e `LiveWidgets` era l'unico stato che non veniva azzerato insieme allo stack.
	//
	// ⚠️ **`Bindings` NON si tocca**, ed e' la differenza con `Deinitialize`: `StartFrontendFrom` registra
	// e *poi* inizializza, quindi azzerarli qui butterebbe via le schermate appena dichiarate.
	// ⚠️ **`bInitialized` cade PRIMA dello smontaggio**, e non e' un riordino cosmetico: `DismissWidget`
	// esegue `RemoveFromParent` -> `NativeDestruct` -> l'evento Blueprint `Destruct`, e ogni funzione di
	// questo navigatore e' `BlueprintCallable`. Un widget che navigasse dal proprio `Destruct` rientrerebbe
	// in `SyncPresentation` **mentre `Stack` e' ancora quello della sessione vecchia**, ripresentando
	// widget che il `Reset()` subito dopo dimenticherebbe — lasciandoli a schermo senza piu' nessun
	// riferimento capace di rimuoverli. Con il flag abbassato, `SyncPresentation` e' un no-op.
	// Trovato in code review su PR #1272.
	bInitialized = false;
	DismissAllWidgets();

	// Il HUD segue la sessione come i widget delle schermate: una radice nuova non eredita il HUD della
	// precedente, e per la stessa ragione — l'istanza appartiene al mondo in cui e' stata costruita.
	// `EnterMatch` lo ripresenta subito dopo; l'ordine smonta-poi-monta e' quello di `SyncPresentation`.
	DismissMatchHud();

	Stack = FRTScreenStack(RootScreenId);
	bInitialized = true;
	SyncPresentation();
	return ERTNavResult::Ok;
}

void URTFrontendNavigator::DismissAllWidgets()
{
	// Le chiavi si copiano prima: `DismissWidget` non modifica la mappa, ma iterarla mentre si chiama
	// qualcosa che puo' rientrare nel navigatore e' il difetto che `PresentWidget` documenta gia'.
	TArray<FName> Live;
	LiveWidgets.GetKeys(Live);
	for (const FName& Id : Live)
	{
		DismissWidget(Id);
	}
	LiveWidgets.Reset();
}

void URTFrontendNavigator::RegisterScreen(FName ScreenId, TSoftClassPtr<UUserWidget> WidgetClass)
{
	if (!ScreenId.IsNone())
	{
		Bindings.Add(ScreenId, WidgetClass);
	}
}

bool URTFrontendNavigator::StartFrontend()
{
	// `LoadConfig` esplicito invece di affidarsi ai valori ereditati dal CDO: quando questa funzione viene
	// chiamata, cio' che conta e' cosa dice il `.ini` **adesso**, e una riga in piu' toglie la domanda.
	LoadConfig();
	return StartFrontendFrom(Screens);
}

bool URTFrontendNavigator::StartFrontendFrom(const TArray<FRTScreenBinding>& InScreens)
{
	const int32 Registered = RegisterScreens(InScreens);

	if (Registered == 0)
	{
		// ⚠️ Un avviso e non un rifiuto, e la scelta e' deliberata: senza binding la navigazione **funziona**
		// — lo stack si muove, `GetCurrentScreen()` risponde — e semplicemente non disegna niente. Rifiutare
		// qui renderebbe il frontend inavviabile in un test headless, che e' proprio il caso in cui i
		// `.uasset` non esistono e non devono esistere. Cio' che non deve succedere e' che nessuno se ne
		// accorga: uno schermo nero senza una riga di log e' indistinguibile da un difetto di rendering.
		UE_LOG(LogRT, Warning,
			TEXT("Frontend avviato senza schermate registrate: nessun `+Screens=` valido in ")
			TEXT("[/Script/RefactorTactics.RTFrontendNavigator], quindi lo schermo restera' vuoto"));
	}

	// Registrare **prima**, aprire dopo: `InitializeFrontend` presenta subito la radice, e con i binding
	// ancora assenti `SyncPresentation` uscirebbe alla prima riga lasciando lo stack corretto sopra uno
	// schermo vuoto.
	const ERTNavResult Opened = InitializeFrontend(RTScreenIds::Main);

	// ⚠️ **Le due condizioni si contano entrambe, e la prima stesura ne restituiva solo la seconda.**
	// Trovato in code review: con zero schermate registrate `InitializeFrontend` risponde comunque `Ok` —
	// lo stack e' legale senza binding, ed e' voluto da CP 46.1 — quindi il chiamante riceveva un successo
	// sopra uno schermo nero. Il conteggio veniva calcolato, loggato e **buttato**: esattamente il segnale
	// che l'header dichiara essere «l'unico» a distinguere un frontend vuoto da uno vivo.
	return Registered > 0 && Opened == ERTNavResult::Ok;
}

int32 URTFrontendNavigator::RegisterScreensFromConfig()
{
	LoadConfig();
	return RegisterScreens(Screens);
}

int32 URTFrontendNavigator::RegisterScreens(const TArray<FRTScreenBinding>& InScreens)
{
	int32 Registered = 0;

	// 🔴 **L'insieme e' locale alla chiamata, e la prima stesura interrogava `Bindings`.** Un duplicato e'
	// due righe **nella stessa dichiarazione**, non la stessa schermata dichiarata di nuovo piu' tardi:
	// contro la mappa persistente, il secondo avvio del frontend trovava tutto «gia' legato», restituiva 0,
	// e faceva rispondere `false` a `StartFrontend` su un menu aperto correttamente — accusando per giunta
	// il `.ini` di una duplicazione inesistente. Il difetto stava esattamente sul ciclo
	// `Main Menu -> partita -> Main Menu` di CP 46.5. Trovato in code review su PR #1272.
	TSet<FName> SeenInThisCall;

	for (const FRTScreenBinding& Binding : InScreens)
	{
		// Le due incompletezze si scartano insieme ma non sono lo stesso difetto: un id vuoto non e'
		// indirizzabile, una classe nulla lo e' e non disegna niente — che e' peggio, perche' produce una
		// navigazione che riesce sopra uno schermo immobile.
		if (Binding.ScreenId.IsNone() || Binding.WidgetClass.IsNull())
		{
			continue;
		}

		// ⚠️ **Il conteggio segue i binding, non le righe lette.** `RegisterScreen` fa `Bindings.Add`, che
		// **sovrascrive**: due righe con lo stesso `ScreenId` producono un binding solo. Contarle entrambe
		// faceva dire «registrate tutte» a un `.ini` in cui una schermata era sparita — trovato in code
		// review, ed e' il difetto che il valore di ritorno esiste per rendere impossibile.
		// Il duplicato **non e' fatale**: l'ultimo vince, che e' la regola dei `.ini` a strati e va lasciata
		// funzionare. Cio' che non deve fare e' passare in silenzio.
		bool bAlreadyDeclaredHere = false;
		SeenInThisCall.Add(Binding.ScreenId, &bAlreadyDeclaredHere);

		// Passa da `RegisterScreen` invece di scrivere in `Bindings`: il controllo sull'id resta in un
		// posto solo, come il filtro sui non-fatali di `URTErrorModalWidgetBase::ShowFor`.
		RegisterScreen(Binding.ScreenId, Binding.WidgetClass);

		if (bAlreadyDeclaredHere)
		{
			UE_LOG(LogRT, Warning,
				TEXT("Schermata '%s' dichiarata due volte: vince l'ultima riga, la precedente e' persa"),
				*Binding.ScreenId.ToString());
			continue;
		}

		++Registered;
	}

	return Registered;
}

TArray<FName> URTFrontendNavigator::GetRegisteredScreenIds() const
{
	// ⚠️ **L'ordine non e' dichiarato**, e chi legge non deve dipenderne: le chiavi arrivano da una `TMap`,
	// e il progetto vieta esplicitamente di appoggiarsi all'ordine di `TMap`/`TSet`. Serve a sapere *se* una
	// schermata c'e', non in che posizione.
	TArray<FName> Ids;
	Bindings.GetKeys(Ids);
	return Ids;
}

ERTNavResult URTFrontendNavigator::PushScreen(FName ScreenId)
{
	const ERTNavResult Result = Stack.PushScreen(ScreenId);
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::PopScreen()
{
	const ERTNavResult Result = Stack.PopScreen();
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::ShowModal(FName ModalId)
{
	const ERTNavResult Result = Stack.ShowModal(ModalId);
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::CloseModal()
{
	const ERTNavResult Result = Stack.CloseModal();
	if (Result == ERTNavResult::Ok)
	{
		SyncPresentation();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::ReturnMain()
{
	const ERTNavResult Result = Stack.ReturnMain();
	SyncPresentation();

	// 🔴 **L'azzeramento sta QUI, non solo in `PlayAgain`.** Il DoD descrive due uscite dal Result, e
	// `MAIN MENU` e' questa: senza la riga, un risultato sopravviveva al ritorno al menu e a tutta la
	// partita successiva avviata da `PLAY`, perche' `StartMatch` diretto non passa da `PlayAgain`.
	// Il commento del campo prometteva «vuoto finche' una partita non e' finita» e non era vero.
	// Trovato in review.
	//
	// ⚠️ **Dopo `SyncPresentation`, non prima**: lo smontaggio del widget esegue `NativeDestruct`, e un
	// widget che leggesse il proprio dato mentre si chiude deve vedere cio' che stava mostrando — e' la
	// stessa ragione per cui `ShowResult` scrive prima di presentare, applicata all'uscita.
	MatchResult = FRTMatchResultViewModel{};
	return Result;
}

ERTNavResult URTFrontendNavigator::StartMatch()
{
	if (MatchLevel.IsEmpty())
	{
		return RejectMatchStart(ERTStartupOutcome::MatchLevelUnset, TEXT("MatchLevel"));
	}

	// 🔴 **Una richiesta ancora pendente e' la prova che nessuno l'ha consumata.**
	//
	// E' il costo dichiarato di questa forma: il navigatore decide e qualcun altro esegue, quindi se
	// l'aggancio non viene collegato `PLAY` non fa nulla — e il difetto vivrebbe nell'ASSENZA di una
	// chiamata, che nessun grep trova e nessun gate vede.
	if (!PendingMatchLevel.IsEmpty())
	{
		return RejectMatchStart(ERTStartupOutcome::MatchRequestNotConsumed, PendingMatchLevel);
	}

	// Da qui in poi e' una RICHIESTA: chi ha il mondo la consuma e apre il livello. Il navigatore non tocca
	// la scena, e la partita resta allestita da `ARTGameMode` col formato spedito da C++ — nessun secondo
	// percorso di avvio, che e' il vincolo del DoD.
	PendingMatchLevel = MatchLevel;
	UE_LOG(LogRT, Log, TEXT("[RT] Avvio partita: chiesto il livello '%s'"), *MatchLevel);

	// ⚠️ **Dopo aver scritto la richiesta, non prima.** Un ascoltatore che consuma dentro il callback deve
	// trovarla: invertire le due righe darebbe un consumatore a mani vuote e una richiesta che resta li'.
	OnMatchRequested.Broadcast(MatchLevel);
	return ERTNavResult::Ok;
}

ERTNavResult URTFrontendNavigator::RejectMatchStart(ERTStartupOutcome Outcome, const FString& Detail)
{
	// ⛔ **Il motivo NON si compone qui.** `RTFrontendWidgets.h` lo dichiara: «Il widget non compone il
	// motivo. Non esiste un accessor che restituisca una `FString` libera da mostrare: si legge
	// `ERTStartupOutcome` e si chiede il testo a `DescribeOutcome`».
	//
	// Una prima stesura di CP 46.4 aveva un `GetLastMatchStartFailure()` che restituiva esattamente quella
	// FString libera — un canale d'errore parallelo a quello che CP 46.2 aveva progettato per impedirlo, e
	// che perdeva localizzazione, filtro sui non-fatali, split Shipping-safe e `PhaseWhenArmed`. Trovato in
	// code review, e questa funzione e' cio' che resta di quel percorso: un esito tipizzato e un dettaglio.
	UE_LOG(LogRT, Error, TEXT("[RT] Avvio partita rifiutato — %s (%s)"),
		*URTStartupReportLibrary::DescribeOutcome(Outcome).ToString(), *Detail);

	return ArmErrorModal(Outcome, Detail);
}

ERTNavResult URTFrontendNavigator::RejectReturnToMain(ERTStartupOutcome Outcome, const FString& Detail)
{
	// Stesso trattamento del rifiuto d'avvio, e per la stessa ragione: un `RETURN TO MAIN MENU` che non fa
	// niente e non dice niente e' il soft-lock che CP 46.1 chiama dead-end — con l'aggravante che qui il
	// giocatore e' dentro una partita e crede di averla lasciata.
	UE_LOG(LogRT, Error, TEXT("[RT] Ritorno al menu rifiutato — %s (%s)"),
		*URTStartupReportLibrary::DescribeOutcome(Outcome).ToString(), *Detail);

	return ArmErrorModal(Outcome, Detail);
}

ERTNavResult URTFrontendNavigator::ArmErrorModal(ERTStartupOutcome Outcome, const FString& Detail)
{
	const ERTNavResult ModalResult = ShowModal(RTScreenIds::ErrorModal);
	if (ModalResult != ERTNavResult::Ok)
	{
		// ⚠️ Il rifiuto del modale non si ingoia: `RTScreenStack.h` dice «ogni rifiuto porta un motivo, un
		// `Blocked` silenzioso e' un difetto». Se il modale non si e' aperto, chi chiama deve saperlo —
		// altrimenti «rifiutato e riportato» e «rifiutato e ingoiato» hanno lo stesso valore di ritorno.
		UE_LOG(LogRT, Error,
			TEXT("[RT] ...e il modale d'errore non si e' aperto: la causa non e' a schermo."));
		return ModalResult;
	}

	// Il widget esiste solo DOPO `ShowModal`, che lo presenta: e' `PresentWidget` a costruirlo.
	if (URTErrorModalWidgetBase* Modal = Cast<URTErrorModalWidgetBase>(FindLiveWidget(RTScreenIds::ErrorModal)))
	{
		Modal->ShowForOutcome(Outcome, Detail);
	}
	else
	{
		// Il binding manca o non e' un modale d'errore: senza questa riga il menu resterebbe disabilitato
		// sotto un modale invisibile — il soft-lock trovato in code review.
		UE_LOG(LogRT, Error,
			TEXT("[RT] ...e il modale d'errore non e' armabile: controlla il binding 'Error' in "
				 "DefaultGame.ini. La schermata sotto resta disabilitata."));
	}

	return ERTNavResult::InvalidScreen;
}

ERTNavResult URTFrontendNavigator::ShowResult(const FRTMatchResult& InResult, const FRTMatchState& InState)
{
	// 🔴 **Il risultato si scrive PRIMA del push, e la versione precedente faceva il contrario.**
	// La motivazione era «nessuno legge durante il push» — ed e' falsa, con la confutazione gia' scritta
	// in questo file: `PushScreen` chiama `SyncPresentation` in modo sincrono, e il commento di
	// `PresentWidget` dice che sia `CreateWidget` (`NativeOnInitialized`) sia `AddToViewport`
	// (`NativeConstruct` e l'evento Blueprint `Construct`) «possono richiamare il navigatore — tutte le
	// sue funzioni sono `BlueprintCallable`». `GetMatchResult()` e' `BlueprintPure`, ed e' l'unico canale
	// che il widget ha: leggerlo nel proprio `Construct` e' il modo naturale di popolare i testi.
	// Scrivendo dopo, quella lettura vedeva `InProgress` / round 0 alla PRIMA apertura. Trovato in review.
	//
	// ⛔ **Questo ordine NON e' coperto da un test, e il perche' e' misurato.** Servirebbe un widget sonda
	// che legga nel proprio `NativeOnInitialized`, e in un test headless quel momento non arriva mai:
	// `PresentWidget` chiama `CreateWidget(GI, ...)`, il widget viene costruito — `FindLiveWidget` lo
	// trova, della classe giusta — ma nasce senza mondo e `Initialize()` esce prima dell'hook. Provato con
	// `MakeNavigator`, con `MakeFrontendWorld`, e legando a mano `FWorldContext::OwningGameInstance`:
	// `mondo=NO`, `BirthCount=0` in tutti e tre. La correzione resta perche' la premessa vecchia e'
	// **confutata leggendo il codice** — `SyncPresentation` e' sincrona e `PresentWidget` lo documenta —
	// non perche' un verde lo dimostri. La prova sta a `PIE-V01-FRONTEND-RESULT`, che e' da creare.
	const FRTMatchResultViewModel Previous = MatchResult;
	MatchResult = FRTMatchResultViewModel::From(InResult, InState);

	const ERTNavResult NavResult = PushScreen(RTScreenIds::Result);
	if (NavResult != ERTNavResult::Ok)
	{
		// ⚠️ Il rollback tiene la garanzia che l'ordine sbagliato voleva: un risultato non deve restare
		// dietro una schermata mai aperta, o la partita dopo lo rileggerebbe come fresco.
		MatchResult = Previous;
		UE_LOG(LogRT, Warning, TEXT("[RT] Fine partita: il Result non si e' aperto (%s), risultato non registrato."),
			*UEnum::GetValueAsString(NavResult));
		return NavResult;
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fine partita al round %d: %s"),
		MatchResult.RoundNumber, *UEnum::GetValueAsString(MatchResult.Outcome));
	return ERTNavResult::Ok;
}

ERTNavResult URTFrontendNavigator::PlayAgain()
{
	// L'ordine conta: prima si smonta il Result, poi si chiede la partita. Al contrario, un `StartMatch`
	// rifiutato lascerebbe l'utente sul Result con un modale d'errore sopra — che e' il posto giusto per
	// vederlo — ma lo stack sarebbe gia' stato svuotato da sotto.
	const ERTNavResult Cleared = ReturnMain();
	if (Cleared != ERTNavResult::Ok)
	{
		return Cleared;
	}

	// (il risultato l'ha gia' azzerato `ReturnMain`, che e' il punto comune alle due uscite)
	return StartMatch();
}

// ======================================================================================================
// CP 46.6 · Pause e smontaggio (`#941`)
// ======================================================================================================

ERTNavResult URTFrontendNavigator::EnterMatch()
{
	// E' `InitializeFrontend` con un'altra radice, e non una terza via: la partita e' l'inizio di una
	// sessione di flow come lo e' il menu — widget della precedente buttati, stack nuovo, radice legale.
	const ERTNavResult Result = InitializeFrontend(RTScreenIds::Match);
	if (Result == ERTNavResult::Ok)
	{
		// Il HUD arriva **dopo** la radice, ed e' l'unico ordine possibile: `InitializeFrontend` ha appena
		// smontato tutto, e presentarlo prima lo farebbe cadere nello smontaggio della sessione vecchia.
		PresentMatchHud();
	}
	return Result;
}

ERTNavResult URTFrontendNavigator::ShowPause()
{
	// ⚠️ Non e' idempotente di proposito: due `Pause` impilate sarebbero due voci di stack e **un solo
	// widget**, e il `RESUME` ne toglierebbe una lasciando l'altra a coprire la partita.
	if (IsPauseOpen())
	{
		return ERTNavResult::ScreenIsAlreadyOnStack;
	}

	// ⛔ **Qui finisce cio' che la pausa fa alla partita: niente.** Nessun `SetPause`, nessuna dilatazione
	// del tempo, nessun flag nel `TurnManager`. E' il vincolo offline-only del DoD, e resta verificabile
	// con un grep invece che con una promessa.
	return PushScreen(RTScreenIds::Pause);
}

ERTNavResult URTFrontendNavigator::ResumeMatch()
{
	if (!IsPauseOpen())
	{
		return ERTNavResult::NoPauseOpen;
	}

	// I modali aperti sopra vanno chiusi **prima**: con un modale aperto `PopScreen` risponde
	// `BlockedByModal`, e il giocatore resterebbe dentro il modale con la partita sotto — il dead-end.
	while (IsModalOpen())
	{
		const ERTNavResult Closed = CloseModal();
		if (Closed != ERTNavResult::Ok)
		{
			return Closed;
		}
	}

	// Si risale finche' la pausa non e' uscita dallo stack: il `SETTINGS` aperto DALLA pausa se ne va con
	// lei. Senza questo giro, `RESUME` premuto da dentro le impostazioni sarebbe un `Back` travestito, e
	// il giocatore tornerebbe alla pausa credendo di aver ripreso la partita.
	//
	// ⚠️ **Non puo' ciclare all'infinito**: `PopScreen` o riduce la profondita' o restituisce un rifiuto
	// che esce di qui — e sulla radice restituisce `BlockedAtRoot`.
	ERTNavResult Result = ERTNavResult::Ok;
	while (IsPauseOpen())
	{
		Result = PopScreen();
		if (Result != ERTNavResult::Ok)
		{
			return Result;
		}
	}

	return Result;
}

bool URTFrontendNavigator::IsPauseOpen() const
{
	// ⚠️ **Contenuta nello stack, non in cima.** Da `Pause` si apre `Settings`, e in quel momento la
	// partita sotto deve restare ugualmente senza puntatore: con il confronto sulla cima, aprire le
	// impostazioni avrebbe restituito l'input al mondo.
	return Stack.GetScreens().Contains(RTScreenIds::Pause);
}

ERTNavResult URTFrontendNavigator::OpenSettings()
{
	// ⚠️ **La stessa guardia di `ShowPause`, e per lo stesso motivo**: la presentazione tiene **un widget
	// per `FName`**, quindi un doppio click su `SETTINGS` impilerebbe due voci per un solo widget — e il
	// primo `BACK` ne toglierebbe una lasciando le impostazioni a schermo, cioe' un pulsante che sembra
	// rotto. Trovato in code review sulla PR #1304.
	//
	// ⛔ **La deduplica NON va messa in `FRTScreenStack::PushScreen`**, benche' sia la sede piu' alta: lo
	// stack non deduplica **per decisione** — *«`Settings` aperto dal Main e `Settings` aperto dalla Pause
	// sono la stessa schermata con due ritorni diversi»* — e `Frontend.SameScreenPushedTwiceKeepsTwoReturns`
	// lo asserisce. Cio' che va impedito e' la ripetizione **consecutiva**, che non ha un secondo ritorno da
	// conservare perche' il ritorno sarebbe a se stessa.
	if (Stack.CurrentScreen() == RTScreenIds::Settings)
	{
		return ERTNavResult::ScreenIsAlreadyOnStack;
	}

	return PushScreen(RTScreenIds::Settings);
}

ERTNavResult URTFrontendNavigator::RequestReturnToMainMenu()
{
	if (FrontendLevel.IsEmpty())
	{
		return RejectReturnToMain(ERTStartupOutcome::FrontendLevelUnset, TEXT("FrontendLevel"));
	}

	// 🔴 Una richiesta ancora pendente e' la prova che nessuno l'ha consumata — gemella della guardia di
	// `StartMatch`, e nata dallo stesso difetto: il consumatore vive in un altro file e puo' non esserci.
	if (!PendingFrontendLevel.IsEmpty())
	{
		return RejectReturnToMain(ERTStartupOutcome::FrontendReturnNotConsumed, PendingFrontendLevel);
	}

	// «Tornare alla radice» ha un solo significato e un solo posto, azzeramento del risultato compreso.
	//
	// ✅ **Ed e' sicuro PERCHE' `EnterMatch` ha messo `Match` come radice.** Prima non lo era: con la
	// radice a `Main`, `SyncPresentation` presentava il **Main Menu sopra la partita viva** nel frame che
	// precede il cambio di livello — il difetto trovato in code review sulla PR #1304. Adesso la radice in
	// partita e' una schermata **senza widget**, quindi tornarci smonta la pausa e non disegna niente; dal
	// menu, dove la radice e' `Main`, presenta il menu, che e' giusto.
	//
	// ⛔ **E NON si smonta a mano.** Una revisione intermedia aveva sostituito questa riga con
	// `bInitialized = false; DismissAllWidgets();`, e produceva due difetti **peggiori** di quello che
	// voleva evitare — entrambi colti dai test, non dal ragionamento:
	//
	//   (a) lo stack restava dov'era, quindi con `Pause` ancora dentro l'input di gioco restava bloccato
	//       **senza niente a schermo** se nessuno consumava la richiesta: lo stesso dead-end di F2, un
	//       livello piu' in la';
	//   (b) con `bInitialized` a `false`, `RejectReturnToMain` non poteva piu' presentare il modale
	//       d'errore — `SyncPresentation` esce alla prima riga — e il rifiuto diventava **muto**, che e'
	//       esattamente cio' che quel modale esiste per impedire.
	const ERTNavResult Cleared = ReturnMain();
	if (Cleared != ERTNavResult::Ok)
	{
		return Cleared;
	}

	PendingFrontendLevel = FrontendLevel;
	UE_LOG(LogRT, Log, TEXT("[RT] Ritorno al menu: chiesto il livello '%s'"), *FrontendLevel);

	// ⚠️ Dopo aver scritto la richiesta, non prima: un ascoltatore che consuma dentro il callback deve
	// trovarla.
	OnReturnToFrontendRequested.Broadcast(FrontendLevel);
	return ERTNavResult::Ok;
}

FString URTFrontendNavigator::ConsumePendingFrontendLevel()
{
	// `MoveTemp` come il gemello quattro righe piu' su: due primitive diverse per lo stesso move-and-clear
	// costringono chi legge a chiedersi se la differenza significhi qualcosa.
	return MoveTemp(PendingFrontendLevel);
}

FString URTFrontendNavigator::ConsumePendingMatchLevel()
{
	// `MoveTemp` svuota gia' la sorgente: un `Reset()` dopo sarebbe codice morto, e farebbe dubitare quale
	// delle due righe fa il lavoro.
	return MoveTemp(PendingMatchLevel);
}

ERTNavResult URTFrontendNavigator::BackFromError(ERTLoadPhase PhaseWhenArmed)
{
	// A partita viva si **smonta**. `ReturnMain` porta via anche i modali, quindi non serve chiuderli
	// prima — ed e' provato da `ReturnMainClearsScreensAndModals`, che esiste dal CP 46.1.
	if (PhaseWhenArmed == ERTLoadPhase::Ready)
	{
		return ReturnMain();
	}

	// Durante il loading non e' stato costruito niente: si torna alla schermata precedente. Ma il modale
	// va chiuso **prima**, o `PopScreen` risponde `BlockedByModal` e il giocatore resta dentro il modale.
	if (IsModalOpen())
	{
		const ERTNavResult Closed = CloseModal();
		if (Closed != ERTNavResult::Ok)
		{
			// Si propaga invece di proseguire: un pop dopo una chiusura fallita agirebbe su uno stato che
			// non e' quello che si crede.
			return Closed;
		}
	}

	// ⚠️ `BlockedAtRoot` **si propaga e non e' un difetto**: se il modale d'errore compare gia' sulla
	// radice non c'e' nulla sotto cui tornare, e il chiamante deve saperlo invece di vedersi un `Ok` che
	// non ha mosso niente. Il dead-end non esiste comunque, perche' il modale e' stato chiuso qui sopra.
	return PopScreen();
}

UUserWidget* URTFrontendNavigator::FindLiveWidget(FName ScreenId) const
{
	const TObjectPtr<UUserWidget>* Found = LiveWidgets.Find(ScreenId);
	return Found ? Found->Get() : nullptr;
}

void URTFrontendNavigator::PresentWidget(FName ScreenId, int32 ZOrder)
{
	// Una schermata senza binding NON e' un errore: lo stack si e' gia' mosso e la navigazione e' valida.
	// Manca solo cosa disegnare — il caso normale di un test headless.
	const TSoftClassPtr<UUserWidget>* Binding = Bindings.Find(ScreenId);
	if (!Binding || Binding->IsNull())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	// 🔴 **Un puntatore locale, non un riferimento dentro la `TMap`.** `FindOrAdd` restituisce un
	// riferimento al valore, e sia `CreateWidget` (che esegue `NativeOnInitialized`) sia `AddToViewport`
	// (che esegue `NativeConstruct` e l'evento Blueprint `Construct`) possono richiamare il navigatore —
	// tutte le sue funzioni sono `BlueprintCallable`. Una rientranza in `PresentWidget` fa `FindOrAdd` su
	// `LiveWidgets`, il rehash sposta i valori, e il riferimento tenuto qui punta a memoria liberata.
	// Trovato in code review; il costo della correzione e' zero e il difetto sarebbe stato intermittente.
	UUserWidget* Widget = FindLiveWidget(ScreenId);
	if (!Widget)
	{
		UClass* WidgetClass = Binding->LoadSynchronous();
		if (!WidgetClass)
		{
			// ⚠️ **Un binding che non risolve e' diverso da un binding assente**, e fino alla code review
			// uscivano dalla stessa porta in silenzio. L'assenza e' normale — un test headless non ha
			// `.uasset` — ma un percorso *dichiarato* che non carica e' quasi sempre un nome sbagliato nel
			// `.ini`, ed e' la trappola che `guida-frontend-main-menu-umg.md` §2 chiama «quella che costa di
			// piu'»: la registrazione riesce, la navigazione risponde `Ok`, e lo schermo resta vuoto.
			// L'unico segnale era un `LogUObjectGlobals: Failed to find object` che non nomina ne' la
			// schermata ne' il file da correggere.
			UE_LOG(LogRT, Warning,
				TEXT("Schermata '%s': la classe widget '%s' non si carica — controlla il percorso in ")
				TEXT("[/Script/RefactorTactics.RTFrontendNavigator] di DefaultGame.ini. Lo schermo restera' vuoto"),
				*ScreenId.ToString(), *Binding->ToString());
			return;
		}

		// ⚠️ **L'unico `CreateWidget` del codebase, insieme a quelli di `DismissWidget`.** Se ne compare uno
		// dentro un widget, il DoD di CP 46.1 e' violato e il `grep` lo dice.
		Widget = CreateWidget<UUserWidget>(GI, WidgetClass);
		if (!Widget)
		{
			return;
		}

		// La mappa si scrive **dopo** la costruzione, e da qui in poi non si tengono riferimenti al suo
		// interno: qualunque rientranza trova una mappa coerente.
		LiveWidgets.Add(ScreenId, Widget);
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}
}

void URTFrontendNavigator::PresentMatchHud()
{
	// Un binding assente e' **normale** — un test headless non ha `.uasset` — e non e' un errore da
	// loggare. E' la stessa distinzione che `PresentWidget` fa fra «assente» e «dichiarato ma non carica».
	if (MatchHudWidgetClass.IsNull())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	if (!MatchHudWidget)
	{
		UClass* WidgetClass = MatchHudWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			// Un percorso **dichiarato** che non carica e' quasi sempre un nome sbagliato nel `.ini`, e
			// senza questa riga il sintomo sarebbe una partita senza HUD e nessuna spiegazione — cioe' un
			// fallimento indistinguibile da un HUD non ancora costruito.
			UE_LOG(LogRT, Warning,
				TEXT("HUD di partita: la classe '%s' non si carica — controlla MatchHudWidgetClass in ")
				TEXT("[/Script/RefactorTactics.RTFrontendNavigator] di DefaultGame.ini. ")
				TEXT("La partita restera' senza Screen HUD"),
				*MatchHudWidgetClass.ToString());
			return;
		}

		MatchHudWidget = CreateWidget<UUserWidget>(GI, WidgetClass);
		if (!MatchHudWidget)
		{
			return;
		}
	}

	if (!MatchHudWidget->IsInViewport())
	{
		MatchHudWidget->AddToViewport(MatchHudZ);
	}
}

void URTFrontendNavigator::DismissMatchHud()
{
	if (MatchHudWidget)
	{
		if (MatchHudWidget->IsInViewport())
		{
			MatchHudWidget->RemoveFromParent();
		}

		// 🔴 **Il puntatore si azzera, e non e' un'ottimizzazione mancata.** L'istanza appartiene al mondo
		// in cui e' stata costruita: riusarla dopo un cambio di livello chiama `AddToViewport` su un mondo
		// smontato. E' il difetto di PR #1264, gia' pagato una volta su `LiveWidgets`.
		MatchHudWidget = nullptr;
	}
}

void URTFrontendNavigator::DismissWidget(FName ScreenId)
{
	TObjectPtr<UUserWidget>* Slot = LiveWidgets.Find(ScreenId);
	if (Slot && *Slot && (*Slot)->IsInViewport())
	{
		(*Slot)->RemoveFromParent();
	}
}

void URTFrontendNavigator::SyncPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	// Cio' che deve stare a schermo: la sola schermata in cima, piu' tutti i modali aperti.
	//
	// ⚠️ **Solo la cima, non l'intero stack.** Le schermate sotto restano nello stack — sono il percorso di
	// ritorno — ma non si disegnano: tenerle a schermo significherebbe che un `Back` cambia lo Z-order
	// invece del contenuto, e che il giocatore vede due schermate sovrapposte.
	TSet<FName> ShouldBeVisible;
	ShouldBeVisible.Add(Stack.CurrentScreen());
	for (const FName& Modal : Stack.GetModals())
	{
		ShouldBeVisible.Add(Modal);
	}

	// Prima si smonta cio' che non serve piu', poi si monta: l'ordine inverso lascerebbe due schermate a
	// schermo per un frame.
	TArray<FName> Live;
	LiveWidgets.GetKeys(Live);
	for (const FName& Id : Live)
	{
		if (!ShouldBeVisible.Contains(Id))
		{
			DismissWidget(Id);
		}
	}

	PresentWidget(Stack.CurrentScreen(), ScreenZBase);

	int32 ModalZ = ModalZBase;
	for (const FName& Modal : Stack.GetModals())
	{
		PresentWidget(Modal, ModalZ++);
	}

	// Solo cio' che sta in CIMA riceve input: il modale piu' recente se ce n'e' uno, altrimenti la
	// schermata corrente. Tutto il resto resta **visibile e inerte**.
	//
	// E' `bIsEnabled` e non `Visibility`: nasconderlo lo farebbe sparire da sotto il modale, mentre deve
	// restare leggibile — un modale che oscura il contesto che sta interrompendo e' peggio di nessun modale.
	//
	// 🔴 **Questa parte disabilitava la sola `CurrentScreen()`**, quindi con due modali impilati quello
	// sotto restava **cliccabile** attraverso quello sopra. Trovato in code review: il DoD dice «il modale
	// disabilita cio' che sta sotto», e un modale sotto un altro modale *sta sotto*.
	const FName InteractiveId = Stack.IsModalOpen() ? Stack.TopModal() : Stack.CurrentScreen();

	for (const TPair<FName, TObjectPtr<UUserWidget>>& Pair : LiveWidgets)
	{
		if (UUserWidget* Widget = Pair.Value)
		{
			if (Widget->IsInViewport())
			{
				Widget->SetIsEnabled(Pair.Key == InteractiveId);
			}
		}
	}

	// Il HUD non e' in `LiveWidgets`, quindi il ciclo sopra non lo vede — ma la regola vale anche per lui:
	// **visibile e inerte** quando qualcosa lo copre. E' interattivo solo quando la partita e' davvero in
	// cima, cioe' nessun modale aperto e nessuna schermata sopra di essa.
	//
	// ⚠️ E' **meta'** della precedenza `Modal/Reaction UI > HUD > world tactical hit`, non tutta: qui si
	// decide se il HUD riceve input, non se un click che lo attraversa arriva al mondo. Quella meta' e' di
	// CP 11.8 (#705), e ha bisogno di hitbox che il Canvas oggi non registra.
	// 🔴 **Niente guardia `IsInViewport()` qui, a differenza del ciclo sopra.** Copiarla da li' e' stata la
	// prima stesura, e `MatchHudIsInertUnderThePause` l'ha bocciata: in un test headless la
	// `UGameInstance` non ha viewport, `AddToViewport` non presenta nulla, e con quella guardia
	// `SetIsEnabled` non veniva chiamato **mai** — il widget restava al default `true` e la regola era vera
	// solo in gioco, cioe' verificabile solo aprendo l'Editor.
	//
	// `SetIsEnabled` su un widget non presentato e' innocuo: scrive un flag che avra' effetto quando il
	// widget entra in viewport. Il costo di toglierla e' zero, e in cambio la regola diventa una proprieta'
	// misurabile invece di una promessa.
	if (MatchHudWidget)
	{
		MatchHudWidget->SetIsEnabled(
			!Stack.IsModalOpen() && Stack.CurrentScreen() == RTScreenIds::Match);
	}
}

void URTFrontendNavigator::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 🔴 **La configurazione si legge QUI, e non solo in `StartFrontend`.** Finche' `LoadConfig` stava
	// unicamente in `StartFrontend` e `RegisterScreensFromConfig`, ogni property `Config` di questa classe
	// esisteva **solo per chi arrivava dal menu**.
	//
	// Il difetto si vedeva sul HUD di partita (CP 11.7): avviando PIE direttamente su `L_HexArena` — il
	// workflow di `PIE-HEXPLAY-*` — si passa solo da `ARTGameMode::BeginPlay` -> `EnterMatch()`, quindi
	// `MatchHudWidgetClass` restava vuota, `PresentMatchHud` usciva sulla prima riga e a schermo non
	// compariva niente. **Senza una riga di log**, perche' un binding assente e' un caso normale: il
	// silenzio era corretto per il codice e indistinguibile da un difetto per chi guardava.
	//
	// ⚠️ Non basta il CDO: il commento di `StartFrontend` lo dichiara gia' — `LoadConfig` esplicito
	// **invece** di affidarsi ai valori ereditati. Qui vale per lo stesso motivo, un livello piu' in alto.
	LoadConfig();

	// 🔴 **Il confine del mondo, non un chiamante alla volta.** `FindLiveWidget` documentava il difetto e
	// indicava questa correzione: i widget vivi appartengono al mondo che li ha costruiti, e `ReturnMain`,
	// `PushScreen` e `ShowModal` raggiungono `PresentWidget` senza passare da `InitializeFrontend`. Con
	// `RETURN TO MAIN MENU` il mondo cambia a ogni ritorno, quindi la cache stantia smette di essere
	// un'ipotesi e diventa il percorso normale.
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &URTFrontendNavigator::HandleWorldCleanup);
}

void URTFrontendNavigator::HandleWorldCleanup(UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
{
	// ⚠️ **Solo i mondi di QUESTA sessione.** `OnWorldCleanup` e' globale: in editor scatta anche per mondi
	// che con questa `GameInstance` non c'entrano — un mondo di test, una preview, un'altra sessione PIE.
	// Senza il filtro, chiudere una finestra qualsiasi svuoterebbe il menu vivo.
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// Non e' una navigazione: lo stack resta com'e'. Si butta cio' che e' legato al mondo, e chi apre il
	// livello successivo ricostruira' da `InitializeFrontend`.
	//
	// ⚠️ **`bInitialized` cade PRIMA, come in `InitializeFrontend`**, e la prima stesura lo saltava —
	// trovato in code review sulla PR #1304, ed e' lo stesso difetto che quella funzione documenta come
	// *«trovato in code review su PR #1272»*: `DismissWidget` esegue `RemoveFromParent` -> `NativeDestruct`
	// -> l'evento Blueprint `Destruct`, e ogni funzione di questo navigatore e' `BlueprintCallable`. Un
	// widget che navigasse da li' rientrerebbe in `SyncPresentation` **dentro un mondo in `CleanupWorld`**,
	// creando un widget che il `LiveWidgets.Reset()` subito dopo dimenticherebbe.
	bInitialized = false;
	DismissAllWidgets();
}

void URTFrontendNavigator::Deinitialize()
{
	// Prima di tutto il resto: un delegate globale che punta a un subsystem morto e' un crash differito.
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

	// ⚠️ Ultima rete: una richiesta che sopravvive alla sessione non e' stata consumata da nessuno. Arriva
	// tardi per correggere qualcosa, ma lascia una traccia invece del nulla.
	if (!PendingMatchLevel.IsEmpty())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Il frontend si chiude con una richiesta di partita mai consumata: '%s'. "
				 "Il consumatore di ConsumePendingMatchLevel non e' collegato."), *PendingMatchLevel);
	}

	// ⚠️ **La stessa rete per il verso opposto, e mancava.** La prima stesura di CP 46.6 controllava solo
	// `PendingMatchLevel`: una richiesta di RITORNO mai consumata sarebbe uscita dalla sessione in silenzio,
	// cioe' proprio il difetto — un aggancio scollegato — che questa diagnostica esiste per nominare.
	// Trovato da un test rosso, che dichiarava un messaggio che nessuno emetteva.
	if (!PendingFrontendLevel.IsEmpty())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Il frontend si chiude con un ritorno al menu mai consumato: '%s'. "
				 "Il consumatore di ConsumePendingFrontendLevel non e' collegato."), *PendingFrontendLevel);
	}

	// Lo stesso smontaggio di `InitializeFrontend`, in un posto solo: le due sedi divergerebbero al primo
	// widget che richiede un passo di pulizia in piu'.
	//
	// ⚠️ **E nello stesso ORDINE**: il flag cade prima dello smontaggio, non dopo. Qui era invertito, e la
	// finestra di rientranza era la stessa di `HandleWorldCleanup` — un `Destruct` che naviga mentre
	// `bInitialized` e' ancora `true`.
	bInitialized = false;
	DismissAllWidgets();
	Bindings.Reset();

	Super::Deinitialize();
}
