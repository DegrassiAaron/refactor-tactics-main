# RefactorTactics — Checklist implementazione Project Graph v1

> File operativo per Claude. Applicare solo dopo audit della HEAD reale.

## Prima di scrivere

- [ ] `git fetch --all --prune`
- [ ] leggere `origin/main`, non una branch datata
- [ ] controllare PR aperte che toccano `feature-registry.yaml`, generatori o file roadmap
- [ ] eseguire baseline:
  - `python scripts/feature_registry.py validate`
  - `python scripts/feature_registry.py generate --check`
  - `python scripts/feature_registry.py shortlist --check`
  - `python scripts/check-docs-links.py`
- [ ] registrare i conteggi baseline ma NON copiarli in source manuali

## Fase 1 — Schema

- [ ] introdurre `docs/roadmap/workstreams.yaml`
- [ ] introdurre `docs/roadmap/work-items.yaml`
- [ ] documentare owner e non-owner
- [ ] aggiungere `execution_lane` a `editor-sessions.yaml`
- [ ] compatibilità: sessione senza campo = `pie`
- [ ] non introdurre `editor_tasks:` nel Feature Registry

## Fase 2 — Loader e validator

- [ ] loader YAML nuovi
- [ ] validazione execution lane
- [ ] validazione domain group
- [ ] Feature ID esistenti
- [ ] WorkItem ID unici
- [ ] riferimenti work item risolvibili
- [ ] ciclo su `requires` = errore
- [ ] ciclo solo su `related/follows` non è necessariamente errore
- [ ] capability provider mancanti = warning actionable
- [ ] sessione ASSET/PIE orfana = warning
- [ ] test di mutazione per ogni nuova classe di errore

## Fase 3 — project-graph.json

- [ ] aggiungere source `workstreams`
- [ ] aggiungere source `work_items`
- [ ] serializzare execution lanes
- [ ] serializzare domain groups
- [ ] serializzare work items
- [ ] serializzare capabilities
- [ ] serializzare edge tipizzati
- [ ] serializzare inverse edges
- [ ] derivare junction
- [ ] derivare stati READY/BLOCKED/WAITING_FOR_PIE/WAITING_FOR_ASSET/DONE
- [ ] derivare `project_stats`
- [ ] `generate --check` rileva staleness
- [ ] due generate consecutivi = diff vuoto

## Fase 4 — Scenari ed evidenza

- [ ] scenario reale = nodo
- [ ] scenario planned = nodo `PLANNED`
- [ ] scenario con capability mancante = `BLOCKED`
- [ ] scenario classe A non crea PIE
- [ ] scenario classe B collega la verifica umana corrispondente
- [ ] classe C resta lavoro umano
- [ ] scenario → feature inverse lookup
- [ ] test → feature inverse lookup
- [ ] PIE → feature inverse lookup
- [ ] non contare scenario e PIE come la stessa metrica

## Fase 5 — Release

- [ ] mostrare v0.1
- [ ] mostrare v0.2
- [ ] introdurre v0.3/v0.4 nel dominio release solo dopo audit della roadmap owner
- [ ] migrare da `future` soltanto Feature ID con ownership di release inequivocabile
- [ ] tutto il resto resta `future`

## Fase 6 — Control Center

- [ ] nuova vista `Execution Graph` o `Dependency Map`
- [ ] NO force graph globale di default
- [ ] layout DAG / lane
- [ ] filtro release
- [ ] filtro CODE/PIE/ASSET
- [ ] filtro domain group
- [ ] filtro state
- [ ] filtro feature/epic/scenario
- [ ] focus upstream
- [ ] focus downstream
- [ ] drawer con `requires`, `blocks`, `provides`, `features`, `scenarios`, `PIE`, `gate`
- [ ] tutte le derivazioni arrivano dal JSON
- [ ] JS non decide READY/BLOCKED/DONE

## Fase 7 — Migrazione dati

Ordine:
- [ ] thin slice v0.1 su 2-3 catene reali
- [ ] review del modello
- [ ] v0.1 completa
- [ ] v0.2 skeleton
- [ ] v0.3
- [ ] v0.4
- [ ] future

Non fare la migrazione massiva prima che il thin slice sia leggibile nel Control Center.

## Fase 8 — Conteggi

- [ ] totale feature
- [ ] totale work item
- [ ] issue open/closed
- [ ] PIE ready/waiting/done
- [ ] ASSET ready/waiting/done
- [ ] scenari runnable/blocked/planned
- [ ] dependencies hard/soft/capability
- [ ] by release
- [ ] by execution lane
- [ ] by domain group

Tutti derivati.

## Fase 9 — Compatibilità con le vecchie lane

- [ ] confrontare legacy lane 1–7 con nuovo grafo
- [ ] non usare le snapshot come owner
- [ ] non cancellarle subito
- [ ] dopo parità informativa, marcarle superseded/archived o generarle
- [ ] evitare doppio sistema di lane vivo

## Acceptance finale

- [ ] una feature porta in un click a issue/work item/scenari/test/PIE
- [ ] uno scenario torna alle feature che verifica
- [ ] un nodo mostra upstream/downstream
- [ ] è visibile quando CODE aspetta PIE
- [ ] è visibile quando CODE aspetta ASSET
- [ ] capability mancanti spiegano i blocchi
- [ ] v0.1–v0.4 navigabili
- [ ] nessun conteggio manuale nuovo
- [ ] nessun generated editato a mano
- [ ] validator verde
- [ ] generator check verde
- [ ] test Control Center verdi
