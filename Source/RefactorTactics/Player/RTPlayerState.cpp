#include "Player/RTPlayerState.h"

#include "GameFramework/PlayerController.h"

int32 ARTPlayerState::TeamIdOf(const APlayerController* Controller)
{
    if (const ARTPlayerState* PS = Controller ? Cast<ARTPlayerState>(Controller->PlayerState) : nullptr)
    {
        return PS->GetTeamId();
    }
    return 0;
}
