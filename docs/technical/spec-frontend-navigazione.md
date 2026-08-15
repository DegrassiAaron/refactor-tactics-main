# Frontend e navigazione — specifica v0.1

> **Owner** dello strato che esiste **prima e dopo** la partita: main menu, transizioni, stati comuni,
> pausa, risultato. Non descrive l'HUD in-match, che ha il proprio owner in
> [`progettazione-hud.md`](progettazione-hud.md).
> **Nasce da** [D-144](../decisions/RT_PDR_00_Decision_Log.md) · epic **E46** in
> [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) · revisione della sorgente in
> [`plans/menu-frontend-spec-panel-2026-08-16.md`](../roadmap/plans/menu-frontend-spec-panel-2026-08-16.md).
> **Stato**: `SPECIFIED` — regole decise, **nessun codice**. Su `4ab36b48` non esiste alcun widget di
> frontend e `Content/` non contiene alcun `WBP_*`.

---

## 1. Perché esiste, in una riga

Il gate **G13** della v0.1 chiede *«partita giocabile senza editor dalla build packaged»*. Oggi il
pacchetto avvia su `MapSource=GeneratedTestArena` via configurazione: è un eseguibile che carica una
mappa, non un gioco che si può iniziare. Questo documento descrive il minimo che separa le due cose.

Tutto ciò che eccede quel minimo — Scenario Browser, Bot Simulation, Training, Briefing, Settings
completo — **non è in v0.1**, per le due ragioni indipendenti che D-144 registra.

---

## 2. Il confine: `Frontend != In-Match HUD`

Sono due strati con due root, due cicli di vita e due owner documentali.

| | Frontend | In-Match HUD |
|---|---|---|
| Root | `WBP_RT_FrontendRoot` | `WBP_RT_TacticalHUD` (CP 11.7) |
| Vive | fuori dalla partita, e in pausa | dentro la partita |
| Owner | questo documento | [`progettazione-hud.md`](progettazione-hud.md) §4.1 |
| Input | navigazione di schermate | contratto del puntatore (CP 11.8) |

⚠️ **Non esiste un `WBP_GameHUDRoot`.** Il root dell'HUD in-match è già `WBP_RT_TacticalHUD`, deciso a
CP 11.7 sulla owner spec §45. Introdurne un secondo significherebbe due root per lo stesso strato.

⚠️ **Il prefisso è `WBP_RT_`**, non `WBP_`. Su un `.uasset` il rename costa più che scriverlo giusto —
è la ragione già registrata nel panel di CP 11.7.

### 2.1 Gerarchia

```text
WBP_RT_FrontendRoot
├── WBP_RT_MainMenu
├── WBP_RT_ResultScreen
├── WBP_RT_PauseMenu
├── WBP_RT_LoadingScreen
└── WBP_RT_ModalLayer          ← sempre in cima, sempre uno solo
```

`WBP_RT_SettingsPanel` esiste in v0.1 come **pannello dichiarato *coming soon***: la voce di menu deve
esistere perché il back stack la attraversi e perché il menu non cambi forma in v0.2. Un pulsante che non
fa nulla *senza dirlo* è un dead-end.

---

## 3. Navigation controller

Un solo owner del flow. I widget **non** si creano e non si distruggono a vicenda.

### 3.1 Le cinque operazioni

| Operazione | Semantica |
|---|---|
| `PushScreen(S)` | `S` diventa la schermata corrente; la precedente resta nello stack, disattivata |
| `PopScreen()` | torna alla schermata che ha spinto quella corrente. Sulla radice **non fa nulla** |
| `ShowModal(M)` | `M` sopra la schermata corrente, che smette di ricevere input |
| `CloseModal()` | chiude il modal in cima; l'input torna alla schermata sotto |
| `ReturnMain()` | **svuota** lo stack e torna alla radice |

### 3.2 Le invarianti, e come si verificano

1. **Nessun widget crea o rimuove widget.**
   `grep -rn "AddToViewport\|RemoveFromParent" Source/` non produce occorrenze fuori dal controller.
   È un criterio meccanico, non un giudizio di stile.
2. **Nessun dead-end.** Da ogni schermata raggiungibile esiste un percorso verso la radice.
3. **La radice non ha `Back`.** `PopScreen()` sulla radice è un no-op, non un'uscita dal gioco.
4. **Un modal per volta**, e mentre è aperto la schermata sotto non riceve input.
5. **`ReturnMain()` svuota lo stack**: dal menu, `Back` non deve poter rientrare in una partita conclusa.

### 3.3 ⚠️ Il confine con CP 11.8, che è il rischio principale

Il navigation controller **non possiede** il contesto `Modal` del `PlayerController`.

[`spec-pointer-interaction.md`](spec-pointer-interaction.md) (CP 11.8) dichiara già sette contesti —
`IdleSelection · Planning · Pathing · Targeting · ResolutionPlayback · ReactionWindow · Modal` — con la
precedenza `Modal/Reaction UI > HUD > world tactical hit` **coperta da dieci test `PlayerInput.*`**.

La divisione:

- il **frontend** decide *quale schermata è visibile*;
- il **pointer contract** decide *chi consuma un click in partita*.

Un navigation controller che cominci a decidere precedenza di input duplicherebbe un contratto già
scritto e testato. Quando il frontend apre un modal **in partita** (la pausa), il `PlayerController` entra
nel contesto `Modal` esistente: il frontend lo *segnala*, non lo reimplementa.

---

## 4. Stati comuni

### 4.1 Loading

Messaggi di fase reali: `Loading map…`, `Initializing scenario…`, `Preparing bots…`.

⚠️ **Nessuna percentuale.** Non esiste un progress model da cui derivarla, e una barra che avanza a caso
è un dato inventato — la stessa regola per cui la UI non ricalcola il risultato.

### 4.2 Error modal

```text
SCENARIO COULD NOT START

Reason:
<causa leggibile>

[ BACK ]        [ DETAILS ]   ← solo Development
```

- La **causa** è leggibile da un utente, non un codice muto. Un rifiuto senza motivo è un difetto: è la
  stessa regola per cui un `Blocked` silenzioso è un difetto in CP 11.8.
- `DETAILS` e `COPY DEBUG INFO` esistono **solo** in build Development. In Shipping resta il messaggio.
- `BACK` riporta alla schermata precedente **con lo stack intatto**.

---

## 5. Stati visivi

| Elemento | Stati |
|---|---|
| Button | `Normal · Hover · Pressed · Selected · Disabled · Focused` |
| Voce di modo | `Available · Locked · Coming soon` |

⚠️ **Nessuno stato è distinguibile dal solo colore.** Serve sempre un secondo segnale — bordo, offset,
icona, testo. Vale per il focus da tastiera come per `Disabled`, ed è la stessa regola del linguaggio
icone di E20 e degli stati di scenario.

**Navigazione da tastiera**: ogni schermata è percorribile senza mouse, e il focus è sempre visibile.
Il controller (gamepad) **non** è in v0.1.

---

## 6. Ciò che il frontend non fa

- **Non ricalcola il risultato.** Esito, vincitore e round sono letti dal risultato canonico; la
  condizione di fine partita è di E10 e il TurnLog ne è il registro.
- **Non costruisce una seconda configurazione di partita.** L'avvio usa `ARTGameMode` e il formato che
  [`#375`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/375) spedisce da C++
  (`Format.Skirmish2v2`).
- **Non mostra pianificazione avversaria**, in nessuna schermata. La regola non nasce qui: è l'invariante
  di privacy che vale in tutto il progetto.
- **Non introduce un secondo catalogo di scenari.** In v0.1 non ne mostra affatto.

---

## 7. La pausa è offline-only, per costruzione

`ESC` in partita apre `RESUME · SETTINGS · RETURN TO MAIN MENU`.

⚠️ **In multiplayer non esisterà una pausa globale**: la v0.5 (E40, «Il turno simultaneo in rete») non può
fermare il tempo di tutti perché un giocatore ha premuto `ESC`. Ciò che la sostituirà — `Surrender` /
`Leave Match` — ha un'altra semantica: non sospende, esce.

La conseguenza è architetturale e va rispettata **adesso**: la pausa non entra in un contratto condiviso
col futuro codice di rete. Scoprirlo in v0.5 costerebbe un refactor del flow.

**DoD del ritorno al menu**, verificabile invece che dichiarato: si torna al menu, si riavvia una partita
**con lo stesso seed**, e l'esito coincide con quello di una partita avviata da fresco. Se diverge,
qualcosa è sopravvissuto allo smontaggio.

---

## 8. Verifica

⚠️ **Nessuno dei checkpoint di E46 ha oggi un test automatico possibile.** Il repository non ha
infrastruttura di test UI: non esiste una suite che istanzi un widget e ne verifichi la navigazione. Il
gate `automation` delle feature `RT-FEAT-UI-FRONTEND-*` nasce quindi `todo`, e la verifica è **manuale in
PIE** — stesso regime di E21, per la stessa ragione.

Le sei voci previste — `PIE-V01-FRONTEND-NAV`, `-ERROR`, `-MAIN`, `-PLAY`, `-RESULT`, `-PAUSE` — **non
esistono ancora**: [`test-manuali-pie.md`](test-manuali-pie.md) è nel `writable` della track
`content_editor` ([#451](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)) secondo
[`parallel-batch.yaml`](../roadmap/parallel-batch.yaml), e D-139 dice che un file non assegnato è uno
**stop**. Si aprono quando quel file torna `integration_only`, o con una riallocazione dichiarata.

Fino ad allora i DoD di E46 nominano verifiche che il registro non contiene. È un debito, ed è scritto
qui perché non si scopra al momento di chiudere un checkpoint.

**La verifica che chiude G13 è sul pacchetto, non in PIE**: il gate chiede «senza editor».

---

## 9. Cosa questo documento **non** decide

- **CommonUI.** Quattro sorgenti archiviati lo rimandano *«dopo proof of concept»* e nessuno di essi è
  normativo. Resta fuori dalla v0.1 e resta una domanda aperta.
- **Il contenuto di Settings.** In v0.1 il pannello è *coming soon*; le voci (Video, Audio, Controls,
  Gameplay, Accessibility) e la loro persistenza sono v0.2.
- **La UI DEV/TEST.** Scenario Browser, Detail, Runner e Bot Simulation seguono
  [`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926): finché `Scenarios/` non è
  staged nel pacchetto, una UI che legge il catalogo reale non avrebbe catalogo.
- **La UI di replay**, che è già [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472).
