# Graykit Procedural Assets — spec panel sul mandato, e il vertical slice che ne è uscito

> `CURRENT` · **Stato**: revisione chiusa, **mandato eseguito in parte con scope ridotto per misura** · **Data**: 2026-09-10
> **HEAD della revisione**: misure aperte a `1a4206c7` (`main`), codice consegnato a `27f43c4a`
> **Oggetto**: il mandato `.temp/Graykit.md` (`GK-01` … `GK-12`), letto contro `Source/`, le issue lato server,
> il Decision Log, `docs/OPEN_DECISIONS.md` e i quattro referti GrayKit precedenti
> **Panel**: Wiegers (lead) · Fowler · Cockburn · Nygard · Crispin · Adzic
> **Modo**: `critique` · **Perimetro**: presentazione dell'unità e visualizzazione tattica. Nessun `.uasset` toccato, nessun PIE.
> **Scade quando**: `GKPROC-1` si chiude, o #288 cambia stato.

Questo referto non è un'autorità: registra **misure**. Gli owner restano #286, #288, #1990, #2453 e il
Decision Log.

---

## 1. Il verdetto in una riga

Il mandato è **corretto sul principio e sovradimensionato sull'ownership**: il suo vincolo centrale — *«la
geometria visuale non deve alterare collisione, occupazione cella o gameplay authority»* — è esattamente la
regola che `CLAUDE.md` §11 già impone, ed è stata implementata e misurata; ma **oltre metà delle dodici
milestone chiede di costruire cose che hanno già un owner vivo**, e una di esse contraddice una decisione
d'autore presa cinque giorni prima.

| | Voci |
|---|---:|
| 🔴 Critico | `GK-12` contraddice la decisione del 2026-09-05; `GK-02` e `GK-09` duplicano owner vivi |
| 🟠 Alto | il tempo normalizzato veniva reinventato accanto a `URTPlaybackLibrary` |
| 🟡 Medio | `GK-01` e `GK-03` erano gap reali, e stretti |

**Raccomandazione operativa, ed è ciò che è stato fatto**: implementare il nucleo che nessuno possiede
(`GK-01` parziale, `GK-02` ridotto agli operatori di posa, `GK-03`), rifiutare la duplicazione, e portare
`GK-12` in `OPEN_DECISIONS` invece di deciderlo scrivendo codice.

---

## 2. Che cosa è stato misurato, e con quale comando

Tutte su `1a4206c7`, working tree pulito all'apertura.

| # | Misura | Comando | Esito |
|---|---|---|---|
| M1 | valutazione a tempo normalizzato | lettura di `Turn/RTPlaybackLibrary.h` | ✅ **esiste**: `InterpolateAlongPath(Waypoints, Alpha)`, `RouteAlpha`, `AlphaAtMicroStep`, `PhaseTime` — funzioni pure |
| M2 | il suo consumatore | `RTTurnManager.cpp:7728-7730` | `SetVisualLocation(InterpolateAlongPath(A.World, Alpha))` |
| M3 | vocabolario dei ruoli | `Unit/RTPresentationRole.h` | `Idle`, `Move`, `Attack`, `Cast`, `Dash`, `Defend`, `Hit`, `Death`, `Fall` |
| M4 | mapping azione → presentazione | `Turn/RTPresentationBinding.h` | ✅ `URTPresentationBindingLibrary`, [D-278], #1801 |
| M5 | geometria tattica condivisa | `Map/RTOverlayArea.h`, `Map/RTOverlayPalette.h` | ✅ `FRTOverlayArea` + `ERTOverlayMeaning` + `ERTOverlayCertainty{Confirmed,Predicted}` |
| M6 | componenti visuali dell'unità | `Unit/RTUnit.cpp:57-201` | `Mesh`(Cylinder), `TeamRing`, `SelectionRing`, `FacingArrow`, `OverlayWidget`, `ContactGhost` |
| M7 | bracci | `grep -rn 'LeftArm\|RightArm' Source/` | **0** — gap reale |
| M8 | anchor nominati | `grep` sugli anchor del mandato | **0** — gap reale |
| M9 | operatori procedurali | `grep -rilE 'squash\|armswing\|\blean\b\|bobbing' Source/` | **0 file** — gap reale |
| M10 | modalità `Final`/`Graykit`/`Hybrid`/`Debug` | `grep -rniE 'PresentationMode\|HybridMode' Source/` | **0** |
| M11 | rig di fallback deciso | chiusura #2449, `rt3b-…-2026-09-05.md` §4 | `UAnimSequence` RT-owned esportate — **senza `D-nnn`, zero asset prodotti** |

---

## 3. Dove il mandato duplicava, e perché la risposta è «non aggiungerlo»

### 3.1 🔴 `GK-09` Tactical Areas — l'owner esiste, e il mandato lo dice da sé

Il mandato scrive: *«Graykit e Tactical HUD non devono implementare geometrie tattiche duplicate. Quando
possibile condividere i dati geometrici e separare soltanto il rendering.»* La riga è giusta. Ma poi elenca
`Hex`, `Disc`, `Ring`, `Cone`, `Line`, `Arc`, `Box`, `Cylinder Volume` come *primitive da fornire*, e gli
stati `valid`/`invalid`/`blocked`/`predicted`/`confirmed` come *stati logici da distinguere*.

M5 dice che quel contratto **esiste già**: `FRTOverlayArea` porta le celle, `ERTOverlayMeaning` il
significato (`Movement`, `PathTrace`, `AttackOriginAim`, `Attack`, `FriendlyFire`, `Hover`),
`ERTOverlayCertainty` la certezza (`Confirmed`, `Predicted`), e `URTOverlayPalette` decide colore, alpha,
priorità, scala e se disegnare attraverso le unità.

> **WIEGERS**: «Il requisito è auto-contraddittorio: vieta la duplicazione e poi la prescrive. Quando una
> spec fa così, la lettura corretta è quella del divieto — è la parte che porta una ragione.»

∴ **nessun operatore di geometria è entrato nell'enum**, e la ragione è scritta nel docstring di
`ERTGraykitOperator` perché il prossimo lettore non la ricostruisca.

### 3.2 🔴 `GK-02` «Action → Visual Descriptor» — il primo arco ha già un owner

La catena proposta è `Action → Visual Descriptor → Operators → Graykit Render State`. Il **primo arco** è
M4: `URTPresentationBindingLibrary` mappa `ERTResolvedEventType` sulle cue, e [D-278] lo ha istituito.
Ricostruirlo dentro il graykit produrrebbe la seconda risposta alla stessa domanda.

∴ il codice consegnato implementa **solo** `Descriptor → Operators → Pose`. *Quale* descriptor si applica lo
decide chi possiede l'azione, non questa libreria.

### 3.3 🟠 Il tempo normalizzato veniva reinventato

Il mandato chiede operatori parametrizzati su `start time`, `duration`, `normalized time` — e propone
`VisualState = Evaluate(ActionDescriptor, NormalizedTime)`. La forma è giusta; ciò che manca alla proposta è
che **quel tempo esiste già** (M1) e ha un consumatore (M2).

> **NYGARD**: «Due orologi sulla stessa risoluzione è un modo affidabile di produrre un difetto che si vede
> solo a velocità di playback diversa da 1 — cioè in replay, cioè dove nessuno lo sta guardando.»

∴ `URTGraykitLibrary::Evaluate` **consuma** l'alpha e non ne calcola uno proprio. Il docstring lo dichiara.

---

## 4. 🔴 `GK-12` — non è un gap, è una contraddizione

Il mandato chiede che il Graykit funzioni come *«modalità debug permanente»*, con `Final` | `Graykit` |
`Hybrid` | `Debug`. Il referto [`rt3b-graykit-animation-feasibility-2026-09-05.md`](rt3b-graykit-animation-feasibility-2026-09-05.md)
poneva invece, come vincolo del proprio mandato, *«senza una seconda pipeline»*; e la decisione d'autore del
2026-09-05 ha scelto `UAnimSequence` RT-owned esportate (M11).

⚠️ **Non è arbitrabile guardando il codice**: M10 dice che le modalità non esistono, e M11 che la decisione
#2449 non ha prodotto asset. Nessuna delle due parti ha un'implementazione che faccia da giudice.

∴ la voce è stata aperta come **`GKPROC-1`** in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), con le
tre uscite e i loro costi. ⛔ Nessuna riga di `GK-12` è stata implementata.

---

## 5. Il gap reale, e il codice che lo copre — `27f43c4a`

M7, M8 e M9 sono gap veri. Il commit consegna:

| File | Contenuto |
|---|---|
| `Unit/RTGraykitTypes.h` | `ERTGraykitAnchor` (i nove punti nominati), `ERTGraykitPart`, `ERTGraykitEasing`, `ERTGraykitOperator` (i sette di posa), `FRTGraykitPart`, `FRTGraykitPose`, `FRTGraykitOperator`, `ERTGraykitLocomotionStyle`, `FRTGraykitDescriptor` |
| `Unit/RTGraykitLibrary.{h,cpp}` | `WindowAlpha`, `ApplyEasing`, `AnchorOffset`, `Evaluate`, `DescriptorForStyle`, `PoseDistance` |
| `Tests/RTGraykitTests.cpp` | i sette test di §6 |

### Le scelte che vale la pena rileggere

🔑 **Le rotazioni si accumulano sommando gli Euler, non componendo quaternioni.** È ciò che rende `Evaluate`
indipendente dall'ordine degli operatori — e l'indipendenza dall'ordine è ciò che permette di comporre due
descriptor concatenandone le liste. La composizione di quaternioni non è commutativa e avrebbe reso quella
promessa falsa.

🔑 **`Pulse` e `Bob` ignorano l'easing.** Sono oscillatori: pesarli con un easing che a sua volta oscilla
(`PingPong`) produce un battimento che nessun autore ha chiesto e che nessuno saprebbe leggere rileggendo il
descriptor. Il patto è scritto nei loro docstring.

🔑 **`Run` non è `Normal` scalato**, e il test lo misura sull'insieme dei **tipi** di operatore, non sulle
pose: due pose diverse si otterrebbero anche solo cambiando le magnitudini, quindi confrontare le pose non
avrebbe dimostrato la proprietà richiesta. `Run` porta uno `Stretch` direzionale che `Normal` non ha.

⚠️ **`URTGraykitLibrary::DefaultHalfHeight` duplica `ARTUnit::UnitHalfHeight`**, per non includere `RTUnit.h`
da una libreria che non conosce abilità, piano né turno. La duplicazione è **sorvegliata**: il test la
confronta e diventa rosso se diverge. Un numero in due posti senza quel test invecchia in silenzio.

⛔ **Nessuna firma della libreria accetta un `AActor`.** La posa non ha un canale verso il gameplay: la
garanzia è strutturale, non una promessa. Il test misura in più che il *consumatore tipico* non lo faccia
per conto suo.

---

## 6. Verification

| Gate | Esito | Evidenza |
|---|---|---|
| **Compile** | `PASS` | `Build.bat RefactorTacticsEditor Win64 Development` → `Result: Succeeded`, 113 s |
| **Tests** | `PASS` | `Automation RunTests RefactorTactics.Graykit` → **7/7 `Success`** |
| **Determinism** | `N/A` | nessun percorso canonico toccato: la posa non entra in `StateHash`, TurnLog né ordinamento |
| **Replay** | `N/A` | idem — la posa è derivata, non registrata |
| **Privacy** | `N/A` | nessun dato di planning letto o esposto |
| **PIE** | `NOT RUN` | 🔴 appartiene alla sessione Editor: §8 |
| **Packaged** | `NOT RUN` | nessun asset prodotto, nulla da cuocere |

I sette test: `Determinismo`, `ComposizioneIndipendenteDallOrdine`, `FinestraTemporale`, `Anchor`,
`MoveNonEUgualeARun`, `OperatoreSconosciuto`, `NonToccaIlGameplay`.

### Validità della misura

`HEAD` invariato durante la run (`028a605c` prima e dopo); i quattro file invariati (hash confrontati).

⏱️ **Due sessioni parallele hanno usato il motore durante questa revisione**, e il fatto è dichiarato perché
una misura si legge sapendo in che finestra è stata presa: una suite in `refactor-tactics-designer` e una in
`refactor-tactics-dev`. Entrambe sono **altri cloni**: `Binaries/` è per clone, il motore è una *installed
build* (`InstalledBuild.txt` presente), e `AGENTS.md` §9 dichiara esplicitamente che una build in un clone e
una suite in un altro non si calpestano. Nessuno dei test consegnati è di **performance**, che è il caso in
cui la contesa morderebbe comunque.

⚠️ **E `HEAD` è avanzato di due commit durante il lavoro, da una sessione che condivide questa working
tree**: `4ec6832c` («gk») e `028a605c`, entrambi su `docs/shared-mcp-kit/`, `.temp/` e un PDF.
`git diff --name-only 1a4206c7..HEAD | grep -c '^Source/'` → **0**: nessun file compilato è cambiato, quindi
il binario misurato appartiene al codice misurato.

---

## 7. Ciò che resta, e il debito dichiarato

- ✅ **RISOLTO il 2026-09-10 da #2880.** Questa riga diceva *«la posa non ha ancora un consumatore»* e lo
  chiamava il rischio principale, per analogia col difetto che
  [`graykit-asset-roadmap-v10-spec-panel-2026-08-30.md`](graykit-asset-roadmap-v10-spec-panel-2026-08-30.md)
  §1 aveva diagnosticato sul kit v0.1 — *«esiste, è generato, è testato e non lo usa nessuno»*.
  Il consumatore esiste: `ARTUnit::LeftArm` / `RightArm`, `ApplyGraykitPose`, `ResetGraykitPose`, e il
  playback che li chiama sullo **stesso alpha** del movimento (`RTTurnManager.cpp`).
  🔴 **E la previsione di questo referto era sbagliata su un punto**: diceva che il consumatore *«è lavoro
  Editor (§8)»*. Non lo era. Il difetto vero non erano i componenti — erano **la regola di visibilità** e
  **il reset**, e nessuno dei due si vede aprendo l'Editor: il primo è `ShouldShowPlaceholderMesh`, che
  nasconde il segnaposto sugli eroi skeletal e che due bracci ignari avrebbero contraddetto; il secondo è
  che un `Lean` concluso *tiene* il proprio valore, quindi senza reset l'unità resta storta dal secondo turno
  in poi. Entrambi sono C++ e si misurano headless.
- ⚠️ **I numeri dei descriptor non sono un accordo di design.** Nessuno li ha visti a schermo: sono `NOT RUN`
  sul piano estetico per quanto verdi siano i test. Il tuning appartiene alla seduta Editor.
- ⚠️ **`Trail`, `Ghost`, `ImpactBurst` restano fuori** con una ragione (sono emissioni con stato, non pose) e
  senza un owner assegnato.
- ⛔ **`GK-04` … `GK-08`, `GK-10`, `GK-11` non sono stati toccati.** `GK-04` è coperto in parte —
  `ERTGraykitLocomotionStyle` ha i quattro stili e `DescriptorForStyle` li rende — ma la *selezione* dello
  stile a partire dall'azione non esiste, ed è l'arco che appartiene a [D-278].

---

## 8. EDITOR HANDOFF

Nessuna di queste voci è stata eseguita. Vedi il documento di consegna separato:
[`../../technical/runbooks/handoff-graykit-editor-2026-09-10.md`](../../technical/runbooks/handoff-graykit-editor-2026-09-10.md).

---

## 9. Issue — trovate, create, commentate

### Trovate, e perché nessuna è stata duplicata

| Issue | Rapporto con il mandato |
|---|---|
| [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) `E21` Presentazione e leggibilità | epic che possiede il dominio |
| [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) `CP E21.2` Locomozione e impatto | **owner diretto** di `GK-03`. Ha già scartato una via (`ABP_*`, #1720): il lavoro consegnato non la riapre |
| [#1990](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1990) Gray Kit Playground | possiede `L_GrayKitPlayground`, cioè il luogo dell'handoff Editor |
| [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) Combat Feedback | possiede `GK-06` e `GK-10` per intero |
| [#2444](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2444) / [#2449](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2449) | portano la decisione sul rig di fallback che `GK-12` contraddice |
| [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095), [#1871](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1871) | ingombro e volume di posa della cella — `E-02` dell'handoff |

### Create

| # | Titolo | Perché non entrava in una esistente |
|---|---|---|
| [#2879](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2879) | `[EPIC]` Graykit Procedural Visualization | il lavoro attraversa #286, #288 e #1990 senza appartenere a nessuna: serviva un nodo che dichiarasse **il confine**, cioè cosa la capability *non* è |
| [#2880](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2880) | la posa non ha un consumatore | gap misurato (M7), ed è il **bloccante**: senza, `27f43c4a` è codice senza uso |
| [#2881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2881) | nessuna regola lega l'azione allo stile | l'arco appartiene a [D-278] e la issue dice **dove la funzione non va** |

⛔ **Nessuna issue aperta per `GK-02`, `GK-09`, `GK-12`**, né per `GK-05` … `GK-08`, `GK-10`, `GK-11`: i
primi tre hanno owner o una decisione aperta, gli altri appartengono a #2453 e a issue future. Aprirle
avrebbe prodotto la forma che `roadmap-balance.md` §11 elenca come primo rischio — il **backlog parallelo**.

### Commentate

[#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) (è l'innesco di `GKPROC-1`) e
[#1990](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1990) (⛔ con l'avvertenza di **non**
allestire una scena che provi le quattro modalità: deciderebbe `GKPROC-1` per effetto collaterale).
