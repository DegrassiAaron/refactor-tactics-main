#pragma once

#include "NativeGameplayTags.h"

// Status effect del combattimento (dichiarati nativamente, nessun asset richiesto).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Root);   // non puo' muoversi
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Slow);   // range di movimento dimezzato
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Reveal); // intento visibile agli avversari (invariante #6)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Exposed); // scoperta: +5 al PRIMO danno diretto, scade nel Cleanup
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Guarded); // guardia: -15 al PRIMO danno diretto e resiste a una spinta di 1
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Marked);  // marchiato: +6 al PROSSIMO attacco alleato, che lo consuma
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Wet);     // bagnato: +8 a Flux.LinearDischarge finche' attivo (E6/CP 6.2).
                                                     // Applicato da Riva (CP 6.3, non ancora costruito).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Braced);  // irrigidito: -10 a OGNI danno diretto e blocca la prima spinta,
                                                     // fino al Cleanup (CP 5.2). Distinto da Guarded, che vale
                                                     // sul PRIMO colpo e regge solo una spinta di 1.
