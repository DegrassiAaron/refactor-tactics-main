# Checklist — Migrazione Project Graph v0.1 → v0.4

## Baseline
- [ ] fetch/pull `origin/main`
- [ ] controllare PR aperte (in particolare file registry / PIE / project-graph)
- [ ] `feature_registry.py validate`
- [ ] `feature_registry.py generate --check`
- [ ] `feature_registry.py shortlist --check`
- [ ] `node --test docs/control-center/`

## v0.1 completa
- [ ] auto-discover 74 Feature v0.1
- [ ] auto-discover E1-E21
- [ ] auto-discover tutti i checkpoint v0.1
- [ ] collegare issue checkpoint
- [ ] importare dependencies Feature
- [ ] importare prerequisiti espliciti roadmap/issue
- [ ] importare sessioni
- [ ] `execution_lane` sessioni, default PIE
- [ ] U7/U8 ASSET, U9 PIE
- [ ] importare PIE refs
- [ ] importare scenari reali
- [ ] importare planned
- [ ] importare G1-G15
- [ ] hard cycle validation
- [ ] complete-coverage diagnostics
- [ ] Execution Map leggibile con v0.1 completa

## v0.2
- [ ] E22 #323
- [ ] E23 #324
- [ ] E24 #325
- [ ] E25 #265
- [ ] E26 #326
- [ ] E35 #322
- [ ] E36 #435
- [ ] E38 #609
- [ ] E39 #704
- [ ] feature v0.2 tutte raggiungibili
- [ ] E23 standability/transition clearance
- [ ] E26 bot tactical
- [ ] E35 roster v02
- [ ] E36 status framework
- [ ] E38 action economy/validation
- [ ] E39 spatial transfer
- [ ] non trasformare E22/E24/E25 extension in release reassignment della base

## Schema release
- [ ] `RELEASES` include v0.3/v0.4
- [ ] documentazione valori ammessi
- [ ] validator
- [ ] JSON
- [ ] shortlist
- [ ] Control Center filters
- [ ] tests

## v0.3
- [ ] BOT-BELIEF → v0.3
- [ ] BOT-PREDICTIVE → v0.3
- [ ] ACTION-TRAPS → v0.3
- [ ] ACTION-DELAYED → v0.3
- [ ] INTENT-CONDITIONAL → v0.3
- [ ] E27 #327
- [ ] E28 #328
- [ ] E29 #329
- [ ] E33 #330
- [ ] perception base resta v0.1
- [ ] scenari planned/reali collegati

## v0.4
- [ ] CHARACTER-STATE → v0.4
- [ ] audit CHAR-TRANSFORMATION vs CHARACTER-STATE
- [ ] E30 #331
- [ ] E31 #332
- [ ] E32 #333
- [ ] E34 #244
- [ ] E30 feature-binding audit
- [ ] E31 feature-binding audit
- [ ] E32 dedicated-feature decision
- [ ] E37 NON assegnata automaticamente a v0.4

## Generated/views
- [ ] project-graph.json contiene release/feature/execution/evidence
- [ ] project_stats derivati
- [ ] post-v0.1 shortlist generata
- [ ] niente conteggi manuali nuovi
- [ ] staleness include execution source
- [ ] due generate consecutive = no diff

## UI
- [ ] Release filter v0.1..v0.4/future
- [ ] CODE/PIE/ASSET
- [ ] domain
- [ ] readiness
- [ ] Roads default
- [ ] Focus Graph
- [ ] upstream/downstream
- [ ] scenarios/capabilities
- [ ] diagnostics gaps
- [ ] no state derivation in JS
