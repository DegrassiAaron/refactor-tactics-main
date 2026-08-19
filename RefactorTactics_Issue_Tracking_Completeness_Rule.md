# Issue Tracking Completeness Rule

> **A chi serve.** Sta nella root perché gli agenti di coding esterni leggono da lì: è il testo integrale
> della regola, autosufficiente, senza rimandi da risolvere. Compagno di
> [`AGENTS.md`](AGENTS.md), che è la guida condivisa e ne porta il richiamo in §*Issue*.
>
> ⚠️ **In caso di divergenza vince l'owner interno**,
> [`docs/technical/tooling/issue-tracking-completeness.md`](docs/technical/tooling/issue-tracking-completeness.md): è la
> versione riscritta sui vincoli reali di questo repository — `feature-registry.yaml` unico owner dello
> stato, la Editor Map che cita gli ID delle voci PIE e mai il loro esito, l'allowlist `.gitignore` prima
> che l'asset esista, `rt_shared_id.py reserve D` per gli ADR, la Wiki come repository separato. Questo
> file resta la formulazione generale; quello dice **dove** atterra in RefactorTactics.
>
> Sono due copie della stessa regola e possono divergere: se ne correggi una, correggi anche l'altra nello
> stesso commit. È il prezzo dichiarato per avere un testo che un agente esterno legge senza contesto.

## Principio

Ogni volta che viene **creata, suddivisa o sostanzialmente modificata una GitHub Issue**, Claude DEVE eseguire un **Tracking Impact Pass**.

Una Issue non deve essere considerata un elemento isolato.

La Issue rappresenta il lavoro eseguibile; tutti gli altri tracking rappresentano ciò che quel lavoro:

- implementa;
- modifica;
- richiede;
- verifica;
- documenta;
- dimostra;
- configura manualmente;
- introduce come contenuto o asset.

La regola fondamentale è:

> **CREATE OR LINK, NEVER IGNORE.**

Per ogni categoria di tracking applicabile Claude deve:

1. cercare prima un elemento esistente;
2. collegarlo alla Issue se appropriato;
3. aggiornarlo se la Issue ne modifica scope o stato;
4. crearne uno nuovo soltanto se non ne esiste uno corretto;
5. dichiarare esplicitamente `N/A` quando la categoria non è applicabile.

Non creare duplicati solo per soddisfare formalmente questa regola.

---

# Tracking Impact Pass obbligatorio

Alla creazione di ogni Issue verificare almeno le seguenti categorie.

## 1. Roadmap / Milestone / Epic

Verificare:

- milestone di appartenenza;
- Epic o parent issue;
- dipendenze da altre Issue;
- Issue bloccanti;
- Issue bloccate;
- ordine raccomandato di implementazione.

Ogni Issue deve essere collocata nel percorso verso una milestone identificabile.

Se appartiene a una Epic esistente:

`LINK`

Se introduce un nuovo gruppo consistente di lavoro:

`CREATE EPIC`

Se è solo un task tecnico locale:

non creare una Epic artificiale.

---

## 2. Feature Map

Chiedersi:

> Questa Issue introduce, completa o modifica una feature percepibile dal gioco o un sistema riutilizzabile?

Se sì:

- collegare la Feature esistente;
- oppure creare una nuova Feature entry;
- aggiornare stato/progresso della Feature;
- indicare quali acceptance criteria della Feature copre la Issue.

Esempi:

- Overwatch;
- Reaction Tactic System;
- Sprint;
- Sound Perception;
- Water/Electric interactions;
- Tactical Camera;
- LOS;
- Team Intent Preview.

Una Feature può essere implementata da molte Issue.

Non creare una Feature diversa per ogni Issue tecnica.

---

## 3. Scenario Map

Chiedersi:

> Esiste uno scenario giocabile o dimostrativo che deve mostrare questa funzionalità?

Se sì:

- collegare uno scenario esistente;
- estenderne gli acceptance criteria;
- oppure creare un nuovo scenario se rappresenta una situazione tattica realmente distinta.

Esempi:

`Overwatch -> enemy crosses cone -> HOLD -> second enemy triggers`

oppure:

`Water + Electricity -> propagation -> deterministic TurnLog`

Lo Scenario deve descrivere il comportamento osservabile, non l'implementazione interna.

---

## 4. Test Tracking

Ogni Issue che cambia comportamento competitivo deve avere una strategia di test.

Verificare la necessità di:

- Automation Test;
- unit/core test;
- Functional Test;
- golden test;
- determinism test;
- network test;
- privacy/canary test;
- packaged test;
- performance test;
- regression test.

Preferire il collegamento a test/scenari esistenti quando coprono già il comportamento.

Una Issue non è completa solo perché "provata in PIE".

Per sistemi deterministici considerare sempre:

`same snapshot + rules + seed -> same result`

Per sistemi di networking considerare sempre:

`unauthorized client receives zero private information`.

---

## 5. Editor Map

Chiedersi:

> Questa Issue richiede lavoro manuale dentro Unreal Editor che Claude non può completare solamente modificando file sorgente?

Se sì, creare o collegare una voce nella **Editor Map**.

Esempi:

- creare Data Asset;
- assegnare mesh/material;
- configurare Blueprint;
- impostare collision;
- creare Niagara/VFX;
- creare Widget Blueprint;
- configurare Enhanced Input;
- modificare una level;
- posizionare Actor;
- impostare GameMode;
- configurare Gameplay Tags attraverso Editor/config;
- verificare proprietà visive.

La Editor Map deve indicare almeno:

- cosa deve fare l'utente;
- dove nell'Editor;
- asset coinvolti;
- prerequisiti;
- risultato atteso;
- come verificare che il task sia completato.

Non nascondere lavoro manuale dentro una normale acceptance criterion.

---

## 6. Asset Tracking

Chiedersi:

> La Issue richiede asset che non fanno parte del codice?

Possibili asset:

- character mesh;
- animation;
- icon;
- texture;
- material;
- VFX;
- sound;
- music;
- UI element;
- font;
- environment prop;
- decal;
- concept/reference;
- Blueprint/Data Asset;
- map asset.

Per ogni asset necessario:

### Se esiste già

Collegare la relativa Asset entry.

### Se deve essere recuperato

Registrare:

- nome/descrizione;
- tipo;
- sorgente;
- URL o percorso di recupero;
- licenza/diritti se rilevante;
- asset temporaneo o definitivo;
- destinazione prevista nel repository.

### Se deve essere creato

Registrare:

- descrizione;
- specifiche;
- formato;
- dimensioni se rilevanti;
- stile/riferimenti;
- destinazione;
- Issue che ne richiede la creazione.

### Se può essere graybox

Indicarlo esplicitamente.

Non bloccare una Issue di gameplay su asset finali quando un placeholder è sufficiente.

---

## 7. Content / Data Tracking

Se la Issue introduce contenuto competitivo data-driven verificare:

- Stable ID;
- Definition Version;
- Gameplay Tags;
- Data Asset;
- catalogo di appartenenza;
- dipendenze;
- validator;
- eventuale manifest/hash.

Esempi:

- AbilityDefinition;
- CharacterDefinition;
- SurfaceDefinition;
- ReactionDefinition;
- ObjectiveDefinition;
- MapDefinition.

Non hardcodare come soluzione permanente un contenuto che appartiene alla pipeline dati.

---

## 8. Wiki / Documentation

Chiedersi:

> Questa Issue introduce o modifica una regola, un sistema, un contratto o una decisione che uno sviluppatore/design deve poter consultare in futuro?

Se sì:

- collegare pagina Wiki esistente;
- aggiornarla;
- oppure creare una nuova pagina se il concetto merita un documento autonomo.

Possibili destinazioni:

- Wiki;
- Docs;
- PDR;
- ADR / Decision Log;
- technical specification;
- gameplay specification.

Le decisioni architetturali o di game design non devono vivere soltanto nel testo di una GitHub Issue.

---

## 9. Decision / ADR

Se durante la Issue viene presa una decisione che:

- cambia un'invariante;
- sceglie tra due architetture;
- modifica una regola competitiva;
- altera la resolution order;
- cambia networking/privacy;
- cambia una convenzione dati;
- influenza diverse Feature;

creare o aggiornare la relativa Decision/ADR.

Non creare ADR per dettagli implementativi banali.

---

## 10. UI / UX Tracking

Se la Feature diventa percepibile o controllabile dal giocatore verificare:

- HUD;
- icone;
- tooltip;
- warning;
- targeting;
- feedback;
- debug visualization;
- combat log;
- stato Confermato / Previsto / Incerto;
- accessibilità.

Se richiede implementazione visuale/manuale, collegare anche Editor Map e Asset Tracking.

---

## 11. Debug / Observability

Per sistemi core verificare se servono:

- log category;
- debug draw;
- console command;
- TurnLog event;
- reason code;
- Unreal Insights scope;
- metriche;
- dump dello stato.

Una Feature competitiva deve poter spiegare **perché** ha prodotto un certo risultato.

---

# Regola di collegamento

Preferire sempre:

`Issue -> existing tracking item`

rispetto a:

`Issue -> new duplicate tracking item`

Una nuova entry viene creata solo quando rappresenta un concetto realmente nuovo.

Esempio:

```text
Issue #412
Implement EnemyEnterArea trigger for Overwatch

Feature:
LINK -> Reaction Tactic System

Scenario:
LINK -> Overwatch Hold/Fire Scenario

Asset:
N/A

Editor Map:
N/A

Automation:
CREATE/LINK -> Reaction Opportunity deterministic test

Wiki:
LINK + UPDATE -> Reaction Tactic System / Overwatch

ADR:
N/A

Milestone:
F2 / Reaction subsystem

Dependencies:
#397 ReactionInstance runtime
#401 Decision Boundary
```

---

# Tracking Completeness Block

Ogni Issue creata da Claude deve contenere o produrre un riepilogo equivalente a:

```text
## Tracking

Milestone: LINK / CREATE / N/A
Epic: LINK / CREATE / N/A
Feature Map: LINK / CREATE / UPDATE / N/A
Scenario Map: LINK / CREATE / UPDATE / N/A
Test: LINK / CREATE / UPDATE / N/A
Editor Map: LINK / CREATE / UPDATE / N/A
Assets: LINK / CREATE / ACQUIRE / N/A
Content/Data: LINK / CREATE / UPDATE / N/A
Wiki/Docs: LINK / CREATE / UPDATE / N/A
ADR/Decision: LINK / CREATE / UPDATE / N/A
UI/UX: LINK / CREATE / UPDATE / N/A
Debug/Observability: LINK / CREATE / UPDATE / N/A
Dependencies: ...
```

`N/A` è valido.

Campo mancante non è valido.

---

# Regola per gli Asset

Quando viene identificato un asset necessario non scrivere solamente:

`Need icon`

Specificare invece almeno:

```text
Asset: Overwatch action icon
Type: UI/Icon
Required by: Issue #...
Status: Missing
Strategy: Acquire | Create | Placeholder
Source: <source/path if known>
License: <if external>
Target path: Content/RefactorTactics/UI/Icons/...
Final required for: <milestone>
Placeholder acceptable: Yes/No
```

---

# Regola per i Test

Quando una Issue modifica simulazione, networking o regole competitive, `Test: N/A` richiede una motivazione esplicita.

Normalmente deve esistere almeno un test.

Per esempio:

```text
Implementation Issue
    |
    +--> Core/Automation Test
    |
    +--> Scenario Map
    |
    +--> Functional/Packaged Test when integration requires it
```

Non è necessario creare tre test se uno solo copre correttamente il rischio.

---

# Regola per Editor Map

La presenza di codice generato da Claude non implica che la Feature sia utilizzabile.

Se per completarla devono ancora essere eseguite operazioni manuali nell'Editor:

> La Issue deve creare o collegare il relativo Editor Task prima di poter essere dichiarata completa.

---

# Controllo prima della creazione

Prima di creare materialmente nuove entry Claude deve cercare:

1. Issue esistenti;
2. Epic;
3. Feature Map;
4. Scenario Map;
5. Editor Map;
6. Asset tracking;
7. test esistenti;
8. Wiki/docs;
9. ADR/Decision Log.

Questo serve a evitare duplicati e frammentazione.

---

# Controllo dopo la creazione

Dopo aver creato Issue e tracking:

1. verificare tutti i link;
2. verificare parent/dependency;
3. aggiornare gli stati delle map interessate;
4. aggiornare eventuali conteggi/progressi;
5. controllare che nessun elemento creato sia orfano;
6. mostrare un breve riepilogo di ciò che è stato creato o riutilizzato.

Formato consigliato:

```text
Created:
- Issue #412
- Automation Test RT.Reaction.Overwatch.EnemyEnterArea

Linked:
- Feature FEAT-REACTION-001
- Scenario SCN-OW-003
- Wiki Reaction-Tactic-System
- Epic #366

Updated:
- Feature Map
- Scenario Map

N/A:
- Assets
- Editor Map
- ADR
```

---

# Definition of Ready della Issue

Una nuova Issue è `Ready` soltanto quando:

- ha scope e acceptance criteria;
- ha milestone/parent quando necessari;
- dipendenze note sono collegate;
- il Tracking Impact Pass è completo;
- asset mancanti sono tracciati;
- lavoro Editor è tracciato;
- test previsto è identificato;
- Feature/Scenario sono collegati quando applicabili;
- documentazione interessata è identificata.

---

# Definition of Done della Issue

Prima di chiudere la Issue Claude deve rieseguire il Tracking Impact Pass.

Verificare che:

- Feature Map rifletta il nuovo stato;
- Scenario Map rifletta ciò che ora è giocabile;
- Editor Task associati siano completati o separatamente tracciati;
- Asset necessari siano disponibili o esplicitamente rinviati;
- test siano presenti e passino;
- documentazione/Wiki sia allineata;
- Decision Log sia aggiornato;
- eventuali dipendenze siano state aggiornate;
- non rimangano tracking item orfani.

**Una Issue chiusa con tracking incoerenti è una Issue incompleta.**

---

# Regola operativa sintetica per Claude

> Whenever you create or substantially modify a GitHub Issue in RefactorTactics, perform a mandatory Tracking Impact Pass. Search existing trackers first. For every applicable domain — milestone/epic, Feature Map, Scenario Map, tests, Editor Map, assets, content/data, Wiki/docs, ADR/Decision Log, UI/UX, debug/observability and dependencies — link an existing item, update it, create it if genuinely missing, or explicitly mark it N/A. Never silently omit a tracking domain and never create duplicate tracking entries simply to satisfy this rule. Re-run the same check before closing the Issue so all project maps remain synchronized.