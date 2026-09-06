# AGENTS.md — RefactorTactics

Contratto operativo condiviso per coding agent nel repository.

Obiettivo: modifiche **piccole, verificabili, coerenti con gli owner correnti e con la milestone attiva**.

> Numeri di test, SHA, issue aperte, checkpoint e altri dati volatili si misurano sul branch corrente. Non copiarli qui.

## 1. Progetto in 30 secondi

**RefactorTactics** è un tattico competitivo a turni simultanei in **Unreal Engine 5.8.1**.

- **v0.1:** 2v2 offline contro bot.
- **Standard:** 3v3 — D-256.
- **2v2:** Skirmish / vertical slice.
- **4v4+:** Operations / stress e scala.
- **Mappa:** grafo esagonale multilivello.
- **Coordinate:** `FRTCellId{X=q, Y=r, Layer}`.
- **Roster v0.1:** Gadget · Phase · Riktor · Wraith.
- **Ability system v0.1:** `UPrimaryDataAsset`.
- **GAS:** fuori scope v0.1.

Loop:

`Planning → Prep → Dash → Blast → Move → Cleanup`

Il Move normale resta l'ultima fase volontaria.

Azioni universali:

`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`

## 2. Fonti da leggere

Non lavorare dalla memoria del progetto.

Quando pertinenti:

1. `docs/product/piano-canonico-mvp.md`
2. `docs/decisions/RT_PDR_00_Decision_Log.md`
3. ADR applicabili
4. `docs/DOC_CONFLICT_MATRIX.md`
5. `docs/OPEN_DECISIONS.md`
6. `docs/roadmap/roadmap-checkpoint.md`
7. `docs/roadmap/roadmap-v0.1.md`
8. issue/task corrente
9. spec owner
10. cataloghi applicabili
11. codice
12. test

### Non sono source of truth per default

- `docs/research/`
- `docs/archive/`
- PDF/export
- handoff
- audit
- vecchi snapshot
- conteggi copiati

Un `.pdf` è reference/export/audit artifact, non owner normativo: **D-009**.

Il vecchio `feature-registry.yaml`, shortlists e viste derivate sono stati rimossi con **D-181**. Non ricrearli implicitamente.

Se due fonti normative sono incompatibili:

**non scegliere per plausibilità.**

Individua l'owner, segnala la deriva e aggiorna conflict matrix/owner quando il task lo richiede.

## 3. Invarianti

1. **La simulazione decide, la presentazione mostra.**
2. Snapshot + regole/versione + seed + decisioni registrate devono poter riprodurre lo stesso risultato.
3. Ordinamenti competitivi sempre espliciti.
4. Non dipendere dall'ordine di `TMap`/`TSet`.
5. Non usare frame rate o arrivo dei pacchetti per decidere un esito.
6. Niente `DeltaTime`, `Delay`, timeline o callback di animazione nel resolver competitivo.
7. Posizione gameplay = `FRTCellId`.
8. World transform = presentazione.
9. Un solo substrato spaziale: niente seconda griglia quadrata.
10. Non creare un Actor per ogni cella/esagono.
11. Pathfinding, LOS, targeting e traiettorie sono servizi distinti.
12. C++ definisce cosa è possibile.
13. Data Asset/Blueprint configurano varianti e presentazione.
14. ID, costi, priorità, duration, reason code e formati serializzati competitivi sono espliciti/versionati.
15. Un'abilità ha un solo owner.
16. Niente `PairBonus`, `ComboAbility` o branch core `if HeroA && HeroB` per sinergie sistemiche.
17. Scenari, fazioni e Wiki non sono una seconda fonte di numeri competitivi.

## 4. Authority e privacy

Il client propone. L'autorità valida e applica.

- Il server può conoscere lo snapshot completo.
- Il client riceve solo informazioni autorizzate.
- Non replicare globalmente intenti privati per poi nasconderli in UI.
- Nessun planning avversario in `GameState`.
- Nessun planning avversario in `PlayerState`.
- Nessun planning avversario su Actor AlwaysRelevant.
- Nessun planning avversario nel log pubblico prima del momento autorizzato.
- UI e warning usano stato pubblico, Team Knowledge e intenti della propria squadra.
- Overwatch e reazioni non leggono intenti privati o trigger futuri.

## 5. Movimento e reazioni

Famiglie di movimento:

### Traversal

Percorre lo spazio.

`Move · Dash · Forced`

Produce una sequenza di celle attraversate.

Ogni cella può generare fatti:

- hazard;
- trigger;
- attraversamento bordo;
- occupazione;
- interazioni.

### Transfer

Cambia posizione senza percorrere celle intermedie.

L'owner gameplay corrente stabilisce quali azioni appartengono alla famiglia.

### Non confondere

`Reaction` è una causa, non una famiglia di movimento.

`Portal` è topologia del grafo.

### Pin

- Sprint = profilo Move.
- Sprint ≠ Dash.
- Overwatch è universale.
- Overwatch compete con l'azione offensiva principale.
- Delayed/Predictive Action viene scelta nel Planning.
- Fast Action continua una propria azione.
- Fast Reaction deriva da un trigger esterno.
- Modello live: `Opportunity → Commit`.
- Fast Reaction baseline: **3,0 s**.
- Timeout: **HOLD**.
- Thin slice Predictive v0.1: `Hero.Wraith.InterceptShot`.
- High Ground non dà automaticamente `+Damage` o `+VisionRange`.

## 6. Repository

| Percorso | Responsabilità |
|---|---|
| `Source/RefactorTactics/` | Runtime C++ e Automation Tests |
| `Source/RefactorTacticsEditor/` | Tooling Editor-only |
| `Plugins/RTDeveloperTools/` | Developer tooling |
| `Content/RT/` | Asset proprietari `/Game/RT/` |
| `Scenarios/` | Scenario Harness JSON |
| `Config/` | Config Unreal |
| `docs/` | Canone e documentazione |
| `tools/` | Validatori e generatori |
| `scripts/rt-suite.ps1` | Suite Unreal locale |

Mappa dettagliata:

`docs/technical/architecture/architettura-codice.md`

## 7. Asset Unreal

- Prefissi C++: `RT` / `URT`.
- Asset proprietari: `/Game/RT/`.
- Struttura Content: feature-first.
- Non modificare `.uasset`/`.umap` a mano.
- Non spostare asset Unreal da Explorer/filesystem.
- Usare Content Browser.
- Quando serve creare, modificare, analizzare o validare asset, mappe, Blueprint o stato Editor-only, usare preferibilmente l'Unreal/Epic MCP disponibile invece di manipolare i binari dal filesystem.
- Avviare Unreal Editor solo quando il task lo richiede realmente.
- Il workflow che avvia l'Editor ne possiede il lifecycle: al termine deve salvare solo le modifiche intenzionali, terminare eventuale PIE/scenario attivo e chiudere l'Editor.
- Su errore o validazione fallita, l'Editor aperto dal workflow va comunque chiuso dopo aver preservato log e diagnostica utili.
- Non lasciare istanze Editor aperte "per comodità" tra task indipendenti.
- Non chiudere o terminare un'istanza preesistente posseduta da un altro utente/processo salvo che il workflow attivo abbia una policy esplicita di ownership esclusiva.
- Dopo rename/spostamenti: Fix Up Redirectors.
- I binari Unreal non sono mergeabili.
- Un asset binario viene modificato da un solo lavoro per volta.
- Rispettare Binary Asset Lease e write-set.
- Non sovrascrivere lavoro non correlato.
- Il repository non usa Git LFS.
- Le eccezioni di asset tracciati seguono la policy corrente del repository.
- Non versionare `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/` o segreti.
- Non editare viste generate: modifica sorgente/generatore e rigenera.

Owner:

`docs/technical/tooling/convenzioni-contenuti-ue.md`

## 8. Protocollo di lavoro

### Prima

Annota:

**obiettivo · branch/HEAD · working tree · owner · assunzioni · write-set · approccio · rischi · test**

Preflight:

```bash
git status
git branch --show-current
git fetch --prune origin
git rev-parse HEAD
git rev-parse origin/main
```

Poi:

- cerca prima di creare;
- verifica i path reali;
- verifica le API UE 5.8.1;
- riusa classi/helper/spec/test esistenti;
- non espandere lo scope;
- evita refactor collaterali;
- controlla riferimenti prima di cancellare o spostare.

### Durante

- Mantieni piccolo il write-set.
- Non creare una seconda source of truth.
- Non fare search/replace ciechi sugli Stable ID.
- Per formati serializzati prevedi migrazione/compatibilità quando richiesta.
- Una feature presentation-only non modifica stato competitivo senza requisito esplicito.
- Se un gate non è stato eseguito: **NOT RUN**.

### Dopo

Riporta:

**risultato · file · decisioni · build/test · verifiche manuali · NOT RUN · limiti · prossimo passo**

Non dichiarare:

- completo;
- production ready;
- sicuro;
- deterministico;
- verificato;

senza evidenza.

### Un work order esterno

Un kit, brief, mandato o «roadmap» che arriva da fuori è un **ingresso**, non un owner.

Entra il **contenuto**: candidate tecniche, difetti nominati, domande aperte.

Non entra il **preambolo di processo**. Ricognizione, anti-duplicazione, priorità, label, milestone e
formato del report sono già scritti qui e in [`CLAUDE.md`](CLAUDE.md). Un kit che li riscrive non li
sostituisce, e non li emenda.

Se il kit e il contratto divergono, vince il contratto.

Prima di eseguirlo:

- misura le sue premesse: decadono in ore, e la più vistosa è spesso già chiusa;
- cerca l'owner di ogni voce prima di crearne una;
- non assegnare `D-nnn`, `Enn` o altri contatori condivisi dal kit;
- non creare le milestone, le label o le epic che nomina senza verificare la tassonomia esistente.

Un kit senza sha e senza data non porta una misura: porta un'opinione datata ignoto.

Dopo:

- il referto va in `docs/roadmap/plans/`, e cita lo sha su cui ha misurato;
- ciò che il kit chiedeva e non è stato fatto si dichiara, col motivo;
- il kit stesso non si versiona: è una consegna effimera, e il referto è l'unico posto in cui resta
  citabile.

## 9. Build e test

Non esiste CI automatica per scelta corrente.

Non introdurre CI, package manager o nuovi build step senza una decisione esplicita.

### Suite Unreal

Da PowerShell:

```powershell
./scripts/rt-suite.ps1
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
./scripts/rt-suite.ps1 -WaitMinutes 40
```

Una misura è valida soltanto se osserva lo stesso:

- `HEAD`;
- working tree;
- binario;
- stato del motore;

dall'inizio alla fine.

Se cambiano:

**NON VALIDA**.

Non equivale a verde.

Non sostituire `-WaitMinutes` con watcher scritti a mano.

Dopo una lunga attesa, ricompila prima di registrare il risultato: il DLL presente sul disco potrebbe provenire da un'altra sessione.

Prima del merge verifica che il gate appartenga al commit che stai mergiando.

### Build Editor

Con Unreal Editor chiuso:

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -project="<repo>/RefactorTactics.uproject" -waitmutex
```

### Tooling locale

Controlla sempre il contenuto corrente di `tools/`.

Controlli noti:

```powershell
node tools/radar/generate.ts --check
node tools/radar/wiki-alt.ts --wiki-root <clone> --check
node tools/radar/doc-links.ts --check
node tools/radar/catalog-code.ts
node tools/radar/doc-tables.ts --check
node tools/radar/issue-refs.ts --check
node tools/radar/scenario-notes.ts --check
node tools/asset-refs/check.ts
node tools/asset-provenance/check.ts
python tools/architettura/misure-strutturali.py --check   # solo se la PR tocca Turn/RTTurnManager.*

cd tools/radar
node --test

cd ../asset-provenance
node --test
```

Ogni tool dichiara nel docstring **cosa non copre**.

⛔ `tools/asset-provenance/check.ts` verifica che ogni asset abbia una **riga** nel registro di
provenienza ([`docs/technical/asset-licenze.md`](docs/technical/asset-licenze.md)), non che la licenza
sia rispettata. Un verde significa **«registrato»**, mai «consentito»: nessun controllo automatico puo'
leggere un EULA. Guarda due popolazioni — cio' che e' versionato sotto `Content/` e `tools/`, e cio' che
i package versionati **referenziano** senza che il repository lo contenga — perche' la prima da sola
sarebbe cieca sui ~15,8 GB di pack che stanno fuori dal repository per scelta.

⛔ `tools/mutation/costanti-combattimento.py` **non e' fra i controlli noti**, e di proposito. Modifica un sorgente e occupa il motore per **un build completo piu' una suite intera per ogni costante**, piu' una baseline: con le 11 di `RTCombatLibrary.h` sono **ore**, e le direzioni di mutazione da misurare sono **due** (`+3` e `-3` danno risposte diverse — vedi il docstring). Mentre gira, ogni altra misura in parallelo e' NON VALIDA. Si lancia per rispondere alla domanda che `#2118` ha posto — *quali costanti si possono cambiare senza che niente diventi rosso* — non a ogni PR.

⚠️ E se non stampa `AUDIT COMPLETO`, **ricostruire prima di qualunque altra misura**: un'interruzione lascia mutato anche il binario, che e' la meta' che `rt-suite` non sa vedere.

⚠️ `scenario-notes.ts` confronta i numeri citati nella **prosa** di uno scenario con ciò che il file
stesso asserisce — è la deriva che `#1904` ha misurato propagarsi nei documenti a valle, e che `#2049`
ha ripulito. **Ordina, non decide**: ogni riga segnalata va letta, e un verde non è una prova di
assenza. Le tre cose che non vede sono nel suo docstring.

Un verde dimostra soltanto ciò che quel tool misura.

### Editor / PIE tramite MCP

Quando il comportamento modificato è osservabile o verificabile in Unreal Editor e l'ambiente lo consente, usare l'Unreal/Epic MCP per eseguire la verifica più piccola e pertinente.

Usi tipici:

- avvio del progetto/editor quando necessario;
- apertura e ispezione di mappe e asset;
- creazione/modifica di asset supportati dal MCP;
- verifica Blueprint/editor-facing;
- PIE e scenari quando aggiungono evidenza rispetto ai soli Automation Test;
- raccolta di log/evidenze;
- chiusura dell'Editor al termine.

PIE non sostituisce build, Automation Test o Scenario Harness quando questi sono richiesti.

Se PIE/MCP non può essere eseguito per limiti dell'ambiente o per ownership concorrente, riportare `NOT RUN` con il motivo invece di simulare il risultato.

Per ogni uso Editor/MCP:

1. verificare se l'Editor serve davvero;
2. verificare ownership/processi concorrenti;
3. avviare o connettersi tramite tooling supportato;
4. eseguire il test/asset operation più piccolo utile;
5. salvare solo le modifiche intenzionali;
6. fermare PIE/scenari;
7. chiudere l'Editor avviato dal workflow;
8. confermare che il processo sia terminato.

### Authoring e acceptance

Non sono la stessa apertura, e la seconda non vale dentro la prima.

**Authoring**: creare o modificare `.uasset`, `.umap`, Data Asset, Blueprint, montage, posa in mappa. Può
precedere l'implementazione — un asset è spesso un prerequisito, non una verifica.

**Acceptance**: giudicare la feature sul risultato consolidato.

Se la sessione ha scritto asset binari, il giudizio non vale nel processo che li ha scritti:

**salva → chiudi l'Editor → *(build/suite se il write-set tocca `Source/`)* → riapri → giudica**

Il build sta nella catena solo quando il work item ha toccato codice: per un write-set di soli asset non
cambia ciò che si sta giudicando, e costa un'ora.

La riapertura è parte dell'oracolo quando si verifica persistenza, serializzazione, riferimenti, startup
map, layout, errori di load, inizializzazione da zero, asset registry o cook. Fuori da questi casi non
serve, e chiedere un restart che nessuna di queste domande richiede costa un'apertura per niente.

Una nuova apertura si giustifica solo se cambia una **precondizione**: asset da salvare, restart pulito
richiesto, processo o configurazione incompatibili. Cambiare mappa, fermare e riavviare PIE, o eseguire un
altro scenario **non** lo sono: si fanno nella stessa apertura. Quali sedute condividano un allestimento è
già dichiarato in `docs/roadmap/editor-sessions.yaml`, campo `shares_setup_with`.

Il verdetto va scritto dove il suo owner lo cerca:

- una voce `PIE-*` in `docs/technical/test-manuali-pie.md`, quando il comportamento è **in gioco**;
- la **issue owner**, quando la verifica sta nell'editor prima del Play;
- un **artifact** versionato, quando la seduta produce un file.

L'assenza in uno dei tre non è un buco se un altro porta il verdetto. Se non lo porta nessuno: `NOT RUN`
con il motivo.

⛔ La scelta non è libera: se la verifica **ha** una voce `PIE-*`, il verdetto va nel registro, che ne
resta l'owner — una issue non lo sostituisce.

### `issue-refs.ts` — l'unico che guarda fuori dal repository

Confronta i percorsi e i comandi citati dalle **issue aperte** con l'albero: chiude il difetto che
`doc-links.ts` dichiara di non coprire, cioè i riferimenti scritti in prosa dove nessun link li rende
verificabili.

Tre cose da sapere prima di usarlo:

- **Segnala il cancellato, non l'assente.** Un percorso mai esistito è un deliverable e non è un
  difetto; uno rimosso è un riferimento morto. La distinzione toglie ~114 falsi positivi.
- **Serve la storia completa.** Su un clone shallow dichiara `NOT RUN` invece di un verde: senza
  `git log --diff-filter=D` nessun percorso risulta rimosso.
- **Senza rete dichiara `NOT RUN` ed esce 0.** Non blocca chi lavora offline e non finge un verde.

Una issue il cui *oggetto* è la rimozione si esenta dal proprio corpo, **col motivo obbligatorio**:

```html
<!-- issue-refs: ignora — perché questa issue cita di proposito percorsi rimossi -->
```

Il gate stampa le esenzioni a ogni esecuzione.

⚠️ **Si lancia a mano, come gli altri radar** — vedi §9: *«non introdurre CI senza una decisione
esplicita»*. Il difetto che chiude però non nasce da un commit, **nasce dal tempo che passa** fra la
rimozione di un percorso e la issue che nessuno riapre.

Per questo `rt-suite.ps1` lo esegue **come promemoria** dopo un verdetto `VALIDA`, e lo si disattiva con
`-NoIssueRefs`:

🔴 **Non concorre al verdetto della suite, ed è una scelta, non una svista.** Legge GitHub, che cambia
mentre la suite gira: in una run da quaranta minuti può passare all'avvio e fallire alla fine. Farlo
entrare nelle invarianti di §9 renderebbe `NON VALIDA` una misura sana per una issue che ha modificato
qualcun altro — cioè il difetto che `rt-suite.ps1` esiste per impedire. Stampa, e l'esito resta quello
dei test.

Gli output generati dei radar non si editano a mano.

## 10. Definition of Done

Quando applicabile:

- Game compila.
- Editor compila.
- Test mirati passano.
- Regressione pertinente passa.
- La misura è valida.
- Determinismo preservato.
- Authority preservata.
- Privacy preservata.
- TurnLog/reason code spiegano l'esito.
- Owner documentale aggiornato.
- Nessun secret introdotto.
- Nessun output locale indesiderato.
- PIE verificato quando richiesto.
- Packaged verificato quando richiesto.
- Per modifiche editor-facing/asset-facing, MCP/Editor usato quando disponibile e pertinente.
- Nessun Unreal Editor avviato dal workflow resta aperto a fine task.

## 11. Lavoro parallelo e figure operative

Il repository viene modificato da più sessioni.

Non assumere stabili:

- `HEAD`;
- `origin/main`;
- working tree;
- DLL;
- issue;
- ID condivisi.

Un worktree separato non elimina il mutex globale Unreal/Live Coding.

Prima del merge rimisura.

### Le tre figure

Il lavoro concorrente si organizza in **tre figure**. Sono tre le figure, non i terminali: le istanze DEV possono essere N, e nulla obbliga ad aprirne esattamente tre.

| Figura | Possiede | Non possiede |
|---|---|---|
| **DEV** | codice, authoring dei test, review, Git/GitHub, tooling statico e headless | Unreal: non avvia Editor, `rt-suite`, packaging o build che monopolizzano il motore |
| **EDITOR** | Unreal Editor, PIE, MCP/editor automation, `.uasset`/`.umap`, Blueprint, authoring asset, acceptance visuale | suite e build concorrenti; il verdetto sui sistemi che può soltanto osservare |
| **VALIDATION** | build, Automation Test mirati, Scenario Harness, suite completa, packaged | i binari; l'approvazione del proprio fix |

`DEV-LEAD`, `DEV-MAIN` e `DEV-TEST` sono **funzioni DEV dentro una wave**, non figure aggiuntive: valgono per intero i limiti DEV, incluso il divieto di occupare il motore.

- `DEV-LEAD` — la singola istanza che per una wave possiede l'integrazione ed emette l'handoff di ingresso;
- `DEV-MAIN` — implementa dentro lo scope assegnato;
- `DEV-TEST` — scrive test, scenari e validator, e dichiara i comandi che VALIDATION eseguirà.

### Workspace, figura e branch sono tre cose diverse

Si confondono facilmente, e ogni confusione ha gia' prodotto un difetto.

| Cosa | Valori | Dove vive |
|---|---|---|
| **Figura** della sessione | `DEV` · `EDITOR` · `VALIDATION` | `RT_TERMINAL_ROLE` |
| **Identita'** del workspace | `MAIN` · `DEV` · `TECHNICAL_DESIGNER` | registro per macchina, non il nome della cartella |
| **Branch** git | qualunque | `git rev-parse` |

`MAIN` **non** e' il branch `main`. E' il checkout che ospita l'unico bridge MCP della macchina. L'authoring asset avviene la', e su un **branch di task** — mai su `main`.

Ogni directory apre tutte e tre le figure. Nessun checkout e' vincolato a un ruolo operativo.

### Il motore e' una risorsa di macchina

Unreal e' **uno** e lo condividono tutti i checkout. Da cui:

- il permesso di occuparlo non puo' vivere dentro un checkout. Uno stato per-root non descrive una risorsa per-macchina, e il finding `parsecell-arity/1-F13` lo ha misurato: con sei checkout attivi, l'unico che dichiarava `VALIDATION` era quello che **non** stava usando il motore;
- il lease si acquisisce **just-in-time**, immediatamente prima di Editor, PIE, build, commandlet o di una chiamata MCP che richieda l'Editor vivo. Aprire un terminale o avviare un agente non lo acquisisce;
- il lease **non e' preemptive**: chi trova la risorsa presa attende, e nessuna sessione termina quella attiva;
- un processo motore vivo che nessun lease rivendica **blocca**: concedere sarebbe promettere un'esclusivita' che non c'e';
- il rilascio e' una **dichiarazione** che la risorsa e' libera. Se un processo avviato dalla sessione e' ancora vivo, non si rilascia.

### Authoring asset: solo dal workspace MAIN

Una chiamata che crea, modifica, rinomina, sposta, cancella, importa o salva un asset Unreal via MCP e' autorizzata solo se valgono **tutte** queste condizioni:

```text
figura            = EDITOR
workspace         = MAIN, verificato sul registro di macchina
branch            = branch di task, diverso da main
task id           presente
write-set asset   dichiarato
lease             vivo, posseduto, per l'operazione giusta
contesto          progetto, Editor ed endpoint coincidono col lease
```

Una figura EDITOR in un workspace `DEV` o `TECHNICAL_DESIGNER` resta legittima: prepara, ispeziona, usa capacita' realmente read-only. **Non muta asset.**

La ragione e' misurabile, non formale: il bridge e' uno solo e vive in MAIN, quindi una sessione che lo usa da un altro checkout **muta gli asset di MAIN** mentre legge il `git status` del proprio.

VALIDATION puo' usare il motore per misurare. Non ripara asset durante il sign-off: un difetto torna a EDITOR, e la nuova evidenza si produce su input rimisurato.

### Una figura per sessione


Una sessione assume **una sola figura**, e la dichiara all'avvio.

Il ruolo **non si deduce dal nome della directory**. `Main`, `Dev`, `Technical Designer` e ogni altro checkout sono luoghi, non figure: la stessa directory ospita figure diverse in momenti diversi, e una cartella chiamata `Dev` non rende DEV la sessione che ci lavora.

Se il ruolo non è dichiarato, non indovinarlo.

### Cosa isola una directory, e cosa no

| Configurazione | Isola | Non isola |
|---|---|---|
| Più sessioni nella **stessa directory** | niente | working tree, index e `HEAD` sono condivisi: il parallelismo è operativo, non Git |
| Sessioni in **directory separate** | working tree, index, `HEAD` | Unreal, Live Coding, DDC, CPU e disco: le risorse macchina restano una sola |

Da cui due conseguenze:

- nella stessa directory `git add -A`, `git commit -am`, `reset`, `restore`, `clean`, `switch` e `pull --rebase` inglobano o distruggono il lavoro non integrato di un'altra istanza. Staging per path espliciti;
- in directory separate due misure Unreal non diventano parallele: diventano una coda.

`git status` non risponde alla domanda «questo file è mio».

### Coordinamento

Le figure si coordinano su **artefatti**, non su copie locali né sul contesto di una conversazione:

- il **branch** e il **parent branch** reali;
- gli **SHA**: il commit ereditato in ingresso e quello prodotto in uscita;
- gli **handoff persistiti** su file.

Un handoff che vive solo in chat non esiste per la figura successiva, e un'evidenza descritta a parole non è riverificabile.

### Esclusione reciproca su Unreal

EDITOR e VALIDATION si escludono a vicenda **quando occupano il motore**.

Un Editor aperto e una suite non convivono: mentre uno dei due tiene Unreal, l'altro attende. Aprire un secondo terminale non produce una seconda istanza del motore.

Le sessioni DEV continuano a lavorare, purché non lo occupino.

### Catena canonica

```text
DEV-LEAD → EDITOR → VALIDATION
```

Tre punti fissi. Le altre istanze DEV contribuiscono a monte di `DEV-LEAD` e non sono punti della catena.

Nessuna figura firma il proprio lavoro:

- EDITOR **non** emette il verdetto sui sistemi che può soltanto osservare — privacy, determinismo, autorità di rete, replay, performance, e gli altri che il contratto elenca. Constatare che un dato non compare nella UI avversaria non prova che non sia sul client: quella prova appartiene a VALIDATION;
- VALIDATION **non** ripara il codice di produzione e poi approva sé stesso. Un difetto torna al suo owner con la correzione richiesta e con la regressione che deve esistere prima della richiusura.

### Validation Window preliminare

Una figura VALIDATION può aprire una finestra **prima** che la catena sia completa, per misurare presto ciò che è già misurabile.

È consentito e utile. **Non è il sign-off finale**: misura un commit che non è quello consegnato, e un verde su una base precedente non copre ciò che è stato scritto dopo. Il sign-off resta il passaggio VALIDATION a valle di EDITOR, sul commit realmente consegnato.

È questa la finestra che compare come passata anticipata nel flusso operativo di [`docs/rt-three-terminals/README.md`](docs/rt-three-terminals/README.md). Non contraddice la catena: la anticipa in un punto, e non la chiude.

### Handoff minimo

Un passaggio di consegne porta almeno:

```text
FEATURE · BRANCH · PARENT_BRANCH
BASE_SHA · PRODUCED_SHA
WRITE_SET · BINARY_ASSETS
STATUS
```

Fail-closed, non best-effort:

- campo vuoto, placeholder non risolto o input non risolvibile ⇒ handoff bloccato, **prima** di leggere il repository e prima di aprire l'Editor. Un placeholder risolto per inferenza è un input inventato;
- `PARENT_BRANCH` non ha default: non è `main` per assunzione;
- un handoff in ingresso bloccato blocca la figura successiva. Non si valida sopra una base che il ruolo a monte ha dichiarato inaffidabile;
- un verdetto senza il campo che lo prova non è un verdetto;
- ciò che non è stato eseguito resta `NOT RUN`, col motivo.

### Dove vive il dettaglio

Verdetti tipizzati, matrice dei sistemi per ruolo, scoping dal write-set, schema completo dell'handoff, persistenza e defect policy hanno un owner unico:

[`docs/rt-three-terminals/prompts/RT3_CONTRACT.md`](docs/rt-three-terminals/prompts/RT3_CONTRACT.md)

Non riscriverli qui. Se una regola di quel contratto contraddice questo file o [`CLAUDE.md`](CLAUDE.md), vince il documento di repository.

Prompt di figura, prompt di wave, installazione e regole di concorrenza operative:

[`docs/rt-three-terminals/README.md`](docs/rt-three-terminals/README.md)

## 12. Git

Branch focalizzati:

- `feat/`
- `fix/`
- `refactor/`
- `docs/`
- `test/`

Usare Conventional Commits.

Controllare sempre:

```bash
git status
git diff
```

prima del commit.

Non confondere:

**file modificato**

con

**file verificato**.

### Chiudere una issue

`fix(605)` **non chiude** la issue 605.

GitHub lo legge come uno **scope Conventional Commits**, non come un riferimento. La forma che chiude e' una
parola chiave seguita da `#N`:

```
Closes #605
```

Le parole riconosciute sono `close`/`closes`/`closed`, `fix`/`fixes`/`fixed`,
`resolve`/`resolves`/`resolved`. `fix(605)` non e' nessuna di queste: manca il `#`, e la parentesi ne fa
uno scope.

**Dove va**: nel **corpo della PR**, in cima. Non nel messaggio di commit.

Il corpo della PR e' il canale che GitHub processa al merge, ed e' l'unico che un agente controlla davvero:
`gh pr create --body-file` scrive li'.

⛔ **`.github/pull_request_template.md` non basta.** Il template si applica solo alle PR aperte
dall'interfaccia web: `gh pr create` con `--body` o `--body-file` lo **sostituisce**, e non avvisa. Chi apre
PR da riga di comando — cioe' ogni agente — deve scrivere la riga a mano.

⚠️ **Se la base della PR non e' il branch di default, `Closes` non chiude niente al merge.** La issue si
chiudera' solo quando quel branch arrivera' su `main`. Una PR verso un branch padre intermedio chiude la
propria issue **a mano**, oppure lo dichiara.

**Perche' questa sezione esiste.** Misurato il 2026-09-02: **57** issue aperte avevano almeno un commit
`fix()`/`feat()` mergiato su `main`, e nessuna si era chiusa da sola. Fra queste, `#1473`, `#605`, `#75` e
`#61` avevano il lavoro **finito**: la loro correzione era su `main` da giorni o settimane, e restavano
aperte perche' il messaggio diceva `fix(1473)` invece di `fixes #1473`.

⚠️ **Non e' un difetto di disciplina, ed e' per questo che vale una regola scritta**: `fix(605)` *sembra* un
riferimento. Il triage completo e' nel commento di chiusura di
[`#1473`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1473).

### ID condivisi

`D-nnn`, Epic `Enn` e altri contatori non si assegnano dalla memoria.

Prima di assegnare o mergiare:

1. `git fetch --prune origin`
2. controlla `main`;
3. controlla ref remoti;
4. controlla PR;
5. controlla issue;
6. riverifica immediatamente prima del merge.

Un branch remoto può rivendicare un ID anche senza PR aperta.

In caso di collisione correggi i riferimenti puntuali.

Non fare search/replace globale.

## 13. Lingua

Documentazione e comunicazione:

**italiano**.

Codice, identificatori e API:

**inglese**.

Tutoring C++/UE su richiesta, non come default.