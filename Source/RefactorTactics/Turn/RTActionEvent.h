#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTCellId.h"
#include "RTActionEvent.generated.h"

/**
 * Effetto ELEMENTARE che un'azione puo' produrre. E' l'unita' minima del motore azioni: un'azione del
 * catalogo dichiara quale effetto genera, e il registry sa tradurla in eventi.
 *
 * Enum e non `FName`: gli effetti sono un insieme chiuso e piccolo, il compilatore verifica che ogni caso sia
 * gestito, e la funzione di traduzione resta TOTALE. Il modding (effetti definiti da dati) e' north-star, non
 * v0.1 — vedi docs/design/spec-motore-azioni-e4.md §7, domanda 1.
 */
UENUM(BlueprintType)
enum class ERTActionEffect : uint8
{
	/** Nessun effetto: l'azione occupa lo slot e basta (es. attesa). */
	None,
	/** Danno diretto al bersaglio (scudo prima, poi salute). */
	Damage,
	/** Cura al bersaglio, senza superare la salute massima. */
	Heal,
	/** Scudo TEMPORANEO al bersaglio: protegge il turno e scade nel Cleanup. */
	Shield,
	/** Spinta del bersaglio lungo la direzione del colpo. */
	Push,
	/** Applica uno stato (Root, Slow, Reveal, ...) per una durata in turni. */
	Status
};

/**
 * Un effetto GIA' RISOLTO, pronto da applicare. Le azioni non mutano lo stato: **producono eventi**, che
 * vengono applicati insieme sullo snapshot iniziale della fase (invariante #3, "raccogli poi applica").
 *
 * Solo interi (invariante #4). Riferimento: docs/design/spec-motore-azioni-e4.md §3.
 */
USTRUCT(BlueprintType)
struct FRTActionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionEffect Kind = ERTActionEffect::None;

	/** Chi subisce l'effetto (indice stabile nello snapshot; INDEX_NONE = nessuno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 TargetUnitId = INDEX_NONE;

	/** Chi lo produce: serve alla direzione della spinta e all'osservabilita' del TurnLog. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 SourceUnitId = INDEX_NONE;

	/** Cella coinvolta (spinta, effetti sul terreno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FRTCellId Cell;

	/** Entita' dell'effetto: danni, cura, punti scudo, celle di spinta, turni di durata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 Amount = 0;

	/** Stato applicato (solo con `Kind == Status`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FGameplayTag StatusTag;

	FRTActionEvent() = default;
};
