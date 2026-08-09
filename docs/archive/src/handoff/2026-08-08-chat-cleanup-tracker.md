> ## 🗄️ `HISTORICAL` — SORGENTE REVISIONATO, NON APPLICATO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked. **Non riguarda il
> repository**: e' il tracker del progetto ChatGPT da cui il pacchetto e' nato. Resta per provenienza, perche'
> [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md) §4
> lo cita riga per riga.
>
> ⚠️ **Sette dei nove «conflitti aperti» che elenca erano gia' chiusi** quando e' stato scritto —
> `ACTION-TAXONOMY-01` (e con la baseline sbagliata), `ROSTER-01`, `BALANCE-WORKBOOK-01`, `HIGHGROUND-01`,
> `TIMEBANK-01`, `UI-REACTION-01`, `UI-GHOST-PRIVACY-01`. Realmente aperti restavano `FACING-01` e lo **scope**
> di rumore e mappa. Un handoff che ripropone come aperte decisioni chiuse invita a ridecidere cio' che e'
> deciso: e' la ragione per cui questo file e' qui e non e' autorita'.

# RefactorTactics — Chat Cleanup Tracker

**Aggiornato:** 2026-08-09

## Cluster completati

| Cluster | Master | Stato | Chat candidate dopo integrazione |
|---|---|---|---|
| Reaction System | `RT_Reaction_System_Master_Consolidation_v0.1.md` | MASTER CREATO | Overwatch focus, Brace focus, Time Bank, Reaction overview specialistici |
| Common Actions | `RT_Common_Actions_Master_Consolidation_v0.1.md` | MASTER CREATO | Move, Wait, Basic Attack, Skill comuni, Azioni base e varianti |
| Characters & Roster | `RT_Characters_Roster_Master_Consolidation_v0.1.md` | MASTER CREATO | Focus personaggi v0.1, Nomi Paragon, Meccaniche uniche, Fazioni, Super/cooldown |
| Map & Environment | `RT_Map_Environment_Master_Consolidation_v0.1.md` | MASTER CREATO | Interazioni mappa giocatore; muri/porte; rumore/percezione specialistici dopo merge |
| UI / UX | `RT_UI_UX_Master_Consolidation_v0.1.md` | MASTER CREATO | HUD, Action Ghosts, planning/reaction UI dopo merge |

## Cluster successivi

| Priorità | Cluster | Chat / fonti principali | Obiettivo |
|---:|---|---|---|
| 1 | Scenarios / QA / Bots | Developer Toolkit, BP GameMode scenario categories, bot/AI | Scenario Registry + test taxonomy |
| 2 | Governance | Feature Registry, Roadmap, Wiki links, docs audit | Un'unica source of truth e stato |
| 3 | Archive | vecchi PDR, workbook research, handoff Claude superati | CURRENT / AS-BUILT / HISTORICAL / RESEARCH |

## Regola cancellazione

Una chat passa a `DELETE` solo quando:
1. le decisioni sono entrate nel canone;
2. Feature/Scenario/Roadmap sono aggiornati;
3. conflitti risolti o registrati come OPEN;
4. esiste test/scenario dove necessario;
5. il master non dipende più dal testo originale.

## Stati

- `KEEP` — lavoro attivo.
- `MASTER` — fonte consolidata di lavoro.
- `ARCHIVE` — storico utile, escluso dal contesto operativo.
- `DELETE` — informazione completamente assorbita.
- `CONFLICT` — non eliminare finché la decisione non è chiusa.

## Conflitti aperti scoperti durante cleanup

### ACTION-TAXONOMY-01
Due audit del 2026-08-08 contengono tassonomie diverse:
- D-014: 6 azioni universali;
- D-AUDIT-01 successiva: 8 azioni universali, reintroducendo `Guard` e `Activate`.

Baseline di cleanup: 8 azioni, ma richiede verifica/emendamento nel Decision Log reale.

### FACING-01
Vecchie proposal permettono Pivot massimi diversi per personaggio.
Audit/ADR più recente descrive invece regole concrete di facing per Linear Move, Budget Move, stationary e forced movement.

Baseline di cleanup: ADR corrente; matrici character-specific restano proposal finché non approvate.

### BALANCE-WORKBOOK-01
Il workbook `RefactorTactics_Balance_Matrices_v0.1.xlsx` contiene roster, timing Reaction e action model storici.
Non trattarlo come source of truth finché non viene classificato RESEARCH o rigenerato/allineato.

### MAP-SCOPE-01
Infografiche e brief ambientali mostrano più meccaniche di quante siano necessariamente CURRENT.
Ogni superficie/combo va classificata `CURRENT / PARTIAL / DESIGNED / FUTURE / RESEARCH` prima di dichiararla canonica operativa.

### NOISE-SCOPE-01
Il design Noise/Perception è consolidato, ma documenti recenti indicano che non deve bloccare automaticamente la v0.1 base.
Va mantenuto come dominio progettato con milestone reale da verificare.

### HIGHGROUND-01
High Ground non ha un bonus numerico generale di visione approvato.
Non introdurre valori finché la decisione non viene chiusa.

### UI-READY-01
Mockup e brief storici possono mostrare `TEAM READY 1/2` anche quando il build locale non supporta davvero lo stato team-ready.
La UI deve mostrare lo stato reale del runtime.

### UI-REACTION-01
Reaction/Fast Reaction non è una quinta macrofase: la Ghost Timeline resta `PREP / DASH / BLAST / MOVE` e le reaction sono rami/Decision Boundary.

### UI-GHOST-PRIVACY-01
Action Ghost e warning possono usare solo intenti locali, team-only sanitizzati e conoscenza lecita.
Nessun ghost nemico deve essere derivato dal planning privato.
