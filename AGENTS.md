# AGENTS.md — RefactorTactics

Contratto operativo condiviso per coding agent nel repository.

Obiettivo: modifiche **piccole, verificabili, coerenti con gli owner correnti e con la milestone attiva**.

> Numeri di test, SHA, issue aperte, checkpoint e altri dati volatili si misurano sul branch corrente. Non copiarli qui.
> ⚠️ **E non copiarli nemmeno in ciò che produci**: la regola vale per questo file *e* per referti, corpi di
> issue, commenti e PR. Vedi **§14**.

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

Con Unreal Editor chiuso, da PowerShell:

```powershell
& "<engine>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<repo>/RefactorTactics.uproject" `
    "-ExecCmds=Automation RunTests RefactorTactics;Quit" `
    -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-log=suite.log"
```

Il filtro è il segmento dopo `RunTests`: `RefactorTactics` esegue tutto, `RefactorTactics.Scenario` solo quel gruppo.

Una misura è valida soltanto se osserva lo stesso:

- `HEAD`;
- working tree;
- binario;
- stato del motore;

dall'inizio alla fine.

Se cambiano:

**NON VALIDA**.

Non equivale a verde.

⚠️ **Questa verifica ora è tua.** Fino al 2026-09-08 la faceva `rt-suite.ps1`, che fotografava `HEAD`, l'hash dell'albero e il binario prima e dopo la run, e dichiarava `NON VALIDA` una misura attraversata da un cambiamento. Lo script è stato rimosso; l'invariante no. Prima di registrare un esito, confronta con ciò che valeva alla partenza:

```powershell
git rev-parse HEAD
git diff HEAD                                          # il CONTENUTO dei modificati
git ls-files -o --exclude-standard | Get-FileHash      # e degli untracked
```

🔴 **`git status --porcelain` non basta, ed è il tranello.** Elenca lettere di stato e percorsi: due modifiche *diverse* dello stesso file danno la riga identica ` M pippo.cpp`. Se stai misurando con l'albero già sporco — un gate di mutazione lo è sempre — il porcelain è cieco proprio al caso a cui sei più esposto: un'altra sessione che riscrive un file che risultava già modificato.

🔴 **E per lo stesso motivo gli untracked si HASHANO, non si elencano.** `git ls-files --others` dà i percorsi: se il contenuto di un file non tracciato cambia durante la run — uno scenario di prova, una fixture generata — l'elenco è identico prima e dopo. È lo stesso tranello un livello più in basso, ed è la ragione per cui `tools/mutation/misura.py` hasha ciascun file invece di fidarsi dei nomi.

Misurato con due file di prova: riscrivendone uno da capo, l'elenco dei percorsi resta `a.txt, b.txt` e gli hash cambiano. ⚠️ Niente `-z` nella pipeline: separa i nomi con NUL, che PowerShell passa a `Get-FileHash` come **un unico percorso** inesistente — zero hash prodotti, e il comando che dovrebbe smascherare il tranello ci cade dentro.

⚠️ Il binario resta fuori da tutti e tre: `Binaries/` è gitignorato, quindi un `Build.bat` di un altro checkout non muove né `HEAD` né l'albero. Va confrontato a parte — mtime e dimensione dei DLL dell'editor, come fa `misura.istantanea()`.

Il motore è **uno solo per macchina**: due run in parallelo, o una build lanciata sotto una suite altrui, si distruggono le misure a vicenda. Non c'è più un lease che lo impedisca — accordati prima.

> 🔑 **Cosa si calpesta davvero, misurato il 2026-09-09.** La riga qui sopra è la cautela giusta, ma è più
> larga del fatto — e la differenza conta, perché su questa macchina convivono più cloni
> (`refactor-tactics-main`, `-dev`, `-designer`, più i worktree) e più sessioni che compilano e misurano a
> rotazione.
>
> **Non si calpestano**: una build in un clone e una suite in un altro. `Binaries/` è **per clone** — ogni
> checkout e ogni worktree ha il proprio `Binaries/Win64/UnrealEditor-RefactorTactics.dll` — quindi una
> build non riscrive il modulo che un'altra suite ha caricato, e `-WaitMutex` serializza le invocazioni di
> UBT.
>
> ⚠️ **E regge per una condizione dichiarata, non per fortuna**: il motore è una **installed build**
> (`D:/EpicGames/UE_5.8/Engine/Build/InstalledBuild.txt` esiste), quindi UBT tratta i moduli Engine come
> read-only e un target di progetto non può riscriverli. Misurato: `UnrealEditor.exe` e
> `UnrealEditor-Engine.dll` erano del **2026-07-31** dopo due build di progetto dello stesso giorno.
>
> 🔴 **Si calpestano ancora**, e queste restano vere:
> * due run nello **stesso** clone o worktree — stesso `Binaries/`;
> * un Editor aperto sullo stesso clone: tiene il DLL, e la build fallisce o gli cambia il modulo sotto;
> * **qualunque cosa tocchi l'Engine** — se `InstalledBuild.txt` sparisce, o si compila un target Engine,
>   l'argomento qui sopra decade **per intero**;
> * le misure di **performance**: la contesa di CPU/GPU cambia i tempi anche quando la correttezza tiene.
>   Per un gate di pacing, «il motore è libero» resta un prerequisito.
>
> Come si verifica, prima di dichiarare una misura valida contro una run altrui:
>
> ```powershell
> Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine
> Test-Path "D:/EpicGames/UE_5.8/Engine/Build/InstalledBuild.txt"      # installed build?
> Get-Item "<clone-altrui>/Binaries/Win64/UnrealEditor-RefactorTactics.dll" | Select Length, LastWriteTime
> ```
>
> ⛔ **La `CommandLine` non è un dettaglio**: dice **quale clone** sta girando, ed è l'unica cosa che
> distingue «una suite altrui» da «la mia». Un conteggio di processi non lo dice.

Dopo una lunga attesa, ricompila prima di registrare il risultato: il DLL presente sul disco potrebbe provenire da un altro commit.

Prima del merge verifica che il gate appartenga al commit che stai mergiando.

### Build Editor

Con Unreal Editor chiuso:

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -Project="<repo>/RefactorTactics.uproject" -WaitMutex
```

`-Project` col percorso **virgolettato** e `-WaitMutex` non sono opzionali: senza il secondo, due build concorrenti si sovrascrivono gli oggetti intermedi.

⚠️ Ricompilare mentre un altro checkout ha una suite in corso rende `NON VALIDA` la sua misura, per l'invariante «binario» qui sopra — [#2529](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2529). Il guard che lo impediva è stato rimosso: verifica che nessuno stia misurando.

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
node tools/mcp/check.ts --check                          # solo dove il ponte MCP e' acceso
python tools/architettura/misure-strutturali.py --check   # solo se la PR tocca Turn/RTTurnManager.*

cd tools/radar
node --test

cd ../asset-provenance
node --test

cd ../mcp
node --test
```

Ogni tool dichiara nel docstring **cosa non copre**.

⛔ `tools/mcp/check.ts` confronta l'endpoint che `.mcp.json` **dichiara** con la porta che i settings
**configurano**, e nient'altro. Un verde significa **«i due file concordano»**, mai «il ponte risponde»:
che il server sia vivo lo dice `Invoke-WebRequest http://127.0.0.1:<porta>/mcp` — `405` su GET e' un
endpoint sano che rifiuta GET — e la diagnosi completa di un ponte muto sta in
[`brief-mcp-developer-bridge.md`](docs/technical/tooling/brief-mcp-developer-bridge.md) §9.2.
⚠️ Ed e' rosso **solo dove `bAutoStartServer` e' acceso**: il bridge e' uno per macchina, e su un
checkout che non lo ospita una porta diversa non verra' mai aperta.

⛔ `tools/asset-provenance/check.ts` verifica che ogni asset abbia una **riga** nel registro di
provenienza ([`docs/technical/asset-licenze.md`](docs/technical/asset-licenze.md)), non che la licenza
sia rispettata. Un verde significa **«registrato»**, mai «consentito»: nessun controllo automatico puo'
leggere un EULA. Guarda due popolazioni — cio' che e' versionato sotto `Content/` e `tools/`, e cio' che
i package versionati **referenziano** senza che il repository lo contenga — perche' la prima da sola
sarebbe cieca sui ~15,8 GB di pack che stanno fuori dal repository per scelta.

⛔ `tools/mutation/costanti-combattimento.py` **non e' fra i controlli noti**, e di proposito. Modifica un sorgente e occupa il motore per **un build completo piu' una suite intera per ogni costante**, piu' una baseline: con le 11 di `RTCombatLibrary.h` sono **ore**, e le direzioni di mutazione da misurare sono **due** (`+3` e `-3` danno risposte diverse — vedi il docstring). Mentre gira, ogni altra misura in parallelo e' NON VALIDA. Si lancia per rispondere alla domanda che `#2118` ha posto — *quali costanti si possono cambiare senza che niente diventi rosso* — non a ogni PR.

⚠️ E se non stampa `AUDIT COMPLETO`, **ricostruire prima di qualunque altra misura**: un'interruzione lascia mutato anche il binario, che è la metà che il confronto su `HEAD` e albero non vede.

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

> 🔴 **`write_graph_dsl` NON è il gemello di `read_graph_dsl`: il giro non torna, e la differenza si paga
> in silenzio.** Misurato il 2026-09-09 su `WBP_RT_TeamRoster`.
>
> `read_graph_dsl` serializza i **pin dinamici** — quelli che un nodo guadagna dopo che una sua input è
> stata assegnata, come il `Card` che `CreateWidget` espone quando `Class` punta a un widget con variabili
> *expose on spawn*. `write_graph_dsl` quegli stessi pin li **rifiuta**:
>
> ```
> UserInterface|CreateWidget received 3 positional arg(s) but has 2 data input pin(s).
>   Available: ['Class', 'OwningPlayer']
> ```
>
> e la forma a keyword non aiuta: `Unknown input pin "Card"`.
>
> ⛔ **Quindi non si rilegge un grafo e lo si riscrive per "aggiungere un ramo".** Il risultato compila,
> nessuno strumento protesta, e i pin dinamici che c'erano prima **spariscono** — nel caso misurato,
> ogni carta del roster sarebbe nata vuota, con un rosso che nessun test esistente produce.
>
> **Come si fa invece**: scrivere col DSL la struttura **senza** quei pin, poi ricablarli con
> `connect_pins`, e **verificare uno per uno** con `get_node_infos` che l'origine sia quella giusta —
> `find_nodes` restituisce i nodi in ordine di creazione, non di semantica, e due `CreateWidget` scambiati
> danno carte con i dati dell'altra squadra senza che nulla fallisca.
>
> ⚠️ Vale per **ogni** nodo con pin dipendenti da una classe: `CreateWidget`, `SpawnActor`, i cast, le
> chiamate a funzioni con parametri wildcard. Prima di riscrivere un grafo, chiediti se qualche pin di
> quel grafo esiste solo perché un'altra input è già assegnata.

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

Fino al 2026-09-08 lo eseguiva `rt-suite.ps1` come promemoria dopo un verdetto `VALIDA`. Rimosso lo
script, si lancia a mano come gli altri radar:

🔴 **Non concorre al verdetto della suite, ed è una scelta, non una svista.** Legge GitHub, che cambia
mentre la suite gira: in una run da quaranta minuti può passare all'avvio e fallire alla fine. Farlo
entrare nelle invarianti di §9 renderebbe `NON VALIDA` una misura sana per una issue che ha modificato
qualcun altro — cioè il difetto che l'invariante esiste per impedire. Stampa, e l'esito resta quello
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

## 11. Lavoro parallelo

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

> ⚠️ **Dal 2026-09-08 nessuno script fa rispettare ciò che segue.** I ruoli operativi, il lease del motore e i guard di `scripts/` sono stati rimossi ([`D-346`](docs/decisions/RT_PDR_00_Decision_Log.md), [`D-347`](docs/decisions/RT_PDR_00_Decision_Log.md)). I vincoli fisici che li avevano motivati **non sono spariti con loro**: Unreal resta uno per macchina, un worktree resta privo dei file gitignorati, due sessioni nella stessa directory restano sullo stesso `HEAD`. Quello che prima veniva rifiutato ora riesce — e produce il danno che il rifiuto evitava.

> 🧹 **Pulizia locale, una volta per macchina.** L'installer scriveva in `.vscode/` — che è gitignorato, quindi nessun commit lo tocca — task e marker del sistema rimosso: `tasks.json` invoca `rt-terminal.ps1`, `rt-workspace.ps1` e `rt-lease.ps1`, e accanto restano `rt-engine-mode.txt` e `rt-workspace-id.txt`. Gli script non esistono più: ogni task «RT: …» fallisce con un file-not-found, e i due marker non li legge nessuno. Vanno rimossi a mano nei checkout dove l'installer era passato — l'installer che li generava è stato rimosso con il resto e non li ripulisce.

### Cosa isola una directory, e cosa no

| Configurazione | Isola | Non isola |
|---|---|---|
| Più sessioni nella **stessa directory** | niente | working tree, index e `HEAD` sono condivisi: il parallelismo è apparente |
| Sessioni in **directory separate** | working tree, index, `HEAD` | Unreal, Live Coding, DDC, CPU e disco: le risorse di macchina restano una sola |

Da cui due conseguenze:

- nella stessa directory `git add -A`, `git commit -am`, `reset`, `restore`, `clean`, `switch` e `pull --rebase` ingoiano il lavoro di chi condivide l'albero;
- in directory separate due misure Unreal non diventano parallele: diventano una coda, e se partono insieme si invalidano a vicenda.

`git status` non risponde alla domanda «questo file è mio».

### Il motore è una risorsa di macchina

Unreal è **uno** e lo condividono tutti i checkout. Da cui:

- un Editor aperto e una suite sullo **stesso** clone non convivono;
- prima di occupare il motore — Editor, PIE, build, commandlet, suite — **accertati di cosa stia già girando**. Il lease che lo diceva prima non c'è più ([`D-347`](docs/decisions/RT_PDR_00_Decision_Log.md)).

> ⌫ **Fino al 2026-09-09 questa lista diceva anche** *«una build lanciata sotto la suite di un altro checkout rende `NON VALIDA` quella misura, riscrivendo il binario che l'invariante osserva»*. **La premessa è falsa**, e la correzione vale perché applicata alla lettera bloccava lavoro che poteva procedere: `Binaries/` è **per clone**, quindi una build non riscrive il binario che un'altra suite ha caricato. La misura completa — cosa si calpesta e cosa no, con i comandi per verificarlo — è in **§9**. ⚠️ Ciò che resta vero è il **resto** della frase in altri casi: stesso clone, Editor che tiene il DLL, qualunque cosa tocchi l'Engine, e ogni misura di **performance**.

### Prendere il motore, senza un lease

`D-347` ha spostato la disciplina dagli script a chi lavora, e non l'ha sostituita con niente. Questo è il niente, reso esplicito — nessuno script, nessun file di lock, nessun processo da ricordare di spegnere.

**1 · Allocazione: una sessione, un clone.** È la sola forma di parallelismo che regge alla misura di §9: due sessioni in due cloni compilano senza calpestarsi, perché i moduli sono per clone e l'Engine è una *installed build*. Due sessioni nello **stesso** clone non sono parallele, sono in coda — e sul working tree non sono nemmeno in coda, sono sovrapposte.

**2 · Prima di prendere: leggi chi c'è, e da dove.**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

⛔ **Un conteggio di processi non serve a niente.** La `CommandLine` porta il `.uproject`, quindi **quale clone**; `UnrealEditor.exe` contro `UnrealEditor-Cmd.exe` dice se è un Editor interattivo o una run headless; e `-abslog` dice **quale sessione**. Sono le tre cose che decidono se aspettare.

**3 · Quando prendi, rendi il tuo processo leggibile.** Ogni run headless passa `-abslog` dentro la propria directory di scratchpad di sessione:

```
-abslog=<scratchpad della sessione>/<nome-parlante>.log
```

🔑 **È l'unica «dichiarazione di possesso» che sopravvive senza script: il processo stesso.** Non va creato, non va ripulito, non può restare stantio dopo un crash — se il processo non c'è, la dichiarazione non c'è. Un file di lock avrebbe tutti e tre i difetti.

**4 · Quando aspettare, e quando no.**

| Cosa gira | Tu vuoi | |
|---|---|---|
| build o suite in un **altro** clone | build o suite | **non aspettare** — §9 |
| misura di **performance** in qualunque clone | qualsiasi cosa sul motore | **aspetta**: la contesa di CPU falsa i tempi |
| qualsiasi cosa nel **tuo** clone | qualsiasi cosa | **aspetta**: stesso `Binaries/` |
| Editor interattivo sul **tuo** clone | build | **aspetta**: tiene il DLL |
| qualsiasi cosa | build di un target **Engine** | **aspetta**, e avvisa: decade l'argomento di §9 |

**5 · Se non puoi aspettare, non misurare comunque.** Si dichiara `NOT RUN` con il motivo — *«motore occupato da `<clone>`»* — invece di produrre un verde in finestra sporca. Un `NOT RUN` onesto costa un giro; una misura invalida costa la fiducia in tutte le altre.

### Authoring asset: appartiene al clone principale

Una chiamata che crea, modifica, rinomina, sposta, cancella, importa o salva un asset Unreal via MCP appartiene al **clone principale**, quello che ospita il bridge.

La ragione è tecnica e silenziosa: un worktree non ha i file **gitignorati**, quindi i riferimenti duri di un asset vi leggono `None`, e salvarlo li **azzera** — senza errore, e ce ne si accorge dopo. Il bridge MCP è inoltre uno solo: usarlo da un altro checkout muta gli asset del principale mentre si legge il `git status` del proprio.

Preparazione, ispezione e query read-only non hanno questo vincolo.

### Coordinamento

Le sessioni si coordinano su **artefatti**, non su copie locali né sul contesto di una conversazione:

- il **branch** e il **parent branch** reali;
- gli **SHA**: il commit ereditato in ingresso e quello prodotto in uscita;
- gli **handoff persistiti** su file.

Un handoff che vive solo in chat non esiste per chi viene dopo, e un'evidenza descritta a parole non è riverificabile.

### Chi ripara non firma

Non è una regola di ruolo — i ruoli non esistono più — ma di indipendenza della misura:

- chi ha scritto una correzione non ne emette da solo il verdetto sui sistemi che quella correzione tocca;
- un difetto trovato durante una verifica torna a chi possiede il codice, con la sua evidenza, e si rimisura su un commit successivo.

Vale anche quando la stessa persona fa entrambe le cose: ciò che cambia non è chi digita, è che la misura avvenga **dopo** e su un artefatto dichiarato.

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

## 14. Numeri nei documenti che produci

La nota in testa a questo file dice di non copiare qui i dati volatili. **Vale identica per ciò che
scrivi**: referto, corpo di issue, commento, corpo di PR, riga di roadmap.

**Un totale che cambia da solo non si scrive.** Quante issue di una famiglia sono aperte, quanti asset
stanno in una cartella, quante milestone esistono, quanti file ha una directory.

Non c'è un gate che rimisuri un numero in prosa. Nessuno lo vede invecchiare, e chi legge lo tratta come
corrente perché sta accanto a fatti che lo sono. Il caso di riferimento è
[`docs/roadmap/plans/README.md`](docs/roadmap/plans/README.md): la sua tabella di conteggi è ferma a una
vecchia misura mentre la cartella è cresciuta di un ordine di grandezza — il generatore è uscito con
`D-181`/`D-182`, e da allora la deriva non la segnala nessuno.

Tre forme, in ordine di preferenza:

1. **Il nome invece del numero.** Se segue un'enumerazione, il numerale è ridondante *e* fragile:
   «tutti i `CP 47.x` (#954 … #959, #1015)» batte «tutti e sette i checkpoint». I nomi non scadono.
2. **`xx` / `yy`, col modo di contarli accanto**, quando il conteggio *è* l'informazione:
   «(`xx` aperte — si contano con `gh api repos/.../milestones`)». ⚠️ Dichiara la convenzione una volta in
   testa al documento, o `xx` si legge come un dato mancante.
3. **Lascia il numero** quando non cambia da solo — e sono tre casi soli:
   - gli **esiti del passaggio che stai scrivendo** (`epic create: 0`);
   - i **conteggi storici citati da una decisione** (`D-242` centralizzò *quattro* filtri di privacy);
   - gli **zero misurati che *sono* il difetto**: `grep -ci eventlog → 0` è un fatto, non un totale.

⛔ **Non è licenza di essere vaghi.** Un fatto misurato resta misurato, con file, riga e comando. Cade il
*totale*, non l'evidenza — e un difetto espresso come conteggio (*«un solo lettore non-test»*) è evidenza.

⚠️ **Se la correzione arriva dopo la pubblicazione, vale su tutti gli artefatti dello stesso giro**: corpi
delle issue, **titolo** se contiene un conteggio, corpo della PR, e i commenti già postati
(`gh api -X PATCH repos/.../issues/comments/<id> -F body=@file`). Lasciarne uno rende la convenzione una
preferenza invece che una regola.

ℹ️ Il precedente meccanico esiste già per un caso: `scenario-notes.ts` confronta i numeri citati nella
**prosa** di uno scenario con ciò che il file dichiara. Per la prosa di documenti e issue quel gate non
c'è — ed è la ragione per cui questa sezione è una regola invece di un controllo.