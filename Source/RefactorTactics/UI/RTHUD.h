#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
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

protected:
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarWidth = 64.f;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarHeight = 8.f;

	/** Altezza (uu) sopra l'origine dell'unita' a cui ancorare la barra. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float WorldHeadOffset = 200.f;
};
