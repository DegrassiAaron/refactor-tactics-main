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
 * 🔴 **Nasce fuori dai `UInteractiveToolPropertySet`, ed e' il punto.** #921 aveva misurato il difetto
 * opposto: `bShowOverlay` viveva in due `PropertySet` distinti, ciascuno con la propria istanza creata in
 * `Setup()`, quindi accenderlo in Select non lo accendeva in Paint e cambiando tool l'impostazione «si
 * perdeva». Uno stato che deve sopravvivere al cambio di strumento non puo' stare dentro lo strumento.
 *
 * ⏱️ **Al passato perche' #921 e' chiusa**: quel flag e' ora `URTHexEditorModeSettings::bShowSurfaceOverlay`,
 * uno stato del mode che i sette `Render` leggono dal context store. Le due classi hanno scelto due sedi
 * diverse per la stessa ragione — `UEditorSubsystem` qui, `UEdMode::SettingsClass` la' — e la differenza e'
 * la persistenza: la selezione **non** va conservata fra sessioni, il flag dell'overlay si'.
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
	/**
	 * Sostituisce la selezione con UN handle **gia' risolto da chi possiede il hit-test**.
	 *
	 * 🔑 **Esiste perche' non tutto passa da `ElementsAt`.** Quella funzione risponde alla domanda
	 * «che cosa c'e' sotto questo bordo», e un arco di transizione non ci sta: collega due celle su
	 * layer diversi e non giace su un bordo. La spec §13.3 assegna quel hit-test al **tool**, e
	 * `URTHexArchTool` lo possiede da sempre — qui entra il suo risultato.
	 *
	 * ⚠️ **Azzera il ciclo**, e deve: il ciclo e' legato a un PUNTO e ai suoi candidati, mentre
	 * questo handle non viene da un punto. Tenerlo in piedi farebbe continuare, al click successivo su una
	 * cella, un ciclo che appartiene a un'altra domanda.
	 */
	void SelectHandle(const FRTMapElementHandle& Handle);

	/**
	 * Aggiunge un handle gia' risolto senza duplicare. `false` se era gia' in selezione.
	 *
	 * ⚠️ La deduplica passa da `SameElement`, che per le transizioni legge la coppia **non
	 * ordinata**: andata e ritorno sono lo stesso arco, e senza quella regola lo stesso arco entrerebbe due
	 * volte e la cancellazione proverebbe a toglierlo due volte.
	 */
	bool AddHandle(const FRTMapElementHandle& Handle);

	void Clear();

	/**
	 * Che cosa e' selezionato, in chiaro — il readout che il pannello del mode mostra.
	 *
	 * ⚠️ **Dichiara il TIPO e l'IDENTITA', non solo che «qualcosa» e' selezionato**: e' cio' che #1864 chiede
	 * al readout, ed e' anche l'unico modo in cui chi clicca capisce a che punto del ciclo si trova. Un
	 * pannello che dicesse «1 elemento» lascerebbe indovinare se il prossimo Erase toglie la porta o la
	 * cella sotto.
	 *
	 * Statica e pura: si prova headless senza toccare lo stato dello store.
	 */
	static FString Describe(const TArray<FRTMapElementHandle>& Handles);

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
