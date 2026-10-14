# Le clip d'azione nel cook, e la classe autorata sulle unità — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `RefactorTactics.Packaging.RequiredActionClipsAreCooked` torna verde perché ogni clip d'azione attiva ha davvero un riferimento **duro**, e i `BP_Unit_*` dei quattro eroi usano la classe d'animazione autorata `ABP_RTUnitAuthored`.

**Architecture:** due commandlet del modulo Editor, eseguiti dal clone principale, l'unico con i contenuti Paragon. `RTBuildAnimBindings`, che esiste, genera `ABP_RTUnitAuthored`: un `UBlueprint` figlio di `URTUnitAnimInstance` che porta solo dati (`ClipsPerHero`), non un grafo. L'asset diventa versionato. `RTSyncUnitAnimRefs`, nuovo, scrive su ogni `BP_Unit_*` `UnitAnimClass` = la classe autorata, e in `CookDeps_Anim` le clip attive dell'eroe come riferimenti duri. Il gate impara a non contare i riferimenti **morbidi** della classe autorata, e a chiedere le clip della classe che le unità usano. Il runtime impara a ripiegare sulla classe C++ se la classe d'animazione manca.

**Tech Stack:** Unreal Engine 5.8.1, C++ (`RefactorTactics`, `RefactorTacticsEditor`), commandlet headless `UnrealEditor-Cmd -run=…`, Automation, `IAssetRegistry`.

**Issue:** [#3562](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3562). **Branch:** `issue/3562-clip-d-azione-nel-cook`, da `origin/main` = `fb4dbf891`. Codice scritto e compilato nel worktree `D:/Repositories/rt-wt-ispettore`; commandlet, asset e misure **nel clone principale** `D:/Repositories/refactor-tactics-main`, sullo stesso branch.

**Decisioni d'autore (2026-10-14):** chiudere #3562 per intero, gate **e** classe autorata, con l'asset generato e **versionato**; scrivere le clip con un commandlet, non a mano.

## Stato misurato (2026-10-14, `fb4dbf891`)

| Fatto | Dove |
|---|---|
| Il gate è rosso sulle clip d'azione attive di tutti e quattro gli eroi; quello di ruolo è verde. L'elenco è la **Misura 0**: le righe `variante ATTIVA` del log della suite intera | `RTPackagingConfigTests.cpp`, `RTVerificaCotturaClip`; log della suite del 2026-10-14 |
| Il gate non segue i riferimenti: cerca il **package path** della clip nei **byte** di ogni `.uasset`/`.umap` **presente su disco** sotto `Content/RT` (`RTPackageFilesUnder`, `RTBytesMentionPath`). Un path morbido conta quanto uno duro, e un file ignorato da git conta quanto uno versionato | stesso file |
| Il set richiesto viene dal CDO **C++** di `URTUnitAnimInstance`, non dalla classe che le unità usano | `RTVerificaCotturaClip` |
| Ogni `BP_Unit_*` ha una variabile Blueprint `CookDeps_Anim` (nome visibile «Cook Deps Anim») che nomina le clip di **ruolo** dell'eroe. Se il suo elemento sia `ObjectProperty` o `SoftObjectProperty` **non è accertato**: la name table contiene entrambi i nomi. Nessun codice la scrive | stringhe di `BP_Unit_Aevik.uasset`; `grep CookDeps_Anim Source` → niente |
| `UnitAnimClass` nasce `URTUnitAnimInstance::StaticClass()`; le clip si risolvono dal CDO di `UnitAnimClass`. Se `UnitAnimClass` è nulla, `ApplyUnitAnimClass` esce e la risoluzione restituisce `nullptr`: **l'unità non si anima, senza ripiego** | `RTUnit.cpp`: costruttore, `ApplyUnitAnimClass`, `GhostFallbackClipPath`, `ResolvedClipPathFor` |
| Il commento di `UnitAnimClass` dice di assegnarla **da C++ e non nei `BP_Unit_*`**, per il peso: duplicare gli AnimBlueprint Paragon (650–735 KB l'uno) per ricablarne l'ingresso. `ABP_RTUnitAuthored` non è uno di quelli: non ha grafo e porta solo dati | `RTUnit.h`, commento di `UnitAnimClass` |
| D-248 stabilisce che il grafo di locomozione vive in C++ e che i quattro `ABP_*` duplicati da Paragon non si creano; il `.gitignore` lo ripete, e `asset-map.md` conta gli `ABP_*` | `.gitignore`, paragrafo sopra le righe dei `BP_Unit_*`; `docs/technical/tooling/asset-map.md` |
| `RTBuildAnimBindings` genera `/Game/RT/Anim/ABP_RTUnitAuthored`: `UBlueprint` + `UBlueprintGeneratedClass`, padre `URTUnitAnimInstance`, CDO = default C++ con sopra i pool del catalogo. Il corpo di `Main` non è mai stato eseguito: `ABP_RTUnitAuthored` non esiste in nessun clone | `RTBuildAnimBindingsCommandlet.cpp`; `ls` |
| Il catalogo `Data/Anim/AnimCatalog.json` ha voci senza legami | il file |
| `Content/**/*.uasset` è ignorato salvo eccezioni nominate; i `BP_Unit_*` sono fra le eccezioni | `.gitignore` |
| I contenuti Paragon (`Content/FabAsset/`) sono ignorati e presenti solo nel clone principale | `.gitignore`; `ls` |
| La mappa eroe → blueprint è `ARTGameMode::HeroUnitClasses`, riempita nel costruttore con quattro `FClassFinder`; un path che non risolve lascia la voce **assente**, in silenzio | `RTGameMode.cpp` |
| Il censimento dei campi di `ARTUnit` classifica `UnitAnimClass` per osservatore e non ne fissa il valore; `RTUnitTests` controlla il CDO **C++**, che non cambia | `RTUnitBlueprintSurfaceTests.cpp`, `RTUnitTests.cpp` |

## Global Constraints

- **Il clone principale si prende solo con la parola «clone libero»** dalla sessione che lo usa, e si lascia pulito su `main` alla fine.
- **Un asset si salva solo dove ogni sua dipendenza dura si carica**: il commandlet nuovo valida tutto prima di scrivere qualunque cosa.
- **Nessun gesto cambia ciò che suona oggi**: col catalogo senza legami la classe autorata porta il default C++. Lo prova un test, non un argomento.
- **Il motore è uno per macchina**; la suite intera solo in una finestra concordata.
- **Build e test in comandi separati**, exit code letto subito.
- **Nessun merge** senza l'ok esplicito dell'autore.
- **Niente totali volatili** nei testi: i blueprint e le clip si nominano, o si rimanda all'elenco di Misura 0.

## Rulings

| | Ruling | Costo se è sbagliato |
|---|---|---|
| **Q1** | Il gate **esclude** dai package che contano il solo `/Game/RT/Anim/ABP_RTUnitAuthored`: i suoi path sono riferimenti morbidi, e contarli renderebbe il gate verde da solo, per sempre | se un giorno l'asset portasse un riferimento duro voluto, il gate non lo vedrebbe; è scritto accanto all'esclusione |
| **Q2** | Il set richiesto viene dal CDO della classe che le unità usano. Se il file `Content/RT/Anim/ABP_RTUnitAuthored.uasset` c'è, la classe **deve** caricarsi: se non si carica il gate è rosso con un errore proprio. Se il file non c'è, il set è quello del default C++, come oggi | un clone con un asset corrotto vede un rosso di caricamento invece di un verde falso: è il prezzo voluto |
| **Q3** | `CookDeps_Anim` diventa **esattamente** l'insieme delle clip attive dell'eroe, ruolo più azione, ordinato per path; le voci in più si tolgono, e il `-DryRun` le elenca prima | una clip messa lì a mano per un'altra ragione sparirebbe; il `-DryRun` lo mostra |
| **Q4** | L'eroe di un blueprint si legge da `ARTGameMode::HeroUnitClasses`. Il commandlet esce con errore se una delle quattro voci attese manca | la mappa vive nel costruttore del GameMode: un eroe nuovo entra lì e nel controllo |
| **Q5** | `ABP_RTUnitAuthored.uasset` diventa versionato con un'eccezione al `.gitignore` per quel solo file, e la scelta entra nel Decision Log: non contraddice D-248, perché non duplica un AnimBlueprint e il grafo resta in C++ | un binario in più, rigenerato a ogni variante promossa; la dimensione si misura e si scrive nella PR, e si rigenera solo dal clone principale |
| **Q6** | Il runtime ripiega su `URTUnitAnimInstance::StaticClass()` quando `UnitAnimClass` è nulla, in applicazione e in risoluzione | un blueprint con la classe persa si anima col default C++ invece di restare fermo: lo si vede solo dal log, ed è scritto lì |

## Review Focus

1. **Il gate verde per la ragione sbagliata**: Misura A deve essere rossa **con** il log «CDO: classe autorata», e una mutazione che toglie l'esclusione deve renderla verde.
2. **Un riferimento scritto morbido**: la Misura B legge le dipendenze **Hard** dell'Asset Registry, non le stringhe.
3. **Il valore che non sopravvive alla compilazione**: dopo il salvataggio, un secondo `-DryRun` deve dire zero aggiunte e zero tolte.
4. **La trappola del worktree**: ogni dipendenza dura di ogni `BP_Unit_*` deve caricarsi prima che uno solo venga scritto.
5. **Le unità che smettono di animarsi**: la classe autorata si carica, è figlia di `URTUnitAnimInstance`, porta il default C++; i quattro `UnitAnimClass` non sono nulli; e se lo fossero, il ripiego.

---

### Task 0: Preflight

- [ ] Worktree su `issue/3562-clip-d-azione-nel-cook`, pulito. `CommandLine` dei processi `UnrealEditor*`.
- [ ] **Misura 0**: l'elenco delle clip scoperte dal log della suite intera del 2026-10-14, salvato nello scratchpad. È l'elenco atteso in Misura A.

### Task 1: il calcolo delle clip attive di un eroe, nel runtime

**Files:** `Source/RefactorTactics/Unit/RTUnitAnimInstance.h` e `.cpp`; il test nel file dei test del grafo.

**Produces:** `void URTUnitAnimInstance::CollectActiveClips(const FName& HeroId, TArray<FSoftObjectPath>& Out) const` — la variante attiva di ogni pool di ruolo e d'azione dell'eroe, senza doppioni, ordinata per path.

- [ ] **Test che fallisce**: `RefactorTactics.Anim.CollectActiveClipsCoversRolesAndActions`. Non ricalcola l'algoritmo: confronta con **elenchi letterali** presi dal CDO C++ di oggi, per due eroi (`Hero.Aevik` con le tre `LMB_Fire_*_Slow_V1`, e `Hero.Branth`, che ha una clip condivisa da due pool: compare una volta sola), più: nessun doppione e ordine per path su ogni eroe.
- [ ] Build, rosso; implementazione; build, verde. Commit.

### Task 2: il ripiego del runtime

**Files:** `Source/RefactorTactics/Unit/RTUnit.cpp` (e `.h` se serve un helper).

- [ ] **Test che fallisce**: un'unità con `UnitAnimClass` nulla risolve la clip d'attacco di Aevik come la risolve con la classe C++, e `GhostFallbackClipPath` non è nulla.
- [ ] Implementazione: un solo punto, `ResolvedAnimClass()`, che restituisce `UnitAnimClass` o, se è nulla, `URTUnitAnimInstance::StaticClass()` con un log una volta per unità. Lo usano `ApplyUnitAnimClass` e le letture del CDO.
- [ ] Build, verde. Commit.

### Task 3: il gate, che non si fa ingannare e chiede le clip giuste

**Files:** `Source/RefactorTactics/Tests/RTPackagingConfigTests.cpp`.

- [ ] In `RTVerificaCotturaClip`: (a) il CDO secondo `Q2`, col log che dice quale; (b) il package `/Game/RT/Anim/ABP_RTUnitAuthored` escluso dai file che contano, col perché; la variabile che li tiene si chiama per ciò che è, i file **su disco**. Conteggio anti-sottrazione e controllo positivo sulla mesh restano.
- [ ] Un test nuovo, `RefactorTactics.Anim.AuthoredClassCarriesTheCppDefaultsWhileTheCatalogHasNoBindings`: se il file della classe autorata c'è, la classe si carica, è figlia di `URTUnitAnimInstance` e non è una `UAnimBlueprintGeneratedClass`; e finché il catalogo non ha legami, per ogni eroe e ogni pool la clip attiva è quella del CDO C++. Se il file non c'è, il test lo dice e controlla solo il ripiego del gate.
- [ ] Nel worktree, dove la classe autorata non c'è: il gate dà lo stesso rosso di prima, sull'elenco di Misura 0 (il ripiego di `Q2`).
- [ ] Commit.

### Task 4: il commandlet `RTSyncUnitAnimRefs`

**Files:** `Source/RefactorTacticsEditor/Private/Content/RTSyncUnitAnimRefsCommandlet.h` e `.cpp`, sul modello di `RTRemoveEnergyBarCommandlet` per caricare, compilare e salvare un blueprint.

- [ ] **Fase 1, validare tutto, senza scrivere niente.** La classe autorata si carica (altrimenti errore). Le quattro voci attese di `HeroUnitClasses` ci sono. Per ogni blueprint: la proprietà `CookDeps_Anim` esiste sulla classe generata, è un array il cui elemento è `FObjectProperty` (non `FSoftObjectProperty`) con una classe compatibile con `UAnimSequenceBase`; ogni dipendenza **Hard** del suo package, secondo l'Asset Registry, esiste e si carica; ogni clip di `CollectActiveClips` sulla classe autorata si carica. Il confronto col valore attuale va nel log, come aggiunte e tolte. Un solo errore ferma tutto: nessun blueprint viene scritto, codice d'uscita diverso da zero. `-DryRun` si ferma qui.
- [ ] **Fase 2, scrivere.** Per ogni blueprint: `UnitAnimClass` = classe autorata; `CookDeps_Anim` = l'insieme di `Q3`, scritto **anche** come default della variabile del Blueprint se la variabile ne ha uno (altrimenti la compilazione lo rimetterebbe); `CompileBlueprint`; se lo stato è `BS_Error`, niente salvataggio e codice d'uscita diverso da zero; salvataggio.
- [ ] **Fase 3, rileggere.** Dopo il salvataggio, le dipendenze **Hard** di ogni package secondo l'Asset Registry contengono ogni clip scritta e la classe autorata. Altrimenti codice d'uscita diverso da zero.
- [ ] Build nel worktree: compila. Non si esegue qui.
- [ ] Commit.
- [ ] **Revisione avversaria del codice dei Task 1–4**, prima di toccare il clone principale: una correzione dopo non deve obbligare a rigenerare binari già committati. Correzioni in una passata.

### Task 5: nel clone principale — generazione, misure, sincronizzazione

Solo dopo «clone libero». Il clone passa sul branch e si compila lì.

- [ ] `.gitignore`: eccezione `!Content/RT/Anim/ABP_RTUnitAuthored.uasset`, con la nota che dice perché non è uno degli `ABP_*` di D-248.
- [ ] `-run=RTBuildAnimBindings -DryRun`, poi senza. Atteso: asset scritto, nessun legame. La dimensione del file nello scratchpad.
- [ ] `Anim.AuthoredClassCarriesTheCppDefaultsWhileTheCatalogHasNoBindings`: verde.
- [ ] **Misura A**: il gate d'azione, con la classe autorata presente e i blueprint non ancora toccati. Atteso: **rosso, sullo stesso elenco di Misura 0**, e nel log «CDO: classe autorata». **Mutazione**: tolta l'esclusione di `Q1`, il gate deve diventare **verde**; poi si ripristina, si ricompila e la Misura A torna rossa. Se la Misura A è verde senza mutazione, ci si ferma.
- [ ] `-run=RTSyncUnitAnimRefs -DryRun`: per ogni eroe entrano le sue clip d'azione dell'elenco di Misura 0, e non ne esce nessuna. Poi senza `-DryRun`.
- [ ] **Misura B**: `Packaging.RequiredActionClipsAreCooked` e `Packaging.RequiredAnimationClipsAreCooked` verdi; la Fase 3 del commandlet verde; un secondo `-DryRun` dice zero aggiunte e zero tolte.
- [ ] Un test nuovo, `RefactorTactics.Heroes.UnitBlueprintsUseTheAuthoredAnimClass`: per le quattro voci di `HeroUnitClasses`, `UnitAnimClass` non è nulla, è la classe autorata, ed è figlia di `URTUnitAnimInstance`. Verde.
- [ ] PIE: un'unità per eroe in scena, ferma e in movimento. Si anima come prima? La risposta la dà l'autore su una tavola; finché non c'è, `NOT RUN`.
- [ ] Commit dei quattro `BP_Unit_*`, di `ABP_RTUnitAuthored` e del `.gitignore`, dal clone principale.

### Task 6: documenti

- [ ] **Decision Log**: una voce nuova, col numero misurato al momento. Dice che la classe autorata è versionata e assegnata nei `BP_Unit_*`, perché non contraddice D-248 né il commento di `UnitAnimClass`, e che il gate esclude i suoi riferimenti morbidi.
- [ ] Il commento di `UnitAnimClass` in `RTUnit.h`; l'intestazione di `RTBuildAnimBindingsCommandlet.h` sul byte binario; la nota del `.gitignore`.
- [ ] `docs/technical/tooling/asset-map.md`: §6 e il suo comando di misura, che conta gli `ABP_*`.
- [ ] `docs/technical/runbooks/guida-animazioni-paragon.md`: la misura storica sostituita dalla nuova, coi due comandi e l'ordine in cui si lanciano quando una variante diventa attiva.
- [ ] I commenti del gate e dei test che dicono «rosso finché #3562» diventano storia datata.
- [ ] Tutti i radar di `tools/radar/`. Commit.

### Task 7: suite intera, PR

- [ ] Suite intera **nel clone principale**, dove le clip si caricano, in una finestra concordata. Atteso: nessun rosso. Si scrive da quale clone viene l'esito: nei worktree le clip Paragon mancano.
- [ ] PR verso `main` con la tabella dei gate: Compile, Tests, Determinism `N/A`, Replay `N/A`, Privacy `N/A`, PIE, Packaged `NOT RUN` se nessun cook è stato eseguito. DoD di #3562 aggiornata. Il merge è dell'autore. Il clone principale torna su `main`, pulito.

---

## ➕ rev. I rilievi del revisore avversario sul piano

Un agente con contesto fresco, in sola lettura, tetto di trenta chiamate, il 2026-10-14. Tutti accolti nei task qui sopra.

| | Gravità | Rilievo | Dove è accolto |
|---|---|---|---|
| **R1** | critico | La Misura A restava rossa anche se la classe autorata non si caricava e il gate ripiegava sul C++: non distingueva i due casi | `Q2`; Task 3; Task 5, Misura A con log e mutazione |
| **R2** | critico | Il tipo di `CookDeps_Anim` non era accertato, e nessuna misura distingueva un riferimento duro da uno morbido | Task 4, Fasi 1 e 3; Review Focus 2 |
| **R3** | importante | Che il valore di una variabile Blueprint scritto sul CDO sopravviva alla compilazione non era provato | Task 4, Fase 2; Misura B, secondo `-DryRun` |
| **R4** | importante | La guardia sulle sole clip non copriva le altre dipendenze dure dei blueprint (mesh, materiali); scrittura per eroe senza validazione completa; eroe assente in silenzio; stato `BS_Error` | Task 4, Fasi 1 e 2; `Q4` |
| **R5** | importante | Si versionavano binari che puntano a una classe mai eseguita: senza la classe le unità non si animano, senza ripiego | `Q6`; Task 2; Task 3, test della classe autorata; Task 5, test dei blueprint e PIE |
| **R6** | importante | Mancavano la voce del Decision Log e i documenti che il cambio contraddice: commento di `UnitAnimClass`, `.gitignore`, `asset-map.md`, intestazione del commandlet | Task 6 |
| **R7** | importante | Revisione e PIE venivano dopo il commit dei binari | Task 4, ultima casella; Task 5, PIE prima del commit |
| **R8** | minore | Il gate scansiona il disco, non git: l'esito può cambiare da un clone all'altro | Stato misurato; Task 3; Task 7 |
| **R9** | minore | Il costo del binario versionato non era misurato | `Q5`; Task 5 |
| **R10** | minore | Il test del Task 1 ripeteva l'algoritmo | Task 1, elenchi letterali |
| **R11** | minore | Totali volatili nel piano | Global Constraints; testo |
