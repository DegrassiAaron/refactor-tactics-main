# Leggibilità e interazione del turno — panel di specifica sulla v0.1 · 2026-09-10

> `CURRENT`
>
> **Ingresso**: un work order esterno — *«il giocatore capisce selezione, movimento e attacco su un'unità, e
> nient'altro»*. Trattato come [`AGENTS.md`](../../../AGENTS.md) §8 prescrive per un kit: ne entra il
> **contenuto** (candidate tecniche, difetti nominati, domande aperte), non il preambolo di processo.
> **Panel**: Wiegers (qualità del requisito) · Cockburn (attore e obiettivo) · Fowler (confini) ·
> Nygard (produttori senza consumatori) · Adzic (falsificabilità).
> **Base misurata**: `origin/main` = `4fdb01a0b1d169963d5ae4ce364f5814d247567f` — `2026-09-10 00:49 +0200`.
> ⚠️ **`origin/main` si è mosso durante la sessione** — `07ccb918`, su cui questo referto è ribasato.
> Rimisurato il delta: fra i file citati in §1 cambiano solo `UI/RTHudViewModel.{h,cpp}`, che nessuna misura
> qui nomina, e `docs/decisions/`. **Le misure di §1 reggono su entrambi gli sha**; il contatore `D-nnn` no,
> ed è stato riassegnato sulla misura nuova (§7).
> **Working tree**: `D:/Repositories/refactor-tactics-designer`, branch `docs/leggibilita-turno-v01`, pulito.
> **Codice modificato**: nessuno. **Build · Automation Suite · PIE · packaged**: `NOT RUN`.
>
> ⚠️ **Convenzione dei numeri** ([`AGENTS.md`](../../../AGENTS.md) §14): dove il conteggio *è* l'informazione
> si scrive `xx` con accanto il comando che lo rifà. Altrove si scrivono i **nomi**, che non scadono.

---

## §1 — Le premesse del kit, rimisurate

Il kit chiedeva di non trattarle come aggiornate. Rimisurate una per una sullo sha in testa.

### 1.1 Targeting

| Premessa del kit | Verdetto | Evidenza su `4fdb01a0` |
|---|---|---|
| `ERTPointerTargetKind::Object` esiste | ✅ **regge** | `Player/RTPointerInteraction.h:68` |
| `TargetKindForAction` produce solo `None` · `Edge` · `Cell` · `Unit` | ✅ **regge** | `Player/RTPointerInteraction.cpp:4-29` — quattro `return`, nessuno è `Object` |
| Nessun produttore normale di `TargetKind::Object` | ✅ **regge, e per una ragione più profonda di quella supposta** | vedi §1.4 |
| Nessun `HandleTargetObject` nel controller | ✅ **regge** | esistono `HandleTargetCell` (`:2527`), `HandleTargetEdge` (`:2593`), `HandleClickOnUnit` (`:1385`), `HandleClickOnCell` (`:1597`), `HandleFacingSector` |
| Il click su un oggetto non produce un riferimento logico stabile nel piano | ✅ **regge, ed è dichiarato** | `FRTPointerCandidates::bMapElement` è un **`bool`**, non un id. Il commento lo dice: *«in v0.1 non si introduce un `MapElementId` generico … L'identità stabile è E23 (#324)»* (`RTPointerInteraction.h:120-127`) |
| `HeavyAttack` dichiara `DamageStructure`, ma il danno colpisce la prima struttura sulla traiettoria | ✅ **regge** | `Combat/RTHexCombatLibrary.cpp:394` — `FirstCoveredEdge(Map, Attacker.Cell, AimCell, …)` → `AccumulateStructureHit` |
| `Action.Interact` lavora su un bordo, non è un attacco generico | ✅ **regge** | `Ability/RTCatalogLibrary.cpp:1156` — *«`TargetKindForAction` forza il targeting a…»* |

⚠️ **Una correzione al kit, e non è un dettaglio.** Il kit lascia intendere che il danno strutturale «finisca»
sulla prima struttura per approssimazione. Non è così: la raccolta avviene **prima** del controllo sulla linea
di tiro, e il commento dichiara perché — *«il muro alto che ferma il colpo è anche l'unico bersaglio che
l'attaccante può avere, visto che gli impedisce di vedere chiunque stia dietro»*. È una regola scelta, non un
ripiego. Chi costruirà il targeting diretto non la sostituisce: **le affianca** un secondo percorso.

### 1.2 Action Dock

| Premessa del kit | Verdetto | Evidenza |
|---|---|---|
| `URTActionSlotWidget` espone azione, icona, cooldown, stato armato | ✅ **regge** | `UI/RTScreenHudWidgets.h:388-460` — `Action`, `bArmed`, `ReceivedCatalog`, `GetResolvedIcon`, `GetIconId` |
| Nessuna funzione player-facing per armare cliccando lo slot | ✅ **regge, ed è più netto** | vedi §1.5 |
| Generiche via hotkey `G` `B` `C` `X` | 🟡 **regge, ma sono cinque** | `Player/RTPlayerController.cpp:251-256` — `G` Guard · `B` Brace · `C` Overwatch · `X` Interact · **`Z` Wait**. Il kit ne ha persa una |
| Abilità via tasti numerici | ✅ **regge** | `OnAbility1`…`OnAbility10` → `SelectAbilityForCurrent(0…9)` (`:2104-2113`) |
| L'interfaccia somiglia a un menu di comandi ed è un display | ✅ **regge** | è la conclusione di §1.5 |

### 1.3 Presentazione

`URTPresentationBindingLibrary::DeclaredBindings()` (`Turn/RTPresentationBinding.cpp`), rimisurata:

| `ERTResolvedEventType` | Dichiarazione | `PendingOwner` |
|---|---|---|
| `Move` | cue: `bIsMovingVisually` · `SetVisualLocation` | — |
| `Attack` | cue: `PlayAttackMontage` · `PlayHitMontage` · `ShowDamageToken` · `PulseHealthBar` | — |
| `Defeated` | cue: `HideForDefeat` · `PlayDefeatMontage` | — |
| `AttackFootprint` | `PendingPresentation` | `E21` |
| `ReactionResolved` | `PendingPresentation` | `#2454` |
| `StatusChanged` | `PendingPresentation` | `#2456` |
| `HazardDamage` | `PendingPresentation` | `#2505` |

🔴 **La premessa del kit è scaduta in un punto, e la correzione conta**: il kit dice *«risultavano ancora
`PendingPresentation`»* come se fosse lo stato peggiore. `PendingPresentation` **è già il miglioramento**: fino
al 2026-09-05 quelle voci erano `NoPresentation`, e `#2483` ha reso il verso dell'assenza un **dato** con un
`PendingOwner` accanto. Le quattro voci non sono orfane: tre hanno una issue e una ha un'epic.

🔴 **E il kit chiede lo stato di ruoli che non esistono.** L'elenco che propone — *«Idle; Move; Attack; Hit;
Death; Cast; Dash; Defend; Fall; Reaction; Interact; StructureHit; StructureDestroyed»* — contiene
**quattro nomi che `ERTPresentationRole` non dichiara**: `Reaction`, `Interact`, `StructureHit`,
`StructureDestroyed` (`Unit/RTPresentationRole.h:26-35`). Non sono «ruoli senza consumer»: sono ruoli **non
modellati**. Lo stato reale, con la distinzione che il kit stesso chiedeva:

| Ruolo | Stato | Evidenza |
|---|---|---|
| `Idle` · `Move` | **consumer presente** | grafo di `URTUnitAnimInstance`, due sequence player |
| `Attack` · `Hit` · `Death` | **consumer presente** | `PlayPresentationRole(…)` in `Turn/RTTurnManager.cpp:7754, 7757, 7793, 7796, 7843, 7939` |
| `Cast` · `Dash` · `Defend` · `Fall` | **modellati, nessun consumer** | dichiarato nell'header: *«non hanno ancora nessun consumatore»* |
| `Reaction` · `Interact` · `StructureHit` · `StructureDestroyed` | **non modellati** | assenti dall'enum |

### 1.4 🔴 Il rilievo che il kit non aveva visto — `ResolveTarget` non ha chiamanti di produzione

**FOWLER.** Il kit conclude *«non risulta un produttore normale di `TargetKind::Object`»*. È vero, ma il
difetto è un piano più sotto: **l'intera precedenza semantica di §4.1 non è cablata al click.**

```
git grep -n "ResolveTarget(" origin/main -- Source/ | grep -v Tests/
→ solo la definizione (RTPointerInteraction.cpp:31) e la dichiarazione (RTPointerInteraction.h:214)
```

Il percorso reale del click è `ARTPlayerController::OnSelect` (`:1180-1300`), che fa la propria cascata:
`Cast<ARTUnit>` → `IRTSelectable` → `ResolveCellUnderCursor` → `HandleClickOnCell`. `FRTPointerCandidates` non
viene mai popolata in partita.

⚠️ **Il repository lo sa già e lo scrive**, in `RTPlayerController.h:729`: *«`Inspect` oggi non esiste —
`ResolveTarget` calcola un bersaglio che nessuno consuma (nessun chiamante di produzione)»*.

∴ **`IMPLEMENTATION DRIFT` dentro la DoD di `#705`**: la casella *«Il resolver di hit restituisce un target
logico … — `URTPointerLibrary::ResolveTarget`»* è **spuntata**, ed è vera della libreria e falsa del gioco. Il
ramo `Object` di `ResolveTarget` è irraggiungibile **due volte**: nessuno popola `bMapElement`, e nessuno
chiama la funzione.

⚠️ **Un puntatore stantio nello stesso documento**: la casella dell'oggetto multi-mesh dice *«serve il raycast
di `OnSelect` che riconosca gli elementi logici (**#74** per le porte)»*, e `#74` è **chiusa** dal 2026-09-01.
La casella resta aperta e nomina come proprio sbloccante una issue conclusa.

⚠️ **`RTPlayerController.h:757` nomina `HandleTargetUnit`** fra i *«tre soli consumatori»* di
`GetPointerContext()`. Quel simbolo **non esiste**: `git grep -n "HandleTargetUnit" -- Source/` dà solo quel
commento.

🔴 **E `HandleDeclareFacing` è il SECONDO fantasma della stessa riga**: anche quel nome non esiste —
`git grep -n "HandleDeclareFacing" 07ccb918 -- Source/` dà solo quel commento. Il simbolo reale è
`HandleFacingSector` (`RTPlayerController.cpp:2776`, che legge `GetPointerContext()` a `:2790`).
⚠️ **E i consumatori non sono tre ma sei**: `IsWorldReadOnly` (`:2310`), `IsGameplayInputBlocked` (`:2320`),
`ApplyBack` (`:2449`), `HandleTargetCell` (`:2540`), `HandleTargetEdge` (`:2606`), `HandleFacingSector`
(`:2790`). Quella riga di commento sbaglia **due nomi su tre e il conteggio**.

### 1.5 🔴 Il secondo rilievo — al dock manca una porta, non un handler

**NYGARD.** Il kit dice *«non sembra esporre una funzione player-facing per attivare l'azione»*. La misura è
più forte, e cambia chi possiede il lavoro:

- `ARTPlayerController::SelectAbilityForCurrent(int32)` sta a `RTPlayerController.h:591`, **dopo `private:`**
  (`:490`) e **senza `UFUNCTION`**;
- **nessun `UFUNCTION` del controller è `BlueprintCallable`**: sono `BlueprintPure` (letture del puntatore)
  oppure `UFUNCTION()` nudi, bersaglio di delegate dinamici — `HandleLockInCommitted` (`:503`),
  `HandlePlaybackFinished` (`:538`), `HandleMatchEndedPresentation` (`:549`) e `OnTogglePause` (`:749`).
  ⚠️ I primi tre sono delegate di **presentazione**, non di input: manca la porta *chiamabile da un
  widget*, non genericamente l'input;
- `URTActionSlotWidget` espone `SetAction` (`BlueprintCallable`, ma la chiama il **dock**, non il giocatore),
  `GetResolvedIcon`, `GetIconId`, `OnActionChanged`. Nessuna accetta un click.

∴ **non manca un grafo Blueprint: manca la porta C++ che quel grafo chiamerebbe.** È lavoro di codice, non di
`.uasset` — ed è il motivo per cui non può stare dentro `#613`, che è un checkpoint di authoring.

📊 **Il contrasto interno allo stesso header lo dimostra**: `URTFastDecisionWidget::ChooseOption(int32)` è
`BlueprintCallable` e porta il click del giocatore fino al core. La finestra di reazione una porta ce l'ha; il
dock no.

### 1.6 Un terzo rilievo — un colpo alla struttura non ha un momento

`ERTResolvedEventType` (`Turn/RTResolvedEvent.h:15`) dichiara `Move`, `Attack`, `HazardDamage`, `Defeated`,
`AttackFootprint`, `ReactionResolved`, `StatusChanged`. **Nessun valore per la struttura.**

Il danno c'è (`FRTStructureHit`, `Map/RTHexCoverLibrary.h:17`, coi campi `From · To · Amount · AttackerId` — normalizzati da `URTHexLibrary::StableLess`
— cioè **un bordo**) e il TurnLog ha `CoverDestroyed` (`Turn/RTTurnLog.h:249`). Ma fra i due non passa nessun
`ResolvedEvent`, quindi **il playback non ha un istante in cui mostrare il colpo alla struttura**.

🔑 È **esattamente** la forma di `#2505`, che per `HazardDamage` scrive: *«il produttore ESISTE … ciò che manca
è l'ISTANTE in cui giocare una cue»*. Qui manca un passo prima: manca anche il valore d'evento.

---

## §2 — Gli owner esistenti, e perché quasi tutto ne ha uno

**WIEGERS.** Prima di scrivere un requisito si cerca chi lo possiede. Risultato della ricerca:

| Responsabilità del kit | Owner esistente | Stato |
|---|---|---|
| Screen HUD in UMG, incluso `WBP_RT_ActionDock`/`WBP_RT_ActionSlot` | **#613** (CP 11.7, epic **#25** E11) | aperta; i widget sono montati e visibili — `#2759`, `#2760`, `#2784` chiuse |
| Contratto del puntatore: contesti, `TargetKind`, Back, precedenza HUD→mondo | **#705** (CP 11.8, epic **#25**) | aperta — ⚠️ `IMPLEMENTING` lo dichiara il corpo di **#25**, non la issue: su `#705` non esiste né come label né nel corpo |
| L'hover si vede | **#1614** | aperta |
| Raggiungibilità dall'input di ogni voce del kit | **#1408** (E48) | aperta; dichiara *«la dock delle azioni è **E11 (#25)**»* |
| Identità stabile di muri, porte, archi; interaction graph; leggibilità | **#324** (E23, v0.1) — `E23.3` e `E23.5` | aperta |
| Posare una porta su una mappa | **#2330** (epic #324) | aperta — 🔴 **nessuna porta esiste nel contenuto versionato** |
| Substrato degli overlay semantici d'area | **#1941** OVL-01 · **#1942** OVL-02 · **#1943** OVL-03 · **#1944** OVL-04 | aperte; palette `D-364`, valori `D-368`. ⚠️ **il MODELLO è già spedito** — vedi §2.1 |
| **Target Preview System** vero e proprio | **#2825** | aperta il 2026-09-10, con referto proprio |
| Overlay delle linee di tiro | **#2742** | aperta, bloccata da `#1941`; `D-368` ne ha sciolta una precondizione. Le residue si contano con `gh issue view 2742 --json body` |
| Rifiuto di bersaglio player-facing | **#2741** | ✅ **chiusa 2026-09-09** (PR #2754) — `RefusalForObserver` + `ARTHUD::SetTargetRefusal` |
| Feed causale del giocatore | **#1936** · **#2697** (epic **#1937**) | aperte — ⚠️ **corretto in §2.2**: `Project` un consumatore in partita ce l'ha |
| Cue di combattimento dal `ResolvedEvent` | **#2453** (epic) · **#2454** · **#2456** · **#2457** · **#2505** | aperte; **#2455** ✅ chiusa |
| Finestra di reazione, `FIRE`/`HOLD`, countdown | **#166** (CP 14.6, epic **#152** E14) | aperta; `URTFastDecisionWidget` esiste con `ChooseOption` |
| Presentazione in scena: mesh, animazioni, anelli | **#286** (E21) | aperta |
| Icon language | **#217** (E20) · **#265** (E25, v0.2) | aperte |
| Accettazione integrata PIE + packaged della v0.1 | **#2623** ROADMAP PIA · **#2616**…**#2621** | aperte |
| Detriti, crolli, Rubble — *i non-goal v0.1 del kit* | **#1848** (E51, v0.2) | aperta |

### 2.1 🔁 Corretto lo stesso giorno da un panel gemello

⚠️ **Questo referto equiparava la famiglia `OVL` al «Target Preview System», e la misura di un altro panel dice
che sono due cose.** Referto:
[`target-preview-overlay-v01-roadmap-spec-panel-2026-09-10.md`](target-preview-overlay-v01-roadmap-spec-panel-2026-09-10.md),
misurato su `main = 89fbb24e` e mergiato con **#2840**. Due sue misure cambiano la §5 di qui:

1. 🔴 **Il modello di `#1941` è già spedito, con i propri test** — `FRTOverlayArea` e `RTOverlayPalette.h`
   esistono. Il residuo di `#1941`/`#1942` **non è il modello**: sono *(a)* il consumatore dell'alpha,
   *(b)* la ribbon a schermo, *(c)* la scelta sul depth test. Trattarlo come fondazione da costruire
   rischia *«il secondo modello»*, che è il difetto che `#1941` esiste per chiudere.
2. 🔑 **Il Target Preview System è `#2825`, e lì il foglio è davvero bianco**: `git grep -il "TargetPreview" 07ccb918 -- Source/`
   non dà nessun file. È un owner **distinto** dal substrato degli overlay.

⚠️ **E una terza misura corregge un mio raccordo**: quel referto verifica che `#2741` **non è mai stata il
bloccante di `#2742`** — il bloccante è `#1941`. `#2741` ha consegnato una cosa diversa, il rifiuto di
bersaglio al click. Qui §2 le teneva vicine senza confonderle, ma la §5 le metteva in fila come se una
sbloccasse l'altra.

### 2.2 🔴 Ritirata una misura di questo referto: il feed **ha** un consumatore

⛔ **La prima stesura scriveva che i tre canali verso lo schermo hanno «zero chiamanti di produzione», e
è falso di uno dei tre — quello che contava.** Trovato in code review sulla PR di questo referto.

```
git grep -n "URTPlayerEventProjector::Project" 07ccb918 -- Source/ | grep -v Tests/
→ UI/RTHudViewModel.cpp:527, dentro BuildPlayerEventFeed
   └─ chiamata da UI/RTHUD.cpp:994 e da UI/RTScreenHudWidgets.cpp:242 — entrambe di produzione
```

🔑 **E `#2697` lo registra già come fatto**, in una casella spuntata del proprio corpo:
*«`URTPlayerEventProjector::Project` ha un consumatore in partita — **fatto** (#2707, `063ab898`)»*. Questo
referto aveva copiato la **tabella** del corpo di `#2697` — ferma alla misura di apertura — senza leggere la
checklist sotto, che la ritira. È lo stesso difetto che il referto contesta al kit in §1.3.

✅ **Cosa resta vero**: i due canali **testuali** — `GetRecentEvents` e `GetRecentEventsForTeam` — non hanno
chiamanti di produzione, e `RTHudViewModel.h:505` dichiara che il testuale *«non entra in questa catena, e non
deve»*. Il residuo di `#2697`/`#1936` è quindi **più stretto** di come la §5 lo dipingeva.

∴ **Il kit chiedeva issue nuove per lavoro che ha già un owner in quasi tutti i casi.** Ciò che segue crea solo
i buchi che restano dopo questa tabella.

---

## §3 — Le quattro epic che il kit chiede: non se ne crea nessuna

**FOWLER.** Il kit propone `Epic v0.1 Turn Comprehension`, `v0.2–v0.3 Tactical Readability`,
`v0.4–v0.6 Content and Presentation Scaling`, `v0.7–v1.0 Production and Multiplayer`. Misurate contro la
tassonomia esistente, **tutte e quattro sono già coperte**, e crearle produrrebbe il difetto che il repository
ha appena finito di vietarsi.

| Epic proposta | Già coperta da |
|---|---|
| v0.1 · Turn Comprehension | **#25** (E11, interfaccia) + **#286** (E21, scena) + **#2453** (combat feedback) + **#1937** (explainability) + **#2623** (PIA, accettazione) |
| v0.2–v0.3 · Tactical Readability | **#1941**–**#1944** (OVL) + **#217**/**#265** (icone) + **#1769** (E49, camera) + **#323** (E22) |
| v0.4–v0.6 · Content & Presentation Scaling | **#2453**, che si dichiara *«capability cross-release v0.1 → v1.0»* + **#286** + **#774** (E41) |
| v0.7–v1.0 · Production & Multiplayer | **#773** (E40) · **#775** (E42) · **#777** (E44) · **#778** (E45) · **#1881** |

⛔ **E la roadmap PIA lo vieta a nome del repository.** `#2623` elenca fra le *«Cose che PIA non crea»*:
*«Epic `E<n>` · … · **una nuova source of truth dello stato della release** · duplicati di #2149 o
#2453–#2457»*. Una epic «Turn Comprehension» che raccogliesse HUD, pointer, preview, strutture, combat feedback
e accettazione sarebbe **precisamente** quella seconda source of truth.

⚠️ [`AGENTS.md`](../../../AGENTS.md) §8 dice la stessa cosa in forma di regola: *«non creare le milestone, le
label o le epic che nomina senza verificare la tassonomia esistente»*.

∴ **Zero epic create. Zero `E<n>` assegnati. Zero milestone create. Zero label nuove.**

⚠️ **Sui `D-nnn` la riga è cambiata in corsa, e va detto invece di lasciarla contraddire la §7.** In
ricognizione non ne è stato assegnato nessuno, ed era il punto: un kit non porta un contatore condiviso
([`AGENTS.md`](../../../AGENTS.md) §12). **`D-369` e `D-370` sono nati dopo**, dalla risposta dell'autore alle
due domande di §7 — su misura a tre posti rifatta, non sulla memoria del kit.
La milestone della v0.1 è **una sola** — `v0.1 — Offline Vertical Slice` — per consolidamento del 2026-09-06.

---

## §4 — I buchi genuini

Restano dopo §2. **Tre**, e ognuno porta il motivo per cui nessun owner esistente lo assorbe.

### 4.1 Il dock non arma

**Perché non è `#613`**: `#613` è authoring `.uasset` e la sua DoD chiede *«azioni con stato disponibile /
selezionato / cooldown»* — cioè la **resa** dello stato. La porta di comando è C++ e non compare in nessuna
sua casella.
**Perché non è `#705`**: `#705` possiede la precedenza `HUD > mondo` — che un click **non** raggiunga la mappa.
Il verso opposto — che il click raggiunga il **controller** — non è nella sua matrice.
**Perché non è `#1408`**: lo esclude per iscritto, e assegna la dock a **E11 (#25)**, che è un'epic e non un
owner di lavoro.

→ **[#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826)**, parent **#25**, dipendenze **#613** · **#705** · **#1408**.

### 4.2 Nessuna azione può chiedere una struttura

`TargetKindForAction` non può restituire `Object` per costruzione, quindi il ramo `Object` di `ResolveTarget`
non è raggiungibile nemmeno il giorno in cui qualcuno popolasse `bMapElement`.

**Perché non è `#324` (E23)**: E23 possiede l'**identità** (`E23.3`) e la **leggibilità** (`E23.5`) degli
oggetti logici, non la forma del bersaglio che un'azione dichiara.
**Perché non è `#705`**: la sua casella sull'oggetto multi-mesh riguarda il **picking**; qui si tratta di cosa
il **catalogo** può dichiarare.

→ **[#2827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2827)**, parent **#324**, dipendenze **#705** · **#2330** · **#1941**.

### 4.3 Il colpo alla struttura non ha un momento di playback

→ **[#2828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2828)**, parent **#2453**, gemella dichiarata di **#2505**.

⚠️ **Verso della dipendenza, corretto**: 4.2 e 4.3 sono **costruibili in modo indipendente** — il targeting
non ha bisogno dell'evento per scrivere il piano, e l'evento non ha bisogno del targeting per esistere. Ciò
che li lega è l'**outcome** della Slice C, che senza 4.3 si ferma un passo prima del playback. Nella §5
l'ordine consigliato resta 4.2 → 4.3.

---

## §5 — La sequenza v0.1

**COCKBURN.** Ogni fetta ha un attore e un obiettivo, e si chiude quando l'obiettivo è osservabile. Non è un
catalogo: è un ordine, e l'ordine è vincolato da ciò che blocca cosa.

### Slice A — il giocatore raggiunge i comandi

`#613` (dock a schermo, ✅ montato) → **#2826** (porta di comando + prompt contestuale) → `#705`
(precedenza HUD→mondo, `HUDConsumesPointerBeforeWorld`).

**Outcome**: il giocatore scopre e arma con il mouse le azioni **già implementate**, e un click sull'HUD non
finisce sulla mappa.
⚠️ Il rifiuto player-facing **c'è già** (`#2741`): questa fetta lo estende agli altri rami di rifiuto, non lo
costruisce.

### Slice B — il giocatore vede cosa sta puntando

⚠️ **Riscritta dopo §2.1**: la prima stesura metteva il modello di `#1941` in testa come fondazione da
costruire. È già spedito.

Residuo di `#1941`/`#1942` — *(a)* consumatore dell'alpha, *(b)* ribbon a schermo, *(c)* depth test →
`#1943` OVL-03 (priorità e `Primary`/`Secondary`) · `#1944` OVL-04 (i significati e la privacy) ·
`#2742` (linee di tiro, **bloccata da `#1941`** e non da `#2741`) · `#1614` (l'hover si vede) ·
`#2793` (l'anteprima non deve avere un buco dove sta un nemico ignoto).
In parallelo, con owner proprio: **`#2825`** — il Target Preview System, dove la traiettoria diventa un asse
dichiarato invece di essere conflazionata in `Shape`.

**Outcome**: prima del commit il giocatore capisce cosa sta puntando e cosa verrebbe colpito.
⛔ **Non si crea nessuna issue di Target Preview**: l'owner è `#2825`, aperta lo stesso giorno da un altro
panel. ⛔ **E non si riparte dal modello**: ridichiarare `FRTOverlayArea` darebbe alla palette due sedi.

### Slice C — una struttura si può bersagliare

**#2827** (l'azione dichiara la forma) → **#2828** (l'evento ha un momento) → `#324` `E23.5` (la
struttura si legge a schermo).
🔴 **Prerequisito di contenuto**: `#2330` — oggi **nessuna porta esiste in nessuna mappa versionata**. Il
prototipo v0.1 va quindi ancorato a una **copertura di bordo** (`FRTHexCover`), che esiste, che
`Action.HeavyAttack` danneggia già con `DamageStructure 20`, e le cui letture d'integrità sono canone
(`D-172`).

### Slice D — la risoluzione si legge

`#2454` (attivazione · impatto · sconfitta) → `AttackFootprint` (owner `E21`) → **#2828** →
`#2457` (diagnostica separata dal layer player-facing).

**Outcome**: guardando il turno si capisce chi ha agito, su cosa, con quale esito.

### Slice E — i cambiamenti indiretti non sembrano casuali

`#166` (Decision Window: `FIRE`/`HOLD`, countdown autorevole) · `#2456` (`StatusChanged`) · `#2505`
(`HazardDamage` acquista un beat) · `ReactionResolved` (owner `#2454`) · `#2697` + `#1936` (**il feed causale
acquista un consumatore**).

⚠️ **Corretto dopo la code review, vedi §2.2.** La prima stesura diceva che i tre canali hanno *«zero
chiamanti fuori dai test»* e ne faceva la fetta che pesa di più. **Falso del canale che conta**:
`URTPlayerEventProjector::Project` è consumato in partita da `RTHudViewModel::BuildPlayerEventFeed`, a sua
volta chiamata da `ARTHUD` e dal widget del log — e `#2697` lo registra già come **fatto** (#2707).

✅ Quel che resta di `#2697`/`#1936` sono i due canali **testuali**, che `RTHudViewModel.h:505` dichiara
fuori dalla catena per scelta. La fetta esiste ancora — le righe che le altre fette producono devono comparire
a schermo — ma **non è il collo di bottiglia** che questo referto le attribuiva.

### Slice F — accettazione

⛔ **Non si crea.** È `#2623` (ROADMAP PIA) con `#2616`…`#2621`, che già raggruppa le sedute Editor per mutua
esclusione — `U15`, `U43`, `U46`, `U9` — esattamente come il kit chiedeva. Le voci PIE pertinenti sono
`PIE-V01-POINTER` (`U43`), `PIE-V01-HUD` · `PIE-V01-LOG` · `PIE-V01-INTENT` · `PIE-V01-DEBUG` (`U15` + `U46`),
`PIE-ICON-01` (`U9`).

---

## §6 — Dipendenze verso la v1.0

```text
v0.1   #613 ─┬─► [#2826 dock] ─► #705 ─┬─► PIA-3 #2618 ─┐
             │                       │                │
      #1941 ─┴─► #2742 · #1943 · #1944 · #1614 ───────┤
                                                      ├─► PIA-6 #2621
       #324 ──► [#2827 targeting] ──► [#2828 evento] ─┤
         └─ #2330 (contenuto)                              │
                                                           │
      #2453 ─► #2454 · #2456 · #2457 · #2505 ─► PIA-2 #2617┤
      #1937 ─► #1936 ─► #2697 (il feed acquista lettori) ──┘
      #152/#166 (Decision Window)

v0.2   #1848 E51 (detriti e crolli) · #265 E25 (icon language) · #323 E22 (Cover Window)
v0.3   #327 E27 · #329 E29 · #330 E33 (percezione e intenti condizionali)
v0.5   #773 E40 (turno simultaneo in rete: le cue si sincronizzano dal risultato autorevole)
v0.6   #774 E41 (GAS come runtime, mai come autorità)
v0.9   #777 E44 · v1.0 #778 E45 (gate di produzione)
```

---

## §7 — Le due decisioni, chiuse in sessione

### ✅ Chiusa da `D-369` — una struttura è bersagliabile solo se l'authoring la dichiara *e* l'azione lo dichiara

Il kit propone: *«una struttura è direttamente attaccabile soltanto da un'azione che dichiara
`DamageStructure`; `BasicAttack` non acquista demolizione»*.

🔁 **Alla stesura non era canone**: `RT_PDR_00_Decision_Log.md` nominava `DamageStructure` in `D-172`,
`D-298` e `D-300` sempre come **effetto già esistente**, mai per decidere chi potesse *bersagliare* una
struttura, e `OPEN_DECISIONS.md` non portava la domanda.

✅ **Decisa in sessione lo stesso giorno — [`D-369`](../../decisions/RT_PDR_00_Decision_Log.md).** La risposta
è **sì**, e aggiunge una seconda metà che la proposta non aveva: **il distruggibile lo dichiara l'Editor**.
Due condizioni, entrambe necessarie, e nessuna delle due è la mesh.

🔁 **Poi emendata da [`D-375`](../../decisions/RT_PDR_00_Decision_Log.md) lo stesso giorno**: `D-369`
concludeva che il dato del distruggibile fosse **per tipo**, perché `FRTHexDoor` non ha `Integrity`.
`D-375` sceglie il verso opposto — **dare** il dato alle porte, nella forma che `FRTHexEdge` già porta
(`Integrity` + `State` terminale, con la guardia di `ValidateMap` a `RTHexMapAsset.cpp:706`). Con
`FRTHexDoor::DefaultIntegrity = 35`, integrità **per voce di bordo**, e `FRTHexEdgeGuard` fuori per
dichiarazione.

🔁 **Ciò che `D-375` fa scadere è un LIMITE DICHIARATO, non un invariante**: la voce **4** di
[`spec-porte-cp93.md`](../../gameplay/spec-porte-cp93.md) `## 8. Limiti dichiarati` — *«Le porte non hanno
integrità»* — e i limiti sono scritti perché scadano. La regola gruppo/segmento invece **non è nuova**: la
stessa spec scriveva già *«se un bordo del gruppo viene distrutto, gli altri restano»*, ed è la riga `E23.2`
di `roadmap-v0.1.md` a essere rimasta indietro. Allineati nello stesso giro **tre** documenti — la spec, la
roadmap e il catalogo icone, che chiedeva proprio se `Gadget.BreachCharge` sfondi una porta — più le due
domande che restano aperte, `INT-9` e `INT-10`, in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

🔑 **E la metà nuova conferma la forma esistente invece di aggiungerne una**: `FRTHexCover`
(`Edge · Type · Integrity`) e `FRTHexDoor` (`Edge · State · DoorId · StableId`) sono già tutti `EditAnywhere`
sull'asset di mappa, e `ERTHexDoorState::Destroyed` è già dichiarato terminale. Non nasce un campo: nasce la
regola che **solo quel dato conta**.

⚠️ **Questa conclusione non regge più per le porte**: [`D-375`](../../decisions/RT_PDR_00_Decision_Log.md) dà loro `Integrity`, quindi un campo nasce. Regge per coperture e archi, che il dato ce l'avevano già.

**Cosa cambia a seconda della risposta**: se `BasicAttack` non demolisce, il primo slice ha **un solo**
produttore (`HeavyAttack`) e la regola è verificabile con un test di esclusione; se demolisce, ogni attacco
diventa un attacco a struttura e il costo di validazione cresce su tutto il catalogo.

### ✅ Chiusa da `D-370` — la tassonomia è derivata, con un'etichetta sull'elemento

Il kit chiede di distinguere *struttura attaccabile · oggetto interagibile · objective · hazard · decorativo ·
visibile ma non conosciuto*. Oggi il puntatore ha **un solo bit** (`bMapElement`) e la conoscenza è già gestita
a parte (`IsKnownToObserver`, `D-225`).

✅ **Decisa in sessione — [`D-370`](../../decisions/RT_PDR_00_Decision_Log.md): derivata.** Il puntatore **non**
acquista un secondo campo; la categoria si interroga ai dati di mappa sul `(Cell, Edge)` scelto, dove ogni
tipologia ha già una rappresentazione autorata — `bIsObjective`, `Covers`, `Doors`, `Surface`/`BodyFill`, e gli
hazard nel terreno. La **forma** dell'etichetta (enum, tag, o un campo che già la implica) resta a chi
implementa, con un vincolo: sta in **un posto solo**, nei dati di mappa.

⛔ **E la derivazione passa dalla conoscenza autorizzata**: una categoria è informazione, quindi un elemento
non conosciuto collassa su «niente» — la forma che `RefusalForObserver` applica già alle unità (`D-225`).
Dove l'etichetta **vive** resta di **E23** (`#324`), `E23.3` e `E23.4`.

---

## §8 — Ciò che il kit chiedeva e non è stato fatto, col motivo

| Richiesta del kit | Esito | Motivo |
|---|---|---|
| Creare 4 epic v0.1 → v1.0 | ⛔ **non fatto** | §3 — tutte già coperte; `#2623` vieta esplicitamente la seconda source of truth |
| Creare una issue `[v0.1] Action Dock interattivo…` | ✅ **fatta**, con titolo che nomina il difetto misurato | convenzione del repository: il titolo è un fatto, non un'etichetta di feature |
| Creare una issue `[v0.1] Targeting diretto di strutture…` | ✅ **fatta**, ristretta al **dichiarare la forma** | il picking è `#705`, l'identità è `#324`: assorbirli sarebbe stato un duplicato |
| Creare una issue sul Target Preview System | ⛔ **non fatto** | l'owner è **`#2825`**, aperta lo stesso giorno da un altro panel; il substrato è `#1941`–`#1944`, il cui modello è **già spedito** (§2.1) |
| Creare una issue sul reason code player-facing | ⛔ **non fatto** | `#2741` è **chiusa**: il canale esiste (`SetTargetRefusal`) |
| Creare una Slice F di accettazione | ⛔ **non fatto** | è `#2623` PIA, con le sedute già raggruppate |
| Un documento roadmap separato dal referto | ⛔ **non fatto** | sarebbe una seconda source of truth della stessa sequenza; §5 è la roadmap |
| Assegnare `E<n>`, milestone o label nuove | ⛔ **non fatto** | `AGENTS.md` §8 e §12 |
| Assegnare `D-nnn` | ✅ **fatto DOPO la risposta dell'autore** — `D-369` e `D-370` | non dalla memoria del kit: misura a tre posti su `07ccb918`, rifatta perché `origin/main` si era mosso durante la sessione |

---

## §9 — Gate

| Gate | Esito |
|---|---|
| Codice modificato | **no** |
| Build Game / Editor | `NOT RUN` |
| Automation Suite | `NOT RUN` |
| Determinismo | `N/A` — nessuna modifica alla simulazione |
| Privacy | `N/A` — nessuna modifica; le misure citate sono di `#2618` e `#2741` |
| PIE | `NOT RUN` |
| Packaged | `NOT RUN` |

⚠️ **Nessun gate è stato eseguito e nessuno andava eseguito**: questo task non tocca codice. Al momento della
misura tre run headless di Unreal erano attive in **altri cloni** (`rt-wt-363`, `rt-wt-2788`,
`refactor-tactics-main`) — rilevate con
`Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'"` e lasciate correre, come
[`CLAUDE.md`](../../../CLAUDE.md) §10 prescrive per un lavoro che non contende il motore.

### Comandi per rifare le misure di §1

```bash
# ⚠️ Lo SHA e' pinnato, non `origin/main`: la tabella di §1 asserisce righe, e un ref che si muove
#    le fa scorrere in silenzio. Per rimisurare su un albero nuovo, sostituire SHA e riverificare le righe.
SHA=07ccb918

git grep -n "ResolveTarget(" $SHA -- Source/ | grep -v Tests/
git grep -n "HandleTargetUnit\|HandleDeclareFacing" $SHA -- Source/
git show $SHA:Source/RefactorTactics/Player/RTPointerInteraction.cpp | sed -n '4,29p'
git show $SHA:Source/RefactorTactics/Player/RTPointerInteraction.h   | sed -n '57,70p'   # l'enum: Object e' a 68
git show $SHA:Source/RefactorTactics/Player/RTPlayerController.h     | sed -n '585,600p'
git show $SHA:Source/RefactorTactics/Unit/RTPresentationRole.h       | sed -n '24,36p'   # fino a Fall, che e' a 36
git show $SHA:Source/RefactorTactics/Turn/RTResolvedEvent.h          | grep -nE "^\s+(Move|Attack|HazardDamage|Defeated|AttackFootprint|ReactionResolved|StatusChanged)[,;]?$"
git grep -n "MakePendingPresentation" $SHA -- Source/RefactorTactics/Turn/RTPresentationBinding.cpp
git grep -n "URTPlayerEventProjector::Project" $SHA -- Source/ | grep -v Tests/   # §2.2
```
