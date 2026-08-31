#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Map/RTCellId.h"
#include "Map/RTMapDependencyLibrary.h"
#include "RTHexSelectionStore.generated.h"

class URTHexMapAsset;

/**
 * LA SELEZIONE, una sola per tutto il mode Hex Map (#1864).
 *
 * 🔴 **Nasce fuori dai `UInteractiveToolPropertySet`, ed e' il punto.** #921 ha misurato il difetto opposto:
 * `bShowOverlay` vive in due `PropertySet` distinti, ciascuno con la propria istanza creata in `Setup()`,
 * quindi accenderlo in Select non lo accende in Paint e cambiando tool l'impostazione «si perde». Uno stato
 * che deve sopravvivere al cambio di strumento non puo' stare dentro lo strumento.
 *
 * Un `UEditorSubsystem` sopravvive ai tool e al mode, non e' un Actor e non tocca l'asset: la selezione e'
 * stato d'editor puro e non va serializzata.
 *
 * ⛔ **Nessuna regola di gioco qui dentro.** Che cosa sia selezionabile e in quale ordine lo decide
 * `URTMapEditLibrary::ElementsAt`, che sta nel modulo runtime ed e' provata headless. Questa classe tiene il
 * risultato e la posizione nel ciclo — cioe' memoria d'interazione, non dominio.
 */
UCLASS()
class REFACTORTACTICSEDITOR_API URTHexSelectionStore : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Seleziona sotto un punto, **ciclando** sui candidati.
	 *
	 * Il primo click prende il piu' specifico; ri-cliccando lo stesso punto si scende al successivo e poi si
	 * ricomincia. Cliccare altrove azzera il ciclo.
	 *
	 * ⚠️ E' la risposta al caso che `ValidateMap` **permette**: una porta e una copertura `Low` sullo stesso
	 * bordo sono uno stato legale, e con una priorita' fissa il secondo elemento non sarebbe raggiungibile.
	 *
	 * Sostituisce la selezione corrente. Restituisce `false` se sotto quel punto non c'e' niente.
	 */
	bool SelectAt(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge);

	/**
	 * Aggiunge alla selezione invece di sostituirla — il gesto con modificatore.
	 *
	 * ⚠️ **Non cicla**: prende il candidato piu' specifico e basta. Un ciclo su un'aggiunta chiederebbe
	 * all'utente di ricordare a che punto del giro sta *per ciascun* punto gia' selezionato.
	 *
	 * Un elemento gia' selezionato non entra due volte, e non e' un dettaglio: la cancellazione itera la
	 * selezione, e due copie dello stesso handle proverebbero a rimuoverlo due volte.
	 */
	bool AddAt(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge);

	/** Gli elementi selezionati, nell'ordine in cui sono stati presi. */
	const TArray<FRTMapElementHandle>& GetSelection() const { return Selection; }

	/** Svuota la selezione e azzera il ciclo. */
	void Clear();

private:
	UPROPERTY()
	TArray<FRTMapElementHandle> Selection;

	/** Il punto dell'ultimo `SelectAt`, per sapere se il prossimo click e' «lo stesso punto». */
	FRTCellId CycleCell;
	ERTHexDirection CycleEdge = ERTHexDirection::E;
	bool bHasCycle = false;

	/** Quale candidato e' stato preso l'ultima volta su quel punto. */
	int32 CycleIndex = INDEX_NONE;
};
