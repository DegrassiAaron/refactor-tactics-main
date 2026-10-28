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
 * Cella, movimento base, `Slow` e corsa negata, e nessun attore: e' il confine che tiene l'area fuori dal piano privato
 * del nemico. Il calcolo che li consuma non vede ne' `ARTUnit` ne' `PlannedWaypoints`, e il solo punto che legge
 * l'attore e' `SubjectFor`.
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

	/** Serve a `FRTPlanningOverlayInputs`: se il soggetto cambia (si muove, prende `Slow`), l'area si rifa'. */
	bool operator==(const FRTFocusMoveSubject& Other) const
	{
		return Cell == Other.Cell
			&& BaseMoveRange == Other.BaseMoveRange
			&& MoveCostModifier == Other.MoveCostModifier
			&& bRunDenied == Other.bRunDenied;
	}
	bool operator!=(const FRTFocusMoveSubject& Other) const { return !(*this == Other); }
};

/**
 * Le due aree di un nemico osservato. Entrambe sono INSIEMI DI CELLE ORDINATI in modo stabile (`StableLess`) e
 * includono la cella del soggetto (`ReachableCells` parte sempre da li'). Quando la corsa c'e', `Normal` e' contenuta
 * in `Run`, e chi disegna sceglie se mostrarla per intero o al netto della normale. Quando la corsa e' negata `Run` e'
 * VUOTA, non coincide con `Normal`.
 */
struct REFACTORTACTICS_API FRTFocusMoveAreas
{
	/** Dove arriva col movimento normale: `1x` il movimento base (profilo `Move`, D-412). */
	TArray<FRTCellId> Normal;

	/** Dove arriva con la corsa: `2x` (profilo `Sprint`). Vuota se la corsa e' negata (`bRunDenied`). */
	TArray<FRTCellId> Run;

	bool bRunDenied = false;
};

/**
 * L'AREA DI MOVIMENTO DI CHI E' A FUOCO: IL POTENZIALE DI UN NEMICO OSSERVATO, MAI IL SUO PIANO (D-552).
 *
 * I divieti sono questi, e ciascuno ha il suo test a mondi gemelli (`RTFocusAreaTests.cpp`):
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
 * ⚠️ **PRECONDIZIONE DEL CHIAMANTE: il soggetto dev'essere OSSERVATO, non solo su una cella in vista.** Il repository
 * distingue le due cose: un'unita' ignota (`ClassifyTarget` la respinge) puo' stare su una cella che `VisibleCells`
 * contiene, e una chiamata per lei darebbe un'area piena, cioe' la sua posizione e la sua portata. Il cancello e' una voce
 * `Live` nella vista di conoscenza (`URTInspectLibrary::IsInspectable`), come per lo sguardo e per la scheda. Questa
 * libreria non lo applica: riceve valori, non una vista. Lo pinna il test della fetta C, con un nemico ignoto su una cella
 * in vista.
 *
 * ⚠️ **Gli archi non sono celle.** Un ponte o una scala fra due celle in vista si legge com'e' adesso, e `VisibleCells`
 * non certifica lo stato di un arco (che puo' cambiare, ad esempio un ponte abbattuto). D-552 parla solo di celle: se
 * serve altro, e' una domanda per l'owner della conoscenza parziale.
 *
 * ⛔ Presentazione pura: nessuno stato canonico, niente hash, TurnLog o replay. Non cablata (fetta C).
 */
UCLASS()
class REFACTORTACTICS_API URTFocusAreaLibrary : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * L'UNICO punto che legge l'attore: cella, movimento base, `Slow`, `Unbalanced`, e nient'altro. Nessuna lettura dal
	 * piano (`PlannedWaypoints`, `PlannedAbilityIndex`, `PlannedMovementProfileId`...): un test lo pinna con due unita'
	 * gemelle dal piano diverso.
	 *
	 * Non dice se l'unita' e' osservata: vedi la precondizione sulla classe.
	 */
	static FRTFocusMoveSubject SubjectFor(const ARTUnit* Unit);

	/**
	 * Una copia della mappa in cui ogni cella fuori da `Knowledge.VisibleCells` blocca il movimento. Solo `VisibleCells`,
	 * mai l'unione con `ExploredCells`. Il chiamante tiene viva la copia e, in partita, la mette in cache:
	 * `DuplicateObject` non va a ogni fotogramma.
	 *
	 * ⚠️ **Neutralizza SOLO il movimento, e non e' «la mappa di cio' che si vede».** La copia conserva copertura, porte,
	 * muri interni, transizioni, superficie, costo e linea di vista delle celle nascoste (a differenza di
	 * `MakeOptimisticPlanningMap`, che le azzera). Per il calcolo del movimento e' inerte, perche' il pathfinder salta
	 * ogni cella che blocca; per qualunque altra domanda NON lo e'. Va passata a `MoveAreas` e a nient'altro: la portata
	 * a tre stati (fetta B) non la riusa per la linea di vista.
	 */
	static URTHexMapAsset* MakeVisibleOnlyMap(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge);

	/** Il calcolo su una mappa gia' ristretta. Funzione pura di mappa e soggetto. */
	static FRTFocusMoveAreas MoveAreas(const URTHexMapAsset* VisibleOnlyMap, const FRTFocusMoveSubject& Subject);

	/**
	 * Mappa ristretta e calcolo insieme. Un soggetto la cui cella la squadra non vede adesso non ha area: nessuna delle
	 * due liste, come una cella vuota. Vale `VisibleCells`, non `ExploredCells`: una cella esplorata e non piu' in vista
	 * non rivela dove sta l'unita'.
	 *
	 * ⚠️ **Duplica la mappa a ogni chiamata**: e' la scorciatoia dei test e di chi calcola una volta. In partita, per
	 * ingressi invariati, si tiene `MakeVisibleOnlyMap` in cache e si chiama `MoveAreas`. E vale la precondizione
	 * sulla classe: il chiamante ha gia' verificato che il soggetto sia osservato.
	 */
	static FRTFocusMoveAreas MoveAreasFor(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge,
		const FRTFocusMoveSubject& Subject);
};
