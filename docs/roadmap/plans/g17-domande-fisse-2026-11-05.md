# G17 — le domande fisse: il dialogo del 2026-11-05 e il perché di ciascuna

> `RECORD` · 2026-11-05 · **Non è il contratto: il contratto è la riga `G17` di [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3, e la decisione è [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md).** Questo file è il **perché** di ogni domanda e la provenienza di ogni classe: nato come bozza dell'assistente, corretto dall'autore in un dialogo a una domanda per volta, una risposta per volta. Se la riga e questo file divergono, vale la riga.
>
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

**Che cosa ha già risposto l'autore** (2026-11-05, dopo aver immaginato la scena in partita; le risposte erano «smetto» / «fastidio» / «togli»): `obiettivo`, `leggibile`, `affordance`, `overlay` e `non-si-blocca` sono *fondamentali*; `senza-console` è *di ondata*; `rigiocare` è tolta; `esito` è *fondamentale* (§2.4). Per le ondate 1 e 2 l'autore ha risposto «smetto» a `graybox`, `leggibilita-tattica`, `skill-distinte`, `audio` e `usabili-da-sole`: sono *fondamentali*. Per le ondate 3 e 4: `scelte`, `credibile` e `varieta` sono *fondamentali* («smetto»); `due-mappe`, `livelli` e `otto` sono *di ondata* («fastidio»). Per l'ondata 5: `un-solo-linguaggio` e `lingua` sono *fondamentali* («smetto»); lo stile coerente dei fumetti è fuso in `fumetti`. **Nessuna classe è più una proposta dell'assistente.**

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
  - Viene da «Si usano da sole» ([§3.2](v01-giocabile-brainstorm-2026-11-04.md)). Il brief non assegna dock, mira e tooltip a un'ondata: **l'autore l'ha messa nell'ondata 1** («Sì, ondata 1», [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md)).
- `fumetti` · *fondamentale* — Durante lo svolgimento del piano, quando un personaggio attiva una skill, appare **sopra di lui un fumetto con un grido**, e **anche un'azione bloccata appare come fumetto**, nello stesso stile; la grafica **ricorda quello dei fumetti/manga**, e i fumetti hanno **lo stesso stile del resto dell'interfaccia**?
  - **Aggiunta dall'autore in dialogo** («voglio anche un'altra cosa o smetto»). Le sue parole, alla lettera: «una grafica che ricorda i fumetti, nel senso che quando i personaggi attivano una skill, appare un fumetto sul personaggio con qualche grido, magari informazioni sulla skill che sta per essere lanciata. si vede durante lo svolgimento del piano, e anche i blocchi devono apparire come fumetto/manga, la grafica deve ricordare quello stile».
  - Lo stile coerente col resto dell'interfaccia è un «smetto» dell'autore (ondata 5), fuso qui per non fare due domande.
  - Le «informazioni sulla skill» sono un «magari»: **restano fuori dalla domanda** finché l'autore non dice che sono un requisito.
  - Non era nel brief: è una scelta nuova sullo stile della presentazione, registrata in [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md); **l'autore l'ha messa nell'ondata 2** («ondata 2»), e tocca l'ondata 5 («un solo linguaggio visivo»).
  - **Tensione con `leggibilita-tattica`.** Un fumetto può coprire un'unità, una cella o una copertura, e le azioni di più unità risolvono nella stessa fase. **L'autore prevede che, vista la grandezza degli esagoni, il problema non nasca.** È una **previsione, non misurata**: la misura la fa `leggibilita-tattica` sul pacchetto, e se si sbaglia l'autore risponde «smetto».
  - **Le due viste (deciso dall'autore).** I fumetti si vedono nella vista ravvicinata. Nella **vista strategica** (lo zoom indietro, [`D-252`](../../decisions/RT_PDR_00_Decision_Log.md) e [`D-488`](../../decisions/RT_PDR_00_Decision_Log.md), semplificata) **non si vedono, e le informazioni appaiono nel log a destra** (`WBP_RT_CombatLog`).
  - **Privacy**: un fumetto su un'unità avversaria che il giocatore non vede rivelerebbe posizione o intenzione. Vincolo scritto in [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md): il fumetto segue la stessa visibilità degli eventi. (L'autore non ha risposto a parte su questo punto: è una lettura dell'assistente, ribaltabile.)

**Ondata 3 · Scelte pre-partita**

- `scelte` · *fondamentale* — Dal Main Menu ho scelto **squadra, mappa, avversario, obiettivo e durata**, e la partita è partita con quello che avevo scelto?
- `due-mappe` · *di ondata* — Ho potuto scegliere fra **almeno due mappe**, e la seconda non si legge come graybox?

**Ondata 4a · Bot**

- `livelli` · *di ondata* — Ho scelto il **livello dell'avversario** e due livelli si sono giocati in modo diverso?
  - Viene da: [`D-578`](../../decisions/RT_PDR_00_Decision_Log.md) (3): un livello è un preset di pesi, la difficoltà dà più ragionamento e mai più informazione.
- `credibile` · *fondamentale* — L'avversario ha giocato in modo **credibile**: coordinato, senza mosse che mi sono sembrate senza senso?
- `varieta` · *fondamentale* — Giocando due partite con **le stesse scelte di setup ma piani diversi**, l'avversario si è comportato in modo diverso, e le due partite **non si sono assomigliate**?
  - L'autore ha risposto «smetto» e ha confermato la lettura **(a)**: è la varietà del bot, non il determinismo. A parità di stato iniziale, seme e piani la partita **deve** ripetersi identica (`G4`, `CLAUDE.md` §7): quella lettura (b) non è nel gate.
  - Viene da «varietà fra una partita e l'altra» ([§3.3](v01-giocabile-brainstorm-2026-11-04.md)). ❓ Non verificato che il tetto C di E26 ([`D-578`](../../decisions/RT_PDR_00_Decision_Log.md)) la garantisca: se il bot è deterministico anche a piani diversi dell'avversario, la domanda misura una cosa che E26 non promette.

**Ondata 4b · Roster a 8**

- `otto` · *di ondata* — Ho potuto scegliere fra **otto eroi**, e riconosco ciascuno a vista e a orecchio?
  - Viene da: [`D-579`](../../decisions/RT_PDR_00_Decision_Log.md) (2): otto eroi in due fette.

**Ondata 5 · HUD, schermate, giro finale**

- `un-solo-linguaggio` · *fondamentale* — Menu, setup, HUD e Result parlano **lo stesso linguaggio visivo** (colori, caratteri, stile)?
- `lingua` · *fondamentale* — Ogni scritta in schermata è **nella stessa lingua**?
  - L'autore ha risposto «smetto»: la lingua unica **è un requisito del gate**. Quale lingua non è detto qui; la bozza non la sceglie.

### 2.4 `esito` — la scheda dei casi *(fondamentale)*

> Sezione scritta da un dialogo del 2026-11-05 fra l'autore e l'assistente, una domanda alla volta. **Le decisioni sono dell'autore; le conseguenze sono dell'assistente e sono ribaltabili.**

- `esito` · *fondamentale* — Ogni volta che qualcosa che avevo pianificato **non è andato come previsto** (azione bloccata, annullata o interrotta; movimento o scatto fermato; bersaglio non più valido; unità messa KO), ho visto **la mia azione bloccata**, ho visto **l'azione che l'ha bloccata** e ho capito il **perché**; e, se l'unità responsabile era visibile, ho capito **da dove arrivava**?

**Deciso dall'autore, in dialogo**

1. **La scena.** Pianifica un Blast; durante l'esecuzione l'azione di un avversario, in una fase precedente al Blast, blocca il suo. È una pianificazione interrotta.
2. **Il minimo è «chi» e «perché».** Vedere solo che l'azione non è avvenuta non basta. Dove non c'è un autore, basta il perché.
3. **Regola generale**, non solo per l'`Interrupt`: vale per ogni cosa pianificata che non va come previsto.
4. **La verifica è una lista di casi** da spuntare uno a uno, con la risposta accanto. Non si affida al caso e non è una prova sola.
5. **Tutti i casi della lista.** Quelli rari si **costruiscono a mano**, con gli scenari.
6. **Si guardano sul pacchetto**, non nell'anteprima dell'Editor: il gate giudica ciò che il giocatore ha in mano.
7. **Nella vista strategica i fumetti non si vedono e le informazioni stanno nel log a destra.** Lettura dell'assistente, ribaltabile: `esito` deve quindi valere **in tutte e due le viste**, con i fumetti nella ravvicinata e il log nella strategica.
8. **Il formato del log**: «eroe: azione x interrotta», e sotto «dettagli> motivazione».
9. **Tre cose con visibilità diversa.** Il giocatore vede sempre **la propria azione bloccata** e **l'azione bloccante**; **da dove arriva** non è detto. Lo spettatore del replay può vedere tutto, comprese due unità che interrompono insieme. Lettura dell'assistente: il «chi» del gate è **l'azione bloccante**, non l'unità; l'unità segue la visibilità degli eventi, e nel **dato** deve esserci sempre, perché il replay la mostra.

**Conseguenze, lettura dell'assistente**

- **La lista si ricava dall'enum `ERTActionInvalidReason`** (`Source/RefactorTactics/Turn/RTActionFallbackLibrary.h`) **al commit del candidate finale**, non da una memoria: l'enum cresce in coda per costruzione, e una lista scritta a mano invecchia in silenzio. I motivi che un giocatore non incontra (per esempio `NoMap`, difetto di configurazione; `NoEffect`, catalogo scritto male; `InsufficientMovementPoints`, senza produttore) restano fuori **con il perché dell'esclusione scritto accanto**.
- **Ogni motivo diventa una riga**: motivo · scenario `-RTScenario=<Id>` · *chi* (sì / no / non applicabile) · *perché* (sì / no) · che cosa hai visto, se la risposta è no.
- **Un caso senza scenario, o non visto, non vale come sì**: `G17` non può essere verde.
- **Gli scenari mancanti vanno scritti.** Nel corpus di `Scenarios/` nessun file nomina `Interrupt`, `Interrupted` o `Neutralised` (cercato con grep): il caso della tua scena non ha uno scenario. È lavoro nuovo, l'owner non è deciso e le issue nascono al punto 5 di [#3985](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3985).
- **Va aggiunta una parte alla precondizione meccanica**, misurata da chi non ha riparato: ogni scenario della scheda **parte dal pacchetto candidate** con `-RTScenario=<Id>` e arriva alla fine.

**Non misurato**

- Che `-RTScenario=<Id>` giri su un pacchetto candidate. È predisposto (`RTGameMode.cpp`, e `Scenarios/` è inclusa nel pacchetto da `Config/DefaultGame.ini`), ma non provato.
- **Che uno scenario lanciato dal pacchetto mostri lo stesso HUD e la stessa presentazione di una partita.** Se mostra una vista diversa, la scheda misura una superficie che il giocatore non vede.
- **Quanto della voce arrivi al `WBP_RT_CombatLog`.** Il testo del motivo `Interrupted` è «interrotta» e quello di `Neutralised` «neutralizzata da un'interruzione reciproca» (`Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp`); come la riga venga composta nel pannello non è stato verificato.

**Misurato: il «chi» non è nel dato**

- La voce di TurnLog che il resolver scrive per un'azione interrotta porta: fase `Blast`, categoria `Fallback`, esito `Cancelled`, la cella e l'unità **della vittima**, la cella a cui puntava l'azione, il motivo `Interrupted` (in `Amount`), l'azione e la sua priorità (`Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp`, righe 1988-1998). **Chi ha interrotto non entra nella voce**: `UnitId` è uno solo e lo prende il soggetto (commento alle righe 1977-1978).
- Il formato chiesto dall'autore è «eroe: azione x interrotta» e, sotto, «dettagli> motivazione». La prima riga ha **tutti i dati**: l'eroe, l'azione e il motivo. Il «chi» no.
- Due strade. **Derivarlo** in presentazione dalla voce dell'`Action.Interrupt` dello stesso turno è ambiguo se due unità interrompono la stessa vittima (il resolver scrive una voce sola per azione cancellata, commento alle righe 1954-1956): **non basta** per il replay, dove l'autore vuole vederle entrambe. Resta **aggiungerlo alla voce**, che è un cambio di formato del TurnLog. **Deciso dall'autore** ([`D-582`](../../decisions/RT_PDR_00_Decision_Log.md) (4)): un campo `InterruptedByUnitId` (uno `StableUnitId`, sul modello di `OriginalTargetUnitId`), **fuori dall'hash**, con un valore nuovo di `ERTTurnLogFormatVersion`; con due interruttori ne nomina uno (il primo in ordine canonico) e il replay ricava l'altro dalle voci degli Interrupt. Il codice non è scritto e l'owner nasce al punto 5 di [#3985](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3985).
- Il vincolo di privacy resta: al giocatore l'unità si mostra solo se la vede, con lo stesso filtro per squadra degli altri eventi (`FilterForTeam`). L'azione bloccante e il motivo si mostrano sempre.

**Aperto**

- Il legame fra la scheda e l'enum come **controllo meccanico** (un test che si rompe se nasce un motivo senza riga): è codice e serve il tuo OK.

## 3. Come si arriva a verde, giallo, rosso *(confermato dall'autore: «si»)*

- **Verde**: la precondizione meccanica è `PASS`, ogni domanda *fondamentale* è **sì** e ogni domanda *di ondata* è **sì**.
- **Giallo**: la precondizione è `PASS`, ogni *fondamentale* è **sì**, e almeno una *di ondata* è **no**. Ogni no apre una issue sull'owner dell'ondata con la tua riga su ciò che hai visto. **Se un giallo basta per rilasciare lo decidi tu**, non l'assistente.
- **Rosso**: la precondizione è rossa, oppure almeno una *fondamentale* è **no**.
- **Nessuna domanda si salta, e nessuna diventa «non applicabile» perché la sua ondata manca.** Se l'ondata non è nel candidate, il candidate non è quello finale e `G17` resta `NON ESEGUITO`. Una domanda esce solo se togli la sua ondata dal perimetro, con un `D-nnn` tuo.
- La conclusione si scrive sempre con lo **SHA del pacchetto** e la lista dei no, ciascuno con la sua riga.

## 4. Che cosa è deciso, e che cosa resta

Le domande, le classi, l'ondata di `usabili-da-sole` (1) e di `fumetti` (2), il campo per il «chi» e la regola del §3 sono dell'autore, registrati in [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md). Le domande sono nella riga `G17` **alla lettera**; qui resta il perché.

Resta, con un owner da assegnare al punto 5 di [#3985](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3985): il campo `InterruptedByUnitId` nel TurnLog; gli scenari per `Interrupted` e `Neutralised`; la verifica che `-RTScenario=<Id>` giri sul pacchetto con lo stesso HUD di una partita; un controllo meccanico che leghi la scheda dei casi all'enum (codice, serve l'OK dell'autore).
