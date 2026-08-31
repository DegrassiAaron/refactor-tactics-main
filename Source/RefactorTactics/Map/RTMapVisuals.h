#pragma once

/**
 * Le misure del TILE che rappresenta una cella, in un posto che si può includere (#983).
 *
 * ⏱️ **Questo header ha parlato di «disco» fino al 2026-08-28**, e il nome descriveva la cosa: `5` uu di
 * spessore su `285` di diametro, un rapporto di `1:57` in cui il fianco del prisma non si vedeva. Dal
 * 2026-08-28 lo spessore è `0.06 H` — `15` uu, `1:19` — e il fianco è la ragione del cambio, quindi la
 * parola è cambiata con lui. **La geometria non è cambiata**: `GetCellPrismMesh` costruiva già un prisma
 * esagonale chiuso, con sei fianchi e uno scafo convesso esatto. Era schiacciato, non piatto.
 *
 * 🔴 **Non sono un dettaglio di rendering: chi disegna qualcosa SOTTO `RTCellTopZ` lo disegna dentro un
 * volume opaco, e a schermo non si distingue da qualcosa che non è stato disegnato affatto.** È successo
 * davvero, due volte: il contorno della superficie stava a `2.0` con la faccia a `2.5`, quindi fango e acqua
 * non si vedevano mentre i marcatori a `3.0` sì; e una stesura di #593 aveva messo il clearance degli anelli
 * a `1.0`, mezza unità dentro il disco — identità di squadra e anello di selezione sarebbero spariti a
 * schermo, con **la suite verde**, e a trovarlo è stata una code review.
 *
 * 🔴 **Vivevano in un namespace anonimo di `RTHexMapActor.cpp`**, quindi non le vedeva nessun altro modulo:
 * chi doveva posarci sopra qualcosa **ricopiava il numero**, e nessun compilatore collegava la copia
 * all'originale. Sono state promosse qui perché il legame lo faccia il compilatore e non la memoria di chi
 * legge — vedi lo `static_assert` accanto a `ARTUnit::RingGroundClearance`.
 *
 * ⚠️ **Sono le convenzioni del cilindro engine che il prisma ha sostituito**, tenute apposta: circumraggio
 * 50 uu (`PlanarScale` divide per 50) e Z centrato in `[-50, +50]`. Cambiarle qui muove in silenzio ogni
 * quota già tarata — i tre lift di debug-line, il contorno di superficie, il glifo, gli anelli a terra.
 * Questo header **non** è un sistema di layering: è il punto in cui i numeri condivisi stanno scritti una
 * volta sola (non-goal dichiarato in #983).
 */

/** Circumraggio del prisma della cella. `PlanarScale` divide per questo, e i lift ci si appoggiano. */
constexpr float RTCellPrismRadius = 50.f;

/**
 * `H` di riferimento: l'altezza del volume-cella, contro cui si misurano le ALTEZZE.
 *
 * ⚠️ **È il default di `URTHexMapAsset::LayerHeight`, ricopiato perché quello non è `constexpr`** — è un
 * `UPROPERTY(EditAnywhere)`, quindi una mappa può cambiarlo e nessuno `static_assert` lo può leggere. La
 * copia è deliberata e ha il suo oracolo: `RefactorTactics.HexMap.SerializationDefaultsArePinned` confronta
 * questo valore col CDO dell'asset, accanto alla riga che pinna `LayerHeight` stesso, e diventa rosso se
 * divergono. Un numero ricopiato senza un test che lo rilega è il difetto di #983; ricopiato *con* il test è
 * una dipendenza dichiarata.
 */
constexpr float RTCellLayerHeightRef = 250.f;

/**
 * 🔑 **Spessore del tile della cella, in frazioni di `H`** — `0.06 H` = **15 uu**.
 *
 * 🔴 **Era una frazione del RAGGIO fino al 2026-08-28, ed è il difetto che ha reso la cella più piatta senza
 * che nessuno lo decidesse.** La costante valeva `RTCellFlatScale = 0.05`, cioè un'altezza espressa contro
 * una LARGHEZZA: esattamente il denominatore sbagliato che `D-168` ha corretto nel contratto graybox, dove le
 * altezze sono passate da frazioni di `C` a frazioni di `H`. Il documento aveva diagnosticato il difetto; il
 * codice lo portava ancora.
 *
 * 🔴 **E si è pagato in silenzio.** `PlanarScale` deriva da `HexSize`, questa no: quando `#1155` ha portato
 * `HexSize` da `100` a `150` (`D-163`), la cella è diventata 1,5× più larga e lo spessore è rimasto `5` uu.
 * Il rapporto spessore:diametro è sceso dal **2,63%** all'**1,75%** — un terzo in meno, senza una riga di
 * diff che lo dicesse. Budgettato in `H` non può più succedere: `H` non cambia quando cambia `HexSize`.
 *
 * ⚠️ **Il tetto NON è lo `static_assert` sugli anelli, ed è più basso.** Il vincolo che morde per primo è
 * `URTHexLibrary::ReliefUnitHeight` (15 uu): un tile più spesso di un gradino di rilievo fa compenetrare due
 * celle a quota adiacente invece di farle gradinare. Lo asserisce `RTHexMapActor.cpp`, che è il file che posa
 * entrambi ed è quindi l'unico che può violarlo.
 */
constexpr float RTCellThicknessInH = 0.06f;

/** Spessore totale del tile in uu: **15**. */
constexpr float RTCellThickness = RTCellThicknessInH * RTCellLayerHeightRef;

/**
 * Quanto il prisma è schiacciato in Z. **DERIVATO dallo spessore, non scelto**: la mesh di
 * `GetCellPrismMesh` nasce con mezza-altezza `RTCellPrismRadius`, quindi alta `2 · RTCellPrismRadius`.
 *
 * ⚠️ **Non si edita questo numero, si edita `RTCellThicknessInH`.** Scriverlo a mano rimetterebbe uno
 * spessore misurato contro il raggio, che è il difetto appena chiuso.
 */
constexpr float RTCellFlatScale = RTCellThickness / (2.f * RTCellPrismRadius);

/**
 * Quota della faccia SUPERIORE del tile sopra il centro della cella: **7,5 uu**, cioè metà spessore.
 *
 * ⚠️ Il prodotto si scrive con `RTCellPrismRadius` e non col letterale `50`, che è ciò che questa riga
 * faceva pur avendo la costante due dichiarazioni più sotto nello stesso namespace — una terza copia dello
 * stesso numero dentro lo stesso file.
 *
 * ⏱️ **Valeva `2,5` fino al 2026-08-28.** Tutte le quote che ci si appoggiano — i quattro lift di
 * `RTHexMapActor.cpp`, `RTLastContactGhostZ`, i pannelli di muro, il rilievo, le colonne di blocco — sono
 * salite con lei senza una modifica, che è precisamente ciò per cui questo header esiste. L'unica che **non**
 * ha potuto seguire è `ARTUnit::RingGroundClearance`, perché è una costante indipendente: lo
 * `static_assert` accanto a lei ha fermato la build finché non è stata rialzata, ed è il comportamento
 * voluto.
 */
constexpr float RTCellTopZ = RTCellPrismRadius * RTCellFlatScale;

/**
 * Quota MONDO della sagoma dell'ultimo contatto (Task 6), sopra il CENTRO della cella (lo stesso riferimento
 * che da' `URTHexLibrary::AxialToWorld`, prima di qualunque `VisualZOffset`).
 *
 * ⚠️ **Non e' una delle quote gia' assegnate in `RTHexMapActor.cpp`** — superficie `RTCellTopZ + 0,5`, glifo
 * `RTCellTopZ + 0,3`, marker di blocco `RTCellTopZ + 1,5`, anteprima di pianificazione `RTCellTopZ + 2,5` —
 * che restano locali a quel file perche' decorano la FACCIA del disco. Questa e' diversa per natura (la base
 * di una sagoma VOLUMETRICA, non un decoro piatto) e vive qui, non in `RTUnit.cpp`, perche' quel file la deve
 * leggere senza ricopiare il numero (#983): sotto `RTCellTopZ` finirebbe dentro il disco e sparirebbe, com'e'
 * gia' successo due volte.
 */
constexpr float RTLastContactGhostZ = RTCellTopZ + 1.0f;

/**
 * 🔑 **Il CONFINE fra due celle** (#1758): la frazione del raggio occupata dall'anello di bordo.
 *
 * 🔴 **Il difetto che chiude non è «manca un colore», è «manca un canale».** Due celle adiacenti della
 * STESSA superficie sono due prismi dello stesso colore appoggiati l'uno all'altro: il colore non dice dove
 * finisce una e comincia l'altra, e su un gioco in cui il costo si conta in celle un giocatore che non vede
 * il confine non può contare il movimento. Colore di superficie e confine sono **due canali diversi**.
 *
 * 🔴 **L'anello è SOTTILE perché il perimetro è già occupato, e la misura va fatta in coordinate MONDO —
 * confrontare le due scale di mesh direbbe il falso.** Il glifo di [D-183] si scala con `HexSize/50` e porta
 * `RTGlyphOuterScale = 0.95` dentro la propria mesh; il prisma si scala con `PlanarScale`, che quel `0.95`
 * lo porta fuori. Il risultato è che **entrambi arrivano esattamente a `0,95 H`**: il glifo non si ferma
 * prima del bordo della cella, ci finisce sopra. Un anello spesso quanto quello del glifo (`0.0526`) lo
 * cancellerebbe quasi per intero.
 *
 * ⚠️ **Con `0.02` l'anello occupa `[0,931 H, 0,950 H]` e copre il `36%` dell'anello esterno del glifo.** È
 * una perdita accettata e non ignorata: il canale informativo di [D-183] è il **conteggio** degli anelli —
 * uno per superficie, da uno a quattro — non la larghezza del primo, e il conteggio non cambia.
 *
 * ⛔ **Questo numero è una taratura di leggibilità, e non ha un oracolo automatico.** Nessun test può dire
 * se un bordo si legge a distanza tattica: lo dice la voce `PIE-*` di #1758, guardando. Finché quella non è
 * eseguita, `0.02` è una scelta motivata — non una misura.
 *
 * ⚠️ È una frazione del RAGGIO, come `RTGlyphThickness`, e non una larghezza in uu: uno spessore assoluto si
 * è già desincronizzato in silenzio quando `HexSize` è passato da `100` a `150` ([D-163]).
 */
constexpr float RTCellBorderOuterScale = 1.0f;
constexpr float RTCellBorderThickness = 0.02f;

/**
 * Quota della griglia sopra la faccia della cella.
 *
 * ⚠️ **Sta SOPRA il glifo (`+0,3`) e non sotto**, ed è la conseguenza diretta della sovrapposizione qui
 * sopra: sotto, il glifo lo coprirebbe proprio sulle quattro superfici che ne hanno uno, e il confine
 * sparirebbe esattamente dove la board è più affollata. Sopra, il bordo si legge sempre e il glifo perde una
 * frazione del suo anello esterno — che è il verso giusto in cui pagare, perché il conteggio sopravvive.
 *
 * 🔴 **Sotto `RTCellTopZ` sarebbe dentro il prisma, e a schermo non si distinguerebbe da «non disegnato»** —
 * è successo davvero due volte, e `PIE-DEBUG-CELLS` registra la prima: il contorno di superficie stava a
 * `2.0` con la faccia a `2.5`. Dal 2026-08-28 `RTCellTopZ` è salito da `2,5` a `7,5`, quindi un numero
 * riscritto a mano oggi sbaglierebbe **di più** di allora. Deriva, non si copia.
 *
 * ⚠️ Resta **sotto** il marker di blocco (`+1,5`) e l'anteprima di pianificazione (`+2,5`): la griglia è il
 * fondo della pila di lettura, e nulla di ciò che il giocatore deve decidere le sta sotto.
 */
constexpr float RTLiftCellBorder = RTCellTopZ + 0.4f;

static_assert(RTLiftCellBorder > RTCellTopZ,
	"La griglia deve stare SOPRA la faccia del prisma, o sparisce dentro il volume opaco.");
static_assert(RTCellBorderThickness > 0.f && RTCellBorderThickness < 0.0526f,
	"L'anello di bordo deve restare piu' sottile di quello del glifo (RTGlyphThickness), o ne cancella il conteggio.");
