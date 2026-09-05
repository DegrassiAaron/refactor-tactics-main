# HUD v0.1 — audit `RT Three Terminals` (2026-09-05)

> `EPHEMERAL` · **Non è owner di niente.** Fotografia datata, prodotta consumando un handoff esterno
> (`CLAUDE_HUD_V01_RT_THREE_TERMINALS_HANDOFF.md`, ora consumato). Lo stato ufficiale resta negli owner
> citati riga per riga: issue GitHub, [`roadmap-v0.1.md`](../roadmap-v0.1.md),
> [`roadmap-checkpoint.md`](../roadmap-checkpoint.md),
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), [`editor-sessions.yaml`](../editor-sessions.yaml),
> [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md).
>
> **Chi lo apre fra una settimana lo legga come un audit, non come uno stato.**

## 0. Come è stato misurato

| Voce | Valore |
|---|---|
| Data | 2026-09-05, 13:50–14:20 |
| Repository | `DegrassiAaron/refactor-tactics-main` |
| Checkout della misura | `D:\Repositories\rt-wt-hudv01` — worktree **sparse** (`docs Source Scenarios tools scripts Config`) di `D:\Repositories\refactor-tactics-main` |
| Branch | `docs/hud-v01-three-terminals-roadmaps` |
| `HEAD` = `origin/main` alla misura | `8a530c6e` (2026-09-05 13:14, merge PR #2438) |
| Modalità globale | **DEV** per tutta la passata |
| Build / `rt-suite` / PIE / packaged | **NOT RUN** — nessuno, e nessuna affermazione qui sotto ne dipende |
| Unreal Editor | **0 processi** all'inizio e alla fine |
| Ponte MCP | probe HTTP su `127.0.0.1:8765`, `:8767`, `:8000` → **000** su tutte e tre |

⚠️ **Perché non è stato usato uno dei tre checkout esistenti.** Misurati tutti e tre prima di scegliere:
`refactor-tactics-technical-designer\refactor-tactics-main` è **fermo in mezzo a un rebase interattivo**
(`.git/rebase-merge`, `pick 0b101738`, 1/2); `D:\Repositories\refactor-tactics-main` è su `main` con **due
binari sporchi** di un altro lavoro (`L_DevSandbox.umap`, `DA_HexMap_Scratch_Basin.uasset`);
`refactor-tactict-dev` è su un branch di feature altrui. Nessuno dei tre poteva ricevere un commit senza
toccare lavoro non mio. Il worktree sparse costa pochi secondi, non scarica `Content/` e **non muove il
`HEAD` di nessuno**. La regola §8.5 dell'handoff — *«3 terminali = 1 checkout»* — regge: questa passata è
**un solo terminale in DEV**, e non ha aperto né Editor né suite.

---

## 1. Verdetto in tre righe

1. **La Lane A è quasi finita, e non è dove sta il residuo della v0.1.** Il view model, gli otto widget di
   partita, il catalogo icone, il proiettore degli eventi giocatore, il countdown del Ready e l'overlay
   dell'unità sono **in `main` e coperti headless**.
2. **Il residuo è a schermo.** Le voci che chiudono E11/E20 sono `PIE-V01-SCREENHUD`, `PIE-ICON-01`,
   `PIE-V01-GHOSTS`, `PIE-V01-POINTER`, `PIE-V01-READY`, `PIE-V01-LOG` — e nessun test le sostituisce.
3. **Restano tre residui di codice veri**, non verifiche: le parti **A** e **F** di `#1936`, i quattro test
   `Preview.*` dichiarati e non scritti di CP 11.5/11.6, e il debito *«`DrawX` stampa, non disegna»* di `#80`.

**Nuove issue proposte: 0.** Ogni riga di questo audit ha trovato un owner vivo.

---

## 2. Tabella di audit

`Issue | Before | Action | After | Parent | Why | Evidence`

| Issue | Before (handoff) | Action | After (misurato) | Parent | Why | Evidence |
|---|---|---|---|---|---|---|
| `#25` | E11 epic | `NO ACTION` | OPEN · 3/11 | — | epic contenitore | `gh issue view 25` |
| `#217` | E20 epic | `NO ACTION` | OPEN · 1/3 | — | epic contenitore | `gh issue view 217` |
| `#77` | CP 11.1 | `REGRESSION` (documentale) | OPEN · 0/10 · **ultimo commento scaduto** | #25 | il commento del 2026-08-14 dice *«`find Content -iname "WBP_*"` è vuoto; `Content/RT/` non ha una cartella `UI/`»*: oggi `Content/RT/UI` ha **85 file** e otto `WBP_RT_*` di partita | `git ls-tree -r origin/main -- Content/RT/UI` → 85 |
| `#78` | CP 11.2 | `ALREADY_DONE` | **CLOSED** 2026-08-25 | #25 | l'handoff la elenca fra i checkpoint da riconciliare | `gh issue view 78` |
| `#79` | CP 11.3 | `REUSE` | OPEN · codice chiuso, resta `PIE-V01-LOG` 🟡 | #25 | difetto misurato **in PIE** (seduta U14, 2026-09-04): *«ho provato e me l'hanno negato»* e *«non ho provato»* sono la stessa riga | commento #79 del 2026-09-04 |
| `#80` | CP 11.4 | `REUSE` | OPEN · 0/5 | #25 | **12** comandi `rt.Debug.*` registrati oggi, **8** nel file console e nel DoD; il debito è che tre `DrawX` stampano | `grep -roE 'TEXT\("rt\.Debug\.[^"]+"\)' Source` |
| `#172` | CP 11.5 | `DECISION_REQUIRED` | OPEN · 0/6 | #25 | contrasto **linea↔superficie** misurato (caso peggiore `dE 1.9`–`2.1`) con due vie possibili e **nessuna scelta** | commento #172 del 2026-08-29 |
| `#173` | CP 11.6 | `REUSE` | OPEN · 0/6 | #25 | dipende da #172 | `gh issue view 173` |
| `#613` | CP 11.7 | `REUSE` | OPEN · **codice chiuso oggi** (PR #2424, merge `3ecd7965`) | #25 | resta `PIE-V01-SCREENHUD` ⏳ e il giudizio 🟡 su zone/centro libero | commenti #613 del 2026-09-05, 09:15 e 09:36 |
| `#705` | CP 11.8 | `LINK_ONLY` | OPEN · **Blocked** | #25 | `AddHitBox` ha **0 occorrenze** in `Source/` anche oggi: la precedenza HUD→mondo non ha nulla da consumare | `grep -rn AddHitBox Source` → 0 |
| `#1614` | *non citata* | `REUSE` | OPEN · 0/12 | #705 | seconda issue di CP 11.8 (overlay dell'hover), che l'handoff non nomina | `gh issue list --search hover` |
| `#1936` | Player Event Log | `REUSE` | OPEN · parti **A** e **F** residue | #1937 | C/D/E in `main` (PR #2104); **F** è un `.uasset` dietro #613, e **A** non può precedere F | commento #1936 del 2026-09-02 |
| `#1937` | *non citata* | `LINK_ONLY` | OPEN · epic | — | **è l'epic owner del Player Event Log**, e l'handoff nomina solo #1936 | `gh issue view 1937` |
| `#219` | CP 20.2 | `NO ACTION` | OPEN · 4/8 | #217 | categorie della v0.1 | `gh issue view 219` |
| `#220` | CP 20.3 | `REUSE` | OPEN · **codice chiuso oggi** (PR #2415, merge `39118445`) | #217 | resta **una** voce: `PIE-ICON-01` | commento #220 del 2026-09-05 09:36 |
| `#637` | tassonomia icone | `DEFER` | OPEN · 1/7 · `question` | #217 | 17 categorie del manifest assenti dal codice: è una domanda di design, non un blocco della v0.1 | `gh issue view 637` |
| `#2193` | Ready countdown | `ALREADY_DONE` | **CLOSED** 2026-09-04 | — | `FRTMatchHeaderView::ReadyCountdownSecondsRemaining` e `SecondsUntilCommit` esistono | `RTHudViewModel.h` |
| `#2288` | UnitOverlay WidgetComponent | `ALREADY_DONE` | **CLOSED** 2026-09-04 | — | `URTUnitOverlayWidget` + `WBP_RT_UnitOverlay` in `main` | `Content/RT/UI/Match/WBP_RT_UnitOverlay.uasset` |
| `#2347` | icone status a schermo | `ALREADY_DONE` | **CLOSED** 2026-09-04 | — | `FRTStatusBadgeView::IconId` + `BuildStatusBadges` | `RTHudViewModel.h` |
| `#2378` | *non citata* | `LINK_ONLY` | **mergiata oggi** (`8223d48a`) | — | porta `PIE-ICON-02` e le sedute **U47**/**U48** | commento #2378 del 2026-09-05 |
| `#2390` | *non citata* | `LINK_ONLY` | OPEN · 0/3 | — | l'anteprima può restare accesa se il turno si chiude per timeout: tocca il flusso Ready | `gh issue view 2390` |
| `#2184` | *non citata* | `LINK_ONLY` | OPEN | E50 | `ARTHUD::DrawHUD` a 706 righe: owner della performance del Canvas §4.2 | `gh issue view 2184` |
| `#286` / `#289` | *non citate* | `LINK_ONLY` | OPEN | — | E21 «Presentazione e leggibilità» confina con E11 e non va assorbita | `gh issue list` |

### Sezioni richieste dall'handoff §15

- **REUSED** — `#79`, `#80`, `#172`, `#173`, `#613`, `#705`, `#1614`, `#1936`, `#220`.
- **UPDATED** — nessuna: questa passata non ha scritto su GitHub. Due aggiornamenti **raccomandati** in §5.
- **CREATED** — **nessuna issue**. Tre documenti: questo, la Lane A e la Lane B.
- **CLOSED** — nessuna chiusa qui. Quattro erano **già** chiuse e l'handoff le trattava come residui:
  `#78`, `#2193`, `#2288`, `#2347`.
- **DEFERRED** — `#637` (tassonomia icone), `PIE-STATE-01…10` (CP 34.x, `post-v0.1`).
- **NO ACTION** — `#25`, `#217`, `#219`.
- **REGRESSIONS FOUND** — una, **documentale**: l'ultimo commento di `#77` (2026-08-14) descrive un albero
  che non esiste più. Nessuna regressione di codice trovata.

---

## 3. Baseline asset — `Content/RT/UI` (su `origin/main` `8a530c6e`)

85 file totali: 1 catalogo, 66 icone, 11 widget di frontend, 8 widget di partita.

| Asset | Classificazione | Evidenza |
|---|---|---|
| `WBP_RT_TacticalHUD` | `EXISTS_PARTIAL` — contenitore in `main`, il feed eventi (parte **F** di #1936) non c'è | `Content/RT/UI/Match/`, commento #1936 |
| `WBP_RT_TurnHeader` | `EXISTS_COMPLETE` — ricablato oggi su `SecondsUntilCommit` | commento #613 delle 09:15 |
| `WBP_RT_TeamRoster` | `EXISTS_COMPLETE` | DoD #613 ✅ |
| `WBP_RT_SelectedUnitPanel` | `EXISTS_COMPLETE` | DoD #613 ✅ |
| `WBP_RT_ActionDock` | `EXISTS_COMPLETE` | DoD #613 ✅ |
| `WBP_RT_ActionSlot` | `EXISTS_COMPLETE` — tre stati | DoD #613 ✅ |
| `WBP_RT_UnitCard` | `EXISTS_COMPLETE` | `Content/RT/UI/Match/` |
| `WBP_RT_UnitOverlay` | `EXISTS_COMPLETE` — aggiunto 2026-09-04 (#2288) | `git log --diff-filter=A` |
| `WBP_RT_EventLog` | **`MISSING`** | nessun file corrisponde in tutto l'albero |
| `DA_IconCatalog` | `EXISTS_COMPLETE` — **61 chiavi / 62 texture**, 61 su 61 risolvono | `roadmap-v0.1.md` §E20 · commento #220 |
| `Icons/RT_UI_Icon_*` | 66 texture; le cinque categorie della v0.1 sono popolate | `git ls-tree -r origin/main -- Content/RT/UI/Icons` |

⚠️ **Le 61 chiavi non sono state rilette dall'asset in questa passata**: `strings` su `DA_IconCatalog.uasset`
restituisce **0** occorrenze (payload compresso). Il numero viene dagli owner, non da una misura mia — ed è
esattamente la ragione per cui `HUD-EDITOR-E1` esiste.

---

## 4. Baseline test (discovery statica, **nessuno eseguito**)

| File | Test dichiarati | Namespace |
|---|---:|---|
| `RTHudViewModelTests.cpp` | 19 | `HudViewModel` |
| `RTScreenHudWidgetTests.cpp` + `RTHudScenarioTests.cpp` | 6 + 4 → **14 nomi `ScreenHud.*` distinti** | `ScreenHud` |
| `RTIconCatalogTests.cpp` | 6 | `IconCatalog` |
| `RTPlayerEventProjectorTests.cpp` | 11 | `UI.PlayerEventLog` |
| `RTPointerInteractionTests.cpp` | 14 | `Pointer`, `PlayerInput` |
| `RTPlayerInteractionTests.cpp` | 20 | `PlayerInput`, `PlayerInteraction` |
| `RTIntentPrivacyTests.cpp` | 11 | `Reactions`, `UI` |
| `RTHudAbilityBar` · `IntentPresentation` · `Marks` · `MatchStatus` · `PlaybackSpeed` · `SlotLines` | 3+2+3+5+3+3 | `HUD`, `UI` |
| `RTFrontendMatchHudTests.cpp` | 4 | `Frontend` |
| `Preview.*` (sparsi) | 9 nomi | `Preview` |

**Quattro test dichiarati nel DoD di CP 11.5/11.6 non esistono** — `grep -rl` su `Source/` dà 0 file per
ciascuno: `Preview.GhostMatchesResolverPath`, `Preview.ReactionIsNotAPhaseEntry`,
`Preview.WarningsComeFromResolverReasons`, `Preview.ArmedReactionRendersAsBranch`.
**Otto** test dichiarati da CP 11.8 restano non scritti (elenco in `roadmap-v0.1.md` §E11).

---

## 5. Registro PIE ed `editor-sessions.yaml`

`test-manuali-pie.md`: **375** occorrenze `PIE-`; subset `RELEASE-V01` = **17** voci (gate G9).

| Voce | Stato | Seduta dichiarata |
|---|---|---|
| `PIE-V01-HUD` `RELEASE-V01` | ✅ | U15 |
| `PIE-V01-INTENT` `RELEASE-V01` | ✅ | U15 |
| `PIE-PREVIEW-AREA` `RELEASE-V01` | ✅ | — |
| `PIE-HUD-CARD-ZERO` | ✅ | — |
| `PIE-V01-ROSTER` `RELEASE-V01` | 🟡 | — |
| `PIE-V01-LOG` `RELEASE-V01` | 🟡 | U15 · U43 · U46 |
| `PIE-V01-DEBUG` | 🟡 | U15 · U46 |
| `PIE-V01-PLAYSPEED` | 🔴 | — |
| `PIE-V01-SCREENHUD` | ⏳ | **nessuna** |
| `PIE-V01-GHOSTS` | ⏳ | **nessuna** |
| `PIE-ACC-HUD` | ⏳ | **nessuna** |
| `PIE-V01-POINTER` | ⏳ | U43 |
| `PIE-V01-READY` | ⏳ | U19 |
| `PIE-PREVIEW-PERSIST` | ⏳ | U3 |
| `PIE-ICON-01` | ⛔ | U9 · U43 |
| `PIE-ICON-02` | ⛔ | U48 |

🔑 **Gap misurato, e non è nel codice**: tre delle voci che chiudono E11 — `PIE-V01-SCREENHUD`,
`PIE-V01-GHOSTS`, `PIE-ACC-HUD` — **non sono nominate da nessuna seduta** di `editor-sessions.yaml`
(48 sedute, `U1`…`U48`). Senza seduta non hanno preparazione dichiarata, e la Lane B non può raggrupparle
in un'apertura sola.

### Due aggiornamenti raccomandati, non eseguiti qui

1. **`editor-sessions.yaml`** — aggiungere le sedute mancanti per le tre voci. **Non fatto** perché `U-nnn`
   è un contatore condiviso e `AGENTS.md` §12 vieta di assegnarlo dalla memoria: lo faccia il run che quelle
   sedute le **esegue**, dopo `git fetch --prune`.
2. **`#77`** — un commento che dichiari scaduto quello del 2026-08-14. **Non fatto**: questa passata non ha
   eseguito niente che cambiasse lo stato di `#77`, e scrivere su GitHub senza una misura nuova aggiunge
   rumore. La misura è in §2, pronta da citare.

---

## 6. Contraddizione fra due referti, e come si risolve

[`hud-planning-prediction-dual-roadmap-spec-panel-2026-09-05.md`](hud-planning-prediction-dual-roadmap-spec-panel-2026-09-05.md)
— scritto **oggi**, su un brief molto simile a quello consumato qui — conclude `F-02` *«il ponte MCP è giù»*
e `F-03` *«mancano sei `WBP_RT_*`, e nessun agente li può fare»*, e da lì dichiara `BLOCKED` tutta la sua
Roadmap B.

Misurato oggi, **entrambe le premesse vanno lette diversamente**:

- i sette `WBP_RT_*` di partita sono in `main` **dal 2026-08-26/27** (`git log --diff-filter=A`), e l'ottavo
  (`WBP_RT_UnitOverlay`) dal 2026-09-04. Quel referto misura su `b063a60f`, un branch che dichiara di essere
  *«due giorni dietro `origin/main`»*, e la citazione viene dal **piano** `screen-hud-umg-2026-08-26.md`, che
  quello stato lo descriveva il giorno in cui è stato scritto;
- il ponte MCP **non è una capability rotta: è una conseguenza.** Vive dentro l'Editor, e alle 07:58 — come
  alle 13:58 di questa misura — non c'era **nessun processo Unreal**. Alle **09:15 dello stesso giorno**
  un'altra sessione lo ha usato per riscrivere l'Event Graph di `WBP_RT_TurnHeader`
  (`break_pins` → `connect_pins` → `compile_blueprint` → `save_assets`, commento #613).

∴ **«MCP giù» è una misura di quel minuto, non un verdetto sulla Lane B.** La riformulazione corretta è
quella che `#613` e `#220` hanno scritto oggi *dopo* averlo usato: il ponte **scrive** asset, e ciò che
**non** sa fare è guardare la finestra di gioco in play mode (`SlateInspector.Snapshot` risponde vuoto,
`CaptureEditorImage` fallisce) — cioè esattamente il giudizio che resta di una persona.

⚠️ La stessa trappola vale al contrario, ed è scritta nel commento di #613: **il `grep` sul `.uasset` non
misura il cablaggio.** `PlanningSecondsRemaining` compare ancora 3 volte nel binario salvato *dopo* essere
stato scollegato, perché il nodo `Break` espone un pin per ogni campo. L'oracolo è rileggere il grafo dopo
la compilazione.

---

## 7. NOT RUN — dichiarazione esplicita

- `./scripts/rt-suite.ps1`, in qualunque filtro — **non eseguita**
- build `RefactorTacticsEditor` — **non eseguita**
- Automation Test mirati — **non eseguiti**
- PIE, seduta Editor, operazioni MCP — **non eseguiti** (resta solo il probe di rete)
- packaged / standalone — **non eseguito**
- lettura di `DA_IconCatalog` dall'asset — **non eseguita** (payload compresso; è `HUD-EDITOR-E1`)
- scritture su GitHub (issue, commenti, label) — **nessuna**

Nessuna affermazione di questo audit dipende da una di queste.

---

## 8. Prossimo passo

**Uno solo**: una seduta Editor che apra `PIE-V01-SCREENHUD`. È la voce che `#613` dichiara come unico
residuo, sblocca la parte **F** di `#1936` e la precedenza HUD→mondo di `#705`, e non ha un sostituto
headless. La sequenza è in [`hud-v01-editor-verification-roadmap.md`](hud-v01-editor-verification-roadmap.md),
da `HUD-EDITOR-E0` a `E3`.
