# Interazioni con la mappa — la grammatica di CP 10.1

> `CURRENT` · **Stato**: owner documentale della **grammatica** delle interazioni ambientali.
> **Checkpoint**: **E10 · CP 10.1**, oggi la voce con la DoD più corta dell'epic. **Release**: v0.1.
> **Provenienza**: consolidamento di `RT_Map_Environment_Master_Consolidation_v0.1.md` §14–§24 e
> `RefactorTactics_Interactive_Map_Elements_Claude_Consolidation.md` §2–§13, filtrati contro il canone —
> triage in [`../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md`](../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).
> **Non introduce numeri**: costi, portate e durate restano dei cataloghi.

---

## 1. Il problema: i pezzi ci sono, la grammatica no

L'epic **E9** è chiusa. Porte ([`spec-porte-cp93.md`](spec-porte-cp93.md)), ponti
([`spec-ponti-cp94.md`](spec-ponti-cp94.md)), coperture ([`spec-copertura-cp91.md`](spec-copertura-cp91.md),
[`spec-copertura-alta-cp92.md`](spec-copertura-alta-cp92.md),
[`spec-coperture-temporanee-cp95.md`](spec-coperture-temporanee-cp95.md)) esistono, hanno stato, mutano il
grafo, incrementano la revisione e finiscono nel TurnLog. Ognuno ha il suo spec e i suoi test.

Manca la domanda che li attraversa tutti, ed è quella che CP 10.1 deve saper risolvere:

> **Questa unità, davanti a questo elemento, in questo stato del mondo, che cosa può fare — e perché no?**

Oggi CP 10.1 la elude elencando gli oggetti: «porta, consolle, ascensore, generatore, sprinkler, ponte,
obiettivo». Un elenco di oggetti non è un modello: cresce a ogni contenuto nuovo, e la prima cosa che chiede
è un `switch` sul tipo di Actor.

Questo documento non aggiunge oggetti. Aggiunge la **regola** che li rende tutti lo stesso problema.

---

## 2. La decisione: l'elemento dichiara i verbi, l'unità dichiara le capability

Non si modella così:

```text
Door      -> Interact
Generator -> Interact
Valve     -> Interact
```

e nemmeno così:

```text
ThisDoorCanBeUsedByFlux = true
```

Il modello è:

```text
Map Element
 ├─ State                 in che stato è adesso
 ├─ Interaction Verbs     che cosa si può chiedergli, in astratto
 ├─ Requirements          che cosa serve per ciascun verbo
 └─ Effects               che cosa succede quando riesce
```

Un'interazione legale è una **coppia `(Elemento, Verbo)`** che sopravvive a tre filtri indipendenti (§4). La
query è una sola funzione, e non conosce né porte né generatori: legge il dato dell'elemento e lo confronta
con quello dell'unità.

**Conseguenza pratica**: la stessa porta produce gameplay diverso per unità diverse senza che nessuno scriva
un ramo. È la stessa forma che CP 9.5 ha già dato alle strutture, dove l'operazione è un dato
(`ERTStructureOp`) e non tre `if` sull'`ActionId`.

---

## 3. Perché non il nome dell'eroe

[`ADR-0006`](../decisions/adr-0006-ownership-abilita-sinergie.md) e
[`D-029`](../decisions/RT_PDR_00_Decision_Log.md) hanno già deciso la forma: le abilità hanno **ownership
singola**, le sinergie sono **derivate**, e non esiste dipendenza hard-coded fra `HeroId` per un payoff
esprimibile sistemicamente.

Un requisito `CanUseOnlyCharacter = Flux` è esattamente ciò che quella decisione vieta, e il costo si paga
due volte: il roster cresce e la mappa va riscritta; la mappa cresce e il roster va riscritto.

Il requisito si esprime quindi come **capability**, cioè un tag governato che l'unità porta e l'elemento
richiede. Il kit d'origine propone questo primo elenco:

```text
Interaction.Tech          Interaction.Force        Interaction.Sensor
Interaction.Engineering   Interaction.Fluid        Interaction.Security
Interaction.Electric      Interaction.Precision    Interaction.Mechanical
Interaction.Remote        Interaction.Demolition
```

> ⚠️ **`PROPOSED`, non canone.** Undici capability sono più di quante ne serva la v0.1, e il pacchetto
> d'origine le presenta come «esempi da verificare». Entrano quelle che un elemento reale della showcase
> richiede davvero; le altre restano nel documento come vocabolario, senza dato.

E il mapping per eroe resta **fuori** dal canone:

> ⚠️ **`INTERACTION-CAPABILITY-01`.** Le assegnazioni discusse — Gadget → `Electric`/`Tech`, Phase → `Fluid`,
> Riktor → `Engineering`/`Force`, Wraith → `Precision`/`Sensor` — sono **coerenti** con le identità già
> canoniche, e proprio per questo è facile scambiarle per decise. Non lo sono: nessuna compare nel
> [catalogo eroi](../balance/RT_HeroCatalog_v0.1.md) né nel Decision Log. Vanno decise con l'autore prima di
> diventare dato, perché sono un **asse di bilanciamento**: assegnare `Force` a un solo eroe rende quel
> personaggio obbligatorio davanti a ogni porta rinforzata (§9).

---

## 4. Le tre dimensioni dell'accesso

I tre filtri sono indipendenti e vanno tenuti separati. Confonderli è il difetto che questo documento esiste
per prevenire: sono tre domande diverse che restituiscono lo stesso «no».

| Dimensione | Domanda | Esempio di rifiuto |
|---|---|---|
| **Unit capability** | l'unità *sa* farlo? | `MissingCapability` |
| **Team / ownership** | l'unità *ha diritto* di farlo? | `NotOwner` |
| **World state** | l'elemento *può* riceverlo adesso? | `WrongState` |

Ownership e world state hanno vocabolari propri:

```text
Ownership   Neutral · TeamA · TeamB · Contested · Locked · Disabled
World state dichiarato dall'elemento: vedi §5
```

Alle tre si aggiungono i filtri che il progetto ha già e che **non vanno riscritti qui**: adiacenza e portata
(CP 10.1 dice *adiacente*), fase, slot d'azione, costo. La query di legalità li interroga, non li ridefinisce.

---

## 5. L'elemento è una macchina a stati

Ogni elemento rilevante dichiara i propri stati e le transizioni legali. Esempio, non catalogo:

```text
Generator
  Off         ├─ Start        -> Online
              └─ Sabotage     -> Damaged
  Online      ├─ Shutdown     -> Off
              ├─ Overload     -> Overloaded
              └─ Destroy      -> Destroyed
  Overloaded  ├─ Stabilize    -> Online
              ├─ Disconnect   -> Off
              └─ Failure      -> Destroyed
  Damaged     ├─ Repair       -> Off
              └─ Destroy      -> Destroyed
```

Le transizioni devono essere **deterministiche, serializzabili, validabili, loggabili e ricostruibili dallo
snapshot** — gli stessi requisiti che la porta soddisfa già dal CP 9.3, dove lo stato viaggia in `Amount` come
intero e uno stato fuori intervallo semplicemente non produce operazione.

**Simultaneità.** Due unità che nello stesso turno chiedono transizioni incompatibili sullo stesso elemento
devono dare lo **stesso esito in qualunque ordine**. La regola esiste già ed è quella delle porte: a parità di
bersaglio vince lo **stato più restrittivo** (invariante **#3**). Non se ne inventa una seconda.

---

## 6. `Interact` resta una sola azione

Questo è il punto in cui è più facile sbagliare, e il pacchetto d'origine sbaglia (vedi §12).

- [`D-025`](../decisions/RT_PDR_00_Decision_Log.md): le azioni generiche sono **sette**, e `Activate` è
  **assorbita** da `Interact`. «Attivare un dispositivo» *è* un'interazione: il bersaglio cambia, il gesto no.
- [`D-033`](../decisions/RT_PDR_00_Decision_Log.md): il modificatore di un'azione generica si chiama
  **`profilo`**, vale per tutte e sette ed è fisso per eroe in v0.1.

I due si compongono senza nomi nuovi:

```text
Action.Interact          l'azione generica — una, universale
  + verbo                payload dell'ELEMENTO   (Open · Override · ForceOpen · …)
  + capability           profilo dell'EROE       (ciò che quell'unità sa fare)
```

Il verbo **non** è un'azione del catalogo e non consuma uno slot suo: è il modo in cui un `Interact`
già speso si specializza. Aggiungere un verbo a un elemento non aggiunge un'azione al gioco — che è
esattamente la proprietà che rende il catalogo degli elementi estendibile senza toccare l'action economy.

---

## 7. La fase — e una correzione a CP 10.1

CP 10.1 dichiara oggi: «effetto risolto nel **Blast**, conseguenze topologiche nel **Cleanup**».

La prima metà è corretta e coerente col catalogo: `Action.Interact` è **Blast**
([`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md)).

**La seconda metà è superata dal 2026-08-08.** Da CP 9.3 la mutazione topologica avviene **dentro il Blast**:
`SetDoorState` è un effetto dichiarato, raccolto nel Blast e applicato a fase conclusa, e
[`spec-porte-cp93.md`](spec-porte-cp93.md) §5 spiega perché non poteva essere altrimenti — `ResolveEnvironment`
gira nel Cleanup, cioè **dopo** il Move, e una porta che si chiude lì non potrebbe mai fermare un movimento.
CP 9.4 ha poi spostato `Action.ModifyArc` dal Cleanup al Blast per la stessa ragione: «porte, muri e ponti
cambiano tutti nello stesso momento».

Una topologia che cambia nel Cleanup e un movimento che risolve nel Move sono incompatibili: **il path
fantasma nasce esattamente lì**, ed è ciò che `TruncatePathToTopology` esiste per impedire.

> **Azione**: la DoD di CP 10.1 va corretta in «conseguenze topologiche **nel Blast**, con incremento della
> revisione e invalidazione della cache». Non è un cambio di design: è l'allineamento di una riga scritta
> prima che E9 decidesse.

### Quel che resta nel Cleanup

Le conseguenze **ambientali** — superfici create, propagazione, scadenze — continuano a risolvere in
`ResolveEnvironment`. La distinzione è netta e va tenuta: *la topologia cambia nel Blast, l'ambiente si
propaga nel Cleanup*.

### Fasi diverse per verbi diversi: registrato, non deciso

Il pacchetto propone `OpenDoor → Prep`, `BreachDoor → Dash`, `Overload → Blast`, `CrossDoor → Move`.

Non si applica, e il motivo è concreto: **una porta aperta in Prep è attraversabile con il Dash**, mentre una
aperta nel Blast è attraversabile solo con il Move. Sono due giochi diversi — il secondo costa un turno intero
al piano di sfondamento, il primo no. È una scelta di design con conseguenze sull'economia del turno, non un
dettaglio di implementazione, e va decisa con un dato in mano. Registrata in §12.

---

## 8. Il rifiuto è informazione, ma non deve perdere informazione

La UI deve spiegare **perché** un verbo non è disponibile: un pulsante grigio senza motivo è un puzzle, non
una scelta tattica. I reason code candidati:

```text
MissingCapability · WrongState · OutOfRange · NotOwner
Blocked · Disabled · Destroyed · InsufficientResource · WrongPhase
```

Con un vincolo che vale più dell'elenco: **nessun reason code può rivelare informazione che il Team Knowledge
non possiede**. Se un elemento è controllato da una consolle che la squadra non ha osservato, il rifiuto dice
`Blocked`, non «controllato da S1». La regola generale è già canonica — invariante **#6**, e
[`D-046`](../decisions/RT_PDR_00_Decision_Log.md) ha appena confermato che la conoscenza è **di squadra** —
ma i reason code sono un canale nuovo e vanno costruiti sapendolo.

Corollario, dallo stesso principio: **il collegamento sorgente → bersaglio non si replica per poi nasconderlo
nel widget.** «Nascondere il widget non è sicurezza» vale qui come nel planning.

---

## 9. La regola delle tre soluzioni

Guideline di level design, non regola di runtime:

> Un'affordance tatticamente importante dovrebbe offrire almeno **tre** modi sensati di essere affrontata.

Esempio, una porta rinforzata: aprirla legittimamente · forzarla con una capability · aggirarla. La quarta via
— la combo di squadra — è un bonus, non un requisito.

**Lo scopo non è la varietà, è evitare il personaggio obbligatorio.** Se un solo eroe del roster porta
`Interaction.Force` e la mappa mette una porta rinforzata sull'unica rotta buona, quell'eroe non è una scelta:
è una tassa. La regola delle tre soluzioni è il contrappeso di §3 — la capability rende il contenuto
componibile *a patto* che il livello non la renda obbligatoria.

Le eccezioni sono ammesse e vanno **dichiarate**: un'affordance a soluzione unica può essere il punto di un
livello, se qualcuno lo ha scelto.

---

## 10. Che cosa esiste già, e non si ricostruisce

| Serve | Esiste | Owner |
|---|---|---|
| Stato di una porta, gruppo di bordi, revisione | ✅ CP 9.3 | [`spec-porte-cp93.md`](spec-porte-cp93.md) |
| Arco attivabile/distruttibile fra layer | ✅ CP 9.4 | [`spec-ponti-cp94.md`](spec-ponti-cp94.md) |
| Copertura direzionale, distruzione, creazione | ✅ CP 9.1/9.2/9.5 | i tre spec copertura |
| Mutazione del grafo + invalidazione cache | ✅ CP 9.3 | `TruncatePathToTopology` |
| Effetto dichiarato applicato a fase conclusa | ✅ CP 9.2/9.3 | `DamageStructure`, `SetDoorState` |
| Operazione su struttura come **dato** | ✅ CP 9.5 | `ERTStructureOp` |
| Propagazione su grafo conduttivo | ✅ CP 8.3 | [`spec-propagazione-elettrica-cp83.md`](spec-propagazione-elettrica-cp83.md) |
| Rumore come secondo canale | 🟡 `SPECIFIED`, E13 | [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) |

**Regola di recepimento**: nessun tipo nuovo prima di aver cercato quello esistente. Il kit propone
`URTMapElementDefinition`, `URTInteractionDefinition`, `URTMapElementCatalog` — e dice esso stesso «prima
cercare, poi consolidare, poi estendere». Il progetto ha già `URTActionData`/`URTHeroData` come
`UPrimaryDataAsset` e un catalogo con validator: la definizione di elemento è un'estensione di quella
pipeline, non una seconda.

---

## 11. Fuori scope dichiarato

Il catalogo del pacchetto elenca otto famiglie e una sessantina di elementi (turret, drone station, radar,
sprinkler, mine controller…). **Un catalogo ampio non è uno scope.**

In v0.1 entrano soltanto gli elementi che la showcase «Il Relè» richiede davvero, e ci entrano perché E9 li ha
già costruiti. Restano fuori, esplicitamente:

- torrette, droni, trappole e ogni **Tactical Device** — sono produttori di reazioni, quindi dipendono da E14;
- telecamere, sensori e radar — sono produttori di conoscenza, quindi dipendono da E13;
- il controllo **remoto** sorgente → bersaglio, che richiede la privacy dei collegamenti (§8) e quindi la rete;
- valvole, pompe e fluidodinamica: l'acqua ha un produttore nel roster ([`D-046`](../decisions/RT_PDR_00_Decision_Log.md),
  `Riva.FluidTrail` **è** `Action.CreateWater`) e non serve un secondo modello per crearla;
- ascensori e piattaforme mobili, che sono transizioni con stato temporale, non elementi con verbi.

Il criterio non è la difficoltà: è che **ognuno di questi dipende da un sistema che non è ancora verde**.
Costruirli ora significherebbe scriverne la metà due volte.

---

## 12. Domande registrate

Nessuna è decidibile dai documenti. Vanno chiuse prima che E10 le incontri in codice.

**Vivono in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md)**, che è il posto delle cose che aspettano una
persona — non qui. Questa sezione dice *quali sono* e *da quale paragrafo nascono*; il testo della domanda e
il motivo per cui non si risolve leggendo stanno lì, una volta sola.

| ID | Nasce da | In una riga |
|---|---|---|
| `INT-1` | §3 | quali capability di interazione porta ciascun eroe della v0.1 |
| `INT-2` | §7 | se un verbo può risolvere in una fase diversa dal Blast |
| `INT-4` | §6 | se il costo di un `Interact` dipende dal verbo |

**`INT-3` non esiste**, ed è deliberato: «`Interact` richiede o impone il facing?» era già `FAC-6` dal
2026-08-08, il suo owner è [ADR-0005](../decisions/adr-0005-orientamento.md), e questo documento ne è il primo
consumatore concreto — non il proprietario. Due ID per la stessa domanda si chiudono in momenti diversi, e il
secondo resta aperto a mentire.

---

## 13. Verifica

Test proposti per la DoD di CP 10.1, in aggiunta ai due già nominati dalla roadmap
(`Objectives.ActivateAdjacentOnly`, `Objectives.ActivateDoorChangesGraph`):

| Test | Che cosa dimostra |
|---|---|
| `Interaction.CapabilityQueryFiltersVerbs` | due unità con capability diverse ricevono **elenchi diversi** sullo stesso elemento |
| `Interaction.MissingCapabilityIsRejected` | il verbo senza requisito soddisfatto non risolve, e il rifiuto ha un reason code |
| `Interaction.WrongStateIsRejected` | `Repair` su un elemento integro non è legale |
| `Interaction.SimultaneousOpsAreOrderIndependent` | due richieste incompatibili nello stesso turno danno lo stesso esito in qualunque ordine (invariante #3) |
| `Interaction.TopologyChangesInBlast` | la mutazione avviene nel Blast e la revisione si incrementa **prima** del Move (§7) |
| `Interaction.ReasonCodeLeaksNothing` | un rifiuto non contiene informazione assente dal Team Knowledge (§8) |

I nomi sono **proposti**, non misurati: nessuno di questi test esiste oggi. Vanno confrontati con la
convenzione reale al momento di scriverli.

---

## 14. Che cosa questo documento **non** possiede

| Tema | Owner |
|---|---|
| Modello spaziale, celle, transizioni fra layer | [`../technical/spec-mappa-multilivello.md`](../technical/spec-mappa-multilivello.md) |
| Porte, ponti, coperture: dato, stato e test | i cinque spec di E9 |
| Superfici, stati temporanei, propagazione | [`spec-terreni-e8.md`](spec-terreni-e8.md) · [`spec-propagazione-elettrica-cp83.md`](spec-propagazione-elettrica-cp83.md) |
| Tassonomia delle azioni generiche | [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) · [`D-025`](../decisions/RT_PDR_00_Decision_Log.md) |
| Conoscenza di squadra e rumore | [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) |
| Finestre di reazione innescate da interazioni | [`../decisions/adr-0004-finestre-di-reazione.md`](../decisions/adr-0004-finestre-di-reazione.md) · E14 |
| Stato di avanzamento | [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml) |
