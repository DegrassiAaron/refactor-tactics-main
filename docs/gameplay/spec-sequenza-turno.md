# Spec — Sequenza canonica del round

> **Stato**: **normativa** · **Ultimo aggiornamento**: 2026-08-31
> **Ruolo**: è *il* documento che dice **cosa succede in un round e in quale ordine**. Chi deve sapere come si
> risolve un turno legge questo; tutto il resto è dettaglio di una singola area.
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) (invarianti) e agli
> ADR. Dove un ADR e questa spec divergono, **prevale l'ADR** e questa spec è da correggere.
> **Owner del concetto**: sequenza del round, tassonomia temporale, ordine deterministico.

---

## 1. La sequenza

```text
PLANNING
  ├─ azione principale (o Overwatch, che la sostituisce)
  ├─ profilo di Move
  ├─ eventuale Predictive Action, interamente precommitted
  └─ eventuale Reaction armata
        │
     READY / COMMIT          ← da qui gli intenti sono canonici e immutabili
        │
RESOLUTION
   Prep  →  Dash  →  Blast  →  Move
        │
CLEANUP
```

Quattro regole che non si negoziano:

1. **Il `Move` normale è l'ultima fase volontaria standard.** Sta **dopo** il Blast. Non esistono sequenze
   `Move → Attack` né timeline libere.
2. **`Dash`, `Charge`, `Leap`, `Blink`, `Reposition` e il displacement forzato non sono `Move`.** Sono
   mobilità speciali, e stanno prima del Blast perché appartengono alla loro fase — non perché il giocatore
   possa riordinare il turno.
3. **La Resolution non riapre un Planning.** Le sole scelte ammesse durante la risoluzione sono quelle
   dichiarate legali da un *decision boundary* (§2.3), e sono poche e locali.
4. **Un *decision boundary* interrompe un segmento, non aggiunge una macro-fase.** Le macro-fasi restano
   cinque; i boundary sono punti *dentro* di esse.

### 1.1 Segmenti

Il turno è una **sequenza di segmenti**, non un blocco unico ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)):

```text
[ snapshot → risolvi → boundary ] · [ snapshot → risolvi → boundary ] · … · Cleanup
└──────── «raccogli poi applica» ────┘   ripetuto, non violato
```

Un segmento è delimitato dall'inizio di una macro-fase **oppure** da un decision boundary. Ogni segmento ha il
**proprio snapshot** ed è un «raccogli poi applica» completo. Il resolver **non attende mai** dentro un
segmento: lo termina e restituisce il controllo.

L'invariante #3 **si compone, non si deroga**: la scelta del giocatore entra nel TurnLog **come dato**, e il
timeout è una funzione pura costante. Per questo il *quando* del click non cambia l'esito.

### 1.2 L'Environment ha due ruoli, non uno

Il diagramma del §1 mette `CLEANUP` in fondo, e da lì si deduce facilmente la cosa sbagliata: che l'ambiente
sia **solo** una fase finale. Non lo è, e il runtime lo dimostra da prima che questo paragrafo esistesse
([D-291](../decisions/RT_PDR_00_Decision_Log.md)).

| | Quando agisce | Chi lo esegue |
|---|---|---|
| **Environment reattivo** | **durante** la Resolution, come conseguenza di una mutazione appena applicata | `ApplyTerrainOnEnterEffects` (ingresso in una cella) · `ApplyEnvironmentChanges` (strutture, in coda al Blast) · `ResolveEnvironment` (nascita delle superfici) |
| **Environment di propagazione** | nel `Cleanup`, sul tempo del turno | `TickDynamicSurfaces` · `TickDynamicArcs` · `TickDynamicCovers` · durate e danno di `Burning` |

I due ruoli **non sono due sistemi**: sono lo stesso ambiente letto in due momenti, e il secondo non è una
generalizzazione del primo. La prova che la distinzione è reale sta in
[`ERTReactionPassPoint`](../../Source/RefactorTactics/Turn/RTReactionLibrary.h): il valore
`CleanupSurfaceBirth` esiste **apposta** per la finestra fra una superficie che nasce e il danno che infligge
— *«l'unico punto fuori dal Blast»* — e senza di essa `Reaction.HazardEscape` reagirebbe a una cella che
diventerà pericolosa una fase dopo ([D-092](../decisions/RT_PDR_00_Decision_Log.md),
[D-093](../decisions/RT_PDR_00_Decision_Log.md)).

> ⚠️ **La mutazione strutturale è reattiva, ma il suo derived state no.** Una struttura abbattuta si applica
> **a colpi risolti**, e vista e grafo si riaprono **dalla fase successiva** (CP 9.2): chi ha sparato nello
> stesso Blast non guadagna la linea perché il muro è caduto. È la ragione per cui l'ordine dei colpi non
> cambia l'esito, ed è una policy — non un effetto collaterale dell'implementazione.

### 1.3 Il KO libera la cella al proprio commit, non nel Cleanup

Una domanda che questo documento non poneva e a cui un altro rispondeva al contrario. La riga **60** della
tabella di [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §3 mappa *«KO, verifica obiettivi,
decremento cooldown, TurnLog»* sulla macro-fase `Cleanup`, e chi la legge deduce che fino al Cleanup
un'unità abbattuta **occupi** ancora il proprio `FRTCellId`. Quella riga è vera di *cosa* il Cleanup
elabora — la rimozione dell'attore e la scrittura del log — e **falsa di quando la cella si libera**
([D-294](../decisions/RT_PDR_00_Decision_Log.md)).

**La cella si libera al commit del segmento che ha inflitto il danno letale.** Non c'è un momento in cui
un'unità è morta *e* bloccante:

- `ARTUnit::IsAlive()` è `Health > 0` — **calcolata**, non un flag alzato da una fase;
- `ARTTurnManager::CollectLivingUnits` filtra sui vivi, e lo dichiara: *«i morti (es. nel Blast) non si
  muovono e non bloccano»*;
- `URTHexSimLibrary::MakeSnapshot` popola `Occupancy` **solo** con `Unit.bAlive`, e
  `URTMatchSetupLibrary::BuildOccupancy` ripete la stessa regola.

Il §1.1 fa il resto: **ogni segmento ha il proprio snapshot**, quindi l'occupancy si rilegge a ogni
boundary. Un'unità caduta nel `Blast` non compare nell'occupancy del `Move` che segue, e chi si muove dopo
trova la cella libera. Il corpo può restare visibile — la presentazione non è autorità (invariante #3).

> ⚠️ **Vale per costruzione, non per regola, e la differenza conta.** `Occupancy` è **congelata** dentro un
> segmento: nessun punto del codice la muta durante la risoluzione. Oggi non si nota, perché il danno da
> terreno del `Move` si applica **dopo** che l'intera fase è risolta e nessun micro-step può osservare un
> occupante stantio. Il giorno in cui un effetto uccidesse **dentro** la risoluzione di un segmento, il
> comportamento diventerebbe l'opposto senza che nessuno lo decida. È la regressione che
> `Match.KODoesNotBlockItsCellNextPhase` deve intercettare, e quel test **non esiste ancora**.

Una meccanica di cadavere bloccante resta possibile, ma richiede un **oggetto o una regola di gioco
espliciti**: non si ottiene lasciando indietro l'unità abbattuta.

---

## 2. Tassonomia — cosa può accadere, e quando si decide

La distinzione che conta è **quando il giocatore decide**. Confonderle è il modo più rapido di costruire due
sistemi per la stessa cosa.

| | Quando si decide | Input durante la Resolution | Esempio |
|---|---|---|---|
| **Normal Action** | Planning | no | `Hero.Gadget.ArcPulse` |
| **Delayed / Predictive Action** | Planning, **interamente** | **no** | «sparo dove *penso* che arriverai» |
| **Prepared Reaction** | Planning (armata) | no — una sola risposta legale | `Hero.Riktor.Interposition` |
| **Fast Reaction** | **live**, al boundary | sì — evento **esterno** | Overwatch: `FIRE` / `HOLD` |
| **Fast Action** | **live**, al boundary | sì — continuazione di una **propria** azione | `LEFT` / `RIGHT` dopo un'ability |
| **Forced movement** | nessuno: è subito | no | knockback, spinta ambientale |

> ⚠️ **`Delayed Action` ≠ `Fast Action`** ([D-019](../decisions/RT_PDR_00_Decision_Log.md)). La prima è
> decisa in Planning e non riceve informazione nuova; la seconda è una scelta live. Sono state confuse in
> passato — [`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) usava «Fast Action»
> per la prima.

### 2.1 Phase boundary

Il passaggio fra due macro-fasi. Sempre presente, sempre deterministico, nessun input.
I quattro nominati sono le uscite delle fasi attive: `EndPrep · EndDash · EndBlast · EndMove`.

### 2.2 Predictive Action

Dichiarata **interamente** in Planning: bersaglio predittivo (`LockCell · LockLine · LockArea · LockDirection`),
boundary di risoluzione e **fallback in caso di errore**. Al boundary il trigger si valuta come funzione pura
sullo stato di quel momento; se la previsione è corretta risolve, altrimenti fa whiff secondo la policy
dichiarata. **Nessun input umano.**

La v0.1 ne include **una sola** ([D-016](../decisions/RT_PDR_00_Decision_Log.md)), come thin slice: il
framework di trap persistenti resta fuori. Dettaglio: [`brief-delayed-actions.md`](brief-delayed-actions.md).

### 2.3 Decision boundary e finestre

Una reaction armata produce, al trigger, una `FRTReactionOpportunity` con le **risposte legali**:

```text
AllowedResponses ≤ 1  →  commit immediato, NESSUNA finestra        ← reazioni di E5
AllowedResponses ≥ 2  →  decision boundary + finestra 3,0 s        ← Overwatch, E14
```

Un solo modello, non due. Quando la finestra si apre:

- la simulazione autorevole si **ferma globalmente** — se il resto proseguisse, l'avanzamento dipenderebbe dal
  tempo di risposta umano, e il *quando* tornerebbe a decidere l'esito;
- la presentazione **può** rallentare, ma non decide nulla;
- `Timeout → HOLD`, mai `FIRE`: un timeout non spende una risorsa irreversibile;
- trigger simultanei nello stesso micro-step producono **una sola** opportunity multi-bersaglio, mai prompt in
  sequenza (l'ordine di iterazione non deve poter decidere);
- **nessun interrupt annidato** nella v0.1;
- la opportunity inviata al client contiene **solo il presente**: nessun trigger futuro, percorso futuro,
  destinazione o conteggio di opportunity ancora possibili.

I tre regimi *Automatic · Conditional · FastSelect* **emergono dai dati** — `AllowedResponses` più una
eventuale condizione dichiarata in Planning — e **non sono un enum parallelo**. Dettaglio:
[`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) §5.

---

## 3. Ordine deterministico — due ordini, e a cosa serve ciascuno

Qui c'erano **due ordini totali** senza che il rapporto fra loro fosse spiegato. Lo è ora.

### 3.1 Ordine delle AZIONI — implementato

`URTActionQueueLibrary::InstanceLess`, cinque chiavi, nessun float:

```text
MacroPhase → Priority (intera) → ActionId (lessicale) → SourceUnitId → EventSequence
```

È l'ordine con cui le **azioni** risolvono dentro una fase. Nessuna coppia resta indistinguibile, quindi
l'ordine di arrivo nel container non decide mai. Verificato da `Actions.OrderByPriority` e
`Actions.PermutationInvariant`.

### 3.2 Ordine degli EFFETTI simultanei — la domanda è chiusa, e la risposta non è un ordine

**Non esiste un secondo ordine.** Vale §3.1, e basta.

Qui stava dal 2026-08-08 una domanda aperta: `piano-canonico-mvp.md` §5.1 adottava come `FR-RESOLVE-01` una
partizione a sei gruppi sugli effetti — *sistema → unità attiva → alleati → avversari → terreno → globali* —
mai costruita, e questa sezione chiedeva se costruirla o ritirarla.

**Ritirata il 2026-08-31 da [D-293](../decisions/RT_PDR_00_Decision_Log.md)**, per due ragioni misurate.

> 🔴 **La premessa non regge.** APNAP mette per primi gli effetti di **chi ha il turno**. Qui i turni sono
> **simultanei**: nessuno ha il turno, e «unità attiva» non è una cosa che il gioco possieda.
>
> 🔴 **E non avrebbe governato il caso reale.** L'unico caso misurato in cui l'ordine cambia l'esito è *«due
> nemici colpiscono lo stesso bersaglio in Guardia»*: i due attaccanti sono **entrambi avversari**, stesso
> gruppo in ogni permutazione. La partizione non li separa.

✅ **Come si è chiuso invece.** Rendendo **commutativa la mitigazione**, non ordinando gli effetti: la Guardia
diventa un **pool** di danni assorbibili ([D-292](../decisions/RT_PDR_00_Decision_Log.md)), e una somma che non
perde pezzi non ha bisogno di sapere chi viene prima. Il difetto che ha portato qui è provato da
`RefactorTactics.Combat.NegativeFirstHitDeltaIsPermutationInvariant`.

⚠️ La domanda si riapre solo se comparisse un effetto ordine-dipendente **non riducibile a un pool** — e allora
con una premessa che il gioco possa avere, non con l'unità attiva.

### 3.3 State-Based Actions

`FR-RESOLVE-02`: morte a HP ≤ 0 e scadenza degli status si controllano **fra un effetto e il successivo**; un
bersaglio morto invalida gli effetti pendenti che lo riguardano. Vale già per il batch del Blast.

---

## 4. Corrente *vs* north-star

Il confine, senza ambiguità.

| **Corrente / deciso** | **North-star — non costruire** |
|---|---|
| Una sola finestra di decisione per boundary | Stack di reazioni **LIFO interattivo** |
| `opportunity → commit`, modello unico | Interrupt **annidati** |
| Finestra **3,0 s**, `Timeout → HOLD` | `Patch`: modificatori dinamici di un'abilità in corso |
| Snapshot per **segmento** | **5 categorie di velocità** (`Immediate/Reaction/Fast/Standard/Slow`) e `EndOfPhase` |
| Trigger valutato come **funzione pura** | **Reveal progressivo** a livelli (generico → elemento → bersaglio) |
| Ordine totale a 5 chiavi (§3.1) | Timeline di esecuzione **45–60 s** |
| Abilità come `UPrimaryDataAsset` | **JSON** come sorgente delle abilità (resta al solo modding) |

> Il modello north-star non è sbagliato: è **più grande di quello che serve adesso**, e ogni suo pezzo
> aumenta la superficie di non-determinismo. La ricerca da cui viene è conservata in
> [`../archive/gameplay/sequenza-turno-exploratory.md`](../archive/gameplay/sequenza-turno-exploratory.md).

---

## 5. Storia — come ci si è arrivati

Questa spec nasce il **2026-08-02** da un panel `/sc:spec-panel` sul design esplorativo, con il compito di
classificare cosa fosse adottabile. Allora concluse che le finestre live erano **incompatibili** con
l'invariante #3 e le mise north-star, dietro un gate a due condizioni: una ragione di gameplay, e il
multiplayer.

Il **2026-08-07** entrambe le premesse sono cadute. Il documento sull'Overwatch ha mostrato che bait e bluff
**non** sono recuperabili con condizioni dichiarate — se dichiaro «spara al primo che entra», il tank brucia
sempre la reaction — e che quindi la ragione di gameplay esisteva. E la riconciliazione **(b)**, già scritta
in questa spec come ipotesi, si è rivelata sufficiente: non serve derogare all'invariante #3, basta
**comporlo**. [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) ha formalizzato la composizione.

> ⚠️ **Corretto il 2026-08-08.** Fino a questa revisione il documento diceva **entrambe le cose**: in §2 che
> il conflitto C1 era chiuso e le finestre erano in scope, e in §4/§5 di «non implementare finestre live
> nell'MVP» perché «serve il multiplayer». Le due affermazioni non potevano essere entrambe vere, e la seconda
> era quella scritta in forma di divieto — la più facile da prendere per buona. Il gate a due condizioni è
> **superato**; il divieto è stato **rimosso**, non barrato, perché viveva in una sezione intitolata
> *Non-goal*.

Il resto della classificazione originale — cosa del sorgente entrava e cosa no — è compresso in §4. Il panel
completo (Wiegers, Adzic, Cockburn, Fowler, Hohpe, Nygard, Crispin) e il dettaglio riga per riga vivono nella
storia git di questo file, fino al commit di questa riscrittura.

---

## 6. Rapporto con gli altri documenti

| Documento | Cosa possiede |
|---|---|
| [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) | **Prevale**: composizione dell'invariante #3, boundary, timeout, privacy della opportunity |
| [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) | Le macro-fasi e la mappatura delle fasi del catalogo |
| [ADR-0005](../decisions/adr-0005-orientamento.md) | Il facing, e la geometria frontale condivisa da difesa, percezione e Overwatch |
| [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) | Overwatch e i suoi checkpoint (E14) |
| [`brief-delayed-actions.md`](brief-delayed-actions.md) | Delayed / Predictive Action e i boundary nominati |
| [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) | Azioni generiche, profili di `Move`, regimi di risoluzione |
| [`spec-economia-del-turno.md`](spec-economia-del-turno.md) | **Quanto** può fare un'unità dentro questa sequenza: come slot, Movement Point, budget di pivot e cooldown/risorsa si tengono insieme. Questa spec dice *quando* si risolve una voce del piano; quella dice *che cosa consuma, e chi le dice di no* |
| [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md) | Come i segmenti si **riproducono a schermo** (presentazione) |
| [`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) | Quanto dura ciascuna finestra temporale |
| [`spec-pacing-turno.md`](spec-pacing-turno.md) | Come si **misura** il tempo di decisione reale |
