# Sedute `U61` e `U63` — la barra dei comandi: lo slot, poi la fascia

> **Tipo**: foglio di conduzione · **Scritto**: 2026-10-05, notte, su `main` = `473b285f4` · **Owner del
> criterio**: le voci `U61` e `U63` di [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Questo
> foglio **non** ridefinisce i criteri: dove divergono, vale la voce.
>
> ⚠️ **Mai eseguito.** È scritto **prima** della seduta. Ogni nome di widget e di funzione qui sotto è
> stato letto dal codice o dalla tabella dei nomi dei `.uasset` (`tools/uasset/names.py`), non dall'Editor.
> La §6 dichiara ciò che da fuori l'Editor non si poteva verificare.
>
> 🔑 **Una sola apertura per entrambe**: le due voci si dichiarano `shares_setup_with` a vicenda, e toccano
> gli stessi due Blueprint, `WBP_RT_ActionSlot` (U61) e `WBP_RT_ActionDock` (U63). Prima lo slot, poi la dock.

---

## 0. Preflight

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'LiveCodingConsole%'" |
  Select-Object ProcessId, Name, CommandLine
```

Deve essere **vuoto**. Il clone principale può essere in uso da un'altra sessione: chiedi prima di cambiargli
branch ([`AGENTS.md`](../../../AGENTS.md) §11).

**Precondizione [D-403]: un commit di `main` che contenga tutto il C++ che le due voci leggono**, con
l'Editor **ricompilato** da quel commit. Una DLL di prima non avrebbe i pin.

| Che cosa | Da dove | PR |
|---|---|---|
| `Phase`, `PhaseMark`, `PhaseLabel` sulla vista | #3465 | #3467 |
| `Group` | #3468 | #3475 |
| `Zone_Bottom`, la fascia unica | #3469 | #3477 |
| `GetActionsInReadingOrder()` | #3478 | #3479 |
| `GetMovementReadout()` | #3470 | #3482 |

```bash
git fetch origin && git merge-base --is-ancestor aa85b271c origin/main && echo "main contiene tutto"
```

Apertura dal **clone principale**, con `-NoLiveCoding` perché non blocchi le build degli altri cloni.

---

## 1. `U61` — lo slot dice la fase, e i suoi stati reggono la scala di grigi

**Blueprint**: `/Game/RT/UI/Match/WBP_RT_ActionSlot`. Oggi contiene `IconImage`, `NomeText`, `CooldownText`
(con binding), `ArmedBorder` (visibilità con binding) e il bottone `ClickSurface`, dentro un `Overlay`.
L'evento `OnActionChanged` chiama `ApplyResolvedIconTo`. **Nessuno** dei campi di fase è ancora usato.

### 1.1 Striscia ed etichetta di fase

1. Aggiungi in cima all'`Overlay` un `Border` alto **4 px**, allineato in alto e stirato in larghezza: è la
   **striscia di fase**.
2. Aggiungi un `TextBlock` in alto a destra: è l'**etichetta di fase**, legata a `Action.PhaseLabel`. Font
   Orbitron, 8 px di design.
   ⛔ **L'etichetta non si omette mai**: è il canale che non dipende dal colore ([D-232] punto 3).
3. Colora la striscia con un `Select` su **`Action.PhaseMark`**, mai su `Slot` o su `ActionId`: il segno
   arriva già deciso dal C++ (`PhaseMarkFor`).

| `PhaseMark` | Colore | Fonte |
|---|---|---|
| `Prep` | `#56B4E9` | [D-233] |
| `Dash` | `#009E73` | [D-233] |
| `Blast` | `#D55E00` | [D-233] |
| `Move` | `#0072B2` | [D-233] |
| `Reaction` | `#7C5CFF` (`RT_UI_Violet`) | `progettazione-hud.md` §32 |
| `Cleanup` | **nessuna tinta**: tratteggio neutro più l'etichetta. Se serve un neutro, il candidato già in palette è `RT_UI_Frame_Mid #4A5568` | [D-232] §1, [D-233] |
| `None` | **nessuna striscia** | — |

### 1.2 Gli stati, ciascuno col proprio secondo canale

Lo stato si legge da **`ResolveSlotState(Action, bArmed)`** (`URTHudViewModel`, `BlueprintPure`). ⛔ Non va
ricomposto nel grafo da `bPlanned`, `TurnsRemaining` e `bUsableNow` (#2988). Le ricette vengono da
[`SPECIFICA-VISIVA.md`](../../research/design/hud/skill-bar-2026-10/SPECIFICA-VISIVA.md) §3:

| Stato | Frame | Secondo canale (obbligatorio) |
|---|---|---|
| Available | `BG_Raised #212733`, bordo 1 px `Frame_Mid #4A5568` | — |
| Hover | fondo più chiaro, bordo 1 px `Cyan #00E0FF` | luminosità — **del widget**, non del modello |
| Selected | bordo **2 px** `Amber #FFD456` | **barra 40×4 sotto lo slot** |
| Planned | bordo **2 px** `Amber` | **angolo pieno 18 px** in alto a destra |
| Cooldown | `BG_Panel #151A23`, striscia al 30% | **numero di turni**, 26 px, sopra l'icona (`CooldownText` c'è già) |
| Unavailable | — | **tratteggio diagonale 135°** |
| Invalid | bordo 2 px `Red #FF4D4D` | **✕** in alto a destra — ⚠️ **ricetta sì, produttore no**: vedi #3483 |
| Warning | bordo **2 px tratteggiato** `Amber` | **triangolo !** in alto a destra — come Invalid |

⚠️ **Selected e Warning condividono l'ambra**: li separa la forma, cioè bordo continuo più barra contro bordo
tratteggiato più triangolo.

⛔ **I colori del mockup fuori da §32** (`dati/tokens.json`, sezione `testo_mockup_non_in_style_guide`) non
si coniano come token nuovi senza conferma.

### 1.3 La prova: cattura in scala di grigi

1. In PIE su `L_DevSandbox`, con un'unità comandata, porta gli slot negli stati che hanno un produttore:
   - **Available**: niente armato;
   - **Selected**: arma un'azione;
   - **Planned**: pianificala;
   - **Cooldown**: il turno dopo una skill con ricarica.

   Invalid e Warning **non** hanno produttore (§1.2): la loro ricetta si verifica nel Designer.
2. Cattura e converti:

   ```bash
   python -c "from PIL import Image; Image.open('cattura.png').convert('L').save('docs/technical/evidence/hud/u61-stati-slot-scala-di-grigi.png')"
   ```

3. **Criterio** (`progettazione-hud.md` §47-bis.1): in grigio **nessuno stato si confonde con un altro**.
   Il riferimento è la riga inferiore di
   [`immagini/08-stati-slot.png`](../../research/design/hud/skill-bar-2026-10/immagini/08-stati-slot.png).

---

## 2. `U63` — la barra occupa la fascia e legge i propri gruppi

**Blueprint**: `/Game/RT/UI/Match/WBP_RT_ActionDock`.

Oggi l'**Event Tick** fa queste cose:
- chiama `GetActions`;
- crea uno `WBP_RT_ActionSlot` per voce in `SlotBox` (un `HorizontalBox`), con `AddChildToHorizontalBox`;
- li ritrova con `GetChildAt`, per indice, e chiama `SetAction`;
- compone il prompt di targeting da `GetPointerContext` e `GetPointerTargetKind`.

La dock vive nel `Content` di **`Zone_Bottom`**, `(0.0, 0.8) → (1.0, 1.0)`.

### 2.1 La sorgente: `GetActionsInReadingOrder()`

Sostituisci il nodo `GetActions` con **`GetActionsInReadingOrder`**: stesso tipo di ritorno, stesse voci,
ordine Comuni → Base → Kit.

🔑 **Il resto del grafo non cambia, ed è il punto**:
- il figlio *i* di `SlotBox` riceve la voce *i* della **lista di lettura**;
- ogni voce porta il proprio `AbilityIndex`, ed è quello che il click arma — `ScreenHud.ActionSlotForwardsItsOwnIndex`;
- la posizione a schermo non è mai un indice ([D-397] punti 2 e 4).

⛔ **Non riordinare nel grafo.** La partizione la fa il C++ (`URTHudViewModel::OrderForReading`, #3478).

⚠️ **Lo stato «armato» si confronta con `Action.AbilityIndex`, mai con l'indice del ciclo.** Con la lista di lettura le due cose **divergono**: le Comuni vengono prima, quindi la posizione 0 a schermo non è più l'attacco base. Lo pinna già `Editor.DockArmedStateReadsTheAbilityIndexNotTheLoopPosition`: se diventa rosso dopo la sostituzione, il grafo stava confrontando la posizione.

### 2.2 I separatori: **padding**, non widget

⚠️ **Non aggiungere widget separatori in `SlotBox`.** Il grafo ritrova gli slot con `GetChildAt(i)`, e un
figlio in più sposterebbe ogni slot successivo sulla voce sbagliata.

La ricetta, nello stesso ciclo di `SetAction`:
1. confronta `Group` della voce *i* con quello della voce *i−1* della **stessa lista di lettura**;
2. sul `HorizontalBoxSlot` dello slot *i*, imposta un padding sinistro di **22 px** se il gruppo cambia,
   **8 px** altrimenti (`dati/tokens.json`: `separatore_gruppi` 22, `gap` 8);
3. facoltativo: l'intestazione `COMUNI` · `BASE` · `KIT` come `TextBlock` dello slot, visibile solo sul
   primo di ogni gruppo, col testo scelto da un `Select` su `ERTActionGroup`. È testo statico: nessun
   campo C++ la porta.

Lungo la lista di lettura i gruppi sono **contigui per costruzione**, quindi «dove `Group` cambia» è
esattamente fra due gruppi. Lungo `GetActions()` non lo sarebbero: in partita Branth e Muiren leggono
`B KKKKK CCCCC KK`.

### 2.3 L'estremità destra: la lettura del movimento

1. Fai **riempire alla dock la larghezza della zona**. Poi, dopo `SlotBox`, uno `Spacer` con `Fill` e un
   `HorizontalBox` «MovementReadout».
2. Dentro: un `TextBlock` legato a `GetMovementReadout().Label` (`Move ×1`, `Sprint ×2`, `Withdraw ×0,25`…)
   e un badge `TextBlock` legato a `.SneakKeyLabel` (`M`), acceso quando `.bSneakDeclared`.
3. **Visibilità**: `Collapsed` quando `.bAuthorized` è falso. ⛔ Mai spento: uno spento direbbe «fermo» di
   un'unità di cui non si sa niente.
4. ✅ **Il badge è cliccabile** ([D-457]): mettilo dentro un `Button` e lega `OnClicked` a **`ToggleSneak()`**
   della dock. Inoltra al corpo del tasto `M`; nessuna regola nel grafo.
5. ⚠️ **Da giudicare a schermo**: con `Sneak` dichiarato e poi `Overwatch` armata, il badge resta acceso
   mentre l'etichetta dice `Withdraw ×0,25`. È coerente — dichiarato contro effettivo — ma se si legge come
   contraddizione, la resa si decide qui.

### 2.4 La prova: cattura col kit da tredici voci

Branth o Muiren, col loadout di default che la partita equipaggia, hanno **tredici** voci: è il caso che ha
motivato la fascia intera ([D-456] punto 2).
- Cattura a 1920×1080 in `docs/technical/evidence/hud/u63-barra-fascia-bassa.png`.
- La barra resta **dentro la fascia** (`Y ≥ 864`), senza scendere nel centro.

### 2.5 ⛔ Cosa NON si fa

- Confirm · Undo in `TopRight`: è `U64`, nella stessa apertura — §2.6.
- La Ghost Timeline: #172.
- La riga d'intestazione del mockup: D-456 punto 6.
- Il selettore di profilo: contraddice D-425.
- La difesa caratteristica: #3130.

---

### 2.6 `U64` — Conferma e Annulla in `TopRight` ([D-458], #3471)

1. Crea `/Game/RT/UI/Match/WBP_RT_PlanCommit` con parent **`URTPlanCommitWidget`**.
2. Due `Button`:
   - `Conferma` → `OnClicked` chiama **`Confirm()`**: dichiara o ritratta il piano dell'unità, come `Invio`;
   - `Annulla` → `OnClicked` chiama **`Undo()`**: l'intero Back del tasto destro, che durante il countdown
     ritira il Ready.

   ⛔ Nessuna regola nel grafo: i pulsanti **inoltrano** alle porte dei tasti.
3. Il testo di `Conferma` è «Conferma» o «Ritira» secondo `IsPlanDeclared()`, col tasto da
   `GetConfirmKeyLabel()`. Quello di `Annulla` porta il tasto da `GetUndoKeyLabel()`.
4. Entrambi spenti quando `HasCommandedUnit()` è falso.
5. Montalo nel `NamedSlot Content` di `Zone_TopRight` (`SetNamedSlotContent`).
6. 🔴 **Nello stesso commit dell'asset**:
   - `ScreenHud.EveryZoneOwnerIsMountedByClass` riceve la riga
     `{ URTPlanCommitWidget::StaticClass(), TEXT("TopRight"), TEXT("#3471") }`;
   - la tabella di `guida-screen-hud-umg.md` §3 aggiorna `Zone_TopRight`.

   Prima dell'asset il gate sarebbe rosso.

⛔ **Non è il `LockIn` di `Spazio`**: [D-458] lo esclude dal pulsante.

## 3. Dopo il salvataggio: rileggere dal disco

Chiudi l'Editor, poi:

```bash
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_ActionDock.uasset --filtro GetActions --unici
# atteso: GetActionsInReadingOrder presente
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_ActionSlot.uasset --filtro Phase --unici
# atteso: PhaseMark / PhaseLabel presenti
```

⚠️ **In un processo fresco**, non nell'Editor che ha scritto gli asset: rileggerebbe dalla memoria.

## 4. I test che devono restare verdi

```bash
python tools/suite/esegui.py RefactorTactics.ScreenHud
python tools/suite/esegui.py RefactorTactics.Editor
python tools/suite/esegui.py RefactorTactics.PlayerInput
```

Contano in particolare:
- `ScreenHud.ActionSlot*` e `ScreenHud.DockSurvivesAHoleInTheKit`;
- `Editor.ActionSlotAppliesTheIconThroughTheCppPort` e `Editor.DockArmedStateReadsTheAbilityIndexNotTheLoopPosition`;
- `PlayerInput.DockClickAndHotkeyReachTheSameAbility` e `ScreenHud.EveryZoneOwnerIsMountedByClass`.

Un rosso qui è un difetto del cablaggio, non un test da aggiornare.

## 5. Dopo

- Committa asset ed evidenze insieme, e marca le due voci come eseguite.
- **Rigiudica a schermo il criterio (1) di `PIE-V01-SCREENHUD`**: l'ingombro delle zone. Il suo ultimo
  verde è sull'albero a otto zone, e la cella lo dichiara in coda.

## 6. ⛔ Cosa questo foglio NON ha potuto verificare

- **Il grafo vero di `WBP_RT_ActionDock`.** La tabella dei nomi dice *quali* nodi esistono (`GetActions`,
  `AddChildToHorizontalBox`, `GetChildAt`, `SetAction`, `SlotBox`), non *come* sono collegati. Che gli slot
  si ritrovino per indice è dedotto da `GetChildAt` e `GetChildrenCount`: verificalo aprendo il grafo prima
  di sostituire la sorgente.
- **Se la dock riempie già la larghezza della zona**, o si dimensiona sul contenuto: dipende dall'allineamento
  del `NamedSlot` di `Zone_Bottom`, che solo il Designer mostra.
- **Il tratteggio diagonale e il bordo tratteggiato**: UMG non li ha come stile nativo, quindi servono una
  texture o un materiale. Il repository non ne ha ancora uno per lo slot.
