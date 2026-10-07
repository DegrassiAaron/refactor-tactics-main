# Vedere un'abilità in azione — il banco Ability Lab → PIE

> **Statuto**: design **accettato in sessione** il 2026-10-07, **non implementato**. Nessuna issue
> ancora aperta: questa spec è il testo da cui nascono. È il **primo di quattro sotto-progetti** di una
> stessa richiesta d'autore, elencati in §0; gli altri tre avranno ciascuno la propria spec quando
> arriverà il loro turno.
>
> **Stato misurato**: 2026-10-07, `main` = `8286276e1`. Ogni riga `file:riga` qui sotto è stata letta su
> quel commit; chi la rilegge più tardi la **rimisura**. Nessun totale volatile in questo documento: dove
> serve una misura c'è il comando che la produce.

---

## 0. La richiesta, le decisioni prese e la decomposizione

### 0.1 La richiesta

> *«voglio fare/associare le animazioni e i fx alle skill dei personaggi, poterle vedere in azione»*

Due metà: un **legame** fra ogni abilità dei quattro eroi e la sua presentazione, e un **banco** su cui
provarle una per una senza giocare una partita intera.

### 0.2 Le quattro decisioni d'autore (2026-10-07)

| # | Domanda | Decisione | Perché |
|---|---|---|---|
| D1 | Perimetro FX | **Dentro la v0.1**: clip per abilità + *Basic Combat Cues* con primitive semplici. **Niente Niagara.** | [D-124](../../decisions/RT_PDR_00_Decision_Log.md) tiene *«Niagara dedicato a ogni abilità»* fuori dalla v0.1; il plugin non è nel `.uproject`; `Content/` non ha un asset Niagara. La grammatica dei cue è di [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454). |
| D2 | Banco | **Ability Lab → PIE**: un pulsante che lancia la fixture dell'abilità nel playback vero. | L'Ability Lab (#2599) costruisce già la fixture per ogni abilità canonica ma la esegue solo in un mondo transitorio senza grafica (`SRTLabPanel.cpp:149-175`). Il percorso scenario → `rt.Test.Scenario` → TurnManager → playback esiste già. |
| D3 | Granularità | **Per abilità**: ogni `(eroe, ActionId)` può avere la propria clip attiva, con il ruolo come ripiego. | Oggi la clip si risolve per `(eroe, ruolo)` e l'`ActionId`, pur presente nell'evento, viene scartato al punto di chiamata (`Turn/RTTurnManager.cpp:8462`). |
| D4 | Abilità senza evento | **Nuovo evento `AbilityActivated`**, uno per intento, in coda all'enum. | Un'abilità che non colpisce, non applica stati e non muove nessuno oggi non produce **nessun** evento risolto, quindi non ha un istante in cui suonare. È la forma di #2505: il produttore c'è, manca il momento. |

### 0.3 La decomposizione, in ordine di consegna

Approccio scelto: **estendere l'asse ANIM CORE** (`Action → PresentationRole → variante`, #2445–#2449)
e tenere il profilo dei cue in una **tabella C++ dichiarativa** accanto a `URTPresentationBindingLibrary`.
È l'asse che [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) nomina in
chiaro: *«se durante l'implementazione il legame giusto passa per l'altro asse, si estende quello e non se
ne crea un terzo»*.

Scartati: un Data Asset di presentazione per abilità — D-278 ha scelto tabella C++ e non Data Asset per
la v0.1, e il panel delle animazioni
([`animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md`](../../roadmap/plans/animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md))
ha respinto i «profili di presentation» come asset; e tutto in tabella C++, percorsi delle clip inclusi —
ogni cambio di clip ricompilerebbe, cioè il costo che il commandlet `RTBuildAnimBindings` è nato per
togliere.

| Ordine | Sotto-progetto | Scope in una riga | Dipende da |
|---|---|---|---|
| **1** | **Il banco** (questa spec) | Ability Lab → PIE: fixture salvata in `Saved/`, seconda radice dell'indice, avvio PIE dal pannello, ripristino CVar. | nessuno |
| 2 | Il momento | `ERTResolvedEventType::AbilityActivated` emesso una volta per intento in Prep, Dash e Blast; voce D-278 con cue di attivazione; beat nel playback. TurnLog e `StateHash` invariati. | nessuno |
| 3 | La clip per abilità | Chiave `ActionId` opzionale in `FRTAnimBinding`, nel CDO (`PerAction` accanto a `PerRole`) e nel commandlet; `PlayPresentationRole` riceve l'`ActionId`; l'Anim Browser guadagna l'asse «Abilità». | 2 per le abilità senza colpo; #2554 per giudicare le clip |
| 4 | Il profilo FX per abilità | Tabella `ActionId → profilo di cue` con default derivati da `Shape` e override per abilità, consumata dalle Basic Combat Cues. | #2454, aperta e owner della grammatica |

Il banco viene **primo** perché non dipende da niente e rende visibile il *prima*: finché 2, 3 e 4 non
atterrano, un attacco suona la clip di ruolo e le abilità senza colpo non hanno beat. Il banco serve a
vedere entrambe le cose.

### 0.4 Owner e confini

| Owner | Relazione |
|---|---|
| E21 [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) | epic della presentazione: le issue dei quattro sotto-progetti nascono qui |
| [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) · [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454) | grammatica dei cue di combattimento: il sotto-progetto 4 la **consuma**, non la ridefinisce |
| [D-278](../../decisions/RT_PDR_00_Decision_Log.md) · #1801 | contratto evento → presentazione e gate di esaustività: il sotto-progetto 2 **estende** la tabella |
| ANIM CORE #2445–#2449 · [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) | catalogo, binding, browser: il sotto-progetto 3 estende l'asse; l'anteprima rotta è una dipendenza di quel lavoro, non di questo |
| [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | coordinata di playback: si legge, non si ricostruisce |
| [#2599](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2599) | Ability Lab: questa spec ne estende il pannello |

---

## 1. Il problema che questa spec risolve

Per guardare un'abilità oggi servono: aprire `L_DevSandbox`, scrivere a mano uno scenario JSON che la
dichiari, aprire la console, digitare `rt.Test.Scenario <Id>`, premere Play, e poi **ricordarsi di
azzerare la CVar**, che vive quanto il processo e dirotta in silenzio ogni Play successivo.

Tre pezzi di questo giro esistono già e non si parlano:

- `URTAbilityLabLibrary::BuildFixture` (`Ability/RTAbilityLab.h`) costruisce uno scenario valido per
  qualunque abilità canonica, con Id `AbilityLab.<AbilityId>` e tag `ability-lab`
  (`Ability/RTAbilityLab.cpp:251-254`). Fail-closed: un'abilità non canonica non produce una fixture a metà;
- `URTScenarioLoader::SaveToFile` (`ScenarioHarness/RTScenarioLoader.h:171`) valida e scrive uno
  scenario; `URTScenarioIndex::Scan` indicizza ricorsivamente **una sola radice**,
  `FPaths::ProjectDir()/Scenarios` (`RTScenarioIndex.cpp:124-129`, `RTScenarioLoader.cpp:709-712`);
- `ARTGameMode::ResolveScenarioToRun` (`RTGameMode.cpp`) legge quattro sorgenti — seduta PIE, console,
  riga di comando, property — e lo Scenario Harness esegue lo scenario **nel TurnManager vero, con
  playback visivo** (`ScenarioHarness/RTScenarioSession.cpp`).

Il Lab, però, esegue la fixture su un `UWorld` transitorio e mostra parametri e TurnLog in testo
(`SRTLabPanel.cpp:149-175`): nessuna resa. E l'indice non vede `Saved/`, quindi una fixture salvata lì
non è lanciabile per Id.

---

## 2. Flusso

Nel pannello Ability Lab, accanto a «Esegui», compare **«Esegui in PIE»**. Al clic:

1. **Costruzione e salvataggio.** Il modello costruisce la fixture con la stessa `BuildFixture` di oggi e
   la salva in `Saved/RTLab/Scenarios/<ScenarioId>.json` tramite `URTScenarioLoader::SaveToFile`, che
   valida prima di toccare il disco. Una fixture invalida non produce un file. `Saved/` è in `.gitignore`
   (riga `Saved/`): niente entra in git.
2. **Indicizzazione.** `URTScenarioIndex::Scan` legge una **seconda radice**, `Saved/RTLab/Scenarios/`.
   `rt.Test.Scenario AbilityLab.Hero.Aevik.ArcPulse` risolve quindi come qualunque altro Id, e
   `rt.Test.List` lo elenca con il suo tag `ability-lab`.
3. **Avvio.** Il pannello, lato Editor, apre `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox` se non è la
   mappa corrente (`FEditorFileUtils::LoadMap`), **cattura** i valori correnti di `rt.Test.Scenario` e
   `rt.Debug.PlaybackControls`, li imposta sull'Id e su `1`, poi chiede PIE con
   `GEditor->RequestPlaySession`. Da lì in avanti il percorso è quello esistente: il GameMode risolve lo
   scenario dalla console e lo Scenario Harness lo gioca nel TurnManager con il playback.
4. **Ripristino.** Su `FEditorDelegates::EndPIE` il pannello riapplica i valori catturati al passo 3 — non
   un `""` cieco: ripristina ciò che c'era. Senza questo passo il Play successivo rilancerebbe lo scenario
   del Lab in silenzio.

Il pannello mostra l'Id lanciato e la riga di log con cui confermare che è partito il banco giusto:

```
LogRT: Warning: [RT-Test] AUTO-RUN AbilityLab.<AbilityId> (da: console rt.Test.Scenario)
```

Le tre API dell'Engine che il passo 3 usa esistono in UE 5.8.1, lette negli header il 2026-10-07:
`UEditorEngine::RequestPlaySession` (`Editor/UnrealEd/Classes/Editor/EditorEngine.h:1817`),
`FEditorFileUtils::LoadMap` (`Editor/UnrealEd/Public/FileHelpers.h:280`), `FEditorDelegates::EndPIE`
(`Editor/UnrealEd/Public/Editor.h:284`).

---

## 3. Componenti e file

| File | Cosa cambia |
|---|---|
| `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.{h,cpp}` | `ScenariosRoot()` resta. Si aggiunge `LabScenariosRoot()` → `FPaths::ProjectSavedDir()/RTLab/Scenarios`. |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.cpp` | `Scan` legge entrambe le radici. Una cartella assente non è un problema né una voce. Un Id presente in entrambe è un **duplicato**, trattato come lo sono già i duplicati dentro `Scenarios/`. |
| `Source/RefactorTacticsEditor/Private/RTLabViewModel.{h,cpp}` | `bool PrepareForPie(FString& OutScenarioId, FString& OutError)`: costruisce, valida, salva. **Pura e headless**: non sa nulla di PIE né di `GEditor`. |
| `Source/RefactorTacticsEditor/Private/RTLabPieLauncher.{h,cpp}` (nuovo) | La sola parte che tocca `GEditor`: carica la mappa, cattura e imposta le CVar, chiede PIE, si aggancia a `EndPIE` per il ripristino, si sgancia dopo il primo scatto. Isolata perché nessun automation test la vede. |
| `Source/RefactorTacticsEditor/Private/SRTLabPanel.{h,cpp}` | Il pulsante «Esegui in PIE» e la riga di stato (Id lanciato, riga di log da cercare, oppure l'errore). |
| `Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp` | I test della seconda radice (§5). |
| `Source/RefactorTacticsEditor/Private/Tests/` | I test di `PrepareForPie` (§5). |

⛔ **Nessun `.uasset`.** Nessuna modifica al TurnManager, al resolver, al formato scenario o al GameMode.

La separazione modello / lanciatore ricalca quella che il pannello ha già: `FRTLabViewModel` è
verificabile headless e `SRTLabPanel` no, e il motivo è scritto in testa a
`RTDevSandboxLauncherTests.cpp` — Slate su un editor vivo non lo vede nessun automation test.

---

## 4. Errori e degrado

| Caso | Comportamento |
|---|---|
| Fixture invalida, abilità non canonica | `PrepareForPie` ritorna `false` con il motivo di `BuildFixture` o di `SaveToFile`; **nulla viene scritto**; il pannello mostra la frase. Stesso fail-closed del Lab di oggi. |
| PIE già in corso (`GEditor->PlayWorld != nullptr`) | Il pulsante rifiuta con «PIE in corso». Non chiede una seconda sessione. |
| Caricamento mappa rifiutato (livello sporco, salvataggio annullato) | Il lanciatore si ferma **prima** di toccare le CVar. Nessuno stato resta a metà. |
| Scenario non risolto dal GameMode | Il log lo dice già con `[RT-Test]`. Il pannello non lo intercetta: il verdetto sul lancio resta di chi guarda, come per ogni scenario. |
| Motore occupato da un'altra sessione | Il pannello non può saperlo. Vale `CLAUDE.md` §10, a carico di chi preme il pulsante. |
| PIE termina per errore | `EndPIE` scatta comunque: il ripristino avviene. |
| Pannello chiuso durante PIE | Il delegate resta agganciato finché non scatta una volta, poi si sgancia. Il ripristino non dipende dalla vita del widget. |

---

## 5. Verifica

### 5.1 Automation, headless

| Test | Asserisce |
|---|---|
| `RefactorTactics.Scenario.Index.ScansLabRoot` | Un file scritto nella seconda radice viene indicizzato con Id, percorso e tag; rimosso il file, l'Id non risolve più. |
| `RefactorTactics.Scenario.Index.LabRootAbsentIsNotAnError` | Cartella `Saved/RTLab/Scenarios` mancante: zero problemi, zero voci da quella radice, le voci di `Scenarios/` intatte. |
| `RefactorTactics.AbilityLab.PrepareForPieWritesAResolvableScenario` | Per un'abilità canonica il file esiste, `URTScenarioIndex::ResolvePath` lo trova e `LoadFromFile` lo rilegge **uguale** alla fixture in memoria. |
| `RefactorTactics.AbilityLab.PrepareForPieRefusesAndWritesNothing` | Abilità non canonica → `false`, `OutError` non vuota, **nessun file**. |

🔴 **Controllo di mutazione dichiarato**: togliere la seconda radice da `Scan` deve far diventare rosso
`ScansLabRoot`. Un test che resta verde con la mutazione non prova niente.

I test scrivono in una sotto-cartella temporanea e la rimuovono: `Saved/` è condiviso con l'Editor e
non è un luogo di prova.

### 5.2 Non misurabile headless, e dichiarato

Il pulsante, il caricamento della mappa, l'avvio PIE e il ripristino delle CVar: automation `N/A`.
Si verificano in una **seduta PIE** registrata nel registro del progetto
([`test-manuali-pie.md`](../../technical/test-manuali-pie.md) e
[`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml)), con criterio binario:

1. dopo il clic, il log porta `AUTO-RUN AbilityLab.<Id>` con `da: console rt.Test.Scenario`;
2. il playback mostra l'abilità scelta sulle due unità della fixture;
3. terminato PIE, un secondo Play **senza** passare dal Lab avvia una partita normale.

Il terzo punto è il controllo del ripristino: senza di esso il banco funzionerebbe e lascerebbe il
processo dirottato.

---

## 6. Limiti dichiarati e non-goal

- La fixture resta quella del Lab: due unità, celle fisse, un turno. Non è uno scenario d'autore e non
  sostituisce i `Scenarios/Visual/`.
- Il banco mostra lo **stato attuale** della presentazione. Finché i sotto-progetti 2, 3 e 4 non
  atterrano, un attacco suona la clip di ruolo e le abilità senza colpo non hanno beat. È voluto.
- Un file in `Saved/RTLab/` sopravvive al processo. Il Lab lo sovrascrive al lancio successivo con lo
  stesso Id; non fa pulizia.
- `rt.Debug.PlaybackStartPaused` **non** viene acceso dal banco. Chi vuole il playback fermo lo imposta
  dalla console come oggi (`FOLLOW-UP CANDIDATES`).
- L'Anim Browser (#2554) e il Gray Kit Playground (#1990) non sono toccati.
- Nessun dato di presentazione entra in `MapState`, snapshot, TurnLog o `StateHash`: questa spec non
  tocca nemmeno quel confine.

---

## 7. Follow-up candidates

- Un interruttore «parte fermo» nel pannello che accenda anche `rt.Debug.PlaybackStartPaused`.
- Pulizia di `Saved/RTLab/Scenarios/` da un pulsante o all'avvio dell'Editor.
- Il pannello che legge il `result.json` della run a fine PIE e lo mostra accanto al TurnLog testuale.
- Le spec dei sotto-progetti 2, 3 e 4, nell'ordine di §0.3.
