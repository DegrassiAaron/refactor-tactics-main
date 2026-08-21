# Conflict report — handoff «Wait, Guard, Brace, Overwatch, Facing e geometria Hex/Wall»

> `CURRENT` · **Data**: 2026-08-10 · **Modo**: `/sc:spec-panel --mode critique --focus requirements,architecture,testing`
> **Sorgente**: [`2026-08-10-wait-guard-brace-overwatch-e-geometria.md`](../../archive/src/handoff/2026-08-10-wait-guard-brace-overwatch-e-geometria.md) — 34 sezioni, archiviato dopo il consumo
> **Owner di una sola domanda**: quali delle 34 sezioni cambiano il canone, e quali il canone aveva già.
>
> Questo file non è autorità. È il **verbale del triage**: le decisioni che ne escono vivono nel
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), gli stati nel
> Feature Registry, le domande aperte in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

## 0. Il risultato in una riga

Le 34 sezioni si scompongono in **45 righe di triage** (§2). Contate lì:

```
18  ALIGNED     il canone lo dice già
 7  PROC        procedura per l'agente, nessun contenuto
 7  CONFLICT    contraddice il canone
 6  NEW         proposta con contenuto
 5  DUP         ha già un owner
 2  misto       (§21 test, §30 domande aperte)
```

Il contributo reale dell'handoff è **uno, ed è grosso**: il modello spaziale
`Cell Anchor + Unit Footprint + Clearance` con la separazione fra *node validity* e *transition validity*.
Il secondo, più piccolo e più rischioso, è la **ripulitura del confine Guard/Brace** (C6). Tutto il resto su
Wait/Brace/Overwatch era già canonico da due giorni o più.

## 1. Gerarchia delle fonti — perché questo handoff prevale, e su cosa

L'handoff dichiara una regola di prevalenza (§, righe 7-13) che mette le decisioni della propria chat sopra il
repository. **Non si applica così**, ed è un punto di metodo che vale la pena fissare perché ricorre:
`AGENTS.md` e `CLAUDE.md` §4 dicono che *«un handoff/audit non è autorità e non autorizza da solo a
implementare tutto ciò che contiene»*. La gerarchia effettiva usata qui è:

```
ADR accettati  >  Decision Log  >  spec/brief owner  >  handoff non consumato
```

Con un'eccezione dichiarata: un handoff **più recente** può *proporre* di emendare un ADR, e allora si registra
come proposta con il suo ID — mai come modifica silenziosa.

### 1-bis. Tre sorgenti non consumati della stessa giornata

Al momento del triage `docs/src/` conteneva **tre** handoff sovrapposti su Brace/Overwatch, tutti non consumati:

| Sorgente | Scritto su disco | Rapporto |
|---|---|---|
| `CLAUDE_Consolidamento_BaseAction_Signatures_Brace_Overwatch_2026-08-10.md` | 03:41 | Base Action Signature `STANDARD/VARIANT/SIGNATURE` per eroe |
| `CLAUDE_Overwatch_Runtime_Lifecycle_Watch_Reposition_Consolidation_2026-08-10.md` | 03:41 | Watch Window, Reposition, lifecycle nella Move Phase. **Si dichiara più recente** di handoff Overwatch precedenti |
| **questo** — `RefactorTactics_Claude_Handoff_Wait_Guard_Brace_Overwatch_Geometry.md` | **03:55** | Wait/Guard/Brace/Overwatch, Facing, geometria |

⚠️ **Il terzo è cronologicamente l'ultimo, ma non cita gli altri due e non li supera.** In particolare le
domande **§30.15** e **§30.16** — *«se alcune reaction persistono o terminano prima della fase Move»* e
*«lifecycle definitivo di Overwatch rispetto a ogni macrofase»* — sono **esattamente l'oggetto** del secondo
handoff, che una risposta ce l'ha. Registrarle qui come «aperte» significherebbe riaprire una domanda che un
sorgente in repository ha già affrontato. Sono marcate `DEFER-TO-SOURCE`, non `OPEN`.

## 2. Triage — le 34 sezioni

Legenda: `ALIGNED` il canone lo dice già · `DUP` ha già un owner · `CONFLICT` contraddice il canone ·
`NEW` proposta nuova · `PROC` procedura per l'agente.

| § | Tema | Esito | Dove vive già / dove va |
|---|---|---|---|
| 1 | Executive summary | `PROC` | — |
| 2 | Vocabolario | `ALIGNED` | `Prepared Reaction`, `Decision Window`, `HOLD`, `Legal Response` sono di [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md). `Cell Anchor`, `Clearance`, `Footprint` sono **nuovi** → §6 |
| 3.1-3.3 | Wait è un No-Op esplicito | `ALIGNED` | Sette generiche, [D-014](../../decisions/RT_PDR_00_Decision_Log.md)/[D-025](../../decisions/RT_PDR_00_Decision_Log.md). Il test `Actions.Wait.AllowsFacingAndReaction` pinna già «Wait non consuma lo slot di facing/reazione» |
| **3.4** | Reason code del Wait | `NEW` **parziale** | `ERTFallbackOutcome::Waited` **esiste** (`RTTurnLog.h:165`). Manca la distinzione *scelto* vs *timeout* vs *disconnesso* → **P5** |
| 4.1-4.2 | Guard mitiga il danno, baseline automatica | `ALIGNED` | Catalogo azioni: −15 e resistenza a 1 cella di spinta ([D-025](../../decisions/RT_PDR_00_Decision_Log.md)). «Automatica» ≡ `AllowedResponses ≤ 1` di ADR-0004 §2 — non serve una regola nuova |
| **4.3** | `FacingBinding = FollowCurrentFacing` | `NEW` | ADR-0005 §4a ha l'arco frontale, ma il *binding* non è mai stato nominato → **P6** |
| 4.4 | Cover e Guard sono sistemi distinti | `ALIGNED` | `FRTHexCover` è dato di mappa, `Status.Guarded` è stato di unità. Pinnato da `Combat.ShieldWorksFromAnyDirection` |
| **4.5** | Guard mitiga **solo** danno · Brace contesta **solo** displacement | **`CONFLICT`** | → **C6**. Nel codice **entrambi** fanno entrambe le cose |
| 5.1-5.2 | Brace è azione distinta, lifecycle | `ALIGNED` | Sette generiche + [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) |
| 5.3 | Riktor: Hold Ground / Shield Read / Pivot Step | `DUP` | [`spec-reaction-clash-e14.md`](../../gameplay/spec-reaction-clash-e14.md) §4 li ha come esempi della grammatica, con `Hold Ground` risposta **universale** |
| 5.4 | Facing policy `Preserve/FaceThreat/FaceMovement` | `DUP` | [ADR-0008](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) `FAC-2`: le policy si dichiarano sul dato. I tre nomi sono un'istanza, non un modello nuovo |
| 6.1-6.2 | Overwatch distinto, Planning + FIRE/HOLD | `ALIGNED` | [`brief-azioni-generiche-overwatch.md`](../../gameplay/brief-azioni-generiche-overwatch.md) §4, §6 |
| 6.3 | HOLD non ruota, non bonifica, non riposiziona | `ALIGNED` | ADR-0004: `Timeout → HOLD`, `HOLD` non consuma la charge |
| 6.4-6.5 | FIRE non concede rotazione gratuita; niente menu di ability | `ALIGNED` | `AllowedResponses` **è** l'insieme chiuso delle risposte legali (ADR-0004) |
| **7** | «Brace e Overwatch restano distinti» — decisione canonica da consolidare | `ALIGNED` | ⚠️ **Non c'era niente da consolidare.** Non sono mai stati fusi: sono due delle sette generiche da D-014, e nessun documento ha mai proposto un pulsante unico. La §7 risolve un problema che il repository non ha |
| 8.1 | Tre modalità `Automatic/Single/Contested` | `ALIGNED` | ADR-0004 §2 le **deriva** dalla cardinalità di `AllowedResponses`; contested da [D-047](../../decisions/RT_PDR_00_Decision_Log.md)/[D-048](../../decisions/RT_PDR_00_Decision_Log.md). Un enum esplicito sarebbe la «seconda verità» già respinta |
| 8.2-8.3 | Decision Boundary, Legal Response | `ALIGNED` | `CLAUDE.md` §5 · `RT-FEAT-CORE-DECISION-BOUNDARY` |
| 9.1-9.2 | Quando esiste un Clash · niente stack annidato | `ALIGNED` | ADR-0004 supera ADR-0003 solo sulla finestra singola; lo stack LIFO **resta scartato** |
| **9.3** | Grammatica `COMMIT/READ/SHIFT` | **`CONFLICT`** | → **C1** |
| 9.4 | REACT-010 Pressure Jet vs Brace | `DUP` | → **C3** e §5 |
| 9.5 | Scelta simultanea nascosta | `ALIGNED` | ADR-0004 §7 + §7-bis ([D-021](../../decisions/RT_PDR_00_Decision_Log.md)), emendata da D-048 per il contested |
| 10 | Agency del giocatore durante il turno | `ALIGNED` | `CLAUDE.md` §5, ADR-0004 |
| 11 | Action economy | `ALIGNED` **in parte** | `Attack \| Ability \| Overwatch` è [D-012](../../decisions/RT_PDR_00_Decision_Log.md). La matrice Sprint/Sneak per azione **non** è canonica — l'handoff lo dice da sé («non rendere canonica senza verificare») → resta aperta |
| 12.1 | Sei direzioni discrete | `ALIGNED` | `ERTHexDirection` esiste; ADR-0005 §1 (emendata da ADR-0008) |
| 12.2 | Il facing non è la geometria del muro | `ALIGNED` | ADR-0005 §4: arco frontale **unico**, indipendente dai bordi. `Combat.ShieldWorksFromAnyDirection` lo pinna |
| **13** | «Non assumere che un muro occupi un lato dell'hex» | **`CONFLICT`** | → **C2** e §4 |
| **14** | Anchor + Footprint + Transition | **`NEW`** | → **P1**, **P2**, **P3** |
| 15 | Case a 90° | `NEW` | Coperto da **P1**; già discusso il 2026-08-09 → §4 |
| 16 | Separare Pathfinding/LOS/Targeting/Cover/Facing | `ALIGNED` | `BlocksTraversal` è consultato **sia** da `GraphNeighbors` **sia** da `URTHexVisionLibrary` — la separazione esiste, e la condivisione del predicato è deliberata |
| 17 | Unit footprint e clearance | `NEW` | → **P4** (l'handoff stesso rinvia Small/Large) |
| 18 | Map editor / validator | `DUP` | `URTHexMapAsset::ValidateMap` + `RefactorTactics.HexMap.Validate*` — `RT-FEAT-TOOL-VALIDATION` **DONE**. Le voci **nuove** del validator sono in **P1/P3** |
| 19 | Render/visual explainer | `NEW` | → Wiki, §7 |
| 20 | Impatti UE5 | `CONFLICT` **parziale** | `FRTCellId` **è già** un identificatore logico; il resto è **C2** |
| 21 | Test da aggiungere | misto | → §5 e §6 |
| **22** | Quattro epic da creare | **`CONFLICT`** | → **C4** |
| 23 | Aggiornamenti roadmap | `PROC` | → §8 |
| 24 | Feature map | `PROC` | → §8. ⚠️ i nomi `Feature.Map.*` non sono la convenzione: è `RT-FEAT-<AREA>-<NOME>` |
| **25** | Scenario map `GEO-001`, `REACT-010`… | **`CONFLICT`** | → **C3** |
| 26 | Editor map | `DUP` | `editormap.shortlist.md` + [`editor-sessions.yaml`](../editor-sessions.yaml) |
| 27 | Wiki | `PROC` | → §7 |
| 28 | Documenti da consolidare | `PROC` | → §8 |
| **29** | Tre ADR da creare | **`CONFLICT`** | → **C5** |
| 30 | Domande aperte | misto | → §9 |
| 31-34 | Procedura, DoD, commit, risultato atteso | `PROC` | — |

## 3. I `CONFLICT`

### C1 — `COMMIT` contro `STAND`: il repository ha già rifiutato questo nome, e ha scritto perché

L'handoff §9.3 propone `READ > COMMIT`, `COMMIT > SHIFT`, `SHIFT > READ`.
[`spec-reaction-clash-e14.md`](../../gameplay/spec-reaction-clash-e14.md) §4 ha **la stessa relazione ciclica**
con un nome diverso — `READ > STAND > SHIFT > READ` ([D-049](../../decisions/RT_PDR_00_Decision_Log.md)) — e
la §4.2 spiega la scelta: *«Commit» è già il nome del secondo tempo della finestra di reazione
(`Opportunity → Commit`)*. Usare la stessa parola per un'intenzione di grammatica rende scrivibile la frase
«il commit del COMMIT».

**Esito: la semantica dell'handoff è accettata perché è già in vigore; il nome è respinto.** Non è una
questione di gusto — `Commit` compila già in `CLAUDE.md` §5 come nome di una fase.

> 💬 **DOUMONT**: «Due parole per due concetti, non una parola per due. Il costo di un omonimo non si paga
> quando lo scrivi, si paga ogni volta che qualcuno lo legge e deve decidere quale dei due intendevi.»

### C2 — La geometria autorevole a runtime contro `replay divergence = 0`

Le §14, §15 e §20 chiedono che *standability* e *transition validity* derivino dall'intersezione fra footprint
e blocking geometry. Se questa derivazione avviene **a runtime**, ogni query di transizione interseca geometria
con estremi float, dentro un hash che oggi tiene fermo il KPI *replay divergence = 0*
([`hex-map-roadmap.md`](../hex-map-roadmap.md): *no float in coord/hash*, *dati indipendenti da Actor/mesh*;
**E23.1**: *«la logica di transizione non legge la mesh: legge archi e stati»*).

**Esito: ammissibile solo come authoring che cuoce dati.** È la stessa sintesi che il panel del 2026-08-09
aveva già raggiunto su un brief diverso — vedi §4.

### C3 — La sesta convenzione di ID

Le §21 e §25 propongono `GEO-001`, `GEO-002`, `REACT-010`, `OW-001`, `ACTION-004`, `FACE-001`.
La convenzione reale del corpus è **`Spec.<Area>.<Nome>`** — misurata sui file: `Spec.Brace.ProfileChangesResponse`,
`Spec.Clash.StandBeatsShift`, `Spec.Facing.BackAttackIgnoresGuard`.

Il precedente è esplicito: il triage del 2026-08-09 ha già respinto `MAP-ED-001` scrivendo *«cinque convenzioni
di ID diverse in un pacchetto sono già costate un audit a questo repository; questa sarebbe la sesta»*.
Questa sarebbe la settima.

**Esito: gli ID si traducono.** La tabella di traduzione è in §5.

### C4 — Quattro epic nuove, quattro owner esistenti

| L'handoff §22 chiede | Esiste come | Stato |
|---|---|---|
| EPIC — Common Actions & Prepared Reactions | **E14** — Overwatch e reazioni interattive ([#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152)) | v0.1, aperta |
| EPIC — Facing & Directional Combat | **E16** — Orientamento e direzionalità ([#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175)) | **chiusa**; l'estensione vive in ADR-0008 (E11/E16) |
| EPIC — Geometry-derived Tactical Graph | **E23** — Muri, porte e interaction graph ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)) | v0.2, aperta |
| EPIC — Map Editor Geometry Validation | `RT-FEAT-TOOL-MAP-EDITOR` (**TESTABLE**) + `RT-FEAT-TOOL-VALIDATION` (**DONE**) | M9 |

La numerazione delle epic è **unica e condivisa** e si assegna **al merge**
([D-039](../../decisions/RT_PDR_00_Decision_Log.md)).

**Esito: nessuna epic nuova.** Si estende il perimetro di **E23** e si annota **E14**. Vedi §8.

> 📚 **COCKBURN**: «Quattro epic per quattro cose che hanno già un attore e un obiettivo scritti altrove non
> aggiungono capacità: aggiungono un secondo posto in cui cercare chi è responsabile.»

### C5 — Tre ADR, di cui due già scritti

| L'handoff §29 chiede | Esito |
|---|---|
| ADR — *Brace and Overwatch remain distinct universal actions* | **Non serve.** Non sono mai stati fusi (D-014/D-025). Un ADR che ratifica lo status quo aggiunge un documento e zero informazione |
| ADR — *Facing uses six tactical directions independent of wall orientation* | **Già scritto due volte**: [ADR-0005](../../decisions/adr-0005-orientamento.md) §4 e [ADR-0008](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) |
| ADR — *Hex grid does not constrain world wall geometry* | **Questo sì.** È l'unico dei tre con contenuto nuovo, ed è la decisione che risolve C2 |

### C6 — «Guard mitiga, Brace contesta»: una separazione che il codice non ha

L'handoff §4.5 propone un confine netto, con un esempio esplicito:

```text
Pressure Jet: Damage + Push
Guard:  mitiga il danno · Push invariato
Brace:  danno invariato · contesta il Push
```

**Misurato sul branch corrente, non è così: entrambi fanno entrambe le cose, e differiscono per *forma*.**

| | Danno | Spinta |
|---|---|---|
| **`Guard`** | **−15** sul **primo** danno diretto (`RT_ActionCatalog_v0.1.md` §135) | resiste a **1 cella** |
| **`Brace`** | **−10** su **ogni** danno diretto (`URTCombatLibrary::BraceDamageReduction`) | blocca la **prima** spinta, **senza limite di distanza** (`TAG_Status_Braced` in `RTTurnManager`) |

Il codice è consapevole della distinzione e la commenta: *«distingue da Guard, che regge solo un passo»*.
Non è un residuo — è un asse di design a due dimensioni (*primo colpo forte* vs *ogni colpo, più a lungo*;
*spinta corta* vs *spinta qualsiasi*), coerente con `PushResistance` come **soglia** e non sottrazione
([D-038](../../decisions/RT_PDR_00_Decision_Log.md)).

**Esito: la proposta è legittima ma non è un consolidamento — è un cambio di bilanciamento.** Applicarla
azzererebbe la mitigazione di `Brace` e l'anti-spinta di `Guard`, cioè quattro numeri già a catalogo e almeno
due scenari verdi (`Visual.Combat.GuardReducesFirstHit`, `Visual.Combat.PushResistance`). Va registrata come
domanda aperta di bilanciamento, non applicata da un consolidamento documentale.

> ✅ **WIEGERS**: «La §4.5 si presenta come chiarimento di identità e chiede invece una modifica di quattro
> valori. È la categoria di requisito più costosa: sembra gratis perché è scritta in prosa.»

> 🧪 **CRISPIN**: «Il test che deciderebbe la questione non esiste ancora, ed è quello che l'handoff descrive
> in §21: *mixed damage + push*. Oggi i due lati sono verificati separatamente. Finché restano separati,
> nessuno si accorge se il confine si sposta.»

## 4. La domanda dei muri — seconda volta, stessa risposta

Il triage del 2026-08-09 ([`map-editor-brief-spec-panel-2026-08-09.md`](map-editor-brief-spec-panel-2026-08-09.md) §4)
aveva già ricevuto questa proposta da un brief diverso, e aveva prodotto la sintesi:

```
authoring          cottura                     dato autorevole
─────────          ───────                     ───────────────
segmento           il tool decide una volta:   FRTHexCellData.bBlocksMovement
world-space   ──►  quale cella, quale bordo,  ──►  FRTHexCover{Edge, High}
(float)            quale effetto                   FRTHexDoor{Edge, State}
                                                   (interi, hashabili)
```

**Che l'handoff arrivi alla stessa conclusione da un'altra strada è la conferma più utile che contenga**: due
sorgenti indipendenti che chiedono la stessa cosa smettono di essere un'opinione.

### 4.1 Quello che l'handoff aggiunge davvero

Il brief del 2026-08-09 si fermava a *«il muro cuoce su bordi»*. L'handoff chiede una cosa che quella sintesi
**non copre**, ed è il punto tecnico più prezioso del documento:

> **`Cell A` valida ∧ `Cell B` valida ∧ `A→B` bloccata** non è la stessa cosa di «c'è una copertura alta sul
> bordo A|B».

Oggi il repository può *esprimere* il caso — `URTHexCoverLibrary::BlocksTraversal` è consultato da
`GraphNeighbors` — ma lo esprime **solo** attraverso una copertura. Un muro che taglia due celle senza essere
una copertura tattica (nessuna riduzione di danno, nessuna integrità, nessuna distruttibilità) oggi si modella
male: o diventa una copertura che non protegge, o diventa `bBlocksMovement` su una cella che invece è
perfettamente calpestabile.

E c'è un secondo buco: gli adiacenti orizzontali sono **calcolati**, non archi
([`spec-pathfinding-pf3-pf4.md`](../../technical/architecture/spec-pathfinding-pf3-pf4.md) §2, [D-013](../../decisions/RT_PDR_00_Decision_Log.md)).
`FRTHexEdge` esiste solo per i salti di layer. Quindi *non esiste oggi un dato su cui appendere «questa
transizione orizzontale è chiusa»* che non sia la copertura.

> 🏗️ **FOWLER**: «Il modello attuale accoppia due responsabilità sullo stesso campo: *cosa mi protegge* e
> *dove non posso passare*. Finché coincidono, l'accoppiamento è invisibile. Il muro non-copertura è il primo
> caso in cui divergono, ed è per questo che va scritto adesso — non perché il codice sia sbagliato oggi.»

> 🎲 **NYGARD**: «La cottura non è invertibile senza conservare il sorgente. Se qualcuno tocca a mano
> `bBlocksMovement` su una cella cotta, il prossimo ricalcolo cancella la modifica in silenzio. Va deciso,
> non scoperto in produzione.» *(già registrato nel triage del 2026-08-09; resta aperto)*

### 4.2 Quello che l'handoff sbaglia a chiamare «superato»

La §13 e la §28 chiedono di correggere la documentazione che afferma `wall == hex side`.
**Misurato: nessun documento canonico lo afferma.** L'unica riga che ci somiglia è
[`spec-mappa-multilivello.md`](../../technical/architecture/spec-mappa-multilivello.md) §4:

> *«Una copertura appartiene a **uno dei sei lati**, non alla cella»*

che dice una cosa **diversa e corretta**: è un'affermazione sul *dato autorevole*, non sulla *geometria del
mondo*. Il muro visivo può stare dove vuole; il suo effetto tattico si registra su un bordo perché la
direzionalità dev'essere esprimibile con interi.

**Esito: nessuna correzione, una precisazione.** La §4 di quel documento acquista una nota che separa i due
piani, così che la prossima lettura non riapra la domanda una terza volta.

> ✅ **WIEGERS**: «"Nessun documento canonico afferma X" è un requisito verificabile, ed è stato verificato:
> `grep -i` su `docs/` con otto formulazioni alternative (`lato dell'esagono`, `lato dell'hex`,
> `bordo dell'esagono`, `bordo esagon`, `wall … hex side`, `lati dell'hex`, `sei lati`, `per lato`).
> Fuori dall'handoff stesso il risultato è **una** riga di documento canonico, e non diceva X. Un
> consolidamento che l'avesse riscritta avrebbe *introdotto* il difetto che credeva di rimuovere.»

## 5. Scenari — cosa esiste già, cosa si traduce, cosa manca

**Quattro dei tredici scenari richiesti esistono già come specifica eseguibile.**

| L'handoff §25 chiede | Esiste come | Esito |
|---|---|---|
| `REACT-010` Reaction Clash 3×3 | `Spec.Clash.StandBeatsShift` · `.ReadBeatsStand` · `.ShiftBeatsRead` · `.TieAppliesOnce` | **DUP** — la matrice c'è, in 4 file |
| `FACE-001` Guard sectors | `Spec.Facing.FrontAttackKeepsGuard` · `.BackAttackIgnoresGuard` | **DUP** |
| `FACE-002` Facing changed by reaction | `Spec.Facing.DerivesFromMove` · `.DashReorients` · `.TargetingReorients` · `.BraceHoldsFromBehind` | **DUP** |
| `OW-002` Overwatch HOLD poi trigger | `Spec.Overwatch.HoldThenFire` | **DUP** |
| `ACTION-004` Guard vs Brace | — (`Visual/Combat/GuardReducesFirstHit`, `PushResistance` coprono i due lati **separatamente**) | **da scrivere**: `Spec.Brace.GuardMitigatesDamageBraceContestsPush` |
| `ACTION-WAIT-001` Wait e fallback | — | **da scrivere**: `Spec.Wait.ReasonDistinguishesChoiceFromFallback` (dipende da **P5**) |
| `OW-001` Overwatch single trigger | — | **da scrivere**: `Spec.Overwatch.SingleTriggerFires` |
| `OW-003` Target simultanei | — | **da scrivere**: `Spec.Overwatch.SimultaneousTargetsOneOpportunity` |
| `GEO-001` … `GEO-005` | — (`Spec.Map.BridgeBreaksThePath` è l'unico `Spec.Map.*`) | **da scrivere in E23/v0.2**: `Spec.Map.WallCrossesCellStillStandable` · `.FootprintCollisionBlocksCell` · `.ValidCellsBlockedTransition` · `.DoorOpensTransition` · `.NinetyDegreeCornerBakesCorrectly` |

> 🧪 **CRISPIN**: «I quattro che esistono non vanno riscritti con l'ID nuovo: vanno *citati*. Un secondo file
> che verifica la stessa cosa con un nome diverso è il modo più economico di far divergere due verità.»

> 📗 **ADZIC**: «`Spec.Clash.TieAppliesOnce` è il file che l'handoff non sapeva di aver chiesto: la §9.5 dice
> "nessun secondo round su tie" in prosa, e quel file lo esegue. È l'esempio di cosa significa avere la
> specifica come test invece che come paragrafo.»

## 6. Le sette proposte nuove

| # | Proposta | § | Collocazione |
|---|---|---|---|
| **P1** | Standability derivata da `Cell Anchor + Unit Footprint + Clearance`, in **cottura** | 14.1-14.3, 17 | **E23** (v0.2) |
| **P2** | *Node validity* ≠ *Transition validity* come invariante dichiarata, con un dato di transizione che non sia la copertura | 14.4 | **E23** (v0.2) — vedi §4.1 |
| **P3** | Swept clearance: la transizione verifica il corridoio, non solo gli estremi | 14.5 | **E23** (v0.2) |
| **P4** | Profili di clearance `Small/Standard/Large` | 17 | **future** — l'handoff stesso rinvia: MVP = `StandardUnitClearance` |
| **P5** | Reason code del Wait: *scelto* vs *timeout* vs *intento invalido* vs *disconnesso* | 3.4 | **E14** per la parte reazione; il timeout di Planning è di [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) |
| **P6** | `FacingBinding` esplicito su Guard, con `LockFacingWhenArmed` come estensione futura | 4.3 | **E16/E11** — estende ADR-0008 `FAC-2` |
| **P7** | Porte autorizzate da geometria non allineata ai bordi, che cuociono su `FRTHexDoor{Edge}` | 14.6 | **E23** (v0.2) — la porta canonica ha 4 stati, non 2 |

⚠️ **P7 contiene una regressione da non recepire**: l'handoff §14.6 e §21 parlano di porta `aperta/chiusa`.
`ERTHexDoorState` ha **quattro** stati — `Open, Closed, Locked, Destroyed` — e perderne due costerebbe
l'apertura autorizzata (CP 10.1) e la terminalità. È lo stesso difetto già rilevato come `C3` il 2026-08-09.

## 7. Wiki

L'handoff §27 chiede una pagina di alto livello **«Hex Grid vs World Geometry»**, ed è la richiesta più
sensata del documento: è l'unico posto in cui il modello spaziale diventa spiegabile a chi non legge gli ADR.

La Wiki è un **repository separato** (`refactor-tactics-main.wiki`, branch `master`, file flat): la pagina
nasce lì come `Meccanica-griglia-e-geometria.md`, con cross-link da `Meccanica-coperture`,
`Meccanica-porte`, `Meccanica-collisioni` e `Meccanica-facing-e-direzionalita`.

I quattro esempi illustrati chiesti dalla §19 restano **da produrre**: il render citato dall'handoff non è
allegato al sorgente e non esiste in `docs/src/media/`. La pagina li dichiara come segnaposto invece di
descrivere immagini che nessuno può vedere.

## 8. Aggiornamenti applicati

Vedi il report finale della sessione. In sintesi: precisazione a `spec-mappa-multilivello.md` §4 · voci
`RT-FEAT-MAP-STANDABILITY` e `RT-FEAT-MAP-TRANSITION-CLEARANCE` nel Feature Registry (stato `DESIGNED`, non
implementate) · perimetro di **E23** esteso con P1/P2/P3/P7 · nota su **E14** per P5 · scenari `planned:` ·
pagina Wiki · decisioni nel Decision Log · domande in `OPEN_DECISIONS.md`.

## 9. Le 17 domande della §30, filtrate

**Sette non erano aperte.**

| # | Domanda §30 | Esito |
|---|---|---|
| 1 | Action economy Brace/Overwatch/Guard | **aperta** — `Attack \| Ability \| Overwatch` è D-012; Guard e Brace no |
| 2 | Compatibilità con Sprint e Move profile | **aperta** — la matrice §11 non è canonica |
| 3 | Numeri di mitigazione del Guard | ✅ **decisa** — −15 e 1 cella di spinta, a catalogo (D-025) |
| 4 | Arco protetto per personaggio | **aperta** — l'arco *unico* è ADR-0005 §4a; la variazione per eroe no |
| 5 | Stacking Cover + Guard | **aperta** |
| — | *(aggiunta dal triage)* Guard e Brace devono separarsi in danno-vs-spinta? | **aperta** — **C6**, è bilanciamento |
| 6 | `COMMIT/READ/SHIFT` come grammatica finale | ✅ **già tracciata** — `CLASH-1`, con `STAND` al posto di `COMMIT` (C1) |
| 7 | Win/Tie/Lose numerici del Clash | ✅ **già tracciata** — `CLASH-2` |
| 8 | UX di destination/facing del Pivot | **aperta** |
| 9 | Clearance standard in metri | **aperta** — ma non da zero: `convenzioni-contenuti-ue.md` §11-bis fissa **lato dell'esagono ≈ 1,5 m** |
| 10 | Hex spacing/dimensioni finali | ✅ **decisa** — stessa riga di sopra |
| 11 | LOS quando il raggio sfiora un angolo | **aperta** |
| 12 | Projectile/cover sui corner case | **aperta** |
| 13 | Come bakeare il tactical graph | ✅ **risposta esiste** — §4, sintesi del 2026-08-09; l'esecuzione è E23 |
| 14 | Small/Standard/Large | **aperta** — P4, `future` |
| 15 | Reaction che persistono oltre il Move | ⚠️ `DEFER-TO-SOURCE` — §1-bis |
| 16 | Lifecycle di Overwatch per macrofase | ⚠️ `DEFER-TO-SOURCE` — §1-bis |
| 17 | Guard compete col Main Commitment | **aperta** — ≡ domanda 1 |

## 10. Il difetto di metodo che questo handoff illustra

Vale la pena scriverlo perché è il terzo sorgente consecutivo con la stessa forma.

Un handoff prodotto in chat **non sa cosa il repository ha deciso ieri**. Le sue sezioni si dividono in due
gruppi con un rapporto stabile: quelle che *ricostruiscono* una decisione già presa — corrette, inutili, e
costose perché sembrano richieste di lavoro — e quelle che *aggiungono* qualcosa. Qui il rapporto è 12 a 7.

Il segnale che distingue i due gruppi non è il tono: la §7 è scritta con la stessa enfasi della §14 e si
intitola «DECISIONE CANONICA DA CONSOLIDARE», ma non c'era niente da consolidare. **Il segnale è la
verificabilità**: la §14 nomina un caso che il repository non sa modellare (`Cell A` valida, `Cell B` valida,
`A→B` chiusa senza copertura); la §7 nomina un rischio che non si è mai materializzato.

> 🧭 **DRUCKER**: «La domanda giusta non è "questo documento è corretto?" — quasi sempre lo è. È "quale
> decisione cambia?". Dodici sezioni su trentaquattro non ne cambiavano nessuna.»
