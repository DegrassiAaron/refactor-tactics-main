> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `HISTORICAL` · **Sorgente recepito il 2026-08-12** · **Materiale NON autorevole**
>
> Handoff di reconciliation prodotto fuori dal repository e **applicato in parte**. Il testo che segue è
> l'originale e **non è stato riscritto**: le correzioni stanno qui in testa e nel referto.
>
> **Referto**: [`../../roadmap-plans/roadmap-reconciliation-2026-08-12.md`](../../roadmap-plans/roadmap-reconciliation-2026-08-12.md)
> **Owner nato da qui**: [`../../../technical/spec-pointer-interaction.md`](../../../technical/spec-pointer-interaction.md) (CP 11.8)
>
> ⚠️ **Quattro premesse erano fuori data al momento della lettura**, e il documento lo aveva previsto (§0.1:
> *«Non fidarti delle checklist storiche…»*). L'audit è stato rifatto su `ee0da4b3`, non su `dda87f1a`:
>
> 1. **§4 — «#152 dice ancora CP 14.1–14.6»**: vero della **issue**, falso della roadmap.
>    `roadmap-v0.1.md` §5 documenta CP 14.7 e 14.8 per esteso **dal 2026-08-09**. Lo stale era su GitHub, non nei documenti.
> 2. **§8/§13 — «`roadmap-v0.1.md` descrive il bot partial-knowledge come totalmente mancante»**: lo diceva
>    **una** riga della §2, mentre §2.1 e §5 dicevano ✅ dal 2026-08-11. Era una contraddizione interna, non una lacuna.
> 3. **§7 — il contratto del puntatore come progetto da fare**: `RMB` è **già** `UndoAction`
>    (`RTPlayerController.cpp:246-247`) e l'hover è **già** sola presentazione (`:298-321`). Nove regole su
>    dieci del contratto **descrivono ciò che il codice fa**. I tre delta reali — nessuno stato esplicito,
>    nessuna consapevolezza di fase nell'input, nessuna precedenza HUD→mondo — **non sono nel testo che segue**.
> 4. **§8 R0 — l'elenco delle correzioni**: non vedeva che la colonna `CP` della tabella §3 era ferma su
>    **tre** epic (E7, E11, E14), né che `PIE-V01-GHOSTS` era citata da tre documenti senza esistere nel registro.
>
> ✅ **Quel che il documento ha visto giusto, ed era il suo contributo principale**: la catena `#159 → #165`
> non regge e le lane sono **parallele** (§1). Verificato e recepito in `roadmap-v0.1.md` §3.
>
> ❌ **Non recepito**: le cinque epic proposte in §10 (Super Actions, Modular Effects, Seeded Map Generation,
> Production Map Generator, Networking). Il documento stesso le marca **PROPOSTE**; senza una decisione non
> diventano roadmap, e il posto di una proposta senza decisione è `docs/OPEN_DECISIONS.md`.

# RefactorTactics — Roadmap & Tracking Reconciliation Handoff

**Data audit:** 2026-08-12  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**HEAD osservato:** `dda87f1a90de13b37e8d1e47eb85e8d91b7f07ab`  
**Obiettivo:** riallineare roadmap, Feature Registry, Epic/issue GitHub e stato reale del codice prima di continuare la v0.1; aggiungere il contratto di interazione mouse mancante senza duplicare sistemi già esistenti.

> Questo documento è un **handoff operativo per Claude Code**. Prima di applicarlo, rifare tutte le misure su `origin/main`: il repository si sta muovendo rapidamente e ci sono PR aperte stacked.

---

## 0. Regole operative

1. **Non fidarti delle checklist storiche se il codice o le issue live le smentiscono.** Misura su `origin/main`.
2. `docs/roadmap/feature-registry.yaml` è la **source of truth dello stato feature**. Gli stati sono derivati dai gate; niente percentuali.
3. `docs/roadmap/feature-registry.json` è **generato**: non editarlo a mano.
4. Il blocco suite dentro le roadmap è **generato**: non correggere a mano il numero dei test.
5. Non aggiornare `meta.last_full_audit` solo perché questo handoff esiste. Fallo **solo dopo un vero full audit** di Source/Tests/Scenarios/docs/Wiki sul commit registrato.
6. Non introdurre un secondo ruleset nella UI: PlayerController/HUD producono richieste; simulatore, pathfinding, targeting e servizi mappa decidono legalità/esito.
7. Nessun dato di planning avversario deve raggiungere il client/DTO nemico, neppure tramite hover, warning, ghost o Decision Window.
8. Non modificare `.umap` da CLI. Le verifiche visive restano sessioni PIE/editor registrate.
9. Prima di scrivere, controllare PR aperte e branch concorrenti. Al momento dell'audit risultavano aperte:
   - **PR #688** — migrazione verificata su asset serializzato; espone il problema #687.
   - **PR #694** — overlay transizioni/raggiungibilità; **stacked su #688**.
10. Non toccare file/aree già modificati da queste PR senza rebase e confronto diff.

---

# 1. Correzione importante della roadmap operativa

La catena proposta in una revisione precedente come `#159 → #165` è troppo rigida.

Lo stato live delle issue dice che **CP 14.5 (#165) non aspetta la chiusura di E13/#159**: #163 è chiusa e #160 documenta esplicitamente che i checkpoint E14 14.5–14.8 non aspettano CP 13.5.

Quindi da ora le lane corrette sono **parallele**:

```text
LANE PERCEPTION
#690 + #686 ─┐
              ├── #159 ── #160 (residui HUD/decoy/packaged)
              ┘

LANE REACTIONS
#165 ── #166 ── [#314] ── [#319]
                   │          │
                   └─ P3 / tagliabile dopo la baseline

LANE UI / ICONS
#219 + #637 ── #220 ── #77 ── #613 ── CP11.8 ── #291 input facing

LANE CONSISTENCY BEFORE GOLDEN
#625 + #687 decision/fix + #649 ── #512 ── #170 ── #171
```

`#314` Reaction Clash e `#319` Time Bank vengono **dopo** la Fast Reaction standard; se E14 va ridotta, sono i primi pezzi da spostare, non #165.

---

# 2. Reconciliation immediata: issue #14 — Epic release v0.1

## Problemi misurati

La issue principale **#14** è aggiornata solo in parte.

### Correzioni certe

La checklist oggi contiene ancora:

- `#22 — E8` come aperta, ma **#22 è CLOSED/completed**.
- `#175 — E16` come aperta, ma **#175 è CLOSED/completed**.
- `E11` indicata con **6 checkpoint**, ma esiste già **#613 = CP 11.7** e questo handoff introduce il nuovo **CP 11.8**.
- E14 viene descritta con checkpoint fino a 14.6, ma esistono anche **#314 CP14.7** e **#319 CP14.8**.

## Patch concettuale da applicare a #14

Portare almeno queste righe a:

```md
- [x] #22 — E8 · Terreni, stati e ambiente (P1, 5 CP) — **chiusa**
- [x] #175 — E16 · Orientamento e direzionalità (P1, 2 CP) — **chiusa**
- [ ] #25 — E11 · HUD, log e debug (...) — include ora CP 11.7 (#613) e CP 11.8 (pointer interaction)
- [ ] #152 — E14 · Overwatch e reazioni interattive — baseline 14.5/14.6; estensioni 14.7 (#314) e 14.8 (#319)
```

**Non** aggiornare il totale checkpoint a memoria. Rimisurarlo dalla roadmap/issue graph dopo aver creato CP11.8 e dopo aver verificato se altri CP sono stati aggiunti.

---

# 3. Reconciliation immediata: issue #25 — E11 HUD/log/debug

## Stato vecchio da rimuovere

#25 dice ancora:

> «nessun `FAutoConsoleCommand` esiste in `Source/`. Oggi si debugga a occhio sul log»

È falso su HEAD. La ricerca codice trova `FAutoConsoleCommand` almeno in:

- `Source/RefactorTactics/Turn/RTPacingConsole.cpp`
- `Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp`
- `Source/RefactorTactics/Map/RTHexOverlayConsole.cpp`
- `Source/RefactorTactics/Map/RTArenaCriteriaConsole.cpp`
- `Source/RefactorTactics/Turn/RTReactionConditionConsole.cpp`

## Checklist E11 da riallineare

La Epic non può più elencare solo #77/#78/#79/#80. Deve includere anche i checkpoint già esistenti:

- #77 — CP11.1 HUD informativo
- #78 — CP11.2 intenti alleati/certainty
- #79 — CP11.3 combat log/reason code
- #80 — CP11.4 debug commands
- #172 — CP11.5 Ghost Timeline
- #173 — CP11.6 scrubbing/Reaction branch
- #613 — CP11.7 Screen HUD UMG
- **nuova issue** — CP11.8 Pointer Interaction Contract

Inoltre collegare **#291** come residuo consumer di E11: il facing dichiarato è cablato nel gameplay, ma manca ancora un produttore di input giocatore/bot.

---

# 4. Reconciliation immediata: issue #152 — E14

La Epic E14 dice ancora **CP 14.1–14.6**.

Aggiornare l'ownership:

- 14.1–14.4: base/opportunity/trigger già atterrati secondo issue live.
- **#165 — CP14.5**: prima Decision Window viva + Wraith InterceptShot.
- **#166 — CP14.6**: counterplay + UI + pacing.
- **#314 — CP14.7**: Reaction Profile + Reaction Clash; P3; la baseline Brace non deve rallentare perché cardinalità 1.
- **#319 — CP14.8**: Decision Time Bank; dopo 14.7 e mai prima di 14.5/14.6.

### Brace: non cambiare numeri durante questo reconciliation

#314 registra come baseline corrente `Hold Ground`:

- **−10 a ogni danno diretto fino al Cleanup**;
- **blocco della prima spinta**.

Esiste però **BAL-1 #403** per decidere il confine Guard/Brace tramite playtest e **#404** per applicare l'esito. Quindi questa PR documentale **non deve ribilanciare Brace**.

---

# 5. Reconciliation E20 / Icon Language

## Stato attuale importante

#637 non è più «17 categorie tutte aperte». L'istruttoria ha ridotto il problema reale a **10 categorie**.

### Già mappabili senza cambiare enum

| Sorgente design | Categoria runtime |
|---|---|
| `Intel.*` | `Information` |
| `Map.*` | `MapInteraction` |
| `Surface.*` | `Environment` |
| `Role.*` | `Identity` |
| `Faction.*` | `Identity` |
| `Boundary.*` | `Phase` |

`UI.*` chrome esce dal catalogo semantico gameplay.

### Restano decisioni vere

- `Effect`
- `Stat`
- `Gadget`
- `Target` solo per `Cell/Object/Direction/Structure`
- `Module`
- `Geometry`
- `Weapon`
- `Decision`
- `Timing`
- `Result`

Non forzarle in categorie semanticamente sbagliate solo per far passare il validator.

### Scope v0.1

#219 ha già il runtime per derivare le **33 chiavi v0.1**; restano produzione/import asset e `DA_IconCatalog` reale. #637 riguarda il linguaggio più ampio e non deve bloccare artificiosamente le 33 icone necessarie alla v0.1.

---

# 6. Nuove issue v0.1 emerse dall'audit e da integrare nella roadmap

## #690 — Rumore per azione: intensità nei cataloghi

È lavoro **data-only**, allineato a #159. Non deve implementare il produttore runtime.

## #686 — Hearing threshold nel catalogo eroi

Valori da D-041 già in runtime:

- Gadget = 5
- Phase = 3
- Riktor = 3
- Wraith = 5

**Soglia bassa = udito migliore.**

#686 e #690 sono i due lati della stessa comparazione acustica e vanno trattati come una piccola coppia documentale/data-driven.

## #625 — Hazard damage fuori dal TurnLog canonico

Da fare **prima di pinning del golden replay #170** se possibile.

`Status.Burning` oggi può togliere HP/uccidere ma lascia solo log leggibile, non entry canonica; il replay non può spiegare la divergenza.

## #687 — `FormatVersion` non serializzato

Problema reale: la delta serialization non scrive la property quando coincide col default, quindi un asset vecchio può apparire già alla nuova versione e la migrazione non parte.

Non decidere in questo reconciliation quale soluzione usare. Aprire/tenere la decisione tecnica esplicita e non promettere migrazioni trasformative finché il meccanismo non è verificato su asset serializzato con binario vecchio/nuovo.

## #649 — tracciabilità cover bypassed

È residuo di osservabilità di #160; considerarlo nella lane «consistency before golden», senza duplicare logica di combat.

---

# 7. Nuova issue da creare — CP 11.8 Pointer Interaction Contract

## Titolo

**CP 11.8 — Pointer Interaction Contract: Hover / LMB / RMB**

## Metadata proposta

- Epic: **#25**
- Release: **v0.1**
- Priority: **P1**
- Milestone coerente con E11 (verificare il numero live prima della creazione)
- Depends on: #77, #613; usa #172 come consumer ghost; coordina con servizi mappa esistenti.

## Body proposto

```md
**Epic**: #25 · **Checkpoint**: CP 11.8 · **Dipende da**: #77 (contenuto HUD), #613 (Screen HUD / view model), #172 (ghost/facing)

## Obiettivo

Fissare **prima del codice** la matrice canonica:

`oggetto sotto il mouse × fase/stato corrente × Hover/LMB/RMB → risultato`

Il mouse non deve diventare un secondo ruleset. Input e HUD propongono operazioni; validità, targeting, path, interazioni e commit restano letti dai servizi/logica autorevoli.

## Scope — oggetti sotto il puntatore

La matrice deve avere almeno queste righe:

- `EmptyWorld`
- `Cell`
- `FriendlyUnit`
- `EnemyUnit`
- `CoverEdge`
- `Door`
- `Bridge/Transition`
- `Hazard/Surface`
- `Objective`
- `InteractionObject`
- `OwnGhost`
- `AllyIntentGhost`
- `HUDActionSlot`
- `HUDUnit/TeamRoster`
- `HUDReady`
- `HUDCombatLog/Tooltip`

Per ogni riga: `Hover`, `LMB`, `RMB` nei contesti rilevanti:
`Idle/Selection`, `Planning`, `Targeting`, `Pathing`, `ReactionWindow`, `ResolutionPlayback`, `UI modal/tooltip`.

## Regole baseline

1. **Hover non committa mai**. Può solo produrre highlight, tooltip e preview sanificate.
2. **LMB = select / primary confirm contestuale**. Non deve avere due significati concorrenti nello stesso stato.
3. **RMB = cancel/back oppure context action esplicita**, scelto per stato; mai commit implicito di un'azione costosa.
4. Il click su HUD **consuma l'input**: niente click-through verso cella/unità sottostante.
5. Durante `ResolutionPlayback`, gli input che cambierebbero il piano sono bloccati; restano camera, inspection e le sole Decision Window autorizzate da E14.
6. Una `ReactionWindow` usa esclusivamente le opzioni sanificate dell'opportunity corrente; non riapre il targeting normale.
7. Un `AllyIntentGhost` è ispezionabile ma non modificabile da chi non ne è owner, salvo comandi di coordinazione esplicitamente previsti.
8. Nessun hover/warning usa intenti nemici privati. I warning usano stato pubblico + intenti della propria squadra.
9. Door/cover/bridge/objective non deducono legalità dalla mesh: risolvono Stable ID / dato logico canonico.
10. La UI distingue sempre `Confermato / Previsto / Incerto`; una preview incerta non viene resa come esito certo.

## Definition of Done

- [ ] Esiste un owner documentale in `docs/technical/` con la matrice completa `object × state × input`
- [ ] Ogni combinazione rilevante produce una classe fra: `NoOp`, `Inspect`, `Select`, `Preview`, `Confirm`, `Cancel`, `OpenContext`, `Blocked(reason)`
- [ ] Le precedenze input sono esplicite: `Modal/Reaction UI > HUD > world tactical hit`
- [ ] Il resolver di hit restituisce un **target logico** (cell/unit/stable object id), non una decisione di gameplay
- [ ] Il PlayerController usa uno state/context esplicito; niente cascata di `if` basata solo sul tipo di Actor colpito
- [ ] Click-through HUD→world coperto da test
- [ ] `RMB` annulla targeting/path preview senza alterare il piano committato
- [ ] Hover su nemico non rilevato non crea target/tooltip che riveli dati privati
- [ ] Hover/click su `AllyIntentGhost` non consente modifica dell'intento alleato
- [ ] Door/Cover/Bridge/Objectives risolvono l'oggetto logico e la legalità viene chiesta ai servizi runtime
- [ ] Durante playback le sole scelte di gameplay possibili sono Decision Boundary autorizzate da E14
- [ ] `PIE-V01-POINTER-INTERACTION` registrata in `docs/technical/test-manuali-pie.md`

## Test automatici minimi

- `RefactorTactics.PlayerInput.HUDConsumesPointerBeforeWorld`
- `RefactorTactics.PlayerInput.HoverNeverCommits`
- `RefactorTactics.PlayerInput.RightClickCancelsPreviewOnly`
- `RefactorTactics.PlayerInput.HiddenEnemyCannotBecomeHoverTarget`
- `RefactorTactics.PlayerInput.AllyGhostIsReadOnly`
- `RefactorTactics.PlayerInput.PlaybackRejectsPlanningInput`
- `RefactorTactics.PlayerInput.ReactionWindowOwnsInputPriority`
- `RefactorTactics.PlayerInput.LogicalMapObjectResolvedFromStableId`

## Scenari

- `Visual.UI.SelectMoveCancel`
- `Visual.UI.TargetEnemyConfirmCancel`
- `Visual.UI.DoorHoverAndInteract`
- `Visual.UI.AllyIntentInspectReadOnly`
- `Visual.UI.ReactionWindowPreemptsWorldInput`
- `Spec.Privacy.HiddenEnemyHoverNoLeak`

## File coinvolti

`Player/RTPlayerController.*` · eventuale `Player/RTPointerInteraction*` · `UI/` view model / UMG di #613 · servizi `Map/`, `Targeting/`, `Pathfinding/` esistenti · `ScenarioHarness/` · `docs/technical/`

## Fuori scope

- redesign della camera
- input dell'Editor Mode
- gamepad completo/CommonUI
- logica di targeting/pathfinding dentro la UI
- nuove regole di Door/Cover/Objective

## Chiusura

Merge con automation verde, verifica PIE registrata e nessun leak di informazioni avversarie. Il contratto deve essere leggibile come specifica senza dover aprire `RTPlayerController.cpp`.
```

---

# 8. Roadmap v0.1 aggiornata — ordine di esecuzione consigliato

## R0 — Tracking Reconcile

- correggere #14;
- correggere #25;
- correggere #152;
- correggere #217/#219 se checklist live e runtime divergono;
- verificare `roadmap-v0.1.md` contro issue/codice;
- non toccare blocchi generati a mano;
- generare/validare registry dopo le modifiche.

**Exit:** roadmap, issue graph e registry non raccontano stati incompatibili.

## R1 — Perception/Noise lane

Lavoro parallelo:

- #690 intensità nei cataloghi;
- #686 soglia udito nel catalogo eroi;
- #159 Acoustic Contact/filter/TurnLog/direction/privacy;
- #160 residui HUD + decoy + packaged + #649.

## R2 — Fast Reaction baseline

- #165 Decision Window viva + Wraith InterceptShot;
- #166 counterplay/UI/pacing;
- solo dopo: #314 Reaction Profile/Clash;
- solo dopo: #319 Time Bank.

## R3 — Icon + HUD + input

- chiudere parte v0.1 di #219;
- risolvere/segmentare #637 senza bloccare le 33 icone v0.1;
- #220 consumer icon catalog;
- #77 slot trio + headless tests + vocabolario Round;
- #613 Screen HUD UMG;
- **CP11.8** pointer contract;
- #291 produttore reale della rotazione dichiarata.

## R4 — Consistency prima del Golden

- #625 hazard damage/KO nel TurnLog;
- #687 decision/fix sulla migrazione versionata;
- #649 log reason per directional cover bypass;
- verificare che nessun altro mutatore di stato logico resti fuori dal TurnLog prima di #170.

## R5 — Objectives / Match loop

- #74 Activate/Interact su oggetti reali;
- #75 contest;
- #76/fine partita se residua;
- riallineare HUD objective.

## R6 — Presentation E21

- #287 mesh personaggi;
- #288 locomotion/cast/hit/death;
- #289 readability;
- includere/risolvere #593 root non neutro e scaling del placeholder prima di moltiplicare Blueprint;
- solo dopo fare misura FPS rappresentativa di #41.

## R7 — Showcase / golden

- #512 DecisionProvider reale dopo #165;
- #170 golden 8 turni dopo TurnLog consistente;
- #171 PIE/showcase/readability.

## R8 — Release gates

- #38 playtest Hex completo;
- #41 preview performance + FPS;
- #82 test manual matrix;
- #83 packaged automation;
- #84 KPI;
- #85 Development + Shipping + G1–G15.

### Tagli controllati

Se il tempo stringe:

1. E17 stress 4v4;
2. #319 Time Bank;
3. #314 Clash avanzato, mantenendo Fast Reaction standard;
4. E7 loadout configurabile può diventare loadout fisso v0.1.

Non tagliare determinismo, TurnLog, privacy, path authoritative o la Decision Window base se la demo la usa.

---

# 9. Post-v0.1 — roadmap canonica già presente

## v0.2

- **E22 #323** — Cover Window OPEN → FIRE → SEAL
- **E23 #324** — Muri, porte, interaction graph
- **E24 #325** — Standard 3v3
- **E25 #265…** — Icon Language completa
- **E26 #326** — Tactical Bot v1
- **E35 #322** — roster 8
- **E36 #435** — Status Framework
- **E38 #609** — turn economy / movement profiles / plan validation

### E38: non anticipare in v0.1

Stato decisionale corrente:

- slot economy resta (`D-114`);
- #653 movement profiles come entità;
- #666 doppio budget passi/asperità;
- #606 ability↔movement compatibility;
- #605 plan validation;
- #607 UI reason;
- #608 scenario/determinismo;
- #641 Sprint migra **post-Blast** con `Exposed` a 2 turni.

**Non spostare Sprint nella v0.1.**

## v0.3

- **E27 #327** — percezione completa (stealth, udito/occlusione, memoria, belief)
- **E28 #328** — Expert Bot v2; checkpoint solo dopo profiling
- **E29 #329** — predictive avanzato/trap persistenti
- **E33 #330** — Conditional Intent: 1 condizione, 2 rami, 1 boundary nominato

## v0.4

- **E30 #331** — Operations map
- **E31 #332** — obiettivi multipli/logistica; va specificata prima di aprirla
- **E32 #333** — competitive 4v4 solo se E24 lo giustifica
- **E34 #244** — Character State / Configuration System

E34 ha già una roadmap completa CP34.1–34.11 + prototipi Phase Flow, Gadget Charged, Riktor Bulwark, Howitzer Siege. **Non creare un secondo sistema Transformation.**

---

# 10. Gap futuri reali — NON canonizzare senza issue/decisione

Le ricerche live non trovano un owner dedicato completo per queste aree. Tenerle come **PROPOSTE**, non fingere che siano già roadmap canonica.

## A. Super Actions — Epic da creare post-v0.1

Roadmap proposta:

1. `Super Definition` data-driven;
2. risorsa/commit;
3. consumo 100% al commit, no refund dopo interrupt/fizzle;
4. staged resolution;
5. revalidation dopo displacement;
6. gli stage già completati non fanno rollback;
7. `StableOriginRequired` solo dove dichiarato;
8. self-displacement;
9. TurnLog/replay;
10. Action Ghost/telegraphing;
11. privacy/packaged.

Usare Action Engine + `FRTActionEffectSpec`; **nessun Super Resolver parallelo**.

## B. Modular Effects + Presentation/VFX

Pipeline proposta:

```text
Action Definition
  → Effect Specs
  → Logical Outcome Events
  → Presentation Mapping
  → Niagara / SFX / UI
```

Le animazioni/VFX non decidono mai il risultato.

## C. Seeded Map Generation

Prototype v0.3:

```text
Seed + MapConstraints
  → canonical Hex/Edge data
  → ValidateMap
  → Hash
```

Validator/generator deve misurare: connectivity, macro-route, choke/counter-route, objective reachability, cover density, high ground, transitions, environment affordances, spawn fairness.

La relazione `numero celle → RoundLimit` resta **ipotesi da misurare**, non formula canonica.

## D. Production Map Generator / Level Designer

v0.5 proposta: authoring completo, batch generation, bot playtest automatico, rejection scoring, freeze/edit, stats, bake, scenario spawn, overlays A*/LOS.

Deve riusare i dati canonici già prodotti dal filone editor #619/#620/#621/#622/#623/#695; niente secondo formato.

## E. Networking / Dedicated server

Proposta v0.6, dopo vertical slice offline stabile:

- CanonicalIntentStore server-only;
- preview alleati 8–12Hz sanitized;
- commit/ready reliable;
- Fast Reaction RPC;
- Team-only ping/labels/drawing;
- privacy canaries;
- reconnect/late join/spectator;
- replay policy;
- dedicated authoritative server.

Mai replicare planning avversario su Actor globale.

---

# 11. Procedura Claude Code per applicare il reconciliation

## A. Prima di modificare

```bash
git fetch origin
git status
git log -1 --oneline origin/main
```

- Se ci sono modifiche locali dell'utente: **non sovrascriverle**.
- Verificare PR #688/#694 e ogni PR aperta nuova.
- Creare un branch dedicato dal **main aggiornato**, ad esempio:
  `docs/roadmap-reconcile-2026-08-12`.

## B. Misure da rifare

1. Issue/Epic aperte e chiuse v0.1.
2. Ricerca `FAutoConsoleCommand` in `Source/`.
3. Stato dei checkpoint E11/E14/E20.
4. Stato #159/#160/#165/#166/#314/#319.
5. Stato #625/#687/#649.
6. Stato PR e merge recenti.
7. Feature Registry: verificare gate contro codice, non solo contro body GitHub.

## C. Script registry

Usare i comandi **documentati nel repository**. In particolare il repository cita:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate
python scripts/feature_registry.py suite
python scripts/feature_registry.py shortlist
```

Se un comando o flag non esiste sull'HEAD corrente, leggere `--help` e la documentazione invece di inventarlo.

Eseguire inoltre il checker link/documentazione secondo il comando già usato dal repository; non inventare un path alternativo.

## D. Aggiornare `last_full_audit` solo se meritato

Il registry oggi porta un `last_full_audit` del 2026-08-08. Non cambiarlo con la data corrente a meno che siano stati davvero confrontati:

- `Source/`
- `Tests/`
- `Scenarios/`
- `docs/`
- Wiki clone/refs richiesti dal registry

Se il lavoro è solo «fix delle contraddizioni evidenti», lasciare `last_full_audit` invariato e registrare una nota di **partial reconciliation** in un plan/report.

## E. Verifica finale

- registry validate verde;
- generated views coerenti;
- link check verde;
- nessuna modifica manuale a output generati;
- issue bodies aggiornati senza dichiarare chiuso ciò che non ha gate/evidenza;
- se viene aggiunto CP11.8 al registry, registrarlo come `SPECIFIED`/`DESIGNED` coerentemente ai gate reali, non `IMPLEMENTING` per il solo fatto che la issue esiste.

---

# 12. Commit/PR suggeriti

## Commit 1

```text
docs(roadmap): reconcile v0.1 tracking with current main
```

Contiene solo:

- roadmap/registry coerenti;
- eventuale report di reconciliation;
- nessun gameplay.

## Commit 2

```text
docs(ui): specify pointer interaction contract for v0.1
```

Solo dopo che CP11.8 esiste e l'owner documentale è stato verificato/non duplicato.

## PR

Titolo suggerito:

```text
docs(roadmap): reconcile v0.1 state and add pointer interaction ownership
```

Nel body riportare:

- commit HEAD su cui è stato fatto l'audit;
- quali issue erano stale e perché;
- output `feature_registry.py validate`;
- output link check;
- nessun cambio di gameplay;
- CP11.8 come nuova ownership, non come nuova regola di targeting.

---

# 13. Definition of Done del reconciliation

Il lavoro è finito quando:

- [ ] #14 riconosce E8 e E16 come chiuse;
- [ ] #25 non afferma più che `FAutoConsoleCommand` è assente e include CP11.5–11.8;
- [ ] #152 include CP14.7/#314 e CP14.8/#319 con ordine corretto;
- [ ] E20 distingue scope v0.1 dalle 10 decisioni tassonomiche future di #637;
- [ ] #690/#686 sono visibili nella lane perception v0.1;
- [ ] #625 e #687 sono visibili prima del golden/release gate appropriato;
- [ ] CP11.8 esiste con owner, test e privacy gate;
- [ ] `roadmap-v0.1.md` non descrive più il bot partial-knowledge come totalmente mancante;
- [ ] nessun blocco suite/generated view è editato a mano;
- [ ] Feature Registry valida sul commit finale;
- [ ] non viene marcata `DONE` nessuna feature solo perché la sua issue è chiusa;
- [ ] viene lasciato un next-action chiaro: **#165 e #159 procedono in parallelo**, con UI/icon lane separata.

---

# 14. Next action dopo il merge

Scelta consigliata per avanzare davvero la v0.1:

```text
Lane A — Reactions:    #165
Lane B — Perception:   #690 + #686 + #159
Lane C — UI:           #219/#637 → #220 → #77/#613 → CP11.8
```

Non aspettare che una lane finisca per far partire le altre se non c'è una dipendenza reale.

