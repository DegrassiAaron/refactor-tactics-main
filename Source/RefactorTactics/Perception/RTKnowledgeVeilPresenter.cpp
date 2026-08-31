#include "Perception/RTKnowledgeVeilPresenter.h"

#include "GameFramework/PlayerController.h"
#include "Map/RTHexMapActor.h"
#include "Player/RTPlayerState.h"
#include "Turn/RTTurnManager.h"

void URTKnowledgeVeilPresenter::Hook(ARTTurnManager* InTurnManager)
{
	if (!InTurnManager)
	{
		return;
	}

	TurnManager = InTurnManager;

	// ⚠️ **`AddUniqueDynamic` e non `AddDynamic`**, per la ragione gia' registrata in
	// `Frontend/RTFrontendGameMode.cpp`: il ciclo di vita puo' passare di qui piu' di una volta sullo stesso
	// oggetto in editor, e due iscrizioni stenderebbero il velo due volte per refresh — non un errore visivo,
	// ma il doppio del costo su ogni cella.
	InTurnManager->OnTeamKnowledgeRefreshed.AddUniqueDynamic(
		this, &URTKnowledgeVeilPresenter::HandleTeamKnowledgeRefreshed);

	// 🔴 **E si stende SUBITO, non al primo refresh.** Senza questa riga la board nasce interamente visibile
	// e si vela al primo `RefreshTeamKnowledgeForPlanning`: il primo fotogramma e' quello che rivela tutta la
	// mappa, ed e' l'unico che nessun test potrebbe prendere dopo. E' una voce esplicita della DoD di `E13.8`.
	Apply();
}

int32 URTKnowledgeVeilPresenter::ViewerTeamId() const
{
	// L'`Outer` E' il viewer: il presenter appartiene al client che guarda. La squadra la risponde il
	// PlayerState, con il ripiego a `0` che vale per tutti e tre i modi di non averla.
	return ARTPlayerState::TeamIdOf(Cast<APlayerController>(GetOuter()));
}

void URTKnowledgeVeilPresenter::Apply()
{
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM)
	{
		return;
	}

	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(TM->GetWorld());
	if (!HexMap)
	{
		return;
	}

	// 🔑 **La via scelta e' `KnowledgeForTeamPublic`, e le altre due sono state scartate per ragioni
	// diverse** — `E13.8` chiede che la scelta sia dichiarata, non solo fatta:
	//
	//  - `MakeCurrentSnapshot` e' pubblica e consegnerebbe la conoscenza di ENTRAMBE le squadre, ma fa
	//    `GetAllActorsOfClass` e due `Sort` ed e' la sua parte cara: qui serve una squadra sola, a ogni
	//    refresh. Il suo stesso commento la dichiara «proibitiva a ogni frame».
	//  - **portare la conoscenza nel payload del delegate** sceglierebbe il team per conto di tutti i
	//    subscriber, cioe' farebbe alla firma esattamente cio' che [D-227] le vieta.
	//
	// ⚠️ `KnowledgeForTeamPublic` **non e' una `UFUNCTION`**, deliberatamente: esporla in Blueprint aprirebbe
	// un canale verso la conoscenza NON filtrata di una squadra qualunque. Da C++ va bene; da Blueprint la
	// porta resta `FRTKnowledgeView`.
	//
	// 🔴 **E' anche il punto che il multiplayer dovra' cambiare**: vedi il disegno di rete nell'header. Qui
	// il client legge una conoscenza canonica LOCALE, cosa che in rete sarebbe una fuga di informazione — il
	// server dovra' sanificarla per squadra e replicarne il DTO al solo owner autorizzato.
	HexMap->ApplyKnowledgeVeil(TM->KnowledgeForTeamPublic(ViewerTeamId()));

	// L'anello osservabile: `GetVeilCounts` dice com'e' la board, questo dice QUANTE VOLTE e' stata
	// ridipinta. Senza, «steso una volta sola» e «ridipinto con conoscenza vuota» sono indistinguibili.
	++Applications;
}

void URTKnowledgeVeilPresenter::HandleTeamKnowledgeRefreshed(int32 /*TurnNumber*/)
{
	// Il numero di turno non serve: il velo non ha memoria e non interpola, ridipinge lo stato corrente.
	// Riceverlo e ignorarlo e' comunque giusto — e' la firma del delegate, non una scelta di questo sito.
	Apply();
}
