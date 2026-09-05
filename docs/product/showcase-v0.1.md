# RefactorTactics — Showcase v0.1 «Relay Basin» — `RT_Showcase_Relay_v01`

> `CURRENT` · **Stato**: scenario definito · **fixture Lite atterrata (CP 15.2)**, turni non ancora scriptati
> · **Ultimo aggiornamento**: 2026-08-08
>
> ## Riallineamento 2026-08-08 — buona parte del «non esiste» ora esiste
>
> Il delta di §3 fu scritto contro uno snapshot precedente. Cosa è cambiato:
>
> | Area | Allora | Ora |
> |---|---|---|
> | Roster 4 eroi (E6) | ⏳ | ✅ chiusa |
> | Ambiente: stati, propagazione, fuoco/acqua, terreno dinamico, azioni ambientali (E8, CP 8.2–8.5) | ⏳ | ✅ **epic chiusa** |
> | Copertura bassa e alta, integrità, distruzione (CP 9.1/9.2) | ⏳ «`FRTHexCellData` non ha il campo» | ✅ **il campo c'è**: `FRTHexCover{Edge, Type, Integrity}` |
> | Reazioni d'eroe cablate (CP 5.5 + 6.7) | ⏳ | ✅ **tre su cinque**: `Interposition`, `Deflection`, `ReactiveCapacitor` |
> | `Hero.Phase.FlowReaction` | ⏳ rinviata | ⏳ **E14** — invariato |
> | `Hero.Wraith.InterceptShot` | ⏳ E14 | ⏳ **E18**, come **Predictive Action** — non serve più una finestra interattiva |
> | Scenario Test Harness | inesistente | ✅ disponibile |
>
> ➕ **Rettifica del 2026-08-17 — due righe di questa tabella sono scadute il 2026-08-10.** Non sono state
> riscritte, perché la colonna *Ora* è una fotografia datata al **2026-08-08** e falsificarla perderebbe il
> confronto che la tabella esiste per mostrare:
> · **`Hero.Wraith.InterceptShot`** non è più `⏳ E18`: E18 è **chiusa** ([#225](https://github.com/DegrassiAaron/refactor-tactics-main/issues/225), 2026-08-10) e l'abilità è
>   consegnata — sette test `Predictive.*`, tre dei quali d'integrazione in un `UWorld` vero.
> · **«tre su cinque»** è ora **tre su quattro**: `InterceptShot` è uscita dall'insieme delle reazioni, e il
>   denominatore è calato con lei. Resta `Hero.Phase.FlowReaction`.
>
> ### La showcase è uno **scenario dell'harness**, non una seconda pipeline
>
> Ora che l'harness esiste, «Il Relè» va costruito come **scenario (o famiglia di scenari) di
> `Scenarios/`** — stesso loader, stesso runner, stesso `result.json`, stesso `StateHash`. Costruirle una
> pipeline propria significherebbe che la showcase gira su un percorso che il gioco non usa, ed è
> esattamente il difetto che l'harness esiste per impedire. Spec:
> [`../technical/tooling/test-automatico-unreal.md`](../technical/tooling/test-automatico-unreal.md).
>
> ### Punto risolto: sì, serve una Predictive Action vera
>
> Era registrato come da valutare. **Deciso** ([D-016](../decisions/RT_PDR_00_Decision_Log.md)): la showcase
> deve mostrare **una** azione predittiva reale su **Wraith** — dichiarata interamente in Planning, con
> payoff se la previsione è corretta e fallback dichiarato se no. È il pilastro della **scommessa sul
> movimento**, e senza di essa la showcase mostra tutto tranne ciò che distingue il gioco.
>
> **Non** si costruisce l'intero framework di trappole per la showcase: una sola fetta verticale, con il suo
> scenario automatico.
> **Epic**: **E15** di [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §5 · **CP 15.1–15.5**
> **Sorgente**: [`../research/design/showcase/showcase-v0.1-integrazione-nel-codice.md`](../research/design/showcase/showcase-v0.1-integrazione-nel-codice.md)
> (handoff del 2026-08-07, consolidato qui — in caso di conflitto prevale questo file)

Questo documento tiene separate tre cose che è facile confondere: **cosa il codice fa già**, **cosa la
showcase vuole mostrare**, **cosa non esiste e chi lo costruirà**. La terza sezione è la più importante:
serve a impedire che una demo si costruisca del codice speciale per sé.

**Regola dell'epic**:

```text
la showcase espone il gap
  → il sistema generale si costruisce nella SUA epic
    → il sistema ha i SUOI test
      → lo scenario lo consuma
        → golden replay
```

Non:

```text
showcase → codice speciale → demo che funziona una volta sola
```

---

## 0. Identità dello scenario

| | |
|---|---|
| Nome di lavoro | `RT_Showcase_Relay_v01` |
| Arena | `L_Showcase_Relay` (asset d'autore) · fixture generata equivalente per i test |
| Squadre | **Team 0**: Gadget + Phase — **Team 1**: Riktor + Wraith (bot) |
| Durata | 8 turni scriptati *(dato di scenario, non il `RoundLimit` del gioco — §3)* |
| Classe di mappa | **Skirmish** (~3–4 Move di attraversamento, primo contatto ~1 round) |
| Obiettivo mostrato | il controllo di un relè contestabile decide la partita, **non** l'eliminazione |

### Roster vigente — e cosa è storico

Il roster canonico della v0.1 è **Gadget · Phase · Riktor · Wraith**
([`balance/RT_HeroCatalog_v0.1.md`](../balance/RT_HeroCatalog_v0.1.md)).

Materiale **storico, non canone**, presente in PDF e documenti precedenti — da non reintrodurre:

| Elemento superato | Valore vigente |
|---|---|
| Aegis · Nyx · Drift · Vex | Gadget · Phase · Riktor · Wraith |
| 100 HP per tutti | 90 / 95 / 120 / 100 (per eroe, dal catalogo) |
| Finestra di interrupt da 5 s | **3.0 s** ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §8) |
| Mappa piatta, griglia quadrata | esagonale assiale multilivello, `FRTCellId{X,Y,Layer}` |
| GAS come motore delle abilità | resolver C++ + `URTActionData`; GAS resta north-star |

---

## 1. Canone corrente — cosa il codice fa **già**

Misurato sul repository il **2026-08-08** (HEAD `3335e36`). Il conteggio dei test vive in
[`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §2 e non si duplica qui. La showcase può
appoggiarsi a tutto ciò che segue **senza costruire nulla**.

| Area | Disponibile | Dove |
|---|---|---|
| Allestimento | `ARTGameMode` cerca/crea `ARTHexMapActor`, crea `ARTTurnManager`, chiama `SetupHexMatch`; supporta `LevelAsset`, `GeneratedDemoArena`, `GeneratedTestArena`; roster dal catalogo eroi; Team 1 marcato bot | `RTGameMode.{h,cpp}`, `Turn/RTMatchSetupLibrary.*` |
| Substrato | griglia esagonale multilivello, A\* a costi interi, transizioni (`Stair`, `Ramp`, `Bridge`, `Tunnel`, `Elevator`, `Jump`) | `Map/`, `Pathfinding/` |
| Turno | `Prep → Dash → Blast → Move → Cleanup`; snapshot, collisioni simultanee, micro-step, TurnLog con hash e serializzazione versionata | `Turn/RTHexSimLibrary.*`, `Turn/RTTurnLog*` |
| Azioni | `FRTActionDef` con `ActionId`, fase, priorità, range, costo, cooldown, `Fallback`, `Slot`, `MovementStyle`, `Effects`, `bAllowsReaction`, `InterruptPolicy` | `Ability/`, `Turn/RTActionQueue*` |
| Reazioni | Counter, Deflect, Brace, Shield, Cleanse, **Intercept** (con pass dedicato **prima** delle altre: cambia il bersaglio del colpo) | `Turn/RTReactionLibrary.*` |
| Terreni | 8 superfici; **Rough** (costo alto, blocca Dash/Charge), **Fire** (10 danni on-enter + `Burning`), **Smoke** (cap targeting a 2), **ShallowWater**/**Conductive** (conducibilità dichiarata), **Ice** (scivolamento di 1 cella nel Move con ≥ 2 MP), **HighGround** (dato) | `Map/RTHexCellData.h`, `Turn/RTHexSimLibrary.*` |
| Zone controllate | `FRTSuppressiveZone`, `FRTSuppressionMover`, `ResolveSuppression`: celle controllate, path a micro-step, primo nemico che entra, ordine totale `StepIndex → UnitId`, una sola attivazione, stop del movimento | `Combat/RTOffensiveActionLibrary.*` |
| Privacy | `FRTPlannedIntent → FilterForTeam → FRTIntentView`: l'HUD **non riceve** il piano avversario, non lo nasconde | `Turn/RTIntentPrivacyLibrary.*` |
| Bot | utility su hex, candidate da `ReachableCells`, pesi tunabili | `Bot/RTHexBotLibrary.*` |

**Limiti noti del canone corrente**, da non scambiare per bug:

- il **Dash lineare che termina sul ghiaccio non scivola** (lo scivolamento è nel Move normale);
- `HighGround` esiste come dato e **nessuna regola gli dà un bonus numerico** — è voluto, non una lacuna: la quota vale per geometria ([D-024](../decisions/RT_PDR_00_Decision_Log.md));
- le reazioni sono **pianificate e automatiche**: non chiedono una scelta live e non sospendono la simulazione
  — è il caso `AllowedResponses ≤ 1` di [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md), non un
  meccanismo diverso;
- **una** reazione d'eroe non è cablata: `Hero.Phase.FlowReaction` (⏳ E14), perché produce **movimento** dentro un
  boundary di risoluzione. Le altre **tre** sono in partita da CP 6.7 — cioè **tre su quattro**.
  `Hero.Wraith.InterceptShot` **non compare in questo conto**: non è più una reazione — è una **Predictive
  Action**, ed è consegnata (vedi sotto).

*(Aggiornato il 2026-08-08: `Wet`/`Obscured` avevano «durate in arrivo con CP 8.2» — CP 8.2 è chiuso; e le
reazioni d'eroe non cablate erano cinque.)*
*(Aggiornato il **2026-08-17**: erano «**due** non cablate», e il conto includeva `InterceptShot` —
riclassificata **Predictive Action** il 2026-08-10 con D-016, quindi uscita dall'insieme delle reazioni.
Il denominatore è calato con lei: **quattro**, non cinque.)*

---

## 2. La mappa canonica — «Relay Basin»

> ⚠️ **Layout autorato il 2026-08-08, non ereditato.** La specifica che avrebbe dovuto portare l'assegnazione
> delle celle (`docs/src/showcase/relay-v0.1-scenario-spec.md`) **non esiste nel repository**: forma, spawn e
> obiettivo vengono dall'handoff, la **disposizione dei terreni è stata progettata qui**, su autorizzazione
> dell'autore. Se la spec originale riemerge, questo layout va confrontato con essa — non sovrascritto in
> silenzio. Dettaglio in [`../roadmap/plans/showcase-v01-audit.md`](../roadmap/plans/showcase-v01-audit.md) §3.1.

**45 celle, un solo `Layer = 0`.** Forma per riga, in coordinate assiali `(q, r)`:

```text
r=-3:  q = -1 .. +1     (3)
r=-2:  q = -2 .. +2     (5)
r=-1:  q = -3 .. +3     (7)
r= 0:  q = -4 .. +4     (9)   <- lane centrale, Relay a (0,0)
r=+1:  q = -4 .. +4     (9)   <- lane acqua/conduttiva
r=+2:  q = -3 .. +3     (7)
r=+3:  q = -2 .. +2     (5)
                       ---- 45
```

### 2.1 Superfici

Ogni cella non elencata è `Floor`. **Nessuna cella compare due volte**: la verifica di coerenza richiesta
dall'handoff §7 è stata eseguita su questa tabella.

| Superficie | Celle | Ruolo tattico |
|---|---|---|
| `Smoke` | `(-3,0)` `(-2,0)` | Corridoio ovest: Gadget ci passa al turno 1; al turno 5 `MistVeil` ne aggiunge |
| `ShallowWater` | `(-3,1)` `(-2,1)` `(-1,1)` `(0,1)` | Lane di Phase; è **conduttiva**, ed è ciò che rende possibile il payoff del turno 7 |
| `Conductive` | `(1,1)` `(2,1)` | Prosegue la lane verso est: il `ConductiveNode` del turno 2 la collega all'acqua |
| `Rough` | `(1,0)` `(2,0)` | Sbarra la via diretta est→Relay ai movimenti lineari: è ciò che invalida il `Ram` del turno 7 |
| `Fire` | `(2,-1)` `(1,-1)` | Fascia sull'approccio **nord** al Relay: Wraith la attraversa al turno 3 scendendo dalla cresta |
| `HighGround` | `(2,-2)` `(3,-1)` | Cresta nord-est, `Height = 1`. Vantaggio **geometrico**, nessun bonus numerico ([D-024](../decisions/RT_PDR_00_Decision_Log.md)) |
| `Ice` | `(-1,2)` `(0,2)` `(1,2)` | Ripiano sud: scivolata deterministica al turno 7 |

Le sette superfici più `Floor` coprono **tutte e otto** quelle richieste dall'handoff, e tutte esistono già in
`ERTHexSurface`: non è stato inventato nulla.

### 2.2 Unità e obiettivo

| | Cella | Squadra |
|---|---|---|
| **Relay** | `(0,0,0)` | `Objective.Relay`, contendibile |
| Gadget | `(-4,0,0)` | **Blue** |
| Phase | `(-4,1,0)` | **Blue** |
| Riktor | `(4,0,0)` | **Red** |
| Wraith | `(4,1,0)` | **Red** |

### 2.3 Elementi di bordo

| Elemento | Bordo | Stato iniziale | Dipendenza |
|---|---|---|---|
| **Copertura bassa** | `(0,0)` ↔ `(0,-1)` | `Low`, integrità 30 | ✅ CP 9.1 |
| **Gate** | `(0,1)` ↔ `(1,1)` | **`Closed`** — Riktor lo apre al turno 5 | ✅ **CP 9.3**: è una porta, non un meccanismo nuovo |
| **Ponte** | `(2,1)` ↔ `(2,2)` | dichiarato, **non attraversato dagli 8 turni** | ⏳ **CP 9.4** |

> **Perché il ponte non è nei turni.** L'handoff §7 chiede un `Bridge Edge` e insieme fissa `Layer = 0` per
> tutte le celle. Ma `FRTHexEdge` è riservato per decisione esplicita alle **sole transizioni fra layer**
> ([D-013](../decisions/RT_PDR_00_Decision_Log.md)): un ponte fra due celle dello stesso layer **non è un
> arco**. È stato quindi modellato come **struttura di bordo**, la stessa famiglia delle porte — che è
> esattamente ciò che **CP 9.4** sta costruendo. Nessun meccanismo inventato: una dipendenza riconosciuta.
> Gli 8 turni non lo attraversano, quindi la showcase **non è bloccata** da CP 9.4.

> ⚠️ **La scala della showcase non è la scala del gioco.** `L_Showcase_Relay` è una mappa **Skirmish**: piccola
> per scelta, perché deve far leggere otto turni in pochi minuti. Le mappe **Standard** saranno sensibilmente
> più ampie (~5–7 Move di attraversamento, 150–200 celle percorribili). **Non usare questa arena come prova**
> che le mappe finali debbano avere questa dimensione —
> [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §4, §18.

**Per il primo test headless non serve un `.umap`**: la fixture è **generata da codice** e versionata come
dati; l'asset d'autore equivalente arriva dopo, per la presentazione.

---

## 3. Gli 8 turni

La **sequenza finale**, non ciò che è giocabile oggi. Ogni turno dichiara cosa dimostra, da cosa dipende e come
si verifica. La regola vale per tutti: **nessun turno introduce codice speciale per sé**.

| # | Dimostra | Dipende da | Test |
|---|---|---|---|
| 1 | Planning, ghost, commit, movimento hex, `Smoke`, Dash, acqua, `HighGround`, **facing**, cover, TurnLog | **`FixtureReference`** — facing e `CreateCover` sono *dimostrati*, non richiesti per passare | `RT.Scenario.Showcase.T1` |
| 2 | `Wet`, Push, setup conduttivo, reconfigure, **Predictive Action**, **whiff** | `FixtureReference` + **`PredictiveAction`** (E18) | `RT.Scenario.Showcase.T2` |
| 3 | Dash prima del Blast, `Fire`, `Burning`, **moving target**, fallback, AoE | **`FixtureReference`** · policy moving-target **da leggere dal catalogo** | `RT.Scenario.Showcase.T3` |
| 4 | Reaction Opportunity, **Decision Boundary**, `HOLD`/`FIRE`, facing, trigger per micro-step | `FixtureReference` + **`DecisionBoundary` + `Reaction` + `Facing`** (E14 dopo E13, E16) | `RT.Scenario.Showcase.T4` |
| 5 | `Smoke`, validazione LOS/target, correzione del piano, `Deflection`, **Interact sul gate**, `GraphRevision++` | **`FixtureReference`** — il gate è una porta, CP 9.3 è chiuso · **`ReactionPlanning`** — la `Deflection` si arma (aggiunta il 2026-08-27, quando il turno è stato scritto: la riga diceva solo `FixtureReference` e lo scenario la citava come prova di non aver bisogno d'altro) | `RT.Scenario.Showcase.T5` |
| 6 | `Intercept`, redirect del bersaglio, **rivalidazione della copertura**, `Wet`, Push, **nessuna reaction annidata** | `FixtureReference` + **`InterceptRevalidation`** ([D-017](../decisions/RT_PDR_00_Decision_Log.md)) — **non** serve il Decision Boundary: `Interposition` è automatica | `RT.Scenario.Showcase.T6` |
| 7 | Acqua+Fuoco, elettricità, `Wet`, conduttivo, `Ice`, `Rough`, validazione in planning | **`FixtureReference`** — E8 è chiusa | `RT.Scenario.Showcase.T7` |
| 8 | Predizione, rotta alternativa, KO, **objective**, cleanup, `MatchEnded` | `FixtureReference` + **`PredictiveAction` + `Objective`** (E18 + E10 CP 10.1/10.2) | `RT.Scenario.Showcase.T8` |

### Turno 1 — mappa e posizionamento

**Gadget** muove dallo spawn verso il centro attraversando `Smoke` a `(-3,0)`/`(-2,0)` e chiude con **facing a
Est**. **Phase** usa `FluidTrail` e percorre la lane d'acqua. **Riktor** usa `KineticPanel`, creando una
copertura direzionale verso il centro, e avanza. **Wraith** raggiunge la cresta `HighGround`.

*Expected*: quattro unità mosse, nessuna collisione, `Smoke` applica il cap di targeting, il TurnLog registra
quattro `Move` con le celle attraversate.

### Turno 2 — Wet e predizione

**Gadget** `ConductiveNode` verso la lane acqua/conduttiva. **Phase** `PressureJet` su Wraith: danno, `Wet` e Push
se legalmente valido. **Riktor** `Reconfigure` sul pannello. **Wraith** dichiara `InterceptShot` su una cella
prevista — e **Phase non la attraversa**.

*Expected*: `PredictionWhiffed` con reason code. La previsione sbagliata **costa** l'azione, e questo si deve
vedere: è metà del valore di una Predictive Action.

### Turno 3 — bersaglio in movimento e fuoco

**Gadget** `LinearDischarge` su Wraith. **Wraith** usa `PassingBlade` e attraversa `Fire` a `(2,-1)` **prima** del
Blast, prendendo `Burning`. **Phase** `CircularTide`. **Riktor** un'azione difensiva realmente a catalogo.

*Expected*: il bersaglio si è spostato fra dichiarazione e risoluzione, quindi si applica la **policy di moving
target del catalogo**, e il TurnLog dice quale. `Burning` scade nel Cleanup.

> ⚠️ **Il `Wet` del turno 2 non arriva qui, e la scarica di questo turno vale 24, non 32.** Il bagnato di
> `PressureJet` dura **1 turno** e `TickStatuses` lo rimuove nel Cleanup del turno in cui è stato applicato:
> fra un turno e il successivo non sopravvive. Non è un difetto — è [D-036](../decisions/RT_PDR_00_Decision_Log.md),
> che ha scelto l'**ordine** invece della durata: la coordinazione acqua+elettricità si fa **dentro lo stesso
> Blast**, dove `PressureJet` (priorità 50) risolve prima di `LinearDischarge` (55).
>
> Fino al 2026-08-08 questo documento lasciava intendere il contrario, e la combo firma della v0.1 **non era
> eseguibile in nessuna forma** (#242). Le due forme che funzionano oggi hanno entrambe uno scenario:
> `Visual.Combat.WaterElectricCoordinated` (Phase bagna e Gadget scarica nello stesso turno) e
> `Visual.Combat.WaterElectric` (il bersaglio entra nell'acqua nel Dash, e arriva al Blast già bagnato).

> 🔒 **Decisione bloccata.** *Quale* sia quella policy va **letta dai dati** di `LinearDischarge`. Se il
> catalogo non la dichiara, è una scelta di gameplay: non si inventa qui.

### Turno 4 — Overwatch

**Wraith** sceglie `Overwatch`: il **facing** definisce il cono controllato. **Gadget** entra per primo →
opportunity con risposta **`HOLD`**. **Phase** entra dopo → **seconda** opportunity, risposta **`FIRE`**.

*Expected* — e sono queste le asserzioni che contano:

```text
HOLD non consuma la carica
la seconda opportunity esiste
FIRE consuma la carica
nessuna informazione futura nel DTO
```

L'ultima riga è un requisito di **privacy**, non di correttezza
([D-021](../decisions/RT_PDR_00_Decision_Log.md)): l'avversario non deve poter dedurre la finestra, nemmeno dal
tempo.

### Turno 5 — fumo e struttura

**Phase** `MistVeil`. **Gadget** dichiara **di proposito** un piano non valido attraverso il fumo: il validator di
planning deve restituire un **reason**, e poi Gadget committa un'azione valida. **Riktor** fa `Interact` sul
**gate** `(0,1)↔(1,1)`: la topologia cambia, `GraphRevision` sale, la cache dei path si invalida. **Wraith**
`Deflection`.

*Expected*: `EdgeDisabled → EdgeEnabled`, `GraphRevisionChanged`, e un percorso che **prima non esisteva**.

### Turno 6 — interposizione

**Gadget** attacca Wraith. **Riktor** interpone: bersaglio originale `Wraith`, bersaglio effettivo `Riktor`. La
geometria si **rivalida su Riktor** — LOS, traiettoria, copertura — senza aprire una reaction annidata
([D-017](../decisions/RT_PDR_00_Decision_Log.md)). **Phase** `PressureJet` su Riktor. **Wraith** attacca Gadget.

*Expected*: `OriginalTargetEquals(Hero.Wraith)`, `EffectiveTargetEquals(Hero.Riktor)`, e la copertura applicata è
**quella di Riktor**. Serve un test **discriminante** — A e B a copertura diversa — altrimenti passerebbe
anche col comportamento sbagliato.

### Turno 7 — combo ambientale

**Phase** porta acqua su una zona `Fire`. **Gadget** usa il miglior attacco elettrico disponibile. **Wraith** fa un
`Move` normale su `Ice`. **Riktor** dichiara un `Ram` attraverso `Rough` a `(1,0)`: **il planning lo rifiuta**
con un reason di restrizione di terreno, e Riktor corregge prima del Commit.

*Expected*: il fuoco si spegne; la propagazione elettrica è ordinata e **non colpisce due volte** per lo stesso
evento; la scivolata su ghiaccio è deterministica; `EnvironmentChanged` nel TurnLog.

> ⚠️ **Vincolo d'ordine, misurato il 2026-08-24** ([#1111](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1111)).
> Acqua e scarica **non si compongono dentro un turno** se sono due intenti dello stesso turno: l'unico
> produttore d'acqua autorizzato è `Gadget.Sprinkler` = `Action.CreateWater`, che è `Environment` e quindi
> risolve nel **Cleanup**, mentre `Hero.Gadget.LinearDischarge` è `Attack` e risolve nel **Blast** — e il
> Blast precede il Cleanup. Scritti insieme, la scarica legge un bersaglio non ancora bagnato.
> ✅ **Il modello che funziona è già in questo documento**: §T2 fa entrare il bersaglio nell'acqua **nel
> Dash**. Chi riempie il T7 sceglie fra quello e due turni distinti — la scelta è aperta, il vincolo no.

> **Cos'è «combo» qui** ([D-029](../decisions/RT_PDR_00_Decision_Log.md)). Questo turno è uno **scenario
> dimostrativo di interazioni sistemiche**, non una combo di squadra: Phase e Gadget non condividono un'abilità e
> non ricevono un bonus perché sono insieme. Phase pubblica uno stato (`Wet` / acqua sulla cella), il sistema
> ambientale lo propaga, e `Hero.Gadget.LinearDischarge` legge **lo stato**, non l'identità di Phase. La stessa
> sequenza vale con qualunque altra sorgente d'acqua autorizzata. Lo scenario **dimostra** la cooperazione:
> non la implementa e non introduce regole competitive proprie
> ([ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) ·
> [`sinergie-e-combinazioni` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni)).

### Turno 8 — l'obiettivo batte il KO

**Wraith** `InterceptShot` sull'accesso previsto al Relay. **Phase** prende una **rotta alternativa**, non
attraversa la cella prevista (`PredictionWhiffed`) e termina **sul Relay**. **Riktor** tenta di contestare e
non riesce. **Gadget** completa un'ultima azione offensiva e va **KO**.

*Expected final state*:

```text
Gadget  = KO
Phase  = viva, sul Relay (0,0,0)
Relay = controllato da Blue
Match = terminato, Blue vince
```

Il punto è assegnato **dopo** ambiente e KO: si vince con un eroe a terra. È il pilastro «obiettivo, non
deathmatch» — e senza il turno 8 la showcase dimostrerebbe soltanto un deathmatch riuscito bene.

> 🔒 **Decisione bloccata.** *Perché* Riktor fallisce la contesa deve essere una causa reale del ruleset
> (costo, path, stato). Il sistema objective non esiste ancora: la causa non si può scegliere prima di sapere
> quali cause il ruleset ammetterà.

---

## 4. Delta di scope — cosa **non** esiste e chi lo costruisce

Nessuna riga di questa tabella si costruisce dentro E15.

| Delta | Stato | Epic / CP proprietario |
|---|---|---|
| Stati temporanei con durata e scadenza deterministica (`Wet`, `Burning`, `Obscured`, `Root`, `Exposed`, `Marked`, `Slow`) | ✅ | **CP 8.2** chiuso |
| `Status.Electrified` — dichiarato, **mai applicato a un'unità**: fuori perimetro, non lavoro da fare | ✅ | **CP 8.3** — [`spec-propagazione-elettrica-cp83.md`](../gameplay/spec-propagazione-elettrica-cp83.md) §D6 (*«non viene applicato»*). Era nella riga sopra come stato consegnato fino al 2026-08-27 ([D-211](../decisions/RT_PDR_00_Decision_Log.md)); se debba esistere è [#1324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1324) |
| Propagazione elettrica (≤ 3 celle, 20/12 danni, una volta per unità, ordine `distanza → CellId → UnitId`) | ✅ | **CP 8.3** chiuso |
| Acqua spegne il fuoco, `Wet` cancella `Burning` | ✅ | **CP 8.4** chiuso |
| Azioni che **creano** terreno (`CreateWater`, `Ignite`, `Electrify`, fumo di `MistVeil`, `ConductiveNode`, acqua di `FluidTrail`) | ✅ | **CP 8.5** chiuso — `CreateCover` rinviata a E9 |
| Cover direzionale sui 6 bordi, integrità, distruzione | ✅ `FRTHexCover{Edge, Type, Integrity}` in `FRTHexCellData` | **CP 9.1/9.2** chiusi |
| Strutture: **porte**, `GraphRevision`, invalidazione cache | ✅ | **CP 9.3** chiuso — la porta è un **bordo** (formato mappa v4), letta dallo stesso `BlocksTraversal` di muri e coperture |
| Strutture: **ponti** | ⏳ | **CP 9.4** — non blocca gli 8 turni, vedi §2.3 |
| `KineticPanel` / `Reconfigure` come struttura, non come mesh spostata | ⏳ | **CP 9.5** |
| Obiettivo contestabile verificato nel Cleanup, dopo ambiente e KO | ⏳ | **CP 10.2** — issue `#75` |
| Reazioni d'eroe cablate al motore (`Interposition`, `Deflection`, `ReactiveCapacitor`) | ✅ **tre su quattro** | **CP 5.5 + 6.7** chiusi — il denominatore era cinque finché `InterceptShot` era contata fra le reazioni |
| `Hero.Phase.FlowReaction` (riposizionamento **dentro** un boundary) | ⏳ rinviata | **E14** |
| Micro-step del movimento sospendibile | ⏳ | **CP 14.2** |
| Finestra `FIRE`/`HOLD` da 3 s | ⏳ | **CP 14.5** |
| `Hero.Wraith.InterceptShot` come **Predictive Action** (dichiarata in Planning, nessun input in Resolution) | ✅ **consegnata il 2026-08-10** | **E18** chiusa ([#225](https://github.com/DegrassiAaron/refactor-tactics-main/issues/225)) — [D-016](../decisions/RT_PDR_00_Decision_Log.md); **sganciata da E14**. Sette test `Predictive.*`, di cui tre d'integrazione in un `UWorld` vero: `InterceptCellHit`, `InterceptCellMiss`, `CrossingIsNotPresence` |
| Orientamento come stato di gioco (facing dal movimento, retro scoperto) | ⏳ | **CP 16.1/16.2** — [ADR-0005](../decisions/adr-0005-orientamento.md) |
| Conoscenza parziale reale (vista **a cono**, rumore, tre livelli) | ⏳ | **E13** (dipende da CP 16.1) |
| Etichette *confermato / previsto / incerto* nell'HUD | ⏳ | **CP 11.2** |

### Design **della showcase**, non del gioco

Queste regole valgono nello scenario e vivono nei **dati**, mai nel codice delle regole:

- la **relocation del Relay** (l'obiettivo si sposta durante la partita);
- il punteggio **«primo a 4 punti»**;
- la durata di **8 turni** — **dato di scenario**, non una regola: il `RoundLimit` è un parametro di formato
  (2v2 v0.1: banda 10–14, valore iniziale 12; 3v3 Standard: 16–20 —
  [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §6). Gli 8 turni della
  showcase sono **più corti apposta**: è una demo scriptata, non una partita competitiva.

> Vietato: `if (Turn == 4) { MoveRelay(); }` in `ARTTurnManager`. Quando la relocation arriverà, sarà uno
> schedule di scenario (`ObjectivePhase[]` con `StartTurn`, `ActiveCells`, `Duration`) sopra il sistema
> obiettivi di E10 — e si progetta **dopo** aver letto l'implementazione reale di E10.

---

## 5. Showcase **Lite** — la prima fetta, senza sistemi nuovi

Costruibile **oggi** (CP 15.2). Usa solo regole atterrate e serve da fixture d'integrazione:

1. spawn Gadget/Phase vs Riktor/Wraith su arena generata deterministica;
2. percorsi e collisioni simultanee;
3. `Rough` nega un Dash;
4. `Ice` fa scivolare nel Move;
5. `Fire` applica l'effetto on-enter;
6. `Smoke` limita il targeting;
7. `Hero.Phase.PressureJet` applica danno (+ `Wet`/Push per quanto rappresentabile);
8. `Hero.Riktor.Ram` usa `LinearCharge` e si ferma all'impatto;
9. Counter / Deflect / Intercept **generici**;
10. fallback su bersaglio che si sposta prima del Blast;
11. TurnLog leggibile con reason code;
12. ripetizione a parità di input ⇒ stesso log e stesso hash.

**Gate**: build Editor + Game verdi, suite verde, scenario ripetuto N volte con log/hash identico.

### 4.1 La fixture atterrata — CP 15.2 *(2026-08-07)*

`URTMatchSetupLibrary::MakeShowcaseRelayLiteArena` + `GetShowcaseRelayLiteSpawns`: nessun secondo
`ARTGameMode`, nessun dato di scenario dentro `ARTTurnManager`.

**Arena**: esagono pieno di raggio 5 sul layer 0 (**91 celle**). Le superfici stanno in **coppie speculari**
`(q,r)` / `(-q,-r)` — nessuna metà campo è più comoda dell'altra, quindi un esito è attribuibile alle scelte
e non al lato. I costi di movimento li detta il **catalogo terreni**: la fixture non incide numeri propri.

| Superficie | Celle `(q,r)` | Perché è lì |
|---|---|---|
| `ShallowWater` | `(0,0)` · `(0,-1)` · `(0,1)` | spina d'acqua centrale: applica `Wet`, conduce (payoff a CP 8.3) |
| `Conductive` | `(1,-1)` · `(-1,1)` | rete conduttiva a contatto con l'acqua |
| `Rough` | `(-2,-1)` · `(2,1)` | vieta Dash/Charge su una via d'avvicinamento |
| `Ice` | `(-2,2)` · `(2,-2)` | scivolamento di chi termina il Move |
| `Fire` | `(0,-2)` · `(0,2)` | 10 danni + `Burning` on-enter |
| `Smoke` | `(-1,-2)` · `(1,2)` | cap del targeting a 2 celle |

**Spawn canonico** (celle di pavimento, anch'esse speculari): `Hero.Gadget` `(-5,2)` e `Hero.Phase` `(-5,3)` per
il team 0; `Hero.Riktor` `(5,-2)` e `Hero.Wraith` `(5,-3)` per il team 1. Le unità si configurano da
`URTHeroCatalogLibrary`, **non** con `ConfigureAsArchetype` (legacy di test).

**Verificato da**: `RefactorTactics.ShowcaseRelay.FixtureLayoutIsStable` (conteggio celle, superfici, costi
dal catalogo, simmetria puntuale, spawn, hash stabile fra due generazioni) e
`RefactorTactics.ShowcaseRelay.LiteScenarioIsDeterministic` (due partite di 4 turni ⇒ stesso hash d'arena,
stesso hash di TurnLog per turno, stesse righe di log, stesso stato finale).

**Limiti dichiarati** — la fixture *contiene* gli elementi che le regole della §4 consumano, ma **non li
esercita ancora su richiesta**: le unità sono guidate dai bot, quindi quale cella venga calpestata in un dato
turno non è deciso dallo scenario. Dichiarare gli intenti per turno e per unità è **CP 15.3** (`#169`); gli 8
turni con hash atteso su file golden sono **CP 15.4** (`#170`). Finché 15.3 non atterra, «`Rough` nega un
Dash» è una proprietà dell'arena verificata dai test di E4/E8, non un evento garantito di questa partita.

---

## 6. Determinismo e golden replay

La formula di determinismo cambia quando entrano le finestre di reazione ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §3):

```text
stessa snapshot + stessi intenti di planning + stesse decisioni di reazione
+ stesse regole/versione + stesso seed  =  stesso risultato
```

Ogni scelta durante la resolution è un **comando autorevole append-only** della stessa esecuzione.

**Nel replay canonico si registrano**: `OpportunityId`, `ReactionInstanceId`, `DecisionBoundary`, `Response`,
`SelectedTargetId`.
**Non entrano nell'hash**: millisecondi di wall-clock, durata dei VFX, frame di presentazione, slow-motion.
Il timeout è una risposta canonica: `Response = Hold, Reason = Timeout`.

Gli input della partita golden sono un **dato**, non un click:

> 🔵 **Il blocco qui sotto è ILLUSTRATIVO, non un formato di file** — dichiarato il 2026-09-03 con
> [#170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/170), che aveva questa decisione fra
> le proprie e non poteva chiuderla senza prenderla. Porta ellissi al posto dei valori e nomi di boundary
> segnaposto (`X`, `Y`): dice **quali informazioni** siano input, non come si scrivano. L'input reale è lo
> scenario versionato — `Scenarios/RT_Showcase_Relay_v01.json` — che quelle informazioni le porta già,
> intenti e `decisions` compresi. Tutto il resto di questa §6 è invece **normativo**: la formula di
> determinismo, i cinque campi del replay canonico, ciò che non entra nell'hash, il timeout come risposta
> canonica, e le due righe su cartella e rigenerazione qui sotto.

```text
Turn 1
  Gadget:    MoveIntent … / MainAction … / Reaction …
  Phase:    …
  Riktor: …
  Wraith:  …
ReactionDecisions:
  Boundary X -> HOLD
  Boundary Y -> FIRE target Phase
```

∴ **l'oracolo del golden è la coppia `JSON + traccia`**, e va detto perché non è ovvio: la riproducibilità
non viene solo dal digest: viene dal fatto che le decisioni di reazione sono **scriptate nel JSON**. La
formula sopra include «stesse decisioni di reazione», e il T4 dello showcase ne ha due. Un golden senza il
proprio scenario non è riproducibile; uno scenario senza golden non è verificabile.

I file golden della showcase vivono con quelli del **CP 12.6**, stesso meccanismo e stessa cartella
(`Source/RefactorTactics/Tests/Golden/`) — **una cartella per `ScenarioId`, un file `turn-NN.rttl` per
turno prodotto**, che per lo showcase sono **otto**. **Rigenerazione solo con flag esplicito**
(`rt.Test.RegenerateGolden`): ogni epic che atterra cambia legittimamente l'esito, e una rigenerazione
automatica trasformerebbe il golden in una firma vuota. La PR che rigenera dichiara *perché* l'esito è
cambiato.

---

## 7. Obiettivi di prodotto — registrati, non usati come gate

La showcase serve a sei scopi con criteri diversi: demo interna, scenario di smoke test, fixture di
integrazione, golden replay, benchmark del resolver, base per il tutorial e per i playtest di leggibilità.

Solo i criteri **verificabili in automatico** sono DoD di checkpoint (CP 15.2–15.4). Gli obiettivi di prodotto
— «si capisce senza spiegazione lunga», «la tensione del turno 4 si legge» — restano qui, e si valutano nel
playtest di CP 15.5: sono ragioni per iterare, non condizioni di chiusura.

---

## 8. Errori da evitare *(dalla §53 della sorgente, ridotti a quelli attivi)*

1. reintrodurre Aegis/Nyx/Drift/Vex o qualunque valore della tabella §0;
2. creare un secondo `ARTGameMode` «showcase» con regole duplicate;
3. cablare gli 8 turni o la relocation dentro `ARTTurnManager`;
4. scrivere `if (HeroId == …)` o `if (ActionId == "Hero.Wraith.InterceptShot")` nel resolver;
5. duplicare la geometria di `FRTSuppressiveZone` per l'Overwatch;
6. valutare l'Overwatch **dopo** che il movimento è già risolto;
7. usare `Delay`, Timeline, montage o frame rate come logica;
8. fare `timeout = FIRE`;
9. dire al giocatore che «arriveranno altri trigger»;
10. sequenziare due trigger simultanei con l'ordine di un array;
11. far leggere al bot il percorso futuro o all'HUD il piano avversario;
12. usare `TMap`/`TSet` come ordine competitivo, o un GUID casuale dentro il replay;
13. anticipare E9 in un `KineticPanel` «showcase-only» o E10 in un `if (Turn == 4)`;
14. una sola PR con E8 + E9 + E10 + Fast Reaction.
