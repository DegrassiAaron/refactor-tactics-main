# Guida — Test, scenari e diagnosi

> **Scopo**: come si verifica una modifica in RefactorTactics, dal test più veloce alla partita in editor, e
> come si risale alla causa quando qualcosa non torna.
> **Complementare a** [`debug-vs-unreal.md`](debug-vs-unreal.md), che copre il *debugger* (breakpoint, step,
> Live Coding). Qui si parla di **verifica automatica** e di **lettura degli esiti**.
> **Ultimo aggiornamento**: 2026-08-07

---

## 1. Tre livelli di verifica, e quando usarli

| Livello | Cosa verifica | Quando | Costo |
|---|---|---|---|
| **Automation Test** | regole pure: geometria, path, LOS, danno, ordinamenti, serializzazione | sempre, a ogni modifica delle regole | secondi |
| **Scenario Harness** | il turno *intero*: piano → snapshot → resolver → TurnLog, su più turni | quando tocchi il flusso di gioco o vuoi un caso riproducibile | secondi |
| **PIE (test manuali)** | ciò che nessun test vede: che si **veda** a schermo e che il giocatore **capisca** | prima di chiudere una milestone | minuti, e serve una persona |

La regola pratica: **se puoi verificarlo senza aprire l'editor, fallo senza aprire l'editor.** Le voci PIE
esistono per ciò che resta — leggibilità, ritmo, giudizio — e sono elencate in
[`test-manuali-pie.md`](../design/test-manuali-pie.md).

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
Tests/Scenarios/Movement/Basic.json
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

File in `Tests/Scenarios/<Categoria>/<Nome>.json`. **L'ID è il percorso**: `Movement.Basic` vive in
`Movement/Basic.json`, e un test lo verifica.

```json
{
  "scenarioId": "Movement.Basic",
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
| `scenarioId` | ID gerarchico, **deve** corrispondere al percorso del file |
| `mapRadius` | arena esagonale piena generata da codice (nessun `.umap` da versionare) |
| `hero` | ID stabile dal catalogo: `Hero.Flux` · `Hero.Riva` · `Hero.Bastion` · `Hero.Vektor` |
| `cell` | `[q, r]` oppure `[q, r, layer]` — il layer è opzionale e vale 0 |
| `move` | lista di **waypoint**, come li produrrebbe il giocatore cliccando |
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

Sono **due** di proposito: si aggiungono quando servono, non prima. Il modello dati regge le altre senza
modifiche strutturali.

### Limiti della prima iterazione

Solo **intent di movimento**: niente abilità, niente reazioni. È lo scope dichiarato — le abilità entrano
quando il movimento è stabile.

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

```
rt.Test.Scenario Movement.Basic
```

Con questa variabile impostata, il `GameMode` esegue lo scenario **invece** di allestire la partita normale.
Nessun click, nessun Actor da trascinare in un livello. Si può impostare anche da `DefaultEngine.ini` o da
riga di comando con `-ExecCmds`. Rimettila vuota per tornare a giocare.

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
  "error": "scenario non leggibile: .../Tests/Scenarios/Non/Esiste.json"
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
rt.Test.Scenario <ScenarioId>           auto-run al prossimo Play (vuoto = partita normale)
```

---

## 11. Dove sta cosa

| Percorso | Contenuto |
|---|---|
| `Source/RefactorTactics/Tests/` | Automation Test (`.cpp`) |
| `Source/RefactorTactics/Test/` | **harness** degli scenari (loader, runner, report, console) |
| `Tests/Scenarios/` | scenari `.json` versionati |
| `Saved/RTTests/` | report delle esecuzioni (artefatti, non versionati) |
| `Saved/Logs/RefactorTactics.log` | log del motore |
| [`test-manuali-pie.md`](../design/test-manuali-pie.md) | verifiche interattive, con stato |

> **Attenzione ai nomi simili**: `Tests/` (plurale, radice) sono gli **scenari**; `Source/.../Tests/` sono i
> **test C++**; `Source/.../Test/` (singolare) è l'**harness** che esegue gli scenari.
