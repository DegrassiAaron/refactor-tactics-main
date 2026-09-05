# HUD v0.1 — Lane B · EDITOR / MCP / USER

> `EPHEMERAL` · **Vista esecutiva temporanea, non un registro.** L'esito delle verifiche vive in
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), che ne resta l'**unico owner**; la sequenza
> delle sedute vive in [`editor-sessions.yaml`](../editor-sessions.yaml); chi esegue cosa lo dichiara
> [`scenario-map.md`](../../technical/tooling/scenario-map.md).
>
> **Questo file non registra verdetti.** Raggruppa i check in aperture e dice, per ciascuno, *che difetto
> può scoprire che nessun test copre*. Quando un check viene eseguito, l'esito si scrive **là**, non qui.
>
> **Misurata**: 2026-09-05 su `origin/main` `8a530c6e`, modalità **DEV**.
> **Tutti i `Result` sono `NOT RUN`**: questa passata non ha aperto l'Editor. L'unica misura eseguita è il
> probe del ponte MCP in `E0`.

## Come si legge

Consuma la Lane A: [`hud-v01-code-architecture-roadmap.md`](hud-v01-code-architecture-roadmap.md).
Baseline e numeri: [`hud-v01-three-terminals-audit-2026-09-05.md`](hud-v01-three-terminals-audit-2026-09-05.md).

Classi, e non sono intercambiabili:

```text
«Che proprietà ha?»          → MCP_READ
«Posso collegarla?»          → MCP_WRITE
«La regola è corretta?»      → AUTO
«Si vede / si capisce?»      → USER_PIE
«Il click si comporta bene?» → USER_PIE
«Funziona cooked?»           → USER_PACKAGED
```

⛔ **`MCP command sent` non è `verified`.** `result=null`, output vuoto o assenza di errore **non** sono
`PASS`. Oracoli validi: rileggere la proprietà, riaprire l'asset, compilare esplicitamente, un test, il PIE,
il packaged.

⚠️ **Due trappole misurate oggi, entrambe scritte da chi ci è caduto.**

1. **Il `grep` sul `.uasset` non misura il cablaggio.** Dopo aver scollegato `PlanningSecondsRemaining` da
   `WBP_RT_TurnHeader`, la stringa compare ancora **3 volte** nel binario salvato: il nodo `Break` espone un
   pin per ogni campo della struct. Contare la stringa avrebbe detto «non è cambiato niente».
2. **In play mode il ponte risponde vuoto senza errore.** `SlateInspector.Snapshot` dà vuoto e
   `CaptureEditorImage` fallisce: un elenco vuoto **sembra** una misura. Fuori da PIE si rimisura; se resta
   assente, `NOT RUN` col motivo.

### Quando un check FALLISCE

```text
EDITOR FAIL
   ↓  classifica: CODE | ASSET | SETUP | SPEC | TOOLING
   ↓  trova l'owner esistente (colonna «Failure owner»)
   ↓  correggi nella lane giusta
   ↓  validazione mirata
   ↓  rilancia il check
```

⛔ **Non correggere in silenzio il widget per compensare un contratto runtime sbagliato.**

---

## Riepilogo e aperture

| ID | Consuma | Classe | Voce owner | Seduta dichiarata | Result |
|---|---|---|---|---|---|
| `E0` Pre-flight | — | `MCP_READ` | — | — | **misurato in parte**, vedi sotto |
| `E1` Catalogo icone | `C1` | `MCP_READ` | `PIE-ICON-01` | U9 · U43 | `NOT RUN` |
| `E2` Gerarchia e binding UMG | `C2` `C3` | `MCP_READ` | DoD #613 | — | `NOT RUN` |
| `E3` Layout e ingombro | `C3` | `USER_PIE` | `PIE-V01-SCREENHUD` | ⚠️ **nessuna** | `NOT RUN` |
| `E4` Matrice di stato HUD | `C2` `C7` | `USER_PIE` | `PIE-V01-HUD` · `PIE-ACC-HUD` | U15 · ⚠️ nessuna per `ACC` | `NOT RUN` |
| `E5` Pointer / hit-test | `C6` | `USER_PIE` | `PIE-V01-POINTER` | U43 | `NOT RUN` |
| `E6` Privacy / conoscenza | `C4` `C5` `C6` | `AUTO` + `USER_PIE` | `PIE-KNOW1…5` | U43 | `NOT RUN` |
| `E7` Ghost Timeline / scrubbing | `C5` | `USER_PIE` | `PIE-V01-GHOSTS` | ⚠️ **nessuna** | `NOT RUN` |
| `E8` Player Event Log | `C4` | `USER_PIE` | `PIE-V01-LOG` | U15 · U43 · U46 | `NOT RUN` |
| `E9` UnitOverlay / status | `C8` | `USER_PIE` | `PIE-ICON-02` · `PIE-VIS-*` | U47 · U48 | `NOT RUN` |
| `E10` Ready / Unready | `C7` | `USER_PIE` | `PIE-V01-READY` | U19 | `NOT RUN` |
| `E11` Debug sviluppatore | `C9` | `USER_PIE` | `PIE-V01-DEBUG` | U15 · U46 | `NOT RUN` |
| `E12` Sottoinsieme packaged | tutte | `USER_PACKAGED` | `PIE-V01-PACKAGED` | — | `NOT RUN` |
| `E13` Riconciliazione delle evidenze | — | — | i tre registri | — | `NOT RUN` |

### Due aperture, non tredici

**SESSIONE A — HUD integration pass** (dopo il batch VALIDATION): `E0 → E1 → E2 → E3 → E4 → E5 → E6 → E7 →
E8 → E9 → E10 → E11`, poi salvataggio dei soli asset intenzionali, rilettura dello stato dirty, `git status`.

**SESSIONE B — final verify**: **solo** se la Sessione A non può produrre l'evidenza finale dopo l'ultimo
rebuild. Cambiare mappa, fermare e riavviare il PIE o eseguire un altro scenario **non** giustificano una
seconda apertura; salvare asset o un restart pulito sì.

⚠️ **Tre voci non hanno una seduta in `editor-sessions.yaml`** — `PIE-V01-SCREENHUD`, `PIE-V01-GHOSTS`,
`PIE-ACC-HUD`. Chi esegue la Sessione A le aggiunga **dopo `git fetch --prune`**: `U-nnn` è un contatore
condiviso e non si assegna dalla memoria.

---

## E0 — Pre-flight

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E0` |
| **Consumes** | — |
| **Owner** | nessuno: è la precondizione di tutti gli altri |
| **Class** | `MCP_READ` + `AUTO` |
| **Preconditions** | stesso checkout dei terminali DEV/VALIDATION; `git rev-parse --show-toplevel`, `HEAD` e branch coincidenti |
| **Setup** | nessun altro processo Unreal; nessuna suite/build/commandlet concorrente |
| **Action** | avviare l'Editor sul checkout dichiarato → scoprire i toolset MCP realmente disponibili → aprire la mappa di verifica → attendere la stabilizzazione dell'asset registry |
| **Expected** | l'Editor apre senza errori di compilazione, la mappa carica, il ponte risponde |
| **Failure class** | checkout sbagliato · moduli da ricompilare · asset registry non stabilizzato · ponte spento |
| **Evidence** | l'elenco dei toolset scoperti, non «il ponte c'è» |
| **Result** | 🟡 **parziale, misurato il 2026-09-05 alle 13:58**: `127.0.0.1:8765`, `:8767`, `:8000` → **HTTP 000**, e **zero processi Unreal**. Il resto: `NOT RUN` |
| **Failure owner** | `SETUP` |
| **Regression set** | — |

🔑 **Il ponte giù non è un blocco: è una conseguenza.** Vive dentro l'Editor, e senza Editor non risponde.
Che la capability esista è misurato: alle **09:15 del 2026-09-05** un'altra sessione ha riscritto l'Event
Graph di `WBP_RT_TurnHeader` via MCP (`break_pins` → `connect_pins` → `compile_blueprint` → `save_assets`).
**Non dichiarare `BLOCKED` la Lane B su un probe fatto a Editor chiuso.**

⛔ **Qui non si giudica ancora il layout.**

---

## E1 — Audit catalogo e asset

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E1` · **Consumes** `HUD-CODE-C1` |
| **Owner** | `#220` · voce `PIE-ICON-01` (⛔ nel registro) |
| **Class** | `MCP_READ`, con `USER_PIE` per la sola parte visiva |
| **Preconditions** | `E0` |
| **Asset** | `/Game/RT/UI/DA_IconCatalog` · `/Game/RT/UI/Icons/RT_UI_Icon_*` (66) |
| **Setup** | nessuno |
| **Action** | rileggere le chiavi del catalogo dall'asset; confrontarle con `URTIconLibrary::RequiredIconIds()`; cercare riferimenti diretti a texture, redirector, collisioni di nome, placeholder |
| **Expected** | 61 chiavi, 61 risolte, zero `MissingIcon` fra le richieste, zero texture referenziate direttamente da un widget |
| **Failure class** | ciò che i 6 test `IconCatalog.*` **non** vedono: texture non caricata, dimensione 0, redirector non risolto, asset sorgente importato due volte |
| **Evidence** | il dump delle chiavi + la tabella qui sotto compilata |
| **Result** | `NOT RUN` — `strings` sul `.uasset` dà **0** occorrenze (payload compresso): il conteggio delle 61 chiavi viene dagli owner, non da una misura recente |
| **Failure owner** | `ASSET` → `#220`; se manca una chiave **richiesta** → `CODE`/`ASSET` su `#219` |
| **Regression set** | `RefactorTactics.IconCatalog` · `RefactorTactics.ScreenHud` |

Tabella dei gap, da compilare durante la seduta:

| Asset / chiave | Richiesta da | Esiste | Collegata | PIE | Packaged | Azione |
|---|---|---|---|---|---|---|
| | | | | | | |

⛔ **`PIE-ICON-01` non si esegue dal ponte**: il dock si popola solo con un'unità selezionata, e `SelectUnit`
non è una `UFUNCTION`. Serve una persona, **oppure** uno scenario che selezioni — e quella è la via che
costa meno.

---

## E2 — Gerarchia e binding UMG

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E2` · **Consumes** `HUD-CODE-C2`, `C3` |
| **Owner** | DoD di `#613` |
| **Class** | `MCP_READ` (il `MCP_WRITE` solo se il run è autorizzato a implementare) |
| **Preconditions** | `E0` |
| **Asset** | `WBP_RT_TacticalHUD` · `TurnHeader` · `TeamRoster` · `SelectedUnitPanel` · `ActionDock` · `ActionSlot` · `UnitCard` · `UnitOverlay` |
| **Setup** | nessuno |
| **Action** | leggere la gerarchia e i binding di ciascun widget **dopo** una compilazione esplicita |
| **Expected** | ogni testo che deve variare ha un binding, non un placeholder statico; classe base corretta; visibilità di default corretta; nessun consumer che salti view model o catalogo |
| **Failure class** | binding mancante · `Text Block` statico rimasto · classe base sbagliata · z-order · hit-test inatteso · widget non montato |
| **Evidence** | il grafo riletto dopo la compilazione — **non** un `grep` sul binario |
| **Result** | `NOT RUN` |
| **Failure owner** | `ASSET` → `#613`; se il dato non c'è → `CODE` → `C2` |
| **Regression set** | `RefactorTactics.ScreenHud` |

✅ Un gate esiste già e va usato come rete: `ScreenHud.DockArmsOnlyTheSelectedAction` prova il
**comportamento** del grafo — arma un'azione e pretende che si accenda **lo slot giusto**. Un grafo scritto
diversamente ma corretto passa; uno che somiglia a quello giusto e non accende niente cade.

---

## E3 — Layout, ingombro, leggibilità

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E3` · **Consumes** `HUD-CODE-C3` |
| **Owner** | `PIE-V01-SCREENHUD` (⏳) e l'estensione di `PIE-V01-HUD` all'ingombro del §4.1 |
| **Class** | `MCP_READ` per la struttura · **`USER_PIE` per il giudizio** |
| **Preconditions** | `E0`, `E2`; partita v0.1 avviata con `WBP_RT_TacticalHUD` montato |
| **Setup** | risoluzione di gioco della v0.1, non la finestra dell'Editor ridimensionata |
| **Action** | guardare header, roster, pannello selezione, dock, feed, controlli del piano; muoversi in Planning e durante il playback della risoluzione |
| **Expected** | **centro della board libero**; nessuna sovrapposizione critica; nessun testo essenziale tagliato; il debug spento |
| **Failure class** | ⛔ **quella per cui questo check esiste**: layout valido e board illeggibile. Nessun test la vede |
| **Evidence** | screenshot alle risoluzioni target + il log corrispondente |
| **Result** | `NOT RUN` |
| **Failure owner** | `ASSET` → `#613`; se è il §4.2 a coprire il centro → `#2184` / `progettazione-hud.md` §3.1 |
| **Regression set** | `RefactorTactics.ScreenHud` dopo ogni modifica di asset |

**Scope**: leggibilità funzionale, non rifinitura. `#613` dichiara questa voce come **unico** residuo, e la
cosa più utile da guardare per prima è il timer durante un countdown di Ready.

---

## E4 — Matrice di stato dell'HUD

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E4` · **Consumes** `HUD-CODE-C2`, `C7` |
| **Owner** | `PIE-V01-HUD` (✅, da estendere) · `PIE-ACC-HUD` (⏳) |
| **Class** | `USER_PIE` |
| **Preconditions** | `E0`; scenario **`Visual.Hud.FirstPlayable`** — esiste: `Scenarios/Visual/Hud/FirstPlayable.json` |
| **Setup** | `rt.Test.Scenario Visual.Hud.FirstPlayable` + Play |
| **Action** | attraversare gli stati: Planning inerte · unità selezionata · anteprima di percorso · anteprima di bersaglio · azione selezionata · cooldown · azione non valida · Ready · countdown · Resolution · ReactionWindow · Modal · unità danneggiata · scudo/energia che cambiano |
| **Expected** | per ogni stato: **quale dato è cambiato**, **quale widget deve rispondere**, e **quale widget NON deve cambiare** |
| **Failure class** | un widget che si aggiorna quando non dovrebbe, o che resta fermo su un dato vecchio: nessun test lo vede perché ciascuno guarda un widget alla volta |
| **Evidence** | video della sequenza + il TurnLog dello stesso turno |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `C2` se il dato è sbagliato · `ASSET` → `#613` se è il binding |
| **Regression set** | `RefactorTactics.HudViewModel` · `RefactorTactics.ScreenHud` |

---

## E5 — Pointer e hit-test

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E5` · **Consumes** `HUD-CODE-C6` |
| **Owner** | `PIE-V01-POINTER` (⏳) · seduta **U43** |
| **Class** | `USER_PIE` |
| **Preconditions** | `E0`, `E3`; ⚠️ la precedenza HUD→mondo è verificabile **solo** dopo che i widget registrano hitbox (`AddHitBox` è a **0** occorrenze) |
| **Action** | rimisurare la matrice corrente: Hover · LMB · RMB · HUD sopra il mondo · Modal · ReactionWindow · Planning · Pathing · Targeting · ResolutionPlayback · bersaglio non valido · Cancel · click-through · nemico nascosto · ghost alleato in sola lettura |
| **Expected** | l'hover non committa mai; l'RMB annulla solo l'anteprima e non deseleziona; ogni rifiuto porta un **motivo** |
| **Failure class** | hit-test e z-order — la classe che nessun test headless raggiunge. E un rifiuto **muto**: se il contratto prevede un reason, «non è successo niente» è un `FAIL` |
| **Evidence** | video + il reason code corrispondente nel log |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `#705`; overlay dell'hover → `#1614`; Back sullo scatto → `#1402` |
| **Regression set** | `RefactorTactics.PlayerInput` · `RefactorTactics.Pointer` |

---

## E6 — Privacy e conoscenza

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E6` · **Consumes** `HUD-CODE-C4`, `C5`, `C6` |
| **Owner** | `PIE-KNOW1…5` · seduta **U43** |
| **Class** | `AUTO` per il contratto · `MCP_READ` sui consumer · `USER_PIE` sulla presentazione finale |
| **Preconditions** | `E0`; una partita con un nemico non rilevato |
| **Action** | verificare che a schermo non compaiano intenti privati avversari, facing pianificato avversario, overlay su nemici nascosti, e che il feed non nomini ciò che l'osservatore non sa |
| **Expected** | nessuna fuga; l'eccezione della morte pubblica **solo** dove è canonica |
| **Failure class** | una fuga che il DTO non ha, introdotta dal widget o da un comando di debug |
| **Evidence** | due viste affiancate (osservatore A / osservatore B) sullo stesso turno |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` — e **mai** una pezza nel widget |
| **Regression set** | `RefactorTactics.UI.NoEnemyIntentExposed` · `UI.LogOmitsRememberedEnemy*` · `RefactorTactics.Reactions` |

⛔ Il contratto è già coperto headless (`NoEnemyIntentExposed` pinna due scene che differiscono **solo** per
i piani nemici). Quello che questa voce aggiunge è il **canale visivo**: un debug acceso, un overlay non
filtrato, un log che nomina.

---

## E7 — Ghost Timeline e scrubbing

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E7` · **Consumes** `HUD-CODE-C5` |
| **Owner** | `PIE-V01-GHOSTS` (⏳) — ⚠️ **nessuna seduta dichiarata** |
| **Class** | `AUTO` + `USER_PIE` |
| **Preconditions** | `E0`; un'unità propria con un piano su più fasi |
| **Action** | un ghost **per fase** (Prep · Dash · Blast · Move): origine, destinazione, facing, bersagli, AoE, certezza, ramo della reaction; poi lo scrubbing fra le fasi |
| **Expected** | **la preview mostrata al giocatore == la semantica usata dal resolver** |
| **Failure class** | preview semanticamente corretta e visivamente ambigua — e il contrasto **linea↔superficie** già misurato (`dE 1.9`–`2.1` nel caso peggiore) |
| **Evidence** | screenshot su almeno tre superfici diverse (`Floor`, `ShallowWater`, `Ice`) + il percorso che il resolver dichiara |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `#172`/`#173`. ⛔ **Se la preview diverge, si corregge il codice, non la regola di rendering locale** |
| **Regression set** | `RefactorTactics.Preview` |

🔴 **Prima di eseguire, la decisione aperta di `#172` va presa**: senza sceglierla, un `FAIL` di contrasto
non ha owner e la seduta produce un'osservazione senza sbocco.

---

## E8 — Player Event Log

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E8` · **Consumes** `HUD-CODE-C4` |
| **Owner** | `PIE-V01-LOG` (🟡, `RELEASE-V01`) · sedute **U15** · **U43** · **U46** |
| **Class** | `AUTO` + `MCP_READ` + `USER_PIE` |
| **Preconditions** | `E0`; **la parte F di `#1936` deve esistere** (nessun `WBP_RT_EventLog` oggi) |
| **Setup** | un turno con un **fallback** e una **modifica ambientale** |
| **Action** | leggere il feed durante un turno risolto; confrontarlo con il TurnLog diagnostico dello stesso turno |
| **Expected** | feed conciso, niente spam per-cella, dominanza applicata, KO/danno/blocco/reaction/obiettivo leggibili, ordine coerente, privacy rispettata, **e la board ancora usabile** |
| **Failure class** | raggruppamento corretto nei test e feed inutilizzabile a schermo; oppure un feed che copre il piano |
| **Evidence** | ⚠️ **accoppiata obbligatoria**: screenshot/video **+** il log diagnostico corrispondente |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `#1936` (parti A/F), `#1937` (asse semantico), `#79` (reason code) |
| **Regression set** | `RefactorTactics.UI` |

🔴 **Difetto già misurato in PIE (U14, 2026-09-04), e il feed lo eredita**: *«ho provato e me l'hanno
negato»* e *«non ho provato»* producono **la stessa riga**. Ciò che li distingue è un `Warning` emesso dal
runner degli scenari — in una partita normale quella riga non esiste.

---

## E9 — UnitOverlay e status

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E9` · **Consumes** `HUD-CODE-C8` |
| **Owner** | `PIE-ICON-02` (⛔) · `PIE-VIS-UNBAL` · `PIE-VIS-PRONE` · `PIE-VIS-SLIDESTATE` · sedute **U47** · **U48** |
| **Class** | `AUTO` + `MCP_READ` + `USER_PIE` |
| **Preconditions** | `E0`; fixture minima: **uno** stato legato alla cella, **uno** a durata, e più stati insieme se l'ordine conta |
| **Action** | guardare l'overlay: icona davvero visibile, nessun ripiego testuale indesiderato, contatore **solo** sugli stati a durata, HP/scudo/energia leggibili, nessun overlay su nemico nascosto, comportamento ai bordi del viewport e dietro la camera |
| **Expected** | le due icone di movimento si distinguono **fra loro, alla dimensione in cui si giocano** |
| **Failure class** | ⛔ **un test verde del resolver di icone non chiude la visibilità**: texture non caricata, dimensione 0, clipping |
| **Evidence** | screenshot alla dimensione reale, non ingrandito |
| **Result** | `NOT RUN` |
| **Failure owner** | `ASSET` → `#2378`/U48 · `CODE` → `C8` |
| **Regression set** | `RefactorTactics.HUD` · `RefactorTactics.HudViewModel` |

---

## E10 — Ready / Unready

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E10` · **Consumes** `HUD-CODE-C7` |
| **Owner** | `PIE-V01-READY` (⏳) · seduta **U19** |
| **Class** | `USER_PIE` (l'`AUTO` è già passato: la regola dei due orologi ha i suoi test) |
| **Preconditions** | `E0`; `#2193` atterrata — lo è |
| **Action** | **Spazio** dichiara Ready, parte il countdown, si annulla, si lascia scadere |
| **Expected** | il countdown si vede; Ready **non** sembra un commit; l'Unready si capisce; l'anteprima torna; nessun doppio commit; la transizione a Resolution è leggibile |
| **Failure class** | dato corretto e **feedback assente** — il caso che ha aperto `#2193` |
| **Evidence** | video del ciclo completo, incluso il caso «lascia scadere» |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `#2390` se l'anteprima resta accesa al timeout |
| **Regression set** | `RefactorTactics.HudViewModel` · `RefactorTactics.HUD` |

---

## E11 — Debug per lo sviluppatore

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E11` · **Consumes** `HUD-CODE-C9` |
| **Owner** | `PIE-V01-DEBUG` (🟡) · sedute **U15** · **U46** |
| **Class** | `AUTO` + `USER_PIE` |
| **Preconditions** | `E0`; build Development o PIE |
| **Action** | per ognuno dei **12** comandi live: esiste, risponde, e **se dice `Draw` disegna davvero** |
| **Expected** | `DrawCells` mostra `CellId`/`TerrainId`/`TraversalCost`; ⛔ `DrawPaths`, `DrawCover`, `DrawResolution` oggi **stampano** — il verdetto atteso è che il difetto si veda, non che passi |
| **Failure class** | un comando che promette un disegno e produce testo; un debug **acceso di default** nella vista del giocatore |
| **Evidence** | screenshot per ciascun `DrawX` + l'output della console |
| **Result** | `NOT RUN` |
| **Failure owner** | `CODE` → `#80` |
| **Regression set** | `RefactorTactics.Debug` |

⛔ **Privacy**: `rt.Debug.DrawIntent` non deve rivelare gli intenti avversari. Oggi è banale perché il gioco è
offline — ed è ora che non si crea l'abitudine sbagliata.

---

## E12 — Sottoinsieme packaged

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-EDITOR-E12` · **Consumes** tutte |
| **Owner** | `PIE-V01-PACKAGED` |
| **Class** | `USER_PACKAGED` |
| **Preconditions** | tutte le voci `USER_PIE` chiuse |
| **Action** | **solo** ciò che aggiunge una failure class: il cook riesce; l'HUD si monta; icone/materiali/animazioni sono presenti; nessun debug acceso; l'avvio arriva alla partita |
| **Expected** | vista giocatore pulita in build cotta |
| **Failure class** | asset editor-only finito nel cook, riferimento non cotto, default diverso in packaged |
| **Evidence** | screenshot dal packaged + il log di cook |
| **Result** | `NOT RUN` |
| **Failure owner** | `ASSET` / `TOOLING` |
| **Regression set** | — |

⛔ **Non replicare l'intera campagna PIE in packaged.**

---

## E13 — Riconciliazione delle evidenze

Aggiornare **solo** gli owner vivi, e **solo** se lo stato è cambiato davvero:

| Dove | Cosa |
|---|---|
| [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) | l'esito della voce `PIE-*` — **è l'owner del verdetto** |
| [`editor-sessions.yaml`](../editor-sessions.yaml) | le sedute mancanti per `PIE-V01-SCREENHUD`, `PIE-V01-GHOSTS`, `PIE-ACC-HUD`, dopo `git fetch --prune` |
| issue owner | un commento **solo** quando la seduta ha misurato qualcosa che l'issue non sa |
| [`roadmap-v0.1.md`](../roadmap-v0.1.md) / [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) | solo se un checkpoint cambia stato |

Stati ammessi: `PASS` · `FAIL` · `BLOCKED` · `NOT RUN`.

⛔ **«Non eseguito» non è verde**, e una voce che ha una `PIE-*` mette il proprio verdetto **nel registro**:
una issue non lo sostituisce.

---

## Report per seduta

```text
SESSION:
ROOT:
BRANCH:
HEAD before:
HEAD after:
Global mode:
Unreal version:
MCP tools discovered:
MCP tools actually used:
Map(s) opened:
Assets opened:
Assets modified:
Assets saved:
Assets dirty at end:
PIE checks:
Packaged checks:
Screenshots/video:
Property dumps:
Relevant logs:
Issues verified:
Issues failed:
Issues blocked:
NOT RUN:
git status after:
```

Per ogni `PASS` visivo: allegare l'evidenza, **oppure** spiegare perché un altro oracolo basta.
