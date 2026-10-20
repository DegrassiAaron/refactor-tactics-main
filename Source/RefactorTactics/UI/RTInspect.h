#pragma once

#include "CoreMinimal.h"
#include "Ability/RTEquipmentData.h"
#include "Map/RTCellId.h"
#include "UI/RTHudViewModel.h"
#include "RTInspect.generated.h"

/**
 * LA SCHEDA DELL'ISPEZIONE: il soggetto pubblico e il compositore delle righe (`#3748`, fetta F3 della spec
 * `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`).
 *
 * 🔑 **Il compositore non vede mai l'attore.** Riceve un soggetto di VALORI, ricavato da una sola funzione che legge
 * l'attore (`ARTHUD::BuildInspectSubject`), e restituisce righe di testo. Del nemico entrano nome, carta, stati, verso,
 * kit dal catalogo ed equipaggiamento (pubblici per decisione d'autore del 2026-10-20); NON entrano il piano, gli slot
 * del turno, l'azione armata, le ricariche vere: il soggetto non ha un campo in cui metterli.
 */

/** Un pezzo di equipaggiamento: nome, slot, vantaggio e svantaggio d'autore. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectEquipmentLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	ERTEquipmentSlot Slot = ERTEquipmentSlot::Gadget;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Advantage;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Drawback;
};

/** Cio' che di un'unita' ispezionata e' pubblico. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectSubject
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FName HeroId;

	/** La carta: la stessa che la targa mostra gia' (PV, scudo). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FRTUnitCardView Card;

	/** Nell'ordine della targa (`BuildStatusBadges`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTStatusBadgeView> Statuses;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Spento in `ResolutionPlayback`, come per lo sguardo (P12): segue la targa. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	bool bShowFacing = false;

	/** Il kit, dal SOLO catalogo: titolo, frase, righe, variante. Mai stato ne' motivo. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTActionTooltipView> Kit;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectEquipmentLine> Equipment;
};

/** Le righe da disegnare. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FString> Lines;
};

UCLASS()
class REFACTORTACTICS_API URTInspectLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Nome, PV e scudo, verso (se acceso), stati col nome e l'effetto del glossario, kit, equipaggiamento. */
	static FRTInspectView Compose(const FRTInspectSubject& Subject);
};
