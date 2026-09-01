# Handoff — le decisioni d'autore `DQA-030`…`DQA-044` e le tre della seduta finale

> `HANDOFF` · **Stato**: chiuso · **Scritto**: 2026-09-01 · **Istruito contro**: `origin/main = 90cdc903`
>
> **Cosa è**: la **tavola di corrispondenza** fra la numerazione `DQA-nnn` — usata da una sessione di
> design esterna e da un handoff Drive — e gli identificatori che questo repository possiede davvero.
> Serve a una cosa sola: permettere a chi trova scritto *«vedi `DQA-037`»* di arrivare all'owner corretto
> senza la conversazione che ha prodotto quel numero.
>
> **Cosa non è**: un owner. ⛔ **Nessuna riga qui è normativa.** Ogni regola vive nel
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e nella sua spec; questo documento *indirizza*.
> Se una riga qui contraddice il Decision Log, ha ragione il Decision Log e questa riga è un difetto.

---

## 1. Perché questo documento esiste

La numerazione `DQA-nnn` **non è mai stata canone in questo repository**, e cercarla dà quasi sempre zero:
`DQA-033`…`DQA-039` e `DQA-042`/`DQA-043` non compaiono in nessun file. Non perché le decisioni manchino —
la maggior parte è registrata da mesi — ma perché il repository le possiede sotto **altri nomi**: `COV-*`,
`MOV-*`, `MSE-*`, `D-nnn`.

🔑 **È un problema di indirizzamento, non di contenuto**, e ha una sola conseguenza pratica: chi eredita un
handoff scritto in `DQA-*` e fa `grep` conclude che il repository non sappia, quando in realtà sa e usa un
altro nome. Questa tavola chiude quella distanza.

---

## 2. La tavola — `DQA-030`…`DQA-044`

| `DQA` | Argomento | ID canonico | Owner della regola | Stato al 2026-09-01 |
|---|---|---|---|---|
| `DQA-030` | KO e occupancy della cella | — | [`D-294`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** dal 2026-08-31 |
| `DQA-031` | Footprint/clearance `Small`/`Medium`/`Large` | `COV-1` | [`D-303`](../../decisions/RT_PDR_00_Decision_Log.md) + [`D-307`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata**, e `D-307` ne fissa i valori `2`/`3`/`4` |
| `DQA-032` | Chi produce un `CoverAnchor` | `COV-2` | [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** — ibrido |
| `DQA-033` | Serializzazione e privacy della `CoverSelection` | `COV-3` | [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** |
| `DQA-034` | La `CoverSelection` nell'hash | `COV-4` | [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** |
| `DQA-035` | `Facing` e copertura indipendenti | `COV-5` | [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** |
| `DQA-036` | `+1 MP` per entrare in copertura | `COV-6` | [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata** |
| `DQA-037` | Reposition e vault | `COV-7` | [`D-308`](../../decisions/RT_PDR_00_Decision_Log.md) | 🆕 **registrata il 2026-09-01** — era **aperta** |
| `DQA-038` | Distruzione di una sorgente di copertura | `COV-8` | [`D-308`](../../decisions/RT_PDR_00_Decision_Log.md) | 🆕 **registrata il 2026-09-01** — era **aperta** |
| `DQA-039` | Contatto puntuale e settori | `MSE-4` | [`D-306`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata e implementata** — PR [#2002](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2002) |
| `DQA-040` | `LinearCharge` si ferma sulla cella adiacente | — | [`D-296`](../../decisions/RT_PDR_00_Decision_Log.md) | ✅ **registrata, implementata e pinnata** |
| `DQA-041` | Dash: attraversare sì, terminare no | — | [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) | ✅ **già canone, e misurata nel codice** — nessuna `D-nnn` nuova |
| `DQA-042` | `StepMicroStep` al Decision Boundary | — | [`D-311`](../../decisions/RT_PDR_00_Decision_Log.md) | 🆕 **registrata il 2026-09-01** — la issue la lasciava aperta |
| `DQA-043` | Replay: nessun supporto legacy | — | [`D-310`](../../decisions/RT_PDR_00_Decision_Log.md) | 🆕 **registrata il 2026-09-01** — ⚠️ **era un conflitto**, vedi §4 |
| `DQA-044` | Coordinate nominate negli scenari | — | [`sync-governance-dqa-026-030`](sync-governance-dqa-026-030-2026-08-31.md) | ✅ **registrata** |

> 🔎 **`DQA-041` non ha una `D-nnn` deliberatamente**, e la ragione è scritta nel referto che l'ha misurata:
> `StepHexMovement` calcola già `bCrossesStationary = PassesThrough(i) && !bFinalStep`, e *«sarebbe un numero
> speso su una regola già posseduta»*. Chi rilegge l'handoff d'autore e non trova una decisione per questa
> riga **non ha trovato un buco**.

---

## 3. Le tre decisioni della seduta finale

| Issue | Decisione d'autore | Dove vive ora | Cosa resta |
|---|---|---|---|
| [#1918](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1918) | `Deflect` diventa **pool di assorbimento commutativa** sul boundary — l'uscita **(B)** delle due che la issue offriva | [`D-309`](../../decisions/RT_PDR_00_Decision_Log.md) | **implementazione**: la issue resta `OPEN`, e il `TestNotEqual` di `Combat.GuardPoolIsPermutationInvariant` va **riscritto** |
| [#1902](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1902) | ⛔ **nessuna nuova regola di scoring**: `MEASURE FIRST` | la issue stessa — ⛔ **nessuna `D-nnn`**, perché non c'è una regola da registrare | la **diagnosi** prima della scelta, e la scelta fra i tre esiti che la issue già elenca |
| [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) | scambio diretto e ciclo chiuso **bloccano** | [`D-295`](../../decisions/RT_PDR_00_Decision_Log.md) + codice | ✅ **niente** — implementata e `CLOSED` il 2026-08-31, PR [#1986](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1986) |

### 3.1 🔑 Il reason code di `#1922` si chiama `BlockedByCycle`, non `BlockedByMovementCycle`

L'handoff d'autore proponeva `BlockedByMovementCycle` e autorizzava esplicitamente il riuso di *«un reason
code semanticamente equivalente»*. Quel codice **esiste** e si chiama
[`ERTMoveOutcome::BlockedByCycle`](../../../Source/RefactorTactics/Turn/RTTurnLog.h) — aggiunto **in coda**
all'enum secondo la disciplina che quel file impone, con la motivazione del perché non riusa
`BlockedContested` (*«dice due unità verso la stessa cella; in un ciclo le celle sono distinte»*),
`BlockedByUnit` (*«dice un'unità ferma; in un ciclo sono tutte in movimento»*) né `BlockedByImpact`.

⚠️ **Non è un sinonimo da aggiungere**: cercare `BlockedByMovementCycle` in `Source/` dà **zero**, ed è
corretto che lo dia. Questa riga esiste perché chi lo cerca sappia dove guardare.

### 3.2 `bPassThrough` non partecipa, e la ragione è più sottile di come suona

L'handoff chiedeva che `bPassThrough` **non** rendesse legali scambio e ciclo. È vero, ma non perché la
regola lo vieti: `bPassThrough` governa il ramo dell'unità **ferma**, mentre ciclo e scambio vivono fra
unità **in movimento**. Sono due mondi che non si toccano.

🔎 Il report d'esecuzione di `#1922` registra che questa lettura ha corretto una prima analisi sbagliata —
il caso *esiste*, perché il resolver avanza a micro-step e la cella dell'altro non è sempre la destinazione
finale — e da lì è nato un test in più. La conclusione regge; la strada per arrivarci no, ed è annotata lì.

---

## 4. ⚠️ L'unico conflitto vero fra handoff e repository

`DQA-043` e [#1880](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1880) dicevano **cose
opposte sullo stesso oggetto**:

| Fonte | Cosa chiedeva |
|---|---|
| L'audit obbligatorio di `#1880` | misurare la *«compatibilità all'indietro degli archivi già scritti»* |
| La decisione d'autore | ⛔ **non esiste supporto legacy** — traccia fuori schema = rifiuto fail-closed |

✅ **Risolto a favore della decisione d'autore**, e registrato come conflitto invece che applicato in
silenzio: [`D-310`](../../decisions/RT_PDR_00_Decision_Log.md). Il criterio *«schema e versioning espliciti,
nessuna migrazione silenziosa»* di `#1880` **non cade**: la decisione lo rafforza, perché non c'è migrazione
silenziosa quando non c'è migrazione.

🔑 **Costa zero perché il parco tracce da proteggere non esiste**: gli archivi v0.1 sono corpus golden
rigenerabili, ed è la conseguenza già pagata da [`D-245`](../../decisions/RT_PDR_00_Decision_Log.md).

---

## 5. Ciò che questo handoff **non** ha cambiato, e sono le cose che sembravano da cambiare

| Sembrava | Misurato |
|---|---|
| `#1733` da lavorare | **`CLOSED`** dal 2026-08-31: la sovrapposizione *non era mai avvenuta*, e la diagnosi veniva dall'assenza del soggetto nel TurnLog |
| `#1801` da lavorare | **`CLOSED`** |
| `#1922` da riaprire come *implementation actionable* | **`CLOSED` e implementata**, PR [#1986](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1986) |
| `COV-1`…`COV-8` tutte già chiuse a livello di design | **sei sì, due no**: `COV-7` e `COV-8` erano **aperte** in `OPEN_DECISIONS.md`, ed è il vero contributo di questo pass |
| `#1826` da chiudere o da lavorare | **fix mergiato** (PR [#2002](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2002)), issue **`OPEN`** per la parte non spedita |

⛔ **Nessuna issue è stata riaperta**, e nessuna chiusa: una decisione di design non chiude
un'implementazione, ed è la disciplina che il Decision Log ripete su ogni voce.

---

## 6. Il lavoro di implementazione ancora aperto, con il suo owner

Nessuna delle quattro decisioni registrate oggi ha scritto una riga di `Source/`.

| Cosa | Owner | Bloccata da |
|---|---|---|
| `CoverSelection` come tipo, digest, `+1 MP`, reposition, vault | [#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833) · [#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828) · [#1827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1827) | `COV-9`/`COV-10`, ancora **aperte** |
| Il terzo valore di `ERTIntraCellTraversal` | [#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828) | ora **sbloccato**: `D-308` gli dà il produttore |
| `Deflect` come pool commutativa | [#1918](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1918) | niente |
| Schema micro-step nella traccia | [#1880](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1880) | l'audit dei campi, che resta obbligatorio |
| `Pause`/`StepMicroStep` | [#1879](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1879) | niente |
| Diagnosi del comportamento del bot | [#1902](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1902) | la seduta PIE di [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) per il termine mancante |
| ~~I due test degli invarianti di micro-step~~ | ~~[#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000)~~ | ✅ **chiusa**: scritti, verificati per mutazione e mergiati in `main` ([PR #2015](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2015)) mentre questo documento veniva scritto — vedi §7 |

---

## 7. Il lavoro parallelo su `#2000`, e cosa insegna

Quando questo pass è cominciato, i due test che
[#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000) chiede —
`OneTransitionMax_PerMicroStep` e `BlockedPath_DoesNotAutoReroute` — erano **non committati** nel working
tree condiviso. Sono stati committati per non perderli (`8f8cafe8`), dichiarandoli non verificati.

🔁 **Quella dichiarazione era sbagliata, ed è stata rettificata da un'altra sessione poche ore dopo**
(`f942d706`): il codice era già compilato e verde, e ora porta anche la **verifica per mutazione** —
portando il resolver ad avanzare due nodi per micro-step, i due test nuovi falliscono e
`StepperMatchesBatchResolver` **resta verde**, che è esattamente la tesi della issue.

🔑 **La lezione vale oltre `#2000`, e va letta prima di committare in questo repository**: un working
tree condiviso non porta la data del proprio contenuto. Un file non committato può essere già stato
compilato, già verificato, o già smontato da qualcun altro — e un referto scritto guardando solo il diff
dichiara con sicurezza qualcosa che nessuno ha misurato. ⚠️ Qui il danno è stato solo un messaggio di
commit da rettificare; con un `.uasset` sarebbe stato un fix disfatto.

✅ **Chiusa**: [PR #2015](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2015) ha mergiato i due test in `main`, e `#2000` è `CLOSED`.

---

## 8. Ciò che questo documento **non** contiene, perché vive altrove

| Argomento | Owner |
|---|---|
| Le regole di copertura, per esteso | [`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md) §13 |
| Le domande ancora aperte | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) — `MAP-4`, `COV-9`, `COV-10`, `MOV-3`, `MOV-4`, `PER-5`, `OW-6` |
| Le decisioni, con la loro istruttoria | [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) |
| Il movimento, per famiglie | [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) |
| Lo stato di rilascio | [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) — ⛔ **non toccato da questo pass** |
| La sincronizzazione `DQA-026`…`DQA-030` | [`sync-governance-dqa-026-030-2026-08-31.md`](sync-governance-dqa-026-030-2026-08-31.md) |

---

## 9. Cosa dovrebbe far riaprire questo documento

- Una decisione d'autore che arriva ancora numerata `DQA-nnn` **oltre** `DQA-044`: la tavola del §2 va estesa.
- La scoperta che una riga del §2 punta all'owner sbagliato: è un difetto di questo file, e si corregge qui.
- ⛔ **Non** una nuova regola di gioco: quella va nel Decision Log, e questo documento al massimo la indirizza.
