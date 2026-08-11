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
	 * I moduli di reazione del catalogo §3 che l'infrastruttura E5 **sa gia' far scattare** (CP 7.3, `#62`).
	 *
	 * Sono **tre** dei sette, e i quattro assenti mancano per due ragioni diverse che vale la pena distinguere,
	 * perche' portano a lavori diversi (entrambi in `#505`, CP 7.5):
	 *
	 * - `HazardEscape`, `Cleanse`, `Anchor` — il **trigger** non esiste: nascono da un evento (`Push`, `Pull`,
	 *   `Status`) e non da un colpo, mentre `EvaluateReactionTrigger` riceve `Hits` e `Intents`;
	 * - `EmergencyDash` — il trigger c'e' (`HitByDirectAttack`), ma manca l'**effetto**: `Reposition 1` sposta
	 *   chi reagisce, e nessun `ERTActionEffect` lo esprime (`Push`/`Pull` spostano il bersaglio).
	 *
	 * Ogni modulo si costruisce su un'azione core che e' **gia' una reazione**, da cui eredita fase, priorita'
	 * e trigger, e porta effetti propri via `GrantedEffects`. E' lo stesso vincolo che
	 * `MakeHeroReactionFromCoreAction` impone agli eroi: sopra un'azione principale il modulo sarebbe
	 * silenziosamente inerte, perche' il pass delle reazioni non lo guarderebbe mai.
	 */
	static TArray<URTEquipmentData*> MakeReactionModules();

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
	 * L'azione che un equipaggiamento concede, costruita dall'azione CORE che dichiara in `GrantedActionId`.
	 * Nullptr se non ne concede nessuna o se l'id non e' nel catalogo.
	 *
	 * Conserva la semantica del core (fase, priorita', portata, `StructureOp`) e sostituisce due cose:
	 * l'**ActionId**, che diventa quello del gadget perche' il TurnLog deve dire CHI ha agito, e il
	 * **cooldown**, che e' dell'oggetto — un gadget si ricarica coi suoi tempi, non con quelli dell'azione.
	 */
	static URTActionData* MakeEquipmentAction(const URTEquipmentData* Item, UObject* Outer);

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
	 * Traduce uno Stable ID **ritirato** nel suo erede; restituisce l'ID invariato se non lo e' (#199).
	 *
	 * Serve perche' gli ActionId entrano nel TurnLog serializzato: un ID tolto dal catalogo resta scritto nei
	 * file gia' su disco, e [D-014] richiede che non venga cancellato ma reindirizzato. Oggi la tabella ha
	 * una voce sola — `Action.Activate` -> `Action.Interact`, per [D-025].
	 */
	static FName ResolveLegacyActionId(const FName& ActionId);

	/**
	 * Azione generica con l'ActionId dato, o una definizione vuota se l'ID non e' nel catalogo.
	 * Gli ID ritirati passano da `ResolveLegacyActionId`, quindi rispondono con l'azione erede.
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
