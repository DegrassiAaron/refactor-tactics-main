# Triage — `RefactorTactics_Facing_Claude_Consolidation.md`

> `PLAN` · **Stato**: consumato · **Data**: 2026-08-10
> **Fonte**: [`../../archive/src/handoff/2026-08-10-facing-consolidation.md`](../../archive/src/handoff/2026-08-10-facing-consolidation.md) — 69 sezioni, 2 100 righe
> **Owner delle regole**: [ADR-0005](../../decisions/adr-0005-orientamento.md) e
> [ADR-0008](../../decisions/adr-0008-rotazione-e-policy-di-facing.md). Questo documento **non è autorità**:
> registra cosa della fonte è stato recepito, cosa era già canonico e cosa è stato respinto.

## Perché serve un triage e non un'applicazione

Il pacchetto si presenta come una lista di lavori da eseguire («Claude must: consolidate… create epics…»), ma
è un **handoff AI**, cioè l'ultima fonte della gerarchia documentale. Applicato alla lettera avrebbe:

- creato un'epic `Tactical Facing & Directional Interaction` **duplicata** di `E16`, che esiste ed è **chiusa**
  ([#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175), con CP 16.1 e CP 16.2);
- aperto 14 issue `FACING-001…014` di cui **la maggior parte descrive codice che esiste**;
- creato una pagina Wiki `Facing` che esiste già
  ([`facing-e-direzionalita.md`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/facing-e-direzionalita));
- spezzato `RT-FEAT-MAP-FACING` in **sedici** feature (§55), moltiplicando per sedici il posto in cui uno stato
  può divergere — il difetto che il feature registry esiste per impedire;
- **rovesciato due decisioni canoniche** senza dichiararlo, perché la fonte non sa che esistono.

Il valore della fonte non è nullo: **quattro domande sono nuove e non erano registrate da nessuna parte**.
Sono in fondo, ed è per quelle che il file è stato letto fino in fondo.

## Misura

| | Sezioni |
|---|---:|
| Già canoniche, nessuna azione | **38** |
| Già aperte e registrate, la fonte le riafferma senza aggiungere | 6 |
| Contraddicono il canone → l'ADR prevale, la proposta si registra | **3** |
| Genuinamente nuove → registrate come `FAC-11…FAC-14` | **4** |
| Procedurali (checklist, naming, report) — nessun contenuto | 18 |

---

## A. Contraddicono il canone (l'ADR prevale)

Tre proposte rovesciano decisioni già prese. Nessuna è un `CONFLICT`: un handoff non supera un ADR. Restano
registrate perché **l'argomento è nuovo anche quando la conclusione è respinta**.

### A1 · §4 — «nessuna banda globale Front/Flank/Rear» → **`FAC-11`**

La fonte dichiara che il modello autorevole devono essere i **sei lati individuali**, e che un arco è una
**raccolta di lati scelta dalla singola abilità**, mai una primitiva globale.

Il canone dice l'opposto, due volte ed esplicitamente:

> ADR-0008 §5: *«l'arco frontale resta `HexCone` e resta **uno solo** per difesa, percezione e reazioni»*

E il codice lo implementa così: `URTHexCombatLibrary::IsInFrontalArc(DefenderCell, Facing, OriginCell)` costruisce
un `HexCone` **profondo quanto la distanza dell'attaccante**. Non è un insieme di tre lati: per un attaccante
lontano un cono e un insieme di lati **non coincidono**, ed è questa la differenza vera fra i due modelli — non
il vocabolario.

**Esito**: l'arco unico resta. La domanda («i sei lati devono diventare la primitiva, con gli archi derivati?»)
è registrata come `FAC-11`, perché tocca `Guard`, la copertura, l'Overwatch e `FAC-3` insieme.

### A2 · §10 — «1 step di pivot = 1 punto movimento» → **`FAC-12`**

ADR-0008 §1 misura la rotazione in **step** e la tratta come un **tetto** (`MoveEndPivotMaxSteps`): quante
direzioni sono legali a fine movimento. La fonte propone invece un **prezzo**: il pivot consuma MP e compete
con le celle percorse (`Move 2 celle + Pivot 60° = 3 MP`).

Tetto e prezzo non sono la stessa cosa detta in due modi:

| | Tetto (ADR-0008) | Prezzo (fonte §10) |
|---|---|---|
| Ruotare al massimo consentito | gratis | costa celle di movimento |
| Chi non si muove | ruota libero (3 step, universale) | pagherebbe 3 MP per 180° |
| Asse di scelta | *dove* arrivo, con quale orientamento | *quanto* mi muovo **contro** quanto ruoto |

**Esito**: il tetto resta. Il prezzo è registrato come `FAC-12` — è una decisione di bilanciamento che
nessuno ha preso, non un dettaglio implementativo, e la revisione dei numeri di ADR-0008 è il punto in cui
guardarla.

### A3 · §11 — categorie `Heavy / Standard / Agile`

Già **respinta esplicitamente** da ADR-0008 (tabella *Alternative considerate*): *«l'handoff §8 dice
esplicitamente "NON deve essere automaticamente uguale per ruolo"»*. I valori vivono per **eroe**
(Flux 2/2 · Riva 2/3 · Bastion 1/0 · Vektor 3/3), non per archetipo.

**Esito**: nessuna azione. Registrata qui solo perché la fonte la ripropone, e chi la rileggesse fra sei mesi
la scambierebbe per una proposta aperta.

---

## B. Genuinamente nuove

Quattro domande che **nessun documento del repository poneva**. Verificate assenti nel codice prima di
registrarle, non dedotte dai documenti.

| ID | Domanda | Verifica di assenza |
|---|---|---|
| **`FAC-11`** | I sei lati devono diventare la primitiva, con gli archi derivati per abilità? | vedi A1 |
| **`FAC-12`** | Il pivot **si paga** in punti movimento, o resta solo un tetto? | vedi A2 |
| **`FAC-13`** | Da dove «arriva» un colpo, quando non ha una sorgente puntuale? | `grep -r 'ImpactDirection\|FromTrajectory\|FromImpactCenter' Source/` → **zero occorrenze** |
| **`FAC-14`** | La **rotazione forzata** è un effetto di controllo a catalogo? | `ERTActionEffect` ha `Damage · Heal · Shield · Push · Pull · Status · DamageReduction · DamageStructure` — **nessuna rotazione** |

### `FAC-13` — la direzione d'impatto (§18)

Oggi la direzione da cui arriva un colpo è **implicitamente** la cella dell'attaccante: `IsInFrontalArc` prende
`OriginCell` e basta. Funziona per un colpo diretto e non ha una risposta per il resto — proiettile con
traiettoria, esplosione con centro d'area, terreno che brucia. La fonte lo dice bene:

> *Do not produce absurd Facing effects such as "the burning floor hit me from behind."*

**Non è un difetto attivo**: il danno ambientale non passa da `Plan.Hits`, quindi non incontra il ramo che
legge il facing, e un'area azzera già la copertura per costruzione (*«un'area investe la cella da ogni lato»*).
È una **lacuna che si apre quando E8/E9 daranno una direzione agli effetti d'area** — registrata adesso perché
allora sarà un caso da correggere invece di una scelta da fare.

### `FAC-14` — la rotazione come controllo (§27)

Girare un avversario è geometricamente identico a spostarlo: apre un lato. `Push` e `Pull` esistono a catalogo,
la rotazione no. Se `FAC-3` rendesse `Brace` direzionale, la rotazione forzata diventerebbe **l'unico modo di
aggirare una difesa senza spostare nessuno** — quindi le due decisioni si tengono, e vanno guardate insieme.

---

## C. Già aperte — la fonte le riafferma, non le fa avanzare

| Sezione | Corrisponde a | Nota |
|---|---|---|
| §21 `Brace` direzionale | `FAC-3` | ⚠️ **Esiste un test che pinna l'opposto**: `Spec.Facing.BraceHoldsFromBehind` verifica che `Brace` riduca di 10 anche da ovest con l'unità girata a est (`120 − (22 − 10) = 108`). Chi accettasse §21 deve **cambiare quello scenario**, non aggiungerne uno |
| §22 pivot di reazione | `FAC-5` | la fonte propone il dato `ReactionPivotSteps = 1`; la domanda resta *se* una reazione possa ruotare |
| §39 pathfinding orientation-aware | `FAC-9` | ADR-0008 lo tiene fuori dalla v0.1 e dichiara che la pressione aumenta. Invariato |
| §26 `Interact` e direzione | `FAC-6` | — |
| §7 status che limitano la rotazione | `FAC-7` | — |
| §8 terreno che limita la rotazione | `FAC-8` | — |

---

## D. Già canoniche — nessuna azione

Le sezioni che la fonte elenca come «decisioni consolidate da questa chat» (§64) sono, quasi tutte, decisioni
**già prese qui prima**. Elencate per chi cerca la provenienza:

| Sezione della fonte | Dove vive già |
|---|---|
| §2 sei direzioni discrete su lati esagonali | ADR-0005 §1 · `ERTHexDirection` |
| §5 facing come stato autorevole persistente | ADR-0005 §2 · `ARTUnit::Facing` |
| §6 la presentazione non decide il facing | Invariante **#1** · CP 16.1 |
| §7 il movimento deriva il facing; il movimento bloccato non ruota | D-020 · `FacingFromPath` · `Spec.Facing.DerivesFromMove` |
| §8-§9 il pivot è un'operazione pianificata in step | ADR-0008 §1 · `PlannedFacing`/`TryApplyDeclaredFacing` |
| §12 il pivot non è un «facing finale desiderato» | ADR-0008 §2 (non retroattivo) |
| §13-§14 niente correzione libera, niente snap-back | ADR-0005 §2-bis · D-020 |
| §15 policy di orientamento per azione | ADR-0008 §3 (`ERTActionFacingPolicy`) |
| §17 direzione relativa all'orientamento del bersaglio | `IsInFrontalArc` (con la riserva di A1) |
| §19 nessun bonus universale al colpo alle spalle | CP 16.2 — l'emisfero posteriore **toglie** copertura, non aggiunge danno |
| §22 l'Overwatch è direzionale e deriva il cono dal facing | ADR-0005 §4c |
| §26 spostamento forzato e rotazione forzata sono separati | ADR-0005 §3 · ADR-0008 §3 (`ERTDisplacementFacingPolicy`) |
| §30-§32 copertura, muri, LOS separati dal facing | ADR-0002 · CP 9.x |
| §34-§35 il facing futuro è privato, quello assunto è pubblico | ADR-0005 §5 · invariante **#6** |
| §44 il TurnLog spiega i cambi di facing | `ERTFacingOutcome` · `RearHitBypassedCover` |
| §51 pagina Wiki del facing | [`facing-e-direzionalita.md`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/facing-e-direzionalita) |
| §53 epic | **E16** ([#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175)), chiusa |

---

## E. Gli scenari proposti (§49) contro quelli che esistono

Sei dei dieci sono già coperti. I quattro restanti **non si scrivono adesso**, e il motivo è diverso per ognuno.

| Proposto | Stato |
|---|---|
| `FACING-02` movimento deriva il facing | ✅ `Spec.Facing.DerivesFromMove` |
| `FACING-04` difesa direzionale stretta | ✅ `Spec.Facing.FrontAttackKeepsGuard` + `BackAttackIgnoresGuard` (con l'arco, non con l'insieme di lati) |
| `FACING-07` l'orientamento d'attacco persiste | ✅ `Spec.Facing.TargetingReorients` |
| — | ✅ `Spec.Facing.DashReorients`, `Spec.Facing.BraceHoldsFromBehind` (nessuno dei due è nella lista della fonte) |
| `FACING-08` copertura dell'Overwatch | ⏳ pianificato come `Spec.Overwatch.FrontlineFollowsFacing` — **E14**, non ora |
| `FACING-01` pivot da fermo | ⏳ arriva col budget di pivot: ADR-0008 lo pianifica come `Facing.StationaryRotationIsUniversal` |
| `FACING-03` movimento bloccato | ⏳ idem, `Facing.PivotBudget*` |
| `FACING-05` difesa a insieme di lati esplicito | ⛔ **bloccato da `FAC-11`**: scriverlo adesso significherebbe decidere `FAC-11` con un file di test |
| `FACING-06` rotazione forzata apre la difesa | ⛔ **bloccato da `FAC-14`**: l'effetto non esiste |
| `FACING-09` boundary d'impatto simultaneo | ⏳ **E14** · CP 14.7 — è la stessa forma dei `Spec.Clash.*` |
| `FACING-10` fog e privacy | ⏳ **E13** · CP 13.4 |

**Nessuno scenario `planned` nuovo** entra nel registry: quattro sono già pianificati altrove con un altro
nome, e due dipendono da decisioni aperte. Aggiungerli produrrebbe la stessa voce contata due volte — che è
esattamente ciò che ha fatto oscillare il totale dei pianificati fra 28, 29 e 38.

---

## F. Feature map — perché **non** si spezza in sedici

La §55 chiede una «famiglia Facing» di sedici voci (`Facing Core`, `Pivot`, `Facing UI`, `Facing Replay`…).

`RT-FEAT-MAP-FACING` esiste, è `IMPLEMENTING`, e i suoi **nove gate** già dicono, per asse, esattamente ciò che
le sedici voci direbbero — con la differenza che i gate sono **derivati e verificati**, mentre sedici status
sarebbero sedici posti in cui scrivere a mano la stessa cosa. La §55 chiede una tassonomia; il registry ha già
una **misura**.

Nessuna modifica alla feature map da questa fonte. L'unica cosa che la fonte avrebbe cambiato — il gate
`runtime` — era **già** passato a `partial` il 2026-08-10 per ADR-0008, con la motivazione scritta nel registry.

---

## G. Editor map

La §57 propone sei voci di lavoro Editor. Cinque sono **presentazione** (decal di debug, selettore a sei lati,
overlay direzionali, marcatore di facing, allineamento della mesh), e una sola tocca una lacuna già dichiarata
altrove: **manca un indicatore dell'arco frontale in HUD**, ed è il motivo per cui il gate `ui_wiki` di
`RT-FEAT-MAP-FACING` è `partial` e per cui `PIE-FACING-1` porta un ⚠️.

Nessuna seduta nuova: la voce esiste, ha già il suo avviso, e duplicarla in `editor-sessions.yaml` la
scollegherebbe dal gate che la misura.

---

## Esito

- **4 decisioni nuove** registrate: `FAC-11`, `FAC-12`, `FAC-13`, `FAC-14` in
  [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)
- **0 epic create** — `E16` esiste ed è chiusa; le decisioni nuove appartengono a `#339`
- **0 issue create** — le 14 proposte descrivono codice esistente o lavoro già tracciato
- **0 scenari nuovi**, **0 feature nuove**, **0 sedute editor nuove**, con la motivazione per ognuno qui sopra
- **1 issue aggiornata**: [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339), che
  dichiarava «nove decisioni aperte» quando erano **sei** (ADR-0008 ne ha chiuse quattro) e ora sono **dieci**
