# Capability Roadmaps — vista longitudinale v0.1 → v1.0

> 🔴 **Questo documento non possiede stato.** La source of truth per lo stato e lo scope corrente sono
> **le issue e le milestone GitHub**. Quando questo file e GitHub non concordano, **vince GitHub**.
>
> Questo documento non introduce una seconda scala di release, non assegna lavoro e non è owner di nessuna
> feature: è una **vista di navigazione** sopra owner che esistono già.

**Fotografia**: 2026-09-04, misurata su `origin/main` `070796a1` e su GitHub LIVE.
**Issue indice**: vedi § *Dove vive questa vista su GitHub*.

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
   #1105, #1861, #1990, #2276 — e questa vista segue la stessa convenzione.

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
| confine public/sanitized vs private audit | #1805 |
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
| Ready / Unready / countdown | **#2193** |
| Result e ritorno al menu | #940 (CP 46.5) |
| finestra di reazione e pacing | #166 · #314 · #319 |

⚠️ Il countdown di #2193 tocca un confine d'autorità: va deciso **prima** se viva nella presentazione o nella
simulazione — nel secondo caso entra nello snapshot, quindi nel replay.

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
prima del momento autorizzato. Correlate: #759 (privacy temporale) · #1805 (public vs audit) · #1466 · #1496.

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

## 4. Dove le capability si incontrano

I contratti sono il posto in cui una capability **consuma** un'altra senza riscriverla.

| Intersezione | Contratto condiviso |
|---|---|
| Replay × Tactical Designer | TurnLog canonico · Resolved Timeline · Replay ViewModel · seek · identità del micro-step |
| Replay × Explainability | proiezione `FRTPlayerEvent` · payload di motivazione · navigazione al punto della timeline |
| Tactical Designer × Map Editor | dati canonici di mappa · stessa semantica runtime · stessa validazione |
| Tactical Designer × Content | Skill Workbench edita varianti, **Scenario Harness** le valuta — non un secondo valutatore |
| Tactical Designer × Bot | gli scenari diventano test di competenza e suite di regressione |
| Presentation × TD / Replay / Playground | **un solo** linguaggio semantico di overlay: selezionato · valido · invalido · previsto · confermato · incerto · copertura · hazard |
| Online × Replay | replay pubblico sanitizzato e audit trace privata sono **due prodotti** sugli stessi dati canonici |
| Online × Explainability | la proiezione avviene **dopo** l'autorizzazione dell'osservatore, mai prima |
| Shell × Replay | Main Menu e Result navigano verso lo stesso viewer di #472 |

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
| [`hex-map-roadmap.md`](hex-map-roadmap.md) | registro storico di ciò che è stato consegnato |
| [`roadmap-editor.md`](roadmap-editor.md) | vista storica ritirata |

Nessuno di questi, incluso questo file, è autorevole sullo stato corrente.
