#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h" // FRTActionEffectSpec: gli effetti di una reazione d'eroe sono un parametro
// `ERTAbilityShape`: serve il tipo COMPLETO, non una forward declaration. Uno scoped enum con underlying
// type fisso si puo' dichiarare in avanti, ma il default `= ERTAbilityShape::Single` nomina un
// **enumeratore**, e per quello il tipo dev'essere definito — provato, `error C2027`.
//
// ✅ E il costo e' due file, non cinquantacinque: dei 55 che aprono questo header, **53** includono gia'
// `RTActionData.h` o `RTHeroData.h`. Misurato, non stimato.
#include "Ability/RTActionData.h"
#include "RTHeroCatalogLibrary.generated.h"

class URTHeroData;
class URTActionData;

/**
 * Lettura e validazione del catalogo eroi: pura, deterministica, senza Actor.
 *
 * Stessa disciplina di `URTCatalogLibrary` per le azioni: il validator e' una funzione pura per costruzione,
 * cosi' un roster incompleto (debolezza mancante, struttura sbagliata) si scopre in CI, non in partita.
 */
UCLASS()
class REFACTORTACTICS_API URTHeroCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Errori strutturali di un roster di eroi (vuoto = roster valido). Rifiuta:
	 * HeroId assente o duplicato · statistiche non positive (salute, movimento) o negative (vista, resistenza
	 * push) · numero di azioni **fuori dall'intervallo 5-6** (attacco base + quattro fondamentali, catalogo
	 * v0.1 §"Struttura di un eroe", piu' al massimo UNA generica del catalogo core portata nel kit) ·
	 * affinita' o debolezza non dichiarate · zero o piu' di UNA azione fondamentale con varianti dichiarate
	 * (vincolo v0.1: una sola abilita' configurabile per eroe).
	 *
	 * ⚠️ **L'intervallo era `== 5` esatte, e il corpo della funzione ne dichiara il costo**: il validator non
	 * dice piu' «questo eroe e' completo» ma «e' nell'intervallo». Il tetto e' 6 e non «quante ne vuoi»
	 * perche' oltre il kit supera le posizioni che l'input raggiunge — `PlayerInput.EveryKitEntryIsReachable`
	 * e' il gate che lo misura. Sul roster v0.1 ne hanno sei **Muiren** (`TideGuard`) e **Ivrin**
	 * (`PhaseGuard`); Aevik e Branth restano a cinque.
	 *
	 * Ogni messaggio nomina l'eroe colpevole: un errore che non dice QUALE eroe e' rotto costringe a
	 * ricontrollare tutto il roster a mano.
	 *
	 * Non e' `UFUNCTION`: UHT non riflette `TArray<const T*>` (stesso limite dichiarato da
	 * `URTCatalogLibrary::ValidateEquipment`), quindi resta una funzione C++ pura, non chiamabile da Blueprint.
	 */
	static TArray<FString> ValidateHeroes(const TArray<const URTHeroData*>& Heroes);

	/**
	 * Costruisce **Aevik**, tecnico della conduzione (catalogo eroi v0.1 §1): 90 HP, 5 MP, **vista 7** (era 6,
	 * alzata da D-073 / #131: l'unico del roster che vede oltre il raggio 6), resistenza
	 * push 0, affinita' elettricita', debolezza acqua (combo dichiarata con Muiren, CP 6.3). Nuova istanza a
	 * ogni chiamata — stesso idioma di `URTCatalogLibrary::GetCoreActionCatalog`: il catalogo eroi non e' un
	 * singleton, e un chiamante che ne vuole due (es. specchio nello stesso 2v2) non condivide lo stato.
	 *
	 * `ReactiveCapacitor` e' **cablata** (CP 6.7): reazione sulla semantica di `Action.Counter` con due
	 * effetti propri — scudo 15 a se' e 10 danni all'attaccante.
	 *
	 * `ConductiveNode` **e'** `Action.Electrify`: cablata da D-046 (#282) e confermata definitiva da D-064
	 * (#207), che dichiara obsoleto il PDF in cui «rende conduttiva una cella per 2 turni». L'azione LEGGE il
	 * grafo conduttivo — acqua e superfici `Conductive` — e non lo crea. Portata e propagazione vengono dal
	 * core, quindi `Range 0` non e' piu' un segnaposto in attesa di un numero.
	 *
	 * 🔑 **La combo `Wet` e' una sinergia SISTEMICA, e `LinearDischarge` resta un'abilita' elettrica.** Il `+8`
	 * dipende da `Status.Wet` **sul bersaglio**, non da chi ha bagnato: acqua e superfici conduttive sono il
	 * *setup*, la scarica e' il *payoff*. L'ownership e' decisa da D-029 e ADR-0006 — qualunque sorgente di
	 * `Wet` autorizzata dalle regole (`Gadget.Sprinkler`, acqua bassa del terreno) abilita lo stesso payoff, e
	 * Muiren non e' un requisito. Il bonus non vive negli `Effects`: passa da `EffectiveAttackPower` +
	 * `URTCombatLibrary::AevikWetDischargeBonus`, ed e' misurato da `Heroes.Aevik.WetBonus`.
	 *
	 * Limite dichiarato che resta: `Overload` non ha un modello di "dispositivo interrompibile", quindi
	 * l'`Interrupt` sui dispositivi non e' rappresentabile. L'azione esiste comunque come DATO, con la sua
	 * identita', portata e cooldown dal catalogo — solo l'effetto aggiuntivo resta un numero non ancora
	 * consumato.
	 *
	 * ⚠️ **Il limite e' vero, la ragione che lo motivava no.** Fin qui questa riga diceva «(E7, gadget)», cioe'
	 * *i gadget non esistono ancora*. **E7 e' chiusa** e l'equipaggiamento esiste — `Gadget.Sprinkler` e
	 * `Gadget.Insulator` sono nel catalogo. Cio' che manca e' piu' stretto: un **bersaglio interrompibile**,
	 * un dispositivo con uno stato che un `Interrupt` possa spegnere. Un limite che cita un'epic chiusa si
	 * legge come gia' risolto, ed e' il modo in cui un vincolo reale sparisce senza che nessuno lo tolga.
	 */
	static URTHeroData* MakeAevik();

	/**
	 * Costruisce **Muiren**, manipolatrice dell'acqua (catalogo eroi v0.1 §2): 95 HP, 5 MP, vista 5, resistenza
	 * push 0, affinita' acqua, debolezza elettricita' — simmetrica a Aevik (stesso `Affinity.Electricity`),
	 * cosi' la rivalita' fra i due e' un solo identificatore condiviso, non due nomi.
	 *
	 * ⚠️ **Muiren porta SEI azioni**: le cinque del catalogo piu' `TideGuard`, lo scudo proattivo derivato da
	 * `Action.Shield`. Non e' una fondamentale — e' la generica core che l'intervallo 5-6 di `ValidateHeroes`
	 * ammette — e il suo gemello e' `Hero.Ivrin.PhaseGuard`, uno per squadra.
	 *
	 * Limiti dichiarati: `CircularTide` **cura e basta** (`{ Heal 18 }`). Il `Wet` ad area e' uscito dalla
	 * dichiarazione con #1006, che allinea Muiren al grado `Access` di #995 — una sola capability elementale,
	 * e resta `PressureJet`. Il limite che *resta* e' un altro: **nessun resolver applica oggi effetti diversi
	 * ad alleati e nemici dentro la stessa area** (`bFriendlyFire` decide solo SE un alleato viene colpito,
	 * non CON QUALE effetto). `FluidTrail` **non crea piu' acqua**: sempre #1006 la riporta a uno scatto puro,
	 * e l'owner della superficie e' l'equipaggiamento (`Gadget.Sprinkler`). `FlowReaction` e' **rinviata a
	 * E14** (ADR-0004): produce movimento dentro un boundary di risoluzione, che il motore delle reazioni di
	 * E5 non fa — il rinvio e' dichiarato come dato (slot `None`, nessun trigger), non lasciato a un commento.
	 *
	 * 🔵 **`MistVeil` non e' piu' un limite, ed e' l'unica delle tre a essere uscita dall'elenco per merito.**
	 * Fin qui questa docstring la teneva accanto a `FluidTrail` dicendo che «non hanno un modello di terreno
	 * dinamico (E8/E9)». **E8 ed E9 sono chiuse**, e #353 ha collegato l'azione al meccanismo che gia'
	 * esisteva: oggi dichiara `bCreatesSurface` e crea `ERTHexSurface::Smoke` raggio 1, nel Cleanup. Le altre
	 * due sono cambiate per ragioni diverse — una per decisione di design, l'altra resta rinviata — e
	 * tenerle sotto la stessa frase le faceva sembrare bloccate dalla stessa cosa.
	 */
	static URTHeroData* MakeMuiren();

	/**
	 * Costruisce **Branth**, architetto del campo (catalogo eroi v0.1 §3): 120 HP, 4 MP, vista 5,
	 * **resistenza push 0**, affinita' strutture, debolezza movimento — simmetrica a
	 * Ivrin (CP 6.5), come Aevik/Muiren lo sono fra loro.
	 *
	 * ⚠️ **Era `1`, l'unico del roster, e questa riga lo ha dichiarato per piu' di quanto sia stato vero.**
	 * D-075 (#402) l'ha azzerata il 2026-08-10: siccome ogni spinta del gioco vale 1 e `PushResistance` e'
	 * una SOGLIA (D-038), il valore `1` non comprava stabilita' ma **immunita' totale a ogni spostamento**,
	 * che nessuno aveva deciso. Cio' che Branth compra col movimento e la vista sono i 120 HP, e quelli non
	 * dipendevano da questo campo. Il roster e' ora interamente a `0`: la meccanica e' **dormiente**, non
	 * dimenticata — vedi il commento accanto al literal nel `.cpp`.
	 *
	 * 🔵 **`KineticPanel` e `Reconfigure` hanno un sistema che le consuma, da CP 9.5.** Fin qui questa
	 * docstring diceva che le strutture «non esistono nel modello dati (E9, `#69`/`#73`)»: **E9 e' chiusa**, e
	 * `ResolveCoverStructures` legge integrita' e durata dalla variante attiva dell'unita'
	 * (`ARTUnit::ActiveVariantId`) invece che dai valori base. I `Parameters` erano stati scritti dichiarando
	 * che sarebbero serviti a E9 — e' successo. `Interposition` e' **cablata** (CP 6.7) sulla semantica di
	 * `Action.Intercept`.
	 */
	static URTHeroData* MakeBranth();

	/**
	 * Costruisce **Ivrin**, duellante predittivo (catalogo eroi v0.1 §4): **90 HP** (era 100, abbassata da
	 * D-069 / #131 perche' «compra mobilita' con l'assenza di difese» sui numeri era falso), **6 MP** (il piu'
	 * mobile), vista 6, resistenza push 0, affinita' movimento, debolezza strutture — simmetrica a Branth, che
	 * chiude il roster in due coppie (Aevik↔Muiren, Branth↔Ivrin).
	 *
	 * ⚠️ **Anche Ivrin porta SEI azioni**: le cinque del catalogo piu' `PhaseGuard`, gemello di
	 * `Hero.Muiren.TideGuard` e derivato dallo stesso `Action.Shield`. Non e' una fondamentale, e' la generica
	 * core che l'intervallo 5-6 di `ValidateHeroes` ammette. ⛔ Il `Muiren` del nome e' la **fase** — DisplayName
	 * «Guardia di fase» — non l'eroe, e infatti il rename di questa identita' l'ha lasciato intatto: e'
	 * `Hero.Ivrin.PhaseGuard`, non `Hero.Ivrin.IvrinGuard` (#2491).
	 *
	 * `Deflection` e' cablata (CP 6.7, semantica di `Action.Deflect`). `InterceptShot` **non e' piu' una
	 * reazione**: dal 2026-08-10 (E18 CP 18.2, D-016) e' una **Predictive Action** — cella dichiarata in
	 * Planning, verificata al boundary del Move, nessun input durante la Resolution. Il rinvio a E14 e'
	 * caduto per la ragione opposta a quella che l'aveva prodotto: non le serve una finestra interattiva.
	 *
	 * Limiti dichiarati: `Feint` marca una CELLA e concede un `Reposition`, e nessuna delle due meta' e' un
	 * `ERTActionEffect` (gli stati si applicano alle unita', il movimento passa da `ERTMovementStyle`).
	 */
	static URTHeroData* MakeIvrin();

	/**
	 * Il roster completo della v0.1, nell'ordine del catalogo eroi: Aevik, Muiren, Branth, Ivrin.
	 * Nuove istanze a ogni chiamata (stesso idioma di `URTCatalogLibrary::GetCoreActionCatalog`).
	 */
	static TArray<URTHeroData*> GetHeroRoster();

	/**
	 * Solo gli `HeroId` del roster (`Hero.Aevik`, `Hero.Muiren`, `Hero.Branth`, `Hero.Ivrin`), senza costruire
	 * gli eroi.
	 *
	 * Esiste perche' `GetHeroRoster()` istanzia quattro `URTHeroData` **con tutte le loro abilita'** a ogni
	 * chiamata, e chi ha bisogno dei soli identificatori — il catalogo icone di CP 20.2, che deriva
	 * `UI.Icon.Identity.*` dal roster — pagherebbe quel prezzo per leggere quattro nomi.
	 *
	 * ⚠️ **E' una seconda copia dell'elenco, e per questo non e' silenziosa**: `Heroes.HeroIdsMatchRoster`
	 * confronta questa lista con gli `HeroId` di `GetHeroRoster()` e cade se divergono. Un eroe nuovo aggiunto
	 * al roster e dimenticato qui fa rosso il test, non un'icona mancante scoperta a schermo.
	 */
	static TArray<FName> GetHeroIds();

	/**
	 * Una reazione D'EROE costruita sopra la semantica di un'azione core (CP 5.5): dal core arrivano fase,
	 * priorita', slot, trigger, fallback e interrompibilita' — cioe' *come* la reazione si comporta nel turno;
	 * dall'eroe l'identita' (`HeroActionId`), il cooldown e gli effetti — cioe' *cosa* fa e quanto.
	 *
	 * Esiste perche' l'alternativa e' peggiore: senza, `Branth.Interposition` dovrebbe riscrivere la
	 * semantica di `Action.Intercept` (o il resolver riconoscerla per ActionId), e ogni eroe con una reazione
	 * aggiungerebbe un ramo al motore. Con questo helper la semantica si dichiara UNA volta nel catalogo core.
	 *
	 * `Effects` vuoto significa «gli stessi del core», non «nessun effetto»: e' il caso di una reazione d'eroe
	 * che cambia solo identita' e cooldown. Per toglierli davvero, il posto e' l'azione core.
	 * `RangeCells` negativo (default) eredita la portata del core; un valore >= 0 la sovrascrive.
	 *
	 * Specchia i campi legacy (`RangeCells`/`CooldownTurns`/`Power`/`Shape`) come `MakeHeroAction`: sono quelli
	 * che `ARTUnit::ConsumeAbility` e `ARTTurnManager` leggono ancora oggi in partita.
	 *
	 * Restituisce `nullptr` se `CoreActionId` non e' nel catalogo o non e' una reazione (slot diverso da
	 * `Reaction`): costruire una "reazione" sopra un'azione principale produrrebbe qualcosa che il pass delle
	 * reazioni non guarderebbe mai, cioe' un'abilita' silenziosamente inerte.
	 */
	static URTActionData* MakeHeroReactionFromCoreAction(const FName& HeroActionId, const FName& CoreActionId,
		int32 CooldownTurns, const TArray<FRTActionEffectSpec>& Effects = TArray<FRTActionEffectSpec>(),
		int32 RangeCells = INDEX_NONE);

	/**
	 * Un'azione d'eroe che EREDITA i suoi valori da un'azione core, e lo dichiara in `DerivedFromActionId`.
	 *
	 * Prende l'**ID** e non il `Def` di proposito: cosi' la derivazione non e' una cosa da annotare dopo
	 * aver letto il catalogo, e' il modo stesso di leggerlo.
	 *
	 * ⚠️ **Sta qui accanto al fratello, e non nel namespace anonimo del `.cpp`, per una ragione precisa**:
	 * la prima stesura lo teneva privato e il suo test provava `MakeHeroReactionFromCoreAction` dicendo che
	 * aveva «la stessa guardia». Non e' vero — quella ha una clausola in piu' sullo slot — e la differenza
	 * nascondeva un fail-OPEN su `NAME_None`. Un helper che nessun test puo' chiamare e' un helper che
	 * nessun test verifica.
	 *
	 * ⛔ Restituisce `nullptr` se `CoreActionId` e' vuoto o non e' nel catalogo. Chi lo chiama **deve**
	 * guardare il risultato: `GetHeroRoster()` gira all'avvio, e un dereferenziamento nullo li' non e' un
	 * fail-closed, e' un crash.
	 *
	 * ⚠️ Eredita **identita', fase, priorita', portata, fallback ed effetti** — e nient'altro. Restano fuori
	 * lo `Slot` e tutti i campi di comportamento: `MovementStyle`, `StructureOp`, `PropagationLimit`,
	 * `bCreatesSurface`/`SurfaceCreated`/`SurfaceRadius`, `InterruptPolicy`, `bSelfTarget`,
	 * `bAllowsReaction`, `bTargetsCell`, `ReactionTrigger` e i campi predittivi. L'elenco e' lungo apposta:
	 * un'abilita' derivata da `Action.Ignite` che non copia `bCreatesSurface` risolve e non crea niente.
	 */
	static URTActionData* MakeHeroActionFromCore(const FName& HeroActionId, const FName& CoreActionId,
		int32 Cooldown, ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0);
};
