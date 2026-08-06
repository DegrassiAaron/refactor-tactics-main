# Spec — Terreni esagonali (E8, CP 8.1)

> Brainstorming del **2026-08-06**. Fonti: [`RT_TerrainCatalog_v0.1.md`](balance/RT_TerrainCatalog_v0.1.md)
> (catalogo canonico) · [`v0.1-issue-plan.md`](v0.1-issue-plan.md) §E8 (issue `#64`, CP 8.1) · codice
> esistente (`Map/RTHexCellData.h`, `Pathfinding/RTHexPathLibrary.*`, `Combat/RTHexCombatLibrary.*`,
> `Ability/RTActionDef.h` + `RTCatalogLibrary.*`). Superate le regole di
> [`spec-terreni.md`](spec-terreni.md) (grid quadrato, rimosso in `7d8889b`).
> Autorità: subordinato al piano canonico e al catalogo terreni.

## 1. Obiettivo e scope

Portare gli 8 terreni del catalogo (`Terrain.Floor/Rough/ShallowWater/Fire/Conductive/Smoke/Ice/HighGround`)
sulle celle esagonali, come **dati**, non come `switch` C++. Scope = CP 8.1 (issue `#64`): identità e
comportamento di movimento/LOS/targeting dei terreni. **Fuori scope**: il sistema di stati con
durata/scadenza in Cleanup (CP 8.2, `#65`), la propagazione elettrica (CP 8.3, `#66`), le interazioni
fuoco/acqua e l'ignite dinamico (CP 8.4, `#67`), le azioni ambientali (CP 8.5, `#68`). Dove CP 8.1 deve
dichiarare un effetto che dipende da questi (es. `Wet` applicato ma non ancora scaduto/consumato da nessuno),
si dichiara il limite — stesso pattern già usato da CP 6.4/6.5 (Bastion/Vektor) per le dipendenze da E5/E9.

## 2. Modello dati

### 2.1 `FRTTerrainDef` — `Terrain/RTTerrainData.h`

```
USTRUCT(BlueprintType)
struct FRTTerrainDef
{
    ERTHexSurface Surface = ERTHexSurface::Floor;
    int32 MoveCost = 1;
    bool bBlocksDashCharge = false;
    bool bBlocksLineOfSight = false;
    bool bConductsElectricity = false;
    int32 MaxTargetingRangeThrough = 0;      // 0 = nessun limite
    TArray<FRTActionEffectSpec> OnEnterEffects; // riuso del vocabolario azioni (Damage/Status)
};
```

Riusa `FRTActionEffectSpec`/`ERTActionEffect` (`Turn/RTActionEvent.h`) invece di inventare un secondo
linguaggio di effetti: `Fire.OnEnterEffects = { {Damage,10}, {Status,TAG_Status_Burning,2} }`,
`ShallowWater.OnEnterEffects = { {Status,TAG_Status_Wet,0} }` (durata 0 = "finché sulla cella", CP 8.2
decide la semantica esatta di scadenza).

### 2.2 `URTTerrainLibrary` — `Terrain/RTTerrainLibrary.*`

Stesso schema di `URTCatalogLibrary` (azioni):
- `static TArray<FRTTerrainDef> GetTerrainCatalog()` — 8 righe letterali, una per terreno.
- `static FRTTerrainDef FindTerrainDef(ERTHexSurface Surface)` — default `Floor` se non trovato (non
  dovrebbe succedere: il validator lo impedisce).
- `static TArray<FString> ValidateTerrainCatalog()` — id duplicati, costi negativi, target range negativo.

**Perché non un `UPrimaryDataAsset` per terreno** (come il vecchio `URTTerrainData` quadrato, rimosso in
`7d8889b`): il paint tool esagonale (`RTHexPaintTool`) già lavora per **enum + valori**, non per riferimenti
ad asset per-cella. Un catalogo letterale in C++, validato a test, è coerente con `FRTActionDef` (stesso
"file coinvolti" nel piano issue: `RTTerrainData.h`/`RTTerrainLibrary.*` rispecchia `RTActionDef.h`/
`RTCatalogLibrary.*`) ed è git-diffabile senza gestione asset in Content Browser.

### 2.3 `ERTHexSurface` — relabel in place

`Map/RTHexCellData.h`. Stessi ordinali (**nessun bump di `FormatVersion`**, resta 2):

| Ordinale | Oggi | Diventa |
|---|---|---|
| 0 | `Normal` | `Floor` |
| 1 | `Water` | `ShallowWater` |
| 2 | `Mud` | `Rough` |
| 3 | `Fire` | `Fire` (invariato) |
| 4 | `Electrified` | `Conductive` |
| 5 | `Ice` | `Ice` (invariato) |
| 6 | `Void` | `Void` (lasciato dichiarato, **inutilizzato** — non è fra gli 8 del catalogo, nessuna cella
  salvata lo perde) |
| 7 (nuovo) | — | `Smoke` |
| 8 (nuovo) | — | `HighGround` |

Rinominare l'identificatore C++ di un enumerator non cambia l'ordinale serializzato, ma rompe la
compilazione dei call site esistenti: vanno aggiornati (non solo rinominati meccanicamente — vedi §4).

### 2.4 `FRTHexCellData` — cosa NON cambia

`Cell.MoveCost` / `bBlocksMovement` / `bBlocksLineOfSight` **restano i valori autorevoli** letti da
`RTHexPathLibrary::GraphNeighbors` e da tutti i 21 call site esistenti: sono la personalizzazione per-cella
che un level designer può ancora divergere dal default del terreno (es. un tratto di `Floor` leggermente più
costoso vicino a un imbuto tattico). Il catalogo terreni è la fonte per i campi **nuovi** che non hanno oggi
un equivalente per-cella (`bBlocksDashCharge`, `OnEnterEffects`, `MaxTargetingRangeThrough`,
`bConductsElectricity`) e per i **default** che il paint tool propone quando si seleziona una `Surface`
(`RTHexPaintTool::Properties.MoveCost/bBlocksMovement` si precompilano da `FindTerrainDef(Surface)` invece
che da `1`/`false` statici — la libertà di override resta, ma il default arriva dai dati).

## 3. Gli 8 terreni (valori dal catalogo)

| Surface | MoveCost | bBlocksDashCharge | bBlocksLineOfSight | OnEnterEffects | Note |
|---|---:|---|---|---|---|
| `Floor` | 1 | no | no | — | |
| `Rough` | 2 | **sì** | no | — | |
| `ShallowWater` | 2 | no | no | `Status.Wet` | `bConductsElectricity=true` |
| `Fire` | 2 | no | no (§5) | `Damage 10` + `Status.Burning(2)` | |
| `Conductive` | 1 | no | no | — | `bConductsElectricity=true`, **non** applica Wet |
| `Smoke` | 1 | no | no | `Status.Obscured` | `MaxTargetingRangeThrough=2` |
| `Ice` | 1 | no | no | — | sliding, vedi §5 |
| `HighGround` | 1 | no | no | — | "bonus visuale" **non implementato** in CP 8.1: nessun
  meccanismo di vista/quota esiste ancora da consumarlo. Dichiarato come limite in PR, stesso pattern di
  `PushResistance` di Bastion. |

## 4. Call site da aggiornare (rename enum)

Riferimenti a `ERTHexSurface::Normal/Water/Mud/Electrified` da correggere al nuovo nome (stesso ordinale,
solo l'identificatore cambia):

- `Tests/RTHexMapTests.cpp`, `Tests/RTHexMovementIntegrationTests.cpp` — fixture di test.
- `Turn/RTMatchSetupLibrary.cpp` (arena demo: fascia "Mud" costo 3 → resta un valore **per-cella**
  legittimo, diverso dal default `Rough` costo 2 del catalogo; solo il nome enum cambia).
- `RefactorTacticsEditor/Private/RTHexEditorClick.cpp` (switch colori overlay — guadagna i casi
  `Smoke`/`HighGround`).
- `RefactorTacticsEditor/Private/Tools/{RTHexPaintTool,RTHexSelectTool,RTHexFillTool}.h` (default
  `ERTHexSurface::Normal` → `::Floor`).

## 5. Hook di risoluzione

### 5.1 Dash/Charge bloccati da `Rough`

`Turn/RTHexSimLibrary.cpp:291` (`LinearDashPath`): il check `Data->bBlocksMovement` guadagna
`|| URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge`. Il movimento normale (`Action.Move`,
pathfinding via `RTHexPathLibrary`) **non** è affetto: `Rough` resta attraversabile a piedi, costa solo di
più (`MoveCost=2`, già gestito dal costo esistente).

### 5.2 Ice — sliding (scelto: implementato ora, non rimandato)

**Solo per il Move normale**, non per lo Scatto (vedi limite sotto). Il Move risolve con
`URTHexSimLibrary::ResolveHexPaths(TArray<TArray<FRTCellId>>) -> TArray<FRTHexMoveResult>`
(`RTHexSimLibrary.h:127`): microstep sincroni, collisioni simultanee **già** risolte in modo
order-independent (destinazione contesa → contendenti fermi da lì; cella di un'unità ferma → bloccata;
scambio diretto → consentito). Lo scivolamento si inserisce **prima** di questa chiamata: se il percorso
troncato al budget (da `FindPathForUnit`/`BuildCompositeHexPath`) termina su `Ice` e il budget residuo
dell'unità è **≥ 2**, si appende una cella nella direzione dell'ultimo passo (stessa direzione
dell'ingresso), verificando solo che la cella esista e non blocchi il movimento (`bBlocksMovement`) — **non**
serve controllare l'occupazione qui: il path esteso entra comunque nel microstep di `ResolveHexPaths`, che
gestisce occupazione e collisioni con lo stesso meccanismo di qualunque altro passo pianificato. **Nessun
limite da dichiarare per il Move**: due unità che scivolano verso la stessa cella libera vengono gestite dal
resolver esistente esattamente come due unità che vi si muovono normalmente.

**Limite dichiarato**: lo Scatto (`LinearDashPath`) che termina su `Ice` **non** innesca lo scivolamento in
CP 8.1. `LinearDashPath` non passa dal microstep condiviso — controlla i bloccati contro uno snapshot
**statico** catturato a inizio fase (`BlockedCellsFor`), quindi estendere il suo path con una cella extra
non avrebbe la stessa garanzia di correttezza sotto collisione simultanea del Move. Introdurlo richiederebbe
far partecipare anche lo Scatto al resolver condiviso — fuori scope per CP 8.1, dichiarato in PR.

**Decisione confermata (2026-08-06, in review del Task 6)**: la soglia "≥2 MP residui" resta **fissa**,
letta alla lettera dal catalogo — **non** verifica se il budget residuo copra il costo reale della cella di
scivolamento. Su una mappa dove il ghiaccio è seguito da una cella a costo >2 (già possibile: la fascia demo
di `RTMatchSetupLibrary.cpp` usa costo 3), un'unità può quindi spendere 1-2 MP oltre il proprio budget.
**Intenzionale**: coerente con la lettera del catalogo e con altre abilità di mobilità rapida (Sprint, Dash)
che già concedono margine oltre le regole normali di movimento come parte della loro identità. Non è un
difetto da correggere.

### 5.3 Fire — danno e Burning all'ingresso

Quando una risoluzione di movimento (Move/Dash) termina **o attraversa** una cella `Fire`, si applicano gli
`OnEnterEffects` (Danno 10 + `Status.Burning` 2 turni) traducendoli in `FRTActionEvent` con la stessa
libreria di traduzione usata dalle azioni (`Turn/RTActionEffectLibrary`), estraendo una funzione
"lista di `FRTActionEffectSpec` + `TargetUnitId` → eventi" che non richieda una `FRTActionInstance`
completa (oggi `ProduceEvents` la richiede). Il dettaglio della funzione estratta è del piano
d'implementazione, non di questo spec.

### 5.4 Smoke — cap di targeting

`Combat/RTHexCombatLibrary.cpp::CollectHexAttacks`: per ogni intento, se la linea attaccante→bersaglio (le
celle già calcolate per la linea di tiro) attraversa **almeno una** cella `Smoke`, la portata effettiva per
quell'intento diventa `min(RangeCells, 2)` invece di `RangeCells`. Non cambia la logica di blocco LOS
esistente (muri/coperture): è un cap aggiuntivo, indipendente.

### 5.5 Fire — LOS

`bBlocksLineOfSight = false` nel catalogo (deciso: "parziale" del PDF letto come "non bloccante" finché non
esiste un sistema di LOS graduata). Nessun nuovo meccanismo.

## 6-bis. Limite scoperto in review (Task 7, 2026-08-06): `Wet`/`Obscured` sono inerti a runtime

`ShallowWater.OnEnterEffects` e `Smoke.OnEnterEffects` (Task 3) dichiarano `Status.Wet`/`Status.Obscured`
con `StatusDuration = 0` (letto come "finché sulla cella", §2.1) — ma `ARTUnit::ApplyStatus` rifiuta
silenziosamente ogni durata `<= 0` (`if (Turns <= 0) { return; }`, nessun log, nessun crash). Con l'hook di
applicazione ora attivo (Task 7, `ApplyTerrainOnEnterEffects`), questo significa che **entrare in acqua bassa
o nel fumo oggi non applica davvero `Wet`/`Obscured`**: il dato è dichiarato nel catalogo, la chiamata
avviene, ma lo stato non si materializza. Il log di risoluzione non mente più a riguardo (il Task 7 ha
condizionato la riga di log all'effettiva applicazione), ma la funzionalità resta assente.

**Non è un difetto da correggere in CP 8.1**: la semantica "dura finché sulla cella" richiede il modello di
scadenza/durata degli stati di **CP 8.2** (`#65`), che questo CP dichiara esplicitamente fuori scope (§6). Un
valore arbitrario tipo `StatusDuration = 1` renderebbe l'effetto tecnicamente "attivo" ma con una semantica
sbagliata (scade dopo un turno anche restando sulla cella), quindi non è la correzione giusta. **Dichiarare
nella PR**: `Fire`/`Burning` funziona (durata 2, valore fisso, non serve il modello "finché sulla cella");
`Wet`/`Obscured` restano dati pronti-ma-inerti fino a CP 8.2, stesso pattern di `PushResistance` di Bastion.

## 6. Fuori scope dichiarato (CP 8.1)

- Scadenza/durata degli stati (`Wet`, `Burning`, `Obscured`) in Cleanup → CP 8.2 (`#65`).
- Propagazione elettrica da `Conductive`/`ShallowWater` → CP 8.3 (`#66`).
- Spegnimento Fuoco da Wet, ignite dinamico → CP 8.4 (`#67`).
- Bonus visuale di `HighGround` → nessun CP dichiarato lo consuma ancora; da rivalutare quando esiste un
  meccanismo di vista modificabile per quota.
- Scivolamento su `Ice` dopo uno **Scatto** (solo il Move normale lo innesca in CP 8.1, vedi §5.2).

## 7. Piano di test

Richiesti dal DoD (`v0.1-issue-plan.md` §CP 8.1):
- `RefactorTactics.Terrain.CostsFromCatalog`
- `RefactorTactics.Terrain.Rough.BlocksDash`
- `RefactorTactics.Terrain.Smoke.LimitsTargeting`

Aggiunti per coprire il resto del DoD dichiarato:
- `RefactorTactics.Terrain.ValidateCatalog.NoDuplicatesNoNegativeCosts`
- `RefactorTactics.Terrain.Ice.SlidesWithSufficientBudget`
- `RefactorTactics.Terrain.Ice.BlockedCellStopsSliding`
- `RefactorTactics.Terrain.Fire.DamagesAndBurnsOnEnter`
- `RefactorTactics.Terrain.ShallowWater.AppliesWet`
- `RefactorTactics.Terrain.Conductive.DoesNotApplyWet`

## 8. File coinvolti

`Terrain/RTTerrainData.h` (nuovo), `Terrain/RTTerrainLibrary.{h,cpp}` (nuovo), `Map/RTHexCellData.h`
(relabel enum + 2 nuovi valori), `Turn/RTHexSimLibrary.cpp` (hook Dash/Charge + sliding),
`Turn/RTActionEffectLibrary.{h,cpp}` (funzione di traduzione spec→eventi estratta), `Combat/RTHexCombatLibrary.cpp`
(cap targeting Smoke), `Core/RTGameplayTags.{h,cpp}` (nuovi tag Wet/Burning/Obscured), tool editor
(`RTHexPaintTool`, `RTHexEditorClick`, `RTHexSelectTool`, `RTHexFillTool`) per i default da catalogo e i
nuovi colori overlay.
