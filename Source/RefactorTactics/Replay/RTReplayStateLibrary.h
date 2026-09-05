#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Turn/RTTurnLog.h"
#include "RTReplayStateLibrary.generated.h"

/**
 * Dove sta un'unita' a un dato punto della traccia.
 *
 * ⚠️ **E' uno stato DI PRESENTAZIONE ricostruito, non uno stato di gioco.** Porta le tre cose che un
 * playback graybox deve disegnare — dov'e', da che parte guarda, se e' ancora in piedi — e nient'altro: chi
 * ha bisogno di salute, scudi o status legge la traccia, che li dichiara voce per voce.
 */
USTRUCT(BlueprintType)
struct FRTTracedUnitState
{
	GENERATED_BODY()

	/** Identita' stabile, la stessa che il TurnLog usa in `UnitId` ([D-063]). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 UnitId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTHexDirection Facing = ERTHexDirection::E;

	/**
	 * `false` dopo una voce che la dichiara abbattuta.
	 *
	 * ⚠️ **Non si toglie dall'elenco**, e non e' indecisione: chi disegna deve poter distinguere *«non c'e'
	 * piu'»* da *«non l'ho mai vista»*, e un'unita' sparita dall'array non lo permette. Il KO e' un fatto
	 * osservabile del playback, non un'assenza.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bAlive = true;
};

/**
 * Ricostruisce **dalla traccia** lo stato visibile a un punto della riproduzione (`#1625`).
 *
 * 🔴 **Non riesegue niente, ed e' il guardrail centrale di `#1625`**: *«nessun resolver, targeting, LOS o
 * pathfinding nella UI di playback»*, *«nessun `SetActorLocation` come esito: la posizione viene dalla
 * traccia»*. Questa libreria e' l'unico posto in cui quella promessa diventa una funzione — e una funzione
 * che non include il resolver non puo' chiamarlo, il che e' piu' forte di una regola scritta.
 *
 * 🔑 **Vive nel CORE e non nell'editor**, perche' serve a **entrambi** i consumer del ViewModel.
 * `spec-tactical-designer.md` §3.2 lo prescrive alla lettera: *«una modifica utile a entrambi si fa nel
 * core, con i test del core»*. ⚠️ E la stessa riga dichiara il limite che questo file rispetta: *«cio' che
 * descrive chi puo' vedere cosa non scende mai nel core condiviso»*. Qui non c'e' nessuna politica di
 * visibilita': si ricostruiscono i fatti che la traccia **gia' contiene**, e chi filtra lo fa a monte —
 * il Replay Viewer aprendo una traccia gia' filtrata ([D-316]), lo Scenario Playback non filtrando affatto.
 *
 * ⛔ **Cosa NON ricostruisce, e perche' e' dichiarato invece che dimenticato.** Il §Scope di `#1625` elenca
 * dodici famiglie di eventi; qui ce ne sono **tre** — posizione, facing, KO — e sono quelle senza cui non
 * si vede *niente* muoversi. Le altre nove (linea d'attacco, AoE, danno/scudo, push/pull, status, terreno,
 * strutture, trigger e decisione di reazione) sono **eventi**, non stato: si disegnano leggendo le voci
 * della fase corrente, che il ViewModel gia' consegna. Metterle qui vorrebbe dire ricostruire uno stato per
 * cose che non ne hanno uno.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayStateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Lo stato delle unita' dopo aver applicato **tutte** le voci fino a `(TurnNumber, Phase)` inclusa.
	 *
	 * `Initial` e' lo schieramento di partenza: la traccia dichiara i **cambiamenti**, non le posizioni
	 * iniziali, quindi senza di lui non c'e' niente da muovere. Un `UnitId` che compare nella traccia e non
	 * in `Initial` viene **aggiunto** invece che scartato — un'unita' evocata a meta' partita e' un caso che
	 * il formato non vieta, e scartarla la renderebbe invisibile senza dirlo.
	 *
	 * ⚠️ **L'ordine delle voci e' quello della traccia**, che e' la forma canonica di `SortTurnLog`: si
	 * applica in quell'ordine e non si riordina. Riordinare qui produrrebbe uno stato che la partita non ha
	 * mai attraversato.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	static TArray<FRTTracedUnitState> UnitsAtPosition(const TArray<FRTTurnLogEntry>& Entries,
		const TArray<FRTTracedUnitState>& Initial, int32 TurnNumber, ERTMatchPhase Phase);

	/**
	 * Lo stato delle unita' al **boundary** `(TurnNumber, Phase, MicroStepIndex)` — la stessa terna che
	 * `URTReplaySeekLibrary::SeekToBoundary` indirizza nella traccia (`#2272`).
	 *
	 * 🔑 **Chiude una asimmetria**: il seek indirizzava tre coordinate, la ricostruzione ne accettava due.
	 * Si poteva chiedere *dove* comincia un micro-step, non *com'era il mondo* a quel micro-step — ed e' la
	 * ragione per cui il criterio `playback ≡ seek` di `#1880` era rimasto aperto anche dopo che `#2260`
	 * aveva popolato il campo.
	 *
	 * `MicroStepIndex == INDEX_NONE` significa **la fase intera**, ed e' esattamente cio' che
	 * `UnitsAtPosition` chiede: quella funzione delega qui, e il comportamento storico e' preservato per
	 * costruzione invece che per somiglianza.
	 *
	 * ⚠️ **LO STATO A UN BOUNDARY E' PARZIALE, E LO E' DI PROPOSITO.** Le voci che non appartengono a un
	 * ciclo di micro-step — `INDEX_NONE`, e fra queste **tutte** le `Action.Move` — stanno **dopo** ogni
	 * boundary e **non** entrano in un taglio fine. ∴ a metà movimento le unita' non hanno ancora la loro
	 * cella finale: e' il mondo com'era a quella barriera, non il mondo a fine fase.
	 *
	 * ⛔ **Un consumer che si aspetta posizioni definitive deve chiedere la fase intera.** Leggere un
	 * boundary e disegnarlo come stato finale mostrerebbe unita' ferme dove non sono mai state, ed e' il
	 * motivo per cui questa nota sta qui e non in un commit.
	 *
	 * 🔴 La ragione per cui `INDEX_NONE` sta **dopo** e non prima, malgrado `-1 < 0`: `BuildMoveLog` gira
	 * dopo `FinishHexMovement`, quindi l'arrivo di un'unita' e' posteriore a ogni barriera che ha
	 * attraversato per arrivarci. Il segno del campo e' una **categoria**, non un ordine.
	 *
	 * ⚠️ Un `MicroStepIndex` che la fase non contiene non e' un errore qui: restituisce lo stato ai
	 * boundary che esistono e sono `<=`. Chi vuole sapere se quel boundary **esiste** lo chiede a
	 * `SeekToBoundary`, che risponde `BoundaryNotFound` — le due domande sono diverse e restano separate.
	 */
	static TArray<FRTTracedUnitState> UnitsAtBoundary(const TArray<FRTTurnLogEntry>& Entries,
		const TArray<FRTTracedUnitState>& Initial, int32 TurnNumber, ERTMatchPhase Phase,
		int32 MicroStepIndex);

	/**
	 * Lo stato dopo aver applicato **l'intera** traccia. E' `UnitsAtPosition` all'ultima voce, e serve a chi
	 * vuole il risultato senza dover conoscere l'ultimo turno.
	 *
	 * 🔑 **E' anche il modo in cui si misura che la velocita' non tocca l'esito** (criterio 4 di `#1625`):
	 * `Instant` e `1x` devono arrivare **qui**, allo stesso array.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	static TArray<FRTTracedUnitState> UnitsAtEnd(const TArray<FRTTurnLogEntry>& Entries,
		const TArray<FRTTracedUnitState>& Initial);

	/**
	 * `true` se questa voce sposta, riorienta o abbatte qualcuno — cioe' se cambia lo stato ricostruito.
	 *
	 * ⚠️ Esiste per rendere **misurabile** cio' che il commento sopra dichiara: le nove famiglie non rese
	 * rispondono `false`, e un test puo' verificare che l'elenco dei renderizzati sia quello dichiarato
	 * invece che quello che il codice fa per caso.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	static bool EntryChangesUnitState(const FRTTurnLogEntry& Entry);
};
