# Cell → Sector12 → Edge6 → Shared Edge — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma il kit
> *«Claude Task — RefactorTactics: consolidare Cell → Sector12 → Edge6 → Shared Edge»*, arrivato come file
> nella radice del checkout di lavoro.
>
> **Data**: 2026-08-29 · **Base**: `origin/main` @ `24cfe99a` · **Modo**: critique · **Focus**: requirements + architecture
>
> **Cosa è**: il verdetto su un **mandato di scrittura su GitHub** — tre checkpoint da creare sotto
> [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324). `/sc:spec-panel` è task
> documentale ([`CLAUDE.md`](../../../CLAUDE.md) §6): **nessuna issue è stata creata, chiusa o modificata**,
> **un** `D-nnn` è stato assegnato — `D-243`, e non a una tesi del kit: vedi §8 — e le azioni GitHub
> raccomandate stanno in §7 come elenco, non come lavoro fatto.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge dall'owner
> ([`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E23, il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md),
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)), **ha ragione l'owner**.
>
> **Archiviato in**: [`../../archive/src/handoff/2026-08-29-cell-sector12-edge6-issues.md`](../../archive/src/handoff/2026-08-29-cell-sector12-edge6-issues.md)

---

## 1. Il verdetto in una riga

> **Il kit chiede tre checkpoint, e la sua stessa §9 li smonta: `CP 23.8` è ≥80% coperto da una issue aperta
> (#1615), `CP 23.9` è già in produzione dentro `EdgesTouchedBy`, e `CP 23.10` chiede di rendere simmetrico
> un bordo che il repository ha reso asimmetrico apposta, in tre punti indipendenti che citano il perché.**

La geometria del kit è **giusta** — `EdgeIndex = SectorIndex / 2` regge, e la tabella §4 è corretta su tutte e
sei le righe. È la mappa del lavoro a essere scaduta: due delle tre dipendenze dichiarate sono chiuse dal 14
agosto, il censimento dei significati di «settore» ne conta tre su **quattro** già a runtime, e la domanda che
`CP 23.10` apre ha già una risposta canonica — **nominale**, non geometrica.

Il contributo che vale il consumo è in §6: una lacuna reale che il kit sfiora senza nominarla.

---

## 2. Base di misura

Misurato su albero e lato server, non ricordato.

```text
Repo       : DegrassiAaron/refactor-tactics-main
Base       : origin/main @ 24cfe99a  (git fetch --prune eseguito). La revisione e' stata condotta su 059c2eaa
             e RIALLINEATA a 24cfe99a: quattordici commit sono atterrati durante il lavoro, e toccano quattro dei
             file che questo referto cita. Le righe qui sotto sono state riverificate DOPO il riallineamento.
Worktree   : D:/Repositories/rt-wt-sector12, branch docs/spec-panel-cell-sector12-edge6, albero pulito
Sessione   : il checkout di lavoro è su feat/1535-velo-in-partita, che diverge da main: non ci si scrive
Milestone  : v0.1 in sei tranche — 66 aperte / 112 chiuse
Epic #324  : OPEN · milestone «v0.1 · Mondo giocabile» · label v0.1, epic, P1
Suite      : NON eseguita. Nessuna riga di codice toccata, quindi niente da misurare (CLAUDE.md §7, D-222)
```

Il kit non nomina un `HEAD`: dice *«se il repository è cambiato dopo questo documento, vince lo stato corrente
misurato»*, ed è la sua clausola migliore — questo referto la applica alla lettera.

---

## 3. Il panel

| Voce | Perché al tavolo |
|---|---|
| **Karl Wiegers** | tre DoD scritte in checkbox: quali sono verificabili e quali descrivono lavoro già fatto |
| **Martin Fowler** | l'identità di un bordo: value type, canonicalizzazione, seconda authority |
| **Sam Newman** | confini fra i modelli — grafo, geometria, presentazione — e chi possiede cosa |
| **Gojko Adzic** | gli esempi del kit contro gli esempi che il codice esegue già |
| **Alistair Cockburn** | chi è il consumatore di ciascun checkpoint, e se esiste |

---

## 4. Rilievi, per checkpoint

Tredici rilievi. Ognuno cita il simbolo o l'issue che lo regge.

### 4.1 `CP 23.8` — indirizzo canonico Cell + Sector12

**🔴 F-01 · È ≥80% coperto da [#1615](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1615), che
è aperta e in v0.1.** *(Wiegers)* — La §9 del kit dice: *«Se trovi una issue che copre già ≥80% di uno dei
checkpoint: NON crearne una duplicata»*. #1615 (`CP 11.8 — Il settore sotto il cursore`, milestone
*v0.1 · Leggibilità*, parent #705) non solo copre lo scope: la sua sezione dichiarata *«più importante
dell'implementazione»* è **esattamente** il *Why* del `CP 23.8` — nominare la relazione fra i significati di
«settore» perché *«un lettore che incontra `Sector` in una firma sappia quale delle tre cose sta leggendo»*.
Perfino l'avvertimento del kit — *«non inventare `FRTSector12`, `FRTDirection6`»* — è già scritto in #1615,
con la misura accanto (zero occorrenze in `Source/`, riverificata qui).

**🔴 F-02 · Il censimento del kit è incompleto: le semantiche sono quattro, non tre.** *(Newman)* — La tabella §2 del kit ne elenca
tre. Ne manca una **già a runtime**:

| Nome | Card. | Semantica | Dove |
|---|---:|---|---|
| `ERTHexDirection` | 6 | direzioni tattiche del grafo | `Map/RTCellId.h:11-19` |
| `HandleFacingSector(ERTHexDirection)` | **6** | il «settore» del **facing** | `Player/RTPlayerController.cpp:1718`, dichiarata in `.h:392` |
| `RT_OccupancySectorCount` | 12 | quanto la cella è invasa da geometria | `Map/RTHexOccupancyLibrary.h:8` |
| settore sotto il cursore | 12 | puntamento locale | #1615, **non implementato** |

La riga `HandleFacingSector` ha già un test (`RefactorTactics.PlayerInput.FacingSectorProducesPlannedFacing`,
`Tests/RTPointerInteractionTests.cpp:448`) ed è citata dall'harness (`ScenarioHarness/RTScenarioSession.cpp:129`):
è un «settore» a **sei** in produzione, che il kit non conta.

⚠️ **Questa tabella non è una scoperta di questo referto, ed è giusto dirlo**: #1615 la porta già, con la
stessa riga e lo stesso `file:line`. Il rilievo è che il **kit** ne elenca tre di quattro — non che nessuno
l'avesse misurato. ⛔ **E il kit non introduce una quinta semantica**: il suo `LocalSector ∈ [0..11]` riusa i
dodici dell'occupancy, che è precisamente il motivo per cui `F-01` lo dà coperto da #1615. La prima stesura di
questo rilievo diceva *«la sua sarebbe la quinta»* e contraddiceva `F-01`: le due cose non possono valere
insieme, e a valere è `F-01`.

**🔴 F-03 · Il dominio è conteso da una decisione che il repository tiene aperta apposta.** *(Cockburn)* —
[#1606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1606) (`[DESIGN] End Placement e
PlacementSector`, stato `PROPOSED`, owner [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)) tiene aperte sette
domande, e due colpiscono il `CP 23.8` in pieno:

- `PLC-1` — *`Placement` e `Facing` possono divergere?* Se coincidono, lo spicchio **è** il facing a sei;
- `PLC-5` — *un gruppo di spicchi è UI o entità logica?* Se è logica, *«`Placement` non è più un intero piccolo»*.

Un `CellSectorAddress = FRTCellId + LocalSector ∈ [0..11]`, con equality, hash e debug string come la DoD
chiede, **decide `PLC-5` per costruzione** e pregiudica `PLC-1`. #1606 esiste con parole proprie per impedirlo:
*«aprire l'implementazione prima significherebbe far decidere l'inerzia»*.

### 4.2 `CP 23.9` — relazione Sector12 ↔ Edge6

**⛔ F-04 · La RELAZIONE è già incisa nel codice e in uso; la FUNZIONE che il kit chiede non esiste, e la
distinzione cambia la taglia del lavoro, non il verdetto.** *(Adzic)* —
`URTGeometryBakeLibrary::EdgesTouchedBy` (`Map/RTGeometryBake.cpp:167-176`) percorre i sei bordi, prende
`Boundary[(2*Edge) % RT_OccupancySectorCount]` e `Boundary[(2*Edge+2) % …]`, e chiude con
`URTHexLibrary::DirectionForEdgeIndex(Edge)`: è la stessa corrispondenza `2k`/`2k+1` che il kit propone,
**percorsa nel verso opposto** — da bordo a settori, non da settore a bordo — con venti righe di commento che
ne spiegano il perché, e coperta da `BakeCoverLandsTowardTheNeighbour`, nato apposta perché i test precedenti
usavano solo `E` e `W`.

🔴 **La prima stesura di questo rilievo scriveva «la conversione è già in produzione», ed era imprecisa**: la
funzione `SectorIndex → EdgeIndex` **non esiste**. Misurato: `SectorIndex` ha **zero** occorrenze in `Source/`,
e nessun simbolo la nomina. Ciò che è in produzione è la relazione, non la sua direzione.

✅ **Il verdetto regge, e anzi si rafforza**: il delta reale è **una funzione pura di una riga** più i suoi
test — non un checkpoint d'epic — e la relazione da cui deriva è già scritta, testata e motivata in un punto
solo. ⚠️ E oggi non ha un consumatore: nessuno chiede *«quale bordo tocca il settore `s`»* —
`FRTOccupancyMask::Sectors` è letta come maschera, non come indice. Vedi §8: `D-243` cambia questo, ma solo
come ragione, non come chiamante.

**✅ F-05 · La tabella §4 è corretta, ed è la sesta riga a dirlo.** *(Fowler)* — `DirectionForEdgeIndex(k) =
(6 − k) % 6` (`Map/RTHexLibrary.cpp:385-393`) sull'enum `E, NE, NW, W, SW, SE` (`Map/RTCellId.h:11-19`)
produce `0→E, 1→SE, 2→SW, 3→W, 4→NW, 5→NE`: **le sei righe del kit, in ordine**. ⚠️ Ma è un'esattezza
fragile, e il kit lo sa: scrive *«non incidere manualmente questa tabella»* e poi la incide, due volte — nel
§4 e nella DoD. È la forma esatta di [#712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712),
il difetto che ha generato queste due funzioni: *«due copie della stessa formula sono esattamente il modo in
cui il difetto è nato»* (`Map/RTHexLibrary.h:181-183`).

**✅ F-06 · `EdgeIndex = SectorIndex / 2` regge, e per una ragione più forte di quella dichiarata.** — Non è
una convenzione scelta: è la **costruzione** di `SectorBoundaryPoints` (`Map/RTHexOccupancyLibrary.cpp:88-102`),
che alterna vertice e punto medio del lato — indici **pari** a raggio `HexSize` e angolo `60k − 30`, indici
**dispari** a raggio inscritto e angolo `60k`. Il bordo geometrico `k` va da `HexCorners[k]` a
`HexCorners[k+1]`, cioè da `−30 + 60k` a `−30 + 60(k+1)` (`Map/RTHexLibrary.cpp:373-381`): i settori `2k` e
`2k+1` **sono** le sue due metà. La formula è un teorema sulla costruzione, non un accordo — e va testata
derivandola, come `EdgeIndexMatchesNeighbourDirection` già fa per l'altro ponte.
⚠️ **Corollario che il kit non dice**: i dodici settori **non sono equidistanti dal centro**. «Dodici spicchi
da 30°» descrive l'angolo; la forma è un ventaglio di triangoli `centro–vertice–puntomedio` a raggio
alternato. Chi disegnasse dodici raggi uguali disegnerebbe un'altra cosa.

### 4.3 `CP 23.10` — identità del bordo condiviso

**🔴 F-07 · Il tipo esiste già, col nome che il kit dice di cercare.** *(Fowler)* —
`FRTStructureEdgeRef { FRTCellId Cell; ERTHexDirection Edge; }` con `operator==`
(`Map/RTStructureIdentityLibrary.h:17-33`), documentato con la **stessa** convenzione che il kit propone —
*«visto DALLA cella … perché un bordo scritto su una faccia sola resti citabile senza dover indovinare quale
delle due l'ha ricevuto»*. Il kit ordina: *«Non introdurre un nome nuovo se esiste già un tipo semanticamente
equivalente»*. Esiste.

**🔴 F-08 · L'asimmetria non è un difetto: è la scelta, e il repository la motiva in tre punti indipendenti.**
*(Newman)* — Il kit assume che *«se equality/hash li trattano come due oggetti diversi … porte, cover,
interaction graph, cache e future transition possono divergere»*. Misurato, è il contrario:

1. **La copertura è per faccia.** `FRTHexCover::Edge` — *«`W` protegge dai colpi che entrano dal vicino a
   ovest»* (`Map/RTHexCellData.h:62-64`). Un muro può riparare una cella e non l'altra; un'identità
   ordine-indipendente non può possedere quel dato.
2. **L'arco è direzionale e porta dati per verso.** `FRTHexEdge` ha `Cost`, `Kind`, `State`, `Integrity`
   (`Map/RTHexCellData.h:404-427`), e `RemoveTransition(From, To, bBothDirections)`
   (`Map/RTHexMapAsset.h:362`) tratta la simmetria come **parametro dell'operazione**, non come proprietà
   dell'identità.
3. **L'assenza di un ID di bordo è deliberata e scritta.** `Turn/RTTurnLog.h:758-760`: *«Non esiste un
   `TransitionId` che lo accompagni, ed è deliberato: `FRTHexEdge` è `From`/`To`/`Cost`/`Kind` **senza ID**,
   perché nel progetto **l'identità di un bordo È la coppia di celle**»*. Ripetuto sul dato:
   *«un arco è identificato dalla coppia `(From, To)` e nient'altro»* (`Map/RTHexCellData.h:440-441`).

**🔴 F-09 · L'identità condivisa esiste già, ed è nominale — e una seconda sarebbe vietata dal doc comment del file
che la porta.** *(Fowler)* — `CP 23.3` / [#832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/832)
ha già risolto *«due descrizioni della stessa struttura»* con `StableId`: `FRTStructureArcRef` documenta
*«un ponte bidirezionale ne produce DUE, che portano lo stesso nome perché sono una struttura sola»*
(`Map/RTStructureIdentityLibrary.h:36-39`), e `FRTHexEdge::StableId` aggiunge *«è l'unica condivisione di nome
ammessa fra archi»* (`Map/RTHexCellData.h:438-446`). Il doc comment della libreria dichiara, a `RTStructureIdentityLibrary.h:67`,
*«⚠️ Nessuna seconda authority»* — la frase parla di chi **muta** una porta, e questo referto la estende a chi
**identifica** un bordo: è un'analogia, non una citazione letterale. Una `SharedEdgeAddress` canonica sarebbe la
seconda risposta alla stessa domanda, introdotta nel modulo che tiene quella disciplina.

**🔴 F-10 · Il modello proposto copre metà del dominio che dichiara di preparare.** *(Newman)* —
`SharedEdgeAddress = canonical(CellA, DirA)` è definita su `Neighbor(A, Dir)`, quindi vive sui **sei bordi
planari**. Le transizioni che il grafo usa davvero per il multilivello — `URTHexMapAsset::Transitions`,
*«archi verticali/speciali: scale, rampe, ponti, tunnel, ascensori»* (`Map/RTHexMapAsset.h:222`) — **non
hanno una `ERTHexDirection`**: sono coppie `(From, To)` fra layer, e nessun `Neighbor` le produce. Il kit
presenta la Shared Edge come *«base per la futura transition»* (`E23.7`): non lo è. È un modello parallelo che
esclude proprio i casi per cui `E23.7` esiste. ✅ Di converso, la DoD *«test su layer diversi: non
collidono»* è già vera per costruzione: `FRTCellId` porta `Layer` e *«celle su layer diversi NON sono
adiacenti»* (`Map/RTCellId.h:22-25`).

### 4.4 Rilievi trasversali

**⚠️ F-11 · Le dipendenze dichiarate non sono «da verificare»: sono chiuse tutte e tre.** — #619 (chiusa il
2026-08-12), #620 (2026-08-13) e #621 (2026-08-13, `closedAt` — non il 14, che è la data di ultimo
aggiornamento e non di chiusura) sono `CLOSED`, e #324 le registra come tali. Il kit le presenta come
*«stato già esistente da verificare»*: verificarle va fatto, ma il loro esito non è aperto.

**⚠️ F-12 · Aggiungere tre checkpoint tocca un owner documentale — e il puntatore di #324 indica quello
sbagliato.** — Il corpo di #324 si apre con *«Owner documentale: `docs/roadmap/roadmap-post-v0.1.md` § E23»*,
ma quel documento, alla sua sezione `E23`, dichiara *«⛔ **E23 NON È PIÙ DI QUESTA RELEASE** — anticipata alla
v0.1 il 2026-08-17»* (`D-160`). La tabella `E23.1`–`E23.7` viva è in
[`roadmap-v0.1.md`](../roadmap-v0.1.md) § E23, con DoD e stato per checkpoint. `23.8`/`23.9`/`23.10` sono
**liberi** — nessuna collisione — ma chi seguisse il puntatore di #324 scriverebbe il checkpoint nuovo nel
documento che disclaima l'epic, lasciando stale l'owner di release: esattamente la divergenza che
`DOC_CONFLICT_MATRIX` esiste per registrare. ➕ **Il puntatore stale di #324 è un difetto suo, non del kit**,
e vale una riga di correzione indipendente da tutto il resto.

**⚠️ F-13 · La tabella di output §12 presuppone la scrittura.** — Pretende `created/reused/updated` per tutti
e tre i CP. `/sc:spec-panel` è task documentale (`CLAUDE.md` §6): la colonna corretta, oggi, è in §7.

---

## 5. Cosa sopravvive

- ✅ **La disciplina anti-duplicazione (§9, §10) è la parte migliore del kit** — e il suo primo bersaglio è il
  kit stesso: F-01, F-04 e F-07 escono tutti e tre dalla sua §9 applicata alla lettera.
- ✅ **«I 12 settori non sono direzioni del grafo»** è vero, va ripetuto, ed è già la tesi di #619 e #1615.
  #712 è il prezzo pagato quando la si dimentica.
- ✅ **La convenzione geometrica non va ridecisa** (§3): `SectorBoundaryPoints` è l'autorità, e il kit lo
  riconosce invece di riaprirla.
- ✅ **`Layer` nell'identità, niente float, niente pointer, regola nel modulo runtime** (§8): coincidono con
  invarianti già scritti — nessun conflitto, nessuna novità.
- ✅ **L'ordine dei tre passi è giusto come ordine di dipendenza** — address → conversione → identità condivisa
  → transizione — anche dove i singoli passi sono già coperti. È una buona mappa di un territorio già in parte
  costruito.
- ✅ **Il divieto di aprire la Transition Key adesso** (§5, *Out of scope* di `CP 23.10`) è corretto e coerente
  con `E23.7` `DESIGNED`.

---

## 6. La domanda che resta aperta, e non è un checkpoint

`StableId` risolve l'identità condivisa **per le strutture che ne hanno una**, ed è un `FName` **autorato**:
`UPROPERTY(EditAnywhere)` su `FRTHexDoor` e `FRTHexEdge`, con `NAME_None` trattato come *«un campo lasciato
indietro»* (`Map/RTStructureIdentityLibrary.h`, `ValidateReferences`). Ne segue che **un bordo senza nome non
ha identità condivisa** — ha due facce e, se è un arco, due versi.

Finché il consumatore è l'interaction graph (`CP 23.4`, #833), la lacuna non morde: si cita ciò che ha un nome.
Se `E23.7` dovrà citare una transizione **priva** di nome autorato — una clearance calcolata, una cache di
percorso, un evento di TurnLog che parla di un bordo anonimo — allora serve una chiave derivata, e la scelta
sarà fra *derivarla dalla coppia ordinata* e *rendere `StableId` obbligatorio in cottura*.

**È una domanda per `E23.7`, non un checkpoint da aprire oggi**, e il kit ha ragione a dire che la Transition
Key viene dopo. Il suo errore è mettere prima un livello — la Shared Edge — che il repository ha già superato
per via nominale.

---

## 7. Azioni GitHub raccomandate — **non eseguite**

Elenco, non lavoro fatto. La scrittura su GitHub è decisione dell'autore.

| # | Azione | Perché |
|---|---|---|
| 1 | **Non creare `CP 23.8`.** ⚠️ **E non aggiungere a #1615 il censimento dei significati: ce lo ha già**, con la stessa riga `HandleFacingSector` e lo stesso `file:line`. L'unica aggiunta che non duplica è il **rimando a #1606**, che #1615 non cita | F-01, F-02, F-03 |
| 2 | **Non creare `CP 23.9` come checkpoint.** Se serve la funzione pura `SectorIndex → EdgeIndex`, è una issue piccola sotto #1615 o un commento in `RTHexOccupancyLibrary.h`, con test derivato e non inciso | F-04, F-05 |
| 3 | **Non creare `CP 23.10`.** La domanda che porta è di `E23.7`; la risposta corrente (`StableId`) va **citata** nel corpo di #324 perché il prossimo kit non la riscopra | F-07, F-08, F-09, F-10 |
| 4 | Se l'autore vuole comunque tracciare la lacuna di §6, **una** issue `DESIGN` sotto `E23.7`, in `OPEN_DECISIONS.md`, con prefisso proprio — non tre checkpoint | §6 |
| 5 | Ogni aggiunta di checkpoint a #324 va scritta **anche** in [`roadmap-v0.1.md`](../roadmap-v0.1.md) § E23 — **non** in `roadmap-post-v0.1.md`, che per `E23` dichiara *«NON È PIÙ DI QUESTA RELEASE»* | F-12 |
| 6 | **Aggiornare #1606**: dopo `D-243` il suo titolo promette *«sette domande»* mentre cinque sono chiuse, e il suo owner (`OPEN_DECISIONS.md`) lo dice già. Non si chiude: restano `PLC-3` e `PLC-4` | §8 |

⛔ **Nessuna issue creata o modificata.** ➕ **Un `D-nnn` è stato assegnato, ma non a una tesi del kit**:
nessuna delle sue tesi era una decisione nuova — coincidono con decisioni esistenti o cadono contro di esse.
`D-243` nasce invece dal §8: è la risposta a `PLC-1` di [#1606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1606),
presa nella stessa sessione perché era il **prerequisito** che questo referto indicava, non un suo esito.

---

## 8. Prossimo passo — **eseguito in parte lo stesso giorno**

Il passo indicato era: *«chiudere prima #1606 (`PLC-1`, `PLC-5`), poi implementare #1615»*. La prima metà è
fatta.

✅ **`D-243` (2026-08-29) chiude cinque delle sette `PLC-*`**: la posa nella cella **non è uno stato di
simulazione** — i dodici spicchi sono presentazione, e ciò che entra nella regola è il facing a sei che ne
deriva (`SectorIndex / 2` → `DirectionForEdgeIndex` → `FacingFinalAfterMove`). Cadono per corollario `PLC-2`,
`PLC-5`, `PLC-6` e `PLC-7`; restano aperte `PLC-3` e `PLC-4`, che riguardano la geometria della cella e non la
posa. Owner: [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) § *Chiuse il 2026-08-29 da `D-243`*.

➕ **E cambia un verdetto di questo referto, ma meno di quanto la prima stesura scrivesse.** `F-04` dava la
conversione *«senza un consumatore»*, e questa riga diceva che con `D-243` *«il consumatore esiste»*. **Non
esiste**: una decisione non produce un chiamante, e #1615 è tuttora `OPEN` e non implementata. Ciò che `D-243`
consegna è la **ragione** per cui quel chiamante nascerà — il traduttore fra il settore sotto il cursore e il
facing dichiarato — dove il giorno prima non ce n'era nessuna. Il rilievo `F-04` resta valido su ciò che
diceva, e la funzione pura resta lavoro non ancora sbloccato.

🔴 **Una correzione a §4.1**: `F-01` resta vero — `CP 23.8` è coperto da #1615 — ma la riga che ne faceva
discendere *«#1615 va sbloccata prima»* era imprecisa. **#1615 non era bloccata** da `PLC-1` finché resta
hover di presentazione, com'è scritto nel suo scope: lo diventa nel momento in cui quello spicchio *dichiara*
un facing.

Resta da fare: implementare #1615, e — solo se `E23.7` produrrà un consumatore senza nome autorato — riaprire
la domanda di §6 con quel consumatore in mano, non prima.

Il kit chiude dicendo *«il repository corrente resta l'autorità»*. Applicata, quella frase è il verdetto.
