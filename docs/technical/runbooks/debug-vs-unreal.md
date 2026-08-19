# Guida — Debug con Visual Studio + Unreal Engine 5.8.1

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-08 · **Owner**: questo file
> **Copre**: compilare, lanciare, mettere breakpoint, leggere i log.
> **Non copre**: la verifica automatica e la lettura degli esiti — quella è
> [`test-e-diagnosi.md`](test-e-diagnosi.md), che resta la guida operativa dei test.
>
> *Riscritta il 2026-08-08: la versione precedente descriveva la milestone **M1 quadrata** — `ARTGridActor`,
> `URTGridLibrary::CellToWorld`, una griglia 10×10, asset di input da creare a mano in `/Game/Input/` e «5 test
> `RefactorTactics.Grid` verdi». Nulla di tutto ciò esiste più: il substrato quadrato è stato rimosso al
> **CP 7.2**.*

## 0. Cosa serve sapere prima

| Elemento | Valore |
|---|---|
| Motore | **UE 5.8.1**, patch bloccata ([D-022](../../decisions/RT_PDR_00_Decision_Log.md)): upgrade solo fra milestone |
| Target di build | `Development Editor`, `Win64` |
| Categoria di log | **`LogRT`** |
| Topologia | **esagonale** multilivello — `FRTCellId{q, r, Layer}` |
| Asset di input | **non vanno creati a mano**: `ARTPlayerController` costruisce `MappingContext` e le `UInputAction` come oggetti `Transient` a runtime |

---

## 1. Compilare

1. Apri **`RefactorTactics.sln`** (o tasto destro sul `.uproject` → *Generate Visual Studio project files*).
2. Configurazione **`Development Editor`**, piattaforma **`Win64`**.
3. Solution Explorer → tasto destro su **`RefactorTactics`** → *Set as Startup Project*.
4. **Build → Build Solution** (`Ctrl+Shift+B`).

> **Chiudi l'editor UE prima di una build completa.** Live Coding tiene le DLL aperte e il link fallisce con
> `LNK1104`. È un errore che si traveste da problema di codice: se una modifica «non ha effetto» o un test che
> dovrebbe fallire resta verde, il primo sospetto è che **il binario non sia stato riscritto**.

---

## 2. Avviare una partita

Il `GameMode` decide da dove viene la mappa, con `ERTMapSource`:

| Valore | Cosa carica |
|---|---|
| `LevelAsset` | la mappa d'autore assegnata all'`ARTHexMapActor` del livello; se manca, ripiega sull'arena demo |
| `GeneratedDemoArena` | un esagono pieno di raggio `DemoArenaRadius`, pavimento liscio — un fondale giocabile |
| `GeneratedTestArena` | **arena di prova generata**: ostacoli, muri che bloccano la vista, terreno costoso e una piattaforma su un secondo layer |

Per il debug quotidiano `GeneratedTestArena` è la scelta giusta: non dipende da nessun `.uasset` e contiene già
i casi interessanti (blocco LOS, costo variabile, secondo layer).

---

## 3. Breakpoint utili

Non servono asset: la partita si allestisce da codice, quindi i punti di ingresso sono stabili.

| Dove | Perché fermarsi lì |
|---|---|
| `ARTTurnManager` — risoluzione del turno | è l'**unico punto di autorità** (invariante #5): ogni esito passa da qui |
| `URTHexPathLibrary::FindPathAvoiding` | percorso non plausibile, unità che non arriva dove dovrebbe |
| `URTHexCoverLibrary::BlocksTraversal` | un bordo che si attraversa (o non si attraversa) quando non dovrebbe: lo consultano **sia** il path **sia** la LOS |
| `URTActionQueueLibrary` | ordine delle azioni, priorità intra-fase |
| `URTCombatLibrary::EffectiveAttackPower` | numeri di danno inattesi |

**Breakpoint condizionale**: tasto destro sul breakpoint → *Conditions*. Su un ciclo per cella è
l'unico modo di lavorare — per esempio `Cell.Layer == 1` per fermarsi solo sul secondo piano.

Comandi: `F10` step over · `F11` step into · `Shift+F11` step out · `F5` continua ·
Debug → Windows → **Call Stack** per sapere chi ha chiamato.

Per agganciare un editor **già aperto**: Debug → **Attach to Process** → `UnrealEditor.exe`.

---

## 4. Console

| Comando | Cosa fa |
|---|---|
| `rt.Debug.DrawCells` | disegna le celle del grafo tattico |
| `rt.Debug.Pacing` | strumentazione del pacing del turno |
| `rt.Test.List` | elenca gli scenari versionati |
| `rt.Test.Run <ScenarioId>` | esegue uno scenario nel mondo corrente e scrive il report |
| `rt.Test.DumpResult [Id]` | stampa l'ultimo `result.json` |
| `rt.Test.Scenario <Id>` | **variabile**: scenario da eseguire automaticamente all'avvio della partita |

`rt.Test.Scenario` va impostata **prima** di premere Play: la partita normale non parte e al suo posto viene
eseguito lo scenario. Dettagli in [`test-automatico-unreal.md`](../tooling/test-automatico-unreal.md).

---

## 5. Log e Live Coding

- **Output Log** (Window → Output Log): filtra su **`LogRT`**.
- **Live Coding** (`Ctrl+Alt+F11`) ricompila i `.cpp` senza chiudere l'editor. **Non** applica: file nuovi,
  modifiche a header/`UPROPERTY`/`USTRUCT`, `Build.cs`/`Target.cs`. Per quelle: chiudi l'editor e ricompila.

---

## 6. Problemi comuni

| Sintomo | Causa / rimedio |
|---|---|
| All'apertura: «missing modules, rebuild?» | Rispondi **Yes**, o compila prima in VS (`Development Editor`) |
| Breakpoint «vuoto», non aggancia | Configurazione sbagliata (non `Development Editor`), oppure modulo non ricompilato / PDB assenti |
| Modifica a `.cpp` senza effetto | Live Coding usato su un cambio che richiede rebuild completo |
| Build fallisce con `LNK1104` | L'editor è aperto e tiene le DLL: chiudilo |
| Una modifica «non cambia niente», un test che dovrebbe rompersi resta verde | **Sospetta la build, non il test**: il binario potrebbe non essere stato riscritto |
| Input non risponde | Non è un problema di asset: gli `UInputAction` sono creati a runtime. Cerca il warning in `LogRT` |
| `.uproject` non si apre, errore `0xFF` | Salvato in UTF-16: deve restare **UTF-8** |

---

## 7. Dove andare dopo

- Verifica automatica, scenari, lettura degli esiti → [`test-e-diagnosi.md`](test-e-diagnosi.md)
- Architettura del modulo C++ → [`architettura-codice.md`](../architecture/architettura-codice.md)
- Struttura e naming di `Content/` → [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md)
- Verifiche interattive in editor → [`test-manuali-pie.md`](../test-manuali-pie.md)
