# Lane 4 — Editor / Tooling

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `52c08286` (riallineato dopo `#552`, `#553`, `#619`)
> **Cosa è**: la sequenza di lavoro della lane *Editor / Tooling*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato. Aperto/chiuso vive su GitHub, e lo stato delle **sedute**
> vive in [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) + `editor-sessions.yaml`.
> In caso di divergenza **vince la fonte**, non questa fotografia.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

Gli strumenti: editor mode della mappa, harness degli scenari, validator, e le **sedute** — il
lavoro che solo una persona davanti a Unreal può fare.

⚠️ **Questa lane non implementa regole di gioco.** Un tool che simulasse un risultato diverso dal
resolver sarebbe una seconda verità; la validazione competitiva resta runtime.

⚠️ **La `editormap` non si aggiorna a mano**: è `GENERATA` da
`python scripts/feature_registry.py shortlist`, che legge `editor-sessions.yaml`. Le sedute qui sotto
si citano per **ID**, mai per contenuto.

---

## Sequenza

### 1. `#38` · CP 2.8 — Playtest della partita hex (sessione D) 🟢 **pronta** · **P0**

**Epic**: `#16` (E2) · **Dipende da**: `#33` ✅ `#35` ✅ `#36` ✅ `#37` ✅ — tutte chiuse

⚠️ **Sblocca un'intera epic**: `#17` (E3, Dismissione del quadrato) dichiara nel proprio corpo
*«non iniziare prima che `#38` sia chiuso»*. È il P0 più economico della release — è una sessione,
non del codice.

### 2. `#451` · U1-C — Seduta: costruire `L_HexArena` e verificare le sette voci 🟢 **pronta** · **P1**

**Dipende da**: `#449` (U1-A, allowlist `.gitignore`) ✅ · `#450` (U1-B, criteri misurabili) ✅

⚠️ **Gli artefatti sono già committati** (`2e2caff`): `DA_HexMap_Arena.uasset` e `L_HexArena.umap`
sono tracciati, e per questo la seduta **U1** è passata da ⏳ a 🟡 nella `editormap`. Resta la
verifica delle sette voci — 0/7 al momento della fotografia.

### 3. `#82` · CP 12.2 — Matrice dei test manuali v0.1 (sessione E) 🟢 **pronta** · **P1**

**Epic**: `#26` (E12) · **Dipende da**: `#81` ✅ (CP 12.1, replay deterministico)

**Sblocca `#85`** (CP 12.5, release interna) insieme a `#84`. Il gate `G9` della Definition of Done
chiede il subset **`RELEASE-V01`**: **17 voci** marcate, contate col comando scritto nel gate — non
con un `grep` che conta anche la prosa.

### 4. Viz editor — la serie 1/4 → 4/4

**Brief**: [`../../technical/brief-editor-map-viz.md`](../../technical/brief-editor-map-viz.md)

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| ~~`#551`~~ | 1/4 · superficie e costo di movimento leggibili senza aprire un pannello | ✅ **chiusa 2026-08-12** | P2 |
| ~~`#552`~~ | 2/4 · dare volume ai blocchi, distinguere «non si passa» da «non si vede» | ✅ **chiusa 2026-08-12** — PR `#670` | P2 |
| ~~`#553`~~ | 3/4 · coperture, porte e transizioni stanno sui **bordi**, e l'overlay non li mostra | ✅ **chiusa 2026-08-12** — PR `#673` | P2 |
| `#554` | 4/4 · le transizioni e la raggiungibilità — capire come si collegano i piani | 🟢 **pronta — ultima della serie** | P2 |

⚠️ **Sono ordinate per frequenza sulla mappa, non da una dipendenza dichiarata**: nessuna delle
quattro blocca formalmente le altre. Tre su quattro sono chiuse lo stesso giorno, nell'ordine
proposto dal brief.

**`#554` non è «il residuo»**: è l'unica delle quattro che guarda il **grafo** invece della
geometria — *una piattaforma scollegata è una mappa rotta* — e finché è aperta la vista in editor
mostra i volumi e tace sulle transizioni. È la ragione per cui `RT-FEAT-TOOL-MAP-EDITOR` tiene
`log_debug: partial` invece di `done`.

⚠️ Chi la raccoglie legga prima `RefactorTactics.HexMap.OnlyTheCellsComponentIsClickable`: `#552` e
`#553` hanno aggiunto un **secondo** `UInstancedStaticMeshComponent` all'actor, e quel test è ciò che
impedisce al pennello di risolvere un click contro la geometria sbagliata. `#554` aggiunge altra
geometria alla stessa scena.

### 5. Map Sketch Editor — la serie geometria, `#619` → `#623`

**Referto**: [`map-sketch-editor-spec-panel-2026-08-12.md`](map-sketch-editor-spec-panel-2026-08-12.md) ·
**Feature**: `RT-FEAT-TOOL-MAP-GEOMETRY` (nuova) e `RT-FEAT-TOOL-MAP-EDITOR`

Nate il 2026-08-12 dal terzo prompt della famiglia map-editor. Le prime tre sono l'**anticipazione
dichiarata** della metà di authoring di E23.1 (v0.2): l'epic [`#324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)
**non** si apre, e la logica di transizione resta sua.

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| ~~`#619`~~ | Occupancy a 12 settori: maschera, `CoreBlocked`, e il `Constrained` che qualcuno legge | ✅ **chiusa 2026-08-12** | P2 |
| `#620` | Grammatica quantizzata delle direttrici, e il validator che la rende una regola | 🟢 **pronta — prossima della serie** | P2 |
| `#621` | Cottura: la geometria disegnata diventa `FRTHexCover` e `bBlocksMovement` | ⏳ dopo `#620` | P2 |
| `#622` | La griglia di lavoro si vede prima di disegnare, e non si confonde con una cella vera | 🟢 pronta — indipendente | P2 |
| `#623` | Seduta: luci leggibili in `L_DevSandbox`, e un modo di inquadrare tutta la mappa | 🟢 pronta — è la seduta **U21** | P2 |

⚠️ **`#623` non è codice**: `L_DevSandbox.umap` è un `.umap`, e la navigazione del viewport la
fornisce Unreal. Vive come seduta `U21` in `editor-sessions.yaml`, e il suo stato si legge nella
[`editormap`](../editormap.shortlist.md) — non qui.

**`#619` ha pagato il debito che aveva fermato i due prompt precedenti**: una geometria enumerabile
in dodici settori non porta estremi in virgola mobile dentro l'hash che tiene fermo *replay
divergence = 0*. Sedici test `RefactorTactics.HexOccupancy.*` e lo scenario
`Spec.Map.ConstrainedCellCostsMore`, tutti nel modulo **runtime** — in
`Source/RefactorTacticsEditor/` non esiste alcun test, quindi ciò che nasce dentro l'editor nasce
non verificabile.

⚠️ **Una decisione aperta è sulla strada di `#621`**, non di `#620`: `MSE-1` in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) — se qualcuno modifica a mano un campo cotto,
vince la modifica o il prossimo rebake la cancella? `#619` cuoce ma **non** innesca la domanda (il
costo ha un produttore solo); `#621` sì, perché i bordi restano a produttore condiviso col pennello.

### 6. ~~`#582`~~ · Harness: il perimetro delle capability di reazione ✅ **CHIUSA il 2026-08-12**

*«…è superato da CP 14.4, e D-109 sta per aggiungere un campo senza produttore.»*

⚠️ Il titolo nomina da solo il difetto ricorrente del progetto — **un campo che nessuno legge**.
Chiusa poche ore dopo la stesura di questo file. ⚠️ **La sua gemella `#583` resta aperta** ed è ora
il primo anello della lane 2: D-109 è atterrata, la condizione è dichiarata, e **nessuno la produce**.

### 7. Coda lunga

| Issue | Cosa | Stato | Prio |
|---|---|---|:--:|
| `#214` | `[EPIC] E19` · Classe di mappa e composizione | ⏳ | P2 |
| `#221` | `[EPIC] E17` · Validazione di stress 4v4 | ⏳ — **non è un gate di release** | P3 |

---

## Ordine consigliato

1. **`#38`** — P0, è una sessione, e sblocca l'epic `#17`.
2. **`#82`** — P1, alimenta la catena di release della lane 5.
3. **`#451`** — P1, gli artefatti ci sono già.
4. `#554` — chiude la serie viz, ed è l'unica delle quattro che guarda il grafo.
5. `#620` → `#621` — la serie geometria nell'ordine in cui paga; `#620` prima perché il validator
   è ciò che rende la grammatica una regola invece di una convenzione.
6. `#622` e `#623` — indipendenti dalle altre, e `#623` è una **seduta** (U21): si può fare in
   qualunque momento ci sia una persona davanti all'editor.

⚠️ **Le prime tre restano davanti alla geometria**: `#38`, `#82` e `#451` sono P0/P1 e sbloccano
epic e release. La serie geometria è P2 e `packaged: na` — è **fuori dai gate della v0.1** per
costruzione, quindi non compete con la consegna. Metterla prima sarebbe una scelta, non un ordine.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#38` | epic `#17` (lane 1/2) | E3 non comincia prima |
| `#82` | `#85` (lane 5) | la release interna chiede la matrice manuale |
| seduta **U20** | `#403` (lane 2) | `BAL-1` si decide giocando, non leggendo |
| ~~`#582`~~ ✅ | `#583` (lane 2) | il campo è entrato con D-109 e il produttore manca ancora |
