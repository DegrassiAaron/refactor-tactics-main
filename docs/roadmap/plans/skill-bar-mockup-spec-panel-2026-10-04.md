# Il mockup della skill bar (2026-10-04) — spec panel sul pacchetto, e che cosa ne è entrato

> `CURRENT` · **Stato**: triage chiuso, lavoro eseguibile consegnato in [#3465](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3465).
> Restano le voci di Decision Log **D-454 e D-455, proposte e non committate** (§5), e la seduta Editor `U61`.
> **Data**: 2026-10-04 · **Misurato su** `origin/main` = `77d253f70`, nel clone `refactor-tactics-dev`:
> il clone principale era occupato da un'altra sessione, che alle 13:44 aveva cambiato branch e stava
> modificando `editor-sessions.yaml`.
> **Oggetto**: il pacchetto `skill-bar-pacchetto`, consegnato dall'autore in `docs/research/` con un
> `PROMPT.md`. La sorgente di design entra in
> [`../../research/design/hud/skill-bar-2026-10/`](../../research/design/hud/skill-bar-2026-10/); il
> `PROMPT.md` è un work order e **non si versiona** ([`AGENTS.md`](../../../AGENTS.md) §8). Questo referto
> è l'unico posto in cui resta citabile.
> **Panel**: Wiegers (lead) · Adzic · Fowler · Crispin · Nygard · **Modo**: critique · **Focus**:
> requirements, architecture, testing.

---

## 1. Il verdetto in una riga

**Il mockup è una buona proposta di resa; il prompt che lo accompagnava era una fotografia del 2026-10-03
scritta su documenti più vecchi di lei.** Delle quattro lavorazioni che prescriveva, una era già fatta
(gli slot occupati) e una contraddiceva una decisione accettata (il selettore di profilo). È entrata **la
fase nella vista**, con una seduta Editor che la rende, e l'autore ha risposto in sessione a quattro
domande.

## 2. Audit F1–F12

Rimisurati su `77d253f70` con i comandi del prompt. ⛔ Il prompt dice *«se una misura diverge, fermati e
riporta»*: le divergenze sono state riportate all'autore **prima** di scrivere codice, e il piano è stato
adattato con le sue risposte, non in silenzio.

| # | Atteso | Misurato | |
|---|---|---|---|
| F1 | Generiche `G B C X Z` | `ARTPlayerController::GenericHotkeys()`: `Guard G · Brace B · Overwatch C · Interact X · Wait Z` | ✅ |
| F2 | Kit su `1…9, 0` | `AbilityHotkeys()`: `One … Nine, Zero` | ✅ |
| F3 | Undici campi, nessuna fase né corsia | i campi attesi ci sono **più `ChargeFraction`**, che il prompt non elenca; nessuna fase, nessuna corsia | ⚠️ |
| F4 | Niente `SignatureDefense` | `ERTActionSlot` = `None · Movement · Main · MovementAndMain · Reaction` | ✅ |
| F5 | `ReservesMovementProfileId` esiste | `RTActionDef.h`, campo di `FRTActionDef` | ✅ |
| F6 | `MinStability` non esiste | nessun campo; il token compare solo in commenti di `RTMovementProfile*` | ✅ |
| F7 | Prep · Blast · Move · Cleanup come elencati | `Guard`/`Brace`/`Overwatch` → `Preparation`; `Interact`/`BasicAttack` → `Attack`; `Wait` → `NormalMovement`; `Electrify` → `Environment`, che `MapResolutionPhase` porta in `Cleanup` | ✅ |
| F8 | Kit di Aevik | `ArcPulse` · `LinearDischarge` · `ConductiveNode` · `Overload` · `ReactiveCapacitor` | ✅ |
| F9 | Palette D-233 e §32 | identica, in `progettazione-hud.md` §32 e nel Decision Log | ✅ |
| F10 | I test nominati, e `ActionSlot*` = 10 | i test nominati esistono; gli `ScreenHud.ActionSlot*` sono **nove** (nominati in §8). Altri test della dock vivono in `Editor.*` e `PlayerInput.*` | ⚠️ |
| F11 | #1410 e #653 aperte | **#1410 chiusa il 2026-09-18, #653 chiusa il 2026-09-12** | ❌ |
| F12 | Ultima seduta | `U60` | ✅ |

**Due fatti trovati misurando, che il prompt non chiedeva:**

- il commento su `FRTAbilityCooldownView::HotkeyLabel` diceva *«non porta il tasto generico, decisione
  aperta in #2990»*, ma `HotkeyLabelFor` restituisce le lettere da [D-397](../../decisions/RT_PDR_00_Decision_Log.md)
  punto 4. Corretto in #3465;
- `ERTActionSlotState` produce già `Empty` · `Selected` · `Planned` · `Cooldown` · `Unavailable` · `Available`
  via `URTHudViewModel::ResolveSlotState` (#2988), e il suo
  commento dichiara che **Invalid e Warning riguardano il bersaglio** (`ERTTargetRefusal`), non l'azione.

## 3. La classificazione del §2 del prompt, corretta

| Elemento del mockup | Classe del prompt | Classe misurata | Perché |
|---|---|---|---|
| Gruppi Comuni · Base · Kit | `PROPOSTA` | `PROPOSTA` → **decisa** (§5.2) | La regola è decisa; il campo non esiste ancora. ⚠️ D-397 punto 2 aveva **differito il raggruppamento per scope**: la decisione lo riapre |
| Tasto su ogni slot | `CURRENT` | `CURRENT` | `HotkeyLabel`, lettere comprese (D-397 punto 4) |
| Striscia ed etichetta di fase | `DESIGNED` | `CURRENT` nella vista con #3465; resa in `U61` | §4.2 |
| Stati Available … Warning | `PARTIAL` | gli stati di `ERTActionSlotState` sono `CURRENT` nel C++; Hover è del widget; **Invalid/Warning in `CONTRACT CONFLICT`** fra `progettazione-hud.md` §7 e il commento di `ERTActionSlotState` | §5.4 |
| Slot occupati | `DESIGNED`, *«unica voce scoperta del piano UI-0»* | **`CURRENT`** — `FRTUnitSlotsView`, `URTSelectedUnitPanelWidget::GetSlots()`, coperti da `ScreenHud.SlotsAreReadNotDeduced`, `ScreenHud.MovementSlotSaysNothingWhenUnauthorized` e `ScreenHud.InspectedEnemyNeverCarriesItsPlannedSlots` | `STALE ROADMAP`: la frase è del piano UI-0 del 2026-08-12, scritta prima che la vista esistesse |
| Selettore di profilo `Withdraw · Sneak · Move · Sprint` | `FUTURE`, bloccato da #1410 e #653 | **`CONTRACT CONFLICT` con [D-425](../../decisions/RT_PDR_00_Decision_Log.md)** | Il profilo non si sceglie fra quattro etichette: `Withdraw` lo impone la riserva, `Sneak` si dichiara col tasto `M`, `Move` e `Sprint` sono la banda derivata dalla distanza. Resta `FUTURE` solo il blocco per `Stability` (#606) |
| Slot Difesa caratteristica | `FUTURE` | `FUTURE` | #3130 |
| `Brace` nelle Comuni | `CONFLITTO` | decisione d'autore del 2026-10-03, voce **proposta** in §5.1 | — |

⚠️ **E un documento owner è indietro**: `progettazione-hud.md` §6.7 cita ancora #1410 e #653 come aperte
e prescrive *«nessun pulsante di movimento nella dock prima di quelle due»*. Con D-425 la frase va
riscritta, non solo datata — è fra i seguiti (§9).

## 4. Il panel

### 4.1 WIEGERS — la premessa del requisito più costoso era falsa

❌ **CRITICAL** — P3 chiedeva di creare `FRTPlanSlotsView`. La view esisteva, con il caso privacy già
pinnato. Il requisito citava come prova un piano del 2026-08-12, quindi misurava l'intenzione di allora e
non il codice di oggi. 📝 Un requisito che dice *«manca X»* va scritto col comando che lo dimostra, ed è
il formato che `.github/ISSUE_TEMPLATE/task.md` chiede già: *«se una premessa è negativa, cercala in tutto
Source/»*. Eseguito alla lettera, P3 avrebbe prodotto una seconda architettura.

### 4.2 FOWLER — tre campi per tre domande, e nessun flag duplicato

⚠️ **MAJOR** — il prompt chiedeva `Phase` più `bIsReaction`. Ma `Slot == Reaction` è già nella vista, e
un secondo booleano sarebbe una seconda verità. Soprattutto la fase **onesta** e il **segno** che lo slot
mostra divergono su due voci: una reazione eredita la fase della sua core, e `Wait` risolve in
`NormalMovement`. Decisione d'autore in sessione: `Phase` (quando risolve) + `PhaseMark` (che cosa si
vede, chiave del colore) + `PhaseLabel` (composta in C++). Il colore resta nel Blueprint, perché il C++
non ha una copia della palette e non deve averne una seconda.

### 4.3 ADZIC — gli esempi sono la specifica

| Azione | `Phase` | `PhaseMark` | `PhaseLabel` |
|---|---|---|---|
| `Action.Guard` | `Prep` | `Prep` | `PREP` |
| `Hero.Aevik.ArcPulse` | `Blast` | `Blast` | `BLAST` |
| `Hero.Aevik.ConductiveNode` | `Cleanup` | `Cleanup` | `CLEANUP` |
| `Hero.Aevik.ReactiveCapacitor` | quella della core | `Reaction` | `REAZ.` |
| `Action.Wait` | `Move` | `None` | `—` |
| posizione di kit vuota | `Planning` (default) | `None` | *(vuota)* |

È la tabella che `HudViewModel.ActionSlotCarriesItsPhase` asserisce caso per caso, scritta a mano e non
chiesta a `PhaseMarkFor`.

### 4.4 CRISPIN — il kit reale non distingue «letto» da «dedotto»

⚠️ **MAJOR** — sul kit di Aevik ogni azione sta sempre nella stessa posizione. Una vista che deducesse la
fase dall'indice sarebbe quindi verde su qualunque test che guardi solo quel kit. `ActionSlotPhaseIsReadNotDeduced`
muove il dato in tre modi: cambia la fase nel dato, scambia due azioni di fase diversa e cambia lo slot.
Lo fa su **copie** delle azioni, perché `ConfigureFromHeroData` condivide gli oggetti del roster con
ogni altra unità del processo. 📝 F10 contava dieci `ActionSlot*`, e sono nove. Il criterio di regressione
resta lo stesso: tutta la suite.

### 4.5 NYGARD — le risorse condivise

⚠️ **MINOR** — il prompt prescriveva *«sessione singola, niente worktree»* ([D-178](../../decisions/RT_PDR_00_Decision_Log.md)).
Il clone principale però era occupato da un'altra sessione. Vale [`AGENTS.md`](../../../AGENTS.md) §11:
*una sessione, un clone*. Il lavoro è passato al clone `refactor-tactics-dev`, fermo dal 2026-09-25 e
pulito. `editor-sessions.yaml` era in modifica anche nell'altra sessione: `U61` è accodata in coda a
`sessions`, prima di `not_schedulable` — ⚠️ non in fondo al file, dove la prima stesura l'aveva messa e dove
`yaml.safe_load` la leggeva come una voce di `not_schedulable`. Il `U<n>` va rimisurato prima del merge.

## 5. Le decisioni

Prese dall'autore in sessione il 2026-10-04, dopo il referto delle divergenze.

### 5.1 DEC-SB-1 — `Brace` nella barra · **voce proposta, non committata**

La decisione d'autore è del 2026-10-03; manca la voce. Il numero è il primo libero su `main` alla misura.
⛔ Va **rimisurato subito prima del merge**, sui tre posti che il registro prescrive.

```markdown
| **D-454** | **`ACTION.BRACE` RESTA UN COMANDO DELLA BARRA: I COMANDI SONO DODICI, NON UNDICI.** Corregge [D-407](RT_PDR_00_Decision_Log.md) punto (1). **(1) Il fatto.** D-407 elenca undici elementi logici — attacco base, Guardia, difesa caratteristica, Overwatch, Move, Interact, Wait e quattro skill — e `Brace` non c'è, mentre [D-025](RT_PDR_00_Decision_Log.md) lo conta fra le sette generiche e `ARTPlayerController::GenericHotkeys()` gli dà il tasto `B`. L'omissione non era una rinuncia: il punto (3) di D-407 nomina `Action.Brace` proprio per dire che *resta ciò che è*. **(2) Deciso.** `Action.Brace` è il dodicesimo comando, fra le generiche accanto alla Guardia. **(3) Che cosa lascia fare al movimento.** Applica `Status.Root` per un turno insieme a `Status.Braced` (`URTCatalogLibrary`, `Action.Brace`): nessuno spostamento volontario, `Withdraw` compreso. Non lo esprime né la soglia né la riserva di [`spec-barra-comandi.md`](../gameplay/spec-barra-comandi.md) §3.1, ma lo **stato** che l'azione applica. È coerente con §1.1 — *«chi si irrigidisce non ripiega»* — e la tabella di §3 ne riceve la riga. **(4) Che cosa NON cambia**: lo slot della difesa caratteristica resta `SignatureDefense` e non si chiama `Brace` (D-407 punto 3); nessuna riga di codice cambia, perché `Action.Brace` è già nel kit di ogni eroe col tasto `B` | **Accettata** — decisione d'autore in sessione *(2026-10-03)*, registrata dal triage del mockup della skill bar | Owner: `docs/gameplay/spec-barra-comandi.md` §1 e §3. ⏳ Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A` — la voce decide e non tocca `Source/` |
```

Le correzioni che la stessa voce porta:

- **D-407**, colonna note: *«⏱️ Il punto (1) è corretto da D-454: i comandi sono dodici, con `Brace`.»*
  Non si riscrive il testo accettato.
- **`spec-barra-comandi.md`**: il titolo passa da *«undici voci»* a *«dodici voci»*. §1 riceve la riga
  `Irrigidimento (Brace) | principale | Difesa comune: Braced −10 da ogni lato, più Root`, e la frase
  *«Sono undici elementi logici»* diventa *dodici*. §3 riceve la riga
  `Irrigidimento (Brace) | nessuno | lo stato Root che l'azione applica`.

### 5.2 DEC-SB-2 — dove vive il gruppo · **regola derivata, voce proposta e non committata**

| | Scelta 1 — campo dichiarato | Scelta 2 — regola derivata ✅ |
|---|---|---|
| **FATTO** | `FRTActionDef` non ha un campo di gruppo | i due dati che servono esistono: `URTCatalogLibrary::GetGenericActionIds()` restituisce esattamente le cinque Comuni del mockup, e `FRTActionDef::BaseActionId == Action.BasicAttack` identifica l'attacco base come **dato** ([D-033](../../decisions/RT_PDR_00_Decision_Log.md)) |
| **INTENTO** | dichiarare il gruppo su ogni azione | derivare `Group` nella vista, in `URTHudViewModel` |
| **PROBLEMI · AMBIGUITÀ** | un enum su ogni azione del catalogo, ed è un dato di presentazione dentro la definizione di gioco | ⛔ non `AbilityIndex == 0`, come proponeva il prompt: sarebbe la deduzione dalla posizione che D-397 punto 2 vieta |
| **RISCHI** | impatto su serializzazione e hash da misurare | la futura `SignatureDefense` entra da `Slot`, non da un caso speciale (#3130) |

```markdown
| **D-455** | **IL GRUPPO DI UNA VOCE DELLA DOCK SI DERIVA DA DATI CHE ESISTONO, NON SI DICHIARA.** Scioglie il differimento di [D-397](RT_PDR_00_Decision_Log.md) punto (2) per il layout della skill bar del 2026-10-04. **(1) Deciso.** `FRTAbilityCooldownView` porta un gruppo derivato in `URTHudViewModel`: **Comuni** se l'`ActionId` è in `URTCatalogLibrary::GetGenericActionIds()`, **Base** se `BaseActionId == Action.BasicAttack` ([D-033](RT_PDR_00_Decision_Log.md)), **Kit** altrimenti. **(2) Il raggruppamento LEGGE il campo e non riordina la lista**: l'ordine di `GetActions()` resta identità (D-397 punto 2). **(3) Scartato**: un campo di gruppo su `FRTActionDef`, perché metterebbe un dato di presentazione nella definizione di gioco; e `AbilityIndex == 0` come criterio della Base, perché deduce dalla posizione | **Accettata** — decisione d'autore in sessione *(2026-10-04)* | Owner del layout: [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613). ⚠️ Zero codice in questa voce: il campo è un seguito |
```

### 5.3 DEC-SB-3 — il colore di `Cleanup` · **confermato il canone, nessuna voce**

[D-232](../../decisions/RT_PDR_00_Decision_Log.md) §1 e [D-233](../../decisions/RT_PDR_00_Decision_Log.md)
lasciano `Cleanup` senza tinta finché una reazione non avrà una card. L'autore lo ha confermato: sola
etichetta `CLEANUP` più tratteggio neutro. ⚠️ Il neutro del mockup, `#A9B4C2`, in §32 non esiste: `U61` lo
segnala e indica `RT_UI_Frame_Mid` come candidato già in palette.

### 5.4 Invalid e Warning sullo slot · **decisione di scope per `U61`, conflitto aperto**

`progettazione-hud.md` §7 elenca Invalid e Warning fra gli stati dello slot. Il commento di
`ERTActionSlotState` li assegna invece al bersaglio. Decisione d'autore per `U61`: **la ricetta visiva si
costruisce, il produttore arriva dopo**, quando lo slot dell'azione armata potrà leggere il rifiuto del
bersaglio. Il conflitto fra le due fonti resta aperto ed è un seguito (§9).

### 5.5 La forma di P2

Vedi §4.2: `Phase` + `PhaseMark` + `PhaseLabel`, con `Slot` come unica fonte del caso reazione.

## 6. Che cosa è stato fatto

| Passo | Esito |
|---|---|
| **0-bis** | Pacchetto copiato senza `PROMPT.md` in `docs/research/design/hud/skill-bar-2026-10/`, con hash identici alla consegna. Intestazione di statuto nel formato di `01-principi.md`, riga in `CHANGELOG_DOCUMENTATION.md` |
| **P1** | Nessuna issue copriva la fase sullo slot: #2826 è la leggibilità, #2988 lo stato come dato, #613 il layout. Creata [#3465](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3465). Il titolo nomina la sola fase, perché gli slot occupati erano già `CURRENT` |
| **P2** | `ERTActionPhaseMark`; `FRTAbilityCooldownView::Phase`, `PhaseMark`, `PhaseLabel`; `URTHudViewModel::PhaseMarkFor` e `PhaseMarkLabel`. Test nuovi `HudViewModel.ActionSlotCarriesItsPhase` e `HudViewModel.ActionSlotPhaseIsReadNotDeduced` |
| **P3** | **Non fatto, perché già fatto** — §3 |
| **P4** | `U61` in `editor-sessions.yaml`: fase, stati con secondo canale, Invalid/Warning senza produttore, prova in scala di grigi |

⚠️ **I nomi dei test differiscono dal prompt**, che li chiedeva in `ScreenHud.*`. Misurano
`BuildAbilityCooldowns`, quindi stanno in `HudViewModel.*` accanto ai fratelli che misurano la stessa
funzione: `ShortcutComesFromTheBindingTableNotTheIndex` e `KitHoleDoesNotRenumberTheSlots`.

## 7. Che cosa il prompt chiedeva e non è stato fatto

| Richiesta | Perché no |
|---|---|
| P3, `FRTPlanSlotsView` | esisteva già come `FRTUnitSlotsView`: sarebbe stata una seconda architettura |
| `bIsReaction` | duplica `Slot`; decisione d'autore §5.5 |
| `WBP_RT_PlanSlots` in `U61` | i chip leggono `GetSlots()` e `WBP_RT_SelectedUnitPanel` li mostra già; spostarli è layout di #613 |
| Separatori di gruppo in `U61` | il campo `Group` non esiste: un Blueprint che deducesse il gruppo dalla posizione violerebbe D-397 |
| Voci di Decision Log committate | il prompt stesso le chiede *«testo pronto, non committato»* (§5) |

## 8. Verifica

Misurato nel clone `refactor-tactics-dev` con `tools/suite/esegui.py` (`-abslog` nello scratchpad di
sessione). L'invariante di `AGENTS.md` §9 è stato verificato a ogni misura: `HEAD`, `git diff HEAD`, hash
degli untracked e hash della DLL identici a inizio e fine.

| Gate | Esito | Su che cosa |
|---|---|---|
| Compile, Editor Development | `PASS` | base `77d253f70`: `Result: Succeeded`. Con #3465: `Result: Succeeded`, **0 warning**, e i due file toccati compilati come unità propria — quindi un warning si sarebbe visto |
| Suite completa, prima | `PASS` — 2823 trovati, 2823 `Success`, 0 `Fail` | `77d253f70` |
| Suite completa, dopo | `PASS` — 2825 trovati, 2825 `Success`, 0 `Fail` | `daebc3194`. La differenza fra i due insiemi di nomi è **esattamente** i due test nuovi |
| Suite completa, dopo la revisione | `PASS` — 2825 trovati, 2825 `Success`, 0 `Fail` | `4d18c5268`, col blocco D e le correzioni della revisione; build pulita, invariante verificato |
| Test nuovi | `PASS` | `HudViewModel.ActionSlotCarriesItsPhase` · `HudViewModel.ActionSlotPhaseIsReadNotDeduced` |
| Test della dock (F10) | `PASS` | dentro la suite: `ScreenHud.ActionDockShowsTheNeutralState`, `DockArmsOnlyTheSelectedAction`, `DockSurvivesAHoleInTheKit`, `SlotsAreReadNotDeduced`, i nove `ActionSlot*` (`CarriesAnIconKey`, `ForwardsItsOwnIndex`, `HasIconSurface`, `IsNotTransparentToThePointer`, `LineCarriesKeyArmedAndReason`, `LineIsTheSameComposerAsTheHud`, `LoadsTheIconItShows`, `ResolvesFromCatalog`, `ResolvesOncePerActionChange`), `MovementSlotSaysNothingWhenUnauthorized`, `InspectedEnemyNeverCarriesItsPlannedSlots`; più `Editor.ActionSlot*`, `Editor.DockArmedStateReadsTheAbilityIndexNotTheLoopPosition`, `PlayerInput.DockClickAndHotkeyReachTheSameAbility`, `PlayerInput.TheDockPortArmsAndDisarms` |
| Corpus golden | `PASS`, nessun bump | `Simulation.GoldenCorpusMatches` verde nella suite dopo: il digest del TurnLog non si è mosso |
| Mutazioni | **i test sanno fallire** | **M1**, senza il ramo `Slot == Reaction`: rossi entrambi — la reazione diceva `BLAST`, la Guardia resa reazione `PREP`. **M2**, segno memorizzato per indice: rosso `ActionSlotPhaseIsReadNotDeduced` su A, B e C, verde `ActionSlotCarriesItsPhase`. ⚠️ È il caso che il docstring del secondo test dichiara: sul solo kit di Aevik letto e dedotto non si distinguono. Sorgente ripristinato, binario ricostruito, `HudViewModel.*` di nuovo tutti verdi. **M3**, `Dash -> Move`: **sopravviveva** a M1 e M2 e a tutta la suite, lo ha trovato la revisione; col blocco D cade su *«D: FastMovement»* |
| Revisione indipendente | nessun bloccante | un agente revisore sul diff della PR, in sola lettura. Ha trovato una lacuna di copertura: i rami `Dash`, `Move` e `Snapshot` di `PhaseMarkFor` non avevano oracolo, perché il kit di Aevik non li raggiunge. Chiusa dal blocco D di `ActionSlotPhaseIsReadNotDeduced`, che percorre ogni `ERTResolutionPhase` e verifica di coprire l'enum intero |
| Radar | `PASS` | `doc-links.ts`, `doc-coherence.ts` (insieme sono `G14`), `doc-tables.ts`, `decision-ids.ts`, `generate.ts` |
| Determinism · Replay | `N/A` | sola vista: nessuno stato canonico, nessun campo serializzato |
| Privacy | `N/A` | la dock legge il kit dell'unità comandata, e la fase è un dato del catalogo: nessun dato del piano altrui entra nella vista |
| PIE · Packaged | `NOT RUN` | la resa è `U61` |

⚠️ **Due fatti di contesto, dichiarati invece che taciuti.**

- **Una build del clone principale**, di un'altra sessione, è partita alle 14:23:39, durante la suite
  prima (14:21–14:26). Stava in un altro clone e la suite misurava un esito, non un tempo: per
  `AGENTS.md` §9 non lo cambia, perché `Binaries/` è per clone e l'Engine è una installed build.
- **`main` si è mosso durante il lavoro**, a `ef1aa1895`, con un solo commit di documenti
  (`editor-sessions.yaml`, corpo di `U60`). Dopo il merge l'albero `Source/` del branch è **identico** a
  quello misurato (`git rev-parse HEAD:Source` → `1362216d`). Anche l'albero `Source/` di `main` è
  identico a quello della base misurata (`bcacb0f0`).

## 9. Seguiti

- **Il campo `Group`** di D-455, quando la voce sarà registrata, e con lui i separatori di gruppo in Editor.
- **Il produttore di Invalid/Warning sullo slot**, e la riconciliazione fra `progettazione-hud.md` §7 e
  il commento di `ERTActionSlotState` (§5.4).
- **`progettazione-hud.md` §6.7**: la nota del 2026-09-11 cita #1410 e #653 come aperte; con D-425 il
  movimento nella dock va riscritto, non solo datato.
- **Il piano UI-0** (`ui-0-first-playable-hud-2026-08-12.md`) dichiara ancora gli slot occupati come
  *«unica voce scoperta»*.
- **D-454 e D-455**: registrazione, con la rimisura del numero prima del merge.

## 10. Prompt di handoff

```text
Contesto: RefactorTactics, triage del mockup della skill bar del 2026-10-04.
Referto: docs/roadmap/plans/skill-bar-mockup-spec-panel-2026-10-04.md
Sorgente di design: docs/research/design/hud/skill-bar-2026-10/
Stato: #3465 su main (Phase, PhaseMark, PhaseLabel nella vista della dock); U61 registrata e non eseguita.

Prossimo passo, in quest'ordine:
1. Registrare D-454 (Brace, dodici comandi) e D-455 (gruppo derivato) col testo pronto del referto §5,
   dopo aver rimisurato il primo D-nnn libero su main, sui rami remoti e nelle PR aperte.
2. Una issue per il campo Group di D-455 (regola: GetGenericActionIds -> Comuni,
   BaseActionId == Action.BasicAttack -> Base, altrimenti Kit), con test sul kit reale e una controprova
   che sposti le azioni.
3. U61 nel clone principale, con l'Editor ricompilato da un main che contiene #3465: striscia ed
   etichetta da Action.PhaseMark/PhaseLabel, stati da ResolveSlotState, prova in scala di grigi
   committata in docs/technical/evidence/hud/.
Fuori scopo: selettore di profilo (contraddice D-425), SignatureDefense (#3130), riordino di GetActions().
```
