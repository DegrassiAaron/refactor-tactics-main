#pragma once

#include "CoreMinimal.h"
#include "Ability/RTEquipmentData.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h"
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

/**
 * Una ricarica DEDOTTA (`#3751`, F6): l'ultimo turno in cui la squadra ha visto usare l'azione, e il turno da cui,
 * a quel conteggio, torna disponibile. E' una STIMA e si legge come tale: un effetto non visto che allunghi o accorci
 * la ricarica la rende sbagliata.
 *
 * ⛔ Quattro campi e nessuno e' la ricarica vera: non c'e' un contatore residuo, ne' un `bUsableNow`. Il test
 * `UI.InspectDeducedRowCarriesNoTrueCooldown` lo pinna per reflection.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectDeducedCooldown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Title;

	/** L'ultimo turno in cui la squadra ha visto usare l'azione. Sempre >= 1: senza avvistamento non c'e' riga. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	int32 LastSeenTurn = 0;

	/** `LastSeenTurn` + durata di catalogo: il turno di pianificazione in cui l'azione, a questo conto, e' di nuovo usabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	int32 AvailableFromTurn = 0;
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

	/**
	 * Le ricariche DEDOTTE di un nemico, nell'ordine del kit (`#3751`). Solo per le azioni che la squadra ha visto
	 * usare e che hanno una ricarica; vuoto per un'unita' propria o alleata, di cui si leggono le ricariche vere.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectDeducedCooldown> DeducedCooldowns;
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
	/**
	 * Nome, PV e scudo, verso (se acceso), stati col nome e l'effetto del glossario, kit, equipaggiamento, e le
	 * ricariche dedotte (dichiarate stima).
	 */
	static FRTInspectView Compose(const FRTInspectSubject& Subject);

	/**
	 * Una riga di ricarica dedotta, o `false` se non c'e' niente da dire (`#3751`).
	 *
	 * Non c'e' riga per un'azione mai vista usare (`LastSeenTurn <= 0`), ne' per una senza ricarica (`CooldownTurns
	 * <= 0`: sarebbe «torna disponibile subito»), ne' senza identita'. La deduzione tace, non indovina.
	 *
	 * 🔑 **`AvailableFromTurn = LastSeenTurn + CooldownTurns`, e non +/-1.** `ARTUnit::ConsumeAbility` scrive
	 * `CooldownTurns` durante la risoluzione del turno T, `TickCooldowns` ne toglie uno nel Cleanup dello stesso T:
	 * a fine turno T restano `CooldownTurns - 1`, e a zero si arriva dopo il Cleanup di T + `CooldownTurns` - 1,
	 * cioe' pianificando il turno T + `CooldownTurns`. Una ricarica di 1 e' «ogni turno» (T + 1). Lo pinna
	 * `UI.InspectDeducedCooldownMatchesTheRealCountdown` contro `ConsumeAbility`/`TickCooldowns` veri.
	 */
	static bool MakeDeducedCooldown(FName ActionId, const FText& Title, int32 CooldownTurns, int32 LastSeenTurn,
		FRTInspectDeducedCooldown& OutRow);

	/**
	 * IL CANCELLO DI CONOSCENZA (#3748, P2): una scheda esiste solo se la vista ha una voce `Live` per l'unita'. Un
	 * nemico non osservato, appena nato, ricordato o ignoto NON ha scheda: stessa uscita di una cella vuota. Non guarda
	 * `ARTUnit::IsKnownToObserver()`, che nasce vero.
	 */
	static bool IsInspectable(const FRTKnowledgeView& View, int32 StableUnitId);

	/** Il cancello e il compositore insieme: vuota se l'unita' non e' `Live`. Il soggetto arriva comunque gia' costruito. */
	static FRTInspectView ResolveInspect(const FRTKnowledgeView& View, int32 StableUnitId, const FRTInspectSubject& Subject);
};
