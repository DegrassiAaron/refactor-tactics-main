#include "Turn/RTTurnManagerAccess.h"

#include "Turn/RTTurnManager.h" // l'UNICO posto che paga l'header pesante per conto dei chiamanti
#include "Kismet/GameplayStatics.h"

ARTTurnManager* FindTurnManagerInWorld(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	// Stessa ricerca che i consumatori facevano ciascuno per conto proprio: un solo actor per mondo, e
	// `GetActorOfClass` rende il primo. Non e' un cambio di semantica, e' un cambio di indirizzo.
	return Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(const_cast<UWorld*>(World), ARTTurnManager::StaticClass()));
}
