#pragma once

#include "CoreMinimal.h"
#include "Ability/RTActionDef.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/Actions/RTActionFallbackLibrary.h"
#include "Turn/Resolution/RTHexSim.h"
#include "RTPlanValidationLibrary.generated.h"

class ARTUnit;

/**
 * Una voce del piano di un'unita': l'azione dichiarata, piu' lo stato che la rende ripetibile o no.
 *
 * Il cooldown residuo e' un PARAMETRO e non un campo dello snapshot perche' oggi vive altrove:
 * `ARTUnit::AbilityCooldowns` e' privato e parallelo ad `Abilities`, sull'Actor. Passarlo qui e' lo stesso
 * pattern gia' in uso in `URTCombatLibrary::IsAbilityUsable(CooldownRemaining)`, e
 * mantiene la validazione una funzione pura: stessa coppia (snapshot, piano) => stesso verdetto.
 */
USTRUCT(BlueprintType, meta = (RTServerOnly))
struct FRTPlannedAction
{
	GENERATED_BODY()

	/**
	 * L'azione dichiarata. Di lei la validazione legge `ActionId` e `Slot`.
	 *
	 * 🔴 `CostMP` **non si legge qui, ed e' una decisione**: [D-190] lo assegna a [D-117], dove diventa
	 * il contributo dell'azione al modificatore del costo PER CELLA — non un costo da sommare contro un
	 * budget. Le due letture non convivono: un modificatore firmato, che D-117 rende negativo apposta,
	 * manderebbe sotto zero una somma poi confrontata con `MoveBudget`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	FRTActionDef Def;

	/** Turni di cooldown ancora da scontare; 0 = ripetibile ora. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 CooldownRemaining = 0;

	/**
	 * L'ordine di dichiarazione della voce (E38 CP 38.12, [D-615] (5)): la posizione della sua voce nella lista di
	 * `FRTUnitPlan`, scritta da `MakePlanFor`. `INDEX_NONE` = non dichiarato (le voci sintetiche dei test).
	 *
	 * 🔑 **Si mostra e non decide.** Legalita' e motivo non lo leggono. Lo legge solo la RESA del verdetto: quale
	 * occorrenza si nomina, e quale voce si accusa di `OverBudget` (la prima che, in quest'ordine, fa sforare la somma).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 DeclarationIndex = INDEX_NONE;

	/**
	 * Il cooldown della RIGA DI CATALOGO dell'azione, prima della variante d'arma ([D-615] (6); E38 CP 38.13): e' cio'
	 * che legge la regola della copia ([D-614]). `MakePlanFor` lo prende da `URTActionData::CooldownTurnsBeforeVariant`
	 * quando una variante e' applicata, e da `Def.CooldownTurns` altrimenti; per un'azione concessa da un gadget e' la
	 * riga del gadget. 0 = ripetibile finche' il budget lo consente (anche nelle voci sintetiche dei test).
	 *
	 * ⛔ **Non e' `Def.CooldownTurns`**, che dopo `Weapon.Overcharge` porta la riga piu' i turni della variante.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 CatalogCooldownTurns = 0;
};

/** Verdetto della validazione: legale, oppure illegale con il motivo e l'azione che lo ha causato. */
USTRUCT(BlueprintType)
struct FRTPlanValidation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	bool bLegal = true;

	/** `None` quando il piano e' legale. Estende la famiglia gia' serializzata del fallback. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionInvalidReason Reason = ERTActionInvalidReason::None;

	/** Quale voce del piano ha causato il rifiuto; `NAME_None` se il piano e' legale. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FName OffendingActionId;

	/**
	 * Quale voce del piano teneva GIA' lo slot quando il rifiuto e' scattato; `NAME_None` se il piano e'
	 * legale o se il motivo non e' un conflitto di slot (`OnCooldown` non ha un detentore).
	 *
	 * 🔴 **Esiste perche' `OffendingActionId` da solo accusa l'azione sbagliata.** L'ordine canonico di
	 * `ValidatePlan` — per larghezza di slot, poi per `ActionId` — incontra `Action.Move` prima di
	 * `Hero.Riktor.Ram`, quindi nomina Ram; ma in risoluzione e' lo scatto a vincere ed e' Move a essere
	 * scartato. Chi comunica il rifiuto nomina ENTRAMBE e non deve indovinare il perdente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FName HolderActionId;

	/**
	 * L'occorrenza di `OffendingActionId` e di `HolderActionId`: il loro `FRTPlannedAction::DeclarationIndex`
	 * (E38 CP 38.12). `INDEX_NONE` quando la voce manca, o non e' dichiarata. Servono a nominare quale delle due copie
	 * di una stessa azione ha causato il rifiuto: sono resa, come i due nomi, e non entrano nel motivo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 OffendingDeclarationIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 HolderDeclarationIndex = INDEX_NONE;
};

/**
 * Il punto unico che risponde LEGALE / ILLEGALE + motivo PRIMA del commit del piano (CP 38.2, epic E38).
 *
 * Cio' che esisteva prima — `URTActionFallbackLibrary::ValidateInstance` — agisce IN RISOLUZIONE: un piano
 * illegale si scopriva quando non funzionava, cadendo su un fallback. Qui si scopre quando lo si compone.
 *
 * Regola in vigore: D-028, confermata da D-114 il 2026-08-12. L'economia e' a **slot** e la legalita' e'
 * STRUTTURALE — due principali non si sommano perche' occupano la stessa cosa, non perche' un totale sfori.
 */
UCLASS()
class REFACTORTACTICS_API URTPlanValidationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Valida il piano di UNA unita'.
	 *
	 * Da `Unit` il corpo legge solo `ActionPointBudget`, il budget dell'unita' per il turno ([D-613] (1); E38 CP
	 * 38.11): chi la costruisce passa da `ARTTurnManager::MakeSimUnit`, lo stesso costruttore di snapshot, lock-in e
	 * HUD, cosi' i tre danno lo stesso verdetto. Un `FRTHexSimUnit()` costruito a mano giudica col default 6.
	 *
	 * Precedenza dei motivi, deterministica e indipendente dall'ordine in cui il giocatore compone:
	 *   1. `OnCooldown` — proprieta' INTRINSECA di una voce sola;
	 *   2. `RepeatedWithCooldown` — la regola della copia ([D-614]; E38 CP 38.13): due voci con lo stesso `ActionId`,
	 *      in qualunque fase ([D-615] (1)), quando la riga di catalogo ha un cooldown (`CatalogCooldownTurns`, [D-615]
	 *      (6)). Precede il budget perche' la copia vietata non deve pesare nella somma, e l'avviso dice la causa vera;
	 *      nomina la prima voce, nell'ordine di dichiarazione, che ripete l'azione di una dichiarata prima. Non ha un
	 *      detentore;
	 *   3. `OverBudget` — proprieta' QUANTITATIVA del piano ([D-594]: la somma dei costi AP supera il budget dell'unita');
	 *      nomina la prima voce, nell'ORDINE DI DICHIARAZIONE, che fa sforare la somma (E38 CP 38.12): le voci dichiarate
	 *      prima di lei stanno tutte nel budget. E' resa, non regola: la legalita' e' la somma intera;
	 *   4. `SecondDash` — un secondo scatto, diverso ([D-625] (4); E38 CP 38.16): la sola regola di classe che resta oltre al
	 *      budget; accusa il secondo nell'ordine di dichiarazione, col primo come detentore. ⏱️ *Fino a CP 38.16 il passo 4 era
	 *      `SlotOccupied`, la proprieta' strutturale della combinazione*: gli slot non decidono piu' ([D-584] (1)), e quel motivo
	 *      non ha produttori.
	 *
	 * ⏳ **Il budget precede lo slot ed e' voluto** (E38 CP 38.6, #4012): ADR-0011 sostituira' gli slot con il
	 * solo budget, quindi il motivo che sopravvivera' va letto per primo. Finche' il resolver accetta una azione
	 * per fase, i due vincoli convivono: `Plan.LegalTodayStaysLegal` fissa che nessuna delle 23 forme legali con
	 * gli slot sfora i 6 punti di D-594, e `Plan.SumOverBudgetIsRejected` che un piano da 8 punti e' rifiutato
	 * prima che lo slot abbia voce.
	 *
	 * 🔴 **Erano tre fino a [D-190]**, e in mezzo stava `InsufficientMovementPoints`: la somma dei `CostMP`
	 * del piano contro `MoveBudget`. E' uscito perche' era una SOTTRAZIONE, cioe' l'ultimo residuo del modello
	 * ad Action Point che [D-114] ha respinto — e perche' il limite di movimento e' gia' applicato dove serve,
	 * nel pathfinding in pianificazione (`ARTPlayerController::HandleClickOnCell`), che rifiuta il waypoint e
	 * sa dire anche quanto era gia' speso. Il valore resta in coda a `ERTActionInvalidReason` e nessuno lo
	 * produce: rinumerare l'enum cambierebbe il significato delle tracce gia' scritte.
	 *
	 * L'ordine dell'array non entra nel verdetto: le voci si esaminano in un ordine canonico (prima chi
	 * occupa piu' slot, poi per `ActionId`, poi per `DeclarationIndex`, che lo rende totale anche su due copie),
	 * altrimenti lo stesso piano composto in due sequenze diverse darebbe due motivi diversi — cioe' la
	 * presentazione deciderebbe la regola. Ne' l'ordine dell'array ne' quello di dichiarazione cambiano legalita'
	 * e motivo ([D-615] (5)); l'ordine di dichiarazione decide solo quale occorrenza si nomina.
	 *
	 * ⚠️ **Non c'e' piu' una precondizione su `Unit`**, e la riga che la dichiarava e' stata tolta: diceva
	 * *«PRECONDIZIONE: `Unit` e' viva»*, ma dopo [D-190] il corpo non legge quel parametro, quindi la
	 * condizione non e' violabile e non puo' cambiare il verdetto. Lasciarla avrebbe contraddetto il
	 * capoverso qui sopra e invitato il prossimo chiamante a filtrare i morti per un requisito che non
	 * esiste. La ragione per cui la liveness non si controlla resta valida e vale la pena tenerla: un'unita'
	 * morta non compone piani, e aggiungere un motivo per un caso che nessun chiamante puo' produrre
	 * significherebbe un valore in piu' in un enum serializzato, mai scritto in nessuna traccia.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static FRTPlanValidation ValidatePlan(const FRTHexSimUnit& Unit, const TArray<FRTPlannedAction>& Plan);

	/**
	 * Il budget di Action Point di partenza ([D-594] (1): 6). Da E38 CP 38.11 ([D-613] (1)) il budget e' un numero
	 * dell'unita' per il turno, `FRTHexSimUnit::ActionPointBudget`, e `ValidatePlan` legge quello: questo e' il suo
	 * default, che `ARTUnit` e `FRTHexSimUnit` prendono all'allestimento.
	 */
	static constexpr int32 DefaultActionPointBudget = RTActionPoints::DefaultBudget;

	/** La somma dei costi AP dichiarati dalle voci del piano (`FRTActionDef::ActionPointCost`, mai negativi). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static int32 ActionPointsSpent(const TArray<FRTPlannedAction>& Plan);

	/** Quanti slot occupa un'azione: 0 per `None`, 2 per `MovementAndMain`, 1 per le altre. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static int32 SlotWidth(ERTActionSlot Slot);

	/**
	 * Vero se un piano con questo motivo si SVUOTA al lock-in e il Ready si rifiuta ([D-615] (1)): ricarica, copia
	 * vietata ([D-614], E38 CP 38.13) e budget. `SlotOccupied` resta registrato e non bloccato fino a CP 38.16. E' il
	 * punto unico della domanda: la leggono il lock-in e il Ready, che devono dare la stessa risposta.
	 */
	static bool EmptiesPlanAtLockIn(ERTActionInvalidReason Reason);

	/**
	 * Compone il piano di un'unita' leggendo i campi `Planned*` che giocatore e bot scrivono sull'Actor.
	 *
	 * **E' la cerniera che mancava a CP 38.2.** `ValidatePlan` era puro sul piano, ma nessuno sapeva
	 * COSTRUIRE quel piano da cio' che il gioco scrive davvero: senza questa funzione il validatore resta
	 * una regola che solo i test possono invocare — e un validatore che nessuno chiama non e' una regola.
	 *
	 * Lo slot di ogni voce lo dichiara il **catalogo** (`FRTActionDef::Slot`), non il campo in cui l'azione
	 * e' scritta. ⏱️ *Fino a E38 CP 38.16 `ARTTurnManager::ResolveDash` spendeva la principale se l'azione dichiarava
	 * `MovementAndMain`*; da [D-628] (1) un kit dice *«questa mobilita' costa tutto il turno»* col costo in AP, che questa
	 * funzione porta nella voce senza sapere di lei.
	 *
	 * ⚠️ Il movimento NORMALE non e' un'abilita' del kit e non ha un `URTActionData`: e' una destinazione
	 * (`PlannedCell`) o un percorso (`PlannedPath`). La sua voce viene da `Action.Move` nel catalogo core,
	 * e se il catalogo non lo conoscesse la voce **non si aggiunge**: inventarne una col `Slot` di riposo
	 * (`Main`) produrrebbe un `SlotOccupied` fantasma contro un'azione che esiste davvero.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static TArray<FRTPlannedAction> MakePlanFor(const ARTUnit* Unit);
};
