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
- Feature Registry e viste derivate non vanno duplicate.
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
13. `docs/roadmap/parallel-batch.yaml` se il lavoro è parallelo
14. `README.md`
15. `.gitignore`
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
- Binary Asset Lease;
- write-set/worktree;
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
- ricordare Binary Asset Lease e write-set;
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

## 13. Binary Asset Lease / D-139

Se tocchi `.uasset` o `.umap`:
- verifica `parallel-batch.yaml`;
- verifica write-set;
- usa Binary Asset Lease esclusiva;
- un holder per path;
- niente binary merge;
- niente rename da filesystem;
- per mappe considera la package family/cartella quando necessario.

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
python scripts/check-docs-links.py
python scripts/check-docs-symbols.py
python scripts/check-docs-naming.py --check
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate --check
```

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
