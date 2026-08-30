# Tactical Grid Overlay — Issue-Driven Execution — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma il kit
> *«CLAUDE — RefactorTactics · Tactical Grid Overlay — Issue-Driven Execution Plan»* (912 righe, 22 sezioni),
> arrivato come file nella radice del checkout di lavoro.
>
> **Data**: 2026-08-30 · **Base**: `main` @ `d9feb9b0`, **7 commit dietro** `origin/main` @ `aec66789` ·
> **Modo**: critique · **Focus**: requirements + architecture
>
> **Cosa è**: il verdetto su un **mandato esecutivo end-to-end** — audit GitHub, creazione/riuso di issue,
> aggiornamento roadmap, implementazione C++/asset, PIE, packaged. La **revisione** è task documentale
> ([`CLAUDE.md`](../../../CLAUDE.md) §6): §1–§9 non hanno scritto nulla, e le azioni raccomandate stanno in
> §8 come elenco.
>
> 🔵 **Poi l'autore ha deciso di eseguirne una parte, e quello è un atto separato — §10.** In due passaggi,
> entrambi su autorizzazione esplicita: `A-1`/`A-2` applicate al corpo di
> [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614); poi
> [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758) **creata**,
> [#289](https://github.com/DegrassiAaron/refactor-tactics-main/issues/289) e
> [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) **aggiornate**, e la roadmap della
> feature creata in [`tactical-grid-overlay-roadmap-2026-08-30.md`](tactical-grid-overlay-roadmap-2026-08-30.md).
> ⛔ **Nessuna issue chiusa o riaperta, nessuna Epic creata, nessun `D-nnn` assegnato** (ultimo: `D-248`),
> **nessuna riga di codice o asset toccata**, [`roadmap-v0.1.md`](../roadmap-v0.1.md) **non toccata**.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge dall'owner
> ([`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md),
> [`roadmap-v0.1.md`](../roadmap-v0.1.md), il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md)),
> **ha ragione l'owner**.
>
> **Archiviato in**: [`../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md`](../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md)

---

## 1. Il verdetto in una riga

> **Delle nove voci della missione, sette sono già vere o già tracciate — `ARTHexMapActor` è ISM per
> costruzione, l'hover esiste ed è di [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614)
> (aperta, P1, v0.1), il multilivello è di #567 dal 12 agosto — e la sola sezione che descrive lavoro nuovo,
> la §9 dei default visivi, prescrive un `GridLiftCm = 2.0` che finisce `5,5` uu DENTRO il tile opaco della
> cella: il difetto che [`RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h) dichiara
> essere già costato due incidenti.**

Il kit è **disciplinato**: `SEARCH BEFORE CREATE`, `REUSE BEFORE DUPLICATE`, i path dichiarati «solo come
hint», il divieto di inventare numeri issue, e la riga *«Se un gate non viene eseguito, scrivere `NOT RUN`»*.
Quella disciplina è ciò che rende il verdetto misurabile invece che polemico: il kit chiede lui stesso di
essere confrontato con l'albero, e confrontato perde su quasi tutto il perimetro.

Il contributo che vale il consumo è **una** casella, in §11, e sta in fondo a §7.

---

## 2. Base di misura

Misurato su albero e lato server, non ricordato. Comandi riproducibili nelle righe che li usano.

| Cosa | Valore | Come |
|---|---|---|
| Branch / `HEAD` | `main` @ `d9feb9b0` | `git rev-parse --short HEAD` |
| Divergenza | `0` avanti, **`7` dietro** `origin/main` @ `aec66789` | `git rev-list --left-right --count HEAD...origin/main` |
| Working tree | pulito, salvo il kit stesso untracked | `git status --porcelain` |
| Test `RefactorTactics.PlayerInput.*` in `Source/` | **16** | `git grep -ohE '"RefactorTactics\.PlayerInput\.[A-Za-z0-9]+"' -- Source \| sort -u` |
| Comandi console `rt.*` | **26** | `grep -rhoE '"rt\.[A-Za-z.]+"' Source/ \| sort -u` |
| `L_DevSandbox` | esiste: `Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap` | `find` |
| Materiale cella | esiste: `Content/RT/Core/Grid/M_HexCell.uasset` | `find` |

⚠️ **I 7 commit di divergenza non toccano nulla che questo referto misuri.** Sono
`identita-di-squadra-spec.md` (nuovo), `editor-sessions.yaml`, `test-manuali-pie.md`, tre cancellazioni di
documenti storici e `fix(1515)` su `ScenarioHarness`/`RTTurnRules`. Nessuno tocca `Map/`, `Player/`,
`spec-pointer-interaction.md` o `roadmap-v0.1.md`. Per la regola di
[`CLAUDE.md`](../../../CLAUDE.md) §7 il worktree qui sarebbe costo puro: si lavora sul checkout e **si
dichiara la divergenza**, che è questa riga.

🔁 **E la divergenza si è chiusa da sola mentre il lavoro era in corso.** Misurato: `HEAD` è passato a
`201016a6` *(«tactical grid docs»)*, che ha unito i sette commit e committato i file di questo giro; la
divergenza da `origin/main` è ora `0/0`. Le misure di §5 e §7 restano riferite a **`d9feb9b0`** — è l'albero
su cui sono state prese, e nessuno dei sette commit tocca `Map/`, `Player/` o gli owner citati, quindi
valgono ancora. Ciò che **non** vale più è lo stato dei gate: vedi §9.

---

## 3. Il precedente — e ce n'è uno, e non è su `main`

Prima di revisionare: il kit riscopre un terreno già battuto **due giorni fa**.

| Reperto già registrato | Dove | Stato |
|---|---|---|
| Overlay hover maggiorato, `HoveredCell` separata dalla selezione | [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614) — `OPEN`, `P1`, `v0.1`, milestone *v0.1 · Leggibilità* | aperta il **2026-08-28** |
| Il contratto del puntatore per intero (hover · LMB · RMB) | [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) — `OPEN`, `checkpoint`, `P1` | CP 11.8, owner `spec-pointer-interaction.md` |
| Griglia di lavoro visibile prima di disegnare (lato **editor**) | [#622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/622) — `OPEN`, `P2` | sub-issue di #1105, stadio TD 0.1 |
| Il settore sotto il cursore, dodici spicchi | [#1615](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1615) — `OPEN` | stessa milestone |
| **Gli otto test `PlayerInput.*` dichiarati verdi e inesistenti** | [#1616](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1616) — **`CLOSED` il 2026-08-29** | è esattamente l'assunzione del kit §14 |

⚠️ **E il referto d'origine di quelle issue non è su `origin/main`.** #1614 e #1616 citano entrambe
`docs/roadmap/plans/hover-cell-sector12-spec-panel-2026-08-28.md` con la nota *«referto non ancora
committato al momento dell'apertura»*. Misurato: quel file **esiste** — commit `fe4af8bf` del 2026-08-29 —
ma è raggiungibile **solo** da `origin/docs/consolidamento-combat-skillgrammar-delta`, e
`git cat-file -e origin/main:…` risponde `does not exist`. Due issue aperte in `v0.1` citano come propria
origine un documento che il ramo principale non contiene. Non è un difetto di questo kit; è il contesto in
cui atterra, e va detto perché chi cerca il precedente sul ramo principale **non lo trova**.

---

## 4. Il panel

Cinque esperti, scelti sul contenuto: il kit è per **due terzi** requisiti e DoD, per un terzo architettura
di presentazione, e chiude su gate operativi.

### 📚 KARL WIEGERS — qualità dei requisiti

> *«Tre delle vostre DoD non sono requisiti: sono fotografie di uno stato. "Nessun Actor per cella" compare
> in §0.7, in §4-A e in §22 — e nel codice è l'invariante d'apertura della classe, scritta nel doc-comment di
> `ARTHexMapActor`: «genera un'ISTANZA per cella (ISM), NON un Actor per cella». Una casella che nessun
> lavoro può spuntare perché è già spuntata non misura niente: consuma attenzione e produce un ✅ che sembra
> una consegna.*
>
> *E la §11 introduce un termine che il glossario del progetto non ha. `HoveredCell == SelectedCell after
> valid LMB` presuppone una «SelectedCell». In questo dominio la selezione è di **unità** — lo pinna
> `PlayerInput.RightClickNeverDeselects` — e il click su una cella produce, secondo il contesto, un
> waypoint, un `PlannedAttackCell` o un `Inspect`. Un requisito che nomina un'entità inesistente non è
> ambiguo: è **non verificabile**, e chi lo implementa deve inventare la cosa da misurare.»*

### 📊 MARTIN FOWLER — confine e ownership

> *«Il vostro §8 offre due preferenze — estendere il renderer o aggiungere un component — e sceglie bene fra
> le due. Il problema è a monte: entrambe presuppongono un modello in cui **la griglia sta sopra la mappa**.
> Qui la griglia **è** la mappa. Le celle sono prismi esagonali istanziati, e il loro materiale trasporta
> già informazione tattica su tre canali di `PerInstanceCustomData`, sopra i quali stanno un glifo ad anelli
> per superficie e un velo di fog of war che moltiplica l'RGB.*
>
> *Aggiungere un fill translucido `0,12` sopra ogni cella non è "un overlay": è un secondo insieme di
> istanze grande quanto la board, posato sopra il canale che porta il significato. Il kit non nomina né il
> glifo né il velo. Chi lo esegue alla lettera scopre a schermo che il ricordo di fog of war — RGB × `0,35`
> — e il contrasto d'area del glifo, misurato leggibile a picco in una seduta, sono entrambi attenuati da
> una patina che nessuno ha collegato a loro.»*

### 🎲 MICHAEL NYGARD — gate e operatività

> *«"Packaged Development verificato" compare come DoD in tutte e tre le issue proposte. Andate a leggere
> `v0.1-definition-of-done.md`, gate `G2`: la suite di questo progetto è **Editor-only per costruzione** —
> `1373` dichiarazioni `EditorContext` contro `11` `ClientContext`. Un test `GridOverlay.*` nuovo nasce
> `EditorContext` e **non è raggiungibile** da un binario impacchettato. La vostra casella non è severa:
> è **insoddisfabile**, e una casella insoddisfabile in ogni issue si spunta per stanchezza.*
>
> *Il gate packaged qui è di release — `G2`, `G13` — e ha già la sua storia: `-nullrhi` vietato sui dati
> cotti, un crash GPU allo shutdown che impedisce di leggere il verdetto dall'exit code. È lavoro reale, ed
> è di qualcun altro.»*

### 📖 LISA CRISPIN — testabilità

> *«§14 dice: "Se esistono già test come `PlayerInput.HoverNeverCommits`, `HUDConsumesPointerBeforeWorld`,
> `HiddenEnemyCannotBecomeHoverTarget`, mantienili e assicurati che continuino a passare".*
>
> ***Nessuno dei tre esiste.*** *E non è una svista fresca: è il difetto per cui è stata aperta e chiusa
> [#1616](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1616) il 29 agosto. Un'istruzione
> che dice "assicurati che continuino a passare" su un test assente ha un esito prevedibile — si esegue il
> filtro, non fallisce niente, si scrive verde. È il modo più efficiente che conosco di produrre una prova
> falsa.*
>
> *La parte buona: `GridOverlay.ToggleDoesNotMutateMapState` e `GridOverlay.HoverChangesOnlyOnCellChange`
> sono **oracoli veri**, verificabili senza occhio umano. Il secondo esiste già, meglio formulato, nella DoD
> di #1614: "attraversando N celle, `HoveredCell` cambia valore al più una volta per cella".»*

### ✏️ GOJKO ADZIC — esempi e numeri

> *«I sei numeri della §9 sono l'unica parte del kit che nessun'altra fonte contiene, quindi sono la parte
> che va guardata più da vicino. Sono in centimetri assoluti. Questo repository ha smesso di scrivere
> altezze in centimetri assoluti il 28 agosto, e ha lasciato scritto perché: `RTCellThicknessInH` è
> `0.06 H` — non `5 uu` — perché quando `HexSize` è passato da `100` a `150` uno spessore assoluto è
> rimasto fermo e il rapporto è sceso dal `2,63%` all'`1,75%` **senza una riga di diff che lo dicesse**.*
>
> *E il primo dei sei numeri è peggio che fragile: è sotto la superficie. Ve lo mostro con l'esempio, che è
> il mio mestiere.*
>
> ```text
>   Dato   RTCellTopZ = 7,5 uu          (faccia superiore del tile, dal 2026-08-28)
>   Quando si posa il fill a GridLiftCm = 2,0
>   Allora il fill sta 5,5 uu DENTRO un volume opaco
>   E a schermo non si distingue da qualcosa che non è stato disegnato affatto
> ```
>
> *Non è una previsione: il doc-comment di quel file elenca **due** episodi in cui è successo, uno dei quali
> trovato in code review con la suite verde.»*

---

## 5. I rilievi

Ordinati per gravità. Ogni riga porta il modo di rifarla.

| # | Sev | Rilievo | Evidenza |
|---|---|---|---|
| **R-1** | 🔴 | **§14 chiede di mantenere verdi tre test che non esistono.** `HoverNeverCommits`, `HUDConsumesPointerBeforeWorld`, `HiddenEnemyCannotBecomeHoverTarget`: zero occorrenze. I `PlayerInput.*` reali sono **16** e nessuno contiene `Hover`, `HUD` o `HiddenEnemy`. Eseguito alla lettera, il kit produce un verde su una prova assente | `git grep -l 'HoverNeverCommits' -- Source/` → vuoto · già tracciato e **chiuso** da [#1616](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1616) |
| **R-2** | 🔴 | **§9 `GridLiftCm = 2.0` disegna dentro il tile.** La faccia superiore è `RTCellTopZ = RTCellPrismRadius · RTCellFlatScale` = **7,5 uu** dal 2026-08-28 (era `2,5`). Un lift di `2,0` finisce `5,5` uu sotto. E i sei default in **cm assoluti** contraddicono `D-168`/`D-163`: le altezze si budgettano in frazioni di `H`. 🔴 **E non è una previsione: `2.0` è il numero esatto che ha già causato l'incidente.** `PIE-DEBUG-CELLS`, 2026-08-07: *«il contorno superficie era disegnato a `z=2.0` mentre la faccia del disco-cella sta a `2.5` … e restava **sepolto nella mesh**»* — il fango non si vedeva. Corretto in `069b616` facendo derivare le quote da `RTCellTopZ`; poi `RTCellTopZ` è **triplicato**, quindi oggi lo stesso numero sbaglia di più | [`RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h) — *«chi disegna qualcosa SOTTO `RTCellTopZ` lo disegna dentro un volume opaco … È successo davvero, due volte»* · [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) riga `748` |
| **R-3** | 🔴 | **Issue A e B esistono già, e la B ha una DoD migliore.** [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614) copre hover visibile, separazione hover/selezione, no-Actor-per-cella, debug `HoveredCell`, `PIE-V01-POINTER`; [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) possiede il contratto. Il kit ne propone la creazione senza vederle | `gh issue list --search "hover in:title" --state all` → esattamente #705 e #1614 |
| **R-4** | 🔴 | **La §3 «Decisione Epic» ha come risposta il proprio Caso A.** [E11 #25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25) possiede #705/#1614/#613/#80; [E21 #286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) possiede *Presentazione e leggibilità*. Milestone già esistente: **v0.1 · Leggibilità**, 20 issue aperte. **Nessuna Epic da creare** | `gh issue list --milestone "v0.1 · Leggibilità"` |
| **R-5** | 🔴 | **Il modello «griglia sopra la mappa» non è quello del repository.** La board *è* le celle: ISM di prismi, `M_HexCell` legge tre `PerInstanceCustomData` per il colore di superficie, sopra ci sono il glifo ad anelli (`D-183`) e il velo di fog of war che moltiplica l'RGB per `0,35` (`D-225`/`D-227`). Un fill translucido su tutta la board attenua entrambi i canali, e il kit non li nomina | `ARTHexMapActor::RebuildInstances` monta **cinque** famiglie di istanze: `Cells`, `SurfaceGlyphs`, `Relief`, `Blockers`, `EdgeFeatures` |
| **R-6** | 🟠 | **`G` è occupato: `Action.Guard`.** Il kit §7 prevede il caso, ma la conclusione giusta non è «scegli un altro tasto»: il toggle **esiste** come `rt.Debug.DrawCells` (CP 11.4, [#80](https://github.com/DegrassiAaron/refactor-tactics-main/issues/80)), fra i **26** comandi `rt.*` | `RTPlayerController.cpp:201` — `{ TEXT("Action.Guard"), EKeys::G }` |
| **R-7** | 🟠 | **«Packaged Development» come DoD per-issue è insoddisfabile.** Suite Editor-only per costruzione: **1373** `EditorContext` contro **11** `ClientContext`, `0` `ApplicationContextMask`. Un `GridOverlay.*` nuovo non sarebbe raggiungibile da un packaged senza dichiararlo `ClientContext`, e il gate packaged della v0.1 è `G2`/`G13`, di release | [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) `G2`, misura del 2026-08-29 |
| **R-8** | 🟠 | **§11 «`HoveredCell == SelectedCell` after valid LMB» non è il contratto.** `D-128`: in Planning nessuna abilità è armata e il click su un nemico **ispeziona**; in `Targeting`/`Cell` produce `PlannedAttackCell`; in `Pathing` un waypoint. Non esiste una `SelectedCell`, e `RightClickNeverDeselects` pinna che la selezione è di unità | [`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md) §4, §5.5 · `URTPointerLibrary::ResolveTarget` |
| **R-9** | 🟠 | **§13 «multilayer safety» è consegnato da mesi.** `ERTLayerViewMode{AllLayers, ActiveOnly, Focus}` + `ActiveLayer` + `GhostLayerRange` (#567); i piani di contesto **non producono istanze**, quindi non hanno collisione e il raycast non li può colpire — che è precisamente *«layer nascosti non sono pickati accidentalmente»*. E il pick guarda il **componente**, non l'actor (#588, chiusa da PR #598) | `RTHexMapActor.h` · `IsPickOnSelectableCell` |
| **R-10** | 🟠 | **Tre DoD ripetono uno stato, non un lavoro.** «Nessun Actor per cella» è l'invariante d'apertura della classe, dichiarata nel doc-comment. Spuntarla non misura nulla | `ARTHexMapActor` doc-comment |
| **R-11** | ⚠️ | **§4 chiede «2–4 issue reali»; il perimetro non tracciato è UNA casella.** Tutto il resto è o consegnato o dentro #1614/#705/#622 | §7 di questo referto |
| **R-12** | ⚠️ | **`GridOverlay.InstanceToCellIdIsStable` va scritto sapendo cosa fa oggi `CellForInstance`.** Fuori range risponde `FRTCellId()` — cioè `(0,0,0)`, una cella **valida**, non un sentinella. Un test che asserisce la stabilità senza validare l'indice prima passa su un indice inesistente | `RTHexMapActor.h` — l'avvertimento è scritto sulla funzione |

---

## 6. Cosa sopravvive del kit

Poco, ma non zero, e va detto per intero.

- ✅ **La §0 come metodo.** `SEARCH BEFORE CREATE` / `REUSE BEFORE DUPLICATE` / `MEASURE BEFORE CLOSE`, il
  divieto di inventare numeri issue e funzioni MCP, i path dichiarati «solo come hint», e — la migliore —
  *«Se un gate non viene eseguito, scrivere `NOT RUN` con motivazione. Mai dichiararlo verde»*. È la stessa
  disciplina con cui questo referto lo ha smontato.
- ✅ **§9, il divieto di `Disable Depth Test`.** Vedere la griglia attraverso i muri è una perdita di
  informazione tattica, e il kit lo vieta esplicitamente. Coerente con il repository, dove l'unico
  `SDPG_Foreground` è motivato in commento e limitato a due casi.
- ✅ **§12, «non distruggere/ribuildare asset ad ogni toggle»** e l'elenco di ciò che il toggle non deve
  mutare (`FRTMapState`, graph revision, path cache, snapshot, TurnLog). È l'invariante #1 del renderer,
  scritta con parole diverse.
- ✅ **§10, «niente rebuild di tutte le istanze» sul movimento del mouse.** Oggi è vero per costruzione —
  l'hover è una debug-line, non un rebuild — ma diventa una regola che morde nell'istante in cui qualcuno
  passa a istanze, ed è la §5 del kit a proporlo.
- ✅ **§17 e §18, deduplicazione e reopen policy.** *«Non riaprire issue storiche se è più corretto creare
  una child issue moderna con ownership chiara»* è esattamente ciò che #1614 ha fatto rispetto a #705, due
  giorni prima che il kit lo raccomandasse.

---

## 7. Il contributo netto — una casella, e non è dove il kit la mette

⚠️ **`FRTCellId` non si legge da nessuna parte a schermo.**

```sh
grep -rn "HoveredCell" Source/RefactorTactics/Debug/ Source/RefactorTactics/UI/    # → zero
```

Il kit §11 prescrive:

```text
Hovered Cell: (X,Y,L)
Selected Cell: (X,Y,L)
Grid: ON/OFF
```

e chiude con *«Riusa l'overlay/debug HUD corrente»*. **Quel consumatore non esiste**: `ARTHexMapActor`
espone già `GetHoveredCell()` e `IsHoveredCellValid()` — dichiarati «diagnostica e test» — e nessuno li
legge fuori dai test. Il dato c'è, la superficie che lo mostra no.

È tracciato — è una casella della DoD di #1614, *«Il debug Development mostra almeno `HoveredCell` e il suo
centro world»* — quindi non è una issue nuova. Ma è la **sola** riga del kit che descrive lavoro reale, non
consegnato, e piccolo.

➕ **E misurando per scriverla è emerso un reperto che nessuno dei due documenti ha, e che falsifica una
premessa di #1614.**

#1614 afferma: *«l'evidenziazione oggi è un `DrawCellOutline` in giallo (`RTHexMapActor.cpp:718`), grande
esattamente quanto la cella»*. Misurato su `d9feb9b0`:

```cpp
// RTHexMapActor.cpp:994
DrawCellOutline(HoveredCell, FColor::Yellow, 0.88f, /*bThroughUnits=*/ true);
```

e `DrawCellOutline` compone `HexCorners(Center, Size * Scale)`, mentre la cella canonica è
`CellCorners = HexCorners(Center, HexSize)` (`RTHexLibrary.cpp:128`). L'overlay hover è quindi a **`0,88×`**:
**più piccolo della cella del 12%**, non uguale. Tre conseguenze, tutte per #1614:

1. la baseline «`1,15×`» non è un `+15%`: rispetto a ciò che si vede oggi è **`+31%`** (`1,15 / 0,88`), ed è
   quello il salto che il giocatore percepirà;
2. il numero di riga citato (`:718`) è **scaduto** — oggi `:994`;
3. l'hover **attraversa già le unità** (`SDPG_Foreground`, `RTHexMapActor.cpp:947`, con la ragione scritta in
   commento: *«si punta un bersaglio molto più spesso di una cella vuota»*), e #1614 non lo registra fra ciò
   che esiste — chi riscrive il visualizzatore rischia di perderlo.

🔵 **Tutti e tre sono stati portati in #1614 il 2026-08-30** (§8), insieme a un quarto emerso applicando: lo
Scope della issue cita `URTHexLibrary::HexVertices`, che **non esiste**.

---

## 8. Azioni raccomandate — due applicate, tre no

`A-1` e `A-2` sono state **applicate a [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614)**
il 2026-08-30, su autorizzazione esplicita dell'autore data alla consegna di questo referto — atto **separato**
dalla revisione, nella forma che questo archivio incontra di rado: un kit *consumato* e poi, come decisione a
parte, *eseguito in ciò che restava vero*. Le altre tre restano elenco.

| # | Azione | Stato |
|---|---|---|
| **A-1** | Correggere la premessa di #1614 — overlay `0,88×` e non `1,00×`, `RTHexMapActor.cpp:994` e non `:718` — e registrare che l'hover attraversa già le unità | 🔵 **applicata**. Vedi sotto |
| **A-2** | Dichiarare il vincolo di quota: sopra `RTCellTopZ`, in frazioni di `H` e non in cm | 🔵 **applicata, e più precisa di come era raccomandata**. Vedi sotto |
| **A-3** | Nessuna Epic, nessuna issue nuova per grid/hover/selezione/toggle/multilayer | 🔵 **superata da una decisione dell'autore, e in parte confermata**: nessuna Epic — resta il Caso A — ma **una** issue è stata aperta, [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758). Vedi §10 |
| **A-4** | Nessuna modifica alla roadmap | 🔵 **superata, e nella forma che non duplica**: creata [`tactical-grid-overlay-roadmap-2026-08-30.md`](tactical-grid-overlay-roadmap-2026-08-30.md), che **punta** a CP 11.8 invece di riscriverlo. `roadmap-v0.1.md` **non è stata toccata**. Vedi §10 |
| **A-5** | Portare `hover-cell-sector12-spec-panel-2026-08-28.md` su `main` | ⛔ **non eseguita** — §3, ed è di chi possiede quel branch |

### Cosa è cambiato in #1614, e cosa no

**Corretti in loco**, con la misura accanto: la premessa di §«Perché» (`0,88×`, `:994`, e il delta reale
`+31%` invece di `+15%`), e il nome della funzione di geometria nello Scope. Aggiunte **due** voci di Scope
e **due** caselle di DoD — da `10` a `12` — entrambe di **conservazione**: la quota e `SDPG_Foreground` sono
comportamenti che oggi esistono e che un visualizzatore riscritto da zero perderebbe in silenzio. In fondo,
un blocco datato con la tabella *diceva / dice il codice*.

⛔ **Nulla è stato rimosso**: nessuna casella tolta, nessun requisito nuovo, Non-goals e Chiusura intatti.
Stato, label (`v0.1`, `P1`) e milestone (*v0.1 · Leggibilità*) invariati — riverificati dopo la scrittura,
e il corpo remoto è `IDENTICO` a quello inviato (135 righe, `12` caselle).

➕ **Applicando è emerso un terzo reperto che il referto non aveva**, ed è il motivo per cui `A-2` è
atterrata più precisa di come era scritta:

| Reperto | Misura |
|---|---|
| **`A-2` non serviva a prevenire un errore futuro: la costante esiste già.** L'hover sta a `RTLiftPreview = RTCellTopZ + 2,5` uu | `RTHexMapActor.cpp:128` — accanto a superficie `+0,5`, glifo `+0,3`, marker `+1,5` |
| **`URTHexLibrary::HexVertices`, citata nello Scope di #1614, NON esiste** — zero occorrenze in `Source/`. La funzione è `HexCorners`; la riga citata (`:379`) è però **quella giusta**, l'angolo `−30°` | `git grep -n "HexVertices" -- Source/` → un solo riscontro, e è il nome di un test |

Il primo cambia la forma della raccomandazione — da *«dichiara un vincolo»* a **«riusa la costante, non
scriverne una»**, che è più forte e più facile da verificare. Il secondo è una citazione storpiata, la stessa
classe di difetto che una code review su questo referto ha già trovato una volta.

⛔ **Non eseguito, e deliberatamente**: l'intero §16 (workflow issue-driven), §19 (branch/commit/PR), §20
(consistency check) e §21 (report finale con `HEAD before`/`after`, `FILES CHANGED`, `ASSETS CHANGED`).
`/sc:spec-panel` non implementa, e [`CLAUDE.md`](../../../CLAUDE.md) §9 vieta commit/push/merge senza
richiesta esplicita.

⛔ **Nessun `D-nnn` assegnato.** L'ultimo è **`D-248`** (`origin/main`, verificato anche sui **18** ref
remoti: nessuno rivendica oltre). Non c'è una decisione nuova da registrare — il verdetto è «il kit riscopre
lavoro tracciato», che è un fatto, non una scelta di progetto — e prendere un ID per non dire nulla è
esattamente la collisione che `CLAUDE.md` §7 previene.

---

## 9. Gate eseguiti

| Gate | Esito |
|---|---|
| `node tools/radar/doc-links.ts --check --with-archive` | ✅ **verde su `d9feb9b0`** — `4861` link in `470` documenti, *«tutti i percorsi citati risolvono»*. 🔴 **Rosso su `201016a6`, per una causa esterna a questo lavoro**: vedi sotto |
| `node tools/radar/doc-tables.ts --check` | ✅ **verde** — `1846` tabelle in `296` documenti |
| `node tools/radar/doc-tables.ts --check --with-archive` | 🟡 **10 segnalazioni, tutte preesistenti e nessuna di questo giro** — nove in `archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md` (diagrammi ASCII dentro celle) e una in `archive/src/handoff/2026-08-08-master-reaction-system.md:569`. Verificato con `grep -E "archive/src/README\|tactical-grid-overlay"` sull'output: nessun riscontro. ⚠️ **Il gate senza `--with-archive` non avrebbe guardato la riga d'indice**, che vive in `docs/archive/` |
| `./scripts/rt-suite.ps1` | ⛔ **NOT RUN** — lavoro puramente documentale, nessuna riga di `Source/` toccata. Eseguirla renderebbe la misura NON VALIDA per un altro checkout senza guadagno |
| Build Editor | ⛔ **NOT RUN** — stessa ragione |
| PIE / packaged | ⛔ **NOT RUN** — nessuna implementazione da verificare |

---

> 🔴 **Il gate dei link è diventato rosso mentre il lavoro era in corso — e la causa non è un file di questo
giro: è una decisione accettata oggi che un commit di nove ore dopo ha disfatto.**

Misurato su `201016a6`: **9 link rotti in 6 documenti**, e tutti puntano ai **tre** file che
[**D-246**](../../decisions/RT_PDR_00_Decision_Log.md) dichiara di aver *spostato*, *promosso a owner* e
*archiviato*. Il commit `c0cc0693` (*«docs»*, 2026-08-30 **10:18**) li ha **cancellati** — nove ore dopo
`cc9d30ff` (**01:19**), il commit che introduce D-246 e la registra **Accettata**, con la riga
*«Gate verdi, rimisurati dopo il rebase»*.

| Il file, e cosa D-246 dice di lui | Oggi | Chi lo cita e resta rotto |
|---|---|---|
| `product/lore-e-worldbuilding.md` — *«**promosso a owner**»* | **assente** | `README.md` · **`RT_PDR_00_Decision_Log.md`** · `OPEN_DECISIONS.md` · `CONTEXT_INDEX.md` · `CHANGELOG_DOCUMENTATION.md` |
| `roadmap/plans/dir-b-core-gameplay-directive-v0.2.md` — *«**spostata** accanto al referto che la corregge»* | **assente** | il suo referto (×2) · `CHANGELOG_DOCUMENTATION.md` |
| `archive/src/RefactorTactics_FeatureRegistry_…_2026-08-08.md` — *«**archiviato** col banner d'esito»* | **assente** | `CHANGELOG_DOCUMENTATION.md` |

🔴 **Il caso peggiore è il Decision Log**: D-246 cita `../product/lore-e-worldbuilding.md` per dire che il
worldbuilding *«ora vive»* lì, e quel percorso non risolve. Una decisione accettata che descrive il proprio
esito in un file cancellato non è una svista di link: è la decisione che non è più verificabile leggendola.

⚠️ **E nessuno l'ha visto perché non c'è CI** (`D-182`): il gate si lancia a mano, e chi ha cancellato non
l'ha lanciato. È il difetto che **D-246 stessa aveva appena finito di diagnosticare** — *«nessun gate
guardava quei sei file»* — ricomparso dall'altro lato: ora i file sono nei posti che i gate guardano, e sono
stati tolti.

✅ **Riparato il 2026-08-30, su richiesta dell'autore** — terzo atto, ancora separato dalla revisione.

La scelta fra le due riparazioni possibili è stata presa **misurando**, non per default: si ripristinano i
file **se** D-246 è ancora vigente. Verificato — nessuna decisione la ritira (`D-247` è `ScoreToWin`, `D-248`
il grafo di locomozione), e [`D-188`](../../decisions/RT_PDR_00_Decision_Log.md) la cita come vigente
descrivendone l'esito come vero. `c0cc0693` è un commit diretto, messaggio *«docs»*, **nessun body**: la
cancellazione non ha un rationale da opporre a una decisione che ne ha uno di quaranta righe.

- **Ripristino esatto, non ricostruzione**: i tre blob di `c0cc0693^` sono **identici** a quelli di
  `cc9d30ff` — confrontati con `git rev-parse` file per file, prima di toccare qualcosa.
- **Gate**: `doc-links --check --with-archive` da **9** segnalazioni a **zero**; `4883` link in `472`
  documenti.
- **Registrato dove conta**: una nota datata dentro **D-246** stessa, che è la voce che l'episodio aveva reso
  non verificabile.
- ⛔ **Non toccate le 8 righe** che `c0cc0693` toglie a `RTScenarioLoader.cpp`: sono codice, altra materia, e
  questa riparazione non le giudica.

🔁 **Rilanciati dopo il secondo atto (§10)**, che ha aggiunto un documento e una riga d'indice:
> `doc-links.ts --check --with-archive` ✅, `doc-tables.ts --check --with-archive` invariato a **10**
> segnalazioni preesistenti, nessuna sui file di questo giro.

---

## 10. Il secondo atto — issue e roadmap, su richiesta dell'autore

Questa sezione **non** è parte della revisione: registra un lavoro deciso dopo, che il referto aveva
raccomandato di **non** fare (`A-3`, `A-4`). La riserva è stata sollevata, l'autore ha deciso diversamente, e
il lavoro è stato eseguito per intero. È la forma che questo archivio incontra di rado: un kit *consumato* e
poi, come atto separato, *eseguito in ciò che restava vero*.

### Il reperto che ha cambiato il piano

Cercando cosa creare, il perimetro «griglia semi-trasparente» si è rivelato **già di un owner**, e non di
quelli che il referto aveva elencato in §3:

> **[#289](https://github.com/DegrassiAaron/refactor-tactics-main/issues/289) — CP E21.3, Leggibilità
> tattica**, seconda casella di DoD: *«I colori delle superfici si leggono **durante la partita**. Oggi
> esistono solo dentro `rt.Debug.DrawCells` … un giocatore non deve digitare un comando console per sapere
> dove c'è il fango.»*

Quindi nemmeno la grid è un dominio scoperto: `OPEN`, `P1`, `checkpoint`, stessa milestone. **Il Caso A del
kit valeva anche qui**, ed è la quinta volta che questo referto lo verifica.

➕ **Ma #289 nomina un canale solo, e ne servono due.** Misurato: `RebuildInstances` monta `Cells`,
`SurfaceGlyphs`, `Relief`, `Blockers`, `EdgeFeatures` — **nessuna disegna un bordo**. Due celle adiacenti
della **stessa** superficie sono due prismi dello stesso colore appoggiati l'uno all'altro: il colore non
dice dove finisce una e comincia l'altra, e in un gioco dove il movimento si conta in celle chi non vede il
confine non può contare il costo. Il colore per superficie e il confine fra celle sono canali **diversi**, e
la DoD di #289 ne chiedeva uno.

### Cosa è stato fatto

| Atto | Oggetto | Nota |
|---|---|---|
| **Creata** | [#1758](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1758) — *«La griglia si vede in partita: il confine fra celle, e un toggle che non è un comando di debug»* | `v0.1`, `P1`, milestone *v0.1 · Leggibilità*, **parent #289**, Epic #286. **Una sola**, non le 2–4 del kit |
| **Aggiornata** | [#289](https://github.com/DegrassiAaron/refactor-tactics-main/issues/289) | Task residuale + la casella del **confine**, che mancava, con il vincolo di quota e il precedente `PIE-DEBUG-CELLS` |
| **Aggiornata** | [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) (Epic E21) | #1758 in «Lavoro collegato», con il perché non è un checkpoint e perché non è E11 |
| **Creata** | [`tactical-grid-overlay-roadmap-2026-08-30.md`](tactical-grid-overlay-roadmap-2026-08-30.md) | Sequenza a **6 nodi** con owner, stato misurato, dipendenza ed exit gate |
| ⛔ **NON toccata** | [`roadmap-v0.1.md`](../roadmap-v0.1.md) | `A-4` regge nella sostanza: CP 11.8 alla riga `887` ha la colonna evidenza riconciliata il 2026-08-29, e una voce accanto sarebbe la **seconda descrizione** dello stesso perimetro |
| ⛔ **NON creata** | nessuna Epic | `A-3` regge: E21 #286 e E11 #25 possiedono già tutto |

### La sequenza, e dove diverge dal kit

Il kit §6 metteva il renderer al nodo 1 e i test al nodo 5. La roadmap li inverte, e non per gusto:

> **Il nodo 1 è `PlayerInput.HoverNeverCommits`, e lo impone #1614**, che dichiara *«un overlay più grande
> rende più visibile un comportamento che nessuno sta misurando. Il test viene prima, o almeno insieme»*.

Gli altri due scarti: **multilayer non è un nodo** (consegnato da #567/#588), e **packaged non è un gate per
nodo** (`G2`: suite `EditorContext` per costruzione — R-7).

🟢 **Un ramo può partire oggi senza precondizioni**: #1758, che disegna la board a riposo e non dipende dal
puntatore.

---

## 11. Provenienza

Kit ricevuto come `CLAUDE_RT_TacticalGridOverlay_IssueDriven_Execution.md` nella radice del checkout di
lavoro, 912 righe, 22 sezioni, senza data e senza riferimento a un `HEAD`. Archiviato **verbatim** in
[`../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md`](../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md),
con `diff -q` prima della rimozione dell'originale.

Panel: Wiegers (requisiti) · Fowler (architettura) · Nygard (gate) · Crispin (testabilità) · Adzic (esempi).
Modo `critique`, focus `requirements + architecture`, una iterazione.
