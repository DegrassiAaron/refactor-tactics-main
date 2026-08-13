# `MSE-2` + `MSE-3` — spec panel sulla issue #717

> `CURRENT` · **Stato**: revisione chiusa, applicata a `#717` · **Data**: 2026-08-13
> **HEAD della revisione**: `621f5e0e`
> **Oggetto**: [`#717`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/717) come pubblicata,
> più `D-071` nel [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e
> `URTHexOccupancyLibrary` su `main`.
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Crispin
> **Particolarità**: la issue revisionata è stata **scritta la mattina stessa** dalla revisione
> precedente ([`hexgeometry-editor-spec-panel-2026-08-13.md`](hexgeometry-editor-spec-panel-2026-08-13.md)).
> Questo referto la corregge: il valore di una revisione sul proprio lavoro sta in ciò che smentisce.

---

## 1. Il verdetto in una riga

I fatti di `#717` reggono tutti, ma la sua **struttura è sbagliata**: presenta come due decisioni con otto
uscite quella che la misura dice essere **una sola domanda** — e non offre alcun criterio per chiuderla.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **4** |

---

## 2. 🔴 C1 — Le due domande sono la stessa domanda

`#717` dice che `MSE-2` e `MSE-3` «risolvono lo stesso sintomo da due lati». È più forte di così:
**entrambi i modelli trattano un contatto di misura nulla come invasione.**

| Modello | Dove il contatto è di misura nulla | Effetto |
|---|---|---|
| **B** — dodici settori | il **vertice** dell'esagono, punto in comune fra quattro triangoli di settore | 4 settori invece di 2, su un muro solo (misurato) |
| **A** — `D-071`, cerchio inscritto | la **tangenza** del cerchio al muro | vedi `C2` |

Una sola risposta — *il contatto in un punto non è invasione* — chiude entrambe. Le otto uscite elencate
sono, in gran parte, rumore intorno a questa.

---

## 3. 🔴 C2 — `D-071` letto alla lettera rende inagibile ogni cella lungo una parete

`D-071` dice, testualmente: una cella è calpestabile se il cerchio inscritto centrato sul `CellAnchor`
**non tocca** blocking geometry, con `StandardUnitClearance` = **apotema** della cella.

Misura, con `HexSize = 100`:

```text
apotema (raggio del footprint di D-071)          = 86.602540
distanza centro → punto medio del lato murato    = 86.602540
differenza                                        = 0
```

**Il muro appoggiato al lato dell'esagono è esattamente tangente al cerchio inscritto.** Alla lettera:
tocca ⇒ cella non calpestabile. Ogni cella adiacente a una parete di stanza sarebbe inagibile, e una stanza
non avrebbe celle utilizzabili lungo i propri muri.

> ⚠️ **E qui `#717` sbaglia il verso.** Il suo testo dice *«un muro appoggiato a un lato è tangente al
> cerchio inscritto — caso limite per A»*, lasciando intendere che `A` sia il modello più permissivo e `B`
> quello severo. È il contrario: `A`, letto alla lettera, dichiara la cella **non calpestabile** dove `B`
> dice `Constrained`. La issue sottostima la divergenza **e ne inverte il segno**.

Nessuno pensa che `D-071` intendesse questo — la sua stessa riga dice *«un muro che taglia solo gli
**angoli** la lascia valida, uno che entra nel **nucleo** la invalida»*, e un muro tangente non entra nel
nucleo. Ma la regola scritta non distingue **toccare** da **entrare**, ed è esattamente la distinzione che
`MSE-2` chiede per i settori.

---

## 4. 🔴 C3 — Otto uscite, nessun criterio

`#717` elenca cinque uscite per `MSE-2` e tre per `MSE-3`, e non dice **come sceglierne una**. Chi la apre
non sa cosa fare, ed è la ragione per cui una decisione resta aperta per mesi.

Il repository ha un precedente che ha funzionato: `AE-1` si è chiusa **in un giorno** perché qualcuno ha
posto una domanda falsificabile — *«c'è un turno che vorresti giocare e non puoi?»*.

L'equivalente qui, e va messo nel titolo della issue:

> ### Esiste una geometria che un designer disegnerebbe davvero, in cui il contatto in un solo punto deve rendere la cella inagibile?

Se la risposta è **no**, entrambe le decisioni si chiudono insieme, le soglie restano dove sono, e `D-071`
acquista la parola che le manca.

---

## 5. 🟠 Alti

| # | Esperto | Problema |
|---|---|---|
| **H1** | Crispin | **Il costo non è stimato — e misurato è zero.** Vedi §6: nessuno dei 19 test cadrebbe. Una decisione con costo nullo sui test, presentata come aperta e rischiosa, è una decisione che nessuno prende |
| **H2** | Fowler | **Le uscite non sono ortogonali**: 5 × 3 = 15 combinazioni nominali, molte incoerenti fra loro, e la issue non dice quali si escludono a vicenda |
| **H3** | Wiegers | **Una voce di DoD non è verificabile**: «#621 sbloccata: la sua prima riga sa quale modello cuoce». Le altre quattro lo sono |
| **H4** | Cockburn | **Nessun owner, nessuna scadenza** — lo stesso difetto contestato ai cinque handoff revisionati fra ieri e oggi |

---

## 6. Il costo, misurato

Quali dei **19** test `HexOccupancy.*` cambierebbero, adottando *«il contatto di misura nulla non è
invasione»*?

| Fixture | Geometria | Tocca un vertice? |
|---|---|---|
| 1 — segmento solido | `-20°`/`-10°`, raggio `0.3`–`0.6` | no |
| 2 — angolo | `-20°`/`10°`/`40°`, raggio `0.5` | no |
| 3 — footprint solido | quadrato centrato, mezzo-lato `20` | no |
| 4 — footprint void | quadrato a `-15°`, raggio `0.6` | no |
| 5 — segmento sul confine | `0°`, raggio `0.2`–`0.6` | no — contatto **esteso**, giace sul lato del triangolo |
| 6 — gemello fuori asse | `15°`, raggio `0.2`–`0.6` | no |
| **7 — muro perimetrale** | `-30 + 60k`, **raggio 1.0** | **sì** |

Le prime quattro fixture stanno a raggio `0.3`–`0.6` con angoli non multipli di 30: nessuna arriva ai
vertici, che stanno a raggio pieno. `GoldenExampleFromTheSource` non usa geometria affatto — costruisce la
maschera a mano (`0b001111000100`).

L'unica fixture con contatto puntuale è il muro perimetrale, usata dal solo
`PerimeterWallsOccupancyIsRecorded`, che asserisce **monotonia** e **sottoinsieme** — entrambe vere per
costruzione e **indipendenti dai valori**. I numeri che stampa cambierebbero; le sue asserzioni no.

> ✅ **Nessuno dei 19 test cadrebbe.** Il che era prevedibile e va detto lo stesso: le quattro fixture
> originali evitavano i multipli di 30 *di proposito*, quindi la suite è cieca proprio al caso in
> discussione. «Zero test cadono» qui significa «la suite non copre la scelta», non «la scelta è innocua».

---

## 7. Cosa resta una scelta d'autore

Ridotte le uscite a due:

| | Uscita | Conseguenza |
|---|---|---|
| **1** | **Il contatto di misura nulla non è invasione**, in entrambi i modelli | `2N` settori per `N` muri consecutivi: 1 muro `Free`, 2 `Constrained`, 3 `Blocked`. `D-071` distingue *toccare* da *entrare*. Soglie invariate |
| **2** | **Si accetta, e lo si dichiara** in entrambi i modelli | Le soglie vanno ritarate (`BlockedFrom` almeno a 8), e `D-071` va riscritta per dire che la tangenza invalida |

⚠️ Non la chiude questa revisione: `D-071` è una decisione d'autore registrata nel Decision Log e spiegata
al giocatore nella pagina Wiki *Griglia esagonale e geometria del mondo*. Cambiarne la lettura è design.

**Innesco invariato**: [`#621`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621), il primo
codice che deve scegliere.
