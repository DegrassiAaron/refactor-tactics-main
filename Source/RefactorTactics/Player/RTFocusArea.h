#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTFocusArea.generated.h"

class ARTUnit;
class URTHexMapAsset;

/**
 * IL SOGGETTO A FUOCO, COME VALORI (`#3750`, fetta F5; [D-552]).
 *
 * Quattro valori e nessun attore: e' il confine che tiene l'area fuori dal piano privato del nemico. Il calcolo che li
 * consuma non vede ne' `ARTUnit` ne' `PlannedWaypoints`, e il solo punto che legge l'attore e' `SubjectFor`.
 */
struct REFACTORTACTICS_API FRTFocusMoveSubject
{
	/** La cella in cui la squadra vede l'unita' adesso. */
	FRTCellId Cell;

	/** Il movimento BASE dell'unita' (`ARTUnit::GetEffectiveMoveRange`): le due bande ne sono il `1x` e il `2x`. */
	int32 BaseMoveRange = 0;

	/** `+1` al costo di ogni cella se l'unita' e' `Status.Slow`: uno stato pubblico, che `GetEffectiveMoveRange` non legge. */
	int32 MoveCostModifier = 0;

	/** Vero se uno stato pubblico nega la corsa (oggi `Status.Unbalanced`, D-319): resta la sola area normale. */
	bool bRunDenied = false;
};

/**
 * Le due aree di un nemico osservato. Entrambe sono INSIEMI DI CELLE ORDINATI in modo stabile (`StableLess`), e
 * `Normal` e' contenuta in `Run`: chi disegna sceglie se mostrare la corsa per intero o al netto della normale.
 */
struct REFACTORTACTICS_API FRTFocusMoveAreas
{
	/** Dove arriva col movimento normale: `1x` il movimento base (profilo `Move`, D-412). */
	TArray<FRTCellId> Normal;

	/** Dove arriva con la corsa: `2x` (profilo `Sprint`). Vuota se la corsa e' negata o se non c'e' nulla da mostrare. */
	TArray<FRTCellId> Run;

	bool bRunDenied = false;
};

/**
 * L'AREA DI MOVIMENTO DI CHI E' A FUOCO: IL POTENZIALE DI UN NEMICO OSSERVATO, MAI IL SUO PIANO (D-552).
 *
 * Tre divieti, e ciascuno ha il suo test a mondi gemelli (`RTFocusAreaTests.cpp`):
 *
 * 🔴 **Mai il piano.** Il budget e' il movimento base alle due bande fisse. NON la banda che il piano produce
 * (`ProfileForPlan`, da cui `ARTTurnManager::MakeSimUnit` ricava il budget dello snapshot): un'area che ne dipendesse
 * sarebbe un canale sul piano privato. Per la stessa ragione NON si passa da `PlanningSnapshotFor`, che e' comodo
 * (ha gia' budget e tetto) e legge il profilo dichiarato e la riserva del piano.
 *
 * 🔴 **Solo cio' che la squadra vede ADESSO, e le celle non in vista sono MURI NEL CALCOLO.** Ritagliare il risultato
 * dopo non basta: una cella in vista raggiungibile solo passando per celle non in vista resterebbe nell'area e lo
 * direbbe. Per questo il calcolo gira su una copia della mappa in cui ogni cella fuori da `VisibleCells` blocca il
 * movimento (`MakeVisibleOnlyMap`).
 *
 * 🔴 **Non `ExploredCells`.** Una cella esplorata e non piu' in vista non si distingue da una mai vista (D-225,
 * D-227): lo stato che aveva quando fu vista non e' conservato, e quello di adesso non si puo' leggere. E' il contrario
 * di `URTHexSimLibrary::MakeOptimisticPlanningMap`, che rende LIBERE le celle non note e unisce visibili ed esplorate:
 * quella e' la mappa di chi pianifica sulla propria conoscenza, questa e' la mappa di chi guarda un altro.
 *
 * Conseguenza dichiarata: l'area si ferma al bordo di cio' che si vede e SOTTOSTIMA invece di rivelare.
 *
 * Non dipende da unita' ignote: lo snapshot costruito qui contiene la sola unita' del soggetto, e l'occupazione non
 * entra nel ventaglio ([D-445], [D-446]).
 *
 * ⛔ Presentazione pura: nessuno stato canonico, niente hash, TurnLog o replay. Non cablata (fetta C).
 */
UCLASS()
class REFACTORTACTICS_API URTFocusAreaLibrary : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * L'UNICO punto che legge l'attore: cella, movimento base, `Slow`, `Unbalanced`. Quattro letture e nessuna dal piano
	 * (`PlannedWaypoints`, `PlannedAbilityIndex`, `PlannedMovementProfileId`...): un test lo pinna con due unita'
	 * gemelle dal piano diverso.
	 */
	static FRTFocusMoveSubject SubjectFor(const ARTUnit* Unit);

	/**
	 * La mappa di chi guarda un altro: una copia in cui ogni cella fuori da `Knowledge.VisibleCells` blocca il movimento.
	 * Solo `VisibleCells`, mai l'unione con `ExploredCells`. Il chiamante tiene viva la copia (e, in partita, la mette in
	 * cache: `DuplicateObject` non va a ogni fotogramma).
	 */
	static URTHexMapAsset* MakeVisibleOnlyMap(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge);

	/** Il calcolo su una mappa gia' ristretta. Funzione pura di mappa e soggetto. */
	static FRTFocusMoveAreas MoveAreas(const URTHexMapAsset* VisibleOnlyMap, const FRTFocusMoveSubject& Subject);

	/**
	 * Mappa ristretta e calcolo insieme. Un soggetto la cui cella la squadra non vede adesso non ha area: nessuna delle
	 * due liste, come una cella vuota.
	 */
	static FRTFocusMoveAreas MoveAreasFor(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge,
		const FRTFocusMoveSubject& Subject);
};
