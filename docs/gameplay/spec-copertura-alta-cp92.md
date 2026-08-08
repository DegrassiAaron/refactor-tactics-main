# Copertura alta e distruzione — CP 9.2

> 📌 **Stato di implementazione storico al 2026-08-08 (CP 9.2).** Numeri, conteggi di test ed esiti qui sotto fotografano
> la chiusura del checkpoint. Lo **stato corrente** è posseduto da
> [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md): questa spec non compete con la roadmap come
> fonte di stato.

> **Stato**: chiuso il 2026-08-07 · **Issue**: [#70](https://github.com/DegrassiAaron/refactor-tactics-main/issues/70) · **Epic**: E9 (#23) · **Segue**: [CP 9.1](spec-copertura-cp91.md)
> **Codice**: `Map/RTHexCoverLibrary.*` (nuovo), `Map/RTHexCellData.h`, `Map/RTHexVisionLibrary.cpp`, `Pathfinding/RTHexPathLibrary.cpp`, `Combat/RTHexCombatLibrary.*`, `Turn/{RTActionEvent.h, RTActionEffectLibrary.cpp, RTTurnLog.h, RTTurnManager.cpp}`, `Ability/RTCatalogLibrary.cpp`

## 1. Cosa fa

`ERTHexCoverType::High` nega l'attraversamento del bordo a **vista, passo e proiettili**, ha **integrità 50**
e può essere **abbattuta**: il colpo che le sbatte contro la danneggia, e quando cade vista e grafo si
riaprono — dalla fase successiva, con la revisione della mappa incrementata.

## 2. Le quattro decisioni (prese con l'autore, 2026-08-07, issue #70)

| # | Decisione | Alternativa scartata |
|---|---|---|
| 1 | Il danno a struttura è un **effetto dichiarato**, `ERTActionEffect::DamageStructure` in coda all'enum. `HeavyAttack` = `Damage 35` + `DamageStructure 20` | Un campo `StructureDamage` su ogni `FRTActionEffectSpec`: ~28 azioni su 30 avrebbero portato uno zero privo di significato. Sfondare è una capacità che il catalogo **concede**, non un effetto dell'essere forti |
| 2 | La riapertura vale **dalla fase successiva**: il Blast raccoglie, l'applicazione è a fase conclusa | Riapertura immediata dentro il Blast: l'esito del turno sarebbe dipeso dall'**ordine** dei colpi, contro l'invariante #3 e contro la proprietà di permutazione già testata su `CollectHexAttacks` |
| 3 | La barriera blocca **nei due versi**: `CoverBetween` prende la più alta delle due facce | Direzionale come `FRTHexEdge`: permetteva lo stato assurdo «si passa da B ad A ma non da A a B» con una sola faccia disegnata. Scartata anche la via di degrado «copertura alta per cella», che avrebbe chiuso la DoD a costo quasi zero rinunciando alla direzionalità — l'idea stessa di E9 — e reso la cella non occupabile |
| 4 | Si mira la **cella**, il bordo si deduce dalla linea | La convenzione di `ModifyArc` (coppia caster→bersaglio) **non può funzionare**: un muro alto toglie la vista di chiunque stia dietro, quindi non esiste nessun bersaglio nominabile oltre — la struttura sarebbe stata indistruttibile |

## 3. Architettura: una sola sede per «cosa c'è fra queste due celle»

`URTHexCoverLibrary` nasce perché la stessa domanda la fanno tre aree che non devono dipendere l'una
dall'altra: la vista (`URTHexVisionLibrary`), il grafo di traversata (`URTHexPathLibrary`) e il combat
(`URTHexCombatLibrary`). Con la regola in un posto solo non possono divergere — una copertura che blocca la
vista ma non il passo sarebbe un difetto invisibile finché qualcuno non ci cammina attraverso.

**La LOS cambia semantica**: da «una cella intermedia oscura» a «un bordo attraversato blocca». Conseguenza
non ovvia: contano anche il **primo e l'ultimo passo**, che il ciclo per-cella escludeva con «non ci si copre
da soli». Un muro addossato al bersaglio lo copre davvero, e non è coprirsi da soli: è la barriera davanti a
lui. Le due regole convivono — `bBlocksLineOfSight` resta per le celle (fumo, ostacoli pieni).

**Il colpo che sbatte è quello che danneggia**: la raccolta avviene **prima** del controllo sulla linea di
tiro, perché quello è il caso principale (§2.4). Un intento respinto dalla LOS non colpisce nessuno, ma i suoi
`DamageStructure` arrivano alla barriera che l'ha fermato.

**Raccogli poi applica**: il danno si somma per bordo, con la coppia di celle **normalizzata** (`StableLess`),
così due attaccanti ai lati opposti colpiscono la stessa barriera invece di due mezze barriere. Ogni modifica
passa da `AddOrUpdateCell`, che incrementa la **revisione**: è il segnale con cui `IsSnapshotStale` avverte
snapshot e cache di percorso — cioè ciò che impedisce i path fantasma, l'obiettivo dichiarato di E9.

**TurnLog**: `CoverDamaged` e `CoverDestroyed` in coda a `ERTEnvironmentOutcome`; la coppia
`SrcCell`/`TgtCell` identifica il bordo senza aggiungere un campo direzione, `Amount` porta l'integrità
residua.

## 4. Due difetti trovati dai test, non a tavolino

1. **`ProduceEvents` traduceva anche `DamageStructure`** in un evento su un'unità, che avrebbe applicato il
   danno-muro a chi sta dietro. L'ha trovato `Actions.HeavyAttack.NoEffectIfInterrupted`, che contava due
   eventi dove l'azione ne ha uno solo verso il bersaglio. ⚠️ Il commento di quel file promette che
   «aggiungere un effetto senza tradurlo non compila»: **non è vero** — lo switch senza `default` passa in
   silenzio. Il gate esiste come intenzione, non come vincolo.
2. **`HexUnits[Blocked.TargetId]` usciva dall'array** quando l'intento mira una cella
   (`TargetId == INDEX_NONE`). Difetto **preesistente** di `Fallback.AttackCell`, che questo checkpoint
   rendeva raggiungibile: mirare una cella è il modo in cui si punta una struttura.

## 5. Limiti dichiarati

1. **`HexLine` e i vertici**: se una linea esagonale saltasse da una cella a una non adiacente, `CoverBetween`
   restituirebbe `None` e la barriera non fermerebbe quel colpo (fail-**open**). Non osservato, ma è la sola
   assunzione geometrica non verificata da un test.
2. **Il bot non conosce le barriere**: il suo `ScoreThreatRespectsCover` ragiona su `bBlocksLineOfSight`.
   Cambiare le sue premesse è CP 13.5.
3. **Nessuna azione crea coperture alte**: si disegnano nel data asset. `CreateCover` (bassa, temporanea)
   resta a CP 9.5, `BreachCharge` a #61.
4. **La copertura bassa si distrugge anche lei** (stessa funzione, integrità 30): non era richiesto dalla DoD,
   ma il dato esisteva dal CP 9.1 senza consumatore e il costo era zero.
5. **L'asimmetria fra bassa e alta è voluta**: la riduzione di danno guarda **solo** la faccia del bersaglio
   (un riparo protegge chi ci sta dietro), il blocco guarda **entrambe** (una barriera è fisica). Lo fissa
   `Cover.LowCover.WrongSideNoReduction`, che verifica che una copertura sulla cella dell'attaccante non
   protegga il bersaglio.

## 6. Verifiche

### 6.1 Test (9 nuovi, suite **403/403**)

`Cover.HighCover.BlocksAll` (vincolante) · `Cover.HighCover.LowCoverStaysPassable` ·
`Cover.Destruction.ReopensLOS` (vincolante) · `Cover.Destruction.UpdatesGraph` (vincolante) ·
`Cover.Destruction.PartialDamageLeavesItStanding` · `Cover.Destruction.OrderIndependent` ·
`Cover.Destruction.BumpsRevisionAndHash` · `Cover.Destruction.HeavyAttackDeclaresStructureDamage` ·
`Cover.Destruction.LoggedInPlayedTurn` (in partita vera, con `UWorld`).

### 6.2 Verifiche di mutazione

| Mutazione | Test caduti |
|---|---|
| `AccumulateStructureHit` non normalizza la coppia di celle | `Destruction.OrderIndependent` |
| Il turno non applica il danno alle strutture | `Destruction.LoggedInPlayedTurn` |
| La LOS ignora l'attraversamento del bordo | `HighCover.BlocksAll`, `Destruction.ReopensLOS` |
| Il grafo ignora la barriera | `HighCover.BlocksAll`, `Destruction.UpdatesGraph`, `Destruction.PartialDamageLeavesItStanding` |
| La copertura a 0 non viene rimossa (resta nel dato) | `Destruction.BumpsRevisionAndHash`, `Destruction.ReopensLOS`, `Destruction.UpdatesGraph` |
| `CoverBetween` guarda una sola faccia | 7 test, fra cui `HighCover.BlocksAll` e `LowCoverStaysPassable` |

`Cover.Destruction.HeavyAttackDeclaresStructureDamage` non ha una riga: il difetto che intercetta è stato
osservato **durante** lo sviluppo (`NoEffectIfInterrupted` caduto quando il catalogo ha dichiarato il secondo
effetto, §4.1).

## 7. Cosa apre

CP 9.3 (porte) userà **lo stesso punto di invalidazione** — `AddOrUpdateCell` e la revisione — e la stessa
convenzione «coppia di celle = bordo» nel TurnLog. CP 9.5 aggiunge le coperture temporanee. CP 16.2 farà
annullare la riduzione della copertura **bassa** ai colpi fuori dall'arco frontale: resta una regola additiva,
e la direzionalità del bordo non si fonde con quella del facing.
