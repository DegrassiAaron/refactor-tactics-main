# `Ctrl` al posto di `Alt` — piano di implementazione, parte uno

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** il modificatore della camera passa da `Alt` a `Ctrl`, sinistro e destro; nessun gesto cambia significato, e `Alt` non è più mappato.

**Architecture:** il tasto vive in un punto solo, `ARTPlayerController::BuildInputMappings`, che mappa `CameraModifierAction` con due `MapKey`. Tutta la famiglia di gesti legge poi il booleano `bCameraModifier`, non il tasto: cambiare i due `MapKey` sposta l'intera famiglia. Il resto del lavoro è far dire la verità a ciò che nomina il tasto: identificatori, commenti, contratto della camera, registro delle decisioni, voci PIE.

**Tech Stack:** Unreal Engine 5.8.1, C++ (`RefactorTactics`), Enhanced Input costruito in C++; Automation `RefactorTactics.<Famiglia>.<Nome>`; radar `node tools/radar/*.ts --check`.

**Spec:** [`2026-10-13-ispettore-del-mondo-design.md`](../specs/2026-10-13-ispettore-del-mondo-design.md), fetta `F4` (§3, §8 `R1`, §9.3 voce 3, §9.4). **Issue:** [#3749](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3749). **Branch:** `issue/3749-ctrl-al-posto-di-alt`, da `origin/main` = `3ccc707c5`, nel worktree `D:/Repositories/rt-wt-ispettore`.

**Che cosa NON è in questo piano:** la parte due di #3749, cioè il fuoco — `Ctrl` + `LMB` click che fissa la scheda e mostra l'area di movimento. Aspetta [#3748](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3748) e [#3750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3750). Dopo questo piano il click col modificatore fa ciò che fa oggi: sposta il pivot.

## Global Constraints

- **Mai toccare il clone principale** `D:/Repositories/refactor-tactics-main`: ogni comando gira nel worktree.
- **Nessun gesto cambia significato**: soglia click/trascinamento, dolly che sopprime l'annulla, peek, pan di precisione e orbita su `MMB` restano come sono. Si cambia il tasto, non la logica.
- **Entrambi i `Ctrl`**, come oggi entrambi gli `Alt` (spec §9.3 voce 3).
- **Il motore è uno per macchina.** Prima di ogni build e di ogni run: leggere le `CommandLine` dei processi `UnrealEditor*`. La suite intera ha asserti sul tempo reale: si lancia solo in una finestra concordata con le altre sessioni.
- **Build e test in comandi separati**; l'exit code della build si legge subito, nello stesso comando.
- **Il registro delle decisioni non si riscrive**: dove una voce nomina il tasto vecchio la si **data**, in coda.
- **Una nota in una cella PIE va in coda**, prima della barra finale.
- **Niente totali volatili** nei testi; i fatti si citano per simbolo, non per riga.
- **Nessun merge** senza l'ok esplicito dell'autore.

## Stato misurato (2026-10-13, `3ccc707c5`)

| Fatto | Dove |
|---|---|
| Il tasto è mappato in un punto solo: due `MapKey(CameraModifierAction, …)` su `EKeys::LeftAlt` e `EKeys::RightAlt` | `ARTPlayerController::BuildInputMappings` |
| **Nel modulo di gioco** nessun altro codice legge `EKeys::LeftAlt`, `RightAlt`, `IsAltDown`, e nessuno legge `LeftControl`, `RightControl`, `IsControlDown` | `grep -rnE "EKeys::(Left\|Right)(Alt\|Control)\|IsAltDown\|IsControlDown" Source/RefactorTactics`: le due righe del controller e una riga di test |
| ➕ rev. **Nel modulo Editor `Ctrl` è già letto**, come modificatore additivo dell'editor esagonale: `FSlateApplication::Get().GetModifierKeys().IsControlDown()` in `RTHexEditorClick.cpp`, e `IsControlDown` in `Tools/RTHexArchTool.cpp`. Sono gestori di click della modalità editor. ⚠️ Non verificato che tacciano durante PIE | `Source/RefactorTacticsEditor/Private` |
| L'azione si chiama `IA_CameraModifier` (`NewObject`), e l'unico scrittore di `bCameraModifier` fuori dai test sono `OnCameraModifierPressed` e `OnCameraModifierReleased` | `RTPlayerController.cpp` |
| Il test `PlayerInput.HotkeysDoNotCollide` controlla che `LeftAlt` sia mappato, e solo il sinistro | `Tests/RTPlayerInteractionTests.cpp` |
| Identificatori che portano `Alt` nel nome: `bAltPrimaryDown`, `bAltSecondaryDown`, `AltPrimaryDragDistance`, `SetAltPrimaryDownForTest`, `GetAltPrimaryDragDistanceForTest`, il test `Camera.AltDragOrbitsOnlyPastTheClickThreshold` (classe `FRTCameraAltDragOrbitsOnlyPastThresholdTest`). Nessun documento cita quel test | `Player/RTPlayerController.h`, `.cpp`, `Tests/RTCameraPawnTests.cpp`; `git grep AltDragOrbitsOnlyPastTheClickThreshold -- ':!Source'` → niente |
| **La soppressione dell'annulla durante il dolly non ha un test Automation.** `OnUndoWaypoint` la decide con `IsInputKeyDown(EKeys::RightMouseButton)`, che headless è sempre falso | `grep -n AltSecondary Source/RefactorTactics/Tests/*.cpp` → niente |
| `Config/DefaultInput.ini` porta `bAltEnterTogglesFullscreen=True`: è del motore, e resta | — |
| Documenti vivi che nominano il tasto: `spec-tactical-camera.md` §3.2, §3.3, §4; `test-manuali-pie.md` (percorso di `PIE-PC-GYM` passi 3–5, la nota sopra la sua tabella, la cella di stato di `PIE-V01-DOCKCLICK`); `editor-sessions.yaml` (note della seduta di #1859); `OPEN_DECISIONS.md` (`CAM-B`, chiusa); Decision Log (D-458 punto 3, D-499 punto 3) | `git grep -nIE '\x60Alt\x60\|Alt ?\+' -- docs ':!docs/archive' ':!docs/roadmap/plans' ':!docs/superpowers'` |
| **Nessuna voce PIE ha mai giudicato a schermo un gesto con `Alt`.** `PIE-PC-GYM` è `NOT RUN` dal 2026-08-30; `PIE-CAM-STRATEGIC` scrive che `Alt`+`LMB` «non è esercitato a schermo»; `PIE-V01-DOCKCLICK` è aperta | `test-manuali-pie.md` |
| ⚠️ `STALE`: la nota sopra `PIE-PC-GYM` e le note della seduta dicono che il pan di precisione col modificatore «NON esiste». Esiste: `RouteCameraGesture`, ramo `bCameraModifier && bOrbiting` | `RTPlayerController.cpp` |

## Rulings

| | Ruling | Costo se è sbagliato |
|---|---|---|
| **P1** | Gli identificatori con `Alt` nel nome si rinominano con un nome **senza tasto**: `bModifierPrimaryDown`, `bModifierSecondaryDown`, `ModifierPrimaryDragDistance`, `SetModifierPrimaryDownForTest`, `GetModifierPrimaryDragDistanceForTest`, `Camera.ModifierDragOrbitsOnlyPastTheClickThreshold` | un diff più largo di due righe, tutto meccanico. Lasciarli vuol dire un campo `bAltSecondaryDown` acceso da `Ctrl` |
| **P2** | La soppressione dell'annulla durante il dolly **non** riceve un test nuovo: il cambio non tocca quel ramo, e provarlo chiede un seam sullo stato dei tasti. La DoD di #3749 la dava per coperta da Automation: si corregge la DoD, e la copre il passo 5 di `PIE-PC-GYM` | il ramo resta difeso solo da una voce PIE mai eseguita. Vale già oggi |
| **P3** | I verbali datati non si riscrivono: la cella di stato di `PIE-CAM-STRATEGIC`, il verbale di `editor-sessions.yaml` che parla dell'`Alt` «dello script», il testo di `CAM-B`. Si aggiunge una riga datata dove il testo è un'istruzione ancora da eseguire | chi legge un verbale trova il tasto vecchio: è ciò che è successo quel giorno |
| **P4** | La spec dell'ispettore non si tocca in questo branch: la PR #3752 ne modifica le stesse righe. Il rimando a D-514 entra con chi mergia per secondo | un rimando mancante per la durata di una PR |

## Review Focus

1. **`AltGr` arma il modificatore, prima e dopo.** Su una tastiera italiana `AltGr` arriva a Windows come `Ctrl` sinistro più `Alt` destro. Oggi lo arma passando da `RightAlt`; dopo il cambio lo armerà passando da `LeftControl`. ∴ il criterio non è «`Alt` non produce più effetti», che sull'`Alt` destro di quella tastiera è falso: è **«i due `Alt` non sono più mappati»**. A schermo si prova il solo `Alt` sinistro. ⚠️ Come il motore consegni `AltGr` non è verificato.
2. **`Ctrl` tenuto più un tasto di gioco, dentro l'Editor.** Mentre si orbita con `Ctrl` + sinistro la mano tocca i tasti di gioco: `W` `A` `S` `D`, `Q` `E`, `Spazio` (lock-in), `P` (pausa della preparazione), `Tab` (vista strategica). Nell'Editor alcuni di questi con `Ctrl` sono accordi — salva, content drawer, apri asset, selettore delle schede. In un gioco impacchettato non esistono. ⚠️ Non verificato chi dei due riceva il tasto per primo: è un passo di `PIE-PC-GYM`.
3. **Un `Alt` rimasto mappato per un'altra azione.** Il test nuovo lo vieta per ogni azione del contesto, e lo prova la seconda mutazione del Task 1.
4. **Un solo `Ctrl`.** La prima mutazione del Task 1 toglie il destro e deve dare rosso.
5. **Un rename a metà.** Dopo il Task 2 `grep` non deve trovare `AltPrimary`, `AltSecondary`, `AltDrag` in `Source/`, modulo Editor compreso.

---

### Task 0: Preflight

- [ ] **Step 1: stato del worktree**

```bash
cd "D:/Repositories/rt-wt-ispettore" && git branch --show-current && git rev-parse --short HEAD && git status --short
```

Expected: `issue/3749-ctrl-al-posto-di-alt`, `3ccc707c5`, albero pulito salvo questo piano.

- [ ] **Step 2: chi tiene il motore**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select-Object ProcessId, Name, CommandLine | Format-List
```

Expected: nessun processo, oppure processi di altri cloni. Se uno sta misurando tempi, si aspetta la sua finestra.

- [ ] **Step 3: il numero della decisione, prima di scriverlo nel codice**

```bash
git fetch -q origin && git show origin/main:docs/decisions/RT_PDR_00_Decision_Log.md | grep -oE '^\| \*\*D-[0-9]+\*\*' | tail -1
for r in $(git for-each-ref --format='%(refname)' refs/remotes/origin); do git grep -lE 'D-51[1-9]' "$r" -- docs/decisions/RT_PDR_00_Decision_Log.md; done
```

Expected: `origin/main` arriva a D-510; fra i ref remoti il solo `docs/3747-voci-decision-log-ispettore` porta D-512 e D-513; D-511 è di un'altra sessione, in locale. ∴ **D-514**. Se la misura dice altro, il numero cambia qui, prima del Task 1.

---

### Task 1: il tasto, con il suo test

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp` (test nuovo dopo `FRTPlayerInputHotkeysDoNotCollideTest`; l'anti-vacuità di quel test)
- Modify: `Source/RefactorTactics/Player/RTPlayerController.cpp` (`BuildInputMappings`)

**Interfaces:**
- Consumes: `ARTPlayerController::BuildAndGetMappingContextForTest()`, già usata dal test vicino.
- Produces: il test `RefactorTactics.PlayerInput.CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped`.

- [ ] **Step 1: scrivere il test che fallisce**

Subito dopo la chiusura di `FRTPlayerInputHotkeysDoNotCollideTest::RunTest`:

```cpp
// --- #3749 · il modificatore della camera e' `Ctrl`, su entrambi i lati ------------------------------

/**
 * Il tasto del modificatore vive in un punto solo: i due `MapKey` di `BuildInputMappings`. Tutta la
 * famiglia di gesti legge poi `bCameraModifier`, quindi questo test e' l'unico posto in cui «quale tasto»
 * si puo' asserire.
 *
 * Tre cose, e la terza e' quella che D-514 chiede con le parole «i due `Alt` non sono piu' mappati».
 * ⚠️ Mappati, non «senza effetto»: su una tastiera italiana `AltGr` arriva come `Ctrl` sinistro piu' `Alt`
 * destro, e arma il modificatore passando dal `Ctrl`. Questo test non lo puo' vedere, e non lo afferma.
 *  1. l'azione esiste nel contesto (anti-vacuita': senza, gli altri due asserti sarebbero veri di niente);
 *  2. i suoi tasti sono ESATTAMENTE i due `Ctrl` — non «almeno il sinistro», che e' cio' che il test
 *     vicino controllava per `Alt`;
 *  3. nessuna azione del contesto, non solo questa, sta su un `Alt`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputCameraModifierIsCtrlTest,
	"RefactorTactics.PlayerInput.CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputCameraModifierIsCtrlTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const UInputMappingContext* Imc = PC ? PC->BuildAndGetMappingContextForTest() : nullptr;
	if (TestNotNull(TEXT("mapping context costruito"), Imc))
	{
		TSet<FKey> ModifierKeys;
		for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
		{
			if (!M.Action) { continue; }
			if (M.Action->GetName() == TEXT("IA_CameraModifier"))
			{
				ModifierKeys.Add(M.Key);
			}
			TestFalse(*FString::Printf(TEXT("%s non sta su un Alt (%s)"), *M.Action->GetName(), *M.Key.ToString()),
				M.Key == EKeys::LeftAlt || M.Key == EKeys::RightAlt);
		}

		TestTrue(TEXT("il modificatore camera e' nel contesto"), ModifierKeys.Num() > 0);
		TestTrue(TEXT("il modificatore sta sul Ctrl sinistro"), ModifierKeys.Contains(EKeys::LeftControl));
		TestTrue(TEXT("il modificatore sta sul Ctrl destro"), ModifierKeys.Contains(EKeys::RightControl));
		TestEqual(TEXT("il modificatore non ha altri tasti oltre ai due Ctrl"), ModifierKeys.Num(), 2);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}
```

- [ ] **Step 2: build, poi vederlo fallire**

```powershell
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex; $rc = $LASTEXITCODE; "build rc=$rc"
```

Expected: `build rc=0`. Poi, in un comando separato, dal worktree:

```bash
python tools/suite/esegui.py RefactorTactics.PlayerInput --log "<scratchpad>/3749-rosso.log"
```

Expected: `CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped` **rosso**, con quattro messaggi: due «non sta su un Alt», «sta sul Ctrl sinistro», «sta sul Ctrl destro». Ogni altro test della famiglia verde.

- [ ] **Step 3: il cambio**

In `ARTPlayerController::BuildInputMappings`:

```cpp
	// #1771, #3749 — **entrambi** i `Ctrl` ([D-514]): chi tiene il destro con la mano sul mouse deve poter
	// orbitare come chi tiene il sinistro. Enhanced Input consegna la stessa azione da due tasti senza
	// doppioni. ⌫ *Fino al 2026-10-13 erano i due `Alt`.*
	MappingContext->MapKey(CameraModifierAction, EKeys::LeftControl);
	MappingContext->MapKey(CameraModifierAction, EKeys::RightControl);
```

E nell'anti-vacuità di `FRTPlayerInputHotkeysDoNotCollideTest`: `bAlt` diventa `bCtrl`, la chiave `EKeys::LeftControl`, il messaggio `Ctrl e' mappato (modificatore camera)`; nel commento sopra il test, «su entrambi gli `Alt`» diventa «su entrambi i `Ctrl`».

- [ ] **Step 4: build, poi vederlo passare**

Stessi due comandi, log `3749-verde.log`.

Expected: l'intera famiglia `RefactorTactics.PlayerInput` verde.

- [ ] **Step 5: commit, a verde**

```bash
git add Source/RefactorTactics/Player/RTPlayerController.cpp Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "feat(3749): il modificatore della camera e' Ctrl, sinistro e destro"
```

- [ ] **Step 6: due mutazioni, sopra il commit**

Insieme, in una sola build: (a) commentare il `MapKey` di `RightControl`; (b) aggiungere `MappingContext->MapKey(UndoAction, EKeys::LeftAlt);`. Build, rilanciare la famiglia (log `3749-mutazione.log`).

Expected: `CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped` rosso con tre messaggi — «sta sul Ctrl destro», «non ha altri tasti oltre ai due Ctrl» per (a), e «IA_UndoWaypoint non sta su un Alt (LeftAlt)» per (b), che è la prova che il terzo asserto guarda ogni azione. `HotkeysDoNotCollide` resta verde: `LeftAlt` non serve due azioni.

Ripristino: `git diff --stat` deve mostrare il solo `RTPlayerController.cpp`; poi `git checkout -- Source/RefactorTactics/Player/RTPlayerController.cpp`, build, famiglia verde (log `3749-ripristino.log`). Il binario si considera ripristinato solo dopo questa build verde, non dopo il checkout.

---

### Task 2: i nomi e i commenti dicono `Ctrl`

**Files:**
- Modify: `Source/RefactorTactics/Player/RTPlayerController.h`, `Source/RefactorTactics/Player/RTPlayerController.cpp`
- Modify: `Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`

**Interfaces:**
- Produces (rename, nessun comportamento): `bModifierPrimaryDown`, `bModifierSecondaryDown`, `ModifierPrimaryDragDistance`, `SetModifierPrimaryDownForTest(bool, float)`, `GetModifierPrimaryDragDistanceForTest()`, il test `RefactorTactics.Camera.ModifierDragOrbitsOnlyPastTheClickThreshold` (classe `FRTCameraModifierDragOrbitsOnlyPastThresholdTest`).

- [ ] **Step 1: rename degli identificatori**, con uno script che sostituisce le parole intere elencate qui sotto nei tre file, e stampa quante sostituzioni ha fatto per parola e per file. Atteso, dalla misura (`.cpp` + `.h` del controller, oppure il file di test): `AltPrimaryDragDistance` 7 + 3, `bAltPrimaryDown` 6 + 2, `bAltSecondaryDown` 4 + 1, `SetAltPrimaryDownForTest` 1 nel `.h` e 1 nel test, `GetAltPrimaryDragDistanceForTest` 1, `FRTCameraAltDragOrbitsOnlyPastThresholdTest` 2, `AltDragOrbitsOnlyPastTheClickThreshold` 1. La sostituzione è a parola intera (`\b…\b`): nessuno di questi nomi ne contiene un altro come parola intera, quindi l'ordine non conta. Un numero diverso ferma il passo.

- [ ] **Step 2: i commenti.** Nei tre file, e nelle righe di `RTPlayerInteractionTests.cpp` già toccate, `` `Alt` `` come nome del modificatore diventa `` `Ctrl` ``; nelle stringhe dei test `Alt` diventa `Ctrl`. A mano, non a tappeto, quattro punti:
  - `RTPlayerController.h`, il commento di `CameraModifierAction`: la riga su `Alt`+`Enter` e `bAltEnterTogglesFullscreen` diventa una riga datata — con `Ctrl` il caso non si pone più, e l'opzione del motore resta com'è;
  - `RTPlayerController.cpp`, il commento sopra il binding `Canceled` del modificatore: la ragione scritta è la finestra che perde il fuoco «con `Alt` premuto», cioè `Alt`+`Tab`. Il binding resta, perché una finestra perde il fuoco anche con `Ctrl` tenuto; la frase si riscrive senza l'esempio che non vale più;
  - i commenti che raccontano una code review datata restano al passato, col tasto di allora nominato come «il modificatore»;
  - **non si toccano**: la variabile locale `Alt` di `RTPlayerInteractionTests.cpp` (un'unità), `Variant.Alt` in `RTHeroCatalogTests.cpp`.

- [ ] **Step 3: la misura del rename**

```bash
grep -rnE "AltPrimary|AltSecondary|AltDrag" Source ; echo "rc=$?"
grep -rnE "\bAlt\b" Source --include=*.h --include=*.cpp
```

Expected: il primo `rc=1` (niente), su tutto `Source/`, modulo Editor compreso. Il secondo: la variabile locale `Alt` dei test, `Variant.Alt`, la riga datata su `Alt`+`Enter`, e i `⌫` che dicono «erano i due `Alt`». (`bAltEnterTogglesFullscreen` non compare: `\bAlt\b` non lo trova, e va bene così.) Ogni altra riga è un difetto del passo 2.

- [ ] **Step 4: build, poi le due famiglie**

Build come sopra; poi `RefactorTactics.Camera` e `RefactorTactics.PlayerInput`, log `3749-rename-camera.log` e `3749-rename-input.log`.

Expected: tutte verdi; `Camera.ModifierDragOrbitsOnlyPastTheClickThreshold` compare, il nome vecchio no. Trovati = verdi + rossi in entrambi i log.

- [ ] **Step 5: commit**

```bash
git add Source/RefactorTactics/Player/RTPlayerController.h Source/RefactorTactics/Player/RTPlayerController.cpp Source/RefactorTactics/Tests/RTCameraPawnTests.cpp Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "refactor(3749): i gesti del modificatore non portano piu' Alt nel nome"
```

---

### Task 3: contratto, registro, voci PIE

**Files:**
- Modify: `docs/technical/systems/spec-tactical-camera.md` (§3.2, §3.3, §4)
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` (riga e nota di D-514; coda datata in D-458 e D-499)
- Modify: `docs/technical/test-manuali-pie.md` (`PIE-PC-GYM`: passi 3–5, un passo nuovo in coda, la nota `STALE`; `PIE-V01-DOCKCLICK`: nota in coda)
- Modify: `docs/roadmap/editor-sessions.yaml` (note della seduta di #1859)
- Modify: `docs/OPEN_DECISIONS.md` (`CAM-B`: una riga datata)

- [ ] **Step 1: `spec-tactical-camera.md`.** Il titolo di §3.2 e le cinque righe della famiglia dicono `Ctrl`; sotto la tabella una riga `⌫`: fino al 2026-10-13 il modificatore era `Alt`, e lo cambia D-514. Il blocco `CAM-B` resta al passato e guadagna una riga: la scelta «entrambi» non cambia, il secondo modo è `Ctrl`+`LMB`. §3.3 e §4 dicono `Ctrl`.

- [ ] **Step 2: Decision Log.** Il numero è quello misurato al Task 0. La riga: il titolo, che dice «i due `Alt` non sono più mappati»; il fatto (`CameraModifierAction` sui due `Alt`, #1771); la domanda che l'autore aveva davanti e la sua risposta *«sostituisci alt con ctrl»*; la lettura `R1` della sessione — tutta la famiglia, non il solo click — dichiarata come tale col suo costo; i gesti in prosa; che cosa non cambia (soglia, dolly, `MMB`, `CAM-B`, D-499 punto 3, D-458 punto 3); che cosa non decide (il fuoco, parte due); che `Ctrl` è già il modificatore additivo dell'editor esagonale; i ⚠️ non verificati del Review Focus, con `AltGr` scritto per ciò che è: arma prima e arma dopo. In coda alle celle d'impatto di D-458 e D-499 una riga datata che rimanda alla voce nuova. I testi originali di D-458 e D-499 non si toccano.

- [ ] **Step 3: voci PIE.** In `PIE-PC-GYM` i passi 3, 4 e 5 dicono `Ctrl`. Si aggiunge **un** passo in coda al percorso, in tre parti: (a) l'`Alt` **sinistro** tenuto non arma nessun gesto camera, e `AltGr` lo arma ancora, come prima del cambio: non è un difetto; (b) con `Ctrl` tenuto, in uno stato sicuro, `S`, `W`, `P`, `Spazio` e `Tab` fanno ciò che fanno senza `Ctrl`, e nessun dialogo o pannello dell'Editor compare; (c) se qualcosa compare, si registra quale tasto e in quale finestra. La frase che conta i passi del percorso perde il numero. La nota sopra la tabella che dice che il pan di precisione «NON esiste» **resta**, e guadagna sotto una riga datata: esiste, in `RouteCameraGesture`, e il percorso non lo esercita ancora. In `PIE-V01-DOCKCLICK` una nota **in coda** alla cella di stato: il dolly è `Ctrl` più destro.

- [ ] **Step 4: `editor-sessions.yaml` e `OPEN_DECISIONS.md`.** Le `notes:` della seduta di #1859 sono un verbale del 2026-08-30 e **non si riscrivono**: si accoda una riga datata, che dice che dal cambio i gesti di quella seduta si eseguono con `Ctrl`. In `CAM-B` una riga datata sotto la risposta. Il file YAML è CRLF: si verifica con `yaml.safe_load` che le chiavi di primo livello, il numero di sedute e quello di `not_schedulable` non cambino.

- [ ] **Step 5: i radar**

```bash
for f in $(ls tools/radar/*.ts | grep -v '\.test\.ts$'); do node "$f" --check >/dev/null 2>&1 || echo "ROSSO $f"; done
```

Expected: gli stessi due rossi di `main`, `pie-verdict-age` e `wiki-alt`.

- [ ] **Step 6: la misura dei documenti**

```bash
git grep -nIE '`Alt`|Alt ?\+' -- docs ':!docs/archive' ':!docs/roadmap/plans' ':!docs/superpowers'
```

Expected: restano, e ciascuna si nomina nel messaggio della PR —
`Alt+P` e `Ctrl+Alt+F11`, scorciatoie dell'Editor (`test-manuali-pie.md`, `debug-vs-unreal.md`, D-473);
i testi originali di D-458 punto 3 e di D-499 punto 3, con la riga datata in coda;
la voce nuova, che nomina il tasto vecchio per dire che cosa cambia;
`CAM-B` in `OPEN_DECISIONS.md` e nel blocco di `spec-tactical-camera.md`, al passato;
la riga `⌫` sotto la tabella di §3.2;
la cella di stato di `PIE-CAM-STRATEGIC` e il corpo di `PIE-V01-DOCKCLICK`, con la nota in coda;
la nota sul pan di precisione sopra `PIE-PC-GYM`, e la parte (a) del passo nuovo;
le `notes:` della seduta di #1859 e il verbale che parla dell'`Alt` dello script, in `editor-sessions.yaml`.
Ogni altra riga è un difetto.

- [ ] **Step 7: commit**

```bash
git add docs/technical/systems/spec-tactical-camera.md docs/decisions/RT_PDR_00_Decision_Log.md docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml docs/OPEN_DECISIONS.md
git commit -m "docs(3749): contratto della camera, la voce del registro e le voci PIE dicono Ctrl"
```

---

### Task 4: la suite intera, la PR

- [ ] **Step 1: finestra pulita.** Chiedere alle altre sessioni una finestra; leggere le `CommandLine`; annotare `HEAD`, albero e hash della DLL prima.

- [ ] **Step 2: la suite**

```bash
python tools/suite/esegui.py RefactorTactics --log "<scratchpad>/3749-suite.log"
```

Expected: un solo rosso, `RefactorTactics.Packaging.RequiredActionClipsAreCooked` (preesistente, owner #3562). Trovati = verdi + rossi. `HEAD`, albero e DLL identici dopo.

- [ ] **Step 3: revisione avversaria del branch**, contesto fresco, sola lettura, tetto di chiamate; i rilievi critici e importanti si correggono in una passata sola.

- [ ] **Step 4: push e PR** verso `main`, con la tabella dei gate: Compile, Tests, Determinism `N/A`, Replay `N/A`, Privacy `N/A`, PIE `NOT RUN`, Packaged `NOT RUN`. Correggere la DoD di #3749 (`P2`, e il fatto che nessuna voce PIE aveva un verdetto su `Alt`).

- [ ] **Step 5: fermarsi.** Il merge è dell'autore.

---

## ➕ rev. I rilievi del revisore avversario

Un agente con contesto fresco, in sola lettura, tetto di trenta chiamate, il 2026-10-13. Nessun rilievo critico.
**I task qui sopra li portano tutti**, accolti il 2026-10-14: questa tabella resta come traccia di che cosa è
cambiato, e perché.

| | Gravità | Rilievo | Correzione accolta nei task |
|---|---|---|---|
| **B1** | importante | Task 1: il passo 5 ripristina con `git checkout --`, ma il commit è al passo 6. Eseguito in ordine, cancella il cambio del passo 3 | Ordine vero: commit a verde subito dopo il passo 4, **poi** la mutazione, poi il checkout. La frase correttiva in fondo al task si toglie |
| **B2** | importante | «Nessun codice legge `Control`» vale solo per `Source/RefactorTactics`. Nel modulo Editor `Ctrl` è già letto: `RTHexEditorClick.cpp` (`GetModifierKeys().IsControlDown()`, selezione additiva) e `Tools/RTHexArchTool.cpp` | La riga della tabella dice «nel modulo di gioco». I grep dei Task 2 e 3 girano su `Source/`. D-514 scrive che `Ctrl` è già il modificatore additivo dell'editor esagonale. ⚠️ Non verificato che quei tool tacciano durante PIE |
| **B3** | importante | `AltGr` non è un rischio **nuovo**: oggi arma già il modificatore passando da `RightAlt`. E «`Alt` non produce più effetti» è falso per l'`Alt` destro di una tastiera italiana | Il criterio è «i due `Alt` non sono più mappati». Il passo PIE prova il solo `Alt` sinistro, e scrive che `AltGr` arma come prima. D-514 e il commento del test usano la stessa frase |
| **B4** | importante, non verificato | Manca il rischio più probabile: con `Ctrl` tenuto si toccano i tasti di gioco `W A S D`, `Q E`, `Spazio`, `P`, che nell'Editor possono diventare accordi (`Ctrl`+`S`, `Ctrl`+`Spazio`, `Ctrl`+`P`) | Il passo PIE nuovo: con `Ctrl` tenuto premere `S`, `W`, `P`, `Spazio` in uno stato sicuro; nessun dialogo dell'Editor, e il tasto fa ciò che fa senza `Ctrl` |
| **B5** | importante | `D-514` entra nel codice al Task 1, prima della rimisura del Task 3 | La rimisura dei numeri passa al Task 0. In alternativa i commenti del Task 1 nascono senza numero |
| **B6** | importante | Le `notes:` della seduta di #1859 in `editor-sessions.yaml` sono un verbale datato: sostituire `Alt` con `Ctrl` lo rende falso, e contraddice `P3` | Una riga datata in coda, senza sostituire |
| **B7** | minore | L'`Expected` del Task 3 passo 6 non elenca ciò che il piano stesso lascia: gli originali di D-458 e D-499, `CAM-B` in due file, `PIE-CAM-STRATEGIC`, il corpo di `PIE-V01-DOCKCLICK` | Elencare le righe attese per nome |
| **B8** | minore | Totali in prosa: «Sedici passi», «le sei parole» (sono sette, con la classe e la stringa del test) | Togliere il numero; citare le sette parole per nome |
| **B9** | minore | La mutazione non prova che il terzo asserto valga per **ogni** azione | Seconda mutazione: `MapKey(UndoAction, EKeys::LeftAlt)` deve dare rosso su `IA_UndoWaypoint` |
| **B10** | minore | Il commento sopra il binding `Canceled` del modificatore motiva con «la finestra perde il fuoco con `Alt` premuto», cioè `Alt`+`Tab`: con `Ctrl` la ragione cambia | È un quarto punto da riscrivere a mano nel Task 2 |
| **B11** | minore | `git commit -am` nei Task 2 e 3 porterebbe dentro anche le correzioni al piano | Percorsi espliciti, come nel Task 1 |
| **B12** | minore | `\bAlt\b` non può trovare `bAltEnterTogglesFullscreen`: l'elenco atteso del Task 2 passo 3 è impreciso | Correggere l'elenco, non il comando |

**Controllato dal revisore, e regge**: il tasto è mappato in un punto solo e il contesto di input è uno; l'elenco
degli identificatori con `Alt` è completo e i conteggi per file tornano; il rename non tocca niente di serializzato
né di citato fuori da `Source/`; la soppressione dell'annulla nel dolly non ha test; il test nuovo dà nel rosso i
quattro messaggi attesi, e la mutazione sul `Ctrl` destro uccide.

**Non controllato**: come il motore consegna `AltGr` e gli accordi con `Ctrl` dentro l'Editor; gli asset binari
(una legenda dei tasti in un `WBP` non si vede col grep); i numeri delle decisioni, che chiedono `git` e `gh`.

---

## ➕ esecuzione. Che cosa è stato misurato (2026-10-14)

I log sono nello scratchpad della sessione che ha eseguito, coi nomi `3749-<giro>-…`.

| Giro | Commit o stato | Esito |
|---|---|---|
| `rosso` | test nuovo, tasti ancora sugli `Alt` | `RefactorTactics.PlayerInput`: un rosso, `CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped`, coi quattro messaggi attesi |
| `verde` | i due `MapKey` sui `Ctrl` | la famiglia intera verde; commit `6db0579db` |
| `mutazione` | sopra `6db0579db`: tolto il `MapKey` di `RightControl`, aggiunto `MapKey(UndoAction, EKeys::LeftAlt)` | un rosso, coi tre messaggi attesi: «IA_UndoWaypoint non sta su un Alt (LeftAlt)», «sta sul Ctrl destro», «non ha altri tasti oltre ai due Ctrl». `HotkeysDoNotCollide` verde |
| `rename` | file ripristinato con `git checkout`, poi il rename | `PlayerInput` e `Camera` verdi; compare `Camera.ModifierDragOrbitsOnlyPastTheClickThreshold`, il nome vecchio no; commit `484866a95` |

**Ruling `E1`.** Il piano chiedeva una build e un giro verde di solo ripristino dopo la mutazione, prima del rename. Sono stati fusi nel giro `rename`: il ripristino è `git checkout` del solo file mutato, controllato col diff, e il rename ricompila lo stesso file. La prova che il binario non porta più la mutazione è il verde di `CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped` in quel giro, cioè il test che la mutazione aveva fatto cadere. Costo se è sbagliato: nessuno che quel verde non scopra.

## ➕ rev2. I rilievi del revisore avversario sul branch

Un secondo agente con contesto fresco, in sola lettura, sul branch finito. Nessun rilievo critico: nel sorgente non c'è logica cambiata oltre ai due `MapKey` e ai rename. Corretti in una passata sola, prima della suite intera.

| | Gravità | Rilievo | Correzione |
|---|---|---|---|
| **C1** | importante | Il passo 17 di `PIE-PC-GYM`, parte (a), poteva dare un falso FAIL: con l'`Alt` sinistro il destro è quello di gioco e toglie un waypoint, mentre il passo 5 chiede che nessun waypoint sia cancellato | Il passo ha una precondizione, dice che il waypoint tolto è l'esito atteso, e che i criteri dei passi 3–5 lì non valgono |
| **C2** | minore | Un commento datato 2026-08-30 diceva `Ctrl`+`LMB`: quel giorno era `Alt` | «il modificatore col sinistro» |
| **C3** | minore | `B10` accolto a metà: il commento sul binding `Canceled` era una sostituzione meccanica | riscritto senza il tasto, con la causa: un clic fuori, un cambio di finestra |
| **C4** | minore | Il commento del header mandava `AltGr` al passo 17, che non chiedeva di premerlo | il passo 17 ha una parte (c) per `AltGr` |
| **C5** | minore | Il punto (c) di D-514 (8), l'editor esagonale, non ha un passo; i tasti del passo non combaciavano con quelli della voce | D-514 lo dichiara senza passo; il passo elenca `W` `A` `S` `D`, `P`, `Tab`, e `Spazio` a parte |
| **C6** | minore | D-514 citava la richiesta d'autore troncata, senza dirlo | citata per intero, e il punto (7) nomina il terzo elemento |
| **C7** | minore | «i cinque gesti della tabella» nella spec della camera: un totale che invecchia | «i gesti della tabella» |
| **C8** | minore | D-514 (4): «sinistro o destro» ambiguo fra i `Ctrl` e i tasti del mouse | «con uno dei due `Ctrl` tenuto» |
| **C9** | minore | `PIE-V01-DOCKCLICK`: l'istruzione ancora da eseguire diceva `Alt`, e la correzione era solo in coda | l'istruzione dice `Ctrl`; la coda dichiara che cosa diceva prima |
| **C10** | minore | Il passo 17 (a) non distingueva «non mappato» da «tasto inghiottito dall'Editor» | il controllo positivo sono i passi 3–5 con `Ctrl` nella stessa seduta; se la barra dei menu prende il fuoco, si ripete |

**Non controllato da nessuno dei due revisori**: gli asset binari (una legenda dei tasti in un `WBP` non si vede col grep); come il motore consegna `AltGr`; gli accordi dell'Editor con `Ctrl`.
