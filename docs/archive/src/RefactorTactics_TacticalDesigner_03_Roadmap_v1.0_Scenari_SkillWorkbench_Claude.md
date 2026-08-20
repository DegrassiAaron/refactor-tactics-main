> 🗄️ **ARCHIVIATO il 2026-08-17 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. Il contenuto recepito vive ora nelle fonti canoniche.
>
> **Cosa è entrato** — [D-154](../../decisions/RT_PDR_00_Decision_Log.md): il *Tactical Designer* è un nome
> di workflow con un owner documentale — [`spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md)
> — non un modulo, non una epic per pannello e non una seconda numerazione di release. Con esso: il
> checkpoint **M9.4** in [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md), le due feature
> `RT-FEAT-TOOL-SCENARIO-COMPOSER` e `RT-FEAT-TOOL-SKILL-WORKBENCH` in
> [`feature-registry.yaml`](../../roadmap/feature-registry.yaml), un'**epic di processo senza numero `E`**
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) (la numerazione `E` è quella delle release, e questo strumento è `out_of_release_scope`: stessa forma di
> [#839](https://github.com/DegrassiAaron/refactor-tactics-main/issues/839) e
> [#422](https://github.com/DegrassiAaron/refactor-tactics-main/issues/422)), la seduta **U26** in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Referto del filtro:
> [`tactical-designer-consolidamento-2026-08-17.md`](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md).
>
> 🔴 **Dieci sezioni su 76 erano superate all'arrivo, e chiedevano di rifare lavoro già in `main`.** Il
> documento si dichiara fotografia del 2026-08-13 e apre con una «regola numero zero» che vieta di fidarsene:
> applicata, dice che `#554`, `#588`, `#619`, `#620` e `#621` sono **tutte chiuse**. Le §5–§10, §25, §26 e §34
> progettano `#620` e `#621` — che esistono come `RTGeometryGrammar` e `RTGeometryBake` con **13 + 8** test
> misurati — e il tool d'editor che il §6 propone di progettare esiste già (`RTHexGeometryTool`, 380 righe,
> `99cd06bd`). La §11 chiede una policy di invertibilità del bake già decisa da
> [D-131](../../decisions/RT_PDR_00_Decision_Log.md).
>
> 🔴 **Il §68 sbaglia tre nomi del roster su quattro**, e il §38 li ripete dentro un mockup di UI: dichiara
> «Gadget, Phaser, Victor, Wrath» contro [D-120](../../decisions/RT_PDR_00_Decision_Log.md) e
> [D-130](../../decisions/RT_PDR_00_Decision_Log.md), che fissano **Gadget · Phase · Riktor · Wraith**.
> Il documento chiede esso stesso di verificare quell'elenco contro `main`: nessuno di quei nomi è entrato
> in un documento normativo.
>
> ⚠️ **Il §36 vieta un secondo formato di scenario e il §37 ne progetta uno.** Il repository ha già
> `FRTTestScenario`, e copre unità, turni, intent, **decisioni di finestra scriptate**
> (`FRTScenarioDecision`), aspettative su otto `ERTAssertionKind`, capability richieste per turno
> (`Requires` → `ERTTestOutcome::Blocked`) e **varianti** (`FRTScenarioVariant`) — cioè anche il §40.3, che
> il documento presenta come da costruire. Ciò che manca davvero è l'**authoring**, ed è il gap che le due
> feature nuove possiedono.
>
> ✅ **Ciò che era davvero nuovo, e vale**: il §3 (*«un issue body non deve contenere una dipendenza
> fattualmente falsa quando quella falsità cambia il modo in cui il prossimo sviluppatore interpreta il
> lavoro»*), il §32 (runtime rule → pure query → visualizzazione, mai una regola parallela) e il §41 (le
> quattro nature di un'aspettativa: invariante forte, aspettativa di design, soglia di bilanciamento,
> osservazione di telemetria). I tre sono recepiti in `spec-tactical-designer.md` §3, §7 e §8.
>
> ⏸️ **Cosa NON è entrato**: la §12 (separare bake logico e render rebuild) perché manca il numero che la
> giustifichi, e il documento stesso dice «misura prima»; la §45 (rename verso `RTTacticalDesigner*`) perché
> è un nome di prodotto; la §31A.5 (`Power Budget`) perché non ha un consumatore e
> [D-102](../../decisions/RT_PDR_00_Decision_Log.md) chiede il competence gate del bot prima di qualunque
> misura di bilanciamento; la §74 (quattro branch paralleli) perché
> `parallel-batch.yaml` governa già il write-set e non si aprono track
> per lavoro non cominciato.

---

# RefactorTactics — Tactical Designer
## Prompt operativo per Claude Code: consolidare Level/Map Designer + Skill Workbench + Scenario Composer e portare la capability fino alla v1.0

**Documento autosufficiente.** Se esistono i precedenti handoff `RefactorTactics_LevelDesigner_01_Context_Claude.md` e `RefactorTactics_LevelDesigner_02_Implementazione_Consolidamento_Claude*.md`, usali come contesto storico, NON come source of truth.

Questo file NON è un invito a implementare tutto in una PR.
È un prompt operativo per:

1. verificare lo stato reale del repository;
2. consolidare Level/Map Designer, Skill Workbench e Scenario Composer dentro un unico **Tactical Designer**;
3. creare una roadmap di maturità della capability fino alla **v1.0**, riconciliandola con le milestone reali del progetto;
4. eliminare drift e duplicazioni;
5. modificare/consolidare le issue esistenti prima di crearne di nuove;
6. creare issue SOLO dove esiste davvero un buco;
7. consolidare documentazione tecnica e Wiki senza creare owner paralleli;
8. mantenere sincronizzati codice, roadmap, Feature Registry, Scenario Map, Wiki, editor sessions e test.

---

# 0. Regola numero zero

NON fidarti dello stato descritto nel file di contesto.

È una fotografia storica aggiornata/consolidata fino al 2026-08-13, ma può essere già superata da `main`.

Prima di fare qualunque modifica:

```bash
git status
git branch --show-current
git log -1 --oneline
git fetch --all --prune
```

Leggi lo stato di `origin/main`.

Poi verifica issue/PR aperte e chiuse.

Il file di contesto serve per conoscere decisioni e intenti, non per sostituire HEAD.

---

# 1. Lavora in worktree / branch dedicato

Non lavorare direttamente sul working tree usato da altre sessioni.

Prima:

1. controlla worktree esistenti;
2. controlla branch correnti;
3. scegli una issue singola;
4. crea worktree/branch focalizzato;
5. evita file contesi non necessari.

Naming suggerito, da adattare alle convenzioni reali:

```text
feat/620-geometry-grammar
feat/621-geometry-bake
feat/554-editor-reachability
feat/622-workspace-grid
```

Non mettere #620, #621, #554, #622 e #623 tutti nella stessa PR.

---

# 2. Audit obbligatorio

Prima di scrivere codice, produci localmente un referto corto:

```text
LEVEL DESIGNER AUDIT
HEAD:
Branch:
Open relevant issues:
Closed relevant issues:
Relevant active PR:
Current test count:
Current feature gates:
Current editor sessions:
Current PIE residues:
Current scenario coverage:
```

Controlla almeno:

- #554
- #588
- #619
- #620
- #621
- #622
- #623
- #324

Verifica se, nel frattempo:

- #620 è stata chiusa;
- #621 è iniziata;
- nuove PR hanno cambiato il grafo;
- nuovi tool sono stati aggiunti;
- la roadmap è stata rigenerata;
- il Feature Registry è avanzato.

Se un punto di questo prompt è già implementato:
**non reimplementarlo.**

Aggiorna invece l’handoff/documentazione se serve.

---

# 3. Correggere il drift già noto

Durante l’audit del 2026-08-12 risultava:

- #588 CLOSED;
- #619 CLOSED;
- #620 OPEN.

Alcuni testi più vecchi dentro #620/#622 citavano ancora #588 come aperta / PR #598 non mergiata.

Se il drift esiste ancora:

- correggilo;
- non cambiare la sostanza della issue;
- non aprire una issue “fix docs” separata per una riga stale;
- cita lo stato attuale corretto.

Regola:

> Un issue body non deve contenere una dipendenza fattualmente falsa quando quella falsità cambia il modo in cui il prossimo sviluppatore interpreta il lavoro.

---

# 4. Consolidare il “Level Designer” come concetto, non come sistema parallelo

Il repository possiede già:

- `RefactorTacticsEditor`;
- Hex Map Editor Mode;
- Select;
- Paint;
- Arch;
- Fill;
- multilayer Focus;
- surface/cost visualization;
- blocker visualization;
- cover/door visualization;
- map graph;
- A*;
- LOS;
- cover;
- doors;
- bridges;
- Scenario Harness;
- TurnLog;
- validator;
- occupancy.

Quindi NON creare:

```text
URTLevelDesignerSubsystem
URTMapDesignerGraph
URTLevelDesignerPathfinder
URTLevelDesignerLOS
URTMapDesignerScenarioRunner
```

se sono solo wrapper duplicati.

Se serve UX comune, estendi il mode/toolkit esistente e crea facade/editor-only solo dove aggiunge valore reale.

---

# 5. Priorità operativa: #620

Se #620 è ancora aperta, è il prossimo lavoro corretto.

## 5.1 Obiettivo

Implementare una **grammar discreta e deterministica** per la geometria tattica di authoring.

Non implementare ancora il bake di #621 nello stesso branch salvo prerequisito minimo inevitabile e documentato.

## 5.2 Prima di codificare

Cerca:

```text
HexCorners
AxialDirection
DirectionTowards
edge center
edge orientation
cover edge geometry
MakeCoverYardArena
geometry occupancy
sector mask
```

Leggi:

- `RTHexLibrary.*`
- `RTHexCellData.*`
- implementazione #619
- codice di #553
- tool Arch
- editor click helper

Domanda da risolvere prima:

> esiste già su HEAD una funzione canonica per centro/orientamento di un bordo?

Se sì, riusala.

Se no, aggiungila UNA volta nel posto corretto e migra eventuali duplicazioni.

---

# 6. Design richiesto per #620

La geometria deve essere serializzabile senza float arbitrari competitivi.

Usa concetti discreti.

Esempio concettuale, NON nome API obbligatorio:

```cpp
enum class ERTTacticalAxis : uint8
{
    Axis0,
    Axis30,
    Axis60,
    Axis90,
    Axis120,
    Axis150
};
```

oppure una rappresentazione più coerente col codice attuale.

Prima di introdurre un nuovo enum:
controlla se esiste già un tipo equivalente.

La grammatica deve poter rappresentare:

- axis segment;
- orthogonal axis segment;
- edge/perimeter segment;
- junction;
- layer.

Nessun endpoint world-space arbitrario deve diventare dato competitivo.

Il world-space può essere una rappresentazione/preview derivata.

---

# 7. Validator #620

Il validator deve essere testabile headless.

Casi minimi rossi e verdi:

- off-axis geometry;
- invalid junction;
- zero-length segment;
- duplicate segment;
- invalid layer;
- outside editable bounds.

Test di mutazione:

- allenta una regola;
- deve cadere esattamente il test che la protegge.

Non lasciare la regola solo in:

- UI validation;
- tooltip;
- commento;
- property panel.

L’invariante deve vivere in codice verificabile.

---

# 8. Test #620

Rispetta la regola del repository:

> ciò che nasce dentro `RefactorTacticsEditor` e non è testabile è fragile.

Quindi sposta la logica pura nel modulo runtime/core appropriato.

L’editor chiama la logica.

Non mettere calcolo deterministico essenziale solo nel tool.

Suite minima:

```text
GeometryGrammar.AcceptsMainAxis
GeometryGrammar.AcceptsOrthogonalAxis
GeometryGrammar.AcceptsEdgeSegment
GeometryGrammar.RejectsOffAxis
GeometryGrammar.RejectsZeroLength
GeometryGrammar.RejectsDuplicate
GeometryGrammar.RejectsInvalidLayer
GeometryGrammar.RejectsOutsideBounds
GeometryGrammar.ValidJunction
GeometryGrammar.RejectsInvalidJunction
GeometryGrammar.PermutationInvariant
```

Nomi da adattare alle convenzioni reali.

---

# 9. Dopo #620: #621

Non partire da #621 se #620 non è pronta.

Scopo:

```text
Authoring geometry
      ↓
pure deterministic bake
      ↓
canonical map data
```

## Mapping canonico

Prima verifica il codice corrente.

Baseline:

```text
LOW WALL → FRTHexCover Low
WALL     → FRTHexCover High
solid footprint → bBlocksMovement
void/cliff → ERTHexSurface::Void
```

Non creare una seconda rappresentazione.

---

# 10. Bake #621: invarianti

Il bake deve essere:

- deterministico;
- stabile rispetto all’ordine input;
- testato;
- nel modulo runtime appropriato;
- indipendente dal viewport;
- indipendente da asset visuali;
- invocato dall’editor;
- compatibile col formato mappa;
- compatibile con cover/door già esistenti.

Il runtime NON deve interrogare la geometria di authoring.

Dopo il bake:

```text
geometry source → editor concern
canonical cells/edges → gameplay concern
```

---

# 11. Attenzione all’invertibilità

Il panel ha lasciato una domanda importante:

> cosa succede se un designer modifica manualmente un campo canonico derivato da una geometria e poi ribakea?

Prima di fare un sistema sofisticato, identifica la policy attuale.

Possibili policy, da NON decidere senza owner:

- source geometry owns baked fields;
- manual override wins;
- rebake overwrite;
- generated flag / provenance;
- dedicated authoring layer.

Se la decisione non esiste:

- NON inventarla silenziosamente;
- apri una decisione/issue solo se è bloccante;
- documenta il rischio.

---

# 12. Non ottimizzare prematuramente il rebake

Il prompt precedente proponeva affected-region rebake.

La issue #621 chiede che la regione investita sia misurabile.

Ma il repository usa `RebuildInstances` completo come difesa semplice contro divergenza visuale.

Quindi:

- separa il **bake logico della regione** dal **render rebuild**;
- non eliminare un full rebuild affidabile per principio;
- misura prima;
- ottimizza solo se il numero lo giustifica.

Performance change senza benchmark = non accettabile.

---

# 13. #554 in parallelo / subito dopo

Se #554 è aperta:

Implementare prima la misura headless, poi la vista.

## Headless

Estendi ciò che già conosce spawn/path.

Serve un output tipo:

```text
ReachableCells
UnreachableCells
UnreachableCount
Connected regions, se già utile
```

Vincoli:

- stabile;
- stesso risultato con TMap/TSet permutati;
- layer senza transition = correttamente isolato.

Non implementare un secondo BFS se il path service fornisce già la query corretta.

Se per performance serve una flood traversal:
riusa le stesse regole di traversabilità del pathfinding, non duplicarle.

---

# 14. #554 visual

Mostrare transizioni anche quando il tool corrente non è Arch.

Il designer deve poter leggere:

- from;
- to;
- verso;
- active/disabled se applicabile;
- unreachable island.

Non salvare geometria nel livello.

Visualizzazione:

- transient;
- derivata dal dato;
- aggiornata da map change;
- non collidibile / non deve rubare click.

---

# 15. #622 workspace grid

Issue piccola.

Non trasformarla in un nuovo grid system.

Serve SOLO:

> visualizzare le posizioni di lavoro dove non esistono ancora celle.

Vincoli:

- chiaramente ghost;
- non confondibile con una cella;
- estensione controllata;
- transient;
- instanced / draw primitive efficiente;
- nessun Actor-per-cell;
- non usare `DemoRadius` come falsa mappa.

Aggiornare la relativa verifica manuale.

---

# 16. #623 seduta

Questa non è una “feature camera”.

NON riscrivere il viewport.

## Codice

Aggiungere solo ciò che manca davvero:

- frame whole editable map / Home equivalente.

Deve funzionare multilayer.

## Editor session

Aprire Unreal e sistemare `L_DevSandbox`.

Obiettivo lighting:

- geometria distinguibile;
- no zone quasi nere;
- grid leggibile;
- exposure prevedibile.

Commit dell’umap solo seguendo le regole di CLAUDE.md / allowlist.

Oracolo:

```bash
git ls-files <path>
```

non “il file esiste sul disco”.

---

# 17. Dopo le issue base: NON implementare subito tutto il futuro

Una volta chiusi:

- #620
- #621
- #554
- #622
- #623

fare un nuovo audit.

Solo allora confrontare il backlog con le idee future:

- Movement Probe;
- Path Inspector;
- LOS Probe;
- Cover Probe;
- Facing Probe;
- Ability Geometry Probe;
- Overwatch Probe;
- Noise Probe;
- Environment Sandbox;
- Spawn/Objective Analyzer;
- TurnLog Heatmaps;
- Complexity Budget;
- Procedural batch evaluation.

Per ciascuna:

1. cerca issue aperte;
2. cerca issue chiuse;
3. cerca feature registry;
4. cerca roadmap;
5. cerca codice;
6. cerca scenari;
7. cerca editor session.

Solo se manca davvero:
proponi issue.

---

# 18. Come creare le nuove issue future

Non usare titoli generici come:

`Improve Level Designer`

Preferire una capacità osservabile:

- `Editor: movement probe shows reachable cells, cost and block reason`
- `Editor: LOS probe reuses runtime LOS and displays canonical blockers`
- `Editor: Overwatch probe renders triggerable entry lanes`
- `Editor: noise probe renders graph propagation and attenuation`
- `Tooling: TurnLog generates movement and combat map heatmaps`

Ogni issue deve contenere:

- Why;
- current state;
- exact owner systems to reuse;
- scope;
- out of scope;
- measurable DoD;
- automation;
- manual PIE if necessary;
- files/symbols affected;
- no-duplicate constraints;
- dependency.

---

# 19. Scenari

Prima di creare scenario:

Leggi:

- `scenario-map.md`;
- registry;
- directory `Scenarios/`.

Classifica:

## Fixture

Test puro di geometry/occupancy/bake.

NON Scenario Harness.

## Scenario

Una partita che dimostra una proprietà gameplay.

Esempio futuro:

```text
Spec.Map.BakedLowCoverAffectsDirectShot
```

solo se serve dimostrare il percorso reale.

## PIE/manuale

Aspetto visivo/editor:

- workspace grid leggibile;
- ghost wall;
- layer context;
- lighting;
- frame map;
- overlay readability.

Non fingere automation per una cosa che richiede un occhio umano.

---

# 20. Feature Registry

Feature probabili coinvolte:

- `RT-FEAT-TOOL-MAP-EDITOR`
- `RT-FEAT-TOOL-MAP-GEOMETRY`

Verifica i nomi reali.

NON modificare status derivati a mano se lo script li calcola.

Aggiorna:

- owner;
- issue mapping;
- gate;
- evidence;
- scenario;
- automation;
- PIE;
- docs;

secondo il modello esistente.

Poi:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py shortlist
```

e gli eventuali `--check` previsti.

---

# 21. EditorMap

`docs/roadmap/editormap.shortlist.md` è GENERATA.

Non editarla a mano.

Owner delle sedute:

`docs/roadmap/editor-sessions.yaml`

Quando introduci una verifica editor reale:

- registrala;
- assegna la sessione;
- eseguila;
- registra esito reale;
- rigenera la shortlist.

Regola del progetto:

> una voce PIE che non appartiene a una seduta tende a non essere mai eseguita.

---

# 22. Wiki / documentazione

Consolidare, non duplicare.

Cerca la pagina owner del Map Editor / Level Design.

Se non esiste una pagina Level Designer, valuta una pagina di overview che punti a:

- Map Editor;
- Map data model;
- architecture geometry;
- surfaces;
- cover;
- transitions;
- validation;
- scenarios.

NON copiare dentro Wiki intere specifiche tecniche che hanno già un owner.

La Wiki deve spiegare:

> come usare il sistema

non diventare un secondo PDR.

---

# 23. README / AGENTS / CLAUDE / CONTEXT_INDEX

Non aggiornare questi file “perché abbiamo toccato una feature”.

Aggiornali solo se cambia:

- orientamento di un nuovo agente;
- toolchain;
- path owner;
- regola strutturale;
- entry point importante.

Se la feature è già scoperta dal Context Index:
non aggiungere rumore.

Se il Level Designer workflow diventa un asse stabile e non è rintracciabile:
aggiungere un singolo riferimento utile.

---

# 24. Errori che devi prevenire

## Duplicazione

Il repository ha già subito più prompt map-editor che riproponevano feature esistenti.

Quindi ogni volta che stai per creare una classe/feature, chiedi:

> “Esiste già un owner o una issue per questa responsabilità?”

## Dato senza consumer

È già successo più volte.

Se aggiungi un campo:

> dimostra quale codice lo legge.

## UI che mente

Workspace/grid/overlay devono derivare dal dato vero.

Mai usare un demo radius o una preview che sembra una cella reale quando non lo è.

## Test che non può diventare rosso

Ogni test nuovo importante deve avere mutation verification quando il repository la richiede.

## Geometry authoritative accidentale

La mesh non decide gameplay.

---

# 25. Ordine di commit consigliato per #620

Adattare dopo audit.

Possibile sequenza:

```text
test(map): add geometry grammar fixtures
feat(map): add canonical quantized geometry representation
feat(map): add edge center/orientation helper
feat(map): validate tactical geometry grammar
test(map): cover geometry grammar boundaries and permutation
docs(map-editor): align geometry authoring spec with implementation
```

Commit focalizzati.

Non un commit da 40 file senza confine.

---

# 26. Ordine di commit consigliato per #621

```text
test(map): add geometry bake fixtures
feat(map): bake quantized segments to affected cells
feat(map): bake wall geometry to canonical cover edges
feat(map): bake solid and void footprints
test(map): verify bake determinism and map validation
feat(editor): invoke runtime bake from authoring tool
docs(map-editor): document authoring-to-runtime bake boundary
```

Solo se l’architettura reale suggerisce questa separazione.

---

# 27. Build e test

Prima di dichiarare Done:

- build Game;
- build Editor;
- suite Automation;
- test mirati;
- mutation check quando previsto;
- Feature Registry validate;
- docs link/symbol checks;
- eventuale packaged gate se la feature lo richiede;
- PIE/manuale se la resa è visuale.

Non dichiarare un PIE verde senza eseguirlo.

---

# 28. Verifica degli asset

Per `.uasset` / `.umap`:

- controllare `.gitignore`;
- controllare allowlist;
- `git check-ignore`;
- `git ls-files`.

Non presumere che `git add` abbia funzionato.

Questo repository ha già avuto asset presenti su disco ma non versionati.

---

# 29. Output finale che devi produrre dopo ogni issue

Alla fine del lavoro, scrivi:

```text
ISSUE:
BRANCH:
HEAD:
WHAT CHANGED:
WHAT WAS REUSED:
WHAT WAS NOT DUPLICATED:
TESTS:
MUTATION:
PIE:
FEATURE REGISTRY:
SCENARIOS:
DOCS:
ASSETS:
PACKAGED:
KNOWN LIMITS:
NEXT ISSUE:
```

Il campo **WHAT WAS REUSED** è obbligatorio in questo filone, perché il rischio principale è duplicare ciò che esiste.

---

# 30. Quando la base Level Designer è “usable”

Non serve avere subito heatmap e Overwatch analyzer.

Baseline pratica:

- Paint/Fill/Select/Arch;
- multilayer Focus;
- workspace visibile;
- wall/low-wall authoring discreto;
- bake canonico;
- cover/door/transition leggibili;
- path/cost leggibili;
- unreachable region evidente;
- map validation;
- lighting sufficiente;
- frame whole map;
- Undo/Redo affidabile;
- asset salvabile e ricaricabile;
- scenari/PIE tracciati.

Quando questi sono solidi, il Level Designer può già produrre mappe utili.

---

# 31. Secondo livello di roadmap da consolidare dopo la baseline

Dopo la baseline, prepara un referto e proponi la roadmap seguente.

## Navigation Sandbox

- Movement Probe
- Path Inspector
- connectivity
- cost breakdown
- blockers/reason codes

## Tactical Geometry

- LOS
- cover
- facing
- displacement
- ability geometry

## Combat Sandbox

- Overwatch
- reaction lanes
- predicted pressure
- objective pressure

## Environment Sandbox

- water
- fire
- electric
- ice
- smoke/steam
- noise

## Analytics

- movement heatmap
- combat heatmap
- KO heatmap
- cover usage
- exposure
- objective pressure
- noise
- displacement
- reactions

## Production

- spawn fairness
- complexity metrics
- archetype constraints
- batch procedural generation
- multi-select/mirror/prefab SOLO se misurati utili

---

# 31A. Skill / Ability Workbench — design e bilanciamento

Questa capability va considerata una naturale estensione del Level Designer verso il **Game/Combat Design Sandbox**.

NON deve diventare un secondo sistema di skill.

Principio:

```text
Designer intent / profile
        │
        ▼
canonical skill data / existing resolver inputs
        │
        ├── runtime execution
        ├── preview/probes
        ├── scenario harness
        └── balance analytics
```

Mai:

```text
Editor-only skill model
        │
        ▼
formula approssimata diversa dal runtime
```

Il Workbench deve usare, validare e visualizzare gli stessi dati e le stesse regole che il gioco usa realmente.

Prima di progettare API o asset nuovi, cercare owner e implementazioni esistenti per:

- skill / ability definitions;
- targeting;
- damage / mitigation;
- range;
- AoE / shape;
- displacement;
- cooldown;
- costs / action economy;
- phase / timing;
- reactions;
- status;
- environment interactions;
- hero data;
- scenario harness;
- TurnLog / replay;
- balance tests e data tables.

Se una responsabilità esiste già, estenderla o creare un editor adapter sottile. Non duplicarla.

## 31A.1 Due modalità di authoring

### Profile mode — default

Il designer non deve essere costretto a modificare numeri grezzi per ogni prova.

Deve poter comporre una skill attraverso **profili semantici** e preset leggibili.

Esempi di dimensioni configurabili:

```text
Delivery
- Direct
- Projectile
- Beam
- Area
- Cone / arc
- Self-centered
- Ground-targeted
- Delayed / predictive
- Reaction-triggered

Range profile
- Melee
- Short
- Medium
- Long
- Extreme

Impact profile
- Light
- Standard
- Heavy
- Finisher-like

Area profile
- Single target
- Small
- Medium
- Large
- Lane / line

Mobility / displacement
- None
- Step
- Dash
- Push
- Pull
- Knockback
- Reposition

Control
- None
- Soft control
- Hard control
- Zone denial

Timing / commitment
- Fast / low commitment
- Standard
- Slow / telegraphed
- Conditional
- Delayed

Availability
- Frequent
- Standard cooldown
- Long cooldown
- Conditional / setup gated
```

I nomi esatti devono essere coerenti con il vocabolario canonico del repository.

Questi profili NON sono autorità runtime.

Sono preset di authoring che producono valori canonici espliciti.

## 31A.2 Advanced mode

Un toggle `Advanced` deve mostrare i valori reali prodotti dal profilo e permettere override puntuali, secondo ciò che il modello dati canonico supporta.

Esempi:

- damage / healing;
- range minima/massima;
- shape / radius / width;
- falloff se esiste;
- cooldown;
- resource cost se esiste;
- action slot / opportunity cost;
- phase / timing;
- displacement distance;
- status duration / strength;
- targeting constraints;
- cover interaction;
- LOS rules;
- friendly-fire policy;
- reaction eligibility;
- environmental interaction flags;
- per-target / per-cast caps.

Regola UX:

> Profile mode deve essere veloce da usare; Advanced mode deve essere trasparente, non magica.

Ogni override deve essere visibile e distinguibile dal valore ereditato dal profilo.

Deve essere possibile:

- reset singolo override;
- reset tutti gli override;
- cambiare profilo preservando solo gli override compatibili, oppure richiedere una scelta esplicita;
- confrontare `profile value` vs `effective value`.

## 31A.3 Profili composabili, non prefab monolitici

Evitare un catalogo enorme di skill prefabbricate.

Preferire componenti/profili ortogonali:

```text
Delivery + Range + Impact + Area + Timing + Control + Availability
```

Esempio concettuale:

```text
Direct
+ Medium Range
+ Standard Impact
+ Single Target
+ Standard Timing
+ No Control
+ Frequent
```

oppure:

```text
Ground Area
+ Short Range
+ Light Impact
+ Medium Area
+ Delayed
+ Zone Denial
+ Long Cooldown
```

Il risultato deve essere sempre ispezionabile come dati canonici effettivi.

## 31A.4 Live map preview

La skill deve poter essere provata direttamente sulla mappa del Level Designer.

Selezionando:

- caster;
- skill/profilo;
- cella/target;
- facing se rilevante;

il tool deve poter mostrare, riusando query runtime:

- celle targettabili;
- range;
- LOS;
- cover interaction;
- AoE;
- falloff se esiste;
- valid/invalid target e reason code;
- displacement path / landing cell;
- affected environment;
- reaction opportunities generate dal cast;
- eventuali celle di pericolo / esposizione del caster;
- phase/timing in cui l'effetto si risolve.

Questo evolve naturalmente l'`Ability Geometry Probe` già previsto.

Non implementare tutte queste overlay in una issue unica.

## 31A.5 Balance panel

Il Workbench deve aiutare il designer a confrontare alternative, non dichiarare matematicamente che una skill è “bilanciata”.

Mostrare metriche osservabili e spiegabili.

Esempi, solo se derivabili dai sistemi reali:

- raw damage / healing;
- effective damage su target fixture;
- cells reachable/targetable;
- AoE cell count;
- expected affected targets su scenario fixture;
- cooldown / availability;
- action-slot opportunity cost;
- mobility/displacement gained;
- control duration / affected space;
- exposure / cover change;
- reaction opportunities created/consumed;
- environment cells modified;
- usage / hit / KO contribution nei batch scenario;
- variance fra mappe/posizioni/target.

Un eventuale `Power Budget` può esistere solo come **indicatore euristico esplicabile**, mai come verità canonica.

Deve mostrare quali componenti contribuiscono al punteggio.

Niente numero opaco tipo:

```text
Power = 83.7
```

senza breakdown.

## 31A.6 Compare mode

Il designer deve poter confrontare almeno due configurazioni:

```text
Skill A / current
Skill B / candidate
```

Con differenze per:

- effective canonical values;
- geometry;
- availability;
- costs;
- fixture results;
- scenario metrics.

Utile anche per confrontare:

- prima/dopo una modifica;
- due profili;
- stessa skill su due hero;
- stessa skill in due mappe/scenari.

## 31A.7 Test fixtures e target dummies

Aggiungere fixture editor/test riusabili per evitare valutazioni casuali.

Esempi:

- target senza cover;
- target in low cover;
- target in high cover;
- target a diverse distanze;
- gruppo compatto;
- gruppo disperso;
- target con mitigation/status rilevanti;
- corridor / choke point;
- open field;
- displacement near blocker/edge;
- environment interaction fixture.

Le fixture pure devono essere Automation quando possibile.

La leggibilità visuale resta PIE/manuale.

## 31A.8 Scenario/batch evaluation

Il valore vero del Workbench arriva quando una configurazione può essere testata in scenari reali.

Dopo la preview singola, prevedere progressivamente:

```text
Skill candidate
      │
      ▼
scenario fixture(s)
      │
      ▼
headless/runtime simulation
      │
      ▼
TurnLog / metrics
      │
      ▼
comparison report
```

Metriche candidate:

- cast count;
- legal-target rate;
- hit/effect rate;
- damage/healing/control contribution;
- displacement contribution;
- cover/environment interaction;
- reaction generation;
- cooldown downtime;
- action opportunity cost;
- KO contribution;
- position delta;
- objective pressure;
- map/side sensitivity.

Non introdurre metriche che richiedono un modello statistico finto se il runtime non produce ancora i dati necessari.

## 31A.9 Save / provenance

Distinguere chiaramente:

- preset/profile di partenza;
- valori effettivi;
- override;
- versione della skill;
- scenario/fixture usato;
- risultati di test.

Il designer deve poter salvare una **candidate configuration** senza sovrascrivere accidentalmente il dato di produzione.

Promozione candidate → canonical production data deve essere un'azione esplicita e validata.

## 31A.10 Guardrail

Il Workbench NON deve:

- creare un resolver editor-only;
- inventare un secondo targeting system;
- stimare LOS con formule diverse;
- mantenere copie indipendenti dei dati hero/skill;
- scrivere direttamente valori derivati che il runtime dovrebbe calcolare;
- usare un unico power score come gate automatico;
- trasformarsi in un spreadsheet nascosto senza preview spaziale;
- fondere in una sola PR authoring, simulator, analytics, UI e tutte le skill.

## 31A.11 Issue decomposition suggerita

Solo dopo audit del repository e solo se capability equivalenti non esistono già.

Esempio di decomposizione osservabile:

1. `Editor: Ability Workbench loads canonical skill definition and exposes effective parameters`
2. `Editor: skill profiles generate canonical candidate parameters with explicit overrides`
3. `Editor: Ability Geometry Probe previews runtime targeting and affected cells`
4. `Tooling: ability fixture runner compares candidate configurations using runtime rules`
5. `Tooling: scenario batch report exposes explainable ability balance metrics`
6. `Editor: compare mode visualizes two ability candidates and fixture results`

Non creare necessariamente una nuova Epic se le issue possono vivere negli owner/epic esistenti.

Prima cercare Feature Registry, roadmap e issue live.

## 31A.12 DoD della prima slice utile

La prima slice NON deve includere tutto il Balance Lab.

Una prima capability utile è sufficiente se permette di:

1. selezionare una skill canonica esistente;
2. selezionare un profilo semplice;
3. vedere i valori effettivi generati;
4. attivare Advanced e fare override;
5. selezionare caster + target sulla mappa;
6. vedere geometry/target validity usando runtime rules;
7. eseguire almeno una fixture deterministica;
8. confrontare baseline vs candidate;
9. salvare la candidate separatamente dalla produzione;
10. avere Automation per profile→effective data e fixture result.

Questo è sufficiente per iniziare a usare il designer come strumento reale di bilanciamento senza costruire subito un sistema analytics enorme.

---

# 32. Principio per i probe futuri

Ogni probe editor deve rispettare:

```text
Runtime Rule/Service
       │
       ▼
Pure query / DTO
       │
       ▼
Editor visualization
```

Mai:

```text
Editor invents a parallel rule
       │
       ▼
looks similar to runtime
```

Se il risultato editor e runtime possono divergere, il tool perde valore.

---

# 33. Goal finale del consolidamento

Alla fine del filone, il repository deve raccontare una sola storia coerente:

```text
FRTCellId / graph / edges
        │
        ▼
canonical map data
        │
        ├── pathfinding
        ├── LOS
        ├── targeting
        ├── environment
        └── resolver
        ▲
        │
geometry authoring
   ── deterministic bake
        ▲
        │
Hex Map Editor Mode
        │
        ├── authoring
        ├── overlays
        ├── validation
        ├── probes
        └── sessions/scenarios
```

Non devono esistere due versioni della mappa:

- una per il designer;
- una per il gioco.

Il designer modifica/produce il dato canonico attraverso strumenti sicuri.

---

# 34. Primo task da eseguire

Se lo stato GitHub è ancora quello dell’audit:

> **inizia da #620.**

Procedura:

1. audit HEAD;
2. leggi #619 chiusa;
3. leggi #620;
4. correggi eventuale testo stale su #588;
5. individua il tipo dati minimo;
6. individua helper edge esistenti;
7. crea fixture/test prima della UI;
8. implementa grammar;
9. implementa validator;
10. mutation test;
11. build + suite;
12. aggiorna Feature Registry/docs owner;
13. PR focalizzata;
14. poi passa a #621.

Se #620 è già chiusa:

> NON rifarla. Vai alla prima issue aperta nella catena e ri-audita.

---

# 35. Messaggio finale per Claude

Non trattare questo handoff come una richiesta di “fare tante feature”.

Trattalo come una richiesta di:

> **rendere coerente e produttivo il Tactical Designer workflow esistente, facendo convergere Map/Level Designer, Skill Workbench e Scenario Composer senza duplicare il sistema.**

La qualità di questo lavoro si misura soprattutto da ciò che NON viene duplicato.

---

# 36. Estensione architetturale: Scenario Composer integrato

Il Tactical Designer non deve fermarsi a mappa e skill.

Una parte fondamentale del design di RefactorTactics è rappresentata dagli **scenari deterministici**, cioè configurazioni iniziali + sequenze di ordini/azioni dei character + aspettative verificabili.

Quindi il tool deve evolvere verso questo loop unico:

```text
MAP
  +
CHARACTERS / INITIAL STATE
  +
SKILLS / CANDIDATES
  +
SCENARIO TIMELINE
  +
EXPECTATIONS
  │
  ▼
RUNTIME RESOLVER / SCENARIO HARNESS
  │
  ├── TurnLog
  ├── replay
  ├── validation
  ├── balance metrics
  └── regression result
```

Non creare un secondo scenario language o un secondo resolver se il repository possiede già un formato scenario/harness canonico.

Prima di introdurre nuovi tipi, cercare almeno:

```text
Scenario
ScenarioHarness
ScenarioDefinition
ScenarioMap
fixture
TurnLog
Replay
Expected
Assertion
ActionPlan
PlannedAction
Planning
Commit
Resolve
phase
stable id
hero id
skill id
```

Obiettivo:

> il Scenario Composer deve essere un authoring layer visuale/editoriale sopra lo stesso scenario format che Automation e runtime possono eseguire.

Mai:

```text
VisualScenarioAsset
      │
      ▼
custom editor interpreter
      │
      ▼
"simile" al gioco
```

Preferire:

```text
Visual editor
      │
      ▼
canonical ScenarioDefinition / DTO
      │
      ▼
Scenario Harness / runtime resolver
```

---

# 37. Modello concettuale dello scenario

Verificare i nomi reali nel repository, ma il modello deve poter rappresentare almeno:

```text
Scenario
├── Metadata
│   ├── Stable Scenario ID
│   ├── title
│   ├── purpose
│   ├── feature coverage
│   └── tags
│
├── Initial World State
│   ├── map
│   ├── character placement
│   ├── facing
│   ├── HP/resources/status
│   ├── environment state
│   └── deterministic seed, se applicabile
│
├── Timeline / Plans
│   ├── Round 1
│   │   ├── Player/Character A planned actions
│   │   ├── Player/Character B planned actions
│   │   └── ...
│   ├── Round 2
│   └── ...
│
├── Interactive Decisions / Reactions
│   ├── deterministic choice
│   ├── scripted branch
│   └── explicit fallback policy
│
└── Expectations
    ├── state assertions
    ├── TurnLog assertions
    ├── event count assertions
    ├── legality assertions
    └── final outcome assertions
```

Lo scenario non deve dipendere da nomi player-facing fragili quando il repository dispone di Stable ID.

Se il roster usa nomi player-facing e Stable Hero ID separati, preservare gli ID canonici nei file scenario/replay e mostrare il display name solo in UI.

---

# 38. Scenario Timeline UI

La UI deve permettere al designer di capire **chi fa cosa, quando e perché**.

Esempio concettuale:

```text
┌───────────────┬───────────────────────────────┬──────────────────────┐
│ CHARACTERS    │             MAP               │ ACTION INSPECTOR     │
│               │                               │                      │
│ Gadget        │        live preview           │ Phaser / Attack      │
│ Phaser        │        LOS / range            │ Target: Enemy.B      │
│ Enemy.A       │        AoE / path             │ Phase: Blast         │
│ Enemy.B       │        reactions              │ Skill: Basic         │
├───────────────┴───────────────────────────────┴──────────────────────┤
│ SCENARIO TIMELINE                                                   │
│                                                                      │
│ Round 1 │ Planning │ Prep │ Dash │ Blast │ Move │ End               │
│ Gadget  │   ●      │  ●   │      │       │  →   │                   │
│ Phaser  │   ●      │      │      │   ●   │  →   │                   │
│ Enemy.A │   ●      │      │  →   │       │      │                   │
│ Enemy.B │   ●OW    │      │      │       │      │                   │
├──────────────────────────────────────────────────────────────────────┤
│ ▶ RUN   ▷ STEP   ↻ RESET   ● RECORD   COMPARE   VALIDATE            │
└──────────────────────────────────────────────────────────────────────┘
```

I nomi esatti delle fasi devono venire dal modello corrente del repository.

Non hardcodare una phase grammar storica se `main` è cambiata.

---

# 39. Action Inspector

Selezionando un'azione nella timeline, mostrare sulla mappa e nell'Inspector:

- actor/character;
- Stable ID;
- skill/action;
- target character/cell/edge/direction;
- facing iniziale e risultante se rilevante;
- fase/timing;
- range;
- LOS;
- cover;
- path;
- expected displacement;
- generated reaction opportunities;
- environment interactions;
- legality;
- reason codes in caso di invalidità;
- stato prima/dopo quando disponibile tramite dry-run/query runtime.

Il tool non deve "predire" con formule editor-only.

Se il runtime non possiede una pure query sufficiente, estrarla dal sistema canonico con test.

---

# 40. Tre modalità di creazione scenario

Il Tactical Designer deve convergere su tre workflow.

## 40.1 Create Empty

Il designer:

1. sceglie/crea la mappa;
2. piazza i character;
3. configura stato iniziale;
4. aggiunge round e ordini;
5. aggiunge aspettative;
6. valida;
7. esegue.

## 40.2 Record Play

Il designer avvia una sessione controllata e il tool registra gli **ordini canonici**, non input UI grezzi.

Output:

```text
initial state
+
canonical plans/actions
+
reaction choices
+
TurnLog reference/result
```

Dopo la registrazione, il designer può trasformarla in scenario modificabile.

Non registrare coordinate world-space o widget events se esistono `FRTCellId`, Stable ID e action DTO canonici.

## 40.3 Clone / Variant

Permettere di derivare una variante senza duplicazione incontrollata.

Esempi:

- stessa sequenza su mappa diversa;
- stessa mappa con candidate skill;
- stessa apertura con posizione iniziale differente;
- stessa fixture con cover/environment diverso;
- baseline vs candidate.

Mantenere provenance tra scenario base e variante se il data model lo consente.

---

# 41. Scenario Expectations

Ogni scenario utile deve poter dichiarare aspettative verificabili.

Esempi concettuali:

```text
EXPECT Enemy.A reaches Cell X
EXPECT Overwatch triggers exactly once
EXPECT Character.B HP after Blast is between N and M
EXPECT no illegal path is accepted
EXPECT door state == Open after interaction
EXPECT TurnLog contains Event.ReactionOpportunity
EXPECT final winner == Team.A
EXPECT deterministic replay hash == baseline
```

Le assertion reali devono usare i sistemi già esistenti.

Distinguere:

- **hard invariant**: deve sempre passare;
- **design expectation**: comportamento atteso dello scenario;
- **balance threshold**: può cambiare intenzionalmente e richiede review;
- **telemetry observation**: informazione, non gate.

Questa distinzione evita che una modifica di bilanciamento rompa falsamente test strutturali o che un bug venga scambiato per "balance change".

---

# 42. Feature coverage collegata agli scenari

Scenario Map e Feature Registry devono diventare navigabili in entrambe le direzioni.

```text
Feature
  │
  └── covered by → Scenario(s)

Scenario
  │
  └── validates/demonstrates → Feature(s)
```

Il designer deve poter visualizzare, quando i dati esistono:

```text
SCN-...
Coverage:
- RT-FEAT-...
- RT-FEAT-...
- RT-FEAT-...
```

Non inventare nuovi `RT-FEAT-*` se esiste già un owner più ampio.

Granularità fine può vivere in:

- scenario;
- test;
- checkpoint;
- acceptance criteria;
- issue.

Il Feature Registry non deve diventare una lista di ogni singolo toggle UI.

---

# 43. Integrazione Scenario Composer ↔ Skill Workbench

Questa è una capability centrale.

Il designer deve poter usare una **candidate skill configuration** dentro uno o più scenari senza promuoverla subito a produzione.

Flusso:

```text
Canonical skill
      │
      ├── baseline
      │
      └── candidate override/profile
                 │
                 ▼
            Scenario A
            Scenario B
            Scenario C
                 │
                 ▼
        deterministic execution
                 │
                 ▼
          TurnLog / metrics
                 │
                 ▼
       baseline vs candidate diff
```

Il report deve distinguere:

- scenario ancora valido;
- scenario con metriche cambiate;
- expectation di balance superata;
- hard invariant fallita;
- outcome tattico cambiato;
- comportamento non deterministico.

Esempio:

```text
Candidate: Phaser.BasicAttack damage 24 → 30

Affected scenarios: 17
PASS / unchanged:          12
PASS / metrics changed:     3
BALANCE EXPECTATION FAIL:   1
HARD INVARIANT FAIL:        1
```

La classificazione deve derivare dai tipi di assertion reali, non da string matching fragile.

---

# 44. Scenario impact analysis

Quando cambia una delle seguenti entità:

- map;
- hero;
- skill;
- action rule;
- terrain/environment;
- LOS/cover/pathfinding;
- reaction;
- phase/timing;

il repository/tooling dovrebbe progressivamente riuscire a rispondere:

> quali scenari possono essere impattati?

Implementare prima una relazione esplicita e semplice tramite Stable ID/feature references.

Non costruire subito un dependency graph sofisticato se bastano riferimenti diretti.

Obiettivo futuro:

```text
Modified: Hero.Flux / Skill.X
Potentially affected:
- SCN-001
- SCN-004
- SCN-018
...
```

Poi eseguire solo la suite pertinente quando utile.

---

# 45. Naming: da Level Designer a Tactical Designer

Il concetto finale è più ampio di un semplice Level Designer.

Nome concettuale raccomandato:

> **RefactorTactics Tactical Designer**

Capability interne:

```text
Tactical Designer
├── Map / Level Authoring
├── Character Setup
├── Skill Workbench
├── Scenario Composer
├── Runtime Probes
├── Simulation / Replay
├── Balance Analysis
└── Validation / Coverage
```

ATTENZIONE:

Non fare un mass rename immediato di moduli/classi/file già stabili solo per uniformare il branding.

Prima:

1. audit del naming attuale;
2. separa product/tool name da C++ API names;
3. rinomina solo dove riduce davvero ambiguità;
4. evita churn in file ad alto conflitto.

---

# 46. Roadmap di maturità Tactical Designer fino alla v1.0

Questa roadmap descrive **milestone di capability del Tactical Designer**.

NON assumere automaticamente che `Tactical Designer v0.3` debba coincidere con `RefactorTactics game v0.3`.

Durante l'audit:

- identifica le release/milestone reali del progetto;
- mappa ogni capability alla roadmap esistente;
- preserva i significati già canonici delle versioni di gioco;
- se il repository usa una sola numerazione globale, integra queste capability nei milestone corretti senza rinumerare arbitrariamente il progetto.

Il risultato finale deve comunque mostrare chiaramente il percorso di maturità da **designer foundation** a **Tactical Designer 1.0 production-ready**.

---

# 47. Tactical Designer v0.1 — Deterministic Map + Scenario Foundation

## Obiettivo

Avere il minimo ambiente utilizzabile per costruire una situazione tattica deterministica e rieseguirla.

## Capability

- Map Editor baseline realmente usable;
- dati mappa canonici;
- geometry grammar/bake necessaria;
- pathfinding/LOS/cover overlays già disponibili o collegati;
- character placement;
- Stable ID nei riferimenti scenario;
- load scenario esistente;
- run;
- step se supportabile senza duplicare resolver;
- reset;
- TurnLog visibile/collegato;
- validation errors leggibili;
- scenario fixture minima Automation.

## Gate

Il designer può:

1. aprire una mappa canonica;
2. piazzare almeno i character richiesti da uno scenario;
3. caricare/eseguire uno scenario esistente;
4. ottenere lo stesso risultato headless/runtime;
5. vedere perché un ordine è invalido;
6. salvare e ricaricare senza perdere Stable ID.

## Non incluso

- editor completo delle skill;
- analytics avanzati;
- batch massivi;
- dashboard balance.

---

# 48. Tactical Designer v0.2 — Scenario Composer MVP

## Obiettivo

Creare e modificare scenari senza scrivere manualmente ogni file.

## Capability

- Scenario Timeline;
- round/action authoring;
- actor/target/cell selection;
- action inspector;
- phase/timing visuale;
- character initial-state editor;
- expectations basilari;
- save/load canonico;
- validation;
- execute/reset;
- Scenario ↔ Feature coverage references;
- clone scenario;
- fixture conversion quando possibile.

## Gate

Da UI è possibile creare uno scenario piccolo equivalente a una fixture scritta a mano e ottenere lo stesso TurnLog/result.

## Test richiesti

- visual authoring → canonical scenario serialization;
- canonical scenario → reload → semantic equality;
- deterministic execution;
- invalid action mutation test;
- Stable ID survival.

---

# 49. Tactical Designer v0.3 — Skill Workbench MVP

## Obiettivo

Disegnare candidate skill con profili leggibili e valori avanzati, usando sempre dati/regole runtime.

## Capability

- load canonical skill;
- semantic profile mode;
- composable profiles;
- Advanced values;
- explicit overrides;
- profile value vs effective value;
- candidate separated from production;
- Ability Geometry Probe;
- target validity/reason codes;
- deterministic fixture runner;
- baseline vs candidate compare.

## Gate

Una skill esistente può essere caricata, modificata come candidate, provata sulla mappa e confrontata con la baseline senza cambiare il dato production.

---

# 50. Tactical Designer v0.4 — Integrated Map + Skill + Scenario Loop

## Obiettivo

Chiudere il primo vero loop di game design.

```text
modify map / setup
      ↓
modify candidate skill
      ↓
run scenario
      ↓
inspect TurnLog/result
      ↓
compare baseline
      ↓
iterate
```

## Capability

- candidate skill binding nello scenario;
- scenario baseline vs candidate;
- before/after world-state inspection;
- scenario diff;
- hard invariant vs balance expectation distinction;
- affected scenario references;
- rerun selected scenario set;
- saved experiment/candidate provenance.

## Gate

Il designer può dimostrare una modifica di bilanciamento usando almeno 3 scenari diversi e capire quali expectation sono cambiate.

---

# 51. Tactical Designer v0.5 — Tactical Probes & Explainability

## Obiettivo

Rendere osservabili le ragioni tattiche, non solo il risultato finale.

## Capability progressiva

- movement/path inspector;
- cost breakdown;
- blockers/reason codes;
- LOS inspector;
- cover inspector;
- facing;
- displacement path/landing;
- reaction opportunities;
- Overwatch/reaction lanes se presenti nel runtime;
- environment interaction preview;
- objective pressure solo se definita canonicamente;
- state/event timeline inspection.

## Gate

Per una action/skill/scenario selezionata, il designer può spiegare con dati runtime perché:

- target è valido/non valido;
- path passa/non passa;
- cover si applica/non si applica;
- reaction nasce/non nasce;
- displacement termina in una certa cella.

Nessuna explainability può basarsi su una regola differente dal resolver.

---

# 52. Tactical Designer v0.6 — Scenario Recording & Replay Authoring

## Obiettivo

Trasformare rapidamente una sessione interessante in scenario riproducibile.

## Capability

- Record Play;
- cattura ordini canonici;
- cattura reaction choices deterministiche;
- initial state snapshot controllato;
- convert recording → editable scenario;
- replay link/TurnLog;
- trim round/actions;
- clone as variant;
- deterministic replay validation.

## Gate

Una breve sessione registrata può essere convertita in scenario, modificata, rieseguita e produrre un replay deterministico.

---

# 53. Tactical Designer v0.7 — Batch Simulation & Balance Analytics

## Obiettivo

Passare da prova singola a confronto sistematico senza introdurre false precisioni.

## Capability

- batch di scenari selezionati;
- baseline/candidate batch compare;
- TurnLog metric extraction;
- per-scenario e aggregate metrics;
- movement heatmap;
- hit/effect contribution;
- damage/healing/control contribution;
- displacement;
- reaction generation/consumption;
- cover/environment interactions;
- KO contribution;
- cooldown/availability observations;
- side/map sensitivity quando misurabile;
- export report machine-readable e human-readable.

## Gate

Il designer può confrontare due candidate su una suite di scenari e ottenere metriche spiegabili riconducibili a eventi runtime.

## Guardrail

Niente "AI balance score" o power score opaco come gate.

---

# 54. Tactical Designer v0.8 — Scenario Matrix, Variants & Regression Suites

## Obiettivo

Ridurre il costo di validare combinazioni tattiche importanti.

## Capability

- parameterized scenario variants;
- map/setup variants;
- hero/skill candidate variants;
- cover/environment variants;
- controlled deterministic seeds se usati;
- golden scenario suite;
- feature-based suite selection;
- impact-driven rerun;
- scenario health report;
- stale scenario detection;
- expected-change workflow.

## Gate

Una modifica a una feature/skill può identificare una suite pertinente e classificare:

```text
unchanged
expected balance change
unexpected regression
hard invariant failure
scenario stale / requires migration
```

---

# 55. Tactical Designer v0.9 — Production Authoring & Governance

## Obiettivo

Rendere il tool affidabile per uso quotidiano, non solo per sviluppatori che conoscono internals.

## Capability

- Undo/Redo affidabile sulle operation supportate;
- dirty-state chiaro;
- autosave/recovery se coerente con tooling UE esistente;
- candidate provenance;
- explicit promote candidate → canonical;
- validation gate prima della promozione;
- diff prima del commit/save canonico;
- dependency/impact summary;
- stable serialization/migration;
- performance su mappe/scenari target;
- error reporting/actionable diagnostics;
- editor sessions documentate;
- user-facing designer documentation;
- regression suite stabile.

## Gate

Un designer può lavorare senza dover conoscere il formato interno di ogni asset e senza rischiare di sovrascrivere accidentalmente dati production con una candidate.

---

# 56. Tactical Designer v1.0 — Production-Ready Tactical Design Environment

## Visione

La v1.0 non significa "ogni tool immaginabile".

Significa che il core workflow è completo, coerente e affidabile:

```text
AUTHOR MAP
    ↓
SETUP CHARACTERS
    ↓
DESIGN / MODIFY SKILL CANDIDATE
    ↓
COMPOSE OR RECORD SCENARIO
    ↓
VALIDATE
    ↓
RUN / STEP / REPLAY
    ↓
COMPARE BASELINE vs CANDIDATE
    ↓
RUN RELEVANT SUITE
    ↓
INSPECT EXPLAINABLE METRICS
    ↓
PROMOTE INTENTIONAL CHANGE
```

## Capability obbligatorie v1.0

### Map / Level Authoring

- dato canonico;
- deterministic geometry authoring/bake necessario;
- validation;
- path/LOS/cover visibility;
- persistence;
- production-safe editing.

### Character Setup

- placement;
- facing;
- initial state;
- Stable ID;
- team/ownership dove richiesto.

### Skill Workbench

- canonical skill load;
- profiles;
- advanced values;
- overrides;
- live runtime preview;
- candidate lifecycle;
- compare.

### Scenario Composer

- timeline;
- create/edit/clone;
- expectations;
- feature coverage;
- record/replay conversion;
- canonical serialization.

### Runtime Probes

- legality;
- range/targeting;
- path;
- LOS;
- cover;
- displacement;
- reaction/environment where supported.

### Simulation / Regression

- deterministic execution;
- headless scenario support;
- baseline/candidate;
- relevant suite selection;
- regression classification.

### Analytics

- explainable metrics from runtime/TurnLog;
- scenario and batch views;
- no opaque balance oracle.

### Governance

- candidate vs production separation;
- explicit promotion;
- validation;
- provenance;
- docs/wiki;
- Feature Registry;
- Scenario Map;
- roadmap;
- issue traceability.

## v1.0 Definition of Done

La capability è v1.0 solo se:

1. non esiste un rules engine parallelo nell'editor;
2. preview e scenario execution usano dati/regole canonici;
3. una mappa può essere creata/modificata e validata;
4. uno scenario può essere creato/modificato/registrato e rieseguito;
5. una candidate skill può essere configurata senza alterare production;
6. baseline e candidate possono essere confrontate;
7. scenari hanno Stable ID e feature coverage tracciabile;
8. Automation copre serialization, determinism e invarianti critiche;
9. regressioni possono essere distinte da cambi di balance intenzionali;
10. TurnLog/replay permettono diagnosi di una differenza;
11. docs/wiki descrivono il workflow reale;
12. roadmap/Feature Registry/Scenario Map non raccontano storie incompatibili;
13. le issue residue sono note e non bloccano il core workflow;
14. una nuova persona può seguire la wiki e completare un piccolo ciclo Map → Skill → Scenario → Compare senza leggere il codice sorgente.

---

# 57. Come Claude deve consolidare la roadmap reale

Non aggiungere questa roadmap come secondo documento concorrente e basta.

Claude deve:

1. trovare l'owner canonico della roadmap;
2. leggere milestone/release correnti;
3. individuare capability già pianificate;
4. mappare le sezioni v0.1→v1.0 sopra la roadmap esistente;
5. aggiornare milestone/checkpoint esistenti;
6. creare nuovi checkpoint solo per gap reali;
7. indicare dipendenze;
8. indicare gates osservabili;
9. evitare date arbitrarie se non esiste una pianificazione temporale reale;
10. mantenere separati `release goal` e `nice-to-have`.

Se la roadmap è YAML/JSON/generated:

- modificare la source of truth;
- rigenerare gli artefatti derivati;
- non patchare solo HTML/Markdown generato.

---

# 58. Audit e consolidamento GitHub Issues

Questa attività è parte del task, non opzionale.

Prima di creare nuove issue:

```text
SEARCH
  ↓
OPEN issue?
  ├─ sì → aggiorna/consolida
  └─ no
       ↓
CLOSED issue già implementa capability?
  ├─ sì → non riaprire senza motivo; collega stato/as-built
  └─ no
       ↓
vero gap?
  ├─ sì → crea issue focalizzata
  └─ no → nessuna issue
```

## 58.1 Cerca per concetti, non solo per titolo

Cercare issue relative a:

- level designer;
- map editor;
- scenario editor/composer/harness;
- fixtures;
- replay;
- TurnLog;
- skill/ability editor;
- balance;
- analytics;
- probes;
- validation;
- feature registry;
- scenario map;
- editor sessions.

## 58.2 Aggiorna issue esistenti quando

- descrivono già la capability;
- acceptance criteria sono incomplete;
- dipendenze sono stale;
- sono state parzialmente implementate;
- la capability è stata spostata nella roadmap;
- il naming è cambiato ma il lavoro è lo stesso.

## 58.3 Crea issue nuove solo quando

- nessuna issue esistente possiede il gap;
- il gap è abbastanza focalizzato per una PR;
- ha DoD osservabile;
- ha dipendenze esplicite;
- è mappabile a Feature Registry/scenario/test.

## 58.4 Non creare una mega-issue "Implement Tactical Designer v1.0"

Può esistere un Epic/umbrella SOLO se la convenzione reale del repository lo usa e se aggiunge valore di tracking.

Le implementazioni devono restare issue piccole/focalizzate.

---

# 59. Backlog capability suggerito da riconciliare con issue esistenti

Questi sono **titoli concettuali**, NON ordini di creare issue con questi nomi.

Claude deve cercare owner equivalenti e riusarli.

## Foundation

- Map authoring canonical-data audit
- Deterministic tactical geometry authoring
- Geometry bake to canonical graph
- Map validation and reason codes
- Character placement / initial tactical state
- Scenario runner integration in editor

## Scenario Composer

- Scenario canonical serialization adapter
- Scenario timeline authoring
- Initial state editor
- Action inspector
- Scenario expectations editor
- Scenario clone/variant workflow
- Scenario ↔ feature coverage
- Record play to canonical scenario

## Skill Workbench

- Canonical skill effective-parameter inspector
- Profile → canonical candidate parameters
- Advanced overrides
- Ability Geometry Probe
- Candidate lifecycle/provenance
- Fixture runner
- Compare baseline/candidate

## Integration

- Candidate skill binding to scenario
- Scenario result diff
- Hard invariant vs balance expectation classification
- Affected-scenario lookup
- Relevant-suite execution

## Analytics

- TurnLog metric extraction
- Per-scenario metric report
- Batch comparison
- Heatmaps/overlays only where data exists
- Scenario health/staleness report

## Production

- Promotion candidate → canonical
- Validation/preflight
- Undo/Redo coverage
- serialization/migration
- diagnostics
- documentation/onboarding

---

# 60. Issue body standard

Quando Claude crea o modifica una issue Tactical Designer, preferire questa struttura, adattata alle convenzioni repository:

```markdown
## Why
Problema concreto / gap.

## Current state
Cosa esiste già su main.

## Scope
Una capability focalizzata.

## Non-goals
Cosa NON entra in questa issue.

## Canonical owners to reuse
Runtime/data/services già esistenti da usare.

## Acceptance criteria
- [ ] comportamento osservabile 1
- [ ] comportamento osservabile 2
- [ ] comportamento osservabile 3

## Tests
Automation / fixture / scenario / PIE richiesti.

## Documentation
Owner docs/wiki/registry da aggiornare.

## Dependencies
Issue/feature reali.

## Roadmap / Feature / Scenario mapping
Link agli owner reali.
```

Non copiare boilerplate inutile se la repo possiede già un template migliore.

---

# 61. Consolidamento Feature Registry

Claude deve trovare il registry canonico e:

- riusare Feature ID esistenti;
- evitare Feature ID per ogni pannello UI;
- distinguere feature gameplay da tooling capability;
- collegare issue/scenari/test dove previsto;
- aggiornare status in base a codice reale, non in base a questo prompt.

Se il repository possiede già owner macro come:

```text
Map Editor
Scenario Harness
Skill/Ability System
TurnLog/Replay
```

preferire estensione/checkpoint rispetto a moltiplicare feature parallele.

---

# 62. Consolidamento Scenario Map

Claude deve:

1. trovare la Scenario Map canonica;
2. censire scenari esistenti;
3. eliminare riferimenti stale;
4. collegare Stable Scenario ID;
5. collegare feature coverage;
6. indicare livello:
   - fixture;
   - headless scenario;
   - PIE/manual scenario;
   - balance benchmark;
7. aggiungere gli scenari mancanti solo quando servono a validare capability reale;
8. evitare 20 scenari quasi identici se una fixture parametrica è migliore.

Per il Tactical Designer, prevedere almeno scenari/fixture che coprano progressivamente:

- map/path baseline;
- LOS/cover;
- character placement/facing;
- skill targeting;
- displacement;
- reaction/Overwatch quando supportata;
- environment interaction;
- scenario timeline serialization;
- candidate vs baseline;
- expected balance change;
- hard regression.

---

# 63. Consolidamento documentazione tecnica

Claude deve identificare gli owner canonici e aggiornare **quelli**, non creare copie.

La documentazione tecnica deve spiegare almeno:

## Architecture

```text
Canonical Game Data / Rules
          │
          ├──────── runtime resolver
          │
          ├──────── scenario harness
          │
          ├──────── TurnLog/replay
          │
          └──────── pure queries
                        │
                        ▼
                 Tactical Designer
```

## Ownership

- chi possiede map data;
- chi possiede scenario data;
- chi possiede skill data;
- chi possiede TurnLog;
- quali adapter sono editor-only;
- cosa NON può diventare authority nell'editor.

## Data flow

- profile → candidate values;
- candidate → scenario override;
- scenario → resolver;
- resolver → TurnLog;
- TurnLog → metrics;
- candidate promotion → production.

## Determinism

Documentare Stable ID, serialization, replay e determinism constraints rilevanti.

---

# 64. Wiki: separare UX designer da internals sviluppatore

La wiki dovrebbe avere una pagina/area orientata a chi **usa** il Tactical Designer.

Esempio struttura:

```text
Tactical Designer
├── Overview
├── Create/Edit a Map
├── Place Characters
├── Design a Skill Candidate
│   ├── Profiles
│   └── Advanced mode
├── Create a Scenario
├── Record a Scenario
├── Add Expectations
├── Run / Step / Replay
├── Compare Baseline vs Candidate
├── Read Balance Metrics
├── Promote a Candidate
└── Troubleshooting / Validation Errors
```

Separare una sezione developer:

```text
Developer Notes
├── canonical data ownership
├── scenario schema
├── adapter architecture
├── determinism
├── test architecture
└── extension points
```

La wiki user-facing non deve essere una parete di C++.

---

# 65. Documentation drift gate

Dopo ogni issue/PR rilevante, verificare:

```text
CODE
ROADMAP
FEATURE REGISTRY
SCENARIO MAP
WIKI
TECH DOCS
ISSUE STATE
```

Se raccontano due stati diversi, il lavoro non è finito.

Non è necessario modificare tutti i file ad ogni PR, ma ogni owner impattato deve essere verificato.

---

# 66. Test strategy Tactical Designer

Il Tactical Designer deve avere test a strati.

## 66.1 Pure / data tests

- profile → effective values;
- override precedence;
- scenario serialization;
- Stable ID preservation;
- migration;
- expectation parsing/model;
- diff classification.

## 66.2 Runtime fixture tests

- targeting;
- LOS;
- cover;
- path;
- displacement;
- reactions;
- environment;
- skill candidate execution.

## 66.3 Scenario tests

- deterministic scenario execution;
- baseline vs candidate;
- expected balance change;
- hard invariant failure detection;
- replay equivalence.

## 66.4 Editor tests

Dove sostenibile:

- toolkit/model binding;
- save/load;
- selection/inspector;
- Undo/Redo.

Non forzare fragile UI automation per ciò che può essere testato sotto il widget layer.

## 66.5 PIE/manual

Usare per:

- readability;
- overlays;
- workflow;
- ergonomia;
- large-map performance perception.

---

# 67. Mutation tests obbligatori per evitare test cosmetici

Per feature importanti, almeno un test deve essere dimostrato capace di fallire modificando intenzionalmente il comportamento.

Esempi:

- invertire un reason code;
- cambiare una cella target;
- rimuovere Stable ID durante round-trip;
- cambiare ordine di due azioni;
- alterare damage candidate;
- non generare una reaction;
- cambiare un expected event count.

Il test deve diventare rosso per la ragione corretta.

Poi ripristinare.

---

# 68. Known project context da verificare su main

Questi elementi provengono dal contesto consolidato del progetto e devono essere **verificati**, non assunti ciecamente:

- Unreal Engine 5.8.1;
- core C++ deterministico;
- grafo/celle esagonali come base autorevole;
- TurnLog/replay come concern trasversale;
- v0.1 di gioco orientata a un vertical slice 2v2 offline contro bot;
- roster player-facing corrente: Gadget, Phaser, Victor, Wrath;
- Stable Hero ID legacy separati dai display name;
- niente secondo resolver editor-only;
- Feature Registry, roadmap e scenari come owner canonici da mantenere allineati.

Se `main` contraddice questo elenco, `main` + ADR/Decision Log più recenti vincono.

---

# 69. Ordine operativo richiesto a Claude

Non iniziare dal coding del Scenario Composer o del Balance Dashboard.

Esegui nell'ordine:

## Phase A — Repository truth

1. sync/audit `origin/main`;
2. leggi ADR/Decision Log;
3. trova roadmap owner;
4. trova Feature Registry;
5. trova Scenario Map;
6. trova Wiki/docs owner;
7. trova issue/PR rilevanti;
8. trova current editor architecture;
9. trova scenario/TurnLog/skill owners;
10. produci `TACTICAL DESIGNER AUDIT`.

## Phase B — Consolidation plan

11. mappa capability già esistenti;
12. mappa capability parziali;
13. mappa gap;
14. deduplica issue;
15. proponi mapping v0.1→v1.0;
16. individua la **prima issue eseguibile**.

## Phase C — Repository consolidation

17. aggiorna roadmap source of truth;
18. aggiorna Feature Registry;
19. aggiorna Scenario Map;
20. aggiorna docs/wiki owner;
21. aggiorna issue esistenti;
22. crea SOLO issue mancanti;
23. chiudi/collega duplicati secondo policy repository.

## Phase D — Implementation

24. implementa una issue alla volta;
25. test prima/durante;
26. build;
27. scenario/fixture;
28. docs impattate;
29. PR focalizzata;
30. ripeti sulla prossima issue solo se il workflow/sessione lo consente esplicitamente.

---

# 70. Deliverable di consolidamento iniziale

Prima di una grande implementazione, Claude deve produrre nel repository, nel posto appropriato, un riepilogo equivalente a:

```text
TACTICAL DESIGNER CONSOLIDATION REPORT

HEAD:
Relevant open PRs:
Relevant open issues:
Relevant closed issues:

Existing capabilities:
- ...

Partial capabilities:
- ...

Missing capabilities:
- ...

Duplicates/drift found:
- ...

Roadmap mapping:
TD v0.1 -> project milestone / issues ...
TD v0.2 -> ...
...
TD v1.0 -> ...

First executable issue:
...

Docs updated:
...

Feature Registry updated:
...

Scenario Map updated:
...

Issues updated:
...
Issues created:
...
Issues not created because already owned by:
...
```

Non serve necessariamente creare un file permanente con questo nome se il repository ha già un formato di audit/handoff migliore.

---

# 71. Acceptance criteria del consolidamento richiesto da questo handoff

Questo handoff è completato correttamente quando:

- [ ] Claude ha verificato `main` e non ha lavorato da una fotografia vecchia;
- [ ] esiste una roadmap coerente Tactical Designer fino a v1.0, mappata sulla roadmap reale;
- [ ] Map Editor, Skill Workbench e Scenario Composer sono descritti come parti dello stesso loop;
- [ ] nessun secondo runtime/resolver è stato introdotto;
- [ ] issue esistenti sono state aggiornate prima di crearne di nuove;
- [ ] le nuove issue hanno scope da PR focalizzata;
- [ ] Feature Registry è coerente;
- [ ] Scenario Map è coerente;
- [ ] docs tecniche sono coerenti;
- [ ] wiki orientata al designer descrive il workflow reale;
- [ ] scenari possono collegare feature/skill/map tramite Stable ID dove previsto;
- [ ] candidate skill e production data sono separati;
- [ ] hard invariants e balance expectations sono distinguibili;
- [ ] TurnLog/replay sono riusati per diagnosi/metriche;
- [ ] ogni milestone ha gate osservabili;
- [ ] la prima issue eseguibile è identificata in base allo stato reale;
- [ ] build/test richiesti passano per le modifiche effettuate.

---

# 72. Non-goals espliciti

Non fare, solo perché questo documento parla di v1.0:

- una mega PR;
- una riscrittura del Map Editor;
- un nuovo skill system;
- un nuovo scenario runtime;
- un secondo TurnLog;
- una replica editor del resolver;
- analytics inventati senza eventi runtime;
- AI balance oracle;
- procedurale massivo prima delle foundation;
- mass rename di ogni classe `LevelDesigner`;
- nuovi Feature ID per ogni checkbox;
- una nuova Epic se una esistente possiede già il lavoro;
- date di release arbitrarie;
- chiudere issue solo perché la roadmap le sposta.

---

# 73. Primo vertical slice Tactical Designer consigliato

Dopo aver chiuso/prerequisiti reali del Map Editor, il primo slice integrato da perseguire dovrebbe essere minimo ma end-to-end:

```text
1 MAP canonica
4 CHARACTER di test o il minimo richiesto dallo scenario
1 SKILL canonica
1 CANDIDATE tramite profilo + override
1 SCENARIO editabile
1 EXPECTATION hard
1 EXPECTATION balance
1 RUN deterministico
1 TURNLOG
1 BASELINE vs CANDIDATE DIFF
```

Il slice è riuscito se una persona può:

1. aprire la mappa;
2. vedere/piazzare i character;
3. selezionare una skill;
4. creare una candidate;
5. inserirla nello scenario;
6. modificare una action nella timeline;
7. eseguire;
8. vedere il risultato;
9. capire una differenza;
10. ripristinare baseline senza aver corrotto production data.

Questo vertical slice vale più di dieci pannelli scollegati.

---

# 74. Parallelizzazione

Se il repository/worktree strategy lo consente, Claude può proporre lavoro parallelo SOLO su slice con ownership di file sufficientemente separata.

Esempio concettuale:

```text
Branch A — canonical scenario DTO / serialization
Branch B — editor timeline model/view
Branch C — skill candidate/profile data
Branch D — fixture/diff/reporting
```

Ma solo dopo che:

- API boundaries sono chiare;
- owner canonici sono identificati;
- non si duplicano tipi per sbloccare branch;
- esiste un integration order.

Non creare quattro implementazioni divergenti della stessa astrazione per lavorare in parallelo.

---

# 75. Output finale richiesto a Claude dopo il consolidamento

Nel messaggio finale, riportare in modo concreto:

```text
1. HEAD / branch / PR
2. audit eseguito
3. capability esistenti riusate
4. roadmap aggiornata e dove
5. issue aggiornate
6. issue create
7. issue duplicate/non create e perché
8. Feature Registry aggiornato
9. Scenario Map aggiornata
10. docs tecniche aggiornate
11. wiki aggiornata
12. codice implementato, se previsto dalla issue corrente
13. test/build eseguiti
14. scenario/fixture usati
15. rischi/debito residuo
16. prossima issue eseguibile
```

Citare path e issue reali.

Non scrivere "done" se roadmap/wiki/issue non sono state realmente aggiornate.

---

# 76. Direttiva finale a Claude

Il risultato cercato non è un editor con tanti widget.

È un **sistema di design tattico verificabile** in cui:

> la mappa definisce lo spazio, i character definiscono lo stato, le skill definiscono le possibilità, gli scenari definiscono sequenze/intenzioni, il resolver produce la verità, il TurnLog spiega cosa è successo e gli analytics aiutano il designer a confrontare alternative.

Tutta la roadmap fino alla v1.0 deve preservare questa catena.

Se una feature rende il Tactical Designer più bello ma aumenta la distanza tra editor e runtime, non è prioritaria.

Se una feature riduce il tempo tra:

```text
idea → candidate → scenario → execution → explanation → comparison
```

allora è probabilmente nella direzione giusta.

