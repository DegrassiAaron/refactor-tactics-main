#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTTurnRules.h"

class URTEquipmentData;
class URTActionData;
#include "RTCatalogLibrary.generated.h"

/**
 * Una risposta del Reaction Profile, e gli **effetti** con cui si esprime (`spec-reaction-clash-e14.md` §5).
 *
 * ⚠️ **Gli effetti non sono un ornamento: sono cio' che rende la risposta eseguibile.** §5 dice che gli esiti
 * si esprimono *«solo con le primitive del catalogo effetti gia' esistente … mai una callback»*, e una
 * risposta senza effetti e' una scelta che il resolver non sa applicare — offrirla aprirebbe una finestra su
 * un'opzione inerte, che e' peggio del non aprirla.
 *
 * 🔴 **Oggi UNO solo dei tre profili la dichiara, e l'assenza degli altri due e' un REGISTRO, non una
 * dimenticanza**: `spec-reaction-clash-e14.md` §2.5 e [D-132] lasciano esplicitamente aperti «Charge del
 * `Grounding`» e «ampiezza della deviazione». `Profile.Sidestep` ce l'ha perche' `BAS-4` lo ha deciso — «e'
 * nuovo, si chiama `Profile.Sidestep` e risponde al **Forced Movement**», nella stessa forma di
 * `Hero.Phase.FlowReaction` (`Reposition 1`) — e si esprime con `SelfReposition`, la primitiva che
 * `Reaction.EmergencyDash` e `Reaction.HazardEscape` gia' usano. Nessun numero nuovo entra qui.
 */
USTRUCT()
struct FRTReactionResponseDef
{
	GENERATED_BODY()

	/** Il token della risposta, come compare in `AllowedResponses`: `SIDESTEP`, `GROUND`, `GLANCE LEFT`. */
	UPROPERTY()
	FString Response;

	/**
	 * Le primitive con cui la risposta si applica. **Vuoto = dichiarata ma non eseguibile**: il catalogo la
	 * conta nella cardinalita' di [D-132] e il resolver non la offre, finche' la decisione che le manca non
	 * e' presa. Le due domande sono diverse e questa e' la seconda.
	 */
	UPROPERTY()
	TArray<FRTActionEffectSpec> Effects;

	FRTReactionResponseDef() = default;
	FRTReactionResponseDef(const FString& InResponse, const TArray<FRTActionEffectSpec>& InEffects)
		: Response(InResponse), Effects(InEffects) {}
};

/**
 * Un **Reaction Profile**: cio' che `Action.Brace` arma, oltre alla risposta universale (E14.7, [D-047]).
 *
 * ⚠️ **Vive nel catalogo e non fra i dati d'eroe**, ed e' `spec-reaction-clash-e14.md` §2.5 a deciderlo:
 * *«un profilo e' un'entita' di catalogo, non un'abilita' d'eroe: vive nel namespace `Profile.<Nome>` come
 * `Action.Brace` e `Reaction.Anchor`»*. Le tre conseguenze che un prefisso d'eroe non avrebbe dato: nessun
 * profilo puo' collidere con un'abilita', nessun token nasce con un nome che [D-120] ha declassato a legacy,
 * e un profilo si **riassegna** fra eroi senza rename quando il roster cresce.
 *
 * 🔵 **`ExtraResponses` sono le risposte OLTRE `Hold Ground`, e la distinzione porta la cardinalita'.**
 * `Hold Ground` e' universale e coincide col comportamento di oggi — cioe' con `Status.Braced` — quindi non
 * si elenca: elencarla renderebbe possibile un profilo che la **omette**, e allora «il profilo base ha
 * cardinalita' 1» smetterebbe di essere vero per costruzione. Il profilo base e' l'array **vuoto**.
 */
USTRUCT()
struct FRTReactionProfileDef
{
	GENERATED_BODY()

	/** `Profile.<Nome>`, senza prefisso d'eroe. Vuoto = profilo base. */
	UPROPERTY()
	FName ProfileId;

	/**
	 * Le risposte oltre `Hold Ground`. **Vuoto per il profilo base**, e in quel caso la cardinalita' resta 1
	 * — nessuna finestra si apre, che e' il caso di Riktor ([D-047], §2.5).
	 *
	 * ⚠️ Da `TArray<FString>` a `TArray<FRTReactionResponseDef>` il 2026-08-19: una risposta porta ora con se'
	 * gli effetti con cui si applica. Il **contenuto** non cambia — i token e la cardinalita' di [D-132] sono
	 * gli stessi — e cambia cio' che il resolver puo' chiedere al catalogo senza dedurlo.
	 */
	UPROPERTY()
	TArray<FRTReactionResponseDef> ExtraResponses;

	FRTReactionProfileDef() = default;
	FRTReactionProfileDef(const FName& InProfileId, const TArray<FRTReactionResponseDef>& InExtraResponses)
		: ProfileId(InProfileId), ExtraResponses(InExtraResponses) {}
};

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
	 * I Reaction Profile del roster v0.1 (E14.7, [D-047] · `spec-reaction-clash-e14.md` §2.5).
	 *
	 * **Tre, non quattro**: `Profile.Grounding` (Gadget), `Profile.Sidestep` (Phase), `Profile.Glance`
	 * (Wraith). Riktor **non ne ha uno**, e non e' un taglio di contenuto — la risposta proposta era
	 * `ANCHOR`, «annulla lo spostamento», ma `Hold Ground` lo fa gia' con la stessa ampiezza: il ramo
	 * `Braced` del resolver non controlla `KnockDist`. Una seconda risposta che coincide con la prima
	 * lascerebbe la cardinalita' a **1** senza aprire nulla, e sarebbe stata la terza scrittura della stessa
	 * regola dopo `Reaction.Anchor` e `Gadget.Anchor`. Il roster conserva cosi' **un eroe senza finestra sul
	 * `Brace`**, che e' la baseline con cui CP 14.6 confronta gli altri tre.
	 *
	 * ⚠️ **Nessun numero di bilanciamento qui**: resistenza del `Brace`, Charge del `Grounding` e ampiezza
	 * della deviazione restano aperti e si restringono al playtest (§2.5).
	 */
	static TArray<FRTReactionProfileDef> GetReactionProfileCatalog();

	/**
	 * Il profilo con questo id, o il **profilo base** se l'id e' vuoto o sconosciuto.
	 *
	 * ⚠️ Fail-closed nel verso giusto: un id che il catalogo non conosce — un piano salvato prima che il
	 * catalogo cambiasse — da' il profilo base, cioe' **una** risposta e nessuna finestra. Il contrario,
	 * inventare risposte per un profilo che non c'e', aprirebbe un boundary su scelte che il gioco non ha.
	 */
	static FRTReactionProfileDef FindReactionProfile(const FName& ProfileId);

	/**
	 * Le risposte legali che `Action.Brace` offre con questo profilo: `Hold Ground` **piu'** le sue extra.
	 *
	 * E' il punto in cui la cardinalita' di [D-047] nasce, e per questo sta nel catalogo e non nel resolver:
	 * col profilo base da' **1** — nessun boundary, esattamente cio' che e' verde oggi — e con un profilo
	 * d'eroe da' **>= 2**, che apre la finestra con la regola che ADR-0004 §2 **ha gia'**. Nessuna regola
	 * nuova, nessun enum di tipo.
	 */
	static TArray<FString> BraceAllowedResponses(const FName& ProfileId);

	/**
	 * Le risposte che il resolver puo' **eseguire** oggi: `Hold Ground` piu' le extra che dichiarano effetti.
	 *
	 * 🔴 **Non e' un doppione di `BraceAllowedResponses`, e la differenza fra le due e' misurabile**: quella
	 * dice cosa il profilo **dichiara** — la cardinalita' che [D-132] ha deciso, 2/2/3 — e questa cosa il
	 * gioco **sa applicare**. Oggi divergono su due profili, e la divergenza non nasce qui: `Profile.Grounding`
	 * e `Profile.Glance` esistono come contenuto deciso mentre «Charge del `Grounding`» e «ampiezza della
	 * deviazione» restano dichiarati aperti dalla spec §2.5. Questa funzione rende quella distanza
	 * **misurabile** invece di lasciarla implicita, e sparisce il giorno in cui le due voci si chiudono.
	 *
	 * ⚠️ E' fail-closed nello stesso verso di `FindReactionProfile`: una risposta che il resolver non sa
	 * applicare non si offre. Offrirla aprirebbe una finestra su una scelta inerte — un prompt che rallenta la
	 * resolution per non fare niente, e che nessun test distinguerebbe da uno che funziona.
	 */
	static TArray<FString> BraceExecutableResponses(const FName& ProfileId);

	/**
	 * Gli effetti con cui una risposta del profilo si applica, o **vuoto** se il profilo non la dichiara.
	 *
	 * `Hold Ground` restituisce vuoto ed e' corretto: il suo esito non passa dal motore effetti — e' il
	 * comportamento che il ramo `Status.Braced` del resolver ha da CP 5.2, e [D-047] dichiara per iscritto che
	 * **non si muove di un numero**. Tradurla in primitive sarebbe riscrivere una regola gia' scritta, cioe' la
	 * seconda verita' che questa epic elenca fra i propri rischi.
	 */
	static TArray<FRTActionEffectSpec> BraceResponseEffects(const FName& ProfileId, const FString& Response);

	/**
	 * Tutte le risposte che un Reaction Profile puo' offrire, `Hold Ground` compresa — l'unione, senza
	 * duplicati e in ordine deterministico.
	 *
	 * Esiste per il **loader degli scenari**, che deve poter rifiutare un refuso in `decisions` senza
	 * riscrivere l'elenco: una seconda lista in `RTScenarioLoader.cpp` divergerebbe al primo profilo
	 * aggiunto, e a divergere sarebbe il gate — cioe' il pezzo il cui mestiere e' accorgersene.
	 *
	 * ⚠️ **Elenca le DICHIARATE, non le eseguibili**, e la differenza qui e' voluta: uno scenario deve poter
	 * **nominare** una risposta che il resolver non sa ancora applicare, perche' e' cosi' che un file di
	 * specifica descrive una feature prima che esista — la prassi con cui questo repository scrive gli
	 * scenari `BLOCKED`. A dire «non e' eseguibile» ci pensa il `requires`, non l'errore di parsing.
	 */
	static TArray<FString> AllReactionProfileResponses();

	/** Vero se la stringa e' una delle risposte di `AllReactionProfileResponses`. Confronto esatto. */
	static bool IsKnownReactionProfileResponse(const FString& Response);

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
	 * consumatore di `Action.CreateCover` — cioe' la prova che la semantica e' condivisa e non e' di Riktor.
	 */
	static URTEquipmentData* MakePortableCoverGadget();

	/**
	 * Le **sei varianti d'arma** del catalogo equipaggiamento §1 (CP 7.1, `#60`).
	 *
	 * Costruite in C++ come il roster (`MakeRiktor`) e non come asset di `Content/`: sono dati di catalogo che
	 * il gioco spedisce sempre, e un asset li renderebbe modificabili senza passare da una review del diff —
	 * oltre a richiedere l'editor per una regola che non ne ha bisogno.
	 *
	 * Ogni variante dichiara il proprio svantaggio **due volte e in due lingue**: come prosa in `Drawback`, per
	 * chi sceglie, e come numero nei delta, per il resolver. `ValidateEquipment` verifica che le due non
	 * divergano — cioe' che nessuna variante sia migliore in ogni parametro.
	 */
	/**
	 * La fascia del danno base, secondo le soglie di [D-087]: `Low 1-10` - `Medium 11-18` - `High 19+`.
	 *
	 * ⚠️ Le soglie sono `PROPOSED FOR PLAYTEST` (`WV-2`) e stanno qui perche' il codice deve poterle
	 * applicare; il loro OWNER resta il catalogo equipaggiamento. Cambiarle e' una ritaratura, non un
	 * refactor: nessun test le pinna, e `DamageBandDerivesFromBaseDamage` verifica la monotonia e la
	 * raggiungibilita' delle tre fasce, non i numeri.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static ERTAttackDamageBand DamageBandOf(int32 BaseDamage);

	/** Il danno DICHIARATO da un'azione: il primo effetto `Damage`, o `0` se non ne ha. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 DeclaredDamage(const FRTActionDef& Action);

	static TArray<URTEquipmentData*> MakeWeaponVariants();

	/**
	 * I moduli di reazione del catalogo §3 che il motore **sa far scattare** (CP 7.3 `#62`, CP 7.5 `#505`).
	 *
	 * Sono **sei** dei sette. L'unico assente e' `HazardEscape`, e non gli manca un dato: gli manca un
	 * PREREQUISITO. Una superficie che nasce sotto un'unita' ferma oggi non le fa niente — tranne l'acqua,
	 * che ha un ramo suo — quindi nel Cleanup non c'e' nessun danno imminente da cui fuggire e il modulo
	 * sarebbe inerte: la trappola di `Phase.MistVeil` (`#353`). Lo chiude `#570`, e questo modulo lo segue.
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
	 * Il gadget di **default** per un eroe (`#63`, CP 7.4), o `None` se l'eroe non ne ha uno dichiarato.
	 *
	 * ⚠️ Fino al 2026-08-16 esisteva **solo** `DefaultWeaponVariantFor`: una colonna su quattro della
	 * tabella §4 del catalogo equipaggiamento, e da sola non compone un 1+1+1. Le altre due categorie
	 * vivevano nel solo markdown, che nessun codice leggeva.
	 */
	static FName DefaultGadgetFor(const FName& HeroId);

	/** Il modulo di reazione di **default** per un eroe (`#63`), o `None`. Stessa disciplina del gadget. */
	static FName DefaultReactionModuleFor(const FName& HeroId);

	/**
	 * Il loadout completo di default: variante d'arma + gadget + modulo, **in quest'ordine**, oppure un
	 * array **vuoto** se l'eroe non ha default dichiarati.
	 *
	 * Vuoto e non parziale: un loadout a due pezzi verrebbe rifiutato da `ValidateLoadout` — che pretende
	 * esattamente uno per slot — e il chiamante si troverebbe a gestire un errore che nasce qui. Se un eroe
	 * non ha default, non ne ha **nessuno**, e chi lo equipaggia lo scopre da un array vuoto invece che da
	 * una validazione fallita tre livelli piu' in la'.
	 *
	 * ⚠️ **La fonte autorevole e' §4 di `docs/balance/RT_EquipmentCatalog_v0.1.md`**, non queste tabelle:
	 * il C++ ne e' una copia.
	 *
	 * 🔴 **E dal 2026-08-21 nulla impedisce che divergano.** A confrontarle era
	 * `scripts/check-equipment-defaults.py`, uscito con **D-182** insieme all'intera cartella `scripts/`.
	 * Chi tocca queste righe o §4 del catalogo deve rileggere **l'altra** copia a mano: non c'e' piu' un
	 * comando che lo dica. Il gate generale markdown↔C++ resta `#576`, aperta.
	 */
	static TArray<FName> DefaultLoadoutFor(const FName& HeroId);

	/**
	 * Avvisi (non errori) su una combinazione eroe/variante: cose legali che con ogni probabilita' non si
	 * vogliono, e che nessun validator strutturale puo' vedere perche' dipendono dalla **coppia**.
	 *
	 * Oggi ne produce due, ed entrambe nascono da D-089:
	 * - la variante **duplica uno status** che l'attacco base gia' applica (`Suppressive` su
	 *   `Riktor.ImpactShot`, che rallenta di suo): si paga il costo pieno per un effetto che si ha;
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
	 * Non esiste un redirect di ID ritirati: [D-134] lo ha rimosso. Un ID che il catalogo non conosce
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
	 * possono occupare lo stesso slot, e chi dichiara `MovementAndMain` non lascia spazio a nessuna azione
	 * principale.
	 *
	 * ⚠️ **Nessuna azione dei cataloghi dichiara oggi `MovementAndMain`.** `Action.Sprint` lo faceva fino a
	 * [D-028] e occupa ora il solo movimento, quindi «PrecisionAttack dopo Sprint» e' un piano LEGALE: non e'
	 * cambiata la regola generale, e' cambiato l'esempio. La forma resta esprimibile per un kit che voglia far
	 * costare una mobilita' anche la principale [D-191].
	 */
	static TArray<FString> ValidateActionSlots(const TArray<FRTActionDef>& PlannedActions);

	/**
	 * Quale dei due slot del turno consuma un'azione. `MovementAndMain` risponde **vero a entrambe**, ed e'
	 * l'unico motivo per cui queste sono due funzioni e non un `== Slot` - anche se oggi nessuna azione dei
	 * cataloghi lo dichiara (`Action.Sprint` lo faceva fino a [D-028]).
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
	 * Sono **quattro** delle sette dichiarate, e le altre tre mancano per ragioni diverse che vale la pena
	 * distinguere:
	 *
	 * - `Move` e `BasicAttack` **ci sono gia'**, per altre strade: il movimento passa da `PlannedPath` e non
	 *   da uno slot azione, e l'attacco base e' l'indice 0 del kit di ogni eroe (catalogo v0.1);
	 * - `Interact` non ha un **consumatore**: nessun codice risolve un'interazione. Aggiungerla darebbe a ogni
	 *   unita' un comando che non fa niente — cioe' lo stesso difetto che questa lista corregge.
	 *
	 * `Guard`, `Brace` e `Wait` entrano perche' sono complete dall'altra parte: `Status.Guarded` e
	 * `Status.Braced` hanno gia' quattro consumatori nel `TurnManager` — riduzione del danno e resistenza alle
	 * spinte — che senza un produttore restavano irraggiungibili in partita.
	 *
	 * ➕ `Overwatch` e' entrata con **CP 14.5**, e questo commento diceva il contrario: «non esiste nemmeno nel
	 * catalogo core: e' E14, e arrivera' con la sua infrastruttura». L'infrastruttura era arrivata prima di
	 * lei — CP 14.3 e CP 14.4 hanno consegnato `FRTReactionOpportunity` e `BuildOverwatchTriggers` — e la
	 * riga era diventata la descrizione del difetto invece che di un rinvio: i `FRTOverwatchWatcher` li
	 * costruivano **solo i test**, quindi nessuna partita poteva aprire una finestra. E' lo stesso criterio
	 * con cui `Guard` e `Brace` erano entrate: si aggiunge la generica quando l'altra meta' esiste.
	 *
	 * ⚠️ L'ordine e' un indice stabile: vedi il commento all'implementazione. `Overwatch` va in CODA.
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
