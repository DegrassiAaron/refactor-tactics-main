# RefactorTactics — Level Designer / Map Editor
## Prompt operativo per Claude Code: implementare, consolidare, allineare

**Leggere prima:** `RefactorTactics_LevelDesigner_01_Context_Claude.md`

Questo file NON è un invito a implementare tutto in una PR.
È un prompt operativo per:

1. verificare lo stato reale del repository;
2. consolidare il focus Level Designer;
3. eliminare drift e duplicazioni;
4. avanzare la roadmap reale partendo dalle issue esistenti;
5. creare issue SOLO dove esiste davvero un buco;
6. mantenere sincronizzati codice, roadmap, Feature Registry, Wiki, scenari, editor sessions e test.

---

# 0. Regola numero zero

NON fidarti dello stato descritto nel file di contesto.

È una fotografia del 2026-08-12.

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

> 🔎 **PANEL — parte di questo lavoro è già fatta: restringi il perimetro.**
>
> Nel Feature Registry la deriva su PR #598 **è già stata rettificata**, con tanto di nota esplicita:
> «*Questa nota citava la PR #598 come «aperta, non mergiata»: è mergiata dal 2026-08-12T07:07:42Z.
> L’argomento non dipendeva da quella PR ed è stato rimisurato.*»
>
> Quindi il residuo reale sono **soltanto i body delle issue #620 e #622** su GitHub. Non ripassare il
> registry né i docs: è lavoro già eseguito, e rifarlo produce un diff senza contenuto.
>
> Stato riverificato il 2026-08-12 su HEAD `402c154b` con `gh issue view`:
>
> | Issue | Stato |
> |---|---|
> | #588 · #619 | CLOSED |
> | #554 · #620 · #621 · #622 · #623 | OPEN |
>
> La catena di §6 del file 01 regge ancora **per intero**. È salito solo l’HEAD (`52c08286` → `402c154b`,
> ultimo commit sulla issue #651): il repository si è mosso altrove, non su questo filone.

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

> 🔎 **PANEL — la risposta è SÌ, ed è già in questo documento. Non rifare la ricerca.**
>
> Verificato su HEAD `402c154b` con `git grep` durante la revisione del 2026-08-12:
>
> | Cosa | Simbolo | Dove |
> |---|---|---|
> | Centro del bordo | `URTHexLibrary::EdgeMidpointWorld` | `RTHexLibrary.h:90` · `.cpp:182` |
> | Orientamento del bordo | `URTHexLibrary::EdgeRotation` | `RTHexLibrary.h:100` · `.cpp:191` |
> | Consumer già vivo | pannelli cover/porta | `RTHexMapActor.cpp:558, 569` |
> | Invariante protetta da test | `RefactorTactics.Hex.EdgeMidpointIsSharedByBothCells` | `RTHexTests.cpp:384` |
>
> Quindi: **riusale**, non aggiungerle. Il ramo «se no, aggiungila UNA volta» non si applica.
>
> L’invariante che quel test pinna è quella che serve a #620 — *lo stesso bordo fisico ha lo stesso centro
> visto dalle DUE celle che lo condividono* — ed è precisamente ciò che impedisce a una copertura di
> esistere due volte con due centri.
>
> ⚠️ Nota di metodo, perché è il difetto che questa revisione ha trovato più volte in entrambi i file:
> **un handoff che pone una domanda già risolta delega al lettore un audit che l’autore poteva chiudere
> con un grep.** Costa trenta secondi a chi scrive e può costare mezza giornata — o una duplicazione —
> a chi legge. In un filone la cui tesi è «la qualità si misura da ciò che NON viene duplicato», è la
> forma di debito più cara.

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
- ~~invalid junction~~ → **rimosso** (vedi §8 del file 01);
- zero-length segment;
- duplicate segment;
- invalid layer;
- outside editable bounds;
- **segment on sector boundary** → 🔎 PANEL: aggiunto, vedi D0-bis in §16 del file 01.

> 🔎 **PANEL — DECISA il 2026-08-12: due strati di validazione, entrambi già in repo.**
>
> La domanda era: un edit invalido viene **respinto** o **accettato-e-segnalato**? La risposta è che il
> repository ha già entrambi, con ruoli distinti, e va seguito il precedente invece di sceglierne uno.
>
> | Strato | Chi | Comportamento | Precedente |
> |---|---|---|---|
> | Grammatica #620 | funzione pura, chiamata dal tool | **rifiuta** al commit dell’edit | `RTHexCoverLibrary` — *«**Più severo di `ValidateMap`, e va detto**»* (`.h:136`) |
> | `ValidateMap` | gate su dati persistiti e **cotti** | **enumera**, non blocca | `RTHexMapAsset.cpp:303` → `TArray<FString>`, loggati come warning da `RTHexMapActor.cpp:690-698` |
>
> `ValidateMap` **non blocca niente** ed è un fatto, non un’opinione: l’asset può già contenere celle
> duplicate, costi negativi e coperture incoerenti. Quindi lo stato invalido è **rappresentabile** —
> come lo è oggi un `MoveCost` negativo — ma non **raggiungibile** dall’editor, perché il tool rifiuta
> prima. Nessun costo nuovo: è lo stesso modello di minaccia di oggi.
>
> Il secondo strato non è opzionale. Il commento di `ValidateMap:321-322` lo designa già esplicitamente:
> *«questo è il gate che deve restare verde sui dati **COTTI** (#621)»*.
>
> **E questo risolve anche il procedurale** (§26 del file 01): un generatore non passa dal tool, quindi
> produce dati che `ValidateMap` deve poter respingere. Il modello a due strati è precisamente ciò che
> rende importabile un batch senza doversi fidare del generatore.

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
GeometryGrammar.PermutationInvariant
```

> 🔎 **PANEL — DECISA il 2026-08-12: `ValidJunction` e `RejectsInvalidJunction` sono stati RIMOSSI da
> questa suite.** La junction non è un concetto di #620: l’OR di `ComputeMask` la rende trasparente, e
> la rappresentazione a polilinea rende la continuità un’invariante di tipo. La motivazione completa è
> in §8 del file 01. Se la grammatica è una polilinea, non c’è niente da rifiutare.

Nomi da adattare alle convenzioni reali.

> 🔎 **PANEL — due buchi in questa suite.**
>
> **1. Le fixture esistono già.** Il §25 propone un commit «add geometry grammar fixtures» senza dire dove.
> Il Feature Registry lo dice: `Source/RefactorTactics/Tests/RTOccupancyFixtures.h`, dove vivono le quattro
> fixture di geometria «da dove le useranno anche #620 e #621». Estendi quel file, non crearne un secondo.
> Le fixture di geometria **non** vanno in `Scenarios/`: `FRTScenarioCell` non ha campi per segmenti o footprint.
>
> **2. Manca il test che tiene insieme la catena.** Undici test per #620, «testato» per #621, esempi senza
> nome per #554: ogni issue ha i suoi, nessuno dimostra che le tre raccontino la stessa storia. Ne basta uno:
>
> ```text
> Spec.Map.BakedWallSeversThePath
>
> Given  una mappa con un unico percorso fra spawn e obiettivo
> When   si cuoce un muro che attraversa quel percorso
> Then   il conteggio di irraggiungibili di #554 sale
> And    VALIDATE MAP segnala la regione isolata
> ```
>
> Il gemello esiste già — `Scenarios/Spec/Map/BridgeBreaksThePath.json` — quindi la forma è nota e il costo
> è basso. Questo è uno **scenario**, non una fixture: dimostra una proprietà gameplay osservabile, che è
> il criterio di §19.
>
> Va messo come **DoD condivisa** fra #621 e #554, non assegnato a una delle due: è l’unico artefatto che
> diventa rosso se le due divergono, ed è quindi l’unico che protegge la giunzione fra loro.

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

# 10-bis. Formato, hash e migrazione — 🔎 PANEL

Sezione aggiunta dalla revisione del 2026-08-12: **mancava del tutto in entrambi i file**, ed è la
categoria di rischio che #620 e #621 introducono senza che nessuno l’abbia nominata.

Il precedente c’è, ed è recentissimo. `#619` ha alzato il formato mappa e ha fatto entrare il proprio
campo nell’hash di partita:

```text
RTHexMapAsset.h:65   static constexpr int32 CurrentFormatVersion = 7;   (era 6)
```

con il test che lo protegge — `SurchargeEntersTheMatchStateHash` — fra i sedici `RefactorTactics.HexOccupancy.*`.

Ora: **#620 introduce una nuova rappresentazione serializzata** e **#621 scrive campi canonici**. Sono
altre due potenziali salite di formato sulla stessa serie. Le domande da rispondere **prima** di aprire
il branch, non dopo:

| Domanda | Perché non è rimandabile |
|---|---|
| La grammatica di #620 viene **serializzata** nell’asset mappa? | Se sì → `CurrentFormatVersion` sale a 8, e serve la migrazione delle mappe già salvate |
| La geometria di authoring entra in **`RTMatchStateHash`**? | Dovrebbe **no** — dopo il bake è arte (§10). Ma va deciso e *testato*, non assunto: è la differenza fra «arte» e «dato competitivo» |
| I campi cotti da #621 sono già nell’hash? | `FRTHexCover` e `bBlocksMovement` sono dati di gameplay: se il bake li cambia, cambia l’hash di ogni mappa esistente |
| Gli scenari registrati sopravvivono? | `Scenarios/Spec/Map/` contiene oggi due file. Se l’hash si muove, vanno ri-registrati — e va detto **da chi** |

Regola che ne segue, sul modello già applicato da #619:

> Ogni salita di `CurrentFormatVersion` porta con sé **il test che la pinna** e la nota di cosa succede
> alle mappe salvate. Il precedente da imitare è `SurchargeEntersTheMatchStateHash`.

⚠️ Se la risposta alla seconda riga è «la geometria di authoring **non** entra nell’hash», allora serve
un test che lo dimostri: disegnare geometria senza cuocerla **non deve** cambiare l’hash di partita.
È l’unico modo di rendere verificabile la frase «dopo il bake, la geometria è arte».

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

> 🔎 **PANEL — la decisione ESISTE, ha un ID, ed è un gate di #621. Non aprirne una seconda.**
>
> È **`MSE-1`**, in `docs/OPEN_DECISIONS.md` (sezione «l’invertibilità della cottura, dallo spec panel
> del 2026-08-12»). Formulazione registrata:
>
> > Quando la geometria disegnata è **cotta** in `bBlocksMovement` / `FRTHexCover`, il sorgente resta
> > autorevole? Cioè: se qualcuno modifica a mano un campo cotto, vince la modifica o il prossimo rebake
> > la cancella?
>
> Tre cose che questo documento non diceva e che cambiano il lavoro:
>
> 1. **L’innesco è pinnato a #621**, non a #619. La ristrettezza del 2026-08-12 lo motiva: `D2` dà al
>    **costo** un produttore solo, quindi la cottura di #619 non innesca `MSE-1`; `FRTHexCover` e
>    `bBlocksMovement` restano invece a produttore condiviso col pennello, ed è lì che la domanda va decisa.
> 2. **È il motivo per cui `RT-FEAT-TOOL-MAP-GEOMETRY` ha `spec: partial`.** Non è un rischio da annotare:
>    è la ragione registrata di un gate aperto. **#621 non si dichiara Done senza chiuderla.**
> 3. **Esiste già un candidato di risposta**, indicato dall’autore nella nota di metodo di `MSE-1`:
>    *separare i produttori invece di arbitrarli* — la stessa forma di `D2`. «Se regge per il costo, è il
>    primo candidato da provare sui bordi.» Prova quello per primo, prima di progettare provenance o flag.
>
> L’elenco di policy qui sopra resta utile come spazio delle opzioni. Ma la riga «apri una decisione/issue
> solo se è bloccante» **è già stata eseguita**: la decisione è aperta e tracciata. Aprirne un’altra è
> esattamente la duplicazione che questo documento esiste per prevenire.
>
> ⚠️ Vedi anche §9 del file 01: `MSE-1` è formulata su due campi, ma dopo #621 i campi contesi sono **tre** —
> manca `ERTHexSurface`, scritta sia dal bake (void/cliff) sia da Paint/Fill.

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

> 🔎 **PANEL — controlla il worktree PRIMA di iniziare: #554 è già in lavorazione.**
>
> `git worktree list` su HEAD `402c154b` mostra:
>
> ```text
> D:/Repositories/rt-migrazione   0485a7b6   [feat/554-transizioni-visibili]
> ```
>
> Esiste un branch dedicato e attivo. Il §1 di questo file dice già «non lavorare direttamente sul working
> tree usato da altre sessioni»: qui si applica letteralmente. **Prima di aprire #554, leggi quel branch** —
> parte del lavoro potrebbe essere fatta, e la misura headless potrebbe già esistere.
>
> **Sequenziamento — questo documento si contraddice su #554**, e con un solo agente la contraddizione conta:
>
> | Dove | Cosa dice |
> |---|---|
> | File 01 §6 | «in parallelo» |
> | File 01 §28 Fase B | terzo in fila, dopo #620 e #621 |
> | Qui, §13 | «in parallelo / subito dopo» |
>
> Il §1 impone «scegli una issue singola». Decidi una volta e scrivilo: **#554 non dipende da #620/#621**
> — legge il grafo, non la geometria cotta — quindi è genuinamente parallelizzabile, ed è l’unica delle
> cinque che lo sia. Con un solo agente resta comunque una scelta di ordine, non di simultaneità.

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

> 🔎 **PANEL — «quando il repository la richiede» non è un trigger: nessuno sa quando è.**
>
> Un requisito condizionale la cui condizione non è scritta si applica sempre o mai, a seconda di chi legge.
> Per questo filone la condizione è deducibile e vale la pena inciderla:
>
> > **Richiede verifica di mutazione ogni test che protegge un’invariante che il codice potrebbe violare
> > silenziosamente** — cioè tutti i `RejectsX` del validator di #620, e il determinismo del bake di #621.
>
> Il criterio operativo è quello che il §7 già enuncia bene: *allenta una regola, deve cadere esattamente
> il test che la protegge*. Se allentando la regola non cade **nessun** test, la regola vive in una
> convenzione — che è precisamente ciò che #620 esiste per eliminare. Se ne cadono **cinque**, i test non
> sono indipendenti e il referto dirà poco.
>
> Precedente da imitare: `RT-FEAT-BOT-FAIRNESS`, la cui proprietà «la tiene solo la verifica di mutazione
> su `HiddenEnemyFairness`» — cioè un caso dove la mutazione **è** il gate, non un extra.

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

> 🔎 **PANEL — due commit di questa sequenza sono da correggere.**
>
> | Commit | Problema | Correzione |
> |---|---|---|
> | `test(map): add geometry grammar fixtures` | Non dice dove, e le fixture esistono già | **Estendere** `Source/RefactorTactics/Tests/RTOccupancyFixtures.h`, che il registry destina già a #620/#621 |
> | `feat(map): add edge center/orientation helper` | 🔴 **Da eliminare.** L’helper esiste: `EdgeMidpointWorld` (`RTHexLibrary.h:90`) e `EdgeRotation` (`:100`) | Se serve qualcosa, è un commit che *riusa*, non che aggiunge |
>
> Il secondo è l’esempio più concreto del rischio che questo intero documento esiste per prevenire: una
> sequenza di commit che **pianifica** la duplicazione di una funzione già presente e già testata.
> Chi esegue la sequenza alla lettera, senza fare il grep di §5.2, la scrive due volte.

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

> **rendere coerente e produttivo il Level Designer workflow esistente, chiudendo il prossimo gap reale senza duplicare il sistema.**

La qualità di questo lavoro si misura soprattutto da ciò che NON viene duplicato.
