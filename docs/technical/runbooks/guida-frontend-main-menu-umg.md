# Guida — il Main Menu, e la mappa da cui il pacchetto avvia

> **A chi serve**: a chi apre Unreal per fare il lavoro che una sessione non può fare — i binari sono
> **human-first**, si toccano da un lavoro solo per volta.
> **Checkpoint**: CP 46.3 ([#938](https://github.com/DegrassiAaron/refactor-tactics-main/issues/938)),
> epic [#934](https://github.com/DegrassiAaron/refactor-tactics-main/issues/934).
> **Seguito di** [`guida-frontend-umg.md`](guida-frontend-umg.md), che copre i cinque widget di CP 46.1 e
> CP 46.2. Quella guida diceva di **non** costruire il Main Menu perché non aveva ancora una spec di
> checkpoint: adesso ce l'ha, ed è questa.
> **La spec dice *cosa*** — [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md) —
> **questa guida dice *come***.

---

## 1. Cosa è già fatto in C++

Tutto quello che sta sotto il layout. Non devi scrivere codice: devi costruire ciò che lo legge.

| | Stato |
|---|---|
| `URTMainMenuWidgetBase` — la classe padre del menu | ✅ |
| `ARTFrontendGameMode` — apre il frontend quando la mappa carica | ✅ |
| `URTFrontendNavigator::StartFrontend()` — registra le schermate e apre la radice | ✅ |
| `RTScreenIds::Main` / `::Settings` — i nomi canonici | ✅ |
| I binding schermata → widget in `Config/DefaultGame.ini` | ✅ |
| `WBP_RT_MenuEntry`, `WBP_RT_MainMenu`, `WBP_RT_SettingsPanel` | ❌ **tuoi** |
| La mappa del frontend | ❌ **tua** |
| `GameDefaultMap` in `Config/DefaultEngine.ini` | ❌ **tuo** (§6) |

Otto test automatici coprono il lato C++ (`RefactorTactics.Frontend.*`). Quello che **nessun test può
dire** è se un bordo di focus si vede, ed è la ragione per cui questa guida esiste.

---

## 2. ⚠️ La trappola che costa di più: i nomi

`Config/DefaultGame.ini` dichiara già i due binding, e i percorsi sono **scritti**:

```ini
[/Script/RefactorTactics.RTFrontendNavigator]
+Screens=(ScreenId="Main",WidgetClass="/Game/RT/UI/Framework/WBP_RT_MainMenu.WBP_RT_MainMenu_C")
+Screens=(ScreenId="Settings",WidgetClass="/Game/RT/UI/Framework/WBP_RT_SettingsPanel.WBP_RT_SettingsPanel_C")
```

Se l'asset che crei ha un nome o un percorso anche solo leggermente diverso, **non succede niente di
rumoroso**: la registrazione riesce comunque — un `TSoftClassPtr` è un percorso, e registrarlo non lo
carica — la navigazione risponde `Ok`, e lo schermo resta vuoto. È un fallimento indistinguibile dal
successo, e cercarlo parte dal posto sbagliato perché tutto sembra funzionare.

Quindi: **`WBP_RT_MainMenu` e `WBP_RT_SettingsPanel`, dentro `/Game/RT/UI/Framework/`.** Esattamente così.
Il prefisso è `WBP_RT_`, non `WBP_` (decisione di CP 11.7).

---

## 2-bis. `WBP_RT_MenuEntry` — la voce di menu, e perché serve

🔴 **`FButtonStyle` non ha uno stato «Focused».** Misurato in `SlateTypes.h:508`: gli stati sono
`Normal · Hovered · Pressed · Disabled`, e basta. Con un `UButton` nudo la voce del DoD *«il focus non è
distinguibile dal solo colore»* **non è soddisfacibile** — non esiste uno stile di focus da riempire.

La via che esiste: `UUserWidget` espone `OnAddedToFocusPath` e `OnRemovedFromFocusPath`
(`UserWidget.h:586,595`), e la *focus path* include un widget anche quando il focus è su un suo **figlio**.
Quindi ogni voce è un widget proprio.

**Classe padre**: `UserWidget`.

```text
WBP_RT_MenuEntry
└── Overlay
    ├── Border          ← FocusMarker · Visibility = Collapsed · Is Variable ✔
    └── Button          ← ClickArea · Is Focusable ✔
        └── TextBlock   ← Label · Is Variable ✔
```

| Elemento | |
|---|---|
| Variabile `EntryLabel` (**Text**) | Instance Editable ✔ · Expose on Spawn ✔ |
| `Event Pre Construct` | → `Label` → `Set Text` ← `EntryLabel`. **PreConstruct**, così l'etichetta si vede anche nel Designer |
| Event Dispatcher `OnEntryClicked` | chiamato da `On Clicked (ClickArea)` |
| `On Added To Focus Path` | → `FocusMarker` → `Set Visibility` → `Visible` |
| `On Removed From Focus Path` | → `FocusMarker` → `Set Visibility` → `Collapsed` |
| Funzione `FocusEntry` | → `ClickArea` → `Set Keyboard Focus` |

⚠️ **Il marcatore deve sopravvivere alla scala di grigi**: un bordo, uno spostamento, una freccia. Una
tinta diversa non basta, ed è il motivo per cui questo widget esiste.

⚠️ **`FocusEntry` non è un di più.** Quando un widget entra nel viewport la tastiera resta sul **viewport
di gioco**: finché nessuno ha il focus, `Tab` non ha una posizione da cui muoversi e il menu sembra non
rispondere mentre è tutto collegato. Il Main Menu la chiama da `Event Construct` sulla prima voce.

---

## 3. `WBP_RT_MainMenu`

**Classe padre**: `URTMainMenuWidgetBase`.

### Cosa leggere

| Funzione | Tipo | Uso |
|---|---|---|
| `GetVersionLabel()` | `FText` | la label di versione. Legge `ProjectVersion` da `DefaultGame.ini` — **non** scriverla a mano nel Blueprint |
| `IsSettingsComingSoon()` | `bool` | `true` in v0.1 |
| `GetSettingsNoticeText()` | `FText` | la frase che dichiara il *coming soon* |

### Layout minimo

```text
[ Canvas / VerticalBox centrato ]
  ├── (titolo del gioco)
  ├── Button  PLAY
  ├── Button  SETTINGS
  ├── Button  QUIT
  └── TextBlock  ← binding su GetVersionLabel()
```

### Cosa fanno i tre pulsanti

| | In CP 46.3 |
|---|---|
| **PLAY** | esiste ed è navigabile, ma **non avvia niente**: l'avvio della partita è CP 46.4 ([#939](https://github.com/DegrassiAaron/refactor-tactics-main/issues/939)). Lascialo senza azione e non fingere che funzioni |
| **SETTINGS** | `Get Game Instance → Get Subsystem (RTFrontendNavigator) → Push Screen` con `ScreenId = Settings` |
| **QUIT** | il nodo `Quit Game` |

⚠️ **Il widget non naviga da sé, ma i pulsanti sì.** Non è una contraddizione: la regola di CP 46.1 è che
esista **un solo owner del flow**, e quell'owner è `URTFrontendNavigator`. Un pulsante che gli chiede
`PushScreen` sta usando l'owner; un widget che scegliesse *da solo* fra `PushScreen` e `ReturnMain`
sarebbe il secondo owner. La differenza è chi decide la regola, non chi preme.

### Il focus — è la parte che il DoD verifica davvero

Due requisiti, e nessuno dei due è opzionale:

1. **Navigabile da mouse e tastiera.** Ogni pulsante raggiungibile con Tab/frecce, senza toccare il mouse.
2. **Il focus non è distinguibile dal solo colore.** Serve un **secondo segnale**: un bordo, uno
   spostamento, un'icona, una freccia. Un cambio di tinta e basta non passa.

La seconda regola non nasce qui: vale per gli stati di scenario in §17 della spec e per il linguaggio
icone di E20. La ragione è che un utente su otto non distingue le due tinte che hai scelto, e per lui la
schermata non ha focus affatto.

Metodo di verifica, e non è un'impressione: **fai uno screenshot in scala di grigi.** Se in bianco e nero
non sai dire quale voce è selezionata, il requisito non è soddisfatto.

---

## 4. `WBP_RT_SettingsPanel`

**Classe padre**: `UUserWidget` (non serve una base C++: questo pannello non ha stato proprio).

Contenuto in v0.1: **la frase di `GetSettingsNoticeText()` e un `BACK`**. Nient'altro.

⚠️ **`GetSettingsNoticeText()` e `IsSettingsComingSoon()` sono funzioni *statiche***, quindi si chiamano da
qualunque Blueprint senza derivare da `URTMainMenuWidgetBase` — nella palette compaiono senza bisogno di un
object pin. È deliberato: la voce del menu e il pannello che si apre devono dire la **stessa** frase, e
leggerla dallo stesso punto è l'unico modo di garantirlo. (La prima stesura di questa guida diceva di
derivare da `UUserWidget` mostrando una funzione che allora era d'istanza: non sarebbe stata chiamabile, e
chi costruiva il pannello avrebbe scritto la stringa a mano — cioè ciò che `RTFrontendWidgets.h` vieta.)

⚠️ **Perché una schermata vuota è comunque una schermata.** La tentazione è disabilitare il pulsante
SETTINGS e non costruire il pannello. È il dead-end che il DoD vieta: *«un pulsante che non fa nulla senza
dirlo»*. La voce esiste per due ragioni indipendenti — il back stack deve attraversarla, e il menu non deve
cambiare forma in v0.2 quando il pannello si riempie.

Il `BACK` chiama `PopScreen` sul navigatore. Un test automatico
(`Frontend.MainMenuSettingsIsReachableAndReversible`) verifica già che l'andata e il ritorno funzionino a
livello di stack; a te resta che il pulsante esista e sia raggiungibile da tastiera.

---

## 5. La mappa del frontend

Serve una mappa da cui il gioco avvia. Non è un livello di gioco: è lo sfondo su cui il menu compare.

**Percorso proposto**: `Content/RT/Maps/Shared/L_Frontend/L_Frontend.umap`

- prefisso `L_` (non `L_RT_`), da `convenzioni-contenuti-ue.md` §7;
- `Shared/` perché la mappa non appartiene al vertical slice né a `Dev/`.

📌 **La categoria è una mia proposta, non una decisione presa**: `Maps/` ha `Dev/ · VerticalSlice/ ·
Shared/` e nessuna delle tre è ovviamente giusta per un frontend. Se preferisci un'altra collocazione
decidila **adesso**, prima di creare il `.umap`: rinominare una mappa dopo costa molto più che sceglierla.

**Contenuto minimo**: praticamente niente. Un `DirectionalLight`, uno `SkyLight` e una `CameraActor` se
vuoi uno sfondo; anche una mappa vuota va bene. Non metterci unità, griglia esagonale o `ARTHexMapActor`:
il frontend non è una partita.

**World Settings → GameMode Override = `RTFrontendGameMode`.**

⚠️ Questo passo è **l'aggancio**, ed è l'unico che non ha un test: senza di lui la mappa carica, il menu
non compare, e nessun errore lo dice. `ARTFrontendGameMode::BeginPlay()` è ciò che chiama
`StartFrontend()`. Se ti compare uno sfondo senza menu, controlla qui per primo.

---

## 6. La riga di configurazione che manca di proposito

Perché il **pacchetto** avvii sul menu — che è la voce di DoD che rende `G13` verificabile — serve questa
riga in `Config/DefaultEngine.ini`:

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/RT/Maps/Shared/L_Frontend/L_Frontend.L_Frontend
```

**Non è stata committata, ed è deliberato**: oggi punterebbe a una mappa che non esiste, e un
`GameDefaultMap` rotto è un progetto che non si apre. Scrivila **dopo** aver creato il `.umap`, e adatta il
percorso se hai scelto un'altra categoria al §5.

`EditorStartupMap` resta su `L_DevSandbox`: aprire l'editor sul menu non serve a nessuno.

---

## 7. Verifica

**In PIE**, aprendo la mappa del frontend:

1. Il menu **compare**, e il **cursore si vede**. Se lo sfondo c'è e il menu no, il log lo dice: cerca
   `Schermata 'Main': la classe widget ... non si carica` (nome sbagliato, §2) oppure
   `Frontend non avviato` (GameMode Override mancante, §5). ⚠️ Nessuno dei due casi resta silenzioso:
   se non trovi né l'uno né l'altro, il problema è il layout dentro il `.uasset`.
2. `PLAY · SETTINGS · QUIT` sono percorribili **senza mouse**, e in **scala di grigi** si vede quale è
   selezionata (§3).
3. La label mostra `v0.1.0`. Se è **vuota**, `ProjectVersion` non è stata letta: cercala in
   `Saved/Logs/RefactorTactics.log`, dove compare `ProjectVersion assente in [...] di DefaultGame.ini`.
   ⚠️ La label **non ripiega mai su un numero plausibile**: una versione sbagliata è peggio di una assente,
   quindi il sintomo è uno spazio bianco, non `v1.0.0.0`.
4. `SETTINGS` apre il pannello, il pannello **dice** che è in arrivo, e `BACK` riporta al menu.
5. `QUIT` chiude.

**Sul pacchetto** — ed è la verifica che conta, perché `G13` chiede *«senza editor»*: builda e avvia. Deve
partire dal menu, non da una mappa.

⚠️ **`G13` resta 🟡 anche quando tutto questo funziona.** Le sue due riserve sono **dati** — la mappa
d'autore di U1 e la via a punti mai esercitata — e nessuna si chiude con un menu. Il corpo di #934
sosteneva il contrario ed è stato corretto in code review. Questo checkpoint toglie l'avvio su
`GeneratedTestArena`, che è reale e vale la pena, ma non è il gate.

⏳ **Le voci PIE.** `PIE-V01-FRONTEND-MAIN` non esiste ancora: è
[#1242](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1242), che le scrive tutte e sei. Il
vincolo che le bloccava è decaduto con **D-178**, quindi ora si possono scrivere — ma sono lavoro di
quella issue, non di questa.

---

## 8. Cosa NON costruire adesso

- **`WBP_RT_ResultScreen` e `WBP_RT_PauseMenu`** — sono CP 46.5 (#940) e CP 46.6 (#941), con DoD propri.
- **Il contenuto vero di `SETTINGS`** — è v0.2. Qui il pannello dichiara sé stesso e basta.
- **Uno stile condiviso in `UI/Styles/`** — nasce quando c'è qualcosa da condividere fra più schermate,
  non prima.
- **L'avvio della partita da `PLAY`** — è CP 46.4 (#939).

---

## 9. In caso di dubbio

- **Cosa** deve fare una schermata → [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md)
- **Dove** va un asset → [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md)
- **I cinque widget precedenti** → [`guida-frontend-umg.md`](guida-frontend-umg.md)
- **Perché** E46 esiste → [D-144](../../decisions/RT_PDR_00_Decision_Log.md)
