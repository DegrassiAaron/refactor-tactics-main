> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-13**, lo stesso giorno in cui è arrivato. Era in `docs/src/`, untracked.
>
> **Recepito come conferma, non come delta.** Il documento dichiarava di non essere una source of truth e
> aveva ragione. Ogni affermazione di stato ricontrollata è risultata **corretta**, e sono queste: `#152`,
> `#165`, `#166`, `#314`, `#319` aperte · `#318` e `#361` chiuse · `RT-FEAT-CORE-DECISION-TIME-BANK` a
> `SPECIFIED` con i dieci gate dichiarati · dieci scenari `planned` nel registry · `D-050`…`D-057` a log ·
> `spec-turnlog.md` §4.2 con `BankConsumed`/`BankAfter`/`BankExhausted` · `FRTReactionOpportunity` in
> `Source/` · nessun runtime del bank e nessun file `Spec.TimeBank.*` su disco. Non ha prodotto né una
> decisione né una feature né un'epic, ed è l'esito giusto per un consolidamento di qualcosa che era già
> consolidato.
>
> ⚠️ **Il suo `HEAD` era già vecchio di 17 commit** — dichiarava `744a25b8` (2026-08-13 11:15) contro
> `0cff74ec`. Nessuna delle sue affermazioni ne è stata invalidata, ma la distanza va letta prima di fidarsi
> di una fotografia.
>
> 🔴 **Il valore reale è stato ciò che il documento *non* diceva.** Affermava che `#318` e `#361` erano chiuse
> — vero — e si fermava lì, senza verificare che i **derivati** l'avessero recepito. Non l'avevano fatto:
> cinque righe in quattro file `CURRENT` più il «prerequisito bloccante» di `#319` dichiaravano ancora
> assente una capability consegnata il 2026-08-10, con un conteggio (`otto`, `undici`) superato dalla
> riconciliazione a **dieci** e **tredici**. Corretti dalla PR di questo consolidamento; il gate che li
> avrebbe presi è [`#738`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/738).
>
> **Owner canonici** (invariati): [`spec-decision-time-bank.md`](../../../gameplay/spec-decision-time-bank.md) ·
> [`spec-turnlog.md`](../../../technical/spec-turnlog.md) §4.2 ·
> [`adr-0004-finestre-di-reazione.md`](../../../decisions/adr-0004-finestre-di-reazione.md).
> **Predecessore**: [`2026-08-09-decision-time-bank.md`](2026-08-09-decision-time-bank.md).

# RefactorTactics — Decision Time Bank
## Consolidamento aggiornato allo stato reale del progetto — 2026-08-13

> **Tipo documento:** handoff / delta di consolidamento per Claude Code  
> **Repository verificato:** `DegrassiAaron/refactor-tactics-main`  
> **Branch:** `main`  
> **HEAD verificato:** `744a25b869f11bfb2dbc7eb97af93df5850b64c8`  
> **Baseline Unreal canonica:** **UE 5.8.1**
>
> Questo documento **NON è una nuova source of truth**. Serve a spiegare cosa della vecchia discussione
> sul Time Bank è già stato assorbito dal repository, cosa è stato corretto/superato e cosa resta realmente
> da implementare.
>
> Prima di modificare qualsiasi cosa, Claude deve rileggere HEAD e usare come fonti canoniche:
>
> - `docs/decisions/RT_PDR_00_Decision_Log.md`
> - `docs/gameplay/spec-decision-time-bank.md`
> - `docs/decisions/adr-0004-finestre-di-reazione.md`
> - `docs/technical/spec-turnlog.md`
> - `docs/roadmap/feature-registry.yaml`
> - `docs/roadmap/roadmap-v0.1.md`
> - `docs/technical/scenario-map.md`
> - Epic GitHub **#152**
> - checkpoint GitHub **#319**
>
> Il vecchio handoff del 2026-08-09 è già stato consumato ed è archiviato:
> `docs/archive/src/handoff/2026-08-09-decision-time-bank.md`.
>
> **Guardrail principale:** non ricreare spec, feature, Epic o issue che esistono già.

---

# 1. Perché questo aggiornamento

La discussione originale proponeva:

```text
30 s per giocatore / match
1 s di grace
3 s max per Decision Window
possibile bank pubblico
timeout Overwatch = HOLD
```

Da allora il progetto ha consolidato la feature in modo molto più preciso.

La direzione generale è rimasta:

> un budget temporale per giocatore, condiviso fra le Decision Window live, che limita lo stalling senza
> allungare la singola Fast Reaction.

Ma alcune proposte della chat sono state **superate**.

Le tre correzioni più importanti sono:

```text
VECCHIA CHAT                           STATO CANONICO ATTUALE

30 s fissi per player                 InitialBank DERIVATO dal formato
bank eventualmente pubblico          bank OWNER-ONLY
Time Bank = anti-stall generale       riduce pacing solo sulle finestre single-responder;
                                      sui Reaction Clash a reveal fisso non accorcia la wall-clock
```

---

# 2. Stato generale del progetto rilevante per questa feature

La v0.1 attuale è:

```text
vertical slice 2v2
offline contro bot
griglia esagonale multilivello
UE 5.8.1
```

Roster player-facing corrente:

```text
Gadget
Phase
Riktor
Wraith
```

Gli Stable ID tecnici restano:

```text
Hero.Flux
Hero.Riva
Hero.Bastion
Hero.Vektor
```

Non rinominare gli Stable ID per adeguarli al display name.

Il formato partita è data-driven:

```text
2v2 RoundLimit: 10–14
valore iniziale corrente: 12
Planning: 30 s nel 2v2 corrente
Fast Reaction: 3.0 s
Resolution target 2v2: circa 8–15 s
```

La durata e il numero di round non sono costanti globali: vedi D-010.

---

# 3. Stato E14 reale

Epic esistente:

```text
#152
[EPIC v0.1] E14 — Overwatch e reazioni interattive
```

Checkpoint:

```text
CP 14.1  #161  ✅ chiuso
CP 14.2  #162  ✅ chiuso
CP 14.3  #163  ✅ chiuso
CP 14.4  #164  ✅ chiuso

CP 14.5  #165  ⬜ aperto
CP 14.6  #166  ⬜ aperto
CP 14.7  #314  ⬜ aperto
CP 14.8  #319  ⬜ aperto
```

Ordine vincolante:

```text
14.5 -> 14.6 -> 14.7 -> 14.8
```

Il Time Bank è **CP 14.8**, non una nuova Epic.

Non creare:

```text
nuova Epic "Time Bank"
nuovo checkpoint parallelo
nuovo Reaction System
```

---

# 4. Cosa esiste già nel runtime E14

Il repository non è più allo stato della vecchia chat.

Sono già implementati:

```text
FRTReactionOpportunity
identity dell'opportunity
modello opportunity -> commit
Overwatch armata
trigger Overwatch valutato durante i micro-step
opportunity prodotta dal movimento
test del trigger / opportunity
```

Stato sintetico:

```text
Planning
  -> arma Overwatch
Resolution
  -> micro-step
  -> trigger
  -> FRTReactionOpportunity
```

Ciò che **non esiste ancora** è la finestra live completa:

```text
Opportunity
  -> Decision Window reale
  -> FIRE / HOLD
  -> timeout
  -> resume segmentato
```

Questo è CP 14.5.

Quindi CP 14.8 non va implementato saltando direttamente dentro il resolver:
deve innestarsi sulla Decision Window consegnata da CP 14.5.

---

# 5. Feature Registry esistente

Feature già creata:

```text
RT-FEAT-CORE-DECISION-TIME-BANK
```

Titolo:

```text
Decision Time Bank (budget di decisione per giocatore)
```

Stato attuale:

```text
SPECIFIED
```

Release:

```text
v0.1
```

Priorità:

```text
P3
```

Roadmap:

```text
Epic E14
Checkpoint 14.8
```

Dipendenze attuali:

```text
RT-FEAT-CORE-DECISION-BOUNDARY
RT-FEAT-CORE-TURNLOG
RT-FEAT-REACTION-FAST
RT-FEAT-MATCH-PACING
```

Gate attuali:

```text
spec                  done
data                  todo
runtime               todo
log_debug             todo
automation            todo
scenario              todo
ui_wiki               partial
packaged              todo
network_privacy       todo
replay_representable  todo
```

Non duplicare la feature.

Quando viene implementata, aggiornare **questa** riga del registry e rigenerare le viste derivate.

---

# 6. Decisioni canoniche D-050 ... D-057

## D-050 — esiste un Decision Time Bank

CANONICAL.

```text
un bank per GIOCATORE
condiviso da tutte le Decision Window live
non per ability
non per opportunity
non per finestra
```

È parte di v0.1 come CP 14.8.

## D-051 — il bank non allunga la finestra

CANONICAL.

```text
MaxWindow = FastReactionDuration
```

Oggi:

```text
FastReactionDuration = 3.0 s
```

Non introdurre un secondo nome/configurazione se duplica lo stesso concetto.

Le risposte legali non cambiano in base al bank.

A bank zero:

```text
meno tempo
stesse opzioni
```

## D-052 — Grace ed ExhaustedGrace

Forma decisa, numeri ancora PLAYTEST.

Baseline:

```text
Grace = 1.0 s
ExhaustedGrace = 0.75 s
```

Il consumo parte solo dopo la Grace.

Il bank a zero **non elimina completamente** il tempo percettivo:
resta `ExhaustedGrace`.

I valori non diventano CANONICAL finché non passano i criteri di playtest.

## D-053 — timeout costa il massimo temporale

CANONICAL.

Una finestra che arriva a timeout consuma:

```text
MaxWindow - Grace
```

Con la baseline corrente:

```text
3.0 - 1.0 = 2.0 s
```

Questo **non** contraddice:

```text
Overwatch timeout -> HOLD
```

Perché:

```text
timeout spende il budget TEMPORALE
timeout NON spende la charge dell'abilità
timeout NON esegue FIRE
Overwatch resta armata se la policy lo consente
```

Requisito UX vincolante:

```text
il fallback deve essere preselezionato
e raggiungibile con un solo input entro la Grace
```

Su disconnessione conclamata:

```text
bank NON drena
```

## D-054 — privacy del bank

CANONICAL.

Il bank è:

```text
OWNER-ONLY
```

La proposta della vecchia chat di mostrarlo agli avversari è stata **esplicitamente scartata**.

Motivo:

```text
un delta pubblico può rivelare:
- che una Decision Window privata è esistita;
- quando il giocatore ha lockato;
- quanta parte della finestra ha consumato.
```

Questo riaprirebbe il side-channel temporale vietato da D-021.

Quindi:

```text
NO public bank
NO team-visible bank salvo nuova decisione
NO bucketed/rounded bank come workaround
```

Canary richiesto a livello network/privacy:

> la vista avversaria deve essere identica nel caso con e senza Decision Window privata.

## D-055 — bank e replay

CANONICAL.

Il wall-clock non entra nel resolver.

Il replay:

```text
LEGGE il bank dal TurnLog
NON riesegue il timer
NON ricalcola il bank dalla storia completa
```

Importante:

```text
ResolverConfigHash NON esiste nel progetto.
NON introdurlo.
```

I parametri del bank sono tempi di parete e devono stare accanto ai timing già esistenti, non in un secondo sistema.

## D-056 — InitialBank non è 30 secondi

CANONICAL nella forma; taratura futura.

Formula corrente:

```text
InitialBank =
    RoundLimit * (MaxWindow - Grace)
```

Con baseline:

```text
2v2, RoundLimit 12:
12 * (3 - 1)
= 24 s
```

Banda 2v2:

```text
RoundLimit 10–14
=> 20–28 s
```

Banda 3v3 ipotizzata:

```text
RoundLimit 16–20
=> 32–40 s
```

Quindi i **30 s fissi** della vecchia chat NON sono più il default del progetto.

## D-057 — bot

CANONICAL.

Il bot possiede un Time Bank.

Non creare:

```cpp
if (IsBot)
{
    SkipDecisionWindow();
}
```

La differenza vive nel `DecisionProvider`.

Per test/bot:

```text
Opportunity
-> deterministic policy
-> Response
-> lockOffset/costo temporale come DATO
```

Nessun `sleep`.

---

# 7. Correzione importante: Reaction Clash

Per una `Reaction Clash`:

```text
due partecipanti
scelta in cieco
reveal a scadenza FISSA
```

La finestra non chiude in anticipo quando entrambi lockano.

Motivo:

```text
il momento del lock sarebbe informazione sull'avversario
```

Conseguenza:

```text
single-responder:
Time Bank può accorciare wall-clock se il player risponde presto

Reaction Clash:
Time Bank NON accorcia la finestra;
serve come pressione/resource management
```

Non promettere quindi che il Time Bank risolva da solo tutto il pacing di E14.

---

# 8. TurnLog — decisione successiva già chiusa

La vecchia chat/handoff chiedeva di decidere come salvare:

```text
BankBeforeMs
BankConsumedMs
BankAfterMs
```

Quella domanda è già stata chiusa da **#361**.

Owner:

```text
docs/technical/spec-turnlog.md §4.2
```

Decisione:

```text
categoria Decision
due voci principali
nessun campo nuovo nel layout
```

Outcome:

```cpp
ERTDecisionOutcome::BankConsumed
ERTDecisionOutcome::BankAfter
ERTDecisionOutcome::BankExhausted
```

Semantica:

```text
BankConsumed:
Amount = ms consumati dalla decisione

BankAfter:
Amount = ms residui dopo la decisione

BankExhausted:
Amount = 0 quando il bank tocca il floor
```

`BankBefore` non viene serializzato come terzo dato perché:

```text
BankBefore = BankAfter + BankConsumed
```

per quella stessa decisione.

Non cambiare lo schema del TurnLog per aggiungere tre campi dedicati.

Issue **#361 è chiusa**.

---

# 9. Scenario Harness — stato aggiornato

Issue **#318 è chiusa**.

Ha già consegnato:

```text
LogEventCount
LogEventOrder
```

e il `FRTScenarioSession` accumula il TurnLog dei round della sessione.

La lista canonica nel Feature Registry per CP 14.8 è ora di **10 scenari pianificati**:

```text
Spec.TimeBank.GraceDoesNotDrain
Spec.TimeBank.DrainsAfterGrace
Spec.TimeBank.NeverBelowZero
Spec.TimeBank.TimeoutCostsFullWindow
Spec.TimeBank.TimeoutSpendsNoCharge
Spec.TimeBank.ClashCostsFullWindow
Spec.TimeBank.BotDrainsLikePlayer
Spec.TimeBank.ExhaustionKeepsResponsesLegal
Spec.TimeBank.ReplayReadsRecordedBank
Spec.TimeBank.PacketOrderInvariant
```

Due comportamenti Overwatch non sono duplicati come scenari TimeBank perché estendono test C++ propri:

```text
Overwatch.TimeoutIsHold
Overwatch.HoldKeepsArmed
```

Il test privacy non appartiene agli scenari offline v0.1: è funzionale/network e si dimostra a M10.

---

# 10. Issue GitHub: cosa esiste già

## Epic

```text
#152
E14 — Overwatch e reazioni interattive
```

## Time Bank

```text
#319
CP 14.8 — Decision Time Bank
OPEN
```

## Prerequisiti storici già chiusi

```text
#318
Harness: assertion TurnLog
CLOSED

#361
Formato Time Bank nel TurnLog + riconciliazione scenari
CLOSED
```

Non creare issue equivalenti.

---

# 11. Stato reale di #319

#319 è ancora aperta.

Il runtime Time Bank non è implementato.

Le aree di DoD principali sono:

```text
1. bank per-player server-authoritative
2. grace / exhausted grace
3. timeout full cost
4. fallback raggiungibile entro grace
5. disconnect non drena
6. owner-only privacy
7. replay legge bank registrato
8. timing policy nel layer wall-clock
9. InitialBank derivato da RoundLimit
10. bot DecisionProvider
11. scenari/test
12. UI
13. packaged / network privacy quando applicabile
```

Ordine:

```text
CP 14.5
    Decision Window funzionante
        |
CP 14.6
    UI + prima vera misura di pacing
        |
CP 14.7
    Reaction Profile / Clash
        |
CP 14.8
    Decision Time Bank
```

Non implementare #319 in anticipo.

---

# 12. Relazione con il pacing partita

Il Time Bank è uno degli strumenti di pacing, non il solo.

Metriche da conservare per CP 14.6 / 14.8:

```text
DecisionWindowCount
ResponseTimeMs
GraceOnlyResponseCount
BankConsumedMs
BankRemainingMs
TimeoutCount
BankExhaustionRound
ResolutionWallClock
single-responder wall-clock
contested-window wall-clock
```

Criteri di calibrazione già dichiarati:

```text
Grace:
p50 response < Grace
p90 response <= MaxWindow

InitialBank:
bank residuo mediano a fine match
fra 20% e 50% dell'iniziale
su >= 10 partite
```

---

# 13. Cosa è cambiato rispetto al file Claude della vecchia chat

| Vecchia richiesta | Stato oggi | Azione |
|---|---|---|
| Creare spec Time Bank | già fatta | usare `spec-decision-time-bank.md` |
| Creare FeatureId | già fatto | usare `RT-FEAT-CORE-DECISION-TIME-BANK` |
| Inserire in E14 | già fatto | CP 14.8 |
| Creare Epic | non serve | E14 è #152 |
| Creare issue Time Bank | già fatta | #319 |
| Decidere privacy | chiusa | owner-only, D-054 |
| Decidere initial bank | forma chiusa | derivato, D-056 |
| Decidere TurnLog | chiusa | `Decision` + `BankConsumed/BankAfter/BankExhausted` |
| Estendere harness | #318 chiusa | non rifare |
| Riconciliare scenari | #361 chiusa | 10 planned |
| Implementare runtime | resta | #319, dopo 14.5–14.7 |
| UI reale | resta | da fare |
| Playtest/taratura | resta | da fare |
| Privacy network packaged | resta | M10 / gate futuro |

---

# 14. Aggiornamenti recenti del progetto da non ignorare

Dalla discussione originaria il repository è avanzato molto oltre il Time Bank.

Alla HEAD verificata:

```text
- UE 5.8.1;
- gameplay quadrato parallelo rimosso;
- substrato hex multilivello;
- E8 terreni chiusa;
- cover direzionali basse/alte esistono;
- fine partita e RoundLimit data-driven esistono;
- Team Knowledge influenza già gameplay e bot;
- E18 Predictive Action è separata dalle Fast Reaction;
- Facing ulteriormente consolidato;
- roster display names Gadget / Phase / Riktor / Wraith;
- TurnLog versionato e più ricco della baseline PDR;
- Scenario Harness legge il TurnLog;
- E14 ha già opportunity e trigger Overwatch runtime.
```

La suite più recente citata dai commit del 2026-08-13 è arrivata a:

```text
781 test completati
0 falliti
```

Non copiare questo numero in documenti manuali: il progetto ha già introdotto la regola che il conteggio suite si genera/misura.

---

# 15. Cosa deve fare Claude ADESSO

## Step 1 — audit HEAD

```text
git status
git branch --show-current
git rev-parse HEAD
git log -10 --oneline
```

## Step 2 — leggere le source of truth

```text
docs/gameplay/spec-decision-time-bank.md
docs/decisions/RT_PDR_00_Decision_Log.md       D-050..D-057
docs/decisions/adr-0004-finestre-di-reazione.md
docs/technical/spec-turnlog.md                 §4.2
docs/roadmap/feature-registry.yaml             RT-FEAT-CORE-DECISION-TIME-BANK
docs/roadmap/roadmap-v0.1.md                   E14
docs/technical/scenario-map.md
```

## Step 3 — controllare GitHub

```text
#152
#165
#166
#314
#319
#318
#361
```

## Step 4 — gap matrix, prima di nuove issue

Confrontare:

```text
DoD #319
vs
runtime attuale
vs
test attuali
vs
scenari attuali
vs
UI attuale
```

Creare child issue soltanto per delta reali e autonomi.

## Step 5 — rispettare l'ordine E14

Se CP 14.5 è ancora aperto:

```text
non iniziare il runtime del bank
```

È consentito correggere solo stale docs o inconsistenze reali.

---

# 16. Test che CP 14.8 deve infine dimostrare

```text
TimeBank.GraceDoesNotDrain
TimeBank.DrainsAfterGrace
TimeBank.TimeoutCostsFullWindow
TimeBank.TimeoutSpendsNoCharge
TimeBank.NeverBelowZero
TimeBank.ExhaustionKeepsResponsesLegal
TimeBank.ClashCostsFullWindow
TimeBank.BotDrainsLikePlayer
TimeBank.ReplayReadsRecordedBank
TimeBank.PacketOrderInvariant
```

In aggiunta:

```text
Overwatch.TimeoutIsHold
Overwatch.HoldKeepsArmed
TimeBank.FallbackReachableWithinGrace
```

Golden:

```text
NO sleep
NO real timer
```

La decisione può essere iniettata come:

```text
OpportunityId
Response
LockOffsetMs
```

---

# 17. Privacy/network — comportamento da preservare

Per la futura rete:

```text
Opponent must NOT receive:
- remaining bank
- bank delta
- existence of hidden Decision Window
- exact lock time
- responder identity if private
- timeout event if private
```

D-021 vale anche sul tempo.

Per Reaction Clash:

```text
i partecipanti sanno che il Clash esiste
ma non conoscono:
- risposta avversaria
- momento di lock
prima del reveal
```

---

# 18. Decisioni ancora realmente aperte

## Taratura

```text
Grace definitiva?
ExhaustedGrace definitiva?
formula InitialBank va mantenuta dopo playtest?
Refill resta 0?
```

## Network

```text
LatencyAllowance definitiva?
soglia late response vs disconnect?
policy reconnect completa?
```

## Pacing

```text
il bank è davvero necessario con i dati reali di CP 14.6?
quanto pesa sulle finestre single-responder?
quanto resta inutile sui Clash a scadenza fissa?
```

La feature però è già decisa; si calibra la sua intensità.

---

# 19. Guardrail aggiornati

NON:

```text
- creare una nuova Epic Time Bank;
- creare un nuovo FeatureId;
- usare 30 s fissi come valore canonico;
- rendere pubblico il bank;
- mettere il bank nel MatchFormatData;
- introdurre ResolverConfigHash;
- cambiare le AllowedResponses quando il bank è basso;
- portare ExhaustedGrace a zero senza nuova decisione;
- usare timer reali nei golden replay;
- far saltare la Decision Window ai bot con IsBot;
- far drenare il bank su disconnessione conclamata;
- riaprire #318 o #361;
- duplicare gli scenari Overwatch dentro `Spec.TimeBank.*`;
- dichiarare che il Time Bank riduce la durata del Reaction Clash;
- iniziare CP 14.8 prima dei suoi predecessori;
- copiare manualmente conteggi test o stato feature nelle viste generate.
```

---

# 20. Riepilogo esecutivo per Claude

```text
TIME BANK: NON E' PIU' UN'IDEA DA CONSOLIDARE.
E' UNA FEATURE SPECIFICATA E TRACCIATA, MA NON ANCORA IMPLEMENTATA.

Owner spec:
docs/gameplay/spec-decision-time-bank.md

Decisioni:
D-050 .. D-057

Feature:
RT-FEAT-CORE-DECISION-TIME-BANK
status SPECIFIED

Epic:
#152 / E14

Checkpoint:
#319 / CP 14.8 / OPEN

Prerequisiti:
#165 -> #166 -> #314 -> #319

Bank:
per player / per match / shared across live Decision Windows

Max individual window:
FastReactionDuration = 3.0 s

Grace:
1.0 s PROPOSED

ExhaustedGrace:
0.75 s PROPOSED

InitialBank:
RoundLimit * (MaxWindow - Grace)
non 30 s fisso

Timeout:
costa tutto il tempo oltre grace
ma Overwatch continua a fare HOLD e non consuma charge

Privacy:
OWNER-ONLY

Replay:
legge il bank registrato nel TurnLog

TurnLog:
Decision.BankConsumed
Decision.BankAfter
Decision.BankExhausted

Bot:
stessa infrastruttura, DecisionProvider deterministico

Reaction Clash:
reveal a deadline fissa;
il bank non riduce la sua wall-clock

Scenari planned:
10, già registrati nel Feature Registry

#318:
CLOSED

#361:
CLOSED
```

---

# 21. Output richiesto a Claude in una nuova sessione

Dopo l'audit, rispondere con:

```text
1. HEAD e stato workspace
2. Stato #165/#166/#314/#319
3. Stato reale di RT-FEAT-CORE-DECISION-TIME-BANK
4. Delta fra spec e runtime
5. Delta fra registry e GitHub
6. Delta fra scenario map e file Scenarios/
7. Delta Wiki/player-facing
8. Nessuna nuova issue / oppure sole issue veramente mancanti
9. Ordine di implementazione suggerito senza violare E14
10. Validator/test eseguiti
```

Se non esiste alcun delta documentale:

> NON modificare file solo per produrre un commit.

Il prossimo lavoro reale deve seguire la roadmap, non rigenerare il consolidamento già completato.
