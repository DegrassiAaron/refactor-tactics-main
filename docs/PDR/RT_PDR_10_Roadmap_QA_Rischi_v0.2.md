# REFACTORTACTICS — PDR-10
## Roadmap tecnica, QA e rischi

> **Sorgente Markdown canonica** (Git) del documento PDR-10, per la regola di manutenzione PDR-00 §6 #5
> (*«i PDF sono snapshot di consultazione; le sorgenti testuali devono vivere nel repository Git»*, decisione
> [D-009](RT_PDR_00_Decision_Log.md)). Trascrive lo snapshot `RT_PDR_10_Roadmap_QA_Rischi_v0.1.pdf` e aggiunge
> una colonna **«Stato»** sintetica allineata al repo. Il **dettaglio di esecuzione** (checkpoint, mappatura
> M↔F, test count) vive nel tracker [`../design/roadmap-checkpoint.md`](../design/roadmap-checkpoint.md)
> (principio «una sola fonte logica per concetto»): qui c'è il **piano/requisiti**, lì l'**avanzamento**.

## Controllo del documento

| Campo | Valore |
|---|---|
| Documento | PDR-10 — Roadmap, QA e rischi |
| Versione | **0.2** — Sorgente Markdown canonica (Git) |
| Data | 2026-08-03 |
| Baseline tecnica | Unreal Engine 5.8 (repo: 5.8.1 bloccata) |
| Stato | Consolidato; sorgente Git prevale sul PDF snapshot v0.1 |
| Regola di prevalenza | Decisioni esplicite del progetto > requisiti consolidati > proposte PDR > ricerca web di supporto |

### Changelog

- **v0.1** (3 agosto 2026) — snapshot PDF iniziale (`RT_PDR_10_..._v0.1.pdf`).
- **v0.2** (2026-08-03) — trascrizione in Git (D-009); aggiunta colonna «Stato» allineata all'implementazione
  (repo, 63 test automatici). **Nessun requisito modificato**: solo trascrizione + stato. Divergenze MVP↔PDR
  segnalate (rete/GAS), prevale il canone MVP.

> **Legenda stato**: ✅ fatto e verificato · 🟡 parziale (vedi nota) · ⏳ da fare / north-star.

---

## Sommario esecutivo · Assunzioni operative

Il piano riduce il rischio in ordine, con ogni incremento *giocabile e osservabile*. **Assunzione operativa**
(v0.1, pag. 2): *«Stime temporali e staffing non sono ancora specificati; il piano è sequenziale per rischio,
non un calendario.»*

## 1. Strategia milestone

> *«Le milestone riducono rischio in ordine: prima determinismo locale e modello mappa, poi rete/privacy, poi
> abilities/GAS, quindi multilivello e dedicated server. Ogni incremento deve essere giocabile e osservabile.»*

## 2. Milestone Fondazioni — 11 deliverable

| # | Deliverable | Output verificabile | Stato (repo) |
|---|---|---|---|
| 1 | Architettura vertical slice | Diagramma e ownership classi. | ✅ |
| 2 | Struttura Source/Content | Cartelle e naming nel repository. | ✅ |
| 3 | Plugin e Build.cs | Build Development Editor pulita. | ✅ |
| 4 | Progetto e L_DevSandbox | Mappa avviabile e GameMode corretto. | ✅ *(mappa demo 2v2)* |
| 5 | Camera, selezione, graybox 2D | Pan/zoom/rotate, cell hover/select. | 🟡 *(hover cella ⏳, CP 1.4)* |
| 6 | FRTCellId, lookup, grafo, A* | Path visibile e testato. | ✅ *(`FRTGridCoord`, pathfinding pesato/grafo, test)* |
| 7 | Due unità e planning movimento | Draft path per unità. | ✅ *(waypoint + preview)* |
| 8 | Ready simultaneo locale | Countdown annullabile. | ✅ *(timer 30s + lock-in)* |
| 9 | Snapshot e movement resolution | Esito indipendente dal frame. | ✅ *(resolver ordine-indipendente, test)* |
| 10 | TurnLog + Automation Test | Log visibile e test automatico. | 🟡 *(combat log + 63 test; TurnLog strutturato/replay ⏳)* |
| 11 | Roadmap successiva | Backlog rete, GAS, multilivello, dedicated. | ✅ *(questo doc + tracker)* |

## 3. Acceptance criteria (Fondazioni)

- Editor e packaged Development build avviano L_DevSandbox. — ✅ *(Development + Shipping)*
- Due unità selezionabili pianificano path validi su griglia 2D. — ✅
- Quando entrambe sono Ready, viene creato uno snapshot immutabile. — ✅ *(raccogli-poi-applica)*
- Il movimento viene risolto per micro-step con policy collisione esplicita. — ✅ *(`ResolvePaths`)*
- Il TurnLog descrive move, block e fine turno; UI riproduce il log. — 🟡 *(combat log a schermo; reason codes ⏳)*
- Automation Test passa da command line e Session Frontend. — ✅ *(63 test da CLI)*
- Nessun sistema GAS, networking completo o modding viene introdotto prematuramente. — ✅

## 4. Roadmap tecnica (F0–F6)

> Fasi **ordinate per rischio**. Mappatura dettagliata M↔F e stato per exit gate nel
> [tracker](../design/roadmap-checkpoint.md) §«Allineamento con i PDR».

| Fase | Obiettivo | Exit gate | Stato (repo) |
|---|---|---|---|
| **F0** Fondazioni | loop locale movimento deterministico | Golden tests + packaged demo | ✅ (≈ M0–M2) |
| **F1** Rete privata | listen server, preview team-only, commit, privacy test | Zero canary leak | ⏳ *(⚠️ differito nell'MVP: 2v2 offline, architettura server-authority-ready)* |
| **F2** Abilities | 4×4 kit, GAS mirror, target/LOS | Golden test per ability | 🟡 *(abilità data-driven + targeting/LOS ✅; GAS ⏳ ⚠️ No-GAS nell'MVP; 2 archetipi)* |
| **F3** Mappa multilivello | layer, porte, ponte, tunnel, acqua/elettrico | Revision/cache/LOS tests | 🟡 *(ponte + LOS di elevazione ✅; porte/tunnel/acqua ⏳)* |
| **F4** Vertical slice | 2v2, objective, UI completa, bot base | Playtest interno 20-30 min | 🟡 *(2v2 + UI + bot ✅ + risoluzione animata; objective/playtest ⏳)* |
| **F5** Dedicated | server target, reconnect, telemetry, replay audit | Packaged soak test | ⏳ north-star |
| **F6** Beta systems | content pipeline, balance, accessibilità | Release checklist | ⏳ north-star |

> **Divergenze segnalate** (prevale il canone MVP): **(a)** rete F1 anticipata dal PDR vs differita dall'MVP;
> **(b)** GAS F2 dal PDR vs No-GAS nell'MVP. Recepite come direzione north-star, non come override — vedi
> [D-009 · note](RT_PDR_00_Decision_Log.md) e [`piano-canonico-mvp.md`](../design/piano-canonico-mvp.md).

## 5. Test pyramid

| Livello | Frequenza | Stato (repo) |
|---|---|---|
| Core automation (logica pura, indipendente dallo stato globale, ripulisce gli artefatti) | ogni commit | ✅ **63 test** (CLI + Session Frontend) |
| Feature tests | PR/CI | 🟡 via PIE + log |
| Network tests | PR critiche / nightly | ⏳ (con F1) |
| Functional maps | nightly | ⏳ |
| Packaged tests | milestone / release | 🟡 build Shipping ok; soak ⏳ |
| Playtest | ogni incremento giocabile | 🟡 PIE headless; playtest utente ⏳ |

## 6. Performance budgets (KPI)

| Budget | Target | Misura | Stato (repo) |
|---|---|---|---|
| Client | 60 FPS | Unreal Insights | ⏳ non misurato |
| Path (mediana) | < 2 ms | CSV per query | ⏳ non misurato |
| Preview completa | < 50 ms | input→server→ally render | ⏳ *(offline: preview locale immediata)* |
| Resolver server | < 100 ms/match MVP | scope timer per fase | ⏳ non misurato *(risoluzione sincrona rapida)* |
| Intent updates | 8-12 Hz | — | ⏳ (con rete) |
| **Replay divergence** | **0** | state/log hash | 🟡 determinismo by-design + test ordine-indip.; TurnLog/replay a hash ⏳ |
| **Intent leak** | **0** | network canary test | ⏳ (con F1) — privacy già invariante #6 |

## 7. Risk register

| Rischio | P/I | Mitigazione | Stato mitigazione |
|---|---|---|---|
| Resolver difficile da spiegare | H/H | TurnLog reason codes e UI certainty dall'inizio. | 🟡 combat log ✅; reason codes/TurnLog ⏳ |
| Leak di planning | M/H | DTO team-only, canary test packaged, no global replication. | 🟡 privacy #6 (offline); canary ⏳ |
| GAS invade autorità | M/H | Confine documentato; resolver puro prima di GAS. | ✅ resolver puro consolidato; GAS non introdotto |
| Mappa Actor-heavy | M/H | Dati compatti + instancing/chunk rendering. | ✅ griglia/terreno/ponte via ISM |
| Path/LOS accoppiati | M/M | Servizi separati e contratti testati. | ✅ `RTGridLibrary` (path) e `HasLineOfSight` separati e testati |
| Scope roster e ambienti | H/M | Vertical slice 4 personaggi, una combo primaria. | 🟡 2 archetipi (Ranger/Guardian) |
| Upgrade UE durante milestone | M/H | Patch lock; upgrade solo tra milestone. | ✅ UE 5.8.1 bloccata (canone) |
| Modding prematuro | M/M | Solo fondamenta ID/validator, niente pubblico. | ✅ fuori scope |

## 8. Definition of Done (7 criteri)

1. Funziona **server/client**, non solo Standalone. — ⏳ *(con rete F1)*
2. Non espone dati oltre la classificazione autorizzata (privacy). — 🟡 *(invariante #6, offline)*
3. Log/debug sufficienti a spiegare l'esito. — ✅ *(`LogRT` + combat log)*
4. Include Automation/Functional Test pertinente. — ✅ *(63 test)*
5. Rispetta i budget o registra la deviazione con owner. — ⏳ *(budget non misurati)*
6. Verificata in **build packaged**. — 🟡 *(Shipping ok)*
7. Documentazione, changelog e commit focalizzato. — ✅ *(`docs/design/`)*

## 9. Build e debug

- Configurazioni iniziali: **Development Editor** per iterazione; Development Client/Server quando entrano F1/F5.
- Comandi debug proposti: `rt.Turn.DumpSnapshot`, `rt.Turn.DumpLog`, `rt.Map.Debug`, `rt.Net.IntentStats`.
- Log categories dedicate: `LogRTTurn`, `LogRTMap`, `LogRTNet`, `LogRTData`. *(repo: attualmente `LogRT` unica — split ⏳)*
- Unreal Insights scopes su path query, snapshot build e ciascuna resolver phase.
- Salvare fixture di snapshot e log in directory test, non nel Content di shipping.

## 10. Sequenza commit proposta

1. `chore(project): bootstrap UE 5.8 C++ project`
2. `feat(camera): add tactical camera and cell selection`
3. `feat(map): add compact grid graph and cell lookup`
4. `feat(path): add authoritative A-star query`
5. `feat(planning): add local move intents and ready state`
6. `feat(turn): add immutable snapshot and move resolver`
7. `test(turn): add deterministic movement golden tests`

## 11. Prossimo passo

Sprint immediato = **solo F0** fino al golden test del movimento; niente GAS o rete completa prima che
snapshot, collisioni e TurnLog siano stabili. *(Stato repo: F0 completato; il lavoro è proseguito su incrementi
post-MVP — pathfinding pesato/grafo, terreni, ponte multilivello, animazione della risoluzione, dash, knockback
— tracciati in [`../design/roadmap-checkpoint.md`](../design/roadmap-checkpoint.md).)*

---

**Fonti** (dal PDF v0.1): Epic Games — Automation Test Framework / Run Automation Tests; UE 5.8.
**Owner** (PDR-00 §6 #3): PDR-10 si aggiorna per primo sui temi roadmap/QA/rischi; gli altri documenti tengono
riferimenti sintetici.
