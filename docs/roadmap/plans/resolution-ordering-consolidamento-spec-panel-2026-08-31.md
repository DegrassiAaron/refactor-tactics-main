# Resolution Ordering, Damage, Reactions ed Environment — spec panel

> **Referto di revisione**, non owner. Consuma il *Session Handoff 2026-08-31 — Damage, Skills, Action Lab &
> TD Roadmap* (Google Drive) e le voci `DQA-026`…`DQA-029` del *Knowledge Index & Consolidation Log*.
>
> **Data**: 2026-08-31 · **Base**: `origin/main` @ `0c0ee87c`, **rimisurato su `6f4e5edc`** prima del push · **Modo**: critique · **Focus**: requirements +
> architecture
>
> Il work order chiede al §25 otto output (A…H). Questo referto li produce. Non implementa nulla.

---

## 1. Il verdetto in una riga

> **Il §11 del kit descrive una gerarchia di risoluzione che il repository ha già, con altri nomi e in una
> forma più piccola — e nei due punti in cui il kit va oltre l'esistente, quel «oltre» è la colonna
> *north-star / non costruire* di un owner spedito. Il §1 (Damage) non è un delta: è il kit del 2026-08-28,
> già riconciliato da `D-238` tre giorni fa e respinto per una ragione che non è cambiata.**

Il contributo che vale il consumo è **uno**, ed è nel §11.5: la distinzione fra *Reactive Environment* ed
*Environment Propagation*. Non perché manchi al runtime — c'è, ed è `ERTReactionPassPoint::CleanupSurfaceBirth`
— ma perché **nessun documento la nomina**, e chi legge `spec-sequenza-turno.md` non può dedurla.

---

## 2. Ciò che è stato misurato

Ogni riga è un comando eseguito su `origin/main` @ `0c0ee87c`, non una lettura.

| Domanda | Comando | Esito |
|---|---|---|
| La base di lavoro è aggiornata? | `git merge-base --is-ancestor origin/main HEAD` | 🔴 **No.** Il working tree era su `feat/bot-ally-planning`, **10 commit indietro**. Tutte le misure di questo referto sono prese su `origin/main` con `git show <rev>:<path>`, mai sul tree |
| Quante sessioni parallele? | `git worktree list` | **5** checkout attivi (`1525`, `camera`, `pacing`, `t6`, radice) |
| Esiste un vocabolario di fase? | `git grep "enum class ERT.*Phase"` | ✅ **Due**, distinti per contratto: `ERTMatchPhase` (macro-fase reale) e `ERTResolutionPhase` (codice del catalogo, `0/10/…/60`) |
| Esiste un concetto di *layer* per le reazioni? | `RTReactionLibrary.h:36` | ✅ `ERTReactionPassPoint`, **6 valori**, con `PassPointFor` come funzione pura ([D-092]) |
| Esiste un *boundary* dichiarato dall'azione? | `RTActionDef.h:258` | ✅ `ERTPredictionBoundary` + campo `PredictionBoundary` su `FRTActionDef` |
| `EvaluationBoundary` esiste? | `git grep -c EvaluationBoundary -- Source` | ⛔ **0 file** — è lo scope di [#1577](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1577), release **v0.3** |
| L'ordine dei contributi è già totale? | `spec-sequenza-turno.md` §3.1 | ✅ **5 chiavi**, implementato: `MacroPhase → Priority → ActionId → SourceUnitId → EventSequence` |
| Esistono test di permutazione? | `git grep -l Permutation -- Tests` | ✅ **6**: `Actions` · `Facing` · `HexCombat` · `HexSim.MoveLog` · `Movement.Stepper` · `Simulation.ChecksumStableAcrossPermutations` |
| Il KO simultaneo ha una policy? | `git grep SimultaneousKO -- Tests` | ✅ `Match.Autobattle.SimultaneousKOFollowsDeclaredPolicy`, e `spec-sequenza-turno.md` §3.3 la dichiara (`FR-RESOLVE-02`) |
| Il refresh derivato dopo mutazione strutturale esiste? | `RTTurnManager_Blast.cpp:1366` | ✅ **Esiste ed è l'opposto di quello proposto** (§4.1) |
| `GraphRevision` è tracciata? | `TurnLog.GraphRevisionEntersTheHash` | ✅ Entra nell'hash ([D-067]) |
| `Armor` esiste? | `git grep -c Armor -- Source` | ⛔ **0 file** — invariato dal 2026-08-28 |
| `DamageResistance` · `DamagePacket` · `Shred` | idem, tre termini | ⛔ **0 file** ciascuno — invariato |
| `DamageType` | idem | ⚠️ **1 file**, e non è codice: è il commento di `RTCombatLibrary.h:29` che *dichiara* la sua assenza (*«non è `DamageType` […] e arriva con E49»*), introdotto da [D-224] il 2026-08-28. Il referto precedente lo misurò a **0** su `483e031a`, prima di quel commit: la differenza è la nota, non un produttore |
| `E49` è libera per il Damage Model? | `gh issue view 1769` | 🔴 **No.** `E49` è **Tactical Camera & Map Presentation**, creata il **2026-08-30** |
| Il commento che punta a `E49` | `git log -S "arriva con E49"` | `6a1c0cb6`, **2026-08-28** — due giorni **prima** che `E49` significasse altro |

---

## 3. Il panel

### 📚 WIEGERS — qualità del requisito

> «Il vostro §1 non è un requisito nuovo. È lo stesso documento di tre giorni fa, e ha già una risposta.»

Il §1 e il §11.7 del kit ripetono **verbatim nella sostanza** la pipeline, le tredici invarianti e la formula
additiva del kit *Combat + SkillGrammar Delta* del **2026-08-28**. Quel kit è stato consumato, ha prodotto
[`combat-skillgrammar-delta-spec-panel-2026-08-28.md`](combat-skillgrammar-delta-spec-panel-2026-08-28.md), e
si è chiuso con **[D-238]**: *«la formula additiva del danno NON si congela»*, per due ragioni misurate —
`Armor` e cinque termini fratelli hanno **zero occorrenze** in `Source/`, e `Armor` collide con il
`BaseShield = 5` di **[D-224]** sullo **stesso asse** (entrambi *«solo danno Direct»*), per un **40%** di
bilanciamento preso per omissione.

**Le due ragioni sono state rimisurate oggi e reggono entrambe.** `git grep -c Armor -- Source` continua a
dare **0 file**; `D-224` non è stata superata.

Il §11.11 del kit chiede *«formula numerica definitiva del damage model se non già congelata nel Decision
Log»*. **È già decisa la non-congelazione**, che è essa stessa una decisione. Riproporre la formula senza
citare `D-238` non è un delta: è la stessa domanda posta a un log che ha già risposto.

Resta requisito nuovo e verificabile solo questo: il §11.5 (*Reactive Environment* vs *Propagation*) e il
§11.4 (policy di ricorsione same-boundary).

### 🔨 ADZIC — falsificabilità

> «Il vostro scenario A è già verde. Non l'avete scritto voi: lo scrive l'ordine delle fasi.»

Lo scenario **A — Wall Break → LOS → Overwatch** è presentato come il caso che giustifica i dieci layer.
Misurato: il repository lo soddisfa **già**, e non con un layer di refresh ma con la **macro-fase**.

```text
Blast   → ApplyEnvironmentChanges  (RTTurnManager_Blast.cpp:1356)  il muro cade QUI, a colpi risolti
Move    → ResolveReactionBoundary  (RTTurnManager.cpp:5550)        l'Overwatch valuta QUI
```

Il muro cade in coda al Blast; l'Overwatch gira nel Move, che viene dopo. Un Overwatch armato **vede già** il
muro caduto, e il commento del resolver dichiara la policy per esteso:

> *«Chi ha sparato in questo Blast **non guadagna la linea** perché il muro è caduto: la vista e il grafo si
> riaprono **dalla fase successiva**, e l'ordine dei colpi non cambia l'esito (invariante #3).»*

🔴 **E qui il kit e l'owner divergono, senza che il kit lo sappia.** Il §11.6 propone
*«Structure mutation → Atomic Commit → Refresh derived state → **later** Movement / Trigger Detection /
Reaction layers»* dentro lo stesso boundary. L'owner dice: il refresh vale **dalla fase successiva**, e
**dentro** il Blast nessuno guadagna la linea. Le due regole danno lo stesso esito sullo scenario A — perché
Move viene dopo Blast — e **esiti diversi** su uno scenario che il kit non scrive: due attaccanti nello stesso
Blast, il primo abbatte il muro, il secondo spara. Col kit il secondo colpisce; con l'owner no.

**Non è una lacuna del kit da colmare: è una policy già decisa che il kit rovescerebbe in silenzio.** Va
trattata come conflitto, non come specifica mancante.

### ⚔️ FOWLER — confini e nomi

> «State proponendo un quinto vocabolario di fase. Ne avete già quattro, e tre hanno un contratto scritto.»

| Vocabolario | Dove | Che domanda risponde | Stato |
|---|---|---|---|
| `ERTMatchPhase` | `RTTurnRules.h:10` | **Quando** risolve, davvero | 7 valori, implementato |
| `ERTResolutionPhase` | `RTActionDef.h:21` | Quale **riga del catalogo** (codici `0…60`) | 8 valori, implementato, con `MapResolutionPhase` come ponte |
| `ERTReactionPassPoint` | `RTReactionLibrary.h:36` | **Dove** un trigger può essere valutato | 6 valori, implementato ([D-092]) |
| `ERTPredictionBoundary` | `RTActionDef.h:258` | A quale punto una **previsione** si verifica | 2 valori, implementato (#226 chiusa) |
| *ResolutionLayer* (§11.2) | — | proposta | **10 nomi, zero occorrenze** |

I dieci layer proposti non sono ortogonali ai quattro esistenti: `Structural/Topology` è
`ApplyEnvironmentChanges`, `Movement/Occupancy` è la macro-fase `Move`, `Reaction Resolution` è
`RunReactionPass`, `Environment Propagation/Cleanup` è la macro-fase `Cleanup`. **Sei dei dieci hanno già un
proprietario.** Introdurre l'enum senza mappare i sei significa costruire il secondo modello che
`CLAUDE.md` §3 vieta — e che #1577 nomina per primo: *«costruire i rami prima dei boundary significherebbe
inventarne un secondo modello, che poi va riconciliato»*.

### 🧭 COCKBURN — chi decide, e quando

> «Il §11.3 chiede di risolvere N contributi. L'owner dice già come, e dice anche che una parte non serve.»

`spec-sequenza-turno.md` §2.3 ha già la regola per i contributi simultanei allo stesso micro-step:

> *«trigger simultanei nello stesso micro-step producono **una sola** opportunity multi-bersaglio, mai prompt
> in sequenza (l'ordine di iterazione non deve poter decidere)»*

E §3.1 ha l'ordine totale a cinque chiavi, verificato da `Actions.PermutationInvariant`. Il *collect →
validate → resolve → atomic commit* del §9 del work order è la disciplina che il resolver chiama
**«raccogli poi applica»**, dichiarata come **invariante #3** e ripetuta in ogni pass.

⚠️ **Ma il §3.2 dello stesso owner dichiara un buco, e il kit ci passa sopra senza vederlo.** L'ordine APNAP a
sei gruppi (*sistema → unità attiva → alleati → avversari → terreno → globali*) è **deciso in
`piano-canonico-mvp.md` §5.1 come `FR-RESOLVE-01..03` e non implementato**. L'owner lo dice con la data:

> *«Verificato il 2026-08-08: i sei gruppi non esistono nel codice. […] Quando E14 porterà reazioni che
> modificano il danno prima che venga applicato, o si costruiscono i gruppi o si dichiara che §3.1 basta e
> §5.1 del canone va riscritto.»*

**Questa è la domanda vera del DQA-027**, ed è più vecchia e più precisa di quella che il kit pone. Il kit
chiede *«come gestire N contributi»*; l'owner chiede *«i sei gruppi normativi valgono ancora, o §3.1 basta?»*
— e ha già nominato l'evento che forzerà la risposta.

### 🛡️ NYGARD — cosa si rompe

> «Il vostro §11.2 elenca dieci layer. Uno di essi è nella colonna *non costruire* di un owner spedito.»

`spec-sequenza-turno.md` §4 separa **corrente/deciso** da **north-star — non costruire**. A destra ci sono:
*stack di reazioni LIFO interattivo*, *interrupt annidati*, *5 categorie di velocità*
(`Immediate/Reaction/Fast/Standard/Slow`) e `EndOfPhase`. E `spec-sequenza-turno.md` §2.3 chiude con:
**«nessun interrupt annidato nella v0.1»**.

Il layer **1 — `Pre-Reaction / Interrupt`** del kit è esattamente la voce che quell'owner vieta. Il §11.4 del
kit lo sa a metà — *«per v0.1 evitare catene ricorsive illimitate»* — ma lo formula come **prudenza**, mentre
nell'owner è una **decisione con un motivo**: *«ogni suo pezzo aumenta la superficie di non-determinismo»*.

⛔ **La differenza non è di tono.** «Evitare per ora» autorizza chi implementa a introdurre il layer e
limitarne la profondità; «non costruire» no.

### 🎲 TALEB — il rischio che il kit non nomina

> «Avete un ID conteso, e il conflitto è già in produzione.»

`RTCombatLibrary.h:30` dichiara che `DamageType` *«arriva con **E49**»*. Il commento è del **2026-08-28**
(`6a1c0cb6`). L'epic **E49** su GitHub è [#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)
**Tactical Camera & Map Presentation**, creata il **2026-08-30** dal referto
[`tactical-camera-consolidamento-spec-panel-2026-08-30.md`](tactical-camera-consolidamento-spec-panel-2026-08-30.md),
che misurò *«Epic camera esistenti: **0**»* — corretto per le epic, cieco al commento nel codice.

🔴 **Il puntatore non è rotto: punta a qualcosa di sbagliato.** Chi cerca `E49` per capire dove nasce
`DamageType` atterra sulla camera, e nulla lo avverte. È una collisione della stessa famiglia di quelle che il
Decision Log registra sui `D-nnn`, e la prima su un `Enn`.

### 🌐 MEADOWS — la struttura sotto le quattro domande

Le quattro DQA sembrano quattro domande. Misurate, sono **una domanda e tre risposte già date**:

- **DQA-028** ha una risposta di tre giorni fa (`D-238`), e i suoi ingressi continuano a non esistere;
- **DQA-029** ha una risposta in `spec-sequenza-turno.md` §3.3 e un test che la esercita;
- **DQA-026** ha quattro vocabolari implementati e un quinto proposto che ne duplica sei valori su dieci;
- **DQA-027** è l'unica viva — ma la sua forma utile non è quella del kit, è il **§3.2** dell'owner: *i sei
  gruppi APNAP valgono, o §3.1 basta?*

Il pattern è quello che il progetto ha già pagato più volte: **un kit misura sé stesso e non l'owner**, e
propone come lacuna ciò che è una decisione. Il §11 lo dichiara onestamente in testa
(*«PROPOSED DESIGN da verificare contro repository HEAD»*) — questo referto è quella verifica.

### ✏️ DOUMONT — cosa dire a chi implementa

Una riga per ciascuna:

- **Non** introdurre `ERTResolutionLayer`. Quattro enum coprono già il dominio; il quinto duplicherebbe sei valori.
- **Non** congelare la formula del danno: `D-238` è la decisione corrente, e le sue due ragioni reggono ancora.
- **Non** introdurre il layer `Pre-Reaction / Interrupt`: `spec-sequenza-turno.md` §4 lo vieta per nome.
- **Sì** al §11.5: la distinzione reattivo/propagazione esiste nel codice e **non** in nessun documento.
- **Sì** al conflitto `E49`, che è un difetto concreto e piccolo.

---

## 4. Le divergenze, per esteso

### 4.1 Refresh derivato: fase successiva, non stesso boundary

| Fonte | Regola | Stato |
|---|---|---|
| Owner (`RTTurnManager_Blast.cpp:1366`, CP 9.2) | La struttura cade **a colpi risolti**; vista e grafo si riaprono **dalla fase successiva** | Implementato, con commento normativo |
| Kit §11.6 | Refresh del derived state **dentro** il boundary, prima dei layer successivi | Proposta |

Coincidono sullo scenario A. Divergono su *«due attaccanti nello stesso Blast, il primo abbatte il muro»* —
uno scenario che il kit non scrive e l'owner ha già deciso.

### 4.2 Ricorsione same-boundary: prudenza contro divieto

| Fonte | Regola |
|---|---|
| Owner (`spec-sequenza-turno.md` §2.3, §4) | *«nessun interrupt annidato nella v0.1»*; stack LIFO e interrupt annidati sono **north-star, non costruire** |
| Kit §11.4 | *«per v0.1 evitare catene ricorsive illimitate […] salvo regola esplicita di propagazione immediata»* |

La clausola *«salvo regola esplicita»* è una porta che l'owner tiene chiusa.

### 4.3 `E49`: un ID, due significati

| Uso | Dove | Data |
|---|---|---|
| Damage Model / `DamageType` | `RTCombatLibrary.h:30`, PRD `…DamageModel_E49_2026-08-27.md` | 2026-08-27 / 28 |
| Tactical Camera & Map Presentation | [#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769) | 2026-08-30 |

---

## 5. Ciò che il kit porta e l'owner non ha

**Uno solo, e va scritto.** Il §11.5 distingue:

- **Reactive Environment** — durante la Resolution: `OnEnter`, superfici che nascono, hazard, cambi di
  topologia. Nel runtime **esiste**: `ERTReactionPassPoint::CleanupSurfaceBirth` è *«l'unico punto fuori dal
  Blast»*, e `ApplyTerrainOnEnterEffects` applica gli effetti d'ingresso.
- **Environment Propagation / Cleanup** — `TickDynamicSurfaces`, `TickDynamicArcs`, `TickDynamicCovers`,
  durate, `Burning`.

⚠️ **Nessun documento owner nomina la distinzione.** `spec-sequenza-turno.md` §1 elenca i segmenti e non
distingue i due ruoli dell'Environment; chi legge la sequenza deduce che l'Environment sia solo Cleanup — che
è ciò che il kit contesta, **avendo ragione sul documento e torto sul runtime**.

Questo è **OBSERVED RUNTIME BEHAVIOR** da ratificare, non una decisione da prendere.

---

## 6. Output richiesti dal §25

### A. Repository baseline

```text
Branch di lavoro   docs/resolution-ordering-consolidamento (da origin/main)
Base               origin/main @ 0c0ee87c
Working tree       pulito salvo DA_HexMap_Arena.uasset (binario di altra sessione, NON toccato)
Sessioni parallele 5 worktree attivi
```

**Owner rilevanti**: `docs/gameplay/spec-sequenza-turno.md` (sequenza, boundary, APNAP, SBA) ·
`docs/decisions/RT_PDR_00_Decision_Log.md` ([D-092], [D-093], [D-197], [D-209], [D-224], [D-238]) ·
`docs/product/piano-canonico-mvp.md` §5.1 (`FR-RESOLVE-01..03`) ·
`docs/gameplay/spec-reaction-clash-e14.md` · `adr-0003` · `adr-0004`.

**Codice**: `RTTurnRules.h` · `RTActionDef.h` · `RTReactionLibrary.h` · `RTCombatLibrary.h` ·
`RTTurnManager.cpp` · `RTTurnManager_Blast.cpp` · `RTActionQueueLibrary`.

**Test**: 6 di permutazione · `Match.Autobattle.SimultaneousKOFollowsDeclaredPolicy` ·
`TurnLog.GraphRevisionEntersTheHash` · `Predictive.ResolvesAtDeclaredBoundary`.

### B. DQA reconciliation matrix

| DQA | Stato | Owner corrente | Evidenza | Azione |
|---|---|---|---|---|
| **026** — ResolutionLayer / causal ordering | **PARTIALLY CANONICAL** + **CONFLICT** sul refresh | `spec-sequenza-turno.md` §2.1/§3.1; `ERTMatchPhase`, `ERTReactionPassPoint`, `ERTPredictionBoundary` | 4 vocabolari implementati; ordine a 5 chiavi verde; policy refresh opposta a §11.6 (`RTTurnManager_Blast.cpp:1366`) | **Non promuovere l'enum.** Registrare il conflitto §4.1 |
| **027** — same-boundary multi-contribution | **OPEN DECISION**, ma riformulata | `spec-sequenza-turno.md` §3.2 + `piano-canonico-mvp.md` §5.1 | APNAP a sei gruppi **deciso e non implementato** dal 2026-08-08; §2.3 già impone una sola opportunity per micro-step | **Riformulare** sulla domanda dell'owner, non su quella del kit |
| **028** — DamagePacket / mitigation | **SUPERSEDED** da [D-238] | [D-238] (2026-08-28) | `Armor`/`DamageType`/`DamageResistance`/`DamagePacket`/`Shred` = **0 file**; collisione `Armor` ↔ `BaseShield` [D-224] | **Nessuna azione.** Il kit ripropone una domanda già chiusa |
| **029** — same-layer KO | **ALREADY CANONICAL** | `spec-sequenza-turno.md` §3.3 (`FR-RESOLVE-02`), [D-197] | *«morte a HP ≤ 0 […] fra un effetto e il successivo; un bersaglio morto invalida gli effetti pendenti che lo riguardano. Vale già per il batch del Blast»* + test | **Nessuna azione.** Ratificato e testato |

### C. GitHub consolidation matrix

| Deliverable | Existing issue | Decision |
|---|---|---|
| Resolution ordering / boundary dichiarato | #1577 (CP 33.1, `EvaluationBoundary`, v0.3) · #226 (chiusa) | **LINK** — #1577 possiede lo scope; è **v0.3 e dichiarata «tracciata, non pronta»** |
| Micro-step indirizzabile / pausa al boundary | #1880 · #1879 | **REUSE** — l'epic #1881 possiede l'ispezione del micro-step |
| Same-boundary reactions / contributi multipli | #314 (CP 14.7, Reaction Clash) | **LINK** — possiede l'opportunity contested; **non** l'ordine APNAP |
| Ordine APNAP a sei gruppi | **nessuna** | **CREATE** → [#1897](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1897), parent #152 — è il solo gap con owner, deliverable e data (§3.2, 2026-08-08) |
| Damage pipeline / `DamagePacket` | **nessuna**, e [D-238] dichiara *«nessuna issue è stata creata»* | **DEFER** — invariato dal 2026-08-28 |
| Same-layer KO | #1473 (curatore che cade) | **REUSE** — la semantica è chiusa, resta un difetto puntuale |
| Environment checkpoints | **nessuna** | **NON CREATA** — la ratifica è documentale e sta in questo pass (`spec-sequenza-turno.md` §1.2). Una issue avrebbe avuto per DoD un paragrafo già scritto |
| `E49` conteso | **nessuna** | **CREATE** → [#1898](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1898) — difetto concreto e piccolo |

### D. Documentation changes

| Documento | Stato | Perché |
|---|---|---|
| `docs/roadmap/plans/resolution-ordering-consolidamento-spec-panel-2026-08-31.md` | **UPDATED** (nuovo) | Questo referto |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | **UPDATED** | `D-291` — riconciliazione delle quattro DQA |
| `docs/gameplay/spec-sequenza-turno.md` | **UPDATED** | §11.5 ratificato: la distinzione reattivo/propagazione è OBSERVED RUNTIME BEHAVIOR e non era scritta |
| `docs/OPEN_DECISIONS.md` | **NOT UPDATED** | La domanda viva (APNAP) ha un owner che la dichiara già (§3.2) e diventa una issue: duplicarla qui creerebbe la terza copia |
| `docs/roadmap/roadmap-v0.1.md` · `roadmap-checkpoint.md` | **NOT UPDATED** | Nessuno scope di release cambia. Modificarli sarebbe simmetria |
| `docs/product/piano-canonico-mvp.md` | **NOT UPDATED** | `FR-RESOLVE-01..03` restano come sono: la domanda è *se* valgano, e la risposta non è di questo pass |
| `docs/research/prd/prd-damage-model-armor-shield.md` | **NOT UPDATED** | Livello 8, non owner; `D-238` lo governa già |
| ADR-0003 / ADR-0004 | **NOT UPDATED** | Nessuna delle quattro DQA tocca il modello azioni o le finestre di reazione |

### E. Issue changes

- **Created**: **due** — [#1897](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1897) l'ordine APNAP (parent #152, l'epic che §3.2 nomina) e [#1898](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1898) il conflitto `E49`.
- **Updated**: nessuna. #1577, #314 e #1881 sono corrette come sono; il referto le **linka**, non le riscrive.
- **Reopened**: nessuna.
- **Deliberately NOT created**:
  - *Environment checkpoints* — la ratifica era un paragrafo, ed è in questo pass (§1.2 dell'owner). Una issue
    avrebbe avuto per Definition of Done un testo già scritto;
  - *Resolution Layer enum* — duplicherebbe sei valori su dieci di quattro enum esistenti;
  - *Combo System* — il §17 del work order lo vieta e il repository non mostra quell'ownership;
  - *Damage pipeline / `DamagePacket`* — `D-238` è la decisione corrente e le sue ragioni reggono;
  - *Same-layer KO* — già canonico e testato.

### F. Remaining human decisions

Due, e **solo** due.

1. **I sei gruppi APNAP valgono ancora?** L'owner (`spec-sequenza-turno.md` §3.2) pone la domanda dal
   2026-08-08 e nomina l'evento che la forza (E14, reazioni che modificano il danno prima dell'applicazione).
   Le opzioni sono già scritte: *costruire i gruppi* **oppure** *dichiarare che §3.1 basta e riscrivere
   `piano-canonico-mvp.md` §5.1*. **Non è risolvibile per inferenza**: è una scelta di modello.
2. **Il refresh derivato vale dalla fase successiva o dentro il boundary?** (§4.1). L'owner ha una policy
   implementata e commentata; il kit ne propone un'altra. Coincidono sullo scenario A e divergono su
   *«due attaccanti, il primo abbatte il muro»*. **Cambiare la policy è un cambio di gioco**, non una pulizia.

### G. Recommended canonical flow

Il canone realmente trovato — nessun layer inventato, ogni riga ha un proprietario nel codice:

```text
Planning
→ Ready / countdown annullabile 3 s
→ LockInAndResolve
→ ValidatePlansAtLockIn                          validazione autoritativa
→ Snapshot                                       ERTResolutionPhase::Snapshot (codice 0)
→ Resolution — macro-fasi ERTMatchPhase, ordine fisso
   → Prep      ResolvePrep
   → Dash      ResolveDash
   → Blast     ResolveCombat → ResolveCombatPasses
                  ├ raccogli (invariante #3: mai applicare durante la raccolta)
                  ├ RunReactionPass(BlastHits)            ERTReactionPassPoint
                  ├ RunReactionPass(BlastDisplacement)
                  ├ RunReactionPass(BlastStatus)
                  ├ ResolveInterceptions                  (BlastIntercept, ciclo proprio)
                  ├ applica insieme
                  └ ApplyEnvironmentChanges               strutture: a colpi risolti
                        └ vista e grafo si riaprono DALLA FASE SUCCESSIVA
   → Move      ResolvePredictiveBoundary                  ERTPredictionBoundary::MovementEntry
                  └ ResolveReactionBoundary               Overwatch: opportunity → commit
                        AllowedResponses ≤ 1 → commit immediato
                        AllowedResponses ≥ 2 → finestra 3,0 s, Timeout → HOLD
               ResolveMovement
   → Cleanup   ResolveEnvironment                         superfici nascono QUI
                  └ RunReactionPass(CleanupSurfaceBirth)  l'unico pass fuori dal Blast
               effetti ambientali (Burning) → KO → obiettivi → cooldown → Tick durate
               DestroyDefeatedUnits
→ ConcludeTurn → CaptureFinalStateHash → RecordTurnToReplay
→ ERTMatchPhase::MatchEnded ? → Planning successivo
```

**Ordine dei contributi dentro una fase** (`URTActionQueueLibrary::InstanceLess`, implementato):

```text
MacroPhase → Priority (intera) → ActionId (lessicale) → SourceUnitId → EventSequence
```

**Ordine degli effetti simultanei** (APNAP, `FR-RESOLVE-01..03`): **deciso, non implementato** — §F.1.

**State-Based Actions** (`FR-RESOLVE-02`): morte a HP ≤ 0 e scadenze si controllano fra un effetto e il
successivo; un bersaglio morto invalida gli effetti pendenti che lo riguardano.

### H. Next executable issue

> **[#1897](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1897) — l'ordine APNAP a sei gruppi:
> decidere se `FR-RESOLVE-01..03` valgono, e chiudere il buco dichiarato dal 2026-08-08.**

È l'unica candidata che soddisfa tutti e cinque i criteri del §16 del work order: il deliverable è concreto (o
i gruppi esistono, o §5.1 del canone viene riscritto), nessuna issue OPEN lo possiede, nessuna CLOSED va
riaperta, non è documentazione duplicata, e ha un parent owner corretto (`spec-sequenza-turno.md` §3.2).

⚠️ **Non è #1577**: quella è release **v0.3** e porta il vincolo *«non si apre prima dei 15 gate della
v0.1»*, con i gate a **7 ✅ · 4 🟡 · 3 ⏳** — lavorarla ora violerebbe un vincolo mai revocato.

⚠️ **E non è una implementazione a scatola chiusa**: il primo passo della issue è la **decisione** §F.1, che è
umana. La issue la porta come domanda con due opzioni misurate, non come lavoro già assegnato.

---

## 7. NOT RUN

- **Nessuna suite eseguita.** Questo pass non tocca `Source/`: `./scripts/rt-suite.ps1` non è stato lanciato,
  e nessun numero di test è dichiarato verde da questo referto.
- **Nessuna verifica PIE o packaged.**
- Le misure `git grep` a zero (`Armor`, `DamagePacket`, `DamageResistance`, `Shred`, `EvaluationBoundary`)
  **scadono** al primo merge che le introduca. Sono state **rimisurate su `6f4e5edc`** — `origin/main` si è
  mosso di due commit durante il pass, e uno di essi tocca `RTCombatLibrary.h` — e reggono tutte.
  ⚠️ Quella rimisura ha **corretto una riga di questo referto**: `DamageType` non è a zero, ha una
  occorrenza che è il commento con cui [D-224] ne dichiara l'assenza (§2).
