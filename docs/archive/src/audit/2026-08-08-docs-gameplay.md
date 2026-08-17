# RefactorTactics — Audit `docs/gameplay/` e piano di consolidamento per Claude Code

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

**Data audit:** 2026-08-08  
**Repository verificata:** `DegrassiAaron/refactor-tactics-main`  
**Scopo:** fornire a Claude Code un handoff operativo per ripulire e consolidare la documentazione gameplay senza reintrodurre decisioni superate, senza inventare nuove regole e senza confondere stato implementato, decisioni approvate, baseline da playtestare e north-star.

> **IMPORTANTE — questo è un handoff esecutivo di consolidamento, non un permesso a cambiare arbitrariamente il design.**
> Le decisioni `D-014`…`D-019` in §4 sono state approvate il **2026-08-08** e vanno trattate come input normativo del consolidamento. Se emergono conflitti nuovi non coperti da queste decisioni, Claude deve segnalarli invece di scegliere silenziosamente.

---

## 0. Workflow obbligatorio per Claude Code

Prima di modificare qualsiasi file:

```text
git status
git branch --show-current
git rev-parse HEAD
```

Poi leggere almeno:

```text
CLAUDE.md
AGENTS.md                    # se presente
README.md

docs/product/piano-canonico-mvp.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/decisions/adr-0003-modello-azioni-v01.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/decisions/adr-0005-orientamento.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md

docs/balance/RT_ActionCatalog_v0.1.md
docs/balance/RT_HeroCatalog_v0.1.md
docs/balance/RT_TerrainCatalog_v0.1.md

docs/technical/brief-planning-visuale.md
docs/gameplay/*
docs/src/* recenti del 2026-08-07
```

Verificare inoltre il codice e i test reali prima di dichiarare una feature implementata o assente.

### Regola di prevalenza operativa

Usare questa gerarchia:

```text
1. Decisioni esplicite più recenti dell'utente già registrate in ADR / Decision Log
2. Codice corrente + test automatici realmente presenti
3. piano-canonico-mvp.md
4. ADR correnti
5. Decision Log corrente
6. Cataloghi balance correnti
7. Roadmap corrente
8. Brief/spec recenti coerenti con 1–7
9. Handoff docs/src recenti
10. PDR / PDF / workbook / documenti storici
```

Se 1–8 sono in conflitto, **non scegliere silenziosamente**: aprire una sezione `Conflict / Decision required`.

---

# 1. Stato del gioco da assumere come baseline corrente

Questi punti sono già sufficientemente consolidati e non vanno riaperti durante il cleanup documentale.

## 1.1 Turno / round

```text
Planning
  → Commit
  → Prep
  → Dash
  → Blast
  → Move
  → Cleanup
```

- Il normale `Move` è l'ultima fase/azione volontaria standard.
- Dash, Charge, Leap, Blink, displacement e movimento reattivo non sono `Move`.
- Le azioni normali vengono configurate in Planning.
- La Resolution non riapre un Planning libero.

## 1.2 Fast Reaction

Modello corrente:

```text
Reaction armed
  → pure trigger evaluation
  → Reaction Opportunity
      → 0/1 legal response: immediate commit, no window
      → 2+ legal responses: deterministic decision boundary
          → Fast Reaction window
          → response becomes authoritative input
          → new segment snapshot
          → resume
```

Baseline:

```text
FastReactionDuration = 3.0 s
Timeout = HOLD
Overwatch charge = 1
HOLD keeps reaction armed
same-microstep targets = one multi-target opportunity
no nested interactive reaction stack in v0.1
simulation pauses globally at a decision boundary
presentation may slow down, but never decides logic
```

## 1.3 Facing

`Facing` è stato logico autorevole.

- `Linear*`: facing derivato dalla direzione del movimento.
- `Budget Move`: il planning sceglie fra ultimo passo e le due direzioni adiacenti.
- Da fermo: rotazione libera fra le 6 direzioni, senza consumare slot.
- Movimento forzato con sorgente: facing verso la sorgente dell'ultimo displacement.
- Consuma una sola geometria frontale condivisa da:
  - difesa;
  - percezione;
  - Overwatch.
- Il cono Overwatch deriva dal facing, non da una seconda `Direction` indipendente.

## 1.4 Roster

Roster operativo v0.1:

```text
Gadget
Phase
Riktor
Wraith
```

Vertical slice/showcase:

```text
2v2
Gadget + Phase
vs
Riktor + Wraith
```

Nomi storici come Aegis/Nyx/Drift/Vex, Mara/Ivo/Nyx/Sol o roster Paragon non devono tornare nelle specifiche operative salvo sezioni storiche.

## 1.5 Formato partita

- Il **formato principale finale non è ancora deciso**.
- `3v3` resta una baseline di lavoro / ipotesi.
- `4v4` è uno scenario di **stress e design validation**, non il formato principale.
- Il vertical slice v0.1 resta 2v2.
- Non consolidare 3v3 o 4v4 come formato finale finché non esiste playtest ≥3v3 sufficiente.

## 1.6 Conoscenza parziale

Distinguere:

```text
LOS geometrica
Detection / contatto
Identification
Team Knowledge
Memory / ultimo contatto
```

Stati correnti di conoscenza:

```text
Nascosto
ContattoIncerto
Rilevato
+ UltimoContatto come memoria
```

Il rumore è un **canale informativo** che alimenta lo stesso `TeamKnowledge`, non un secondo sistema indipendente.

## 1.7 Overwatch universale

Decisione corrente:

```text
Attack OR Ability OR Overwatch
```

salvo eccezione dichiarata.

Overwatch è universale come **framework/postura/comando di Planning**, ma il profilo dipende dall'eroe.

`Automatic`, `Conditional`, `FastSelect` non devono diventare un enum parallelo: emergono da `AllowedResponses` + eventuale condizione dichiarata in Planning.

---

# 2. Audit file-per-file di `docs/gameplay/`

Legenda:

- `KEEP` — sostanzialmente corretto; solo link/status/changelog.
- `UPDATE` — valido ma contiene parti stale.
- `REWRITE` — contraddizioni operative importanti.
- `ARCHIVE` — storico/non normativo; non deve vivere come spec attiva.
- `DECISION` — serve decisione utente prima del consolidamento definitivo.

---

## 2.1 `brief-azioni-generiche-overwatch.md`

**Verdetto:** `UPDATE — decisioni chiuse in D-014/D-015`

### Corretto e ora canonico

- Overwatch universale come framework/postura.
- Costo opportunità: `Attack OR Ability OR Overwatch`, salvo eccezione dichiarata.
- Profilo Overwatch per eroe.
- Regimi Automatic/Conditional/FastSelect derivati dai dati.
- 3 s, timeout HOLD, HOLD non consuma la charge.
- Le azioni generiche canoniche sono:

```text
Wait
BasicAttack
Interact
Brace
Move
Overwatch
```

- `Activate` viene assorbita semanticamente da `Interact`.
- `Guard` non è più una fondamentale universale: resta disponibile come capacità/stance specifica dove il catalogo o il kit la richiedono.
- `Sneak / Normal / Sprint` sono profili della stessa famiglia `Move`.
- `Sprint` **non è un Dash**: il Dash resta una mobilità speciale pre-Blast; Sprint appartiene alla famiglia Move e quindi non deve insegnare una seconda semantica di fase.

### Migrazione obbligatoria, non cancellazione cieca

Il codice/catalogo corrente può ancora contenere Stable ID come:

```text
Action.Guard
Action.Activate
Action.Sprint
```

Non eliminarli o rinominarli brutalmente.

Claude deve:
1. verificare tutti i consumer reali;
2. distinguere identità legacy da semantica gameplay corrente;
3. usare deprecation/redirect/versioning se il repository dispone già di tale meccanismo;
4. creare o aggiornare issue di migrazione per codice e dati se il refactor non è sicuro nella stessa PR;
5. mantenere compatibilità di TurnLog/replay dove necessaria.

### Action economy ancora tunable

La tassonomia è chiusa, ma i **costi esatti** dei profili `Sneak` e `Sprint` — inclusa l'eventuale occupazione aggiuntiva di slot da parte di Sprint — restano parametri di balance/playtest. Non devono bloccare il consolidamento documentale e non vanno inventati.

---

## 2.2 `brief-conoscenza-parziale.md`

**Verdetto:** `REWRITE parziale`

### Corretto

- separazione LOS / detection / identification;
- TeamKnowledge;
- bot non onnisciente;
- rumore come secondo canale informativo;
- contatto incerto;
- memoria dell'ultimo contatto;
- propagazione sul grafo.

### Da correggere

1. La decisione storica `D15` che vieta finestre Fast Reaction acustiche perché `D7` vietava finestre è **superata da ADR-0004**.
2. La geometria visiva originariamente omnidirezionale / ricalcolata solo secondo il vecchio modello va allineata ad **ADR-0005**:
   - vista piena nel cono frontale;
   - awareness 360° entro 2 celle;
   - facing come prerequisito.
3. `Fog of War` va usato con attenzione: la mappa statica resta nota; il sistema corrente è soprattutto **conoscenza parziale delle unità/eventi**.
4. Il bonus di vista di `HighGround` non ha ancora una regola numerica approvata.

### Azione Claude

- Rimuovere il vecchio divieto assoluto sulle finestre acustiche.
- Scrivere invece: un evento acustico **può** generare una `ReactionOpportunity` solo se una reaction/profile lo dichiara; il regime Automatic/Conditional/FastSelect decide se apre davvero una finestra.
- Non inventare una Fast Reaction acustica di default.
- Collegare la percezione ad ADR-0005.
- Applicare D-018: `HighGround` **non** fornisce un bonus numerico piatto a `VisionRange` nella v0.1. La quota modifica la geometria/LOS/topologia; eventuali bonus numerici futuri richiedono playtest e una decisione separata.

---

## 2.3 `brief-delayed-actions.md`

**Verdetto:** `UPDATE — thin slice v0.1 approvato in D-016`

### Corretto

- Delayed Action completamente dichiarata in Planning.
- Boundary logici nominati.
- Nessun nuovo input umano.
- Target predittivo `LockCell/LockLine/LockArea/LockDirection`.
- Stato reale valutato al boundary.
- D-013 trigger su transizione come dato dell'azione, non della mappa.

### Decisione di scope

La v0.1 deve includere **un solo thin slice di Predictive Action** per rendere visibile il pilastro della predizione senza introdurre subito l'intero framework di trappole.

Target preferito:

```text
Hero.Wraith.InterceptShot
```

Semantica:

```text
Planning
→ previsione completamente dichiarata
→ trigger/boundary deterministico
→ se la previsione è soddisfatta: risoluzione automatica
→ altrimenti: whiff/fallback dichiarato
→ nessun input umano durante la Resolution
```

Questa azione **non deve essere trasformata in Fast Reaction**.

### Fuori scope v0.1

- framework completo traps/mines/persistent gambits;
- editor visuale di trigger;
- catene arbitrarie di predictive actions;
- nuovi interrupt annidati.

### Azione Claude

- aggiornare roadmap/issue affinché il thin slice predittivo sia esplicito;
- verificare dove `Hero.Wraith.InterceptShot` è classificato oggi;
- se dati/codice lo trattano ancora come Reaction, segnalare la migrazione necessaria invece di conservare due semantiche parallele;
- non inventare in questa PR nuovi costi/cooldown/slot se il catalogo corrente non li risolve chiaramente: la **semantica predittiva** è bloccata, i numeri restano balance data.

---

## 2.4 `brief-ghiaccio.md`

**Verdetto:** `KEEP + piccole correzioni`

### Corretto

- slide base già implementato e da mantenere;
- momentum/traction/prone/cracked ice/rottura come motore futuro;
- interazioni termiche avanzate fuori v0.1;
- legame con rumore e propagazione condivisa.

### Da correggere

- Assicurare che l'ordine Cleanup sia rimandato alla **spec ambientale più recente**, non duplicato.
- `CrackedIce` resta fuori scope salvo decisione esplicita.
- Divergenza numerica sul rumore dell'acqua deve essere risolta nel catalogo di balance, non qui.

---

## 2.5 `brief-overwatch-reazioni.md`

**Verdetto:** `UPDATE`

### Corretto

È il brief corretto per:
- opportunity→commit;
- FIRE/HOLD;
- 3 s;
- timeout HOLD;
- detection;
- trigger multi-target simultanei;
- niente dati futuri;
- niente nested reaction.

### Da aggiungere

- D-012: Overwatch universale e costo opportunità.
- Profilo per eroe.
- Regimi Automatic/Conditional/FastSelect dal nuovo brief generico.
- ADR-0005: il cono nasce da `Facing`, non da una `Direction` separata.

---

## 2.6 `brief-unita-ausiliarie.md`

**Verdetto:** `KEEP`

È coerente e utile.

### Piccolo aggiornamento

Rafforzare A4:

> il codice non deve assumere `TeamSize == 2`, ma neppure `==3` o `==4`.

Il 4v4 stress serve proprio a scoprire questi hard-code.

---

## 2.7 `sequenza-turno.md`

**Verdetto:** `ARCHIVE`

È un documento esplorativo basato su:
- progressive reveal;
- stack LIFO interattivo;
- 5 speed categories;
- Patch;
- JSON;
- timeline 45–60 s.

Non è più una specifica corrente.

### Azione Claude

Preferito:

```text
docs/archive/gameplay/sequenza-turno-exploratory.md
```

oppure mantenere il path solo con un header inequivocabile:

```text
HISTORICAL / NORTH-STAR / NON NORMATIVE
```

e link alla spec canonica nuova.

Non cancellare la ricerca: preservarla come provenance.

---

## 2.8 `spec-anima-risoluzione.md`

**Verdetto:** `REWRITE CRITICO`

### Problema

Il documento assume ancora, in parti importanti:

```text
LockIn
→ calcola tutto il risultato
→ produce timeline completa
→ playback
```

Questo non regge più con ADR-0004.

Con una Fast Reaction vera, una scelta live può cambiare i segmenti successivi. Non si può considerare sempre tutta la timeline futura come già conclusa al lock-in.

### Modello corretto

```text
Commit
→ snapshot segmento
→ resolve segment
→ emit authoritative events
→ playback segment
→ if no decision boundary: continue
→ if decision boundary:
     globally pause logical simulation
     present reaction window
     response becomes authoritative input
     snapshot next segment
     continue resolution
→ Cleanup
```

Il playback resta presentation-only.

### Da correggere anche

- ogni frase “stack/reazioni fuori MVP” è superata;
- le bande 3v3 devono essere marcate come working baseline, non formato consolidato;
- mantenere 8–15 s per 2v2 / 12–20 s baseline ≥3v3 come target da misurare, non regola logica.

---

## 2.9 `spec-bot-utility.md`

**Verdetto:** `ARCHIVE`

È esplicitamente il bot quadrato storico.

### Cosa manca

Serve una **spec corrente del bot hex** che raccolga almeno:

- utility scoring attuale;
- partial knowledge / TeamKnowledge;
- facing/perception;
- reaction policy;
- deterministic tie-break;
- 4v4 stress;
- nessun accesso a hidden enemy intents.

Non è necessario creare subito una nuova AI: serve almeno una documentazione corrente che punti a `URTHexBotLibrary`.

---

## 2.10 `spec-copertura-cp91.md`

**Verdetto:** `UPDATE — D-017 chiude il gap Intercept`

È una buona spec chiusa, ma il limite storico su `Intercept` è ora superato.

### Regola canonica

Quando un colpo originariamente destinato ad A viene intercettato e il bersaglio effettivo diventa B:

1. l'identità dell'azione sorgente e i dati già risolti che non dipendono dal target restano invariati;
2. la geometria target-dependent viene rivalidata rispetto a **B**;
3. LOS/traiettoria, cover applicabile, facing e difese geometriche usano il bersaglio effettivo;
4. la rivalidazione **non genera una nuova Reaction Opportunity**;
5. nessun nested reaction/loop viene introdotto.

Il TurnLog deve spiegare il redirect e il contesto difensivo effettivamente usato.

---

## 2.11 `spec-copertura-alta-cp92.md`

**Verdetto:** `KEEP`

Nessun conflitto importante con le discussioni recenti.

### Follow-up

Conservare come rischio:
- assunzione `HexLine` su passi adiacenti;
- bot non ancora cover-aware;
- creazione cover temporanee rimandata.

---

## 2.12 `spec-dash.md`

**Verdetto:** `UPDATE`

### Contraddizione interna

La parte aggiornata dice correttamente:

```text
Dash lineare
ostacolo lo ferma
MovementStyle decide
```

ma nei limiti resta testo vecchio equivalente a:

> scelto il pathfinding, il dash aggira ostacoli.

È falso rispetto allo stato corrente.

### Da aggiungere

- Integrazione con ADR-0005: `LinearDash/Charge/Leap` derivano il facing.
- Chiarire che “bot dash-only” è una policy AI, non una regola del sistema.
- Applicare D-015: `Sprint` è un profilo della famiglia `Move`, non un Dash. La spec Dash deve descrivere solo mobilità speciali pre-Blast e deve marcare l'eventuale `Action.Sprint` legacy come debito di migrazione/catalogo.

---

## 2.13 `spec-durata-partita-e-scala-mappe.md`

**Verdetto:** `REWRITE IMPORTANTE`

### Errore 1 — formato principale

Parla ancora di:

```text
3v3 Standard — formato competitivo principale
```

Questo è superato da **D-011**.

Forma corretta:

```text
3v3 = working baseline / hypothesis
4v4 = stress validation only
primary competitive format = OPEN
2v2 = current v0.1 slice
```

I numeri 3v3 possono restare come baseline di confronto, ma non come decisione normativa.

### Errore 2 — Fast Action

Nel glossario viene usato “Fast Action” per l'azione dichiarata in Planning che risolve più tardi.

Questo va rinominato:

```text
Delayed Action / Predictive Action
```

`Fast Action` deve restare una **decisione live limitata** esplicitamente autorizzata, distinta dal Planning normale.

### Manca

- cross-reference a E17 / stress 4v4;
- nota chiara che non si decide il formato con questo documento.

---

## 2.14 `spec-fuoco-acqua-cp84.md`

**Verdetto:** `KEEP`

Spec chiusa e coerente.

### Piccolo aggiornamento

Aggiungere link a `brief-ghiaccio.md` per chiarire che:
- freeze;
- melt;
- steam;
- transizioni termiche avanzate

restano fuori v0.1 salvo futura epic.

---

## 2.15 `spec-knockback.md`

**Verdetto:** `ARCHIVE`

È esplicitamente il substrato quadrato storico.

### Cosa manca

Una nota/spec corrente del knockback hex deve includere ADR-0005:

- il forced displacement con sorgente cambia facing verso la sorgente;
- quello ambientale senza sorgente lascia il facing invariato;
- Move volontario successivo prevale.

Non serve una nuova meccanica: serve documentare quella corrente.

---

## 2.16 `spec-motore-azioni-e4.md`

**Verdetto:** `UPDATE / possibile ARCHIVE-AS-BUILT`

### Problema

Lo stato dice ancora:

```text
proposta da approvare
```

ma E4 è stata implementata/chiusa.

Le “domande aperte” originali sono in parte già superate dal codice e dai checkpoint successivi.

### Decisione successiva da registrare

La spec E4 usa storicamente come fondamentali:

```text
Wait, Move, BasicAttack, Guard, Activate, Interact
```

D-014/D-015 hanno successivamente consolidato:

```text
Wait
BasicAttack
Interact
Brace
Move
Overwatch

MoveProfile = Sneak | Normal | Sprint
```

Questa differenza non va nascosta.

### Strategia obbligatoria

Trasformare `spec-motore-azioni-e4.md` in:

```text
AS-BUILT / historical implementation spec for E4
```

e aggiungere una sezione `Superseded by later gameplay decisions` che punti a D-014/D-015.

Non riscrivere retroattivamente la storia di E4 come se il nuovo modello fosse sempre esistito. Eventuali migrazioni di Stable ID/codice vanno tracciate separatamente.

---

## 2.17 `spec-pacing-turno.md`

**Verdetto:** `UPDATE leggero`

Il metodo di misura è valido.

### Da ripulire

La premessa storica:

```text
reazioni pre-committed, nessuna finestra
```

è già segnalata come superata, ma resta troppo prominente.

Riscrivere la testa del documento distinguendo:

```text
historical premise
current ADR-0004 model
```

e misurare Fast Reaction come Decision Time separato.

Le bande 3v3 restano baseline, non formato consolidato.

---

## 2.18 `spec-propagazione-elettrica-cp83.md`

**Verdetto:** `KEEP`

BFS sul grafo conduttivo, limite, determinismo e distinzione cella Wet/unità Wet sono coerenti.

### Da verificare, senza assumere

La frase “nessun eroe usa Action.Electrify” è uno **snapshot del CP**. Verificare il catalogo eroi corrente prima di lasciarla come stato presente.

### Evoluzione architetturale

La discussione sul rumore propone una `Propagation Query` generale riusabile da:
- suono;
- elettricità;
- calore.

Non refactorizzare CP83 solo per simmetria finché una issue reale non lo richiede.

---

## 2.19 `spec-reazioni-componibili-cp55.md`

**Verdetto:** `UPDATE`

È valida come storia/as-built di E5.

### Da aggiungere in testa

ADR-0004 ha unificato il modello:

```text
E5 pre-committed reaction
= caso degenere di opportunity→commit
= AllowedResponses <= 1
= nessuna interactive window
```

E14 non è un secondo sistema di reazioni.

Conservare i dettagli CP5.5/6.7, ma impedire al lettore di dedurre due motori paralleli.

---

## 2.20 `spec-sequenza-turno.md`

**Verdetto:** `REWRITE CRITICO`

Il documento si è aggiornato solo a metà.

### Contraddizione interna

In alto dice:

```text
finestre live in scope
ADR-0004 chiude C1
```

ma più avanti continua a dire:

```text
sistema reazioni completo north-star gated
non implementare finestre live nell'MVP
serve multiplayer
```

Le due cose non possono essere entrambe vere.

### Modello corrente da scrivere

La nuova spec deve diventare la **sequenza canonica del round**:

```text
Planning
→ Ready / Commit
→ Resolution
   Prep
   Dash
   Blast
   Move
   [Decision Boundaries possono interrompere un segmento,
    non aggiungono una macro-fase]
→ Cleanup
```

Separare nettamente:

**Corrente**
- single decision window;
- opportunity→commit;
- no nested stack;
- 3 s;
- timeout HOLD;
- segment snapshots.

**North-star**
- LIFO stack interattivo;
- nested interrupts;
- Patch;
- 5 speed categories;
- progressive reveal generico;
- 45–60s execution timeline.

### Ordine deterministico

Non lasciare due “ordini totali” non spiegati:
- Action Queue corrente;
- APNAP-adapted ordering storico.

Se APNAP resta, specificare esattamente **a quale tipo di eventi** si applica e come si compone con `MacroPhase → Priority → ActionId → SourceUnitId → EventSequence`.

---

## 2.21 `spec-stati-temporanei-cp82.md`

**Verdetto:** `KEEP + UPDATE link`

Spec chiusa e valida.

### Da aggiornare

- `Obscured` ora è un input reale di E13 / TeamKnowledge, non soltanto osservabilità futura.
- L'ordine definitivo del Cleanup deve rimandare alla spec ambiente più recente se CP8.4/8.5 lo ha esteso.
- Non duplicare l'ordine in due documenti come due fonti normative.

---

## 2.22 `spec-terreni-e8.md`

**Verdetto:** `UPDATE`

La base hex è corretta.

### Da aggiornare

- `Smoke`:
  - il cap offensivo esistente resta;
  - la semantica di **detection/contact** è posseduta da E13 + ADR-0005;
  - non duplicare la logica di percezione nel terreno.
- `HighGround`:
  - D-018: nessun `+VisionRange` piatto nella v0.1;
  - quota/Layer influenzano LOS, occlusione, cover e topologia;
  - un bonus numerico futuro richiede playtest e decisione esplicita.
- `Ice`: slide base vigente; motore avanzato resta nel brief ghiaccio.
- D-015: documentare `Sprint` come profilo `Move`; qualsiasi classificazione legacy come Dash va marcata come migrazione da pianificare.

---

## 2.23 `spec-terreni.md`

**Verdetto:** `ARCHIVE`

È il terreno quadrato storico.

Spostare sotto `docs/archive/gameplay/` o mantenerlo solo con redirect/header storico.

La spec corrente è `spec-terreni-e8.md`.

---

# 3. Cosa manca in `docs/gameplay/`

Questi sono i gap principali emersi dall'audit.

## 3.1 Una vera spec canonica del round

Oggi esistono:
- `sequenza-turno.md` esplorativo;
- `spec-sequenza-turno.md` parzialmente aggiornato;
- ADR-0004;
- brief delayed;
- brief Overwatch;
- Action Ghosts in `docs/technical`.

Manca **un singolo documento gameplay corto e normativo** che dica senza storia:

```text
Planning / Commit
Prep
Dash
Blast
Move
Cleanup

Phase boundary
Decision boundary
Normal Action
Delayed Action
Prepared Reaction
Fast Reaction
Fast Action
forced movement
```

Proposta:

```text
docs/gameplay/spec-round-canonico.md
```

oppure riscrivere `spec-sequenza-turno.md` con questo ruolo.

## 3.2 Predictive Actions / Traps / Tactical Gambits non sono ancora consolidate in gameplay

Esiste il sorgente:

```text
docs/src/RefactorTactics_Predictive_Actions_Traps_Claude.md
```

e `brief-delayed-actions.md` ne ha assorbito solo parte + D-013.

Manca una spec/brief che separi chiaramente:

```text
Delayed boundary action
Predictive triggered action
Persistent trap
Prepared reaction
Fast reaction
```

senza chiamare tutto “Reaction”.

## 3.3 Bot hex corrente

Manca una spec attiva equivalente a quella storica square che raccolga il comportamento reale di `URTHexBotLibrary` e le nuove premesse E13/E14/E16.

## 3.4 Action economy canonica aggiornata

Serve un posto unico che materializzi D-014/D-015.

Tassonomia gameplay:

```text
Generic actions:
  Wait
  BasicAttack
  Interact
  Brace
  Move
  Overwatch

Move profiles:
  Sneak
  Normal
  Sprint

Special pre-Blast movement:
  Dash / Charge / Leap / Blink / Reposition / forced displacement
```

Regole già chiuse:

```text
Attack OR Ability OR Overwatch
Activate -> Interact semantics
Guard -> non più fondamentale universale
Sprint != Dash
```

Restano **tunable**, non bloccanti:
- costi MP;
- rumore;
- exposure;
- eventuale costo extra di slot di Sprint;
- differenze di profilo per eroe.

Questi numeri devono vivere nei dati/cataloghi e non essere inventati durante il cleanup.

## 3.5 Glossario temporale unificato

Terminologia da fissare:

```text
Round
TurnNumber legacy code
Normal Action
Delayed Action
Predictive Action
Prepared Reaction
Fast Reaction
Fast Action
Phase Boundary
Decision Boundary
Playback
Simulation Time
Presentation Time
Decision Time
```

Soprattutto: **Delayed Action != Fast Action**.

## 3.6 Regola cover + Intercept

**Risolta da D-017.**

Quando `Intercept` cambia il bersaglio effettivo, la geometria target-dependent viene rivalidata sul nuovo bersaglio. Nessuna nuova Reaction Opportunity viene aperta dalla rivalidazione.

## 3.7 HighGround + vision

**Risolta da D-018.**

Nella v0.1 `HighGround` non applica un bonus piatto a `VisionRange`.

La quota incide attraverso:
- geometria;
- LOS;
- occlusione;
- cover;
- topologia/layer.

Eventuali bonus numerici futuri richiedono playtest e una nuova decisione.

## 3.8 Stato di implementation nei documenti

Molti file sono “spec di checkpoint” e riportano numeri/test/stato del giorno in cui sono stati chiusi.

Non devono competere con la roadmap come fonte di stato.

Aggiungere un banner comune:

```text
Implementation status is historical as of <date>.
Current state is owned by docs/roadmap/roadmap-v0.1.md.
```

---

# 4. Decisioni approvate il 2026-08-08

Queste decisioni **non sono più bloccanti**. Claude deve registrarle nel Decision Log con gli ID indicati, verificando che non esistano già ID occupati o decisioni equivalenti. Se la numerazione reale del repository è avanzata, preservare il contenuto e usare i prossimi ID disponibili, riportando la mappatura nel changelog.

## D-014 — Azioni generiche canoniche

Azioni universali:

```text
Wait
BasicAttack
Interact
Brace
Move
Overwatch
```

Regole:

- `Activate` viene assorbita semanticamente da `Interact`.
- `Guard` non è una fondamentale universale; può restare come stance/capacità specifica dove richiesta.
- Overwatch è universale come framework ma il profilo dipende dall'eroe.
- economia standard: `Attack OR Ability OR Overwatch`, salvo eccezione esplicita.

### Migrazione

Gli Stable ID legacy non vanno cancellati o rinominati brutalmente. Servono deprecation/redirect/versioning o issue di migrazione coerenti con il sistema dati realmente presente.

---

## D-015 — `Sneak / Normal / Sprint` sono profili di `Move`

```text
MoveProfile.Sneak
MoveProfile.Normal
MoveProfile.Sprint
```

- Tutti appartengono alla famiglia `Move`.
- `Sprint` **non è Dash**.
- Dash/Charge/Leap/Blink/Reposition restano mobilità speciali pre-Blast.
- Il normale Move continua a essere l'ultima fase volontaria standard.
- Rumore, distanza, exposure e costi sono data-driven.

### Non ancora bloccato come numero

L'eventuale costo di Sprint in termini di slot aggiuntivi resta tuning da playtest e non deve essere inventato nel cleanup.

---

## D-016 — Thin slice Predictive Action nella v0.1

La v0.1 include **una** Predictive Action reale, preferibilmente `Hero.Wraith.InterceptShot`.

Principio:

```text
decisione completa in Planning
→ trigger/boundary deterministico
→ risoluzione automatica se la previsione è corretta
→ whiff/fallback se è errata
→ nessun input umano live
```

Separazione obbligatoria:

```text
Delayed/Predictive Action != Fast Action
Predictive Action != Fast Reaction
Persistent Trap != Fast Reaction
```

L'intero framework di trap/tactical gambit resta fuori dalla v0.1.

---

## D-017 — Intercept rivalida la geometria sul bersaglio effettivo

Se A viene sostituito da B come bersaglio:

```text
Target = B
→ recalc target-dependent geometry
→ LOS / trajectory / cover / facing / geometric defense against B
```

Non si rivalutano arbitrariamente dati indipendenti dal target.

La rivalidazione non apre una seconda reaction e non consente nested reaction loops.

---

## D-018 — HighGround senza bonus numerico piatto alla vista in v0.1

Nessun:

```text
HighGround => +1 VisionRange
```

di default.

La quota ha già valore attraverso geometria, LOS, cover, occlusione e layer.

Un bonus numerico futuro richiede playtest e decisione separata.

---

## D-019 — Fast Action e Fast Reaction sono categorie semantiche distinte

Entrambe possono usare la stessa infrastruttura tecnica `DecisionWindow`, ma:

```text
Fast Action
= scelta live limitata come continuazione esplicita di una propria azione

Fast Reaction
= scelta live provocata da un evento/trigger esterno
```

Una Fast Action non è una Delayed Action.

La v0.1 non deve inventare una Fast Action concreta se nessuna ability reale la richiede.

Baseline comune iniziale per eventuali decision window live:

```text
3.0 s
```

salvo futura eccezione ability-specific esplicita.

---

## 4.1 Modello temporale risultante

```text
PLANNING
  ├─ azione principale/generica
  ├─ profilo Move
  ├─ eventuale Predictive Action completamente precommitted
  └─ eventuale Reaction armata

COMMIT

RESOLUTION
  Prep
  Dash
  Blast
  Move

Durante un segmento:
  Predictive trigger
    → automatic resolve, no player input

  Reaction Opportunity
    → Fast Reaction solo se AllowedResponses richiede scelta live

  Own-action Decision Boundary
    → Fast Action solo se la definition lo prevede

CLEANUP
```

Questo schema deve diventare il riferimento per `spec-sequenza-turno.md`.

---

# 5. Piano esecutivo di consolidamento

Non esistono più blocker noti fra le sei decisioni discusse. Claude può procedere, ma deve distinguere:

```text
A. correzione documentale sicura
B. registrazione decisioni
C. migrazione dati/codice
D. nuove feature/issue
```

## 5.1 Correzione documentale sicura

1. marcare/spostare gli storici square:
   - `spec-bot-utility.md`;
   - `spec-knockback.md`;
   - `spec-terreni.md`;
   - `sequenza-turno.md` esplorativo.
2. correggere la contraddizione interna di `spec-dash.md`.
3. aggiornare `spec-reazioni-componibili-cp55.md` con il modello unificato ADR-0004.
4. aggiornare `brief-overwatch-reazioni.md` con D-012, D-014, D-019 e ADR-0005.
5. aggiornare `brief-conoscenza-parziale.md` rimuovendo il vecchio veto D15, collegando ADR-0005 e applicando D-018.
6. correggere `spec-durata-partita-e-scala-mappe.md`:
   - 3v3 = baseline, non formato deciso;
   - 4v4 = stress;
   - Delayed/Predictive Action != Fast Action.
7. riscrivere `spec-anima-risoluzione.md` a segmenti/Decision Boundary.
8. riscrivere `spec-sequenza-turno.md` come spec canonica del round.
9. aggiungere banner “implementation status historical; roadmap owns current state” alle spec di CP chiusi.
10. aggiornare `spec-terreni-e8.md` con D-015/D-018.
11. aggiornare `spec-copertura-cp91.md` con D-017.

## 5.2 Decision Log e governance

Registrare D-014…D-019, usando i prossimi ID liberi se necessario.

Aggiornare:
- changelog documentale;
- conflict matrix se presente;
- cross-link da ADR/spec interessate.

Non riscrivere la storia: le vecchie decisioni restano visibili come superate/deprecate quando utile.

## 5.3 Migrazione action economy / catalogo

Verificare il codice e i cataloghi correnti per:

```text
Action.Guard
Action.Activate
Action.Sprint
```

Target semantico:

```text
Guard -> specific/non-universal capability
Activate -> Interact
Sprint -> Move profile
```

Se la migrazione è invasiva:
- non forzarla in una PR documentale;
- creare/aggiornare issue con DoD e test;
- aggiungerla alla roadmap nella posizione coerente con dipendenze reali.

Requisiti:
- Stable ID/replay safety;
- validator aggiornato;
- test di migrazione o redirect se esiste serializzazione persistente;
- nessuna doppia verità runtime.

## 5.4 Predictive Action thin slice

Integrare D-016 nella roadmap.

Claude deve verificare `Hero.Wraith.InterceptShot` nel catalogo/codice attuale e proporre la minima migrazione necessaria affinché sia una Predictive Action precommitted.

Acceptance minima:

```text
Planning chooses prediction
Commit stores canonical intent
Resolver evaluates pure trigger/boundary
No human input during resolution
Whiff/fallback logged
Deterministic replay
```

Non introdurre il framework completo di trap.

## 5.5 Intercept geometry

Applicare D-017 a documentazione e test plan.

Se il codice corrente conserva cover del target originale:
- creare fix issue o implementare il fix solo se la dipendenza E5/E9 è pronta;
- aggiungere un test discriminante in cui A e B hanno geometrie di cover diverse.

## 5.6 Fast Action

Applicare D-019 al glossario e alla spec temporale.

Non creare classi/runtime dedicate solo perché il termine esiste. Riutilizzare `DecisionWindow` quando una feature concreta ne avrà bisogno.

# 6. Requisiti per la PR di Claude

La PR documentale deve includere:

```text
docs/gameplay/...
docs/decisions/RT_PDR_00_Decision_Log.md   # D-014…D-019 / prossimi ID liberi
docs/CHANGELOG_DOCUMENTATION.md
docs/DOC_CONFLICT_MATRIX.md                # se il repo lo usa ancora
docs/roadmap/...                            # solo se cambia scope/stato
```

Non modificare codice solo per farlo combaciare con un documento stale.

Se il codice è in conflitto con una **decisione approvata più recente**, creare/aggiornare issue con:
- comportamento attuale;
- comportamento desiderato;
- test da cambiare/agggiungere;
- dipendenze;
- rischio;
- DoD.

---

# 7. Guardrail — cose da NON reintrodurre

Durante il consolidamento Claude non deve reintrodurre:

```text
griglia quadrata
Move prima del Blast
timeline libera Move→Attack→Move
stack LIFO interattivo annidato come modello v0.1
5 categorie di velocità come requisito corrente
Patch dinamiche come feature corrente
Fast Reaction da 5–8 secondi come baseline
Timeout che consuma FIRE
enemy intent replicati e poi “nascosti” in UI
JSON come fonte primaria delle ability v0.1
GAS come autorità del resolver
3v3 come formato finale già deciso
4v4 come formato finale già deciso
Aegis/Nyx/Drift/Vex come roster operativo
Sprint trattato come Dash nel nuovo canone
Activate come azione universale distinta da Interact
Guard come azione fondamentale universale
Predictive Action trasformata automaticamente in Fast Reaction
HighGround con bonus numerico di vista inventato
```

---

# 8. Test documentale finale

Prima di chiudere il consolidamento, cercare nel repository almeno questi pattern e classificare ogni hit:

```text
"5 seconds" / "5 s" vicino a Reaction
"45–60"
"3v3 principal"
"main format"
"Fast Action"
"Reaction Stack"
"LIFO"
"Move -> Attack"
"Move → Attack"
"Aegis"
"Drift"
"FRTGridCoord"
"URTGridLibrary"
"bDash"
"pathfinding" vicino a "Dash"
"Direction" vicino a "Overwatch"
"Action.Sprint"
"Action.Activate"
"Action.Guard"
"InterceptShot"
"HighGround" vicino a "VisionRange"
```

Per ogni hit:

```text
CURRENT
HISTORICAL
NORTH-STAR
WRONG / UPDATE REQUIRED
```

Nessun hit ambiguo deve restare senza etichetta.

---

# 9. Deliverable richiesto a Claude Code

Produrre alla fine:

```text
1. Audit summary
2. Files changed
3. Files archived/moved
4. Decisions preserved
5. Decisions changed
6. New conflicts/open questions discovered (expected: none of D-014…D-019)
7. Roadmap/issues changed
8. Code changes, if any, with justification
9. Tests/document checks executed
10. Remaining contradictions
```

Obiettivo finale:

> un nuovo sviluppatore o una nuova sessione Claude deve poter leggere `docs/gameplay/` senza imparare per errore una regola superata.

## 9.1 Exit criteria del consolidamento

Il task è chiuso solo quando:

- le sei decisioni D-014…D-019 sono registrate;
- nessun documento gameplay attivo insegna `Sprint = Dash`;
- `Activate` non è più presentata come fondamentale distinta da `Interact`;
- `Guard` non è più presentata come fondamentale universale;
- Predictive/Delayed/Fast Action/Fast Reaction hanno definizioni non sovrapposte;
- `spec-sequenza-turno.md` è la spec canonica del round a segmenti;
- le spec square sono archiviate o inequivocabilmente storiche;
- 3v3 e 4v4 non sono presentati come formato finale deciso;
- HighGround non riceve un bonus numerico di vista inventato;
- il limite Intercept+cover è sostituito dalla regola di rivalidazione sul bersaglio effettivo;
- ogni divergenza codice↔decisione nuova ha una issue/roadmap entry o una fix testata;
- non restano blocker noti derivanti dalle sei decisioni chiuse il 2026-08-08.

