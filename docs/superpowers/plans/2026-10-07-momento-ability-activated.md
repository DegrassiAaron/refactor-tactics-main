# Il momento — `AbilityActivated` — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni intento con un `ActionId` (Prep, Dash, Blast; esclusi `Action.Wait`, `Action.Move`, reazioni, fasi Cleanup/Environment) emette un `ERTResolvedEventType::AbilityActivated` nel punto in cui il resolver lo accetta; il playback lo riproduce come beat proprio — la clip del ruolo `Cast` sulla sorgente, una riga `Attiva:` nel feed — **prima** del colpo dello stesso intento, con il Blast svelato come una sola sequenza per intento, e solo se la sorgente è nella conoscenza della squadra che guarda.

**Architecture:** un helper privato `ARTTurnManager::EmitAbilityActivated` è l'unico costruttore dell'evento; lo chiamano i siti di accettazione di `ResolvePrep`/`ResolveCoverStructures`, il primo ciclo di `ResolveDash`, `ResolveCleanseActions`, `CollectHealActions`, il ramo `ModifyArc` di `CollectAttackIntents` e un pass nuovo `EmitAttackIntentActivations` in `ResolveCombatPasses`. `InterruptedIntents` sale in `FRTBlastContext`. L'evento congela il verdetto di [D-223] in un campo nuovo `SourceVerdict`. Il playback filtra con `SourceVerdict.AllowsTeam(Viewer)`, smista le attivazioni di Prep e Dash in due code proprie, e costruisce il Blast con una funzione pura `URTPlaybackLibrary::BuildBlastSequence` che sostituisce i tre canali paralleli; `PhaseTime`, `BlastPhaseIsActive` e `IsActBoundary` cambiano firma di conseguenza.

**Tech Stack:** Unreal Engine 5.8.1, C++ (modulo `RefactorTactics`), Automation Test framework, Scenario Harness (JSON), registro PIE in Markdown, sedute in YAML.

**Spec:** `docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md` — il piano argomenta dalla spec; chi esegue legge entrambi. Decisioni D1–D6 e `Ruling` sono chiusi: non si riaprono qui.

## Global Constraints

- Engine: **UE 5.8.1** in `D:/EpicGames/UE_5.8`. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **`AbilityActivated` in CODA** a `ERTResolvedEventType`, dopo `ArcHit` (`Turn/RTResolvedEvent.h:158`): è un `uint8` esposto a Blueprint.
- ⛔ **Nessun dato nuovo in snapshot, TurnLog o `StateHash`.** La timeline è playback (`Turn/RTTurnManager.cpp:2831-2833`, `Turn/RTTurnLog.h:827-829`). Misurato in pianificazione: `grep -n ResolvedTimeline Source/RefactorTactics/Turn/RTMatchStateHash.h` → **0** righe; il Task 3 lo rimisura sul proprio commit.
- **Ordinamento solo esplicito**: `SortUnitsForResolution`, `SortActionInstances`, `IntentIndex`, l'ordine dei pass, l'indice di timeline. Mai l'ordine di iterazione di un `TMap`/`TSet`/Actor (un `TSet` si usa solo con `Contains`/`Add`).
- ⛔ **Nessun `.uasset`**, nessun Blueprint toccato: `PlayCastMontage` nasce `BlueprintImplementableEvent` e nessun `BP_Unit_*` lo implementa.
- ⛔ **Nessun totale volatile** in commenti, documenti, celle del registro PIE, YAML, issue, PR (`AGENTS.md` §14): niente «N attivazioni», «N scenari», «N voci». I conteggi di un test restano nel test.
- Stile: commenti in italiano, nella forma dei file toccati (🔴 ⚠️ 🔑 ⛔ dove il file li usa). Nomi dei test nelle famiglie esistenti `RefactorTactics.<Famiglia>.<Nome>` (`Turn`, `Playback`, `Presentation`, `Unit`, `Match.Autobattle`). Helper dei test con nomi **distinti per file** (unity build).
- Commit: `<type>(<scope>): <descrizione>`, chiuso da `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`. Un commit per task.
- Branch: `issue/<n>-momento-ability-activated`, da `main` (Task 0).
- `<checkout>` = la cartella in cui sta il branch (clone principale `D:/Repositories/refactor-tactics-main` o un suo worktree). `<scratchpad>` = la cartella scratchpad della sessione.
- **Motore uno per macchina** (`CLAUDE.md` §10): prima di ogni build o run, `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`. Se un altro clone misura **tempi**, si aspetta; altrimenti si procede.
- Build (Editor chiuso su questo clone):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="<checkout>/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
  ```
- Test (filtro dopo `RunTests`, `+` fra più filtri, `;Quit` separato, `-abslog` **fra virgolette**):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<checkout>/RefactorTactics.uproject" "-ExecCmds=Automation RunTests <filtro>;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-abslog=<scratchpad>/<nome-parlante>.log"
  ```
  Un esito vale **solo** se il log porta `**** TEST COMPLETE`. Conteggio (Git Bash): `grep -c '\*\*\*\* TEST COMPLETE' <log>` (atteso 1), `grep -c 'Result={Success}' <log>`, `grep -c 'Result={Fail}' <log>` (atteso 0). Un rosso si legge con `grep -n 'Result={Fail}' -A3 <log>`.

## Review Focus

Cinque ingressi che la spec implica e che i test «di spec» (§5.1) non esercitano. Ciascuno ha ora il suo test, **scritto per intero** nello Step 1 del task che possiede il codice; chi rivede controlla che ci sia e che sia rosso prima del codice:

1. **Una copertura di Prep accettata contro una rifiutata** (`ResolveCoverStructures`, §2.2 terzo punto) → Task 2 Step 1, `Turn.CoverStructureActivatesOnlyWhenApplied`.
2. **La predittiva senza cella** (`Ruling` di §2.2, `RTTurnManager.cpp:4268`) → Task 2 Step 1, `Turn.PredictiveWithoutCellStillActivates`.
3. **L'estensione D-355 su un Blast sospeso davvero** (un `Brace` con `Profile.Sidestep` che apre la finestra, montaggio di `Tests/RTDefensiveReactionTests.cpp:985-1047`) → Task 7 Step 1, `Playback.ActivationPrefixSurvivesASuspendedBlast`.
4. **`Step` e corsa durante le attivazioni del Dash** (l'orologio della rotta spostato, Task 7 Step 5; la corsa sul posto di #3519) → Task 7 Step 1, `Playback.DashStepLandsOnCellsAfterTheActivations`.
5. **Una sorgente nemica davvero fuori vista** (dietro un muro alto, non «una squadra senza unità») → Task 7 Step 1, `Playback.EnemyBehindHighCoverHasNoActivationBeat`.

Chiusi anche due buchi segnalati in review: `Next Action` sulle attivazioni di Prep e Dash (Task 7, `Playback.NextActionStopsOnPrepAndDashActivations`) e il cancello del Dash aperto dalle sole attivazioni (Task 7, `Playback.DashPhaseOpensForActivationsOnly`).

---

### Task 0: Issue e branch

**Files:** nessuno nel repository. GitHub: una issue nuova sotto l'epic di presentazione [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453).

**Interfaces:**
- Produces: il numero `<n>`, usato da branch e commit.

- [ ] **Step 1: Apri la issue** (azione verso l'esterno: chiedere conferma all'autore se non già data)

Corpo in `<scratchpad>/issue-momento.md`, poi `--body-file` (mai `--body` inline: i backtick vengono eseguiti).

```markdown
> **Epic**: #2453 · **Refs**: #3532 (sotto-progetto 1) · #2454 · #3292 · #3293 · **Spec**: `docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md`

## Why

Un'abilità senza colpo non ha un istante: la timeline di playback nasce da colpi, impronte, stati, reazioni, strutture e movimenti, e Heal, Cleanse, ModifyArc, Interrupt e ogni istanza di Prep non producono alcun evento. Il ruolo di presentazione `Cast` esiste e nessuno lo suona. Secondo dei quattro sotto-progetti di «associare animazioni e FX alle skill e vederle in azione».

## Scope

1. `ERTResolvedEventType::AbilityActivated` (in coda) con `SourceVerdict` congelato; voce D-278 `Cues { PlayCastMontage }`; `ARTUnit::PlayCastMontage` e `case Cast`; ruolo `Cast` nel roster.
2. Emissione in Prep (coperture, predittiva, Overwatch, istanze), Dash (prima del `Move`), Blast (Cleanse, Heal, ModifyArc, intenti d'attacco); `InterruptedIntents` in `FRTBlastContext`.
3. Playback: code di Prep e Dash, sequenza per intento del Blast (`BuildBlastSequence`), `PhaseTime`/`BlastPhaseIsActive`/`IsActBoundary` nuove firme, filtro di privacy, D-355.
4. Voce PIE `PIE-CAST-BEAT`, seduta, scenario `Visual.Ability.CastBeat`, riga in `conoscenza-parziale-visibile-spec.md` §1.3.

## Out of scope

Segnale visivo di attivazione (#2454), clip per abilità (sotto-progetto 3), profili FX (sotto-progetto 4), reazioni/stati/hazard, `ArcHit` (#3293), riordino per unità fra le sorgenti del Blast.

## DoD

- [ ] I test `Turn.*Activat*`, `Playback.*` e `Presentation.*` della spec §5.1 verdi sul commit reale, log con `**** TEST COMPLETE`.
- [ ] Le mutazioni (1)–(5) di §5.1, e quelle che i task aggiungono, eseguite: ciascuna fa cadere il test dichiarato.
- [ ] `Match.Autobattle.DeterminismIsIndependentOfPlayback` verde; nessun dato nuovo in snapshot/TurnLog/hash.
- [ ] Build `RefactorTacticsEditor` verde.
- [ ] Voce `PIE-CAST-BEAT` (⏳) e seduta nel file delle sedute; il verdetto a schermo è dell'autore.
- [ ] Nessun `.uasset`; nessun totale volatile in ciò che si pubblica.
```

```powershell
gh issue create --repo DegrassiAaron/refactor-tactics-main --title "Il momento: AbilityActivated, un beat di cast per ogni intento di abilita'" --body-file "<scratchpad>/issue-momento.md" --label enhancement
```

Rileggi dal server e conta i backtick: `gh issue view <n> --json body -q .body | grep -o '`' | wc -l` deve coincidere con `grep -o '`' <scratchpad>/issue-momento.md | wc -l`.

- [ ] **Step 2: Branch da `main`, e la spec nel repository**

```bash
git fetch -q origin && git switch -c issue/<n>-momento-ability-activated origin/main
git config branch.issue/<n>-momento-ability-activated.parent main
```

Copia la spec rivista nel percorso `docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md` e questo piano in `docs/superpowers/plans/2026-10-07-momento-ability-activated.md`, poi:

```bash
git add docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md docs/superpowers/plans/2026-10-07-momento-ability-activated.md
git commit -F - <<'EOF'
docs(<n>): spec e piano del momento AbilityActivated

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 1: L'evento, la voce D-278, `PlayCastMontage` e il ruolo `Cast` nel roster

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTResolvedEvent.h:158` (in coda all'enum) e `:334` (docstring delle forme)
- Modify: `Source/RefactorTactics/Turn/RTPresentationBinding.cpp:249-257` (voce nuova dopo `ArcHit`, prima di `return Out;`)
- Modify: `Source/RefactorTactics/Unit/RTUnit.h:1315-1325` (evento e seam accanto ai tre esistenti)
- Modify: `Source/RefactorTactics/Unit/RTUnit.cpp:766-775` (`switch` di `PlayPresentationRole`)
- Modify: `Source/RefactorTactics/Unit/RTPresentationRole.h:9-13` (commento)
- Modify: `Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp:40-62` (commento-trappola e `MakeClips`)
- Test: `Source/RefactorTactics/Tests/RTPresentationBindingTests.cpp:56-70` e `:503-522`
- Test: `Source/RefactorTactics/Tests/RTAnimChannelTests.cpp` (in coda, prima di `#endif`)
- Test: `Source/RefactorTactics/Tests/RTUnitTests.cpp:596-617` (`Unit.DiscreteRoleClipsMatchThePacks`: l'asserto «il ruolo `Cast` resta VUOTO» scade con questo task)

**Interfaces:**
- Produces:
  - `ERTResolvedEventType::AbilityActivated` (ultimo valore).
  - `UFUNCTION(BlueprintImplementableEvent) void ARTUnit::PlayCastMontage(UAnimSequenceBase* Resolved);`
  - `int32 ARTUnit::CastCuesPlayedForTest() const;` — quante volte `PlayPresentationRole(Cast)` è stata chiamata su questa unità; incrementato solo sotto `WITH_DEV_AUTOMATION_TESTS`.
  - `URTUnitAnimInstance` CDO: `ActiveClipFor(<eroe del roster>, ERTPresentationRole::Cast)` non nullo e uguale a quello del ruolo `Attack`.

- [ ] **Step 1: I test rossi**

In `RTPresentationBindingTests.cpp`, `EnumSizeIsPinned` (`:56-70`): aggiungi la riga di storia dopo `8 -> 9` e porta l'atteso a 10:

```cpp
	//   8 -> 9   2026-09-24   `ArcHit`            (#3280, [D-437])
	//   9 -> 10  2026-10-07   `AbilityActivated`  (#<n>, il momento: entra CON cue, `PlayCastMontage`)
```

```cpp
	TestEqual(TEXT("ERTResolvedEventType dichiara dieci valori: l'ultimo aggiunto ha una voce nella tabella?"),
		URTPresentationBindingLibrary::DeclaredEventTypeCount(), 10);
```

In `AbsenceCensusIsPinned`, dopo l'asserto di `StructureHit` (`:515-516`):

```cpp
	// ✅ `AbilityActivated` nasce con cue (`PlayCastMontage`, #<n>): NON e' in attesa, quindi non ha un
	// `PendingOwner`. Il conteggio delle attese qui sopra resta quello di prima, ed e' la prova che la voce
	// non e' stata dichiarata in attesa per comodita'.
	TestEqual(TEXT("AbilityActivated non attende nessuno: nasce con cue"),
		OwnerDi(ERTResolvedEventType::AbilityActivated), FString());
```

In coda a `RTAnimChannelTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
/**
 * Il ruolo `Cast` risolve una clip per ogni eroe del roster — spec «il momento» §2.3.
 *
 * 🔑 **Il PATH, senza caricare**: i pack Paragon non sono versionati, e headless `LoadSynchronous` darebbe
 * `nullptr` su ogni clone appena creato. E' la stessa ragione di `ResolvedAnimationReachesTheUnit`.
 *
 * ⚠️ **In v0.1 la clip e' la STESSA del ruolo `Attack`**, ed e' una decisione (D2): cast e colpo si
 * distinguono per MOMENTO, non per forma. La seconda asserzione lo pinna, cosi' che chi la cambia lo faccia
 * nel catalogo ANIM CORE e non per sbaglio qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitCastRoleResolvesAClipForEveryHeroTest,
	"RefactorTactics.Unit.CastRoleResolvesAClipForEveryHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitCastRoleResolvesAClipForEveryHeroTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO di URTUnitAnimInstance"), Cdo)) { return false; }

	static const TCHAR* Eroi[] = { TEXT("Hero.Aevik"), TEXT("Hero.Muiren"), TEXT("Hero.Branth"), TEXT("Hero.Ivrin") };
	for (const TCHAR* Eroe : Eroi)
	{
		const TSoftObjectPtr<UAnimSequenceBase> Cast = Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Cast);
		const TSoftObjectPtr<UAnimSequenceBase> Attacco = Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Attack);
		TestFalse(*FString::Printf(TEXT("%s: il ruolo Cast ha una clip attiva"), Eroe), Cast.IsNull());
		TestEqual(*FString::Printf(TEXT("%s: in v0.1 e' la stessa del ruolo Attack"), Eroe),
			Cast.ToSoftObjectPath().ToString(), Attacco.ToSoftObjectPath().ToString());
	}
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build.
Expected: **il rosso di questo passo è di COMPILAZIONE**: l'asserto aggiunto ad `AbsenceCensusIsPinned` nomina `ERTResolvedEventType::AbilityActivated`, che non esiste ancora (`error C2039` o equivalente su `AbilityActivated`). Il rosso «a runtime» di `EnumSizeIsPinned` (9 contro 10) e di `CastRoleResolvesAClipForEveryHero` (Cast nullo) si vede solo dopo lo Step 3; se vuoi vederlo, esegui lo Step 3 da solo, build, e lancia `RefactorTactics.Presentation+RefactorTactics.Unit.CastRoleResolvesAClipForEveryHero`: atteso `Result={Fail}` su entrambi, e `EveryEventTypeIsCovered` rosso con `mancanti: AbilityActivated`.

- [ ] **Step 3: Il valore d'enum**

`RTResolvedEvent.h`, dopo `ArcHit` (`:158`), sostituendo `ArcHit` con `ArcHit,`:

```cpp
	ArcHit,

	/**
	 * Un intento di abilita' e' stato ACCETTATO dal resolver: «questa unita' sta agendo adesso con questa
	 * azione» (spec «il momento», #<n>). Una voce per INTENTO, in Prep, Dash e Blast.
	 *
	 * 🔴 **Un'abilita' senza colpo non aveva un istante.** Cure, purificazioni, archi, interruzioni e ogni
	 * istanza di Prep non producevano alcun evento: il produttore c'era, mancava il momento — la forma di
	 * #2505 e #2828.
	 *
	 * ⛔ **Niente celle colpite, niente esiti**: quelli restano di `AttackFootprint` e `Attack`. Questo evento
	 * racconta il GESTO, e si emette anche quando il gesto non tocca nulla (linea di tiro bloccata, fuori
	 * portata, degradato da [D-300]).
	 *
	 * 🔑 `SourceVerdict` porta il verdetto di [D-223] congelato all'emissione: il playback lo filtra con
	 * `AllowsTeam`, come le righe di log, non con `ObservedPrefixLength` come il `Move` (spec §2.5).
	 *
	 * ⚠️ **In CODA, come i valori sopra e per la stessa ragione**: e' un `uint8` esposto a Blueprint.
	 */
	AbilityActivated
```

E la docstring di `:334`:

```cpp
	// --- `AttackFootprint` ([D-301]); `AimCell` e `Shape` anche per `AbilityActivated` (#<n>). Vuoti/di default
	//     per ogni altro `Type`. ---
```

- [ ] **Step 4: La voce D-278**

`RTPresentationBinding.cpp`, dopo la voce di `ArcHit` (`:249-256`), prima di `return Out;`:

```cpp
	// AbilityActivated — il momento del cast (#<n>). **Con cue**, non in attesa.
	//
	// 🔑 **`PlayCastMontage` e' l'evento Blueprint che `ARTUnit::PlayPresentationRole(Cast)` notifica**, con la
	// clip gia' risolta dal CDO: la stessa forma di `PlayAttackMontage`. Il segnale visivo (ring, pulse) resta
	// di #2454 e non e' dichiarato qui.
	//
	// ⚠️ **Il gate `FindMissingBindings` verifica solo che la cue non sia `NAME_None`**: che venga CHIAMATA lo
	// prova `Playback.ActivationPlaysTheCastCue`, che conta le chiamate sulla sorgente.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::AbilityActivated,
		{ FName(TEXT("PlayCastMontage")) }));
```

- [ ] **Step 5: `PlayCastMontage`, il seam e il `case Cast`**

`RTUnit.h`, dopo `PlayDefeatMontage` (`:1323-1325`):

```cpp
	/** La sorgente esegue il cast di un'abilita' (`AbilityActivated`, #<n>), con la clip gia' risolta. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayCastMontage(UAnimSequenceBase* Resolved);

	/**
	 * Quante volte `PlayPresentationRole(Cast)` e' stata chiamata su questa unita' — seam di misura (#<n>).
	 *
	 * ⚠️ **Il contatore cresce solo sotto `WITH_DEV_AUTOMATION_TESTS`** (in `RTUnit.cpp`): fuori dai build di
	 * test resta `0`. Il campo non e' una `UPROPERTY`, come gli accessori `*ForTest` di `ARTTurnManager`, e
	 * permette di asserire il MECCANISMO — la cue chiamata — senza un Blueprint.
	 */
	int32 CastCuesPlayedForTest() const { return CastCuesPlayed; }

private:
	int32 CastCuesPlayed = 0;

public:
```

(Se la sezione che segue in `RTUnit.h` non è già `public:`, lascia il `public:` finale come sopra; se lo è, è innocuo.)

`RTUnit.cpp`, lo `switch` di `:766-775` diventa:

```cpp
	switch (Ruolo)
	{
	case ERTPresentationRole::Attack: PlayAttackMontage(Sequenza); break;
	case ERTPresentationRole::Hit:    PlayHitMontage(Sequenza);    break;
	case ERTPresentationRole::Death:  PlayDefeatMontage(Sequenza); break;
	case ERTPresentationRole::Cast:
#if WITH_DEV_AUTOMATION_TESTS
		++CastCuesPlayed; // seam di misura: la cue e' stata CHIAMATA, che il BP la implementi o no
#endif
		PlayCastMontage(Sequenza);
		break;
	default:
		// `Idle` e `Move` li suona il grafo; `Dash`, `Defend` e `Fall` non hanno ancora un consumatore.
		// ⏱️ *Fino a #<n> anche `Cast` stava qui: il ruolo esisteva e nessuno lo suonava.*
		break;
	}
```

- [ ] **Step 6: I commenti che dicevano «Cast non suona»**

`RTPresentationRole.h:9-13`, la frase su `Cast` diventa:

```cpp
 * `PlayHitMontage`, `PlayDefeatMontage`) — vedi #2448 — e `Cast` passa da `PlayCastMontage` dal 2026-10-07
 * (`AbilityActivated`, #<n>); `Dash`, `Defend` e `Fall` non hanno ancora nessun consumatore.
```

- [ ] **Step 7: Il ruolo `Cast` nel roster**

`RTUnitAnimInstance.cpp:40-62`, il commento e `MakeClips`:

```cpp
	/**
	 * I sei ruoli di un eroe del roster, ciascuno con la sua clip attiva.
	 *
	 * 🔴 **La clip dei pack che si CHIAMA `Cast` riempie DUE ruoli, e non e' un errore** (#2450, #<n>). Sul
	 * ruolo `Attack` e' il colpo; sul ruolo `Cast` e' il gesto di attivazione di `AbilityActivated`. In v0.1
	 * cast e colpo suonano la stessa sequenza in DUE MOMENTI diversi (spec «il momento» D2); una clip d'attacco
	 * diversa e' un giudizio umano nel catalogo ANIM CORE, non un ritocco qui.
	 *
	 * ⚠️ I nomi si MISURANO: §AS.3b li ha letti sul disco, e **quattro caselle su dodici** fra i tre
	 * ruoli discreti non si chiamano come ci si aspetta.
	 */
	FRTHeroPresentationClips MakeClips(const TCHAR* Pack, const TCHAR* Idle, const TCHAR* Move,
		const TCHAR* Attack, const TCHAR* Hit, const TCHAR* Death)
	{
		FRTHeroPresentationClips Clips;
		Clips.PerRole.Add(ERTPresentationRole::Idle, MakeRuolo(Pack, Idle));
		Clips.PerRole.Add(ERTPresentationRole::Move, MakeRuolo(Pack, Move));
		Clips.PerRole.Add(ERTPresentationRole::Attack, MakeRuolo(Pack, Attack));
		// La STESSA clip del ruolo `Attack`: vedi il commento qui sopra.
		Clips.PerRole.Add(ERTPresentationRole::Cast, MakeRuolo(Pack, Attack));
		Clips.PerRole.Add(ERTPresentationRole::Hit, MakeRuolo(Pack, Hit));
		Clips.PerRole.Add(ERTPresentationRole::Death, MakeRuolo(Pack, Death));
		return Clips;
	}
```

- [ ] **Step 7b: L'asserto che il roster rende falso, riscritto e non ereditato**

`RTUnitTests.cpp:597-617` lo chiede da sé: *«il giorno in cui `Cast` acquista un consumatore, questa riga va RIVISTA, non ereditata»*. Sostituisci il blocco da `// ⚠️ **\`Cast\` e' vuoto per DUE ragioni` fino all'asserto `TestTrue(... il ruolo Cast resta VUOTO ...)` compreso con:

```cpp
		// ⚠️ **Fino a #<n> `Cast` era vuoto per DUE ragioni, e solo la prima era permanente** (#2535):
		//
		//  1. la clip che si chiama `Cast` appartiene ad `Attack` — **permanente**, ed e' cio' che questo
		//     asserto difende ancora: l'asserto per `Attack` qui sopra cade se la clip finisce SOLO sul ruolo
		//     `Cast`;
		//  2. ⏱️ *il ruolo `Cast` non aveva un consumatore* — **scaduta il 2026-10-07**: `AbilityActivated`
		//     (#<n>) lo suona via `PlayCastMontage`, e il roster lo popola con la STESSA clip di `Attack`
		//     (spec «il momento» D2: cast e colpo si distinguono per momento, non per forma).
		//
		// 🔑 ∴ l'invariante non e' piu' «`Cast` vuoto» ma «`Cast` uguale ad `Attack`». Una clip diversa sul
		// ruolo `Cast` e' un giudizio umano del catalogo ANIM CORE, e chi la fa cambia anche questa riga.
		TestEqual(*FString::Printf(TEXT("%s: in v0.1 il ruolo Cast suona la stessa clip di Attack"), *Chi),
			Cdo->ActiveClipFor(Chiave, ERTPresentationRole::Cast).ToSoftObjectPath().ToString(),
			Cdo->ActiveClipFor(Chiave, ERTPresentationRole::Attack).ToSoftObjectPath().ToString());
		TestFalse(*FString::Printf(TEXT("%s: e non e' vuoto"), *Chi),
			Cdo->ActiveClipFor(Chiave, ERTPresentationRole::Cast).IsNull());
```

- [ ] **Step 8: Build e test verdi**

Run: build; test `RefactorTactics.Presentation+RefactorTactics.Unit+RefactorTactics.Anim`.
Expected: build verde; `Result={Fail}` = 0; `**** TEST COMPLETE`.

- [ ] **Step 9: Mutazione della voce D-278**

Commenta le due righe `Out.Add(... AbilityActivated ...)` dello Step 4, build, test `RefactorTactics.Presentation.EveryEventTypeIsCovered`.
Expected: `Result={Fail}` con `mancanti:` che nomina `AbilityActivated`. Ripristina le righe, build, rilancia: verde.

- [ ] **Step 10: Commit**

```bash
git add Source/RefactorTactics/Turn/RTResolvedEvent.h Source/RefactorTactics/Turn/RTPresentationBinding.cpp Source/RefactorTactics/Unit/RTUnit.h Source/RefactorTactics/Unit/RTUnit.cpp Source/RefactorTactics/Unit/RTPresentationRole.h Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp Source/RefactorTactics/Tests/RTPresentationBindingTests.cpp Source/RefactorTactics/Tests/RTAnimChannelTests.cpp Source/RefactorTactics/Tests/RTUnitTests.cpp
git commit -F - <<'EOF'
feat(<n>): AbilityActivated in coda all'enum, voce D-278 con PlayCastMontage, ruolo Cast suonato e popolato nel roster

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 2: L'emissione in Prep e in Dash

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (dichiarazione privata accanto a `FreezeVerdictFor`, `:2439`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:3869-3896` (le due struct di `ResolveCoverStructures`), `:3993`, `:4025`, `:4077-4080`, `:4185`, `:4285-4290`, `:4342-4349`, `:4418-4425`, `:4826-4876`; e la definizione dell'helper dopo `FreezeVerdictFor` (`:312`)
- Test: `Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp` (nuovo)

**Interfaces:**
- Consumes: `ERTResolvedEventType::AbilityActivated` (Task 1).
- Produces:
  - `void ARTTurnManager::EmitAbilityActivated(ARTUnit* Source, ERTMatchPhase InPhase, FName ActionId, FName BaseActionId, int32 TargetStableUnitId, const FRTCellId& AimCell, ERTAbilityShape Shape);` (privata) — l'unico costruttore dell'evento; con `ActionId` `NAME_None` non emette e scrive un `ensureMsgf`.
  - Timeline: un `AbilityActivated` di fase `Prep` per ogni copertura applicata, predittiva spesa, Overwatch armato, istanza consumata; di fase `Dash` per ogni `DashAbilityIdx[i] != INDEX_NONE`, **prima** del `Move` dello stesso scatto.

- [ ] **Step 1: I test rossi**

Crea `Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp`:

```cpp
// IL MOMENTO: `AbilityActivated` (#<n>, spec «il momento»).
//
// 🔑 **Cosa misura questo file**: che il resolver emetta UNA attivazione per intento, nel punto in cui lo
// accetta, nell'ordine che gia' ha. ⛔ **Non misura il playback** — quello e' `RTPlaybackActivationTests.cpp`
// — e non tocca golden, hash o archivi: la timeline non entra ne' in `StateHash` ne' nel TurnLog.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	URTHexMapAsset* SpawnAttivazioneMap(UWorld* World, int32 Radius = 8)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnAttivazioneUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** L'indice nel kit dell'abilita' con questo `ActionId`, letto dal CATALOGO dell'unita'. */
	int32 IndiceAbilitaAttivazione(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}

	/** Gli indici di timeline delle attivazioni, in ordine di timeline. */
	TArray<int32> IndiciAttivazioni(const ARTTurnManager* TM)
	{
		TArray<int32> Out;
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i = 0; i < T.Num(); ++i)
		{
			if (T[i].Type == ERTResolvedEventType::AbilityActivated) { Out.Add(i); }
		}
		return Out;
	}

	/** Il primo indice di timeline che soddisfa il predicato, `INDEX_NONE` se nessuno. */
	template <typename TPred>
	int32 PrimoIndiceAttivazione(const ARTTurnManager* TM, TPred Pred)
	{
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i = 0; i < T.Num(); ++i)
		{
			if (Pred(T[i])) { return i; }
		}
		return INDEX_NONE;
	}
}

/**
 * La Prep attiva ogni tipo di intento che accetta, nei suoi ordini — spec §2.2.
 *
 * 🔑 **Due ordini nella stessa fase**: l'Overwatch esce dal ciclo di raccolta, le istanze dal ciclo di
 * consumo dopo `SortActionInstances`. Il primo precede il secondo, ed e' l'ordine del resolver.
 * 🔴 **E lo stordimento non si attiva**: l'intento e' rifiutato, non accettato. Il controllo positivo e' lo
 * stesso turno senza stordimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnPrepActivatesEveryKindOfIntentTest,
	"RefactorTactics.Turn.PrepActivatesEveryKindOfIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnPrepActivatesEveryKindOfIntentTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bStordito, int32& OutScudi, int32& OutOverwatch, int32& OutStordito,
		int32& OutIndiceOverwatch, int32& OutIndiceScudo) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Scudo = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		ARTUnit* Sentinella = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		ARTUnit* Stordito = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Scudo || !Sentinella || !Stordito) { return false; }

		const int32 TideGuard = IndiceAbilitaAttivazione(Scudo, TEXT("Hero.Muiren.TideGuard"));
		if (!TestTrue(TEXT("premessa: Muiren ha TideGuard"), TideGuard != INDEX_NONE)) { return false; }
		Scudo->PlannedAbilityIndex = TideGuard;
		Scudo->PlannedCell = Scudo->Cell;

		Sentinella->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Sentinella, TEXT("Action.Overwatch"));
		Sentinella->PlannedCell = Sentinella->Cell;

		Stordito->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Stordito, TEXT("Action.Shield"));
		Stordito->PlannedCell = Stordito->Cell;
		if (bStordito) { Stordito->ApplyStatus(TAG_Status_Stunned, 2); }

		TM->LockInAndResolve();

		OutScudi = OutOverwatch = OutStordito = 0;
		OutIndiceOverwatch = OutIndiceScudo = INDEX_NONE;
		const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
		for (int32 i : IndiciAttivazioni(TM))
		{
			const FRTResolvedEvent& Ev = T[i];
			TestEqual(TEXT("ogni attivazione di questo turno e' di fase Prep"),
				static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Prep));
			if (Ev.SourceStableUnitId == Scudo->StableUnitId) { ++OutScudi; OutIndiceScudo = i; }
			if (Ev.SourceStableUnitId == Sentinella->StableUnitId) { ++OutOverwatch; OutIndiceOverwatch = i; }
			if (Ev.SourceStableUnitId == Stordito->StableUnitId) { ++OutStordito; }
			if (Ev.SourceStableUnitId == Scudo->StableUnitId)
			{
				TestEqual(TEXT("in Prep il bersaglio e' chi usa l'azione"), Ev.TargetStableUnitId, Scudo->StableUnitId);
				TestEqual(TEXT("e l'azione e' quella del catalogo"), Ev.ActionId, FName(TEXT("Hero.Muiren.TideGuard")));
			}
		}
		return true;
	};

	int32 Scudi = 0, Overwatch = 0, Stordito = 0, IdxOw = INDEX_NONE, IdxScudo = INDEX_NONE;
	if (!GiraIlTurno(/*bStordito=*/ true, Scudi, Overwatch, Stordito, IdxOw, IdxScudo)) { return false; }
	TestEqual(TEXT("un'istanza di Prep: una attivazione"), Scudi, 1);
	TestEqual(TEXT("un Overwatch armato: una attivazione"), Overwatch, 1);
	TestEqual(TEXT("🔴 uno stordito non si attiva: l'intento e' rifiutato"), Stordito, 0);
	TestTrue(TEXT("l'Overwatch (ciclo di raccolta) precede l'istanza (ciclo di consumo)"), IdxOw < IdxScudo);

	// ⛔ Controllo positivo: senza stordimento la stessa unita' SI attiva. Senza, «zero» sopra sarebbe vero
	// anche per un'emissione che ignora `Action.Shield`.
	if (!GiraIlTurno(/*bStordito=*/ false, Scudi, Overwatch, Stordito, IdxOw, IdxScudo)) { return false; }
	TestEqual(TEXT("✅ senza stordimento la stessa unita' si attiva"), Stordito, 1);
	return true;
}

/**
 * Nel Dash l'attivazione PRECEDE il `Move` dello stesso scatto — D4, spec §2.2.
 *
 * 🔑 **E una carica che non entra in nessuna cella si attiva lo stesso**: il primo ciclo di `ResolveDash`
 * emetteva solo con `Resolved[i].Entered.Num() > 0`, e l'attivazione non deve dipendere dallo spostamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnDashActivationPrecedesItsMoveTest,
	"RefactorTactics.Turn.DashActivationPrecedesItsMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnDashActivationPrecedesItsMoveTest::RunTest(const FString&)
{
	// --- 1. Uno scatto che si muove: la disposizione di `Visual.Core.PhaseOrder` -----------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
		ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bersaglio || !Caricatore) { return false; }

		const int32 Ram = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
		if (!TestTrue(TEXT("premessa: Branth ha Ram"), Ram != INDEX_NONE)) { return false; }
		Caricatore->PlannedDashAbility = Ram;
		Caricatore->PlannedDashCell = Bersaglio->Cell;
		Caricatore->PlannedCell = Caricatore->Cell;

		TM->LockInAndResolve();

		const int32 Sid = Caricatore->StableUnitId;
		const int32 IdxAttivazione = PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::AbilityActivated && E.Phase == ERTMatchPhase::Dash
				&& E.SourceStableUnitId == Sid;
		});
		const int32 IdxMove = PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid;
		});
		if (!TestTrue(TEXT("⛔ premessa: lo scatto si e' mosso"), IdxMove != INDEX_NONE)) { return false; }
		if (!TestTrue(TEXT("lo scatto si e' attivato"), IdxAttivazione != INDEX_NONE)) { return false; }
		TestTrue(TEXT("🔴 l'attivazione PRECEDE il Move dello stesso scatto"), IdxAttivazione < IdxMove);
		TestEqual(TEXT("con l'azione dello scatto"),
			TM->ResolvedTimelineForTest()[IdxAttivazione].ActionId, FName(TEXT("Hero.Branth.Ram")));
	}

	// --- 2. Una carica che non entra in nessuna cella: il bersaglio e' gia' adiacente ------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
		ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bersaglio || !Caricatore) { return false; }

		Caricatore->PlannedDashAbility = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
		Caricatore->PlannedDashCell = Bersaglio->Cell;
		Caricatore->PlannedCell = Caricatore->Cell;

		TM->LockInAndResolve();

		const int32 Sid = Caricatore->StableUnitId;
		TestEqual(TEXT("⛔ premessa: nessun Move di fase Dash per chi era gia' adiacente"),
			PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
			{ return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid; }),
			static_cast<int32>(INDEX_NONE));
		TestTrue(TEXT("🔴 e l'attivazione c'e' lo stesso"),
			PrimoIndiceAttivazione(TM, [Sid](const FRTResolvedEvent& E)
			{ return E.Type == ERTResolvedEventType::AbilityActivated && E.Phase == ERTMatchPhase::Dash && E.SourceStableUnitId == Sid; })
			!= INDEX_NONE);
	}
	return true;
}

/**
 * Una copertura di Prep si attiva solo se APPLICATA — `Ruling` di spec §2.2 (Review Focus 1).
 *
 * 🔑 Stessa unita', stesso piano, due esiti: con il bordo dichiarato `ResolveCoverStructures` la applica
 * (`CoverCreated`), senza bordo la rifiuta con `Reject` (`CoverRejected`, «nessun bordo dichiarato»). Un
 * rifiuto non e' un gesto accettato: zero attivazioni. Le premesse leggono il TurnLog, cosi' il «zero» non e'
 * vero per un piano che non e' mai arrivato alla funzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnCoverStructureActivatesOnlyWhenAppliedTest,
	"RefactorTactics.Turn.CoverStructureActivatesOnlyWhenApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnCoverStructureActivatesOnlyWhenAppliedTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConBordo, int32& OutAttivazioni, bool& OutCreata, bool& OutRifiutata) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Branth = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth) { return false; }

		const int32 Pannello = IndiceAbilitaAttivazione(Branth, TEXT("Hero.Branth.KineticPanel"));
		if (!TestTrue(TEXT("premessa: Branth ha KineticPanel"), Pannello != INDEX_NONE)) { return false; }
		Branth->PlannedAbilityIndex = Pannello;
		Branth->bAttackTargetsCell = true;
		Branth->PlannedAttackCell = FRTCellId(1, 0); // a portata: la portata del pannello e' 3 (#2283)
		Branth->bHasPlannedCoverEdge = bConBordo;
		Branth->PlannedCoverEdge = ERTHexDirection::E;
		Branth->PlannedCell = Branth->Cell;

		TM->LockInAndResolve();

		OutAttivazioni = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Branth->StableUnitId)
			{
				++OutAttivazioni;
				TestEqual(TEXT("di fase Prep"), static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Prep));
				TestEqual(TEXT("con l'azione del pannello"), Ev.ActionId, FName(TEXT("Hero.Branth.KineticPanel")));
			}
		}
		auto HaEsito = [TM](ERTEnvironmentOutcome Esito)
		{
			return TM->GetTurnLog().ContainsByPredicate([Esito](const FRTTurnLogEntry& E)
				{ return E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Esito); });
		};
		OutCreata = HaEsito(ERTEnvironmentOutcome::CoverCreated);
		OutRifiutata = HaEsito(ERTEnvironmentOutcome::CoverRejected);
		return true;
	};

	int32 Attivazioni = 0;
	bool bCreata = false, bRifiutata = false;
	if (!GiraIlTurno(/*bConBordo=*/ true, Attivazioni, bCreata, bRifiutata)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: con il bordo la copertura e' stata eretta"), bCreata)) { return false; }
	TestEqual(TEXT("🔴 applicata: una attivazione"), Attivazioni, 1);

	if (!GiraIlTurno(/*bConBordo=*/ false, Attivazioni, bCreata, bRifiutata)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: senza bordo il pannello e' stato rifiutato"), bRifiutata)) { return false; }
	TestEqual(TEXT("🔴 rifiutata: nessuna attivazione"), Attivazioni, 0);
	return true;
}

/**
 * La predittiva SENZA cella si attiva — `Ruling` di spec §2.2 (Review Focus 2): l'unita' ha speso l'azione,
 * anche se non arma niente (`RTTurnManager.cpp:4268`). La cella mirata e' allora quella di chi agisce.
 * ⛔ Controllo positivo: con la cella dichiarata si attiva anch'essa, e porta QUELLA cella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnPredictiveWithoutCellActivatesTest,
	"RefactorTactics.Turn.PredictiveWithoutCellStillActivates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnPredictiveWithoutCellActivatesTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConCella, TArray<FRTResolvedEvent>& OutAttivazioni, FRTCellId& OutCellaIvrin) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Ivrin = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin) { return false; }

		// `Hero.Ivrin.InterceptShot` e' la Predictive Action del roster (`PredictiveTargeting = LockCell`,
		// `Ability/RTHeroCatalogLibrary.cpp:950`).
		const int32 Intercetto = IndiceAbilitaAttivazione(Ivrin, TEXT("Hero.Ivrin.InterceptShot"));
		if (!TestTrue(TEXT("premessa: Ivrin ha InterceptShot"), Intercetto != INDEX_NONE)) { return false; }
		Ivrin->PlannedAbilityIndex = Intercetto;
		Ivrin->bAttackTargetsCell = bConCella;
		Ivrin->PlannedAttackCell = FRTCellId(2, 0);
		Ivrin->PlannedCell = Ivrin->Cell;
		OutCellaIvrin = Ivrin->Cell;

		TM->LockInAndResolve();

		OutAttivazioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated) { OutAttivazioni.Add(Ev); }
		}
		return true;
	};

	TArray<FRTResolvedEvent> Attivazioni;
	FRTCellId CellaIvrin;
	if (!GiraIlTurno(/*bConCella=*/ false, Attivazioni, CellaIvrin)) { return false; }
	if (TestEqual(TEXT("🔴 senza cella: una attivazione"), Attivazioni.Num(), 1))
	{
		TestEqual(TEXT("di fase Prep"), static_cast<int32>(Attivazioni[0].Phase), static_cast<int32>(ERTMatchPhase::Prep));
		TestEqual(TEXT("con l'azione predittiva"), Attivazioni[0].ActionId, FName(TEXT("Hero.Ivrin.InterceptShot")));
		TestEqual(TEXT("e la cella di chi agisce: nessuna previsione dichiarata"), Attivazioni[0].AimCell, CellaIvrin);
	}

	if (!GiraIlTurno(/*bConCella=*/ true, Attivazioni, CellaIvrin)) { return false; }
	if (TestEqual(TEXT("✅ con la cella: una attivazione"), Attivazioni.Num(), 1))
	{
		TestEqual(TEXT("✅ che porta la cella prevista"), Attivazioni[0].AimCell, FRTCellId(2, 0));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Build e rosso**

Run: build; test `RefactorTactics.Turn.PrepActivatesEveryKindOfIntent+RefactorTactics.Turn.DashActivationPrecedesItsMove+RefactorTactics.Turn.CoverStructureActivatesOnlyWhenApplied+RefactorTactics.Turn.PredictiveWithoutCellStillActivates`.
Expected: build verde; tutti `Result={Fail}` (zero attivazioni nella timeline). Le premesse sul TurnLog dei due test di Review Focus devono già essere **verdi** qui: se una cade, il montaggio non arriva al sito e il test va corretto prima del codice. Se il secondo blocco del Dash cade sulla **premessa** («nessun Move»), il resolver muove chi carica un adiacente: riportalo come `IMPLEMENTATION DRIFT` e scegli un bersaglio a distanza 2 con un ostacolo — non togliere l'asserto.

- [ ] **Step 3: L'helper**

`RTTurnManager.h`, accanto a `FreezeVerdictFor` (`:2439`), nella stessa sezione privata:

```cpp
	/**
	 * L'UNICO costruttore di `AbilityActivated` (#<n>, spec «il momento» §2.1). Copia, non ricalcola: chi
	 * chiama passa cio' che il resolver ha gia' in mano nel punto in cui ACCETTA l'intento.
	 *
	 * ⚠️ Un `ActionId` `NAME_None` non emette e scrive un `ensureMsgf`: e' cio' che il sotto-progetto 3
	 * consuma, e un vuoto dimenticato non farebbe fallire nessun test (`RTResolvedEvent.h:310-312`).
	 */
	void EmitAbilityActivated(ARTUnit* Source, ERTMatchPhase InPhase, FName ActionId, FName BaseActionId,
		int32 TargetStableUnitId, const FRTCellId& AimCell, ERTAbilityShape Shape);
```

`RTTurnManager.cpp`, dopo la chiusura di `FreezeVerdictFor` (`:312`):

```cpp
void ARTTurnManager::EmitAbilityActivated(ARTUnit* Source, ERTMatchPhase InPhase, FName ActionId,
	FName BaseActionId, int32 TargetStableUnitId, const FRTCellId& AimCell, ERTAbilityShape Shape)
{
	if (!ensureMsgf(!ActionId.IsNone(), TEXT("AbilityActivated senza ActionId (fase %d): il produttore ha perso l'azione"),
		static_cast<int32>(InPhase)))
	{
		return;
	}
	if (Source == nullptr)
	{
		return; // nessuno da attivare: il sito chiamante ha gia' perso l'attore, e un id 0 non e' un'unita' ([D-063])
	}

	FRTResolvedEvent Ev;
	Ev.Phase = InPhase;
	Ev.Type = ERTResolvedEventType::AbilityActivated;
	Ev.SourceStableUnitId = Source->StableUnitId;
	Ev.TargetStableUnitId = TargetStableUnitId;
	Ev.ActionId = ActionId;
	Ev.BaseActionId = BaseActionId;
	Ev.AimCell = AimCell;
	Ev.Shape = Shape;
	// ⛔ Nessun riordino: l'ordine in timeline E' l'ordine di emissione (spec §2.2).
	ResolvedTimeline.Add(MoveTemp(Ev));
}
```

- [ ] **Step 4: Le coperture di Prep**

`ResolveCoverStructures`: aggiungi `FName BaseActionId;` **in coda** a `FRTPendingCoverOp` (dopo `ARTUnit* Actor = nullptr;`, `:3881`) e a `FRTPendingMoveOp` (dopo `int32 AbilityIndex = INDEX_NONE;`, `:3894`); poi estendi i due inizializzatori:

```cpp
			Moves.Add({ TargetCell, Edge, Def.ActionId, Unit, Index, Def.BaseActionId });
```

```cpp
		Pending.Add({ TargetCell, Edge, Integrity, Turns, FreeRotations, Def.ActionId, Unit, Def.BaseActionId });
```

Nel ciclo di applicazione delle creazioni, dopo `++Applied;` (`:4080`):

```cpp
		// #<n>: la copertura e' APPLICATA, quindi l'intento e' accettato. Un `Reject` e un `AddCover` rifiutato
		// sopra non arrivano qui: un rifiuto non si attiva (spec §2.2, `Ruling` sulle coperture). Una struttura
		// di un'unita' stordita SI attiva — questa funzione non passa da `RefuseMainActionIfStunned`.
		EmitAbilityActivated(Op.Actor, ERTMatchPhase::Prep, Op.ActionId, Op.BaseActionId,
			Op.Actor ? Op.Actor->StableUnitId : 0, Op.Cell, ERTAbilityShape::Single);
```

Nel ciclo degli spostamenti, dopo `AppendLogEntry(Entry2, Mover);` (`:4185`):

```cpp
		EmitAbilityActivated(Mover, ERTMatchPhase::Prep, Move.ActionId, Move.BaseActionId,
			Mover ? Mover->StableUnitId : 0, Move.Cell, ERTAbilityShape::Single);
```

- [ ] **Step 5: Predittiva, Overwatch e istanze**

Predittiva, prima di `Unit->ConsumeAbility(Index);` di `:4287` (fuori dall'`if (Unit->bAttackTargetsCell)`):

```cpp
			// #<n> `Ruling`: anche la predittiva SENZA cella si attiva — l'unita' ha speso l'azione, e
			// un'attivazione muta sarebbe indistinguibile da un difetto.
			EmitAbilityActivated(Unit, ERTMatchPhase::Prep, Ability->Def.ActionId, Ability->Def.BaseActionId,
				Unit->StableUnitId, Unit->bAttackTargetsCell ? Unit->PlannedAttackCell : Unit->Cell, Ability->Shape);
```

Overwatch, prima di `Unit->ConsumeAbility(Index);` di `:4346`:

```cpp
			EmitAbilityActivated(Unit, ERTMatchPhase::Prep, Ability->Def.ActionId, Ability->Def.BaseActionId,
				Unit->StableUnitId, Unit->Cell, Ability->Shape);
```

Il ciclo di consumo `:4418-4425` diventa:

```cpp
	// 5. Consuma le abilita' usate e libera i piani. #<n>: qui ogni istanza si ATTIVA, nell'ordine di
	// `SortActionInstances` — il terzo dei tre ordini della fase (coperture, raccolta, istanze; spec §2.2).
	for (const FRTActionInstance& Instance : Instances)
	{
		ARTUnit* Unit = Units[Instance.SourceUnitId];
		const URTActionData* Usata = Unit->GetAbility(Unit->PlannedAbilityIndex);
		EmitAbilityActivated(Unit, ERTMatchPhase::Prep, Instance.Def.ActionId, Instance.Def.BaseActionId,
			Unit->StableUnitId, Unit->Cell, Usata ? Usata->Shape : ERTAbilityShape::Single);
		Unit->ConsumeAbility(Unit->PlannedAbilityIndex);
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumato in Prep
		Unit->ClearPlannedAttack();
	}
```

- [ ] **Step 6: Il primo ciclo del Dash**

`RTTurnManager.cpp:4829-4876`: la condizione dell'`if` si spezza in due. Il ciclo diventa:

```cpp
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] == INDEX_NONE)
		{
			continue;
		}

		// #<n>, D4: l'attivazione PRIMA del `Move` dello stesso scatto, e anche quando lo scatto non entra in
		// nessuna cella — una carica su un adiacente colpisce senza muoversi, e il gesto c'e' stato.
		if (const URTActionData* Attivata = Units[i]->GetAbility(DashAbilityIdx[i]))
		{
			EmitAbilityActivated(Units[i], ERTMatchPhase::Dash, Attivata->Def.ActionId, Attivata->Def.BaseActionId,
				/*TargetStableUnitId=*/ 0, Units[i]->PlannedDashCell, Attivata->Shape);
		}

		if (Resolved[i].Entered.Num() > 0)
		{
			// <<< QUI il corpo di oggi: RTTurnManager.cpp:4833-4874, da `TArray<FRTCellId> Route;` fino a
			//     `ResolvedTimeline.Add(Ev);` compreso — reindentato di UN livello, nessuna riga cambiata >>>
		}
	}
```

Il blocco da spostare è il corpo dell'attuale `if (DashAbilityIdx[i] != INDEX_NONE && Resolved[i].Entered.Num() > 0) { ... }` del ciclo `RTTurnManager.cpp:4829-4876`, cioè le righe **`:4833-4874`**: si apre con `TArray<FRTCellId> Route;` e si chiude con `ResolvedTimeline.Add(Ev);`. **Reindentalo di un livello e non cambiare una riga**: commenti, `FreezeRouteVerdicts`, la lettura di `DashDef` e `Ev.CellVerdicts = Tracked.CellVerdicts;` restano identici. Verifica con `git diff -w` sul file: le uniche righe del ciclo che cambiano devono essere l'`if` spezzato e l'emissione nuova. Il secondo ciclo, `:4891`, non cambia.

- [ ] **Step 7: Build e verde**

Run: build; test `RefactorTactics.Turn+RefactorTactics.Playback.ResolvedEventCarriesTheActionIdentity+RefactorTactics.Playback.ActionBoundaryWalksARealTimeline`.
Expected: verdi, `**** TEST COMPLETE`.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp
git commit -F - <<'EOF'
feat(<n>): AbilityActivated emesso in Prep (coperture, predittiva, Overwatch, istanze) e nel Dash prima del Move

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 3: `InterruptedIntents` nel contesto e le quattro sorgenti del Blast

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTBlastContext.h:263-274` (campo nuovo accanto a `DegradedIntents`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp:446-486` (Cleanse), `:597` (Heal), `:704-705` (ModifyArc), `:1228` (`ApplyInterrupts`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (dichiarazione di `EmitAttackIntentActivations`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:5487-5503` (chiamata) e definizione nuova dopo `ResolveCombatPasses`
- Test: `Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp`

**Interfaces:**
- Consumes: `EmitAbilityActivated` (Task 2).
- Produces:
  - `TSet<int32> FRTBlastContext::InterruptedIntents;` — gli `IntentIndex` cancellati da un Interrupt efficace, riempito da `ApplyInterrupts`.
  - `void ARTTurnManager::EmitAttackIntentActivations(const FRTBlastContext& Ctx);` (privata).
  - Timeline di Blast: attivazioni nell'ordine dei pass — Cleanse, Heal, ModifyArc, intenti d'attacco per `IntentIndex`.

- [ ] **Step 1: I test rossi**

In `RTAbilityActivatedTests.cpp`, prima di `#endif`:

```cpp
/**
 * Un turno con un intento per fase produce UNA attivazione per fase — spec §5.1.
 *
 * 🔴 **La disposizione e' quella di `Visual.Core.PhaseOrder`, con tre intenti aggiunti**: lo scatto spende il
 * movimento (D-028), quindi Prep, Dash e Blast stanno su tre unita' diverse. `Action.Move` e `Action.Wait` ci
 * sono e non si attivano.
 * ⛔ **Controllo positivo**: una cura in piu' produce una quarta attivazione col suo `ActionId`. Senza, «tre»
 * sarebbe vero anche per un produttore che emette un numero fisso.
 * ✅ Validato per mutazione: togliere `EmitAttackIntentActivations(Ctx);` fa cadere il conteggio del Blast.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnAbilityActivatedOncePerIntentTest,
	"RefactorTactics.Turn.AbilityActivatedIsEmittedOncePerIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnAbilityActivatedOncePerIntentTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bConCura, TArray<FRTResolvedEvent>& OutAttivazioni) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* F1 = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
		ARTUnit* R1 = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 1));
		ARTUnit* B1 = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		ARTUnit* V1 = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, -1));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !F1 || !R1 || !B1 || !V1) { return false; }

		R1->PlannedAbilityIndex = IndiceAbilitaAttivazione(R1, TEXT("Hero.Muiren.TideGuard")); // Prep
		R1->PlannedCell = FRTCellId(-1, 1);                                                       // e cammina
		B1->PlannedDashAbility = IndiceAbilitaAttivazione(B1, TEXT("Hero.Branth.Ram"));           // Dash
		B1->PlannedDashCell = F1->Cell;
		B1->PlannedCell = B1->Cell;
		V1->PlannedAbilityIndex = IndiceAbilitaAttivazione(V1, TEXT("Hero.Ivrin.PulseShot"));     // Blast
		V1->PlannedAttackTarget = F1;
		V1->PlannedCell = V1->Cell;
		// F1: `Action.Wait`, oppure — nel controllo positivo — una cura su se stessa.
		F1->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(F1, bConCura ? TEXT("Action.Heal") : TEXT("Action.Wait"));
		F1->PlannedCell = F1->Cell;
		if (!TestTrue(TEXT("premessa: i tre intenti del catalogo esistono"),
			R1->PlannedAbilityIndex != INDEX_NONE && B1->PlannedDashAbility != INDEX_NONE && V1->PlannedAbilityIndex != INDEX_NONE))
		{
			return false;
		}

		TM->LockInAndResolve();

		OutAttivazioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated) { OutAttivazioni.Add(Ev); }
		}
		auto Una = [&OutAttivazioni](ERTMatchPhase Fase, int32 Sorgente, const TCHAR* Azione)
		{
			return OutAttivazioni.FilterByPredicate([&](const FRTResolvedEvent& E)
				{ return E.Phase == Fase && E.SourceStableUnitId == Sorgente && E.ActionId == FName(Azione); }).Num();
		};
		TestEqual(TEXT("Prep: TideGuard, una volta"), Una(ERTMatchPhase::Prep, R1->StableUnitId, TEXT("Hero.Muiren.TideGuard")), 1);
		TestEqual(TEXT("Dash: Ram, una volta"), Una(ERTMatchPhase::Dash, B1->StableUnitId, TEXT("Hero.Branth.Ram")), 1);
		TestEqual(TEXT("Blast: PulseShot, una volta"), Una(ERTMatchPhase::Blast, V1->StableUnitId, TEXT("Hero.Ivrin.PulseShot")), 1);
		// ⛔ I due esclusi, con il loro SOGGETTO. ⏱️ *La prima stesura asseriva «nessuna attivazione porta
		// `Action.Move`»: vero per costruzione, perche' nessun sito emette con quell'id — un asserto senza
		// soggetto.* Ora: R1 cammina davvero (premessa: il suo `Move` di fase Move c'e') e la fase Move non ha
		// attivazioni; F1 ha `Action.Wait` nel piano e, nel giro senza cura, nessuna attivazione.
		const bool bR1Cammina = TM->ResolvedTimelineForTest().ContainsByPredicate([R1](const FRTResolvedEvent& E)
		{
			return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Move
				&& E.SourceStableUnitId == R1->StableUnitId;
		});
		if (!TestTrue(TEXT("⛔ premessa: R1 ha camminato nel Move"), bR1Cammina)) { return false; }
		TestEqual(TEXT("⛔ il Move normale non si attiva"), OutAttivazioni.FilterByPredicate(
			[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Move; }).Num(), 0);
		if (!bConCura)
		{
			TestEqual(TEXT("⛔ Action.Wait non si attiva"), OutAttivazioni.FilterByPredicate(
				[F1](const FRTResolvedEvent& E) { return E.SourceStableUnitId == F1->StableUnitId; }).Num(), 0);
		}
		return true;
	};

	TArray<FRTResolvedEvent> Attivazioni;
	if (!GiraIlTurno(/*bConCura=*/ false, Attivazioni)) { return false; }
	TestEqual(TEXT("🔴 tre intenti, tre attivazioni"), Attivazioni.Num(), 3);

	if (!GiraIlTurno(/*bConCura=*/ true, Attivazioni)) { return false; }
	TestEqual(TEXT("✅ controllo positivo: la cura e' la quarta"), Attivazioni.Num(), 4);
	TestTrue(TEXT("✅ e porta il suo ActionId"), Attivazioni.ContainsByPredicate([](const FRTResolvedEvent& E)
		{ return E.Phase == ERTMatchPhase::Blast && E.ActionId == FName(TEXT("Action.Heal")); }));
	return true;
}

/**
 * Le quattro sorgenti del Blast si attivano nell'ordine dei PASS — Cleanse, Heal, Arc, Attack (spec §2.2).
 * Un `ModifyArc` fuori portata non si attiva: il ramo esce prima di `PendingArcOps.Add`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnBlastActivatesAllFourSourcesTest,
	"RefactorTactics.Turn.BlastActivatesAllFourSources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnBlastActivatesAllFourSourcesTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](bool bArcoInPortata, TArray<FName>& OutAzioni) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World);

		ARTUnit* Purificatore = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Ferito       = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(1, 2));
		ARTUnit* Curatore     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 2));
		ARTUnit* Arcista      = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),
			bArcoInPortata ? FRTCellId(2, 2) : FRTCellId(-4, 0));
		ARTUnit* Tiratore     = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Purificatore || !Ferito || !Curatore || !Arcista || !Tiratore) { return false; }

		// Nel secondo giro la Cleanse non trova lo stato dichiarato: `NoEffect`, ma SPESA — e si attiva lo
		// stesso (decisione (c): il gesto, non l'esito).
		if (bArcoInPortata) { Purificatore->ApplyStatus(TAG_Status_Root, 3); }
		Purificatore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Purificatore, TEXT("Action.Cleanse"), 3);
		Purificatore->PlannedCleansePriority = { TAG_Status_Root };
		Purificatore->PlannedCell = Purificatore->Cell;

		Ferito->Health = FMath::Max(1, Ferito->Health - 30);
		const int32 Cura = RTAbilityFixtures::AddCoreAbility(Curatore, TEXT("Action.Heal"));
		Curatore->PlannedAbilityIndex = Cura;
		Curatore->PlannedAttackTarget = Ferito;
		Curatore->PlannedCell = Curatore->Cell;
		// Nel secondo giro la cura e' SENZA EFFETTO: `Amount <= 0` e' il ramo `NoEffect` di
		// `RTTurnManager_Blast.cpp:587-593`. E' partita (spesa a `:558`), quindi si attiva lo stesso (decisione (c)).
		if (!bArcoInPortata)
		{
			for (FRTActionEffectSpec& Spec : Curatore->Abilities[Cura]->Def.Effects)
			{
				if (Spec.Effect == ERTActionEffect::Heal) { Spec.Amount = 0; }
			}
		}

		const int32 Arco = RTAbilityFixtures::AddCoreAbility(Arcista, TEXT("Action.ModifyArc"));
		Arcista->PlannedAbilityIndex = Arco;
		Arcista->PlannedAttackTarget = Ferito;
		Arcista->PlannedCell = Arcista->Cell;
		const bool bInPortata = URTHexLibrary::HexDistance(Arcista->Cell, Ferito->Cell)
			<= Arcista->GetAbility(Arco)->Def.RangeCells;
		if (!TestEqual(TEXT("premessa: la portata dell'arco e' quella che il caso dichiara"), bInPortata, bArcoInPortata))
		{
			return false;
		}

		Tiratore->PlannedAbilityIndex = IndiceAbilitaAttivazione(Tiratore, TEXT("Hero.Branth.ImpactShot"));
		Tiratore->PlannedAttackTarget = Purificatore;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->LockInAndResolve();

		if (!bArcoInPortata)
		{
			const bool bCuraVuota = TM->GetTurnLog().ContainsByPredicate([Curatore](const FRTTurnLogEntry& E)
			{
				return E.Category == ERTLogCategory::Fallback && E.UnitId == Curatore->StableUnitId
					&& E.ActionId == FName(TEXT("Action.Heal"))
					&& E.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect);
			});
			if (!TestTrue(TEXT("⛔ premessa: la cura senza effetto ha la sua voce NoEffect"), bCuraVuota)) { return false; }
		}

		OutAzioni.Reset();
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == ERTMatchPhase::Blast)
			{
				OutAzioni.Add(Ev.ActionId);
			}
		}
		return true;
	};

	TArray<FName> Azioni;
	if (!GiraIlTurno(/*bArcoInPortata=*/ true, Azioni)) { return false; }
	const TArray<FName> Attese = { FName(TEXT("Action.Cleanse")), FName(TEXT("Action.Heal")),
		FName(TEXT("Action.ModifyArc")), FName(TEXT("Hero.Branth.ImpactShot")) };
	TestEqual(TEXT("🔴 quattro sorgenti, nell'ordine dei pass"), Azioni, Attese);

	if (!GiraIlTurno(/*bArcoInPortata=*/ false, Azioni)) { return false; }
	TestFalse(TEXT("⛔ un arco fuori portata non si attiva"), Azioni.Contains(FName(TEXT("Action.ModifyArc"))));
	TestTrue(TEXT("🔑 una cura senza effetto (Amount <= 0, NoEffect) si attiva: e' stata spesa"),
		Azioni.Contains(FName(TEXT("Action.Heal"))));
	TestTrue(TEXT("🔑 una Cleanse senza effetto (NoEffect) si attiva: e' stata spesa"),
		Azioni.Contains(FName(TEXT("Action.Cleanse"))));
	return true;
}

/**
 * Per ogni intento di Blast l'attivazione precede l'impronta e i colpi con la stessa `(Source, ActionId)`.
 * ⛔ Nulla sui `StructureHit`: in timeline PRECEDONO l'attivazione (spec §2.2), li riordina solo la sequenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnActivationPrecedesFootprintAndHitsTest,
	"RefactorTactics.Turn.AbilityActivatedPrecedesItsFootprintAndHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnActivationPrecedesFootprintAndHitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World, 12);

	ARTUnit* Attaccante = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Bersaglio  = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { return false; }
	Attaccante->PlannedAbilityIndex = 0;
	Attaccante->PlannedAttackTarget = Bersaglio;
	Attaccante->PlannedCell = FRTCellId(2, 3);

	TM->LockInAndResolve();

	const TArray<FRTResolvedEvent>& T = TM->ResolvedTimelineForTest();
	int32 Confronti = 0;
	for (int32 a = 0; a < T.Num(); ++a)
	{
		if (T[a].Type != ERTResolvedEventType::AbilityActivated || T[a].Phase != ERTMatchPhase::Blast) { continue; }
		for (int32 j = 0; j < T.Num(); ++j)
		{
			const bool bStessoAtto = T[j].SourceStableUnitId == T[a].SourceStableUnitId && T[j].ActionId == T[a].ActionId;
			const bool bImprontaOColpo = T[j].Type == ERTResolvedEventType::AttackFootprint
				|| T[j].Type == ERTResolvedEventType::Attack;
			if (bStessoAtto && bImprontaOColpo)
			{
				TestTrue(*FString::Printf(TEXT("attivazione %d prima dell'evento %d dello stesso atto"), a, j), a < j);
				++Confronti;
			}
		}
	}
	// ⛔ ANTI-VACUITA': almeno un'impronta e un colpo confrontati, o il ciclo e' vero per assenza.
	TestTrue(TEXT("⛔ almeno due confronti (impronta e colpo)"), Confronti >= 2);
	return true;
}

/**
 * La carica si attiva nel Dash e NON di nuovo nel Blast: gli impatti entrano con `IntentAbilityIndex =
 * INDEX_NONE` e `IntentDefs = Impact.Def`, che e' il `Def` dello scatto (`RTTurnManager.cpp:4683`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnChargeActivatesInDashNotInBlastTest,
	"RefactorTactics.Turn.ChargeActivatesInDashNotInBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnChargeActivatesInDashNotInBlastTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World);

	ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
	ARTUnit* Caricatore = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bersaglio || !Caricatore) { return false; }
	Caricatore->PlannedDashAbility = IndiceAbilitaAttivazione(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->LockInAndResolve();

	const FName Ram(TEXT("Hero.Branth.Ram"));
	int32 Attivazioni = 0, ColpiDiImpatto = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.ActionId == Ram)
		{
			++Attivazioni;
			TestEqual(TEXT("l'attivazione della carica e' di fase Dash"),
				static_cast<int32>(Ev.Phase), static_cast<int32>(ERTMatchPhase::Dash));
		}
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.ActionId == Ram
			&& Ev.SourceStableUnitId == Caricatore->StableUnitId && Ev.Phase == ERTMatchPhase::Blast)
		{
			++ColpiDiImpatto;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: l'impatto ha colpito nel Blast con la chiave dello scatto"), ColpiDiImpatto > 0))
	{
		return false;
	}
	TestEqual(TEXT("🔴 una sola attivazione: Dash, non anche Blast"), Attivazioni, 1);
	return true;
}

/**
 * Interrotto: nessuna attivazione. Bloccato dalla linea di tiro: SI attiva (`Ruling`: il gesto, non l'esito).
 * Morto prima del Blast: nessuna (`IsAlive()`, `RTTurnManager_Blast.cpp:1377-1379`). Fuori portata: nessuna,
 * perche' `Fallback Cancelled` lo toglie prima che diventi un intento del Blast.
 * ✅ Validato per mutazione: tenere `InterruptedIntents` locale in `ApplyInterrupts` fa cadere il primo ramo;
 * togliere la guardia `IsAlive()` da `EmitAttackIntentActivations` fa cadere il terzo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnInterruptedOrDeadDoesNotActivateTest,
	"RefactorTactics.Turn.InterruptedOrDeadIntentDoesNotActivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnInterruptedOrDeadDoesNotActivateTest::RunTest(const FString&)
{
	auto AttivazioniDi = [](const ARTTurnManager* TM, const ARTUnit* U)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == U->StableUnitId) { ++N; }
		}
		return N;
	};

	// --- 1. Interrotto (montaggio di `RTControlActionTests.cpp:174-201`, distanze di UNA cella) ----------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 6);
		ARTUnit* Interruttore = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0));
		ARTUnit* Attaccante   = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, 0));
		ARTUnit* Vittima      = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(5, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Interruttore || !Attaccante || !Vittima) { return false; }

		Interruttore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Interruttore, TEXT("Action.Interrupt"));
		Interruttore->PlannedAttackTarget = Attaccante;
		Interruttore->PlannedCell = Interruttore->Cell;
		Attaccante->PlannedAbilityIndex = 0;
		Attaccante->PlannedAttackTarget = Vittima;
		Attaccante->PlannedCell = Attaccante->Cell;

		TM->LockInAndResolve();

		const bool bCancellato = TM->GetTurnLog().ContainsByPredicate([Attaccante](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback && E.UnitId == Attaccante->StableUnitId
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
		});
		if (!TestTrue(TEXT("⛔ premessa: l'azione e' stata CANCELLATA, non degradata"), bCancellato)) { return false; }
		TestEqual(TEXT("🔴 l'interrotto non si attiva"), AttivazioniDi(TM, Attaccante), 0);
		TestEqual(TEXT("✅ l'interruttore si'"), AttivazioniDi(TM, Interruttore), 1);
	}

	// --- 2. Linea di tiro bloccata da un muro alto: il gesto c'e' stato --------------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		URTHexMapAsset* Map = SpawnAttivazioneMap(World, 6);
		ARTUnit* Tiratore  = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Map || !Tiratore || !Bersaglio) { return false; }

		// Il bordo verso (1,0) e' l'indice di quella cella nell'anello: la stessa convenzione di
		// `ResolveCoverStructures` (`Ring[EdgeIndex]`, `RTTurnManager.cpp:4042-4044`).
		const TArray<FRTCellId> Anello = URTHexLibrary::Neighbors(FRTCellId(0, 0));
		const int32 Bordo = Anello.IndexOfByKey(FRTCellId(1, 0));
		if (!TestTrue(TEXT("premessa: (1,0) e' un vicino di (0,0)"), Bordo != INDEX_NONE)) { return false; }
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 0), static_cast<ERTHexDirection>(Bordo), ERTHexCoverType::High, 100);

		Tiratore->PlannedAbilityIndex = IndiceAbilitaAttivazione(Tiratore, TEXT("Hero.Ivrin.PulseShot"));
		Tiratore->PlannedAttackTarget = Bersaglio;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->LockInAndResolve();

		int32 Colpi = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Attack && Ev.SourceStableUnitId == Tiratore->StableUnitId) { ++Colpi; }
		}
		if (!TestEqual(TEXT("⛔ premessa: il muro ha fermato il colpo"), Colpi, 0)) { return false; }
		TestEqual(TEXT("🔑 bloccato dalla linea di tiro: SI attiva"), AttivazioniDi(TM, Tiratore), 1);
	}

	// --- 3. Morto prima del Blast: il piano gli resta addosso, la guardia no ----------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 6);
		ARTUnit* Morto     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Morto || !Bersaglio) { return false; }
		Morto->PlannedAbilityIndex = 0;
		Morto->PlannedAttackTarget = Bersaglio;
		Morto->PlannedCell = Morto->Cell;
		// Come `RTControlActionTests.cpp:2050-2053`: conta la guardia del resolver, non il modo in cui l'unita'
		// e' caduta — in Prep e nel Dash nessun danno si applica prima del Blast.
		Morto->Health = 0;

		TM->LockInAndResolve();
		TestEqual(TEXT("🔴 chi e' morto prima del Blast non si attiva"), AttivazioniDi(TM, Morto), 0);
	}

	// --- 4. Fuori portata: `Fallback Cancelled` PRIMA di entrare nel Blast (decisione (6), IMPLEMENTATION DRIFT)
	//
	// `CollectAttackIntents` manda a `ApplyFallback` ogni motivo che non sia `NoLineOfSight`/`NoMap`, e con un
	// `Cancel` fa `continue` prima di `Intents.Add` (`RTTurnManager_Blast.cpp:855-892`): l'intento non esiste
	// nel Blast, quindi non si attiva. Il rifiuto lo racconta il TurnLog — ed e' la premessa del ramo.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnAttivazioneMap(World, 8);
		ARTUnit* Lontano   = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(6, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Lontano || !Bersaglio) { return false; }
		const int32 Colpo = IndiceAbilitaAttivazione(Lontano, TEXT("Hero.Branth.ImpactShot"));
		if (!TestTrue(TEXT("premessa: il bersaglio e' oltre la portata"), Colpo != INDEX_NONE
			&& URTHexLibrary::HexDistance(Lontano->Cell, Bersaglio->Cell) > Lontano->GetAbility(Colpo)->Def.RangeCells))
		{
			return false;
		}
		Lontano->PlannedAbilityIndex = Colpo;
		Lontano->PlannedAttackTarget = Bersaglio;
		Lontano->PlannedCell = Lontano->Cell;

		TM->LockInAndResolve();

		const bool bAnnullato = TM->GetTurnLog().ContainsByPredicate([Lontano](const FRTTurnLogEntry& E)
		{
			return E.Category == ERTLogCategory::Fallback && E.UnitId == Lontano->StableUnitId
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange);
		});
		if (!TestTrue(TEXT("⛔ premessa: il TurnLog registra Fallback Cancelled per fuori portata"), bAnnullato))
		{
			return false;
		}
		TestEqual(TEXT("🔴 fuori portata: nessuna attivazione"), AttivazioniDi(TM, Lontano), 0);
	}
	return true;
}
```

- [ ] **Step 2: Build e rosso**

Run: build; test `RefactorTactics.Turn.AbilityActivatedIsEmittedOncePerIntent+RefactorTactics.Turn.BlastActivatesAllFourSources+RefactorTactics.Turn.AbilityActivatedPrecedesItsFootprintAndHits+RefactorTactics.Turn.ChargeActivatesInDashNotInBlast+RefactorTactics.Turn.InterruptedOrDeadIntentDoesNotActivate`.
Expected: build verde; `Result={Fail}` sui conteggi del Blast (zero attivazioni di fase Blast). `ChargeActivatesInDashNotInBlast` è già verde: il suo asserto morde solo dopo lo Step 4 — è la ragione per cui vive in questo task.

- [ ] **Step 3: `InterruptedIntents` sale nel contesto**

`RTBlastContext.h`, dopo `TSet<int32> DegradedIntents;` (`:274`):

```cpp
	/**
	 * Intenti che un Interrupt efficace ha CANCELLATO (`ApplyInterrupts`), l'insieme complementare a
	 * `DegradedIntents` (#<n>).
	 *
	 * 🔑 **Vive qui e non nel pass perche' ha un secondo lettore**: `EmitAttackIntentActivations`, che gira
	 * molto piu' tardi. `ApplyInterrupts` toglie gli interrotti da `Plan.Hits`, `Plan.Footprints` e dalle
	 * porte, NON da `Intents`: senza questo trasporto un intento cancellato si attiverebbe.
	 * ⚠️ Solo `Contains`: l'ordine di un `TSet` non decide niente.
	 */
	TSet<int32> InterruptedIntents;
```

`RTTurnManager_Blast.cpp:1228`:

```cpp
	TSet<int32>& InterruptedIntents = Ctx.InterruptedIntents; // cancellati: il colpo sparisce, l'azione e' annullata
```

- [ ] **Step 4: Le quattro sorgenti**

Cleanse, `RTTurnManager_Blast.cpp:463`, subito dopo `Ctx.MarkAbilitySpent(Unit, CleanseIdx);`:

```cpp
		Ctx.MarkAbilitySpent(Unit, CleanseIdx); // parte qui, si paga in `SpendStartedAbilities` (`#1451`)
		// #<n>, spec §2.2 punto 1: la purificazione si ATTIVA quando e' SPESA — il gesto, non l'esito. Anche una
		// Cleanse che non trova lo stato dichiarato (la voce `NoEffect` qui sotto) si attiva: e' lo stesso
		// `Ruling` della predittiva senza cella. ⛔ Nessuna condizione su `Removed`.
		EmitAbilityActivated(Unit, ERTMatchPhase::Blast, Cleanse->Def.ActionId, Cleanse->Def.BaseActionId,
			Unit->StableUnitId, Unit->Cell, ERTAbilityShape::Single);
```

Heal, `RTTurnManager_Blast.cpp:558`, subito dopo `Ctx.MarkAbilitySpent(Unit, HealIdx);` — **prima** del controllo `Amount <= 0` di `:587-593`:

```cpp
		Ctx.MarkAbilitySpent(Unit, HealIdx); // parte qui, si paga in `SpendStartedAbilities` (`#1451`)
		// #<n>, spec §2.2 punto 2: la cura si ATTIVA quando e' SPESA, anche se poi non ha effetto (`NoEffect`,
		// `:587-593`): il gesto, non l'esito. ⛔ Il fuori portata e' uscito con `continue` sopra (`:552`) e non si
		// paga: non e' un gesto, e non si attiva.
		EmitAbilityActivated(Unit, ERTMatchPhase::Blast, Heal->Def.ActionId, Heal->Def.BaseActionId,
			HealTarget->StableUnitId, HealTarget->Cell, ERTAbilityShape::Single);
```

ModifyArc, dopo `PendingArcOps.Add({ Unit->Cell, ArcTarget->Cell, Unit, PlannedNow->Def });` (`:705`):

```cpp
				// #<n>: dopo la validazione di portata — un arco fuori portata e' uscito a `:701` e non si attiva.
				EmitAbilityActivated(Unit, ERTMatchPhase::Blast, PlannedNow->Def.ActionId, PlannedNow->Def.BaseActionId,
					ArcTarget->StableUnitId, ArcTarget->Cell, ERTAbilityShape::Single);
```

`RTTurnManager.h`, accanto a `ResolveCombatPasses`:

```cpp
	/**
	 * Un `AbilityActivated` per ogni intento d'attacco ACCETTATO, in ordine di `IntentIndex` (#<n>, spec §2.2
	 * punto 4). Esclusi: gli impatti di carica (`IntentAbilityIndex == INDEX_NONE`, gia' attivati nel Dash),
	 * gli interrotti (`Ctx.InterruptedIntents`), chi e' morto prima del Blast. Inclusi: i degradati ([D-300])
	 * e i bloccati da linea di tiro o senza mappa (`NoLineOfSight`, `NoMap`, `UnverifiableIntents`): entrano in
	 * `Ctx.Intents`, e l'attivazione racconta il gesto.
	 *
	 * ⛔ **Non vede mai i `Fallback Cancelled`** — fuori portata, bersaglio ignoto, bersaglio sparito: per
	 * loro `CollectAttackIntents` fa `continue` PRIMA di `Intents.Add` (`RTTurnManager_Blast.cpp:855-892`),
	 * quindi non si attivano. E' la stessa regola del `ModifyArc` fuori portata (spec §2.2, §6).
	 */
	void EmitAttackIntentActivations(const FRTBlastContext& Ctx);
```

`RTTurnManager.cpp`, in `ResolveCombatPasses` dopo `ApplyEnvironmentChanges(Ctx);` (`:5487`):

```cpp
	ApplyEnvironmentChanges(Ctx);

	// #<n>: le attivazioni degli intenti d'attacco, PRIMA delle impronte qui sotto e dei colpi del ciclo del
	// danno: in timeline l'attivazione precede l'impronta e i colpi dello stesso intento (D3). ⚠️ I
	// `StructureHit` di `ApplyEnvironmentChanges` restano PRIMA: e' dichiarato (spec §2.2), e la sequenza di
	// playback li raggruppa per intento.
	EmitAttackIntentActivations(Ctx);
```

E la definizione, dopo la chiusura di `ResolveCombatPasses`:

```cpp
void ARTTurnManager::EmitAttackIntentActivations(const FRTBlastContext& Ctx)
{
	for (int32 k = 0; k < Ctx.Intents.Num(); ++k)
	{
		if (!Ctx.IntentAbilityIndex.IsValidIndex(k) || Ctx.IntentAbilityIndex[k] == INDEX_NONE)
		{
			continue; // impatto di carica: attivato nel Dash, con lo stesso `Def` dello scatto
		}
		if (Ctx.InterruptedIntents.Contains(k) || !Ctx.IntentDefs.IsValidIndex(k))
		{
			continue; // cancellato da un Interrupt efficace: l'azione e' annullata, non attivata
		}
		const FRTHexAttackIntent& Intent = Ctx.Intents[k];
		ARTUnit* const Attaccante = Ctx.Units.IsValidIndex(Intent.AttackerId) ? Ctx.Units[Intent.AttackerId] : nullptr;
		if (Attaccante == nullptr || !Attaccante->IsAlive())
		{
			continue; // morto in Prep o nel Dash: la stessa guardia di `ApplyInterrupts` (`RTTurnManager_Blast.cpp:1377-1379`)
		}
		const ARTUnit* const Bersaglio = (Intent.TargetId != INDEX_NONE && Ctx.Units.IsValidIndex(Intent.TargetId))
			? Ctx.Units[Intent.TargetId] : nullptr;
		const FRTActionDef& Def = Ctx.IntentDefs[k];
		EmitAbilityActivated(Attaccante, ERTMatchPhase::Blast, Def.ActionId, Def.BaseActionId,
			Bersaglio ? Bersaglio->StableUnitId : 0, Bersaglio ? Bersaglio->Cell : Intent.TargetCell, Intent.Shape);
	}
}
```

- [ ] **Step 5: Build, verde, e l'hash che non si muove**

Run: build; test `RefactorTactics.Turn+RefactorTactics.Actions+RefactorTactics.Reactions+RefactorTactics.Playback+RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback`.
Expected: tutti verdi, `**** TEST COMPLETE`. Poi rimisura: `grep -n ResolvedTimeline Source/RefactorTactics/Turn/RTMatchStateHash.h` → nessuna riga (annotalo nel commit come evidenza: è uno zero che conta).

- [ ] **Step 6: Mutazione (1) — l'emissione degli intenti d'attacco**

Commenta la riga `EmitAttackIntentActivations(Ctx);` in `ResolveCombatPasses`, build, test `RefactorTactics.Turn.AbilityActivatedIsEmittedOncePerIntent`.
Expected: `Result={Fail}` su «Blast: PulseShot, una volta» e «tre intenti, tre attivazioni». Ripristina, build, verde.

- [ ] **Step 7: Mutazione (4) — `InterruptedIntents` locale**

Riporta `RTTurnManager_Blast.cpp:1228` a `TSet<int32> InterruptedIntents;`, build, test `RefactorTactics.Turn.InterruptedOrDeadIntentDoesNotActivate`.
Expected: `Result={Fail}` su «l'interrotto non si attiva». Ripristina il riferimento, build, verde.

- [ ] **Step 7a: Mutazione della cura senza effetto**

Sposta l'`EmitAbilityActivated` della cura **dopo** il controllo `if (Amount <= 0) { ... continue; }` (`RTTurnManager_Blast.cpp:587-593`), build, test `RefactorTactics.Turn.BlastActivatesAllFourSources`.
Expected: `Result={Fail}` su «una cura senza effetto (Amount <= 0, NoEffect) si attiva», nel secondo giro. Ripristina la posizione accanto a `MarkAbilitySpent` (`:558`), build, verde.

Riepilogo delle mutazioni di questo task:

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| (1) Commentare `EmitAttackIntentActivations(Ctx);` | `Turn.AbilityActivatedIsEmittedOncePerIntent` | «Blast: PulseShot, una volta» |
| (4) `TSet<int32> InterruptedIntents;` locale in `ApplyInterrupts` | `Turn.InterruptedOrDeadIntentDoesNotActivate` | «l'interrotto non si attiva» |
| Emissione della cura dopo `Amount <= 0` | `Turn.BlastActivatesAllFourSources` | «una cura senza effetto … si attiva» |
| Guardia `IsAlive()` tolta | `Turn.InterruptedOrDeadIntentDoesNotActivate` | «chi e' morto prima del Blast non si attiva» |

- [ ] **Step 7b: Mutazione della guardia `IsAlive()`**

In `EmitAttackIntentActivations` sostituisci `if (Attaccante == nullptr || !Attaccante->IsAlive())` con `if (Attaccante == nullptr)`, build, test `RefactorTactics.Turn.InterruptedOrDeadIntentDoesNotActivate`.
Expected: `Result={Fail}` su «chi e' morto prima del Blast non si attiva» — è la prova che il morto **arriva** in `Ctx.Intents` (`CollectAttackIntents` non filtra i morti, `RTTurnManager_Blast.cpp:1377-1379`). ⚠️ Se resta verde, il ramo è vacuo: il morto non diventa un intento (per esempio `ValidatePlansAtLockIn` gli toglie il piano). Allora fermati e riportalo come `IMPLEMENTATION DRIFT`: non togliere la guardia, e cambia il montaggio con una morte avvenuta **dentro** il turno prima del Blast, dichiarandolo nel test. Ripristina, build, verde.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTBlastContext.h Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp
git commit -F - <<'EOF'
feat(<n>): AbilityActivated dalle quattro sorgenti del Blast nell'ordine dei pass; InterruptedIntents nel contesto

grep -n ResolvedTimeline Turn/RTMatchStateHash.h -> 0 righe: nessun dato nuovo nell'hash.

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 4: `SourceVerdict`, congelato all'emissione, e il beat `ShowActivation`

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTResolvedEvent.h` (campo nuovo dopo `CellVerdicts`, `:244-245`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`EmitAbilityActivated`, Task 2) e definizione di `ShowActivation`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (dichiarazione privata di `ShowActivation`)
- Test: `Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp`

**Interfaces:**
- Consumes: `EmitAbilityActivated` (Task 2), `FreezeVerdictFor` (`RTTurnManager.h:2439`), `FRTLogSubject::Frozen(int32, const FRTKnowledgeVerdict&)` (`Turn/RTCombatLog.h:55`).
- Produces:
  - `UPROPERTY() FRTKnowledgeVerdict FRTResolvedEvent::SourceVerdict;` — vuoto = `NoOne()` = fail-closed.
  - `void ARTTurnManager::ShowActivation(const FRTResolvedEvent& Ev);` (privata) — suona `Cast` sulla sorgente e scrive `Attiva: <unità> -> <ActionId>` con il verdetto **congelato** (nessun ricalcolo). La chiamano i rami di playback del Task 7.

- [ ] **Step 1: Il test rosso**

In `RTAbilityActivatedTests.cpp`, prima di `#endif`:

```cpp
/**
 * Il verdetto di [D-223] si congela all'emissione: la squadra della sorgente vede la propria attivazione, una
 * squadra assente dalla partita no (fail-closed). Spec §2.1 e §2.5.
 *
 * ⚠️ **Precondizione dichiarata**: `FreezeVerdict` itera solo le squadre presenti in `TeamKnowledgeState`, e il
 * Blast lo rinfresca in testa (`RefreshTeamKnowledgeForBlast`). Per la Prep serve il refresh di pianificazione:
 * qui lo si chiama esplicitamente, ed e' la stessa precondizione delle righe di log.
 * ✅ Validato per mutazione: togliere `Ev.SourceVerdict = ...` da `EmitAbilityActivated` fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnActivationFreezesSourceVerdictTest,
	"RefactorTactics.Turn.AbilityActivatedFreezesTheSourceVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnActivationFreezesSourceVerdictTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnAttivazioneMap(World);

	ARTUnit* Scudo     = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
	ARTUnit* Tiratore  = SpawnAttivazioneUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Bersaglio = SpawnAttivazioneUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Scudo || !Tiratore || !Bersaglio) { return false; }

	Scudo->PlannedAbilityIndex = IndiceAbilitaAttivazione(Scudo, TEXT("Hero.Muiren.TideGuard"));
	Scudo->PlannedCell = Scudo->Cell;
	Tiratore->PlannedAbilityIndex = 0;
	Tiratore->PlannedAttackTarget = Bersaglio;
	Tiratore->PlannedCell = Tiratore->Cell;

	TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep
	TM->LockInAndResolve();

	const int32 SquadraAssente = 7; // nessuna unita' in campo: fuori da `TeamKnowledgeState`
	int32 Viste = 0;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type != ERTResolvedEventType::AbilityActivated) { continue; }
		++Viste;
		TestTrue(*FString::Printf(TEXT("%s: la squadra della sorgente la vede"), *Ev.ActionId.ToString()),
			Ev.SourceVerdict.AllowsTeam(0));
		TestFalse(*FString::Printf(TEXT("%s: ⛔ una squadra assente no (fail-closed)"), *Ev.ActionId.ToString()),
			Ev.SourceVerdict.AllowsTeam(SquadraAssente));
	}
	TestEqual(TEXT("⛔ anti-vacuita': le due attivazioni, Prep e Blast"), Viste, 2);
	return true;
}
```

- [ ] **Step 2: Build e rosso**

Run: build. Expected: errore di compilazione su `SourceVerdict` non membro di `FRTResolvedEvent` — è il rosso di questo passo.

- [ ] **Step 3: Il campo e il congelamento**

`RTResolvedEvent.h`, dopo `TArray<FRTKnowledgeVerdict> CellVerdicts;` (`:245`):

```cpp
	/**
	 * Chi puo' vedere la SORGENTE agire, nell'istante in cui agisce — solo per `AbilityActivated` (#<n>).
	 *
	 * 🔴 **Non e' `CellVerdicts`**: un'attivazione non ha rotta ne' celle, e un vettore vuoto letto con
	 * `ObservedPrefixLength` nasconderebbe ogni attivazione, comprese quelle di chi guarda. Il predicato e'
	 * quello delle righe di log: `AllowsTeam`, su questo verdetto congelato da `FreezeVerdictFor`.
	 *
	 * ⚠️ Vuoto = `NoOne()` = fail-closed. `UPROPERTY()` nudo per la stessa ragione di `CellVerdicts`: un
	 * verdetto leggibile da Blueprint sarebbe anche un verdetto aggirabile da Blueprint.
	 */
	UPROPERTY()
	FRTKnowledgeVerdict SourceVerdict;
```

`EmitAbilityActivated` (Task 2), prima di `ResolvedTimeline.Add`:

```cpp
	// [D-223], spec §2.1: il verdetto si decide ADESSO, quando l'unita' agisce, e si trasporta. Lo stesso valore
	// fa da verdetto alla riga `Attiva:` del playback (`ShowActivation`): calcolato una volta, mai due.
	Ev.SourceVerdict = FreezeVerdictFor(FRTLogSubject::Unit(Source));
```

- [ ] **Step 4: `ShowActivation`**

`RTTurnManager.h`, accanto a `PausePlaybackAtActBoundary` (`:2355`):

```cpp
	/**
	 * Il beat di un `AbilityActivated`: la clip del ruolo `Cast` sulla sorgente e la riga `Attiva:` del feed
	 * (#<n>, spec §2.3). ⛔ Non decide se mostrarlo: il filtro di privacy sta a monte, in `BeginPlayback` e in
	 * `BuildBlastSequence`. La riga porta il verdetto CONGELATO nell'evento, non uno ricalcolato.
	 */
	void ShowActivation(const FRTResolvedEvent& Ev);
```

`RTTurnManager.cpp`, dopo `PausePlaybackAtActBoundary` (`:7888-7894`):

```cpp
void ARTTurnManager::ShowActivation(const FRTResolvedEvent& Ev)
{
	ARTUnit* const Src = UnitByStableId(Ev.SourceStableUnitId);
	AddLogEvent(FString::Printf(TEXT("Attiva: %s -> %s"), Src ? *Src->GetName() : TEXT("?"), *Ev.ActionId.ToString()),
		FRTLogSubject::Frozen(Ev.SourceStableUnitId, Ev.SourceVerdict));
	if (Src)
	{
		Src->PlayPresentationRole(ERTPresentationRole::Cast);
	}
}
```

- [ ] **Step 5: Build e verde**

Run: build; test `RefactorTactics.Turn`. Expected: verdi, `**** TEST COMPLETE`. (`ShowActivation` non ha ancora chiamanti: la usa il Task 7. Se il compilatore segnala la funzione inutilizzata come errore, non è così per i membri — procedi.)

- [ ] **Step 6: Mutazione del congelamento**

Commenta `Ev.SourceVerdict = FreezeVerdictFor(...)`, build, test `RefactorTactics.Turn.AbilityActivatedFreezesTheSourceVerdict`. Expected: `Result={Fail}` su «la squadra della sorgente la vede». Ripristina, build, verde.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Turn/RTResolvedEvent.h Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTAbilityActivatedTests.cpp
git commit -F - <<'EOF'
feat(<n>): SourceVerdict congelato all'emissione di AbilityActivated e beat ShowActivation con la riga Attiva:

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 5: La matematica pura — `BuildBlastSequence`, `BlastPhaseIsActive` a cinque, `PhaseTime` nuova

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.h:153-245` (`BlastPhaseIsActive`, `PhaseDuration`, `PhaseTime` e loro commenti) e prima di `UCLASS()` (`:110`) per la struct
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp:47-133`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:7795-7796` e `:8881-8883` (adattamento **transitorio**, sostituito nel Task 7)
- Test: `Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp:145-192`, `:514`, `:794-1084` e in coda

**Interfaces:**
- Consumes: `FRTResolvedEvent::SourceVerdict` (Task 4), `FRTKnowledgeVerdict::AllowsTeam(int32)` (`Perception/RTTeamKnowledge.h:187`).
- Produces:
  - `struct FRTBlastSequenceElement { int32 TimelineIndex; int32 SourceStableUnitId; FName ActionId; bool operator==(...) const; };`
  - `static TArray<FRTBlastSequenceElement> URTPlaybackLibrary::BuildBlastSequence(const TArray<FRTResolvedEvent>& Timeline, const TArray<FRTBlastSequenceElement>& Previous, int32 FrozenPrefix, int32 ViewerTeamId);`
  - `static bool URTPlaybackLibrary::BlastPhaseIsActive(int32 NumAttacks, bool bHasBlastMove, int32 NumFootprints, int32 NumStructureHits, int32 NumActivations);`
  - `static FRTPhaseTime URTPlaybackLibrary::PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumActivations, int32 NumSequenceElements, float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds);`
  - `PhaseDuration` invariata nella firma: delega con `NumActivations = 0`, `NumSequenceElements = NumAttacks`.

- [ ] **Step 1: I test puri rossi**

In coda a `RTPlaybackLibraryTests.cpp`, prima di `#endif`:

```cpp
namespace
{
	/** Un evento sintetico di fase Blast, visibile a tutti salvo `bNascosto`. Nome distinto per l'unity build. */
	FRTResolvedEvent SeqEvento(ERTResolvedEventType Type, int32 Sorgente, const TCHAR* Azione, bool bNascosto = false)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = Type;
		Ev.SourceStableUnitId = Sorgente;
		Ev.ActionId = Azione ? FName(Azione) : NAME_None;
		Ev.SourceVerdict = bNascosto ? FRTKnowledgeVerdict::NoOne() : FRTKnowledgeVerdict::Everyone();
		return Ev;
	}

	TArray<int32> SeqIndici(const TArray<FRTBlastSequenceElement>& S)
	{
		TArray<int32> Out;
		for (const FRTBlastSequenceElement& E : S) { Out.Add(E.TimelineIndex); }
		return Out;
	}
}

/**
 * La sequenza del Blast e' ordinata PER INTENTO — D5, spec §2.4.
 *
 * 🔑 `A1 F1 H1 H1 A2 F2 H2`: un gruppo per `(Source, ActionId)` nell'ordine di prima apparizione, e dentro il
 * gruppo attivazione, impronte, muri, colpi. Un colpo senza attivazione e' un atto proprio; `ArcHit` non entra;
 * un'attivazione non visibile non entra, ma il suo colpo si'.
 * ✅ Validato per mutazione: raggruppare per TIPO invece che per chiave fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBlastSequenceOrderedPerIntentTest,
	"RefactorTactics.Playback.BlastSequenceIsOrderedPerIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBlastSequenceOrderedPerIntentTest::RunTest(const FString&)
{
	// 🔑 **Lo `StructureHit` di B sta IN TESTA**, come in un turno vero: `ApplyEnvironmentChanges` lo emette
	// prima delle attivazioni (spec §2.2). Ordinando gli atti per prima apparizione B passerebbe davanti ad A,
	// che ha `IntentIndex` minore; per decisione (d) gli atti si ordinano per la loro ATTIVAZIONE.
	TArray<FRTResolvedEvent> T;
	T.Add(SeqEvento(ERTResolvedEventType::StructureHit,     2, TEXT("B")));          // 0  W2 (muro di B, in testa)
	T.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 1, TEXT("A")));          // 1  A1
	T.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 2, TEXT("B")));          // 2  A2
	T.Add(SeqEvento(ERTResolvedEventType::AttackFootprint,  1, TEXT("A")));          // 3  F1
	T.Add(SeqEvento(ERTResolvedEventType::AttackFootprint,  2, TEXT("B")));          // 4  F2
	T.Add(SeqEvento(ERTResolvedEventType::Attack,           1, TEXT("A")));          // 5  H1
	T.Add(SeqEvento(ERTResolvedEventType::Attack,           2, TEXT("B")));          // 6  H2
	T.Add(SeqEvento(ERTResolvedEventType::Attack,           1, TEXT("A")));          // 7  H1
	T.Add(SeqEvento(ERTResolvedEventType::Attack,           3, TEXT("C")));          // 8  senza attivazione
	T.Add(SeqEvento(ERTResolvedEventType::ArcHit,           0, nullptr));            // 9  non entra
	T.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 4, TEXT("D"), true));    // 10 nascosta: non entra
	T.Add(SeqEvento(ERTResolvedEventType::Attack,           4, TEXT("D")));          // 11 il suo colpo resta

	const TArray<FRTBlastSequenceElement> S = URTPlaybackLibrary::BuildBlastSequence(T, {}, 0, /*Viewer*/ 0);
	// A1 F1 H1 H1 | A2 F2 W2 H2 | C | D. ⛔ Con «prima apparizione» uscirebbe { 2, 4, 0, 6, 1, 3, 5, 7, 8, 11 }:
	// e' la mutazione che fissa la decisione (d), Step 7.
	const TArray<int32> Atteso = { 1, 3, 5, 7, 2, 4, 0, 6, 8, 11 };
	TestEqual(TEXT("🔴 atti per attivazione: A1 F1 H1 H1, A2 F2 W2 H2, poi gli atti senza attivazione"),
		SeqIndici(S), Atteso);
	TestFalse(TEXT("⛔ ArcHit non entra nella sequenza"), SeqIndici(S).Contains(9));
	TestFalse(TEXT("⛔ un'attivazione nascosta non entra"), SeqIndici(S).Contains(10));
	if (S.Num() > 0)
	{
		TestEqual(TEXT("l'elemento porta la chiave del suo atto"), S[0].SourceStableUnitId, 1);
		TestEqual(TEXT("e la sua azione"), S[0].ActionId, FName(TEXT("A")));
	}
	return true;
}

/**
 * Il prefisso gia' mostrato e' STABILE all'estensione — D-355, spec §2.4.
 *
 * 🔑 Gli eventi nuovi di un atto gia' interamente mostrato si accodano come atto proprio (`Ruling`: coda, non
 * inserimento); quelli di un atto non ancora mostrato vi si uniscono. Precondizione: la timeline cresce solo
 * per accodamento, e lo si asserisce confrontando i `TimelineIndex` del prefisso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBlastSequencePrefixStableTest,
	"RefactorTactics.Playback.BlastSequencePrefixIsStableUnderExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBlastSequencePrefixStableTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T1;
	T1.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 1, TEXT("A"))); // 0
	T1.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 2, TEXT("B"))); // 1
	T1.Add(SeqEvento(ERTResolvedEventType::AttackFootprint,  1, TEXT("A"))); // 2
	T1.Add(SeqEvento(ERTResolvedEventType::Attack,           1, TEXT("A"))); // 3
	T1.Add(SeqEvento(ERTResolvedEventType::Attack,           2, TEXT("B"))); // 4

	const TArray<FRTBlastSequenceElement> S1 = URTPlaybackLibrary::BuildBlastSequence(T1, {}, 0, 0);
	TestEqual(TEXT("premessa: S1 = A F H | B H"), SeqIndici(S1), TArray<int32>({ 0, 2, 3, 1, 4 }));

	TArray<FRTResolvedEvent> T2 = T1;
	T2.Add(SeqEvento(ERTResolvedEventType::Attack, 1, TEXT("A")));            // 5: atto A gia' mostrato
	T2.Add(SeqEvento(ERTResolvedEventType::Attack, 2, TEXT("B")));            // 6: atto B non ancora mostrato

	const int32 Mostrati = 3; // l'atto A per intero
	const TArray<FRTBlastSequenceElement> S2 = URTPlaybackLibrary::BuildBlastSequence(T2, S1, Mostrati, 0);
	for (int32 i = 0; i < Mostrati; ++i)
	{
		TestEqual(*FString::Printf(TEXT("🔴 l'elemento %d del prefisso e' riprodotto verbatim"), i),
			S2.IsValidIndex(i) ? S2[i].TimelineIndex : INDEX_NONE, S1[i].TimelineIndex);
	}
	TestEqual(TEXT("B raccoglie il proprio colpo nuovo; il colpo nuovo di A sta in CODA"),
		SeqIndici(S2), TArray<int32>({ 0, 2, 3, 1, 4, 6, 5 }));

	// Con prefisso zero e' la costruzione da zero: `Previous` non conta.
	TestEqual(TEXT("FrozenPrefix 0 ignora Previous"),
		SeqIndici(URTPlaybackLibrary::BuildBlastSequence(T2, S1, 0, 0)),
		SeqIndici(URTPlaybackLibrary::BuildBlastSequence(T2, {}, 0, 0)));
	return true;
}

/** Un Blast di sole attivazioni apre la fase — spec §2.4, C3. Il quinto termine e' INDIPENDENTE. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBlastOpensForActivationsOnlyTest,
	"RefactorTactics.Playback.BlastPhaseOpensForActivationsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBlastOpensForActivationsOnlyTest::RunTest(const FString&)
{
	TestTrue(TEXT("una sola attivazione apre il Blast"), URTPlaybackLibrary::BlastPhaseIsActive(0, false, 0, 0, 1));
	TestFalse(TEXT("⛔ il vuoto resta vuoto"), URTPlaybackLibrary::BlastPhaseIsActive(0, false, 0, 0, 0));
	return true;
}

/**
 * `PhaseTime` conta le attivazioni — spec §2.4, I5.
 *
 * Prep: `N x ASS` mostrati piu' il beat; Dash: `N x ASS` piu' il movimento; Blast: la sequenza, col `Max` sulla
 * spinta. Con zero tutto resta com'era, e `PhaseDuration` non cambia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackPhaseTimeCountsActivationsTest,
	"RefactorTactics.Playback.PhaseTimeCountsActivations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackPhaseTimeCountsActivationsTest::RunTest(const FString&)
{
	const FRTPhaseTime Prep3 = URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Prep, 0, 3, 0, 2.f, 0.5f, 0.3f);
	TestTrue(TEXT("Prep: 3 attivazioni = 1,5 s mostrati"), FMath::IsNearlyEqual(Prep3.Shown, 1.5f, RTTol));
	TestTrue(TEXT("Prep: il beat resta slack"), FMath::IsNearlyEqual(Prep3.Slack, 0.3f, RTTol));
	const FRTPhaseTime Prep0 = URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Prep, 0, 0, 0, 2.f, 0.5f, 0.3f);
	TestTrue(TEXT("Prep senza attivazioni: com'era"), FMath::IsNearlyEqual(Prep0.Shown, 0.f, RTTol)
		&& FMath::IsNearlyEqual(Prep0.Slack, 0.3f, RTTol));

	TestTrue(TEXT("Dash: 2 attivazioni + 3 celle a 2 c/s = 2,5 s"), FMath::IsNearlyEqual(
		URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Dash, 3, 2, 0, 2.f, 0.5f, 0.3f).Shown, 2.5f, RTTol));
	TestTrue(TEXT("Dash senza attivazioni: com'era, 1,5 s"), FMath::IsNearlyEqual(
		URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Dash, 3, 0, 0, 2.f, 0.5f, 0.3f).Shown, 1.5f, RTTol));

	TestTrue(TEXT("Blast: 4 elementi = 2,0 s"), FMath::IsNearlyEqual(
		URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Blast, 0, 0, 4, 2.f, 0.5f, 0.3f).Shown, 2.0f, RTTol));
	TestTrue(TEXT("⛔ Blast: le attivazioni contano GIA' nella sequenza, non due volte"), FMath::IsNearlyEqual(
		URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Blast, 0, 5, 0, 2.f, 0.5f, 0.3f).Shown, 0.5f, RTTol));

	TestTrue(TEXT("PhaseDuration invariata: Blast 1,5 s come prima"), FMath::IsNearlyEqual(
		URTPlaybackLibrary::PhaseDuration(ERTMatchPhase::Blast, 3, 2, 2.f, 0.5f, 0.3f), 1.5f, RTTol));
	return true;
}
```

- [ ] **Step 2: Build e rosso**

Run: build. Expected: errori su `FRTBlastSequenceElement`, `BuildBlastSequence`, `BlastPhaseIsActive` a cinque argomenti, `PhaseTime` a sette.

- [ ] **Step 3: La struct e le dichiarazioni**

`RTPlaybackLibrary.h`, prima di `UCLASS()` (`:110`):

```cpp
/**
 * Un elemento della sequenza di Blast (#<n>, D5): l'indice dell'evento in timeline e la chiave del suo atto.
 *
 * ⚠️ **Non e' un `USTRUCT`**: vive fra `BuildBlastSequence` e il playback, non si serializza, non entra in
 * snapshot, TurnLog o hash. `TimelineIndex` vale solo finche' la timeline cresce per accodamento (D-355).
 */
struct FRTBlastSequenceElement
{
	int32 TimelineIndex = INDEX_NONE;
	int32 SourceStableUnitId = 0;
	FName ActionId;

	bool operator==(const FRTBlastSequenceElement& Other) const
	{
		return TimelineIndex == Other.TimelineIndex && SourceStableUnitId == Other.SourceStableUnitId
			&& ActionId == Other.ActionId;
	}
};
```

`BlastPhaseIsActive` (`:184-186`):

```cpp
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static bool BlastPhaseIsActive(int32 NumAttacks, bool bHasBlastMove, int32 NumFootprints,
		int32 NumStructureHits, int32 NumActivations);
```

Aggiungi al commento di `BlastPhaseIsActive`, prima di `⛔ **Nessuna somma e nessuna soglia`:

```cpp
	 * 🔴 **E `NumActivations` e' il quinto, per la stessa ragione** (#<n>, spec §2.4 C3): un Blast di sole
	 * cure o purificazioni non ha colpi, impronte, muri ne' spinta, e senza questo termine le sue attivazioni
	 * non avrebbero una fase in cui accadere.
	 *
```

`PhaseTime` (`:242-245`) e il suo commento (`:232-241`):

```cpp
	/**
	 * La formula di durata, nei suoi due termini: quanto della fase e' mostrato e quanto e' attesa.
	 *
	 *  - `Prep`  → `Shown = NumActivations x ASS`, `Slack = PhaseBeatSeconds`: il beat di oggi resta, le
	 *              attivazioni si mostrano (#<n>).
	 *  - `Dash`  → `Shown = NumActivations x ASS + movimento`: prima le attivazioni, poi le rotte.
	 *  - `Move`  → tutto `Shown`, il movimento.
	 *  - `Blast` → tutto `Shown`, `Max(Max(1, NumSequenceElements) x ASS, spinta)`. ⏱️ *Fino a #<n> era il
	 *              `Max` fra TRE canali paralleli (colpi, muri da #2828, impronte da #3278); la sequenza per
	 *              intento (D5) li svela uno dopo l'altro, quindi la fase dura quanto la SEQUENZA.* Le
	 *              attivazioni di Blast sono gia' elementi della sequenza: `NumActivations` qui non conta.
	 *  - ogni altra fase → tutto `Slack`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static FRTPhaseTime PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumActivations,
		int32 NumSequenceElements, float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds);

	/**
	 * La sequenza per intento del Blast (D5, spec §2.4). Gli eventi di fase Blast di tipo AbilityActivated,
	 * AttackFootprint, StructureHit e Attack si raggruppano per (SourceStableUnitId, ActionId). I gruppi si
	 * ordinano per l'indice in timeline della loro ATTIVAZIONE visibile se ne hanno una, altrimenti per prima
	 * apparizione: cosi' uno `StructureHit` — che `ApplyEnvironmentChanges` emette PRIMA delle attivazioni —
	 * non porta il suo intento davanti a uno con `IntentIndex` minore. Dentro il gruppo: attivazione, impronte,
	 * muri, colpi, ciascuno nell'ordine di timeline. Un evento senza `ActionId` (lo `StructureHit` aggregato,
	 * #3281) e' un atto proprio; un gruppo senza attivazione visibile (impatti di carica, contrattacchi, una
	 * sorgente nascosta) e' un atto proprio alla sua prima apparizione.
	 * `ArcHit` non entra (#3293).
	 *
	 * 🔑 `ViewerTeamId`: un'attivazione con `!SourceVerdict.AllowsTeam(ViewerTeamId)` non entra (D6); le sue
	 * impronte e i suoi colpi restano — il velo sul bersaglio e' di [D-223], non di questa funzione.
	 * 🔑 `FrozenPrefix`: i primi N elementi di `Previous` si riproducono VERBATIM (D-355); gli eventi non ancora
	 * sequenziati si raggruppano fra loro e si accodano. Con N = 0 e' la costruzione da zero.
	 */
	static TArray<FRTBlastSequenceElement> BuildBlastSequence(const TArray<FRTResolvedEvent>& Timeline,
		const TArray<FRTBlastSequenceElement>& Previous, int32 FrozenPrefix, int32 ViewerTeamId);
```

Nel commento di `PhaseDuration` (`:218-226`) sostituisci il paragrafo `⛔ **QUESTO WRAPPER NON CONOSCE NE I MURI NE LE IMPRONTE...` con:

```cpp
	 * ⛔ **QUESTO WRAPPER NON CONOSCE NE I MURI, NE LE IMPRONTE, NE LE ATTIVAZIONI** — tre zeri dichiarati
	 * (#2828, #3278, #<n>). Delega a `PhaseTime` con `NumActivations = 0` e `NumSequenceElements = NumAttacks`:
	 * su un `Blast` la sequenza vera e' piu' lunga dei soli colpi, e la durata restituita e' **sottostimata**.
```

- [ ] **Step 4: Le definizioni**

`RTPlaybackLibrary.cpp`, `BlastPhaseIsActive` (`:47-54`):

```cpp
bool URTPlaybackLibrary::BlastPhaseIsActive(int32 NumAttacks, bool bHasBlastMove, int32 NumFootprints,
	int32 NumStructureHits, int32 NumActivations)
{
	// Cinque ragioni indipendenti; la quinta e' quella di #<n>: un Blast di sole cure si vede. ⛔ Nessuna
	// somma e nessuna soglia: basta che UNA sia vera.
	return NumAttacks > 0 || bHasBlastMove || NumFootprints > 0 || NumStructureHits > 0 || NumActivations > 0;
}
```

`PhaseDuration` (`:59-68`, commento compreso — diceva «**DUE** zeri»):

```cpp
	// Una riga: la formula sta in `PhaseTime`, e il totale e' la somma dei suoi due termini. Non c'e' un
	// secondo calcolo da tenere allineato.
	// ⚠️ **TRE zeri, e nessuno e' una dimenticanza**: questo wrapper non conosce ne i colpi a struttura
	// (`#2828`), ne le impronte (`#3278`), ne le attivazioni (#<n>). Passa `NumActivations = 0` e usa i soli
	// colpi come sequenza, quindi su un `Blast` la durata restituita e' SOTTOSTIMATA. ⛔ Chi dimensiona il
	// playback vero non passa di qui — `PhaseTimeForPlaybackPhase` chiama `PhaseTime` con la sequenza intera.
	// Questa forma sopravvive per i gate di pacing sulle fasi classiche. ⏱️ *Fino a #<n> diceva «DUE zeri».*
	return PhaseTime(Phase, MaxMoveSegments, /*NumActivations=*/ 0, /*NumSequenceElements=*/ NumAttacks,
		CellsPerSecond, AttackShowSeconds, PhaseBeatSeconds).Total();
```

`PhaseTime` (`:71-133`) diventa:

```cpp
FRTPhaseTime URTPlaybackLibrary::PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumActivations,
	int32 NumSequenceElements, float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	const float MoveTime = (CellsPerSecond > 0.f)
		? (FMath::Max(0, MaxMoveSegments) / CellsPerSecond)
		: 0.f;
	// Il tempo delle attivazioni di Prep e Dash (#<n>): mostrato, quindi incomprimibile come i colpi.
	const float ActivationTime = FMath::Max(0, NumActivations) * FMath::Max(0.f, AttackShowSeconds);

	FRTPhaseTime Out;

	switch (Phase)
	{
	case ERTMatchPhase::Dash:
		// Prima le attivazioni, poi le rotte: `RouteAlpha` parte dopo il loro tempo.
		Out.Shown = ActivationTime + MoveTime;
		break;

	case ERTMatchPhase::Move:
		Out.Shown = MoveTime;
		break;

	case ERTMatchPhase::Blast:
	{
		// 🔴 **La SEQUENZA, non il `Max` fra canali** (#<n>, D5). I tre canali paralleli di #2828/#3278 sono
		// diventati una sequenza per intento, svelata un elemento per volta: la fase dura quanto la sequenza.
		// `Max(1, ...)`: un Blast di sola spinta si vede e non puo' durare zero.
		const float SequenceTime = FMath::Max(1, NumSequenceElements) * AttackShowSeconds;
		// `Max` con la spinta e non somma: i colpi si vedono MENTRE il bersaglio scivola. Tutto `Shown`, zero
		// `Slack` — vedi `FRTPhaseTime`.
		Out.Shown = FMath::Max(SequenceTime, MoveTime);
		break;
	}

	case ERTMatchPhase::Prep:
		// Le attivazioni si mostrano; il beat di oggi resta, ed e' l'unica parte che il budget puo' togliere.
		Out.Shown = ActivationTime;
		Out.Slack = PhaseBeatSeconds;
		break;

	default:
		Out.Slack = PhaseBeatSeconds;
		break;
	}

	return Out;
}
```

E in coda al file:

```cpp
namespace
{
	/** Il rango di un tipo DENTRO un atto: attivazione, impronte, muri, colpi (spec §2.4). -1 = non entra. */
	int32 RTRangoNellAtto(ERTResolvedEventType Type)
	{
		switch (Type)
		{
		case ERTResolvedEventType::AbilityActivated: return 0;
		case ERTResolvedEventType::AttackFootprint:  return 1;
		case ERTResolvedEventType::StructureHit:     return 2;
		case ERTResolvedEventType::Attack:           return 3;
		default:                                     return -1; // `ArcHit` compreso: #3293
		}
	}
}

TArray<FRTBlastSequenceElement> URTPlaybackLibrary::BuildBlastSequence(const TArray<FRTResolvedEvent>& Timeline,
	const TArray<FRTBlastSequenceElement>& Previous, int32 FrozenPrefix, int32 ViewerTeamId)
{
	TArray<FRTBlastSequenceElement> Out;

	// D-355: il prefisso gia' mostrato si riproduce VERBATIM. ⚠️ `TSet` solo per `Contains`.
	const int32 Congelati = FMath::Clamp(FrozenPrefix, 0, Previous.Num());
	TSet<int32> GiaInSequenza;
	for (int32 i = 0; i < Congelati; ++i)
	{
		Out.Add(Previous[i]);
		GiaInSequenza.Add(Previous[i].TimelineIndex);
	}

	struct FRTAttoInCostruzione
	{
		int32 Source = 0;
		FName ActionId;
		TArray<int32> Indici;
		int32 PrimaApparizione = INDEX_NONE;
		int32 IndiceAttivazione = INDEX_NONE; // l'attivazione VISIBILE del gruppo, se c'e'
		/** La chiave d'ordine fra gli atti (decisione (d)): l'attivazione se c'e', altrimenti la prima apparizione. */
		int32 Chiave() const { return IndiceAttivazione != INDEX_NONE ? IndiceAttivazione : PrimaApparizione; }
	};
	// Gli atti nascono nell'ordine di prima apparizione (la scansione va in avanti) e si RIORDINANO poi per
	// `Chiave()`: uno `StructureHit` emesso prima delle attivazioni non deve trascinare il suo intento davanti a
	// quelli con `IntentIndex` minore. ⚠️ Le chiavi sono indici di timeline distinti: l'ordine e' totale.
	TArray<FRTAttoInCostruzione> Atti;

	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		const FRTResolvedEvent& Ev = Timeline[i];
		if (Ev.Phase != ERTMatchPhase::Blast || RTRangoNellAtto(Ev.Type) < 0 || GiaInSequenza.Contains(i))
		{
			continue;
		}
		// D6: un'attivazione che chi guarda non ha il diritto di vedere non entra — tutto o niente.
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && !Ev.SourceVerdict.AllowsTeam(ViewerTeamId))
		{
			continue;
		}

		int32 Atto = INDEX_NONE;
		if (!Ev.ActionId.IsNone()) // senza identita' = atto proprio, mai fuso (#3281)
		{
			Atto = Atti.IndexOfByPredicate([&Ev](const FRTAttoInCostruzione& A)
			{
				return !A.ActionId.IsNone() && A.Source == Ev.SourceStableUnitId && A.ActionId == Ev.ActionId;
			});
		}
		if (Atto == INDEX_NONE)
		{
			FRTAttoInCostruzione& Nuovo = Atti.AddDefaulted_GetRef();
			Nuovo.Source = Ev.SourceStableUnitId;
			Nuovo.ActionId = Ev.ActionId;
			Nuovo.PrimaApparizione = i;
			Atto = Atti.Num() - 1;
		}
		Atti[Atto].Indici.Add(i);
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Atti[Atto].IndiceAttivazione == INDEX_NONE)
		{
			Atti[Atto].IndiceAttivazione = i; // solo le VISIBILI arrivano qui: le altre sono uscite sopra
		}
	}

	// Decisione (d): l'ordine fra gli atti e' quello delle loro attivazioni.
	Atti.StableSort([](const FRTAttoInCostruzione& X, const FRTAttoInCostruzione& Y) { return X.Chiave() < Y.Chiave(); });

	for (FRTAttoInCostruzione& A : Atti)
	{
		// Ordine TOTALE (rango, poi indice): nessuna dipendenza dalla stabilita' dell'algoritmo.
		A.Indici.StableSort([&Timeline](int32 X, int32 Y)
		{
			const int32 RX = RTRangoNellAtto(Timeline[X].Type);
			const int32 RY = RTRangoNellAtto(Timeline[Y].Type);
			return (RX != RY) ? (RX < RY) : (X < Y);
		});
		for (const int32 Indice : A.Indici)
		{
			FRTBlastSequenceElement E;
			E.TimelineIndex = Indice;
			E.SourceStableUnitId = Timeline[Indice].SourceStableUnitId;
			E.ActionId = Timeline[Indice].ActionId;
			Out.Add(E);
		}
	}
	return Out;
}
```

- [ ] **Step 5: I chiamanti — `RTTurnManager.cpp` (transitorio) e i test esistenti**

`RTTurnManager.cpp:7795-7796`:

```cpp
	if (URTPlaybackLibrary::BlastPhaseIsActive(PlaybackAttacks.Num(), bHasBlastMove,
		PlaybackFootprints.Num(), PlaybackStructureHits.Num(), /*NumActivations=*/ 0))
```

`RTTurnManager.cpp:8881-8883` — ⚠️ **transitorio, sostituito nel Task 7**: i tre canali restano paralleli fino ad allora, e la loro somma è una durata che li contiene tutti:

```cpp
	return URTPlaybackLibrary::PhaseTime(InPhase, MaxSeg, /*NumActivations=*/ 0,
		PlaybackAttacks.Num() + PlaybackStructureHits.Num() + PlaybackFootprints.Num(),
		PlaybackCellsPerSecond, AttackShowSeconds, PhaseBeatSeconds);
```

`RTPlaybackLibraryTests.cpp` — ogni chiamata a `BlastPhaseIsActive` (`:801-802`, `:807`, `:811`, `:813`, `:819`, `:848-849`, `:854`, `:859-861`) riceve `, 0` come quinto argomento.

`PhaseTimeSplitsShownFromSlack` (`:153-187`), le quattro chiamate diventano:

```cpp
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Move, /*MaxSeg*/ 4, /*Attivazioni*/ 0, /*Sequenza*/ 0,
			/*CellsPerSec*/ 2.f, 0.5f, 0.3f);
```
```cpp
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Prep, 0, 0, 0, 2.f, 0.5f, /*Beat*/ 0.3f);
```
```cpp
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Blast, /*MaxSeg*/ 1, /*Attivazioni*/ 0, /*Sequenza*/ 4,
			/*CellsPerSec*/ 2.f, 0.5f, 0.3f);
```
```cpp
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(
			ERTMatchPhase::Blast, /*MaxSeg*/ 6, /*Attivazioni*/ 0, /*Sequenza*/ 1,
			/*CellsPerSec*/ 2.f, 0.5f, 0.3f);
```

`:514`:

```cpp
			const float Durata = URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Move, S, 0, 0, V, 0.f, 0.f).Shown;
```

`BlastPhaseLastsForStructureHits` (`:884-929`) e `BlastPhaseLastsForFootprints` (`:949-992`): **i casi «`Max` e non SOMMA» encodano la decisione che D5 sostituisce**, e vanno riscritti, non cancellati. Aggiungi in testa a ciascun corpo:

```cpp
	// ⏱️ *Fino a #<n> questo gate pinnava il `Max` fra tre canali paralleli. D5 (spec «il momento») li ha
	// fatti diventare UNA sequenza per intento, svelata un elemento per volta: un muro e un colpo sono due
	// elementi, e la fase li dura entrambi. Il gate resta per la ragione per cui era nato — il canale deve
	// DIMENSIONARE la fase, non solo aprirla — con la formula nuova.*
```

e sostituisci le chiamate: ogni `PhaseTime(ERTMatchPhase::Blast, M, A, S, F, cps, ass, beat)` diventa `PhaseTime(ERTMatchPhase::Blast, M, 0, A + S + F, cps, ass, beat)` scritto con il numero, con questi attesi:

| Riga | Chiamata nuova | Atteso | Messaggio nuovo |
|---|---|---|---|
| `:892` | `(Blast, 0, 0, 4, 2.f, 0.5f, 0.3f)` | 2,0 | «quattro muri: quattro elementi, 2,0 s» |
| `:905` | `(Blast, 0, 0, 0, 2.f, 0.5f, 0.3f)` | 0,5 | invariato |
| `:914` | `(Blast, 0, 0, 5, 2.f, 0.5f, 0.3f)` | **2,5** | «tre colpi e due muri: cinque elementi in sequenza, 2,5 s (D5)» |
| `:922` | `(Blast, 0, 0, 5, 2.f, 0.5f, 0.3f)` | **2,5** | «quattro colpi e un muro: 2,5 s» |
| `:957` | `(Blast, 0, 0, 4, 2.f, 0.5f, 0.3f)` | 2,0 | «quattro impronte: 2,0 s» |
| `:968` | `(Blast, 0, 0, 0, 2.f, 0.5f, 0.3f)` | 0,5 | invariato |
| `:977` | `(Blast, 0, 0, 9, 2.f, 0.5f, 0.3f)` | **4,5** | «due colpi, tre muri e quattro impronte: nove elementi, 4,5 s (D5)» |
| `:985` | `(Blast, 0, 0, 6, 2.f, 0.5f, 0.3f)` | **3,0** | «cinque colpi e un'impronta: 3,0 s» |

`EveryChannelIsFullyRevealedByPhaseEnd` (`:1018-1084`): la chiamata di `:1051-1052` diventa

```cpp
		const int32 Sequenza = C.Attacks + C.Strutture + C.Impronte; // D5: un elemento per fatto, in serie
		const FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(ERTMatchPhase::Blast, C.MaxSeg,
			/*NumActivations=*/ 0, Sequenza, CellsPerSec, ShowSeconds, BeatSeconds);
```

e i tre asserti per canale (`:1063-1068`) diventano uno:

```cpp
		TestEqual(FString::Printf(TEXT("%s: la SEQUENZA e' tutta rivelata a fine fase"), C.Nome),
			URTPlaybackLibrary::AttacksToShow(Sequenza, PhaseDur, ShowSeconds), Sequenza);
```

(l'anti-vacuità di `:1077-1078` resta com'è).

I commenti dei test riscritti dicono il contrario della formula nuova e vanno riscritti con loro:

- `:911-912` (`⛔ **\`Max\` e non SOMMA**: i due canali si rivelano in parallelo…`) diventa:
  ```cpp
  	// 🔴 **SOMMA e non `Max`, da #<n>** (D5): colpi e muri sono elementi della STESSA sequenza, svelati uno
  	// dopo l'altro. ⏱️ *Fino a #<n> questa riga diceva «`Max` e non SOMMA: i due canali si rivelano in
  	// parallelo».* Tre colpi e due muri sono cinque elementi.
  ```
- `:974-975` (`⛔ **\`Max\` e non SOMMA, su TRE canali.**…`) diventa:
  ```cpp
  	// 🔴 **SOMMA, su tre tipi di elemento** (#<n>, D5): due colpi, tre muri e quattro impronte sono nove
  	// elementi in sequenza. ⏱️ *Diceva «`Max` e non SOMMA, su TRE canali».*
  ```
- La docstring di `EveryChannelIsFullyRevealedByPhaseEnd`, `:1004-1007` (`La durata della fase e' Max(AttackTime, MoveTime) con AttackTime = Max(1, maxCanale) * AttackShowSeconds…`), diventa:
  ```cpp
   * 🔑 **Perche' e' diventato inutile, e chi lo ha reso tale.** La durata della fase e'
   * `Max(SequenceTime, MoveTime)` con `SequenceTime = Max(1, N) * AttackShowSeconds`, dove `N` e' la
   * lunghezza della sequenza per intento (#<n>, D5: attivazioni, impronte, muri, colpi). ∴
   * `PhaseDur >= N * ASS`, e `AttacksToShow` a quel punto vale `Min(N, 1 + floor(PhaseDur / ASS)) = N`.
   * ⏱️ *Fino a #<n> erano tre canali paralleli e `maxCanale`; prima di #2828 e #3278 un Blast di soli muri
   * o di sole impronte durava UN intervallo.*
  ```
  Il nome del test resta: un rename spezzerebbe i riferimenti in `RTTurnManager.cpp` e nel registro.

- [ ] **Step 6: Build e verde**

Run: build; test `RefactorTactics.Playback`.
Expected: verdi. ⚠️ Se un test di pacing su un turno **reale** cade perché la durata del `Blast` è cresciuta (la somma transitoria di Step 5), leggi il valore pinnato: se è la conseguenza dichiarata dalla spec §6 (*«la sequenza per intento allunga il Blast»*), aggiorna l'atteso con una riga `⏱️` che cita D5 — mai allargarlo a un intervallo. Qualunque altro rosso è un difetto: fermati.

- [ ] **Step 7: Mutazione (2) — raggruppare per tipo**

In `BuildBlastSequence`, sostituisci temporaneamente il blocco `int32 Atto = INDEX_NONE; ... Atti[Atto].Indici.Add(i);` con:

```cpp
		if (Atti.Num() == 0) { Atti.AddDefaulted(); Atti[0].PrimaApparizione = i; } // MUTAZIONE: un solo atto, ordinato per tipo
		const int32 Atto = 0;
		Atti[Atto].Indici.Add(i);
```

(il blocco `if (Ev.Type == ERTResolvedEventType::AbilityActivated && ...)` che segue resta com'è e compila sull'`Atto` della mutazione).

Build, test `RefactorTactics.Playback.BlastSequenceIsOrderedPerIntent`. Expected: `Result={Fail}` sull'asserto d'ordine (l'ordine diventa `1 2 3 4 0 5 6 7 8 11`). Ripristina, build, verde.

Mutazione (d) — l'ordine fra gli atti: sostituisci il corpo di `Chiave()` con `return PrimaApparizione;`, build, stesso test. Expected: `Result={Fail}`, con l'ordine `2 4 0 6 1 3 5 7 8 11` — l'atto di B davanti ad A per il suo muro. Ripristina, build, verde. (`BlastSequencePrefixIsStableUnderExtension` resta verde in entrambe le forme: lì nessun muro precede un'attivazione.)

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp
git commit -F - <<'EOF'
feat(<n>): BuildBlastSequence per intento, BlastPhaseIsActive col quinto termine, PhaseTime su attivazioni e sequenza

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 6: Il confine d'atto su `(SourceStableUnitId, ActionId)`

*Spezzato dal Task 7 perché è puro e rivedibile da solo: cambia una regola con una sola implementazione (`ActBoundaryRuleHasOneImplementation`) e i suoi tre consumatori in `RTTurnManager.cpp`, senza toccare la forma del playback.*

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.h:408-491` (commenti e firma di `IsActBoundary`)
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp:290-349`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h:3359-3391` (due membri nuovi)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:7891`, `:7927-7929`, `:7968-7970`, `:7995`, `:8052`, `:8134`, `:8149`, `:8498-8500`, `:8673-8674`
- Test: `Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp:704-755`, `Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp` (in coda)

**Interfaces:**
- Produces:
  - `UFUNCTION(BlueprintPure) static bool URTPlaybackLibrary::IsActBoundary(const FRTResolvedEvent& Event, FName CurrentAction, int32 CurrentSource = INDEX_NONE);` — ⚠️ **il parametro nuovo è IN CODA e ha un default**: la funzione è `BlueprintPure`, e un nodo Blueprint che la chiama non si trova con `grep` (i `.uasset` sono binari). Con il default `INDEX_NONE` un chiamante che non passa la sorgente ottiene il criterio storico (solo `ActionId`); ogni chiamante C++ di questo piano la passa esplicitamente.
  - `int32 ARTTurnManager::PlaybackStopFromSource = 0;` e `int32 ARTTurnManager::PlaybackLastShownSource = 0;` (privati, accanto ai due `FName` omologhi).

- [ ] **Step 1: Il test puro rosso**

In coda a `RTPlaybackLibraryTests.cpp`:

```cpp
/**
 * Due unita' con la stessa azione generica sono DUE atti — `Ruling` di spec §2.4 (#<n>).
 * ⏱️ *Era il «limite noto» di #2855: il criterio guardava l'`ActionId` da solo.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActBoundaryDistinguishesSourceTest,
	"RefactorTactics.Playback.ActBoundaryDistinguishesTheSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActBoundaryDistinguishesSourceTest::RunTest(const FString&)
{
	FRTResolvedEvent CuraDiDue = SeqEvento(ERTResolvedEventType::AbilityActivated, 2, TEXT("Action.Heal"));
	TestTrue(TEXT("🔴 stessa azione, sorgente diversa: confine"),
		URTPlaybackLibrary::IsActBoundary(CuraDiDue, FName(TEXT("Action.Heal")), /*CurrentSource*/ 1));
	TestFalse(TEXT("stessa azione, stessa sorgente: stesso atto"),
		URTPlaybackLibrary::IsActBoundary(CuraDiDue, FName(TEXT("Action.Heal")), /*CurrentSource*/ 2));
	TestTrue(TEXT("azione diversa: confine, come prima"),
		URTPlaybackLibrary::IsActBoundary(CuraDiDue, FName(TEXT("Action.Shield")), 2));
	// ⚠️ Il default `INDEX_NONE` e' il criterio STORICO, per i nodi Blueprint che non passano la sorgente.
	TestFalse(TEXT("senza sorgente (default): solo l'ActionId, come prima di #<n>"),
		URTPlaybackLibrary::IsActBoundary(CuraDiDue, FName(TEXT("Action.Heal"))));

	TArray<FRTResolvedEvent> T;
	T.Add(SeqEvento(ERTResolvedEventType::AbilityActivated, 1, TEXT("Action.Heal")));
	T.Add(CuraDiDue);
	TestEqual(TEXT("NextActionBoundary si ferma sulla seconda cura"), URTPlaybackLibrary::NextActionBoundary(T, 0), 1);
	return true;
}
```

- [ ] **Step 2: Build e rosso**

Run: build. Expected: errore di compilazione — `IsActBoundary` non accetta tre argomenti.

- [ ] **Step 3: La regola**

`RTPlaybackLibrary.h:490-491`:

```cpp
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static bool IsActBoundary(const FRTResolvedEvent& Event, FName CurrentAction, int32 CurrentSource = INDEX_NONE);
```

⚠️ **In coda e con default, non in testa**: è `BlueprintPure`, e i nodi Blueprint che la chiamano non si possono cercare. Aggiungi al commento, prima di `UFUNCTION`:

```cpp
	 * ⚠️ **`CurrentSource` e' IN CODA e vale `INDEX_NONE` per default** (#<n>): un nodo Blueprint scritto prima
	 * di #<n> continua a compilare e ottiene il criterio storico, l'`ActionId` da solo. Ogni chiamante C++
	 * passa la sorgente.
```

Nel commento di `IsActBoundary`, il paragrafo `🔑 **Il criterio resta quello di #2855: l'ActionId da solo.**` (`:474-477`) diventa:

```cpp
	 * 🔑 **Il criterio e' la coppia `(SourceStableUnitId, ActionId)`** (#<n>, `Ruling` della spec «il
	 * momento» §2.4). ⏱️ *Fino a #<n> era l'`ActionId` da solo, come #2855 lo scriveva.* Un evento e' un confine
	 * quando porta un'azione diversa da quella in corso **o** la porta un'altra unita': due cure consecutive da
	 * due unita' sono due atti. Piu' eventi con la stessa coppia sono **un** atto — impronta e colpi dello
	 * stesso intento non fanno fermare due volte.
```

e nel commento di `NextActionBoundary` il paragrafo `🔴 **Limite noto, dichiarato e non aggirato**` (`:451-456`) diventa:

```cpp
	 * ✅ **Il limite noto di #2855 e' chiuso** (#<n>): due unita' con la stessa azione generica sono due atti,
	 * perche' l'atto in corso e' la coppia `(SourceStableUnitId, ActionId)` dell'ultimo evento con un'azione.
```

`RTPlaybackLibrary.cpp`, `NextActionBoundary` (`:304-322`):

```cpp
	FName Corrente = NAME_None;
	int32 SorgenteCorrente = 0;
	for (int32 i = FMath::Min(FromIndex, Fine - 1); i >= 0; --i)
	{
		if (!Timeline[i].ActionId.IsNone())
		{
			Corrente = Timeline[i].ActionId;
			SorgenteCorrente = Timeline[i].SourceStableUnitId;
			break;
		}
	}

	for (int32 i = FMath::Max(0, FromIndex + 1); i < Fine; ++i)
	{
		if (IsActBoundary(Timeline[i], Corrente, SorgenteCorrente))
		{
			return i;
		}
	}
```

`IsActBoundary` (`:330-349`): firma come sopra (`FName CurrentAction, int32 CurrentSource`); l'ultima riga diventa

```cpp
	// #<n>: la COPPIA. Stessa azione da un'altra unita' e' un altro atto (spec «il momento» §2.4).
	// `INDEX_NONE` = sorgente non dichiarata (il default per i nodi Blueprint): il criterio storico.
	return Event.ActionId != CurrentAction
		|| (CurrentSource != INDEX_NONE && Event.SourceStableUnitId != CurrentSource);
```

- [ ] **Step 4: I consumatori in `RTTurnManager`**

`RTTurnManager.h`, dopo `FName PlaybackStopFromAction;` (`:3372`):

```cpp
	/** La sorgente dell'atto in corso all'armamento: con `PlaybackStopFromAction` e' la COPPIA del confine (#<n>). */
	int32 PlaybackStopFromSource = 0;
```

dopo `FName PlaybackLastShownAction;` (`:3391`):

```cpp
	/** La sorgente dell'ultimo fatto MOSTRATO con un'azione: con `PlaybackLastShownAction` e' l'atto in corso (#<n>). */
	int32 PlaybackLastShownSource = 0;
```

`RTTurnManager.cpp` — ogni sito che scrive l'`FName` scrive anche l'intero, nella stessa riga:

- `:7891`, `:8052`, `:8134`: `PlaybackStopFromAction = NAME_None;` → `PlaybackStopFromAction = NAME_None; PlaybackStopFromSource = 0;`
- `:7995`: `PlaybackLastShownAction = NAME_None;` → `PlaybackLastShownAction = NAME_None; PlaybackLastShownSource = 0;`
- `:8149`: `PlaybackStopFromAction = PlaybackLastShownAction;` → `PlaybackStopFromAction = PlaybackLastShownAction; PlaybackStopFromSource = PlaybackLastShownSource;`
- `:8673-8674`: aggiungi `PlaybackStopFromSource = 0;` e `PlaybackLastShownSource = 0;`
- `:7927-7929` (e identici `:7968-7970` con `Colpo`, `:8498-8500` con `Atk`):

```cpp
		if (!Footprint.ActionId.IsNone())
		{
			PlaybackLastShownAction = Footprint.ActionId;
			PlaybackLastShownSource = Footprint.SourceStableUnitId;
		}
		if (PlaybackStopAt == ERTPlaybackStopAt::NextAction
			&& URTPlaybackLibrary::IsActBoundary(Footprint, PlaybackStopFromAction, PlaybackStopFromSource))
```

Verifica che non resti un sito: `grep -n "PlaybackStopFromAction\|PlaybackLastShownAction" Source/RefactorTactics/Turn/RTTurnManager.cpp` — ogni riga che assegna l'`FName` deve avere accanto l'assegnazione dell'intero.

- [ ] **Step 5: Il gate «una sola implementazione» con la sorgente**

`RTPlaybackStopPredicateTests.cpp:704-755`: la lambda `Evento` guadagna la sorgente e la scansione all'indietro la porta:

```cpp
	auto Evento = [](ERTResolvedEventType Type, const TCHAR* Azione, int32 Sorgente = 0)
	{
		FRTResolvedEvent Ev;
		Ev.Type = Type;
		Ev.ActionId = (Azione != nullptr) ? FName(Azione) : NAME_None;
		Ev.SourceStableUnitId = Sorgente;
		return Ev;
	};
```

Aggiungi in coda alla timeline (dopo `ArcHit`, `:721`):

```cpp
	Timeline.Add(Evento(ERTResolvedEventType::AbilityActivated, TEXT("Action.Heal"), 1)); // atto nuovo
	Timeline.Add(Evento(ERTResolvedEventType::AbilityActivated, TEXT("Action.Heal"), 2)); // #<n>: altra sorgente = confine
```

Il ciclo `:731-737`:

```cpp
		FName Corrente = NAME_None;
		int32 SorgenteCorrente = 0;
		for (int32 k = i - 1; k >= 0; --k)
		{
			if (!Timeline[k].ActionId.IsNone())
			{
				Corrente = Timeline[k].ActionId;
				SorgenteCorrente = Timeline[k].SourceStableUnitId;
				break;
			}
		}

		const bool bPredicato = URTPlaybackLibrary::IsActBoundary(Timeline[i], Corrente, SorgenteCorrente);
```

Le due chiamate dirette `:751-755` restano come sono: col default `INDEX_NONE` misurano il criterio storico, che su `StructureHit`/`ArcHit` senza azione è identico.

- [ ] **Step 6: Build e verde**

Run: build; test `RefactorTactics.Playback+RefactorTactics.Actions`.
Expected: verdi. ⚠️ `RTEnvironmentActionTests.cpp:3760`, `:3834`, `:3898` chiamano `NextActionBoundary` su timeline vere con `StructureHit` letti da due lati: se uno cade, guarda le sorgenti dei due eventi — un `StructureHit` aggregato con sorgenti diverse è un confine **nuovo** e voluto dal `Ruling`; aggiorna l'atteso con una riga `⏱️` solo se è quello il caso.

- [ ] **Step 7: Mutazione (5), forma pura**

Riporta l'ultima riga di `IsActBoundary` a `return Event.ActionId != CurrentAction;`, build, test `RefactorTactics.Playback.ActBoundaryDistinguishesTheSource`. Expected: `Result={Fail}` su «stessa azione, sorgente diversa: confine». Ripristina, build, verde. (La forma su un turno vero è il Task 7, Step 9.)

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp
git commit -F - <<'EOF'
feat(<n>): il confine d'atto e' la coppia (sorgente, azione); chiuso il limite noto di #2855

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 7: Il playback — code di Prep e Dash, sequenza di Blast, privacy, D-355

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h:2335-2355` (dichiarazioni), `:3293-3349` (membri), accessore di test accanto a `ResolvedTimelineForTest` (`:634`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:7672-7800` (`BeginPlayback`), `:7896-7977` (le due `Reveal*` sostituite), `:7982-7991` (`EnterPlaybackPhase`), `:8094-8119` (`StepMicroStep`), `:8275-8282` (alpha del Dash), `:8418-8506` (Blast), `:8521-8562` (catch-all), `:8869-8884` (`PhaseTimeForPlaybackPhase`)
- Test: `Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp` (nuovo), `Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp:604-680`

**Interfaces:**
- Consumes: `ShowActivation` (Task 4), `BuildBlastSequence`, `BlastPhaseIsActive`, `PhaseTime` (Task 5), `IsActBoundary` a coppia e i due membri `*Source` (Task 6), `ARTUnit::CastCuesPlayedForTest` (Task 1), `RTWorldFixtures::MakePlayerOnTeam(UWorld*, int32)` (`Tests/RTWorldFixtures.h:142`), `ARTTurnManager::RefreshTeamKnowledgeNow()` (`RTTurnManager.h:1024`).
- Produces:
  - membri privati: `TArray<FRTResolvedEvent> PlaybackActivationsPrep, PlaybackActivationsDash; TArray<FRTBlastSequenceElement> PlaybackBlastSequence; int32 ActivationsShown = 0; int32 BlastShown = 0;` — rimossi `PlaybackAttacks`, `PlaybackFootprints`, `PlaybackStructureHits`, `AttacksShown`, `FootprintsShown`, `StructureHitsShown`, `RevealPlaybackFootprints`, `RevealPlaybackStructureHits`.
  - privati: `bool RevealPlaybackActivations(const TArray<FRTResolvedEvent>& Activations, int32 UpTo); bool RevealBlastSequence(int32 UpTo); bool NotePlaybackActShown(const FRTResolvedEvent& Ev); void ShowPlaybackAttack(const FRTResolvedEvent& Atk); float PlaybackActivationLeadSeconds(ERTMatchPhase InPhase) const;`
  - pubblico: `int32 PlaybackActivationsQueuedForTest() const;` — attivazioni nelle code di Prep e Dash più gli elementi `AbilityActivated` della sequenza.
  - pubblico: `int32 PlaybackAnimCellIndexForTest(const ARTUnit* Unit) const;` — l'indice di cella dell'anim di `Unit` nella fase di playback corrente (`PlaybackAnimCellIndex`, privato, `RTTurnManager.h:2752`), `INDEX_NONE` se l'unità non ha un'anim in questa fase.
  - pubblico: `int32 PlaybackBlastShownForTest() const;` — `BlastShown`, il prefisso già mostrato della sequenza.
  - pubblico: `TArray<int32> PlaybackBlastSequenceIndicesForTest() const;` — i `TimelineIndex` di `PlaybackBlastSequence`, in ordine.
  - Già esistenti e usati dai test: `ARTUnit::bIsMovingVisually` (`UPROPERTY` pubblica, `Unit/RTUnit.h:1122`), `ARTTurnManager::GetRecentEventsForTeam(int32) const` → `TArray<FString>` (`RTTurnManager.h:912`), `KnowledgeForTeamPublic(int32) const` (`:1053`), `ExpireReactionWindow()` (`:1994`), `SetStartPlaybackPaused(bool)`, `StepMicroStep()`, `GetPlaybackPhaseName()`.

- [ ] **Step 0: Misura — ogni `Attack` della timeline è di fase `Blast`?**

`BuildBlastSequence` prende solo `Phase == Blast`, mentre il vecchio `BeginPlayback` metteva in `PlaybackAttacks` **ogni** `Attack` senza guardare la fase (`RTTurnManager.cpp:7741-7744`). Se un `Attack` nascesse in un'altra fase, il filtro lo farebbe sparire dal playback senza errore.

```bash
grep -n "ERTResolvedEventType::Attack" Source/RefactorTactics/Turn/*.cpp
```

Per ogni riga che **assegna** `Ev.Type = ERTResolvedEventType::Attack` (non i confronti), leggi l'`Ev.Phase` assegnato nello stesso blocco. In pianificazione il sito era uno solo, `RTTurnManager.cpp:6259`, con `Ev.Phase = ERTMatchPhase::Blast` a `:6258`. Rimisura sul tuo commit.
Expected: tutte le emissioni sono `Blast`. ⛔ **Se una è in un'altra fase**, fermati: è un `IMPLEMENTATION DRIFT`, e il filtro di fase di `BuildBlastSequence` va deciso — con l'autore — **prima** di scrivere il codice di questo task. Annota l'esito della misura nel commit del task.

- [ ] **Step 1: I test rossi**

Crea `Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp`:

```cpp
// IL BEAT DI ATTIVAZIONE NEL PLAYBACK (#<n>, spec «il momento» §2.4-§2.5).
//
// 🔑 **Il meccanismo, non il disegno**: headless nessun `AnimInstance` esiste, quindi si conta la cue CHIAMATA
// (`ARTUnit::CastCuesPlayedForTest`) e la riga `Attiva:` del feed filtrato per squadra.
// ⚠️ Il viewer e' `RTWorldFixtures::MakePlayerOnTeam`: uno `SpawnActor<ARTPlayerController>` nudo non ha un
// `ARTPlayerState` e `TeamIdOf` ripiega su 0 (`Player/RTPlayerState.cpp:5-12`).
// ⚠️ La squadra «che non vede» e' una squadra SENZA unita' in campo: fuori da `TeamKnowledgeState`, quindi
// fail-closed. Esercita il FILTRO; la percezione vera (un muro alto) e' Review Focus 5.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Perception/RTTeamKnowledge.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FRTBeatDiProva
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Scudo = nullptr;    // Prep: TideGuard
		ARTUnit* Tiratore = nullptr; // Blast: ImpactShot
	};

	ARTUnit* SpawnBeatUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Due attivazioni della squadra 0 — una di Prep, una di Blast — e un viewer della squadra data. */
	bool CostruisciBeat(FAutomationTestBase& Test, int32 SquadraDelViewer, FRTBeatDiProva& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), Out.World)) { return false; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTHexMapActor* MapActor = Out.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		Out.Scudo    = SpawnBeatUnit(Out.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 0));
		Out.Tiratore = SpawnBeatUnit(Out.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnBeatUnit(Out.World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0));
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.TM || !Out.Scudo || !Out.Tiratore || !Bersaglio) { return false; }
		if (!Test.TestNotNull(TEXT("viewer"), RTWorldFixtures::MakePlayerOnTeam(Out.World, SquadraDelViewer))) { return false; }

		for (int32 i = 0; i < Out.Scudo->NumAbilities(); ++i)
		{
			if (Out.Scudo->GetAbility(i) && Out.Scudo->GetAbility(i)->Def.ActionId == FName(TEXT("Hero.Muiren.TideGuard")))
			{
				Out.Scudo->PlannedAbilityIndex = i;
			}
		}
		Out.Scudo->PlannedCell = Out.Scudo->Cell;
		Out.Tiratore->PlannedAbilityIndex = 0;
		Out.Tiratore->PlannedAttackTarget = Bersaglio;
		Out.Tiratore->PlannedCell = Out.Tiratore->Cell;

		Out.TM->RefreshTeamKnowledgeNow(); // la precondizione della Prep (spec §2.5)
		return true;
	}

	void FinoAllaFineDelPlayback(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	bool HaRigaAttiva(const ARTTurnManager* TM, int32 Squadra)
	{
		return TM->GetRecentEventsForTeam(Squadra).ContainsByPredicate(
			[](const FString& Riga) { return Riga.StartsWith(TEXT("Attiva:")); });
	}
}

/**
 * Un'attivazione visibile SUONA la cue `Cast` sulla sorgente e scrive la riga `Attiva:` — spec §5.1, I8.
 * ⛔ Con la sorgente non osservata dal viewer: zero cue, nessuna riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPlaysTheCastCueTest,
	"RefactorTactics.Playback.ActivationPlaysTheCastCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPlaysTheCastCueTest::RunTest(const FString&)
{
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		FinoAllaFineDelPlayback(B.TM);
		TestEqual(TEXT("🔴 la cue Cast e' suonata sul tiratore, una volta"), B.Tiratore->CastCuesPlayedForTest(), 1);
		TestEqual(TEXT("e sullo scudo, nella Prep"), B.Scudo->CastCuesPlayedForTest(), 1);
		TestTrue(TEXT("la riga Attiva: e' nel feed della squadra"), HaRigaAttiva(B.TM, 0));
	}
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 7, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		FinoAllaFineDelPlayback(B.TM);
		TestEqual(TEXT("⛔ sorgente non osservata: nessuna cue"), B.Tiratore->CastCuesPlayedForTest(), 0);
		TestFalse(TEXT("⛔ e nessuna riga Attiva: per quella squadra"), HaRigaAttiva(B.TM, 7));
	}
	return true;
}

/**
 * Una sorgente non osservata non entra nelle code di playback — D6, spec §2.5. Tutto o niente.
 * ⛔ Controllo positivo: osservata, entrano entrambe (Prep e Blast).
 * ✅ Validato per mutazione: togliere il filtro in `BeginPlayback` (Prep/Dash) o in `BuildBlastSequence`
 * (Blast) fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackHiddenSourceHasNoBeatTest,
	"RefactorTactics.Playback.HiddenSourceHasNoActivationBeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackHiddenSourceHasNoBeatTest::RunTest(const FString&)
{
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 7, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		TestEqual(TEXT("🔴 non osservata: nessuna attivazione in coda"), B.TM->PlaybackActivationsQueuedForTest(), 0);
		// Il colpo resta: il velo sul bersaglio e' di [D-223], non di questa feature.
		TestTrue(TEXT("e la timeline canonica le conserva"), B.TM->ResolvedEventCountOfTypeForTest(
			ERTResolvedEventType::AbilityActivated) >= 2);
	}
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		B.TM->LockInAndResolve();
		TestEqual(TEXT("✅ osservata: Prep e Blast in coda"), B.TM->PlaybackActivationsQueuedForTest(), 2);
	}
	return true;
}

namespace
{
	/** L'indice nel kit dell'abilita' con questo `ActionId`. Nome distinto per l'unity build. */
	int32 BeatIndiceAbilita(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			if (U->GetAbility(i) && U->GetAbility(i)->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}

	URTHexMapAsset* BeatMappa(UWorld* World, int32 Raggio)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Raggio);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;
		return M;
	}
}

/**
 * Il prefisso gia' mostrato della sequenza sopravvive a un Blast SOSPESO davvero — D-355 (Review Focus 3).
 *
 * 🔑 Il montaggio e' quello di `Tests/RTDefensiveReactionTests.cpp:985-1047`: un `Brace` con
 * `Profile.Sidestep` contro un `Push` apre la finestra, la resolution si ferma a meta' Blast e il playback
 * parte sulla timeline PARZIALE; `ExpireReactionWindow` la completa e `BeginPlayback(true)` ricostruisce la
 * sequenza. Il test puro (`BlastSequencePrefixIsStableUnderExtension`) prova la funzione; questo prova che il
 * TurnManager le passa il prefisso giusto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPrefixSurvivesSuspensionTest,
	"RefactorTactics.Playback.ActivationPrefixSurvivesASuspendedBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPrefixSurvivesSuspensionTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 6);

	ARTUnit* Bracer = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bracer || !Pusher || !RTWorldFixtures::MakePlayerOnTeam(World, 0)) { return false; }

	Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
	Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
	Bracer->PlannedCell = Bracer->Cell;
	Pusher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3);
	Pusher->PlannedAttackTarget = Bracer;
	Pusher->PlannedCell = Pusher->Cell;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: la resolution e' sospesa sulla finestra del Brace"), TM->IsResolutionSuspended()))
	{
		return false;
	}

	// ⛔ **Premessa esplicita: durante la sospensione il playback e' IN CORSO su una timeline PARZIALE**, e la
	// sua sequenza di Blast non e' vuota — e' il prefisso che l'estensione deve conservare. ⚠️ Se questa
	// premessa cade, il MONTAGGIO va rifatto prima di toccare il codice (per esempio il playback non parte
	// durante la finestra, o il Push non e' visibile al viewer): l'invariante resta coperta dal test puro del
	// Task 5, `BlastSequencePrefixIsStableUnderExtension`, e questo test non deve essere allentato per passare.
	if (!TestTrue(TEXT("⛔ premessa: il playback e' in corso durante la sospensione"), TM->IsResolving())
		|| !TestTrue(TEXT("⛔ premessa: la sequenza di Blast della timeline parziale non e' vuota"),
			TM->PlaybackBlastSequenceIndicesForTest().Num() > 0))
	{
		return false;
	}

	// Il playback parziale scorre fino a mostrare almeno un elemento del Blast: e' il prefisso da congelare.
	for (int32 I = 0; I < 200 && TM->IsResolving() && TM->PlaybackBlastShownForTest() == 0; ++I)
	{
		TM->Tick(0.05f);
	}
	const int32 Mostrati = TM->PlaybackBlastShownForTest();
	if (!TestTrue(TEXT("⛔ premessa: un prefisso del Blast e' stato mostrato prima della ripresa"), Mostrati > 0))
	{
		return false;
	}
	const TArray<int32> Prima = TM->PlaybackBlastSequenceIndicesForTest();

	for (int32 Scadenze = 0; Scadenze < 8 && TM->IsResolutionSuspended(); ++Scadenze)
	{
		TM->ExpireReactionWindow();
	}
	if (!TestFalse(TEXT("⛔ premessa: la finestra e' chiusa e il turno ripreso"), TM->IsResolutionSuspended())) { return false; }
	TM->Tick(0.01f); // l'estensione e' gia' avvenuta; un tick breve non tocca il prefisso, che e' gia' mostrato

	const TArray<int32> Dopo = TM->PlaybackBlastSequenceIndicesForTest();
	if (!TestTrue(TEXT("la sequenza estesa non e' piu' corta del prefisso"), Dopo.Num() >= Mostrati)) { return false; }
	for (int32 i = 0; i < Mostrati; ++i)
	{
		TestEqual(*FString::Printf(TEXT("🔴 l'elemento %d del prefisso mostrato e' lo stesso dopo l'estensione"), i),
			Dopo[i], Prima[i]);
	}
	return true;
}

/**
 * Nel Dash lo `Step` atterra sulle celle DOPO le attivazioni, e durante l'anticipo lo scattatore NON corre
 * sul posto (Review Focus 4; la classe di difetti di #3519).
 *
 * 🔑 Il playback parte in pausa all'inizio del Dash: l'anticipo dura un `AttackShowSeconds` (una attivazione).
 * ✅ Validato per mutazione: con `Anticipo = 0.f` in `StepMicroStep` il primo `Step` cade dentro l'anticipo e
 * l'indice di cella resta 0; togliendo la condizione `bInAnticipo` da `bInCorsa` il flag di corsa e' acceso
 * durante l'anticipo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackDashStepAfterActivationsTest,
	"RefactorTactics.Playback.DashStepLandsOnCellsAfterTheActivations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackDashStepAfterActivationsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 8);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Caricatore || !Bersaglio || !RTWorldFixtures::MakePlayerOnTeam(World, 0)) { return false; }

	Caricatore->PlannedDashAbility = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;
	if (!TestTrue(TEXT("premessa: Branth ha Ram"), Caricatore->PlannedDashAbility != INDEX_NONE)) { return false; }

	TM->RefreshTeamKnowledgeNow();
	TM->SetPlaybackControlsEnabled(true);
	TM->SetStartPlaybackPaused(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il playback e' fermo all'inizio del Dash"),
		TM->IsResolving() && TM->IsPlaybackPaused() && TM->GetPlaybackPhaseName() == TEXT("Dash")))
	{
		return false;
	}

	// --- Durante l'anticipo: il cast suona, il cilindro e' fermo e NON corre ---------------------------
	TestFalse(TEXT("🔴 all'ingresso del Dash con un'attivazione la corsa non e' accesa"), Caricatore->bIsMovingVisually);
	// ⛔ Premessa del tick qui sotto: 0,1 s cade DENTRO l'anticipo solo se la cadenza e' piu' lunga. Il valore
	// e' la `UPROPERTY` `ARTTurnManager::AttackShowSeconds` (`RTTurnManager.h:1263-1265`, default 0,50).
	if (!TestTrue(TEXT("AttackShowSeconds > 0.1"), TM->AttackShowSeconds > 0.1f)) { return false; }
	TM->ResumePlayback();
	TM->Tick(0.1f); // dentro l'anticipo: 0,1 s contro un AttackShowSeconds
	TestFalse(TEXT("🔴 durante l'anticipo lo scattatore non corre sul posto"), Caricatore->bIsMovingVisually);
	TestEqual(TEXT("e resta sulla cella di partenza"), TM->PlaybackAnimCellIndexForTest(Caricatore), 0);
	TestEqual(TEXT("il cast e' gia' suonato"), Caricatore->CastCuesPlayedForTest(), 1);

	// --- Il primo Step dopo l'anticipo: cella 1, e ora corre ---------------------------------------------
	TM->PausePlayback();
	TM->StepMicroStep();
	for (int32 I = 0; I < 200 && TM->IsResolving() && !TM->IsPlaybackPaused(); ++I)
	{
		TM->Tick(0.02f);
	}
	TestEqual(TEXT("🔴 il primo Step atterra sulla PRIMA cella della rotta"), TM->PlaybackAnimCellIndexForTest(Caricatore), 1);
	TestTrue(TEXT("e a rotta iniziata la corsa e' accesa"), Caricatore->bIsMovingVisually);
	return true;
}

/**
 * Una sorgente nemica DAVVERO fuori vista — dietro un muro alto, a distanza 3 — non ha beat (Review Focus 5).
 *
 * 🔑 I test con la «squadra senza unita'» esercitano il FILTRO; questo esercita la PERCEZIONE: la squadra del
 * viewer e' in campo, ma il muro le nega la linea di vista sulla sorgente e la consapevolezza a 360° vale entro
 * 2 celle. Il tiro e' bloccato e SI attiva (`Ruling`), ma il viewer non lo vede.
 * ⛔ Controllo positivo: lo stesso turno visto dalla squadra della sorgente ha il beat.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEnemyBehindHighCoverTest,
	"RefactorTactics.Playback.EnemyBehindHighCoverHasNoActivationBeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEnemyBehindHighCoverTest::RunTest(const FString&)
{
	auto GiraIlTurno = [this](int32 SquadraDelViewer, int32& OutCue, bool& OutVisibile) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		URTHexMapAsset* Map = BeatMappa(World, 6);

		ARTUnit* Tiratore  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Map || !Tiratore || !Bersaglio || !RTWorldFixtures::MakePlayerOnTeam(World, SquadraDelViewer))
		{
			return false;
		}
		// Il muro alto sul bordo fra (0,0) e (1,0): la convenzione `Ring[EdgeIndex]` di `ResolveCoverStructures`.
		const int32 Bordo = URTHexLibrary::Neighbors(FRTCellId(0, 0)).IndexOfByKey(FRTCellId(1, 0));
		if (!TestTrue(TEXT("premessa: (1,0) e' un vicino di (0,0)"), Bordo != INDEX_NONE)) { return false; }
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 0), static_cast<ERTHexDirection>(Bordo), ERTHexCoverType::High, 100);

		Tiratore->PlannedAbilityIndex = BeatIndiceAbilita(Tiratore, TEXT("Hero.Ivrin.PulseShot"));
		Tiratore->PlannedAttackTarget = Bersaglio;
		Tiratore->PlannedCell = Tiratore->Cell;

		TM->LockInAndResolve();
		OutVisibile = TM->KnowledgeForTeamPublic(0).VisibleCells.Contains(Tiratore->Cell);
		FinoAllaFineDelPlayback(TM);
		OutCue = Tiratore->CastCuesPlayedForTest();
		return true;
	};

	int32 Cue = 0;
	bool bVisibile = true;
	if (!GiraIlTurno(/*Viewer*/ 0, Cue, bVisibile)) { return false; }
	if (!TestFalse(TEXT("⛔ premessa: la squadra 0 non vede la cella del tiratore"), bVisibile)) { return false; }
	TestEqual(TEXT("🔴 sorgente nemica fuori vista: nessun beat"), Cue, 0);

	if (!GiraIlTurno(/*Viewer*/ 1, Cue, bVisibile)) { return false; }
	TestEqual(TEXT("✅ visto dalla squadra della sorgente: il beat c'e'"), Cue, 1);
	return true;
}

/**
 * Il cancello del Dash si apre per le sole attivazioni — spec §2.4 C3. Una carica su un bersaglio gia'
 * adiacente non entra in nessuna cella: nessun `MoveAnim` di fase Dash, e la fase deve nascere lo stesso.
 * ✅ Validato per mutazione: riportare il cancello a `if (bHasDash)` fa cadere entrambi gli asserti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackDashOpensForActivationsOnlyTest,
	"RefactorTactics.Playback.DashPhaseOpensForActivationsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackDashOpensForActivationsOnlyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 6);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Caricatore || !Bersaglio || !RTWorldFixtures::MakePlayerOnTeam(World, 0)) { return false; }
	Caricatore->PlannedDashAbility = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	const bool bMoveDash = TM->ResolvedTimelineForTest().ContainsByPredicate([](const FRTResolvedEvent& E)
		{ return E.Type == ERTResolvedEventType::Move && E.Phase == ERTMatchPhase::Dash; });
	if (!TestFalse(TEXT("⛔ premessa: nessun Move di fase Dash"), bMoveDash)) { return false; }

	bool bFaseDash = false;
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		bFaseDash |= (TM->GetPlaybackPhaseName() == TEXT("Dash"));
		TM->Tick(0.05f);
	}
	TestTrue(TEXT("🔴 la fase Dash e' nata dalla sola attivazione"), bFaseDash);
	TestEqual(TEXT("🔴 e il cast dello scatto e' suonato"), Caricatore->CastCuesPlayedForTest(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

Le firme usate sono misurate: `GetRecentEventsForTeam(int32) const` restituisce `TArray<FString>` (`RTTurnManager.h:912`), `ResolvedEventCountOfTypeForTest(ERTResolvedEventType) const` è a `:648`, `KnowledgeForTeamPublic(int32) const` a `:1053`.

In `RTPlaybackStopPredicateTests.cpp`, `NextActionDoesNotStopTwiceWithinOneIntent` (`:604-680`): dopo le premesse (`:643`) aggiungi la premessa dell'attivazione, e dopo l'asserto finale (`:676-677`) il secondo blocco:

```cpp
	// #<n>: l'atto ora comincia con la sua ATTIVAZIONE, e resta un atto solo — attivazione, impronta e colpo
	// portano la stessa coppia (sorgente, azione).
	const bool bAttivazione = TM->ResolvedTimelineForTest().ContainsByPredicate([&AzioniDistinte](const FRTResolvedEvent& Ev)
		{ return Ev.Type == ERTResolvedEventType::AbilityActivated && AzioniDistinte.Contains(Ev.ActionId); });
	if (!TestTrue(TEXT("⛔ premessa: l'intento ha la sua attivazione"), bAttivazione)) { return false; }
```

```cpp
	// --- #<n>: due cure CONSECUTIVE da due unita' diverse sono DUE atti --------------------------------
	//
	// 🔴 La stessa azione generica (`Action.Heal`) da due sorgenti: con il confine sul solo `ActionId` il
	// playback si fermerebbe una volta, e `Next Action` salterebbe il secondo curatore.
	// ✅ Validato per mutazione: `IsActBoundary` sul solo `ActionId` fa cadere questo asserto.
	{
		UWorld* World2 = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("secondo mondo"), World2)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World2); };
		SpawnStopPredicateMap(World2);
		ARTUnit* C1 = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0));
		ARTUnit* Ferito = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(1, 0));
		ARTUnit* C2 = SpawnStopPredicateUnit(World2, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0));
		ARTTurnManager* TM2 = World2->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM2 || !C1 || !Ferito || !C2) { return false; }
		Ferito->Health = FMath::Max(1, Ferito->Health - 30);
		for (ARTUnit* Curatore : { C1, C2 })
		{
			Curatore->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Curatore, TEXT("Action.Heal"));
			Curatore->PlannedAttackTarget = Ferito;
			Curatore->PlannedCell = Curatore->Cell;
		}

		TM2->SetPlaybackControlsEnabled(true);
		TM2->LockInAndResolve();
		if (!TestTrue(TEXT("⛔ premessa: il turno delle cure si riproduce"), TM2->IsResolving())) { return false; }

		int32 FermateCure = 0;
		for (int32 I = 0; I < 600 && TM2->IsResolving(); ++I)
		{
			if (TM2->IsPlaybackPaused())
			{
				if (TM2->GetPlaybackPhaseName() == TEXT("Blast")) { ++FermateCure; }
				TM2->ResumePlayback();
				TM2->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
			}
			else if (TM2->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
			{
				TM2->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
			}
			TM2->Tick(0.05f);
		}
		TestEqual(TEXT("🔴 due cure da due unita': DUE fermate"), FermateCure, 2);
	}
```

Aggiungi `#include "RTAbilityFixtures.h"` agli include del file.

E, sempre in `RTPlaybackStopPredicateTests.cpp`, prima di `#endif`, il buco di copertura sulle attivazioni di Prep e Dash:

```cpp
/**
 * `Next Action` si ferma sulle attivazioni di Prep e di Dash — #<n>, spec §2.4 (confini d'atto).
 *
 * 🔑 Prima di #<n> le due fasi non avevano un canale che passasse dal predicato: `Next Action` le attraversava
 * fermandosi solo al cambio di fase. Ora i rami Prep e Dash aggiornano l'atto in corso come il Blast.
 * 🔢 Le fermate attese: Prep e' la PRIMA fase (nessuna fermata d'ingresso) e ha un'attivazione → 1; nel Dash
 * si entra con una fermata di cambio fase, poi l'attivazione → 2.
 * ✅ Validato per mutazione: togliere `NotePlaybackActShown` da `RevealPlaybackActivations` porta le fermate a
 * 0 in Prep e 1 nel Dash.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackNextActionStopsOnPrepAndDashTest,
	"RefactorTactics.Playback.NextActionStopsOnPrepAndDashActivations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackNextActionStopsOnPrepAndDashTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnStopPredicateMap(World);

	ARTUnit* Scudo      = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-3, 1));
	ARTUnit* Caricatore = SpawnStopPredicateUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnStopPredicateUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(),  FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Scudo || !Caricatore || !Bersaglio) { return false; }

	auto Indice = [](const ARTUnit* U, const TCHAR* Id)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			if (U->GetAbility(i) && U->GetAbility(i)->Def.ActionId == FName(Id)) { return i; }
		}
		return static_cast<int32>(INDEX_NONE);
	};
	Scudo->PlannedAbilityIndex = Indice(Scudo, TEXT("Hero.Muiren.TideGuard"));
	Scudo->PlannedCell = Scudo->Cell;
	Caricatore->PlannedDashAbility = Indice(Caricatore, TEXT("Hero.Branth.Ram"));
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	// Viewer: senza controller `TeamIdOf` ripiega sulla squadra 0, che e' quella delle due sorgenti. Il refresh
	// e' la precondizione delle attivazioni di Prep e Dash (spec §2.5).
	TM->RefreshTeamKnowledgeNow();
	TM->SetPlaybackControlsEnabled(true);
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il turno si riproduce"), TM->IsResolving())) { return false; }

	int32 FermatePrep = 0, FermateDash = 0;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		if (TM->IsPlaybackPaused())
		{
			if (TM->GetPlaybackPhaseName() == TEXT("Prep")) { ++FermatePrep; }
			if (TM->GetPlaybackPhaseName() == TEXT("Dash")) { ++FermateDash; }
			TM->ResumePlayback();
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		else if (TM->GetArmedPlaybackStop() == ERTPlaybackStopAt::None)
		{
			TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		}
		TM->Tick(0.05f);
	}
	TestEqual(TEXT("🔴 Prep: una fermata, sull'attivazione"), FermatePrep, 1);
	TestEqual(TEXT("🔴 Dash: il cambio di fase e l'attivazione"), FermateDash, 2);
	return true;
}
```

- [ ] **Step 2: Build e rosso**

Run: build. Expected: errore su `PlaybackActivationsQueuedForTest` mancante.

- [ ] **Step 3: Membri e dichiarazioni**

`RTTurnManager.h`: rimuovi `PlaybackAttacks` (`:3294`), `PlaybackFootprints` e il suo commento (`:3297-3305`), `PlaybackStructureHits` e il suo commento (`:3307-3315`), `AttacksShown`, `FootprintsShown`, `StructureHitsShown` (`:3347-3349`), le dichiarazioni di `RevealPlaybackFootprints` (`:2335`) e `RevealPlaybackStructureHits` (`:2346`) con i loro commenti. Al posto dei tre array:

```cpp
	/**
	 * Le attivazioni VISIBILI di Prep e di Dash, in ordine di timeline (#<n>, spec §2.4). Una sorgente che
	 * chi guarda non ha il diritto di vedere non entra: tutto o niente (D6).
	 */
	TArray<FRTResolvedEvent> PlaybackActivationsPrep;
	TArray<FRTResolvedEvent> PlaybackActivationsDash;

	/**
	 * Il Blast come UNA sequenza per intento (#<n>, D5): attivazione, impronte, muri, colpi, un atto dopo
	 * l'altro. 🔴 Sostituisce i tre canali paralleli (colpi, impronte da #2454, muri da #2828), che si
	 * svelavano con contatori indipendenti: «l'attivazione precede il colpo dello stesso intento» non
	 * discendeva dall'ordine delle code. La costruisce `URTPlaybackLibrary::BuildBlastSequence`.
	 */
	TArray<FRTBlastSequenceElement> PlaybackBlastSequence;
```

Al posto dei tre contatori:

```cpp
	int32 ActivationsShown = 0;             // attivazioni gia' rivelate nella fase corrente (Prep o Dash), #<n>
	int32 BlastShown = 0;                   // elementi della sequenza di Blast gia' rivelati; prefisso congelato di D-355
```

Al posto delle due `Reveal*`:

```cpp
	/** Rivela le attivazioni fino a `UpTo`; `true` se un confine d'atto ha messo in pausa (#<n>). */
	bool RevealPlaybackActivations(const TArray<FRTResolvedEvent>& Activations, int32 UpTo);

	/** Rivela la sequenza di Blast fino a `UpTo`, un ramo per tipo; `true` se un confine ha messo in pausa (#<n>). */
	bool RevealBlastSequence(int32 UpTo);

	/** Aggiorna l'atto in corso e consuma un `Next Action` armato: la regola e' `IsActBoundary` (#3292, #<n>). */
	bool NotePlaybackActShown(const FRTResolvedEvent& Ev);

	/** Il colpo: riga `Colpo:`, ruoli `Attack`/`Hit`, token, broadcast — il corpo del vecchio ciclo dei colpi. */
	void ShowPlaybackAttack(const FRTResolvedEvent& Atk);

	/** Quanto la fase spende in attivazioni PRIMA delle rotte: solo il Dash ne ha (#<n>, spec §2.4). */
	float PlaybackActivationLeadSeconds(ERTMatchPhase InPhase) const;
```

Accanto a `ResolvedTimelineForTest` (`:634`), pubblico:

```cpp
	/** Le attivazioni nelle code di playback (Prep, Dash, e gli elementi `AbilityActivated` della sequenza), #<n>. */
	int32 PlaybackActivationsQueuedForTest() const;

	/** L'indice di cella dell'anim di `Unit` nella fase di playback corrente; `INDEX_NONE` se non ne ha una (#<n>). */
	int32 PlaybackAnimCellIndexForTest(const ARTUnit* Unit) const;

	/** Quanti elementi della sequenza di Blast sono gia' stati mostrati: il prefisso congelato di D-355 (#<n>). */
	int32 PlaybackBlastShownForTest() const { return BlastShown; }

	/** I `TimelineIndex` della sequenza di Blast, in ordine (#<n>). */
	TArray<int32> PlaybackBlastSequenceIndicesForTest() const
	{
		TArray<int32> Out;
		for (const FRTBlastSequenceElement& E : PlaybackBlastSequence) { Out.Add(E.TimelineIndex); }
		return Out;
	}
```

- [ ] **Step 4: `BeginPlayback` — smistamento, filtro, D-355, cancelli**

`RTTurnManager.cpp:7672-7675`: `PlaybackAttacks.Reset(); ... PlaybackStructureHits.Reset();` diventano

```cpp
	PlaybackActivationsPrep.Reset();
	PlaybackActivationsDash.Reset();
	// `PlaybackBlastSequence` NON si azzera qui: estendendo (D-355) e' il `Previous` della ricostruzione.
```

Nel ciclo `:7688-7778`, rimuovi i rami `Attack`, `AttackFootprint`, `StructureHit`. I loro commenti non si perdono: il loro contenuto vivo è riscritto nel commento di testa di `RevealBlastSequence` (Step 6, codice completo lì). ⚠️ Il commento del ramo `StructureHit` (`:7761-7769`, *«NESSUN filtro su `Ev.Phase`… inventerebbe una fase `Blast`»*) **non si copia**: è diventato falso, perché il filtro di fase ora c'è — `BuildBlastSequence` prende solo `Phase == Blast` — e il commento di `RevealBlastSequence` lo dice con un ⏱️. Al posto dei tre rami aggiungi:

```cpp
		else if (Ev.Type == ERTResolvedEventType::AbilityActivated)
		{
			// 🔴 D6, spec §2.5: il predicato e' quello delle RIGHE DI LOG, non quello del `Move` qui sopra. Un
			// `ObservedPrefixLength` su un'attivazione — nessuna rotta, nessuna cella — leggerebbe il vuoto come
			// fail-closed e nasconderebbe ogni attivazione, comprese quelle di chi guarda.
			// ⚠️ Il Blast non passa di qui: lo smista `BuildBlastSequence`, con lo stesso predicato.
			if (Ev.SourceVerdict.AllowsTeam(ViewerTeamId))
			{
				if (Ev.Phase == ERTMatchPhase::Prep) { PlaybackActivationsPrep.Add(Ev); }
				else if (Ev.Phase == ERTMatchPhase::Dash) { PlaybackActivationsDash.Add(Ev); }
			}
		}
```

Dopo il ciclo, prima di `// Fasi attive` (`:7780`):

```cpp
	// La sequenza per intento del Blast (D5). 🔑 **Estendendo (D-355) il prefisso gia' mostrato e' stabile**:
	// i primi `BlastShown` elementi si riproducono verbatim e gli eventi nuovi si accodano (`Ruling`: coda, non
	// inserimento). Precondizione: la timeline cresce solo per accodamento.
	PlaybackBlastSequence = URTPlaybackLibrary::BuildBlastSequence(ResolvedTimeline,
		bPreserveClock ? PlaybackBlastSequence : TArray<FRTBlastSequenceElement>(),
		bPreserveClock ? BlastShown : 0, ViewerTeamId);

	int32 NumColpi = 0, NumImpronte = 0, NumMuri = 0, NumAttivazioniBlast = 0;
	for (const FRTBlastSequenceElement& E : PlaybackBlastSequence)
	{
		switch (ResolvedTimeline[E.TimelineIndex].Type)
		{
		case ERTResolvedEventType::Attack:           ++NumColpi; break;
		case ERTResolvedEventType::AttackFootprint:  ++NumImpronte; break;
		case ERTResolvedEventType::StructureHit:     ++NumMuri; break;
		case ERTResolvedEventType::AbilityActivated: ++NumAttivazioniBlast; break;
		default: break;
		}
	}
```

I cancelli `:7789-7799`:

```cpp
	// #<n>, C3: una fase NASCE anche dalle sole attivazioni visibili — un Blast di cure, una Prep nemica nota.
	if (bPrepActiveThisTurn || PlaybackActivationsPrep.Num() > 0) { PlaybackPhases.Add(ERTMatchPhase::Prep); }
	if (bHasDash || PlaybackActivationsDash.Num() > 0) { PlaybackPhases.Add(ERTMatchPhase::Dash); }
	if (URTPlaybackLibrary::BlastPhaseIsActive(NumColpi, bHasBlastMove, NumImpronte, NumMuri, NumAttivazioniBlast))
	{
		PlaybackPhases.Add(ERTMatchPhase::Blast);
	}
```

- [ ] **Step 5: `EnterPlaybackPhase`, durate, alpha del Dash, `Step`**

`EnterPlaybackPhase` (`:7982-7991`): `AttacksShown = 0; FootprintsShown = 0;` e `StructureHitsShown = 0;` diventano

```cpp
	ActivationsShown = 0;
	// ⚠️ Si azzera a ogni FASE come i tre contatori che sostituisce, e il ramo `bPreserveClock` salta questa
	// funzione: e' cio' che rende `BlastShown` il prefisso congelato di D-355.
	BlastShown = 0;
```

`PhaseTimeForPlaybackPhase` (`:8879-8883`):

```cpp
	int32 NumAttivazioni = 0;
	if (InPhase == ERTMatchPhase::Prep) { NumAttivazioni = PlaybackActivationsPrep.Num(); }
	else if (InPhase == ERTMatchPhase::Dash) { NumAttivazioni = PlaybackActivationsDash.Num(); }

	// La formula sta in `URTPlaybackLibrary::PhaseTime` (#1817). ⏱️ *Fino a #<n> riceveva tre conteggi di
	// canale; ora le attivazioni di Prep/Dash e la lunghezza della sequenza di Blast.*
	return URTPlaybackLibrary::PhaseTime(InPhase, MaxSeg, NumAttivazioni, PlaybackBlastSequence.Num(),
		PlaybackCellsPerSecond, AttackShowSeconds, PhaseBeatSeconds);
```

Dopo `DurationForPlaybackPhase` (`:8900`):

```cpp
float ARTTurnManager::PlaybackActivationLeadSeconds(ERTMatchPhase InPhase) const
{
	// Solo il Dash: in Prep non ci sono rotte, nel Blast la sequenza contiene gia' le attivazioni.
	return (InPhase == ERTMatchPhase::Dash)
		? PlaybackActivationsDash.Num() * FMath::Max(0.f, AttackShowSeconds)
		: 0.f;
}

int32 ARTTurnManager::PlaybackActivationsQueuedForTest() const
{
	int32 N = PlaybackActivationsPrep.Num() + PlaybackActivationsDash.Num();
	for (const FRTBlastSequenceElement& E : PlaybackBlastSequence)
	{
		if (ResolvedTimeline.IsValidIndex(E.TimelineIndex)
			&& ResolvedTimeline[E.TimelineIndex].Type == ERTResolvedEventType::AbilityActivated)
		{
			++N;
		}
	}
	return N;
}
```

Alpha per anim (`:8277-8282`):

```cpp
		// #<n>: nel Dash le rotte partono DOPO le attivazioni (spec §2.4). `RouteAlpha` clampa un tempo negativo
		// a zero: durante le attivazioni il cilindro resta sulla cella di partenza.
		const float AnticipoAttivazioni = PlaybackActivationLeadSeconds(Ph);
		TArray<float, TInlineAllocator<16>> AlphaAnim;
		AlphaAnim.SetNumUninitialized(MoveAnims.Num());
		for (int32 AnimIdx = 0; AnimIdx < MoveAnims.Num(); ++AnimIdx)
		{
			AlphaAnim[AnimIdx] = bAlphaPerPercorso
				? URTPlaybackLibrary::RouteAlpha(MoveAnims[AnimIdx].World.Num() - 1,
					PlaybackPhaseElapsed - AnticipoAttivazioni, PlaybackCellsPerSecond)
				: AlphaFase;
		}
```

`StepMicroStep` (`:8115-8119`):

```cpp
	// #<n>: nel Dash i micro-step cominciano DOPO le attivazioni — i confini si contano sul solo tratto delle
	// rotte, o `Step` si fermerebbe a meta' di un segmento. Fuori dal Dash l'anticipo e' zero e la formula e'
	// quella di prima.
	const float Anticipo = FMath::Clamp(PlaybackActivationLeadSeconds(PlaybackPhases[PlaybackPhaseIdx]), 0.f, Durata);
	const float DurataRotte = Durata - Anticipo;
	if (DurataRotte <= 0.f)
	{
		bPlaybackPaused = true;
		PlaybackStepTargetElapsed = -1.f;
		return;
	}
	const float AlphaCorrente = FMath::Clamp((PlaybackPhaseElapsed - Anticipo) / DurataRotte, 0.f, 1.f);
	const float AlphaTarget = URTPlaybackLibrary::NextMicroStepBoundary(AlphaCorrente, Passi);

	// Il confine in SECONDI, calcolato ora: il tick ci arriva senza sapere quanti frame servono.
	PlaybackStepTargetElapsed = Anticipo + AlphaTarget * DurataRotte;
```

**Niente corsa sul posto durante l'anticipo** (decisione (9), spec §6). `EnterPlaybackPhase` (`:8010-8016`) diventa:

```cpp
	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		// #<n>: con attivazioni nel Dash la corsa NON parte con la fase — la accende `TickPlayback` quando le
		// rotte cominciano, dopo l'anticipo. Accenderla qui farebbe correre sul posto lo scattatore mentre suona
		// il cast: la classe di difetti di #3519. L'annuncio `OnUnitMoveStarted` resta all'ingresso.
		const bool bCorsaRinviata = PlaybackActivationLeadSeconds(Ph) > 0.f;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid())
			{
				if (!bCorsaRinviata) { A.Unit->bIsMovingVisually = true; }
				OnUnitMoveStarted.Broadcast(A.Unit.Get());
			}
		}
	}
```

In `TickPlayback`, il calcolo di `bInCorsa` (`:8301-8306`):

```cpp
					bool bInCorsa = false;
					// #<n>: durante l'anticipo delle attivazioni lo scattatore e' fermo sul punto di partenza mentre
					// suona il cast — `RouteAlpha` lo tiene a zero — e NON corre: la corsa parte con la rotta.
					const bool bInAnticipo = PlaybackPhaseElapsed < AnticipoAttivazioni;
					for (int32 Altra = 0; Altra < MoveAnims.Num() && !bInCorsa; ++Altra)
					{
						bInCorsa = !bInAnticipo && MoveAnims[Altra].Phase == Ph && MoveAnims[Altra].Unit == A.Unit
							&& AlphaAnim[Altra] < 1.f;
					}
```

(le righe `:8307-8312` restano: spengono la posa insieme al flag). E la guardia della posa graykit (`:8396`):

```cpp
				// #<n>: e nemmeno la posa di corsa durante l'anticipo — l'alpha e' zero, ma la posa a zero e' gia'
				// un passo accennato.
				const bool bPosaInAnticipo = bAlphaPerPercorso && PlaybackPhaseElapsed < AnticipoAttivazioni;
				if ((!bAlphaPerPercorso || Alpha < 1.f) && !bPosaInAnticipo)
```

Il commento di `:8244-8251` (da `// ⚠️ **Il \`Blast\` resta sull'\`Alpha\` di fase` fino a `non un effetto collaterale di questa.`) diventa:

```cpp
		// ⚠️ **Il `Blast` resta sull'`Alpha` di fase, e resta di proposito.** Li' la durata vale
		// `Max(Max(1, N) x AttackShowSeconds, spinta)` con `N` la lunghezza della SEQUENZA per intento (#<n>, D5),
		// e NON `MaxSeg / rate`: la spinta del knockback si distende sulla finestra della sequenza, ed e'
		// documentato come deliberato in `URTPlaybackLibrary.h`.
		// ⏱️ *Fino a #<n> questa riga parlava di tre canali paralleli (colpi, muri, impronte) e del `Max` fra
		// loro.* ⚠️ Ne segue cio' che la spec dichiara (§6): la sequenza e' piu' lunga del piu' lungo dei canali,
		// quindi la spinta rallenta ancora, e la puo' allungare un intento che con l'unita' spinta non ha rapporto.
		// Cambiarlo e' una decisione separata con la sua evidenza, non un effetto collaterale di questa.
		// 🔑 Nel Dash, invece, le rotte partono DOPO l'anticipo delle attivazioni (`PlaybackActivationLeadSeconds`).
```

E la definizione dell'accessore, accanto a `PlaybackActivationsQueuedForTest`:

```cpp
int32 ARTTurnManager::PlaybackAnimCellIndexForTest(const ARTUnit* Unit) const
{
	if (!PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return INDEX_NONE;
	}
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	for (int32 i = 0; i < MoveAnims.Num(); ++i)
	{
		if (MoveAnims[i].Phase == Ph && MoveAnims[i].Unit.Get() == Unit && PlaybackAnimCellIndex.IsValidIndex(i))
		{
			return PlaybackAnimCellIndex[i];
		}
	}
	return INDEX_NONE;
}
```

- [ ] **Step 6: Le rivelazioni**

Sostituisci `RevealPlaybackFootprints` e `RevealPlaybackStructureHits` (`:7896-7977`) con:

```cpp
bool ARTTurnManager::NotePlaybackActShown(const FRTResolvedEvent& Ev)
{
	// L'atto in corso si aggiorna solo su un'azione vera: un `NAME_None` non lo azzera (#3292).
	if (!Ev.ActionId.IsNone())
	{
		PlaybackLastShownAction = Ev.ActionId;
		PlaybackLastShownSource = Ev.SourceStableUnitId;
	}
	if (PlaybackStopAt == ERTPlaybackStopAt::NextAction
		&& URTPlaybackLibrary::IsActBoundary(Ev, PlaybackStopFromAction, PlaybackStopFromSource))
	{
		PausePlaybackAtActBoundary();
		return true; // cio' che questo tick avrebbe ancora rivelato resta per la ripresa
	}
	return false;
}

bool ARTTurnManager::RevealPlaybackActivations(const TArray<FRTResolvedEvent>& Activations, int32 UpTo)
{
	const int32 Target = FMath::Min(UpTo, Activations.Num());
	while (ActivationsShown < Target)
	{
		const FRTResolvedEvent& Ev = Activations[ActivationsShown];
		ShowActivation(Ev);
		++ActivationsShown;
		// #<n>: i rami Prep e Dash aggiornano l'atto in corso come fa il Blast, o `Next Action` salterebbe le
		// attivazioni (spec §2.4, confini d'atto).
		if (NotePlaybackActShown(Ev))
		{
			return true;
		}
	}
	return false;
}

void ARTTurnManager::ShowPlaybackAttack(const FRTResolvedEvent& Atk)
{
	ARTUnit* const AtkSrc = UnitByStableId(Atk.SourceStableUnitId);
	ARTUnit* const AtkTgt = UnitByStableId(Atk.TargetStableUnitId);
	AddLogEvent(FString::Printf(TEXT("Colpo: %s -> %s (%d)"),
		AtkSrc ? *AtkSrc->GetName() : TEXT("?"),
		AtkTgt ? *AtkTgt->GetName() : TEXT("(eliminato)"),
		Atk.Amount), FRTLogSubject::Unit(AtkSrc));
	if (AtkSrc) { AtkSrc->PlayPresentationRole(ERTPresentationRole::Attack); }
	if (AtkTgt)
	{
		AtkTgt->PlayPresentationRole(ERTPresentationRole::Hit);
		AtkTgt->ShowDamageToken(Atk.Amount); // #2455: sul BERSAGLIO, dallo stesso evento della cue
	}
	OnAttackResolved.Broadcast(AtkSrc, AtkTgt, Atk.Amount);
}

// La sequenza di Blast (#<n>, D5). I commenti dei tre rami di `BeginPlayback` che questa funzione sostituisce,
// in cio' che hanno di ancora vero:
//
// ⛔ **Nessun filtro di conoscenza sull'impronta e sul muro, e non e' una dimenticanza**: l'impronta e' un fatto
// dell'AZIONE e le sue celle sono terreno, non occupazione — la catena `BlastOriginCell -> HexHitCells` non
// tocca l'occupazione (`#2791`); il soggetto di uno `StructureHit` e' un BORDO, e un muro che cade non rivela
// chi ci stava dietro, che resta coperto dal velo. ⚠️ Se un giorno l'impronta portasse un dato dipendente da
// CHI e' stato colpito, diventerebbe un canale e andrebbe filtrata come il `Move`.
// ⚠️ **Nessun `Src` richiesto per impronte e muri**: il fatto riguarda la mappa e resta vero anche se chi ha
// sparato e' morto nello stesso Blast. Pretendere l'Actor perderebbe proprio i colpi dei caduti.
// 🔴 **Il filtro di FASE ora c'e'.** ⏱️ *Fino a #<n> il ramo `StructureHit` di `BeginPlayback` non filtrava
// `Ev.Phase`, e un secondo produttore in Cleanup avrebbe inventato una fase `Blast`.* `BuildBlastSequence`
// prende solo `Phase == Blast`: un muro colpito in Cleanup non entra in questa sequenza e non apre la fase.
// 🔑 **L'unico filtro di privacy di questa sequenza e' sulle ATTIVAZIONI** (D6), ed e' a monte, in
// `BuildBlastSequence`: qui si mostra tutto cio' che e' in sequenza.
bool ARTTurnManager::RevealBlastSequence(int32 UpTo)
{
	const int32 Target = FMath::Min(UpTo, PlaybackBlastSequence.Num());
	if (BlastShown >= Target)
	{
		return false;
	}

	// ⚠️ L'actor si cerca UNA volta per chiamata, non per elemento: `FindInWorld` itera gli attori.
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(GetWorld());

	while (BlastShown < Target)
	{
		const int32 Indice = PlaybackBlastSequence[BlastShown].TimelineIndex;
		++BlastShown; // ⚠️ avanza anche senza `MapActor` o con un indice fuori range: un fatto consumato non ripassa
		if (!ResolvedTimeline.IsValidIndex(Indice))
		{
			continue;
		}
		const FRTResolvedEvent& Ev = ResolvedTimeline[Indice];

		// 🔑 **Un `switch`, e i rami sono le cue di prima** (#<n>, D5): la cue per elemento non cambia, cambia
		// chi decide QUANDO — la sequenza, non tre contatori.
		switch (Ev.Type)
		{
		case ERTResolvedEventType::AbilityActivated:
			ShowActivation(Ev);
			break;
		case ERTResolvedEventType::AttackFootprint:
			// ⛔ Le celle COSI' COME ARRIVANO ([D-301]); nessun filtro di conoscenza: sono terreno, non occupazione.
			if (MapActor) { MapActor->AddPlaybackFootprint(Ev.HitCells); }
			break;
		case ERTResolvedEventType::StructureHit:
			// ⛔ Il bordo che l'evento PORTA, e nient'altro (#2828).
			if (MapActor)
			{
				MapActor->AddPlaybackStructureHit(Ev.StructureCell, Ev.StructureToward,
					Ev.EnvironmentOutcome == ERTEnvironmentOutcome::CoverDestroyed);
			}
			break;
		case ERTResolvedEventType::Attack:
			ShowPlaybackAttack(Ev);
			break;
		default:
			break;
		}

		if (NotePlaybackActShown(Ev))
		{
			return true;
		}
	}
	return false;
}
```

- [ ] **Step 7: `TickPlayback`**

Prima di `if (Ph == ERTMatchPhase::Blast)` (`:8422`), e sostituendo l'intero blocco `:8418-8506`:

```cpp
	// #<n>: Prep e Dash svelano le proprie attivazioni una per volta, con lo stesso ritmo dei colpi.
	if (Ph == ERTMatchPhase::Prep || Ph == ERTMatchPhase::Dash)
	{
		const TArray<FRTResolvedEvent>& Attivazioni =
			(Ph == ERTMatchPhase::Prep) ? PlaybackActivationsPrep : PlaybackActivationsDash;
		if (RevealPlaybackActivations(Attivazioni,
			URTPlaybackLibrary::AttacksToShow(Attivazioni.Num(), PlaybackPhaseElapsed, AttackShowSeconds)))
		{
			return; // stessa ragione del Blast: uscire dal tick, o la fermata durerebbe zero
		}
	}
	// ⚠️ `if`, NON `else if`: il Blast fa DUE cose insieme — scivolare (knockback, sopra) e rivelare (#911).
	if (Ph == ERTMatchPhase::Blast)
	{
		// 🔴 **Una sequenza, un contatore, una cadenza** (#<n>, D5): attivazione, impronta, muri, colpi di un
		// intento, poi il successivo. `AttacksToShow` resta il contro-termine di `PhaseTime`.
		if (RevealBlastSequence(URTPlaybackLibrary::AttacksToShow(
			PlaybackBlastSequence.Num(), PlaybackPhaseElapsed, AttackShowSeconds)))
		{
			return;
		}
	}
```

Il catch-all `:8521-8562` (il blocco `if (Ph == ERTMatchPhase::Blast) { ... }` dentro `if (PlaybackPhaseElapsed >= PhaseDur)`) diventa:

```cpp
		// Chi non ha fatto in tempo a comparire compare adesso: una fase non deve PERDERE un fatto.
		// ⛔ **Rete senza casi, e non si toglie** (#3277): `PhaseTime` dimensiona Prep, Dash e Blast sul loro
		// conteggio, quindi a fine fase `AttacksToShow` vale gia' `N`. Se qualcuno accorciasse la fase, la rete
		// tornerebbe necessaria e nessun rosso lo direbbe: lo pinna `Playback.EveryChannelIsFullyRevealedByPhaseEnd`.
		if (Ph == ERTMatchPhase::Prep) { RevealPlaybackActivations(PlaybackActivationsPrep, PlaybackActivationsPrep.Num()); }
		if (Ph == ERTMatchPhase::Dash) { RevealPlaybackActivations(PlaybackActivationsDash, PlaybackActivationsDash.Num()); }
		if (Ph == ERTMatchPhase::Blast) { RevealBlastSequence(PlaybackBlastSequence.Num()); }
```

⚠️ **Dichiarato (spec §6): il recupero di fine fase del Blast ora fa due cose che prima non faceva.** Scrive anche le righe `Colpo:` — il vecchio ciclo di recupero (`:8540-8561`) suonava ruoli, token e broadcast **senza** la riga del feed — e passa dal predicato di confine (`NotePlaybackActShown`), quindi un `Next Action` armato può fermarsi anche lì. Su un turno normale non cambia nulla: la rete è senza casi (`EveryChannelIsFullyRevealedByPhaseEnd`), e arriva a fine fase con la sequenza già tutta mostrata. Se ci arrivasse con elementi in coda e si fermasse su un confine, gli elementi restanti di quella fase verrebbero saltati dal passaggio alla fase successiva: lo stesso limite che i vecchi `RevealPlaybackFootprints`/`RevealPlaybackStructureHits` avevano nel recupero.

Controlla che non resti un riferimento ai nomi rimossi: `grep -nE "\b(PlaybackAttacks|PlaybackFootprints|PlaybackStructureHits|AttacksShown|FootprintsShown|StructureHitsShown|RevealPlaybackFootprints|RevealPlaybackStructureHits)\b" Source/RefactorTactics/Turn/RTTurnManager.*` → solo commenti storici, nessun codice. (I membri omonimi di `ARTHexMapActor` sono un'altra classe e restano.)

- [ ] **Step 8: Build e verde**

Run: build; test `RefactorTactics.Playback+RefactorTactics.Turn+RefactorTactics.Presentation+RefactorTactics.Veil+RefactorTactics.Actions+RefactorTactics.Environment+RefactorTactics.Reactions+RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback` (`Reactions` perché i test del `Brace` di `RTDefensiveReactionTests.cpp` attraversano la ricostruzione D-355 che questo task cambia).
Expected: verdi, `**** TEST COMPLETE`. Le stesse regole del Task 5 Step 6 per un tempo pinnato su un turno reale che cresce (spec §6). `DeterminismIsIndependentOfPlayback` verde è il gate del «nessun dato nuovo nell'hash».

- [ ] **Step 9: Mutazioni (3) e (5)**

(3) In `BeginPlayback` sostituisci `if (Ev.SourceVerdict.AllowsTeam(ViewerTeamId))` con `if (true)`, build, test `RefactorTactics.Playback.HiddenSourceHasNoActivationBeat`: `Result={Fail}` su «non osservata: nessuna attivazione in coda». Ripristina. Ripeti commentando in `BuildBlastSequence` la riga `if (Ev.Type == ... && !Ev.SourceVerdict.AllowsTeam(ViewerTeamId)) { continue; }`: stesso rosso. Ripristina, build, verde.

(5) Riporta l'ultima riga di `IsActBoundary` a `return Event.ActionId != CurrentAction;`, build, test `RefactorTactics.Playback.NextActionDoesNotStopTwiceWithinOneIntent`: `Result={Fail}` su «due cure da due unita': DUE fermate». Ripristina, build, verde.

Le mutazioni dei test aggiunti in questo task, una per volta, ciascuna seguita da ripristino, build e verde:

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| In `StepMicroStep`, `const float Anticipo = 0.f;` | `Playback.DashStepLandsOnCellsAfterTheActivations` | «il primo Step atterra sulla PRIMA cella della rotta» |
| In `bInCorsa`, togli `!bInAnticipo &&` | `Playback.DashStepLandsOnCellsAfterTheActivations` | «durante l'anticipo lo scattatore non corre sul posto» |
| In `RevealPlaybackActivations`, togli il blocco `if (NotePlaybackActShown(Ev)) { return true; }` | `Playback.NextActionStopsOnPrepAndDashActivations` | entrambi i conteggi |
| Cancello del Dash riportato a `if (bHasDash)` | `Playback.DashPhaseOpensForActivationsOnly` | «la fase Dash e' nata dalla sola attivazione» |
| In `BeginPlayback`, `BuildBlastSequence(ResolvedTimeline, TArray<FRTBlastSequenceElement>(), 0, ViewerTeamId)` anche con `bPreserveClock` | `Playback.ActivationPrefixSurvivesASuspendedBlast` | ⚠️ cade **solo** se l'estensione aggiunge al Blast un evento che la ricostruzione da zero metterebbe prima di un elemento già mostrato; se resta verde, annotalo nel commit come limite del montaggio (la proprietà resta pinnata dal test puro del Task 5), non allentare l'asserto |

- [ ] **Step 10: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp
git commit -F - <<'EOF'
feat(<n>): playback del momento - code di Prep e Dash, sequenza per intento del Blast, filtro D6, prefisso stabile D-355

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 8: Documenti — voce PIE, seduta, scenario, conoscenza parziale, radar

**Files:**
- Create: `Scenarios/Visual/Ability/CastBeat.json` — la cartella `Scenarios/Visual/Ability/` **non esiste** e si crea con il file (`Scenarios/Visual/` ha oggi `Core/`, `Reaction/`, … ma nessun `Ability/`)
- Modify: `docs/technical/runbooks/scenari-validazione-visiva.md` — una riga nella tabella del catalogo che contiene `Visual.Core.PhaseOrder` (`:201`), in coda alla tabella
- Modify: `docs/technical/test-manuali-pie.md` — una riga **in coda** alla tabella di `## Scenari di validazione visiva — corpus Visual.*` (`:1885`, tabella da `:1903`), e la sezione `## Stato in numeri` (`:90-96`)
- Modify: `docs/roadmap/editor-sessions.yaml` — una seduta **subito prima di `^not_schedulable:`** (`:5095`)
- Modify: `docs/technical/systems/conoscenza-parziale-visibile-spec.md` — una riga **in coda** alla tabella dei canali di §1.3 (ultima riga `:119`)

**Interfaces:** nessuna di codice. Regole: l'esito atteso vive **solo** in `test-manuali-pie.md`; il file delle sedute cita gli **ID**, mai l'esito; nessun totale volatile; lo YAML è CRLF; nessuna pipe `|` dentro le celle.

- [ ] **Step 1: Lo scenario**

`Scenarios/Visual/Ability/CastBeat.json`:

```json
{
  "scenarioId": "Visual.Ability.CastBeat",
  "tags": ["animation", "ability", "cast", "muiren", "branth"],
  "version": 1,
  "seed": 0,
  "mapRadius": 4,

  "_nota": "Il momento del cast (#<n>, spec 2026-10-07-momento-ability-activated-design.md). Muiren usa TideGuard, uno scudo di Prep SENZA colpo: prima di #<n> la Prep aveva solo il beat generico, ora la sorgente ha un beat proprio. Branth usa ImpactShot su Muiren: cast e impatto sono DUE momenti, nell'ordine. In v0.1 cast e colpo suonano la stessa clip (D2): si distinguono per momento, non per forma.",

  "units": [
    { "id": "M1", "hero": "Hero.Muiren", "team": 0, "cell": [-1, 0, 0] },
    { "id": "B1", "hero": "Hero.Branth", "team": 1, "cell": [ 1, 0, 0] }
  ],

  "turns": [
    {
      "intents": [
        { "unit": "M1", "ability": "Hero.Muiren.TideGuard" },
        { "unit": "B1", "ability": "Hero.Branth.ImpactShot", "target": "M1" }
      ]
    }
  ],

  "expect": [
    { "type": "UnitAtCell",     "unit": "M1", "cell": [-2, 0, 0] },
    { "type": "UnitAtCell",     "unit": "B1", "cell": [ 1, 0, 0] },
    { "type": "TurnsCompleted", "value": 1 }
  ]
}
```

⌫ ➕ impl. *Questo paragrafo prescriveva `"target": "M1"` su TideGuard, «ciò che il caricatore pretende», ed era falso*: eseguito nel Task 8, il caricatore rifiuta quell'intento con *«l'unita' bersaglia se stessa»* (`ScenarioHarness/RTScenarioLoader.cpp:2827-2832`), perché `TideGuard` è self-target (`bSelfTarget`, `AbilityResolvesOnSelf`) e il campo va **omesso**, come fa `Scenarios/Spec/Combat/ProactiveShieldAbsorbsWhereBaseShieldDoesNot.json`. Con il campo cadevano `ShippedScenariosAreValid`, `ShippedScenariosRequireKnownCapabilities` ed `EveryShippedScenarioRuns`. In Prep l'istanza agisce su chi la usa (`Instance.TargetUnitId = i`): il `target` esplicito non serve e non è accettato. ➕ impl. Anche l'atteso su `M1` è `[-2, 0, 0]`, non `[-1, 0, 0]`: `Hero.Branth.ImpactShot` porta `Weapon.Impact`, che aggiunge una spinta di una cella verso ovest (misurato dal fallimento di `UnitAtCell(M1)` al secondo run).

Il corpus golden **non** chiede nulla per l'Id nuovo: è un elenco esplicito (`Tests/RTGoldenCorpusTests.cpp:583`), e `Visual.Ability.CastBeat` non vi entra.

Run: test `RefactorTactics.ScenarioIndex+RefactorTactics.ScenarioWriter+RefactorTactics.Simulation`.
Expected: verdi; `ShippedScenariosAreTagged` vede i tag dello scenario nuovo.

Il catalogo degli scenari visivi elenca ogni `Visual.*` in una tabella (`docs/technical/runbooks/scenari-validazione-visiva.md:201`, colonne `scenario | fixture | cosa si guarda | stato`). In coda a quella tabella:

```markdown
| `Visual.Ability.CastBeat` | r4 | il **momento** del cast: TideGuard di Muiren, che non colpisce nessuno, ha un beat sulla sorgente in Prep; ImpactShot di Branth mostra **due** momenti, il cast e poi l'impatto | scritto |
```

- [ ] **Step 2: La voce PIE**

Prima di toccare il file: `node tools/radar/doc-coherence.ts --check` e annota il ricalcolo A1 che stampa (voci e stati). Poi, in coda alla tabella di `## Scenari di validazione visiva — corpus Visual.*`, come unica riga con le cinque colonne:

```markdown
| **PIE-CAST-BEAT** | Un'abilità ha il suo momento sulla sorgente ([#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>)) | `Visual.Ability.CastBeat`, con `rt.Debug.PlaybackControls 1` per il criterio (2); per il criterio (3) il banco di [#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532) con un'abilità di una sorgente **nemica fuori dalla conoscenza** della squadra che guarda | **(0)** `Hero.Muiren.TideGuard`, che non colpisce nessuno, ha un beat visibile **sulla sorgente** durante la Prep: prima di questa voce la Prep aveva solo il beat generico. **(1)** Per `Hero.Branth.ImpactShot` si vedono **due** momenti, il cast su Branth e poi l'impatto su Muiren, in quest'ordine. **(2)** Con `Next Action`, il playback si ferma **una** volta per intento — sul cast — e non una seconda sull'impronta o sul colpo dello stesso intento. **(3)** Con la sorgente nemica fuori dalla conoscenza, **nessun** beat sulla sorgente e nessuna riga `Attiva:` nel feed. ⚠️ In v0.1 cast e colpo usano la **stessa clip** dell'eroe: il criterio (1) giudica due MOMENTI, non due forme. Coperto headless: `Turn.AbilityActivatedIsEmittedOncePerIntent`, `Playback.ActivationPlaysTheCastCue`, `Playback.HiddenSourceHasNoActivationBeat`, `Playback.NextActionDoesNotStopTwiceWithinOneIntent`; qui resta ciò che nessun test vede — il beat a schermo | ⏳ da eseguire — scritta il 2026-10-07 insieme al codice di [#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>); il verdetto è dell'autore |
```

Poi rilancia `node tools/radar/doc-coherence.ts --check`: il nuovo ricalcolo A1 deve dare **una voce in più, nuova e ⏳**, nessun'altra voce cambiata.

🔑 **La riga dei totali di `## Stato in numeri` (`:92`, oggi `**257 voci**: ✅ **96 verdi** · 🟡 **30 parziali** · ❌ **7 fallite** · ⏳ **124 aperte**.`) non è prosa: è il valore che A1 misura**, e A1 la confronta con il ricalcolo. Riscrivila **con i numeri che il tool stampa dopo l'inserimento**, nella stessa forma. Non sono un totale volatile ai sensi di `AGENTS.md` §14, perché un gate la rimisura a ogni `--check`.

Poi la riga «Rimisurato» nuova va **subito sotto la riga dei totali**, cioè **sopra** la riga `➕ **Rimisurato il 2026-10-07 (seduta \`U67\`…` (`:94`): le righe vanno dalla più recente in giù. Nella forma di `:96`:

```markdown
➕ **Rimisurato il 2026-10-07 ([#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>), il momento `AbilityActivated`): il ricalcolo di `doc-coherence` A1 dava `<prima>` **prima** di toccare il file e `<dopo>` **dopo**.** Delta **uno, nuova e ⏳** — `PIE-CAST-BEAT`: nessuna voce esistente cambia stato.
```

(`<prima>` e `<dopo>` sono le due stringhe A1 misurate in questo step: sono esiti del passaggio corrente, ammessi da `AGENTS.md` §14.)

- [ ] **Step 3: La seduta**

`editor-sessions.yaml`, subito prima della riga `not_schedulable:`. Prima misura l'ultimo id: `grep -n "^  - id: U" docs/roadmap/editor-sessions.yaml | tail -1` (in pianificazione era `U67`, `:5069`); usa il successivo.

```yaml
  - id: U69   # ➕ impl. il piano diceva U68, ma main l'ha presa per PIE-V01-TRACER (#2454) prima del merge
    title: Il momento — il beat di cast sulla sorgente (AbilityActivated)
    block: 6
    critical: false
    execution_lane: pie
    produces: >-
      il verdetto su `PIE-CAST-BEAT`; nessun asset
    artifacts: []
    unblocked_by: []
    shares_setup_with: [U67]
    verifies: [PIE-CAST-BEAT]
    issues: [<n>]
    done_when: >-
      la voce `PIE-CAST-BEAT` ha un verdetto nel registro, dato a schermo da una persona dopo il merge del codice
    notes: |
      Scritta il 2026-10-07 dal piano `docs/superpowers/plans/2026-10-07-momento-ability-activated.md`.
      L'esito atteso vive in `test-manuali-pie.md`; qui solo l'allestimento: `rt.Test.Scenario Visual.Ability.CastBeat`
      da `L_DevSandbox`, `rt.Debug.PlaybackControls 1` per `K`/`L` (➕ impl. il criterio (2) è `NOT RUN` in PIE: la fermata per atto non ha un ingresso). Il criterio (3) si allestisce col
      banco Ability Lab di U67, un'abilita' di una sorgente nemica non osservata.
```

Verifica:

```powershell
python -c "import yaml; d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8')); print([x['id'] for x in d['sessions']][-2:], len(d['not_schedulable']))"
```

Expected: l'elenco termina con l'id nuovo; il secondo numero è quello di prima dell'inserimento (misuralo prima). Line ending (Git Bash): `file docs/roadmap/editor-sessions.yaml` → `with CRLF line terminators`.

- [ ] **Step 4: La riga di conoscenza parziale**

`conoscenza-parziale-visibile-spec.md`, in coda alla tabella `| Canale | Cosa rivelerebbe | Stato | Misura |` di §1.3 (dopo `:119`):

```markdown
| Beat di attivazione (`AbilityActivated`, [#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>)) | che una unità nemica ha agito, e con quale azione, anche senza colpo | ✅ **mostrato solo con sorgente osservata** | `FRTResolvedEvent::SourceVerdict` congelato all'emissione da `FreezeVerdictFor`; `BeginPlayback` e `URTPlaybackLibrary::BuildBlastSequence` lo filtrano con `AllowsTeam(Viewer)`, tutto o niente. La timeline canonica conserva l'evento: è il **playback** a filtrare, come per il `Move`. Pinnato da `Playback.HiddenSourceHasNoActivationBeat` |
```

- [ ] **Step 5: Il radar**

```powershell
node tools/radar/doc-coherence.ts --check
node tools/radar/doc-tables.ts --check
```

Expected: nessun rosso nuovo. Poi **tutti** i controlli del radar, l'elenco si misura: `ls tools/radar/*.ts | grep -v test` e lancia ciascuno con `--check`.

- [ ] **Step 6: Commit**

```bash
git add Scenarios/Visual/Ability/CastBeat.json docs/technical/runbooks/scenari-validazione-visiva.md docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml docs/technical/systems/conoscenza-parziale-visibile-spec.md
git commit -F - <<'EOF'
docs(<n>): voce PIE-CAST-BEAT, seduta e scenario Visual.Ability.CastBeat; riga del beat di attivazione in conoscenza parziale

Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 9: Chiusura — suite, PR, review, merge, issue

**Files:** nessuno. GitHub.

- [ ] **Step 1: Suite completa sul commit finale**

Run: test con filtro `RefactorTactics`, log in scratchpad. Expected: `**** TEST COMPLETE`, `Result={Fail}` = 0. Annota lo SHA (`git rev-parse HEAD`).

- [ ] **Step 2: Push e PR verso il parent**

Il parent è `main` (`git config branch.$(git branch --show-current).parent`). Corpo in `<scratchpad>/pr-momento.md`, `--body-file`, con: cosa; i gate eseguiti (`PASS` col nome del log e lo SHA); le cinque mutazioni con il test caduto; i gate `NOT RUN` (PIE: `PIE-CAST-BEAT`, seduta dello Step 3 del Task 8; Packaged: `NOT RUN`); nessun totale volatile; chiusura `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

```bash
git push -u origin issue/<n>-momento-ability-activated
gh pr create --repo DegrassiAaron/refactor-tactics-main --base main --title "Il momento: AbilityActivated (#<n>)" --body-file "<scratchpad>/pr-momento.md"
```

- [ ] **Step 3: Code review**

`/code-review` sulla PR; accogli i findings con lo skill `receiving-code-review` (verifica, non adesione). Un fix è un commit nuovo e un giro di test sullo SHA nuovo.

- [ ] **Step 4: Merge, pulizia, issue**

Merge dopo review verde. Poi `git branch -D issue/<n>-momento-ability-activated`, `git remote prune origin`. Aggiorna la issue: DoD spuntato nel **commento** di chiusura con lo SHA del merge; la voce PIE resta ⏳ finché l'autore non la giudica a schermo.
