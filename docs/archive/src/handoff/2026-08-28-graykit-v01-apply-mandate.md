# CLAUDE CODE — Apply GrayKit v0.1 Consolidation

> `HISTORICAL` · **Mandato d'autore consumato**, non una fonte. · **Consumato**: 2026-08-28 · **Base**:
> `483e031a` (`main`).
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Il file stava alla radice del repository come
> `CLAUDE_Apply_GrayKit_v0.1_Consolidation_2026-08-28.md`, untracked, 305 righe. È il **mandato di
> applicazione** del kit archiviato accanto, in
> [`2026-08-28-graykit-v01-roadmap-consolidated.md`](2026-08-28-graykit-v01-roadmap-consolidated.md): i due
> si leggono insieme e sono stati revisionati insieme.
>
> **Cosa possiede**: le dodici decisioni d'autore da preservare e gli undici passi di applicazione, verbatim.
> **Cosa non possiede**: nessuna autorità, e nessuna esecuzione. Il referto completo è
> [`../../../roadmap/plans/graykit-v01-consolidation-spec-panel-2026-08-28.md`](../../../roadmap/plans/graykit-v01-consolidation-spec-panel-2026-08-28.md).
>
> ⛔ **Nessuno dei suoi passi è stato eseguito.** Le dieci azioni GitHub della roadmap §6 e le otto del
> PASSO 8 **non sono state applicate**: il mandato di quella sessione era consumare e archiviare. L'unico
> passo eseguito alla lettera è il **PASSO 10** — *«archivia o elimina il precedente file standalone; non
> lasciare due source of truth»*.

## Il verdetto, in breve

✅ **La disciplina delle clausole condizionali è la cosa migliore di questo mandato**, ed è la ragione per
cui i suoi difetti sono correggibili invece che dannosi: *«se HEAD non la decide già»*, *«se ancora
presenti»*, *«se la prova su main è ancora vera»*, e soprattutto il PASSO 0 — *«Se uno di questi è cambiato,
**HEAD vince** e devi riportare la divergenza»*. In **tre** casi su tre la condizione si è rivelata falsa,
il che significa che le clausole hanno funzionato e la prosa attorno no.

🔴 **Il PASSO 6 prescrive l'opposto della decisione che invoca.** Dice *«Chiudi/scrivi la decisione D-225
prima di produrre `GB_FOW_CellFull`»*: `D-225` è **accettata** il 2026-08-28, sceglie il **nascondimento** e
dichiara *«rende necessaria l'intera famiglia `GB_FOW_*`, `CellFull` incluso»*. Chi esegue il passo omette
il proxy che la decisione rende obbligatorio.

🔴 **Il PASSO 0 elenca quindici issue da auditare, e tre di quelle su cui manda a lavorare sono chiuse**:
`#956`, `#1467`, `#1094`. Il PASSO 8 assegna azioni su tutte e tre.

🔴 **Il PASSO 6 descrive il nodo FoW come lavoro di proxy**, mentre `D-225` dichiara che il lavoro vero è
l'**anello mancante** — mappa inversa cella→istanza e aggiornamento per-istanza a runtime, *«non stimato
qui»*. `RebuildInstances` gira solo all'allestimento.

🟠 **Il PASSO 3 pone la domanda obbligatoria sulla struttura sbagliata**: cita
`FRTGeometrySegment{Axis, Offset, AlongStart, AlongEnd}`; la struttura reale ha **sei** campi — mancano
`Layer` (*«stessa semantica di `FRTCellId::Layer`»*, in un gioco hex multilivello) e `WallType`. E il punto 7
delle decisioni da preservare manda a cercare una granularità d'anchor che `Map/RTGeometryGrammar.h` decide
già: `RT_GeometryQuanta = 12`.

🟠 **Il PASSO 5 chiede di creare o aggiornare `GEO-4`**, che esiste già completa in `OPEN_DECISIONS.md` con
due uscite argomentate, e che porta un collegamento assente dai kit: la stessa domanda vale per il
**proiettile**, owner [`#1392`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1392), *«e le
due non vanno separate»*.

⚠️ **Ridondanza fra i due file**: PASSO 8 (otto azioni) e roadmap §6 (dieci azioni) sono la stessa lista con
numerazioni diverse, e PASSO 10 e §7 danno **due** commit plan divergenti per lo stesso lavoro. Due elenchi
per lo stesso lavoro sono la seconda source of truth che la §0 del kit vieta — dentro il kit stesso.

---


## Mandato

Applica al repository RefactorTactics la roadmap consolidata GrayKit v0.1 del 2026-08-28.

**Non creare una seconda roadmap, una seconda Epic GrayKit, una nuova occupancy issue o una nuova FoW issue.**

Prima di scrivere, rimisura HEAD. Le informazioni sotto derivano da audit GitHub del 2026-08-28 ma HEAD può essere avanzato.

---

## Decisioni d'autore da preservare

1. `HexSize = 150 cm`.
2. `LayerHeight = H = 250 cm`.
3. Occupancy = 12 settori da 30° + core.
4. Le direzioni tattiche restano 6.
5. Muro/polilinea aperta != volume occupancy.
6. Muri/cover/porte possono usare geometria quantizzata anche **interna alla cella**.
7. Nuovo requisito: `Center → SideAnchor`, con `SideAnchor` quantizzato. Non inventare la granularità se HEAD non la decide già.
8. Nessuna microcella / nuova `FRTCellId` per questo.
9. FoW GrayKit per team/viewer: `Cell`, `Partial`, `Full` solo se lo scope abilita hiding.
10. Altezza visuale Full = `0.88 H` (=220 cm oggi), NON `LayerHeight=220`.
11. I 12 settori occupancy non diventano visibility sectors.
12. Nessun dato privato avversario viene inviato per il FoW.

---

# PASSO 0 — Audit HEAD

Verifica:

- issue #286 #324 #619 #620 #621 #712 #832 #956 #1094 #1095 #1096 #1155 #1174 #1239 #1467;
- `docs/roadmap/roadmap-v0.1.md`;
- `docs/technical/systems/spec-graybox-placement-contract.md`;
- `docs/technical/systems/spec-hex-geometry-authoring.md`;
- `docs/OPEN_DECISIONS.md`;
- Decision Log;
- Team Knowledge/LOS spec owner corrente.

Conferma o smentisci:

- #1174 è tecnicamente risolta e solo amministrativamente aperta;
- #1094 ha solo GBX-1/GBX-5 rinviate a U25;
- U22/#712 è 4/4 osservabile;
- #1239 possiede già InteriorWalls + D-179;
- #1467 ha già il commento GrayKit con `0.88 H` e D-225 pendente;
- #324 ha titolo v0.2 ma label/body v0.1;
- il Feature Registry è rimosso e non va reintrodotto.

Se uno di questi è cambiato, HEAD vince e devi riportare la divergenza.

---

# PASSO 1 — Roadmap canonica

Aggiorna `docs/roadmap/roadmap-v0.1.md`.

Non creare una nuova Epic.

Inserisci una sottosezione GrayKit v0.1 sotto E21 (o nel punto che HEAD usa per linked work) con questo ordine:

```text
baseline: #1155 #619 #620 #621 #832 #1096
admin: close #1174 · rename #324
board: #956
U25 dimensions: #1095 → GBX-1 / GBX-5
internal geometry: #1239
knowledge/FoW: #1467 + D-225
integration: #286
```

Dichiara esplicitamente che è una **execution overlay**, non una release ladder parallela e non un nuovo CP E21.x.

---

# PASSO 2 — Graybox placement contract

Owner:
`docs/technical/systems/spec-graybox-placement-contract.md`

Integra:

## Volume vs boundary

```text
closed footprint → occupancy (12 sectors + core)
open segment     → boundary semantics
```

Un muro non entra nel sector count solo perché attraversa la cella.

## Internal segment placement

L'attuale `EdgeBound` descrive il bordo condiviso fra due celle e non basta per un muro interno.

Estendi il **vocabolario di authoring** con una classe/descrizione per binding a un segmento geometrico quantizzato interno. Non trasformarla automaticamente in enum/runtime data se non serve. Scegli il nome coerente con HEAD.

## FoW GrayKit

Documenta come presentation contract:

- `GB_FOW_Cell`;
- `GB_FOW_CellPartial`;
- `GB_FOW_CellFull` condizionale a D-225;
- `DBG_LOS_Ray`;
- `DBG_VisibleSector`;
- `DBG_OccludedSector`;
- footprint `HexSize`;
- Full = `0.88 H`;
- Team/viewer-relative;
- presentation-only;
- 12 occupancy sectors != visibility model.

Non duplicare le regole di Team Knowledge/LOS.

## Decisioni aperte

Riallinea il testo a:
- GBX-2/3/4/6 chiuse;
- GBX-1/5 → U25.

---

# PASSO 3 — Geometry Authoring

Owner:
`docs/technical/systems/spec-hex-geometry-authoring.md`

Aggiungi il requisito:

> Una frontiera strutturale può partire dal centro della cella e terminare su un anchor quantizzato del lato/perimetro (`Center → SideAnchor`).

Vincoli:

- `Center` exact, non epsilon;
- `SideAnchor` discreto;
- no world-float persistito;
- no subcell `FRTCellId`;
- no 12 movement directions;
- open segment non contribuisce automaticamente a occupancy;
- stessa source quantizzata → stesso derivato/bake/hash.

### Rimisura la rappresentazione esistente

Controlla `FRTGeometrySegment{Axis, Offset, AlongStart, AlongEnd}`.

Domanda obbligatoria:

> rappresenta davvero tutto lo scope `Center → SideAnchor` richiesto, oppure solo i punti notevoli già derivabili dagli assi?

Se copre solo midpoint/vertex canonici, NON scrivere che supporta anchor arbitrari. Registra il delta su #1239 e lascia l'esatta granularità come decisione d'authoring.

---

# PASSO 4 — Riusa #1239, non creare una issue radiale

#1239 è già l'owner di:

- `InteriorWalls`;
- segmenti che attraversano il centro;
- conservazione anche quando chiudono bordi;
- movement blocking da D-179;
- ComputeHash;
- migrazione formato.

Aggiorna #1239 con il requisito `Center → SideAnchor`.

Non cambiare la decisione D-179 di straforo.

Aggiungi DoD solo per il delta misurato:

- rappresentazione discreta di SideAnchor;
- round-trip;
- validator;
- preservazione del segmento quando tocca un edge;
- occupancy invariata se non c'è footprint chiuso;
- determinismo/hash;
- test di mutazione.

Se serve un nuovo formato, dichiararlo e testarlo; non approssimare.

---

# PASSO 5 — GEO-4: LOS intra-cella resta separata

#1239 dice esplicitamente che movement != LOS.
#1467 ribadisce che la LoS attraverso frontiere interne non è già decisa.

Controlla `docs/OPEN_DECISIONS.md`:

- se `GEO-4` esiste, aggiorna il trigger/owner;
- se manca, aggiungilo come decisione aperta;
- non implementare LoS intra-cella senza decisione.

Se D-225 / #1467 rende GEO-4 necessario per il gate v0.1, allora apri una issue esecutiva dedicata DOPO aver scritto la decisione. Altrimenti resta decisione rinviata.

---

# PASSO 6 — #1467 / Fog of War

Non creare una nuova issue.

Preserva il commento GrayKit già presente:

```text
veil: Cell + Partial
hide: Cell + Partial + Full
Full height: 0.88 H
```

Chiudi/scrivi la decisione D-225 prima di produrre `GB_FOW_CellFull`.

Smoke != Fog of War.

Il renderer deve leggere Team Knowledge; non costruire una seconda source of truth.

---

# PASSO 7 — U25 / #1095

Estendi la scena di validazione con:

- Cell Placement Volume;
- unità;
- Low/High cover;
- porta e stati già canonici;
- acqua/ghiaccio;
- cella esagonale reale;
- visualizer occupancy 12+core;
- esempio di binding su segmento interno;
- FoW proxy permessi da D-225.

U25 deve chiudere:

- GBX-1 Safe Placement inset;
- GBX-5 unit visual footprint.

Valida a camera:

- close;
- gameplay;
- tactical;
- scala di grigi dove richiesto.

---

# PASSO 8 — Issue reconciliation

Applica solo dopo l'audit:

1. #1174 → close completed se la prova su main è ancora vera.
2. #324 → titolo `[EPIC v0.1] E23 · Muri, porte e interaction graph` se label/body restano v0.1.
3. #286 → linked GrayKit v0.1 execution block.
4. #1094 → non riaprire D-171/172/173/163; residui GBX-1/5 → U25.
5. #712 → niente nuovo ghost/snap; collega residuo runtime a #1239.
6. #1239 → Center→SideAnchor.
7. #1467 → decisione D-225 + proxy consentiti.
8. #956 → resta board grammar gate.

Non creare issue per simmetria.

---

# PASSO 9 — Validation

Esegui e registra:

- geometry grammar suite;
- occupancy suite;
- hash/migration tests toccati da #1239;
- Team Knowledge/FoW tests toccati da #1467;
- U25 PIE;
- board PIE;
- packaged smoke per EditorOnly;
- docs consistency check disponibile su HEAD.

Verifica manualmente:

- nessun `220` trattato come `LayerHeight`;
- nessun `12 sectors` trasformato in 12 directions;
- nessun wall open segment trasformato in occupancy volume;
- nessun vecchio Feature Registry reintrodotto come authority.

---

# PASSO 10 — Chiusura del consolidamento

Quando roadmap/spec/issue sono allineate:

- archivia o elimina il precedente file standalone GrayKit roadmap;
- non lasciare due source of truth;
- aggiorna Wiki solo come spiegazione/indice verso gli owner;
- proponi commit separati docs / issue reconciliation / runtime / assets.

Commit suggeriti:

1. `docs(graykit): consolidate v0.1 into canonical owners`
2. `docs(geometry): specify center-to-side internal boundaries`
3. `docs(perception): bind graykit fog proxies to team knowledge`
4. `chore(roadmap): reconcile graykit v0.1 issue tracking`
5. codice/test #1239 separati.

