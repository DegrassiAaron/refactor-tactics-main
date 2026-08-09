#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Map/RTCellId.h"
#include "RTHUD.generated.h"

/**
 * HUD disegnato in C++ (nessun asset UMG): barra HP/scudo sopra ogni unita' viva
 * e, a partita conclusa, il messaggio di esito al centro dello schermo.
 */
UCLASS()
class REFACTORTACTICS_API ARTHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/**
	 * Chi verrebbe colpito dai piani d'attacco **delle proprie unita'**: celle bersagliate e, fra queste,
	 * quelle occupate da un ALLEATO — il fuoco amico.
	 *
	 * Si calcola dai PIANI e non dall'anteprima. La differenza non e' interna: l'anteprima appartiene
	 * all'unita' **selezionata**, quindi l'avviso di fuoco amico spariva appena si selezionava qualcun altro
	 * — per esempio per muoverlo, cioe' mentre si finisce il turno. E' l'opposto di cio' che l'avviso serve a
	 * fare: «un alleato dentro l'area va visto PRIMA del lock-in». Segnalato in PIE il 2026-08-09
	 * (`PIE-PREVIEW-PERSIST`).
	 *
	 * Statica e senza accesso alla selezione **di proposito**: l'indipendenza dalla selezione diventa cosi'
	 * una proprieta' della firma, non una disciplina da ricordare.
	 *
	 * Legge solo i piani di `PlayerTeamId` (invariante #6, privacy dell'intento): i piani avversari non
	 * entrano, nemmeno per dedurne una cella.
	 */
	static void ComputePlannedHitMarks(const TArray<class ARTUnit*>& Units, int32 PlayerTeamId,
		TSet<FRTCellId>& OutHitCells, TSet<FRTCellId>& OutAllyHitCells);

protected:
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarWidth = 64.f;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarHeight = 8.f;

	/** Altezza (uu) sopra l'origine dell'unita' a cui ancorare la barra. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float WorldHeadOffset = 200.f;
};
