# Claude Code Prompt — RefactorTactics TD Trial / Scenario Sandbox
## Aggiornamento issue, epic, documentazione e roadmap

**Data di riferimento:** 2026-08-29  
**Repository:** `DegrassiAaron/refactor-tactics-main`

---

## RUOLO

Agisci come **Technical Lead / Unreal Gameplay Tooling Engineer / maintainer della roadmap di RefactorTactics**.

Devi eseguire una **riconciliazione documentale e di tracking**, NON implementare gameplay o feature C++ in questa attività.

L'obiettivo è formalizzare nel repository una **TD Trial / Scenario Sandbox**: una versione utilizzabile del Tactical Designer che permetta a un technical/game designer di:

> creare o modificare uno scenario → impostare stato iniziale e azioni → validare → eseguire usando il runtime reale → vedere cosa è successo → ispezionare TurnLog/risultati → reset → modificare → rieseguire.

Devi aggiornare in modo coerente:
- issue GitHub;
- epic Tactical Designer;
- documentazione owner;
- roadmap/checkpoint;
- eventuali note di decisione SOLO se realmente necessarie.

**Non creare una roadmap parallela a quella esistente. Non creare un secondo simulatore. Non inventare stato senza misurarlo sul repository.**

---

# 0. REGOLE OPERATIVE OBBLIGATORIE

Prima di modificare qualunque file o issue:

1. Leggi `CLAUDE.md` e ogni istruzione repository-scoped applicabile.
2. Esegui:
   - `git status`
   - `git branch --show-current`
   - `git log -10 --oneline`
   - `gh auth status`
3. Non sovrascrivere modifiche locali dell'utente.
4. Se il working tree non è pulito, distingui chiaramente:
   - modifiche già presenti;
   - modifiche introdotte da questa attività.
5. Non modificare `.umap`, `.uasset` o altri asset binari.
6. Non implementare codice runtime/editor in questa attività.
7. Non chiudere issue perché "sembrano fatte": serve evidenza misurata.
8. Non aprire issue nuove prima di aver cercato duplicati/apparentemente equivalenti fra issue aperte e chiuse.
9. Non riesumare `feature-registry.yaml`, `feature_registry.py`, Control Center o shortlist rimosse da **D-181**.
10. Non usare un documento storico/archiviato come autorità corrente.
11. Se una nota datata è storicamente corretta ma oggi falsa, **non riscrivere la storia**: aggiungi una nota di riallineamento o aggiorna solo la prescrizione corrente.
12. Mantieni il principio:
    **dati canonici + runtime/resolver = autorità; editor = authoring, query e visualizzazione.**

---

# 1. FONTI DA LEGGERE PRIMA DI DECIDERE

Leggi almeno questi file correnti:

- `docs/technical/tooling/spec-tactical-designer.md`
- `docs/technical/tooling/scenario-map.md`
- `docs/technical/tooling/scenario-index-e-tag.md`
- `docs/technical/runbooks/test-e-diagnosi.md`
- `docs/technical/architecture/spec-turnlog-serialize.md`
- `docs/roadmap/roadmap-checkpoint.md`
- `docs/roadmap/roadmap-v0.1.md`
- `docs/roadmap/editor-sessions.yaml`
- `docs/technical/test-manuali-pie.md`
- `docs/product/piano-canonico-mvp.md`
- `docs/decisions/RT_PDR_00_Decision_Log.md`
- `docs/decisions/adr-0009-replay-logico-canonico.md`

Leggi anche l'epic e le issue live con `gh`:

- **#1105** — Tactical Designer
- **#1114** — Scenario writer
- **#1115** — Scenario initial state / unit placement
- **#1116** — Turn Move/Wait + expectations
- **#1117** — Run/Reset/Result/TurnLog
- **#622** — workspace grid
- **#623** — DevSandbox lights / frame map
- **#695** — door visualization
- **#711** — movement probe
- **#472** — replay viewer
- **#1515** — Scenario Harness validation gaps
- **#1540** — StateHash test coverage gaps

Poi cerca issue esistenti relative a:
- scenario visual playback nell'Editor;
- Scenario Composer;
- multi-turn authoring;
- attack/ability/dash intent authoring;
- reaction/decision authoring `FIRE/HOLD`;
- initial statuses/environment authoring;
- runtime probes LOS/cover/targeting;
- state diff;
- result inspector;
- scenario template/preset;
- seed authoring/consumption;
- ability override/variant authoring.

Usa ricerche ampie: nomi tecnici, sinonimi e vecchi nomi delle feature.

---

# 2. BASELINE NOTA — DA VERIFICARE, NON DA ASSUMERE

Alla data 2026-08-29 la baseline osservata è questa:

- #1105 Tactical Designer: **OPEN**
- #1114: **CLOSED**
- #1115: **CLOSED**
- #1116: **CLOSED**
- #1117: **CLOSED**
- #623: **CLOSED**
- #622: **OPEN**
- #695: **OPEN**
- #711: **OPEN**
- #472: **OPEN**

L'epic #1105 risultava con **5 sub-issue completate su 9**.

Verifica tutto nuovamente con GitHub prima di modificare.

### Nota importante su #472

#472 è attualmente un **Replay Viewer player-facing**, con navigazione frontend e UI per guardare una partita registrata.

NON trasformarla automaticamente nel playback della TD Trial.

Devi prima stabilire se:

A. il core già prodotto per replay/seek/playback può essere **riusato** dal Tactical Designer e serve una nuova issue Editor-only sotto #1105;

oppure

B. esiste già un'issue Editor-specific equivalente;

oppure

C. una porzione dello scope di #472 è realmente condivisibile senza alterare la sua Definition of Done.

Preferenza: **riuso del core, ownership separata della UI Editor**, se i due use case hanno DoD differenti.

---

# 3. PROBLEMA DA FORMALIZZARE

Il Scenario Composer non è più soltanto "designed".

Le slice #1114–#1117 hanno costruito il loop:

```text
MAP
→ INITIAL STATE
→ TURN / INTENT
→ EXPECTATION
→ VALIDATE
→ RUN
→ RESULT / TURNLOG
→ RESET
→ MODIFY
→ RUN
```

La TD Trial deve trasformare questo loop tecnico in un **Scenario Sandbox visuale utilizzabile da un designer**.

NON deve diventare:
- un nuovo gioco;
- un nuovo resolver;
- un nuovo formato scenario;
- una nuova simulazione semplificata;
- un balance lab;
- uno Skill Workbench completo;
- un editor runtime player-facing.

---

# 4. INVARIANTE ARCHITETTURALE

Mantieni e rendi esplicito ovunque serva:

```text
Dati canonici + regole runtime
        │
        ├── Resolver                  unica autorità sull'esito
        ├── Scenario Harness          esegue il percorso reale
        ├── TurnLog / Replay          traccia e spiega
        └── Runtime Query / DTO
                    │
                    ▼
              Editor / TD Trial
```

È vietato:

```text
Editor
  └── secondo pathfinder
  └── secondo targeting
  └── secondo resolver
  └── danno applicato a mano
  └── SetActorLocation come esito simulato
  └── branch "if Editor" nelle regole per ottenere un altro risultato
```

Lo stesso scenario deve poter essere eseguito headless e nell'Editor con lo **stesso risultato logico**.

---

# 5. TARGET DELLA TD TRIAL

Non introdurre una numerazione che confligga con:
- release `v0.x`;
- milestone `M<n>`;
- scala di maturità Tactical Designer `TD 0.1 ... TD 0.9`.

Puoi usare nel piano di riconciliazione i nomi **Trial Slice T0–T8** come etichette locali NON normative.

## T0 — Baseline / smoke gate

Scopo:
- verificare il loop già consegnato da #1114–#1117;
- creare/aprire scenario;
- unit placement;
- Move/Wait;
- expectation;
- Save;
- Run;
- TurnLog;
- Reset;
- Run di nuovo.

Gate:

```text
Editor Run == Headless Run
same semantic result
same StateHash/LogHash quando previsti
RUN → RESET → RUN stabile
```

Non aprire una nuova issue se i test/gate esistenti coprono già T0.
Se manca solo una verifica aggregata, valuta una piccola issue di acceptance/seduta, non una nuova feature.

---

## T1 — Visual Scenario Playback nell'Editor — P0 Trial

Il technical designer deve poter **vedere** la resolution prodotta dal runtime.

Controlli minimi desiderati:

```text
RUN
PAUSE
NEXT/PREV PHASE
NEXT/PREV TURN
RESET
speed: 0.25x / 0.5x / 1x / 2x / 4x / Instant
```

Il playback:
- consuma TurnLog/replay/eventi canonici;
- non riesegue le regole;
- non chiama un resolver alternativo;
- non diventa authority.

Visualizzazione graybox sufficiente:
- unit position;
- movement;
- facing;
- target;
- attack line;
- AoE;
- damage/shield;
- push/pull;
- status;
- KO;
- terrain/structure change;
- reaction trigger/decision se presenti.

### Azione richiesta
Cerca issue esistente.
Se non esiste, crea una sub-issue di #1105.
Non riutilizzare #472 se questo ne altera il perimetro player-facing.

---

## T2 — Combat Intent Authoring

Il Composer oggi ha una thin slice Move/Wait.
Il formato `FRTScenarioIntent` possiede già più campi/capability.

Target visual authoring, solo per capability runtime realmente supportate:

- Wait
- Move
- Basic Attack
- Ability
- Dash
- Brace/defensive action se il vocabolario runtime la tratta come intent
- Overwatch/predictive action solo nel modo già canonico
- Interact solo se già modellato

Per ogni action la UI deve compilare il **dato canonico esistente**.

Esempi di proprietà:
- `AbilityId`
- target unit
- target cell
- facing/direction
- AoE/shape se è dato e non risultato
- destination/path input nei limiti del formato

Niente nuovo vocabolario se `FRTScenarioIntent` lo esprime già.

### Azione richiesta
Cerca issue esistente.
Se assente, crea issue sotto #1105 con acceptance criteria verificabili.

---

## T3 — Multi-turn Scenario Timeline

`FRTTestScenario` supporta già `Turns[]`.

Serve authoring visuale per più turni:

- `+ Turn`
- select turn;
- duplicate;
- delete;
- reorder solo se semanticamente valido;
- enable/disable SOLO se il formato ha un modo canonico; altrimenti non inventarlo;
- Run All;
- eventuale Run To Here solo se può essere implementato come esecuzione canonica fino al boundary, non come simulazione separata.

Il dato canonico resta `FRTScenarioTurn`.

---

## T4 — Reaction / Decision Authoring

Il formato supporta già `FRTScenarioDecision` e risposta scriptata:

```text
FIRE
HOLD
```

La Trial deve consentire di authorare le decisioni necessarie agli scenari di reaction/Overwatch senza JSON manuale.

Target:
- associare decisione a unit/opportunity secondo il vocabolario esistente;
- vedere durante playback:
  trigger → decision boundary → FIRE/HOLD → conseguenza;
- nessun reaction resolver Editor-specific.

Cerca issue esistente prima di crearne una.

---

## T5 — Initial State + Environment authoring

Misura la struct corrente: non affidarti alla lista di questo prompt.

La Trial dovrebbe permettere, se il formato/runtime già lo supporta o può essere esteso senza duplicarlo:

### Unit
- Hero
- Team
- Cell
- Facing
- HP
- Shield
- Energy
- Status
- cooldown/loadout dove canonico

### World
- surfaces/states;
- water/fire/smoke/ice ecc.;
- door/structure state;
- cover state;
- objective state;
- team score se il modello scenario lo consente.

La spec Tactical Designer aveva già identificato gap quali:
- status iniziali;
- stato ambiente;
- seed;
- ability overrides nelle varianti.

NON aggiungerli tutti automaticamente.
Per ciascuno:
1. misura se esiste già;
2. stabilisci quale owner deve contenerlo;
3. apri issue solo se serve alla Trial e non esiste già.

---

## T6 — Runtime Tactical Probes

#711 copre già il **Movement Probe**.

La Trial ideale deve poter spiegare, tramite query runtime:

- Reachability;
- Path + cost;
- exclusion reason;
- LOS;
- cover;
- targeting legality;
- facing;
- displacement legality;
- reaction legality;
- environment interaction.

Non creare una mega-issue se gli owner sono diversi.
Valuta:
- aggiornare #711 solo per ciò che è davvero movement;
- creare issue sorelle sottili per LOS/Targeting/etc. se mancanti.

Ogni probe deve rispondere:

```text
VALID / INVALID
ReasonCode
dati rilevanti
```

Non creare un nuovo reason vocabulary nell'Editor.

---

## T7 — Result Inspector / State Diff

Dopo RUN servono almeno:

### Result
- PASS / FAIL / ERROR / BLOCKED
- expected vs actual;
- hash disponibili;
- scenario/turn identity.

### TurnLog
filtri visuali che NON cambiano il dato:
- movement;
- combat;
- reaction;
- environment;
- status;
- objective/errors.

### State Diff
Se il runtime espone in modo affidabile initial/final state:
- unit cell before/after;
- HP;
- shield;
- energy;
- status;
- facing;
- struttura/terreno quando utile.

Un click su un evento dovrebbe poter focalizzare gli elementi coinvolti se è presentation-only.

Cerca prima se esiste un inspector equivalente.

---

## T8 — Scenario Preset / Templates

Valuta un piccolo sistema di template canonici per accelerare l'authoring:

- Empty
- Movement
- Duel 1v1
- 2v2
- Cover Test
- Environment Test
- Reaction Test
- Objective Test

I preset devono produrre **normali `FRTTestScenario`**.
Nessun formato parallelo.

Questa è P3/P4: se allarga troppo la Trial, documentala come post-Trial invece di aprirla subito.

---

# 6. SCOPE DELLA TRIAL

## IN
La Trial è pronta quando un designer può, senza editare JSON/C++:

1. creare/aprire un 2v2;
2. piazzare le unità;
3. impostare stato iniziale sufficiente;
4. authorare almeno Move + Attack + Ability + una reaction/decision rappresentativa;
5. lavorare su più turni;
6. validare;
7. eseguire tramite Scenario Harness reale;
8. vedere la resolution nel viewport;
9. leggere TurnLog/result;
10. capire almeno i principali `reason`;
11. Reset;
12. modificare;
13. Run di nuovo;
14. Save/reload senza perdere Stable ID/dati;
15. ottenere lo stesso risultato logico headless.

## OUT
Non mettere nel gate Trial:

- Skill Workbench completo;
- visual ability scripting;
- balance dashboard;
- mass simulation / E43;
- bot tournament;
- heatmap;
- promotion verso dati production;
- modding pubblico;
- networking tool;
- UI/art finali;
- animazioni finali;
- export video;
- editor mappe runtime player-facing.

---

# 7. RICONCILIAZIONE DELLA DOCUMENTAZIONE

## 7.1 `spec-tactical-designer.md`

Questo file è owner del **concetto/confine**, NON tracker.

Aggiornalo per:
- riconoscere che Scenario Composer visual authoring/run non è più solo "dati sì, authoring no", se il codice lo smentisce;
- descrivere la TD Trial come **first usable designer loop**, non come release;
- mantenere l'invariante runtime/editor;
- aggiungere il ruolo del visual playback come consumer di TurnLog/replay;
- chiarire il confine player Replay Viewer vs Editor Scenario Playback;
- aggiornare l'elenco dei gap scenario solo dopo aver rimisurato la struct corrente;
- NON aggiungere una tabella di stato destinata a marcire.

### IMPORTANTE — D-181
Il documento contiene/ha contenuto testo che dice che lo stato vive nel `feature-registry.yaml`.

D-181 ha rimosso Feature Registry, Control Center e shortlist il 2026-08-21.

Se quella prescrizione è ancora presente nella parte CURRENT:
- correggila;
- indica le autorità correnti reali (`roadmap-v0.1.md`, `roadmap-checkpoint.md`, issue GitHub) secondo le regole già adottate nel repo;
- preserva eventuali note storiche come tali.

---

## 7.2 `roadmap-checkpoint.md`

Questa è la vista di **esecuzione**.

Rimisura M9 e in particolare M9.4 / Tactical Designer.

Aggiorna:
- stato reale;
- checkpoint/DoD necessari alla Trial;
- dipendenze;
- metodo di verifica;
- quali elementi sono Editor/PIE e quali Automation/headless.

Non duplicare stato per-feature che non appartiene a questa vista.

Se è utile, suddividi M9.4 in checkpoint coerenti con il metodo già usato dal file, ma NON creare una seconda famiglia di milestone.

---

## 7.3 `roadmap-v0.1.md`

Questa è la vista di **release/epic**.

Il Tactical Designer è stato definito `out_of_release_scope`.

NON trasformarlo in una nuova epic E* o in requisito v0.1 senza una decisione esplicita che cambi quel principio.

Aggiorna `roadmap-v0.1.md` SOLO se:
- contiene stato stale che questa riconciliazione deve correggere;
- serve una nota di relazione/strumento che non cambia lo scope release;
- una decisione già esistente richiede l'allineamento.

Non infilare la TD Trial nella v0.1 "dalla porta di servizio".

---

## 7.4 `roadmap-checkpoint.md` vs `roadmap-v0.1.md`

Mantieni la distinzione:

```text
roadmap-checkpoint.md = esecuzione / milestone / checkpoint
roadmap-v0.1.md       = release / epic
GitHub issues         = lavoro concreto
spec-tactical-designer.md = concetto / confini / owner
```

Una stessa informazione non va copiata in quattro posti se non serve.

---

# 8. AGGIORNAMENTO EPIC #1105

Rimisura l'epic live e aggiorna il corpo.

Obiettivi:

1. eliminare stato palesemente stale;
2. riconoscere #1114–#1117 come consegnate;
3. riconoscere #623 come chiusa se ancora vero;
4. mantenere #622/#695/#711 aperte solo se ancora aperte;
5. aggiornare la scala di maturità TD 0.1–0.9 senza confonderla con la Trial;
6. introdurre una sezione:
   **"TD Trial / Scenario Sandbox — first usable loop"**
7. linkare le issue concrete che coprono le Trial Slice;
8. mantenere `out_of_release_scope` salvo decisione contraria già presente;
9. NON assegnare un numero `E` all'epic;
10. NON cambiare nomi di classi/API;
11. NON ricreare Feature IDs come registro se sono diventati identificatori orfani dopo D-181.

La sezione Trial deve indicare:
- gate di ingresso;
- gate di uscita;
- issue child;
- cosa è riuso;
- cosa è nuovo;
- cosa è esplicitamente post-Trial.

---

# 9. ISSUE: REGOLE DI CREAZIONE/AGGIORNAMENTO

Per ogni Trial Slice:

1. cerca issue equivalente;
2. apri e leggi anche issue chiuse correlate;
3. misura il codice/formato se la issue afferma che qualcosa "non esiste";
4. se esiste un owner corretto:
   - aggiorna/linka;
5. se non esiste:
   - crea una issue piccola;
6. collega la issue a #1105 come sub-issue se il repository/GitHub lo consente e se la relazione è corretta;
7. assegna priorità coerente con il sistema corrente, non inventare label;
8. non assegnare release `v0.1` solo perché la Trial è utile;
9. ogni issue deve avere:
   - Why;
   - cosa esiste già e non va rifatto;
   - Scope;
   - Out of scope;
   - Acceptance criteria misurabili;
   - test/PIE;
   - dependencies;
   - tracking/document owner;
   - guardrail "no second simulator" dove applicabile.

### Ordine di priorità Trial proposto

**P0 Trial**
- visual playback Editor;
- combat intents sufficienti;
- multi-turn;
- reaction decision authoring minimo.

**P1 Trial**
- initial state/environment sufficiente;
- probes necessari a capire un esito;
- result/TurnLog inspector.

**P2/P3**
- state diff avanzato;
- preset/templates;
- comfort UI non necessaria al loop.

Adatta le label reali del repository; NON creare una label `P0 Trial` se non esiste.

---

# 10. ISSUE ESISTENTI DA NON DISTORCERE

## #622
Workspace grid.
Resta Map Editor authoring.
Non caricarla di Scenario Composer.

## #623
Se è chiusa e DoD verificato, non riaprirla per la Trial.
Può essere dipendenza/allestimento già pagato.

## #695
Door visualization.
È una capability di leggibilità/probe, non il cuore del Scenario Composer.
Rispetta dipendenza da interaction graph se ancora valida.

## #711
Movement Probe.
Usala come prima slice del sistema "Why?".
Non trasformarla in mega-probe LOS/targeting se questo rompe ownership/scope.

## #472
Replay Viewer player-facing.
Riusa il core dove possibile, ma non cambiare silentemente il suo attore principale da giocatore a technical designer.

## #1515
Se i bug di validazione Scenario Harness sono ancora aperti, valuta se sono blocker della Trial.
Non duplicarli in issue nuove.

## #1540
La Trial usa hash per dimostrare parità/determinismo.
Se i test guardiani per Shield/Energy/Layer sono ancora aperti, dichiarali come rischio/dipendenza di verifica, non come feature TD.

---

# 11. NUOVO DOCUMENTO DI RICONCILIAZIONE

Crea:

`docs/roadmap/plans/tactical-designer-trial-reconciliation-2026-08-29.md`

Deve essere un **referto di misura e piano**, non una nuova autorità.

Struttura minima:

```markdown
# Tactical Designer Trial — reconciliation 2026-08-29

## 1. Domanda
## 2. Fonti e commit misurato
## 3. Stato reale del Tactical Designer
## 4. Cosa è già consegnato
## 5. Gap misurati
## 6. Duplicati / issue riusate
## 7. Trial Slice T0–T8 → owner/issue
## 8. Dependency graph
## 9. Gate TD Trial
## 10. Out of scope
## 11. Documenti aggiornati
## 12. Issue create/modificate
## 13. Decisioni non prese
## 14. Rischi / debito rilevato
```

Inserisci una tabella finale del tipo:

| Trial Slice | Capability | Existing owner | Issue | Stato | Azione |
|---|---|---|---|---|---|
| T0 | baseline | Scenario Harness | #... | done | verify only |
| T1 | editor playback | Replay core + Editor | #... | ... | create/update |
| ... | ... | ... | ... | ... | ... |

---

# 12. DECISION LOG

NON creare automaticamente una nuova D-nnn.

Prima verifica se:
- D-154 già copre la collocazione del Tactical Designer;
- D-181 già copre la rimozione del Feature Registry;
- ADR-0009 già copre replay/TurnLog;
- altre decisioni già coprono Scenario Harness e authority.

Crea una nuova decisione SOLO se la TD Trial richiede una scelta architetturale nuova e duratura che non può essere espressa come semplice scope di epic/issue.

Esempio di decisione potenzialmente nuova:
"Editor visual playback è un consumer del replay logico canonico e non un percorso di simulazione."

Ma prima verifica se questo è già conseguenza sufficiente di D-154 + ADR-0009.
Se sì, NON creare duplicato.

---

# 13. VALIDAZIONE DOPO LE MODIFICHE

Dopo gli aggiornamenti:

### Git/docs
- `git diff --check`
- cerca link/path rotti nei file modificati con gli strumenti attualmente presenti nel repo;
- cerca riferimenti CURRENT a:
  - `feature-registry.yaml`
  - `feature_registry.py`
  - Control Center
  - shortlist generate
- distingui riferimenti storici legittimi da prescrizioni stale.

### Tracking
Verifica con `gh issue view`:
- #1105;
- tutte le issue create/modificate;
- parent/sub-issue se supportato;
- label;
- stato;
- eventuale milestone release.

### Coerenza
Deve essere vero:

```text
spec non è tracker
roadmap-checkpoint non è release epic list
roadmap-v0.1 non assorbe tool out-of-release
issue descrivono lavoro concreto
Editor non diventa authority
```

### Non modificare
Questa attività non deve produrre diff in:
- `Source/**`
- `Content/**`
- `.umap`
- `.uasset`

Se per capire lo stato devi leggere codice, fallo.
Non modificarlo.

---

# 14. GIT

Segui le convenzioni di `CLAUDE.md`.

Se il repository richiede branch dedicato, crealo secondo naming convention esistente.

Commit suggerito, solo se coerente con le convenzioni:

```text
docs(tactical-designer): define scenario sandbox trial roadmap
```

Se gli aggiornamenti GitHub issue avvengono separatamente dal commit, nel report finale elenca esattamente:
- issue create;
- issue editate;
- issue lasciate intenzionalmente invariate;
- documenti modificati.

Non pushare/mergiare se `CLAUDE.md` o la policy corrente richiedono un passaggio diverso.

---

# 15. OUTPUT FINALE CHE VOGLIO DA TE

Alla fine non limitarti a dire "fatto".

Restituisci:

## A. Baseline misurata
- commit/branch;
- stato #1105;
- stato issue principali;
- gap reali trovati.

## B. Modifiche GitHub
Tabella:

| Issue | Prima | Dopo | Motivo |
|---|---|---|---|

## C. Issue nuove
Per ognuna:
- numero;
- titolo;
- parent;
- priorità;
- dipendenze;
- Trial Slice coperta.

## D. Documenti modificati
Per file:
- cosa hai cambiato;
- perché;
- quale autorità possiede ora l'informazione.

## E. Mappa Trial finale
Ordine suggerito:

```text
Baseline Composer
    ↓
Visual Playback
    ↓
Combat Intent Authoring
    ↓
Multi-turn
    ↓
Reaction Decisions
    ↓
Initial State / Environment
    ↓
Runtime Probes
    ↓
Result Inspector
    ↓
Optional Presets
    ↓
TD TRIAL READY
```

Correggi questo ordine se il dependency graph reale dimostra che è sbagliato.

## F. Gate TD Trial
Lista corta e misurabile.

## G. Cose NON fatte
Elenca esplicitamente:
- feature rinviate;
- decisioni non prese;
- issue che hai scelto di non creare perché duplicate;
- eventuali conflitti o blocchi.

## H. Git
- file changed;
- commit;
- branch;
- status finale.

---

# 16. PRINCIPIO FINALE

La riuscita di questa attività NON si misura dal numero di nuove issue.

Si misura da questo:

> Dopo la riconciliazione, un altro sviluppatore deve poter aprire #1105 e i documenti correnti e capire senza ambiguità **quale lavoro manca per avere una TD Trial utilizzabile**, in quale ordine farlo, quale sistema possiede ogni risposta, quali pezzi esistono già e quali non devono essere riscritti.

Se scopri che una Trial Slice è già implementata, **correggi la roadmap invece di aprire lavoro finto**.

Se scopri che una proposta di questo prompt confligge con il codice o con una decisione CURRENT, **vince il repository misurato**: documenta la correzione nel referto e procedi con la versione coerente.
