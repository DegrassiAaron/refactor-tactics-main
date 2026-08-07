#pragma once

#include "NativeGameplayTags.h"

// Status effect del combattimento (dichiarati nativamente, nessun asset richiesto).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Root);   // non puo' muoversi
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Slow);   // range di movimento dimezzato
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Reveal); // intento visibile agli avversari (invariante #6)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Exposed); // scoperta: +5 al PRIMO danno diretto, scade nel Cleanup
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Guarded); // guardia: -15 al PRIMO danno diretto e resiste a una spinta di 1
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Marked);  // marchiato: +6 al PROSSIMO attacco alleato, che lo consuma
// Bagnato: UNA sola dichiarazione per DUE sorgenti che il catalogo vuole convergenti — l'acqua bassa
// (E8/CP 8.1) e Riva (E6/CP 6.3). Effetti: +8 a Flux.LinearDischarge finche' attivo, conduce elettricita'
// (CP 8.3) e rimuove Burning (CP 8.4). Oggi il terreno lo dichiara ma non lo applica davvero: la durata
// "finche' sulla cella" arriva con CP 8.2 (vedi spec-terreni-e8.md §6-bis).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Wet);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Braced);  // irrigidito: -10 a OGNI danno diretto e blocca la prima spinta,
                                                    // fino al Cleanup (CP 5.2). Distinto da Guarded, che vale
                                                    // sul PRIMO colpo e regge solo una spinta di 1.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Burning); // in fiamme: danno nel Cleanup (CP 8.2), rimosso da Wet
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Obscured); // offuscato: targeting limitato a 2 celle (fumo)
