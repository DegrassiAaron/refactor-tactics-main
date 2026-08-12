# Lane 3 — Client / UX

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `59fa6f8a` (riallineato al merge)
> **Cosa è**: la sequenza di lavoro della lane *Client / UX*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub; il `⏳`/`✅` qui dentro è la
> fotografia di una data. In caso di divergenza **vince GitHub**.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

Ciò che il giocatore **vede e fa**: HUD, camera, input, selezione, ghost, combat log come vista —
tutto ciò che vive nello **spazio schermo**. Non chi calcola il risultato (lane 2), non chi sta sulla
cella (lane 6).

**Invariante che questa lane non può violare**: la classificazione la calcola il **simulatore**, non
la UI, e nessun intento avversario si mostra né si replica — l'occultamento **non è grafico**.

---

## Il primo anello sblocca l'unico P0 fermo

### `#77` · CP 11.1 — HUD di partita completo 🟢 **pronta** · **P1**

**Epic**: `#25` (E11) · **Dipende da**: `#45` ✅ · `#59` ✅ — entrambe chiuse, il checkpoint è
eseguibile

*«Tutto quello che serve per giocare un turno senza indovinare»*: barre HP, scudo, energia, e il
resto del DoD.

**Sblocca tre rami**: `#78` (P0), `#79`, `#172`.

---

## Sequenza

### Catena A — HUD, log e debug (E11, `#25`)

```text
#77 ──┬──> #78 (P0) ──┐
CP 11.1│    CP 11.2    ├──> #173
      │                │    CP 11.6
      ├──> #172 ───────┘
      │    CP 11.5
      └──> #79 ──> #80
           CP 11.3  CP 11.4
```

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#77` | CP 11.1 — HUD di partita completo | 🟢 **pronta** | `#45` ✅ · `#59` ✅ | P1 |
| `#78` | CP 11.2 — Intenti alleati con livello di certezza | ⏳ bloccata | `#53` ✅ · `#77` | **P0** |
| `#79` | CP 11.3 — Combat log con reason code completi | ⏳ bloccata | `#77` | P1 |
| `#80` | CP 11.4 — Comandi `rt.Debug.*` | ⏳ bloccata | `#79` | P1 |
| `#172` | CP 11.5 — Ghost Timeline: preview del piano per fase | ⏳ bloccata | `#77` | P1 |
| `#173` | CP 11.6 — Scrubbing delle fasi e ramo condizionale della reaction | ⏳ bloccata | CP 11.5 · `#78` | P1 |

⚠️ **`#78` è l'unico P0 della release fermo per una dipendenza, e la dipendenza è `#77`.** Tre
livelli — *confermato*, *previsto*, *incerto* — calcolati dal simulatore, mai dalla UI.

### Catena B — Presentazione (E21, `#286`) → **spostata alla lane 6**

⚠️ **Dal 2026-08-12 `#287`–`#289` vivono in [`roadmap_lane_6.md`](roadmap_lane_6.md)**, insieme al bug
`#593` che le blocca di fatto. Il criterio non è «personaggi vs interfaccia» ma **dove vive il pixel**:
questa lane possiede lo **schermo** (HUD, camera, input, ghost, log), la lane 6 il **mondo** — chi sta
sulla cella e come si presenta.

Restano qui i due legami, che non si spostano:

- **E21 è una dipendenza della lane 1**: finché non atterra, il KPI `FPS client` di `#41` è un valore
  pre-presentazione — e la Definition of Done lo dichiara ⛔ *non misurabile prima di E21*.
- **`#289` (leggibilità tattica) tocca entrambe**: gli anelli team/selezione stanno nel mondo, ma il
  giudizio «si capisce cosa succede» si dà guardando lo schermo intero.

### Catena C — HUD Icon Language (E20, `#217`)

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#219` | CP 20.2 — Categorie della v0.1 | 🟢 **pronta** | — | P2 |
| `#220` | CP 20.3 — I widget consumano il catalogo | ⏳ bloccata | `#219` | P2 |

⚠️ Il registry lo dice esplicitamente per `RT-FEAT-UI-ICON-LANGUAGE`: le icone sono un **catalogo
semantico**, non texture nei widget — *«va fatto mentre E11 costruisce l'HUD»*, cioè in parallelo
alla catena A, non dopo.

### Voci singole

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| `#291` | La rotazione dichiarata: cablaggio fatto, resta l'**input** (E11) | 🟢 **pronta** — `bug` | P2 |
| `#224` | CP 17.3 · Leggibilità con otto unità | ⏳ dopo E17 (lane 4) | P3 |

⚠️ **`#583` potrebbe essere lavoro di questa lane** (è elencata nella lane 2, dove sta il suo
consumatore `#163`): `D-109` ammette una condizione dichiarata in planning e **nessuno la scrive**.
Il produttore può essere un comando, il bot — lane 2 — oppure **l'interfaccia di planning**, che è
qui. La scelta va fatta prima di aprire il codice, non dedotta dopo.

⚠️ Stessa forma di `#291`, che è già in questa lane per la stessa ragione: *«cablaggio fatto, resta
l'input»*. Una rotazione dichiarabile che nessun input dichiara è un dato senza produttore.

---

## Ordine consigliato

1. **`#77`** — è il collo di bottiglia della lane e dell'unico P0 fermo.
2. **`#78`** — appena `#77` chiude.
3. `#219` insieme alla catena A, come dice il registry.
4. `#291` — è un bug piccolo con il cablaggio già fatto.

⚠️ `#287` era il terzo posto di questa lista fino al 2026-08-12: ora è della **lane 6**, e lì è
preceduta da `#593`.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| **E21** (`#287`–`#289`) → **lane 6** | `#41` (lane 1) | `FPS client` non è stabile prima |
| `#160` (lane 2) | HUD conoscenza parziale | metà checkpoint è di questa lane |
| `#173` | `#512` (lane 2) | il ramo condizionale della reaction esiste se le decisioni di finestra sono un dato |
