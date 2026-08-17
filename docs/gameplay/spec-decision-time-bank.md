# Decision Time Bank — specifica di design

> `CURRENT` · **Stato**: design deciso dall'autore il **2026-08-09**, ID del Decision Log da assegnare al merge
> **Owner**: questo file · **Epic**: E14 come **CP 14.8**, dopo [CP 14.7](spec-reaction-clash-e14.md) · **Release**: v0.1
> **Dipende da**: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) (accettato) · CP 14.5–14.6 per la calibrazione
> **Contesto sorgente**: discussione del 2026-08-09, kit `RefactorTactics_Decision_Time_Bank_Claude_Consolidation_2026-08-09.md`
> **Audit di provenienza**: [`decision-time-bank-conflict-report-2026-08-09.md`](../roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md)
> **Esteso il 2026-08-17** ([D-156](../decisions/RT_PDR_00_Decision_Log.md), [D-157](../decisions/RT_PDR_00_Decision_Log.md)):
> carico di controllo (§3.4) e Preferred Response (§4.4), dal kit
> [`…MultiHero_TimeBank_PreferredReaction_2026-08-17`](../archive/src/RefactorTactics_Claude_MultiHero_TimeBank_PreferredReaction_2026-08-17.md),
> filtrato dallo [spec panel](../roadmap/plans/multihero-timebank-preferred-response-spec-panel-2026-08-17.md).
>
> Le **regole** di questo documento sono decise. I **valori numerici** restano `PROPOSED FOR PLAYTEST` con i
> criteri di promozione di §3.2: non vanno pubblicati come definitivi né sulla Wiki né altrove.

---

## 1. Problema, e cosa questa feature risolve davvero

[ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) accetta un rischio dichiarato: la resolution può
allungarsi senza limite, perché `MaxPromptsPerReaction = 3` limita **una** reaction ma nessun cap aggregato
limita il turno (decisione **D20**). Il caso peggiore documentato è `3 × 3,0 s = 9 s` per **una sola** unità
armata, contro i 12 s di `Resolution_sec` del workbook.

Il Decision Time Bank è una **risorsa temporale per giocatore**, condivisa da tutte le Decision Window live
della Resolution: chi decide in fretta la conserva, chi usa tutta la finestra la consuma.

### 1.1 Portata dichiarata

| Classe di finestra | Il bank riduce la wall-clock? | Cosa fa il bank |
|---|---|---|
| Single-responder (Overwatch, reazioni ≥ 2 risposte) | ✅ sì | il commit chiude la finestra: rispondere presto accorcia davvero la resolution |
| **Contested / Reaction Clash** | ❌ **no** | il reveal è a **scadenza fissa** ([`spec-reaction-clash-e14.md`](spec-reaction-clash-e14.md) §7.1): la finestra costa 3,0 s anche se entrambi lockano subito |

> Questa riga è la più importante del documento e va detta prima di tutto il resto: **sulla classe di finestre
> più costosa il Time Bank non produce alcun guadagno di pacing.** Lì è uno strumento di pressione strategica,
> non di durata. Presentarlo come soluzione generale anti-stalling sovrastima ciò che consegna.

### 1.2 Cosa il bank **non** è

- non allunga la singola Fast Reaction: `FastReactionDuration` resta **3,0 s** e la finestra resta bounded;
- non è un timer per abilità: è **uno** per giocatore, come `FastReactionDuration` è **uno** di sistema;
- non è un anti-AFK timer: l'anti-AFK è il timeout, che esiste già ed è una funzione pura;
- **non cresce con il numero di Hero controllati** — nemmeno di un decimo di secondo. Chi ne controlla due
  riceve un **budget aggregato** più largo (§3.4), non una finestra più lunga: allungare la finestra
  allungherebbe la Resolution a ogni prompt, cioè peggiorerebbe il problema di pacing che il bank esiste per
  contenere. *(invariante di [D-156](../decisions/RT_PDR_00_Decision_Log.md))*

---

## 2. Collocazione — CP 14.8, e cosa resta di D20

> Il Time Bank entra in **v0.1** come **CP 14.8**, senza gate. *(decisione dell'autore, 2026-08-09)*

La misura di CP 14.5/14.6 **non** è più una condizione di ammissione: resta un **input di calibrazione**, ed è
il primo dato con cui si tarano `InitialBank` e `Grace` secondo i criteri di §3.2.

### 2.1 Rapporto con D20 — rischio accettato, dichiarato

D20 aveva scelto *nessun cap aggregato*, rimandando la misura. Il Time Bank **è** un cap aggregato, in tempo
anziché in prompt: costruirlo prima della misura sostituisce quella decisione con una nuova, presa senza il
dato che l'avrebbe informata.

Il rischio è **accettato e dichiarato**, nella stessa forma in cui D20 dichiarava il suo:

| Cosa può succedere | Come ce ne accorgiamo |
|---|---|
| CP 14.5/14.6 misura una resolution già sotto i 20 s | il bank non aveva un problema da risolvere: i parametri di §3.2 si tarano larghi e la feature diventa presenza leggera, non si smonta |
| I due rientri di ADR-0004 (cap per turno · `MaxPromptsPerReaction = 1`) sarebbero bastati | restano disponibili: sono parametri e non sono in conflitto col bank. Si possono attivare *insieme* |

Conseguenza operativa: **CP 14.8 non precede CP 14.5/14.6**. L'ordine dei checkpoint resta quello della
roadmap, così la prima misura arriva comunque prima della taratura.

### 2.2 Cosa questo non autorizza

Il bank non tocca il modello di reazione: il trigger resta puro, la finestra resta bounded a 3,0 s, le risposte
legali non cambiano mai (§5). Se la misura di CP 14.6 mostrasse che il problema di pacing è altrove, si tarano
i valori — non si riapre [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md).

---

## 3. Modello

### 3.1 Grammatica

```
Decision Window aperta
  ├─ [0 .. Grace]                  → FREE       nessun consumo
  ├─ (Grace .. MaxWindow]          → DRAIN      consumo = (LockTime − Grace − LatencyAllowance)
  └─ scadenza senza lock           → TIMEOUT    consumo = MaxWindow − Grace, il massimo (§4)
```

Il bank è **per giocatore** e **per match**. `BankFloor = 0`: mai negativo.

### 3.2 Parametri

| Parametro | Baseline proposta | Stato | Criterio di promozione a `CANONICAL` |
|---|---|---|---|
| `InitialBankMs` | `RoundLimit × (MaxWindow − Grace)` → **24 s** in 2v2, 32–40 s in 3v3 | `PROPOSED` | il bank residuo mediano a fine match cade fra il 20 % e il 50 % dell'iniziale su ≥ 10 partite |
| `GraceMs` | **1,0 s** | `PROPOSED` | il **p50** delle risposte in playtest cade sotto la grace e il **p90** non supera `MaxWindow` |
| `MaxWindowMs` | — | **`CANONICAL`** | è `FastReactionDuration` = 3,0 s. **Non introdurre un secondo nome** |
| costo del timeout | **`MaxWindow − Grace`** = 2,0 s | **deciso** | non è un parametro: è la durata realmente occupata (§4) |
| `ExhaustedGraceMs` | **0,75 s** | `PROPOSED` | il p90 delle risposte *a bank esaurito* resta sotto la soglia: se lo supera, la finestra ridotta sta escludendo il giocatore invece di metterlo sotto pressione |
| `LatencyAllowanceMs` | `min(HalfRTT, 250 ms)` | `PROPOSED` | vedi §8 |
| `RefillPerTurnMs` | **0** | `PROPOSED` | variante futura, fuori dal primo consolidamento |
| `LoadFactor(n)` | `1` per `n = 1`; **1,75** per `n = 2` | `PROPOSED` | §3.4 — un solo parametro libero, con limiti argomentati |
| `ExtraControlledHeroGraceMs` | **+0,50 s** per Hero oltre il primo | `PROPOSED` | §3.4 — `TB-9`, non promuovibile prima di `TB-8` |
| `ExtraControlledHeroExhaustedGraceMs` | **+0,25 s** per Hero oltre il primo | `PROPOSED` | §3.4 — stesso vincolo di sopra |

> **`InitialBankMs` è derivato, non inventato.** I «30 s» della discussione erano l'unico numero senza
> ancoraggio. `RoundLimit` esiste già in `URTMatchFormatData` (10–14 in 2v2, 16–20 in 3v3, implementato e
> testato a CP 10.3): derivarne il bank lo fa scalare col formato da solo, e toglie un numero magico a un
> progetto che ha appena finito di toglierne altri.

### 3.3 Esempi

```
Grace = 1,0 s · MaxWindow = 3,0 s · LatencyAllowance = 0 (offline)

lock a 0,7 s   → consumo 0,0 s      (dentro la grace)
lock a 2,4 s   → consumo 1,4 s
lock a 3,0 s   → consumo 2,0 s
timeout        → consumo 2,0 s      (§4 — la finestra è stata occupata per intero)
```

Lockare `HOLD` a 0,4 s e lasciare scadere producono lo **stesso esito** e costi opposti: 0,0 s contro 2,0 s.
È il gradiente che rende il bank uno strumento di pacing, e il motivo per cui §4.2 è vincolante.

### 3.4 Carico di controllo — [D-156](../decisions/RT_PDR_00_Decision_Log.md)

> Il bank è del **giocatore**, e un giocatore che decide per due Hero riceve più domande nello stesso turno.
> `InitialBank`, `Grace` ed `ExhaustedGrace` scalano con il numero di Hero **realmente controllati in questo
> match**; `MaxWindow` no, mai (§1.2).

Il conteggio non è la dimensione del roster e non è `UnitsPerTeam`: è quante unità quel giocatore ha il
diritto di comandare, e lo dichiara il formato
([`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) §16.4). In v0.1 — 2v2 offline
con **un** umano — i due numeri valgono entrambi `2` e la distinzione è invisibile: è il caso in cui leggere il
campo sbagliato passa ogni test, e per questo il campo giusto va **nominato**, non dedotto.

#### Il fattore sta dentro la derivazione, non dopo

[D-056](../decisions/RT_PDR_00_Decision_Log.md) ha reso `InitialBank` **derivato** per togliere un numero
magico. Moltiplicare il risultato per una costante lo rimetterebbe dentro, e fuori dalla formula, dove non
scala più con niente. La forma è quindi:

```
InitialBank = RoundLimit × (MaxWindow − Grace) × LoadFactor(ControlledHeroes)

LoadFactor(1) = 1            per costruzione: il caso di riferimento non si tocca
LoadFactor(n) ∈ [1, n]       il carico non riduce mai il budget, e due Hero non costano più di due giocatori
```

I due limiti sono argomentati, non assunti: sotto `1` un giocatore verrebbe punito per controllare di più,
sopra `n` riceverebbe più tempo di quanto ne riceverebbero `n` persone separate, e il bank smetterebbe di
essere un budget. **1,75 per `n = 2` è un punto dell'intervallo**, non una scelta: il playtest lo confronta con
`1,0` e `2,0`, che sono i due estremi con un significato.

#### Grace ed esaurimento

| Parametro | Forma | Con i valori proposti · 1 Hero | 2 Hero |
|---|---|---|---|
| `Grace` | `GraceMs + ExtraControlledHeroGraceMs × (n − 1)` | 1,00 s | 1,50 s |
| `ExhaustedGrace` | `ExhaustedGraceMs + ExtraControlledHeroExhaustedGraceMs × (n − 1)` | 0,75 s | 1,00 s |
| `MaxWindow` | **invariante** | 3,00 s | **3,00 s** |

> ⚠️ **La variante `+0,75 s` di Grace per Hero extra è registrata e non adottata.** Porterebbe la grace a
> 1,75 s su una finestra da 3,00 s: più della metà della finestra diventerebbe gratuita, e il gradiente di
> §3.3 — che è ciò che rende il bank uno strumento di pacing — si dimezzerebbe. Resta un candidato di
> playtest sotto `TB-9`, non una baseline alternativa.
>
> 🔴 E vale per tutti e tre i coefficienti: **si tarano su un valore che non è tarato**. `GraceMs = 1,0 s` è
> `PROPOSED`, e il suo criterio di promozione (§3.2) dipende da un p50 che CP 14.6 non ha ancora misurato.
> Promuovere un moltiplicatore prima della sua base significa misurare due incognite con un esperimento solo:
> `TB-9` è esplicitamente **bloccata da `TB-8`** (§17).

#### L'effetto composto — da leggere prima di tarare

⚠️ **La Grace entra due volte, e la seconda è facile da non vedere.** `InitialBank` è derivato da
`(MaxWindow − Grace)`: una grace più larga **riduce** il costo di un timeout e, con lo stesso `RoundLimit`,
riduce anche il bank. Con `RoundLimit = 12` e i valori proposti:

| | `Grace` | costo di un timeout | `InitialBank` | timeout prima dell'esaurimento | **per Hero** |
|---|--:|--:|--:|--:|--:|
| 1 Hero | 1,00 s | 2,00 s | `12 × 2,00 × 1,00` = **24,0 s** | 12 | **12** |
| 2 Hero | 1,50 s | 1,50 s | `12 × 1,50 × 1,75` = **31,5 s** | 21 | **10,5** |

Il risultato è che chi controlla due Hero riceve un budget più largo in assoluto e **leggermente più stretto
per unità** — che è la direzione giusta, ma è un esito *derivato*, non un obiettivo dichiarato. Va detto
perché la taratura di `LoadFactor` non si fa a mente: cambiare `ExtraControlledHeroGraceMs` sposta anche la
colonna del bank, e chi ne modifica uno solo pensando di isolare una variabile ne muove tre.

#### Cosa questo non cambia

- il bank resta **uno** per giocatore, condiviso fra tutte le sue finestre: il carico cambia la **taglia**,
  non il numero di bank;
- le `AllowedResponses` restano invariate (§5): il carico non tocca le opzioni, come non le tocca
  l'esaurimento;
- i parametri restano **wall-clock** (§6.1) anche se leggono un valore di regola dal formato — leggere
  `ControlledHeroes` non li rende regola, esattamente come non lo faceva leggere `RoundLimit`;
- il conteggio è un **input canonico registrato** come il residuo: il replay lo legge, non lo ricalcola dal
  numero di unità vive al momento della lettura, che cambia durante la partita.

---

## 4. Costo del timeout — pieno, e cosa lo rende legittimo

> Una finestra chiusa per scadenza costa `MaxWindow − Grace`, cioè il **massimo**. Il bank paga il tempo
> **occupato**, non l'intenzione. *(decisione dell'autore, 2026-08-09)*

### 4.1 Precisazione su ADR-0004 §3

[ADR-0004 §3](../decisions/adr-0004-finestre-di-reazione.md) stabilisce che il timeout non produce mai `FIRE`,
*«perché consuma una risorsa irreversibile e un mancato input non deve spenderla»*. Il soggetto di quel divieto
è la **charge** dell'Overwatch: un consumo binario e irreversibile di un'abilità.

> **Precisazione (2026-08-09)**: il divieto di §3 riguarda le **risorse di abilità** — charge, cooldown, stato
> del dispositivo. Il Decision Time Bank è un **budget temporale continuo**, non una risorsa di abilità: il
> timeout lo consuma. La regola sostanziale di §3 resta integra — allo scadere non si spende `FIRE`, non si
> spende alcuna charge, e la reaction resta armata.

Questa è una lettura, non un emendamento: ADR-0004 non va riaperto. Va invece **registrata**, perché una
lettura implicita è esattamente ciò che [`DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md) esiste per
impedire.

### 4.2 Requisito vincolante: il fallback deve essere raggiungibile entro la grace

Il costo pieno è legittimo **solo** se ottenere il proprio default è a portata di mano. Lockare `HOLD` a 0,4 s
e lasciare scadere danno lo stesso esito: se il primo è facile, il timeout è *assenza* e tassarlo è giusto; se
è scomodo, il timeout diventa una tassa sull'interfaccia.

Quindi, come requisito e non come raccomandazione:

- il fallback di ogni Decision Definition è raggiungibile con **un solo input**, nessuna conferma, nessuna
  animazione bloccante, percorso equivalente su mouse, tastiera e controller;
- è **preselezionato** all'apertura della finestra: confermare il default non richiede prima di trovarlo;
- il tempo dall'apertura al primo input possibile — apparizione del prompt inclusa — sta **dentro la grace**.
  Se il prompt impiega 0,4 s a comparire, la grace utile è 0,6 s e va tarata di conseguenza.

L'ultimo punto è misurabile e va misurato: è il primo controllo di CP 14.8, prima di qualunque taratura.

### 4.3 Disconnessione — l'eccezione che resta

Su **disconnessione conclamata** il bank **non drena**: il giocatore assente riceve il fallback deterministico
senza pagare. Non è una deroga al costo pieno, è il riconoscimento che lì non c'è nessuna finestra da occupare:
la partita non sta aspettando una decisione, sta aspettando una riconnessione.

Senza questa eccezione, a M10 si innesca una spirale — lag o disconnessione → timeout → bank a zero → finestra
ridotta a `ExhaustedGrace` → altri timeout — in cui la qualità della linea decide la partita. La soglia che
distingue «lento» da «disconnesso» dipende dalla policy di rete e resta aperta come `TB-7`.

### 4.4 Preferred Response — [D-157](../decisions/RT_PDR_00_Decision_Log.md)

§4.2 chiede che il **default** sia preselezionato. La Preferred Response chiede il passo successivo: che a
essere preselezionata sia la risposta che il giocatore ha **scelto in anticipo**, quando è ancora legale.

> Un giocatore può dichiarare in planning, insieme alla reaction che arma, quale risposta preferisce se la
> finestra si aprirà. All'apertura, se quella risposta è ancora fra le `AllowedResponses`, è quella a essere
> preselezionata. Un `Confirm` la committa.

```
REACTION                          Preferred = FIRE:7, ancora legale
> FIRE  <                         → preselezionata
  HOLD
[CONFIRM]                         → un input, e spara

REACTION                          Preferred = FIRE:7, il bersaglio non è più un'opzione
> HOLD  <                         → si torna alla scelta sicura di §4.2
  FIRE:9
[CONFIRM]                         → un input, e tiene
```

#### I quattro invarianti

Sono la ragione per cui questa è una preferenza e non una macro, e nessuno dei quattro è negoziabile.

| # | Invariante | Perché |
|---|---|---|
| 1 | **Il timeout ignora la Preferred Response.** Allo scadere vale sempre `URTReactionOpportunityLibrary::DecisionOnTimeout`, cioè `HOLD` | ADR-0004 §3: un input mancato non spende una risorsa irreversibile. Se `FIRE` preferito diventasse `FIRE` allo scadere, l'assenza di input spenderebbe una charge — il divieto sarebbe aggirato dalla preselezione invece che dal codice |
| 2 | **Preselezionato non è committato.** Nessuna risorsa si muove finché non arriva un commit valido | vedere `FIRE` evidenziato non consuma la charge, non arma nulla, non entra nel TurnLog come decisione |
| 3 | **Non cambia le `AllowedResponses`.** La preferenza ordina la presentazione, non filtra la legalità | è la riga che la distingue dalla **condizione dichiarata** di [D-109](../decisions/RT_PDR_00_Decision_Log.md), che invece le riduce al trigger. Due dichiarazioni di planning, due effetti opposti, sullo stesso oggetto: se si confondono, un profilo a tre risposte ne mostra una sola e nessun test se ne accorge |
| 4 | **Il confronto è esatto.** Una preferenza si applica solo se compare **identica** fra le `AllowedResponses` | `FIRE:<UnitId>` porta il bersaglio dentro la stringa: con `FIRE:7` preferito e il solo `FIRE:9` legale, la risposta è *degrada alla scelta sicura*, non *spara a un altro*. Il predicato è `IsResponseAllowed`, che già fa confronto esatto su elenco chiuso — nessun matching parziale, nessuna riscrittura del bersaglio |

#### Dove vive, e dove non vive

Una reaction armata porta già una dichiarazione fatta dal decisore in planning:
`FRTArmedOverwatch::Condition` ([D-109](../decisions/RT_PDR_00_Decision_Log.md), *«vuota = nessuna»*). La
Preferred Response è la seconda, e sta **accanto a quella**, non altrove.

⚠️ **Non su `FRTReactionOpportunity`.** Quel tipo ha due campi (`Key`, `AllowedResponses`), è costruito dal
server e raggiunge il client: è il DTO che `Overwatch.OpportunityLeaksNoFuture` esiste per tenere pulito. La
preferenza è informazione **privata del decisore** (§7) e non ha ragione di attraversarlo.

#### Quick Confirm

Il percorso «prompt aperto → risposta preselezionata committata» deve essere **un solo input**, con percorso
equivalente su mouse, tastiera e controller, e il tempo fino a quell'input dentro la grace: è §4.2 applicata
alla preselezione, non un requisito nuovo.

⚠️ **La Reaction conosce un'azione semantica, mai un tasto.** Il binding sta nel mapping context, non dentro
la logica della finestra né dentro il widget.

> 🔴 **Space è già occupato, misurato il 2026-08-17**: `RTPlayerController.cpp:256` mappa `LockInAction` su
> `EKeys::SpaceBar`, cioè la chiusura del planning (`LockInAndResolve`). Planning e Decision Boundary non sono
> mai aperti insieme, quindi il riuso è **possibile** — ma è una decisione sul contesto di input, non un
> default naturale, e chi la prende deve dire perché non confonde chi gioca. Il mapping è costruito in C++ con
> `NewObject<UInputAction>` e `Config/DefaultInput.ini` non nomina `Space`: non c'è un asset da guardare, e il
> conflitto si vede solo leggendo il codice.

---

## 5. Bank esaurito

Con `RemainingBank == 0` la Decision Window non continua a concedere tempo esteso.

| Aspetto | Regola |
|---|---|
| Durata utile | solo `ExhaustedGraceMs`, poi timeout policy |
| Risposte legali | **invariate** — il bank limita il *tempo*, mai le opzioni |
| Timeout | resta la policy della Decision Definition (`HOLD` per Overwatch) |
| Bank | resta a 0, mai negativo |

Il bank esaurito non deve poter **cambiare l'esito** di una finestra rispetto a un giocatore con bank pieno che
risponde nello stesso modo: cambia solo quanto tempo ha per arrivarci.

---

## 6. Determinismo e replay

Il wall-clock non entra nel resolver. Il bank sopravvive a questo vincolo solo con una regola esplicita:

> Il bank residuo è un **input canonico registrato per boundary**. Il replay lo **legge dal TurnLog**, non lo
> ricalcola da un timer.

| Tempo | Ruolo |
|---|---|
| Simulation Time | dentro il segmento, invariato |
| Presentation Time | può rallentare, non decide ([ADR-0004 §3](../decisions/adr-0004-finestre-di-reazione.md)) |
| Decision Time | produce **un dato**: `Response` oppure `TimeoutResponse`, più `BankAfterMs` |
| Wall-clock | non è mai letto dal resolver |

### 6.1 Dove vivono i parametri — chiuso per precedente

> `DecisionTimingPolicy` è **wall-clock**, non regola: **non** entra in `URTMatchFormatData` e non contribuisce
> ad alcun hash. Sta accanto a `PlanningSeconds` e `MaxPlaybackSeconds`.

La regola esiste già, scritta in `Source/RefactorTactics/Turn/RTMatchFormatData.h`:

> *«Contiene solo parametri di **REGOLA**: input deterministici da cui l'esito dipende. I tempi di parete
> (`PlanningSeconds`, `MaxPlaybackSeconds`) **NON** stanno qui e restano `UPROPERTY` sul `TurnManager` —
> spostarli senza un consumatore creerebbe la seconda verità.»*

Il bank limita il **tempo**, mai le risposte legali (§5): dato lo stesso input di decisione, l'esito non cambia.
È esattamente la posizione di `PlanningSeconds`, che pure influenza *indirettamente* come si gioca senza per
questo essere una regola. Nessuna decisione nuova: si applica un precedente.

Due corollari:

- `ResolverConfigHash` **non esiste** nel codice del progetto — compare solo negli handoff. Non va introdotto
  qui: sarebbe la seconda verità che l'header citato esiste per impedire;
- `InitialBank` **legge** `RoundLimit` dalle `FRTMatchRules` (§3.2) e lo calcola a inizio match. Leggere un
  parametro di regola non rende il risultato un parametro di regola.

⚠️ **Non introdurre un secondo sistema di configurazione**: i parametri del bank stanno dove il progetto tiene
già i tempi di parete, non in un asset nuovo.

---

## 7. Privacy — chiusa, non aperta

> Il bank residuo è **owner-only**. Nessun valore, delta o evento derivato entra in un DTO destinato a un
> giocatore che non sia il proprietario.

La discussione proponeva di rendere il bank pubblico per abilitare tattiche di pressione. È attraente e va
**scartata**: la privacy review esiste già.

- [ADR-0004 §7-bis](../decisions/adr-0004-finestre-di-reazione.md) (**D-021**): il ritmo osservato non deve
  dipendere dal tempo di risposta di un altro giocatore. «Il canale non è il pacchetto, è la sua assenza.»
- [`spec-reaction-clash-e14.md`](spec-reaction-clash-e14.md) §7.1 (**D-048**): nel Clash il reveal è a
  **scadenza fissa** proprio perché *il momento del lock non sia osservabile*.

Un bank pubblico prende quell'istante e lo trasforma in un numero **persistente, quantificato e archiviato**:
è il canale che le due decisioni chiudono, in forma peggiore. Un delta di 1,4 s su un boundary in cui
l'avversario non sapeva esistere una finestra rivela che una finestra c'è stata, a chi, e quanto ha pensato.

| Opzione discussa | Esito |
|---|---|
| `A` owner-only | ✅ **scelta** |
| `B` team-only | ⚠️ ammissibile — gli intenti alleati sono già condivisi (invariante #6), ma nessun caso d'uso lo richiede: non si apre |
| `C` public live | ❌ scartata: viola D-021 |
| `D` public a boundary pubblici | ⚠️ residuo: anche aggregato, il delta resta correlato a finestre private. Riesaminabile solo con canary test a M10 |
| `E` public bucketed | ❌ scartata: quantizzare non decorrela |

**Verifica**: canary test come per l'invariante #6 — un giocatore riceve una finestra privata, e la vista
avversaria (DTO, replicazione, log pubblico, spettatore, late join) deve essere **identica** al caso senza
finestra.

### 7.1 Preferred Response e carico di controllo — cosa è privato, cosa non lo è

| Dato | Visibilità | Perché |
|---|---|---|
| `PreferredResponse` (§4.4) | **owner-only** | è un'intenzione dichiarata prima che la finestra esista. Rivelarla anticipa la risposta *prima del momento in cui la Reaction Definition permette il reveal*, che nel Clash è a scadenza fissa ([D-048](../decisions/RT_PDR_00_Decision_Log.md)) |
| Numero di Hero controllati | **pubblico se il formato lo rende pubblico** | è una proprietà del formato, non una decisione: in un 2v2 dove ognuno comanda due unità lo sanno entrambi prima di cominciare |
| `LoadFactor` applicato · bank residuo | **owner-only** | il residuo resta chiuso da §7; il fattore è derivabile dal formato, ma il **prodotto** è il bank |

⚠️ Il Quick Confirm non deve creare un canale: un input che arriva prima non deve produrre un pacchetto, un
tempo di elaborazione o un'animazione osservabili dall'avversario. È la stessa forma di `D-021` — *«il canale
non è il pacchetto, è la sua assenza»* — applicata a un input reso deliberatamente più veloce.

---

## 8. Rete e latenza

> Il bank non deve misurare la qualità della connessione.

Senza correttivo lo misura: se il server conta da apertura finestra ad arrivo del pacchetto, l'RTT è dentro la
misura. A 150 ms, 3 prompt per turno su 12 turni sono **5,4 s** su 24 — il 22 % del budget — bruciati dalla
linea.

```
BankConsumed = max(0, ServerReceive − WindowStart − Grace − min(HalfRTT, LatencyCap))
```

- `HalfRTT` viene dalla stima di rete già in uso, mai da un valore dichiarato dal client;
- `LatencyCap` (baseline 250 ms) impedisce che una latenza dichiarata alta diventi bank gratuito;
- il client **non** è autorevole su nulla: né sull'istante del click, né sulla deadline, né sul residuo.

**Validazione server minima** per ogni response: `DecisionId` valido · owner corretto · boundary ancora aperto ·
risposta fra le `AllowedResponses` · opportunity ancora valida · non già committata · deadline non superata.
Trasporto: quello del Fast Decision path esistente, non uno parallelo.

Questa sezione è **integralmente a carico di M10**: in v0.1 offline `LatencyAllowance = 0` e nulla di ciò è
verificabile. La seconda revisione di ADR-0004 è già prevista lì.

---

## 9. Portata di release — cosa fa il bank in v0.1

Il bank entra in **v0.1**, che è **offline 2v2 vs bot**: c'è **un solo** giocatore umano.

| Release | Cosa il bank produce | Cosa **non** può produrre |
|---|---|---|
| **v0.1** | pacing della resolution misurabile · sistema completo (runtime, TurnLog, UI, telemetria) validato prima della rete · pressione sul giocatore che indugia | pressione *competitiva*: non c'è avversario umano da leggere. La tattica «ha poco bank, forziamolo» non esiste ancora |
| **M10** | budget competitivo pieno · privacy temporale verificabile con canary · latenza reale | — |

Questa asimmetria è **dichiarata, non risolta**: in v0.1 il bank vale per il pacing e come infrastruttura, e il
suo valore competitivo arriva con la rete. Progettarlo ora significa che a M10 non c'è da retrofittarlo dentro
un modello di decisione già chiuso — che è il costo che questa scelta compra.

### 9.1 Il bot ha un bank

> Il bot possiede un bank come il giocatore umano, e lo consuma secondo la policy del suo `DecisionProvider`.

Non è simmetria estetica: è la regola del progetto che vieta i rami di caso nel core. Se il bank esistesse solo
per l'umano servirebbe un `if (IsBot)` dentro la Decision Window, e ogni scenario di pacing misurerebbe una
partita che il gioco non gioca. Con un solo percorso:

- gli scenari misurano un match completo e simmetrico;
- la telemetria di §14 è confrontabile fra i due lati;
- a M10 sostituire il `DecisionProvider` del bot con un giocatore umano non tocca il sistema del bank.

Il bot **non attende wall-clock**: la sua policy dichiara un costo (immediato, medio, pieno) e il bank drena di
quello. Vedi §12.

### 9.2 Un numero da non riusare

Il calcolo «30 s × 6 giocatori = 180 s» della discussione presupponeva un 3v3, formato che
[D-011](../decisions/RT_PDR_00_Decision_Log.md) dichiara **non deciso** (3v3 baseline, 4v4 solo stress) e che
non è v0.1. Con `InitialBank` derivato da `RoundLimit` (§3.2) quel calcolo non serve più: il budget aggregato
scala col formato e si legge dal formato stesso.

---

## 10. TurnLog

⚠️ [`spec-turnlog.md`](../technical/spec-turnlog.md) è l'**owner** dei nomi di evento e dei reason code: i nomi
qui sotto sono *requisiti informativi*, non identificatori. Non si ipotizzano nomi senza aprire quel file.

Il log deve poter rispondere, per ogni Decision Window: chi decideva, cosa poteva scegliere, cosa ha scelto,
quanto ha speso, quanto gli resta, e se è stato un timeout e perché.

| Informazione | Nota |
|---|---|
| `DecisionId` · `OpportunityId` · owner | riusare l'identità già in uso per le opportunity |
| `AllowedResponses` · `CanonicalResponse` | il replay riproduce **questa**, non il countdown |
| `BankBeforeMs` · `BankConsumedMs` · `BankAfterMs` | il residuo è un dato letto, non ricalcolato (§6) |
| `TimeoutReason` | distingue scadenza, disconnessione e bank esaurito: la (2) di §4 dipende da questa distinzione |

Il TurnLog autorevole è **server-side**. Nessuno di questi campi, per un giocatore diverso dall'owner, entra in
una vista replicata in-match (§7).

### 10.1 Il nome della categoria è già preso — da risolvere prima di scrivere il bank

🔴 **Misurato il 2026-08-17, e va risolto da chi implementa CP 14.8.**
[`spec-turnlog.md`](../technical/spec-turnlog.md) §4.2 (deciso il 2026-08-09 con `#361`) prescrive che
`ERTLogCategory` guadagni **`Decision`** in coda, e `RTTestScenario.h` lo ripete nel commento di
`LogEventAmount`. Ciò che è atterrato con CP 14.5 è però `ReactionDecision`:

```
Source/RefactorTactics/Turn/RTTurnLog.h:17
    enum class ERTLogCategory : uint8 { Move, Combat, Fallback, Reaction, Environment, Facing, Predictive, ReactionDecision };
```

Già serializzata in TurnLog v8 e letta da cinque call site. Aggiungerne una seconda in coda darebbe **due
categorie sulla stessa finestra** — esattamente il difetto che il commento di `ERTReactionDecisionOutcome`
argomenta di aver evitato tenendo un enum solo.

Le due uscite, e nessuna è ovvia: *(a)* il bank scrive dentro `ReactionDecision` con esiti propri
(`BankConsumed`, `BankAfter`), e §4.2 di `spec-turnlog.md` va emendata; *(b)* nasce `Decision` come categoria
distinta perché il bank serve anche finestre che **non** sono reazioni (è la ragione per cui il FeatureId è
`RT-FEAT-CORE-*` e non `-REACTION-*`, §14), e allora va detto perché due categorie non sono due verità.
La scelta è dell'owner del TurnLog e va fatta **prima** della prima riga di runtime, non dopo.

⚠️ La Preferred Response, invece, **non** entra nel log come dato proprio: l'esito competitivo dipende dalla
risposta committata, e `Outcome` distingue già `HOLD` scelto da `HOLD` scaduto. Registrarla servirebbe solo a
verificare una proprietà di UI, che è una verifica funzionale e non una del replay (§13).

---

## 11. UI

Requisiti, in ordine di vincolo:

1. **il fallback è raggiungibile entro la grace** — un tasto, nessuna conferma (§4.3): è vincolante;
2. countdown della finestra corrente sempre visibile;
3. distinzione leggibile fra fase **free** (grace, nessun drenaggio) e fase **drain**;
4. bank residuo visibile **al solo proprietario** (§7);
5. stato di esaurimento comunicato con forma o testo, **mai col solo colore**;
6. percorso tastiera/controller equivalente a quello del mouse;
7. **la risposta preselezionata è visibilmente tale** (§4.4), e si distingue da «prima della lista». Se il
   giocatore non vede *quale* risposta sta per confermare, il Quick Confirm diventa un tasto che fa qualcosa
   di ignoto — che è peggio del menu che sostituisce;
8. quando la Preferred Response **decade** perché non più legale, la UI lo dice invece di cambiare selezione in
   silenzio: chi ha dichiarato `FIRE` e si vede confermare `HOLD` deve sapere perché, o attribuirà al gioco un
   errore che non ha commesso.

> ⚠️ I requisiti (7) e (8) sono **verifiche PIE**, non test headless: la preselezione è uno stato di
> interfaccia e il TurnLog registra la risposta committata, non ciò che era evidenziato. Il testo pronto per
> [`test-manuali-pie.md`](../technical/test-manuali-pie.md) sta nel DoD di
> [`#319`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/319) — la voce non è stata scritta da
> questo consolidamento perché quel file appartiene a un'altra track (`D-139`), e la ragione è registrata
> nello [spec panel](../roadmap/plans/multihero-timebank-preferred-response-spec-panel-2026-08-17.md) §6.

Se il playtest mostra che i numeri peggiorano la lettura in tre secondi, si degrada la presentazione — barra
senza cifre — non il requisito (1).

---

## 12. Bot e test

Il bot **non** attende wall-clock. Espone un `DecisionProvider`: `Opportunity → canonical response`, con
policy dichiarate (`CommitFirstValid`, `Hold`, `Timeout`, `HoldFirstThenCommit`, …).

Per il bot il bank è **stato simulato**, non attesa reale (§9.1): consuma un costo dichiarato dalla policy,
così che gli scenari di pacing possano misurare un match completo senza sleep.

| Policy del bot | Costo dichiarato sul bank |
|---|---|
| `CommitFirstValid` · `Hold` | 0 — decisione immediata, dentro la grace |
| `HoldFirstThenCommit` · policy di valutazione | costo intermedio dichiarato dalla policy |
| `Timeout` | `MaxWindow − Grace`, come per l'umano (§4) |

Il costo è un **dato della policy**, non una misura: due esecuzioni dello stesso scenario devono produrre lo
stesso residuo, altrimenti il replay diverge.

> **Nessun `sleep` nei test deterministici.** I golden test iniettano `(OpportunityId → response, lockOffsetMs)`
> come dato. Il timer reale si usa solo nei test funzionali il cui oggetto **è** il timer.

---

## 13. Copertura di verifica

Nomi allineati alla tassonomia di [`scenario-map.md`](../technical/scenario-map.md) (`Spec.*`).
**Riconciliati con la mappa il 2026-08-09** (issue `#361`): prima di allora questa tabella diceva «da
confermare con l'owner» e i nomi erano stati scritti nella mappa lo stesso, in numero e livello diversi.

Il livello ora significa una cosa sola:

- **`harness`** — file `Spec.TimeBank.*` eseguibile dal RT Scenario Test Harness. Sono **tredici** dal
  2026-08-17 — erano dieci, e le tre nuove sono le righe `ControlLoad*` e `TimeoutIgnoresPreferredResponse` di
  §3.4 e §4.4; il conteggio è la somma delle righe marcate **harness** nella tabella sotto, non un numero
  incrementato a mano. Sono
  diventati scrivibili quando `#318` ha dato all'harness le assertion che leggono il TurnLog e `#361` ha
  deciso *come* il bank ci entra ([`spec-turnlog.md`](../technical/spec-turnlog.md) §4.2). Prima erano
  classificati `golden` e non erano esprimibili in nessuna forma automatica.
- **`estende`** — non è uno scenario nuovo: estende un test C++ che esiste già. Duplicarlo come file
  produrrebbe due verità sullo stesso comportamento.
- **`funzionale`** — verifica umana o di rete, fuori dalla portata dell'harness. `M10` = multiplayer, quindi
  **non v0.1**: la v0.1 è offline contro bot, e uno scenario di latenza non ha dove girare.

| Scenario | Verifica | Livello |
|---|---|---|
| `Spec.TimeBank.GraceDoesNotDrain` | lock dentro la grace ⇒ bank invariato | **harness** |
| `Spec.TimeBank.DrainsAfterGrace` | lock a 2,4 s con grace 1,0 ⇒ consumo 1,4 s | **harness** |
| `Spec.TimeBank.NeverBelowZero` | il floor regge anche con costi concatenati | **harness** |
| `Spec.TimeBank.TimeoutCostsFullWindow` | il timeout costa `MaxWindow − Grace` (§4) | **harness** |
| `Spec.TimeBank.TimeoutSpendsNoCharge` | allo scadere il bank cala **ma la charge no** e la reaction resta armata: è la §4.1 resa verificabile | **harness** |
| `Spec.TimeBank.FallbackReachableWithinGrace` | dall'apertura del prompt al primo input possibile passa meno della grace, su mouse e su controller (§4.2) | funzionale · UI |
| `Spec.TimeBank.DisconnectDoesNotDrain` | disconnessione conclamata ⇒ consumo 0 (§4.3) | funzionale |
| `Spec.TimeBank.BotDrainsLikePlayer` | il bank del bot esiste e drena secondo la policy; nessun ramo `IsBot` nel percorso della Decision Window (§9.1) | **harness** |
| `Spec.TimeBank.ExhaustionKeepsResponsesLegal` | a bank 0 le `AllowedResponses` sono invariate (§5) | **harness** |
| `Spec.TimeBank.OverwatchTimeoutIsHold` | invariato rispetto a `Overwatch.TimeoutIsHold`: **estendere quel test, non duplicarlo** | estende |
| `Spec.TimeBank.HoldKeepsReactionArmed` | idem con `Overwatch.HoldKeepsArmed` | estende |
| `Spec.TimeBank.ReplayReadsRecordedBank` | il replay non ricalcola il residuo (§6) | **harness** |
| `Spec.TimeBank.PacketOrderInvariant` | permutare l'ordine di arrivo non cambia esito né residui | **harness** |
| `Spec.TimeBank.RejectsLateResponse` · `…RejectsWrongOwner` · `…IgnoresClientTiming` | validazione server (§8) | funzionale |
| `Spec.TimeBank.LatencyIsNotCharged` | due client a RTT diverso che lockano allo stesso istante logico pagano lo stesso | funzionale · M10 |
| `Spec.TimeBank.PrivacyNoBankLeak` | canary: vista avversaria identica con e senza finestra privata (§7) | funzionale · M10 |
| `Spec.TimeBank.ClashCostsFullWindow` | in un Clash la wall-clock resta 3,0 s a prescindere dal bank (§1.1) | **harness** |
| `Spec.TimeBank.PacingScriptedMatch` | match scriptato: prompt totali, tempo di decisione, esaurimenti, timeout | funzionale |
| `Spec.TimeBank.ControlLoadScalesInitialBank` | due Hero controllati ⇒ il bank iniziale registrato segue `LoadFactor`, non la baseline (§3.4) | **harness** |
| `Spec.TimeBank.ControlLoadNeverExtendsWindow` | al variare del conteggio la finestra resta 3,0 s: il costo del timeout è `MaxWindow − Grace` con la **grace scalata**, mai un `MaxWindow` diverso (§1.2 · §3.4) | **harness** |
| `Spec.TimeBank.TimeoutIgnoresPreferredResponse` | `PreferredResponse = FIRE:<n>`, nessun input ⇒ risposta `HOLD`, **nessuna charge spesa**, reaction ancora armata. Tre asserzioni separate: una sola passerebbe anche col codice sbagliato (§4.4, invariante 1) | **harness** |
| `Spec.TimeBank.PreferredResponseFallsBackWhenStale` | preferenza non più fra le `AllowedResponses` ⇒ si preseleziona la scelta sicura, **confronto esatto**: `FIRE:7` preferito con solo `FIRE:9` legale non spara a 9 (§4.4, invariante 4) | estende |
| `Spec.TimeBank.PreselectionSpendsNoCharge` | preselezionare non muove risorse: nessun commit, nessuna charge, nessuna voce di decisione nel log (§4.4, invariante 2) | estende |
| `Spec.TimeBank.PreferredResponseKeepsAllowedResponses` | l'elenco legale è identico con e senza preferenza dichiarata — è la riga che distingue la preferenza dalla condizione di [D-109](../decisions/RT_PDR_00_Decision_Log.md) (§4.4, invariante 3) | estende |
| `Spec.TimeBank.QuickConfirmReachableWithinGrace` | dall'apertura del prompt al commit della risposta **preselezionata** passa un solo input e meno della grace, su mouse, tastiera e controller (§4.4 · §11) | funzionale · UI |

**Un test non va scritto**: nessun test che verifichi solo che una costante valga 30, 1 o 3. I valori sono
`PROPOSED` e cambieranno; ciò che va pinnato sono le **relazioni** (grace non drena, timeout costa meno del
massimo, il floor regge), non i numeri.

---

## 14. Feature Registry e roadmap

**FeatureId** — verificati a HEAD. `RT-FEAT-REACTION-MULTI-TRIGGER`, `-SIMULTANEOUS` e `-PRIVACY` **non
esistono** e non vanno creati: i temi sono già coperti.

| Feature | Rapporto col bank |
|---|---|
| `RT-FEAT-CORE-DECISION-BOUNDARY` | il bank consuma al commit, dopo il boundary |
| `RT-FEAT-REACTION-FAST` | la finestra resta bounded a 3,0 s |
| `RT-FEAT-REACTION-OVERWATCH` | primo consumatore |
| `RT-FEAT-REACTION-CLASH` | §1.1: il bank drena, la wall-clock no |
| `RT-FEAT-REACTION-PROFILE` · `-PREPARED` | consumatori futuri, nessuna regola dedicata |
| `RT-FEAT-MATCH-PACING` | owner delle metriche; `ReactionDecisionSeconds` esiste già e non va sostituito |

**FeatureId nuovo**: `RT-FEAT-CORE-DECISION-TIME-BANK` — «core» e non «reaction» perché il bank è proprietà del
**giocatore** e serve ogni Decision Window, incluse quelle che non sono reazioni.
Status iniziale: `SPECIFIED`, release `v0.1`, roadmap `E14.8`. Lo status **si deriva dai gate**, non si scrive
a mano: vedi [`feature-registry.yaml`](../roadmap/feature-registry.yaml) e il suo validator.

**Roadmap**: **CP 14.8** dentro E14 (`#152`), dopo CP 14.7. Non serve un'epic nuova: E14 è già l'owner delle
Decision Window, e il pacing ha il suo owner in `RT-FEAT-MATCH-PACING`.

---

## 15. Rischi

| ID | Rischio | Mitigazione | Residuo |
|---|---|---|---|
| `TB-R1` | Si costruisce un sistema per un problema mai misurato | ordine dei CP: 14.5/14.6 precedono 14.8 (§2.1); i rientri di ADR-0004 restano disponibili | ⚠️ **accettato**: il rischio di D20 è stato assunto in senso opposto, con la stessa consapevolezza |
| `TB-R2` | Il bank punisce la latenza | §8, `LatencyAllowance` con cap | reale a M10: va misurato, non assunto |
| `TB-R3` | Il bank riapre il canale temporale | §7 owner-only + canary | ⚠️ il TurnLog conserva i tempi: va verificato che nessuna vista in-match lo esponga |
| `TB-R4` | Il costo pieno sul timeout tassa l'interfaccia invece della decisione | §4.2: fallback preselezionato e raggiungibile entro la grace, **misurato** prima della taratura | se la misura di §4.2 fallisce, il costo pieno diventa ingiusto: si corregge la UI, non il costo |
| `TB-R4b` | Spirale lag → timeout → bank a zero → altri timeout | §4.3 non-drenaggio su disconnessione | ⚠️ la soglia «lento vs disconnesso» è `TB-7`, aperta fino a M10 |
| `TB-R5` | Bank troppo scarso ⇒ reazioni prive di senso a fine match | criterio di promozione §3.2 (residuo mediano 20–50 %) | variante refill resta disponibile |
| `TB-R6` | Bank troppo generoso ⇒ obiettivo anti-stall mancato | scenario di pacing + p90 | — |
| `TB-R7` | Il bank non aiuta dove serve di più (Clash) | §1.1, dichiarato | strutturale: è il prezzo della privacy temporale, già accettato da D-048 |

---

## 16. Decisioni da registrare

Da assegnare al **prossimo ID reale** al momento del merge — gli ID si prendono al merge, non in sessione.

Registrate nel [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-09.

| ID | Decisione | Stato |
|---|---|---|
| [`D-050`](../decisions/RT_PDR_00_Decision_Log.md) | Esiste un Decision Time Bank per giocatore, condiviso fra tutte le Decision Window live. Entra in **v0.1** come CP 14.8, senza gate | **Consolidata** |
| [`D-051`](../decisions/RT_PDR_00_Decision_Log.md) | La singola finestra resta bounded a `FastReactionDuration` e le `AllowedResponses` non cambiano mai | **Consolidata** |
| [`D-052`](../decisions/RT_PDR_00_Decision_Log.md) | Grace prima del consumo; a bank esaurito resta `ExhaustedGrace`, mai zero | **`PROPOSED FOR PLAYTEST`** nei valori |
| [`D-053`](../decisions/RT_PDR_00_Decision_Log.md) | Il timeout costa `MaxWindow − Grace`, il massimo. **Precisazione** di ADR-0004 §3: il divieto di spesa su mancato input riguarda le risorse di **abilità**, non il budget temporale | **Consolidata** · matrice riga 54 |
| [`D-054`](../decisions/RT_PDR_00_Decision_Log.md) | Il bank residuo è **owner-only**; il bank pubblico è registrato e scartato (D-021 · D-048) | **Consolidata** · matrice riga 55 |
| [`D-055`](../decisions/RT_PDR_00_Decision_Log.md) | Input canonico registrato, non ricalcolato; fuori da ogni hash perché wall-clock e non regola | **Consolidata** *(per precedente)* |
| [`D-056`](../decisions/RT_PDR_00_Decision_Log.md) | `InitialBank` **derivato** da `RoundLimit × (MaxWindow − Grace)` | **Consolidata** nella forma |
| [`D-057`](../decisions/RT_PDR_00_Decision_Log.md) | Il bot possiede un bank: nessun ramo `IsBot` nella Decision Window | **Consolidata** |

Registrate il **2026-08-17**, con gli ID presi da `rt_shared_id.py reserve D`:

| ID | Decisione | Stato |
|---|---|---|
| [`D-156`](../decisions/RT_PDR_00_Decision_Log.md) | Il bank scala col **carico di controllo** attraverso `LoadFactor` **dentro** la derivazione di D-056; `Grace` ed `ExhaustedGrace` scalano per policy data-driven; `FastReactionDuration` resta invariata (§1.2 · §3.4) | **Consolidata** nella forma · numeri `PROPOSED` |
| [`D-157`](../decisions/RT_PDR_00_Decision_Log.md) | `PreferredResponse` è una dichiarazione di planning distinta dal timeout: non cambia le `AllowedResponses`, non consuma risorse finché non è committata, e allo scadere vale sempre `DecisionOnTimeout` (§4.4) | **Consolidata** |

⚠️ [`D-155`](../decisions/RT_PDR_00_Decision_Log.md) — il conteggio degli Hero controllati dichiarato dal
formato — **non è di questo documento**: il suo owner è
[`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) §16.4. Il bank lo **legge**,
come legge `RoundLimit`, e non lo definisce.

I restanti valori numerici restano baseline di playtest con i criteri di promozione di §3.2.

> Gli ID sono partiti da **D-050** e non da D-044: al momento dell'assegnazione `docs/decisioni-movimento`
> rivendicava già `D-044`–`D-045` e `wip/cp132-conoscenza-parziale` il `D-044`. Un ID va verificato contro i
> **branch aperti**, non solo contro `main`.

---

## 17. Domande aperte

### Chiuse il 2026-08-09

| ID | Domanda | Esito |
|---|---|---|
| `TB-1` | Quanto costa il timeout? | **`MaxWindow − Grace`**, il massimo (§4) |
| `TB-2` | v0.1 o M10? | **v0.1**, CP 14.8, senza gate (§2) |
| `TB-3` | `InitialBank` derivato o fisso? | **derivato** da `RoundLimit` (§3.2) |
| `TB-4` | `DecisionTimingPolicy` dentro o fuori dagli hash? | **fuori**: è wall-clock, non regola. Chiusa **per precedente** (`RTMatchFormatData.h` §14: i tempi di parete non stanno fra i parametri di regola), non per scelta. `ResolverConfigHash` non esiste nel codice |
| `TB-6` | `ExhaustedGrace`? | **0,75 s**, valore ancora `PROPOSED` (§5) |

### Aperte

| ID | Domanda | Quando si può chiudere |
|---|---|---|
| `TB-5` | Quale quota di RTT si sottrae dal consumo, con quale cap? | **M10**: dipende da una policy di rete che non esiste |
| `TB-7` | Quale soglia distingue «lento» da «disconnesso», e il fallback è immediato o alla deadline? | **M10**, stesso motivo. Da cui dipende §4.3 |
| `TB-8` | La taratura di `InitialBank` e `Grace` regge la misura di CP 14.5/14.6? | alla chiusura di **CP 14.6**, con i criteri di §3.2 |
| `TB-9` | Quale `LoadFactor(2)` e quale `ExtraControlledHeroGrace`? `1,75` / `+0,50 s` è la baseline, `+0,75 s` la variante registrata (§3.4) | **dopo `TB-8`**, e non prima: sono moltiplicatori di un valore che non è ancora tarato, e un playtest solo non separa due incognite |
| `TB-10` | Più Hero dello stesso Player possono ricevere **un solo** batch di decisione invece di finestre in serie? | quando esiste un formato che assegna due Hero a una persona. Oggi non c'è, e la misura dice che il cambiamento non è gratuito: `ARTTurnManager::ResolveReactionBoundary` applica ogni decisione **prima** di chiedere la successiva, ed è ciò che rende corretto il caso «due watcher, `Charges = 1`». Da valutare contro ADR-0004 §4 e `#314`, non dentro CP 14.8 |

---

## 18. Definition of Done

Il bank non è Done perché il countdown appare in UMG.

```
[ ] decisioni §16 (a…h) registrate nel Decision Log con ID reali, presi al merge
[ ] precisazione §4.1 registrata in DOC_CONFLICT_MATRIX: la lettura di ADR-0004 §3 non resta implicita
[ ] CP 14.5/14.6 chiusi prima della taratura (§2.1)
[ ] runtime server-authoritative, bank mai negativo
[ ] integrazione con Decision Window e timeout policy per definizione
[ ] fallback preselezionato e raggiungibile entro la grace (§4.2), MISURATO su mouse e controller
[ ] il bot ha un bank e nessun ramo IsBot attraversa la Decision Window (§9.1)
[ ] replay legge il residuo dal log (§6)
[ ] il SOGGETTO del bank esiste: un'identita' di giocatore distinta dall'unita' (D-155), o il bank
    finisce attaccato all'unita' e D-050 e' violata dal primo commit
[ ] il carico di controllo entra nella derivazione, non dopo (D-156 §3.4); MaxWindow invariata
[ ] PreferredResponse: i quattro invarianti di §4.4, ciascuno con il proprio test
[ ] Quick Confirm: azione semantica, nessun tasto fisico nella logica di Reaction ne' nel widget
[ ] il nome della categoria di log e' risolto PRIMA del runtime (§10.1): Decision o ReactionDecision,
    non entrambe
[ ] TurnLog allineato a spec-turnlog.md, nomi confermati con l'owner
[ ] scenari §13 automatizzati, nessun sleep nei golden
[ ] telemetria collegata a RT-FEAT-MATCH-PACING senza sostituire ReactionDecisionSeconds
[ ] canary di privacy (M10)
[ ] test di latenza (M10)
[ ] Wiki player-facing aggiornata, senza numeri PROPOSED presentati come regola
[ ] feature registry e roadmap allineati con issue reali
```
