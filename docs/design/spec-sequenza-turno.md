# Spec — Sequenza di Risoluzione del Turno: reazioni, reveal, ordinamento deterministico

> Prodotta da un panel di revisione specifiche (`/sc:spec-panel`) il **2026-08-02**.
> Consolida il design esplorativo [`sequenza-turno.md`](sequenza-turno.md) (trascrizione leggibile del
> PDF *«RefactorTactics – Sequenza di Risoluzione del Turno»*).
> Autorità: **subordinata** a [`piano-canonico-mvp.md`](piano-canonico-mvp.md) (fonte di verità #1). Il grosso
> di questo modello è **north-star** (post-MVP): **non modifica l'MVP**. Convive con la riconciliazione fasi già
> decisa in [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) §0 (R1).

## 0. Cos'è il documento sorgente

`sequenza-turno.md` è una **sintesi di ricerca/design** (lo dichiara: ispirata a *Phantom Brigade* e *Magic: The
Gathering*), non una specifica operativa. Descrive un modello di risoluzione molto più ricco dell'MVP:

```
Pianificazione segreta (WEGO ~30s) → Reveal progressivo → Finestre di Reazione (stack LIFO)
  → Risoluzione (ordine APNAP) → Cleanup
```

con 5 categorie di velocità, tre code (Command/Reaction/State), *Patch* dinamici e un modello dati JSON.
Va usato come **input north-star**, non come sorgente di decisioni. Questa spec lo classifica e ne estrae la
sola parte adottabile a breve.

## 1. Classificazione — cosa entra, cosa no

| Concetto del sorgente | Stato vs canone | Esito |
|---|---|---|
| Pianificazione segreta WEGO ~30s | ✅ già MVP (invariante #6; timer 30s) | **Compatibile as-is** |
| Ordine deterministico degli effetti simultanei (APNAP + tie-break totale) | ✅ **rafforza l'invariante #4** | **Recepito** nel canone §5.1 (`FR-RESOLVE-01..03`, dettaglio §3) |
| Politiche di fallback/reazione **pre-committed** (funzioni pure sullo snapshot) | 🟡 compatibile con "raccogli poi applica" | Adottabile, **gated** da una ragione di gameplay |
| Reveal a **livelli** (generico → elemento → bersaglio) | 🟡 elaborazione del binario `Status.Reveal` (M3.4) | North-star; compatibile in spirito |
| **Finestre di reazione live** (popup 3s, input a metà esecuzione) | ~~❌ conflitto con l'invariante #3~~ → 🟡 **riconciliabile per composizione** | **In scope dal 2026-08-07** con la via **(b)** di C1 — vedi [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md); richiede **ADR-0004** |
| **Reaction Stack LIFO** interattivo | 🟡 l'*ordine* LIFO sì; lo *stack live* no | Adotta l'ordine come regola pura; differisci l'interattivo |
| 5 categorie di velocità (Immediate/Reaction/Fast/Standard/Slow) + `timing` (`EndOfPhase`…) | ❌ non nel canone; mal si sposa col batch a 4 fasi | North-star (§3) |
| **Patch** (modificatori dinamici di un'abilità in corso) | 🟡 potente per combo, amplia il non-determinismo | North-star (§3) |
| Modello dati **JSON** per azioni/effetti | ❌ conflitto con `URTAbilityData : UPrimaryDataAsset` (no JSON) | **Rigetta** come sorgente abilità (§4); JSON solo per il modding [canone §8.1] |
| ≤4 unità/squadra · 2 RP/squadra · timeline/camera UI | ➖ scala e presentazione | North-star; l'MVP resta **2v2** senza RP |
| Esecuzione real-time 45–60s | ❌ l'MVP risolve in modo istantaneo/deterministico | North-star |

> Lo schema fasi del sorgente (`Reveal → Reazione → Risoluzione`) **non** sostituisce il canone
> `Planning → Prep → Dash → Blast → Move → Cleanup`: è un'elaborazione **mappabile** su di esso — Blast/Move
> restano il punto di risoluzione; il layer reazioni si innesta sui checkpoint interni (`OnHit`, `OnMove`…).
> (Decisione già presa: [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) §0 R1.)

## 2. Conflitti con gli invarianti (da NON risolvere in silenzio)

Per `CLAUDE.md` (conflitto tra fonti → segnala, non sovrascrivere una decisione approvata). Tre conflitti
*load-bearing*, con la riconciliazione proposta:

- **C1 — Purezza del resolver (invariante #3)** vs finestre di reazione live + timeline di esecuzione 45–60s.
  Il resolver MVP è **"raccogli poi applica"**: snapshot a inizio fase, niente `Delay`/timeline/montage,
  l'ordine dell'array non cambia l'esito. Uno stack di reazioni che accetta **input umano durante l'esecuzione**
  è una risoluzione interattiva in tempo reale → **incompatibile** col resolver.
  **Riconciliazione:** tenere il resolver puro. Le reazioni entrano solo come
  (a) **politiche pre-committed** valutate come funzioni pure dello snapshot, oppure
  (b) north-star: ogni finestra apre un **nuovo round di sotto-risoluzione deterministico** (modello a
  *priority passes* stile MTG) — **mai** un `Delay` dentro il resolver.

  > ✅ **Risolto il 2026-08-07 con la via (b)**, alla luce di
  > `docs/src/RefactorTactics_Overwatch_FastReaction_Claude.md` (che non esisteva quando questa spec è stata
  > scritta). La formulazione operativa è in [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md):
  > il turno diventa una **sequenza di sotto-risoluzioni**, ciascuna «raccogli poi applica» con snapshot
  > proprio. L'invariante #3 **si compone, non si deroga** — nessun `Delay` entra nel resolver, che si ferma
  > su un *decision boundary* e riparte con l'input come **dato**. Restano vere le due condizioni di C2: il
  > default allo scadere è una funzione pura (`Timeout → HOLD`) e l'esito non dipende da *quando* arriva il
  > click. ✅ **Formalizzato in [ADR-0004](adr-0004-finestre-di-reazione.md)** (2026-08-07): l'invariante #3
  > del canone §5 è riformulato — «snapshot a inizio **segmento**», dove un segmento è delimitato da una
  > macro-fase **o** da un decision boundary. **C1 è chiuso.**

- **C2 — Determinismo (invariante #4)** vs timing da orologio delle finestre (3s) e timing dell'input umano.
  **Riconciliazione:** l'esito deve dipendere **solo** da *se* una reazione pre-committed scatta (condizioni sul
  trigger), **mai** da *quando* nei 3s arriva il click. Il default allo scadere è **funzione pura dello stato**,
  non del wall-clock. Ogni RNG usa seed/stream espliciti; il log eventi è versionato → **golden-hash**.

- **C3 — Modello dati** (JSON del sorgente) vs decisione canonica **`URTAbilityData : UPrimaryDataAsset`
  (no JSON)**. **Riconciliazione:** i campi `speed/timing/trigger/conditions/effects/priority` diventano
  **proprietà di `URTAbilityData`**, non uno schema JSON parallelo. Lo schema JSON resta confinato al **layer di
  modding** north-star (piano canonico §8.1, «3 schemi JSON»).

## 3. Adottabile ora (basso rischio, alto valore): ordinamento deterministico degli effetti simultanei

**È l'unica estrazione a breve.** È un incremento del comportamento dei resolver, **recepito nel piano
canonico** ([`piano-canonico-mvp.md`](piano-canonico-mvp.md) §5.1, 2026-08-02) come `FR-RESOLVE-01..03` — sul
modello delle riconciliazioni R1/R2/R3 del pathfinding.

**Ordine totale APNAP-adattato** per un insieme di effetti simultanei:

1. Effetti di sistema (**State**: morti a HP≤0, scadenza status) — *già fatto post-Blast, da formalizzare*
2. Unità **attiva**
3. **Alleati** dell'attivo
4. **Avversari**
5. **Terreno**/oggetti
6. Effetti **globali** di scenario

Intra-gruppo: **velocità → priorità (intera) → tie-break assoluto** (id unità / coord stabile, come lo
`StableTieBreak` del pathfinding) — così due effetti pari **non** dipendono mai dall'ordine dell'array.

**Requisiti (SMART):**
- `FR-RESOLVE-01` — dato un insieme di effetti simultanei, il resolver produce un **ordine totale**
  deterministico secondo i 6 gruppi APNAP + tie-break assoluto. Generalizza l'attuale "danni sommati per
  bersaglio" ai casi in cui l'ordine **conta** (scudo/buff/reazione prima del danno).
  *Verifica: permutare l'array di input non cambia il log eventi (estende l'invariante #3 agli effetti
  ordine-dipendenti).*
- `FR-RESOLVE-02` — le **State-Based Actions** (morte, scadenza status) sono controllate **fra un effetto e il
  successivo**, non solo a fine fase; un bersaglio morto **invalida** gli effetti pendenti che lo riguardano.
- `FR-RESOLVE-03` — nessun **float** nell'ordinamento/hash (invariante #4); priorità intere.

**Esempio (Given/When/Then):**
```
Given  A e B attaccano simultaneamente C (HP 30, scudo 20); A infligge 25, B infligge 25
       C ha una reazione pre-committed "Barriera" (+20 scudo) con trigger OnTargeted
When   il resolver ordina gli effetti (APNAP: la reazione del difensore C precede i danni degli avversari)
Then   "Barriera" risolve per prima (scudo 20 -> 40)
And    danno totale 50 sullo scudo 40 -> 40 assorbiti, 10 agli HP -> C a HP 20
And    permutando l'input [A,B] o [B,A] l'esito e' identico (FR-RESOLVE-01)

Given  A (25) e B (25) colpiscono C (HP 30, scudo 0); nessuna reazione
When   il resolver applica il gruppo "avversari"
Then   C muore (HP<=0); una eventuale reazione di C "OnLowHealth" NON scatta piu' (FR-RESOLVE-02: SBA)
```

**Test plan:** (1) ordine-indipendenza col tie-break su effetti ordine-dipendenti *(discriminante)*;
(2) SBA fra effetti invalidano i target pendenti; (3) determinismo con priorità pari (tie-break assoluto);
(4) assenza di float nel percorso di ordinamento/hash.

## 4. North-star (gated): sistema di reazioni completo

**Gate — NON implementare finché non esistono entrambi:**
- **(a)** una **ragione di gameplay** che richieda reazioni interattive (combo emergenti, contatori, *Patch*);
- **(b)** il **multiplayer** — o comunque un design che preservi la purezza del resolver (C1/C2 risolti).
  In **2v2 vs bot** c'è un solo attore umano: il bot necessita politiche pre-committed comunque → **l'intero
  valore interattivo emerge solo col PvP**. Costruirlo prima è YAGNI.

Componenti del sorgente, con nota di riconciliazione:

| Componente | Nota |
|---|---|
| **Reaction Stack LIFO** | Adotta l'**ordine** LIFO come regola pura (§2); differisci lo *stack interattivo*. |
| **Command / Reaction / State Queue** | Command = piani (esiste); State = SBA (parziale, §2 FR-RESOLVE-02); Reaction = north-star. |
| **5 categorie di velocità** + `timing` (`Now/EndOfAction/EndOfPhase`) | North-star. `Slow`/`EndOfPhase` mal si sposano col batch a 4 fasi → richiedono il modello a *priority passes*. |
| **Patch** (modificatori dinamici) | North-star. Potente per combo emergenti ma amplia la superficie di non-determinismo → richiede C1/C2. |
| **Reveal a livelli** (generico→elemento→bersaglio) | Elaborazione north-star del binario `Status.Reveal` già presente (M3.4, invariante #6). Compatibile in spirito; da definire *cosa mostra ogni livello* per verificare la privacy. |
| **Budget UX** (2 RP/squadra, stack depth 5, 2–3 finestre, timer 3s, ≤4 unità) | Parametri north-star; la scala MVP resta 2v2 senza RP. |
| **Timeline UI / camera / accelerazione** | Presentazione (Blueprint), post-MVP. |

**Failure mode da progettare col north-star** (Nygard):
- **Overflow stack**: regola di overflow **deterministica** (non "posticipa" vago).
- **Timeout finestra**: default = **funzione pura dello stato** (C2).
- **Bersaglio dangling** (morto in-risoluzione): invalidato dalle SBA (FR-RESOLVE-02).
- **Loop di reazioni** (A↔B): rotto dal limite di profondità + tie-break.

## 5. Non-goal / YAGNI (esplicito)

- Niente **reaction stack interattivo**, niente **finestre live**, niente **timeline di esecuzione** nell'MVP.
- Niente **schema JSON** come sorgente delle abilità (solo modding north-star).
- Niente **categorie di velocità / `EndOfPhase`** finché il resolver non adotta un modello a *priority passes*.
- Il valore interattivo è **PvP**: non costruirlo per il 2v2-vs-bot.

## 6. Sintesi del panel (critique)

| Esperto | Rilievo principale | Raccomandazione |
|---|---|---|
| **Wiegers** (requisiti) | Suggerimenti, non requisiti; niente criteri misurabili; niente tracciabilità agli invarianti | Distinguere MVP/north-star; `SHALL` + accettazione solo per §2 |
| **Adzic** (esempi) | Esempi narrativi, non eseguibili | Given/When/Then con stato-finale asserito (fatto in §2) |
| **Cockburn** (casi d'uso) | Attore primario delle reazioni non dichiarato; scope 2v2-vs-bot implicito | Reazioni interattive = feature **PvP** |
| **Fowler** (architettura) | Accoppia policy (LIFO) e timing (finestre); JSON duplica `URTAbilityData` | Separare ordine/timing (§2 vs §4); campi su URTAbilityData (§4/C3) |
| **Hohpe** (ordinamento) | APNAP è il valore forte, allineato all'invariante #4; tie-break non totale | Formalizzare l'ordine totale + tie-break assoluto (§2) |
| **Nygard** (failure) | Overflow/timeout/dangling vaghi | Regole deterministiche (§4) |
| **Crispin** (test) | Nessun test di determinismo; il timing è superficie di non-determinismo | Golden-hash; il *timing* non influenza l'esito (§2, C2) |

## Appendice — sorgente e nota di collocazione

Sorgente: [`sequenza-turno.md`](sequenza-turno.md) (trascrizione del PDF omonimo).
**Nota storica:** il sorgente viveva nella radice `docs/`; spostato in `docs/design/` il **2026-08-02**
(convenzione `CLAUDE.md`: i design nella sottocartella pertinente) contestualmente a questo consolidamento.
Conserva dettaglio non ripreso qui: snippet JSON, diagramma mermaid, tabelle UX e valori di default.
