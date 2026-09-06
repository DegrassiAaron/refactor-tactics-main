# Bot Analysis & Consolidation — revisione del kit esterno

> `CURRENT` · **Referto di revisione**, non owner. Consuma
> `RefactorTactics_Bot_Analysis_Claude_Consolidation.md` (1087 righe, arrivato in radice il **2026-09-04
> 09:57** locali).
>
> **Data**: 2026-09-05 · **Base**: `origin/main` @ `4d6964a4` · **Worktree**:
> `.claude/worktrees/bot-consolidation-handoff` · **Modo**: verifica delle premesse + matrice di
> consolidamento.
>
> **Archiviato in**: [`../../archive/src/handoff/2026-09-05-bot-analysis-consolidation.md`](../../archive/src/handoff/2026-09-05-bot-analysis-consolidation.md)
>
> ⛔ **Nessuna scrittura su GitHub è stata eseguita.** Il kit chiede al §16 di *«consolidare i findings
> nelle issue live»* e al §25 di mutarle. In questo repository «consumare» significa **revisionare e
> archiviare**: gli atti su GitHub restano una decisione dell'autore, e il §7 di questo referto li elenca
> senza eseguirli.
>
> ⚠️ **Perché un worktree e non il branch corrente**: `fix/1793-2167-posa-e-offset` è **367 commit dietro**
> `origin/main`. Un referto che misura il codice bot da lì misurerebbe codice superato.

---

## 1. Il verdetto in una riga

> **L'ipotesi centrale del kit è vera e vale la pena di eseguirla — manca davvero un planner di squadra, e
> `Source/RefactorTactics/Bot/` non contiene una sola occorrenza di `TeamPlan`, `TopK`, `Overkill` o
> `MarginalUtility`. Ma tre delle nove debolezze che elenca non descrivono più il repository, e la più
> grossa era già chiusa: il kit è stato scritto alle 09:57 del 2026-09-04, e `CP 26.5` #2269 — «l'obiettivo
> entra nel punteggio del bot» — è stata chiusa alle 11:29Z dello stesso giorno.**

Il kit vale come **agenda architetturale**, non come misura dello stato. Due dei suoi findings sono
difetti correnti che **nessuna issue aperta copre** — e sono i due che il kit stesso classifica come
minori.

---

## 2. Il lavoro precedente, che il kit non cita

Il kit apre dicendo di verificare lo stato live, e poi non nomina nessuno dei referti già scritti su
questa lane:

| Referto | Data | Cosa aveva già misurato |
|---|---|---|
| [`bot-stall-spec-panel-2026-08-29.md`](bot-stall-spec-panel-2026-08-29.md) | 2026-08-29 | La catena `#1655 · #1551 · #1550`: due oracoli di parcheggio che rispondono in modo **opposto** alla stessa domanda, e `BOT-STALL-1` in `OPEN_DECISIONS.md` |
| [`dir-c-qa-scenario-bot-autobattle-handoff-2026-08-28.md`](dir-c-qa-scenario-bot-autobattle-handoff-2026-08-28.md) | 2026-08-28 | Lo stato delle lane QA/Bot/Autobattle su `HEAD`: 86 scenari, corpus golden, `RTBotStalemateProbeTests` |

⚠️ **Il §6 del kit chiede di verificare il «kiter panic» con scenari comportamentali** senza sapere che il
2026-08-29 un referto aveva già aperto la questione dei due oracoli divergenti sul parcheggio del bot —
materia adiacente, e la sua raccomandazione era di **non allinearli**, perché allinearli distruggerebbe la
prova che uno dei due porta.

---

## 3. Verifica delle premesse

Tutto misurato su `4d6964a4`, non ripreso dal kit.

| Affermazione del kit | Misura | Esito |
|---|---|---|
| §1.1 · Nessun planner di squadra: `PlanBots()` pianifica un'unità alla volta | `TeamPlan\|TopK\|Overkill\|MarginalUtility` in `Source/RefactorTactics/Bot/` → **zero** occorrenze | ✅ **Confermato** |
| §2.1 · `WThreat = 100` | `RTHexBotLibrary.h:200` | ✅ |
| §1.3 · §7 · L'obiettivo non è integrato nella utility corrente | `RTHexBotLibrary.h:264-309` — `WObjective = 120`, `WObjectiveFalloff = 30`, **due invarianti dichiarate**, `ScoreObjectiveTerm` a `.h:464` | 🔴 **Falsificato** |
| §1.6 · §6 · Guardia «panico» del kiter intorno a `NearestDistance <= Standoff / 2`, prima della utility | `RTTurnManager.cpp:1325` — `if (bKiter && Nearest && NearestDistance <= Standoff / 2)`, e il commento sopra dice *«Guardia del bot quadrato, conservata: **non passa dalla utility**»* | ✅ **Confermato alla lettera**, e il codice dichiara il bypass come intenzionale |
| §1.7 · §9 · `DecideReactionResponse()` sceglie il primo `FIRE:` legale | `RTHexBotLibrary.cpp:779-790` — ciclo su `AllowedResponses`, `return` al primo con `FireResponseTarget() != INDEX_NONE` | ✅ **Confermato** |
| §9 · «l'arming ha del punteggio, la scelta della risposta no» | `ScoreReaction` (`.cpp:809`) punteggia quale reazione **armare**; la scelta della risposta non lo usa | ✅ La distinzione del kit è esatta |
| §1.4 · §8 · Ricerca senza contatto primitiva, verso il centro mappa | `RTTurnManager.cpp:1104-1243`, `CP 13.5` — *«la condotta è la più povera che ristabilisce il contatto: avvicinarsi al CENTRO della mappa»* | ✅ |
| §7 · «manda entrambe le unità sullo stesso percorso di ricerca» | `RTTurnManager.cpp:1243` — *«puntano ENTRAMBE la cella più vicina al centro, che è una sola»* | ✅ **Il codice lo dichiara già** |
| §8 · «nessuna memoria delle aree già cercate» | `FRTLastKnownContact` (`CP 13.4`) esiste ed è usata a `.cpp:1157` | ⚠️ **Parziale**: la memoria del **contatto** c'è, quella dei **settori cercati** no |
| §10 · `D-095` `D-096` `D-097` `D-098` sono live | Decision Log righe **112-115**, tutte **Accettate** il 2026-08-11, owner `spec-bot-tattico.md` | ✅ Citazioni esatte |
| §19 · `#326` E26 Tactical Bot v1 | **OPEN** — `[EPIC v0.2]`, milestone `v0.2 · Struttura e finestre` | ✅ ma **v0.2**, non v0.1 |
| §19 · `#531` `#532` `#533` `#534` | tutte **OPEN**, tutte milestone `v0.2 · Struttura e finestre` | ✅ |
| §19 · `#149` | **OPEN**, milestone `v0.1 · Difetti e bilanciamento` | ✅ |
| §19 · `#327` = «E27 belief» | **OPEN** — titolo reale: *«E27 · Percezione completa: vista, udito, memoria»*, `v0.3` | ⚠️ Il belief è **§8 di `spec-bot-tattico.md`**, non il titolo dell'epic |
| §19 · `#328` = «E28 predictive» | **OPEN** — `#328` è *«E28 · Expert Bot v2»*; il predictive è **`#329` · E29 · Predictive avanzato** | 🔴 **Mappatura errata** |
| §24 · I dieci nomi di test proposti | `grep` su tutto l'albero → **0 su 10** esistono | ⚠️ Sono **proposte**, non pin: nessuno le assuma già presidiate |
| §24 · «reuse existing naming conventions if the repository has them» | I nomi vivi sono `RefactorTactics.HexBot.` (**37**) e `RefactorTactics.Bot.` (**29**). `Spec.Bot.` → **zero** | 🔴 Il prefisso che il kit propone non esiste, e nessun nome del repository comincia senza `RefactorTactics.` |

### 3.1 La premessa che era stale entro tre ore

| | |
|---|---|
| Kit scritto | **2026-09-04 09:57** locali (`07:57Z`) — `mtime` del file in radice |
| `#2269` `CP 26.5` chiusa | **2026-09-04 `11:29Z`** — `gh issue view 2269 --json closedAt` |
| Scarto | **3h 32m** |

`CP 26.5` si chiama *«L'obiettivo entra nel punteggio del bot: oggi non conosce la condizione di vittoria
del formato spedito»*. È **esattamente** il §7 del kit, ed è chiusa. Il codice che la implementa cita `#2269`
nel proprio commento (`RTHexBotLibrary.h:440`).

🔑 **Le due invarianti che quel lavoro ha lasciato sono più interessanti del finding**, e nessuna delle due
è nel kit:

- `WObjective < WKill` — *«un obiettivo non vale mai quanto un colpo letale»*, pinnata dall'esito
  `HexBot.ObjectiveNeverOutweighsAKill`;
- 🔴 `WObjectiveFalloff > WApproach` — dichiarata come **l'invariante che PUÒ fallire**: il termine tira
  verso l'obiettivo mentre `WApproach` tira verso il nemico, e sotto quella soglia il gradiente
  dell'obiettivo si annulla. Pinnata da `HexBot.ObjectivePullBeatsClosingOneCell`.

Questa è la stessa classe di problema che il §4 del kit descrive in astratto — «un nuovo termine scalare
non è necessariamente accordabile» — misurata su un termine **reale e già spedito**. Chi lavorerà `CP 26.1`
ha qui un precedente concreto invece di un argomento generale.

---

## 4. Matrice di consolidamento

Ogni finding del kit contro l'issue che lo possiede già.

| # | Finding del kit | Issue live | Stato | Classificazione |
|---|---|---|---|---|
| 1 | §3.1 · Nessun piano di squadra: unità pianificate in sequenza | [#531](https://github.com/DegrassiAaron/refactor-tactics-main/issues/531) `CP 26.1` | OPEN · v0.2 | **Già coperto** — nessuna issue nuova |
| 2 | §11 · Compatibilità temporale delle sinergie | [#532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/532) `CP 26.2` | OPEN · v0.2 | **Già coperto** |
| 3 | §21.C · Conflitti, ridondanza, risorsa contesa, overkill | [#533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/533) `CP 26.3` | OPEN · v0.2 | **Già coperto** |
| 4 | §21.D · Isteresi / ripianificazione stabile | [#534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/534) `CP 26.4` | OPEN · v0.2 | **Già coperto** |
| 5 | §7 · §21.E · Utilità dell'obiettivo | [#2269](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2269) `CP 26.5` | 🔴 **CLOSED** 2026-09-04 | **Obsoleto** — ⛔ non riaprire, non duplicare |
| 6 | §4 · §21.J · Scala della utility non accordabile | [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149) | OPEN · v0.1 | **Parziale** — il finding di scala sopravvive, gli scenari Guardian/carica no |
| 7 | §8 · §21.H · Ricerca senza contatto non coordinata | [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) `CP 13.5` · [#1902](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1902) | OPEN | **Parziale** — `#1902` è già *«nessun peso premia la visione»* |
| 8 | §5 · §21.I · Threat grossolana | — | — | **Mancante per scelta**: il kit stesso vieta il ticket vago. Serve **misura prima** |
| 9 | §6 · §21.F · Il kiter scavalca la utility | — | — | 🔴 **GAP REALE** — nessuna issue aperta lo nomina |
| 10 | §9 · §21.G · La reazione sceglie il primo `FIRE` legale | — | — | 🔴 **GAP REALE** — nessuna issue aperta lo nomina |
| 11 | §8 · Belief / stato nascosto probabilistico | [#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327) E27 | OPEN · v0.3 | **Già coperto** (titolo: *Percezione completa*) |
| 12 | §8 · Predizione dell'avversario | [#329](https://github.com/DegrassiAaron/refactor-tactics-main/issues/329) E29 — **non** `#328` | OPEN · v0.3 | **Già coperto**, con la mappatura del kit da correggere |

**Bilancio**: 12 findings · **7 già posseduti** · 1 obsoleto · 2 parziali · **2 gap reali**.

### 4.1 I due gap, misurati

**GAP 1 — `RTTurnManager.cpp:1325`.** La guardia del kiter è un `continue` prima del pool di candidate:
quando scatta, il bot **non costruisce nemmeno** le candidate d'attacco. Il commento la dichiara
intenzionale (*«Guardia del bot quadrato, conservata»*), quindi non è una svista — è una **policy v0.1 non
scritta in nessun owner**. La domanda che una issue deve porre non è «va corretta» ma: *quale documento
possiede questa scelta, e la v0.1 la vuole?* Se la risposta è che la vuole, il posto giusto è una riga in
`spec-bot-tattico.md`, non un ticket.

**GAP 2 — `RTHexBotLibrary.cpp:779`.** Il primo `FIRE:` legale, con l'ordine reso stabile da
`BuildOverwatchTriggers` che ordina per `UnitId` crescente. **Non è indeterminismo** — il commento lo
argomenta bene — ma è indifferenza al valore: fra due bersagli legali, quello con `UnitId` minore vince
sempre, anche se l'altro è letale. `ScoreReaction` esiste già a venti righe di distanza e sa misurare
`WThreat`: il materiale per decidere c'è, non viene consultato.

⚠️ **Entrambi i gap sono in v0.1**, che è `2v2 offline vs bot`: sono comportamenti che un giocatore vede
oggi, non in v0.2.

---

## 5. Cosa il kit sbaglia, e va detto prima di eseguirlo

| | |
|---|---|
| 🔴 **§7 «Objective blindness»** | Falso su `4d6964a4`. Eseguirlo riaprirebbe `CP 26.5` chiusa il giorno prima |
| 🔴 **§19 `#328` = E28 predictive** | `#328` è *Expert Bot v2*; il predictive è `#329` E29. Il §8 che assegna la predizione a «E28» va tradotto |
| ⚠️ **§8 «nessuna memoria»** | La memoria del contatto (`FRTLastKnownContact`, `CP 13.4`) esiste. Manca quella dei settori cercati |
| ⚠️ **§26 «ordine preferito di esecuzione»** | Mette `CP 26.1` al secondo posto, ma **E26 è v0.2** e i due gap reali sono **v0.1**. L'ordine del kit inverte le priorità di release |
| ⚠️ **§24 dieci nomi di test** | Zero esistono. Il kit dice *«reuse existing naming conventions»* senza misurarle: i nomi vivi sono `RefactorTactics.HexBot.` (37) e `RefactorTactics.Bot.` (29), e `Spec.Bot.` non compare mai |

⛔ **Non misurato, e il kit lo dà per scontato**: il §3.1 afferma che il planning sequenziale «impedisce»
il focus fire senza overkill. Non ho eseguito nessuno scenario che lo dimostri — è un'inferenza
dall'architettura, plausibile e non provata. `RTBotTeamPlanningTests.cpp` esiste ma misura **prenotazione
di rotta e lock-in**, non combinazione tattica: sei test, nessuno sull'overkill.

⚠️ **E un test si chiama già come il finding senza esserne la misura.**
`RefactorTactics.HexBot.ScoreFocusFire` (`RTHexBotTests.cpp:263`) confronta quattro punteggi di **una sola
unità** — attaccare batte non attaccare, forte batte debole, letale prende il bonus — su un `Ctx` con un
nemico solo. È la utility locale, non il focus fire di squadra: chi cerca copertura per il §13.1 del kit
la trova per nome e non per contenuto. **Il nome è occupato**, e questo vincola come scrivere il test
nuovo.

---

## 6. Verifiche

| Verifica | Esito |
|---|---|
| `git rev-parse HEAD` nel worktree = `origin/main` | ✅ `4d6964a4`, 0 commit di scarto |
| `diff -q` sorgente ↔ copia archiviata | ✅ **verbatim**, 1087 righe |
| `grep` dei simboli bot su `Source/` | ✅ 38 file, 365 occorrenze |
| `gh issue view` su 8 issue nominate dal kit | ✅ tutte risolte |
| Conteggio archivio riletto dal disco | ✅ `120` = 17+72+2+4+25 |

### NOT RUN

- ⛔ **Nessuna suite eseguita.** `./scripts/rt-suite.ps1` non è stato lanciato: questo è lavoro puramente
  documentale, e il write-set non tocca `Source/`.
- ⛔ **Nessuno scenario, nessuna PIE, nessun packaged.**
- ⛔ **Nessun `.uasset` toccato**, nessun Unreal Editor avviato.
- ⛔ **Nessuna scrittura su GitHub** — vedi §7.

---

## 7. Atti GitHub raccomandati — **elencati, non eseguiti**

Il kit li prescrive nel proprio testo. In questo repository restano una decisione dell'autore.

| # | Atto | Su | Perché |
|---|---|---|---|
| A1 | **Creare** una issue per il bypass del kiter (`RTTurnManager.cpp:1325`) | nuova, milestone v0.1 | Gap reale, comportamento visibile in v0.1, nessuna issue lo nomina |
| A2 | **Creare** una issue per la scelta della risposta di reazione (`RTHexBotLibrary.cpp:779`) | nuova, milestone v0.1 | Gap reale, `ScoreReaction` è già lì e non viene consultato |
| A3 | **Aggiungere** a `#531` i criteri d'accettazione mancanti che il §21.A elenca (focus fire senza overkill, diversità delle candidate, invarianza per permutazione, traccia diagnostica) | `#531` | Il kit li propone e `#531` va **esteso, non duplicato** |
| A4 | **Commentare** su `#149` che il finding di scala sopravvive e gli scenari Guardian/carica no | `#149` | Il §12 del kit lo chiede, e la issue è v0.1 aperta |
| A5 | **Nessun atto** su `#2269` | `#2269` | ⛔ chiusa il 2026-09-04: riaprirla sarebbe il difetto che questo referto esiste per prevenire |
| A6 | **Nessuna issue nuova** per team planning, sinergia temporale, conflitti, isteresi, belief, predictive | `#531` `#532` `#533` `#534` `#327` `#329` | Già posseduti |

⚠️ **A1 e A2 vanno decisi insieme a una domanda che non è mia**: entrambi i comportamenti sono *dichiarati
intenzionali nel codice*. Se la v0.1 li vuole così, l'atto giusto non è una issue — è una riga in
`spec-bot-tattico.md` che li possieda.

---

## 8. Rischi e aperti

- 🔴 **Il worktree è su `origin/main`, il branch dell'utente è 367 commit indietro.** Chi legge questo
  referto dal checkout condiviso vedrà codice diverso da quello misurato qui.
- ⚠️ **Il kit chiede al §26 di aprire E26**, ma E26 è milestone `v0.2` mentre la v0.1 non è chiusa. Il §26
  stesso dice *«roadmap gates override this order»*: il gate vince.
- ⚠️ **Il sorgente originale è ancora in radice nel checkout condiviso** (`D:\Repositories\refactor-tactict-dev\`).
  L'archiviazione qui è una copia verificata: l'originale va rimosso dopo il merge, non prima.
- ⛔ **Il §3.1 non è provato da uno scenario.** Se l'assenza di team planning va dimostrata prima di
  investire su `CP 26.1`, serve uno scenario di overkill che oggi non esiste.

---

## 9. Prossimo passo

**Una sola azione**: decidere su A1/A2 — se il bypass del kiter e la risposta di reazione sono difetti v0.1
o policy volute. Da quella risposta dipende se si aprono due issue o si scrive una riga nella spec owner.
