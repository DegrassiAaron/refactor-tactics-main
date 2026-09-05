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
	 * push) · affinita' o debolezza non dichiarate · numero di azioni diverso da **5** (attacco base + quattro
	 * fondamentali, catalogo v0.1 §"Struttura di un eroe") · zero o piu' di UNA azione fondamentale con
	 * varianti dichiarate (vincolo v0.1: una sola abilita' configurabile per eroe).
	 *
	 * Ogni messaggio nomina l'eroe colpevole: un errore che non dice QUALE eroe e' rotto costringe a
	 * ricontrollare tutto il roster a mano.
	 *
	 * Non e' `UFUNCTION`: UHT non riflette `TArray<const T*>` (stesso limite dichiarato da
	 * `URTCatalogLibrary::ValidateEquipment`), quindi resta una funzione C++ pura, non chiamabile da Blueprint.
	 */
	static TArray<FString> ValidateHeroes(const TArray<const URTHeroData*>& Heroes);

	/**
	 * Costruisce **Gadget**, tecnico della conduzione (catalogo eroi v0.1 §1): 90 HP, 5 MP, vista 6, resistenza
	 * push 0, affinita' elettricita', debolezza acqua (combo dichiarata con Phase, CP 6.3). Nuova istanza a
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
	 * Limite dichiarato che resta (nessun sistema a valle esiste ancora, quindi l'effetto non e'
	 * rappresentabile): `Overload` non ha un modello di "dispositivo interrompibile" (E7, gadget). L'azione
	 * esiste comunque come DATO, con la sua identita', portata e cooldown dal catalogo — solo l'effetto
	 * aggiuntivo resta un numero non ancora consumato.
	 */
	static URTHeroData* MakeGadget();

	/**
	 * Costruisce **Phase**, manipolatrice dell'acqua (catalogo eroi v0.1 §2): 95 HP, 5 MP, vista 5, resistenza
	 * push 0, affinita' acqua, debolezza elettricita' — simmetrica a Gadget (stesso `Affinity.Electricity`),
	 * cosi' la rivalita' fra i due e' un solo identificatore condiviso, non due nomi.
	 *
	 * Limiti dichiarati: `CircularTide` dichiara Heal E Status.Wet nella stessa lista Effects, ma **nessun
	 * resolver applica oggi effetti diversi ad alleati e nemici dentro la stessa area** (`bFriendlyFire`
	 * decide solo SE un alleato viene colpito, non CON QUALE effetto) — la differenziazione arriva quando
	 * Phase sara' davvero cablata (CP 6.6+). `FluidTrail` (acqua lungo il percorso) e `MistVeil` (fumo) non
	 * hanno un modello di terreno dinamico (E8/E9): identita', fase, portata e cooldown sono dati veri,
	 * l'effetto no. `FlowReaction` e' **rinviata a E14** (ADR-0004): produce movimento dentro un boundary di
	 * risoluzione, che il motore delle reazioni di E5 non fa — il rinvio e' dichiarato come dato (slot `None`,
	 * nessun trigger), non lasciato a un commento.
	 */
	static URTHeroData* MakePhase();

	/**
	 * Costruisce **Riktor**, architetto del campo (catalogo eroi v0.1 §3): 120 HP, 4 MP, vista 5,
	 * **resistenza push 1** (l'unico del roster), affinita' strutture, debolezza movimento — simmetrica a
	 * Wraith (CP 6.5), come Gadget/Phase lo sono fra loro.
	 *
	 * ⚠️ **Resta l'eroe piu' incompleto del roster, e la sua issue (#57) chiede esplicitamente di dichiararlo.**
	 * `KineticPanel` e `Reconfigure` manipolano STRUTTURE, che non esistono nel modello dati (`FRTHexCellData`
	 * non ha coperture: E9, issue `#69`/`#73`) — nessun effetto dichiarabile, i numeri del catalogo terreni
	 * (`Structure.KineticPanel`: integrita' 30, protezione 10) vivono nei `Parameters` della variante finche'
	 * qualcuno non li consuma. `Interposition` e' **cablata** (CP 6.7) sulla semantica di `Action.Intercept`.
	 */
	static URTHeroData* MakeRiktor();

	/**
	 * Costruisce **Wraith**, duellante predittivo (catalogo eroi v0.1 §4): 100 HP, **6 MP** (il piu' mobile),
	 * vista 6, resistenza push 0, affinita' movimento, debolezza strutture — simmetrica a Riktor, che chiude
	 * il roster in due coppie (Gadget↔Phase, Riktor↔Wraith).
	 *
	 * `Deflection` e' cablata (CP 6.7, semantica di `Action.Deflect`). `InterceptShot` **non e' piu' una
	 * reazione**: dal 2026-08-10 (E18 CP 18.2, D-016) e' una **Predictive Action** — cella dichiarata in
	 * Planning, verificata al boundary del Move, nessun input durante la Resolution. Il rinvio a E14 e'
	 * caduto per la ragione opposta a quella che l'aveva prodotto: non le serve una finestra interattiva.
	 *
	 * Limiti dichiarati: `Feint` marca una CELLA e concede un `Reposition`, e nessuna delle due meta' e' un
	 * `ERTActionEffect` (gli stati si applicano alle unita', il movimento passa da `ERTMovementStyle`).
	 */
	static URTHeroData* MakeWraith();

	/**
	 * Il roster completo della v0.1, nell'ordine del catalogo eroi: Gadget, Phase, Riktor, Wraith.
	 * Nuove istanze a ogni chiamata (stesso idioma di `URTCatalogLibrary::GetCoreActionCatalog`).
	 */
	static TArray<URTHeroData*> GetHeroRoster();

	/**
	 * Solo gli `HeroId` del roster (`Hero.Gadget`, `Hero.Phase`, `Hero.Riktor`, `Hero.Wraith`), senza costruire
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
	 * Esiste perche' l'alternativa e' peggiore: senza, `Riktor.Interposition` dovrebbe riscrivere la
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
