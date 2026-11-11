# Struttura del codice — personaggi, azioni, eventi, cartelle e Blueprint (spec panel)

> **Tipo**: referto di sessione spec-panel · **Data**: 2026-11-05 · **HEAD misurato**: `34b6c314f` (`main`)
> **Epic**: [E50 #1816](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816) ·
> [#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818)
> **Mandato dell'autore**: "ci sono classi da oltre 2000 righe; usiamo gli standard del settore; decidiamo
> l'architettura di personaggi, azioni, eventi per ottimizzare directory, file, classi, configurazioni e Blueprint".
>
> Non è un owner e non aggiunge canone. L'owner architetturale resta
> [`architettura-codice.md`](../../technical/architecture/architettura-codice.md); le decisioni restano del
> Decision Log. Qui c'è **cosa è stato misurato, cosa il panel raccomanda e quali decisioni mancano** perché
> una fetta possa partire. Ogni numero porta accanto il comando che lo produce; i numeri sono esiti della
> misura di questo passaggio e invecchiano con `HEAD`.
>
> **Panel**: Fowler e Newman (confini, moduli, dati) · Nygard, Crispin e Wiegers (modi di guasto, rete di test,
> criteri). Due coppie in agenti separati con un brief di fatti verificati e un tetto di chiamate; le premesse
> che decidono la forma sono state rimisurate da una seconda fonte prima della sintesi (§2.4). Un verificatore
> avversario ha poi riletto la bozza contro il codice con un tetto di chiamate: i rilievi confermati sono stati
> applicati (quattro cambiavano una fetta: §4.2, §4.4, §4.5, §6) e le premesse che ha chiuso stanno in §2.4.

---

## 1. Snapshot

| Voce | Valore |
|---|---|
| HEAD misurato | `34b6c314f` (merge di #3999, 2026-11-05) |
| Branch del referto | `docs/struttura-codice-spec-panel-2026-11-05`, worktree separato da `main` allo stesso commit |
| Clone principale | occupato da un'altra sessione (`docs/e52-ambiente-e-conferme-d582-2026-11-05`, PR #4008): non toccato |
| Ultimo `D-nnn` | D-582 |
| Moduli | `RefactorTactics` (Runtime) e `RefactorTacticsEditor` (Editor): `RefactorTactics.uproject` |

I referti precedenti sullo stesso tema, che questo **non ripete**:
[`architecture-hardening-spec-panel-2026-08-30.md`](architecture-hardening-spec-panel-2026-08-30.md) (audit SOLID,
prima fetta #1817) e [`architecture-hardening-e50-rimisura-spec-panel-2026-09-03.md`](architecture-hardening-e50-rimisura-spec-panel-2026-09-03.md)
(nasce `tools/architettura/misure-strutturali.py`, l'asse del churn, "i due terzi del costo stanno nei resolver").

---

## 2. La misura

### 2.1 Le classi oltre le 2000 righe esistono, e sono sei Actor più un header di widget

`find Source -type f \( -name '*.h' -o -name '*.cpp' \) -not -path '*/Tests/*' -exec wc -l {} + | sort -rn`

| File | Righe di file | Natura |
|---|---:|---|
| `Turn/RTTurnManager.cpp` | 10 478 | orchestratore + resolver Prep/Dash/Combat |
| `Player/RTPlayerController.cpp` | 6 094 | input, selezione, planning, camera, ispezione |
| `Map/RTHexMapActor.cpp` | 4 951 | vista ISM della mappa + anteprime |
| `Turn/RTTurnManager_Blast.cpp` | 4 158 | pass del Blast (`CollectAttackIntents`, `ApplyInterrupts`, `ApplyDisplacements`, `RunBlastReactions`); `ResolveCombatPasses` sta invece in `RTTurnManager.cpp:6113` |
| `Turn/RTTurnManager.h` | 4 051 | la classe: 72 `UPROPERTY`, 51 `UFUNCTION`, 30 include |
| `ScenarioHarness/RTScenarioLoader.cpp` | 3 620 | parser JSON degli scenari |
| `UI/RTScreenHudWidgets.cpp` / `.h` | 2 995 / 2 429 | 15 `UCLASS` di widget in un solo header |
| `ScenarioHarness/RTScenarioSession.cpp` | 2 894 | |
| `Unit/RTUnit.h` / `.cpp` | 2 383 / 2 256 | 106 `UPROPERTY`: stato, piano e presentazione insieme |
| `Turn/RTTurnLogLibrary.cpp` | 2 283 | hash, serializzazione, `DescribeEntry` (688 righe di testo) |
| `Map/RTHexMapActor.h` | 2 173 | |
| `Ability/RTCatalogLibrary.cpp` | 2 138 | `GetCoreActionCatalog`: 783 righe, 0 rami |
| `Turn/RTHexSimLibrary.cpp` | 2 131 | |
| `UI/RTHudViewModel.h` / `.cpp` | 2 087 / 2 040 | |
| `UI/RTHUD.cpp` | 2 081 | `DrawHUD` 631 righe (#2184) |
| `Turn/RTTurnManager_Movement.cpp` | 2 055 | risoluzione del movimento **e** la macchina della finestra di reazione, compresa la ripresa del Blast (`ResumeBlastResolution`, `ResumeSuspendedResolution`, `SubmitReactionResponse`, `PumpReactionTriggers`): 19 definizioni `ARTTurnManager::` |
| `Player/RTPlayerController.h` | 2 039 | |

Per cartella (`Source/RefactorTactics/`, produzione): `Turn/` 45 411 righe in 73 file; `Map/` 24 086 in 67;
`UI/` 19 600 in 46; `ScenarioHarness/` 15 437 in 26; `Player/` 9 903 in 11; `Ability/` 7 844 in 21;
`Unit/` 7 820 in 15. Test: 317 file `.cpp` in `Source/RefactorTactics/Tests/`
(`find Source/RefactorTactics/Tests -name '*.cpp' | wc -l`), compilati nel modulo runtime (deliberatamente:
`RefactorTactics.Build.cs:58`, #950), più 36 nel modulo Editor.

### 2.2 Lo strumento di E50, oggi

`python tools/architettura/misure-strutturali.py` su `34b6c314f`:

| Misura | 2026-09-03 (rimisura) | 2026-11-05 |
|---|---:|---:|
| `ARTTurnManager` righe di **codice** | 5 788 | 8 112 |
| metodi `ARTTurnManager::` | 102 | **187** |
| quota di file di test che spawnano un Actor | — | 42,0 % (133/317) |
| funzioni ≥ 100 righe | — | 160, pari al 33,7 % del codice di produzione |

In mezzo: `git log --no-merges --since=2026-09-03 --oneline -- 'Source/RefactorTactics/Turn/RTTurnManager*' | wc -l` → 310.

🔴 **Il gate righe/metodi dello strumento misura tre file e non vede `RTTurnManager_Movement.cpp`**
(`TURNMANAGER_FILES`, `misure-strutturali.py:160-164`): le 2 055 righe del Movement non entrano in `--check`
(la scansione delle funzioni ≥ 100 righe invece lo vede: `FinishMovementResolution` è fra le 12). Oggi "spostare
in `_Movement.cpp`" fa scendere la misura senza spostare nulla. È la prima fetta (§6, fetta 0).

⚠️ **"Metodi" nello strumento sono menzioni distinte di `ARTTurnManager::X` nei tre file, commenti compresi**
(`misure-strutturali.py:209`): un metodo spostato resta contato finché un commento lo nomina, e dopo la fetta 0
la baseline sale. La misura di accettazione delle fette usa le **definizioni**:
`grep -oE '^[A-Za-z_:<>*&, ]*ARTTurnManager::[A-Za-z]+\(' <file> | grep -oE 'ARTTurnManager::[A-Za-z]+' | sort -u | wc -l`
→ `RTTurnManager.cpp` 133, `_Blast.cpp` 20, `_Movement.cpp` 19 (172 distinte sui tre file).

Le funzioni più lunghe, per contenitore: Actor 53 funzioni / 16 354 righe; Function Library 43 / 8 251.
Le prime: `ResolveCombatPasses` 1 235 righe e 93 rami; `RTBotPlanningLibrary.cpp:98` 1 053; `ApplyDisplacements` 890;
`ResolveDash` 859; `GetCoreActionCatalog` 783 con **0 rami** (una tabella); `RebuildInstances` 693;
`DescribeEntry` 688; `ParseScenarioTurns` 677; `DrawHUD` 631.

### 2.3 Personaggi, azioni, eventi: come sono

- **Personaggio**: `ARTUnit` tiene insieme stato canonico (HP, scudo, energia, cella, status), **piano**
  (`PlannedCell/Path/Waypoints/AttackTarget/AbilityIndex/DashAbility/ReactionAbility/Facing/CoverEdge`) e
  presentazione (anim, outline, colore). `grep -oE 'Category *= *"[^"]+"' Source/RefactorTactics/Unit/RTUnit.h | sort | uniq -c`
  → Unit 50, Turn 11, Combat 9, Plan 4, Anim 4, Ability 4, HUD 1. Lo snapshot di simulazione è uno struct
  separato e magro (`FRTHexSimUnit`, `FRTUnitCombatState`), copiato a mano da `ARTTurnManager::MakeSimUnit`.
  Roster: quattro `BP_Unit_<Eroe>` in `Content/RT/Characters/<Eroe>/Blueprints/`, trovati da `ARTGameMode`
  con `ConstructorHelpers::FClassFinder` per path (`RTGameMode.cpp:593`). `URTHeroData : UPrimaryDataAsset`
  esiste come classe; **nessun** `DA_Hero_*` esiste in `Content/RT`: il catalogo eroi è una tabella C++.
- **Azione**: `FRTActionDef` + `URTActionData : UPrimaryDataAsset` (classe senza istanze in Content); il
  catalogo è `URTCatalogLibrary::GetCoreActionCatalog`, 783 righe, 0 rami, che ogni azione nuova tocca.
  Esecuzione in librerie pure (`RTActionQueueLibrary`, `RTActionEffectLibrary`, `RTActionFallbackLibrary`,
  `RTOffensiveActionLibrary`, `RTMovementActionLibrary`, `RTReactionLibrary`) chiamate dai resolver del
  TurnManager. Catalogo canonico di bilanciamento in markdown (`docs/balance/RT_ActionCatalog_v0.1.md`).
- **Eventi**: tre famiglie. `FRTActionEvent` (intento/effetto in ingresso), `FRTResolvedEvent` (esito,
  `RTServerOnly`, consumato da playback e HUD), `FRTTurnLogEntry` (canonico, versionato, due hash). Più il
  combat log testuale e la telemetria di pacing su canali separati. Non esiste un bus, e **non serve**: il
  TurnLog ha già un punto di scrittura unico, `TurnLog.Add` a `RTTurnManager.cpp:3168`, imbuto `AppendLogEntry`.
  I siti che lo chiamano stanno però dentro i resolver:
  `git grep -c -E 'AddLogEvent\(|AppendLogEntry\('` → `RTTurnManager.cpp` 112, `_Blast.cpp` 43, `_Movement.cpp` 6.
- **Input e piano**: i campi `Planned*` di `ARTUnit` sono scritti direttamente da quattro file
  (`git grep -c -E -e '->Planned[A-Za-z]+ *=[^=]' -- Source/RefactorTactics ':!Source/RefactorTactics/Tests/*'`
  → `Player/RTPlayerController.cpp` 23, `ScenarioHarness/RTScenarioSession.cpp` 19, `Turn/RTTurnManager.cpp` 18,
  `Turn/RTTurnManager_Blast.cpp` 7). Il piano lo scrive chi propone, ma anche chi risolve: "il client propone"
  è oggi una frase, non un tipo, e il piano è anche stato di risoluzione.

### 2.4 Le premesse rimisurate da una seconda fonte

| Premessa | Esito | Comando |
|---|---|---|
| L'hash di stato legge `ARTUnit` o lo snapshot? | **l'Actor**: `BuildUnitDigests(const TArray<ARTUnit*>&)` | `RTMatchStateHash.cpp:210-226` |
| `PlannedAbilityIndex` indicizza l'ordine del catalogo? | **no**: indicizza il kit dell'unità (slot 1-4) | `RTUnit.h:305-316` |
| Il corpus golden vede l'ordine di emissione? | **no**: `RTGoldenCorpusTests.cpp:778` → `CompareSerializedTraces` → `HashTurnLog(A) == HashTurnLog(B)` (`RTTurnLogLibrary.cpp:2043`), permutazione-invariante; nessun file di `Tests/Golden/` porta l'hash ordinato. ⚠️ Le righe :147 e :1106 dello stesso test nominano `HashTurnLogOrdered`: è `GoldenEntriesMatch`, un confronto **per voce**, non del corpus | `grep -rli OrderedHash Source/RefactorTactics/Tests/Golden \| wc -l` → 0; scenari: `ls Source/RefactorTactics/Tests/Golden \| wc -l` → 9 |
| L'hash ordinato (D-062) esiste ed è persistito da qualche parte? | **sì**: `HashTurnLogOrdered` (`RTTurnLogLibrary.h:447`) è scritto **per turno** nel manifest replay, `OrderedHashPerTurn` (`RTReplayManifest.h:106`), da `RecordTurnToReplay`, e testato in `RTReplayRecordingIntegrationTests.cpp:142-148`. L'oracolo dell'ordine non va inventato: va applicato al corpus | `grep -n OrderedHashPerTurn Source/RefactorTactics/Replay/RTReplayManifest.h` |
| I contesti dei resolver portano Actor? | **entrambi**, in misura diversa: Movement 1 (`TArray<TWeakObjectPtr<ARTUnit>> Units`, `:87`; `_Movement.cpp` li deferenzia, `grep -c 'Units\['` → 52), Blast 27. Lo strato senza Actor esiste solo per il Movement: `FRTMovementResolutionState` (`RTHexSim.h:348`), che il contesto **contiene** (`:68`) | `grep -c -E 'ARTUnit\*\|TWeakObjectPtr<ARTUnit>\|TObjectPtr<ARTUnit>' Source/RefactorTactics/Turn/RTMovementResolutionContext.h Source/RefactorTactics/Turn/RTBlastContext.h` → 1, 27 (il pattern `ARTUnit\*` da solo dà 0 e 27: non vede il `TWeakObjectPtr`) |
| Dove sta `ResolveCombatPasses` e chi chiama `RunReactionPass`? | `ResolveCombatPasses(FRTBlastContext&)` è in `RTTurnManager.cpp:6113` e nel suo corpo `RunReactionPass(` non compare; lo chiamano tre funzioni di `_Blast.cpp`: `RunBlastReactions` (:2386), `ApplyDisplacements` (:3217), `ApplyControlStatuses` (:4009). La **ripresa** (`ResumeBlastResolution`, `ResumeSuspendedResolution`) è definita in `_Movement.cpp` | `git grep -n 'ARTTurnManager::ResolveCombatPasses' -- 'Source/RefactorTactics/Turn/*.cpp'`; `grep -nE '^\s*[^/]*RunReactionPass\(' Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp` |
| Valori di `ERTCombatOutcome` coperti dai test | 6 su 6 (`Hit`, `ShieldAbsorbed`, `Lethal`, `NoLineOfSight`, `TerrainBonus`, `Healed`), 122 riferimenti in 20 file | `git grep -ohE 'ERTCombatOutcome::[A-Za-z]+' Source/RefactorTactics/Tests/ \| sort -u` |
| Cosa coprono i test sulla sospensione | la sospensione vista da input (`RTSuspendedResolutionInputTests`), HUD (`RTReactionWindowViewModelTests`), playback (`RTPlaybackActivationTests`, `RTAttackTracerPlaybackTests`), le controreazioni (`RTDefensiveReactionTests`) e la ripresa del **Movement** (`RTMovementResumeTests`: `WindowSuspendsResolution`, `ResolveMovementFingerprintIsStable`); **nessuno** caratterizza lo stato di `FRTBlastContext` attraverso `ResumeBlastResolution` | nomi dei test nei 6 file |
| Un test vincola la classe madre dei `BP_Unit_*`? | **no**: `RTPackagingConfigTests` li nomina solo per clip e dipendenze di cook; nessun `IsChildOf`/`ParentClass` | `git grep -n 'BP_Unit_' -- Source/RefactorTactics/Tests/RTPackagingConfigTests.cpp` |
| Include da `Turn/` verso `UI/`, `Player/`, `Camera/` | 2: `RTReactionConditionConsole.cpp:6` (`Player/RTPlayerController.h`) e `RTTurnManager.cpp:19` (`Player/RTPlayerState.h`, la porta `TeamIdOf` di D-285) | `git grep -n -E '#include "(UI\|Player\|Camera)/' -- Source/RefactorTactics/Turn` |
| Guardie di autorità dentro i resolver | **nessuna**: `HasAuthority\|RTServerOnly` → `RTTurnManager.cpp` 1 (un commento), `_Blast.cpp` 0, `_Movement.cpp` 0. Spostare un resolver non perde una guardia, perché non c'è: l'autorità oggi è il fatto che il TurnManager è l'unico a chiamare | `grep -c -E 'HasAuthority\|RTServerOnly' <file>` |
| Superficie Blueprint del TurnManager | 47 `BlueprintCallable`/`BlueprintPure` in `RTTurnManager.h`; stringhe `GetMatchResult` in `WBP_RT_ResultScreen` e `GetPhase` in `WBP_RT_ErrorModal`/`WBP_RT_LoadingScreen` (hit di stringa negli asset, non prova di chiamata) | `grep -cE 'BlueprintCallable\|BlueprintPure' Source/RefactorTactics/Turn/RTTurnManager.h` |
| Firme dei resolver | `ResolvePrep()`, `ResolveDash()`, `ResolveCombat()`, `ResolveMovement()` senza parametri (leggono membri); `ApplyDisplacements(FRTBlastContext&)`, `ResolveCombatPasses(FRTBlastContext&)` | `RTTurnManager.h:1779-1782, 2070, 2101` |
| Ganci `*ForTest` chiamati da codice **non** di test | **2**: `RTScenarioSession.cpp:1796` (`PlanBotsForTest`) e `RTPlayerController.cpp:1980` (`OrbitCameraForTest`) | `git grep -n 'ForTest(' -- 'Source/RefactorTactics/*.cpp' ':!Source/RefactorTactics/Tests/*'` meno le definizioni |
| Occorrenze `ForTest` negli header di produzione | PC 50, TurnManager 29, ScreenHudWidgets 25, CameraPawn 18, Unit 14 | `grep -c ForTest <header>` |
| Fan-in di `Turn/RTTurnManager.h` | produzione 16, test 106 | `git grep -l '"Turn/RTTurnManager.h"' -- Source ':!*/Tests/*' \| wc -l` |
| Librerie che includono l'Actor o il TurnManager | 3: `RTActionQueueLibrary.cpp`, `RTPlanValidationLibrary.cpp`, `RTKnowledgeVeilPresenter.cpp` | `git grep -l -E '"(Unit/RTUnit\|Turn/RTTurnManager)\.h"' -- Source/RefactorTactics/Combat Source/RefactorTactics/Pathfinding Source/RefactorTactics/Perception 'Source/RefactorTactics/Turn/*Library*'` |
| `IsDataValid` nei `UPrimaryDataAsset` | 1 file su tutte le classi dati: `RTHexMapAsset.cpp` | `git grep -l IsDataValid -- Source/RefactorTactics ':!*/Tests/*'` |
| Test che nominano `PlannedAttackTarget` | 55 file | `git grep -l PlannedAttackTarget -- Source/RefactorTactics/Tests \| wc -l` |
| Test sulla sospensione del Blast | 6 file nominano `ResumeSuspendedResolution`/`IsResolutionSuspended` | `git grep -l -E 'ResumeSuspendedResolution\|IsResolutionSuspended' -- Source/RefactorTactics/Tests \| wc -l` |

---

## 3. Dentro cosa si lavora (non in discussione)

- **#1818**: "la direzione NON è spezzare in sei classi: è togliere i carichi che non sono orchestrazione, un
  carico alla volta, con misura prima/dopo"; nessuna classe a priori; nessun rename dei metodi pubblici consumati
  da HUD, PlayerController, harness e test, **e dai Blueprint** (47 `BlueprintCallable`/`BlueprintPure` in
  `RTTurnManager.h`, §2.4: ogni fetta che ne tocca uno dichiara quali `WBP_RT_*` lo nominano); criterio per
  fetta: riduce almeno una fra quota di test con mondo,
  righe di codice, metodi. Fette già mergiate: #1817, #1863, #1907, #2171, #2174, #2178. Aperte: #2184, #2182,
  #1821, #1820 (DEFER a E40), epic #2586.
- **D-229**: la sorgente è il codice, l'asset è il suo output (mesh, materiali, `DA_IconCatalog`, `ABP_RTUnitAuthored`).
  **D-248**: il grafo di locomozione vive in C++, non in un ABP per eroe. **ADR-0010**: lo Scenario Harness si
  affaccia su Blueprint da una facade sola. **ADR-0003**: fasi `Prep → Dash → Blast → Move → Cleanup` fisse.
- `Tests/` compilata in ogni target (#950): ogni volta che il codice di test è uscito dal perimetro ha rotto la
  sola build Shipping.
- Invarianti: determinismo (TurnLog, golden), autorità (client propone, server valida e applica), privacy del
  piano avversario, simulazione separata dalla presentazione, "C++ definisce ciò che è permesso, Data e Blueprint
  configurano una variante permessa".

---

## 4. Le risposte del panel, per domanda

Per ogni domanda: la posizione su cui il panel converge, il fatto che la regge, la forma concreta, il rischio,
la misura. I disaccordi sono in §4.10.

### 4.1 Moduli Unreal (Q1): nessun modulo nuovo ora; il confine si **misura** oggi e si **compila** quando la misura è a zero

- **Fatto**: due moduli, nessun `Public/Private` (`PublicIncludePaths.Add(ModuleDirectory)`: ogni header è
  pubblico per costruzione), `REFACTORTACTICS_API` su 161 file di produzione, 317 file di test nel modulo
  runtime, #950. Un modulo `RTSim` oggi significa decidere in un colpo l'API esportata di 161 file: lo stesso
  Big Bang che #1818 ha rifiutato per il TurnManager, a livello di build.
- **Forma**: una **regola di direzione** scritta nella mappa del codice e fatta rispettare da un test `Meta`
  (il progetto ha già l'idioma, `RefactorTactics.Meta.*`): `Combat/`, `Pathfinding/`, `Perception/`
  (eccetto `*Presenter*`) e `Turn/*Library*` non includono `Unit/RTUnit.h`, `Turn/RTTurnManager.h`,
  `Engine/World.h`. Nella mappa del codice una tabella "cartella → modulo di destinazione" (`RTSim`: solo
  `Core`, `CoreUObject`, `GameplayTags`; `RefactorTactics`: Actor, UI, Player, Camera; `RefactorTacticsEditor`
  com'è). Il nome entra nella tabella; il `.Build.cs` no. `Public/Private` **non si applica** al modulo
  esistente (churn su centinaia di include senza vincolo nuovo): si crea solo nel modulo nuovo, solo per gli
  header che attraversano il confine.
- **Rischio**: nullo per golden e determinismo; di processo: un'estrazione a metà lascia due convenzioni di include.
- **Misura**: il comando della riga "librerie che includono l'Actor" di §2.4, da 3 a 0 (il presenter escluso per
  nome); `git grep -l -E '#include "(UI|Player|Camera)/' -- Source/RefactorTactics/Turn | wc -l` come gate di
  direzione (non misurato in questo passaggio).

### 4.2 Personaggio (Q2): struct puri, non componenti; la prima fetta è il **piano**, non lo stato

- **Fatto**: 106 `UPROPERTY` in `ARTUnit`; il piano tiene `TObjectPtr<ARTUnit> PlannedAttackTarget`;
  `RTActionQueueLibrary.h` nomina `ARTUnit` 7 volte; i campi `Planned*` sono scritti dal PlayerController (23),
  dall'harness (19) **e dai resolver** (18 + 7, §2.3). Il piano non è estraibile *perché* punta all'Actor, e il
  resolver è inquinato dall'Actor *perché* il piano glielo porta. Lo strato senza Actor esiste solo per il
  Movement (`FRTMovementResolutionState`), ma il suo contesto tiene ancora i `TWeakObjectPtr<ARTUnit>` (1 riga
  contro le 27 del Blast, §2.4).
- **Perché non `UActorComponent`**: un componente è un `UObject` con mondo: non si copia in uno snapshot, non
  si hasha, e il progetto ne ha zero. I componenti servono alla presentazione (anello di squadra, freccia di
  facing, outline), dopo.
- **Forma**, in due fette separate:
  1. `USTRUCT FRTUnitPlan` in `Turn/RTUnitPlan.h` (file nuovo) che assorbe i campi `Planned*` e
     `bTurnPlanDeclared` di `Unit/RTUnit.h`, con `PlannedAttackTargetStableId` al posto del puntatore.
     `ARTUnit` tiene **un** membro `FRTUnitPlan Plan`; i getter pubblici restano (vincolo #1818). È l'**API**
     fra chi propone e chi valida: tipizzarla **prima** che E40 online la serializzi, non dopo.
  2. `FRTUnitCanonicalState` (nome da decidere nella fetta) che fonde ciò che `FRTHexSimUnit` e
     `FRTUnitCombatState` copiano a mano: è il tipo che assorbe `MakeSimUnit` fuori dal TurnManager e che
     fa scendere i 27 `ARTUnit*` di `RTBlastContext.h`, un campo alla volta.
- **Roster**: `DA_Hero_*` **generati** da `URTHeroCatalogLibrary` via commandlet (D-229, il modulo Editor ha
  già i generatori), con `TSoftClassPtr<ARTUnit> PresentationClass` al posto del `FClassFinder` di
  `RTGameMode.cpp:593`; `URTHeroData::IsDataValid` confronta l'asset con la riga della tabella. Direzione
  fissa: C++ → asset, mai il contrario.
- **Rischio**: privacy: una fetta che lascia due copie (campi vecchi e struct nuovo) lette da chiamanti diversi.
  Semantica: i resolver **scrivono** il piano durante la risoluzione (25 assegnazioni in `RTTurnManager*.cpp`):
  un resolver che scrive `Planned*` usa il piano come stato di risoluzione, e la fetta deve dire, per ciascuna
  di quelle scritture, se è un fallback dichiarato (resta nel piano, e va nel TurnLog) o stato di fase (va nel
  contesto). Hash: `BuildUnitDigests` legge i campi dell'Actor; spostarli in uno struct non cambia i valori, e
  il golden è il gate. Nygard: ogni campo spostato cambia *quando* viene letto (snapshot o live); un valore letto a metà fase
  non fa rosso il golden se i 9 scenari non hanno quel caso. Criterio di Wiegers: per ogni campo spostato un
  test che lo **muta fra snapshot e risoluzione** e dimostra che il resolver legge lo snapshot.
- **Misura**: `grep -c UPROPERTY Source/RefactorTactics/Unit/RTUnit.h` in discesa del numero di campi `Planned*`;
  `git grep -c ARTUnit -- Source/RefactorTactics/Turn/RTActionQueueLibrary.h` da 7 a 0; le assegnazioni dirette
  `->Planned* =` fuori dai test (comando di §2.3: 23 + 19 + 18 + 7) a 0 fuori dal setter del piano, con le 25 dei
  resolver classificate prima; `grep -c -E 'ARTUnit\*|TWeakObjectPtr<ARTUnit>' Source/RefactorTactics/Turn/RTBlastContext.h`
  da 27 in giù; golden invariato;
  `git grep -c FClassFinder -- Source/RefactorTactics/RTGameMode.cpp` a 0. Costo dichiarato: 55 file di test
  nominano `PlannedAttackTarget`: la fetta del piano si fa a tappe, con il getter che resta.

### 4.3 Azioni e cataloghi (Q3): la sorgente resta C++; la tabella smette di essere una funzione; **un solo validator** e nessun asset autorato

- **Fatto**: `GetCoreActionCatalog` 783 righe, 0 rami, rende `TArray<FRTActionDef>` per valore; `docs/balance`
  è una seconda verità in markdown; `IsDataValid` esiste in 1 file. `PlannedAbilityIndex` indicizza il kit
  dell'unità, non l'ordine del catalogo (§2.4): riordinare la tabella **non** muove il replay via quell'indice;
  l'ordine del kit per eroe sì.
- **Forma**: `Ability/Catalog/RTActionCatalog_<Famiglia>.cpp` (uno per eroe più `Core`), ciascuno con un
  array statico di `FRTActionDef`; `GetCoreActionCatalog` resta con la stessa firma e concatena nell'ordine di
  oggi. `URTCatalogLibrary::ValidateCatalog(TArray<FText>& OutErrors)`: `ActionId` unici, `Slot` unico per
  eroe, `ResolutionPhase` ammessa da ADR-0003, `Effects` non vuoto; lo chiama un test. Un test `Meta` che parsa
  la tabella di `docs/balance/RT_ActionCatalog_v0.1.md` e la confronta con il catalogo C++: costa un file e
  uccide la divergenza oggi; il generatore markdown (un commandlet D-229) viene dopo, se serve.
- **`DA_Action_*`**: **no** finché non c'è un consumatore (un designer senza compilatore, o un Blueprint che
  legge il kit). Crispin: un asset *autorato* è una seconda sorgente che il golden non vede: una costante
  cambiata nell'asset rende rosso il corpus con un diff vuoto nel C++. Se un giorno si generano, con
  `IsDataValid` round-trip contro la tabella.
- **Misura**: `GetCoreActionCatalog` sparisce dalle 12 funzioni più lunghe di `misure-strutturali.py`; test
  `RefactorTactics.Catalog.OrderIsFrozen` con la lista degli `ActionId` nell'ordine di oggi (e, per eroe,
  l'ordine del kit); `git grep -c 'public UPrimaryDataAsset' -- Source/RefactorTactics` contro il numero di
  `IsDataValid`: ogni classe dati **autorata** ne ha uno.

### 4.4 Eventi (Q4): le tre famiglie sono giuste; non serve un bus; serve che un resolver spostato **non chiami `this`**

- **Fatto**: TurnLog con imbuto unico (`RTTurnManager.cpp:3168`); i chiamanti dentro i resolver (112 + 43 + 6);
  `FRTResolvedEvent` costruito in 14 siti
  (`git grep -c -E 'FRTResolvedEvent [A-Za-z]+;|FRTResolvedEvent [A-Za-z]+\(|Emplace_GetRef\(\)' -- Source/RefactorTactics/Turn/RTTurnManager*.cpp`
  → `RTTurnManager.cpp` 12, `_Blast.cpp` 1, `_Movement.cpp` 1); combat log testuale sparso. È CQRS con il
  TurnLog come event store: intento in, esito out, log canonico.
- **Forma**: l'**emettitore** ricevuto per riferimento dal resolver spostato, che assorbe `AppendLogEntry`,
  `AddLogEvent`, la produzione di `FRTResolvedEvent` e le `Append*Entry`/`MakeStatus*Entry` di
  `RTTurnManager.h`. Non nasce a priori: nasce **nella fetta del primo resolver spostato** (§6, fetta 2) ed esce
  da `RTTurnManager.cpp`. `DescribeEntry` (688 righe) è presentazione del log, non log: va in un file suo
  (`RTTurnLogDescriptions.cpp`) nella stessa cartella.
- **Modo di guasto** (Nygard): spostare un resolver non cambia *cosa* emette, cambia facilmente *in che ordine* e
  *rispetto a cosa* (prima o dopo `RunReactionPass`), e **nessun gate automatico lo vede** (§2.4: il corpus è
  cieco all'ordine). Lo vedono playback, event log dell'HUD e manifest replay, tutti in PIE. Il riordino di
  emissione è il difetto che la suite verde *garantisce* di non trovare.
- **Rete da scrivere PRIMA di qualunque fetta sui resolver** (Crispin): (a) oracolo dell'ordine, **riusando
  ciò che esiste** (§2.4): il manifest replay persiste già `OrderedHashPerTurn` per ogni turno; la fetta registra
  il manifest dei 9 scenari golden e lo confronta, invece di inventare un formato sidecar. Controllo positivo:
  rosso se si scambiano due voci dello stesso turno in memoria; (b) per ogni voce Combat/Move del TurnLog esiste
  un `FRTResolvedEvent` con lo stesso `(Turn, Phase, Src, Tgt, ActionId)`. Privacy: i resolver leggono
  `KnowledgeForTeam` (`grep -c` → `RTTurnManager.cpp` 7, cioè anche Prep e Dash; `_Blast.cpp` 5;
  `_Movement.cpp` 0); la fetta dichiara che la lettura passa dalla stessa funzione e lo misura.
- **Misura**: `git grep -c 'TurnLog\.Add' -- 'Source/RefactorTactics/*.cpp' ':!Source/RefactorTactics/Tests/*'`
  = 1 prima e dopo; i produttori di `FRTResolvedEvent` dopo la fetta in un solo file; `OrderedHashPerTurn` identico;
  `git grep -c 'Append.*Entry\|MakeStatus' -- Source/RefactorTactics/Turn/RTTurnManager.h` a 0.

### 4.5 TurnManager (Q5): riaprire il perimetro dei resolver, con due precondizioni, **Movement prima e Blast per ultimo**

- **Fatto**: 102 → 187 metodi e 5 788 → 8 112 righe di codice in due mesi, 310 commit; il gate `--check` esiste e
  non ha frenato perché la classe cresce **per gameplay**: finché i resolver stanno dentro, ogni feature atterra
  nel God Object per costruzione. #2182 misura che il 97,5 % degli `SpawnActor<ARTTurnManager>` sta in test che
  girano turni veri: delle tre misure di #1818 la quota con mondo **non può muoversi** con una fetta sui
  resolver; restano righe di codice e metodi. I resolver sono già funzioni di un contesto:
  `ApplyDisplacements(FRTBlastContext&)`, `ResolveCombatPasses(FRTBlastContext&)`; il Movement attraversa più
  frame con `FRTMovementResolutionContext`, che contiene lo stato puro `FRTMovementResolutionState` e un elenco
  di `TWeakObjectPtr<ARTUnit>` (§2.4).
- **Posizione sul golden**: è una ragione per muoverli **solo dopo** che l'oracolo dell'ordine, che il replay
  già persiste per turno, è applicato al corpus; oggi il corpus è una rete con il buco esattamente dove lo
  spostamento crea rischio. L'esclusione di #1818 era corretta quando scritta (nessun oracolo ordinato sul
  corpus, strumento di misura nato dopo); i dati di oggi dicono che il suo prezzo è la crescita per gameplay.
  Va scritta come decisione con il prezzo accanto (§7).
- **Modi di guasto**: (1) lo strumento non vede `_Movement.cpp`; (2) **rientro**: `RunReactionPass` è chiamato
  da tre funzioni di `_Blast.cpp` (`RunBlastReactions`, `ApplyDisplacements`, `ApplyControlStatuses`, §2.4), il
  contesto ha `bSuspended`, e la **ripresa** (`ResumeBlastResolution`, `ResumeSuspendedResolution`) è definita
  in `_Movement.cpp`: una funzione libera che si sospende a metà cambia chi possiede la ripresa, osservabile in
  PIE con finestre di reazione aperte; i 6 file di test sulla sospensione coprono input, HUD, playback e la
  ripresa del Movement, **nessuno** lo stato di `FRTBlastContext` attraverso `ResumeBlastResolution` (§2.4);
  (3) ordine di emissione (§4.4).
- **Forma**: funzioni libere in `Turn/Resolution/RT<Fase>Resolver.{h,cpp}` con firma
  `(FRT<Fase>Context&, <emettitore>&)`. `ResolveDash()` e `ResolvePrep()` non hanno parametri: la loro fetta
  è in due tempi: prima il contesto estratto **da** quel corpo (i membri che legge, elenco prodotto nella fetta),
  poi la funzione fuori.
- **Sul gate** (Nygard): un gate relativo al merge-base è *già* negoziato a ogni feature, come dimostra
  102 → 187. Alternativa: la classe `ARTTurnManager` ha un **tetto assoluto** (metodi e righe di codice di oggi,
  in discesa per fetta) che una feature non può alzare: la feature atterra nel resolver, fuori dall'Actor; il
  gate misura `Turn/` intero più i file dei resolver. Wiegers dissente (§4.10).
- **Misura per fetta**: `python tools/architettura/misure-strutturali.py --base <sha-prima>` dopo `git fetch`
  (il confronto è con `origin/main`: senza fetch è stantio); le **definizioni** `ARTTurnManager::` nei file
  del TurnManager (comando di §2.2, oggi 172) in discesa del numero di funzioni spostate, perché il conteggio
  dello strumento include le menzioni nei commenti; il file nuovo non nomina membri del TurnManager
  (`grep -c 'this->'` e l'elenco dei membri letti, a 0); golden dell'insieme e `OrderedHashPerTurn` identici.

### 4.6 PlayerController (Q6): il confine è "ciò che decide" contro "ciò che inoltra"

- **Fatto**: ~163 funzioni, 36 ganci `*ForTest(` (50 occorrenze della parola `ForTest`: i due pattern di §2.4
  contano cose diverse), il massimo del progetto; la camera ha già una casa (`Camera/`, 2 584 righe,
  `RTCameraPawn.h` con 18 occorrenze `ForTest`); il PC chiama un proprio gancio di test dalla produzione
  (`RTPlayerController.cpp:1980`); i campi `Planned*` scritti in 23 punti. Un gancio `ForTest`
  su una decisione è la prova che quella decisione non ha una firma pura.
- **Forma**: tre carichi: routing dell'input (Enhanced Input in C++), **sessione di planning** (selezione →
  hotkey → waypoint → intento proposto, cioè il produttore di `FRTUnitPlan` di §4.2), camera. Le funzioni camera
  del PC inoltrano a `Camera/`; la sessione di planning esce da `RTPlayerController.cpp` portandosi le funzioni
  che oggi hanno un gancio `ForTest`, una alla volta, e diventa testabile senza mondo. Nessuna classe a priori:
  il tipo prende il nome dal primo carico che assorbe.
- **Modo di guasto**: `BuildInputMappings` costruisce i binding in C++: una funzione spostata e non ricablata
  compila *bene* e il tasto non fa nulla, osservabile solo in PIE. Prima di toccare il routing, un test che
  enumera i binding attesi contro quelli costruiti. Privacy: il PC è lato client; la sessione di planning legge
  lo stato avversario solo da `FilterForTeam`/`FRTKnowledgeView`: oggi zero replica, quindi il guasto è latente
  fino a E40, il peggior momento per scoprirlo.
- **Misura**: `grep -c 'ForTest(' Source/RefactorTactics/Player/RTPlayerController.h` da 36 in discesa; un
  file di test nuovo con `grep -c SpawnActor` = 0 che copre le decisioni spostate (se non si può scrivere senza
  mondo, la fetta ha estratto la cosa sbagliata); le funzioni camera del PC senza aritmetica, solo inoltro.

### 4.7 Cartelle (Q7): feature folder **dentro `Turn/`**, come esito delle fette, mai come `git mv` di 73 file

- **Fatto**: `Turn/` tiene 73 file di sei nature (`ls Source/RefactorTactics/Turn | grep -ciE '<chiave>'`:
  `reaction` 8, `pacing` 7, `playback|presentation|planpreview` 6, `turnlog|combatlog|matchstatehash` 7,
  `matchformat|matchsetup` 5, `replay` 2, più resolver e contesti); `PublicIncludePaths` sulla radice del
  modulo: ogni spostamento è churn di include, anche nei 317 file di test; 310 commit di storia su
  `RTTurnManager*` sono l'archeologia con cui si debugga il prossimo rosso del golden.
- **Forma**: `Turn/Resolution/` (contesti, `*ActionLibrary`, `RTHexSim*`, `RTTurnRules`, i resolver spostati),
  `Turn/Reaction/`, `Turn/Log/` (`RTTurnLog*`, `RTCombatLog`, `RTMatchStateHash`, le descrizioni),
  `Turn/Pacing/`, `Turn/Playback/` (`RTPlaybackLibrary`, `RTPresentationBinding`, `RTPlanPreview`),
  `Turn/Format/`; `RTReplayRecording` esce verso `Replay/`. Newman preferisce `Reaction/` top-level perché ha
  tipi, libreria, test e un consumatore esterno (l'HUD legge `FRTReactionWindowView`): differenza di grado.
  Il vertical slice puro **non si applica**: la simulazione è un kernel condiviso, non feature indipendenti;
  `UI/`, `Map/`, `Core/` restano per layer.
- **Regola**: nessuna cartella nuova senza almeno un file che ci arriva da una fetta misurata; uno spostamento
  è un commit **senza semantica** (golden invariato) che aggiorna gli include dei test nello stesso commit
  (#950: la build Shipping è la prima a saperlo).
- **Misura**: per ogni header spostato `git grep -l '"Turn/<File>.h"' -- Source | wc -l` = file da toccare,
  dichiarato prima; dopo, `ls Source/RefactorTactics/Turn/*.h | wc -l` tiene solo orchestratore e tipi condivisi.

### 4.8 Blueprint e configurazione (Q8): Blueprint = ciò che non ha un numero che cambia un esito

- **Fatto**: `ABP_RTUnitAuthored`, `M_HexVolume`, `DA_IconCatalog`, `UI/_Generated/` sono già output di
  commandlet (D-229, D-248); `RTScreenHudWidgets.h` dichiara che nessun accessor dà il TurnManager o un
  `ARTUnit` a un Blueprint; due pipeline di presentazione convivono (`DrawHUD` immediate-mode e UMG).
- **Lista chiusa di ciò che è Blueprint**: `BP_Unit_<Eroe>` (mesh, materiali, varianti visive),
  `BP_GameMode`, `BP_RTMapPresentation_*`, `WBP_RT_*` (layout e stile). Tutto ciò che è competitivo è C++ o
  asset generato con validator. Predicato testabile (Wiegers): un `BP_Unit_*` eredita da `ARTUnit` e non
  sovrascrive proprietà delle categorie `Turn`/`Combat`/`Plan`; il posto è un test di packaging/config, se non
  c'è ancora.
- **`Content/RT`**: `Data/Heroes/_Generated/DA_Hero_*` (generati), `Data/Formats/DA_Format_*` (autorati: una
  variante permessa; `URTMatchFormatData::IsDataValid` oggi **non esiste** e va scritto, §2.4: l'unico format
  autorato è `Maps/Dev/L_DevSandbox/Data/DA_Format_Scratch`), `Maps/<Mappa>/Data/DA_HexMap_*` restano accanto
  alla mappa. La convenzione `Data/` è proposta, non stato. Convenzione unica: **ogni output di commandlet sta sotto un `_Generated` e non si edita a
  mano**; test `Meta` che ogni generatore scrive solo lì (il giorno in cui qualcuno edita a mano un generato, il
  commandlet lo sovrascrive senza errore).
- **MVVM di UE 5.x**: **non si applica ora**: `URTHudViewModel` è già il view model letto come DTO; il plugin
  aggiunge FieldNotify e una dipendenza per un beneficio che un dev singolo senza designer UMG non incassa.
  Si rivaluta quando l'HUD ha una sola pipeline: quella fusione è #2184 e viene prima di qualunque regola di naming.
- **Misura**: `git grep -n 'BlueprintCallable\|BlueprintPure' -- Source/RefactorTactics | grep -c 'ARTUnit\*\|ARTTurnManager\*'`
  è **già 0** su `34b6c314f`: è una guardia che non deve salire, non un obiettivo. La misura che parte sopra
  zero è la superficie Blueprint del TurnManager, `grep -cE 'BlueprintCallable|BlueprintPure' Source/RefactorTactics/Turn/RTTurnManager.h`
  → 47: ogni fetta che ne sposta uno dichiara quali `WBP_RT_*` lo nominano (scansione di stringhe degli asset).
  `IsDataValid` presente in ogni `UPrimaryDataAsset` autorato (oggi 1 su 6 classi:
  `git grep -c 'public UPrimaryDataAsset' -- Source/RefactorTactics`); `ls Content/RT/Data/Heroes/_Generated/DA_Hero_*.uasset | wc -l`
  uguale alle righe del catalogo eroi (test `Meta`).

### 4.9 Test e ganci `*ForTest` (Q9): né modulo di test separato, né `friend` indiscriminato: prima la classificazione

- **Fatto**: 105 occorrenze `ForTest(` negli header di produzione
  (`git grep -c 'ForTest(' -- 'Source/RefactorTactics/*.h' ':!Source/RefactorTactics/Tests/*' | awk -F: '{s+=$2} END{print s}'`);
  2 ganci chiamati dalla produzione (§2.4); #950; #2182: il pavimento della quota con mondo è l'allestimento,
  non l'orchestrazione, e la quota è **salita** (39,3 % alla baseline di #2182, 42,0 % oggi) perché i test
  nuovi girano il flusso vero. Un test con mondo che gira `LockInAndResolve` è copertura d'integrazione, non debito.
- **Forma**: ogni gancio è una di tre cose: **(a)** *imposta* stato che la produzione non sa impostare → è un
  parametro mancante di un contesto: la fetta giusta è quella di §4.5/§4.6, non si tocca qui; **(b)** *legge*
  stato non esposto → una query che manca all'API, o `friend struct FRT<Classe>TestAccess` dichiarato in
  `Tests/`, con tetto per classe; **(c)** *scorciatoia* di flusso (salta una fase) → va in
  `URTScenarioAuthoring` (ADR-0010), la facade che esiste già. `PlanBotsForTest` chiamato da
  `RTScenarioSession.cpp:1796` è un (c) già usato come API: va rinominato come tale, non nascosto.
  Un modulo di test separato è lo stato finale corretto, ma ribalta #950 e i 106 test che includono
  `RTTurnManager.h` sono il suo prezzo: prima gli accessor, poi il modulo, se mai.
- **Rete**: ogni gancio (a) rimosso senza il parametro corrispondente toglie al test l'unico modo di costruire
  quel caso, e il test resta verde perché *non costruisce più il caso*. Criterio: il test che lo usava ha un
  asserto che fallisce se il caso non è costruito (controllo positivo), e resta.
- **Misura**: inventario dei 105 per classe (a)/(b)/(c) in un commento di issue; `grep -c 'ForTest('` per header
  in discesa; `grep -c friend` per header ≤ 1; la riga "file che spawnano un Actor" di `misure-strutturali.py`
  non sale; nessun file di test nuovo con `SpawnActor` che non giri il flusso.

### 4.10 Disaccordi dichiarati

1. **`DA_Action_*` generati** (Fowler: sì, l'asset è il contratto leggibile) contro **C++ più test sul markdown**
   (Newman, Crispin: cerimonia senza consumatore, e una seconda sorgente che il golden non vede). Convergenza:
   validator e test d'ordine comunque; il generatore aspetta un consumatore.
2. **Modulo `RTSim`**: conseguenza di una misura a zero (Fowler, Nygard) contro **nominato e datato ora** perché
   una convenzione non ha retto (Newman, Wiegers). Convergenza: il nome entra nella mappa del codice con il
   prezzo scritto (Build.cs, #950); il `.Build.cs` no.
3. **Blast: quando**. Nygard: ultimo, e non prima di un test di caratterizzazione della sospensione/ripresa;
   Crispin: è dove stanno rischio e valore, e rinviarlo a "dopo tutto il resto" è il modo in cui non si fa mai:
   i test di caratterizzazione per esito si scrivono adesso, con mondo, e servono anche senza spostare nulla;
   Wiegers: senza un criterio scritto per "ripresa preservata" il Blast non è accettabile per #1818 a
   prescindere dal calendario.
4. **La quota con mondo nel criterio di #1818** per le fette sui resolver: toglierla (Wiegers: #2182 prova che non
   può muoversi, e un criterio immobile è falso) contro tenerla con la definizione di #2182, spawn senza flusso
   (Crispin: è l'unico numero che impedisce di "estrarre" una funzione portandosi dietro il mondo). Nygard:
   entrambe, se lo strumento stampa le due righe separate (già lo fa: "ALLESTISCONO soltanto").
5. **Tetto assoluto** sulla classe (Nygard) contro **gate relativo per fetta** (Wiegers: un tetto assoluto
   viene alzato alla prima urgenza e diventa un numero negoziato; Nygard: il gate relativo è già negoziato a
   ogni feature, come dimostra 102 → 187).
6. **Feature folder**: sottocartelle di `Turn/` (Fowler) contro `Reaction/` top-level (Newman).
7. **Accessor `friend`**: economico e rumoroso al guasto (Nygard) contro "nasconde la cucitura mancante: ogni
   gancio sotto `friend` è un parametro che non verrà mai aggiunto" (Crispin). Compromesso: solo per i (b), con
   conteggio pubblicato e tetto per classe.

---

## 5. L'architettura bersaglio, in una pagina

Il bersaglio non è un numero di classi: è **la direzione delle dipendenze resa misurabile**, e poi compilata.

```
RTSim (oggi: una regola di direzione; domani: un modulo)      dipende solo da Core, CoreUObject, GameplayTags
  Core/           tipi, tag, guardie
  Map/            FRTCellId, FRTHexCellData, URTHexMapAsset, librerie hex/vision/cover/path   (senza ARTHexMapActor)
  Ability/        FRTActionDef, cataloghi per famiglia, validator, URTHeroData/URTActionData (classi)
  Turn/Resolution/ contesti per fase, resolver come funzioni (ctx&, emettitore&), RTHexSim*, RTTurnRules
  Turn/Reaction/  opportunità, decisioni, finestre (DTO)
  Turn/Log/       FRTTurnLogEntry, hash (insieme e ordinato), serializzazione, emettitore, descrizioni
  Turn/Format/    URTMatchFormatData, regole risolte
  Combat/ Perception/ Pathfinding/ Terrain/ Bot/   librerie pure (già oggi a zero GetWorld)
  Turn/RTUnitPlan.h  FRTUnitPlan: l'intento proposto, con StableUnitId, filtrabile per squadra
  (stato canonico dell'unità: lo struct che assorbe MakeSimUnit e svuota RTBlastContext dagli ARTUnit*)

RefactorTactics (Runtime: Actor, input, presentazione)       dipende da RTSim
  Turn/RTTurnManager    orchestratore: fasi, ready, timer, snapshot, chiamate ai resolver, replay recording
  Turn/Playback/        riproduce, non decide (#1817)
  Turn/Pacing/          telemetria
  Unit/ARTUnit          presentazione + FRTUnitPlan Plan + stato canonico come membro struct
  Player/               routing input → sessione di planning (produce FRTUnitPlan) → inoltro camera
  Camera/ UI/ Map/ARTHexMapActor World/ Frontend/ Replay/ (viewer) PieSession/ Debug/
  ScenarioHarness/      facade unica per Blueprint (ADR-0010)
  Tests/                nel modulo (#950); ganci di test: (a) parametri, (b) accessor friend, (c) harness

RefactorTacticsEditor                                          generatori D-229, tool, test editor

Content/RT
  Characters/<Eroe>/Blueprints/BP_Unit_<Eroe>   (presentazione; non sovrascrive Turn/Combat/Plan)
  Data/Heroes/_Generated/DA_Hero_*               (output del catalogo C++)
  Data/Formats/DA_Format_*                       (autorati; IsDataValid da scrivere)
  Maps/<Mappa>/Data/DA_HexMap_*                  (accanto alla mappa)
  UI/Framework, UI/Match, UI/Profile (WBP_RT_*)  UI/_Generated  Anim/ (generato)  World/  Core/
```

Ciò che **non** cambia: `ERTMatchPhase` e l'ordine delle fasi; i metodi pubblici consumati da HUD, PC, harness e
test; `HashTurnLog`; il formato del TurnLog; la posizione di `Tests/`; la cartella `Content/RT/Maps`.

---

## 6. La sequenza delle fette (una issue ciascuna sotto E50, ordinate dal rischio più basso)

| # | Fetta | Esce da | Misura di accettazione (prima/dopo, con comando) | Precondizione |
|---|---|---|---|---|
| 0 | `misure-strutturali.py` vede `RTTurnManager_Movement.cpp` | `tools/architettura/` | l'output elenca quattro file; le righe salgono di quanto pesa il file; nessun codice di produzione toccato | — |
| 1 | Oracolo dell'ordine di emissione applicato al corpus: il manifest replay (`OrderedHashPerTurn`, che esiste già) registrato per i 9 scenari golden e confrontato, con controllo positivo | `Tests/Golden/`, `RTGoldenCorpusTests.cpp`, riuso di `RecordTurnToReplay` | test rosso se si scambiano due voci dello stesso turno in memoria; verde su `main`; `grep -rli OrderedHash Source/RefactorTactics/Tests/Golden \| wc -l` > 0; nessun formato nuovo (`git grep -c OrderedHashPerTurn -- Source/RefactorTactics/Replay/RTReplayManifest.h` invariato) | — |
| 2 | `FRTUnitPlan` (campi `Planned*` fuori da `ARTUnit`, `StableUnitId` al posto del puntatore, getter invariati) e il setter unico; le 25 scritture dei resolver classificate prima (fallback dichiarato o stato di fase) | `Unit/RTUnit.h`, `Player/RTPlayerController.cpp`, `ScenarioHarness/RTScenarioSession.cpp`, `Turn/RTTurnManager*.cpp` | `grep -c UPROPERTY RTUnit.h` in discesa; assegnazioni dirette `->Planned* =` fuori dai test (comando di §2.3) da 23 + 19 + 18 + 7 a 0; golden invariato; 55 file di test aggiornati a tappe | — |
| 3 | Movement fuori: `ResolveMovement` e `FinishMovementResolution` (dove stanno i 6 siti di log) → `Turn/Resolution/RTMovementResolver.{h,cpp}` con `(FRTMovementResolutionContext&, emettitore&)`; l'emettitore nasce qui. **Fuori dalla fetta**: la macchina della finestra di reazione definita nello stesso file (19 definizioni, fra cui la ripresa del Blast), che resta nel TurnManager finché la fetta 10 non la reclama; `StepHexMovement` è già una funzione libera di `RTHexSimLibrary.cpp:1304` | `RTTurnManager_Movement.cpp`, `RTTurnManager.h` | definizioni `ARTTurnManager::` (comando di §2.2) da 172 a 170; `AddLogEvent\|AppendLogEntry` in `_Movement.cpp` a 0 e nel file nuovo a 6; `grep -c 'Units\['` nel file nuovo dichiarato (oggi 52 in `_Movement.cpp`: il contesto porta ancora `TWeakObjectPtr<ARTUnit>`); golden dell'insieme e `OrderedHashPerTurn` identici; `RTMovementResumeTests` verde | 0, 1 |
| 4 | Catalogo azioni per famiglia + `ValidateCatalog` + test d'ordine degli `ActionId` e dei kit + test `Meta` contro `docs/balance` | `Ability/RTCatalogLibrary.cpp` | `GetCoreActionCatalog` fuori dalle 12 più lunghe; test `Catalog.OrderIsFrozen` verde; divergenza markdown/C++ a 0 | — |
| 5 | `DA_Hero_*` generati e `PresentationClass` al posto di `FClassFinder` | `RTGameMode.cpp:593`, commandlet nuovo nel modulo Editor | `git grep -c FClassFinder -- Source/RefactorTactics/RTGameMode.cpp` a 0; `IsDataValid` su `URTHeroData`; conteggio asset = righe del catalogo | 4 |
| 6 | Inventario dei 105 ganci `ForTest` in (a)/(b)/(c); i (c) in `URTScenarioAuthoring` (compreso `PlanBotsForTest` usato dall'harness); i (b) sotto accessor `friend` in `Tests/`, una classe per fetta | header di produzione | `grep -c 'ForTest('` per header in discesa; `grep -c friend` ≤ 1; nessun gancio chiamato dalla produzione (`git grep` di §2.4 a 0) | — |
| 7 | PlayerController: test dei binding di Enhanced Input; camera inoltrata; sessione di planning fuori un gancio alla volta | `Player/RTPlayerController.cpp` | `grep -c 'ForTest('` da 36 in discesa; test senza `SpawnActor` sulle decisioni spostate; righe di `Player/` in calo | 2, 6 |
| 8 | Stato canonico dell'unità: il tipo che assorbe `MakeSimUnit`, un campo alla volta, con il test "mutato fra snapshot e risoluzione" | `RTTurnManager.h:1732`, `RTBlastContext.h` | `grep -c 'ARTUnit\*' RTBlastContext.h` da 27 in giù; `MakeSimUnit` fuori dal TM; golden invariato | 2 |
| 9 | Dash in due tempi: il contesto estratto dal corpo di `ResolveDash()`, poi la funzione fuori; stesso schema per `ResolvePrep()` | `RTTurnManager.cpp` | `ResolveDash(FRTDashContext&)` senza membri del TM nel corpo (elenco dei membri letti, a 0); metodi in discesa | 1, 3 |
| 10 | Blast per ultimo: `ResolveCombatPasses` (da `RTTurnManager.cpp:6113`) e `ApplyDisplacements`, `RunBlastReactions`, `ApplyControlStatuses` (da `_Blast.cpp`, i tre chiamanti di `RunReactionPass`) fuori, dopo che i 27 `ARTUnit*` del contesto sono scesi e con il criterio scritto per "ripresa preservata" (`bSuspended`, `ResumeBlastResolution` in `_Movement.cpp`) | `RTTurnManager.cpp`, `RTTurnManager_Blast.cpp`, `RTTurnManager_Movement.cpp` | come la fetta 3 più un test che caratterizza lo stato di `FRTBlastContext` attraverso `ResumeBlastResolution` (oggi nessuno lo fa, §2.4); gli enum di esito Combat sono già coperti 6 su 6, Reaction e Fallback da misurare con lo stesso comando | 1, 8, 9 |
| 11 | Cartelle di `Turn/` come esito: `Resolution/`, `Reaction/`, `Log/`, `Pacing/`, `Playback/`, `Format/`; `RTReplayRecording` in `Replay/` | `Turn/` | commit senza semantica, golden invariato, include dei test aggiornati nello stesso commit, churn dichiarato prima | 3, 9, 10 |
| 12 | Regola di direzione nella mappa del codice + test `Meta` (librerie senza `RTUnit.h`/`RTTurnManager.h`/`World.h`) + tabella "cartella → modulo" | `architettura-codice.md`, `Tests/` | il comando di §2.4 da 3 a 0; `Turn/` senza include verso `UI/`, `Player/`, `Camera/` | — |
| 13 | Modulo `RTSim` nel `.Build.cs` | `Source/` | solo quando la fetta 12 è a zero da tre fette consecutive; prezzo dichiarato: export `*_API` (161 file), #950 con 317 file di test nel runtime e 36 nell'Editor, 106 test che includono `RTTurnManager.h`; la fetta 12 parte da 2 include (`RTReactionConditionConsole.cpp:6`, e `RTPlayerState.h` in `RTTurnManager.cpp:19`, che è la porta di D-285 e va nominata come eccezione) | 12 |

Fuori da questa sequenza ma già aperte e coerenti: #2184 (`DrawHUD`, la fusione delle due pipeline HUD viene
prima di qualunque regola di naming per `WBP_RT_*`), #2182 (allestimento nei test), #2586 (gli array paralleli
di `FRTBlastContext` sono la stessa cosa della fetta 8 vista dal Blast), #1821 (sbloccata dalla fetta 12).

---

## 7. Decisioni minime richieste (`BLOCKED — DECISION REQUIRED` per le fette 3, 9, 10)

1. **Riaprire il perimetro di #1818 sui resolver**, con il prezzo scritto: "i resolver restano dentro" costa la
   crescita per gameplay (102 → 187 metodi in due mesi); "i resolver escono" costa l'applicazione al corpus
   golden dell'hash ordinato che il replay già persiste (fetta 1) e, per il Blast, un criterio di ripresa. Senza
   questa decisione le fette 3, 9, 10 non sono accettabili per #1818 a prescindere dalla loro qualità.
2. **Il criterio di #1818 per le fette sui resolver**: togliere la quota con mondo, o tenerla con la definizione
   di #2182 (spawn senza flusso). Disaccordo 4 di §4.10.
3. **Gate assoluto o relativo** sulla classe `ARTTurnManager`. Disaccordo 5 di §4.10.
4. **`DA_Hero_*` generati** (fetta 5): sì o no, e dove (`Content/RT/Data/Heroes/_Generated/`).
5. **Politica dei ganci `ForTest`**: `friend` solo per (b) con tetto per classe, e `URTScenarioAuthoring` per (c).

Tutto il resto (fette 0, 1, 2, 4, 6, 7, 8, 11, 12) sta dentro le decisioni esistenti e può partire con una
issue ciascuna sotto E50.

---

## 8. Ciò che questo referto NON prova

- **Nessun build e nessuna run della suite**: non è stata cambiata una riga di codice di produzione; misurare
  non è verificare. Compile `N/A` · Tests `N/A` · Determinism `N/A` · Replay `N/A` · Privacy `N/A` · PIE `N/A` ·
  Packaged `N/A`.
- Le misure valgono su `34b6c314f`; il clone principale era su un altro branch durante la sessione e non è stato
  usato per misurare.
- I numeri in righe di file (§2.1) contano anche il commento normativo; quelli dello strumento (§2.2) no. Non
  vanno confrontati fra loro.
- **Non verificato**: quali membri del TurnManager leggono i corpi di `ResolvePrep` (`RTTurnManager.cpp:4693`)
  e `ResolveDash` (`:4940`); quanti valori degli enum di esito Reaction e Fallback del TurnLog hanno un test
  (per Combat è 6 su 6, §2.4); quante delle 161 `REFACTORTACTICS_API` consuma il modulo Editor (decide il
  costo reale della fetta 13); se gli hit di stringa `GetPhase` nei `WBP_RT_*` siano chiamate al TurnManager o
  a un'altra classe; se esistano istanze di `URTActionData` in `Content/` (non scansionato); il 97,5 % citato
  da #2182 è preso dalla issue, non rimisurato.
- **Un rilievo del verificatore resta aperto per scelta**: le 25 scritture di `Planned*` dentro i resolver
  (§2.3) non sono state classificate in questo passaggio; la fetta 2 lo fa come primo passo.
- **Non dice quale fetta fare per prima fra le indipendenti** (0, 4, 6, 12): sono tutte a rischio basso; la
  scelta è dell'autore e sta nel Decision Log, non qui.

---

## Verdetto

Il mandato "classi da oltre 2000 righe" è vero e misurato (§2.1), ma la **causa** non è l'assenza di
decomposizione: il progetto ha già una simulazione in librerie pure, contesti per fase, un TurnLog con punto di
scrittura unico e un gate di non regressione. La causa è che **i resolver vivono nell'Actor** e ogni feature di
gameplay atterra lì per costruzione: 310 commit e +85 metodi in due mesi con il gate acceso.

Gli "standard del settore" che **si applicano** a questo progetto sono quattro: Model/Actor split (stato canonico
e piano come struct, Actor come presentazione); resolver come funzioni di un contesto con un emettitore, cioè
event sourcing con il TurnLog già esistente come store; dati come tabelle validate con una sorgente sola (C++ →
asset generato, D-229); direzione delle dipendenze misurata e poi compilata come modulo. Quelli che **non si
applicano** ora: componenti per lo stato, asset autorati per le azioni, MVVM di UE 5.x, vertical slice top-level,
modulo di test separato.

La condizione perché tutto questo sia una fetta e non una scommessa è una sola, e precede ogni altra:
**l'oracolo dell'ordine di emissione applicato al corpus** (fetta 1), perché il corpus golden confronta insiemi
e il riordino è esattamente il guasto che spostare un resolver produce; l'hash ordinato esiste già e il replay lo
persiste per turno, quindi la fetta è un riuso, non un formato nuovo. Con quello, e con lo strumento che vede
tutti i file del TurnManager (fetta 0), il perimetro di #1818 può riaprirsi: Movement prima (ma il suo file
contiene anche la macchina della finestra di reazione, che resta fuori dalla fetta), Blast per ultimo, una
decisione scritta con il prezzo accanto.
