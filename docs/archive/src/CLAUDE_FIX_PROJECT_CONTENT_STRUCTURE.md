> 🔎 **ESITO DELLA REVISIONE — 2026-08-30.** Direttiva **assorbita dagli owner**, non eseguita come
> scritta. Misurata su `fff33020`, delle diciassette sezioni numerate (§0–§16): **due** descrivono uno
> stato **già vero**, **tre** sono **superate** — due dall'owner dei percorsi, una da `D-181` —, **una**
> collide con l'owner e perde, **una** ha prodotto l'unico rilievo nuovo, e le restanti **dieci** sono
> disciplina o procedura che non chiede un'azione oggi.
>
> ✅ **Già vero, misurato**: `Content/RT/` è il namespace proprietario e le sue sei cartelle sono
> feature-first (`Art`, `Characters`, `Core`, `Maps`, `UI`, `World`) — nessuna `Blueprints/`, `Meshes/` o
> `Textures/` globale, che è il divieto del §1. Le quattro cartelle personaggio del §4 esistono col nome
> canonico: `Gadget`, `Phase`, `Riktor`, `Wraith`, più `Shared`.
>
> ⛔ **§2 e §9 sono superate dall'owner, e nel verso opposto.** Chiedono di creare *ora* lo scheletro
> `SourceAssets/{Blender,FBX,Textures,Audio,UI,References}/`;
> [`convenzioni-contenuti-ue.md`](../../technical/tooling/convenzioni-contenuti-ue.md) §230–235 dichiara
> che `SourceAssets/` **non esiste ancora e va creata alla prima necessità reale, non in anticipo**. La
> cartella infatti non c'è, e la sua assenza **non è un residuo**: è la decisione dell'owner. I due script
> che il §2 allega — `create_content_directories.ps1` / `.py` — non sono mai entrati nel repository, e
> dopo `D-182` non potrebbero nemmeno: `scripts/` contiene solo `rt-suite.ps1`.
>
> ⛔ **§12 è morta con [D-181](../../decisions/RT_PDR_00_Decision_Log.md)**: il Feature Registry,
> le sue viste generate e i loro validator sono usciti dal repository il 2026-08-21. Il §7 elenca fra i
> link minimi del README `docs/roadmap/feature-registry.yaml`, che non esiste più; il §0 lo mette al
> punto 11 del preflight. Non vanno riaperti — il sorgente che li aveva costruiti è archiviato accanto a
> questo, con l'esito scritto in testa.
>
> ⚠️ **§3 collide con l'owner, e vince l'owner.** Chiede di spostare
> `Content/RT_UI_AssetPack_FromHUD/` fuori da `Content/`, verso `SourceAssets/UI/HUDPrototype/`.
> [`asset-map.md`](../../technical/tooling/asset-map.md) tratta invece quel kit come *«la famiglia più
> vicina a essere pronta»*, destinata all'import a `/Game/RT/UI/Icons/` con naming già deciso
> (`T_UI_Icon_<Categoria>_<Nome>`), e ne registra il difetto vero: di tutto il kit il repository traccia
> **due** file — `README.md` e `manifest.json` — e i PNG no, perché nessuna riga d'allowlist prevede quel
> percorso. *(Sul disco della working directory condivisa il kit conta **57** file; è una misura del disco
> e non dell'albero, e in un clone pulito quei PNG non ci sono.)*
> Il problema non è *dove sta il kit*, è che `git add` tace su di esso — e quello ha già un owner e una
> epic (**E20**).
>
> ✅ **§8 ha prodotto l'unico rilievo nuovo, ed è stato applicato.** Chiede di rimisurare l'asset map e di
> *«non conservare conteggi vecchi»*. Rimisurato: i file tracciati sotto `Content/` che non sono
> `.uasset`/`.umap` sono **140**, non i `145` che `asset-map.md` dichiarava, e il loro dettaglio era
> sbagliato in due punti — gli SVG di `Content/Icons/` sono **134** e non 137, e i file non-SVG di quella
> cartella sono **tre** (`LEGGIMI.md`, `manifest.json`, `rticonehud20260826.zip`) e non uno. 🔴 **Il numero
> era sbagliato quando è stato scritto, non è invecchiato**: sul commit che lo ha introdotto (`505e5234`,
> 2026-08-28) l'albero rispondeva già `140` e `134`. La correzione è nello stesso commit che archivia
> questo file.
>
> ⚠️ **Il resto è disciplina, non lavoro**: §5–§6 (mantenere `AGENTS.md` e `CLAUDE.md` allineati agli
> owner), §10 (non inventare una policy sui Data Asset), §11 (un workbook, una sola categoria — e il
> workbook di balance è `RESEARCH` da [D-023](../../decisions/RT_PDR_00_Decision_Log.md)), §13 (un lavoro
> solo per volta sui binari) e §14–§15. Nessuna richiede un'azione oggi.
>
> ℹ️ **Le note in linea del corpo restano**: erano state aggiunte al documento mentre era in radice, e
> registrano da sole la caduta di `D-181`/`D-182` e la correzione «sette comandi / ne resta uno» → «cinque
> / ne restano due». Non sono state riscritte: sono la storia di questa direttiva.

---

# CLAUDE — RefactorTactics: allineamento struttura Content e consolidamento progetto

## Missione

Allinea il repository `DegrassiAaron/refactor-tactics-main` alle decisioni correnti sulla struttura dei contenuti Unreal e sulla governance documentale. Non creare una nuova architettura: verifica `main`, individua le derive reali, correggile con il diff minimo e mantieni una sola source of truth per ogni dato.

Baseline da verificare su HEAD:
- Unreal Engine 5.8.1.
- Asset proprietari Unreal sotto `/Game/RT/` (`Content/RT/`).
- Struttura `Content` feature-first, non type-first.
- Gameplay competitivo/autorevole in C++; Blueprint/Data/UMG/VFX per presentazione/configurazione dove previsto.
- Scenari testuali in `Scenarios/`, non in `Content/`.
- Sorgenti non importati in Unreal in `SourceAssets/`.
- `.uasset` e `.umap` si modificano/spostano solo tramite Unreal/Content Browser/API Editor.
- ~~Feature Registry e viste derivate non vanno duplicate.~~ ⛔ Decaduto con **D-181** (2026-08-21): il registry e le sue viste sono usciti dal repository.
- Git LFS va descritto secondo lo stato reale del repository corrente, non per memoria.

## 0. Preflight obbligatorio

Prima di modificare:

```bash
git fetch --prune origin
git status
git rev-parse HEAD
git log -10 --oneline
```

Leggi nell'ordine:
1. `AGENTS.md`
2. `CLAUDE.md`
3. `docs/product/piano-canonico-mvp.md`
4. `docs/decisions/RT_PDR_00_Decision_Log.md`
5. ADR pertinenti
6. `docs/DOC_CONFLICT_MATRIX.md`
7. `docs/OPEN_DECISIONS.md`
8. `docs/technical/tooling/convenzioni-contenuti-ue.md`
9. `docs/technical/tooling/asset-map.md`
10. `docs/technical/architecture/spec-asset-pipeline.md` se esiste
11. `docs/roadmap/feature-registry.yaml`
12. `docs/roadmap/execution-graph.yaml`
13. `README.md`
14. `.gitignore`
16. `.gitattributes`

Verifica in particolare le decisioni più recenti che hanno superato regole precedenti, incluse D-130, D-134, D-135, D-136 e D-139. Se un ID è stato rinumerato, segui la tesi/contenuto della decisione, non il numero isolato.

## 1. Regola Content

L'owner normativo dei percorsi resta `docs/technical/tooling/convenzioni-contenuti-ue.md`.

Tutti gli asset proprietari Unreal runtime/editor di RefactorTactics stanno sotto:

```text
/Game/RT/
```

fisicamente:

```text
Content/RT/
```

La struttura è feature-first. Non introdurre cartelle globali tipo:

```text
Content/RT/Blueprints
Content/RT/Meshes
Content/RT/Textures
Content/RT/Materials
```

Un asset deve vivere vicino alla feature che lo possiede.

## 2. Crea ORA le directory target vuote

L'autore richiede esplicitamente di creare anche le directory vuote target. Questa è un'eccezione operativa alla precedente preferenza di non crearle in anticipo.

Usa lo script allegato `create_content_directories.ps1` oppure `create_content_directories.py`.

IMPORTANTE:
- le directory vuote devono esistere sul filesystem;
- Git non conserva directory vuote;
- NON aggiungere `.gitkeep` in massa;
- se serve versionare lo scheletro, proponi una strategia separata invece di introdurre rumore.

## 3. Bonifica UI prototype

Audit obbligatorio:

```text
Content/RT_UI_AssetPack_FromHUD/
```

Questo materiale è un prototype/reference kit, non deve restare come namespace proprietario parallelo accanto a `Content/RT`.

Se contiene PNG, crop, reference image, manifest o sorgenti di authoring, spostalo fuori da `Content/` verso:

```text
SourceAssets/UI/HUDPrototype/
```

oppure, solo se è puramente documentale/reference:

```text
docs/src/media/ui/
```

Scegli sulla base del contenuto reale.

Correggi eventuali istruzioni obsolete del tipo:

```text
Content/RefactorTactics/UI/Prototype/
```

Gli asset realmente importati in Unreal devono andare invece sotto:

```text
/Game/RT/UI/...
```

Per le icone runtime usa il path/naming già deciso dall'owner, ad esempio:

```text
/Game/RT/UI/Icons/
T_UI_Icon_<Categoria>_<Nome>
```

Non importare automaticamente tutti i PNG prototype come asset finali.

## 4. Personaggi e naming

Le directory dei personaggi/content owner devono essere coerenti con le decisioni correnti:

```text
Content/RT/Characters/Gadget/
Content/RT/Characters/Phase/
Content/RT/Characters/Riktor/
Content/RT/Characters/Wraith/
```

Prima di modificare Stable ID o ActionId, rileggi D-130 e soprattutto le decisioni successive che la emendano (inclusa D-134). Non lasciare una doppia verità tra Decision Log, `AGENTS.md` e `CLAUDE.md`.

Non fare search/replace cieco e non modificare binari Unreal fuori dall'Editor.

## 5. Correggi AGENTS.md

`AGENTS.md` è il contratto operativo condiviso. Deve restare sintetico ma coerente con gli owner.

Audit almeno:
- roster/naming/Stable ID;
- Content feature-first;
- Feature Registry;
- viste generate;
- binari Unreal, un lavoro per volta;
- Git LFS;
- scenari;
- numeri canonici;
- policy `.uasset`/`.umap`.

Se una riga è superata da una decisione recente, correggila e linka l'owner. Non copiare l'intero Decision Log.

## 6. Correggi CLAUDE.md

`CLAUDE.md` deve restare un overlay corto su `AGENTS.md`.

Deve:
- imporre lettura di `AGENTS.md` + owner;
- contenere solo pin operativi ad alto rischio;
- riflettere la decisione più recente su naming/Stable ID/redirect;
- ricordare che i binari si toccano da un lavoro solo per volta;
- ricordare che le viste generate non si editano.

Rimuovi policy superate e numeri volatili hardcoded.

## 7. Rifai README.md come entry point stabile

Il README non deve usare vecchi snapshot come stato corrente.

Rimuovi/riscrivi, se non più dimostrati da HEAD:
- `Stato attuale (2026-08-05)`;
- conteggi fissi tipo `419 test`;
- milestone vecchie descritte come correnti;
- riferimenti al vecchio gameplay quadrato se superati.

Il README deve descrivere:
- cosa è RefactorTactics;
- come compilare/eseguire;
- dove sono gli owner;
- come trovare lo stato corrente.

Link minimi:
- `docs/README.md`
- `docs/product/piano-canonico-mvp.md`
- `docs/roadmap/roadmap-checkpoint.md`
- `docs/roadmap/feature-registry.yaml`
- `docs/technical/tooling/convenzioni-contenuti-ue.md`
- `docs/technical/tooling/asset-map.md`
- `docs/technical/test-manuali-pie.md`
- `docs/OPEN_DECISIONS.md`

Riallinea la sezione Git LFS a `.gitattributes`/`.gitignore` correnti.

## 8. Asset Map

`docs/technical/tooling/asset-map.md` è il tracker degli asset, non l'owner dei percorsi.

Rimisura:
- allowlist `.gitignore`;
- `git ls-files Content`;
- asset committati;
- asset solo su disco;
- asset assenti;
- file tracciati fuori allowlist;
- famiglie senza path;
- icone HUD.

Non conservare conteggi vecchi.

## 9. SourceAssets

Crea:

```text
SourceAssets/
├── Blender/
├── FBX/
├── Textures/
├── Audio/
├── UI/
│   └── HUDPrototype/
└── References/
```

Regola:
- `.blend`, `.psd`, `.kra`, `.svg`, `.fbx`, audio master, reference art -> `SourceAssets`;
- `.uasset`, `.umap` -> `Content`;
- JSON scenario -> `Scenarios`;
- C++ -> `Source`;
- Markdown normativo -> `docs`.

## 10. Data Asset: non inventare una policy

Prima di creare/spostare `DA_Hero_*`, `PDA_*` o simili:
- verifica `URTHeroData`;
- Asset Manager;
- asset map;
- roadmap;
- codice;
- Decision Log.

Se documentazione e implementazione divergono, registra il conflitto nell'owner corretto. Non creare binari solo per far sembrare coerente la documentazione.

## 11. Excel/workbook

Audit di tutti i `.xlsx` versionati. Classifica ogni workbook come UNA sola categoria:

```text
OWNER
AUTHORING_SOURCE
DERIVED_GENERATED
RESEARCH_HISTORICAL
DEPRECATED
```

Per il workbook balance v0.1 verifica la decisione vigente: se è `RESEARCH`, non usarlo per risolvere conflitti contro i cataloghi Markdown e non rattopparlo cella per cella.

Se un workbook è generato, modifica la source e rigenera.

## 12. Registry, roadmap, scenario/milestone/editor map e Wiki

Non creare registri paralleli.

Lo stato feature vive in:

```text
docs/roadmap/feature-registry.yaml
```

Le viste devono essere derivate/generate.

Audit:
- feature map;
- scenario map;
- milestone map;
- editor map;
- shortlist;
- project graph;
- Wiki status;
- workbook che citano feature.

Niente `status` duplicato manualmente se esiste già una source of truth.

Non introdurre nuove tassonomie di lane/track se `execution-graph.yaml` possiede già il concetto.

## 13. Binari Unreal — D-178

Se tocchi `.uasset` o `.umap`:
- un lavoro solo per volta su quel path;
- niente binary merge: due `.uasset` non si fondono, uno dei due si rifà a mano;
- niente rename da filesystem;
- per mappe considera la package family/cartella quando necessario.

*(Fino al 2026-08-20 la regola passava per una Binary Asset Lease dichiarata in
`docs/roadmap/parallel-batch.yaml`; `D-178` ha rimosso il meccanismo, non il vincolo fisico.)*

Se non hai lease, STOP sulla parte binaria. Puoi completare docs/script e lasciare una lista precisa dei passaggi Editor.

## 14. Test di conformità

Prima cerca validator già esistenti. Estendili solo se l'ownership/write-set lo consente.

Controlli desiderati:
- nessun asset proprietario Unreal fuori da `Content/RT` salvo eccezioni normative;
- niente `Content/RT/Blueprints`, `Meshes`, `Textures`;
- nessun source/reference kit parallelo sotto `Content`;
- nessuna dipendenza runtime verso Developers/Dev/Tests dove verificabile;
- `AGENTS.md`/`CLAUDE.md` non conservano policy superate.

## 15. Validazione finale

Esegui i gate realmente presenti, con le firme correnti, ad esempio:

```bash
node tools/radar/generate.ts --check
node tools/radar/wiki-alt.ts --wiki-root <clone> --check
node --test "tools/radar/**/*.test.ts"
```

> ⛔ **Questa lista era di cinque comandi e ne restano due**, più la suite. Il 2026-08-21 sono usciti il
> Feature Registry (**D-181**) e l'intera cartella `scripts/` (**D-182**): i cinque gate documentali, i
> due controlli sui dati di gioco e i due generatori.
>
> ⚠️ **La prima stesura di questa nota diceva «sette comandi» e «ne resta uno», e sbagliava entrambi i
> numeri**: la lista cancellata ne aveva cinque, e i `--check` vivi sono due — `generate.ts` guarda gli
> SVG, `wiki-alt.ts` gli alt sulla Wiki, e il primo **non** copre il secondo. Un numero in prosa dentro
> il paragrafo che esiste per dire quanta copertura si è persa: contarlo costa una riga.

Poi:
- verifica viste generate;
- verifica `git status`;
- se hai toccato binari: Fix Up Redirectors, Reference Viewer, salvataggio, PIE mirato e gate packaged solo se richiesto.

Non dichiarare build/PIE/package eseguiti se non lo sono.

## 16. Deliverable richiesto

Consegna una tabella:

| Area | Prima | Dopo | Evidenza |
|---|---|---|---|
| Content namespace | | | |
| UI prototype | | | |
| Character dirs | | | |
| SourceAssets | | | |
| AGENTS.md | | | |
| CLAUDE.md | | | |
| README.md | | | |
| Asset Map | | | |
| Icon allowlist | | | |
| Registry/views | | | |
| Excel governance | | | |
| Empty directory skeleton | | | |

Poi:
- file modificati;
- directory create;
- asset spostati via filesystem;
- asset spostati via Unreal;
- decisioni applicate;
- conflitti trovati;
- test/gate eseguiti;
- verifiche manuali ancora necessarie;
- issue da aggiornare/aprire dopo aver cercato duplicati;
- commit suggeriti.

Commit suggeriti, se coerenti col diff reale:

```text
docs(governance): align agent instructions with current decisions
refactor(content): move HUD prototype sources out of runtime content
chore(content): establish target content directory skeleton
docs(readme): remove stale project status snapshot
docs(assets): refresh asset map from repository state
```

Non commit/push/merge senza richiesta esplicita.

## Definition of Done

DONE solo quando:
- `Content/RT` è il namespace proprietario Unreal;
- il prototype HUD non crea un namespace proprietario parallelo;
- le directory target richieste esistono sul filesystem;
- `AGENTS.md` e `CLAUDE.md` non contraddicono il Decision Log corrente;
- `README.md` non usa snapshot vecchi come stato corrente;
- Git LFS è descritto correttamente;
- Asset Map è rimisurata;
- pipeline icone HUD è coerente;
- Excel non è fonte concorrente;
- roadmap/mappe/wiki restano derivate dalle source of truth;
- i gate applicabili passano;
- ogni verifica Editor non eseguita è dichiarata.

Obiettivo finale: eliminare le doppie verità, non nasconderle.
