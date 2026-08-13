# Porte e revisione del grafo — CP 9.3

**Epic**: E9 · **Issue**: [#71](https://github.com/DegrassiAaron/refactor-tactics-main/issues/71) ·
**Dipende da**: CP 9.1 (#69), CP 9.2 (#70) · **Data**: 2026-08-08

**Obiettivo del checkpoint**: la topologia cambia a metà turno senza mai produrre path fantasma.

---

## 1. La decisione: una porta è un **bordo**, non un arco

Il codice offriva due sedi già esistenti per «cosa c'è fra queste due celle»:

| | Bordo (`FRTHexCover` su `FRTHexCellData`) | Arco (`FRTHexEdge` in `Transitions`) |
|---|---|---|
| Natura | **sottrattiva**: nega un'adiacenza che esiste | **additiva**: crea un collegamento che non esiste |
| Letto da | vista, pathfinding e combat, tramite `URTHexCoverLibrary` | solo il pathfinding (`GraphNeighbors`, secondo ciclo) |
| Linea di vista | sì — `RTHexVisionLibrary.cpp:27` | **mai**: `HasLineOfSight` non guarda `Transitions` |
| Checkpoint proprietario | CP 9.1 / 9.2 (coperture) | CP 9.4 (ponti e transizioni fra layer) |

**Decisione (2026-08-08, con l'autore)**: la porta è un **bordo fra due celle adiacenti**, modellata come le
coperture e letta dallo stesso punto.

Le ragioni, in ordine di peso:

1. **La DoD chiede che una porta chiusa blocchi la linea di vista.** La LOS interroga soltanto
   `URTHexCoverLibrary::BlocksTraversal`. Con la porta sul bordo la DoD è soddisfatta senza toccare
   `HasLineOfSight`; con la porta sull'arco servirebbe un secondo meccanismo dentro la vista — due punti da
   tenere allineati, che è esattamente ciò che l'epic E9 esiste per evitare.
2. **Un arco fra celle adiacenti non nega nulla.** `GraphNeighbors` (`RTHexPathLibrary.cpp:19-28`) aggiunge i
   sei vicini planari **prima** e indipendentemente dagli archi. Due celle adiacenti restano collegate anche
   togliendo l'arco: una porta-arco *chiusa* lascerebbe il passaggio aperto. Il codice lo dichiara già —
   `ValidateMap` emette un **Warning** per «transizione ridondante tra celle già adiacenti»
   (`RTHexMapAsset.cpp:295-299`).
3. **Le tre aree si aggiornano insieme.** Vista, grafo e combat consultano già `BlocksTraversal`. Estendendo
   quella funzione la porta entra in tutte e tre nello stesso commit, e nessuna può divergere dalle altre.

### Alternative scartate

- **Arco `FRTHexEdge` con stato.** Richiederebbe di inventare un arco *negativo* (che sopprima l'adiacenza
  planare) e un controllo archi dentro `HasLineOfSight`. Cambierebbe il significato di `Transitions`, che oggi
  è additivo per definizione. Gli archi restano la sede giusta per ciò che CP 9.4 gli assegna: ponti e
  transizioni fra layer, dove è **la presenza** dell'arco a creare il collegamento.
- **Porta come cella** (`bBlocksMovement` + `bBlocksLineOfSight` sulla cella-soglia). Gratis in lettura — i due
  flag sono già rispettati da grafo e vista — ma una cella si può **occupare**: bisognerebbe decidere cosa
  succede all'unità ferma sulla soglia quando il portone si chiude (spinta? danno? chiusura rifiutata?).
  Nessuna delle tre è chiesta dalla DoD, e CP 9.2 ha deliberatamente evitato la domanda: la copertura alta nega
  il passo **senza** togliere celle («restano entrambe occupabili», `RTHexPathLibrary.cpp:17-18`). In più il
  gioco avrebbe due modelli di barriera: il muro sui bordi, la porta sulle celle.

---

## 2. Il dato

```cpp
UENUM(BlueprintType)
enum class ERTHexDoorState : uint8
{
    Open,       // si passa e si vede
    Closed,     // nega passo e vista; riapribile
    Locked,     // come Closed, ma `SetDoorState` non la apre (serve l'apertura autorizzata di CP 10.1)
    Destroyed   // aperta per sempre: stato TERMINALE, nessuna transizione ne esce
};

USTRUCT(BlueprintType)
struct FRTHexDoor
{
    ERTHexDirection  Edge;                    // bordo, visto DALLA cella (stessa convenzione di FRTHexCover)
    ERTHexDoorState  State  = ERTHexDoorState::Closed;
    int32            DoorId = INDEX_NONE;     // gruppo; INDEX_NONE = porta singola
};
```

`FRTHexCellData::Doors` è un array **sparso**, come `Covers`: le celle senza porte — la quasi totalità di una
mappa — non pagano nulla, e l'hash di una mappa senza porte è identico a quello di prima.

**Formato v4.** `MigrateToCurrentFormat` porta v3 → v4 senza convertire nulla (`Doors` nasce vuoto): quello che
la migrazione deve dimostrare è di **non toccare** i dati esistenti, e lo dimostra sulla serializzazione vera —
asset scritto col binario vecchio, riletto col nuovo, digest dei soli campi vecchi confrontato.

### Porte larghe: il gruppo

Un portone largo tre celle sono **tre bordi** che devono aprirsi e chiudersi insieme:

```
        C1      C2      C3          <- celle di un lato
     ────╫───────╫───────╫────      <- 3 bordi, stesso DoorId
        D1      D2      D3          <- celle dell'altro lato
```

`DoorId` è un'etichetta, non una struttura nuova: `SetDoorState` commuta tutti i bordi che lo condividono con
**un solo** incremento di revisione (il portone è un evento, non tre). Il gruppo non deve essere rettilineo —
un varco a gomito sono bordi su direzioni diverse con lo stesso id — e se un bordo del gruppo viene distrutto,
gli altri restano.

### Due facce, un bordo

Come le coperture, il bordo può essere dichiarato dalla cella A verso B, da B verso A, o da entrambe. La
disciplina è quella già pagata in CP 9.2 (`ApplyStructureDamage` scala **entrambe** le facce insieme,
`RTHexCoverLibrary.cpp:140-141`, altrimenti un muro disegnato due volte reggerebbe il doppio):

- `SetDoorState` scrive su **tutte** le facce dichiarate del bordo;
- `BlocksTraversal` blocca se **una qualunque** faccia è chiusa.

Così le due facce non possono divergere, e una porta disegnata da un lato solo vale comunque.

---

## 3. La regola: un solo punto di lettura

```cpp
bool URTHexCoverLibrary::BlocksTraversal(Map, From, To)
{
    return CoverBetween(Map, From, To) == ERTHexCoverType::High
        || URTHexDoorLibrary::BlocksBetween(Map, From, To);   // Closed | Locked
}
```

L'OR è **restrittivo**: un muro alto e una porta sullo stesso bordo lasciano il bordo chiuso anche a porta
aperta. Il muro è il dato più forte; `ValidateMap` segnala la coppia come incoerente — una porta dentro un muro
pieno non ha senso — ma la regola di runtime non cambia, così un livello mal disegnato non produce mai un varco
a sorpresa.

Chi consuma questa funzione non è stato toccato, ed è il punto della decisione:

| Consumatore | Riga | Modifica necessaria |
|---|---|---|
| `URTHexVisionLibrary::HasLineOfSight` | `RTHexVisionLibrary.cpp:27` | **nessuna** |
| `URTHexPathLibrary::GraphNeighbors` | `RTHexPathLibrary.cpp:23` | **nessuna** |
| combat (riduzione danno, forme) | `RTHexCombatLibrary` | **nessuna** |

---

## 4. La mutazione: un solo punto di scrittura

`URTHexDoorLibrary::SetDoorState(Map, From, To, State)` è l'unico ingresso, e porta con sé le regole di
transizione — stanno qui perché è l'unico posto che può garantirle: chi le bypassasse produrrebbe stati
impossibili.

| Da → a | `Open` | `Closed` | `Locked` | `Destroyed` |
|---|---|---|---|---|
| `Open` | – | ✅ | ✅ | ✅ |
| `Closed` | ✅ | – | ✅ | ✅ |
| `Locked` | ❌ (serve l'apertura autorizzata, CP 10.1) | ✅ | – | ✅ |
| `Destroyed` | ❌ | ❌ | ❌ | – |

Ritorna solo ciò che è **cambiato davvero**, in ordine canonico, così il chiamante ha esattamente le voci da
scrivere nel TurnLog — stessa forma di `ApplyStructureDamage`.

**La revisione.** Ogni scrittura passa da `URTHexMapAsset::UpdateCells`, che aggiorna N celle e incrementa
`Revision` **una volta**. È il segnale che `URTHexSimLibrary::IsSnapshotStale` (`RTHexSimLibrary.cpp:113`) già
confronta insieme all'hash: non è stato costruito un secondo meccanismo di invalidazione, si è usato quello che
CP 9.1 aveva messo lì per questo.

---

## 5. Il vettore in partita

La risoluzione del turno è monolitica dentro `LockInAndResolve` (`RTTurnManager.cpp:534-553`): le fasi
Prep → Dash → Blast → Move girano in un solo ciclo, quindi nessuno può mutare la mappa «fra una fase e l'altra»
dall'esterno. Perché una porta si chiuda *durante* il turno serve un vettore interno alla risoluzione.

**Decisione**: un **effetto dichiarato**, `ERTActionEffect::SetDoorState`, raccolto nel Blast e applicato a
fase conclusa — la stessa forma che CP 9.2 ha dato a `DamageStructure`.

- **Non** è un'azione nuova nel catalogo: l'azione di gioco che apre e chiude porte è CP 10.1
  (`Action.Interact` — una sola, dopo `#199` e [D-134](../decisions/RT_PDR_00_Decision_Log.md); test
  `Objectives.ActivateDoorChangesGraph`). Anticiparla qui sarebbe scope creep, e andrebbe bilanciata
  (costo, portata, slot).
- Il bordo su cui agisce è la **prima porta attraversata** dalla linea attaccante → bersaglio, per simmetria
  con `DamageStructure`, che colpisce il primo bordo **coperto** attraversato.
- Lo stato richiesto viaggia in `Amount` (valore di `ERTHexDoorState`): è la convenzione già in uso nel
  progetto — «la durata viaggia in `Amount`: interi soltanto», `RTActionEffectLibrary.cpp:58`. Un valore fuori
  intervallo non produce nessuna operazione.

**Perché non ModifyArc.** `Action.ModifyArc` (CP 8.5) è in `ERTResolutionPhase::Environment`, e
`ResolveEnvironment` gira nel **Cleanup**, cioè *dopo* il Move (`RTTurnManager.cpp:574`). Non potrebbe mai
chiudere qualcosa prima del movimento, che è precisamente ciò che questa DoD chiede.

**Ordine irrilevante.** Le operazioni si raccolgono in `FRTHexBlastPlan::DoorOps` e si applicano a colpi
risolti. A parità di bordo vince lo **stato più restrittivo**: due unità che nello stesso turno una apre e una
chiude danno lo stesso esito in qualunque ordine (invariante #3).

---

## 6. Il rischio vero: il percorso già validato

È il difetto che il checkpoint esiste per impedire, e non lo trova nessun test puro sul pathfinding.

`ResolveMovement` prende il percorso pianificato dal client e lo esegue com'è
(`RTTurnManager.cpp:2495-2497`); l'unico filtro è `TruncatePathToBudget`, che intercetta «il budget è cambiato
da quando il piano è stato scritto» e **non guarda la topologia**. Lo snapshot del Move è già fresco — il buco
non è lì. Una porta chiusa nel Blast lascerebbe quindi passare un percorso calcolato prima: il path fantasma.

**Soluzione**: `URTHexSimLibrary::TruncatePathToTopology`. Non duplica la regola dei bordi — **chiede al
grafo**: un passo sopravvive solo se la sua destinazione compare fra i `GraphNeighbors` della cella corrente.
Con una sola domanda copre porte, muri, celle bloccate e celle sparite, e resterà corretta quando CP 9.4
toccherà gli archi.

Il movimento **si ferma** all'ultima cella valida (`Fallback.Stop`), non viene annullato.

**Il reason code.** Il resolver puro non può sapere che il percorso è stato troncato per topologia: il
troncamento avviene prima che lui lo veda, e `BuildMoveLog` classificherebbe `Moved` — che direbbe il falso.
Lo sa il chiamante, e lo scrive lui: `ERTMoveOutcome::BlockedByTopology`, aggiunto **in coda** all'enum, così
le tracce già scritte non cambiano significato.

**Il Dash non è a rischio**: `ResolveDash` ricalcola con `ResolveLinearMove`/`FindPathForUnit` sulla mappa
corrente, non esegue un percorso pre-calcolato.

---

## 7. TurnLog

Nessun campo nuovo. La coppia `SrcCell`/`TgtCell` identifica il bordo, come per le coperture di CP 9.2, e i due
esiti si aggiungono **in coda** a `ERTEnvironmentOutcome`:

- `DoorClosed` — da qui in poi quel bordo non si attraversa e non si vede attraverso;
- `DoorOpened` — spiega perché al turno successivo esiste un passaggio dove prima non c'era.

---

## 8. Limiti dichiarati

1. **Nessuna mappa `.uasset` disegna ancora porte.** Il dato esiste, i consumatori runtime esistono e sono
   testati, ma l'editor per disegnarle arriva con E9/E11. `DA_HexMap_Sandbox` è vuoto in partenza (0 celle).
2. **Nessuna azione di catalogo apre o chiude porte.** C'è l'effetto dichiarato; l'azione è CP 10.1.
   Conseguenza: in partita oggi solo un'abilità a cui l'effetto viene aggiunto esplicitamente può agire su una
   porta.
3. **`Locked` non ha ancora chi la sblocchi.** `SetDoorState` rifiuta `Locked → Open` per costruzione;
   l'apertura autorizzata (chiave, consolle, obiettivo) è CP 10.1.
4. **Le porte non hanno integrità.** `Destroyed` è uno stato raggiungibile solo per comando esplicito: il danno
   strutturale che le abbatta è fuori dal perimetro di questo checkpoint (le coperture ce l'hanno da CP 9.2).
5. **Il gruppo `DoorId` non è disegnabile.** Il dato lo prevede e i test lo esercitano, ma nessuno strumento lo
   assegna: la scelta è stata di pagare ora un `int32` invece di una seconda migrazione di formato dopo.
6. **Nessuna durata.** Una porta resta nello stato in cui la si lascia: la scadenza degli stati topologici è
   CP 9.4.

---

## 9. Verifica

### Test automatici

| Test | Cosa dimostra |
|---|---|
| `Structures.Door.StateChangeBumpsRevision` | la revisione sale a ogni cambio di stato, **una volta sola** anche per un portone di tre bordi |
| `Structures.Door.InvalidatesPathCache` | uno snapshot preso prima del cambio diventa stale; il percorso ricalcolato non attraversa più |
| `Structures.Door.ClosingStopsMovement` | **turno vero in `UWorld`**: porta chiusa nel Blast, percorso già pianificato che l'attraversava, l'unità si ferma prima |
| `Structures.Door.BlocksLineOfSight` | chiusa toglie la linea di tiro, aperta la restituisce |
| `Structures.Door.GroupClosesTogether` | `DoorId` condiviso: un comando, tre bordi, una revisione |
| `Structures.Door.WallOverridesOpenDoor` | l'OR restrittivo: una porta aperta dentro un muro alto non apre nulla |
| `Structures.Door.OpsOrderIndependent` | apertura e chiusura nello stesso turno danno lo stesso esito in qualunque ordine |
| `Structures.Door.DestroyedStaysOpen` | `Destroyed` è terminale e `Locked` non si apre da sola |
| `Structures.Door.ReadsBothFaces` | il bordo vale da entrambi i lati, anche se una faccia sola lo dichiara |
| `HexMap.DoorHashDeterminism` | l'hash non dipende dall'ordine dell'array e cambia con lo stato |
| `HexMap.DoorValidation` | porte sovrapposte sullo stesso bordo, porta dentro un muro alto |
| `HexMap.DoorFormatMigration` | v3 → v4 non perde celle, coperture né transizioni |

Nessuno di questi nomi è prefisso gerarchico di un altro (§6.1 delle convenzioni di test): `Structures.Door`
da solo non esiste come test.

### Verifica manuale

`PIE-V01-DOOR` — vedi [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md).

---

## 10. Verifica di mutazione

Ogni riga è una modifica deliberata al codice di produzione, applicata **da sola**, seguita da build completa
e suite intera. Nessuna usa un `return` anticipato: in questo progetto il codice non eseguibile (C4702) è un
errore di compilazione, quindi si muta un valore o una condizione già presenti.

Baseline: **425 test, nessuno fallito** (misura sul ramo prima del merge con `main`; dopo il merge la suite
e' a **432 in 65 file** — le mutazioni sono state girate sul ramo, che e' dove il codice nuovo vive).

| # | Mutazione | File | Test caduti |
|---|---|---|---|
| 1 | L'OR con la porta esce da `BlocksTraversal` | `RTHexCoverLibrary.cpp` | `Door.{BlocksLineOfSight, ClosingStopsMovement, DestroyedStaysOpen, GroupClosesTogether, InvalidatesPathCache, OpsOrderIndependent, TruncatesPlannedPath}` — **7** |
| 2 | `Destroyed` non è più terminale | `RTHexDoorLibrary.cpp` | `Door.DestroyedStaysOpen` |
| 3 | `Locked → Open` non è più rifiutata | `RTHexDoorLibrary.cpp` | `Door.DestroyedStaysOpen` |
| 4 | Una revisione per cella invece che per gruppo | `RTHexMapAsset.cpp` | `Door.StateChangeBumpsRevision` |
| 5 | Il gruppo `DoorId` non commuta insieme | `RTHexDoorLibrary.cpp` | `Door.GroupClosesTogether`, `Door.StateChangeBumpsRevision` |
| 6 | Nessun troncamento topologico | `RTHexSimLibrary.cpp` | `Door.ClosingStopsMovement`, `Door.TruncatesPlannedPath` |
| 7 | A parità di bordo vince lo stato **meno** restrittivo | `RTHexDoorLibrary.cpp` | `Door.OpsOrderIndependent` |
| 8 | Lo stato della porta esce dall'hash | `RTHexMapAsset.cpp` | `HexMap.DoorHashDeterminism` |
| 9 | Porta dentro un muro alto non più segnalata | `RTHexMapAsset.cpp` | `HexMap.DoorValidation`, `Door.WallOverridesOpenDoor` |
| 10 | Il movimento fermato non lo dichiara nel log | `RTTurnManager.cpp` | `Door.ClosingStopsMovement` |
| 11 | La porta dichiarata dalla faccia opposta non viene trovata | `RTHexCombatLibrary.cpp` | `Door.ClosingStopsMovement` |
| 12 | La lettura guarda **una faccia sola** | `RTHexDoorLibrary.cpp` | `Door.ReadsBothFaces` ⚠️ *(vedi sotto)* |

### Cosa ha trovato

**La mutazione 12 non uccideva nulla.** Al primo giro, far leggere a `DoorBetween` una faccia sola invece di
due lasciava passare tutti e dodici i test. Il motivo: ognuno dichiarava la porta esattamente dal lato da cui
poi la interrogava, quindi il ramo «faccia opposta» non era esercitato da nessuno — mentre la decisione di
progetto (§2, *Due facce, un bordo*) dice il contrario. Il test `Structures.Door.ReadsBothFaces` colma il buco:
verifica la lettura **e** il comando dal lato che non dichiara la porta. Con lui la mutazione muore.

È il difetto ricorrente in questo repository, in una forma nuova: non un dato senza consumatore, ma una
**decisione senza verifica** — la regola era implementata correttamente e nessuno l'avrebbe notata rompersi.

**La run della mutazione 8 si era fermata a 298 test su 425** senza crash nel log. Il dato non era utilizzabile:
rieseguita insieme a una baseline di controllo, ha dato 425 completati e un solo test caduto, quello atteso. Il
troncamento era dell'ambiente, non della mutazione — ed è la ragione per cui il numero di `Test Completed` va
letto a ogni run e non dato per scontato.
