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
// (E8/CP 8.1) e Phase (E6/CP 6.3). Effetti: +8 a Gadget.LinearDischarge finche' attivo, conduce elettricita'
// (CP 8.3) e rimuove Burning (CP 8.4).
//
// La durata "finche' sulla cella" e' VIVA, e passa da tre pezzi: il terreno la dichiara con
// `StatusDuration == 0` (`URTTerrainLibrary::CellBoundStatusesFor`), `ARTUnit::ApplyStatus` la riceve come
// `PersistentWhileOnCell` (-1) e la mette in `CellBoundStatuses` invece che in `StatusTurns`, e
// `RevokeCellBoundStatusesNotIn` la toglie quando l'unita' lascia la cella. Non scade a tempo: scade a
// geografia.
// ⚠️ Questa riga diceva *«oggi il terreno lo dichiara ma non lo applica davvero: la durata arriva con
// CP 8.2»* — al futuro, e **E8.2 e' chiusa dal 2026-08-07** (#1322).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Wet);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Braced);  // irrigidito: -10 a OGNI danno diretto e blocca la prima spinta,
                                                    // fino al Cleanup (CP 5.2). Distinto da Guarded, che vale
                                                    // sul PRIMO colpo e regge solo una spinta di 1.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Burning); // in fiamme: danno nel Cleanup (CP 8.2), rimosso da Wet
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Obscured); // offuscato: targeting limitato a 2 celle (fumo)
/**
 * Elettrificato: il catalogo terreni §2 lo dichiara ISTANTANEO — «una sola volta per evento» — quindi non
 * entra in `StatusTurns` e non ha durata: e' l'etichetta del danno di un evento di propagazione, non uno
 * stato che dura nel tempo.
 *
 * 🔴 **CONSUMATORE ANCORA ASSENTE, e ora non e' piu' una previsione: e' un residuo.** Questa riga diceva
 * *«la propagazione elettrica arriva con CP 8.3»* — **CP 8.3 e' arrivato il 2026-08-07** (`#66`, E8.3:
 * propagazione sul grafo dell'acqua, limite tre passi, unicita' per evento) **e non consuma questo tag**.
 * Misurato: fuori da `RTGameplayTags.cpp` l'unica occorrenza in tutto `Source/` e'
 * `RTIconCatalogTests.cpp`, che ne pretende l'ICONA. Un tag inerte con un'icona obbligatoria.
 * ⚠️ Se serva o vada tolto e' una decisione, ed e' **#1324** invece di restare qui.
 *
 * ✅ **L'argomento sull'istantaneita' resta valido**: il tag e' qui perche' `Wet` e `Conductive` esistono
 * gia' e la loro semantica lo nomina; dargli una durata inventata per farlo sembrare vivo sarebbe peggio di
 * un dato dichiaratamente inerte (stesso pattern di `PushResistance` di Riktor).
 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Electrified);
