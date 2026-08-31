# Sonda di movimento nell'editor — #711 (`T6` della Trial, prima fetta di `TD 0.5`)

**Data**: 2026-08-31
**Base**: `origin/main` @ `4f890610`
**Checkout**: `D:/Repositories/refactor-tactics-technical-designer/refactor-tactics-main`
**Branch**: `feat/711-sonda-movimento-editor`, aperto da `main`
**Dipende da**: PR **#1877** (`fix/graybox-tolerance-unity-shadow`) — senza, il modulo Editor non compila

---

## 1. Perche' questa slice e non un'altra

Le tre issue di percezione aperte dall'audit del 2026-08-30 — #1753, #1754, #1755 — sono **chiuse e in
`main`**, insieme a #1705. Il perimetro TD aperto contava undici issue, e la scelta e' stata fatta su tre
criteri misurati, non su preferenza:

| | #711 (questa) | #1625 (`T1` playback) | #1682 (`L6` workspace) |
|---|---|---|---|
| il dato che la UI compila esiste | ✅ `ReachableCells` + `FromCell` | 🔴 **il ponte manca** | 🟡 |
| AC misurabili | ✅ con verifica di mutazione | ⚠️ uno aperto su 13 categorie | 🟡 |
| verificabile senza PIE | ✅ in gran parte | ❌ classe C | 🟡 |
| seduta PIE gia' collocata | ✅ `U26` | ⚠️ `U26` senza voce | ❌ |

🔴 **Il gap di #1625, misurato e lasciato scritto qui perche' chi la prende non lo riscopra.** Quella issue
dichiara *«il trasporto e' costruito, questa e' UI sopra un core esistente»*. E' vero per il **seek** e falso
per la **sorgente**:

```text
FRTReplayViewModel::Open(const FString& ReplaysRoot, const FGuid& MatchId)
  └── delega a URTReplayPlayerLibrary::OpenArchive → FRTReplaySession   ← archivio su DISCO
URTScenarioAuthoring::GetLastRunLog() -> TArray<FRTScenarioLogEntryView> ← DTO in MEMORIA
```

Non esiste un `Open` che accetti una sessione in memoria, e i due tipi di voce non coincidono. Le strade sono
tre — far registrare al run dello scenario un archivio vero e riaprirlo; aggiungere al ViewModel una seconda
porta d'apertura; oppure la terza, che il guardrail *«no second simulator»* della issue stessa vieta. **E' una
decisione architetturale, e va presa prima di scrivere codice**, non durante.

⚠️ #1625 porta anche un criterio **da recepire nella DoD** e non ancora recepito, lasciato da #1754: *in
`Team N`, durante il playback, un nemico mai visto non viene rivelato*.

---

## 2. Il difetto di build trovato prima di poter cominciare

⚠️ **`main` @ `4f890610` non compilava il modulo Editor.** Non e' una deduzione: riprodotto con `git stash`,
albero pulito, nessuna modifica.

```text
RTGrayboxMeshTests.cpp(58): constexpr float Tolerance = 0.1f;   <- namespace ANONIMO
Rotator.h(640): TRotator<T>::Equals(const TRotator<T>&, T Tolerance) const
-> C4459, che in questo progetto e' un errore
```

In compilazione separata i due non si vedono. Nella unity build finiscono nella stessa unita' di traduzione —
`RTScenarioPerspectiveTests.cpp` (#1754, mergiato la sera prima) istanzia `TRotator<double>::Equals` — e il
parametro dell'engine nasconde la costante del test.

🔴 **E' la terza volta che questa forma di difetto si presenta in due giorni.** L'audit del 2026-08-30 ha
registrato la `SaveAssetPackage` duplicata fra i due commandlet; qui e' un name-shadow; e durante **questa**
passata ne e' stata evitata una quarta prima che mordesse — l'helper `FindReachable` del nuovo file di test
aveva lo stesso nome di quello introdotto in `RTHexSimLibrary.cpp`, stesso modulo, entrambi in namespace
anonimo. Rinominato in `FindInProbeSet` **prima** che UBT li raggruppasse.

∴ la regola che vale piu' del singolo fix: **un identificatore in un namespace anonimo condivide lo spazio dei
nomi con ogni simbolo che finisca nella stessa unita' di traduzione**, e chi lo rompe non e' chi lo ha scritto
— e' chi aggiunge il file successivo.

Corretto su un branch **separato** (PR #1877), perche' blocca chiunque ricompili l'Editor.

---

## 3. Il gap che la sonda colma, misurato sul codice

`ClassifyWaypointCell` risponde con quattro valori: `Ok`, `NotOnMap`, `BlocksMovement`, `Occupied`. Per il suo
chiamante e' completo, e la sua doc lo dichiara: *«`Ok` significa che la cella in se' va bene: se il percorso
composito fallisce comunque, il motivo e' il budget»*.

⚠️ **Per una sonda, `Ok` di fronte a una cella esclusa non e' una risposta.** E il test esistente lo pinna
gia', senza che nessuno lo avesse letto come un gap:

```cpp
// RTHexSimTests.cpp:1082, precedente a questa passata
TestTrue(TEXT("cella libera -> Ok (un eventuale rifiuto e' questione di budget)"),
    URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, FRTCellId(0, 2, 0)) == ERTHexWaypointReason::Ok);
```

Sotto quell'unico `Ok` stanno **due** situazioni che mandano il designer in due posti opposti:

| | cosa fa il designer | dove sta il difetto |
|---|---|---|
| **fuori budget** | alza il movimento, o abbassa il costo del terreno | nel profilo o nella superficie |
| **nessuna strada** | apre un varco | **nella mappa** |

Dirgli «ti manca movimento» davanti a un'isola lo manda a cambiare un numero che non aprira' quella cella.

---

## 4. Cosa e' stato aggiunto, e dove passa il confine

### Runtime — `Source/RefactorTactics/Turn/` (le due regole)

| Simbolo | Cosa |
|---|---|
| `ERTHexProbeExclusion` | `Reachable` · `NotOnMap` · `BlocksMovement` · `Occupied` · `OutOfBudget` · `NoRoute` |
| `URTHexSimLibrary::ProbePathTo` | il percorso **risalendo `FromCell`**, non una seconda ricerca |
| `URTHexSimLibrary::ClassifyProbeCell` | il motivo: delega i tre di cella, separa gli altri due |

🔑 **Nessun pathfinder nuovo, e la prova e' nella forma della chiamata.** La distinzione fra «fuori budget» e
«nessuna strada» si ottiene girando la domanda **alla stessa** `URTHexPathLibrary::FindPathAvoiding` che
`FindPathForUnit` usa, con `MaxCost == 0` (illimitato) e **gli stessi ostacoli dinamici**. Se a budget
illimitato un percorso esiste, allora mancava solo il budget. Ecco perche' quella funzione e' finita in
`RTHexSimLibrary.cpp` e non in un file nuovo: `BlockedCellsFor` e' privata li', e un file nuovo avrebbe dovuto
**riscrivere il set degli ostacoli** — cioe' esattamente la seconda risposta che si stava evitando.

🔑 **`FromCell` era gia' pagato.** `RTHexSim.h:88` lo porta dal facing (CP 13.5): il Dijkstra ha gia' calcolato
il predecessore di ogni cella, e la sonda lo **legge** invece di ricercare. L'oracolo del test non e' un
percorso scritto a mano: e' `FindPathForUnit`.

### Editor — `Source/RefactorTacticsEditor/` (nessuna regola)

| File | Cosa |
|---|---|
| `Public/RTHexProbeReadout.h` · `Private/RTHexProbeReadout.cpp` | cosa il pannello **scrive**, e quando richiede |
| `Public/RTHexHoverGate.h` · `Private/RTHexHoverGate.cpp` | il gate dell'hover, **estratto** da #1755 |
| `Private/Tools/RTHexProbeTool.{h,cpp}` | il settimo tool: un guscio |
| `Private/Tests/RTHexProbeReadoutTests.cpp` | 4 test |
| mode · commands | il tool entra nella palette |

⚠️ **Il divieto di #711 diceva *«`Source/RefactorTacticsEditor/` non ha test (misurato)»*, e quella misura e'
scaduta**: sono **sette** file di test oggi. La regola resta valida, la giustificazione cambia — la logica non
va li' perche' sarebbe **una seconda risposta alla stessa domanda**, non perche' il modulo sia intestabile. E'
la sesta volta che il repository incontra questa deduzione, e l'epic #1105 ne registra gia' cinque.

### 🔵 L'estrazione del gate, e perche' non e' scope creep

La sonda aveva bisogno di `ShouldRequery` **identica** a quella di #1755. Ricopiarla avrebbe dato due cancelli
con la stessa regola. Il corpo si e' spostato in `RTHexHover::ShouldRequery`; `RTHexLos::ShouldRequery`
conserva firma e doc e **delega**. I quattro test di #1755 non sono stati toccati e restano verdi: sono
l'oracolo dell'estrazione, non un danno collaterale.

### ⚠️ Due scelte che deviano da quanto ci si aspetterebbe

1. **La sonda non tiene una cache del ventaglio.** `FRTHexSnapshot` porta un puntatore alla mappa che non e'
   una `UPROPERTY`, e il suo owner dichiara che non va conservata oltre la fase che la produce. Ricostruire a
   ogni domanda filtrata dal gate costa un Dijkstra su un budget di movimento, e **elimina la regola di
   invalidazione** — che sarebbe stata la seconda copia di `IsSnapshotStale`. Il criterio DoD *«ricalcolato
   sulla revisione, non su un refresh a tempo»* e' soddisfatto per costruzione: non c'e' niente da invalidare.
2. **`Budget` nel pannello e' in sola lettura.** #711 chiede profilo e budget *«da dati reali, non da costanti
   d'editor»*: si sceglie **quale eroe** (`HeroId`), e `URTHeroData::MovePoints` da' il numero — la stessa
   fonte da cui `ARTUnit` prende `MoveRange` e da cui il Composer prende il budget della sua anteprima. Un
   `Budget` scrivibile avrebbe risposto alla domanda sbagliata.

---

## 5. Referto di build e test

### Build

```text
comando   : Build.bat RefactorTacticsEditor Win64 Development
            -Project=...\RefactorTactics.uproject -WaitMutex -NoHotReloadFromIDE
esito     : Result: Succeeded
```

⚠️ **`-NoHotReloadFromIDE` e' un bypass dichiarato.** Il mutex Live Coding e' globale sull'eseguibile
dell'engine e vale fra checkout diversi; qui `Binaries/` e `Intermediate/` sono disgiunti da quelli delle
altre copie, quindi non c'e' nulla da proteggere.

### Test — `rt-suite`, con le sue invarianti

```text
comando   : ./scripts/rt-suite.ps1 -Filter "RefactorTactics.MovementProbe"
            -LogName rt-suite-711.log -WaitMinutes 40
verdetto  : VALIDA
HEAD      : 61834e09   albero ae48caf4
esito     : 11/11 completati, 0 fallimenti
durata    : 00:20        exit code 0
```

```text
comando   : ./scripts/rt-suite.ps1 -LogName rt-suite-711-full.log -WaitMinutes 40
verdetto  : VALIDA
HEAD      : 61834e09   albero ae48caf4
esito     : 1508/1508 completati, 1 fallimento
durata    : 01:27        exit code 1
```

⚠️ **L'unico rosso non e' di questa passata, e la prova non e' un ragionamento.** E'
`RefactorTactics.Startup.ShippedGameModeSetsUpTheAdvertisedMatch` (`RTShippedGameModeTests.cpp:114`), e
argomentare che «una sonda di movimento non c'entra con il GameMode spedito» sarebbe stato plausibile e
insufficiente. Rimisurato sulla **baseline** — `fix/graybox-tolerance-unity-shadow` @ `668592e5`, cioe' `main`
piu' il solo fix di build, **senza** una riga di #711 — ricompilata e rilanciata:

```text
comando   : ./scripts/rt-suite.ps1 -Filter "RefactorTactics.Startup" ...
verdetto  : VALIDA
HEAD      : 668592e5
esito     : 12/12 completati, 1 fallimento   <- lo STESSO test, lo stesso messaggio
```

✅ **Ed e' gia' posseduto e gia' corretto**: la PR **#1862** (aperta) lo attribuisce al commit `fb190a94`,
che entrando in `main` col merge di `wip/938` ha sovrascritto `BP_GameMode.uasset` e **disfatto** il fix di
#1069 — quello che questo test pinnava. 🔵 E' lo stesso `fb190a94` che l'audit del 2026-08-30 aveva segnalato
come *«binario senza owner accertato»*: l'owner ora c'e'.

∴ per questa slice: **1507/1508**, con il rosso preesistente dichiarato, misurato sulla baseline e attribuito.

⚠️ **Il binario e' stato ricostruito dopo il ritorno dalla baseline** (`07:43:09`): un checkout di branch
lascia un DLL che appartiene all'altro albero, e una misura fatta li' sopra sarebbe verde su codice che non e'
quello che si sta guardando.

| test | cosa difende |
|---|---|
| `HoverPathComesFromTheReachableSet` | il percorso viene da `FromCell` e **coincide** con `FindPathForUnit` |
| `OutOfBudgetIsNotTheSameAsNoRoute` | i due motivi non si accorpano — e pinna che il vecchio vocabolario dice `Ok` |
| `NoRouteWhenTheGraphIsCut` | un'isola non e' «ti manca movimento» |
| `ExclusionKeepsTheExistingVocabulary` | i tre motivi di cella sono **delegati**, uno per ragione |
| `BudgetChangesTheSetAndTheAnswer` | cambiando budget cambiano set **e** risposta |
| `SurfaceEditInvalidatesTheSetByRevision` | dipinta una superficie, il set si restringe; la revisione lo dice |
| `NoSelectedUnitMeansNoRoute` | nessuna unita' non e' «fuori budget» del budget di nessuno |
| `ReadoutSaysCostAgainstBudget` | costo **su** budget, e i passi non sono il costo |
| `EveryExclusionGetsItsOwnLine` | cinque motivi, cinque frasi diverse, nessuna vuota |
| `ReadoutWithoutAUnitPromisesNothing` | il pannello appena aperto non promette |
| `RequeryOnlyWhenTheHoveredCellChanges` | l'hover e' event-driven, non per-fotogramma |

⚠️ **Due dei tre criteri d'onesta' su come sono nati.** `SurfaceEditInvalidatesTheSetByRevision` e
`NoSelectedUnitMeansNoRoute` sono **passati alla prima esecuzione**: non hanno visto il rosso, e per la
disciplina TDD sono test di caratterizzazione, non test-first. Vengono dichiarati invece che presentati come
gli altri nove — che il rosso lo hanno visto, ciascuno per la propria ragione. ✅ Il primo dei due si e' pero'
rivelato **discriminante**: cade sotto la mutazione qui sotto.

### Verifica di mutazione (criterio esplicito della DoD)

Mutazione applicata a `ClassifyProbeCell`: `MaxCost = 0` (illimitato) → `MaxCost = Unit->MoveBudget`. Cioe' la
domanda «esiste una strada a qualunque costo?» diventa «esiste una strada entro il budget?», e la distinzione
fra i due motivi collassa.

```text
falliscono : OutOfBudgetIsNotTheSameAsNoRoute
             BudgetChangesTheSetAndTheAnswer
             SurfaceEditInvalidatesTheSetByRevision
restano OK : gli altri otto
```

✅ **Cadono esattamente i tre che asseriscono `OutOfBudget`, e nessun altro** — `NoRouteWhenTheGraphIsCut`
resta verde, ed e' giusto: un grafo tagliato e' `NoRoute` in entrambe le versioni.

⚠️ **Ripristinato E ricostruito**, in quest'ordine e con la seconda verificata: un binario mutato lasciato sul
disco farebbe dichiarare verde codice che non esiste. Dopo il ripristino: **11/11, exit 0**, DLL delle
`07:32:04`.

### ⛔ NOT RUN

- 🔵 **La suite completa ERA in questa sezione, e non ci sta piu'**: e' stata eseguita con `rt-suite` appena
  l'autore ha liberato il motore — vedi il referto qui sopra, **VALIDA** in entrambe le forme. Durante la
  costruzione il motore era conteso (`refactor-tactict-dev` teneva il mutex per `RefactorTactics.Map.Dependency`),
  ed e' la ragione per cui i cicli TDD sono stati misurati con automation dirette.
- **Le tre voci PIE**, che sono `⏳` e stanno in `U26`. La leggibilita' del ventaglio, il percorso che segue il
  cursore e la fluidita' dell'hover **non sono osservabili headless**, ed e' la ragione per cui la seduta
  esiste.

---

## 6. Stato

**DONE**
- Le due regole nel runtime, con 7 test; il readout d'editor, con 4; il settimo tool nella palette.
- Gate dell'hover estratto e condiviso (`RTHexHover`); i 4 test di #1755 restano verdi.
- Tre voci PIE aperte in `test-manuali-pie.md` (conteggio canonico: **178 → 181**, `84 → 87` aperte) e
  collocate in `U26`, che le riceve in `verifies`.
- Verifica di mutazione eseguita, mirata, e binario ripristinato.
- PR #1877: il modulo Editor torna a compilare.

**NOT STARTED**
- La seduta `U26`.

**RISCHI E APERTI**
- ⚠️ `U26` **non si chiude** con questa PR: `done_when` cita anche le voci di **#622**, che non esistono
  ancora. Chi esegue la seduta chiude il verdetto sulla sonda, non la seduta — e ora `done_when` lo dice.
- ⚠️ Questo branch **contiene** il commit di #1877. Se quella PR entra prima, il rebase e' pulito; se entra
  dopo, i due commit sono lo stesso contenuto e uno dei due sparisce nel merge.
- 🔴 La sonda rappresenta **il primo passo di un turno, da fermo, senza status**. `Action.Root` azzera il
  budget e `Action.Slow` alza il costo per cella (CP 4.7): un'unita' d'editor non ha una partita da cui
  leggerli. E' dichiarato sulla classe del tool, e va detto anche a chi la usa in seduta.

---

## 7. Prossimo passo esatto

1. **#1862 prima di questa PR, se possibile**: e' l'unico rosso della suite, e mergiarla toglie l'unica
   ragione per cui `rt-suite` esce `1` su questo branch.
2. `U26` in seduta, sulle tre voci — e su una mappa che abbia **davvero** una superficie costosa, un blocco e
   una zona irraggiungibile: senza quelle tre condizioni la sonda non ha modo di mostrare motivi diversi.
3. Per chi prende **#1625**: la decisione di §1 prima del codice, e il criterio di #1754 recepito nella DoD.
