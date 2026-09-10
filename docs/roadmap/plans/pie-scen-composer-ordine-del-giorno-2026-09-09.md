# Ordine del giorno — `PIE-SCEN-COMPOSER`, la prima apertura

> `OPERATIVO` · **Data**: 2026-09-09 · **Per**: una singola seduta in Editor
> **Owner dell'esito atteso**: [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md),
> voce `PIE-SCEN-COMPOSER`. ⛔ Questo file **non** riscrive l'esito atteso — lo cita e lo mette in
> sequenza, che è la regola `R-6`.
> **Contesto**: [`match-lab-discovery-spec-panel-2026-09-09.md`](match-lab-discovery-spec-panel-2026-09-09.md) §8
> **Contratto del widget**: [`../../technical/tooling/contratto-wbp-scenario-composer.md`](../../technical/tooling/contratto-wbp-scenario-composer.md)

---

## 1. Perché questa seduta, e perché adesso

`test-manuali-pie.md` dice della voce `PIE-SCEN-COMPOSER`:

> ⏳ *«eseguibile dal 2026-09-03, e **mai eseguita**. 🔴 Ha smesso di essere non eseguibile senza che
> nessuno la riguardasse»*

E `terminali-code-editor-e-dir-c-spec-panel-2026-08-29.md` §12 la colloca:

| Slice | Stato |
|---|---|
| `TD-EDITOR-01` graybox unit + initial state UI | 🔴 **il collo di bottiglia** — widget assente, nessuna seduta |
| `TD-EDITOR-02` move + intent UI | 🔄 dietro `TD-EDITOR-01` |
| `TD-EDITOR-03` Run/Reset/TurnLog | 🔄 dietro `TD-EDITOR-01` |

⚠️ **Il «widget assente» di quella riga non vale più.** L'asset esiste:

```sh
git ls-files 'Content/RT/Editor/**/WBP_RT_ScenarioComposer*'
# → Content/RT/Editor/Scenario/WBP_RT_ScenarioComposer.uasset
```

Introdotto da `#1804` / `D-280`, ultimo commit `ad188823`. **Resta vero il resto**: nessuna seduta lo
rivendica — misurato il 2026-09-09, `grep -ic composer docs/roadmap/editor-sessions.yaml` → `0`, idem
per `td-editor`.

🔑 **Un'unica apertura risponde a tre domande** che oggi non hanno risposta: il widget fa quello che il
contratto dice? cosa manca per non uscire mai da una superficie 2D? e le due slice a valle sono
davvero bloccate, o lo erano solo sulla carta?

---

## 2. Prima di aprire — precondizioni di macchina

⛔ **`CLAUDE.md` §2**: *«Unreal è uno per macchina. Prima di aprire l'Editor, lanciare PIE, compilare o
misurare, accertati che nessun'altra sessione lo stia usando: due job insieme si invalidano le misure a
vicenda.»*

🔴 **Misurato il 2026-09-09**: un processo era già attivo.

```sh
tasklist | grep -i "unreal"
# → UnrealEditor-Cmd.exe   67276   752.164 K
# → UnrealTraceServer.exe  78376
```

`UnrealEditor-Cmd.exe` è un processo headless (commandlet o automation), non l'Editor grafico.
**Rimisurare e chiuderlo prima di aprire**, o le osservazioni di questa seduta valgono meno di quanto
costano.

### 2.1 Il bridge MCP — opzionale, e oggi non disponibile

`U44` è stata attraversata il 2026-09-04 *«pilotando il pannello via MCP»*. Il ponte esiste:

```json
// .mcp.json
{ "mcpServers": { "unreal-mcp": { "type": "http", "url": "http://127.0.0.1:8765/mcp" } } }
```

Plugin Epic `ModelContextProtocol` in `UE_5.8/Engine/Plugins/Experimental/`, da abilitare — vedi
[`../../technical/tooling/brief-mcp-developer-bridge.md`](../../technical/tooling/brief-mcp-developer-bridge.md).

🔴 **Stato al 2026-09-09**: `curl -s -m 5 http://127.0.0.1:8765/mcp` → nessuna risposta. Il bridge non è
in ascolto.

⚠️ **E una sessione Claude Code avviata prima del clone non lo ha fra i propri tool**: i server MCP di
progetto si caricano all'avvio. Per usarlo servono entrambe le cose — plugin abilitato con l'Editor
aperto, **e** una sessione riavviata nella cartella del progetto.

**La seduta si può fare senza.** Il bridge la renderebbe assistita, non possibile.

---

## 3. La sequenza

L'esito atteso di ogni passo è in `test-manuali-pie.md`, voce `PIE-SCEN-COMPOSER`. Qui c'è **l'ordine** e
**cosa guardare**.

### Passo 0 — apertura

Aprire `Content/RT/Editor/Scenario/WBP_RT_ScenarioComposer`.

- [ ] il widget si apre senza errori nel log;
- [ ] annotare **come** ci si arriva: da un menu, da un tab, o solo aprendo l'asset nel Content Browser.

🔑 Il *come* è metà della risposta alla domanda «è un tool 2D?». Un widget che si apre solo dal Content
Browser non è ancora uno strumento: è un asset.

### Passo 1 — il DoD di `TD-EDITOR-01`

> *«piazza almeno due unità, salva, riapri, verifica Cell / Facing / ID»*

- [ ] `NewScenario` — un draft nuovo;
- [ ] `[ + UNIT ]` due volte;
- [ ] le due unità compaiono nella lista con **Hero · Team · Facing · Cell**;
- [ ] `[ SAVE ]` scrive;
- [ ] riaprendo per ID i quattro campi coincidono.

⚠️ **Il punto che il DoD non prevedeva è l'`ID`.** Da `#1115` `AddUnit` prende `UnitId` **in ingresso**
invece di restituirlo: l'id lo conia il widget. Due unità aggiunte di seguito devono avere id
**distinti** che sopravvivono al round-trip.

> 🔑 *«Un conio che collide si manifesta come `Invalid` con "id già preso", cioè un messaggio che accusa
> lo **scenario** per un difetto della **schermata**: chi lo legge cerca nel posto sbagliato.»*

### Passo 2 — l'oracolo che distingue una lente da un secondo gioco

- [ ] posizionare il ghost su una cella **occupata**: dice *invalido* **prima** del click?

⛔ **Se l'invalidità compare solo dopo il click, il widget sta deducendo invece di chiamare — ed è quello
il difetto, non la geometria.** `AddUnit` fallisce subito e non lascia cadere `Validate`.

È il criterio di `spec-tactical-designer.md` §3 reso osservabile in un gesto solo. Vale più di ogni
altro passo di questa seduta.

### Passo 3 — le due domande che riguardano il tuo obiettivo

Non sono nel DoD: sono la ragione per cui questa seduta viene aperta adesso.

- [ ] **si esce mai dal 2D?** Per fare il giro completo — mappa, unità, pianificazione — quante volte
      bisogna passare dal viewport 3D o da un altro strumento? Annotare ogni passaggio, non il
      giudizio complessivo;
- [ ] **la mappa da dove viene?** Il Composer piazza unità su una mappa che qualcun altro ha disegnato
      (`URTHexEditorMode`, sette tool, viewport 3D). Verificare se dal widget si sceglie la mappa, e
      cosa si vede di essa.

### Passo 4 — i «cinque rilievi» di `#1527`

🔴 **Non sono scritti da nessuna parte.** `#1527` ha il body letterale `@-` — l'argomento di
`--body-file` non risolto, cioè il contenuto perso in fase di creazione. Nessun commento.

E la sua origine dichiarata, `#1524`, **è una pull request, non una issue** (`is_pr=true`, closed), con
lo stesso body perduto. I cinque rilievi non sono recuperabili da nessuna delle due.

- [ ] durante la seduta, annotare ciò che si osserva come **rilievi nuovi**, senza tentare di indovinare
      quali fossero i cinque originali;
- [ ] a valle, decidere se `#1527` va **ripopolata** o chiusa come non ricostruibile.

### 4.1 L'episodio, misurato

Prima di trattarlo come un caso singolo è stato misurato il **2026-09-09**, e non lo è:

```sh
gh issue list -R … --state all --limit 3000 --json number,body \
  --jq '[.[] | select(.body == "@-") | .number]'          # → [1527]
gh pr    list -R … --state all --limit 2000 --json number,body \
  --jq '[.[] | select(.body == "@-") | .number]'          # → [1524, 1526, 1531, 1532, 1534]
```

**Una issue e cinque PR**, tutte nella finestra `1524`–`1534`: un singolo episodio, non un difetto
ricorrente. ⚠️ La prima misura era stata fatta con `--limit 1000` e dava `1`: tagliava senza dirlo, e
`#1524` — che era già nota — non compariva. Il limite va tenuto sopra il numero di elementi, o la
misura mente per omissione.

⛔ Le PR sono chiuse: non c'è niente da riparare lì. L'unica ancora viva è `#1527`.

---

## 4. Cosa NON ricontrollare

Già coperto headless, e ripeterlo a schermo costa senza aggiungere evidenza:

| Cosa | Chi lo copre |
|---|---|
| il round-trip del formato | `RTScenarioWriterTests` · `RTScenarioAuthoringTests` |
| che la facade sia raggiungibile da Blueprint nei due versi | `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint` |
| che l'esecuzione dall'Editor coincida con l'headless | `RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun` |
| se una cella sia occupata | il runtime — il widget non ha voce in capitolo |

> Questa voce **non ripete il round-trip**: verifica che *la schermata* faccia arrivare i dati giusti al
> writer.

---

## 4bis. ESITO — seduta eseguita il 2026-09-09, pilotata via MCP

> Eseguita sul clone **principale** (`D:\Repositories\refactor-tactics-main`, `main`), Editor avviato e
> chiuso dalla sessione. Nessun asset salvato, nessun commit, nessuna issue toccata.

### Il pannello non è il `WBP_RT_ScenarioComposer`

La finestra che si apre sul livello di bootstrap è il **DevSandbox Launcher** (`#1678`) —
`SRTLauncherScenarioPanel`, Slate, quindi già 2D. Il widget UMG del Composer non entra in questo giro.

### Cosa fa, misurato

| Gesto | Esito osservato |
|---|---|
| apertura | il tab si presenta da solo all'apertura di `L_DevSandbox` (`EditorStartupMap`) |
| filtro testuale | `Type` su «cerca fra gli scenari filtrati» → la lista si riduce, **la selezione funziona solo sugli item renderizzati** (lista virtualizzata) |
| selezione scenario | readout completo: `terreno · squadre · unità · turni · attese · tag · Viewport` |
| `Start Session` | ✅ `"Sessione aperta su 'Movement.Basic'."` — abilita `Esegui`, `Riproduci`, `>`, `>\|`, `Reset` |
| `Esegui` | ✅ **funziona**: `"Nessun playback: esegui uno scenario."` → `"Posa iniziale."` |
| `Map` · `Scenario` · `Validation` | ✅ tutte e tre attivano: nessuna produce `"La superficie … non si è aperta."` |

⚠️ **`Esegui` è indistinguibile da un no-op su uno scenario `freerun`.** Con
`AutoBattle.ArenaV01` (`turni 0`) il playback risulta **già aperto** dopo `Start Session`, quindi il
click non cambia una sola parola sullo schermo. Il difetto non è nel pulsante: è che lo stato «playback
aperto sulla posa» non distingue *«non ho eseguito»* da *«ho eseguito uno scenario senza turni»*.
Serve uno scenario con turni authorati per vedere la transizione. **È un rilievo nuovo, e sostituisce i
cinque perduti di `#1527`.**

### La risposta alla domanda del §3

**Si esce dal 2D una volta sola, per la mappa, ed è deliberato.** Da `RTLauncherWorkspace::Surfaces()`:

```cpp
{ "Map",        declared,     ActivationKind::EditorMode, EM_RTHexEditorModeId }   // → viewport 3D
{ "Scenario",   declared,     ActivationKind::Tab,        LauncherTabId        }   // → stesso tab
{ "Validation", declared,     ActivationKind::Tab,        LauncherTabId        }   // → stesso tab
{ "Playback",   NOT declared, …, pending #1625 }
{ "TurnLog",    NOT declared, …, pending #1630 }
```

Il pannello **dichiara da sé** ciò che non ha, con la issue che lo porta:
*«Playback: non ancora costruita — la porta #1625. TurnLog: non ancora costruita — la porta #1630.»*

### 🔴 Il blocco che ha impedito due tentativi

I primi due avvii dell'Editor sono **crashati** a ~50s, in compilazione widget:

```
17:18  Widget [WBP_RT_SelectedUnitPanelBottom] was added but did not get a GUID   :781
17:28  Variable [WBP_RT_ActionDock] was deleted but still has a GUID
       referenced by WidgetBlueprint [WBP_RT_TacticalHUD]                         :815
18:19  Widget [WBP_RT_EventLogRight] was added but did not get a GUID             :781
```

Causa: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` lasciato con la mappa GUID incoerente da
operazioni `UMGToolSet` (`AddWidget`, `RenameWidget`, `WrapWidgets`, `ToggleWidgetAsVariable`) di una
**sessione parallela**. Sbloccato con `git stash push` sul solo asset — `stash@{0}`, recuperabile.

⚠️ **E si è riprodotto durante la seduta**: l'asset è tornato `M` alle 18:22 con un terzo ensure
(`WBP_RT_EventLogRight`), mentre la seduta era in corso. Non è un incidente isolato: è il costo di
authoring asset da due sessioni sulla stessa macchina, che `CLAUDE.md` §2 descrive come *«Unreal è uno
per macchina»*.

## 5. Esito da produrre

Un verdetto, non un'impressione. Al termine si deve poter rispondere a:

1. il DoD di `TD-EDITOR-01` è **soddisfatto**, sì o no;
2. il ghost dice *invalido* **prima** del click, sì o no;
3. quanti passaggi fuori dal 2D richiede il giro completo — un numero e l'elenco;
4. `TD-EDITOR-02` e `TD-EDITOR-03` sono ancora bloccate da questa slice, sì o no.

Con queste quattro risposte si decide cosa costruire. Senza, qualunque progetto di tool 2D poggia su
un'assunzione.

⚠️ **Se la seduta viene eseguita, va registrata**: `editor-sessions.yaml` è l'owner di *quale voce e
quando*. Questo file non può inventarsi una seduta — può solo dire che manca.

✅ **E non manca più, dal 2026-09-10: è `U50`**
([#1527](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1527)). *La riga qui sopra
proseguiva con «e oggi nessuna sua seduta nomina il Composer», ed era vera fino a quel giorno.*
`U50` dichiara `verifies: [PIE-SCEN-COMPOSER]`, `issues: [1527, 1105]` e
`shares_setup_with: [U31, U32]` — la stessa apertura di `L_DevSandbox` su cui quelle due già
guardano il tab **Tactical Designer**, quindi la seduta entra in sequenza senza reclamare un terzo
avvio dell'Editor.

⛔ **Aprire la seduta non è eseguirla**, e i due passi hanno validazioni diverse: il primo si misura
con un `grep`, il secondo vuole il motore. Il DoD resta **osservato a metà** finché due unità su celle
diverse non sono state viste a schermo.


---

## 6. ESITO FINALE — la voce ha cambiato soggetto, e l'asset non c'è più

> Aggiunto il 2026-09-09 a valle di [#2789](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2789), chiusa.

La seduta di §4bis ha risposto alla domanda per cui era stata convocata, e la risposta ha reso obsoleto
il suo stesso oggetto.

**`WBP_RT_ScenarioComposer` è stato rimosso.** Aperto nell'editor UMG, la sua gerarchia era
`[WBP_RT_ScenarioComposer] → [RootBox]`: un guscio. Il DoD che questa seduta doveva verificare —
*«piazza almeno due unità, salva, riapri»* — non era eseguibile lì, e non lo era mai stato.

**La voce `PIE-SCEN-COMPOSER` è stata ripuntata, non chiusa**, su `SRTLauncherScenarioPanel`. L'ID resta
invariato: rinominarlo avrebbe rotto i rimandi da `editor-sessions.yaml` e dai referti già scritti.

### Cosa è stato verificato sulla superficie viva

Da [#2786](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2786), il pannello ha i quattro
gesti. Osservato pilotando l'Editor via MCP:

```
readout PRIMA:  squadre  team 0: 1 · team 1: 1  |  unità 2
readout DOPO :  squadre  team 0: 1 · team 1: 2  |  unità 3
```

e il file su disco: `{"id": "U1", "hero": "Hero.Aevik", "team": 1, "cell": [0, 0, 0]}`.

⚠️ **Il DoD resta parzialmente osservato**: il secondo `+ UNIT` è stato rifiutato da `AddUnit` perché la
cella era la stessa. Il rifiuto è corretto — è l'autorità della facade — ma non dimostra *due* id
distinti a schermo. Quello lo dimostra `RefactorTactics.DevSandboxLauncher.CoinUnitIdAvoidsCollisions`,
headless.

### I «cinque rilievi» di #1527 restano perduti

`#1527` è stata ripopolata dichiarando che il suo corpo era il letterale `@-`. I rilievi trovati in
questa seduta **non li sostituiscono**: sono altri, e hanno preso issue proprie (#2786, #2788, #2789)
invece di essere infilati sotto un titolo che promette altro.

### 🔑 Il difetto che questa seduta ha davvero trovato

Non era nel widget. Era nel **contratto**, che descriveva minuziosamente controlli mai costruiti su una
superficie che non si raggiungeva. Chi lo leggeva concludeva che il piazzamento fosse a portata di clic —
e fino al 2026-09-09 non lo era da nessuna parte.
