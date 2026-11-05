# G17 — le domande fisse: bozza dell'assistente, da correggere dall'autore

> `BOZZA` · 2026-11-05 · **non è contratto.** La riga `G17` di [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3 dice che le domande fisse non sono ancora scritte, e continua a dirlo finché questo file non è corretto.
>
> - **Come si corregge**: modifica questo file direttamente. Cancella una domanda, riscrivila, aggiungine una, cambia la classe. Quello che resta in fondo entra **alla lettera** nella riga `G17` e nel passo 3 del record `U98` ([#3997](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3997)); poi questo file si cancella.
> - **Da dove viene**: la risposta dell'autore del 2026-11-05, «La scrivo io, tu correggi» ([`D-580`](../../decisions/RT_PDR_00_Decision_Log.md), nota ➕): le due domande percettive che `G13` teneva per l'occhio, più una per ondata.

## 1. Che cosa sono e che cosa non sono

- `G17` è **un giudizio d'autore, non un test di usabilità**: nessuna seconda persona, e il limite è scritto nel criterio.
- Ogni domanda ha risposta **sì** o **no**. Il **sì** è sempre il caso buono.
- Un **no** non chiede la causa. Chiede **che cosa hai visto**, in una riga: la causa la cerca chi possiede il codice, su una issue sull'owner.
- **Nessuna risposta è dell'assistente, per costruzione**: ha letto il foglio di conduzione, ha scritto le domande e non gioca. In particolare `affordance` non può avere una risposta sua.
- La **precondizione meccanica** (cancello `tools/seduta/esiti.py`, undo col tasto destro) **non è qui**: sta nella riga `G17`, si misura prima che tu giochi, e se è rossa `G17` non può essere verde.
- Le risposte valgono sul pacchetto di cui dichiari lo **SHA**. Se il pacchetto cambia (ricongelamento) non si riportano: le deroghe di [`D-544`](../../decisions/RT_PDR_00_Decision_Log.md) e seguenti non valgono sul candidate finale ([`D-580`](../../decisions/RT_PDR_00_Decision_Log.md) (2)).

## 2. Le domande

Il nome fra apici è solo un'etichetta per correggere senza rinumerare. **Classe**: *fondamentale* o *di ondata*; la regola del §3 dice che cosa cambia.

**Che cosa ha già risposto l'autore** (2026-11-05, dopo aver immaginato la scena in partita; le risposte erano «smetto» / «fastidio» / «togli»): `obiettivo`, `leggibile`, `affordance`, `overlay` e `non-si-blocca` sono *fondamentali*; `senza-console` è *di ondata*; `rigiocare` è tolta; `esito` è *fondamentale* (§2.4). Per le ondate 1 e 2 l'autore ha risposto «smetto» a `graybox`, `leggibilita-tattica`, `skill-distinte`, `audio` e `usabili-da-sole`: sono *fondamentali*. **Le classi delle domande per le ondate 3, 4a, 4b e 5 sono ancora una proposta dell'assistente.**

### 2.1 Il giro

- `non-si-blocca` · *fondamentale* — In almeno una partita completa del giro «Con scelte» (Play, Match Setup, partita, Result) sono arrivato fino al **Result senza che la partita si bloccasse o si fermasse**?
  - Viene da: [`D-576`](../../decisions/RT_PDR_00_Decision_Log.md) (5), [`D-580`](../../decisions/RT_PDR_00_Decision_Log.md) (1), minimo di una partita completa. **L'autore ha confermato: un blocco in partita è «smetto».**
  - Un no vuol dire: dove si è fermata.
  - Il cancello meccanico controlla già, prima che l'autore giochi, che la partita si completi e che il risultato sia raggiungibile (precondizione della riga `G17`): questa domanda copre il blocco **che il cancello non vede** e che l'autore incontra.
- `senza-console` · *di ondata* — Ho giocato **senza aprire la console, l'editor o una riga di comando**?
  - Viene da: la metà *(a)* della vecchia `fine`. L'autore ha risposto «fastidio»; la lettura dell'assistente è che il «fastidio» fosse per questa metà e che la metà *(b)*, il blocco, sia `non-si-blocca`.
  - Un no vuol dire: che cosa hai dovuto aprire.
- `obiettivo` · *fondamentale* — Una partita è finita **per obiettivo** (la via a punti), e il Result mi ha detto perché era finita, e il perché era vero?
  - Viene da: [`D-578`](../../decisions/RT_PDR_00_Decision_Log.md) (5), [`D-247`](../../decisions/RT_PDR_00_Decision_Log.md) (ScoreToWin vale 5 dal 2026-08-30).
  - Un no vuol dire: che cosa diceva il Result e che cosa era successo davvero.
- ~~`rigiocare`~~ — **tolta dall'autore** («togli»). Lettura dell'assistente, **non confermata**: il «diverte» del traguardo ([§3.1](v01-giocabile-brainstorm-2026-11-04.md)) sta nel verdetto finale che l'autore scrive, che resta libero, e non in una domanda sì/no.

### 2.2 Le due che `G13` teneva per l'occhio

- `leggibile` · *fondamentale* — In ogni schermata che ho incontrato (menu, setup, pianificazione, Result) ho **letto ogni testo e riconosciuto lo stato** senza ingrandire, avvicinarmi o indovinare, e nessun testo era tagliato o sovrapposto?
  - Viene da: `G13`, «l'interfaccia è leggibile». Sul v3 l'assistente ha osservato testo tagliato a 1600×900, sovrapposizioni e un mix inglese/italiano nella schermata di fine partita: sono **osservazioni, non giudizi tuoi** ([§7](../../technical/evidence/85/misure-candidate-v3-2026-10-26.md)).
- `affordance` · *fondamentale* — Ho **trovato dove cliccare** senza aver letto il foglio di conduzione?
  - Viene da: `G13`, «il giocatore trova l'affordance». Il CSV prova che l'input è arrivato, non che fosse ovvio darlo.
- `overlay` · *fondamentale* — **Nessun overlay di debug** era acceso che non avessi acceso io?
  - Viene da: il terzo giudizio di `G13` sul v2. La precondizione lo cerca nel log; qui lo guarda l'occhio.

### 2.3 Una o più per ondata

*(L'ordine delle ondate è quello del brief, [§5](v01-giocabile-brainstorm-2026-11-04.md).)*

**Ondata 1 · Ambiente**

- `graybox` · *fondamentale* — La mappa **non si legge come graybox**: guardandola non vedo superfici grigie di sviluppo?
- `leggibilita-tattica` · *fondamentale* — Unità, coperture, dislivelli e celle restano **leggibili sull'arte**?
  - Il tetto resta quello di [`D-124`](../../decisions/RT_PDR_00_Decision_Log.md) per l'ondata 1: «leggibile, non bello» ([`D-577`](../../decisions/RT_PDR_00_Decision_Log.md) (4)(5)). La domanda non chiede se è bella.

**Ondata 2 · Skill viste e sentite, audio**

- `skill-distinte` · *fondamentale* — Per ogni skill che ho usato ho visto un'**animazione e un effetto** e sentito un **suono** che distinguo da quelli delle altre?
- `esito` — **non è più una domanda di ondata**: è *fondamentale* e ha una sezione sua, il §2.4.
- `audio` · *fondamentale* — Ho sentito **musica, suoni dell'interfaccia ed effetti**, senza dover spegnere nulla perché disturbava?
- `usabili-da-sole` · *fondamentale* — Ho **scelto una skill, mirato, visto l'anteprima dell'esito e confermato** senza spiegazioni?
  - Viene da «Si usano da sole» ([§3.2](v01-giocabile-brainstorm-2026-11-04.md)). ❓ **Il brief non assegna dock, mira e tooltip a un'ondata**: dimmi tu a quale va.
- `fumetti` · *fondamentale* — Durante lo svolgimento del piano, quando un personaggio attiva una skill, appare **sopra di lui un fumetto con un grido**, e **anche un'azione bloccata appare come fumetto**, nello stesso stile; e la grafica **ricorda quello dei fumetti/manga**?
  - **Aggiunta dall'autore in dialogo** («voglio anche un'altra cosa o smetto»). Le sue parole, alla lettera: «una grafica che ricorda i fumetti, nel senso che quando i personaggi attivano una skill, appare un fumetto sul personaggio con qualche grido, magari informazioni sulla skill che sta per essere lanciata. si vede durante lo svolgimento del piano, e anche i blocchi devono apparire come fumetto/manga, la grafica deve ricordare quello stile».
  - Le «informazioni sulla skill» sono un «magari»: **restano fuori dalla domanda** finché l'autore non dice che sono un requisito.
  - ⚠️ **Non è nel brief e non è in nessuna ondata**: è una scelta nuova sullo stile della presentazione. Serve una decisione registrata dell'autore, e l'assistente non assegna il numero. La bozza la mette accanto all'ondata 2 (i fumetti delle skill) e tocca l'ondata 5 («un solo linguaggio visivo»).
  - ❓ **Tensione con `leggibilita-tattica`, da risolvere**: un fumetto può coprire un'unità, una cella o una copertura, e le azioni di più unità risolvono nella stessa fase.
  - ❓ **Privacy**: un fumetto su un'unità avversaria che il giocatore non vede rivelerebbe posizione o intenzione. La bozza lo scrive come vincolo (il fumetto segue la stessa visibilità degli eventi), da confermare.

**Ondata 3 · Scelte pre-partita**

- `scelte` · *di ondata* — Dal Main Menu ho scelto **squadra, mappa, avversario, obiettivo e durata**, e la partita è partita con quello che avevo scelto?
- `due-mappe` · *di ondata* — Ho potuto scegliere fra **almeno due mappe**, e la seconda non si legge come graybox?

**Ondata 4a · Bot**

- `livelli` · *di ondata* — Ho scelto il **livello dell'avversario** e due livelli si sono giocati in modo diverso?
  - Viene da: [`D-578`](../../decisions/RT_PDR_00_Decision_Log.md) (3): un livello è un preset di pesi, la difficoltà dà più ragionamento e mai più informazione.
- `credibile` · *di ondata* — L'avversario ha giocato in modo **credibile**: coordinato, senza mosse che mi sono sembrate senza senso?
- `varieta` · *di ondata* — Con le stesse scelte, due partite **non sono andate allo stesso modo**?
  - ❓ **Probabilmente da cancellare.** Viene da «varietà fra una partita e l'altra» ([§3.3](v01-giocabile-brainstorm-2026-11-04.md)), ma l'assistente **non ha verificato** che il tetto C di E26 la preveda, e a parità di seme una partita deve poter ripetersi uguale (determinismo).

**Ondata 4b · Roster a 8**

- `otto` · *di ondata* — Ho potuto scegliere fra **otto eroi**, e riconosco ciascuno a vista e a orecchio?
  - Viene da: [`D-579`](../../decisions/RT_PDR_00_Decision_Log.md) (2): otto eroi in due fette.

**Ondata 5 · HUD, schermate, giro finale**

- `un-solo-linguaggio` · *di ondata* — Menu, setup, HUD e Result parlano **lo stesso linguaggio visivo** (colori, caratteri, stile)?
- `lingua` · *di ondata* — Ogni scritta in schermata è **nella stessa lingua**?
  - ❓ **Domanda tua, non mia**: non so se la lingua unica è una regola del progetto o una scelta che non hai ancora fatto. Se non lo è, togli la domanda: non va a creare la regola dal fondo.

### 2.4 `esito` — la scheda dei casi *(fondamentale)*

> Sezione scritta da un dialogo del 2026-11-05 fra l'autore e l'assistente, una domanda alla volta. **Le decisioni sono dell'autore; le conseguenze sono dell'assistente e sono ribaltabili.**

- `esito` · *fondamentale* — Ogni volta che qualcosa che avevo pianificato **non è andato come previsto** (azione bloccata, annullata o interrotta; movimento o scatto fermato; bersaglio non più valido; unità messa KO), ho capito il **perché** e, dove c'è un'unità che l'ha causato, **chi**, senza leggere il log?

**Deciso dall'autore, in dialogo**

1. **La scena.** Pianifica un Blast; durante l'esecuzione l'azione di un avversario, in una fase precedente al Blast, blocca il suo. È una pianificazione interrotta.
2. **Il minimo è «chi» e «perché».** Vedere solo che l'azione non è avvenuta non basta. Dove non c'è un autore, basta il perché.
3. **Regola generale**, non solo per l'`Interrupt`: vale per ogni cosa pianificata che non va come previsto.
4. **La verifica è una lista di casi** da spuntare uno a uno, con la risposta accanto. Non si affida al caso e non è una prova sola.
5. **Tutti i casi della lista.** Quelli rari si **costruiscono a mano**, con gli scenari.
6. **Si guardano sul pacchetto**, non nell'anteprima dell'Editor: il gate giudica ciò che il giocatore ha in mano.

**Conseguenze, lettura dell'assistente**

- **La lista si ricava dall'enum `ERTActionInvalidReason`** (`Source/RefactorTactics/Turn/RTActionFallbackLibrary.h`) **al commit del candidate finale**, non da una memoria: l'enum cresce in coda per costruzione, e una lista scritta a mano invecchia in silenzio. I motivi che un giocatore non incontra (per esempio `NoMap`, difetto di configurazione; `NoEffect`, catalogo scritto male; `InsufficientMovementPoints`, senza produttore) restano fuori **con il perché dell'esclusione scritto accanto**.
- **Ogni motivo diventa una riga**: motivo · scenario `-RTScenario=<Id>` · *chi* (sì / no / non applicabile) · *perché* (sì / no) · che cosa hai visto, se la risposta è no.
- **Un caso senza scenario, o non visto, non vale come sì**: `G17` non può essere verde.
- **Gli scenari mancanti vanno scritti.** Nel corpus di `Scenarios/` nessun file nomina `Interrupt`, `Interrupted` o `Neutralised` (cercato con grep): il caso della tua scena non ha uno scenario. È lavoro nuovo, l'owner non è deciso e le issue nascono al punto 5 di [#3985](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3985).
- **Va aggiunta una parte alla precondizione meccanica**, misurata da chi non ha riparato: ogni scenario della scheda **parte dal pacchetto candidate** con `-RTScenario=<Id>` e arriva alla fine.

**Non misurato**

- Che `-RTScenario=<Id>` giri su un pacchetto candidate. È predisposto (`RTGameMode.cpp`, e `Scenarios/` è inclusa nel pacchetto da `Config/DefaultGame.ini`), ma non provato.
- **Che uno scenario lanciato dal pacchetto mostri lo stesso HUD e la stessa presentazione di una partita.** Se mostra una vista diversa, la scheda misura una superficie che il giocatore non vede.

**Aperto**

- Il legame fra la scheda e l'enum come **controllo meccanico** (un test che si rompe se nasce un motivo senza riga): è codice e serve il tuo OK.

## 3. Come si arriva a verde, giallo, rosso *(proposta dell'assistente)*

- **Verde**: la precondizione meccanica è `PASS`, ogni domanda *fondamentale* è **sì** e ogni domanda *di ondata* è **sì**.
- **Giallo**: la precondizione è `PASS`, ogni *fondamentale* è **sì**, e almeno una *di ondata* è **no**. Ogni no apre una issue sull'owner dell'ondata con la tua riga su ciò che hai visto. **Se un giallo basta per rilasciare lo decidi tu**, non l'assistente.
- **Rosso**: la precondizione è rossa, oppure almeno una *fondamentale* è **no**.
- **Nessuna domanda si salta, e nessuna diventa «non applicabile» perché la sua ondata manca.** Se l'ondata non è nel candidate, il candidate non è quello finale e `G17` resta `NON ESEGUITO`. Una domanda esce solo se togli la sua ondata dal perimetro, con un `D-nnn` tuo.
- La conclusione si scrive sempre con lo **SHA del pacchetto** e la lista dei no, ciascuno con la sua riga.

## 4. Che cosa decidi tu

*`esito` (§2.4) e le classi delle domande del giro e di quelle ereditate da `G13` sono già decise. Per le altre:*

1. Quali domande tieni, quali riscrivi, quali aggiungi.
2. La **classe** di ognuna: la mia è una proposta, e decide che cosa rende rosso.
3. `usabili-da-sole`: a quale ondata va.
4. `varieta` e `lingua`: tenerle o togliere.
5. Se la regola del §3 è la tua, o preferisci un'altra (per esempio: un solo no *di ondata* è già rosso).

Quando hai finito, dimmelo: scrivo le domande nella riga `G17` e nello step 3 di `U98` in un commit solo, rieseguo `RefactorTactics.Meta` e i radar, e cancello questo file.
