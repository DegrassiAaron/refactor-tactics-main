> ## 🗄️ `HISTORICAL` — SORGENTE NON APPLICABILE AL REPOSITORY
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked. Come i due tracker,
> **riguarda il progetto ChatGPT** — quali conversazioni chiudere e in che ordine — non il codice ne' la
> documentazione. Nessun owner documentale lo recepisce, e non ne serve uno.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).

# RefactorTactics — Final Chat Cleanup Plan v0.1

**Data:** 2026-08-09

Questa lista è la checklist operativa per pulire il progetto ChatGPT dopo che i master sono stati integrati nella documentazione canonica.

---

# 1. KEEP — CORE

Tenere nel progetto principale:

```text
00 — RefactorTactics Control Center
Feature Registry Roadmap
Current Sprint / v0.1
Open Design Decisions
Documentation Consolidation
Release v0.1
```

Se una di queste chat non esiste, crearla solo quando serve davvero.

---

# 2. MASTER / TEMP KEEP

Tenere finché il repository non assorbe tutto:

```text
Reaction System Master
Common Actions Master
Characters & Roster Master
Map & Environment Master
UI / UX Master
Scenarios / QA / Bots Master
Governance Master
```

Dopo merge nel canone possono diventare ARCHIVE.

---

# 3. ARCHIVE / DELETE CANDIDATES — Reaction

Dopo merge Reaction Master:

```text
Focus su Overwatch
Focus su Brace Skill
Time Bank in giochi
Reaction System Overview
```

Preferenza:
- Archive se contiene storia di design utile;
- Delete se il master + ADR hanno assorbito tutto.

---

# 4. ARCHIVE / DELETE CANDIDATES — Common Actions

```text
Skill Move in RefactorTactics
Focus attacco base
Focus abilità Wait
Skill comuni per personaggi
Azioni base e varianti
```

Non eliminare prima di chiudere `ACTION-TAXONOMY-01` e `FACING-01`.

---

# 5. ARCHIVE / DELETE CANDIDATES — Characters

```text
Focus personaggi v0.1
Nomi Paragon e RefactorTactics
Meccaniche personaggio uniche
Super colpi e cooldown
Fazioni
```

`Artwork Paragon 0.1` -> Archive/Art.

Vecchi roster non vanno mantenuti come chat operative.

---

# 6. ARCHIVE / DELETE CANDIDATES — Map

```text
Interazioni mappa giocatore
```

Documenti specialistici Walls/Doors/Noise possono restare Archive/Reference dopo integrazione.

---

# 7. ARCHIVE / DELETE CANDIDATES — UI

```text
Lista HUD da implementare
Action Ghosts e Pianificazione
```

Dopo merge nel UI Master + Feature/Scenario Registry.

---

# 8. ARCHIVE / DELETE CANDIDATES — QA / Tooling / AI

```text
Developer Toolkit
Modifica BP Game Mode
Bot e intelligenza artificiale
```

Dopo merge nel Scenarios/QA/Bots Master.

---

# 9. DELETE forte candidato — Meta già conclusa

```text
Aggiornamento file cloud e agent
```

Se gli attuali `CLAUDE.md`/`AGENTS.md` sono già corretti e versionati, la chat non serve come source.

---

# 10. ARCHIVE / Research

Tenere fuori dal CORE:

```text
Asset recovery v0.1
Artwork / image brainstorming
vecchi PRD/PDR non current
vecchi workbook balance
vecchie roadmap didattiche
vecchi roster
research esterna
```

---

# 11. Condizione per DELETE

Prima di cancellare una chat:

```text
[ ] decisione registrata
[ ] Feature Registry aggiornato
[ ] Scenario Registry aggiornato se applicabile
[ ] Roadmap aggiornata
[ ] Wiki aggiornata
[ ] test/scenario registrato se applicabile
[ ] conflitti chiusi o marcati OPEN
[ ] nessun dettaglio unico rimasto solo nella chat
```

---

# 12. Conflitti da NON perdere

```text
ACTION-TAXONOMY-01 — 6 vs 8 Universal Actions
FACING-01 — character pivot proposal vs ADR corrente
ROSTER-01 — roster storici vs current 8
TIMEBANK-01 — scope/valori open
HIGHGROUND-01 — no generic numeric bonus
NOISE-SCOPE-01 — design completo vs milestone reale
UI-READY-01 — team-ready mockup vs runtime reale
```

Le chat che contengono l'unica provenance di un conflitto restano Archive finché l'ADR non lo chiude.

---

# 13. Target numerico

Obiettivo ragionevole dopo cleanup:

```text
CORE: 5–10 chat operative
Domain projects: solo master + discussioni realmente attive
Archive: storico/research
Delete: duplicati assorbiti
```

Non cercare di arrivare a “zero chat”: il target è ridurre rumore e contraddizioni.

---

# 14. Sequenza pratica nell'app

1. Crea/usa `RT — Archive / Research`.
2. Mantieni i master nel progetto operativo finché il repository non è aggiornato.
3. Sposta prima gli storici evidenti.
4. Risolvi i conflitti ADR.
5. Esegui il merge di docs/registry/wiki/roadmap.
6. Sposta le vecchie chat di dominio in Archive.
7. Elimina solo quelle completamente ridondanti.
8. Ricontrolla il CORE: deve rimanere corto e orientato al lavoro corrente.
