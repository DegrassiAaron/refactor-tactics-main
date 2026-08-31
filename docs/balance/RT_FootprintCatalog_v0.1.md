# RT — Catalogo dei profili di footprint v0.1

> **Decisione abilitante**: [`D-303`](../decisions/RT_PDR_00_Decision_Log.md) (il footprint è un conteggio di
> settori contigui, non un raggio) · **Valori fissati da**: [`D-307`](../decisions/RT_PDR_00_Decision_Log.md)
> **Owner del modello**: [`spec-cover-placement-intra-hex.md`](../technical/systems/spec-cover-placement-intra-hex.md) §13.0
> **Consumatore previsto**: `E23.6` ([#1827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1827))

🔴 **Nessuna unità dichiara ancora un footprint.** Questo catalogo esiste come **sorgente d'autore**, nella
sede in cui questo repository tiene i numeri (`docs/balance/`), e non ha ancora un consumatore in `Source/`.
Non è una svista: è la stessa forma di [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) prima che `E6`
lo consumasse.

---

## I tre profili

| Profilo | `MinContiguousWedges` | Arco | `bRequiresFreeCore` |
|---|---|---|---|
| `Small` | **2** | 60° | `false` |
| `Medium` | **3** | 90° | `false` |
| `Large` | **4** | 120° | `false` |

Il default di `FRTFootprintProfile` resta **`1`** — l'identità, che non descrive nessuna unità: è il valore
di chi non ha ancora dichiarato un profilo, non una quarta taglia.

---

## Perché questi tre numeri, e non altri

⚠️ **Non sono tarati: sono gli unici tre interi che i vincoli lasciano.** È la ragione per cui vivono qui e
non in una revisione di bilanciamento — cambiarli richiede prima di spostare uno dei tre vincoli.

### ① Limite inferiore — **≥ 2**, dal default

`MinContiguousWedges = 1` è *«l'identità, che non decide niente»* ([`D-289`](../decisions/RT_PDR_00_Decision_Log.md)):
una cella con **un solo** settore libero sarebbe calpestabile. Un profilo a `1` non distingue nulla da
nessun profilo, quindi il primo valore che descrive una taglia è `2`.

### ② Il valore centrale — **3**, letterale in `D-289`

La frase che giustifica l'intera decisione di posa:

> *«rocce sui settori `1,2,3` più un albero su `7,8,9` lasciano **sei** settori liberi in **due gruppi da
> tre**, e uno ci sta benissimo»*

∴ un'unità standard sta in **tre** settori contigui. Non è una scelta presa qui: è la misura su cui `D-289`
ha superato `FRTOccupancyThresholds::BlockedFrom = 6`, e `Medium = 3` la rende esplicita invece di lasciarla
in un esempio.

### ③ Limite superiore — **≤ 4**, dalla geometria misurata

Un muro **diametrale** — il taglio più severo che la grammatica esprime — occupa `0xC3`, cioè i settori
`{0, 1, 6, 7}`: **quattro** settori, che lasciano **due regioni da quattro**.

🔴 **Questa misura DIPENDE da `MSE-4`, e la dipendenza è dichiarata.** Su `main` oggi un muro
diametrale accende **dodici** settori su dodici — il contatto puntuale nel centro conta come invasione —,
e con zero settori liberi **nessuno** dei tre profili starebbe in quella cella. La misura `0xC3` diventa
vera quando atterra [#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826)
([PR #2002](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2002)), che chiude `MSE-4` sull'intersezione di lunghezza non nulla e porta
`RefactorTactics.CoverPlacement.DiameterOccupiesFourSectorsOnEveryAxis` a pinnarla su ogni asse.

⚠️ **Finché quella PR non atterra, i tre valori sono corretti ma il vincolo ③ non è verificabile su
`main`.** È detto qui perché un catalogo che si legge fra sei mesi non deve far dedurre che la misura sia
sempre stata vera.

⛔ **Un profilo oltre `4` renderebbe quella cella non calpestabile**, che è esattamente la regola che `D-289`
ha superato: *«il muro divide lo spazio di posa, e dividere non è vietare»*. Il limite non è prudenza, è il
vincolo che tiene in piedi la decisione a monte.

⚠️ **`Large = 4` sta al limite esatto**, ed è dichiarato: in una cella tagliata da un diametro un'unità
grande entra, ma senza margine. Se la regola d'intersezione di `ComputeMask` diventasse più conservativa,
`Large` sarebbe il primo profilo a perdere celle — ed è il motivo per cui `MSE-4`
([#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826)) è il vicino di casa di questo
catalogo, non una questione lontana.

---

## Cosa dicono i tre profili sulle configurazioni misurate

| Configurazione | Settori liberi | `Small` (2) | `Medium` (3) | `Large` (4) |
|---|---|---|---|---|
| Cella vuota | 12, una regione | ✅ | ✅ | ✅ |
| Muro diametrale (`{0,1,6,7}` occupati) | 8, **due regioni da 4** | ✅ | ✅ | ✅ *(esatto)* |
| Due ostacoli (`{1,2,3}` e `{7,8,9}`) | 6, **due regioni da 3** | ✅ | ✅ | ❌ |
| Sei in fila (`{0..5}` occupati) | 6, **una regione da 6** | ✅ | ✅ | ✅ |

🔑 **La terza riga è il punto di avere tre taglie.** Stesso *conteggio* di settori liberi della quarta — sei —
e risposta diversa, perché ciò che decide è la **contiguità**. È la stessa dimostrazione che
`RefactorTactics.CoverPlacement.FootprintDecidesStandabilityNotTheCount` porta già in `Source/`.

---

## Cosa questo catalogo NON dichiara

- ⛔ **Nessuna relazione con l'occupancy.** Un'unità `Large` occupa **uno** slot come tutte: `Large` significa
  *«più difficile da piazzare»*, non *«più posto occupato»* — invariante di `D-289` punto (4).
- ⛔ **Nessun profilo per la transizione fra celle.** Il corridoio spazzato è `MAP-4` in
  [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md): un conteggio di settori non si trasla lungo `A→B`.
- ⛔ **Nessuna assegnazione unità → profilo.** Quale eroe sia `Medium` non si decide qui, e oggi non si decide
  affatto: i quattro eroi di [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) non dichiarano una taglia, e
  nessuna creatura non umanoide è prevista né in `roadmap-v0.1.md` né in `roadmap-post-v0.1.md`.
  ∴ **oggi userebbero tutti `Medium`**, e `Small`/`Large` sono capacità per contenuto futuro.
