# Spec — Economia del turno: quanto può fare un'unità, e come il movimento cambia cosa può fare

> `CURRENT` · **Owner** della domanda «quanto può fare un'unità in un turno, e che cosa il movimento scelto
> toglie o aggiunge alle sue azioni». · Creato il **2026-08-12** consolidando il kit d'autore
> `CLAUDE_ActionEconomy_Movement_Facing_Consolidation_2026-08-12.md` — referto:
> [`../roadmap/plans/action-economy-consolidamento-2026-08-12.md`](../roadmap/plans/action-economy-consolidamento-2026-08-12.md).
>
> **Non sostituisce nessuno**: [`spec-sequenza-turno.md`](spec-sequenza-turno.md) resta owner dell'**ordine
> delle fasi**, [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) del confronto **fra le famiglie
> di movimento**, [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) del **facing e del pivot**,
> [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) dei **numeri**,
> [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) dell'**Overwatch**.
> Qui vive ciò che nessuno di loro possiede: **come i quattro budget si tengono insieme**, e la domanda —
> nuova — se il movimento scelto debba cambiare le azioni.

## Perché esiste

Il progetto ha **quattro** limiti diversi al turno di un'unità — gli slot, i punti movimento, il budget di
pivot, cooldown e risorsa — e li ha decisi in quattro momenti diversi, ognuno nel proprio documento. Nessuno
risponde alla domanda che si fa chi progetta un'abilità nuova: *questa cosa cosa consuma, e chi le dice di no?*

Rispondere caso per caso è il modo in cui nascono cinque economie parallele. Questa pagina è la risposta unica.

La seconda ragione è più recente: il kit del 2026-08-12 propone di **legare** movimento e azione — che un
Move lungo tolga precisione, che uno scatto renda un'abilità impossibile. Il canone oggi **non lo fa**: il
profilo di movimento cambia distanza, rumore ed esposizione, e finisce lì. Quella proposta ha bisogno di un
posto dove essere scritta come candidata senza essere scambiata per regola.

## 1. La regola, oggi

> **Un turno dà a ciascuna unità un movimento, un'azione principale e una reazione.** Ciò che il giocatore
> sceglie non è *quante* cose fare, ma **quando** muoversi: *schivo e sparo* (mobilità in fase `Dash`, poi
> attacco nel `Blast`) oppure *sparo e muovo* (attacco nel `Blast`, poi `Move`). Il costo di un'azione non è
> un numero da spendere: è **quale slot occupa**. Il movimento ha un budget proprio in Movement Point, il
> pivot un budget proprio in step, e cooldown e risorsa firma dicono ogni quanto un'abilità è ripetibile.

Owner della regola: [D-028](../decisions/RT_PDR_00_Decision_Log.md) per gli slot,
[D-015](../decisions/RT_PDR_00_Decision_Log.md) per i profili, [D-060](../decisions/RT_PDR_00_Decision_Log.md)
e [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) per il pivot.

**Non è un sistema di Action Point**, e la differenza non è terminologica: con gli AP due azioni leggere
valgono una pesante, e la legalità è una sottrazione. Qui la legalità è **strutturale** — `Guard` e
`BasicAttack` non si sommano perché occupano la stessa cosa, non perché il totale sfori.

> ℹ️ Il catalogo azioni registra che **`Slot ≡ Action Points`** come nota terminologica: il workbook
> `RefactorTactics_Balance_Matrices_v0.1.xlsx` modella gli stessi slot come risorse `RES_ACTION` (cap 2) e
> `RES_REACTION` (cap 1). È un sinonimo ammesso nei documenti di bilanciamento, **non** un secondo sistema —
> e il workbook resta `RESEARCH` ([D-023](../decisions/RT_PDR_00_Decision_Log.md)).

## 2. Che cosa dichiara un piano

Un piano completo dichiara: **percorso di movimento · azione principale · reazione (se disponibile) · facing
finale · fallback**. Bersaglio, cella, area, direzione, destinazione, politica di trigger sono **parametri**
di quelle voci, non voci in più.

| Voce del piano | Slot | Fase di risoluzione | Esempi |
|---|---|---|---|
| mobilità speciale | Movimento | **`Dash`** | `Dash`, `Leap`, `Reposition` |
| movimento normale | Movimento | **`Move`** | profili `Sneak` · `Move` · `Withdraw` — e `Sprint`, che è di questa famiglia ma risolve **pre-Blast**: vedi §3.1 |
| azione principale | Principale | dipende dall'azione | `BasicAttack`, `Interact`, `Charge` (**`Blast`**) · `Guard`, `Brace`, `Overwatch`, `CreateCover` (**`Prep`**) |
| reazione | Reazione | al trigger | `Counter`, `Intercept`, `Deflect` |
| facing finale | — | fine `Move` | rotazione dichiarata entro il budget di pivot |
| `Wait` | **nessuno** | `Move`, priorità ultima | resta osservabile nel TurnLog senza togliere niente |

Tre conseguenze che si sbagliano spesso:

- **la fase `Prep` non è uno slot.** `Guard`, `Brace` e `Overwatch` risolvono presto, ma occupano la
  **principale**: prepararsi *è* la propria azione del turno. Un piano «`Guard` + attacco» non è legale.
  ⚠️ **I tre non pagano però lo stesso prezzo**, e la differenza non è scritta nel loro slot: `Brace` applica
  a sé `Braced` **e `Root`**, quindi si pianta — `EffectiveMoveRange` va a zero già nel `Move` dello stesso
  turno — e l'`Overwatch` riserva il movimento a `Withdraw`. Solo `Guard` costa esattamente uno slot. È la
  ragione per cui `ECO-1` ha probabilmente **due** risposte e non una ([#617](https://github.com/DegrassiAaron/refactor-tactics-main/issues/617));
- **la reazione non è un premio.** È indipendente da movimento e principale: chi si muove e agisce la
  conserva comunque. `Wait` non la regala, e `Sprint` la toglie per una regola propria dichiarata;
- **armare l'`Overwatch` non vieta il `Dash` con una regola apposta.** Riserva lo slot movimento al profilo
  `Withdraw`, quindi il `Dash` non è vietato: **non c'è più lo slot**
  ([D-070](../decisions/RT_PDR_00_Decision_Log.md)). La differenza conta perché una regola a sé andrebbe
  mantenuta, testata e spiegata; una conseguenza no.

## 3. I quattro budget, e che cosa ciascuno decide

| Budget | Risponde a | Unità | Dove vive |
|---|---|---|---|
| **Slot** | *cosa posso fare questo turno?* | 1 movimento · 1 principale · 1 reazione | `ERTActionSlot`, catalogo §«Slot per turno» |
| **Movement Point** | *fin dove arrivo?* | interi, per cella e per profilo | `FRTActionDef::CostMP`, `MoveBudget` |
| **Pivot** | *con quale orientamento arrivo?* | step esagonali (0–3), **per eroe** | `MoveEndPivotMaxSteps` / `DashEndPivotMaxSteps` |
| **Cooldown + risorsa firma** | *ogni quanto posso ripeterlo?* | turni interi · punti risorsa | `CooldownTurns`, catalogo eroi §«Risorsa firma» |

Sono **quattro assi separati e non convertibili**: non si compra un pivot con dei Movement Point, non si paga
un cooldown con uno slot. È ciò che impedisce al sistema di collassare in un pool unico dove ogni scelta è
una sottrazione.

### 3.1 I profili del movimento normale

`Sneak` · `Move` · `Sprint` non sono tre azioni concorrenti: sono **tre profili della stessa famiglia**, con
lo stesso slot e la stessa macro-fase. Cambiano **distanza, rumore ed esposizione** — non l'economia del turno
([D-015](../decisions/RT_PDR_00_Decision_Log.md)). `Withdraw` è il quarto profilo e **non si sceglie**: lo
impone l'`Overwatch`.

I budget sono nel catalogo (`Move` 5 MP · `Sprint` 8 · `Withdraw` 2; **`Sneak` non è definito da nessuna
fonte corrente** e non si inventa).

**`Sprint` è l'eccezione che vale la pena leggere due volte**: appartiene alla famiglia `Move` — percorso a
budget, pathfinding, slot movimento — ma **risolve pre-Blast**, in `ERTResolutionPhase::FastMovement`. Non è
un arretrato di migrazione: è una decisione esplicita ([D-068](../decisions/RT_PDR_00_Decision_Log.md)),
verificata da `Actions.SprintIsAMoveProfileResolvedPreBlast`, che asserisce **stile, slot e fase insieme** ed
è scritta per *cadere* se un giorno la fase venisse spostata. «Profilo di `Move`» è un'affermazione sulla
**famiglia**, non sul **momento**; e spostarlo dopo il Blast gli toglierebbe l'ultimo prezzo che paga,
l'esposizione al fuoco di questo turno.

### 3.2 Il pivot non si paga in Movement Point

La rotazione a fine movimento è una **capacità del personaggio** misurata in step, non un acquisto. ADR-0008
§5 lo dice in negativo — *«la rotazione non consuma slot»* — e la scala 0–3 esiste proprio per esprimere
«questo eroe non può girarsi» senza doverlo dedurre da un budget esaurito.

Ciò che il movimento decide è **da quale lato arrivi**: l'ultimo passo fissa l'orientamento di partenza, e il
budget di pivot dice quanto lo si può correggere. Un percorso più lungo può quindi essere la mossa migliore.

> ⚠️ Il kit del 2026-08-12 propone di far **consumare Movement Point** al pivot. È una proposta che cambia
> ADR-0008, non un aggiornamento: registrata come **`FAC-12`** in
> [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) — dove era **già** aperta dal 2026-08-10, proposta da un
> altro kit. Finché è aperta, «il pivot costa MP» non si scrive in
> nessun documento come se fosse vero.

## 4. La proposta nuova: il movimento cambia le azioni — `PROTOTYPE`

> 🧪 **Tutto ciò che segue è un candidato, non una regola.** Nessun dato, nessun codice e nessun test lo
> esprime oggi. Feature: `RT-FEAT-ACTION-MOVEMENT-COMPAT`, epic **E38**, release **v0.2**.

Oggi il profilo di movimento non tocca le azioni: si può sprintare e sparare con la stessa precisione con cui
si spara da fermi, e il prezzo dello Sprint è tutto in `Status.Exposed` e nella rinuncia alla reazione. Il
kit propone che il movimento **entri nella legalità e nell'efficacia** delle azioni.

### 4.1 Quattro stati, non una tabella di percentuali

Un'abilità dichiara come si comporta sotto ciascun profilo:

```text
NORMAL     nessun cambiamento
IMPAIRED   legale, ma con un effetto ridotto dichiarato
ENHANCED   legale, con un effetto aumentato dichiarato
BLOCKED    non pianificabile con quel profilo, con reason code
```

I nomi dell'enum non sono bloccati. Ciò che va conservato è la **discretezza**: quattro stati leggibili in una
finestra di Planning valgono più di una pila di modificatori percentuali che nessuno somma a mente.

Esempi del kit, riportati come esempi e non come contenuto approvato:

| Abilità | Fermo | `Sneak` | `Move` | `Sprint` |
|---|---|---|---|---|
| tiro di precisione | NORMAL | NORMAL | IMPAIRED | **BLOCKED** |
| tiro mobile | NORMAL | NORMAL | NORMAL | IMPAIRED |
| colpo di slancio | IMPAIRED | NORMAL | NORMAL | **ENHANCED** |

### 4.2 Perché è interessante, e cosa costa

**Interessante**: risolve un difetto reale e già registrato — senza costo di slot, lo `Sprint` regge il
proprio prezzo solo su `Exposed`, e se il playtest dicesse che non basta oggi non c'è nessuna leva che non
sia un numero. La compatibilità è una leva **strutturale**: lo Sprint smette di essere «un Move più lungo»
perché cambia *cosa puoi fare arrivandoci*.

**Costa**: un asse di bilanciamento per abilità × quattro profili, cioè lo stesso tipo di costo che ADR-0008
ha dichiarato apertamente per gli otto numeri del pivot. E costa in leggibilità: il giocatore deve capire
**prima di confermare** perché un'abilità è grigia.

**Vincolo non negoziabile**: dev'essere un **dato dell'abilità**, mai un ramo per eroe nel resolver
(invariante #7, [D-029](../decisions/RT_PDR_00_Decision_Log.md)). Se per aggiungere «Vektor spara in corsa»
serve toccare `ARTTurnManager`, il modello non serve.

### 4.3 I fatti del percorso sono un'altra cosa

Il kit propone anche modificatori derivati dal **percorso compiuto** — celle percorse, dislivello, cambi di
direzione, superfici attraversate — non dal profilo scelto.

Vanno tenuti separati da §4.1, e la ragione è di determinismo: il **profilo** è noto in Planning e si
previsualizza esattamente; i **fatti del percorso** sono noti solo dopo la risoluzione del movimento, e un
Move può essere troncato, contestato o annullato ([D-045](../decisions/RT_PDR_00_Decision_Log.md)). Una
preview che promette «+1 spinta perché avrai percorso 4 celle» promette qualcosa che il turno può smentire.

Confonderle produrrebbe la prima anteprima del gioco che mente. Sono due feature, e la seconda **non** è
nello scope di E38.

## 5. Reason code: si estendono le famiglie, non se ne crea una

Il TurnLog ha già famiglie di esito **serializzate e versionate** per fase — `ERTMoveOutcome`,
`ERTCombatOutcome`, `ERTFallbackOutcome`, `ERTMovementStopReason` — con l'invariante che i valori nuovi si
aggiungono **in coda**, perché viaggiano come interi nei replay.

Un `PlanRejectionReason` parallelo con undici valori nuovi rifarebbe l'errore già respinto in
[`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) §6, dove dieci reason code proposti da un kit
erano **sette duplicati** di codici esistenti con un altro nome.

Resta però un buco vero, e va detto: **la validazione in Planning non esiste come componente**. `git grep
ValidatePlan` non restituisce nulla; ciò che esiste è `ValidateInstance`/`ApplyFallback`, che agisce **in
risoluzione**. Oggi un piano illegale si scopre quando non funziona, non quando lo si compone. È il
contributo strutturale che E38 può dare a prescindere da come `AE-1` venga decisa.

## 6. Che cosa resta aperto

Otto voci in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), sezione «economia delle azioni». Le tre che
bloccano tutto il resto:

| ID | Domanda | Blocca |
|---|---|---|
| `AE-1` | Gli slot diventano una **capacità numerica** con costi per azione? | tutto il modello: schema del piano, validatore, HUD |
| `AE-2` | Il profilo di movimento può **cambiare la legalità e l'efficacia** di un'azione? | §4, E38 |
| `FAC-12` | Il pivot consuma Movement Point? | due scenari, il costo del facing |

Il resto — nome player-facing della capacità, valori dei profili, budget dello `Sneak`, taratura della
risorsa firma — è taratura, e non si decide a tavolino.

## Rapporto con gli altri documenti

| Documento | Cosa possiede |
|---|---|
| [`spec-sequenza-turno.md`](spec-sequenza-turno.md) | l'ordine delle fasi e dei boundary |
| [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) | il confronto fra le famiglie di movimento |
| [`spec-dash.md`](spec-dash.md) | il Dash: dati, risoluzione, bot, playback |
| [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) | Overwatch, profili delle generiche |
| [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) | pivot, rotazione dichiarata, policy di facing |
| [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) | i numeri: slot, fasi, MP, cooldown |
| [`../technical/progettazione-hud.md`](../technical/progettazione-hud.md) | Action Dock e Ghost Timeline |
| *questa pagina* | come i quattro budget si tengono, e la proposta di legarli al movimento |
