#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTUIPalette.generated.h"

/**
 * I token colore dell'HUD: `progettazione-hud.md` §32, con il prefisso `RT_UI_` tolto.
 *
 * Il nome del token e' `RT_UI_` + il nome del valore: `Text_Primary` e' `RT_UI_Text_Primary`. Il test
 * `RefactorTactics.UI.Palette.MatchesStyleGuide` lo usa per trovare la riga di §32 di ogni valore.
 *
 * Aggiungere valori solo IN CODA, e nello stesso commit la riga in §32: il test fallisce su un valore che
 * §32 non elenca e su una riga di §32 che l'enum non ha.
 */
UENUM(BlueprintType)
enum class ERTUIToken : uint8
{
	// §32 «Neutrali»
	BG_Deep,
	BG_Panel,
	BG_Raised,
	Frame_Deep,
	Frame_Mid,

	// §32 «Accenti»
	Cyan,
	Violet,
	Amber,
	Red,
	White,

	// §32 «Testo e fondi di stato» ([D-482])
	Text_Primary,
	Text_Secondary,
	Text_Disabled,
	Frame_Off,
	BG_Selected,
	BG_Reaction,
	BG_Invalid,
	BG_ProfileActive,

	// [D-489]: i colori che la barra dei comandi usava e §32 non aveva
	Icon_Cooldown,
	Violet_Light,
	Phase_Prep,
	Phase_Dash,
	Phase_Blast,
	Phase_Move,
};

/**
 * **La sede a runtime della palette dell'HUD** ([D-489], #3610).
 *
 * Prima di questa classe i colori di §32 vivevano come esadecimali ricopiati a mano nel costruttore di
 * `URTActionSlotWidget` e come valori impostati nei `WBP_*`. Un colore cambiato in §32 non raggiungeva
 * nessuna delle copie, e nessun test se ne accorgeva. Il precedente e' `URTOverlayPalette` (#1941), la sede
 * dei colori degli overlay del mondo.
 *
 * ⛔ **Il colore resta il secondo canale** (§47-bis.1): nessun token porta da solo uno stato.
 *
 * ⛔ **Non e' la palette degli overlay del mondo.** Quella sta in `URTOverlayPalette`, e la linea dello
 * scatto del Canvas (`RTHUD.cpp`, [D-234]) resta dov'e': `overlay_colors()` di
 * `tools/hud-assets/color_metrics.py` la rilegge da quella riga con una regex.
 */
UCLASS()
class REFACTORTACTICS_API URTUIPalette : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il colore di un token, pronto per un widget: **lineare**, decodificato da sRGB.
	 *
	 * ⚠️ `FLinearColor(R / 255, ...)` darebbe una tinta slavata: il costruttore prende valori lineari, e §32
	 * scrive sRGB. Lo stesso errore e' documentato in `Map/RTHexMapActor.cpp`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD|Palette")
	static FLinearColor ColorFor(ERTUIToken Token);

	/** Il colore di un token come §32 lo scrive: sRGB, opaco. */
	static FColor SRGBFor(ERTUIToken Token);

	/** Il nome del token in §32: `RT_UI_` + il nome del valore dell'enum. */
	static FString TokenName(ERTUIToken Token);
};
