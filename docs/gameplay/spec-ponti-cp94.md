# Ponti e `ModifyArc` — CP 9.4

**Epic**: E9 · **Issue**: [#72](https://github.com/DegrassiAaron/refactor-tactics-main/issues/72) ·
**Dipende da**: CP 9.3 ([#71](https://github.com/DegrassiAaron/refactor-tactics-main/issues/71)) ·
**Data**: 2026-08-08

**Obiettivo del checkpoint**: gli archi verticali diventano oggetti di gioco — si creano, si disattivano, si
distruggono — e rompendoli i due layer tornano irraggiungibili: il path **fallisce**, non teletrasporta.

---

## 1. L'arco è additivo, e questo cambia tutto

CP 9.3 ha stabilito che una **porta è un bordo**. La domanda speculare di questo checkpoint è cosa significhi
rompere un **arco**, e la risposta non è simmetrica:

| | Porta (bordo, CP 9.3) | Ponte (arco, CP 9.4) |
|---|---|---|
| Natura | **sottrattiva**: nega un'adiacenza che esiste | **additiva**: crea un collegamento che non esiste |
| Chiusa/rotta, il percorso… | **si allunga**: si gira intorno | **fallisce**: non c'è via alternativa |
| Linea di vista | la blocca | non la tocca (la LOS è planare) |
| Riserva geometrica | i sei vicini planari | **nessuna**: fra due layer non c'è adiacenza |

È la ragione per cui `GraphNeighbors` basta a soddisfare la DoD con una riga: gli archi vivono nel suo secondo
ciclo, e saltare quelli non attivi **è** il «path fallisce, non teletrasporta». Nessun controllo aggiuntivo,
nessun secondo posto da tenere allineato.

---

## 2. Il dato

```cpp
UENUM(BlueprintType)
enum class ERTHexArcState : uint8
{
    Active,    // il collegamento esiste e si percorre
    Inactive,  // spento: non si passa, ma si può riattivare
    Destroyed  // abbattuto: stato TERMINALE
};
```

`FRTHexEdge` guadagna tre campi: `State`, `Integrity` (40 di catalogo) e `bConductsElectricity`.
**Formato v5**, con migrazione v4 → v5 che non converte nulla — ma con una responsabilità precisa: i default
devono essere quelli di un ponte **sano** (`Active`, 40, non conduttivo). Un arco letto da un asset vecchio che
risultasse spento cambierebbe il significato della mappa solo per essere stata ricaricata.

`Inactive` e `Destroyed` sono indistinguibili per il grafo — da nessuno dei due si passa — e differiscono per
la **reversibilità**, esattamente come `Closed` e `Destroyed` per le porte.

### Un ponte è un evento, non due archi

`AddTransition` bidirezionale scrive **due** archi. Ogni operazione li tratta insieme:

- `SetArcState` e `DamageArc` raccolgono entrambi i versi e chiamano `UpdateTransitions` **una volta** → una
  sola revisione;
- comandare il ponte «dal verso opposto» funziona identicamente: chi dice «spegnilo» non deve sapere quale dei
  due versi è stato scritto per primo. Senza questa raccolta si potrebbe spegnere l'andata e lasciare aperto il
  ritorno — un ponte percorribile in un senso solo per errore.

---

## 3. La decisione: `ModifyArc` passa al Blast

**Era un conflitto fra fonti**, segnalato all'autore invece di risolverlo in silenzio.

Il catalogo motivava esplicitamente la fase (`RTCatalogLibrary.cpp`, prima di questo checkpoint):

> *«Fase `Environment` come le altre ambientali: cambiare un arco a metà Blast renderebbe invalido un percorso
> già calcolato in questo stesso turno.»*

La DoD di CP 9.4 lo confermava («effetto topologico nel Cleanup»). Ma CP 9.3 aveva fatto l'**opposto** per le
porte, e per una ragione che era essa stessa una DoD: una porta chiusa a metà turno non deve lasciar passare un
percorso già validato.

**Il fatto che ha sciolto il conflitto**: `TruncatePathToTopology` (CP 9.3) ha tolto la ragione originale del
Cleanup. Un percorso invalidato a metà turno non produce più un fantasma — viene troncato con un reason code
(`BlockedByTopology`). La motivazione scritta nel catalogo era valida quando è stata scritta e non lo era più.

**Decisione (2026-08-08, con l'autore)**: `Action.ModifyArc` risolve in `ERTResolutionPhase::Attack` (macro-fase
**Blast**), dove `Attack` copre «attacchi, abilità, cure, **interazioni**». Porte, muri e ponti cambiano tutti a
fase conclusa e il Move che segue li vede. Due tempi diversi per due oggetti topologici sarebbero una regola che
nessun giocatore può dedurre guardando il campo.

Il commento del catalogo è stato riscritto: lasciarlo avrebbe significato una fonte che contraddice il codice.

### Il difetto che il cambio di fase nascondeva

Spostare la fase **e basta** avrebbe trasformato `ModifyArc` in un'azione che infligge danno. La raccolta degli
intenti legge il danno dagli effetti dichiarati e, non trovandone, **ripiega sul campo legacy `Ability->Power`**
(`RTTurnManager.cpp`, raccolta del Blast). Un'azione che cambia la topologia si sarebbe messa a colpire.

Rimedio: `ModifyArc` viene **intercettata prima** della raccolta degli intenti, quindi non diventa mai un
intento d'attacco. Consuma l'abilità, registra l'operazione, e l'applicazione avviene a fase conclusa insieme a
porte e strutture.

---

## 4. Ponte temporaneo e conduttivo

La DoD chiede che `ModifyArc` possa «creare un ponte temporaneo e renderlo conduttivo». Nessuna delle due cose
aveva un significato eseguibile prima di questo checkpoint.

**Temporaneo.** La durata è **stato di partita**, non dato di mappa: vive in `ARTTurnManager::DynamicArcs`
accanto a `DynamicSurfaces`, con la stessa divisione — l'arco corrente sta nella mappa (è ciò che il grafo legge
già), la scadenza sta nel TurnManager, perché due partite sulla stessa arena non devono ereditarsi i ponti a
vicenda. Durata **2 turni**, la stessa delle altre modifiche ambientali del catalogo (`Ignite`, `CreateWater`).

> ⚠️ **Un difetto trovato scrivendo il test.** Il ponte nasce nel **Blast**, ma il tick di scadenza gira nel
> **Cleanup dello stesso turno**: un ponte da due turni ne perdeva uno prima che qualcuno potesse
> attraversarlo. Le superfici dinamiche non hanno il problema perché nascono *nel* Cleanup, dopo il proprio
> tick. Risolto con un `CreatedOnTurn` esplicito — non con un `+1` magico, che avrebbe funzionato senza
> spiegare niente. Lo ha scoperto `Structures.Bridge.TemporaryBridgeExpires`, che verifica **tre** turni
> proprio per questo.

> ⚠️ **Un secondo difetto, trovato dalla code review di #292 e corretto il 2026-08-09**
> ([#302](https://github.com/DegrassiAaron/refactor-tactics-main/issues/302)). Un ponte temporaneo **distrutto
> in combattimento** lasciava la propria entry in `DynamicArcs`: a toglierlo dalla mappa è `DamageArc`, che di
> quella lista non sa nulla, mentre `ModifyArc` la ripuliva già. L'asimmetria non era innocua, perché
> `DamageArc` **non rimuove** l'arco — lo marca `Destroyed` con integrità 0 e lo lascia dov'è. Quindi alla
> scadenza del timer del fantasma `RemoveTransition` *riusciva*, e faceva due danni in uno: scriveva un
> `BridgeRemoved` per un crollo avvenuto due turni prima, e si portava via le macerie. Corretto alla radice —
> l'entry muore quando muore il ponte — con la stessa disciplina applicata alle coperture in #301.
>
> **Regola che ne discende, e che prima non era scritta da nessuna parte: le macerie restano.** Un arco
> `Destroyed` sopravvive sulla mappa finché un `ModifyArc` non lo toglie; nessun timer se ne occupa, perché
> nessun timer lo ha creato. È ciò che rende eseguibile il «terminale» di §5: `SetArcState` rifiuta di
> riattivare un ponte abbattuto, e per rifiutarlo deve averlo ancora davanti.

**Conduttivo.** `CollectElectricPropagation` camminava su `URTHexLibrary::Neighbors`, cioè i soli sei vicini
planari: l'elettricità non attraversava **nessun** arco e non saliva mai di layer. Il BFS ora aggiunge anche gli
archi uscenti che dichiarano la conduttività. Un ponte conduttivo diventa così un **rischio** oltre che una
scorciatoia: una scarica al piano terra raggiunge chi sta sopra.

Un ponte **spento** non conduce, per quanto dichiari la conduttività: non c'è collegamento da risalire.

---

## 5. Il danno: l'arco identificato dalla coppia

Il ponte ha integrità 40, ma **non esiste un modo di mirare a un arco**: il targeting è per unità
(`PlannedAttackTarget`) e la linea di vista è planare sul layer del tiratore, quindi un ponte fra layer non sta
mai sulla linea di tiro.

**Decisione**: l'arco è identificato dalla coppia (cella di chi agisce, cella del bersaglio) — la stessa
convenzione che `Action.ModifyArc` usa dal CP 8.5. Un'azione che dichiara `DamageStructure` e mira a un'unità
all'altro estremo di un ponte lo scalfisce; se fra le due celle non c'è un arco, non succede nulla e la
revisione non si muove.

Il danno si applica a **fase conclusa**, come per le coperture: l'ordine dei colpi non cambia l'esito.

---

## 6. TurnLog

Quattro esiti aggiunti **in coda** a `ERTEnvironmentOutcome`, così le tracce già scritte non cambiano
significato. `SrcCell`/`TgtCell` sono le due celle collegate — nessun campo nuovo:

- `BridgeCreated` — `Amount` = i turni di durata (0 = permanente);
- `BridgeRemoved` — tolto da `ModifyArc` o scaduto;
- `BridgeDamaged` — `Amount` = integrità **residua**;
- `BridgeDestroyed` — spiega perché un percorso fra due layer non esiste più.

---

## 7. Limiti dichiarati

1. **`ModifyArc` non valida la portata.** Il catalogo le dà portata 3, ma il ramo che la esegue applica
   l'operazione senza verificarlo. **Non è una regressione** — era così anche quando risolveva nel Cleanup — ma
   ora che l'azione è nel Blast è più visibile. Issue [#206](https://github.com/DegrassiAaron/refactor-tactics-main/issues/206)
   invece di allargare lo scope di questo checkpoint.
2. **Nessuna mappa `.uasset` disegna ancora ponti con stato.** Il dato esiste e i consumatori runtime sono
   testati; lo strumento per disegnarli arriva con E9/E11.
3. **Il ponte creato da `ModifyArc` ha durata e conduttività fisse** (2 turni, sempre conduttivo). Renderle
   parametri dell'azione richiede un campo nel catalogo che oggi servirebbe a una sola azione.
4. **`Inactive` non ha un vettore in partita.** `SetArcState` lo espone e i test lo esercitano, ma nessuna
   azione lo usa: `ModifyArc` rimuove l'arco, il danno lo distrugge. Il ponte «disattivabile e riattivabile» è
   materiale per CP 10.1 (`Activate`/`Interact`).
5. **Nessun `Flux.ConductiveNode`.** Il suo commento nel catalogo eroi dichiara «nessun modello di conduttività
   di cella esiste» ed è **obsoleto** da CP 8.4 (terreno dinamico): quel modello ora c'è. Fuori scope, issue
   [#207](https://github.com/DegrassiAaron/refactor-tactics-main/issues/207) — non una correzione di iniziativa.

---

## 8. Verifica

### Test automatici

| Test | Cosa dimostra |
|---|---|
| `Structures.Bridge.RemovalBreaksPath` | tolto il ponte il percorso **fallisce** (`NoPath`, `Path` vuoto), e la cella oltre esiste ancora: manca il collegamento, non la destinazione |
| `Structures.Bridge.NoTeleportOnRemoval` | **turno vero**: il ponte cade nel Blast e chi lo attraversava resta dov'era |
| `Structures.Bridge.StateChangeBumpsRevision` | due archi, **una** revisione; comandabile dai due versi; riapplicare lo stesso stato non muove nulla |
| `Structures.Bridge.DestroyedIsTerminal` | un ponte abbattuto non si riattiva |
| `Structures.Bridge.DamageBreaksAtZero` | integrità 40 scalata fino al crollo, mai negativa; un ponte già crollato non incassa |
| `Structures.Bridge.ConductsElectricity` | la scarica risale il ponte conduttivo e **non** quello spento né quello non conduttivo |
| `Structures.Bridge.TemporaryBridgeExpires` | tre turni: nasce, regge, scade — ed è conduttivo |
| `HexMap.ArcHashDeterminism` | stato, integrità e conduttività entrano nell'hash |
| `HexMap.ArcValidation` | un arco attivo a zero punti struttura è un dato incoerente |
| `HexMap.ArcFormatMigration` | v4 → v5 non perde celle, coperture, porte né transizioni, e i default sono quelli di un ponte sano |
| `Structures.Bridge.DamagedInPlayedTurn` | **turno vero**: `DamageStructure` raggiunge davvero l'arco, su entrambi i versi (aggiunto dopo la verifica di mutazione) |
| `Structures.Bridge.DestroyedBridgeLeavesNoGhost` | **turno vero, tre turni**: un ponte temporaneo abbattuto in combattimento non lascia un'entry in `DynamicArcs` — alla data della sua scadenza le macerie sono ancora lì e il TurnLog non porta un `BridgeRemoved` mai avvenuto (#302, scritto **rosso prima** del fix) |

Aggiornato **consapevolmente**, non fatto passare: `Actions.EnvironmentalSetMatchesCatalog` non elenca più
`ModifyArc` fra le ambientali, e un blocco nuovo verifica che risolva nel **Blast**, col perché. Il cambio di
fase è inchiodato da un test invece di essere un effetto collaterale silenzioso.

### Verifica manuale

`PIE-HEXPLAY-8` — movimento via arco su due layer. Vedi
[`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md).

---

## 9. Verifica di mutazione

Ogni riga è una modifica deliberata al codice di produzione, applicata **da sola**, con build completa e suite
intera. Nessuna usa un `return` anticipato: in questo progetto il codice non eseguibile (C4702) è un errore di
compilazione, quindi si muta un valore o una condizione già presenti.

Baseline: **443 test, nessuno fallito**. Lo script confronta `Test Completed` con il totale misurato
staticamente e marca la run come *non valida* se non coincide — la lezione dell'incidente di CP 9.3, dove una
run parziale (298 su 425) sembrava un'attribuzione buona.

| # | Mutazione | File | Test caduti |
|---|---|---|---|
| 1 | Il grafo ignora lo stato dell'arco | `RTHexPathLibrary.cpp` | `Bridge.{RemovalBreaksPath, DestroyedIsTerminal, DamageBreaksAtZero}` |
| 2 | `Destroyed` non è più terminale | `RTHexArcLibrary.cpp` | `Bridge.DestroyedIsTerminal` |
| 3 | Si guarda un verso solo del ponte | `RTHexArcLibrary.cpp` | `Bridge.{StateChangeBumpsRevision, DamageBreaksAtZero}` |
| 4 | Una revisione per arco invece che per ponte | `RTHexMapAsset.cpp` | `Bridge.StateChangeBumpsRevision` |
| 5 | L'integrità a zero non abbatte il ponte | `RTHexArcLibrary.cpp` | `Bridge.DamageBreaksAtZero` |
| 6 | L'elettricità sceglie gli archi sbagliati | `RTTerrainLibrary.cpp` | `Bridge.ConductsElectricity` |
| 7 | Un ponte spento conduce comunque | `RTHexArcLibrary.cpp` | `Bridge.ConductsElectricity` |
| 8 | Lo stato dell'arco esce dall'hash | `RTHexMapAsset.cpp` | `HexMap.ArcHashDeterminism` |
| 9 | Un arco a zero punti ancora attivo non è segnalato | `RTHexMapAsset.cpp` | `HexMap.ArcValidation` |
| 10 | Il tick mangia un turno al ponte appena nato | `RTTurnManager.cpp` | `Bridge.TemporaryBridgeExpires` |
| 11 | Il ponte creato non è conduttivo | `RTTurnManager.cpp` | `Bridge.TemporaryBridgeExpires` |
| 12 | `ModifyArc` torna a risolvere dopo il Move | `RTCatalogLibrary.cpp` | `Actions.EnvironmentalSetMatchesCatalog`, `Actions.ModifyArc.BumpsChunkRevision`, `Bridge.{NoTeleportOnRemoval, TemporaryBridgeExpires}` |
| 13 | Il danno alle strutture non raggiunge gli archi | `RTTurnManager.cpp` | `Bridge.DamagedInPlayedTurn` ⚠️ *(vedi sotto)* |

### Cosa ha trovato

**La mutazione 13 non uccideva nulla.** Disattivando la raccolta del danno verso gli archi nel `TurnManager`,
tutti e dieci i test di CP 9.4 passavano: `DamageBreaksAtZero` chiama `URTHexArcLibrary::DamageArc`
**direttamente**, quindi la libreria era coperta e il *cablaggio* no. È il difetto ricorrente di questo
repository nella sua forma classica — codice corretto che nessuno chiama — e la stessa famiglia della
`ReadsBothFaces` di CP 9.3, dove invece mancava la verifica di una decisione.

`Structures.Bridge.DamagedInPlayedTurn` colma il buco su un **turno vero**: un'azione che dichiara
`DamageStructure` mira a un'unità all'altro estremo del ponte, e il ponte incassa — su **entrambi** i versi,
con le due voci nel TurnLog. Con lui la mutazione muore.

**La mutazione 12 è la più informativa del gruppo.** Rimettere `ModifyArc` nel Cleanup fa cadere quattro test
di tre famiglie diverse: il catalogo (`EnvironmentalSetMatchesCatalog`), l'azione (`ModifyArc.BumpsChunkRevision`)
e due comportamenti di gioco. Il cambio di fase è quindi inchiodato da più punti, non da un'asserzione sola.

