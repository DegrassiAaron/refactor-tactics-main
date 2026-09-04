# Brief — `Unbalanced` e `Prone`: due stati di movimento

> **Stato**: brief di design · **Data**: 2026-09-04 · **Origine**: `/sc:brainstorm` in sessione, con decisioni
> d'autore prese durante il dialogo
> **Autorità**: subordinato al canone, ad [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §1 per
> l'ordine delle macro-fasi e a [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) per il
> **contratto runtime** degli status.
> **Nessuna implementazione**: questo brief fissa forma, effetti e vincoli. Non apre lavoro.

> 🔴 **`Prone` come è specificato qui NON è rappresentabile dal contratto esistente, e la cosa va decisa
> prima di scrivere codice.** `ARTUnit::ApplyStatus` conosce **due** forme e nessuna terza:
> `Turns > 0`, un conto alla rovescia che `TickStatuses()` decrementa a ogni Cleanup; e la sentinella
> `PersistentWhileOnCell` (`-1`), che finisce in `CellBoundStatuses` e significa *«scade quando l'unità lascia
> la cella»*. Ogni **altro** `Turns <= 0` esce senza applicare nulla — «altro» perché la sentinella `-1` **è**
> un `Turns <= 0`, ed è testata per prima; e l'uscita non è muta, perché propaga comunque lo spegnimento del
> `Burning` che un `Wet` provoca.
> [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) lo dichiara verbatim: *«`ARTUnit::ApplyStatus`
> sa rappresentare `N` turni **oppure** il legame alla cella»*.
> Uno stato che dura *finché non paghi per uscirne* non è nessuna delle due: la sentinella di cella sarebbe
> attivamente sbagliata, perché un'unità a terra **spinta** su un'altra cella si rialzerebbe da sola.
> **Le vie sono tre**, e la scelta è §8.1 — non una domanda di bilanciamento, come questo brief la
> classificava nella sua prima stesura, ma un vincolo di rappresentabilità:
>
> | Via | Costo |
> |---|---|
> | `Prone` ha **anche** una durata, e `1 MP` la rimuove in anticipo (`StatusTurns.Remove`, che già esiste) | nessun contratto nuovo |
> | Una **terza forma** in `ApplyStatus` | è il secondo meccanismo che [D-279](../decisions/RT_PDR_00_Decision_Log.md) vieta |
> | `Prone` **non** è uno status ma un campo di `ARTUnit` | esce dal contratto per la porta di servizio: HUD, bot, checksum e TurnLog perdono `HasStatus` come API unica |

> ⚠️ **Confine con `RT-FEAT-CHARACTER-STATE`.**
> [`brief-stati-personaggio-e-trasformazioni.md`](brief-stati-personaggio-e-trasformazioni.md) dichiara fra i
> propri perimetri gli stati `Environmental`. `Unbalanced` e `Prone` **non** sono stati ambientali in quel
> senso: non descrivono una condizione dell'ambiente subita passivamente, ma l'esito di un **movimento non
> pianificato**. [D-279](../decisions/RT_PDR_00_Decision_Log.md) impone che *«lo stesso contratto di stato
> runtime ha UN solo owner, non entrambi»* e che il confine sia **scritto prima di `E34`**. Questo brief non
> rivendica il framework: dichiara il confine come **da confermare**, e non lo certifica.

---

## 1. La catena

```text
Move · scivolamento                          Blast · push/pull
(causa: ambientale, nessuna sorgente)        (causa: forzata, c'è una sorgente)
             │                                        │
             ▼                                        ▼
      ┌─────────────┐      subisci spinta      ┌─────────────┐
      │ UNBALANCED  │ ───────────────────────► │    PRONE    │
      │  in piedi   │   +1 cella, poi cadi     │   a terra   │
      └─────────────┘   Unbalanced RIMOSSO     └─────────────┘
             │                                        │
      no Sprint                             niente reazione
      spinta ricevuta +1                    Overwatch disarmato, charge persa
      Guard/Brace inerti                    predictive persa
      ri-scivoli di 2                       interposizione non disponibile
      reazione INTATTA                      1 MP per rialzarsi
```

La distinzione fra le due cause **ha già un tipo**: `ERTDisplacementCause`
([`RTFacingLibrary.h:19`](../../Source/RefactorTactics/Turn/RTFacingLibrary.h)) porta `Forced` e
`Environmental`, con il commento che li motiva — *«una spinta ha una sorgente verso cui girarsi, uno
scivolamento no»*.

> ⚠️ **Ma il tipo non è ancora un canale.** Misurato: `ERTDisplacementCause::Environmental` compare **solo nei
> test** (`RTFacingTests.cpp`), l'unico sito di produzione vivo (`RTTurnManager.cpp:2037`) scrive
> `Forced` costante, e `ApplyIceSliding` non nomina l'enum. L'enum è oggi un **parametro** di
> `URTFacingLibrary::FacingAfterDisplacement` con un solo produttore reale, non una causa che lo scivolamento
> già trasporta. Il produttore `Environmental` va **scritto**: è costo, non riuso.

## 2. `Unbalanced` — hai perso l'equilibrio, sei in piedi

**Chi lo applica**: lo scivolamento, nella fase Move.
**Durata**: `2`.

> 🔑 **`2` e non `1`, ed è una conseguenza misurata.** `ARTUnit::TickStatuses()` decrementa ogni status nel
> **Cleanup**, e lo scivolamento avviene nel **Move** — la fase immediatamente precedente. Uno stato applicato
> con durata `1` nascerebbe nel Move e morirebbe nel Cleanup dello stesso turno, senza che nessuna fase
> interposta possa leggerlo.
>
> Il confronto che lo dimostra è `Status.Exposed`, applicato da `Action.Sprint` con **durata 1**
> (`RTCatalogLibrary.cpp:1067`). Sprint risolve in `ERTResolutionPhase::FastMovement`, cioè nel **Dash**, e ha
> quindi Blast e Move davanti a sé prima del proprio Cleanup: durata `1` significa lì *«vale per il resto di
> questo turno»*, ed è come `RTStatusTests.cpp` la pinna — *«durata esplicita: deve scadere in questo
> Cleanup»*. La stessa durata applicata **nel Move** non ha più nessun resto di turno davanti. `2` è il numero
> che rende vera, per uno stato che nasce nel Move, l'espressione di design «dura un round».

**Effetti**

| Effetto | Nota |
|---|---|
| Niente `Sprint` | Coerente con CP 5.1: `Action.Sprint` è già l'unica azione che nega la reazione a chi si muove sregolatamente. Qui la relazione si inverte — chi è sregolato non può sprintare. ⚠️ Il **punto di applicazione non è deciso**: vedi §5. |
| Spinta ricevuta **+1 cella** | Secondo valore su una scala già modellata: `Status.Braced` e `Status.Guarded` sono gli unici stati che *resistono* alla spinta. |
| Trazione ricevuta **+1 cella** | ⚠️ **Qui l'asse è nuovo, e va detto.** [D-038](../decisions/RT_PDR_00_Decision_Log.md) è esplicita: *«La trazione non è resistita: il catalogo v0.1 §1 riserva la resistenza di Guard alla spinta»*. Non esiste un antonimo da rovesciare: amplificare il `Pull` **crea** la scala, non ne aggiunge un valore. |
| `Guard` e `Brace` inerti | ⚠️ **Non è solo resistenza alla spinta.** `Guarded` è un **pool di 15 danni assorbibili** sull'arco frontale ([D-292]) e `Braced` toglie **10 a ogni colpo** da ogni lato: renderli inerti cancella uno strato difensivo molto più grande di quello che l'argomento di §6 discute, su un'unità già dichiarata più spostabile. Il cumulo va prezzato prima dell'implementazione. |
| Ri-scivolamento a **2 celle** | Chi scivola mentre è già `Unbalanced` percorre due celle invece di una. |
| **La reazione resta intatta** | Deliberato. Vedi §4. |

> ⚠️ **Due effetti si sovrappongono sulla spinta.** Con `Braced` inerte il `+1` è superfluo per batterlo:
> il ramo di `Braced` *«non guarda `KnockDist`, quindi regge una spinta di qualunque distanza»*. Il `+1`
> conta contro `Guarded` e contro le unità senza difese, non contro `Braced`.

## 3. `Prone` — sei a terra

**Chi lo applica**: subire `Push` o `Pull` mentre si è `Unbalanced`. Si applica **dopo** che il movimento
forzato (già amplificato di +1) è stato risolto.
**Uscita**: **1 MP** dal budget di movimento del turno.
**Durata**: 🔴 **non decisa** — vedi il riquadro in testa e §8.1. Non è un dettaglio rinviabile: senza una
durata lo stato non è esprimibile con l'API esistente.
**Al momento della caduta `Unbalanced` viene rimosso**: si è `Prone` e basta.

> 🔑 **Perché `Unbalanced` si consuma.** È un tetto naturale: una caduta per scivolata. Chi si rialza è
> stabile e non può essere immediatamente riabbattuto, quindi due avversari che spingono a turno non
> producono una catena di `Prone` senza fine. La coesistenza dei due stati resta **possibile in linea di
> principio** — sono stati indipendenti, non due livelli di una scala — ma nessuna fonte attuale la produce.

**Effetti**

| Effetto | Nota |
|---|---|
| Nessuna reazione per il turno | Per [D-092](../decisions/RT_PDR_00_Decision_Log.md) è **una** attivazione per turno: non si perde «qualche» reazione, si perde *la* reazione. |
| Overwatch disarmato, **charge persa** | Apre una linea di gioco deliberata: spingere per disarmare. |
| Predictive armata persa | ⚠️ Tocca il thin slice v0.1 `Hero.Wraith.InterceptShot`. Vedi §8.4. |
| Interposizione non disponibile | Stesso punto di controllo delle altre reazioni. |
| **1 MP** per rialzarsi | Scala con la mobilità: chi ha 4 MP si rialza e si muove ancora, chi ne ha 1 si rialza e basta. Lo slot principale resta libero — `Prone` costa un turno di posizionamento, non un turno di gioco. |

**Effetti futuri, non ora**: il documento sorgente (§11) propone anche `reduced defense` e
`melee vulnerability`. Restano fuori: `Prone` nasce con il costo di rialzarsi e nient'altro, e cresce quando
servirà.

## 4. Perché la reazione vive su `Prone` e non su `Unbalanced`

È la decisione di bilanciamento centrale di questo brief, e nasce da un'asimmetria misurabile.

Lo scivolamento scatta con **≥ 2 MP residui**: colpisce cioè chi si è mosso *poco*, che è tipicamente chi tiene
posizione — e chi tiene posizione è chi conta sulla reazione. Mettere il blocco della reazione su `Unbalanced`
avrebbe fatto pagare il prezzo più alto del gioco proprio all'archetipo il cui unico strumento veniva tolto,
e lo avrebbe fatto **senza che nessun avversario spendesse nulla**: bastava il terreno.

Con il blocco su `Prone`, la perdita della reazione richiede che qualcuno **investa un'azione** per spingere.
La punizione smette di essere una tassa automatica del terreno e diventa il premio di una giocata in due
tempi: prima lo sbilanciamento, poi la spinta che lo trasforma in caduta.

Il documento sorgente apre §11 con la stessa preoccupazione, e vale come criterio di accettazione:

> «Non rendere ogni errore una perdita completa del turno.»

## 5. Agganci nel codice

Riferimenti misurati al 2026-09-04 su `origin/main`.

| Elemento | Sede | Costo |
|---|---|---|
| Causa `Environmental` | il **tipo** esiste; il **produttore** no (§1) | **da scrivere**, non zero |
| `Unbalanced` applicato | ⚠️ **non `ApplyIceSliding`**: è `static` su un `const FRTHexSnapshot&` e restituisce un percorso — non ha l'`ARTUnit` su cui chiamare `ApplyStatus`. E il chiamante non può dedurlo: cinque uscite diverse (niente ghiaccio, budget insufficiente, arrivo per transizione di layer, ultimo passo non adiacente, cella di slide bloccata) restituiscono **tutte** il `Path` immutato. Serve rendere lo slide **osservabile** — un flag come `bStoppedByTopology`, che due righe più sotto fa già esattamente questo | **medio** |
| Spinta +1 | il punto in cui `KnockDist` accumula la distanza | piccolo |
| Trazione +1 | il punto in cui `PullDist` registra la distanza | piccolo, ma è un **asse nuovo** (§2) |
| `Prone` → niente reazione | le due condizioni che già producono `ERTReactionOutcome::Unavailable`, una in `RTTurnManager.cpp` e una in `RTTurnManager_Blast.cpp` | 2 righe |
| `Guard`/`Brace` inerti | contro `Status.Guarded` / `Status.Braced` — cancella un pool da 15 e un −10, da prezzare (§2) | medio |
| Niente `Sprint` | ⚠️ **punto di applicazione non deciso**: rifiuto in validazione del piano (con un `ERTActionInvalidReason` e una voce `Fallback`) **oppure** scarto al momento della risoluzione (un evento nuovo). Due costi diversi e due tracce di replay diverse | **da decidere** |
| Disarmo + charge persa | `ArmedOverwatches`, `ArmedPredictions` | medio |
| StandUp 1 MP | budget di movimento | medio |
| Ri-scivolamento a 2 celle | `FRTTerrainDef::SlideCells` è oggi letto come **booleano** — limite dichiarato nel suo stesso commento | medio |
| Tag e icone | 🔴 **il costo è invertito rispetto all'intuizione**: `RTIconCatalogTests` **non** legge `Content/Icons/manifest.json` (zero occorrenze in `Source/`). Asserisce che `FindMissingRequiredIcons` sia vuoto su `RequiredIconIds()`, che enumera **ogni tag registrato sotto `Status.`** e pretende una voce che punti a `/Game/RT/UI/Icons/T_<Foglia>`. `RTIconLibrary.cpp` lo dice: *«un tag nuovo senza icona fa cadere la copertura il giorno in cui viene definito»*. Definire i due tag **rompe la suite** finché non esistono due `Texture2D` reali — e `CLAUDE.md` §5 vieta di scrivere `.uasset` a mano | **medio, e bloccante** |

> 🔑 **Il punto giusto per la regola sulla spinta è dichiarato dal codice stesso.** Il commento che accompagna
> `PushResistance` — il gemello di questa regola — dice: *«La regola sta QUI, nel punto in cui la distanza
> viene registrata, e non nei sette produttori di spinta del catalogo: il settimo nascerebbe già rotto»*.
> L'amplificazione di `Unbalanced` appartiene allo stesso punto.

> ⚠️ **`Push` e `Pull` non sono simmetrici.** La distanza di spinta **accumula**
> ([D-085](../decisions/RT_PDR_00_Decision_Log.md)), quella di trazione **sovrascrive**; e `PushResistance`
> non si applica al `Pull`, per scelta esplicita del catalogo v0.1 §1. Una regola che vale «pushato **o**
> tirato» tocca due rami con due semantiche diverse, e non può essere scritta una volta sola senza prima
> unificarli.

## 6. Tre effetti collaterali, tutti nel verso giusto

**`Guard` e `Brace` cadono proprio quando servirebbero.** `Unbalanced` li rende inerti, e sono anche i due
stati che resistono alla spinta. Chi è sbilanciato non può piantarsi per non cadere. Non è un difetto del
modello: è il modello. ⚠️ Ma il prezzo eccede la resistenza alla spinta, ed è il punto aperto di §2.

**Rialzarsi riduce il rischio di ri-scivolare.** La condizione che *impedisce* lo slide è «budget residuo
inferiore a 2», misurata come `MoveBudget - PathCost`. Lo StandUp non è una cella e **non** entra in
`PathCost`: agisce abbassando `MoveBudget`. La conclusione regge — chi si è appena rialzato scivola meno — ma
la leva è il **budget**, non il costo del percorso. ⚠️ E dipende da una cosa non ancora decisa: se lo StandUp
decrementi il budget dello **snapshot** che lo slide legge. Finché non lo è, l'anti-loop è un'aspettativa, non
una proprietà.

**La catena è inter-turno per costruzione.** Il Blast precede il Move — [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md)
**§1**, *«il Move resta dopo il Blast: l'attacco vale da fermo, e muoversi è un impegno che si paga»*.
«Scivolo e poi mi spingono» nello stesso turno non può accadere. L'avversario ha un turno per capitalizzare,
la vittima un turno per lasciare il ghiaccio.

## 7. Prerequisito — lo scivolamento non è ancora un evento

`ERTMoveOutcome` non ha un valore per lo scivolamento. Un'unità che scivola arriva una cella oltre la
destinazione pianificata e il TurnLog scrive **`Moved`**, che quel valore definisce come *«raggiunta la
destinazione pianificata»*: falso.

Finché resta così, tutti gli effetti di questo brief discenderebbero da una causa che il replay non nomina.
Un giocatore che perde Overwatch, predictive, guardia e un punto movimento vedrebbe, nella traccia,
la parola *«Moved»*.

> ⚠️ **E il valore che sembra adatto non lo è.** L'enum porta già `Displaced` — *«spostamento SUBITO, non
> scelto»* (`#307`) — ma il suo commento ne dichiara il meccanismo di attribuzione: chi ha spinto *«si
> ricostruisce dal log stesso»*, cercando nello stesso Blast la voce di categoria `Combat` il cui `TgtCell`
> coincide con il `SrcCell` di questa. Per uno scivolamento quella voce **non esiste**: è la definizione della
> causa ambientale, *«nessuna sorgente»*. Riusare `Displaced` manderebbe il lettore del replay a cercare un
> attaccante che non c'è — cioè produrrebbe, per un evento reale, il sospetto di un difetto del resolver che
> `#307` esiste proprio per rimuovere.

> 📌 **Nessun effetto di questo brief va implementato prima di un valore proprio per lo scivolamento.**
> Aggiunto **in coda** all'enum, dove stanno già `BlockedByTopology`, `StoppedByPrediction`, `Displaced`,
> `DisplacementResisted`, `StoppedByOverwatch`, `SupersededByDash` e `BlockedByCycle`: l'esito viaggia come
> `uint8` nel formato serializzato, quindi le tracce già scritte non cambiano significato. Sette valori prima
> di questo portano lo stesso argomento — è una disciplina consolidata, non un'eccezione da negoziare.

## 8. Domande aperte

1. 🔴 **Come si rappresenta `Prone`.** Non è una domanda di bilanciamento ma di **esprimibilità**: l'API
   conosce due forme e uno stato che finisce quando paghi non è nessuna delle due. Le tre vie sono nel
   riquadro in testa al documento. Fino a che non è decisa, `Prone` non è implementabile — e con essa la
   domanda di contorno: un'unità con `Root`, o senza budget residuo, resta a terra e senza reazione finché
   non recupera movimento.
2. **Spinta bloccata: si cade lo stesso?** `Prone` si applica «dopo il movimento forzato». Se la spinta non
   produce spostamento (nessuna destinazione, cella bloccata, resistenza), non c'è un movimento dopo cui
   cadere. 🔑 La risposta ha già dove essere scritta: `ERTMoveOutcome::DisplacementResisted` esiste per il
   caso *«la spinta è stata registrata e risolta, e l'unità è rimasta dov'era»* (`#420`), e porta il perché in
   `Amount` come `ERTDisplacementBlockReason`. Che quell'esito produca o no `Prone` è la decisione; il posto
   in cui si legge c'è già.
3. **`Unbalanced` permanente sul ghiaccio largo.** `ApplyStatus` conserva la durata maggiore fra quella
   presente e quella nuova: chi cammina su una lastra estesa rinnova lo stato a ogni turno e non lo vede
   scadere mai. Con la reazione fuori da `Unbalanced` la cosa è molto meno grave di quanto sarebbe stata, ma
   resta uno stato senza scadenza effettiva.
4. **Le Predictive Actions.** `Prone` le disarma, e l'unica della v0.1 è `Hero.Wraith.InterceptShot`: una
   scelta dichiarata e pagata un turno prima verrebbe cancellata da una spinta. Va confermato con E18 davanti,
   non ora.
5. **Il prezzo di «`Guard`/`Brace` inerti».** Cancellare un pool da 15 e un −10 su ogni colpo è molto più che
   togliere la resistenza alla spinta. Va misurato prima di implementare, e potrebbe voler dire limitare
   l'inerzia alla sola componente di spostamento.
6. **Dove si nega lo `Sprint`** — in validazione del piano o alla risoluzione. Cambia il costo e cambia ciò
   che il replay racconta.
7. **`Status.Prone` o `Status.Movement.Prone`?** I tag esistenti sono a due livelli; il documento sorgente
   propone tre. Scegliere il primo e restare coerenti, o aprire il sottolivello per l'intera famiglia.
8. **Il ramo «sbatti contro qualcosa».** `ApplyIceSliding` esce senza estendere il percorso quando la cella di
   destinazione è bloccata — ma ⚠️ **quel ramo non distingue niente**: restituisce lo stesso `Path` immutato
   delle altre quattro uscite, e per giunta accorpa «ho sbattuto contro un ostacolo» e «la cella non esiste»
   (fuori mappa). È spazio di design **potenziale**, e va reso osservabile prima di poterci appendere
   qualcosa.

## 9. Rapporto con l'epic G.2 — una proposta, non una sostituzione

[`brief-ghiaccio.md`](brief-ghiaccio.md) §5 colloca `Unbalanced` e `Prone` in **G.2**, epic post-v0.1, insieme
a Traction/Stability per eroe — e **G.3** vi aggiunge il Momentum con resolver a punto fisso, subordinato al
fuzzing deterministico.

Questo brief **propone** di anticipare i due stati senza il motore. È possibile perché l'innesco non è
numerico: il sorgente separava `Unbalanced` da `Prone` con soglie di Momentum (§7: `<60`, `60-89`, `≥90`,
dichiarate graybox), mentre qui la separazione è **categoriale** — quale causa ti ha spostato. Nessuna scala
da tarare, nessun resolver iterativo, nessun fuzzing come prerequisito.

> ⛔ **Questo brief non supersede `brief-ghiaccio.md`, e la scelta non è implicita.**
> [`DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md) vieta di risolvere un conflitto in silenzio: *«O si
> registra `SUPERSEDED` con la fonte che prevale, o diventa una voce di `OPEN_DECISIONS.md`. Mai una scelta
> implicita»*. Finché la decisione non è presa, **`brief-ghiaccio.md` §5 resta vigente** e i due stati
> restano in G.2. Questo documento è la proposta di cambiarlo, non il cambiamento.
>
> **Dove si decide**: la voce [`STA-5`](../OPEN_DECISIONS.md), aperta il 2026-09-04 nella famiglia degli
> status — accanto a `STA-3`, che pone la stessa domanda su `Suppressed` e `Dazed`. Quel che manca per
> chiudere non è più la voce, che esiste: è la **risposta**. Se è sì, si aggiornano `brief-ghiaccio.md` §5 e
> [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md), che elenca i due stati fra ciò che è fuori dalla v0.1.
>
> 🔑 **Una conseguenza che questo brief misura senza trarla.** L'asimmetria di §8.1 — `Unbalanced` è uno
> status a durata come gli altri, `Prone` no — non è solo un problema implementativo: rende la domanda di
> scope divisibile. Si può anticipare il solo `Unbalanced` e lasciare `Prone` col resto del motore, ed è la
> terza uscita registrata in `STA-5`. ⚠️ Non «gratis», però: `Unbalanced` porta con sé i punti aperti §8.5,
> §8.6 e §8.7, e l'amplificazione del `Pull`, che è un asse nuovo.
