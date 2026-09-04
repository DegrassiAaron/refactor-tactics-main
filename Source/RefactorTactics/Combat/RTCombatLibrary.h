#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnLog.h"
#include "RTCombatLibrary.generated.h"

class URTHexMapAsset;

/**
 * Perche' un bersaglio non e' ingaggiabile. Serve a spiegare l'esito nel log con il motivo GIUSTO: "fuori
 * portata" e "coperto" sono difetti diversi da correggere per chi gioca, e confonderli rende il log una bugia.
 */
UENUM(BlueprintType)
enum class ERTHexTargetReason : uint8
{
	Ok,             // ingaggiabile
	NoMap,          // nessuna mappa autorevole: non si valida (fail-closed)
	OutOfRange,     // oltre la portata dell'abilita'
	NoLineOfSight   // in portata, ma la traiettoria e' bloccata
};

/**
 * Da dove viene il danno ([D-224]). Decide se lo scudo BASE partecipa all'assorbimento: il cuscinetto
 * passivo che ogni unita' porta ferma i colpi, non gli hazard. Lo scudo TEMPORANEO assorbe entrambi —
 * quello e' protezione che qualcuno ha speso un'azione per costruire.
 *
 * Non e' `DamageType` (`Kinetic`/`Fire`/...), che risponde a un'altra domanda — quale resistenza leggere.
 * Qui la domanda e' una sola e binaria: la base partecipa?
 *
 * ⚠️ **`DamageType` non ha un'epic, e il numero che questo commento citava era di un altro.**
 * Diceva *«arriva con E49»*, ma `E49` e' l'epic della **Tactical Camera** (`#1769`, 2026-08-30): chi
 * seguiva il puntatore per capire dove nascera' `DamageType` atterrava sulla camera, e nulla lo
 * avvertiva. Un link a vuoto si nota, un link a un'altra cosa no. L'owner reale e' **`D-238`**, che
 * decide di non congelare la formula e dichiara che *«E49 resta una proposta e nessuna issue e' stata
 * creata»*; il materiale di studio sta in `docs/research/prd/prd-damage-model-armor-shield.md`, che
 * e' livello 8 e **non e' normativo**. Corretto da `#1898`.
 */
UENUM(BlueprintType)
enum class ERTDamageSource : uint8
{
	Direct,         // colpi: Blast, decision boundary, Overwatch
	Environmental   // Burning, danno da terreno, propagazione elettrica
};

/** Esito dell'applicazione del danno: HP, scudo e quota temporanea risultanti. */
USTRUCT(BlueprintType)
struct FRTDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	/** Quota di `Shield` che scade nel Cleanup: il resto e' scudo base dell'unita'. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 TemporaryShield = 0;

	FRTDamageResult() = default;
	FRTDamageResult(int32 InHealth, int32 InShield, int32 InTemporaryShield = 0)
		: Health(InHealth), Shield(InShield), TemporaryShield(InTemporaryShield) {}
};

/** Calcoli puri di combattimento (indipendenti dagli Actor, testabili). */
UCLASS()
class REFACTORTACTICS_API URTCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * `Status.Exposed`: +5 al PRIMO danno diretto ricevuto (catalogo v0.1 §2, `Action.Sprint`).
	 *
	 * Costante nominata e non numero sparso nel resolver: e' un valore di catalogo, e quando gli stati
	 * diventeranno dati (CP 8.2) sara' un campo dell'asset, non una modifica in tre punti.
	 */
	static constexpr int32 ExposedFirstHitBonus = 5;

	/**
	 * `Action.Guard`: **POOL** di 15 danni assorbibili, che i colpi dell'arco FRONTALE consumano finche'
	 * dura ([D-292] + [D-206]). Non protegge dagli hazard ambientali gia' presenti — quelli non passano dai
	 * colpi diretti e arrivano con l'epic E8.
	 *
	 * 🔴 **Questo commento diceva «riduce di 15 il PRIMO danno diretto ricevuto» fino al 2026-09-03, e
	 * D-292 l'aveva superato il 2026-08-31.** La differenza non e' di parole: col vecchio delta la riduzione
	 * che avanzava si PERDEVA, e quanta se ne perdesse dipendeva da quale colpo fosse arrivato prima — un
	 * bersaglio colpito da 10 e da 30 incassava 30 o 25 a seconda dell'indice dell'attaccante. Il pool
	 * consuma sempre lo stesso totale, quindi l'esito e' invariante per permutazione **per costruzione**.
	 *
	 * ⚠️ **Il NOME resta `GuardFirstHitReduction`, e non e' una svista.** Rinominarlo tocca i chiamanti ed
	 * e' un refactor, non una correzione di prosa: finche' il nome vive, questo commento e' l'unico posto
	 * che dice cosa il valore fa davvero. Chi lo rinomina porti via anche questo paragrafo.
	 *
	 * Il valore lo consuma `URTCombatResolver::ApplyAbsorptionPool`, **non** `ApplyFirstHitDelta` — che
	 * resta la strada di `Status.Exposed` e `Status.Marked`. ⚠️ *Questa riga diceva «`Status.Exposed` e
	 * `Action.Deflect`», ed era vera quando fu scritta: [D-309] ha reso un pool anche il `Deflect` il giorno
	 * dopo. E' il modo in cui una deriva si allarga — correggendo meta' di una regola.* Esercitato dal corpus con
	 * `Spec.Combat.GuardPoolSpansMultipleHits`, che usa colpi PIU' PICCOLI del pool: sopra i 15 le due
	 * regole danno lo stesso numero, ed e' la ragione per cui il corpus non si accorse del cambio (`#1919`).
	 */
	static constexpr int32 GuardFirstHitReduction = 15;

	/**
	 * `Status.Burning`: 8 danni nel Cleanup, per la durata dello stato (catalogo terreni §2, CP 8.2).
	 *
	 * Non e' un danno diretto: `Guard`, `Brace` e `Deflect` non lo riducono (i loro delta si applicano ai
	 * colpi del Blast), e il catalogo lo conferma escludendo `Counter` dal danno ambientale.
	 */
	static constexpr int32 BurningCleanupDamage = 8;

	/**
	 * Scudo BASE di ogni unita' ([D-224]): 5 punti che non crescono e tornano pieni a fine turno.
	 *
	 * Sta qui e non in `Config/` perche' non e' un parametro di formato come `RoundLimit`: e' una regola del
	 * combattimento, e le altre nove vivono in questa stessa lista.
	 *
	 * ⚠️ Ferma solo il danno `Direct`. La ragione e' misurata: a 5 punti indistinti un contrattacco da 10
	 * perderebbe meta' del suo peso e `BurningCleanupDamage` due terzi, e le fonti di danno piccole
	 * diventerebbero ornamentali.
	 */
	static constexpr int32 BaseShield = 5;

	/**
	 * Danno che la propagazione elettrica porta OLTRE la cella colpita (catalogo terreni §2, CP 8.3): 12,
	 * contro i 20 del colpo diretto dichiarati da `Action.Electrify`.
	 *
	 * Sta qui e non negli `Effects` dell'azione perche' non e' un effetto dell'azione su un bersaglio — quelli
	 * hanno un bersaglio scelto in pianificazione — ma il valore che l'AMBIENTE trasporta a chi la scarica
	 * raggiunge senza che nessuno l'abbia mirato. Stessa natura del -20 di Deflect prima di CP 5.5 e del danno
	 * di `Burning`: un numero del calcolo, non un effetto dichiarato.
	 *
	 * Come `Burning`, non e' danno diretto: `Guard`/`Brace`/`Deflect` non lo riducono e `Counter` non scatta
	 * (il catalogo esclude esplicitamente il danno ambientale dai trigger di reazione).
	 */
	static constexpr int32 PropagatedElectricDamage = 12;

	/** `Action.Guard`: spinta massima (in celle) a cui si resiste restando fermi. */
	static constexpr int32 GuardResistedPushDistance = 1;

	/**
	 * Le due PROVENIENZE dei pool d'assorbimento, nel vocabolario di `FRTDamageStageEntry::SourceId`
	 * (`#2213`). Stanno qui e non come letterali ai chiamanti per la stessa ragione degli altri valori di
	 * questa lista: un letterale ripetuto e' un refuso che compila.
	 *
	 * ⛔ **`ReactionReductionPoolSource` NON nomina un `ActionId`, ed e' deliberato.** Il pool si costruisce
	 * da `FRTReactionPassResult::DeflectDelta`, che il dispatcher riempie per QUALUNQUE reazione dichiari
	 * `ERTActionEffect::DamageReduction` — *«Qui non si guarda mai l'`ActionId`: e' cio' che permette a una
	 * reazione d'eroe di riusare la semantica di `Action.Deflect` con numeri propri»* (`RTTurnManager.cpp`).
	 * Etichettarlo `Action.Deflect` attribuirebbe a `Hero.Wraith.Deflection` un'azione che l'unita' non ha
	 * usato: lo stesso difetto che `#2213` corregge, un livello piu' sotto. Trovato da una code review.
	 *
	 * ⚠️ La Guardia invece un tag ce l'ha, ed e' esatto: il suo pool e' gated su `TAG_Status_Guarded`.
	 */
	static const FName GuardPoolSource;
	static const FName ReactionReductionPoolSource;

	/**
	 * `Status.Marked` (`Action.MarkTarget`, catalogo v0.1 §3): +6 al PROSSIMO attacco alleato contro il
	 * bersaglio, che consuma il marchio.
	 *
	 * Passa dalla stessa `ApplyFirstHitDelta` di `Exposed` e `Guard` — quindi "consumato una volta sola" e'
	 * una proprieta' del resolver, non una corsa fra gli attaccanti a chi colpisce per primo. Vale solo sui
	 * colpi DIRETTI: il danno ambientale non passa di li' (catalogo: «non aumenta il danno ambientale»).
	 */
	static constexpr int32 MarkedFirstHitBonus = 6;

	/**
	 * `Action.Deflect` (catalogo v0.1 §4): apre un POOL di 20 danni assorbibili sui colpi diretti del
	 * boundary che ha fatto scattare la reazione.
	 *
	 * Passa da `ApplyAbsorptionPool` come la `Guard` ([D-309], che estende al `Deflect` la forma che
	 * [D-292] aveva dato alla Guardia): cio' che un colpo non consuma **resta** per i successivi, quindi il
	 * totale assorbito non dipende da quale colpo arriva per primo. ⚠️ La REAZIONE si attiva una volta sola
	 * — e' quello che la distingue dalla `Guard`, che e' uno stato — ma cio' che l'attivazione produce e' un
	 * budget per l'intero boundary, non uno sconto sul colpo innescante. ⛔ **Mai attraverso boundary diversi**:
	 * aggregare colpi di boundary differenti distruggerebbe la simultaneita' che il resolver garantisce.
	 * Se il danno arriva a zero l'attacco resta comunque un colpo AVVENUTO (il clamp e' sul valore, non sulla
	 * voce): conta per trigger e marchi, come dice il catalogo.
	 *
	 * ⚠️ Quando due pool coprono lo stesso colpo, `Deflect` assorbe PRIMA di `Guard` — [D-312], e non e' un
	 * dettaglio d'implementazione: su 2940 configurazioni raggiungibili 558 danno un esito diverso.
	 */
	static constexpr int32 DeflectDamageReduction = 20;

	/**
	 * `Action.Brace` (catalogo v0.1 §4): riduce di 10 OGNI danno diretto fino al Cleanup.
	 *
	 * A differenza di `Guard`/`Deflect` NON e' un POOL: quelli hanno un budget che si esaurisce
	 * ([D-292] e [D-309]), questo e' un delta su OGNI colpo che non si consuma mai — `ApplyDamageDelta`,
	 * nessun gate "una volta sola". E' la differenza che rende `Brace` un'azione diversa da una guardia
	 * piu' forte: contro molti colpi piccoli la `Brace` non finisce, un pool si'.
	 * ⚠️ *Questa riga diceva «NON passa da `ApplyFirstHitDelta`», il che implicava che `Guard` e `Deflect`
	 * ci passassero: non e' piu' vero per nessuno dei due. L'argomento — `Brace` vale su tutti i colpi —
	 * regge lo stesso, ma il termine di paragone e' cambiato.*
	 */
	static constexpr int32 BraceDamageReduction = 10;

	/**
	 * Copertura bassa (catalogo v0.1, CP 9.1): riduce di 10 il danno diretto che ATTRAVERSA il bordo riparato.
	 *
	 * Sta qui accanto a `Guard`/`Deflect`/`Brace` perche' e' la stessa famiglia di numeri — quanto danno
	 * diretto si toglie — anche se la condizione che la attiva e' geometrica invece che di stato. Come `Brace`
	 * vale su OGNI colpo che arriva da quel lato (non si consuma): non passa da `ApplyFirstHitDelta`.
	 *
	 * NON riduce il danno ambientale (`Burning`, propagazione elettrica): quelli non attraversano un bordo,
	 * nascono nella cella. E non riduce le AREE: un'esplosione non e' un proiettile che si possa intercettare
	 * con un muretto — vale anche quando il centro sta dal lato riparato.
	 */
	static constexpr int32 LowCoverDamageReduction = 10;

	/**
	 * `Gadget.LinearDischarge` (catalogo eroi v0.1 §1): +8 danni contro un bersaglio `Status.Wet`.
	 *
	 * A differenza di `Exposed`/`Guard`/`Marked`, NON passa da `ApplyFirstHitDelta`: il bonus non si consuma
	 * al primo colpo, vale per OGNI colpo finche' `Wet` e' attivo (come `Root`/`Slow`) — e riusa
	 * `EffectiveAttackPower` (bonus di cella), non un meccanismo nuovo. E' specifico di UN'abilita', non una
	 * regola di combattimento universale: per questo il nome non e' generico come gli altri.
	 */
	static constexpr int32 GadgetWetDischargeBonus = 8;

	/**
	 * Applica il danno: erode il temporaneo, poi — solo se la sorgente e' `Direct` — lo scudo base, poi gli
	 * HP. Nessun valore scende sotto 0.
	 *
	 * `TemporaryShield` non e' ridondante con `Shield`: e' la QUOTA di `Shield` che scade nel Cleanup, e
	 * senza di essa la funzione non puo' sapere quanta protezione e' base — cioe' quanta ne deve saltare
	 * quando il danno viene dall'ambiente.
	 *
	 * ⚠️ Nessun overload con la firma vecchia, di proposito: lascerebbe un chiamante a saltare la regola
	 * della sorgente senza accorgersene, e un errore di compilazione e' preferibile a un danno che ignora
	 * lo scudo base in silenzio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static FRTDamageResult ApplyDamage(int32 Damage, ERTDamageSource Source, int32 Shield,
		int32 TemporaryShield, int32 Health);

	/** Accumula energia con clamp in [0, Max]. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 GainEnergy(int32 Current, int32 Gain, int32 Max);

	/** Vero se l'ultimate e' disponibile (energia al massimo). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsUltimateReady(int32 Energy, int32 Max);

	/**
	 * Range di movimento effettivo con lo status Root: azzera. `Slow` NON passa piu' da qui (CP 4.7): il
	 * catalogo v0.1 §5 lo definisce come **+1 al costo di ogni cella**, un meccanismo di pathfinding
	 * (`FRTHexSimUnit::MoveCostModifier`, `URTHexPathLibrary::FindPathAvoiding`), non una riduzione flat del
	 * raggio. Dimezzare qui era la scelta pre-CP4.2 (prima del budget a costi per cella): con Slow tolto,
	 * questa funzione fa esattamente cio' che il suo nome dice, senza un secondo bool che mente sulla firma.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 EffectiveMoveRange(int32 BaseRange, bool bRooted);

	/** Vero se un'abilita' e' utilizzabile: non in ricarica e con energia sufficiente. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsAbilityUsable(int32 CooldownRemaining, int32 Energy, int32 EnergyCost);

	/**
	 * Invariante #6 (privacy dell'intento): il piano di un'unita' e' visibile agli alleati
	 * (stessa squadra) sempre, agli avversari solo se l'unita' e' "rivelata" (status Reveal).
	 *
	 * CONSUMATORE: `URTIntentPrivacyLibrary::FilterForTeam`, che costruisce il DTO spedito al client — cioe'
	 * il percorso che la HUD attraversa davvero. Fino a #507 la regola era riscritta li' inline e questa
	 * funzione non aveva chiamanti: il suo test era verde e non copriva niente di vivo. Se un giorno tornasse
	 * senza consumatori, la risposta e' rimuoverla, non lasciarla a fare da falsa copertura.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsIntentVisibleTo(int32 ObserverTeamId, int32 OwnerTeamId, bool bOwnerRevealed);

	/**
	 * AUTORITA' sull'unita': un giocatore comanda solo le unita' della PROPRIA squadra — quindi puo' selezionarle
	 * e pianificarne le azioni. Regola esplicita e testabile invece che implicita nel controller: senza, un click
	 * su un'unita' avversaria la rende "selezionata" e da li' si finisce a pianificare i turni del nemico
	 * (oltre a bloccare la selezione delle proprie, vedi OnSelect). Parente dell'invariante #6: quel che non e'
	 * tuo non lo vedi e non lo comandi.
	 *
	 * ⚠️ **Due condizioni, non una, e la seconda e' arrivata con l'alleato bot.** La squadra dice di CHI e'
	 * l'unita'; `bUnitIsBotControlled` dice se qualcun altro la sta gia' pianificando. Finche' il bot ha
	 * posseduto solo la squadra 1 le due domande avevano la stessa risposta e bastava la prima — ma con un
	 * compagno pianificato dal bot (`rt.Match.BotAllies`) la squadra 0 contiene entrambi i casi, e un
	 * predicato scritto sul solo `TeamId` lascerebbe il giocatore selezionare Phase e scriverle un piano
	 * che `PlanBots()` sovrascrive a inizio turno senza dirlo. E' lo stesso difetto di coerenza gia'
	 * misurato per l'autobattle (#971), un livello piu' in basso: li' l'input era inerte per la sessione,
	 * qui lo e' per la singola unita'.
	 *
	 * ⛔ **Non e' il gate della modalita' non presidiata.** Quella la decide `IsPlanningInputInert()` sul
	 * GameMode, per le ragioni scritte alla sua dichiarazione. Questo predicato risponde a una domanda piu'
	 * stretta — «questa unita' e' comandabile?» — e i due insiemi non coincidono.
	 *
	 * @param bUnitIsBotControlled `ARTUnit::bIsBotControlled` dell'unita' in esame. Il default `false`
	 *        conserva il comportamento storico per i chiamanti che non conoscono l'unita' (i test di
	 *        `RTCombatLibraryTests` interrogano la sola regola di squadra).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId, bool bUnitIsBotControlled = false);

	/**
	 * IL GRUPPO DI CONTROLLO di un'unita': quale persona della squadra la comanda — `CP 19.3`, `#1124`.
	 *
	 * `IndexInTeam / UnitsPerPlayer`, e non c'e' altro: con `UnitsPerPlayer = 2` su una squadra da due, le
	 * unita' `0` e `1` stanno **entrambe** nel gruppo `0` — una sola persona comanda la squadra intera, che
	 * e' il caso della v0.1. Con `UnitsPerPlayer = 1` finiscono in gruppi diversi, ed e' il modello
	 * `1 player = 1 character` che il formato dichiara come default.
	 *
	 * ⛔ **Fail-closed su `UnitsPerPlayer <= 0`**: restituisce `INDEX_NONE`, non `0`. Il default del campo e'
	 * `0` — un formato che non dichiara quante unita' comanda una persona non deve produrre un gruppo
	 * *valido* per inerzia, ed e' la stessa scelta che `ARTGameMode::AssignSeats` fa un livello sopra
	 * rifiutando di dividere per zero. Un `INDEX_NONE` non coincide con nessun gruppo di giocatore, quindi
	 * nessuno comanda nulla: si perde il controllo, non lo si regala.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 ControlGroupForUnit(int32 IndexInTeam, int32 UnitsPerPlayer);

	/**
	 * LA REGOLA DI CONTROLLO con i gruppi: stessa squadra, **stesso gruppo**, e non pianificata dal bot.
	 *
	 * 🔑 **Con i valori della v0.1 risponde come `CanPlayerControlUnit`**, ed e' voluto: `Format.Skirmish2v2`
	 * dichiara `UnitsPerPlayer = 2` su `UnitsPerTeam = 2`, quindi un posto per squadra e un gruppo solo. La
	 * regola nuova non cambia nessuna partita di oggi — cambia cio' che diventa esprimibile domani, quando
	 * due persone siedono nella stessa squadra e ciascuna comanda le proprie unita'.
	 *
	 * ⚠️ **Non sostituisce `CanPlayerControlUnit`, che resta** la regola di SQUADRA e ha ancora i propri
	 * chiamanti: qui si aggiunge una condizione, non se ne riscrive una.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanPlayerControlUnitInGroup(int32 UnitTeamId, int32 UnitControlGroup, int32 PlayerTeamId,
		int32 PlayerControlGroup, bool bUnitIsBotControlled = false);

	/**
	 * Vero se il bersaglio e' ingaggiabile su griglia esagonale: entro `RangeCells` (distanza esagonale) e con
	 * linea di tiro libera sulla mappa.
	 *
	 * **FAIL-CLOSED**: `Map == nullptr` -> falso. Senza mappa autorevole non si valida la linea di tiro, quindi
	 * non si pianifica l'attacco. La versione precedente nel controller faceva l'opposto (`!Grid || HasLOS`) e
	 * lasciava passare ogni bersaglio quando la griglia non c'era.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanTargetHexCell(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 RangeCells);

	/**
	 * Come `CanTargetHexCell`, ma dice **perche'**: portata prima, poi linea di tiro. Il chiamante logga il
	 * motivo esatto invece di attribuire ogni rifiuto alla copertura.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static ERTHexTargetReason ClassifyHexTargeting(const URTHexMapAsset* Map, const FRTCellId& From,
		const FRTCellId& To, int32 RangeCells);

	/**
	 * Danno effettivo di un attacco dato il bonus della cella occupata dall'attaccante
	 * (es. Altura +danno). Risultato con clamp >= 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 EffectiveAttackPower(int32 BasePower, int32 OccupantDamageBonus);

	/**
	 * Indici delle unita' "appena eliminate": vive (HP > 0) nello stato PRIMA e morte (HP <= 0) nello
	 * stato DOPO. Serve al playback per sapere CHI muore ora (e generare l'evento Defeated) senza
	 * ri-notificare chi era gia' morto. Confronto per indice, fino al minimo delle due lunghezze.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<int32> NewlyDefeated(const TArray<int32>& HealthBefore, const TArray<int32>& HealthAfter);

	/**
	 * Classifica l'esito di un colpo a segno secondo la priorita' Lethal > ShieldAbsorbed > TerrainBonus > Hit.
	 * ShieldAbsorbed = HP invariati (assorbito dallo scudo). TerrainBonus = HP calati con bonus altura > 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static ERTCombatOutcome ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus);
};
