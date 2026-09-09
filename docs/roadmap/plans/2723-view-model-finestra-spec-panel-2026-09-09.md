# `#2723` — Il view model della finestra di reazione · spec panel 2026-09-09

**Issue**: [#2723](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2723) · **Epic**: #152 (E14) · **CP** 14.6 (#166)
**Governata da**: [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md) · **ADR**: `adr-0004-finestre-di-reazione.md` §7-bis
**Misurato su**: `e5a82900` · **Consegnato su**: `944c9ab0`

---

## Il fatto, e perché una riga spegneva tre issue

Tre issue avevano costruito la finestra di reazione fino al confine della UI — [#2679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2679) l'Overwatch, [#2692](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2692) il `Brace`, [#2717](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2717) l'orologio. Nessuna era raggiungibile in partita.

Il ramo che apre la finestra è protetto da quattro condizioni, e la prima è `OnReactionWindowOpened.IsBound()`. Misurato su `e5a82900`: **zero binding di produzione** — le sole tre occorrenze stavano nei test che lo legavano apposta. In partita quel ramo non si prendeva mai, l'esito restava `NoDecider`, la risposta sicura veniva applicata d'ufficio, e il giocatore non sapeva che una scelta esisteva.

⚠️ **È la stessa forma di difetto che `#1467` aveva lasciato aperta per il velo**: un meccanismo coperto al 100% e mai invocato è indistinguibile, dalla suite, da uno che funziona.

---

## Le due decisioni che il panel ha cambiato rispetto al disegno della issue

### 🔴 F1 — Nessun ripiego senza proprietario

`ARTGameMode::GetKnowledgeVeilPresenter()` ha un ramo dichiarato: *«senza client — harness headless, test di simulazione — il presenter nasce qui»*. È corretto per il velo, perché la board è nel mondo e va velata comunque.

⛔ **Copiarlo per la finestra sarebbe stato l'opposto di corretto.** Il delegate legato in ogni harness che monta un GameMode aprirebbe finestre dove nessuno disegna e nessuno risponde, e dove il `Tick` del manager può non girare mai: la prima finestra sospenderebbe la resolution senza che nessuno la riprenda.

🔑 **`IsBound()` non è un dettaglio implementativo: è il segnale che distingue «umano con UI» da «umano senza UI»**, ed è ciò che tiene bot, test e Verifier sul modello sincrono senza un ramo che li nomini. Un ripiego gli farebbe rispondere «c'è una UI» proprio quando non c'è.

∴ `ARTGameMode::HookReactionWindow()` non lega niente senza `ARTPlayerController`. Sorvegliato da `Reactions.ViewModel.NoPlayerControllerLeavesTheDelegateUnbound`.

### 🔴 F2 — Push per *esistere*, pull per *disegnare* — e vale anche per aperto/chiuso

La issue applicava la regola al **tempo**: *«il countdown a schermo si legge dal manager, non si conta nel widget»*. Il panel ha trovato che vale identicamente all'**esistenza**, e che la issue non lo nominava.

**Non esiste un `OnReactionWindowClosed`.** Il manager notifica l'apertura e nient'altro, mentre la finestra si chiude **anche per scadenza** — `TickReactionWindow` → `ExpireReactionWindow`, un percorso che non passa dal view model. Un `bOpen` proprio sarebbe rimasto vero per sempre dopo il primo timeout, e il widget avrebbe disegnato una finestra che il core aveva già chiuso.

∴ la vista in cache vale **solo finché** `GetOpenReactionWindowId()` nomina ancora la finestra per cui è stata costruita. ✅ **Nessuna API nuova richiesta**: quell'accessore copre già entrambi i siti dal 2026-09-09.

---

## Cosa è stato costruito

| | |
|---|---|
| `URTReactionWindowViewModel` | `UObject` per-client, sul modello di `URTKnowledgeVeilPresenter` |
| proprietà | `ARTPlayerController::GetReactionWindowViewModel()` |
| cablaggio | `ARTGameMode::HookReactionWindow()`, accanto a `HookKnowledgeVeil()` |
| **unica API nuova sul manager** | `GetOpenReactionWindowRemainingSeconds()` |

`GetOpenReactionWindowRemainingSeconds()` risponde **negativo** quando nessuna finestra attende — la convenzione già motivata da `FRTMatchHeaderView::PlanningSecondsRemaining`, dove *«un `0.f` direbbe "scaduto adesso", che è un'altra cosa»* — ed è **clampato a `0`** verso il basso. ⚠️ Il clamp non è difensivo: `TickReactionWindow` somma il `DeltaSeconds` **prima** di confrontarlo, quindi fra l'incremento e `ExpireReactionWindow` il residuo è già sotto zero.

⛔ **Il view model non decide**: la scadenza è dell'orologio (#2717), la legalità della risposta è di `AskReactionDecision`, la durata è server-authoritative. Inoltra.

---

## La privacy per squadra non è verificata nel view model, e la riga ha una data di scadenza

Due ragioni misurate, non una fiducia:

* il gate a monte è `!bOwnerIsBot`, e in v0.1 — 2v2 offline, **un umano** con due unità ([`D-155`](../../decisions/RT_PDR_00_Decision_Log.md)) — quel gate **è** il gate di squadra;
* `FRTReactionWindowOpenedSignature` è un `DECLARE_DELEGATE_TwoParams`, cioè **single-cast**: due giocatori umani locali non potrebbero nemmeno legare due view model. La forma del delegate esclude già il caso, e un gate qui difenderebbe da uno stato irrappresentabile.

⚠️ **Il giorno del secondo umano per squadra questa è la prima riga da rileggere**, insieme al payload del delegate — che oggi non porta l'`OwnerTeamId` che servirebbe a decidere. Owner: #166 / multiplayer.

---

## Tre cose che la misura ha corretto, e che valgono più del codice

### 1. Un criterio di DoD non era verificabile

La issue proponeva *«falsificato da: un `grep` fuori da `Tests/` che continua a dare zero»*. ⛔ Quel criterio misura il **testo del sorgente**, non il comportamento: un `Hook()` scritto e mai chiamato lo passerebbe lasciando il difetto esattamente dov'era — cioè il falso positivo perfetto per il difetto che questa issue chiude. Sostituito con `Reactions.ViewModel.GameModeBindsTheWindowInProduction`, che chiama il cablaggio di produzione e guarda la resolution fermarsi.

### 2. Chiudere una finestra può aprirne subito un'altra

La prima stesura di `SubmitClosesAndResumesOverwatch` asseriva `!IsWindowOpen()` dopo l'inoltro, **ed è andata rossa**. Chiudere una finestra *riprende* la resolution, e la ripresa può aprirne un'altra sullo stesso boundary — che il view model, essendo legato, riceve nello stesso stack. `IsWindowOpen()` torna vero, e ha ragione: c'è una finestra, e non è più quella.

∴ l'osservabile corretto è l'**identità**, non «nessuna finestra aperta». ⚠️ Il sito del `Brace` non lo mostra, perché quello scenario ne apre una sola: è il motivo per cui i due siti hanno due test e non uno parametrizzato.

### 3. Il controller non si registra in un mondo non inizializzato

`UGameplayStatics::GetPlayerController` itera la `PlayerControllerList`, dove un `APlayerController` si iscrive da `AController::PostInitializeComponents` — che `AActor::PostActorConstruction` chiama **solo se `World->AreActorsInitialized()`**. In un mondo di prova non inizializzato il controller esiste, è spawnato, è valido, e `GetPlayerController` risponde `nullptr`: cinque assertion rosse in cascata da una causa sola, con i sintomi di un cablaggio rotto che non lo era.

`RTVeilTests` aveva già registrato la stessa famiglia di trappole per un'altra ragione (`ProcessEvent` che scarta gli eventi dinamici). La riga è la stessa: `World->InitializeActorsForPlay(FURL())`.

---

## Verifica

| gate | esito |
|---|---|
| Compile (`RefactorTacticsEditor`, UE 5.8) | **PASS** |
| `RefactorTactics.Reactions.ViewModel.*` (7 test) | **PASS** |
| Suite intera `RefactorTactics` | **2.221 PASS · 3 FAIL preesistenti** |
| Determinism / Replay | **PASS** (dentro la suite, nessuna regressione) |
| Privacy | **PASS** — `MakeReactionWindowView` invariata, sanitizzazione a monte |
| PIE | **NOT RUN** — di #166 |
| Packaged | **NOT RUN** |

⚠️ **I 3 fallimenti sono preesistenti, misurati e non dedotti**: la stessa terna fallisce su `main` a `e5a82900` con il branch fuori dall'albero — `Bot.StallDefinitionsOnTheGeneratedTestArena`, `IconCatalog.RealCatalogCoversRequiredIds` (chiave `UI.Icon.Identity.Branth` non coperta), `Match.Autobattle.EngagesOnTheGeneratedTestArena`.

**Bound [#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818)**: righe di `RTTurnManager.cpp` + `.h`, **10.575 → 10.609** su 11.000. Margine residuo **391**. Il corpo del nuovo accessore vive in `RTTurnManager_Movement.cpp`, che non è contato — come già `GetOpenReactionWindowId`.

---

## Cosa questa issue sblocca, e cosa resta fuori

✅ **Sbloccate**: `PIE-V01-RXBRACE` e `PIE-V01-RXPLAYBACK` (#166) · la misura `p50`/`p90` del pacing, che diventa possibile perché solo ora le finestre vengono contate.

⛔ **Fuori**: il widget `WBP_FastDecision` e il countdown *disegnato* (#166) · la slow-motion ([`D-350`](../../decisions/RT_PDR_00_Decision_Log.md)) · la vista in sola lettura per l'alleato · il gate di squadra nel view model, che nascerà col secondo umano per squadra.

### Follow-up candidate

⚠️ Il commento di `RefactorTactics.Reactions.WindowSuspendsResolution` (`RTMovementResumeTests.cpp:253-266`) descrive un `ensure` atteso e un `AddExpectedError` che il corpo del test **non contiene più**: la fetta 3 di #2679 ha reso `LockInAndResolve` capace di uscire senza concludere, e il commento è rimasto indietro. Non è di questa issue e non è stato toccato — non è una regressione, è un commento che descrive una versione precedente del proprio test.
