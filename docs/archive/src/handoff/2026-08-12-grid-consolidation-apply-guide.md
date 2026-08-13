# APPLY_WITH_CLAUDE.md — Consolidamento 2026-08-12

## Scopo

Applicare al repository `DegrassiAaron/refactor-tactics-main` le decisioni recenti su:

- hex geometry / muri / standability;
- acqua dinamica e rete conduttiva;
- connector/bridge/Chain Lightning;
- hazard elettrici persistenti futuri;
- Planning Preview visual grammar;
- governance di Feature/Scenario/Milestone/Editor Map;
- README/agent instructions;
- workbook/data governance.

**IMPORTANTE:** questo bundle è stato preparato dopo lettura di `main`, ma il connettore disponibile non ha
permesso di creare un branch remoto (`403 Resource not accessible by integration`). Claude deve quindi
**rileggere HEAD prima di applicare** e non assumere che nessun altro abbia modificato gli stessi file.

## 1. Preflight obbligatorio

1. `git fetch --all --prune`
2. verificare `git status`;
3. creare worktree/branch focalizzato, per esempio:
   `docs/consolidate-water-planning-ui-2026-08-12`;
4. leggere `AGENTS.md`, `CLAUDE.md`, Decision Log, conflict matrix, open decisions;
5. cercare feature/spec/issue già esistenti prima di crearne altre;
6. verificare che non ci siano worktree paralleli che toccano `feature-registry.yaml`,
   `editor-sessions.yaml`, HUD owner o Wiki water/electric.

## 2. Root files

Confronta i tre replacement del bundle con HEAD e applica **semantic merge**, non overwrite cieco:

- `repo-root/CLAUDE.md`
- `repo-root/AGENTS.md`
- `repo-root/README.md`

Obiettivi:
- rimuovere dal README lo stato quadrato/stale e conteggi test hardcoded;
- mantenere agent docs durevoli e non trasformarli in spec lunghe;
- aggiungere governance source-vs-generated;
- pin D-071, D-081, D-082 e la relazione CP8.3/CP9.4.

## 3. Nuove spec da integrare

Proposte:

- `docs/gameplay/spec-rete-conduttiva-e-hazard-elettrici.md`
- `docs/technical/spec-planning-preview.md`

Prima di aggiungerle:
- cercare owner già equivalenti;
- se l'owner esiste, incorporare la parte nuova lì oppure usare la nuova spec come sub-owner linkato;
- evitare due documenti normativi per la stessa decisione.

### Regole da preservare

**D-071** supersede il vecchio test center-only per standability.

**D-081** supersede la proposta di `WaterDepth` come asse separato:
future depth = composite surfaces finché non viene deliberatamente sostituita.

**D-082**: structural slot future = `Bulkhead`, non `BreachSlot`.

CP8.3:
- BFS conductive;
- `PropagationLimit` steps;
- cell conductivity, not unit Wet;
- instantaneous discharge;
- no unit `Status.Electrified`.

CP9.4:
- `FRTHexEdge::bConductsElectricity`;
- conductive arcs can cross Layer;
- inactive/nonconductive arc stops propagation.

Quindi **non creare un secondo graph engine** se arcs/GraphNeighbors possono esprimere connector e conduzione.

## 4. Feature Registry / Feature Map

`docs/roadmap/feature-registry.yaml` è la sorgente.

### Cercare e aggiornare, non duplicare

Almeno:
- `RT-FEAT-MAP-WATER-DYNAMICS`
- feature ambiente/elettricità già esistente
- `RT-FEAT-MAP-STRUCTURAL`
- `RT-FEAT-MAP-VERTICALITY`
- `RT-FEAT-UI-PLANNING`
- `RT-FEAT-UI-CERTAINTY`
- `RT-FEAT-UI-ACTION-GHOSTS`

Per `RT-FEAT-MAP-WATER-DYNAMICS`:
- correggere eventuale nota legacy che descrive `WaterDepth` come asse ortogonale;
- linkare D-081 e composite surfaces;
- aggiungere flooding/current come future design, senza spostare in v0.1;
- distinguere runtime già presente (dynamic surfaces) da future depth/current.

Per electric:
- annotare ConnectedConductiveGraph come concetto;
- CP8.3 + CP9.4 già coprono cell + conductive arcs;
- connector multi-port / long conductor segmentation / persistent hazard sono FUTURE;
- evitare feature duplicate se possono stare nell'owner electric esistente.

Per UI:
- collegare `spec-planning-preview.md`;
- registrare semantic grammar e preview-resolver parity;
- non associare automaticamente `PIE-V01-HUD` a feature che il testo della verifica non prova.

### Rigenerare

Usare i comandi reali presenti nello script, tipicamente:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate
python scripts/feature_registry.py shortlist
python scripts/feature_registry.py wiki
```

Poi i corrispondenti `--check`.

Non editare a mano:
- `feature-registry.json`
- `featuremap.shortlist.md`
- `scenariomap.shortlist.md`
- `milestonemap.shortlist.md`
- `editormap.shortlist.md`
- project graph / wiki derivate.

## 5. Roadmap / Milestone Map

Non spostare future water dynamics dentro v0.1 solo perché il design è più completo.

Aggiornare:
- `roadmap-post-v0.1.md` / epic owner pertinente;
- `roadmap-checkpoint.md` solo se cambia uno stato **misurato**;
- milestone generated view solo tramite registry.

Separare chiaramente:
- già implementato CP8.3/CP9.4;
- design consolidato ma non implementato;
- decisioni ancora future.

## 6. Scenario Map

`docs/technical/scenario-map.md` decide **chi verifica cosa**.
`scenario-index-e-tag.md` decide identità/tag.

Non creare `SCN-*` legacy: rispettare la convenzione `Spec.<Area>.<Name>`.

Prima di aggiungere scenari, cercare equivalenti.

Candidates:
- `Spec.Environment.ConductiveBridgeExtendsPropagation`
- `Spec.Environment.ConductiveGraphCrossesLayer`
- `Spec.Environment.NoArcAcrossGap`
- `Spec.Environment.LongConductorConsumesSteps`
- `Spec.Environment.PersistentElectricHazardReevaluatesGraph`
- `Spec.Environment.PersistentElectricHazardOnEnter`
- preview tests/scenarios per Move/Attack/Chain/NoEnemyOverwatchLeak/CVD.

Persistent hazard scenarios restano `planned`/future finché la capability non esiste.

Non aggiornare a mano i conteggi nel Scenario Map se il generator li possiede.

## 7. Editor Map

Live source:
`docs/roadmap/editor-sessions.yaml`

Generated:
`docs/roadmap/editormap.shortlist.md`

Aggiungere sessioni manuali solo se davvero richiedono un occhio, per esempio:
- Planning semantic overlay / focus per step;
- CVD readability;
- conductive graph debug visualization;
- connector/branch explanation;
- Layer conductive bridge visual explanation.

Non riattivare il corpo storico di `roadmap-editor.md` come tracker.

## 8. Wiki

Cercare owner/pagine esistenti e aggiornare almeno:
- `docs/wiki/meccaniche/acqua-e-elettricita.md`;
- pagina Planning/azioni/come-si-gioca pertinente;
- eventuale pagina mappa/geometria.

Regole:
- Wiki spiega, non possiede valori;
- numeri 20/12/PropagationLimit devono arrivare dagli owner/cataloghi, non essere una copia incontrollata;
- distinguere “current runtime” da “future design”.

## 9. Excel / dati

### NON fare

Non “correggere” `docs/balance/RefactorTactics_Balance_Matrices_v0.1.xlsx` cella per cella.
Per D-023 / `docs/balance/README.md` è **RESEARCH**, non fonte competitiva.

Non inserire acqua/connector/HUD nel Character Wiki workbook:
`docs/characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx` è dataset character-authoring.

### Fare

- Verificare che i cataloghi Markdown contengano i numeri correnti.
- Se serve un workbook aggiornato, creare **un output derivato/generato** dai cataloghi, con provenance e
  versione, non promuovere il research workbook a canone.
- Se una decisione cambia una ability specifica di Flux/Riva, aggiornare prima il catalogo owner e poi
  rigenerare/aggiornare il dataset Character Wiki come derivato.

Il bundle include un audit workbook separato: non è una nuova fonte di gameplay.

## 10. Issue / Epic

Cercare prima:
- E8 / #22, CP8.3 #66;
- E9 / bridge owner;
- E23 geometry;
- `RT-FEAT-MAP-WATER-DYNAMICS`;
- UI planning issues (#77, #78, #613 e correlate).

Aggiornare issue esistenti se semanticamente equivalenti.
Creare nuove issue solo per delta non coperto:
- connector generalized / long conductor segmentation;
- persistent electric hazard;
- Planning Preview grammar/parity;
- future water current/depth implementation.

Non inventare numero epic/checkpoint: assegnare secondo governance del repository.

## 11. Test / verification gates

Minimo documentale:
- link check;
- registry validate/generate/shortlist/wiki + checks;
- grep per termini superseded:
  - `WaterDepth` come field/axis normativo;
  - `BreachSlot`;
  - center-only standability;
  - hidden enemy Overwatch preview;
  - hardcoded test count in root README.

Se tocca codice:
- test CP8.3/CP9.4 correlati;
- preview parity;
- privacy no-leak;
- determinism;
- build Editor/Game;
- PIE/packaged solo quando il gate lo richiede.

## 12. Commit suggeriti

```text
docs(core): align agents and readme with current project control
docs(environment): specify conductive network and persistent electric hazards
docs(ui): define planning preview visual grammar
docs(roadmap): align feature scenario milestone and editor maps
docs(wiki): reconcile water electricity and planning guidance
```

Non comprimere tutto in un mega-commit se i file toccano owner diversi.

## 13. Report finale richiesto

Restituire:
- HEAD/branch usati;
- file changed;
- decisioni superseded/corrette;
- feature IDs aggiornati;
- issue/epic create/update;
- scenario planned/real;
- editor sessions;
- wiki;
- generator/check output;
- Excel: cosa è stato **volutamente non modificato** e perché;
- test/build/PIE realmente eseguiti;
- limiti residui.
