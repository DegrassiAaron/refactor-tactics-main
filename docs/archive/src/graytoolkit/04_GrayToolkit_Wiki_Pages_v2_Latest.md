# REFACTORTACTICS — Wiki Pack v2

> ## 🗄️ `HISTORICAL` — sorgente archiviato il **2026-08-17**
>
> **Materiale NON autorevole.** Propone **cinque** pagine Wiki; ne è stata pubblicata **una**
> ([Graybox Toolkit](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/graybox-toolkit)), perché
> le altre quattro descrivono materiale che ha già un owner nel repository. La ragione sta in
> [`D-158`](../../../decisions/RT_PDR_00_Decision_Log.md) e in [`README.md`](README.md).
>
> ⚠️ Il testo pronto della pagina UML la presenta come *«vista tecnica per sviluppatori»*: **non lo è** —
> delle classi che nomina una sola esiste in `Source/`.

## Gray Toolkit / Asset Roadmap / Asset Contract
**Data:** 2026-08-17  
**Scopo:** aggiornare le pagine Wiki usando **le ultime due immagini approvate**, distinguendo chiaramente:
- una **infografica pubblica / high level**
- una **UML tecnica per sviluppatori**, focalizzata sui graybox

---

# 0. Immagini correnti da usare

## A. Infografica pubblica
**File:** `/mnt/data/RT_GrayToolkit_Public_Infographic_v2.png`  
**Ruolo:** immagine principale per pubblico / stakeholder / overview veloce.

### Titolo consigliato
**RefactorTactics — Graybox Toolkit**

### Sottotitolo consigliato
**Infografica pubblica del kit graybox e delle regole base**

### Cosa comunica
- perché usiamo il graybox
- riferimento di scala
- kit base v0.1
- regole asset ad alto livello
- roadmap asset semplificata
- esempio di mappa graybox
- principi chiave

### Note
Questa immagine è adatta a:
- wiki overview
- documentazione high-level
- onboarding rapido
- presentazioni interne leggere

---

## B. UML tecnica per sviluppatori
**File:** `/mnt/data/RT_GrayToolkit_UML_Developer_v2.png`  
**Ruolo:** diagramma tecnico per sviluppatori / issue / documentazione di struttura.

### Titolo consigliato
**RefactorTactics — UML Graybox Toolkit**

### Sottotitolo consigliato
**Vista tecnica per sviluppatori**

### Cosa comunica
- core data layer
- graybox asset layer
- assembly / validation layer
- process / backlog layer
- asset contract
- validator
- prototype catalog
- unit scale reference
- issue / epic hooks

### Note
Questa immagine è adatta a:
- wiki tecnica
- pagine dev
- issue / epic / tracking
- documentazione di architettura graybox

---

# 1. Regola editoriale nuova

Da ora la Wiki deve separare chiaramente i due piani:

## Pagina pubblico / overview
Usa la **infografica pubblica**.

Obiettivo:
- spiegare il sistema in modo leggibile;
- non sovraccaricare con classi, layer e validator;
- mostrare solo la grammatica base del Graybox Toolkit.

## Pagina sviluppatori / tecnica
Usa la **UML tecnica**.

Obiettivo:
- descrivere componenti, contratti, validazione e backlog hooks;
- supportare implementazione, issue ed epic;
- chiarire che la UML è focalizzata sui **graybox**, non su altri sistemi del progetto.

---

# 2. Aggiornamento pagine Wiki

## Pagina 1 — `Graybox Toolkit`
**Audience:** pubblico / overview  
**Immagine principale:** `/mnt/data/RT_GrayToolkit_Public_Infographic_v2.png`

## Pagina 2 — `Graybox Toolkit UML`
**Audience:** sviluppatori  
**Immagine principale:** `/mnt/data/RT_GrayToolkit_UML_Developer_v2.png`

## Pagina 3 — `Asset Roadmap`
**Audience:** misto, ma più documentazione progettuale  
**Immagine principale:** può riusare l’infografica  
**Secondaria opzionale:** link alla pagina UML, non necessariamente embed completo

## Pagina 4 — `Asset Rules & Import Contract`
**Audience:** sviluppatori / tech art / content pipeline  
**Immagine principale:** link o embed della UML tecnica  
**Secondaria opzionale:** crop o reuse dell’infografica per la parte scala

## Pagina 5 — `Character & Environment Art Roadmap`
**Audience:** sviluppatori / art pipeline  
**Immagine principale:** nessuna obbligatoria, ma link all’infografica e alla roadmap

---

# 3. Pagina Wiki — Graybox Toolkit
**Audience:** overview / pubblico interno

## Obiettivo
Questa pagina deve spiegare **cos’è** il Graybox Toolkit e **perché esiste**, senza entrare troppo nella struttura interna class-by-class.

## Hero image
Usare l’infografica pubblica.

## Testo pronto

```md
# Graybox Toolkit

Il **Graybox Toolkit** è il linguaggio visivo e di authoring usato per costruire e validare le mappe di RefactorTactics prima dell’art finale.

![Graybox Toolkit](../images/RT_GrayToolkit_Public_Infographic_v2.png)

## Perché esiste

Il toolkit graybox serve a:

- iterare velocemente;
- testare leggibilità e proporzioni;
- allineare unità, coperture e spazi a una scala coerente;
- separare logica e presentazione;
- creare una base stabile per l’art replacement.

## Principi base

- **Geometria 3D = cosa è l’oggetto**
- **Colore / accent = stato o famiglia funzionale**
- **Trasformazione fisica = stato meccanico**
- **Overlay = stato ambientale**
- **Mesh e collisione graybox non sono authority gameplay**

## Scala di riferimento

Baseline corrente:

| Parametro | Valore |
|---|---:|
| 1 UU | 1 cm |
| Lato esagono | 150 cm |
| Flat-to-flat | ~260 cm |
| Reference Human | 180 cm |
| Unit visual footprint | ~70–80 cm |

## Kit base

Il primo kit comprende:

- Cell
- CellPlacementVolume
- Unit
- Wall
- CoverLow
- CoverHigh
- Stairs
- Door

e altri proxy di supporto come Floor, Platform, Water, Ice, Valve, Generator, HazardTank, Relay e SpawnMarker.

## Regole asset

Le regole high-level sono:

- snap coerente alla griglia/geometry grammar;
- pivot consistente;
- materiali leggibili e neutri;
- niente sbordi incontrollati oltre il volume cella;
- nomenclatura stabile;
- modularità e riutilizzo.

## Roadmap sintetica

- v0.1 → Cell + CellPlacementVolume + Unit
- v0.2 → Door e primi componenti base
- v0.4 → Props semplici e varianti
- v0.7 → Graybox multilayer
- v1.0 → Handoff verso produzione

## Vedi anche

- [Graybox Toolkit UML](./Graybox-Toolkit-UML)
- [Asset Roadmap](./Asset-Roadmap)
- [Asset Rules & Import Contract](./Asset-Rules-and-Import-Contract)
```

---

# 4. Pagina Wiki — Graybox Toolkit UML
**Audience:** sviluppatori

## Obiettivo
Questa pagina deve spiegare il modello tecnico del toolkit, con focus esplicito sui **graybox**.

## Hero image
Usare la UML tecnica.

## Testo pronto

```md
# Graybox Toolkit UML

Questa pagina descrive la struttura tecnica del **Graybox Toolkit** e i suoi punti di integrazione con dati, asset, validazione e tracking.

![UML Graybox Toolkit](../images/RT_GrayToolkit_UML_Developer_v2.png)

## Focus

La UML è focalizzata sui **graybox** e sul loro contratto tecnico, non su tutta l’architettura del progetto.

## Layer principali

### A. Core Data Layer
Comprende i dati di base della cella e del placement:

- `FRTCellId`
- `FRTCellData`
- `FRTCellPlacementVolume`

### B. Graybox Asset Layer
Definisce come i prototipi graybox vengono descritti e istanziati:

- `URTGrayboxAssetContract`
- `URTGrayboxPrototypeCatalog`
- `ARTGrayboxPrototypeActor`

### C. Assembly / Validation Layer
Gestisce assemblaggio e verifica:

- `URTGrayboxMapAssembler`
- `URTGrayboxValidator`
- `URTUnitScaleReference`

### D. Process / Backlog Layer
Collega roadmap e tracking:

- `Graybox Asset Roadmap`
- `Issue / Epic Hooks`

## Concetti chiave

### Asset Contract
Il contratto asset definisce:
- footprint;
- height class;
- cover class;
- snap policy;
- pivot policy;
- collision profile;
- material profile;
- LOD policy.

### Validator
Il validator controlla:
- overflow footprint;
- snap coerente;
- pivot coerente;
- collisioni;
- naming convention;
- coerenza di scala.

### Unit Scale Reference
Serve come riferimento per:
- unit diameter;
- cover low/high height;
- wall height.

## Regola fondamentale

Il visual graybox resta separato dalla logica autorevole del gameplay.

## Vedi anche

- [Graybox Toolkit](./Graybox-Toolkit)
- [Asset Rules & Import Contract](./Asset-Rules-and-Import-Contract)
- [Asset Roadmap](./Asset-Roadmap)
```

---

# 5. Pagina Wiki — Asset Roadmap
**Audience:** progettuale / mista

## Aggiornamento richiesto
Usare la nuova infografica come immagine principale e aggiungere un box/link verso la pagina UML tecnica.

## Nota di stile
Non usare la UML come immagine hero qui: è troppo tecnica per una pagina roadmap.

---

# 6. Pagina Wiki — Asset Rules & Import Contract
**Audience:** sviluppatori / tech art

## Aggiornamento richiesto
Questa pagina deve:
- usare la UML come immagine tecnica di supporto;
- richiamare la scala dell’infografica;
- collegare chiaramente:
  - import scale
  - actor scale
  - pivot
  - placement class
  - art replacement contract

Aggiungere una sezione iniziale:

```md
> Questa pagina approfondisce il lato tecnico del Graybox Toolkit.  
> Per una overview visuale e sintetica, vedi [Graybox Toolkit](./Graybox-Toolkit).
```

---

# 7. Pagina Wiki — Character & Environment Art Roadmap
**Audience:** art pipeline / sviluppatori

## Aggiornamento richiesto
Linkare entrambe le pagine:
- overview pubblica
- UML tecnica

Aggiungere che le lane C0–C6 e E0–E5 devono rispettare il contratto definito nel Graybox Toolkit.

---

# 8. Controlli di coerenza richiesti a Claude

## Naming
Uniformare la documentazione sul termine:
**Graybox Toolkit**

Se la repo ha già un naming canonico diverso, usare quello, ma mantenerlo coerente in tutte le pagine.

## Audience separation
Verificare che:
- la pagina overview non diventi troppo tecnica;
- la pagina UML resti chiaramente per sviluppatori;
- le roadmap non confondano infografica e UML.

## Immagini
Copiare le immagini nella posizione canonica della wiki/docs e rinominarle coerentemente se necessario.

Esempio suggerito:
- `RT_GrayToolkit_Public_Infographic_v2.png`
- `RT_GrayToolkit_UML_Developer_v2.png`

---

# 9. Output richiesto a Claude

Restituire:

## `GRAYBOX TOOLKIT WIKI REFRESH REPORT`

Con:
- pagine aggiornate;
- immagini copiate;
- pagine overview vs pagine developer distinte;
- link incrociati aggiunti;
- eventuali pagine duplicate accorpate;
- path finali delle immagini nella wiki/docs;
- commit / PR;
- prossimo passo consigliato.

---

# 10. Definition of Done

- [ ] la wiki overview usa la nuova infografica pubblica
- [ ] la wiki tecnica usa la nuova UML
- [ ] la UML è presentata come pagina per sviluppatori
- [ ] la infografica è presentata come pagina pubblica / overview
- [ ] la nomenclatura è coerente
- [ ] le pagine hanno cross-link
- [ ] la roadmap richiama l’infografica, non la sostituisce con la UML
- [ ] asset rules richiama la UML tecnica
- [ ] il report finale esplicita cosa è stato aggiornato
