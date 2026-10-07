# Il momento — `AbilityActivated`, un beat di attivazione per ogni intento di abilità

> **Statuto**: design **accettato in sessione** il 2026-10-07 e **rivisto dopo un panel indipendente** lo
> stesso giorno (verdetto: approvata con modifiche; le modifiche sono incorporate e marcate `➕ rev.`).
> **Non implementato.** Issue [#3549](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3549),
> piano [`2026-10-07-momento-ability-activated.md`](../plans/2026-10-07-momento-ability-activated.md) (rivisto
> due volte, con i `➕ piano.` di questa spec che ne derivano). È il **secondo di quattro sotto-progetti**
> della richiesta d'autore *«associare animazioni e FX alle skill e vederle in azione»*; il primo, il banco
> Ability Lab → PIE, è in `main`
> ([#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532), spec
> [`2026-10-07-banco-abilita-in-pie-design.md`](2026-10-07-banco-abilita-in-pie-design.md) §0.3).
>
> **Stato misurato**: 2026-10-07, `main` = `5b553e640`. Ogni `file:riga` è stato letto su quel commit, e le
> citazioni sono state ricontrollate dal panel; chi le rilegge più tardi le **rimisura**. Nessun totale
> volatile: dove serve una misura c'è il comando.

---

## 0. Le decisioni d'autore (2026-10-07)

| # | Domanda | Decisione |
|---|---|---|
| D1 | Chi emette | **Ogni intento con un `ActionId`, in Prep, Dash e Blast**, tranne `Action.Wait` e la locomozione normale (`Action.Move`). Anche gli intenti che già producono `Attack` o `AttackFootprint`: un beat di cast uniforme su tutto il kit. |
| D2 | Cosa suona in v0.1 | **La clip del ruolo `Cast` sulla sorgente**, via `ARTUnit::PlayPresentationRole(ERTPresentationRole::Cast)`: il ruolo esiste nell'enum e nessuno lo consuma. L'evento porta l'`ActionId`, pronto per la clip per abilità del sotto-progetto 3. Il segnale visivo di attivazione (ring, pulse) resta di [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454). |
| D3 | Ordine | **Prima del colpo dello stesso intento, con una pausa propria**: cast e impatto sono due momenti. |
| D4 | Dash | **Sì**: `AbilityActivated` precede lo spostamento di fase Dash. |
| D5 | Ritmo del Blast | **Sequenza per intento**: il Blast si svela come una sola sequenza ordinata — attivazione, impronta, colpi di quell'intento, poi l'intento successivo — con un solo contatore al posto dei tre canali paralleli di oggi. |
| D6 | Privacy | **Filtrata come il `Move`**: il beat c'è solo se la sorgente è nella conoscenza del team che guarda. |

Le decisioni prese **in sessione** per realizzarle, dopo il panel, sono marcate `Ruling` nel testo: sono
scelte di chi scrive la spec, revocabili dall'autore, e ognuna dice cosa costa se è sbagliata.

---

## 1. Il problema, misurato

**Un'abilità senza colpo non ha un istante.** La timeline di playback (`FRTResolvedEvent`,
`Turn/RTResolvedEvent.h:15-159`) nasce solo da colpi, impronte, stati, reazioni, strutture, archi e
movimenti. Nella fase Blast gli eventi che entrano in timeline sono `Attack` (per vittima), `AttackFootprint`
(`Turn/RTTurnManager.cpp:5526`), `ReactionResolved` (`:5228`, `Turn/RTTurnManager_Blast.cpp:1547`),
`StructureHit` e `Defeated` (`:6437`); Heal, Cleanse, ModifyArc e Interrupt **non producono alcun evento**.
In Prep le istanze (scudi, cure, stati, Overwatch e predittiva armati) producono solo testo di log e, per gli
stati, una voce `Status` (`RTTurnManager.cpp:4390-4406`): **nessuna voce per l'intento**. È la forma di #2505
e #2828: il produttore c'è, manca il momento.

**Il ruolo `Cast` non suona.** `ERTPresentationRole` lo elenca (`Unit/RTPresentationRole.h:26-37`) e il
commento di testa (`:9-13`) dichiara che *«`Cast` … non hanno ancora nessun consumatore»*.
`ARTUnit::PlayPresentationRole` notifica il Blueprint solo per `Attack`, `Hit`, `Death`
(`Unit/RTUnit.cpp:766-775`, con il commento a `:771-773` che lo dichiara); i default C++ del roster popolano
Idle, Move, Attack, Hit, Death (`Unit/RTUnitAnimInstance.cpp:52-62`) e la clip dei pack che si chiama `Cast`
è legata al ruolo **`Attack`** (`:157-164`, trappola dichiarata a `:43-47`).

**Il playback non ha un posto per un beat di Prep, e nel Blast non garantisce l'ordine per intento.** La Prep
è solo `Slack` comprimibile fino a zero (`Turn/RTPlaybackLibrary.cpp:125-129`, `RTTurnManager.cpp:8886-8899`)
e `TickPlayback` non ha un ramo Prep. Il Blast svela impronte, muri e colpi su **tre canali paralleli** con
contatori indipendenti e la stessa `AttacksToShow` (`RTTurnManager.cpp:8422-8506`): «l'attivazione precede
il colpo dello stesso intento» non discende dall'ordine delle code. ➕ rev. Le fasi **nascono** solo da ciò
che oggi si vede: il Dash da un `MoveAnim` (`:7781-7790`), il Blast da `BlastPhaseIsActive(colpi, spinta,
impronte, muri)` (`RTPlaybackLibrary.cpp:47-54`, chiamata a `:7795`). Un Blast di sole cure non apre la
fase.

**Dove gli intenti sono già enumerati in ordine deterministico.** Prep: predittiva e Overwatch escono dal
ciclo di raccolta in ordine di unità (`RTTurnManager.cpp:4263-4350`); le istanze sono ordinate da
`SortActionInstances` dopo `SortUnitsForResolution` (`:4219, 4379`) e consumate una per una a `:4419-4425`.
Dash: il ciclo che emette il `Move` di fase Dash (`:4829-4876`) e quello che applica e spende (`:4891-4893`),
`ActionId` da `DashDef->Def.ActionId` (`:4862-4866`). Blast: `ResolveCombatPasses` (`:5429`) è dichiarata
*«una SEQUENZA»* nell'ordine del catalogo: `ResolveCleanseActions` (`:5436`), `CollectHealActions`,
`CollectAttackIntents`, `AppendChargeImpactIntents`, poi `ApplyInterrupts`/`ResolveInterceptions`,
`RunBlastReactions`, `ApplyEnvironmentChanges` (`:5487`) e le impronte (`:5503`). ➕ rev. **Non tutto passa da
`Ctx.Intents`**: le cure stanno in `HealDefs` (`Turn/RTBlastContext.h:211-245`), le Cleanse si risolvono in
`ResolveCleanseActions` (`RTTurnManager_Blast.cpp:412`), e `ModifyArc` è intercettata **prima** della raccolta
e finisce in `PendingArcOps` con un `continue` (`:659-708`), applicata più tardi in ordine canonico
(`:2005-2017`). Gli impatti di carica entrano in `Ctx.Intents` da `AppendChargeImpactIntents` con
`IntentAbilityIndex = INDEX_NONE` e `IntentDefs = Impact.Def`, che è il `Def` dello scatto
(`:1010-1048`; `Impact.Def = Dash->Def` a `RTTurnManager.cpp:4683`, e `Crossed.Def = Dash->Def` per gli
attraversati a `:4701`). ➕ rev. `ApplyInterrupts` toglie gli
interrotti da `Plan.Hits`, `Plan.Footprints` e dalle porte (`RTTurnManager_Blast.cpp:1399-1440`), **non** da
`Ctx.Intents`; `InterruptedIntents` è un `TSet` locale (`:1228`); e `Ctx.Intents` contiene anche chi è morto in
Prep o Dash, come dichiara la guardia `IsAlive()` di `:1377-1379`.

---

## 2. Il disegno

### 2.1 L'evento

`ERTResolvedEventType::AbilityActivated`, **in coda** all'enum dopo `ArcHit` (`RTResolvedEvent.h:158`;
`uint8` esposto a Blueprint: inserirlo in mezzo rinumererebbe i valori serializzati).

Payload, tutto **copiato** da ciò che il resolver ha già in mano, mai ricalcolato. I campi esistono già
(`Phase` `:188`, `SourceStableUnitId` `:203`, `TargetStableUnitId` `:215`, `ActionId` `:315`,
`BaseActionId` `:332`, `Shape` `:356`, `AimCell` `:373`); nasce **un** campo nuovo.

| Campo | Valore |
|---|---|
| `Phase` | la fase in cui l'intento risolve: `Prep`, `Dash`, `Blast` |
| `SourceStableUnitId` | chi agisce |
| `ActionId`, `BaseActionId` | dal `Def` dell'intento (`IntentDefs[i]`, `Ability->Def`, `DashDef->Def`, `HealDefs[i]`, `Op.Def`) |
| `TargetStableUnitId` | il bersaglio dichiarato se l'intento ne ha uno, altrimenti `0`. In Prep il bersaglio è chi usa l'azione (`RTTurnManager.cpp:4355`): si scrive quello |
| `AimCell`, `Shape` | la cella mirata e la forma dichiarate, se l'intento le ha; altrimenti default. ➕ rev. La docstring di `:334` (*«solo per `AttackFootprint`»*) va aggiornata |
| ➕ rev. `SourceVerdict` (nuovo, `UPROPERTY()` nudo, `FRTKnowledgeVerdict`) | il verdetto di [D-223] **congelato all'emissione** con `FreezeVerdictFor(FRTLogSubject::Unit(Unit))` (`RTTurnManager.h:2439`): quali squadre hanno il diritto di vedere questa unità agire, nell'istante in cui agisce. Stessa forma e stessa disciplina di `CellVerdicts` (`RTResolvedEvent.h:225-245`): vuoto = `NoOne()` = fail-closed |

⛔ Niente celle colpite, niente esiti: quelli restano di `AttackFootprint` e `Attack`. L'attivazione dice
*«questa unità sta agendo adesso con questa azione»*, non cosa succede dopo.

### 2.2 Chi emette, dove (D1, D4)

Una voce **per intento**, nel punto in cui il resolver lo **accetta**, nell'ordine che già ha. L'ordine in
timeline è l'ordine di emissione; nessun riordino a posteriori.

**Prep** — `ResolvePrep`:
- la predittiva (`:4263-4291`) e l'Overwatch (`:4303-4350`) nel ciclo di raccolta, in ordine di unità,
  nel ramo che le arma; ➕ rev. `Ruling`: la predittiva **senza cella** (`:4268`, spesa ma non armata) **si
  attiva**: l'unità ha speso l'azione, e un'attivazione muta sarebbe indistinguibile da un difetto. Costo
  se sbagliato: un beat su un'azione a vuoto, da togliere con un `if`;
- le istanze dopo `SortActionInstances`, nel consumo per istanza (`:4419-4425`);
- ➕ rev. `ResolveCoverStructures` (`:3860`, chiamata a `:4225`, **prima** del ciclo di raccolta): consuma un
  piano con `ActionId` (`Ability->Def.StructureOp`, `Def.ActionId`, azzera `PlannedAbilityIndex`), rifiuta con
  `Reject` per «nessun bersaglio», «nessun bordo dichiarato», «fuori portata», e **non** passa da
  `RefuseMainActionIfStunned`. `Ruling`: **emette solo per le operazioni accettate**, nel punto in cui le
  applica (lo stesso criterio di `ModifyArc`); un rifiuto `Reject` non si attiva; la struttura di un'unità
  stordita **si attiva**, perché la funzione la applica. Costo se sbagliato: un beat su una struttura che il
  gioco ha comunque piazzato;
- ➕ rev. **tre ordini nella stessa fase**, dichiarati: prima le coperture, applicate in ordine di cella e
  bordo; poi predittiva/Overwatch in ordine di unità; poi le istanze in ordine di `SortActionInstances`. È
  l'ordine del resolver, e il playback lo rispetta;
- escluso lo stordimento (`:4251-4257`): l'intento è rifiutato, non attivato.

**Dash** — `ResolveDash`, ➕ rev. nel **primo** ciclo (`:4829-4876`), riscritto come: per ogni `i` con
`DashAbilityIdx[i] != INDEX_NONE` emetti `AbilityActivated` (Phase `Dash`, `ActionId` da `DashDef`), **poi**,
se `Resolved[i].Entered.Num() > 0`, il `Move` come oggi. Così in timeline l'attivazione **precede** il `Move`
dello stesso scatto (D4), e una carica che colpisce senza muoversi si attiva lo stesso. Il secondo ciclo
(`:4891`) non cambia.

**Blast** — ➕ rev. **quattro sorgenti**, nell'ordine dei pass di `ResolveCombatPasses`, ciascuna nel pass che
la accetta:
1. **Cleanse** — in `ResolveCleanseActions` (`RTTurnManager_Blast.cpp:412`), ➕ piano. accanto a
   `Ctx.MarkAbilitySpent` (`:463`), **senza** condizione sullo stato tolto: si attiva quando è **spesa**, il
   gesto e non l'esito (stesso `Ruling` della predittiva senza cella). Una Cleanse che non toglie nessuno
   stato (voce `NoEffect`) **si attiva**;
2. **Heal** — in `CollectHealActions`, ➕ piano. nel punto in cui la cura viene **spesa**, anche quando non
   ha effetto (`:587-593`); `ActionId` da `HealDefs[i]`, sorgente `HealActors[i]`;
3. **ModifyArc** — nel ramo di intercettazione, dove si fa `PendingArcOps.Add` (`:705`): dopo la validazione
   di portata, quindi un arco fuori portata (`Fallback Cancelled`, `:678-700`) **non** si attiva;
4. **intenti d'attacco** — in `ResolveCombatPasses`, subito dopo `ApplyEnvironmentChanges` e prima del ciclo
   delle impronte (`:5487-5503`), iterando `Ctx.Intents` in ordine di `IntentIndex` con:
   - `IntentAbilityIndex != INDEX_NONE` (esclude gli impatti di carica, già attivati in Dash);
   - ➕ rev. `!Ctx.InterruptedIntents.Contains(k)`: `InterruptedIntents` **sale in `FRTBlastContext`**
     accanto a `DegradedIntents` (`RTBlastContext.h:274`), riempito da `ApplyInterrupts` invece del `TSet`
     locale di `:1228`. Un intento interrotto **non** si attiva;
   - ➕ rev. `Units[AttackerId]->IsAlive()`: chi è morto in Prep o Dash non si attiva (stessa guardia di
     `:1377-1379`);
   - un intento **degradato** (D-300, `DegradedIntents`) **si attiva**: l'unità ha agito, con meno;
   - un intento bloccato dalla linea di tiro o senza mappa (`NoLineOfSight`, `NoMap`, `UnverifiableIntents`)
     **si attiva**: entra in `Ctx.Intents` e l'unità ha agito. `Ruling`: l'attivazione racconta il gesto,
     non l'esito; costo se sbagliato: un beat su un'azione che non ha toccato nulla.
   - ➕ piano. **Non** si attiva un intento che `CollectAttackIntents` manda ad `ApplyFallback` con esito
     `Cancel` — fuori portata, bersaglio ignoto, bersaglio sparito (`RTTurnManager_Blast.cpp:855-892`): fa
     `continue` **prima** di `Intents.Add`, quindi non è mai un intento del Blast. È la stessa regola del
     `ModifyArc` fuori portata: un `Fallback Cancelled` è un rifiuto visibile nel TurnLog, non un gesto.
     Dichiarato in §6.

`Ruling` sull'ordine fra le sorgenti: **l'ordine dei pass**, non l'ordine delle unità — è l'ordine che
`ResolveCombatPasses` dichiara di rispettare (*«spostare una chiamata cambia il gioco»*). Costo se
sbagliato: nel playback una cura si vede prima di un attacco anche quando l'attaccante viene prima nel
roster; è coerente con la risoluzione, e il riordino per unità è un filtro di presentazione aggiungibile
dopo.

➕ rev. **Dichiarato**: i `StructureHit` di `ApplyEnvironmentChanges` (`:5487`) entrano in timeline **prima**
delle attivazioni degli intenti d'attacco. Il playback li raggruppa per intento (§2.4), quindi a schermo
l'ordine è comunque attivazione → impronta → colpi/muri.

**Esclusi**: ⌫ ➕ impl. *Questa riga diceva «esclusi per costruzione: `Action.Wait` … non passa da questi
siti», ed era falso* — `IMPLEMENTATION DRIFT` trovato dal Task 3 al primo run verde: `CollectAttackIntents`
filtra solo `IsFastMovement`, e `Wait` (fallback `Stop`, che produce effetti) arriva fino a `Intents.Add`.
L'esclusione è quindi **per filtro di fase**, in `EmitAttackIntentActivations`: salta gli intenti il cui
`MapResolutionPhase(Def.ResolutionPhase)` è `Move` — `Action.Wait` (risolve in `NormalMovement`,
`Ability/RTCatalogLibrary.cpp:1184`), `Sprint`, `Withdraw` e il `Move` normale. È il criterio che D1
nomina. Mutazione dichiarata: filtro disattivato → cade «`Action.Wait` non si attiva». Restano fuori per
costruzione le abilità di fase Cleanup/Environment (`RTTurnManager_Blast.cpp:628-637`), che non sono
intenti di Prep, Dash o Blast, e le **reazioni**, il cui momento è `ReactionResolved` (#2191).

Ordinamento: solo da `SortUnitsForResolution`, `SortActionInstances`, `IntentIndex` e l'ordine dei pass, mai
da `TMap`/`TSet`/Actor. La timeline resta fuori da TurnLog, snapshot e `StateHash`
(`RTTurnManager.cpp:2831-2833`, `Turn/RTTurnLog.h:827-829`): un evento in più non muove nessun hash, e
`Match.Autobattle.DeterminismIsIndependentOfPlayback` resta il gate. ⚠️ `Turn/RTMatchStateHash.h` senza
occorrenze della timeline: `NOT RUN` dal panel, da misurare nel piano con `grep -n ResolvedTimeline`.

### 2.3 Cosa suona (D2)

- Voce D-278 in `URTPresentationBindingLibrary::DeclaredBindings()` (`Turn/RTPresentationBinding.cpp`):
  `AbilityActivated` → `Cues { PlayCastMontage }`. ➕ rev. Il gate `FindMissingBindings` (`:356-363`)
  verifica solo che la cue dichiarata non sia `NAME_None`; il commento a `:113-116` è una convenzione, non una
  misura. Che la cue venga **chiamata** lo prova un test di playback (§5.1), non il gate.
- Nasce `ARTUnit::PlayCastMontage(UAnimSequenceBase* Resolved)` (`BlueprintImplementableEvent`, come i tre
  esistenti, `Unit/RTUnit.h:1316-1325`) e il `case ERTPresentationRole::Cast` nello `switch` di
  `PlayPresentationRole` (`RTUnit.cpp:766-775`); i commenti di `RTUnit.cpp:771-773` e
  `RTPresentationRole.h:9-13` vanno riscritti: il ruolo `Cast` ora **suona**.
- ➕ rev. Un seam di misura, solo sotto `WITH_DEV_AUTOMATION_TESTS`: `ARTUnit::CastCuesPlayedForTest()`, un
  contatore incrementato nel `case Cast`. È ciò che permette al test di §5.1 di asserire il **meccanismo**
  senza un Blueprint.
- Il ruolo `Cast` si popola nei default C++ del roster (`RTUnitAnimInstance.cpp:52-62, 157-164`) con la clip
  `Cast` dei quattro pack, **la stessa che oggi sta sul ruolo `Attack`**. In v0.1 cast e colpo suonano la
  stessa sequenza in **due momenti diversi**; la differenziazione per abilità è il sotto-progetto 3, e un
  eventuale rimpiazzo della clip del ruolo `Attack` è un giudizio umano da catalogo (ANIM CORE). Il
  commento-trappola di `:43-47` va riscritto.
- `Presentation.EnumSizeIsPinned` passa da 9 (`Tests/RTPresentationBindingTests.cpp:69-70`) a 10 con una
  riga di storia; `AbsenceCensusIsPinned` (4 in attesa, 0 decise, `:478`, `:492`) non cambia conteggio e
  asserisce owner vuoto per `AbilityActivated`, come fa per `AttackFootprint` e `StructureHit` (`:503-522`).
- Il feed di sviluppo riceve una riga `Attiva: <unità> -> <ActionId>` con `AddLogEvent(…,
  FRTLogSubject::Unit(Unit))`, come `Colpo:` (`RTTurnManager.cpp:8456`): il verdetto della riga è lo stesso
  che si congela nell'evento (`:266`), calcolato una volta.

### 2.4 Il playback (D3, D5)

**Una funzione pura costruisce la sequenza.** ➕ rev. In `URTPlaybackLibrary`:

```cpp
/** Un elemento della sequenza di Blast: l'indice dell'evento in timeline e la chiave del suo atto. */
struct FRTBlastSequenceElement { int32 TimelineIndex; int32 SourceStableUnitId; FName ActionId; };

/**
 * La sequenza per intento del Blast (D5). Gli eventi di fase Blast di tipo AbilityActivated, AttackFootprint,
 * StructureHit e Attack si raggruppano per (SourceStableUnitId, ActionId); i gruppi si ordinano per l'indice
 * in timeline della loro ATTIVAZIONE visibile se ne hanno una, altrimenti per prima apparizione (➕ piano:
 * così uno StructureHit emesso prima delle attivazioni non porta il suo intento davanti a uno con
 * IntentIndex minore); dentro il gruppo: attivazione, impronte, muri, colpi, nell'ordine di timeline.
 * Un gruppo senza attivazione (impatti di carica, StructureHit senza identità, Attack di reazione) è un
 * atto proprio, alla posizione della sua prima apparizione.
 * FrozenPrefix: i primi N elementi della sequenza precedente si riproducono VERBATIM (D-355, §2.4);
 * gli eventi non ancora sequenziati si raggruppano DOPO il prefisso. ➕ impl. La chiave di un gruppo si
 * legge dall'attivazione OVUNQUE stia in timeline, anche dentro il prefisso: così Build(T, S, k) == S per
 * ogni k (idempotenza, anche a metà atto) e un evento nuovo con chiave già aperta si unisce al suo gruppo
 * nella parte non congelata. Con N = 0 è la costruzione da zero.
 */
static TArray<FRTBlastSequenceElement> BuildBlastSequence(const TArray<FRTResolvedEvent>& Timeline,
    const TArray<FRTBlastSequenceElement>& Previous, int32 FrozenPrefix, int32 ViewerTeamId);
```

`ViewerTeamId` serve al filtro di §2.5: le attivazioni non visibili non entrano; i loro impronte e colpi
restano, come oggi (il velo sul bersaglio è di [D-223], non di questa spec). ➕ rev. `ArcHit` **non** entra
nella sequenza: oggi `BeginPlayback` non ha un ramo per lui (`:7688-7778`), la sua voce D-278 è in attesa con
owner #3293 (`RTPresentationBinding.cpp:249`) e porta sempre `NAME_None` (`RTPlaybackLibrary.cpp:337-339`).

**Smistamento.** `BeginPlayback` (`RTTurnManager.cpp:7688-7778`) raccoglie le attivazioni visibili per fase
(`PlaybackActivationsPrep`, `…Dash`) e costruisce `PlaybackBlastSequence` con `BuildBlastSequence`. Ne
discende:

- **Cancelli di fase** (➕ rev., C3): la Prep è attiva se `bPrepActiveThisTurn` **o** ha attivazioni
  visibili; il Dash se ha `MoveAnim` di fase Dash **o** attivazioni visibili; il Blast con
  `BlastPhaseIsActive(NumAttacks, bHasBlastMove, NumFootprints, NumStructureHits, NumActivations)`, quinto
  argomento nuovo, «basta che UNA sia vera».
- **Prep** — la fase guadagna un canale `Shown`: `TickPlayback` riceve un ramo Prep che svela le
  attivazioni una per volta con `AttacksToShow`, nell'ordine di timeline.
- **Dash** — prima le attivazioni (`AttacksToShow`), poi le `MoveAnim` come oggi: `RouteAlpha` dei Dash parte
  dopo il tempo delle attivazioni.
- **Blast** — **una sequenza per intento** sostituisce i tre canali (`:8422-8506`): un solo contatore
  `BlastShown`, una sola cadenza `AttackShowSeconds`. Le funzioni di rivelazione esistenti
  (`RevealPlaybackFootprints`, i muri, il ciclo dei colpi) diventano i **rami per tipo** di un unico `switch`
  sull'elemento corrente: la cue per elemento non cambia, cambia chi decide quando. Per l'attivazione il
  ramo chiama `Src->PlayPresentationRole(ERTPresentationRole::Cast)` e scrive la riga `Attiva:`.
  ➕ merge. Con il tracer di [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454)
  (spec `2026-10-07-tracer-attacco-base-design.md` §2.3) il contatore è un **cursore di battiti** sulla
  sequenza, `BlastBeatsDone`: il battito `2k` rivela l'elemento `k` — per un `Attack` è il **lancio** — e il
  `2k+1` è il suo **arrivo** (`Hit`, numero, `Colpo:`, `OnAttackResolved`, confine `Next Action`), con i voli
  paralleli alla sequenza e nulli per ogni altro tipo. La cadenza resta `k·A`; `BlastShown` è
  `BlastElementsShown()`, derivato dal cursore, e resta il prefisso congelato di D-355.
  ➕ merge `CONTRACT CONFLICT` con la spec del tracer §2.1 («il ritmo non dipende da chi guarda»): con D6 la
  sequenza del Blast si costruisce per squadra, quindi l'indice — e con il tracer l'istante di lancio e di
  arrivo — di uno stesso colpo differisce fra chi vede la sorgente e chi no. **Governa questa spec**:
  un'attivazione nascosta non deve lasciare un buco nel ritmo che ne riveli l'esistenza. La durata delle fasi
  dipendeva già dal viewer (rotte troncate al tratto osservato). Pinnato da
  `Playback.BlastSequenceIndexDependsOnTheViewer`.
- **`PhaseTime`** (➕ rev., I5): la firma guadagna `int32 NumActivations` e, per il Blast,
  `int32 NumSequenceElements` prende il posto dei tre conteggi di canale:
  - Prep: `Shown = NumActivations × AttackShowSeconds`, `Slack = PhaseBeatSeconds` (il beat di oggi resta);
  - Dash: `Shown = NumActivations × AttackShowSeconds + MoveTime`;
  - Blast: `Shown = Max(Max(1, NumSequenceElements) × AttackShowSeconds, MoveTime)` — il `Max` con la spinta
    resta (`RTPlaybackLibrary.cpp:111-121`), la sequenza sostituisce il `Max` fra canali;
  - `PhaseDuration` (`:56-69`) sopravvive per i gate di pacing con **tre** zeri dichiarati invece di due.
  Con `AttackShowSeconds <= 0`, `AttacksToShow` mostra tutto in un frame (`:37-40`): la sequenza lo
  accetta, come oggi i canali.
- **Confini d'atto** (#3292): `IsActBoundary` resta l'unica implementazione
  (`ActBoundaryRuleHasOneImplementation`). ➕ rev. `Ruling`: il confronto passa da `ActionId` solo (`:348`) a
  **`(SourceStableUnitId, ActionId)`**, e `PlaybackLastShownAction` porta entrambi: due unità consecutive con
  la stessa azione generica (`Action.Heal`, `Action.Overwatch`) sono **due** atti, come il criterio PIE (2)
  pretende. I rami Prep e Dash aggiornano `PlaybackLastShownAction` e chiamano `IsActBoundary` come oggi fa
  solo il Blast (`:8498-8504`), altrimenti `Next Action` salterebbe le attivazioni. Costo se sbagliato: una
  fermata in più fra due colpi della stessa area da unità diverse — che oggi non esistono (un intento ha una
  sorgente).
- **D-355** (➕ rev., I6): il Blast può sospendersi (`:5416-5423`) e `BeginPlayback(bPreserveClock)`
  ricostruisce le code mantenendo i contatori (`:7635-7639`, `:7857-7862`). Invariante dichiarata: **il
  prefisso già mostrato della sequenza è stabile all'estensione** — la ricostruzione chiama
  `BuildBlastSequence(Timeline, PlaybackBlastSequence, BlastShown, Viewer)`, i primi `BlastShown` elementi
  sono riprodotti verbatim; ⌫ ➕ impl. *la prima stesura diceva che gli eventi nuovi di un gruppo già aperto
  si accodavano come atto proprio («coda e non inserimento»)*: la review del Task 5 ha mostrato che con un
  prefisso a metà atto il resto dell'atto perdeva la sua attivazione (nel prefisso) e scivolava dietro gli
  atti successivi — `Build(T, S, k) != S` senza che la timeline fosse cresciuta. `Ruling` rivisto: la chiave
  di un gruppo si legge dall'attivazione **ovunque stia**, anche nel prefisso; la parte non congelata si
  ricostruisce da zero con quelle chiavi, quindi `Build(T, S, k) == S` per ogni `k` e un evento nuovo con
  chiave aperta si unisce al suo gruppo nel tail. La regola «il prefisso mostrato non si muove» resta
  intera: l'inserimento avviene solo oltre `BlastShown`. Test: idempotenza per ogni `k`; evento nuovo con
  chiave aperta nel suo gruppo.
  Precondizione dichiarata: la timeline cresce **solo per accodamento** e gli indici già mostrati restano
  stabili; `BlastSequencePrefixIsStableUnderExtension` lo asserisce confrontando i `TimelineIndex` del prefisso.
- **D-287**: nessuna fase nuova (punto 1); i tempi restano `PROPOSED FOR PLAYTEST` (punto 7).

➕ rev. **Riordino dichiarato** (I3): oggi `Plan.Hits` è ordinato per `AttackerId, TargetId, Power`
(`Combat/RTHexCombatLibrary.cpp:653-659`) e gli impatti di carica si intercalano fra gli altri colpi. Con la
sequenza per intento i colpi si vedono **raggruppati per atto**, nell'ordine di prima apparizione: gli
impatti di carica (gruppo senza attivazione di Blast, attivati in Dash) alla posizione del loro primo
`Attack`; l'`Attack` di un contrattacco di reazione come atto proprio, alla posizione in cui la timeline lo
porta, quindi **può** vedersi staccato dal colpo che lo ha innescato. È D5, ed è un limite (§6).

### 2.5 Privacy (D6)

➕ rev. (C4) Il predicato del `Move` — `ObservedPrefixLength(Path, CellVerdicts, ViewerTeamId)`
(`RTTurnManager.cpp:7699-7700`) — legge verdetti **per cella** congelati da `FreezeRouteVerdicts`, e
un'attivazione non ha né rotta né celle: vuoto si leggerebbe fail-closed e **tutte** le attivazioni
sparirebbero, comprese quelle della squadra di chi guarda. Il predicato di questa spec è quindi quello delle
righe di log ([D-223]): **`Ev.SourceVerdict.AllowsTeam(ViewerTeamId)`** (`Perception/RTTeamKnowledge.h:187-194`, fail-closed fuori
intervallo),
sul verdetto congelato all'emissione (§2.1). ➕ rev. `FreezeVerdictFor` (`RTTurnManager.cpp:275-311`)
risponde `NoOne` per un'unità nulla (`:291-295`) e altrimenti delega a `FreezeVerdict`
(`Perception/RTTeamKnowledge.cpp:207-232`), che itera **solo le squadre presenti in `TeamKnowledgeState`**;
la squadra della sorgente entra dal ramo «alleato» di `ClassifyTarget` (`:190-197`). Quindi le attivazioni
della propria squadra e degli alleati sono visibili **a condizione che lo stato di conoscenza sia popolato**:
in una fixture senza refresh il verdetto è `NoOne` e nessuna attivazione si vede. È la stessa precondizione
delle righe di log, e i test di §5.1 la dichiarano con un controllo positivo sulla squadra del viewer.

Un'attivazione non visibile **non entra** nelle code di playback (Prep, Dash, sequenza di Blast): tutto o
niente, non c'è prefisso. L'evento resta nella timeline canonica (è il **playback** a filtrare, come oggi
per il `Move`), così replay e diagnostica lo vedono. Il viewer è il giocatore locale
(`ARTPlayerState::TeamIdOf(GetPlayerController(this, 0))`, `:7685`): limite dichiarato di v0.1, lo stesso
del velo. La tabella dei canali in `docs/technical/systems/conoscenza-parziale-visibile-spec.md` §1.3 riceve
la riga *«attivazione: mostrata solo con sorgente osservata»* (`NOT RUN` dal panel: il piano verifica che la
tabella esista in quella forma).

---

## 3. Fuori scope

- Il segnale visivo di attivazione (ring, pulse, flash): [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454).
- Clip per abilità e chiave `ActionId` nel catalogo: sotto-progetto 3. Profili FX: sotto-progetto 4.
- Una clip `Cast` **diversa** da quella del ruolo `Attack`: giudizio umano nell'Anim Browser.
- Attivazione delle reazioni, degli stati, degli hazard; un beat per `ArcHit` (#3293).
- `rt.Debug.PlaybackStartPaused`, Gray Kit Playground; il riordino per unità fra le sorgenti del Blast.

---

## 4. Errori e degrado

| Caso | Comportamento |
|---|---|
| Nessuna clip attiva per `(eroe, Cast)` | `PlayPresentationRole` degrada come per gli altri ruoli (`RTUnit.cpp:740-745`): nessuna clip, notifica Blueprint con `nullptr`, la partita si gioca uguale. |
| Blueprint che non implementa `PlayCastMontage` | Nessun effetto: invariante #1. |
| Un intento senza `ActionId` arriva a un sito di emissione | Non emette e scrive un `ensureMsgf`: l'`ActionId` è ciò che il sotto-progetto 3 consuma, e un `NAME_None` dimenticato non fa fallire nessun test (`RTResolvedEvent.h:310-312`). ➕ impl. Vale per i siti che leggono dal catalogo (Prep, Dash, Cleanse, Heal, ModifyArc). Gli **intenti d'attacco legacy** senza `ActionId` (abilità di `EnsureDefaultAbilities`/`MakeAbility`, ammesse da `CollectAttackIntents`) sono il caso normale delle unità nude dei test: `EmitAttackIntentActivations` li **salta prima** dell'helper, senza `ensure` — D1 dice «ogni intento **con un `ActionId`**». |
| Evento di Blast senza attivazione corrispondente | Atto proprio alla posizione di prima apparizione: si vede comunque. |
| Sorgente non osservata | Nessun beat, nessuna fermata d'atto: la sequenza salta l'elemento. |
| Estensione D-355 dentro un gruppo già aperto | ➕ impl. L'evento si unisce al suo gruppo nella parte non ancora mostrata; il prefisso mostrato non si muove. Se l'atto era già tutto mostrato, l'evento resta nel suo gruppo e si colloca nella parte non mostrata **secondo la chiave del gruppo** (l'indice della sua attivazione): un atto aperto presto ha una chiave bassa, quindi il suo evento tardivo è fra i primi oltre il prefisso, non in fondo. |
| Replay / seek | Timeline per turno e per playback; nessun dato nuovo in snapshot, TurnLog o hash. |

---

## 5. Verifica

### 5.1 Automation, headless

Fixture: `RTWorldFixtures::MakeWorld`, piano scritto a mano, `LockInAndResolve`,
`ResolvedTimelineForTest()` (`Tests/RTPlaybackActionIdentityTests.cpp:108-136`). ➕ rev. Il turno di
`IsEmittedOncePerIntent` vuole **tre** unità, perché lo scatto spende il movimento (D-028). Il viewer dei test
di playback: `SpawnActor<ARTPlayerController>()` come in `Tests/RTBlindFireTests.cpp:132`; senza un
`ARTPlayerState` assegnato, `ARTPlayerState::TeamIdOf` ripiega su `0` (`Player/RTPlayerState.cpp:5-12`), quindi
per un viewer di squadra diversa da 0 il test assegna un `ARTPlayerState` esplicito. Precondizione dei test
di privacy: `TeamKnowledgeState` popolato (refresh eseguito), verificato da un controllo positivo in cui
l'attivazione della squadra del viewer **è** visibile.

| Test | Asserisce |
|---|---|
| `Turn.AbilityActivatedIsEmittedOncePerIntent` | Un turno con un intento di Prep (scudo), un Dash e un Blast con colpo, su tre unità: tre `AbilityActivated`, uno per fase, con `Source` e `ActionId` dell'intento; `Action.Move` e `Action.Wait` non ne producono (➕ impl. per il filtro di fase Move, non per costruzione: mutazione «filtro disattivato» → `Wait` si attiva). **Controllo positivo**: aggiungere un quarto intento (una cura) produce un quarto evento con il suo `ActionId`. ➕ impl. Un intento legacy senza `ActionId` (unità con `EnsureDefaultAbilities`) non si attiva e non fa scattare l'`ensure`. |
| `Turn.BlastActivatesAllFourSources` (➕ rev.) | Cleanse, Heal, ModifyArc e un intento d'attacco nello stesso Blast: quattro attivazioni, nell'ordine dei pass (Cleanse, Heal, Arc, Attack). Un `ModifyArc` fuori portata: nessuna. |
| `Turn.AbilityActivatedPrecedesItsFootprintAndHits` | Per ogni intento di Blast, l'indice dell'attivazione è minore di quello dell'`AttackFootprint` e degli `Attack` con lo stesso `(Source, ActionId)`. ⛔ Non asserisce nulla sui `StructureHit`: in timeline precedono l'attivazione (§2.2) e solo la sequenza di playback li riordina. |
| `Turn.DashActivationPrecedesItsMove` (➕ rev.) | In timeline l'attivazione di fase Dash ha indice minore del `Move` di fase Dash della stessa unità; una carica che non entra in nessuna cella ha l'attivazione e nessun `Move`. |
| `Turn.ChargeActivatesInDashNotInBlast` | Una carica che colpisce: una sola attivazione, di fase Dash; gli `Attack` dell'impatto in Blast hanno lo stesso `(Source, ActionId)` dello scatto (`Impact.Def = Dash->Def`). |
| `Turn.InterruptedOrDeadIntentDoesNotActivate` (➕ rev.) | Un intento interrotto: nessuna attivazione; uno bloccato dalla LOS: attivazione presente; un attaccante morto in Prep: nessuna. Mutazione: tenere `InterruptedIntents` locale fa cadere il primo ramo. |
| `Presentation.EnumSizeIsPinned` (10) · `EveryEventTypeIsCovered` · `AbsenceCensusIsPinned` | Il gate D-278 resta verde con la voce `Cues { PlayCastMontage }`. Mutazione: togliere la voce fa cadere `EveryEventTypeIsCovered`. |
| `Playback.ActivationPlaysTheCastCue` (➕ rev., I8) | Un turno con un'attivazione visibile, playback mandato avanti oltre il suo istante: `CastCuesPlayedForTest()` della sorgente vale 1; con la sorgente non osservata vale 0. |
| `Unit.CastRoleResolvesAClipForEveryHero` | `ActiveClipFor(HeroId, Cast)` sul CDO di `URTUnitAnimInstance` (`RTUnitAnimInstance.h:197`) non è nullo per i quattro eroi del roster (il path, senza caricare). |
| `Playback.PhaseTimeCountsActivations` | `PhaseTime` con N attivazioni: Prep e Dash crescono di `N × AttackShowSeconds`; Blast usa `NumSequenceElements`; con zero tutto resta com'era (regressione, e `PhaseDuration` invariato). |
| `Playback.BlastSequenceIsOrderedPerIntent` (puro) | Timeline sintetica con due intenti aggressivi: `A1 F1 H1 H1 A2 F2 H2`; un `Attack` senza attivazione è un atto proprio alla sua prima apparizione; `ArcHit` assente. Mutazione: raggruppare per tipo invece che per chiave fa cadere l'asserto. |
| `Playback.BlastSequencePrefixIsStableUnderExtension` (puro, ➕ rev.) | `BuildBlastSequence(T2, S1, N, V)` con `T2 = T1 + eventi` riproduce i primi `N` elementi di `S1` e accoda i nuovi; un evento nuovo con chiave già aperta si unisce al suo gruppo oltre il prefisso (Ruling H). |
| `Playback.BlastPhaseOpensForActivationsOnly` (puro, ➕ rev.) | `BlastPhaseIsActive(0,false,0,0,1)` è vero; `(0,false,0,0,0)` falso. |
| `Playback.NextActionDoesNotStopTwiceWithinOneIntent` (esteso, in `Tests/RTPlaybackStopPredicateTests.cpp:604-680`) | Attivazione + impronta + colpo dello stesso intento = **una** fermata; due cure consecutive da unità diverse = **due**. |
| `Playback.HiddenSourceHasNoActivationBeat` | Sorgente non osservata dal viewer: l'attivazione non entra nelle code; osservata: entra. Mutazione: togliere il filtro fa cadere il primo ramo. |
| `Match.Autobattle.DeterminismIsIndependentOfPlayback` (esistente) | Resta verde. |

🔴 Controlli di mutazione dichiarati: (1) togliere l'emissione degli intenti d'attacco → cade
`IsEmittedOncePerIntent`; (2) `BuildBlastSequence` per tipo → cade `BlastSequenceIsOrderedPerIntent`;
(3) togliere il filtro → cade `HiddenSourceHasNoActivationBeat`; (4) `InterruptedIntents` locale → cade
`InterruptedOrDeadIntentDoesNotActivate`; (5) `IsActBoundary` sul solo `ActionId` → cade il ramo «due cure».

### 5.2 Seduta PIE

Voce nuova **`PIE-CAST-BEAT`** nel registro (⏳) e seduta nel file delle sedute, scenario `Visual` con due
unità: una che usa un'abilità **senza colpo** (`Hero.Muiren.TideGuard`, un `Action.Shield` di Prep,
`RTHeroCatalogLibrary.cpp:657`) e una con colpo (`Hero.Branth.ImpactShot`, `:740`). Criteri binari: (0)
l'abilità senza colpo ha un beat visibile **sulla sorgente** (prima la Prep aveva solo il beat generico);
(1) per quella con colpo si vedono **tre** momenti, cast, lancio e impatto, nell'ordine — ➕ merge: il lancio
è il proiettile del tracer di [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454), che
dopo il merge separa l'arrivo dalla rivelazione; (2) ➕ impl. `NOT RUN` in PIE: la fermata per atto
(`RequestPlaybackStopAt`, `Next Action`) non ha oggi nessun ingresso da tastiera né da console, quindi «una
fermata per intento, non due» è coperta solo headless (`Playback.NextActionDoesNotStopTwiceWithinOneIntent`,
`Playback.ActivationAndWallOfOneIntentAreOneAct`) e si giudicherà a schermo il giorno in cui avrà un ingresso;
(3) con la sorgente nemica fuori dalla conoscenza, nessun beat. Il banco di #3532 serve ad allestire ciascuna
abilità da sola; `rt.Debug.PlaybackControls 1` resta utile per `K`/`L` (pausa e passo singolo per micro-step),
non per il criterio (2).

---

## 6. Limiti dichiarati

- In v0.1 cast e colpo usano la **stessa clip** per eroe: si distinguono per momento, non per forma.
- La sequenza per intento allunga il Blast di un elemento per intento e **rallenta la spinta**, che si anima
  sulla durata della fase; i tempi restano `PROPOSED FOR PLAYTEST` (D-287 punto 7).
- I colpi del Blast si vedono raggruppati per atto, non più nell'ordine `AttackerId, TargetId, Power`; il
  contrattacco di una reazione può vedersi staccato dal colpo che lo ha innescato.
- Un `StructureHit` senza identità d'azione (#3281) resta un atto proprio.
- ⌫ ➕ impl. *«Un evento arrivato in estensione D-355 si accoda anche se il suo atto era già aperto»*: ora si
  unisce al suo gruppo oltre il prefisso mostrato; se l'atto era già tutto mostrato, si vede staccato dalla
  sua attivazione — è il solo caso in cui il cast e il colpo si separano, e nasce dalla sospensione.
- La Prep di un nemico nascosto non produce beat sulla sorgente: esito voluto da D6.
- ➕ merge `CONTRACT CONFLICT` con la spec del tracer §2.1 («il ritmo non dipende da chi guarda»): con D6 la
  sequenza del Blast si costruisce per squadra, quindi l'indice — e con il tracer l'istante di lancio e di
  arrivo — di uno stesso colpo differisce fra chi vede la sorgente e chi no. Governa questa spec: un'attivazione
  nascosta non deve lasciare un buco nel ritmo che ne riveli l'esistenza. La durata delle fasi dipendeva già dal
  viewer (rotte troncate al tratto osservato). Pinnato da `Playback.BlastSequenceIndexDependsOnTheViewer`.
- Fra le sorgenti del Blast l'ordine è quello dei pass, non quello delle unità.
- ➕ piano. Un intento d'attacco fuori portata, con bersaglio ignoto o sparito, finisce in `Fallback Cancelled`
  prima di entrare nel Blast e **non** si attiva: il TurnLog lo racconta, il playback no.
- ➕ piano. Durante l'anticipo delle attivazioni nel Dash lo scattatore non deve correre sul posto (la classe
  di difetti di #3519): la posa di corsa parte con le rotte, non con la fase.
- ➕ piano. Il recupero di fine fase del Blast scrive anche le righe `Colpo:` e passa dal predicato di confine,
  cosa che prima non faceva.

---

## 7. Follow-up candidates

- Un `CastShowSeconds` separato da `AttackShowSeconds`, se il playtest lo chiede.
- Il rimpiazzo della clip del ruolo `Attack` con un attacco vero dei pack, nel catalogo ANIM CORE.
- Attivazione delle reazioni come secondo beat accanto a `ReactionResolved`; un beat per `ArcHit` (#3293).
- Riordino per unità fra le quattro sorgenti del Blast, come filtro di presentazione.
