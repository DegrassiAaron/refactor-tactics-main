# Decision Time Bank — conflict report

> `CURRENT` · **Stato**: audit chiuso, **conflitti risolti dall'autore il 2026-08-09** (vedi §9)
> **Audit eseguito su**: `f1b2038` · **Registrazioni scritte su**: `75eb0f3` — nell'intervallo nessun owner auditato è cambiato
> **Sorgente auditato**: `RefactorTactics_Decision_Time_Bank_Claude_Consolidation_2026-08-09.md`
> (copia in `todo/consolidazione-chat-openai/`)
> **Scopo**: eseguire lo Step 2 richiesto dal kit — classificare ogni affermazione dell'handoff contro le
> source of truth reali — **prima** di qualunque modifica a documenti normativi, registry o roadmap.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR accettato,
> prevale l'ADR e la proposta si **registra**, non si applica.

---

## 1. Perimetro dell'audit

Consultati a HEAD:

| Documento | Ruolo nell'audit |
|---|---|
| [`decisions/adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) | owner canonico delle finestre di reazione, con gli emendamenti D-021, D-047, D-048 |
| [`gameplay/spec-reaction-clash-e14.md`](../../gameplay/spec-reaction-clash-e14.md) | owner dell'estensione *contested* (E14.7) |
| [`roadmap/roadmap-v0.1.md`](../roadmap-v0.1.md) · [`roadmap/v0.1-issue-plan.md`](../v0.1-issue-plan.md) | epic E14 = `#152`, CP 14.1–14.6 = `#161`–`#166`; CP 14.7 e 14.8 **senza issue** |
| `roadmap/feature-registry.yaml` | FeatureId reali |
| [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) | decisioni aperte e conflitti già registrati |
| [`product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) | scope MVP |
| [`technical/architettura-codice.md`](../../technical/architecture/architettura-codice.md) | stato di implementazione di E14 |

**Non** consultati: issue GitHub reali (`gh` non invocato in questa sessione),
[`technical/spec-turnlog.md`](../../technical/architecture/spec-turnlog.md) oltre la citazione come owner dei reason code.
Entrambi restano da verificare prima di toccare TurnLog o numeri di issue — vedi §9.

---

## 2. Sintesi

| Classificazione | Voci | Significato |
|---|---|---|
| `CURRENT` | 6 | l'handoff riporta correttamente il canone |
| `PROPOSED` | 7 | idea nuova, nessun conflitto: si registra |
| `CONFLICT` | 4 | l'handoff contraddice una decisione accettata |
| `STALE` | 3 | l'handoff usa una formulazione superata |
| `DUPLICATE` | 2 | ridefinisce qualcosa che ha già un owner |

**Esito**: il concetto di Time Bank **non esiste** in nessuna forma nel repository — nessun conflitto di
esistenza. I quattro `CONFLICT` non riguardano l'idea, riguardano **come l'handoff la aggancia** al modello
di reazione già deciso.

---

## 3. Matrice

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Fonte che prevale | Stato | Azione |
|---|---|---|---|---|---|---|
| 1 | Esistenza di un Time Bank | feature nuova da consolidare | nessuna occorrenza in `docs/`, `Source/`, `Content/` | — | `PROPOSED` | registrare come decisione, non come recepimento |
| 2 | `FastReactionDuration = 3,0 s` | «baseline nota da confrontare» | **3,0 s**, baseline di sistema per **ogni** Fast Reaction | [ADR-0004 §8](../../decisions/adr-0004-finestre-di-reazione.md) | `CURRENT` | — |
| 3 | `Timeout Overwatch = HOLD` | riportato correttamente | funzione **pura** dello stato; mai `FIRE` | [ADR-0004 §3](../../decisions/adr-0004-finestre-di-reazione.md) | `CURRENT` | — |
| 4 | `Overwatch charge = 1` | riportato | `Charges = 1` | [ADR-0004 §8](../../decisions/adr-0004-finestre-di-reazione.md) | `CURRENT` | — |
| 5 | Macro-fasi e Move dopo Blast | riportato | invariato | [ADR-0003 §1](../../decisions/adr-0003-modello-azioni-v01.md) | `CURRENT` | — |
| 6 | Formula di determinismo | riportata quasi alla lettera | invariante #4 + TurnLog come dato | canone · [ADR-0004 §3](../../decisions/adr-0004-finestre-di-reazione.md) | `CURRENT` | — |
| 7 | Trigger simultanei | riportato: «una singola multi-target opportunity» | idem, **mai** prompt in sequenza | [ADR-0004 §4](../../decisions/adr-0004-finestre-di-reazione.md) | `CURRENT` | — |
| 8 | **Bank pubblico live** (§16) | `PROPOSED / REQUIRES PRIVACY REVIEW`, cinque opzioni A–E aperte | la privacy review **è già stata fatta**: il ritmo osservato non deve dipendere dal tempo di risposta altrui | [D-021 · ADR-0004 §7-bis](../../decisions/adr-0004-finestre-di-reazione.md) | **`CONFLICT`** | le opzioni `C` (public live) ed `E` (bucketed) si scartano in sede di spec: quantizzare un delta correlato al tempo di lock non chiude il canale |
| 9 | **Bank nel Reaction Clash** (§15) | «consumano il bank personale dei due giocatori», reveal non discusso | il **reveal è a scadenza fissa**: la finestra dura *sempre* 3,0 s e il momento del lock **non è osservabile** | [D-048 · `spec-reaction-clash-e14.md` §7.1](../../gameplay/spec-reaction-clash-e14.md) | **`CONFLICT`** | il bank converte l'istante di lock in una risorsa persistente e archiviata: peggiora il canale che §7.1 chiude. Ammesso solo `owner-only` + nessun delta in DTO avversario |
| 10 | **Costo del timeout** (§6, §10) | il timeout allo scadere costa `MaxWindow − Grace`, cioè **il massimo possibile** | «il timeout è una funzione pura dello stato […] **un mancato input non deve spenderla**», detto di una risorsa irreversibile | [ADR-0004 §3](../../decisions/adr-0004-finestre-di-reazione.md) | **`CONFLICT`** | il bank **è** una risorsa: far pagare al timeout la tariffa massima è esattamente ciò che §3 vieta. Aggravante a M10: disconnessione o lag → timeout → bank a zero → grace ridotta → altri timeout. Serve un `TimeoutBankCost` fisso e ridotto, più una regola di non-drenaggio su disconnessione conclamata |
| 11 | **Cap aggregato** (§7, §19) | «`MaxPromptsPerReaction` + Time Bank» come protezione combinata | il cap aggregato è **assente per decisione** (D20), rischio dichiarato, misura rimandata a CP 14.5 — *mai eseguita* | [ADR-0004 §8](../../decisions/adr-0004-finestre-di-reazione.md) · [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | **`CONFLICT`** | il Time Bank **è** un cap aggregato in tempo. Introdurlo prima della misura consuma in anticipo una decisione rimandata apposta, e ignora i due rientri già valutati (cap per turno · `MaxPromptsPerReaction = 1`), entrambi *parametri* |
| 12 | `MaxPromptsPerReaction` | «data-driven, baseline storica proposta = 3» | **3**, data-driven, valore corrente | [ADR-0004 §8](../../decisions/adr-0004-finestre-di-reazione.md) | `STALE` | non è «storica proposta»: è il valore in vigore |
| 13 | Finestre simultanee (§14) | presentata come problema nuovo da risolvere | risolto per lo stesso responder ([ADR-0004 §4](../../decisions/adr-0004-finestre-di-reazione.md)) e per due responder contested ([D-048](../../gameplay/spec-reaction-clash-e14.md) §8: un boundary = **un** prompt) | ADR-0004 · D-048 | `STALE` | riformulare come estensione al caso di *N responder indipendenti*, unico residuo genuino |
| 14 | E14 e `#152` | «da verificare a HEAD» | E14 = `#152`; CP 14.1–14.6 = `#161`–`#166`; **CP 14.7** esiste (Reaction Clash) e l'handoff non lo conosce | [`v0.1-issue-plan.md`](../v0.1-issue-plan.md) | `STALE` | agganciare a E14 come CP nuovo **dopo** 14.7, non come sottoalbero parallelo |
| 15 | `MaxDecisionWindowDuration = 3,0 s` | nuovo parametro | è `FastReactionDuration` | [ADR-0004 §8](../../decisions/adr-0004-finestre-di-reazione.md) | `DUPLICATE` | un solo nome. Il precedente esiste già: riga 5 di [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) (`FastDecisionDuration` → `FastReactionDuration`) |
| 16 | `DecisionTimingPolicy` (§29) | ruleset nuovo, contribuisce a `ResolverConfigHash` | la configurazione competitiva ha già i suoi contenitori (`URTMatchFormatData`, ResolverConfig) | canone · CP 10.3 | `DUPLICATE` | non introdurre un secondo sistema di configurazione. Vedi anche §6 di questo report |
| 17 | Grace window | nuova | nessun concetto equivalente | — | `PROPOSED` | — |
| 18 | Bank exhaustion + `ExhaustedResponseGrace` | nuovo | — | — | `PROPOSED` | — |
| 19 | Telemetria di pacing | nuove metriche | esistono `ResolutionPlaybackSeconds` e `ReactionDecisionSeconds` (metrica **separata**, per costruzione) | [ADR-0004 §Revisione](../../decisions/adr-0004-finestre-di-reazione.md) | `PROPOSED` | le nuove metriche si **aggiungono** a `ReactionDecisionSeconds`, non lo sostituiscono |
| 20 | Refill per turno | variante futura | — | — | `PROPOSED` | resta fuori dal primo consolidamento, come chiede l'handoff stesso |
| 21 | Policy di latenza | «verificare se esiste, altrimenti issue» | **non esiste**; la seconda revisione di ADR-0004 è prevista a **M10** | [ADR-0004 §Revisione](../../decisions/adr-0004-finestre-di-reazione.md) | `PROPOSED` | la mitigazione dichiarata dal kit (TIMEBANK-03) non mitiga: senza sottrazione dell'RTT il bank misura la connessione |
| 22 | Portata di release | §19 calcola `30 s × 6 players` su un 3v3 | v0.1 = **offline 2v2 vs bot**: un solo giocatore umano; 3v3 è baseline **non decisa** (D-011) | [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) · [D-011](../../decisions/RT_PDR_00_Decision_Log.md) | `PROPOSED` | il valore competitivo del bank si materializza a **M10**. In v0.1 resterebbe un anti-AFK timer — ciò che §37 del kit dichiara di non voler essere |

---

## 4. Classificazione dei punti richiesti dallo Step 2

Il kit chiede di cercare sei formulazioni specifiche. Esito:

| Formulazione cercata | Presente a HEAD | Stato |
|---|---|---|
| `5s interrupt` | solo in righe che si dichiarano storiche | `STALE` — già registrato come `SUPERSEDED`, riga 4 di [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) |
| `3s Fast Reaction` | sì, come baseline di sistema | `CURRENT` |
| `per-window timer only` | sì: è il modello in vigore | `CURRENT` — il bank non lo sostituisce, lo affianca |
| `future per-player bank` | **nessuna traccia** | `PROPOSED` — nessuna aspettativa pregressa da onorare o smentire |
| `reaction-specific timers` | nessuno: `FastReactionDuration` è unico e di sistema | `CURRENT` — il guardrail «un solo bank, non uno per abilità» è già soddisfatto dall'architettura |
| `public/private timing` | sì, deciso in senso restrittivo | `CURRENT` — [D-021](../../decisions/adr-0004-finestre-di-reazione.md) §7-bis, [D-048](../../gameplay/spec-reaction-clash-e14.md) §7 |

---

## 5. FeatureId — reali contro citati

Il kit §23 elenca nove FeatureId come «già note da verificare». Verifica su
`feature-registry.yaml`:

| FeatureId citato | Esiste | Nota |
|---|---|---|
| `RT-FEAT-CORE-DECISION-BOUNDARY` | ✅ | — |
| `RT-FEAT-REACTION-OPPORTUNITY` | ✅ | — |
| `RT-FEAT-REACTION-FAST` | ✅ | — |
| `RT-FEAT-REACTION-OVERWATCH` | ✅ | — |
| `RT-FEAT-REACTION-FAST-ACTION` | ✅ | — |
| `RT-FEAT-MATCH-PACING` | ✅ | — |
| `RT-FEAT-REACTION-MULTI-TRIGGER` | ❌ | **non esiste** |
| `RT-FEAT-REACTION-SIMULTANEOUS` | ❌ | **non esiste** |
| `RT-FEAT-REACTION-PRIVACY` | ❌ | **non esiste** |

Esistono e il kit **non** li nomina: `RT-FEAT-REACTION-PREPARED`, `RT-FEAT-REACTION-PROFILE`,
`RT-FEAT-REACTION-CLASH`.

⚠️ **Rischio operativo**: la formulazione «feature già note» invita a crearle per riallineamento. Creare quei
tre ID violerebbe il guardrail §36 del kit stesso e i tre temi corrispondenti sono **già coperti**: multi-trigger
da [ADR-0004 §4](../../decisions/adr-0004-finestre-di-reazione.md) sotto `RT-FEAT-REACTION-OPPORTUNITY`,
simultaneous da `RT-FEAT-REACTION-CLASH`, privacy dall'invariante #6 e da `RT-FEAT-REACTION-CLASH`.

---

## 6. Numeri — stato di ciascun valore

| Parametro | Valore nel kit | Stato reale | Nota |
|---|---|---|---|
| `FastReactionDuration` | 3,0 s (come `MaxDecisionWindowDuration`) | **`CANONICAL`** | non duplicare il nome |
| `MaxPromptsPerReaction` | 3 | **`CANONICAL`** | data-driven |
| `DefaultTimeoutBehavior` | `HOLD` | **`CANONICAL`** | — |
| `InitialDecisionTimeBank` | 30,0 s | `PROPOSED` | numero non ancorato: vedi nota sotto |
| `GracePerDecisionWindow` | 1,0 s | `PROPOSED` | manca il criterio di promozione |
| `ExhaustedResponseGrace` | 0,75 s (oppure 0) | `PROPOSED` | due alternative aperte |
| `BankRefill` | 0 in MVP | `PROPOSED` | variante futura dichiarata |
| Cap aggregato di finestre | — | **`CANONICAL`: nessuno** (D20) | il bank lo introdurrebbe di fatto |
| Soglia d'allarme resolution | non citata dal kit | **`CANONICAL`: 20 s** | ⚠️ **mai misurata** — CP 14.5 |

> **Nota sui 30 s.** È l'unico numero del kit privo di ancoraggio. Il repository ha già un parametro di formato
> che scala col match: `RoundLimit` da `URTMatchFormatData` (10–14 in 2v2, 16–20 in 3v3, riga 18 di
> [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md)). Derivare il bank invece di inventarlo —
> `InitialBank = RoundLimit × (MaxWindow − Grace)` — dà 24 s in 2v2 e 32–40 s in 3v3, scala da sé col formato
> ed elimina un numero magico. Proposta registrata nella spec, non applicata qui.

---

## 7. Righe candidate per i registri normativi

Da **non** scrivere finché l'autore non conferma. Elencate perché il conflitto sia registrabile e non implicito.

### 7.1 `DOC_CONFLICT_MATRIX.md` — ✅ scritte

| # | Area | Stato registrato |
|---|---|---|
| 53 | Budget di decisione aggregato | `SUPERSEDED` — D20 sostituita prima della misura, rischio dichiarato; i rientri di ADR-0004 restano validi |
| 54 | Costo del timeout su una risorsa | `CONFIRMED` — lettura **stretta** di ADR-0004 §3: il divieto copre le risorse di abilità, non il budget temporale |
| 55 | Visibilità del tempo di decisione | `CONFIRMED` — `owner-only`; la proposta di bank pubblico è registrata **e scartata** |

### 7.2 `OPEN_DECISIONS.md` — ✅ scritta

Una sola voce, in *Aperte — livello regole*: **«Con quali valori si tara il Decision Time Bank?»**. Non chiede
più *se* costruirlo — è deciso — ma la taratura, con i criteri di uscita della spec §3.2 e la prima misura a
CP 14.6. `TB-5` e `TB-7` restano nella spec §17 e **non si duplicano** nel registro.

---

## 8. Cosa **non** è un conflitto

Registrato per evitare che un audit successivo li riapra:

- **Il bank non è un secondo sistema di reazioni.** Vive fuori dal trigger, che resta puro: consuma solo al
  *commit*, che [ADR-0004 §2](../../decisions/adr-0004-finestre-di-reazione.md) dichiara già «passo distinto e
  successivo». L'invariante #3 non viene toccato.
- **Il bank non allunga la Fast Reaction.** Il kit lo vieta esplicitamente (§6) e la regola coincide col canone.
- **Un solo bank per giocatore, non uno per abilità** (§4 del kit): coerente con `FastReactionDuration` unico
  di sistema.
- **Il timeout server-authoritative** (§11) è già la regola: [ADR-0004 §7-bis](../../decisions/adr-0004-finestre-di-reazione.md),
  riga «Autorità».

---

## 9. Esito — decisioni dell'autore, 2026-08-09

La matrice §3 registra i conflitti **come rilevati**. Non va riscritta: qui sotto come sono stati risolti, lo
stesso giorno, dall'autore. Owner dell'esito: [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md).

| Riga | Conflitto | Esito | Nota |
|---|---|---|---|
| 8 | bank pubblico live | **risolto come proposto**: `owner-only`, opzioni `C`/`E` scartate | D-021 prevale, nessuna riapertura di ADR-0004 |
| 9 | bank nel Reaction Clash | **risolto**: il bank drena, la wall-clock resta 3,0 s | limite dichiarato in §1.1 della spec, non nascosto |
| 10 | costo del timeout | **risolto in senso opposto alla raccomandata**: costa `MaxWindow − Grace`, il massimo | ⚠️ vedi §9.1 |
| 11 | cap aggregato prima della misura | **rischio accettato**: il bank entra in v0.1 come CP 14.8, senza gate | D20 sostituita consapevolmente; i rientri di ADR-0004 restano disponibili e compatibili |
| 22 | portata di release | **v0.1**, non M10 | l'asimmetria (un solo umano) è dichiarata in §9 della spec; il bot riceve un bank per non introdurre un ramo `IsBot` |

Chiuse anche, **per derivazione e non per scelta**:

- `ResolverConfigHash` **non esiste** nel codice: la domanda «il bank contribuisce all'hash?» era mal posta.
  `RTMatchFormatData.h` §14 separa già parametri di **regola** da **tempi di parete**, e il bank è un tempo di
  parete — sta accanto a `PlanningSeconds`. Riga 16 di §3 (`DUPLICATE`) si chiude qui.
- `InitialBank` è **derivato** da `RoundLimit × (MaxWindow − Grace)`: 24 s in 2v2, 32–40 s in 3v3. L'unico
  numero magico del kit sparisce.

### 9.1 La riga 10 non si chiude in silenzio

Il conflitto rilevato leggeva ADR-0004 §3 — *«un mancato input non deve spenderla»* — come valido per ogni
risorsa. È un'**estensione analogica**: il soggetto letterale di quella riga è la **charge** dell'Overwatch.

La decisione dell'autore adotta la lettura stretta: il divieto copre le risorse di **abilità**, non il budget
temporale. È legittima e non contraddice l'ADR, ma **non deve restare implicita**:

> Azione richiesta: registrare la precisazione in [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) e nel
> Decision Log — decisione (d) di §16 della spec. Chi leggerà ADR-0004 §3 fra sei mesi deve trovare scritto
> quale delle due letture vale.

Resta vincolante la conseguenza pratica: il costo pieno è equo **solo** se il fallback è preselezionato e
raggiungibile entro la grace (§4.2 della spec), e quel tempo va **misurato**, non assunto.

---

## 10. Limiti dichiarati

1. **Issue GitHub non interrogate.** I numeri `#152`, `#161`–`#166` vengono da
   [`v0.1-issue-plan.md`](../v0.1-issue-plan.md), non da `gh`. Prima di scrivere numeri in registry o roadmap
   vanno confermati sul remoto.
2. **`spec-turnlog.md` non letto in dettaglio.** I nomi di evento e i reason code proposti dal kit §13 non sono
   stati confrontati con la taxonomy reale: quel file è l'owner e nessun nome va ipotizzato senza averlo aperto.
3. **Nessuna misura eseguita.** Questo report non misura la durata della resolution: è esattamente il dato
   mancante che rende prematuro il consolidamento, e la sua raccolta è CP 14.5.
4. **Nessun file normativo modificato.** Il report è un artefatto di analisi: registry, roadmap, ADR, Wiki e
   matrice dei conflitti sono invariati.
