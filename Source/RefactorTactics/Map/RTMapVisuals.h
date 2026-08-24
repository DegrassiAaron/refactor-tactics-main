#pragma once

/**
 * Le misure del DISCO che rappresenta una cella, in un posto che si può includere (#983).
 *
 * 🔴 **Non sono un dettaglio di rendering: chi disegna qualcosa SOTTO `RTCellTopZ` lo disegna dentro un
 * cilindro opaco, e a schermo non si distingue da qualcosa che non è stato disegnato affatto.** È successo
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

/** Quanto il prisma è appiattito in Z: è ciò che rende il disco un disco invece che una colonna. */
constexpr float RTCellFlatScale = 0.05f;

/**
 * Quota della faccia SUPERIORE del disco sopra il centro della cella: **2,5 uu**.
 *
 * ⚠️ Il prodotto si scrive con `RTCellPrismRadius` e non col letterale `50`, che è ciò che questa riga
 * faceva pur avendo la costante due dichiarazioni più sotto nello stesso namespace — una terza copia dello
 * stesso numero dentro lo stesso file.
 */
constexpr float RTCellTopZ = RTCellPrismRadius * RTCellFlatScale;
