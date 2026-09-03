# Guida di seduta — U42, il corpus Visual

> `CURRENT` · **Creata**: 2026-09-03 · **Seduta**: `U42` in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml) · **Issue**: #2187 ·
> **Owner degli esiti**: [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come**
> osservare, non **cosa è risultato**.
> Preparata su `origin/main = 6aa51ec7`.

## Perché questa seduta viene prima delle altre

**Diciannove voci aperte in una sola convocazione**, e nessun prerequisito: `unblocked_by: []`, con le
quattro issue della seduta (#231, #233, #1919, #2009) **tutte chiuse**. Due sedute — U42 e U43 — coprono
**29 delle 68** voci aperte schedulate; le altre ventuno si dividono il resto, molte con una voce sola.

✅ E gli scenari **girano**: `Scenario.EveryShippedScenarioRuns` e `Scenario.ShippedScenariosAreValid` sono
`Success` (run VALIDA `168/168`, 2026-09-03).
⛔ Il che prova che il gioco non si rompe, **non** che a schermo si veda ciò che deve vedersi — ed è
esattamente la ragione per cui questa seduta esiste.

---

## 1. Allestimento — e la via da NON usare

🔴 **`rt.Test.Run` non è la via per questa seduta, e la prima stesura di questa guida sbagliava.**
Misurato il 2026-09-03, dopo che l'autore ha osservato *«vedo sempre i 4 personaggi, poi i cilindri al
comando»*:

- il runner spawna le proprie unità con `ARTUnit::StaticClass()` (`RTScenarioSession.cpp:753`), cioè
  **cilindri segnaposto** — corretto, quelle unità non hanno skeletal;
- i **quattro personaggi** del roster restano in scena: li schiera il GameMode in `BeginPlay`
  (`RTGameMode.cpp:349-352`), e **il runner non li rimuove** — nessuna `Destroy`, misurato;
- ⛔ **`R` non rimedia, per due ragioni indipendenti**: a partita viva **non fa nulla** ed è silenzioso
  (`RTPlayerController.cpp:1657`, guardato da `GetPhase() == MatchEnded`); e quando funziona chiama
  `OpenLevel`, che rifà `BeginPlay` e **rischiera i quattro**.

∴ ogni verdetto preso così sarebbe su una scena **contaminata**: due allestimenti sovrapposti.

## 2. La via pulita: `ScenarioToRun`

`ARTGameMode::ScenarioToRun` (`RTGameMode.h:274`) esegue lo scenario **al posto** della partita normale.
Verificato nel codice: il ramo `ERTScenarioStart::Started` fa **`return`** prima di allestire la partita
(`RTGameMode.cpp:476-483`), quindi **nessun roster e nessuna sovrapposizione**.

### Il ciclo, diciannove volte

1. Details del **GameMode** → categoria `RefactorTactics|Test` → **`Scenario To Run`**.
2. Scegli lo scenario dal **menu a tendina**.
3. **Play** → osserva → **Stop**.
4. Prossimo scenario dal menu.

✅ **Il menu immunizza dal difetto che ha aperto questa correzione**: gli ID si **scelgono**, non si
scrivono. Il 2026-09-03 un `Enviorment` per `Environment` non ha prodotto nessun errore visibile — la partita
è andata avanti col timer e sembrava che il comando avesse fatto qualcosa. Con diciannove ID da digitare,
quel typo sarebbe tornato.

⚠️ **`ScenarioTurnPauseSeconds`** (default `1.5`) è la pausa **prima** di risolvere ogni turno: è ciò che
rende uno scenario osservabile. Se una scena scorre troppo in fretta per essere giudicata, si alza — e lo si
**annota nell'esito**, perché un verdetto preso a velocità diversa è un verdetto su un'altra cosa.

⚠️ La cvar **`rt.Test.Scenario`** fa lo stesso e **prevale** sulla proprietà: utile per una volta sola senza
toccare l'asset. ⛔ Ma va svuotata dopo, o continuerà a scavalcare il menu in silenzio.

### Se preferisci il pannello

Esiste **Tactical Designer** (`Window → Tools`), col browser di scenari, e il test
`DevSandboxLauncher.EveryShippedScenarioMapsCleanly` conferma che **ogni scenario spedito vi si mappa**.
⚠️ Non è stato provato in questa preparazione: che li **mappi** è misurato, che li **faccia girare nel
viewport** no.

### Note che restano vere comunque

- Se l'editor chiede la versione → **5.8**; se chiede di ricompilare i moduli → accetta.
- Lo sfondo **resta nero anche in gioco**: il GameMode aggiunge una luce direzionale e nessun cielo.
- L'esito dei comandi si legge nell'**Output Log**, non nell'overlay della console.
- ⚠️ **La console ha tre modalità**: `Cmd`, `Python`, `Python (REPL)`. Un comando digitato in modalità Python
  dà `LogPython: Error: SyntaxError` — succede, ed è successo.
- ⚠️ **I verdetti saranno su CILINDRI, non su personaggi**: le unità degli scenari sono `ARTUnit` nudi. Per
  voci come `PIE-VIS-KO` non cambia niente; per altre potrebbe non bastare, e va deciso **voce per voce**
  invece di assumerlo.

## 3. Le diciannove voci, con ciò che le falsifica

L'ordine raggruppa per affinità di scena. La colonna che conta è l'ultima: **un criterio scritto prima è ciò
che distingue un verdetto da un «sembra ok»**.

### Ambiente

| # | scenario | voce | ❌ falsificata se |
|---|---|---|---|
| 1 | `Visual.Environment.IceSlide` | `PIE-VIS-ICE` | il terzo passo — quello **subìto** — si legge identico ai due voluti |
| 2 | `Visual.Environment.WetExtinguishesFire` | `PIE-VIS-WETFIRE` | le fiamme restano accese: è un'**assenza** (i danni non arrivano), e se non si vede il giocatore non sa che la regola è scattata |

### Combattimento

| # | scenario | voce | ❌ falsificata se |
|---|---|---|---|
| 3 | `Visual.Combat.Defeat` | `PIE-VIS-KO` | l'unità sparisce **prima** che il colpo sia arrivato. Attesi: due colpi per turno, barra giù due volte, rimozione al **sesto** |
| 4 | `Visual.Combat.WaterElectric` | `PIE-VIS-COMBO` | l'enfasi cade su chi ha bagnato invece che sul **terreno** bagnato (`D-029`) |
| 5 | `Visual.Combat.WaterElectricCoordinated` | `PIE-VIS-COORD` | i due colpi sembrano simultanei: si deve capire che il secondo è più forte **perché** il primo è arrivato prima |
| 6 | `Visual.Combat.FallbackTargetMoved` | `PIE-VIS-FALLBACK` | il colpo sulla cella vuota sembra **casuale** invece che un piano che ha trovato il bersaglio altrove |
| 7 | `Visual.Combat.SmokeCapsTargeting` | `PIE-VIS-SMOKE` | il fumo si comporta come un **muro**: Riktor deve **vedersi** e non essere colpibile, o si impara la regola sbagliata |
| 8 | `Visual.Core.PhaseOrder` | `PIE-VIS-PHASES` | carica, colpo e camminata accadono **insieme**: la spina dorsale del turno resta invisibile |

### Movimento

| # | scenario | voce | ❌ falsificata se |
|---|---|---|---|
| 9 | `Visual.Movement.Charge` | `PIE-VIS-CHARGE` | si vede uguale a `Movement.LongWalk`: la differenza fra **Dash** e **Move** non arriva |
| 10 | `Visual.Movement.RoughRefusesCharge` | `PIE-VIS-ROUGH` | il rifiuto sembra un'**animazione interrotta** — suggerisce un bug dove c'è una regola |

### Mappa

| # | scenario | voce | ❌ falsificata se |
|---|---|---|---|
| 11 | `Visual.Map.MultiLevel` | `PIE-VIS-LEVEL` | non si capisce **dove è finito** Gadget: è il banco di prova della camera su due layer |
| 12 | `Visual.Map.LowCoverEdge` | `PIE-VIS-COVER` | il bordo riparato non è distinguibile dagli altri cinque **prima** di sparare |
| 13 | `Visual.Map.ClosedDoor` | `PIE-VIS-DOOR` | la porta chiusa non si vede: il giro lungo sembra un difetto del **pathfinding** |
| 14 | `Visual.Map.HighGroundNoBonus` | `PIE-VIS-HIGH` | la presentazione **enfatizza** il tiro dall'alto, suggerendo un vantaggio numerico che in v0.1 non esiste (`D-024`) |
| 15 | `Visual.Map.HighCoverBlocks` | `PIE-VIS-HIGHCOVER` | la barriera **alta** somiglia alla copertura **bassa**: il giocatore prova a sparare dove non può |

### Le difese — quattro voci che si leggono insieme

| # | scenario | voce | ❌ falsificata se |
|---|---|---|---|
| 16 | `Visual.Combat.GuardReducesFirstHit` | `PIE-VIS-GUARD` | Riktor non arriva a **97**: la guardia porta 15 assorbibili, e il **primo** colpo da 22 li consuma tutti |
| 17 | `Visual.Combat.BraceReducesEveryHit` | `PIE-VIS-BRACE` | Riktor non arriva a **102**: ogni colpo perde 10 e la riduzione **non finisce mai** |
| 18 | `Visual.Combat.AreaGuardFromImpactCenter` | `PIE-VIS-AREAGUARD` | la Guardia non regge con il lanciatore **dietro** e l'esplosione **davanti** |
| 19 | `Visual.Combat.GuardVsBraceUnderSmallHits` | `PIE-ACC-GUARDBRACE` | i tre difensori non si distinguono: atteso **Brace illeso**, **Guard** poco colpito, **senza difesa** molto |

🔑 **Il confronto 16 ↔ 17 è il punto, non i due numeri presi da soli**: *una difesa si consuma, l'altra
dura*. Se si presentano uguali, la scelta tattica che le due azioni esistono per offrire non arriva a chi
gioca — ed è una conclusione che nessun test headless può raggiungere.

## 4. Come si registra un esito

⛔ **«Sembra ok» non è un verdetto.** Per ciascuna voce serve una delle tre:

- ✅ **osservato e conforme** — dicendo *cosa* si è visto, non che andava bene;
- ❌ **osservato e difforme** — con il difetto descritto; si apre una issue, **non** si ripara in corsa;
- ⚠️ **non osservabile** — con la ragione. È un esito legittimo, e vale più di un verde generoso.

⚠️ Il conteggio canonico va rimisurato **prima e dopo**, con `senza-marcatore=0` a entrambi i passi. Il
comando è in testa a `test-manuali-pie.md`, e **non si reinventa**: una misura fatta con un criterio diverso
ha già prodotto uno scarto del 27% il 2026-09-03.

## 5. Cosa questa seduta NON copre

- ⛔ **Le due voci `PIE-VIS-*` che restano fuori** — `PIE-VIS-INTERPOSE` e `PIE-VIS-DEFLECT` — dichiarano un
  ostacolo proprio e non sono in U42.
- ⛔ **Le sedute con `execution_lane: asset`**: producono `.uasset`, ed è un altro mestiere.
- ⛔ **Riparare ciò che la seduta trova.** Si osserva e si registra; le correzioni sono issue.
