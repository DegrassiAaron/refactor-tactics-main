#pragma once

#include "CoreMinimal.h"

class ARTTurnManager;
class UWorld;

/**
 * Trova il `ARTTurnManager` del mondo, **senza costringere il chiamante a includere le sue 1 856 righe**.
 *
 * ## Perche' esiste (`#1821`)
 *
 * `Turn/RTTurnManager.h` e' l'unica porta per chiunque debba anche solo *nominare* l'orchestratore, e
 * `Cast<>` piu' `StaticClass()` vogliono il tipo completo. Cosi' un consumatore che non chiama **nessun
 * metodo** — gli basta tenere un `TWeakObjectPtr` e passarlo altrove — paga comunque l'header intero, e
 * ogni modifica a quell'header lo ricompila.
 *
 * Misurato il 2026-09-03: dei 12 file di produzione che lo includono, `UI/RTScreenHudWidgets.cpp` usa il
 * tipo in tre punti — la ricerca, la firma di `SetMatchContextForTest`, il membro — e **non chiama un solo
 * metodo**. Per lui l'header e' interamente costo.
 *
 * Questa funzione sposta il `Cast` dietro un confine: la dichiarazione qui vuole solo una forward, e
 * l'unico file che include l'header pesante e' il `.cpp` accanto.
 *
 * ⛔ **Non e' una facade, e la distinzione conta.** Non nasce un secondo `UObject`, non si rivende lo
 * stato, non si duplica niente: e' una ricerca. #1821 esclude esplicitamente la facade («non servono due
 * `UObject`»), e il progetto ha gia' questa forma in `ARTHexMapActor::FindInWorld`.
 *
 * ⚠️ **Rende `nullptr` quando il manager non c'e' ancora, e non e' un caso limite.**
 * `ARTGameMode::BeginPlay` presenta il HUD **prima** di spawnare il `ARTTurnManager`: un widget che nasce
 * in quel momento cerca un actor che non esiste. Chi chiama questa funzione tiene il proprio retry — non
 * lo eredita da qui.
 */
REFACTORTACTICS_API ARTTurnManager* FindTurnManagerInWorld(const UWorld* World);
