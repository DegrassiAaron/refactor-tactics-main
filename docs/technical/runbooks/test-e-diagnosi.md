# Guida — Test, scenari e diagnosi

> **Scopo**: come si verifica una modifica in RefactorTactics, dal test più veloce alla partita in editor, e
> come si risale alla causa quando qualcosa non torna.
> **Complementare a** [`debug-vs-unreal.md`](debug-vs-unreal.md), che copre il *debugger* (breakpoint, step,
> Live Coding). Qui si parla di **verifica automatica** e di **lettura degli esiti**.
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-08 (verificata contro `Scenarios/` e `ScenarioHarness/`:
> cinque scenari, quattro comandi console, percorsi corretti — nessuna correzione necessaria).
> La **spec** dell'harness — schema, assertion, esiti, `StateHash` — è
> [`test-automatico-unreal.md`](../tooling/test-automatico-unreal.md).

---

## 1. Tre livelli di verifica, e quando usarli

| Livello | Cosa verifica | Quando | Costo |
|---|---|---|---|
| **Automation Test** | regole pure: geometria, path, LOS, danno, ordinamenti, serializzazione | sempre, a ogni modifica delle regole | secondi |
| **Scenario Harness** | il turno *intero*: piano → snapshot → resolver → TurnLog, su più turni | quando tocchi il flusso di gioco o vuoi un caso riproducibile | secondi |
| **PIE (test manuali)** | ciò che nessun test vede: che si **veda** a schermo e che il giocatore **capisca** | prima di chiudere una milestone | minuti, e serve una persona |
| **Packaged** | che il gioco esista **fuori dall'editor**: compila in Shipping, cuoce, e una partita parte davvero | prima di una release (gate `packaged`, CP 12.5) | ~10 minuti, nessuna persona |

La regola pratica: **se puoi verificarlo senza aprire l'editor, fallo senza aprire l'editor.** Le voci PIE
esistono per ciò che resta — leggibilità, ritmo, giudizio — e sono elencate in
[`test-manuali-pie.md`](../test-manuali-pie.md).

### Il livello packaged, e perché serve

Ha trovato un difetto che nessun altro livello poteva trovare: il **2026-08-09** la build **Shipping era
rotta in `main`** e la suite era verde. `RTHexSimTests.cpp` chiudeva `#if WITH_DEV_AUTOMATION_TESTS` a
metà file, lasciando 197 righe di test fuori dalla guardia — invisibile in Editor, dove la guardia vale 1,
fatale in Shipping, dove vale 0. La causa strutturale: **la suite gira sul target Editor, e il target Game
non lo compila niente di automatico.**

```bash
# 1. i due target del gioco compilano? (secondi, ed e' il controllo che manca)
Build.bat RefactorTactics Win64 Development -Project=<uproject> -WaitMutex
Build.bat RefactorTactics Win64 Shipping    -Project=<uproject> -WaitMutex

# 2. cuoce e si impacchetta?
RunUAT.bat BuildCookRun -project=<uproject> -noP4 -platform=Win64 \
  -clientconfig=Development -cook -build -stage -pak -archive \
  -archivedirectory=<dir>/Packaged -utf8output

# 3. il gioco pacchettizzato gira davvero? (smoke test, nessuna persona)
Packaged/Windows/RefactorTactics.exe -nullrhi -unattended -nosound -abslog=<log>
grep "Partita finita" <log>   # la partita si gioca DA SOLA: i bot giocano entrambe le squadre
```

**Scegliere la mappa da fuori**, senza aprire l'editor:

```bash
RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset   # oppure GeneratedTestArena, GeneratedDemoArena
```

`rt.Map.Source` scavalca la proprieta' `MapSource` del `GameMode`, che vive nei Class Defaults di
`BP_GameMode` — cioe' in un `.uasset` che senza editor non si tocca. Un valore sconosciuto **non** ripiega
in silenzio: il GameMode lo dichiara e tiene la proprieta'.

> ⚠️ **Serve `-dpcvars=`, non `-ExecCmds=`.** `-ExecCmds` gira **dopo** l'inizializzazione, quando il
> GameMode ha gia' allestito la partita: la variabile viene impostata e non serve a niente, **senza un
> errore che lo dica**. Misurato sul pacchettizzato il 2026-08-10 — il log continuava a dire
> `MapSource=GeneratedTestArena` mentre la riga di comando chiedeva `LevelAsset`.

**Tre insidie, tutte costate tempo la prima volta:**

- **`-clientconfig=Development -clientconfig=Shipping` non fa due configurazioni**: la seconda viene
  ignorata in silenzio e ti ritrovi con un solo binario. Il separatore e' `+`, oppure si fanno due passate.
- **Il cook Shipping non produce log.** `UE_LOG` a livello `Display` e' compilato fuori, quindi `-abslog`
  resta vuoto e sembra che il gioco non parta. La partita si verifica sul pacchetto **Development**; per
  Shipping ci si ferma a «il processo gira» (CPU e RAM, non il log).
- **`Packaged/Windows/RefactorTactics.exe` e' un launcher stub da 168 KB.** Lanciarlo da' un processo a
  ~7 MB di RAM e 0 CPU, che sembra bloccato. Il binario vero sta in
  `Packaged/Windows/RefactorTactics/Binaries/Win64/`.

E una che e' dell'ambiente, non della ricetta: se un'altra sessione sta compilando, UAT esce con
**`Error_SDKNotFound`** — codice fuorviante, perche' la causa vera sta tre righe piu' su nel log:
`A conflicting instance of ... UnrealBuildTool_Mutex ... is already running` -> `Failed (ConflictingInstance)`.
Rimedio: `-ubtargs="-WaitMutex"`, che aspetta invece di fallire.

> **Un worktree basta, e questo va detto perché il contrario sembra ovvio.** `Content/**/*.uasset` è
> ignorato da `.gitignore`, quindi verrebbe da concludere che un worktree non abbia contenuti e non possa
> cuocere. **Non è così**: i **7** asset che il gioco usa davvero sono stati aggiunti a forza al repository
> (`git ls-files Content/`), e il cook è limitato a `+DirectoriesToAlwaysCook=(Path="/Game/RT")`.
> `Content/FabAsset` — i 44 GB di pack Paragon — **non è referenziato da nulla** (né codice, né config, né
> gli asset versionati) e **non viene cotto**: il `.pak` risultante pesa **10 MB**. Misurato il 2026-08-10 da
> un worktree senza `FabAsset`: `BUILD SUCCESSFUL`, pacchetto 915 MB, partita 2v2 su 65 celle avviata e fase
> Move risolta.
>
> Corollario pratico: **non serve nessun junction su `Content`**. Se una procedura ne chiede uno, è
> ferma a prima che i 7 asset fossero versionati.

### Confrontare lo `StateHash` fra Development e Shipping

È la procedura **Packaged** che l'invariante #4 del piano canonico elenca fra le sei di PDR-05 §10: stesso
scenario, due configurazioni di build, lo stesso hash dello stato finale. Serve a escludere che una
differenza di ottimizzazione, di `checkf` compilati fuori o di layout cambi un esito competitivo.

**Una passata sola per entrambe le configurazioni**, che è ciò che rende il confronto onesto: un cook solo,
lo stesso `.pak`, due binari. Due passate ricuocerebbero, e la differenza non sarebbe più la sola
configurazione.

```bash
RunUAT.bat BuildCookRun -project=<uproject> -noP4 -platform=Win64 \
  -clientconfig=Development+Shipping -cook -build -stage -pak -archive \
  -archivedirectory=<dir> -utf8output -ubtargs="-WaitMutex"
```

```text
Binaries/Win64/RefactorTactics.exe                 336 MB   Development
Binaries/Win64/RefactorTactics-Win64-Shipping.exe  167 MB   Shipping
Content/Paks/RefactorTactics-Windows.pak            10 MB   lo stesso per entrambi, scenari inclusi
```

Il confronto si legge da **`result.json`, non dal log** — ed è una scelta, non un dettaglio: in Shipping il
logging è compilato fuori e `-abslog` non produce nemmeno il file, mentre
`Saved/RTTests/<Id>/<Run>/result.json` è file I/O e viene scritto lo stesso.

```bash
# Development
<Staged>/RefactorTactics/Binaries/Win64/RefactorTactics.exe \
  -RTScenario=Movement.Collision -nullrhi -unattended -nosound

# Shipping — stesso flag, stesso scenario
<Staged>/RefactorTactics/Binaries/Win64/RefactorTactics-Win64-Shipping.exe \
  -RTScenario=Movement.Collision -nullrhi -unattended -nosound
```

⚠️ **`-RTScenario=`, non `-dpcvars=rt.Test.Scenario=`.** La seconda funziona solo in Development: in
`DeviceProfileManager.cpp` tutto il parsing di `-dpcvars=` sta dentro `#if !UE_BUILD_SHIPPING`, quindi in
Shipping la variabile non viene mai impostata e il gioco allestisce la partita normale — **senza dirlo**,
perche' anche il logging e' compilato fuori. `FParse::Value` non ha quella guardia.

🔴 **E il report NON e' dove lo cerchi: in Shipping `Saved/` e' redirezionata nella cartella utente.**

```text
Development   <Staged>/RefactorTactics/Saved/RTTests/<Id>/<Run>/result.json
Shipping      %LOCALAPPDATA%/RefactorTactics/Saved/RTTests/<Id>/<Run>/result.json
```

E' costato mezz'ora di diagnosi sbagliata: cercando accanto all'eseguibile si trova **un solo report** invece
di due, e la conclusione ovvia — «Shipping non ha girato» — e' falsa. Il segnale di «non ha girato» resta
l'assenza del report, ma va cercata **nella cartella giusta**.

> ⚠️ **In Shipping «nessun file» significa «non ha girato», non «è andato bene».** Senza log, l'assenza di un
> report è l'unico segnale che resta, e va letta come fallimento — mai come conferma.

#### ✅ Eseguita per intero il 2026-08-16, e i due hash coincidono

```text
Development   PASS   stateHash 572184bb   seed 0   1 turno
Shipping      PASS   stateHash 572184bb   seed 0   1 turno
```

E' la prima volta che questa procedura produce un verdetto invece di una previsione. Le due cause che la
bloccavano sono cadute, e valgono come lezione piu' del risultato:

1. **`Scenarios/` non entrava nel pacchetto.** Sono `.json` **fuori** da `Content/`: il cook non li vede e lo
   staging non li copiava — l'indice diceva *«0 scenari»*. Chiuso in `DefaultGame.ini` con
   `+DirectoriesToAlwaysStageAsUFS=(Path="../Scenarios")`.
   🔴 Il `Path` e' relativo a **`Content/`**, non alla radice: UAT fa `Combine(ProjectContentRoot, RelativePath)`.
   Scritto senza `../` cerca `Content/Scenarios`, non lo trova, **stagea zero file e lascia la build
   `SUCCESSFUL`** — l'unico segnale e' un warning nel log della cottura.
   ⚠️ E non si verifica con `find` sullo staged: i file UFS finiscono **dentro il `.pak`**, quindi contare i
   `.json` sotto la cartella da' `0` anche quando ha funzionato. Restano la dimensione del pak
   (**10 654 115 → 10 790 839** byte, i 76 JSON) e, l'unico che conta, **eseguire**.
2. **In Shipping non c'era modo di scegliere lo scenario dall'esterno.** `-dpcvars` e' compilato fuori (sopra).
   Chiuso con una terza sorgente in `ARTGameMode::ResolveScenarioToRun()`, letta con `FParse::Value`:

```text
proprieta' del GameMode  <  -RTScenario=<Id>  <  rt.Test.Scenario
(persistente)               (questo avvio)      (adesso, anche a meta' sessione)
```

   La console resta la piu' specifica perche' si puo' digitare **dopo** l'avvio: se vincesse il flag, in
   editor non si potrebbe cambiare scenario senza riavviare. In Shipping l'ordine non e' osservabile — li'
   la console non arriva — ed e' giusto che l'invariante sia dell'editor.

✅ **Prova controllata, non aneddotica**: prima del flag la Shipping non scriveva **nessun** report in nessuna
delle due cartelle; dopo, ne scrive uno con lo stesso hash della Development. La differenza e' il flag.
---

## 2. Test automatici

### Eseguirli

```bash
# tutti
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics+Quit" \
  -unattended -nopause -nullrhi -NoSound

# una sola area (molto più veloce)
-ExecCmds="Automation RunTests RefactorTactics.Scenario+Quit"
```

Gli esiti finiscono in `Saved/Logs/RefactorTactics.log`, non nello stdout:

```bash
grep -aE "Test Completed. Result=" Saved/Logs/RefactorTactics.log \
  | sed 's/.*Result={/  /;s/} Name={/  /;s/}.*//'
grep -a "tests performed" Saved/Logs/RefactorTactics.log | tail -1
```

### Contarli

Non citare mai il numero a memoria — **misuralo**:

```bash
git grep -hA 1 'IMPLEMENT_[A-Z_]*AUTOMATION_TEST(' -- 'Source/**/Tests/*.cpp' \
  | grep -oE '"RefactorTactics\.[A-Za-z0-9_.]+"' | tr -d '"' | sort -u | wc -l
```

Questo comando è la fonte: la documentazione che dichiara un numero diverso è indietro, non il contrario.

> ⚠️ **Due dettagli del comando non sono stile: sono la differenza fra contare e sbagliare** — e li ha
> trovati entrambi una code review, dopo che una prima correzione ne aveva sistemato solo uno.
>
> **È ancorato alla macro.** Cercare *qualunque* stringa che cominci per `RefactorTactics.` conta anche
> ciò che test non è: `RefactorTactics.Probe.LatestRunDirectory` è uno `ScenarioId` dentro
> `FRTScenarioLatestRunIsTheMostRecentTest`, usato per costruire una directory sotto `Saved/RTTests/`. Il
> filtro costruito su quel nome rispose `0 tests performed`, che a prima vista è indistinguibile da un
> test sparito, e per un momento sembrò che la suite fosse troncata.
>
> **E cerca in tutti i moduli**, non nel solo `RefactorTactics`: `Source/RefactorTacticsEditor/Private/Tests/`
> dichiara `HexEditor.BrushMoveCostFollowsSurface` e `HexEditor.ReadoutDoesNotAutoUpdate`, che il filtro
> `Automation RunTests RefactorTactics` **esegue**. Sul solo modulo principale il comando dava 1183 dove
> la run ne fa **1185** — uno scarto in difetto, che è il verso peggiore: fa concludere che siano stati
> eseguiti test che non esistono invece che il contrario.
>
> ∴ quando il conteggio non torna, prima di concludere che la run sia troncata **cerca la stringa che
> avanza o il modulo che manca**: `git grep -rn "<id>" -- Source/` dice subito se è il nome di un test, un
> dato dentro a uno, o un test che vive altrove.

### Compilare

```bash
"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development \
  -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex
```

Cerca `Result: Succeeded`. **L'editor deve essere chiuso**, altrimenti il link fallisce (vedi §7).

---

## 3. Scenari: cos'è un test di scenario

Un Automation Test verifica una funzione. Uno **scenario** fa giocare una partita vera e verifica come è
finita — passando dal codice reale, non da scorciatoie.

```
Scenarios/Movement/Basic.json
        │
        ▼
  URTScenarioLoader          parsing + validazione   →  ERROR se il file è sbagliato
        │
        ▼
  URTScenarioRunner          scrive i piani sulle unità, chiama LockInAndResolve()
        │                    ← STESSA strada del giocatore: nessun SetActorLocation
        ▼
  ARTTurnManager → Snapshot → Resolver → TurnLog
        │
        ▼
  assertion → FRTTestResult → Saved/RTTests/<Id>/<Run>/result.json
```

Il turn manager e il resolver **non sanno** di essere sotto test: non esiste nessun `if (IsTest)` nel
gameplay. È la proprietà che rende un test verde significativo.

### Lanciarne due di seguito in PIE — il campo si sgombera da solo

Fuori dal PIE ogni corsa riceve un `UWorld` **temporaneo** e parte pulita per costruzione. In PIE il mondo è
quello della sessione e non si può ricreare: fino al 2026-09-04 il secondo scenario si **sommava** al primo,
e ciò che si misurava era il residuo ([#2223](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2223)).

Ora `FRTScenarioSession::Start` toglie dal mondo, **prima di posare qualunque cosa**, le unità marcate da una
corsa precedente.

- ⛔ **Si tolgono solo le unità che uno scenario ha messo**, mai tutte quelle in campo: in PIE lo scenario
  gira *dentro* una partita, e sgomberarla sarebbe un rimedio peggiore del difetto. Il marchio è
  `FRTScenarioSession::SpawnedByScenarioTag`, in `AActor::Tags`.
- ⚠️ **Si sgombera all'ingresso, non alla fine**: uno scenario interrotto a metà non arriverebbe mai a un
  teardown finale — ed è proprio quello a lasciare il campo sporco. Entrando, la corsa è pulita *qualunque
  cosa* sia successa alla precedente.
- La **board** non c'entra e non si duplicava: `BuildScenarioArena` riusa l'actor esistente e chiama
  `RebuildInstances()`.

### 3-bis. Chi decide: un harness solo, e provider che restituiscono decisioni

**[D-101](../../decisions/RT_PDR_00_Decision_Log.md)** (2026-08-11). L'harness sopra è **l'unico**, e ogni modo
di giocare uno scenario è una sua modalità — non un secondo harness.

Oggi chi produce le decisioni è deciso in due punti diversi, entrambi nati per necessità:

| Chi decide | Come entra oggi |
|---|---|
| Intent scriptati | `FRTScenarioIntent`, letti dalla definizione dello scenario |
| Bot | `ARTTurnManager::PlanBotsForTest()` — che il commento di `RTScenarioRunner.h` dichiara per quello che è: *«l'unico appiglio, che esisteva già per i test d'integrazione»* |

Funziona per due casi. Al terzo — partita bot contro bot, replay di una traccia, squadra mista umano+bot,
batch di partite — servirebbe un terzo appiglio, poi un quarto: è così che nasce un secondo harness, **per
accumulo e senza che nessuno lo decida**.

La forma decisa è un **provider**: riceve un contesto sanificato e restituisce un `Intent` o una risposta di
reazione. Scriptato, bot, replay, umano e policy di test differiscono solo per *come scelgono*.

> ⚠️ **Un provider non restituisce mai un esito.** Se potesse, un test dichiarerebbe cosa deve succedere e poi
> verificherebbe che è successo — senza che il resolver abbia deciso niente. È lo stesso difetto che questo
> documento evita già vietando `SetActorLocation`, e la ragione per cui il riquadro qui sopra dice *«STESSA
> strada del giocatore»*: qui prende la forma di un'interfaccia, quindi va vietato **nell'interfaccia**.

> ⚠️ **L'umano non si simula.** Il provider umano è il normale `PlayerController`: un harness che finge input
> collauderebbe un percorso che nessun giocatore attraversa.

Le modalità di esecuzione — visuale, veloce, headless, batch, audit di replay — cambiano **cosa si vede**,
mai cosa succede. È lo stesso confine che [D-078](../../decisions/RT_PDR_00_Decision_Log.md) traccia fra chi
riproduce un replay e chi lo verifica.

### 3-ter. Un risultato bot-contro-bot non è ancora una misura di bilanciamento

**[D-102](../../decisions/RT_PDR_00_Decision_Log.md)** (2026-08-11). Vale quando l'harness comincerà a produrre
partite in serie, ed è già vero adesso su un numero che il repository usa.

`roadmap-checkpoint.md` misura i round per partita e annota, correttamente: *«**10** misurato bot-vs-bot —
dentro banda, **ma è un bot contro un bot**»*. Il dubbio era già formulato; mancava cosa farne.

La regola: una metrica di bilanciamento prodotta da partite bot-contro-bot porta con sé lo **stato di
competenza** delle capability che la determinano — `PASS`, `PARTIAL`, `FAIL`, `UNTESTED`, ciascuno legato a
uno scenario reale. Se una capability decisiva è `FAIL` o `UNTESTED`, la conclusione ammessa è *«il bot non
sa giocarla»*, **non** *«l'eroe è debole»*.

> ⚠️ **Il costo di ignorarla è invisibile, ed è il motivo per cui la regola sta qui e non in una nota.** Un
> nerf deciso su un bot che non sa usare un'abilità **sembra** funzionare: il win rate si muove. Il difetto
> vero — che quell'abilità non entra mai fra le candidate — resta coperto proprio dalla correzione.

Non è un gate della CI sul win rate: una variazione dell'1% non fa fallire niente. I fallimenti duri restano
crash, intent illegale, divergenza di determinismo, fuga di stato nascosto e **collasso della generazione di
candidate** — che è il sintomo tecnico di ciò che questa regola protegge.

---

## 4. Scrivere uno scenario

File `.json` sotto `Scenarios/`, in **qualunque** sottocartella. L'ID lo dichiara il file, non il percorso:
spostare uno scenario non ne cambia l'identità, e a trovarlo ci pensano i **tag**. Il modello, il perché e la
tabella di redirect stanno in [`scenario-index-e-tag.md`](../tooling/scenario-index-e-tag.md).

```json
{
  "scenarioId": "Movement.Basic",
  "tags": ["movement", "core", "animation", "gadget", "riktor"],
  "version": 1,
  "seed": 0,
  "mapRadius": 3,

  "units": [
    { "id": "A1", "hero": "Hero.Gadget",    "team": 0, "cell": [-2, 0, 0] },
    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
  ],

  "turns": [
    { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] }
  ],

  "expect": [
    { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] },
    { "type": "TurnsCompleted", "value": 1 }
  ]
}
```

| Campo | Significato |
|---|---|
| `scenarioId` | ID stabile e gerarchico, **univoco** fra tutti gli scenari. Non deve corrispondere al percorso |
| `tags` | *(opzionale)* parole per cui filtrare nell'Editor: tipologia, lente, personaggio. Vedi [`scenario-index-e-tag.md`](../tooling/scenario-index-e-tag.md) |
| `mapRadius` | arena esagonale piena generata da codice (nessun `.umap` da versionare) |
| `cells` | *(opzionale)* celle da modificare: `blocksMovement`, `blocksLineOfSight`, `moveCost` |
| `hero` | ID stabile dal catalogo: `Hero.Gadget` · `Hero.Phase` · `Hero.Riktor` · `Hero.Wraith` |
| `cell` | `[q, r]` oppure `[q, r, layer]` — il layer è opzionale e vale 0 |
| `move` | lista di **waypoint**, come li produrrebbe il giocatore cliccando |
| `ability` | `ActionId` dell'abilità (`Hero.Gadget.ArcPulse`) — per **ID**, non per indice |
| `target` | ID di scenario del bersaglio; obbligatorio con `ability` |
| `reaction` | `ActionId` della reazione che l'unità **arma** per il turno — nessun bersaglio |
| `bot` | *(opzionale)* l'unità è guidata dal **pianificatore del gioco**, non dal file. Un intent scritto per lei è un errore |
| `health` · `shield` · `visionRange` | *(opzionale)* condizione iniziale al posto di quella del roster. Assenti = valori dell'eroe |
| `variants` | *(opzionale)* rigioca lo scenario spostando alcune unità — vedi sotto |
| `expectSameAcrossVariants` | *(opzionale)* le varianti devono produrre lo **stesso** TurnLog |
| `freeRun` | *(opzionale)* gioca **fino alla fine partita** invece di enumerare i turni — vedi sotto |
| `maxTurns` | tetto di sicurezza del free-run, obbligatorio con `freeRun` e vietato senza |
| `repeatCount` | *(opzionale)* esegue lo scenario N volte e confronta le tracce. `1` = una sola |
| `requires` | *(opzionale, solo `freeRun`)* capability richieste dall'**intero** scenario |
| `expect` | assertion; **almeno una**, altrimenti lo scenario passerebbe sempre |

### `bot` — chi decide l'intent

Un'unità dichiarata `"bot": true` non prende il piano dal file: lo produce `ARTTurnManager::PlanBots()`, lo
stesso che gira in partita. L'harness non apre un canale nuovo — usa `PlanBotsForTest()`, l'appiglio che il
runner dichiara da sempre come l'unico.

> ⚠️ **Non è il seam dei `DecisionProvider`** ([D-101](../../decisions/RT_PDR_00_Decision_Log.md), #542, v0.2).
> Quello serve quando i modi di giocare uno scenario diventano tre — bot vs bot, replay, umano+bot. Qui resta
> uno: file per gli umani, pianificatore per i bot, che è la composizione della v0.1.

Un intent dichiarato per un'unità bot è **rifiutato dal loader**: `PlanBots` azzera il piano di ogni unità che
guida, quindi uno dei due sovrascriverebbe l'altro in silenzio e lo scenario resterebbe verde nei casi in cui
le due scelte coincidono per caso.

`health`, `shield` e `visionRange` usano `-1` come «non dichiarato», mai `0`: uno scudo azzerato è una
richiesta legittima, e un sentinella che coincide con un valore valido non sa distinguere le due cose.

### `variants` — l'unica forma di un canary d'indipendenza

```json
"variants": [
  { "name": "hidden-ovest", "units": [ { "id": "HIDDEN", "cell": [-6, 3, 0] } ] },
  { "name": "hidden-est",   "units": [ { "id": "HIDDEN", "cell": [6, -3, 0] } ] }
],
"expectSameAcrossVariants": true
```

Lo scenario si gioca una volta per variante, cambiando **solo** le celle dichiarate; ogni variante deve
superare le stesse `expect`. Con `expectSameAcrossVariants` i TurnLog devono coincidere.

Serve quando la proprietà da dimostrare è che un ingresso **non ha avuto effetto** — «il bot non ha usato
un'informazione». Non è osservabile in una partita sola: ha la stessa forma di «il bot ha deciso così», e le
due si distinguono soltanto cambiando quell'informazione e guardando se l'esito si muove.

Il confronto è sul **TurnLog** e non sullo stato finale, che contiene la posizione dell'unità spostata e
sarebbe diverso per costruzione. Due normalizzazioni, entrambe necessarie e nessuna delle due indulgente:

- si escludono le voci **emesse** dall'unità che la variante sposta (`BuildMoveLog` ne scrive una per ogni
  unità, ferme incluse, con chiave la cella di partenza). Restano le voci che la *riguardano*: se il bot la
  bersagliasse, la voce di combattimento avrebbe come sorgente la cella del bot;
- si azzera `UnitId`, che è l'**indice** nello snapshot: lo snapshot ordina per cella, quindi spostare
  un'unità fa scalare gli indici di tutte le altre. Misurato — era l'unica differenza fra due tracce
  altrimenti identiche. L'identità stabile di una voce resta `SrcCell`, come `BuildMoveLog` dichiara.

Il loader rifiuta le quattro forme in cui un confronto nasce vuoto: una sola variante, una variante che non
sposta nulla, due varianti omonime, un'unità spostata sopra un'altra.

> ⚠️ **`expectSameAcrossVariants` da solo non basta**: due partite in cui non succede niente hanno TurnLog
> identici. Serve accanto una `expect` che dimostri che qualcosa è successo — in `Spec.Bot.HiddenEnemyFairness`
> è `LogEventCount(Combat.Hit) = 1`.

### `freeRun` — la partita decide quando finire *(CP 47.4)*

```json
"freeRun": true,
"maxTurns": 40,
"units": [
  { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-4, 2, 0], "bot": true },
  { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [4, -2, 0], "bot": true }
]
```

Una partita autobattle **non sa in anticipo quanti turni durerà**, e scriverne il numero nel file
significherebbe dichiarare ciò che lo scenario dovrebbe misurare. Con `freeRun` il file non elenca turni: la
sessione ne gioca uno dopo l'altro finché il turn manager non entra in `MatchEnded`.

Le regole, tutte rifiutate dal loader quando violate:

- `turns` dev'essere **vuoto** — o decide la partita, o decide il file;
- **ogni** unità è `bot` — in free-run gli intent non li scrive nessuno, e un'unità umana resterebbe ferma per
  tutta la partita senza dirlo;
- `maxTurns` si **dichiara**, è positivo e non supera `URTScenarioRunner::MaxTurnsHardCap` (100).

> 🔴 **Raggiungere il tetto è un `FAIL`, non un `PASS`.** Il tetto è una guardia di sicurezza, non una regola di
> gioco: una partita che non finisce è il difetto misurato in
> [`#1088`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1088) — dodici round di soli
> spostamenti, zero combattimento, pareggio allo scadere. Un tetto che producesse verde renderebbe invisibile
> esattamente ciò che deve cogliere.

Il verdetto arriva come **assertion generata**, `MatchReachedEnd`, con l'esito nel campo `actual`
(`"Vince il team 0 per eliminazione, al turno 21"`): così porta atteso e ottenuto come tutte le altre, invece
di essere un codice d'uscita che costringe a rieseguire per capire.

#### `requires` di scenario

`requires` vive sul **turno**, e un free-run non ha turni. `AutoBattle.Objective` è il caso: chiede
`Objective`, che è un nome **noto e non disponibile** (owner `#75`), ed esce `BLOCKED` senza giocare un turno.

> 🔴 **Senza la chiave di scenario l'esito non sarebbe un rosso, sarebbe un verde.** Misurato disattivando il
> blocco: quel file gioca la partita fino all'eliminazione ed esce **`PASS` in 10 turni**. Dichiarerebbe «fine
> partita per obiettivo» verificando una fine per **eliminazione** — e a un verde non va a guardare nessuno.

Fuori dal free-run la chiave è rifiutata: lì il posto del requisito è il turno, e due posti per la stessa
dichiarazione divergono al primo edit di uno solo.

#### `repeatCount` — la domanda opposta a `variants`

```json
"repeatCount": 3
```

Le varianti cambiano un ingresso per vedere se l'esito **si muove**; `repeatCount` non cambia niente per
vedere se **sta fermo**. È il veicolo del corpus di determinismo (E47.5), e il confronto è su due grandezze:
il **TurnLog** serializzato turno per turno (`SameTurnLogAcrossRuns`) e lo **StateHash** finale
(`SameStateHashAcrossRuns`).

Nessuna delle due basta da sola, ed è misurato: il log non registra tutto ciò che il digest copre, e lo
`StateHash` da solo *«sarebbe rimasto verde su #990»*, dove la divergenza compariva al turno 2 e lo stato
finale tornava lo stesso.

> ⚠️ Le due chiavi **non si combinano**: il loader rifiuta `repeatCount` insieme a `variants`. Un ciclo
> annidato produrrebbe N×M tracce di cui metà differiscono per costruzione, e un confronto che mescola le due
> domande non risponde a nessuna.

### `seed` — dichiarato, non consumato

Il campo esiste ma **oggi non fa nulla**: il progetto non ha alcun RNG, e il determinismo viene da coordinate
intere e ordinamenti totali. È lì perché il giorno in cui un RNG entrerà nel resolver, gli scenari sappiano
già dichiararlo. Viene registrato nel report.

### Assertion disponibili

| Tipo | Verifica |
|---|---|
| `UnitAtCell` | l'unità è sulla cella attesa a fine scenario |
| `TurnsCompleted` | sono stati giocati almeno N turni |
| `UnitHpEquals` | la salute è **esattamente** il valore atteso — è così che si verifica un danno |
| `UnitAlive` | l'unità è viva (`true`) o abbattuta (`false`) |

Si aggiungono quando servono, non prima. Il modello dati regge le altre senza modifiche strutturali.

> `UnitHpEquals` riporta anche lo **scudo** nell'`actual` (`"98 (scudo 15)"`): un danno assorbito dallo scudo
> si legge come «HP invariati», che è diverso da «nessun danno». La distinzione conta quando arriveranno le
> reazioni difensive.

### Ostacoli e terreno

Per gli scenari che hanno bisogno di un muro o di terreno costoso, `cells` modifica l'arena generata senza
versionare un `.umap`:

```json
"cells": [
  { "cell": [0, 0, 0], "blocksMovement": true },
  { "cell": [1, 0, 0], "moveCost": 3 },
  { "cell": [2, 0, 0], "blocksLineOfSight": true }
]
```

Le celle non elencate restano pavimento a costo 1. Due controlli evitano scenari privi di senso: una cella
modificata **fuori dall'arena** (un ostacolo che non blocca niente) e un'unità che **parte dentro un
ostacolo** (una situazione che il gioco non produrrebbe mai) vengono rifiutate con `ERROR`.

> ⚠️ **Coordinate assiali**: il raggio limita anche `r`. Su un'arena di raggio 3, per `q = -1` le `r` valide
> vanno da **−2 a 3**, non da −3 a 3. Sforare produce «cella fuori dall'arena» — succede facilmente quando si
> genera un muro con un ciclo.

### Scenari disponibili

| ID | Verifica |
|---|---|
| `Movement.Basic` | l'unità raggiunge la cella adiacente pianificata |
| `Movement.BasicFailsOnPurpose` | **FAIL voluto**: dimostra che il report diagnostica invece di dire solo «fallito» |
| `Movement.Blocked` | un muro rende il percorso impossibile: il piano è rifiutato, l'unità resta ferma, il turno si chiude lo stesso |
| `Movement.Collision` | due unità verso la stessa cella si fermano **entrambe** |
| `Movement.LongWalk` | due unità attraversano l'arena, 3 celle per turno per 2 turni — **fatto per essere guardato** in PIE |
| `Combat.BasicAttack` | Gadget colpisce Riktor: 120 → 103 HP — 22 dichiarati, 5 assorbiti dal `BaseShield` (D-224), 17 applicati. Il primo scenario che verifica un **danno** |
| `Combat.BlockedByWall` | stesso attacco con un muro in mezzo: il colpo **non parte** |
| `Combat.SplashHitsAlliesNotSelf` | l'area colpisce i vicini **e** l'alleato, ma non chi la lancia |
| `Combat.LineHitsThrough` | la linea colpisce chi sta in mezzo, non solo il bersaglio |
| `Combat.CounterStrikesBack` | una reazione **armata** scatta e colpisce chi ha colpito |
| `Combat.NoCounterWhenUnarmed` | senza armarla, la stessa reazione non scatta |
| `Movement.SwapRejectedByPlanning` | **caratterizzazione**: due unità adiacenti *non* si scambiano di posto, perché la pianificazione rifiuta un percorso verso una cella occupata (vedi §12) |
| `AutoBattle.OpenField` | **free-run**: 2v2 bot contro bot su campo aperto, giocato fino alla fine partita |
| `AutoBattle.Obstacles` | lo stesso 2v2 su un campo con ostacoli, muri e fango — lo stallo di `#1088` si formava con la geometria, non senza |
| `AutoBattle.Hazard` | lo stesso 2v2 sulla fixture `RelayLite`: acqua conduttiva, fuoco, ghiaccio, fumo |
| `AutoBattle.Objective` | **BLOCKED**: fine partita per obiettivo, che aspetta la capability `Objective` (owner `#75`) |

### Scenari di caratterizzazione

Non tutti gli scenari descrivono una regola *voluta*. Alcuni fissano il comportamento **attuale** perché non
cambi per sbaglio, e dichiarano nel file stesso perché sono lì. Si riconoscono dal campo `_nota` che dice
«caratterizzazione».

Servono quando si scopre qualcosa che **non si è autorizzati a correggere**: se il comportamento sia giusto è
una decisione di design, non una svista. Il test tiene fermo lo stato di fatto e diventa rosso il giorno in
cui qualcuno lo cambia — che è il segnale desiderato, non un fastidio da mettere a tacere.

### Scenari in coppia

`Combat.BasicAttack` e `Combat.BlockedByWall` vanno letti **insieme**: da solo, «l'attacco fa 22 danni» non
distingue un gioco che rispetta la copertura da uno che spara attraverso i muri.

E nemmeno la coppia basta. Entrambi passerebbero se l'attacco non partisse **mai** — abilità sbagliata,
bersaglio nullo, intent ignorato — perché «120 HP» è anche il risultato di «non è successo niente». Serve un
terzo test che confronti i due `stateHash` e pretenda che **differiscano**
(`Scenario.WallIsWhatStopsTheShot`). È la stessa forma di `Simulation.StateHashDistinguishesOutcomes`: quando
due test si confrontano fra loro, qualcosa deve garantire che non stiano confrontando due zeri.

### Armare non è agire

`reaction` dichiara solo **cosa succederà se** il trigger scatta durante la risoluzione: non è un'azione, e
non ha bersaglio — chi la subirà lo decide il trigger (chi ha colpito, quale alleato è stato preso). Per
questo è un campo suo e non riusa `ability`/`target`: sono due slot diversi dell'unità, e la stessa unità
può attaccare **e** tenere armata una reazione nello stesso turno.

> Una reazione difensiva protegge dal colpo **che l'ha innescata**, non dai successivi. In
> `Combat.CounterStrikesBack` lo scudo 15 assorbe i 24 in arrivo e Gadget resta a 81, non a 66. È la ragione
> per cui `Action.Deflect` (una `DamageReduction`) ha senso: se agisse solo sui colpi futuri, rispondere a
> chi ti ha appena sparato non servirebbe a niente.

La validazione della reazione sta nel **loader**, non a runtime, perché il suo modo di fallire è silenzioso:
armare qualcosa che non è una reazione non produce nessun effetto e nessun errore, e si vedrebbe solo
un'assertion sui danni che non torna. Il loader controlla due cose — che l'eroe la possieda, e che occupi
davvero lo slot `Reaction`.

### Limiti attuali

Niente **reazioni** e niente abilità ad area con bersaglio su cella: l'intent bersaglia sempre un'unità. Le
forme (`Line`, `Area`, `Cone`) si risolvono normalmente — è il *bersaglio dichiarato* a dover essere un'unità.

---

## 5. Eseguire uno scenario

### Da PIE (durante una partita)

```
rt.Test.List                            elenca gli scenari
rt.Test.Run Movement.Basic              esegue e scrive il report
rt.Test.DumpResult                      stampa l'ultimo result.json in console
```

Le assertion fallite si stampano in console con **atteso e ottenuto**, non serve aprire il file.

### Auto-run: premi Play e parte

Due modi, con precedenze diverse perché servono a cose diverse.

**Dal `BP_GameMode`** — la via normale, e quella che **sopravvive alla sessione**:

| Proprietà (categoria *RefactorTactics\|Test*) | Effetto |
|---|---|
| `ScenarioFilterA` / `ScenarioFilterB` | due tag che **restringono** la tendina sottostante, in intersezione. Vuoti = nessun filtro |
| `ScenarioToRun` | **menu a tendina** con gli scenari che passano i filtri. Prima voce **vuota** = partita normale |
| `ScenarioPlanningSeconds` | durata della pianificazione **mentre gira uno scenario** (default **3 s**). `0` = nessuna scadenza, l'immagine resta ferma |

Il menu si popola **leggendo i file** in `Scenarios/` (`GetScenarioOptions`), non da un elenco scritto nel
codice: aggiungere uno scenario lo fa comparire nella tendina senza toccare nulla, e non si può selezionare
un ID che non esiste. Vale anche per il vocabolario dei due filtri, che è l'unione dei tag realmente presenti
negli scenari — così non esistono voci che non filtrano niente.

I filtri sono una **vista, non un vincolo**: restringere l'elenco non tocca mai `ScenarioToRun`. Uno scenario
già scelto resta scelto ed eseguito anche mentre i filtri mostrano altro. Il perché sta in
[`scenario-index-e-tag.md`](../tooling/scenario-index-e-tag.md).

Si impostano una volta nei *Class Defaults* di `BP_GameMode`, si salva, e da lì in poi **al primo Play lo
scenario parte**. Non c'è niente da ridigitare a ogni riavvio dell'editor.

**Da console o riga di comando** — l'override estemporaneo, che **prevale** sulla proprietà:

```
rt.Test.Scenario Movement.Collision
```

La regola è quella di ogni override di configurazione: **il più specifico vince**. La proprietà dice «questo
progetto, per ora, esegue questo scenario»; la console dice «adesso, solo per questa volta, un altro» — ed è
ciò che serve in CI, dove l'asset non si tocca. Se fosse il contrario, impostare la proprietà renderebbe
impossibile eseguire uno scenario diverso da riga di comando.

> ⚠️ **La console variable dura quanto il processo dell'editor.** Digitata una volta, resta attiva per **ogni
> Play successivo** e continua a scavalcare la tendina. È già costato una sessione di diagnosi: si sceglieva
> uno scenario nel Details Panel e ne partiva un altro.
>
> Ora il log lo dice sempre — `AUTO-RUN <scenario> (da: proprietà del GameMode | console rt.Test.Scenario)` —
> e in caso di conflitto avverte esplicitamente. Per tornare alla proprietà:
> ```
> rt.Test.Scenario ""
> ```

In entrambi i casi il `GameMode` esegue lo scenario **invece** di allestire la partita normale. Per tornare a
giocare: svuota la proprietà (e la console variable, se l'avevi impostata).

> **Perché la pianificazione si accorcia.** Lo scenario risolve i propri turni da solo e poi lascia il turn
> manager in pianificazione. Col timer normale (30 s) si resterebbe a guardare un turno vuoto per mezzo
> minuto, poi un altro, poi un altro ancora — un'attesa che rende inguardabile una verifica che dura un
> secondo. La partita normale continua a usare i suoi 30 s: è ritmo di presentazione, non una regola di gioco.

### Da test automatico

Gli scenari sono anche Automation Test: `RefactorTactics.Scenario.*`. Chiamano lo **stesso** runner, quindi
non esiste logica duplicata fra «eseguito a mano» ed «eseguito in CI».

---

## 6. Leggere un report

`Saved/RTTests/<ScenarioId>/<RunId>/result.json`. Il `RunId` è cronologico, quindi *l'ultima run* è l'ultima
in ordine alfabetico — nessun indice da mantenere.

### FAIL — il gioco non fa quel che ci si aspettava

```json
{
  "result": "FAIL",
  "turnsPlayed": 1,
  "assertions": { "passed": 0, "failed": 1 },
  "failures": [{
    "assertion": "UnitAtCell",
    "expected": "(q=3,r=0,L=0)",
    "actual":   "(q=-1,r=0,L=0)",
    "turn": 1
  }]
}
```

### ERROR — non si è potuto eseguire

```json
{
  "result": "ERROR",
  "error": "scenario non leggibile: .../Scenarios/Non/Esiste.json"
}
```

**La distinzione è la cosa più importante del report.**

| | Significato | Dove cercare |
|---|---|---|
| `FAIL` | simulazione completata, aspettativa non soddisfatta | **il gioco** — una regola si comporta diversamente |
| `ERROR` | non si è potuto eseguire | **il test** — scenario malformato, eroe inesistente, file mancante |

Il campo `error` compare **solo** negli `ERROR`: la sua presenza è ciò che impedisce di inseguire una
regressione che non esiste.

---

## 7. Diagnosticare un fallimento

1. **Leggi `result.json`** — `expected` vs `actual` dicono già cosa è successo.
2. **`ERROR`?** Il difetto è nello scenario. Il messaggio dice cosa: eroe sconosciuto, cella fuori arena, ID
   duplicato, assertion sconosciuta, versione di formato non supportata.
3. **`FAIL`?** Guarda `turnsPlayed`: se è 0, la partita si è chiusa prima; se l'unità è rimasta alla cella di
   partenza, il percorso è stato **rifiutato** (budget, blocco, occupante) e il log lo dice:
   ```
   [RT-Test] <Scenario>: percorso rifiutato per 'A1' (l'unita' resta ferma)
   ```
4. **Restringi**: esegui solo l'area di test interessata (`Automation RunTests RefactorTactics.HexMove`).
5. **Riproduci in PIE**: `rt.Test.Scenario <Id>` + Play, e guarda cosa succede a schermo.

---

## 8. Vedere le regole in partita

In partita le celle sono dischi grigi identici: fango, muri e ostacoli sono **invisibili** finché non li
accendi. Una mappa che non comunica le proprie regole rende impossibile distinguere un difetto del gioco da
una regola non mostrata.

```
rt.Debug.DrawCells 1        contorno = superficie · rosso interno = blocca il movimento
                            giallo interno = blocca la vista
rt.Debug.DrawCells 0        spegne (senza argomento fa da interruttore)
```

L'**anteprima di pianificazione** è sempre attiva: celle raggiungibili in verde tenue, percorso in ciano,
zona colpita in **rosso**, alleati dentro l'area in **arancione** (fuoco amico, visibile *prima* del lock-in).

Per una mappa con muri, fango, dislivello e rampa già pronti: `MapSource = GeneratedTestArena` sul
`BP_GameMode`.

---

## 9. Insidie note

Ognuna è costata tempo almeno una volta.

| Sintomo | Causa | Rimedio |
|---|---|---|
| `LNK1104: impossibile aprire UnrealEditor-RefactorTactics.dll` | un `UnrealEditor-Cmd` di un run precedente non è uscito e tiene la DLL | chiudi il processo, poi ricompila |
| I test «passano» ma sono di un run vecchio | `RefactorTactics.log` contiene ancora l'esecuzione precedente | controlla il **timestamp** delle righe, o attendi `tests performed` del run nuovo |
| `-ExecCmds` sembra ignorare un filtro | `+` separa i **comandi**, non i filtri: `RunTests A+B` esegue `RunTests A` e poi il comando `B` | un solo filtro per esecuzione |
| `rt.Test.Run` non fa nulla headless | senza una mappa caricata non esiste un mondo di gioco | usa l'Automation Test, o eseguilo in PIE |
| Errori di simboli duplicati fra file di test | la *unity build* mette più `.cpp` nella stessa translation unit | dai **nomi distinti** agli helper nei namespace anonimi di ogni file |
| Linee di debug invisibili | disegnate **sotto** la faccia del disco-cella (che sta a `z = 2.5`) | usa le costanti `RTLift*` di `RTHexMapActor.cpp`, che derivano dallo spessore reale |
| Un `.md` dichiara un numero di test diverso | la documentazione è indietro rispetto al codice | fidati del comando di §2 |
| Le unità si muovono **dopo** che lo scenario è finito | piani rimasti appesi ririsolti a ogni turno: corretto in `4e6c2e0` | se ricompare, guarda i **timestamp** — la riga `AUTO-RUN` viene prima di ogni turno visibile |
| Parte uno scenario **diverso** da quello scelto nella tendina | una `rt.Test.Scenario` digitata prima è ancora attiva: le console variable durano quanto il processo dell'editor | `rt.Test.Scenario ""`, oppure leggi `(da: …)` nella riga `AUTO-RUN` del log |

---

## 10. Comandi di riferimento

```bash
# build (editor chiuso)
"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development \
  -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex

# test di un'area
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Scenario+Quit" -unattended -nopause -nullrhi -NoSound

# esiti e conteggio
grep -aE "Test Completed. Result=" Saved/Logs/RefactorTactics.log
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l

# ultimo report di uno scenario
ls -t Saved/RTTests/Movement.Basic/ | head -1
```

In PIE:

```
rt.Debug.DrawCells 1                    rende visibili le regole della mappa
rt.Test.List                            scenari disponibili
rt.Test.Run <ScenarioId>                esegue e scrive il report
rt.Test.DumpResult [ScenarioId]         stampa l'ultimo result.json
rt.Test.Scenario <ScenarioId>           auto-run al prossimo Play, PREVALE su BP_GameMode (vuoto = vale la proprieta)
```

---

## 11. Dove sta cosa

| Percorso | Contenuto |
|---|---|
| `Source/RefactorTactics/Tests/` | Automation Test (`.cpp`) |
| `Source/RefactorTactics/ScenarioHarness/` | **harness** degli scenari (loader, runner, report, console) |
| `Scenarios/` | scenari `.json` versionati |
| `Saved/RTTests/` | report delle esecuzioni (artefatti, non versionati) |
| `Saved/Logs/RefactorTactics.log` | log del motore |
| [`test-manuali-pie.md`](../test-manuali-pie.md) | verifiche interattive, con stato |

Tre cose diverse, tre nomi diversi: **`Scenarios/`** sono i *dati* (cosa deve succedere),
**`ScenarioHarness/`** è il *motore* che li esegue, **`Tests/`** sono i *test C++*.

> **Nota storica**: fino al 2026-08-07 l'harness stava in `Source/.../Test/` e gli scenari in
> `Tests/Scenarios/`. Tre percorsi con lo stesso nome al singolare e al plurale, per tre contenuti diversi:
> bastava questa guida per accorgersene, e la rinomina è costata dieci minuti. Se aggiungi una directory qui
> dentro, controlla che il nome dica **cosa contiene** e non somigli a nessun'altra.

---

## 12. Cosa un test d'integrazione trova e uno unitario no

Il **2026-08-08**, scrivendo uno scenario per un caso che le note PIE davano per scoperto, è emerso un difetto
che nessuno dei test esistenti poteva mostrare. Vale come esempio di quando conviene uno scenario.

**Il caso**: due unità adiacenti si scambiano di posto.

| Livello | Regola | Test | Esito |
|---|---|---|---|
| Resolver | lo scambio **blocca** — come ciclo, `BlockedByCycle` | `HexSim.ResolveSwapBlocked` | ✅ verde |
| Planner | goal occupato → **`NoPath`** | `HexSim.PathAvoidsOccupiedCell` | ✅ verde |

Entrambe corrette, entrambe verdi, **ognuna guardata da sola**. Insieme rendono la regola del resolver
**irraggiungibile**: nessun giocatore può pianificare uno scambio, perché cliccare sulla cella di un nemico
adiacente non produce un percorso. Il test del resolver non se ne accorge perché costruisce i percorsi a mano,
bypassando il planner.

È il pattern **«dato senza consumatore»**: una regola che esiste, funziona, ed è irraggiungibile da chi
dovrebbe attivarla.

**Come cercarlo**: quando due sistemi hanno regole sullo stesso fenomeno — uno che *pianifica* e uno che
*risolve*, uno che *produce* e uno che *consuma* — un test unitario per ciascuno non dice niente su cosa
succede quando lavorano insieme. Serve qualcosa che percorra la catena intera. È esattamente ciò che uno
scenario fa.

**Cosa NON fare**: correggere la regola di iniziativa. Se lo scambio debba essere possibile è una decisione di
design. Il difetto si fissa con una caratterizzazione e si segnala; chi decide, decide.

> ✅ **Deciso il 2026-08-31, ed è la conferma che questa consegna funziona.** `AUTHOR-MOVE-001` — seduta
> d'autore, sincronizzata da [D-295](../../decisions/RT_PDR_00_Decision_Log.md) — dichiara che **lo scambio
> diretto e i cicli chiusi bloccano**, salvo permesso esplicito. Vince quindi la regola del *planner*, e il
> test del resolver è quello che deve cambiare: l'implementazione è
> [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922), che porta anche i due test
> mancanti — il **ciclo chiuso** (nessuno ruota) e il **convoy a coda libera** (tutte avanzano), oggi coperto
> solo per costruzione.
>
> 🔎 **La parte che vale come metodo**: questo paragrafo è del **2026-08-19** (`a3e789d8`), e la domanda è
> rimasta senza risposta **dodici giorni** — **non** perché nessuno la vedesse, è scritta qui sopra, ma
> perché **non aveva un owner cercabile**: nessuna voce in
> `OPEN_DECISIONS.md`, nessuna issue. Un difetto segnalato in un paragrafo di runbook è un difetto che solo
> chi rilegge il runbook ritrova. Segnalare e **dare un owner** sono due gesti diversi, e questo caso ha
> pagato il secondo.
