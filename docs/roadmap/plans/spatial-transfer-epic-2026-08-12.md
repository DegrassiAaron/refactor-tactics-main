# Referto — Spatial Transfer, epic e serie di checkpoint

> `HISTORICAL` · **Referto di triage e spec panel**, non una fonte. · **Data**: 2026-08-12 · **Base**: `dda87f1a`
> **Sorgente esaminato**: `RefactorTactics_SpatialTransfer_Epic_Claude_2026-08-12.md`, archiviato in
> [`../../archive/src/handoff/2026-08-12-spatial-transfer-epic.md`](../../archive/src/handoff/2026-08-12-spatial-transfer-epic.md).
>
> **Cosa possiede**: il verdetto sezione per sezione, le misure che lo sostengono e la revisione di qualità
> della specifica.
> **Cosa non possiede**: nessuna regola. La regola vive in
> [`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md); le decisioni
> in [D-118](../../decisions/RT_PDR_00_Decision_Log.md) e
> [D-119](../../decisions/RT_PDR_00_Decision_Log.md); il piano in
> [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E39.
>
> **È il secondo handoff sul trasferimento nella stessa giornata.** Il primo —
> [`teleport-instant-movement-2026-08-12.md`](teleport-instant-movement-2026-08-12.md) — ha chiuso con due
> domande aperte e **una sola issue**. Questo le chiude e apre l'epic.

## 1. Il verdetto in una riga

Il kit è **accurato**, e questa è la notizia: quattro handoff su cinque arrivano con almeno una premessa di
stato scaduta, questo ha misurato il repository prima di scrivere e ci ha preso — inclusi i nove stati di
issue che elenca e il numero di epic libero.

Il suo difetto non è ciò che afferma. È **quanto propone di aprire in una volta**: tredici checkpoint di cui
otto la cui prima riga sarebbe *«serve un consumatore che non esiste»*. Quattro sono stati aperti, gli altri
vivono nel corpo dell'epic.

## 2. Le misure che decidono il triage

Nessuna viene dal kit; tutte prese su `dda87f1a`.

| # | Misura | Comando / riferimento | Esito |
|---|---|---|---|
| **M1** | `ERTMovementStyle` ha **sei** valori e `LinearLeap` è fra questi | `RTActionDef.h:185` | il kit li elenca **corretti**, incluso `LinearPass` |
| **M2** | La semantica del trasferimento **esiste**: `Result.Entered = { destinazione }` | `RTMovementActionLibrary.cpp` | confermata, ed è il fondamento di tutto il resto |
| **M3** | Il test che la dimostra è **verde** | `RefactorTactics.Actions.Leap.IgnoresIntermediateCells` | confermato |
| **M4** | Lo scenario esiste, ed è `BLOCKED` di proposito | `Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json` | `requires: ["Teleport"]`, Flux 90 HP, due celle `Fire`, istruzioni di completamento dentro |
| **M5** | 🔴 **E39 è libera** | ultima epic assegnata: **E38** = [#609](https://github.com/DegrassiAaron/refactor-tactics-main/issues/609); E37 chiusa | il kit **ordina di non hardcodarla prima dell'audit**, e aveva ragione a ordinarlo: il repository ha già pagato una collisione su `E21` e **tredici** su `D-nnn` |
| **M6** | Le **nove** issue di dipendenza hanno gli stati che il kit dichiara | `#645 #605 #436 #165 #159` OPEN · `#307 #308 #146 #425` CLOSED | **9 su 9 corrette** |
| **M7** | `Action.Anchor` e `Reaction.Anchor` esistono con semantica di resistenza | [D-094](../../decisions/RT_PDR_00_Decision_Log.md) | la collisione di nome che il kit segnala al §19 è **reale** |
| **M8** | La milestone **v0.2** esiste ed è usata | `v0.2 · Struttura e finestre`, 22 issue aperte | ⚠️ ma **E38 non la usa**: `#609` e i suoi checkpoint hanno milestone `null` |
| **M9** | Nessuna feature del registry possiede il dominio | `RT-FEAT-ACTION-MOVE-PROFILES` · `-DASH-DISPLACEMENT` · `-MOVEMENT-COMPAT` | nessuna delle tre: la prima ospitava lo scenario **in prestito** |

## 3. Audit GitHub, come richiesto dal §21

Eseguito su open **e** closed, prima di creare qualunque cosa.

```text
Search performed (repo-scoped, open + closed), issue < #700:
  blink              -> #645
  teleport           -> #645
  spatial transfer   -> #645
  recall             -> 0
  portal             -> 0
  forced teleport    -> 0
  swap               -> #31 (CLOSED, "Allestimento della partita" — omonimia, non il dominio)

Epic esistenti con [EPIC] in titolo: 40 (31 OPEN, 9 CLOSED). Massimo assegnato E38.
```

**Una sola issue possedeva qualcosa di questo dominio, ed è #645.** Non è stata duplicata né assorbita
nell'epic: possiede il ramo che **già esiste** e non ha consumatore, mentre l'epic costruisce la famiglia.
Il rapporto è di **precedenza, non di contenimento** — annotato con un commento sulla issue.

## 4. Spec panel — la qualità della specifica

Modalità `critique`. Il panel giudica il **sorgente**, non ciò che il repository ne ha fatto.

**KARL WIEGERS** — qualità dei requisiti
❌ **CRITICO**: la Definition of Done del §8 è una lista di **quattordici sostantivi** — *«targeting /
visibility policy»*, *«bot»*, *«determinism»* — non di criteri verificabili. Nessuna riga dice come si
constata il soddisfacimento.
📝 Convertita: ogni voce è diventata un checkpoint con un DoD misurabile, e la voce *«determinism»* è
diventata la sola cosa che si può misurare davvero — **permutation invariance** e stabilità dell'hash.
🎯 Priorità alta: senza questo l'epic non ha un gate di chiusura, solo un elenco di argomenti.

**GOJKO ADZIC** — testabilità
✅ **Il punto più forte del kit.** §12 chiede esplicitamente una **verifica di mutazione** e ordina di
*«dichiarare prima quale test deve fallire»*. È la disciplina che il repository ha già adottato, e vederla
arrivare dal sorgente invece che dal canone è raro.
⚠️ **MAGGIORE, però**: le sei liste di nomi di test del §15 sono **proposte**, non specifiche. Il Feature
Registry di questo progetto **verifica che ogni pattern dichiarato matchi un test reale**, quindi copiarle in
`tests:` avrebbe rotto il validator. Sono rimaste dove appartengono — nel corpo delle issue, come nomi
indicativi.

**MARTIN FOWLER** — confini e design
✅ **Corretto e insolito**: §9.2 chiede una **primitive pura**, e specifica *«non Actor/subsystem»*. §20
elenca nove anti-pattern per nome, incluso `TeleportPerceptionSystem` e `TeleportReactionManager`.
📝 Quell'elenco è stato promosso da consiglio a **divieto**, dentro [D-118](../../decisions/RT_PDR_00_Decision_Log.md):
un anti-pattern in un handoff archiviato non vincola nessuno, in una decisione sì.

**ALISTAIR COCKBURN** — attore e obiettivo
❌ **La domanda che il kit non fa mai**: *chi* si teletrasporta. Tredici checkpoint descrivono un sistema
completo di trasferimento e **nessuno nomina un eroe**. Nel canone di questo progetto
([D-029](../../decisions/RT_PDR_00_Decision_Log.md)) un'abilità ha **un solo owner**, e senza quello il Blink
è una capacità del motore — cioè esattamente il difetto che [#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645)
documenta per `Action.Leap` e [#425](https://github.com/DegrassiAaron/refactor-tactics-main/issues/425) per
altre tre.
📝 Registrato nel *Non fa* dell'epic: assegnare il Blink è **contenuto**, e ricade sul kit del suo owner.

**MICHAEL NYGARD** — modi di fallimento
⚠️ **MEDIO**: §9.2 propone `same destination → FailAll` come baseline e la chiama *«raccomandata»*, ma non
dice cosa succede al **piano** di chi fallisce — l'azione è consumata? il turno è perso? Un fallimento
silenzioso su una risorsa contesa è il tipo di regola che si scopre al playtest.
📝 Nella issue è scritto che se il progetto volesse la **priorità** invece del `FailAll`, quella è una
decisione da registrare in `OPEN_DECISIONS.md`, non da scegliere in implementazione.

**LISA CRISPIN** — copertura ed edge case
✅ La matrice §15 copre sei aree (pure, runtime, reaction, replay, privacy, portal, swap).
⚠️ **Manca l'unico caso che il repository sa già essere difficile**: un trasferimento **verso una cella che
un'altra unità sta per liberare** nello stesso turno. È il caso che `Movement.Collision` risolve per il
`Traversal` e che nel `Transfer` non ha un `Prog` con cui essere ordinato.

**Sintesi del panel** — punteggi indicativi, non misure:

| Dimensione | Valutazione | Motivo in una riga |
|---|---|---|
| Accuratezza dello stato | **alta** | 9 stati di issue su 9, l'enum, lo scenario, il numero di epic: tutto verificato e corretto |
| Testabilità | **alta** | chiede la verifica di mutazione e la dichiara prima |
| Criteri di accettazione | **bassa** | il §8 elenca argomenti, non condizioni |
| Confini architetturali | **alta** | nomina nove anti-pattern e vieta il secondo motore |
| Attore e ownership | **assente** | nessun eroe, in un progetto dove un'abilità ha un owner |
| Dimensionamento | **bassa** | 13 checkpoint aperti insieme, 8 senza consumatore |

## 5. Verdetto sezione per sezione

| Sezione del kit | Esito | Nota |
|---|---|---|
| §0 regola operativa | ✅ **eseguita** | audit prima, worktree, issue open+closed, riuso |
| §1 stato già misurato | ✅ **riverificato e corretto** | M1–M4 |
| §2 problema architetturale | ✅ **confermato** | `ARTTurnManager` riscrive il transfer in `Path = [Origin, Destination]` per `ResolveHexPaths`, che è a micro-step. Con due nodi degenera: **non è sbagliato oggi**, lo diventa con due trasferimenti nello stesso turno |
| §3 owner documentale | ✅ **rispettato** | nessuna seconda tassonomia. L'owner esistente è stato **esteso**: la colonna «Teleport» è diventata `Transfer`, e la partizione `Traversal`/`Transfer` è §1 |
| §4 decisioni aperte | ✅ **chiuse**, non aggirate | `MOV-1` → D-118 · `MOV-2` → D-119, decise **dall'autore in sessione**. Nessuna «decision issue» equivalente aperta, come il kit ordina |
| §5 riuso di #645 | ✅ **non duplicata** | commento di aggiornamento, nessuna sub-issue: il rapporto è di precedenza |
| §6 precedenti da non duplicare | ✅ **collegati** | `#307 #308 #146 #425`, tutti citati nell'epic con il **perché non lo possiedono** |
| §7 epic proposta | ✅ **creata**, numero verificato | **E39** = [#704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/704). Il kit vieta di hardcodare `E39` prima dell'audit, e l'audit gli ha dato ragione |
| §8 DoD dell'epic | ⚠️ **riscritta** | vedi panel/Wiegers: quattordici sostantivi diventati checkpoint con criteri |
| §9 tredici checkpoint | ✂️ **quattro aperti, uno già chiuso, otto rinviati** | vedi §6 |
| §10 dipendenze GitHub | ✅ **tutte collegate** | 9 su 9, con lo stato verificato |
| §11 ordine di implementazione | ✅ **adottato** | `39.2 → 39.3 → 39.4 → 39.12` è il percorso critico, scritto nella roadmap e nell'epic |
| §12 epic close gate | ✅ **recepito** | 23 voci nel corpo dell'epic, con `MOV-1`/`MOV-2` già spuntate |
| §13 roadmap/registry/milestone | ✅ **eseguito** | E39 in `roadmap-post-v0.1.md`, feature `RT-FEAT-ACTION-SPATIAL-TRANSFER`, milestone **v0.2** |
| §14 scenario map e capability | ✅ **rispettato alla lettera** | `Teleport` **non** aggiunta all'allowlist dell'harness. La regola che il kit cita è quella giusta, ed è ora in D-119 |
| §15 test matrix | ⚠️ **non dichiarata nel registry** | il validator verifica che ogni pattern matchi un test reale; i nomi restano nelle issue |
| §16 mutation test | ✅ **recepito**, ed è il punto migliore del kit | scritto nel DoD di #700 e #702 |
| §17 privacy | ✅ **già canone**, ⚠️ con un simbolo inesistente | *«nessun planning avversario al client»* è `AGENTS.md`, e il kit lo riafferma correttamente. Ma dei due riusi che propone **uno solo esiste**: `FilterForTeam` è `URTIntentPrivacyLibrary::FilterForTeam`, mentre `CanonicalIntentStore` ha **zero occorrenze in `Source/`** — è un termine di design che vive in una dozzina di documenti d'archivio e che il registry già segnala come non-codice su `RT-FEAT-NET-PRIVATE-PLANNING`. Trovato dalla code review, non dall'istruttoria: citarlo in backtick accanto a un simbolo vero lo fa sembrare reperibile |
| §18 portal e GraphRevision | ✅ **recepito e spostato** | il portale è **grafo**, non transfer: la regola è finita in `spec-mappa-multilivello.md`, dove vive l'enum |
| §19 collisione `Anchor` | ✅ **verificata e vera** | M7. Scritta nel CP 39.9 |
| §20 errori da evitare | ✅ **promossi a divieto** | dentro D-118, non solo nel corpo dell'epic |
| §21 issue creation policy | ✅ **applicata** | ogni issue creata dichiara *perché nessuna esistente possiede il delta*, con una tabella |
| §22 output finale | ✅ | §8 di questo referto |

## 6. I nove checkpoint che non sono diventati issue

Il kit ne propone tredici. **Quattro** sono stati aperti — il percorso critico che porta lo scenario da
`BLOCKED` a verde. **Uno** era già chiuso prima di cominciare. **Otto** restano nel corpo dell'epic.

Il criterio non è la prudenza: è lo stesso che il referto del mattino aveva già applicato, ed è
**verificabile**. Una issue è aperta quando esiste un delta che qualcuno può cominciare; `39.5` (reazione),
`39.6` (rumore), `39.7` (UI/bot), `39.8`–`39.11` (Swap, Recall, Portal, forced transfer) e `39.13` hanno
tutte la stessa prima riga: *serve un trasferimento che il turno risolva*, che è `39.4`.

⚠️ **`39.1` non ha una issue, ed è una scelta da dichiarare**: il suo intero contenuto era documentale ed è
atterrato in questo commit. Aprire e chiudere una issue nello stesso momento avrebbe prodotto tracciabilità
finta. Il rimando è a D-118/D-119, che sono l'artefatto vero.

## 7. Cosa è cambiato

| File | Cosa |
|---|---|
| `docs/decisions/RT_PDR_00_Decision_Log.md` | **D-118** (`MOV-1` → famiglia propria) · **D-119** (`MOV-2` → v0.2, E39) |
| `docs/OPEN_DECISIONS.md` | `MOV-1` e `MOV-2` chiuse, con l'istruttoria conservata sotto l'esito |
| `docs/gameplay/spec-tassonomia-movimento.md` | §1 partizione `Traversal`/`Transfer` · §2 matrice: colonna «Teleport» → **`Transfer`**, `stato nel codice` da «assente» a `LinearLeap` · §2.1 nuova · §7 riscritta |
| `docs/technical/spec-mappa-multilivello.md` | il `Portal` è un valore di `ERTHexTransitionKind`, non un transfer |
| `docs/CONTEXT_INDEX.md` | la riga della tassonomia dice la partizione, non l'elenco vecchio |
| `docs/roadmap/roadmap-post-v0.1.md` | **E39** in v0.2, con i tredici checkpoint e l'ordine |
| `docs/roadmap/feature-registry.yaml` | nuova `RT-FEAT-ACTION-SPATIAL-TRANSFER` (`SPECIFIED`) · lo scenario Teleport **spostato** lì da `RT-FEAT-ACTION-MOVE-PROFILES`, dove stava in prestito |
| `docs/roadmap/plans/teleport-instant-movement-2026-08-12.md` | puntatore in avanti: le sue due domande aperte sono chiuse |
| viste generate | `feature-registry.json`, `project-graph.json`, `featuremap.shortlist.md` — **rigenerate, mai editate** |

**Non toccati, e la ragione**: il codice (nessuna riga di runtime — era il DoD di 39.1) · gli scenari (quello
che serve esiste, e si accende in 39.12) · l'allowlist dell'harness (§14 del kit, ora D-119) · il workbook di
bilanciamento (vietato da [`balance/README.md`](../../balance/README.md)) · `tools/radar/` (nessun catalogo
toccato, quindi gli SVG non divergono).

## 8. Tabella finale, come richiesta dal §22

| Tipo | ID | Stato iniziale | Azione | Stato finale |
|---|---|---|---|---|
| Open Decision | `MOV-1` | OPEN | decisa dall'autore in sessione | ✅ **D-118** — famiglia propria |
| Open Decision | `MOV-2` | OPEN | decisa dall'autore in sessione | ✅ **D-119** — post-v0.1, v0.2 |
| Issue | [#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645) | OPEN | commento di aggiornamento, **non** duplicata né assorbita | OPEN, invariata nella sostanza |
| Epic | **E39** | mancante (max E38) | creata dopo audit del numero libero | [#704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/704) OPEN, milestone v0.2 |
| Issue | CP 39.1 | proposta | **già chiusa** da D-118/D-119, nessuna issue creata | chiusa nell'epic |
| Issue | CP 39.2 | proposta | creata | [#700](https://github.com/DegrassiAaron/refactor-tactics-main/issues/700) OPEN 🥇 |
| Issue | CP 39.3 | proposta | creata | [#701](https://github.com/DegrassiAaron/refactor-tactics-main/issues/701) OPEN |
| Issue | CP 39.4 | proposta | creata | [#702](https://github.com/DegrassiAaron/refactor-tactics-main/issues/702) OPEN |
| Issue | CP 39.12 | proposta | creata | [#703](https://github.com/DegrassiAaron/refactor-tactics-main/issues/703) OPEN |
| Issue | CP 39.5–39.11, 39.13 | proposte | **rinviate**, nel corpo dell'epic | 8 non aperte |
| Scenario | `TeleportSkipsIntermediateCells` | BLOCKED | **non toccato**; owner spostato alla feature nuova | BLOCKED — si accende in #703 |
| Feature Registry | `RT-FEAT-ACTION-SPATIAL-TRANSFER` | inesistente | creata, `spec: done` e il resto `todo` | `SPECIFIED` |
| Feature Registry | `RT-FEAT-ACTION-MOVE-PROFILES` | `RELEASE_READY` | scenario in prestito rimosso, note aggiornate | `RELEASE_READY`, invariata |
| Milestone | v0.2 | 22 issue | +5 | 27 issue |

```text
NEW ISSUES CREATED:        5   (1 epic + 4 checkpoint)
EXISTING ISSUES UPDATED:   1   (#645, commento)
CLOSED ISSUES REFERENCED:  4   (#307 #308 #146 #425)
OPEN ISSUES LINKED:        4   (#605 #436 #165 #159)
DECISIONS REGISTERED:      2   (D-118, D-119)
SCENARIOS CREATED:         0
SCENARIOS UNBLOCKED:       0   (si sblocca in #703, non prima)
TESTS CREATED:             0   (nessuna riga di runtime: era il DoD di 39.1)
MILESTONES UPDATED:        1   (v0.2)
```

```text
NEXT ISSUE:
#700 — CP 39.2 · Il resolver puro dei trasferimenti e i conflitti simultanei
```

⚠️ **Ed è una issue nuova, non una esistente**, contro la preferenza del §22 del kit. La ragione è misurata:
l'unica issue preesistente del dominio è `#645`, che possiede una **domanda di contenuto** (un eroe prende
`Action.Leap`, o è un ramo morto?) e non il resolver. Nessuna issue possedeva il primo passo.

## 9. Cosa questo referto non ha fatto

- **Non ha scritto una riga di runtime.** Era il DoD esplicito di 39.1, ed è la ragione per cui la feature
  nasce `SPECIFIED` con nove gate su dieci a `todo`.
- **Non ha aggiunto `Teleport` alla capability allowlist dell'harness.** §14 del kit lo vieta finché il gioco
  non sa produrre un trasferimento, e ora è D-119.
- **Non ha dichiarato i nove scenari del corpus finale come `planned`.** Si dichiarano quando il loro
  checkpoint si apre: un corpus anticipato gonfia il conteggio della scenario map senza che nulla lo
  verifichi — difetto già pagato tre volte in un giorno rimisurando quei numeri.
- **Non ha assegnato il Blink a un eroe.** È contenuto, ha un owner, e nessuno l'ha deciso.
- **Non ha scelto fra «valore in coda a `ERTMovementStyle`» e «asse separato».** D-118 stabilisce che quella
  scelta è una migrazione di formato serializzato e appartiene a #701, con un test a due binari.
