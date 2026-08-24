# Guida — il menu di pausa, e l'unico tasto che l'editor ti ruba

> **A chi serve**: a chi apre Unreal per la seduta **U30**. I binari sono **human-first**, si toccano da un
> lavoro solo per volta.
> **Checkpoint**: CP 46.6 ([#941](https://github.com/DegrassiAaron/refactor-tactics-main/issues/941)),
> epic [#934](https://github.com/DegrassiAaron/refactor-tactics-main/issues/934).
> **Seguito di** [`guida-frontend-main-menu-umg.md`](guida-frontend-main-menu-umg.md), che ha prodotto
> `WBP_RT_MenuEntry` — **lo riusi, non lo rifai**.
> **La spec dice *cosa*** — [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md)
> §7 — **questa guida dice *come***.

---

## 1. Cosa è già fatto in C++

| | Stato |
|---|---|
| `URTFrontendNavigator::ShowPause()` / `ResumeMatch()` / `OpenSettings()` / `RequestReturnToMainMenu()` | ✅ |
| `ESC` mappato su `IA_Pause`, con toggle apri/chiudi | ✅ — **nessun asset di input**, si costruisce in C++ |
| `RTScreenIds::Pause` e `RTScreenIds::Match` | ✅ |
| L'input **di piano** bloccato mentre la pausa è aperta | ✅ `ARTPlayerController::IsGameplayInputBlocked()` — vedi il distinguo qui sotto |
| Lo smontaggio della partita al ritorno al menu | ✅ `ARTGameMode` consuma e apre il livello |
| `WBP_RT_MenuEntry` — la voce di menu, **come struttura** | ⚠️ ✅ da `U28`, ma **il marcatore di focus non è finito** — vedi §3 |
| **`WBP_RT_PauseMenu`** | ⛔ **è questa seduta** |
| La riga `+Screens=(ScreenId="Pause",…)` in `DefaultGame.ini` | ⛔ **è questa seduta** — vedi §2 |
| **`PLAY` nel Main Menu → avvia una partita** | ⚠️ **il C++ c'è** — `URTFrontendNavigator::StartMatch()` è `BlueprintCallable` — ma **manca il collegamento d'editor**: vedi §4-bis, e cambia come si verifica |

⚠️ **«Bloccato» non vuol dire tutto.** `IsGameplayInputBlocked()` è consultato da `OnSelect`, `OnLockIn`,
`OnRestart`, `OnUndoWaypoint` e `SelectAbilityForCurrent` (le quattro abilità): quello che tocca **il piano
o la simulazione**. La **camera resta libera** — pan, zoom, rotazione, orbita, recenter, focus e la scala
di riproduzione continuano a rispondere con la pausa aperta. È deliberato: non toccano il piano, e la
precedenza dichiarata da CP 11.8 parla di *chi consuma un click*, non di chi muove la vista. Se in playtest
qualcuno segnala «la camera si muove sotto la pausa», **è il comportamento previsto**, non un difetto.

`RefactorTactics.Frontend.*` conta **74 test** su questo ref, di cui **14** in `RTFrontendPauseTests.cpp`
per CP 46.6. ⚠️ Sono numeri in prosa che nessun gate legge: rimisurali invece di citarli — **uno per
volta**, perché sono due domande diverse e un comando solo risponde alla prima:

```sh
git grep -h -o '"RefactorTactics\.Frontend\.[A-Za-z0-9_]*"' -- Source/RefactorTactics/Tests | sort -u | wc -l   # 74
git grep -h -o '"RefactorTactics\.Frontend\.[A-Za-z0-9_]*"' -- Source/RefactorTactics/Tests/RTFrontendPauseTests.cpp | sort -u | wc -l   # 14
```

Quello che nessun test può dire è se un bordo di focus si vede, ed è la ragione per cui questa guida esiste.

---

## 2. ⚠️ La riga di configurazione manca **di proposito**, e va aggiunta qui

`#941` non l'ha scritta, e non per dimenticanza.

🔴 **Non si aggiunge in fondo: si SOSTITUISCE un blocco che dichiara quella riga assente.** In
`Config/DefaultGame.ini`, subito sotto la voce `Error`, c'è un commento che dice *«⛔ La voce `Pause` di
CP 46.6 NON sta qui, e l'assenza è deliberata»*. Lasciarlo in piedi sopra la riga che lo contraddice
produce un commento auto-invalidante: il prossimo lettore o gli crede — e cancella una riga corretta — o
smette di fidarsi dell'intero blocco. **Cancella quel commento e mettici la riga**, dentro la sezione
`[/Script/RefactorTactics.RTFrontendNavigator]` che già esiste (non riscrivere l'intestazione: ne
nascerebbe una seconda):

```ini
+Screens=(ScreenId="Pause",WidgetClass="/Game/RT/UI/Framework/WBP_RT_PauseMenu.WBP_RT_PauseMenu_C")
```

`RefactorTactics.Frontend.EveryConfiguredScreenLoads` itera **ogni** voce `+Screens=` e pretende che il
widget si costruisca: scritta prima dell'asset, quella riga fa cadere il test. Va aggiunta **insieme** al
`.uasset`, nello stesso commit.

⚠️ **Il nome esatto, dentro `/Game/RT/UI/Framework/`.** Un percorso anche solo leggermente diverso non
produce niente di rumoroso a runtime — la registrazione riesce comunque, perché un `TSoftClassPtr` è un
percorso — e lo schermo resta vuoto. A coglierlo è il test qui sopra, non il log: il warning «non si carica»
è un `Warning`, e in automation un warning non fa fallire una run.

⛔ **`RTScreenIds::Match` NON ha una riga qui, e non deve averla.** È la schermata che rappresenta *la
partita in corso*, e «nessun widget» è la sua definizione: darle un binding metterebbe qualcosa sopra il
gioco a ogni `RESUME`.

---

## 3. `WBP_RT_PauseMenu`

**Classe padre**: `UUserWidget`. Non serve una base C++ — questo widget non ha stato proprio e non legge
nulla: le tre voci *chiamano* e basta.

### Layout minimo

```text
[ Canvas ]
  ├── Border            ← fondo scuro semitrasparente, copre tutto lo schermo
  └── VerticalBox       ← centrato
      ├── TextBlock                "PAUSA"
      ├── WBP_RT_MenuEntry         EntryLabel = "RESUME"
      ├── WBP_RT_MenuEntry         EntryLabel = "SETTINGS"
      └── WBP_RT_MenuEntry         EntryLabel = "RETURN TO MAIN MENU"
```

⚠️ **Il fondo deve coprire, non oscurare del tutto.** La partita sotto resta leggibile: è il contesto che
il giocatore sta interrompendo, e nasconderlo rende la pausa disorientante. Stessa scelta del modale
d'errore, per la stessa ragione.

### Cosa fanno le tre voci

Ogni `WBP_RT_MenuEntry` espone `OnEntryClicked`. Collega ciascuno così:

| Voce | Nodo |
|---|---|
| **RESUME** | `Get Game Instance → Get Subsystem (RTFrontendNavigator) → Resume Match` |
| **SETTINGS** | `… → Open Settings` |
| **RETURN TO MAIN MENU** | `… → Request Return To Main Menu` |

⛔ **Nessuna di queste è `Push Screen` con l'id scritto a mano**, ed è la differenza che il `done_when`
della seduta chiede. Da Blueprint le costanti C++ non sono raggiungibili, quindi un `Push Screen` con
`"Setings"` risponderebbe `Ok` senza disegnare niente e nessun compilatore lo vedrebbe. `OpenSettings()`
esiste esattamente per togliere quella possibilità.

⛔ **E nessuna è `Return Main`.** Quella muove lo stack e **non smonta la partita**: il menu si disegnerebbe
sopra una partita ancora viva, che è lo stato vietato dal DoD. `RequestReturnToMainMenu()` chiede il
cambio di livello, ed è ciò che fa morire il mondo con dentro `TurnManager` e unità.

### Il focus

Valgono i due requisiti del Main Menu, senza sconti:

1. **Navigabile da mouse e tastiera**, ogni voce raggiungibile con Tab/frecce.
2. **Il focus non è distinguibile dal solo colore** — serve un secondo segnale.

🔴 **E oggi quel requisito NON è soddisfatto**, per dichiarazione dell'autore nelle note di `U28`
(`editor-sessions.yaml`): *«il marcatore attuale è un `Border` che compare, e in scala di grigi dipende
tutto dal contrasto di luminanza. Il segnale che non fallisce è il **movimento**: `Set Render Translation`
sulla voce col focus, che nessun trattamento del colore può cancellare. Finché non è fatto, la voce del DoD
non è soddisfatta»*. Misurato su `WBP_RT_MenuEntry.uasset`: `RenderTranslation` compare **zero** volte.

⚠️ *Una stesura precedente di questa guida diceva che `WBP_RT_MenuEntry` «lo porta già col suo
`FocusMarker`: non riscriverlo»* — l'opposto di quanto la seduta che l'ha prodotto aveva dichiarato, e a
tre righe di distanza nello stesso file. Trovato in code review su #1305.

**Quindi**: riusa `WBP_RT_MenuEntry` per la struttura, ma il marcatore va **completato** — è lavoro che
`U28` ha lasciato aperto e che vale per tutte le voci di menu, non solo per la pausa. Aggiungi lo
spostamento su `On Added To Focus Path` / `On Removed From Focus Path`.

Chiama `FocusEntry` su **RESUME** da `Event Construct`: quando un widget entra nel viewport la tastiera
resta sul viewport di gioco, e finché nessuno ha il focus `Tab` non ha una posizione da cui muoversi — il
menu sembra non rispondere mentre è tutto collegato.

Metodo di verifica: **screenshot in scala di grigi.** Se in bianco e nero non sai dire quale voce è
selezionata, il requisito non è soddisfatto — e con il solo `Border` è il caso atteso finché non aggiungi
il movimento.

---

## 4. 🔴 In PIE `ESC` è anche lo **stop della sessione**, e la precedenza è dell'editor

È la trappola che costa di più in questa seduta, perché il sintomo dice la cosa sbagliata: premi `ESC`, la
sessione si chiude, e concludi che il tasto non è collegato.

Due vie, scegli quella che preferisci:

- **Editor Preferences → Level Editor → Play → «Escape» key stops PIE**: togli la spunta;
- oppure verifica la pausa in una **build packaged**, dove il conflitto non esiste.

⚠️ Non è un difetto del binding.

⚠️ **Questa istruzione ora esiste in tre posti** — qui, accanto al `MapKey` in `RTPlayerController.cpp`, e
nelle note di `U30` in `editor-sessions.yaml`. La sede autorevole è **il commento nel codice**, perché
vive accanto alla riga che crea il conflitto: se il nome della preferenza cambia in una versione futura di
Unreal, correggi quello e fai puntare qui.

---

## 4-bis. 🔴 Da dove si parte, perché «dal menu» oggi non si può

**`PLAY` nel Main Menu non è collegato a niente.** Misurato: `StartMatch` compare **zero** volte in
`WBP_RT_MainMenu.uasset`, e le note di `U28` lo dicono — *«Nessun evento su `EntryPlay`, che è CP 46.4 e
non questa seduta»*. Il C++ di CP 46.4 c'è; la sua metà d'editor non è mai stata fatta.

⚠️ *Una stesura precedente di questa guida apriva la verifica con «avvia una partita **dal menu**» e
chiudeva con «`PLAY` subito dopo». Erano due passi impossibili, e in mezzo c'era tutta la checklist.*
Trovato in code review su #1305.

Due vie, e conviene percorrerle **entrambe** perché coprono cose diverse:

| Via | Come | Cosa copre |
|---|---|---|
| **A — partita diretta** | Play su `L_HexArena` | tutto tranne il ritorno al menu «ad anello». È anche il caso di §6 |
| **B — collegare `PLAY`** | in `WBP_RT_MainMenu`, `EntryPlay → OnEntryClicked → … → Start Match` | chiude il buco d'editor di CP 46.4 e rende percorribile l'anello completo |

➕ **La via B è mezz'ora di lavoro e vale la seduta**: senza, il **punto 7** della checklist resta
inosservabile per sempre e il gate `G13` continua a non avere un percorso giocabile end-to-end. ⚠️ **Il
punto 6 invece si osserva anche per la via A** — §6 dice che di lì `RETURN TO MAIN MENU` apre comunque il
livello del menu — ed è il comportamento centrale del DoD di CP 46.6: non saltarlo. Se la
fai, **dichiarala nell'handoff**: è lavoro di CP 46.4, non di CP 46.6.

---

## 5. Verifica — cosa guardare, e in che ordine

| # | Cosa | Criterio |
|---|---|---|
| 1 | `ESC` in partita | la pausa compare, con le tre voci leggibili |
| 2 | `Spazio` con la pausa aperta | **non succede niente** — il turno non si chiude. Se si chiude, `IsGameplayInputBlocked()` non sta arrivando all'handler |
| 2-bis | `WASD` / rotella con la pausa aperta | **la camera si muove**, ed è corretto: vedi il distinguo in §1. Qui si verifica che *non* sia stata bloccata per errore |
| 3 | `SETTINGS` | si apre **lo stesso pannello** del menu principale, e `Back` torna alla pausa |
| 4 | `RESUME` (o un secondo `ESC`) | la partita torna **senza nulla sopra**. ⛔ Se compare il Main Menu, è la regressione di `ResumeDoesNotOpenTheMainMenu` |
| 5 | da dentro `SETTINGS`, premi **`ESC`** | torna alla **partita**, non alla pausa. ⚠️ **Non cercare un pulsante `RESUME`**: con `Settings` in cima il widget della pausa è smontato dal viewport — `SyncPresentation` presenta *«solo la cima, non l'intero stack»* — quindi da lì la sola via è il tasto |
| 6 | `RETURN TO MAIN MENU` | si torna al menu, e la partita **non è più sotto** |
| 7 | `PLAY` subito dopo | parte una partita nuova, non un riavvio di quella lasciata. ⛔ **Solo se hai fatto la via B** di §4-bis |
| 8 | `ESC` a **partita finita**, col Result a schermo | ⚠️ **caso noto e non deciso**: lo stack diventa `[Match, Result, Pause]` e `RESUME` riporta al *Result*, non a una partita. Non c'è una guardia di fase in `OnTogglePause`. Annota cosa vedi: serve a `U29`, che costruisce il Result |

Il criterio forte del DoD — *«stesso esito dopo il ritorno al menu»* — **non è di questa seduta**: è già
provato headless da `RefactorTactics.Frontend.SameOutcomeAfterReturnToMainMenu`. Qui si verifica che i
pulsanti chiamino quell'API e che a schermo si veda ciò che deve.

📋 **Questa checklist è la bozza di `PIE-V01-FRONTEND-PAUSE`**, che non esiste ancora nel registro
(`docs/technical/test-manuali-pie.md`, atto [#1242](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1242)) —
benché due commenti C++ la nominino già come proprietaria di questa verifica. La track playtest vuole che
l'esito si **proponga in handoff**: quando esegui questi **nove** punti, il risultato è ciò che alimenta
quella voce.

---

## 6. Il caso che si dimentica: la partita avviata **senza** passare dal menu

Premi Play direttamente su `L_HexArena` (il workflow `PIE-HEXPLAY-*`) e premi `ESC`.

**Deve funzionare lo stesso**, e finché non c'era `RTScreenIds::Match` non funzionava affatto: la pausa
diventava una radice da cui `RESUME` non usciva più, con l'input bloccato e lo schermo vuoto. Adesso
`ARTGameMode::BeginPlay` dichiara la partita con `EnterMatch()`, e c'è sempre una radice sotto.

⚠️ In quel percorso `RETURN TO MAIN MENU` apre comunque il livello del menu, ed è corretto: `FrontendLevel`
sta in configurazione e non dipende da come la partita è cominciata.

---

## 7. Cosa NON costruire adesso

- **Il contenuto di `SETTINGS`**: in v0.1 è il pannello *coming soon* di CP 46.3, e la pausa apre **quello**.
  Un secondo pannello sarebbe la copia che il DoD vieta.
- **Un `QUIT` nella pausa**: le voci sono tre, e sono quelle del DoD. `QUIT` è nel Main Menu.
- **Un `Surrender` / `Leave Match`**: sono v0.5, hanno un'altra semantica — non sospendono, escono — e la
  differenza va tenuta. Vedi §7 della spec.
- **Qualunque cosa che fermi il tempo**: nessun `Set Game Paused`, nessuna dilatazione. La pausa copre e
  toglie l'input di piano; non sospende. È il vincolo offline-only.

  🔴 **E qui NON c'è rete: sei tu la verifica.** *Una stesura precedente diceva «è verificato da un test e
  da un grep»* — falso per il Blueprint che stai costruendo, e trovato in code review su #1305. Il grep è
  scoped a `-- Source/` e non può vedere dentro un `.uasset`; il test `PauseLeavesNoTraceInTheSimulation`
  chiama il navigatore direttamente e non istanzia alcun widget. Un nodo `Set Game Paused` in
  `WBP_RT_PauseMenu` passerebbe **la suite intera senza un rosso**.

---

## 8. In caso di dubbio

La spec owner è [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md) §7. Se questa
guida e quella divergono, **vince la spec** — e la divergenza è un difetto da segnalare, non una scelta.
