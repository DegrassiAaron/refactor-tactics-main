#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// `ESlateVisibility`: e' il tipo che il binding di `Visibility` accetta, e senza di lui la funzione che
// lo restituisce non sarebbe collegabile. Stessa ragione di `RTFrontendWidgets.h`.
#include "Components/SlateWrapperTypes.h"
#include "Replay/RTMatchHistoryLibrary.h" // FRTMatchHistoryEntry e' un valore di ritorno: serve la definizione
#include "Replay/RTReplayPlayerLibrary.h" // ERTReplayOpenResult
#include "RTReplayScreenWidgets.generated.h"

class URTFrontendNavigator;
class URTReplayViewerSubsystem;

/**
 * Le classi BASE delle due schermate del replay (`#472`, R6).
 *
 * ⚠️ **Qui non c'e' layout.** Il `.uasset` `WBP_RT_*` fa aspetto e disposizione; questo file dichiara
 * **cosa il widget puo' fare**, e soprattutto cosa non puo'. E' la stessa divisione di
 * `Frontend/RTFrontendWidgets.h` e `UI/RTScreenHudWidgets.h`.
 *
 * 🔴 **Perche' stanno in `Replay/` e non in `Frontend/`**, che e' «lo strato prima e dopo la partita» e
 * sembrerebbe la casa giusta: perche' **il frontend non conosce il replay**. Il navigatore non lo conosce
 * nemmeno per chiudere l'archivio — lo fa la schermata uscendo — e mettere qui la dipendenza la tiene nel
 * verso in cui esiste gia': `Replay/` sa del frontend, il frontend non sa del replay.
 *
 * ⛔ **Nessuna delle due possiede la riproduzione.** Posizione, ritmo, bordi e comandi abilitati stanno in
 * `FRTReplayViewModel` e si provano senza widget e senza mondo: e' la separazione che la spec chiama *«la
 * logica che governa la UI non e' UI»*. Cio' che questi tipi aggiungono e' l'**unica** cosa che il view
 * model non puo' sapere — dove si trova chi guarda, e cosa succede quando clicca.
 *
 * Owner documentale: [`spec-frontend-navigazione.md`](../../../docs/technical/systems/spec-frontend-navigazione.md) §2.2.
 */

/**
 * La **lista delle partite registrate**: si sceglie una riga, e si va a guardarla.
 *
 * ⚠️ **Porta l'indice, non gli archivi**: `LoadMatchList` legge `history.rtindex` e non apre nessuna
 * cartella. E' cio' che rende la lista istantanea, e ha una conseguenza che la UI deve mostrare invece di
 * nascondere — una riga il cui archivio e' stato cancellato **resta nella lista**, e lo si scopre solo
 * aprendola. L'indice per design non lo sa.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTMatchHistoryWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Le partite da disegnare, dalla piu' recente.
	 *
	 * ⚠️ **L'ordinamento e' un parametro e non una scelta di questo tipo**: lo fa `LoadMatchList`, perche'
	 * Blueprint non sa ordinare un `TArray<FRTMatchHistoryEntry>` per `FDateTime` e scendere in C++ per
	 * questo e' precisamente cio' che il DoD di `#999` esclude.
	 *
	 * Un array **vuoto** e' un esito legittimo — nessuna partita giocata — e non un errore: e' la
	 * schermata a dover distinguere «vuoto» da «non ho potuto leggere», e per questo c'e' `bOutReadFailed`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	TArray<FRTMatchHistoryEntry> LoadMatches(bool& bOutReadFailed);

	/**
	 * La lettura, interrogabile: **«vuoto» e «non ho potuto leggere» sono due esiti diversi**.
	 *
	 * ⚠️ Un `TArray` vuoto da solo non li distingue, e una schermata che li confondesse direbbe «non hai
	 * ancora giocato» a chi ha l'indice illeggibile. Vedi `SelectAndNavigate` per il perche' e' statica.
	 */
	static TArray<FRTMatchHistoryEntry> LoadMatchesFrom(URTReplayViewerSubsystem* Replay,
		bool& bOutReadFailed);

	/**
	 * Sceglie una partita e **va** al viewer: dichiara la selezione, poi naviga.
	 *
	 * `false` se il `MatchId` non e' valido o se la navigazione e' stata rifiutata — e in **nessuno** dei
	 * due casi la selezione resta appesa. Il contratto sta su `SelectAndNavigate`, che e' dove si prova.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool OpenMatch(const FGuid& MatchId);

	/**
	 * La **decisione** dietro `OpenMatch`, separata da chi risolve i subsystem.
	 *
	 * 🔴 **Statica e con le dipendenze in ingresso, come `ARTHUD::ComputePlannedHitMarks` e
	 * `BuildVersionLabel`**: un metodo d'istanza che chiama `GetGameInstance()` non e' interrogabile in un
	 * test headless — i test costruiscono i widget con `NewObject`, che non da' loro una `GameInstance` —
	 * e la regola finirebbe provata solo in PIE, cioe' non provata.
	 *
	 * 🔴 **L'ordine e' il contratto**, ed e' la stessa ragione per cui `PendingMatchLevel` si scrive prima
	 * del broadcast: `PushScreen` presenta il widget in modo **sincrono**, quindi il viewer consuma la
	 * selezione *durante* la chiamata. Dichiararla dopo darebbe un viewer che si apre su niente, sempre —
	 * non a intermittenza.
	 *
	 * ⛔ **Non apre l'archivio**: lo fa il viewer alla sua comparsa. Aprirlo qui vorrebbe dire che la lista
	 * sa riprodurre, e che un archivio resta aperto anche quando la navigazione e' stata rifiutata.
	 */
	static bool SelectAndNavigate(URTReplayViewerSubsystem* Replay, URTFrontendNavigator* Navigator,
		const FGuid& MatchId);

	/** `true` quando non c'e' nessuna partita da elencare: e' la condizione del messaggio «nessun replay». */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool IsEmpty() const { return bLastLoadWasEmpty; }

	/**
	 * La visibilita' del messaggio di lista vuota, nel tipo che il binding accetta.
	 *
	 * ⚠️ Esiste per la stessa ragione di `GetLoadingVisibility()` in `RTFrontendWidgets.h`: `IsEmpty()`
	 * restituisce `bool` e lo slot vuole un `ESlateVisibility`, quindi il menu dei binding lo **filtra
	 * via** e chi cerca non lo trova.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	ESlateVisibility GetEmptyNoticeVisibility() const;

protected:
	/** Il subsystem del replay, o `nullptr` fuori da una `GameInstance`. */
	URTReplayViewerSubsystem* Replay() const;

	/** Il navigatore, o `nullptr` fuori da una `GameInstance`. */
	URTFrontendNavigator* Navigator() const;

private:
	/** Aggiornato da `LoadMatches`: la vista non ricalcola, e `IsEmpty` non rilegge il disco. */
	UPROPERTY(Transient)
	bool bLastLoadWasEmpty = true;
};

/**
 * Il **viewer** di un replay: si apre su una partita gia' scelta, e la si guarda.
 *
 * 🔴 **Non e' raggiungibile senza un `MatchId`**, e questo tipo lo rende vero invece di prometterlo:
 * `OpenSelected` consuma la selezione dichiarata da `MatchHistory`, e senza quella non apre niente. Una
 * schermata che si aprisse su nulla sarebbe il dead-end che §3.2 vieta.
 *
 * ⛔ **I comandi non sono qui.** Play, pausa, salti di turno e fase, posizione e bordi sono
 * `UFUNCTION` del `URTReplayViewerSubsystem` da `#999`, e un widget li chiama direttamente. Ricopiarli qui
 * darebbe due superfici per la stessa cosa, e la seconda divergerebbe.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTReplayViewerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Apre la partita che `MatchHistory` ha scelto, **con gli occhi di chi l'ha giocata**.
	 *
	 * Passa da `OpenMatchAsRecordedObserver` ([D-317], `#2156`) e non da `OpenMatch`: l'osservatore lo
	 * dichiara l'archivio, e questa schermata non ha modo di conoscerlo. ⚠️ Chiamare `OpenMatch` qui
	 * darebbe la vista dello spettatore **neutrale** — legittima per chi la sceglie, sbagliata per chi ci
	 * finisce senza averlo chiesto.
	 *
	 * Si chiama alla comparsa. Senza una selezione risponde `ManifestUnreadable`, che e' l'esito onesto:
	 * non c'e' un archivio da leggere.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult OpenSelected();

	/**
	 * Torna alla **lista**, e chiude l'archivio.
	 *
	 * ⚠️ **La chiusura e' della schermata e non del navigatore**, che non conosce il replay: senza,
	 * `FRTReplaySession::Traces` — il TurnLog di ogni turno registrato — resterebbe in memoria per tutta la
	 * vita del processo, perche' un subsystem di `GameInstance` sopravvive a ogni caricamento di livello.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool Back();

	/** La decisione dietro `OpenSelected`, interrogabile. Vedi `SelectAndNavigate` per il perche' statica. */
	static ERTReplayOpenResult OpenSelectedOn(URTReplayViewerSubsystem* Replay);

	/**
	 * La decisione dietro `Back`, interrogabile.
	 *
	 * ⛔ **L'archivio si chiude SOLO se si e' usciti davvero.** Su una navigazione rifiutata si resta nel
	 * viewer, e chiuderlo lascerebbe una schermata viva davanti a una sessione svuotata — cioe' un replay
	 * a schermo che non ha piu' niente da mostrare.
	 */
	static bool BackToListOn(URTReplayViewerSubsystem* Replay, URTFrontendNavigator* Navigator);

	/**
	 * Il messaggio per un'apertura fallita, gia' distinto nei **quattro** esiti (criterio di `#472`).
	 *
	 * ⛔ **Il testo lo compone questa funzione e non il Blueprint**, per la stessa ragione di
	 * `GetPhaseText()`: quattro rami in un graph node sono quattro occasioni di scriverne tre.
	 * Vuoto su `Opened`: non c'e' niente da spiegare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	static FText GetOpenFailureText(ERTReplayOpenResult Result);

protected:
	/** Il subsystem del replay, o `nullptr` fuori da una `GameInstance`. */
	URTReplayViewerSubsystem* Replay() const;

	/** Il navigatore, o `nullptr` fuori da una `GameInstance`. */
	URTFrontendNavigator* Navigator() const;
};
