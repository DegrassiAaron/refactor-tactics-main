# Tactical Grid Overlay — Issue-Driven Execution — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma il kit
> *«CLAUDE — RefactorTactics · Tactical Grid Overlay — Issue-Driven Execution Plan»* (912 righe, 22 sezioni),
> arrivato come file nella radice del checkout di lavoro.
>
> **Data**: 2026-08-30 · **Base**: `main` @ `d9feb9b0`, **7 commit dietro** `origin/main` @ `aec66789` ·
> **Modo**: critique · **Focus**: requirements + architecture
>
> **Cosa è**: il verdetto su un **mandato esecutivo end-to-end** — audit GitHub, creazione/riuso di issue,
> aggiornamento roadmap, implementazione C++/asset, PIE, packaged. `/sc:spec-panel` è task documentale
> ([`CLAUDE.md`](../../../CLAUDE.md) §6): **nessuna issue creata, chiusa o riaperta**, **nessun `D-nnn`
> assegnato**, **nessuna riga di codice o asset toccata**. Le azioni raccomandate stanno in §8 come elenco.
>
> 🔵 **Una sola scrittura, e dopo il referto**: su autorizzazione esplicita dell'autore alla consegna, `A-1` e
> `A-2` sono state applicate al **corpo di [#1614](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1614)**
> — la sola mutazione GitHub di questo giro. Dettaglio e verifica in §8.
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
| **R-2** | 🔴 | **§9 `GridLiftCm = 2.0` disegna dentro il tile.** La faccia superiore è `RTCellTopZ = RTCellPrismRadius · RTCellFlatScale` = **7,5 uu** dal 2026-08-28 (era `2,5`). Un lift di `2,0` finisce `5,5` uu sotto. E i sei default in **cm assoluti** contraddicono `D-168`/`D-163`: le altezze si budgettano in frazioni di `H` | [`RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h) — *«chi disegna qualcosa SOTTO `RTCellTopZ` lo disegna dentro un volume opaco … È successo davvero, due volte»* |
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
| **A-3** | Nessuna Epic, nessuna issue nuova per grid/hover/selezione/toggle/multilayer | ⛔ **non eseguita** — R-3, R-4, R-9: il dominio ha già quattro owner aperti nella milestone giusta |
| **A-4** | Nessuna modifica alla roadmap | ⛔ **non eseguita** — CP 11.8 è in `roadmap-v0.1.md:887` con la colonna evidenza **già riconciliata** il 2026-08-29: elenca i **6** test che esistono e i **10** dichiarati e non scritti. Aggiungere una feature «Tactical Grid Overlay» accanto duplicherebbe CP 11.8 |
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
| `node tools/radar/doc-links.ts --check --with-archive` | ✅ **verde** — `4861` link in `470` documenti, *«tutti i percorsi citati risolvono»* |
| `node tools/radar/doc-tables.ts --check` | ✅ **verde** — `1846` tabelle in `296` documenti |
| `node tools/radar/doc-tables.ts --check --with-archive` | 🟡 **10 segnalazioni, tutte preesistenti e nessuna di questo giro** — nove in `archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md` (diagrammi ASCII dentro celle) e una in `archive/src/handoff/2026-08-08-master-reaction-system.md:569`. Verificato con `grep -E "archive/src/README\|tactical-grid-overlay"` sull'output: nessun riscontro. ⚠️ **Il gate senza `--with-archive` non avrebbe guardato la riga d'indice**, che vive in `docs/archive/` |
| `./scripts/rt-suite.ps1` | ⛔ **NOT RUN** — lavoro puramente documentale, nessuna riga di `Source/` toccata. Eseguirla renderebbe la misura NON VALIDA per un altro checkout senza guadagno |
| Build Editor | ⛔ **NOT RUN** — stessa ragione |
| PIE / packaged | ⛔ **NOT RUN** — nessuna implementazione da verificare |

---

## 10. Provenienza

Kit ricevuto come `CLAUDE_RT_TacticalGridOverlay_IssueDriven_Execution.md` nella radice del checkout di
lavoro, 912 righe, 22 sezioni, senza data e senza riferimento a un `HEAD`. Archiviato **verbatim** in
[`../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md`](../../archive/src/handoff/2026-08-30-tactical-grid-overlay-issue-driven.md),
con `diff -q` prima della rimozione dell'originale.

Panel: Wiegers (requisiti) · Fowler (architettura) · Nygard (gate) · Crispin (testabilità) · Adzic (esempi).
Modo `critique`, focus `requirements + architecture`, una iterazione.
