#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "RTHexWorkGrid.h"
#include "RTHexEditorMode.generated.h"

class ARTHexWorkGridActor;

/**
 * Editor Mode dedicato alla mappa esagonale (UEdMode + Interactive Tools Framework). Non ha autorita' sui dati di
 * gioco: scrive solo sull'asset mappa via transaction. In H5a e' un guscio (nessun tool); H5b aggiunge la selezione.
 */
UCLASS()
class URTHexEditorMode : public UEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_RTHexEditorModeId;

	URTHexEditorMode();

	// UEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;

	/**
	 * `#622`: l'unico posto da cui la griglia di lavoro si accorge che qualcosa e' cambiato.
	 *
	 * ⚠️ **`UEdMode::ModeTick` e' un no-op virtuale** (`UEdMode.h:160`): questo override non cambia il
	 * comportamento di nient'altro. Cio' che fa a ogni fotogramma e' **un confronto**; il lavoro vero
	 * avviene solo quando `RTHexWorkGrid::FWatch` cambia, e il perche' sta nel docstring di quella struct.
	 */
	virtual void ModeTick(float DeltaTime) override;

	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

protected:
	/** Aggiunge il binding di `Home` a quelli che `UEdMode` installa da se' (fra cui `F` sulla selezione). */
	virtual void BindCommands() override;

private:
	/**
	 * `#623`: porta in vista tutte le celle dell'asset mappa, su tutti i layer.
	 *
	 * ⚠️ **Nessuna geometria qui dentro.** L'ingombro lo calcola `URTHexLibrary::CellsBoundsWorld`, che sta
	 * nel modulo runtime e ha i propri test: una regola che decide *dove finisce la mappa* non e' una
	 * decisione dello strumento che la disegna. Questo metodo raccoglie gli ingressi, chiama, e passa il
	 * risultato al viewport.
	 */
	void FrameEditableMap();

	/**
	 * `#1864`: cancella cio' che la selezione condivisa nomina, in **una sola** transazione.
	 *
	 * ⚠️ La regola non e' qui: `URTMapEditLibrary::DeleteElement` sta nel runtime, e' provata headless e sa
	 * gia' che cosa non puo' sopravvivere a cio' che si cancella. Questo metodo apre la transazione, chiama,
	 * e ricostruisce la vista — cioe' fa il mestiere dell'editor e non quello del dominio.
	 */
	void EraseSelection();

	/**
	 * `#622`: rifa' la griglia di lavoro se cio' da cui dipende e' cambiato.
	 *
	 * ⛔ **Nessuna regola qui**: quali coordinate marcare lo decide `RTHexWorkGrid::BuildPlan`, che e' puro
	 * e provato headless. Questo metodo raccoglie gli ingressi, chiama, e passa il risultato al portatore.
	 */
	void RefreshWorkGrid();

	/**
	 * Il portatore della griglia di lavoro, posato in `Enter()` e distrutto in `Exit()`.
	 *
	 * 🔴 **Debole e non `UPROPERTY`, e non e' un dettaglio.** Un riferimento forte a un actor transiente
	 * trattiene il mondo che lo ospita: e' la ragione per cui `URTScenarioPreviewSubsystem` tiene i propri
	 * con `TWeakObjectPtr` (`RTScenarioPreviewSubsystem.h:226-256`). Debole, il puntatore diventa
	 * automaticamente nullo se il mondo se ne va sotto i piedi — e `RefreshWorkGrid` ne posa uno nuovo.
	 */
	TWeakObjectPtr<ARTHexWorkGridActor> WorkGrid;

	/** Lo stato che DEFINISCE l'insieme, all'ultima ricostruzione. Vedi `RTHexWorkGrid::FWatch`. */
	RTHexWorkGrid::FWatch WorkGridWatch;

	/**
	 * DOVE l'insieme era posato, all'ultima posa.
	 *
	 * 🔑 **Separato dalla chiave perche' trascinare l'actor non cambia le celle.** Con un'unica chiave, un
	 * gizmo trascinato per tre secondi rifaceva sessanta volte al secondo la dilatazione, la `TSet` e la
	 * ricostruzione di migliaia di istanze, per un insieme identico traslato.
	 */
	RTHexWorkGrid::FPlacement WorkGridPlacement;

	/** L'ultimo piano calcolato, per poterlo **ri-posare** senza ricalcolarlo. */
	RTHexWorkGrid::FPlan WorkGridPlan;

	/**
	 * Quanti esagoni ha posato l'ultima ricostruzione.
	 *
	 * ⚠️ **Serve a non rifare il giro quando non c'e' niente da rifare.** Il portatore si posa solo se c'e'
	 * qualcosa da mostrare, quindi «nessun portatore» e' lo stato NORMALE su una mappa senza asset: senza
	 * questo contatore la guardia lo leggerebbe come «portatore perduto», rifarebbe il piano a ogni
	 * fotogramma e scriverebbe una riga di log per ciascuno.
	 */
	int32 WorkGridDrawn = 0;
};
