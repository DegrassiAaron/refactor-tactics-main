# RT — Catalogo terreni, coperture e strutture v0.1

> **Fonte**: `docs/src/prd/catalogo-e-bilanciamento-v0.1.pdf` §§4–5 · `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf`
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Implementazione**: epic **E8** (terreni, `#64`–`#68`) ed **E9** (strutture, `#69`–`#73`).

## Stato dell'implementazione (aggiornato 2026-08-07)

Le **superfici agiscono**: costi, blocchi di Dash, `Wet`/`Obscured` legati alla cella, `Burning` nel Cleanup,
scivolata sul ghiaccio (CP 8.1/8.2) e — da **CP 8.3** — la **propagazione elettrica** su acqua e superfici
conduttive, che cammina sul grafo delle celle (BFS, massimo 3 passi, ogni unità colpita una volta) e risolve
nel Cleanup prima del danno di `Burning`. Dettaglio in
[`../gameplay/spec-propagazione-elettrica-cp83.md`](../gameplay/spec-propagazione-elettrica-cp83.md).

> ⚠️ **Aggiornato il 2026-08-08.** Il paragrafo che seguiva elencava come «da costruire» cose ormai
> **costruite**. Stato reale:
>
> | Area | Stato |
> |---|---|
> | Interazioni **fuoco/acqua** (CP 8.4) | ✅ fatte |
> | **Terreno dinamico** — cella che cambia superficie a runtime (`CreateWater`, `Ignite`, `Flux.ConductiveNode`) | ✅ fatto |
> | **Coperture direzionali** — «non esistono nel formato dell'asset» | ✅ **esistono**: `FRTHexCover{Edge, Type, Integrity}` in `FRTHexCellData`, con `URTHexCoverLibrary`. Copertura **bassa** (CP 9.1) e **alta** (CP 9.2), con `Integrity` e distruzione |
> | Azioni ambientali (CP 8.5) | vedi roadmap |
> | Porte, ponti, coperture temporanee | ⏳ **da completare** |
>
> Lo stato per checkpoint è della [roadmap](../roadmap/roadmap-checkpoint.md), non di questo catalogo.

---

## 1. Terreni del vertical slice

| TerrainId | Terreno | Costo | Movimento | LOS | Effetto |
|---|---|---:|---|---|---|
| `Terrain.Floor` | Pavimento | 1 MP | consentito | libera | nessuno |
| `Terrain.Rough` | Accidentato | 2 MP | Dash/Charge **vietati** | libera | rallentamento |
| `Terrain.ShallowWater` | Acqua bassa | 2 MP | consentito | libera | applica `Wet`, conduce elettricità |
| `Terrain.Fire` | Fuoco | 2 MP | consentito | parziale | 10 danni all'ingresso, applica `Burning` |
| `Terrain.Conductive` | Superficie metallica | 1 MP | consentito | libera | conduce elettricità |
| `Terrain.Smoke` | Fumo | 1 MP | consentito | ridotta | applica `Obscured` |
| `Terrain.Ice` | Ghiaccio | 1 MP | consentito (scivoloso) | libera | scivolamento (`Sliding`) |
| `Terrain.HighGround` | Quota elevata | 1 MP | dipende dagli archi | libera | **vantaggio geometrico**, nessun bonus numerico |

### Pavimento
Interazioni: movimento · Sprint · Dash · posizionamento gadget. **Nessun effetto.**

### Terreno accidentato
Interazioni: attraversare · livellare · distruggere ostacoli.
Effetti: costo **2 MP** · **Dash e Charge non possono attraversarlo** · non modifica la linea di vista.

### Acqua superficiale
Interazioni: elettrificare · congelare · evaporare · spostare tramite abilità.
Effetti: costo **2 MP** · applica `Wet` · **spegne `Burning`** · **conduce elettricità**.

**Propagazione elettrica** — l'elettricità attraversa celle d'acqua adiacenti entro il limite dichiarato
dall'azione (`Action.Electrify`: max **3 celle**, danno iniziale 20, propagato 12). Ordine di propagazione
deterministico: **distanza dalla sorgente → `CellId` → `UnitId`**. Ogni unità è colpita **una sola volta** dallo
stesso evento.

### Fuoco
Interazioni: spegnere con acqua · propagare su vegetazione · alimentare con olio o gas · attraversare.
Effetti all'ingresso di un'unità: **10 danni immediati** + `Burning`.
`Burning`: **8 danni** durante il Cleanup, durata **2 turni**, **rimosso da `Wet`**.

### Superficie conduttiva
Interazioni: elettrificare · isolare · collegare a dispositivi.
Effetti: **non** applica `Wet` · propaga elettricità · può attivare generatori o porte configurate.

### Fumo
Interazioni: ventilare · attraversare · incendiare se associato a gas combustibile.
Effetti: le unità interne hanno `Obscured` · **range massimo di targeting dentro o attraverso il fumo: 2** ·
non blocca il movimento.

### Ghiaccio
Interazioni: sciogliere con fuoco · rompere · elettrificare dopo lo scioglimento.
Effetti: costo 1 MP · se un'unità entra con **almeno 2 MP residui**, scivola di una cella nella direzione
d'ingresso; una cella bloccata impedisce lo scivolamento.
> ~~Il PDF stesso segnala che questa regola **va rimandata**~~ — **superato il 2026-08-07**: la regola è
> **implementata e vigente**, verificata da `Terrain.Ice.SlidesWithSufficientBudget`,
> `Terrain.Ice.SlideBudgetBoundaryIsExactlyTwo` (la soglia è esattamente 2), `Terrain.Ice.BlockedCellStopsSliding`
> e `Terrain.Ice.SlidesInMatch` (nel turno reale, non solo in isolamento). Lo scivolamento **resta limitato a
> una cella**: momentum, slide a catena e collisioni fra unità scivolanti sono fuori dalla v0.1
> ([`brief-ghiaccio.md`](../gameplay/brief-ghiaccio.md)).

### Quota elevata

Costo 1 MP · la percorribilità **dipende dagli archi** (rampe e scale: i piani non sono adiacenti per
costruzione).

**L'altura dà vantaggio geometrico, non numerico** ([D-018](../decisions/RT_PDR_00_Decision_Log.md) ·
[D-024](../decisions/RT_PDR_00_Decision_Log.md)):

1. modifica **geometria, LOS, copertura e accessibilità** — e questo basta a renderla una posizione contesa;
2. **nessun `+Damage`** per il solo fatto di stare in alto;
3. **nessun `+VisionRange`** finché un playtest non lo giustifichi;
4. un **eroe, tratto, abilità, equipaggiamento o specializzazione** può dichiarare esplicitamente un bonus
   legato alla quota: allora è suo, ed è scritto nel suo kit.

> *Fino al 2026-08-08 questa riga diceva «bonus visuale», senza quantificarlo, e il workbook proponeva
> `Sight_Mod = +1/+2/−1`. Quel numero veniva da un'esplorazione, non da un playtest.*
>
> **Nel codice il bonus da altura non è mai stato applicato**: `URTCombatLibrary::EffectiveAttackPower(Base,
> OccupantDamageBonus)` è un parametro **generico** e ogni call site runtime passa `0`. Il meccanismo resta —
> serve ad altri effetti — ma non è «l'altura». Il test
> `RefactorTactics.Combat.EffectiveAttackPowerWithTerrainBonus` insegna ancora il contrario nel nome: issue di
> rinomina, non correzione al volo.

---

## 2. Stati applicati dai terreni

| Stato | Origine | Effetto | Durata |
|---|---|---|---|
| `Wet` | acqua bassa, `CreateWater`, Riva | conduce elettricità, rimuove `Burning` | finché sulla cella / **1 turno** se applicato da un'abilità (CP 8.2) |
| `Burning` | fuoco, `Ignite` | 8 danni nel Cleanup | 2 turni, rimosso da `Wet` |
| `Electrified` | propagazione elettrica | danno dell'evento | istantaneo (una sola volta per evento) |
| `Obscured` | fumo | targeting limitato a 2 celle | finché nel fumo |
| `Sliding` | ghiaccio | scivolamento di 1 cella | all'ingresso |

Gli stati non-ambientali (`Rooted`, `Exposed`, `Marked`, `Slow`) sono nel [catalogo azioni](RT_ActionCatalog_v0.1.md).

---

## 3. Coperture e strutture

| StructureId | Elemento | Movimento | LOS | Integrità | Protezione |
|---|---|---|---|---:|---|
| `Structure.LowCover` | Copertura bassa | blocca l'arco | parziale | 30 | 10 |
| `Structure.HighCover` | Copertura alta | blocca l'arco | bloccata | 50 | totale |
| `Structure.Door` | Porta | variabile | variabile | 40 | variabile |
| `Structure.Bridge` | Ponte | consente l'arco | libera | 40 | nessuna |
| `Structure.KineticPanel` | Pannello temporaneo | blocca l'arco | parziale | 30 | 10 |

**Copertura bassa** — è associata a uno **specifico bordo** della cella (direzionale): riduce il danno diretto di
**10** solo dagli attacchi che attraversano quel bordo · non protegge da un attacco proveniente da un'altra
direzione · non protegge dagli AoE con centro sul lato protetto.

**Copertura alta** — blocca movimento, linea di vista e proiettili · può essere distruttibile.

**Porta** — stati `Open` · `Closed` · `Locked` · `Destroyed`. **Ogni cambio di stato aggiorna la revisione del
grafo** (invalidazione delle cache di percorso).

**Ponte** — rappresenta un **arco** fra due celle · può essere attivo, disattivo o distrutto · nel vertical slice
**non si muove durante la risoluzione**: la modifica è discreta, fra un turno e l'altro.

---

## 4. Divergenze rispetto al PDF (dichiarate)

| # | PDF | Qui | Motivo |
|---|---|---|---|
| 1 | Tabella dei terreni con colonne sfalsate nell'estrazione (costo/LOS/effetto disallineati) | Ricostruita associando ogni riga alla propria descrizione a parole (§4.x del PDF) | Le descrizioni testuali sono la fonte più affidabile: i numeri delle tabelle sono stati verificati contro di esse |
| 2 | `Terrain.Fire` LOS «parziale» | Mantenuto «parziale», ma **non** definito quanto riduca | Il PDF non lo quantifica: resta **non specificato**, da fissare in E8 |
| 3 | Ghiaccio con scivolamento obbligatorio | Marcato come **rimandabile** | È il PDF stesso a dirlo; lo scivolamento crea movimento non pianificato dentro la risoluzione |
| 4 | Coperture come proprietà di cella | Coperture **direzionali per bordo** | Coerente con ADR-0002 (6 lati) e con la protezione «non da altre direzioni» del PDF stesso |

**Non specificato nel PDF** (da decidere in E8/E9): riduzione esatta della LOS attraverso il fuoco ·
~~durata di `Wet` fuori dall'acqua~~ **deciso al CP 8.2: 1 turno**, il valore che il catalogo eroi dichiarava
già per `Riva.PressureJet` e `Riva.CircularTide` · comportamento del ghiaccio quando due unità scivolano nella
stessa cella · se la copertura bassa protegga anche dai `Push`.
