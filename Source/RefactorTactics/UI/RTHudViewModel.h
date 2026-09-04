#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Ability/RTActionDef.h" // ERTActionSlot: la vista dei cooldown raggruppa per slot
// `FRTPlannedIntent` serve COMPLETO, non in forward declaration: `BuildAuthoritativeIntents` lo
// restituisce dentro un `TArray` per valore, e il distruttore del container pretende il tipo definito.
#include "Turn/RTIntentPrivacyLibrary.h"
#include "RTHudViewModel.generated.h"

class AActor;
class ARTTurnManager;
class ARTUnit;

/**
 * Lo stato dell'intestazione di partita: round, fase, tempo.
 *
 * Esiste perche' il §4.1 di `progettazione-hud.md` vieta ai widget di ricalcolare, e senza una vista
 * dichiarata l'unica alternativa e' che `WBP_RT_TurnHeader` legga `ARTTurnManager` da solo — cioe' che la
 * regola resti una disciplina da ricordare invece di una proprieta' della firma.
 */
USTRUCT(BlueprintType)
struct FRTMatchHeaderView
{
	GENERATED_BODY()

	/** Round corrente, 1-based come lo mostra il gioco. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Round = 0;

	/**
	 * Limite di round del **formato in vigore**, mai una costante.
	 *
	 * `0` significa «nessun limite dichiarato» e NON va mostrato come «su 0»: una partita senza formato non
	 * e' una partita gia' scaduta. E' la stessa distinzione che `ARTHUD` fa oggi in Canvas.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 RoundLimit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	/**
	 * Secondi che restano al Planning. **Negativo** quando la domanda non si applica — fuori dal Planning,
	 * o senza timer. Un `0.f` direbbe «scaduto adesso», che e' un'altra cosa.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float PlanningSecondsRemaining = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bResolving = false;

	/**
	 * Il progresso sull'OBIETTIVO contendibile, una voce per squadra — `CP 10.2`, `#75`.
	 *
	 * Il punteggio esiste e si registra nel Cleanup da quando `RTTurnManager` valuta l'obiettivo, ma fino a
	 * `#75` non aveva NESSUN lettore di interfaccia: `GetTeamScore` era letto solo dallo Scenario Harness e
	 * dall'hash di stato. La DoD chiedeva che il progresso comparisse «nell'HUD **e** nel TurnLog», e la
	 * metà nel TurnLog era l'unica soddisfatta.
	 *
	 * ⚠️ **Interi, come nel resolver.** Il tipo qui non e' una scelta di presentazione: un `float`
	 * renderebbe il punteggio mostrato dipendente dall'ordine delle somme, e la vista smetterebbe di poter
	 * dire la stessa cosa che dice il TurnLog. `int32` a entrambe le estremita' è cio' che rende la vista
	 * verificabile contro il log invece che solo simile.
	 *
	 * 🔴 **Significa qualcosa solo se la MAPPA dichiara un obiettivo**, e il tipo non lo dice: chi legge
	 * questo campo deve saperlo da fuori. Su una mappa che non ne ha, `0-0` non e' una parita' — e' un
	 * punteggio inventato per una gara che nessuno sta correndo, e il resolver lo evita gia': non scrive la
	 * voce di TurnLog senza `HasObjectiveCell()`, e `Objectives.SilentWithoutObjectiveCell` lo misura.
	 *
	 * La condizione si chiede a `URTHexMapAsset::HasObjectiveCell()`, che si raggiunge da
	 * `ARTHexMapActor::FindInWorld(World)`. Il percorso Canvas la rispetta — `ARTHUD::ComposeMatchStatusLine`
	 * la riceve come parametro e tace senza — e `RefactorTactics.HUD.MatchStatusShowsObjectiveOnlyWhenMapDeclaresOne`
	 * la misura. ⚠️ **Un binding Blueprint su questo campo NON e' protetto da niente**: misurato il
	 * 2026-09-04, oggi nessun `WBP_*` lo lega, e portare la condizione dentro il tipo e' **#2281**.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Team0Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Team1Score = 0;

	/**
	 * Punti che servono per vincere, dal **formato in vigore** — mai una costante, come `RoundLimit`.
	 *
	 * `0` significa «via per obiettivo **disattivata**», che è il valore della v0.1, e NON va mostrato come
	 * «su 0»: una partita che non si vince per obiettivo non è una partita già vinta. È la stessa
	 * distinzione che `RoundLimit` fa qui sopra, e il motivo per cui questo campo esiste separato dai due
	 * punteggi: senza di lui un widget non può sapere se `1` sia molto o niente.
	 *
	 * 🔴 **Stessa precondizione dei due punteggi qui sopra**: senza un obiettivo dichiarato dalla mappa
	 * questa soglia non ha soggetto, e mostrarla scriverebbe un traguardo per una gara che nessuno corre.
	 * E' una reticenza in piu' rispetto a `RoundLimit`, che invece dipende dal solo formato. Vedi **#2281**.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 ScoreToWin = 0;
};

/**
 * Una unita' come la vede il pannello: salute, scudo, identita'.
 *
 * Non contiene intenti ne' piani: quelli hanno gia' `FRTIntentView` e la loro privacy e' verificata la'
 * (invariante #6). Duplicarli qui significherebbe due filtri da tenere allineati.
 */
USTRUCT(BlueprintType)
struct FRTUnitCardView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 MaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bIsAlly = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bAlive = false;
};

/**
 * Uno dei tre slot del turno, come lo vede il pannello: occupato o libero, e **da cosa**.
 *
 * «Occupato» e «da cosa» sono due dati distinti perche' non coincidono sempre: un movimento normale occupa
 * lo slot movimento e **non ha un `ActionId`** — il piano lo rappresenta come una lista di waypoint
 * (`PlannedWaypoints`), non come un'azione scelta. Uno scatto invece e' un'azione, e si puo' nominare.
 * Un widget che avesse solo `ActionId` mostrerebbe lo slot movimento vuoto per chi ha appena tracciato un
 * percorso, che e' il caso piu' comune del gioco.
 */
USTRUCT(BlueprintType)
struct FRTPlannedSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bOccupied = false;

	/** `None` quando lo slot e' libero **o** quando cio' che lo occupa non e' un'azione (un percorso). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName ActionId;

	/** Vuoto quando `ActionId` e' `None`: il widget mette l'etichetta generica dello slot. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FText DisplayName;
};

/**
 * I tre slot di un turno: movimento, azione principale, reazione.
 *
 * ⚠️ **Non sono tre booleani indipendenti.** Un'azione che dichiara `MovementAndMain` ne occupa **due**,
 * ed e' il caso che rende sbagliata la mappatura ovvia «un'azione, uno slot» - nessuna lo dichiara oggi
 * (`Action.Sprint` lo faceva fino a [D-028]), ma una vista non puo' assumerlo. Chi decide resta
 * `URTCatalogLibrary::TakesMovementSlot`/`TakesMainSlot`, gli stessi predicati che usa il validatore del
 * piano: qui non si riscrive la regola, la si interroga.
 */
USTRUCT(BlueprintType)
struct FRTUnitSlotsView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTPlannedSlotView Movement;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTPlannedSlotView Main;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTPlannedSlotView Reaction;
};

/**
 * Uno stato temporaneo da mostrare sopra un'unita': quale, con che icona, per quanto ancora (`#2274`).
 *
 * 🔑 **Porta l'`IconId` e non lascia che sia chi disegna a comporlo.** La chiave si deriva dal tag con
 * `URTIconLibrary::MakeIconId`, che e' l'owner della regola; comporla nel widget significherebbe una
 * seconda derivazione, e un widget in Blueprint e' il posto con **meno** copertura del progetto.
 */
USTRUCT(BlueprintType)
struct FRTStatusBadgeView
{
	GENERATED_BODY()

	/** Il tag dello stato (`Status.Burning`, …). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName Tag;

	/** La chiave dell'icona nel catalogo (`UI.Icon.Status.Burning`), gia' derivata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName IconId;

	/**
	 * Turni residui, `> 0`.
	 *
	 * 🔴 **Vale `0` quando lo stato e' legato alla cella**, e li' **non c'e' un conteggio da mostrare**:
	 * dura finche' l'unita' resta dov'e'. Chi disegna guarda `bCellBound` PRIMA di stampare un numero — il
	 * `-1` di `ARTUnit::PersistentWhileOnCell` non arriva mai fin qui, proprio perche' scritto sopra la
	 * testa di un'unita' si leggerebbe come «meno un turno».
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 RemainingTurns = 0;

	/** Vero se lo stato dura finche' l'unita' resta sulla cella: allora `RemainingTurns` non significa nulla. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bCellBound = false;

	/**
	 * Vero se e' uno stato di CONTROLLO (`Root`, `Slow`): la famiglia che
	 * `URTReactionLibrary::ControlStatusesBySeverity` ordina per gravita', e che questa vista mette per prima.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bIsControl = false;
};

/**
 * La sovrapposizione sopra un'unita', come un widget la riceve gia' composta (`#2288`, `D-320`).
 *
 * 🔴 **Esiste perche' il widget non debba comporre nulla.** Un `UserWidget` in Blueprint ha copertura
 * headless **zero**: ogni `if` scritto li' dentro e' un `if` che nessun test vede. Qui invece tutto e' un
 * campo, e chi disegna sceglie solo *dove* metterlo.
 */
USTRUCT(BlueprintType)
struct FRTUnitOverlayView
{
	GENERATED_BODY()

	/** Il nome da mostrare, gia' risolto (`ARTUnit::DisplayLabel`): il nome canonico del catalogo, non l'ID. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FString DisplayName;

	/** Vita e scudo: la stessa vista che il pannello di squadra usa gia'. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTUnitCardView Card;

	/** Gli stati attivi, gia' ordinati e con la durata (`#2274`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	TArray<FRTStatusBadgeView> Statuses;

	/**
	 * Il colore di squadra **dal punto di vista di chi guarda**: alleato o avversario, non «team 0/team 1».
	 *
	 * ⚠️ Deriva da `Card.bIsAlly`, quindi da `PlayerTeamId`: la stessa unita' e' due colori diversi per due
	 * osservatori, ed e' voluto.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FLinearColor TeamColor = FLinearColor::White;

	/**
	 * Questa unita' e' dentro l'area di un piano d'attacco della PROPRIA squadra: **fuoco amico**.
	 *
	 * 🔴 **La cosa piu' importante che questa vista porta, e per poco andava persa.** L'avviso nasce da
	 * un'osservazione in PIE del 2026-08-08 — *«non capisco se sto facendo un tiro e se nel tiro si
	 * interseca con un cilindro»* — e vive sull'UNITA' e non solo sulla cella, perche' la domanda che ci si
	 * fa guardando lo schermo e' «questo cilindro lo prendo o no?».
	 *
	 * ⚠️ Si calcola dai PIANI e non dall'anteprima dell'unita' selezionata: l'avviso deve restare acceso
	 * anche mentre si seleziona qualcun altro per muoverlo — cioe' proprio mentre si finisce il turno.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bFriendlyFire = false;

	/** Questa unita' e' dentro l'area di un piano d'attacco: e' un bersaglio. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bTargeted = false;
};

/** La ricarica residua di una singola azione del kit, in TURNI INTERI. */
USTRUCT(BlueprintType)
struct FRTAbilityCooldownView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FText DisplayName;

	/** Indice nel kit dell'unita': e' cio' che l'hotkey arma, quindi il widget ne ha bisogno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 AbilityIndex = INDEX_NONE;

	/** Quale slot consuma: il pannello raggruppa per slot, non per ordine nel kit. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	ERTActionSlot Slot = ERTActionSlot::None;

	/** Turni interi che mancano. `0` = ricarica finita. **Mai negativo.** */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 TurnsRemaining = 0;

	/**
	 * La ricarica DICHIARATA dall'azione. `0` = l'azione non ne ha una.
	 *
	 * 🔴 **Esiste perche' `TurnsRemaining` da solo e' un numeratore senza denominatore** (`#1896`). Un
	 * widget che voglia disegnare una barra proporzionale deve pur prendere il totale da qualche parte, e
	 * l'unica fonte a portata di Blueprint era `URTActionData::CooldownTurns` — il cui default e' **`0`**
	 * per la maggioranza del kit (attacco base, `Move`, `Guard`, `Brace`). Il risultato misurato in PIE e'
	 * un `Divide by zero: Divide_DoubleDouble` a ogni selezione: in Blueprint non interrompe nulla,
	 * restituisce **0**, e la barra dice «scarica» di un'abilita' pronta.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 TotalTurns = 0;

	/**
	 * Quanto la ricarica e' completa, in `[0,1]`. `1` = pronta.
	 *
	 * 🔑 **La divisione la fa il C++, ed e' il punto di `#1896`.** Un widget che divide `TurnsRemaining`
	 * per `TotalTurns` deve ricordarsi della guardia sullo zero; uno che legge questo campo non puo'
	 * sbagliare, perche' non c'e' niente da dividere. La guardia vive in un posto solo, testato.
	 *
	 * ⚠️ **Un'azione SENZA ricarica vale `1`, non `0`**: e' pronta per definizione, e zero si leggerebbe
	 * come «scarica» — cioe' esattamente il difetto che questo campo esiste per togliere. Chi vuole
	 * distinguere «non ha ricarica» da «ricarica completa» guarda `TotalTurns`, che per la prima e' `0`:
	 * sono due domande diverse e restano due campi.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float ChargeFraction = 1.f;

	/**
	 * Usabile **adesso**.
	 *
	 * ⚠️ Non era `TurnsRemaining == 0`: serviva anche l'energia. Da
	 * [D-324](../../../docs/decisions/RT_PDR_00_Decision_Log.md) le due condizioni **coincidono**, perche' il
	 * cooldown e' rimasto l'unico gate. Il campo resta perche' dichiara l'intenzione — *usabile* — invece di
	 * far dedurre a ogni lettore che uno zero in un contatore significhi permesso.
	 *
	 * I due dati restano separati perche' rispondono a domande diverse — «quanto manca?» e «posso adesso?»
	 * — anche ora che una implica l'altra: e' il contratto della vista a doverle distinguere, cosi' una
	 * seconda clausola futura entra in un campo che gia' esiste invece di doverne creare uno.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bUsableNow = false;
};

/**
 * Le viste che alimentano lo Screen HUD (§4.1 di `progettazione-hud.md`, CP 11.7).
 *
 * Statiche e pure per la stessa ragione per cui lo e' `ARTHUD::ComputePlannedHitMarks`: l'indipendenza dallo
 * stato del widget diventa una proprieta' della **firma**, non una regola che qualcuno deve ricordare. Un
 * widget che chiama queste funzioni non puo' sbagliare filtro, perche' il filtro non e' suo.
 *
 * ⚠️ Non e' il layer §4.2. Path, AoE, fuoco amico e le barre ancorate alle unita' restano in `ARTHUD`, dove
 * la spec li vuole — «non devono essere realizzati come grandi widget HUD statici».
 *
 * ⚠️ **`BlueprintPure` e non `BlueprintCallable`, e la differenza si vede nel widget.** Un nodo
 * `BlueprintCallable` porta gli exec pin, che in un **binding di proprieta'** UMG non si possono collegare:
 * il widget sarebbe costretto a chiamare la funzione in un evento e a **tenerne una copia** in una variabile
 * — cioe' esattamente la seconda verita' che questo view model esiste per non far nascere. Stesso motivo per
 * cui sono `BlueprintPure` `URTIntentPrivacyLibrary::FilterForTeam` e `URTIconLibrary::MakeIconId`.
 */
UCLASS()
class REFACTORTACTICS_API URTHudViewModel : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'intestazione. `TurnManager` nullo da' una vista neutra (round 0, nessun limite, timer negativo):
	 * un widget che parte prima del manager mostra «—», non un «Turno 0/0» che sembra un dato.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static FRTMatchHeaderView BuildMatchHeader(const ARTTurnManager* TurnManager);

	/**
	 * Il contatore del round come si mostra: `Round N/L` con un formato in vigore, `Round N` senza.
	 *
	 * 🔴 **Una sola sede, e non e' una preferenza di stile.** `RoundLimit == 0` significa «nessun limite
	 * dichiarato», e un `Round 3/0` si legge come una partita gia' scaduta: e' il punto esatto in cui un
	 * consumatore sbaglia. Fino a #2184 la regola viveva in due implementazioni — il binding UMG di
	 * `URTTurnHeaderWidget::GetRoundCounterText` e il Canvas di `ARTHUD` — che rendono **entrambe** a
	 * schermo con `rt.HUD.CanvasPanels` attivo: due contatori che potevano dissentire. Ora la decidono
	 * qui, e i due chiamanti la vestono (`FText` per UMG, concatenazione per il Canvas).
	 */
	static FString ComposeRoundCounter(const FRTMatchHeaderView& Header);

	/** La carta di una singola unita', vista da `PlayerTeamId`. Unita' nulla da' una carta vuota e non viva. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static FRTUnitCardView BuildUnitCard(const ARTUnit* Unit, int32 PlayerTeamId);

	/**
	 * Il roster laterale: **solo le unita' di `PlayerTeamId`**, nell'ordine in cui arrivano.
	 *
	 * La squadra avversaria non entra, e non per privacy — gli HP nemici sono gia' pubblici sopra le teste in
	 * `ARTHUD` — ma perche' il roster risponde a «chi comando io». Un elenco che mescola le due squadre
	 * costringe a leggere un colore per sapere di chi e' una riga, e la spec vuole la relazione di squadra
	 * distinguibile per **forma** prima che per tinta.
	 *
	 * Le unita' morte restano, con `bAlive = false`: sparire dall'elenco e' peggio che comparire barrato —
	 * il giocatore perde il conto di quanti ne aveva.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static TArray<FRTUnitCardView> BuildTeamRoster(const TArray<ARTUnit*>& Units, int32 PlayerTeamId);

	/**
	 * Gli slot occupati dal piano corrente dell'unita' (CP 11.1). Unita' nulla da' tre slot liberi.
	 *
	 * ⚠️ **Riporta, non arbitra.** Se un piano occupasse due volte lo stesso slot — cosa che
	 * `URTCatalogLibrary::ValidateActionSlots` dichiara errore — questa vista mostra cio' che c'e', non
	 * decide chi vince. La HUD non e' il posto dove si applica una regola di legalita': mostrarne una
	 * versione «pulita» nasconderebbe proprio il piano che il validatore rifiutera'.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static FRTUnitSlotsView BuildUnitSlots(const ARTUnit* Unit);

	/**
	 * La ricarica di **ogni** azione del kit, nell'ordine del kit (CP 11.1). Unita' nulla da' un elenco vuoto.
	 *
	 * I numeri si LEGGONO dal simulatore (`ARTUnit::GetAbilityCooldown`, `CanUseAbility`): il widget non ne
	 * tiene una copia, ed e' la voce di DoD che esiste per impedire la seconda verita' che si scollega al
	 * primo turno in cui qualcuno dimentica di aggiornarla.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static TArray<FRTAbilityCooldownView> BuildAbilityCooldowns(const ARTUnit* Unit);

	/**
	 * Quali stati mostrare sopra un'unita', **in che ordine** e con quale durata residua (`#2274`, `D-320`).
	 *
	 * 🔴 **Esiste perche' il giudizio non nasca dentro il widget.** Un `UserWidget` in Blueprint ha copertura
	 * headless **zero**; senza questa funzione dovrebbe interrogare `HasStatus` dieci volte per unita' a ogni
	 * fotogramma e inventarsi ordine e durata — cioe' la presentazione che ricostruisce un fatto che il
	 * simulatore ha gia' stabilito. E' la stessa disciplina di `ShouldDrawUnitOverlay`.
	 *
	 * ## L'ordine, e perche' non ne inventa uno nuovo
	 *
	 * 1. **prima i controlli, per gravita'** — chiedendola a `URTReactionLibrary::ControlSeverityRank`, che
	 *    ne e' l'owner (`0` = il piu' grave);
	 * 2. **poi gli altri**, nell'ordine gia' deterministico di `ARTUnit::GetActiveStatusTags`.
	 *
	 * 🔴 **Chiude una duplicazione che esisteva**: `ARTHUD::DrawHUD` mostrava `ROOT` e poi `SLOW` in un
	 * `if`/`else if` — lo **stesso** ordine di `ControlStatusesBySeverity`, ricopiato a mano. Se quella lista
	 * cambiasse, o nascesse un terzo controllo, l'HUD sarebbe rimasto fermo e nessun test lo avrebbe detto:
	 * `Reaction.ControlStatusesAreTwo` sorveglia la lista, non chi la copia.
	 *
	 * ⛔ **Non inventa una gravita' per gli altri otto stati.** `ControlStatusesBySeverity` ne copre **due** e
	 * dichiara il proprio limite (*«e' una lista nel codice, non un dato del catalogo»*); una scala completa e'
	 * `E36`, **v0.2**. Inventarla qui creerebbe una seconda tassonomia che quell'epic dovrebbe riconciliare.
	 *
	 * ⚠️ **Non tronca.** Quante icone stiano sopra un cilindro e' una domanda di layout, e la risposta
	 * appartiene a chi disegna: la lista esce **ordinata** proprio perche' troncarla in coda sia una
	 * decisione del consumatore, cambiabile senza toccare il giudizio.
	 *
	 * ⚠️ **Non c'e' nessun filtro su `Status.Electrified`, e non deve essercene**: la funzione mostra cio' che
	 * l'unita' porta, e l'unita' non lo porta. L'inerzia (`#1324`) e' garantita **a monte** — nessun produttore
	 * lo applica con una durata, la scarica emette `AppliedInstantly` e non tocca `StatusTurns` — ed e'
	 * coperta da `RTElectricPropagationTests`, non da qui. 🔎 La prima stesura di questa riga prometteva un
	 * filtro, e il test scritto per pinnarlo e' diventato **rosso**: `ApplyStatus` con una durata positiva
	 * applica qualunque tag, quindi «Electrified non compare» e' una proprieta' del produttore, non di questa
	 * funzione.
	 *
	 * @return vuoto se `Unit` e' nullo o non ha stati attivi.
	 */
	static TArray<FRTStatusBadgeView> BuildStatusBadges(const ARTUnit* Unit);

	/**
	 * Tutto cio' che la sovrapposizione sopra un'unita' mostra, in **una** vista (`#2288`, `D-320`).
	 *
	 * 🔑 **Non calcola niente di nuovo: unisce due produttori che esistono gia'** — `BuildUnitCard` per vita,
	 * e scudo, `BuildStatusBadges` per gli stati — e aggiunge le sole due cose che nessuno dei due
	 * possiede: il **nome** da mostrare e il **colore di squadra**, che dipendono da chi guarda.
	 *
	 * ⚠️ **`PlayerTeamId` non e' un parametro decorativo**: decide `bIsAlly`, quindi il colore. La stessa
	 * unita' vista da due osservatori e' due viste diverse — ed e' la ragione per cui il driver di questa
	 * vista sta sull'HUD, che un osservatore ce l'ha, e non su un attore condiviso.
	 *
	 * ⛔ **Non decide se mostrarla.** Quello lo dice `ARTUnit::IsKnownToObserver()`, scritto dal velo
	 * (`#2246`): un secondo giudizio qui sarebbe la divergenza che quella issue ha appena tolto.
	 */
	static FRTUnitOverlayView BuildUnitOverlay(const ARTUnit* Unit, int32 PlayerTeamId,
		const TSet<FRTCellId>& PlannedHitCells, const TSet<FRTCellId>& PlannedAllyHitCells);

	/**
	 * I piani **autorevoli** di tutte le unita' vive, non filtrati per nessun osservatore.
	 *
	 * 🔴 **Il valore di ritorno non si mostra a nessuno cosi' com'e'**: passa da
	 * `URTIntentPrivacyLibrary::FilterForTeam`, che e' il punto in cui l'invariante #6 diventa vera. Il
	 * nome dice `Authoritative` per questo — in rete (M10) e' lo stato lato server.
	 *
	 * ⚠️ **Estratta da `ARTHUD::DrawHUD` il 2026-08-24 (CP 11.4, `#80`), e la ragione vale piu' del
	 * refactoring**: `rt.Debug.DrawIntent` deve mostrare *gli stessi* intenti che la HUD disegna. Con due
	 * costruzioni separate, il giorno in cui un campo si aggiunge a `FRTPlannedIntent` — com'e' successo a
	 * `bDeclaresRotation` con #291 — una delle due lo dimentica, e lo strumento di debug mente proprio
	 * sulla cosa che si sta debuggando. Una sola sede, due chiamanti.
	 */
	static TArray<FRTPlannedIntent> BuildAuthoritativeIntents(const TArray<AActor*>& Actors);
};
