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

int32 ARTPlayerState::ControlGroupOf(const APlayerController* Controller)
{
	// Stesso ripiego di `TeamIdOf`, e per la stessa ragione: senza uno stato non c'e' un gruppo da leggere, e
	// `0` e' l'unico gruppo che esista quando il formato ne dichiara uno solo per squadra — la v0.1.
	if (const ARTPlayerState* PS = Controller ? Cast<ARTPlayerState>(Controller->PlayerState) : nullptr)
	{
		return PS->GetControlGroup();
	}
	return 0;
}
