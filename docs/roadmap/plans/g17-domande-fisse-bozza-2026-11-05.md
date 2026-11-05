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

### 2.1 Il giro

- `fine` · *fondamentale* — In almeno una partita completa del giro «Con scelte» (Play, Match Setup, partita, Result) sono arrivato in fondo **senza aprire la console, l'editor o una riga di comando**, e senza che la partita si bloccasse?
  - Viene da: [`D-576`](../../decisions/RT_PDR_00_Decision_Log.md) (5), [`D-580`](../../decisions/RT_PDR_00_Decision_Log.md) (1), minimo di una partita completa.
  - Un no vuol dire: dove ti sei fermato, o che cosa hai dovuto aprire.
- `obiettivo` · *fondamentale* — Una partita è finita **per obiettivo** (la via a punti), e il Result mi ha detto perché era finita, e il perché era vero?
  - Viene da: [`D-578`](../../decisions/RT_PDR_00_Decision_Log.md) (5), [`D-247`](../../decisions/RT_PDR_00_Decision_Log.md) (ScoreToWin vale 5 dal 2026-08-30).
  - Un no vuol dire: che cosa diceva il Result e che cosa era successo davvero.
- `rigiocare` · *fondamentale* — Finito il Result, avrei giocato **subito una seconda partita**, senza che nessuno me lo chiedesse?
  - Viene da: il traguardo del brief, «giudicare se il gioco diverte, anche se ancora grezza» ([§3.1](v01-giocabile-brainstorm-2026-11-04.md)).
  - Un no vuol dire: la riga su che cosa ti ha fermato. Non serve che sia un difetto.

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

- `graybox` · *di ondata* — La mappa **non si legge come graybox**: guardandola non vedo superfici grigie di sviluppo?
- `leggibilita-tattica` · *di ondata* — Unità, coperture, dislivelli e celle restano **leggibili sull'arte**?
  - Il tetto resta quello di [`D-124`](../../decisions/RT_PDR_00_Decision_Log.md) per l'ondata 1: «leggibile, non bello» ([`D-577`](../../decisions/RT_PDR_00_Decision_Log.md) (4)(5)). La domanda non chiede se è bella.

**Ondata 2 · Skill viste e sentite, audio**

- `skill-distinte` · *di ondata* — Per ogni skill che ho usato ho visto un'**animazione e un effetto** e sentito un **suono** che distinguo da quelli delle altre?
- `esito` · *di ondata* — Ho capito l'**esito di un'azione** (colpo, danno, KO) guardando e ascoltando, senza leggere il log?
- `audio` · *di ondata* — Ho sentito **musica, suoni dell'interfaccia ed effetti**, senza dover spegnere nulla perché disturbava?
- `usabili-da-sole` · *di ondata* — Ho **scelto una skill, mirato, visto l'anteprima dell'esito e confermato** senza spiegazioni?
  - Viene da «Si usano da sole» ([§3.2](v01-giocabile-brainstorm-2026-11-04.md)). ❓ **Il brief non assegna dock, mira e tooltip a un'ondata**: dimmi tu a quale va.

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

## 3. Come si arriva a verde, giallo, rosso *(proposta dell'assistente)*

- **Verde**: la precondizione meccanica è `PASS`, ogni domanda *fondamentale* è **sì** e ogni domanda *di ondata* è **sì**.
- **Giallo**: la precondizione è `PASS`, ogni *fondamentale* è **sì**, e almeno una *di ondata* è **no**. Ogni no apre una issue sull'owner dell'ondata con la tua riga su ciò che hai visto. **Se un giallo basta per rilasciare lo decidi tu**, non l'assistente.
- **Rosso**: la precondizione è rossa, oppure almeno una *fondamentale* è **no**.
- **Nessuna domanda si salta, e nessuna diventa «non applicabile» perché la sua ondata manca.** Se l'ondata non è nel candidate, il candidate non è quello finale e `G17` resta `NON ESEGUITO`. Una domanda esce solo se togli la sua ondata dal perimetro, con un `D-nnn` tuo.
- La conclusione si scrive sempre con lo **SHA del pacchetto** e la lista dei no, ciascuno con la sua riga.

## 4. Che cosa decidi tu

1. Quali domande tieni, quali riscrivi, quali aggiungi.
2. La **classe** di ognuna: la mia è una proposta, e decide che cosa rende rosso.
3. `usabili-da-sole`: a quale ondata va.
4. `varieta` e `lingua`: tenerle o togliere.
5. Se la regola del §3 è la tua, o preferisci un'altra (per esempio: un solo no *di ondata* è già rosso).

Quando hai finito, dimmelo: scrivo le domande nella riga `G17` e nello step 3 di `U98` in un commit solo, rieseguo `RefactorTactics.Meta` e i radar, e cancello questo file.
