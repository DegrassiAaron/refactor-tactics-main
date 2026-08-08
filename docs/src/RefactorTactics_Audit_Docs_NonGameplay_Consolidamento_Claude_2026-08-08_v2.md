# RefactorTactics — Audit completo di `docs/` non-Gameplay e piano di consolidamento per Claude Code — DECISIONI CHIUSE

**Data audit:** 2026-08-08  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Branch verificato:** `main`  
**HEAD verificato:** `fe30e4e647cddaac16c3d7079a112193245dc5cd`  
**Scope:** tutti i documenti sotto `docs/`, **esclusi**:
- `docs/gameplay/`
- `docs/src/`
- `docs/archive/`

> Questo file è un **handoff di audit + prompt operativo**.  
> Non deve essere considerato una nuova fonte canonica di gameplay.  
> Le 10 decisioni di design emerse dall'audit sono state **chiuse dall'autore il 2026-08-08** e sono integrate in questo file. Claude deve applicarle come direttive di consolidamento, dopo aver verificato il codice/repository corrente. Non inventare numeri di bilanciamento non ancora definiti.

---

# 1. Obiettivo

La repository è già stata riorganizzata e la situazione è molto migliore rispetto ai vecchi PDR, ma il secondo audit mostra un problema diverso:

> la struttura documentale è buona, mentre diversi documenti all'interno della nuova struttura contengono ancora **stato storico, snapshot intermedi, correzioni stratificate o assunzioni precedenti**.

Il risultato è che oggi un lettore può trovare nello stesso `docs/`:

- una regola canonica corretta;
- una vecchia spec tecnicamente superata;
- una spec "as-built" che descrive correttamente il 5 agosto ma non lo stato di oggi;
- una roadmap con una tabella aggiornata in alto e una tabella vecchia più sotto;
- un prompt di implementazione ancora posizionato come se fosse una specifica tecnica corrente;
- valori di bilanciamento nel workbook che non corrispondono più al roster e al modello di turno correnti.

L'obiettivo di questa attività è quindi:

1. correggere la **deriva interna** dei documenti non-Gameplay;
2. separare chiaramente:
   - **CANONICAL / CURRENT**
   - **AS-BUILT / DELIVERED**
   - **HISTORICAL**
   - **RESEARCH**
   - **OPEN**
3. evitare che documenti vecchi appaiano ancora operativi;
4. rimuovere duplicazioni di stato;
5. fare in modo che ogni dato volatile venga **misurato o linkato**, non copiato in 5 file;
6. recepire nel canone le decisioni già accettate;
7. applicare le decisioni chiuse nella sezione **6**;
8. non modificare `docs/gameplay/` durante questa passata, salvo creare issue/link di follow-up quando una decisione richiede anche un aggiornamento Gameplay.

---

# 2. Stato del repository assunto come baseline

Prima di modificare documentazione, Claude deve **rimisurare** e verificare questi dati.

Al momento dell'audit:

- UE: **5.8.1** di fatto bloccata nel repository;
- topologia gameplay: **solo hex**;
- `FRTCellId`: coordinate assiali hex + `Layer`;
- vecchio substrato quadrato: rimosso;
- roster dati corrente: **Flux · Riva · Bastion · Vektor**;
- E4 motore azioni: implementato;
- E5 reazioni core: implementato;
- E6 roster: implementato;
- E8 ambiente: implementato;
- E9:
  - CP 9.1 copertura bassa: implementato;
  - CP 9.2 copertura alta/distruzione/danno struttura: implementato;
  - porte/ponti/coperture temporanee: ancora da completare;
- CP 10.3 fine partita / `RoundLimit`: implementato;
- networking: fuori dalla v0.1 corrente;
- GAS: fuori dalla v0.1 corrente;
- Scenario Test Harness:
  - JSON versionati sotto `Scenarios/`;
  - percorso gameplay reale;
  - `result.json`;
  - PASS / FAIL / ERROR;
  - console `rt.Test.*`;
  - auto-run tramite CVar/GameMode;
  - **nessun `ARTTestDirector` necessario** nella prima implementazione;
  - scenari movimento attuali: `Movement.Basic`, `Movement.BasicFailsOnPurpose`, `Movement.Blocked`, `Movement.Collision`;
- suite automation misurata dopo l'ultimo merge: **415 test unici in 63 file**.

Questi numeri NON devono essere copiati ciecamente: usare il comando di misura presente nel repo dopo avere sincronizzato il branch.

---

# 3. Decisioni recenti che devono guidare l'audit

## 3.1 Turno

Il modello corrente è:

```text
Planning
  ↓
Prep
  ↓
Dash
  ↓
Blast
  ↓
Move
  ↓
Cleanup
```

Il normale `Move` è sempre l'ultima fase volontaria.

Le reazioni NON sono una quinta macro-fase.

---

## 3.2 Decision Boundary

ADR-0004 ha cambiato una premessa importante.

Il modello non è più necessariamente:

```text
Commit
→ calcola tutto il turno
→ riproduci tutto
```

È invece:

```text
Commit
→ Segmento deterministico
→ eventuale Decision Boundary
→ risposta autorizzata
→ nuovo Segmento deterministico
→ ...
→ fine round
```

All'interno del singolo segmento resta la disciplina:

```text
snapshot
→ collect
→ order
→ apply
→ log
```

La simulazione non aspetta dentro un resolver segment; l'attesa vive tra segmenti.

---

## 3.3 Fast Action e Fast Reaction

Condividono l'infrastruttura di Decision Window ma hanno semantica diversa.

**Fast Reaction**:
- trigger esterno;
- esempio: Overwatch;
- baseline: **3,0 s**;
- timeout conservativo, normalmente `HOLD`.

**Fast Action**:
- continuazione live limitata di una propria azione;
- NON è sinonimo di Delayed Action;
- non va implementata finché non esiste un caso concreto che la richieda.

**Delayed/Predictive Action**:
- completamente definita durante Planning;
- può risolversi più tardi;
- non offre una nuova scelta solo perché è ritardata.

---

## 3.4 Overwatch

Decisione già registrata:

- universale;
- preparata in Planning;
- non skill esclusiva di un eroe;
- **compete con l'azione offensiva principale**;
- profilo/effetto può cambiare per eroe/equipaggiamento;
- si basa sul modello generale `Opportunity → Response/Commit`;
- nessun leak dei trigger futuri;
- trigger simultanei nello stesso micro-step raccolti in una singola opportunity;
- nessun interrupt annidato nell'MVP.

---

## 3.5 Facing

ADR-0005 rende il facing **stato gameplay**, non semplice presentazione.

Influenza:
- difesa;
- percezione;
- Overwatch/reaction directional cone.

Resta una questione da chiarire sul **timing** del facing dopo Dash e prima del Blast; vedi Human Decisions.

---

## 3.6 Percezione

Non usare più "Fog of War" come scorciatoia se suggerisce una mappa nascosta classica.

Il modello corrente è **conoscenza parziale**:

```text
LOS geometrica
+
Detection / awareness
+
rumore
+
memoria dell'ultimo contatto
=
Team Knowledge
```

La mappa statica può essere conosciuta; ciò che è incompleto è soprattutto l'informazione sulle unità/eventi.

Il facing aggiunge:
- cono frontale;
- consapevolezza locale 360° entro una distanza limitata.

Il rumore:
- è un secondo canale percettivo;
- si propaga sul grafo;
- usa valori interi;
- niente hidden RNG nel sistema base.

---

## 3.7 Formato partita

Non documentare il **3v3 come formato definitivamente scelto**.

Stato corretto:

- 2v2 = vertical slice corrente;
- 3v3 = baseline di lavoro / ipotesi principale;
- 4v4 = scenario di stress E17;
- formato prodotto definitivo = da validare con misure.

---

# 4. Problemi trasversali trovati

## P0 — Il canone non ha ancora assorbito tutte le decisioni che dice di sovrastare

`docs/README.md` afferma che `product/piano-canonico-mvp.md` prevale su tutto.

Ma ADR-0004 e ADR-0005 hanno modificato parti sostanziali del modello:

- invariant collect/apply → deve essere espresso **per segmento**;
- facing è ora gameplay;
- Decision Window cambia la forma della resolution.

Questo crea un paradosso di governance:

```text
Canone > ADR
```

ma un ADR accettato può correggere il canone.

### Correzione consigliata

Una delle due:

**A — preferita**

Ogni ADR accettato viene immediatamente recepito nel canone.  
Il canone resta sempre davvero al livello 1.

oppure:

**B**

La gerarchia dichiara:

```text
Canone
+ emendamenti ADR accettati non ancora recepiti
```

La soluzione A è più semplice.

---

## P0 — Le roadmap mantengono più snapshot "correnti" nello stesso file

Diversi documenti sono cresciuti per append successive:

```text
stato vecchio
↓
nota di correzione
↓
stato nuovo
↓
altra correzione
```

Questo conserva la storia, ma rende difficile capire cosa vale adesso.

La storia deve vivere:
- nel Git history;
- nel changelog;
- in sezioni esplicitamente storiche.

La sezione "stato corrente" deve avere **una sola verità**.

---

## P0 — Il workbook di bilanciamento non è più coerente col canone

`RefactorTactics_Balance_Matrices_v0.1.xlsx` contiene ancora una grossa quantità di ricerca precedente.

Esempi:

- roster Paragon / Steel / Aurora / Murdock / Kwang;
- Fast Reaction da 5–7 s;
- valore generale `FastReaction_sec = 6`;
- planning 35/35/40;
- `Max_Turni = 12`;
- nota percezione "360° base; coni solo Overwatch/sensori";
- action model precedente.

Non è semplicemente "qualche cella vecchia".

È un **workbook di esplorazione** che oggi rischia di apparire come balance source corrente.

Decisione necessaria:
- archiviarlo/classificarlo come `RESEARCH`;
- oppure produrre un nuovo workbook v0.2 realmente derivato dai cataloghi Markdown correnti.

---

## P1 — Le specifiche tecniche "as-built" non sono chiaramente separate dalle specifiche correnti

Esempi:
- H6 hex sim;
- H6 vision;
- H6 bot;
- vecchie spec pathfinding;
- piano TurnLog.

Sono utilissime come storia tecnica.

Non devono però sembrare il punto in cui modificare la regola oggi.

---

## P1 — Troppo stato volatile dentro documenti normativi

I conteggi test sono il caso più evidente.

La repository ha già imparato la lezione quattro volte.

Regola proposta:

> Nei documenti normativi preferire il **comando che misura** al numero statico.  
> Se il numero serve, scriverlo in UNA vista di stato e indicare commit/data.

---

# 5. Audit documento per documento

Legenda:

- **KEEP** — corretto; solo manutenzione minima.
- **UPDATE** — struttura valida, contenuto corrente da riallineare.
- **REWRITE** — il file ha ancora un ruolo utile, ma la maggior parte della narrativa corrente è superata.
- **AS-BUILT** — congelare come specifica di ciò che fu implementato in un checkpoint.
- **HISTORICAL** — materiale storico; non aggiornare il corpo come se fosse corrente.
- **MOVE/ARCHIVE** — non dovrebbe stare fra i documenti correnti.
- **BLOCKED** — non correggere il punto ambiguo finché l'autore non decide.

---

# 5A. Root `docs/`

| Documento | Azione | Audit |
|---|---|---|
| `docs/README.md` | **UPDATE** | Ottimo indice e buona regola "un concetto, un owner". Correggere la gerarchia Canone/ADR come descritto sopra. Verificare che il test count sia misurato. Esporre chiaramente Decision Boundary, E13–E17 e lo stato attuale della v0.1. |
| `docs/DOC_CONFLICT_MATRIX.md` | **UPDATE** | Non può dichiarare "0 OPEN / 0 CONFLICT" finché esistono le decisioni elencate nella sezione Human Decisions. La riga Scenario Harness è indietro. La riga Overwatch deve distinguere "decisione confermata" da "propagazione non ancora completa nei cataloghi". |
| `docs/OPEN_DECISIONS.md` | **UPDATE** | Buona struttura. Aggiungere i dubbi reali emersi nella revisione turno/action economy. Rivedere D-007: UE 5.8.1 è di fatto bloccata; decidere se consolidarla formalmente. |
| `docs/CHANGELOG_DOCUMENTATION.md` | **KEEP + APPEND** | Non riscrivere la storia. Dopo il consolidamento aggiungere una nuova entry 2026-08-08 con file corretti, decisioni recepite e documenti riclassificati. |
| `docs/brief-consolidamento-documentale.md` | **HISTORICAL / MOVE** | È il verbale della revisione precedente. Utile come provenance, ma al root sembra un documento operativo. Spostare in `roadmap/plans/` o aggiungere banner molto evidente `REVIEW CONCLUSA — NON NORMATIVO`. |

---

# 5B. `docs/product/`

## `product/piano-canonico-mvp.md`

**Azione: REWRITE mirata / P0**

È il file più importante da riallineare.

Deve recepire:

1. ADR-0004:
   - collect/apply per **segmento deterministico**;
   - Decision Boundary tra segmenti;
   - nessuna attesa dentro un resolver segment;
2. ADR-0005:
   - facing come stato gameplay;
3. D-012:
   - Overwatch universale;
4. D-013:
   - trigger di transizione posseduto dalla reaction/trap, non dalla mappa;
5. conoscenza parziale:
   - Team Knowledge;
   - vista + udito;
6. formato:
   - 3v3 baseline, non decisione definitiva;
7. Scenario Harness:
   - test passa dalla stessa pipeline gameplay;
8. stato UE:
   - 5.8.1;
9. no-GAS v0.1 resta valido.

Non caricare il canone con lo stato di ogni checkpoint.

Il canone deve descrivere:
- identità;
- invarianti;
- scope;
- decisioni operative.

Lo stato vive nella roadmap.

---

## `product/showcase-v0.1.md`

**Azione: UPDATE forte / P0**

Il documento è stato scritto contro uno snapshot precedente.

Aggiornare il delta "esiste/non esiste" usando il repository attuale:

- roster E6 fatto;
- E8 fatto;
- low cover fatta;
- high cover fatta;
- tre reaction d'eroe cablate;
- due reaction rinviate a E14;
- Scenario Harness disponibile;
- 415 test alla baseline dell'audit.

La showcase dovrebbe diventare anche uno **Scenario Harness scenario** o una famiglia di scenari, non una seconda pipeline di test.

### Punto BLOCKED

Valutare se la showcase deve includere **una vera predictive action di Vektor** per far vedere il pilastro "scommessa sul movimento".

Non implementare l'intero framework traps solo per la showcase.

---

# 5C. `docs/decisions/`

## `decisions/RT_PDR_00_Decision_Log.md`

**Azione: UPDATE**

- mantenere D-001/D-011;
- mantenere D-012/D-013;
- correggere eventuali link rimasti a `../design/...`;
- aggiungere nuove `D-014+` solo dopo chiusura delle Human Decisions di questo audit;
- non trasformare automaticamente i dubbi in decisioni.

---

## `decisions/adr-0001-skeletal-unit.md`

**Azione: UPDATE STATUS**

La decisione architetturale rimane sensata:
- unità logica indipendente dalla rappresentazione;
- skeletal presentation con fallback.

Ma lo status operativo non deve far pensare che il roster corrente abbia già una mapping visuale definitiva.

Usare qualcosa come:

```text
Status: Accepted — presentation prototype / rollout deferred
```

Non rinominare automaticamente Gideon/Sparrow in Flux/Riva/Bastion/Vektor: la mapping Paragon → eroe non è stata decisa.

---

## `decisions/adr-0002-griglia-esagonale.md`

**Azione: UPDATE STATUS**

Non è più "in implementazione".

La decisione è ormai realizzata:
- hex unica topologia gameplay;
- quadrato rimosso;
- multilayer attivo.

Aggiungere:
- data di completamento;
- tag di rollback storico se utile;
- link alle spec as-built.

---

## `decisions/adr-0003-modello-azioni-v01.md`

**Azione: UPDATE / AMENDMENT P0**

Il file è importante ma rischia di essere letto senza ADR-0004.

Aggiungere in testa un blocco:

```text
AMENDED BY ADR-0004
```

Spiegare:

- la macro-sequenza `Prep → Dash → Blast → Move` resta;
- il vecchio rifiuto delle finestre live è stato superato dal modello segmentato;
- E5 legacy = caso semplice del modello unificato di reaction;
- Overwatch universale è un'azione principale che arma una reaction;
- non confondere "slot reaction preparata" con "costo di selezione Overwatch";
- `Sneak`, `Move`, `Sprint` sono profili del **movement slot** e risolvono nel `Move`;
- `Dash`, `Leap`, `Charge`, `Reposition` restano movimenti speciali della fase `Dash`.

---

## `decisions/adr-0004-finestre-di-reazione.md`

**Azione: UPDATE P0**

Architettura sostanzialmente corretta.

Chiarire:

1. `Detected/Rilevato` non deve diventare requisito universale di **ogni** reaction:
   - Overwatch visiva sì;
   - una reaction acustica può essere legale con Team Knowledge derivata dal rumore.

2. **Privacy temporale — decisione chiusa**:
   - una Decision Window privata **non deve essere rivelata agli avversari**;
   - i personaggi continuano a essere percepiti come agenti che agiscono in contemporanea;
   - il client avversario non riceve metadata della finestra, del trigger, delle opzioni o del giocatore autorizzato;
   - evitare un freeze/pause osservabile che permetta di dedurre l'esistenza della finestra;
   - la simulazione logica può attendere la risposta al boundary, ma la presentazione/networking deve mascherare il tempo di decisione tramite buffering/pacing/fixed beat o meccanismo equivalente;
   - questa proprietà va trattata come **privacy requirement**, non solo come polish UI;
   - aggiungere test/canary per packet privacy e, dove possibile, per timing/presentation leakage.

3. Nessuna reaction annidata nell'MVP.

---

## `decisions/adr-0005-orientamento.md`

**Azione: UPDATE P0**

Il facing come stato gameplay è corretto.

Decisione chiusa:

> quando un'azione ha un target/direzione, il personaggio **si rivolge verso il target/direzione dell'azione prima che l'azione risolva**.

Formalizzare almeno:

```text
FacingStartOfRound
FacingAfterPrepActionTargeting
FacingAfterDash
FacingUsedByBlast
FacingUsedByOverwatch
FacingFinalAfterMove
```

Regole:

- un Dash orientato cambia il facing verso la direzione/target del Dash;
- un attacco/ability con target orienta l'unità verso quel target prima della risoluzione dell'azione;
- Overwatch usa la direzione/cone pianificata come facing autorevole della reaction;
- una reaction successiva usa il facing autorevole più recente;
- il normale `Move`, che risolve per ultimo, aggiorna il facing finale secondo l'ultima direzione percorsa o la direzione finale esplicitamente pianificata se il sistema la supporta;
- il facing finale persiste nel turno successivo finché una nuova azione lo cambia.

Aggiungere test di sequenza Dash → Blast e Overwatch directional.

---

# 5D. `docs/balance/`

## `balance/README.md`

**Azione: REWRITE**

La parte iniziale sullo "stato attuale" è vecchia:
- non siamo più a due archetipi;
- superfici attive esistono;
- cover low/high esistono.

Un README di balance non dovrebbe duplicare lo stato della roadmap.

Riscrivere come:
- quali cataloghi sono normativi;
- quali numeri sono vigenti;
- come cambiare un valore;
- come collegare un valore a test/playtest;
- workbook: stato esplicito.

---

## `balance/RT_ActionCatalog_v0.1.md`

**Azione: UPDATE P0**

Il catalogo deve assorbire definitivamente D-012 e le decisioni chiuse di questo audit.

Le azioni generiche da mantenere sono:

```text
Wait
Move
BasicAttack
Guard
Brace
Activate
Interact
Overwatch
```

`Overwatch` è **universale** e compete con l'azione principale offensiva:

```text
Main action:
Attack OR Ability OR Overwatch
```

Lo slot/preparazione di reaction dell'eroe o dell'equipaggiamento resta un concetto distinto.

### Nuovo modello del movimento per la v0.1

`Sneak`, `Move` e `Sprint` diventano **profili/modalità della stessa famiglia di movimento normale**, non tre economie d'azione incompatibili.

Vincoli:

- tutti e tre appartengono al **movement slot**;
- il movimento normale continua a risolvere nella macro-fase **Move**, quindi dopo `Blast`;
- `Dash`, `Leap`, `Charge`, `Reposition` restano movimenti **speciali** della fase `Dash`;
- non trasformare un normale `Sprint` in una `Dash`;
- mantenere i valori già canonici dove esistono (`Move` 5 MP, `Sprint` 8 MP) salvo conflitto verificato;
- per `Sneak` NON inventare MP, rumore o altri numeri mancanti: introdurre l'identità/modello e aprire un balance follow-up se il valore non è già definito da una fonte corrente;
- `Sprint` deve conservare un trade-off reale; migrare i drawback correnti in modo coerente col nuovo modello invece di trasformarlo in upgrade puro.

### `SuppressiveLine` vs Overwatch

Non fondere automaticamente le due identità.

Regola per il consolidamento:
- `Overwatch` = azione universale / infrastruttura di controllo reattivo;
- `SuppressiveLine` può restare una azione/profilo specifico con effetti propri se il catalogo la usa come contenuto distinto.

Se il codice dimostra che `SuppressiveLine` è solo una duplicazione nominale, creare issue di refactor invece di cancellarla durante il doc cleanup.

---

## `balance/RT_EquipmentCatalog_v0.1.md`

**Azione: UPDATE**

Buona struttura orizzontale.

Aggiornare:

- reaction modules rispetto al modello ADR-0004:
  - automatici = `AllowedResponses <= 1`;
  - interattivi = Decision Window;
- `Gadget.Sensor` deve parlare il linguaggio E13:
  - cambia Team Knowledge / detection;
  - non "rivela tutto" genericamente;
- `PortableCover` deve linkare il modello E9;
- distinguere chiaramente elementi già implementati da E7 ancora assente.

---

## `balance/RT_HeroCatalog_v0.1.md`

**Azione: UPDATE forte**

Roster corretto:
- Flux
- Riva
- Bastion
- Vektor.

Da correggere:

- status implementazione E8/E9;
- sezione percezione.

La frase:

```text
gli altri parametri di percezione non entrano nella v0.1
lo slice è binario
```

non è più compatibile con:
- E13;
- ADR-0005;
- rumore;
- Team Knowledge a livelli.

Aggiungere la relazione con Overwatch universale: ogni eroe può avere un **profilo/effetto Overwatch differente**, ma `Overwatch` non diventa una skill esclusiva.

Non inventare numeri o quattro profili se non esistono già nel codice o nelle fonti correnti.

### High Ground specifico per personaggio

La quota **non aumenta genericamente il danno**.

Un personaggio, tratto, abilità, equipaggiamento o specializzazione può però dichiarare un bonus legato all'altura.

Quindi:
- rimuovere dal modello eroe qualsiasi assunzione che "tutti fanno più danno dall'alto";
- supportare la possibilità data-driven di un modificatore specifico.

---

## `balance/RT_TerrainCatalog_v0.1.md`

**Azione: UPDATE P0**

Aggiornare lo stato:
- E8 completata;
- terrain dinamico presente;
- fire/water implementati;
- cover low/high implementate.

Integrare:
- Smoke/Obscured con E13;
- ice con rumore futuro;
- cover edge model corrente.

### High Ground — decisione chiusa

La quota di base fornisce **vantaggio geometrico**, non un bonus numerico universale al danno.

Regola v0.1:

1. l'altura modifica geometria, LOS, copertura e accessibilità;
2. **nessun `+Damage` generico** solo perché l'unità è più in alto;
3. **nessun `+VisionRange` generico** finché il playtest non lo giustifica;
4. un eroe/trait/ability/equipment può dichiarare esplicitamente un bonus da altura.

Verificare il codice corrente:
- se `OccupantDamageBonus` è usato come bonus universale di High Ground, rimuovere quella semantica dal terrain;
- non eliminare necessariamente il campo se è un meccanismo generico usato da altri effetti: rinominarlo/ricontestualizzarlo solo se serve e con test;
- aggiornare `EffectiveAttackPower` e i test solo se la verifica dimostra che l'altura base applica oggi il bonus.

---

## `balance/RT_TestMatrix_v0.1.md`

**Azione: REWRITE**

Molte righe "stato oggi" sono vecchie:
- E4;
- E5;
- E8;
- E9;
- comandi debug.

La matrice dovrebbe diventare principalmente:
- requisito → test;
- test type;
- acceptance result.

Lo stato dinamico dovrebbe essere derivato dalla suite / roadmap, non raccontato a mano.

Aggiungere coverage per:
- Decision Boundary;
- Overwatch HOLD/FIRE;
- simultaneous opportunities;
- privacy;
- facing;
- Team Knowledge;
- noise;
- Scenario Harness no-bypass;
- cover high/low;
- structure destruction.

---

## `balance/RefactorTactics_Balance_Matrices_v0.1.xlsx`

**Azione: RECLASSIFY AS RESEARCH**

Decisione chiusa:

```text
RefactorTactics_Balance_Matrices_v0.1.xlsx
= RESEARCH / BALANCE EXPLORATION

docs/balance/RT_*Catalog_v0.1.md
= canonical balance data
```

Divergenze individuate nel workbook:

- `02_Roster`: banca archetipi Paragon;
- `06_Abilita_VS`: Steel/Aurora/Murdock/Kwang;
- `07_Fast_Reactions`: 5–7 secondi;
- `15_Azioni_Turni`: `FastReaction_sec = 6`;
- planning 35/35/40;
- `Max_Turni = 12`;
- note percezione 360° incompatibili con ADR-0005.

Non correggere il workbook v0.1 cella per cella per trasformarlo in una falsa fonte corrente.

Azioni:
- aggiungere un README/banner/nota di classificazione dove il repo lo presenta;
- rimuovere link che lo trattano come source of truth;
- quando servirà una matrice operativa, creare `v0.2` da cataloghi canonici con processo di generazione/validazione che riduca la deriva.

---

# 5E. `docs/technical/`

## `technical/architettura-codice.md`

**Azione: REWRITE / P0**

È il principale owner dell'architettura C++, quindi deve descrivere l'as-built corrente.

Aggiungere/aggiornare almeno:

- ScenarioHarness;
- current hex-only architecture;
- environment E8;
- low/high cover + `URTHexCoverLibrary`;
- current match-format / match-end logic;
- E5 reaction engine;
- boundary verso E14;
- TeamKnowledge E13 come target;
- no-GAS v0.1.

Rimuovere:
- descrizioni che dicono che sistemi già esistenti sono futuri;
- test count statici vecchi.

---

## `technical/brief-planning-visuale.md`

**Azione: UPDATE MINORE**

È ben allineato.

Ripulire:
- note sul working tree temporaneo;
- riferimenti a test "esistono nel branch" ormai mergiati.

Aggiungere:
- preview di Decision Boundary / branch reaction;
- facing timing una volta deciso;
- Team Knowledge come unica fonte per enemy-related uncertainty.

---

## `technical/convenzioni-contenuti-ue.md`

**Azione: UPDATE**

Regole di naming/feature-first valide.

Da togliere o aggiornare:
- "milestone attiva = M6";
- stato temporaneo di cartelle.

La convenzione non deve diventare un tracker.

Aggiungere:
- root `Scenarios/` = dati testuali di Scenario Harness, NON `Content/RT/Tests`;
- Git LFS: documentare la realtà attuale senza riportare come regola attiva il vecchio PDR.

---

## `technical/debug-vs-unreal.md`

**Azione: REWRITE COMPLETA**

È ancora una guida M1 quadrata:
- `RTGridActor`;
- `RTGridLibrary`;
- 10×10;
- vecchi percorsi Content;
- 5 test Grid.

Non va semplicemente rattoppata.

Nuova guida:
- UE 5.8.1;
- hex;
- current breakpoints;
- Scenario Harness;
- `rt.Debug.*`;
- `rt.Test.*`;
- `test-e-diagnosi.md`;
- current Content conventions.

---

## `technical/guida-animazioni-paragon.md`

**Azione: RECLASSIFY / UPDATE**

La guida è utile come recipe Unreal, ma è legata a:
- Gideon;
- Sparrow;
- Guardian/Ranger.

Questi NON sono il roster canonico.

Non sostituire automaticamente i nomi con Flux/Riva/Bastion/Vektor.

Soluzione:
- titolo/stato "Workflow prototipo Paragon";
- separare il metodo generale dalla mapping personaggio;
- mapping visuale del roster = OPEN finché non decisa;
- rimuovere conteggi test vecchi.

---

## `technical/h6-4-hex-vision-spec.md`

**Azione: AS-BUILT**

Congelare come spec consegnata H6.4.

Aggiungere banner:

```text
AS-BUILT 2026-08-05
Current LOS is later amended by E9 covers and E13/E16 perception.
```

Non aggiornare ogni vecchia sezione come se fosse design corrente.

---

## `technical/h6-5-hex-bot-spec.md`

**Azione: AS-BUILT**

Congelare.

Aggiungere "successive requirements":
- bot non può usare hidden enemy state;
- E13 Team Knowledge;
- facing;
- reaction policy;
- E17 stress.

---

## `technical/h6-hex-sim-spec.md`

**Azione: AS-BUILT**

Congelare come ponte temporaneo verso hex.

Il documento dice che square e hex convivono: era vero allora, non oggi.

Banner:

```text
Delivered migration slice.
Square implementation removed at CP 7.2.
Do not use this file as current architecture owner.
```

---

## `technical/plan-turnlog.md`

**Azione: KEEP HISTORICAL**

È già correttamente marcato "Piano di esecuzione consegnato".

Non riscrivere il corpo.

Valutare solo di spostarlo in `roadmap/plans/` per uniformità.

---

## `technical/progettazione-hud.md`

**Azione: UPDATE MINORE**

È uno dei documenti più moderni.

Verificare:
- 3 s reaction window;
- Team Knowledge terminology;
- front cone/facing;
- Sound overlay;
- no enemy private intents.

La PNG style guide è presentazione, non gameplay authority: già corretto.

---

## `technical/spec-asset-pipeline.md`

**Azione: REWRITE / RECLASSIFY**

I principi restano:
- presentation-only;
- soft references;
- fallback;
- BP/AnimBP consumer of authoritative events.

Ma "stato corrente" e pipeline a due archetipi sono vecchi.

Separare:
- **principi asset pipeline current**;
- **vecchio esperimento Gideon/Sparrow** come history.

`convenzioni-contenuti-ue.md` deve restare owner del naming/percorso.

---

## `technical/spec-hover-cella.md`

**Azione: KEEP HISTORICAL**

Già marcato correttamente come superato dal pivot hex.

Nessuna riscrittura del corpo necessaria.

---

## `technical/spec-mappa-multilivello.md`

**Azione: REWRITE o DOWNGRADE / P0**

Il README la presenta come owner del concetto current, ma il corpo usa ancora:
- quadrato 10×10;
- `FRTGridCoord`;
- due ISM da vecchio sistema.

Questo è troppo facile da interpretare male.

Scelta consigliata:
- creare una spec corrente hex multilayer;
- spostare questo corpo in storico/as-built.

La nuova spec deve usare:
- `FRTCellId(q,r,Layer)`;
- `URTHexMapAsset`;
- transizioni esplicite cross-layer;
- cover edge;
- revision;
- current LOS rules.

---

## `technical/spec-pathfinding-pf3-pf4.md`

**Azione: REWRITE COMPLETA / P0**

È indicata come spec corrente, ma descrive ancora per larga parte:
- `FRTGridCoord`;
- Manhattan;
- 4-way;
- paradigma precedente.

La nuova spec current deve descrivere l'as-built:

- 6 vicini hex;
- A*;
- `FRTCellId`;
- costi interi;
- layer transition;
- occupancy;
- `GraphNeighbors`;
- cover alta che può bloccare un bordo;
- map revision;
- determinismo;
- cache solo se realmente implementata;
- target performance misurato.

Preservare il vecchio documento come history, non perderlo.

---

## `technical/spec-pathfinding.md`

**Azione: KEEP HISTORICAL**

Già marcato superato dal pivot hex.

Non aggiornare la logica quadrata.

---

## `technical/spec-team-identity.md`

**Azione: UPDATE STATUS / AS-BUILT**

La spec parla ancora come piano documentale.

Il registro PIE indica che il team ring è stato verificato.

Marcare implementata/as-built.

Non agganciare la spec a una mapping visuale definitiva del roster.

---

## `technical/spec-turnlog.md`

**Azione: UPDATE FORTE**

Il principio resta corretto.

Problema:
una revisione storica nel documento dice che la cover:
- non riduce il danno;
- blocca soltanto la LOS.

Questo non è più vero in generale:
- low cover riduce danno;
- high cover blocca.

Aggiungere current amendment e link alle spec cover.

Aggiornare inoltre le categorie/outcome correnti:
- Environment;
- CoverDamaged;
- CoverDestroyed;
- eventuali altri reason già presenti.

Non riscrivere la storia come se il 3 agosto fosse sbagliato: indicare che era corretto per quel checkpoint.

---

## `technical/spec-turnlog-serialize.md`

**Azione: UPDATE MINORE / AS-BUILT**

Buona implementazione.

Allineare:
- nomenclatura hex current;
- tipi correnti;
- eventuali enum nuovi.

Verificare che l'aggiunta di nuovi outcome/category non richieda una nuova format version.

Non incrementare la versione solo per paura: verificare il layout reale.

---

## `technical/test-automatico-unreal.md`

**Azione: REWRITE COMPLETA / P0**

Questo file è ancora il **prompt originale** di implementazione, non una spec current.

Fra le proposte del prompt c'era un `ARTTestDirector`.

L'implementazione reale ha scelto una soluzione diversa e più semplice:
- CVar;
- GameMode;
- stesso runner;
- zero Actor test obbligatorio.

Quindi:

1. spostare il prompt originale in `src/` o storico;
2. trasformare questo file nell'owner current:
   - architettura Scenario Harness;
   - schema JSON;
   - assertion supportate;
   - PASS/FAIL/ERROR;
   - output;
   - console;
   - auto-run;
   - future Fast Reaction policy;
   - no bypass.

`test-e-diagnosi.md` resta la guida operativa.

---

## `technical/test-e-diagnosi.md`

**Azione: KEEP + UPDATE MINORE**

È attuale e aderente al codice.

Verificare:
- elenco scenari;
- assertion;
- comandi;
- paths.

Evitare test count statici.

---

## `technical/test-manuali-pie.md`

**Azione: REWRITE STRUTTURALE / P0**

Ha un difetto concreto:

```text
74 voci = 23 verdi + 17 parziali + 34 aperte
```

ma più sotto descrive ancora:

```text
31 aperte
2+9+9+4+4+1+2 = 31
```

Le tre nuove voci Scenario Harness hanno cambiato il totale e non tutta la narrativa è stata aggiornata.

Correzione:
- una sola tabella/registro;
- conteggi derivati dal comando;
- gruppi derivati o aggiornati automaticamente;
- nessun "31 open" hard-coded separato.

Ripulire anche i vecchi branch name dalle precondizioni quando ormai mergiati.

---

## `technical/use-case-list.md`

**Azione: MOVE/ARCHIVE o DELETE**

Contenuto:

```text
stiamo lavorando in parallelo su feat/hex-grid su questo terminale, usa un worktree
/sp:spec-panel
```

Non è una specifica, un use case o una guida.

Non deve restare in `technical/`.

Se serve provenance:
- spostare in archive/session-notes.

Altrimenti eliminare.

---

## Asset visuali sotto `technical/img/`

Non sono specifiche testuali ma vanno classificati:

- `Mock-HUD.png`
- `UI-style-guide.png`
- `brainstorming.png`

`UI-style-guide.png` è il riferimento visuale indicato da `progettazione-hud.md`.

Verificare se `brainstorming.png` è un duplicato byte-identico / blob-identico della style guide.  
Se sì:
- mantenere un solo asset canonico;
- aggiornare link.

Non rimuovere immagini senza verificare i riferimenti.

---

# 5F. `docs/roadmap/`

## `roadmap/roadmap-checkpoint.md`

**Azione: REWRITE STRUTTURALE / P0**

La parte alta è relativamente aggiornata.

Più sotto restano snapshot che contraddicono la parte alta, per esempio E8 ancora descritta parziale dopo essere stata chiusa.

Rifattorizzare:

```text
CURRENT STATE — una tabella
↓
milestone detail
↓
historical changelog — link, non snapshot duplicato
```

Non duplicare l'intero stato epic dentro due viste.

---

## `roadmap/roadmap-v0.1.md`

**Azione: REWRITE STRUTTURALE / P0**

Stesso problema:
- stato corrente;
- vecchio snapshot;
- §2.1 che corregge lo snapshot.

Risultato: il file contiene contemporaneamente "E8 da fare" e "E8 chiusa".

Tenere una sola tabella current.

### Correzione scope

Oggi E14 è parte della roadmap.

Quindi "reaction stack interattivo fuori v0.1" deve essere precisato:

**fuori scope**:
- stack LIFO arbitrario;
- reaction annidate;
- finestre che aprono finestre.

**in scope E14**:
- Decision Window bounded;
- non annidata;
- Overwatch/Fast Reaction.

---

## `roadmap/v0.1-definition-of-done.md`

**Azione: UPDATE FORTE / P0**

La tabella DoD epic arriva solo a E12.

Mancano:
- E13 Team Knowledge;
- E14 Decision Windows/Overwatch;
- E15 showcase;
- E16 facing;
- E17 stress validation, se è davvero un gate o soltanto post-showcase measurement.

Non fare di E17 un gate release se la roadmap non lo richiede.

### G9

Il gate "12 verifiche manuali" non è più sostenibile:
il registro PIE ha molte più voci.

Definire:
- un subset `RELEASE-V01`;
- oppure un tag/colonna nel PIE registry.

### KPI

Aggiornare:
- ambiente;
- cover;
- scenario harness;
- reaction determinism;
- TeamKnowledge/privacy;
- facing.

Rimuovere "ice optional" se la regola è ormai implementata e accettata.

---

## `roadmap/hex-map-roadmap.md`

**Azione: FREEZE AS-BUILT**

È una roadmap consegnata H0–H6.5.

Il corpo contiene ancora la narrativa di coesistenza square/hex.

Non aggiornarlo come tracker corrente.

Banner in testa:

```text
DELIVERED / HISTORICAL EXECUTION RECORD
Hex became the only gameplay substrate.
Current roadmap: roadmap-checkpoint.md
```

---

## `roadmap/roadmap-editor.md`

**Azione: REWRITE o RETIRE**

Era una buona idea: separare le sedute che richiedono editor.

Ma il "prossimo lavoro U1/U2/U3" è indietro rispetto al codice e a `GeneratedTestArena`.

Due opzioni:

### A — mantenere

Rigenerarla dal PIE registry e dalla roadmap corrente.

### B — preferita se non si automatizza

Eliminare la terza vista operativa e usare:
- `test-manuali-pie.md` = registro;
- roadmap-checkpoint = priorità;
- una breve sezione "next editor session" generata.

Evitare tre tracker da sincronizzare a mano.

---

## `roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`

**Azione: REWRITE COME LONG-TERM REQUIREMENTS**

Buona conservazione del PDR, ma la colonna "Stato repo" è diventata vecchia:
- 63 test;
- 2 archetipi;
- E8 non fatto;
- vecchie classi.

Un PDR di lungo periodo non dovrebbe duplicare lo stato giornaliero.

Tenere:
- F0–F6;
- risk model;
- performance target;
- DoD north-star.

Rimuovere o sostituire lo stato con link a:
- `roadmap-checkpoint.md`;
- `roadmap-v0.1.md`.

Correggere i path `../design/...`.

---

## `roadmap/v0.1-issue-plan.md`

**Azione: RECLASSIFY HISTORICAL / GENERATED**

Il file contiene i body usati per creare issue, ma molti descrivono lo stato del giorno in cui furono creati.

Non deve essere un'altra source of truth.

Opzioni:

- `ISSUE CREATION SNAPSHOT — non normativo`;
- oppure rigenerare i body dalle current roadmap.

GitHub Issues + current roadmap devono vincere.

Correggere heading tipo "Le 12 epic" se si mantiene current.

---

# 5G. `docs/roadmap/plans/` — audit individuale

Questi file sono **piani consegnati**.  
Non vanno riscritti per farli sembrare moderni: cambiare vecchi comandi, branch o percorsi falsificherebbe la storia.

Regola:

```text
corpo storico invariato
+
header uniforme:
DELIVERED EXECUTION PLAN — NON NORMATIVO
Current owner: <link>
```

| Documento | Azione |
|---|---|
| `cp6-1-hex-match-setup-plan.md` | **HISTORICAL — KEEP BODY** |
| `cp6-3-hex-input-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5-editor-mode-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5-editor-mode-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c-followup-erase-lazy-transaction-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c-paint-tool-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c-paint-tool-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c2-arch-gizmo-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c2-arch-gizmo-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c3-drag-brush-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c3-drag-brush-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c4-radius-brush-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c4-radius-brush-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c5-arch-remove-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c5-arch-remove-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `h5c6-overlay-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c7-flood-fill-plan.md` | **HISTORICAL — KEEP BODY** |
| `h5c7-flood-fill-spec.md` | **AS-BUILT/HISTORICAL — KEEP BODY** |
| `pacing-turno-plan.md` | **HISTORICAL — KEEP BODY**; current timing lives in gameplay/decision docs |
| `plan-terreni-e8.md` | **HISTORICAL — E8 DELIVERED** |
| `handoff-prossima-sessione.md` | **MOVE/ARCHIVE** — un handoff temporaneo non deve restare come istruzione corrente |

Claude deve prima verificare che non esistano altri file in `roadmap/plans/`; se ce ne sono, applicare lo stesso criterio e aggiungerli al report finale.

---

# 6. Decisioni chiuse dall'autore — applicare

Le 10 decisioni emerse dall'audit sono state chiuse il **2026-08-08**.  
Claude deve prima verificare il repository corrente, poi recepirle in:

- canone;
- ADR / Decision Log;
- balance catalogs;
- roadmap;
- DoD;
- issue plan;
- test plan;
- codice, **solo dove la documentazione rivela una divergenza reale che richiede implementazione**.

Non inventare valori mancanti.

---

## D-AUDIT-01 — Azioni generiche

Confermate per la v0.1:

```text
Wait
Move
BasicAttack
Guard
Brace
Activate
Interact
Overwatch
```

Non eliminare `Guard`, `Brace`, `Activate` o `Interact`.

Semantica:
- `Guard` = protezione generale;
- `Brace` = stance/anti-displacement più specifica;
- `Activate` = attivazione di dispositivo/mappa;
- `Interact` = interazione generica/obiettivo;
- `Overwatch` = azione universale che compete con l'azione principale offensiva.

---

## D-AUDIT-02 — Sneak / Move / Sprint diventano profili del movement slot

Cambiare il modello **ora, nella v0.1**.

Il movimento normale usa una famiglia:

```text
MovementMode.Sneak
MovementMode.Move
MovementMode.Sprint
```

Regole strutturali:

- tutti occupano il **movement slot**;
- tutti risolvono nella macro-fase **Move**, quindi dopo `Blast`;
- `Dash`, `Leap`, `Charge`, `Reposition` restano movimenti speciali della fase **Dash**;
- non usare `Sprint` come sinonimo di `Dash`;
- mantenere `Move = 5 MP` e `Sprint = 8 MP` se questi valori sono ancora i canonici verificati;
- `Sprint` conserva un trade-off reale: non deve diventare un upgrade puro;
- per `Sneak`, se costo/range/rumore non sono già definiti da una fonte corrente, introdurre il concetto e aprire una decisione/balance issue invece di inventare numeri.

Conseguenza:
- aggiornare ActionCatalog;
- aggiornare ADR-0003;
- aggiornare resolver/data model se oggi `Sprint` consuma anche il Main Action;
- aggiungere test per slot, fase e trade-off;
- migrare issue/roadmap.

---

## D-AUDIT-03 — Una Predictive Action entra nella v0.1

La v0.1/showcase deve mostrare **almeno una vera azione predittiva**.

Direzione consigliata e approvata:
- usarla su **Vektor**;
- dichiarata interamente in Planning;
- il giocatore scommette su cella/linea/boundary attraversata dal nemico;
- se il trigger si verifica, payoff;
- se non si verifica, l'azione è sprecata o produce il fallback definito;
- NON apre una nuova scelta live solo perché il payoff avviene più tardi.

Quindi è:

```text
Predictive / Delayed
!=
Fast Action
```

Scope:
- una sola vertical slice rappresentativa;
- niente framework enorme di trappole se non necessario;
- riusare la stessa infrastruttura trigger/opportunity dove sensato;
- scenario test automatico obbligatorio;
- showcase E15 deve usarla o dimostrarla chiaramente.

---

## D-AUDIT-04 — Intercept rivalida contro il vero bersaglio

Se A era il bersaglio e B intercetta:

```text
Attacker → A
          ↓ Intercept
Attacker → B
```

la validazione geometrica deve essere rifatta rispetto a **B**.

Rivalidare:
- LOS/traiettoria;
- cover;
- target legality rilevante.

Non usare i modificatori calcolati sulla vittima originale se non sono ancora validi sul bersaglio reale.

Vincolo:
- la rivalidazione NON apre una reaction annidata;
- il risultato resta deterministico;
- aggiungere test con cover differente su A e B.

---

## D-AUDIT-05 — High Ground base = geometria, non danno

Decisione:

> l'altura di base NON aumenta il danno.

Default:
- vantaggio geometrico;
- LOS;
- cover;
- accessibilità;
- eventuale informazione/percezione data dalla geometria.

Non aggiungere:
- `+Damage` globale;
- `+VisionRange` globale.

È invece permesso che:
- un personaggio;
- una trait;
- una ability;
- un equipaggiamento;
- una specializzazione

dichiari esplicitamente un bonus collegato alla quota.

Verificare `OccupantDamageBonus`:
- se implementa il vecchio bonus generico di altura, rimuovere quella connessione;
- se è un meccanismo generico riusabile, mantenerlo ma non valorizzarlo automaticamente per High Ground;
- aggiornare test e cataloghi.

---

## D-AUDIT-06 — Fast Action: significato confermato

`Fast Action` indica esclusivamente:

> una decisione live breve e limitata, dentro una Decision Window, come continuazione/opzione della propria azione.

Non è sinonimo di:
- Delayed Action;
- Predictive Action;
- azione completamente pianificata.

Non implementare Fast Action solo per "avere il sistema".

Implementarla quando esiste una ability concreta che la richiede.

La Decision Window resta infrastruttura condivisa con Fast Reaction.

---

## D-AUDIT-07 — Facing: l'unità si orienta verso il target

Decisione:

> prima che una azione direzionale/targeted risolva, il personaggio si orienta verso il suo target/direzione.

Esempi:

```text
Dash NE
→ facing = NE

Blast su Target X
→ facing = direzione di X
→ Blast risolve

Overwatch cone NW
→ facing reaction = NW
```

Regole:
- il facing è stato gameplay;
- una azione può cambiarlo prima del `Move`;
- reaction successive leggono il facing autorevole più recente;
- il normale `Move`, essendo ultimo, definisce il facing finale tramite ultima direzione percorsa o facing finale pianificato se supportato;
- il facing finale persiste nel round successivo.

Aggiungere test:
- Dash → Blast;
- target switch;
- Overwatch cone;
- Move finale.

---

## D-AUDIT-08 — Una Decision Window privata non viene rivelata agli avversari

Decisione forte di privacy:

> gli avversari NON devono poter capire che un altro giocatore ha ricevuto una Fast Reaction/Decision Window.

Motivazione:
- i personaggi agiscono in contemporanea;
- la reaction privata non deve creare un "freeze globale" osservabile che tradisca il trigger.

Requisiti:

1. il server può sospendere la progressione logica al Decision Boundary;
2. il client autorizzato riceve la finestra/opzioni;
3. il client avversario NON riceve:
   - trigger;
   - opportunity;
   - allowed responses;
   - identità del responder;
   - timeout;
   - metadata che permettano di inferire la finestra;
4. la presentazione avversaria non deve avere una pausa variabile correlata alla scelta privata;
5. usare buffering/pacing/fixed resolution beats o altro meccanismo per mantenere la percezione di simultaneità;
6. timeout e risposta devono essere server-authoritative;
7. aggiungere test di packet/privacy e un piano di verifica del timing side-channel in M10/E14.

Questo diventa parte dell'invariante "zero leak di intenti/informazioni private".

---

## D-AUDIT-09 — Unreal Engine 5.8.1 consolidata

La baseline ufficiale del progetto è:

```text
Unreal Engine 5.8.1
```

Regola:
- upgrade solo tra milestone;
- upgrade solo con migrazione esplicita;
- documentazione, build scripts e guide devono parlare della stessa baseline;
- `OPEN_DECISIONS` non deve più presentarla come semplice assunzione se il repo conferma 5.8.1.

---

## D-AUDIT-10 — Workbook v0.1 = research, Markdown = canonico

Decisione:

```text
RefactorTactics_Balance_Matrices_v0.1.xlsx
= RESEARCH / EXPLORATION

docs/balance/RT_*Catalog_v0.1.md
= CANONICAL BALANCE DATA
```

Il workbook non deve più essere usato per risolvere conflitti contro i cataloghi.

Un futuro workbook v0.2 dovrà essere:
- derivato dai dati canonici;
- versionato;
- validato;
- idealmente generabile/controllabile automaticamente.

---

## Registrazione delle decisioni

Nel `RT_PDR_00_Decision_Log.md`:

- verificare il prossimo ID libero;
- aggiungere una decisione per ciascun blocco che merita tracciamento permanente;
- è consentito raggruppare decisioni strettamente collegate se il log resterebbe più leggibile;
- non riutilizzare ID esistenti;
- annotare `2026-08-08` come data;
- linkare ADR/documenti aggiornati.


# 7. Cose che mancano davvero, oltre alle correzioni

## 7.1 Una current architecture spec corta

`architettura-codice.md` deve diventare una mappa affidabile:

```text
Input / Bot / Scenario
        ↓
Planned Intent
        ↓
Turn Manager
        ↓
Segment Snapshot
        ↓
Action Queue / Resolver
        ↓
Decision Boundary?
        ↓
TurnLog
        ↓
Presentation
```

Con:
- map;
- path;
- LOS;
- cover;
- environment;
- reaction;
- scenario harness.

---

## 7.2 Una current hex path/map spec realmente corrente

Oggi l'owner linkato dal README è ancora intriso del passaggio square→hex.

Serve una spec pulita che una persona possa leggere senza conoscere la migrazione.

---

## 7.3 Una spec as-built dello Scenario Harness

Il prompt originale non può continuare a fare da documentazione.

Serve descrivere ciò che **è stato realmente implementato**.

---

## 7.4 Gate release per E13–E16

La DoD non è stata ampliata insieme alla roadmap.

Quindi feature nuove sono pianificate senza avere ancora un release gate equivalente.

---

## 7.5 Test matrix per Decision Boundary

Prima di E14 servono test espliciti almeno per:

```text
no nested decision
timeout
HOLD
FIRE
multiple opportunities over time
same-microstep multi-target opportunity
stale response
unauthorized response
deterministic resume
TurnLog records response
no future information in DTO
```

---

## 7.6 Privacy temporale

È un gap reale non coperto dalla vecchia definizione "payload team-only".

Decision Boundary introduce una nuova forma di side channel.

---

## 7.7 Un solo modello di status corrente

Molti documenti registrano "stato implementazione".

La roadmap deve essere l'unica owner.

Le spec devono dire:
- requisito;
- as-built version;
- link al tracker.

---

# 8. Piano operativo per Claude Code

## Fase 0 — Non modificare ancora

1. checkout/sync `main`;
2. leggere:
   - `CLAUDE.md`;
   - `AGENTS.md`;
   - `docs/README.md`;
3. misurare:
   - HEAD;
   - numero test;
   - file test;
4. inventariare esattamente tutti i file in scope;
5. confrontare l'inventario con questo audit;
6. se esiste un documento in scope non elencato qui, aggiungerlo all'audit prima di modificarlo.

---

## Fase 1 — Aggiornare governance

Prima di toccare decine di documenti:

1. aggiornare `DOC_CONFLICT_MATRIX.md`;
2. aggiornare `OPEN_DECISIONS.md`;
3. NON chiudere gli `HD-*`;
4. correggere la gerarchia Canone/ADR;
5. definire i tag documentali:

```text
CURRENT
CANONICAL
AS-BUILT
DELIVERED PLAN
HISTORICAL
RESEARCH
OPEN
```

---

## Fase 2 — Correzioni sicure

Claude può applicare senza decisione umana:

- test count misurati;
- path rotti;
- old `../design/...` links;
- status "in implementazione" quando il codice dimostra che è completo;
- banner storico/as-built;
- rimozione di snapshot current duplicati;
- aggiornamento E8/E9/Scenario Harness;
- `use-case-list.md` → archive/remove from current technical;
- `test-automatico-unreal.md` → spec current;
- roadmap stale duplicate state;
- DoD E13–E16;
- Scenario Harness gate;
- current architecture as-built.

---

## Fase 3 — Applicare le decisioni chiuse senza inventare numeri

I precedenti HD-1…HD-10 sono ora chiusi nella sezione 6.

Claude deve:

```text
APPLY THE DECISION
VERIFY THE CURRENT CODE
DO NOT INVENT MISSING BALANCE NUMBERS
ADD TESTS / ISSUES WHEN CODE MUST CHANGE
```

Se una decisione richiede un valore numerico che non è stato definito (es. parametri esatti di `Sneak`), non scegliere un numero arbitrario: lasciare il campo esplicitamente `TBD`/non specificato nella documentazione appropriata e creare una issue di bilanciamento.

---

## Fase 4 — Riclassificare la storia

Non cancellare conoscenza utile.

Spostare/etichettare:
- delivered plans;
- as-built checkpoint specs;
- old square specs;
- old asset prototype guides.

Il loro valore è storico/tecnico, non normativo.

---

## Fase 5 — Roadmap cleanup

Obiettivo:

```text
roadmap-checkpoint.md = CURRENT EXECUTION STATE
roadmap-v0.1.md      = CURRENT RELEASE SCOPE
test-manuali-pie.md  = MANUAL VERIFICATION REGISTRY
```

Non devono esistere altre tre tabelle concorrenti che raccontano lo stesso stato.

---

## Fase 6 — Validation

Eseguire almeno:

### Link relativi

Verificare tutti i Markdown link in `docs/`.

### Symbol drift

```bash
python scripts/check-docs-symbols.py
```

### Test count

Usare il comando dichiarato nel repo.

### Search di termini obsoleti

Cercare nei documenti CURRENT/CANONICAL:

```text
FRTGridCoord
URTGridLibrary
ARTGridActor
4-way
Manhattan
5 second reaction
6 second reaction
7 second reaction
FastReaction = 6
Aegis
Nyx
Drift
Vex
Mara
Ivo
Sol
Steel
Aurora
Murdock
Kwang
2 archetypes
63 tests
172 tests
324 tests
338 tests
390 tests
403 tests
412 tests
GAS in v0.1
3v3 main format decided
```

Non ogni occorrenza è un errore:
- uno storico può citarli;
- una conflict matrix deve citarli.

Il gate deve verificare **solo documenti current/canonical**.

### Search delle decisioni appena consolidate

Nei documenti CURRENT/CANONICAL verificare inoltre:

```text
Sprint consumes Movement + Main Action
generic high-ground damage bonus
global +VisionRange from High Ground
reaction window visible to enemy
global resolution pause reveals reaction
facing only changes after Move
Intercept keeps original target cover
workbook is canonical
UE 5.8 assumption
```

Ognuna di queste frasi deve essere rimossa, corretta o classificata come storico se contraddice la sezione 6.

---

# 8.1 Impatto codice/issue delle decisioni chiuse

Questo audit nasce come consolidamento documentale, ma alcune decisioni sono **semantic changes** per la v0.1.

Claude deve confrontare codice e test prima di dichiararle recepite.

## Movement profile refactor

Verificare:
- `Action.Sprint`;
- action slot model;
- `MovementStyle`;
- planner/HUD;
- resolver;
- bot;
- catalog validator.

Se `Sprint` oggi usa Movement + Main Action, creare/refactorare il modello per `Sneak/Move/Sprint` nel movement slot.

## Predictive Action

Creare/aggiornare epic/checkpoint per una slice Vektor:
- dati;
- trigger;
- resolver;
- UI preview;
- TurnLog;
- Scenario Harness;
- golden replay.

## Intercept

Aggiungere test:
- original target in cover / interceptor exposed;
- original target exposed / interceptor in cover;
- LOS to interceptor invalid;
- no nested reaction.

## High Ground

Cercare:
- `OccupantDamageBonus`;
- `EffectiveAttackPower`;
- map generation defaults;
- hero/bot scoring that assumes generic damage bonus.

Rimuovere solo la semantica generica da altura.

## Facing

Aggiungere timeline test:
- Dash changes facing;
- Blast turns to target;
- Overwatch cone;
- Move defines final facing.

## Private Decision Window

La privacy temporale è un requisito di E14/M10:
- server-authoritative boundary;
- private DTO;
- non-observable opponent timing;
- deterministic resume;
- replay/logging.

Se non implementabile nella v0.1 offline, documentare l'architettura e creare test/issue per il multiplayer; **non degradare il requisito**.

---

# 9. Output richiesto a Claude

Prima delle modifiche:

```text
AUDIT DIFF
- file in scope
- safe fixes
- blocked fixes
- files to reclassify
- files to move
```

Dopo:

```text
CONSOLIDATION REPORT
1. HEAD
2. files inspected
3. files changed
4. files moved/reclassified
5. conflicts added
6. open decisions
7. current test count
8. links checked
9. docs symbol gate result
10. code/document mismatches still present
11. suggested GitHub issues
```

---

# 10. Commit sequence consigliata

Non fare un mega-commit.

```text
docs(governance): register second-pass documentation conflicts
docs(product): fold accepted ADR amendments into canonical model
docs(technical): refresh current architecture and scenario harness
docs(technical): separate current specs from delivered hex migration specs
docs(balance): align catalog status and mark workbook role
docs(roadmap): remove stale duplicate status snapshots
docs(qa): align release gates and PIE registry
docs(changelog): record 2026-08-08 consolidation pass
```

Non pushare senza richiesta.

---

# 11. Stop condition

Le 10 decisioni principali dell'audit sono chiuse.

Claude deve fermarsi e chiedere decisione umana solo se durante il consolidamento emerge **una nuova scelta non coperta** da questo file, per esempio:

- un valore numerico di `Sneak` non presente nelle fonti;
- la forma esatta della Predictive Action di Vektor se esistono più alternative incompatibili;
- un cambio di action economy oltre il modello movement-slot approvato;
- una modifica sostanziale al numero di slot per turno;
- una nuova forma di reaction annidata;
- una modifica al formato principale 3v3/4v4;
- una rinomina pubblica dei quattro eroi;
- una nuova mapping definitiva Paragon → Flux/Riva/Bastion/Vektor;
- una scelta di rete che indebolisca la privacy temporale approvata.

Non fermarsi per semplici divergenze documentali già risolte qui.

# 12. Risultato finale desiderato

Dopo il consolidamento:

```text
docs/README.md
```

deve portare a una catena coerente:

```text
CANONE
  ↓
ADR / Decision Log
  ↓
CURRENT SPECS
  ↓
BALANCE CATALOGS
  ↓
CURRENT ROADMAP
  ↓
TEST / EVIDENCE
```

e un documento storico deve essere immediatamente riconoscibile come:

```text
AS-BUILT
DELIVERED
HISTORICAL
RESEARCH
```

senza che il lettore debba interpretare date, commit e correzioni per capire se una frase vale ancora.

La priorità non è produrre più documenti.

La priorità è fare in modo che **quelli esistenti non possano contraddirsi silenziosamente**.
