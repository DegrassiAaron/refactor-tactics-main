# Executive Summary
Assumi il ruolo di modello IA (es. Claude) per **revisionare e realizzare la roadmap v0.1** di RefactorTactics (UE5). L’obiettivo: vertical slice 2v2 su griglia hex multilivello (4 personaggi, abilità, ambienti, obiettivi dinamici, HUD, log). Produci i prodotti attesi e la pianificazione delle issue come indicato.

## Prodotti attesi
- `docs/design/roadmap-v0.1.md`: roadmap completa (Epics + checkpoint).  
- `docs/design/v0.1-definition-of-done.md`: gate di release, KPI, checklist.  
- `docs/design/v0.1-issue-plan.md`: titoli e body per issue.  
- Aggiorna `docs/design/roadmap-checkpoint.md`, `README.md`, `docs/design/test-manuali-pie.md`.  

## Analisi Preliminare
Esamina `README.md`, `CLAUDE.md`; i doc in `docs/design/` elencati; `docs/PDR/*`; il codice in `Source/RefactorTactics*/`; `Content/RT/`, `Config/`, `RefactorTactics.uproject`.

## Principi Obbligatori
- Il simulatore decide, UI/animazioni mostrano.  
- Determinismo (stessa snapshot = stesso risultato).  
- Coordinate intere (no float).  
- No gameplay quadrato parallelo.  
- Offline only (no networking).  
- DoD chiaro e test automatici per checkpoint.  

## Struttura Epic/Checkpoint
Ogni Epic/checkpoint deve includere: ID stabile, titolo, descrizione, file coinvolti, DoD misurabile, test automatici, controlli PIE (se servono), rischi, criterio di chiusura.

## Branch e commit
Branch di lavoro: `docs/v0.1-roadmap-review`. Commit focalizzati e convenzionali (`docs: ...`, `fix: ...`, ecc.).

## Issue GitHub
- **Se permessi disponibili**: crea l’Epic principale e le sue figlie (titoli `[EPIC v0.1]`), assegna a `DegrassiAaron`, collega le dipendenze.  
- **Se permessi mancanti**: crea `v0.1-issue-plan.md` con titoli e body Markdown per ogni issue. Mostra all’utente: “Permessi **Issues Read/Write** necessari su questo repo.”

## Tabelle Richieste
- Mappatura **feature → file → stato** attuale.  
- Elenco **Epic** (checkpoint stimati, priorità).  
- Checklist **DoD** trasversale (build Game/Editor, test verdi, determinismo, ecc.).

## Diagrammi
Includi immagini o diagrammi utili (usa Mermaid per timeline e dipendenze).

## Output finale
- Stato corrente del progetto.  
- File creati o modificati.  
- Elenco Epic definiti e totale checkpoint.  
- Problemi riscontrati nella roadmap precedente.  
- Attività escluse o rimandate.  
- Issue GitHub create o motivazione del mancato.  
- Prossimo checkpoint consigliato.  
- Comandi Git per verifiche.  
- Commit suggeriti.