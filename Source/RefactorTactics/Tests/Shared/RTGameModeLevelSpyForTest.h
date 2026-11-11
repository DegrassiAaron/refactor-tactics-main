#pragma once

#include "CoreMinimal.h"
#include "RTGameMode.h"
#include "RTGameModeLevelSpyForTest.generated.h"

/**
 * `ARTGameMode` che REGISTRA l'apertura di un livello invece di eseguirla (CP 46.6, `#941`).
 *
 * ⚠️ **Gemello di `ARTFrontendGameModeForTest`, e per la stessa ragione**: `UGameplayStatics::OpenLevel` in
 * un mondo di prova non porta da nessuna parte, quindi il seam sta esattamente dove l'apertura avviene.
 * Senza, il consumatore di CP 46.6 si potrebbe provare solo in PIE — e il DoD chiede *«nessuno stato
 * vivo»*, che e' una proprieta' del ciclo di vita, non del layout di una schermata.
 *
 * ⛔ **Non sovrascrive nient'altro.** In particolare non tocca `BeginPlay`: e' proprio da li' che deve
 * passare l'iscrizione, ed e' la lezione di `#939` — otto test verdi non videro che il consumatore non era
 * collegato a niente perche' lo collegavano tutti da se'.
 */
UCLASS()
class ARTGameModeLevelSpyForTest : public ARTGameMode
{
	GENERATED_BODY()

public:
	/** I livelli di cui e' stata chiesta l'apertura, in ordine. */
	TArray<FString> OpenedLevels;

	virtual void OpenLevelByName(const FString& LevelName) override
	{
		OpenedLevels.Add(LevelName);
	}
};
