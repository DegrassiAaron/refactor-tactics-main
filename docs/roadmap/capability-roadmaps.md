# Capability Roadmaps — vista longitudinale v0.1 → v1.0

> 🔴 **Questo documento non possiede stato.** La source of truth per lo stato e lo scope corrente sono
> **le issue e le milestone GitHub**. Quando questo file e GitHub non concordano, **vince GitHub**.
>
> Questo documento non introduce una seconda scala di release, non assegna lavoro e non è owner di nessuna
> feature: è una **vista di navigazione** sopra owner che esistono già.

**Fotografia**: 2026-09-23, misurata su `origin/main` `3cda8ef5` e su GitHub LIVE (la precedente era il
2026-09-20 su `f7aa7b32`).
**Issue indice**: [#2325](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2325) — *[ROADMAP] Capability Roadmaps — vista longitudinale v0.1 → v1.0*. ⌫ **Fino al 2026-09-20 questa riga rimandava a una sezione che non esiste** (*«vedi § Dove vive questa vista su GitHub»*): `grep -c` la trovava una volta sola, cioè il rimando stesso, e il documento non nominava mai la propria issue indice.

> 🔴 **RIMISURATA UNA SECONDA VOLTA lo stesso 2026-09-23, e la certificazione di poche ore prima era
> gia' scaduta.** Due ancore chiuse nel frattempo e **mute**: **#2579** (`09:11Z`, riga `BAL-METRICS`) e
> **#2745** (`09:05Z`, riga `H` di §4). La prima l'ha chiusa **la sessione stessa** che aveva appena
> certificato questo file.
>
> ⚠️ **E il controllo di quella certificazione non le avrebbe viste.** Confrontava le citazioni che
> *portano* una parola di stato con GitHub e trovava zero divergenze — correttamente: il buco non e'
> nelle annotazioni **sbagliate**, e' in quelle **assenti**. Una convenzione che dice *«le chiuse si
> annotano, le aperte no»* si rompe dal lato del silenzio, ed e' il lato che nessuno guardava.
>
> ⌫ **`#2579` portava anche una descrizione falsa**, e nessun comando puo' accorgersene: diceva
> *«solo per eroe»*, ed e' chiusa **proprio perche' non lo e' piu'**.
>
> ✅ **Da questa passata il controllo e' un GATE, non una fotografia** — `#2325`:
>
> ```sh
> node tools/radar/anchor-state.ts --check     # esce 1 se un'ancora chiusa e' muta
> ```
>
> 🔑 **Falsificato prima di essere creduto**: girato sul documento com'era **prima** di questa passata
> esce `1` e nomina quelle due righe e **nessun'altra**; sul documento corretto esce `0`. Senza rete
> stampa `NOT RUN` ed esce `0` — mai un verde offline.
> ⛔ **Non sostituisce il comando qui sotto**: quello trova le annotazioni *sbagliate*, il gate quelle
> *assenti*. Sono due difetti, e si trovano in due modi.

> 🔁 **Rimisurata il 2026-09-23 contro GitHub LIVE, con lo stesso comando qui sotto.** Una sola ancora
> stantia: **#1805**, chiusa `COMPLETED` il **2026-09-21** — cioè **il giorno dopo** la rimisura che questa
> pagina certificava. Compariva in **tre** punti (la tabella di `CR-REPLAY`, i correlati di `CR-NET`, la
> riga `F` di §4), tutti e tre senza annotazione, e in una convenzione dove **le chiuse si annotano e le
> aperte no** un'ancora muta si legge come aperta. Annotati tutti e tre.
>
> 🔑 **È la SECONDA volta in tre giorni che questa certificazione scade, ed è la tesi che il documento già
> enuncia — ora con due misure invece di una.** Il 2026-09-20 durò ventidue minuti (#543); questa è durata
> tre giorni. ⛔ Non è un errore di chi ha misurato: una fotografia di stato **non è un gate**, e l'unico
> rimedio durevole resta il comando qui sotto, che va **eseguito**.
>
> ✅ **E la convenzione regge, il che è il risultato che vale la pena avere.** Verificate tutte le
> **76** issue citate: **49** aperte e **27** chiuse; delle citazioni che portano una parola di stato
> accanto, **zero** divergono da GitHub. Le uniche chiuse senza annotazione erano le tre di #1805 — più
> i casi in **prosa** che la nota qui sotto dichiara già legittimi (#1754, #472 come riferimento, le note
> di rimisura che parlano *delle* chiusure). ⚠️ Il metodo conta: la parola di stato si cerca **dopo** la
> citazione e fino alla successiva, perché un `(chiuse)` a fine elenco copre tutti i numeri che lo
> precedono — `#1626 · #1627 · #1629 · #1630 (chiuse)` è una riga corretta, e un controllo ingenuo ne
> segnalerebbe tre su quattro.

> 🔁 **Rimisurata il 2026-09-20 contro GitHub LIVE.** Le ancore stantie erano #2193, **#2556**, #782, #784,
> #2697, #2578 e #2629 — chiuse nei quattordici giorni dopo la fotografia del 2026-09-06 — più **#1496**,
> che a quella fotografia era **già** stantia: è chiusa dal **2026-09-02**, quattro giorni prima.
> ✅ **Nessuna ancora dichiarava aperto ciò che GitHub aveva chiuso, né il contrario.**
> ⌫ **E questa certificazione è scaduta in ventidue minuti — il che vale più della certificazione
> stessa.** Era vera quando è stata scritta (`13:50:05Z`); **#543** si è chiusa alle `14:12:37Z`, ed è
> citata in due punti di questo documento, ora annotati. 🔑 **Non è un errore di chi ha misurato**:
> è la dimostrazione che una fotografia di stato **non è un gate**, e che l'unico rimedio durevole è il
> comando qui sotto — che va **eseguito**, non letto.
>
> ⌫ **Questa nota è stata corretta il 2026-09-20 in tre punti, ed è istruttiva che ne avesse bisogno.**
> Diceva *«otto ancore su settantacinque erano stantie»* e poi ne **nominava sette**: mancava #2556, che lo
> stesso commit annota alla propria riga. Includeva **#1496** fra le chiusure *«dei quattordici giorni»*
> mentre il documento, poche righe più sotto, la data già al 2026-09-02 — la nota contraddiceva il corpo che
> certificava. E portava *«su settantacinque»*, un totale che cambia da solo appena il documento cita una
> issue in più: è successo **in questo stesso commit**, che ne aggiunge una aggiungendo il link all'issue
> indice. 🔑 Le ancore ora si **nominano** (`AGENTS.md` §14, forma 1): otto nomi non invecchiano.
>
> ⚠️ E la formula giusta è *«nessuna **ancora**»*, non *«nessuna riga»*: #1754 è chiusa e compare senza
> annotazione di stato, ma è citata in **prosa** come riferimento di codice (`RTScenarioKnowledge::OmniscientTeamId`),
> non come ancora di una vista — non dichiara nulla sul proprio stato.
>
> **Si rifà così**, e non serve fidarsi di questa pagina:
>
> ```sh
> # ogni issue citata, col suo stato vero
> grep -oE '#[0-9]{2,4}' docs/roadmap/capability-roadmaps.md | tr -d '#' | sort -un \
>   | xargs -I{} gh issue view {} --json number,state,stateReason,closedAt \
>       --jq '[(.number|tostring), .state, (.stateReason // "-"), (.closedAt[0:10] // "-")] | @tsv'
> ```
>
> 🔑 **E una chiusura `NOT_PLANNED` qui non significa «lavoro annullato».** #782 e #784 sono state
> chiuse così il 2026-09-10 con la stessa motivazione — *«chiusa come capitolo, non come lavoro»*: un
> **capitolo di milestone** non è un difetto che si apre e si chiude in un giro, e il suo indice è stato
> ripiegato nell'epic che lo possiede (**#773**). Il corpo resta leggibile e linkabile.
>
> ⚠️ Per una vista di navigazione la distinzione è **il punto**: leggere `CLOSED` e concludere *«fatto»*
> sbaglia in un verso, leggere `NOT_PLANNED` e concludere *«abbandonato»* sbaglia nell'altro. Il comando
> qui sopra chiede `stateReason` per questo, e la colonna va guardata.

---

## 1. Perché esiste

La roadmap di prodotto è **orizzontale**: `v0.1 → v0.2 → … → v1.0`, e risponde a *quando*.

Alcune domande però non sono orizzontali:

- «cosa manca ancora per un prodotto Replay utilizzabile?»
- «quanto è maturo il Tactical Designer?»
- «quale release fa avanzare la competenza del bot?»
- «dove si incontrano explainability e replay?»

Rispondere richiede di attraversare **più release** seguendo **una** capability. Questa vista fa quello, e
solo quello.

⛔ **Non fa** l'altra cosa che le somiglia: non è una seconda roadmap di release. Le maturità di capability
(`Replay 0.3`, `TD 0.5`, `PRESENT 0.2`) **non sono** versioni di prodotto e non vanno confrontate con `v0.3`.

---

## 2. Il modello di governance

| Livello | Possiede | Dove vive |
|---|---|---|
| **Release di prodotto** | *quando* — accettazione di release | Milestone GitHub |
| **Epic / system owner** | *cosa* si implementa | Issue `[EPIC]` |
| **Capability roadmap** | *come* una feature matura fra le release | Questo documento + issue indice |
| **Contratto / dipendenza** | *dove* le capability si incontrano | Link fra issue |

Tre regole che questo modello esiste per proteggere:

1. **Un solo primary owner per issue.** Le capability si intersecano; il lavoro no.
2. **Si linka, non si duplica.** Se una capability ha bisogno di qualcosa che un'altra possiede, la consuma
   con un link di dipendenza — non ne apre una copia.
3. **Nessun `E<n>` nuovo per organizzare una vista.** Gli identificatori `E<n>` hanno semantica di release e
   sono allocati fino a `E51`. Le epic trasversali del repository non ne hanno per scelta — #1881, #1937,
   #1105, #1861, #1990, #2276, #2388 — e questa vista segue la stessa convenzione.

---

## 3. Le capability

Ogni voce dichiara la sua **ancora**: l'issue che possiede la capability. L'ancora è il posto dove si legge
lo stato vero.

### CR-REPLAY — Replay & Inspection

**Ancora**: **#1881** *«Resolution Playback & Inspection — bot, replay e debug fino alla v1.0»* (cross-release)

L'invariante che questa capability non negozia:

```text
Resolver autorevole
  → TurnLog / Resolved Timeline canonica
       → Playback / Inspection core
            → Replay Viewer · Autobattle · TD / Debug
```

⛔ **Mai** `Playback → modifica/ricalcola Resolver`. Il playback cambia **come** e **quanto velocemente** si
guarda un risultato **già deciso**; non cambia stato logico, targeting, LOS, esito delle reazioni, `StateHash`
né TurnLog.

| Ruolo | Issue |
|---|---|
| superficie player-facing | #472 (chiusa il 2026-09-03) |
| indice delle partite | #416 (chiusa) |
| consumer autobattle | #952 |
| consumer Tactical Designer | #1625 |
| confine public/sanitized vs private audit | #1805 (chiusa il 2026-09-21) |
| seek per turno e fase | #415 (chiusa) |
| Player che non ricalcola | #470 (chiusa) |
| ponte Blueprint | #999 (chiusa) |
| micro-step indirizzabile | #1880 (chiusa) |
| pacing e controlli | #1878 · #1879 (chiuse) |

Maturità di capability: `Trace Core → Viewer MVP → Precise Inspection → Explain → Replay Product → Public/Audit split → Production`.

### CR-TD — Tactical Designer

**Ancora**: **#1105** *«Tactical Designer — un solo loop fra mappa, skill e scenario»* — `out_of_release_scope`

L'invariante: dati canonici e regole di gioco stanno **a monte**; l'editor è un consumer di pure query e DTO.
Se editor e runtime possono divergere, lo strumento ha smesso di essere una lente sul gioco.

| Sotto-vista | Owner |
|---|---|
| TD-MAP — authoring della mappa | #1861 *Map Editor 0.1* |
| TD-SCENARIO — authoring degli scenari | #1625 · #1628 (aperte) · #1626 · #1627 · #1629 · #1630 (chiuse) |
| TD-INSPECT — ispezione | consuma il playback core di **CR-REPLAY** |
| TD-LAB — laboratorio visivo | #1990 *Gray Kit Playground* |

⚠️ **Fuori dalla scala di release.** `GKP 0.1`, `TD 0.4` e simili sono maturità di tooling: non bloccano una
release di prodotto se nessun gate di release le consuma esplicitamente.

### CR-PRESENT — Tactical Presentation & Explainability

Questa capability ha **due owner distinti**, e non vanno fusi in una mega-epic.

| Asse | Owner |
|---|---|
| spaziale — camera e presentazione della mappa | **#1769** *E49 · Tactical Camera & Map Presentation* |
| semantico — eventi del giocatore | **#1937** *Player Event Log & Explainability* |

Pipeline di #1937, nell'ordine — l'ordine è il punto:

```text
Resolver → TurnLog canonico → predicato di autorizzazione → Player Event Projector → FRTPlayerEvent[] → UI
```

⛔ **Mai**: derivare eventi dal parsing di stringhe diagnostiche; far ricalcolare le regole alla
presentazione; **proiettare prima di sanificare**; esporre pianificazione privata per comporre un riassunto.

⚠️ L'occlusione di camera è **presentazione**, e non è la LOS di gioco.

### CR-BOT — Bot & Autonomous Play

Catena longitudinale già espressa dalle issue esistenti:

```text
#952 Autobattle (v0.1) → #326 Tactical Bot v1 (v0.2) → #327 percezione/belief (v0.3) → #328 Expert Bot v2 (v0.3)
```

L'autobattle è anche un **consumer di CR-REPLAY**: guarda una risoluzione, non ne produce una seconda.

### CR-MATCH — Match Flow / Turn Loop

Il ciclo che questa vista organizza:

```text
Planning → Ready → countdown annullabile → Commit affidabile → validazione d'autorità
        → Snapshot immutabile → risoluzione deterministica → Cleanup / Result
```

| Anello | Owner |
|---|---|
| Ready / Unready / countdown / soggetto del Ready / quorum | **#2193** — chiusa `COMPLETED` il 2026-09-09 |
| coordinamento del bot alleato prima del Ready | #534 (CP 26.4) — **post-v0.1** |
| protocollo ready/commit in rete | ~~#782~~ (`CP 40.4`) — chiusa **`NOT_PLANNED`** il 2026-09-10: capitolo **ripiegato nell'epic #773**, non lavoro annullato |
| Result e ritorno al menu | #940 (CP 46.5) |
| finestra di reazione e pacing | #166 · #314 · #319 |

✅ **Il confine d'autorità del countdown è deciso**: vive nella **presentazione**. `ReadyCountdownSeconds` sta
fra i *Tempi UX* di [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md)
§11, e #2193 è atterrata senza countdown in snapshot, `TurnLog` o `StateHash`.

⚠️ **Ready ha un owner solo, e resta #2193** — anche dopo l'estensione del 2026-09-07 (soggetto del Ready per
partecipante, quorum). La metà «coordinamento del bot alleato» **non** vive qui: è di #534, con
[`spec-bot-tattico.md`](../gameplay/spec-bot-tattico.md) §3 e `D-096`. Sono due owner di cose diverse, non due
owner della stessa.

### CR-NET — Online / Competitive Runtime

**Ancora**: **#773** *E40 · Il turno simultaneo in rete* (v0.5)

L'invariante di privacy, che vale già **oggi** e non dalla v0.5:

```text
CanonicalIntentStore = verità completa SOLO sul server
        ↓ proiezione autorizzata
Team Relay sanitizzato → preview solo agli alleati
        ↓
I client avversari non ricevono alcun payload di pianificazione privata
```

Nessun intento avversario in `GameState`, in `PlayerState`, su Actor `AlwaysRelevant`, né nel log pubblico
prima del momento autorizzato. Correlate: #759 (privacy temporale) · #1805 (public vs audit, chiusa il 2026-09-21) · #1466 · #1496 (chiusa il 2026-09-02).

### CR-CONTENT — Character & Ability Pipeline

**Ancora**: **#774** *E41 · GAS come runtime delle abilità, mai come autorità* (v0.6)

```text
dati canonici Hero/Action → variante/loadout → Skill Workbench → valutazione scenario
        → runtime (GAS come runtime, non come autorità) → batch balance → contenuto validato
```

ID stabili, versioni esplicite, validator. Le varianti sono **trade-off**, non upgrade puri.
In v0.1 l'ability system è `UPrimaryDataAsset` e **GAS è fuori scope**.

### CR-SHELL — Application Shell & Navigation

**Ancora**: **#934** *E46 · Frontend shell e ciclo di partita* (v0.1)

```text
Main Menu ── Play ── Vs Bot
         ├─ Replays ── Match History ── Replay Viewer
         ├─ Settings
         └─ Quit
Result ── Play Again · Main Menu
```

| Schermata | Owner |
|---|---|
| navigator e root | #936 (chiusa) |
| Main Menu | #938 (chiusa) |
| Play | #939 (chiusa) |
| Result | #940 |
| Pause | #941 (chiusa) |
| Replay Viewer / Match History | **#472** (chiusa) — la shell **naviga**, non reimplementa |

---

### CR-VERT — Verticalità tattica

**Ancora**: **#2388** *«Verticalità tattica — Ledge, Fall e Forced Movement»* (cross-release)

✅ **Release: v0.1**, da [`D-332`](../decisions/RT_PDR_00_Decision_Log.md) (2026-09-05), che chiude `REL-3`.
⚠️ *Questa nota diceva «è l'unica capability di questa vista che non ha ancora una release», ed era vera per
un giorno.* L'owner semantico delle regole è [`../gameplay/spec-caduta-e-bordi.md`](../gameplay/spec-caduta-e-bordi.md);
questa vista **linka e non forka**, come le altre otto.

La catena che questa capability non negozia:

```text
spostamento forzato
  → bordo aperto attraversato
       → displacement orizzontale TERMINATO
            → risoluzione della caduta (effetti, sempre)
                 → esito di atterraggio  →  posizione finale
```

⛔ **Mai** il verso opposto — «prima decido dove atterra, poi vedo se la caduta è avvenuta»: gli effetti non
dipendono dalla disponibilità della cella finale, ed è il punto che rende il fallback saturo leggibile invece
che un difetto.

| Confine | Owner reale |
|---|---|
| bordo, muri, porte, interaction graph | **#324** · E23 |
| authoring e validazione della mappa | **#1861** · Map Editor 0.1 |
| resolver dello spostamento | `ApplyDisplacements` · `HexKnockbackDestination` |
| traccia, replay, seek | **CR-REPLAY** · #1881 |
| leggibilità dell'esito | **CR-PRESENT** · #1937 |

Maturità di capability: `Contratto → Runtime → Traccia → Authoring validato → Leggibilità`.
⚠️ Sono maturità di **capability**, non versioni di prodotto: non si confrontano con `v0.2`.

---

### CR-BALANCE — Balance & Tuning

**Ancora**: **#2565** *«Bilanciamento: dalla variante d'abilità al contenuto validato»* (cross-release) — `out_of_release_scope`
**Dettaglio**: [`roadmap-balance.md`](roadmap-balance.md)

Il ciclo che questa capability chiude, e che oggi è **aperto in due punti**:

```text
catalogo canonico → variante sperimentale → validazione → metriche derivate
     → esecuzione deterministica sui sistemi REALI → diff baseline↔variante
          → evidenza (TurnLog + StateHash) → promozione esplicita al dato canonico
```

L'invariante: ⛔ **nessun secondo simulatore**. Se lo strumento dice 28 e la partita dice 24, lo strumento ha
smesso di essere una lente — ed è motivo di arresto, non un numero da tarare. È `ADR-0010` più il §3 di
[`../technical/tooling/spec-tactical-designer.md`](../technical/tooling/spec-tactical-designer.md).

| Sotto-vista | Owner |
|---|---|
| BAL-DATA — i numeri canonici | [`../balance/`](../balance/) (`D-023`) · gate `tools/radar/catalog-code.ts` · #2578 (chiusa il 2026-09-18: le azioni sono scoperte) |
| BAL-VARIANT — la variante sperimentale | #1950 *Skill Workbench* (`TD 0.3`) — dato consegnato · #2577 (manca l'ingresso) |
| BAL-DIFF — il confronto fra due run | #2576 (`TD 0.4`, dichiarato dall'owner e senza issue fino a oggi) |
| BAL-METRICS — le metriche derivate | `tools/radar/{rubric,power,precision,profile,balance}.ts` (`D-108`) · #2579 (chiusa il 2026-09-23) — ⌫ *diceva «solo per eroe», ed è chiusa proprio perché non lo è più: `abilityContributions` espone il contributo **per abilità** e `powerRaw` somma quello. La riga descriveva il difetto che la issue è andata a togliere* |
| BAL-BATCH — la misura a lotti | **#776** (`E43`) — 🔴 dopo il competence gate `D-102` (~~#543~~, chiusa `COMPLETED` il 2026-09-20) |
| BAL-RUNTIME — il runtime d'abilità | consuma **CR-CONTENT** · #774 (`E41`) |
| BAL-GATE — il gate umano | #403 (`BAL-1`, `U20/PIE-BAL1`) |

Maturità di capability: `BAL 0.1 … BAL 1.0`, tracciate come figlie di #2565.
⚠️ Sono maturità di **capability**, non versioni di prodotto: `BAL 0.7` non ha niente a che vedere con `v0.7`.
È la stessa distinzione che `D-154` ha già dovuto fare per `TD 0.7`, dopo che il 2026-08-13 una milestone
*«Skill Balance Lab v0.3»* fu proposta e dichiarata superata — *«uno strumento collocato nella roadmap di
release compete con la consegna»*.

⛔ Il vocabolario è **`Variant`**, non `Candidate`: `D-154` ha misurato che `Candidate` è già occupato in
cinque header con due significati.
⛔ `RefactorTactics_Balance_Matrices_v0.1.xlsx` resta `RESEARCH` (`D-023`): materiale d'analisi, mai fonte per
risolvere un conflitto contro i cataloghi `.md`.

---

### CR-WATCH — La partita che si guarda

**Ancora**: **#952** *«E47 · Mini v0.1 Autobattle — la partita che si guarda»* (v0.1)
**Dettaglio**: [`plans/autobattle-showcase-long-term-roadmap-2026-09-09.md`](plans/autobattle-showcase-long-term-roadmap-2026-09-09.md)

⚠️ **Aggiunta il 2026-09-09**, misurata su `origin/main` `c3151afd`. La fotografia dichiarata in testa a
questo documento resta quella del 2026-09-06: **questa sezione non la rimisura**, e nessun'altra riga qui
sotto è stata riletta in quell'occasione.

Non è `CR-BOT` con un altro nome. `CR-BOT` segue la **competenza** di chi gioca da solo; questa segue la
**leggibilità** di ciò che sta accadendo — sono due domande, e il 2026-09-09 la seconda non aveva una vista.

L'invariante, ed è tutto ciò che questa capability protegge:

```text
#952 non possiede il gioco: lo CONSUMA.
     HUD · Bot · Camera · Presentation · Objectives · Playback
     restano owner dei propri sistemi.
```

⛔ **Mai** una mega-epic «Showcase» che assorba metà repository. ⛔ **Mai** un secondo Scenario Harness per
configurare una demo. ⛔ **Mai** uno spettatore che allenti la privacy: l'autorizzazione è un **dato**
(`ARTTurnManager::IsUnattendedSession()`, `D-242`), non una guardia ammorbidita.

Le nove tappe d'esperienza e il loro owner reale — ⚠️ **lettere di questo documento, non milestone GitHub**
(`D-145`):

| Tappa | Outcome | Owner reale | Release |
|---|---|---|---|
| `A` watchable | la partita gira e si guarda | #952 (i `CP 47.x` sono chiusi) · **#2744** | v0.1 |
| `B` readable | capisco *perché* è successo | #1937 → #1936 · #2697 (chiusa il 2026-09-12) · #2281 · #613 | v0.1 |
| `C` useful playtest | ci si può giudicare il gameplay | #2556 · #2629 (chiuse il 2026-09-09 e il 2026-09-18) · #2477 · #326 | v0.1 → v0.2 |
| `D` spectator / camera | guardo da spettatore | **#1769** · #1781 (`CAM-12`) | v0.1 parziale (`D-286`) |
| `E` match story | so chi sta vincendo, e perché | #2281 · #331 · #332 | v0.1 → v0.4 |
| `F` replay / inspection | studio la partita | **#1881** · #472 (chiusa) · #2411 · #1805 (chiusa) | v0.1 → v1.0 |
| `G` presentation | comincia a sembrare un gioco | #286 · #217 · #2453 | v0.1 → v0.2 |
| `H` showcase / video | configuro una demo e la ripeto | **#2745** (discovery, chiusa il 2026-09-23) | post-v0.1 |
| `I` representative match | mostra il gioco futuro, non l'arena | #325 · #221 · #333 · #331 · #332 | post-v0.1 |

⚠️ **`F` non è a valle delle altre**: cammina in parallelo dalla v0.1 ed è la tappa più avanzata di tutte.
Le uniche dipendenze vere sono `A → B` (parziale), `H → A·D·E` e `I → E·G`; il resto è preferenza d'ordine.

Decisione aperta che questa capability porta con sé: **`OBS-1`** in
[`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) — se l'osservatore in partita diventi una **posizione
nominata** come già è nello Scenario Harness (`RTScenarioKnowledge::OmniscientTeamId`, #1754), o resti la
squadra del `PlayerController` con un ramo per la sessione non presidiata.

---

## 4. Dove le capability si incontrano

I contratti sono il posto in cui una capability **consuma** un'altra senza riscriverla.

| Intersezione | Contratto condiviso |
|---|---|
| Replay × Tactical Designer | TurnLog canonico · Resolved Timeline · Replay ViewModel · seek · identità del micro-step |
| Replay × Explainability | proiezione `FRTPlayerEvent` · payload di motivazione · navigazione al punto della timeline |
| Tactical Designer × Map Editor | dati canonici di mappa · stessa semantica runtime · stessa validazione |
| Tactical Designer × Content | Skill Workbench edita varianti, **Scenario Harness** le valuta — non un secondo valutatore |
| Balance × Tactical Designer | `BAL 0.1` **consuma** `TD 0.3` e `TD 0.4` e non li rivendica: il workbench è l'ingresso, il bilanciamento è la domanda |
| Balance × Content | i cataloghi Markdown possiedono i numeri (`D-023`); GAS sarà il **runtime** delle abilità, mai l'autorità (#774) |
| Balance × Bot | un risultato bot-contro-bot **non è ancora** una misura di bilanciamento: `D-102` (~~#543~~, chiusa `COMPLETED` il 2026-09-20) precede ogni lotto |
| Balance × Online | la misura non può vedere l'intento avversario prima della risoluzione — il turno è simultaneo |
| Tactical Designer × Bot | gli scenari diventano test di competenza e suite di regressione |
| Presentation × TD / Replay / Playground | **un solo** linguaggio semantico di overlay: selezionato · valido · invalido · previsto · confermato · incerto · copertura · hazard |
| Online × Replay | replay pubblico sanitizzato e audit trace privata sono **due prodotti** sugli stessi dati canonici |
| Online × Explainability | la proiezione avviene **dopo** l'autorizzazione dell'osservatore, mai prima |
| Shell × Replay | Main Menu e Result navigano verso lo stesso viewer di #472 |
| Verticalità × Map Editor | stessa qualificazione del bordo in authoring e a runtime — una sola sede, mai due |
| Verticalità × Replay | l’esito di atterraggio è un valore serializzato: si aggiunge **in coda**, non si riordina |
| Verticalità × Explainability | la caduta è avvenuta anche quando la posizione finale non lo mostra — il log lo deve dire |
| Watch × Explainability | l'autorizzazione dell'osservatore è **un dato** (`IsUnattendedSession()`), e vale per il §4.2 come per il §4.1: due filtri divergenti sono il debito che `D-242` ha chiuso |
| Watch × Camera | l'autobattle è un **consumer** della camera, mai una seconda camera: il Camera Director resta #1781 e non anticipa il core |
| Watch × Replay | guardare live e ristudiare passano dallo **stesso** playback core (#1881) — nessun secondo viewer, nessun resolver dentro il viewer |
| Watch × Online | uno spettatore locale che vede entrambe le squadre è ammesso; in rete (#773, `E40`) è un client che **possiede** quei piani — o è lato server, o non esiste (~~#784~~ — chiusa `NOT_PLANNED` il 2026-09-10, capitolo in #773; la procedura vive in [`../technical/systems/procedura-canary-anti-leak.md`](../technical/systems/procedura-canary-anti-leak.md)) |

---

## 5. Cosa questo documento non è

- ⛔ Non è una seconda roadmap di release.
- ⛔ Non è lo stato delle feature: quello vive nelle issue e nelle milestone.
- ⛔ Non è l'owner di nessuna specifica. Le spec dettagliate restano nei loro owner — #1881, #1105, #1769,
  #1937, #1861 e i documenti di sistema. Qui si **linka**, non si forka.
- ⛔ Non introduce label o identificatori nuovi.

Una percentuale di issue chiuse **non è** una misura di maturità di prodotto: il peso di una issue e di un
gate non è lo stesso. Se serve un numero, si dichiari come conteggio strutturale — `33 chiuse / 14 aperte` —
e non come «70% completo».

---

## 6. Documenti di roadmap correlati

| Documento | Ruolo |
|---|---|
| [`roadmap-v0.1-v1.0.md`](roadmap-v0.1-v1.0.md) | vista di navigazione sulle dieci release |
| [`roadmap-main-v0.1.md`](roadmap-main-v0.1.md) | vista di esecuzione della v0.1 |
| [`roadmap-post-v0.1.md`](roadmap-post-v0.1.md) | scope di release post-v0.1 |
| [`roadmap-gray-kit-playground.md`](roadmap-gray-kit-playground.md) | maturità di tooling, fuori dalla scala di release |
| [`roadmap-balance.md`](roadmap-balance.md) | dettaglio di `CR-BALANCE`: `BAL 0.1 → BAL 1.0`, fuori dalla scala di release |
| [`hex-map-roadmap.md`](hex-map-roadmap.md) | registro storico di ciò che è stato consegnato |
| [`roadmap-editor.md`](roadmap-editor.md) | vista storica ritirata |

Nessuno di questi, incluso questo file, è autorevole sullo stato corrente.
