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

## 1. Allestimento

1. Apri il progetto. Se l'editor chiede la versione → **5.8** (`D:\EpicGames\UE_5.8`); se chiede di
   ricompilare i moduli → accetta.
2. **Play** (`Alt+P`). ⚠️ Lo sfondo **resta nero anche in gioco**: il GameMode aggiunge una luce
   direzionale e nessun cielo. Non è un difetto.
3. `rt.Test.List` per confermare che gli scenari siano registrati. ⚠️ **Il numero si legge, non si cita**:
   questa richiesta ha detto «4», poi «8», poi «9», e al 2026-08-17 ne contava **78**.

⚠️ L'esito dei comandi si legge nell'**Output Log**, che è il medium legittimo deciso dall'autore il
2026-08-16: l'overlay della console in PIE mostra poche righe e scorre via.

## 2. 🔴 La procedura, e non è una raccomandazione

```
rt.Test.Run <ScenarioId>   →   osserva   →   R   →   prossimo
```

Il registro lo dichiara in `PIE-TEST-CONSOLE`:

> *«eseguire uno scenario **sostituisce la mappa** e aggiunge unità alla partita in corso — è previsto (il
> runner riusa mappa e turn manager), ma dopo conviene riavviare con `R`»*
>
> *«due esecuzioni consecutive dello stesso scenario **non sono confrontabili** senza `R` in mezzo»*

∴ con diciannove scenari in fila, **dimenticare `R` una volta contamina tutti i verdetti successivi** — e
non se ne accorge nessuno, perché il gioco continua a funzionare e ogni scena sembra plausibile.

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
