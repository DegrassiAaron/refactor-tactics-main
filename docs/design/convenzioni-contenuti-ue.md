# RefactorTactics — Convenzioni per la struttura dei contenuti Unreal

> **Stato**: normativo · **Ultimo aggiornamento**: 2026-08-05
> Regole vincolanti per la struttura di `Content/`. Si applicano quando si creano asset, si suggeriscono
> percorsi, si modificano asset esistenti, si generano script Editor Utility, si propone un refactoring o si
> implementa una feature. **Non creare directory alternative senza una motivazione tecnica esplicita.**
>
> Lo **stato attuale del progetto non è ancora conforme**: il piano di allineamento è nell'**Appendice A**.

---

## 1. Namespace principale

Tutti gli asset proprietari stanno sotto `/Game/RT/` (fisicamente `Content/RT/`).

**Non** creare asset proprietari direttamente in `/Game/`, `/Game/Blueprints/`, `/Game/Materials/`,
`/Game/Textures/`, `/Game/Maps/`.

Gli asset Marketplace, Paragon, Megascans e di terze parti **restano nelle proprie directory originali** o
sotto `/Game/ThirdParty/`. Non spostarli dentro `/Game/RT` senza aver verificato riferimenti, licenze e
dipendenze.

> **Nel progetto**: i 22 pack `Content/Paragon*` sono asset Epic di terze parti e **restano dove sono**
> (sono anche esclusi dal versionamento, vedi §9). Lo spostamento non porterebbe benefici e moltiplicherebbe
> i redirector.

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

# Appendice A — Stato attuale e piano di migrazione

> Rilevato il **2026-08-05**. Da eseguire come **CP 6.0** di M6 (prima di creare gli asset nuovi della
> milestone, così nascono già nella struttura corretta).

## A.1 Stato attuale (non conforme)

10 asset proprietari, organizzati **per tipo** — l'anti-pattern di §2. Nessuno sta sotto `/Game/RT`.

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

Terze parti: 22 pack `Content/Paragon*` → **non si toccano** (§1). Presenti anche `Content/Developers/` e
`Content/Collections/` (cartelle di sistema UE).

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

## A.5 Procedura (ordine obbligato)

L'ordine conta: `Fix Up Redirectors` **elimina** i redirector, e da quel momento ogni percorso hard-coded non
aggiornato punta nel vuoto.

1. **Editor chiuso** → commit di partenza pulito (punto di ritorno).
2. Editor aperto, Content Browser: crea le cartelle di A.2 e **sposta** gli asset con drag&drop
   (*Move Here*, non *Copy Here*). Rinomina `ABP_Gideon`→`ABP_Guardian`, `ABP_Sparrow`→`ABP_Ranger`,
   `M_Unit`→`M_Global_Tint`. **Salva tutto** (`Ctrl+Shift+S`).
3. **Non ancora** `Fix Up`: chiudi l'editor.
4. Applica le patch di A.4 (`.ini`, C++, `.gitignore`) e **ricompila** il target Editor.
5. Riapri l'editor: verifica che `L_Prototype` sia la mappa di avvio e che il GameMode sia risolto.
6. `Fix Up Redirectors in Folder` su `Content/` (o almeno sulle cartelle di origine vuote).
7. Elimina le cartelle rimaste vuote (`Content/Blueprints`, `Content/Materials`, `Content/Maps`).
8. **Verifica**: PIE su `L_Prototype` (unità colorate, anelli, GameMode attivo) e apertura di `L_DevSandbox`
   con `ARTHexMapActor` che mostra la griglia; suite Automation verde; `git status` senza sparizioni impreviste.

**Commit proposto**: `refactor(content): struttura feature-first sotto /Game/RT`

## A.6 Limiti dichiarati

Gli asset `.uasset`/`.umap` sono binari e vanno spostati **dall'editor** (§11): non sono spostabili da questa
sessione. Il passo 2 richiede l'utente; i passi 4 e le patch di A.4 sono preparabili e applicabili da qui,
**solo dopo** che lo spostamento è avvenuto — applicarle prima lascerebbe il progetto con percorsi rotti.
