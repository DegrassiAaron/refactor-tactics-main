# Referto — consolidamento Bot/AI, Team Planner, Belief e tracking

> **Data**: 2026-08-11 · **Sorgenti**: [`…bot-ai-team-planner-belief-e-tracking.md`](../../archive/src/handoff/2026-08-11-bot-ai-team-planner-belief-e-tracking.md)
> e [`…battle-simulation-harness-unificato-e-release-bot.md`](../../archive/src/handoff/2026-08-11-battle-simulation-harness-unificato-e-release-bot.md)
> **Base verificata**: `main` @ `1f1dc24`, poi **rimisurata su `eb3cc42`** dopo che `main` si è mosso durante
> il lavoro (PR [#530](https://github.com/DegrassiAaron/refactor-tactics-main/issues/530)) — quel merge non
> tocca nessun file di `Source/`, quindi le misure reggono. È il commit scritto in `last_verified`, e non la
> base da cui il branch è partito: [PR #484](https://github.com/DegrassiAaron/refactor-tactics-main/pull/484)
> ha già pagato quella confusione una volta. UE **5.8** (`RefactorTactics.uproject`, `EngineAssociation`)
> **Natura**: referto di revisione. Non è un owner documentale: dice **cosa è entrato dove**, e perché il
> resto non è entrato.

L'handoff chiedeva di consolidare «tutto il focus Bot/AI» in documentazione, Wiki, Feature Registry, roadmap,
Scenario Map, Editor Map, Epic e issue. Chiedeva anche — al §0, e va riconosciuto — di **ispezionare il
repository reale prima di modificare**, e di non trattarsi come fonte di verità.

Fatto questo, il consolidamento è risultato **molto più piccolo di quanto il documento suppone**, e per la
ragione migliore: il repository possedeva già la maggior parte di ciò che l'handoff propone. Il valore vero
sta nelle quattro cose che **non** possedeva, e nel misurare quante delle sue premesse fossero false.

---

## 1. Le quattro premesse false

Sono i numeri con cui l'handoff descrive lo stato del progetto. Misurati su `main` @ `1f1dc24`.

| L'handoff dice | Misurato | Conseguenza |
|---|---|---|
| §42: undici `RT-FEAT-BOT-*` con status noti | `grep "feature_id: RT-FEAT-BOT"` → **due**: `RT-FEAT-BOT-BASE`, `RT-FEAT-BOT-TACTICAL` | Nove ID citati non esistono. Chi avesse aggiornato «lo status di `RT-FEAT-BOT-BELIEF`» avrebbe creato la feature invece di aggiornarla |
| §42: status `IMPLEMENTED_PARTIAL`, `SPECIFIED`, `FUTURE` | Vocabolario reale: `IDEA · DESIGNED · SPECIFIED · IMPLEMENTING · TESTABLE · INTEGRATED · RELEASE_READY · DONE · DEFERRED · BLOCKED` | Tre dei quattro status citati non sono valori ammessi. `status` è inoltre **derivato dai gate**, non scritto: assegnarlo a mano fa fallire `validate` |
| §45: sei Epic «da creare o consolidare» | Quattro esistono già come issue: `E26` [#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326), `E27` [#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327), `E28` [#328](https://github.com/DegrassiAaron/refactor-tactics-main/issues/328), `E29` [#329](https://github.com/DegrassiAaron/refactor-tactics-main/issues/329) | Nessuna Epic nuova creata. Le esistenti sono state **estese**, che è ciò che il §45 stesso prescrive («cercare prima equivalenti») |
| §52: 33 ScenarioId in forma `AI.<Area>.<Caso>` | Convenzione reale: `Spec.<Area>.<Caso>`, `Visual.…`, `Simulation.…`, con il file in `Scenarios/Spec/<Area>/` | Adottare `AI.*` avrebbe creato un secondo spazio di nomi accanto a quello che l'indice e il harness già risolvono |

Il §42 chiede esplicitamente «Claude deve verificare il registry corrente», e il §0 impone l'audit. Le
premesse false non sono quindi un difetto dell'handoff: sono **il difetto che l'handoff sapeva di poter
avere**, e l'unica cosa che non funziona è che le presenta come «stato noto» invece che come ipotesi.

---

## 2. Revisione della specifica — panel

Quattro rilievi che cambiano cosa è stato scritto, non quattro opinioni.

### ⚠️ `CRITICAL` — Il §61 «Definition of Done AI» non è verificabile (Wiegers)

Tredici voci in un blocco di testo, senza soggetto e senza soglia: *«no hidden-state leak»*, *«deterministic
result»*, *«decision trace/debug»*. Nessuna dice **chi** la verifica né **quando** è falsa. Applicata così
com'è, una feature è Done quando qualcuno dichiara che lo è.

**Come è entrata invece**: i nove gate del Feature Registry esistevano già ed erano **la stessa lista**,
scritta per essere derivabile — `spec · data · runtime · log_debug · automation · scenario · ui_wiki ·
packaged · network_privacy`. Il §61 non aggiunge un gate: aggiunge tredici modi di dire gli stessi nove.
Recepito come **contenuto dei gate** delle feature Bot, non come nuova lista.

### ⚠️ `MAJOR` — «same seed ⇒ same plan» presuppone un seed che non esiste (Nygard)

Il §15 fissa il determinismo come `same snapshot + same profile + same seed = same Intent`. Nel repository
**non esiste un RNG**: il `Seed` degli scenari è dichiarato in `RTTestScenario.h` e mai consumato, e il
determinismo viene da coordinate intere e ordinamenti totali — è scritto in `D-077`. Un requisito che nomina
il seed come ingresso rende **non falsificabile** il test che dovrebbe difenderlo: si può passare
mantenendo un `FMath::Rand()` e fissando il seed.

**Come è entrata invece**: `D-096` fissa il determinismo **senza seed** — stesso snapshot e stesso profilo
⇒ stesso Intent, con l'ordine totale come meccanismo. È più forte, non più debole: vieta l'RNG invece di
domarlo.

### ⚠️ `MAJOR` — «budget di conteggio, non wall-clock» è giusto e il §24 lo contraddice (Fowler)

Il §15 vieta `think for 500 ms` come budget decisionale. Il §24 poi propone una policy di replanning
scandita da *«inizio planning / ultimi secondi / quasi fine»*, cioè dal tempo reale. Le due sezioni non si
citano.

**Risolto** in `D-096`: la separazione è quella che il progetto applica già al Decision Boundary — il tempo
reale può decidere **quando si smette di ripianificare**, mai **quale piano esce**. La soglia di isteresi è
un confronto fra punteggi, e la fase di planning in cui la si applica è presentazione.

### ⚠️ `MINOR` — Il §54 «Editor Map» propone nove task per feature che non esistono (Cockburn)

Nove voci di verifica manuale sulla leggibilità di overlay di debug del bot. Nessuno dei nove overlay esiste,
e le feature che li produrrebbero sono v0.2/v0.3.

**Non entrate.** Le sedute in editor (`editor-sessions.yaml`) dichiarano `unblocked_by` verso codice che
deve esistere: nove sedute bloccate da lavoro non iniziato avrebbero allungato la coda senza spostarne
l'ordine. Registrate qui come §6, che è il posto in cui si va a cercarle quando `RT-FEAT-BOT-TACTICAL` esce
da `IDEA`.

---

## 3. Cosa il repository possedeva già — e dove

Undici affermazioni dell'handoff presentate come «da consolidare» erano già canoniche. Elencate perché il
prossimo handoff sul bot le riproporrà, e la risposta deve costare una riga.

| L'handoff propone | Già scritto in |
|---|---|
| §3 — il bot produce Intent normali, il resolver resta autorità | [`spec-bot-hex.md`](../../gameplay/spec-bot-hex.md) §1 · `CLAUDE.md` §5 |
| §4 — il bot non legge lo stato nascosto; difficoltà = più ragionamento, mai più informazione | [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E26, riquadro «Invariante di difficoltà» — con la ragione per cui è pubblicata, non interna |
| §5.5 — NavMesh non è autorità del movimento | [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md) · [`spec-pathfinding-pf3-pf4.md`](../../technical/spec-pathfinding-pf3-pf4.md) §1 |
| §14 — niente `if (Character == Flux)` nello scoring | [ADR-0006](../../decisions/adr-0006-ownership-abilita-sinergie.md) (`D-029`) · [`spec-bot-hex.md`](../../gameplay/spec-bot-hex.md) §9 |
| §15 — niente ordine di `TMap`/`TSet` come tie-break | `CLAUDE.md` §4 · [`spec-bot-hex.md`](../../gameplay/spec-bot-hex.md) §5, con il test `ChooseBestPlanOrderIndependent` |
| §26 — Overwatch `FIRE`/`HOLD`, timeout = `HOLD` | `D-012` · [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) · `CLAUDE.md` §2 |
| §38 — niente look-ahead profondo/Monte Carlo in v0.1 | [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E28 (v0.3) |
| §40 — il trace privato non finisce al client avversario | `D-043` (privacy delle reazioni) · `CP 5.4` (#53, chiusa) |
| §53.3 — canary sull'onniscienza | La premessa è già misurata e **negativa**: [`spec-bot-hex.md`](../../gameplay/spec-bot-hex.md) §6 dichiara che oggi il bot vede tutte le posizioni nemiche, ed è il lavoro di `CP 13.5` ([#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160)) |
| §60 — non promuovere a `DONE` perché la spec è cresciuta | È il funzionamento del registry: `status` è derivato dai gate e il validator lo verifica |
| §64 — non creare una seconda AI parallela | L'entry point è uno: `URTHexBotLibrary`, consumato da `ARTTurnManager::PlanBots` |

---

## 4. Cosa è entrato

### 4.1 Decisioni — `D-095`…`D-099`

Cinque. Il criterio per scriverne una è stato: *esiste un documento corrente che possiede questa
affermazione?* Se sì, non è una decisione nuova ma un rimando.

| ID | Decide | Perché non c'era |
|---|---|---|
| `D-095` | Il ruolo dei framework AI di Unreal: **Utility custom** è il core; StateTree orchestratore opzionale; Behavior Tree e EQS non autorità; Learning Agents solo offline; Mass AI fuori scope | `StateTree`, `Behavior Tree`, `EQS`, `Learning Agents`, `Mass AI`: **zero occorrenze** in `docs/` e `Source/` prima di oggi. È il solo gap tecnologico reale dell'handoff |
| `D-096` | Determinismo del planner **senza seed**, con budget di conteggio; il tempo reale decide quando si smette di ripianificare, mai quale piano esce | Il divieto di wall-clock c'era per il *resolver*, non per il *planner*; e il §15 dell'handoff introduceva un seed che nel progetto non esiste |
| `D-097` | Il Team Planner è **Top-K per unità + combinazione centrale**, non un assegnatore di ruoli | `E26` diceva «coordinazione vera» senza dire quale architettura. Due modelli incompatibili erano entrambi leggibili in quella riga |
| `D-098` | Una sinergia intra-turno vale solo se **compatibile con l'ordine delle fasi**, e la compatibilità si chiede alle regole invece di riscriverla nel bot | Nessun documento vietava al bot di duplicare l'ordine del resolver, che è il modo in cui nascono le combo immaginarie |
| `D-099` | **Una belief non diventa conoscenza** perché è lo scenario più plausibile; e la confidenza del bot **non è un enum nuovo**: è un ordinamento *dentro* `ERTAwareness::Uncertain` | `E27` diceva «gradi di certezza» senza fissarne la regola di promozione — e il §28 dell'handoff proponeva quattro categorie che sarebbero state il **quarto** vocabolario sullo stesso asse (§5) |

### 4.2 Owner documentale nuovo

[`../../gameplay/spec-bot-tattico.md`](../../gameplay/spec-bot-tattico.md) — l'architettura del bot **oltre**
`RT-FEAT-BOT-BASE`. Il registry dichiarava che `RT-FEAT-BOT-TACTICAL` era «citato in `roadmap-post-v0.1.md`
fra le epic E21–E32 ma **senza owner documentale proprio**»: era vero, e ora ha l'owner.

Non descrive codice che esiste. Lo dice in testa, ed è il motivo per cui non tocca nessun gate `runtime`.

### 4.3 Feature Registry

`RT-FEAT-BOT-TACTICAL` copriva da solo E26 (v0.2), E27 e E28 (v0.3): un `epic:` singolo e una `release:`
singola non possono rappresentarlo, e infatti valevano `null` e `v0.2`. Diviso in **quattro** feature con
gate che si muovono davvero in modo indipendente:

| Feature | Release | Epic | Cosa raccoglie |
|---|---|---|---|
| `RT-FEAT-BOT-FAIRNESS` | **v0.1** | E13 | L'ingresso del bot è la Team Knowledge della sua squadra, e un canary lo dimostra. È il solo pezzo con lavoro **già aperto in v0.1**: `CP 13.5` ([#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160)) |
| `RT-FEAT-BOT-TACTICAL` | v0.2 | E26 | Team Planner Top-K, ruoli dinamici, capability/job, sinergia, conflitti, compatibilità temporale |
| `RT-FEAT-BOT-BELIEF` | v0.3 | E27 | Known vs Belief, confidenza discreta, celle plausibili, decadimento, threat projection, information gain |
| `RT-FEAT-BOT-PREDICTIVE` | v0.3 | E28 | Scenari nemici limitati, robust scoring, bait/minaccia non contabilizzata, counterfactual |

Nessuna di esse è oltre `DESIGNED`/`SPECIFIED`: **l'handoff ha prodotto specifica, non implementazione**, e
il §60 lo dice per primo.

### 4.4 Scenario Map

**Tredici** ScenarioId `planned:` nella convenzione reale (`Spec.Bot.*`), non i 33 di forma `AI.*` del §52 —
misurati dopo la rigenerazione, in `scenariomap.shortlist.md`. La riduzione non è pigrizia: venti dei 33
descrivono comportamenti di feature che non hanno né spec né gate, e uno scenario pianificato per un sistema
che non ha ancora una forma è un nome che verrà rinominato.

I `planned` totali del registry passano da **34** a **47**, e i 65 scenari versionati non cambiano: è la
misura giusta di cosa ha prodotto questo consolidamento — tredici promesse verificabili, zero test verdi.

### 4.5 GitHub

**Nessuna Epic nuova.** Tre estese, una issue di checkpoint della v0.1 estesa, sei checkpoint creati.

| Issue | Azione | Cosa cambia |
|---|---|---|
| [#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326) `E26` | aggiornata | Architettura fissata (`D-095`–`D-098`), scope ridotto: belief e predittivo escono in feature proprie |
| [#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327) `E27` | aggiornata | `D-099`: «gradi di certezza» **non** è un enum nuovo |
| [#328](https://github.com/DegrassiAaron/refactor-tactics-main/issues/328) `E28` | aggiornata | Perimetro fissato; checkpoint **deliberatamente non creati** — vedi sotto |
| [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) `CP 13.5` | aggiornata | Il DoD acquista il **canary** e il caso decoy. È l'unico lavoro Bot/AI aperto in **v0.1** |
| [#531](https://github.com/DegrassiAaron/refactor-tactics-main/issues/531)–[#534](https://github.com/DegrassiAaron/refactor-tactics-main/issues/534) `CP 26.1`–`26.4` | **create** | Top-K e diversità · compatibilità temporale · conflitti e risorsa contesa · isteresi |
| [#535](https://github.com/DegrassiAaron/refactor-tactics-main/issues/535)–[#536](https://github.com/DegrassiAaron/refactor-tactics-main/issues/536) `CP 27.1`–`27.2` | **create** | Belief e decadimento · threat projection e information gain |

Le sei nuove sono **sub-issue** delle rispettive epic (`sub_issues` API), che è la relazione che il repository
non aveva ancora usato: prima il legame epic↔checkpoint viveva solo nella riga «**Epic**: #N» del corpo, cioè
in prosa. Le epic ora mostrano l'avanzamento senza che nessuno lo conti.

**E28 non riceve checkpoint, ed è una scelta.** A differenza di E26 ed E27 — dove due decisioni hanno fissato
l'architettura — lì ciò che resta da decidere è più di ciò che è deciso: quanti scenari per difficoltà, la
formula del robust score, quando il resolver diventa estraibile. Checkpoint scritti su quelle domande
avrebbero DoD che si riscrivono al primo profiling.

---

## 5. Cosa NON è entrato, e perché

| Proposta | Motivo |
|---|---|
| §43.2 — cinque `RT-FEAT-BOT-*` nuovi (`THREAT-PROJECTION`, `ENEMY-SCENARIOS`, `HUMAN-COORDINATION`, `DEBUG-TRACE`, `MULTITURN-STRATEGY`) | Il §43.2 stesso dice «NON crearle alla cieca» e chiede di verificare se sono gate. Lo sono: quattro su cinque diventano criteri di accettazione delle quattro feature sopra. `MULTITURN-STRATEGY` resta fuori da qualunque release ed è già `E28`/futuro |
| §46–§51 — 50 titoli di issue | Creare 50 issue per lavoro che comincia in v0.2 riempie la coda della v0.1 di rumore. I titoli restano nel sorgente archiviato, che è consultabile quando l'epic si apre |
| §54 — nove Editor Task | Vedi §2, rilievo `MINOR`. Riportati qui sotto al §6 |
| §55 — dieci pagine Wiki nuove | La Wiki ha **una** pagina sul bot (`avversario-bot`) ed è lato giocatore. Dieci pagine di architettura interna non sono materiale da Wiki pubblicata: [`D-076`](../../decisions/RT_PDR_00_Decision_Log.md) ha appena ridotto la Wiki a fonte unica per non tenere allineate due copie. `avversario-bot` è stata aggiornata su un punto solo, quello che il giocatore può verificare |
| §28 — quattro categorie di confidenza (`Confirmed · Strong · Plausible · Weak`) | **Sarebbe stato il quarto vocabolario sullo stesso asse.** Misurati in `Source/Perception/`: `ERTAwareness` (`Hidden · Uncertain · Detected`) dice *quanto* la squadra sa, `ERTTargetKnowledge` (`Allowed · CellOnly · Rejected`) dice *cosa può farne* il targeting, e `FRTLastKnownContact.TurnNumber` è già il campo «che fa scadere il ricordo». Aggiungerne un quinto valore chiamato «confidenza» avrebbe prodotto due risposte diverse alla domanda *sappiamo dov'è?* — `D-099` lo risolve ordinando **dentro** `Uncertain` invece di affiancarlo |
| §58 — undici Data Asset | `URTActionData`/`URTHeroData` sono il modello del progetto e non c'è ancora nulla da configurare. Un Data Asset senza consumatore è un dato che nessuno legge |
| §62 — undici metriche di performance | Nessun profiling è stato fatto, e il §62 stesso dice «non fissare numeri nuovi senza profiling». Le metriche sono elencate nella spec nuova §9 come *cosa misurare quando ci sarà cosa misurare*, senza soglie |

---

## 6. Verifiche manuali future — non ancora sedute

Quando `RT-FEAT-BOT-TACTICAL` e `RT-FEAT-BOT-BELIEF` avranno un overlay, questi diventano voci di
[`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) e poi sedute in
[`../editor-sessions.yaml`](../editor-sessions.yaml). Oggi sarebbero sedute bloccate da lavoro non iniziato.

1. Leggibilità dell'overlay dei punteggi per cella sulla mappa esagonale.
2. Leggibilità della confidenza di belief a 1080p — le quattro categorie devono distinguersi senza legenda.
3. Leggibilità della threat projection **attraverso i livelli**: è il caso in cui una mappa multilivello
   confonde una proiezione piatta.
4. Il debug del bot non copre copertura, facing e hazard — cioè non nasconde ciò che serve a giudicarlo.
5. Presentazione di `FIRE`/`HOLD` negli scenari `Visual`.

---

## 7. Gap e domande aperte

Il §67 dell'handoff ne elenca venti. Diciassette sono **prematuri per costruzione**: chiedono un valore
(quota Top-K, pesi di confidenza, numero di scenari per difficoltà) che si fissa profilando codice che non
esiste. Registrarli come domande aperte li avrebbe fatti scadere prima di essere letti.

Tre no, e sono quelli con un innesco osservabile:

| Domanda | Innesco | Dove vive |
|---|---|---|
| Il bot in squadra mista umano+bot: il draft umano è un vincolo o un suggerimento? | Il primo formato in cui un umano e un bot condividono la squadra. **Non è la v0.1**, che è 2v2 offline contro il bot | Spec nuova §7, con la forma della risposta già vincolata da `D-097` |
| Il `BotDecisionTrace` è visibile in replay/spectator? | L'archivio replay (`R1`, `D-077`) più un bot che produce trace | Spec nuova §8. `D-043` dà già la regola per le reazioni, e la forma è la stessa |
| Quando la threat projection diventa feature propria invece di gate di `RT-FEAT-BOT-BELIEF`? | Quando un secondo consumatore la legge — la UI, non il solo planner | `RT-FEAT-BOT-BELIEF`, nota |

---

## 8. Fuori perimetro, corretto perché trovato

`D-091` era **duplicata su `main`**: due decisioni diverse con lo stesso ID — i tre valori acustici di `PER-2`
(`c7ce400`) e `Weapon.Environmental` fuori dalla v0.1 (`c4d5e6e`). Undicesima collisione di contatore e la
prima **atterrata**, trovata cercando il primo ID libero per `D-095`.

Aggiunto il gate che l'avrebbe intercettata: `check-docs-links.py` ora fallisce su un `D-nnn` duplicato nella
tabella. Verificato per mutazione — su `origin/main` prima della correzione trova `D-091` alle righe 121 e
122.

**E poi è successo di nuovo, mentre questo referto era in scrittura.** La PR
[#530](https://github.com/DegrassiAaron/refactor-tactics-main/issues/530) ha portato su `main` un `D-094`
diverso (`Reaction.Anchor`), cioè proprio l'ID a cui `WV-4` era appena stata spostata. Dodicesima collisione,
a poche ore dall'undicesima.

`WV-4` è quindi passata a **`D-100`** — il primo ID libero *dopo* le cinque di questo consolidamento, non
prima: rinumerare quelle avrebbe toccato una ventina di rimandi e quattro issue GitHub, questa ne aveva due.

> **Le due collisioni dicono cose diverse, ed è il motivo per cui vale la pena distinguerle.** L'undicesima è
> saltata perché nessuno ha eseguito il controllo. La dodicesima è stata trovata **perché il controllo è stato
> eseguito** — rileggere `main` dopo il push e prima del merge, che è esattamente il rimedio che la nota di
> `D-073` raccomandava. Cioè: la disciplina funziona, e funziona solo per chi se ne ricorda.
>
> ⚠️ **Il gate non ha trovato nessuna delle due**, e va detto: l'undicesima l'ha trovata una ricerca del primo
> ID libero, la dodicesima il controllo pre-merge su `origin/main`. Entrambe a mano. Ciò che il gate
> garantisce è la **tredicesima** — verificato per mutazione su entrambe: reintroducendo il duplicato esce
> `1` con le due righe stampate, rimuovendolo esce `0`. Un gate che non è stato eseguito non ha trovato
> niente, e questa riga esiste per non far diventare la mutazione una prova di qualcosa che non è successo.

**E il gate stesso è arrivato in code review con due difetti**, entrambi riprodotti eseguendolo e corretti
prima del merge:

1. **Non rispettava l'esenzione dei blocchi recintati** che ogni altro controllo del file applica. Una riga
   d'esempio in un fence — `| **D-100** | esempio di riga… |` in un documento che *spiega il formato della
   tabella* — sarebbe stata contata come duplicato. È il difetto che questo script ha già trovato su se
   stesso due volte (i link d'esempio nel proprio README, il `!` dell'immagine citata), e la sua stessa
   docstring ne dà la ragione: *«meglio stretto e creduto che largo e ignorato»*, perché **un gate che
   sbaglia viene disattivato al terzo falso positivo**.
2. **Non catturava `UnicodeDecodeError`**, mentre il ciclo principale sì. Un byte non valido nel Decision Log
   avrebbe ucciso l'intero gate *prima* che potesse riportare i link rotti — cioè avrebbe tolto la verifica
   proprio quando serve.

Verificato dopo la correzione: esempio in fence → `0`; duplicato vero fuori dal fence → `1`, righe corrette;
Decision Log con byte non validi → `0` e nessun traceback.

---

## 9. Secondo handoff dello stesso giorno — Battle Simulation e harness unificato

Sorgente: [`../../archive/src/handoff/2026-08-11-battle-simulation-harness-unificato-e-release-bot.md`](../../archive/src/handoff/2026-08-11-battle-simulation-harness-unificato-e-release-bot.md).

**È calibrato molto meglio del primo, e va detto perché è raro.** Il §1.1 nomina i **due** Feature ID reali
invece di undici inventati; il §1.2 nomina `#326` e `#328` con i numeri giusti e ordina di non duplicarli; il
§3 avverte da sé di non reintrodurre il roster storico `Aegis/Nyx/Drift/Vex`; il §33.1 dice di aggiornare i
Feature ID esistenti invece di moltiplicarli. Nessuna delle quattro premesse false del §1 di questo referto
si ripete.

Restano due imprecisioni, entrambe di documenti spariti sotto di lui:

| Dice | Realtà |
|---|---|
| `docs/wiki/feature-status.md` fra le viste generate | Non esiste più: [D-076](../../decisions/RT_PDR_00_Decision_Log.md) ha reso il clone la fonte unica, e la vista è `Stato-delle-feature` **nel clone** |
| `docs/roadmap/roadmap-editor.md` come owner della Editor Map | `HISTORICAL` dal 2026-08-08; l'owner dei dati è `editor-sessions.yaml`, la vista è `editormap.shortlist.md` — **generata** |

### 9.1 La contraddizione col consolidamento della mattina, e come si risolve

Il §1.1 dice: *«Non creare una costellazione di nuovi `RT-FEAT-BOT-*` solo perché questo handoff contiene
molte capability. La granularità fine va nelle Issue, nei checkpoint, negli scenari e nei test.»*

Il consolidamento della mattina (§4.3) ne aveva creati **tre**. La contraddizione è apparente, e la
distinzione è quella che il §33.1 scrive da sé — *«creare un Feature ID nuovo solo se la granularità corrente
del registry lo richiede»*:

- **Vietato**: dividere per **capability**. `threat projection`, `information gain`, `capability tags`,
  `dynamic roles` non hanno un ID proprio, e infatti non ce l'hanno: sono gate e criteri di accettazione.
- **Richiesto dallo schema**: dividere per **release ed epic**. `roadmap.epic` e `release` sono **valori
  singoli**. `RT-FEAT-BOT-TACTICAL` copriva E26 (v0.2), E27 e E28 — e il registry lo dichiarava con
  `epic: null`, cioè con l'unica cosa che poteva dire: *non lo so*.

La prova che la divisione non è cosmetica sta in `RT-FEAT-BOT-FAIRNESS`: senza di essa, il lavoro Bot/AI
**già aperto in v0.1** ([#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160)) non
comparirebbe nella roadmap di release, perché sarebbe attaccato a una feature `v0.2`. Dopo la divisione
compare sotto E13, dove qualcuno lo leggerà.

E la granularità fine è andata **esattamente dove il §1.1 chiede**: sei checkpoint, tredici scenari.

### 9.2 Cosa entra dal secondo handoff

| ID | Decide | Perché non c'era |
|---|---|---|
| `D-101` | **Un solo harness**, e un `DecisionProvider` restituisce **decisioni**, mai **esiti** | L'harness esiste (`Source/RefactorTactics/ScenarioHarness/`), il contratto no: ci sono **due** appigli ad hoc — gli intent scriptati di `FRTScenarioIntent` e `ARTTurnManager::PlanBotsForTest()`, che il commento del runner dichiara essere *«l'unico appiglio, che esisteva già per i test d'integrazione»*. Ogni modalità nuova ne aggiungerebbe un altro |
| `D-102` | **Un risultato bot-contro-bot non è evidenza di bilanciamento** finché il bot non è certificato sulle capability che lo producono | Nessuna occorrenza di *competence*/*certificazione* nel repository — ma il dubbio era già scritto, vedi sotto |

### 9.3 Il §18 aveva già un'istanza viva, e nessuno l'aveva chiamata per nome

È il motivo per cui `D-102` vale più di una buona idea generica.

`roadmap-checkpoint.md` §Metriche misura i round per partita e annota:

> 🟡 **10** misurato bot-vs-bot (2026-08-06, `HexMatch.PlaysToCompletion`) — dentro banda, **ma è un bot
> contro un bot**

e `v0.1-definition-of-done.md` ripete che *«l'unico dato reale è ancora 10 round bot-vs-bot»*.

Il dubbio c'era, ed era formulato correttamente. Mancavano la **regola** — cosa si può concludere da quel
numero — e il **modo di scioglierlo**: uno stato di competenza per capability, legato a scenari reali, che
distingue *«l'eroe è debole»* da *«il bot non sa giocarlo»*.

> ⚠️ **Il costo di non averla è asimmetrico e invisibile.** Un nerf deciso su un bot che non sa usare
> un'abilità **sembra** funzionare: il win rate si muove. E il difetto vero — che quell'abilità non entra mai
> fra le candidate — resta coperto proprio dalla correzione. Chi rileggesse i numeri un anno dopo troverebbe
> una decisione motivata da dati veri e conclusioni false.

### 9.4 Cosa NON entra dal secondo handoff

| Proposta | Motivo |
|---|---|
| §21–§27 — sette release di roadmap bot (v0.1 → v0.7+) con ~90 titoli di issue | Il repository ha **quattro** release pianificate (`v0.1`–`v0.4`) e il registry ammette `v0.1 \| v0.2 \| future`. Una progressione a sette assi creerebbe una terza numerazione accanto alle due che il §20 stesso avverte di non confondere. Le capability per release sono utili come *ordine di lavoro* e restano nel sorgente archiviato |
| §21 — Epic v0.1 «Bot Baseline & Unified Scenario Execution» | Il §21 dice «non inventare numero se non esiste già», e infatti non serve: il bot baseline della v0.1 è `RT-FEAT-BOT-BASE`, **`RELEASE_READY`**, e la parte che manca è `RT-FEAT-BOT-FAIRNESS` sotto E13 |
| §17 — sette famiglie di telemetria (match, unit, ability, capability, combo, reaction, mappa) | Nessun consumatore. È il difetto ricorrente del progetto — il dato che nessuno legge — e la casa naturale è `RT-FEAT-TOOL-BALANCE-GROUND`, che dichiara già di non avere «un banco che li eserciti in modo ripetibile» |
| §29 — ~50 ScenarioId su sei release | Stessa ragione del §4.4: uno scenario pianificato per un sistema senza forma è un nome che verrà rinominato |
| §30 — otto Editor Task | Come il primo handoff: sedute bloccate da lavoro non iniziato. Restano al §6 |
| §32 — aggiornamento di dieci PDR | I PDR `v0.1` sono snapshot di consultazione ([D-009](../../decisions/RT_PDR_00_Decision_Log.md)); gli owner correnti sono i documenti di `docs/`, e sono quelli aggiornati |

### 9.5 Aperto

Il §20 chiede di collegare **due assi** — la progressione dell'harness (`Harness v0.1…v0.6+`) e quella delle
release di gioco — senza confonderli. Il primo **non esiste nel repository**: zero occorrenze. Esiste
`RT-FEAT-TEST-SCENARIO-HARNESS` (`INTEGRATED`, E15), che è il presente dell'harness, non la sua progressione.

Non è stato creato: un asse di numerazione nuovo, senza owner e senza gate, sarebbe la terza vista di stato
da tenere allineata a mano — ed è esattamente la gara che la Editor Map ha già perso una volta
([`roadmap-editor.md`](../roadmap-editor.md), ritirata il 2026-08-08 perché *«tre tracker sincronizzati a
mano diventano tre verità diverse»*). Se servirà, nasce **generato**, come è rinata la Editor Map.
