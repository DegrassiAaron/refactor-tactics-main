#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
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

	/** Danno/scudo/durata secondo Type. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 Amount = 0;

	FRTResolvedEvent() = default;
};
