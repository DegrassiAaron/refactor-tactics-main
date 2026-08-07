#pragma once

#include "NativeGameplayTags.h"

// Status effect del combattimento (dichiarati nativamente, nessun asset richiesto).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Root);   // non puo' muoversi
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Slow);   // range di movimento dimezzato
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Reveal); // intento visibile agli avversari (invariante #6)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Exposed); // scoperta: +5 al PRIMO danno diretto, scade nel Cleanup
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Wet);      // conduce elettricita', spegne Burning
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Burning);  // danno nel Cleanup (CP 8.2), rimosso da Wet
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Obscured); // targeting limitato (Smoke)
