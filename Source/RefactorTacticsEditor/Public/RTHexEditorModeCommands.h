#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/** Comandi (bottoni tool) dell'Editor Mode hex. In H5a nessun comando; H5b aggiunge SelectTool. */
class FRTHexEditorModeCommands : public TCommands<FRTHexEditorModeCommands>
{
public:
	FRTHexEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

	/** Tool di selezione a click (H5b). */
	TSharedPtr<FUICommandInfo> SelectTool;

	/** Tool di paint/erase a click (H5c). */
	TSharedPtr<FUICommandInfo> PaintTool;

	/** Tool creazione transizioni con gizmo (H5c.2). */
	TSharedPtr<FUICommandInfo> ArchTool;

	/** Tool secchiello / flood-fill (H5c.7). */
	TSharedPtr<FUICommandInfo> FillTool;

	/** #712: il gesto dell'autore — disegna un muro quantizzato che cuoce in coperture. */
	TSharedPtr<FUICommandInfo> GeometryTool;

	/**
	 * `#623` / seduta `U21`: `Home` inquadra l'intera mappa editabile, multilivello compreso.
	 *
	 * ⚠️ **Non e' un tool e non entra in `Commands`**, che e' la palette del mode: e' un'AZIONE, e la issue
	 * chiede esattamente una scorciatoia — *«`Home` -> inquadrare l'intera mappa editabile: oggi non esiste
	 * un modo di dire fammi vedere tutto»*. Tutto il resto della navigazione (pan, orbit, zoom, `F`) lo
	 * fornisce gia' il viewport di Unreal, e un `UEdMode` non possiede la camera del viewport.
	 */
	TSharedPtr<FUICommandInfo> FrameMap;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};
