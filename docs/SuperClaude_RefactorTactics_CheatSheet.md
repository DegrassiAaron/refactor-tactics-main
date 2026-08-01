# SuperClaude — RefactorTactics Quick Reference

## Documentali: non modificano il codice

| Comando | Uso |
|---|---|
| `/sc:brainstorm` | Chiarire requisiti |
| `/sc:research` | Ricerca web |
| `/sc:design` | Architettura, API, schemi |
| `/sc:workflow` | Ordine di implementazione |
| `/sc:spawn` | Epic → Story → Task |
| `/sc:analyze` | Audit del codice |
| `/sc:estimate` | Stima |
| `/sc:spec-panel` | Review tecnica |
| `/sc:business-panel` | Review strategica |
| `/sc:troubleshoot` | Diagnosi, salvo `--fix` |

## Esecutivi: possono modificare

| Comando | Uso |
|---|---|
| `/sc:implement` | Task piccolo e definito |
| `/sc:task` | Task multidominio |
| `/sc:improve` | Refactoring/ottimizzazione |
| `/sc:cleanup` | Rimozione controllata |
| `/sc:test` | Test |
| `/sc:build` | Build |
| `/sc:git` | Git |
| `/sc:troubleshoot --fix` | Fix dopo diagnosi |

## Scelta rapida

```text
Idea vaga -> brainstorm
Ricerca esterna -> research
Struttura -> design
Ordine -> workflow
Epic enorme -> spawn
Codice piccolo -> implement
Task multidominio -> task
Bug -> troubleshoot
Audit -> analyze
Test -> test
Build -> build
Migliorare -> improve
Pulire -> cleanup
Documentare -> document
Verificare -> reflect
Commit -> git
```

## Workflow feature

```text
brainstorm
-> design
-> spec-panel
-> workflow
-> spawn (solo se grande)
-> implement/task
-> test
-> build
-> analyze
-> document
-> reflect
-> git
```

## Regole RefactorTactics

- Server autoritativo.
- Simulazione deterministica.
- I planning intents sono replicati solo agli alleati.
- C++ e Blueprint sono lo stack predefinito.
- C# è un riferimento didattico, non una dipendenza automatica.
- MVP prima di roguelike, deckbuilding e modding.
- Nessun comando remoto o distruttivo senza richiesta esplicita.
