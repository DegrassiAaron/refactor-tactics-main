# Debug con Visual Studio Community + Unreal Engine 5.8

Guida pratica per compilare, avviare e **debuggare** RefactorTactics. Riferita allo stato attuale
del codice M1 (camera, griglia, input, gamemode).

## 0. Cosa è già pronto
- Il codice M1 **compila** (target `Development Editor`, UE 5.8.1).
- I project file Visual Studio sono **rigenerati** (i nuovi file compaiono nel Solution Explorer).
- Categoria di log dedicata: **`LogRT`**.

---

## 1. Apri e compila in Visual Studio
1. Apri **`RefactorTactics.sln`** (doppio clic) — oppure tasto destro sul `.uproject` → *Generate Visual Studio project files*, poi apri il `.sln`.
2. In alto imposta la configurazione su **`Development Editor`** e piattaforma **`Win64`**.
3. Solution Explorer → tasto destro su **`RefactorTactics`** → *Set as Startup Project*.
4. **Build → Build Solution** (`Ctrl+Shift+B`). Deve finire senza errori.

> Se l'editor UE è aperto, la build da VS fallisce (Live Coding blocca le DLL). Chiudi l'editor prima di una build completa.

---

## 2. Prepara un livello che usa il nostro GameMode
Serve un livello impostato su `RTGameMode` (che genera la griglia e possiede la camera).
1. Editor UE: **File → New Level → Empty Level**. Salvalo come **`L_Prototype`** in `Content/Maps/`.
2. **Edit → Project Settings → Maps & Modes**:
   - *Default GameMode* = **`RTGameMode`**
   - *Editor Startup Map* e *Game Default Map* = **`L_Prototype`**
   - (in alternativa: *Window → World Settings → GameMode Override* = `RTGameMode` sul singolo livello)
3. Salva. Premendo **Play** dovresti vedere una griglia **10×10** vista dall'alto.

---

## 3. Prima sessione di debug (breakpoint + step)
Non servono gli asset di input: la griglia si costruisce al Play, ottimo bersaglio per il debug.
1. In VS apri **`RTGridActor.cpp`** e metti un **breakpoint** (`F9`) sulla riga dentro `BuildGrid()`:
   `const FVector World = URTGridLibrary::CellToWorld(FRTGridCoord(X, Y), Origin, CellSize);`
2. Premi **`F5`** (*Start Debugging*): VS compila se serve e lancia l'editor **con il debugger agganciato**.
3. Nell'editor premi **Play** (`Alt+P`). Il GameMode chiama `BuildGrid()` → **il breakpoint scatta**.
4. Comandi utili mentre sei fermo:
   - **`F10`** step over · **`F11`** step into (entra in `CellToWorld`) · **`Shift+F11`** step out.
   - Passa il mouse su `X`, `Y`, `World`, o usa **Autos / Locals / Watch** (Debug → Windows).
   - **Call Stack** (Debug → Windows → Call Stack): vedi chi ha chiamato `BuildGrid`.
   - **`F5`** continua. Il breakpoint riscatta a ogni cella → usa un **breakpoint condizionale**: tasto destro sul breakpoint → *Conditions* → `X == 5 && Y == 5` (si ferma solo al centro).
5. Per fermare: Stop nell'editor, o in VS **`Shift+F5`** (*Stop Debugging*).

---

## 4. Abilita pan/zoom/selezione (asset Enhanced Input)
Il `RTPlayerController` cerca gli asset in **`/Game/Input/`**. Creali (una volta sola):
1. Content Browser → crea la cartella **`Input`**.
2. Tasto destro → **Input → Input Action**, crea:
   - **`IA_Pan`** → *Value Type* = **Axis2D (Vector2D)**
   - **`IA_Zoom`** → *Value Type* = **Axis1D (float)**
   - **`IA_Select`** → *Value Type* = **Digital (bool)**
3. Tasto destro → **Input → Input Mapping Context**, crea **`IMC_Tactical`**, aprilo e aggiungi i mapping:
   - **IA_Pan**: `D` (nessun modifier); `A` (+ *Negate*); `W` (+ *Swizzle Input Axis Values* = **YXZ**); `S` (+ *Swizzle* YXZ **e** *Negate*).
   - **IA_Zoom**: `Mouse Wheel Axis` (aggiungi *Negate* se lo zoom risulta invertito).
   - **IA_Select**: `Left Mouse Button`.
4. I nomi/percorsi combaciano con quelli attesi dal controller → al prossimo **Play** funzionano.
   Metti un breakpoint in **`RTPlayerController::OnPan`** o **`OnSelect`** per seguirli passo-passo.

> Se vedi nel log `LogRT: Warning: IMC_Tactical non trovato…`, gli asset non sono al percorso/nome giusto.

---

## 5. Live Coding, log e ricompilazioni
- **Output Log** (Window → Output Log): scrivi **`LogRT`** nel filtro per vedere solo i nostri messaggi.
- **Live Coding** (`Ctrl+Alt+F11` nell'editor): ricompila le modifiche ai **`.cpp`** senza chiudere l'editor.
  - **NON** applica: nuovi file, cambi a header/`UPROPERTY`/`USTRUCT`, `Build.cs`/`Target.cs` → per quelle **chiudi l'editor** e fai *Build* completa in VS.
- Per debuggare un editor **già aperto** (non lanciato da VS): VS → **Debug → Attach to Process** → `UnrealEditor.exe`.

---

## 6. Eseguire gli unit test della griglia
- Editor: **Tools → Session Frontend → Automation** → spunta `RefactorTactics.Grid` → **Start Tests** (5 verdi).
- Da riga di comando:
  ```
  "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<...>\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.Grid" -TestExit="Automation Test Queue Empty" -unattended -nullrhi -nopause
  ```

---

## 7. Problemi comuni
| Sintomo | Causa / Fix |
|---|---|
| All'apertura: "missing modules, rebuild?" | Rispondi **Yes**, oppure builda prima in VS (Development Editor). |
| Breakpoint "vuoto/non aggancia" | Config sbagliata: usa **Development Editor** (non Shipping); modulo non buildato / PDB assenti. |
| Modifica a `.cpp` non ha effetto | Hai usato Live Coding su un cambio che richiede rebuild completo (header/nuovo file). Chiudi editor → Build in VS. |
| Build da VS fallisce, editor aperto | Live Coding blocca le DLL → **chiudi l'editor**. |
| Input non risponde | Asset non in `/Game/Input/` con nomi esatti, o *Value Type* errato. Controlla il warning `LogRT`. |
| `.uproject` non si apre / errore `0xFF` | Il file è stato salvato in UTF-16 → deve restare **UTF-8** (vedi memoria/`.editorconfig`). |
