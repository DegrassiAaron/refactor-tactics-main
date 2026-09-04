# Correzioni meccaniche — Gameplay Review & handoff del 2026-09-04

> `SNAPSHOT` · **Ruolo**: `SUPPORTING / HANDOFF` · **Misurato**: 2026-09-04 · **Base**: `origin/main` `b7be0eda`, GitHub LIVE
> **Cosa è**: la preservazione nel repository della review gameplay del 2026-09-04 — tesi, fatti, dieci
> finding, cinque scenari diagnostici, work order — **più** l'esito della riconciliazione contro lo stato
> corrente. Esiste perché la conversazione che l'ha generata possa essere rimossa senza perdere le
> conclusioni di design.
> **Cosa non è**: un owner normativo. Per la semantica delle regole vincono
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md), gli ADR e gli
> owner spec `CURRENT`; per lo stato di issue, epic e PR vince GitHub LIVE.
> **Sedi collegate**: Epic [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276) ·
> prima issue [#2277](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2277) ·
> mirror Drive *«RefactorTactics — Correzioni meccaniche — Gameplay Review & Claude Handoff — 2026-09-04»*
> ([doc](https://docs.google.com/document/d/1WCV5jZ9o7wLDueKKoALUyDhH-mGXHjAHEI5y9tshKKw/edit)).

## 0. Come si legge questo documento

Il testo della review è preservato per intero nelle §1–§5 e §8–§9. Ciò che la review dichiarava
**inferenza** resta inferenza: dove la misura contro `origin/main` la conferma, la §6 lo dice; dove la
falsifica, la §6 lo dice più forte. **Nessuna riga di questo file autorizza una modifica meccanica.**

---

## 1. Tesi della review

RefactorTactics ha una base sistemica forte: WEGO deterministico, snapshot immutabile, resolver a fasi,
micro-step atomici, TurnLog/replay, cover e facing direzionali, reaction boundary. Il rischio principale
non è la mancanza di meccaniche, ma il **sovraccarico di previsione**: il giocatore può essere costretto a
prevedere troppe regole contemporaneamente, e perdere perché non comprende il resolver invece che perché
ha letto male l'avversario.

> **Principio guida**: trasformare complessità sistemica in decisioni **leggibili, falsificabili e con
> counterplay**; non aggiungere sistemi per coprire sistemi già difficili da leggere.

## 2. Fatti di contesto da preservare

Elencati dalla review come fatti. La colonna di verifica è di questo audit, non della review.

- Baseline v0.1: **2v2 offline contro bot** su griglia hex multilivello, WEGO deterministico.
- Loop: `Planning → Ready/Commit → Snapshot → Prep → Dash → Blast → Move → Cleanup → TurnLog → Next Turn / Match End`.
- Reaction/Fast Reaction è un **Decision Boundary**, non una macro-fase autonoma.
- Movimento normale: micro-step **simultaneo e atomico**; tutte le proposte dello stesso boundary leggono
  lo stesso stato, e il successful-move set viene applicato insieme.
- Baseline collisioni documentata: stessa destination senza precedence esplicita ⇒ tutti `BLOCK`; direct
  swap ⇒ `BLOCK`; ciclo chiuso ⇒ `BLOCK`; free convoy con coda libera ⇒ avanza; blocked tail ⇒ il blocco
  propaga indietro. ⚠️ **Precisata dalla misura**, §6.2.
- Facing: una transizione riuscita aggiorna il `Facing` alla direzione del passo completato; una
  bloccata/fallita lo **preserva**; la reaction su entered-cell legge quel `Facing`; il Final Pivot è
  successivo e **non retroattivo**.
- Ordine rilevante al modello mentale: `Dash → Blast/Attack → Move`. Il `Move` normale **non** sposta
  l'origine dell'attacco di quel turno; il `Dash` può farlo.
- Cover/intra-hex: `FRTCellId` resta nodo di navigazione e **singolo occupancy slot**;
  `CoverOption`/`CoverSide`/`Facing` sono stato tattico **terminale**, non sottocelle navigabili.
- UX di postura destinazione fissata nel materiale Drive:
  `Destination Cell → CoverOption/CoverSide → Facing → BorderView`; il passo Cover si salta se non ci sono
  opzioni selezionabili.
- Objective contest: verifica nel `Cleanup`; anche `Wait` può contestare; parità ⇒ nessun progresso.
- Reaction pacing: la DoD di `E14`/`CP 14.6` richiede misura con **1/2/3 unità armate**; oltre **20 s**
  si apre la revisione dell'ADR.
- Il backend di danno e skill è volutamente ricco e modulare; ciò **non** implica che la baseline di
  contenuto debba attivare tutti gli assi contemporaneamente.

## 3. I dieci finding

### `CM-01` — **CRITICO** — Prediction overload / explainability

**Inferenza.** Una decisione apparentemente semplice può dipendere da percorso, collisione prevista,
Layer, `CoverOption`, `CoverSide`, `Facing` di transito, Pivot, Dash, origine del Blast, LOS, reaction,
status ed environment. Se il giocatore non distingue **ciò che è certo** da **ciò che è previsto**, la
profondità diventa rumore.

**Proposta.** Trattare la prediction UI come meccanica core, non come polish:

- Ghost Timeline per fase;
- stato `CERTAIN` / `PREDICTED` / `CONDITIONAL`, con un canale **non solo cromatico**;
- reason code già in hover, **prima** del click;
- warning di celle potenzialmente contese, **senza leggere il planning privato avversario**;
- spiegazione post-resolution della catena causale.

**Owner esistenti da riusare.** `E11` #25, #172, #173; playback/inspection #1881.
➕ **Aggiunti dall'audit**: #1937 (Player Event Log & Explainability) e #1936, che la review non conosceva.

### `CM-02` — **CRITICO** — Collisioni e choke

**Inferenza da testare, non regola.** `same destination ⇒ BLOCK ALL` è tecnicamente elegante ma può
generare turni sprecati, stalli nei choke, o **denial intenzionale** come strategia dominante.

⛔ **Non cambiare la policy per inerzia.** Eseguire un confronto diagnostico fra:

- **A.** strict `BLOCK` corrente;
- **B.** una variante di test tipo *«stop all'ultima posizione sicura»*, **solo** come prototipo.

**Metriche**: percentuale di `Move` senza variazione di posizione · turni consecutivi bloccati nello
stesso choke · collisioni intenzionali usate come denial · previsione corretta dell'esito da parte del
giocatore.

**Owner/issue citati dalla review**: #1922 per swap/cicli; #1733 per occupancy/diagnostica correlata.
🔴 **Entrambe CLOSED** — §6.2.

### `CM-03` — **ALTO** — `Destination → Cover → Facing` può diventare falsa scelta

**Rischio.** Se contro una minaccia esiste quasi sempre una postura dominante, costringere il giocatore a
scegliere manualmente `CoverOption` e `Facing` aggiunge **click**, non profondità.

**Proposta da testare.** Default/postura consigliata prevedibile, con override manuale esperto. Misurare
quanto spesso il giocatore cambia il default, e quanto spesso quel cambio **altera davvero l'esito**.

**Owner esistente**: `E23` #324 e il cluster cover/placement; gli owner correnti del `Facing`.

### `CM-04` — **ALTO** — `Dash → Blast → Move` è interessante ma controintuitivo

**Fatto.** L'attacco usa la posizione **post-Dash**, non post-Move.

**Proposta.** Preservare inizialmente l'ordine e renderlo esplicito in planning e playback:

1. `DASH` → ghost della posizione;
2. `BLAST` → linea/footprint **dall'origine prevista**;
3. `MOVE` → destinazione finale.

Se dopo pochi match i giocatori continuano sistematicamente a credere che l'attacco parta dalla posizione
di `Move` finale, rivalutare il costo cognitivo della regola rispetto alla profondità che produce.

### `CM-05` — **ALTO** — Le reaction possono serializzare troppo il WEGO

**Rischio.** Ogni prompt `FIRE`/`HOLD` sospende la risoluzione simultanea e crea un momento seriale.

**Proposta.** Oltre ai secondi totali, misurare i **Manual Decision Boundaries** per turno e per
giocatore. La soglia dei **20 s** va trattata come *red line di revisione*, non come target desiderabile.

**Owner esistente**: `E14` #152 / `CP 14.6` #166.

### `CM-06` — **ALTO** — Backend del danno ricco, baseline di contenuto troppo ricca

**Rischio.** `Armor` + `DamageResistance` + `TemporaryShield` + `Shield` + Active Defense + status +
environment, se tutti presenti subito, rendono difficile stimare l'esito.

**Proposta.** Mantenere l'architettura estensibile ma **limitare la baseline di contenuto**. Prima
dimostrare che il giocatore sa stimare *«se faccio X, circa succede Y»*, poi aggiungere assi di
mitigazione e interazione.

### `CM-07` — **ALTO** — La modularità può cancellare l'identità degli eroi

**Rischio.** La pipeline `Base Skill → Specialization → Equipment → Talents → Status → Environment →
Action Context` può diventare *modifier soup* e rendere i personaggi intercambiabili.

**Proposta.** Ogni eroe deve possedere un'**identità tattica stabile**. La modularità modifica **come**
esprime quell'identità, non la sostituisce. Guardrail da conservare, per ogni skill: *perché la uso ·
perché non la uso · counterplay · opportunity cost*.

### `CM-08` — **ALTO** — Geometria intra-hex che «mente» sull'occupancy

**Fatto.** Un `FRTCellId` ha **un solo** occupancy slot anche se la geometria crea Side A/Side B o più
`CoverOption`.

**Regola di level design proposta.** Se visivamente sembrano due spazi separati in cui due unità
dovrebbero poter stare contemporaneamente, **non dovrebbero essere lo stesso occupancy node**: in quel
caso servono nodi/celle tattici distinti.

### `CM-09` — **MEDIO/ALTO** — Objective deadlock

**Rischio da misurare.** Un contest paritario nel `Cleanup` con zero progresso può incentivare il
parcheggio passivo e il congelamento dello stato.

**Metriche**: turni consecutivi contestati senza progresso · cambi di posizione · azioni e rischi
intrapresi · frequenza con cui la squadra in vantaggio preferisce **congelare** anziché creare gioco.

⛔ **Non introdurre anti-stall prima di aver dimostrato il problema.**

### `CM-10` — **CRITICO** — Profondità strategica fra i turni

**Domanda aperta.** La documentazione è molto forte su **come** si risolve un turno, meno esplicita su
**perché** una buona scelta ora differisce da una buona scelta pensando a tre turni avanti.

Auditare e rendere esplicita l'**economia strategica realmente posseduta**: posizione, HP/attrition,
cooldown, objective pressure, risorse consumabili o altri assi già esistenti. ⛔ **Evitare di inventarne
uno per riempire il vuoto.**

**Failure mode**: turni tatticamente profondi, partita strategicamente piatta.

## 4. Cinque scenari diagnostici minimi

1. **Collision Choke** — due squadre convergono sulla stessa zona/choke.
2. **Dash → Blast → Move** — verifica della comprensione delle tre posizioni e dell'origine dell'attacco.
3. **Cover Choice** — una destination con due posture/cover realmente confrontabili.
4. **Overwatch Pressure** — 1, 2 e 3 reaction potenziali.
5. **Objective Deadlock** — controllo centrale contestato per diversi turni.

Per ogni scenario registrare almeno: tempo di Planning · numero di modifiche al piano · outcome inattesi ·
casi in cui il giocatore dice *«non so perché è successo»* · azioni che non producono effetto · numero di
**Decision Boundary manuali** · durata della Resolution · turni senza cambiamento significativo dello
stato. Per *Collision Choke* si aggiunge: collisioni intenzionali usate come **denial**.

## 5. Due gate qualitativi fondamentali

Prima della Resolution: **«il giocatore sa descrivere cosa pensa che succederà?»**
Dopo un risultato inatteso: **«sa indicare quale propria assunzione era sbagliata?»**

Se sì, la complessità tende a essere **profondità**. Se no, tende a essere **rumore** e incomprensione del
resolver.

---

## 6. Riconciliazione — misurata il 2026-09-04

### 6.1 Base della misura

| Cosa | Valore |
|---|---|
| `origin/main` | `b7be0eda` |
| Epic allocate | `E1`–`E51` — `E49` #1769, `E50` #1816, `E51` #1848 |
| `E52` | **non aperto**, e non si apre: nessun owner di roadmap lo richiede |
| Issue nuove aperte da questo audit | **2** — l'Epic #2276 e la prima issue #2277. Zero per i dieci finding |

### 6.2 Le sei affermazioni che il LIVE falsifica

1. 🔴 **#1922 è CLOSED.** Chiusa il **2026-08-31** con `D-295`: lo swap diretto e il ciclo chiuso ora
   bloccano con `ERTMoveOutcome::BlockedByCycle`, il convoy a coda libera continua ad avanzare. Owner
   documentale: [`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §19.
   ⚠️ Prima del 2026-08-31 quella riga diceva *«già implementato»* per entrambi, ed era falsa per metà —
   `HexSim.ResolveSwapAllowed` asseriva che lo scambio **riusciva**.
2. 🔴 **#1733 è CLOSED.** La review la elenca fra gli owner di occupancy/diagnostica da riusare.
3. 🔴 **#1605 è CLOSED** (budget di pivot di `ADR-0008`), mentre §7 della review la dà come «da verificare».
4. 🔴 **La baseline «stessa destination ⇒ tutti `BLOCK`» è vera solo *a parità di precedenza*.** Esiste
   `FRTActionDef::Priority`, un dato di **catalogo dichiarato dall'azione** — numero più basso vince,
   `RTHexSimLibrary.cpp:679-701` — e il caso di parità è pinnato da
   `HexSim.ResolvePriorityTieStillContested`. Non è initiative score né UnitId priority: quelli sono
   vietati, e il divieto ha i suoi test (`Actions.Collisions.NoPlayerIdBias`,
   `HexSim.ResolveOrderIndependent`, `Movement.StepperIsDeterministicUnderPermutation`).
5. 🔴 **La domanda di `CM-02` è già registrata come `MOV-4`** in
   [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), con due uscite già istruite — *(a)* riformulare
   la clausola come «a parità di precedenza dichiarata» · *(b)* rimuovere la precedenza. E il costo di
   *(b)* è misurato: `ERTMoveOutcome::BlockedByPriority` è **serializzato nel TurnLog, formato v7**
   (`RTTurnLog.h:368`), quindi rimuoverla è una **migrazione di formato con rigenerazione del corpus
   golden** — la conseguenza già pagata da `D-245`. ⚠️ La variante **B** della review — *«stop
   all'ultima posizione sicura»* — **non è fra le uscite di `MOV-4`**: aggiungerla è un atto d'autore.
6. 🔴 **La review non conosce #1937 né #1936**, aperte il 2026-08-31: sono owner primari di `CM-01`.

### 6.3 Cosa il LIVE conferma

- L'ordine `Dash → Blast → Move` e la sua ragione: [`../../gameplay/spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md)
  §1 dichiara che *«il `Move` normale è l'ultima fase volontaria standard: sta dopo il Blast»*.
- La soglia dei **20 s** come *soglia d'allarme* con 3 unità armate:
  [`../../gameplay/spec-pacing-turno.md`](../../gameplay/spec-pacing-turno.md). Il Decision Log la
  corrobora dal lato del costo: `2 × MaxPromptsPerReaction × 3,0 s` = **18 s** in un turno.
- Il contest valutato nel `Cleanup` e la parità come pareggio dichiarato (`CP 10.3`):
  [`../../gameplay/spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md).
  ⚠️ `D-244` ha già deciso che **«stallo» è relativo alla board**: un anti-stall globale la
  contraddirebbe prima ancora di essere misurato.
- Un `FRTCellId` con **un solo** occupancy slot, e `D-270` che ammette l'ingombro interno come blocco di
  movimento e occlusione della LoS.
- La domanda di `CM-06` è già aperta come **`AUTHOR-DAMAGE-001`**, con due conflitti misurati contro
  `D-224` e `D-238`.

### 6.4 La mappatura — `REUSE` / `UPDATE` / `DECISION_GATE` / `PLAYTEST` / `DEFER`

| # | Esito | Owner reale | Nota |
|---|---|---|---|
| `CM-01` | `REUSE` | #25 · #172 · #173 · #1881 · #1937 · #1936 | La tripartizione `CERTAIN/PREDICTED/CONDITIONAL` è l'unico pezzo senza owner nominato |
| `CM-02` | `DECISION_GATE` + `PLAYTEST` | `MOV-4` | La decisione esiste; la variante B è un'uscita candidata, non scelta |
| `CM-03` | `DECISION_GATE` + `PLAYTEST` | `COV-9` · `COV-10` · #324 | Il «default consigliato» non ha owner: si misura, non si implementa |
| `CM-04` | `REUSE` + `PLAYTEST` | `spec-sequenza-turno.md` §1 · #172 · #173 | La rivalutazione della regola è `DEFER` |
| `CM-05` | `UPDATE` | #166 · #152 | L'aggiunta è una **metrica**, non una feature |
| `CM-06` | `DECISION_GATE` | `AUTHOR-DAMAGE-001` | Limitare la baseline è conseguenza della decisione, non sostituto |
| `CM-07` | `DEFER` | [`../../gameplay/spec-ownership-abilita-interazioni-sinergie.md`](../../gameplay/spec-ownership-abilita-interazioni-sinergie.md) | Guardrail di design, non deliverable |
| `CM-08` | `UPDATE` | #1861 · #1990 · `D-270` | Una riga di specifica di level design, non un sistema |
| `CM-09` | `PLAYTEST` | `spec-durata-partita-e-scala-mappe.md` · `D-244` | Nessun anti-stall prima dell'evidenza |
| `CM-10` | `DECISION_GATE` | #609 (`E38`) · [`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) | Prima l'audit degli assi esistenti, poi la domanda |

### 6.5 Criteri di decisione dei cinque scenari

| # | Scenario | Criterio |
|---|---|---|
| 1 | Collision Choke | Denial dominante o turni sprecati sistematici ⇒ `MOV-4` si istruisce con i dati. Altrimenti la policy resta, e la clausola si riformula come *(a)* |
| 2 | Dash → Blast → Move | Errore sistematico **dopo** che il playback mostra le tre posizioni ⇒ si rivaluta il costo della regola. Errore solo **prima** ⇒ è UI, e rientra in `CM-01` |
| 3 | Cover Choice | Override raro **e** ininfluente ⇒ la scelta manuale è un click: `COV-9`/`COV-10` si istruiscono in quel verso |
| 4 | Overwatch Pressure | `> 20 s` stabilmente ⇒ revisione di `ADR-0004` aperta **con i dati**, come già prescrive la DoD di #166 |
| 5 | Objective Deadlock | Congelamento dominante ⇒ il problema è dimostrato e la decisione si apre. Altrimenti `CM-09` si chiude senza intervento |

---

## 7. Work order originale, preservato

**Pre-flight** — aggiornare e misurare `origin/main`; leggere `AGENTS.md`, `CLAUDE.md`, Decision Log, ADR
e owner spec `CURRENT` dei domini toccati; cercare su GitHub `OPEN`/`CLOSED` e PR vive prima di creare
qualunque owner; riconciliare l'handoff contro lo stato corrente; verificare che `E1`–`E51` siano
allocate e **non** inventare `E52` per simmetria. ✅ Eseguito, §6.

**Deliverable A — Epic.** ✅ [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276),
trasversale, senza `E<n>`, che **collega** e non assorbe gli owner di `E11`/`E14`/`E23`/`E12`.

**Deliverable B — una sola issue iniziale.** ✅
[#2277](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2277), sub-issue nativa di #2276.

**Deliverable C — preservazione nel repository.** ✅ Questo file. Il **referto datato** che il work order
chiede *dopo* l'audit — `Finding → owner reale → stato → evidence necessaria → decisione/esito` — è la
§6.4 di questo stesso documento, unita alla §6.5: separarlo in un secondo file avrebbe creato due sedi
per un contenuto solo.

**Deliverable D — sincronizzazione Drive.** ✅ §9.

**Divieti** — non cambiare la collision policy sulla sola base di questa review · non creare un secondo
resolver, pathfinder, LOS, targeting, modello di `Facing` o di cover · non introdurre anti-stall sugli
obiettivi senza evidenza · non aggiungere assi strategici nuovi per colmare `CM-10` prima di auditare
quelli esistenti · non aprire una cascata di issue post-v0.1 · non trasformare un'inferenza di questo
handoff in **fatto**.

## 8. Criterio di chiusura della prima issue

La prima issue **non** è done quando implementa dieci fix. È done quando:

1. `CM-01`…`CM-10` hanno owner e stato verificati;
2. i duplicati sono stati evitati;
3. le vere decisioni aperte sono separate dai bug già posseduti;
4. i cinque scenari diagnostici hanno specifica riproducibile e metriche;
5. repository e Drive contengono abbastanza informazione da non richiedere la chat originale;
6. è indicata **una sola** prossima azione concreta, basata su evidenza e non su preferenza.

**La prossima azione dichiarata**: *scrivere lo scenario 1 — Collision Choke — come scenario riproducibile,
e misurarlo*. È il primo perché è l'unico che può falsificare una policy già spedita (`D-295`) e sbloccare
una decisione già aperta (`MOV-4`).

## 9. Sedi, e cosa possiede ciascuna

| Sede | Ruolo |
|---|---|
| Epic [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276) | il contenitore trasversale: findings, scenari, DoD, divieti |
| Issue [#2277](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2277) | l'audit operativo e il piano di playtest |
| Questo file | la preservazione della review e l'esito della riconciliazione |
| [Doc Drive](https://docs.google.com/document/d/1WCV5jZ9o7wLDueKKoALUyDhH-mGXHjAHEI5y9tshKKw/edit) | design intent e provenance della review |
| Doc Drive di reconciliation | gli ID GitHub reali e l'esito della riconciliazione, affiancato al precedente |

⚠️ **Il connettore Drive non modifica il corpo di un documento esistente**: può creare file e cambiare
titolo/cartella, non appendere testo. Per questo la sincronizzazione del §7-D prende la forma che il work
order stesso autorizza — *«aggiungere un nuovo documento di reconciliation con puntatore esplicito a
questo, senza fingere di aver aggiornato il precedente»*.

## 10. Nota di preservazione

Questo documento è deliberatamente autosufficiente rispetto alla conversazione che l'ha generato. Con
l'Epic #2276, la issue #2277 e questo file, la chat del 2026-09-04 può essere rimossa senza perdere le
conclusioni di design qui elencate.
