# Catalogo dei kit del 2026-11-20 — piano di esecuzione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** registrare le scelte dell'autore del 2026-11-20 in una voce del Decision Log, conservare i due file d'input come `RESEARCH`, poi aprire una issue per eroe che parte da quella voce e chiudere #4014 come superata.

**Architecture:** una PR di sola documentazione sul ramo `docs/catalogo-14-eroi-2026-11-20` (worktree `.claude/worktrees/catalogo-14-eroi`, base `origin/main` = `fb858a0fc`); dopo il merge, le scritture su GitHub fatte da uno script con i corpi in file. Nessun codice, asset o scenario.

**Tech Stack:** Markdown, Python 3 (`python -I`, script nello scratchpad), `node tools/radar/*.ts`, `gh` con `GH_TOKEN` di `DegrassiAaron`.

**Spec:** [`docs/superpowers/specs/2026-11-20-catalogo-14-eroi-design.md`](../specs/2026-11-20-catalogo-14-eroi-design.md)

## Global Constraints

- Il testo che l'autore ha visto (domande, opzioni, risposte libere) si cita **alla lettera**, dalla trascrizione della sessione; «(Recommended)» è l'etichetta dell'assistente e si riporta com'era.
- Ciò che l'assistente ha proposto e l'autore non ha contestato è **lettura dell'assistente, ribaltabile**, mai «deciso».
- Nessun totale che cambia da solo in titoli e corpi (issue, PR, commenti): nomi o il comando che conta.
- Ogni `gh` che scrive: `GH_TOKEN` di `DegrassiAaron`, corpi passati con `--body-file`, mai `--body` inline.
- Script che scrivono testo con « »: niente heredoc; dopo ogni scrittura `U+FFFD` = 0 e nessuna riga preesistente con `<<`/`>>` alterata.
- Numero della voce: **D-633**, misurato su `origin/main` = `fb858a0fc` (massimo D-632, nessuna PR aperta); vale al merge e si rimisura prima.

## Review Focus

1. Una citazione d'autore che arriva su disco con `U+FFFD` o con le virgolette cambiate: conteggio dei byte `EF BF BD` = 0 su ogni file toccato (Task 1, 2, 3).
2. Uno script di annotazione che colpisce zero o due righe, o rompe una riga di tabella: assert di **una** riga per ID e di **quattro** celle prima e dopo (Task 2, 3).
3. `D-633` rivendicata da un altro ramo prima del merge: `node tools/radar/decision-ids.ts --check` e rimisura (Task 4).
4. `gh issue create` che scrive col conto sbagliato e scarta etichette e milestone senza errore: verifica di `author`, `labels`, `milestone` su ogni issue (Task 5).
5. Un corpo di issue che linka un file non ancora su `main`: le issue nascono solo dopo il merge, e lo script verifica ogni percorso con `git cat-file -e origin/main:<path>` (Task 5).

---

### Task 1: File d'input in `docs/research/handoff/` e design

**Files:**
- Create: `docs/research/handoff/catalogo-14-eroi-input-2026-11-20.md` — banner `RESEARCH` in testa, poi il file dell'autore `docs/characters/refactor_tactics_catalogo_master_personaggi_e_skill_14_eroi.md` (clone principale, non tracciato) byte per byte
- Create: `docs/research/handoff/catalogo-14-eroi-input-2026-11-20.csv` — copia byte per byte di `docs/characters/Refactor_Tactics_14_Heroes_Skills.csv`
- Modify: `docs/superpowers/specs/2026-11-20-catalogo-14-eroi-design.md` §1 — link ai due file e alla voce D-633
- Create: questo piano

**Interfaces:**
- Produces: i due percorsi sopra, citati dalla voce (Task 2) e dalle issue (Task 5).

- [ ] **Step 1:** Banner sul modello di `uiux-giochi-tattici-input-2026-11-16.md`: `RESEARCH`, materiale non autorevole, richiesta dell'autore alla lettera, nomi originali dei due file, il fatto misurato che `.md` e `.csv` divergono solo su Riktor (9 voci su 18; vale il `.md`), che le regole in vigore sono quelle della voce D-633 e che il file dichiara di sé «Source of Truth» ma non governa.
- [ ] **Step 2:** Verifica: `sha256` del CSV copiato = originale; il `.md` dopo il banner = originale (assert in Python); `U+FFFD` = 0 su tutti e tre.
- [ ] **Step 3:** Commit `docs(catalogo-14-eroi): input del 2026-11-20 in research/handoff, design e piano`, con percorsi espliciti in `git add`.

### Task 2: Voce D-633 e annotazioni nel Decision Log

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` — riga `D-633` subito dopo la riga `D-632`; nota in `## Note` dopo quella di D-632; annotazione ➕ in fondo alla cella **Stato** di D-012, D-014, D-586, D-588, D-593, D-596, D-612, D-620

**Interfaces:**
- Consumes: i percorsi del Task 1.
- Produces: l'ancora `D-633`, citata dal Task 3 e dalle issue.

- [ ] **Step 1: Frase guida di D-633** (cella Decisione, in maiuscolo come le vicine): il catalogo dei kit del 2026-11-20 è `RESEARCH` e la base per scegliere e bilanciare i kit degli eroi che nomina; tutte le opzioni di ogni slot entrano nel kit, come per Riktor; gli elementi sono nove, per ora; ogni eroe ha una skill di overclock; l'Overwatch è una reazione del kit; la rianimazione si riapre; restano le cinque fasi, il bonus alle spalle, la copertura fissa, nessuna progressione, il Tier 3 fuori; per Riktor vale la versione in Markdown.
- [ ] **Step 2: Corpo di D-633**, nell'ordine di D-620:
  - la richiesta alla lettera e lo scopo alla lettera («servono come base questi kit per scegliere e bilanciare le skill. crea una issue per eroe»);
  - **(1)** la risposta libera ai conflitti, alla lettera, con l'elenco dei conflitti che l'assistente aveva mostrato;
  - **(2)–(5)** primo giro (Spalle, Progressione, Rianimazione, Riktor), **(6)–(9)** secondo giro (Overwatch, Copertura, Tier 3, Kit), **(10)** terzo giro (procedura, #4014): ogni domanda e il testo dell'opzione scelta alla lettera, le scartate per etichetta;
  - una riga 🔴 che dice quali scelte erano marcate «(Recommended)» (Spalle, Progressione, Copertura, Tier 3, procedura, #4014) e quali no (Rianimazione, Riktor, Overwatch, Kit);
  - **(11) Letture dell'assistente, non decisioni**: il roster dei nomi del catalogo e il rientro di Phase e Aurora contro D-586 (1); gli identificatori di D-593 per i nove (Flux = `Element.Warp`, escono `Decay`, `Life`, `Arcane`); spicchi → celle e passi da 60°; Guard/Counter/Evasion → `Intercept`/`Deflect`/`Counter`/`Evade`; le combo → candidate per il catalogo delle coppie; i numeri del catalogo sostituiscono i vigenti solo con la `D-nnn` d'accettazione; `LinearDischarge` quarta opzione dell'Abilità Speciale; la regola della rianimazione presa dal catalogo (Ultimate di Prep di The Fey, 25% HP, bozzolo, Phase Lock), con HP, stato e vittoria da decidere nella issue di The Fey; i kit in codice di Gadget, Muiren, Ivrin e Riktor escono con le rispettive accettazioni;
  - **Che cosa non cambia**: D-583 (1) e (4), D-587/D-599, D-589/D-602, D-598, D-613, D-620 (6) e (7);
  - **Stato**: accettata per (1)–(10) nel testo visto; (11) dell'assistente; ⛔ nessun codice, asset o scenario cambia;
  - **Impatto**: owner (questa voce, `OPEN_DECISIONS.md`, le issue per eroe) e, misurati con `git grep` al momento della scrittura, i documenti **non** aggiornati che dicono il contrario (almeno `RT_ElementPairCatalog_v0.1.md`, `RT_HeroCatalog_v0.1.md`, `docs/characters/index.md`, `paragon.md`, `riktor-kit-traduzione-2026-11-13.md`, `brief-azioni-generiche-overwatch.md`, `spec-economia-del-turno.md`).
- [ ] **Step 3: Annotazioni**, una frase ciascuna, forma `➕ **2026-11-20, [D-633](RT_PDR_00_Decision_Log.md)**: …`: D-012 e D-014 (l'Overwatch smette di essere universale: è una reazione del kit); D-586 (Phase e Aurora tornano come eroi propri, lettura; i kit nascono dal catalogo del 2026-11-20); D-588 e D-593 (nove elementi per ora; escono Morte, Vita, Arcano); D-596 (riaperta); D-612 (per Riktor vale la versione in Markdown; (2) vale per ogni eroe; (5) ogni eroe ha un overclock); D-620 (la (2) «restano i dodici» è superata; `UXR-1` e `UXR-5` decise).
- [ ] **Step 4: Nota del numero** in `## Note`, sul modello di quella di D-632, con i comandi e il loro esito.
- [ ] **Step 5:** Uno script in `scratchpad/` fa tutte le scritture: conta **prima** ogni ancora (una riga per ID, quattro celle con il separatore `(?<!\\)\|`), scrive solo se tutte tornano, riconta dopo; legge e scrive in binario conservando i fine riga del file.
- [ ] **Step 6:** Verifica: `node tools/radar/decision-ids.ts --check`, `node tools/radar/doc-links.ts --check`, `node tools/radar/doc-tables.ts --check` → exit 0; `U+FFFD` = 0; `git diff -U0 | grep -cE '^-.*(<<|>>)'` → 0.
- [ ] **Step 7:** Commit `docs(decisions): D-633, il catalogo dei kit del 2026-11-20 e le scelte dell'autore`.

### Task 3: `OPEN_DECISIONS.md`

**Files:**
- Modify: `docs/OPEN_DECISIONS.md` — intestazione «Ultimo aggiornamento» al 2026-11-20; righe `UXR-1`, `UXR-5`, `KIT-1`, `KIT-2`, `KIT-3`, `MSP-5.d`, `GDD-4`

**Interfaces:**
- Consumes: l'ancora D-633.

- [ ] **Step 1:** Righe: `UXR-1` ✅ decisa, resta piatta 60; `UXR-5` ✅ per l'Overwatch (reazione del kit), la Guard come lettura dell'assistente; `KIT-1` e `KIT-2` valgono per ogni eroe, sempre aperte; `KIT-3` «se ogni eroe ne ha una»: sì, la skill di ciascuno si progetta nella sua issue; `MSP-5.d` superata dal catalogo del 2026-11-20, #4014 chiusa; `GDD-4` riaperta. Ogni modifica in coda alla cella di stato, con ➕ e D-633; il testo esistente non si riscrive.
- [ ] **Step 2:** Stesso script del Task 2 (asserti su ancore e celle), poi `doc-links`, `doc-tables`, `U+FFFD` = 0.
- [ ] **Step 3:** Commit `docs(open-decisions): UXR-1, UXR-5, KIT-*, MSP-5.d e GDD-4 dopo D-633`.

### Task 4: PR, revisione, merge

- [ ] **Step 1:** Gate sull'albero del ramo: `doc-links`, `doc-tables`, `doc-coherence`, `issue-refs`, `decision-ids` (tutti `--check`) e `node tools/radar/generate.ts --check` → exit 0. `RefactorTactics.Meta` `NOT RUN`: nessun file letto dai test C++ cambia (verificato con `git grep` sui nomi dei file toccati in `Source/`).
- [ ] **Step 2:** Push con l'helper `gh` e `GH_TOKEN`; PR verso `main` (genitore del ramo), corpo da file, con: cosa registra, letture dell'assistente, file non aggiornati, gate e `NOT RUN`, e la riga finale di attribuzione.
- [ ] **Step 3:** Revisione avversaria di un agente fresco sul diff della PR, in sola lettura: fedeltà delle citazioni alla trascrizione, nulla «deciso» che l'autore non ha scelto, ancore e link. Correzioni in commit separati, rimisura dei gate.
- [ ] **Step 4:** Rimisura di `D-633` su `origin/main` e sulle PR aperte; merge **solo con l'«unisci» dell'autore**; `gh pr view --json state,mergeCommit` = `MERGED`.

### Task 5: Issue per eroe e chiusura di #4014

**Files:**
- Create: `scratchpad/issue_eroi.py` e i corpi in `scratchpad/issue-body/<eroe>.md`

**Interfaces:**
- Consumes: D-633 su `main`; i percorsi del Task 1.

- [ ] **Step 1: Titolo** `Kit di <Eroe> dal catalogo del 2026-11-20: traduzione, scelta e bilanciamento`. **Corpo**: il modello approvato dall'autore (Decisioni a monte D-633 e D-612; Input = sezione dell'eroe nel `.md` e righe `<PFX>_*` del `.csv`; Lavoro 1–4; DoD; Fuori scope), con link `blob/main/...`. Prefissi: GAD, RIK, STL, MRL, KAL, MUR, FNG, FEY, PHS, AUR, SEV, YIN, CRN, DRN.
- [ ] **Step 2: Specifiche per eroe**: Gadget, `LinearDischarge` quarta opzione dell'Abilità Speciale con i numeri e la catena di D-621; Riktor, vale il `.md`, la traduzione del 13/11 si riusa per le voci comuni e si rifà per le nove cambiate, l'equipaggiamento di default si ridisegna (D-612 (6)); Muriel e Kallari, la loro fetta di #4011 aspetta questa issue; The Fey, la rianimazione con HP, stato e vittoria da decidere qui. Scheda: `v0.1/` per Gadget, Riktor, Muriel, Kallari; `v0.2/` per Steel, Murdock, Feng Mao, The Fey, Aurora; `candidates/` per Phase, Sevarog, Yin, Crunch, Drongo.
- [ ] **Step 3: Etichette e milestone**: Gadget, Riktor, Muriel, Kallari, Steel, Murdock → `enhancement`, `v0.1`, `P1`, milestone `v0.1 — Offline Vertical Slice`; Feng Mao, The Fey → `enhancement`, `P2`, `post-v0.1`, milestone `v0.2 · Struttura e finestre`; gli altri → `enhancement`, `P2`, `post-v0.1`, nessuna milestone.
- [ ] **Step 4:** Prova a secco: lo script stampa titoli e percorsi, verifica ogni link con `git cat-file -e origin/main:<path>` e `U+FFFD` = 0; poi crea con `--body-file`.
- [ ] **Step 5:** Verifica per ogni issue: `gh issue view <n> --json author,labels,milestone` → autore `DegrassiAaron`, etichette e milestone come allo Step 3.
- [ ] **Step 6:** Commento su #322 con le issue degli eroi «Core»; commento su #4011 (Muriel e Kallari ora aspettano le loro issue, non più #4014); commento su #4014 che rimanda alle quattro issue e a D-633, poi chiusura `--reason "not planned"`.

### Task 6: Chiusura

- [ ] **Step 1:** Ramo remoto e locale e worktree rimossi dopo `git merge-base --is-ancestor <tip> origin/main`.
- [ ] **Step 2:** Chiedere all'autore se cancellare i due originali non tracciati in `docs/characters/` del clone principale (sono suoi).
- [ ] **Step 3:** Memoria del progetto aggiornata con D-633 e i numeri delle issue.
