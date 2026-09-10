#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// `ESlateVisibility`: e' il tipo che il binding di `Visibility` accetta, e senza di lui la funzione che
// lo restituisce non sarebbe collegabile.
#include "Components/SlateWrapperTypes.h"
#include "Frontend/RTStartupReport.h"
#include "RTFrontendWidgets.generated.h"

/**
 * Le classi BASE dei widget del frontend (CP 46.2, `#937`).
 *
 * ⚠️ **Qui non c'e' layout.** Il `.uasset` `WBP_RT_*` fa aspetto e disposizione; questo file dichiara
 * **cosa il widget puo' leggere**, e soprattutto cosa non puo'. E' la stessa divisione di
 * `UI/RTScreenHudWidgets.h` (CP 11.7), applicata allo strato che vive **prima e dopo** la partita.
 *
 * Tre vincoli del DoD diventano proprieta' della **firma** invece che disciplina da ricordare:
 *
 *  1. **Il widget non compone il motivo.** Non esiste un accessor che restituisca una `FString` libera da
 *     mostrare: si legge `ERTStartupOutcome` e si chiede il testo a `DescribeOutcome`. E' la ragione per
 *     cui l'ottavo vocabolario e' stato istruito **prima** di scrivere questo file — senza, il motivo
 *     sarebbe nato qui, e la UI sarebbe diventata la sorgente della spiegazione.
 *  2. **Il widget non decide se e' fatale.** `IsFatal` vive nella libreria ed e' l'unico posto in cui la
 *     divisione modale/banner e' scritta.
 *  3. **Il widget non naviga da se'.** Nessuna di queste classi chiama `PushScreen`/`PopScreen`: espongono
 *     un **dato** — `GetPhaseWhenArmed()` — e il navigatore decide, perche' e' lui l'unico owner del flow
 *     (CP 46.1). La regola di *dove* torna il `BACK` vive in `URTFrontendNavigator::BackFromError`.
 *     🔴 **Questa riga citava `RequestBack`, che non e' mai esistito.** Per due giorni e' stata l'unica
 *     occorrenza di quel nome in tutto `Source/`, e descriveva un meccanismo che nessuno aveva scritto:
 *     il pulsante `BACK` era disegnato nel `.uasset` e non chiamava niente. Un commento che nomina un
 *     simbolo assente e' peggio di un commento mancante — si legge come una garanzia.
 *
 * ⚠️ **Il limite di questi vincoli, detto invece che sottinteso**: valgono per la superficie C++. Un
 * Blueprint derivato puo' sempre aggiungersi una variabile stringa e disegnarla.
 * 🔴 **La ragione che questa riserva dava e' scaduta il 2026-08-18**: diceva *«nessun gate lo impedisce,
 * perche' i `.uasset` non sono versionati in questo repository»*, e i cinque `WBP_RT_*` **sono versionati**
 * (PR #1178). Esiste anche un gate: `RTFrontendWidgetAssetTests.cpp` apre i package e verifica i binding
 * dentro di essi. La riserva **regge lo stesso** — un test sui binding non impedisce a un Blueprint di
 * disegnare una stringa propria — ma regge per un motivo diverso da quello scritto, e i due non vanno
 * confusi: il primo era un'assenza di infrastruttura, il secondo e' un limite di cio' che si verifica.
 */

/**
 * Il **Main Menu** (CP 46.3, `#938`): la schermata da cui il pacchetto avvia.
 *
 * ⚠️ **Qui non ci sono i tre pulsanti.** `PLAY · SETTINGS · QUIT` sono widget dentro
 * `WBP_RT_MainMenu.uasset`, e la loro disposizione, il focus visibile e la percorribilita' da tastiera
 * sono lavoro d'editor: restano di `PIE-V01-FRONTEND-MAIN`, che e' l'unico posto in cui si puo' dire se
 * un bordo di focus si vede. Questa classe dichiara **cosa il menu puo' leggere**.
 *
 * ⚠️ **E non naviga**, come le altre tre di questo file: i pulsanti chiamano `URTFrontendNavigator`, che
 * e' gia' `BlueprintCallable`. L'invariante 1 di CP 46.1 dice che il flow ha un owner solo, e un widget
 * che scegliesse la destinazione sarebbe il secondo.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTMainMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * La riga di versione da mostrare a schermo. E' la voce *«version/build label leggibile»* del DoD.
	 *
	 * ⚠️ **Non e' una costante.** Legge `ProjectVersion` da `DefaultGame.ini`, e la ragione e' il modo in
	 * cui questa riga marcisce: una label letterale resta identica dopo il bump, e l'unico segnale che
	 * qualcosa non torna arriva mesi dopo, da chi segnala un difetto su una build che crede di essere
	 * un'altra. Una versione che mente e' peggio di una versione assente.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	FText GetVersionLabel() const;

private:
	/** Legge `ProjectVersion` e compone la riga. Chiamata **una volta**: vedi `CachedVersionLabel`. */
	static FText BuildVersionLabel();

	/**
	 * La label gia' composta.
	 *
	 * ⚠️ `mutable` perche' `GetVersionLabel()` e' `const` — e lo e' giustamente, dato che non cambia nulla
	 * di osservabile. Il valore non dipende dal ciclo di vita del widget, quindi un calcolo pigro batte
	 * `NativeOnInitialized`: non obbliga chi deriva la classe a ricordarsi di chiamare `Super`.
	 */
	mutable TOptional<FText> CachedVersionLabel;

public:

	/**
	 * `true` finche' `SETTINGS` non ha contenuto — cioe' per tutta la v0.1.
	 *
	 * La voce **esiste comunque**: il DoD la vuole perche' il back stack la attraversi, e perche' il menu
	 * non cambi forma in v0.2. Cio' che questo flag governa e' se il pannello si dichiara.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	static bool IsSettingsComingSoon();

	/**
	 * Cosa dice `SETTINGS` quando non ha contenuto. Vuoto quando il pannello e' vero.
	 *
	 * ⚠️ **Il testo nasce qui e non nel Blueprint**, per la stessa ragione di `GetPhaseText()`: un pulsante
	 * che non fa nulla **senza dirlo** e' il dead-end che il DoD vieta, e lasciare la frase al `.uasset`
	 * significherebbe che il rispetto di quella regola dipende da chi ha disegnato il widget.
	 *
	 * 🔴 **`static`, e la prima stesura non lo era.** Trovato in code review: il runbook diceva di derivare
	 * `WBP_RT_SettingsPanel` da `UUserWidget` e di mostrarci questa frase — impossibile, perche' su una
	 * `UUserWidget` la funzione non esiste. Chi costruiva il pannello non l'avrebbe trovata nella palette e
	 * avrebbe scritto la stringa a mano, cioe' proprio cio' che questo file vieta. Statica e' chiamabile da
	 * **qualunque** Blueprint, e i due posti che devono dire la stessa cosa — la voce del menu e il pannello
	 * che si apre — la leggono dallo stesso punto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	static FText GetSettingsNoticeText();

	/**
	 * La visibilita' della riga *coming soon*, gia' nel tipo che il binding di `Visibility` accetta.
	 *
	 * 🔴 **Mancava, ed e' la quarta volta.** `GetLoadingVisibility` spiega il problema per le altre tre
	 * classi di questo file — un `bool` non compare nel menu dei binding di `Visibility`, che vuole un
	 * `ESlateVisibility`, quindi **chi cerca non lo trova** — e chiude dicendo che *«averne risolta una sola
	 * avrebbe lasciato le altre due a far perdere tempo nello stesso identico punto»*. La classe nuova era
	 * stata aggiunta senza. Trovato in code review.
	 *
	 * `Collapsed` e non `Hidden`, come il banner: una riga assente non deve occupare spazio nel layout.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	static ESlateVisibility GetSettingsNoticeVisibility();
};

/**
 * La schermata di **attesa**: mostra la fase raggiunta, e nient'altro.
 *
 * ⚠️ **Nessuna percentuale, e nessun testo scritto a mano.** La fase e' un dato (`ERTLoadPhase`) prodotto
 * da `ARTGameMode`: il widget lo legge. Il DoD nominava tre messaggi che nessuno produceva — tre stringhe
 * scelte a mano sarebbero state tre percentuali con un nome, cioe' lo stesso dato inventato che il DoD
 * vieta per la barra di avanzamento.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTLoadingScreenWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** La fase corrente. `Idle` finche' l'allestimento non comincia. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ERTLoadPhase GetPhase() const { return Phase; }

	/**
	 * La riga da mostrare per la fase corrente. Vuota su `Idle` e su `Ready`: in nessuno dei due casi c'e'
	 * un'attesa da spiegare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	FText GetPhaseText() const;

	/** `true` mentre c'e' qualcosa da aspettare: e' la condizione con cui il Blueprint si mostra. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	bool IsLoading() const { return Phase != ERTLoadPhase::Idle && Phase != ERTLoadPhase::Ready; }

	/** Aggiorna la fase. La chiama chi osserva il `GameMode`; nei test la chiama il test. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void SetPhase(ERTLoadPhase InPhase);

	/**
	 * La visibilita' della schermata, nel tipo che il binding di `Visibility` accetta.
	 *
	 * ⚠️ Esiste per la stessa ragione di `GetBannerVisibility()`: `IsLoading()` restituisce `bool` e lo
	 * slot vuole un `ESlateVisibility`, quindi il menu dei binding lo **filtra via** e chi cerca non lo
	 * trova. Le tre classi di questo file hanno lo stesso problema, e averne risolta una sola avrebbe
	 * lasciato le altre due a far perdere tempo nello stesso identico punto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ESlateVisibility GetLoadingVisibility() const;

private:
	UPROPERTY(Transient)
	ERTLoadPhase Phase = ERTLoadPhase::Idle;
};

/**
 * Il **modale d'errore**: la partita non e' partita, e si deve poter uscire.
 *
 * Compare **solo** per un esito fatale — `IsFatal(Outcome) == true`. Un ripiego non passa di qui: quello
 * e' del banner, e confondere i due era il difetto del DoD originale.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTErrorModalWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** L'esito che ha impedito l'avvio. `Ok` quando il modale non ha ragione di esistere. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ERTStartupOutcome GetOutcome() const { return Outcome; }

	/** La causa, leggibile. Prodotta da `DescribeOutcome`, **non** composta qui. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	FText GetReasonText() const;

	/**
	 * Il dettaglio tecnico, per il pulsante `DETAILS`.
	 *
	 * ⚠️ **Vuoto in Shipping**, e non per una `if` nel Blueprint: la stringa non esiste proprio, perche'
	 * `#if !UE_BUILD_SHIPPING` la compila fuori. Un Blueprint che la disegnasse comunque mostrerebbe una
	 * riga vuota invece di un dettaglio interno — che e' il comportamento voluto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	FString GetDetail() const;

	/** `true` se il pulsante `DETAILS` deve esistere: solo dove c'e' un dettaglio da mostrare. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	bool ShouldShowDetails() const;

	/** Arma il modale con una nota fatale. Ignora le note non fatali: quelle sono del banner. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void ShowFor(const FRTStartupNote& Note);

	/**
	 * Arma il modale **dal rapporto d'avvio**: prende il fatale se c'e', altrimenti non fa nulla.
	 *
	 * E' la forma che serve in gioco — chi mostra il modale ha in mano il rapporto del `GameMode`, non una
	 * nota isolata — ed evita al chiamante di dover sapere *quale* delle note e' quella fatale. La regola
	 * di quale lo sia vive in `IsFatal`, e resta li'.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void ShowFromReport(const FRTStartupReport& Report);

	/**
	 * Arma il modale da un esito e un dettaglio.
	 *
	 * ⚠️ **Esiste perche' `FRTStartupNote` non e' costruibile da Blueprint**: i suoi campi sono
	 * `BlueprintReadOnly` — di proposito, perche' una nota la produce chi rileva la condizione, non chi la
	 * mostra — e questo rende il nodo `Make` inutilizzabile. Senza questa funzione il modale non sarebbe
	 * armabile da un Blueprint, e quindi **non sarebbe provabile in PIE** finche' non esiste il codice che
	 * lo arma davvero. Scoperto costruendo il widget, non previsto.
	 *
	 * Rifiuta gli esiti non fatali con la stessa regola di `ShowFor`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void ShowForOutcome(ERTStartupOutcome InOutcome, const FString& InDetail);

	/** `true` quando il modale ha qualcosa da dire. Il Blueprint lo usa per mostrarsi. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	bool IsArmed() const { return Outcome != ERTStartupOutcome::Ok; }

	/**
	 * La fase raggiunta dall'allestimento quando questo modale e' stato armato.
	 *
	 * Serve a **una** cosa: il `BACK`. `URTFrontendNavigator::BackFromError` la legge per scegliere fra
	 * tornare indietro e smontare, e la regola di quale dei due vive **li'**, non qui — questo widget
	 * espone un dato, non una decisione di navigazione (invariante 1 di CP 46.1).
	 *
	 * ⚠️ **`Idle` quando il modale e' stato armato senza un report**, cioe' da `ShowFor` o
	 * `ShowForOutcome`: e' il default prudente, e porta il `BACK` a `PopScreen`. L'alternativa —
	 * assumere `Ready` — smonterebbe una partita che potrebbe non essere mai stata avviata.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ERTLoadPhase GetPhaseWhenArmed() const { return PhaseWhenArmed; }

	/**
	 * La visibilita' del modale, gia' nel tipo del binding.
	 *
	 * ⚠️ Qui e' **`Visible` e non `SelfHitTestInvisible`**: un modale deve *fermare* i click su cio' che
	 * sta sotto, ed e' l'unica delle tre schermate per cui il valore non e' una formalita'.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ESlateVisibility GetModalVisibility() const;

	/**
	 * La visibilita' del pulsante `DETAILS`, gia' nel tipo del binding.
	 *
	 * ⚠️ In Shipping e' **sempre `Collapsed`**, perche' `GetDetail()` restituisce una stringa vuota: il
	 * dettaglio non e' nascosto, non e' proprio compilato. Un Blueprint che disegnasse il pulsante lo
	 * stesso mostrerebbe un bottone che apre il vuoto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ESlateVisibility GetDetailsVisibility() const;

private:
	UPROPERTY(Transient)
	ERTStartupOutcome Outcome = ERTStartupOutcome::Ok;

	UPROPERTY(Transient)
	FString Detail;

	/** Vedi `GetPhaseWhenArmed`. La scrive solo `ShowFromReport`, che e' l'unica ad avere un report. */
	UPROPERTY(Transient)
	ERTLoadPhase PhaseWhenArmed = ERTLoadPhase::Idle;
};

/**
 * Il **banner di ripiego**: la partita e' partita, ma non e' quella che si crede.
 *
 * ⚠️ **Elenca TUTTE le condizioni degradate**, non la piu' grave. Un avvio ne accumula piu' d'una, e
 * mostrarne una sola nasconderebbe l'altra — che e' il modo esatto in cui queste cose sono rimaste
 * invisibili finora: due righe di log separate, nessuna delle quali qualcuno aveva motivo di cercare.
 *
 * Riprende la forma di `ARTGameMode::GetScenarioBannerText()`, che esiste dal 2026-08-08 con la stessa
 * motivazione — *«il sintomo non punta alla causa»* — generalizzata all'avvio.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTFallbackBannerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Una riga per condizione degradata, nell'ordine in cui si sono presentate. Vuoto se non ce n'e'. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	const TArray<FText>& GetLines() const { return Lines; }

	/** `true` se c'e' qualcosa da mostrare: e' la condizione con cui il Blueprint si accende. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	bool HasAnything() const { return Lines.Num() > 0; }

	/**
	 * Tutte le righe in un solo `FText`, separate da `\n`.
	 *
	 * ⚠️ **Esiste per una ragione pratica misurata sul lavoro vero**: senza, il Blueprint deve ciclare
	 * l'array e costruire un `TextBlock` per riga — cinque nodi, `Construct Object from Class` compreso.
	 * Con questo accessor il widget e' **un `Text Block` con un binding**, e il ciclo vive dove e'
	 * testabile. E' la stessa regola che il progetto applica altrove: i widget non compongono, leggono.
	 *
	 * Il `TextBlock` che lo consuma deve avere **`Auto Wrap Text`** attivo e `Justification` a sinistra;
	 * il numero di righe resta osservabile — e resta la cosa da verificare, perche' mostrarne una sola
	 * nasconderebbe l'altra.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	FText GetLinesAsText() const;

	/**
	 * La visibilita' del banner, gia' nel tipo che il binding di `Visibility` accetta.
	 *
	 * ⚠️ **Esiste perche' `HasAnything()` non e' collegabile a quel binding**: restituisce `bool` e lo
	 * slot vuole un `ESlateVisibility`, quindi il menu lo filtra via e chi cerca non lo trova. La via
	 * alternativa — un `Select` in Blueprint — metterebbe in tre nodi una scelta che qui e' una riga, e
	 * la ripeterebbe identica in ogni widget che ne ha bisogno.
	 *
	 * `Collapsed` e non `Hidden`: un banner assente non deve occupare spazio nel layout sotto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Frontend")
	ESlateVisibility GetBannerVisibility() const;

	/**
	 * Riempie il banner dal rapporto d'avvio.
	 *
	 * ⚠️ **Filtra i fatali di proposito**: se la partita non e' partita, il banner non ha una partita
	 * sotto cui stare. Quel caso e' del modale.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void SetFromReport(const FRTStartupReport& Report);

private:
	UPROPERTY(Transient)
	TArray<FText> Lines;
};
