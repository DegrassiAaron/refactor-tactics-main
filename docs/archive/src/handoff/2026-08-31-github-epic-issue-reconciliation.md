# REFACTORTACTICS — GITHUB EPIC / ISSUE LIVE RECONCILIATION

> `HISTORICAL` · **Materiale NON autorevole.** Sorgente archiviato il **2026-08-31**.
> ⚠️ **Provenienza**: questo kit è arrivato **inline**, come argomento di `/sc:spec-panel`, e non come file.
> Il testo qui sotto è **trascritto verbatim** da quell'argomento: non esiste un originale su disco da cui
> verificarlo con `diff`, ed è la sola differenza rispetto agli altri sorgenti di questa cartella.
> **Referto**: [`../../../roadmap/plans/github-epic-issue-reconciliation-spec-panel-2026-08-31.md`](../../../roadmap/plans/github-epic-issue-reconciliation-spec-panel-2026-08-31.md)

---

Devi eseguire una riconciliazione completa del tracker GitHub di RefactorTactics.

Repository canonico:

`DegrassiAaron/refactor-tactics-main`

Data di riferimento dell'handoff: **2026-08-31**.

IMPORTANTE: i dati sotto sono seed verificati prima dell'esecuzione, NON uno snapshot da applicare ciecamente. Prima di modificare qualunque cosa devi rimisurare GitHub e repository live.

---

# 1. OBIETTIVO

Allineare:

* Epic;
* Issue;
* milestone;
* label;
* parent/child;
* checklist;
* riferimenti alle decisioni;
* riferimenti alle roadmap;
* stato release v0.1;
* ownership cross-release;

eliminando prescrizioni obsolete senza distruggere la storia delle issue.

NON implementare codice gameplay in questo task.

Il risultato deve essere un tracker GitHub coerente con:

1. HEAD corrente del repository;
2. Decision Log / ADR correnti;
3. roadmap CURRENT del repository;
4. issue e PR live;
5. stato reale dei child item.

Google Drive è materiale di design/handoff/provenance, NON il database dello stato operativo.

---

# 2. GERARCHIA DELLE FONTI

Per lo stato operativo:

1. repository HEAD corrente;
2. Decision Log / ADR / owner spec CURRENT;
3. roadmap CURRENT del repository;
4. GitHub Epic / Issue / PR live;
5. Drive come handoff, source e provenance.

Per:

* issue open/closed;
* milestone;
* label;
* parent-child;
* PR;
* commit;
* stato di esecuzione;

GitHub live prevale su snapshot Drive datati.

Non usare un conteggio, una checkbox o uno stato riportato in un documento Drive come prova che oggi sia ancora vero.

---

# 3. PREFLIGHT OBBLIGATORIO

Prima di qualsiasi write:

```text
git status --short
git branch --show-current
git fetch --prune origin
git rev-parse HEAD
git rev-parse origin/main
git log -15 --oneline
gh pr list --state open
gh pr list --state merged --limit 30
```

Leggi inoltre:

* `CLAUDE.md`
* `AGENTS.md`
* `docs/roadmap/roadmap-v0.1.md`
* `docs/roadmap/roadmap-checkpoint.md`
* `docs/roadmap/v0.1-definition-of-done.md`
* `docs/roadmap/v0.1-issue-plan.md`, se esiste ancora
* `docs/roadmap/roadmap-post-v0.1.md`
* `docs/OPEN_DECISIONS.md`
* Decision Log corrente
* ADR/owner spec richiamate dalle Epic interessate.

Non ricreare file rimossi soltanto perché una issue storica li nomina.

---

# 4. FATTI SEED DA RIVERIFICARE LIVE

Al momento dell'handoff risultavano:

## Release root

* #14 — `[EPIC v0.1] Vertical slice 2v2 su hex — release v0.1`
* stato: OPEN
* milestone: #6
* label: `v0.1`, `epic`, `P0`

Rimisura tutto prima di usarlo.

## Tracking ritirato

`D-181` ha ritirato:

* Feature Registry;
* G15.

Il gate set corrente è:

`G1–G14`

NON:

`G1–G15`.

Non ricreare:

* Feature Registry;
* G15;
* tracking paralleli equivalenti.

Cerca nelle issue correnti riferimenti prescrittivi a:

* `feature-registry`
* `Feature Registry`
* `G15`
* `15 gate`
* `15 gates`

e distingui:

* riferimento storico → preservare;
* istruzione corrente → correggere.

---

# 5. AUDIT DELLA ROOT EPIC #14

Rileggi integralmente #14 e tutti i child Epic reali.

Devi ricostruire LIVE:

```text
Issue
E-number
Title
State
Milestone
Labels
Parent
Children
Checkbox in #14
Current release scope
```

Non assumere che la checklist del 30/08 sia ancora aggiornata.

Per ogni Epic figlia:

* confronta checkbox in #14 con stato reale;
* controlla milestone;
* controlla label;
* controlla backlink verso #14;
* controlla che #14 abbia il riferimento inverso;
* verifica se è veramente v0.1 oppure cross-release/post-v0.1;
* verifica se la chiusura è supportata dalla DoD e dalle evidenze.

Se una Epic è chiusa ma una frase storica è vecchia, NON riaprirla automaticamente.

Se una Epic è aperta ma la checkbox della #14 è `[x]`, correggi il tracking dopo aver capito il motivo.

Aggiorna la sezione CURRENT della #14 affinché non presenti Feature Registry/G15 come istruzioni correnti.

Preserva le note storiche datate.

---

# 6. POLICY DI MODIFICA DELLE ISSUE

Regola generale:

`SEARCH → REUSE → UPDATE → LINK → CREATE`

CREATE è l'ultima scelta.

Non creare una nuova Epic perché un documento Drive contiene una sezione autonoma.

Prima di CREATE devi dimostrare che:

* non esiste owner equivalente;
* non esiste Epic cross-release equivalente;
* non esiste issue che possa essere aggiornata;
* non esiste capability già posseduta altrove.

Ogni nuova issue deve avere immediatamente:

* parent;
* backlink;
* milestone corretta;
* label;
* scope;
* out-of-scope;
* dipendenze;
* acceptance falsificabili;
* test;
* PIE quando applicabile;
* packaged validation quando applicabile;
* privacy/security gate quando applicabile.

Nessuna issue orfana.

---

# 7. E49 / CAMERA — RICONCILIAZIONE OBBLIGATORIA

Rileggi completamente:

`#1769 — E49 · Tactical Camera & Map Presentation`

E49 è il live owner del dominio camera/map presentation salvo cambiamenti successivi scoperti durante questo audit.

NON creare Epic camera separate per ogni milestone.

Il piano Drive precedente usava:

* `CAM-WP-01..08`
* `CAM-01..CAM-10`

come ID locali.

Questi ID NON devono essere usati come identità GitHub senza traduzione.

Esiste una collisione di namespace: i `CAM-xx` del repository/E49 hanno significati diversi da alcuni `CAM-xx` del vecchio handoff Drive.

Per l'esecuzione:

**i CAM identifier di E49/repository sono quelli autorevoli.**

Traduci sempre un item Drive nel numero GitHub reale.

Verifica in particolare le correzioni già emerse:

### CAM-WP-01

Il vecchio handoff proponeva:

* E11/#25;
* E21/#286;
* E13;

come candidate ownership.

Ora E49/#1769 è owner camera. Gli altri possono essere dipendenze/consumer/integration owner, non Epic camera principale.

### CAM-WP-05

Il vecchio mapping:

`E40 Gamefeel / E42 Accessibility`

è sbagliato.

Non inferire semantica dal numero E.

Risolvi live gli owner reali.

### CAM-WP-08

Il vecchio handoff proponeva E46.

È sbagliato come launch owner:

* E45/#778 è il gate v1.0;
* E46/#934 è una Epic v0.1 frontend shell.

### D-286

Verifica live che resti valida la promozione v0.1 di:

* #1834 — ZoomAlpha;
* #1835 — camera state;
* #1836 — knowledge-domain navigation;
* #1837 — contextual Home;
* #1838 — edge pan.

Non promuovere automaticamente il resto di E49 nella v0.1.

Verifica inoltre la famiglia Semantic Area Overlay sotto E49, inclusi #1941–#1944, senza creare una seconda Epic "Tactical Grid Overlay" per simmetria.

---

# 8. FORMAT POLICY — D-256 / #333

Rileggi:

`#333 — E32`

Decisione corrente seed:

`D-256`

significa:

* **3v3 = Standard**
* **2v2 = Skirmish / vertical slice**
* **4v4+ = Operations / stress / scale**
* 4v4 NON è lo standard competitivo.

La #333 aveva già una nota correttiva ma manteneva:

* titolo storico `Formato 4v4 competitivo`;
* riferimenti prescrittivi ai "15 gate".

Audit completo della issue.

Correggi come minimo ogni istruzione corrente incompatibile con:

* D-181;
* D-256.

Preserva la spiegazione storica.

Valuta se rinominare la Epic in qualcosa semanticamente corretto, ad esempio:

`E32 · Operations 4v4+ — scala, stress e leggibilità`

ma fallo solo dopo aver verificato riferimenti repository/roadmap che dipendono dal titolo.

Non cambiare E-number.

---

# 9. RANKED / v1.0 — D-259

Rileggi:

`#777 — E44`

e gli owner online/launch correlati.

Decisione seed:

`D-259`

ha spostato post-v1.0:

* ranked queue;
* rating/MMR persistente;
* ranked forfeit/disconnect policy;
* rank/rating mostrati al giocatore;
* MMR nascosto equivalente.

Dentro v1.0 rimane:

* formato competitivo Standard 3v3;
* matchmaking/gioco NON classificato.

Non reintrodurre ranked in:

* E44;
* E45;
* E42;
* altre Epic pre-v1.0

soltanto perché vecchie roadmap o issue lo nominano.

Cerca:

```text
ranked
rating
MMR
classified
classificata
competitive 4v4
4v4 competitivo
```

e riconcilia le prescrizioni obsolete.

---

# 10. CROSS-RELEASE OWNERS DA RIVERIFICARE

Nel controllo del 31/08 risultavano owner nuovi/recenti da non duplicare:

* #1769 — E49 Tactical Camera & Map Presentation
* #1816 — E50 Architecture Hardening
* #1848 — E51 Structural Debris / Rubble
* #1861 — Map Editor 0.1
* #1881 — Resolution Playback & Inspection
* #1937 — Player Event Log & Explainability

Rimisura stato e scope live.

Nota importante:

non tutti i capability owner devono ricevere un E-number.

Un capability cross-release può restare un Epic/owner senza introdurre un nuovo numero E se la governance live lo prevede.

---

# 11. STALE REFERENCE SWEEP

Cerca in TUTTE le Epic aperte e nelle issue release-critical almeno:

```text
feature-registry
Feature Registry
G15
15 gates
15 gate
docs/src/
docs/design/
parallel-batch
check-docs-links.py
CAM-WP-
CAM-01
CAM-02
CAM-03
CAM-04
CAM-05
CAM-06
CAM-07
CAM-08
CAM-09
CAM-10
4v4 competitivo
competitive 4v4
ranked
rating
MMR
```

Per ogni match classificalo:

```text
HISTORICAL_VALID
CURRENT_VALID
CURRENT_STALE
AMBIGUOUS
```

Solo `CURRENT_STALE` va corretto automaticamente.

`AMBIGUOUS` va riportato nell'output finale.

Non fare search-and-replace ciechi.

---

# 12. PARENT / CHILD GRAPH AUDIT

Costruisci una matrice per tutte le Epic:

```text
Parent → Child
Child → Parent
State
Milestone
Labels
Backlink present?
Checkbox coherent?
Decision owner
Roadmap owner
```

Cerca:

* child senza backlink;
* parent senza child;
* issue duplicate;
* milestone incoerenti;
* `post-v0.1` dentro milestone v0.1 senza motivo;
* v0.1 issue fuori milestone #6;
* Epic chiuse con child release-critical aperte;
* child chiuse ma checklist non aggiornata.

Correggi solo quando l'evidenza è univoca.

---

# 13. PR / IMPLEMENTATION EVIDENCE

Non dichiarare "Done" perché:

* una PR esiste;
* un commit esiste;
* una issue è stata chiusa;
* un test era verde su un commit precedente.

Quando una issue rivendica implementazione conclusa verifica:

* PR realmente merged;
* commit incluso in `main`;
* test riferiti al commit corretto;
* eventuale PIE;
* eventuale packaged validation;
* nessuna modifica successiva che invalida l'evidenza.

Se l'evidenza è datata, lasciala datata.

---

# 14. COSA NON DEVI FARE

NON:

* implementare gameplay;
* creare Feature Registry;
* creare G15;
* creare una seconda roadmap;
* creare Epic per simmetria;
* inventare nuovi E-number;
* rinumerare E-number esistenti;
* cancellare note storiche datate;
* riaprire automaticamente issue chiuse perché il prose è vecchio;
* chiudere issue solo perché una checklist sembra completa;
* cambiare milestone future senza owner/decisione;
* assumere che Drive sia più recente di GitHub;
* usare un CAM-ID Drive come se fosse un CAM-ID GitHub;
* reintrodurre ranked nella v1.0;
* promuovere 4v4 a Standard competitivo.

---

# 15. STRATEGIA DI WRITE

Procedi in piccoli gruppi.

Ordine consigliato:

## Wave A — governance

* #14
* Feature Registry/G15 references
* gate terminology
* root parent/child graph

## Wave B — v0.1

* tutte le Epic della #14
* milestone #6
* checkbox
* backlinks
* release labels

## Wave C — structural owners

* E49
* E50
* E51
* Map Editor
* Playback
* Player Event Log

## Wave D — future ladder decisions

* #333 / D-256
* #777 / D-259
* owner launch/online correlati

## Wave E — global stale-reference sweep

* remaining open Epic
* current release-critical issue
* cross-links.

Dopo ogni Wave ricontrolla il diff delle issue modificate.

---

# 16. OUTPUT FINALE OBBLIGATORIO

Produci:

## GITHUB_RECONCILIATION_REPORT

Con:

```text
Repository:
HEAD:
Execution timestamp:

Issues inspected:
Epics inspected:
Issues modified:
Epics modified:
Issues created:
Issues closed:
Issues reopened:
Milestones changed:
Labels changed:
Parent/child links repaired:

No-op verified:
Ambiguities:
Remaining blockers:
```

Poi una tabella:

```text
Issue | Before | After | Why | Evidence | Decision
```

Includi URL GitHub reali.

---

# 17. DRIVE_SYNC_PAYLOAD — OBBLIGATORIO

Alla fine genera anche questo blocco, completamente compilato:

```yaml
DRIVE_SYNC_PAYLOAD:
  verified_at:
  repository:
  head_sha:

  release:
    master_issue: 14
    state:
    milestone:
    gates:
    feature_registry_status:

  changed_epics:
    - issue:
      title:
      old_state:
      new_state:
      milestone:
      labels:
      parent:
      change:
      decision:
      url:

  changed_issues:
    - issue:
      title:
      change:
      parent:
      milestone:
      url:

  no_op_verified:
    - issue:
      reason:

  decisions_confirmed:
    - id: D-181
      effect:
    - id: D-256
      effect:
    - id: D-259
      effect:

  camera:
    owner_issue:
    v01_children:
    cam_identifier_policy:
    overlay_children:

  stale_drive_claims:
    - document_or_claim:
      old:
      current:
      evidence:

  drive_documents_to_update:
    - title:
      requested_change:

  unresolved_conflicts:
    - conflict:
      recommended_action:

  current_open_prs:
    - pr:
      title:
      url:
```

Questo payload deve rappresentare **lo stato DOPO le modifiche GitHub**, non quello prima.

Non ometterlo.

Serve al successivo riallineamento automatico di Google Drive.

---

# 18. DEFINITION OF DONE DI QUESTO TASK

Il task è finito solo quando:

* hai rimisurato GitHub live;
* #14 riflette il child graph reale;
* nessuna istruzione corrente usa Feature Registry/G15 come tracking corrente;
* i gate correnti sono G1–G14;
* parent/child principali sono coerenti;
* E49 è trattata correttamente come camera owner;
* i CAM-ID Drive non contaminano i CAM-ID repository;
* D-256 è coerente nelle issue interessate;
* D-259 è coerente nelle issue interessate;
* non hai creato Epic duplicate;
* ogni modifica ha evidenza;
* hai prodotto `GITHUB_RECONCILIATION_REPORT`;
* hai prodotto `DRIVE_SYNC_PAYLOAD`.

Se scopri che uno dei seed di questo prompt è già cambiato, **vince il live state**: documenta la differenza e continua usando la realtà misurata.
