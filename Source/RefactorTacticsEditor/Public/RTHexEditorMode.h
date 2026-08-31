#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "RTHexEditorMode.generated.h"

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
};
