# Debug bounds dello Screen HUD via Unreal MCP

Strumentazione di debug che rende visibile l'**ingombro** delle macro-zone dello Screen HUD, per sbloccare
la parte visiva di `PIE-V01-SCREENHUD`
([#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613), CP 11.7).

> **Convenzione sui numeri.** Dove serve un conteggio, questo documento scrive `xx` e accanto il comando che
> lo produce. Un numero in prosa invecchia da solo e nessun gate lo rimisura.

⚠️ **Non è una modifica estetica di produzione.** È uno strumento temporaneo con un criterio di rimozione
(§15). Se resta dentro senza che nessuno decida che ci resta, è debito su file binari.

---

## 0. Preflight — prima di toccare qualunque cosa

🔴 **Questa sezione non esisteva nella prima stesura, e la sua assenza era il rischio più caro.**
`CLAUDE.md` §10 lo impone e nessuno script lo verifica più: è a carico di chi lavora.

### 0.1 Il motore è uno

```powershell
Get-Process UnrealEditor*, UnrealEditor-Cmd* -ErrorAction SilentlyContinue |
  Select-Object Id, ProcessName, @{n='Threads';e={$_.Threads.Count}}, MainWindowHandle
```

Se compare qualcosa, **fermati e capisci di chi è** prima di compilare, salvare asset o avviare PIE:

```powershell
Get-CimInstance Win32_Process -Filter "Name like 'UnrealEditor%'" |
  Select-Object ProcessId, CreationDate, CommandLine | Format-List
```

* Un `UnrealEditor-Cmd` con **decine di thread** è una run di automation viva — probabilmente di un'altra
  sessione. Compilare adesso riscrive i binari **sotto la sua misura** e la rende `NON VALIDA`.
* Un processo con **un solo thread**, `MainWindowHandle = 0` e working set quasi nullo è uno **zombie**: non
  sta misurando niente, ma tiene il mutex globale di Live Coding. Si supera con `-NoHotReloadFromIDE`, e
  **solo** se nessun editor vero è aperto.
* La `CommandLine` dice il checkout: un editor aperto su un altro clone dello stesso progetto blocca la
  build **anche qui**, perché il mutex è sull'eseguibile del motore.

### 0.2 Il checkout di authoring è il clone principale

Ogni chiamata MCP che **crea, modifica, salva o rinomina** un asset appartiene al clone principale, quello
che ospita il bridge. Due ragioni tecniche, non convenzioni:

* il bridge MCP è **uno solo**: usarlo da un altro checkout muta gli asset del principale mentre leggi il
  `git status` del tuo;
* un worktree **non ha i file gitignorati**: i riferimenti duri di un asset vi leggono `None`, e salvarlo
  **li azzera** — senza errore, e te ne accorgi dopo.

Ispezione e query read-only non hanno questo vincolo.

### 0.3 Registra la baseline prima di modificare

```powershell
# SHA di partenza — serve al report finale e a chi rimisura dopo
git rev-parse HEAD
git status --short

# conteggio dei test PRIMA della modifica: xx = i Result={Success} della run di partenza
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<percorso>\RefactorTactics.uproject" `
  "-ExecCmds=Automation RunTests RefactorTactics.ScreenHud;Quit" `
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Senza questo numero, «il conteggio dei test non è calato» non è verificabile: è un'impressione.

---

## 1. Cosa esiste già — e cosa va guardato due volte

Gli asset dello Screen HUD sono in `Content/RT/UI/Match/`: `WBP_RT_TacticalHUD`, `WBP_RT_TurnHeader`,
`WBP_RT_TeamRoster`, `WBP_RT_SelectedUnitPanel`, `WBP_RT_ActionDock`, `WBP_RT_ActionSlot` — più
`WBP_RT_UnitCard`, `WBP_RT_UnitOverlay` e `WBP_RT_FastDecision`. **Non ricrearli.**

⚠️ **In questo repository i `.uasset` sono versionati.** Verificabile con
`git ls-files Content/RT/UI/Match/`. Ne discende che ogni modifica di debug è un commit binario: non
diffabile, non mergiabile, e visibile a chiunque faccia `git pull`. Il commento in
`Source/RefactorTactics/UI/RTScreenHudWidgets.h` che afferma il contrario è `IMPLEMENTATION DRIFT` ed è
tracciato a parte.

### 1.1 Il sospetto da sciogliere prima di disegnare qualsiasi bordo

🔴 **L'albero di `WBP_RT_TacticalHUD` potrebbe contenere due segnaposto al posto di due widget.** Nel report
di `RefactorTactics.ScreenHud.MatchWidgetsLoad` la colonna della **classe** dice:

```text
=== TacticalHUD: albero (radice = CanvasPanel_99) ===
  Zone_Top                       Border                       Offsets=(L0 T0 R96 B96)
  WBP_RT_TurnHeader              WBP_RT_TurnHeader_C            <- istanza vera
  ZoneRight                      Border                       Offsets=(L-200 T0 R200 B0)
  WBP_RT_SelectedUnitPanelRight  WBP_RT_SelectedUnitPanel_C     <- istanza vera
  ZoneLeft                       Border                       Offsets=(L0 T0 R280 B0)
  WBP_RT_TeamRosterLeft          WBP_RT_TeamRoster_C            <- istanza vera
  ZoneBottom                     Border                       Offsets=(L0 T0 R0 B200)
  ZoneBottomContainer            HorizontalBox
  WBP_RT_SelectedUnitPanel       HorizontalBox                  <- NON è il widget: è un HorizontalBox
  WBP_RT_ActionDock              HorizontalBox                  <- NON è il widget: è un HorizontalBox
```

Un `UUserWidget` composto da un Widget Blueprint ha classe `WBP_..._C`. Due nodi della zona bassa portano il
**nome** dei widget ma la **classe** `HorizontalBox`.

⚠️ **`docs/technical/test-manuali-pie.md` dichiara invece quelle zone «reali e popolate»**, sulla base di
`UMGToolSet.GetWidgetDescription`. Le due evidenze non concordano, e la spiegazione più semplice è che
quella lettura abbia guardato i **nomi** dei nodi e non i tipi — lo stesso errore che farebbe chiunque.

🔑 **Perché viene prima di tutto il resto:** se l'`ActionDock` non è montato, il bordo magenta cadrà su un
contenitore vuoto e non si vedrà. Chi guarda lo screenshot concluderà *«il decorator non funziona»* invece
di *«il dock non c'è»*, e la passata di debug avrà prodotto una diagnosi sbagliata.

**Da fare come primo passo MCP**, prima di aggiungere decorator: leggere l'albero e verificare il **tipo**
dei due nodi. Se sono segnaposto, si ferma qui e si apre la issue: montare un widget non è lavoro di questa
strumentazione.

### 1.2 🔴 `ZoneBottom` è disegnata fuori dallo schermo — misurato

`RefactorTactics.ScreenHud.PanelsLeaveTheCenterFree`, alla sua prima esecuzione (2026-09-09), ha misurato
le quattro zone del Canvas radice a 1920×1080:

```text
Zone_Top     anchors=(0.00,0.00)-(1.00,0.00) align=(0.00,0.00)  ->  X 0..1824      Y 0..96      (1824x96)
ZoneRight    anchors=(1.00,0.00)-(1.00,1.00) align=(0.00,0.00)  ->  X 1720..1920   Y 0..1080    (200x1080)
ZoneLeft     anchors=(0.00,0.00)-(0.00,1.00) align=(0.00,0.00)  ->  X 0..280       Y 0..1080    (280x1080)
ZoneBottom   anchors=(0.00,1.00)-(1.00,1.00) align=(0.00,0.00)  ->  X 0..1920      Y 1080..1280 (1920x200)
```

⛔ **`ZoneBottom` comincia dove lo schermo finisce.** Ancorata al bordo inferiore con `Alignment.Y = 0`,
l'origine resta *sul* bordo invece di risalire della propria altezza: la zona occupa `Y 1080..1280`, cioè
duecento pixel **sotto** la viewport. Tutto ciò che contiene — `ActionDock` e il `SelectedUnitPanel` in
basso — non è visibile in partita. La correzione è `Alignment.Y = 1`, che la riporta a `Y 880..1080`.

🔑 **Questo spiega il sintomo meglio di §1.1, e va tenuto separato da esso.** «Il dock non si vede» ha ora
due cause candidate e indipendenti: i nodi della zona bassa potrebbero essere segnaposto (§1.1) **e** la
zona che li ospita è comunque fuori campo. Correggere l'una senza l'altra non farà comparire nulla.

⚠️ **Nessuna delle due è lavoro di questa strumentazione**: sono difetti di #613 e vanno trattati lì. Ma
disegnare un bordo magenta su una zona fuori schermo produce uno screenshot senza magenta e una diagnosi
sbagliata — per questo la verifica sta prima, non dopo.

⌫ *Nota per chi legge `docs/technical/test-manuali-pie.md`*: la riga che dichiara le quattro zone «reali e
popolate» e il centro libero «per costruzione» è corretta sul **centro** e va corretta sul resto. Il centro
resta libero davvero — ma perché la zona bassa è fuori campo, non perché sia al suo posto.

---

## 2. La soglia — cosa vuol dire «centro libero»

🔴 **Questa è la modifica che vale di più.** La prima stesura costruiva i righelli e non diceva quanto fosse
abbastanza: due persone guardano lo stesso fotogramma e passano entrambe legittimamente.

**Il riquadro che nessuna zona può toccare** è il **60% × 60% centrato** della risoluzione di riferimento
1920×1080:

```text
X 384 .. 1536      Y 216 .. 864       (1152 × 648)
```

Non è un numero sacro: è un numero **scritto**, che si discute in una issue invece che in un playtest.

### 2.1 La metà geometrica non è più affare del PIE

`RefactorTactics.ScreenHud.PanelsLeaveTheCenterFree` (in
`Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp`) misura le zone del Canvas radice con la stessa
formula che `SConstraintCanvas::OnArrangeChildren` applica a runtime, e fallisce nominando la zona che
invade e di quanto.

⛔ **Oggi è ROSSO su `main`, e per un difetto vero**: `ZoneBottom` fuori viewport (§1.2). Non va reso verde
allargando la soglia — va reso verde correggendo l'`Alignment` dell'asset. Il resto della suite
`RefactorTactics.ScreenHud` è verde, quindi il rosso è isolato e attribuito.

🔑 **Il criterio del centro libero, da solo, non bastava** — ed è la lezione che questo test ha imparato
alla prima esecuzione. Una zona fuori schermo *lascia* il centro libero, e passava. Il gate verifica perciò
due cose: la zona non invade il centro, **e** la zona sta dentro lo schermo. Un pannello invisibile non è
un pannello a posto.

| Domanda | Chi risponde |
|---|---|
| Bounding box di ciascuna zona | `PanelsLeaveTheCenterFree` |
| Le zone si sovrappongono? Invadono il centro? | `PanelsLeaveTheCenterFree` |
| Il layout cambia con debug OFF? | `PanelsLeaveTheCenterFree` (stessi rettangoli prima e dopo) |
| Una zona si dimensiona sul contenuto (`AutoSize`)? | `PanelsLeaveTheCenterFree` — è un errore |
| Le proporzioni fra pannelli sono sensate? | 👤 PIE |
| L'informazione è comprensibile? Icone, `—`, placeholder? | 👤 PIE |
| Le due barre (sopra l'unità / nel roster) sembrano un duplicato? | 👤 PIE |
| I pannelli saltano durante planning → resolution? | 👤 PIE |

⚠️ **Il test vede il layout dichiarato nell'asset**, non un fotogramma: un widget spostato a runtime da
Blueprint gli sfugge e resta di PIE.

---

## 3. Cosa deve essere visibile in debug

Quattro macro-bounds più il riferimento del centro. Il colore non è mai l'unico segnale: **ogni zona porta
anche una label testuale**.

| Zona | Widget | Outline | Label |
|---|---|---|---|
| TOP | `WBP_RT_TurnHeader` | ciano | `TOP / TURN HEADER` |
| LEFT | `WBP_RT_TeamRoster` | verde | `LEFT / ROSTER` |
| RIGHT | `WBP_RT_SelectedUnitPanel` | arancio | `RIGHT / SELECTED` |
| BOTTOM | `WBP_RT_ActionDock` | magenta | `BOTTOM / ACTIONS` |
| CENTER | riferimento in `WBP_RT_TacticalHUD` | bianco / grigio | `CENTER FREE` |

🔑 **Il rettangolo CENTER FREE disegna esattamente la soglia di §2**, `X 384..1536 · Y 216..864` a 1920×1080
— non un'approssimazione a occhio. Così smette di essere decorativo e diventa **l'oracolo disegnato**: se
una zona lo tocca, si vede dove.

Nessun riempimento, `HitTestInvisible`, e non deve oscurare path, AoE, facing o world overlay.

### 3.1 Nomina i decorator in modo che un oracolo possa escluderli

Prefisso obbligatorio: **`DebugBounds_`** (`DebugBounds_Top`, `DebugBounds_CenterFree`, …).

⚠️ **Non è cosmesi.** La suite ispeziona già gli alberi dei widget contando per tipo — per esempio
`ActionSlotHasIconSurface` conta le `UImage`. Un decorator costruito con quattro `Image` entra in quel
conteggio. Il prefisso è ciò che permetterà a un test futuro di distinguere strumentazione da contenuto.

---

## 4. Implementazione UMG conservativa

Priorità assoluta: **il debug non cambia il layout di produzione.**

Per ogni macro-zona, un decorator sovrapposto:

* outline-only, padding `0`, nessun contenuto che influenzi la Desired Size;
* `HitTestInvisible`;
* visibilità pilotata da `bShowDebug`;
* spessore 2–3 px a 1920×1080, label piccola ma leggibile.

**Non avvolgere** un widget di produzione in un nuovo parent se questo cambia anchor, desired size,
alignment, padding, z-order o hit testing.

⛔ **Non usare un `Border` con background pieno semitrasparente**: altererebbe proprio la valutazione del
centro e dell'ingombro che la passata deve produrre.

### Vietato

* editare i byte dei `.uasset`;
* ricreare gli asset via script;
* esportare/reimportare per aggirare MCP;
* aggiungere C++ per un problema risolvibile nel WBP;
* modificare `ARTHUD::DrawHUD`;
* migrare il Tactical World Overlay in UMG;
* scrivere «via MCP» per un'operazione fatta a mano.

Se l'MCP non espone una capability necessaria: **fermati su quella operazione**, registra quale tool manca,
non simulare di aver configurato l'asset.

---

## 5. Dove configurarli — e cosa comporta la scelta

Ispeziona prima la gerarchia con MCP. Non assumere la struttura.

* **Decorator nel root `WBP_RT_TacticalHUD`**: le zone sono già lì (`Zone_Top`, `ZoneLeft`, `ZoneRight`,
  `ZoneBottom`), gli anchor pure. Un solo asset da modificare.
* **Decorator nei singoli WBP**: evita di duplicare coordinate, ma moltiplica gli asset toccati.

🔴 **La scelta decide quanti interruttori dovrai accendere, e la prima stesura non lo notava.**
`bShowDebug` è dichiarato su `URTScreenHudWidgetBase`, quindi **ogni** Blueprint derivato ne possiede una
copia indipendente. Decorator nel root ⇒ **un** toggle; decorator nei singoli WBP ⇒ **uno per widget**, e il
DoD «`bShowDebug` resta `false` di default» va verificato su ciascuno.

**Preferisci il root**, salvo che la misura della gerarchia dimostri il contrario. Documenta la scelta nel
report.

---

## 6. Visibility

```text
bShowDebug == false  ->  Collapsed
bShowDebug == true   ->  HitTestInvisible
```

Preferisci `Collapsed` da spento. Se `Collapsed` cambia il layout, **la gerarchia è sbagliata**: si corregge
la struttura, non si ripiega su `Hidden`.

🔑 **Come si verifica**, invece di dichiararlo: `PanelsLeaveTheCenterFree` con debug OFF e con debug ON deve
riportare **gli stessi rettangoli** per le zone di produzione. È l'unico modo non soggettivo di sostenere
«nessun cambiamento layout con debug OFF».

---

## 7. Action slot

**Non** disegnare un bordo per ogni `WBP_RT_ActionSlot` in questa passata: crea rumore e non aiuta a
misurare le macro-zone.

Eccezione: se in PIE emerge che il dock esiste ma i singoli slot non sono distinguibili, si apre una seconda
passata. Non anticiparla.

---

## 8. Passo MCP — configurazione

Prima: **scopri quali tool il server espone davvero**. Non inventare nomi.

Poi, asset per asset:

1. apri e ispeziona `WBP_RT_TacticalHUD`, leggi il Widget Tree;
2. **verifica il tipo dei nodi della zona bassa** (§1.1) — se sono segnaposto, fermati qui;
3. localizza Top / Left / Right / Bottom;
4. scegli la strategia meno invasiva (§5) e annotala;
5. aggiungi i decorator `DebugBounds_*`: colore, label, spessore, visibility, hit-test, z-order;
6. compila ogni Blueprint modificato e correggi i warning introdotti;
7. salva tramite Editor/MCP;
8. `git status --short` — **solo** gli asset previsti devono risultare modificati.

---

## 8-bis. Ordine delle operazioni e ripristino

🔴 **Senza questa sezione il DoD contraddiceva sé stesso.** `bShowDebug` è `EditDefaultsOnly`: si accende
cambiando il **default della classe**, cioè modificando e salvando il Blueprint. Lo screenshot `DEBUG_ON`
esiste quindi solo in uno stato che il DoD vieta (*«`bShowDebug` resta `false` di default»*), e fra i due
stati c'è un `git status` sporco su file binari.

L'ordine è vincolante:

1. decorator aggiunti, `bShowDebug` **ancora `false`** → compila → salva;
2. **screenshot OFF** — nessun bordo, nessuna label, HUD di produzione invariato;
3. `PanelsLeaveTheCenterFree` → registra i rettangoli;
4. accendi `bShowDebug` (uno o più asset, secondo §5) → compila → salva;
5. **screenshot ON** + gli stati dinamici di §10;
6. `PanelsLeaveTheCenterFree` → **gli stessi rettangoli** del punto 3;
7. 🔴 **rispegni `bShowDebug`** → compila → salva;
8. `git status --short` e verifica che l'unico delta rimasto siano i decorator, non lo stato acceso;
9. solo ora commit.

⚠️ **Il punto 7 è quello che si dimentica**, ed è l'unico il cui fallimento arriva in `main`: un HUD con i
bordi magenta accesi per tutti.

---

## 9. Passo MCP — PIE

`L_HexArena` monta lo Screen HUD anche avviando PIE direttamente, senza passare dal frontend.

**Catena già misurata** (`docs/technical/test-manuali-pie.md`) — da riverificare, non da assumere:

```text
SceneTools.load_level → EditorAppToolset.StartPIE → SlateInspectorToolset.PressKey → CaptureEditorImage
```

Tre trappole già pagate da chi è passato prima:

* ⚠️ **`CaptureViewport` non è chiamabile senza argomenti**: `captureTransform` è obbligatorio pur essendo
  descritto come opzionale. `CaptureEditorImage` non ha parametri ed è la via più breve.
* ⚠️ **`CaptureEditorImage` restituisce il PNG in `returnValue.data` (base64)**, non come content di tipo
  `image`. Un client che cerca solo i content `image` conclude «nessuna immagine» su una chiamata riuscita.
* 🔴 **La prima inquadratura non mostra la board**: la camera parte dall'origine. Serve un `Home` prima di
  catturare, altrimenti si fotografa una viewport che sembra un livello rotto.

**Risoluzione.** Il gate parla di 1920×1080 e PIE non ci arriva da solo: usa una finestra Editor con
risoluzione fissata, e **dichiara nel report la risoluzione effettiva della cattura**. Un giudizio
d'ingombro preso a 1280×720 non è confrontabile con la soglia di §2.

**Evidenza.** I PNG vanno in `docs/technical/evidence/` con i nomi `SCREENHUD_DEBUG_OFF_1080P` e
`SCREENHUD_DEBUG_ON_1080P`, e si allegano a #613 con un commento nuovo.
⚠️ Su una issue con un solo account `gh issue comment --edit-last` non discrimina e sovrascrive il commento
sbagliato: usa un commento nuovo.

---

## 10. Stati dinamici da provare

Una schermata all'ingresso non basta.

| Stato | Cosa verificare |
|---|---|
| A — ingresso PIE | `TurnHeader`: Round, Phase, Timer |
| B — primo tick utile | comparsa e popolamento del `TeamRoster` |
| C — unità selezionata | `SelectedUnitPanel`, `ActionDock`, `ActionSlot` |
| D — azione selezionata, path/overlay visibile | il centro resta leggibile; i bordi non coprono l'overlay |

⚠️ **Due falsi allarmi già osservati**: il dock può restare vuoto finché non selezioni un'unità, e il roster
può comparire solo dopo il primo tick. Nessuno dei due significa «asset mancante».
🔑 Ma il caso di §1.1 è diverso e non va confuso con questi: lì il widget **non è nell'albero**.

---

## 11. Le domande che questa passata deve sbloccare

Con debug ON, a colpo d'occhio: quale zona occupa quanto spazio, dove si sovrappongono, quale genera un
difetto. Le domande **metriche** (bounding box, sovrapposizione, invasione) hanno ora una risposta
automatica in §2.1 — qui restano quelle di giudizio:

* il `SelectedUnitPanel` è sproporzionato rispetto al roster?
* il dock è troppo alto o troppo largo?
* qualcosa cambia in modo anomalo selezionando un'unità?
* i pannelli saltano o si ridimensionano durante planning → resolution?

🔴 **La leggibilità si giudica a debug OFF, e sono due passate distinte.** Quattro outline saturi più cinque
label aiutano la geometria e **peggiorano** proprio la schermata in cui devi giudicare comprensione e barre
duplicate. Non chiedere allo stesso screenshot di rispondere a entrambe.

⚠️ Il tema **barre duplicate** non è toccato da questa strumentazione: resta interamente a `PIE-V01-SCREENHUD`.

---

## 12. Test e non regressione

⛔ **`scripts/rt-suite.ps1` non esiste**: la cartella `scripts/` è stata rimossa (`D-346`, `D-347`).
`find . -iname "*.ps1"` non trova nulla in questo repository. La forma reale è:

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<percorso>\RefactorTactics.uproject" `
  "-ExecCmds=Automation RunTests RefactorTactics.ScreenHud;Quit" `
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Poi si contano `Result={Success}` e `Result={Fail}` nel log più recente sotto `Saved/Logs/`.

⚠️ **Nessuna CVar prima di `Automation` in `-ExecCmds`**: fa saltare la coda, e il log troncato sembra verde.

**Filtri.** `RefactorTactics.HUD` non identifica un insieme: nel codice convivono `RefactorTactics.Hud.*`,
`RefactorTactics.HudViewModel.*` e `RefactorTactics.ScreenHud.*`. Dichiara quale hai eseguito.

Non dichiarare verde se: il conteggio dei test scende sotto la baseline di §0.3; un asset non compila; il
debug resta acceso di default; il debug intercetta input; il layout cambia con debug OFF.

---

## 13. Definition of Done

Voci verificabili da una macchina:

* [ ] configurato tramite Unreal MCP reale, nessun `.uasset` editato fuori Editor;
* [ ] decorator prefissati `DebugBounds_`;
* [ ] decorator `HitTestInvisible`;
* [ ] `bShowDebug` torna `false` su tutti gli asset toccati (§8-bis punto 7);
* [ ] `git status --short` mostra solo gli asset previsti;
* [ ] Blueprint compile green;
* [ ] `RefactorTactics.ScreenHud.PanelsLeaveTheCenterFree` verde — ⛔ **oggi non lo è**, e non per colpa dei
      decorator: `ZoneBottom` è fuori viewport (§1.2). Questa casella non si spunta prima che #613 abbia
      corretto l'asset, e **non si spunta allargando la soglia**;
* [ ] stessi rettangoli con debug ON e OFF;
* [ ] suite `RefactorTactics.ScreenHud` ≥ baseline di §0.3.

Voci che **richiedono una firma umana** — nessuna macchina le vede, e non sono auto-dichiarabili da chi ha
configurato i decorator:

* [ ] `HUMAN-SIGNED` — screenshot debug OFF a risoluzione dichiarata: HUD di produzione invariato;
* [ ] `HUMAN-SIGNED` — screenshot debug ON: quattro bordi e quattro label distinguibili;
* [ ] `HUMAN-SIGNED` — `CENTER FREE` visibile e non invaso;
* [ ] `HUMAN-SIGNED` — stato con unità selezionata verificato;
* [ ] `HUMAN-SIGNED` — leggibilità giudicata a debug OFF (§11);
* [ ] evidenza collegata a #613.

🔑 **Chi configura non firma il proprio lavoro** (`CLAUDE.md` §6): la misura vale se avviene **dopo** la
correzione e su un commit dichiarato. Vale anche quando è la stessa persona a fare entrambe le cose.

---

## 14. Report finale

```text
HUD DEBUG BOUNDS — RESULT

Commit di partenza:
Checkout di authoring:
Preflight motore (§0.1):        [libero / occupato da ...]
MCP server e toolset usati:
Tipo dei nodi della zona bassa (§1.1):
Asset modificati:
Strategia UMG (§5) e perché:
Numero di toggle bShowDebug:
TOP / LEFT / RIGHT / BOTTOM / CENTER FREE:
Blueprint compile:
PIE map e risoluzione effettiva:
PIE debug OFF / ON:
PanelsLeaveTheCenterFree:
Suite ScreenHud (baseline -> dopo):
bShowDebug rispento e verificato:
Problemi trovati:
Issue da aggiornare:
Voci HUMAN-SIGNED ancora aperte:
```

Se un passaggio è stato manuale, **dichiaralo**. Non scrivere «via MCP» per ciò che non lo è stato.

---

## 15. Ciclo di vita — quando questi bordi escono

I decorator restano finché #613 è aperta. Alla chiusura del checkpoint, una delle due:

* **rimossi**, con la stessa procedura MCP e un `git status` che lo dimostri;
* **promossi a strumento permanente**, con una decisione dichiarata che ne motivi il costo su asset binari.

⛔ **Non c'è una terza opzione.** «Temporaneo» senza criterio di uscita, su file che non si diffano, è debito
che nessuno vedrà più.
