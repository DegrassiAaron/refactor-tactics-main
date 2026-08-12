# Referto — economia delle azioni, accoppiamento col movimento e costi del facing

> `HISTORICAL` · **Referto di triage**, non una fonte. · **Data**: 2026-08-12 · **Base**: `4072a1e9`
> **Sorgente esaminato**: `CLAUDE_ActionEconomy_Movement_Facing_Consolidation_2026-08-12.md`, archiviato in
> [`../../archive/src/handoff/2026-08-12-action-economy-movement-facing.md`](../../archive/src/handoff/2026-08-12-action-economy-movement-facing.md).
>
> **Cosa possiede**: il verdetto sezione per sezione del kit d'autore e le misure che lo sostengono.
> **Cosa non possiede**: nessuna regola. La regola consolidata vive in
> [`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md); le domande aperte
> in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md); i conflitti in
> [`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md).

## 1. Perché il verdetto non è «recepire»

Il kit dichiara sé stesso *«a repository consolidation task, not a greenfield redesign»* e ordina di
preservare il canone più recente (§2). Applicando la sua stessa regola di prevalenza, **quattro delle sue
direzioni portanti sono già decise nel repository in una forma diversa**, e una quinta è vietata per iscritto.

Vale la lezione già registrata in [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §6:
un kit scritto in parallelo al lavoro non conosce ciò che è atterrato nel frattempo. Qui la distanza è
maggiore che altrove, perché l'economia del turno è la parte del canone che ha ricevuto **più** decisioni
esplicite fra il 2026-08-07 e il 2026-08-10 — `D-012`, `D-014`, `D-015`, `D-023`, `D-025`, `D-027`, `D-028`,
`D-033`, `D-045`, `D-060`, `D-068`, `D-070`.

## 2. Le cinque misure che decidono il triage

Nessuna viene dal kit; tutte sono state prese sul branch.

| # | Misura | Comando | Conseguenza |
|---|---|---|---|
| **M1** | L'economia del turno **è a slot**, non a budget: `ERTActionSlot` vale `None · Movement · Main · MovementAndMain · Reaction` | `RTActionDef.h:71` | `ActionCapacity` non è un'aggiunta, è una **sostituzione** del modello |
| **M2** | «Slot ≡ Action Points, cap **2**» è **già consolidato** dal 2026-08-07 | catalogo azioni §«Slot per turno» | il kit ripropone come nuova una equivalenza già registrata, e con un cap diverso |
| **M3** | I Movement Point **esistono**: `FRTActionDef::CostMP`, `FRTHexSimUnit::MoveBudget`, costi interi, `Move` 5 MP · `Sprint` 8 · `Withdraw` 2 | `RTActionDef.h:299`, `RTHexSim.h:34`, catalogo §2.1 | §14 del kit descrive un sistema che c'è; il valore aggiunto è zero |
| **M4** | Il pivot **non si paga in MP**: è un budget in **step per eroe**, e ADR-0008 §5 dichiara che «la rotazione **non consuma slot**» | [ADR-0008](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) §1, §5 · `D-060` | §15 del kit è un **conflitto**, non un aggiornamento |
| **M5** | La compatibilità abilità↔movimento **non esiste**: zero occorrenze fuori dall'archivio | `grep -rn "MovementCompat\|Impaired\|Enhanced" docs/ Source/` | §6, §7 e §8 sono l'unico contributo genuinamente nuovo del kit |

## 3. Verdetto sezione per sezione

Legenda: **già canone** · **già implementato** · **nuovo** · **contraddice** · **meta** (istruzioni al
processo, senza contenuto di dominio).

| § del kit | Verdetto | Dove vive davvero / perché |
|---|---|---|
| §0–§2 audit, prevalenza | **meta** — e coincide con [`AGENTS.md`](../../../AGENTS.md) | nessuna azione |
| §3 struttura del round | **già canone** | [`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md) |
| §4 anatomia del piano (Prep + Dash + Main + Move + Facing) | ⚠️ **contraddice** | `Guard`, `Brace` e `Overwatch` occupano la **principale** (catalogo §1, §4). «Prep e Main coesistono» non è una precisazione del modello: è la sua sostituzione. Registrato come `AE-1` |
| §5 Action Capacity / ActionBudget | ⚠️ **contraddice** (M1, M2) | il modello a slot è `D-028`; l'equivalenza con gli AP e il cap 2 sono del 2026-08-07. Registrato come `AE-1` |
| §6 sovrapposizione movimento/azioni | ✅ **nuovo** | recepito nella spec §4 come `PROTOTYPE` |
| §7 compatibilità abilità↔movimento (`NORMAL/IMPAIRED/ENHANCED/BLOCKED`) | ✅ **nuovo** (M5) | recepito nella spec §4; feature `RT-FEAT-ACTION-MOVEMENT-COMPAT` |
| §8 modificatori derivati dal percorso | ✅ **nuovo** | recepito nella spec §4.3. ⚠️ da tenere separato da §7: sono due dipendenze diverse (**profilo** contro **fatti del percorso**) |
| §9 il Dash resta mobilità speciale | **già canone** | `D-015`, `D-028`, [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §1 |
| §10 vincolo Overwatch | **già canone, con formulazione migliore** | `D-070`: l'Overwatch **riserva** lo slot movimento a `Withdraw`, quindi il divieto di Dash è una *conseguenza*. ⚠️ Il reason code `OverwatchDisallowsVoluntaryDash` proposto dal kit è **precisamente** la regola a sé che D-070 dichiara non necessaria |
| §11 Guard / Brace / Overwatch distinti | **già canone** | `D-025` (sette generiche), `D-047`, catalogo §4 |
| §12 `Wait` | **già canone e già implementato** | `Actions.Wait.AllowsFacingAndReaction`. ⚠️ Nota di lettura: la reazione non è un premio del `Wait`, è uno **slot indipendente** — chi si muove e agisce la conserva comunque |
| §13 cooldown + risorsa persistente | **parzialmente già deciso** | `CooldownTurns` è a catalogo e implementato. La domanda «condivisa o per personaggio» è **già chiusa**: risorsa **firma per eroe**, cap 4, ricarica 1 (catalogo eroi §Risorsa firma). Resta aperto il nome player-facing: `AE-4`. ⚠️ **E c'è un conflitto interno preesistente**, vedi §4 |
| §14 Movement Points | **già implementato** (M3) | `CostMP`, `MoveBudget`, costi interi |
| §15 il pivot consuma MP | ⚠️ **contraddice** (M4) | ADR-0008 §1. **Non apre un ID nuovo**: è già `FAC-12`, riga 68 della matrice dei conflitti, aperta il 2026-08-10 da `Facing_Claude_Consolidation`. Aggiornata con la riproposizione |
| §16 locomotion facing vs pivot tattico | **già canone** | ADR-0008 §2, `spec-tassonomia-movimento.md` §3-bis. Il kit descrive la distinzione che il canone ha già, con un vocabolario diverso (`FAC-10` la fissa: **pivot** = capacità, **rotazione dichiarata** = atto) |
| §17 le azioni orientano | **già canone, non implementato** | ADR-0008 §3 (`ERTActionFacingPolicy`), `D-060`. Il gate `runtime` di `RT-FEAT-MAP-FACING` è `partial` per questo |
| §18 modello dati suggerito | **misto** | `Phase`, `ActionCost`(=`Slot`), `CostMP`, `CooldownTurns`, `Targeting`, `FacingPolicy` esistono o sono decisi; `MovementCompatibility`/`MovementModifiers` sono nuovi; `EnergyCost` **non è nel catalogo azioni**, vedi §4 |
| §18 reason code | ⚠️ **da non duplicare** | il TurnLog ha già famiglie serializzate per fase (`ERTMoveOutcome`, `ERTCombatOutcome`, `ERTFallbackOutcome`, `ERTMovementStopReason`) e l'invariante «valori nuovi **in coda**». Undici codici nuovi in un enum nuovo rifarebbero l'errore già respinto in `spec-tassonomia-movimento.md` §6 (*«§36 dieci reason code nuovi: duplicati»*) |
| §19–§20 HUD e Ghost Timeline | **nuovo sul contenuto, esistente sulla forma** | `RT-FEAT-UI-PLANNING`, `RT-FEAT-UI-ACTION-GHOSTS`, [`progettazione-hud.md`](../../technical/progettazione-hud.md). Il feedback dinamico ha senso **solo se** §7 viene adottata: senza compatibilità non c'è nulla da mostrare |
| §21–§22 documenti e Wiki | **meta** | eseguito |
| §23 Feature Registry | **misto** | 5 ID su 9 **esistono già** con quel nome esatto (`ACTION-GENERIC`, `ACTION-MOVE-PROFILES`, `ACTION-DASH-DISPLACEMENT`, `ACTION-COOLDOWNS`, `MAP-FACING`). 3 sono nuovi. `RT-FEAT-ACTION-RESOURCE` **non si crea**: la risorsa vive fra `ACTION-COOLDOWNS` (titolata «Cooldown ed economia delle risorse») e `ACTION-SUPERS` |
| §24 ordine di roadmap | ✅ **utile e recepito**, con una correzione | l'ordine proposto è sensato, ma mette al punto 1 un validatore di piano che presuppone il modello del punto 2. Invertiti: prima si decide `AE-1`, poi si valida |
| §25 Epic | ✅ **recepito** come **E38**, release **v0.2** | nessuna epic esistente la possiede: E4 e E16 sono **chiuse**, E14 possiede le reazioni, E11 la HUD |
| §26 issue AE-001…AE-014 | **filtrate** | vedi §5 |
| §27 aggiornamento Overwatch | **parzialmente respinto** | il cross-reference si aggiunge; la riga «no voluntary Dash» si riscrive secondo `D-070` |
| §28 scenari AE-S01…S12 | **filtrati** | vedi §6 |
| §29 editor map | **rinviato, non respinto** | authoring e debug dei costi hanno senso quando `AE-1` è decisa. Nessuna seduta editor si apre oggi |
| §30 workbook di bilanciamento | 🔴 **respinto per iscritto** | [`docs/balance/README.md`](../../balance/README.md) vieta la correzione cella per cella e `D-023` declassa il workbook a `RESEARCH`. È lo stesso errore che `D-106` e `D-112` hanno appena chiuso due volte |
| §31 test e determinismo | **già canone** | invarianti #2, #4; `AGENTS.md` §Test |
| §32 rete e privacy | **già canone** | invariante #6, ADR-0004 §7-bis, `RT-FEAT-NET-PRIVATE-PLANNING` |
| §33 Definition of Done | **già canone** | i dieci punti sono i gate del Feature Registry, con nomi diversi |
| §34 decisioni «correnti da questa chat» | **da filtrare, non da recepire in blocco** | 12 righe su 24 sono già canone; 3 contraddicono; 5 sono nuove; 4 sono riformulazioni |
| §35–§36 domande aperte | ✅ **recepite** | `AE-1`…`AE-7` in `OPEN_DECISIONS.md`, più l'aggiornamento di `FAC-12` |
| §37–§38 commit e checklist | **meta** | |
| §39 «short canonical design statement» | ⚠️ **non citabile così com'è** | contiene *«voluntary Pivot may consume Movement Points»*, che è `FAC-12` e non una decisione. La formulazione canonica è nella spec §1 |

## 4. Un conflitto che il kit non ha causato ma fa emergere

§13 chiede di documentare la separazione fra risorsa e cooldown. Facendolo si vede che **la risorsa ha tre
valori in tre posti**:

| Dove | Cap | Ricarica | Natura |
|---|---:|---|---|
| `ARTUnit::MaxEnergy` (`RTUnit.h:114`) | **100** | `EnergyPerTurn = 25`, `EnergyOnHit = 15` | parametri dell'MVP quadrato, mai rivisti |
| Catalogo eroi §«Risorsa firma» | **4** | **1** sul trigger d'affinità | canone, per eroe, con nome proprio |
| Catalogo azioni §«Slot per turno» | 4 | 1 | ripete il canone |

E il **costo** in risorsa non è un campo del catalogo azioni: `EnergyCost` sta su `URTActionData`
(`RTActionData.h:137`), il data asset legacy, non su `FRTActionDef`. Quindi un'azione del catalogo v0.1 non
sa dichiarare quanto costa in risorsa.

Non è materia di questo consolidamento — non l'ha introdotta il kit e non la chiude un documento — ma è
esattamente ciò che `AE-4` deve trovare quando verrà aperta, ed è registrato in
[`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) perché non sparisca di nuovo.

## 5. Le 14 issue proposte, filtrate

| Kit | Esito | Perché |
|---|---|---|
| AE-001 schema del piano | **assorbita** dalla spec e da `AE-1` | lo schema non si consolida prima di sapere se è a slot o a budget |
| AE-002 Action Capacity | **diventa decisione**, non implementazione | `AE-1` |
| AE-003 profili di Move nel validatore | **parziale**: i profili esistono, il validatore no | confluisce in E38 |
| AE-004 compatibilità e modificatori | ✅ **aperta** | il contributo nuovo |
| AE-005 MP + pivot | **diventa decisione già aperta** | `FAC-12`, non un ID nuovo |
| AE-006 facing indotto dalle azioni | ❌ **non si apre**: esiste | ADR-0008 §3, feature `RT-FEAT-MAP-FACING`, issue `#291` |
| AE-007 contratto della risorsa | ✅ **aperta come design**, con la §4 dentro | `AE-4` |
| AE-008 HUD | ✅ **aperta**, dipendente da AE-004 | |
| AE-009 Ghost Timeline | **assorbita** in AE-008 | una sola issue di preview: separarle produrrebbe due modi di dire lo stesso stato |
| AE-010 reason code | ✅ **aperta**, riformulata | *estendere* le famiglie esistenti, non crearne una |
| AE-011 test golden | **assorbita** nelle issue che introducono le regole | un test senza la sua regola non ha oracolo |
| AE-012 scenari | ✅ **aperta** | §6 |
| AE-013 privacy del piano | ❌ **non si apre**: coperta | `RT-FEAT-NET-PRIVATE-PLANNING`, `#589` |
| AE-014 matrici di bilanciamento | 🔴 **respinta** | §30 |

## 6. I 12 scenari proposti, filtrati

I `ScenarioId` seguono la convenzione `Spec.<Area>.<Fatto>` già in uso; `AE-Snn` non è un formato del
repository.

| Kit | Esito | ScenarioId |
|---|---|---|
| AE-S01 densità da fermo | **assorbito** | nessun fatto proprio: è il caso base di ogni scenario esistente |
| AE-S02 il Move modifica un'azione di precisione | ✅ `planned` | `Spec.ActionEconomy.MoveImpairsPrecision` |
| AE-S03 lo Sprint blocca | ✅ `planned` | `Spec.ActionEconomy.SprintBlocksPrecision` |
| AE-S04 lo Sprint potenzia | ✅ `planned` | `Spec.ActionEconomy.SprintEnhancesMomentum` |
| AE-S05 Dash + attacco + Move | ❌ **contraddice** `D-028` | come regola generale non è legale: lo scatto **occupa** lo slot movimento. Un eroe può dichiararlo nel proprio kit |
| AE-S06 Overwatch + Dash rifiutato | ✅ `planned`, riformulato | `Spec.ActionEconomy.OverwatchReservesMovementSlot` — asserisce la **causa** di `D-070`, non un divieto a sé |
| AE-S07 Brace + attacco + movimento | ❌ **contraddice**: `Brace` è principale | il caso «prepared + main» non esiste finché `AE-1` non è decisa |
| AE-S08 il pivot accorcia il percorso | ⏸️ **bloccato su `FAC-12`** | si scrive quando la decisione esiste, non prima |
| AE-S09 nessun doppio addebito del pivot | ⏸️ **bloccato su `FAC-12`** | idem |
| AE-S10 cooldown/risorsa bloccano | ✅ `planned` | `Spec.ActionEconomy.CooldownBlocksWithSlotFree` — copre il buco dichiarato di `RT-FEAT-ACTION-COOLDOWNS` (`scenario: todo`) |
| AE-S11 la lunghezza del percorso cambia l'effetto | ✅ `planned` | `Spec.ActionEconomy.PathLengthChangesEffect` |
| AE-S12 privacy | **assorbito** | `RT-FEAT-NET-PRIVATE-PLANNING` lo possiede già |

## 7. Cosa questo referto non ha fatto

- **Non ha assegnato un numero `D-nnn`.** Le tre direzioni portanti del kit sono domande, non decisioni, e
  `AGENTS.md` vieta di sceglierle per plausibilità. Chi decide è l'autore.
- **Non ha toccato il codice.** Il kit stesso lo esclude (§0).
- **Non ha toccato il workbook** (§30).
- **Non ha aperto sedute editor** (§29): senza `AE-1` non si sa cosa si autora.
