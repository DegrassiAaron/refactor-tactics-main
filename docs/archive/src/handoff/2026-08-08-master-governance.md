> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md) §«cluster Governance».
>
> Il master descrive in gran parte cose che il repository ha gia', e spesso meglio. **I nove gate di §4
> coincidono alla lettera** con quelli di `feature-registry.yaml`:
> quella sezione si cita, non si riscrive. Cio' che mancava davvero — la relazione Feature ↔ Scenario resa
> **eseguibile** — e' oggi il controllo dello scenario orfano.
>
> ⚠️ **Non applicare** i 13 status di §5: il registro ne ha 10, **derivati dai gate** con regola
> deterministica, e `feature_registry.py validate` fallisce se lo stato dichiarato non regge. Ne' gli 8 stati
> documentali di §2: i tag sono 7, e includono `CANONICAL` e `DELIVERED PLAN` che il master non ha.

# RefactorTactics — Governance / Feature Registry / Roadmap Master Consolidation v0.1

**Data:** 2026-08-09  
**Scope:** source of truth, Feature Registry, Scenario Registry, Roadmap, Wiki, ADR/Decision Log, Epic/Issue, document lifecycle e cleanup delle fonti storiche.

---

# 0. Obiettivo

Evitare che chat, PDR, handoff Claude, workbook, Wiki, Feature Map e Roadmap diventino fonti concorrenti.

La governance deve permettere di rispondere rapidamente a:

```text
Qual è la decisione corrente?
Quale feature la implementa?
In quale milestone si trova?
Quale issue la realizza?
Quale scenario la dimostra?
Quale test la verifica?
Quale pagina Wiki la spiega?
Quale documento è ormai storico?
```

# 1. Ordine di prevalenza

Usare questa gerarchia operativa:

```text
1. Decision Log / ADR approvati correnti
2. Codice e Data/Cataloghi as-built verificati
3. Feature Registry machine-readable corrente
4. Roadmap canonica corrente
5. Scenario Registry / Test Registry
6. Wiki corrente
7. Specifiche master consolidate
8. PDR storici
9. Handoff Claude / chat consolidate
10. Workbook / research / brainstorming storico
```

Una fonte inferiore non può sovrascrivere silenziosamente una superiore.

# 2. Classificazione documenti

Ogni documento importante deve avere uno stato esplicito:

```text
CURRENT
AS_BUILT_REFERENCE
PROPOSAL
OPEN_DESIGN
HISTORICAL
RESEARCH
SUPERSEDED
ARCHIVE
```

E preferibilmente:
- owner/domain;
- last reviewed;
- superseded-by;
- related FeatureIds;
- related DecisionIds.

# 3. Chat lifecycle

Le chat non sono la source of truth.

```text
CHAT
 -> brainstorm
 -> decision candidate
 -> decision/ADR
 -> Feature Registry
 -> Scenario/Test/Roadmap/Wiki
 -> MASTER consolidation
 -> Archive/Delete
```

Una chat può essere eliminata solo quando il contenuto utile è assorbito e i conflitti sono registrati.

# 4. Feature Registry

Il Feature Registry è l'inventario unico delle capability.

Campi minimi consigliati:

```text
FeatureId
Title
Domain
Status
Release/Milestone
OwnerSpec
DecisionRefs[]
Dependencies[]
Gates
IssueRefs[]
ScenarioRefs[]
TestRefs[]
WikiRefs[]
Notes
LastReviewed
```

Gate canonici:

```text
spec
data
runtime
log_debug
automation
scenario
ui_wiki
packaged
network_privacy
```

Non tutte le feature richiedono ogni gate, ma `DONE` richiede tutti quelli applicabili.

# 5. Status

Usare status coerenti:

```text
IDEA
OPEN
PROPOSED
SPECIFIED
DATA_SPEC
DESIGNED
IMPLEMENTING
IMPLEMENTED_PARTIAL
IMPLEMENTED
DEFERRED
FUTURE
BLOCKED
DONE
HISTORICAL
```

`IMPLEMENTED` non equivale automaticamente a `DONE`.

# 6. Scenario Registry

Ogni scenario ha Stable ScenarioId e una PrimaryCategory.

Campi minimi:

```text
ScenarioId
Version
PrimaryCategory
FeatureIds[]
CharacterIds[]
FactionIds[]
Milestone
PurposeTags[]
Automated
ExecutionModes[]
IssueRefs[]
TestRefs[]
WikiRefs[]
Status
```

Relazione obbligatoria:

```text
Feature <-> Scenario
```

Niente scenari orfani.

# 7. Roadmap

La Roadmap non è un secondo Feature Registry.

La Roadmap risponde a:
- quando;
- dipendenze;
- exit gate;
- ordine di rischio.

Il Feature Registry risponde a:
- che cosa esiste;
- status;
- ownership;
- link operativi.

Regola:

```text
Feature Registry = inventory/status graph
Roadmap = delivery order
```

# 8. Epic / Issue

Non inventare numeri o URL.

Workflow:

```text
search existing
 -> update existing
 OR
 -> create missing
 -> write actual ref back to Feature Registry
 -> Roadmap
 -> Scenario Registry
 -> Wiki
```

Ogni issue dovrebbe contenere:

```text
Goal
Scope
Non-goals
Dependencies
Acceptance Criteria
Tests
Definition of Done
Suggested commit
```

# 9. Wiki

La Wiki è la vista leggibile, non la fonte unica dei dati competitivi.

Ogni pagina feature dovrebbe mostrare, quando utile:

```text
Feature ID
Status
Milestone
Epic/Issue
Scenarios
Tests
Dependencies
Last update
```

Separare:
- player-facing explanation;
- design rule;
- implementation notes;
- scenario/test references.

Non duplicare valori numerici competitivi se possono essere letti/generati dal catalogo.

# 10. Decision Log / ADR

Usare ADR/Decision Log per conflitti che cambiano semantica.

Esempi trovati durante cleanup:

## ACTION-TAXONOMY-01
6 vs 8 Universal Actions.

## FACING-01
Pivot per-personaggio vs regola Facing ADR più recente.

## ROSTER-01
Roster storici vs v0.1 Gadget/Phase/Riktor/Wraith e v0.2 Steel/Aurora/Murdock/Kwang.

## TIMEBANK-01
Valori/scope del Decision Time Bank ancora playtest/open.

## HIGHGROUND-01
Nessun bonus numerico generale approvato.

## NOISE-SCOPE-01
Spec completa ma scope milestone reale da verificare.

I conflitti non vanno “risolti” cancellando la fonte precedente senza provenance.

# 11. Master consolidati creati

```text
RT_Reaction_System_Master_Consolidation_v0.1.md
RT_Common_Actions_Master_Consolidation_v0.1.md
RT_Characters_Roster_Master_Consolidation_v0.1.md
RT_Map_Environment_Master_Consolidation_v0.1.md
RT_UI_UX_Master_Consolidation_v0.1.md
RT_Scenarios_QA_Bots_Master_Consolidation_v0.1.md
```

Questi sono documenti di transizione/consolidamento.

Dopo integrazione nel repository, la source of truth deve tornare a ADR + Registry + Roadmap + Wiki + codice/data.

# 12. Feature graph desiderato

```text
Decision
   ↓
Feature
   ├── Dependency Feature
   ├── Roadmap checkpoint
   ├── Epic / Issue
   ├── Scenario
   │     └── Test
   ├── Wiki
   └── Runtime/Data owner
```

Il graph deve essere navigabile in entrambe le direzioni dove utile.

# 13. Definition of Done globale

Una feature non è Done perché funziona in Editor.

DoD:

```text
[ ] decision/spec coerente
[ ] data/schema validato
[ ] runtime nel path reale
[ ] deterministic/privacy invariants rispettati
[ ] TurnLog/debug/explainability
[ ] Automation/Functional test
[ ] scenario visuale/playable quando utile
[ ] Feature Registry aggiornato
[ ] Roadmap aggiornata
[ ] Wiki aggiornata
[ ] packaged validation
[ ] network/privacy test quando applicabile
```

# 14. Naming / Stable IDs

Mai usare display name come identità unica.

Governare:
- FeatureId;
- ScenarioId;
- CharacterId;
- AbilityId;
- MapElementId;
- RulesVersion;
- Content version.

Rename tramite redirect/migrazione esplicita.

# 15. Generated views

Dove possibile, generare viste derivate:

```text
Character page -> related scenarios/features
Feature page -> related scenarios/tests/issues
Scenario browser -> metadata registry
Release checklist -> Feature Registry gates
```

Ridurre copie manuali.

# 16. Audit automatico documentale

Validator/document consistency check dovrebbe cercare:

```text
FeatureId duplicati
ScenarioId duplicati
link rotti
feature senza owner
scenario senza feature
issue senza feature
roadmap ref stale
Wiki ref stale
DONE con gate mancanti
CURRENT e SUPERSEDED simultanei senza redirect
numeric value duplicato in più normative
roster/version mismatch
```

# 17. Classificazione corpus esistente

## CURRENT / operativo
- ADR/Decision Log recenti;
- Feature Registry canonico;
- Roadmap corrente;
- Scenario Registry corrente;
- codice/data catalog corrente;
- Wiki corrente.

## Consolidation / transition
- master creati in questo cleanup;
- handoff recenti ancora da integrare.

## Historical / Research
- vecchi PDR con roster/action model superati;
- vecchi workbook balance non riallineati;
- vecchie guide square-grid;
- vecchie roadmap didattiche;
- spec TestDirector se l'as-built usa CVar+GameMode+runner.

# 18. PDR

I PDR restano utili per architettura e rationale, ma non devono essere letti come stato runtime corrente senza data/status.

Aggiungere banner/header tipo:

```text
Historical baseline — see current Feature Registry / ADR for current status
```

quando necessario.

# 19. Workbook

Workbook character/data recente può essere authoring input se allineato.

Workbook Balance storico:

```text
RESEARCH / HISTORICAL
```

finché non viene rigenerato dalla baseline corrente.

Non leggere direttamente Excel nel runtime competitivo.

# 20. Cleanup policy

## KEEP
Chat attiva che contiene decisioni ancora aperte/non assorbite.

## MASTER
Chat/documento di consolidamento temporaneo.

## ARCHIVE
Storico utile ma da escludere dal contesto operativo.

## DELETE
Informazione completamente assorbita, nessun valore storico significativo residuo.

## CONFLICT
Non eliminare finché l'ADR non chiude la contraddizione.

# 21. CORE target

Il progetto CORE dovrebbe mantenere poche chat operative:

```text
00 — Control Center
01 — Feature Registry & Roadmap
02 — Current Sprint / v0.1
03 — Open Design Decisions
04 — Documentation Consolidation
05 — Release v0.1
```

Le discussioni specialistiche vanno nei domini o vengono archiviate dopo consolidamento.

# 22. Control Center

Contenuto consigliato:

```text
CURRENT VERSION
CURRENT SPRINT
ACTIVE FEATURES
OPEN DECISIONS
BLOCKERS
CANONICAL DOC LINKS
RECENTLY CONSOLIDATED
NEXT VALIDATION GATES
```

Non usarlo per specifiche lunghe.

# 23. Cleanup execution order

```text
1. Resolve CONFLICT items in Decision Log
2. Update Feature Registry
3. Update Scenario Registry
4. Update Roadmap
5. Update Wiki
6. Update Epic/Issues with real refs
7. Run document consistency validation
8. Mark source docs CURRENT/SUPERSEDED/HISTORICAL
9. Move old chats to Archive
10. Delete only fully absorbed chats
```

# 24. Epic suggerita

## Documentation & Project Governance Hardening

Scope:
- source-of-truth hierarchy;
- Feature Registry hardening;
- Scenario relations;
- generated Wiki links;
- doc lifecycle metadata;
- consistency validator;
- archive migration.

# 25. Exit criteria

Governance è consolidata quando:

1. esiste un Feature Registry canonico;
2. esiste un Scenario Registry canonico;
3. Roadmap non duplica lo status graph;
4. ADR chiudono i conflitti principali;
5. Wiki punta alle feature/scenari correnti;
6. issue reali sono linkate senza numeri inventati;
7. PDR/workbook storici sono marcati;
8. nessuna feature Done ha gate mancanti invisibili;
9. document consistency check esiste almeno come procedura;
10. il CORE può essere ridotto a poche chat operative.
