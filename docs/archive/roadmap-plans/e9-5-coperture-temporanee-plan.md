# E9.5 — Pannello cinetico e coperture temporanee · piano

> `HISTORICAL` · **Piano eseguito** · **Data**: 2026-08-09
> Epic **E9** e issue [`#73`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/73) sono
> **chiuse**: questo documento e' il piano di un lavoro atterrato, e si legge per la provenienza.
> La regola vive in [`../../gameplay/`](../../gameplay/) e nel Feature Registry.
>
> Issue [`#73`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/73) · epic **E9** (`#23`) ·
> branch `feat/73-coperture-temporanee` · piano scritto il **2026-08-09**, prima del codice.
> Chiude l'ultima casella aperta di E9.

## 1. Obiettivo

`Bastion.KineticPanel` e `Bastion.Reconfigure` smettono di essere identità a `Effects` vuoto: creano e spostano
una **copertura bassa temporanea**, che scade nel Cleanup e la cui rimozione aggiorna vista e grafo.

## 2. Stato verificato sul branch (non a memoria)

| Fatto | Dove | Conseguenza |
|---|---|---|
| `FRTHexCover{Edge, Type, Integrity}` esiste, per **bordo**, formato v3 | `Map/RTHexCellData.h` | il dato c'è: non serve un nuovo formato mappa |
| `CoverBetween` legge **entrambe** le facce del bordo | `Map/RTHexCoverLibrary.cpp` | dichiarare una sola faccia basta |
| `ApplyStructureDamage` scala l'integrità e rimuove a 0, passando da `AddOrUpdateCell` | idem | la revisione (→ cache e path) si incrementa già di lì |
| **Nessuna nozione di durata** per le coperture | — | è il pezzo che manca |
| `FRTDynamicSurface`/`TickDynamicSurfaces` (E8.4) e `FRTDynamicArc`/`TickDynamicArcs` (E9.4) | `Turn/RTTurnManager.*` | il pattern di temporaneità esiste **due volte**: la terza lo segue, non lo reinventa |
| `Action.CreateCover` **non** è nel catalogo core, e un test asserisce che non ci sia | `Ability/RTCatalogLibrary.cpp`, `Tests/RTEnvironmentActionTests.cpp:307` | il test va aggiornato, non aggirato |
| Targeting per **cella** già disponibile (`PlannedAttackCell` + `bAttackTargetsCell`) | `Unit/RTUnit.h:132` | il limite che il catalogo dava per scontato non c'è più (chiuso a E8.3) |
| `ResolvePrep` forza `TargetUnitId = i` e `TargetCell = Unit->Cell` | `Turn/RTTurnManager.cpp:1142` | le azioni di Prep agiscono **solo** su chi le usa: serve un ramo dedicato |
| **Nessuna variante di abilità è consumata a runtime** in tutto il progetto | `grep Variants` → solo catalogo, validator e test | i `Parameters` di rinforzato/adattivo sono dati che nessuno legge |
| Nessun catalogo equipaggiamento, nessun loadout | `Ability/RTEquipmentData.h` esiste, popolato da nessuno | `Gadget.PortableCover` non ha oggi un modo di essere usato |

## 3. Decisioni prese all'apertura (autore, 2026-08-09)

**D-a · La copertura si erige in `Prep`, e il catalogo core si allinea.**
`RT_HeroCatalog` e il codice dichiarano già `Preparation` con motivazione esplicita («eretto nel Blast
arriverebbe dopo aver incassato»), e un test lo fissa. `RT_ActionCatalog` dice `Blast` per
`Action.CreateCover`: **prevale la fase di Prep** e si corregge il catalogo azioni.
*Perché non vale il precedente di E9.4* (che portò `ModifyArc` nel Blast «perché la topologia cambia tutta
nello stesso momento»): una copertura **bassa** non tocca il grafo né la vista — riduce il danno. Non c'è
nessun percorso da invalidare, quindi la ragione dell'uniformità topologica non la riguarda.

**D-b · Portata 3, con il bordo dichiarato nel piano.**
Si tiene il valore del catalogo azioni (`Range 3`). Con una cella non adiacente il bordo **non è derivabile**
dalla coppia (caster, bersaglio), quindi la pianificazione guadagna un dato esplicito: *quale dei sei bordi*.
Il catalogo eroi va allineato (oggi `KineticPanel` ha `Range 1`), insieme ai suoi test.

**D-c · `Gadget.PortableCover` entra in questo checkpoint.**
Comporta il minimo indispensabile di E7 — non l'epic: una voce di catalogo equipaggiamento con la sua
`Drawback` obbligatoria e un modo perché un'unità la usi. Il loadout completo, gli slot e la validazione
restano a `#61`/`#63`.

**D-d · Corollario di D-c, non deciso a parte: serve una variante attiva.**
La seconda voce del DoD (rinforzato 45/1 turno · adattivo 25 + una rotazione) è oggi **non verificabile**:
nessuno legge `Variants`. Si aggiunge la selezione minima — una variante attiva per unità — che rende i
`Parameters` consumabili. Il resto della configurazione (chi la sceglie, quando, con quale UI) resta a E7.

## 4. Fette

Ognuna è compilabile, con i suoi test, e si committa solo a suite verde.

| # | Fetta | Test |
|---|---|---|
| **F1** | Copertura temporanea nel modello: `FRTDynamicCover`, `TickDynamicCovers` nel Cleanup, `AddCover`/`RemoveCover` in `URTHexCoverLibrary`, outcome `CoverCreated`/`CoverExpired` nel TurnLog | `Structures.KineticPanel.TemporaryCover`, `Structures.KineticPanel.ExpiryUpdatesLOS` |
| **F2** | `Action.CreateCover` nel catalogo core (fase Prep, portata 3, cooldown 2) + ramo in `ResolvePrep`: portata **validata**, bordo dal piano, non sovrapponibile, fallback `Cancel` con la sua voce | `Actions.CreateCover.{RejectsOutOfRange, RejectsOccupiedEdge}` |
| **F3** | `Bastion.KineticPanel` cablato al core come `Bastion.Ram` lo è a `Action.Charge`; variante attiva che rende consumabili integrità e durata | `Heroes.Bastion.{KineticPanelCreatesCover, VariantParametersApplied}` |
| **F4** | `Bastion.Reconfigure`: sposta una copertura temporanea **senza duplicarla**, conservando integrità e durata residua; rotazione gratuita dell'adattivo | `Heroes.Bastion.ReconfigureDoesNotDuplicate` |
| **F5** | `Gadget.PortableCover`: voce di catalogo con `Drawback`, e uso in partita che riusa la semantica core di F2 | `Equipment.PortableCover.CreatesCover` |
| **F6** | Documentazione: spec del checkpoint, Decision Log (D-a), cataloghi balance allineati, roadmap, feature registry, issue | `scripts/check-docs-links.py` |

### Regola di durata (F1)

Il pannello eretto in **Prep** del turno *N* con durata 2 protegge il Blast di *N* **e** quello di *N+1*, e
sparisce nel Cleanup di *N+1*. Quindi il tick **decrementa già nel turno di nascita** — al contrario del ponte
di E9.4, che salta il proprio turno perché nasce nel Blast, a valle della fase che lo userebbe. Le due regole
divergono per la fase in cui nascono, non per capriccio: va scritto nel codice e nella spec.

## 5. Rischi

- **Ambiguità di `Reconfigure`**: se la cella indicata porta più di una copertura temporanea, quale si sposta?
  Si rifiuta con reason code invece di sceglierne una per ordine di array — una scelta implicita che il
  giocatore non può dedurre guardando il campo è peggio di un rifiuto leggibile.
- **Scope che sconfina in E7** (D-c, D-d): il confine dichiarato è *usare* un gadget e *una* variante attiva.
  Slot, loadout e validazione del set restano fuori; se il codice inizia a chiederli, la fetta si ferma e la
  parte restante torna a `#61`.
- **`Content/` non è nel worktree** (è fuori dal repo): i test che costruiscono la mappa in memoria non ne
  risentono, ma nessuna verifica PIE è eseguibile da qui — le voci editor restano ⏳ in `test-manuali-pie.md`.
- **Il conteggio dei test non si copia dalla roadmap**: si misura sul branch prima di dichiararlo.

## 6. Fuori scope, dichiarato

Copertura **alta** temporanea (E9.2 la tratta come dato di mappa, e nessuna azione del catalogo la crea) ·
targeting per bordo nell'**HUD** (E11) · slot e validazione del loadout (`#61`, `#63`) ·
`BreachCharge` (`#61`).
