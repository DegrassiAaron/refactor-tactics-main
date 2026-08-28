#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RTVeilProbeForTest.generated.h"

/**
 * Sonda per `ARTTurnManager::OnTeamKnowledgeRefreshed` ([D-227], `#1467`).
 *
 * ⚠️ **Un `UObject` e non una lambda**: `AddDynamic` richiede una `UFUNCTION`, che una lambda non puo'
 * essere. Stessa forma, e stessa motivazione, di `URTAttackPlaybackProbeForTest`.
 *
 * Registra **quante** volte la conoscenza viene rinfrescata e **a quale turno**. Il conteggio e' il punto:
 * un velo aggiornato a `Tick` darebbe lo stesso risultato visivo e centinaia di emissioni, quindi e' l'unico
 * numero che distingue «segue i punti di refresh» da «segue il tempo reale».
 */
UCLASS()
class URTVeilProbeForTest : public UObject
{
	GENERATED_BODY()

public:
	/** Un elemento per emissione: il turno dichiarato dal TurnManager. */
	TArray<int32> RefreshTurns;

	UFUNCTION()
	void OnKnowledgeRefreshed(int32 TurnNumber)
	{
		RefreshTurns.Add(TurnNumber);
	}
};
