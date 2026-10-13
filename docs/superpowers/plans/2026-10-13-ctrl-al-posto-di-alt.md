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
| Nessun altro codice legge `EKeys::LeftAlt`, `RightAlt`, `IsAltDown`; nessun codice legge `LeftControl`, `RightControl`, `IsControlDown` | `grep -rnE "EKeys::(Left\|Right)(Alt\|Control)\|IsAltDown\|IsControlDown" Source/RefactorTactics`: le due righe del controller e una riga di test |
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

1. **`AltGr`.** Su una tastiera italiana `AltGr` arriva a Windows come `Ctrl` sinistro più `Alt` destro. Dopo il cambio armerebbe ancora il modificatore, passando da `LeftControl`. Nessun test lo può vedere: è del sistema operativo. ⚠️ Non verificato; va in `PIE-PC-GYM` come passo.
2. **`Ctrl` tenuto più un tasto di gioco, dentro l'Editor.** `Tab` è la vista strategica, e `Ctrl`+`Tab` nell'Editor apre il selettore delle schede. In un gioco impacchettato non esiste. ⚠️ Non verificato; va nello stesso passo PIE.
3. **Un `Alt` rimasto mappato per un'altra azione.** Il test nuovo lo vieta per ogni azione del contesto, non solo per il modificatore.
4. **Un solo `Ctrl`.** La mutazione del Task 1 toglie il destro e deve dare rosso.
5. **Un rename a metà.** Dopo il Task 2 `grep` non deve trovare `AltPrimary`, `AltSecondary`, `AltDrag` in `Source/`.

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
 * Tre cose, e la terza e' quella che D-514 chiede con le parole «`Alt` non e' piu' mappato»:
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

- [ ] **Step 5: mutazione**

Commentare il `MapKey` di `RightControl`, build, rilanciare la famiglia (log `3749-mutazione.log`).

Expected: rosso con «sta sul Ctrl destro» e «non ha altri tasti oltre ai due Ctrl». Ripristinare con `git checkout -- Source/RefactorTactics/Player/RTPlayerController.cpp` **dopo** aver controllato con `git diff` che il file porti solo la mutazione sopra il commit; build; famiglia verde (log `3749-ripristino.log`). Il binario si considera ripristinato solo dopo questa build verde, non dopo il checkout.

- [ ] **Step 6: commit**

```bash
git add Source/RefactorTactics/Player/RTPlayerController.cpp Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "feat(3749): il modificatore della camera e' Ctrl, sinistro e destro"
```

Il commit del passo 6 precede la mutazione del passo 5 nell'ordine reale: si committa a verde, poi si muta sopra il commit.

---

### Task 2: i nomi e i commenti dicono `Ctrl`

**Files:**
- Modify: `Source/RefactorTactics/Player/RTPlayerController.h`, `Source/RefactorTactics/Player/RTPlayerController.cpp`
- Modify: `Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`

**Interfaces:**
- Produces (rename, nessun comportamento): `bModifierPrimaryDown`, `bModifierSecondaryDown`, `ModifierPrimaryDragDistance`, `SetModifierPrimaryDownForTest(bool, float)`, `GetModifierPrimaryDragDistanceForTest()`, il test `RefactorTactics.Camera.ModifierDragOrbitsOnlyPastTheClickThreshold` (classe `FRTCameraModifierDragOrbitsOnlyPastThresholdTest`).

- [ ] **Step 1: rename degli identificatori**, con uno script che sostituisce le sei parole intere nei tre file e stampa quante sostituzioni ha fatto per parola e per file. Atteso, dalla misura: `AltPrimaryDragDistance` 7 + 3, `bAltPrimaryDown` 6 + 2, `bAltSecondaryDown` 4 + 1, `SetAltPrimaryDownForTest` 1 + 1, `GetAltPrimaryDragDistanceForTest` 1, `FRTCameraAltDragOrbitsOnlyPastThresholdTest` 2, `AltDragOrbitsOnlyPastTheClickThreshold` 1. Un numero diverso ferma il passo.

- [ ] **Step 2: i commenti.** Nei tre file, e nelle righe di `RTPlayerInteractionTests.cpp` già toccate, `` `Alt` `` come nome del modificatore diventa `` `Ctrl` ``; nelle stringhe dei test `Alt` diventa `Ctrl`. A mano, non a tappeto, tre punti:
  - `RTPlayerController.h`, il commento di `CameraModifierAction`: la riga su `Alt`+`Enter` e `bAltEnterTogglesFullscreen` diventa una riga datata — con `Ctrl` il caso non si pone più, e l'opzione del motore resta com'è;
  - i commenti che raccontano una code review datata restano al passato, col tasto di allora nominato come «il modificatore»;
  - **non si toccano**: la variabile locale `Alt` di `RTPlayerInteractionTests.cpp` (un'unità), `Variant.Alt` in `RTHeroCatalogTests.cpp`.

- [ ] **Step 3: la misura del rename**

```bash
grep -rnE "AltPrimary|AltSecondary|AltDrag" Source/RefactorTactics ; echo "rc=$?"
grep -rnE "\bAlt\b" Source/RefactorTactics --include=*.h --include=*.cpp
```

Expected: il primo `rc=1` (niente). Il secondo: solo la variabile `Alt` dei test, `Variant.Alt`, `bAltEnterTogglesFullscreen` nella riga datata, e i `⌫` che dicono «erano i due `Alt`». Ogni altra riga è un difetto del passo 2.

- [ ] **Step 4: build, poi le due famiglie**

Build come sopra; poi `RefactorTactics.Camera` e `RefactorTactics.PlayerInput`, log `3749-rename-camera.log` e `3749-rename-input.log`.

Expected: tutte verdi; `Camera.ModifierDragOrbitsOnlyPastTheClickThreshold` compare, il nome vecchio no. Trovati = verdi + rossi in entrambi i log.

- [ ] **Step 5: commit**

```bash
git commit -am "refactor(3749): i gesti del modificatore non portano piu' Alt nel nome"
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

- [ ] **Step 2: Decision Log.** Rimisurare i numeri (`git fetch`, `git grep` sui ref remoti, `gh pr diff` delle PR aperte). Atteso: D-511 di un'altra sessione, D-512 e D-513 della PR #3752, quindi **D-514**. La riga: titolo; il fatto (`CameraModifierAction` sui due `Alt`, #1771); la domanda che l'autore aveva davanti e la sua risposta *«sostituisci alt con ctrl»*; la lettura `R1` della sessione — tutta la famiglia, non il solo click — dichiarata come tale col suo costo; la tabella dei gesti in prosa; che cosa non cambia (soglia, dolly, `MMB`, `CAM-B`, D-499 punto 3, D-458 punto 3); che cosa non decide (il fuoco, parte due); i due ⚠️ non verificati del Review Focus. In coda alle celle d'impatto di D-458 e D-499 una riga datata che rimanda a D-514.

- [ ] **Step 3: voci PIE.** In `PIE-PC-GYM` i passi 3, 4 e 5 dicono `Ctrl`. Si aggiunge un passo in coda al percorso: `AltGr` e `Ctrl`+`Tab` (Review Focus 1 e 2), con ciò che deve essere vero. La nota sopra la tabella che dice che il pan di precisione «NON esiste» diventa una riga datata: esiste, in `RouteCameraGesture`; il percorso non lo esercita ancora. La frase «Sedici passi» si aggiorna al numero vero, contato sulle righe della tabella. In `PIE-V01-DOCKCLICK` una nota **in coda** alla cella di stato: il dolly è `Ctrl` più destro.

- [ ] **Step 4: `editor-sessions.yaml` e `OPEN_DECISIONS.md`.** Nelle note della seduta di #1859 i gesti dicono `Ctrl`, con la data. In `CAM-B` una riga datata sotto la risposta. Il file YAML è CRLF: si verifica con `yaml.safe_load` che le chiavi di primo livello e il numero di sedute non cambino.

- [ ] **Step 5: i radar**

```bash
for f in $(ls tools/radar/*.ts | grep -v '\.test\.ts$'); do node "$f" --check >/dev/null 2>&1 || echo "ROSSO $f"; done
```

Expected: gli stessi due rossi di `main`, `pie-verdict-age` e `wiki-alt`.

- [ ] **Step 6: la misura dei documenti**

```bash
git grep -nIE '`Alt`|Alt ?\+' -- docs ':!docs/archive' ':!docs/roadmap/plans' ':!docs/superpowers'
```

Expected: restano solo `Alt+P` e `Ctrl+Alt+F11` (scorciatoie dell'Editor), i verbali datati di `P3`, e le righe `⌫` che nominano il tasto vecchio al passato. Ogni riga rimasta si giustifica per nome nel messaggio della PR.

- [ ] **Step 7: commit**

```bash
git commit -am "docs(3749): contratto della camera, D-514 e voci PIE dicono Ctrl"
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

## ➕ rev. I rilievi del revisore avversario — DA ACCOGLIERE PRIMA DI ESEGUIRE

Un agente con contesto fresco, in sola lettura, tetto di trenta chiamate, il 2026-10-13. Nessun rilievo critico.
⚠️ **Il testo dei task qui sopra non li porta ancora**: chi esegue applica prima questa tabella. Dove un rilievo
contraddice un passo, vince il rilievo.

| | Gravità | Rilievo | Correzione che vincola chi esegue |
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
