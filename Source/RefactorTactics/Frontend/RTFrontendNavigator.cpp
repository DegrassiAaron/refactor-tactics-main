#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendWidgets.h"   // URTErrorModalWidgetBase: il modale si ARMA, non si mostra e basta
#include "Frontend/RTStartupReport.h"    // DescribeOutcome: il testo del motivo vive li'

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
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
}

void URTFrontendNavigator::Deinitialize()
{
	// ⚠️ Ultima rete: una richiesta che sopravvive alla sessione non e' stata consumata da nessuno. Arriva
	// tardi per correggere qualcosa, ma lascia una traccia invece del nulla.
	if (!PendingMatchLevel.IsEmpty())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Il frontend si chiude con una richiesta di partita mai consumata: '%s'. "
				 "Il consumatore di ConsumePendingMatchLevel non e' collegato."), *PendingMatchLevel);
	}

	// Lo stesso smontaggio di `InitializeFrontend`, in un posto solo: le due sedi divergerebbero al primo
	// widget che richiede un passo di pulizia in piu'.
	DismissAllWidgets();
	Bindings.Reset();
	bInitialized = false;

	Super::Deinitialize();
}
