# CLAUDE.md — RefactorTactics

Overlay operativo per **Claude Code / SuperClaude**.
Le regole condivise del repository sono in **[`AGENTS.md`](AGENTS.md)**: leggilo prima di lavorare.
Questo file resta volutamente corto: dà la **mappa**, i **comandi** e i **pin verificati**, e per ogni regola
rimanda al suo owner. Ridurre le duplicazioni è ciò che tiene bassa la deriva.

> Riallineata al codice il **2026-08-27**, su HEAD `9c08e98`. I fatti strutturali qui sotto sono stati
> **misurati**, non ricordati. I numeri volatili — test, scenari, issue — non stanno qui apposta: si misurano
> sul branch corrente quando servono, mai copiati da un documento.

## 1. Context protocol

Non lavorare dalla memoria del progetto. Verifica branch/HEAD, task, codice, test e documenti owner.

Carica il contesto in questo ordine, **solo quando pertinente**:

1. `AGENTS.md`.
2. Decisioni/invarianti: `docs/product/piano-canonico-mvp.md`.
3. Decisioni recenti: `docs/decisions/RT_PDR_00_Decision_Log.md` + ADR applicabili.
4. Supersessioni/conflitti: `docs/DOC_CONFLICT_MATRIX.md` + `docs/OPEN_DECISIONS.md`.
5. Stato/scope: `docs/roadmap/roadmap-checkpoint.md` + `docs/roadmap/roadmap-v0.1.md`.
6. Feature: specifica owner + cataloghi `docs/balance/` + test + implementazione esistente.

Usa search/grep prima di aprire file lunghi. `docs/research/` è input/north-star non ancora consumato: non
usarlo come autorità implicita. `docs/archive/` è storico, e `docs/archive/src/` conserva i sorgenti già
recepiti — utile per la provenienza, mai per la regola.

> ⚠️ **Era `docs/src/` fino al 2026-08-19, e quella cartella non esiste più** (`git ls-files docs/src` → zero,
> [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165)). Chi seguiva questa riga
> cercava una cartella vuota e non leggeva mai la casella di posta reale.

Un `.pdf` non è **mai** autoritativo (**D-009**): resta fuori dal preflight e si apre solo per provenienza,
rationale storico o confronto richiesto; se è l'export di una spec Markdown corrente, vince il Markdown. La
sigla `PDR` nel nome di un file Markdown non lo rende storico — `docs/decisions/RT_PDR_00_Decision_Log.md` è
canonico perché è l'owner corrente. Regola estesa in [`AGENTS.md`](AGENTS.md).

Quando un catalogo di `docs/balance/` contraddice una spec di `docs/gameplay/` che il codice ha **già
recepito**, vince la spec (**D-210**): è l'unico caso in cui la gerarchia numerica delle fonti si inverte, ed
è stato deciso perché nessuna delle quattro formulazioni di prevalenza del repository lo copriva.

## 2. Mappa del repository

| Percorso | Cosa contiene | Owner / nota |
|---|---|---|
| `Source/RefactorTactics/` | modulo **Runtime**: regole, resolver, mappa, pathfinding, TurnLog, harness, test | mappa per cartella: [`architettura-codice.md`](docs/technical/architecture/architettura-codice.md) |
| `Source/RefactorTacticsEditor/` | modulo **Editor**: strumenti di authoring e i loro test | — |
| `Plugins/RTDeveloperTools/` | plugin **Editor-only**: il ponte MCP di sola lettura (§5) | [`brief-mcp-developer-bridge.md`](docs/technical/tooling/brief-mcp-developer-bridge.md) |
| `Content/RT/` | asset proprietari sotto `/Game/RT/`, struttura feature-first | [`convenzioni-contenuti-ue.md`](docs/technical/tooling/convenzioni-contenuti-ue.md) |
| `Scenarios/` | corpus JSON dello Scenario Harness, alla **radice** e non in `Content/`: un `.uasset` non è diffabile in una PR | [`test-automatico-unreal.md`](docs/technical/tooling/test-automatico-unreal.md) |
| `Config/` | `.ini` di progetto | — |
| `docs/` | canone, decisioni, specifiche, roadmap, cataloghi | indice: [`CONTEXT_INDEX.md`](docs/CONTEXT_INDEX.md) · gerarchia: [`docs/README.md`](docs/README.md) |
| `tools/radar/`, `tools/asset-refs/` | toolchain **Node 22**, zero dipendenze: i controlli vivi (§4) | il docstring in testa a ogni file |
| `scripts/rt-suite.ps1` | l'**unico** file di `scripts/`: la suite che dichiara se la misura vale | **D-222** |

Dentro il modulo runtime, a grana grossa — il dettaglio per file è dell'owner citato sopra, non di qui:

| Cartelle | Responsabilità |
|---|---|
| `Core/` `Map/` `Pathfinding/` | tipi base, `FRTCellId`, asset autorevole della mappa, A\* deterministico sul grafo tattico |
| `Turn/` | fasi, autorità (`ARTTurnManager`), reazioni, TurnLog, privacy degli intenti, formati di partita |
| `Combat/` `Ability/` `Unit/` `Terrain/` `Perception/` | danno e attacchi, azioni data-driven, unità, terreni/stati, vista e udito |
| `Bot/` | l'avversario offline della v0.1 |
| `ScenarioHarness/` `Replay/` | scenario JSON → **stesso** percorso di gioco; replay logico canonico |
| `UI/` `Frontend/` `Camera/` `Player/` `Selection/` `Debug/` | presentazione e input: **non decidono esiti** |
| `Tests/` | Automation Framework: dominio, regressione, corpus golden |

I test stanno **dentro** il modulo runtime (`Source/RefactorTactics/Tests/`), più quelli del modulo Editor e i
sei del plugin MCP. Le regole vivono in `UBlueprintFunctionLibrary` con funzioni statiche pure: si testano
senza mondo e senza Actor, ed è il motivo per cui la suite gira headless.

## 3. Pin rapidi

- UE **5.8.1**; v0.1 **2v2 offline vs bot**; hex multilivello; roster **Gadget/Phase/Riktor/Wraith**
  (**D-120**). I nomi legacy sono **usciti dal repository** (**D-130**): gli `Hero.<Nome>` sono stati
  rinominati e i venti token abilità sono atterrati su `Hero.<Nome>.<Abilità>` **senza redirect** — **D-134**
  ha cancellato `ResolveLegacyActionId`, quindi non esiste una doppia verità da risolvere in lettura.
  Le cinque fette del piano sono chiuse (#753–#757) —
  [`docs/technical/piano-migrazione-roster.md`](docs/technical/piano-migrazione-roster.md).
  ⚠️ **Chi lo controlla oggi, e chi no.** Il gate documentale `scripts/check-docs-naming.py` è uscito con
  **D-182**, quindi un nome ritirato che ricompare **in prosa** non lo segnala nessuno. Gli **ID** invece
  restano coperti da due test C++ che interrogano il **roster reale**, non una lista di ID scritta nel test:
  `Heroes.AbilityIdsAreNamespacedUnderTheirHero` e `Unit.CanonicalHeroIdHasNoLegacyName`, sotto il prefisso
  `RefactorTactics.`. Un ID che contiene un nome ritirato, o un'abilità che non sta sotto il proprio eroe,
  fanno **rosso** — e un quinto eroe col nome sbagliato pure, senza che nessuno aggiorni il test.
  🔴 Le due liste `{ Flux, Riva, Bastion, Vektor }` scritte in quei test **non si rinominano**: sono l'unico
  posto del progetto in cui nominarli è il punto, e un rename massivo le ha già rovesciate due volte.
- Fasi: `Planning → Prep → Dash → Blast → Move → Cleanup`; Move normale resta dopo Blast.
- Movimento in due famiglie (**D-118**), e la linea non è la velocità: **`Traversal` percorre lo spazio**
  (`Move · Dash · Forced`, produce celle attraversate e ogni cella è un fatto), **`Transfer` cambia posizione
  senza percorrerlo** (`Leap` oggi). La riga si verifica in codice, ed è cosa contiene `Result.Entered`.
  `Reaction` è una causa, non una famiglia; `Portal` è topologia del grafo.
- Un solo substrato: `FRTCellId`; no gameplay quadrato parallelo.
- **No GAS nella v0.1**: `URTActionData` / `URTHeroData` / `URTEquipmentData`.
- Azioni generiche (sette, **D-025**): `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`.
- Ownership contenuti (**D-029**): abilità → singolo owner; interazioni → sistemi; sinergie/fazioni/scenari →
  esempi. Niente ability di coppia né branch `if HeroA && HeroB`.
- **Sprint = profilo Move, non Dash**.
- Reazioni: `Opportunity → Commit`; Fast Reaction **3,0 s**, timeout **HOLD**.
- `Hero.Wraith.InterceptShot` = thin slice Predictive v0.1.
- High Ground: nessun bonus numerico alla vista in v0.1.
- Formato competitivo finale non deciso: 3v3 è baseline, 4v4 stress test.

Il dettaglio resta negli owner documentali; non duplicarlo qui.

## 4. Comandi e controlli

**Nessuna CI, per scelta dichiarata**: `.github/workflows/` non esiste (**D-182**, il nucleo regge). Tutto
quanto segue si esegue a mano. Non introdurre CI, package manager o build step senza chiedere — la toolchain
Node ha **zero dipendenze** apposta.

```powershell
# La suite. L'unica via che dichiara se la misura VALE (§7): legge HEAD, albero, binario e processi del
# motore prima e dopo, e confronta `Test Completed` con il `Found N` dichiarato in testa al log.
# Exit: 0 verde · 1 test falliti · 2 non avviata (motore occupato) · 3 NON VALIDA, esito non registrabile.
./scripts/rt-suite.ps1
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario   # una sola area, molto più veloce
```

PowerShell e **non** Git Bash: MSYS traduce gli argomenti che iniziano con `/` e l'harness non parte nemmeno.

```powershell
# Build. L'Editor deve essere CHIUSO, altrimenti il link fallisce con LNK1104. Cerca `Result: Succeeded`.
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -project="<repo>/RefactorTactics.uproject" -waitmutex
```

```sh
# I controlli vivi. Nessuno è imposto da uno script: li esegue chi committa, o non li esegue nessuno.
node tools/radar/generate.ts --check                      # gli SVG dei radar contro i cataloghi di balance
node tools/radar/generate.ts                              # riscrive gli otto SVG: sono output, non si editano
node tools/radar/wiki-alt.ts --wiki-root <clone> --check  # gli alt sulla Wiki, che il primo NON copre
node tools/radar/doc-links.ts --check                     # i percorsi citati da docs/ + AGENTS.md + CLAUDE.md + README.md
node tools/radar/catalog-code.ts                          # stat base degli eroi: catalogo contro C++
node tools/radar/doc-tables.ts --check                    # righe di tabella più strette o più larghe delle sorelle
node tools/asset-refs/check.ts                            # .uasset versionati che citano file assenti da git
cd tools/radar && node --test                             # la suite: da DENTRO la cartella, non dalla radice
```

⚠️ **`tools/asset-refs/check.ts` è il sesto controllo, e l'elenco «cinque» di `AGENTS.md` non lo nomina.** È
lo stesso difetto che `CONTEXT_INDEX.md` §«Le due toolchain» dichiara per i gate: un elenco scritto a mano non
si accorge di un file nuovo. Prima di fidarti di un elenco di strumenti, `ls` la cartella.

⚠️ Ogni gate dichiara nel proprio docstring **cosa non copre** — ancore e URL per `doc-links.ts`, la riga di
tabella staccata dalla sua tabella per `doc-tables.ts`, «cita» invece di «dipende» per `asset-refs`. Leggilo
prima di trattare un verde come una prova.

⚠️ **`tools/radar/` non è solo documentazione**: i rating si calcolano dai cataloghi di bilanciamento
(**D-106**), quindi cambiare una `Salute`, un danno o un cooldown rende rossi gli SVG versionati finché non li
rigeneri **nello stesso commit** (**D-108**).

### Le tre trappole della misura

- 🔴 **Una console variable in testa a `-ExecCmds` fa saltare l'intera coda di automation**: l'editor esegue la
  CVar, all'automation non arriva niente, e il log finisce a metà avvio somigliando a una run riuscita. La via
  d'uscita è **`-dpcvars="nome=valore"`**, che imposta la CVar senza toccare la coda.
- **Nel log servono due righe**: `Found <n> automation tests based on '<filtro>'` in testa e
  `**** TEST COMPLETE. EXIT CODE: <n> ****` in fondo. Se manca la prima, la run non ha misurato niente.
- **L'exit code non è un oracolo**: è misurato che una run sia uscita `0` con un test fallito. Leggi i
  `Result={...}`, non il codice di ritorno. E un numero di test sono **due** numeri: «N eseguiti su M
  dichiarati» — lo script che li confrontava è uscito con **D-181**, oggi il filtro si legge a mano nel log.

## 5. Ponte MCP verso l'Editor

Dal **2026-08-27** esiste un canale di sola lettura fra Claude Code e l'Unreal Editor. Serve a **ispezionare**,
non a decidere: non è un'autorità sul canone e non sostituisce build e test.

- `.mcp.json` dichiara il server `unreal-mcp` su `http://127.0.0.1:8765/mcp` — porta non standard perché la
  8000 di default era occupata sulla macchina di sviluppo.
- Il server è il plugin **ufficiale Epic** `ModelContextProtocol` di UE 5.8; i tool RT vivono in
  [`Plugins/RTDeveloperTools/`](Plugins/RTDeveloperTools/RTDeveloperTools.uplugin), registrati come
  `UToolsetDefinition` del `ToolsetRegistry`. **Zero Python**, nessuna dipendenza nel modulo runtime.
- Cinque tool, tutti **facade** sul codice autorevole: `ProjectStatus`, `GetCurrentMap`, `DumpCell`,
  `FindPath`, `ValidateTacticalMap`. Il percorso lo calcola `URTHexPathLibrary::FindPath`, la cella la
  risponde `URTHexMapAsset`, la validazione `ValidateMap`: nessun A\* riscritto, nessuna scrittura.
- **Se la connessione è rifiutata non è un errore da diagnosticare a lungo**: l'Editor non è aperto, il plugin
  non è abilitato su questa macchina, oppure `bAutoStartServer` è `false`. Quel flag vive in
  `EditorPerProjectUserSettings` — **per utente e non versionato** — quindi si accende una volta per macchina
  e non comparirà mai in un diff.
- Con `bEnableToolSearch = true` (default) `tools/list` espone **solo** i tre meta-tool `list_toolsets`,
  `describe_toolset` e `call_tool`: i cinque tool RT si raggiungono attraverso quelli.

## 6. Classifica il task

**Documentale/analitico** (`/sc:brainstorm`, `research`, `design`, `workflow`, `analyze`, `estimate`, panel,
`troubleshoot` senza `--fix`): produci l'output richiesto e **non passare automaticamente al codice**.

**Esecutivo** (`/sc:implement`, `task`, `improve`, `cleanup`, `test`, `build`, `git`, `troubleshoot --fix`):
verifica prima codice/test esistenti, poi applica il **diff minimo**. Niente refactor opportunistici.

Per implementazioni non banali, preflight breve:

**Obiettivo · Stato verificato · Assunzioni · File · Approccio · Rischi · Test**

## 7. Guardrail Claude

- Non inventare API Unreal: verifica la **5.8.1** e le firme realmente presenti.
- Simulazione/authority in C++; presentazione/configurazione in Blueprint/Data dove appropriato.
- Niente `Delay`, montage, Tick o `DeltaTime` per decidere sequencing competitivo.
- Niente dipendenza dall'ordine di `TMap`/`TSet`.
- Niente branch per eroe nel core quando il comportamento può essere data-driven/componibile.
- Nei test non aggirare il gameplay con `SetActorLocation`, `ApplyDamage` o `if (IsTest)` che salta la regola.
- Non modificare `.uasset`/`.umap` a mano; i passi Editor restano verifiche manuali finché non eseguiti.
  I binari sono **human-first**: li tocchi solo su richiesta esplicita e attraverso Unreal. Due `.uasset`
  non si fondono, quindi un binario si modifica da un lavoro solo per volta.
- Prima di cancellare/rinominare cerca riferimenti C++, config, reflection, soft reference e Blueprint.
- Un handoff/audit non è autorità e non autorizza da solo a implementare tutto ciò che contiene.
- **Sviluppo parallelo, misura protetta** (**D-222**, supera la clausola operativa di **D-178**): piu'
  sessioni condividono davvero questa working directory — misurato, 101 checkout e 6 sessioni in un
  giorno. Non fingere che sia una alla volta. Cio' che va protetto e' la **misura**: una suite vale solo
  se `HEAD`, l'albero, il binario e i processi del motore sono gli stessi all'inizio e alla fine.
  Altrimenti non e' rossa ne' verde: e' **NON VALIDA**, e non si registra. Per questo la suite si lancia
  da [`scripts/rt-suite.ps1`](scripts/rt-suite.ps1) (§4) e non a mano: quei controlli, a mano, si dimenticano.
  ⚠️ Niente worktree per parallelizzare: il mutex del motore e' globale sull'eseguibile, quindi due run
  di automation si uccidono anche da checkout diversi.
- `D-nnn` si legge dall'ultimo assegnato nel Decision Log e si **riverifica prima del merge**: un branch
  che rivendica lo stesso ID con una tesi diversa è una collisione, e rinumeri la seconda. Il progetto ha
  già pagato **diciassette** collisioni. ⛔ **Il numero dell'ultimo assegnato non si copia qui**: scade in
  ore, e un numero fermo in un documento è la stessa classe di difetto che questa regola previene — si
  chiede al comando qui sotto, che lo produce in un secondo.
- 🔴 **La rete sono i ref REMOTI, non `gh pr list`** ([#1600](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1600)).
  Fra il commit che prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è **preso e
  invisibile**; e `gh pr list` elenca comunque le PR, non gli ID che rivendicano. Misurato il 2026-08-28:
  9 ref remoti contro 3 PR aperte, **cinque branch di lavoro fuori dalla rete**.

  ```bash
  git fetch --prune origin
  for b in $(git branch -r --format='%(refname:short)' | grep -v HEAD); do
    id=$(git show "$b:docs/decisions/RT_PDR_00_Decision_Log.md" 2>/dev/null \
         | grep -oE '^\| \*\*D-[0-9]{3}\*\*' | grep -oE 'D-[0-9]{3}' | sort -u | tail -1)
    [ -n "$id" ] && echo "$b -> $id"
  done
  ```

  Il *perché* sta nella nota su `D-213` del Decision Log, che ha pagato la collisione da cui questa
  riga nasce: un branch aveva preso l'ID venticinque minuti prima e nessuna PR poteva mostrarlo.
- Prima del merge verifica anche che il gate sia girato **sul commit che stai mergiando**: il 2026-08-27
  due PR sono state mergiate prima che il proprio gate finisse.

## 8. Decision Boundary

Una finestra live non è un'attesa del resolver:

`Resolve segment → Opportunity → Decision Boundary → response → Validate/Commit → next segment`

Visual può rallentare la presentazione; Fast/Headless risponde subito tramite policy. Il risultato logico non
dipende dal tempo reale. Non inviare al client trigger futuri, percorsi futuri o intenti privati avversari.

## 9. Test e consegna

Ordine preferito: **test mirati → regressione correlata → suite richiesta dal DoD → build → PIE/packaged se gate**.
Per scenari integrati usa il **RT Scenario Test Harness** e il percorso reale
`Intent → Planning → Snapshot → Resolver → TurnLog`. Uno scenario che non attraversa il codice vero non prova
niente sul codice vero.

Non copiare conteggi test dalla roadmap: **misurali sul branch corrente** quando servono.
Prima di consegnare controlla `git status` e diff. Niente commit/push/merge/force/delete remoto senza richiesta
esplicita; niente file generati o segreti.

Output finale:

**Risultato · File modificati · Decisioni · Test/Build · Verifiche manuali · Limiti · Prossimo passo**

Non dichiarare “funziona”, “completo”, “sicuro”, “production ready” o “deterministico” senza evidenza.

## Lingua

Rispondi e commenta in **italiano**; identificatori e termini tecnici restano in English quando naturale.
Tutoring C++/UE solo se richiesto.
