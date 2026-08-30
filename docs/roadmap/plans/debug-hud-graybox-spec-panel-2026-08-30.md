# Debug/Graybox HUD v0.1 — spec panel sul kit «ALL IN ONE» di Claude Cloud

> `CURRENT` · **Stato**: revisione chiusa. Il kit è **consumato e archiviato**, non applicato ·
> **Data**: 2026-08-30
> **HEAD della revisione**: `fff33020` (`origin/main`) — **lo stesso SHA che il kit dichiara di aver
> osservato**, quindi per una volta la fotografia e il repository coincidono e nessuna misura qui sotto
> soffre di deriva temporale.
> **Oggetto**: il file untracked in radice `RefactorTactics_ClaudeCloud_DebugHUD_Graybox_ALL_IN_ONE.md` —
> **1033 righe, 29 874 byte**, consolidamento di quattro sorgenti (`README`,
> `CLAUDE_CLOUD_EXECUTE_DebugHUD_Graybox_v0.1`, `EDITOR_MCP_CHECKLIST_DebugHUD_Graybox`,
> `ACCEPTANCE_AND_TESTS_DebugHUD_Graybox`) — letto contro `Source/`, `Content/RT/UI/Match/`, `Config/`,
> le tre issue che nomina e il piano di `#613`.
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md`](../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md)
> **Non è un re-drop**: misurato contro il master *Visual Slice* archiviato il 2026-08-29 —
> **4 righe condivise su 657** normalizzate. È materiale nuovo, ed è stato revisionato da zero.

---

## 1. Il verdetto in una riga

Il kit è **architetturalmente giusto e operativamente in ritardo**: prescrive con precisione la disciplina
che il repository già rispetta — il HUD è un consumatore, niente autorità in UMG, niente widget parallelo —
e poi ordina di **costruire un asset che esiste da cinque commit**.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **3** |
| 🟡 Medio | **4** |

**Raccomandazione operativa**: **non eseguire il kit come handoff.** Delle quattordici caselle P0 che
dichiara, **dieci sono già verdi su `fff33020` senza toccare nulla**, una è parzialmente vera, e le altre
sono gate d'esecuzione (PIE, automation, packaging) che nessuna lettura statica può spuntare. Il valore
netto del pacchetto è **una riga di lavoro** — la guardia sul timer negativo — che però il repository si era
già scritto da solo nel messaggio di commit `16ba67a4`, sedici giorni prima che il kit arrivasse.

⚠️ **Nessuna suite eseguita, nessuna build, nessun Editor aperto.** Le issue sono lette lato server con `gh`;
`Source/`, `Content/`, `Config/` e `docs/` con `git grep`/`git ls-tree` su `origin/main`. Il contenuto dei
due `.uasset` è misurato per estrazione di stringhe, non aprendoli nell'Editor: dice **quali nomi e quali
binding ci sono**, non come si vedono a schermo. **Nessuna scrittura su GitHub.**

---

## 2. Ciò che il kit ordina, contro ciò che c'è

La colonna «misura» è la prova, non l'impressione.

| Il kit chiede (§) | Stato reale su `fff33020` | Misura |
|---|---|---|
| Creare `WBP_RT_TurnHeader`, derivato da `URTTurnHeaderWidget` (§2, §8, C) | **Esiste**, 70 614 byte | `Content/RT/UI/Match/WBP_RT_TurnHeader.uasset`; il test `RefactorTactics.ScreenHud.MatchWidgetsDeriveFromCppBase` verifica proprio quella parentela |
| Gerarchia `Border → HorizontalBox → Text_Round/Phase/Timer` (§8) | **Esiste**, con altri nomi | stringhe nell'asset: `HorizontalBox`, `HorizontalBoxSlot`, `RoundText`, `PhaseText`, `TimerText` |
| Legare Round al ViewModel (§D) | **Legato** | binding a `GetRoundCounterText` — la funzione C++, non una query Blueprint |
| Legare Phase alla fase sanitizzata (§13) | **Legata** | `Get_PhaseText_Text` + `ERTMatchPhase` nell'asset; nessuna stringa `PLANNING` hard-coded |
| Legare il Timer (§12) | **Legato, ma difettoso** | `Get_TimerText_Text` con `Truncate`; vedi 🔴-2 |
| Creare/ospitare `WBP_RT_TacticalHUD` (§9, Case B/C) | **Esiste e compone già quattro figli** | `WBP_RT_TurnHeader`, `WBP_RT_TeamRosterLeft`, `WBP_RT_SelectedUnitPanelRight`, `WBP_RT_ActionDock` in un `CanvasPanel` con `AnchorData` |
| «Solo `URTFrontendNavigator` può fare `CreateWidget`» (§9) | **Già così** | `PresentMatchHud()`/`DismissMatchHud()`, `MatchHudZ = -100`; chiamato solo da `EnterMatch()`, a sua volta da `ARTGameMode::BeginPlay` (`RTGameMode.cpp:295`) |
| Cablare `Config/DefaultGame.ini` «solo se manca» (§21) | **Già cablato** | `DefaultGame.ini:65` → `MatchHudWidgetClass="/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C"` |
| Usare `rt.HUD.CanvasPanels 0` in validazione (§10) | **Esiste** | `RTHUD.cpp:41`; il commit che introdusse il TurnHeader lo cita come «la duplicazione col Canvas ha un posto dove stare» |
| Riusare `bShowDebug` se esiste (§14) | **Esiste** | `RTScreenHudWidgets.h:54` |
| Ready dal ViewModel o da un adapter minimo (§6, P0.5) | **Nessun dato, nessun comando** | `FRTMatchHeaderView` ha `Round`, `RoundLimit`, `Phase`, `PlanningSecondsRemaining`, `bResolving` — e basta. `SetReady`/`RequestReady`/`CommitPlan`/`ConfirmPlan`: **zero occorrenze** fuori dai test |
| Verificare che UMG/Slate siano in `Build.cs` (§11) | **Il kit ha ragione a dire di non toccarlo** | il piano di `#613` li dava già presenti |

**Cockburn.** L'attore primario di questo handoff è *«chi apre l'Editor e non sa cosa c'è dentro»*. Per
quell'attore il kit è scritto bene: dice di misurare prima. Ma poi **gli dà quattordici sezioni di
costruzione e una riga di verifica**, e il rapporto è rovesciato rispetto alla realtà del repository.

---

## 3. 🔴 Critico

### 🔴-1 · Il P0 è già consegnato, e il kit lo tratta come lavoro da fare

**Wiegers.** Un requisito la cui condizione di accettazione è **già vera prima di iniziare** non è un
requisito: è una verifica travestita. Il §6 «P0 — must deliver» elenca dieci punti; **otto sono osservabili
oggi** senza aprire l'Editor, e i due che restano (centro libero, nessun errore rosso) sono giudizi a
schermo, non consegne.

Il kit **prevede** il caso — §9 «Case A — already implemented: reuse it» — ma lo seppellisce a metà
documento, dopo aver dedicato §7 e §8 alla geometria e alla gerarchia di un widget che non va costruito. Un
esecutore che legga in ordine passa per 350 righe di istruzioni di costruzione prima di incontrare la riga
che le annulla.

**Effetto pratico:** il rischio non è che si perda tempo, è che si **sovrascriva**. Il §C della checklist
d'Editor dice *«Build only primitive hierarchy»* con nomi (`Text_Round`, `Text_Phase`, `Text_Timer`) **diversi
da quelli reali** (`RoundText`, `PhaseText`, `TimerText`). Chi la eseguisse alla lettera creerebbe una
seconda tripletta accanto alla prima, o rifarebbe l'asset perdendo i binding. La checklist ha una guardia —
*«Do not overwrite a richer current asset blindly»* — ma è a §B, ed è una raccomandazione, non un passo.

**Cosa fare invece.** Il primo passo dell'esecutore non è §A ma: aprire `WBP_RT_TurnHeader`, guardarlo, e
fermarsi. Se il kit dovesse essere rieseguito, la sua §0 andrebbe riscritta come *«questo asset esiste:
la tua consegna è il §12 e nient'altro»*.

### 🔴-2 · Il difetto vivo del timer c'è, ma il kit non sa di averlo trovato

Questo è il punto in cui il kit vale qualcosa, e il modo in cui lo dice ne dimezza il valore.

Il §12 chiede: *«no context / invalid timer displays a neutral placeholder, e.g. `--:--`»*. È esattamente
il difetto aperto del repository, scritto nel corpo del commit `16ba67a4`:

> ⚠️ Manca ancora la guardia sul negativo: fuori dalla fase di planning il campo vale -1, e a schermo
> comparira' un numero negativo. Serve un `Select` su `< 0` che mostri «—», come fa il contatore di
> round per il caso «nessun contesto».

Il kit lo formula come **requisito nuovo** invece che come **bug noto con una fix già progettata**, e
scrivendolo da zero introduce due divergenze:

1. **Il placeholder diverge.** Il repository ha già scelto `—` per «nessun contesto» — `GetRoundCounterText`
   lo restituisce, e il test a `RTScreenHudWidgetTests.cpp:68` lo pinna. Il kit propone `--:--`. Due glifi per
   la stessa idea nello stesso widget: è il difetto che il kit stesso denuncia altrove quando dice
   «usa la terminologia del repository».
2. **La condizione diverge.** Il repository distingue `PlanningSecondsRemaining < 0` («la domanda non si
   applica») da `== 0` («scaduto adesso») — la distinzione è documentata nella `UPROPERTY` a
   `RTHudViewModel.h:44-49`. Il kit dice solo «no context / invalid», che non nomina quel confine e lo
   lascia decidere a chi implementa.

**Nygard.** Il valore di un handoff sui modi di fallimento sta nel dire *quale* stato produce *quale*
schermata. Qui lo stato di fallimento è già tipizzato nel view model, con un commento che spiega perché:
il kit avrebbe dovuto citarlo e vincolarlo, non riderivarlo.

### 🔴-3 · `MM:SS` è un cambio di formato mascherato da requisito

Il §12 chiede *«timer shows `MM:SS` or the current repository-approved format»*. L'implementazione attuale
mostra **secondi interi troncati**, per una decisione presa e committata (`16ba67a4`). Non c'è
`%02d:%02d` in tutto `Source/RefactorTactics`, e non c'è nessun helper di formattazione: la conversione è un
`Conv_NumericPropertyToText` dentro il Blueprint.

L'alternativa «or the current repository-approved format» rende il requisito **non falsificabile**: due
esecutori possono soddisfarlo con due schermate diverse. E il kit applica la deferenza al repository per il
lessico — §7, *«se il testo canonico è `TURN` e non `ROUND`, usa la terminologia corrente»* — ma **non** per
il formato numerico, dove servirebbe di più, perché lì una divergenza è invisibile in review e visibile solo
a schermo.

**Adzic.** Manca l'esempio che decide. Tre righe avrebbero chiuso la questione:

```gherkin
Dato   Planning, PlanningSecondsRemaining = 21.4
Allora TimerText mostra "21"        # oppure "00:21" — ma UNA delle due, scritta qui
Dato   Resolution, PlanningSecondsRemaining = -1
Allora TimerText mostra "—"         # lo stesso glifo del contatore di round
```

Il kit contiene due diagrammi ASCII del risultato atteso (§1) che mostrano `00:27` e `00:18` — cioè
**decidono `MM:SS`** — mentre la prosa del §12 lascia la scelta aperta. Il documento si contraddice fra la
figura e il testo, e la figura è la parte che verrà guardata.

---

## 4. 🟠 Alto

### 🟠-1 · Il piano di `#613` dichiara `0 / 61`, e il kit lo manda a leggere

Il kit ordina (§4) di leggere [`screen-hud-umg-2026-08-26.md`](screen-hud-umg-2026-08-26.md) e (§8) di
*«seguire il runbook»*. Quel piano ha **61 caselle e zero spuntate**.

Il lavoro invece è chiaramente avanzato: sei asset in `Content/RT/UI/Match/`, il layer HUD nel Navigator, il
binding in `DefaultGame.ini`, tre suite di test dedicate. Il commit `5682345a` lo dice esplicitamente —
*«0 caselle su 61 sono spuntate nel piano, siamo intorno al Task 2»* — e da allora ne sono atterrati almeno
altri quattro senza che il registro si muovesse.

**Wiegers.** Un piano che non registra il proprio avanzamento smette di essere un piano e diventa una
trappola: chi lo apre in buona fede conclude che non è stato fatto nulla, ed è **esattamente la conclusione
che questo kit ha tratto**. La causa a monte del 🔴-1 non è il kit, è il registro.

**L'azione che il kit avrebbe dovuto ordinare, e che non ordina:** riconciliare le 61 caselle con
`origin/main` prima di qualunque lavoro d'Editor. Vale più dell'intero P0 del pacchetto.
✅ **Eseguita il 2026-08-30**, subito dopo questo referto: la sezione «Riconciliazione del registro» in
testa a [`screen-hud-umg-2026-08-26.md`](screen-hud-umg-2026-08-26.md) misura le 61 caselle su `285d2322` e
ne spunta **36**. Ha anche trovato ciò che nessuno dei due documenti sapeva: lo **Step 7.4 non era
implementato** — `WBP_RT_ActionDock` non chiamava `GetArmedActionIndex()`, e `bArmed` era una **costante
`false`**, quindi nessuno slot poteva accendersi mai — e lo **Step 3.4 è per metà**, che è la stessa lacuna
del 🔴-2 qui sopra vista dal lato del piano. ✅ **Lo Step 7.4 è stato chiuso lo stesso giorno** (37/61); lo
Step 3.4 resta. Le cinque caselle «verifica a schermo» restano aperte per costruzione.

### 🟠-2 · Ready: sei gate per una funzione senza dato e senza comando

Il kit dedica a Ready il §6 (P0.5), metà del §16, il §19 «Ready acceptance» e sei gate `R01`–`R06`. La
misura:

- **Nessun campo Ready** in `FRTMatchHeaderView` — la vista ha cinque campi, elencati al §2 qui sopra.
- **Nessuna API di comando**: `SetReady`, `MarkReady`, `RequestReady`, `ConfirmPlan`, `CommitPlan` danno zero
  occorrenze in `Source/RefactorTactics` fuori dai test. Le uniche `bReady` che esistono sono locali e di
  altro dominio: `RTPlayerController.cpp:823` (l'abilità è utilizzabile) e `RTHUD.cpp:624` (l'energia è piena).

Il kit **si auto-blocca correttamente** — *«If no correct Ready command path exists: do not make the Button
call `ARTTurnManager` directly… record the exact blocker»* — e questa è la sua sezione migliore: nomina in
anticipo la scorciatoia sbagliata, e la vieta. Il rilievo non è che sbagli, è che **paghi per intero un
ramo già determinato**: la condizione è decidibile con due `git grep`, e il kit la lascia aperta per quattro
sezioni.

**Esito da mettere a verbale:** Ready **omesso**, per assenza sia del dato sanitizzato sia del path di
comando. Non è un difetto del HUD ed è **fuori** da `#613`: appartiene a `#77` (CP 11.1 — HUD di partita
completo, `OPEN`) o a un owner del planning. Nessuna issue nuova aperta da questa revisione.

### 🟠-3 · Il §20 dichiara fuori scope tre widget che il root già ospita

«Do not implement now: Team Roster; Selected Unit panel; Action Dock; Action Slots…» — ma
`WBP_RT_TacticalHUD` **li contiene già tutti e tre**, con nomi di slot che dicono anche dove stanno:
`WBP_RT_TeamRosterLeft`, `WBP_RT_SelectedUnitPanelRight`, `WBP_RT_ActionDock`. Esistono pure gli asset
`WBP_RT_ActionSlot` e `WBP_RT_UnitCard`.

**Fowler.** «Fuori scope» e «non deve esistere» sono due affermazioni diverse, e un documento operativo che
le confonde autorizza una rimozione. Il §20 chiude con *«Do not "helpfully" widen the pass»*: letto insieme
alla lista, un esecutore zelante potrebbe **restringere** il root per farlo combaciare con lo scope,
distruggendo lavoro atterrato. La formulazione corretta è *«non lavorarci in questo passaggio»*, mai
*«non implementare»*, quando l'oggetto è già in `main`.

---

## 5. 🟡 Medio

| # | Rilievo, con la misura che lo regge |
|---|---|
| 🟡-1 | **Il §2 vieta un widget che nessuno ha creato.** «Do not create `WBP_RT_DebugPlanningHeader`» è la sezione più forte del kit — l'idea che il graybox debba essere *il widget vero, stilizzato male* invece che un doppione usa-e-getta è giusta e vale oltre questo passaggio. È però una correzione a un brainstorming precedente, non un'istruzione: `git ls-tree` non trova nessun `DebugPlanningHeader` in `Content/`. La regola resta buona; il pericolo che sventa non esiste |
| 🟡-2 | **`5.8.1` contro `5.8`.** Il kit dichiara «Engine expected: Unreal Engine **5.8.1**»; `RefactorTactics.uproject` dichiara `"EngineAssociation": "5.8"`. Non è un conflitto — il piano di `#613` scrive anch'esso `UE 5.8.1` — ma la patch non è verificabile dal repository, e un handoff che la afferma come dato di preflight offre una precisione che non possiede |
| 🟡-3 | **`L_DevSandbox` come mappa di validazione è plausibile ma non dimostrato.** Quella mappa è `EditorStartupMap` (`DefaultEngine.ini`), e `GlobalDefaultGameMode` è `BP_GameMode`, la cui base chiama `EnterMatch()` in `BeginPlay` (`RTGameMode.cpp:291-295`) → il HUD si presenta. **Ma dipende da un eventuale override di GameMode sulla mappa**, che sta dentro il `.umap` e questa revisione non l'ha aperto. Il kit lo dà per scontato in §16 e §H senza il passo «verifica che il HUD si presenti affatto», che è il primo che fallirebbe |
| 🟡-4 | **I gate `G12`/`G13` esistono già come test, e il kit non li nomina.** Chiede «Blueprint compila e salva» e «automation rilevanti verdi» in prosa, mentre il repository ha tre suite mirate: `RTMatchWidgetAssetTests.cpp` (carica ogni `.uasset` di Match e ne verifica la classe base), `RTScreenHudWidgetTests.cpp`, `RTFrontendMatchHudTests.cpp`. Un handoff che ne citasse i nomi darebbe all'esecutore un comando invece di un criterio |

---

## 6. Le caselle P0 del kit, ricalcolate su `fff33020`

Sono le stesse del suo §19, con la colonna che il kit non poteva scrivere.

| Casella (§19) | Esito | Perché |
|---|---|---|
| `WBP_RT_TurnHeader` canonico esiste | ✅ **già** | asset + test di parentela |
| Solo visuali graybox primitive | ✅ **già** | `TextBlock` in `HorizontalBox`, nessuna texture nell'asset |
| Round reale da stato sanitizzato | ✅ **già** | binding a `GetRoundCounterText` |
| Fase reale da stato sanitizzato | ✅ **già** | `Get_PhaseText_Text` su `ERTMatchPhase` |
| Timer reale da stato sanitizzato | ⚠️ **parziale** | legato, ma negativo fuori Planning → 🔴-2 |
| Nessuna autorità di gioco in UMG | ✅ **già** | i binding leggono il view model; nessun `Get All Actors Of Class` nell'asset |
| Nessuna dipendenza diretta da texture | ✅ **già** | nel TurnHeader; l'`IconCatalog` sta su `URTTacticalHUDWidget`, non qui |
| Il centro tattico resta libero | ❓ **a occhio** | `AnchorData` presente, ma il giudizio è a schermo |
| I pannelli Canvas legacy si spengono | ✅ **già** | `rt.HUD.CanvasPanels` |
| Il world overlay resta | ✅ **già** | il cvar spegne solo i pannelli **screen-space** (`RTHUD.cpp:894-995`) |
| Blueprint compila e salva | ✅ **presumibile** | i test caricano le `WidgetBlueprintGeneratedClass`; non rieseguiti qui |
| Automation verdi | ⛔ **non eseguito** | nessuna suite lanciata in questa revisione |
| PIE in `L_DevSandbox` | ⛔ **non eseguito** | nessun Editor aperto |
| Nessun errore rosso a runtime | ⛔ **non eseguito** | idem |
| Packaged Development | ⛔ **non eseguito** | idem — e il kit ha ragione a dire di **non** spuntarlo senza averlo fatto |

**Dieci già verdi, una parziale, una a occhio, quattro non eseguibili staticamente.**

---

## 7. Il residuo che vale

Tolto ciò che è già a terra, del kit sopravvivono **tre cose**, in ordine di valore:

1. **La guardia sul timer negativo** (§12) — un `Select` su `PlanningSecondsRemaining < 0` che mostri `—`,
   lo **stesso** glifo del contatore di round. È lavoro d'Editor su `WBP_RT_TurnHeader`, nessun C++, e chiude
   un difetto che oggi mette un numero negativo davanti al giocatore fuori dal Planning.
   **Owner: `#613`.** Non eseguito qui: è `.uasset`, e questa sessione non ha aperto l'Editor.
2. **Il principio del §2** — *il graybox è il widget canonico stilizzato male, mai un doppione usa-e-getta*.
   Vale oltre il HUD, e vale scritto: è la ragione per cui oggi non esiste un `WBP_RT_DebugPlanningHeader`
   da smontare.
3. **La forma del §6/P0.5** — enumerare in anticipo le scorciatoie vietate (*non chiamare `ARTTurnManager`
   dal Button, non inventare un `bReady` locale che mente*) invece di vietarle a posteriori in review. È il
   pezzo che un handoff futuro dovrebbe copiare.

Non sopravvivono: §7 (geometria), §8 (gerarchia), §9 (integrazione root), §21 (file da cambiare), §22 (git) —
tutti superati dallo stato del repository.

---

## 8. Cosa questa revisione **non** ha fatto

**Crispin.** Il confine di ciò che è stato verificato è parte del referto, non una nota a piè di pagina.

- ⛔ **Nessuna suite eseguita**, nessuna build, nessun packaging.
- ⛔ **Nessun Editor aperto**: i due `.uasset` sono misurati per estrazione di stringhe. So *quali* nomi e
  *quali* binding contengono; **non** so come si vedono a 1920×1080, né se il centro è davvero libero.
- 🔄 **Le caselle del piano di `#613`**: quando questo referto è stato scritto la riconciliazione era solo
  raccomandata. È stata **eseguita lo stesso giorno** — vedi 🟠-1 — con lo stesso confine di misura: i
  `.uasset` letti per estrazione di stringhe, nessun Editor aperto.
- ⛔ **Nessuna scrittura su GitHub**: nessuna issue aperta, chiusa o commentata. `#613`, `#77` e `#705` sono
  state solo **lette**, e risultano tutte e tre `OPEN` — per una volta il kit non manda a lavorare su issue
  già chiuse, che è il difetto ricorrente di questa famiglia di pacchetti.
- ⛔ **Nessun file di `Source/` o `Content/` toccato.** Questo consumo scrive solo `docs/`.

---

## 9. Provenienza

Il sorgente è archiviato verbatim in
[`../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md`](../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md)
con banner `📸 HISTORICAL`, e rimosso dalla radice del repository, dove stava come file untracked e faceva
da falsa autorità accanto al piano di `#613` — che è l'owner vero.
