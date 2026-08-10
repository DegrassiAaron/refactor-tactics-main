# Guida — Test, scenari e diagnosi

> **Scopo**: come si verifica una modifica in RefactorTactics, dal test più veloce alla partita in editor, e
> come si risale alla causa quando qualcosa non torna.
> **Complementare a** [`debug-vs-unreal.md`](debug-vs-unreal.md), che copre il *debugger* (breakpoint, step,
> Live Coding). Qui si parla di **verifica automatica** e di **lettura degli esiti**.
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-08 (verificata contro `Scenarios/` e `ScenarioHarness/`:
> cinque scenari, quattro comandi console, percorsi corretti — nessuna correzione necessaria).
> La **spec** dell'harness — schema, assertion, esiti, `StateHash` — è
> [`test-automatico-unreal.md`](test-automatico-unreal.md).

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
[`test-manuali-pie.md`](test-manuali-pie.md).

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
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp \
  | tr -d '"' | sort -u | wc -l
```

Questo comando è la fonte: la documentazione che dichiara un numero diverso è indietro, non il contrario.

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

---

## 4. Scrivere uno scenario

File `.json` sotto `Scenarios/`, in **qualunque** sottocartella. L'ID lo dichiara il file, non il percorso:
spostare uno scenario non ne cambia l'identità, e a trovarlo ci pensano i **tag**. Il modello, il perché e la
tabella di redirect stanno in [`scenario-index-e-tag.md`](scenario-index-e-tag.md).

```json
{
  "scenarioId": "Movement.Basic",
  "tags": ["movement", "core", "animation", "flux", "bastion"],
  "version": 1,
  "seed": 0,
  "mapRadius": 3,

  "units": [
    { "id": "A1", "hero": "Hero.Flux",    "team": 0, "cell": [-2, 0, 0] },
    { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [2, 0, 0] }
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
| `tags` | *(opzionale)* parole per cui filtrare nell'Editor: tipologia, lente, personaggio. Vedi [`scenario-index-e-tag.md`](scenario-index-e-tag.md) |
| `mapRadius` | arena esagonale piena generata da codice (nessun `.umap` da versionare) |
| `cells` | *(opzionale)* celle da modificare: `blocksMovement`, `blocksLineOfSight`, `moveCost` |
| `hero` | ID stabile dal catalogo: `Hero.Flux` · `Hero.Riva` · `Hero.Bastion` · `Hero.Vektor` |
| `cell` | `[q, r]` oppure `[q, r, layer]` — il layer è opzionale e vale 0 |
| `move` | lista di **waypoint**, come li produrrebbe il giocatore cliccando |
| `ability` | `ActionId` dell'abilità (`Flux.ArcPulse`) — per **ID**, non per indice |
| `target` | ID di scenario del bersaglio; obbligatorio con `ability` |
| `reaction` | `ActionId` della reazione che l'unità **arma** per il turno — nessun bersaglio |
| `expect` | assertion; **almeno una**, altrimenti lo scenario passerebbe sempre |

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
| `Combat.BasicAttack` | Flux colpisce Bastion: 120 → 98 HP. Il primo scenario che verifica un **danno** |
| `Combat.BlockedByWall` | stesso attacco con un muro in mezzo: il colpo **non parte** |
| `Combat.SplashHitsAlliesNotSelf` | l'area colpisce i vicini **e** l'alleato, ma non chi la lancia |
| `Combat.LineHitsThrough` | la linea colpisce chi sta in mezzo, non solo il bersaglio |
| `Combat.CounterStrikesBack` | una reazione **armata** scatta e colpisce chi ha colpito |
| `Combat.NoCounterWhenUnarmed` | senza armarla, la stessa reazione non scatta |
| `Movement.SwapRejectedByPlanning` | **caratterizzazione**: due unità adiacenti *non* si scambiano di posto, perché la pianificazione rifiuta un percorso verso una cella occupata (vedi §12) |

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
> `Combat.CounterStrikesBack` lo scudo 15 assorbe i 24 in arrivo e Flux resta a 81, non a 66. È la ragione
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
[`scenario-index-e-tag.md`](scenario-index-e-tag.md).

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
| Le unità si muovono **dopo** che lo scenario è finito | piani rimasti appesi ririsolti a ogni turno: corretto in `4e6c2e0`, ma se ricompare, guarda i **timestamp** — la riga `AUTO-RUN` viene prima di ogni turno visibile |
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
| [`test-manuali-pie.md`](test-manuali-pie.md) | verifiche interattive, con stato |

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
| Resolver | lo scambio **è consentito** | `HexSim.ResolveSwapAllowed` | ✅ verde |
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
