#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RTPlayerState.generated.h"

class APlayerController;

/**
 * L'IDENTITA' DI SQUADRA DEL GIOCATORE, e la sua unica porta di lettura.
 *
 * 🔑 **Il viewer e' del giocatore, non della partita.** Fino a questa fetta viveva su
 * `ARTPlayerController::PlayerTeamId`, un `EditDefaultsOnly` che valeva `0` e che nessuno assegnava a
 * runtime: con due client varrebbe `0` per entrambi, e si romperebbe **in silenzio** perche' `0` e' una
 * risposta plausibile.
 *
 * ⛔ **`TeamId` non e' editabile**: e' stato di runtime, scritto da `AssignTeam`. Quando la replicazione
 * arrivera' il campo diventa `Replicated` con condizione owner-only e il seam e' gia' al posto giusto.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    /** La squadra assegnata. Ripiego `0` finche' nessuno ha assegnato: vedi `TeamIdOf`. */
    int32 GetTeamId() const { return TeamId; }

    /**
     * Assegna la squadra. **Il nome dichiara l'autorita'**: oggi non ha ancora nessun chiamante, perche'
     * il cablaggio (chi decide le squadre e chiama questo metodo) arriva in una fetta successiva del
     * piano; quando arrivera' sara' server-side.
     */
    void AssignTeam(int32 InTeamId) { TeamId = InTeamId; }

    /**
     * 🔑 **L'UNICA porta.** Risale dal controller al proprio `ARTPlayerState` e ripiega su `0`.
     *
     * ⚠️ **Il ripiego ha TRE cause e una sola risposta**: controller nullo, nessun PlayerState, PlayerState
     * della classe sbagliata — quest'ultimo e' cio' che `InitializeActorsForPlay` produce nei mondi di
     * prova, misurato il 2026-08-30. Tutte e tre valgono `0`.
     *
     * ⛔ **`0` e non `INDEX_NONE`**, e la ragione non e' pigrizia: `URTIntentPrivacyLibrary::FilterForTeam`
     * decide con `Intent.TeamId == ObserverTeamId`, quindi un osservatore invalido non nasconde **di
     * meno** — rovescia la simmetria, e gli intenti non rivelati dell'avversario diventano «alleati».
     * Nessuno dei consumatori ha una risposta per «nessuna squadra».
     */
    static int32 TeamIdOf(const APlayerController* Controller);

private:
    int32 TeamId = 0;
};
