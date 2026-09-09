# Spec panel — Ready per Participant/ControlGroup e coordinamento del bot alleato (#2193 · #534)

**Data**: 2026-09-09 · **Misurato su**: `main` `866bbd80` · **Modalità**: critique · **Focus**: requirements · architecture · testing
**Panel**: Wiegers (requisiti) · Cockburn (attore/goal) · Fowler (confini e ownership) · Nygard (failure mode) · Adzic (esempi) · Crispin (testabilità) · Newman (evoluzione)

> Referto della sessione di specifica richiesta dal work order *«CLAUDE WORK ORDER — Ready per
> Player/ControlGroup + bot delay»* (decisione 2026-09-07, revisione operativa 2026-09-09).
> **Non è un owner**: la regola resta di `spec-durata-partita-e-scala-mappe.md` §7, l'owner Ready è la issue
> che la riconciliazione designerà, la policy di replanning del bot è di
> [`../../gameplay/spec-bot-tattico.md`](../../gameplay/spec-bot-tattico.md) §3 e di #534.
>
> ⛔ **Questo referto non ha modificato documentazione live né GitHub.** Nessuna issue riaperta, aggiornata o
> creata; nessuna Epic creata. La riconciliazione (§5 e §6 del work order) resta da eseguire, e questo referto
> dice **cosa** portarci e **dove**.

---

## 0. Cosa è stato rimisurato prima del panel

Il work order dichiara cinque FATTI e chiede di riverificarli. Rimisurati su `866bbd80` e su GitHub live.

| FATTO | Dichiarato dal work order | Misurato oggi | Esito |
|---|---|---|---|
| **A** — #2193 owner Ready | chiusa il 2026-09-04, `RequestLockIn`/`CancelLockIn`, countdown 3 s | `RTTurnManager.h:297,308,2121` — `ReadyCountdownSeconds = 3.f`; issue **CLOSED** `2026-09-04T17:33Z` | ✅ regge |
| **B** — #2325 vieta un secondo owner Ready | issue aperta, CR-MATCH | **OPEN**, `[ROADMAP] Capability Roadmaps` | ✅ regge |
| **C** — modello multi-Hero già consegnato | `UnitsPerPlayer`, `ControlGroup`, `CanPlayerControlUnitInGroup` | `RTCombatLibrary.cpp:72-101`, `RTGameMode.cpp:820-856`, `RTPlayerState.h:44-75`, `RTUnit.h:78` | ✅ regge — **ma non significa quello che il work order gli fa dire** (→ F1) |
| **D** — #2358 «owner HUD **aperto**» | da consolidare perché ancora aperta | **CLOSED** `2026-09-04T23:57Z`; il consumatore esiste (`RTHUD.cpp:1038-1078`, riga `Ready - <n>s`) | 🔴 **stale** |
| **E** — #2356 «se ancora aperta» | canary da rimisurare | **CLOSED** `2026-09-04T20:33Z` | 🔴 **stale** |

Misure aggiuntive che il work order non aveva:

| Misura | Esito |
|---|---|
| stato Ready per partecipante, oggi | **non esiste**: `grep` di `bReady`/`bIsReady` in `Source/` fuori dai test dà solo `CanUseAbility` e il countdown HUD. Il Ready è un **evento** (`RequestLockIn()`), non uno stato |
| quante persone per squadra in v0.1 | **una**: `UnitsPerPlayer = 2` su `UnitsPerTeam = 2` ⇒ `UnitsPerTeam / UnitsPerPlayer = 1` (`RTMatchFormatData.h:60`, `RTCombatLibrary.h:367`) — *«un posto per squadra e un gruppo solo»* |
| dove vive il bot alleato | **dentro la squadra 0 e dentro il gruppo 0**: `bIsBotControlled = (TeamId == 1) \|\| Config.bAutobattle \|\| bBotAlly` (`RTMatchBootstrapper.cpp:322`), `bBotAlly = (TeamId == 0) && (Slot >= Team0Size - BotAllies)` (`:590`); `ControlGroup` non lo guarda (`RTGameMode.cpp:853`) |
| quando pianifica il bot | **una volta, a inizio Planning**: `PlanBots()` da `StartPlanningTimer` (`RTTurnManager.cpp:1713`, *«il bot pianifica a inizio turno»*) |
| cosa fa un secondo passaggio di `PlanBots()` | **azzera i campi del piano** (`RTCombatLibrary.cpp:68`) e **riscrive la conoscenza di Planning** (`RTTurnManager.cpp:807-808`) |
| `TeamPlanningRevision` o equivalente in codice | **assente**: `grep` in `Source/` e `docs/` non trova né `TeamPlanningRevision` né `PlanningRevision` |
| owner della privacy dell'intento | **esiste ed è uno**: `URTCombatLibrary::IsIntentVisibleTo` (#507), consumato da `RTIntentPrivacyLibrary.cpp:47` e `RTReactionWindowView.cpp:28` |
| issue live che nominano già questa semantica | **#534** `CP 26.4 · Isteresi: il bot non cambia piano per un delta minimo` — **OPEN**, E26, post-v0.1; **#782** `CP 40.4 · Protocollo ready/commit: reliable e idempotente` — **OPEN**, P0 post-v0.1; **#1902** compagno bot e ricognizione — **OPEN** |

🔑 **L'ultima riga cambia la mappa della ownership**, ed è il finding F2.

---

## 1. Findings

### 🔴 F1 — Il soggetto del Ready non può essere il ControlGroup: in v0.1 umano e bot alleato stanno nello stesso gruppo — *Cockburn · Fowler*

Il work order fissa due regole e le presenta come compatibili:

- §3.1/3.2: *«Non deve esistere uno stato valido Character A = Ready / Character B = Not Ready all'interno dello stesso ControlGroup»*;
- §3.3: *«Human Participant = READY / Ally Bot Participant = NOT READY / EVALUATING»*.

Misurato: `ControlGroup = IndexInTeam / UnitsPerPlayer` (`RTCombatLibrary.cpp:81`), e la v0.1 dichiara
`UnitsPerPlayer = 2` su `UnitsPerTeam = 2`. **Un gruppo per squadra.** Il bot alleato non è un altro gruppo:
è un'unità *dello stesso gruppo* con `bIsBotControlled = true` — ed è precisamente il caso che l'header di
`CanPlayerControlUnit` già descrive: *«con un compagno pianificato dal bot (`rt.Match.BotAllies`) la squadra 0
contiene entrambi i casi»*.

Quindi, nel formato che v0.1 gioca, le due regole sono la stessa frase con due esiti opposti.

E c'è di peggio: **le due metà della decisione non coesistono in nessuna configurazione v0.1**.

| `BotAllyCount` | Chi comanda cosa | «due Hero, un Ready» | finestra di coordinamento |
|---|---|---|---|
| `0` | l'umano comanda A e B | ✅ è il caso | ⛔ **nessun bot alleato**: la finestra non ha soggetto |
| `1` | l'umano comanda A, il bot comanda B | ⛔ **l'umano ha un Hero solo** | ✅ è il caso |

**Fix — e non richiede un nuovo sistema di ownership** (il guardrail del work order regge):

> Il soggetto del Ready è il **controller** (seat umano, oppure l'insieme delle unità pianificate dal bot),
> non il gruppo. `ControlGroup` resta ciò che è già: la chiave di **autorizzazione**, consegnata da #1124.
> L'invariante si riscrive: **nessuno split Ready fra le unità dello stesso controller**.

Con questa forma entrambe le regole diventano vere insieme, e il caso `UnitsPerPlayer = 2, BotAllyCount = 0`
— dove il controller umano comanda A e B — è esattamente il requisito «premere Ready su uno rende Ready
entrambi». ⚠️ Ma va detto in chiaro: **quel caso e la finestra del bot non si osservano nella stessa partita
finché il formato ha un posto per squadra.**

### 🔴 F2 — La metà «bot» ha già un owner aperto, e portarla in #2193 apre il secondo owner che #2325 vieta — *Fowler*

#534 (**OPEN**, CP 26.4, E26, post-v0.1) dichiara nel proprio scope, testualmente:

- *«**Ripianificazione su revisione**, non su timer: il planner reagisce a un cambio di `TeamPlanningRevision`, non a un intervallo»*;
- *«**Ready/unready** del bot integrato col ciclo di planning reale: un piano stabile porta a Ready, un cambio significativo dell'alleato lo toglie»*;
- e cita `D-096`, **accettata il 2026-08-11**: *«il tempo reale decide quando si smette di ripianificare, mai quale piano esce»*.

Il work order propone la stessa identica separazione come **inferenza nuova** (§3.3: *«il 1.0 s decide quando
applicare la decisione, non quale decisione viene scelta»*) e la porta in #2193. Non è nuova: è `D-096`, e ha
già un owner documentale (`spec-bot-tattico.md` §3) e un owner issue (#534).

Portarla in #2193 replica il difetto di #2325 sull'altro asse: **due owner per la policy di replanning del bot**.

**Fix — split della ownership lungo la linea che il repository ha già:**

```text
Ready core (v0.1)                          Coordinamento del bot (E26)
owner: lignaggio #2193                     owner: #534 · spec-bot-tattico.md §3 · D-096
- soggetto del Ready = controller          - finestra di coordinamento
- propagazione a tutte le sue unità        - replanning su revisione del piano alleato
- quorum                                   - isteresi: cosa è un «cambio significativo»
- countdown 3 s esistente                  - revoca del Ready del bot
- PlanningMax vince                        - soglia e grace: buchi dichiarati, da playtest
```

L'interfaccia fra i due è **una** e va scritta in entrambi: *il partecipante bot dichiara Ready quando il suo
piano è finalizzato; l'owner Ready non sa perché ci ha messo quel tempo.*

### 🔴 F3 — La «PROPOSTA da validare» è già decisa, e la sua forma corretta è più stretta di quella scritta — *Wiegers*

Il work order (§4) marca come *«INFERENZA FORTE / PROPOSTA da validare nello spec panel»* il caso
«bot già Ready + l'umano cambia piano ⇒ il bot perde Ready».

**Il panel non la valida: constata che è già in scope di #534**, con una differenza che conta.

| | Work order | #534 |
|---|---|---|
| innesco della revoca | *«qualunque modifica **rilevante** del piano alleato»* | *«un cambio **significativo** dell'alleato»*, dove *significativo* è definito dalla soglia: `NuovoScore > AttualeScore + Soglia` |

La versione del work order, applicata alla lettera, produce il difetto che #534 esiste per prevenire —
*«un bot che ripianifica a ogni revisione diventa illeggibile»* — e apre un failure mode nuovo:

```text
umano: micro-modifica del piano  ->  Ready del bot revocato  ->  nuova finestra 1 s
       ripetuto                  ->  il quorum non si raggiunge mai
       fino a                    ->  PlanningMax
```

In solo è autoinflitto e innocuo. In co-op — cioè appena #782 atterra — è un **vettore di griefing**: un
compagno può impedire il commit anticipato a tutta la squadra muovendo un waypoint avanti e indietro.

**Fix**: la revoca del Ready del bot passa dalla **stessa soglia** della ripianificazione. Un cambio sotto
soglia non tocca il Ready del bot. E il tetto resta l'unica garanzia di terminazione — che è già vero, ma va
scritto come proprietà e non come effetto.

### 🔴 F4 — «Il bot rivaluta» non è una rilettura: oggi il secondo passaggio azzera i piani e riscrive la conoscenza — *Nygard*

Misurato:

| Sito | Cosa dice |
|---|---|
| `RTTurnManager.cpp:1713` | `PlanBots(); // il bot pianifica a inizio turno` |
| `RTCombatLibrary.cpp:68` | *«la pianificazione che vince e' sempre quella di `PlanBots()`, che gira a inizio turno e **azzera i campi del piano**»* |
| `RTTurnManager.cpp:807-808` | *«si riaprono a ogni passaggio, perche' `PlanBots` gira due volte sullo stesso turno …: **la conoscenza di Planning viene riscritta dal secondo passaggio**»* |

`PlanBots()` pianifica **tutti** i bot, con uno snapshot per squadra (#1088). Rieseguirlo al Ready umano
significherebbe: ripianificare anche i bot **avversari**, riscrivere la conoscenza di squadra a metà
pianificazione, e azzerare piani già scritti. Cioè rompere, nello stesso gesto, i due vincoli che il work
order dichiara di voler proteggere — la privacy cross-team e l'assenza di impatto su resolver e replay.

**Fix**: la finestra richiede un percorso di replanning **circoscritto al partecipante bot alleato**, che non
resetta i piani altrui e non rinfresca la conoscenza. Ed è falsificabile senza ambiguità:

> dopo la finestra, i piani dei bot della squadra avversaria sono **identici** a quelli di prima — stessa
> `PlannedCell`, stesso `PlannedPath`, stessa abilità, stessa reazione.

⚠️ Questo è **il costo reale della decisione**, e il work order non lo nomina: non è «un timer», è un secondo
punto di ingresso nel planner che oggi ha un ingresso solo.

### 🔴 F5 — Due FATTI sono stale, e la conseguenza non è cosmetica — *Wiegers*

`#2358` e `#2356` sono **chiuse** (misurato oggi, §0). Il work order §6.3 istruisce *«se è ancora aperta e non
attivamente claimata … estendi la sua definizione UI»*: quel ramo non ha soggetto.

Conseguenza operativa, che il work order lasciava condizionale e ora non lo è più: **oggi non esiste un owner
Ready aperto, né core né UI.** La scelta *«riapri #2193»* contro *«un solo successore designato da #2325»* non
è più un'alternativa fra un ramo comodo e uno di ripiego — è **la prima decisione** della riconciliazione, e va
presa prima di scrivere qualunque criterio, perché decide dove i criteri vanno scritti.

⚠️ E la stessa domanda si ripresenta per la presentazione: la riga di stato che nomina il Ready e il gesto per
annullarlo esiste (`RTHUD.cpp:1038-1085`), ma **non conosce due partecipanti**: ha una parola sola per lo stato
della partita. «Human READY / Bot pending» non ha, oggi, né owner aperto né consumatore.

### 🟡 F6 — Il quorum non ha membri dichiarati, e la definizione sbagliata blocca l'autobattle — *Cockburn · Nygard*

Il work order usa «quorum» come se fosse definito e non dice mai **chi ne fa parte**. Le tre letture possibili
non sono equivalenti:

| Membri del quorum | Conseguenza misurabile |
|---|---|
| tutti i partecipanti, bot avversari inclusi | i bot avversari sono Ready da inizio Planning (pianificano lì): il quorum si chiuderebbe subito e **l'autobattle committerebbe dopo 3 s invece che al tetto** — cambia il ritmo di ogni partita non presidiata |
| solo i partecipanti di squadra 0 | funziona in v0.1, **non** in PvP: è una regola che si rompe da sola quando #782 atterra |
| i partecipanti **che possono dichiarare Ready** (seat umani + bot alleati di un umano) | v0.1 e PvP coincidono; l'autobattle ha quorum **vuoto** e resta sul percorso del tetto, che è il comportamento odierno |

La terza è l'unica che non cambia nulla di ciò che già funziona. Ma va **scritta**, con il suo caso degenere:
*quorum vuoto ⇒ nessun commit anticipato*. Senza, il primo test che allestisce un autobattle scopre la regola
per caduta — ed è esattamente ciò che #2356 è costata una volta.

### 🟡 F7 — Senza una manopola, tutti i nuovi criteri diventano test a orologio — *Crispin*

Precedente misurato: il countdown è testabile headless perché ha una manopola che lo annulla —
`SetReadyCountdownSeconds(0)` rende il commit sincrono (`RTHexMatchIntegrationTests.cpp:1024`,
`RTAutobattleInputInertTests.cpp:395`), e l'header lo dichiara: *«`0` non e' «countdown istantaneo»: e'
«nessun countdown»»* (`RTTurnManager.h:2117`).

La finestra da 1 s ha bisogno della stessa cosa, o *«il bot non è Ready prima di 1 s»* diventa un test che
misura il `FPlatformTime` della macchina di CI.

E alcune voci della lista del work order §7 **non sono falsificabili come scritte**:

| Voce | Perché non è un criterio |
|---|---|
| *«bot Evaluating/Pending è leggibile e non sembra input lag»* | «sembra» non è osservabile da un asserto — è una voce PIE, e va dichiarata tale |
| *«il bot produce il nuovo piano atteso»* | *atteso* da chi? Serve lo scenario che fissa l'ingresso e il piano di uscita |
| *«Variare il delay non cambia la scelta bot»* | vera solo **a parità di revisione consumata**: senza quel qualificatore è falsa per costruzione, perché una revisione più tarda è un ingresso diverso |
| *«Unready durante countdown conserva comportamento #2193»* | già coperto da `.UnreadyReturnsToPlanningWithThePlanIntact`: è un test esistente, non un criterio nuovo |

### 🟡 F8 — Due orologi in serie costano 4 s dove il progetto ne aveva scelto 3 — *Nygard · Doumont*

`Human Ready → 1 s → quorum → 3 s → Commit` porta il percorso minimo a **≈ 4 s**. Il work order lo dichiara
onestamente (*«non nascondere questo costo»*), e fa bene. Ma vale la domanda che il panel deve porre.

La spec dichiara **perché** il countdown esiste in v0.1, e non è l'attesa reciproca: *«il valore che il
countdown porta oggi non è l'attesa reciproca, è annullare una chiusura involontaria»* (§7.2). Se il countdown
è la finestra di ripensamento dell'umano, e la finestra del bot è il tempo che serve al bot per finalizzare,
**i due intervalli non hanno motivo di essere in serie**:

```text
in serie (work order)        Ready -> [1 s bot] -> quorum -> [3 s countdown] -> Commit     ~4 s

in parallelo (alternativa)   Ready -> [3 s countdown]                        -> Commit     ~3 s
                                   -> [1 s bot   ] -> finalizzato
                             il commit richiede: countdown scaduto AND ogni partecipante finalizzato
```

In parallelo si conserva tutto ciò che la decisione chiede — il bot che pensa è osservabile, l'Unready
funziona, il replanning sull'ultima revisione resta — al prezzo di **zero** secondi. Si perde una cosa sola: la
regola *«tutti Ready ⇒ countdown»* non vale più letteralmente per il partecipante bot.

→ **DOMANDA all'autore** (§3, sotto). Il panel non la decide: cambia una regola consolidata di §7.2.

### 🔵 F9 — Nessuno scenario eseguibile — *Adzic*

Il work order elenca criteri in prosa. Gli scenari sono in §4 di questo referto.

### 🔵 F10 — #782 è il consumatore futuro, e questa forma o lo aiuta o gli costa — *Newman*

`CP 40.4 · Protocollo ready/commit: reliable e idempotente` (**OPEN**, P0, post-v0.1) è il punto in cui il
Ready diventa un messaggio. Un Ready **per partecipante, idempotente, con revoca esplicita** è esattamente la
forma che quel CP dovrà replicare; un Ready per unità o un Ready implicito nel countdown non lo è.

Non anticipare nulla di rete. Ma la scelta di F1 — soggetto = controller, con identità stabile — è quella che
regge il passaggio, e questo è un argomento in più per prenderla ora.

---

## 2. Cosa resta vero del work order, e va portato invariato

Il panel non tocca queste, e le conferma con la loro misura:

| Requisito | Perché regge |
|---|---|
| nessun nuovo `ERTMatchPhase` | la readiness è uno stato dentro Planning; nessuna fase nuova è richiesta da nessun criterio |
| `PlanningMax` vince sempre | è strutturale, non un `if`: i due orologi chiamano `LockInAndResolve`, che li spegne entrambi (spec §7.2) — e la finestra del bot deve entrare nella stessa struttura, non accanto |
| il delay non entra in snapshot / `TurnLog` / `StateHash` | classificazione già decisa (spec §11, *Tempi UX*); il criterio di verifica è un `grep`, non un giudizio |
| nessuna nuova Epic | confermato: la decisione si distribuisce fra due owner esistenti (F2), e nessuno dei due è nuovo |
| privacy cross-team | ⚠️ **e ha già un owner**: `IsIntentVisibleTo` (#507) con i suoi consumatori. Il criterio non è «scrivere una regola di privacy» ma **non aprire un secondo canale**: il piano alleato che il bot consuma deve passare da lì |
| `1.0 s` come valore | ✅ come **baseline da playtestare** (spec §19, bucket 🧪), non come costante consolidata. #534 dichiara il *grace* di stabilità prima del Ready un **buco dichiarato**: *«UX da playtest, non decisioni di questo CP»* |

---

## 3. Le domande che il panel non può chiudere

| # | Domanda | Perché serve l'autore |
|---|---|---|
| **Q1** | Riaprire #2193 o designare **un** successore da #2325? | Entrambi i candidati sono chiusi (F5). È governance, non tecnica |
| **Q2** | I due intervalli in **serie** (≈4 s) o in **parallelo** (≈3 s)? | Cambia §7.2, che è consolidata (F8) |
| **Q3** | La finestra del bot alleato è **v0.1** o **E26 post-v0.1**? | #534 è post-v0.1 (F2). Se è v0.1, lo scope di #534 va **diviso**, non duplicato — e la parte tirata avanti va nominata |
| **Q4** | In v0.1 il caso «due Hero, un Ready» si osserva solo con `BotAllyCount = 0`, dove il bot alleato non esiste (F1). Si accetta che le due metà non siano osservabili nella stessa partita, o il formato cambia? | È una scelta di prodotto sul formato, non una conseguenza tecnica |

---

## 4. Scenari eseguibili — *Adzic*

```gherkin
# --- Owner Ready ---

Scenario: il Ready copre tutte le unità dello stesso controller
  Dato Skirmish2v2 con UnitsPerPlayer = 2 e BotAllyCount = 0
    E il seat umano comanda A e B, entrambe in ControlGroup 0 della squadra 0
  Quando l'umano dichiara Ready con A selezionata
  Allora il partecipante umano risulta Ready
    E lo stato Ready osservabile per A è identico a quello per B
    E non esiste alcun istante in cui A risulta Ready e B no

Scenario: il gesto è simmetrico, e l'Unready lo è quanto il Ready
  Dato il partecipante umano Ready con A e B
  Quando l'umano dichiara Unready con B selezionata
  Allora A e B risultano entrambe Not Ready
    E il piano di A e quello di B sono invariati

Scenario: gruppi distinti non si contaminano
  Dato un formato con due posti per squadra
    E il seat P0 Ready, il seat P1 non Ready
  Allora il quorum non è raggiunto
    E nessun countdown è armato

Scenario: quorum vuoto — l'autobattle non cambia comportamento
  Dato un autobattle, senza alcun partecipante che possa dichiarare Ready
  Quando la pianificazione scorre
  Allora nessun commit anticipato viene armato
    E il commit resta quello del tetto di Planning

Scenario: il tetto vince sulla finestra
  Dato il partecipante bot in valutazione
  Quando PlanningSeconds scade
  Allora il turno committa per timeout
    E la callback pendente della finestra non arma alcun Ready dopo l'uscita da Planning

# --- Owner coordinamento bot (#534) ---

Scenario: il bot finalizza sull'ultima revisione, non su quella di partenza
  Dato l'umano che dichiara Ready con il piano alla revisione R1
    E il partecipante bot che apre la finestra di coordinamento
  Quando l'umano porta il piano alla revisione R2 prima della scadenza
  Allora il piano prodotto dal bot è quello che R2 determina
    E non quello che R1 determinava

Scenario: sotto soglia il bot non si muove — e non perde il Ready
  Dato il partecipante bot Ready sul piano alleato R1
  Quando l'umano produce R2 il cui delta di punteggio è sotto la soglia di isteresi
  Allora il piano del bot è invariato
    E il partecipante bot resta Ready

Scenario: sopra soglia il bot revoca e rivaluta
  Dato il partecipante bot Ready sul piano alleato R1
  Quando l'umano produce R2 il cui delta di punteggio supera la soglia
  Allora il partecipante bot torna Not Ready
    E riapre la finestra sull'ultima revisione
    E il countdown eventualmente armato è annullato

Scenario: solo l'ultima generazione può completare
  Dato due revisioni del piano alleato prodotte in rapida successione
  Quando entrambe le valutazioni pendenti scadono
  Allora solo quella della revisione più recente porta il bot a Ready

Scenario: la finestra non tocca l'avversario
  Dato lo stato dei piani dei bot di squadra 1 rilevato prima del Ready umano
  Quando la finestra di coordinamento si apre, scade e il bot alleato finalizza
  Allora i piani dei bot di squadra 1 sono invariati
    E nessun dato del piano umano è transitato verso la squadra 1

Scenario: il tempo di parete non decide quale piano esce
  Dato lo stesso stato canonico e la stessa revisione del piano alleato
  Quando la finestra vale 1.0 s, poi 0 s, poi 4.0 s
  Allora il piano prodotto dal bot è lo stesso nei tre casi
    E il TurnLog del turno committato è lo stesso
```

---

## 5. Definizione operativa da portare nell'owner Ready

> Criteri per la sezione datata che la riconciliazione aggiunge all'owner Ready (Q1). **Non mescolare** con i
> criteri che hanno chiuso il lavoro del 2026-09-04: quelli descrivono un'altra consegna.

### Scope

- Il **soggetto** del Ready è il partecipante = controller. `ControlGroup` resta la chiave di autorizzazione consegnata da #1124 e non viene ridefinito.
- Il Ready di un partecipante vale per **tutte** le unità che quel controller comanda.
- **Quorum** fra i partecipanti che possono dichiarare Ready; quorum vuoto ⇒ nessun commit anticipato.
- Riuso del countdown esistente e di `RequestLockIn()` / `CancelLockIn()`.
- ⛔ **Fuori scope**: la finestra di coordinamento del bot, la soglia di isteresi e la revoca del Ready del bot → owner #534.

### Autorità

```text
UI propone Ready
  -> l'autorità risolve il partecipante dal controller
  -> l'autorità cambia il Ready del partecipante
  -> l'HUD deriva lo stato mostrato per ogni unità
```

La UI non scrive lo stato di due Character. Non esiste un `bReady` per unità.

### Criteri

| ID | Criterio | Come si falsifica |
|---|---|---|
| **D011** | Il Ready è uno stato del partecipante, non dell'unità né del ControlGroup | `grep` di un campo Ready per unità in `Source/` deve restare a zero occorrenze |
| **D012** | Ready dichiarato da una qualunque unità del controller ⇒ tutte le sue unità risultano Ready | scenario «il Ready copre tutte le unità dello stesso controller» |
| **D013** | Unready simmetrico: tutte tornano Not Ready, nessun piano perso | scenario «il gesto è simmetrico» + `.UnreadyReturnsToPlanningWithThePlanIntact` esistente |
| **D014** | Non esiste uno stato osservabile in cui due unità dello stesso controller hanno Ready diverso | asserto sull'invariante, non sull'HUD |
| **D015** | Il countdown parte **solo** al quorum, e `RequestLockIn()` è chiamata **una volta** | contatore di chiamate nello scenario a due partecipanti |
| **D016** | Quorum vuoto ⇒ nessun commit anticipato; l'autobattle committa come oggi | scenario «quorum vuoto»; regressione su `Match.Autobattle.*` |
| **D017** | Partecipanti di seat/gruppi distinti non si contaminano | scenario «gruppi distinti» |
| **D018** | `PlanningMax` vince su qualunque stato di readiness pendente; all'uscita da Planning ogni callback pendente è innocua | scenario «il tetto vince» |
| **D019** | Nessun campo di readiness o di tempo di parete entra in snapshot, `TurnLog` o `StateHash` | `grep` sui costruttori dello snapshot e sul calcolo dell'hash |
| **D020** | L'input di Ready resta inerte dove `IsPlanningInputInert()` lo dichiara | regressione su `RTAutobattleInputInertTests` |
| **D021** | La readiness non introduce un `ERTMatchPhase` nuovo | enumerazione invariata |
| **D022** | La presentazione distingue «partecipante Ready / altro partecipante pendente» senza inventare un secondo owner della regola | l'HUD legge; se il consumatore non esiste, è un gap da dichiarare, non da colmare qui |

### Criteri per l'owner #534 — da portare **là**, non qui

| Criterio | Come si falsifica |
|---|---|
| il partecipante bot non dichiara Ready prima della fine della finestra | manopola che azzera la finestra + scenario, **non** un'attesa di parete |
| alla scadenza il bot consuma l'**ultima** revisione del piano alleato | scenario «finalizza sull'ultima revisione» |
| un delta sotto soglia non cambia il piano **e** non revoca il Ready | scenario «sotto soglia» — è il simmetrico che #534 già richiede |
| un delta sopra soglia revoca il Ready e riapre la finestra | scenario «sopra soglia» |
| una sola valutazione valida per partecipante e per revisione | scenario «solo l'ultima generazione» |
| a parità di stato canonico e revisione, la durata della finestra non cambia il piano né il `TurnLog` | scenario «il tempo di parete non decide» — è la forma testabile di `D-096` |
| il replanning del bot alleato non altera i piani dei bot avversari né rinfresca la conoscenza di squadra | confronto dei piani della squadra 1 prima/dopo (F4) |
| il piano alleato consumato passa da `IsIntentVisibleTo`, senza un secondo canale | `grep` dei consumatori del piano umano lato bot |
| nessuna lettura di wall-clock nel percorso che calcola il punteggio | `grep` di `FPlatformTime` / `DeltaTime` in `Bot/` — criterio già scritto in #534 |

### DoD del consolidamento

- un solo owner Ready, e uno solo per la policy bot — **due owner distinti, non due owner della stessa cosa**;
- nessuna Epic nuova;
- nessun secondo sistema di ownership del Player;
- `spec-durata-partita-e-scala-mappe.md` §7 aggiornata, con `1.0 s` nel bucket 🧪 di §19;
- `test-manuali-pie.md` / `PIE-V01-READY` esteso solo per ciò che un test non vede;
- #1124, #534, #782, #1902 cross-linkati; #2358 e #2356 citate come **chiuse**;
- `NOT RUN` dichiarato per ogni gate non eseguito.

---

## 6. Cosa questo panel lascia esplicitamente fuori

- **Non scrive codice**: il work order lo vieta finché la definizione non è chiusa, e l'implementazione parte dalla issue owner.
- **Non giudica il valore di `1.0 s`**: è una misura da playtest, e dichiararla decisa qui sarebbe inventare un dato.
- **Non decide la soglia di isteresi**: è un buco dichiarato di #534.

---

## 7. Esito — le risposte dell'autore e la riconciliazione eseguita

**Risposte del 2026-09-09**, che chiudono le domande di §3:

| # | Domanda | Risposta |
|---|---|---|
| **Q1** | riaprire #2193 o successore unico? | **riaprire #2193** |
| **Q2** | i due intervalli in serie o in parallelo? | **in serie** — `Human Ready → finestra bot → quorum → countdown 3 s → Commit`, ≈ 4 s |
| **Q3** | finestra bot in v0.1 o post-v0.1? | **post-v0.1**, owner #534 |
| **Q4** | si accetta che le due metà non siano osservabili nella stessa partita? | **sì**, e va dichiarato (fatto: §7.3 della spec, `D013` di #2193, `PIE-V01-READY`) |

**Riconciliazione eseguita nello stesso passaggio.**

Documentazione:

| File | Cosa è stato consolidato |
|---|---|
| `docs/gameplay/spec-durata-partita-e-scala-mappe.md` | **§7.3** nuova — soggetto del Ready, propagazione, quorum, quorum vuoto, e la dichiarazione che in v0.1 non cambia nulla di osservabile; il coordinamento del bot come sottosezione **post-v0.1**. §7.2 qualificata sulla terza strada verso l'osservabilità. §19: la finestra fra le **baseline da playtestare** |
| `docs/roadmap/capability-roadmaps.md` | `CR-MATCH`: tre owner di cose diverse (#2193 · #534 · #782); risolta la nota sul confine d'autorità del countdown |
| `docs/technical/test-manuali-pie.md` | `PIE-V01-READY` estesa con l'invariante fra le unità dello stesso partecipante e il **divieto** di cercarvi oggi il bot che «pensa» |
| `docs/roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md` | riga 8 qualificata: il quorum diventerà osservabile in 2v2 con #534 |
| `docs/product/piano-canonico-mvp.md` | **verificato, non stale** — non modificato |

GitHub:

| Issue | Azione |
|---|---|
| **#2193** | **riaperta** + sezione datata *«Estensione 2026-09-07»* in coda al corpo (`D011`–`D019`). `D001`–`D010` **intatti** |
| **#2325** | `CR-MATCH` aggiornata: owner unico confermato, i tre anelli distinti, confine d'autorità risolto |
| **#534** | commento di assegnazione: possiede la finestra, il replanning, la soglia e la revoca — con le due correzioni del panel (revoca sopra soglia; `PlanBots()` non è rieseguibile a costo zero) |
| #1124 · #2358 · #2356 | **non toccate** — chiuse, citate come tali |

⛔ **Nessuna Epic creata. Nessuna issue creata.** Owner Ready aperti: **uno**.

Verifiche: `git diff --check` **PASS** · `doc-links` **PASS** · `doc-tables` **PASS** ·
`issue-refs` **1 finding preesistente** su #2753, estraneo · suite Unreal **NOT RUN** — questo passaggio non
tocca codice.

**Prossimo passo**: `/issue-run 2193`, che parte da `D016` e ha in `D019` la propria DoD.
