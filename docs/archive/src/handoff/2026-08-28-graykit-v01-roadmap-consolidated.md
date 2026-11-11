# RefactorTactics — GrayKit v0.1 Roadmap CONSOLIDATA

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-28 · **Base**:
> `483e031a` (`main`).
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Il file stava alla radice del repository come
> `RefactorTactics_GrayKit_v0.1_Roadmap_CONSOLIDATED_2026-08-28.md`, untracked, 360 righe. Il suo **mandato
> di applicazione** è archiviato accanto, in
> [`2026-08-28-graykit-v01-apply-mandate.md`](2026-08-28-graykit-v01-apply-mandate.md): i due si leggono
> insieme e sono stati revisionati insieme.
>
> **Cosa possiede**: le decisioni d'autore GrayKit del 2026-08-28 — scala, occupancy, geometria interna,
> proxy FoW — e la roadmap a sei nodi, verbatim.
> **Cosa non possiede**: nessuna autorità, e nessuna esecuzione. Il referto completo — misura per misura —
> è [`../../../roadmap/plans/graykit-v01-consolidation-spec-panel-2026-08-28.md`](../../../roadmap/plans/graykit-v01-consolidation-spec-panel-2026-08-28.md).
>
> ✅ **La sua §0 ha chiesto questa archiviazione**, ed è la ragione per cui è qui: *«questa roadmap non deve
> diventare un nuovo owner permanente […] dopo l'integrazione, questo file va archiviato»*.

## Il verdetto, in breve

✅ **Disciplinato sull'ownership, e accurato sulla baseline.** La §0 è la sezione migliore dei due file. Le
sei issue dichiarate «già chiuse — non rifare» sono **6 su 6 verificate** CLOSED lato server; i quattro
owner documentali citati esistono tutti; le otto decisioni citate (`D-146` `D-163` `D-171` `D-172` `D-173`
`D-179` `D-183` `D-189`) sono tutte nel Decision Log. Il rilievo su
[`#324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) regge (titolo `[EPIC v0.2]`,
label `v0.1`), e l'osservazione che l'occupancy 12+core **non** è nella spec di placement è lavoro reale
(`grep` → **0**).

🔴 **Ma inverte la decisione centrale del proprio nodo 4.** `D-225` è **accettata** il 2026-08-28 e sceglie
il **NASCONDIMENTO**, dichiarando *«rende necessaria l'intera famiglia `GB_FOW_*`, `CellFull` incluso»*. Il
kit la tratta come pendente e prescrive l'opposto: *«non modellare `GB_FOW_CellFull` finché la decisione di
scope non abilita il vero nascondimento»*. Eseguirlo alla lettera **omette** l'artefatto che la decisione
rende obbligatorio — e lo fa con una giustificazione scritta accanto. «`D-225` riservata nel thread» è
inoltre scaduto: l'ultimo assegnato è **`D-233`**.

🔴 **E due dei sei nodi della sua roadmap sono issue chiuse**:
[`#956`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/956) (nodo 1, board readability) e
[`#1467`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1467) (nodo 4, team knowledge) sono
**CLOSED**, come pure `#1094`. L'audit ha misurato ciò che dichiarava acquisito e assunto ciò su cui manda a
lavorare.

🔴 Altri due critici: il **lavoro vero** del nodo 4 — mappa inversa cella→istanza e aggiornamento
per-istanza a runtime, che `D-225` dichiara *«non stimato»* — non compare in nessuno dei due kit; e
**`0.88 H` è un numero nuovo e scelto** (`grep "0.88"` su `docs/` → zero occorrenze di testo) mentre `D-225`
si chiude con *«Zero numeri nuovi»*.

🟠 Fra gli alti: la granularità d'anchor che il kit dice di non inventare **è già decisa**
(`RT_GeometryQuanta = 12`, `Map/RTGeometryGrammar.h`); `FRTGeometrySegment` ha **sei** campi e non quattro —
l'omesso è `Layer`, in un gioco hex multilivello; `GEO-4` esiste già completa e porta un collegamento a
`#1392` che i kit non hanno.

---


**Data:** 2026-08-28  
**Stato:** handoff di consolidamento pronto da applicare agli owner canonici  
**Obiettivo:** integrare la roadmap GrayKit v0.1 nella roadmap e nelle spec vive del repository, senza creare una seconda source of truth.

---

## 0. Regola di ownership

Questa roadmap **non deve diventare un nuovo owner permanente**.

Owner canonici:

- `docs/roadmap/roadmap-v0.1.md` — vista release v0.1;
- `docs/technical/systems/spec-graybox-placement-contract.md` — contratto visuale/authoring GrayKit;
- `docs/technical/systems/spec-hex-geometry-authoring.md` — geometria quantizzata, occupancy e bake;
- spec Team Knowledge / LOS — semantica visibility/Fog of War;
- GitHub issue — esecuzione e DoD.

Dopo l'integrazione, questo file va archiviato o eliminato dal percorso operativo.

---

# 1. Decisioni consolidate della chat

## 1.1 Cella e scala

- esagono regolare;
- `HexSize = 150 cm` = lato/circumraggio;
- `LayerHeight = H = 250 cm` resta invariato;
- il GrayKit usa misure relative dove il contratto già le richiede.

## 1.2 Occupancy

- occupancy = **12 settori da 30° + core**;
- i 12 settori sono una misura di invasione della cella, **non 12 direzioni di movimento**;
- movimento/facing/bordi restano sulle **6 direzioni tattiche**;
- volume/footprint chiuso può alimentare occupancy;
- muro/polilinea aperta **non diventa occupancy** solo perché attraversa settori.

## 1.3 Geometria strutturale

Muri, cover e porte possono essere rappresentati dalla geometria quantizzata del Tactical Geometry Authoring.

Nuovo requisito d'autore da consolidare:

> una struttura può seguire un segmento che parte dal **centro della cella** e termina su un **anchor quantizzato sul lato/perimetro** (`Center → SideAnchor`).

Vincoli:

- niente endpoint float arbitrari nell'authority serializzata;
- niente nuove `FRTCellId` per microcelle;
- niente 12 direzioni;
- la geometria interna deve essere conservata come dato di gioco quando ha conseguenze;
- exact anchor granularity sul lato **non va inventata** durante il consolidamento: va definita dall'owner geometrico e testata.

## 1.4 Boundary vs volume

Il GrayKit deve distinguere chiaramente:

```text
VOLUME / FOOTPRINT CHIUSO
    → quanto spazio della cella è invaso
    → 12-sector occupancy + core

BOUNDARY / SEGMENTO APERTO
    → una frontiera strutturale
    → bordo esterno oppure segmento interno quantizzato
    → conseguenze separate: movement / LOS / projectile / cover / door state
```

Il placement vocabulary del GrayKit oggi conosce `EdgeBound`, ma una frontiera interna non è un edge condiviso fra due celle. L'owner GrayKit deve quindi **estendere il vocabolario di placement per i segmenti interni**, senza trasformarlo automaticamente in un nuovo enum runtime. Il nome definitivo va scelto coerentemente con la spec corrente (`SegmentBound`, `GeometryBound` o equivalente), non imposto da questo handoff.

## 1.5 Fog of War GrayKit

Famiglia richiesta:

- `GB_FOW_Cell`;
- `GB_FOW_CellPartial`;
- `GB_FOW_CellFull` solo se la decisione di scope abilita il vero nascondimento;
- `DBG_LOS_Ray`;
- `DBG_VisibleSector`;
- `DBG_OccludedSector`.

Contratto:

- footprint = cella esagonale `HexSize = 150 cm`;
- altezza Full = **`0.88 H`**, pari a 220 cm con `H = 250 cm` oggi;
- `0.88 H` non cambia `LayerHeight`;
- stati visuali: `Visible`, `Partial`, `Unseen`;
- risultato relativo a viewer/team;
- FoW = presentation del risultato Team Knowledge/LOS, non authority;
- nessun planning/intento privato avversario è necessario per disegnarlo;
- i 12 settori occupancy **non diventano visibility sectors**. Un eventuale riuso grafico è presentation-only.

---

# 2. Audit delle issue: stato vero da usare

## Già chiuse / consegnate — non rifare

- **#1155** — `HexSize = 150` ✅
- **#619** — occupancy 12 settori + core ✅
- **#620** — grammatica quantizzata + validator ✅
- **#621** — bake geometria verso dati tattici ✅
- **#832** — Stable ID strutture ✅
- **#1096** — voci PIE GrayKit ✅

## #1174 — amministrativamente aperta, tecnicamente risolta

La mesh/fallback esagonale è già in `main`; `PIE-HEX-VIZ-BORDI` è stato rifatto ed è verde.

**Azione di consolidamento:** chiudere #1174 come `completed`; non tenerla nel percorso critico.

## #1094 — solo due decisioni restano da U25

Stato consolidato:

- `GBX-2` ✅ chiusa da D-171;
- `GBX-3` ✅ chiusa da D-172;
- `GBX-4` ✅ chiusa da D-173;
- `GBX-6` ✅ chiusa da D-163 / #1155;
- `GBX-1` ⏳ rinviata a U25;
- `GBX-5` ⏳ rinviata a U25.

**Azione:** non trattare #1094 come cinque decisioni ancora aperte. U25 è l'unico consumatore residuo per `GBX-1` e `GBX-5`.

## #712 — authoring UX osservabile già verificata

`U22` è 4/4:

- ghost ✅;
- snap ✅;
- Undo ✅;
- residui ✅.

#712 resta aperta non perché manchi il gesto, ma perché da quella seduta sono emersi i muri interni e le loro conseguenze.

**Azione:** spostare il percorso critico dal “finire ghost/snap” a **#1239** e ai residui geometrici reali.

## #1239 — RIUSARE per `Center → SideAnchor`

È già l'owner più vicino e corretto:

- conserva `InteriorWalls`;
- include esplicitamente `centro → punto medio di un lato`;
- `Offset == 0` identifica il passaggio per il centro;
- il centro rende la cella non entrabile;
- il dato entra nell'hash;
- la LoS intra-cella è volutamente fuori scope.

**Non creare una nuova issue radiale.**

Estendere #1239 con il nuovo requisito d'autore:

1. supportare/definire `Center → SideAnchor` come famiglia di authoring;
2. `SideAnchor` deve essere discreto/quantizzato;
3. l'esatta granularità di anchor sul lato è una decisione geometrica, non un float libero;
4. il segmento resta `InteriorWall` anche quando tocca/chiude un edge;
5. il volume occupancy resta separato;
6. mantenere il predicato movement già deciso da D-179;
7. non dedurre automaticamente la LoS.

Se la struttura corrente `FRTGeometrySegment{Axis,Offset,AlongStart,AlongEnd}` non può rappresentare un anchor laterale richiesto, #1239 deve esplicitare il delta di formato invece di approssimarlo.

## GEO-4 — LoS attraverso frontiere interne

La LoS dei muri interni è esplicitamente esclusa da #1239/D-179 e anche dal consolidamento FoW di #1467.

**Azione:** registrare/tenere `GEO-4` come decisione separata nell'owner delle decisioni aperte. Non dedurre `blocks LOS` da `blocks movement`.

Diventa requisito v0.1 solo se un consumer v0.1 — in particolare #1467 — necessita realmente di LoS intra-cella.

## #1467 — riusare, niente seconda issue FoW

Il commento GrayKit è già presente e ha consolidato:

- `GB_FOW_Cell`;
- `GB_FOW_CellPartial`;
- `GB_FOW_CellFull` condizionale;
- debug LOS;
- `0.88 H`;
- Team/viewer semantics;
- 12 settori occupancy ≠ visibility.

Resta una decisione di scope (`D-225` riservata nel thread):

```text
VELO           → Cell + Partial
NASCONDIMENTO  → Cell + Partial + Full
```

**Azione:** non modellare `GB_FOW_CellFull` finché la decisione di scope non abilita uno stato `Unseen` realmente nascosto.

## #956 — board grammar resta lavoro reale

Resta il checkpoint principale per colore + forma della cella/terreno e la verifica in scala di grigi.

È parte del percorso GrayKit 0.1 perché rende la board leggibile senza affidarsi solo al colore.

## #1095 — U25 è il gate dimensionale del GrayKit

U25 deve chiudere:

- `GBX-1` Safe Placement inset;
- `GBX-5` visual footprint unità;
- validazione a tre distanze;
- Cell Placement Volume;
- confronto CellBound / EdgeBound / nuovo binding su segmento interno;
- occupancy 12+core come debug authoring;
- proxy FoW coerenti con l'esito di #1467/D-225.

## #324 — correggere incoerenza amministrativa

Il titolo dice ancora `[EPIC v0.2]`, mentre label e corpo dichiarano l'anticipazione a v0.1.

**Titolo da riallineare:**

`[EPIC v0.1] E23 · Muri, porte e interaction graph`

Non creare una seconda epic GrayKit.

---

# 3. Roadmap GrayKit v0.1 definitiva

```text
BASELINE CONSEGNATA
#1155 #619 #620 #621 #832 #1096
          │
          ▼
0 · NORMALIZZAZIONE TRACKING
close #1174
rename #324 v0.2 → v0.1
update #286 GrayKit linked work
          │
          ▼
1 · BOARD READABILITY
#956
          │
          ├──────────────────────┐
          ▼                      ▼
2 · GRAYKIT DIMENSIONS       3 · INTERNAL GEOMETRY
#1095                    #1239 (+ Center→SideAnchor)
  ├─ GBX-1                  └─ GEO-4 resta separata
  └─ GBX-5
          │                      │
          └──────────┬───────────┘
                     ▼
4 · TEAM KNOWLEDGE PRESENTATION
#1467 + decisione D-225
  ├─ veil: Cell + Partial
  └─ hide: Cell + Partial + Full
                     │
                     ▼
5 · INTEGRATION GATE
#286 / E21 + E47 + E13
PIE + packaged smoke + docs owners allineati
```

---

# 4. Definition of Done — GrayKit v0.1

GrayKit 0.1 è Done quando:

1. la cella renderizzata è realmente esagonale e i 6 lati sono leggibili;
2. la board usa il canale forma previsto da D-146/D-183 nello scope v0.1;
3. U25 chiude `GBX-1` e `GBX-5` con osservazione alle tre distanze;
4. `CellBound`, `EdgeBound` e binding su segmento interno sono distinguibili come concetti di authoring;
5. occupancy 12+core è visualizzabile/debuggabile ma resta una misura di volume;
6. `Center → SideAnchor` ha una rappresentazione discreta e deterministica oppure una decisione esplicita sul delta di formato;
7. i muri interni necessari alla v0.1 hanno conseguenze movement/hash coerenti con D-179/#1239;
8. la LoS intra-cella non viene dedotta: se richiesta, `GEO-4` è decisa e testata;
9. #1467 ha una decisione di scope chiusa e modella solo i proxy che il gioco produce davvero;
10. FoW/veil usa Team Knowledge e non introduce leak di intenti/dati privati;
11. `GB_FOW_CellFull`, se esiste, usa altezza `0.88 H`, non un `220` assoluto come authority;
12. guide EditorOnly non compaiono nella build packaged;
13. le mesh restano sostituibili senza cambiare regole, TurnLog o StateHash;
14. `roadmap-v0.1.md`, Graybox contract, Geometry Authoring spec e issue non si contraddicono;
15. questo handoff non resta come seconda roadmap operativa.

---

# 5. File canonici da aggiornare

## `docs/roadmap/roadmap-v0.1.md`

Aggiungere sotto E21 o in una sottosezione chiaramente non-epic:

**GrayKit v0.1 — execution overlay, non una seconda release ladder**

Tracking:

- baseline: #1155 #619 #620 #621 #832 #1096;
- amministrazione: #1174 da chiudere, #324 titolo da riallineare;
- board: #956;
- dimensions/U25: #1095 → GBX-1/GBX-5;
- interior geometry: #1239;
- knowledge/FoW: #1467;
- integration: #286.

Non creare CP `E21.4` o una nuova Epic per simmetria.

## `docs/technical/systems/spec-graybox-placement-contract.md`

Integrare:

- distinzione `volume footprint` vs `open boundary`;
- placement vocabulary per segmento interno quantizzato, senza trasformarlo automaticamente in enum runtime;
- proxy FoW e altezza `0.88 H`;
- nota che `Full` è condizionale alla decisione di scope di #1467;
- U25 come owner delle due decisioni visuali residue GBX-1/GBX-5;
- rimuovere riferimenti vivi al Feature Registry se ancora presenti come authority.

## `docs/technical/systems/spec-hex-geometry-authoring.md`

Integrare:

- requisito `Center → SideAnchor`;
- anchor discreto/quantizzato;
- #1239 come owner dell'internal geometry runtime;
- distinzione wall/open segment vs occupancy;
- dichiarare esplicitamente che l'attuale `Axis/Offset/Along*` va misurato contro il nuovo requisito prima di promettere supporto generale;
- `GEO-4` separa LoS intra-cella da movement.

## Knowledge / LOS spec

Integrare solo ciò che #1467/D-225 decide.

Mai copiare i 12 settori occupancy come visibility model.

---

# 6. GitHub actions da applicare

1. **#1174** → close `completed`.
2. **#324** → rename `[EPIC v0.2]` → `[EPIC v0.1]` e collegare #1239.
3. **#286** → aggiungere blocco “GrayKit v0.1 linked execution”.
4. **#1094** → nessuna nuova decisione; chiarire che restano solo GBX-1/GBX-5 → U25.
5. **#1095** → estendere U25 con internal-segment binding + occupancy debug + FoW proxy condizionali.
6. **#712** → non assegnare nuovo lavoro di ghost/snap; collegare il residuo runtime a #1239.
7. **#1239** → estendere con `Center → SideAnchor`; non creare issue radiale duplicata.
8. **#1467** → mantenere il commento GrayKit già consolidato; chiudere D-225 prima di `CellFull`.
9. **#956** → resta board-readability gate.
10. **GEO-4** → decisione LoS intra-cella separata; issue solo se/quanto il trigger v0.1 la rende eseguibile.

---

# 7. Commit plan consigliato

1. `docs(graykit): consolidate v0.1 execution into canonical roadmap`
2. `docs(graykit): separate volume occupancy from internal boundaries`
3. `docs(geometry): specify center-to-side anchor requirement`
4. `docs(perception): bind GrayKit fog proxies to team knowledge scope`
5. `chore(issues): reconcile GrayKit v0.1 tracking`
6. eventuale codice/test di #1239 in commit separato dalle decisioni/documentazione.

