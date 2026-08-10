> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).
>
> **Recepito in due tempi**: l'attacco base per eroe in
> [`ADR-0007`](../../../decisions/adr-0007-attacco-base-per-eroe.md) (kit dedicato,
> [`2026-08-09-attacco-base-per-eroe.md`](2026-08-09-attacco-base-per-eroe.md)); il **residuo** — fazioni,
> Signature Mechanics, Super e cooldown v0.2, data model — in
> [`brief-super-e-cooldown.md`](../../../gameplay/brief-super-e-cooldown.md), che e' owner della **forma** e
> non dei contenuti: una Super e' una Ability Definition con gate e commit policy, **non** un sistema
> `Ultimate`. Issue `#336`, chiusa dalla PR `#349`.
>
> ⚠️ **Non applicare** lo schema di pagina personaggio di §20 (11 sezioni): il template ne ha 16 e un campo
> obbligatorio che il master non nomina — *Misplay / Failure State*, criterio anti-clone di `D-032`
> ([`_Template.md`](../../../characters/_Template.md)). Le fazioni v0.2 sono decise e non `TBD`:
> Sentinel Directorate e Resonance, non Conflux/Constrine.

# RefactorTactics — Characters & Roster Master Consolidation v0.1

**Data:** 2026-08-09  
**Scope:** roster v0.1/v0.2, identità RT, mapping asset Paragon, Signature Mechanics, fazioni, Base Action Signature, Super Actions/Cooldowns e governance dei character data.

## 1. Fonte di verità operativa

Per il cleanup, preservare questa gerarchia:

```text
Decision Log / ADR più recenti
-> codice + cataloghi correnti
-> piano canonico MVP
-> Character Wiki Data corrente
-> roadmap/feature registry
-> handoff recenti
-> PDR / workbook storici / research
```

Non usare un vecchio PDF o workbook per sovrascrivere il roster corrente.

---

# 2. Roster ufficiale corrente

## v0.1

```text
Flux
Riva
Bastion
Vektor
```

Showcase 2v2:

```text
Flux + Riva
vs
Bastion + Vektor
```

## v0.2

```text
Steel
Aurora
Murdock
Kwang
```

Totale roster ufficialmente pianificato:

```text
8 personaggi
```

Il candidate pool Paragon restante NON appartiene al roster ufficiale finché non viene promosso da una decisione esplicita.

---

# 3. Roster storici da non reintrodurre

Le seguenti famiglie appartengono a fasi precedenti, proposal o documentazione storica:

```text
Aegis / Nyx / Drift / Vex
Mara / Ivo / Nyx / Sol
Kairo / Morrow / Vela / Rook
altri nomi generati in brainstorming non approvati
```

Regola:
- mantenerli solo in archive/history/research;
- nessuna pagina Wiki operativa;
- nessun CharacterId runtime nuovo;
- nessun rename globale.

---

# 4. Identità RefactorTactics vs asset Paragon

Principio:

```text
Gameplay identity = RefactorTactics
Visual implementation = Paragon Asset Slot
```

Mapping approvato:

| RefactorTactics | Paragon Asset Slot | Release |
|---|---|---|
| Flux | Gadget | v0.1 |
| Riva | Phase | v0.1 |
| Bastion | Riktor | v0.1 |
| Vektor | Wraith | v0.1 |
| Steel | Steel | v0.2 |
| Aurora | Aurora | v0.2 |
| Murdock | Murdock | v0.2 |
| Kwang | Kwang | v0.2 |

Non rinominare:
- Flux -> Gadget
- Riva -> Phase
- Bastion -> Riktor
- Vektor -> Wraith

Il mapping Paragon riguarda mesh/animazioni/portrait/VFX/reference visuale, non l'identità gameplay.

Per v0.2 i nomi correnti coincidono con gli slot Paragon, ma non assumere che siano necessariamente i nomi retail definitivi se il repository li mantiene come working names.

---

# 5. Character IDs e release governance

Per v0.1 usare gli ID canonici già presenti nel progetto, ad esempio:

```text
Hero.Flux
Hero.Riva
Hero.Bastion
Hero.Vektor
```

Per v0.2:
- non inventare ID definitivi se restano TBD;
- separare `WorkingName`, `ParagonAssetSlot`, `FinalRTCharacterId`;
- promuovere un candidate richiede aggiornamento coordinato di data, wiki, roadmap, scenari e validator.

---

# 6. Signature Mechanics — baseline corrente

## v0.1

### Flux
```text
Signature: Conduction / Charge / Conductive Network
Role: Controller / Striker
```

Identità:
- elettricità;
- reti conduttive;
- setup e propagazione;
- forte sinergia con Wet/acqua;
- valore derivato anche dalla mappa.

### Riva
```text
Signature: Water Shaping / Flow / Wet Territory
Role: Support / Controller
```

Identità:
- creazione/modifica di acqua;
- displacement;
- Wet;
- setup di combo;
- controllo del terreno.

### Bastion
```text
Signature: Field Architecture / Directional Structures / Protection
Role: Guardian / Controller
```

Identità:
- cover e strutture direzionali;
- protezione;
- canalizzazione delle rotte;
- controllo degli archi/transizioni.

### Vektor
```text
Signature: Predictive Interception
Role: Striker / Controller
```

Identità:
- previsione;
- intercettazione di traiettorie;
- cell/line lock;
- payoff alto contro movimento prevedibile;
- whiff come trade-off reale.

## v0.2

### Steel
```text
Signature: Guard Meter / Effective Protection
Role: Guardian / Controller
```

### Aurora
```text
Signature: Frozen Domain
Role: Controller / Striker
```

### Murdock
```text
Signature: Focus + Fire Sector
Role: Marksman / Controller
```

### Kwang
```text
Signature: Electric Anchor
Role: Bruiser / Controller
```

Le Signature sono direzione di design; numeri e payload finali dipendono dai cataloghi/playtest.

---

# 7. Fazioni / gruppi

Baseline confermata per la showcase v0.1:

```text
CONFLUX
- Flux
- Riva

CONSTRINE
- Bastion
- Vektor
```

Per v0.2:

```text
Steel
Aurora
Murdock
Kwang
```

il mapping Conflux/Constrine non è sufficientemente confermato nelle fonti recuperate.

Quindi:

| Character | Release | Faction |
|---|---|---|
| Flux | v0.1 | Conflux |
| Riva | v0.1 | Conflux |
| Bastion | v0.1 | Constrine |
| Vektor | v0.1 | Constrine |
| Steel | v0.2 | TBD |
| Aurora | v0.2 | TBD |
| Murdock | v0.2 | TBD |
| Kwang | v0.2 | TBD |

Non inferire la fazione da:
- ruolo;
- palette;
- asset Paragon;
- vecchio lore;
- somiglianza meccanica.

---

# 8. Character Base Action Signature

Ogni personaggio deve essere riconoscibile anche fuori dalle 4 signature ability.

Profilo:

```text
Move / movement profile
Special movement
Basic Attack
Guard
Brace
Overwatch
Activate / Interact affinity
Wait behavior se esplicito
Facing/orientation behavior entro le regole canoniche
```

Obiettivo:

> anche con le signature ability indisponibili, il personaggio deve continuare a “sentirsi” se stesso.

Non trasformare però le azioni universali in altre 6 signature ability.

La complessità deve restare leggibile.

---

# 9. Direzioni Base Action v0.1

Sono direzioni di prototipo/playtest, non numeri canonici.

## Flux
- Move: cerca geometrie utili alla conduzione;
- Basic Attack: Engine/Setup Attack elettrico;
- Guard/Brace: possibile grounding/carica;
- Overwatch: profilo conduttivo/settoriale;
- Activate: alta affinità con dispositivi elettrici.

## Riva
- Move: fluido e legato alle superfici;
- Basic Attack: setup Wet/displacement;
- Guard/Brace: evasione/flow dove definito;
- Overwatch: pressione/spinta/Wet;
- Interact: forte relazione con valvole/acqua/ambiente.

## Bastion
- Move: pesante/stabile;
- Basic Attack: utility/fallback/finish;
- Guard: identità forte di protezione;
- Brace: forte anti-displacement;
- Overwatch: controllo frontale;
- Activate/Interact: cover/porte/strutture.

## Vektor
- Move: agile/predittivo;
- Basic Attack: Primary Weapon;
- Guard/Brace: line control/focus più che tanking;
- Overwatch: settore stretto/lungo/intercettazione;
- Interact: standard salvo kit specifico.

Le vecchie matrici Pivot per eroe restano proposal se confliggono con ADR Facing più recente.

---

# 10. Kit v0.1 — principio

Ogni personaggio v0.1 deve avere:
- kit fondamentale riconoscibile;
- almeno 4 signature ability secondo il catalogo corrente;
- almeno una relazione con ambiente/mappa;
- counterplay;
- moving-target policy;
- scenario coverage;
- TurnLog/explainability.

Non duplicare qui i payload numerici: devono vivere nel catalogo/data source competitivo.

---

# 11. Combo team v0.1

## Flux + Riva

Pattern principale:

```text
Riva crea Wet / acqua
-> Flux sfrutta conduzione / propagazione elettrica
```

La combo deve:
- avere setup visibile;
- consentire counterplay;
- poter coinvolgere friendly-fire/rischio se previsto;
- non diventare kill garantita.

## Bastion + Vektor

Pattern principale:

```text
Bastion modifica cover/rotte
-> Vektor sfrutta traiettorie prevedibili / Intercept
```

La combo deve premiare la canalizzazione senza eliminare tutte le opzioni nemiche.

---

# 12. Scenari personaggio minimi

Ogni character deve avere almeno:

```text
Signature.HappyPath
Signature.Counterplay
Signature.Boundary
Ability.Core
Interaction.Team
Determinism.Repeat
```

Namespace consigliato:

```text
Character.Flux.*
Character.Riva.*
Character.Bastion.*
Character.Vektor.*
Character.Steel.*
Character.Aurora.*
Character.Murdock.*
Character.Kwang.*
```

Non legare il namespace alla fazione se il mapping di v0.2 è TBD.

---

# 13. Scenario team/fazione

## v0.1

```text
Team.Conflux.WaterElectric
Team.Constrine.ArchitectureInterception
Showcase.v0.1.FluxRivaVsBastionVektor
```

Ogni scenario deve poter essere scoperto da metadata:
- CharacterIds;
- FactionIds;
- FeatureIds;
- milestone/version;
- purpose.

Niente liste manuali duplicate nella Wiki se il registry può derivarle.

---

# 14. Super Actions — v0.2 design baseline

Le Super appartengono alla v0.2 design baseline, non devono gonfiare la v0.1.

Principio:

> Una Super è il massimo commitment della Signature del personaggio, non solo un attacco grosso con cooldown lungo.

Ogni Super deve avere almeno uno, preferibilmente più gate:

```text
Resource Gate
State Gate
Environment Gate
Geometry Gate
Prediction Gate
Setup Gate
Risk / Recovery
```

La Super usa il normale Ability Framework.

Non creare un secondo motore `Ultimate`.

```text
Ability Definition
+ Super metadata/tags
+ Requirements
+ Costs
+ Cooldowns
+ Commit Policy
+ Resolver
= Super Action
```

Il resolver deterministico resta autorità.

---

# 15. Cooldown System — v0.2

Direzione consolidata:

Supportare:
- cooldown individuale;
- shared cooldown group;
- charges;
- recovery;
- risorse;
- eventuali lockout dichiarati.

Non introdurre un global cooldown obbligatorio.

I cooldown avanzano in turni logici/Cleanup, non con il tempo delle animazioni.

Commit policy:
- dopo il commit, whiff/fizzle può comunque consumare cooldown se la definition lo dichiara;
- il TurnLog deve spiegare consumo e motivo.

---

# 16. Super + phases

Una Super “lenta” non dipende dalla durata dell'animazione.

Esempio:

```text
Planning -> commit
Prep -> telegraph/setup
Dash -> il campo può cambiare
Blast -> payoff
Move -> eventuale limitazione
Cleanup -> cooldown/recovery
```

---

# 17. Data model character

Il modello deve poter descrivere almeno:

```text
RTCharacterId
DisplayName
ReleaseVersion
ImplementationStatus

Role
SubRole
PrimarySignature
SecondaryMechanic
PlayerQuestion

BaseActionProfile
MovementProfile
ReactionProfile
OverwatchProfile

Ability01..04
SuperAction      # v0.2+

PersonalResource
CooldownGroups

EnvironmentAffinity
Counterplay
TeamSynergy

ParagonAssetSlot
```

Non aggiungere campi runtime solo perché compaiono qui: riusare strutture esistenti e introdurre nuovi campi solo per casi concreti.

---

# 18. Data governance

Pipeline preferita:

```text
Authoring dataset
-> validator/generator
-> Primary Data Assets / cataloghi
-> manifest/hash
-> runtime
```

La Wiki può essere generata/derivata.

Il runtime competitivo NON deve leggere direttamente Excel.

Validator:
- CharacterId unico;
- release coerente;
- mapping asset valido;
- ability references valide;
- tag governati;
- nessun candidate promosso accidentalmente;
- v0.1/v0.2 status coerente.

---

# 19. Workbook status

## Character Wiki Data v0.4
È molto più vicino alla baseline corrente:
- roster ufficiale 8;
- v0.1 = Flux/Riva/Bastion/Vektor;
- v0.2 = Steel/Aurora/Murdock/Kwang.

Può essere usato come authoring/reconciliation input se il repository lo considera ancora attivo.

## Balance Matrices v0.1
Contiene materiale storico incompatibile:
- roster precedenti;
- Fast Reaction 5–7s;
- action model precedente;
- timing vecchi.

Classificarlo:
```text
RESEARCH / HISTORICAL
```
oppure rigenerarlo in una nuova versione allineata.

Non usarlo come source of truth corrente.

---

# 20. Wiki

Devono esistere pagine operative solo per gli 8 personaggi ufficiali.

Sezione standard:

```text
Identity
Role / Signature
Base Action Signature
Abilities
Reaction / Overwatch
Environment Affinity
Counterplay
Team Synergy
Related Scenarios
Feature / Roadmap Status
Asset / Provenienza visuale
```

Esempio asset:

```text
RefactorTactics Character: Flux
Paragon asset base: Gadget
Release: v0.1
```

Non scrivere “Flux è Gadget”.

---

# 21. Feature Registry

Consolidare, senza duplicare, capability equivalenti a:

```text
Character Roster
Character Definition
Character Base Action Signature
Character Signature Mechanics
Character Visual Asset Mapping
Character Reaction Profile
Character Overwatch Profile
Character Super Action          # v0.2
Layered Cooldowns               # v0.2
Faction / Group Metadata
Character Scenario Coverage
```

Ogni feature deve puntare a:
- roadmap;
- scenarios;
- tests;
- wiki;
- implementation status.

---

# 22. Test

## Data
- 8 official roster entries;
- 4 v0.1 + 4 v0.2;
- no candidate has release by accident;
- unique CharacterId;
- asset mapping non-empty;
- v0.1 IDs canonical;
- v0.2 TBD preserved where applicable.

## Gameplay
Per ogni v0.1:
- signature happy path;
- counterplay;
- boundary;
- each core ability;
- base action signature;
- team combo.

## Determinism
- repeat same scenario -> same TurnLog/state hash.

## Wiki/docs
- only 8 official operative pages;
- no stale v0.1 roster;
- visual mapping correct;
- historical names only under archive/history.

---

# 23. Telemetry/playtest v0.2

Per Super/Cooldown pianificare metriche come:
- Super availability turn;
- uses/match;
- commit rate;
- whiff/fizzle rate;
- effective value;
- affected units/cells;
- recovery turns;
- turns ability unavailable;
- shared cooldown conflicts;
- charges wasted.

Per signature:
- Flux/Kwang: network/propagation;
- Riva/Aurora/Bastion: cells/edges modified;
- Vektor/Murdock: prediction/reaction opportunities;
- Steel: effective protections.

Non trasformare queste metriche in production telemetry obbligatoria per la v0.1.

---

# 24. Cleanup chat

Dopo integrazione canonica diventano candidate ad Archive/Delete:

```text
Focus personaggi v0.1
Nomi Paragon e RefactorTactics
Meccaniche personaggio uniche
Super colpi e cooldown
Fazioni
```

`Artwork Paragon 0.1` va in Archive/Art, non nel CORE.

La Character Master Matrix può restare come authoring/design matrix se viene allineata e chiaramente marcata CURRENT/PROPOSAL per colonna.

---

# 25. Documenti da correggere/archiviare

1. qualunque `v0.1 = Steel/Aurora/Murdock/Kwang` -> superseded.
2. Aegis/Nyx/Drift/Vex operativi -> archive/history.
3. Mara/Ivo/Nyx/Sol operativi -> archive/history.
4. vecchi mapping asset di Flux diversi da Gadget -> historical.
5. workbook Character Wiki v0.2 con v0.1 sbagliato -> superseded da v0.3/v0.4.
6. `Roster_8_Conflux_Constrine.md` non corretto -> archive/superseded.
7. `Roster_8_Conflux_Constrine_CORRETTO.md` -> utile come history/recovery, ma v0.2 faction mapping resta TBD.
8. Balance Matrices v0.1 -> RESEARCH/HISTORICAL fino a riallineamento.

---

# 26. Epic suggerite

## Character Roster Canonicalization
- roster 8;
- releases;
- Stable IDs;
- candidate governance;
- wiki generation;
- validator.

## Character Visual Asset Mapping
- RT identity -> Paragon asset slot;
- content references;
- artwork/wiki;
- migration test se runtime usa nomi Paragon.

## Character Base Action Signature
- profiles per v0.1;
- Basic Attack roles;
- Guard/Brace/Overwatch;
- scenarios/playtest.

## v0.1 Signature Validation
- Flux Conduction;
- Riva Water;
- Bastion Architecture;
- Vektor Prediction;
- team combos;
- counterplay.

## v0.2 Character Data Spec
- Steel/Aurora/Murdock/Kwang;
- signature validation;
- faction TBD closure;
- final RT identity review.

## v0.2 Super Actions & Layered Cooldowns
- shared framework;
- 8 Super baselines;
- cooldown groups/charges/recovery;
- TurnLog;
- scenarios/telemetry.

---

# 27. Exit criteria

Il cluster Characters è consolidato quando:

1. una sola fonte current elenca gli 8 personaggi;
2. v0.1/v0.2 sono coerenti ovunque;
3. mapping Paragon è separato dall'identità RT;
4. vecchi roster sono historical/archive;
5. Conflux/Constrine v0.1 è chiaro;
6. v0.2 faction mapping è esplicitamente TBD o deciso;
7. Signature Mechanics sono uniformi fra Data/Wiki/Feature Registry;
8. Base Action Signature è collegata al Common Actions Master;
9. Super/Cooldowns sono chiaramente v0.2;
10. ogni v0.1 character ha scenari minimi;
11. le chat character-specific consolidate possono uscire dal CORE.
