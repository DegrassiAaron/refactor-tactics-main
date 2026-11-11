# Level Designer handoff (2 file) — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **applicata ai sorgenti** · **Data**: 2026-08-12
> **HEAD della revisione**: `402c154b`
> **Sorgenti revisionati** (erano untracked in radice; **archiviati il 2026-08-13** in
> `docs/archive/src/handoff/`, con la loro riga d'indice):
> [`2026-08-12-level-designer-01-context.md`](../../archive/src/handoff/2026-08-12-level-designer-01-context.md) (1396 → 1585 righe) ·
> [`2026-08-12-level-designer-02-implementazione-consolidamento.md`](../../archive/src/handoff/2026-08-12-level-designer-02-implementazione-consolidamento.md) (1077 → 1254)
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Newman · Crispin
> **Scopo**: verificare che il handoff possa essere eseguito da un agente **senza produrre duplicazione**,
> che è la proprietà che il handoff stesso dichiara di voler difendere.
> **Quarto della sua famiglia**: segue `map-editor-brief-spec-panel-2026-08-09` e
> `map-sketch-editor-spec-panel-2026-08-12`, di cui è il consolidamento a valle.

---

## 1. Il verdetto in una riga

Sono i due documenti map-editor **meglio orientati** della serie — la tesi «non costruire un secondo editor,
completa quello che c'è» è corretta e sostenuta dal codice — e sono anche i primi che **pianificano
esplicitamente una duplicazione**: il §25 del file 02 mette in sequenza un commit
`feat(map): add edge center/orientation helper` per una funzione che esiste, è testata e ha già consumatori.

| | Voci | Significato |
|---|---:|---|
| 🔴 Critico | **4** | blocca l'esecuzione o produce lavoro duplicato |
| 🟠 Alto | **6** | il lavoro parte, ma su una base che va decisa prima |
| 🟡 Medio | **8** | attrito, incoerenza interna, tracciabilità |
| 🟢 Basso | **2** | forma |

**La contraddizione centrale**: entrambi i file predicano «non duplicare, verifica sempre», e poi
(a) mantengono a mano una copia di uno stato **generato**, (b) lasciano aperte **due domande la cui risposta
è già nel repository**. Chiuderle è costato venti righe.

---

## 2. Le cinque verifiche che hanno cambiato la revisione

Fatte con `git grep` e `gh` contro HEAD `402c154b`, non contro lo snapshot dichiarato dal handoff.

| Verifica | Esito |
|---|---|
| I quattro tool di §4 (file 01) | ✅ confermati — `RTHexEditorMode.cpp:29-32` |
| Feature ID di §20 (file 02) | ✅ esistono — `feature-registry.yaml:4584, 4709` |
| «esiste una funzione canonica di centro/orientamento bordo?» (§5.2 file 02) | ❌ **domanda già risolta**: `EdgeMidpointWorld` `RTHexLibrary.h:90` + `EdgeRotation` `:100` |
| «se la decisione non esiste…» sull'invertibilità (§11 file 02) | ❌ **decisione già registrata**: `MSE-1`, `docs/OPEN_DECISIONS.md` |
| #620/#621 come blocco «MISSING» (§5.3 file 01) | ⚠️ contraddetto: `RT-FEAT-TOOL-MAP-GEOMETRY` è `IMPLEMENTING` 3/7 |

**Stato issue riverificato** — la catena di §6 regge per intero: #588 e #619 `CLOSED`; #554, #620, #621,
#622, #623 `OPEN`. È salito solo l'HEAD (`52c08286` → `402c154b`, ultimo commit su #651): il repository si è
mosso altrove, non su questo filone.

⚠️ **Worktree attivo non noto ai due file**: `D:/Repositories/rt-migrazione` è su
`feat/554-transizioni-visibili`. Chi apre #554 deve leggerlo prima.

---

## 3. Findings

### 🔴 Critici

| # | Esperto | Problema | Dove |
|---|---|---|---|
| C1 | Nygard | `MSE-1` esiste, ha innesco pinnato a #621 ed **è la ragione registrata** di `spec: partial`; il file la tratta come domanda senza owner e rischia di farne aprire una seconda | 02 §11 |
| C2 | Fowler · Wiegers | Matrice di stato scritta a mano che duplica e contraddice `featuremap.shortlist.md`, **generata**, senza una sola colonna di evidenza | 01 §5 |
| C3 | Fowler | La domanda sull'helper di bordo ha risposta `SÌ`; il §25 pianifica un commit che lo riscrive | 02 §5.2, §25 |
| C4 | Adzic | **«Junction» non è definita in nessuno dei due file**, ma il validator deve rifiutarne le invalide e i test sono già nominati: requisito non implementabile | 01 §8 · 02 §7-8 |

Su C1 vale la pena la citazione integrale della nota di metodo di `MSE-1`, che il handoff non riportava e
che contiene già il candidato di risposta: *«`D2` è la stessa forma di risposta che `MSE-1` cerca — separare
i produttori invece di arbitrarli. Se regge per il costo, è il primo candidato da provare sui bordi.»*

### 🟠 Alti

| # | Esperto | Problema | Dove |
|---|---|---|---|
| H1 | Fowler | Orientamento **pointy-top** mai dichiarato, mentre si propone `ERTTacticalAxis{Axis0…Axis150}`: nomi ancorati a un datum implicito, e i nomi non sono testabili | 01 §16 · 02 §6 |
| H2 | Nygard | Dopo #621 i campi a produttore condiviso sono **tre**, non due: manca `ERTHexSurface`, scritta sia dal bake (void/cliff) sia da Paint/Fill, e non coperta da `MSE-1` | 01 §9 |
| H3 | Newman | Nessuna clausola su formato/hash/migrazione, benché #619 abbia appena fatto `CurrentFormatVersion` 6→7 (`RTHexMapAsset.h:65`) con il suo test | entrambi |
| H4 | Crispin | «chiaramente ghost», «no zone quasi nere», «exposure prevedibile»: aggettivi, non criteri. Una seduta senza condizione di pass produce un ✅ che significa «l'ho guardato» | 01 §10-11 · 02 §15-16 |
| H5 | Adzic | Nessun test lega #620+#621+#554. Ne basta uno — `Spec.Map.BakedWallSeversThePath` — e il gemello esiste già (`BridgeBreaksThePath.json`) | 02 §8 |
| H6 | Crispin | «add geometry grammar fixtures» senza citare `RTOccupancyFixtures.h`, che il registry destina già a #620/#621 | 02 §25 |

### 🟡 Medi

| # | Problema | Dove |
|---|---|---|
| M1 | `IMPLEMENTED` non definito: 30 righe IMPLEMENTED convivono con «production usability non completa» | 01 §5 |
| M2 | §27 è un diagramma di successo: sedici passi, zero rami di fallimento. Cosa fa il designer se `VALIDATE` fallisce al passo dodici? | 01 §27 |
| M3 | Venti divieti, quasi nessuno con metodo di rilevazione, in un repository che usa già `git ls-files` come oracolo | 01 §29 |
| M4 | Mutation test con trigger indefinito («quando il repository la richiede») | 02 §24 |
| M5 | #554 sequenziato in **tre** modi diversi fra i due file | 01 §6 vs §28 · 02 §13 |
| M6 | Handoff senza owner né condizione di scadenza | entrambi |
| M7 | §31 omette `docs/OPEN_DECISIONS.md` — dove vive `MSE-1` — e `featuremap.shortlist.md` | 01 §31 |
| M8 | La deriva su PR #598 **è già stata corretta** nel registry: il perimetro residuo sono i soli body di #620/#622 | 02 §3 |

### 🟢 Bassi

| # | Problema | Dove |
|---|---|---|
| L1 | «Min cost 600» senza unità | 01 §21 |
| L2 | Colonna «Owner corrente» = numero di issue, in un file che dichiara inaffidabile lo stato issue | 01 §5.3 |

---

## 4. Qualità dei due sorgenti

Giudizio qualitativo del panel sulle quattro dimensioni, **non** una misura.

| Dimensione | File 01 | File 02 | Motivo |
|---|---:|---:|---|
| Chiarezza | 8/10 | 8/10 | Prosa asciutta, diagrammi che servono, principi memorabili |
| Completezza | 6/10 | 7/10 | Junction indefinita, formato/migrazione assenti, produttori di `Surface` non contati |
| Testabilità | 4/10 | 6/10 | Il 01 è quasi privo di oracoli; il 02 è salvato dalla suite di §8 |
| Consistenza | 6/10 | 7/10 | Matrice §5 vs shortlist generata; #554 sequenziato in tre modi |

**Da non perdere nella revisione**: il §29 del file 02 — il referto obbligatorio con `WHAT WAS REUSED` — e
il §32 — `Runtime Rule → Pure query → Editor visualization`. Sono i due meccanismi che davvero prevengono
la duplicazione, sono sopra la media dei documenti, e vanno preservati intatti.

---

## 5. Cosa è stato applicato ai sorgenti

**Ventuno** interventi marcati `🔎 PANEL` inseriti nei due file — **20** blocchi in citazione (11 nel 01,
9 nel 02) più la §10-bis nuova del 02 — senza rimuovere contenuto originale: i sorgenti restano leggibili
come referto d'audit, e le correzioni sono distinguibili da ciò che l'autore aveva scritto. In più **3**
annotazioni inline (`… — 🔎 PANEL: …`), che non sono blocchi e non vanno contate come tali.

> ⚠️ **Questa riga portava quattro numeri scritti a mano, e tutti e quattro erano sbagliati**: diceva
> «diciassette (10 nel 01, 7 nel 02)» e `1396 → 1585` / `1077 → 1254` righe. Rimisurati il 2026-08-13
> dopo l'archiviazione — `Select-String '^> .{0,3} \*\*PANEL'` per i blocchi, `(Get-Content $f).Count`
> per le righe, che è l'unico modo che non scarta le righe vuote — i due file sono **1672** e **1280**
> righe. Il valore *prima* della revisione non è verificabile **da nessuno**: i sorgenti non erano
> versionati fino a oggi, quindi non esiste una baseline con cui confrontarli. È la ragione per cui un
> numero così non andava scritto, ed è lo stesso difetto che il README dell'archivio si porta dietro da
> sei versioni.

**File 01** — intestazione (owner, scadenza, HEAD corretto, worktree attivo) · §5 (autorità della shortlist
+ vocabolario di stato) · §5.3 (owner = feature ID, `MSE-1` come gate) · §8 (junction da definire; helper di
bordo già esistente) · §9 (terzo produttore su `Surface`) · §10 (condizione di pass per il ghost) · §11
(oracoli del lighting) · §16 (pointy-top e nomi degli assi) · §27 (casi d'uso con estensioni) · §29
(divieti con comando di rilevazione) · §31 (`OPEN_DECISIONS.md` e shortlist fra le fonti).

**File 02** — §3 (perimetro del drift ristretto) · §5.2 (risposta con simboli e test) · §8 (fixture esistenti
+ scenario di catena) · **§10-bis nuova** (formato, hash, migrazione) · §11 (`MSE-1` per ID, innesco,
candidato) · §13 (worktree attivo + sequenziamento di #554) · §24 (trigger della mutazione) · §25 (i due
commit da correggere).

---

## 6. Le tre decisioni aperte — chiuse in sessione socratica il 2026-08-12

Le tre domande che il panel aveva lasciato all'autore sono state riaperte in modo socratico e **chiuse
leggendo il codice**, non ragionando a tavolino. Tutte e tre si sono rivelate meno costose di come erano
poste, e due hanno **tolto** lavoro:

| Domanda | Esito | Evidenza che l'ha decisa |
|---|---|---|
| Definizione di junction | ❌ **Non è un concetto di #620.** Rappresentazione a **polilinea** → continuità strutturale. `ValidJunction` e `RejectsInvalidJunction` **cancellati** dalla suite | L'OR di `ComputeMask` (`.cpp:143-152`) rende la junction trasparente; la Fixture 2 `Corner()` è già una junction e `CornerSpansEverySectorItCrosses` la copre |
| `ERTHexSurface` dentro o fuori `MSE-1` | ✅ **Fuori. Il bake non scrive `Surface`.** `MSE-1` resta a due campi | `Void` è una superficie dipinta fra nove; `Fill` propaga sulla contiguità di superficie, quindi una `Surface` cotta cambierebbe lo **strumento**, non il dato; il precipizio è già `bBlocksMovement` + `!bBlocksLineOfSight` |
| Edit invalido: respinto o segnalato | ✅ **Entrambi, a due strati** — il precedente esiste già | `ValidateMap` restituisce `TArray<FString>` e **non blocca** (`RTHexMapAsset.cpp:303`, `RTHexMapActor.cpp:690-698`); `RTHexCoverLibrary.h:136` è già *«più severo di `ValidateMap`»* |

### E una quarta che nessuno aveva visto: `MSE-2`

Cercando la risposta alla prima domanda è emersa una collisione fra due decisioni entrambe corrette: gli
assi tattici di #620 (`0/30/60/90/120/150`) **coincidono** con i confini dei dodici settori di #619
(`-30 + 30k`), e il contatto sul confine conta per entrambi i settori adiacenti. Le soglie `ConstrainedFrom = 4`
e `BlockedFrom = 6` sono state calibrate su quattro fixture che **evitano di proposito** i multipli di 30°.

Registrata come **`MSE-2`** in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), innesco `#620`.

⚠️ **Nota di metodo, la più utile della sessione.** Ho derivato il conteggio dei settori a mano tre volte
ottenendo tre risultati diversi: la geometria dei dodici triangoli con la regola «il bordo conta» è più
insidiosa di quanto la lettura suggerisca. Per questo la revisione ha prodotto **due test invece di un
numero** — uno che asserisce il caso verificabile con certezza, uno che *misura e registra* quello che non
lo è. Nessun conteggio è stato scritto nei documenti come se fosse una misura.

### Codice aggiunto

| File | Cosa |
|---|---|
| `Tests/RTOccupancyFixtures.h` | Fixture 5 (`SegmentOnSectorBoundary`), 6 (`SegmentJustOffSectorBoundary`), più `WallOnHexEdge` / `WallOnHexEdgeInset` / `WallsOnConsecutiveEdges` |
| `Tests/RTHexOccupancyTests.cpp` | `SegmentOnSectorBoundaryOccupiesBothAdjacentSectors` (asserisce) · `PerimeterWallsOccupancyIsRecorded` (misura, asserisce solo monotonia e sottoinsieme) |

### ✅ Compilati ed eseguiti il 2026-08-13 — `MSE-2` è quantificata

Il primo ambiente non aveva una build Unreal; questo sì (`D:/EpicGames/UE_5.8`).
Build `RefactorTacticsEditor` Win64 Development → `Result: Succeeded`.
`Automation RunTests RefactorTactics.HexOccupancy` headless (`-nullrhi`) → **19 dichiarati, 19 eseguiti,
0 falliti**. I due numeri coincidono: nessun test è rimasto fuori dalla run.

**Il margine non regge**: 1 muro perimetrale → 4/12 settori (`Constrained`), **2 muri → 6/12
(`Blocked`) con quattro lati ancora aperti**, 3 muri → 8/12. Due muri consecutivi sono l'angolo di una
stanza.

**E la causa non sono le soglie.** La misura aggiunta oggi confronta il muro intero con lo stesso muro
rientrato del 5% agli estremi: 4 settori contro **2**. Metà del conteggio è contatto sul solo *vertice*
dell'esagono — punto in comune fra quattro triangoli di settore, area invasa nulla. È una quinta uscita
per `MSE-2`, non elencata dalla revisione perché invisibile senza la misura, e lascia le soglie intatte.
Registrata in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

> La nota di metodo qui sopra si conferma da sola: la derivazione a mano aveva prodotto tre risultati
> diversi, e il conteggio corretto (`{0,1,2,11}` per un muro solo) contiene un settore — l'`11` — che sta
> dall'**altra parte** del lato murato. Nessuna delle tre derivazioni ci era arrivata.
