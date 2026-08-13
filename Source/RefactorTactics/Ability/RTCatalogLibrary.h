#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTTurnRules.h"

class URTEquipmentData;
class URTActionData;
#include "RTCatalogLibrary.generated.h"

/**
 * Lettura e validazione del catalogo azioni: pura, deterministica, senza Actor e senza asset.
 *
 * Il validator e' una funzione pura per costruzione, cosi' a M11 puo' diventare un commandlet di CI senza
 * riscritture (stessa disciplina di URTHexMapAsset::ValidateMap).
 */
UCLASS()
class REFACTORTACTICS_API URTCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Macro-fase di Atlas in cui l'azione risolve davvero. Funzione TOTALE: ogni valore dell'enum ha una
	 * macro-fase, nessun default silenzioso (il test lo verifica valore per valore).
	 *
	 * Rimappatura (ADR-0003 §3): Snapshot -> Planning · Preparation -> Prep · FastMovement -> **Dash** ·
	 * NormalMovement -> **Move** (dopo il Blast: qui il catalogo divergeva) · Control -> Blast ·
	 * Attack -> Blast · Environment -> Cleanup (dopo il Move) · Cleanup -> Cleanup.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static ERTMatchPhase MapResolutionPhase(ERTResolutionPhase Phase);

	/**
	 * Vero se l'azione e' una mobilita' RAPIDA, cioe' risolve nella macro-fase Dash (scatto, carica, salto,
	 * riposizionamento, corsa). E' l'unico gate di «questa azione e' uno scatto»: prima di #142 il resolver
	 * leggeva la fase e il resto del gioco un flag booleano sull'asset, e le due risposte divergevano — le
	 * azioni degli eroi dichiarano la fase e non il flag, quindi il bot non pianificava scatti per loro.
	 *
	 * Non dice COME ci si sposta: quello e' `FRTActionDef::MovementStyle`, ed e' cio' che distingue una
	 * mobilita' lineare da una a budget dentro la stessa fase.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static bool IsFastMovement(const FRTActionDef& Def);

	/**
	 * Danno dichiarato da un'azione: il PRIMO effetto `Damage` della sua lista, 0 se non ne ha. Una sola
	 * risposta alla domanda «quanto fa male questa azione», condivisa da chi la valuta (il bot) e da chi la
	 * applica (il Blast) — la stessa disciplina con cui `IsFastMovement` ha unificato il gate dello scatto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 FirstDamage(const FRTActionDef& Def);

	/** Codice numerico del catalogo (0/10/20/30/40/50/60): serve a rileggere i PDF, non alla risoluzione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 ResolutionPhaseCode(ERTResolutionPhase Phase);

	/**
	 * Errori strutturali di un catalogo di azioni (vuoto = catalogo valido). Rifiuta:
	 * ID assente o duplicato · priorita' negativa · portata/costo/cooldown negativi · azione che dichiara di
	 * risolvere nello Snapshot (fase di congelamento: nessuna azione risolve li') · azione di movimento con
	 * fallback diverso da `Stop` (regola del vertical slice: ci si ferma nell'ultima cella valida).
	 *
	 * Ogni messaggio nomina l'azione colpevole: un errore che non dice QUALE riga e' rotta costringe a
	 * ricontrollare tutto il catalogo a mano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Catalog")
	static TArray<FString> ValidateActions(const TArray<FRTActionDef>& Actions);

	/**
	 * Errori strutturali di un insieme di equipaggiamenti (vuoto = valido). Rifiuta: id assente o duplicato ·
	 * cooldown negativo · **svantaggio non dichiarato**. Quest'ultima e' la regola di prodotto: un
	 * equipaggiamento senza svantaggio e' una scelta verticale, cioe' potere che si accumula.
	 */
	static TArray<FString> ValidateEquipment(const TArray<const URTEquipmentData*>& Equipment);

	/**
	 * `Gadget.PortableCover` del catalogo equipaggiamento (CP 9.5). E' l'**unico** gadget costruito in v0.1, e
	 * non e' l'inizio del catalogo completo: quello, con slot, loadout e validazione dell'insieme, e' l'epic E7
	 * (`#61`, `#63`). Sta qui perche' il DoD di questo checkpoint lo nomina, e perche' e' il secondo
	 * consumatore di `Action.CreateCover` — cioe' la prova che la semantica e' condivisa e non e' di Bastion.
	 */
	static URTEquipmentData* MakePortableCoverGadget();

	/**
	 * Le **sei varianti d'arma** del catalogo equipaggiamento §1 (CP 7.1, `#60`).
	 *
	 * Costruite in C++ come il roster (`MakeBastion`) e non come asset di `Content/`: sono dati di catalogo che
	 * il gioco spedisce sempre, e un asset li renderebbe modificabili senza passare da una review del diff —
	 * oltre a richiedere l'editor per una regola che non ne ha bisogno.
	 *
	 * Ogni variante dichiara il proprio svantaggio **due volte e in due lingue**: come prosa in `Drawback`, per
	 * chi sceglie, e come numero nei delta, per il resolver. `ValidateEquipment` verifica che le due non
	 * divergano — cioe' che nessuna variante sia migliore in ogni parametro.
	 */
	static TArray<URTEquipmentData*> MakeWeaponVariants();

	/**
	 * I moduli di reazione del catalogo §3 che il motore **sa far scattare** (CP 7.3 `#62`, CP 7.5 `#505`).
	 *
	 * Sono **sei** dei sette. L'unico assente e' `HazardEscape`, e non gli manca un dato: gli manca un
	 * PREREQUISITO. Una superficie che nasce sotto un'unita' ferma oggi non le fa niente — tranne l'acqua,
	 * che ha un ramo suo — quindi nel Cleanup non c'e' nessun danno imminente da cui fuggire e il modulo
	 * sarebbe inerte: la trappola di `Riva.MistVeil` (`#353`). Lo chiude `#570`, e questo modulo lo segue.
	 *
	 * I sei raccontano le tre ragioni per cui un modulo puo' restare fermo, e la differenza porta a lavori
	 * diversi: a `EmergencyDash` mancava l'**effetto** (`SelfReposition`, D-093, perche' `Push`/`Pull`
	 * spostano il bersaglio e nessun effetto muoveva la sorgente); ad `Anchor` e `Cleanse` mancava il
	 * **momento** — si valutano dove il loro evento e' deciso e non ancora applicato, che sono i punti
	 * `BlastDisplacement` e `BlastStatus` di `URTReactionLibrary::PassPointFor`; a `HazardEscape` manca
	 * l'**evento** stesso, che nessuno produce.
	 *
	 * Ogni modulo si costruisce su un'azione core che e' **gia' una reazione**, da cui eredita fase, priorita'
	 * e trigger, e porta effetti propri via `GrantedEffects`. E' lo stesso vincolo che
	 * `MakeHeroReactionFromCoreAction` impone agli eroi: sopra un'azione principale il modulo sarebbe
	 * silenziosamente inerte, perche' il pass delle reazioni non lo guarderebbe mai.
	 */
	static TArray<URTEquipmentData*> MakeReactionModules();

	/**
	 * I gadget del catalogo equipaggiamento §2 che il motore sa gia' far funzionare (CP 7.2, `#61`).
	 *
	 * Sono **quattro** degli otto — `Medkit`, `BreachCharge`, `Sprinkler`, `PortableCover` — e i quattro
	 * assenti mancano per quattro ragioni distinte, scritte accanto a ciascuno nel `.cpp`: manca l'azione
	 * core (`SmokeEmitter`), manca il modello di immunita' (`Insulator`), manca l'epic della conoscenza
	 * parziale (`Sensor`), manca un consumo per turno che `PushResistance` non e' (`Anchor`).
	 *
	 * Tutti hanno **cooldown 3**, che il catalogo dichiara una volta sopra la tabella invece che riga per riga.
	 */
	static TArray<URTEquipmentData*> MakeGadgets();

	/**
	 * L'attacco base modificato dalla variante: e' **il consumatore** dei delta, e l'unico.
	 *
	 * Funzione pura su `FRTActionDef` e non su `URTActionData` per la ragione di sempre: cosi' e' verificabile
	 * senza costruire un `UObject`, un mondo o un'unita'. Chi ha bisogno dell'azione come oggetto la costruisce
	 * dopo, dalla def che esce di qui.
	 *
	 * Regole, tutte visibili nel test `Equipment.WeaponVariantHasTradeoff`:
	 * - i delta si **sommano**, non sostituiscono: la variante non conosce l'arma che modifica;
	 * - portata e cooldown non scendono **mai sotto zero** — `Weapon.Impact` su un'arma a portata 1 la lascia a
	 *   contatto invece di produrre una portata negativa, che il validator del catalogo rifiuterebbe;
	 * - il danno **non** e' clampato a zero: un attacco base portato sotto zero e' un errore di bilanciamento
	 *   che deve **restare visibile**, e `ValidateEquipment` lo dice sul catalogo invece di nasconderlo qui;
	 * - `AddedEffects` va in **coda**, cosi' un attacco che gia' bagna continua a bagnare.
	 *
	 * Una variante che non sia `WeaponVariant` non modifica niente: restituisce l'azione invariata.
	 */
	static FRTActionDef ApplyWeaponVariant(const FRTActionDef& BasicAttack, const URTEquipmentData* Variant);

	/**
	 * Applica la variante a un'ISTANZA di attacco base, `Def` **e** campi specchio legacy.
	 *
	 * Esiste perche' `ApplyWeaponVariant` lavora su `FRTActionDef` — puro e testabile — mentre in partita chi
	 * decide legge altrove: il bot pianifica su `Ability->RangeCells` e `Ability->Power`
	 * (`RTTurnManager.cpp`, ramo delle candidate d'attacco) e il resolver costruisce l'intento con
	 * `Intent.RangeCells = Ability->RangeCells`. Sono i campi legacy dell'MVP quadrato, che `URTActionData`
	 * porta ancora.
	 *
	 * ⚠️ **Senza questa funzione il ponte esisteva solo in un helper di TEST**, ed e' un difetto che nessun
	 * test avrebbe scoperto: l'helper faceva la cosa giusta, quindi verificava un percorso che la produzione
	 * non avrebbe attraversato. Un loadout che assegnasse solo `->Def` avrebbe prodotto una `Weapon.Precision`
	 * che non usa la cella pagata con 4 danni, una `Weapon.Impact` che fa pianificare al bot un attacco che il
	 * resolver poi rifiuta — il *pulsante finto* — e una `Weapon.Overcharge` che non applica mai la ricarica,
	 * cioe' il suo intero prezzo ([D-090](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * Chi cabla il loadout (CP 7.4, `#63`) deve chiamare **questa**, non `ApplyWeaponVariant` da solo.
	 */
	static void EquipWeaponVariant(URTActionData* BasicAttack, const URTEquipmentData* Variant);

	/**
	 * La variante d'arma di **default** per un eroe (D-089), o `None` se l'eroe non ne ha una dichiarata.
	 *
	 * Il criterio e' che il default **rinforzi l'identita'** dell'eroe; compensarne la debolezza resta la
	 * scelta alternativa del giocatore. `None` invece di un fallback e' voluto: un default silenzioso e
	 * sbagliato e' peggio di un default assente, che almeno si vede.
	 */
	static FName DefaultWeaponVariantFor(const FName& HeroId);

	/**
	 * Avvisi (non errori) su una combinazione eroe/variante: cose legali che con ogni probabilita' non si
	 * vogliono, e che nessun validator strutturale puo' vedere perche' dipendono dalla **coppia**.
	 *
	 * Oggi ne produce due, ed entrambe nascono da D-089:
	 * - la variante **duplica uno status** che l'attacco base gia' applica (`Suppressive` su
	 *   `Bastion.ImpactShot`, che rallenta di suo): si paga il costo pieno per un effetto che si ha;
	 * - la variante porta il danno diretto **a zero o sotto**: un pulsante finto, che ADR-0007 esiste per
	 *   evitare.
	 *
	 * Sono `TArray<FString>` come gli errori di `ValidateEquipment` e non un booleano, perche' un avviso che
	 * non dice **quale** coppia e **perche'** costringe a ricontrollare tutto il roster a mano.
	 */
	static TArray<FString> WarnOnVariantForAttack(const FRTActionDef& BasicAttack,
		const URTEquipmentData* Variant);

	/**
	 * Errori di un LOADOUT (vuoto = valido). La regola del catalogo e' **1+1+1**: esattamente una variante
	 * d'arma, un gadget e un modulo di reazione — CP 7.4.
	 *
	 * «Esattamente» in entrambe le direzioni, e non e' pedanteria: **zero** e **due** sono errori diversi e
	 * il messaggio li distingue, perche' portano a correzioni diverse — un pezzo dimenticato contro un pezzo
	 * di troppo. Un loadout con due gadget non e' «piu' equipaggiato»: e' una configurazione che il gioco non
	 * sa risolvere, visto che ogni slot ha un solo posto nell'economia del turno.
	 *
	 * Passa anche da `ValidateEquipment` sui singoli pezzi: un loadout di tre oggetti rotti ha la forma
	 * giusta e il contenuto sbagliato, e sarebbe accettato da un controllo che conta soltanto.
	 */
	static TArray<FString> ValidateLoadout(const TArray<const URTEquipmentData*>& Loadout);

	/**
	 * L'azione che un equipaggiamento concede, costruita dall'azione CORE che dichiara in `GrantedActionId`.
	 * Nullptr se non ne concede nessuna o se l'id non e' nel catalogo.
	 *
	 * Conserva la semantica del core (fase, priorita', portata, `StructureOp`, e per le reazioni il
	 * `ReactionTrigger`) e sostituisce **tre** cose:
	 * - l'**ActionId**, che diventa quello dell'equipaggiamento perche' il TurnLog deve dire CHI ha agito;
	 * - il **cooldown**, che e' dell'oggetto — un gadget si ricarica coi suoi tempi, non con quelli dell'azione;
	 * - gli **effetti**, ma **solo se** l'oggetto ne dichiara di propri in `GrantedEffects` (CP 7.2/7.3): un
	 *   modulo di reazione eredita dal core cio' che lo rende una reazione e porta numeri suoi
	 *   (`Reaction.CounterShot` fa 14 dove `Action.Counter` ne fa 16). Con `GrantedEffects` vuoto gli effetti
	 *   del core restano intatti, ed e' il caso di `Gadget.Sprinkler`, il cui esito e' una superficie.
	 *
	 * ⚠️ **Non e' fail-closed come `MakeHeroReactionFromCoreAction`**, che rifiuta un core non-reazione perche'
	 * costruirci sopra darebbe un'abilita' che il pass delle reazioni non guarda mai. Qui la stessa disciplina
	 * e' affidata al chiamante — `MakeReactionModules` sceglie apposta core gia' reazione — e verificata da
	 * `Equipment.ReactionModule.SingleActivation`, che asserisce slot e trigger sull'azione prodotta.
	 */
	static URTActionData* MakeEquipmentAction(const URTEquipmentData* Item, UObject* Outer);

	/**
	 * Il pezzo di equipaggiamento con questo `EquipmentId`, cercato nei **tre** cataloghi — varianti d'arma
	 * (§1), gadget (§2), moduli di reazione (§3) — o `nullptr` se non esiste (`#602`).
	 *
	 * Una funzione sola e non tre lookup dal chiamante: chi equipaggia non deve sapere in quale sezione del
	 * catalogo viva un pezzo, e un `EquipmentId` e' unico fra le tre — lo stesso id in due sezioni sarebbe un
	 * difetto di catalogo, non un caso da gestire qui.
	 *
	 * ⚠️ L'oggetto e' **appena costruito** (i cataloghi si generano a ogni chiamata) e non e' radicato: va usato
	 * subito, o passato a `MakeEquipmentAction` che ne copia cio' che serve. Tenerne il puntatore attraverso un
	 * frame significa leggere memoria che il GC puo' aver ripreso.
	 */
	static const URTEquipmentData* FindEquipment(FName EquipmentId);

	/**
	 * Le azioni GENERICHE del catalogo v0.1 (`Action.*`), quelle che non appartengono a un eroe.
	 *
	 * Fino al 2026-08-10 conviveva con `GetShippedActionCatalog`, il catalogo dei due archetipi legacy
	 * (`Ranger.*`, `Guardian.*`) con le loro otto azioni. Quel catalogo esisteva per tenere allineate le
	 * definizioni a cio' che `ARTUnit::ConfigureAsArchetype` assegnava davvero — ma quel percorso era
	 * uscito dalla partita a CP 6.6, quindi l'allineamento era verificato su unita' che nessuno schierava.
	 * Le azioni degli eroi vivono in `URTHeroCatalogLibrary`, e sono quelle che il gioco spedisce.
	 *
	 * Contiene le azioni fondamentali (§1), le mobilita' (§2) e le offensive (§3) del catalogo v0.1.
	 * Controllo, supporto e ambiente arrivano con CP 4.7 e le epic E8/E9.
	 */
	static TArray<FRTActionDef> GetCoreActionCatalog();

	/**
	 * Azione generica con l'ActionId dato, o una definizione vuota se l'ID non e' nel catalogo.
	 *
	 * Non esiste un redirect di ID ritirati: [D-132] lo ha rimosso. Un ID che il catalogo non conosce
	 * torna vuoto e basta — chi legge una traccia vede l'ID che la traccia dichiaro', non un erede.
	 */
	static FRTActionDef FindCoreAction(const FName& ActionId);

	/**
	 * Danno dell'attacco base per FASCIA di portata (catalogo v0.1 §1): corpo a corpo 28/r1 · corto 25/r3 ·
	 * medio 22/r4 · lungo 20/r6. Piu' lontano si colpisce, meno si fa male — e' la scelta orizzontale del
	 * catalogo, non una scala di potenza.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 BasicAttackDamageForRange(int32 WeaponRangeCells);

	/**
	 * `Action.BasicAttack` completata per una data arma: identita', fase, priorita' e fallback vengono dal
	 * catalogo, portata e danno dalla fascia. Un solo ActionId per tutti gli eroi, non quattro varianti.
	 */
	static FRTActionDef MakeBasicAttack(int32 WeaponRangeCells);

	/**
	 * Un'azione a bersaglio completata per una data arma: identita', danno, fase, priorita' e fallback
	 * vengono dal catalogo, la PORTATA dall'arma dell'eroe (il catalogo dichiara «bersaglio», non un numero).
	 * Portate degeneri ricadono su 1: un'azione a portata zero non colpirebbe nessuno.
	 */
	static FRTActionDef MakeWeaponAttack(const FName& ActionId, int32 WeaponRangeCells);

	/**
	 * `Action.PrecisionAttack` per una data arma: portata dell'arma **+1** (catalogo v0.1 §3), danno 24 dal
	 * catalogo. Il bonus e' l'identita' dell'azione, non un parametro del chiamante.
	 */
	static FRTActionDef MakePrecisionAttack(int32 WeaponRangeCells);

	/**
	 * Errori negli SLOT di un piano di turno di una singola unita' (vuoto = piano valido): due azioni non
	 * possono occupare lo stesso slot, e chi consuma entrambi (`Action.Sprint`) non lascia spazio a nessuna
	 * azione principale.
	 *
	 * E' qui che «PrecisionAttack non e' usabile dopo Sprint» diventa una regola generale invece di
	 * un'eccezione sull'ActionId: lo Sprint prende la principale, e l'attacco non la trova piu'.
	 */
	static TArray<FString> ValidateActionSlots(const TArray<FRTActionDef>& PlannedActions);

	/**
	 * Quale dei due slot del turno consuma un'azione. `MovementAndMain` (`Action.Sprint`) risponde **vero a
	 * entrambe**, ed e' l'unico motivo per cui queste sono due funzioni e non un `== Slot`.
	 *
	 * Estratte da `ValidateActionSlots` il 2026-08-13, quando la HUD di CP 11.1 ha avuto bisogno della stessa
	 * domanda al contrario — non «questo piano e' valido?» ma «quali slot risultano occupati?». La regola
	 * resta una sola: due copie di `Slot == Movement || Slot == MovementAndMain` sarebbero divergite alla
	 * prima azione che consuma gli slot in un modo nuovo, e il widget avrebbe mostrato uno slot libero che il
	 * validatore considera pieno.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static bool TakesMovementSlot(const FRTActionDef& Action);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static bool TakesMainSlot(const FRTActionDef& Action);

	/**
	 * Gli `ActionId` delle azioni **generiche** che ogni unita' possiede in aggiunta al proprio kit (D-025).
	 *
	 * Sono **tre** delle sette dichiarate, e le altre quattro mancano per ragioni diverse che vale la pena
	 * distinguere:
	 *
	 * - `Move` e `BasicAttack` **ci sono gia'**, per altre strade: il movimento passa da `PlannedPath` e non
	 *   da uno slot azione, e l'attacco base e' l'indice 0 del kit di ogni eroe (catalogo v0.1);
	 * - `Interact` non ha un **consumatore**: nessun codice risolve un'interazione. Aggiungerla darebbe a ogni
	 *   unita' un comando che non fa niente — cioe' lo stesso difetto che questa lista corregge;
	 * - `Overwatch` non esiste nemmeno nel catalogo core: e' **E14** (ADR-0004, finestre di reazione), e
	 *   arrivera' con la sua infrastruttura.
	 *
	 * `Guard`, `Brace` e `Wait` entrano perche' sono complete dall'altra parte: `Status.Guarded` e
	 * `Status.Braced` hanno gia' quattro consumatori nel `TurnManager` — riduzione del danno e resistenza alle
	 * spinte — che senza un produttore restavano irraggiungibili in partita.
	 */
	static TArray<FName> GetGenericActionIds();

	/**
	 * Istanze NUOVE delle azioni generiche, da accodare al kit di un'unita'.
	 *
	 * Nuove e non condivise: `URTActionData` e' un oggetto, e due unita' che ne condividessero uno
	 * condividerebbero anche ogni campo che un giorno diventasse mutabile. Il cooldown vive gia' nell'unita'
	 * (`AbilityCooldowns`), ma la garanzia sta nell'istanza, non nel fatto che oggi non ci sia altro stato.
	 *
	 * @param Outer proprietario degli oggetti creati: l'unita' che li accoda, cosi' la loro vita e' la sua.
	 */
	static TArray<URTActionData*> MakeGenericActions(UObject* Outer);
};
