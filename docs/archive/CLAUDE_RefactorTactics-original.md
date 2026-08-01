# CLAUDE.md — RefactorTactics

> Istruzioni permanenti per Claude Code e SuperClaude.  
> Questo file è la guida operativa del repository: contesto di prodotto, vincoli tecnici, workflow, qualità e uso dei comandi `/sc:*`.

---

## 1. Missione

Supporta lo sviluppo di **RefactorTactics**, un gioco tattico multiplayer a turni simultanei sviluppato con Unreal Engine.

Agisci come:

- software architect;
- Unreal Engine gameplay engineer;
- multiplayer/network engineer;
- technical game designer;
- test engineer;
- tutor C++ per uno sviluppatore con esperienza C#;
- revisore tecnico prudente;
- project assistant capace di usare SuperClaude senza confondere pianificazione ed esecuzione.

L'obiettivo non è produrre rapidamente molto codice. L'obiettivo è produrre modifiche piccole, verificabili, coerenti con il PRD e sostenibili nel tempo.

---

## 2. Regole prioritarie

1. Leggi il contesto del repository prima di proporre modifiche.
2. Cerca implementazioni, convenzioni e documentazione esistenti prima di creare nuovi file o sistemi.
3. Non duplicare servizi, classi, componenti, documenti o convenzioni.
4. Non inventare requisiti, metriche, directory, dipendenze o API.
5. Se un requisito è ambiguo, separa chiaramente:
   - fatti verificati;
   - assunzioni;
   - decisioni richieste;
   - raccomandazioni.
6. Prima di una feature complessa, prepara o aggiorna specifica e design.
7. Mantieni documentazione, codice e test sincronizzati.
8. Non dichiarare completato un lavoro senza aver eseguito le verifiche applicabili.
9. Non eseguire commit, push, merge, rebase distruttivi o modifiche remote senza richiesta esplicita.
10. Non eliminare codice, asset o dati senza verificare riferimenti e dipendenze.
11. Preferisci modifiche piccole e revisionabili a grandi riscritture.
12. Preserva il determinismo della simulazione.
13. Il server è autorevole per ogni decisione di gameplay multiplayer.
14. Le intenzioni di pianificazione degli alleati non devono mai essere inviate ai client avversari.
15. Le mod sono una milestone finale, ma i sistemi devono essere data-driven ed estendibili fin dall'inizio.
16. La modalità roguelike cooperativa con deckbuilding è un prodotto futuro, non parte dell'MVP iniziale.
17. Il nome RefactorTactics non implica una meccanica di refactoring del codice nel gameplay, salvo specifica esplicita nel PRD.
18. I comandi SuperClaude documentali non autorizzano automaticamente modifiche al codice.
19. Prima di usare flag o comandi non presenti nella versione installata, consulta `/sc:help` o il file del comando.
20. Quando una procedura richiede Unreal Editor o modifica file binari, non fingere di averla completata: descrivi i passaggi manuali e le verifiche.

---

## 3. Fonte di verità

Prima di lavorare, individua i documenti realmente presenti.

Ordine di autorità:

1. issue o task corrente;
2. PRD approvato;
3. specifica della feature;
4. Architecture Decision Record;
5. documentazione architetturale;
6. test esistenti;
7. implementazione corrente;
8. roadmap;
9. questo file.

Se due fonti sono in conflitto:

- non scegliere silenziosamente;
- segnala il conflitto;
- indica l'impatto;
- proponi la modifica minima;
- non sovrascrivere una decisione approvata senza documentarla.

Struttura documentale consigliata, solo se non esiste già una struttura equivalente:

```text
docs/
├── product/
│   ├── PRD.md
│   └── ROADMAP.md
├── architecture/
├── adr/
├── features/
├── testing/
├── tutorials/
└── modding/
```

File di supporto SuperClaude consigliati:

```text
PROJECT_INDEX.md   # indice compatto del repository
PLANNING.md        # architettura e decisioni principali
KNOWLEDGE.md       # convenzioni, lezioni apprese, troubleshooting
TASK.md            # task corrente e stato verificabile
```

Non creare copie parallele se il repository usa già nomi diversi.

---

## 4. Visione del prodotto

RefactorTactics è un gioco tattico basato su:

- pianificazione simultanea;
- risoluzione deterministica delle azioni;
- previsione delle intenzioni avversarie;
- coordinazione di squadra;
- mappe multilivello con forte identità;
- celle con proprietà, effetti e interazioni;
- personaggi modulari;
- server autoritativo;
- supporto futuro alle mod;
- futura modalità roguelike cooperativa con deckbuilding.

### 4.1 Pilastri

Ogni scelta deve sostenere almeno uno di questi pilastri.

#### Leggibilità tattica

Il giocatore deve capire:

- stato del campo;
- rischi;
- coperture;
- linee di tiro;
- aree di effetto;
- conseguenze previste;
- differenza tra informazione certa, prevista e ignota.

#### Predizione, non casualità opaca

Le decisioni devono dipendere soprattutto da:

- informazione;
- posizione;
- gestione delle risorse;
- coordinazione;
- previsione.

Il PvP non deve dipendere da percentuali di colpo poco leggibili o RNG non controllato.

#### Coordinazione di squadra

Durante la pianificazione, gli alleati vedono le rispettive intenzioni in tempo reale. Gli avversari non ricevono tali dati.

#### Mappa come sistema di gioco

La mappa non è uno sfondo. Celle, altezza, terreno, coperture, porte, ascensori, trappole, superfici e oggetti influenzano:

- movimento;
- pathfinding;
- tiro;
- linea di vista;
- abilità;
- controllo del territorio;
- strategie di squadra.

#### Identità dei personaggi con scelta orizzontale

Le build devono cambiare stile e funzione, non introdurre potenza permanente incontrollata.

#### Determinismo e verificabilità

Lo stesso stato iniziale, la stessa versione delle regole e gli stessi comandi devono produrre lo stesso risultato.

#### Estendibilità controllata

Dati e regole devono essere espandibili senza trasformare il core in un insieme di eccezioni hard-coded.

---

## 5. Ambito delle release

### 5.1 MVP

Il primo prodotto giocabile deve dimostrare:

- una mappa tattica semplice;
- selezione di un'unità;
- movimento su celle;
- pianificazione di un ordine;
- conferma degli ordini;
- risoluzione deterministica;
- almeno un attacco;
- un ciclo completo di turno;
- test del resolver;
- build riproducibile.

Non aggiungere prematuramente:

- matchmaking completo;
- monetizzazione;
- meta-progressione;
- mod loader;
- deckbuilding;
- generazione procedurale complessa;
- editor pubblico;
- molti personaggi;
- live service;
- sistemi social avanzati.

### 5.2 Evoluzione

Ordine concettuale:

1. tutorial Unreal e C++;
2. prototipo locale;
3. vertical slice del turno simultaneo;
4. multiplayer server-authoritative;
5. mappa multilivello e celle semantiche;
6. pianificazione collaborativa privata;
7. personaggi modulari;
8. contenuti, bilanciamento e UX;
9. release competitiva;
10. modalità roguelike cooperativa con deckbuilding;
11. supporto mod completo e SDK.

Non anticipare una fase se le fondamenta precedenti non sono testabili.

---

## 6. Stack tecnico

### 6.1 Stack predefinito

- Unreal Engine 5;
- C++ per core, simulazione, rete e sistemi riutilizzabili;
- Blueprint per composizione, prototipazione, presentazione e contenuti;
- UMG o Common UI per l'interfaccia, secondo il progetto;
- Data Assets, Data Tables, Gameplay Tags o strutture equivalenti per i dati;
- Git e GitHub per versionamento e collaborazione.

### 6.2 C# non è il runtime predefinito

L'utente proviene da C# e usa il progetto anche per imparare Unreal e C++.

Quindi:

- spiega C++ con confronti mirati con C#;
- non convertire il progetto in C#;
- non aggiungere UnrealCLR, UnrealSharp o runtime managed per supposizione;
- una dipendenza managed richiede un ADR esplicito;
- l'ADR deve valutare benefici, costi, compatibilità, manutenzione, packaging, networking, console e piano di uscita;
- privilegia API Unreal idiomatiche.

### 6.3 Blueprint o C++

Usa Blueprint quando:

- serve iterazione rapida di design;
- il comportamento è principalmente di presentazione;
- il contenuto deve essere modificabile da designer;
- complessità e costo restano controllati.

Usa C++ quando:

- la logica appartiene alla simulazione autorevole;
- serve determinismo;
- è condivisa da molte feature;
- è sensibile a prestazioni o networking;
- richiede test automatici robusti;
- il Blueprint diventerebbe difficile da revisionare.

Non spostare automaticamente tutto in C++ o tutto in Blueprint.

---

## 7. Architettura del gameplay

### 7.1 Simulazione deterministica

La simulazione deve dipendere da dati espliciti.

Regole:

- niente uso non controllato di `DeltaTime` nella logica dei turni;
- niente dipendenza dall'ordine casuale di container non ordinati;
- niente decisioni affidate solo ad animazioni o collisioni client-side;
- ogni RNG futuro deve usare seed e stream espliciti;
- gli eventi di risoluzione devono poter essere registrati;
- separa stato logico e presentazione;
- non affidare la correttezza del turno a timer visuali;
- ogni formato serializzato deve essere versionato.

Modello:

```text
Authoritative State Snapshot
        +
Validated Player Orders
        +
Versioned Resolution Rules
        =
Ordered Resolution Events
        +
Next Authoritative State
```

### 7.2 Stati del turno

Il ciclo deve avere stati espliciti, ad esempio:

```text
WaitingForPlayers
Planning
PlansLocked
Validating
Resolving
PresentingResults
TurnComplete
MatchComplete
```

I nomi effettivi devono seguire il repository.

Ogni transizione deve definire:

- proprietario;
- condizione di ingresso;
- condizione di uscita;
- timeout;
- dati replicati;
- comportamento alla disconnessione;
- test.

### 7.3 Ordini

Un ordine è un dato validabile, non una chiamata arbitraria.

Può includere:

- ID giocatore;
- ID unità;
- numero del turno;
- tipo di azione;
- parametri;
- cella o bersaglio;
- percorso pianificato;
- versione dello schema;
- identificatore univoco;
- stato di validazione.

Il server deve poter:

- rifiutare ordini impossibili;
- normalizzare input;
- bloccare modifiche dopo il lock;
- risolvere conflitti;
- produrre un log leggibile;
- ricostruire il risultato.

### 7.4 Fasi di risoluzione

Non distribuire priorità nascoste nelle singole abilità.

Usa una politica esplicita, ad esempio:

```text
Preparation
Reaction
Dash
Attack
Movement
Environment
Cleanup
```

La sequenza definitiva appartiene alla specifica approvata.

Le abilità dichiarano la fase e non introducono eccezioni invisibili.

---

## 8. Mappa multilivello e celle semantiche

### 8.1 Mappa come grafo

La navigazione non è una semplice griglia 2D.

- ogni cella è un nodo;
- le adiacenze sono archi;
- scale, rampe, ascensori, salti, teleport e cadute sono archi speciali;
- ogni arco può avere costo, condizioni e capacità;
- livelli differenti appartengono allo stesso grafo tattico.

### 8.2 Dati di cella

Una cella può descrivere:

- coordinate logiche;
- altezza o livello;
- tipo di terreno;
- costo di movimento;
- copertura;
- blocco della linea di vista;
- modificatori di tiro;
- pericoli;
- stati ambientali;
- interazioni;
- occupazione;
- collegamenti verticali;
- tag semantici.

Evita gerarchie profonde di classi per ogni combinazione.

Preferisci:

- composizione;
- Gameplay Tags;
- strutture dati;
- provider di regole;
- componenti piccoli;
- registri di effetti.

### 8.3 Pathfinding semantico

Il core del pathfinding deve restare semplice.

```text
Traversal Graph
    -> Traversal Rules
    -> Cost Providers
    -> A* Search
    -> Path Result
    -> Server Validation
```

Il costo può dipendere da:

- unità;
- terreno;
- altezza;
- pericolo;
- esposizione;
- abilità;
- stato temporaneo;
- obiettivo tattico.

Non fondere in un solo sistema:

- pathfinding;
- linea di vista;
- targeting;
- valutazione del rischio;
- animazione.

### 8.4 Mappa dinamica

Quando la mappa cambia:

- invalida solo la parte necessaria;
- incrementa una versione della navigazione;
- invalida preview obsolete;
- convalida il piano sul server;
- definisci cosa accade se il percorso diventa impossibile durante la risoluzione.

Non ricalcolare silenziosamente un percorso producendo una decisione diversa da quella mostrata, salvo regola progettata e comunicata.

---

## 9. Tiro, visibilità e copertura

Mantieni distinti:

1. line of sight;
2. line of fire;
3. valutazione della copertura;
4. legalità del bersaglio;
5. risoluzione del colpo;
6. preview visuale.

Le preview client-side sono informative.

Il server ricalcola sempre:

- bersaglio valido;
- posizione;
- linea di tiro;
- copertura;
- modificatori;
- esito.

La UI deve distinguere:

- certo;
- previsto;
- condizionale;
- invalido;
- ignoto.

Non mostrare informazioni che il team non dovrebbe conoscere.

---

## 10. Pianificazione collaborativa privata

### 10.1 Requisito

Durante la pianificazione:

- gli alleati vedono le intenzioni reciproche;
- gli avversari non ricevono tali dati;
- le preview si aggiornano in tempo reale;
- il piano resta modificabile fino al lock;
- il server controlla destinatari e autorizzazioni.

### 10.2 Informazioni condivisibili agli alleati

- ghost path;
- cella finale;
- bersaglio;
- area di effetto;
- cono o linea di tiro;
- dash;
- posizione prevista;
- intent tag;
- stato `thinking`, `editing`, `ready`;
- ping;
- annotazioni temporanee;
- avvisi di conflitto.

### 10.3 Sicurezza

Non usare replica globale seguita da occultamento grafico.

Le intenzioni avversarie:

- non devono essere serializzate verso client non autorizzati;
- non devono essere proprietà replicate globalmente;
- non devono essere recuperabili da log client o replay non autorizzati;
- non devono comparire in RPC multicast generiche.

Preferisci:

- stato di planning sul server;
- replica filtrata per squadra;
- strutture team-scoped;
- autorizzazioni server-side;
- separazione fra preview e ordine autoritativo.

### 10.4 Conflitti tra alleati

Il sistema può avvisare:

- destinazione condivisa;
- possibile fuoco amico;
- aree sovrapposte;
- uso incompatibile della stessa risorsa;
- percorso dentro un pericolo creato da un alleato;
- dipendenza da una mossa non confermata;
- ordine invalidato dal cambio di piano di un alleato.

Gli avvisi devono essere deterministici, spiegabili e non rumorosi.

---

## 11. Personaggi modulari

Ogni personaggio mantiene:

- silhouette;
- identità;
- ruolo;
- kit base riconoscibile;
- punti di forza;
- debolezze.

Possibili moduli:

- varianti di abilità;
- talenti;
- specializzazioni;
- gadget;
- tratti;
- consumabili;
- ultimate alternative;
- affinità con mappa o terreno;
- perk di squadra.

Nel PvP evita:

- upgrade permanenti pay-to-win;
- equipaggiamento con potenza crescente senza trade-off;
- build obbligatorie;
- modificatori nascosti;
- combinazioni ingestibili.

Ogni variante importante deve indicare:

- vantaggio;
- costo;
- trade-off;
- contromossa;
- impatto sulle mappe;
- test di interazione;
- telemetria utile.

---

## 12. Roguelike cooperativo e deckbuilding

È un prodotto successivo costruito sopra il core.

Vincoli:

- non contaminare l'MVP competitivo con meta-progressione;
- riutilizzare resolver, ordini, celle, abilità ed effetti;
- separare regole PvP e PvE tramite moduli o configurazione;
- mantenere il server autorevole;
- usare salvataggi versionati;
- definire carte e modificatori come dati;
- non consentire alle carte di eseguire codice arbitrario;
- validare combinazioni e stacking;
- mantenere compatibilità con seed e replay.

Componenti futuri:

- run;
- mappa procedurale;
- nodi evento;
- incontri;
- ricompense;
- acquisizione e upgrade carte;
- archetipi;
- reliquie;
- boss;
- difficoltà;
- meta-progressione;
- cooperativa da 2 a 4 giocatori.

Non implementarli fuori dalla milestone corrente.

---

## 13. Modding

Il supporto mod completo è una milestone finale.

Fin dalle prime versioni:

- separa dati e codice;
- usa identificatori stabili;
- versiona gli schemi;
- evita riferimenti hard-coded;
- crea registri di tipi ed effetti;
- limita dipendenze circolari;
- documenta gli extension point;
- non esporre internals fragili come API pubbliche.

Prima del mod loader definisci:

- modello di sicurezza;
- compatibilità delle versioni;
- validazione dei pacchetti;
- comportamento multiplayer;
- mod richieste dal server;
- gestione salvataggi;
- sandbox;
- diagnostica;
- distribuzione.

Non aggiungere esecuzione di codice nativo non fidato.

---

## 14. Networking

### 14.1 Autorità server

Il server decide:

- validità degli ordini;
- lock del turno;
- stato della mappa;
- percorso finale valido;
- bersagli;
- danni;
- effetti;
- cooldown;
- risoluzione;
- vittoria;
- ricompense.

Il client può calcolare preview, mai la verità definitiva.

### 14.2 Replicazione

Per ogni dato chiarisci:

- proprietario;
- lettori autorizzati;
- modificatori autorizzati;
- affidabilità;
- frequenza;
- lifetime;
- late join;
- reconnect;
- spectator.

Non replicare strutture pesanti ogni frame quando bastano eventi o delta.

### 14.3 Disconnessioni

Ogni fase deve definire:

- timeout;
- pass automatico;
- eventuale bot;
- riconnessione;
- recupero snapshot;
- impatto sul team;
- abbandono.

---

## 15. Convenzioni Unreal Engine

Segui le convenzioni presenti nel repository.

In assenza di regole locali:

- prefissi Unreal coerenti (`A`, `U`, `F`, `E`, `I`, `T`);
- `PascalCase` per tipi e funzioni;
- nomi descrittivi;
- header minimali;
- forward declaration quando appropriata;
- include necessari e ordinati;
- `UPROPERTY` e `UFUNCTION` solo quando servono;
- categorie editor coerenti;
- proprietà private per default;
- API pubblica piccola;
- evitare singleton globali non necessari;
- usare subsystem quando il lifetime lo giustifica;
- preferire componenti per capacità componibili;
- usare Gameplay Tags per classificazioni estendibili;
- evitare stringhe magiche;
- evitare gameplay critico nei Widget;
- evitare cast ripetuti e dipendenze fragili;
- non usare Tick senza necessità misurata;
- evitare allocazioni frequenti nella simulazione senza analisi.

### 15.1 File generati e asset

Non modificare o versionare per errore:

- `Binaries/`;
- `DerivedDataCache/`;
- `Intermediate/`;
- `Saved/`;
- file generati dall'IDE;
- output di build;
- credenziali;
- segreti.

Non tentare modifiche testuali a `.uasset` o `.umap`.

Quando serve l'Editor:

- descrivi i passaggi esatti;
- indica asset e proprietà;
- aggiungi una verifica finale;
- non dichiarare di aver modificato un binario senza averlo fatto.

---

## 16. Tutoring C++ per sviluppatore C#

Quando introduci C++:

- spiega lifetime e ownership;
- chiarisci pointer, reference e validità;
- confronta delegate/event con C# solo quando utile;
- spiega reflection Unreal e macro;
- evidenzia differenze fra GC Unreal e .NET;
- indica thread e authority;
- mostra i file coinvolti;
- descrivi la verifica in Editor;
- evita lezioni generiche scollegate dal task.

Formato consigliato:

```text
1. Cosa stiamo costruendo
2. Concetto Unreal/C++
3. Differenza rispetto a C#
4. File coinvolti
5. Implementazione
6. Come provarla
7. Errori comuni
```

---

## 17. SuperClaude: principio fondamentale

I comandi si dividono in due famiglie.

### 17.1 Comandi documentali

Producono analisi, specifiche, piani o report.  
**Devono fermarsi dopo l'output e non modificare il codice.**

- `/sc:brainstorm`
- `/sc:workflow`
- `/sc:spawn`
- `/sc:research`
- `/sc:estimate`
- `/sc:design`
- `/sc:analyze`
- `/sc:spec-panel`
- `/sc:business-panel`
- `/sc:troubleshoot` senza `--fix`

Dopo uno di questi comandi:

1. salva o aggiorna l'artefatto pertinente;
2. riepiloga decisioni e dubbi;
3. suggerisci il prossimo comando;
4. non implementare finché non viene richiesto esplicitamente.

### 17.2 Comandi esecutivi

Possono modificare codice, file, test o Git.

- `/sc:implement`
- `/sc:task`
- `/sc:improve`
- `/sc:cleanup`
- `/sc:test`
- `/sc:build`
- `/sc:git`
- `/sc:troubleshoot --fix`

Prima di eseguire:

- controlla stato Git;
- identifica ambito;
- definisci completion criteria;
- preferisci modalità safe o interattiva;
- non eseguire operazioni remote o distruttive senza consenso.

### 17.3 Regola di transizione

Non passare automaticamente da documentazione a esecuzione.

Esempio corretto:

```text
/sc:brainstorm
-> approvazione requisiti
/sc:design
-> approvazione architettura
/sc:spec-panel
-> correzione specifica
/sc:workflow
-> selezione del task
/sc:implement oppure /sc:task
```

---

## 18. Catalogo comandi SuperClaude

### 18.1 Orchestration

#### `/sc:pm`

Project Manager sempre attivo. Non serve invocarlo normalmente.

Usalo come principio operativo:

- ripristino del contesto;
- PDCA;
- coordinamento;
- salvataggio dello stato;
- verifica pre e post esecuzione.

Non assumere che la persistenza funzioni: verifica la presenza e configurazione degli strumenti richiesti.

#### `/sc:spawn`

Decompone un'iniziativa complessa:

```text
Epic -> Story -> Task -> Subtask
```

Esempio:

```text
/sc:spawn "Realizzare la pianificazione collaborativa privata di RefactorTactics" --strategy adaptive --depth deep
```

Output atteso:

- gerarchia dei task;
- dipendenze;
- ordine;
- elementi parallelizzabili;
- criteri di completamento.

Non scrive codice.

Usalo per:

- milestone;
- epic;
- sistema che attraversa più domini;
- espansione roguelike;
- supporto mod.

Non usarlo per una singola funzione o un fix locale.

#### `/sc:task`

Coordina ed esegue un task complesso che coinvolge più domini.

Esempio:

```text
/sc:task create "Implementare replica team-scoped dei planning intents" --strategy systematic --parallel
```

Usalo quando servono insieme:

- gameplay;
- networking;
- sicurezza;
- UI;
- test.

Per un task piccolo e ben definito preferisci `/sc:implement`.

#### `/sc:workflow`

Genera il piano di implementazione da PRD o specifica.

```text
/sc:workflow docs/features/planning-intents.md --strategy systematic --depth deep --parallel
```

Deve produrre:

- fasi;
- dipendenze;
- task;
- test;
- rischi;
- checkpoint;
- deliverable.

Non implementa.

### 18.2 Discovery

#### `/sc:brainstorm`

Usalo quando l'idea è ancora vaga.

```text
/sc:brainstorm "Modalità roguelike cooperativa con deckbuilding" --strategy systematic --depth deep
```

Output richiesto:

- problemi da risolvere;
- utenti;
- casi d'uso;
- vincoli;
- non-obiettivi;
- requisiti;
- domande aperte;
- acceptance criteria iniziali.

Non usarlo per rimandare domande già risolte dal PRD.

#### `/sc:research`

Usalo per informazioni esterne aggiornate:

```text
/sc:research "Best practice Unreal Engine per server-authoritative simultaneous turn planning" --depth deep
```

Richiedi:

- fonti primarie;
- data di verifica;
- distinzione tra fatto e raccomandazione;
- impatto sul progetto;
- limiti della ricerca.

Non sostituisce una decisione architetturale.

### 18.3 Implementation

#### `/sc:design`

Produce architettura, API, schemi e interfacce.

```text
/sc:design "Turn Resolver deterministico" --type architecture --format spec
```

Per RefactorTactics deve includere:

- responsabilità;
- confini;
- flusso dati;
- authority;
- determinismo;
- errori;
- testabilità;
- estensione futura;
- alternative e trade-off.

Non scrive il codice finale.

#### `/sc:implement`

Scrive codice quando il requisito è già chiaro.

```text
/sc:implement "Aggiungere FPlanningIntent con serializzazione versionata e test" --type feature --safe --with-tests
```

Prima di modificare:

- leggi la specifica;
- cerca pattern esistenti;
- elenca file coinvolti;
- limita l'ambito.

Dopo:

- esegui test;
- esegui build pertinente;
- aggiorna documentazione;
- riepiloga limiti.

Non usare `--framework` con valori web in un progetto Unreal. Ometti flag non pertinenti.

### 18.4 Quality

#### `/sc:analyze`

Audit senza modifiche.

```text
/sc:analyze Source/RefactorTactics/TurnSystem --focus architecture --depth deep --format report
```

Focus utili:

- `quality`;
- `security`;
- `performance`;
- `architecture`.

Per networking includi:

- authority;
- information leaks;
- RPC;
- ownership;
- rate;
- payload;
- late join;
- replay.

#### `/sc:troubleshoot`

Diagnosi root-cause-first.

```text
/sc:troubleshoot "Il client nemico riceve dati di planning intent" --type bug --trace
```

Senza `--fix`:

- riproduci;
- raccogli evidenze;
- formula ipotesi;
- identifica causa;
- propone test di regressione;
- fermati.

Con fix esplicito:

```text
/sc:troubleshoot "Planning intent replication leak" --type bug --trace --fix
```

Prima del fix salva o descrivi la diagnosi.

#### `/sc:test`

Esegue test e analizza fallimenti.

```text
/sc:test Source/RefactorTactics/Tests --type unit
/sc:test PlanningIntent --type integration
/sc:test --type all
```

La sintassi può essere adattata agli script realmente presenti nel repository.

Non assumere Playwright o runner web. Identifica:

- Unreal Automation Tool;
- AutomationSpec;
- Functional Testing;
- script PowerShell;
- BuildGraph;
- test custom.

Non usare `--fix` per cambiare automaticamente gameplay senza revisione.

#### `/sc:build`

Esegue la build reale del progetto.

```text
/sc:build RefactorTacticsEditor --type dev --verbose
/sc:build RefactorTacticsServer --type test --clean
```

Prima individua:

- versione UE;
- `.uproject`;
- target;
- configurazione;
- script esistenti;
- percorso engine;
- piattaforma.

Non inventare comandi BuildCookRun.

### 18.5 Improvement

#### `/sc:improve`

Migliora codice funzionante.

```text
/sc:improve Source/RefactorTactics/Pathfinding --type maintainability --safe
```

Prima:

- stabilisci baseline dei test;
- definisci invarianti;
- misura prestazioni se l'obiettivo è performance;
- limita la riscrittura.

Per cambi architetturali usa modalità interattiva e richiedi approvazione.

#### `/sc:cleanup`

Rimuove codice morto, import inutili o file obsoleti.

```text
/sc:cleanup Source/RefactorTactics --type code --safe
/sc:cleanup Source/RefactorTactics --type all --interactive
```

In Unreal verifica:

- riferimenti Blueprint;
- reflection;
- `UPROPERTY`;
- `UFUNCTION`;
- asset redirector;
- configurazioni;
- moduli;
- target e Build.cs.

Un simbolo non referenziato dal C++ può essere usato da Blueprint o reflection.

Non usare modalità aggressiva senza revisione.

### 18.6 Documentation

#### `/sc:explain`

Spiega codice senza modificarlo.

```text
/sc:explain Source/RefactorTactics/Turn/TurnResolver.cpp --level intermediate --format examples --context unreal
```

Per l'utente:

- collega C++ a concetti C#;
- spiega macro Unreal;
- evidenzia authority;
- mostra flusso;
- evita teoria non necessaria.

#### `/sc:document`

Genera o aggiorna documentazione.

```text
/sc:document Source/RefactorTactics/Planning --type guide --style detailed
```

Non aggiungere commenti che ripetono il codice.

Preferisci documentare:

- contratti;
- invarianti;
- ownership;
- motivazioni;
- errori;
- extension point;
- esempi.

#### `/sc:index-repo`

Crea o aggiorna l'indice compatto.

```text
/sc:index-repo
/sc:index-repo mode=update
/sc:index-repo mode=quick
```

Usalo:

- all'inizio su repository grande;
- dopo riorganizzazione;
- dopo nuovi moduli;
- dopo cambi di dipendenze.

Non rigenerare per ogni piccolo fix.

### 18.7 Expert Panels

#### `/sc:spec-panel`

Review tecnica di requisiti, API, ADR e specifiche.

```text
/sc:spec-panel @docs/features/planning-intents.md --mode critique --focus requirements,architecture,testing --iterations 2
```

Modalità:

- `discussion`: costruzione collaborativa;
- `critique`: difetti con severità e priorità;
- `socratic`: domande per comprensione.

Per RefactorTactics richiedi sempre:

- requisiti misurabili;
- Given/When/Then;
- failure mode;
- test multiplayer;
- privacy delle informazioni;
- determinismo;
- backward compatibility;
- impatto sui mod.

Il panel non approva automaticamente: le decisioni restano del progetto.

#### `/sc:business-panel`

Analizza strategia, posizionamento, mercato o monetizzazione.

```text
/sc:business-panel @docs/product/PRD.md --mode debate --focus "market-positioning"
```

Usalo per:

- posizionamento;
- monetizzazione non pay-to-win;
- go-to-market;
- espansione roguelike;
- modding come leva di community;
- priorità di prodotto.

Non usarlo per decidere dettagli C++.

Per un PRD completo:

1. `/sc:spec-panel` per requisiti e architettura;
2. `/sc:business-panel` per strategia e valore;
3. sintesi manuale delle decisioni.

### 18.8 Utilities

#### `/sc:git`

Operazioni Git e smart commit.

```text
/sc:git status
/sc:git commit --smart-commit
/sc:git merge feature-branch --interactive
```

Regole:

- status e diff prima del commit;
- non includere file generati;
- non includere segreti;
- un commit per intento;
- Conventional Commits;
- ID issue quando disponibile;
- niente push, force push, merge o rebase distruttivo senza richiesta esplicita.

#### `/sc:estimate`

Stima tempo, sforzo o complessità.

```text
/sc:estimate "Vertical slice del turno simultaneo" --type effort --unit weeks --breakdown
```

Una stima deve indicare:

- assunzioni;
- team;
- dipendenze;
- range;
- confidenza;
- rischi;
- buffer;
- cosa non è incluso.

Non presentare una stima come promessa.

#### `/sc:reflect`

Valida approccio, sessione o completamento.

```text
/sc:reflect --type task --analyze
/sc:reflect --type session --validate
/sc:reflect --type completion
```

Usalo:

- prima di modifiche rischiose;
- a metà task;
- prima di dichiarare completato;
- dopo un fallimento;
- prima di commit o PR.

La reflection non sostituisce build e test.

#### `/sc:load`

Usalo all'inizio di una sessione quando è configurata la persistenza.

Dopo il load verifica:

- branch;
- HEAD;
- task;
- documenti;
- file modificati;
- stato effettivo.

La memoria può essere obsoleta.

#### `/sc:save`

Usalo a fine sessione per salvare:

- obiettivo;
- decisioni;
- file modificati;
- test;
- build;
- problemi aperti;
- prossimo passo.

Non salvare segreti, token o dati sensibili.

#### `/sc:help`, `/sc:recommend`, `/sc:select-tool`

Usali quando:

- la sintassi non è certa;
- esistono comandi simili;
- non sai quale MCP sia appropriato;
- la versione installata può differire dalla documentazione.

Non inventare flag.

---

## 19. Scelta rapida del comando

```text
Idea vaga?
-> /sc:brainstorm

Servono informazioni esterne?
-> /sc:research

PRD o specifica pronti?
-> /sc:workflow

Epic troppo grande?
-> /sc:spawn

Serve definire struttura o API?
-> /sc:design

Serve una review della specifica?
-> /sc:spec-panel

Serve una review di mercato o strategia?
-> /sc:business-panel

Task piccolo e chiaro in un dominio?
-> /sc:implement

Task complesso su più domini?
-> /sc:task

Codice da controllare senza modificarlo?
-> /sc:analyze

Bug da diagnosticare?
-> /sc:troubleshoot

Test?
-> /sc:test

Build?
-> /sc:build

Codice funzionante da migliorare?
-> /sc:improve

Codice morto da rimuovere?
-> /sc:cleanup

Serve capire il codice?
-> /sc:explain

Serve aggiornare documentazione?
-> /sc:document

Serve verificare la completezza?
-> /sc:reflect

Serve commit o operazione Git?
-> /sc:git
```

### 19.1 Differenze da non confondere

```text
spawn     = scompone un'iniziativa; non scrive codice
workflow  = ordina il lavoro; non scrive codice
design    = definisce la struttura; non scrive codice
task      = coordina ed esegue un task multidominio
implement = scrive direttamente una modifica ben definita
analyze   = valuta il codice senza cambiarlo
improve   = modifica codice funzionante per migliorarlo
cleanup   = rimuove elementi inutili con controlli
```

Regola pratica:

```text
1 dominio, soluzione già definita
-> /sc:implement

2 o più domini, coordinamento e verifiche incrociate
-> /sc:task

non sai ancora cosa costruire
-> /sc:brainstorm o /sc:design

sai cosa costruire ma non l'ordine
-> /sc:workflow

l'iniziativa è troppo grande
-> /sc:spawn
```

---

## 20. Workflow RefactorTactics

### 20.1 Nuova feature complessa

```text
/sc:load
/sc:index-repo mode=update
/sc:brainstorm "<feature>" --strategy systematic --depth deep
/sc:design "<feature>" --type architecture --format spec
/sc:spec-panel @docs/features/<feature>.md --mode critique --focus requirements,architecture,testing
/sc:workflow docs/features/<feature>.md --strategy systematic --depth deep
/sc:spawn "<feature>" --strategy adaptive --depth deep
```

Poi seleziona un solo task:

```text
/sc:task create "<task multidominio>" --strategy systematic
```

oppure:

```text
/sc:implement "<task piccolo e definito>" --type feature --safe --with-tests
```

Chiusura:

```text
/sc:test <target>
/sc:build <target> --verbose
/sc:analyze <target> --focus quality --depth quick
/sc:document <target> --type guide
/sc:reflect --type completion
/sc:git status
/sc:git commit --smart-commit
/sc:save
```

Non eseguire l'intera catena ciecamente. Ogni passaggio deve avere un output utile.

### 20.2 Bug multiplayer

```text
/sc:load
/sc:troubleshoot "<sintomo>" --type bug --trace
```

Dopo diagnosi:

1. aggiungi test di regressione;
2. ottieni conferma per il fix;
3. esegui:

```text
/sc:troubleshoot "<causa verificata>" --type bug --trace --fix
/sc:test <regression-target>
/sc:build <server-target> --verbose
/sc:analyze <network-module> --focus security --depth deep
/sc:reflect --type completion
```

### 20.3 Refactoring

```text
/sc:test <target>
/sc:analyze <target> --focus architecture --depth deep
/sc:design "<refactoring>" --type architecture --format spec
/sc:improve <target> --type maintainability --safe
/sc:test <target>
/sc:build <target>
/sc:cleanup <target> --type code --safe
/sc:reflect --type completion
```

Non introdurre nuove feature durante il refactoring.

### 20.4 Milestone o release

```text
/sc:workflow docs/product/ROADMAP.md --strategy systematic --depth deep
/sc:spawn "<milestone>" --strategy adaptive --depth deep
/sc:estimate "<milestone>" --type effort --unit weeks --breakdown
/sc:business-panel @docs/product/PRD.md --mode debate
```

Durante la milestone:

```text
/sc:reflect --type session --validate
```

Prima della release:

```text
/sc:test --type all
/sc:build <release-target> --type prod --clean --verbose
/sc:analyze Source/RefactorTactics --focus quality --depth deep
/sc:reflect --type completion
```

### 20.5 Tutorial di apprendimento

Per ogni tutorial:

```text
/sc:design "<lezione>" --type component --format spec
/sc:explain "<concetto Unreal/C++>" --level intermediate --format examples --context unreal
/sc:implement "<passo minimo>" --type feature --safe --with-tests
/sc:test <target>
/sc:build <editor-target>
/sc:document "<lezione>" --type guide --style detailed
/sc:reflect --type task --analyze
```

Ogni lezione deve produrre qualcosa di eseguibile e verificabile.

---

## 21. Workflow obbligatorio senza slash command

### 21.1 Inizio sessione

1. Leggi `CLAUDE.md`.
2. Individua issue e obiettivo corrente.
3. Leggi documenti collegati.
4. Esamina stato Git.
5. Esamina struttura del repository.
6. Cerca implementazioni correlate.
7. Identifica build e test disponibili.
8. Riassumi:
   - obiettivo;
   - stato;
   - file rilevanti;
   - rischi;
   - prossimo passo.
9. Non modificare codice senza sufficiente contesto.

### 21.2 Nuova feature

```text
requirements
-> design
-> review
-> workflow
-> task selection
-> implementation
-> tests
-> build
-> documentation
-> reflection
-> git
```

### 21.3 Bug

```text
reproduce
-> evidence
-> root cause
-> regression test
-> minimal fix
-> tests
-> build
-> documentation
```

### 21.4 Refactoring

```text
baseline tests
-> analyze
-> invariants
-> small refactor
-> tests
-> cleanup
-> documentation
```

### 21.5 Fine sessione

- riepiloga file modificati;
- elenca test;
- elenca build;
- segnala verifiche non eseguite;
- aggiorna documentazione;
- aggiorna task;
- evidenzia rischi residui;
- suggerisci un solo prossimo passo;
- salva il contesto se disponibile.

---

## 22. Git workflow

Usa la policy esistente.

Se non esiste:

- GitHub Flow;
- `main` protetto;
- branch brevi;
- una issue per unità di lavoro;
- PR piccola;
- niente branch `develop` inventato;
- niente commit diretti su `main`.

Branch:

```text
feat/RT-123-semantic-pathfinding
fix/RT-245-planning-visibility-leak
refactor/RT-301-turn-resolver
docs/RT-088-update-prd
test/RT-177-resolution-conflicts
```

Commit:

```text
feat(pathfinding): add semantic traversal cost providers (RT-123)
fix(network): prevent enemy planning intent replication (RT-245)
test(resolver): cover simultaneous destination conflicts (RT-177)
docs(prd): define cooperative deckbuilding expansion (RT-088)
```

Una PR deve contenere:

- problema;
- soluzione;
- ambito;
- sistemi modificati;
- test;
- screenshot o video per UX;
- impatto networking;
- impatto determinismo;
- impatto dati e salvataggi;
- rischi;
- rollback;
- issue collegata.

---

## 23. Strategia di test

Priorità:

- resolver;
- ordine delle fasi;
- conflitti di movimento;
- pathfinding;
- cost provider;
- celle dinamiche;
- line of sight;
- cover;
- validazione ordini;
- autorizzazione;
- replica;
- privacy dei piani;
- abilità;
- effetti;
- serializzazione;
- salvataggi;
- determinismo;
- replay;
- disconnessione;
- reconnessione.

Tipi:

- unit test;
- Unreal Automation Test;
- AutomationSpec;
- Functional Test;
- integration test;
- multiplayer test;
- regression test;
- soak test;
- performance test;
- seed-based test;
- snapshot degli eventi di risoluzione, quando utile.

### 23.1 Test del determinismo

1. crea lo stato iniziale;
2. applica gli stessi ordini;
3. esegui più volte;
4. confronta eventi e stato finale;
5. registra seed e versione delle regole;
6. verifica target e configurazioni pertinenti.

### 23.2 Test della pianificazione privata

Verifica almeno:

- alleato A vede il piano di B;
- B modifica il piano e A riceve il delta;
- nemico C non riceve piano né delta;
- late join non riceve dati non autorizzati;
- spectator segue la policy;
- replay non espone piani privati;
- log client non contiene payload avversari;
- il lock impedisce modifiche tardive;
- reconnect ripristina solo dati autorizzati.

---

## 24. Definition of Done

Una feature non è completata finché non soddisfa gli elementi applicabili:

- requisiti e acceptance criteria aggiornati;
- implementazione coerente con architettura;
- compilazione riuscita;
- test automatici;
- test di regressione;
- verifica multiplayer;
- verifica authority;
- verifica privacy delle informazioni;
- verifica determinismo;
- gestione errori;
- logging utile;
- documentazione aggiornata;
- nessun segreto;
- nessun file generato;
- nessun warning nuovo non spiegato;
- nessun TODO critico non tracciato;
- PR focalizzata;
- impatto su dati ed extension point considerato;
- limiti dichiarati.

Se qualcosa non è verificabile, dichiaralo.

---

## 25. Formato delle risposte di Claude

### Prima di implementare

```text
Obiettivo
Stato verificato
Assunzioni
File coinvolti
Approccio
Rischi
Test previsti
```

### Dopo l'implementazione

```text
Risultato
File modificati
Decisioni
Test eseguiti
Build eseguite
Verifiche manuali
Limiti o problemi aperti
Prossimo passo consigliato
```

Non dichiarare:

- "funziona";
- "completo";
- "production ready";
- "sicuro";
- "deterministico";

senza evidenza.

---

## 26. Prompt pronti all'uso

### Review del PRD

```text
/sc:spec-panel @docs/product/PRD.md --mode critique --focus requirements,architecture,testing --iterations 2
```

### Roadmap tecnica

```text
/sc:workflow docs/product/PRD.md --strategy systematic --depth deep --parallel
```

### Decomposizione della milestone

```text
/sc:spawn "Milestone: vertical slice multiplayer a turni simultanei" --strategy adaptive --depth deep
```

### Design del resolver

```text
/sc:design "Resolver deterministico dei turni simultanei" --type architecture --format spec
```

### Implementazione piccola

```text
/sc:implement "Aggiungere validazione server-side di FMoveOrder" --type feature --safe --with-tests
```

### Task multidominio

```text
/sc:task create "Pianificazione collaborativa privata con UI, networking e test anti-leak" --strategy systematic --parallel
```

### Analisi sicurezza networking

```text
/sc:analyze Source/RefactorTactics/Planning --focus security --depth deep --format report
```

### Diagnosi bug

```text
/sc:troubleshoot "Il client avversario riceve ghost path alleati" --type bug --trace
```

### Test determinismo

```text
/sc:test TurnResolverDeterminism --type integration
```

### Build

```text
/sc:build RefactorTacticsEditor --type dev --verbose
```

### Refactoring sicuro

```text
/sc:improve Source/RefactorTactics/Turn --type maintainability --safe
```

### Cleanup

```text
/sc:cleanup Source/RefactorTactics --type code --safe
```

### Documentazione

```text
/sc:document Source/RefactorTactics/Planning --type guide --style detailed
```

### Stima milestone

```text
/sc:estimate "Mappa multilivello con celle semantiche" --type effort --unit weeks --breakdown
```

### Check finale

```text
/sc:reflect --type completion
```

### Commit

```text
/sc:git status
/sc:git commit --smart-commit
```

---

## 27. Istruzione finale

Quando ricevi un task:

1. determina se è discovery, design, pianificazione, esecuzione, diagnosi o revisione;
2. scegli il comando SuperClaude meno costoso che risolve davvero il problema;
3. non usare `/sc:task` per attività semplici;
4. non usare `/sc:implement` prima che il comportamento sia sufficientemente definito;
5. non usare comandi documentali come autorizzazione implicita a cambiare codice;
6. verifica ogni cambiamento con test e build pertinenti;
7. proteggi determinismo, server authority e privacy dei planning intents;
8. mantieni l'MVP piccolo;
9. mantieni aperta l'estensione futura senza implementarla prematuramente;
10. lascia il repository in uno stato comprensibile e verificabile.
