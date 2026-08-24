#pragma once

#include "CoreMinimal.h"
#include "RTScreenStack.generated.h"

/**
 * L'esito di un'operazione di navigazione. **L'elenco e' chiuso**, come gli otto esiti del contratto del
 * puntatore (CP 11.8): se una transizione non produce uno di questi, il contratto non la descrive e si
 * dichiara prima di scriverla.
 *
 * ⚠️ Ogni rifiuto porta un motivo. Un `Blocked` silenzioso e' un difetto — stessa regola di
 * `spec-pointer-interaction.md`, e per la stessa ragione pratica: chi chiama deve poter distinguere
 * «non si puo' adesso» da «non e' successo niente».
 */
UENUM(BlueprintType)
enum class ERTNavResult : uint8
{
	/** La transizione e' avvenuta. */
	Ok,
	/** `PopScreen` sulla radice: non c'e' nulla sotto, e uno stack vuoto non ha schermate da mostrare. */
	BlockedAtRoot,
	/** Un modale e' aperto: la schermata sotto e' disabilitata, quindi non naviga. */
	BlockedByModal,
	/** `CloseModal` senza modali aperti. Distinto da `Ok` perche' altrimenti un doppio click mangerebbe una schermata. */
	NoModalOpen,
	/** Il nome della schermata e' vuoto: non identifica niente. */
	InvalidScreen,
	/**
	 * Un `FName` non puo' essere insieme schermata nello stack e modale sopra di esso.
	 *
	 * ⚠️ **Aggiunto in code review il 2026-08-16, e l'elenco e' dichiarato chiuso: eccolo esteso
	 * esplicitamente invece che allargato in silenzio.** Non e' una raffinatezza: la presentazione tiene
	 * **un widget per nome**, quindi lo stesso `FName` in entrambi i ruoli produce un widget solo, che
	 * viene disabilitato in quanto «schermata sotto un modale» mentre **e'** il modale. Il pulsante di
	 * chiusura diventa inerte e l'unica uscita e' un `ReturnMain` chiamato da fuori: e' il dead-end che
	 * il DoD di CP 46.1 vieta.
	 *
	 * ⚠️ **Il nome e' piu' generale della sua prima causa, e CP 46.6 usa la seconda**: `ShowPause` con la
	 * pausa gia' nello stack risponde con questo esito. La ragione e' la stessa — un `FName` per un widget,
	 * quindi due `Pause` impilate sarebbero due voci di stack e un widget solo, e il `RESUME` ne
	 * smonterebbe una lasciando l'altra a coprire la partita.
	 */
	ScreenIsAlreadyOnStack,

	/**
	 * `ResumeMatch` senza una pausa aperta (CP 46.6, `#941`).
	 *
	 * ⚠️ **L'elenco e' dichiarato chiuso, ed eccolo esteso esplicitamente** — stessa procedura di
	 * `ScreenIsAlreadyOnStack`. Serve per la stessa ragione per cui `NoModalOpen` e' distinto da `Ok`: un
	 * `RESUME` che non aveva niente da chiudere e un `RESUME` riuscito non devono avere lo stesso valore di
	 * ritorno, o un doppio invio mangerebbe la schermata sotto.
	 *
	 * ⛔ Non e' `NoModalOpen`: la pausa e' una **schermata**, e confonderli manderebbe chi indaga a cercare
	 * un modale che non c'e' mai stato.
	 */
	NoPauseOpen
};

/**
 * Lo **stack di navigazione** del frontend: chi e' in cima, cosa c'e' sotto, quali modali sono aperti.
 *
 * ## Perche' e' uno `USTRUCT` puro e non un `UObject`
 *
 * Perche' la navigazione **non e' UI**: e' una macchina a stati che *governa* la UI. Separandola dalla
 * presentazione si prova senza mondo, senza widget e senza editor — ed e' cio' che rende il DoD di CP 46.1
 * verificabile invece che ispezionabile.
 *
 * ⚠️ **Contraddice una previsione scritta in D-144 e nella spec owner**, che dichiaravano i checkpoint di
 * E46 privi di «un test automatico possibile». La previsione era sbagliata due volte: `RTScreenHudWidgets`
 * prova gia' widget UMG headless (CP 11.7), e soprattutto lo stack non e' un widget. Cio' che resta non
 * automatizzabile e' il **layout** dentro il `.uasset`, che e' di `PIE-V01-FRONTEND-NAV`.
 *
 * ## Cosa NON fa, e non e' una dimenticanza
 *
 * - **Non conosce i widget.** Le schermate sono `FName`; la mappa nome -> classe e' un *dato* del
 *   navigator, non logica di questo tipo. Cosi' un test non ha bisogno di asset per esistere.
 * - **Non tocca il `PlayerController`.** `ERTPointerContext::Modal` appartiene a CP 11.8, che ha gia' i
 *   sette contesti e la precedenza `Modal/Reaction UI > HUD > world`. Sono **due strati**: qui si decide
 *   *quale schermata e' visibile*, li' *chi consuma un click in partita*. Un navigatore che decidesse la
 *   precedenza dell'input duplicherebbe un contratto gia' scritto — ed e' il vincolo esplicito di #936.
 * - **Non deduplica.** `Settings` aperto dal Main e `Settings` aperto dalla Pause sono la stessa schermata
 *   con due ritorni diversi: collassarli manderebbe il `Back` nel posto sbagliato.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTScreenStack
{
	GENERATED_BODY()

	/**
	 * 🔴 **Il default costruisce uno stack VUOTO, e va detto perche' i commenti qui sotto dicevano il
	 * contrario.** «Sempre >= 1», «non vuoto per costruzione»: falsi per questo costruttore, e non in
	 * teoria — `URTFrontendNavigator::Stack` e' default-costruito, quindi **prima** di
	 * `InitializeFrontend` le sue `BlueprintPure` rispondono `Depth() == 0` e `CurrentScreen() == None`.
	 * Trovato in code review.
	 *
	 * Non si puo' cancellare: `USTRUCT` con `GENERATED_BODY()` **richiede** un costruttore di default —
	 * la reflection lo usa per serializzazione e per i valori di default delle `UPROPERTY`. Quindi lo
	 * stato vuoto e' **rappresentabile per forza**, e l'unica scelta onesta e' renderlo **innocuo e
	 * dichiarato** invece di negarlo in prosa:
	 *
	 * - `IsValid()` lo distingue, ed e' la domanda che un chiamante puo' porre;
	 * - `ReturnMain()` su uno stack vuoto **non restituisce piu' `Ok`** — dichiarava successo restando
	 *   senza schermata, che era il difetto peggiore dei tre;
	 * - il navigatore non lo espone: `bInitialized` resta `false` finche' non c'e' una radice.
	 */
	FRTScreenStack() = default;

	/**
	 * Uno stack costruito **con la sua radice**. E' l'unico modo per ottenerne uno utilizzabile: il
	 * default esiste solo perche' la reflection lo pretende (vedi sopra).
	 */
	explicit FRTScreenStack(FName InRootScreen)
	{
		if (!InRootScreen.IsNone())
		{
			Screens.Add(InRootScreen);
		}
	}

	/**
	 * `false` su uno stack default-costruito o costruito con un nome vuoto: nessuna schermata, nessuna
	 * radice, nessuna operazione sensata.
	 */
	bool IsValid() const { return Screens.Num() > 0; }

	/** Impila una schermata sopra quella corrente. Rifiutata se un modale e' aperto. */
	ERTNavResult PushScreen(FName ScreenId);

	/** Torna alla schermata **che ha spinto** quella corrente. Rifiutato sulla radice e sotto un modale. */
	ERTNavResult PopScreen();

	/** Apre un modale sopra la schermata corrente, che smette di essere interattiva. */
	ERTNavResult ShowModal(FName ModalId);

	/** Chiude il modale in cima. Rifiutato se non ce ne sono: non deve mai diventare un `PopScreen`. */
	ERTNavResult CloseModal();

	/**
	 * Svuota tutto e torna alla radice, **modali compresi**. E' l'uscita di sicurezza del «nessun
	 * dead-end»: se esistesse uno stato da cui non riporta alla radice, quello stato sarebbe un dead-end.
	 */
	ERTNavResult ReturnMain();

	/** La schermata in cima. `NAME_None` **solo** su uno stack non valido (`IsValid() == false`). */
	FName CurrentScreen() const { return Screens.Num() > 0 ? Screens.Last() : NAME_None; }

	/** La radice: dove `ReturnMain` riporta. */
	FName RootScreen() const { return Screens.Num() > 0 ? Screens[0] : NAME_None; }

	/** Il modale in cima, `NAME_None` se non ce ne sono. */
	FName TopModal() const { return Modals.Num() > 0 ? Modals.Last() : NAME_None; }

	/** Quante schermate sono impilate, radice inclusa. `>= 1` su ogni stack valido, `0` se non lo e'. */
	int32 Depth() const { return Screens.Num(); }

	/** Quanti modali sono aperti. */
	int32 ModalDepth() const { return Modals.Num(); }

	bool IsModalOpen() const { return Modals.Num() > 0; }

	/** `false` sulla radice: e' li' che il `Back` non esiste, e la UI deve poterlo sapere per non disegnarlo. */
	bool CanGoBack() const { return Screens.Num() > 1 && !IsModalOpen(); }

	/** La schermata sotto riceve input solo se nessun modale la copre. */
	bool IsScreenInteractive() const { return !IsModalOpen(); }

	/** L'intero stack, dalla radice alla cima. Serve alla presentazione per sapere cosa smontare. */
	const TArray<FName>& GetScreens() const { return Screens; }

	const TArray<FName>& GetModals() const { return Modals; }

private:
	/** Dalla radice `[0]` alla cima `Last()`. Vuoto **solo** su uno stack default-costruito — vedi `IsValid()`. */
	UPROPERTY()
	TArray<FName> Screens;

	/** I modali aperti sopra la schermata corrente, in ordine di apertura. */
	UPROPERTY()
	TArray<FName> Modals;
};
