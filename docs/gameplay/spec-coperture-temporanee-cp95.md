# Coperture temporanee e pannello cinetico — E9.5

> Checkpoint **E9.5** (issue [`#73`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/73)),
> chiuso il **2026-08-09**. Chiude l'epic **E9**.
> Owner del modello di copertura: [`spec-copertura-cp91.md`](spec-copertura-cp91.md) (bassa) e
> [`spec-copertura-alta-cp92.md`](spec-copertura-alta-cp92.md) (alta). Qui si aggiunge una sola cosa: le
> coperture si possono **erigere e spostare durante la partita**, e possono **scadere**.

## 1. Che cosa cambia

Fino a E9.4 una copertura era dato di mappa: la disegnava chi costruiva il livello, la si poteva solo
danneggiare. Da qui:

- `Action.CreateCover` erige una copertura bassa su un bordo, con integrità e durata;
- `Bastion.KineticPanel` è quell'azione con un nome d'eroe, e la sua **variante** decide i due numeri;
- `Bastion.Reconfigure` sposta una copertura esistente senza duplicarla;
- `Gadget.PortableCover` dà la stessa capacità a chi non è Bastion;
- una copertura temporanea **scade nel Cleanup**, e la sua rimozione passa dal canale che invalida le cache.

Tre identità a `Effects` vuoto smettono di essere inerti. Erano dichiarate tali di proposito — il catalogo
scriveva «nessun modello di struttura esiste» — e quella dichiarazione era vera fino a E9.1.

## 2. Le decisioni

### 2.1 La fase è `Prep`, e il catalogo azioni si è allineato al catalogo eroi

`RT_ActionCatalog_v0.1.md` dava `Action.CreateCover` nel **Blast**; `RT_HeroCatalog_v0.1.md` e il codice
davano `Bastion.KineticPanel` in **Prep**, con motivazione scritta. Prevale **Prep** ([D-040](../decisions/RT_PDR_00_Decision_Log.md)).

Il precedente in senso opposto esiste ed è recente: E9.4 portò `Action.ModifyArc` **nel** Blast, perché
«porte, muri e ponti cambiano tutti nello stesso momento e il Move che segue li vede». Non si applica qui, e
la ragione è precisa: quell'argomento riguarda la **topologia**. Un arco o una porta cambiano il grafo, e un
percorso già calcolato va troncato con un reason code. Una copertura **bassa** non tocca né grafo né vista —
riduce il danno di 10 dal lato riparato. Non c'è nessun percorso da invalidare.

L'argomento contrario invece regge: eretta nel Blast, la copertura arriverebbe **dopo** aver incassato i colpi
di quel Blast (invariante #3, «raccogli poi applica»), cioè nel turno in cui la si paga non servirebbe a nulla.

### 2.2 Portata 3, e il bordo entra nel piano

Si tiene il `Range 3` del catalogo azioni; il catalogo eroi, che dava 1 a `KineticPanel`, si allinea.

Il prezzo è un dato nuovo nella pianificazione. A portata 1 il bordo era **derivabile**: la coppia (chi erige,
cella adiacente bersaglio) lo determina. A portata 3 no — una cella ha sei bordi e la cella bersaglio da sola
non dice quale. Perciò `ARTUnit` guadagna `PlannedCoverEdge` + `bHasPlannedCoverEdge`, con la stessa forma di
`PlannedAttackCell` + `bAttackTargetsCell`: serve un flag perché tutte e sei le direzioni sono legittime e
nessuna può fare da «non dichiarata».

**Non dichiarare il bordo non è un caso da indovinare**: è `Cancel` con la sua voce di TurnLog.

### 2.3 L'operazione su struttura è un dato, non un `ActionId`

`FRTActionDef` guadagna `ERTStructureOp` (`None` · `CreateCover` · `MoveCover`), come già esiste
`ERTMovementStyle` per le cariche.

Senza, il resolver avrebbe **tre** confronti di ActionId per la stessa semantica — l'azione core, l'abilità di
Bastion, il gadget — cioè un ramo per eroe nel core, che il progetto vieta. `Ignite`, `CreateWater` ed
`Electrify` restano riconosciute per ActionId: lì il produttore è uno ciascuno, e il campo si aggiungerà
quando smetteranno di esserlo.

### 2.4 La durata si conta dal turno di nascita

Un pannello da 2 turni eretto nel turno *N* ripara il Blast di *N* **e** quello di *N+1*, e cade nel Cleanup
di *N+1*.

È l'**opposto** del ponte temporaneo di E9.4, che salta il proprio turno di nascita. Le due regole divergono
per la fase in cui nascono, non per svista: il ponte nasce nel **Blast**, a valle della fase Move che lo
userebbe, quindi contargli quel turno gliene toglierebbe uno dei due; il pannello nasce in **Prep**, a monte
del Blast che ripara, quindi il turno dell'erezione è già un turno in cui ha protetto qualcuno.

`DurationTurns = 0` significa **non scade da sola** (pannello adattivo), non «scade subito».

### 2.5 `Reconfigure` rifiuta invece di indovinare

Due casi, entrambi risolti con un rifiuto leggibile invece che con una scelta implicita:

| Situazione | Perché non si indovina |
|---|---|
| La cella porta **più di una** copertura | «la prima dell'array» è un ordine che il giocatore non vede |
| Il bordo di destinazione è **già riparato** | la copertura torna dov'era: la via naturale (togli, poi aggiungi) la farebbe sparire per un'azione nemmeno riuscita |

Il secondo è il più pericoloso dei due, ed è quello coperto dalla verifica di mutazione: disattivando il
ripristino cade esattamente un test, e nessun altro.

### 2.6 La rotazione gratuita, e il primo consumatore delle varianti

Il catalogo eroi dichiarava da tempo due compromessi per il pannello — rinforzato **45/1 turno**, adattivo
**25 + una rotazione gratuita** — nei `Parameters` della variante. **Nessuna variante era letta a runtime, in
tutto il progetto**: il gioco applicava sempre 30/2.

`ARTUnit::ActiveVariantId` è il minimo che li rende consumabili, ed è dichiaratamente il minimo: **chi**
sceglie la variante, quando e con quale interfaccia resta l'epic **E7**. Il vincolo di catalogo — una sola
abilità fondamentale con variante, per eroe — è la ragione per cui basta un id per unità.

La rotazione gratuita non spende il cooldown di `Reconfigure`, viaggia con la copertura quando questa si
sposta, e si consuma una volta sola.

### 2.7 Il gadget, e il confine con E7

`Gadget.PortableCover` è l'**unico** gadget costruito in v0.1 e non è l'inizio del catalogo completo: slot,
loadout e validazione dell'insieme restano `#61`/`#63`. Sta qui perché il DoD del checkpoint lo nomina, e
perché è il **secondo consumatore** di `Action.CreateCover` — la prova che la semantica è condivisa.

Un equipaggiamento dichiara ora quale azione core **concede** (`GrantedActionId`), non una copia dei suoi
numeri. Di suo sostituisce l'identità nel TurnLog (chi legge il replay deve vedere il gadget) e il cooldown.

**Lo svantaggio non è inventato.** Il validator ne esige uno dichiarato, il catalogo equipaggiamento non ne dà
uno specifico per questo gadget, e allora si dichiara quello che i cataloghi già dicono: **cooldown 3** di ogni
gadget contro il **2** del pannello d'eroe, più l'unico slot gadget occupato. Chi non è Bastion può erigere
pannelli, ma più di rado e rinunciando a medkit, isolante o sensore.

## 3. Il TurnLog

Quattro esiti nuovi, tutti aggiunti **in coda** a `ERTEnvironmentOutcome` — viaggia come `uint8` nel formato
serializzato, e inserirli in mezzo rinumererebbe i replay del corpus golden.

| Esito | Quando | `Amount` |
|---|---|---|
| `CoverCreated` | una copertura è stata eretta | turni di durata (0 = permanente) |
| `CoverExpired` | una temporanea è scaduta nel Cleanup | 0 |
| `CoverMoved` | `Reconfigure` l'ha spostata | integrità, che viaggia con lei |
| `CoverRejected` | fuori portata, bordo non dichiarato, bordo occupato, spostamento ambiguo | 0 |

`SrcCell`/`TgtCell` sono le due celle del bordo, come per ogni altro evento di struttura: nessun campo
direzione nel log.

`CoverRejected` esiste perché il `Cancel` del catalogo dev'essere **visibile**: un'azione che sparisce in
silenzio è indistinguibile da un difetto.

## 4. Portata validata — e il debito di `ModifyArc`

`CreateCover` valida la portata dichiarata **prima** di toccare la mappa. È esattamente il difetto che
[`#206`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/206) registra su `Action.ModifyArc`,
che dichiara `Range 3` e opera a qualunque distanza.

Questo checkpoint **non lo corregge** — sarebbe un cambio di regole in un'altra azione — ma toglie a quel
difetto l'argomento «si è sempre fatto così»: da oggi il precedente nel repository è la validazione.

## 5. Limiti dichiarati

- **Nessun targeting per bordo nell'HUD**: cella e bordo si scrivono nel piano, e il produttore è il test o lo
  scenario. Puntare un bordo col mouse è **E11**. È lo stesso limite che il targeting per cella aveva a E8.3.
- **Solo copertura bassa**: nessuna azione del catalogo v0.1 crea copertura **alta** temporanea.
- **`RemoveCover` toglie una faccia sola** — quella creata in partita. Una barriera disegnata a mano su
  entrambe le facce resta dato di mappa e non si smonta da qui.
- **La voce «LOS» del DoD non è verificabile su una copertura bassa.** Il DoD della issue chiede un test
  `ExpiryUpdatesLOS`, ma una copertura bassa non tocca la vista né il grafo — lo ha deciso E9.1, e il catalogo
  terreni dà a `Structure.KineticPanel` gli stessi valori di `Structure.LowCover` (30 / protezione 10). Un test
  con quel nome sarebbe **vacuo**: verificherebbe che nulla cambia. Al suo posto si verifica ciò che cambia
  davvero — la riduzione del danno che compare e sparisce, e la **revisione** che si incrementa in entrambi i
  casi, cioè il canale che invalida cache di vista e percorso.
- **Nessuna verifica PIE**: `Content/` è fuori dal repository e il checkpoint è interamente headless.

## 6. Test

| Test | Che cosa dimostra |
|---|---|
| `Cover.AddCover.RejectsOccupiedEdge` | un bordo regge una copertura sola, sulle **due** facce; il rifiuto non tocca la revisione |
| `Cover.AddCover.RemovedStopsProtecting` | la riduzione la chiede il **consumatore reale** (`CollectHexAttacks`), non un percorso scritto per il test |
| `Structures.KineticPanel.TemporaryCover` | ciclo di vita in un **turno vero**: eretta, regge, scade, non scade due volte |
| `Actions.CreateCover.RejectsOutOfRangeAndOccupied` | la portata dichiarata vale, e il rifiuto è leggibile |
| `Heroes.Bastion.KineticPanelVariantApplied` | la variante decide integrità e durata; `DurationTurns 0` non è «scade subito» |
| `Heroes.Bastion.ReconfigureDoesNotDuplicate` | **conta** le coperture: guardare il bordo d'arrivo non accorgerebbe di una duplicazione |
| `Heroes.Bastion.ReconfigureRefusesInsteadOfGuessing` | ambiguità e destinazione occupata rifiutate, e nulla sparisce |
| `Equipment.PortableCover.CreatesCover` | la semantica è condivisa: se il resolver riconoscesse l'ActionId, sarebbe rosso |
| `Spec.Cover.TemporaryCoverExpires` | scenario-specifica scritto **prima**: da `BLOCKED` a `PASS`, 4 turni |

**Verifica di mutazione**: disattivando il ripristino della copertura nello spostamento fallito cade
esattamente `ReconfigureRefusesInsteadOfGuessing`, e nient'altro.

Suite alla chiusura: **517** (baseline del branch prima di iniziare: **509**, misurata, non copiata).
Scenari: **55/55**.
