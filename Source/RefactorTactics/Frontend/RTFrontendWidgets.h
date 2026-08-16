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
 *     l'**intenzione** (`RequestBack`) e il navigatore decide, perche' e' lui l'unico owner del flow
 *     (CP 46.1).
 *
 * ⚠️ **Il limite di questi vincoli, detto invece che sottinteso**: valgono per la superficie C++. Un
 * Blueprint derivato puo' sempre aggiungersi una variabile stringa e disegnarla — nessun gate lo impedisce,
 * perche' i `.uasset` non sono versionati in questo repository. E' la stessa riserva che CP 11.7 scrive
 * per le proprie basi.
 */

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
