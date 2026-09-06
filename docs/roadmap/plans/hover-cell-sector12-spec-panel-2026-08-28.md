# Hover Cell + 12-Sector Pointer Feedback — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **mandato non eseguito** · **Data**: 2026-08-28
> **HEAD della revisione**: `483e031a` · branch `main` · working directory con 2 file modificati e 6 untracked
> **Sorgente revisionata**: mandato «CLAUDE TASK — RefactorTactics v0.1 · Consolidamento issue, Hover Cell /
> 12-Sector Pointer Feedback e roadmap di chiusura», consegnato via `/sc:spec-panel` il 2026-08-28.
> **Perché non eseguito**: `/sc:spec-panel` è un task **documentale/analitico** ([`CLAUDE.md`](../../../CLAUDE.md) §6),
> e la revisione ha trovato che le premesse fattuali del mandato non reggono. Applicarlo avrebbe creato issue,
> tabelle e sezioni di roadmap su una tassonomia inesistente.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia ([`CLAUDE.md`](../../../CLAUDE.md) §7).
> Dove contraddice un fatto misurabile sul branch, prevale il repository.

---

## 1. Preflight — le premesse del mandato, misurate

Il mandato dichiara «Stato verificato il 2026-08-28» su dieci issue e sei documenti. Sono stati riverificati
tutti. **Sette premesse su undici sono false.**

| # | Premessa del mandato | Misura su `483e031a` | Esito |
|---|---|---|---|
| P1 | `docs/spec/geometry-taxonomy.md`, `map-geometry.md`, `geometry-conventions.md` | `git ls-files docs/spec` → **zero**. `docs/spec/` non è una cartella del repository | ❌ |
| P2 | `docs/qa/manual/PIE-V01-POINTER.md` | `git ls-files docs/qa` → **zero**. `PIE-V01-POINTER` vive in [`docs/technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) | ❌ |
| P3 | `docs/roadmap/roadmap-v0.1.md`, `v0.1-definition-of-done.md` | presenti, 2036 e 288 righe | ✅ |
| P4 | «14 mandatory gates: CORE · POINTER · LOS · VIP · AUDIO · DATA · CLEAN · NET · NET-DED · REPLAY · VISMAP · PERF · CLAIMDP · TARGET» | i gate della v0.1 sono **`G1`…`G14`**, con altra semantica. `CLAIMDP` e `VISMAP`: **zero occorrenze** in tutto il repository | ❌ |
| P5 | `FRTSector12` è «geometria canonica» | **zero occorrenze** in `Source/` e `docs/` | ❌ |
| P6 | `FRTDirection6` è «le sei direzioni tattiche» | **zero occorrenze** in `Source/`; 3 in un handoff archiviato. Il tipo reale è **`ERTHexDirection`** ([`RTCellId.h:11`](../../../Source/RefactorTactics/Map/RTCellId.h)) | ❌ |
| P7 | I 12 settori da 30° sono da implementare | **esistono già**: `RT_OccupancySectorCount = 12` ([`RTHexOccupancyLibrary.h:8`](../../../Source/RefactorTactics/Map/RTHexOccupancyLibrary.h)), con boundary `-30° + 30°·Sector` già ancorata al primo vertice. Chiusi da **#619** | ❌ |
| P8 | Overlay **flat-top** | la griglia è **pointy-top**, dichiarato in 15 punti di `Source/`, incluso il commento che chiama «primo vertice a −30 gradi» l'invariante che *«impedisce ai due disegni di divergere di nuovo»* | ❌ |
| P9 | #705 OPEN, è il parent corretto | **OPEN ✅**, milestone 3, label `v0.1`+`checkpoint`+`P1`. Ma il titolo reale è *«CP 11.8 — Pointer Interaction Contract: Hover / LMB / RMB»*, non quello citato | 🟡 |
| P10 | #33 CLOSED, ha chiuso world-to-cell | **CLOSED ✅**, ma è *«CP 2.3 — Input, selezione e preview su hex»* | 🟡 |
| P11 | #25 = «E11 — Input, controlli e accessibilità» | è **«[EPIC v0.1] E11 — HUD, log e debug»** | ❌ |

### 1.1 L'audit «noto» delle dieci issue

Il mandato §3 elenca dieci issue con titolo e argomento. **Cinque sono sbagliate, e tre non sono issue.**

| Rif. | Argomento dichiarato dal mandato | Titolo reale · stato reale | Verdetto |
|---|---|---|---|
| #705 | pointer contract, platform parity, camera assist, recovery | *CP 11.8 — Pointer Interaction Contract: Hover / LMB / RMB* · **OPEN** | 🟡 parent corretto, titolo inventato |
| #33 | click hex → `FRTCellId` + highlight | *CP 2.3 — Input, selezione e preview su hex* · **CLOSED** | 🟡 conclusione giusta, titolo sbagliato |
| #25 | E11 Input, controlli e accessibilità | *[EPIC v0.1] E11 — **HUD, log e debug*** · **OPEN** | ❌ epic sbagliato |
| #75 | «AoE preview + facing», **KEEP CLOSED** | *CP 10.2 — Obiettivo contestabile* · **OPEN** | ❌ argomento e stato sbagliati |
| #218 | «6 tactical directions / fixed-point» | *CP 20.1 · `URTIconCatalogData`* · **CLOSED** | ❌ è il catalogo icone |
| #77 | «Shared Planning completo» | *CP 11.1 — HUD di partita completo* · **OPEN** | ❌ argomento sbagliato |
| #291 | — (solo «verifica») | *La rotazione dichiarata: cablaggio fatto, resta l'input (E11)* · **OPEN** | ✅ **è l'unica davvero pertinente** |
| #681 | issue da verificare | è una **PR** *(feat(editor)…)* · **CLOSED** | ❌ non è una issue |
| #11 | issue da verificare | è una **PR** *(feat(camera)…)* · **MERGED** | ❌ non è una issue |
| #659 | issue da verificare | è una **PR** *(test(scenari)…)* · **MERGED** | ❌ non è una issue |

Nessuna issue del repository — su **642** in stato `all` — porta hover overlay o quantizzazione a settori
per il puntatore. Il gap dichiarato al §4 del mandato **esiste**; è tutto il resto attorno che non regge.

---

## 2. Rilievi del panel

### R-1 🔴 CRITICO — I quattordici gate su cui si regge metà del mandato non esistono

**Wiegers.** Il §8 li chiama «MANDATORY GATES DA PRESERVARE», il §9 impone una tabella live con tutti e
quattordici, il §15 fa della loro presenza un guardrail di accettazione. Nessuna delle tre istruzioni è
eseguibile: i gate normativi della v0.1 sono **`G1`…`G14`** in
[`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3, e dicono altro — `G1` build senza warning,
`G9` subset `RELEASE-V01`, `G12` packaging. Il numero quattordici coincide; i nomi no.

Due dei quattordici — `CLAIMDP` e `VISMAP` — hanno **zero occorrenze in tutto il repository**. Non sono
gate rinominati: non sono mai esistiti qui.

> **Effetto se applicato**: la roadmap avrebbe guadagnato una seconda tassonomia di gate accanto a `G1…G14`,
> con quattordici righe «MISSING OWNER — audit required» che il §9 stesso prescrive di riempire così. La
> tabella sarebbe sembrata un inventario e sarebbe stata un'invenzione.

### R-2 🔴 CRITICO — Quattro dei sei documenti canonici sono fantasmi, e l'owner reale non è mai nominato

**Cockburn.** Il §6 ordina di leggere sei file e di «trovare il canonico equivalente» se un percorso è
cambiato. Ma `docs/spec/` e `docs/qa/` non sono cartelle che sono state spostate: **non sono mai esistite**
in questo repository. Le cartelle di primo livello di `docs/` sono quindici, e nessuna delle due c'è.

Il documento che possiede davvero il contratto del puntatore —
[`docs/technical/systems/spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md),
571 righe, la matrice `oggetto × contesto × Hover|LMB|RMB` a otto esiti — **il mandato non lo nomina mai**.
Un mandato di consolidamento che non conosce l'owner del sistema che consolida sta scrivendo accanto al
canone, non dentro.

### R-3 🔴 CRITICO — La quantizzazione a 12×30° che ISSUE B chiede di implementare esiste già, ed è già chiusa

**Fowler.** ISSUE B chiede di «implementare una funzione pura equivalente a `SectorFromWorldPoint`», con
ampiezza 30°, dodici settori, boundary da documentare. Il repository ha già tutto questo:

```cpp
// Source/RefactorTactics/Map/RTHexOccupancyLibrary.h:8
static constexpr int32 RT_OccupancySectorCount = 12;

// Source/RefactorTactics/Map/RTHexOccupancyLibrary.cpp:96-101
for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
{
    // -30 gradi e' il PRIMO VERTICE, lo stesso da cui `URTHexLibrary` costruisce il perimetro pointy-top.
    const double Radians = FMath::DegreesToRadians(-30.0 + 30.0 * Sector);
```

La convenzione di boundary che ISSUE B mette fra le «Boundary convention da documentare/testare» — orientamento
0°, passo 30°, ancoraggio — **è già decisa e già scritta**, e la sua issue **#619** *(«Occupancy a 12 settori:
la maschera, `CoreBlocked`, e il `Constrained` che qualcuno legge»)* è **CLOSED**.

⚠️ E #619 porta già, nel proprio corpo, l'avvertimento che ISSUE B ripete come se fosse nuovo:

> *«Questi dodici settori non aggiungono direzioni. Le direzioni restano sei, pointy-top, e chiunque le
> scriva a mano ripete un errore già pagato per un'intera seduta (#553).»*

Il guardrail «i 12 settori NON diventano 12 archi del grafo» è corretto — ed è già stato scritto, pagato e
chiuso otto giorni fa da qualcun altro.

### R-4 🔴 CRITICO — «Settore» nomina già due cose diverse; il mandato ne aggiunge una terza senza dichiarare la relazione

**Hohpe.** Nel repository, oggi:

| Nome | Cardinalità | Semantica | Dove |
|---|---|---|---|
| `ERTHexDirection` | **6** | direzioni tattiche del grafo, pointy-top, ordine stabile `E,NE,NW,W,SW,SE` | `RTCellId.h:11` |
| `HandleFacingSector(ERTHexDirection)` | **6** | il «settore» del *facing*: è una direzione, non un dodicesimo | `RTPlayerController.cpp:1718` |
| `RT_OccupancySectorCount` | **12** | i settori dell'*occupancy*: quanto una cella è invasa da geometria | `RTHexOccupancyLibrary.h:8` |

Il mandato introduce un terzo «settore» a 12 per l'hover direzionale e lo chiama `FRTSector12`, dichiarando
di «riusare il canonico se già esistente». Ma il candidato a 12 che esiste è **occupancy** — misura di
invasione geometrica — non direzione di puntamento; e il «settore» che *è* direzionale ne ha **sei**, non
dodici, e ha già un consumatore (`FacingSectorProducesPlannedFacing`).

> **Il rilievo non è che i dodici settori direzionali siano sbagliati.** È che la specifica non dichiara
> quale dei due significati esistenti estende, e quale lascia intatto. Senza quella riga, il primo lettore
> che incontra `Sector` in una firma non sa quale delle tre cose stia leggendo.

### R-5 🔴 CRITICO — Il gap reale non è l'overlay: è che la regola «hover non committa» è dichiarata provata e non lo è

**Crispin.** Questo è emerso verificando la DoD del mandato, e vale più di tutto ciò che il mandato chiede.

[`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md) §«Test automatici
minimi» elenca otto test `RefactorTactics.PlayerInput.*` e li dichiara:

> ✅ **Scritti il 2026-08-13 sera** — dieci, ognuno per una regola nuova, tutti verdi e tutti passati per la
> verifica di mutazione

[`roadmap-v0.1.md:858`](../roadmap-v0.1.md) li ripete nella colonna evidenza di CP 11.8. **Nessuno degli otto
esiste.** I test `PlayerInput.*` reali sono sedici, elencati per nome esatto:

```
ChargeIsPlannedByClickingTheEnemy       GhostIsNeverAGameplayTarget            RightClickNeverDeselects
DashRejectsNonLinearDestination         IllegalFacingIsRejectedNotCorrected    TargetCellIgnoresOccupyingUnit
EveryKitEntryIsReachable                NeutralEnemyClickDoesNotPlan           TargetCellProducesPlannedAttackCell
FacingSectorProducesPlannedFacing       PathingCellWinsOverDoorMesh            TargetEdgeProducesPlannedCoverEdge
GenericHotkeyResolvesByNameNotPosition  RightClickBackFollowsTotalOrder        UndoShortensThePlan
                                                                               WaypointClicksBuildAndRejectPlans
```

Non è un rename: nessun nome contiene `Hover`, `HUD`, `Playback`, `ReactionWindow`, `AllyGhost` o
`LogicalMapObject`. Mancano **otto su sedici dichiarati**, fra cui:

🔴 **`HoverNeverCommits`** — il test che prova esattamente la separazione hover/selezione che ISSUE A e
ISSUE B mettono in DoD (*«Cambiare hover non modifica unità selezionata, waypoint, azione armata o intent
lockato»*). Il mandato la tratta come una proprietà **già garantita** da preservare. È una proprietà
**dichiarata verde e mai provata**.

🔴 **`HiddenEnemyCannotBecomeHoverTarget`** — la privacy dell'hover, §6.1 della spec, che il guardrail §15 del
mandato («nessun intent team-only reso globale») dà per presidiata.

> **Conseguenza sull'ordine dei lavori.** Il mandato mette l'overlay ingrandito in Wave 0 come «pointer slice
> immediato». Ma un overlay più grande su un contratto la cui invariante centrale non è provata rende più
> visibile un comportamento che nessuno sta misurando. **Scrivere gli otto test mancanti viene prima** — e
> non è lavoro nuovo da specificare: i nomi, le regole e le sezioni della spec che li giustificano sono già
> scritti.

### R-6 🟡 MAGGIORE — «Overlay flat-top» contraddice la griglia del progetto

**Fowler.** ISSUE A prescrive «Overlay **flat-top** centrato sulla `FRTCellId` hovered». La griglia è
**pointy-top** ovunque, e `RTHexLibrary.cpp:379` fissa `60·i − 30` come angolo dei sei vertici. Un overlay
flat-top sopra una cella pointy-top è ruotato di 30°: al 115% di scala non sarebbe un evidenziatore, sarebbe
una stella a dodici punte.

È un refuso di una parola, e sarebbe costato una seduta di «perché l'highlight non si allinea».

### R-7 🟡 MAGGIORE — Tre criteri di DoD non sono falsificabili

**Wiegers.** Sono in entrambe le issue:

| Criterio | Perché non è verificabile | Riformulazione misurabile |
|---|---|---|
| «Nessun flicker evidente nelle transizioni normali» | *evidente* per chi, a quale framerate, su quale transizione? Due osservatori danno due esiti | «attraversando N celle a velocità di cursore costante, `HoveredCell` cambia valore al più una volta per cella» — verificabile in automation, senza occhio umano |
| «Aggiorna materiale/visualizer solo quando cella o settore cambiano, **quando pratico**» | «quando pratico» rende la clausola vera per costruzione: chi non la rispetta dichiara che non era pratico | o è un invariante (`se (Cell,Sector) invariati ⇒ zero set sul materiale`) o non è un criterio |
| «Center dead-zone configurabile» | nessun valore di default, nessun intervallo, nessun criterio per sceglierlo | dichiarare la baseline come frazione dell'inraggio e il test che la ancora |

**Adzic.** Il terzo è il più costoso: la dead-zone è l'unico parametro dell'intera ISSUE B che decide se un
punto ha settore o no, ed è l'unico rimasto senza numero. Un `Given/When/Then` la fisserebbe in una riga:

```gherkin
Given una cella di HexSize 100 e dead-zone 0.25·inradius
When il puntatore è a 20 unità dal centro
Then il settore è None
And a 30 unità dal centro il settore è definito
```

### R-8 🟡 MAGGIORE — L'epic dichiarato è quello sbagliato

**Cockburn.** Entrambi i body dichiarano «Epic: #25 — E11 Input, controlli e accessibilità». #25 è
**«E11 — HUD, log e debug»**, e la sua milestone è *v0.1 · Leggibilità*, il cui criterio di chiusura parla di
`rt.Debug.*`, catalogo icone e voci PIE. L'hover overlay ci sta — è leggibilità — ma per la ragione scritta
nella milestone, non per quella scritta nel mandato.

### R-9 🟢 MINORE — Il mandato non nomina l'unica issue aperta davvero adiacente

**Nygard.** **#291** *(«La rotazione dichiarata: cablaggio fatto, resta l'input»)* è OPEN, è E11, e riguarda
precisamente l'input mancante per un settore direzionale. Il mandato la elenca fra quelle da «verificare
inoltre», senza argomento. È l'unica delle dieci il cui contenuto reale tocca ISSUE B — e il commento in
[`RTPlayerController.h:106`](../../../Source/RefactorTactics/Player/RTPlayerController.h) dice che
`HandleFacingSector` *«aveva come unici chiamanti dei test»*: c'è già un produttore senza consumatore in
quest'area.

---

## 3. Cosa del mandato resta valido

Il metodo è buono e va conservato. Quattro cose reggono alla misura:

1. **La regola fondamentale** — «aggiorna un issue equivalente prima di crearne uno nuovo» — è corretta, ed è
   la stessa disciplina che il repository applica altrove.
2. **Il gap è reale.** Su 642 issue, nessuna copre l'overlay hover ingrandito né la quantizzazione
   direzionale del puntatore. Le due issue **vanno aperte** — con altro contenuto.
3. **#705 è il parent giusto**, è OPEN, ed è davvero l'owner del contratto puntatore.
4. **I vincoli architetturali del §1 sono tutti corretti**: nessun collider per cella, nessun Actor
   persistente per cella, riuso del percorso world-to-cell esistente, funzione di settore pura, hover
   presentation-only. Nessuno di questi va indebolito.

---

## 4. Il mandato corretto

Se il lavoro va fatto, questo è ciò che l'audit sostiene.

**Ordine.** Prima gli otto test mancanti di CP 11.8 (**R-5**), poi l'overlay, poi il settore. La ragione non
è di processo: `HoverNeverCommits` è la precondizione che entrambe le issue mettono in DoD e che nessuna
delle due può verificare da sola.

**Correzioni ai due body prima di aprirli:**

| Voce | Da | A |
|---|---|---|
| epic | «#25 — E11 Input, controlli e accessibilità» | «#25 — E11 HUD, log e debug» |
| riuso | «#33 — Click hex → FRTCellId» | «#33 — CP 2.3 Input, selezione e preview su hex» |
| owner documentale | `docs/spec/geometry-conventions.md` | [`docs/technical/systems/spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md) |
| prova manuale | `docs/qa/manual/PIE-V01-POINTER.md` | [`docs/technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) |
| orientamento overlay | flat-top | **pointy-top**, primo vertice a −30° |
| tipo a 6 | `FRTDirection6` | **`ERTHexDirection`** |
| tipo a 12 | «riusa `FRTSector12` canonico» | dichiarare la relazione con `RT_OccupancySectorCount` (occupancy, **non** direzionale) e con `HandleFacingSector` (direzionale, **sei**) — vedi **R-4** |
| boundary convention | «da documentare/testare» | **già fissata** da #619: `-30° + 30°·Sector`. Il lavoro è *riusarla*, non deciderla |
| dead-zone | «configurabile» | baseline numerica + il test che la ancora |
| flicker | «nessuno evidente» | invariante contabile su `HoveredCell` |

**Sulla roadmap.** La sezione «v0.1 Closure Sequence» a cinque Wave con tabella dei quattordici gate **non va
scritta**: la sequenza di chiusura normativa è già `G1…G14` in
[`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md), con stati misurati e datati. Una seconda
sequenza operativa accanto sarebbe la seconda source of truth che il mandato stesso vieta al §15.

Ciò che manca davvero alla roadmap è **una riga sola**: che gli otto test citati alla riga 858 non esistono.

**Fuori scope, ma va aperto separatamente:** la dichiarazione ✅ falsa nella spec e nella roadmap (**R-5**) è
un difetto del repository indipendente da hover e settori, e non va sepolto dentro una issue di presentazione.

---

## 5. Limiti di questa revisione

- **Non eseguita**: nessuna issue creata, riaperta o modificata; nessun documento canonico toccato; nessun
  commit. Il mandato chiedeva scritture su GitHub e su tre documenti: sono state **sospese**, non negate.
  I permessi `gh` ci sono (`repo` fra gli scope, account `DegrassiAaron` attivo): il blocco è di merito.
- **Non misurato**: se gli otto test mancanti siano mai esistiti e siano stati rimossi, o non siano mai stati
  scritti. Serve `git log -S HoverNeverCommits`, non fatto qui.
- **Non misurato**: nessuna suite eseguita. Questa revisione non tocca codice e non ne aveva bisogno.
- **Non verificato**: i body completi delle 642 issue. La ricerca di equivalenti è stata fatta sui **titoli**
  con nove termini (`hover|pointer|puntator|settor|sector|overlay|highlight|mouse`). Un'issue che copra
  l'hover overlay descrivendolo solo nel corpo sfuggirebbe a questa misura.
- **Contesto**: altri due consolidamenti datati 2026-08-28 sono in corso o arrivati nella stessa working
  directory (`master-issue-reconciliation-spec-panel-2026-08-28.md`,
  `RefactorTactics_GrayKit_v0.1_Roadmap_CONSOLIDATED_2026-08-28.md`, quest'ultimo untracked a radice). Non
  sono stati diffati contro questo mandato.
