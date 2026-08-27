#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge, ERTTargetKnowledge
#include "RTKnowledgeView.generated.h"

/**
 * Un soggetto ridotto a cio' che serve per decidere SE SI SA. NON e' un'unita': prendere `ARTUnit`
 * legherebbe una regola pura al mondo di gioco, come `FRTPerceiver` evita di fare per la percezione.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeSubject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	int32 StableUnitId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	int32 TeamId = 0;

	/** Cella ATTUALE. E' informazione autorevole: non deve attraversare la porta per un ignoto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FRTCellId Cell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FName HeroId;

	/** ⚠️ `FText`, non `FName`: e' il tipo che `ARTUnit::HeroDisplayName` ha davvero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	FText HeroDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Knowledge")
	bool bAlive = true;

	FRTKnowledgeSubject() = default;
};

/** Se cio' che si sa e' presente ORA, oppure e' un ricordo. */
UENUM(BlueprintType)
enum class ERTKnowledgeVisibility : uint8
{
	/** La squadra lo vede: cella attuale, rappresentazione normale. */
	Live,
	/** Solo un ricordo: cella dell'ULTIMO CONTATTO, sagoma. Mai la posizione vera. */
	Remembered
};

/**
 * Cosa un osservatore puo' sapere di UN soggetto.
 *
 * ⚠️ Non porta la CONDIZIONE (HP, scudo). La squadra conosce l'identita', non lo stato: e' il confine che
 * il bot rispetta gia' (`CellOnly` -> HP massimi). Un campo qui costringerebbe a inventarne il valore.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	int32 StableUnitId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	ERTKnowledgeVisibility Visibility = ERTKnowledgeVisibility::Live;

	/** Attuale se `Live`, del CONTATTO se `Remembered`. Chi legge non deve sapere quale delle due. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FName HeroId;

	/** ⚠️ `FText`, come su `ARTUnit`. `FText` non ha `IsNone()`: si interroga con `IsEmpty()`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	FText HeroDisplayName;

	FRTKnowledgeEntry() = default;
};

/**
 * Il mondo come un osservatore puo' vederlo.
 *
 * 🔴 Un soggetto IGNOTO non e' una voce con un flag: **non c'e' nessuna voce**. Un flag si puo' leggere per
 * sbaglio; un campo che non esiste no. E' la stessa disciplina di `FRTPlannedIntent -> FilterForTeam ->
 * FRTIntentView`, che nel progetto e' gia' viva e verde.
 */
USTRUCT(BlueprintType)
struct FRTKnowledgeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	int32 ObserverTeamId = 0;

	/** Ordinate per `StableUnitId`: ordine canonico, mai quello di scoperta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Knowledge")
	TArray<FRTKnowledgeEntry> Entries;

	FRTKnowledgeView() = default;
};

/**
 * La porta fra lo stato autorevole e la presentazione (CP 13.5).
 *
 * Pura e headless: nessun Actor, nessun `UWorld`, nessuno snapshot. E' anche la ragione per cui NON prende
 * `FRTHexSnapshot`: `MakeCurrentSnapshot` fa `GetAllActorsOfClass` e due `Sort`, e il disegno gira a ogni
 * frame; inoltre `FRTHexSimUnit` non porta `TeamId`, quindi non basterebbe.
 */
UCLASS()
class REFACTORTACTICS_API URTKnowledgeViewLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Knowledge")
	static FRTKnowledgeView ViewForTeam(const FRTTeamKnowledge& Knowledge,
		const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId);

	/** La voce di un soggetto, o `nullptr` se l'osservatore non ne sa nulla. */
	static const FRTKnowledgeEntry* FindEntry(const FRTKnowledgeView& View, int32 StableUnitId);
};
