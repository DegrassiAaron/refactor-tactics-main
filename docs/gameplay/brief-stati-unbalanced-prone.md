# Brief — `Unbalanced` e `Prone`: due stati di movimento

> **Stato**: brief di design · **Data**: 2026-09-04 · **Origine**: `/sc:brainstorm` in sessione, con decisioni
> d'autore prese durante il dialogo
> **Autorità**: subordinato al canone, ad [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §3 per
> l'ordine delle fasi e a [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) per il **contratto
> runtime** degli status.
> **Nessuna implementazione**: questo brief fissa forma, effetti e vincoli. Non apre lavoro.

> 🔑 **Nessun contratto nuovo.** `StatusTurns` / `ApplyStatus` / `HasStatus` restano l'unica verità e l'unica
> API, come stabilito da `spec-stati-temporanei-cp82.md` §3 D1. Questo brief aggiunge **due stati dentro
> quel contratto**, non un secondo meccanismo. Il vincolo di [D-279](../decisions/RT_PDR_00_Decision_Log.md)
> — *«lo stesso contratto di stato/dati a runtime deve avere un solo owner»* — è rispettato per costruzione.

> ⚠️ **Confine con `RT-FEAT-CHARACTER-STATE`.**
> [`brief-stati-personaggio-e-trasformazioni.md`](brief-stati-personaggio-e-trasformazioni.md) dichiara fra i
> propri perimetri gli stati `Environmental`. `Unbalanced` e `Prone` **non** sono stati ambientali in quel
> senso: non descrivono una condizione dell'ambiente subita passivamente, ma l'esito di un **movimento non
> pianificato**. Il confine va confermato prima di `E34`; finché non lo è, questo brief non rivendica il
> framework, solo i due stati.

---

## 1. La catena

```text
Move · scivolamento                          Blast · push/pull
ERTDisplacementCause::Environmental          ERTDisplacementCause::Forced
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

La distinzione fra i due inneschi **non è nuova**: `ERTDisplacementCause`
([`RTFacingLibrary.h:19`](../../Source/RefactorTactics/Turn/RTFacingLibrary.h)) la porta già, con il commento
che la motiva — *«una spinta ha una sorgente verso cui girarsi, uno scivolamento no»*. Questo brief le dà un
secondo uso, non un secondo enum.

## 2. `Unbalanced` — hai perso l'equilibrio, sei in piedi

**Chi lo applica**: lo scivolamento, nella fase Move (`URTHexSimLibrary::ApplyIceSliding`).
**Durata**: `2`.

> 🔴 **`2` e non `1`, ed è una conseguenza misurata, non una preferenza.** `ARTUnit::TickStatuses()` decrementa
> ogni status nel **Cleanup**, e lo scivolamento avviene nel **Move** — la fase immediatamente precedente. Uno
> stato applicato con durata `1` nascerebbe nel Move e morirebbe nel Cleanup dello stesso turno, senza che
> nessuna fase interposta possa leggerlo. `2` è il numero che rende vera l'espressione di design «dura un
> round». È la stessa ragione per cui `Exposed`, che dura anch'esso «un round», funziona: viene applicato nel
> **Blast**, e ha delle fasi davanti a sé.

**Effetti**

| Effetto | Nota |
|---|---|
| Niente `Sprint` | Coerente con CP 5.1: `Action.Sprint` è già l'azione che nega la reazione a chi si muove sregolatamente. Qui la relazione si inverte — chi è sregolato non può sprintare. |
| Spinta e trazione ricevute **+1 cella** | Antonimo esatto di `Status.Braced` e `Status.Guarded`, che sono gli unici due stati che *resistono* al forced movement. Nessun asse nuovo: un secondo valore su una scala già modellata. |
| `Guard` e `Brace` inerti | Le due azioni difensive non producono il loro effetto finché sei sbilanciato. |
| Ri-scivolamento a **2 celle** | Chi scivola mentre è già `Unbalanced` percorre due celle invece di una. |
| **La reazione resta intatta** | Deliberato. Vedi §4. |

## 3. `Prone` — sei a terra

**Chi lo applica**: subire `Push` o `Pull` mentre si è `Unbalanced`. Si applica **dopo** che il movimento
forzato (già amplificato di +1) è stato risolto.
**Uscita**: **1 MP** dal budget di movimento del turno.
**Al momento della caduta `Unbalanced` viene rimosso**: si è `Prone` e basta.

> 🔑 **Perché `Unbalanced` si consuma.** È un tetto naturale: una caduta per scivolata. Chi si rialza è
> stabile e non può essere immediatamente riabbattuto, quindi due avversari che spingono a turno non
> producono una catena di `Prone` senza fine. La coesistenza dei due stati resta **possibile in linea di
> principio** — sono stati indipendenti, non due livelli di una scala — ma nessuna fonte attuale la produce.

**Effetti**

| Effetto | Nota |
|---|---|
| Nessuna reazione per il turno | Per [D-092] è **una** attivazione per turno: non si perde «qualche» reazione, si perde *la* reazione. |
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
| Distinzione della causa | `ERTDisplacementCause::{Environmental, Forced}` — **esiste** | zero |
| `Unbalanced` applicato | `URTHexSimLibrary::ApplyIceSliding` | piccolo |
| Spinta +1 | il punto in cui `KnockDist` accumula la distanza | piccolo |
| Trazione +1 | il punto in cui `PullDist` registra la distanza | piccolo |
| `Prone` → niente reazione | le due condizioni che già producono `ERTReactionOutcome::Unavailable`, una in `RTTurnManager.cpp` e una in `RTTurnManager_Blast.cpp` | 2 righe |
| `Guard`/`Brace` inerti | contro `Status.Guarded` / `Status.Braced` | piccolo |
| Disarmo + charge persa | `ArmedOverwatches`, `ArmedPredictions` | medio |
| StandUp 1 MP | budget di movimento | medio |
| Ri-scivolamento a 2 celle | `FRTTerrainDef::SlideCells` è oggi letto come **booleano** — limite dichiarato nel suo stesso commento | medio |
| Tag e icone | `Status.Unbalanced`, `Status.Prone` + due chiavi in `Content/Icons/manifest.json`, che `RTIconCatalogTests` esige | piccolo |

> 🔑 **Il punto giusto per la regola sulla spinta è dichiarato dal codice stesso.** Il commento che accompagna
> `PushResistance` — il gemello di questa regola — dice: *«La regola sta QUI, nel punto in cui la distanza
> viene registrata, e non nei sette produttori di spinta del catalogo: il settimo nascerebbe già rotto»*.
> L'amplificazione di `Unbalanced` appartiene allo stesso punto.

> ⚠️ **`Push` e `Pull` non sono simmetrici.** La distanza di spinta **accumula** ([D-085]), quella di trazione
> **sovrascrive**; e `PushResistance` non si applica al `Pull`, per scelta esplicita del catalogo v0.1 §1. Una
> regola che vale «pushato **o** tirato» tocca due rami con due semantiche diverse, e non può essere scritta
> una volta sola senza prima unificarli.

## 6. Tre effetti collaterali, tutti nel verso giusto

**`Guard` e `Brace` cadono proprio quando servirebbero.** `Unbalanced` li rende inerti, e sono esattamente i
due stati che resistono alla spinta. Chi è sbilanciato non può piantarsi per non cadere. Non è un difetto del
modello: è il modello.

**Rialzarsi riduce il rischio di ri-scivolare.** Spendere 1 MP alza il costo del percorso, e la condizione che
*impedisce* lo slide è proprio «budget residuo inferiore a 2». Chi si è appena rialzato scivola meno. Anti-loop
ottenuto senza scriverlo.

**La catena è inter-turno per costruzione.** Il Blast precede il Move ([ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) §3):
«scivolo e poi mi spingono» nello stesso turno non può accadere. L'avversario ha un turno per capitalizzare,
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
> coincide con il `SrcCell` di questa. Per uno scivolamento quella voce **non esiste**: è la definizione di
> `ERTDisplacementCause::Environmental`, *«nessuna sorgente»*. Riusare `Displaced` manderebbe il lettore del
> replay a cercare un attaccante che non c'è — cioè produrrebbe, per un evento reale, il sospetto di un
> difetto del resolver che `#307` esiste proprio per rimuovere.

> 📌 **Nessun effetto di questo brief va implementato prima di un valore proprio per lo scivolamento.**
> Aggiunto **in coda** all'enum, come `BlockedByTopology`, `StoppedByPrediction`, `Displaced` e
> `DisplacementResisted` prima di lui: l'esito viaggia come `uint8` nel formato serializzato, quindi le
> tracce già scritte non cambiano significato.

## 8. Domande aperte

1. **Durata massima di `Prone`.** Se si esce solo pagando 1 MP, un'unità con `Root` — o senza budget residuo —
   resta a terra, e senza reazione, finché non recupera movimento. Serve un tetto, o si accetta?
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
5. **`Status.Prone` o `Status.Movement.Prone`?** I tag esistenti sono a due livelli; il documento sorgente
   propone tre. Scegliere il primo e restare coerenti, o aprire il sottolivello per l'intera famiglia.
6. **Il ramo «sbatti contro qualcosa» resta inerte.** `ApplyIceSliding` distingue già lo slide che finisce
   libero da quello fermato da una cella bloccata, e in questo modello la differenza non produce nulla. È
   spazio di design disponibile, non un difetto.

## 9. Rapporto con l'epic G.2

[`brief-ghiaccio.md`](brief-ghiaccio.md) §5 colloca `Unbalanced` e `Prone` in **G.2**, epic post-v0.1, insieme
a Traction/Stability per eroe — e **G.3** vi aggiunge il Momentum con resolver a punto fisso, subordinato al
fuzzing deterministico.

Questo brief **anticipa i due stati senza il motore**. È possibile perché l'innesco non è numerico: il
sorgente separava `Unbalanced` da `Prone` con soglie di Momentum (§7: `<60`, `60-89`, `≥90`, dichiarate
graybox), mentre qui la separazione è **categoriale** — quale causa ti ha spostato. Nessuna scala da tarare,
nessun resolver iterativo, nessun fuzzing come prerequisito.

**Conseguenza da registrare**: se questo brief entra, G.2 va riscritta per non rivendicare i due stati una
seconda volta. Il perimetro residuo di G.2 diventa Traction e Stability per eroe.
