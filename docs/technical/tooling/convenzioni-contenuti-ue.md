# RefactorTactics — Convenzioni per la struttura dei contenuti Unreal

> **Stato**: normativo · **Ultimo aggiornamento**: 2026-08-18
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
/Game/RT/Characters/Gadget/          <- nome del PACK Paragon, non dell'eroe (§5b): qui vive Gadget
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

## 4. Sottoinsieme in uso

La struttura completa di §3 è il **target**; il progetto ne usa oggi solo questo sottoinsieme. Quali cartelle
servano dipende dal lavoro in corso, ma **questo documento non è un tracker**: per sapere a che punto è il
progetto si legge [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md), che ne è l'unico owner.

Per la domanda vicina ma diversa — **quali asset servono, quali esistono e quanti ne mancano** — l'owner è
[`asset-map.md`](asset-map.md) *(dal 2026-08-13)*, che deriva da qui ogni percorso e misura lo stato
sull'allowlist di `.gitignore`.

*(Fino al 2026-08-08 questa sezione si intitolava «struttura attiva nella milestone corrente» e nominava la
**M6 — Parità hex**, chiusa da tempo: una convenzione che invecchia insieme a una milestone smette di essere
una convenzione.)*

```text
/Game/RT/
├── Core/            Framework/ · Input/ · Camera/
├── Systems/         Planning/ · Turns/ · Resolution/ · Movement/
├── World/Grid/      Hex/ · Selection/ · Visualization/ · Generation/
├── World/Graybox/   Cover/ · Doors/ · Surfaces/ · Volumes/     (kit CONDIVISO, D-173)
├── Characters/      Shared/{Blueprints,Data,Materials}
├── Maps/Dev/        L_DevSandbox/{Data,Graybox,Tests}          (Graybox LOCALE della mappa)
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
| Kit graybox degli **oggetti** | `/Game/RT/World/Graybox/` | primitive posabili riusabili fra mappe: `Cover/` · `Doors/` · `Surfaces/` · `Volumes/`. **D-173** ⚠️ Non è `Grid/Generation/`, che sono i **generatori**: qui ci sono gli oggetti che si posano. Owner del contratto di ingombro e pivot: [`spec-graybox-placement-contract.md`](../systems/spec-graybox-placement-contract.md) |
| Turni | `/Game/RT/Systems/` | `Planning/` intenti · `Turns/` fase e Ready · `Resolution/` playback · `Movement/` configurazione |
| Personaggi | `/Game/RT/Characters/<CharacterId>/` | ogni personaggio è **autonomo**. ⚠️ `<CharacterId>` = nome del **pack Paragon** (§5b) |
| Abilità di un personaggio | `/Game/RT/Characters/<CharacterId>/Abilities/` | |
| Abilità realmente condivise | `/Game/RT/Abilities/{Shared,Targeting,Effects,Cues}/` | |
| Mappe | `/Game/RT/Maps/<Category>/<MapName>/` | una cartella per mappa |
| UI | `/Game/RT/UI/<funzione>/` | divisa per funzione, non per tipo tecnico |
| **Strumenti di authoring solo-Editor** | `/Game/RT/Editor/<Strumento>/` | **D-280**. Widget e asset che servono a *costruire* il gioco e non ne fanno parte: `Editor/Scenario/` per il Composer (#1804), `Editor/GrayKit/UI/` per il Playground Panel (**D-304**). ⚠️ **La cartella è organizzazione: `/Editor/` non garantirebbe da sé l'esclusione dal cook** — `Config/DefaultGame.ini` dichiara `+DirectoriesToAlwaysCook=(Path="/Game/RT")`, che coprirebbe anche questa. ✅ **Dal 2026-09-02 l'esclusione è configurata** ([#1804](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1804)): `+DirectoriesToNeverCook=(Path="/Game/RT/Editor")`, letta dal cooker in `CookOnTheFlyServer.cpp:12463` e **solo in CookByTheBook**, cioè la via di `BuildCookRun`. 🔴 **È una famiglia aperta, e il rischio è il tuo**: tutto ciò che metti qui sotto sparirà dal pacchetto **per sempre e in silenzio** — nessun avviso, nessun errore di cook. È lo scopo del namespace, ma **se un asset serve a runtime non va qui.** Presidiata da `RefactorTactics.Packaging.EditorNamespaceIsNeverCooked`, che ⛔ verifica la *configurazione*, non il contenuto del pacchetto: quell'oracolo è `PIE-PKG-EDITOR-NAMESPACE`. ⚠️ La riga d'allowlist esiste da 2026-08-31: `!Content/RT/Editor/**/*.uasset` |
| **Scene di sviluppo e laboratorio** | `/Game/RT/Maps/Dev/<MapName>/` | Non sono strumenti, sono mappe: restano sotto `Maps/` con le altre. `L_DevSandbox`, `L_HexArena`, `L_Prototype` e — da **D-304** — `L_GrayKitPlayground`. ⛔ **Non vanno sotto `/Game/RT/Editor/`**: D-280 sceglie il namespace degli *strumenti*, e una scena non lo e'. Per una mappa l'esclusione dal pacchetto e' l'assenza da `MapsToCook` piu' l'assenza di riferimenti, come `DefaultGame.ini` dichiara misurandolo su un pacchetto vero |
| Dati di feature | vicino alla feature | `Characters/Gadget/Data/DA_Hero_Gadget` — cartella del pack, nome dell'**eroe** (§5b) |
| Dati globali e cataloghi | `/Game/RT/Data/` | `Data/Catalogs/DA_HeroCatalog`, `Data/Rulesets/DA_Ruleset_Dev` |

⚠️ **Due cartelle si chiamano `Graybox`, e la regola che decide quale usare esiste già — non è una nuova
eccezione.** `World/Graybox/` è il **kit condiviso**; `Maps/Dev/L_DevSandbox/Graybox/` è materiale graybox
**locale a quella mappa**. Le distingue la **regola decisiva sugli asset di mappa** enunciata in §5b: *se
eliminando la mappa l'asset non serve più, può restare nella cartella della mappa; se è usato da più mappe,
va in una cartella condivisa.* Non è il caso di `Grid/Generation/` (i **generatori**) accanto a un ipotetico
`Grid/Graybox/` (gli **oggetti**), dove due cose di natura diversa avrebbero portato lo stesso nome: qui la
natura è la stessa e cambia lo **scope**, che è precisamente ciò che quel criterio governa. *Registrato il
2026-08-18 perché `D-173` scartava un'alternativa per ambiguità senza dire perché questa non lo è — trovato
in code review su #1188.*

**La logica autorevole C++ resta sotto `Source/`, mai dentro `Content/`** (invariante #1: le regole decidono
l'esito, gli asset no).

`Characters/Shared` contiene **solo** asset usati concretamente da almeno due personaggi — non è un
parcheggio.

## 5b. `<CharacterId>` è il nome del PACK, non dell'eroe (dal 2026-08-11)

Decisione dell'autore, motivata così: *«allineamo i nomi utilizzati a quelli veri di Paragon, per non creare
problemi»*. In editor si vede `Gadget`, quindi si cerca `Gadget`: il salto mentale eroe→pack sparisce, e con
esso l'errore di aprire la cartella sbagliata.

| Eroe (gioco) | `HeroId` (C++) | `<CharacterId>` (contenuti) |
|---|---|---|
| Gadget | `Hero.Gadget` | **`Gadget`** |
| Phase | `Hero.Phase` | **`Phase`** |
| Riktor | `Hero.Riktor` | **`Riktor`** |
| Wraith | `Hero.Wraith` | **`Wraith`** |

Vale per la cartella e per gli asset di **presentazione**: `Characters/Gadget/Blueprints/BP_Unit_Gadget`,
`Characters/Gadget/Animation/AM_Gadget_Attack`.

⚠️ *Questo esempio citava `ABP_Gadget` fino al 2026-08-30. La **regola di naming non cambia** — il nome
segue il pack — ma quell'asset non è più previsto: il grafo di animazione vive in C++
([D-248](../../decisions/RT_PDR_00_Decision_Log.md)), e in `Animation/` ci vanno i montaggi.*

**Eccezione: i dati restano intitolati all'eroe.** `DA_Hero_Gadget` sta in `Characters/Gadget/Data/` ma non
diventa `DA_Hero_Gadget`. Un data asset eroe descrive *statistiche e abilità*, che non dipendono dalla mesh:
se Gadget cambiasse base visuale, quel file resterebbe valido parola per parola. Lo stesso vale per `HeroId`,
che in C++ è e resta `Hero.Gadget`. La mappatura fra i due mondi è **D-037**, tabella owner in
[`../../characters/paragon.md`](../../characters/paragon.md).

⚠️ **Costo accettato, non rimosso.** Fino al 2026-08-11 §A raccomandava l'**opposto** — il nome del
personaggio di gioco — con l'argomento che *«il nome del pack lega l'asset a una mesh sostituibile»*.
L'argomento resta vero: se un eroe cambia base visuale, `BP_Unit_Gadget` diventa un nome falso e va
rinominato. La scelta è di privilegiare chi lavora in editor **oggi** rispetto a un rename ipotetico domani,
ed è consapevole. Chi la ribaltasse di nuovo tocchi anche `editor-sessions.yaml` (U7/U8) e l'allowlist di
`.gitignore`: gli otto path degli artefatti vi sono elencati **per esteso**, quindi un rename li rende muti
senza che niente lo segnali.

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

Esempi validi: `BP_Grid_HexGenerator`, `WBP_Planning_ActionBar`, `DA_Ability_Gadget_ArcShot`,
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
| `/Game/RT/Tests` | Functional Test, mappe di test, fixture, replay deterministici — **asset**, non dati testuali |
| `/Game/ThirdParty` | asset esterni **privi** di un namespace proprio |

> **Gli scenari dello Scenario Test Harness non stanno in `Content/`.** Vivono in **`Scenarios/`, alla radice
> del repository**: sono JSON versionati, si leggono in una diff e si modificano senza aprire l'editor. Metterli
> sotto `/Game/RT/Tests` li avrebbe trasformati in `.uasset` binari, cioè in qualcosa che nessuna code review
> può leggere. Vedi [`test-automatico-unreal.md`](test-automatico-unreal.md).

Non modificare direttamente un asset esterno se puoi crearne una variante proprietaria sotto `/Game/RT`.

## 9. File sorgente esterni

I sorgenti non importati in Unreal non stanno in `Content/`:

```text
<ProjectRoot>/SourceAssets/{Blender,FBX,Textures,Audio,UI,References}/
```

(`.blend`, `.psd`, `.kra`, `.svg`, `.fbx`, `.wav` originali, file Substance, reference art.)

> **Nel progetto**: `SourceAssets/` **non esiste ancora** — va creata alla prima necessità reale, non in
> anticipo. ⚠️ **Git LFS non è attivo** su questo repo: gli asset binari UE **non sono versionati** salvo
> le riammissioni per ruolo dichiarate nel `.gitignore`. Prima di introdurre binari grandi va deciso come
> versionarli — la regola generale «binari grandi via Git LFS» qui **non è attiva**.
> 🔴 *Diceva «disattivato (budget esaurito, vedi `.gitattributes`) [...] salvo **due** eccezioni
> esplicite» fino al 2026-08-29 ([#1659](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1659)). Due difetti in una riga: `.gitattributes` **non esiste**, quindi il
> rimando non porta a nulla; e le riammissioni non sono due — si contano con `grep -c "^!Content" .gitignore`,
> che è lo stesso comando che [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) `FMT-2` prescrive di rieseguire
> invece di copiare, dopo che quel numero era già scaduto una volta.*

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

## 11-bis. La scala: il lato dell'esagono

Chi modella una mesh, una porta o un pezzo di architettura ha bisogno di un riferimento metrico. La scala
adottata è:

```text
lato dell'esagono = 1,5 m        (esattamente: HexSize = 150)
```

> ⏱️ **Questa riga diceva «la scala di *authoring*» e «≈ 1,5 m» fino al 2026-08-17.** Da
> [`D-163`](../../decisions/RT_PDR_00_Decision_Log.md) non è più solo d'authoring — governa anche il mondo — e
> il `≈` non è più approssimativo: il valore è esatto. La correzione sta qui e non solo in §11-bis.1 perché
> **gli altri documenti citano «§11-bis», non la sotto-sezione**, e leggerebbero la versione vecchia.

Serve a dimensionare **mesh, proporzioni e architettura**, così che una porta sembri una porta accanto a
un'unità e due strutture autorate da persone diverse combacino — **e dal 2026-08-17 è anche la scala del
mondo** (§11-bis.1): non è più solo una convenzione di modellazione, e `HexSize` non è libero.

> ⚠️ **Non è una metrica di design, e non va usata come tale.**
> [`../../gameplay/spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) è
> esplicito: «Metrica primaria — **non i metri**, non il numero assoluto di celle». Una mappa si dimensiona
> contando i **Move** necessari a raggiungere le zone rilevanti, non i metri quadrati; e
> [`D-030`](../../decisions/RT_PDR_00_Decision_Log.md) ribadisce che il canone non fissa un numero di celle.
>
> Le due cose convivono perché rispondono a domande diverse: *quanto è grande questo modello* è una domanda
> d'arte, *quanto è grande questa mappa* è una domanda di design. La prima ha un metro, la seconda no.

Non derivare da questo valore raggi d'abilità, portate o costi di movimento: quelli sono in celle, e la cella
è l'unità del gioco. Deciso con l'autore il **2026-08-09**, in sede di consolidamento del cluster Map &
Environment ([triage](../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md) §8.1).

### 11-bis.1 — La scala del mondo segue questa, e per otto giorni non l'ha fatto

✅ **Confermata dall'autore il 2026-08-17** e promossa da scala d'*authoring* a scala **del mondo**
([`D-163`](../../decisions/RT_PDR_00_Decision_Log.md)). Il lato è `1,5 m`; l'unità di `HexSize` è il
centimetro — lo dichiara `RTHexMapAsset.h` accanto alla proprietà — e la **dimensione** è il circumraggio,
cioè il lato (`RTHexLibrary.cpp`: *«un pointy-top di circumraggio HexSize»*). Quindi il valore canonico è
**`HexSize = 150`**.

**La quota fra i piani resta `2,50 m`** (`LayerHeight = 250`) e non segue la larghezza — decisione
d'autore dello stesso giorno. Chi modella in verticale ha quindi **due riferimenti indipendenti**: il lato
della cella per la pianta, la quota di piano per l'alzato. Non si deriva l'uno dall'altro.

| | Scala d'arte | Scala del mondo, **fino al** 2026-08-25 | Scala del mondo, **oggi** (canone) |
|---|---:|---:|---:|
| Lato della cella | 1,50 m *(dal 2026-08-09)* | 1,00 m | **1,50 m** |
| Cella lato-a-lato (`C`) | 2,60 m | 1,73 m | **2,60 m** |
| Altezza del volume | — | 2,50 m | **2,50 m** *(già allineata)* |

⚠️ **La colonna che cambia il 2026-08-17 è la terza, non la prima**: la scala d'arte è quella dal
2026-08-09 e `D-163` non l'ha toccata — ha deciso che il **mondo** la segue. Chi ha modellato fra il 09 e
il 17 ha modellato giusto. ✅ **E dal 2026-08-25 la colonna che il gioco usa è la terza**: `#1155` è atterrata, `HexSize = 150`, e le
tre colonne coincidono per il lato. 🔴 *Questa riga diceva «finché la issue di migrazione è aperta, la
colonna che il gioco usa è la **seconda**» — la migrazione era chiusa da tre giorni, e l'intestazione della
seconda colonna diceva «oggi». Corrette entrambe il 2026-08-28: chi leggeva la tabella top-down modellava
a `1,00 m`, cioè il difetto che il paragrafo qui sotto dichiara chiuso.*

> 🔴 **Fino al 2026-08-17 questa sezione descriveva una scala che nessuna mappa usava.** Misurato:
> `HexSize` non compare in **nessun** binario di `Content/RT` — misura e oracolo in
> [`D-163`](../../decisions/RT_PDR_00_Decision_Log.md) — quindi ogni mappa restava al default `100.f` — lato `1,00 m`, e una divergenza di **1,5×** fra ciò che si modellava e il mondo in cui
> atterrava. Non era un errore di questa sezione: era che nessuno aveva chiuso il cerchio sul codice.
> `GBX-6` in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) ha reso visibile il divario e `D-163` lo chiude.

✅ **[`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è atterrata il 2026-08-25: il mondo gira a `1,5 m`**, la stessa scala con cui questa
sezione dice di modellare. Le due scale coincidono, quindi un asset autorato qui è corretto per il canone
**e** per la mappa in cui si posa.

🔴 **Questa sezione diceva il contrario fino al 2026-08-28**: *«finché `#1155` è aperta il mondo gira ancora
a `1,00 m`»*, e concludeva che conveniva rimandare **il commit** dei volumi finiti «prima che il cambio
atterri». Il cambio è atterrato tre giorni prima, e la clausola gli è sopravvissuta — è la stessa deriva
corretta in [`D-173`](../../decisions/RT_PDR_00_Decision_Log.md), che cita questa sezione come normativa:
chi seguiva il rinvio atterrava sull'istruzione che lo smentiva.

**Il commit non attende più nulla.** La validazione *in PIE* resta il solo modo di chiudere `GBX-1` e
`GBX-5`, ma è un gate di qualità e non un divieto di versionare: confonderli tiene fuori dal repository
asset che ci possono entrare.

> ⚠️ *La prima stesura diceva «una ragione per non produrre volumi finiti», mentre
> [`spec-graybox-placement-contract.md`](../systems/spec-graybox-placement-contract.md) §6.1 diceva «modella a
> `2,60 m`»: due owner con istruzioni opposte per la stessa persona, e nessuna gerarchia dichiarata fra
> loro. Trovato in code review.*

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
| `/Game/Blueprints/Units/ABP_Gideon` | `/Game/RT/Characters/Guardian/Animation/ABP_Guardian` | ~~l'anim BP appartiene al personaggio di **gioco**, non al pack Paragon di origine; rinomina consigliata perché il nome attuale lega l'asset a una mesh sostituibile~~ → **superata da [§5b](#5b-characterid-è-il-nome-del-pack-non-delleroe-dal-2026-08-11)** (2026-08-11): la regola è ora l'opposto, il nome segue il **pack**. Questa riga resta come registro di CP 6.0 |
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
- ~~`ARTGridActor` (`TerrainMaterial`: piani colorati del terreno **e** evidenziazione hover)~~ — **la classe
  non esiste più**, rimossa col substrato quadrato al CP 7.2. Il ruolo è oggi di `ARTHexMapActor`, che
  renderizza le celle via ISM. *(Corretto il 2026-08-08.)*

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

~~Stessa logica per gli anim BP: `ABP_Gideon`/`ABP_Sparrow` prendono il nome dal pack Paragon di origine, ma
l'asset appartiene all'archetipo di gioco — che può cambiare mesh senza cambiare identità. Diventano
**`ABP_Guardian`** e **`ABP_Ranger`**.~~

> 🗄️ **Superata da [§5b](#5b-characterid-è-il-nome-del-pack-non-delleroe-dal-2026-08-11)** (2026-08-11): la
> regola è ora **l'opposto** — il nome segue il **pack**, quindi `ABP_Gadget`, non `ABP_Flux`. Il paragrafo <!-- rename-exempt: la riga contrappone la regola nuova alla vecchia: sostituirla la renderebbe muta -->
> resta come registro di CP 6.0. ⚠️ Era rimasto attivo per tre sezioni dopo che la riga gemella della tabella
> A.2 era già stata barrata: due regole opposte vive nello stesso documento **normativo**, trovate dalla
> code review della PR #483.

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
[`test-manuali-pie.md`](../test-manuali-pie.md).

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
      ...                      (26 pack, 46,4 GB, 34.017 file)
```

> **Residui della migrazione**: 88 asset di `Characters/Global/` (parameter collection, texture condivise,
> un `ShinbiPlayerCharacter`) sono rimasti in 11 cartelle `Content/Paragon<Nome>/`. UE si è rifiutato di
> rinominarli e **sono caricabili**, quindi spostarli a mano li romperebbe (B.3 punto 4): restano al vecchio
> path, dove i pack migrati li referenziano correttamente. Ignorati da git come directory.

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
   scheletro, poche animazioni e i relativi materiali. Procedura in **B.2a**, che è la via **scelta**
   (decisione utente, 2026-08-10) per i pack che servono a `U7`/`U8`.

## B.2a Procedura: magazzino + Migrate (via scelta dal 2026-08-10)

> Risolve il problema pratico da cui nasce: **Fab installa dove vuole lui**, tipicamente `Content/<Pack>/`, e
> quella non è la posizione voluta dal §1.

**Perché serve un magazzino invece di scegliere la cartella: non si può.** Verificato sulla documentazione
ufficiale il 2026-08-10 — di configurabile ci sono solo due cose, e nessuna è quella che servirebbe:

| | Si sceglie? |
|---|---|
| **Quale progetto** riceve il pack | ✅ il popup di *Add to Project* |
| **La cartella dentro `Content/`** | ❌ **no** — sempre `Content/<NomePack>/` |
| La **cache** dei file scaricati | ✅ Vault Location nel Launcher; il Fab Window ha una propria *cache directory* |

La cache è dove il Launcher tiene i file scaricati, **non** dove finiscono nel progetto: spostarla libera
spazio, non cambia la destinazione. La documentazione del Fab Window descrive il download e le impostazioni
di cache, e **nessun path di installazione**.

∴ L'unica leva che Fab lascia è **quale progetto**. Il magazzino non è un giro largo: è l'unico punto in cui
si può decidere, e da lì in poi comanda `Migrate`. Fonti:
[Fab Window](https://dev.epicgames.com/documentation/en-us/unreal-engine/fab-window-in-unreal-engine) ·
[thread sulla cartella della libreria](https://forums.unrealengine.com/t/how-to-change-fab-library-folder-in-unreal-engine/2116077),
rimasto senza risposta definitiva.

⚠️ **Il punto su cui si sbaglia: `Migrate` PRESERVA il path virtuale.** Un asset che nel magazzino sta in
`/Game/ParagonGideon/…` atterra in `Content/ParagonGideon/…` del progetto di destinazione, **non** in
`Content/FabAsset/Paragon/`. Migrate copia con le dipendenze, non rimappa. Per questo il rename si fa **nel
magazzino, prima** di migrare: là un errore costa un nuovo download, nel progetto vero costa il danno.

**Una volta sola** — creare il magazzino: progetto UE **Blank**, senza starter content, fuori dal repository
(es. `D:\UE_FabVault\`).

> Non è il vault di B.2b. Quello era **linkato con junction** e il progetto ne dipendeva in permanenza; questo
> è solo una **sorgente di copia**: dopo il Migrate gli asset stanno nel progetto e il magazzino si può
> cancellare. La ragione per cui il vault fu abbandonato — la dipendenza da un percorso esterno — qui non si
> presenta.

**Per ogni pack**:

1. Fab → *Add to Project* → il magazzino. Il pack atterra in `FabVault/Content/<Pack>/`.
2. Apri il magazzino e sposta `/Game/<Pack>/` → `/Game/FabAsset/Paragon/<Pack>/` **dal Content Browser**
   (che riscrive i riferimenti — mai da Esplora File, punto 2 qui sopra).
3. Seleziona **solo ciò che serve**: SkeletalMesh, Skeleton, PhysicsAsset, le animazioni che userai,
   materiali e texture. **Niente `SoundCue`, niente `DialogueWave`.**
4. Tasto destro → **Migrate…** → cartella `Content/` del progetto vero.

Il punto 3 non è solo economia di spazio: è **anche** ciò che dovrebbe evitare la perdita di B.3 punto 1,
perché sparivano i `DialogueWave` *referenziati dai `SoundCue`* e qui non si porta né gli uni né gli altri.

### Eseguita per la prima volta il 2026-08-11 su `ParagonGadget` — cosa si è imparato

La procedura è stata eseguita **headless**, sostituendo i punti 2-4 con `rename_directory` nel magazzino più
una copia del filesystem: una volta rinominato, `/Game/FabAsset/Paragon/<Pack>/` corrisponde a
`Content/FabAsset/Paragon/<Pack>/` in **entrambi** i progetti, quindi la copia equivale al Migrate quando si
porta il pack intero. Cinque cose che il documento non diceva:

1. ⚠️ **`Paragon_Proto_Retarget` blocca il rename.** È di classe `Rig`, **rimossa in UE 5.8**: l'asset esiste
   ma non è caricabile, e `rename_directory` fallisce in blocco con *«Some assets couldn't be renamed»*
   senza spostare niente. È l'insidia 2 di B.3, e il suo rimedio funziona: **quarantena fuori da `Content/`**
   prima del rename. Il fallimento è **innocuo** — il pack resta integro, si può riprovare.
2. ⚠️ **Un tentativo fallito lascia la destinazione creata e vuota**, e al secondo giro lo script si ferma
   perché «esiste già». Va rimossa prima di riprovare.
3. ⚠️ **`duplicate_directory` NON è l'alternativa che sembra.** Provata proprio per evitare la perdita di
   audio, sull'ipotesi che il difetto stesse nel cancellare l'originale: **falsa**. Perde *gli stessi* 12 file
   **e in più** lascia i riferimenti al path vecchio su **19 asset su 40** campionati, contro i 9 su 1229 del
   rename — cioè produce il «pack che sembra migrato e non lo è» dell'insidia 4. L'origine resta intatta,
   il che dimostra che il difetto è nella **riscrittura** dell'asset, non nella rimozione.
4. ⚠️ **La perdita di audio dipende dal PACK, non è una legge.** Su Gadget si è verificata — `_Engage_` da
   **25 a 13**, mentre i 174 `_Effort_` sono sopravvissuti tutti, esattamente come descrive B.3 punto 1. Su
   Gideon non era avvenuta. Non si può prevedere: si misura.
5. ⚠️ **Il rename lascia soft reference al path vecchio, e questo è lo stato NORMALE dei pack qui.**
   Su Gadget **9 asset su 1229**, su `ParagonGideon` — migrato nel 2026-08-06 e in uso — **6 su 1698**. Sono
   SkeletalMesh e skin che puntano ai propri materiali: è il difetto di A.6, *«il resave non aggiorna i soft
   reference»*. Se una mesh appare senza materiali, è questo, e si ripara con la procedura di A.6.

> 🔴 **Un campione casuale non prova l'assenza di un difetto raro.** Il 2026-08-10 avevo dichiarato Gideon
> «25 su 25 con i riferimenti corretti» campionando 25 file su 1698: con 6 asset difettosi quel campione
> aveva **meno del 10%** di probabilità di trovarne uno. La scansione completa li ha trovati. Per i
> riferimenti si scansiona **tutto il pack** — costa secondi — e il campione si usa solo per confermare un
> difetto, mai per dichiararlo assente.

**Guadagno collaterale**: si porta ciò che serve invece dell'intero pack, quindi l'asset registry non indicizza
peso inutile a ogni avvio — il costo descritto in B.1b.

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

## B.3b Quali pack sono davvero danneggiati — e cosa significa «danneggiato»

Più documenti ripetono che *«Gideon, Sparrow e altri 3 pack sono stati danneggiati dalla migrazione del
2026-08-06 e vanno riscaricati da Fab»*. ⚠️ **Gli «altri 3» non sono nominati da nessuna parte**: l'avviso
non è azionabile, e va completato quando i pack vengono identificati. Registrato il 2026-08-10.

Misura sul disco, stessa data:

| Pack | File | Dimensione | Riferimento |
|---|---:|---:|---|
| `ParagonGideon` | 1.700 | 2,6 GB | 2,74 GB dichiarati in B.2 punto 3 |
| `ParagonSparrow` | 1.508 | 2,82 GB | — |
| `ParagonDekker` | 1.604 | 1,56 GB | 1.310 asset dai riferimenti rotti (B.3 punto 4) |

**«Danneggiato» qui non vuol dire «assente»**: su Dekker il problema sono i **riferimenti rotti**, su altri
pack i `DialogueWave` persi (B.3 punto 1). Le due cause hanno rimedi diversi, e solo la seconda è
irrecuperabile senza riscaricare.

### ✅ `ParagonGideon` NON è danneggiato — verificato il 2026-08-10

L'avviso è stato **messo alla prova** confrontando il pack in `FabAsset/` con un download fresco da Fab,
fatto lo stesso giorno proprio perché quell'avviso lo prescriveva:

| | in `FabAsset/Paragon/` | download fresco |
|---|---:|---:|
| File | 1.700 | 1.700 |
| File audio-dialogo | 205 | 205 |
| Dimensione | 2,6 GB | 2,73 GB |

**Nessun file mancante e nessun `DialogueWave` perso** — su Gideon la perdita di B.3 punto 1 non è avvenuta
(su Gadget invece sì: vedi B.2a). L'utente ha confermato a schermo che gli asset si aprono.

⚠️ **Correzione del 2026-08-11**: la riga che stava qui diceva *«su 25 `.uasset` campionati, 25 citano
`/Game/FabAsset/Paragon/` e zero il path vecchio»*, e la presentava come prova che il pack fosse pulito. Non
lo era: la **scansione completa** ne trova **6 su 1698** che citano ancora il path vecchio — le SkeletalMesh
e le skin, coi soft reference ai materiali non aggiornati (A.6). Il campione di 25 su 1698 aveva meno del
10% di probabilità di incontrarne uno. Il pack resta **usabile e in uso**, ma «zero» era un artefatto del
metodo, non un fatto.

Restano 130 MB di differenza, che **non sono contenuto assente**: gli asset a destinazione sono stati
risalvati dal rename, quindi ricompressi. Un delta di dimensione, da solo, non è una prova di danno.

⚠️ **Conseguenza sull'avviso**: per Gideon era **falso**, e ha fatto scaricare 2,73 GB inutili. Non
propagarlo agli altri pack senza misurarli: `ParagonSparrow` e i tre mai nominati sono **non verificati**, non
«danneggiati». Il metodo per verificarli è quello qui sopra — conteggio file, conteggio audio, e il campione
sui path citati — e costa pochi minuti contro un download da gigabyte.

Conseguenza pratica generale: **la perdita documentata è audio, non geometria**. Un `BP_Unit` usa
SkeletalMesh, Skeleton, PhysicsAsset, animazioni e materiali. Verifica *quegli* asset prima di riscaricare;
se poi il download serve davvero, la via è **B.2a**.

## B.4 Regola operativa

Prima di aggiungere un pack al progetto, chiediti: **lo sto usando adesso?** Se la risposta è «servirà più
avanti», scaricalo quando servirà. Un pack inutilizzato non costa spazio su disco: costa **tempo di editor a
ogni avvio** (B.1b) e 48 GB che nessun backup del repo protegge.
