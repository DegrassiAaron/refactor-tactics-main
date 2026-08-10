> ## 🗄️ `HISTORICAL` — SUPERATO DALLA PROPRIA SECONDA VERSIONE
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked, accanto a una copia piu'
> recente di se stesso.
>
> ⚠️ **Questa e' la versione da ignorare.** La seconda
> ([`2026-08-08-chat-cleanup-tracker.md`](2026-08-08-chat-cleanup-tracker.md)) aggiunge i cluster Map e UI fra
> i completati e cinque conflitti che qui non compaiono. Resta solo per far vedere che le due esistevano
> entrambe, indistinguibili per nome.

# RefactorTactics — Chat Cleanup Tracker

**Aggiornato:** 2026-08-09

## Cluster completati

| Cluster | Master | Stato | Chat candidate dopo integrazione |
|---|---|---|---|
| Reaction System | `RT_Reaction_System_Master_Consolidation_v0.1.md` | MASTER CREATO | Overwatch focus, Brace focus, Time Bank, Reaction overview specialistici |
| Common Actions | `RT_Common_Actions_Master_Consolidation_v0.1.md` | MASTER CREATO | Move, Wait, Basic Attack, Skill comuni, Azioni base e varianti |
| Characters & Roster | `RT_Characters_Roster_Master_Consolidation_v0.1.md` | MASTER CREATO | Focus personaggi v0.1, Nomi Paragon, Meccaniche uniche, Fazioni, Super/cooldown |

## Cluster successivi

| Priorità | Cluster | Chat / fonti principali | Obiettivo |
|---:|---|---|---|
| 1 | Map & Environment | Interazioni mappa, rumore, terrain/cover/doors/bridges | Master di affordance e sistemi ambientali |
| 2 | UI / UX | HUD, Action Ghosts, planning UI, reaction UI | HUD registry e conditional visibility |
| 3 | Scenarios / QA / Bots | Developer Toolkit, BP GameMode scenario categories, bot/AI | Scenario Registry + test taxonomy |
| 4 | Governance | Feature Registry, Roadmap, Wiki links, docs audit | Un'unica source of truth e stato |
| 5 | Archive | vecchi PDR, workbook research, handoff Claude superati | CURRENT / AS-BUILT / HISTORICAL / RESEARCH |

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
