# Tactical Designer — DevSandbox Launcher · spec panel sulla roadmap L0–L8

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **consumato e archiviato**, non applicato ·
> **Data**: 2026-08-29
> **Base di misura**: `bbf0d780` (`origin/main` dopo `git fetch --prune`). ⚠️ Il checkout usato
> (`refactor-tactics-technical-designer/refactor-tactics-main`) è a `4ca09bcd`, **6 commit indietro**:
> nessuna misura è stata presa lì. Tutto ciò che segue è letto con `git show origin/main:<path>`, senza
> checkout — §13.
> **Oggetto**: `RefactorTactics_TacticalDesigner_Claude_Handoff.md` (1020 righe, diciassette sezioni `§0`–`§16`),
> una roadmap in nove slice `TD-L0`–`TD-L8` che introduce un **launcher d'editor** su `L_DevSandbox` come
> punto d'ingresso del workflow Tactical Designer: scegli mappa, formato e scenario, poi `Start Session`.
> Letto contro `Source/RefactorTactics/ScenarioHarness/`, `Source/RefactorTactics/Turn/`,
> `Source/RefactorTacticsEditor/`, gli 88 scenari di `Scenarios/`, i byte di `L_DevSandbox.umap`, le tredici
> issue che nomina, `spec-tactical-designer.md`, `scenario-index-e-tag.md`, ADR-0010 e l'epic #1105.
> **Panel**: Wiegers (lead) · Fowler · Cockburn · Nygard · Crispin · Adzic
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-29-tactical-designer-devsandbox-launcher.md`](../../archive/src/handoff/2026-08-29-tactical-designer-devsandbox-launcher.md)
> **Referto gemello dello stesso giorno**: [`td-trial-scenario-sandbox-spec-panel-2026-08-29.md`](td-trial-scenario-sandbox-spec-panel-2026-08-29.md)
> — le issue #1625–#1631 che questo documento dà per esistenti sono state aperte da quella passata.

---

## 1. Il verdetto in una riga

Il documento **trova un buco vero che l'epic #1105 non copre** — l'epic descrive tutto ciò che il designer
può fare *una volta dentro*, e non dice mai come ci entra — ma poi lo riempie con **tre assi di selezione che
il dato non ha**: uno scenario non riferisce una mappa, non dichiara un formato, e l'indice che dovrebbe
filtrarlo non porta né l'una né l'altro. `TD-L2`, `TD-L3` e `TD-L4` — un terzo delle slice, e le due che il
documento propone di parallelizzare — non sono implementabili come scritte.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **3** |
| 🟡 Medio | **7** |
| ➕ Trovato misurando, non è del sorgente | **1** |

**Raccomandazione operativa**: **eseguire `TD-L0` e `TD-L1`, fermarsi prima di `TD-L2`.** Le prime due slice
sono corrette, piccole e poggiano su API che esistono (§10, `M1`). Le tre successive vanno **riscritte prima
di essere aperte**, non corrette dopo: l'asse sbagliato non produce una UI da rifinire, produce una UI che
insegna al designer un vocabolario che il gioco non ha — ed è precisamente il difetto che il §1.1 dello
stesso documento vieta con la frase migliore che contiene, *«il launcher NON decide il gioco»*.

⚠️ Nessuna suite eseguita, nessuna build, nessuna scrittura su GitHub, nessun file di codice toccato. Issue
lette lato server con `gh` il 2026-08-29; `Source/`, `docs/`, `Scenarios/` e `Content/` a `bbf0d780`.

---

## 2. Baseline misurata

Il §5 del documento chiede a Claude di misurare `HEAD` prima di toccare codice ed elenca otto ricerche.
Eseguite tutte. **Sette affermazioni su otto reggono**, ed è più di quanto l'archivio abbia abituato ad
aspettarsi:

| # | Il documento asserisce | Misurato a `bbf0d780` | |
|---|---|---|---|
| 1 | Repository `DegrassiAaron/refactor-tactics-main` | `gh repo view` → **esatto** | ✅ |
| 2 | `URTScenarioAuthoring` è la porta canonica | `ScenarioHarness/RTScenarioAuthoring.h`, **35 `UFUNCTION`** | ✅ |
| 3 | Esiste un Scenario Index canonico | `URTScenarioIndex`, **8** test `RefactorTactics.ScenarioIndex.*` | ✅ |
| 4 | `URTHexEditorMode` esiste e non va rinominato | `Source/RefactorTacticsEditor/Public/RTHexEditorMode.h` | ✅ |
| 5 | `GetLayers()` esiste e non va duplicato | `URTHexMapAsset::GetLayers()`, [`RTHexMapAsset.cpp:184`](../../../Source/RefactorTactics/Map/RTHexMapAsset.cpp) | ✅ |
| 6 | Il modulo Editor **ha già** Automation Tests | **2**, `RefactorTactics.HexEditor.*` in `Private/Tests/` | ✅ |
| 7 | `RunFromTheEditorMatchesTheHeadlessRun` è il guardiano da non duplicare | [`RTScenarioRunResetTests.cpp:91`](../../../Source/RefactorTactics/Tests/RTScenarioRunResetTests.cpp), verde | ✅ |
| 8 | `SetModeSettingsObject` (§5, riga 7 delle ricerche) | **0 occorrenze** in tutto `Source/` | ⛔ |

E le tredici issue citate sono **tutte esatte**: #1105 `OPEN`, #1186 `OPEN`, #1625–#1631 `OPEN`, #1114–#1117
`CLOSED`. Va detto, perché il documento le elenca senza averle potute leggere.

⚠️ **La riga 8 non è un difetto grave ed è istruttiva**: `SetModeSettingsObject` è un metodo del framework
`UEdMode` del motore, non del repository, e cercarlo in `Source/` non poteva trovarlo. Il §5 lo mette in un
elenco insieme a sette simboli di progetto, e chi esegue quelle otto righe alla lettera legge un `0` e non sa
se significhi *«non c'è»* o *«hai cercato nel posto sbagliato»*.

---

## 3. 🔴 C1 — uno scenario non riferisce una mappa, e l'asse «Map» è il primo della UI

È il difetto strutturale del documento, e ne genera altri tre.

Il flusso proposto è `Map → Format → Scenario`, con `DA_HexMap_Arena` nella tendina, `Cells:` / `Layers:` in
readout, una riga di validazione `✓ Map compatible`, un failure state *«Scenario incompatible with selected
map»*, un secondo *«Map changed after scenario load»*, e un test `FiltersScenariosByMap`.

Misurato, **un `FRTTestScenario` non contiene nessun riferimento a un `URTHexMapAsset`**. I campi che
descrivono il terreno sono due:

```text
Fixture   : FString   nome di una fixture generata da codice
                      (RelayBasin · RelayLite · TestArena · ArenaV01 · CoverYard)
MapRadius : int32     raggio dell'esagono generato quando Fixture e' vuoto
```

e il commento del campo `MapRadius` lo dice in cinque parole: *«Mappa da codice: **nessun `.umap`**»**.

[`URTScenarioArenaLibrary::BuildArena`](../../../Source/RefactorTactics/ScenarioHarness/RTScenarioArena.cpp)
non carica mai un asset da disco: chiama `MakeFixtureArena` oppure `MakeFlatArena`, poi applica le celle
elencate nel file. Misurato sugli **88** scenari versionati:

| Sorgente dell'arena | Scenari |
|---|---:|
| `mapRadius` (arena generata) | **67** |
| `fixture` (fixture da codice) | **21** |
| asset mappa d'autore | **0** |

**Wiegers**: quattro requisiti del documento — il filtro per mappa, la riga `✓ Map compatible`, e i due
failure state — chiedono di confrontare due cose di cui la seconda non esiste. Non sono requisiti difficili:
sono requisiti **senza soggetto**. Chi li prende non scopre un ostacolo, scopre che deve inventarsi il
significato di «compatibile», e qualunque cosa inventi diventa una regola d'editor su cui nessun test
runtime ha voce — cioè la seconda autorità che il §1.1 vieta.

**Fowler**: e l'asse giusto è già esposto, in un DTO di sola lettura fatto apposta.
`FRTScenarioSummary` — ciò che la facade restituisce con `GetSummary()`, marcato `BlueprintReadOnly` uno per
uno secondo ADR-0010 — porta:

```text
ScenarioId · Version · Tags · Fixture · MapRadius · UnitCount · TurnCount · ExpectationCount · VariantCount
```

`Fixture` e `MapRadius` sono l'asse «dove si gioca». Sono già canonici, già filtrabili, già leggibili senza
aprire l'editor. Il launcher non deve procurarsene uno: deve smettere di cercarne un altro.

⚠️ **E c'è una terza conferma indipendente, nella porta stessa**: la funzione con cui si crea uno scenario
nuovo è

```cpp
void NewScenario(const FString& ScenarioId, int32 MapRadius = 3);
```

Chiede un **raggio**. Non un asset mappa, non un formato. Il §TD-L5 del documento disegna
`Map + Format + New Scenario → Create FRTScenarioDraft`: quei due ingressi non hanno un parametro dove
atterrare.

### Correzione
Sostituire l'asse `Map` con **`Fixture` / `MapRadius`**, che è ciò che lo scenario dichiara, e togliere le
due righe di validazione e i due failure state che ne dipendono. Se si vuole davvero che uno scenario possa
riferire una `URTHexMapAsset` d'autore, quella è **un'estensione del formato scenario** — appartiene
all'elenco dei gap di [`spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md) §5,
con il proprio innesco dichiarato, e a nessuna slice di un launcher.

---

## 4. 🔴 C2 — il formato ha già un owner canonico, e la condizione del §TD-L3 si risolve al contrario

Il §TD-L3 prescrive: *«NON introdurre automaticamente una nuova enum runtime se non esiste un reale owner
canonico del concetto»*, e in mancanza di owner propone di *«derivare il formato da Team A unit count / Team
B unit count»*. È la forma giusta di un requisito — condizionato a una misura. **La misura dà l'esito
opposto a quello che il documento assume.**

L'owner esiste, si chiama formato, ed è documentato:

| | Dove |
|---|---|
| Asset sorgente | `URTMatchFormatData` — [`Turn/RTMatchFormatData.h`](../../../Source/RefactorTactics/Turn/RTMatchFormatData.h) |
| Regole risolte | `FRTMatchRules` — `FormatId · RoundLimit · ScoreToWin · UnitsPerTeam · UnitsPerPlayer · MapClass` |
| Validazione | `URTMatchFormatLibrary::ValidateRules` · `ValidateFormat` · **`ValidateAgainstMap`** |
| Copertura | **14** test `RefactorTactics.MatchFormat.*` |

`ValidateAgainstMap` è, alla lettera, la riga `✓ Map compatible` che il documento disegna: *«una mappa
disegnata per una classe diversa da quella che il formato richiede […] va rifiutata all'allestimento, non
scoperta al terzo turno»*.

E la deriva proposta come ripiego è **esattamente la confusione che un test esistente esiste per prendere**.
`FRTMatchRules::UnitsPerPlayer` porta l'avvertimento scritto:

> *«⚠️ **NON è `UnitsPerTeam`, e la differenza è invisibile proprio dove conta.** […] In `Format.Skirmish2v2`
> valgono **entrambi 2** […] quindi un percorso che legga l'uno al posto dell'altro passa ogni test esistente
> e sbaglia al primo formato che divide una squadra fra due persone.»*

Un conteggio di unità per squadra non distingue i due. `MatchFormat.ControlCountIsNotUnitsPerTeam` esiste per
questo.

**Nygard**: e c'è un secondo fatto che la tendina proposta non regge. `URTMatchFormatLibrary::FindShippedFormat`
oggi conosce **un** formato — `Format.Skirmish2v2` — e il commento dichiara la regola: *«Gli altri entrano
quando un checkpoint li consuma»*. Non esiste nessun `DA_MatchFormat` su disco. Una tendina che offre
`1v1` e `3v3` offre due formati **che non esistono**, e il `DoD` del §TD-L3 — *«3v3 è facilmente selezionabile
per main format»* — chiede di renderne selezionabile uno che andrebbe prima creato: cioè autorare dato
canonico di runtime, dentro una roadmap che si dichiara `Editor-only / out_of_release_scope` in testa.

**Cockburn**: infine, i due concetti nemmeno si toccano. Misurato: **`Source/RefactorTactics/ScenarioHarness/`
non nomina `MatchFormat` una sola volta.** Uno scenario non ha un formato, e non è una lacuna — è che uno
scenario allestisce un banco di prova, non una partita. Il corpus lo dimostra:

| Composizione | Scenari |
|---|---:|
| `1v1` | **54** |
| `2v2` | **13** |
| `1v2` | 9 |
| `2v1` | 8 |
| `3v3` | **2** |
| `2v4` · `1v3` | 1 · 1 |

**Diciannove scenari su 88 sono asimmetrici** — e un `1v2` non è un formato di partita né un «Custom»: è un
allestimento che verifica una regola. Etichettarlo con il vocabolario di `URTMatchFormatData` insegna al
designer che il gioco ha un formato `1v2`. E il `3v3` che il documento vuole «facilmente selezionabile» ha
**due** scenari.

### Correzione
Due mosse, e la seconda è quella che salva la slice.

1. **Non chiamarlo `Format`.** L'asse utile è la **composizione** (`2v2`, `1v2`, …) derivata da
   `units[].team`, e va nominata composizione, mai `Format.*`. Il vocabolario `Format.*` resta di
   `URTMatchFormatData`, che il launcher non tocca.
2. **La composizione non può essere un filtro della lista, ma può essere un readout dello scenario
   selezionato** — ed è la forma in cui la slice diventa costruibile oggi. Il perché è al §5.

---

## 5. 🔴 C3 — l'indice degli scenari non porta né mappa né composizione, e l'unico asse è il tag

Il §1.5 prescrive *«Usare il sistema di Scenario Index / discovery già esistente. NON implementare una
scansione parallela»*, e il §TD-L4 chiede un browser filtrato per **mappa** e **formato**. Le due
prescrizioni non stanno insieme.

`FRTScenarioEntry` ha tre campi:

```text
ScenarioId : FString
Path       : FString
Tags       : TArray<FString>
```

e `URTScenarioIndex::ReadHeader` legge **solo** `scenarioId` e `tags`, per una ragione dichiarata nel codice:

> *«Non interpreta unità, turni e assertion: costruire l'indice non deve costare quanto caricare ogni
> scenario, e soprattutto uno scenario con un intent malformato deve restare **trovabile**.»*

`ListIds(FilterA, FilterB)` prende **esattamente due tag** e ne fa l'intersezione. Non c'è un terzo
parametro, e [`scenario-index-e-tag.md`](../../technical/tooling/scenario-index-e-tag.md) §6 dichiara che
*due e non tre* è una decisione, non un limite: *«il terzo diventerebbe rumore prima di diventare utile»*.

Quindi filtrare per mappa o per composizione ha tre strade, e il documento non ne sceglie nessuna:

| | Strada | Costo reale |
|---|---|---|
| **a** | Aprire tutti gli scenari per leggerne unità e fixture | Rompe la ragione per cui `ReadHeader` esiste. 88 file oggi, e la lista si apre a ogni ridisegno del pannello |
| **b** | Estendere `FRTScenarioEntry` / `ReadHeader` con `fixture` e composizione | È una modifica a un componente **runtime canonico**, con owner documentale proprio. Fattibile e forse giusta — ma è una issue di `scenario-index-e-tag.md`, non una slice di launcher |
| **c** | Convenzione sui tag (`map:arena`, `2v2`) | Il vocabolario dei tag **non si dichiara** per decisione: è l'unione di quelli presenti nei file. Un namespace convenzionale lo dichiarerebbe dalla porta di servizio |

**Crispin**: e i tre test proposti dal §TD-L4 — `FiltersScenariosByMap`, `FiltersScenariosByFormat`,
`SearchFiltersScenarioList` — non sono scrivibili contro nessuna delle tre finché la strada non è scelta.
Il primo non ha soggetto (`C1`); il secondo richiede `b` o `c`; il terzo richiede una superficie Slate,
perché [`scenario-index-e-tag.md`](../../technical/tooling/scenario-index-e-tag.md) §2 dichiara misurato in
Editor che *«la combo di UE non filtra da testo»* — è la ragione per cui i tag esistono. Una casella di
ricerca non è una property con `GetOptions`: è un widget, ed è un costo che il documento non nomina.

⚠️ **E la soglia del vocabolario è già scattata**: `#1261` è `OPEN` — *«il vocabolario dei tag ha superato la
soglia dei 40: la tendina non si auto-denuncia più»*. Chi costruisce un browser sui tag eredita quel difetto.

### Correzione
Ritirare i filtri per mappa e composizione **dalla lista** e tenerli come **readout dello scenario
selezionato**, dove il dato c'è già: `FRTScenarioSummary` porta `Fixture` e `MapRadius`, e
`FRTScenarioUnitView::TeamId` dà la composizione attraverso la facade una volta che lo scenario è aperto. La
lista resta filtrata per **tag**, che è ciò che l'indice sa fare, con la ricerca testuale come unica aggiunta
— ed è l'aggiunta che risolve il problema misurato («oltre la ventina di voci scorrerle smette di essere un
modo di trovare qualcosa») invece di aggiungerne due che nessuno ha misurato.

---

## 6. 🟠 A1 — un browser di scenari con filtri esiste già nell'Editor, e il documento non lo nomina

Il §TD-L6 motiva l'intera roadmap con un «prima» che non è la baseline:

```text
open Unreal → find tool → find map → find scenario → configure → test
```

Misurato, il «prima» reale è più corto e ha già un owner documentale. Su `ARTGameMode`, nel Details Panel,
vivono tre property dichiarate nell'ordine in cui si usano:

```text
▼ RefactorTactics|Test
    Scenario Filter A   [movement ▼]   GetOptions: vocabolario tag reale
    Scenario Filter B   [core     ▼]   intersezione col primo
    Scenario To Run     [...      ▼]   solo chi passa entrambi
```

più `ShippedFormatId` (`Format.Skirmish2v2` di default), `MapSource` (`LevelAsset` ·
`GeneratedDemoArena` · `GeneratedTestArena`), `bAutobattle` e `ScenarioStepDelay` sotto
`RefactorTactics|Match` e `|Map`. Owner: [`scenario-index-e-tag.md`](../../technical/tooling/scenario-index-e-tag.md)
§6. Copertura: `ScenarioIndex.TwoFiltersIntersect`, `Scenario.AutoRunOptionsAreFilteredByTags`, e **due
verifiche PIE verdi dal 2026-08-08** — `PIE-SCEN-FILTER` e `PIE-SCEN-KEEP`.

**Adzic**: un documento che propone una superficie di selezione deve dire cosa fa la superficie esistente e
perché non basta. Qui la risposta c'è ed è buona — il Details Panel non ha un `Start Session`, non ha un
readout della mappa, non ricorda una sessione, e richiede di sapere **quale actor selezionare** — ma il
documento non la dà, e chi esegue rischia di costruire il secondo browser invece del primo launcher.

⚠️ **E una decisione già presa viene contraddetta.** Quel documento owner deriva dal fatto che i filtri sono
una lente: *«I filtri non toccano la selezione […] Ne segue anche che **non esiste un errore di "category
mismatch"**: con una lente non c'è niente da cui uno scenario possa essere incoerente.»*
Il §8 del sorgente elenca fra i failure state *«Scenario incompatible with selected map»* e *«Unknown/custom
format»*. Sono errori di category mismatch, in un modello che ha deciso di non averne — e la deviazione
sarebbe legittima solo dichiarandola, che è il contrario di come compare.

✅ Da tenere: il §TD-L3 prescrive *«il filtro non altera scenario esistente»*, che è **la stessa decisione**,
raggiunta indipendentemente. È il punto in cui il documento e il repository si danno ragione senza sapersi.

---

## 7. 🟠 A2 — la mappa del livello su cui il launcher si apre è vuota, e quella che il livello usa è volatile

Il documento apre il launcher su `L_DevSandbox` e ci mette in testa un readout `Map / Cells / Layers`.
Misurato, quel readout sul primo avvio direbbe qualcosa di imbarazzante.

Lo stato dei tre `URTHexMapAsset` versionati è scritto nel repository, misurato il 2026-08-24 dentro
[`RTHexMapTests.cpp`](../../../Source/RefactorTactics/Tests/RTHexMapTests.cpp):

| Asset | Celle | Note |
|---|---:|---|
| `DA_HexMap_Arena` | **64** | *«l'unico asset mappa con contenuto reale del repository»* — ≥2 layer, transizioni, validator pulito. Appartiene a `L_HexArena` |
| `DA_HexMap_Sandbox` | **0** | *«è vuoto»*, 1396 byte. È l'asset in `L_DevSandbox/Data/` |
| `DA_HexMap_Scratch_Basin` | **45** | vive in `_Scratch`, e `GenerateFixtureIntoAsset` **lo sovrascrive a ogni rigenerazione** |

E i byte di `L_DevSandbox.umap` dicono a quale dei tre il livello è legato. La tabella dei riferimenti nomina
`/Game/RT/Maps/Dev/_Scratch/DA_HexMap_Scratch_Basin` e **non nomina `DA_HexMap_Sandbox`**.

∴ il launcher si aprirebbe su un livello il cui `ARTHexMapActor` punta all'asset **volatile**, mentre l'asset
che la convenzione di cartella associa a quel livello è **vuoto**, e l'unica mappa con contenuto vero
appartiene a un altro livello.

**Nygard**: il §8 elenca `No Map Asset` ma non *«MapAsset presente e vuoto»*, e quella è la condizione reale —
il difetto è stato osservato in PIE proprio su `L_DevSandbox`. Un failure state che le fonde perde l'unico caso
che si verifica oggi.

🔴 **Correzione del 2026-08-31: la prima stesura di questa voce diceva «il runtime le distingue già», ed era
falso.** Portava a prova la documentazione di `DemoArenaRadius` — ripiego *«quando il livello non porta una
mappa esagonale **con celle** (asset assente **oppure presente ma vuoto**)»* — che è invece la
**documentazione della fusione**: è la frase con cui un unico valore dichiara di coprire entrambi i casi.
Misurato su `origin/main` = `511a1cb4`, il runtime li fonde in tre punti su tre.

```cpp
// RTMatchBootstrapper.cpp:145 — un solo ramo
if ((!HexMap->MapAsset || HexMap->MapAsset->NumCells() == 0) && Config.DemoArenaRadius > 0)

// RTStartupReport.h:94 — un solo outcome
/** La mappa del livello e' assente o **senza celle**: si ripiega sull'arena demo. */
LevelMapMissing,
```

Più una sola frase di log: *«Mappa esagonale del livello assente o senza celle»*. Ciò che il runtime distingue
davvero è `LevelMapMissing` (degradato, ripiega) da `MatchLevelUnset` (fatale, non ripiega); **dentro**
`LevelMapMissing` i due casi sono indistinguibili per enum, per ramo e per frase.

∴ la distinzione non va **preservata**, va **creata**, e sceglierne il posto ha un costo che va deciso:
costruirla nel launcher mentre il runtime la fonde produce due tassonomie per la stessa condizione, cioè la
seconda autorità che i guardrail vietano altrove. [#1683](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1683)
(`L8`) porta l'acceptance criterion riscritto di conseguenza — `diventano` al posto di `restano` — più
l'out of scope che tiene il ramo runtime fuori da quella slice.

✅ Il documento ha però ragione su una cosa che nessuno aveva scritto: il §8 elenca *«Map changed after
scenario load»*, e con un asset rigenerabile sotto il livello quel caso non è ipotetico.

> ➕ **Questa voce ha un seguito misurato lo stesso giorno**, in
> [`terminali-code-editor-e-dir-c-spec-panel-2026-08-29.md`](terminali-code-editor-e-dir-c-spec-panel-2026-08-29.md) §11
> `P1`: l'asset con **le celle vere della sandbox** esiste, si chiama `DA_Format_Scratch.uasset` (20583 byte,
> riammesso apposta in `.gitignore` per #623) ed è oggi **orfano** — `21f4042f` (#956) ha ripuntato il livello
> su `_Scratch/DA_HexMap_Scratch_Basin`. Due documenti dicono ancora il contrario, uno dei quali è
> `tools/asset-refs/check.ts`, cioè lo strumento che controlla i riferimenti fra asset.

---

## 8. 🟠 A3 — «Session» è un nome già occupato, due volte, e il documento non dice quale

Il §TD-L0 chiede giustamente di separare `Start Session` da `Run`, e il §TD-L5 chiama `Tactical Designer
Session` la cosa che nasce dal launcher. Misurato, nel runtime esistono già due oggetti che si chiamano
sessione e fanno cose diverse:

| Oggetto | Cos'è |
|---|---|
| `FRTScenarioSession` | l'esecuzione di uno scenario, un passo per frame (`RTScenarioSession.cpp`) |
| `URTScenarioAuthoring` | la facade che possiede il `FRTScenarioDraft` aperto: `OpenById` · `IsOpen` · `Close` |

**Wiegers**: `URTScenarioAuthoring` **è già** una sessione d'authoring — ha apertura, stato aperto, chiusura,
validazione, `Run` e `Reset`. Se `Tactical Designer Session` è quella, il §TD-L5 descrive lavoro fatto; se è
un terzo oggetto che le contiene entrambe, va detto, perché quel terzo oggetto è l'unica cosa nuova che la
slice introduce. Il documento non sceglie, e il §1.2 — che vieta il nome sbagliato — non aiuta a trovare
quello giusto.

E la porta copre già quasi tutto il §TD-L5, verbatim:

| Il §TD-L5 chiede | Esiste |
|---|---|
| `Load canonical scenario` | `OpenById(ScenarioId, OutError)` |
| `Validate` | `Validate(OutError)` → `ERTScenarioAuthoringResult` |
| `Create FRTScenarioDraft` | `CreateScenarioDraft(Outer)` + `NewScenario(ScenarioId, MapRadius)` |
| `InvalidScenarioCannotStartSession` | `Validate` rifiuta e dice perché — resta da scriverne il test sul launcher |
| `ExistingScenarioIsNotSilentlyOverridden` | ⚠️ **niente da sovrascrivere**: `OpenById` non prende mappa né formato |

L'ultima riga è la più utile del referto. La regola *«lo scenario è source of truth, Map e Format non devono
sovrascriverlo»* è giusta — e il **solo** modo di violarla è avere due tendine che portano quei due valori.
Tolte quelle (`C1`, `C2`), l'invariante non va difesa: non è attaccabile.

---

## 9. 🟡 Medi

| | Punto | Misura |
|---|---|---|
| **M1** | Il §TD-L1 chiede apertura deterministica, *«nessun polling, nessun Tick»* | ✅ **il gancio esiste**: `FEditorDelegates::OnMapOpened(const FString& Filename, bool bAsTemplate)` — `UE_5.8/Engine/Source/Editor/UnrealEd/Public/Editor.h:360` — e c'è anche un pre-gancio, `OnMapLoad`, con un `FCanLoadMap` in uscita. ⚠️ **Che si attivi anche sul caricamento dell'`EditorStartupMap` all'avvio è precisamente ciò che va misurato prima di scrivere il `DoD`**, non assunto: è il caso d'uso principale della slice |
| **M2** | Il documento assume che aprire `L_DevSandbox` sia un atto deliberato | ⚠️ `Config/DefaultEngine.ini:27` — **`EditorStartupMap=/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox`**, col commento *«aprire l'editor sul menu non serve a nessuno»*. La premessa del documento è **confermata** ✅, e ne segue un requisito che non ha: il launcher si apre **a ogni avvio dell'editor**, anche per chi apre Unreal per lavorare sul Frontend. `DoesNotOpenOnNormalMaps` non copre il caso, perché DevSandbox *è* la mappa d'avvio |
| **M3** | Il §10 propone `RefactorTactics.Editor.DevSandboxLauncher.*` e dice di seguire il naming esistente | Misurati **1357** test in **76** aree. I nomi a tre segmenti **esistono** (`RefactorTactics.Actions.Fallback.*`, `RefactorTactics.Match.Autobattle.*`), quindi la forma va bene. Ciò che non esiste è `RefactorTactics.Editor.*`: il modulo editor usa **`RefactorTactics.HexEditor`**, e aprirne un secondo namespace per lo stesso modulo è la sola correzione |
| **M4** | Il §11 dice *«aggiungere una sessione manuale/PIE/editor nel registro appropriato»* | I registri sono **due** e hanno ruoli diversi: [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) è il **registro** (misurate **175** voci `PIE-*`), [`editor-sessions.yaml`](../editor-sessions.yaml) è la **sequenza** (massima seduta misurata: **U30**). E la regola sta nella nota di `U26`: *«una voce che non sta in una seduta non viene eseguita mai»*, e *«le crea la PR che implementa, che è anche l'unica che sa che aspetto avranno»* — cioè **non adesso** |
| **M5** | Il §9 dice di usare una category esistente *«altrimenti crearne una sola»* | Ne esiste **una**, `LogRT` ([`RefactorTactics.h:6`](../../../Source/RefactorTactics/RefactorTactics.h)), e il modulo editor dipende già dal runtime. La condizione si risolve: non serve crearne |
| **M6** | Il `DoD` del §TD-L8 chiede *«nessun dato locale entra in source control»* | ✅ già garantito dalla struttura: `.gitignore:18` ignora `Saved/`, dove atterrano le config per-utente dell'editor. È un vincolo gratuito, e va scritto come tale invece che come lavoro |
| **M7** | Il §4 ammette `L3 + L4` in parallelo *«solo quando il contract del formato e lo Scenario Index sono stati misurati»* | La condizione è **la cosa giusta da chiedere**, e la misura la chiude: nessuno dei due porta l'asse (`C2`, `C3`). La parallelizzazione va **ritirata**, non tenuta condizionata — una condizione già valutata falsa che resta scritta come condizione verrà rivalutata da qualcun altro |

---

## 10. Cosa il documento ha ragione, e va tenuto

È la parte maggiore, e sopravvive intera.

- ✅ **Il §1.1 e il §16 sono corretti e non deformati.** La catena `dati canonici → resolver / harness /
  TurnLog → pure query → UI d'editor` corrisponde a
  [`spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md) §3 e al corpo di #1105, e
  l'ultima riga — *«Il launcher NON decide il gioco»* — è la formulazione più compatta che l'archivio abbia
  di quell'invariante.
- ✅ **Il §1.2 cita la spec correttamente**: *«non esiste — e non deve nascere — un
  `URTTacticalDesignerSubsystem`»* è §2 della spec, alla lettera. ⚠️ Va aggiunta la **forma positiva**, che
  sta in ADR-0010 §4: *«sarà un `UEditorSubsystem` di poche righe **sopra questa stessa facade**, nel modulo
  Editor»*. Senza, il divieto si legge come «niente nel modulo Editor» — la deduzione che #1105 registra come
  già commessa **cinque volte**, l'ultima il 2026-08-29.
- ✅ **Il §1.3 nomina la porta** e **ADR-0010 è nell'elenco delle fonti del §1**. Il prompt gemello di stamattina
  non ce l'aveva, ed era il suo difetto critico `C1`.
- ✅ **Il §10, ultima riga**: *«Il modulo Editor ha già Automation Tests: NON dichiarare che i test editor
  sono impossibili»*. Misurato **vero**, ed è la **sesta** volta che questo repository deve dirlo — le prime
  tre sono #871, #921, #931, la quarta l'epic #1105, la quinta un referto di stamattina. È la riga migliore
  del documento perché anticipa un errore invece di ripararlo.
- ✅ **Il §TD-L7 non duplica**: dice che #1625–#1630 esistono, che la slice deve solo collegarsi, e che
  `RunFromTheEditorMatchesTheHeadlessRun` *«non va creato in una seconda versione con semantica diversa»*.
  Esatto: quel test è il `T0` dell'epic e la prova dell'assenza del secondo simulatore.
- ✅ **Il §8 vieta `silent fallback` / `silent mutation` / `implicit conversion`**, ed è la politica che il
  runtime applica già in tre punti indipendenti: `MakeFixtureArena` (*«nome sconosciuto → `nullptr`, mai
  un'arena vuota»*), `ResolvePath` (un id ambiguo si rifiuta, non si sceglie), `ResolveRules` (**fail-closed**,
  non tocca l'uscita).
- ✅ **Il §TD-L1 protegge `L_DevSandbox.umap` dal dirty**, e il costo è reale: è un `.umap`, due binari non si
  fondono, e questo file ha già richiesto di scartare un lato in un merge.
- ✅ **Il §13 (non-goals) coincide con l'elenco «post-Trial» di #1105** — Skill Workbench, mass simulation,
  bot tournament, promozione a produzione, modding, UI finale. Nessuna divergenza.
- ✅ **Il §12 chiede che ogni commit compili** e una issue per slice verificabile. È la disciplina della casa.

---

## 11. Slice → owner misurato → azione

Con le correzioni di §3–§8 applicate. **Stato** è ciò che esiste a `bbf0d780`.

| Slice | Cosa chiede | Owner misurato | Azione |
|---|---|---|---|
| **L0** | contract di `L_DevSandbox`, `Start Session` ≠ `Run` | `EditorStartupMap` già puntato lì (`M2`) · `Run` già esiste con guardiano | ✅ **aprire**. Aggiungere: cosa succede a chi apre l'editor per altro (`M2`), e **quale oggetto è la Session** (`A3`) |
| **L1** | auto-open deterministico | `FEditorDelegates::OnMapOpened` (`M1`) | ✅ **aprire**. Prima riga di lavoro: misurare se il delegate scatta sulla mappa d'avvio. Il test va estratto su un **predicato puro** (`ShouldOpenFor(FName)`), come i due test editor esistenti che non caricano nulla |
| **L2** | selettore mappa + metadata canonici | ⛔ `URTHexMapAsset` non è l'asse dello scenario (`C1`) · un solo asset con contenuto (`A2`) · #1186 `OPEN` | 🔴 **non aprire come scritta**. Diventa: *readout* di `Fixture`/`MapRadius` dal `FRTScenarioSummary`. Il pannello mappa dell'editor mode resta **#1186**, che ha già un owner |
| **L3** | filtro formato `1v1/2v2/3v3/Custom` | ⛔ `URTMatchFormatData` possiede il concetto (`C2`) · l'harness non lo usa · 19/88 asimmetrici | 🔴 **non aprire come scritta**. Diventa: readout della **composizione** da `FRTScenarioUnitView::TeamId`, senza il vocabolario `Format.*` |
| **L4** | scenario browser per mappa e formato | ⛔ l'indice porta solo i tag (`C3`) · un browser a due filtri esiste già (`A1`) · #1261 `OPEN` | 🔴 **non aprire come scritta**. Diventa **una** issue: *ricerca testuale sopra `URTScenarioIndex::ListIds`*, che è il problema misurato e non ancora risolto |
| **L5** | bootstrap scenario esistente/nuovo | 🟡 **quasi tutto consegnato**: `CreateScenarioDraft` · `NewScenario` · `OpenById` · `Validate` (`A3`) | 🟡 **ridurre**: resta il cablaggio launcher→facade e il test `InvalidScenarioCannotStartSession`. `ExistingScenarioIsNotSilentlyOverridden` **decade** con `C1`/`C2` |
| **L6** | entrare nel workspace | #1625–#1630 costruiscono le superfici | 🟡 **tenere, come issue sottile**: è il pezzo davvero nuovo — un punto d'ingresso, non una mega-UI, come il documento stesso dice |
| **L7** | `Run` via Scenario Harness | ✅ `URTScenarioAuthoring::Run` + `RunFromTheEditorMatchesTheHeadlessRun` (`T0` dell'epic) | ⏸️ **nessuna issue**: dopo `L6` non resta contenuto. Il collegamento è una riga di `L6` |
| **L8** | ricordare l'ultima sessione | `Saved/` già ignorato (`M6`) · l'asset di `L_DevSandbox` è volatile (`A2`) | ✅ **aprire**, e il caso di corruzione da coprire per primo è **quello vero**: la mappa rigenerata sotto la sessione salvata |

**Sulla issue parent**: il §2 chiede di crearne una sotto #1105, ed è corretto — #1105 non copre l'ingresso,
e le sue nove sub-issue diventerebbero orfane nella sola vista che questo repository usa per il progresso.
⚠️ Ma **non nove sub-issue**: quattro (`L0`, `L1`, `L6`, `L8`) più una **issue di decisione** che sostituisce
`L2`+`L3`+`L4` e ha una sola domanda — *quali assi di selezione esistono davvero nel dato*. Aprire `L2`–`L4`
come scritte significa consegnare a chi le prende la domanda che questo referto ha già misurato.

⚠️ **Priorità**: le label sono legate alla release (`P0` *«blocca la release»*, `P1` *«core della v0.1»*) e il
Tactical Designer è `out_of_release_scope`. Le sub-issue esistenti stanno a `P2`/`P3`, e queste vanno lì.

⛔ **Nessun `D-nnn` va aperto**: la decisione che il launcher richiederebbe — *«il launcher non è
un'autorità»* — è già `D-154` + ADR-0010. L'ultimo assegnato misurato su `origin/main` è **D-238**, da
riverificare sui ref remoti prima di qualunque merge (CLAUDE.md §7): ci sono **11** branch remoti vivi e
**4** PR aperte.

---

## 12. ➕ Trovato misurando: il registro PIE dichiara vuoto un livello che non lo è più

Non è un difetto del sorgente, e tocca esattamente la slice `L1`.

[`test-manuali-pie.md`](../../technical/test-manuali-pie.md) — *ultimo aggiornamento 2026-08-09* — dichiara:

> *«I livelli del demo (`L_Prototype`, `L_DevSandbox`) sono **vuoti nell'editor**: griglia, luce, unità e turn
> manager li allestisce a runtime il `RTGameMode`. Viewport nera prima del Play = normale.»*

Misurato sui byte di `L_DevSandbox.umap` a `bbf0d780`, il livello contiene:

```text
RTHexMapActor_0        (/Script/RefactorTactics.RTHexMapActor)
StaticMeshActor_0
DirectionalLight_0 · SkyLight_0 · SkyAtmosphere_0      ← #623, chiusa
Level Blueprint L_DevSandbox_C con EventGraph, due K2Node_Event
    e un riferimento a WBP_RT_ErrorModal
```

**Fowler**: la riga scaduta non è un dettaglio di documentazione per questa roadmap. `TD-L1` promette che
*«`L_DevSandbox.umap` non viene modificata»*, e quel file ha un **Level Blueprint vivo** che al Play mostra un
modale d'errore. Chi legge il registro crede di lavorare su un contenitore vuoto e trova un livello con
logica dentro — che è la differenza fra «il launcher non sporca la mappa» e «il launcher non interferisce con
ciò che la mappa già fa».

**Azione**: una riga da correggere nel registro, e la correzione appartiene alla PR che implementa `L1`,
perché è quella che avrà guardato il livello.

---

## 13. Come è stata protetta la misura

**D-222**: il checkout usato — `D:\Repositories\refactor-tactics-technical-designer\refactor-tactics-main` —
era pulito ma su `main` a `4ca09bcd`, **6 commit indietro** rispetto a `origin/main` (`bbf0d780`) dopo
`git fetch --prune`. Nessun checkout, nessun `pull`, nessuna scrittura sull'albero prima delle misure: tutto
è stato letto con `git show origin/main:<path>`, che non tocca la working directory e quindi non può
interferire con un'altra sessione né col mutex del motore. I file prodotti da questa passata sono i soli due
elencati al §14.

⚠️ L'unica misura fatta fuori dal repository è `FEditorDelegates::OnMapOpened` (`M1`), letta in
`D:\EpicGames\UE_5.8\Engine\Source\Editor\UnrealEd\Public\Editor.h` — sola lettura.

---

## 14. Cose non fatte

- ⛔ **Nessuna issue creata o modificata.** Il §2 e il §15.7 del sorgente lo chiedono; il §15.5 dello stesso
  sorgente dice *«NON modificare codice ancora»* e subordina la creazione alla verifica. La verifica è questa,
  e il suo esito è che **tre delle nove slice vanno riscritte prima di essere aperte** (§11). Aprirle adesso
  produrrebbe il lavoro che il §11 di questo referto raccomanda di non produrre.
- ⛔ **Nessun `D-nnn` assegnato** (verificato che non serve, §11).
- ⛔ **Nessun documento owner aggiornato.** La riga scaduta di `test-manuali-pie.md` (§12) resta com'è: la
  corregge la PR di `L1`, che è la sola che avrà misurato il livello.
- ⛔ **Nessun codice, nessuna build, nessuna suite.** Il modulo editor non è stato compilato, e questo referto
  non afferma niente sulla compilabilità di nulla.
- ⏸️ **Non misurato**: se `FEditorDelegates::OnMapOpened` scatti sul caricamento dell'`EditorStartupMap`
  all'avvio dell'editor. Richiede l'editor, ed è la prima riga di lavoro di `L1` (`M1`).
- ⏸️ **Non misurato**: a quale property dell'`ARTHexMapActor` di `L_DevSandbox` sia assegnato
  `DA_HexMap_Scratch_Basin`. La tabella dei riferimenti della `.umap` lo nomina e non nomina
  `DA_HexMap_Sandbox` (`A2`); l'assegnazione precisa sta dentro i byte serializzati dell'actor e si legge
  aprendo il livello.
- ⚠️ **Non consumati**, e restano alla radice di `refactor-tactics-technical-designer`:
  `CLAUDE_REFACTORTACTICS_TACTICAL_DESIGNER_CODE.md`,
  `CLAUDE_REFACTORTACTICS_TACTICAL_DESIGNER_EDITOR.md` e
  `REFACTORTACTICS — DIR-C · QA - SCENARIO - BOT - AUTOBATTLE v0.1.md`. Sono materia di altre passate.
