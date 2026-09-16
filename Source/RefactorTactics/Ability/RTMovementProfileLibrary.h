#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTMovementProfile.h"
#include "Turn/RTPlanValidationLibrary.h"
#include "RTMovementProfileLibrary.generated.h"

/**
 * Il catalogo dei profili di movimento, e la proiezione che ricava dal PIANO quale profilo l'unita' abbia
 * scelto (`#653`, prerequisito di [#641] e [#606]).
 */
UCLASS()
class REFACTORTACTICS_API URTMovementProfileLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** `MovementProfile.Still` — nessun movimento pianificato. */
	static const FName ProfileStill;
	/** `MovementProfile.Move` — il profilo neutro, che eredita il budget dall'unita'. */
	static const FName ProfileMove;
	/** `MovementProfile.Sprint` — 8 punti, [D-116]. */
	static const FName ProfileSprint;
	/** `MovementProfile.Withdraw` — 2 punti, il ripiegamento che [D-070] vincola all'Overwatch. */
	static const FName ProfileWithdraw;
	/** `MovementProfile.Sneak` — dichiarato SENZA numeri (`AE-5`), quindi non pianificabile. */
	static const FName ProfileSneak;

	/**
	 * I profili spediti. L'ordine e' stabile perche' da qui puo' nascere una UI, e un elenco che cambia
	 * ordine e' un elenco che si legge diverso a ogni apertura.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static TArray<FRTMovementProfile> GetCoreMovementProfileCatalog();

	/** Il profilo con quell'`Id`, o un profilo invalido (`IsValid() == false`) se non esiste. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile FindProfile(FName ProfileId);

	/**
	 * Quale profilo il PIANO dichiara: quello nominato dall'azione di movimento pianificata, oppure
	 * `Still` se l'unita' non ne ha pianificata nessuna.
	 *
	 * 🔑 **Il profilo si RICAVA dal piano e non e' un campo accanto ad esso**, ed e' la scelta strutturale
	 * di questo checkpoint. Un secondo campo si potrebbe compilare `Move` mentre l'azione dice `Sprint`:
	 * sarebbero due autorita' sulla stessa domanda, e il resolver dovrebbe sceglierne una. Derivandolo,
	 * la coerenza non e' una regola da far rispettare — e' una proprieta' del tipo.
	 *
	 * ⚠️ **Un piano con piu' di un'azione di movimento restituisce la PRIMA in ordine di piano**, non un
	 * errore: che due azioni possano occupare insieme lo slot `Movement` e' una domanda di
	 * `ValidatePlan`, che la rifiuta gia' per conto suo ([D-028]). Qui non si duplica quel verdetto.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile ProfileForPlan(const TArray<FRTPlannedAction>& Plan);

	/**
	 * L'azione del catalogo core che NOMINA quel profilo, o un `Def` vuoto (`ActionId.IsNone()`) se
	 * nessuna lo fa (`#1410`).
	 *
	 * E' l'inversa di `FRTActionDef::MovementProfileId`, e serve al selettore: il giocatore sceglie un
	 * **profilo**, il piano porta un'**azione**. Senza questa funzione il selettore dovrebbe conoscere la
	 * corrispondenza per nome, che e' la seconda sede che `#653` ha evitato.
	 *
	 * ⚠️ **Il catalogo si scorre una volta sola.** `GetCoreActionCatalog()` costruisce l'array a ogni
	 * invocazione, e questa funzione la chiama il compositore del piano — cioe' la HUD a ogni click. La
	 * cache e' la stessa disciplina gia' applicata a `MakePlanFor` e a `GetReactionProfileCatalog`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTActionDef FindActionForProfile(FName ProfileId);

	/**
	 * Se il giocatore puo' **dichiarare** questo profilo ([D-425] punto (1b)).
	 *
	 * Oggi ne esiste uno solo, `Sneak`, e le esclusioni degli altri quattro hanno **quattro ragioni
	 * diverse** che chi le comunica non deve confondere (`#1410` `AC-4`):
	 *
	 * - `Still` e' **derivato**: lo produce l'assenza di piano. Si esce dal fermo cancellando i waypoint,
	 *   e offrirlo darebbe due gesti per lo stesso fatto;
	 * - `Move` e `Sprint` sono **letti**: li produce la distanza pianificata. Dichiararli chiederebbe al
	 *   giocatore di ripetere cio' che il suo percorso gia' dice, ed e' il selettore che [D-425] smonta;
	 * - `Withdraw` e' **riservato**: lo impone l'`Overwatch` ([D-070]), e sceglierlo a mano sarebbe una
	 *   seconda verita' sullo stesso vincolo.
	 *
	 * 🔑 **E' una regola, non un dato di catalogo, ed e' per questo che sta qui e non in
	 * `FRTMovementProfile`.** Quali profili si dichiarino dipende da come [D-425] distribuisce tetti e
	 * bande, non da cosa il profilo *e'*: il giorno in cui la distribuzione cambiasse, a cambiare sarebbe
	 * questa funzione e non cinque voci di catalogo da tenere d'accordo.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static bool IsDeclarableProfile(FName ProfileId);

	/**
	 * Il **TETTO**: fin dove si puo' pianificare, espresso come il profilo che lo porta ([D-425] punti
	 * (1) e (9)).
	 *
	 * 🔑 **Cio' che si DICHIARA e' un tetto; cio' che si DERIVA e' una banda** ([D-425] punto (2)). Questa
	 * funzione risponde alla prima meta': quanto lontano il giocatore ha il permesso di arrivare. La
	 * seconda meta' — che profilo ne risulti — e' `ProfileForPlannedSteps`.
	 *
	 * In ordine di precedenza:
	 *
	 * - `ReservedProfileId` non vuoto → quel profilo, **imposto**. E' l'`Overwatch` che riserva il
	 *   `Withdraw` a ¼ ([D-070]), e la riserva vince su qualunque dichiarazione: il ripiegamento non e'
	 *   una scelta;
	 * - `DeclaredProfileId` non vuoto → quel profilo, **dichiarato**. Oggi e' il solo `Sneak` a ½, e
	 *   dichiararlo significa **rinunciare a meta' distanza** in cambio del silenzio ([D-425] punto (8));
	 * - altrimenti il tetto **nudo**, che e' `Sprint` — **2×** ([D-425] punto (9)). ⛔ Non e' `Move`: senza
	 *   dichiarare niente si pianifica fino al doppio, e superare `1×` fa scattare da se' i prezzi che lo
	 *   `Sprint` gia' dichiara. E' il punto in cui il giocatore sceglie *fin dove arrivare* invece di
	 *   dichiarare *come*.
	 *
	 * ⚠️ **`bRunDenied` abbassa il tetto nudo a `Move`, e non e' un caso speciale di questa funzione**:
	 * e' [D-319] — *«chi ha perso l'equilibrio non corre»* — espressa dove le altre riduzioni gia' vivono.
	 * Prima di [D-425] era un rifiuto della dichiarazione, perche' correre si dichiarava; ora che si deriva
	 * dalla distanza, negare la corsa significa **negare la distanza**, ed e' un tetto. ⛔ Non tocca un
	 * tetto DICHIARATO piu' basso: chi sguscia da sbilanciato sguscia, perche' `Sneak` non e' una corsa.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile CeilingProfile(FName DeclaredProfileId, FName ReservedProfileId,
		bool bRunDenied = false);

	/**
	 * La regola di [D-425] per intero: quale profilo produce una distanza **gia' pianificata**.
	 *
	 * ```
	 * (a) slot riservato     -> il profilo imposto      (Overwatch -> Withdraw, tetto ¼)
	 * (b) profilo dichiarato -> il profilo dichiarato   (Sneak, tetto ½)
	 * (c) nessun passo       -> Still
	 * (d) altrimenti         -> la BANDA: fino a 1x Move, fino a 2x Sprint
	 * ```
	 *
	 * 🔑 **Il denominatore e' SEMPRE `UnitMoveRange`, il movimento base dell'unita'** ([D-425] punto (1)),
	 * anche quando una skill di mobilita' concede di piu': la skill decide *quanto puoi spendere*, le bande
	 * restano ancorate all'eroe. Un secondo denominatore renderebbe `Sprint` una funzione dell'equipaggia-
	 * mento invece che dell'andatura.
	 *
	 * ⚠️ **I passi si CLAMPANO al tetto prima di leggere la banda**, e questa riga e' cio' che tiene
	 * insieme le due meta'. Senza, un piano scritto quando il tetto era piu' alto — o un'unita' diventata
	 * `Unbalanced` dopo averlo scritto — produrrebbe la banda `Sprint` e con essa i prezzi dello Sprint,
	 * mentre `TruncatePathToBudget` gli accorcia il percorso a `1x`: pagherebbe una corsa che non fa.
	 *
	 * ⛔ **Una distanza oltre il tetto piu' alto non ripiega su `Move`**: si legge `Sprint`, la banda piu'
	 * larga. Non dovrebbe accadere — il troncamento lo previene — ma se accadesse, dire «cammino» di un
	 * percorso che nessun tetto copre nasconderebbe il difetto invece di mostrarlo.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile ProfileForPlannedSteps(int32 PlannedSteps, int32 UnitMoveRange,
		FName DeclaredProfileId, FName ReservedProfileId, bool bRunDenied = false);

	/**
	 * Il profilo a cui il PIANO riserva lo slot movimento, o `NAME_None` se nessuna voce lo fa
	 * (`#1410` `AC-5`, [D-070]).
	 *
	 * Legge `FRTActionDef::ReservesMovementProfileId`, quindi vale per qualunque azione lo dichiari e non
	 * per il solo `Action.Overwatch`: chi la consuma non ha bisogno di sapere **quale** azione ha imposto
	 * il vincolo, solo che c'e' e a cosa costringe.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FName ReservedProfileForPlan(const TArray<FRTPlannedAction>& Plan);
};
