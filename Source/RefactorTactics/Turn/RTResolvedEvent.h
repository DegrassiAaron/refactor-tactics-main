#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "RTResolvedEvent.generated.h"

/** Tipo di evento risolto, per la riproduzione temporizzata (playback) del turno. */
UENUM(BlueprintType)
enum class ERTResolvedEventType : uint8
{
	Move,        // un'unita' ha percorso un path (Path = start + celle attraversate)
	Attack,      // un colpo risolto (Source -> Target, Amount = danno effettivo)
	HazardDamage,// danno da terreno (attraversamento o fine turno)
	Defeated     // rimozione visiva di un'unita' eliminata
};

/**
 * Evento gia' risolto dalla logica, emesso a lock-in per essere RIPRODOTTO nel tempo.
 * L'animazione legge questi eventi: non decide nulla (invariante #1).
 *
 * 🔑 **E' un VALUE TYPE, e i soggetti sono id e non puntatori.** Fino a #1800 i due campi erano
 * `TWeakObjectPtr<ARTUnit>`: il significato del fatto dipendeva dalla vita di un Actor, proprio nel punto
 * in cui la presentazione deve leggere qualcosa di **gia' accaduto**. Un evento che porta due id si
 * confronta, si serializza e si asserisce **senza mondo** — ed e' la stessa scelta gia' fatta da
 * `FRTMoveRoute` nel TurnLog, che porta `StableUnitId` e nessun puntatore.
 *
 * ⚠️ **`0` non e' un'unita'** ([D-063]): `EnsureMatchRoster` assegna gli id **a partire da 1** e lascia lo
 * `0` libero apposta per dire «nessuna unita' dichiarata». Un `Defeated` ha quindi `TargetUnitId == 0`, e
 * un evento nato prima che il roster fosse congelato porta `0` anche in `SourceUnitId`: chi lo consuma
 * deve trattarlo come «nessuno», non come «l'unita' numero zero».
 *
 * 🔴 **Chi anima risolve, chi risolve non anima.** `ARTTurnManager::UnitByStableId` e' la porta che
 * ritrasforma l'id in `ARTUnit*`, e va usata **solo** quando c'e' davvero da muovere un cilindro o da
 * far partire un montage. Se l'unita' e' stata distrutta nel frattempo la porta risponde `nullptr`, che
 * e' esattamente cio' che rispondeva `TWeakObjectPtr::Get()` — il comportamento del playback non cambia,
 * cambia dove sta il puntatore.
 */
USTRUCT(BlueprintType)
struct FRTResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTResolvedEventType Type = ERTResolvedEventType::Move;

	/**
	 * Chi ha agito, come `ARTUnit::StableUnitId`. `0` = nessun soggetto dichiarato.
	 *
	 * ⛔ **`StableUnitId` nel nome, e non `SourceUnitId` soltanto**: nel progetto esiste gia' una famiglia
	 * di campi chiamati `SourceUnitId` — `FRTActionInstance`, `FRTActionEvent`, `FRTNoiseEvent` — e
	 * `RTActionQueue.h` dichiara che li' l'intero e' l'**indice nello snapshot**, con sentinella
	 * `INDEX_NONE`. Sono due identita' diverse con due sentinelle diverse: chiamarle allo stesso modo
	 * significherebbe che un giorno qualcuno assegna l'una all'altra e il compilatore non ha nulla da dire.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 SourceStableUnitId = 0;

	/** Chi ha subito, come `ARTUnit::StableUnitId`. `0` = nessuno. Solo `Attack` lo valorizza. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 TargetStableUnitId = 0;

	/** Per Move: la rotta in celle (start incluso + celle attraversate, in ordine). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TArray<FRTCellId> Path;

	/** Danno/scudo/durata secondo Type. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 Amount = 0;

	FRTResolvedEvent() = default;
};
