#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Perception/RTTeamKnowledge.h" // ERTTargetKnowledge: la regola di [D-223] decide chi ha un nome
#include "RTReactionWindowView.generated.h"

/**
 * Che cosa ha aperto una finestra di reazione, detto da chi la apre (`#3667`).
 *
 * 🔑 **Lo scrive il PRODUTTORE, e la presentazione lo legge.** Il sottotitolo cambia frase a seconda di cio'
 * che e' successo — qualcuno e' entrato nell'area controllata, qualcuno ti spinge — e chi apre la finestra lo
 * sa. La prima stesura lo deduceva nel testo dall'id della reazione (`Action.Overwatch`, `Action.Brace`): una
 * terza copia di quegli id, che una reazione nuova avrebbe lasciato senza frase in silenzio.
 *
 * Non e' informazione futura: dice che cosa e' APPENA successo a chi decide.
 */
UENUM(BlueprintType)
enum class ERTReactionTriggerKind : uint8
{
	/** Nessuna specie dichiarata: la finestra non ha un sottotitolo. E' il default, e cio' che riceve un avversario. */
	None,
	/** Una o piu' unita' sono entrate nell'area che chi decide controlla (Overwatch). */
	EnteredControlledArea,
	/** Chi decide sta per essere spinto (`Brace`). */
	Pushed
};

/**
 * Una risposta legale dentro una finestra: la stringa ESATTA da rimandare al core, e il bersaglio quando ne
 * ha uno.
 *
 * 🔴 **Ogni risposta e' rappresentabile, e non solo le `FIRE:<id>`.** La prima stesura elencava i soli
 * bersagli, e sarebbe stata muta su ogni finestra che non fosse un Overwatch: `RTTurnManager_Blast.cpp`
 * costruisce finestre del `Brace` da `URTCatalogLibrary::BraceExecutableResponses`, che offre
 * `Hold Ground` piu' le maneuver eseguibili del profilo — nessuna col prefisso `FIRE:`. Il widget avrebbe
 * mostrato un countdown **senza un bottone da premere**, e ogni finestra del `Brace` sarebbe scaduta da
 * sola. Trovato in code review, non dalla suite: i tre test partivano tutti da una finestra dell'Overwatch.
 *
 * La stringa viaggia con l'opzione invece di essere ricomposta da chi disegna: `FIRE:<indice>` e' un
 * FORMATO, e il suo unico produttore e' `URTReactionOpportunityLibrary::FireResponse`. Un widget che se la
 * componesse da solo sarebbe il secondo produttore dello stesso formato — la duplicazione che
 * `FireResponseTarget` esiste per non far nascere, spostata di un livello e fuori dai test del core.
 */
USTRUCT(BlueprintType)
struct FRTReactionWindowOptionView
{
	GENERATED_BODY()

	/** La risposta da rimandare al core. Si spedisce, non si interpreta: e' l'unico campo sempre valorizzato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FString Response;

	/**
	 * Il bersaglio, quando la risposta ne ha uno; `INDEX_NONE` per le risposte che non bersagliano —
	 * `HOLD`, `Hold Ground`, una maneuver del `Brace`.
	 *
	 * 🔴 **E' un indice nello spazio di `MakeCurrentSnapshot`, che scarta i morti — NON un id stabile.** Il
	 * pericolo e' gia' documentato in rosso in `RTTurnManager_Chunk.Blast` per `Key.OwnerId`: *«un solo
	 * caduto che ordina prima di questa unita' sposta di uno tutti gli indici a valle»*, e chi risolvesse
	 * l'indice su un roster, su `StableUnitId` o su una lista di Actor nominerebbe **l'unita' sbagliata** in
	 * ogni partita in cui qualcuno e' gia' caduto. Si risolve solo contro lo stesso array che ha costruito
	 * la finestra.
	 *
	 * ⚠️ Percio' il nome dice `SnapshotIndex` e non `UnitId`: la prima stesura lo chiamava `UnitId`, ed era
	 * un invito a risolverlo nel posto sbagliato. Portare qui un id stabile e' lavoro del PRODUTTORE, che ha
	 * lo snapshot in mano — non di questa funzione, che non ce l'ha.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	int32 TargetSnapshotIndex = INDEX_NONE;

	/**
	 * Il nome del bersaglio, quando la risposta ne ha uno: «Ivrin» per un `FIRE:<i>`. Vuoto altrimenti.
	 *
	 * 🔑 **E' la risposta al problema che `TargetSnapshotIndex` dichiara qui sopra** (`#3667`): l'indice non
	 * si puo' risolvere fuori dall'array che ha costruito la finestra, quindi il nome lo risolve il
	 * PRODUTTORE, in quell'istante, e arriva qui come testo gia' scritto. Chi disegna non risolve niente.
	 *
	 * ⚠️ Non e' informazione nuova per chi lo riceve: e' l'unita' a cui quella risposta spara, e la
	 * risposta e' gia' sua.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FText TargetName;
};

/**
 * I nomi che il PRODUTTORE di una finestra risolve nell'istante in cui la costruisce (`#3667`).
 *
 * Non e' una `USTRUCT` e non attraversa nessun confine: e' l'argomento con cui chi ha in mano lo snapshot
 * — il turn manager — passa i nomi a `FilterWindowForTeam`, che decide a chi consegnarli. Un avversario
 * non ne riceve nessuno, perche' il filtro esce prima di leggerli.
 */
struct FRTReactionWindowNames
{
	/** Il nome della reazione nel kit di chi decide: «Guardia reattiva». Vuoto se il kit non la nomina. */
	FText ReactionName;

	/** Il nome dell'unita' che decide: «Aevik». */
	FText ReactorName;

	/**
	 * Chi ha fatto scattare la finestra, nell'ordine in cui il produttore li ha trovati.
	 *
	 * 🔴 **Entra solo chi la squadra del proprietario RILEVA in quell'istante** (R2.2). Per l'Overwatch e'
	 * vero per costruzione: il trigger esiste solo per un'unita' `Detected` e in linea di vista. Per il
	 * `Brace` no — chi spinge puo' farlo da una cella che la squadra non vede — e il nome passa da
	 * `URTReactionWindowLibrary::AddTriggerIfKnown`, che e' l'unico punto in cui la regola e' scritta.
	 *
	 * ⚠️ **Non e' l'elenco dei bersagli.** Una condizione dichiarata riduce le risposte `FIRE`, non chi e'
	 * entrato: con «solo sotto meta' vita», due unita' entrano e a una sola si puo' sparare.
	 */
	TArray<FText> TriggerNames;

	/** Che cosa ha aperto la finestra. Lo scrive il sito che la apre. */
	ERTReactionTriggerKind TriggerKind = ERTReactionTriggerKind::None;

	/** Il nome di ogni bersaglio di un `FIRE:<i>`, per indice di snapshot. Si legge per chiave, non si itera. */
	TMap<int32, FText> TargetNames;
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
	 *
	 * 🔴 **`UPROPERTY()` semplice, NON `BlueprintReadOnly`, e la differenza e' una decisione.** Esporlo ai
	 * Blueprint avrebbe richiesto di marcare `BlueprintType` la chiave AUTOREVOLE — cioe' allargare in
	 * permanenza la superficie di riflessione di un tipo del resolver per un consumatore che questo stesso
	 * file dichiara futuro. E' lo stesso argomento con cui il campo «posso rispondere?» e' stato rifiutato,
	 * e va applicato anche quando costa a me: la prima stesura aveva allargato la chiave, e una code review
	 * ha fatto notare che un `UPROPERTY()` semplice compila lo stesso. Il giorno in cui un widget dovra'
	 * leggerla, si esporra' allora — con il consumatore davanti.
	 */
	UPROPERTY()
	FRTReactionOpportunityKey Key;

	/**
	 * Durata della finestra, in secondi, **server-authoritative** (ADR-0004 §8, 3,0 s).
	 *
	 * Il valore arriva da chi apre la finestra e non si calcola qui: un client lento non allunga la finestra,
	 * e la presentazione non ha voce sulla sua durata. Zero quando la finestra non e' aperta.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	float WindowSeconds = 0.f;

	/**
	 * Le risposte offerte, **tutte**, nell'ordine dell'opportunity. Vuoto quando la finestra non e' aperta.
	 *
	 * Include la scelta sicura: `SafeResponse` dice QUALE delle opzioni e' — non e' una seconda lista. Un
	 * elenco che la escludesse costringerebbe il widget a ricomporre il bottone «tieni» da solo, e il suo
	 * nome non e' universale (§`SafeResponse`).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	TArray<FRTReactionWindowOptionView> Options;

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

	// --- I nomi (`#3667`): testo gia' scritto dal produttore, mai risolto da chi disegna -----------------
	//
	// 🔑 **Si risolvono dove l'indice di snapshot VALE**, cioe' nell'istante in cui il turn manager
	// costruisce la vista. Da `#3666` la vista si costruisce prima e si consegna dopo: fra i due momenti
	// questi campi sono testo, e non si ricalcolano. Un'unita' che cade nel frattempo non cambia il titolo.
	//
	// ⚠️ **Non sono informazione futura**, ed e' la ragione per cui `OpportunityLeaksNoFuture` li ammette:
	// nominano la reazione e l'unita' di chi decide, che sono sue, e le unita' che la sua squadra rileva gia'.
	//
	// 🔴 **Tutto questo vale se la vista arriva alla squadra di chi decide, e oggi non lo verifica nessuno alla
	// CONSEGNA.** I due siti la costruiscono per la squadra del proprietario e la spediscono a un delegate
	// single-cast, legato al view model del giocatore locale. Che quel giocatore sia della stessa squadra lo
	// garantisce il solo `!bOwnerIsBot`, che in v0.1 — un umano, due unita' ([D-155]) — coincide col gate di
	// squadra. Il giorno in cui un processo ospita unita' umane di due squadre, questa vista arriverebbe
	// all'avversario: prima di `#3667` portava l'esistenza della finestra e le risposte, ora anche i nomi.
	// La riga da rileggere quel giorno sta nell'intestazione di `URTReactionWindowViewModel`.

	/** Il nome della reazione, dal kit di chi decide: «Guardia reattiva». Vuoto se il kit non la nomina. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FText ReactionName;

	/** Il nome dell'unita' che decide: «Aevik». */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	FText ReactorName;

	/**
	 * Chi ha fatto scattare la finestra. Vuoto quando nessuno e' nominabile.
	 *
	 * 🔴 **Un nome avversario sta qui solo se la squadra di chi decide lo rileva** (R2.2). Per il `Brace`
	 * spinto da un'unita' che la squadra non vede l'elenco resta vuoto, e il sottotitolo dice «Un nemico ti
	 * spinge»: il DTO non porta il nome, quindi nessun widget puo' mostrarlo per sbaglio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	TArray<FText> TriggerNames;

	/**
	 * Che cosa ha aperto la finestra: e' cio' che sceglie la frase del sottotitolo. `None` quando la finestra
	 * non e' aperta, o quando chi la apre non lo ha dichiarato.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	ERTReactionTriggerKind TriggerKind = ERTReactionTriggerKind::None;
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
	 * - Osservatore della squadra del proprietario -> vista completa: chiave, countdown, opzioni, scelta
	 *   sicura. La DoD chiede che la finestra sia *«visibile in sola lettura alla squadra»*, e in v0.1 —
	 *   2v2 offline, un umano per squadra ([D-155]) — squadra e proprietario coincidono. La distinzione fra
	 *   «vedo» e «decido» nascera' con il secondo giocatore per squadra, e sara' un campo in piu' con un
	 *   produttore vero; oggi sarebbe una costante replicata.
	 * - Squadra non risolta (`INDEX_NONE`, da un lato o dall'altro) -> **vista ai default**. Fail-closed, e
	 *   non e' teorico: `INDEX_NONE` e' il default dei team id in tutto il progetto, e senza questa guardia
	 *   un osservatore ignoto e un proprietario ignoto **coinciderebbero** — l'uguaglianza fra due valori
	 *   che significano «non lo so» avrebbe consegnato la finestra intera. Segnalato in code review.
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
	 *
	 * ➕ **`Names`** (`#3667`): i nomi che il produttore ha risolto dal proprio snapshot. Entrano nella vista
	 * SOLO se la vista si apre: i rifiuti qui sopra escono prima di leggerli, quindi un avversario non
	 * riceve un campo in piu' — `FilterWindowForTeam` resta l'unico filtro (R2.4).
	 */
	static FRTReactionWindowView FilterWindowForTeam(int32 ObserverTeamId, int32 OwnerTeamId,
		const FRTReactionOpportunity& Opportunity, float WindowSeconds,
		const FRTReactionWindowNames& Names = FRTReactionWindowNames());

	/**
	 * Aggiunge a `Names` il nome di chi ha fatto scattare la finestra, **solo se la squadra del proprietario
	 * lo conosce** (R2.2, `#3667`). E' l'unico punto in cui la regola e' scritta.
	 *
	 * 🔑 **`OwnerTeamKnowledge` e' cio' che `URTTeamKnowledgeLibrary::ClassifyTarget` risponde** per quell'unita',
	 * con la conoscenza della squadra di chi decide: la regola di [D-223], la stessa con cui si congela il
	 * verdetto di una riga del registro. Solo `Allowed` nomina — un'unita' rilevata, o un compagno. `CellOnly`
	 * e' un contatto ricordato: la squadra sa dove lo ha visto l'ultima volta, non che e' lui ADESSO, e dargli un
	 * nome glielo direbbe.
	 *
	 * ⏱️ *Fino alla seconda revisione di `#3739` prendeva un `ERTAwareness`, che il sito del `Brace` calcolava da
	 * se' con gli osservatori vivi DOPO i danni. Era un secondo predicato accanto a quello del Blast.*
	 *
	 * @return vero se il nome e' entrato.
	 */
	static bool AddTriggerIfKnown(FRTReactionWindowNames& Names, const FText& Name,
		ERTTargetKnowledge OwnerTeamKnowledge);
};
