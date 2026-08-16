# Frontend e navigazione — specifica v0.1

> **Owner** dello strato che esiste **prima e dopo** la partita: main menu, transizioni, stati comuni,
> pausa, risultato. Non descrive l'HUD in-match, che ha il proprio owner in
> [`progettazione-hud.md`](progettazione-hud.md).
> **Nasce da** [D-144](../decisions/RT_PDR_00_Decision_Log.md) · epic **E46** in
> [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) · revisione della sorgente in
> [`plans/menu-frontend-spec-panel-2026-08-16.md`](../roadmap/plans/menu-frontend-spec-panel-2026-08-16.md).
> **Stato**: `IMPLEMENTING` *(aggiornato il 2026-08-16)*. Il C++ di **CP 46.1** (navigation controller) e
> **CP 46.2** (fasi di caricamento, esiti d'avvio, classi base dei widget) è in `main` — **32 test**.
> ⏳ Restano i `WBP_RT_*`, che sono `.uasset` e quindi lavoro d'editor: la ricetta per costruirli sta in
> [`guida-frontend-umg.md`](guida-frontend-umg.md).
> *(Questa riga diceva «`SPECIFIED` — regole decise, **nessun codice**», vero fino al 2026-08-16: la
> lascio citata perché uno stato che invecchia in silenzio è il difetto che questo repository misura più
> spesso.)*

---

## 1. Perché esiste, in una riga

Oggi il pacchetto avvia su `MapSource=GeneratedTestArena` via configurazione: è un eseguibile che carica
una mappa, non un gioco che si può iniziare, riavviare o lasciare. Questo documento descrive il minimo che
separa le due cose.

⚠️ **Non è il completamento di `G13`**, e la prima stesura di questa riga lo affermava. Le due riserve di
quel gate sono **dati** — la mappa d'autore (`PIE-V01-ARENA`, seduta **U1**) e la via a punti mai
esercitata — e nessuna delle due si chiude con un menu: `G13` resta 🟡 anche a E46 completa. Questo è
**scope nuovo**, deciso come tale in [D-144](../decisions/RT_PDR_00_Decision_Log.md).

Tutto ciò che eccede quel minimo — Scenario Browser, Bot Simulation, Training, Briefing, Settings
completo — **non è in v0.1**, per la ragione che D-144 registra. *(D-144 ne portava **due**; la seconda —
il catalogo assente dal pacchetto — è caduta il 2026-08-16 con la chiusura di `#926`.)*

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
├── WBP_RT_MatchHistory        ← la lista delle partite registrate (#416)
├── WBP_RT_ReplayViewer        ← il viewer, spinto DA MatchHistory (#472)
└── WBP_RT_ModalLayer          ← sempre in cima, sempre uno solo
```

`WBP_RT_SettingsPanel` esiste in v0.1 come **pannello dichiarato *coming soon***: la voce di menu deve
esistere perché il back stack la attraversi e perché il menu non cambi forma in v0.2. Un pulsante che non
fa nulla *senza dirlo* è un dead-end.

### 2.2 Le due schermate del replay

➕ **Entrano il 2026-08-16**, con la revisione di R6 in
[`../roadmap/plans/replay-r6-spec-panel-2026-08-16.md`](../roadmap/plans/replay-r6-spec-panel-2026-08-16.md)
§5(a). Prima di allora [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472)
avrebbe introdotto due schermate in uno strato che non le prevedeva — cioè un secondo flow accanto a
quello che §3 dichiara unico.

Non sono un caso particolare: sono **due schermate ordinarie**, e la ragione per cui vale la pena
scriverlo è che la loro relazione è l'unica del frontend in cui una schermata ne spinge un'altra
portandosi dietro un dato.

| | `MatchHistory` | `ReplayViewer` |
|---|---|---|
| Raggiunta da | `PushScreen` dal Main Menu | `PushScreen` **da `MatchHistory`**, mai dal Main |
| Porta | l'indice (`URTMatchHistoryLibrary::LoadIndex`) | un `MatchId`, che è il solo dato in ingresso |
| `Back` | torna al Main | torna alla **lista**, non al Main |

⚠️ **`ReplayViewer` non è raggiungibile senza un `MatchId`**, e questo è il motivo per cui non è una voce
di menu: una schermata che si apra senza il suo dato non avrebbe niente da mostrare, e sarebbe il
dead-end che §3.2 vieta. Il percorso di ritorno esiste in entrambi i sensi — `Back` risale alla lista,
`ReturnMain` svuota — quindi l'invariante 2 regge senza eccezioni.

⚠️ **Il viewer non possiede la riproduzione**: posizione, ritmo e abilitazione dei comandi stanno nel
view model di `#472` (`Replay/RTReplayViewModel.h`), che si prova senza widget. È la stessa separazione
di `FRTScreenStack` rispetto ai widget di navigazione — *la logica che governa la UI non è UI* — e vale
qui per la stessa ragione: senza, i criteri di R6 sarebbero verificabili solo a schermo.

⛔ **Nessuna delle due chiama il resolver**, ed è un requisito ereditato, non una raccomandazione:
[ADR-0009](../decisions/adr-0009-replay-logico-canonico.md) §3 fissa il confine `Player`/`Verifier` e
`#472` lo estende al percorso della UI. Si verifica sugli `#include`, come per il Player.

⚠️ **Release**: entrambe sono **v0.1**, allineate a E46 con la decisione registrata nel panel §5(b). Il
core che consumano (`RT-FEAT-REPLAY-ARCHIVE`) è però ancora `v0.2` nel registry: l'incoerenza è
dichiarata lì e non si risolve in questo documento.

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

[`spec-pointer-interaction.md`](spec-pointer-interaction.md) (CP 11.8) **dichiara** sette contesti —
`IdleSelection · Planning · Pathing · Targeting · ResolutionPlayback · ReactionWindow · Modal` — e la
precedenza `Modal/Reaction UI > HUD > world tactical hit`.

⚠️ **Dichiarata, non ancora implementata**, e va detto perché una stesura precedente di questa riga
affermava che fosse *«coperta da dieci test `PlayerInput.*`»*. È falso, misurato:
`grep -rn "HUDConsumesPointerBeforeWorld\|ReactionWindowOwnsInputPriority" Source/` → **zero**. I dieci
test `PlayerInput.*` che esistono in `RTPointerInteractionTests.cpp` coprono altro (bersaglio, facing,
Back, ghost); la precedenza è il **delta (c)** che CP 11.8 esiste per colmare — la nota di quel checkpoint
in [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) lo scrive a lettere: *«non c'è precedenza
HUD → mondo, perché il Canvas HUD non registra hitbox … oggi **ogni** click passa al mondo»*.

Il confine qui sotto **non ne soffre**: regge sulla spec, che esiste, non sui test, che non ci sono. Ma il
frontend non deve dare per implementato ciò che deve ancora arrivare — con i widget UMG il problema si
inverte, ed è la ragione dell'ordine fra CP 11.7 e CP 11.8.

La divisione:

- il **frontend** decide *quale schermata è visibile*;
- il **pointer contract** decide *chi consuma un click in partita*.

Un navigation controller che cominci a decidere precedenza di input duplicherebbe un contratto già
scritto e testato. Quando il frontend apre un modal **in partita** (la pausa), il `PlayerController` entra
nel contesto `Modal` esistente: il frontend lo *segnala*, non lo reimplementa.

---

## 4. Stati comuni

### 4.1 Loading

Il widget mostra la fase corrente **leggendo un dato**, non componendo una stringa:

```cpp
enum class ERTLoadPhase : uint8 { Idle, Map, Scenario, Bots, Ready };
```

`ARTGameMode::SetupHexMatch` la fa avanzare nei punti reali dell'allestimento, e
`URTLoadingScreenWidgetBase::GetPhaseText()` produce la riga. Il testo nasce in C++ e non nel Blueprint,
per la stessa ragione per cui esiste l'enum: due schermate che raccontassero la stessa fase con parole
diverse sarebbero due verità sullo stesso stato.

⚠️ `Idle` e `Ready` **non hanno testo**, ed è voluto: una schermata di caricamento che dice «pronto»
resta a schermo dicendo il contrario di ciò che sta facendo. `IsLoading()` è falso in entrambe.

⚠️ **Nessuna percentuale.** Non esiste un progress model da cui derivarla, e una barra che avanza a caso
è un dato inventato — la stessa regola per cui la UI non ricalcola il risultato.

🔴 **E fino al 2026-08-16 le tre fasi erano lo stesso difetto in forma discreta.** Questa sezione le
elencava come *«messaggi di fase reali»* quando `grep -rn "Loading map\|Initializing scenario\|Preparing
bots\|LoadingPhase" Source/` dava **zero**: nessun enum, nessun evento, niente che un widget potesse
leggere. Tre stringhe scelte a mano sono tre percentuali con un nome — e il divieto due righe sopra le
avrebbe vietate, se qualcuno avesse guardato. Trovato dallo spec panel di
[`../roadmap/plans/cp462-loading-error-spec-panel-2026-08-16.md`](../roadmap/plans/cp462-loading-error-spec-panel-2026-08-16.md);
l'autore ha scelto di **costruire il produttore** invece di togliere le fasi.

⚠️ **Conseguenza sul write-set**: CP 46.2 tocca `RTGameMode`, cioè codice d'avvio e non solo UI. Quel file
non è nel `writable` di nessuna track: va **assegnato prima** di aprire il lavoro (D-139).

### 4.2 Error modal

```text
SCENARIO COULD NOT START

Reason:
<causa leggibile>

[ BACK ]        [ DETAILS ]   ← solo Development
```

- La **causa** è leggibile da un utente, non un codice muto. Un rifiuto senza motivo è un difetto: è la
  stessa regola per cui un `Blocked` silenzioso è un difetto in CP 11.8.
- `DETAILS` e `COPY DEBUG INFO` esistono **solo** in build Development (`#if !UE_BUILD_SHIPPING`, già in
  uso in 12 file). In Shipping resta il messaggio.
- **`BACK` dipende da cosa è stato costruito**, e i due esiti esistono già in `ERTNavResult`:
  - errore **prima** dell'avvio (durante il loading) → `PopScreen`: nulla è stato costruito;
  - errore **a partita già avviata** → `ReturnMain`, che **smonta**. `PopScreen` lascerebbe una partita
    viva sotto il menu, cioè lo stato che CP 46.6 vieta.

### 4.3 Banner di ripiego — il caso che il modale non copre

⚠️ **Il gioco quasi non fallisce: ripiega.** Misurato il 2026-08-16 su `RTGameMode.cpp`: **21 warning
contro 8 errori**, e i due casi principali dell'avvio sono entrambi ripieghi silenziosi —
`MapSource=GeneratedTestArena` (*«uso la mappa di PROVA»*) e `MakeFallbackRules()` (*«uso il RIPIEGO … le
misure di playtest vanno attribuite al formato giusto»*). Sono **le due riserve che tengono `G13` 🟡**, e
un modale d'errore non le intercetta perché non sono errori.

Due forme per due casi:

| Forma | Caso | Esempio |
|---|---|---|
| **Modale** | non si parte | scenario inesistente, mappa corrotta |
| **Banner** persistente | si parte, in condizioni degradate | arena di PROVA · formato di RIPIEGO |

Il banner riusa la forma di `ARTGameMode::GetScenarioBannerText()`, che esiste dal 2026-08-08 per lo
stesso problema e ne porta la motivazione: *«il sintomo non punta alla causa … la spiegazione c'è, ma è in
una riga di Output Log che non si ha motivo di andare a cercare»*.

### 4.4 Il vocabolario: `ERTStartupOutcome`

Nove valori, **misurati** sui punti di uscita reali di `RTGameMode.cpp` e non immaginati:

| | Esiti | Forma |
|---|---|---|
| **Fatali** (3) | `FormatAssetInvalid` · `ShippedFormatInvalid` · `FormatMapMismatch` | **modale** |
| **Degradati** (5) | `UsingTestArena` · `UsingDemoArena` · `LevelMapMissing` · `UsingFallbackFormat` · `NoTurnManager` | **banner** |
| | `Ok` | niente |

`URTStartupReportLibrary::IsFatal()` è **l'unico posto** in cui la divisione è scritta: un widget che la
ridecidesse sarebbe la seconda autorità che questo vocabolario esiste per evitare. Usa uno `switch` senza
`default`, così un decimo esito farà fallire la compilazione invece di diventare un ripiego in silenzio.

**Il motivo è l'enum; la stringa è il suo dettaglio.** `ResolveRules` e `ValidateAgainstMap` producono già
quelle stringhe: qui vengono **trasportate**, non ricomposte — lo stesso rapporto che il TurnLog ha fra un
reason code e i suoi parametri.

⚠️ **Le note sono una lista.** Un avvio accumula più condizioni insieme, e mostrarne una sola nasconderebbe
l'altra: è il modo esatto in cui queste cose sono rimaste invisibili finora — due righe di log separate,
nessuna delle quali qualcuno aveva motivo di cercare.

🔴 **Una correzione da registrare**: una stesura precedente di questa sezione chiamava `UsingFallbackFormat`
la *«seconda riserva di `G13`»*. **Falso**, e l'ha trovato un test rosso. Le due riserve sono l'arena di
test e *«la via a punti non è mai stata esercitata, perché la soglia obiettivo è 0»* — cioè un **valore**
del formato in vigore, non il ripiego del formato. `UsingFallbackFormat` è per giunta un ramo **raro**:
`Format.Skirmish2v2` è spedito da C++ (`9f44570d`), quindi in una build normale non si raggiunge.

∴ di ciò che `G13` dichiara, il banner rende visibile **la prima riserva**, non entrambe. Non le chiude:
restano mancanze di dati. Ma smette di renderla invisibile, che è il motivo per cui il 2026-08-10 non se
n'era accorto nessuno guardando lo schermo.

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
- **Non costruisce una seconda configurazione di partita.** L'avvio usa `ARTGameMode` e
  `Format.Skirmish2v2`, **spedito da C++** dal commit `9f44570d` — *«spedito col gioco, non un asset da
  creare»*. *(Una stesura precedente attribuiva il lavoro a `#375`, che è una PR sul determinismo del
  checksum.)*
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

🔴 **Questa sezione diceva «nessuno dei checkpoint di E46 ha un test automatico possibile: il repository
non ha infrastruttura di test UI», ed era falsa. L'ha falsificata l'implementazione di CP 46.1** (#936,
2026-08-16), che ha prodotto **17 test** `RefactorTactics.Frontend.*`.

Sbagliata in due modi, e il secondo conta più del primo:

1. **L'infrastruttura esiste già.** `RTScreenHudWidgetTests.cpp` prova widget UMG headless costruendo un
   mondo — è di CP 11.7, cioè scritta *prima* che io dichiarassi che non esistesse.
2. **La navigazione non è UI.** È una macchina a stati che *governa* la UI. Separata dalla presentazione
   — `FRTScreenStack` è un `USTRUCT` puro — si prova senza mondo, senza widget e senza asset. La
   previsione nasceva dall'aver dato per scontato che «frontend» ⇒ «widget» ⇒ «non testabile», e i tre
   termini non sono la stessa cosa.

**Cosa resta davvero manuale**: il *layout* dentro il `.uasset` — che una schermata sia leggibile, che il
focus si veda, che il modale copra ciò che deve. È di `PIE-V01-FRONTEND-NAV`, e lo è **per costruzione,
non per rinuncia**.

Il gate `automation` di `RT-FEAT-UI-FRONTEND-SHELL` passa quindi da `todo` a `done` per la parte
consegnata; gli altri restano `todo` finché il loro checkpoint non produce codice.

Le sei voci previste — `PIE-V01-FRONTEND-NAV`, `-ERROR`, `-MAIN`, `-PLAY`, `-RESULT`, `-PAUSE` — **non
esistono ancora**, e non per dimenticanza: [`test-manuali-pie.md`](test-manuali-pie.md) non è di questa
sessione. Dal 2026-08-16 appartiene alla track **`playtest`** — *«l'autore davanti a Unreal»* — secondo
[`parallel-batch.yaml`](../roadmap/parallel-batch.yaml), e D-139 dice che un file non assegnato è uno
**stop**, non una «piccola fix».

⚠️ **La procedura non è aspettare**, ed è la track stessa a dirlo: *«Le altre track producono, questa
giudica. Chi finisce una feature che ha una voce `PIE-*` non scrive il proprio esito qui: lo **propone** in
handoff.»* Le sei voci si propongono quando il primo checkpoint di E46 arriva a qualcosa da guardare —
prima non c'è nulla da verificare, perché E46 è `SPECIFIED` e non c'è codice.

*(La prima stesura di questo paragrafo attribuiva il file a `content_editor` su `#451`: era vero fino al
2026-08-16, quando la track `playtest` è nata proprio per dargli un proprietario stabile.)*

Fino ad allora i DoD di E46 nominano verifiche che il registro non contiene. È un debito, ed è scritto
qui perché non si scopra al momento di chiudere un checkpoint.

**La verifica che chiude G13 è sul pacchetto, non in PIE**: il gate chiede «senza editor».

---

## 9. Cosa questo documento **non** decide

- **CommonUI.** Quattro sorgenti archiviati lo rimandano *«dopo proof of concept»* e nessuno di essi è
  normativo. Resta fuori dalla v0.1 e resta una domanda aperta.
- **Il contenuto di Settings.** In v0.1 il pannello è *coming soon*; le voci (Video, Audio, Controls,
  Gameplay, Accessibility) e la loro persistenza sono v0.2.
- **La UI DEV/TEST.** Scenario Browser, Detail, Runner e Bot Simulation restano fuori dalla v0.1 perché
  sono tooling `out_of_release_scope`. ⚠️ Non più per un blocco tecnico: la prima stesura le faceva
  dipendere da [`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926) (*«`Scenarios/`
  non è staged»*), causa **chiusa il 2026-08-16** da
  [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935). Il catalogo nel pacchetto c'è.
- **La UI di replay**, che è già [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472).
