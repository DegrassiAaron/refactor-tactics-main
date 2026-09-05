# Kit «Movement Microsteps / Collision / Facing» — spec panel contro il repository

> **Referto di revisione**, non owner. Sottopone a critique il kit Drive *CLAUDE — RefactorTactics Movement
> Microsteps — Epic v0.1→v1.0 + Issues v0.1 — 2026-08-31* e ne misura ogni claim contro il repository.
>
> **Data**: 2026-08-31 · **Modo**: critique · **Focus**: requirements + testing
> **Base della revisione** (§1–§7): `origin/main` `bbd97482` · **base delle correzioni** (§6.2, §10):
> `origin/main` `188183b9`, dopo il merge di #1929.
>
> ⛔ **Nessun owner doc toccato, nessuna suite eseguita, nessuna riga di `Source/` cambiata.**
> ✅ **Tracking eseguito dopo conferma**: quattro issue agganciate a release ed Epic — #1922, #1733, #1800,
> #1605 — una issue creata, [#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933), e
> quattro commenti di legame su #16, #26, #152, #25. Il dettaglio è in §8.

---

## 1. Il verdetto in una riga

> **Diciannove dei ventidue work package chiesti hanno già un owner nel repository; il kit riapre come
> "intenzionalmente aperta" una decisione che `D-294` ha chiuso oggi; e la sua mappa delle release
> contraddice le dieci release del repository su tre scalini. Il valore reale sono tre gap misurati — e
> nessuno dei tre è nell'elenco per il motivo per cui il kit lo elenca.**

---

## 2. La sovrapposizione che il kit non sa di avere

Il kit dichiara di aver aggiornato il mirror Drive con `AUTHOR-MOVE-001`, `AUTHOR-FACING-002`,
`AUTHOR-PIVOT-001`, `AUTHOR-MOVETIME-001` e `DQA-030`. **Tutte e cinque sono già state consumate dal
repository**, in due pass mergiati nelle ultime ore:

| Riga Drive | Consumata da | Referto | Esito |
|---|---|---|---|
| `AUTHOR-MOVE-001` | [`D-295`](../../decisions/RT_PDR_00_Decision_Log.md) | [`sync-decisioni-movimento-2026-08-31.md`](sync-decisioni-movimento-2026-08-31.md) | 5 clausole su 7 già spedite; 2 aperte in [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) |
| `AUTHOR-FACING-002` | `D-295` | idem | già canone — ADR-0008 §2, `FAC-4`, `D-020` |
| `AUTHOR-PIVOT-001` | `D-295` | idem | già canone — ADR-0008 |
| `AUTHOR-MOVETIME-001` | `D-295` | idem | vera per costruzione: `ResolveHexPaths` non riceve il costo |
| `DQA-030` — KO Occupancy | [`D-294`](../../decisions/RT_PDR_00_Decision_Log.md) | [`sync-governance-dqa-026-030-2026-08-31.md`](sync-governance-dqa-026-030-2026-08-31.md) | ⛔ **CHIUSA — Option A** |

∴ Il kit è la stesura **espansa** di un corpus che il repository ha già assorbito. Applicarlo alla lettera
riaprirebbe lavoro chiuso.

---

## 3. 🔴 La contraddizione da risolvere prima di tutto: `DQA-030`

Il kit §8 istruisce:

> *«Questa decisione resta intenzionalmente aperta. […] Se non esiste un owner più recente, mantieni
> **DQA-030 OPEN** e crea/aggiorna il necessario decision gate.»*

**L'owner più recente esiste, ed è di oggi.** `D-294` la chiude scegliendo **Option A** — l'unità KO smette
di bloccare il proprio `FRTCellId` al commit del segmento che l'ha abbattuta — e non la sceglie per
preferenza: la misura come **comportamento già applicato in tre sedi indipendenti**.

| Sede | Regola misurata |
|---|---|
| `ARTUnit::IsAlive()` | `Health > 0`, **calcolata**, non un flag alzato da una fase |
| `ARTTurnManager::CollectLivingUnits` | filtra sui vivi — *«i morti (es. nel Blast) non si muovono e non bloccano»* |
| `URTHexSimLibrary::MakeSnapshot` · `URTMatchSetupLibrary::BuildOccupancy` | popolano `Occupancy` **solo** con `Unit.bAlive` |

⚠️ **Il residuo che `D-294` lascia non è la scelta, è la sua fragilità.** La regola vale *per costruzione*:
`Occupancy` è congelata dentro un segmento (`Occupancy.Remove` ha **zero** occorrenze in `Source/`) e nessun
micro-step può osservare un occupante stantio solo perché il danno da terreno del `Move` si applica **dopo**
`ResolveMoves`. Un effetto che uccidesse **dentro** la risoluzione rovescerebbe il comportamento senza che
nessuno lo decida. ⛔ **Nessun test lo pinna**: `Match.KODoesNotBlockItsCellNextPhase` non esiste.

**Uscita corretta**: non riaprire `DQA-030`; scrivere il test che `D-294` dichiara mancante. È l'unica azione
che il kit §8 avrebbe prodotto se la sua misura fosse stata fatta sul repository.

---

## 4. 🔴 La mappa delle release del kit non è quella del repository

Il kit §9 propone sette scalini. Il repository ne ha **dieci**, con milestone GitHub attive e una
[roadmap di navigazione](../roadmap-v0.1-v1.0.md) che ne dichiara le quattro soglie.

| Kit | Repository | Conflitto |
|---|---|---|
| v0.2 — *Edge Case Hardening* | **v0.2 · Struttura e finestre** (ms 8, 45 aperte) | ⚠️ nome diverso, contenuto compatibile |
| v0.3 — *Scalable Rule/Data Pipeline* | **v0.3 · Informazione** (ms 9) | ⚠️ contenuti disgiunti |
| **v0.5 — 3v3 Alpha Stress** | **v0.5 · Online Foundation** (ms 12) | 🔴 **incompatibile** |
| v0.7 — *Authoritative Online Integration* | **v0.7 · Competitive Alpha** (ms 14) | ⚠️ parziale — il dedicated server è la SOGLIA 3, fra v0.6 e v0.7 |
| *(assenti)* | v0.4 · Operations · v0.6 · Ability Runtime · v0.8 · Beta | 🔴 tre release non nominate |

🔑 **Il conflitto grave è il 3v3.** Il kit lo colloca in v0.5. Nel repository il 3v3 è la **SOGLIA 1**, fra
v0.1 e v0.2, ed è posseduto da [E24 · Formato Standard 3v3](https://github.com/DegrassiAaron/refactor-tactics-main/issues/325)
in v0.2 — coerente con **D-256**, che `AGENTS.md` §1 e `CLAUDE.md` §2 pinnano entrambi. Adottare la mappa del
kit sposterebbe il formato standard di tre release senza una decisione.

∴ **Gli Epic non si creano.** Esistono, coprono v0.1→v1.0, e le loro release boundary sono owner.

---

## 5. I ventidue work package contro gli owner reali

Misurato con `gh` lato server e con `grep` su `Source/`. **Non è una proposta di issue: è la mappa di chi già
li possiede.**

| # | Work package del kit | Owner reale | Stato |
|---|---|---|---|
| 1 | Canonical simultaneous microstep protocol | `StepHexMovement` — punto fisso monotono; `HexSim.ResolveOrderIndependent` | ✅ spedito |
| 2 | Collect/validate all `MoveProposal` before commit | idem | ✅ spedito |
| 3 | Same-destination contest → BLOCK set | `HexSim.ResolveContestedDestination` · `ResolvePriorityTieStillContested` | ✅ spedito |
| 4 | Occupancy dependency resolver — free convoy | per costruzione (blocca solo chi è fermo) | ⚠️ **nessun test** — [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) |
| 5 | Block propagation da tail fallita | ciclo `while (bChanged)`; `HexSim.ResolveBlockedByStationary` | ✅ spedito |
| 6 | Direct swap block | ⛔ **oggi riesce** — `HexSim.ResolveSwapAllowed` è verde e asserisce l'opposto | 🔴 [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) |
| 7 | Closed-cycle block | ⛔ **oggi ruota**, nessun test | 🔴 [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) |
| 8 | Full `FRTCellId` / Layer occupancy | `FRTCellId::operator==` confronta `Layer` (`RTCellId.h:47`) | ✅ + [#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) per il vincolo in partita |
| 9 | Successful-transition Facing update | ADR-0008 §2 · `Facing.LinearMoveDerivesDirection` | ✅ spedito |
| 10 | Blocked move preserves Facing | ADR-0008 §2 · `FAC-4` | ✅ canone |
| 11 | **Reaction/Overwatch legge il Facing di boundary** | E14 [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152) | 🔴 **GAP — vedi §6.2** |
| 12 | Final-cell reaction prima del Final Pivot | ADR-0008 | 🔴 **GAP — vedi §6.3** |
| 13 | Move interrotto nega il Pivot non raggiunto | ADR-0008 | 🔴 **GAP — vedi §6.3** |
| 14 | TurnLog causal reason per movimento/facing | `ERTMoveOutcome` (8 valori, serializzato) · `Facing.TurnLogNamesConsumerAndReason` | ✅ spedito |
| 15 | Permutation test su insertion order | `Actions.Collisions.NoPlayerIdBias` · `HexSim.ResolveOrderIndependent` · `Facing.PermutationInvariant` | ✅ spedito |
| 16 | Repeat / state-hash / log-hash golden | E12 [#26](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26) `G1`; CP 12.6 `#178` **chiusa** | ✅ + [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) per il Facing nel digest (`D-261`) |
| 17 | PIE collision su ultimo step + Pivot | `test-manuali-pie.md` · `G9` | ⚠️ `PIE-FACING-1` **bloccata dai cilindri** |
| 18 | PIE Overwatch interruption + Facing | idem | ⚠️ stessa riserva |
| 19 | Dash → Move occupancy handoff | `MakeSnapshot` per segmento (`spec-sequenza-turno.md` §1.1) | ✅ — `D-294` §(2) lo dichiara |
| 20 | Packaged end-to-end evidence | `G2` metà packaged · `G12` · `G13` | ⚠️ 🟡 — **11 test `ClientContext` su 1373** |
| 21 | DQA-030 tracking | ⛔ **chiusa da `D-294`** | 🔴 vedi §3 |
| 22 | Documentation/roadmap update | `D-294` e `D-295` hanno già toccato gli owner | ✅ fatto |

**Conteggio**: 13 ✅ · 4 ⚠️ · **5 🔴**, di cui 2 già in `#1922` e 1 basato su una premessa falsa.

---

## 6. I tre gap reali

### 6.1 Swap e ciclo — già istruito, ma invisibile al tracking

[#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) è aperta, ha un corpo istruito
(il ciclo e il convoy sono la stessa forma; serve un rilevamento di ciclo sul grafo `target → occupante`, non
un confronto a coppie), e **non ha né milestone, né label `v0.1`, né epic dichiarato**. Porta solo `bug`.

⚠️ Lo stesso vale per [#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) (nessuna
label, nessuna milestone), [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800)
(`P1`, ma nessuna `v0.1` e nessuna milestone) e — trovata mentre si misurava il §6.3 —
[#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) (`P2`, idem).
**Quattro issue che portano il lavoro di questo dominio non erano raggiungibili da nessuna vista di
release.** È il difetto di tracking che il kit voleva trovare, e non è dove il kit lo cercava.

✅ **Corretto**: le quattro sono state agganciate a release ed Epic — §8.

### 6.2 🔴 `FacingUsedByOverwatch` — l'unico momento di `D-020` senza codice

[`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) riga 238 lo dichiara dal **2026-08-11**:

> *«Resta il **cono Overwatch**, che non dipende da E16 ma da **E14**: `FacingUsedByOverwatch` è l'unico
> momento di D-020 senza codice.»*

È esattamente il WP 11 del kit e il golden scenario *«Overwatch → Facing = ultima transizione riuscita»*.
I test Overwatch esistenti coprono l'**interruzione** — `Overwatch.FireTruncatesFutureMovement`,
`InterruptionAffectsLaterCollision`, `TriggersPerMicroStep`, `SteppedCallMatchesWholePath`,
`OrderIsDeterministic` — ma **nessuno asserisce quale facing il trigger legge**. Owner: E14
[#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152).

🔎 **Misura aggiunta il 2026-08-31 su `188183b9`, e precisa la diagnosi del DoD.** Il difetto non è che
manchi un test: è che manca il **produttore**. `URTFacingLibrary::ReadFacingForConsumer` esiste, il
vocabolario esiste (`ERTFacingOutcome::UsedByBlast`, `UsedByOverwatch`), il mapper del log li descrive — e
**nessun codice di gioco la chiama**. Lo dichiara il repository stesso in `Turn/RTTurnLog.h:659`:
*«`ReadFacingForConsumer` […] non ha nessun chiamante in gioco — solo due test. Quelle due voci non entrano
in nessuna traccia reale.»*

🔴 **E un verde sembra coprirlo.** `Facing.TurnLogNamesConsumerAndReason` costruisce una voce
`UsedByOverwatch` **a mano** (`RTFacingTests.cpp:411`) per verificare che l'hash distingua i consumatori:
misura il TurnLog, non l'Overwatch. Aperta come
[#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933) sotto E14.

### 6.3 🔴 La tabella §Verifica di ADR-0008 elenca undici test, e **nessuno esiste**

Misurato per nome su `Source/`:

| Test dichiarato da ADR-0008 §Verifica | Occorrenze |
|---|---|
| `Facing.PivotBudgetLimitsLegalFacings` | **0** |
| `Facing.MicroStepFacingIsLastCompletedStep` | **0** |
| `Facing.MicroStepZeroKeepsEntryFacing` | **0** |
| `Facing.MicroStepFacingMatchesFinalAtLastStep` | **0** |
| `Facing.FinalPivotIsNotRetroactive` | **0** |
| `Overwatch.TriggerReadsMicroStepFacing` | **0** |
| `Facing.DefaultActionPolicyMatchesD020` | **0** |
| *(le altre quattro voci della tabella)* | **0** |

I tredici test `Facing.*` che **esistono** portano nomi diversi e coprono altri assi (`LinearMoveDerivesDirection`,
`RoundInheritsFinalFacing`, `PermutationInvariant`, `IntentIsTeamFiltered`, …). La tabella §Verifica è una
lista di test **attesi**, non una lista di test presenti — e non lo dichiara.

🔴 **Conseguenza su una decisione già registrata.** `D-295` §(2) chiude `AUTHOR-PIVOT-001` con
*«ADR-0008 lo dichiara e `Facing.FinalPivotIsNotRetroactive` lo pinna per nome»*. La prima metà è vera, **la
seconda no**: quel test non esiste. La decisione resta canonica — l'ADR la dichiara — ma **non è pinnata**, e
`D-295` va corretto sul punto. Nessun test in `Source/RefactorTactics/Tests/` nomina il pivot: le uniche
occorrenze di `pivot` sono in `RTCameraPawnTests.cpp`, ed è il pivot della camera.

∴ I WP 12 e 13 del kit — *«final-cell reaction prima del Pivot»*, *«Move interrotto nega il Pivot»* — sono
**gap di copertura reali**, e sono l'unico punto in cui il kit trova qualcosa che il repository non sapeva.

---

## 7. Golden scenari — cosa la suite copre davvero

| Scenario chiesto dal kit §12 | Copertura misurata |
|---|---|
| `A → X ← B` = BLOCK/BLOCK | ✅ `HexSim.ResolveContestedDestination` |
| contesa a tre | ⚠️ non misurata per nome |
| convoy libero | ⛔ **assente** — `#1922` |
| convoy bloccato / propagazione all'indietro | ✅ `HexSim.ResolveBlockedByStationary` |
| swap diretto | 🔴 **verde con l'asserzione opposta** — `HexSim.ResolveSwapAllowed` |
| ciclo chiuso a tre | ⛔ **assente** |
| catene indipendenti nello stesso microstep | ⚠️ non misurata per nome |
| stesso X/Y, `Layer` diverso | ✅ per `operator==` — ⚠️ nessun test di collisione lo esercita |
| facing da transizione riuscita / bloccata | ✅ parziale (`Facing.*` esistenti) |
| Pivot negato su endpoint non raggiunto | ⛔ **assente** (§6.3) |
| reaction di cella prima del Pivot | ⛔ **assente** (§6.3) |
| Overwatch FIRE interrompe il residuo | ✅ `Overwatch.FireTruncatesFutureMovement` |
| l'unità interrotta è occupante al microstep seguente | ✅ `Overwatch.InterruptionAffectsLaterCollision` |
| Facing letto da Overwatch | ⛔ **assente** (§6.2) |
| permutazione unità / proposal | ✅ `Actions.Collisions.NoPlayerIdBias` · `HexSim.ResolveOrderIndependent` |
| ripetizione: stesso state hash / log hash | ✅ `G1` · CP 12.6 — ⚠️ il Facing entra nel digest solo con [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) |

---

## 8. Cosa è stato fatto su GitHub, e cosa no

**Fatto** — quattro issue esistevano già e portavano il lavoro di questo dominio **senza essere raggiungibili
da nessuna vista di release**: nessuna label `v0.1`, nessuna milestone, nessun Epic dichiarato. È il difetto
di tracking che il kit cercava, e non era dove il kit lo cercava.

| Issue | Prima | Ora |
|---|---|---|
| [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922) swap/ciclo nel resolver | solo `bug` | `v0.1` · `v0.1 · Mondo giocabile` · Epic #16 |
| [#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) due unità vive sulla stessa cella | solo `bug` | `v0.1` · `v0.1 · Mondo giocabile` · Epic #16 |
| [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) Facing nel digest (`D-261`) | `P1`, nessuna release | `v0.1` · `v0.1 · Gate di release` · Epic #26 · gate `G1` |
| [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) budget di pivot a runtime | `P2`, nessuna release | `v0.1` · `v0.1 · Percezione e reazioni` · Epic #25 |
| [#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933) **creata** — l'Overwatch non registra il facing letto | — | `v0.1` · `v0.1 · Percezione e reazioni` · Epic #152 |

Ogni issue porta un commento con la misura, e il legame è chiuso **dall'altro lato** con un commento su ogni
Epic — #16, #25, #26, #152 — che è la convenzione già usata da E12 il 2026-08-30 per #1663/#1665.

⚠️ **Due scelte dichiarate invece che nascoste.** L'Epic naturale di #1922/#1733 è **#18** (E4, CP 4.8), che
è **CLOSED**: è stata usata l'epic aperta che possiede `URTHexSimLibrary`, riassegnabile senza costo. E a
**#1922 non è stata assegnata una priorità**: il suo corpo misura che in v0.1 l'esito osservabile cambia
raramente, e fra `P1` e `P2` decide chi pianifica.

**Non fatto**, e perché:

| Non fatto | Perché |
|---|---|
| Creare le altre issue del kit | Il kit ne chiede fino a 22. Diciannove hanno già un owner: crearle sarebbe la duplicazione che il kit §16 vieta. L'unica creata è quella per un gap senza owner (§6.2) |
| Creare Epic | Esistono, coprono v0.1→v1.0, e la mappa del kit ne contraddice tre (§4) |
| Riaprire `DQA-030` | Chiusa da `D-294` **oggi**, con la misura (§3) |
| Toccare `ADR-0008` | Il difetto è che la §Verifica non dichiara di elencare test **attesi**: è una riga da chiarire, non una decisione da emendare — e va fatta insieme ai test che la sanano |
| Eseguire la suite | Nessuna riga di `Source/` toccata da questo pass. Tutti i verdi citati sono lo **stato dichiarato** del repository, non una misura presa qui |

---

## 9. Verifiche eseguite

| Verifica | Esito |
|---|---|
| Baseline git, `origin/main`, working tree | ✅ misurata |
| UE bloccata dal repository | ✅ `.uproject` → `EngineAssociation: 5.8`; `AGENTS.md` §1 → **5.8.1** — coerenti |
| Owner docs (Decision Log, ADR-0005/0008/0009, `spec-tassonomia-movimento`, `spec-sequenza-turno`, DoD, roadmap) | ✅ letti |
| Issue/Epic/milestone/label GitHub | ✅ interrogati lato server con `gh` |
| Esistenza dei test citati | ✅ `grep` per nome su `Source/` |
| **Build** | ⛔ **NOT RUN** |
| **Automation suite** | ⛔ **NOT RUN** |
| **Scenario harness** | ⛔ **NOT RUN** |
| **PIE** | ⛔ **NOT RUN** |
| **Packaged** | ⛔ **NOT RUN** |

---

## 10. La prossima azione

> ⌫ **Questa sezione è stata riscritta il 2026-08-31 dopo la misura del §6.2.** La prima stesura
> raccomandava *«scrivere i tre test che mancano al Facing di boundary»*. **Due dei tre non sono
> scrivibili**, e il terzo non è un test: è un produttore che manca.

**Implementare la lettura tracciata del facing nell'Overwatch —
[#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933), sotto E14
[#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152).**

| Test proposto nella prima stesura | Perché non si scrive oggi |
|---|---|
| `Facing.FinalPivotIsNotRetroactive` | ⛔ il **Final Pivot non esiste a runtime**: `MoveEndPivotMaxSteps` e `DashEndPivotMaxSteps` hanno **0** occorrenze in `Source/` — [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) |
| `Facing.PivotDeniedWhenEndpointNotReached` | ⛔ stessa ragione |
| `Overwatch.TriggerReadsMicroStepFacing` | ⚠️ scrivibile **solo dopo** che l'Overwatch chiami `ReadFacingForConsumer`: oggi non lo fa nessuno (§6.2) |

Resta la raccomandazione con il miglior rapporto rischio/valore, per tre ragioni misurate. ① Chiude la riga
*Facing* del DoD §4, 🟡 dal **2026-08-11** con la causa già nominata — quindi sblocca un gate, non una
casella. ② È **piccola e delimitata**: la funzione, il vocabolario e il mapper del log esistono già; manca la
chiamata. ③ Non tocca il resolver, quindi non interferisce con
[#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922), l'unico lavoro di questo dominio
che cambia un esito serializzato nei replay.

🔴 **Ha un costo da dichiarare prima, non da scoprire dopo**: una voce in più per boundary sposta
`HashTurnLog`, quindi il **corpus golden va rigenerato** — la stessa conseguenza già pagata da `D-245` e che
`D-261`/[#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) richiederà una seconda
volta. Le due vanno ordinate.

⚠️ **In parallelo, e indipendente**: `ADR-0008` §Verifica deve **dichiarare** che elenca test attesi. Oggi si
legge come copertura esistente e ha già indotto in errore `D-295`.
