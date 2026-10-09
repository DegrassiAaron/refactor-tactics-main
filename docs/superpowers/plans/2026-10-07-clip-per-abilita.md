# La clip per abilità — una chiave `ActionId` sopra il ruolo di presentazione — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni abilità degli eroi può suonare una clip **propria** sui due beat che conoscono l'azione — `Cast` (`ShowActivation`, il momento di #3549) e `Attack` (`LaunchPlaybackAttack`) — con il ripiego `(ActionId, Ruolo)` → `(BaseActionId, Ruolo)` → `Ruolo`; il default C++ porta la mappa abilità→clip giudicata dall'autore (spec §2.6); il catalogo JSON guadagna la chiave `actionId` (`formatVersion` 2), validata contro le azioni conosciute; il commandlet la traduce in un pool proprio e **fonde per pool** sopra il default; il gate di cook pretende anche le clip d'azione.

**Architecture:** una `USTRUCT` intermedia `FRTActionPresentationClips` (UHT vieta i contenitori annidati) dà a `FRTHeroPresentationClips` una mappa `PerAction` accanto a `PerRole`; un overload a quattro argomenti di `URTUnitAnimInstance::ActiveClipFor` è l'unico punto che risolve i tre livelli. `ARTUnit::ResolvedClipPathFor`/`PlayPresentationRole` guadagnano due argomenti con default (non sono `UFUNCTION`), e un seam per ruolo `LastResolvedClipPathForTest`. Il costruttore del CDO chiama un helper `MakeActionClips` accanto a `MakeClips`. Lato authoring: `FRTAnimBinding::ActionId`, parse/scrittura `actionId`, `ValidateCatalog` con chiave `(Hero, Role, ActionId)` e insieme esplicito delle azioni valide; `BuildClipsPerHero` resta pura e smista per pool, `MergeClipsPerHero` (nuova, pura) fonde sopra il CDO della classe base. Il calcolo del set richiesto dal cook esce in un helper con un test verde proprio.

**Tech Stack:** Unreal Engine 5.8.1, C++ (moduli `RefactorTactics` e `RefactorTacticsEditor`), Automation Test framework, catalogo JSON in `Data/Anim/`, registro PIE in Markdown, sedute in YAML.

**Spec:** `docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md` — il piano argomenta dalla spec; chi esegue legge entrambi. Decisioni D1–D4, `Ruling`, `➕ rev.` e `➕ rev2.` sono chiusi: non si riaprono qui. Dove il piano se ne discosta lo dice con `➕ piano.` e la ragione.

**Stato misurato in pianificazione:** worktree `D:/Repositories/rt-wt-sp3-clip`, `origin/main` = `12bd4def4`. Ogni `file:riga` è stato letto su quel commit; chi esegue più tardi lo **rimisura** prima di toccare il file (`grep -n` sull'ancora testuale citata accanto al numero).

## Global Constraints

- Engine: **UE 5.8.1** in `D:/EpicGames/UE_5.8`. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **Nessun `.uasset`**, nessun Blueprint toccato. I `BP_Unit_*`, `UnitAnimClass` e i riferimenti duri delle clip sono di [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562) (eredita la chiusa #2444); `ABP_RTUnitAuthored` non è versionato e **non si rigenera** in questo lavoro (il commandlet si prova headless sulle sue funzioni pure).
- ⛔ **Nessuna `UFUNCTION` nuova né rinominata**, nessuna `UPROPERTY` nuova su `ARTUnit`: `Unit.BlueprintSurfaceIsCensused` (`Tests/RTUnitBlueprintSurfaceTests.cpp:317`, itera le `FProperty` di `ARTUnit` a `:363`) non cambia. I seam nuovi sono membri C++ nudi, come `CastCuesPlayed` (`Unit/RTUnit.h:1373-1374`).
- ⛔ **`Play*Montage(UAnimSequenceBase*)` invariati** (`Unit/RTUnit.h:1349-1362`): l'`ActionId` non arriva al Blueprint (spec §3).
- ⛔ **Nessun dato nuovo in snapshot, TurnLog o `StateHash`**, nessun tipo di evento nuovo (D-278 invariato): tutto il lavoro è presentazione e authoring. `Hit` e `Death` restano a un argomento (D3).
- **Ordinamento solo esplicito**: nessun esito dipende dall'ordine di iterazione di un `TMap`/`TSet`. I `TSet` nuovi si usano solo con `Contains`/`Add`; i cicli su `TMap` nei test producono asserti indipendenti dall'ordine.
- ⛔ **Nessun totale volatile** in commenti, documenti, celle del registro PIE, YAML, issue, PR (`AGENTS.md` §14): niente «N clip», «N abilità», «N voci». I conteggi di un test restano nel test; le misure di un passaggio si scrivono con il comando che le produce.
- ⛔ **Nessun glifo di stato (⏳ ✅ ❌ 🟡) accanto al nome di una voce PIE in ciò che si pubblica su GitHub**: issue, titolo e corpo della PR, commenti (anche quello di chiusura), messaggi di commit. Lo stato si scrive **in parole**: `PIE: NOT RUN`, `PIE: PASS`. Un glifo citato accanto a un ID PIE in un corpo GitHub fa cadere A3 di `doc-coherence`. Il glifo resta solo dove il formato del repository lo vuole: la cella di stato del registro `test-manuali-pie.md` (Task 7 Step 1).
- Stile: commenti in **italiano**, nella forma dei file toccati (🔴 ⚠️ 🔑 ⛔ ✅ ⏱️ dove il file li usa). Nomi dei test nelle famiglie esistenti `RefactorTactics.<Famiglia>.<Nome>`: `Unit` (`Tests/RTAnimChannelTests.cpp`), `Playback` (`Tests/RTPlaybackActivationTests.cpp`), `Packaging` (`Tests/RTPackagingConfigTests.cpp`), `Anim.Catalog`/`Anim.Bindings`/`Anim.Browser` (`RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp` — i test del modulo Editor stanno in `Source/RefactorTacticsEditor/Private/Tests/`). Helper dei test con nomi **distinti per file** (unity build).
- Line ending: i sorgenti e i Markdown toccati sono **CRLF** (misurato con `file`); `Data/Anim/AnimCatalog.json` è **LF** e resta LF. Dopo ogni modifica: `file <percorso>` deve dire la stessa cosa di prima. ⛔ Niente `sed -i` sui file CRLF (converte a LF).
- Commit: `<type>(3563): <descrizione>`, chiuso da `Co-Authored-By: Claude <modello> <noreply@anthropic.com>`. Un commit per task. ⚠️ **Il trailer nomina il modello che ESEGUE il task**, non quello che ha scritto il piano: l'implementatore sostituisce `<modello>` con il proprio (es. `Claude Sonnet 5.5`) in ogni blocco di commit qui sotto.
- Branch: `issue/3563-clip-per-abilita`, da `origin/main` (Task 0). `3563` è il numero della issue aperta nel Task 0, e si sostituisce ovunque compaia (codice, commenti, documenti).
- `<checkout>` = la cartella in cui sta il branch (questo worktree, `D:/Repositories/rt-wt-sp3-clip`, o un altro). `<scratchpad>` = la cartella scratchpad della sessione.
- **Motore uno per macchina** (`CLAUDE.md` §10, `AGENTS.md` §11): prima di ogni build o run, `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`. Se un altro clone misura **tempi**, si aspetta; altrimenti si procede. Se non si può aspettare: `NOT RUN` col nome del clone che tiene il motore.
- Build (Editor chiuso su questo clone):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="<checkout>/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
  ```
- Test (filtro dopo `RunTests`, `+` fra più filtri, `;Quit` separato, `-abslog` **fra virgolette**):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<checkout>/RefactorTactics.uproject" "-ExecCmds=Automation RunTests <filtro>;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-abslog=<scratchpad>/<nome-parlante>.log"
  ```
  Un esito vale **solo** se il log porta `**** TEST COMPLETE`. Conteggio (Git Bash): `grep -c '\*\*\*\* TEST COMPLETE' <log>` (atteso 1), `grep -c 'Result={Success}' <log>`, `grep -c 'Result={Fail}' <log>` (atteso 0). Un rosso si legge con `grep -n 'Result={Fail}' -A3 <log>`.
- **Una mutazione** è: modifica della riga indicata, build, test indicato, `Result={Fail}` sull'asserto indicato, ripristino (`git diff` vuoto sul file), build, verde. Se una mutazione **non** fa cadere il test, ci si ferma: il test è vacuo, e si riporta — non si allenta l'asserto.

## Review Focus

Cinque ingressi che la spec implica e che i test «di spec» (§5.1) non esercitano. Ciascuno ha il suo test, **scritto per intero** nello Step 1 del task che possiede il codice; chi rivede controlla che ci sia e che sia rosso prima del codice:

1. **(a) Un `ActionId` con voce, ma per un ruolo diverso da quello richiesto** (voce solo `Cast`, richiesta `Attack`) → ripiego alla generica o al ruolo, mai nullo → Task 1 Step 1, `Unit.ActionEntryWithoutTheRoleFallsBack` (mutazione (P1): la voce dell'azione ferma la ricerca).
2. **(b) Binding d'azione attivo + binding di ruolo attivo per lo stesso `(eroe, ruolo)`**: entrambi validi, l'azione vince → Task 5 Step 1, `Anim.Bindings.ActionAndRoleBindingsCoexist` (con la metà «validi» anche in Task 4, `Anim.Catalog.RejectsTwoActivePerActionRole`, controllo positivo).
3. **(c) `formatVersion` 2 senza nessun `actionId`** (catalogo solo di ruolo) deve caricare → Task 4 Step 1, `Anim.Catalog.FormatVersion2WithoutActionIdLoads`.
4. **(d) Un'abilità tolta dal catalogo eroi mentre resta nella mappa di `MakeActionClips`** → Task 3 Step 2, `Unit.DefaultActionClipsResolveForEveryKitAbility` (la lista attesa è una funzione di `URTHeroCatalogLibrary::GetHeroRoster()`, e il test controlla **anche il verso opposto**: ogni chiave di `PerAction` nel CDO è un'abilità di quell'eroe; mutazione (P3)).
5. **(e) `LaunchPlaybackAttack` per l'impatto di una carica** (`ActionId` = lo scatto, `Impact.Def = Dash->Def`, `Turn/RTTurnManager.cpp:4746`) risolve la clip `Attack` dello scatto → Task 2 Step 1, `Playback.ChargeImpactPlaysTheDashAttackClip` (mutazione (6)).

Mutazioni della spec (§5.1) e del piano, e dove si eseguono: (1) ordine dei livelli → Task 1 Step 6; (2) una riga del default tolta → Task 3 Step 6; (3) `ShowActivation` a un argomento → Task 2 Step 6; (4) `PerAction` saltato nel set richiesto → Task 6 Step 5; (5) azione ignota non controllata → Task 4 Step 6; (6) `LaunchPlaybackAttack` a un argomento → Task 2 Step 6; (7) quarto predicato di `MakeActive` senza `ActionId` → Task 5 Step 6; (8) `MergeClipsPerHero` = assegnazione → Task 5 Step 6; (9) controllo del ruolo tolto → Task 4 Step 6; più (P1)–(P6) del piano, ciascuna nel suo task.

### Dove sono i cinque punti della re-review (`➕ rev2.`)

| Punto | Task / Step |
|---|---|
| `BuildClipsPerHero` pura e invariata, `MergeClipsPerHero` seconda funzione pura sul CDO della **classe base**, test `MergeKeepsDefaultPools`, mutazione (8), commenti `:96`, `:126`, `:155-156` | Task 5, Step 1 (test), Step 3 (codice e commenti), Step 6 (mutazione) |
| Azioni concesse dall'equipaggiamento nell'insieme valido (R10), `RejectsActionIdOnNonPropagatingRole` (mutazione (9)) e `AcceptsEquipmentActionId` | Task 4, Step 1 (test), Step 4 (`RTAzioniConosciute`, enumerazione misurata), Step 6 (mutazione) |
| Messaggio del gate di cook `#2444` → `#3562` (`RTPackagingConfigTests.cpp:724-730`) | Task 6, Step 3 |
| `LaunchPlaybackAttack` (`RTTurnManager.cpp:8256`) parte davvero nella fixture: asserto positivo prima della mutazione (6), ed estensione della fixture se non parte | Task 2, Step 1 (premesse sulla traccia `L0`), Step 4 (verifica), Step 5 (se non parte) |
| `RTUnitTests.cpp:611-612`; §4 e §6 nuovi della spec nel runbook | Task 3, Step 5 (commento); Task 7, Step 4 (runbook) |

---

### Task 0: Issue, branch, spec e piano nel repository

**Files:** `docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md` (create), `docs/superpowers/plans/2026-10-07-clip-per-abilita.md` (create). GitHub: una issue nuova sotto l'epic di presentazione [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453).

**Interfaces:**
- Produces: il numero `3563`, usato da branch, commit, commenti e documenti.

- [ ] **Step 1: Apri la issue** (azione verso l'esterno: chiedere conferma all'autore se non già data)

Corpo in `<scratchpad>/issue-clip.md`, poi `--body-file` (mai `--body` inline: i backtick vengono eseguiti). ⛔ Nessun glifo di stato accanto al nome di una voce PIE nel corpo: la voce si nomina e basta.

```markdown
> **Epic**: #2453 · **Refs**: #3532 (sotto-progetto 1) · #3549 (sotto-progetto 2) · #3562 (cablaggio dei BP_Unit_ e riferimenti duri) · **Spec**: `docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md`

## Why

Una clip per (eroe, ruolo), mai per abilità: `URTUnitAnimInstance::ClipsPerHero` porta `PerRole` e basta, e ogni abilità di un eroe suona la stessa sequenza. L'evento porta già `ActionId` e `BaseActionId`, ma `ShowActivation` e `LaunchPlaybackAttack` li scartano. Terzo dei quattro sotto-progetti di «associare animazioni e FX alle skill e vederle in azione».

## Scope

1. Modello dati: `FRTActionPresentationClips`, `PerAction` in `FRTHeroPresentationClips`, `ActiveClipFor` a quattro argomenti con ripiego `(ActionId, Ruolo)` → `(BaseActionId, Ruolo)` → `Ruolo`.
2. Runtime: `ResolvedClipPathFor`/`PlayPresentationRole` con l'azione; `ShowActivation` (Cast) e `LaunchPlaybackAttack` (Attack) la passano; seam `LastResolvedClipPathForTest` per ruolo.
3. Default C++: la mappa abilità→clip della spec §2.6 in `MakeActionClips`, ogni nome misurato sul disco prima di entrare.
4. Catalogo JSON: `actionId` nei binding, `formatVersion` 2, `ValidateCatalog` con chiave `(eroe, ruolo, azione)` e insieme esplicito delle azioni valide (core, generiche, eroi, equipaggiamento).
5. Commandlet e modello del browser: pool d'azione, fusione per pool sopra il default, predicati con `ActionId` opzionale. Nessuna UI nuova.
6. Gate di cook: il set richiesto include le clip d'azione, con un test verde proprio; il gate resta rosso per i riferimenti duri (#3562).
7. Voce PIE nuova e seduta; runbook delle animazioni.

## Out of scope

Pannello di legame nell'Anim Browser; `UnitAnimClass`, `ABP_RTUnitAuthored` e riferimenti duri nei `BP_Unit_*` (#3562); `ActionId` nei `Play*Montage` Blueprint; clip per `Hit`/`Death`; FX (sotto-progetto 4).

## DoD

- [ ] I test della spec §5.1 verdi sul commit reale, log con `**** TEST COMPLETE`.
- [ ] Le mutazioni (1)–(9) della spec e quelle che il piano aggiunge eseguite: ciascuna fa cadere il test dichiarato.
- [ ] `Unit.BlueprintSurfaceIsCensused` e `Unit.DiscreteRoleClipsMatchThePacks` verdi e invariati.
- [ ] `Packaging.RequiredSetIncludesActionClips` verde; `Packaging.RequiredAnimationClipsAreCooked` rosso dichiarato, con il set rimisurato prima e dopo.
- [ ] Build `RefactorTacticsEditor` verde.
- [ ] Voce `PIE-CLIP-ABILITA` e seduta nel file delle sedute; il verdetto a schermo è dell'autore.
- [ ] Nessun `.uasset`; nessun totale volatile in ciò che si pubblica.
```

```powershell
gh issue create --repo DegrassiAaron/refactor-tactics-main --title "La clip per abilita': una chiave ActionId sopra il ruolo di presentazione" --body-file "<scratchpad>/issue-clip.md" --label enhancement
```

Rileggi dal server e conta i backtick: `gh issue view 3563 --json body -q .body | grep -o '`' | wc -l` deve coincidere con `grep -o '`' <scratchpad>/issue-clip.md | wc -l`.

- [ ] **Step 2: Branch da `origin/main`, e spec e piano nel repository**

```bash
git -C <checkout> fetch -q origin
git -C <checkout> switch -c issue/3563-clip-per-abilita origin/main
git -C <checkout> config branch.issue/3563-clip-per-abilita.parent main
```

Copia la spec rivista (`<scratchpad>/sp3/2026-10-07-clip-per-abilita-design.md`) in `docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md` e questo piano (`<checkout>/.superpowers/sp3-plan.md`) in `docs/superpowers/plans/2026-10-07-clip-per-abilita.md`. Sostituisci `3563` in entrambi. Line ending come il piano SP2 (misurato: CRLF):

```powershell
foreach ($f in "docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md","docs/superpowers/plans/2026-10-07-clip-per-abilita.md") {
  $t = [IO.File]::ReadAllText("<checkout>/$f") -replace "`r`n","`n" -replace "`n","`r`n"
  [IO.File]::WriteAllText("<checkout>/$f", $t, [Text.UTF8Encoding]::new($false))
}
```

`file` su entrambi → `with CRLF line terminators`. `.superpowers/` è ignorata da git (`.gitignore:297`): non entra nel commit.

```bash
git add docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md docs/superpowers/plans/2026-10-07-clip-per-abilita.md
git commit -F - <<'EOF'
docs(3563): spec e piano della clip per abilita'

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 1: Il modello dati — `PerAction` e `ActiveClipFor` a tre livelli

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnitAnimInstance.h:129-150` (struct nuova dopo `FRTAnimRoleClips`, campo `PerAction` in `FRTHeroPresentationClips`) e `:190-197` (overload)
- Modify: `Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp:168-183` (overload dopo l'esistente)
- Test: `Source/RefactorTactics/Tests/RTAnimChannelTests.cpp` (include a `:1-12`, helper in coda al namespace `:30-103`, test in coda prima di `#endif` a `:333`)

**Interfaces:**
- Produces:
  ```cpp
  USTRUCT(BlueprintType) struct FRTActionPresentationClips { TMap<ERTPresentationRole, FRTAnimRoleClips> PerRole; };
  // in FRTHeroPresentationClips:
  TMap<FName, FRTActionPresentationClips> PerAction;
  // in URTUnitAnimInstance:
  TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
      const FName& ActionId, const FName& BaseActionId) const;
  ```
- Consumes: `FRTAnimRoleClips::AddVariant`/`MakeActive`/`FindActive` (`RTUnitAnimInstance.h:86-111`), invariati.

- [ ] **Step 1: I test rossi**

`RTAnimChannelTests.cpp`, dopo `#include "Ability/RTHeroCatalogLibrary.h"` (`:12`):

```cpp
#include "Misc/ScopeExit.h"
```

In coda al namespace anonimo, prima della `}` di `:103`:

```cpp

	// --- Le voci per azione (#3563, spec «la clip per abilita'» §2.1-§2.2) -------------------------------------

	/** Due eroi sintetici: non toccano il roster nel CDO, e `GenericActionClipIsSharedAcrossHeroes` ne vuole DUE. */
	const FName IdAzioneDiProva(TEXT("Hero.AzioneDiProva"));
	const FName IdAzioneDiProvaBis(TEXT("Hero.AzioneDiProvaBis"));

	const TCHAR* PathRuoloCast    = TEXT("/Game/Prova/RuoloCast.RuoloCast");
	const TCHAR* PathRuoloAttacco = TEXT("/Game/Prova/RuoloAttacco.RuoloAttacco");
	const TCHAR* PathProfilo      = TEXT("/Game/Prova/Profilo.Profilo");
	const TCHAR* PathGenerica     = TEXT("/Game/Prova/Generica.Generica");

	/** Un pool con una sola variante, attiva: la forma di `MakeRuolo` del default, con un path sintetico. */
	FRTAnimRoleClips PoolAzioneDiProva(const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_ProvaPool")), FName(TEXT("A")),
			TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_ProvaPool")));
		return Pool;
	}

	/**
	 * Scrive nel CDO la voce di `Eroe` con i ruoli `Cast` e `Attack` popolati e NESSUNA azione — la stessa
	 * disciplina di `ConfiguraVariante` (`:75-96`): il ripiego sul ruolo e' il controllo positivo di ogni
	 * asserto sotto, e senza un path di ruolo «e' tornato il ruolo» e «e' tornato nulla» sarebbero lo stesso.
	 *
	 * ⚠️ Restituisce un riferimento dentro `ClipsPerHero`: lo si usa SUBITO, prima di aggiungere un altro eroe
	 * (un `Add` successivo puo' riallocare la mappa).
	 */
	FRTHeroPresentationClips& ConfiguraVoceAzione(const FName& Eroe)
	{
		FRTHeroPresentationClips Voce;
		Voce.PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathRuoloCast));
		Voce.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathRuoloAttacco));
		return GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero.Add(Eroe, Voce);
	}

	/** Come `PulisciVariante` (`:99-102`): il CDO e' stato globale. */
	void PulisciVoceAzione()
	{
		URTUnitAnimInstance* Cdo = GetMutableDefault<URTUnitAnimInstance>();
		Cdo->ClipsPerHero.Remove(IdAzioneDiProva);
		Cdo->ClipsPerHero.Remove(IdAzioneDiProvaBis);
	}

	FString PathDi(const TSoftObjectPtr<UAnimSequenceBase>& Clip) { return Clip.ToSoftObjectPath().ToString(); }
```

In coda al file, prima di `#endif // WITH_DEV_AUTOMATION_TESTS` (`:333`):

```cpp

/**
 * La clip d'AZIONE vince su quella di ruolo, e senza voce si torna al ruolo — spec «la clip per abilita'» §2.2, D2.
 *
 * 🔑 **Controllo positivo e ripiego nello stesso test**: con la sola voce di ruolo il risultato e' il path di ruolo;
 * aggiunta la voce d'azione, e' il path d'azione. Lo stesso eroe, la stessa chiamata: cambia solo il dato, e il
 * risultato deve cambiare con lui.
 * ✅ Validato per mutazione (1): il ruolo consultato PRIMA delle azioni fa cadere «l'azione vince».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitActionClipWinsOverRoleClipTest,
	"RefactorTactics.Unit.ActionClipWinsOverRoleClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitActionClipWinsOverRoleClipTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Profilo(TEXT("Hero.AzioneDiProva.Colpo"));
	const FName Generica(TEXT("Action.BasicAttack"));

	ConfiguraVoceAzione(IdAzioneDiProva);
	TestEqual(TEXT("senza voce d'azione: il ripiego e' il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathRuoloCast));

	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(Profilo).PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathProfilo));
	TestEqual(TEXT("🔴 con la voce d'azione attiva: l'azione vince sul ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathProfilo));
	TestEqual(TEXT("l'overload a due argomenti resta il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast)), FString(PathRuoloCast));

	// Una voce d'azione con la variante NON attiva e' «non popolata»: si torna al ruolo (spec §4).
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction[Profilo].PerRole[ERTPresentationRole::Cast].ActiveClipVariant = NAME_None;
	TestEqual(TEXT("voce d'azione senza variante attiva: ripiego sul ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathRuoloCast));
	return true;
}

/**
 * La generica (`BaseActionId`) e' condivisa fra eroi, e il profilo la batte — spec §2.2, D2.
 * ✅ Validato per mutazione (P2): i due livelli d'azione in ordine inverso fanno cadere «il profilo batte la generica».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitGenericActionClipIsSharedAcrossHeroesTest,
	"RefactorTactics.Unit.GenericActionClipIsSharedAcrossHeroes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitGenericActionClipIsSharedAcrossHeroesTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Generica(TEXT("Action.BasicAttack"));
	const FName ProfiloUno(TEXT("Hero.AzioneDiProva.Colpo"));
	const FName ProfiloDue(TEXT("Hero.AzioneDiProvaBis.Colpo"));

	// ⚠️ Un eroe per volta: il riferimento di `ConfiguraVoceAzione` non sopravvive all'`Add` del secondo.
	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));
	ConfiguraVoceAzione(IdAzioneDiProvaBis).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));

	TestEqual(TEXT("primo eroe, profilo senza voce: la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, ProfiloUno, Generica)), FString(PathGenerica));
	TestEqual(TEXT("secondo eroe, profilo senza voce: la STESSA generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProvaBis, ERTPresentationRole::Attack, ProfiloDue, Generica)), FString(PathGenerica));

	// 🔴 Il profilo batte la generica: e' il primo livello, la generica il secondo.
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(ProfiloUno).PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathProfilo));
	TestEqual(TEXT("🔴 con entrambe popolate il profilo batte la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, ProfiloUno, Generica)), FString(PathProfilo));
	TestEqual(TEXT("e il secondo eroe, senza profilo, resta sulla generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProvaBis, ERTPresentationRole::Attack, ProfiloDue, Generica)), FString(PathGenerica));
	return true;
}

/**
 * `BaseActionId` si legge dall'EVENTO e non si indovina — `Ruling` di §2.2, `Turn/RTResolvedEvent.h:393-397`.
 *
 * 🔑 Con la generica popolata e nessuna voce per il profilo: passando `Action.BasicAttack` si risolve la generica
 * (controllo positivo); passando `NAME_None` si risolve il RUOLO. Un `ActiveClipFor` che derivasse la generica dal
 * profilo darebbe la generica anche nel secondo caso.
 * ✅ Validato per mutazione (P4): la generica derivata quando manca fa cadere il secondo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitBaseActionIdIsNeverDerivedTest,
	"RefactorTactics.Unit.BaseActionIdIsNeverDerived",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitBaseActionIdIsNeverDerivedTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Generica(TEXT("Action.BasicAttack"));
	const FName Profilo(TEXT("Hero.AzioneDiProva.Colpo"));

	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));

	TestEqual(TEXT("controllo positivo: con BaseActionId dichiarato si risolve la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, Generica)), FString(PathGenerica));
	TestEqual(TEXT("🔴 con BaseActionId vuoto si salta il livello: il RUOLO, non la generica indovinata"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, NAME_None)), FString(PathRuoloAttacco));
	TestEqual(TEXT("e con entrambi vuoti, il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, NAME_None, NAME_None)), FString(PathRuoloAttacco));
	return true;
}

/**
 * Review Focus (a): una voce per l'azione che NON ha il ruolo richiesto non ferma la ricerca.
 *
 * 🔴 Il caso reale: `Hero.Muiren.TideGuard` ha solo un beat `Cast` (spec §2.6). Un `Attack` con quella chiave — o
 * qualunque ruolo assente dalla voce — deve passare alla generica, poi al ruolo; mai restituire nulla perche' «la
 * voce dell'azione c'era».
 * ✅ Validato per mutazione (P1): `return` incondizionato quando la voce dell'azione esiste → cade il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitActionEntryWithoutTheRoleFallsBackTest,
	"RefactorTactics.Unit.ActionEntryWithoutTheRoleFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitActionEntryWithoutTheRoleFallsBackTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Profilo(TEXT("Hero.AzioneDiProva.Scudo"));
	const FName Generica(TEXT("Action.BasicAttack"));

	// Il profilo ha SOLO `Cast`.
	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Profilo)
		.PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathProfilo));

	const TSoftObjectPtr<UAnimSequenceBase> Attacco =
		Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, NAME_None);
	TestFalse(TEXT("🔴 voce solo Cast, richiesta Attack: NON nulla"), Attacco.IsNull());
	TestEqual(TEXT("e' la clip di ruolo Attack"), PathDi(Attacco), FString(PathRuoloAttacco));

	// Con la generica popolata per Attack, il ripiego si ferma li', prima del ruolo.
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(Generica).PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));
	TestEqual(TEXT("voce solo Cast, richiesta Attack, generica popolata: la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, Generica)), FString(PathGenerica));
	TestEqual(TEXT("controllo positivo: il Cast dello stesso profilo e' la voce d'azione"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathProfilo));
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build.
Expected: **rosso di compilazione** — `PerAction` non è un membro di `FRTHeroPresentationClips` e `ActiveClipFor` non ha un overload a quattro argomenti (`error C2039` su `PerAction`, `C2660` su `ActiveClipFor`). È il rosso di questo passo: i test non esistono senza il modello.

- [ ] **Step 3: La struct e il campo**

`RTUnitAnimInstance.h`, dopo la `};` di `FRTAnimRoleClips` (`:129`), prima del commento di `FRTHeroPresentationClips` (`:131`):

```cpp

/**
 * Le clip di UN'azione, per ruolo di presentazione (#3563, spec «la clip per abilita'» §2.1).
 *
 * 🔴 **Esiste per lo stesso vincolo di `FRTHeroPresentationClips`**: UHT non ammette contenitori ANNIDATI come
 * `UPROPERTY`, e `TMap<FName, TMap<ERTPresentationRole, ...>>` non compila.
 *
 * ⚠️ **Un pool DISTINTO da quello di ruolo**, e «una sola attiva» vale per pool: una clip di ruolo e una d'azione
 * attive per lo stesso `(eroe, ruolo)` convivono, e a risolvere vince l'azione (`ActiveClipFor` a quattro
 * argomenti). L'alternativa — un secondo `ActiveClipVariant` per azione dentro il pool di ruolo — mescolava le
 * varianti dei due livelli e rendeva «una sola attiva» ambiguo (spec §2.1, alternativa scartata).
 */
USTRUCT(BlueprintType)
struct FRTActionPresentationClips
{
	GENERATED_BODY()

	/** Solo i ruoli che qualcuno ha popolato. In v0.1 li consultano solo `Cast` e `Attack` (spec D3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<ERTPresentationRole, FRTAnimRoleClips> PerRole;
};
```

(`➕ piano.` La spec §2.1 scrive `USTRUCT()` ed `EditDefaultsOnly, Category = "RT|Anim"`; qui si usa la forma di `FRTAnimRoleClips`/`FRTHeroPresentationClips` dello stesso file, `BlueprintType` + `EditAnywhere, BlueprintReadOnly, "RefactorTactics|Anim"`: una struct annidata in una `UPROPERTY` `BlueprintReadOnly` dev'essere `BlueprintType`, e la categoria è quella del file. Non tocca `ARTUnit`, quindi il censimento resta invariato.)

In `FRTHeroPresentationClips`, dopo `PerRole` (`:146`), prima di `FindRole` (`:148`):

```cpp

	/**
	 * Clip per `(ActionId, ruolo)` (#3563). Chiave: l'`ActionId` dell'evento — un profilo `Hero.X.Y` o una generica
	 * `Action.Z` — mai derivato a valle (`RTResolvedEvent.h:393-397`).
	 *
	 * ⚠️ Un'azione assente, un ruolo assente o una variante non attiva sono tutti «non popolato», e la risoluzione
	 * passa al livello successivo: e' `ActiveClipFor` a quattro argomenti a saperlo, non chi chiama.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<FName, FRTActionPresentationClips> PerAction;
```

- [ ] **Step 4: L'overload**

`RTUnitAnimInstance.h`, dopo la dichiarazione di `:197`:

```cpp

	/**
	 * La clip attiva di `Role` per `HeroId` quando il beat conosce l'AZIONE (#3563, spec D2): `PerAction[ActionId]`,
	 * poi `PerAction[BaseActionId]`, poi `PerRole` — l'overload a due argomenti qui sopra.
	 *
	 * 🔴 **Ogni livello «non popolato» passa al successivo**: azione assente, ruolo assente dalla voce dell'azione,
	 * nessuna variante attiva. Una voce d'azione che ha `Cast` ma non `Attack` NON ferma un `Attack`.
	 * ⛔ **`BaseActionId` vuoto salta il proprio livello**: non si deriva dal profilo (`Ruling` di §2.2).
	 */
	TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
		const FName& ActionId, const FName& BaseActionId) const;
```

`RTUnitAnimInstance.cpp`, dopo la `}` di `ActiveClipFor` (`:183`):

```cpp

TSoftObjectPtr<UAnimSequenceBase> URTUnitAnimInstance::ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
	const FName& ActionId, const FName& BaseActionId) const
{
	if (const FRTHeroPresentationClips* Eroe = FindClipsFor(HeroId))
	{
		// 🔑 L'ORDINE dei livelli e' la decisione D2: il profilo, poi la generica condivisa fra eroi.
		for (const FName& Chiave : { ActionId, BaseActionId })
		{
			if (Chiave.IsNone())
			{
				continue;   // un livello senza chiave si salta: non si indovina
			}
			const FRTActionPresentationClips* Azione = Eroe->PerAction.Find(Chiave);
			if (Azione == nullptr)
			{
				continue;
			}
			const FRTAnimRoleClips* Pool = Azione->PerRole.Find(Role);
			const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
			if (Attiva != nullptr)
			{
				return Attiva->Clip;
			}
			// ⚠️ La voce dell'azione c'era ma non per questo ruolo, o senza attiva: si prosegue (Review Focus (a)).
		}
	}
	// Il ripiego e' la clip di ruolo di oggi, con le sue tre uscite a nulla tutte normali.
	return ActiveClipFor(HeroId, Role);
}
```

- [ ] **Step 5: Build e verde**

Run: build; test `RefactorTactics.Unit+RefactorTactics.Anim`.
Expected: verdi, `**** TEST COMPLETE`, `Result={Fail}` = 0. `Unit.CastRoleResolvesAClipForEveryHero`, `Unit.DiscreteRoleClipsMatchThePacks` e `Unit.BlueprintSurfaceIsCensused` verdi e invariati.

- [ ] **Step 6: Mutazioni (1), (P1), (P2), (P4)**

Una per volta, ciascuna seguita da ripristino, build e verde:

| Mutazione (in `ActiveClipFor` a quattro argomenti) | Test | Asserto che deve cadere |
|---|---|---|
| (1) Prima riga del corpo: `if (!ActiveClipFor(HeroId, Role).IsNull()) { return ActiveClipFor(HeroId, Role); }` (il ruolo consultato per primo) | `Unit.ActionClipWinsOverRoleClip` | «🔴 con la voce d'azione attiva: l'azione vince sul ruolo» |
| (P1) La riga `if (Attiva != nullptr)` sostituita da `return Attiva ? Attiva->Clip : TSoftObjectPtr<UAnimSequenceBase>(nullptr);` (la voce dell'azione ferma la ricerca) | `Unit.ActionEntryWithoutTheRoleFallsBack` | «🔴 voce solo Cast, richiesta Attack: NON nulla» |
| (P2) `{ ActionId, BaseActionId }` → `{ BaseActionId, ActionId }` | `Unit.GenericActionClipIsSharedAcrossHeroes` | «🔴 con entrambe popolate il profilo batte la generica» |
| (P4) `{ ActionId, BaseActionId }` → `{ ActionId, BaseActionId.IsNone() ? FName(TEXT("Action.BasicAttack")) : BaseActionId }` | `Unit.BaseActionIdIsNeverDerived` | «🔴 con BaseActionId vuoto si salta il livello…» |

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Unit/RTUnitAnimInstance.h Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp Source/RefactorTactics/Tests/RTAnimChannelTests.cpp
git commit -F - <<'EOF'
feat(3563): clip per azione nel modello - PerAction e ActiveClipFor a tre livelli con ripiego sul ruolo

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 2: `ARTUnit` e i due consumatori — il beat conosce l'azione

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnit.h:1336` (`PlayPresentationRole`), `:1346` (`ResolvedClipPathFor`), `:1364-1376` (seam)
- Modify: `Source/RefactorTactics/Unit/RTUnit.cpp:723-740` e `:742-747`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:8103` (`ShowActivation`) e `:8289` (`LaunchPlaybackAttack`)
- Test: `Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp` (include a `:15-33`, helper in coda al namespace `:37-118`, test in coda al file, dopo il namespace di `:188-206` che dichiara `BeatIndiceAbilita` e `BeatMappa`)

**Interfaces:**
- Consumes: `URTUnitAnimInstance::ActiveClipFor(HeroId, Role, ActionId, BaseActionId)` (Task 1); `FRTResolvedEvent::ActionId`/`BaseActionId` (`Turn/RTResolvedEvent.h:387, 405`); `ARTTurnManager::bRecordAttackBeatsForTest`/`AttackBeatTraceForTest()` (`Turn/RTTurnManager.h:643, 652`).
- Produces:
  ```cpp
  // ARTUnit, non UFUNCTION: i chiamanti a un argomento compilano invariati
  void PlayPresentationRole(ERTPresentationRole Ruolo, FName ActionId = NAME_None, FName BaseActionId = NAME_None);
  TSoftObjectPtr<UAnimSequenceBase> ResolvedClipPathFor(ERTPresentationRole Ruolo,
      FName ActionId = NAME_None, FName BaseActionId = NAME_None) const;
  FSoftObjectPath LastResolvedClipPathForTest(ERTPresentationRole Ruolo) const; // vuoto se mai risolto
  ```

🔑 **Path sintetici iniettati, come prescrive `Ruling` R11 della spec (§5.1).** `Playback.ActivationPlaysTheActionClip` e `Playback.ChargeImpactPlaysTheDashAttackClip` scrivono nel CDO voci `PerAction` con path che non esistono nei pack, e ripristinano la mappa alla fine: provano la catena evento → `ActionId` → risoluzione senza dipendere dal default del Task 3, e la mappa non vive in una terza copia (§6 ne dichiara due). Che i path del default siano quelli giusti lo dice `Unit.DefaultActionClipsResolveForEveryKitAbility` (Task 3); che suonino a schermo, la seduta PIE (Task 7). Le mutazioni (3) e (6) cadono sullo Scudo e sul Tiratore, come §5.1 chiede.

- [ ] **Step 1: I test rossi**

`RTPlaybackActivationTests.cpp`, dopo `#include "Unit/RTUnit.h"` (`:20`):

```cpp
#include "Unit/RTUnitAnimInstance.h"
```

In coda al namespace anonimo, prima della `}` di `:118`:

```cpp

	// --- La clip per abilita' (#3563) ------------------------------------------------------------------------

	/** Path sintetici: non esistono nei pack, quindi non si confondono con una clip vera del default. */
	const TCHAR* ClipAzioneScudo    = TEXT("/Game/Prova/AzioneTideGuard.AzioneTideGuard");
	const TCHAR* ClipAzioneTiratore = TEXT("/Game/Prova/AzioneImpactShot.AzioneImpactShot");
	const TCHAR* ClipCaricaLancio   = TEXT("/Game/Prova/AzioneRamCast.AzioneRamCast");
	const TCHAR* ClipCaricaImpatto  = TEXT("/Game/Prova/AzioneRamImpatto.AzioneRamImpatto");

	/** Una variante attiva in `PerAction[ActionId][Ruolo]` dell'eroe, nel CDO di `URTUnitAnimInstance`. */
	void IniettaClipAzioneBeat(const FName& HeroId, const FName& ActionId, ERTPresentationRole Ruolo, const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_ProvaBeat")), FName(TEXT("A")),
			TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_ProvaBeat")));
		GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero.FindOrAdd(HeroId)
			.PerAction.FindOrAdd(ActionId).PerRole.Add(Ruolo, Pool);
	}
```

In coda al file, prima di `#endif // WITH_DEV_AUTOMATION_TESTS` — cioè **dopo** il secondo namespace anonimo (`:188-206`), che dichiara `BeatIndiceAbilita` (`:192`) e `BeatMappa` (`:201`): i test li riusano invece di duplicarli, e in C++ devono venire dopo la loro dichiarazione:

```cpp

/**
 * Il beat conosce l'AZIONE: il cast dello Scudo suona la clip di TideGuard, l'attacco del Tiratore quella di
 * ImpactShot — spec «la clip per abilita'» §2.2, D3.
 *
 * 🔴 **Prima misura che il beat `Attack` PARTA** (`L0` nella traccia dei battiti): nessun altro test su questa
 * fixture asserisce un attacco — contano le cue `Cast` — e senza la premessa la mutazione (6) sarebbe vacua.
 * 🔑 Il `Cast` del Tiratore e' il ripiego (nessuna voce per `ImpactShot`/`Cast`, nessuna generica): e' il
 * controllo positivo dello stesso turno.
 * ✅ Validato per mutazione: (3) `ShowActivation` a un argomento → cade sullo Scudo; (6) `LaunchPlaybackAttack` a
 * un argomento → cade sul Tiratore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationPlaysTheActionClipTest,
	"RefactorTactics.Playback.ActivationPlaysTheActionClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationPlaysTheActionClipTest::RunTest(const FString&)
{
	const TMap<FName, FRTHeroPresentationClips> Salvato = GetDefault<URTUnitAnimInstance>()->ClipsPerHero;
	ON_SCOPE_EXIT{ GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero = Salvato; };
	IniettaClipAzioneBeat(FName(TEXT("Hero.Muiren")), FName(TEXT("Hero.Muiren.TideGuard")),
		ERTPresentationRole::Cast, ClipAzioneScudo);
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), FName(TEXT("Hero.Branth.ImpactShot")),
		ERTPresentationRole::Attack, ClipAzioneTiratore);

	FRTBeatDiProva B;
	const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }
	B.TM->bRecordAttackBeatsForTest = true;
	B.TM->LockInAndResolve();
	FinoAllaFineDelPlayback(B.TM);

	// ⛔ Le premesse: i due beat sono PARTITI. Senza, ogni asserto sotto confronterebbe un path vuoto.
	if (!TestEqual(TEXT("⛔ premessa: il cast dello scudo e' suonato"), B.Scudo->CastCuesPlayedForTest(), 1)
		|| !TestTrue(TEXT("⛔ premessa: LaunchPlaybackAttack e' partito (L0 nella traccia dei battiti)"),
			B.TM->AttackBeatTraceForTest().Contains(TEXT("L0"))))
	{
		return false;
	}

	TestEqual(TEXT("🔴 il cast dello scudo suona la clip d'azione di TideGuard"),
		B.Scudo->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(), FString(ClipAzioneScudo));
	TestEqual(TEXT("🔴 l'attacco del tiratore suona la clip d'azione di ImpactShot"),
		B.Tiratore->LastResolvedClipPathForTest(ERTPresentationRole::Attack).ToString(), FString(ClipAzioneTiratore));
	TestEqual(TEXT("controllo positivo: il cast del tiratore non ha voce, e' il ruolo"),
		B.Tiratore->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(),
		GetDefault<URTUnitAnimInstance>()->ActiveClipFor(FName(TEXT("Hero.Branth")), ERTPresentationRole::Cast)
			.ToSoftObjectPath().ToString());
	return true;
}

/**
 * Review Focus (e): l'impatto di una carica porta l'`ActionId` dello SCATTO (`Impact.Def = Dash->Def`,
 * `RTTurnManager.cpp:4746`), quindi suona la clip `Attack` dello scatto — spec §2.6, riga di `Hero.Branth.Ram`.
 *
 * ⚠️ Il caricatore e' della squadra 0 e il test non crea nessun controller: il viewer e' il ripiego sulla squadra 0
 * (`RTPlaybackActivationTests.cpp:9-10`), cioe' la squadra delle sorgenti. E' la fixture di
 * `Playback.DashStepLandsOnCellsAfterTheActivations` (`:326-379`: `BeatMappa`, mondo non inizializzato, viewer di
 * ripiego), con il bersaglio adiacente alla cella d'arrivo come in `Turn.ChargeActivatesInDashNotInBlast`
 * (`Tests/RTAbilityActivatedTests.cpp:715-756`); qui si asserisce il PATH suonato.
 * ✅ Validato per mutazione (6).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackChargeImpactPlaysTheDashAttackClipTest,
	"RefactorTactics.Playback.ChargeImpactPlaysTheDashAttackClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackChargeImpactPlaysTheDashAttackClipTest::RunTest(const FString&)
{
	const TMap<FName, FRTHeroPresentationClips> Salvato = GetDefault<URTUnitAnimInstance>()->ClipsPerHero;
	ON_SCOPE_EXIT{ GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero = Salvato; };
	const FName Ram(TEXT("Hero.Branth.Ram"));
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), Ram, ERTPresentationRole::Cast, ClipCaricaLancio);
	IniettaClipAzioneBeat(FName(TEXT("Hero.Branth")), Ram, ERTPresentationRole::Attack, ClipCaricaImpatto);

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	BeatMappa(World, 8);

	ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
	ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Caricatore || !Bersaglio) { return false; }
	const int32 IndiceRam = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
	if (!TestTrue(TEXT("⛔ premessa: Branth ha Ram"), IndiceRam != INDEX_NONE)) { return false; }
	Caricatore->PlannedDashAbility = IndiceRam;
	Caricatore->PlannedDashCell = Bersaglio->Cell;
	Caricatore->PlannedCell = Caricatore->Cell;

	TM->bRecordAttackBeatsForTest = true;
	TM->RefreshTeamKnowledgeNow();
	TM->LockInAndResolve();

	bool bImpattoConLaChiaveDelloScatto = false;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.ActionId == Ram
			&& Ev.SourceStableUnitId == Caricatore->StableUnitId && Ev.Phase == ERTMatchPhase::Blast)
		{
			bImpattoConLaChiaveDelloScatto = true;
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: l'impatto e' un Attack di Blast con la chiave dello scatto"),
			bImpattoConLaChiaveDelloScatto))
	{
		return false;
	}

	FinoAllaFineDelPlayback(TM);
	if (!TestTrue(TEXT("⛔ premessa: LaunchPlaybackAttack e' partito per l'impatto"),
			TM->AttackBeatTraceForTest().Contains(TEXT("L0"))))
	{
		return false;
	}
	TestEqual(TEXT("🔴 l'impatto della carica suona la clip Attack dello SCATTO"),
		Caricatore->LastResolvedClipPathForTest(ERTPresentationRole::Attack).ToString(), FString(ClipCaricaImpatto));
	TestEqual(TEXT("e il cast della carica, nel Dash, la clip Cast dello scatto"),
		Caricatore->LastResolvedClipPathForTest(ERTPresentationRole::Cast).ToString(), FString(ClipCaricaLancio));
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build.
Expected: **rosso di compilazione** su `LastResolvedClipPathForTest` (`error C2039`): il seam non esiste. È il rosso del passo.

- [ ] **Step 3: Il seam, le firme, la risoluzione**

`RTUnit.h:1336`:

```cpp
	void PlayPresentationRole(ERTPresentationRole Ruolo, FName ActionId = NAME_None, FName BaseActionId = NAME_None);
```

e sopra di essa, in coda al commento `:1329-1335`, prima di ` */`:

```cpp
	 * 🔑 **`ActionId`/`BaseActionId` (#3563)**: li passano solo i beat che conoscono l'azione — `Cast` da
	 * `ShowActivation`, `Attack` da `LaunchPlaybackAttack` (spec D3). Vuoti, la risoluzione e' quella di ruolo.
	 * ⛔ Non e' una `UFUNCTION`: il default e' lecito, e `Hit`/`Death` restano a un argomento.
```

`RTUnit.h:1346`:

```cpp
	TSoftObjectPtr<UAnimSequenceBase> ResolvedClipPathFor(ERTPresentationRole Ruolo,
		FName ActionId = NAME_None, FName BaseActionId = NAME_None) const;
```

`RTUnit.h:1364-1376`, il blocco del seam diventa (il commento di `CastCuesPlayedForTest` resta com'è, si aggiunge il gemello dopo):

```cpp
	/**
	 * Quante volte `PlayPresentationRole(Cast)` e' stata chiamata su questa unita' — seam di misura (#3549).
	 *
	 * ⚠️ **Il contatore cresce solo sotto `WITH_DEV_AUTOMATION_TESTS`** (in `RTUnit.cpp`): fuori dai build di
	 * test resta `0`. Il campo non e' una `UPROPERTY`, come gli accessori `*ForTest` di `ARTTurnManager`, e
	 * permette di asserire il MECCANISMO — la cue chiamata — senza un Blueprint.
	 */
	int32 CastCuesPlayedForTest() const { return CastCuesPlayed; }

	/**
	 * Il path che l'ULTIMA `PlayPresentationRole(Ruolo)` ha risolto su questa unita' — seam di misura (#3563).
	 *
	 * 🔑 **Per ruolo, non uno slot unico**: la stessa unita' suona `Cast` e poi `Attack` nello stesso turno, e uno
	 * slot unico mostrerebbe solo l'ultimo. Vuoto se quel ruolo non e' mai stato suonato, o se ha risolto nulla.
	 * ⚠️ Scritto solo sotto `WITH_DEV_AUTOMATION_TESTS`; non e' una `UPROPERTY` (`BlueprintSurfaceIsCensused`).
	 */
	FSoftObjectPath LastResolvedClipPathForTest(ERTPresentationRole Ruolo) const
	{
		const FSoftObjectPath* Path = LastResolvedClipPaths.Find(Ruolo);
		return Path ? *Path : FSoftObjectPath();
	}

private:
	int32 CastCuesPlayed = 0;
	TMap<ERTPresentationRole, FSoftObjectPath> LastResolvedClipPaths;

public:
```

`RTUnit.cpp:723`:

```cpp
TSoftObjectPtr<UAnimSequenceBase> ARTUnit::ResolvedClipPathFor(ERTPresentationRole Ruolo, FName ActionId,
	FName BaseActionId) const
```

e `:738-739`:

```cpp
	// 🔑 Risolve SENZA caricare: headless i pack non ci sono, e il PATH e' cio' che un test puo' asserire.
	// Con l'azione (#3563): profilo, poi generica, poi ruolo — l'ordine vive in `ActiveClipFor`, non qui.
	return Defaults->ActiveClipFor(HeroId, Ruolo, ActionId, BaseActionId);
```

`RTUnit.cpp:742-747`:

```cpp
void ARTUnit::PlayPresentationRole(ERTPresentationRole Ruolo, FName ActionId, FName BaseActionId)
{
	// ⛔ Ogni uscita anticipata di questa funzione e' un DEGRADO previsto, non un errore: l'unita' resta in
	// posa di riferimento e la partita si gioca uguale (invariante #1, come D-248 per la locomozione).
	const TSoftObjectPtr<UAnimSequenceBase> Path = ResolvedClipPathFor(Ruolo, ActionId, BaseActionId);
#if WITH_DEV_AUTOMATION_TESTS
	LastResolvedClipPaths.Add(Ruolo, Path.ToSoftObjectPath()); // seam: QUALE path, prima di caricare
#endif
	UAnimSequenceBase* const Sequenza = Path.IsNull() ? nullptr : Path.LoadSynchronous();
```

`RTTurnManager.cpp:8103`:

```cpp
		// L'azione arriva al beat (#3563, D3): la clip del profilo, poi della generica, poi del ruolo.
		Src->PlayPresentationRole(ERTPresentationRole::Cast, Ev.ActionId, Ev.BaseActionId);
```

`RTTurnManager.cpp:8289`:

```cpp
		// ⚠️ Per l'impatto di una carica `ActionId` e' lo SCATTO (`Impact.Def = Dash->Def`): e' voluto (#3563).
		AtkSrc->PlayPresentationRole(ERTPresentationRole::Attack, Atk.ActionId, Atk.BaseActionId);
```

`Hit` (`:8307`) e `Death` (`:8999`, `:9162`) **non** si toccano.

- [ ] **Step 4: Build e verde — e la prova che il beat `Attack` parte**

Run: build; test `RefactorTactics.Playback+RefactorTactics.Anim+RefactorTactics.Unit+RefactorTactics.Turn`.
Expected: verdi, `**** TEST COMPLETE`. In particolare **le due premesse `L0`** di `ActivationPlaysTheActionClip` e `ChargeImpactPlaysTheDashAttackClip` passano: sono la misura che `LaunchPlaybackAttack` (chiamato a `RTTurnManager.cpp:8256`, ramo `case ERTResolvedEventType::Attack` della sequenza del Blast) parte nella fixture. Annota nel commit che la premessa è passata **prima** della mutazione (6).

- [ ] **Step 5: Solo se una premessa `L0` cade — estendere la fixture finché il beat parte**

Il ramo che chiama `LaunchPlaybackAttack` (`:8256`) gira per ogni elemento `Attack` della sequenza del Blast, che `BuildBlastSequence` costruisce filtrando con `SourceVerdict.AllowsTeam(Viewer)`. Diagnosi in quest'ordine, una sola modifica per volta, rilanciando il test:

1. **L'`Attack` è nella timeline?** Aggiungi temporaneamente un asserto `ResolvedTimelineForTest()` contiene un `Attack` di Blast con `SourceStableUnitId == B.Tiratore->StableUnitId`. Se manca: il colpo è stato rifiutato dal resolver. `ImpactShot` (`PlannedAbilityIndex = 0`, `RTPlaybackActivationTests.cpp:100`) deve essere l'attacco base: verifica con `B.Tiratore->GetAbility(0)->Def.ActionId == Hero.Branth.ImpactShot`; se l'indice 0 non è `ImpactShot`, sostituisci `:100` con un ciclo come quello di `TideGuard` a `:92-98` (⚠️ non `BeatIndiceAbilita`: è dichiarata a `:192`, dopo `CostruisciBeat`). Distanza: Tiratore `(0,0)`, Bersaglio `(1,0)`, adiacenti su arena piatta — portata e linea di tiro sono soddisfatte; se il resolver lo rifiuta comunque, leggi il motivo nella voce di TurnLog del Tiratore (`GetTurnLog()`).
2. **È nella timeline ma `L0` manca?** Allora è filtrato dal viewer: la fixture di `CostruisciBeat` usa il viewer della squadra 0 con il mondo inizializzato (`:70`), e il Tiratore è della squadra 0 — una sorgente propria è sempre nota. Se il filtro la scarta, è un difetto del filtro, non della fixture: **fermati e riporta**.
3. Per la carica: se l'`Attack` con chiave `Ram` manca, ripristina le squadre di `Turn.ChargeActivatesInDashNotInBlast` (caricatore squadra 1) e crea il viewer della squadra 1 come fa `CostruisciBeat` (`InitializeActorsForPlay` + `RTWorldFixtures::MakePlayerOnTeam(World, 1)`).

Ogni estensione resta nel test e nel commit, con un commento che dice quale delle tre cause era.

- [ ] **Step 6: Mutazioni (3) e (6)**

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| (3) `RTTurnManager.cpp:8103` → `Src->PlayPresentationRole(ERTPresentationRole::Cast);` | `Playback.ActivationPlaysTheActionClip` | «🔴 il cast dello scudo suona la clip d'azione di TideGuard» |
| (6) `RTTurnManager.cpp:8289` → `AtkSrc->PlayPresentationRole(ERTPresentationRole::Attack);` | `Playback.ActivationPlaysTheActionClip` e `Playback.ChargeImpactPlaysTheDashAttackClip` | «🔴 l'attacco del tiratore suona la clip d'azione di ImpactShot» e «🔴 l'impatto della carica suona la clip Attack dello SCATTO» |

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Unit/RTUnit.h Source/RefactorTactics/Unit/RTUnit.cpp Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp
git commit -F - <<'EOF'
feat(3563): il beat conosce l'azione - Cast e Attack passano ActionId e BaseActionId, seam per ruolo

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 3: Il default C++ — la mappa abilità→clip della spec §2.6

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp:40-50` (commento-trappola), dopo `:63` (helper `MakeActionClips`), `:156-166` (costruttore)
- Modify: `Source/RefactorTactics/Tests/RTUnitTests.cpp:611-612` (commento)
- Test: `Source/RefactorTactics/Tests/RTAnimChannelTests.cpp` (include, test nuovo in coda, estensione di `Unit.CastRoleResolvesAClipForEveryHero` a `:313-331`)

**Interfaces:**
- Consumes: `FRTHeroPresentationClips::PerAction`, `ActiveClipFor` a quattro argomenti (Task 1); `URTHeroCatalogLibrary::GetHeroRoster()` (`Ability/RTHeroCatalogLibrary.h:161`), `URTHeroData::Actions` (`Ability/RTHeroData.h:164`).
- Produces: il CDO di `URTUnitAnimInstance` con `PerAction` popolata per le voci di §2.6 (una variante `AV_Roster` attiva per (abilità, beat)). Helper privato nel `.cpp`:
  ```cpp
  struct FRTVoceClipAzione { const TCHAR* ActionId; ERTPresentationRole Role; const TCHAR* Clip; };
  TMap<FName, FRTActionPresentationClips> MakeActionClips(const TCHAR* Pack, std::initializer_list<FRTVoceClipAzione> Voci);
  ```

- [ ] **Step 1: Misura i nomi sul disco PRIMA di scrivere la mappa**

I nomi non si deducono (`RTUnitAnimInstance.h:180-182`). Sul clone principale (la cartella è ignorata da git, `.gitignore:108`, e un worktree non la ha), il comando della spec, esteso a tutte le voci della mappa:

```powershell
$Root = "D:\Repositories\refactor-tactics-main\Content\FabAsset\Paragon"
$Attese = @{
  Gadget = 'LMB_Fire_A','LMB_Fire_B','LMB_Fire_C','Ability_Q_Target','Throw_Ready'
  Phase  = 'Primary_Attack_A_Medium','R_Ability_Intro','Ability_E','Ability_R_Alt'
  Riktor = 'PrimaryAttack_A_Slow','PrimaryAttack_B_Slow','Ability_Lockdown','Ability_Hook_Pull','Ability_Hook_Start','Ability_Hook_Cast','Ability_ShockingPunch'
  Wraith = 'Fire_A_Fast_V1','Ability_Q_Fire_Fwd','Ability_E_Targeting_Start','Ability_R_InMotion','Ability_E','Ability_RMB_Start'
}
foreach ($Pack in $Attese.Keys) {
  $Presenti = Get-ChildItem "$Root\Paragon$Pack\Characters\Heroes\$Pack\Animations" -Filter *.uasset | Select-Object -ExpandProperty BaseName
  foreach ($Clip in $Attese[$Pack]) { if ($Presenti -ccontains $Clip) { "OK    $Pack/$Clip" } else { "MANCA $Pack/$Clip" } }
}
```

Expected: nessuna riga `MANCA` (confronto **sensibile alle maiuscole**, `-ccontains`: il path è case-sensitive nel cook). Misurato in pianificazione sul clone principale il 2026-10-07: nessuna `MANCA`. ⛔ **Se una riga dice `MANCA`, il task si ferma qui** e lo riporta all'autore col nome: è un dato d'autore, non si inventa un sostituto. Salva l'output in `<scratchpad>/clip-misurate.txt` — **solo appoggio**: non sopravvive alla sessione. ⛔ L'evidenza sono le righe stesse: lo Step 7 incolla le righe `OK <Pack>/<Clip>` nel **corpo del commit**, e il Task 8 Step 2 le ripete nel **corpo della PR**.

⚠️ Il comando prova il **nome**, non la classe: un `.uasset` con quel nome potrebbe essere un montaggio o un additivo. I nomi scelti non portano i suffissi `_Montage`/`_Additive`/`_MSA` dei montaggi e degli additivi dei pack (misurato nell'elenco); la classe la conferma a schermo la seduta PIE (Task 7), e un path a classe sbagliata degrada come uno assente (spec §4, `LoadSynchronous` nullo).

- [ ] **Step 2: I test rossi**

`RTAnimChannelTests.cpp`, dopo `#include "Ability/RTHeroCatalogLibrary.h"` (`:12`):

```cpp
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
```

In coda al namespace anonimo (dopo `PathDi` del Task 1):

```cpp

	// --- La mappa di default (spec §2.6) — la SECONDA copia dichiarata ---------------------------------------
	//
	// ⚠️ Ogni ritocco a schermo tocca DUE righe: questa e quella di `MakeActionClips` (spec §6). E' il prezzo di un
	// test che non legge il default per confrontarlo con se stesso.

	/** Una riga di §2.6. `nullptr` = nessuna voce per quel beat: resta la clip di ruolo. */
	struct FRTClipAttesaDefault
	{
		const TCHAR* ActionId;
		const TCHAR* Pack;
		const TCHAR* Cast;
		const TCHAR* Attack;
	};

	const FRTClipAttesaDefault ClipAtteseDefault[] = {
		// Aevik (Gadget)
		{ TEXT("Hero.Aevik.ArcPulse"),          TEXT("Gadget"), nullptr,                          TEXT("LMB_Fire_A") },
		{ TEXT("Hero.Aevik.LinearDischarge"),   TEXT("Gadget"), TEXT("Ability_Q_Target"),         TEXT("LMB_Fire_B") },
		{ TEXT("Hero.Aevik.Overload"),          TEXT("Gadget"), TEXT("Throw_Ready"),              TEXT("LMB_Fire_C") },
		{ TEXT("Hero.Aevik.ConductiveNode"),    TEXT("Gadget"), nullptr,                          nullptr },  // Environment: nessun beat in v0.1
		{ TEXT("Hero.Aevik.ReactiveCapacitor"), TEXT("Gadget"), nullptr,                          nullptr },  // reazione: non si attiva
		// Muiren (Phase)
		{ TEXT("Hero.Muiren.PressureJet"),      TEXT("Phase"),  nullptr,                          TEXT("Primary_Attack_A_Medium") },
		{ TEXT("Hero.Muiren.CircularTide"),     TEXT("Phase"),  TEXT("R_Ability_Intro"),          nullptr },
		{ TEXT("Hero.Muiren.FluidTrail"),       TEXT("Phase"),  TEXT("Ability_E"),                nullptr },
		{ TEXT("Hero.Muiren.TideGuard"),        TEXT("Phase"),  TEXT("Ability_R_Alt"),            nullptr },
		{ TEXT("Hero.Muiren.FlowReaction"),     TEXT("Phase"),  nullptr,                          nullptr },  // inerte: resta il ruolo
		{ TEXT("Hero.Muiren.MistVeil"),         TEXT("Phase"),  nullptr,                          nullptr },  // Environment
		// Branth (Riktor)
		{ TEXT("Hero.Branth.ImpactShot"),       TEXT("Riktor"), nullptr,                          TEXT("PrimaryAttack_A_Slow") },
		{ TEXT("Hero.Branth.KineticPanel"),     TEXT("Riktor"), TEXT("Ability_Lockdown"),         nullptr },
		{ TEXT("Hero.Branth.Reconfigure"),      TEXT("Riktor"), TEXT("Ability_Hook_Pull"),        nullptr },
		{ TEXT("Hero.Branth.Ram"),              TEXT("Riktor"), TEXT("Ability_Hook_Start"),       TEXT("Ability_ShockingPunch") },
		{ TEXT("Hero.Branth.MortarShot"),       TEXT("Riktor"), TEXT("Ability_Hook_Cast"),        TEXT("PrimaryAttack_B_Slow") },
		{ TEXT("Hero.Branth.Interposition"),    TEXT("Riktor"), nullptr,                          nullptr },  // reazione
		// Ivrin (Wraith)
		{ TEXT("Hero.Ivrin.PulseShot"),         TEXT("Wraith"), nullptr,                          TEXT("Fire_A_Fast_V1") },
		{ TEXT("Hero.Ivrin.InterceptShot"),     TEXT("Wraith"), TEXT("Ability_E_Targeting_Start"), nullptr }, // nessun evento Attack: RTTurnManager.cpp:7133-7136
		{ TEXT("Hero.Ivrin.PassingBlade"),      TEXT("Wraith"), TEXT("Ability_R_InMotion"),       TEXT("Ability_Q_Fire_Fwd") },
		{ TEXT("Hero.Ivrin.Feint"),             TEXT("Wraith"), TEXT("Ability_E"),                nullptr },
		{ TEXT("Hero.Ivrin.PhaseGuard"),        TEXT("Wraith"), TEXT("Ability_RMB_Start"),        nullptr },
		{ TEXT("Hero.Ivrin.Deflection"),        TEXT("Wraith"), nullptr,                          nullptr },  // reazione
	};

	/** Un beat di una riga: la voce c'e', e' attiva, punta al path atteso e NON e' quello di ruolo — o non c'e'. */
	void ControllaBeatDiDefault(FAutomationTestBase& Test, const URTUnitAnimInstance& Cdo,
		const FRTHeroPresentationClips& Voce, const FName& HeroId, const FName& ActionId,
		ERTPresentationRole Ruolo, const TCHAR* Pack, const TCHAR* ClipAttesa)
	{
		const FString Chi = FString::Printf(TEXT("%s / %s"), *ActionId.ToString(), *UEnum::GetValueAsString(Ruolo));
		const FRTActionPresentationClips* Azione = Voce.PerAction.Find(ActionId);
		const FRTAnimRoleClips* Pool = Azione ? Azione->PerRole.Find(Ruolo) : nullptr;
		if (ClipAttesa == nullptr)
		{
			Test.TestNull(*FString::Printf(TEXT("%s: nessuna voce di default, resta la clip di ruolo"), *Chi),
				(const void*)Pool);
			return;
		}
		const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: la voce di default c'e' ed e' attiva"), *Chi), (const void*)Attiva))
		{
			return;
		}
		const FString Atteso = FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/%s.%s"), Pack, Pack, ClipAttesa, ClipAttesa);
		Test.TestEqual(*FString::Printf(TEXT("%s: il path e' quello della mappa"), *Chi),
			Attiva->Clip.ToSoftObjectPath().ToString(), Atteso);
		Test.TestNotEqual(*FString::Printf(TEXT("%s: ed e' DIVERSO dalla clip di ruolo"), *Chi),
			Attiva->Clip.ToSoftObjectPath().ToString(), Cdo.ActiveClipFor(HeroId, Ruolo).ToSoftObjectPath().ToString());
	}
```

In coda al file, prima di `#endif`:

```cpp

/**
 * Il default C++ porta la mappa abilita'→clip della spec §2.6 per OGNI abilita' dei kit — e nient'altro.
 *
 * 🔑 **La lista attesa e' una FUNZIONE del catalogo eroi**, non un elenco letterale: si parte da
 * `GetHeroRoster()` e ogni abilita' d'eroe (profilo `Hero.<Eroe>.*`, D-033) deve avere una riga nella tabella
 * qui sopra — con una clip o dichiarata senza voce. Un'abilita' nuova senza riga e' ROSSO: la sua clip e' una
 * decisione dell'autore, non un silenzio.
 * 🔴 **E nel verso opposto** (Review Focus (d)): ogni chiave di `PerAction` nel CDO deve essere un'abilita' di
 * QUELL'eroe nel catalogo, e ogni riga della tabella deve essere stata visitata. Un'abilita' tolta dal catalogo
 * mentre resta nella mappa di `MakeActionClips` cade qui, col suo nome.
 * ✅ Validato per mutazione (2) — una riga tolta da `MakeActionClips` — e (P3) — una riga per un'abilita'
 * inesistente aggiunta a `MakeActionClips`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDefaultActionClipsResolveForEveryKitAbilityTest,
	"RefactorTactics.Unit.DefaultActionClipsResolveForEveryKitAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDefaultActionClipsResolveForEveryKitAbilityTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO di URTUnitAnimInstance"), Cdo)) { return false; }

	TSet<FString> RigheVisitate;   // solo Contains/Add: nessun asserto dipende dall'ordine
	int32 AbilitaConfrontate = 0;
	for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!TestNotNull(TEXT("eroe del roster"), Eroe)) { continue; }
		const FString NomeEroe = Eroe->HeroId.ToString();
		const FString Prefisso = NomeEroe + TEXT(".");
		const FRTHeroPresentationClips* Voce = Cdo->FindClipsFor(Eroe->HeroId);
		if (!TestNotNull(*FString::Printf(TEXT("%s ha una voce nel CDO"), *NomeEroe), (const void*)Voce)) { continue; }

		TSet<FName> AzioniDellEroe;
		for (const URTActionData* Azione : Eroe->Actions)
		{
			if (Azione == nullptr) { continue; }
			const FName Id = Azione->Def.ActionId;
			AzioniDellEroe.Add(Id);
			if (!Id.ToString().StartsWith(Prefisso)) { continue; }   // solo i profili d'eroe
			++AbilitaConfrontate;

			const FRTClipAttesaDefault* Riga = nullptr;
			for (const FRTClipAttesaDefault& R : ClipAtteseDefault)
			{
				if (Id == FName(R.ActionId)) { Riga = &R; break; }
			}
			if (!TestNotNull(*FString::Printf(
					TEXT("%s: l'abilita' del catalogo ha una riga nella mappa di §2.6 (decidi la clip, o dichiarala senza voce)"),
					*Id.ToString()), (const void*)Riga))
			{
				continue;
			}
			RigheVisitate.Add(Riga->ActionId);
			ControllaBeatDiDefault(*this, *Cdo, *Voce, Eroe->HeroId, Id, ERTPresentationRole::Cast, Riga->Pack, Riga->Cast);
			ControllaBeatDiDefault(*this, *Cdo, *Voce, Eroe->HeroId, Id, ERTPresentationRole::Attack, Riga->Pack, Riga->Attack);
		}

		// 🔴 Il verso opposto: il default non nomina abilita' che il catalogo non ha, e solo i ruoli di D3.
		for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce->PerAction)
		{
			TestTrue(*FString::Printf(TEXT("%s: la voce di default '%s' e' un'abilita' che il catalogo eroi ha"),
				*NomeEroe, *Azione.Key.ToString()), AzioniDellEroe.Contains(Azione.Key));
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Azione.Value.PerRole)
			{
				TestTrue(*FString::Printf(TEXT("%s / %s: solo Cast e Attack conoscono l'azione (D3)"),
					*Azione.Key.ToString(), *UEnum::GetValueAsString(Ruolo.Key)),
					Ruolo.Key == ERTPresentationRole::Cast || Ruolo.Key == ERTPresentationRole::Attack);
			}
		}
	}

	TestTrue(TEXT("⛔ premessa: il catalogo eroi ha abilita' da confrontare"), AbilitaConfrontate > 0);
	for (const FRTClipAttesaDefault& R : ClipAtteseDefault)
	{
		TestTrue(*FString::Printf(TEXT("%s: la riga della mappa nomina un'abilita' che il catalogo eroi ha ancora"),
			R.ActionId), RigheVisitate.Contains(FString(R.ActionId)));
	}
	return true;
}
```

Estensione di `Unit.CastRoleResolvesAClipForEveryHero`: nel ciclo (`:322-329`), dopo il secondo `TestEqual`:

```cpp
		// ➕ #3563: un'azione senza voce — ne' profilo ne' generica — ripiega sul ruolo, non su nulla.
		TestEqual(*FString::Printf(TEXT("%s: un'azione senza voce ripiega sul ruolo Cast"), Eroe),
			Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Cast, FName(TEXT("Hero.SenzaVoce.Prova")),
				FName(TEXT("Action.BasicAttack"))).ToSoftObjectPath().ToString(),
			Cast.ToSoftObjectPath().ToString());
```

e nel commento (`:303-312`), dopo la riga ⚠️, prima di ` */`:

```cpp
 *
 * ➕ #3563: le clip per AZIONE stanno sopra il ruolo (`Unit.DefaultActionClipsResolveForEveryKitAbility`); qui si
 * pinna che il ruolo resti il ripiego quando l'azione non ha voce — anche con una generica dichiarata.
```

- [ ] **Step 3: Verifica che fallisca**

Run: build; test `RefactorTactics.Unit.DefaultActionClipsResolveForEveryKitAbility+RefactorTactics.Unit.CastRoleResolvesAClipForEveryHero`.
Expected: `DefaultActionClipsResolveForEveryKitAbility` `Result={Fail}` su «… la voce di default c'e' ed e' attiva» per ogni beat con una clip (il CDO non ha ancora `PerAction`); le righe «nessuna voce di default» e il verso opposto passano. `CastRoleResolvesAClipForEveryHero` verde (il ripiego esiste dal Task 1).

- [ ] **Step 4: L'helper e la mappa**

`RTUnitAnimInstance.cpp`, dopo `#include "Unit/RTUnit.h"` (`:5`):

```cpp

#include <initializer_list>
```

Dopo la `}` di `MakeClips` (`:63`), dentro il namespace (prima della `}` di `:64`):

```cpp

	/** Una voce della mappa abilita'→clip: quale azione, su quale beat, con quale clip del pack. */
	struct FRTVoceClipAzione
	{
		const TCHAR* ActionId;
		ERTPresentationRole Role;
		const TCHAR* Clip;
	};

	/**
	 * Le clip per AZIONE di un eroe del roster (#3563, spec «la clip per abilita'» §2.4, D4).
	 *
	 * 🔑 **Una riga per (abilita', beat)**, ciascuna una variante `AV_Roster` gia' attiva — la stessa forma di
	 * `MakeRuolo`. Solo `Cast` e `Attack` (D3): gli altri ruoli non conoscono l'azione, e una voce li' non suonerebbe.
	 *
	 * ⚠️ **La mappa e' un giudizio dell'autore, scelto dai NOMI** (spec §2.6, approvata il 2026-10-07): la seduta
	 * `PIE-CLIP-ABILITA` puo' cambiarne ogni riga. Chi la cambia cambia anche la seconda copia dichiarata,
	 * `ClipAtteseDefault` in `Tests/RTAnimChannelTests.cpp`. Ogni nome e' stato MISURATO sul disco prima di
	 * entrare qui (piano, Task 3 Step 1): i nomi non si deducono.
	 */
	TMap<FName, FRTActionPresentationClips> MakeActionClips(const TCHAR* Pack, std::initializer_list<FRTVoceClipAzione> Voci)
	{
		TMap<FName, FRTActionPresentationClips> PerAzione;
		for (const FRTVoceClipAzione& Voce : Voci)
		{
			PerAzione.FindOrAdd(FName(Voce.ActionId)).PerRole.Add(Voce.Role, MakeRuolo(Pack, Voce.Clip));
		}
		return PerAzione;
	}
```

Il costruttore (`:156-166`) diventa:

```cpp
URTUnitAnimInstance::URTUnitAnimInstance()
{
	// ⚠️ Il riferimento restituito da `Add` si usa SUBITO: l'`Add` dell'eroe successivo puo' riallocare la mappa.
	ClipsPerHero.Add(FName(TEXT("Hero.Aevik")), MakeClips(TEXT("Gadget"), TEXT("Idle"), TEXT("Run_Fwd"),
		TEXT("Cast"), TEXT("Hitreact_Fwd"), TEXT("Death_Fwd"))).PerAction = MakeActionClips(TEXT("Gadget"), {
		{ TEXT("Hero.Aevik.ArcPulse"),        ERTPresentationRole::Attack, TEXT("LMB_Fire_A") },
		{ TEXT("Hero.Aevik.LinearDischarge"), ERTPresentationRole::Cast,   TEXT("Ability_Q_Target") },
		{ TEXT("Hero.Aevik.LinearDischarge"), ERTPresentationRole::Attack, TEXT("LMB_Fire_B") },
		{ TEXT("Hero.Aevik.Overload"),        ERTPresentationRole::Cast,   TEXT("Throw_Ready") },
		{ TEXT("Hero.Aevik.Overload"),        ERTPresentationRole::Attack, TEXT("LMB_Fire_C") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Muiren")), MakeClips(TEXT("Phase"), TEXT("Idle"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Fwd"), TEXT("Death"))).PerAction = MakeActionClips(TEXT("Phase"), {
		{ TEXT("Hero.Muiren.PressureJet"),  ERTPresentationRole::Attack, TEXT("Primary_Attack_A_Medium") },
		{ TEXT("Hero.Muiren.CircularTide"), ERTPresentationRole::Cast,   TEXT("R_Ability_Intro") },
		{ TEXT("Hero.Muiren.FluidTrail"),   ERTPresentationRole::Cast,   TEXT("Ability_E") },
		{ TEXT("Hero.Muiren.TideGuard"),    ERTPresentationRole::Cast,   TEXT("Ability_R_Alt") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Branth")), MakeClips(TEXT("Riktor"), TEXT("Idle"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Front"), TEXT("Death_Fwd"))).PerAction = MakeActionClips(TEXT("Riktor"), {
		{ TEXT("Hero.Branth.ImpactShot"),   ERTPresentationRole::Attack, TEXT("PrimaryAttack_A_Slow") },
		{ TEXT("Hero.Branth.KineticPanel"), ERTPresentationRole::Cast,   TEXT("Ability_Lockdown") },
		{ TEXT("Hero.Branth.Reconfigure"),  ERTPresentationRole::Cast,   TEXT("Ability_Hook_Pull") },
		// L'impatto di una carica porta l'ActionId dello SCATTO (`Impact.Def = Dash->Def`): Ram ha un beat Attack.
		{ TEXT("Hero.Branth.Ram"),          ERTPresentationRole::Cast,   TEXT("Ability_Hook_Start") },
		{ TEXT("Hero.Branth.Ram"),          ERTPresentationRole::Attack, TEXT("Ability_ShockingPunch") },
		{ TEXT("Hero.Branth.MortarShot"),   ERTPresentationRole::Cast,   TEXT("Ability_Hook_Cast") },
		{ TEXT("Hero.Branth.MortarShot"),   ERTPresentationRole::Attack, TEXT("PrimaryAttack_B_Slow") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Ivrin")), MakeClips(TEXT("Wraith"), TEXT("Idle_NonCombat"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Front"), TEXT("Death_Forward"))).PerAction = MakeActionClips(TEXT("Wraith"), {
		{ TEXT("Hero.Ivrin.PulseShot"),     ERTPresentationRole::Attack, TEXT("Fire_A_Fast_V1") },
		// Il colpo predittivo non emette un `Attack` (`RTTurnManager.cpp:7133-7136`): solo il beat Cast.
		{ TEXT("Hero.Ivrin.InterceptShot"), ERTPresentationRole::Cast,   TEXT("Ability_E_Targeting_Start") },
		{ TEXT("Hero.Ivrin.PassingBlade"),  ERTPresentationRole::Cast,   TEXT("Ability_R_InMotion") },
		{ TEXT("Hero.Ivrin.PassingBlade"),  ERTPresentationRole::Attack, TEXT("Ability_Q_Fire_Fwd") },
		{ TEXT("Hero.Ivrin.Feint"),         ERTPresentationRole::Cast,   TEXT("Ability_E") },
		{ TEXT("Hero.Ivrin.PhaseGuard"),    ERTPresentationRole::Cast,   TEXT("Ability_RMB_Start") },
	});
}
```

Il commento-trappola di `:40-50` diventa:

```cpp
	/**
	 * I ruoli di un eroe del roster, ciascuno con la sua clip attiva.
	 *
	 * 🔴 **La clip dei pack che si CHIAMA `Cast` riempie DUE ruoli, e non e' un errore** (#2450, #3549). Sul
	 * ruolo `Attack` e' il colpo; sul ruolo `Cast` e' il gesto di attivazione di `AbilityActivated`. Sul RUOLO
	 * cast e colpo suonano la stessa sequenza in due momenti diversi (spec «il momento» D2).
	 *
	 * 🔑 **Da #3563 sopra il ruolo ci sono le clip per ABILITA'** (`MakeActionClips`, qui sotto): il ruolo resta
	 * il RIPIEGO, e una clip diversa per un'abilita' si scrive li', non cambiando questo ruolo.
	 *
	 * ⚠️ I nomi si MISURANO: §AS.3b li ha letti sul disco, e **quattro caselle su dodici** fra i tre
	 * ruoli discreti non si chiamano come ci si aspetta.
	 */
```

(La frase «quattro caselle su dodici» è una misura storica già nel file e citata come tale: resta.)

- [ ] **Step 5: Il commento di `RTUnitTests.cpp:611-612`**

Le due righe diventano:

```cpp
		// 🔑 ∴ l'invariante non e' piu' «`Cast` vuoto» ma «`Cast` uguale ad `Attack`» SUL RUOLO. Una clip diversa
		// per un'ABILITA' vive in `PerAction` (#3563, spec «la clip per abilita'» §2.6) e non tocca questo asserto;
		// una clip diversa sul RUOLO `Cast` resta un giudizio umano del catalogo ANIM CORE, e chi la fa cambia
		// anche questa riga.
```

L'asserto (`:613-617`) **non** cambia.

- [ ] **Step 6: Build, verde, mutazioni (2) e (P3)**

Run: build; test `RefactorTactics.Unit+RefactorTactics.Anim+RefactorTactics.Playback`.
Expected: verdi, `**** TEST COMPLETE`. `Unit.DiscreteRoleClipsMatchThePacks` verde e invariato (pinna `Cast == Attack` sul **ruolo**). I test di playback del Task 2 restano verdi: iniettano sopra il default e ripristinano.

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| (2) Togli la riga `{ TEXT("Hero.Branth.MortarShot"), ERTPresentationRole::Attack, TEXT("PrimaryAttack_B_Slow") },` | `Unit.DefaultActionClipsResolveForEveryKitAbility` | «Hero.Branth.MortarShot / ERTPresentationRole::Attack: la voce di default c'e' ed e' attiva» |
| (P3) Aggiungi a Branth `{ TEXT("Hero.Branth.Ritirata"), ERTPresentationRole::Cast, TEXT("Ability_Hook_Pull") },` (un'abilità che il catalogo non ha: la forma di un'abilità tolta dal catalogo e rimasta nella mappa) | `Unit.DefaultActionClipsResolveForEveryKitAbility` | «Hero.Branth: la voce di default 'Hero.Branth.Ritirata' e' un'abilita' che il catalogo eroi ha» |

- [ ] **Step 7: Commit**

Prima di eseguire il comando, sostituisci il segnaposto `<righe OK …>` con le righe `OK <Pack>/<Clip>` stampate dallo Step 1, **alla lettera** e una per riga: un messaggio che rimanda a un file dello scratchpad cita un'evidenza che nessuno potrà rileggere.

```bash
git add Source/RefactorTactics/Unit/RTUnitAnimInstance.cpp Source/RefactorTactics/Tests/RTAnimChannelTests.cpp Source/RefactorTactics/Tests/RTUnitTests.cpp
git commit -F - <<'EOF'
feat(3563): la mappa abilita'-clip di default dai pack, nomi misurati sul disco

Nomi misurati sul clone principale prima di entrare in MakeActionClips, col comando del piano
(Task 3 Step 1: Get-ChildItem <pack>/Animations -Filter *.uasset, confronto -ccontains). Nessuna MANCA:

<righe OK …: le righe OK <Pack>/<Clip> dello Step 1, incollate alla lettera>

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 4: Il catalogo JSON — `actionId`, `formatVersion` 2, `ValidateCatalog` sulla terna

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTAnimCatalogTypes.h:96-99` (commento), `:110-114` (campo nuovo fra `Role` e `bActive`), `:210-218` (versione)
- Modify: `Source/RefactorTactics/Unit/RTAnimCatalogLibrary.cpp:1-8` (include), `:37-40` (chiave), `:394-427` (parse), `:462-463` e `:492-499` (scrittura), prima di `:511` (helper), `:600-630` (unicità e azioni)
- Modify: `Data/Anim/AnimCatalog.json:2` (`"formatVersion": 2`, file LF)
- Test: `Source/RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp` (helper in coda al namespace `:9-57`, test dopo `Anim.Catalog.RejectsTwoActivePerRole` `:227-269`)

**Interfaces:**
- Produces:
  ```cpp
  // FRTAnimBinding
  FName ActionId;   // NAME_None = binding di ruolo
  // FRTAnimCatalog
  static constexpr int32 CurrentFormatVersion = 2;
  static constexpr int32 FirstFormatVersionWithActionId = 2;
  ```
  JSON: chiave `actionId` opzionale in ogni binding, scritta solo quando non vuota; `formatVersion` scritto sempre come `CurrentFormatVersion`.
  `URTAnimCatalogLibrary::ValidateCatalog` (firma invariata, `RTAnimCatalogLibrary.h:140`): unicità su `(HeroId, Role, ActionId)`; errore per azione ignota e per `actionId` su un ruolo diverso da `Cast`/`Attack`.
- Consumes: `URTCatalogLibrary::GetCoreActionCatalog()` (`Ability/RTCatalogLibrary.h:504`), `GetGenericActionIds()` (`:602`), `MakeWeaponVariants()` (`:267`, impl. `.cpp:371`), `MakeGadgets()` (`:308`, impl. `.cpp:446`, comprende `MakePortableCoverGadget` a `.cpp:503`), `MakeReactionModules()` (`:293`, impl. `.cpp:526`), `MakeEquipmentAction(Item, Outer)` (`:441`, impl. `.cpp:918-940`: `nullptr` se il pezzo non concede un'azione, altrimenti `Def.ActionId = Item->EquipmentId` a `.cpp:933`); `URTHeroCatalogLibrary::GetHeroRoster()` (`Ability/RTHeroCatalogLibrary.h:161`).

**L'insieme delle azioni valide, misurato:** `grep -rn "static TArray<FRTActionDef>\|static TArray<URTActionData\*>\|static TArray<URTEquipmentData\*>" Source/RefactorTactics/Ability/` → `GetCoreActionCatalog`, `MakeGenericActions`, `MakeWeaponVariants`, `MakeReactionModules`, `MakeGadgets`. Le «ambientali» della spec §2.3 non hanno un catalogo proprio: le abilità di fase Environment degli eroi (`ConductiveNode`, `MistVeil`) stanno in `URTHeroData::Actions` (`Ability/RTHeroData.h:164`), e `grep -rhoE 'TEXT\("(Env|Environment|Hazard)\.[A-Za-z.]+"\)' Source/RefactorTactics/{Ability,Turn,Environment}` dà **0** righe. Se nel frattempo è nato un catalogo ambientale, si aggiunge a `RTAzioniConosciute` con la sua riga e lo si dichiara nel commit.

- [ ] **Step 1: I test rossi**

`RTAnimBrowserModelTests.cpp`, dopo `#include "Unit/RTAnimCatalogLibrary.h"` (`:5`):

```cpp
#include "Unit/RTUnitAnimInstance.h"
#include "Misc/ScopeExit.h"
```

In coda al namespace anonimo, prima della `}` di `:57`:

```cpp

	/**
	 * Un catalogo VALIDO con una voce `Promoted` e un binding ATTIVO per Aevik (#3563). `ActionId` nullo = binding
	 * di ruolo. E' la base dei test sulla terna: ogni rosso sotto parte da un verde misurato.
	 */
	FRTAnimCatalog CatalogoConLegame(ERTPresentationRole Role, const TCHAR* ActionId)
	{
		FRTAnimCatalog Catalog;
		Catalog.NextId = 2;
		FRTAnimCatalogEntry E;
		E.Id = FName(TEXT("AV_0001"));
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), TEXT("Throw_Ready"));
		E.Derived.AssetName = TEXT("Throw_Ready");
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Aevik"));
		B.Role = Role;
		B.ActionId = ActionId ? FName(ActionId) : NAME_None;
		B.bActive = true;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
		return Catalog;
	}

	/**
	 * Aggiunge al catalogo una voce `Promoted` con UN binding per Aevik, e tiene `NextId` dominante.
	 * ⚠️ `Clip` distinta per voce: due voci sullo stesso path sono gia' un errore di `ValidateCatalog` (`:576-582`).
	 */
	void AggiungiLegame(FRTAnimCatalog& Catalog, const TCHAR* Id, const TCHAR* Clip, ERTPresentationRole Role,
		const TCHAR* ActionId, bool bActive)
	{
		FRTAnimCatalogEntry E;
		E.Id = FName(Id);
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), Clip);
		E.Derived.AssetName = Clip;
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Aevik"));
		B.Role = Role;
		B.ActionId = ActionId ? FName(ActionId) : NAME_None;
		B.bActive = bActive;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
		Catalog.NextId = Catalog.Entries.Num() + 1;
	}

	bool QualcheRigaContiene(const TArray<FString>& Righe, const TCHAR* Frammento)
	{
		return Righe.ContainsByPredicate([Frammento](const FString& R) { return R.Contains(Frammento); });
	}

	/** Un catalogo JSON minimo con UN binding Cast di Aevik; `ActionIdJson` e' il frammento della chiave, o vuoto. */
	FString JsonConUnLegame(int32 Versione, const TCHAR* ActionIdJson)
	{
		return FString::Printf(TEXT(R"({ "formatVersion": %d, "nextId": 2, "entries": [ { "id": "AV_0001", )")
			TEXT(R"("derived": { "assetPath": "/Game/A.A" }, "authored": { "status": "Promoted", "bindings": [ )")
			TEXT(R"({ "hero": "Hero.Aevik", "role": "Cast", %s"active": true } ] } } ] })"), Versione, ActionIdJson);
	}
```

Dopo la `}` di `Anim.Catalog.RejectsTwoActivePerRole` (`:269`):

```cpp

// ─── La chiave `actionId` (#3563, spec «la clip per abilita'» §2.3) ──────────────────────────────────────────

/**
 * Il JSON con `actionId` fa round-trip, e un catalogo v1 che guadagna un `actionId` si RISALVA come v2.
 * ✅ Validato per mutazione (P5): il writer che scrive `Catalog.FormatVersion` invece di `CurrentFormatVersion`
 * fa rifiutare la rilettura («un actionId esiste solo da v2») → cade «la rilettura riesce».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogActionIdRoundTripsTest,
	"RefactorTactics.Anim.Catalog.ActionIdRoundTrips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogActionIdRoundTripsTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	Catalog.FormatVersion = 1;   // 🔑 un catalogo letto da un file v1, a cui l'autore aggiunge un binding d'azione
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);

	FString Json;
	TestTrue(TEXT("scrittura riuscita"), URTAnimCatalogLibrary::SaveToString(Catalog, Json));
	TestTrue(TEXT("il file scritto dichiara formatVersion 2"), Json.Contains(TEXT("\"formatVersion\": 2")));
	TestTrue(TEXT("il binding d'azione porta actionId"), Json.Contains(TEXT("\"actionId\": \"Hero.Aevik.Overload\"")));
	int32 Occorrenze = 0;
	for (int32 Da = Json.Find(TEXT("\"actionId\"")); Da != INDEX_NONE;
		Da = Json.Find(TEXT("\"actionId\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Da + 1))
	{
		++Occorrenze;
	}
	TestEqual(TEXT("il binding di ruolo NON porta la chiave"), Occorrenze, 1);

	FRTAnimCatalog Riletto;
	FString Errore;
	if (!TestTrue(TEXT("🔴 la rilettura riesce"), URTAnimCatalogLibrary::LoadFromString(Json, Riletto, Errore)))
	{
		AddInfo(Errore);
		return false;
	}
	TestEqual(TEXT("round-trip: l'azione del primo binding"),
		Riletto.Entries[0].Authored.Bindings[0].ActionId, FName(TEXT("Hero.Aevik.Overload")));
	TestEqual(TEXT("round-trip: il secondo resta di ruolo"),
		Riletto.Entries[1].Authored.Bindings[0].ActionId, FName(NAME_None));
	return true;
}

/**
 * Un `actionId` che non e' un'azione conosciuta e' un ERRORE, col suo nome — `Ruling` di §2.3.
 * ✅ Validato per mutazione (5): il controllo dell'azione ignota tolto → cade «un refuso e' un errore».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsUnknownActionIdTest,
	"RefactorTactics.Anim.Catalog.RejectsUnknownActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsUnknownActionIdTest::RunTest(const FString&)
{
	const FRTAnimCatalog Buono = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"));
	TestEqual(TEXT("controllo positivo: un'abilita' del kit e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&Buono).Num(), 0);
	const FRTAnimCatalog Generica = CatalogoConLegame(ERTPresentationRole::Attack, TEXT("Action.BasicAttack"));
	TestEqual(TEXT("controllo positivo: una generica core e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&Generica).Num(), 0);

	const FRTAnimCatalog Refuso = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overlaod"));
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Refuso);
	TestTrue(TEXT("🔴 un refuso e' un errore"), Errori.Num() > 0);
	TestTrue(TEXT("e la riga nomina l'azione"), QualcheRigaContiene(Errori, TEXT("Hero.Aevik.Overlaod")));
	return true;
}

/**
 * Al piu' una attiva per POOL: due sulla stessa terna sono errore; una di ruolo e una d'azione sullo stesso
 * `(eroe, ruolo)` sono due pool, e convivono (Review Focus (b), meta' «entrambi validi»).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsTwoActivePerActionRoleTest,
	"RefactorTactics.Anim.Catalog.RejectsTwoActivePerActionRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsTwoActivePerActionRoleTest::RunTest(const FString&)
{
	FRTAnimCatalog Pool;
	AggiungiLegame(Pool, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Pool, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	TestEqual(TEXT("🔑 una attiva di ruolo e una d'azione sullo stesso (eroe, ruolo): valido"),
		URTAnimCatalogLibrary::ValidateCatalog(&Pool).Num(), 0);

	AggiungiLegame(Pool, TEXT("AV_0003"), TEXT("Ability_Q_Target"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Pool);
	TestTrue(TEXT("🔴 due attive sulla stessa terna sono un errore"), Errori.Num() > 0);
	bool bNominaEntrambe = false;
	for (const FString& E : Errori)
	{
		if (E.Contains(TEXT("AV_0002")) && E.Contains(TEXT("AV_0003")) && E.Contains(TEXT("Hero.Aevik.Overload")))
		{
			bNominaEntrambe = true;
		}
	}
	TestTrue(TEXT("la riga nomina le due clip e l'azione"), bNominaEntrambe);
	return true;
}

/**
 * `formatVersion: 1` con un `actionId` e' rifiutato: una build vecchia lo leggerebbe come binding di RUOLO, cioe'
 * una clip sbagliata e attiva (spec §2.3, §4). Lo stesso testo dichiarato v2 si legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogFormatVersion2IsRequiredTest,
	"RefactorTactics.Anim.Catalog.FormatVersion2IsRequired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogFormatVersion2IsRequiredTest::RunTest(const FString&)
{
	FRTAnimCatalog Letto;
	FString Errore;
	TestFalse(TEXT("🔴 v1 con actionId: rifiutato"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(1, TEXT(R"("actionId": "Hero.Aevik.Overload", )")), Letto, Errore));
	TestTrue(TEXT("e il messaggio nomina actionId"), Errore.Contains(TEXT("actionId")));

	FRTAnimCatalog LettoV2;
	FString ErroreV2;
	TestTrue(TEXT("controllo positivo: lo stesso testo v2 si legge"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, TEXT(R"("actionId": "Hero.Aevik.Overload", )")), LettoV2, ErroreV2));
	return true;
}

/** Review Focus (c): un catalogo v2 di soli binding di RUOLO si legge, ed e' valido. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogFormatVersion2WithoutActionIdLoadsTest,
	"RefactorTactics.Anim.Catalog.FormatVersion2WithoutActionIdLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogFormatVersion2WithoutActionIdLoadsTest::RunTest(const FString&)
{
	FRTAnimCatalog Letto;
	FString Errore;
	if (!TestTrue(TEXT("🔴 v2 senza actionId: si legge"),
			URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, TEXT("")), Letto, Errore)))
	{
		AddInfo(Errore);
		return false;
	}
	TestEqual(TEXT("il binding e' di ruolo"), Letto.Entries[0].Authored.Bindings[0].ActionId, FName(NAME_None));
	TestEqual(TEXT("ed e' valido"), URTAnimCatalogLibrary::ValidateCatalog(&Letto).Num(), 0);
	FRTAnimCatalog LettoV1;
	TestTrue(TEXT("e un v1 senza actionId si legge ancora"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(1, TEXT("")), LettoV1, Errore));
	return true;
}

/**
 * Un `actionId` VALIDO su un ruolo che non propaga l'azione (`Move`) e' un errore: non suonerebbe mai (D3).
 * ✅ Validato per mutazione (9): il controllo del ruolo tolto → cade il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsActionIdOnNonPropagatingRoleTest,
	"RefactorTactics.Anim.Catalog.RejectsActionIdOnNonPropagatingRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsActionIdOnNonPropagatingRoleTest::RunTest(const FString&)
{
	const FRTAnimCatalog SuMove = CatalogoConLegame(ERTPresentationRole::Move, TEXT("Hero.Aevik.Overload"));
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&SuMove);
	TestTrue(TEXT("🔴 actionId valido su Move: errore"), Errori.Num() > 0);
	TestTrue(TEXT("e la riga nomina il ruolo"), QualcheRigaContiene(Errori, TEXT("Move")));

	const FRTAnimCatalog SuCast = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"));
	TestEqual(TEXT("controllo positivo: la stessa azione su Cast e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&SuCast).Num(), 0);
	return true;
}

/**
 * `Ruling` R10: un'azione concessa dall'equipaggiamento porta l'id del PEZZO (`MakeEquipmentAction`,
 * `RTCatalogLibrary.cpp:933`) e si attiva come le altre: `Gadget.Sprinkler` su `Cast` e' accettato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogAcceptsEquipmentActionIdTest,
	"RefactorTactics.Anim.Catalog.AcceptsEquipmentActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogAcceptsEquipmentActionIdTest::RunTest(const FString&)
{
	const FRTAnimCatalog Gadget = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Gadget.Sprinkler"));
	TestEqual(TEXT("🔴 l'azione di un gadget e' un'azione conosciuta"),
		URTAnimCatalogLibrary::ValidateCatalog(&Gadget).Num(), 0);
	// Il controllo positivo del rifiuto: senza, «accettato» non distinguerebbe «conosciuta» da «nessun controllo».
	const FRTAnimCatalog Refuso = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Gadget.Sprinklr"));
	TestTrue(TEXT("e un pezzo inesistente resta un errore"), URTAnimCatalogLibrary::ValidateCatalog(&Refuso).Num() > 0);
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build.
Expected: **rosso di compilazione** su `FRTAnimBinding::ActionId` (`error C2039`). È il rosso del passo.

- [ ] **Step 3: Il tipo**

`RTAnimCatalogTypes.h`, fra `Role` (`:112`) e il commento di `bActive` (`:114`):

```cpp

	/**
	 * L'azione a cui il legame vale (#3563, spec «la clip per abilita'» §2.3). `NAME_None` = binding di RUOLO, come
	 * prima di questo campo: il pool di `(eroe, ruolo)`.
	 *
	 * 🔑 **Un binding d'azione vive in un pool proprio**: `(eroe, ruolo, azione)`. Ha senso solo su `Cast` e
	 * `Attack`, gli unici beat che conoscono l'azione (D3), e solo per un'azione che il catalogo conosce:
	 * `ValidateCatalog` rifiuta entrambi i casi, perche' un binding che non suona mai non lo vedrebbe nessun test.
	 * ⚠️ Esiste solo da `formatVersion` 2 (`FRTAnimCatalog::FirstFormatVersionWithActionId`).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FName ActionId;
```

Il commento `:96-99`:

```cpp
 * ⚠️ **`bActive` e' unico dentro il POOL, non dentro la voce**: `(HeroId, Role)` per un binding di ruolo,
 * `(HeroId, Role, ActionId)` per uno d'azione (#3563). Una clip puo' essere attiva per Aevik/`Move` e
 * legata-ma-inattiva per Phase/`Move`, e una clip d'azione attiva convive con quella di ruolo dello stesso
 * `(eroe, ruolo)`: e' la stessa invariante che `FRTAnimRoleClips::ActiveClipVariant` porta a runtime, un pool alla
 * volta. Qui vive nel testo, e `ValidateCatalog` la difende — perche' un file lo si puo' modificare a mano.
```

`RTAnimCatalogTypes.h:218`:

```cpp
	static constexpr int32 CurrentFormatVersion = 2;

	/**
	 * Da quale versione un binding puo' portare `actionId` (#3563).
	 *
	 * 🔴 **Il bump e' la ragione per cui questa riga esiste**: una build v1 che ignorasse `actionId` leggerebbe un
	 * binding d'azione come binding di RUOLO — una clip sbagliata e attiva, in silenzio. Con la versione a 2 quella
	 * build rifiuta il file (sopra), e il reader nuovo rifiuta un `actionId` in un file dichiarato v1.
	 */
	static constexpr int32 FirstFormatVersionWithActionId = 2;
```

- [ ] **Step 4: Parse, scrittura, validazione**

`RTAnimCatalogLibrary.cpp`, dopo `#include "Serialization/JsonWriter.h"` (`:8`):

```cpp

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "UObject/Package.h"
```

Dopo `KeyActive` (`:40`):

```cpp
	const TCHAR* KeyActionId = TEXT("actionId");   // #3563: solo da formatVersion 2
```

Il commento `:394-395`:

```cpp
			// I binding (#2443). Assenti nei cataloghi scritti prima: un'assenza e' zero legami, non un
			// errore di formato. ⏱️ *Fino a #3563 questo commento giustificava il NON-bump di `formatVersion`; la
			// chiave `actionId` lo ha reso necessario — vedi `FirstFormatVersionWithActionId`.*
```

Fra la lettura del ruolo (`:423`) e `TryGetBoolField(KeyActive, …)` (`:425`):

```cpp

					// `actionId` (#3563): opzionale; assente o vuoto = binding di ruolo. ⛔ In un file dichiarato v1 e'
					// un ERRORE: una build v1 lo avrebbe ignorato, legando la clip al ruolo invece che all'azione.
					FString ActionText;
					if ((*BindingObj)->TryGetStringField(KeyActionId, ActionText) && !ActionText.IsEmpty())
					{
						if (OutCatalog.FormatVersion < FRTAnimCatalog::FirstFormatVersionWithActionId)
						{
							OutError = FString::Printf(
								TEXT("voce #%d ('%s'): 'actionId' in un catalogo di formato %d — esiste solo da %d"),
								Index, *IdText, OutCatalog.FormatVersion, FRTAnimCatalog::FirstFormatVersionWithActionId);
							return false;
						}
						Binding.ActionId = FName(*ActionText);
					}
```

`:463`:

```cpp
	// 🔑 **Sempre la versione CORRENTE, non quella letta** (#3563): un catalogo v1 a cui si aggiunge un `actionId`
	// deve uscire v2, o la sua rilettura lo rifiuterebbe. Ogni salvataggio di una build nuova e' v2, anche senza
	// `actionId`: una sola versione in circolazione, per scelta (spec §4, §6).
	Writer->WriteValue(KeyFormatVersion, FRTAnimCatalog::CurrentFormatVersion);
```

Dopo `Writer->WriteValue(KeyRole, RoleToString(Binding.Role));` (`:496`):

```cpp
			// ⚠️ **Solo quando c'e'**, al contrario dell'array `bindings` (sopra): un binding di ruolo resta scritto
			// come prima di #3563, quindi il diff di un catalogo che non usa le azioni non cambia riga per riga.
			if (!Binding.ActionId.IsNone())
			{
				Writer->WriteValue(KeyActionId, Binding.ActionId.ToString());
			}
```

Prima di `TArray<FString> URTAnimCatalogLibrary::ValidateCatalog(` (`:511`):

```cpp
namespace
{
	/** I soli beat che conoscono l'azione (spec D3): un `actionId` altrove non suonerebbe mai. */
	bool RTRuoloPropagaAzione(ERTPresentationRole Role)
	{
		return Role == ERTPresentationRole::Cast || Role == ERTPresentationRole::Attack;
	}

	/**
	 * Ogni `ActionId` che un beat puo' portare (#3563, spec §2.3): core, generiche, abilita' degli eroi (reazioni e
	 * fasi Environment comprese, `URTHeroData::Actions`) e azioni concesse dall'equipaggiamento (`Ruling` R10:
	 * `MakeEquipmentAction` riscrive `ActionId` con l'id del pezzo).
	 *
	 * ⚠️ **Costa**: costruisce il roster e i pezzi a ogni chiamata. `ValidateCatalog` la chiama SOLO se un binding
	 * nomina un'azione, e gira nel commandlet e nei test, mai in partita (spec §2.3, costo accettato).
	 */
	TSet<FName> RTAzioniConosciute()
	{
		TSet<FName> Azioni;
		for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
		{
			Azioni.Add(Def.ActionId);
		}
		for (const FName& Id : URTCatalogLibrary::GetGenericActionIds())
		{
			Azioni.Add(Id);
		}
		for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Eroe == nullptr) { continue; }
			for (const URTActionData* Azione : Eroe->Actions)
			{
				if (Azione != nullptr) { Azioni.Add(Azione->Def.ActionId); }
			}
		}
		TArray<URTEquipmentData*> Pezzi = URTCatalogLibrary::MakeWeaponVariants();
		Pezzi.Append(URTCatalogLibrary::MakeGadgets());
		Pezzi.Append(URTCatalogLibrary::MakeReactionModules());
		for (const URTEquipmentData* Pezzo : Pezzi)
		{
			// `nullptr` se il pezzo non concede un'azione, o ne dichiara una che il core non ha (`.cpp:920-930`).
			if (const URTActionData* Concessa = URTCatalogLibrary::MakeEquipmentAction(Pezzo, GetTransientPackage()))
			{
				Azioni.Add(Concessa->Def.ActionId);
			}
		}
		return Azioni;
	}
}

```

`:600-630` (commento e ciclo dell'unicità) diventano:

```cpp
	// 🔴 **Al piu' UNA variante attiva per POOL, e qui e' l'unico posto che puo' difenderlo.** Un pool e'
	// `(eroe, ruolo)` per un binding di ruolo, `(eroe, ruolo, azione)` per uno d'azione (#3563): una clip di ruolo
	// attiva e una d'azione attiva per lo stesso `(eroe, ruolo)` sono due pool, e convivono.
	//
	// A runtime l'invariante e' strutturale: `FRTAnimRoleClips::ActiveClipVariant` e' UN `FName`, e due
	// attive nello stesso pool non sono nemmeno rappresentabili. Nel testo lo sono — bastano due `"active": true`
	// scritti a mano, o un merge che unisce due rami che hanno legato la stessa Action.
	//
	// Senza questo controllo il commandlet dovrebbe scegliere quale delle due vince, e sceglierebbe per
	// posizione nell'array: cioe' l'autore vedrebbe cambiare la clip che suona riordinando un file.
	//
	// ⛔ **E un `actionId` deve poter suonare** (#3563): un'azione che il catalogo non conosce, o un ruolo che non
	// propaga l'azione, sono un binding che non suona mai — un errore, non un avviso, perche' nessun test lo vedrebbe.
	TSet<FName> AzioniConosciute;
	bool bAzioniCalcolate = false;   // l'insieme costa: si costruisce solo se un binding nomina un'azione
	TMap<TTuple<FName, ERTPresentationRole, FName>, FName> AttivaPerPool;
	for (const FRTAnimCatalogEntry& Entry : Catalog->Entries)
	{
		for (const FRTAnimBinding& Binding : Entry.Authored.Bindings)
		{
			if (!Binding.ActionId.IsNone())
			{
				if (!RTRuoloPropagaAzione(Binding.Role))
				{
					Errors.Add(FString::Printf(
						TEXT("%s / %s / %s ('%s'): il ruolo %s non conosce l'azione — solo Cast e Attack la propagano, e il binding non suonerebbe mai"),
						*Binding.HeroId.ToString(), *RoleToString(Binding.Role), *Binding.ActionId.ToString(),
						*Entry.Id.ToString(), *RoleToString(Binding.Role)));
				}
				if (!bAzioniCalcolate)
				{
					AzioniConosciute = RTAzioniConosciute();
					bAzioniCalcolate = true;
				}
				if (!AzioniConosciute.Contains(Binding.ActionId))
				{
					Errors.Add(FString::Printf(
						TEXT("%s / %s ('%s'): l'azione '%s' non e' nel catalogo — un refuso sarebbe un binding che non suona mai"),
						*Binding.HeroId.ToString(), *RoleToString(Binding.Role), *Entry.Id.ToString(),
						*Binding.ActionId.ToString()));
				}
			}

			if (!Binding.bActive)
			{
				continue;   // legata e inattiva e' lo stato normale: nessun vincolo di unicita'
			}
			const TTuple<FName, ERTPresentationRole, FName> Chiave(Binding.HeroId, Binding.Role, Binding.ActionId);
			if (const FName* Gia = AttivaPerPool.Find(Chiave))
			{
				const FString Pool = Binding.ActionId.IsNone()
					? FString::Printf(TEXT("%s / %s"), *Binding.HeroId.ToString(), *RoleToString(Binding.Role))
					: FString::Printf(TEXT("%s / %s / %s"), *Binding.HeroId.ToString(), *RoleToString(Binding.Role),
						*Binding.ActionId.ToString());
				Errors.Add(FString::Printf(
					TEXT("%s: '%s' e '%s' sono entrambe attive, e il pool ne ammette una sola"),
					*Pool, *Gia->ToString(), *Entry.Id.ToString()));
			}
			else
			{
				AttivaPerPool.Add(Chiave, Entry.Id);
			}
		}
	}
```

(`GetTypeHash` e `operator==` di `TTuple` sono del motore, `Templates/Tuple.h`. Se il compilatore non trova l'hash per `ERTPresentationRole` dentro la tupla, la chiave diventa `FString::Printf(TEXT("%s|%d|%s"), …)` — stessa semantica, dichiarata nel commit.)

`Data/Anim/AnimCatalog.json:2`, file LF, nessun binding da aggiungere:

```powershell
$p = "<checkout>/Data/Anim/AnimCatalog.json"
$t = [IO.File]::ReadAllText($p)
$n = ([regex]'"formatVersion": 1,').Replace($t, '"formatVersion": 2,', 1)
[IO.File]::WriteAllText($p, $n, [Text.UTF8Encoding]::new($false))
```

`git diff Data/Anim/AnimCatalog.json` → una sola riga cambiata; `file` → ancora senza `CRLF`.

- [ ] **Step 5: Build e verde**

Run: build; test `RefactorTactics.Anim`.
Expected: verdi, `**** TEST COMPLETE`. `Anim.Catalog.RejectsTwoActivePerRole`, `Anim.Browser.BindingRules`, `Anim.Bindings.MapToCdo` e i test di `Tests/RTAnimCatalogTests.cpp` verdi e invariati (nessuno pinna `formatVersion: 1` in scrittura: misurato con `grep -rn 'formatVersion\|FormatVersion' Source/RefactorTactics/Tests/RTAnimCatalogTests.cpp` → solo testi d'ingresso v1, che restano leggibili).

- [ ] **Step 6: Mutazioni (5), (9), (P5)**

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| (5) In `ValidateCatalog`, il blocco `if (!AzioniConosciute.Contains(Binding.ActionId)) { … }` commentato | `Anim.Catalog.RejectsUnknownActionId` | «🔴 un refuso e' un errore» |
| (9) Il blocco `if (!RTRuoloPropagaAzione(Binding.Role)) { … }` commentato | `Anim.Catalog.RejectsActionIdOnNonPropagatingRole` | «🔴 actionId valido su Move: errore» |
| (P5) `Writer->WriteValue(KeyFormatVersion, Catalog.FormatVersion);` | `Anim.Catalog.ActionIdRoundTrips` | «il file scritto dichiara formatVersion 2» e «🔴 la rilettura riesce» |

E una verifica di R10 per sottrazione: togli `Pezzi.Append(URTCatalogLibrary::MakeGadgets());` → `Anim.Catalog.AcceptsEquipmentActionId` cade su «🔴 l'azione di un gadget e' un'azione conosciuta». Ripristina.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Unit/RTAnimCatalogTypes.h Source/RefactorTactics/Unit/RTAnimCatalogLibrary.cpp Data/Anim/AnimCatalog.json Source/RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp
git commit -F - <<'EOF'
feat(3563): actionId nel catalogo animazioni - formatVersion 2, unicita' per pool, azioni valide esplicite

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 5: Commandlet e modello del browser — pool d'azione, fusione per pool, predicati con `ActionId`

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Content/RTBuildAnimBindingsCommandlet.h:27-32` (commento), `:59-60` (dichiarazione nuova accanto)
- Modify: `Source/RefactorTacticsEditor/Private/Content/RTBuildAnimBindingsCommandlet.cpp:43-44` (smistamento), dopo `:63` (`MergeClipsPerHero`), `:96-98`, `:124-127`, `:155-156`, `:163`
- Modify: `Source/RefactorTacticsEditor/Private/RTAnimBrowserModel.h:65-83`, `RTAnimBrowserModel.cpp` (`BindToRole` fino a `:187`, `MakeActive` `:189-218`, `Unbind` `:220-233`)
- Test: `Source/RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp` (test nuovi dopo `Anim.Bindings.MapToCdo`, `:357`)

**Interfaces:**
- Consumes: `FRTAnimBinding::ActionId` (Task 4), `FRTHeroPresentationClips::PerAction`, `ActiveClipFor` a quattro argomenti (Task 1).
- Produces:
  ```cpp
  // URTBuildAnimBindingsCommandlet — BuildClipsPerHero invariata nella firma e, per i binding di ruolo, nel risultato
  static TMap<FName, FRTHeroPresentationClips> MergeClipsPerHero(
      const TMap<FName, FRTHeroPresentationClips>& Base, const TMap<FName, FRTHeroPresentationClips>& PerEroe);
  // FRTAnimBrowserModel
  bool BindToRole(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);
  bool MakeActive(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);
  bool Unbind(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);
  ```

- [ ] **Step 1: I test rossi**

`RTAnimBrowserModelTests.cpp`, in coda al namespace anonimo (dopo `JsonConUnLegame` del Task 4):

```cpp

	/** Un pool di una variante attiva, per costruire a mano le mappe della fusione (#3563). */
	FRTAnimRoleClips PoolDiFusione(const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_Fusione")), FName(TEXT("A")), TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_Fusione")));
		return Pool;
	}

	FString PathAttivoDi(const FRTAnimRoleClips* Pool)
	{
		const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
		return Attiva ? Attiva->Clip.ToSoftObjectPath().ToString() : FString();
	}
```

Dopo la `}` di `Anim.Bindings.MapToCdo` (`:357`):

```cpp

// ─── Il pool d'azione nel CDO (#3563, spec «la clip per abilita'» §2.3) ──────────────────────────────────────

/**
 * Un binding con `actionId` va in `PerAction[azione].PerRole[ruolo]`, uno senza in `PerRole` — due pool.
 * ✅ Validato per mutazione (P6): lo smistamento sostituito da `Eroe.PerRole.FindOrAdd(Binding.Role)` → cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsMapToCdoPerActionTest,
	"RefactorTactics.Anim.Bindings.MapToCdoPerAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsMapToCdoPerActionTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	AggiungiLegame(Catalog, TEXT("AV_0003"), TEXT("Ability_Q_Target"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), false);

	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe = URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);
	if (!TestEqual(TEXT("tre legami tradotti"), Legami, 3)) { return false; }
	const FRTHeroPresentationClips* Aevik = PerEroe.Find(FName(TEXT("Hero.Aevik")));
	if (!TestNotNull(TEXT("Aevik c'e'"), (const void*)Aevik)) { return false; }

	const FRTAnimRoleClips* Ruolo = Aevik->FindRole(ERTPresentationRole::Cast);
	if (!TestNotNull(TEXT("il pool di ruolo Cast c'e'"), (const void*)Ruolo)) { return false; }
	TestEqual(TEXT("🔴 il pool di ruolo ha SOLO il binding di ruolo"), Ruolo->Variants.Num(), 1);
	TestEqual(TEXT("ed e' AV_0001, attiva"), Ruolo->ActiveClipVariant, FName(TEXT("AV_0001")));

	const FRTActionPresentationClips* Azione = Aevik->PerAction.Find(FName(TEXT("Hero.Aevik.Overload")));
	const FRTAnimRoleClips* PoolAzione = Azione ? Azione->PerRole.Find(ERTPresentationRole::Cast) : nullptr;
	if (!TestNotNull(TEXT("🔴 il pool d'azione Overload/Cast c'e'"), (const void*)PoolAzione)) { return false; }
	TestEqual(TEXT("con le due varianti d'azione"), PoolAzione->Variants.Num(), 2);
	TestEqual(TEXT("attiva quella dichiarata, AV_0002"), PoolAzione->ActiveClipVariant, FName(TEXT("AV_0002")));
	return true;
}

/**
 * Review Focus (b): un binding d'azione attivo e uno di ruolo attivo per lo stesso `(eroe, ruolo)` sono VALIDI, e a
 * risolvere vince l'azione; senza azione resta il ruolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsActionAndRoleBindingsCoexistTest,
	"RefactorTactics.Anim.Bindings.ActionAndRoleBindingsCoexist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsActionAndRoleBindingsCoexistTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	if (!TestEqual(TEXT("⛔ premessa: le due attive sono valide"), URTAnimCatalogLibrary::ValidateCatalog(&Catalog).Num(), 0))
	{
		return false;
	}
	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe = URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);

	// Il CDO e' l'unico `ActiveClipFor` disponibile: si scrive e si ripristina, come `ConfiguraVariante`.
	URTUnitAnimInstance* Cdo = GetMutableDefault<URTUnitAnimInstance>();
	const TMap<FName, FRTHeroPresentationClips> Salvato = Cdo->ClipsPerHero;
	ON_SCOPE_EXIT{ Cdo->ClipsPerHero = Salvato; };
	Cdo->ClipsPerHero = PerEroe;

	const FName Aevik(TEXT("Hero.Aevik"));
	TestEqual(TEXT("🔴 con l'azione: vince la clip d'azione"),
		Cdo->ActiveClipFor(Aevik, ERTPresentationRole::Cast, FName(TEXT("Hero.Aevik.Overload")), NAME_None).ToSoftObjectPath().ToString(),
		PathDi(TEXT("Gadget"), TEXT("Throw_Ready")));
	TestEqual(TEXT("senza azione: la clip di ruolo"),
		Cdo->ActiveClipFor(Aevik, ERTPresentationRole::Cast).ToSoftObjectPath().ToString(),
		PathDi(TEXT("Gadget"), TEXT("Cast")));
	return true;
}

/**
 * La fusione per pool (`Ruling` di §2.3, ➕ rev2.): eroi e pool che il catalogo non nomina tengono il default.
 * ✅ Validato per mutazione (8): `MergeClipsPerHero` ridotta a `return PerEroe;` → cade «eroe assente».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsMergeKeepsDefaultPoolsTest,
	"RefactorTactics.Anim.Bindings.MergeKeepsDefaultPools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsMergeKeepsDefaultPoolsTest::RunTest(const FString&)
{
	const FName Aevik(TEXT("Hero.Aevik"));
	const FName Ivrin(TEXT("Hero.Ivrin"));
	const FName Overload(TEXT("Hero.Aevik.Overload"));

	TMap<FName, FRTHeroPresentationClips> Base;
	FRTHeroPresentationClips& BaseAevik = Base.Add(Aevik);
	BaseAevik.PerRole.Add(ERTPresentationRole::Move, PoolDiFusione(TEXT("/Game/Prova/DefMove.DefMove")));
	BaseAevik.PerRole.Add(ERTPresentationRole::Cast, PoolDiFusione(TEXT("/Game/Prova/DefCast.DefCast")));
	Base.Add(Ivrin).PerRole.Add(ERTPresentationRole::Idle, PoolDiFusione(TEXT("/Game/Prova/DefIdle.DefIdle")));

	TMap<FName, FRTHeroPresentationClips> PerEroe;
	FRTHeroPresentationClips& DalCatalogo = PerEroe.Add(Aevik);
	DalCatalogo.PerRole.Add(ERTPresentationRole::Cast, PoolDiFusione(TEXT("/Game/Prova/CatCast.CatCast")));
	DalCatalogo.PerAction.FindOrAdd(Overload).PerRole.Add(ERTPresentationRole::Cast,
		PoolDiFusione(TEXT("/Game/Prova/CatOverload.CatOverload")));

	const TMap<FName, FRTHeroPresentationClips> Fuso = URTBuildAnimBindingsCommandlet::MergeClipsPerHero(Base, PerEroe);

	// 1. Eroe assente dal catalogo: tutti i pool del default.
	const FRTHeroPresentationClips* FusoIvrin = Fuso.Find(Ivrin);
	if (!TestNotNull(TEXT("🔴 eroe assente dal catalogo: resta"), (const void*)FusoIvrin)) { return false; }
	TestEqual(TEXT("con il suo Idle di default"), PathAttivoDi(FusoIvrin->FindRole(ERTPresentationRole::Idle)),
		FString(TEXT("/Game/Prova/DefIdle.DefIdle")));

	// 2. Eroe con solo Cast nel catalogo: Move del default, Cast del catalogo.
	const FRTHeroPresentationClips* FusoAevik = Fuso.Find(Aevik);
	if (!TestNotNull(TEXT("Aevik c'e'"), (const void*)FusoAevik)) { return false; }
	TestEqual(TEXT("🔴 il Move che il catalogo non nomina resta quello di default"),
		PathAttivoDi(FusoAevik->FindRole(ERTPresentationRole::Move)), FString(TEXT("/Game/Prova/DefMove.DefMove")));
	TestEqual(TEXT("il Cast che il catalogo nomina e' quello del catalogo"),
		PathAttivoDi(FusoAevik->FindRole(ERTPresentationRole::Cast)), FString(TEXT("/Game/Prova/CatCast.CatCast")));

	// 3. Il pool d'azione si aggiunge a PerAction senza toccare PerRole.
	const FRTActionPresentationClips* Azione = FusoAevik->PerAction.Find(Overload);
	TestEqual(TEXT("il pool d'azione del catalogo e' entrato"),
		PathAttivoDi(Azione ? Azione->PerRole.Find(ERTPresentationRole::Cast) : nullptr),
		FString(TEXT("/Game/Prova/CatOverload.CatOverload")));
	TestEqual(TEXT("e PerRole ha ancora i suoi due ruoli"), FusoAevik->PerRole.Num(), 2);
	return true;
}

/**
 * I predicati del modello distinguono `(eroe, ruolo)` da `(eroe, ruolo, azione)`: attivare un binding di ruolo non
 * spegne quello d'azione, e viceversa (spec §2.3, il quarto predicato).
 * ✅ Validato per mutazione (7): il ciclo atomico di `MakeActive` senza `&& Binding.ActionId == ActionId` → cade
 * «attivare il ruolo non spegne l'azione».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserBindingRulesPerActionTest,
	"RefactorTactics.Anim.Browser.BindingRulesPerAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserBindingRulesPerActionTest::RunTest(const FString&)
{
	FRTAnimBrowserModel M = ModelloDiProva();
	const FName Aevik(TEXT("Hero.Aevik"));
	const FName Overload(TEXT("Hero.Aevik.Overload"));
	const FName Idle(TEXT("AV_0001"));    // Promoted
	const FName Jog(TEXT("AV_0004"));     // Promoted

	auto Attiva = [&M](const FName& Id, const FName& Hero, ERTPresentationRole Role, const FName& ActionId) -> bool
	{
		for (const FRTAnimCatalogEntry& E : M.GetCatalog().Entries)
		{
			if (E.Id != Id) { continue; }
			for (const FRTAnimBinding& B : E.Authored.Bindings)
			{
				if (B.HeroId == Hero && B.Role == Role && B.ActionId == ActionId) { return B.bActive; }
			}
		}
		return false;
	};

	TestTrue(TEXT("lega Idle al ruolo Cast"), M.BindToRole(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔑 la STESSA clip si lega anche all'azione: e' un altro pool, non un duplicato"),
		M.BindToRole(Idle, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("lega Jog all'azione"), M.BindToRole(Jog, Aevik, ERTPresentationRole::Cast, Overload));

	TestTrue(TEXT("attiva Jog sull'azione"), M.MakeActive(Jog, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("attiva Idle sul ruolo"), M.MakeActive(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔴 attivare il ruolo non spegne l'azione"), Attiva(Jog, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("e il ruolo e' attivo"), Attiva(Idle, Aevik, ERTPresentationRole::Cast, NAME_None));
	TestFalse(TEXT("Idle sull'azione resta inattiva"), Attiva(Idle, Aevik, ERTPresentationRole::Cast, Overload));
	TestEqual(TEXT("il catalogo e' valido: una attiva per pool"), URTAnimCatalogLibrary::ValidateCatalog(&M.GetCatalog()).Num(), 0);

	TestTrue(TEXT("unbind del ruolo"), M.Unbind(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔴 non tocca il binding d'azione della stessa clip"),
		M.MakeActive(Idle, Aevik, ERTPresentationRole::Cast, Overload));
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build.
Expected: **rosso di compilazione** su `MergeClipsPerHero` (`error C2039`) e sugli overload a quattro argomenti di `BindToRole`/`MakeActive` (`C2660`).

- [ ] **Step 3: Il commandlet**

`RTBuildAnimBindingsCommandlet.cpp:43-44`:

```cpp
			FRTHeroPresentationClips& Eroe = PerEroe.FindOrAdd(Binding.HeroId);
			// 🔑 #3563: un binding d'azione va nel pool dell'AZIONE, mai in quello di ruolo — due pool, una attiva per
			// pool. Un binding di ruolo resta dov'era: per lui questa funzione non cambia.
			FRTAnimRoleClips& Ruolo = Binding.ActionId.IsNone()
				? Eroe.PerRole.FindOrAdd(Binding.Role)
				: Eroe.PerAction.FindOrAdd(Binding.ActionId).PerRole.FindOrAdd(Binding.Role);
```

Dopo la `}` di `BuildClipsPerHero` (`:63`):

```cpp

/**
 * Il catalogo SOPRA il default, un pool alla volta (#3563, `Ruling` di §2.3).
 *
 * 🔴 **Prima di questa funzione il commandlet SOSTITUIVA l'intera mappa**: con la classe autorata cablata (#3562),
 * un eroe senza binding avrebbe perso ogni clip. Qui ogni `(eroe, ruolo)` e ogni `(eroe, azione, ruolo)` che il
 * catalogo nomina sostituisce il pool di `Base` per intero; eroi e pool che non nomina restano quelli di `Base`.
 * ⚠️ Pura e separata da `BuildClipsPerHero`, che resta la traduzione del solo catalogo (`Anim.Bindings.MapToCdo`).
 */
TMap<FName, FRTHeroPresentationClips> URTBuildAnimBindingsCommandlet::MergeClipsPerHero(
	const TMap<FName, FRTHeroPresentationClips>& Base, const TMap<FName, FRTHeroPresentationClips>& PerEroe)
{
	TMap<FName, FRTHeroPresentationClips> Fuso = Base;
	for (const TPair<FName, FRTHeroPresentationClips>& Eroe : PerEroe)
	{
		FRTHeroPresentationClips& Dest = Fuso.FindOrAdd(Eroe.Key);
		for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Pool : Eroe.Value.PerRole)
		{
			Dest.PerRole.Add(Pool.Key, Pool.Value);   // `Add` su chiave esistente sostituisce: il pool intero
		}
		for (const TPair<FName, FRTActionPresentationClips>& Azione : Eroe.Value.PerAction)
		{
			FRTActionPresentationClips& DestAzione = Dest.PerAction.FindOrAdd(Azione.Key);
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Pool : Azione.Value.PerRole)
			{
				DestAzione.PerRole.Add(Pool.Key, Pool.Value);
			}
		}
	}
	return Fuso;
}
```

Il commento `:96-98`:

```cpp
	// ⛔ **Si rifiuta di generare da un catalogo non valido.** Due attive nello stesso POOL — `(eroe, ruolo)` o,
	// da #3563, `(eroe, ruolo, azione)` — sono rappresentabili nel testo e non a runtime: generare comunque
	// significherebbe sceglierne una per posizione nell'array, cioe' far dipendere la clip che suona dall'ordine
	// delle righe di un file. Lo stesso vale per un `actionId` che non suonerebbe mai.
```

Il commento `:124-127`:

```cpp
	// ⚠️ Rigenerare qui e' **non distruttivo per definizione**: questo asset non ha grafo né layout autorato, e
	// porta solo `ClipsPerHero` — che e' il default C++ con sopra i POOL che il catalogo nomina (#3563). Il
	// catalogo non possiede la mappa intera: possiede i pool che nomina. E' la differenza con
	// `RTBuildPlaygroundPanel`, che invece si rifiuta senza `-Force` perche' cancellerebbe un grafo.
```

Il commento `:153-156` e l'assegnazione `:163`:

```cpp
	// ── 3. Il CDO ───────────────────────────────────────────────────────────────────────────────────
	//
	// 🔑 Si scrive sul CDO della classe generata, non sull'oggetto Blueprint: e' il default che ogni istanza
	// erediterà. Il valore e' il default C++ della CLASSE BASE con sopra i pool del catalogo (#3563).
```

```cpp
	// ⛔ La base e' il CDO di `URTUnitAnimInstance`, NON quello della classe generata: quello si porterebbe dietro
	// la mappa della run precedente, e un pool tolto dal catalogo resterebbe nell'asset.
	Cdo->ClipsPerHero = URTBuildAnimBindingsCommandlet::MergeClipsPerHero(
		URTUnitAnimInstance::StaticClass()->GetDefaultObject<URTUnitAnimInstance>()->ClipsPerHero, PerEroe);
```

`RTBuildAnimBindingsCommandlet.h:27-32`:

```cpp
 * ⛔ **Non promuove niente e non lega niente.** Legge i binding che una persona ha scritto e li traduce, e li
 * FONDE sopra il default C++ un pool alla volta (#3563): eroi e pool che il catalogo non nomina tengono il
 * default. Se il catalogo non ha binding, l'asset generato porta il default e basta.
 *
 * ⛔ **Rifiuta un catalogo non valido.** Due varianti attive nello stesso pool sono rappresentabili nel testo
 * ma non a runtime: generare comunque significherebbe sceglierne una per posizione nell'array, cioe' far
 * dipendere la clip che suona dall'ordine delle righe di un file.
```

`RTBuildAnimBindingsCommandlet.h`, dopo `:60`:

```cpp

	/**
	 * Il catalogo sopra il default, un pool alla volta (#3563): ogni `(eroe, ruolo)` e `(eroe, azione, ruolo)` di
	 * `PerEroe` sostituisce quello di `Base`; il resto di `Base` resta. Pura, e provata da
	 * `Anim.Bindings.MergeKeepsDefaultPools`.
	 */
	static TMap<FName, FRTHeroPresentationClips> MergeClipsPerHero(
		const TMap<FName, FRTHeroPresentationClips>& Base, const TMap<FName, FRTHeroPresentationClips>& PerEroe);
```

- [ ] **Step 4: Il modello del browser**

`RTAnimBrowserModel.h:65-83`, le tre dichiarazioni:

```cpp
	/**
	 * Lega la clip a `(eroe, ruolo)` — o, con `ActionId`, a `(eroe, ruolo, azione)` (#3563). **Entra sempre
	 * INATTIVA**, qualunque sia lo stato del pool.
	 *
	 * ⛔ Rifiuta se la clip non e' `Promoted`: legare cio' che nessuno ha guardato e' esattamente il
	 * salto che questo strumento esiste per impedire.
	 */
	bool BindToRole(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);

	/**
	 * Rende attiva questa clip nel suo pool, **atomicamente**: qualunque altra attiva dello STESSO pool torna
	 * inattiva nello stesso passo. Un pool di ruolo e uno d'azione dello stesso `(eroe, ruolo)` non si toccano.
	 */
	bool MakeActive(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);

	/**
	 * Toglie il legame dal suo pool. Se era l'attiva, il pool resta **senza attiva** e non si elegge una
	 * sostituta: la scelta e' dell'autore, e il ripiego e' il livello successivo (l'azione → il ruolo).
	 */
	bool Unbind(const FName& Id, const FName& HeroId, ERTPresentationRole Role, const FName& ActionId = NAME_None);
```

`RTAnimBrowserModel.cpp`: le tre definizioni guadagnano `, const FName& ActionId` in firma (senza default); i **quattro** predicati diventano:

- `:174-175` → `[&HeroId, Role, &ActionId](const FRTAnimBinding& B) { return B.HeroId == HeroId && B.Role == Role && B.ActionId == ActionId; }`
- `:198-199` → lo stesso predicato
- `:211` → `if (Binding.HeroId == HeroId && Binding.Role == Role && Binding.ActionId == ActionId)` (e il commento `:205-206` dice «ogni altra attiva di QUESTO pool»)
- `:227-228` → lo stesso predicato della prima riga

In `BindToRole`, dopo `Binding.Role = Role;` (`:183`): `Binding.ActionId = ActionId;`.

`grep -n "B.HeroId == HeroId && B.Role == Role\|Binding.HeroId == HeroId && Binding.Role == Role" Source/RefactorTacticsEditor/Private/RTAnimBrowserModel.cpp` → quattro righe, **tutte** con `ActionId`. Il pannello (`SRTAnimBrowserPanel.cpp`) non chiama queste funzioni: nessun chiamante da aggiornare (`grep -rn "BindToRole\|\.Unbind(" Source/RefactorTacticsEditor --include=*.cpp | grep -v Tests` → solo `RTAnimBrowserModel.cpp`).

- [ ] **Step 5: Build e verde**

Run: build; test `RefactorTactics.Anim`.
Expected: verdi, `**** TEST COMPLETE`. `Anim.Bindings.MapToCdo` (`:277-357`) verde e **invariato** — asserisce ancora che Ivrin non abbia `Move` nell'uscita di `BuildClipsPerHero`; `Anim.Browser.BindingRules` verde e invariato.

- [ ] **Step 6: Mutazioni (7), (8), (P6)**

| Mutazione | Test | Asserto che deve cadere |
|---|---|---|
| (7) `RTAnimBrowserModel.cpp`, ciclo di `MakeActive`: `if (Binding.HeroId == HeroId && Binding.Role == Role)` | `Anim.Browser.BindingRulesPerAction` | «🔴 attivare il ruolo non spegne l'azione» |
| (8) Corpo di `MergeClipsPerHero` → `return PerEroe;` | `Anim.Bindings.MergeKeepsDefaultPools` | «🔴 eroe assente dal catalogo: resta» |
| (P6) Lo smistamento → `FRTAnimRoleClips& Ruolo = Eroe.PerRole.FindOrAdd(Binding.Role);` | `Anim.Bindings.MapToCdoPerAction` | «🔴 il pool di ruolo ha SOLO il binding di ruolo» |

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTacticsEditor/Private/Content/RTBuildAnimBindingsCommandlet.h Source/RefactorTacticsEditor/Private/Content/RTBuildAnimBindingsCommandlet.cpp Source/RefactorTacticsEditor/Private/RTAnimBrowserModel.h Source/RefactorTacticsEditor/Private/RTAnimBrowserModel.cpp Source/RefactorTacticsEditor/Private/Tests/RTAnimBrowserModelTests.cpp
git commit -F - <<'EOF'
feat(3563): commandlet e browser per azione - pool d'azione, fusione per pool sul default, predicati con ActionId

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 6: Il gate di cook — il set richiesto include le clip d'azione

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTPackagingConfigTests.cpp:207-275` (helper in coda al namespace), `:541-739` (gate: `:576-605`, `:631-641`, `:719-730`)
- Test: lo stesso file, test nuovo dopo la `}` del gate (`:739`)

**Interfaces:**
- Consumes: `FRTHeroPresentationClips::PerAction` (Task 1), il default di Task 3.
- Produces (namespace anonimo del file di test):
  ```cpp
  TArray<FString> RTRequiredAnimationPackages(const URTUnitAnimInstance* Cdo,
      TMap<FString, FString>& OutProvenienza, int32& OutTerneCoperte);
  int32 RTTerneConVarianteAttiva(const URTUnitAnimInstance* Cdo);
  ```

**Il gate è già rosso, e resta rosso.** La misura storica del runbook (`docs/technical/runbooks/guida-animazioni-paragon.md:283-285`) lo dà rosso per le clip di ruolo senza riferimento duro; le clip d'azione allargano il set. Il rosso è di #3562 e **non** misura questo sotto-progetto: la mutazione (4) cade nel test **nuovo e verde** `Packaging.RequiredSetIncludesActionClips`, che asserisce il SET e non il cook.

- [ ] **Step 1: Misura il gate PRIMA del codice**

Sul commit del Task 5 (default già presente, helper non ancora estratto):
Run: test `RefactorTactics.Packaging.RequiredAnimationClipsAreCooked`, log `<scratchpad>/cook-prima.log`.
Expected: `Result={Fail}`. Annota la riga dell'asserto finale (`grep -n 'clip richieste senza un riferimento' <scratchpad>/cook-prima.log`): con il ciclo di oggi le clip d'azione **non** sono nel set, quindi il numero è quello delle sole clip di ruolo. È un esito del passaggio corrente: va nella PR, non in un documento.

- [ ] **Step 2: Il test rosso**

Dopo la `}` del gate (`:739`):

```cpp

/**
 * Il set che il cook deve portare include le clip per AZIONE (#3563, spec «la clip per abilita'» §2.5) — verde, e
 * separato dal gate qui sopra che resta ROSSO per i riferimenti duri mancanti (#3562).
 *
 * 🔑 **Perche' un test a parte**: dentro un gate gia' rosso, la mutazione «`PerAction` saltato» si distinguerebbe
 * da un altro fallimento solo leggendo il messaggio. Qui asserisce il SET, e il suo verde e' informativo.
 * ✅ Validato per mutazione (4): il ciclo su `PerAction` dell'helper saltato → cade «terne coperte».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRequiredSetIncludesActionClipsTest,
	"RefactorTactics.Packaging.RequiredSetIncludesActionClips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRequiredSetIncludesActionClipsTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO del grafo di animazione"), Cdo)) { return false; }

	TMap<FString, FString> Provenienza;
	int32 TerneCoperte = 0;
	const TArray<FString> Richieste = RTRequiredAnimationPackages(Cdo, Provenienza, TerneCoperte);

	// 🔴 Anti-sottrazione su ENTRAMBI i pool: il conteggio indipendente conta ruoli e azioni.
	TestEqual(TEXT("🔴 terne coperte: il set richiesto copre ogni variante attiva, di ruolo E d'azione"),
		TerneCoperte, RTTerneConVarianteAttiva(Cdo));

	int32 ClipDAzioneViste = 0;
	for (const TPair<FName, FRTHeroPresentationClips>& Voce : Cdo->ClipsPerHero)
	{
		for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce.Value.PerAction)
		{
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Azione.Value.PerRole)
			{
				const FRTAnimVariant* Attiva = Ruolo.Value.FindActive();
				if (Attiva == nullptr || Attiva->Clip.IsNull()) { continue; }
				++ClipDAzioneViste;
				const FString Package = Attiva->Clip.ToSoftObjectPath().GetLongPackageName();
				const FString Chi = FString::Printf(TEXT("%s / %s / %s"), *Voce.Key.ToString(),
					*Azione.Key.ToString(), *UEnum::GetValueAsString(Ruolo.Key));
				TestTrue(*FString::Printf(TEXT("%s: la clip d'azione e' nel set richiesto"), *Chi), Richieste.Contains(Package));
				const FString* Prov = Provenienza.Find(Package);
				TestTrue(*FString::Printf(TEXT("%s: e la provenienza la nomina"), *Chi), Prov && Prov->Contains(Chi));
			}
		}
	}
	TestTrue(TEXT("⛔ premessa: il default porta clip per azione (Task 3)"), ClipDAzioneViste > 0);
	return true;
}
```

Run: build.
Expected: **rosso di compilazione** su `RTRequiredAnimationPackages` (`error C3861`).

- [ ] **Step 3: L'helper, e il gate che lo usa**

In coda al namespace anonimo di `:207-275`, prima della `}` di `:275`:

```cpp

	/**
	 * Il set di package che il cook deve portare: la variante ATTIVA di ogni pool del CDO — `(eroe, ruolo)` e, da
	 * #3563, `(eroe, azione, ruolo)`. Estratto dal gate perche' abbia un test proprio, verde
	 * (`RequiredSetIncludesActionClips`), mentre il gate resta rosso per i riferimenti duri (#3562).
	 *
	 * ⚠️ Un package puo' servire piu' pool (`Cast` = `Attack` sul ruolo; la stessa clip per due azioni): le
	 * provenienze si ACCODANO con `; `. `OutTerneCoperte` cresce per OGNI pool entrato, dopo l'inserimento: un
	 * `continue` futuro fra i due sottrarrebbe path al cook senza far divergere il conteggio indipendente.
	 */
	TArray<FString> RTRequiredAnimationPackages(const URTUnitAnimInstance* Cdo,
		TMap<FString, FString>& OutProvenienza, int32& OutTerneCoperte)
	{
		TArray<FString> Richieste;
		OutTerneCoperte = 0;
		auto Accoda = [&Richieste, &OutProvenienza, &OutTerneCoperte](const FSoftObjectPath& Path, const FString& Chi)
		{
			// Il PACKAGE path, non l'object path: la chiave della tabella di import e' `/.../Idle`.
			const FString Package = Path.GetLongPackageName();
			Richieste.AddUnique(Package);
			FString& Gia = OutProvenienza.FindOrAdd(Package);
			if (!Gia.IsEmpty()) { Gia += TEXT("; "); }
			Gia += Chi;
			++OutTerneCoperte;   // per ULTIMO
		};
		for (const TPair<FName, FRTHeroPresentationClips>& Voce : Cdo->ClipsPerHero)
		{
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Voce.Value.PerRole)
			{
				const FSoftObjectPath Path = Cdo->ActiveClipFor(Voce.Key, Ruolo.Key).ToSoftObjectPath();
				if (Path.IsNull()) { continue; }   // nessuna attiva: posa di riferimento, e va bene
				Accoda(Path, FString::Printf(TEXT("%s / %s"), *Voce.Key.ToString(), *UEnum::GetValueAsString(Ruolo.Key)));
			}
			// ➕ #3563: i pool d'AZIONE. La loro variante attiva entra nel set come quella di ruolo.
			for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce.Value.PerAction)
			{
				for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Azione.Value.PerRole)
				{
					const FRTAnimVariant* Attiva = Ruolo.Value.FindActive();
					const FSoftObjectPath Path = Attiva ? Attiva->Clip.ToSoftObjectPath() : FSoftObjectPath();
					if (Path.IsNull()) { continue; }
					Accoda(Path, FString::Printf(TEXT("%s / %s / %s"), *Voce.Key.ToString(), *Azione.Key.ToString(),
						*UEnum::GetValueAsString(Ruolo.Key)));
				}
			}
		}
		return Richieste;
	}

	/** Il conteggio INDIPENDENTE dei pool con una variante attiva, nei due livelli: il presidio anti-sottrazione. */
	int32 RTTerneConVarianteAttiva(const URTUnitAnimInstance* Cdo)
	{
		int32 Terne = 0;
		for (const TPair<FName, FRTHeroPresentationClips>& Voce : Cdo->ClipsPerHero)
		{
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Voce.Value.PerRole)
			{
				if (Ruolo.Value.FindActive() != nullptr) { ++Terne; }
			}
			for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce.Value.PerAction)
			{
				for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Azione.Value.PerRole)
				{
					if (Ruolo.Value.FindActive() != nullptr) { ++Terne; }
				}
			}
		}
		return Terne;
	}
```

Nel gate, `:576-605` (dichiarazioni di `Richieste`, `Provenienza`, `CoppieCoperte` e il doppio ciclo) diventano — i commenti `:552-575` restano, e si aggiunge in coda a loro:

```cpp
	// ➕ #3563: il calcolo vive in `RTRequiredAnimationPackages`, che attraversa ANCHE i pool d'azione con la
	// provenienza `Hero.X / Azione / Ruolo`; il suo test verde e' `RequiredSetIncludesActionClips`.
	TMap<FString, FString> Provenienza;
	int32 CoppieCoperte = 0;
	const TArray<FString> Richieste = RTRequiredAnimationPackages(Cdo, Provenienza, CoppieCoperte);
```

`:631-641` (il ciclo di `CoppieAttese`) diventa:

```cpp
	const int32 CoppieAttese = RTTerneConVarianteAttiva(Cdo);   // ➕ #3563: conta ruoli E azioni
```

e il testo dell'asserto `:643-644` dice `«… TUTTE le terne (eroe, [azione,] ruolo) con una variante attiva …»`.

Il messaggio `:719-730` diventa (owner `#3562`, non più `#2444`):

```cpp
		// 🔑 **Il messaggio dice COSA FARE, e non solo cosa manca.** Chi incontra questo rosso la prima
		// volta lo legge come un guasto dello strumento se non gli si spiega che il cook segue solo i
		// riferimenti duri, e che scriverlo e' lavoro di #3562 (eredita la chiusa #2444; proprietario dei
		// `BP_Unit_*`, che sono binari e non si mergiano). Da #3563 la provenienza nomina anche l'AZIONE.
		const FString* Chi = Provenienza.Find(Scoperta);
		AddError(FString::Printf(
			TEXT("%s e' la variante ATTIVA di [%s] e nessun asset versionato sotto Content/RT la ")
			TEXT("referenzia duro: il cook non ha nessuna dipendenza da seguire, e nel pacchetto ")
			TEXT("l'unita' resta in posa di riferimento (D-262). ")
			TEXT("Per chiudere: aggiungi il riferimento duro nel BP_Unit_ dell'eroe (#3562), oppure ")
			TEXT("disattiva la variante se non deve entrare nel pacchetto."),
			*Scoperta, Chi ? **Chi : TEXT("ruolo ignoto")));
```

`grep -n '#2444' Source/RefactorTactics/Tests/RTPackagingConfigTests.cpp` dopo la modifica: le righe che restano sono storiche (narrano il passato); nessuna dentro una stringa di `AddError`.

- [ ] **Step 4: Build, verde del test nuovo, e il gate misurato DOPO**

Run: build; test `RefactorTactics.Packaging`, log `<scratchpad>/cook-dopo.log`.
Expected: `Packaging.RequiredSetIncludesActionClips` `Result={Success}`; gli altri `Packaging.*` come prima; `Packaging.RequiredAnimationClipsAreCooked` `Result={Fail}` **dichiarato**, con l'asserto anti-sottrazione verde (le terne coincidono) e gli `AddError` che elencano i package per nome, anche quelli d'azione con provenienza `Hero.X / Hero.X.Y / ERTPresentationRole::…`. Confronta la riga «clip richieste senza un riferimento…» con quella dello Step 1: il set è cresciuto delle clip d'azione. Entrambe vanno nella PR come esiti del passaggio.

- [ ] **Step 5: Mutazione (4)**

Nell'helper, commenta il ciclo `for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce.Value.PerAction) { … }` di `RTRequiredAnimationPackages` (**non** quello di `RTTerneConVarianteAttiva`), build, test `RefactorTactics.Packaging.RequiredSetIncludesActionClips`: `Result={Fail}` su «🔴 terne coperte…» e su «…: la clip d'azione e' nel set richiesto». Ripristina, build, verde.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Tests/RTPackagingConfigTests.cpp
git commit -F - <<'EOF'
test(3563): il set richiesto dal cook include le clip per azione, con un test verde proprio; owner del gate #3562

Il gate RequiredAnimationClipsAreCooked resta rosso per i riferimenti duri dei BP_Unit_ (#3562): dichiarato.

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 7: Documenti — voce PIE, seduta, runbook, architettura, capability map, statuto della spec, radar

**Files:**
- Modify: `docs/technical/test-manuali-pie.md` — una riga **in coda** alla tabella di `## Scenari di validazione visiva — corpus Visual.*` (`:1894`; ultima riga oggi `PIE-CAST-BEAT`, `:1942`), e `## Stato in numeri` (`:90-98`)
- Modify: `docs/roadmap/editor-sessions.yaml` — una seduta **dentro `sessions:`**, dopo l'ultima (`U69`, `:5116`) e prima di `^not_schedulable:` (`:5138`)
- Modify: `docs/technical/runbooks/guida-animazioni-paragon.md` — il riquadro del catalogo (`:110-165`) e la riga del gate (`:283-285`)
- Modify: `docs/technical/architecture/architettura-codice.md:46` (riga `Unit/RTAnimCatalogTypes.h`)
- Modify: `docs/technical/architecture/capability-map.yaml` — la riga `issues:` del blocco `Animation Runtime & Catalog` (`:1675-1691`)
- Modify: `docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md` — lo statuto (`:3-12`)

**Interfaces:** nessuna di codice. Regole: l'esito atteso vive **solo** in `test-manuali-pie.md`; il file delle sedute cita gli **ID**, mai l'esito; nessun totale volatile; i file sono CRLF; nessuna pipe `|` dentro le celle; una nota con un numero di issue in una cella PIE va **in coda** alla cella.

- [ ] **Step 1: La voce PIE**

Prima di toccare il file: `node tools/radar/doc-coherence.ts --check` e annota il ricalcolo A1 che stampa (voci e stati). Poi, in coda alla tabella di `:1894`, dopo la riga di `PIE-CAST-BEAT` (`:1942`), una riga con le cinque colonne:

```markdown
| **PIE-CLIP-ABILITA** | Un'abilità suona la sua clip, non quella del ruolo ([#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563)) | Il banco Ability Lab di [#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532), un'abilità per volta; per le pose senza partita, l'anteprima dell'Anim Browser ([#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554)) | **(0)** Per ciascuna abilità con una clip di default **diversa** dalla clip di ruolo (spec §2.6), il beat di cast mostra **quella** clip e non la `Cast` del pack: la posa è un'altra. **(1)** Per `Hero.Branth.Ram` e `Hero.Ivrin.PassingBlade`, anche il colpo mostra la clip d'abilità (per Ram è l'impatto della carica). **(2)** Un'abilità senza voce (un attacco base sul cast, `Hero.Muiren.FlowReaction`) mostra la `Cast` di prima. **(3)** Da guardare per prime: `R_Ability_Intro` (CircularTide) e `Ability_R_InMotion` (PassingBlade), che potrebbero essere lunghe o preludere a un loop. ⚠️ Le clip sono scelte **dai nomi** dei pack: il giudizio estetico è dell'autore, e cambiare una clip è una riga di `MakeActionClips` più la sua gemella nel test. ⚠️ Il catalogo JSON non arriva al gioco finché i `BP_Unit_*` non impostano la classe autorata: qui suona il default C++. Coperto headless: `Unit.DefaultActionClipsResolveForEveryKitAbility`, `Playback.ActivationPlaysTheActionClip`, `Playback.ChargeImpactPlaysTheDashAttackClip`; qui resta ciò che nessun test vede — la posa a schermo, e che il nome sia davvero la clip giusta | ⏳ da eseguire — scritta il 2026-10-07 insieme al codice di [#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563); il verdetto è dell'autore; dipende da [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562) solo per il catalogo JSON, non per il default |
```

Poi rilancia `node tools/radar/doc-coherence.ts --check`: il nuovo ricalcolo A1 deve dare **una voce in più, nuova e ⏳**, nessun'altra voce cambiata.

🔑 La riga dei totali di `## Stato in numeri` (`:92`) **è il valore che A1 misura** e A1 la confronta con il ricalcolo: riscrivila con i numeri che il tool stampa **dopo** l'inserimento, nella stessa forma. Non è un totale volatile ai sensi di `AGENTS.md` §14: un gate la rimisura a ogni `--check`.

La riga «Rimisurato» nuova va **subito sotto** la riga dei totali, sopra quelle esistenti (`:94`, `:96`, `:98`: dalla più recente in giù), nella forma di `:96`:

```markdown
➕ **Rimisurato il <data> ([#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563), la clip per abilità): il ricalcolo di `doc-coherence` A1 dava `<prima>` **prima** di toccare il file e `<dopo>` **dopo**.** Delta **uno, nuova e ⏳** — `PIE-CLIP-ABILITA`: nessuna voce esistente cambia stato.
```

(`<prima>` e `<dopo>` sono le due stringhe A1 misurate in questo step: esiti del passaggio corrente, ammessi da `AGENTS.md` §14.)

- [ ] **Step 2: La seduta**

Misura l'ultimo id: `grep -n "^  - id: U" docs/roadmap/editor-sessions.yaml | tail -1` (in pianificazione `U69`, `:5116`); usa il successivo. Inserisci **dopo l'ultima seduta di `sessions:` e prima della riga `not_schedulable:`** — mai in fondo al file: una `U3563` accodata finisce in `not_schedulable:` senza errore.

```yaml
  - id: U70
    title: La clip per abilita' — ogni abilita' suona la sua clip
    block: 6
    critical: false
    execution_lane: pie
    produces: >-
      il verdetto su `PIE-CLIP-ABILITA`; nessun asset
    artifacts: []
    unblocked_by: []
    shares_setup_with: []
    verifies: [PIE-CLIP-ABILITA]
    issues: [3563]
    done_when: >-
      la voce `PIE-CLIP-ABILITA` ha un verdetto nel registro, dato a schermo da una persona dopo il merge del codice
    notes: |
      Scritta il 2026-10-07 dal piano `docs/superpowers/plans/2026-10-07-clip-per-abilita.md`.
      L'esito atteso vive in `test-manuali-pie.md`; qui solo l'allestimento: il banco Ability Lab (#3532), un'abilita'
      per volta, con il default C++ (il catalogo JSON non arriva al gioco senza #3562). Le clip si possono vedere prima,
      senza partita, nell'anteprima dell'Anim Browser.
```

Verifica:

```powershell
python -c "import yaml; d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8')); print([x['id'] for x in d['sessions']][-2:], len(d['not_schedulable']))"
```

Expected: l'elenco termina con l'id nuovo; il secondo numero è quello di prima dell'inserimento (misuralo prima con lo stesso comando). `file docs/roadmap/editor-sessions.yaml` → `with CRLF line terminators`.

- [ ] **Step 3: Architettura e capability map**

`architettura-codice.md:46`: nella cella della riga `Unit/RTAnimCatalogTypes.h`, `RTAnimCatalogLibrary`, dove elenca i campi di `FRTAnimCatalog`/`FRTAnimBinding`, aggiungi in coda alla cella questo testo, alla lettera (nessuna pipe nella cella):

```markdown
Da #3563: `formatVersion` 2 e `FRTAnimBinding::ActionId` (vuoto = binding di ruolo); `ValidateCatalog` difende una sola attiva **per pool** — `(eroe, ruolo)` o `(eroe, ruolo, azione)` — e rifiuta un `actionId` ignoto o su un ruolo diverso da `Cast`/`Attack`.
```

`capability-map.yaml`, blocco `Animation Runtime & Catalog`: `3563` in coda a `issues: { open: [...] }`. La lista `files` **non si tocca**: nomina già gli header e il dato del catalogo, e il modello del browser sta nel blocco accanto (`:2252-2266`), che non cambia. Lo spostamento di `3563` in `closed` è del Task 8 Step 4.

- [ ] **Step 4: Il runbook delle animazioni**

`guida-animazioni-paragon.md`, nel riquadro del catalogo, dopo il paragrafo «Genera `/Game/RT/Anim/ABP_RTUnitAuthored` …» (`:162-163`), un paragrafo nuovo nella forma `> …` del riquadro:

```markdown
>
> 🎯 **L'asse per azione, dal 2026-10-07** ([#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563),
> spec [`2026-10-07-clip-per-abilita-design.md`](../../superpowers/specs/2026-10-07-clip-per-abilita-design.md)).
> Un binding può portare `actionId`: la clip vale per quell'abilità su quel ruolo, sopra la clip di ruolo. La
> risoluzione è `(ActionId, ruolo)` → `(BaseActionId, ruolo)` → `ruolo`, e solo i beat `Cast` e `Attack` conoscono
> l'azione. Il default C++ porta già una clip per le abilità di ogni kit del roster, scelta dai nomi dei pack
> (`MakeActionClips` in `URTUnitAnimInstance`): il catalogo la sovrascrive un pool alla volta.
>
> - **`formatVersion` 2.** Un `actionId` esiste solo da v2, e il reader lo rifiuta in un file dichiarato v1. ⚠️ **Ogni
>   salvataggio da una build nuova produce un file v2, anche senza nessun `actionId`**: le build vecchie lo
>   rifiutano per versione. È voluto — una sola versione in circolazione, nessun file «v1 ma scritto da v2».
> - **Validazione.** Un `actionId` deve essere un'azione conosciuta (core, generiche, abilità degli eroi, azioni
>   concesse dall'equipaggiamento come `Gadget.Sprinkler`) e stare su `Cast` o `Attack`: altrimenti il commandlet
>   non genera. «Una sola attiva» vale **per pool**: una clip di ruolo attiva e una d'azione attiva per lo stesso
>   `(eroe, ruolo)` convivono, e a suonare è l'azione.
> - **Fusione per pool.** Il commandlet parte dal default C++ della classe base `URTUnitAnimInstance` e sostituisce
>   solo i pool che il catalogo nomina: eroi e pool senza binding tengono il default, e una run precedente del
>   commandlet non lascia traccia nel risultato. ⚠️ Un pool d'autore si toglie solo con una voce nel catalogo: un
>   binding vuoto esplicito è un follow-up, non esiste ancora.
> - **Il pannello non lega.** Il modello ha l'API per `(eroe, ruolo, azione)`, il pannello no: oggi si lega
>   scrivendo il JSON, e il commandlet lo valida.
```

La riga del gate (`:283-285`): sostituisci «Il gesto appartiene a **#2444**.» con:

```markdown
Il gesto appartiene a **[#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562)**, che eredita
la chiusa #2444. ➕ Da [#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563) il set richiesto
include anche le clip per **azione** del default, quindi il rosso cresce con loro; il set stesso è pinnato dal test
verde `RefactorTactics.Packaging.RequiredSetIncludesActionClips`. Il numero di clip scoperte si rimisura lanciando il
gate: ⏱️ *la frase citata qui sopra è la misura del 2026-09-05, ed è storica.*
```

- [ ] **Step 5: Lo statuto della spec**

`docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md:3-5`: lo statuto diventa «design **accettato** il 2026-10-07, rivisto dal panel (`➕ rev.`, `➕ rev2.`), **implementato** in [#3563](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3563) (PR #<pr>); la voce `PIE-CLIP-ABILITA` resta da eseguire». `<pr>` si scrive nel Task 8 dopo l'apertura della PR (un commit di una riga, come `5e27ac88e` per #2454).

- [ ] **Step 6: Il radar**

```powershell
node tools/radar/doc-coherence.ts --check
node tools/radar/doc-tables.ts --check
node tools/radar/doc-links.ts --check
```

Expected: nessun rosso nuovo. Poi **tutti** i controlli del radar — l'elenco si misura, non si ricorda: `ls tools/radar/*.ts | grep -v test`, e lancia con `--check` ciascuno che lo accetta. Un rosso preesistente su `origin/main` si dichiara col nome del controllo, non si corregge qui.

- [ ] **Step 7: Commit**

```bash
git add docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml docs/technical/runbooks/guida-animazioni-paragon.md docs/technical/architecture/architettura-codice.md docs/technical/architecture/capability-map.yaml docs/superpowers/specs/2026-10-07-clip-per-abilita-design.md
git commit -F - <<'EOF'
docs(3563): voce PIE-CLIP-ABILITA e seduta; runbook dell'asse per azione, formatVersion 2 e fusione per pool

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 8: Chiusura — suite, PR, review, merge, issue

**Files:** nessuno nuovo. GitHub.

- [ ] **Step 1: Suite completa sul commit finale**

`git fetch -q origin` e `git merge-base --is-ancestor origin/main HEAD`: se `origin/main` è avanzato, fondilo prima di misurare (un branch indietro falsa la misura). Run: test con filtro `RefactorTactics`, log in scratchpad. Expected: `**** TEST COMPLETE`; `Result={Fail}` = **solo** `Packaging.RequiredAnimationClipsAreCooked` (rosso dichiarato, #3562) e gli eventuali rossi che il log di `origin/main` già porta — confrontali con una run su `origin/main` o con il log di una suite recente sullo stesso commit di base. Annota lo SHA (`git rev-parse HEAD`).

- [ ] **Step 2: Push e PR verso il parent**

Il parent è `main` (`git config branch.issue/3563-clip-per-abilita.parent`). Corpo in `<scratchpad>/pr-clip.md`, `--body-file`, con: cosa; i gate eseguiti (`PASS` col nome del log e lo SHA); le mutazioni (1)–(9) e (P1)–(P6) con il test caduto; il gate di cook prima e dopo (Task 6 Step 1 e Step 4) come esiti del passaggio; i gate `NOT RUN` scritti **in parole** (`PIE: NOT RUN` — voce `PIE-CLIP-ABILITA`, seduta del Task 7 Step 2; `Packaged: NOT RUN`; commandlet su asset vero: `NOT RUN`, provato sulle funzioni pure); le righe `OK <Pack>/<Clip>` del Task 3 Step 1, le stesse del commit, col comando che le produce; ⛔ nessun glifo di stato (⏳ ✅ ❌ 🟡) accanto al nome di una voce PIE, né nel titolo né nel corpo; nessun totale volatile; chiusura `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

```bash
git push -u origin issue/3563-clip-per-abilita
gh pr create --repo DegrassiAaron/refactor-tactics-main --base main --title "La clip per abilita' (#3563)" --body-file "<scratchpad>/pr-clip.md"
```

Poi il commit di una riga dello statuto della spec (Task 7 Step 5) con il numero della PR, e push.

- [ ] **Step 3: Code review**

`/code-review` sulla PR; accogli i findings con lo skill `receiving-code-review` (verifica, non adesione). Un fix è un commit nuovo e un giro di test sullo SHA nuovo — chi ha scritto la correzione non ne firma da solo il verdetto.

- [ ] **Step 4: Merge, pulizia, issue**

Prima del merge, come **ultimo commit del branch**: in `docs/technical/architecture/capability-map.yaml`, blocco `Animation Runtime & Catalog`, sposta `3563` da `open` a `closed` nella riga `issues: { open: [...], closed: [...] }` — la forma delle voci chiuse lì è il numero in coda all'array `closed` (oggi `closed: [2443, 2445, 1663]`, `:1691`; rimisura la riga con `grep -n`). Diventa vero nell'istante del merge, che chiude la issue; farlo dopo richiederebbe un'altra PR. `node tools/radar/doc-coherence.ts --check` verde, poi:

```bash
git add docs/technical/architecture/capability-map.yaml
git commit -F - <<'EOF'
docs(3563): la issue passa fra le chiuse della capability map

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
git push
```

Merge dopo review verde. Poi `git branch -D issue/3563-clip-per-abilita`, `git remote prune origin`. Aggiorna la issue: DoD spuntato nel **commento** di chiusura con lo SHA del merge. Lo stato della seduta si scrive **in parole** — `PIE: NOT RUN (PIE-CLIP-ABILITA, verdetto dell'autore a schermo)` — ⛔ mai un glifo di stato accanto al nome della voce: il commento è un corpo GitHub come gli altri.

---

## Self-review del piano (eseguito in scrittura)

**Copertura della spec, sezione per sezione.**
- §2.1 modello → Task 1 Step 3. §2.2 risoluzione, `Ruling` su `BaseActionId`, seam per ruolo, consumatori → Task 1 Step 4, Task 2 Step 3. §2.3 catalogo (`formatVersion` 2, writer sempre corrente, reader che rifiuta v1+`actionId`, insieme esplicito con equipaggiamento R10, ruoli D3) → Task 4; commandlet (smistamento, `MergeClipsPerHero` sul CDO della classe base, commenti `:96`, `:126`, `:155-156`) → Task 5 Step 3; browser (quattro predicati) → Task 5 Step 4. §2.4 default e commenti (`.cpp:43-47`, `RTUnitTests.cpp:611-612`, `ValidateCatalog` `:600`, commandlet `:96-98`) → Task 3 Step 4-5, Task 4 Step 4, Task 5 Step 3. §2.5 gate (helper + test verde, messaggio `#3562`, `CastRoleResolvesAClipForEveryHero` esteso, test esistenti invariati) → Task 6, Task 3 Step 2. §2.6 mappa e misura sul disco → Task 3 Step 1 e Step 4. §4 errori e degrado → test di Task 1 (non popolato, variante non attiva, `BaseActionId` vuoto), Task 4 (ignoto, v1+`actionId`, due attive, v2 senza `actionId`), Task 3 Step 1 (path assente → degrado e gate). §5.1 → ogni test elencato ha il suo step; §5.2 → Task 7 Step 1. §6 limiti → Task 7 Step 4 (runbook) e PR.
- **Iniezione dei path nei test di playback**: è `Ruling` R11 della spec (§5.1), non una deviazione (Task 2).
- `➕ piano.` **«Estesi alla chiave nuova» (spec §2.5) = affiancati da un gemello.** `Anim.Bindings.MapToCdo`, `Anim.Browser.BindingRules`, `Anim.Catalog.RejectsTwoActivePerRole` e `Playback.ActivationPlaysTheCastCue` restano **invariati** e verdi (lo verificano gli step di build dei Task 2, 4 e 5); la chiave nuova la esercitano `MapToCdoPerAction`, `BindingRulesPerAction`, `RejectsTwoActivePerActionRole` e `ActivationPlaysTheActionClip`, cioè i test elencati in §5.1. Così un rosso sul vecchio dice «regressione sui binding di ruolo», uno sul nuovo «la chiave d'azione».
- **Testi pubblicati**: nessun glifo di stato accanto a una voce PIE in issue, PR, commenti e commit (Global Constraints; Task 0, Task 8 Step 2 e Step 4).

**Premesse verificate in scrittura.** Le reazioni degli eroi stanno in `URTHeroData::Actions` (`AddAbility`, `Ability/RTHeroCatalogLibrary.cpp:159`; `ReactiveCapacitor` a `:416`): la tabella del Task 3 le trova, e `RTAzioniConosciute` le include. Tutti gli `ActionId` della mappa esistono nel catalogo eroi (`grep -ohE 'TEXT\("Hero\.(Aevik|Muiren|Branth|Ivrin)\.[A-Za-z]+"\)' Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp | sort -u`). Tutti i nomi di clip della mappa esistono sul disco del clone principale (Task 3 Step 1, misurato il 2026-10-07).

**Placeholder.** Nessun «TBD»; `3563`, `<pr>`, `<checkout>`, `<scratchpad>`, `<prima>`/`<dopo>`, `<data>` sono variabili definite nei Global Constraints o nello step che le misura.

**Tipi fra task.** `FRTActionPresentationClips::PerRole` e `FRTHeroPresentationClips::PerAction` (Task 1) sono usati con lo stesso nome nei Task 2, 3, 5, 6; `ActiveClipFor(HeroId, Role, ActionId, BaseActionId)` ha la stessa firma ovunque; `FRTAnimBinding::ActionId` (Task 4) è `FName` con `NAME_None` = ruolo nei Task 4 e 5; `LastResolvedClipPathForTest` restituisce `FSoftObjectPath` e i test chiamano `.ToString()`; `MergeClipsPerHero(Base, PerEroe)` ha la firma della spec rev2.

**Review Focus.** (a) Task 1 `ActionEntryWithoutTheRoleFallsBack` + (P1); (b) Task 5 `ActionAndRoleBindingsCoexist` + metà validità in Task 4; (c) Task 4 `FormatVersion2WithoutActionIdLoads`; (d) Task 3 `DefaultActionClipsResolveForEveryKitAbility` + (P3); (e) Task 2 `ChargeImpactPlaysTheDashAttackClip` + (6).
