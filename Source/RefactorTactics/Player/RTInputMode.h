#pragma once

#include "GameFramework/PlayerController.h"

/**
 * IL MODO DI INPUT DEL GIOCO, dichiarato in un posto solo (`#2989`).
 *
 * 🔴 **Fino al 2026-10-12 lo dichiarava solo il frontend, e la partita lo ereditava senza dirlo.**
 * `FInputModeGameAndUI::ApplyInputMode` scrive sul `UGameViewportClient` — cattura del mouse
 * `CaptureDuringMouseDown`, cursore nascosto durante la cattura — e il viewport sopravvive al cambio di
 * mappa: una partita aperta DAL FRONTEND girava con quelle impostazioni, una aperta direttamente sulla mappa
 * col default del progetto (`DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown`,
 * `Config/DefaultInput.ini`). Due partite uguali, due modi di input, e nessuna riga che lo dicesse.
 *
 * 🔑 **Una funzione, due chiamanti**: `ARTFrontendGameMode::ConfigureMenuInput` e
 * `ARTPlayerController::BeginPlay`. La parita' non e' una convenzione da ricordare: e' la stessa chiamata.
 * Decisione d'autore del 2026-10-12 (`#2989`), fra «dichiararlo nella partita» e «solo documentare».
 *
 * ⚠️ **Cosa e' misurato e cosa no.** Con questo modo il click su uno slot del dock arma: seduta PIE del
 * 2026-10-07, partita aperta dal frontend (`docs/technical/evidence/hud/dock-click-dal-frontend-2026-10-07.log`).
 * Che la partita aperta direttamente sulla mappa si comporti ora allo stesso modo e' vero per costruzione
 * della chiamata, non per misura: lo guarda la voce `PIE-V01-DOCKCLICK`.
 *
 * ⚠️ **In headless non fa niente oltre al cursore**: `APlayerController::SetInputMode` esce se manca il
 * viewport di gioco o il `ULocalPlayer`. Un Automation Test non puo' quindi osservare il modo, solo la meta'
 * `bShowMouseCursor` — che e' cio' che `Frontend.*` asserisce gia'.
 *
 * `GameAndUI` e non `UIOnly`: la ragione sta accanto al chiamante del frontend.
 */
namespace RTInputMode
{
	inline void ApplyGameAndUI(APlayerController& PlayerController)
	{
		PlayerController.bShowMouseCursor = true;
		PlayerController.SetInputMode(FInputModeGameAndUI());
	}
}
