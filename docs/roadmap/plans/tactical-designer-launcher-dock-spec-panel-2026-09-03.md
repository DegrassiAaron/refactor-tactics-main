# Tactical Designer — il launcher dockato invece che fluttuante · spec panel

> `CURRENT` · **Stato**: revisione **chiusa e consumata** — le sue conclusioni sono in #2168, mergiate ·
> **Data**: 2026-09-03
> **Base di misura**: `9e67d4fd` (`origin/main` dopo `git fetch --prune`). ⚠️ Il checkout locale è a
> `bd45cfb6`, **2 commit indietro**: `git diff --name-only bd45cfb6..origin/main` tocca **16** file, tutti
> di `Turn/`, `Tests/` e `docs/decisions/` sul facing — **nessuno** dei file misurati qui. Le letture su
> `Source/RefactorTacticsEditor/`, `editor-sessions.yaml` e `spec-tactical-designer.md` valgono quindi
> anche a `9e67d4fd`.
> **Oggetto**: la richiesta *«vorrei che il Tactical Designer, all'avvio dell'editor, stesse nel dock a dx,
> nel layout. Non voglio una finestra ma un panel nel layout.»*
> **Letto contro**: `RTDevSandboxLauncherSubsystem.cpp/.h`, `RefactorTacticsEditorModule.cpp`,
> `RTDevSandboxLauncherTests.cpp`, `spec-tactical-designer.md`, `editor-sessions.yaml` (`U31`), le issue
> #1678 / #1680 / #1105, e il sorgente del motore `UE_5.8` (`SLevelEditor.cpp`, `TabManager.cpp`,
> `LayoutExtender.h`, `LevelEditor.h`).
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique

---

## 1. Il verdetto in una riga

La richiesta **è già stata registrata come constatazione il 2026-09-02** — dalle note della seduta `U31`,
che chiudono con *«chi decide che il posto giusto è dockato apra la sua issue»* — quindi non è una scoperta
ma la **decisione** che quella nota aspettava. È implementabile, la via del motore esiste ed è quella che
Content Browser e Output Log usano già. Ma la frase contiene **due** requisiti, non uno, e il secondo
contraddice un contratto deciso e testato: *«all'avvio dell'editor»* letto come *«sempre visibile»* è
esattamente ciò che #1680 — titolata *«e non addosso a chi apre l'editor per altro»* — ha deciso di non
fare, e che cinque test proteggono.

| | Voci |
|---|---:|
| 🔴 Critico | **2** |
| 🟠 Alto | **2** |
| 🟡 Medio | **4** |
| ➕ Trovato misurando, non è della richiesta | **1** |

**Raccomandazione operativa**: una modifica sola — dare al tab una **posizione nel layout** con
`ETabState::ClosedTab`, lasciando intatto il `TryInvokeTab` di `HandleMapOpened`. Il tab smette di essere
una finestra (risolve il requisito vero) senza comparire a chi apre Unreal per altro (non regredisce
#1680), e nessuno dei cinque test esistenti cambia. §7 dice perché `OpenedTab` sarebbe la scelta sbagliata.

---

## 2. Baseline misurata

| # | Fatto | Misura | |
|---|---|---|---|
| 1 | Il tab esiste ed è un nomad | `RegisterNomadTabSpawner(TabId, …)`, [`RTDevSandboxLauncherSubsystem.cpp:67`](../../../Source/RefactorTacticsEditor/Private/RTDevSandboxLauncherSubsystem.cpp) | ✅ |
| 2 | Il display name è `Tactical Designer` | `.SetDisplayName(LOCTEXT("TabTitle", "Tactical Designer"))`, riga 70 | ✅ |
| 3 | Vive sotto *Window → Tools* | `.SetGroup(…GetToolsCategory())`, riga 72 | ✅ |
| 4 | Nessuna posizione nel layout è mai dichiarata | `grep -c "FLayoutExtender\|OnRegisterLayoutExtensions" Source/` → **0** | ✅ |
| 5 | Il modulo editor non estende niente | `RefactorTacticsEditorModule.cpp` è **19 righe**: solo `FRTHexEditorModeCommands::Register()` | ✅ |
| 6 | I test della slice sono **7**, e nessuno vede Slate | `RTDevSandboxLauncherTests.cpp`, sette `IMPLEMENT_SIMPLE_AUTOMATION_TEST` | ✅ |
| 7 | La constatazione «fluttuante» è già scritta | `editor-sessions.yaml`, `U31`, note del 2026-09-02 | ✅ |
| 8 | Il parent è aperto | #1678 `OPEN`; #1680 `CLOSED`; epic #1105 `OPEN` | ✅ |

E dal motore, che è ciò che decide se la richiesta è realizzabile:

| # | Fatto | Misura | |
|---|---|---|---|
| 9 | Il Level Editor accetta estensioni di layout | `LevelEditorModule.OnRegisterLayoutExtensions().Broadcast(LayoutExtender)`, `SLevelEditor.cpp:1815` | ✅ |
| 10 | Le estensioni si applicano **dopo** il caricamento dell'ini | `LoadFromConfig(…)` a `1789`, `Layout->ProcessExtensions(…)` a `1816` | ✅ |
| 11 | La colonna destra è uno splitter con **due** stack | `SLevelEditor.cpp:1766-1782`: Outliner/Layers sopra, Details/WorldSettings sotto | ✅ |
| 12 | Un tab manager figlio sa spawnare un nomad | `FTabManager::FindTabSpawnerFor` → `NomadTabSpawner->Find(TabId)`, `TabManager.cpp:3240` | ✅ |
| 13 | Il layout di default docka già dei nomad come `ClosedTab` | `AddTab("ContentBrowserTab1", ETabState::ClosedTab)`, `AddTab(…OutputLog, ETabState::ClosedTab)` | ✅ |

La riga **13** è la prova che la raccomandazione del §1 funziona: aprire l'Output Log non produce una
finestra, lo docka in basso — ed è dichiarato `ClosedTab`, non `OpenedTab`.

---

## 3. 🔴 C1 — il layout salvato dell'utente vince, e proprio per chi ha il problema

**WIEGERS**: il requisito dice *«all'avvio dell'editor sta a destra»*. Misurato, per una parte degli utenti
sarà falso il giorno dopo il merge, e non c'è modo di accorgersene guardando il codice.

`ProcessExtensions` inserisce un tab esteso **solo se il layout non lo contiene già**:

```cpp
// TabManager.cpp:674, e le altre cinque occorrenze identiche nella stessa funzione
if (!AllTabs.Contains(NewTab.TabId))
{
    Stack->Tabs.Insert(NewTab, InsertedTabIndex++);
}
```

E `AllTabs` è raccolto dal layout **già caricato da `EditorLayout.ini`** (`SLevelEditor.cpp:1789`, prima
del `ProcessExtensions` a `1816`). Chi ha aperto il launcher anche una sola volta ha `RTDevSandboxLauncher`
scritto nel proprio ini nella posizione fluttuante in cui è comparso — quindi `Contains` risponde `true` e
**l'estensione non lo sposta**. Il difetto colpisce esattamente le persone che oggi vedono la finestra:
chi non ha mai aperto il pannello riceve il comportamento nuovo, chi ce l'ha aperto no.

**NYGARD**: e il sintomo è il peggiore che ci sia — *«a me funziona»*. Due macchine, stesso commit, esito
diverso, e la differenza sta in un file che non è nel repository.

📝 **Raccomandazione**: il `done_when` della seduta dichiara **entrambi** i casi e la via d'uscita —
*Window → Load Layout → Default Editor Layout* ripristina il default e fa comparire l'estensione. ⛔ **Non**
bumpare la versione del layout: `LayoutName` è `LevelEditor_Layout_v1.8` ed è del **motore**
(`SLevelEditor.cpp:1702`), non nostra; e comunque resetterebbe l'intero layout dell'utente per spostare un
tab.

---

## 4. 🔴 C2 — «all'avvio dell'editor» è un secondo requisito, e ha già una decisione contraria

**COCKBURN**: chi è l'attore, e qual è il suo obiettivo? Ce ne sono **due**, e la frase li fonde.

*Il designer che apre il progetto per lavorare al Tactical Designer* vuole il pannello lì, pronto. *Chi apre
Unreal per correggere un materiale* non vuole niente. #1680 ha scelto il secondo come vincolo — è nel titolo
della issue, *«e non addosso a chi apre l'editor per altro»* — e il codice lo difende con un commento che
elenca i quattro cancelli del motore a monte del caricamento della mappa d'avvio
(`RTDevSandboxLauncherSubsystem.cpp:88-101`), fra cui `LoadLevelAtStartup != None`, definita lì
*«la via d'uscita nativa di chi apre Unreal per lavorare su altro»*.

Nel caso normale i due requisiti coincidono, perché `EditorStartupMap` **è** `L_DevSandbox`: l'editor si
apre, la mappa di bootstrap si carica, `HandleMapOpened` invoca il tab. Divergono per chi ha impostato
`LoadLevelAtStartup`, per chi passa una mappa da riga di comando, e per chi apre il progetto su un altro
livello.

📝 **Raccomandazione**: leggere *«all'avvio dell'editor»* come *«quando l'editor si apre sul livello di
bootstrap»* — cioè lasciare il gancio dov'è. Se l'intenzione fosse invece *«sempre, su qualsiasi livello»*,
è una **revoca di #1680** e va scritta come tale, non ottenuta di sponda cambiando `ClosedTab` in
`OpenedTab`.

> ✅ **Chiusa dall'autore della richiesta il 2026-09-03**: *«solo su `L_DevSandbox`»*. `ETabState::ClosedTab`,
> `HandleMapOpened` invariato, #1680 non si tocca. Era l'unica domanda di questo referto non chiudibile
> misurando — le altre tre restano verifiche da eseguire, non decisioni da prendere.

---

## 5. 🟠 A1 — «il dock a dx» non nomina un contenitore, e a destra ce ne sono due

**FOWLER**: la colonna destra del Level Editor non è un posto, è uno splitter verticale con **due** stack
(`SLevelEditor.cpp:1766-1782`):

```text
Splitter verticale (SizeCoefficient 0.25)
├── Stack (0.4)   LevelEditorSceneOutliner [Opened] · LevelEditorLayerBrowser [Closed]
└── Stack         LevelEditorSelectionDetails [Opened] · WorldSettings [Closed]
```

Tre destinazioni diverse, tre costi diversi: **in scheda con i Details** (zero spazio nuovo, ma il pannello
copre i Details quando è in primo piano), **in scheda con l'Outliner** (stesso, sopra), oppure **stack
proprio** con `ELayoutExtensionPosition::Below` (spazio dedicato, ma la colonna si divide in tre e i
Details si stringono).

📝 **Raccomandazione**: in scheda con `LevelEditorSelectionDetails`, `ELayoutExtensionPosition::After`. È la
posizione che il layout di default riserva ai pannelli d'authoring, non ruba spazio, e il pannello ha già
un modo di venire in primo piano — `TryInvokeTab` all'apertura di `L_DevSandbox`. ⚠️ **E va detto che si
paga qualcosa**: quando il launcher è in primo piano, i Details non si vedono.

> ✅ **Scelta il 2026-09-03**: in scheda con i Details. Le altre due destinazioni restano scritte perché il
> costo pagato — i Details coperti — sia leggibile fra sei mesi da chi si chiede perché non è uno stack
> proprio.

---

## 6. 🟠 A2 — l'iscrizione non può stare nel subsystem, e allora l'ownership del tab si divide

**FOWLER**: `OnRegisterLayoutExtensions` è un evento **broadcast una volta**, dentro la costruzione del
Level Editor (`SLevelEditor.cpp:1815`). Chi si iscrive dopo non riceve niente. Un `UEditorSubsystem` si
inizializza al caricamento del modulo, e il commento a `RTDevSandboxLauncherSubsystem.cpp:83-86` dichiara la
misura che lo riguarda — *«alle 14:44:32 contro le 14:45:12 della mappa d'avvio»* — ma quella misura riguarda
il **broadcast di startup della mappa**, non la costruzione del Level Editor. ⚠️ **Sono due istanti diversi e
il repository non ha misurato il secondo.**

Il posto sicuro è `FRefactorTacticsEditorModule::StartupModule()`, che oggi ha due righe e nessun handle da
disfare. Ma questo divide l'ownership: il `TabId` si registra nel subsystem, la sua posizione nel modulo —
due file per una cosa sola, ed è il tipo di separazione che nessuno ricorda sei mesi dopo.

📝 **Raccomandazione**: iscrizione nel modulo, e in `RTDevSandboxLauncherSubsystem.h` accanto a `TabId` un
commento che dichiara **dove** vive la posizione. La costante è già documentata come *«la chiave con cui il
layout ne ricorda la visibilità»* (riga 34): la riga esiste, le manca solo il secondo consumatore.

⚠️ **Se invece l'iscrizione dal subsystem funzionasse**, l'ownership resterebbe intera ed è la soluzione
migliore. È una misura, non un'opinione: §8, `V2`.

---

## 7. 🟡 Le altre quattro

**M1 — `OpenedTab` vs `ClosedTab`, e perché la seconda.** `OpenedTab` sembra rispondere meglio a *«all'avvio
sta lì»*, ma apre il pannello su **ogni** livello e per **ogni** utente del progetto: è la regressione del
§4 ottenuta senza dichiararla. `ClosedTab` dà al tab una posizione senza aprirlo, e `TryInvokeTab` lo apre
**in quella posizione** invece che in una finestra — che è il meccanismo con cui Content Browser e Output
Log sono dockati (baseline **13**).

**M2 — «non voglio una finestra» non è ottenibile, e non è ciò che serve.** Un `NomadTab` resta trascinabile
fuori dall'utente, e non esiste flag che lo impedisca. Il requisito realizzabile è *«il default è dockato»*.
Va scritto, o alla prima volta che qualcuno lo stacca si dirà che il lavoro è regredito.

**M3 — la seduta `U31` va estesa, non duplicata.** Il suo `done_when` non nomina la posizione — è la ragione
per cui il ritrovamento è finito nelle `notes` invece che in un criterio. La riga nuova appartiene a `U31`,
che è già `🔄 NON CONCLUSA` con i passi 3, 4 e 5 non eseguiti: aggiungerne una sesta costa nulla, aprire una
seconda seduta sullo stesso avvio costa un secondo giro d'editor.

**M4 — la issue non esiste ancora e il suo numero non si inventa.** Il parent è #1678 (`OPEN`), l'epic
#1105 (`OPEN`). Il numero si legge da `gh` dopo la creazione.

---

## 8. ➕ Cosa **è** testabile senza aprire l'editor — e il repository dice di no troppo presto

**CRISPIN**: l'intestazione di `RTDevSandboxLauncherTests.cpp` dice che *«che il pannello COMPAIA è Slate su
un editor vivo: nessun automation test lo vede»*. Vero per il **pixel**. Falso per il **layout**.

`FTabManager::FLayout` e `FLayoutExtender` sono oggetti puri: si costruisce un layout finto con uno stack
che contiene `LevelEditorSelectionDetails`, gli si applica l'extender del progetto, e si verifica che lo
stack ora contenga anche `RTDevSandboxLauncher`. Nessuna finestra, nessun Slate, nessun editor vivo. E lo
stesso test, alimentato con un layout che **contiene già** il tab, misura il difetto del §3 — cioè
trasforma C1 da rischio scritto in comportamento asserito.

📝 **Raccomandazione**: due test, `LayoutExtensionDocksTheLauncher` e `LayoutExtensionRespectsASavedTab`. Il
secondo è quello che conta: asserisce il limite, non la funzione.

⚠️ **Non coprono che il tab sia visibile a destra**: quello resta occhio umano, ed è `U31`.

---

## 9. Il requisito, riscritto

**WIEGERS**: la richiesta originale in forma verificabile. Tre affermazioni separate, ognuna falsificabile
da sola.

> **R-1 · Posizione.** Il tab `RTDevSandboxLauncher` è dichiarato nel layout del Level Editor nello stack
> che contiene `LevelEditorSelectionDetails`, in posizione `After`, con stato `ETabState::ClosedTab`.
>
> **R-2 · Apparizione.** All'apertura di `L_DevSandbox` il pannello compare **dockato in quello stack** e in
> primo piano. Su ogni altro livello non compare: il comportamento di #1680 non cambia, e i sette test di
> `RTDevSandboxLauncherTests.cpp` restano verdi senza modifiche.
>
> **R-3 · Limite dichiarato.** Per un utente il cui `EditorLayout.ini` contiene già una voce
> `RTDevSandboxLauncher`, R-2 **non** vale finché non ripristina il layout di default. È un limite noto del
> motore, non un difetto: `TabManager.cpp:674`.

**ADZIC**, in esempi:

```text
Given  un utente senza voce `RTDevSandboxLauncher` in `EditorLayout.ini`
When   apre l'editor sul progetto (`EditorStartupMap = L_DevSandbox`)
Then   il pannello `Tactical Designer` e' in scheda con `Details`, in primo piano, e nessuna
       finestra separata compare

Given  lo stesso utente, con il pannello dockato
When   apre `L_HexArena` dal Content Browser
Then   il pannello non passa in primo piano e non ricompare da se'
       (e' il criterio di #1680: la scheda resta dov'e', spenta)

Given  un utente cha ha gia' visto il pannello fluttuante prima di questa modifica
When   apre l'editor
Then   il pannello compare ANCORA fluttuante, finche' non esegue
       `Window -> Load Layout -> Default Editor Layout`
```

Il terzo esempio è il più importante dei tre: è l'unico che oggi nessuno si aspetta.

---

## 10. Verifiche che chiudono le domande aperte

| | Domanda | Misura che la chiude |
|---|---|---|
| `V1` | ~~*«all'avvio dell'editor»* significa **sul livello di bootstrap** o **sempre**?~~ | ✅ **Chiusa il 2026-09-03**: sul livello di bootstrap. §4 |
| `V2` | Il subsystem è inizializzato **prima** del broadcast di `OnRegisterLayoutExtensions`? | Un `UE_LOG` in `Initialize` e uno nell'handler dell'extender, letti in ordine nell'`Output Log` di un avvio. Se sì, §6 decade e l'ownership resta intera |
| `V3` | L'estensione compare davvero a destra e non altrove? | `U31`, criterio nuovo. Occhio umano |
| `V4` | Un layout salvato blocca l'estensione? | `LayoutExtensionRespectsASavedTab`, §8 — automation, non seduta |

---

## 11. Esito

Il lavoro è [#2168](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2168), sotto #1678.
R-1, R-2 e R-3 sono il suo DoD; i test del §8 esistono; il criterio è in `U31`.

➕ **E una cosa che questo referto NON aveva visto, trovata dalla code review della PR.** L'estensione di
layout, da sola, non basta: la posizione vive nel layout del `LevelEditorTabManager`, che è un
**sub-manager**, e `FGlobalTabmanager::Get()->TryInvokeTab` non sa scendere fin lì — `AttemptToOpenTab`
cerca nelle proprie `DockAreas`, e il motore ha un ramo che sale dal sub-manager al globale
(`TabManager.cpp:1778`) e **nessuno** che scenda. Invocando dal globale, il tab si sarebbe aperto in una
finestra *anche con la posizione dichiarata*.

⚠️ **Il §2 aveva misurato la riga 12** — `FindTabSpawnerFor` risale al `NomadTabSpawner` — e l'ha letta come
prova che il global bastasse. Prova che un tab manager sa **costruire** un nomad, non che sappia **trovarne
il posto**: due domande diverse sullo stesso oggetto. Il pattern del motore lo diceva già
(`SOutputLog.cpp:2509-2515` prova il manager specifico e usa il globale come fallback), e questo referto non
l'ha cercato.

## 12. NOT RUN — al momento della revisione

⚠️ Questa sezione descrive lo stato **il 2026-09-03 prima di #2168**, e si legge come storia: nessuna build,
nessuna suite, nessun editor, nessun file di codice toccato *da questa revisione*. Ciò che #2168 ha poi
eseguito sta nella sua PR.

Di `V2`, `V3` e `V4`: `V4` è chiusa dai test, `V2` è decaduta (la guardia sui commandlet e l'iscrizione in
`StartupModule` rendono la domanda irrilevante), **`V3` resta aperta** ed è la seduta `U31`.
