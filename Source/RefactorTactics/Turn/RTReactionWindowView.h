#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "RTReactionWindowView.generated.h"

/**
 * Un bersaglio proponibile dentro una finestra di reazione: chi e', e la stringa ESATTA che lo sceglie.
 *
 * La stringa viaggia insieme all'id invece di essere ricostruita da chi disegna: `FIRE:<UnitId>` e' un
 * FORMATO, e il suo unico produttore e' `URTReactionOpportunityLibrary::FireResponse`. Un widget che se la
 * componesse da solo sarebbe il secondo produttore dello stesso formato — la stessa duplicazione che
 * `FireResponseTarget` esiste per non far nascere, spostata di un livello e fuori dai test del core.
 */
USTRUCT(BlueprintType)
struct FRTReactionWindowTargetView
{
	GENERATED_BODY()

	/** Unita' bersagliabile. E' gia' nota a chi guarda: ha appena armato il trigger dentro la sua zona. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	int32 UnitId = INDEX_NONE;

	/** La risposta da rimandare al core per colpire questo bersaglio. Si spedisce, non si interpreta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FString Response;
};

/**
 * La finestra di reazione **come la riceve un osservatore**: il DTO sanitizzato della DoD di CP 14.6
 * (`#166`) — *«UI FIRE/HOLD con countdown e bersaglio, alimentata da un DTO sanitizzato: nessuna logica di
 * gioco nel widget»*.
 *
 * 🔴 **Per un avversario questa struttura e' vuota, e vuota significa che la finestra NON ESISTE.** La DoD
 * chiede che *«l'avversario non riceva nulla, nemmeno l'esistenza della finestra»*: non un countdown a zero,
 * non un elenco di bersagli vuoto da nascondere in Blueprint. E' la stessa disciplina di
 * `URTIntentPrivacyLibrary::FilterForTeam`, dove un avversario non riceve una riga vuota ma **nessuna riga**.
 *
 * ⚠️ **L'elenco dei campi e' chiuso**, e il guardiano e' `RefactorTactics.Overwatch.OpportunityLeaksNoFuture`
 * — lo stesso che chiude `FRTReactionOpportunity`, esteso invece che duplicato. Chi aggiunge un campo qui
 * passa di la' e dichiara perche' non e' informazione futura.
 */
USTRUCT(BlueprintType)
struct FRTReactionWindowView
{
	GENERATED_BODY()

	/**
	 * C'e' una finestra da mostrare a questo osservatore?
	 *
	 * Falso in tre casi che per il widget sono lo stesso: l'osservatore e' avversario, l'opportunity non apre
	 * un boundary (`AllowedResponses` ≤ 1 — ADR-0004 §2, e la regola resta di
	 * `RequiresDecisionBoundary`), oppure non c'e' nessuna opportunity. Tenerli distinti nel DTO
	 * significherebbe dire a un avversario *quale* dei tre e' — cioe' rispondergli.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	bool bOpen = false;

	/**
	 * Identita' della finestra a cui si sta rispondendo.
	 *
	 * E' la chiave autorevole, non una copia ridotta: i suoi sei campi sono gia' dichiarati non-futuri da
	 * `OpportunityLeaksNoFuture`, e ricostruirne una versione «per la UI» avrebbe creato un secondo
	 * identificatore per la stessa finestra — che e' il difetto che `FRTReactionOpportunityKey` esiste per
	 * impedire. Serve a correlare la risposta quando il decisore umano sara' asincrono: oggi
	 * `AskReactionDecision` e' sincrono e la correlazione e' implicita.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FRTReactionOpportunityKey Key;

	/**
	 * Durata della finestra, in secondi, **server-authoritative** (ADR-0004 §8, 3,0 s).
	 *
	 * Il valore arriva da chi apre la finestra e non si calcola qui: un client lento non allunga la finestra,
	 * e la presentazione non ha voce sulla sua durata. Zero quando la finestra non e' aperta.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	float WindowSeconds = 0.f;

	/** I bersagli offerti, nell'ordine dell'opportunity. Vuoto quando la finestra non e' aperta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	TArray<FRTReactionWindowTargetView> Targets;

	/**
	 * Cio' che si applica allo scadere del countdown, per nome: `HOLD` nell'Overwatch, `Hold Ground` nel
	 * `Brace` ([D-047]).
	 *
	 * ⚠️ **Non e' la costante `HOLD`**, ed e' il motivo per cui il campo esiste. Un widget che scrivesse
	 * `HOLD` da solo sarebbe corretto oggi e sbagliato con la prima finestra che non e' un Overwatch — la
	 * regressione che `SafeResponse` ha gia' evitato una volta nel core, il 2026-08-19. Qui il valore si
	 * LEGGE da `URTReactionOpportunityLibrary::SafeResponse`, che resta l'unico posto in cui si decide.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FString SafeResponse;
};

/**
 * La query che produce il DTO di una finestra di reazione (CP 14.6, `#166`).
 *
 * Vive in `Turn/` accanto a `URTIntentPrivacyLibrary` e non in `UI/`: e' una **deterministic query** del
 * core, non un view model di presentazione. Il criterio e' quello del filtro degli intenti — la privacy si
 * applica COSTRUENDO la vista di chi guarda, invece di consegnare lo stato completo e chiedere alla UI di
 * comportarsi bene. Un widget non puo' rispettare una regola che non conosce, e non deve doverla conoscere.
 *
 * Funzione PURA: nessun Actor, nessun `UWorld`, nessun accesso al `TurnManager`. Cosi' la regola di privacy
 * si verifica headless, che e' l'unica forma in cui questo checkpoint puo' essere misurato senza Editor.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionWindowLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * La finestra come la riceve `ObserverTeamId`, sapendo che il proprietario e' di `OwnerTeamId`.
	 *
	 * - Osservatore della squadra del proprietario -> vista completa: chiave, countdown, bersagli, scelta
	 *   sicura. La DoD chiede che la finestra sia *«visibile in sola lettura alla squadra»*, e in v0.1 —
	 *   2v2 offline, un umano per squadra ([D-155]) — squadra e proprietario coincidono. La distinzione fra
	 *   «vedo» e «decido» nascera' con il secondo giocatore per squadra, e sara' un campo in piu' con un
	 *   produttore vero; oggi sarebbe una costante replicata.
	 * - Osservatore avversario -> **vista ai default**: `bOpen` falso e nient'altro. Non un countdown a zero
	 *   da nascondere: proprio nessun dato.
	 * - Opportunity senza boundary (`AllowedResponses` ≤ 1) -> vista ai default anche per la propria squadra:
	 *   non c'e' niente da scegliere, e mostrare una finestra che il resolver non apre insegnerebbe al
	 *   giocatore un'attesa che non esiste.
	 *
	 * `WindowSeconds` si passa e non si legge da nessuna parte qui: la fonte e'
	 * `ARTTurnManager::GetFastReactionDuration()`, e tenerla fuori da questa funzione e' cio' che la rende
	 * pura — e testabile senza montare una partita.
	 *
	 * 🔴 **Non e' una `UFUNCTION`, e non e' una dimenticanza.** Prenderebbe `FRTReactionOpportunity` come
	 * parametro, e per esporla ai Blueprint bisognerebbe rendere `BlueprintType` l'opportunity AUTOREVOLE —
	 * cioe' mettere `AllowedResponses` a portata di un widget, che e' esattamente cio' che questo filtro
	 * esiste per impedire. Il chiamante e' il core, che ha gia' l'opportunity in mano; la presentazione
	 * riceve il **risultato**, che e' `BlueprintType` per intero. UHT lo ha rifiutato al primo tentativo, e
	 * aveva ragione.
	 */
	static FRTReactionWindowView FilterWindowForTeam(int32 ObserverTeamId, int32 OwnerTeamId,
		const FRTReactionOpportunity& Opportunity, float WindowSeconds);
};
