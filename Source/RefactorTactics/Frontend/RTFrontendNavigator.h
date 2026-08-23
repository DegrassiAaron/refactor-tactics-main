#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Frontend/RTScreenStack.h"
#include "Frontend/RTMatchResultViewModel.h"
// Per `ERTLoadPhase`, che `BackFromError` prende come parametro. E' l'unico punto in cui la navigazione
// tocca un tipo d'avvio, e la direzione della dipendenza e' voluta: il flow legge un dato dello startup,
// mai il contrario — `RTStartupReport.h` non sa che esista un navigatore.
#include "Frontend/RTStartupReport.h"
#include "RTFrontendNavigator.generated.h"

class UUserWidget;

/**
 * Una schermata dichiarata in configurazione: **il nome, e cosa disegnarlo** (CP 46.3, `#938`).
 *
 * ⚠️ **Questo tipo era stato rimosso in code review a CP 46.1, ed e' tornato adesso — non per
 * ripensamento, ma alla condizione che quella rimozione aveva scritto.** La nota diceva: *«era una
 * `USTRUCT` con `ScreenId` + `WidgetClass` e zero consumatori […] Quando servira' un array configurabile
 * da `.ini` o da data asset, si riscrive insieme al suo lettore»*. Il lettore adesso c'e' ed e'
 * `RegisterScreensFromConfig`, che senza questo tipo non avrebbe niente da leggere: `UPROPERTY(Config)`
 * sa deserializzare un array di `USTRUCT`, non una `TMap`.
 *
 * ⚠️ **Niente `EditAnywhere`, e la differenza rispetto alla prima stesura e' il punto**: questi campi si
 * scrivono in `DefaultGame.ini`, non in un pannello. Esporli all'editor rifarebbe l'errore per cui il tipo
 * era stato tolto — sembrare il modo previsto di dichiarare le schermate senza esserlo.
 */
USTRUCT()
struct FRTScreenBinding
{
	GENERATED_BODY()

	/** Il nome della schermata. Vuoto significa **scartata**: un id assente non e' indirizzabile. */
	UPROPERTY()
	FName ScreenId;

	/**
	 * La classe del widget da presentare.
	 *
	 * ⚠️ `TSoftClassPtr` e non `TSubclassOf`: registrare una schermata **non deve caricare** il suo
	 * `.uasset`. E' cio' che permette a `RegisterScreens` di girare in un test headless prima che i
	 * `WBP_RT_*` esistano — e i binari sono lavoro d'editor, quindi quel «prima» dura giorni, non minuti.
	 */
	UPROPERTY()
	TSoftClassPtr<UUserWidget> WidgetClass;
};

/**
 * `PLAY` e' stato premuto e la richiesta e' PENDENTE: chi ha il mondo la consuma e apre il livello.
 *
 * ⚠️ **L'ordine e' parte del contratto**: il broadcast avviene DOPO che `PendingMatchLevel` e' scritto,
 * cosi' un ascoltatore che consuma dentro il callback la trova. E' verificato, perche' l'ordine inverso
 * darebbe un consumatore che non trova nulla e una richiesta che resta li' — cioe' il difetto che la
 * guardia di `StartMatch` segnala al `PLAY` successivo.
 *
 * ⛔ **Serve perche' il consumatore non puo' SAPERE quando.** Senza annuncio dovrebbe interrogare lo stato
 * a ogni frame, e `CLAUDE.md` vieta il Tick per decidere sequencing.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTOnMatchRequested, const FString&, LevelName);

/**
 * **L'unico owner del flow del frontend** (CP 46.1, #936).
 *
 * Possiede lo stack logico (`FRTScreenStack`) e ne traduce le transizioni in widget. E' l'unico punto del
 * codebase autorizzato a chiamare `CreateWidget`, `AddToViewport` e `RemoveFromParent` — ed e' il criterio
 * **verificabile** del DoD, non un'intenzione:
 *
 *     grep -rn "AddToViewport\|RemoveFromParent\|CreateWidget" Source/
 *
 * non deve produrre occorrenze fuori da questo file. La baseline era **zero** in tutto `Source/`
 * (misurata su `5530bd03`): questa classe e' la prima a introdurle, e deve restare la sola.
 *
 * ## Perche' un `UGameInstanceSubsystem`
 *
 * Perche' il frontend deve **sopravvivere al cambio di livello**: `Main Menu -> partita -> Main Menu` e' il
 * ciclo di E46, e un owner del flow che muore col livello perderebbe lo stack proprio nel momento in cui
 * serve a tornare indietro. Un `AHUD` o un `APlayerController` non vanno bene per la stessa ragione — sono
 * per-livello e per-giocatore, mentre il back stack e' del **gioco**.
 *
 * ## Cosa NON fa
 *
 * - **Non decide la precedenza dell'input.** `ERTPointerContext::Modal` e la precedenza
 *   `Modal/Reaction UI > HUD > world tactical hit` sono di **CP 11.8**, e questo navigatore non li tocca:
 *   e' il vincolo esplicito di #936. Qui si decide *quale schermata e' visibile*, li' *chi consuma un
 *   click in partita*.
 * - **Non contiene regole di gioco.** Nessuna schermata legge o scrive lo stato del resolver.
 */
UCLASS(Config = Game)
class REFACTORTACTICS_API URTFrontendNavigator : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * **L'unica cosa che il GameMode del frontend deve sapere** (CP 46.3): registra i binding dichiarati
	 * in configurazione e apre la radice.
	 *
	 * ⚠️ **L'ordine e' la ragione per cui questa funzione esiste**, invece di lasciare due chiamate dentro
	 * un `BeginPlay`. `InitializeFrontend` presenta la radice **subito**: se i binding non fossero ancora
	 * arrivati, `SyncPresentation` uscirebbe alla prima riga e lascerebbe lo stack corretto sopra uno
	 * schermo vuoto — di nuovo un fallimento indistinguibile dal successo. Dentro un `BeginPlay`
	 * quell'ordine sarebbe una convenzione da ricordare; qui e' una riga sola e un test lo verifica.
	 *
	 * ⚠️ **Non la chiama il subsystem da se'.** Un `UGameInstanceSubsystem` nasce con la `GameInstance`,
	 * cioe' su **ogni** mappa: auto-avviarsi metterebbe il Main Menu sopra una partita in corso. Chi apre
	 * il frontend e' la mappa del frontend, tramite `ARTFrontendGameMode`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	bool StartFrontend();

	/**
	 * Avvia il frontend da un **elenco esplicito** invece che dalla configurazione. E' il corpo di
	 * `StartFrontend`, separato dalla lettura del `.ini`.
	 *
	 * ⚠️ **Non e' `BlueprintCallable`, e la prima stesura ci aveva provato.** L'avevo giustificata come «la
	 * via Blueprint» citando il commento di `RegisterScreen` — *«popolate da configurazione o da Blueprint
	 * all'avvio»* — ma quella via **esiste gia'**: `RegisterScreen` e' esposta da CP 46.1. UHT ha rifiutato
	 * la firma (`TArray<FRTScreenBinding>` non e' un tipo Blueprint), e la correzione giusta non era marcare
	 * la struct `BlueprintType`: sarebbe stata l'esposizione senza consumatori per cui questo stesso tipo
	 * era stato rimosso la prima volta.
	 *
	 * Resta C++ puro perche' e' cio' che rende **verificabile** il contratto di `StartFrontend`: senza un
	 * modo di passare un elenco vuoto, il ramo «zero schermate» si proverebbe solo manomettendo `GConfig`.
	 */
	bool StartFrontendFrom(const TArray<FRTScreenBinding>& InScreens);

	/**
	 * Registra i binding dichiarati in `DefaultGame.ini`, e restituisce **quanti ne sono entrati davvero**.
	 *
	 * Sezione `[/Script/RefactorTactics.RTFrontendNavigator]`, una riga `+Screens=(...)` per schermata.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	int32 RegisterScreensFromConfig();

	/**
	 * Registra un elenco di binding, e restituisce **quanti ne sono entrati**.
	 *
	 * ⚠️ **Il valore di ritorno non e' una comodita'.** Le voci incomplete vengono scartate in silenzio —
	 * e' l'unico comportamento sensato per un `.ini`, dove un refuso non deve impedire l'avvio — quindi il
	 * conteggio e' l'unico segnale che distingue «registrate tutte» da «registrate alcune». Un chiamante
	 * che lo ignora non si accorge di un frontend senza schermate finche' non guarda lo schermo.
	 */
	int32 RegisterScreens(const TArray<FRTScreenBinding>& InScreens);

	/**
	 * Le schermate che hanno un binding, per i test e per la diagnostica.
	 *
	 * Stesso ruolo di `GetStack()`, e stessa scelta: accessor C++ semplice e **non** `UFUNCTION`, perche'
	 * la superficie Blueprint del navigatore descrive la navigazione, non il suo inventario.
	 */
	TArray<FName> GetRegisteredScreenIds() const;

	/**
	 * Dichiara la radice e apre la prima schermata. **Va chiamata prima di ogni altra cosa**: senza radice
	 * lo stack non ha uno stato legale, e `ReturnMain` non avrebbe dove tornare.
	 *
	 * ⚠️ **Smonta e dimentica i widget della sessione precedente.** Non e' una navigazione: e' l'inizio di
	 * una sessione nuova, e i widget vivi appartengono al mondo in cui sono stati costruiti. Chi la chiama
	 * come un «assicurati di essere alla radice» idempotente paga una ricostruzione completa e **perde
	 * scroll, selezione e campi di testo** di ogni schermata gia' aperta — vedi `FindLiveWidget`.
	 * `Bindings` invece sopravvive: `StartFrontendFrom` registra e *poi* inizializza.
	 *
	 * ⚠️ **Restituisce l'esito, e la prima stesura era `void`.** Con un `RootScreenId` vuoto il navigatore
	 * restava muto: ogni `PushScreen` successivo rispondeva `Ok` — lo stack si muove — ma nessun widget
	 * compariva mai, perche' `SyncPresentation` esce subito. Un fallimento indistinguibile dal successo e'
	 * esattamente cio' che `ERTNavResult` esiste per impedire. Trovato in code review.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult InitializeFrontend(FName RootScreenId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult PushScreen(FName ScreenId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult PopScreen();

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult ShowModal(FName ModalId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult CloseModal();

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult ReturnMain();

	/**
	 * Il `BACK` di un modale d'errore d'avvio: **due esiti, e la fase decide quale**.
	 *
	 * - errore **durante il loading** (`Phase` diversa da `Ready`): non e' stato costruito niente, quindi
	 *   si chiude il modale e si torna alla schermata precedente;
	 * - errore **a partita gia' viva** (`Phase == Ready`): `ReturnMain`, che **smonta**. Un `PopScreen`
	 *   lascerebbe una partita viva **sotto** il menu, ed e' lo stato che CP 46.6 vieta.
	 *
	 * ⚠️ **Il modale si chiude prima del pop, e non e' un dettaglio d'ordine**: `PopScreen` con un modale
	 * aperto risponde `BlockedByModal` — la schermata sotto e' disabilitata. Chiamarlo direttamente
	 * lascerebbe il giocatore dentro il modale, cioe' il dead-end che il DoD di CP 46.1 vieta.
	 *
	 * ⚠️ **Il parametro e' un dato d'avvio, non una scelta gia' presa.** Chi chiama passa la fase in cui
	 * il modale e' stato armato — `URTErrorModalWidgetBase::GetPhaseWhenArmed()` — e la regola di *dove*
	 * si torna vive qui, perche' il navigatore e' l'unico owner del flow (invariante 1 di CP 46.1). Un
	 * widget che scegliesse fra `PopScreen` e `ReturnMain` sarebbe la seconda autorita' sulla navigazione.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult BackFromError(ERTLoadPhase PhaseWhenArmed);

	/**
	 * `PLAY`: chiede di avviare il vertical slice (CP 46.4, #939).
	 *
	 * ⛔ **Non apre il livello, lo CHIEDE.** Questo subsystem non ha un mondo — vive sulla `GameInstance` e
	 * per contratto non tocca la scena — quindi decide *cosa* e *quando*, e l'apertura resta a chi il mondo
	 * ce l'ha, via `ConsumePendingMatchLevel`. Non e' un secondo percorso di avvio: e' lo stesso percorso
	 * in due meta', e la partita resta allestita da `ARTGameMode` col formato spedito da C++.
	 *
	 * Su `MatchLevel` vuoto **fallisce rumorosamente**: modale d'errore con la causa, non un ritorno
	 * silenzioso al menu. E' il primo punto del DoD, e il progetto ha gia' la famiglia di difetti che
	 * quel silenzio produce — un percorso che non risolve e' «un fallimento indistinguibile dal successo».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	ERTNavResult StartMatch();

	/** Annuncio di `StartMatch`: la richiesta e' pendente e attende chi la consuma. */
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Frontend")
	FRTOnMatchRequested OnMatchRequested;

	/**
	 * Il livello che `StartMatch` ha chiesto di aprire, e lo AZZERA.
	 *
	 * ⚠️ `Consume` e non `Get`: una richiesta letta due volte aprirebbe il livello due volte.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	FString ConsumePendingMatchLevel();

	/**
	 * Apre la schermata di fine partita con il risultato **canonico** (CP 46.5, `#940`).
	 *
	 * ⛔ **Non calcola niente.** Prende il verdetto già dato — la condizione di fine partita è di E10 e il
	 * TurnLog ne è il registro — e lo traduce nella vista che il widget legge. Il DoD lo chiede alla UI:
	 * *«un secondo calcolo nel widget sarebbe una seconda autorità che può divergere»*, e la regola vale
	 * uguale un livello più in basso: se il navigatore ricalcolasse, il widget leggerebbe comunque un
	 * secondo numero.
	 *
	 * ⚠️ **Il risultato si scrive prima del push, e si annulla se il push fallisce.** L'ordine è lo stesso
	 * di `StartMatch` e per la stessa ragione: `PushScreen` presenta il widget in modo *sincrono*, e
	 * `AddToViewport` esegue `Construct` — che è dove un widget legge `GetMatchResult()` per popolare i
	 * testi. Scrivere dopo il push significava mostrargli `InProgress` e round 0 alla prima apertura.
	 * Il rollback tiene la garanzia opposta: un risultato non resta dietro una schermata mai aperta.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	ERTNavResult ShowResult(const FRTMatchResult& InResult, const FRTMatchState& InState);

	/**
	 * Ciò che la schermata di fine partita mostra.
	 *
	 * ⚠️ Vuoto finché una partita non è finita, **e di nuovo vuoto appena si torna al menu**: non è un
	 * archivio dell'ultima partita, è ciò che il Result sta mostrando adesso.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	const FRTMatchResultViewModel& GetMatchResult() const { return MatchResult; }

	/**
	 * `PLAY AGAIN`: ripercorre CP 46.4.
	 *
	 * ⚠️ **Passa dallo stesso `StartMatch`**, non da una seconda via d'avvio. Due percorsi che chiedono una
	 * partita sarebbero due posti in cui la guardia `MatchRequestNotConsumed` può mancare, e la ragione per
	 * cui quella guardia esiste è che una richiesta non consumata è un avvio silenziosamente perduto.
	 *
	 * Svuota lo stack **prima** di ripartire: un `Result` lasciato sotto sarebbe raggiungibile col `Back`
	 * dalla partita nuova, che è il dead-end vietato da CP 46.1.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	ERTNavResult PlayAgain();

	/**
	 * Il livello del vertical slice, da `DefaultGame.ini`.
	 *
	 * ⚠️ **Sta qui e non in `DefaultEngine.ini`**: `GameDefaultMap` dichiara da dove il PACCHETTO avvia —
	 * il menu — e non quale partita il menu apre. Erano due dati diversi nello stesso posto, e finche' non
	 * si e' scritto questo il secondo non esisteva affatto.
	 *
	 * ⛔ **Niente `EditAnywhere` ne' `BlueprintReadWrite`**, per la stessa ragione scritta su
	 * `FRTScreenBinding`: questo campo si scrive in `DefaultGame.ini`, non in un pannello. Esporlo
	 * sembrerebbe il modo previsto di dichiararlo senza esserlo — e un valore scritto da Blueprint verrebbe
	 * comunque sovrascritto dal `LoadConfig()` del prossimo `StartFrontend`.
	 */
	UPROPERTY(Config)
	FString MatchLevel;

	/** La schermata in cima. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	FName GetCurrentScreen() const { return Stack.CurrentScreen(); }

	/** `false` sulla radice: la UI lo legge per **non disegnare** il pulsante Back invece di disegnarlo inerte. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool CanGoBack() const { return Stack.CanGoBack(); }

	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool IsModalOpen() const { return Stack.IsModalOpen(); }

	/** La schermata sotto riceve input? `false` mentre un modale la copre. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool IsScreenInteractive() const { return Stack.IsScreenInteractive(); }

	UFUNCTION(BlueprintPure, Category = "Frontend")
	int32 GetDepth() const { return Stack.Depth(); }

	/** Lo stack, per i test e per la diagnostica. */
	const FRTScreenStack& GetStack() const { return Stack; }

	/**
	 * Le associazioni nome -> widget. Popolate da configurazione o da Blueprint all'avvio.
	 *
	 * ⚠️ Una schermata **senza binding non e' un errore di navigazione**: lo stack si muove lo stesso e
	 * `GetCurrentScreen()` risponde. Manca solo cosa disegnare, e in un test headless e' il caso normale.
	 * Confondere le due cose legherebbe la logica agli asset, che e' cio' che questa divisione evita.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	void RegisterScreen(FName ScreenId, TSoftClassPtr<UUserWidget> WidgetClass);

	/**
	 * Il widget **istanziato** per quella schermata, `nullptr` se non ne e' mai stato creato uno.
	 *
	 * ⚠️ **Non dice se e' a schermo.** La prima stesura di questo commento affermava il contrario
	 * (*«nullptr … se non e' sullo schermo»*) mentre l'implementazione legge solo la mappa — trovato in
	 * code review, e corretto qui invece che nel codice per la ragione sotto.
	 *
	 * I widget **restano istanziati** dopo un `Pop`: `DismissWidget` chiama `RemoveFromParent` e lascia la
	 * voce. E' **cache voluta**, non una perdita: ricrearli a ogni navigazione costerebbe un caricamento
	 * per ogni Back. ⚠️ Ha pero' una conseguenza osservabile che va saputa: una schermata ri-mostrata
	 * conserva scroll, selezione e campi di testo, perche' e' **la stessa istanza**. Se una schermata deve
	 * ripartire pulita, e' lei a doversi azzerare in `NativeConstruct` — il navigatore non lo fa e non deve
	 * indovinarlo.
	 *
	 * Chi vuole sapere se un widget e' visibile chiede `IsInViewport()` al widget.
	 *
	 * ⚠️ **La cache muore a `InitializeFrontend`**, e il paragrafo qui sopra vale **dentro** una sessione.
	 * Una sessione nuova ricostruisce tutto, perche' i widget vecchi appartengono a un mondo smontato.
	 *
	 * 🔴 **Cio' che NON e' coperto, e va saputo**: `ReturnMain`, `PushScreen` e `ShowModal` raggiungono
	 * `PresentWidget` **senza** passare da `InitializeFrontend`. Se fra la costruzione di un widget e una
	 * di quelle chiamate il mondo e' cambiato — per esempio `BackFromError(Ready)` dopo un cambio di
	 * livello — la cache restituisce ancora l'istanza vecchia. La correzione robusta guarda il **confine
	 * del mondo** (`FWorldDelegates::OnWorldCleanup`, o una validazione dentro `PresentWidget`) invece di
	 * un solo chiamante, ed e' piu' larga di CP 46.3. Trovato in code review su PR #1272.
	 */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	UUserWidget* FindLiveWidget(FName ScreenId) const;

	// ~UGameInstanceSubsystem
	virtual void Deinitialize() override;

private:
	/** Crea (o riusa) il widget di `ScreenId` e lo mette a schermo. No-op senza binding. */
	void PresentWidget(FName ScreenId, int32 ZOrder);

	/** Toglie dallo schermo il widget di `ScreenId`, se c'e'. */
	void DismissWidget(FName ScreenId);

	/**
	 * Smonta e **dimentica** tutti i widget istanziati.
	 *
	 * Chiamata da `InitializeFrontend` — una sessione nuova non eredita i widget della precedente, che
	 * appartengono a un mondo ormai smontato — e da `Deinitialize`. In un posto solo perche' le due sedi
	 * divergerebbero al primo widget che richiede un passo di pulizia in piu'.
	 */
	void DismissAllWidgets();

	/** Riallinea cio' che e' a schermo con lo stack: e' l'unico punto in cui la presentazione cambia. */
	void SyncPresentation();

	UPROPERTY()
	FRTScreenStack Stack;

	UPROPERTY()
	TMap<FName, TSoftClassPtr<UUserWidget>> Bindings;

	/**
	 * Le schermate dichiarate in `DefaultGame.ini`. **Non sono i binding**: sono cio' che
	 * `RegisterScreensFromConfig` legge per produrli, e restano com'erano scritte anche dopo che una voce
	 * incompleta e' stata scartata. Tenerle separate e' cio' che permette di rileggere la configurazione
	 * senza perdere le registrazioni fatte a mano da un Blueprint.
	 */
	UPROPERTY(Config)
	TArray<FRTScreenBinding> Screens;

	/**
	 * Rifiuta un avvio partita: log, modale ARMATO con l'esito, e il valore di ritorno per chi ha chiesto.
	 *
	 * Il motivo e' un `ERTStartupOutcome`, non una stringa: il testo lo compone `DescribeOutcome`, che e'
	 * localizzato e filtra i non-fatali.
	 */
	ERTNavResult RejectMatchStart(ERTStartupOutcome Outcome, const FString& Detail);

	/** Richiesta pendente di apertura livello: la produce `StartMatch`, la consuma chi ha il mondo. */
	UPROPERTY()
	FString PendingMatchLevel;

	/**
	 * L'ultimo risultato mostrato.
	 *
	 * ⚠️ **Azzerato da `ReturnMain`**, che è il punto comune alle due uscite del Result: `MAIN MENU` lo è
	 * direttamente, `PLAY AGAIN` ci passa attraverso. Prometterlo di `PlayAgain` soltanto era falso —
	 * `MAIN MENU` seguito da `PLAY` lasciava il risultato vecchio leggibile per tutta la partita nuova.
	 */
	UPROPERTY()
	FRTMatchResultViewModel MatchResult;


	/** I widget istanziati, per nome. Sopravvivono al pop finche' `SyncPresentation` non li smonta. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UUserWidget>> LiveWidgets;

	/** `true` dopo `InitializeFrontend`: prima, le transizioni non hanno una radice a cui riferirsi. */
	bool bInitialized = false;
};
