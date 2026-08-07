# Brief — Ghiaccio: cosa è già vigente, cosa resta fuori

> **Stato**: brief di requisiti · **Data**: 2026-08-07 · **Origine**: `/sc:brainstorm` su
> `docs/src/RefactorTactics — Implementazione terreno Ghiaccio v0.1 in Unreal Engine 5.md` (36 sezioni)
> **Esito**: lo **scivolamento base resta in v0.1 perché è già implementato**; il **motore** (Momentum,
> Traction, Prone, collisioni a catena, integrità, rottura, ponti) è **fuori**, epic post-release.
> **Autorità**: subordinato a [`piano-canonico-mvp.md`](piano-canonico-mvp.md) e
> [`RT_TerrainCatalog_v0.1.md`](balance/RT_TerrainCatalog_v0.1.md).

## 1. La sorpresa: lo slide c'è già

Il catalogo terreni dichiara lo scivolamento **rimandabile** («introduce un movimento non pianificato dentro
la risoluzione») e la roadmap lo segna come *opzionale*. La misura sul repository dice altro:

| Test | Cosa garantisce |
|---|---|
| `Terrain.Ice.SlidesWithSufficientBudget` | chi entra con ≥ 2 MP residui scivola di 1 cella nella direzione d'ingresso |
| `Terrain.Ice.SlideBudgetBoundaryIsExactlyTwo` | la soglia è **esattamente** 2, non «più di 1» |
| `Terrain.Ice.BlockedCellStopsSliding` | una cella bloccata impedisce lo scivolamento |
| `Terrain.Ice.SlidesInMatch` | funziona nel turno reale, non solo in isolamento |

**Conseguenza**: la regola non va né costruita né rimossa. Va **promossa da "opzionale" a "vigente"** nel
catalogo e nella roadmap — è l'unico caso in cui la documentazione è più conservativa del codice.

## 2. Cosa propone il documento sorgente

Un sistema molto più ampio, in 5 blocchi:

| Blocco | Contenuto | Verdetto |
|---|---|---|
| **Superfici e stati** | `Ice`, `CrackedIce`, `Water`, `Steam`, `ElectrifiedWater` con state machine | 🟡 parziale: `Ice`, `ShallowWater`, `Smoke` esistono; `CrackedIce`, `Steam`, `ElectrifiedWater` no |
| **Fisica del movimento** | Momentum, Traction, Stability, Slide esteso, cambio di direzione, pendenze | ⏳ **fuori**: è un motore, non un terreno |
| **Stati dell'unità** | `Unbalanced`, `Prone`, `Sliding` | ⏳ fuori |
| **Integrità e rottura** | integrità del ghiaccio, crepe, rottura, caduta in acqua, ponti di ghiaccio | ⏳ fuori |
| **Interazioni termiche** | Heat/Melt/Freeze, fuoco↔ghiaccio, steam, vento | 🟡 parziale: `Terrain.Fire.*` esiste; le transizioni di stato no |

## 3. Due conflitti col canone

**C1 — L'ordine delle fasi è sbagliato.** §27 propone
`effects → movement → control → attacks → environment → cleanup`, cioè il **movimento prima degli attacchi**.
Il canone ha il **Move dopo il Blast** (ADR-0003 §3, ribadito come *non superato*). È la stessa rimappatura
che l'ADR ha già risolto in senso opposto: chi implementasse §27 alla lettera invertirebbe il turno.

> **Ordine corretto per le meccaniche del ghiaccio**: lo slide è movimento forzato → risolve dentro la fase
> **Move**, dopo il Blast. Le transizioni ambientali (melt, freeze, steam) risolvono nel **Cleanup**, prima
> dei KO, come già stabilito per gli effetti ambientali (roadmap E8).

**C2 — Il momentum contraddice «raccogli poi applica».** Uno slide che provoca una collisione che provoca un
altro slide richiede un resolver **a punto fisso**: si itera finché lo stato non cambia più. È possibile in
modo deterministico, ma è un motore nuovo, non un'estensione — e nessuna batteria di casi ne copre lo spazio
di stato. Se rientrasse in scope, rientrerebbe con esso il **fuzzing deterministico**, valutato e scartato in
[roadmap-v0.1 §5 CP 12.6](roadmap-v0.1.md).

## 4. Cosa entra nella v0.1

Nulla di nuovo dal documento. Restano gli impegni già presi:

- lo **slide base** già implementato (§1), promosso a regola vigente nel catalogo;
- `Terrain.Ice` costo 1 MP, come da catalogo;
- **CP 8.3** propagazione elettrica su acqua (max 3 celle, ogni unità colpita una volta) — ancora assente;
- **CP 8.4** l'acqua rimuove il fuoco e cancella `Burning` — ancora assente.

Il documento sorgente **è però utile subito** su un punto: i suoi §33 (12 test) e §34 (casi limite) sono la
migliore lista di verifica esistente per CP 8.3/8.4, e i casi limite «due slide contrapposti», «ghiaccio che
si rompe sotto due unità», «forced movement su cambio Layer» vanno **documentati come non gestiti** prima
che qualcuno li scopra in partita.

## 5. L'epic rinviata — «Motore ambientale del ghiaccio»

Da aprire **dopo** la v0.1, con questo perimetro:

| CP | Contenuto |
|---|---|
| G.1 | `CrackedIce` + integrità + rottura + caduta in acqua |
| G.2 | Traction/Stability per eroe; `Unbalanced` e `Prone` |
| G.3 | Momentum e slide esteso con resolver a punto fisso + **fuzzing deterministico** |
| G.4 | Transizioni termiche: Heat/Melt/Freeze, `Steam` con opacità (si innesta su E13: lo steam è occultamento) |
| G.5 | Ponti di ghiaccio: creazione, attraversamento, distruzione, revisione del grafo |

**Prerequisito dichiarato**: G.3 non parte senza il fuzzing, e G.4 non parte prima di E13 (lo steam è un
modificatore di visibilità, e il modello della conoscenza deve esistere prima dei suoi modificatori).

## 6. Il collegamento con il rumore

Il §10 del documento sorgente e il §10 del [documento sul rumore](../src/RefactorTactics_Rumore_Claude.md)
descrivono la stessa combo da due lati:

```
GHIACCIO ──(fuoco)──> ACQUA ──(freddo)──> GHIACCIO
                       │
                       └──(elettricità)──> ACQUA ELETTRIFICATA
                       │
                       └──(calore)──────> VAPORE  →  occulta la vista, non l'udito
```

È l'esempio migliore del perché la **query di propagazione** vada costruita una volta sola
([brief-conoscenza-parziale §12](brief-conoscenza-parziale.md)): calore, elettricità e suono percorrono lo
stesso grafo con costi diversi. Il contrasto «la zona diventa difficile da vedere ma facile da interpretare
acusticamente» è gameplay reale, e nasce gratis se i due canali condividono l'infrastruttura.

## 7. Domande aperte

1. **`CrackedIce` in v0.1?** Il catalogo terreni dichiara 8 superfici e `CrackedIce` non è fra queste.
   Aggiungerla ora significa toccare il formato dell'asset mappa (come per il campo cover, CP 9.1).
2. **Divergenza sul rumore dell'acqua**: il workbook dà `Noise_Mod 4` (rumorosa), il documento rumore `+2`.
   Va scelto un valore prima di CP 13.3.
3. **Casi limite non gestiti**: si documentano come «non gestiti» o si scrivono test che ne fissano il
   comportamento attuale (caratterizzazione)? La seconda costa poco e impedisce regressioni silenziose.
