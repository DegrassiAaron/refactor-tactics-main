# Roadmap a 5 lane (Editor + Replay) — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **nessuna modifica applicata** al canone · **Data**: 2026-08-11
> **HEAD della revisione**: `6bdeb9a` · branch `feat/hex-layer-focus-view`
> **Sorgente revisionata**: `RefactorTactics_5_Lane_Roadmap_Editor_Replay_Claude.md` (1837 righe, untracked),
> archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-11-five-lane-roadmap-editor-replay.md`](../../archive/src/handoff/2026-08-11-five-lane-roadmap-editor-replay.md)
> **Scopo**: classificare ogni affermazione del documento contro il repository **prima** che qualcuno la
> applichi a ownership, roadmap, milestone, gate, epic o codice.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR, una `D-0xx`,
> un gate già definito o un fatto misurabile sul branch, prevale il repository e la proposta si **registra**.
> **Cosa non è**: un piano di implementazione. Il §8 dice quali due fette sono reali, e si aprono solo se
> l'autore le vuole.

---

## 1. Il verdetto in una riga

Il documento descrive un modello di processo sano — **Editor e Replay sono consumer del core, non progetti
laterali** — e poi lo indirizza a un repository che non esiste: dei **51** path `Source/`/`Content/`/`Scripts/`
che assegna alle cinque lane, **45 non esistono**, tutti e **11** gli identificatori di gate che introduce sono
**già in uso** nella Definition of Done della v0.1 con significato diverso, e la milestone `F4.5` che propone
apre un **quarto** spazio di numerazione accanto a tre che collidono già.

Il documento contiene però il proprio antidoto, ed è la lettura più giusta che se ne possa dare: il §33 chiede
esplicitamente di «verificare i path reali» (punto 3), di «non creare directory speculative se esiste già un
equivalente» (punto 4), di «deduplicare» (16) e di «creare solo gap reali» (17). Questa revisione è
l'esecuzione di quei quattro punti. Il risultato è che ne sopravvive poco — ma ciò che sopravvive è buono.

---

## 2. Il conto

| | Voci | Significato |
|---|---:|---|
| `WRONG` | **4** | l'affermazione è falsificata da un fatto misurabile sul branch |
| `DUPLICATE` | **9** | chiede di costruire qualcosa che ha già codice, test e owner |
| `CONFLICT` | **6** | contraddice un ADR, una `D-0xx` o un gate già definito |
| `CURRENT` | **4** | riporta correttamente una pratica del repository, spesso senza sapere che è già pratica |
| `PROPOSED` | **3** | idea nuova, nessun conflitto: si registra o si costruisce |

**La misura che decide la revisione**, ripetibile:

```bash
grep -oE '(Source|Content|Scripts)/[A-Za-z0-9_./]+(\*\*)?' <sorgente> | sed 's|/\*\*$||' | sort -u \
  | while read p; do [ -e "$p" ] || echo "MANCA: $p"; done
# → 45 mancanti su 51
```

⚠️ **Il criterio meccanico è stato validato sul contenuto**, perché da solo direbbe troppo poco. I **6** path
superstiti non sono un campione casuale: sono `RefactorTactics.Build.cs`, `.cpp`, `.h`, la cartella
`Source/RefactorTacticsEditor/` e il suo `.Build.cs` — cioè esattamente lo **scheletro standard di un modulo
UE**, l'unica parte che si può indovinare senza aprire il repository. Nessuno dei path di **dominio** è
corretto. Il documento non ha sbagliato qualche riga: non ha guardato l'albero.

---

## 3. Il panel

Sei revisori, un focus ciascuno. Le citazioni sono ricostruzioni della metodologia, non attribuzioni reali.

### 📋 WIEGERS — falsificabilità dei gate

> «Il §25 è la parte migliore del documento e la porto avanti io stesso: cinque caselle per gate, e "N/A +
> reason" ammesso. È una checklist di **copertura**, e le checklist di copertura funzionano.
>
> Il problema è un livello sopra. Il §12 scrive come acceptance del gate G3 "1000 repeat executions → 0
> divergence". Da dove viene **1000**? Il repository ha già questo requisito, si chiama `G4`, dice **100
> ripetizioni** ed è ancorato a un test che esiste con nome e cognome — `RefactorTactics.Simulation.Deterministic
> Replay`. Il documento non propone di alzare la soglia: la **riscrive di nascosto**, decuplicandola, dentro un
> gate che ha un nome diverso. Se qualcuno applicasse il documento, la v0.1 acquisterebbe un requisito 10×
> senza che nessuno abbia mai deciso di alzarlo, e senza una riga che spieghi perché 100 non bastava.
>
> Un requisito che cambia un numero già rilasciato deve dire *quale* numero sta cambiando. Questo non lo sa.»

### 🎯 COCKBURN — attore primario e obiettivo

> «Chiedo sempre la stessa cosa: chi è l'attore primario, e qual è il suo obiettivo?
>
> Il §6 risponde senza accorgersene: "Claude #1 → Spatial … Claude #5 → Replay". Cinque worktree, cinque
> branch, cinque sessioni concorrenti. L'attore primario di questo repository è **una persona sola** —
> ADR-0009 lo scrive nell'intestazione, "Decisore: utente (dev singolo)", e la D-083 costruisce
> un'intera decisione su quel fatto: *"in v0.1 quella persona c'è sempre ed è una sola, quindi l'hash
> comprerebbe un'informazione che il contesto già fornisce"*.
>
> Cinque lane parallele non sono cinque sviluppatori. Sono **una persona che fa da Integration Gate a se
> stessa cinque volte per ciclo**, e il gate è la parte che non si può delegare: è dove si legge il diff. Il
> documento ottimizza la produzione e lascia il collo di bottiglia dov'era.
>
> Il modello di ownership resta utile — ma come **disciplina di una sessione alla volta**, non come
> topologia di worktree.»

### 🏗️ FOWLER — dove passa davvero il confine

> «La tesi architetturale del §1 è corretta e la sottoscrivo: il runtime non dipende dall'Editor, il resolver
> non dipende dal Replay. Il documento però la difende con lo strumento sbagliato.
>
> Propone di **assegnare cartelle alle lane** — `Public/Map/**` a A, `Public/Turn/**` a B — e nessuna di quelle
> cartelle esiste. Il modulo runtime non ha lo split `Public/`/`Private/`: è piatto, `Source/RefactorTactics/Map/`,
> `Turn/`, `Replay/`, e i test stanno in `Tests/` con **88 file** e una sola sottocartella, `Golden/`. Il
> content non sta in `Content/RefactorTactics/` ma in `Content/RT/`.
>
> Ma la nota importante è un'altra: il confine che il documento vuole proteggere **è già protetto meglio**.
> `D-078` non lo affida a una convenzione di ownership, lo rende *impossibile dalla struttura* — il Player vive
> dove il resolver non è raggiungibile — e dice a chiare lettere che il test d'architettura è "la rete
> secondaria, non la garanzia: un test si aggira con un `#include`, una dipendenza che non esiste no".
>
> Il documento propone un galateo dove il repository ha già un muro portante.»

### 📚 ADZIC — documentazione viva

> «Il §33 chiede, ai punti 10, 11 e 12, di "aggiornare Feature Map", "aggiornare Scenario Map", "aggiornare
> Editor Map". Apro quei tre file. Tutti e tre iniziano con la stessa parola: `GENERATA`. Due portano un
> avviso esplicito — *"⚠️ Non si modifica a mano. Una modifica fatta qui viene persa alla prossima
> rigenerazione, e `--check` la segnala prima"*.
>
> Non è un dettaglio di forma. È il difetto che questo repository ha **già pagato quattro volte** e che il
> Feature Registry esiste per chiudere: "un numero scritto a mano in due viste diverge, e la seconda copia
> diventa una bugia con la data sbagliata". La `editormap` in particolare **è già morta una volta**, ritirata
> l'8 agosto perché era la terza vista di stato mantenuta a mano, ed è tornata solo alla condizione di essere
> generata.
>
> E poi il §28 — nello stesso documento — dice che la Editor Map deve contenere "soltanto attività realmente
> manuali/editoriali" e che "se un'attività può essere interamente automatizzata, non marcarla come Editor Task
> obbligatorio". È **esattamente** la regola di `editor-sessions.yaml`. Il documento ha ragione sul modello e
> torto sull'operazione: chiede di scrivere a mano la vista che lui stesso descrive come derivata.»

### 🔥 NYGARD — modi di guasto

> «Guardo cosa si rompe, non cosa funziona. Tre cose.
>
> **Uno: gli identificatori di gate.** Il documento introduce `G0`…`G10` più `G9.5`. Il repository usa `G1`…`G15`
> in `v0.1-definition-of-done.md`, e ogni singolo ID è già occupato con un significato diverso. Il caso
> peggiore non è la collisione, è la **quasi-coincidenza**: il `G3` del documento ("F0 Determinism") è il `G4`
> del repository; il `G4` del documento ("F1 Privacy") è il `G8` del repository ("nessun intento avversario
> replicato"); il `G9` del documento ("Vertical Slice") è un `G9` che **esiste già** e chiede altro — il subset
> `RELEASE-V01` delle 17 verifiche manuali. Un ID sbagliato che *non risolve* lo trovi subito. Un ID sbagliato
> che risolve alla cosa **quasi giusta** ti resta addosso per settimane.
>
> **Due: i contatori condivisi.** Il §6 raccomanda cinque sessioni concorrenti. Il Decision Log registra
> l'**ottava** collisione di contatore `D-nnn` — "la prima trovata in review invece che al push" — con meno
> parallelismo di quello proposto. Non è un rischio ipotetico: è un guasto ricorrente misurato, e il documento
> ne moltiplica la frequenza senza nominarlo.
>
> **Tre: il quarto spazio di numerazione.** `F0`…`F6` + `F4.5` si affianca a `M6`–`M11`, alle epic `E1`–`E36`,
> ai checkpoint `CP x.y` e alle **undici** milestone GitHub organizzate per fetta di release — che portano già
> la regola *"i nomi non usano mai `M<n>`: i due spazi di numerazione collidono e GitHub non ha modo di
> disambiguare"*. Il documento aggiunge una quinta coordinata a un problema di disambiguazione già dichiarato.»

### ✅ CRISPIN — cosa è già verde

> «Prima di chiedere un test, si guarda se c'è. Ho guardato.
>
> Il §9 chiede di creare in E0 le cartelle Replay "senza congelare un formato". Il §12 chiede in E3 di
> "congelare il primo formato persistente" con un header. Il §18 chiede in E9 record/load/seek/hash verify.
>
> `Source/RefactorTactics/Replay/` contiene `RTReplayManifest.h`, `RTReplayRecorderLibrary` e
> `RTReplaySeekLibrary`. `ERTReplayManifestVersion::Initial = 1` è **congelato**. I test sono **16** e coprono
> punto per punto ciò che il documento pianifica per tre milestone: `Replay.Manifest.RoundTrip`,
> `.UnknownVersionIsRejected`, `Replay.Recorder.TurnBytesMatchSerializeTurnLog`, `.MatchIdStaysOutOfHashes`,
> `.WallClockStaysOutOfHashes`, `.OutOfSequenceTurnIsRejected`, `.FailedWriteLeavesManifestUntouched`,
> `.UnclosedArchiveIsRecognisable`, `Replay.Recording.AMatchLeavesAnArchive`, `.UnstartedManagerWritesNothing`,
> `Replay.Seek.TurnEqualsLinearScan`, `.PhaseEqualsLinearScan`, `.PhaseStartsAtItsFirstEntry`,
> `.PhaseWithoutEntriesIsNotAPosition`, `.MissingTurnFailsExplicitly`, `.TraceWithoutTurnNumberIsNotAddressable`.
>
> Anche il "replay diff tool" del §19 ha un precedente: `CompareSerializedTraces` esiste in
> `RTTurnLogLibrary` e nomina turno, fase e `ActionId` della prima divergenza.
>
> ⚠️ Con una eccezione onesta, che il documento non poteva sapere ma che va scritta qui perché è l'unico punto
> in cui la sua insistenza ha ragione: `D-078` avverte che **i tre test richiesti da ADR-0009 non esistono
> ancora** — "si scrivono in R3, e finché mancano `REPLAY-04` resta aperto, nessuno li citi come presenti".
> Quello è un gap vero. Non è nessuno dei quindici che il documento propone.»

---

## 4. I quattro difetti strutturali

### 4.1 `WRONG` — l'ownership indirizza un albero che non esiste

45 path su 51. Il modulo runtime **non ha** lo split `Public/`/`Private/`; il content root **non è**
`Content/RefactorTactics/`; i test **non sono** in `Private/Tests/<dominio>/`.

| Il documento dice | Il repository ha |
|---|---|
| `Source/RefactorTactics/Public/Map/**` + `Private/Map/**` | `Source/RefactorTactics/Map/` (piatto) |
| `Source/RefactorTactics/Public/Turn/**` + `Private/Turn/**` | `Source/RefactorTactics/Turn/` |
| `Source/RefactorTactics/Public/Log/**` | *(nessuna)* — il TurnLog vive in `Turn/RTTurnLog.h` |
| `Source/RefactorTactics/Public/Planning/**`, `Reaction/**`, `Query/**`, `Presentation/**` | *(nessuna)* |
| `Source/RefactorTactics/Private/Tests/{Map,Path,Turn,Resolver,Reaction,Replay}/**` | `Source/RefactorTactics/Tests/` — 88 file, unica sottocartella `Golden/` |
| `Content/RefactorTactics/{UI,VFX,Input,Maps/Grid,Replay,Editor}/**` | `Content/RT/**` |
| `Scripts/Editor/**` | *(nessuna)* — il documento stesso la marca condizionale |

**Conseguenza operativa**: il §33 punto 5 ("aggiornare `FILE_OWNERSHIP.md`") non è eseguibile come scritto —
quel file **non esiste** in tutto il repository, l'unica occorrenza della stringa è dentro il documento
stesso. Non è un aggiornamento, è una creazione, e va deciso come tale.

### 4.2 `CONFLICT` — undici identificatori di gate già occupati

| Il documento | Il repository, `v0.1-definition-of-done.md` |
|---|---|
| `G0` Bootstrap | *(libero — è l'unico)* |
| `G1` Cell + Event Visibility | **G1** Build Game + Editor senza warning nuovi |
| `G2` Playable/Reproducible Movement | **G2** Suite automation completa verde |
| `G3` F0 Determinism (**1000** ripetizioni) | **G3** I 10 test nominati dal catalogo esistono |
| `G4` F1 Privacy + Replay | **G4** Determinismo, **100** ripetizioni |
| `G5` Combat + Replay | **G5** Nessun gameplay quadrato residuo |
| `G6` Reaction Audit | **G6** ID stabili e unici |
| `G7` Environment Reproducibility | **G7** Nessun float in costi/priorità/danni |
| `G8` F3 Multilevel | **G8** Nessun intento avversario replicato |
| `G9` F4 Vertical Slice | **G9** Subset `RELEASE-V01`, 17 voci manuali |
| `G9.5` Tooling & Replay | — |
| `G10` F5 Dedicated + Audit | **G10** Partita completa 2v2 multilivello |

Tre coppie sono **quasi** la stessa cosa e questo le rende peggiori di una collisione netta: `doc G3` ≈ `repo G4`
(determinismo, ma 1000 contro 100), `doc G4` ≈ `repo G8` (privacy degli intenti), `doc G9` ≈ `repo G10`
(partita 2v2 completa) mentre `repo G9` è tutt'altro.

### 4.3 `CONFLICT` — `F4.5` è un quarto asse di numerazione

Il repository ha già: milestone **M6–M11**, epic **E1–E36**, checkpoint **CP x.y**, e **undici** milestone
GitHub per fetta di release con la regola scritta di non usare `M<n>` proprio perché due spazi collidono.
`F0`…`F6` + `F4.5` è il quinto. Il §8 del documento riconosce l'attrito («Se il progetto vuole mantenere meno
milestone, F4.5 può essere trattata come sub-milestone») ma sottostima la scala: il problema non è una
milestone in più, è una **coordinata** in più.

### 4.4 `CONFLICT` — tre viste generate da aggiornare a mano

`featuremap.shortlist.md`, `scenariomap.shortlist.md`, `editormap.shortlist.md` portano tutte l'intestazione
`GENERATA` e si riscrivono con `python scripts/feature_registry.py shortlist`. Lo stato vive **solo** in
`feature-registry.yaml`, derivato dai gate. I punti 10–12 del §33 chiedono l'operazione che il repository ha
smesso di fare dopo averla pagata quattro volte.

⚠️ Se una di queste viste va estesa — ed è il caso, vedi §7 — l'estensione si fa nel **modello**
(`feature-registry.yaml` / `editor-sessions.yaml`), non nell'output.

---

## 5. I conflitti puntuali col canone replay

Il dominio replay è il più deciso del repository: `D-062`, `D-063`, `D-067`, `D-077`, `D-078`/[ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md),
`D-080`, `D-083`, `D-084`, più un [conflict report dedicato](replay-system-conflict-report-2026-08-10.md) del
2026-08-10 e la catena di issue `#412`–`#416`. È anche il dominio in cui il documento propone di più.

| § | Proposta | Classificazione |
|---|---|---|
| §12 E3 | Header replay con `ContentManifestHash` + `ResolverConfigHash` + `Build/RulesVersion` | ⛔ `CONFLICT` con **D-083**: si costruiscono alla **v0.2**, con innesco dichiarato («quando un archivio esce dalla macchina che l'ha prodotto»). E `ResolverConfigHash` non è un campo a sé: la config del resolver è **dentro** il perimetro di `ContentManifestHash`, già deciso |
| §12 E3 | «Solo qui congelare il primo formato persistente» | ⛔ `DUPLICATE`: `FRTReplayManifest` esiste, `ERTReplayManifestVersion::Initial = 1` è congelato (**D-077**, `#414`), 16 test |
| §12 E3 | `InitialSnapshotRef`, `TurnCount`, hash per checkpoint nell'header | ⛔ `DUPLICATE`/`CONFLICT`: **D-077** ha già deciso i campi — id, `FormatId`, topologia, hash ordinati per turno, checksum di fine partita, esito, numero di turni, wall-clock. La traccia è **per turno**, non per partita |
| §13 E4 | Tre classi di replay (Server Audit / Team / Public) + canary `CANARY_TEAM_A_PRIVATE` | ⛔ `CONFLICT` di scope: la v0.1 è **2v2 offline contro bot**. `D-078` lo dice per esteso — «nessun avversario, nessun server». È materiale v0.2+, e la privacy degli intenti in v0.1 ha già il suo gate (`G8`) e il suo test |
| §12 G3 | «1000 repeat executions → 0 divergence» | ⛔ `CONFLICT` con `G4`: **100**, ancorate a `Simulation.DeterministicReplay`. ⚠️ E quel test **è già stato segnalato per rinomina** da `D-078`: è un test del *Verifier*, non un replay |
| §21 §30 | «B owns TurnLog production, E owns Replay consumption/storage/audit» | ✅ `CURRENT`, ma `DUPLICATE`: è **D-078**, che lo formula meglio (Player vs Verifier) e lo rende strutturale invece che organizzativo |
| §4 | Nomi file `RTReplayRecorder.*`, `RTReplayReader.*`, `RTReplaySession.*`, `RTReplayCheckpoint.*` | 🟡 metà `DUPLICATE`: esistono `RTReplayRecorderLibrary` e `RTReplaySeekLibrary`. Il documento avverte già «nomi esatti da adattare» |
| §29 | Classificazione dati (`ServerOnly`/`TeamOnly`/`OwnerOnly`/`Public`) e default prudente | 🟢 `PROPOSED`, buono, **prematuro**: nessun consumatore in v0.1. Da registrare per la v0.2 |

---

## 6. Cosa era già fatto (`DUPLICATE`)

Nove voci del documento chiedono di costruire ciò che ha già codice, test e owner.

| § | Chiede | Esiste |
|---|---|---|
| §9 D0 | Creare il modulo `RefactorTacticsEditor` in I0 | `Source/RefactorTacticsEditor/` con `RTHexEditorMode`, toolkit, commands |
| §10 D1 | Debug inspector di cella (CellId, layer, blocked, occupant) | Editor mode H5/H5c: `RTHexSelectTool`, `RTHexPaintTool`, `RTHexFillTool`, `RTHexArchTool` + overlay |
| §11 D2 | Scenario Authoring MVP | `ScenarioHarness/` (`RTScenarioIndex`, `RTScenarioLoader`, `RTScenarioRunner`, `RTScenarioSession`) + `Scenarios/Spec/` con **13** categorie |
| §12 D3 | Golden scenario tools, PASS/FAIL, expected vs actual hash | `Tests/Golden/`, `RTGoldenCorpusTests.cpp`, `RTTestReportWriter`, `CompareSerializedTraces` |
| §18 D9 | Scenario Browser / catalogo | `scenario-index-e-tag.md` + selettore in `BP_GameMode` ([handoff archiviato](../../archive/src/handoff/scenario-browser-bp-gamemode.md)) |
| §9 E0 | Cartelle e contratti Replay | `Source/RefactorTactics/Replay/` — manifest, recorder, seek |
| §10 E1 | TurnLog Replay Smoke, ingest in-memory, ordine preservato | `Replay.Recorder.TurnBytesMatchSerializeTurnLog`, `Replay.Recording.AMatchLeavesAnArchive` |
| §18 E9 | Record / load / seek / hash verify | 16 test `Replay.*`; il seek è `#415` |
| §31 | Validator runtime e Editor con reason code condivisi | pratica corrente: i tool editor chiamano le library runtime |

---

## 7. Cosa sopravvive

Quattro cose. Sono la parte del documento che vale la sessione.

**A. Il livello `DoD Replay` come gate di feature** (§23) — `PROPOSED`, reale, piccolo.
Il documento propone di dividere la Definition of Done in **Core / Authoring / Replay** e di rendere il terzo
obbligatorio «quando la feature produce stato o eventi osservabili dopo la resolution», con esempi calibrati
bene: *A\* interno → nessun evento nuovo; `MoveBlocked` → deve essere replayable; nuova reazione →
opportunity/response/outcome replayable*.

Il repository ha già i gate derivati per feature in `feature-registry.yaml`, ma **non ha un gate
"replay-representable"**. È l'aggiunta più utile del documento e costa un campo nel modello, non una milestone.
⚠️ Va fatta nel `.yaml`, non nelle viste generate (§4.4).

**B. La checklist di gate a cinque caselle** (§25) — `PROPOSED`, cheap.
`A dati validi? · B deterministico e loggato? · C leggibile? · D configurabile senza hack? · E riproducibile
senza divergenza o leak?`, con `N/A + reason` ammesso e la nota finale — *«non inventare lavoro inutile solo
per riempire tutte le lane»* — che è ciò che la rende usabile. Si innesta sui gate **esistenti** `G1`–`G15`
come dimensione di copertura, **senza** introdurre nuovi ID (§4.2).

**C. La classificazione dei dati replay** (§29) — `PROPOSED` per la v0.2.
Prematura in v0.1 (offline vs bot), ma il default che propone — *«non pubblicare automaticamente intenti
segreti storici»* — è la posizione giusta e va **registrata prima** che esista un consumatore, non dopo.
Naturale in `OPEN_DECISIONS.md`.

**D. La conferma del modello Editor Map** (§28) — `CURRENT`.
Non è lavoro: è una convergenza indipendente sulla regola di `editor-sessions.yaml` («solo attività realmente
manuali»). Vale citarla come validazione esterna di una scelta già presa.

---

## 8. Cosa è stato fatto — 2026-08-12

La revisione **non** ha toccato il canone. Le tre proposte sopravvissute sono state applicate il giorno dopo,
su decisione dell'autore, e questa sezione è il registro di cosa è entrato.

**1. Il gate `replay_representable` è nel Feature Registry** (§7A). Aggiunto a `GATE_NAMES` e al gradino
`DONE` di `derive_status`, accanto a `packaged` e `network_privacy`. Assegnato a tutte le **91** feature:
**14 `done` · 32 `todo` · 45 `na`**. Il criterio non è a intuito — l'oracolo è `ERTLogCategory` in
`RTTurnLog.h` (`Move`, `Combat`, `Fallback`, `Reaction`, `Environment`, `Facing`, `Predictive`): se una
feature non produce voci in nessuna di quelle sette, il gate è `na`.

⚠️ **Sta nel gradino `DONE` per una ragione misurata**: al momento dell'ingresso una sola feature era `DONE`
(`RT-FEAT-TOOL-VALIDATION`, per cui la risposta è `na` senza ambiguità), quindi **nessuno stato è regredito**.
Un gate nuovo che retrocede metà registry al primo giro non viene creduto, viene aggirato.

✅ **Il gate ha guadagnato il suo posto, ma non dove questo referto aveva scritto.** La prima assegnazione
metteva `RT-FEAT-MAP-HEXGRAPH` a `todo` citando la nota di `D-067` — «`GraphRevision` non ha un produttore».
⚠️ **Quella nota è vera al 2026-08-10 e falsa oggi**: il produttore è atterrato con `908b84b`, e
`ARTTurnManager::AppendLogEntry` valorizza il campo su **ogni** voce, essendo l'unico `TurnLog.Add` di
produzione del progetto; `RefactorTactics.TurnLog.GraphRevisionRisesWithinTheTurn` lo pinna sul percorso
reale. Quattro feature di mappa — grafo, copertura dinamica, porte, ponti — sono state corrette a `done` dopo
aver verificato **uno per uno** che i rispettivi `ERTEnvironmentOutcome` abbiano un produttore fuori dai test.

Il caso vero è un altro, ed è un ritrovamento: il danno da **`Status.Burning`** nella Cleanup toglie HP e può
**uccidere**, ma passa da `AddLogEvent` — `UE_LOG` più un buffer circolare troncato — e **non** da
`AppendLogEntry`. Nessuna voce canonica: chi riproduce vede un'unità perdere HP o sparire senza un evento che
lo spieghi, e il codice lo ammette (*«l'eliminazione da hazard non ha un beat di playback … la nasconde il
catch-all di `ConcludeTurn`»*). È la differenza fra `log_debug` (l'esito è **osservabile**) e
`replay_representable` (la voce **basta a ricostruirlo**), ed è la regola 7 in
[`../feature-registry.md`](../feature-registry.md) §4.

**2. `REP-1` è in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)** (§7C): chi può leggere una traccia
archiviata. Registrata come domanda **senza consumatore** — la v0.1 è offline — con il default prudente in
vigore fino alla decisione e tre inneschi osservabili. ⚠️ ID `REP-`, non `REPLAY-`: `REPLAY-01`…`REPLAY-09`
sono i **rischi** del §32 del kit di consolidamento, e il prefisso era già occupato.

**3. La checklist a cinque caselle è in [`../feature-registry.md`](../feature-registry.md) §4.2** (§7B), e
qui il referto **si è corretto**: aveva proposto di innestarla su `G1`–`G15`, ma quelli sono criteri di
release già falsificabili, e il posto giusto è dove i gate vivono. Mappata sui gate esistenti, la checklist
ha rivelato che **la casella scoperta non è il replay** — su cui il sorgente insisteva per tre milestone — ma
l'**authoring**: nessun gate chiede se una feature sia configurabile e mal-configurabile in modo rilevabile.
Registrato come constatazione; **nessun gate `authorable` è stato aperto**, perché prima va deciso se la
domanda è per-feature o per-tool.

**4. `FILE_OWNERSHIP.md` non si crea** — deciso dall'autore il 2026-08-12. La domanda che il §4.1 lasciava
aperta è chiusa: il file non esiste e non deve esistere, quindi il punto 5 del §33 del sorgente decade
insieme all'intero impianto di ownership per lane. ⚠️ **Non è stata aperta una `D-0xx`**: il Decision Log
registra decisioni di prodotto e architettura, e «non adottiamo l'artefatto di un handoff non autorevole» è
già registrato qui e nel banner d'archivio. Se serve un riferimento citabile altrove, il numero si assegna
al merge.

**Non fatto, e non da fare** salvo decisione esplicita: i cinque worktree (§3 COCKBURN, §3 NYGARD), la
milestone `F4.5` e l'asse `F0`–`F6` (§4.3), i gate `G0`–`G10` (§4.2), l'header replay del §12 E3 (§5), il
modello privacy del §13 E4 (§5).

---

## 9. Provenienza

Sorgente archiviato in
[`../../archive/src/handoff/2026-08-11-five-lane-roadmap-editor-replay.md`](../../archive/src/handoff/2026-08-11-five-lane-roadmap-editor-replay.md)
con il banner d'esito in testa. Questo referto è la colonna «Recepito da» di quella riga: non c'è un owner
documentale, perché non c'è nulla da possedere.

**Un secondo sorgente della stessa data** — `RefactorTactics_BattleSimulation_UnifiedScenarioHarness_Bot
ReleaseRoadmap_Claude_2026-08-11.md` — tocca lo Scenario Harness, che è perimetro adiacente a questo (§6,
D2/D3/D9). Alla prima stesura di questa sezione era ancora untracked e non revisionato; **era già stato
consumato su `main`** lo stesso giorno — archiviato come
`docs/archive/src/handoff/2026-08-11-battle-simulation-harness-unificato-e-release-bot.md`, con referto in
`docs/roadmap/plans/bot-ai-consolidamento-2026-08-11.md` §9–§10. I due path sono citati **senza link**:
questo branch è 130 commit dietro `origin/main` e non li contiene ancora.

⚠️ **Una terza revisione dello stesso sorgente, nata in parallelo su questo branch, è stata rimossa**
(commit `3bafe28`): scritta su uno stato del tree vecchio di 130 commit, era un duplicato e in tre punti
falsa — il bot pianifica già su `TeamKnowledge`, `epic: null` su `RT-FEAT-BOT-TACTICAL` è deliberato, e il
seed corpus del §16 poggia su un RNG che non esiste. Prima di applicare qualcosa che riguardi l'harness
vale il referto su `main`.
