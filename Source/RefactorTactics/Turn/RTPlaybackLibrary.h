#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnRules.h"
#include "RTPlaybackLibrary.generated.h"

/**
 * Di che cosa e' fatto il tempo di UNA fase del playback: due termini, e la ragione per cui sono due.
 *
 * 🔑 **Il budget di presentazione puo' comprimere `Slack` e non puo' toccare `Shown`.** E' la decisione
 * del product owner del 2026-08-30 (`#1878`) resa esprimibile: *«la durata target della Resolution non
 * deve determinare la velocita' visuale base della locomozione»*. Finche' la fase aveva un numero solo,
 * «comprimere un'attesa» e «accelerare un cilindro» erano la stessa moltiplicazione.
 *
 *  - `Shown` — **incomprimibile**: il tempo che serve a mostrare qualcosa. La locomozione (celle diviso
 *    il rate base) e, nel `Blast`, anche il tempo di lettura dei colpi.
 *  - `Slack` — **comprimibile**: il beat di una fase che non mostra nulla.
 *
 * 🔴 **Il tempo dei colpi sta in `Shown`, e la prima stesura lo metteva in `Slack`.** L'ordine di recupero
 * di `#1878` autorizza a comprimere *«beat non informativi (`PhaseBeatSeconds` su fasi che non mostrano
 * nulla)»* — i colpi mostrano, quindi non sono in quella lista. Classificarli comprimibili produceva due
 * difetti misurati in review:
 *  1. con lo slack a zero la fase `Blast` durava **zero**, e il blocco di finalizzazione scaricava tutti i
 *     colpi rimasti **in un frame** — distruggendo il pavimento *«una fase che si vede non puo' durare
 *     zero»* proprio nella fase che questa issue esiste per rendere leggibile;
 *  2. la spinta del knockback usa `Alpha = Elapsed / PhaseDur`, quindi comprimere quello slack la faceva
 *     scorrere **fino a 6,5x piu' in fretta** — cioe' esattamente *«la durata target determina la
 *     velocita' visuale»*, l'invariante che questa issue nega.
 *
 * ⚠️ **Conseguenza da sapere: oggi il budget ha poco su cui agire** — solo i beat di `Prep` e `Cleanup`.
 * E' poco ed e' onesto: gli altri sei livelli dell'ordine di recupero (idle gap, camera hold, transizioni,
 * code VFX, eventi paralleli) **non esistono ancora nel codice**, e questa struttura non li anticipa. Quando
 * arriveranno, ciascuno decidera' da se' in quale dei due termini cade.
 *
 * ⚠️ **`Total()` vale esattamente quanto valeva `PhaseDuration` prima di questa separazione**, fase per
 * fase: la somma dei due termini non e' una formula nuova, e' la stessa scomposta. Chi cerca il totale
 * continua a chiamare `PhaseDuration`.
 */
USTRUCT(BlueprintType)
struct FRTPhaseTime
{
	GENERATED_BODY()

	/** Tempo che serve a mostrare qualcosa: movimento, e i colpi del Blast. Il budget non lo tocca mai. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	float Shown = 0.f;

	/** Il beat di una fase che non mostra nulla. E' su questo, e solo su questo, che il budget agisce. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	float Slack = 0.f;

	/** La durata della fase senza compressione: e' cio' che `PhaseDuration` restituisce. */
	float Total() const { return Shown + Slack; }
};

/**
 * Matematica pura del playback della risoluzione (posizione-nel-tempo + durata).
 * Non tocca Actor ne' World: e' testabile in automation. L'animazione degli Actor la usa
 * per interpolare i cilindri e stimare/limitare la durata del round (invariante #1: presentazione,
 * non decisione).
 */
UCLASS()
class REFACTORTACTICS_API URTPlaybackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Yaw (gradi) per orientare un attore da From verso To sul piano XY (facing planare, Z ignorata).
	 * Convenzione UE: +X = 0, +Y = 90, -X = +/-180, -Y = -90. Direzione nulla -> 0.
	 *
	 * Vive qui, con il resto della presentazione del turno: lavora su coordinate MONDO e non sa nulla della
	 * topologia della griglia. Stava in `URTGridLibrary` per ragioni storiche ed e' stata spostata con la
	 * rimozione del substrato quadrato (CP 7.2).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float DirectionYaw(const FVector& From, const FVector& To);

	/**
	 * Posizione lungo una polilinea di waypoint a velocita' costante per segmento.
	 * Alpha in [0,1] copre l'intero percorso: 0 = primo waypoint, 1 = ultimo.
	 * Ogni segmento occupa la stessa frazione di Alpha (1/(N-1)), indipendentemente dalla lunghezza
	 * in mondo (una rampa piu' lunga non rallenta: 1 passo logico = 1 segmento).
	 * Vuoto -> ZeroVector; un solo waypoint -> quello.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static FVector InterpolateAlongPath(const TArray<FVector>& Waypoints, float Alpha);

	/**
	 * Quanti colpi devono essere GIA' MOSTRATI dopo PhaseElapsed secondi di fase Blast: il primo esce
	 * subito, e ogni AttackShowSeconds successivi ne compare un altro, fino a NumAttacks.
	 * = Min(NumAttacks, 1 + Floor(PhaseElapsed/AttackShowSeconds)).
	 * AttackShowSeconds <= 0 significa "nessuno scaglionamento": escono tutti insieme.
	 *
	 * E' il contro-termine di `PhaseDuration`, che per il `Blast` riserva gia' il tempo dei colpi: la
	 * durata mostrata a chi guarda e il ritmo con cui i colpi compaiono devono venire dalla stessa
	 * formula, o la barra promette un tempo che la riproduzione non usa (#911).
	 * ⚠️ Poiche' il primo colpo esce a t=0, N colpi occupano N-1 intervalli: l'ultimo compare a
	 * (N-1)*AttackShowSeconds, un beat prima che la durata riservata alla fase finisca. Quel beat
	 * e' il tempo di lettura dell'ultimo colpo, non un residuo da recuperare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static int32 AttacksToShow(int32 NumAttacks, float PhaseElapsed, float AttackShowSeconds);

	/**
	 * Durata (secondi) di UNA fase del playback, prima di qualunque accelerazione.
	 *
	 * `MaxMoveSegments` e' il percorso PIU' LUNGO fra quelli riprodotti in questa fase, non la loro somma:
	 * le unita' si muovono in parallelo, quindi la fase finisce quando finisce l'ultima.
	 *
	 *  - `Dash` / `Move`  → `MaxMoveSegments / CellsPerSecond`. Gli attacchi non entrano.
	 *  - `Blast`          → `Max(colpi, spinta)`, **non** la somma: i colpi e lo scivolamento del knockback
	 *                       occupano la stessa finestra. Il tempo dei colpi ha un pavimento di uno anche
	 *                       quando non ce ne sono, perche' un Blast di sola spinta si vede e deve durare.
	 *  - ogni altra fase  → un beat (`PhaseBeatSeconds`).
	 *
	 * `CellsPerSecond <= 0` significa movimento istantaneo, non una divisione per zero.
	 *
	 * 🔑 **E' l'UNICA formula di durata del playback**, e il totale del round non ne ha una propria:
	 * `ARTTurnManager::BeginPlayback` somma questa su tutte le fasi attive (`RawTotal`).
	 *
	 * ⛔ **Non aggiungerne una aggregata.** Ne e' esistita una — `EstimatePlaybackSeconds`, rimossa il
	 * 2026-08-31 — che sommava movimento, colpi e beat sull'intero round: dava un numero **diverso** da
	 * questo, perche' qui il `Blast` prende `Max(colpi, spinta)` e non la somma. Era coperta da quattro
	 * asserzioni e chiamata da nessuno, cioe' una verita' verde e morta accanto a quella viva. Se serve il
	 * totale, si somma questa.
	 *
	 * ✅ **`PhaseTime` non e' una seconda formula**: questa e' `PhaseTime(...).Total()`, una riga sola. La
	 * scomposizione ha un solo owner, e non esiste modo di farne divergere le due letture.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float PhaseDuration(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
		float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds);

	/**
	 * La formula di durata, nei suoi due termini: quanto della fase e' movimento e quanto e' attesa.
	 * Gli argomenti sono quelli di `PhaseDuration`, e la somma dei termini e' il suo risultato.
	 *
	 *  - `Dash` / `Move`  → tutto `Shown`. Non c'e' nulla da comprimere: la fase dura quanto il percorso
	 *                       piu' lungo impiega, e comprimerla sarebbe accelerare i cilindri.
	 *  - `Blast`          → tutto `Shown`, e vale `Max(colpi, spinta)`: i due si sovrappongono, non si
	 *                       sommano. ⚠️ Zero slack **di proposito** — vedi `FRTPhaseTime`.
	 *  - ogni altra fase  → tutto `Slack`: un beat non mostra nulla, ed e' l'unica attesa comprimibile.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static FRTPhaseTime PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
		float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds);

	/**
	 * Quanto comprimere lo `Slack` del round per stare nel budget di presentazione: `1` se ci si sta gia'
	 * (o se il budget non e' positivo), altrimenti la frazione che serve, mai sotto `0`.
	 *
	 * = `Clamp((MaxSeconds - ShownSeconds) / SlackSeconds, 0, 1)`
	 *
	 * 🔑 **E' un budget SOFT, e il clamp a zero e' il punto in cui lo diventa**: quando la sola locomozione
	 * eccede gia' il budget non resta slack da togliere, e la risposta e' `0` — cioe' *«ho compresso tutto
	 * il comprimibile, e la durata sfora»*. Non e' un fallimento da correggere accelerando: e' la decisione
	 * del PO applicata al caso peggiore (`#1878`).
	 *
	 * ⚠️ **Sostituisce `SpeedMultiplierForCap`, rimossa il 2026-09-02, e la sostituisce per un difetto
	 * misurato**: quella restituiva un fattore `>= 1` che `TickPlayback` moltiplicava dentro `Dt`, cioe'
	 * dentro l'unico orologio che governa anche l'interpolazione del movimento. Il tetto accelerava i
	 * cilindri, ed era esattamente cio' che il PO ha escluso.
	 *
	 * ✅ **Non e' la SOSTITUZIONE scartata da CP 47.2** (`#955`, *«solo Viewer: il tetto smette di valere e
	 * MaxPlaybackSeconds diventa un campo morto»*). Il tetto continua a valere e ha un consumatore vivo:
	 * questa funzione. Cambia su COSA agisce — lo slack invece della velocita' — non SE agisce.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float SlackScaleForBudget(float ShownSeconds, float SlackSeconds, float MaxSeconds);

	/**
	 * La velocita' con cui il playback scorre: quella SCELTA da chi guarda (x1/x2/x4), normalizzata.
	 * Valori non positivi sono trattati come 1 — un campo azzerato (variabile Blueprint, `Memzero`) vale
	 * «non scelto» e non ferma la riproduzione.
	 *
	 * ⚠️ **Aveva un secondo argomento — `CapSpeed` — e non ce l'ha piu' dal 2026-09-02** (`#1878`). Non e'
	 * la SOSTITUZIONE che CP 47.2 (`#955`) aveva scartato: li' l'alternativa uccideva il tetto, qui il
	 * tetto e' vivo e agisce su `SlackScaleForBudget`. Cade il secondo argomento perche' **non ha piu' un
	 * produttore**: nessuno calcola piu' un fattore di velocita' dal tetto.
	 *
	 * Le altre due composizioni di `#955` restano scartate e restano registrate, perche' varrebbero di
	 * nuovo se un secondo fattore di velocita' tornasse:
	 *  - PRODOTTO (Viewer*Cap): un round gia' accelerato 3x, visto a x4, va a 12x — illeggibile proprio nei
	 *    round che piu' avrebbero bisogno di essere letti;
	 *  - TETTO RIDEFINITO (x2 -> Max/2): sotto il tetto non si applicherebbe affatto, quindi su un round da
	 *    4 s premere x2 non farebbe NULLA — ed e' il caso comune.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float EffectivePlaybackSpeed(float ViewerSpeed);
};
