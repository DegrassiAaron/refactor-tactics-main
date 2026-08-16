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

/** Esito dell'applicazione del danno: HP e scudo risultanti. */
USTRUCT(BlueprintType)
struct FRTDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	FRTDamageResult() = default;
	FRTDamageResult(int32 InHealth, int32 InShield) : Health(InHealth), Shield(InShield) {}
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
	 * `Action.Guard`: riduce di 15 il PRIMO danno diretto ricevuto (catalogo v0.1 §1). Non protegge dagli
	 * hazard ambientali gia' presenti — quelli non passano dai colpi diretti e arrivano con l'epic E8.
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
	 * `Status.Marked` (`Action.MarkTarget`, catalogo v0.1 §3): +6 al PROSSIMO attacco alleato contro il
	 * bersaglio, che consuma il marchio.
	 *
	 * Passa dalla stessa `ApplyFirstHitDelta` di `Exposed` e `Guard` — quindi "consumato una volta sola" e'
	 * una proprieta' del resolver, non una corsa fra gli attaccanti a chi colpisce per primo. Vale solo sui
	 * colpi DIRETTI: il danno ambientale non passa di li' (catalogo: «non aumenta il danno ambientale»).
	 */
	static constexpr int32 MarkedFirstHitBonus = 6;

	/**
	 * `Action.Deflect` (catalogo v0.1 §4): riduce di 20 il danno diretto che ha fatto scattare la reazione.
	 *
	 * Passa da `ApplyFirstHitDelta` come `Guard`: la reazione si attiva UNA volta, quindi vale sul colpo che
	 * l'ha innescata, non su tutti quelli del turno. Se il danno arriva a zero l'attacco resta comunque un
	 * colpo AVVENUTO (il clamp e' sul valore, non sulla voce): conta per trigger e marchi, come dice il catalogo.
	 */
	static constexpr int32 DeflectDamageReduction = 20;

	/**
	 * `Action.Brace` (catalogo v0.1 §4): riduce di 10 OGNI danno diretto fino al Cleanup.
	 *
	 * A differenza di `Guard`/`Deflect` NON passa da `ApplyFirstHitDelta`: "tutti i danni diretti" e' un'altra
	 * regola, e usa `ApplyDamageDelta` (nessun gate "una volta sola"). E' la differenza che rende `Brace`
	 * un'azione diversa da una guardia piu' forte.
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

	/** Applica il danno: lo scudo assorbe per primo, poi gli HP. Nessun valore scende sotto 0. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static FRTDamageResult ApplyDamage(int32 Damage, int32 Shield, int32 Health);

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
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId);

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
