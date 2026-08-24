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
| L'input di gioco bloccato mentre la pausa è aperta | ✅ `ARTPlayerController::IsGameplayInputBlocked()` |
| Lo smontaggio della partita al ritorno al menu | ✅ `ARTGameMode` consuma e apre il livello |
| `WBP_RT_MenuEntry` — la voce di menu col focus visibile | ✅ da `U28` |
| **`WBP_RT_PauseMenu`** | ⛔ **è questa seduta** |
| La riga `+Screens=(ScreenId="Pause",…)` in `DefaultGame.ini` | ⛔ **è questa seduta** — vedi §2 |

**14 test** `RefactorTactics.Frontend.*` coprono il lato C++. Quello che nessun test può dire è se un bordo
di focus si vede, ed è la ragione per cui questa guida esiste.

---

## 2. ⚠️ La riga di configurazione manca **di proposito**, e va aggiunta qui

`#941` non l'ha scritta, e non per dimenticanza:

```ini
[/Script/RefactorTactics.RTFrontendNavigator]
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
2. **Il focus non è distinguibile dal solo colore** — serve un secondo segnale. `WBP_RT_MenuEntry` lo porta
   già col suo `FocusMarker`: non riscriverlo.

Chiama `FocusEntry` su **RESUME** da `Event Construct`: quando un widget entra nel viewport la tastiera
resta sul viewport di gioco, e finché nessuno ha il focus `Tab` non ha una posizione da cui muoversi — il
menu sembra non rispondere mentre è tutto collegato.

Metodo di verifica: **screenshot in scala di grigi.** Se in bianco e nero non sai dire quale voce è
selezionata, il requisito non è soddisfatto.

---

## 4. 🔴 In PIE `ESC` è anche lo **stop della sessione**, e la precedenza è dell'editor

È la trappola che costa di più in questa seduta, perché il sintomo dice la cosa sbagliata: premi `ESC`, la
sessione si chiude, e concludi che il tasto non è collegato.

Due vie, scegli quella che preferisci:

- **Editor Preferences → Level Editor → Play → «Escape» key stops PIE**: togli la spunta;
- oppure verifica la pausa in una **build packaged**, dove il conflitto non esiste.

⚠️ Non è un difetto del binding, ed è già scritto accanto al `MapKey` in `RTPlayerController.cpp`.

---

## 5. Verifica — cosa guardare, e in che ordine

Con la riga del `.ini` aggiunta e il widget creato, avvia una partita **dal menu** (non `PIE` diretto: quello
è il caso del punto 6).

| # | Cosa | Criterio |
|---|---|---|
| 1 | `ESC` in partita | la pausa compare, con le tre voci leggibili |
| 2 | `Spazio` con la pausa aperta | **non succede niente** — il turno non si chiude. Se si chiude, `IsGameplayInputBlocked()` non sta arrivando all'handler |
| 3 | `SETTINGS` | si apre **lo stesso pannello** del menu principale, e `Back` torna alla pausa |
| 4 | `RESUME` (o un secondo `ESC`) | la partita torna **senza nulla sopra**. ⛔ Se compare il Main Menu, è la regressione di `ResumeDoesNotOpenTheMainMenu` |
| 5 | `RESUME` da dentro `SETTINGS` | torna alla partita, non alla pausa |
| 6 | `RETURN TO MAIN MENU` | si torna al menu, e la partita **non è più sotto** |
| 7 | `PLAY` subito dopo | parte una partita nuova, non un riavvio di quella lasciata |

Il criterio forte del DoD — *«stesso esito dopo il ritorno al menu»* — **non è di questa seduta**: è già
provato headless da `RefactorTactics.Frontend.SameOutcomeAfterReturnToMainMenu`. Qui si verifica che i
pulsanti chiamino quell'API e che a schermo si veda ciò che deve.

---

## 6. Il caso che si dimentica: la partita avviata **senza** passare dal menu

Premi Play direttamente su `L_HexArena` (il workflow `PIE-HEXPLAY-*`) e premi `ESC`.

**Deve funzionare lo stesso**, e finché non c'era `RTScreenIds::Match` non funzionava affatto: la pausa
diventava una radice da cui `RESUME` non usciva più, con l'input bloccato e lo schermo vuoto. Adesso
`ARTGameMode::BeginPlay` dichiara la partita con `EnterMatch()`, e c'è sempre una radice sotto.

⚠️ In quel percorso `RETURN TO MAIN MENU` apre comunque il livello del menu: è corretto: `FrontendLevel` è
in configurazione e non dipende da come la partita è cominciata.

---

## 7. Cosa NON costruire adesso

- **Il contenuto di `SETTINGS`**: in v0.1 è il pannello *coming soon* di CP 46.3, e la pausa apre **quello**.
  Un secondo pannello sarebbe la copia che il DoD vieta.
- **Un `QUIT` nella pausa**: le voci sono tre, e sono quelle del DoD. `QUIT` è nel Main Menu.
- **Un `Surrender` / `Leave Match`**: sono v0.5, hanno un'altra semantica — non sospendono, escono — e la
  differenza va tenuta. Vedi §7 della spec.
- **Qualunque cosa che fermi il tempo**: nessun `Set Game Paused`, nessuna dilatazione. La pausa copre e
  toglie l'input; non sospende. È il vincolo offline-only, ed è verificato da un test e da un grep.

---

## 8. In caso di dubbio

La spec owner è [`spec-frontend-navigazione.md`](../architecture/spec-frontend-navigazione.md) §7. Se questa
guida e quella divergono, **vince la spec** — e la divergenza è un difetto da segnalare, non una scelta.
