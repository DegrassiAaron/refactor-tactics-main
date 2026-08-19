# RefactorTactics — Handoff consolidato per Claude
## Project Control Center · Roadmap/Feature/Scenario/Editor Maps · GitHub · Wiki
### Baseline verificata: 2026-08-17
### Repository: `DegrassiAaron/refactor-tactics-main`
### Branch di riferimento: `main`
### HEAD osservato durante questo handoff: `bce32a288dc8a0e18c5b290d4f079621224ffa10`

> **IMPORTANTE**
>
> Questo handoff sostituisce i precedenti handoff sul Project Control Center e sulla sua integrazione Wiki.
> Il repository si muove rapidamente: il commit sopra è una baseline osservata, non un dato eterno.
> Prima di modificare file, issue o documentazione, rimisurare `origin/main`.

---

# 0. Obiettivo

Mantenere e sviluppare un unico **Project Control Center** per RefactorTactics capace di mostrare e collegare:

1. **Roadmap** — epic, milestone, checkpoint, issue e ordine del lavoro;
2. **Feature Map** — feature, gate, stato, dipendenze e riferimenti;
3. **Scenario Map** — scenari demo/test e feature che dimostrano;
4. **Editor Map** — attività che richiedono intervento umano in Unreal Editor;
5. **My Editor Queue** — cosa può/deve fare ora l'autore nell'Editor;
6. **Execution Map** — cosa blocca cosa;
7. **Diagnostica** — riferimenti rotti, incoerenze e warning;
8. **Wiki Developer Zone** — versione leggibile senza checkout delle informazioni di progetto utili.

Regola principale:

> **Le viste non sono nuove fonti di verità.**
> Ogni stato nasce nel proprio owner canonico, viene derivato una volta e poi visualizzato.

---

# 1. Stato reale: non ricostruire ciò che esiste

Il Project Control Center è già implementato:

```text
docs/control-center/
├── index.html
├── graph.js
├── graph.test.mjs
├── package.json
└── README.md
```

Owner del disegno:

```text
docs/roadmap/plans/project-control-center-spec.md
```

Feature Registry canonico:

```text
docs/roadmap/feature-registry.yaml
```

Artefatti generati consumati dalla dashboard:

```text
docs/roadmap/feature-registry.json
docs/roadmap/project-graph.json
```

Tool owner delle derivazioni:

```text
scripts/feature_registry.py
```

Altri owner importanti:

```text
docs/roadmap/editor-sessions.yaml
docs/roadmap/execution-graph.yaml
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/technical/test-manuali-pie.md
Scenarios/
```

Le shortlist generate esistono:

```text
docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/roadmap/editormap.shortlist.md
```

Il Control Center mostra già:

- Overview;
- Roadmap;
- Feature Map;
- Execution Map;
- Scenario Map;
- Editor Map;
- My Editor Queue;
- Diagnostica;
- ricerca;
- filtri;
- detail drawer;
- relazioni inverse;
- link GitHub/Wiki derivati;
- controllo di staleness;
- test Node.

**Non creare un secondo dashboard, un secondo registry, un secondo Editor Map o un secondo grafo.**

---

# 2. Architettura canonica

```text
feature-registry.yaml
        |
        | feature_registry.py generate
        v
feature-registry.json
        |
        +--------------------+
                             |
altri owner canonici        |
        |                    |
        v                    v
project-graph.json <---------+
        |
        +-------------------+
        |                   |
        v                   v
Control Center          Wiki generate/deploy
```

Owner e viste:

```text
feature-registry.yaml         -> feature-registry.json
editor-sessions.yaml          -> project-graph.json
execution-graph.yaml          -> project-graph.json
Scenarios/                    -> project-graph.json
roadmap-v0.1.md               -> project-graph.json
roadmap-checkpoint.md         -> project-graph.json
test-manuali-pie.md           -> project-graph.json
```

## Regola non negoziabile

> **La dashboard non calcola stato.**
> Se serve un valore che gli artefatti generati non contengono, estendere il generatore Python.
> Non riderivare la stessa regola in JavaScript.

Il JavaScript può derivare:

- URL;
- relazioni inverse;
- indici;
- filtri;
- layout;
- focus grafico.

Non deve derivare:

- `status`;
- gate completion;
- readiness;
- stato milestone;
- stato Editor;
- stato scenario;
- decisioni di release.

---

# 3. Source of truth del Feature Registry

File canonico:

```text
docs/roadmap/feature-registry.yaml
```

Contratto:

```text
feature-registry.yaml = SORGENTE
feature-registry.json = GENERATO
```

Configurazione centrale GitHub già presente:

```yaml
meta:
  project:
    github:
      owner: DegrassiAaron
      repository: refactor-tactics-main
      branch: main
```

Da qui devono essere derivati:

- issue URL;
- PR URL;
- milestone URL;
- file/document URL;
- Wiki URL.

Non introdurre URL completi duplicati dentro ogni feature.

---

# 4. Progresso e status

Non introdurre percentuali manuali.

Modello:

```text
N / M gate
```

con `na` escluso dal denominatore.

Lo `status` è derivato dai gate e il validator deve continuare a verificarne la coerenza.

---

# 5. Editor Map: decisione già chiusa

NON aggiungere `editor_tasks:` al Feature Registry.

L'owner è:

```text
docs/roadmap/editor-sessions.yaml
```

Una seduta Editor usa dati come:

```text
unblocked_by
unblocks
verifies
artifacts
critical
lane
```

e il suo stato viene derivato.

`My Editor Queue` esiste già:

```text
BLOCKING
READY
WAITING
DONE
```

Non aggiungere `status: TODO` scritto a mano.

Principio:

```text
AUTOMATIZZABILE
C++ / script / Claude / test / generator
        |
        v
Issue / track tecnica

MANUALE
Unreal Editor / scelta visuale / asset / verifica a schermo
        |
        v
Editor Session
```

---

# 6. La Wiki: source of truth attuale

Decisione D-076:

> Il clone GitHub Wiki è la fonte unica della prosa pubblicata.

Repository Wiki:

```text
refactor-tactics-main.wiki
```

Clone:

```bash
git clone https://github.com/DegrassiAaron/refactor-tactics-main.wiki.git
```

Il repository principale NON deve tornare a contenere una copia completa della prosa Wiki.

`docs/wiki/` può contenere soltanto artefatti che hanno ancora un owner reale nel repository,
non una seconda copia delle pagine.

---

# 7. `wiki_refs`: contratto corrente

Pagina posseduta dal clone Wiki:

```yaml
wiki_refs:
  - wiki:overwatch
```

Semantica:

```text
wiki:<PageName>
```

`PageName` è il nome della pagina pubblicata, non il path fisico della cartella.

Documento posseduto dal repository e pubblicato anche nella Wiki:

```yaml
wiki_refs:
  - docs/characters/...
```

Non convertire automaticamente una forma nell'altra.

---

# 8. Namespace Wiki

Nel progetto è già stato misurato che:

- una pagina può fisicamente stare in sottocartelle del clone;
- il namespace/URL della Wiki è basato sul nome della pagina;
- i nomi devono restare globalmente non ambigui;
- i link Wiki vanno trattati per nome;
- il case conta;
- rename di pagine può rompere molti link;
- i controlli devono essere case-sensitive anche su Windows.

Per questo `wiki:<PageName>` è intenzionale e stabile anche se il clone viene riorganizzato.

---

# 9. Deploy Wiki esistente

```bash
python scripts/feature_registry.py deploy --wiki-root <path>
python scripts/feature_registry.py deploy --wiki-root <path> --check
python scripts/feature_registry.py deploy --wiki-root <path> --write
```

Semantica:

```text
senza --write  = non scrive
--check        = gate
--write        = modifica esplicita del clone
```

Validazione completa:

```bash
python scripts/feature_registry.py validate --wiki-root <path>
```

Non rendere la scrittura Wiki implicita.

---

# 10. Wiki già generata dal Project Control Center

La Wiki possiede già:

```text
Stato-delle-feature
Stato-del-progetto
```

`Stato-del-progetto` deriva dallo stesso `project-graph.json` del Control Center e rende disponibile,
senza checkout:

- gate di release;
- stato aggregato;
- My Editor Queue;
- execution graph;
- capability.

Quindi la Wiki non deve copiare manualmente il Control Center: deve pubblicare viste derivate dagli stessi artefatti.

---

# 11. Player Wiki vs Developer Zone

Epic corrente:

```text
#422 — Wiki Player-First
```

Separazione:

```text
PLAYER
├── regole
├── esempi
├── scelte
├── counterplay
└── strategia

DEVELOPER ZONE
├── stato progetto
├── stato feature
├── roadmap
├── issue
├── gate
├── scenari QA
├── Editor Queue
├── execution graph
└── tooling
```

Tracking:

```text
#827 — Developer Zone separata   OPEN
#828 — Cleanup/link/verifica     OPEN
```

Non creare una seconda epic Wiki.

Le pagine Player non devono essere riempite con:

```text
RT-FEAT-*
#issue
gate
checkpoint
SHA
simboli C++
diagnostica
```

Questi elementi appartengono alla Developer Zone.

---

# 12. Collegamento Control Center → GitHub/Wiki

`docs/control-center/graph.js` possiede già helper come:

```text
githubBase()
issueUrl()
milestoneUrl()
docUrl()
wikiRefUrl()
```

Ogni card/detail deve riusare gli stessi resolver.

Da una feature si deve poter raggiungere, quando presente:

- issue;
- Wiki;
- owner spec;
- scenario;
- test;
- epic/milestone/checkpoint;
- Editor session collegata;
- dependency;
- `completed_by`.

Un riferimento rotto non deve sparire: deve essere mostrato come rotto.

---

# 13. Collegamento Wiki → GitHub

GitHub specifica che i riferimenti automatici a issue/PR come:

```text
#165
```

non vengono autolinkati nelle Wiki o nei normali file repository.

Quindi una pagina Wiki che deve portare a una issue deve generare un link Markdown esplicito.

Esempio concettuale:

```markdown
[Issue #165](<URL derivato da meta.project.github>)
```

Il numero della issue viene dal registry.

Owner/repository/branch vengono dalla configurazione centrale.

Non hardcodare owner/repository in ogni renderer.

---

# 14. Dove mettere i link GitHub nella Wiki

## Pagine Player

Al massimo:

```text
Stato pubblico
Release
breve nota
[Dettagli tecnici]
```

## Developer Zone

`Stato-delle-feature` può mostrare:

```text
Feature
Status
N/M gate
Release

GitHub
- Issue ...

Roadmap
- Epic
- checkpoint

Scenari
- ...

Test
- ...

Owner specs
- ...

Wiki
- pagina player-facing
```

Questa è la sede giusta per i collegamenti completi.

---

# 15. Blocchi `RT_FEATURE_STATUS`

Meccanismo esistente:

```markdown
<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-... -->
...
<!-- RT_FEATURE_STATUS:END RT-FEAT-... -->
```

Non editarli manualmente.

Se serve una resa diversa:

```text
PLAYER-SAFE
vs
DEVELOPER-RICH
```

farla nello stesso generatore, non con due fonti.

---

# 16. Backlink Wiki → Feature detail

Non creare una pagina Wiki per ogni feature solo per metadata.

Usare principalmente:

```text
Stato-delle-feature
```

come indice Developer.

Se si vuole un deep-link a una sezione, prima verificare realmente come GitHub genera gli anchor.

Non hardcodare una slugification non testata.

Fallback sicuro: link alla pagina `Stato-delle-feature` senza anchor.

---

# 17. GitHub Issue → Wiki: fuori MVP

Dal registry esiste già la relazione:

```text
issue
  -> feature
      -> wiki_refs
```

Il Control Center può navigare in quel verso senza modificare le issue GitHub.

Solo se emerge un bisogno reale, in futuro valutare un blocco managed:

```markdown
<!-- RT_WIKI_LINKS:BEGIN -->
...
<!-- RT_WIKI_LINKS:END -->
```

Vincoli:

- generato;
- idempotente;
- dry-run di default;
- tocca soltanto il blocco;
- mai riscrivere il corpo umano;
- niente rollout massivo senza prova.

Non implementarlo ora salvo decisione esplicita.

---

# 18. Stato GitHub live: decisione nuova di #857

Informazioni come:

- issue open/closed;
- milestone correnti;

cambiano su GitHub senza un commit locale.

Queste informazioni NON devono entrare in `project-graph.json` committato.

Decisione:

```text
docs/roadmap/project-graph.json
    committato
    solo stato derivato da fonti versionate

docs/roadmap/project-graph.live.json
    NON committato
    .gitignore
    stato live proveniente da GitHub
```

Target previsto:

```bash
python scripts/feature_registry.py generate --live
```

deve produrre anche:

```text
project-graph.live.json
```

senza alterare il contratto committato.

Regola:

```text
stato che cambia solo con un commit  -> può essere committato
stato che cambia su GitHub da solo   -> live, non committato
```

Se il file live manca:

- il Control Center continua a funzionare;
- le diagnostiche che dipendono da GitHub devono dire **NON DISPONIBILE**;
- non devono apparire come verdi.

Il limite/troncamento della query GitHub deve essere rilevato e dichiarato.

---

# 19. Roadmap Map: follow-up corrente #857

Issue aperta:

```text
#857 — Roadmap Map
```

Non sostituisce la Execution Map.

```text
Execution Map
    domanda: cosa blocca cosa / in quale ordine posso lavorare?

Roadmap Map
    domanda: cosa c'è in ogni area/corsia per ogni release?
```

Target:

```text
X = corsia tematica
Y = fascia di release
```

Diagnostiche previste:

- fan-in/fan-out;
- registry vs milestone GitHub;
- mismatch release/milestone;
- epic completate troppo presto;
- densità corsia × release;
- copertura `roadmap.epic`.

---

# 20. Roadmap Map in TRE superfici

Decisione esplicita nei commenti di #857:

```text
UNA SORGENTE
project-graph.json
+ project-graph.live.json quando necessario
        |
        +-----------------------------+
        |              |              |
        v              v              v
roadmap-map.svg    Wiki Roadmap Map   Control Center tab
```

Target file:

```text
docs/roadmap/charts/roadmap-map.svg
```

Caratteristiche:

- generato;
- committato;
- testo/diffabile;
- non editato a mano.

Pagina Wiki:

```text
Roadmap Map
```

Target:

```text
SVG in apertura
+
Mermaid navigabile dentro <details>
```

Control Center:

```text
tab Roadmap Map
+
focus
+
slider temporale
+
diagnostiche
```

SVG serve per il layout preciso a corsie × release.
Mermaid resta utile per navigazione e archi cliccabili.

Un test deve assicurare che SVG, Mermaid e Control Center derivino dalla stessa build del graph.

Verifica visuale obbligatoria:

- Mermaid in browser reale;
- SVG guardandolo realmente;
- non soltanto con grep.

---

# 21. Wiki Developer Zone e Roadmap Map

Coordinare:

```text
#827 Developer Zone
#828 Cleanup
#857 Roadmap Map
```

Non creare iniziative sovrapposte.

Target Developer Zone:

```text
Developer Home
│
├── Stato del progetto            [GENERATO]
├── Stato delle feature           [GENERATO]
├── Roadmap Map                   [GENERATO]
├── Percorso di release
├── Scenari e QA                  [DERIVATO]
├── Editor / My Editor Queue      [DERIVATO]
├── Execution / dipendenze        [DERIVATO]
└── documentazione tecnica selezionata
```

---

# 22. Scenario Map nella Wiki

Non creare un nuovo scenario database.

Sorgenti:

```text
Scenarios/
feature-registry.yaml
project-graph.json
```

La Developer Zone può mostrare:

```text
ScenarioId
stato
capability richieste
feature che lo dichiarano
eventuali issue raggiungibili tramite le feature
```

---

# 23. Editor Map nella Wiki

La Wiki non deve duplicare `editor-sessions.yaml`.

Usare la coda derivata:

```text
BLOCKING
READY
WAITING
DONE
```

e collegarla, quando utile, a:

- checkpoint;
- feature;
- issue;
- verifica PIE.

---

# 24. Execution Map nella Wiki

La Wiki non deve imitare l'interazione JavaScript.

Pubblicare una versione derivata:

```text
Bloccato da
Sblocca
Follows
Related
Capability mancanti
```

Owner:

```text
docs/roadmap/execution-graph.yaml
```

Nessuno stato deve nascere nel renderer Wiki.

---

# 25. Chiarimento sulla lettura del file GitHub

Il primo handoff chiedeva alla pagina di leggere direttamente il raw YAML su GitHub.

Questa architettura è stata deliberatamente superata.

La dashboard corrente legge:

```text
feature-registry.json
project-graph.json
```

dal working tree e usa GitHub solo in modo opzionale per informazioni live/freschezza.

Motivi:

- niente parser YAML nel browser;
- niente duplicazione di `derive_status()`;
- niente seconda implementazione dei gate.

Per l'accesso senza checkout si usa la Wiki generata e, dove serve, artefatti statici pubblicabili.

Non riaprire la lettura del raw YAML dal browser senza una decisione nuova.

---

# 26. Tracking GitHub corrente

Verificato aperto alla baseline di questo handoff:

```text
#422  Wiki Player-First                         OPEN
#827  Developer Zone separata                  OPEN
#828  Cleanup/link/verifica Wiki               OPEN
#857  Roadmap Map                              OPEN
```

Prima di creare nuove issue:

1. cercare per tema;
2. verificare questi owner;
3. consolidare invece di duplicare.

---

# 27. Cosa Claude deve fare

## Fase A — riallineamento

```bash
git fetch origin
git status
git rev-parse origin/main
```

Leggere almeno:

```text
docs/control-center/README.md
docs/control-center/graph.js
docs/control-center/graph.test.mjs
docs/roadmap/plans/project-control-center-spec.md
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/project-graph.json
docs/roadmap/editor-sessions.yaml
docs/roadmap/execution-graph.yaml
scripts/feature_registry.py
docs/roadmap/plans/migrazione-wiki-fonte-unica-2026-08-10.md
docs/roadmap/plans/wiki-audit-player-first-2026-08-13.md
```

Leggere anche:

```text
#422
#827
#828
#857 + commenti
```

## Fase B — consolidare documentazione

Marcare come superate o archiviare le affermazioni vecchie:

```text
- Control Center da creare
- Editor Map da creare
- editor_tasks nel Feature Registry
- dashboard legge YAML raw
- docs/wiki è source della Wiki
```

## Fase C — ponte Wiki -> GitHub

Verificare se `Stato-delle-feature` mostra già link espliciti alle issue.

Se manca:

1. estendere il renderer Python;
2. derivare gli URL da `meta.project.github`;
3. lasciare le pagine Player pulite;
4. aggiungere test;
5. verificare il clone.

## Fase D — #857

Prima verificare che non esistano già:

```text
project-graph.live.json
roadmap-map.svg
```

Se ancora mancanti:

1. implementare il live graph non committato;
2. mantenere la Execution Map invariata;
3. produrre Roadmap Map;
4. pubblicarla sulla Wiki;
5. usare la stessa build graph per SVG/Mermaid/tab;
6. aggiungere test.

## Fase E — tracking

Aggiornare le issue esistenti con i risultati reali.
Non aprire una nuova epic "Control Center + Wiki" se #422/#827/#828/#857 coprono il lavoro.

---

# 28. Test minimi

Control Center:

```text
node --test docs/control-center/graph.test.mjs
```

Copertura minima:

```text
issue URL derivato
milestone URL derivato
document URL derivato
wiki:<PageName> risolto
repository wiki_ref risolto
riferimento rotto visibile
feature -> scenario
scenario -> feature
issue -> feature
dependency cycles
filters
staleness
```

Wiki:

```text
wiki ref inesistente
wiki ref case mismatch
pagina generata idempotente
link issue esplicito
nessun bare #123 usato come unico link
Player page senza tracking tecnico
Developer page con tracking tecnico
```

Roadmap Map:

```text
project-graph.live.json ignorato da git
generate senza --live non cambia
generate --live produce live state
troncamento GitHub dichiarato
degrado esplicito senza live
SVG deterministico
Mermaid e SVG dalla stessa build
Control Center dalla stessa build
```

---

# 29. Gate finali

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
python scripts/feature_registry.py wiki --check
python scripts/feature_registry.py deploy --wiki-root <wiki-clone> --check

node --test docs/control-center/graph.test.mjs
```

Quando si modifica la Wiki:

```bash
python scripts/feature_registry.py validate --wiki-root <wiki-clone>
python scripts/feature_registry.py deploy --wiki-root <wiki-clone> --check
```

Solo dopo review:

```bash
python scripts/feature_registry.py deploy --wiki-root <wiki-clone> --write
```

Poi verificare la Wiki pubblicata in browser.

---

# 30. Errori da evitare

❌ Creare una seconda source of truth.

❌ Far calcolare status/gate alla pagina JS.

❌ Far leggere YAML raw al browser e reimplementare le regole.

❌ Inserire `editor_tasks:` nel Feature Registry.

❌ Scrivere status manuali nelle editor sessions.

❌ Ricreare `docs/wiki/` come copia delle pagine pubblicate.

❌ Usare bare `#123` nella Wiki aspettandosi un link.

❌ Hardcodare URL GitHub in ogni renderer.

❌ Mettere tracking tecnico nel percorso Player.

❌ Committare stato live GitHub in `project-graph.json`.

❌ Trattare "dato live non disponibile" come "nessun problema".

❌ Creare una seconda Roadmap Map anziché estendere #857.

❌ Usare Mermaid come unico strumento per un layout che richiede corsie fisse.

❌ Editare manualmente SVG generati.

❌ Considerare una pagina Wiki corretta solo perché il deploy è riuscito.

---

# 31. Definition of Done consolidata

È corretto quando:

- il Feature Registry resta owner delle feature;
- `feature-registry.json` resta generato;
- `project-graph.json` resta il contratto versionato delle viste non-feature;
- lo stato GitHub live non viene committato;
- il Control Center non calcola stato;
- le Editor Session restano owner del lavoro umano;
- la Wiki resta source unica della prosa pubblicata;
- `Stato-del-progetto` e `Stato-delle-feature` restano generate;
- la Developer Zone contiene il tracking tecnico;
- il Player path resta player-first;
- i link GitHub nella Wiki sono espliciti e derivati;
- i `wiki_refs` risolvono contro il clone reale;
- i riferimenti rotti sono visibili;
- Roadmap Map, quando implementata, ha SVG + Wiki + tab Control Center dalla stessa sorgente;
- `validate`, `generate --check`, `shortlist --check`, `deploy --check` sono verdi;
- i test Control Center sono verdi;
- la Wiki è verificata a schermo;
- nessuna informazione di stato è mantenuta manualmente in due posti.

---

# 32. Commit suggeriti

```text
docs(control-center): supersede stale handoff with current architecture
feat(wiki): derive explicit github links in developer feature status
test(wiki): cover wiki github link resolution and player boundary
feat(registry): add non-committed live github graph snapshot
feat(control-center): add roadmap map and live diagnostics
feat(wiki): publish roadmap map from generated graph
test(control-center): pin shared roadmap map data contract
docs(wiki): align developer zone with project control center
```

Usare questi messaggi solo se il diff reale corrisponde.

---

# 33. Passo successivo raccomandato

```text
1. verificare il ponte Wiki -> GitHub nelle pagine Developer generate
2. consolidare #827 / #828 con ciò che esiste già
3. prendere #857 come unico owner della Roadmap Map
4. implementare il live graph NON committato
5. produrre Roadmap Map su SVG + Wiki + Control Center
6. verificare tutto a schermo e aggiornare il tracking
```

Questo mantiene Roadmap, Feature Map, Scenario Map, Editor Map, Execution Map e Wiki come diverse
**viste dello stesso progetto**, non sistemi paralleli.
