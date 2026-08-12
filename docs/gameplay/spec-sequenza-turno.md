# Spec — Sequenza canonica del round

> **Stato**: **normativa** · **Ultimo aggiornamento**: 2026-08-08
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

---

## 2. Tassonomia — cosa può accadere, e quando si decide

La distinzione che conta è **quando il giocatore decide**. Confonderle è il modo più rapido di costruire due
sistemi per la stessa cosa.

| | Quando si decide | Input durante la Resolution | Esempio |
|---|---|---|---|
| **Normal Action** | Planning | no | `Flux.ArcPulse` |
| **Delayed / Predictive Action** | Planning, **interamente** | **no** | «sparo dove *penso* che arriverai» |
| **Prepared Reaction** | Planning (armata) | no — una sola risposta legale | `Bastion.Interposition` |
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

### 3.2 Ordine degli EFFETTI simultanei (APNAP) — deciso, **non implementato**

[`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) §5.1 adotta come `FR-RESOLVE-01..03` un ordine a
sei gruppi — sistema → unità attiva → alleati → avversari → terreno → globali — per il caso in cui più
**effetti** risolvono nello stesso istante **e l'ordine conta** (scudo o reazione prima del danno).

> ⚠️ **Verificato il 2026-08-08: i sei gruppi non esistono nel codice.** `grep -rn "APNAP\|FR-RESOLVE" Source/`
> trova **un solo commento**, in `RTActionQueueLibrary.h`, che dichiara di *estendere* la regola. Ciò che è
> stato costruito è l'ordine di §3.1, che assorbe la parte intra-gruppo di APNAP («priorità intera →
> tie-break assoluto») ma **non** la partizione per appartenenza.
>
> Non è un difetto da correggere di corsa: il canone dichiara l'implementazione **gated**, «si esercita quando
> esistono effetti ordine-dipendenti». Ma è la solita forma — **una regola normativa che nessun consumer
> legge** — e va detta, non lasciata dedurre. Quando E14 porterà reazioni che modificano il danno *prima* che
> venga applicato, o si costruiscono i gruppi o si dichiara che §3.1 basta e §5.1 del canone va riscritto.

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
