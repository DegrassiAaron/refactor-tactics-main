#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "RTGameViewportClient.generated.h"

/**
 * **La 3D si rende solo nel rettangolo fra le fasce dell'HUD** ([D-497] (1), #3661).
 *
 * 🔑 **Come.** `UGameViewportClient::LayoutPlayers` scrive `ULocalPlayer::Origin`/`Size` dalla tabella dello
 * split-screen; qui, dopo `Super`, il giocatore riceve il rettangolo di `RTHudLayout`. Tutto il resto segue da
 * se': `ULocalPlayer::GetProjectionData` legge `Origin`/`Size`, quindi la proiezione della camera, la
 * deproiezione del click e `ProjectWorldLocationToScreen` usano lo stesso rettangolo.
 *
 * 🔑 **Quando.** UE chiama `LayoutPlayers` a ogni fotogramma prima del `Draw`: in PIE da `UEditorEngine::Tick`,
 * in standalone da `UGameEngine::RedrawViewports`. Un ridimensionamento, il 16:10 e il 21:9 si seguono da
 * soli, senza un evento da ascoltare.
 *
 * ⚠️ **Solo mentre l'HUD tattico e' montato.** Lo accende `URTTacticalHUDWidget` in `NativeConstruct` e lo
 * spegne in `NativeDestruct`: una scena senza fasce attorno (un menu, una mappa di laboratorio senza HUD)
 * resta a schermo intero, invece di una cornice nera senza pannelli.
 *
 * ⚠️ **Un giocatore solo.** Con piu' giocatori locali la tabella dello split-screen resta quella di UE: lo
 * split-screen e' fuori da #3661.
 *
 * Registrata in `Config/DefaultEngine.ini` (`GameViewportClientClassName`).
 */
UCLASS()
class REFACTORTACTICS_API URTGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual void LayoutPlayers() override;

	/**
	 * Chiede (o smette di chiedere) il rettangolo della scena. Conta le richieste invece di tenere un `bool`:
	 * due HUD montati in sequenza non si spengono a vicenda nell'ordine sbagliato.
	 */
	void SetSceneInsetRequested(bool bRequested);

	bool IsSceneInsetActive() const { return SceneInsetRequests > 0; }

	/**
	 * Il viewport client del mondo di `WorldContext`, se e' questo tipo. `nullptr` in un mondo senza viewport
	 * di gioco (i test headless, un server dedicato).
	 */
	static URTGameViewportClient* Get(const UObject* WorldContext);

private:
	int32 SceneInsetRequests = 0;
};
