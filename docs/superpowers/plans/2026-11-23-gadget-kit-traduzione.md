# Kit di Gadget dal catalogo del 2026-11-20 — piano di esecuzione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** tradurre le voci `GAD_*` del catalogo del 2026-11-20 nel contratto, con la fonte di ogni valore, aggiungere il kit proposto alla scheda di Gadget come `DESIGN_SPEC`, portare le domande all'autore e chiudere con la `D-nnn` d'accettazione ([#4170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4170)).

**Architecture:** una PR di sola documentazione sul ramo `docs/4170-kit-gadget` (worktree `.claude/worktrees/4170-kit-gadget`, base `origin/main` = `f012ed660`). Il documento nuovo è `docs/roadmap/plans/gadget-kit-traduzione-2026-11-23.md`, sul modello di [`riktor-kit-traduzione-2026-11-13.md`](../../roadmap/plans/riktor-kit-traduzione-2026-11-13.md). Nessun codice, asset, scenario o catalogo di `docs/balance/`.

**Tech Stack:** Markdown, Python 3 (script nello scratchpad, scritti con lo strumento di scrittura file e mai con un heredoc), `node tools/radar/*.ts`, `git grep`, `gh` con `GH_TOKEN` di `DegrassiAaron`.

**Spec:** [`docs/superpowers/specs/2026-11-23-gadget-kit-traduzione-design.md`](../specs/2026-11-23-gadget-kit-traduzione-design.md)

## Global Constraints

- Ogni valore porta la sua fonte fra parentesi quadre: «Un numero senza fonte non c'è.» Etichette: `[CATALOGO]` (testo del `.md`, §3 «Gadget» del [file d'input](../../research/handoff/catalogo-14-eroi-input-2026-11-20.md)), `[CSV <Skill_ID>]` (campo che c'è solo nel [`.csv`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.csv)), `[DECISO D-nnn (k)]`, `[LETTURA D-nnn (k)]`, `[SESSIONE 2026-11-23]` (scelte dell'autore registrate nel §2 del design, fino alla `D-nnn`), `[PROPOSTA]` (col motivo), `[DOMANDA → TG-n]`, `[codice]` (`file:riga` letto su `f012ed660`, **non eseguito**).
- Le letture di D-633 (11) restano letture, mai «deciso».
- **Decisioni di D-633**: cinque fasi; nove elementi; overclock come tipo di skill che hanno tutti; spalle di oggi; copertura bassa fissa 60; nessuna progressione; Overwatch come reazione del kit; Tier 3 fuori; tutte le opzioni di ogni slot nel kit; nomi Paragon.
- **Nel kit**: le diciotto voci `GAD_*` del catalogo, `Hero.Gadget.LinearDischarge` e una skill di overclock. **Escono** `ArcPulse`, `ConductiveNode`, `Overload` e `ReactiveCapacitor` `[SESSIONE 2026-11-23]`.
- `LinearDischarge` con i numeri e la catena di oggi `[LETTURA D-633 (11g)]`.
- Ogni voce si traduce anche se il suo sistema manca; il sistema mancante si nomina con il `git grep` che lo cerca e il suo esito su `f012ed660`, e non si inventa.
- Le domande per Gadget si chiamano `TG-n`. Una domanda già aperta per Riktor non si duplica: la `TG-n` cita la `TR-n` di Riktor e dice che una risposta vale per entrambi.
- Nessun totale che cambia da solo; `U+FFFD` = 0 su ogni file toccato.
- Gate: `node tools/radar/{doc-links,doc-tables,doc-coherence,issue-refs,decision-ids,scenario-notes}.ts --check` e `node tools/radar/generate.ts --check` → exit 0.

## Review Focus

1. **Una proposta o una lettura presentata come decisa**, per esempio «Flux è `Element.Warp`» senza `[LETTURA D-633 (11b)]`. Il controllore del Task 1 rifiuta ogni cella di valore senza etichetta (Task 1, 2, 3, 4).
2. **Una regola copiata dalla pagina di Riktor che una decisione successiva ha cambiato.** A parità di priorità decide l'id stabile, non l'ordine di dichiarazione (D-615 (5)). TR-29 è chiusa da D-619. L'Overwatch non è più universale (D-633 (6)). Gli elementi sono nove (D-633 (1)). Ogni riga di «Come leggere la traduzione» cita una decisione la cui cella Stato non la dichiara superata (Task 1).
3. **Un sistema dato per mancante che esiste, o viceversa.** Ogni riga di «Sistemi che mancano» riporta il comando e il suo esito (Task 2, 3, 4).
4. **`LinearDischarge` con numeri diversi dal codice.** In codice: 240 danni, portata 5, cooldown 2, priorità 55, forma `Line`, ripiego `AttackCell` (`RTHeroCatalogLibrary.cpp:428-430`); +80 contro `Wet` (`RTCombatLibrary.h:521`); varianti a 300 e a 180 + 180 (`RTHeroCatalogLibrary.cpp:503-521`). Si confronta riga per riga (Task 3).
5. **Aree contate male.** Un'area esagonale di raggio r ha `3r(r+1)+1` celle: 7 per r = 1, 37 per r = 3. Ogni «3x3», «raggio N» o «spicchio» del catalogo porta nella scheda la forma scelta e il numero di celle (Task 2, 3).

---

### Task 1: Testa del documento, regole di lettura, statistiche, ciò che esce

**Files:**
- Create: `docs/roadmap/plans/gadget-kit-traduzione-2026-11-23.md`
- Create: `<scratchpad>/controlla_traduzione.py` (fuori dal repository)

**Interfaces:**
- Produces: le sezioni `## Come leggere le provenienze`, `## Come leggere la traduzione`, `## Statistiche base`, `## Che cosa esce`; il formato di scheda `### <Nome del catalogo> — \`Hero.Gadget.<Id>\``; il controllore, che i Task 2–4 rilanciano.

- [ ] **Step 1: Testa**, come quella di Riktor. Stato `DESIGN_SPEC` · **PROPOSTA** per la `D-nnn` d'accettazione della #4170. «Non è in vigore»: il kit in vigore è quello di [`gadget.md`](../../characters/v0.1/gadget.md). Base `f012ed660`, input, decisioni rispettate.
- [ ] **Step 2: Le due tabelle di lettura.** Le etichette sono quelle dei Global Constraints. «Come leggere la traduzione» si **rideriva** dalle righe attuali del Decision Log, non si copia da Riktor: fasi e priorità del resolver, AP e Ultimate, cooldown e ripetizione, ordine dentro la fase (D-615 (5)), tipo di danno, elementi (i nove di D-633 con gli identificatori della lettura (11b)), combo, overclock (definizione d'autore di D-633 (1) alla lettera), formula del danno (D-589, D-602, copertura bassa 60 di D-633 (7)), spicchi → celle (lettura (11c)), reazioni Guard/Counter/Evasion (lettura (11d)) e Overwatch (D-633 (6)). Per ogni riga leggi la cella Stato della decisione citata; le righe del Decision Log sono una riga fisica ciascuna, e le celle si separano con `(?<!\\)\|`.
- [ ] **Step 3: Statistiche base.** Una tabella con il valore in vigore (`docs/balance/RT_HeroCatalog_v0.1.md`, riga di Gadget), il valore del catalogo e la proposta, per HP, DEF, RES, movimento, gittata, vista e resistenza alla spinta. Ci vanno le domande aperte: la «RES Energetica» unica contro la RES per elemento (#4170 punto 4), il movimento 3 contro i 5 MP di oggi, e la vista che il catalogo non dà.
- [ ] **Step 4: Che cosa esce**, per nome. `ArcPulse`, `ConductiveNode`, `Overload`, `ReactiveCapacitor` `[SESSIONE 2026-11-23]`, con quello che ciascuno lascia:
  - `Action.Electrify` resta senza un eroe che la usi;
  - gli scenari che le citano, contati con `git grep -l '<id>' -- Scenarios | wc -l` (si scrive il comando, non il numero);
  - il profilo `Profile.Grounding` e l'affinità o debolezza dell'eroe, che diventano domande.
- [ ] **Step 5: Il controllore.** Uno script Python che legge il documento e verifica:
  - (a) ogni `Skill_ID` `GAD_*` del `.csv` ha una scheda con il nome del catalogo;
  - (b) in ogni tabella `| Campo | Valore |` ogni cella di valore contiene almeno un'etichetta della legenda;
  - (c) `U+FFFD` = 0;
  - (d) nessun `TG-n` citato manca dall'elenco delle domande, quando l'elenco esiste.

  Stampa ciò che non torna ed esce con 1. Dopo il Task 1 fallisce solo su (a).
- [ ] **Step 6:** `doc-links` e `doc-tables` `--check` → exit 0; commit `docs(4170): traduzione del kit di Gadget, testa, regole di lettura, statistiche e voci che escono`.

### Task 2: Schede dei gruppi 1–3 (attacchi base, Prep, Dash)

**Files:**
- Modify: `docs/roadmap/plans/gadget-kit-traduzione-2026-11-23.md` — sezioni `## 1. Attacchi base`, `## 2. Prep`, `## 3. Dash`

**Interfaces:**
- Consumes: le regole e il formato del Task 1.
- Produces: le schede `SparkBolt`, `PhaseDart`, `StaticArc`, `StructureShift`, `SpeedGate`, `HoloBarricade`, `SeekerBot`, `FluxBlink`, `CryoDrone`; i `TG-n` che aprono, numerati da `TG-1`.

- [ ] **Step 1:** Per ogni voce, una scheda con i campi della pagina di Riktor: Ruolo, Fase, Costo AP, Cooldown, Priorità, Portata e linea di vista, Forma (con il numero di celle), Bersaglio, Elemento, Tipo di danno, Danno base, Movimento, Stati, Oggetti persistenti. Si aggiungono il campo **Sistemi che mancano**, con comando ed esito, e un **Esempio** numerico con la formula di D-589, secondo la convenzione degli esempi di Riktor. Fonte di ogni valore.
- [ ] **Step 2:** Le note «Sintonizzazione» del catalogo si scartano per nome: nessuna progressione `[DECISO D-633 (3)]`.
- [ ] **Step 3:** Il controllore passa su (b), (c) e (d) per queste sezioni; `doc-links` e `doc-tables` → exit 0; commit `docs(4170): schede di attacchi base, Prep e Dash`.

### Task 3: Schede dei gruppi 4–6 (Abilità Speciale con `LinearDischarge`, Ultimate, reazioni)

**Files:**
- Modify: stesso documento — sezioni `## 4. Abilità Speciale`, `## 5. Ultimate`, `## 6. Reazioni`

**Interfaces:**
- Consumes: le regole del Task 1, e il primo `TG-n` libero dopo il Task 2.
- Produces: le schede `EmpDisruption`, `GravityCollapse`, `ThermitePulse`, `LinearDischarge`, `StickyBomb`, `TeslaDome`, `RealityRefactor`, `FluxDeflector`, `FeedbackLoop`, `TeslaTrap`.

- [ ] **Step 1:** Schede come nel Task 2. `LinearDischarge` copia i valori di oggi dal codice `[codice]`, quarta opzione dell'Abilità Speciale `[LETTURA D-633 (11g)]`, con la catena di D-621. Le varianti `Concentrated`/`Branched` e la condizione `Wet` diventano domande. Le Ultimate costano 3 `[DECISO D-613 (4)]`. `TeslaTrap` è l'Overwatch di Gadget `[DECISO D-633 (6)]`. Le reazioni seguono la lista di D-592 e una attivazione per reazione di D-619.
- [ ] **Step 2:** Il confronto riga per riga di `LinearDischarge` con `RTHeroCatalogLibrary.cpp:428-430`, `RTCombatLibrary.h:521` e `RTHeroCatalogLibrary.cpp:503-521` va scritto nella scheda.
- [ ] **Step 3:** Controllore, gate e commit `docs(4170): schede dell'Abilità Speciale, delle Ultimate e delle reazioni`.

### Task 4: Overclock, misure, combo, kit in breve, domande

**Files:**
- Modify: stesso documento — sezioni `## 7. Overclock`, `## Misure`, `## Combo candidate`, `## Sistemi che mancano`, `## Il kit in breve` (subito dopo «Che cosa esce»), `## Domande per l'autore`, `## Provenienza e limiti`

**Interfaces:**
- Consumes: tutte le schede.
- Produces: l'ancora `#domande-per-lautore` e l'elenco `TG-n` completo, che il controllore (d) verifica; l'id `Hero.Gadget.<Id>` dell'overclock.

- [ ] **Step 1: Overclock.** Una `[PROPOSTA]` sulla definizione d'autore di D-633 (1): un vantaggio per un round e un costo nel round dopo, con budget noto al planning `[DECISO D-613 (1)]`. L'Overclock generico del catalogo («+2 AP», *Phase Lock*) è l'alternativa della domanda, che cita TR-33 di Riktor.
- [ ] **Step 2: Misure** `[PROPOSTA]`, una tabella per voce di danno. Danno per AP; colpi per abbattere ciascuno degli altri eroi Core del catalogo, con HP, DEF e RES del §2 del catalogo, di fronte, senza copertura né Guard. Se il danno energetico paghi la RES è la domanda di TR-1: la tabella dice quale ipotesi usa.
- [ ] **Step 3: Combo candidate.** Le cinque righe della matrice di Gadget, come candidate per il [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md), mai regole dell'eroe `[LETTURA D-633 (11e)]`; le combo a tre elementi senza regola `[DECISO D-591 (1)]`.
- [ ] **Step 4: Sistemi che mancano**, consolidati. Una riga per sistema: voci che lo chiedono, comando ed esito, dominio owner (dalla tabella dei domini di `CLAUDE.md` §12).
- [ ] **Step 5: Il kit in breve**, ricavato dalle schede: una riga per voce con id, gruppo, fase, AP, cooldown, priorità, portata e forma, elemento, danno base.
- [ ] **Step 6: Domande per l'autore**, in ordine di peso. Ognuna dice le voci che tocca. Devono comparire le quattro regole del catalogo non misurate (#4170 punto 4), la dock (`KIT-1`, `KIT-2`) e l'equipaggiamento di default. Ogni `TR-n` di Riktor citata va controllata contro il Decision Log: se una decisione l'ha chiusa, va detto (TR-29 → D-619).
- [ ] **Step 7:** Il controllore passa per intero; tutti i gate dei Global Constraints → exit 0; commit `docs(4170): overclock, misure, combo, sistemi che mancano e domande per l'autore`.

### Task 5: Sezione `DESIGN_SPEC` nella scheda di Gadget

**Files:**
- Modify: `docs/characters/v0.1/gadget.md` — un avviso sotto la testata e una sezione `## Kit proposto dal catalogo del 2026-11-20 (DESIGN_SPEC)` prima di `## Governance`

- [ ] **Step 1:** L'avviso dice che il kit descritto nella pagina resta in vigore fino alla `D-nnn` d'accettazione, e rimanda alla sezione e alla traduzione.
- [ ] **Step 2:** La sezione riporta il kit in breve (le righe del Task 4, Step 5), ciò che esce e il link alle domande. Nessun numero della pagina in vigore cambia.
- [ ] **Step 3:** `git diff` della pagina = solo righe aggiunte; gate; commit `docs(4170): kit proposto nella scheda di Gadget`.

### Task 6: Revisione del ramo e domande all'autore (lo fa chi coordina)

- [ ] **Step 1:** Revisione finale avversaria dell'intero ramo, prima delle domande. Una domanda costruita su una traduzione sbagliata la vede l'autore, e la decisione resta il testo che ha visto.
- [ ] **Step 2:** Le `TG-n` portate all'autore a giri di domande a scelta, le più pesanti prima. Il testo visto si conserva; una domanda rinviata resta aperta.

### Task 7: `D-nnn` d'accettazione e aggiornamento dei documenti

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` (riga nuova dopo l'ultima, nota del numero in `## Note`), `docs/OPEN_DECISIONS.md` (`KIT-3` per Gadget, le voci che le risposte chiudono), il documento di traduzione e `gadget.md` (le risposte al posto delle domande chiuse)

- [ ] **Step 1:** La voce riporta alla lettera, controllati sulla trascrizione della sessione: la richiesta, le tre scelte del §2 del design e ogni giro del Task 6. Le letture dell'assistente stanno in un punto a parte. Lo Stato dice per quali punti è accettata e quali restano rinviati. ⛔ Nessun codice cambia.
- [ ] **Step 2:** Uno script fa le scritture nel Decision Log e in `OPEN_DECISIONS.md`. Prima conta le ancore (una riga per id, celle con il separatore `(?<!\\)\|`) e scrive solo se tornano tutte; conserva i fine riga.
- [ ] **Step 3:** Gate, `U+FFFD` = 0, `git diff -U0 | grep -cE '^-.*(<<|>>)'` → 0; commit `docs(decisions): D-nnn, accettazione del kit di Gadget`.

### Task 8: PR, merge, chiusura

- [ ] **Step 1:** Il numero `D-nnn` si rimisura su `origin/main` e sulle PR aperte; gate sull'albero del ramo.
- [ ] **Step 2:** La PR va verso `main`, con il corpo da file: cosa contiene, cosa è deciso e cosa è letto, i documenti non aggiornati, i gate, `Compile/Automation/PIE/Packaged: N/A`. Merge **solo con l'«unisci» dell'autore**.
- [ ] **Step 3:** DoD della #4170 aggiornata, issue chiusa; ramo e worktree rimossi dopo `git merge-base --is-ancestor`; memoria aggiornata.
