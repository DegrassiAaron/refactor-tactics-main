# Spec — Terreni esagonali (E8, CP 8.1)

> 🏔️ **[D-018](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08) — `HighGround` e vista.**
> Nella v0.1 la quota **non** dà alcun bonus numerico a `VisionRange`: nessun `+1` di default. Il «bonus
> visuale» che questa spec dichiarava senza quantificarlo **non va quantificato**: la quota vale già
> attraverso geometria, LOS, occlusione, copertura e topologia dei layer. Un bonus numerico futuro richiede
> playtest e una decisione separata.
>
> 👁️ **`Smoke`**: il cap offensivo definito qui resta. Ma la semantica di **detection e contatto** è posseduta
> da **E13** ([`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md)) e da
> [ADR-0005](../decisions/adr-0005-orientamento.md) — il terreno **non** duplica la logica di percezione.
>
> 🏃 **[D-015](../decisions/RT_PDR_00_Decision_Log.md)**: `Sprint` è un **profilo di `Move`**, non un Dash.
> Dove la classificazione legacy lo tratta come azione di fase Dash, è **debito di migrazione** dichiarato:
> nessun documento deve insegnare «Sprint = Dash».
>
> 🔥 **[D-059](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-09) — `Fire` e `Smoke` sono superfici, non
> stati.** Leggere l'elenco degli 8 terreni accanto a quello degli stati temporanei di **CP 8.2** fa sembrare
> che due valori siano dichiarati due volte: **non lo sono**. La superficie sta sulla **cella**, lo status
> sull'**unità**, e la superficie *produce* lo status su chi entra; il transitorio ha un **terzo** strato,
> `ARTTurnManager::DynamicSurfaces`, che ricorda l'originale e la scadenza. Corollario: nessun campo
> `BaseSurface` nella cella, nessun `MetalWet` nell'enum. Due brief esterni ci sono già inciampati.

> Brainstorming del **2026-08-06**. Fonti: [`RT_TerrainCatalog_v0.1.md`](../balance/RT_TerrainCatalog_v0.1.md)
> (catalogo canonico) · [`v0.1-issue-plan.md`](../roadmap/v0.1-issue-plan.md) §E8 (issue `#64`, CP 8.1) · codice
> esistente (`Map/RTHexCellData.h`, `Pathfinding/RTHexPathLibrary.*`, `Combat/RTHexCombatLibrary.*`,
> `Ability/RTActionDef.h` + `RTCatalogLibrary.*`). Superate le regole di
> [`spec-terreni.md`](../archive/gameplay/spec-terreni.md) (grid quadrato, rimosso in `7d8889b`).
> Autorità: subordinato al piano canonico e al catalogo terreni.

## 1. Obiettivo e scope

Portare gli 8 terreni del catalogo (`Terrain.Floor/Rough/ShallowWater/Fire/Conductive/Smoke/Ice/HighGround`)
sulle celle esagonali, come **dati**, non come `switch` C++. Scope = CP 8.1 (issue `#64`): identità e
comportamento di movimento/LOS/targeting dei terreni. **Fuori scope**: il sistema di stati con
durata/scadenza in Cleanup (CP 8.2, `#65`), la propagazione elettrica (CP 8.3, `#66`), le interazioni
fuoco/acqua e l'ignite dinamico (CP 8.4, `#67`), le azioni ambientali (CP 8.5, `#68`). Dove CP 8.1 deve
dichiarare un effetto che dipende da questi (es. `Wet` applicato ma non ancora scaduto/consumato da nessuno),
si dichiara il limite — stesso pattern già usato da CP 6.4/6.5 (Riktor/Wraith) per le dipendenze da E5/E9.

## 2. Modello dati

### 2.1 `FRTTerrainDef` — `Terrain/RTTerrainData.h`

```
USTRUCT(BlueprintType)
struct FRTTerrainDef
{
    ERTHexSurface Surface = ERTHexSurface::Floor;
    int32 MoveCost = 1;
    int32 SlideCells = 0;                    // 0 = non scivola; solo Ice lo valorizza (=1)
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
  `PushResistance` di Riktor. |

## 4. Call site da aggiornare (rename enum)

Riferimenti a `ERTHexSurface::Normal/Water/Mud/Electrified` da correggere al nuovo nome (stesso ordinale,
solo l'identificatore cambia):

- `Tests/RTHexMapTests.cpp`, `Tests/RTHexMovementIntegrationTests.cpp` — fixture di test.
- `Turn/RTMatchSetupLibrary.cpp` (arena demo: fascia "Mud" costo 3 → resta un valore **per-cella**
  legittimo, diverso dal default `Rough` costo 2 del catalogo). **Non è solo un rename**: dal Task 5 quella
  fascia eredita anche `bBlocksDashCharge` dal catalogo, quindi nell'arena demo — usata da `RTGameMode.cpp` e
  da 4 test d'integrazione — Scatto e Carica non la attraversano più. È un cambio di comportamento tattico
  voluto (la fascia diventa una barriera alla mobilità rapida, non solo costosa), non un effetto collaterale.
- `RefactorTacticsEditor/Private/RTHexEditorClick.cpp` (switch colori overlay — guadagna i casi
  `Smoke`/`HighGround`).
- `RefactorTacticsEditor/Private/Tools/{RTHexPaintTool,RTHexSelectTool,RTHexFillTool}.h` (default
  `ERTHexSurface::Normal` → `::Floor`).

## 5. Hook di risoluzione

### 5.1 Dash/Charge bloccati da `Rough`

`Turn/RTMovementActionLibrary.cpp` (`IsTraversableByStep`, usata da `ResolveLinearMove`): oltre al muro
(`bBlocksMovement`) la cella è negata anche da
`URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge`. Il movimento normale (`Action.Move`,
pathfinding via `RTHexPathLibrary`) **non** è affetto: `Rough` resta attraversabile a piedi, costa solo di
più (`MoveCost=2`, già gestito dal costo esistente). Il **salto** scavalca: il predicato vale per gli stili
che avanzano passo passo, non per l'atterraggio del `LinearLeap`.

> **Aggiornato dopo la issue `#140`**: la regola viveva in `URTHexSimLibrary::LinearDashPath`, una seconda
> implementazione dello scatto poi rimossa. Oggi esiste una sola funzione, e il bot vi accede col predicato
> `URTMovementActionLibrary::IsLinearReachable`.

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

**Limite dichiarato**: una mobilità **lineare** che termina su `Ice` **non** innesca lo scivolamento.
`URTMovementActionLibrary::ResolveLinearMove` non passa dal microstep condiviso — valuta gli ostacoli contro
l'occupazione **congelata** a inizio fase, quindi estendere il suo percorso con una cella extra non avrebbe
la stessa garanzia di correttezza sotto collisione simultanea del Move. Introdurlo richiederebbe far
partecipare anche lo Scatto al resolver condiviso — fuori scope per CP 8.1, dichiarato in PR.

**Regola dai dati, non dall'enum**: chi scivola lo dichiara il catalogo con `FRTTerrainDef::SlideCells`
(`Ice = 1`, tutti gli altri `0`), non un `Surface == ERTHexSurface::Ice` inciso in `ApplyIceSliding` — era
l'ultima regola di terreno ancora hard-coded contro l'enum, contro l'obiettivo di §1 e il DoD («non in
`switch` C++»). **Limite dichiarato**: `ApplyIceSliding` legge il campo come booleano (`> 0` → scivola di
**una** cella) e non srotola ancora N celle; l'intero è la forma giusta del dato per le dinamiche di CP 8.4,
non una funzionalità consumata in CP 8.1.

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

`Terrain/RTTerrainLibrary.cpp::EffectiveTargetingRange(Map, From, To, RangeCells)`: se la linea
attaccante→bersaglio sta **dentro o attraversa** almeno una cella `Smoke` (wording del DoD canonico,
`v0.1-issue-plan.md` §CP 8.1), la portata effettiva diventa `min(RangeCells, 2)` invece di `RangeCells`. Non
cambia la logica di blocco LOS esistente (muri/coperture): è un cap aggiuntivo, indipendente.

**Estremi della linea inclusi**, quindi anche l'attaccante **fermo su** una cella `Smoke` si vede cappata la
portata (non serve che la nuvola sia davanti a lui). Diverge di proposito dalla convenzione di
`URTHexVisionLibrary::HasLineOfSight`, che gli estremi li **esclude**: là `From`/`To` sono chi guarda e cosa
guarda, e non ha senso che si ostruiscano da soli; qui la superficie su cui si sta è esattamente ciò che il
DoD chiama «dentro». Non è una svista.

**Un solo posto per la regola**: la funzione è condivisa da tutti i cancelli che decidono «il bersaglio è a
portata» — `URTCombatLibrary::ClassifyHexTargeting` (preview del giocatore),
`URTActionFallbackLibrary::ValidateInstance` (validità dell'ordine al momento della risoluzione),
`URTHexBotLibrary::BuildCandidates` (proposte del bot) e `CollectHexAttacks` (resolver). Se il cap vivesse
solo nel resolver, gli altri tre accetterebbero un intento che poi viene scartato: slot speso, nessun effetto,
nessuna riga di log che lo spieghi — l'opposto della disciplina di `RTTurnManager` («l'azione fallita non
sparisce più in silenzio»).

### 5.5 Fire — LOS

`bBlocksLineOfSight = false` nel catalogo (deciso: "parziale" del PDF letto come "non bloccante" finché non
esiste un sistema di LOS graduata). Nessun nuovo meccanismo.

## 6-bis. ~~Limite~~ RISOLTO al CP 8.2 (2026-08-07): `Wet`/`Obscured` sono inerti a runtime

> ✅ **Chiuso da CP 8.2** (`#65`, [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) §3 D1):
> `ARTUnit::ApplyStatus` accetta ora la durata sentinella `PersistentWhileOnCell`, e
> `ARTTurnManager::ApplyTerrainOnEnterEffects` traduce la durata 0 del catalogo in quella sentinella. La revoca
> avviene nel Cleanup leggendo lo stesso catalogo (`URTTerrainLibrary::CellBoundStatusesFor`). Entrare in acqua
> bassa applica `Wet` davvero, uscirne lo toglie nello stesso turno. Il testo qui sotto resta come storia della
> scoperta.

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
`Wet`/`Obscured` restano dati pronti-ma-inerti fino a CP 8.2, stesso pattern di `PushResistance` di Riktor.

## 6. Fuori scope dichiarato (CP 8.1)

- Scadenza/durata degli stati (`Wet`, `Burning`, `Obscured`) in Cleanup → CP 8.2 (`#65`).
- Propagazione elettrica da `Conductive`/`ShallowWater` → CP 8.3 (`#66`).
- Spegnimento Fuoco da Wet, ignite dinamico → CP 8.4 (`#67`).
- Bonus visuale di `HighGround` → nessun CP dichiarato lo consuma ancora; da rivalutare quando esiste un
  meccanismo di vista modificabile per quota.
- Scivolamento su `Ice` dopo uno **Scatto** (solo il Move normale lo innesca in CP 8.1, vedi §5.2).

## 6-ter. La profondità dell'acqua è una **superficie**, non un asse — `GEO-1` ([D-081](../decisions/RT_PDR_00_Decision_Log.md))

Il [quinto handoff](../archive/src/handoff/2026-08-10-full-grid-geometry-walls-water.md) dà l'acqua come
**asse ortogonale** alla superficie (`WaterDepth = None | Shallow | Deep | Impassable`), con la profondità
che cambia durante il match. `#429` chiedeva se CP 8.1 si **estende** o si **riscrive**.

⚠️ **La domanda poggiava su una premessa misurabile e falsa.** La issue diceva che `ShallowWater` «non cambia
mai». Cambia: `Action.CreateWater` (CP 8.4) la **crea a runtime** attraverso `ApplyDynamicSurface`, che
ricorda la superficie originale, conta i turni e scrive nel TurnLog sia l'applicazione sia il ritorno. Il
*flooding* che l'handoff chiede come proprietà nuova di un asse **esiste già**, come cambio di superficie.

Da qui la scelta: **superficie composta.** `DeepWater` e `ImpassableWater` si aggiungono in coda all'enum
quando una feature le chiede, e riusano il meccanismo che esiste.

| | Cosa costa davvero |
|---|---|
| **Asse separato** | `FRTHexCellData` acquista un campo → **versione del formato mappa** e migrazione degli asset salvati. E `ApplyDynamicSurface` ricorda **una** superficie originale per cella: una profondità dinamica vorrebbe la **propria** macchina a stati accanto, non un campo in più |
| **Superficie composta** *(scelta)* | Zero migrazione, zero formato nuovo, e il produttore del flooding è già scritto. Il costo è l'esplosione combinatoria — che però si paga **al secondo asse**, non al primo |

**L'espressività persa non è nuova e non la paga nessuno.** L'argomento dell'asse è «`Ice` su acqua profonda
è esprimibile». Ma il repository non sa esprimere nemmeno `Ice` su `ShallowWater`, perché sono entrambe
superfici — e nessuna feature lo chiede: `RT-FEAT-MAP-WATER-DYNAMICS` è `IDEA`, e i profili `Swim`,
`Amphibious`, `Hover` non esistono.

**Quando si riapre.** Due inneschi, entrambi osservabili e non a discrezione:

1. compare un **secondo** asse ortogonale davvero indipendente dalla superficie (non «acqua profonda» ma, per
   dire, una temperatura che si combina con tutte e otto);
2. un profilo di movimento deve leggere la **profondità** senza leggere la superficie — cioè la
   composizione smette di essere un dettaglio di rappresentazione.

Finché nessuno dei due scatta, l'asse è un campo di formato pagato in anticipo per un'espressività che
nessuna regola consuma.

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

Aggiunti nella review finale di branch (2026-08-06), a chiusura dei difetti trovati:
- `RefactorTactics.Terrain.Smoke.CapAgreesAcrossGates` — preview, validazione ordini, bot e resolver devono
  dare la **stessa** risposta sul cap del Fumo (con controprova senza Fumo, così non passa a vuoto).
- `RefactorTactics.Terrain.Smoke.CapsWhenAttackerStandsInIt` — il «dentro» del DoD: estremi della linea inclusi.
- `RefactorTactics.Terrain.Ice.SlideBudgetBoundaryIsExactlyTwo` — confine esatto della soglia (2 scivola, 1 no).
- `RefactorTactics.Terrain.Ice.SlidesInMatch` — esteso: la cella di arrivo della scivolata è `Fire` e brucia
  (composizione Ghiaccio→Fuoco).
- `RefactorTactics.Terrain.Status.LogMatchesState` — ancorato al fatto che l'unità abbia davvero raggiunto
  l'acqua (prima l'asserzione di coerenza era `false == false` e passava anche a movimento mai avvenuto).

## 8. File coinvolti

`Terrain/RTTerrainData.h` (nuovo), `Terrain/RTTerrainLibrary.{h,cpp}` (nuovo), `Map/RTHexCellData.h`
(relabel enum + 2 nuovi valori), `Turn/RTHexSimLibrary.cpp` (hook Dash/Charge + sliding),
`Turn/RTActionEffectLibrary.{h,cpp}` (funzione di traduzione spec→eventi estratta),
`Terrain/RTTerrainLibrary.{h,cpp}::EffectiveTargetingRange` (cap targeting Smoke, in un posto solo) letta da
`Combat/RTHexCombatLibrary.cpp`, `Combat/RTCombatLibrary.cpp`, `Turn/RTActionFallbackLibrary.cpp` e
`Bot/RTHexBotLibrary.cpp`, `Core/RTGameplayTags.{h,cpp}` (nuovi tag Wet/Burning/Obscured), tool editor
(`RTHexPaintTool`, `RTHexEditorClick`, `RTHexSelectTool`, `RTHexFillTool`) per i default da catalogo e i
nuovi colori overlay.
