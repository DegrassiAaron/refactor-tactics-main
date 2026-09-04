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
 * 🔴 **Ed e' una funzione LIBERA, non una `UBlueprintFunctionLibrary`, contro l'abitudine del modulo.**
 * Il pattern qui e' `UCLASS` + statiche, e questa e' l'unica `REFACTORTACTICS_API` su una funzione libera
 * del modulo — quindi la deroga va motivata invece di lasciarla notare. Una libreria Blueprint che
 * rendesse un `ARTTurnManager*` lo **esporrebbe ai Blueprint**, e l'header di `UI/RTScreenHudWidgets`
 * dichiara la regola opposta: *«non c'e' un accessor che dia l'`ARTTurnManager` o un `ARTUnit` a un
 * Blueprint»*. Esporlo per alleggerire un include sarebbe pagare un confine di privacy con un tempo di
 * compilazione.
 *
 * 🔴 **Rende un `TWeakObjectPtr`, e non e' una comodita': e' la ragione per cui il distacco REGGE.**
 * `TWeakObjectPtr<T>::operator=(T*)` deve convertire `T*` in `UObject*`, e una conversione di base vuole il
 * **tipo completo**. Il chiamante tiene un `TWeakObjectPtr<ARTTurnManager>`: se questa funzione rendesse un
 * puntatore nudo, l'assegnazione richiederebbe l'header proprio nel file che questa esiste per liberarne.
 *
 * ⚠️ **Con un `ARTTurnManager*` il file compilava lo stesso, ma per un accidente della unity build**:
 * il tipo completo arrivava da un `.cpp` vicino nello stesso blob. Appena UBT estrae il file dal blob —
 * cosa che fa da sola per ogni file modificato di recente — la compilazione cade con `C2679` su
 * `WeakObjectPtrTemplates.h`. Misurato il 2026-09-04 su `UI/RTScreenHudWidgets.cpp` al contenuto di #2217,
 * con una sola riga di commento aggiunta per forzarne la compilazione isolata. La copia
 * `TWeakObjectPtr` → `TWeakObjectPtr` non tocca `T` e non vuole niente.
 *
 * 🔑 Conta oltre il singolo file: l'AC di #1821 dichiara che **la compilazione** e' la prova
 * strutturale del distacco — «un file che non include piu' l'header e continua a compilare non ne aveva
 * bisogno». Con l'unity build attiva quella prova si puo' superare a vuoto, ed e' cosi' che era passata.
 *
 * ⚠️ **Rende un riferimento NON VALIDO quando il manager non c'e' ancora, e non e' un caso limite.**
 * `ARTGameMode::BeginPlay` presenta il HUD **prima** di spawnare il `ARTTurnManager`: un widget che nasce
 * in quel momento cerca un actor che non esiste. Chi chiama questa funzione tiene il proprio retry — non
 * lo eredita da qui.
 */
REFACTORTACTICS_API TWeakObjectPtr<ARTTurnManager> FindTurnManagerInWorld(const UWorld* World);
