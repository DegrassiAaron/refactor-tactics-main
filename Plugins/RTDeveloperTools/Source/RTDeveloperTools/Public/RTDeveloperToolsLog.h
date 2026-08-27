#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

/**
 * Categoria di log del bridge MCP. Registra CHI ha chiamato COSA e con quale esito.
 *
 * Livelli, e la ragione della divisione: `Log` per una riga per chiamata (nome del tool, esito, durata) —
 * e' il tracciato che serve a capire cosa ha chiesto il client. `Verbose` per il dettaglio dei parametri.
 * Il percorso nodo-per-nodo di un A* non si logga a nessun livello da qui: il pathfinder e' autorevole e non
 * viene strumentato dal bridge.
 *
 * Non loggare dati sensibili: intenti di squadra, contenuti di `CanonicalIntentStore`, credenziali. Il bridge
 * non li legge affatto, e questa riga esiste perche' resti vero anche per chi aggiungera' il prossimo tool.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogRTDevTools, Log, All);
