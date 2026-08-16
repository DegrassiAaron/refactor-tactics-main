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
	InvalidScreen
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

	FRTScreenStack() = default;

	/**
	 * Uno stack nasce **con la sua radice**, e non vuoto: non esiste uno stato legale senza schermata
	 * corrente, quindi non e' rappresentabile. Un default vuoto costringerebbe ogni chiamante a gestire
	 * un caso che il dominio non ha.
	 */
	explicit FRTScreenStack(FName InRootScreen)
	{
		Screens.Add(InRootScreen);
	}

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

	/** La schermata in cima allo stack. Mai `NAME_None` su uno stack costruito con una radice. */
	FName CurrentScreen() const { return Screens.Num() > 0 ? Screens.Last() : NAME_None; }

	/** La radice: dove `ReturnMain` riporta. */
	FName RootScreen() const { return Screens.Num() > 0 ? Screens[0] : NAME_None; }

	/** Il modale in cima, `NAME_None` se non ce ne sono. */
	FName TopModal() const { return Modals.Num() > 0 ? Modals.Last() : NAME_None; }

	/** Quante schermate sono impilate, radice inclusa. Sempre >= 1. */
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
	/** Dalla radice `[0]` alla cima `Last()`. Non vuoto per costruzione. */
	UPROPERTY()
	TArray<FName> Screens;

	/** I modali aperti sopra la schermata corrente, in ordine di apertura. */
	UPROPERTY()
	TArray<FName> Modals;
};
