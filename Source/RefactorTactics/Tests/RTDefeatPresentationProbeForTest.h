#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Unit/RTUnit.h"

#include "RTDefeatPresentationProbeForTest.generated.h"

/**
 * Sonda per `ARTTurnManager::OnUnitDefeated` (#2452).
 *
 * ⚠️ **Un `UObject` e non una lambda**: `AddDynamic` richiede una `UFUNCTION`, che una lambda non puo'
 * essere. E' l'unica ragione per cui questo tipo esiste — stessa forma, e stessa motivazione, di
 * `URTAttackPlaybackProbeForTest`.
 *
 * 🔑 **Registra lo stato di visibilita' NELL'ISTANTE dell'annuncio**, che e' l'unica cosa che discrimina il
 * difetto dalla sua correzione. Il conteggio degli annunci, da solo, non lo vedrebbe: era **1** anche con il
 * difetto, perche' l'hide stesso faceva da guardia di idempotenza. Cio' che cambia e' *su che cosa* arriva
 * l'annuncio — un attore visibile o uno gia' nascosto.
 *
 * ⛔ **Non guarda l'animazione, e non potrebbe.** Negli scenari headless nessun `AnimInstance` viene mai
 * istanziato (`ApplyUnitAnimClass()` esce senza fare nulla quando l'unita' non ha una skeletal), quindi
 * un'asserzione sul montaggio sarebbe verde per costruzione. Qui si misura la **precondizione** del
 * montaggio: che al momento in cui parte l'unita' sia ancora disegnata. Il montaggio in se' e' `PIE-AS4b`,
 * e il suo oracolo e' una persona.
 */
UCLASS()
class URTDefeatPresentationProbeForTest : public UObject
{
	GENERATED_BODY()

public:
	/** Quanti annunci di morte sono arrivati, in tutto il playback. */
	int32 Announcements = 0;

	/**
	 * Quanti sono arrivati su un'unita' **gia' nascosta**.
	 *
	 * 🔴 E' l'oracolo del difetto: con `HideForDefeat()` chiamato la riga prima di `PlayDefeatMontage()`
	 * questo valore eguaglia `Announcements`; con la correzione vale **zero**.
	 */
	int32 AnnouncedWhileHidden = 0;

	/** I nomi annunciati, in ordine di arrivo: serve a distinguere «due unita'» da «due volte la stessa». */
	TArray<FString> AnnouncedNames;

	/** Tick di risoluzione in corso, scritto dal test prima di ogni `ARTTurnManager::Tick`. */
	int32 CurrentTick = -1;

	/**
	 * Il tick in cui e' arrivato il PRIMO annuncio, `-1` se non ne e' arrivato nessuno.
	 *
	 * 🔑 Serve a misurare la **coda della morte** (#2452): quanto il playback continua DOPO l'annuncio.
	 * Quando l'eliminazione cade nell'ultima fase riprodotta, senza coda il playback finisce nello stesso
	 * tick e il montaggio `Death` ha finestra zero.
	 */
	int32 FirstAnnouncementTick = -1;

	UFUNCTION()
	void OnUnitDefeated(ARTUnit* Unit)
	{
		++Announcements;
		if (FirstAnnouncementTick < 0)
		{
			FirstAnnouncementTick = CurrentTick;
		}
		if (Unit)
		{
			AnnouncedNames.Add(Unit->GetName());
			if (Unit->IsHidden())
			{
				++AnnouncedWhileHidden;
			}
		}
	}
};
