# CP 14.5 — spec panel sulla issue #165 e sul lavoro consegnato

> `CURRENT` · **Stato**: revisione chiusa, **applicata** a `#165` e `#166`; `#886` e `#888` aperte ·
> **Data**: 2026-08-14
> **HEAD della revisione**: `44a6af96` (`feat/165-finestra-overwatch`, worktree `D:/rt-simulation`)
> **Oggetto**: [`#165`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/165) come pubblicata
> (riconciliata il 2026-08-12 e corretta il 2026-08-13, `D-128`), più la **PR
> [#878](https://github.com/DegrassiAaron/refactor-tactics-main/pull/878)** che la implementa,
> [`adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) e
> [`adr-0009-replay-logico-canonico.md`](../../decisions/adr-0009-replay-logico-canonico.md).
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Crispin
> **Modo**: critique
> **Particolarità**: è la prima revisione di questa serie che legge una issue **contro il codice che la
> chiude**, non contro un piano. Il DoD non è più un'ipotesi: si può misurare quanto di esso il lavoro
> abbia potuto verificare, e le voci che nessun lavoro avrebbe potuto verificare si vedono per differenza.

---

## 1. Il verdetto in una riga

Il lavoro consegnato è più completo del DoD che lo chiedeva; ciò che resta aperto **non è lavoro mancante,
è specifica mal collocata** — due caselle di `#165` non sono falsificabili in questo checkpoint per
costruzione, e una terza è verificata su un soggetto diverso da quello che nomina.

| | Voci |
|---|---:|
| 🔴 Critico | **2** |
| 🟠 Alto | **3** |
| 🟡 Medio | **1** |
| 🟢 Basso | **1** |

**Raccomandazione operativa**: `#165` **si chiude** — le caselle sono state spezzate e le metà non
falsificabili riallocate (§10). `C2` aveva bisogno di una destinazione prima della chiusura di `#166`, e ora
ce l'ha: **`#886`**.

---

## 2. 🔴 C1 — La prima casella tiene due requisiti con testabilità opposta

> - [ ] Finestra da **3.0 s**; `FIRE` consuma la charge, `HOLD` **mantiene armata** la reaction e una nuova
>   opportunity può ancora comparire

**Wiegers**: sono due requisiti, non uno, e la loro verificabilità in CP 14.5 è opposta.

| Metà | Stato | Evidenza |
|---|---|---|
| `FIRE` consuma la charge, `HOLD` la mantiene | ✅ consegnata e testata | `FRTArmedOverwatch::bCharged`, azzerato solo in `ApplyReactionDecision`; `Overwatch.HoldKeepsArmed` verifica **le due metà in opposizione** (armato apre, speso non apre) |
| Finestra da **3,0 s** | ❌ non falsificabile qui | `grep -rn "FastReactionDuration" Source/` → **una sola occorrenza, dentro un commento**. Nessun decisore attende: il bot risponde subito, i test rispondono subito, l'umano non ha UI |

Una casella non si spunta a metà. Lo stato di `#165` è quindi indeciso per una ragione che **non riguarda il
lavoro fatto**, ed è il difetto: il DoD costringe a scegliere fra dichiarare falso ciò che è vero e dichiarare
vero ciò che nessuno ha verificato.

**La collocazione giusta esiste già.** ADR-0004 §8 dichiara `FastReactionDuration` **baseline di sistema per
ogni Fast Reaction**, non deliverable di questo checkpoint; e `#166` ha già il consumatore — *«UI `FIRE`/`HOLD`
con **countdown** e bersaglio»* — ma **non nomina né il valore né il timeout**. La casella arriverebbe su una
issue che la può falsificare, e chiuderebbe una lacuna che quella issue ha già.

**Adzic** — perché non dichiararla ora come costante testata, sul modello di `MaxPromptsPerReaction`:
la PR risponde *«sarebbe un dato senza consumatore»*, ed è coerente con la disciplina del repository. Ma la
simmetria va detta: `MaxPromptsPerReaction` è un parametro **della stessa tabella** ADR-0004 §8, è entrato in
questo stesso checkpoint come `constexpr`, ed è testato con `TestEqual(Cap, 3)`. La differenza che regge è una
sola, e va scritta invece che sottintesa: **quello ha un lettore** (`ResolveReactionBoundary` salta il watcher
che l'ha esaurito), questo no. Il criterio non è «costante sì / costante no», è la catena *dichiarato ·
trasportato · letto*.

### Correzione proposta a `#165`

```markdown
- [x] `FIRE` consuma la charge, `HOLD` **mantiene armata** la reaction e una nuova opportunity può
      ancora comparire
- [ ] ~~Finestra da **3.0 s**~~ — **spostata in CP 14.6 (#166)**: `FastReactionDuration` è una baseline
      di sistema (ADR-0004 §8), non un deliverable di questo checkpoint, e qui non ha lettore — nessun
      decisore attende. Il suo consumatore è il countdown della UI.
```

### Correzione proposta a `#166`

```markdown
- [ ] Il countdown della UI dura `FastReactionDuration` = **3,0 s** (ADR-0004 §8), valore unico e
      **server-authoritative**: un client lento non allunga la finestra, e allo scadere l'esito è
      `HoldTimeout` — mai `FIRE`
```

---

## 3. 🔴 C2 — «il replay la riproduce senza reinterrogare nessuno» è verificata su un altro soggetto

> - [ ] La decisione entra nel TurnLog (…) e il **replay la riproduce** senza reinterrogare nessuno

**Crispin**: il test che porta il nome del requisito verifica **la serializzazione**, non il replay.

`Overwatch.DecisionIsReplayable` (`RTOverwatchTriggerTests.cpp:979–1065`) costruisce una `FRTTurnLogEntry`
**a mano**, la serializza, la rilegge, confronta gli hash e verifica la metà negativa (bersaglio diverso ⇒
hash diverso; id vuoto ⇒ i campi v8 non perturbano). È un buon test — la sua stessa nota registra che una
versione precedente confrontava «copia contro copia» e non dimostrava nulla — ma dimostra che **il formato
regge**, non che qualcuno rilegga.

**Misura**: `grep -rn "OpportunityId" Source/` → fuori da `RTTurnLog.h` (dichiarazione), dalla
serializzazione e dai test, **nessun consumatore legge `FRTTurnLogEntry::OpportunityId`**. È lo stesso
argomento con cui la PR rifiuta `FastReactionDuration`, applicato all'altra metà della stessa casella.

**Fowler** — e non è una svista, è un confine che ADR-0009 disegna e che nessuno ha ancora attraversato:

| Prodotto (ADR-0009) | Autorità | La proprietà del DoD |
|---|---|---|
| **Player** (`URTReplaySeekLibrary`, `Replay.Player.RunsWithoutResolver`) | la **traccia** | vera **per costruzione**: non calcola, quindi non interroga nessuno |
| **Verifier** (`Replay.Verifier.ResimulationIsDeterministic`) | il **resolver** | ri-simula. Oggi regge **per assenza**, non per costruzione |

`Replay.Verifier.ResimulationIsDeterministic` rilancia `URTScenarioRunner::Run` sullo **scenario**, non sul
TurnLog. Oggi non diverge perché il decisore umano non esiste (`ReactionDecider` non legato ⇒
`HoldNoDecider`) e la politica del bot è pura.

**Nygard — lo scenario di rottura, per data e per causa.** Alla chiusura di `#166` esiste una UI, quindi
esiste una decisione umana che vive **solo** nel TurnLog:

```
partita reale : micro-step 2, il giocatore risponde FIRE:7 → traccia con FireChosen, unità 7 troncata
il Verifier   : ri-simula lo scenario, ReactionDecider non legato → HoldNoDecider
                unità 7 non viene fermata → le collisioni dei micro-step 3..n divergono
                → replay divergence ≠ 0, con un resolver corretto
```

Il seam **esiste già** ed è ben separato — `AskReactionDecision` (decide) e `ApplyReactionDecision`
(applica), con il commento che dichiara *«rieseguire un turno significa saltare questa e chiamare solo
quella, con le risposte lette dal TurnLog»*. Ciò che manca è **chi le legge**.

**Destinazione: nessuna, oggi.** `#166` non ha una casella per il seam. `#813` (CP 45.6, `v1.0`) è
l'**audit** — *«replay divergence = 0 sul campione»* — e presuppone il seam invece di costruirlo: sarebbe la
issue che scopre il difetto, non quella che lo previene.

### Correzione applicata — issue propria, [`#886`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/886)

Issue propria e non casella di `#166`, perché è un requisito di ADR-0009 e non della UI: dentro il DoD di
`#166` diventerebbe una rifinitura della finestra e sparirebbe alla sua chiusura. `#166` la nomina nella
**Chiusura** come propria conseguenza. Il DoD di `#886`, in sintesi:

```markdown
- [ ] Il **Verifier** ri-simula un turno con decisioni **leggendole dal TurnLog** invece di richiederle:
      `OpportunityId` è la chiave, e una risposta registrata vince sul decisore corrente
      → `Replay.Verifier.ReactionDecisionsComeFromTheTrace`
      ⚠️ Senza, il primo `FIRE` umano rende `Replay.Verifier.ResimulationIsDeterministic` falso con un
      resolver corretto. Oggi la proprietà regge **per assenza di decisore**, non per costruzione.
```

---

## 4. 🟠 A1 — La misura che il DoD chiede non passa dal codice che il gioco esegue

> - [ ] **Misura dell'overhead della risoluzione segmentata** con un decisore di test locale a questo
>   checkpoint (risposte immediate, nessun timer): durata registrata come **limite inferiore**

**Nygard**: la misura c'è, il numero è dichiarato onestamente come limite inferiore, e il test rifiuta
correttamente di asserire una soglia temporale. Ma `Overwatch.SegmentedResolutionOverhead`
(`RTOverwatchTriggerTests.cpp:1126–1228`) **riscrive il ciclo di `ResolveMovement` dentro il test**:

| Cosa | Nel gioco | Nella misura |
|---|---|---|
| il ciclo `Begin`/`ResolveNext`/`Finish` | `ARTTurnManager::ResolveMovement` | riscritto inline |
| costruzione dei watcher | `ResolveReactionBoundary` | un watcher costante, costruito una volta |
| **cap dei prompt** | `ResolveReactionBoundary`, prima di costruire il watcher | **assente** → 5 finestre invece di ≤ 3 |
| proiezione della Team Knowledge | ricalcolata a ogni micro-step | **assente**, dichiarata assente |
| decisione | `AskReactionDecision` → bot o delegate | `IsResponseAllowed(...)` scartata con `(void)` |

Il numero della PR — `5 finestre costruite senza cap` — descrive quindi un sistema che il gioco non esegue.
La PR lo dichiara; **la issue no**, e il difetto è della specifica: il DoD chiede una misura senza nominarne
il **soggetto**, e senza soggetto una replica dello strato puro ne soddisfa la lettera. Una replica può poi
divergere dall'originale senza che nessun test se ne accorga — che è il costo vero, più del numero.

**Wiegers**: la casella non va allargata, va **qualificata**. Delle due formulazioni, la seconda è quella che
il lavoro ha già pagato:

```markdown
- [ ] Misura dell'overhead della risoluzione segmentata con un decisore a risposte immediate, presa
      **sullo strato puro** (`Begin`/`ResolveNext`/`Finish` + `BuildOverwatchTriggers`) e non sul
      percorso di partita: durata registrata come **limite inferiore**, con l'elenco esplicito di ciò
      che la replica non include — cap dei prompt, proiezione della Team Knowledge, decisione reale
```

---

## 5. 🟠 A2 — Il DoD elenca cinque campi; il TurnLog ne consegna tre con quel nome, uno rinominato, uno assente

> - [ ] La decisione entra nel TurnLog (`OpportunityId`, `ReactionInstanceId`, `DecisionBoundary`,
>   `Response`, `SelectedTargetId`) …

| Campo chiesto | Consegnato | Nota |
|---|---|---|
| `OpportunityId` | ✅ `FRTTurnLogEntry::OpportunityId` | entra nell'hash |
| `ReactionInstanceId` | ✅ | fuori dall'hash, motivato (numero d'ordine, non discriminante) |
| `SelectedTargetId` | ✅ come `SelectedTargetUnitId` | entra nell'hash |
| `Response` | ⚠️ sostituito da `ERTReactionDecisionOutcome` | **più** informativo: separa `HoldChosen` da `HoldTimeout`, da `HoldNoDecider`, da `HoldRejected` |
| `DecisionBoundary` | ⚠️ **deliberatamente assente** | il micro-step è già uno dei sei campi della chiave da cui `OpportunityId` è derivato; un campo proprio sarebbe una seconda copia dello stesso fatto |

**Cockburn**: entrambe le divergenze sono migliorie, ed entrambe sono state decise **in implementazione**.
Il difetto non è nella scelta, è nella forma della casella: un DoD che elenca **campi** invece di
**proprietà** mette chi implementa davanti a un aut-aut fra la lettera e la cosa giusta, e comunque scelga la
casella resta ambigua da spuntare. La proprietà vera è la riga successiva — *«il replay la riproduce senza
reinterrogare nessuno»* — ed è quella su cui `C2` insiste.

**Correzione proposta**: riscrivere l'elenco come **non normativo** («almeno: identità della finestra, esito,
bersaglio scelto, istanza della reaction») e registrare nella issue le due rinegoziazioni con la loro ragione,
perché il prossimo lettore non le scopra diffando header e DoD.

---

## 6. 🟠 A3 — Il cap dei prompt è entrato in corsa, ed è «data-driven» in un ADR e `constexpr` nel codice

Il lavoro qui è **giusto e necessario**: la misura di overhead ha registrato **cinque** finestre per
risoluzione contro le tre che ADR-0004 §8 ammette, perché `Charges = 1` limita i `FIRE` e non le **domande** —
e un `HOLD` non spende la charge. Il cap non è stato dedotto, è stato visto.

Restano due cose non registrate:

1. **Nessuna casella di `#165` lo nomina.** È arrivato col secondo commit, e la sola traccia è la PR. Un
   parametro canonico che entra in vigore senza una riga di DoD è un cambiamento che nessun documento
   sorveglia.
2. **ADR-0004 §8 dice `data-driven`**, testualmente: `| MaxPromptsPerReaction | 3 | §5 sorgente; data-driven |`.
   Il codice dice `static constexpr int32 MaxPromptsPerReaction() { return 3; }`. Chi legge l'ADR domani
   crederà che esista un dato da modificare.

**Fowler**: `constexpr` è la scelta giusta per la v0.1 — non esiste ancora un profilo di reaction su cui
appoggiare il valore, e inventarne uno per rispettare un aggettivo sarebbe peggio. Ma l'aggettivo va emendato
dove sta scritto:

```markdown
| `MaxPromptsPerReaction` | **3** | §5 sorgente; in v0.1 **costante**
  (`URTReactionOpportunityLibrary::MaxPromptsPerReaction`), data-driven quando il profilo di reaction
  esisterà |
```

⚠️ `docs/decisions/adr-0004-finestre-di-reazione.md` è **fuori dal write-set** della track `simulation`
(`parallel-batch.yaml`), quindi l'emendamento non si fa da questo branch: è una riallocazione o un passo di
integrazione. Vedi §9.

---

## 7. 🟡 M1 — Il danno di un `FIRE` non passa dalla geometria della copertura, e non è un difetto di questo checkpoint

`ApplyReactionDecision` applica `URTCombatLibrary::ApplyDamage(Armed.Damage, Shield, Health)` **diretto**.
Il Blast passa invece da `URTHexCombatLibrary::CollectHexAttacks`, che sottrae `EffectiveCoverReduction` e
riporta `CoverBypassedByFacing` (`RTHexCombatLibrary.cpp:357–366`). Conseguenza osservabile: un bersaglio
dietro copertura prende **danno pieno** da un Overwatch e danno ridotto da un attacco base della stessa arma —
mentre il design dice che l'Overwatch *«spara con la propria arma»*.

⚠️ **La verifica dice che non è una deviazione introdotta qui.** `ResolvePredictiveBoundary` — E18 CP 18.2,
**chiusa dal 2026-08-10** — fa esattamente lo stesso (`RTTurnManager.cpp:4819`). Sono due istanze della stessa
scelta implicita: *il danno deciso a un boundary non usa la pipeline di combattimento*. Nessun documento la
prende, né per affermarla né per negarla.

**Adzic**: non è lavoro di `#165`, e allargare il DoD adesso sarebbe scope creep su una issue pronta a
chiudere. È una **domanda aperta con due istanze e nessun proprietario**, che è precisamente ciò che
`OPEN_DECISIONS.md` esiste per tenere:

> Il danno applicato a un decision boundary (Predictive CP 18.2, Overwatch CP 14.5) usa la copertura e il
> facing come il Blast, o è per natura un colpo a bruciapelo che li ignora? Oggi li ignora, in due punti,
> senza che nessuno l'abbia deciso.

✅ **Aperta come [`#888`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/888)** (`question`),
perché `docs/OPEN_DECISIONS.md` è `integration_only` in questo batch e una prescrizione che resta dentro un
referto non ha destinatario. Va elencata in `OPEN_DECISIONS.md` al primo passo di integrazione.

---

## 8. 🟢 B1 — Un commento promette un discriminante che il lookup non usa

`ResolveReactionBoundary`, ritrovando l'armamento di una opportunity:

> *«`OwnerId` è l'indice del proprietario, che può avere due Overwatch: a distinguerli è `ReactionDefId` più
> l'istanza»*

Il codice confronta `Watchers[w].Zone.OwnerUnitId == Opportunity.Key.OwnerId && Watchers[w].ReactionDefId ==
Opportunity.Key.ReactionDefId`. Per due Overwatch della **stessa** unità entrambi i campi sono identici —
l'`ActionId` è sempre `Action.Overwatch` — e `FRTReactionOpportunityKey` non porta l'istanza (i suoi sei campi
sono `TurnNumber`, `MacroPhase`, `MicroStepIndex`, `OwnerId`, `ReactionDefId`, `Seq`). Il secondo armamento
ricadrebbe sull'indice del primo, troverebbe `bCharged` falso dopo un `FIRE` e verrebbe saltato in silenzio.

**Irraggiungibile oggi**: una unità pianifica una sola abilità per turno, quindi due Overwatch armati non si
producono. Non è un difetto in partita — è un commento che documenta una capacità che il codice non ha, e
`ArmedIndexForWatcher` la renderebbe vera con un confronto in più (`Watchers[w].ReactionInstanceId`).

---

## 9. Cosa regge, misurato

Voci controllate che **non** hanno prodotto rilievi, elencate perché una revisione che nomina solo i difetti
non dice quanto ha guardato:

| Proprietà | Come regge |
|---|---|
| `Timeout → HOLD` puro | `DecisionOnTimeout` è costante per costruzione; `Overwatch.TimeoutIsHold` verifica anche la metà negativa (`FireResponseTarget == INDEX_NONE`) su tre cardinalità |
| Nessun `Sleep`/`Delay`/timer nella logica pura | verificato per lettura su `ResolveReactionBoundary`, `AskReactionDecision`, `ApplyReactionDecision` e sullo strato puro: la sospensione è il fatto che la funzione **ritorni** prima del micro-step successivo |
| `FIRE` tronca il residuo, le collisioni cambiano | `StopUnitInPlace` scrive `Done`, `Final` e un `BlockReason` **terminale**; `InterruptionAffectsLaterCollision` verifica che la seconda unità ora **entri** nella cella prima contesa, che una correzione a posteriori non produrrebbe |
| Non-regressione della risoluzione segmentata | `HoldResumesSameMovementState` confronta cella finale, esito **e** celle attraversate di tre unità, con la guardia `Steps > 0` contro il confronto di due array vuoti |
| Il bot decide con la sola opportunity | vero **per firma**: `DecideReactionResponse(const FRTReactionOpportunity&)` non ha altro da cui leggere. `Bot.DecidesWithoutFutureKnowledge` non è vacuo — fissa che con bersagli legali il bot **spari** |
| La opportunity non contiene futuro | già consegnata da CP 14.3, elenco chiuso dei campi per riflessione (`RTReactionOpportunityTests.cpp:159`) |
| Sospensione globale (ADR-0004 §5) | il ciclo non gira mentre una finestra è aperta: nessuna unità avanza, e non perché qualcuno la fermi |

**Non verificato in questa revisione, e va detto**: la suite non è stata rieseguita (nessuna run di UE
automation in questa sessione). «842 verdi su 842» e la verifica di mutazione su `StopUnitInPlace` sono
affermazioni della PR, non misure di questo referto.

---

## 10. Cosa fare, in ordine

| # | Azione | Dove | Stato | Blocca la chiusura di `#165`? |
|---|---|---|---|---|
| 1 | Spezzare la prima casella; la metà «3,0 s» esce da `#165` (`C1`) | body di `#165` | ✅ applicata | **sì** |
| 2 | Voce del countdown a 3,0 s, server-authoritative (`C1`) | body di `#166` | ✅ applicata | no |
| 3 | **Seam del Verifier** (`C2`) | issue propria | ✅ **#886**, più la conseguenza registrata nella Chiusura di `#166` | no, ma **sì** per `#166` |
| 4 | Qualificare la casella della misura col suo soggetto (`A1`) | body di `#165` | ✅ applicata | no |
| 5 | Elenco dei campi del TurnLog reso non normativo + le due rinegoziazioni registrate (`A2`) | body di `#165` | ✅ applicata | no |
| 6 | Casella retroattiva sul cap dei prompt (`A3`) | body di `#165` | ✅ applicata | no |
| 7 | Emendare la riga `data-driven` di ADR-0004 §8 (`A3`) | `docs/decisions/adr-0004-…` | ⏳ **integrazione** — la prescrizione vive nel DoD di `#165` | no |
| 8 | Domanda aperta sul danno ai boundary (`M1`) | `docs/OPEN_DECISIONS.md` | ✅ **#888** (`question`); il file resta da aggiornare in integrazione | no |
| 9 | Il commento che prometteva un discriminante assente (`B1`) | `RTTurnManager.cpp` | ✅ **corretto il commento, non il codice** | no |

⚠️ **Perché la 9 corregge il commento e non il lookup.** Il confronto non può disambiguare due Overwatch
della stessa unità con i dati che ha: `FRTOverwatchTrigger` porta `Opportunity` e `TargetUnitIds`, non
l'indice del watcher da cui nasce, e la chiave non porta l'istanza. Il fix vero è un campo nuovo, cioè un
cambiamento di struttura per un caso che oggi **non si produce** — una unità pianifica una sola abilità per
turno. Il commento ora dice questo, invece di promettere il contrario.

**Write-set** (`D-139`). Delle nove azioni, la **9** cade dentro il `writable` della track `simulation`; la
**7** tocca `docs/decisions/`, che è `integration_only`; le altre vivono su GitHub, fuori dal repository.
Questo referto è a sua volta un file nuovo sotto `docs/roadmap/plans/`, che il batch non assegna a nessuna
track: **riallocazione dichiarata**, con il path aggiunto per **file** e non per prefisso al `writable` di
`simulation`. La ragione è misurata e non prudenziale — `docs/roadmap/plans/` è toccata da **cinque** branch
vivi (`docs/38-spec-panel-playtest-hex`, `docs/five-lane-roadmap`, `docs/lane-6-7`, `docs/lane-7-vault`,
`wip/icon-visual-language`), quindi dichiarare la directory violerebbe l'invariante su file che questa track
non tocca; sul singolo path le collisioni sono **zero**.

**Nessun `D-nnn` riservato.** Le due riallocazioni fra checkpoint non introducono una regola nuova né
superano una decisione esistente: applicano ADR-0004 §8 (`FastReactionDuration` è baseline di sistema) e
ADR-0009 (il Verifier è autorevole sul resolver) alle issue che li avevano collocati altrove. Se il triage
successivo ritenesse che serva comunque un id, non si sceglie a mano:
`python scripts/rt_shared_id.py reserve D`.
