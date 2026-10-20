#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Perception/RTKnowledgeView.h"
#include "Player/RTPointerInteraction.h"
#include "UI/RTHudViewModel.h"
#include "RTGlance.generated.h"

/**
 * LO SGUARDO: il soggetto pubblico e il compositore delle righe (`#3747`, fetta F2 della spec
 * `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`).
 *
 * 🔑 **Il compositore non vede mai l'attore.** Riceve due strutture di VALORI gia' pubblici e restituisce righe di
 * testo. Il cancello di conoscenza e' `ResolveGlance`: riceve anche i dati dell'unita' (letti dall'attore) e li scarta se la squadra non la conosce
 * `Live`; `ComposeUnit` e `ComposeCell` non decidono nulla. Il test per reflection impedisce che un tipo grezzo entri nel soggetto.
 *
 * ⚠️ Le righe dello sguardo sono di testo d'autore (`RTGlossary`) piu' dati; nessuna cifra e' scritta qui tranne
 * quelle che il gioco compone dai dati (costo, piano).
 */

/** Un riparo su un bordo: il tipo e il bordo, niente altro. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTGlanceCoverSide
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	ERTHexDirection Edge = ERTHexDirection::E;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	ERTHexCoverType Type = ERTHexCoverType::None;
};

/** Cio' che di un'unita' e' pubblico: nome, stati, verso. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTGlanceUnitSubject
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	FText DisplayName;

	/** Nell'ordine della targa (`BuildStatusBadges`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	TArray<FRTStatusBadgeView> Statuses;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Spento in `ResolutionPlayback` finche' non e' misurato quando `Facing` cambia rispetto alla scena (P12). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	bool bShowFacing = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	FRTCellId Cell;
};

/** Cio' che di una cella in vista e' pubblico. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTGlanceCellSubject
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	/** `FRTHexCellData::TotalMoveCost()`. Si dice solo se diverso da 1. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	int32 EntryCost = 1;

	/** Il piano e' `FRTCellId::Layer`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	int32 Layer = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	TArray<FRTGlanceCoverSide> Covers;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	bool bBlocksLineOfSight = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	bool bIsObjective = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	bool bHasDoor = false;
};

/** Le righe da disegnare. `bTruncated`: l'ultima riga dichiara che c'e' altro. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTGlanceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	TArray<FString> Lines;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Glance")
	bool bTruncated = false;
};

UCLASS()
class REFACTORTACTICS_API URTGlanceLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Massimo di righe di un'unita' (spec: quattro). */
	static constexpr int32 MaxUnitLines = 4;
	/** Massimo di righe di una cella: superficie, costo, piano, ripari, segni. Scelta di F2. */
	static constexpr int32 MaxCellLines = 5;

	/**
	 * Manda a capo le righe a `MaxChars` caratteri, spezzando sugli spazi. Una parola piu' lunga del limite resta
	 * intera (non si perde nulla). E' presentazione: il massimo di righe e' quello del compositore, non di questo.
	 */
	static TArray<FString> WrapLines(const TArray<FString>& Lines, int32 MaxChars);

	/** Nome, verso (se acceso), poi gli stati col nome e l'effetto del glossario; il taglio e' dichiarato. */
	static FRTGlanceView ComposeUnit(const FRTGlanceUnitSubject& Subject);

	/** Superficie, costo d'ingresso (solo se diverso da 1), piano, ripari per lato, vista, obiettivo, porta. */
	static FRTGlanceView ComposeCell(const FRTGlanceCellSubject& Subject);
	/** Vero se la vista ha una voce `Live` per l'unita' del bersaglio: l'unico caso in cui lo sguardo parla dell'unita'. */
	static bool UnitMayBeNamed(const FRTKnowledgeView& View, const FRTGlanceTarget& Target);

	/**
	 * IL CANCELLO DI CONOSCENZA, in una funzione pura (P2, P10).
	 *
	 * `Unit` e `CellAtTarget` sono letti dal chiamante PRIMA di sapere se si possono dire: qui si decide, e cio'
	 * che non si puo' dire non arriva alle righe.
	 *  - un'unita' parla solo con una voce `Live` nella vista (non `bKnownToObserver`, che nasce vero);
	 *  - altrimenti lo sguardo cade sulla cella sotto il cursore, e quella parla solo se e' in `VisibleCells`
	 *    ADESSO (una cella esplorata e non in vista vale una mai vista);
	 *  - fuori vista: nessuna riga, come per una cella vuota. Mai un `nullopt` che sia un canale.
	 */
	static FRTGlanceView ResolveGlance(const FRTKnowledgeView& View, const FRTTeamKnowledge& Knowledge,
		const FRTGlanceTarget& Target, const FRTGlanceUnitSubject& Unit, const FRTGlanceCellSubject& CellAtTarget);
};
