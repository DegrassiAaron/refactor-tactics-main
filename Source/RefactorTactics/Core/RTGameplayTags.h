#pragma once

#include "NativeGameplayTags.h"

// Status effect del combattimento (dichiarati nativamente, nessun asset richiesto).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Root);   // non puo' muoversi
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Slow);   // range di movimento dimezzato
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Reveal); // intento visibile agli avversari (invariante #6)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Exposed); // scoperta: +5 al PRIMO danno diretto, scade nel Cleanup
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Guarded); // guardia: POOL di 15 assorbibili sull'arco frontale ([D-292]), resiste a una spinta di 1
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
 * ✅ **IL CONSUMATORE C'E', dal 2026-09-03** (`#1324`, [D-315]): `ARTTurnManager` scrive una voce
 * `ERTLogCategory::Status` con esito `AppliedInstantly` per ogni unita' che la propagazione raggiunge, nel
 * Cleanup e sulla cella in cui l'ha raggiunta. Il consumatore e' il **TurnLog**, non `ApplyStatus`.
 *
 * 🔴 **E non poteva essere `ApplyStatus`, che e' la ragione per cui il tag e' rimasto inerte tanto a
 * lungo.** CP 8.3 e' arrivato il 2026-08-07 (`#66`) e non l'ha consumato: `ARTUnit::ApplyStatus` sa
 * rappresentare `N` turni oppure il legame alla cella, e per `Turns <= 0` **ritorna in silenzio**. Un
 * evento istantaneo non e' nessuna delle due forme, quindi il consumatore atteso non l'ha *rifiutato* —
 * **non poteva applicarlo** senza inventargli una durata.
 *
 * ⛔ **Non entra in `StatusTurns` e l'unita' non lo porta addosso**: non ha scadenza, nessuno lo revoca, e
 * nessuna voce di morte lo segue. Cio' che e' cambiato e' che la scarica non e' piu' **muta** in un replay.
 *
 * ✅ **L'argomento sull'istantaneita' e' stato onorato, non aggirato**: il tag e' qui perche' `Wet` e
 * `Conductive` esistono gia' e la loro semantica lo nomina; dargli una durata inventata per farlo sembrare
 * vivo sarebbe peggio di un dato dichiaratamente inerte (stesso pattern di `PushResistance` di Riktor).
 *
 * ⚠️ `UI.Icon.Status.Electrified` resta una chiave richiesta in `RTIconCatalogTests`, e la sua icona
 * **esiste**: `Content/RT/UI/Icons/` porta **undici** `RT_UI_Icon_Status_*` versionate — `Braced`,
 * `Burning`, `Electrified`, `Exposed`, `Guarded`, `Marked`, `Obscured`, `Reveal`, `Root`, `Slow`, `Wet`.
 *
 * 🔴 **Questa riga diceva l'opposto — «in `Content/` versionato non esiste nessuna icona di stato, e
 * `Wet`, `Burning`, `Obscured` contano tutte zero» — ed era falsa gia' quando e' stata scritta** (`#2244`).
 * Le icone entrano con `7112056f` il **2026-08-28**; la frase con `2c43dbfc` il **2026-09-03**, sei giorni
 * dopo. Non e' pedanteria: e' la frase che il prossimo autore legge per decidere se puo' usare un'icona, e
 * gli fa scrivere il ripiego testuale che non serviva — quello che `RTHUD.cpp` porta ancora, con **due**
 * stati su undici mostrati come testo.
 *
 * ⚠️ **Cio' che resta vero di questo tag e' l'INERZIA, ed e' l'altra meta' della nota**: `Electrified`
 * **accade e non permane**. Ha una voce nel TurnLog — `ERTStatusOutcome::AppliedInstantly` esiste apposta
 * per lui — ma non entra in `StatusTurns`, l'unita' non lo porta addosso, e `HasStatus` non risponde mai
 * vero. Chi disegna gli stati non deve aprirgli uno stato persistente: sarebbe un'icona accesa per sempre.
 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Electrified);
