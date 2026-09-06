# Work order — Enemy Tactical Query: le primitive prendono lo snapshot autorevole, e nessuna conosce l'osservatore

Ingresso della wave `enemy-tactical-query/1`. È il file che [`RT3_CONTRACT.md`](../../prompts/RT3_CONTRACT.md) §4 richiede come `INPUT_HANDOFF`: un artefatto rileggibile, non testo incollato.

## Origine

Issue [#2596](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2596) — `enhancement` · `v0.1` · `P1` · milestone `v0.1 · Leggibilità` · epic [#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769).

Consumatore previsto: [#2597](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2597) (Enemy Inspection UI). **Non è in questa wave.**

Reconnaissance del 2026-09-06 su `origin/main` = `a59671c8`, misurata sul sorgente.

## Lacuna

Il giocatore deve poter chiedere di un nemico osservato dove può arrivare e dove può colpire. Tutte le primitive esistono e sono deterministiche:

| Serve | Esiste | Ordine |
|---|---|---|
| celle raggiungibili | `URTHexSimLibrary::ReachableCells` (`RTHexSimLibrary.h:96`) | `StableLess` |
| celle investite | `URTHexCombatLibrary::HexHitCells` → `FRTBlastPreview::HitCells` | `StableLess` |
| linea di tiro | `URTHexVisionLibrary::HasLineOfSight` / `DescribeLineOfSight` | — |
| slot e famiglia | `TakesMainSlot` · `TakesMovementSlot` · `IsFastMovement` (`RTCatalogLibrary.cpp:1717-1725`, `:187`) | — |
| conoscenza autorizzata | `FRTKnowledgeView::ViewForTeam` (`RTKnowledgeView.h:129`) | — |

🔴 **Nessuna di esse conosce l'osservatore.** `ReachableCells` e `HexHitCells` prendono lo **snapshot autorevole**. Chiamarle dal client per disegnare un'anteprima su un nemico significherebbe *costruire la vista* dal dato pieno — ciò che #1805 vieta:

> **«la privacy non è non disegnare, è non costruire la vista»**

È lo stesso difetto che #2485 documenta per `URTDebugReportLibrary::DescribeCell`, che *«non prende `ObserverTeamId`»*. Qui il consumatore non è uno strumento di debug: è la UI di partita.

## Il ruleset corrente, misurato

`URTCatalogLibrary::MapResolutionPhase` (`RTCatalogLibrary.cpp:169-185`) è una funzione **totale** senza `default`, e implementa ADR-0003 §3:

```
FastMovement    (20) → ERTMatchPhase::Dash    "la mobilita' rapida precede il Blast"   (:177)
NormalMovement  (20) → ERTMatchPhase::Move    "il percorso normale lo segue"           (:178)
Control (30) · Attack (40) → ERTMatchPhase::Blast                                      (:179-180)
```

∴ **il Move normale risolve dopo il Blast e non abilita alcun attacco nello stesso turno.** Il nome storico `PostMoveThreat` descrive una minaccia che il ruleset non consente. Il nome adottato è `PostDashThreat`.

Non è un caso limite: `Sprint` (`:1065`), `Dodge` (`:1225`), `Charge` (`:1241`), `Leap` (`:1255`) e `Reposition` (`:1261`) occupano **tutte** solo `ERTActionSlot::Movement`, e il codice dichiara che `MovementAndMain` *«oggi nessuna azione dei cataloghi usa»*. Ogni mobilità rapida lascia lo slot principale libero.

`Action.Charge` è `FastMovement` **e** `bCountsAsAttack = true`: il movimento *è* l'attacco, e va classificato esplicitamente.

## Contratto richiesto

Una funzione pura che, dati un osservatore e un nemico che quell'osservatore osserva davvero, restituisce le tre regioni — e che **non può** rispondere diversamente in funzione di ciò che l'osservatore non ha diritto di sapere.

Forma da imitare, già provata nel repository: `URTDebugReportLibrary::DescribeIntents(int32 ObserverTeamId, ...)`, che passa da `URTIntentPrivacyLibrary::FilterForTeam` e **non riscrive la regola**.

## Scope

- un DTO autorizzato con `ReachableCells`, `ImmediateThreat`, `PostDashThreat`, ordinati `StableLess`;
- la query **consuma** i servizi canonici della tabella sopra;
- ingresso `FRTKnowledgeView` / `FRTTeamKnowledge`, mai `FRTHexSnapshot` pieno;
- nemico non osservato → nessuna regione, non una regione vuota;
- Automation, incluso il canary di privacy.

## Fuori scope

- ⛔ resa, materiali, colori, boundary: #1941–#1944 e #2597;
- ⛔ contratto del puntatore e affordance del click: #705;
- ⛔ threat projection del bot: #536 resta bot/perception, e questa query non legge la sua belief;
- ⛔ modifiche a `FRTMapState`, snapshot, resolver, `GraphRevision`, TurnLog, replay, `StateHash`, planning, Ready, commit;
- ⛔ nuova replica di rete;
- ⛔ i due test rossi di #2556 — né soglie abbassate, né correzioni incidentali;
- ⛔ `RTActionDef.h:79-80` (commento stale su `Action.Sprint`/`MovementAndMain`): misurato, non lavoro di questa wave.

## Acceptance criteria

1. La firma della query **non** accetta `FRTHexSnapshot` pieno. Verificato sulla firma, non sull'uso.
2. Le tre regioni escono ordinate `StableLess`; nessun ordine di `TMap`/`TSet` raggiunge l'output.
3. `PostDashThreat` è calcolata solo su azioni per cui `IsFastMovement` è vero. **Anti-vacuità**: rendendo `Action.Move` idoneo, un test diventa rosso — verificato per mutazione.
4. `Action.Charge` ha una classificazione dichiarata nel codice e un test che la pinna.
5. Un muro blocca il raggiungibile; una porta aperta/chiusa lo modifica; due celle con stesso `X/Y` e `Layer` diverso restano distinte.
6. LOS bloccata rimuove la cella dalla minaccia. Per le aree la regione è l'insieme delle celle **investite**, non dei centri bersagliabili.
7. **Canary di privacy**, nella forma di `RefactorTactics.Debug.DrawIntentHidesEnemyIntent`: due stati nascosti diversi con la stessa conoscenza autorizzata producono lo stesso output.
8. Un nemico non osservato non produce alcuna regione.
9. `git grep` sul diff non trova accessi a `CanonicalIntentStore`, percorso pianificato, destinazione, bersaglio, direzione, Ready o commit avversari.
10. Nessun Actor per cella e nessun `Tick` per cella introdotti.
11. Baseline: i **soli** rossi sono i due di #2556. Un terzo rosso è una regressione finché non è dimostrato il contrario.

## Precondizione dichiarata — working tree

⚠️ `.mcp.json` risulta modificato e **non appartiene a questa wave**: è preesistente all'apertura, prodotto dall'installer RT3 (`docs/rt-three-terminals/install-rt-terminals.ps1`, che riscrive il file per checkout e ne fa il `.bak`).

È dichiarato qui perché `RT3_CONTRACT.md` §5 legge `BLOCKED` un working tree con modifiche **non dichiarate** nel write-set in ingresso. Dichiarata, non è un blocco.

⛔ Non va committata in questa wave, né inclusa in alcun `WRITE_SET`.

## Note RT3

- `PARENT_BRANCH` è `main`: questo branch nasce da `main`, e la PR va lì.
- La wave è **DEV puro**. Nessun `.uasset`, nessun `.umap`, nessun lease Unreal, nessun MCP asset write.
- EDITOR non è in catena per questa wave: non c'è nulla da autorare. VALIDATION sì — possiede il gate `PRIVACY`.
- `SEED_SOURCE`: `none` atteso (la query non ha RNG). Se la reconnaissance ne trova uno, `generated` manda `DETERMINISM` in `BLOCKED` e va dichiarato prima, non dopo.
