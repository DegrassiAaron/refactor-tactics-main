#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Ability/RTActionDef.h" // ERTActionSlot: la vista dei cooldown raggruppa per slot
#include "RTHudViewModel.generated.h"

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
};

/**
 * Una unita' come la vede il pannello: salute, scudo, energia, identita'.
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
	int32 Energy = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 MaxEnergy = 0;

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
 * ⚠️ **Non sono tre booleani indipendenti.** `Action.Sprint` dichiara `MovementAndMain` e ne occupa **due**,
 * ed e' il caso che rende sbagliata la mappatura ovvia «un'azione, uno slot». Chi decide resta
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
	 * Usabile **adesso**, che non e' `TurnsRemaining == 0`: serve anche l'energia.
	 *
	 * I due dati restano separati perche' rispondono a domande diverse e il giocatore le pone entrambe —
	 * «quanto manca?» e «posso adesso?». Un widget che mostrasse solo il primo direbbe «pronta» di
	 * un'ultimate senza energia.
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
};
