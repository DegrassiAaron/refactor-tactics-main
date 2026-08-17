# REFACTORTACTICS — CLAUDE HANDOFF
## Skill Plus — consolidamento documentazione, Epic/Issue, roadmap e tracking fino alla v1.0

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-17** dopo il consolidamento.
> **Non si applica**: si legge per sapere da dove viene una decisione. Le fonti autorevoli sono
> [`spec-ownership-abilita-interazioni-sinergie.md`](../../gameplay/spec-ownership-abilita-interazioni-sinergie.md),
> [`adr-0006-ownership-abilita-sinergie.md`](../../decisions/adr-0006-ownership-abilita-sinergie.md) e
> [`feature-registry.yaml`](../../roadmap/feature-registry.yaml).
>
> **Matrice di riconciliazione**: [`skill-plus-consolidamento-2026-08-17.md`](../../roadmap/plans/skill-plus-consolidamento-2026-08-17.md)
> — `REUSE 6 · CREATE 1 · LINK 2 · DEFER 5 · CONFLICT 1 · NOT NEEDED 1`.
>
> 🔴 **Il sistema che propone di costruire esiste già, è `INTEGRATED` ed è in `v0.1`.**
> `RT-FEAT-ENV-SYSTEMIC-COMBOS` — *«Interazioni sistemiche producer/consumer»* — ha nove gate su dieci
> `done`, l'`ADR-0006`, il test `RefactorTactics.Reactions.NoHeroSpecificBranchInResolver` che pinna il
> divieto di branch per eroe (il suo §5.2), e gli scenari `Visual.Combat.WaterElectric*` (il suo §7.2).
> L'epic **E8** (#22) e i checkpoint `CP 8.3`/`8.4`/`8.5` sono **chiusi**: le sue `SP-040`/`SP-041`,
> messe in `v0.3–v0.4`, sono in `main` da tempo.
>
> ✅ **Il gap reale è contenuto, non architettura**: `Debris` · `Rubble` · `Scatter` hanno **0**
> occorrenze in `Source/`. Diventa
> [#1132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1132) — i detriti dentro la
> grammatica **esistente**, non come sistema nuovo.
>
> ⚠️ **E il suo caso di riferimento ha entrambi i termini assenti.** `Wind + Debris → Flying Debris`:
> `grep -il "Wind"` dà sei header e sono **tutti** `Window`/`Rewind`. Il vento non esiste. È la ragione
> per cui #1132 lo dichiara **fuori scope** — introdurre l'elemento e il consumatore insieme
> renderebbe impossibile dire quale dei due ha rotto lo scenario.
>
> 🔴 **CONFLICT sul nome, non deciso qui**:
> [#1133](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1133). La §9 della spec owner
> decide già il vocabolario — `Sinergia` · `Interazione sistemica` · `Setup → Payoff` · `Combo`
> descrittivo — e «Skill Plus» sarebbe il quarto termine, l'unico inglese in un asse nominato in
> italiano. Il suo §1 chiede di non scegliere in silenzio: la scelta è nella issue.
>
> ⏸️ **Non è entrato**: le 10 epic `SP-*` (9 su 10 hanno un owner o una release che le differisce), le
> 5 feature di §14 (quattro hanno un omonimo semantico), le 6 voci `PIE-SKILLPLUS-*` (il registro è di
> un'altra track e le voci **si propongono**, `D-139`), e `release: v1.0` — che lo schema non ammette,
> come il documento stesso avverte al §2.2.

**Data handoff:** 2026-08-17  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Branch canonico da verificare prima di ogni modifica:** `main`

---

# 0. SCOPO

Questo handoff consegna a Claude il focus **Skill Plus** discusso nel progetto RefactorTactics.

Obiettivo operativo:

1. verificare lo stato reale del repository e delle issue;
2. consolidare il concetto Skill Plus nella documentazione esistente;
3. NON creare sistemi paralleli se esiste già un owner documentale o un tracker adatto;
4. riconciliare Epic e Issue esistenti prima di crearne di nuove;
5. creare una roadmap Skill Plus dal primo vertical slice fino alla **v1.0**;
6. aggiornare tutti gli elementi di tracking già usati dal progetto:
   - Roadmap;
   - Feature Registry;
   - Decision Log / ADR / Open Decisions;
   - Epic e Issue;
   - Scenario Map / scenari;
   - Test / Validation Map;
   - Editor Map / PIE manual tests;
   - Wiki;
   - Product / Feature Map se presenti;
   - asset/data tracking;
   - Balance / Telemetry;
   - Execution Graph / dependency tracking;
   - validator / content pipeline;
   - eventuali dashboard o viste generate;
7. produrre una riconciliazione leggibile che distingua chiaramente:
   - `REUSE`
   - `UPDATE`
   - `CREATE`
   - `DEFER`
   - `CONFLICT / DECISION REQUIRED`

Non limitarsi a creare issue.  
Il lavoro è terminato solo quando il sistema è collegato end-to-end:

`Decision -> Feature -> Epic/Issue -> Definition/Data -> Runtime Consumer -> UI -> Scenario -> Test -> Evidence -> Release`

---

# 1. REGOLE DI PREVALENZA

Prima di modificare qualunque cosa applicare questo ordine:

1. decisioni esplicite già approvate nel repository;
2. ADR / Decision Log / Open Decisions;
3. Feature Registry canonico;
4. owner specification / PDR / gameplay specs;
5. roadmap;
6. issue GitHub;
7. Wiki e viste derivate;
8. contenuto di questo handoff.

Questo handoff è una **seed specification** da riconciliare, non una scusa per sovrascrivere decisioni più recenti.

Se trovi un conflitto, NON scegliere silenziosamente.
Documenta:

- sorgente A;
- sorgente B;
- differenza;
- proposta;
- owner decision;
- issue/ADR necessaria.

---

# 2. PREFLIGHT OBBLIGATORIO

Prima di creare o aggiornare contenuti:

## 2.1 Repository

Verificare:

- branch `main`;
- HEAD attuale;
- working tree;
- ultimi commit rilevanti;
- roadmap corrente;
- Feature Registry;
- issue aperte e chiuse;
- milestone;
- Wiki;
- scenari;
- test;
- documenti ability/environment/debris/UI;
- validator;
- generatori delle viste.

## 2.2 Feature Registry

Il repository usa come sorgente canonica:

`docs/roadmap/feature-registry.yaml`

Regole già osservate:

- `feature-registry.yaml` è la sorgente;
- `feature-registry.json` è generato e NON va editato a mano;
- stato feature derivato dai gate;
- niente percentuali manuali;
- roadmap/wiki/dashboard devono referenziare `feature_id` senza duplicare stato;
- issue, test, scenari, wiki_refs e pie_refs vanno collegati alla feature;
- `last_verified` deve riflettere una verifica reale.

**Nota importante:** al momento del controllo di questo handoff, i valori release documentati nel registry erano:

`v0.1 | v0.2 | v0.3 | v0.4 | future`

Quindi NON scrivere semplicemente `release: v1.0`.

Prima:

1. verificare `scripts/feature_registry.py`;
2. verificare `RELEASE_ORDER`;
3. verificare roadmap/milestone reali;
4. decidere se la roadmap verso 1.0 usa:
   - nuove release canoniche;
   - milestone intermedie;
   - `future`;
   - un'estensione ufficiale dello schema.

Se serve estendere le release, farlo come modifica esplicita e testata, non come edit locale isolato.

## 2.3 Ricerca issue

Al momento di questo handoff non risultavano open issue trovate con ricerca esatta:

- `"Skill Plus"`
- `debris OR rubble`

Questo NON significa che il lavoro non esista.

Cercare anche semanticamente:

- environmental interaction;
- environment reaction;
- combo;
- ability interaction;
- ability effect;
- terrain interaction;
- surface;
- debris;
- rubble;
- destruction;
- wind;
- scatter;
- affordance;
- combo hint;
- tactical designer;
- ability authoring;
- ability validator;
- tooltip;
- targeting preview;
- environment preview;
- team combo;
- synergy;
- TurnLog ability effect;
- gameplay tags;
- content validation.

Per ogni candidato esistente classificare:

`REUSE / UPDATE / LINK / DUPLICATE / NOT RELATED`

Creare una nuova issue solo se nessuna issue esistente copre davvero il deliverable.

---

# 3. CONCETTO CONSOLIDATO — SKILL PLUS

## 3.1 Definizione

**Skill Plus** NON significa “skill potenziata”.

È un livello sistemico opzionale di una ability:

`Primary Effect + Context Interaction -> Secondary Effect`

Il Plus permette alla skill di reagire a:

- ambiente;
- superfici;
- stati;
- cover;
- map elements;
- hazard;
- debris;
- eventi;
- displacement;
- setup di un'altra skill;
- setup di un alleato.

Il Plus deve produrre soprattutto **nuove possibilità tattiche**, non semplicemente bonus numerici.

Evitare come baseline:

`Wet -> +5 damage`

Preferire:

`Wet -> chain electricity`

oppure:

`Wind + Debris -> debris diventano proiettili`

oppure:

`Water + Fire -> Steam`

oppure:

`Forced Move + prepared zone -> secondary interaction`

---

# 4. REFERENCE USE CASE — WIND + DEBRIS

Questo è il caso di riferimento con cui validare il framework.

## 4.1 Esempio

Una skill di vento ha un effetto primario.

Esempio concettuale:

`Gale Sweep`

Primary:

- cono di vento;
- push leggero;
- eventuale dispersione di smoke/steam.

Skill Plus:

`Wind + Debris.Light -> Flying Debris`

Quando il cono colpisce debris compatibili:

1. il resolver rileva il debris;
2. calcola una traiettoria nella direzione del vento;
3. i frammenti attraversano celle secondo regole deterministiche;
4. possono colpire unità;
5. producono danno/effect;
6. la cover può bloccare o modificare l'esito se la regola lo prevede;
7. il debris sorgente viene consumato, ridotto o trasformato secondo policy;
8. tutto viene registrato nel TurnLog.

Il risultato NON deve dipendere da Chaos Physics, framerate o animazioni.

---

# 5. PRINCIPI DI DESIGN

## 5.1 Skill Plus deve essere leggibile

Il giocatore non deve conoscere la combo solo tramite Wiki.

Quando seleziona una skill con Plus deve poter capire:

> “Se uso questa skill su questa cella / su questo elemento, attivo questo effetto secondario.”

Quindi Skill Plus comprende tre parti inseparabili:

1. **regola sistemica**;
2. **icon grammar / affordance**;
3. **contextual preview**.

## 5.2 Non hard-codare coppie di personaggi

NO:

`if RivaUsedFloodgate`
`if BastionCreatedDebris`

SÌ:

Producer:

`produces Surface.Water`
`produces Environment.Debris.Light`
`produces Event.ForcedMove`

Consumer:

`interacts with Surface.Water`
`interacts with Environment.Debris.Light`
`interacts with Event.ForcedMove`

Questo permette a contenuti futuri di riutilizzare la stessa grammatica.

## 5.3 Un Plus deve cambiare una decisione

La presenza del Plus deve:

- cambiare target;
- cambiare facing;
- cambiare timing;
- cambiare posizione;
- cambiare uso della mappa;
- creare setup/payoff;
- creare counterplay.

Se è solo un numero extra, verificare che non appartenga a un normale modifier.

## 5.4 Vertical slice

Baseline proposta:

- massimo **1 Skill Plus significativo per singola skill**;
- almeno alcuni Skill Plus nei quattro personaggi del vertical slice;
- non obbligare tutte le skill ad averne uno;
- preferire pochi esempi fortemente sistemici e leggibili.

---

# 6. TASSONOMIA INIZIALE

Usare tassonomia solo se non confligge con tag/termini esistenti.

Categorie concettuali:

1. `Environment Plus`
   - debris;
   - cover;
   - door;
   - bridge;
   - fire;
   - smoke;
   - water;
   - map object.

2. `State Plus`
   - Wet;
   - Burning;
   - Charged;
   - Marked;
   - Guarded;
   - status analoghi.

3. `Skill/Setup Plus`
   - un'altra skill ha prodotto uno stato o modificato la mappa.

4. `Event Plus`
   - ForcedMove;
   - CoverDestroyed;
   - CrossedEdge;
   - NoiseGenerated;
   - DetectionChanged;
   - ecc.

Non è necessario modellarle con quattro classi C++ diverse.
Potrebbero essere categorie di authoring/documentazione sopra un sistema comune di condition/effect.

---

# 7. GRAMMATICA UI / COMBO DISCOVERY

La UI deve supportare la discoverability.

## 7.1 Badge Skill Plus

Ogni skill con Plus deve avere un simbolo/badge coerente.

Non affidarsi solo al colore.

## 7.2 Icon grammar

Esempio concettuale:

- Wind
- Water
- Electric
- Fire
- Debris
- Cover
- Displacement
- Visibility
- Noise
- Interactive Map Element

Pattern:

`INPUT + CONTEXT -> OUTPUT`

Esempi:

`Wind + Debris -> Flying Debris`

`Electric + Water -> Conductive Propagation`

`Fire + Water -> Steam`

Le icone finali devono usare asset originali e accessibili.

## 7.3 Map affordance

Quando il giocatore seleziona una skill:

- mostra il normale AoE/targeting;
- evidenzia celle/elementi compatibili con il Plus;
- non enfatizza elementi incompatibili;
- non rivela informazioni nascoste.

## 7.4 Tooltip contestuale

Hover/focus su un elemento compatibile:

`SKILL PLUS — DEBRIS SCATTER`

Testo breve:

> I debris leggeri verranno scagliati nella direzione del vento.

Numeri solo se utili.

## 7.5 Preview secondaria

Se la combo è attiva mostra:

- origine effetto secondario;
- direzione;
- traiettoria;
- celle interessate;
- target potenziali;
- friendly-fire warning;
- eventuale stato finale del map element.

## 7.6 Certainty model

Integrare con il sistema già adottato:

- `Confermato`
- `Previsto`
- `Incerto`

Esempi:

### Confermato
Il debris esiste nello stato pubblico e il Plus è sicuramente attivabile.

### Previsto
Un alleato pianifica un'azione che dovrebbe creare il debris prima dell'impatto.

### Incerto
L'esito dipende da stato futuro lecito ma non certo.

Mai usare planning nemico server-only per costruire hint o preview client.

---

# 8. TEAM COMBO DISCOVERY

Skill Plus deve aiutare anche la coordinazione.

Esempio:

`Ally breaks cover`
-> `Debris created`
-> `Wind skill`
-> `Debris Scatter`

La UI può mostrare una combo prevista usando SOLO:

- stato pubblico;
- proprio intent;
- intenti alleati autorizzati.

NON usare intenti nemici.

Il suggerimento deve essere informativo, non prescrittivo.

Buono:

> “Skill Plus previsto: Debris Scatter”

Evitare:

> “Questa è la giocata ottimale.”

---

# 9. MODELLO TECNICO CONCETTUALE

NON copiare i nomi seguenti senza verificare convenzioni reali.

Possibile struttura:

```cpp
struct FRTSkillPlusSpec
{
    FName SkillPlusId;
    ERTSkillPlusTiming Timing;
    TArray<FGameplayTag> RequiredTags;
    TArray<FGameplayTag> ExcludedTags;
    TArray<FGameplayTag> ConsumedTags;
    FName SecondaryEffectId;
    int32 Priority;
};
```

Possibili timing:

- `OnImpact`
- `AfterPrimaryEffect`
- `AfterDisplacement`

Estendere solo quando necessario.

Ability definition:

```text
AbilityDefinition
  PrimaryEffects[]
  SkillPlus[]
```

Principio:

- C++ definisce cosa è possibile;
- Data Asset / dati scelgono le combinazioni;
- resolver è autorità;
- GAS non decide l'esito competitivo;
- presentation legge gli eventi.

---

# 10. PIPELINE DI RISOLUZIONE

Baseline:

```text
Resolve Ability
    |
    v
Primary Effect
    |
    v
Canonical Events
    |
    v
Evaluate Skill Plus
    |
    +-- condition false --> continue
    |
    +-- condition true
            |
            v
      Secondary Effect
            |
            v
      Canonical Events
            |
            v
         TurnLog
```

Definire esplicitamente:

- timing;
- priority;
- chain depth;
- recursion guard;
- ordering;
- tie-break;
- consumed/transformed state.

Nessun risultato deve derivare dall'ordine accidentale di TMap/TSet.

---

# 11. CHAIN DEPTH

Evitare catene incontrollate:

`Plus -> Plus -> Plus -> Plus`

Serve una policy.

Baseline da valutare:

- ogni evento secondario può dichiarare se genera ulteriori interaction opportunities;
- depth massima esplicita;
- cycle detection;
- stable order;
- TurnLog reason codes.

Per v0.1 preferire:

- un primary effect;
- un Plus;
- niente ricorsione libera.

---

# 12. ROADMAP SEED — EPIC SKILL PLUS

Questa roadmap è una proposta di consolidamento.
Prima di creare Epic/Issue mappare ogni voce contro tracking esistente.

Usare ID locali `SP-*` solo nel piano di riconciliazione se il repository non ha già una numerazione canonica.
Non imporre questa numerazione se esiste uno schema Epic/Checkpoint corrente.

---

## EPIC SP-01 — Skill Plus Core
**Target concettuale:** v0.1

Obiettivo:
framework minimo data-driven e deterministico.

Candidate issues:

### SP-001 — Define Skill Plus gameplay grammar
Deliverable:
- definizione;
- scope;
- categorie;
- producer/consumer;
- primary vs secondary effect;
- non-hard-code rule.

### SP-002 — Add Skill Plus data specification to ability definitions
Deliverable:
- ID;
- timing;
- requirements;
- excluded tags;
- consume/transform policy;
- secondary effect;
- validation hooks.

### SP-003 — Integrate Skill Plus evaluation into authoritative resolver
Deliverable:
- deterministic evaluation;
- stable ordering;
- no presentation dependency.

### SP-004 — Define Skill Plus activation timing
MVP:
- OnImpact;
- AfterPrimaryEffect;
- AfterDisplacement.

### SP-005 — Add Skill Plus TurnLog events and reason codes
Candidate events:
- available;
- triggered;
- skipped/not triggered;
- secondary effect;
- transform/consume.

### SP-006 — Determinism automation tests for Skill Plus
Golden fixture:
`Wind + Debris.Light -> Flying Debris`

---

## EPIC SP-02 — Debris Scatter Reference Interaction
**Target concettuale:** v0.1

### SP-010 — Define debris interaction properties
Verificare sistema Debris/Rubble esistente.
Non creare un secondo modello.

Candidate properties:
- size/intensity;
- scatterable;
- material;
- damage profile;
- current state;
- transform policy.

### SP-011 — Implement Wind -> Debris Scatter interaction

### SP-012 — Reuse trajectory/LOS/cover services for flying debris

### SP-013 — Add Flying Debris damage/effect profile

### SP-014 — Define debris consume/downgrade/relocation policy

### SP-015 — Define and test friendly-fire policy

### SP-016 — Add visible DevSandbox/functional scenario

---

## EPIC SP-03 — Skill Plus Affordance UI
**Target concettuale:** v0.1

### SP-020 — Universal Skill Plus badge

### SP-021 — Skill Plus icon grammar

### SP-022 — Highlight compatible map cells/elements on skill selection

### SP-023 — Contextual Skill Plus tooltip

### SP-024 — Secondary-effect trajectory/AoE preview

### SP-025 — Confirmed/Predicted/Uncertain styling

### SP-026 — Discoverability usability test

Exit gate:
un giocatore che non ha letto la Wiki deve poter scoprire Wind + Debris dalla UI.

---

## EPIC SP-04 — Team Combo Discovery
**Target concettuale:** v0.2

### SP-030 — Producer/Consumer metadata model

### SP-031 — Detect predicted Skill Plus from allied intents

### SP-032 — Team Combo badge

### SP-033 — Multi-step combo preview

### SP-034 — Live invalidation/update when ally intent changes

### SP-035 — Privacy tests for combo hints

---

## EPIC SP-05 — Interaction Grammar Expansion
**Target concettuale:** v0.3-v0.4

### SP-040 — Water + Electricity
### SP-041 — Water + Fire -> Steam
### SP-042 — Wind + Smoke/Steam
### SP-043 — Forced Move + Hazard
### SP-044 — Skill + Cover
### SP-045 — Skill + interactive map edge
### SP-046 — Debris state interactions
### SP-047 — Chain-depth/cycle policy

Creare solo esempi che servono davvero alle milestone.
Non aprire tutto come work-in-progress contemporaneamente.

---

## EPIC SP-06 — Skill Plus Authoring & Tactical Designer
**Target concettuale:** v0.4-v0.6

### SP-050 — Data Asset authoring workflow
### SP-051 — Interaction Matrix
### SP-052 — Map Editor compatibility preview
### SP-053 — Validator: invalid IDs/tags/effects
### SP-054 — Validator: impossible trigger/target combinations
### SP-055 — Validator: recursion/cycles
### SP-056 — Validator: missing UI metadata

---

## EPIC SP-07 — Player Learning & Combo Codex
**Target concettuale:** v0.6-v0.8

### SP-060 — Contextual combo hints
### SP-061 — Extended tooltip Skill Plus section
### SP-062 — Combo Codex
### SP-063 — Map Element interaction page
### SP-064 — Guided Skill Plus tutorial scenario
### SP-065 — Accessibility pass

---

## EPIC SP-08 — Advanced System Integration
**Target concettuale:** v0.8-v0.9

### SP-070 — Skill Plus + Reaction System
### SP-071 — Skill Plus + Noise
### SP-072 — Skill Plus + Team Knowledge/Fog of War
### SP-073 — Skill Plus + Auxiliary Units
### SP-074 — Multilevel Skill Plus
### SP-075 — Replay/explainability integration

---

## EPIC SP-09 — Balance, Telemetry & QA
**Target concettuale:** v0.9

### SP-080 — Skill Plus telemetry
Metrics candidate:
- available;
- highlighted;
- previewed;
- committed;
- triggered;
- damage/control contribution;
- team combo rate.

### SP-081 — Interaction power budget
Valutare:
`Primary Power + Plus Opportunity Value`

### SP-082 — Combo dominance tests

### SP-083 — TurnLog explainability pass

### SP-084 — Automated interaction corpus

### SP-085 — Performance profiling

---

## EPIC SP-10 — Skill Plus 1.0 Hardening
**Target:** v1.0

### SP-090 — Freeze Skill Plus v1 schema
### SP-091 — Complete content validation
### SP-092 — Packaged network/privacy tests
### SP-093 — Determinism regression corpus
### SP-094 — Accessibility certification pass
### SP-095 — Shipping content audit
### SP-096 — Skill Plus v1 Definition of Done

DoD v1:

Una nuova interazione Skill Plus può essere aggiunta prevalentemente tramite dati, è:
- leggibile;
- previewable;
- deterministica;
- spiegabile;
- testata;
- accessibile;
- priva di leak;
- compatibile con replay;
- validata in packaged build.

---

# 13. ROADMAP DEPENDENCY GRAPH

Usare la vista/format esistente nel repository se presente.

Concettualmente:

```text
SP-01 Core
   |
   +----> SP-02 Debris Reference
   |          |
   |          +----> SP-03 UI Affordance
   |
   +----> SP-04 Team Combo
                |
                v
         SP-05 Interaction Grammar
                |
        +-------+-------+
        v               v
 SP-06 Authoring   SP-07 Learning
        |               |
        +-------+-------+
                v
       SP-08 Integrations
                |
                v
       SP-09 Balance / QA
                |
                v
          SP-10 v1.0
```

---

# 14. FEATURE REGISTRY

Dopo la riconciliazione creare il MINIMO numero di feature canoniche necessarie.

Evitare una feature registry entry per ogni micro-issue.

Possibile decomposizione da verificare:

- `RT-FEAT-ABILITY-SKILLPLUS-CORE`
- `RT-FEAT-ENV-DEBRIS-SCATTER`
- `RT-FEAT-UI-SKILLPLUS-AFFORDANCE`
- `RT-FEAT-TEAM-SKILLPLUS-COMBO`
- `RT-FEAT-TOOLS-SKILLPLUS-AUTHORING`

NON usare questi ID se esistono già feature equivalenti.

Ogni feature deve avere:

- area;
- kind;
- release compatibile con schema;
- priority;
- status derivato;
- gates;
- roadmap linkage;
- dependencies;
- owner_specs;
- issues;
- tests;
- scenarios;
- wiki_refs;
- pie_refs se pertinenti;
- last_verified.

---

# 15. DOCUMENTAZIONE DA CONSOLIDARE

Individuare owner document reali prima di creare nuovi file.

Aree da verificare:

## Ability System
Integrare:
- definizione Skill Plus;
- data model;
- timing;
- secondary effect;
- producer/consumer;
- authoring.

## Environmental / Elemental Reaction
Integrare:
- Skill Plus come consumer/producer di stato ambientale;
- Wind + Debris reference;
- Water + Electric;
- Water + Fire;
- interaction matrix.

## Debris / Rubble
Integrare:
- scatterability;
- transformation;
- trajectory;
- damage;
- material;
- interaction tags.

## UI/UX
Integrare:
- badge;
- icon grammar;
- contextual highlight;
- preview;
- Confirmed/Predicted/Uncertain;
- allied combo hints;
- accessibility.

## Simulation / TurnLog
Integrare:
- timing;
- canonical events;
- chain depth;
- deterministic order;
- explainability.

## Networking / Privacy
Integrare:
- team-only combo hints;
- no enemy intent leakage;
- DTO/view model restrictions.

## Data / Validation
Integrare:
- IDs;
- tags;
- effect references;
- validator;
- impossible combos;
- cycle detection;
- metadata completeness.

## Tactical Designer
Integrare:
- authoring UI;
- interaction matrix;
- map compatibility;
- cross-reference skill <-> map element.

---

# 16. WIKI

Non creare una Wiki parallela.

Verificare struttura e navigazione esistente.

Aggiornamenti candidate:

- Ability System / Skill Plus;
- Environmental Interactions;
- Debris/Rubble;
- Combo Grammar;
- UI Affordances;
- Tactical Designer;
- Systems Overview;
- System Registry.

La Wiki deve essere una vista leggibile, NON source of truth per status.

Aggiungere collegamenti bidirezionali:

`Skill -> Interaction -> Map Element -> Scenario -> Feature -> Issue`

---

# 17. ADR / DECISION LOG / OPEN DECISIONS

Non creare ADR per ogni dettaglio.

Valutare ADR solo per decisioni architetturali stabili.

Possibili decisioni candidate:

## ADR candidate — Skill Plus is data-driven interaction grammar
Decision:
le skill non conoscono hero/source ability specifiche; leggono stati/tag/eventi.

## ADR candidate — Skill Plus resolution is authoritative and deterministic
Decision:
la physics presentation non decide Flying Debris.

## ADR candidate — UI combo hint privacy boundary
Decision:
preview Skill Plus usa solo public + owner/team-authorized knowledge.

## Open decisions candidate
Se non già decisi:

- friendly fire dei Flying Debris;
- consume vs downgrade debris;
- chain depth massima;
- se Plus secondari possono generare altri Plus;
- naming ufficiale `Skill Plus`;
- simbolo/badge;
- reveal policy nel Combo Codex;
- timing exact set per v0.1.

Se una decisione è già presente altrove, aggiorna/collega invece di duplicare.

---

# 18. SCENARI

Ogni scenario deve usare lo schema reale del repository.

Candidate scenario intent:

## Scenario A — Wind + Debris basic
- debris già presente;
- skill Wind;
- preview mostra Plus;
- commit;
- Flying Debris colpisce target;
- TurnLog spiega.

## Scenario B — Wind + Debris blocked by cover
- stessa sorgente;
- cover interrompe traiettoria.

## Scenario C — Friendly fire
- alleato nella linea dei debris;
- warning durante planning;
- esito coerente con policy.

## Scenario D — Ally creates Debris
- alleato rompe cover;
- secondo alleato usa Wind;
- preview `Previsto`;
- combo si risolve.

## Scenario E — Ally changes plan
- combo inizialmente prevista;
- alleato cambia intent;
- hint/preview viene invalidata.

## Scenario F — Privacy canary
- planning nemico contiene dati che renderebbero possibile una combo futura;
- client opposto NON riceve hint/preview derivati da quei dati.

---

# 19. AUTOMATION / FUNCTIONAL TESTS

Mappare a naming e suite reali.

Coverage minima:

## Core
- condition matching;
- required/excluded tag;
- timing;
- stable order;
- consume/transform;
- cycle protection.

## Debris
- light scatter;
- medium/heavy policy;
- collision;
- cover;
- multi-target;
- friendly fire;
- deterministic target order.

## Determinism
- permutation;
- repeat;
- stable TurnLog;
- frame-rate independent playback;
- same snapshot/rules/seed -> same outcome.

## UI/ViewModel
- compatible cells;
- certainty state;
- ally intent update;
- stale preview invalidation.

## Privacy
- no enemy canonical intent data;
- no enemy future combo hints;
- packaged canary test.

## Data validator
- duplicate PlusId;
- unknown tag;
- missing effect;
- invalid timing;
- impossible condition;
- cycle;
- missing UI metadata.

---

# 20. EDITOR MAP / PIE

Aggiornare `docs/technical/test-manuali-pie.md` o owner equivalente se esiste.

Candidate checks:

- `PIE-SKILLPLUS-01` Wind + Debris visible affordance;
- `PIE-SKILLPLUS-02` preview secondary trajectory;
- `PIE-SKILLPLUS-03` Confirmed/Predicted/Uncertain;
- `PIE-SKILLPLUS-04` allied combo hint update;
- `PIE-SKILLPLUS-05` friendly fire warning;
- `PIE-SKILLPLUS-06` TurnLog explanation.

Usare il naming reale esistente.
Non aggiungere questi ID se collidono con convenzioni correnti.

---

# 21. ASSET / CONTENT TRACKING

Skill Plus richiede anche asset di comunicazione.

Verificare tracker asset esistente.

Candidate asset:

- Skill Plus badge icon;
- category icons:
  - Wind;
  - Water;
  - Electric;
  - Fire;
  - Debris;
  - Cover;
  - Displacement;
  - Noise;
  - interactive object;
- patterns accessibility;
- map overlay marker;
- combo-link graphic;
- tooltip icon set;
- debug visualization;
- optional VFX:
  - Flying Debris trajectory;
  - impact;
  - environment transform.

Tutti gli asset:
- scala coerente;
- contrasto;
- leggibilità 1080p;
- non solo colore;
- originali;
- tracciati con owner/milestone/status secondo il sistema esistente.

---

# 22. BALANCE / TELEMETRY TRACKING

Aggiornare il sistema esistente, non crearne uno parallelo.

Metriche candidate:

- Plus availability rate;
- Plus preview rate;
- Plus commit rate;
- Plus trigger rate;
- successful team combos;
- average secondary damage/control;
- friendly fire caused;
- map element usage;
- percentage of skill uses where Plus materially changes outcome;
- combo dominance;
- character dependency;
- map dependency.

Design checks:

- una skill non deve essere inutile senza Plus;
- un Plus non deve essere automatico in quasi ogni uso;
- una mappa non deve rendere un personaggio obbligatorio;
- setup forte deve avere telegraph/counterplay;
- secondary damage non deve creare scaling incontrollabile.

---

# 23. EXECUTION GRAPH / DEPENDENCY TRACKING

Collegare Skill Plus alle dipendenze reali.

Candidate dependencies:

`Ability Definition`
-> `Gameplay Tags`
-> `Targeting/Trajectory`
-> `Environment State`
-> `Action Resolver`
-> `TurnLog`
-> `Planning ViewModel`
-> `Skill Plus Preview`
-> `Team Intent Relay`
-> `Scenario`
-> `Automation`
-> `Packaged QA`

Wind + Debris in particolare:

`Debris/Rubble State`
-> `Scatterability Data`
-> `Wind Interaction`
-> `Trajectory`
-> `Damage/Event`
-> `Environment Transform`
-> `TurnLog`
-> `UI Preview`

Segnalare blocchi reali se un sistema non è ancora implementato.

---

# 24. ISSUE FORMAT

Per ogni issue nuova o consolidata usare il template reale del repository.

Ogni issue Skill Plus deve dichiarare almeno:

- Context;
- Goal;
- Scope;
- Out of Scope;
- Dependencies;
- Acceptance Criteria;
- Test / Verification;
- Scenario;
- Feature Registry linkage;
- Docs/Wiki linkage;
- Privacy impact;
- Determinism impact;
- Packaged verification quando pertinente.

Evitare issue vaghe tipo:

`Implement Skill Plus`

Preferire deliverable verificabile.

---

# 25. EPIC FORMAT

Un Epic deve:

- avere un exit gate;
- elencare feature e issue figlie;
- mostrare dipendenze;
- dichiarare milestone/release;
- collegare scenario/test;
- non duplicare stato che vive nel Feature Registry.

Exit gate esempio SP-03:

> A player selecting a Skill Plus ability can discover compatible map elements, understand the resulting secondary effect before commit, and distinguish confirmed/predicted/uncertain outcomes without receiving unauthorized enemy planning data.

---

# 26. RELEASE / MILESTONE MAPPING

Usare release realmente supportate.

Seed concettuale:

- v0.1:
  - Core;
  - Wind + Debris reference;
  - UI affordance base.

- v0.2:
  - allied combo discovery;
  - privacy-aware team preview.

- v0.3-v0.4:
  - interaction grammar expansion;
  - more surfaces/events;
  - validation hardening.

- post-v0.4/future:
  - authoring tooling;
  - codex/tutorial;
  - Reaction/Noise/Auxiliary integration;
  - telemetry;
  - final v1 hardening.

Se il repo ha già milestone più precise, mappare questo seed a quelle milestone.

---

# 27. CONSOLIDATION MATRIX OBBLIGATORIA

Prima dei write, produrre una tabella tipo:

| Seed | Existing owner | Existing issue/epic | Action | Reason |
|---|---|---|---|---|
| Skill Plus Core | ... | #... | UPDATE | existing ability-effect framework |
| Wind + Debris | ... | #... | UPDATE/CREATE | ... |
| UI affordance | ... | #... | ... | ... |

Azioni ammesse:

- REUSE
- UPDATE
- CREATE
- LINK
- DEFER
- CONFLICT

Questa matrice è parte del deliverable.

---

# 28. MODIFICHE AL FEATURE REGISTRY

Dopo issue/docs reconciliation:

1. aggiornare feature esistenti se coprono Skill Plus;
2. creare feature solo se realmente necessarie;
3. aggiornare dependencies;
4. aggiornare owner_specs;
5. collegare issue;
6. collegare tests;
7. collegare scenarios;
8. collegare wiki_refs;
9. collegare pie_refs;
10. aggiornare `last_verified` solo dopo verifica reale;
11. rigenerare JSON tramite script canonico;
12. eseguire validator.

Non editare `feature-registry.json` a mano.

---

# 29. VALIDAZIONE REPOSITORY

Alla fine eseguire ciò che il repository già prevede, ad esempio:

- Feature Registry validator;
- generator;
- doc/link validation;
- test suite pertinente;
- scenario validation;
- eventuale lint;
- Unreal automation quando possibile;
- packaged/PIE evidence se parte del DoD.

Non inventare comandi.
Leggere README/scripts/CI.

---

# 30. DELIVERABLE CLAUDE

Al termine fornire un report con:

## A. Audit
- HEAD;
- file esaminati;
- issue/epic esaminate;
- feature esistenti rilevanti.

## B. Reconciliation
Tabella:
`REUSE / UPDATE / CREATE / DEFER / CONFLICT`

## C. Docs updated
Elenco file e sintesi modifiche.

## D. Feature Registry
- feature aggiornate;
- feature nuove;
- gate toccati;
- release mapping;
- validator result.

## E. Epic / Issues
Per ogni elemento:
- numero;
- titolo;
- milestone;
- parent Epic;
- feature;
- scenario;
- test.

## F. Roadmap
Vista v0.1 -> v1.0/future compatibile con il modello del repo.

## G. Wiki
Pagine create/aggiornate e navigation links.

## H. Scenarios / Tests / PIE
Mappa completa feature -> scenario -> automation -> evidence.

## I. Assets / Data
Icone, metadata, Data Assets, tag e validator necessari.

## J. Decisions
- ADR updated/created;
- Open Decisions;
- conflitti ancora irrisolti.

## K. Validation
Comandi realmente eseguiti e risultato.

## L. Remaining gaps
Solo gap concreti.

---

# 31. DEFINITION OF DONE DEL CONSOLIDAMENTO

Il lavoro di Claude è Done solo se:

1. `main` è stato controllato prima delle modifiche;
2. non sono state create issue duplicate;
3. non sono stati creati tracker paralleli;
4. Skill Plus è definito in un owner documentale;
5. Wind + Debris è documentato come reference interaction;
6. la roadmap arriva concettualmente alla 1.0 usando il release model reale;
7. Feature Registry è coerente;
8. Epic/Issue sono collegate alle feature;
9. scenari e test sono collegati;
10. UI affordance è tracciata;
11. privacy e determinismo hanno acceptance criteria;
12. Tactical Designer/authoring è tracciato per milestone futura;
13. asset/icon grammar sono tracciati;
14. Wiki è aggiornata come vista;
15. Decision Log/ADR/Open Decisions sono riconciliati;
16. JSON/vista generata deriva dal source canonico;
17. validator passa o i failure sono documentati;
18. ogni nuovo artefatto ha owner e dipendenze;
19. è chiaro cosa è v0.1, cosa è futuro e cosa è bloccato;
20. il report finale permette di risalire:

`Feature -> Issue -> Spec -> Scenario -> Test -> Evidence`

---

# 32. PRIMA IMPLEMENTAZIONE CONSIGLIATA

Non implementare l'intera roadmap insieme.

Primo vertical slice Skill Plus:

```text
Ability Definition
    |
    v
Skill Plus condition
    |
    v
Wind + Debris.Light
    |
    v
Flying Debris trajectory
    |
    v
Damage / transform
    |
    v
TurnLog
    |
    v
Targeting preview + badge
    |
    v
Functional Scenario
    |
    v
Automation + PIE
```

Exit gate:

> Il giocatore seleziona una skill Wind, vede che una cella Debris è compatibile, vede la preview dei frammenti, committa l'azione, il resolver produce deterministicamente Flying Debris, il TurnLog spiega l'esito e il test automatico verifica il risultato.

Solo dopo questo proof procedere con team combo, interaction matrix e authoring avanzato.

---

# 33. COMMIT / PR STRATEGY

Preferire commit focalizzati.

Sequenza indicativa, da adattare al repository:

1. `docs(skill-plus): define interaction grammar and Wind-Debris reference`
2. `docs(roadmap): map Skill Plus milestones and feature dependencies`
3. `chore(tracking): reconcile Skill Plus feature registry and issue links`
4. `docs(wiki): add Skill Plus affordance and combo discovery pages`
5. `test(skill-plus): add scenario and validation mapping`

Se le modifiche sono numerose, usare branch/PR dedicata.

Non mischiare implementazione runtime non richiesta con puro consolidamento documentale, salvo che il task assegnato a Claude includa esplicitamente anche il codice.

---

# 34. PRINCIPIO FINALE

Skill Plus deve diventare una grammatica comune di RefactorTactics, non una lista di eccezioni.

Il giocatore deve poter vedere:

> “Questa skill interagisce con questo elemento.”

Il designer deve poter dichiarare:

> “Questa skill consuma/trasforma/reagisce a questo tag o evento.”

Il resolver deve poter determinare:

> “La condizione è valida in questo boundary; produco questo evento secondario.”

Il TurnLog deve poter spiegare:

> “Il Plus si è attivato perché Wind ha colpito Debris.Light.”

Il sistema di tracking deve poter dimostrare:

> “La feature è specificata, implementata, testata, documentata e verificata.”

Questo è il criterio con cui consolidare docs, roadmap, Epic, Issue e tutti gli altri strumenti del progetto.
