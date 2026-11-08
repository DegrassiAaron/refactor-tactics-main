# E38 — I numeri degli Action Point: simulazione delle forme di turno, proposta e asserti (`MSP-1.n`)

> `PROPOSED` · **Owner**: E38 (economia del turno) · **Scritto**: 2026-11-08 · **Nasce da**: [D-584](../../decisions/RT_PDR_00_Decision_Log.md) (3)
> *«Budget e costi li propone E38 su una simulazione del catalogo attuale (quali combinazioni di oggi restano legali,
> quali nuove entrano), e l'autore li accetta in una sessione successiva»* · **Modello**: [ADR-0011](../../decisions/adr-0011-action-point.md)
> ⛔ **Nessun numero qui è in vigore.** Finché una `D-nnn` non accetta un vettore, il catalogo non porta costi e il codice
> esegue gli slot. Questo documento è la misura che la decisione chiederà di leggere, con il comando che la produce.

## 1. Che cosa si è misurato, e come

Una **forma di turno** è il multinsieme delle classi di fase delle azioni che un'unità dichiara in un turno:

| Classe | Significato | Esempi dal catalogo di oggi |
|---|---|---|
| `P` | principale che risolve nel **Prep** | `Guard`, `Brace`, `Overwatch`, `Shield`, `CreateCover`, `SuppressiveLine`; `Hero.Muiren.FlowReaction`, `Hero.Branth.Reconfigure`, `Hero.Ivrin.InterceptShot` |
| `D` | mobilità speciale nel **Dash** | `Dodge`, `Charge`, `Leap`, `Reposition`; `Hero.Ivrin.PassingBlade`, `Hero.Muiren.FluidTrail` |
| `B` | principale che risolve nel **Blast** | `BasicAttack`, gli attacchi, `Heal`, `Push`, `Pull`, `Root`, `Slow`, `Interrupt`, `Cleanse`, `MarkTarget`, `ModifyArc`; gli attacchi d'eroe |
| `M` | camminare nel **Move** | `Move`, `Sprint`, `Withdraw`, `Sneak` |
| `R` | reazione | `Counter`, `Intercept`, `Deflect`, `Anchor`, `Purge`, `Evade` |
| `C` | principale che risolve nel **Cleanup** | `CreateWater`, `Ignite`, `CreateSmoke`, `Electrify` |

**Oggi** (slot): al più una fra `D` e `M`, al più una fra `P`, `B` e `C`, al più una `R`. Sono **23 forme**, elencate dal
comando. **Con D-584**: nessun vincolo di slot; una forma è legale se la somma dei costi delle sue classi non supera
il budget; più azioni della stessa classe sono ammesse (scelta (1) di D-584).

Il comando enumera i vettori di costo `P D B M R C` con valori 1–3 e budget 3–8, scarta quelli che perdono anche una
sola forma legale oggi, e ordina i rimanenti per **quante forme nuove a 2–3 azioni** introducono, poi per quelle a
4 o più azioni. Il criterio è dell'assistente ed è dichiarato: *il vettore migliore è quello che rende legale tutto
ciò che è legale oggi aggiungendo il minimo di combinazioni nuove*, perché ogni forma nuova è una combinazione che
bot, HUD e bilanciamento devono imparare.

```
python tools/balance/ap_forms_sim.py
```

Il comando stampa la tabella del §2 e la lettura del vettore della specifica (§3). 🔁 Dal 2026-11-08 l'input per i costi è il
«Master Game Design Document» ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3)), che porta gli stessi costi di fase della specifica
(Prep 1 · Dash 2 · Blast 2, Ultimate 3 · Reaction 1–2) più i movimenti di §2.3: il §3 vale anche per lui. È una misura su **classi di
fase**, non sulle singole azioni: non vede cooldown, portata, né il fatto che due reazioni hanno bisogno di due trigger.

## 2. I vettori che non perdono nulla, ordinati per collaterale

| nuove forme (2–3 azioni) | nuove forme (4+ azioni) | budget | max azioni | costi `P D B M R C` | che cosa diventa legale, in parole |
|---|---|---|---|---|---|
| 25 | 7 | **5** | 5 | `2 2 2 2 1 2` | **due azioni qualunque più una reazione** (anche due nella stessa fase: `BB`, `PP`, `DD`, `MM`, `CC`, `BP`, `BC`, `CP`, `DM`), oppure un'azione e più reazioni |
| 32 | 15 | 4 | 4 | `2 1 2 1 1 2` | un'azione pesante più due leggere fra Dash, Move e reazione; tre movimenti |
| 40 | 27 | 5 | 5 | `1 2 2 2 1 2` | come la prima riga, più il Prep gratuito accanto a tutto |
| 60 | 28–84 | 6 | 6 | `1 2 2 1–2 2 2` (la specifica) | tre azioni qualunque, sei Prep, quattro reazioni |
| 60 | 0 | 3 | 3 | `1 1 1 1 1 1` | tre azioni qualunque: gli AP contano le azioni |
| 60 | 0 | 6 | 3 | `2 2 2 2 2 2` | identico al precedente, scala doppia |

Lettura: con costi uniformi gli AP non dicono niente che il conteggio delle azioni non dicesse (`BBB` è legale quanto
`BDR`). La differenziazione che **costa meno collaterale** è una sola: la reazione costa la metà di tutto il resto.

## 3. Il vettore della specifica, letto sulle cinque fasi

La specifica dà `Prep 1 · Dash 2 · Blast 2 (Ultimate 3) · Reaction 2` su **6 AP** e non prezza il camminare. Letto
sulle cinque fasi con `M = 1` o `M = 2`: nessuna forma di oggi si perde, entrano **60 forme nuove a 2–3 azioni** e da
28 a 84 a 4 o più azioni, con turni fino a **sei** azioni (`PPPPPP`). Il referto §9 lo aveva già misurato in piccolo:
`Prep 1 + Dash 2 + Ultimate 3 = 6` non lascia la reazione.

## 4. Proposta (dell'assistente, da accettare o respingere)

**Budget 5. Ogni azione costa 2, la reazione costa 1.** In parole: *un'unità fa due cose e tiene una reazione pronta,
in qualunque fase le due cose cadano*. È il vettore con il minor numero di forme nuove fra quelli che non perdono
nulla; le forme nuove sono esattamente «due azioni nella stessa fase» e «un'azione e più reazioni».

Conseguenze che la proposta porta con sé, da decidere con il vettore:

1. **Più reazioni per turno.** Con `R = 1` un'unità può dichiararne fino a tre al posto di un'azione. Ogni reazione ha
   un trigger proprio, quindi non è la stessa cosa di «tre colpi»; ma il catalogo reazioni oggi assume **una** reazione
   armata (`ARTUnit` ne tiene una). O il budget la limita (`R = 2` e budget 6: diventa «tre azioni qualunque», 60 forme
   nuove), o il contratto delle reazioni accetta una lista. È una domanda per l'autore, non una taratura.
   ✅ **Decisa il 2026-11-08 ([D-592](../../decisions/RT_PDR_00_Decision_Log.md))**: lista di reazioni, limite solo dal budget. La proposta non ha
   più prerequisiti aperti: il vettore va all'accettazione in #4012 dopo gli asserti.
2. **Le mobilità d'eroe e il camminare costano uguale** (2): `Dash + Move` nello stesso turno diventa legale (`DM`), e
   D-412 (`Sprint` ×2 budget) resta un fatto dei Movement Point, non degli AP.
3. **L'Ultimate non ha un costo proprio.** La specifica lo prezza 3; qui gli attacchi pesanti continuano a pagare in
   cooldown (D-584 (4)). Dare un costo 3 a una classe «ultimate» è una colonna in più per riga, non una classe di fase.
4. **Il bot** esplora combinazioni di due azioni più una reazione: lo spazio cresce ma resta enumerabile per unità.

## 5. Gli asserti, prima dei numeri (il primo consegnabile)

Da scrivere **verdi sul modello a slot** dove possibile, così che diventino rossi solo quando E38 cambia il validatore:

| Asserto | Che cosa fissa | Dove |
|---|---|---|
| `Plan.LegalTodayStaysLegal` | le 23 forme di §1 sono accettate dal validatore, prima e dopo | fixture: la lista stampata dal comando |
| `Plan.SumOverBudgetIsRejected` | un piano la cui somma supera il budget è rifiutato con un reason code proprio | `ValidatePlan` |
| `Resolution.SamePhaseOrderIsDeterministic` | due azioni della stessa unità nella stessa fase risolvono nello stesso ordine su due run con lo stesso snapshot (priorità del catalogo, poi criterio stabile) | resolver, TurnLog, hash |
| `TurnLog.PlanIsAListNotThreeSlots` | il piano serializzato porta una lista di azioni con fase e costo, e la versione dello schema è avanzata | snapshot, replay |
| `Catalog.EveryActionDeclaresACost` | nessuna riga del catalogo, universale o d'eroe, ha costo mancante o zero (salvo `Wait`) | catalogo |

Il quinto non può essere verde oggi: è l'asserto che la voce dei numeri rende scrivibile.

## 6. Limiti di questa misura

- Classi, non azioni: `BB` può essere `BasicAttack + Heal` o `HeavyAttack + HeavyAttack` (quest'ultimo lo nega il cooldown, non il budget).
- Le reazioni sono contate come azioni dichiarate, non come trigger scattati.
- Il catalogo d'eroe letto è quello dei quattro eroi a runtime (undici abilità: sei nel Blast, tre nel Prep, una nel
  Dash, una di controllo); Ward e Vigil (E35.1) non sono ancora a catalogo.
- Nessuna partita simulata: la misura dice che cosa è **legale**, non che cosa è **forte**.

## 7. Stati di verifica

Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. Il comando è stato eseguito il 2026-11-08
sul branch `docs/master-spec-csharp-spec-panel-2026-11-07` e la tabella del §2 è la sua uscita, riordinata per leggibilità.
