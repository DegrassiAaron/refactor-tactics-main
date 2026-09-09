# Istruttoria — il `T7` dello showcase: due strade, costi misurati (#2149), 2026-09-09

> `ISTRUTTORIA` · **Oggetto**: il `T7` di `Scenarios/RT_Showcase_Relay_v01.json`, primo anello da sciogliere
> per realizzare il `§T8` come [`D-360`](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso.
> **Decisione da prendere**: quale forma dia il `T7` alla combo ambientale, che oggi **non è ottenibile in un
> turno solo**.
> **Metodo**: sola lettura. Nessun file modificato, nessuna build, nessun test eseguito.
> **⛔ Questa istruttoria non sceglie.** Pone le uscite con i costi; la decisione è d'autore.

## Nota di provenienza

Prodotta in sola lettura su `HEAD = 57112a47`. I fatti su cui poggiano le conclusioni sono stati
**riverificati indipendentemente** prima di registrarla: `Hero.Gadget.LinearDischarge` non imposta
`PropagationLimit` (solo `ConductiveNode`, `RTHeroCatalogLibrary.cpp:378`); il `§T2` della spec non contiene
alcun Dash nell'acqua (`showcase-v0.1.md:237-244`); il `Ram` di Branth è la sorgente della categoria
`fallback` (`RTShowcaseScenarioTests.cpp:2275-2281`).

---

## 0. `Riktor` = `Branth`

La spec usa ancora `Riktor` (`showcase-v0.1.md:77, 84, 91, 183, 191`); scenario e codice usano `Branth`
([`D-334`](../../decisions/RT_PDR_00_Decision_Log.md), commit `748fc090`). Gli asset conservano il nome
vecchio (`Content/RT/Characters/Riktor/`).

## 1. Cosa fa il `T7` oggi

Il turno **gira** — `Scenario.ShowcaseRelayV01PlaysEveryTurn` asserisce otto turni, esito `Pass`
(`RTShowcaseScenarioTests.cpp:728`). Quattro intenti (`RT_Showcase_Relay_v01.json:299-341`):

| Unità | Intento | Fase |
|---|---|---|
| Phase | `Gadget.Sprinkler` su Wraith | `Environment` → **Cleanup** |
| Gadget | `Hero.Gadget.LinearDischarge` su Wraith | `Attack` → **Blast** |
| Wraith | `move` → `(1,2,0)` | **Move** |
| Branth | `dash: Hero.Branth.Ram` → `(1,0,0)` | **Dash** |

Il vincolo d'ordine descritto dalla nota è reale: il `Blast` precede il `Cleanup`, quindi la scarica legge un
bersaglio non ancora bagnato.

### 🔴 Tre fatti che le note non dicono, e che cambiano il perimetro

**(a) `LinearDischarge` non propaga, e non può.** `PropagationLimit` vale `0` di default
(`RTActionDef.h:523`) e l'azione non lo imposta (`RTHeroCatalogLibrary.cpp:347-349`); il catalogo lo scrive —
*«LinearDischarge base non propaga»* (`:403`). E `CollectElectricPropagation` ha **un solo sito di chiamata in
produzione** (`RTTurnManager.cpp:3523`), dentro il ramo filtrato su `ERTMatchPhase::Cleanup` (`:3433-3437`).

∴ **la riga dell'`Expected` *«la propagazione elettrica è ordinata e non colpisce due volte»* non è ottenibile
da `LinearDischarge` in nessuna delle due strade.** Richiede `Hero.Gadget.ConductiveNode`
(`PropagationLimit = 3`), che è **anch'essa** `Environment` → Cleanup.

**(b) Un'unità pianifica UNA sola abilità per turno.** `PlannedAbilityIndex` è un `int32` singolo
(`RTUnit.h:280`), consumato una volta nel Cleanup (`RTTurnManager.cpp:3431-3442`). ∴ Gadget non può fare
`LinearDischarge` **e** `ConductiveNode` nello stesso turno.

**(c) Dentro il Cleanup l'ordine è per CELLA, non per priorità.** `RTTurnManager.cpp:3405` ordina con
`StableLess(A.Cell, B.Cell)` → `Layer → X → Y` (`RTHexLibrary.cpp:617-622`). ∴ mettere acqua e scarica
entrambe nel Cleanup **non risolve l'ordine**: lo fa decidere dalla `X` dei lanciatori. Al `T7` Gadget sta a
`(-3,-1,0)` e Phase a `(0,-1,0)`: **Gadget risolverebbe per primo**, ancora prima che l'acqua esista.

### Dove finisce l'acqua, oggi

La cella bersaglio si legge **nel Cleanup** (`RTTurnManager.cpp:3439, 3484`), quando il Move ha già portato
Wraith a `(1,3,0)`. ∴ **l'acqua nasce attorno a `(1,3,0)`, non sulla fascia `Fire`** — corroborato dalle
cinque voci `Gadget.Sprinkler` nel golden `turn-07.rttl`, che è l'area di raggio 1 attorno a `(1,3)` dentro
l'arena.

Conseguenze non ancora asserite da nessuna riga: `(1,2,0)` è `Ice` e viene **sostituita** da `ShallowWater`
per 2 turni; e Wraith **si bagna nel Cleanup del T7**, con durata `PersistentWhileOnCell`.

### Il `Ram` di Branth viene **rifiutato**, e la nota su questo è scaduta

La nota teme che partendo da `Rough` *«il rifiuto avrebbe un'altra causa»*. Misurato: **falso**.
`StepBlockReason` guarda le celle **attraversate** (`RTMovementActionLibrary.cpp:26-31, 44-58`), e `(1,0,0)`
è `Rough` con `bBlocksDashCharge = true`.

🔴 **E quel rifiuto è l'unica sorgente della categoria `Fallback` in tutto lo showcase**
(`RTShowcaseScenarioTests.cpp:2275-2281`). Togliere o modificare il `Ram` rimette `fallback` fra i mancanti di
`EveryKeyEventAppearsInTheLog` e lo manda rosso.

### La scarica colpisce **anche Phase**

Gadget `(-3,-1,0)`, Phase `(0,-1,0)` e Wraith `(1,-1,0)` sono sulla riga `r=-1`, e `LinearDischarge` è `Line`
portata 5. L'assertion `UnitHpEquals Phase = 60` lo dichiara. ∴ **qualunque spostamento di Phase cambia quel
numero.**

## 2. Il «modello §T2» non esiste nella forma citata

Il `§T2` (`showcase-v0.1.md:237-244`) descrive il coordinamento **dentro il Blast**: nessun Dash, nessun
ingresso in acqua. Il modello *«il bersaglio entra nell'acqua nel Dash»* è nominato al **`§T3`** (righe
261-263) e attribuito a **scenari**, non a turni. I modelli funzionanti sono **due**, non intercambiabili:

| Modello | Scenario | Chi bagna | Cosa NON fa |
|---|---|---|---|
| **M1** coordinamento in-Blast | `Visual/Combat/WaterElectricCoordinated.json` | `PressureJet`, priorità 50 < 55 | **non cambia la superficie** ⇒ il fuoco non si spegne |
| **M2** bagnato prima del Blast | `Visual/Combat/WaterElectric.json` | il **terreno**, nel Dash | richiede che l'acqua **esista già** |

Il meccanismo che li rende entrambi validi è dichiarato in `RTTurnManager.cpp:5580-5597`.

## 3. La propagazione regge la strada «due turni»?

**Sì, ma solo se la scarica cambia azione.** Le regole (`RTTerrainLibrary.cpp:154-256`) — limite BFS,
una-volta-per-unità, ordine totale `Steps → StableLess → UnitId`, arresto sul non conduttivo — reggono senza
essere toccate, a due condizioni: la scarica dev'essere `ConductiveNode`; e se acqua e scarica finiscono nello
stesso Cleanup, l'ordine lo decide la `X` (§1c).

⚠️ `Status.Electrified` **non viene applicato all'unità**: è un'etichetta nel TurnLog
(`RTTurnManager.cpp:3547-3560`). Non è asseribile come stato portato.

## 4. Le due strade

### A1 — coordinamento in-Blast

Si sostituisce l'intento di Phase con `Hero.Phase.PressureJet`. **Si ottiene** il +8
(`GadgetWetDischargeBonus`). **Non si ottiene** *«il fuoco si spegne»*: `PressureJet` non crea superficie, e
quella è la prima riga dell'`Expected`. Sparisce anche l'unico evento `Environment` del turno.

**Costo**: un intento riscritto, nessun turno aggiunto. La più economica e la meno completa.

### A2 — bagnato nel Dash

**Non realizzabile sulla geometria del Basin.** Da `(1,-1)` verso l'acqua a `(0,1)` lo scostamento non è
multiplo di nessuna direzione assiale: `IsStraightLine` lo rifiuta (`RTMovementActionLibrary.cpp:61-75`).
`(1,1,0)` è raggiungibile ma è `Conductive`, che **non applica `Wet`**. ∴ A2 richiede acqua preesistente, e
**collassa nella strada B**.

### B — due turni distinti

| Turno | Chi | Cosa cambia |
|---|---|---|
| T6 o turno nuovo | Phase | acquisisce `Gadget.Sprinkler`. ⚠️ il T6 è il turno dell'interposizione, il cui oracolo è `UnitHpEquals Branth = 103`: un'azione ambientale risolve nel Cleanup e non tocca quel Blast, **ma va verificato** |
| T7 | Gadget | `LinearDischarge` per il +8, **oppure** `ConductiveNode` per la propagazione — non entrambe (§1b) |
| T7 | Phase | libera l'azione, il che è rilevante per il T8 |

**Regge perché**: la superficie dura 2 turni (`RTTurnManager.cpp:3480`); il `Wet` che ne deriva è
`PersistentWhileOnCell`, non a scadenza; e il precedente eseguibile è verde nel corpus —
`Spec/Environment/WaterQuenchesFire.json`.

**Costo**: il bersaglio deve **restare** sulla cella allagata fra i due turni (il `Wet` legato alla cella si
revoca uscendo), il che vincola il Move di Wraith e quindi la scivolata su `Ice` che il T7 esiste per
mostrare.

⚠️ **Rischio da verificare, non da dedurre**: `Wet` **cancella** `Burning`. L'`expect`
`LogEventCount(Combat, Hit, "Status.Burning") = 2` conta i tick del T3. Se la strada B anticipa il bagnato,
quel conteggio scende. Non dimostrabile senza eseguire.

## 5. Gli `expect` toccati

Il file porta le voci `expect` che questo comando conta:

```
python -c "import json,io;print(len(json.load(io.open('Scenarios/RT_Showcase_Relay_v01.json',encoding='utf-8'))['expect']))"
```

In gioco:

| Assertion | A1 | B |
|---|---|---|
| `TurnsCompleted` | invariata | **cambia** se si aggiunge un turno |
| `UnitHpEquals Phase = 60` | **cambia** se Phase si sposta | **cambia**, idem |
| `UnitAtCell Wraith = (1,3,0)` | invariata | **a rischio**: restare sull'acqua confligge con la scivolata |
| `LogEventCount(Objective, Unclaimed)` | invariata | **cambia** con un turno in più |
| `LogEventCount(Combat, Hit, "Status.Burning")` | invariata | **da verificare** |
| `LogEventOrder(LinearDischarge → Sprinkler)` | **cade** | **cade o va riscritta** — il suo `_assertion` dichiara che *deve* cadere quando il vincolo si scioglie |

## 6. Il golden

Confronto **per turno**; rigenerazione solo con `rt.Test.RegenerateGolden`, che **riscrive tutti** i turni di
tutti gli scenari: la selezione la fa il diff di git (`RTGoldenCorpusTests.cpp:302, 316, 646, 682-694`).

| Strada | File attesi in diff |
|---|---|
| A1 | `turn-07`, più `turn-08` se cambiano posizioni o HP |
| B (riusando il T6) | `turn-06`, `turn-07`, `turn-08` |
| B (turno nuovo) | dal turno inserito in poi, **più** un `turn-09` nuovo |

⚠️ Un turno aggiunto fa cadere `ShowcaseRelay.EightTurnsGoldenMatches`, la cui docstring dichiara la
cardinalità *«parte dell'affermazione»*. Va aggiornato deliberatamente.

## 7. I test toccati

**Per certo**: `Simulation.GoldenCorpusMatches` · `Scenario.ShowcaseRelayV01PlaysEveryTurn` ·
`ShowcaseRelay.EightTurnsGoldenMatches`.

**Se si tocca il `Ram` o si aggiunge un KO**: `ShowcaseRelay.EveryKeyEventAppearsInTheLog` (l'insieme dei
mancanti dev'essere **esattamente** `"KO"`) · `Simulation.GoldenCorpusCoversItsCategories`.

**Se cambia la traccia di un turno**: `ShowcaseRelay.DivergenceNamesTurnPhaseAndAction` (guasta il turno 8) ·
`.StateHashStablePerTurnUnderPermutation` · `Meta.PlayerMoveDecayOnTheShowcase` (legge gli otto `.rttl`) ·
`Simulation.GoldenCorpusHasNoOrphanTurns` · `.GoldenCorpusIsAtCurrentFormat`.

**Restano verdi ma vincolano** — sono i contratti che ogni strada deve rispettare: la famiglia
`Environment.Propagation.*`, i test sul bonus bagnato e sulla durata del `Wet`,
`Environment.WaterExtinguishesFire`, `Terrain.Rough.BlocksDash`, `Terrain.Ice.*`, e soprattutto
**`ShowcaseRelay.BasinLayoutMatchesSpec`**: le superfici del Basin sono pinnate, quindi nessuna strada può
spostare acqua, fuoco, rough o ghiaccio senza toccare anche `MakeShowcaseRelayBasinArena`.

## 8. Effetti sul `T8`

### La motivazione del Move di Phase al T5 **è già ritirata nel file stesso**

La `_turno` del T5 dice ancora che Phase si avvicina *«perché al T7 lo Sprinkler possa arrivarci»*, ma la
`_nota_avvicinamento_di_phase` dello stesso turno lo corregge: la portata di `Gadget.Sprinkler` è **4**,
ereditata dal core (`MakeEquipmentAction` non tocca `RangeCells`, `RTCatalogLibrary.cpp:900-902`), e la
distanza era **3**. Non serviva avvicinarsi.

∴ **quel Move è coreografia, non un vincolo di raggiungibilità**, ed è disponibile per la seconda rotta che
il `T8` vuole — al prezzo di far cadere `UnitHpEquals Phase = 60`.

### Branth è impegnato in un **rifiuto**, non in un'azione

Il `Ram` viene scartato nel Dash e Branth resta a `(2,0,0)`: il suo Move del T7 è **libero**. Ma rimuoverlo
costa il `fallback` (§1).

### Il KO di Gadget: le distanze

Gadget `(-3,-1,0)`. A fine T7: **Branth** a distanza **6** con `MovePoints = 4`; **Wraith** a distanza **8**
con `MovePoints = 6`. ∴ **nessuno arriva a contatto in un turno**, e il T8 ne ha uno solo. Realizzare il KO
richiede di avvicinare un rosso **già al T7**.

### 🔴 Il collo di bottiglia

Lo stesso Move di Wraith al T7 è chiesto da **tre cose che si escludono**: restare sull'acqua (strada B),
scivolare sull'`Ice` (l'`Expected` del §T7 e il suo `expect`), avvicinarsi a Gadget (`D-360`). Il Move di
Branth ne serve due. **Non è un conflitto che un'istruttoria possa sciogliere**: è la ragione per cui la nota
diceva *«registra il vincolo, non lo scioglie»*.

## 9. Cosa NON è verificabile senza eseguire

1. se una strada produca i numeri che promette — ogni `UnitHpEquals` è un'ipotesi finché non gira;
2. il contenuto esatto dei `.rttl` — letti solo per **identità e cardinalità** delle voci, non deserializzati;
3. se il `Wet` anticipato della strada B cancelli i tick di `Burning`: il meccanismo è certo, il fatto no;
4. se un Blast del T6 con un intento ambientale in più lasci intatto `UnitHpEquals Branth = 103`;
5. quale voce `Objective` produca un turno aggiunto.

## 10. Decisione minima richiesta

Tre risposte, **separabili**:

**D1 — quale `Expected` del §T7 la showcase deve realizzare.** Le quattro righe hanno owner tecnici diversi e
**non sono soddisfacibili tutte dallo stesso intento**:

| Riga | Chi la produce | Fase |
|---|---|---|
| *«il fuoco si spegne»* | solo `CreateWater`/`Sprinkler` | Cleanup |
| *«propagazione ordinata»* | solo `Electrify`/`ConductiveNode` | Cleanup |
| *«scivolata deterministica»* | il Move di Wraith | Move |
| *«`EnvironmentChanged`»* | una delle prime due | Cleanup |

E le prime due nello stesso Cleanup ricadono nel problema d'ordine per `X`. **Un turno solo non le porta
entrambe.**

**D2 — payoff elettrico (`LinearDischarge` +8) o propagazione (`ConductiveNode`)?** Azioni diverse, fasi
diverse, una sola per unità per turno.

**D3 — chi paga il Move di Wraith al T7?** Tre pretendenti, uno slot.

Le tre risposte determinano la strada; la strada — e non il contrario — determina quanti golden si rigenerano
e quali `expect` cadono.

## 11. Cosa non va scritto, qualunque strada si scelga

Ognuna di queste è già dichiarata difettosa nel repository:

* **invertire l'`expect` sull'ordine** per farlo passare: il suo `_assertion` dichiara che deve **cadere**
  quando il vincolo si scioglie, non essere negato;
* **scrivere «fuoco spento» senza cambiare l'azione**: è il difetto che `#1111` ha misurato;
* **anticipare le battute del §T8**: hanno owner `#2149`, e `D-360` lascia fuori *«quale unità raggiunga
  Gadget»*;
* **spostare una superficie del Basin** senza toccare `MakeShowcaseRelayBasinArena` e
  `ShowcaseRelay.BasinLayoutMatchesSpec` nello stesso commit;
* **rigenerare il golden senza dichiarare perché**: il requisito è dentro il messaggio d'errore del test.

## Verification

* Compile · Determinism · Privacy · PIE · Packaged: `N/A` — istruttoria in sola lettura
* Tests: `NOT RUN` — i verdetti provengono da lettura di sorgenti, non da esecuzione
* Replay: `NOT RUN` — i `.rttl` letti per identità e cardinalità, non deserializzati
