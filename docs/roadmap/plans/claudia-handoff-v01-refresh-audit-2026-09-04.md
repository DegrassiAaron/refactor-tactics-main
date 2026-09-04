# Kit d'autore *Claudia Handoff v0.1 REFRESH* — audit LIVE del 2026-09-04

> `SNAPSHOT` · **Misurato**: 2026-09-04 · **Base**: `origin/main` `b4719994`, checkout `a30d8281`
> **Cosa è**: la misura del kit d'autore `RefactorTactics_Claudia_Handoff_v0.1_REFRESH_2026-09-03.md`
> contro lo stato LIVE di GitHub e del codice. Il kit è stato **consumato**: non esiste più come file, e
> questo documento è ciò che resta della sua istruttoria.
> **Cosa non è**: un owner. I gate restano di [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md),
> la topologia di [`../execution-graph.yaml`](../execution-graph.yaml), le verifiche a schermo di
> [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md), le domande aperte di
> [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

## 1. Cosa chiedeva, e perché la risposta non è un piano nuovo

Il kit proponeva quattro gate — `D0 → CP-A → CP-B → CP-C → G13` — con prerequisiti, scenari, checklist di
PASS e un dependency graph «target» da scrivere nel grafo reale. La sua regola d'apertura dice già la cosa
giusta, ed è quella che questo audit ha applicato:

> *«questo file è un handoff operativo, non una fonte di verità. Prima di modificare GitHub, verificare lo
> stato LIVE delle issue, label, milestone, assignee, PR e dependency graph.»*

Misurato: **diciassette delle issue che il kit mette in coda sono già chiuse**, e i quattro gate che
propone di istituire **esistono già** nel repository con altri nomi e altri owner. Perciò questo giro
registra l'audit e — deliberatamente — **non** riscrive `execution-graph.yaml`, **non** apre issue nuove e
**non** introduce una seconda nomenclatura di gate accanto a quella del DoD.

## 2. Stato LIVE delle issue che il kit elenca

Misurato via GraphQL sulle 46 voci nominate dal kit (§2, §3, §11, §13).

| Insieme | Numeri | Conteggio |
|---|---|--:|
| **CLOSED/COMPLETED** | `#75` `#170` `#625` `#686` `#690` `#729` `#1496` `#1497` `#1499` `#1525` `#1663` `#1665` `#1800` `#1801` `#1922` `#1945` `#1957` | **17** |
| **OPEN nel perimetro v0.1** | `#77` `#159` `#160` `#219` `#220` `#291` `#613` `#637` `#705` `#938` `#940` `#1535` `#1933` | **13** |
| **OPEN post-v0.1** | `#327` `#773` `#774` `#775` `#776` `#777` `#778` `#784` | **8** |
| **Epic OPEN v0.1** | `#14` `#25` `#26` `#151` `#152` | **5** |
| **Fuori critical path, ma già OPEN in v0.1 `P3`** | `#314` `#319` | **2** |
| **Non è una issue** | `#728` — è una **PR chiusa** | **1** |

Nessuna voce è `SUPERSEDED`: tutte e diciassette le chiuse lo sono come `COMPLETED`, nessuna
`not_planned`.

## 3. Le sei affermazioni del kit che il LIVE falsifica

1. **`#728` non è una issue.** È la PR *«Facing: FAC-11 chiude in D-126»*, chiusa. Il kit la elenca fra le
   «issue fuori critical path» da non spostare nella v0.1: non c'è niente da spostare. Nella stessa lista
   **`#729` è già chiusa**.
2. **`#784` non è un prerequisito di `CP-B`.** Il kit la mette in *Flow B — Knowledge* e fra gli anti-leak
   del checkpoint. LIVE è `CP 40.6`, label `post-v0.1`, milestone **`v0.5 · Online Foundation`**: è il
   canary che fallisce se un client riceve un byte non autorizzato, e **la replica di rete non esiste
   ancora**. Portarla nella v0.1 richiederebbe la decisione esplicita che il kit stesso vieta in §11.
3. **`#314` e `#319` sono già nella v0.1.** Il kit chiede di «non spostarle automaticamente»: portano già
   label `v0.1`, milestone `v0.1 · Percezione e reazioni` e priorità `P3`. Ciò che è vero — e che
   `execution-graph.yaml` dichiara già — è che entrano in catena A con archi `follows`, cioè **ordine e
   non blocco**.
4. **L'edge `#220 → #77` non esiste.** Il kit disegna `#219/#637 → #220 → #77/#613`. Il corpo di `#77`
   dichiara *«Dipende da #45, #59 (entrambe chiuse — il checkpoint è eseguibile)»*; quello di `#613`
   dichiara `#219` e `#77`; quello di `#705` dichiara `#77` e `#613`. La direzione misurata è
   `#219 → #613` e `#77 → #613 → #705`, non il passaggio per `#220`.
5. **I prerequisiti HARD di `G13` sono soddisfatti, e `G13` resta 🟡 lo stesso.** Il kit scrive
   *«`#1663` CLOSED, `#1665` CLOSED → poi package»*: entrambe **sono** chiuse. Ma la riga `G13` del DoD è
   gialla per una riserva che il kit non nomina — la via a punti non è esercitata in autobattle (il bot non
   cerca l'obiettivo, è E26) e `PIE-V01-BOARD` non è stata eseguita. Chiudere i due blocker non accende il
   gate.
6. **`#1933` non blocca il Core Freeze, e la ragione è più forte della condizione che il kit pone.** Il kit
   la subordina (*«se il suo residuo è solo fidelity Overwatch/TurnLog»*). Misurato: il residuo **è** solo
   quello — `ReadFacingForConsumer` non ha chiamanti in gioco, quindi nessuna traccia reale porta
   `UsedByOverwatch` — e il commento del 2026-09-03 sulla issue la classifica già *«confinante, non
   bloccante»* dopo la PR `#2165`.

## 4. I gate esistono già, e i nomi del kit non sono i loro

| Nome nel kit | Owner reale | Nome reale |
|---|---|---|
| `D0 Decision Gate` | [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | `AUTHOR-RESOLUTION-001` · `AUTHOR-DAMAGE-001`, **aperte** |
| `CP-A Core Contract Freeze` | test Automation nominati + `G4` del DoD | nessun nome proprio: è la somma di `#1957` `#1922` `#1800` |
| `CP-B` | `G9` del DoD | subset **`RELEASE-V01`**, 17 voci in `test-manuali-pie.md` |
| `CP-C` | `G10` del DoD | *partita completa con esito terminale dichiarato* |
| `G13` | `G13` del DoD | **stesso nome, stesso oggetto** |
| `E1 · G1 · E2 · G13` (policy Editor §14) | [`../editor-sessions.yaml`](../editor-sessions.yaml) | `PRE-0 · E1 · G1 · E2 · P1` |

⚠️ **Una collisione da conoscere, non da sanare qui**: `G1` nomina due cose in due owner — *«build dei tre
target senza warning nuovi»* nel DoD, *«clean gate: build + suite + validator, 0 Editor»* nelle sedute. Non
si contraddicono (il secondo contiene il primo), ma il repository ha già pagato una collisione fra i due
spazi di numerazione dei checkpoint, ed è la stessa forma.

## 5. `D0` — non si chiude misurando, e non è ambiguo nel codice

Le due domande del kit sono, verbatim, due voci aperte di `OPEN_DECISIONS.md` che il repository ha già
istruito e che chiedono **una scelta d'autore**, non una misura.

Rimisurato oggi su `a30d8281` con `git grep -w … -- Source`:

| Termine | Occorrenze in `Source/` |
|---|--:|
| `ResolutionLayer` | **0** |
| `Armor` | **0** |
| `DamageResistance` | **0** |
| `DamagePacket` | **0** |
| `Shred` | **0** |
| `BaseShield` | presente (`Combat/RTCombatLibrary.h` + quattro file di test) |

∴ **il PASS che il kit chiede — «nessun contratto v0.1 ambiguo» — è già vero nel codice**: un solo
contratto di assorbimento è spedito (`D-224`: lo scudo base ferma il solo `Direct`) e un solo ordine di
risoluzione esiste (`ERTMatchPhase` / `ERTResolutionPhase`). L'ambiguità è **fra il kit Drive e il
repository**, ed è registrata dove va — due voci aperte — non come un contratto doppio in `Source/`.

## 6. `CP-A` — eseguito, e ogni riga dello scenario aveva già il suo oracolo

Suite completa `./scripts/rt-suite.ps1`, verdetto **`VALIDA`** — `HEAD` `a30d8281`, albero `d44d818a`,
binario delle 10:10 ricompilato su questo `HEAD`, invariati dall'inizio alla fine.
**1921/1921 completati, 0 fallimenti**, `EXIT CODE: 0`, durata 02:25.

| Clausola dello scenario §5 del kit | Test | Esito |
|---|---|---|
| primo planning non si chiude prematuramente | `HexMatch.FirstTurnDoesNotCloseItself` | `Success` |
| `A ↔ B = BLOCKED` | `HexSim.ResolveSwapBlocked` · `…EvenWhenPassingThrough` · `ResolveHeadOnBlocksLinearSwap` · `HexMove.StalePlanSwapBlocks` | `Success` ×4 |
| `A → B → C → A = BLOCKED` | `HexSim.ResolveClosedCycleBlocked` | `Success` |
| `A → B → C → FREE = ALLOWED` | `HexSim.ResolveFreeTailConvoyStillAdvances` | `Success` |
| stesso stato + facing diverso ⇒ hash diverso | `Simulation.ChecksumDiscriminatesFacing` | `Success` |
| Facing nel digest | `Simulation.DigestUsesStableUnitIdAndKeepsTheDead` | `Success` |
| stesso input ⇒ stesso hash | `Replay.Verifier.ResimulationIsDeterministic` (è `G4`) | `Success` |
| snapshot immutabile / ordine irrilevante | `Simulation.ChecksumStableAcrossPermutations` · `HexSim.SnapshotOrderIndependent` | `Success` ×2 |
| nessuna dipendenza da frame/timing | `Scenario.SimulationTimeIsDeterministicWallClockIsNot` | `Success` |

Per gruppo, nella stessa run: `HexSim` **37/37** · `Replay` **85/85** · `Simulation` **21/21** ·
`Scenario` **161/161** · `Match` **44/44** · `Frontend` **82/82** · `ScreenHud` **13/13**.

⚠️ **`-Filter Determinism` non è un filtro di questo repository.** Il kit lo prescrive accanto a `HexSim` e
`Replay`, che invece esistono come gruppi. I test di determinismo vivono sparsi in `Simulation.*`,
`Replay.Verifier.*`, `HexMap.*`, `Match.Autobattle.*`: si raggiungono con la suite intera, non con quel
filtro. È il caso che il kit stesso prevede scrivendo *«usare i filtri realmente presenti»*.

## 6-bis. `CP-B` — i due scenari del kit esistono, e la loro metà funzionale è verde

Il kit nomina `Visual.Hud.FirstPlayable` e `Visual.Perception.Acceptance` con la riserva *«se esposto dal
repository»*. Lo sono entrambi (`Scenarios/Visual/Hud/FirstPlayable.json`,
`Scenarios/Visual/Perception/Acceptance.json`), e nella run di oggi:

```text
[RT-Test] Visual.Hud.FirstPlayable:      PASS (9/9 assertion, 2 turni)
[RT-Test] Visual.Perception.Acceptance:  PASS (8/8 assertion, 3 turni)
```

Con essi i **13/13** `ScreenHud.*`, fra cui `WidgetApiExposesNoTexture`, che è il DoD di `#220`.

⛔ **Questo non è il PASS che `CP-B` chiede.** Le assertion misurano ciò che il codice decide; la checklist
del kit — HUD **leggibile**, event log che si **capisce**, pointer contract, assenza di dati privati **a
schermo** — chiede il giudizio di una persona davanti al monitor, e quelle quattro voci restano 🟡 in
`G9` per quella sola ragione. Il `Pointer` ha **due** test in tutto
(`CycleDeclaredFacingStaysWithinTheLegalSet`, `PlannedFacingPreviewFollowsThePlan`): la fetta headless di
`#705` è chiusa, il resto no.

## 7. `CP-C` — le grandezze che il kit dichiara, verificate nel codice

`Turn/RTMatchFormatLibrary.cpp::FindShippedFormat` — **cinque su cinque coincidono**, quindi né contratto
nuovo né regressione:

`FormatId = Format.Skirmish2v2` · `UnitsPerTeam = 2` · `RoundLimit = 12` · `ScoreToWin = 5` ·
`MapClass = ERTMapClass::Skirmish`.

Lo scenario `AutoBattle.ArenaV01` coincide anch'esso: `version 4`, `seed 0`, `fixture ArenaV01`,
`maxTurns 40`, celle di partenza `A1(-4,0,0)` `A2(-4,1,0)` `B1(4,0,0)` `B2(4,-1,0)`. La mappa canonica è
`L_HexArena` (`Config/DefaultGame.ini` → `MatchLevel`), come il kit suppone.

Nella run: `Scenario.FreeRun.ArenaV01ReachesAWinner` **`Success`** — la partita non presidiata arriva a un
vincitore **prima** del tetto di 40, che è l'invariante che il kit chiede senza fissare il turno. E
`Match.Autobattle.ShippedFormatEndsTheAuthoredMatch` **`Success`**.

⛔ Ciò che la suite **non** dimostra, e che il kit chiede a `CP-C`: la partita a schermo su `L_HexArena` con
le sue fasi, il `Result`, il `Replay` e la registrazione. Quella è `PIE-V01-PACKAGED`, eseguita il
2026-09-04 sotto `#959` — e resta una seduta, non un test.

## 8. Cosa questo giro ha cambiato, e cosa no

**Cambiato**: la riga `G9` del DoD, che era ferma al 2026-08-24 e dichiarava un blocco caduto il 2026-08-30
(vedi la cella per la misura). Nient'altro.

**Non cambiato, con la ragione**:

- `execution-graph.yaml` — il grafo «target» del kit §13 è in gran parte fatto di nodi **chiusi**: scriverlo
  aggiungerebbe archi storici a un file che dichiara di essere una `THIN_SLICE` di quattro catene reali.
  Gli unici archi del target che il grafo già contiene (`#75 → #170`) ci sono, e sono corretti.
- **nessuna issue nuova** — l'audit non ha trovato lavoro senza owner. Il kit lo prescrive in §12, e la
  misura lo conferma.
- **nessuna issue toccata** — nessuna delle 46 afferma qualcosa che il LIVE falsifichi: le sei
  falsificazioni di §3 sono del **kit**, non delle issue.
