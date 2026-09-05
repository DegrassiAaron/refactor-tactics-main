# Spec — Durata della partita, budget del round e scala delle mappe

> **Stato**: decisioni consolidate + baseline da playtestare · **Data**: 2026-08-07 · **Decisore**: utente (dev singolo)
> **Ambito**: quanto dura una partita, quanti round la compongono, quanto dura ogni finestra temporale del round,
> quanto è grande una mappa e **come si misura** che sia della dimensione giusta.
>
> **Non è**: il tempo di *un* turno misurato sul giocatore reale (→ [`spec-pacing-turno.md`](spec-pacing-turno.md)),
> né il modello delle reazioni interattive (→ [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)), né il pacing del
> playback (→ [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md)). Questa spec dice **quali numeri
> puntare**; quelle dicono **come misurarli** e **come si riproducono a schermo**.

> ⚠️ **Questo documento NON decide il formato di partita** ([D-011](../decisions/RT_PDR_00_Decision_Log.md),
> 2026-08-08). Vale:
>
> ```text
> 2v2  = vertical slice corrente della v0.1
> 3v3  = baseline di lavoro / ipotesi   <- tutti i numeri "Standard" qui sotto
> 4v4  = validazione di stress (epic E17), NON formato principale
> formato competitivo principale = APERTO, si consolida con la prima misura su una partita >=3v3
> ```
>
> I numeri 3v3 restano utili come **termine di paragone** e come taratura di partenza: non sono una decisione
> normativa, e nessun altro documento deve citarli come «il formato». `D-001` è stata declassata ad
> *Assunzione da bloccare* proprio perché nessun 3v3 è mai stato giocato.
>
> 🕐 **Glossario corretto** ([D-019](../decisions/RT_PDR_00_Decision_Log.md)): «Fast Action» qui indicava
> l'azione dichiarata in Planning che risolve più tardi. È una **Delayed / Predictive Action**. `Fast Action`
> è invece una scelta **live** limitata, continuazione di una propria azione — vedi §2.
>
> **Supera**: `RT_PDR_00_Decision_Log.md` **D-002** («massimo 12 turni; planning 30 s; resolution 6-12 s»),
> il «limite di **12 turni**» come regola universale di [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §3, e ogni
> residuo di «finestra di interrupt / Reaction Charge da **5 s**» presentato come baseline corrente.

---

## 0. Glossario — terminologia vincolante

Il repository usa storicamente **«turno»** per ciò che qui si chiama **Round**. **Non si fa un rename globale**:
il codice (`ERTMatchPhase`, `TurnNumber`, `ARTTurnManager`, `FRTTurnLogEntry`) resta com'è. Si fissa il
glossario, così i documenti nuovi non moltiplicano i significati.

| Termine | Significato | Nel codice / nei doc storici |
|---|---|---|
| **Match** (partita) | Dall'allestimento alla condizione di fine | «partita» |
| **Round** | Un ciclo completo `Planning → Commit → Resolution → Cleanup` | **«turno»**, `TurnNumber` |
| **Planning** | Finestra di pianificazione simultanea, con timer massimo | `ERTMatchPhase::Planning`, `PlanningSeconds` |
| **Ready** | Dichiarazione «ho finito», anticipabile rispetto al timer | lock-in (Spazio) |
| **Commit** | Chiusura irreversibile degli intenti, snapshot dello stato | `LockInAndResolve` |
| **Resolution** | Calcolo autorevole + playback delle macro-fasi `Prep → Dash → Blast → Move` | `Resolving` |
| **Delayed / Predictive Action** | Azione dichiarata **interamente in Planning** che risolve a un boundary successivo, **senza input umano** | [`brief-delayed-actions.md`](brief-delayed-actions.md) · [D-016](../decisions/RT_PDR_00_Decision_Log.md) |
| **Fast Action** | Scelta **live** limitata, continuazione esplicita di una **propria** azione (es. `LEFT`/`RIGHT` dopo un'ability) | [D-019](../decisions/RT_PDR_00_Decision_Log.md) — ⚠️ questo documento usava «Fast Action» per la riga sopra: **uso errato**, corretto il 2026-08-08 |
| **Fast Reaction** | Scelta **live** provocata da un evento **esterno** (es. `FIRE`/`HOLD` dell'Overwatch) | [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) · [D-019](../decisions/RT_PDR_00_Decision_Log.md) |
| **Fast Reaction** | Scelta live richiesta **dentro** la resolution a un decision boundary | ADR-0004, `FRTReactionOpportunity` |
| **Overtime** | Prolungamento oltre il `RoundLimit` a punteggio pari | ⏳ non esiste |
| **Match Format** | Il pacchetto di parametri di un formato (3v3 Standard, 2v2 Skirmish…) | ✅ **esiste** — `URTMatchFormatData` (`Turn/RTMatchFormatData.h`): `FormatId`, `RoundLimit`, `ExpectedRounds`, `ScoreToWin`, `UnitsPerTeam`, `UnitsPerPlayer`, `MapClass`. ⏱️ *Rimisurata sull'header il 2026-08-17*: la riga citava `FormatVersion`, **rimosso** da [D-141](../decisions/RT_PDR_00_Decision_Log.md), e dichiarava mancanti `UnitsPerTeam` e la classe di mappa, **entrambe atterrate** con CP 19.1/19.2 il 2026-08-09. Era falsa in tre punti su tre |
| **Ruleset** | L'insieme di regole/policy che governa un Match Format | ⏳ non esiste |

**«Turno» non va usato** in documenti nuovi per indicare «il giro di una singola unità»: quel concetto non
esiste in RefactorTactics — tutte le unità agiscono nello stesso round.

---

## 1. Il principio

> **RefactorTactics deve essere compatto nel tempo, non necessariamente piccolo nello spazio.**

*Atlas Reactor* resta il riferimento per **planning simultaneo**, **leggibilità delle fasi**, **commitment
delle azioni** e **resolution simultanea**. Non è un riferimento per la **dimensione fisica della mappa**, la
**distanza fra le squadre**, il **numero di round** né la **durata esatta della partita**.

La dimensione della mappa si decide dalle necessità del gameplay di RefactorTactics, non per analogia.

---

## 2. Perché serve più spazio tattico

RefactorTactics ha (o avrà) sistemi che senza spazio non producono decisioni:

Fog of War · visibilità limitata · stealth · rumore e percezione acustica · Overwatch direzionale ·
Fast Reaction · percorsi alternativi · flank · coperture · quota · ponti · porte · tunnel · acqua · fuoco ·
elettricità · hazard · obiettivi dinamici · controllo del territorio.

Una mappa troppo piccola li **degrada tutti insieme**, e ciascuno per la stessa ragione — il raggio dei sistemi
diventa comparabile al diametro della mappa:

| Sistema | Come degrada su mappa piccola |
|---|---|
| Fog of War / vista | la visibilità diventa di fatto globale: non c'è nulla da scoprire |
| Rumore | ogni rumore è udito da tutta la squadra avversaria: smette di essere informazione parziale |
| Stealth | non esiste una rotta abbastanza lunga da restare non visti |
| Flank | il fianco è a due celle: aggirare non costa nulla, quindi non è una scelta |
| Overwatch | una sola zona controllata copre tutte le rotte principali → dominante senza counterplay |
| Rotte alternative | esistono sulla carta ma convergono subito: nessuna vera differenza di rischio |
| Positional gameplay | compresso: la posizione conta meno del semplice ordine delle azioni |

Le mappe di RefactorTactics possono quindi essere **sensibilmente più spaziose** di quelle di Atlas Reactor.

---

## 3. Come si dimensiona una mappa — *temporal map size*

**Metrica primaria** — non i metri, non il numero assoluto di celle, non il confronto con Atlas Reactor:

> **Quanti Move servono per raggiungere una zona tatticamente rilevante?**

Una mappa è dimensionata correttamente quando:

1. già nel **round 1** esistono decisioni tattiche significative;
2. entro **1–2 round** si può contestare una zona importante o entrare in un primo contatto significativo;
3. **attraversarla tutta** costa sensibilmente di più;
4. esistono almeno **2–3 rotte strategicamente diverse**;
5. il giocatore deve scegliere fra **rapidità, sicurezza, visibilità, rumore, copertura e opportunità di flank**.

Da evitare:

```text
Spawn → Move → Move → Move → finalmente succede qualcosa
```

Da preferire:

```text
Spawn → scelta immediata della rotta → primo contatto / contestazione entro 1–2 round
      → attraversamento completo molto più costoso
```

**Perché la metrica è temporale e non spaziale**: il costo di traversata è già un intero per cella e dipende
dal terreno (1 normale, 2 difficile/rampa — [`balance/RT_TerrainCatalog_v0.1.md`](../balance/RT_TerrainCatalog_v0.1.md)).
Contare le celle misura la cosa sbagliata: 40 celle di `Rough` e 80 celle normali sono la stessa mappa dal punto
di vista del giocatore. Il numero di **Move** le rende confrontabili.

---

## 4. Classi di mappa — data-driven, non hard-coded

Le classi sono un **attributo del dato mappa**, non un `enum` con regole cablate: un livello dichiara la classe
a cui appartiene e i target si verificano su di esso, non nel codice delle regole.

### Skirmish

**Uso**: tutorial, test, vertical slice iniziale, 2v2, match rapidi.

| Target indicativo | Valore |
|---|---|
| Attraversamento completo | ~**3–4 Move** normali |
| Primo contatto significativo | ~**1 round** |

### Standard — baseline di lavoro 3v3

**Uso**: 3v3, **baseline di lavoro**, non formato deciso ([D-011](../decisions/RT_PDR_00_Decision_Log.md)).

| Target indicativo | Valore |
|---|---|
| Attraversamento completo | ~**5–7 Move** normali |
| Primo contatto / contestazione | **1–2 round** |
| Macro-rotte | almeno **2–3** |
| Topologia | choke point **e** percorsi alternativi |

**La quantità esatta di celle non è bloccata.** Una Standard può tranquillamente superare le 100 celle e
arrivare a **~150–200 celle percorribili** se il layout mantiene il tempo di contatto corretto. Quei numeri sono
un **ordine di grandezza da playtestare**, non un requisito.

### Operations — **futuro, fuori scope**

Formato possibile in seguito: mappe sensibilmente più grandi, Fog of War più determinante, esplorazione,
zone/obiettivi multipli, logistica e repositioning, match potenzialmente da **45–60+ minuti**.

L'architettura deve **consentirlo** (parametri di formato nei dati, nessun limite cablato). **Non si implementa
ora**: non è nella v0.1 né in M6–M11.

---

## 5. Durata desiderata della partita

Baseline di lavoro **3v3 Standard** — ipotesi, non formato deciso:

| Caso | Durata |
|---|---|
| Match veloce | ~20–25 min |
| **Match tipico (target)** | **~25–30 min** |
| Match combattuto | ~30–40 min |
| Eccezione / **hard ceiling di design** | ~**45 min** |

I 45 minuti sono il **limite superiore da evitare nella maggioranza delle partite**, non un obiettivo: una
normale partita competitiva non va progettata per durare 45 minuti.

Sono **target di playtest**, non invarianti: nessuna regola del resolver dipende da questi numeri.

---

## 6. Numero di round

La precedente «max 12 turni» **non è più una decisione definitiva** e **non è una regola universale**. Con
resolution rapide e planning efficiente, 12 round producono partite troppo corte per la baseline 3v3.

| Formato | Round attesi | Hard cap indicativo |
|---|---|---|
| **3v3 Standard** | ~**16–20** | ~**20–22** |
| **2v2 vertical slice / Skirmish** | ~**10–14** | ~**14–16** |

Il numero di round è un **parametro del Match Format / Ruleset**, non una costante:

```text
MatchFormat.Standard3v3     ExpectedRounds = 16–20   RoundLimit = 20–22
MatchFormat.Skirmish2v2     ExpectedRounds = 10–14   RoundLimit = 14–16
```

I valori reali si fissano con telemetria e playtest (§16). **Nessuna affermazione secondo cui la struttura
finale del gioco è obbligatoriamente limitata a 8 o 12 round è vigente**: gli 8 turni della showcase sono un
**dato di scenario** ([`showcase-v0.1.md`](../product/showcase-v0.1.md) §3), i 12 del catalogo v0.1 diventano il valore
iniziale di `RoundLimit` per il **solo** formato 2v2 della v0.1.

> **Dato misurato che sostiene la banda 2v2**: `RefactorTactics.HexMatch.PlaysToCompletion` (2026-08-06) chiude
> una partita bot-vs-bot al **round 10** — dentro la banda 10–14, e per un motivo noto (la scadenza dello scudo
> nel Cleanup; prima erano 25, issue `#96`). È l'unico numero reale che abbiamo oggi, e viene da bot, non da
> giocatori.

---

## 7. Planning e Ready

### 7.1 Timer massimo + Ready anticipato

Il planning usa **timer massimo** *e* **Ready anticipato**. **Non si assume che ogni round consumi l'intero
timer**: il timer è il tetto, il Ready è il caso normale.

| Formato | `PlanningMax` (baseline da testare) |
|---|---|
| **3v3 Standard** | **40–45 s** |
| **2v2 v0.1** | **30 s** — valore corrente in codice (`RTTurnManager.h:239`), **da tarare sul misurato**, non da alzare per analogia col 3v3 |

**Perché 40–45 s e non 30 s per il 3v3**: il giocatore deve valutare più personaggi, path, abilità, ghost delle
azioni, AoE, azioni ritardate, Overwatch, Fast Reaction preparate, terreno, intenti alleati, collisioni,
friendly fire, Fog of War e rischio informativo. 30 s possono risultare insufficienti nelle situazioni
complesse — ed è esattamente la coda che [`spec-pacing-turno.md`](spec-pacing-turno.md) §7 chiama *taglio*.

Il sistema deve però **incentivare round più rapidi** attraverso il Ready, non attraverso un timer corto.

### 7.2 Ready countdown

```text
Player A READY @ 22 s
Player B READY @ 28 s
  → tutti Ready → countdown 3 s → Commit → Resolution
```

| Parametro | Baseline |
|---|---|
| `ReadyCountdown` | **3 s** |
| Unready durante il countdown | **annulla** il countdown e torna al planning |

Il countdown **non sostituisce** il timer massimo del planning: è la scorciatoia quando tutti hanno finito prima.

> ~~⚠️ **Stato di implementazione (verificato 2026-08-07)**: oggi il lock-in è **immediato** — Spazio chiude il~~
> ~~planning senza countdown e senza possibilità di annullare (`RTPlayerController.cpp`, `LockInAndResolve`).~~
> ~~Il countdown annullabile **non esiste**. `RT_PDR_10 §2` riga 8 lo dichiarava ✅ ed è stato corretto a 🟡.~~
>
> ✅ **Il countdown ESISTE dal 2026-09-04** ([#2193](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2193)).
> `ARTTurnManager::RequestLockIn()` arma `ReadyCountdownSeconds` (**3 s**) e solo al suo scadere chiama
> `LockInAndResolve()`; `CancelLockIn()` è l'Unready. Il gesto è **RMB / Backspace** — `UndoAction`, che
> durante il countdown non ha waypoint da annullare — e non un toggle su Spazio: chi preme due volte per
> abitudine annullerebbe senza volerlo, cioè l'opposto del difetto che il countdown previene.
>
> 🔴 **Il tetto vince, ed è una proprietà della struttura.** I due orologi chiamano entrambi
> `LockInAndResolve`, che li spegne tutti e due appena entra: il primo che scatta vince, e il countdown non
> allunga mai `PlanningSeconds`. *«Non sostituisce il timer massimo»* non è un `if` da ricordare.
> ⚠️ E l'**Unready non riarma il tetto**: sarebbe l'altro modo di sostituirlo, in un verso che permetterebbe
> di pianificare senza limite.
>
> Il perché di questa forma — e il motivo per cui il countdown **non** è uno stato di simulazione — sta nel
> referto [`../roadmap/plans/ready-countdown-spec-panel-2026-09-04.md`](../roadmap/plans/ready-countdown-spec-panel-2026-09-04.md):
> la risposta era già in §11 di questo documento, dove `ReadyCountdownSeconds` è classificato fra i **Tempi UX**.
>
> ⚠️ **In 2v2 offline la differenza resta quella dichiarata** (un solo umano): il valore che il countdown porta
> *oggi* non è l'attesa reciproca, è **annullare una chiusura involontaria**. La forma «tutti Ready» diventa
> osservabile con il 3v3 e con **M10**, e `RequestLockIn` è un punto solo perché aggiungerci il quorum non
> richieda di spostare il countdown.

---

## 8. Fast Reaction

| Parametro | Valore |
|---|---|
| `FastReactionDuration` | **3.0 s** — **baseline di sistema** |
| `DefaultTimeoutBehavior` | **HOLD** (mai `FIRE`: un mancato input non consuma una risorsa irreversibile) |
| `MaxPromptsPerReaction` | 3, data-driven |

Formalizzata in [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §8, che resta la fonte del modello.

La Fast Reaction **non è una seconda fase di planning**. Deve essere breve, immediata, con poche opzioni,
contestuale, leggibile, **senza menu profondi**. L'esempio canonico è l'Overwatch:

```text
FIRE     HOLD          ← timeout standard: HOLD
```

**I 5 secondi non sono più una baseline.** Ogni «interrupt window» o «Reaction Charge = 5 s» nei documenti
sorgente (`docs/src/`) è **superato**: quei file sono materiale north-star, non canone. Reaction specifiche
future possono dichiarare durate diverse **se il Ruleset lo consente**; 3 s resta il default di sistema.

---

## 9. Resolution

La resolution resta spettacolare ma **relativamente rapida**.

| Formato | Playback tipico per round |
|---|---|
| Vertical slice / 2v2 | ~**8–15 s** |
| 3v3 Standard | ~**12–20 s** |

Le Fast Reaction **estendono la durata reale** e non rientrano in questa banda (sono Decision Time, §11).

La resolution **non va rallentata artificialmente** per raggiungere la durata desiderata della partita.
Si preferisce **più decision cycle significativi** a resolution molto lunghe.

La simulazione deterministica resta **separata** dal playback: la durata visuale non modifica ordine logico,
seed, collisioni, stato né esiti (invariante #1 e #4; `Replay.Verifier.ResimulationIsDeterministic`).

> **Conseguenza da tarare, non da implementare ora**: `MaxPlaybackSeconds = 12` (`RTTurnManager.h:134`) è la
> soglia oltre la quale scatta lo speed-up automatico. Con la banda 8–15 s (2v2) comprimerebbe i round più
> pieni. Il valore va rivisto **quando la banda viene misurata**, non adesso —
> [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md) §6, la cui raccomandazione «round tipico 6–12 s» è
> **sostituita** da questa tabella.

---

## 10. Budget temporale del round

Un round è composto concettualmente da:

```text
Planning
  → Ready Countdown (eventuale)
    → Commit
      → Resolution
        → Fast Decision Windows (eventuali)
          → Cleanup / Score / transizione di round
```

Baseline **3v3 Standard**:

| Segmento | Baseline |
|---|---|
| Planning | max **40–45 s**, spesso chiuso prima dal Ready |
| Ready countdown | **3 s** se tutti Ready |
| Resolution | ~**12–20 s** tipica |
| Fast Reaction | **3 s** per opportunity **realmente generata** |
| Cleanup / score | ~**3–5 s** di presentazione, possibilmente integrata nel playback |

> ⚠️ **Non sommare i massimi** per stimare la durata media della partita. Il massimo del planning si verifica
> raramente (è il Ready il caso normale), le finestre di reazione esistono solo quando un trigger scatta, e il
> Cleanup si sovrappone al playback. Una stima ottenuta sommando i tetti sovrastima sistematicamente.
> **La metrica reale si raccoglie con la telemetria** (§16).

---

## 11. Quattro tempi da non confondere

| Tempo | Cos'è | Ordine di grandezza oggi |
|---|---|---|
| **Simulation Time** | Il resolver calcola il round | **0,41 ms/round** misurato (`Perf.TurnResolverMedian`, 2026-08-06) |
| **Presentation Time** | Il playback riproduce ciò che è già stato deciso | secondi (§9) |
| **Decision Time** | Attesa reale di un input umano: planning e Fast Reaction | secondi (§7, §8) |
| **Wall-clock Match Time** | Quanto dura la partita per chi la gioca | minuti (§5) |

Il resolver chiude un round logicamente in **meno di un millisecondo**; il playback dura secondi; la Fast
Reaction è l'unica cosa che crea **vera Decision Time dentro la resolution**. Confondere queste categorie
produce due errori opposti: credere che il gioco sia lento perché il playback dura, o credere che si possa
accorciare la partita ottimizzando il resolver.

---

## 12. Fine partita

La partita **non deve dipendere esclusivamente** dal raggiungimento del `RoundLimit`. Il sistema deve poter
supportare quattro vie, governate dal **Ruleset**:

```text
Victory Condition raggiunta
  → vittoria immediata / a fine fase
ELSE Score Threshold raggiunta
  → vittoria secondo Ruleset
ELSE RoundLimit raggiunto
  → confronto dei punteggi
     IF pari → policy di overtime
```

| Via | Stato v0.1 |
|---|---|
| Eliminazione della squadra | ✅ implementata |
| Obiettivo raggiunto | 🟡 **il giudice c'è, la fonte no** (CP 10.3): `ScoreToWin` chiude la partita, ma il progresso lo produrrà **CP 10.2** (`#75`) |
| `RoundLimit` → confronto punteggio | ✅ **CP 10.3**; parità = **pareggio dichiarato** |
| Overtime | **fuori scope v0.1** — non si costruisce un sistema di overtime sofisticato se non serve |

Il `RoundLimit` è un parametro del Match Format (§6), non una costante del `TurnManager`.

**Come è realizzata** *(CP 10.3, 2026-08-07)*: la regola è `URTTurnRules::EvaluateMatchEnd(FRTMatchState,
FRTMatchRules)` — pura, valutata nel **Cleanup** dal `TurnManager`, mai dentro un resolver. Ritorna
`FRTMatchResult`: **esito** (`ERTMatchOutcome`) e **via** (`ERTMatchEndReason`), separati perché «vince il
team 0» è la stessa frase per un'eliminazione e per un punto di vantaggio allo scadere. Il progresso entra da
`ARTTurnManager::AddTeamScore`, che è l'ingresso che CP 10.2 dovrà chiamare.

---

## 13. Obiettivi dinamici e anti-stallo

Mappe più grandi **non devono** produrre camping o round vuoti. Lo strumento principale è il sistema Objective
(epic **E10**).

> **Principio**: gli obiettivi devono **comprimere progressivamente il conflitto** senza richiedere mappe
> artificialmente piccole.

Progressione **concettuale** del match, da non trasformare in regola:

| Fase | Cosa domina |
|---|---|
| Early | controllo delle rotte, raccolta di informazioni |
| Mid | obiettivo centrale o laterale in gioco |
| Late | obiettivo di maggior valore, escalation, pressione crescente |

Un esempio puramente illustrativo — «Round 1–5: Objective A · Round 6–10: entra Objective B · Late game:
high-value objective» — serve a mostrare la forma, **non** a fissare i numeri. La schedulazione degli obiettivi,
quando arriverà, è **dato di scenario** sopra il sistema di E10, mai un `if (Round == N)` nel `TurnManager`
(stessa regola della showcase, [`showcase-v0.1.md`](../product/showcase-v0.1.md) §3).

---

## 14. Relazioni con gli altri sistemi

### 14.1 Fog of War

```text
Fog of War + mappa sufficientemente ampia = informazione come risorsa tattica
```

Su mappe Standard: **non** deve essere normale vedere sempre tutti gli avversari · perdere il contatto visivo
deve essere possibile · il rumore può dare informazione **senza** dare la posizione esatta · si possono
scegliere rotte per evitare il rilevamento · un flank reale è possibile · la **memoria dell'ultima posizione
nota** ha significato.

Questo vincola il level design futuro. Sistema: **E13** → [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md)
(tre livelli: `Rilevato` / `Incerto` / `UltimoContatto`).

### 14.2 Rumore

Il rumore diventa significativo solo su una mappa in cui **non** è automaticamente percepito da tutta la squadra
avversaria. La scala deve permettere: zone acusticamente separate · materiali diversi · tunnel che propagano ·
aree con rumore ambientale · Sprint come trade-off velocità/stealth · decoy · Acoustic Mask · deduzione della
**direzione** senza la posizione esatta.

Il sistema non si modifica qui: sorgente in `docs/archive/src/design/rumore-e-percezione-acustica.md`, scope in
[`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md).

### 14.3 Overwatch

L'Overwatch direzionale beneficia di choke point, percorsi alternativi, porte, ponti, corridoi, tunnel e
possibilità di flank. Su mappa troppo piccola diventa **troppo facilmente dominante**.

**Vincolo di level design**: nessuna posizione da cui una singola Overwatch controlli sistematicamente **tutte**
le rotte principali senza counterplay. Sistema: **E14** → [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md),
[`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md); la direzione della zona nasce dal **facing**
([ADR-0005](../decisions/adr-0005-orientamento.md) §4c).

### 14.4 Movimento

**Non si aumenta il Move range per compensare una mappa più grande.** La distanza deve creare una **scelta**:

| Modo | Trade-off |
|---|---|
| **Move** | sicuro, standard — 5 MP, costo intero per cella |
| **Sprint** | più distanza (8 MP) ma più rumore, rischio, interazioni col terreno |
| **Dash / Charge / Leap** | spostamento speciale nella fase Dash, distanza fissa, traiettoria lineare |
| **Sneak** *(futuro)* | meno rumore, meno velocità |
| **Fast traversal** | porte, ascensori, ponti, scorciatoie, abilità |

È questo che permette mappe più grandi senza creare un «walking simulator tattico».

---

## 15. Fasi del turno — invariate

**L'ordine macro delle fasi non cambia**:

```text
Planning → Prep → Dash → Blast → Move → Cleanup
```

Il **Move normale resta l'ultima fase di spostamento volontario**, dopo il Blast
([ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §1). Non si introducono sequenze tipo `Move → Attack`.

**Fast Action e Fast Reaction sono finestre contestuali della resolution** e **non** autorizzano una seconda
fase completa di planning (ADR-0004 §1: l'invariante #3 si *compone*, non si deroga).

---

## 16. Configurazione data-driven

Tutti i parametri temporali devono essere data-driven tramite **Match Format / Ruleset** dove ragionevole.
Elenco **concettuale** dei parametri, non una firma di struct:

```text
MatchFormatId · TeamSize
PlanningMaxSeconds · ReadyCountdownSeconds · FastReactionDefaultSeconds
ExpectedRoundCount · RoundLimit
VictoryPolicy · ScoreThreshold · OvertimePolicy
ExpectedMatchDurationMinutes · SoftMaxMatchDurationMinutes
MapClass (Skirmish | Standard | Operations)
```

> ⚠️ **Questi nomi non sono vincolanti** e **non si crea architettura nuova per documentarli**.
> Oggi `PlanningSeconds` e `MaxPlaybackSeconds` sono `UPROPERTY` su `ARTTurnManager` e `RoundLimit` **non
> esiste affatto**. Il passaggio a formato dichiarato è lavoro di **CP 10.3** in poi, non di questa spec.

### 16.1 Forma decisa *(2026-08-07, issue `#185`)*

| Decisione | Valore |
|---|---|
| **Collocazione** | **`URTMatchFormatData : UPrimaryDataAsset`** + libreria statica pura + validator, in **`Turn/`** — accanto a `RTTurnRules`, che è dove vivono le regole del turno |
| **Cosa entra subito** | `RoundLimit` **e** `ScoreToWin` (D2: entra ciò che un checkpoint **consuma**, e CP 10.3 consuma entrambi). Gli altri migrano quando avranno un lettore |
| **Formato assente** | la partita **si avvia comunque**, con un formato di ripiego, **anche in build packaged** (D1) |
| **Formato invalido** | il validator lo **rifiuta**: la partita **non** si allestisce. Il ripiego copre l'assenza, non il contenuto sbagliato |
| **Versionamento** | l'asset nasce con un proprio `FormatVersion`, come `URTHexMapAsset` |

È il pattern già in uso per il resto dei dati di gioco (`URTActionData`, `URTHeroData`, `URTCatalogLibrary`
col validator che fallisce su asset invalido, gate **E1** della DoD). **Nessun `Subsystem`, nessun
`ActorComponent`**: il progetto non ne usa (stessa motivazione di
[`spec-pacing-turno.md`](spec-pacing-turno.md) §4).

> **Perché `PlanningMaxSeconds` non è entrato** *(CP 10.3, 2026-08-07)*. La DoD di `#185` lo elencava fra i
> casi che il validator deve rifiutare, ma **D2** lascia il timer di pianificazione come `UPROPERTY` sul
> `TurnManager`: metterlo anche nell'asset avrebbe creato **due sorgenti per lo stesso valore**, che è il
> difetto respinto da ADR-0005 §4c e la ragione per cui l'opzione **C** era stata scartata. Il campo entrerà
> quando il `TurnManager` lo leggerà **al posto** della propria `UPROPERTY`, non accanto ad essa.
> Per la stessa ragione `VictoryPolicy`/`OvertimePolicy` restano fuori: in v0.1 nessuna policy li consuma.

**Realizzazione** *(CP 10.3)*: `URTMatchFormatData` (asset, con `FormatVersion`) + `FRTMatchRules` (le regole
**risolte**, ciò che il `TurnManager` legge) + `URTMatchFormatLibrary` (`ValidateFormat`, `ResolveRules`
fail-closed, `MakeFallbackRules`). Casi rifiutati dal validator: `RoundLimit ≤ 0` · `ExpectedRounds >
RoundLimit` · `FormatId` assente · `FormatVersion ≤ 0` · `ScoreToWin < 0`. Il ripiego ha l'identità riservata
`Format.Fallback`, così una traccia dice sempre se è stata prodotta senza formato dichiarato.

**Dove vive il ripiego**: la libreria pura **rifiuta sempre** (`nullptr`/`false` + motivo); la politica di
ripiego sta **solo** in `ARTGameMode`, dove già sta quella dell'arena generata. È la separazione che il
repository usa già — `MakeDemoArena` ritorna `nullptr` senza scusarsi, è il `GameMode` che sceglie di
ripiegare e lo logga in Warning.

> **Conseguenza vincolante di D1.** Siccome il ripiego esiste **ovunque**, l'onere si sposta tutto
> sull'**osservabilità**: il formato realmente in vigore deve finire nel log e nella telemetria. Un ripiego
> silenzioso non produce una partita rotta — produce una partita che gira benissimo mentre i numeri di §17
> vengono attribuiti a un **formato fantasma**. Da rivedere a **M10**: fra due client, un ripiego silenzioso
> non è un default comodo, è una divergenza di regole.

### 16.2 Tre classi di parametri, tre regimi di determinismo

«Il formato» non è una cosa sola, e trattarlo come blocco unico fa aggirare un divieto già scritto:

| Classe | Parametri | Regime |
|---|---|---|
| **Regole** | `RoundLimit` · `VictoryPolicy` · `ScoreThreshold` · `OvertimePolicy` | Input **deterministici**: la simulazione li legge, l'esito ne dipende |
| **Tempi UX** | `PlanningMaxSeconds` · `ReadyCountdownSeconds` · `FastReactionDefaultSeconds` | Tempo di **parete**: non devono **mai** raggiungere il TurnLog — [`spec-pacing-turno.md`](spec-pacing-turno.md) **D3**, «un tempo di parete lì dentro lo renderebbe non deterministico» |
| **Target di design** | `ExpectedMatchDurationMinutes` · `SoftMaxMatchDurationMinutes` | **Documentazione**: non li legge nessun codice |

Conseguenza diretta: nel TurnLog entra l'**identità** del formato (un `FormatId` stabile, come `ActionId`),
**mai i suoi campi**.

### 16.3 Snapshot e TurnLog

| Dove | Entra? | Perché |
|---|---|---|
| **Snapshot** (`FRTHexSnapshot`) | **No**, per ora | Nessuna funzione pura lo legge: la fine partita si valuta nel Cleanup dal `TurnManager`, non dentro `ResolveHexPaths`. Metterlo ora significherebbe che ogni resolver porta un parametro che nessun resolver consulta. Quando servirà, entra **per riferimento + hash** come la mappa — mai copiato |
| **Header del TurnLog** | **Sì** — `FormatId` | Stesso motivo di `ERTLogTopology`: *«senza questo marcatore le due tracce sarebbero indistinguibili e un confronto incrociato darebbe un falso "nessuna divergenza"»*. Due esecuzioni dello stesso scenario con `RoundLimit` diverso divergono al round in cui il limite morde, e la divergenza verrebbe attribuita al **codice** invece che alla **configurazione** |
| **Voci del TurnLog** | **No** | Sarebbe una costante ripetuta N volte; `ActionId` è già dichiarato «il primo campo a lunghezza variabile del formato» |
| **`HashTurnLog`** | **No** | L'hash mescola solo i campi delle voci, e la topologia infatti non ci entra. Includere il formato **invaliderebbe in blocco ogni hash golden**, cioè proprio ciò che la regola di CP 12.6 — rigenerazione solo con flag esplicito — esiste per impedire |

Versione del formato serializzato a **4**; i file **v3 restano leggibili** con `FormatId` neutro, come
`Square = 0` per la topologia e come l'`ActionId` vuoto di v3.

**Se l'hash non copre il formato, deve coprirlo la procedura di confronto**, o il falso «nessuna divergenza»
ricompare un piano più su: il test del corpus golden verifica che il formato coincida **prima** di confrontare
gli hash, e fallisce dicendo *formato diverso*, non *divergenza*.

### 16.4 Quanti Hero controlla un Player — [D-155](../decisions/RT_PDR_00_Decision_Log.md)

> **`1 Player = 1 Hero` non è un'invariante del gioco: è un valore del formato.** Il numero di unità che una
> persona comanda lo dichiara il Match Format, e nessun altro — non il `PlayerController`, non il
> `TurnManager`, non il resolver. *(decisione dell'autore, 2026-08-17)*

È lo **stesso buco** che CP 19.2 ha chiuso per `UnitsPerTeam`, un piano più sotto: allora «il 2v2 è
un'assunzione del `GameMode`», adesso «una persona comanda una unità» è un'assunzione di tutto ciò che sta
sopra il resolver. E i due numeri non sono lo stesso numero:

| Domanda | Campo | In v0.1 |
|---|---|---|
| Quante unità schiera una squadra? | `UnitsPerTeam` (esiste, `FRTMatchRules`) | 2 |
| Quante ne comanda **una persona**? | *non esiste* | 2 — **e coincide per caso** |

⚠️ **In v0.1 i due valori coincidono, ed è la trappola.** 2v2 offline ha un solo umano che comanda la propria
squadra intera: un'implementazione che legga `UnitsPerTeam` al posto del campo giusto passa **ogni test
esistente** e sbaglia al primo formato in cui una squadra è divisa fra due persone. Il campo va **nominato**,
non dedotto da quello che gli somiglia.

#### Cosa questo vincola, e cosa no

- il **resolver resta invariante**: risolve unità e intenti per id stabili e ordinamento deterministico, e non
  deve mai sapere quante persone stanno dietro. L'associazione Player → unità serve ad **autorizzazione,
  input, planning, `Ready`, privacy, UI e ownership della decisione di reazione** — non è una regola di
  risoluzione, e non entra nell'hash (§16.2, classe *Regole*, per ciò che dipende dall'esito);
- il **Planning resta una sola finestra per persona**: chi comanda due unità passa fra le proprie, vede i
  propri intenti insieme, e diventa `Ready` solo quando tutti gli intenti richiesti sono committabili. Non si
  aprono due Planning Phase — §7 non si sdoppia;
- la **Decision Window guadagna un soggetto**, ed è la ragione per cui questa riga è v0.1 e non v0.2:
  [`spec-decision-time-bank.md`](spec-decision-time-bank.md) dichiara un bank **per giocatore**
  ([D-050](../decisions/RT_PDR_00_Decision_Log.md)) e oggi
  `ARTTurnManager::AskReactionDecision` conosce un `OwnerUnitId` e un `bIsBotControlled` letto
  da `ARTUnit`. Senza un'identità di chi decide, quel bank finirebbe attaccato all'unità — cioè `D-050`
  violata dal primo commit che lo implementa;
- **non** autorizza il 16v16, né una UI di scala maggiore, né il networking per più giocatori. È un guardrail:
  il sistema non deve **impedire** quelle modalità, non deve prepararle.

#### Portata dichiarata — e il valore che v0.1 dichiara è **2**, non 1

> 🔴 **Questo paragrafo diceva il contrario ed è stato corretto in code review, prima del merge.** Diceva
> *«l'intervallo utile in v0.1 è `[1, 1]` … finché nessun formato dichiara `2`»*, e la tabella ventisette
> righe più sopra — nella stessa sezione — rispondeva già **2**. Le due letture non erano equivalenti:
> decidono se il fattore di carico del Time Bank sia **vivo oggi** o **dormiente**, cioè un bank di 24 s
> contro uno di 31,5 s nell'unico formato che il gioco spedisce.

`Format.Skirmish2v2` è **offline contro bot**: c'è **un** umano, e comanda la propria squadra intera. Quindi:

| Formato | `UnitsPerTeam` | Hero per **persona** |
|---|--:|--:|
| `Format.Skirmish2v2` (v0.1, spedito) | 2 | **2** — un umano, due unità |
| formato competitivo ipotizzato (una persona per eroe) | 3 · 4 | **1** |

**La v0.1 è già il caso multi-Hero**, ed è la ragione più forte per cui CP 19.3 non è lavoro futuro: il codice
non assume «1 : 1» per prudenza verso una modalità che verrà, lo assume **contro il formato che gira adesso**,
e la coincidenza fra `UnitsPerTeam` e il conteggio per persona è ciò che rende l'errore invisibile.

Il caso `1` è quello **ipotetico**: nasce quando una squadra viene divisa fra più persone. I valori
ammessi sono i **divisori di `UnitsPerTeam`** — non tutto l'intervallo `[1, UnitsPerTeam]`, ed è una
correzione fatta in code review: con `UnitsPerTeam = 4` valgono `1`, `2` e `4`, mentre `3` sta dentro
l'intervallo e viene **rifiutato** dalla ripartizione uniforme descritta sotto. La prima stesura di questa
riga, e di [D-155](../decisions/RT_PDR_00_Decision_Log.md), dichiarava l'intervallo pieno: chi avesse
seguito il Decision Log avrebbe scritto un formato che il validator respinge all'avvio.

Un `MinUnitsPerPlayer` **non serve** comunque: l'estremo inferiore è `1` per costruzione, e due campi per
un solo estremo libero sono un vocabolario più grande del problema.

#### Il campo — `UnitsPerPlayer`, e perché non `HeroesPerPlayer`

> Il campo si chiama **`UnitsPerPlayer`** e sta in `URTMatchFormatData` e `FRTMatchRules`, accanto a
> `UnitsPerTeam`. *(CP 19.3, deciso il 2026-08-17)*

Il vocabolario del formato è **unità**, non *Hero*: `UnitsPerTeam` lo usa da CP 19.2, e `Hero` è il livello
dei **dati** (`URTHeroData`). Due parole per la stessa entità dentro lo stesso struct sarebbero il difetto
che questa spec passa il tempo a evitare altrove.

Il rapporto `UnitsPerTeam / UnitsPerPlayer` è **il numero di persone per squadra**: `2 / 2 = 1` in v0.1,
`3 / 1 = 3` in un competitivo dove ognuno comanda un eroe.

#### La ripartizione è uniforme, e il validator la impone

> Tutte le persone di una squadra comandano lo **stesso** numero di unità. Un formato in cui la squadra non
> si divide viene **rifiutato**. *(decisione dell'autore, 2026-08-17)*

Il validator (`URTMatchFormatLibrary::ValidateRules`) rifiuta con **tre messaggi distinti** — ma i vincoli
indipendenti sono **due**, e la tabella lo dice invece di lasciarlo dedurre. I messaggi sono tre perché chi
legge un allestimento fallito deve sapere **quale** numero correggere:

| Caso | Perché è un errore |
|---|---|
| `UnitsPerPlayer <= 0` | come per `UnitsPerTeam`, zero non è «nessun limite»: è una persona che non comanda niente |
| `UnitsPerPlayer > UnitsPerTeam` | non si comandano unità che la squadra non schiera. ⚠️ **Non è un vincolo indipendente**: per valori positivi `A > B` implica già `B % A ≠ 0`, quindi la riga sotto lo catturerebbe. Esiste per il **messaggio**, che dice la cosa giusta invece di parlare di divisibilità a chi ha sbagliato di grosso |
| `UnitsPerTeam % UnitsPerPlayer != 0` | un resto lascerebbe un gruppo di dimensione diversa, e **un** numero non sa esprimerne due |

⚠️ **Il costo dichiarato**: questo vieta il 3v3 diviso `2 + 1`. È il prezzo di tenere il modello a un solo
campo, e si paga finché nessun formato ne ha bisogno — il giorno in cui servisse, la sostituzione non è un
altro `int32` ma una **ripartizione esplicita**, e va decisa allora con il caso d'uso in mano invece che oggi
per simmetria.

#### Chi legge il campo — e la regola di autorizzazione, rientrata col suo consumatore

Il campo ha per lettori il **validator** e la **regola di autorizzazione**. Fino al 2026-09-02 aveva solo il
primo, ed era deliberato: il precedente sta in questo stesso asset, dove `ExpectedRounds` **non è letto da
alcun codice di gioco** e vive perché il validator ne ha bisogno.

⛔ **La regola era stata scritta e RIMOSSA prima del merge, in code review.** Erano due funzioni pure
(`ControlGroupForUnit`, `CanPlayerControlUnitInGroup`) con il loro test, e non avevano **nessun chiamante**:
l'assegnazione delle unità ai gruppi vive in `ARTGameMode` — dove stanno tutti e sei i consumatori runtime di
`UnitsPerTeam`. *(Fino al 2026-08-20 quel file apparteneva a un'altra track del write-set di batch e si
aspettava il suo owner; con [D-178](../decisions/RT_PDR_00_Decision_Log.md) il vincolo non esiste più.)*

La ragione per cui erano uscite non era la prudenza: è una **regola scritta nel repository**, e sta
nell'header che le ospitava. Il commento di `URTCombatLibrary::IsIntentVisibleTo` dice, di sé:
*«Se un giorno tornasse senza consumatori, la risposta e' rimuoverla, non lasciarla a fare da falsa
copertura»*. Un test verde su una funzione che nessuno chiama misura la funzione, non il gioco — ed è il
difetto che `#507` ha già pagato una volta su quella stessa riga.

✅ **Sono rientrate insieme al consumatore con `#1124`**, dopo che `#937` ha rilasciato `ARTGameMode`. Oggi:

| Pezzo | Dove |
|---|---|
| `ControlGroupForUnit(IndexInTeam, UnitsPerPlayer)` | `URTCombatLibrary` — divisione intera, `INDEX_NONE` fail-closed su ingressi che non partizionano |
| `CanPlayerControlUnitInGroup(...)` | `URTCombatLibrary` — **squadra prima**, poi gruppo; `INDEX_NONE` da un lato o dall'altro rifiuta |
| Assegnazione all'allestimento | `ARTGameMode::AssignUnitControlGroups()`, in coda ad `AssignSeats()` |
| Il gruppo della persona | `ARTPlayerState::ControlGroup`, `Arrival / 2` — i posti si assegnano alternati |
| Il gruppo dell'unità | `ARTUnit::ControlGroup` |
| I due consumatori | le guardie di comando in `ARTPlayerController::HandleClick` |
| Il test | `RefactorTactics.Combat.ControlGroupPartitionsTheTeam` |

⚠️ **Nella v0.1 la regola non cambia alcun esito**, e questo è il punto delicato: un giocatore per squadra
significa un solo gruppo, e ogni unità cade nel caso degenere. Il valore non è comportamentale — è che la
partizione sia *esprimibile e fissata* prima che i posti diventino due. Perché la sostituzione ai due call
site non sia una regressione muta, il test misura che con tutti a gruppo `0` la regola nuova risponda **come
quella vecchia** su ogni combinazione di squadre; i default di entrambi i campi sono `0` proprio per questo.

⚠️ **L'ordine con cui le unità entrano nei gruppi è `StableLess` sulla cella**, lo stesso di
`ARTTurnManager::CollectLivingUnits` — non quello di `GetAllActorsOfClass`, che non promette nulla. Il gruppo
decide *chi comanda* un'unità: farlo dipendere dall'ordine di registrazione degli Actor renderebbe la
partizione diversa a ogni avvio.

**Lavoro tracciato**: E19 · CP 19.3, feature `RT-FEAT-MATCH-FORMAT`. Il consolidamento che ha prodotto questa
sezione è nello [spec panel del 2026-08-17](../roadmap/plans/multihero-timebank-preferred-response-spec-panel-2026-08-17.md) §3 F1.

---

## 17. Telemetria — cosa misurare al playtest

Il canale è quello già progettato in [`spec-pacing-turno.md`](spec-pacing-turno.md) §4: **separato dal
TurnLog**, interi in millisecondi, nessun ritorno verso il gameplay. Metriche da aggiungere:

| Categoria | Metriche |
|---|---|
| **Durata** | `MatchDurationSeconds` · `RoundDurationSeconds` · `PlanningDurationSeconds` · `ResolutionPlaybackSeconds` |
| **Reazioni** | `ReactionWindowCount` · `ReactionDecisionSeconds` |
| **Ready** | `ReadyAtSeconds` |
| **Struttura** | `RoundsPlayed` · `TeamEliminationRound` |
| **Contatto** | `FirstEnemyContactRound` · `FirstObjectiveContestRound` |
| **Mappa** | `CellsTraversedPerUnit` · `MapTraversalRounds` |
| **Stallo** | `TimeWithNoEnemyContact` · `TimeWithNoMeaningfulDecision` |

**Le due che contano di più**: **P50** e **P90** di `MatchDurationSeconds`.

| Target iniziale (3v3 Standard) | Valore |
|---|---|
| **P50** durata match | **~25–30 min** |
| **P90** durata match | **< ~40–45 min** |

Non sono SLA tecnici: sono **target di game design**, e come tutti i KPI del progetto vanno **registrati anche
quando sono fuori target** ([`v0.1-definition-of-done.md`](../roadmap/v0.1-definition-of-done.md) §3, G11).

> **Riserva sul campione, da ripetere accanto a ogni numero**: lo scope corrente è 2v2 offline contro bot. Il
> campione è **un solo giocatore, che è l'autore del gioco** — un P50 misurato così non descrive né il 3v3 né un
> giocatore nuovo. Vale la stessa riserva già registrata per il pacing (`spec-pacing-turno.md` §12).

---

## 18. Impatto sul vertical slice

Il vertical slice **resta 2v2** e **non si espande** per implementare una Standard map completa o Operations.
La demo può usare una mappa **Skirmish** più piccola della futura Standard: **la dimensione della demo non è una
prova che tutte le mappe finali debbano avere quella scala.**

Il vertical slice deve però essere costruito in modo da poter **misurare**: planning time · resolution time ·
numero di decisioni · first contact · round count · match duration · comportamento del Ready · Fast Reaction
da 3 s. La sonda esiste già come design (`spec-pacing-turno.md`); qui si aggiunge cosa deve saper contare.

---

## 19. Stato delle decisioni

### 🔒 Consolidate

- Il match Standard punta a **≤ 30 min medi**.
- **40–45 min** è limite superiore, non target.
- Le mappe **non devono** essere obbligatoriamente piccole come quelle di Atlas Reactor.
- **«Compatto nel tempo, non necessariamente piccolo nello spazio.»**
- `FastReactionDuration` = **3 s** (baseline di sistema).
- **Ready anticipato**.
- **Ready countdown annullabile**.
- Gli obiettivi devono mantenere pressione **anche** su mappe più grandi.
- Un vertical slice più piccolo **non determina** la scala delle mappe finali.
- Le macro-fasi e l'ordine `Prep → Dash → Blast → Move` **non cambiano**.
- La **forma** della configurazione: `URTMatchFormatData` in `Turn/`, ripiego solo in `ARTGameMode`, `FormatId`
  nell'header del TurnLog e **non** nell'hash (§16.1–16.3, issue `#185`).
- **Fine partita a tre vie** — eliminazione, obiettivo, `RoundLimit` — con la **via** dichiarata accanto
  all'esito, e parità allo scadere = **pareggio dichiarato**. Implementata in CP 10.3 (2026-08-07, `#76`).
- **`1 Player = 1 Hero` è un valore del formato, non un'invariante** ([D-155](../decisions/RT_PDR_00_Decision_Log.md),
  §16.4, 2026-08-17): il resolver resta invariante al numero di persone, il Planning resta **una** finestra per
  persona, e il conteggio non è `UnitsPerTeam`.

### 🧪 Baseline da playtestare

| Parametro | Baseline |
|---|---|
| Standard `PlanningMax` | 40–45 s |
| Standard resolution | ~12–20 s |
| Standard round | ~16–20 |
| Standard hard cap | ~20–22 |
| Standard traversal | ~5–7 Move |
| Primo contatto significativo | entro 1–2 round |
| Skirmish round | ~10–14 |
| Skirmish traversal | ~3–4 Move |
| Celle percorribili Standard | ordine di grandezza 150–200 |

**Non trasformare questi valori in requisiti immutabili.** Nessuno di essi è oggi verificato da un test.

### 🔭 Future / fuori scope

Mappe **Operations** · match da 45–60+ min · overtime sofisticato · escalation definitiva degli obiettivi ·
Sneak · Acoustic Mask.

---

## 20. Cosa questa spec **non** fa

- **Non introduce codice.** Nessuna struct, nessun `enum`, nessun parametro nuovo su `ARTTurnManager`.
- **Non cambia `PlanningSeconds`** (30 s in 2v2): il valore arriva dal misurato, come stabilito da
  `spec-pacing-turno.md` D6.
- **Non cambia gameplay già funzionante** per adattarlo a valori ancora da playtestare.
- **Non fa rename globale** di «turno» → «round» (§0).
- **Non modifica il sistema del rumore, della vista o delle reazioni**: rimanda alle rispettive specifiche.
- **Non pianifica Operations**: registra solo che l'architettura non deve precluderlo.

---

## 21. Riferimenti

| Tema | Documento |
|---|---|
| Decisioni vincolanti del progetto | [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) §6 |
| Tempo di **un** turno, sonda e taratura | [`spec-pacing-turno.md`](spec-pacing-turno.md) |
| Pacing del **playback** | [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md) §6 |
| Modello delle reazioni e finestra 3 s | [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) |
| Macro-fasi e vittoria a tre vie | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) |
| Orientamento (direzione dell'Overwatch) | [ADR-0005](../decisions/adr-0005-orientamento.md) |
| Conoscenza parziale, vista e rumore | [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) |
| Obiettivi dinamici e fine partita | [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §5 → **E10** |
| Gate di release e KPI | [`v0.1-definition-of-done.md`](../roadmap/v0.1-definition-of-done.md) §3–§4 |
| Verifiche interattive | [`test-manuali-pie.md`](../technical/test-manuali-pie.md) |
| Decision Log del PDR | [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md) **D-010** |
| Codice toccato dai parametri | `Turn/RTTurnManager.h:134` (`MaxPlaybackSeconds`), `:239` (`PlanningSeconds`) |
| Forma della configurazione, DoD e test | issue `#185` — decisa il 2026-08-07, consumata da **CP 10.3** (`#76`) |
| Precedenti citati in §16.3 | `Turn/RTHexSim.h` (snapshot), `Turn/RTTurnLog.h` (`ERTLogTopology`, versioni 1→3), `Turn/RTTurnLogLibrary.cpp` (`HashTurnLog`) |
