# RefactorTactics — Convenzioni per la struttura dei contenuti Unreal

> **Stato**: normativo · **Ultimo aggiornamento**: 2026-08-05
> Regole vincolanti per la struttura di `Content/`. Si applicano quando si creano asset, si suggeriscono
> percorsi, si modificano asset esistenti, si generano script Editor Utility, si propone un refactoring o si
> implementa una feature. **Non creare directory alternative senza una motivazione tecnica esplicita.**
>
> Lo stato del progetto è **conforme** dal 2026-08-05: registro della migrazione nell'**Appendice A**.

---

## 1. Namespace principale

Tutti gli asset proprietari stanno sotto `/Game/RT/` (fisicamente `Content/RT/`).

**Non** creare asset proprietari direttamente in `/Game/`, `/Game/Blueprints/`, `/Game/Materials/`,
`/Game/Textures/`, `/Game/Maps/`.

Gli asset Marketplace, Paragon, Megascans e di terze parti stanno sotto **`/Game/FabAsset/<Fornitore>/`**.
Non spostarli dentro `/Game/RT` senza aver verificato riferimenti, licenze e dipendenze.

> **Dove stanno**: i 26 pack Paragon sono in **`Content/FabAsset/Paragon/`** (48,1 GB), dentro il progetto ma
> **ignorati da git** — chi clona se li riscarica da Fab (README § *Asset di terze parti*, registro e insidie
> in **Appendice B**). Resta valido che un pack **non si sposta dentro `/Game/RT`**: quello è il namespace
> proprietario, e mescolarci dentro un pack rompe l'allineamento con i suoi aggiornamenti.

## 2. Principio organizzativo

La struttura è **feature-first**, non organizzata per tipo di asset. Evita cartelle globali come
`/Game/RT/Blueprints/`, `/Game/RT/Meshes/`, `/Game/RT/Textures/`.

Un asset sta **vicino alla feature che lo possiede**:

```text
/Game/RT/Characters/Flux/
├── Abilities/
├── Animation/
├── Audio/
├── Blueprints/
├── Data/
├── FX/
├── Materials/
├── Meshes/
└── Textures/
```

## 3. Struttura target

```text
Content/
├── RT/
│   ├── Core/            Framework · Input · Camera · Common · Debug
│   ├── Systems/         Planning · Turns · Resolution · Movement · Combat · Objectives · Replay · Networking
│   ├── World/
│   │   ├── Grid/        Hex · Selection · Visualization · Generation
│   │   ├── Pathfinding/ · LOS/ · Targeting/ · Cover/ · Surfaces/ · Hazards/ · Transitions/ · Interactables/
│   ├── Characters/      Shared/ · <CharacterId>/{Abilities,Animation,Audio,Blueprints,Data,FX,Materials,Meshes,Textures}
│   ├── Abilities/       Shared · Targeting · Effects · Cues · Debug
│   ├── Maps/            Dev/ · VerticalSlice/ · Shared/
│   ├── Environment/     ModularKits · Architecture · Props · Terrain · Materials · Textures · Decals · Foliage
│   ├── UI/              Framework · HUD · Planning · Resolution · UnitPanels · CombatLog · Tooltips · Icons · Styles · Debug
│   ├── Data/            Catalogs · Rulesets · Balance · GameplayTags · Curves · Tables · AssetManagement/Labels · Validation
│   ├── Art/             GlobalMaterials · PostProcess · ColorPalettes · DebugMaterials
│   ├── Audio/           Music · UI · Ambience · Shared
│   ├── FX/              Shared · Environment · Gameplay
│   ├── Dev/             TestAssets · DebugMaterials · Prototypes
│   └── Tests/           Functional · Fixtures · Maps · ReplayData
├── Developers/
└── ThirdParty/
```

**Non creare preventivamente le directory vuote.** Una directory nasce quando contiene almeno un asset reale
o quando serve alla feature corrente.

## 4. Struttura attiva nella milestone corrente

La struttura completa di §3 è il **target**; oggi il progetto ha bisogno solo di questo sottoinsieme
(milestone **M6 — Parità hex**, vedi [`roadmap-checkpoint.md`](roadmap-checkpoint.md)):

```text
/Game/RT/
├── Core/            Framework/ · Input/ · Camera/
├── Systems/         Planning/ · Turns/ · Resolution/ · Movement/
├── World/Grid/      Hex/ · Selection/ · Visualization/ · Generation/
├── Characters/      Shared/{Blueprints,Data,Materials}
├── Maps/Dev/        L_DevSandbox/{Data,Graybox,Tests}
├── UI/              HUD/ · Planning/ · Debug/
├── Data/            Rulesets/ · GameplayTags/ · AssetManagement/
├── Dev/             DebugMaterials/
└── Tests/           Functional/ · Maps/
```

Non introdurre cartelle per matchmaking, progressione, modding pubblico o sistemi non ancora previsti dalla
milestone corrente (coerente con la regola di scope in `CLAUDE.md`).

## 5. Regole di posizionamento

| Contenuto | Percorso | Note |
|---|---|---|
| Input (Enhanced Input) | `/Game/RT/Core/Input/` | `IMC_Tactical`, `IA_CameraPan`, `IA_SelectCell`, … |
| Camera tattica | `/Game/RT/Core/Camera/` | |
| Griglia esagonale | `/Game/RT/World/Grid/` | `Hex/` mesh e cella base · `Selection/` selezione e hover · `Visualization/` materiali, path preview, overlay · `Generation/` generatori graybox |
| Turni | `/Game/RT/Systems/` | `Planning/` intenti · `Turns/` fase e Ready · `Resolution/` playback · `Movement/` configurazione |
| Personaggi | `/Game/RT/Characters/<CharacterId>/` | ogni personaggio è **autonomo** |
| Abilità di un personaggio | `/Game/RT/Characters/<CharacterId>/Abilities/` | |
| Abilità realmente condivise | `/Game/RT/Abilities/{Shared,Targeting,Effects,Cues}/` | |
| Mappe | `/Game/RT/Maps/<Category>/<MapName>/` | una cartella per mappa |
| UI | `/Game/RT/UI/<funzione>/` | divisa per funzione, non per tipo tecnico |
| Dati di feature | vicino alla feature | `Characters/Flux/Data/DA_Hero_Flux` |
| Dati globali e cataloghi | `/Game/RT/Data/` | `Data/Catalogs/DA_HeroCatalog`, `Data/Rulesets/DA_Ruleset_Dev` |

**La logica autorevole C++ resta sotto `Source/`, mai dentro `Content/`** (invariante #1: le regole decidono
l'esito, gli asset no).

`Characters/Shared` contiene **solo** asset usati concretamente da almeno due personaggi — non è un
parcheggio.

**Asset di mappa** — regola decisiva: *se eliminando la mappa l'asset non serve più, può restare nella
cartella della mappa; se è usato da più mappe, va in una cartella condivisa.*

## 6. Naming

Formato: `<Tipo>_<Feature>_<Nome>_<Variante>`.

| Prefisso | Tipo | | Prefisso | Tipo |
|---|---|---|---|---|
| `BP_` | Blueprint Actor | | `SM_` | Static Mesh |
| `BPC_` | Blueprint Component | | `SK_` | Skeletal Mesh |
| `WBP_` | Widget Blueprint | | `M_` / `MI_` | Material / Instance |
| `ABP_` | Animation Blueprint | | `T_` | Texture |
| `DA_` | Data Asset | | `NS_` | Niagara System |
| `DT_` | Data Table | | `SFX_` / `MUS_` | Suono / Musica |
| `Curve_` | Curve | | `L_` | Level |
| `IMC_` / `IA_` | Input Mapping Context / Action | | | |

Esempi validi: `BP_Grid_HexGenerator`, `WBP_Planning_ActionBar`, `DA_Ability_Flux_ArcShot`,
`MI_Grid_HexPreview_Valid`, `L_DevSandbox`.

**Vietati**: `BP_Test2`, `BP_Final`, `NewMaterial`, `Hero_v3`, `MappaDefinitiva`. Niente numeri di versione,
«final», «new», «fixed» o date nel nome: **la cronologia è di Git**.

## 7. Dipendenze consentite

```text
Core  ←  Systems, World, Data  ←  Characters, Abilities, Objectives  ←  Maps  ←  UI, Dev, Tests
```

- `Core` non dipende da una mappa né da un personaggio specifico.
- Un personaggio non dipende da una mappa specifica.
- **Il simulatore non dipende da UI, animazioni o VFX**; le animazioni non determinano l'esito (invariante #1).
- La UI legge eventi, ViewModel e stato autorizzato.
- Nessun asset runtime dipende da `/Game/Developers` né da `/Game/RT/Tests`; i test possono dipendere dal runtime.
- Evita dipendenze circolari fra cartelle e feature.

Preferisci `TSoftObjectPtr<>`, `TSoftClassPtr<>`, `FPrimaryAssetId`, `FName`, `FGameplayTag` alle hard
reference non necessarie.

## 8. Cartelle speciali

| Cartella | Uso |
|---|---|
| `/Game/Developers/<Nome>` | esperimenti personali temporanei; **mai referenziata dal runtime** |
| `/Game/RT/Dev` | strumenti e materiali di debug condivisi (`M_Debug_CellValid`, `BP_Debug_IntentVisualizer`) |
| `/Game/RT/Tests` | Functional Test, mappe di test, fixture, replay deterministici |
| `/Game/ThirdParty` | asset esterni **privi** di un namespace proprio |

Non modificare direttamente un asset esterno se puoi crearne una variante proprietaria sotto `/Game/RT`.

## 9. File sorgente esterni

I sorgenti non importati in Unreal non stanno in `Content/`:

```text
<ProjectRoot>/SourceAssets/{Blender,FBX,Textures,Audio,UI,References}/
```

(`.blend`, `.psd`, `.kra`, `.svg`, `.fbx`, `.wav` originali, file Substance, reference art.)

> **Nel progetto**: `SourceAssets/` **non esiste ancora** — va creata alla prima necessità reale, non in
> anticipo. ⚠️ **Git LFS è disattivato** su questo repo (budget esaurito, vedi `.gitattributes`): gli asset
> binari UE **non sono versionati** salvo due eccezioni esplicite nel `.gitignore`. Prima di introdurre
> binari grandi va deciso come versionarli — la regola generale «binari grandi via Git LFS» qui **non è
> attiva**.

## 10. Formato da usare quando si propone un asset

```text
Asset:        BP_Grid_HexGenerator
Percorso:     /Game/RT/World/Grid/Generation/BP_Grid_HexGenerator
Motivazione:  strumento di generazione della griglia, non un elemento runtime di una mappa specifica
Dipendenze:   Core, World/Grid, Data
Commit:       feat(grid): add hex graybox generator
```

Ogni proposta verifica anche: che non esista già una directory equivalente · che non nascano directory
globali per tipo · che non si introducano dipendenze inverse · se ci sono asset da spostare · se serve
`Fix Up Redirectors` · se va aggiornata questa documentazione.

## 11. Spostamenti

Gli asset si spostano **dal Content Browser o via API Editor**, mai da Esplora File. Dopo lo spostamento:

1. aggiorna tutti i riferimenti (inclusi quelli **hard-coded** in `Config/*.ini` e in C++ — vedi Appendice A);
2. `Fix Up Redirectors in Folder`;
3. controlla il Reference Viewer;
4. salva tutti gli asset modificati;
5. verifica che mappe e Blueprint si aprano senza errori;
6. esegui almeno un test PIE;
7. verifica il packaging se lo spostamento tocca asset runtime.

**Non eliminare i redirector a mano dal file system.**

## 12. Checklist di chiusura

- [ ] tutti gli asset proprietari sono sotto `/Game/RT`
- [ ] non sono nate cartelle globali (`Blueprints`, `Meshes`, `Textures`)
- [ ] gli asset specifici stanno vicino alla feature proprietaria
- [ ] `Shared` contiene solo asset realmente condivisi
- [ ] nessun asset runtime dipende da `Developers`, `Dev` o `Tests`
- [ ] nessun riferimento circolare evitabile
- [ ] nomi conformi a §6
- [ ] redirector e riferimenti corretti
- [ ] `L_DevSandbox` si apre
- [ ] il progetto compila · il PIE funziona · i test automatici passano

## 13. In caso di dubbio

Ordine di scelta: **1)** feature che possiede l'asset → **2)** personaggio o mappa specifica → **3)** sistema
condiviso → **4)** cartella globale solo se l'asset è realmente trasversale.

Non usare `Shared`, `Common`, `Misc`, `Other`, `Temp`, `General` come soluzione predefinita. Se non è chiaro
chi possiede l'asset: **fermati** e descrivi le due collocazioni possibili, le dipendenze di ciascuna, quale
raccomandi e perché è più semplice da mantenere.

## 14. Regola finale

La struttura deve rendere evidente: chi possiede ogni asset · quali feature lo usano · quali dipendenze
introduce · se è runtime, debug, test o third-party · se può essere eliminato insieme alla propria feature.

**Non ottimizzare per avere meno cartelle**: ottimizza per manutenibilità, migrazione, packaging,
collaborazione e prevenzione delle dipendenze circolari.

---

# Appendice A — Migrazione a `/Game/RT` (eseguita il 2026-08-05)

> **Fatta** come CP 6.0: 11 asset spostati con l'API Editor in modalità headless, verifica di integrità
> superata, suite Automation **172/172** verde. L'appendice resta come registro del *come*: le insidie
> incontrate valgono per qualunque riorganizzazione futura di `Content/`.

## A.1 Stato di partenza (non conforme)

11 asset proprietari organizzati **per tipo** — l'anti-pattern di §2. Nessuno sotto `/Game/RT`.

| Asset | Tracciato da Git | Note |
|---|---|---|
| `Content/Blueprints/BP_GameMode.uasset` | no | referenziato da `DefaultEngine.ini` |
| `Content/Blueprints/Units/BP_Unit_Guardian.uasset` | no | archetipo Guardian (mesh Paragon Gideon) |
| `Content/Blueprints/Units/BP_Unit_Ranger.uasset` | no | archetipo Ranger (mesh Paragon Sparrow) |
| `Content/Blueprints/Units/ABP_Gideon.uasset` | no | anim BP del Guardian |
| `Content/Blueprints/Units/ABP_Sparrow.uasset` | no | anim BP del Ranger |
| `Content/Maps/L_Prototype.umap` | **sì** | mappa del demo quadrato |
| `Content/Maps/L_DevSandbox.umap` | no | sandbox di sviluppo |
| `Content/Maps/DA_HexMap_Sandbox.uasset` | no | `URTHexMapAsset` della sandbox |
| `Content/Materials/M_Unit.uasset` | **sì** | materiale colorabile (parametro `Color`) |
| `Content/Materials/M_TeamRing.uasset` | no | anello di team sotto le unità |
| `Content/Materials/M_SelectionRing.uasset` | no | anello di selezione — **comparso a inventario iniziato**: ricontare gli asset subito prima di agire |

Terze parti: 22 pack `Content/Paragon*` (~3 GB l'uno) → **non si toccano** (§1). Presenti anche
`Content/Developers/` e `Content/Collections/` (cartelle di sistema UE).

## A.2 Mapping di migrazione

| Da | A | Motivazione |
|---|---|---|
| `/Game/Blueprints/BP_GameMode` | `/Game/RT/Core/Framework/BP_GameMode` | è il framework della partita: non appartiene a una mappa né a un personaggio (§5, §7 «Core non dipende da mappe») |
| `/Game/Blueprints/Units/BP_Unit_Guardian` | `/Game/RT/Characters/Guardian/Blueprints/BP_Unit_Guardian` | ogni personaggio è autonomo (§5) |
| `/Game/Blueprints/Units/BP_Unit_Ranger` | `/Game/RT/Characters/Ranger/Blueprints/BP_Unit_Ranger` | idem |
| `/Game/Blueprints/Units/ABP_Gideon` | `/Game/RT/Characters/Guardian/Animation/ABP_Guardian` | l'anim BP appartiene al personaggio di **gioco**, non al pack Paragon di origine; rinomina consigliata perché il nome attuale lega l'asset a una mesh sostituibile |
| `/Game/Blueprints/Units/ABP_Sparrow` | `/Game/RT/Characters/Ranger/Animation/ABP_Ranger` | idem |
| `/Game/Maps/L_DevSandbox` | `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox` | una cartella per mappa (§5) |
| `/Game/Maps/DA_HexMap_Sandbox` | `/Game/RT/Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox` | dato specifico della sandbox: se sparisce la mappa non serve più |
| `/Game/Maps/L_Prototype` | `/Game/RT/Maps/Dev/L_Prototype/L_Prototype` | mappa del demo quadrato; **in dismissione con M7** — si sposta solo per non lasciare residui in `/Game/Maps` |
| `/Game/Materials/M_TeamRing` | `/Game/RT/Characters/Shared/Materials/M_TeamRing` | usato da **entrambi** gli archetipi: `Shared` è legittimo (§5) |
| `/Game/Materials/M_Unit` | `/Game/RT/Art/GlobalMaterials/**M_Global_Tint**` | **vedi A.3**: non è un materiale di personaggio ma un tint parametrico trasversale — spostato **e rinominato** |

Cartelle risultanti: `Core/Framework`, `Characters/{Guardian,Ranger,Shared}`, `Maps/Dev/{L_DevSandbox,L_Prototype}`,
`Art/GlobalMaterials`. Le altre di §4 nascono quando avranno un asset.

## A.3 Decisione di collocazione: `M_Unit` (§13)

`M_Unit` è un materiale con parametro vettoriale `Color`, usato da:
- `ARTUnit` (colore squadra dell'unità) — `RTUnit.h:324`
- `ARTGridActor` (`TerrainMaterial`: piani colorati del terreno **e** evidenziazione hover) — `RTGridActor.h:121`

Quindi è usato da **due sistemi diversi** (Characters e World/Grid), non da due personaggi.

| Collocazione | Dipendenze introdotte | Valutazione |
|---|---|---|
| `Characters/Shared/Materials/` | `World/Grid` → `Characters` | ❌ dipendenza inversa: la griglia non deve dipendere dai personaggi (§7) |
| **`Art/GlobalMaterials/`** | tutti → `Art` (trasversale) | ✅ conforme a §13 punto 4: l'asset è realmente trasversale |

**Decisione (2026-08-05)**: `/Game/RT/Art/GlobalMaterials/` — l'unica opzione che non crea una dipendenza
inversa — **con rinomina a `M_Global_Tint`**. Il nome `M_Unit` mentirebbe sul proprietario (lo usa anche la
griglia per terreno e hover) e §6 chiede `<Tipo>_<Feature>_<Nome>`. Costo accettato: 3 riferimenti C++, i 2
Blueprint di unità e una eccezione del `.gitignore`; si paga **una volta sola** perché la rinomina viaggia
nello stesso giro di redirector dello spostamento.

Stessa logica per gli anim BP: `ABP_Gideon`/`ABP_Sparrow` prendono il nome dal pack Paragon di origine, ma
l'asset appartiene all'archetipo di gioco — che può cambiare mesh senza cambiare identità. Diventano
**`ABP_Guardian`** e **`ABP_Ranger`**.

## A.4 Riferimenti hard-coded da aggiornare (non coperti dai redirector dopo `Fix Up`)

**`Config/DefaultEngine.ini`**

```ini
GlobalDefaultGameMode=/Game/Blueprints/BP_GameMode.BP_GameMode_C   →  /Game/RT/Core/Framework/BP_GameMode.BP_GameMode_C
GameDefaultMap=/Game/Maps/L_Prototype                              →  /Game/RT/Maps/Dev/L_Prototype/L_Prototype
EditorStartupMap=/Game/Maps/L_Prototype                            →  /Game/RT/Maps/Dev/L_Prototype/L_Prototype
```

**C++** — il vecchio path `/Game/Materials/M_Unit.M_Unit` diventa
`/Game/RT/Art/GlobalMaterials/M_Global_Tint.M_Global_Tint`:

| File | Riga | Contenuto |
|---|---|---|
| `Source/RefactorTactics/Grid/RTGridActor.h` | 121 | default di `TerrainMaterial` |
| `Source/RefactorTactics/Unit/RTUnit.h` | 320, 324 | commento + default di `TeamColorMaterial` |
| `Source/RefactorTactics/Unit/RTUnit.cpp` | 107 | testo del warning «crea /Game/Materials/M_Unit» |

**`.gitignore`** — le due eccezioni al blocco dei binari puntano ai percorsi vecchi e vanno riscritte, altrimenti
i due asset versionati escono silenziosamente dal controllo di versione:

```gitignore
!Content/Materials/M_Unit.uasset   →  !Content/RT/Art/GlobalMaterials/M_Global_Tint.uasset
!Content/Maps/L_Prototype.umap     →  !Content/RT/Maps/Dev/L_Prototype/L_Prototype.umap
```

⚠️ Con la rinomina il file **cambia nome oltre che cartella**: per Git è una cancellazione + un'aggiunta.
Verifica dopo il commit che `git ls-files Content/` elenchi ancora **due** asset, non zero.

## A.5 Procedura seguita (e perché in quest'ordine)

Eseguita **headless** con l'API Editor via `PythonScriptPlugin` — §11 consente il Content Browser *o* l'API
Editor. Il plugin è stato abilitato nel `.uproject` solo per la migrazione e poi rimosso.

```
UnrealEditor-Cmd.exe <project> -run=pythonscript -script="<script>.py" -unattended -nopause -nosplash -nullrhi
```

1. **Backup** degli asset proprietari fuori dal repo (432 KB). Sono **non versionati** (§9): senza copia non
   esiste un annulla.
2. Editor chiuso (due processi che scrivono sugli stessi `.uasset` li corrompono).
3. Patch di A.4 ai riferimenti testuali (`DefaultEngine.ini`, 3 punti C++) **prima** dello spostamento,
   + **ricompilazione** del target Editor.
4. Spostamento: `AssetTools.rename_assets` per gli asset normali, *duplicate + delete* per le mappe.
5. Resave di `/Game/RT`, rimozione dei redirector orfani e delle cartelle vuote.
6. **Riparazione dei soft reference** dei `BP_Unit_*` (vedi A.6).
7. Verifica: script di integrità + suite Automation + `.gitignore` aggiornato.

## A.6 Insidie incontrate (valgono per ogni migrazione futura)

**1. Il dialogo che annulla tutto.** `EditorAssetLibrary.rename_asset` apre *«Source code, config INI, and
text files may need Find/Replace for: X … Continue with rename?»* per gli asset citati in file di testo. Con
`-unattended` il dialogo si chiude da solo con **Cancel** e il rename fallisce — e con `rename_assets` in
batch **un solo Cancel annulla l'intero lotto**. Rimedio: aggiornare INI e C++ *prima*, così il controllo non
ha più nulla da segnalare, e ricompilare (il soft reference a `M_Unit` vive nel CDO generato dal binario:
finché non si ricompila, UE continua a vederlo).

**2. Le mappe avvisano comunque.** Anche dopo la pulizia dei riferimenti, i World continuano ad aprire il
dialogo. Per le due mappe si è usato `duplicate_asset` + `delete_asset`: legittimo *solo* perché nessun asset
referenzia una mappa (i soli riferimenti erano negli INI, già aggiornati).

**3. `delete_asset` su un World mente.** Ritorna `True` ma il `.umap` resta su disco, lasciando **due copie**
della stessa mappa. Verificare sempre con `find`, non fidarsi del valore di ritorno. I due file residui sono
stati rimossi dal file system — deviazione da §11 accettabile qui perché erano duplicati **senza referenti**.

**4. Il resave NON aggiorna i soft reference.** `TeamRingMaterial`/`SelectionRingMaterial` dei `BP_Unit_*` sono
`TSoftObjectPtr`: a tenerli validi era il redirector. Eliminato il redirector insieme a `/Game/Materials`, i
due riferimenti sono diventati orfani (letti dal CDO valevano `None`: **gli anelli sarebbero spariti in
gioco**). Riparati riassegnando i materiali sul CDO + `compile_blueprint` + save. *Prima di cancellare un
redirector, verifica che nessun soft reference dipendesse da lui.*

**5. L'asset registry mente entro la stessa sessione.** Subito dopo una modifica, `get_dependencies` e
`get_referencers` restituiscono dati stale (una cartella risultava «assente» mentre le dipendenze la
citavano). **Ogni verifica va rifatta in un processo nuovo**, altrimenti si insegue un fantasma.

## A.7 Esito verificato

| Verifica | Esito |
|---|---|
| 11 asset presenti nei nuovi percorsi | ✅ |
| `Content/Blueprints`, `Content/Materials`, `Content/Maps` rimosse | ✅ |
| GameMode dell'INI caricabile e classe risolta | ✅ |
| `BP_Unit_*` senza dipendenze sui vecchi path | ✅ (dopo A.6 punto 4) |
| `L_DevSandbox` punta al nuovo `DA_HexMap_Sandbox` | ✅ |
| Nessun redirector residuo in `/Game/RT` | ✅ |
| Suite Automation | ✅ **172/172**, 0 fail |
| I 2 asset versionati restano tracciati nei nuovi percorsi | ✅ |

**Resta da fare in editor** (non verificabile headless): un **PIE su `L_Prototype`** che confermi unità
colorate, anello di team e anello di selezione visibili — è la prova finale del punto 4 di A.6 — e l'apertura
di `L_DevSandbox` con la griglia esagonale. Voci `PIE-AS5`/`PIE-SEL` in
[`test-manuali-pie.md`](test-manuali-pie.md).

---

# Appendice B — Asset di terze parti: nel progetto, fuori dal repo (dal 2026-08-06)

> I pack scaricati da **Fab** stanno in **`Content/FabAsset/<Fornitore>/<Pack>/`** — dentro il progetto, ma
> **fuori dal repository**. Chi clona se li riscarica e li rimette lì; il progetto compila e parte anche senza
> (unità col cilindro segnaposto, fallback di `ARTGameMode`).

## B.1 Struttura

```
Content/
  RT/                          <- asset proprietari (§1)
  FabAsset/
    Paragon/
      ParagonGideon/           <- /Game/FabAsset/Paragon/ParagonGideon/...
      ParagonSparrow/
      ...                      (26 pack, 48,1 GB, 39.589 file)
```

Il livello `<Fornitore>` (`Paragon/`) tiene aperta la porta ad altri fornitori (Megascans, Marketplace) senza
rimescolare quanto già presente. Resta valido il §1: un pack **non si sposta dentro `/Game/RT`**.

`.gitignore` la salta **come directory** (`/Content/FabAsset/`), non per estensione: con un pattern per file
git attraverserebbe la cartella e statterebbe ~40.000 file a ogni `git status` (misurato dopo: **47 ms**).

> ⚠️ **`git clean -fdx` cancella questi 48 GB** — `-x` include gli ignorati, e ora i pack sono dentro il repo
> (nel vault esterno erano fuori portata). Usare `git clean -fd`, oppure `git clean -fdx -e Content/FabAsset`.

## B.1b Il costo vero non è lo spazio su disco

I pack inutilizzati **non sono inerti**: l'asset registry li scansiona a ogni avvio dell'editor, e ne
risentono apertura, DDC e cook. Il sintomo che lo fece scoprire (2026-08-05) fu il **Content Browser che non
mostrava le cartelle appena create** da A.1: l'editor stava ancora indicizzando 48 GB.

Tenerli nel progetto **accetta** quel costo. Non è però un peggioramento rispetto alle junction che c'erano
prima: anche quelle venivano indicizzate come se fossero locali. Chi vuole recuperare il tempo di avvio deve
**togliere i pack che non usa**, non spostarli altrove lasciando un link.

## B.2 Come aggiungere un pack

1. Si scarica il pack da Fab e lo si mette in `Content/FabAsset/<Fornitore>/<Pack>/`.
2. Se l'Epic Games Launcher lo installa altrove (tipicamente in `Content/<Pack>/`), lo spostamento **non si
   fa da Esplora File**: il path virtuale segue la posizione su disco, e gli asset citano i propri
   riferimenti **per path assoluto** (`/Game/<Pack>/...`, verificato: 230 occorrenze in 40 asset campionati).
   Spostando i file a mano si ottiene un pack che *sembra* a posto e ha ogni mesh senza materiali. Si usa il
   **Content Browser** (o lo script di B.3), che riscrive i riferimenti.
3. Se serve **solo una parte** di un pack, l'alternativa migliore è il **Migrate** da un progetto-magazzino:
   porta l'asset e **solo le sue dipendenze**. `ParagonGideon` pesa 2,74 GB, ma a un `BP_Unit` servono mesh,
   scheletro, poche animazioni e i relativi materiali.

## B.2b Storia: il vault esterno (2026-08-05 → 2026-08-06)

Per un giorno i pack hanno vissuto **fuori** dal progetto, in `D:\UE_AssetVault\` (con un `.uproject`
minimale che rendeva il magazzino apribile, perché *Migrate* esiste solo dentro l'editor e l'editor apre
progetti, non cartelle). Il progetto li vedeva tramite **junction** `Content/Paragon*` → vault.

Perché si è tornati indietro (decisione utente, 2026-08-06):

- il vantaggio di avvio era **già annullato dalle junction** — ciò che è linkato viene indicizzato come se
  fosse locale, quindi si pagavano comunque 48 GB;
- restava però lo **svantaggio**: il progetto dipendeva da un percorso esterno, e spostare o rinominare
  `D:\UE_AssetVault` rompeva le junction e con loro ogni Blueprint che referenziasse quei pack.

Un pack **linkato** non rende autosufficiente l'asset proprietario che lo usa. Con i file nel progetto,
l'unica dipendenza esterna che resta è il download da Fab, documentato nel README.

## B.3 Come è stata fatta (2026-08-06) — e cosa costa ignorarlo

Verifica **prima** di toccare qualsiasi cosa: nessun riferimento ai pack in `Config/`, in `Source/` e negli
asset di `Content/RT/` (i `BP_Unit_*`/`ABP_*` che li usavano erano già stati rimossi in A.1). I 26 pack sono
quindi **privi di referenti**: si potevano spostare tutti.

Poi due passaggi. Il primo è file system puro: rimozione delle junction (`rmdir` sul link, **mai**
`Remove-Item -Recurse`, che seguirebbe il reparse point e cancellerebbe il bersaglio) e rename delle cartelle
dal vault a `Content/` — vault e progetto sullo stesso disco, quindi istantaneo anche per 48 GB.

Il secondo è il rename del path virtuale `/Game/<Pack>` → `/Game/FabAsset/Paragon/<Pack>`, headless via
`PythonScriptPlugin` (abilitato temporaneamente nel `.uproject`) con
`UnrealEditor-Cmd -run=pythonscript -script=… -unattended -nullrhi`. Le insidie, in ordine di quanto costano:

1. **I `DialogueWave` referenziati dai `SoundCue` si perdono.** È la più costosa e non ha rimedio noto:
   spariscono dal disco senza comparire nei log, **~15-18% dei file di un pack** (297 su ParagonZinx, 305 su
   ParagonYin, 340 su ParagonDrongo). I `DialogueWave` *non* referenziati (`*_Effort_*`) sopravvivono; quelli
   citati da un Cue (`*_Engage_*`) no, e il Cue resta con il riferimento irrisolto (51 su ParagonDrongo).
   Riproducibile **sia** con `rename_directory` **sia** con `rename_assets`: è nel motore. Sono dialoghi
   vocali dei personaggi Paragon — perdita **accettata** (decisione utente), ma va saputa prima di iniziare.
   ⚠️ L'asset registry dichiara `destinazione=1655` mentre sul disco i file sono 1350: **il conteggio di UE
   non è una verifica**. Contare i file prima e dopo, e confrontare gli elenchi.
2. **`rename_directory` è atomico**: un solo asset non caricabile annulla l'**intera cartella** (`RenameDirectory
   failed: … exists but was not able to be loaded`). In questi pack bastavano **22 asset** su 39.589 — i Rig
   legacy UE4 (`*_Proto_Retarget`, `*_SkeletonRig`, classe `URig` rimossa in UE5) — per bloccare **21 pack su
   26**. Rimedio: metterli in **quarantena fuori da `Content/`** prima del rename e rimetterli a posto dopo.
   Essendo non caricabili non hanno riferimenti da aggiornare, quindi lì lo spostamento a mano è lecito.
3. **Non usare `rename_assets` a lotti** come alternativa: UE 5.8 crasha in
   `ObjectTools::CleanupAfterSuccessfulDelete()` (access violation), e l'asset registry interrogato subito
   dopo l'avvio vede **una frazione** del pack (914 asset su 1911) perché sta ancora indicizzando, quindi
   servono più giri e ogni giro può crashare. `rename_directory` vede l'intero pack e non crasha.
4. **Non "completare" a mano sul file system.** È l'errore che è costato di più: uno script che spostava i
   residui per far tornare i conti ha prodotto un `ParagonDekker` con **1310 asset dai riferimenti rotti**,
   183 buoni e 110 misti — un pack che sembra migrato e non lo è.
5. **I file lockati restano indietro**: UE non riesce a cancellare gli originali di `.umap` e `_BuiltData`
   (`RenameDirectory: Could not delete the original directory but the assets have been renamed`). Hanno già la
   loro controparte risalvata a destinazione — hash diverso, appunto perché riscritta — quindi si eliminano.
6. **L'asset registry dà dati stale** entro la stessa sessione: un processo per pack, e le verifiche in un
   processo nuovo.

Verifica di correttezza, non di conteggio: un asset a destinazione deve citare **solo** `/Game/FabAsset/…`.
Se cita ancora il path vecchio è stato spostato, non rinominato.

## B.4 Regola operativa

Prima di aggiungere un pack al progetto, chiediti: **lo sto usando adesso?** Se la risposta è «servirà più
avanti», scaricalo quando servirà. Un pack inutilizzato non costa spazio su disco: costa **tempo di editor a
ogni avvio** (B.1b) e 48 GB che nessun backup del repo protegge.
