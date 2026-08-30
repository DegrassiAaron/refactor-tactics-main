#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Perception/RTTeamKnowledge.h" // FRTKnowledgeVerdict: il verdetto congelato di [D-223], come in `RTTurnLog.h`
#include "Turn/RTTurnRules.h"
#include "RTResolvedEvent.generated.h"

class ARTUnit;

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
 * L'animazione legge questi eventi: non decide nulla (invariante #1). I riferimenti alle unita'
 * sono weak perche' un'unita' puo' essere logicamente morta mentre il playback la mostra ancora.
 */
USTRUCT(BlueprintType)
struct FRTResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTResolvedEventType Type = ERTResolvedEventType::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TWeakObjectPtr<ARTUnit> Source;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TWeakObjectPtr<ARTUnit> Target;

	/** Per Move: la rotta in celle (start incluso + celle attraversate, in ordine). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TArray<FRTCellId> Path;

	/**
	 * Chi puo' vedere il modello percorrere CIASCUNA cella di `Path` ([D-223], `#1525`). Parallelo a
	 * `Path`, stesso indice.
	 *
	 * 🔴 **Esiste perche' il playback e' la seconda meta' di un difetto di cui la prima e' gia' chiusa.**
	 * `FRTMoveRoute::CellVerdicts` nomina i due errori speculari che [D-223] esiste per chiudere: il
	 * **leak** (la polilinea entra nella nebbia) e la **contraddizione** (la traccia nascosta mentre il
	 * modello e' disegnato). `#1497` ha chiuso il primo troncando la traccia; il secondo restava vivo
	 * perche' questo evento portava la rotta **senza** il verdetto, e il modello la percorreva intera.
	 *
	 * ⚠️ **Non e' un secondo calcolo.** Il verdetto e' lo STESSO che `FreezeRouteVerdicts` congela per la
	 * traccia, due righe sopra il punto in cui questo evento viene costruito, sulla stessa `Route`: la
	 * riparazione e' stata copiarlo, non ricalcolarlo. Se un giorno divergessero, traccia e modello
	 * tornerebbero a raccontare frasi diverse sullo stesso movimento.
	 *
	 * ⚠️ **Vuoto significa «nessun verdetto», e si legge fail-closed.** Chi consuma passa da
	 * `URTTeamKnowledgeLibrary::ObservedPrefixLength`, che su lunghezze disallineate risponde `0` invece
	 * di indovinare — mai indicizzando questo array direttamente.
	 *
	 * ⚠️ **`UPROPERTY()` nudo, non `BlueprintReadOnly`**: `FRTKnowledgeVerdict` non e' un tipo Blueprint, ed
	 * e' la stessa forma che `FRTMoveRoute::CellVerdicts` usa per lo stesso dato. Un verdetto leggibile da
	 * Blueprint sarebbe anche un verdetto **aggirabile** da Blueprint.
	 */
	UPROPERTY()
	TArray<FRTKnowledgeVerdict> CellVerdicts;

	/** Danno/scudo/durata secondo Type. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 Amount = 0;

	FRTResolvedEvent() = default;
};
