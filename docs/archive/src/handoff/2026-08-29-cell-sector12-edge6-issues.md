# Claude Task — RefactorTactics: consolidare Cell → Sector12 → Edge6 → Shared Edge

## Obiettivo

Lavora sul repository **RefactorTactics** e crea/aggiorna le issue GitHub necessarie per rendere canonica la relazione:

```text
Cell
 └─ 12 settori locali da 30°
      └─ coppie di settori
           └─ 6 bordi geometrici
                └─ bordo condiviso fra due celle
                     └─ futura transition del grafo
```

Questa attività deriva da una discussione di design già svolta. **Non reinventare il modello**: prima misura il repository e GitHub attuali, poi crea solo il delta realmente mancante.

---

# 1. Release / Epic

La release prevista è **v0.1**.

Esiste già l'epic:

- **#324 — `[EPIC v0.1] E23 · Muri, porte e interaction graph`**

Questa epic è l'owner corretto del dominio perché possiede già:

- separazione geometria/logica;
- bordi e porte;
- identity/binding;
- interaction graph;
- standability;
- transition clearance.

## Regola importante

**NON creare una nuova epic** se #324 è ancora l'owner corretto su `main`.

Prima di fare qualunque write:

1. apri #324;
2. controlla milestone, stato, body e sub-issue;
3. cerca issue duplicate con parole chiave:
   - `sector edge`
   - `sector12 edge`
   - `shared edge`
   - `cell sector address`
   - `edge identity`
   - `canonical edge`
   - `wedge`
4. controlla le issue già esistenti citate sotto.

Se il repository è cambiato dopo questo documento, **vince lo stato corrente misurato**.

---

# 2. Stato già esistente da NON duplicare

Verifica almeno queste issue:

- **#619** — occupancy a 12 settori;
- **#620** — grammatica geometrica quantizzata;
- **#621** — bake geometria → dati canonici;
- **#1615** — settore sotto il cursore;
- **#324** — E23.

Il modello attuale distingue già:

| Concetto | Cardinalità | Significato |
|---|---:|---|
| `ERTHexDirection` | 6 | direzioni tattiche / adiacenza del grafo |
| occupancy sectors | 12 | misura locale/angolare dentro una cella |
| hover sector | 12 | puntamento locale sotto il cursore |

I **12 settori NON sono direzioni del grafo**.

Le direzioni tattiche restano **6**.

Non introdurre una terza semantica ambigua della parola `Sector`.

---

# 3. Convenzione geometrica già canonica

Misura il codice, ma la baseline osservata è:

```cpp
Radians = DegreesToRadians(-30.0 + 30.0 * Sector);
```

Quindi i dodici settori sono ancorati alla convenzione pointy-top già usata dalla griglia.

Non ridecidere:

- orientamento;
- verso;
- offset iniziale;
- cardinalità.

Usa l'autorità già presente in `URTHexOccupancyLibrary::SectorBoundaryPoints`.

La griglia ha già helper per:

- `Neighbor`
- `DirectionBetween`
- `EdgeMidpointWorld`
- `EdgeRotation`
- `OppositeDirection`
- `DirectionForEdgeIndex`
- `EdgeIndexForDirection`

**Non duplicare questi calcoli.**

---

# 4. Mappatura attesa Sector12 → Edge6

La relazione logica da consolidare è:

```text
EdgeIndex = SectorIndex / 2
```

con `SectorIndex` in `[0..11]`.

La baseline risultante è:

| Settori | EdgeIndex | ERTHexDirection |
|---|---:|---|
| S0, S1 | 0 | E |
| S2, S3 | 1 | SE |
| S4, S5 | 2 | SW |
| S6, S7 | 3 | W |
| S8, S9 | 4 | NW |
| S10, S11 | 5 | NE |

ATTENZIONE: non incidere manualmente questa tabella nel gameplay se il repository può derivarla da helper canonici.

La forma preferita è:

```text
SectorIndex
    ↓ / 2
GeometricEdgeIndex
    ↓ URTHexLibrary::DirectionForEdgeIndex
ERTHexDirection
```

Il test può pin-nare la tabella; il codice dovrebbe comporre helper esistenti.

---

# 5. Issue da creare, se non esistono già

## CP 23.8 — Indirizzo canonico Cell + Sector12

Titolo consigliato:

**`CP 23.8 · Indirizzo canonico della cella e del settore locale a 12 spicchi`**

### Why

Oggi esistono l'identità della cella e i dodici settori, ma serve un modo non ambiguo per riferirsi a uno specifico settore locale di una specifica cella senza confonderlo con:

- una direzione del grafo;
- un edge;
- un indice runtime;
- un Actor.

### Scope

Definire il contratto logico:

```text
CellAddress = FRTCellId{X,Y,Layer}
LocalSector = integer/enumerazione [0..11]
CellSectorAddress = CellAddress + LocalSector
```

Esempio leggibile per log/debug:

```text
(10,5,0):S03
```

Il nome C++ definitivo va scelto **solo dopo aver cercato i tipi esistenti**.

Non inventare automaticamente `FRTSector12`, `FRTDirection6` o nomi simili se non esistono.

### DoD minima

- [ ] Una cella + settore identifica esattamente uno dei 12 wedge locali.
- [ ] Range `0..11` validato.
- [ ] `FRTCellId` resta l'identità della cella.
- [ ] Nessun `CellIndex` runtime diventa identità persistente.
- [ ] Equality/hash deterministici se viene introdotto un value type.
- [ ] Debug string stabile.
- [ ] Nessuna replica automatica introdotta.
- [ ] Nessun dato di planning team/private coinvolto.
- [ ] Test su tutti i 12 valori.
- [ ] Test che due celle diverse con stesso sector index non collidono.
- [ ] Test che la stessa cella con due sector index diversi non collida.

### Out of scope

- edge identity;
- transition identity;
- pathfinding;
- replication;
- hover rendering;
- occupancy classification.

### Dipendenze

- #619
- #1615 come consumer/adiacenza, non come owner del modello.

---

## CP 23.9 — Relazione canonica Sector12 ↔ Edge6

Titolo consigliato:

**`CP 23.9 · Dodici settori locali, sei bordi: una sola conversione canonica`**

### Why

Due wedge adiacenti appartengono allo stesso lato geometrico dell'esagono.

Serve una conversione canonica senza creare nuove direzioni di movimento.

### Scope

Aggiungere/centralizzare helper puri equivalenti a:

```text
Sector12 -> GeometricEdgeIndex
GeometricEdgeIndex -> ERTHexDirection
ERTHexDirection -> GeometricEdgeIndex
```

La regola baseline:

```text
EdgeIndex = SectorIndex / 2
```

Poi usare gli helper già esistenti della hex library.

### DoD minima

- [ ] Tutti i 12 settori mappano in esattamente 6 edge.
- [ ] Ogni edge possiede esattamente 2 settori.
- [ ] I 12 settori NON diventano 12 archi del grafo.
- [ ] `ERTHexDirection` resta a 6 valori.
- [ ] Nessuna trigonometria duplicata.
- [ ] Nessuna tabella manuale duplicata se derivabile.
- [ ] Test esaustivo `0..11`.
- [ ] Test round-trip edge ↔ direction.
- [ ] Test che la convenzione resta coerente con `SectorBoundaryPoints`.
- [ ] Test che resta coerente con `DirectionForEdgeIndex`.
- [ ] Mutation check: rompere `/2` deve far cadere il test atteso.

### Test attesi

Esempi di naming:

```text
RefactorTactics.Hex.Sector12MapsToSixEdges
RefactorTactics.Hex.EachEdgeOwnsExactlyTwoSectors
RefactorTactics.Hex.SectorEdgeMappingMatchesPointyTopConvention
```

Adatta i nomi allo stile reale del repository.

### Out of scope

- shared edge identity;
- transition state;
- porte;
- cover;
- pathfinding;
- networking.

### Dipendenze

- CP 23.8
- #619
- helper hex già esistenti.

---

## CP 23.10 — Identità canonica del bordo condiviso

Titolo consigliato:

**`CP 23.10 · Un bordo condiviso ha una sola identità, da entrambe le celle`**

### Why

Lo stesso confine può essere visto in due modi:

```text
Cell A + E
Neighbor(A,E) + W
```

Sono due descrizioni dello **stesso bordo fisico/logico**.

Se equality/hash li trattano come due oggetti diversi, porte, cover, interaction graph, cache e future transition possono divergere.

### Scope

Definire un indirizzo canonico del bordo condiviso.

Forma concettuale:

```text
SharedEdgeAddress = canonical(CellA, DirA)
```

con proprietà:

```text
Canonical(A, E)
==
Canonical(Neighbor(A,E), W)
```

e analogamente per tutte le coppie opposte.

Il nome C++ è da scegliere dopo audit del repository.

Possibili forme, NON prescrittive:

```text
FRTCellEdgeAddress
FRTHexEdgeAddress
FRTSharedEdgeId
```

Non introdurre un nome nuovo se esiste già un tipo semanticamente equivalente.

### Canonicalizzazione

Preferire una regola deterministica basata sui dati logici, per esempio ordinamento stabile dei due endpoint/celle, senza dipendere da:

- pointer;
- ordine di inserimento;
- hash map;
- Actor;
- world-space float;
- frame;
- asset load order.

### DoD minima

- [ ] Ogni edge di una cella produce un'identità canonica.
- [ ] Il bordo visto dalla cella adiacente produce la stessa identità.
- [ ] Verificato su tutte le 6 direzioni.
- [ ] Equality/hash indipendenti dall'ordine dei due lati.
- [ ] Nessun float nell'identità.
- [ ] Nessun Actor pointer nell'identità.
- [ ] Nessun `CellIndex` runtime persistente.
- [ ] Debug string leggibile.
- [ ] Test di simmetria per tutte le direzioni.
- [ ] Test hash `A:E == B:W`.
- [ ] Test su layer diversi: non collidono.
- [ ] Test su celle non adiacenti: non possono formare uno shared edge valido.

### Test attesi

```text
RefactorTactics.Hex.SharedEdgeHasSameIdentityFromBothCells
RefactorTactics.Hex.SharedEdgeHashIsOrderIndependent
RefactorTactics.Hex.SharedEdgeDoesNotCrossLayers
```

Adatta allo stile reale.

### Out of scope

**NON creare ancora la Transition Key completa.**

Il bordo condiviso è una base per la futura transition, ma la transition può avere semantica ulteriore:

- altezza;
- tipo di passaggio;
- porta;
- climb;
- jump;
- bridge;
- tunnel;
- elevator;
- one-way state;
- clearance.

La Transition Key appartiene al passo successivo / E23.7, salvo che il repository dimostri che esista già ed abbia bisogno solo di integrazione.

### Dipendenze

- CP 23.9
- #324 / E23
- futura E23.7.

---

# 6. Aggiornamento della Epic #324

Dopo aver creato le issue, aggiorna #324 soltanto se serve per renderle tracciabili.

Aggiungi una sezione tipo:

```markdown
### Consolidamento indirizzamento spaziale

- [ ] #NNNN — CP 23.8 · Cell + Sector12 address
- [ ] #NNNN — CP 23.9 · Sector12 ↔ Edge6
- [ ] #NNNN — CP 23.10 · Shared Edge identity
```

Non riscrivere la storia dell'epic.

Non cancellare note datate, decisioni o rationale esistenti.

Se GitHub supporta sub-issue/parent relationship nel workflow attuale, collega le issue anche strutturalmente.

---

# 7. Milestone e label

Prima di creare:

1. misura le milestone esistenti;
2. misura le label reali;
3. usa la nomenclatura del repository.

Atteso:

```text
Release/Milestone: v0.1
Epic: #324
```

NON inventare label.

Se esistono label per:

- priority;
- map;
- architecture;
- gameplay;
- testing;
- v0.1;

usa solo quelle già presenti e coerenti con issue vicine.

---

# 8. Regole architetturali RefactorTactics da rispettare

## Determinismo

Stesso:

- snapshot;
- regole;
- versione;
- seed;

deve produrre lo stesso risultato.

Per questi address/ID:

- preferire interi/enum;
- ordine stabile;
- niente pointer;
- niente `TMap` iteration come fonte di identità;
- niente float come chiave canonica.

## Mappa

La mappa è un grafo tattico 3D.

`FRTCellId` contiene:

```text
X
Y
Layer
```

`Layer` è parte dell'identità.

## Networking / privacy

Queste issue non devono introdurre replica di planning.

In particolare:

- niente planning avversario su Actor globali replicati;
- niente cursore nemico replicato;
- niente intenti team dentro questi address.

Sono tipi/invarianti di mappa, non canali di rete.

## Runtime vs Editor

Le regole pure e testabili devono vivere nel modulo runtime/core.

L'Editor le consuma.

Non mettere una regola fondamentale solo in `RefactorTacticsEditor`.

---

# 9. Audit obbligatorio prima della creazione

Esegui ricerche equivalenti a:

```bash
git grep -n "SectorBoundaryPoints" Source/
git grep -n "DirectionForEdgeIndex" Source/
git grep -n "EdgeIndexForDirection" Source/
git grep -n "OppositeDirection" Source/
git grep -n "EdgeAddress\|SharedEdge\|EdgeId\|TransitionKey\|TransitionId" Source/
git grep -n "Sector12\|SectorIndex\|OccupancySector" Source/
```

Poi cerca le issue GitHub per concetti equivalenti.

Se trovi una issue che copre già ≥80% di uno dei checkpoint:

- NON crearne una duplicata;
- aggiorna quella esistente, se appropriato;
- registra nel report finale perché hai riusato quella issue.

---

# 10. Non fare

- ❌ Non creare una nuova Epic se #324 resta l'owner.
- ❌ Non creare 12 direzioni.
- ❌ Non modificare `ERTHexDirection` per rappresentare wedge.
- ❌ Non creare una seconda convenzione angolare.
- ❌ Non duplicare `SectorBoundaryPoints`.
- ❌ Non duplicare `DirectionForEdgeIndex`.
- ❌ Non usare world-space float come identità.
- ❌ Non usare Actor/component pointer come ID.
- ❌ Non trasformare hover-sector in dato di simulazione.
- ❌ Non trasformare occupancy-sector in arco di pathfinding.
- ❌ Non introdurre subito Transition Key se manca ancora il contratto Shared Edge.
- ❌ Non bumpare il formato asset solo perché nasce un helper/value type non serializzato.
- ❌ Non inventare API Unreal o tipi che il repository non possiede senza dichiararli come proposta.

---

# 11. Definition of Done globale

L'attività è conclusa quando:

- [ ] repository e GitHub correnti sono stati auditati;
- [ ] nessuna issue duplicata è stata creata;
- [ ] CP 23.8 esiste o è coperto da issue equivalente;
- [ ] CP 23.9 esiste o è coperto da issue equivalente;
- [ ] CP 23.10 esiste o è coperto da issue equivalente;
- [ ] tutte sono associate alla v0.1;
- [ ] tutte puntano a #324;
- [ ] #324 è aggiornata con i nuovi checkpoint se necessario;
- [ ] ogni issue contiene Why, Scope, Out of Scope, dipendenze, DoD e test;
- [ ] è esplicito che 12 sectors ≠ 12 graph directions;
- [ ] è esplicito che `Layer` partecipa all'identità;
- [ ] è esplicito che nessun dato di planning viene replicato;
- [ ] il report finale contiene numeri e URL delle issue create/riusate.

---

# 12. Output finale richiesto a Claude

Alla fine NON limitarti a dire “done”.

Produci una tabella:

| CP | Issue | Azione | Milestone | Dipendenze |
|---|---|---|---|---|
| 23.8 | #... | created/reused/updated | v0.1 | ... |
| 23.9 | #... | created/reused/updated | v0.1 | ... |
| 23.10 | #... | created/reused/updated | v0.1 | ... |

Poi riporta:

1. URL di #324;
2. URL di ogni issue creata/riusata;
3. eventuali duplicati evitati;
4. differenze trovate fra questo documento e `origin/main`;
5. eventuali decisioni ancora aperte;
6. il **passo successivo consigliato**: definire la Transition Key / E23.7 solo dopo che Shared Edge identity è stabile.

---

# Nota finale

Questo documento descrive l'intento emerso dalla discussione, ma **il repository corrente resta l'autorità**.

Quando testo, issue e codice divergono:

1. misura;
2. cita il simbolo/file/issue corrente;
3. evita duplicazioni;
4. crea soltanto il delta necessario.
