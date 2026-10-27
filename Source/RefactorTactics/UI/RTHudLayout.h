#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/RTHudZoneWidget.h"

/**
 * **Le fasce dell'HUD di partita, e il rettangolo della scena che lasciano libero** ([D-497], #3661).
 *
 * 🔑 **Una sorgente sola.** Prima di qui il riquadro libero viveva in tre posti, nessuno di runtime: una
 * frazione nei test (`CenterFraction`), una griglia nei test (`RTGrigliaZone`) e gli anchor dell'asset
 * `WBP_RT_TacticalHUD`. Ora le zone dell'HUD (`URTTacticalHUDWidget`), il rettangolo in cui si rende la 3D
 * (`URTGameViewportClient`), la finestra di reazione e i test leggono **questo** dato.
 *
 * 🔑 **Le fasce sono la misura dei loro inquilini, in unita' Slate** (decisione d'autore del 2026-10-11 in
 * #3661). Le frazioni 20/19,2/19/18 ne sono il valore a 1920×1080 (D-497; la bassa era il 17% fino a D-549). Con la scala dell'interfaccia di
 * UE (`ShortestSide`) gli inquilini tengono la loro misura in unita' Slate, quindi una colonna a frazione fissa
 * a 16:10 non li conterrebbe: a 1280×800 la sinistra vale 384 × 0,741 ≈ 284 px, il 22,2%.
 *
 * ⚠️ **Il margine di 4 unita' sta DENTRO le fasce**: la scena confina con le fasce, e le zone si staccano di 4
 * dai loro bordi, come nella griglia di prima.
 */
struct REFACTORTACTICS_API FRTHudBands
{
	/** Colonna sinistra: roster e scheda dell'unita' finiscono a x=380 (D-497 (2)), piu' il margine. */
	float Left = 384.f;
	/** Colonna destra: il registro e il badge accanto a `Conferma` cominciano a x=1556. */
	float Right = 368.64f;
	/** Fascia alta: il roster, in `Zone_TopLeft`, finisce a y=200. */
	float Top = 205.2f;
	/**
	 * Fascia bassa: il dock col suo prompt di bersaglio e i due margini di zona. L'altezza la misura il gate
	 * `ScreenHud.TenantsFitTheirBands` a ogni risoluzione, e decide il caso piu' stretto; 194,4 (il 18% a 1920×1080) e' il
	 * valore che D-549 sceglie sopra quella soglia. Era il 17% (183,6), D-497 (2).
	 */
	float Bottom = 194.4f;

	/**
	 * **La cella alta della colonna sinistra**, che nella colonna sinistra prende il posto della fascia alta.
	 *
	 * 🔑 **La colonna sinistra non segue il taglio della riga** (decisione d'autore del 2026-10-11, #3661). Il
	 * roster vero con tre unita' e' alto 259 unita' Slate, misurato da `ScreenHud.TenantsFitTheirBands`, e la
	 * fascia alta al 19% gliene lascia 197: il mockup su cui D-497 aveva misurato y=200 ne mostrava meno. La
	 * colonna e' tutta fascia, quindi il roster prende la sua altezza e la scheda parte sotto, senza toccare la
	 * scena. Vale 259 piu' i due margini. ⚠️ Il formato della v0.1 schiera due unita' per squadra; tre sono la
	 * sandbox, e il dato copre il caso piu' alto.
	 */
	float LeftTop = 267.f;
};

namespace RTHudLayout
{
	/** Il distacco di ogni zona dai bordi della sua fascia, in unita' Slate. */
	constexpr float ZoneMargin = 4.f;

	/** Il dato. */
	REFACTORTACTICS_API const FRTHudBands& Bands();

	/**
	 * Quanti pixel vale un'unita' Slate nel layer di gioco, per un viewport di questa misura.
	 *
	 * E' `UUserInterfaceSettings::GetDPIScaleBasedOnSize`, la stessa che `SGameLayerManager` applica all'HUD:
	 * il DPI della piattaforma che `SGameLayerManager::GetGameViewportDPIScale` toglie lo rimette Slate, quindi
	 * il netto e' questo.
	 */
	REFACTORTACTICS_API float UiScaleFor(const FIntPoint& ViewportPixels);

	/** Il rettangolo della scena in unita' Slate, per un viewport che in unita' Slate misura `ViewportSlate`. */
	REFACTORTACTICS_API FBox2D SceneRectSlate(const FVector2D& ViewportSlate, const FRTHudBands& InBands);

	/**
	 * Il rettangolo della scena in pixel del viewport.
	 *
	 * ⚠️ **Se le fasce non lasciano niente** (una finestra minuscola), torna il viewport intero: una scena di
	 * zero pixel non mostrerebbe la partita, e un HUD che copre la scena e' un difetto minore di nessuna scena.
	 */
	REFACTORTACTICS_API FBox2D SceneRectPixels(const FIntPoint& ViewportPixels, float UiScale, const FRTHudBands& InBands);

	/** Lo stesso rettangolo come `ULocalPlayer::Origin`/`Size`, in frazione del viewport. */
	/**
	 * Dove cade il riquadro dello sguardo: angolo alto-sinistro, accanto all'ancora e SEMPRE dentro il rettangolo della
	 * scena (mai sulle fasce dell'HUD). Se il riquadro e' piu' grande della scena vince il bordo alto-sinistro.
	 */
	REFACTORTACTICS_API FVector2D GlancePlacement(const FVector2D& Anchor, const FVector2D& Size, const FBox2D& SceneRect, float Margin);

	REFACTORTACTICS_API void SceneOriginSize(const FIntPoint& ViewportPixels, float UiScale, const FRTHudBands& InBands,
		FVector2D& OutOrigin, FVector2D& OutSize);

	/**
	 * Dove il Canvas dell'HUD deve mettere una zona: ancore ai bordi e misure in unita' Slate.
	 *
	 * 🔑 **Gli anchor a punto qui sono il modello, non il difetto.** Con la griglia a frazioni una zona doveva
	 * avere anchor stirati per scalare; con le fasce in unita' Slate una zona d'angolo e' proprio un punto
	 * dell'angolo piu' una misura fissa, e la stessa scala dell'interfaccia la porta a ogni risoluzione.
	 */
	REFACTORTACTICS_API FAnchorData ZoneLayout(ERTHudZone Zone, const FRTHudBands& InBands);

	/**
	 * Lo spostamento del centro della scena dal centro dello schermo, in unita' Slate: `((L − R)/2, (T − B)/2)`.
	 *
	 * Non dipende dalla risoluzione: le fasce sono misure fisse in unita' Slate, quindi la finestra di reazione,
	 * ancorata al centro dello schermo, si sposta sul centro della scena con un offset costante (D-497 (5)).
	 */
	REFACTORTACTICS_API FVector2D SceneCenterOffsetSlate(const FRTHudBands& InBands);
}
