#pragma once

#include "CoreMinimal.h"
#include "RTHexEditorModeSettings.generated.h"

/**
 * LE IMPOSTAZIONI DEL MODE Hex Map — una sola istanza per sessione di editing (#921).
 *
 * 🔴 **Nasce fuori dai `UInteractiveToolPropertySet`, ed e' il punto.** #921 ha misurato il difetto:
 * `bShowOverlay` viveva in due `PropertySet` distinti, ciascuno con la propria istanza creata in `Setup()`,
 * quindi accenderlo in Select non lo accendeva in Paint e cambiando strumento l'impostazione «si perdeva».
 * Uno stato che deve sopravvivere al cambio di strumento non puo' stare dentro lo strumento — e' la stessa
 * ragione per cui la selezione condivisa vive in `URTHexSelectionStore` (#1864).
 *
 * ⚠️ **L'alternativa scartata e' una property set base condivisa fra i tool**: lascerebbe il flag nel
 * pannello di *ogni* strumento, cioe' sette posti che mostrano la stessa impostazione. `UEdMode` ha gia' il
 * meccanismo giusto — `SettingsClass` / `SettingsObject` — e il toolkit mostra questo oggetto **sopra** i
 * tool, non dentro uno di essi (`FModeToolkit::SetModeSettingsObject`).
 *
 * 🔑 **`config` non e' decorativo, ed e' la ragione per cui questa classe non e' `Transient`.** I sette
 * property set del mode sono `UCLASS(Transient)` — lo stato di uno strumento non si conserva — quindi un
 * `UPROPERTY(config)` dichiarato li' dentro non persisterebbe, e il difetto sarebbe invisibile finche'
 * qualcuno non riapre l'editor. Qui `UEdMode::Enter` fa `LoadConfig()` e `UEdMode::Exit` fa `SaveConfig()`:
 * il flag sopravvive al rientro nel mode **e** alla chiusura dell'editor.
 */
UCLASS(config = EditorPerProjectUserSettings)
class REFACTORTACTICSEDITOR_API URTHexEditorModeSettings : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Disegna le superfici di tutte le celle sotto lo strumento attivo, qualunque esso sia.
	 *
	 * ⚠️ **Il contenuto dell'overlay non e' deciso qui**: lo disegna `RTHexEditor::DrawSurfaceOverlay`, che
	 * questa issue non tocca. Questo flag dice *se*, non *cosa*.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Hex Map",
		meta = (DisplayName = "Mostra overlay superfici",
			ToolTip = "Colora ogni cella secondo la propria superficie. Vale per tutti gli strumenti del mode."))
	bool bShowSurfaceOverlay = false;
};
