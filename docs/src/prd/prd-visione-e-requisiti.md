# PRD — Visione, obiettivi e requisiti di prodotto

> **Non è fonte normativa.** Livello **8** della gerarchia — *visione north-star*: descrive un prodotto più
> ambizioso dello scope corrente e si legge come **direzione**, non come backlog. In caso di conflitto
> prevalgono [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) e gli ADR
> applicabili.
>
> **Testo estratto dai PDF originari, non riscritto** (regola di `docs/src/`: i sorgenti non si riscrivono).
> Ripetizioni e contraddizioni fra i tre PRD sono nell'originale e restano.

## Da dove viene

| Sorgente (rimosso il 2026-08-12) | Pagine | Cosa contribuisce qui |
|---|---|---|
| `idee-base.pdf` | 4 | Idee fondative: terreno, ruoli, azioni, interazioni azione↔terreno↔personaggio, telemetria |
| `prd-stampabile.pdf` | 40 (di cui 15 qui) | Executive summary · visione, obiettivi e perimetro · requisiti funzionali del gioco |
| `prd-e-piano-di-sviluppo.pdf` | 45 (di cui 6 qui) | Executive summary · Product Requirements Document |
| `prd-roadmap-e-percorso-didattico.pdf` | 35 (di cui 13 qui) | Executive summary · product requirements document |

Le parti tecniche degli stessi PDF stanno in
[`prd-architettura-rete-e-intenti.md`](prd-architettura-rete-e-intenti.md); tutorial, roadmap e produzione in
[`prd-percorso-didattico-e-produzione.md`](prd-percorso-didattico-e-produzione.md).

## Cosa resta vero, cosa no

**Recepito nel canone.** Il core loop a turni simultanei con planning privato, la mappa come sistema attivo
(quota, coperture, acqua/fuoco/elettricità, porte e ponti che cambiano il grafo), la progressione
**orizzontale** con varianti che dichiarano cosa perdono, la privacy dell'intento come requisito di
architettura e non di UI: tutto questo è §2, §5 e §8 del [piano canonico](../../product/piano-canonico-mvp.md).

**Superato.** Il formato **4v4** (e il 3v3 come «principale») — il formato di partita **non è deciso**,
[D-011](../../decisions/RT_PDR_00_Decision_Log.md); la vittoria **a punteggio** — oggi la fine partita è a più
vie, [ADR-0003](../../decisions/adr-0003-modello-azioni-v01.md); la griglia **quadrata** con
`FRTGridCoord`/`FRTGridCellId` — [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md); il campo verticale
chiamato `Level` — si chiama **`Layer`**; il costo di traversata **moltiplicativo float** — il modello adottato
è additivo intero (piano canonico §3.1). Il **modello a chunk multilivello** di `FRTGridCellId` resta da
riconciliare se un giorno entrerà in scope.

**Recuperabile, e non ancora recepito da nessuno.** Sono i pezzi che il canone §8.1 elenca come «riferimento
per quando le feature entrano in scope», e che qui diventano finalmente `grep`-abili:

- **Modalità «Relay Control»** — relay da controllare a fine round, rotazione ogni 2 round, vittoria a
  punteggio, knockout con rientro. Il «max 12 round» va letto come parametro **di quel formato**.
- **Economia del round e le sette intent label** — `Focus · Protezione · Scout · Controllo · Fuga · Trappola ·
  Attesa`, con 1 movimento + 1 azione + 0-1 reazione + 0-1 interazione e **una** label.
- **Configurazione di build competitiva** — chassis fisso, 1 spec su 3, 2 modificatori abilità, 2 gadget,
  2 tratti a budget, 1 ultimate su 2-3.
- **Roguelike cooperativo** — 1-4 giocatori, 3 atti, deck 8-12, energia, reliquie, «stanze curate + validator».
  Nessun documento corrente lo tratta; è la deviazione di scope più grande dell'intero corpus.
- **Metriche di prodotto e telemetria POC** — *action usage*, *damage per action*, *success rate*,
  *turn length*, distribuzione vittorie per composizione. Il repo misura test, non partite.

**Da leggere con sospetto.** `idee-base.pdf` propone il **Gameplay Ability System** e una struttura a
`DataTable` CSV: entrambi fuori dalla v0.1. Ed è il documento più generico del gruppo — molte affermazioni sono
game design generale («i boschi rallentano e forniscono copertura»), non decisioni su questo gioco. È già stato
consumato una volta, come input di
[`../../archive/gameplay/spec-terreni.md`](../../archive/gameplay/spec-terreni.md).

---

## Idee fondative (`idee-base`)

### Executive Summary
• **Terreno variegato:** Includere tipi come copertura (muri, cespugli), terreno difficile (fango, acqua) e ostacoli (trincee, barriere) che influenzino movimento e visibilità【10†L99-L107】. Ad esempio, i boschi rallentano e forniscono copertura, l’altura migliora l’accuratezza e l’acqua rallenta drasticamente (tabella 1). Evitare inconsistenza nei cover system: un crate deve sempre comportarsi uguale【17†L289-L294】. Meccaniche dinamiche (es. distruggere coperture o incendiare terreno) aggiungono profondità, ma vanno usate con coerenza o eliminate【6†L138L145】【10†L99-L107】.

• **Personaggi e classi:** Definire ruoli classici (Tank, DPS corpo a corpo, DPS a distanza, Support/ Healer, CC) con statistiche (HP, difesa, risorse) e risorse (es. punti azione, mana) differenziate 【19†L54-L62】【19†L67-L75】. Progressione tramite livelli, equipaggiamento e talent tree (scelte permanenti) fornisce senso di crescita. Ad esempio, i talenti di WoW permettono a un Druido di cambiare ruolo【19†L130-L137】. Queste scelte aumentano la profondità, ma richiedono attento bilanciamento. Tabella 2 riassume punti di forza e debolezze dei ruoli principali.

• **Azioni e abilità:** Le azioni includono **movimento** (uso di PA per spostarsi), **attacco base** (singolo bersaglio, costo basso) e **abilità speciali** (es. magie, abilità uniche) o **interazioni ambientali** (spingere massi, aprire porte). Ogni abilità ha parametri come costo (PA, cooldown), raggio di azione (singolo bersaglio o area) e tempo di esecuzione. Esempi concreti in tabella 3 (nomi, danno base, costo PA, CD, area). Le abilità possono scalare con statistiche (es. +% danno per punto di forza). È importante testare tempi e costi per evitare turni troppo lunghi. Unreal Engine supporta sistemi data-driven (Gameplay Ability System) per definire queste abilità via dati (Data Asset/CSV)【33†L23-L32】.

• **Interazioni Azione–Terreno–Personaggio:** Le azioni possono colpire personaggi o modificare terreno. Ad esempio, un attacco a proiettile infuoca il suolo (terreno → condizione). Il terreno può fornire copertura (+difesa) o ostacolare (difficile da attraversare)【10†L99-L107】. Condizioni e trigger (es. stordito, avvelenato) permettono combo: spingere un nemico nel fuoco crea ingaggio ambientale. I buff/debuff possono dipendere dal terreno (es. +Danno se il nemico è bagnato). È utile consentire manovre come knockback per creare effetti a catena【6†L112L119】.

<!-- Start of picture text -->
Inizio Turno<br>Fase Giocatore<br>Sposta/Agisci con ogni<br>unità<br>UnitActions<br>Fine Fase Giocatore<br>Fase Nemici<br>AI muove/attacca ogni<br>nemico<br>EnemyActions<br>Fine Fase Nemici<br>Fine Turno<br><!-- End of picture text -->

- **Fasi di gioco e condizioni:** Un **turno** può essere strutturato a team (turno intero del giocatore, poi dei nemici) o a unità singole. Tra i turni si aggiornano stati come recupero PA, durata buff e trigger di eventi ambientali (per es. pioggia che spegne incendi). Stati di campo (es. nebbia, notte) possono alterare visibilità o abilità. Condizioni speciali (avvelenamento, paralisi) influenzano temporaneamente unità. In certi giochi i round contengono più di un giro (es. in _Fire Emblem_ tutti i giocatori, poi tutti i nemici)【15†L117-L124】. Diagramma sopra mostra un flusso tipico di turno.

- **Personalizzazione:** Differenziare estetica (skin) da funzionalità. Per la **custom di gameplay** , usare loadout (equipaggiamento e abilità selezionate prima della partita) e talent tree per sbloccare modifiche permanenti (incrementi di stat o abilità speciali). Esempi: gemme/ incantesimi inseribili nelle armi per effetti speciali (fire arrow, effetto acid)【6†L118-L124】, oppure rune che aggiungono effetti (knockback, cura extra). La scelta tra molte opzioni accresce replayability, ma richiede un’interfaccia chiara. Un albero di talenti consente specializzazioni (es. Tank più difensivo o più agro【19†L130-L137】). Varianti: abilitare moduli (oggetti equipaggiabili con passivi), sistemi di crafting. _Pro:_ grande varietà, senso di crescita. _Contro:_ complessità e possibile squilibrio se non testato.

- **Ruoli e bilanciamento:** Classi archetipo includono **Tank** (alto HP/difesa, attacco basso), **DPS melee** (alto danno fisico, mobilità media), **DPS ranged** (danno a distanza, bassa difesa), **Healer/ Support** (cura/bonus, offensiva debole), **CC/Utility** (stordimenti, buff/debuff, danno ridotto). Vedi tabella 2 per pro/contro. Ogni ruolo dovrebbe avere contromisure: per esempio, tank protegge il team ma è vulnerabile alle debolezze elementali; gli attacchi ad area danno vantaggio contro gruppi ma poco contro tank. Il sistema trinità (Tank/Healer/DPS) è base 【19†L54-L62】, ma è utile mixare ruoli (es. un DPS con qualche cura【19†L67-L75】). Bilanciare genera e risorse (PA/mana) garantisce che nessun ruolo domini sempre.

|**Ruolo**|**Punti di forza**|**Debolezze**|
|---|---|---|
|**Tank**|Molta salute, alta difesa, agro<br>(aggro)|Danno basso, mobili più lenti|
|**DPS Cc**|Alto danno in mischia, buona<br>mobilità|Difesa bassa, range limitato|
|**DPS Ranged**|Alto danno a distanza, flessibilità|Richiede copertura, meno resistenza|
|**Support/**<br>**Healer**|Cura e buff alleati|Danno quasi nullo, risorse critiche|
|**Controllo (CC)**|Abilità di stordire/imbavagliare|Danno medio-basso, dipende da<br>tempistica|

**Esempi di abilità:** Alcuni esempi concreti (tutte modificabili per bilanciamento):

•

|**Abilità**|**Tipo**|**Danno/**<br>**Effetto**|**Raggio/**<br>**AOE**|**Costo**<br>**PA**|**Cooldown**|**Trigger**|
|---|---|---|---|---|---|---|
|Colpo<br>Trascinante|Attacco|40 danni +<br>spinta|1 casella|1|0|Knockback se<br>bordo (es.)|
|Freccia<br>Infuocata|Magia|30 danni + +5<br>DOT/turno|Raggio 5,<br>AOE 2|2|2 turni|Ignite terreno<br>al punto<br>d’impatto|
|Scudo<br>Benedetto|Supporto|+20 Difesa a<br>un alleato|0<br>(contatto)|1|3 turni|Se alleato<br><50% HP|
|Terremoto<br>del Guerriero|AoE/<br>Danno|25 danni +<br>stordimento|Raggio 3,<br>area 2|2|4 turni|-|

_(Esempio: “Freccia Infuocata” infligge danno magico e innesca un bonus di danno nel tempo se il bersaglio è nel terreno infuocato. “Scudo Benedetto” si autoattiva quando un alleato scende sotto il 50% di salute.)_

• **Meccaniche emergenti e design pattern:** Fondamentale prevedere _counterplay_ e _risk/reward_ . Per esempio, abilità forti dovrebbero comportare un rischio (danno all’utilizzatore o lunga ricarica), creando tensione decisionale. Il concetto di hard/soft counter da picchiaduro【37†L288L299】 si traduce in tattica come mosse che “cancellano” certe strategie nemiche (es. parata che annulla un colpo forte) o reazioni condizionate (es. schivata con tempismo). Varietà nei contatori arricchisce il gioco【37†L288-L299】. Evitare info overload: troppo detailing (stati, buff) rende difficile orientarsi【26†L100-L107】. Il gioco dovrebbe permettere decisioni sensate senza paralisi da opzioni, facilitando momenti di “brillantezza tattica” senza doverlo essere ad ogni mossa【26†L119-L127】.

• **Telemetria e metriche POC:** Raccogliere dati di uso durante il testing: percentuale di utilizzo di ogni abilità, tempo medio per turno, tassi di uccisione e morti per ruolo, copertura delle mappe utilizzata. Monitorare esempi come abilità mai usate (da ribilanciare) o unità sempre in vantaggio. Metriche consigliate: _Action Usage_ (quante volte e quali azioni vengono scelte), _Damage per Action_ , _Success Rate_ (hit percentuali), _Turn Length_ , distribuzione di vittorie/sconfitte per composizione. Questi dati aiutano a bilanciare costi e danni ed evidenziare meccaniche in crisi.

• **Implementazione rapida in UE:** Sfruttare un sistema data-driven. Unreal offre il Gameplay Ability System (GAS) con AttributeSets e Gameplay Effects【33†L23-L32】. Definire ogni abilità come Data Asset (o tabella CSV) contenente parametri (costo PA, danno, area, cooldown) e codificare in Blueprint/C++ uno “executor” che applica quegli effetti ai bersagli. Le **DataTables** (CSV) in UE possono contenere l’elenco delle abilità con i valori di base. Ad esempio, un file JSON/ CSV può specificare “danno=50, raggio=3, effetto=Stordimento”. Le Interfacce Blueprint permettono di chiamare dinamicamente questi dati. In breve, **Attribute** per stat (HP, forza, resistenza), **Ability** per logiche (es. AttackTask, BuffTask), e **GameplayEffect** per modifiche (a valori) riducono codice rigido【33†L23-L32】.

<!-- Start of picture text -->
Sviluppo POC "RefactorTactics"<br>Pre-Produzione Analisi requisiti Concept & design document<br>Setup progetto UE<br>Sviluppo Base Implementazione griglia & movimento<br>Azioni base (attacco/mossa)<br>Aggiunta coperture/danni da terreno<br>Meccaniche Complesse Skill & classe personali<br>UI e feedback visivi<br>Testing & Rifiniture Playtest e bilanciamentoCorrezione bug e ottimizzazioni<br>-08-09 -08-16 -08-23 -08-30 -09-06 -09-13 -09-20 -09-27 -10-04<br><!-- End of picture text -->

|**Fase**|**Attività Principali**|**Stima**<br>**Ore**<br>**(min)**|**Stima**<br>**Ore**<br>**(max)**|**Deliverable**|
|---|---|---|---|---|
|Pre-produzione|Analisi/Concept/Game Design<br>Document|20h|40h|Documento GDD<br>completo|
|Impostazione<br>progetto|Creazione progetto UE5, asset<br>liberi (ad esempio da<br>Kenney e<br>Quaternius)|10h|20h|Progetto iniziale<br>funzionante|
|Movimenti e<br>turni|Implementazione griglia,<br>spostamenti e azioni base|20h|35h|Meccanica di<br>base (grid +<br>turni)|
|Terreno e<br>coperture|Aggiunta tipi di terreno, cover,<br>effetti ambientali|15h|25h|Mappe e ostacoli<br>funzionanti|
|Personaggi/skill<br>di base|Definizione classi, abilità<br>fondamentali|25h|40h|Sistema classi e<br>abilità POC|
|Iterazione e<br>debug|Playtest, bilanciamento abilità,<br>correzioni|15h|30h|POC bilanciato,<br>bugfix|

Le stime basse/alte dipendono dalla complessità dei sistemi (es. **Low** = meccaniche semplici, asset di prototipo, **High** = implementazione completa di talent tree e effetti avanzati).

**Fonti:** Analisi basata su linee guida di game design (es. uso di coperture e terreno【10†L99-L107】, coerenza delle meccaniche【17†L289-L294】【6†L138-L145】, ruoli classici【19†L54-L62】【19†L67L75】【15†L117-L124】, sistemi data-driven【33†L23-L32】, strategie di counterplay【37†L288-L299】).

---

## PRD stampabile — visione, perimetro e requisiti funzionali

### RefactorTactics — Product Requirements Document
#### Executive summary
**RefactorTactics** è un gioco tattico competitivo a turni simultanei, sviluppato con Unreal Engine, nel quale ogni giocatore pianifica in segreto rispetto alla squadra avversaria ma condivide in tempo reale le proprie intenzioni con gli alleati. Il risultato è un’esperienza basata su previsione, coordinazione e lettura del campo, non sulla velocità di esecuzione.

La principale differenza rispetto ai riferimenti del genere è che la mappa non è uno sfondo passivo. È un sistema di gioco vero e proprio: multilivello, modificabile, dotato di celle con proprietà, effetti, interazioni e collegamenti speciali. Terreno, altezza, coperture, visibilità, incendi, acqua, elettricità, ascensori, ponti, porte e portali partecipano direttamente alla risoluzione del turno.

Il secondo elemento distintivo è la collaborazione durante la pianificazione. Gli alleati vedono percorsi, bersagli, aree d’effetto, intenzioni, modifiche e stato di conferma degli altri membri del team. I nemici non ricevono questi dati, nemmeno a livello di replica di rete. Il sistema deve quindi essere progettato con una separazione rigorosa tra informazioni private, informazioni di squadra e stato pubblico.

Il terzo elemento è la personalizzazione orizzontale dei personaggi. Ogni personaggio mantiene una propria identità visiva e meccanica, ma può equipaggiare moduli, talenti, gadget, specializzazioni e varianti delle abilità. La progressione non deve concedere vantaggi statistici permanenti nel PvP: deve ampliare le possibilità strategiche senza creare giocatori numericamente superiori.

Il quarto elemento è l’estensibilità. Il supporto alle mod sarà l’ultima grande feature di prodotto, ma l’architettura dati dovrà essere predisposta sin dall’inizio. Terreni, abilità, personaggi, effetti ambientali e modalità dovranno essere definiti tramite asset, tag e contratti versionati, evitando dipendenze rigide da classi native.

La baseline di prodotto proposta è:

|Area|Decisione di prodotto|
|---|---|
|Genere|Tattico competitivo a turni simultanei|
|Piattaforma iniziale|PC, tastiera e mouse; controller successivo|
|Squadre|3 contro 3 nel formato principale|
|Modalità iniziale|Controllo di obiettivi dinamici|
|Durata obiettivo|20–30 minuti per partita|
|Pianificazione|30 secondi, configurabile per modalità|
|Risoluzione|6–12 secondi per turno|

|Area|Decisione di prodotto|
|---|---|
|Visuale|Isometrica 3D con rotazione, zoom e gestione dei livelli|
|Monetizzazione prevista|Non definita nel presente PRD|
|Progressione|Orizzontale e cosmetica|
|Server|Autorevole, preferibilmente dedicato|
|Mod|Data-only inizialmente; niente codice nativo non fidato|
|Motore|Unreal Engine 5, versione stabile bloccata per milestone|

Unreal offre sia C++ sia Blueprints come strumenti di gameplay: Epic raccomanda generalmente una base C++ che esponga API controllate, estendibili tramite Blueprint. C++ offre maggiore controllo su prestazioni, serializzazione, rete e merge; Blueprint accelera prototipazione e iterazione. Questo modello ibrido è particolarmente adatto a RefactorTactics. 1

Il Gameplay Ability System, disponibile come plugin, fornisce astrazioni per abilità, attributi, costi, cooldown, effetti, tag e networking. È quindi un buon fondamento per i personaggi, ma non deve controllare direttamente il simulatore tattico: il simulatore resta il sistema autorevole che decide ordine e risultato delle azioni. 2

**Decisione go/no-go:** il progetto deve dimostrare entro il vertical slice che la combinazione di mappa multilivello, turni simultanei e intenti condivisi è leggibile e divertente. Modding avanzato, progressione estesa, matchmaking competitivo e grande quantità di contenuti non devono essere sviluppati prima di questa validazione.

#### Visione, obiettivi e perimetro
**Visione del prodotto.** RefactorTactics deve far sentire il giocatore come parte di una squadra che sta costruendo un piano sotto pressione. Il momento centrale non è soltanto l’esecuzione dell’abilità, ma la negoziazione visiva del piano: “io mi muovo qui, tu blocca quella scala, il terzo compagno elettrifica l’acqua”.

La mappa deve produrre decisioni continuamente. La stessa abilità deve poter avere valore diverso a seconda della quota, del materiale della cella, della presenza di acqua, della copertura, del vento, della visibilità e delle connessioni verticali.

**Pubblico di riferimento.** Il pubblico principale comprende giocatori interessati a tattica competitiva, hero-based combat, deckbuilding leggero, giochi a turni e coordinazione cooperativa. Il prodotto deve essere comprensibile senza chat vocale, ma abbastanza profondo da premiare squadre organizzate.

##### Obiettivi di esperienza.
|Obiettivo|Comportamento atteso|
|---|---|
|Coordinazione immediata|Un giocatore comprende il piano del team osservando la mappa<br>per pochi secondi|

|Obiettivo|Comportamento atteso|
|---|---|
|Profondità senza build<br>obbligatorie|Più configurazioni risultano valide per ogni personaggio|
|Mappa memorabile|I giocatori nominano e riconoscono aree, transizioni ed eventi<br>della mappa|
|Informazione affidabile|Preview, warning e risoluzione producono risultati coerenti|
|Apprendimento progressivo|Il tutorial introduce una variabile tattica alla volta|
|Rigiocabilità|Personaggi, moduli e condizioni della mappa generano piani<br>differenti|
|Estensibilità|Nuovi contenuti richiedono prevalentemente dati e asset, non<br>modifiche al core|

**Non-obiettivi iniziali.** Il primo rilascio non deve includere open world, campagna narrativa completa, combattimento in tempo reale, fisica distruttiva generalizzata, generazione procedurale delle mappe competitive, mercato interno, supporto console simultaneo o scripting nativo arbitrario per le mod.

##### Principi inderogabili.
|Pilastro|Requisito|
|---|---|
|Mappa come attore|Le caratteristiche ambientali devono influire su movimento, tiro,<br>visibilità, abilità e obiettivi|
|Turni simultanei|Tutti pianificano nello stesso intervallo e il server risolve un unico<br>snapshot|
|Intenti condivisi|Gli alleati vedono i piani in tempo reale; gli avversari non ricevono quei<br>dati|
|Personaggi modulari|La build modifica stile e funzione, non soltanto valori numerici|
|Mod support|I dati di gioco sono identificabili, versionabili e validabili|
|Determinismo<br>osservabile|Lo stesso snapshot, regole, versione e seed devono produrre lo stesso<br>risultato logico|
|Server authority|Il client propone azioni; il server valida e applica lo stato ufficiale|

**Formato della partita.** La modalità principale, provvisoriamente denominata **Convergenza** , utilizza due o tre obiettivi ambientali per mappa. Le squadre guadagnano punti controllando, attivando o modificando tali obiettivi. Le eliminazioni contribuiscono al vantaggio ma non sono l’unica condizione di vittoria.

La configurazione iniziale proposta è:

|Parametro|Target|
|---|---|
|Giocatori|3 contro 3|

|Parametro|Target|
|---|---|
|Turni massimi|12|
|Tempo di pianificazione|30 secondi|
|Tempo di grazia rete|2 secondi|
|Punti vittoria|8|
|Punti per obiettivo|1–2 in base alla difficoltà|
|Punti per KO|1|
|Rientro dopo KO|Dopo un turno completo|
|Abbandono|Bot o controllo condiviso, da definire in alpha|

Il 3 contro 3 riduce il carico visivo e cognitivo prodotto da percorsi, aree, ping e interazioni ambientali. L’architettura deve comunque supportare 4 contro 4 per modalità personalizzate e playtest.

##### Ciclo fondamentale.
<!-- Start of picture text -->
Pianificazione privata persquadra Condivisione intenti traalleati Ready o scadenza timer Validazione server Snapshot immutabile Risoluzione deterministica Effetti ambientali ecleanup<br>Stato iniziale del turno Broadcast del risultato<br><!-- End of picture text -->

Il client deve mostrare una simulazione previsionale, non una promessa assoluta. La UI distingue quindi tre livelli:

|Livello|Significato|
|---|---|
|Confermato|Il risultato non dipende da azioni nemiche sconosciute|
|Previsto|Risultato probabile in base allo stato pubblico corrente|
|Incerto|Può cambiare per collisioni, reazioni, interruzioni o azioni nemiche|

La risoluzione segue una sequenza stabile:

1. snapshot degli intenti e del seed;

- effetti di inizio turno e reazioni preparatorie;

3. transizioni e movimenti, risolti per micro-step;

4. controllo, difesa e interruzioni;

5. attacchi e abilità;

- propagazione degli effetti ambientali;

7. KO, obiettivi, cooldown e cleanup.

L’ordine esatto deve essere data-driven, ma non modificabile liberamente nelle partite classificate.

#### Requisiti funzionali del gioco
**Architettura della mappa.** La mappa è un grafo tattico tridimensionale. Ogni nodo rappresenta una posizione valida; ogni arco rappresenta una transizione. La posizione logica non deve coincidere necessariamente con una griglia visiva perfettamente regolare.

<!-- Start of picture text -->
FCellId<br>├── X<br>├── Y<br>├── Layer<br>└── LocalIndex<br><!-- End of picture text -->

Il campo `Layer` distingue ponte, piano terra, tetto, tunnel o altri livelli sovrapposti. Gli archi verticali descrivono scale, rampe, ascensori, salti, zipline, portali e cadute.

Ogni cella contiene dati compatti, non un Actor completo. Gli Actor e gli ActorComponent sono adatti a oggetti e comportamenti riutilizzabili; per migliaia di celle è preferibile un archivio dati centralizzato, suddiviso in chunk, con Actor dedicati soltanto a rendering, collisione e interazioni visibili. Questa è una decisione architetturale del progetto, coerente con la distinzione Unreal tra Actor e componenti riutilizzabili. 3

|Dato della cella|Esempi|
|---|---|
|Identità|ID stabile, coordinate, layer|
|Geometria tattica|Quota, pendenza, altezza disponibile|
|Superficie|Metallo, roccia, acqua, legno, vetro|
|Movimento|Costo base, blocco, direzioni consentite|
|Difesa|Copertura bassa, alta, direzionale|
|Visibilità|Opacità, fumo, illuminazione|
|Stato ambientale|Fuoco, ghiaccio, elettricità, veleno|
|Occupazione|Unità, oggetti, ostacoli temporanei|
|Interazione|Porta, terminale, leva, ascensore|
|Tag|`Terrain.Water` ,<br>`Height.High` ,<br>`Hazard.Fire`|
|Revisione|Versione locale per cache e pathfinding|

I Gameplay Tags di Unreal sono etichette gerarchiche leggere pensate per identificare, categorizzare, confrontare e filtrare oggetti. Sono adatti a descrivere terreni, tipi di danno, stati, requisiti e sinergie, purché il progetto mantenga una tassonomia governata. 4

**Componenti logici della cella.** Il termine “componente” nel dominio della mappa indica un frammento dati, non necessariamente un `UActorComponent` .

|Componente logico|Responsabilità|
|---|---|
|Traversal|Costo, blocchi e requisiti di attraversamento|
|Surface|Materiale, conducibilità, combustibilità|
|Height|Quota, dislivello e vantaggio elevato|
|Cover|Protezione direzionale e distruttibilità|
|Visibility|Opacità, fumo, illuminazione|
|Hazard|Danno, controllo o rischio|
|Interaction|Azioni contestuali disponibili|
|Trigger|Condizioni e conseguenze|
|Occupancy|Unità e oggetti presenti|
|Acoustic|Rumore e propagazione, feature post-MVP|

**Sistemi ambientali.** I dati delle celle vengono processati da sistemi indipendenti:

|Sistema|Input|Output|
|---|---|---|
|MovementSystem|Unità, arco, celle|Percorribilità e costo|
|VisibilitySystem|Celle, quota, ostacoli|Visibilità e occultamento|
|FireSystem|Combustibilità, fuoco, vento|Propagazione e danno|
|WaterSystem|Volume, pendenza, aperture|Celle bagnate o allagate|
|ElectricitySystem|Conducibilità, acqua|Propagazione elettrica|
|IceSystem|Acqua, temperatura|Superficie ghiacciata|
|InteractionSystem|Azione, oggetto, requisiti|Modifica della mappa|
|ObjectiveSystem|Celle e unità|Controllo e punteggio|

L’MVP implementa soltanto movimento, quota, copertura, visibilità, fuoco, acqua ed elettricità. Gli altri sistemi entrano dopo che le interazioni fondamentali sono leggibili.

**Pathfinding.** Il pathfinding autorevole deve operare sul grafo tattico, non sulla sola NavMesh. A _nasce come ricerca euristica di percorsi a costo minimo su grafi; HPA_ riduce il problema tramite cluster gerarchici, mentre D* Lite è progettato per riutilizzare il lavoro quando i costi degli archi cambiano. Recast/Detour costruisce invece mesh navigabili poligonali, anche suddivise in tile, ed è particolarmente utile per movimento continuo e ambienti geometrici. 5

|Approccio|Vantaggi|Limiti per RefactorTactics|Decisione|
|---|---|---|---|
|NavMesh/<br>Recast|Ottimo movimento<br>geometrico e<br>smoothing|Non rappresenta naturalmente<br>celle semantiche, turni e costi<br>discreti complessi|Supporto visivo,<br>non autorità<br>tattica|

|Approccio|Vantaggi|Limiti per RefactorTactics|Decisione|
|---|---|---|---|
|A* su griglia|Semplice,<br>deterministico,<br>leggibile|Può espandere molti nodi su<br>mappe grandi|Scelta MVP|
|A* su grafo<br>multilivello|Modella scale, portali,<br>salti e requisiti|Richiede authoring e debug<br>dedicati|Scelta principale|
|HPA*|Riduce ricerche su<br>mappe grandi|Introduce cache e astrazioni da<br>invalidare|Alpha, se<br>necessario|
|D* Lite|Efficiente con cambi<br>frequenti dei costi|Più complesso; vantaggio ridotto<br>su mappe piccole a turni|Valutazione post-<br>alpha|
|Flow field|Utile per molte unità<br>con stessa<br>destinazione|Poco adatto a costi fortemente<br>dipendenti dall’unità|Non prioritario|

Unreal include anche `FGraphAStar` nell’AI Module, tramite `GraphAStar.h` . Il progetto può 6 utilizzarlo come implementazione di base, mantenendo propri graph adapter e query filter.

Il costo di attraversamento è calcolato con interi o fixed-point per evitare differenze tra piattaforme:

- `TraversalCost = BaseEdgeCost + TerrainCost + HeightCost`

- `+ HazardCost`

- `+ ExposurePreference`

- `+ UnitModifiers`

- `+ TemporaryEffects`

Il pathfinder deve distinguere:

|Costo|Descrizione|
|---|---|
|Costo fisico|Punti movimento realmente consumati|
|Costo tattico|Preferenza usata da AI o suggerimenti|
|Rischio|Danno o controllo atteso|
|Validità|Requisiti rigidi che rendono l’arco disponibile o vietato|

Il giocatore umano riceve per default il percorso dal costo fisico minimo. Un’opzione UI può mostrare alternative “sicura”, “coperta” o “rapida”, senza modificare le regole.

API proposta:

```
USTRUCT(BlueprintType)
structFRTCellId
```

```
{
```

```
GENERATED_BODY()
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32X=0;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Y=0;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Layer=0;
booloperator==(constFRTCellId&Other)const
{
returnX==Other.X
&&Y==Other.Y
&&Layer==Other.Layer;
}
};
```

```
USTRUCT(BlueprintType)
structFRTPathQuery
{
GENERATED_BODY()
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FRTCellIdStart;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FRTCellIdGoal;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FGameplayTagContainerUnitTags;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32MovementBudget=6;
```

```
};
```

```
UCLASS()
classREFACTORTACTICS_APIURTPathfindingSubsystem
:publicUWorldSubsystem
{
GENERATED_BODY()
```

```
public:
UFUNCTION(BlueprintCallable,Category="RefactorTactics|Pathfinding")
boolFindPath(
constFRTPathQuery&Query,
TArray<FRTCellId>&OutPath,
```

```
int32&OutCost)const;
};
```

Contratto funzionale:

##### `FindPath(Query)`

- `→ controlla la revisione del grafo`

- `→ filtra gli archi non validi`

- `→ interroga i cost provider registrati`

- `→ esegue A*`

- `→ restituisce percorso, costo e motivi di fallimento`

La cache utilizza almeno `GraphRevision` , `UnitTraversalProfile` , `Start` , `Goal` e `MovementBudget` . Un evento dinamico incrementa la revisione dei chunk interessati, non necessariamente dell’intera mappa.

**Line of sight e tiro.** LOS, pathfinding e traiettoria sono servizi separati. La LOS logica deve attraversare le celle interessate e considerare quota, copertura, fumo e opacità. Una successiva trace Unreal può confermare collisioni geometriche tra punti di tiro e bersaglio. Unreal espone line trace, shape trace e simulazioni di traiettorie con collisione; queste funzioni sono utili per la validazione geometrica e per le preview visive. 7

##### `CanTarget`

- `├── gittata valida`

- `├── cella bersaglio nota`

- `├── LOS logica valida`

- `├── regole di copertura`

- `├── requisiti dell’abilità`

- `└── trace geometrica, quando richiesta`

La quota modifica il tiro attraverso regole esplicite, non bonus impliciti difficili da capire. Esempio:

|Condizione|Effetto|
|---|---|
|Attaccante più alto|Ignora copertura bassa adiacente al bersaglio|
|Dislivello eccessivo|Bersaglio fuori arco per armi specifiche|
|Fumo tra due celle|Tiro diretto vietato o penalizzato|
|Parete distrutta|LOS aggiornata nello stesso turno dopo la distruzione|
|Tiro ad arco|Può superare ostacoli bassi ma non soffitti|

**Interazioni tra abilità e mappa.** Ogni effetto deve dichiarare target, tag richiesti, tag applicati e durata.

|Abilità|Risultato|
|---|---|
|Muro di ghiaccio|Crea ostacolo, copertura e archi bloccati|

|Abilità|Risultato|
|---|---|
|Granata fumogena|Aggiunge opacità e limita il targeting|
|Campo gravitazionale|Aumenta il costo degli archi nell’area|
|Portale|Registra un arco temporaneo tra due celle|
|Esplosivo|Danneggia unità e componenti distruttibili|
|Fulmine|Si propaga nelle celle con tag conduttivi|
|Congelamento|Trasforma acqua in terreno solido e scivoloso|
|Spinta|Sposta un’unità lungo archi forzati, con collisione|

Gli effetti ambientali sono risolti tramite eventi, per esempio:

##### `AbilityImpact`

- `→ AddTag(Cell, State.Wet)`

- `→ ElectricitySystem riceve CellChanged`

- `→ cerca celle conduttive connesse`

- `→ applica effetto alle unità presenti`

- `→ registra il risultato nel TurnLog`

**Eventi dinamici della mappa.** Gli eventi possono essere programmati, attivati da abilità o causati da obiettivi.

|Evento|Conseguenza tecnica|
|---|---|
|Ponte distrutto|Rimozione di archi e aggiornamento LOS|
|Porta aperta|Attivazione di archi e riduzione opacità|
|Ascensore spostato|Cambio dei collegamenti verticali|
|Stanza allagata|Modifica superficie e conducibilità|
|Incendio|Hazard, visibilità e propagazione|
|Crollo|Nuove celle bloccate e possibili cadute|
|Terminale violato|Modifica obiettivo o sistema di sicurezza|

Ogni evento deve produrre un `MapChangeSet` serializzabile:

##### `MapChangeSet`

- `├── TurnNumber`

- `├── SourceId`

- `├── ChangedCells[]`

- `├── AddedEdges[]`

- `├── RemovedEdges[]`

```
├── AppliedTags[]
└── NewGraphRevision
```

**Pianificazione e intenti condivisi.** Durante la pianificazione ogni giocatore vede:

|Elemento|Alleato|Nemico|
|---|---|---|
|Percorso pianificato|Sì|No|
|Cella finale|Sì|No|
|Abilità selezionata|Sì|No|
|Bersaglio|Sì|No|
|Area d’effetto|Sì|No|
|Direzione di dash/spinta|Sì|No|
|Label dell’intento|Sì|No|
|Stato “pronto”|Sì|No|
|Cursore esatto|Opzionale|No|
|Esito dopo la risoluzione|Sì|Sì|

Gli intenti alleati devono aggiornarsi quasi in tempo reale, con una frequenza obiettivo di 10 aggiornamenti al secondo e interpolazione grafica lato client. Non è necessario replicare ogni movimento del mouse.

##### Struttura proposta:

```
USTRUCT(BlueprintType)
structFRTPlannedIntent
```

```
{
```

```
GENERATED_BODY()
```

```
UPROPERTY(BlueprintReadWrite)
int32Sequence=0;
```

```
UPROPERTY(BlueprintReadWrite)
FGuidUnitId;
```

```
UPROPERTY(BlueprintReadWrite)
TArray<FRTCellId>PlannedPath;
```

```
UPROPERTY(BlueprintReadWrite)
FPrimaryAssetIdAbilityId;
```

```
UPROPERTY(BlueprintReadWrite)
FRTCellIdTargetCell;
```

```
UPROPERTY(BlueprintReadWrite)
FGameplayTagIntentLabel;
UPROPERTY(BlueprintReadWrite)
boolbReady=false;
};
```

Le label minime sono:

```
Intent.Focus
Intent.Protect
Intent.Scout
Intent.Trap
Intent.Wait
Intent.Escape
Intent.Interact
Intent.Control
```

**Warning di conflitto.** Il sistema segnala ma non impedisce automaticamente:

|Warning|Severità|
|---|---|
|Due alleati terminano nella stessa cella|Alta|
|Percorsi alleati si incrociano nello stesso micro-step|Media|
|Area alleata può colpire un compagno|Alta|
|Un alleato modifica una cella necessaria al tuo percorso|Alta|
|Due giocatori usano la stessa risorsa limitata|Media|
|Un’azione interrompe una combo dichiarata|Media|
|Il bersaglio previsto non sarà più visibile|Informativa|
|Il piano dipende da un esito incerto|Informativa|

I warning non devono rivelare dati nemici. Il calcolo usa esclusivamente stato pubblico e intenti della propria squadra.

**Ping rapidi.** Il menu contestuale include attacco, difesa, movimento, pericolo, attesa, interazione e richiesta di aiuto. I ping possiedono posizione, autore, tipo, durata e opzionale riferimento a cella, unità o obiettivo.

**Disegno tattico.** I giocatori possono tracciare frecce e linee temporanee. Il disegno è:

- visibile soltanto alla squadra;

- limitato in frequenza e lunghezza;

- cancellabile dall’autore;

- eliminato all’inizio della risoluzione;

- disattivabile nelle impostazioni;

##### • soggetto a mute e strumenti anti-abuso.

##### Indicatori di stato.
|Stato|Significato|
|---|---|
|Pianificazione|Nessun intento confermato|
|Modifica|L’intento è cambiato di recente|
|Provvisorio|Intento presente, non confermato|
|Pronto|Intento confermato|
|Disconnesso|Ultimo intento conservato temporaneamente|
|Auto-ready|Piano confermato dal sistema alla scadenza|

Quando tutti sono pronti il turno può iniziare dopo un countdown breve, per esempio 1,5 secondi, durante il quale è ancora possibile annullare il ready.

##### **Sistema dei personaggi.** Ogni personaggio è composto da:

- `Personaggio ├── Identità base ├── Statistiche base`

- `├── Kit fondamentale`

- `├── Moduli abilità`

- `├── Talenti`

- `├── Equipaggiamento tattico`

- `├── Specializzazione`

- `├── Tratti`

- `├── Affinità ambientali`

- `└── Ultimate`

Il kit fondamentale garantisce riconoscibilità. Un personaggio non può cambiare simultaneamente arma primaria, mobilità, difesa e ultimate fino a diventare indistinguibile dagli altri.

|Slot|Quantità equipaggiata|Pool target|
|---|---|---|
|Variante attacco|1|2–3|
|Variante mobilità|1|2|
|Variante difesa/utility|1|2–3|
|Ultimate|1|2|
|Talenti|3|9–12|
|Gadget|1|4–6 compatibili|
|Specializzazione|1|2–3|

|Slot|Quantità equipaggiata|Pool target|
|---|---|---|
|Tratto minore|1|3–5|

Esempio di variazioni della stessa abilità:

|Granata base|Variante|
|---|---|
|Impatto|Esplode immediatamente|
|Ritardata|Esplode nel turno successivo|
|Fumogena|Rimuove il danno e crea fumo|
|Mine|Distribuisce tre cariche più piccole|
|Conduttiva|Bagna l’area e prepara sinergie elettriche|

Le varianti devono scambiare vantaggi e svantaggi. Non sono accettabili upgrade che conservano tutti i benefici della versione base aggiungendo soltanto potenza.

**Specializzazioni.** Una specializzazione modifica il ruolo senza cambiare completamente l’identità. Per esempio:

|Personaggio da ricognizione|Focus|
|---|---|
|Recon|Visione, sensori e mobilità|
|Ambusher|Trappole e attacchi da posizione|
|Guardian|Copertura e protezione degli alleati|

**Equipaggiamento tattico.** I gadget sono strumenti situazionali: drone, sensore, mina, beacon, kit medico, barriera, rampino o scanner. Non devono essere semplici bonus passivi alle statistiche.

**Affinità con la mappa.** Le affinità modificano le interazioni ambientali:

|Affinità|Effetto|
|---|---|
|Acquatico|Ignora parte del costo dell’acqua|
|Piromante|Resiste al fuoco e lo utilizza come risorsa|
|Tecnico|Interagisce più rapidamente con dispositivi|
|Montanaro|Riduce il costo su roccia e dislivelli|
|Conduttivo|Potenzia elettricità ma aumenta rischi specifici|
|Ombra|Ottiene vantaggi in celle poco illuminate|

Le affinità non devono rendere obbligatorio un personaggio su una determinata mappa. Ogni ostacolo strategico deve avere più soluzioni.

**Perk di squadra.** Prima della partita il team sceglie un perk condiviso, dopo la selezione dei personaggi ma prima della schermata finale di caricamento.

|Perk|Funzione|
|---|---|
|Rete di sensori|Migliora la ricognizione iniziale|
|Kit logistico|Una carica gadget aggiuntiva condivisa|
|Fortificazione|Copertura iniziale su una zona definita|
|Riserva energetica|Recupero risorsa al primo turno|
|Protocollo medico|Rientro o cura leggermente migliorati|

I perk sono visibili alla squadra avversaria al caricamento, così da evitare vantaggi nascosti non leggibili.

**Progressione.** La progressione permanente sblocca possibilità, non potenza numerica.

|Consentito|Non consentito nel competitivo|
|---|---|
|Nuovi moduli alternativi|Danno permanente superiore|
|Nuovi gadget equivalenti|Più salute per anzianità account|
|Specializzazioni|Riduzione permanente dei cooldown|
|Skin, animazioni, banner|Precisione superiore sbloccabile|
|Linee vocali ed emote|Slot extra esclusivi|
|Modalità e mutatori|Perk numericamente migliori|

##### Vincoli di bilanciamento.
- Ogni build deve conservare almeno una debolezza rilevante.

- Nessun personaggio deve ignorare contemporaneamente movimento, LOS e pericoli ambientali.

- Le combo ambientali più forti richiedono almeno due azioni, una condizione della mappa o un rischio evidente.

- Il danno massimo prevedibile deve essere mostrabile dalla UI.

- Gli effetti di controllo devono avere contromisure o costi significativi.

- Le varianti non devono moltiplicare in modo incontrollato il numero di interazioni da testare.

- Il competitivo usa un catalogo di contenuti approvato e versionato.

##### <u>Scarica il diagramma vettoriale del ciclo di turno</u>

---

## PRD e piano di sviluppo — Product Requirements Document

### RefactorTactics: Product Requirements Document e piano completo di sviluppo
#### Executive summary
**RefactorTactics** è un gioco tattico a turni simultanei nel quale ogni giocatore pianifica movimento e azioni senza conoscere il piano avversario. Gli alleati, invece, vedono in tempo reale percorsi, bersagli, aree d’effetto e intenzioni strategiche dei compagni. La risoluzione avviene contemporaneamente secondo regole deterministiche e verificabili.

La caratteristica distintiva non è soltanto il combattimento, ma la **mappa sistemica multilivello** . Le celle rappresentano superfici, altezza, copertura, visibilità, pericoli, collegamenti verticali, condizioni ambientali e interazioni. Il pathfinding non cerca semplicemente la strada più corta: interpreta un grafo semantico in base al personaggio, agli effetti attivi e allo stato corrente della mappa.

L’architettura raccomandata è:

|Strato|Tecnologia principale|
|---|---|
|Regole, pathfinding, risoluzione,<br>networking|Unreal C++|
|Presentazione, authoring, animazioni,<br>prototipi|Blueprint|
|Abilità e status|Gameplay Ability System|
|Classificazione semantica|Gameplay Tags|
|Contenuti interni|Data Asset e Primary Data Asset|
|Mod pubbliche|JSON dichiarativo validato|
|Modalità di gioco|Game Feature plugin|
|Interfaccia|UMG, Common UI, Enhanced Input|
|Multiplayer|Server autoritativo con replica filtrata|
|Test e profiling|Automation Framework, Functional Tests, Unreal<br>Insights|

Alla data del **1º agosto 2026** , la baseline consigliata è **Unreal Engine 5.8** , pubblicato il 23 giugno 2026 e indicato da Epic come l’ultima major release pianificata della generazione UE5. Il progetto deve bloccare una versione precisa del motore e aggiornarla soltanto dopo una build stabile e una suite di regressione completa. 1

La strategia di linguaggio è **C++ più Blueprint** , non C# come dipendenza di produzione. Epic documenta esplicitamente che Blueprint e C++ sono pensati per essere combinati: C++ offre una base

rigorosa, testabile e performante, mentre Blueprint consente iterazione visuale e specializzazione dei sistemi esposti dal codice. 2

Il percorso di prodotto è diviso in quattro grandi risultati:

|Risultato|Prodotto ottenuto|
|---|---|
|Sandbox didattica|Camera, selezione, griglia e pedine|
|Vertical slice locale|Turni simultanei, mappa multilivello, abilità e interfaccia|
|Prodotto multiplayer|Match competitivo, personalizzazione, IA e servizi online|
|Piattaforma estendibile|Mod support e successiva espansione roguelike deckbuilding|

##### Deliverable stampabili
Sono disponibili tre versioni dello stesso documento:

- <u>Scarica il PRD stampabile in PDF</u>

- <u>Scarica il PRD modificabile in DOCX</u>

- <u>Scarica il sorgente Markdown</u>

Il PDF usa formato pagina standard, margini da stampa, tabelle senza sfondi scuri, diagrammi testuali e numeri di pagina. Il DOCX è pensato per aggiungere note, decisioni e revisioni del progetto.

#### Product Requirements Document
##### Identità del prodotto
**Nome di lavoro:** RefactorTactics **Genere:** tattico multiplayer a turni simultanei **Motore:** Unreal Engine **Modalità principale:** competitivo a squadre **Espansione futura:** cooperativa roguelike con deckbuilding **Principio centrale:** la mappa non è uno sfondo, ma un sistema di gioco.

La promessa al giocatore è:

Ogni turno è un problema tattico condiviso: leggi una mappa viva, comunichi il tuo intento agli alleati, prevedi il nemico e osservi la risoluzione simultanea delle decisioni.

##### Assunzioni e decisioni ancora aperte
Le informazioni non specificate vengono mantenute come decisioni formali, anziché nasconderle dietro assunzioni implicite.

|Area|Baseline proposta|Stato|
|---|---|---|
|Piattaforma iniziale|PC Windows|Da confermare|
|Store|Steam o Epic Games Store|Da decidere|

|Area|Baseline proposta|Stato|
|---|---|---|
|Formato completo|Quattro contro quattro|Proposta|
|Formato prototipo|Uno contro uno, poi due contro due|Raccomandato|
|Co-op roguelike|Da uno a quattro giocatori|Proposta|
|Durata match competitivo|Venti–trentacinque minuti|Da validare|
|Durata fase di<br>pianificazione|Quaranta–sessanta secondi<br>configurabili|Da validare|
|Monetizzazione|Non specificata|Aperta|
|Budget|Non specificato|Aperto|
|Team|Solo developer o micro-team|Assunzione di roadmap|
|Direzione artistica|Non specificata|Aperta|
|Rating|Non specificato|Aperto|
|Localizzazione|Italiano e inglese iniziali|Proposta|
|Backend online|Astrazione compatibile con EOS o<br>Steam|Da decidere|
|Matchmaking classificato|Successivo alla closed beta|Proposta|
|Fog of war|Non ancora definita|Decisione di game<br>design|

La roadmap temporale presentata più avanti è aggressiva per uno sviluppatore alle prime armi. È plausibile per un micro-team o per sviluppo quasi full-time; lavorando part-time e imparando contemporaneamente, è ragionevole considerare un orizzonte più lungo.

##### Obiettivi del prodotto
RefactorTactics deve:

- offrire decisioni simultanee comprensibili e non arbitrarie;

- rendere la comunicazione tra alleati parte naturale dell’interfaccia;

- costruire mappe che modificano movimento, tiro, visibilità e abilità;

- consentire build differenti senza introdurre progressione competitiva pay-to-win;

- risolvere ogni turno in maniera deterministica;

- separare regole, dati e presentazione;

- essere predisposto per le mod senza dipenderne durante lo sviluppo iniziale;

- riutilizzare il core competitivo nella futura modalità roguelike;

9. funzionare come percorso pratico per imparare Unreal Engine, Blueprint e C++.

##### Utenti principali
|Utente|Bisogno|
|---|---|
|Giocatore competitivo|Decisioni leggibili, equilibrio, counterplay e assenza di informazioni<br>illegittime|
|Gruppo di amici|Pianificazione collaborativa anche senza chat vocale|
|Giocatore tattico single-<br>player|IA credibile e strumenti per comprendere gli errori|
|Giocatore co-op|Sinergie, progressione della run e decisioni condivise|
|Creatore di mappe|Validator, anteprima del grafo e strumenti di test|
|Modder|Schema documentato, errori chiari e contenuti caricabili senza<br>ricompilazione|
|Spettatore|Risoluzione leggibile, replay e visualizzazione degli intenti autorizzati|

##### Loop competitivo
Il loop completo di un round è:

```
STATO PUBBLICO DELLA MAPPA
          |
          v
PIANIFICAZIONE PRIVATA
  Movimento + azione + reazione opzionale
          |
          +------> Gli alleati vedono gli intenti
          |
          +------> I nemici non ricevono i piani
          |
          v
CONFERMA O SCADENZA TIMER
          |
          v
VALIDAZIONE SERVER
          |
          v
RISOLUZIONE SIMULTANEA
          |
          v
EFFETTI AMBIENTALI E OBIETTIVI
          |
          v
NUOVO ROUND
```

Ogni personaggio dispone, nella baseline proposta, di:

|Elemento<br>Quantità per round|
|---|
|Percorso di movimento<br>Uno|
|Azione principale<br>Una|
|Reazione o stance<br>Zero o una|
|Interazione ambientale<br>Integrata nell’azione o nel movimento|
|Modifica del piano<br>Illimitata fino al lock|
|Intent label<br>Una facoltativa|

L’intento può essere etichettato come `Focus` , `Protezione` , `Scout` , `Controllo` , `Fuga` , `Trappola` o `Attesa` . L’etichetta comunica il motivo della scelta, non soltanto la geometria.

##### Modalità competitiva iniziale
Per evitare che il tutorial debba risolvere subito tutti i problemi di un’arena completa, la modalità iniziale dovrebbe essere **Relay Control** :

|Regola|Baseline|
|---|---|
|Squadre tutorial|Una contro una, poi due contro due|
|Squadre prodotto|Quattro contro quattro|
|Obiettivo|Controllare un relay attivo al termine del round|
|Rotazione|Il relay cambia posizione ogni due round|
|Vittoria|Punteggio obiettivo o vantaggio alla fine del limite round|
|Knockout|Rientro dopo un periodo configurabile|
|Durata massima|Dodici round, da bilanciare|
|Funzione della mappa|Il relay può attivare ascensori, porte o pericoli|

Questa modalità costringe i giocatori a muoversi, sfruttare la verticalità e interagire con le celle, invece di trasformare ogni partita in uno scontro statico.

##### Feature list consolidata
|Priorità|Feature|Descrizione|
|---|---|---|
|Fondamentale|Turni simultanei|Pianificazione, lock, risoluzione e aftermath|
|Fondamentale|Planning privato|I nemici non ricevono i piani prima della risoluzione|
|Fondamentale|Intenti alleati|Percorso, target, area, stato e motivazione condivisi|
|Fondamentale|Griglia multilivello|Coordinate orizzontali più layer e archi verticali|
|Fondamentale|Pathfinding<br>semantico|Costi dipendenti da unità, terreno, status ed effetti|

|Priorità|Feature|Descrizione|
|---|---|---|
|Fondamentale|Mappa dinamica|Celle e collegamenti modificabili durante la partita|
|Fondamentale|Linea di vista|Servizio separato da movimento e pathfinding|
|Fondamentale|Combattimento|Danno, cure, spostamenti forzati, status e reazioni|
|Fondamentale|Server authority|Il server valida e risolve tutte le azioni|
|Importante|Personalizzazione|Specializzazioni, moduli abilità, gadget e tratti|
|Importante|Gameplay Tags|Vocabolario comune per superfici, unità ed effetti|
|Importante|IA tattica|Planning simultaneo senza accesso ai piani privati<br>umani|
|Importante|Editor mappe|Authoring, connettività, heatmap e validator|
|Importante|Replay|Log degli eventi deterministici e riproduzione|
|Importante|Accessibilità|Forme, pattern, scaling e controlli rimappabili|
|Importante|Analytics|Hook per UX, bilanciamento, rete e performance|
|Successiva|Spettatore|Viste autorizzate e timeline delle azioni|
|Successiva|Matchmaking|Sessioni, lobby, classificato e riconnessione|
|Finale del core|Mod support|JSON, pacchetti di contenuto e manifest|
|Espansione|Co-op roguelike|Run, incontri, boss, ricompense e meta-progression|
|Espansione|Deckbuilding|Carte tattiche, draft, reliquie e trasformazioni build|

##### Personalizzazione dei personaggi
Ogni personaggio conserva una forte identità riconoscibile ma ammette varianti orizzontali.

###### `PERSONAGGIO`

- `├── Chassis fisso │   ├── statistiche base │   ├── silhouette │   ├── ruolo narrativo │   └── affinità ambientale ├── Specializzazione ├── Modificatori abilità ├── Gadget ├── Tratti └── Ultimate`

Configurazione proposta per una build competitiva:

|Slot|Scelta|
|---|---|
|Chassis|Fisso|
|Specializzazione|Una tra tre|
|Modificatore abilità|Due|
|Gadget|Due|
|Tratto|Due entro un budget|
|Ultimate|Una tra due o tre|
|Cosmetici|Liberi, senza impatto sulle regole|

Un modificatore non deve essere semplicemente “più cinque per cento di danno”. Deve cambiare l’uso tattico.

|Abilità base|Variante|
|---|---|
|Granata|Esplosione immediata|
|Granata|Fumo senza danno|
|Granata|Mina ritardata|
|Granata|Tre cariche minori|
|Dash|Attraversa una cella pericolosa|
|Dash|Lascia una scia|
|Dash|Scambia posizione con un alleato|
|Scudo|Protezione frontale|
|Scudo|Cupola statica|
|Scudo|Riflette il primo proiettile|

Il Gameplay Ability System è adatto a organizzare abilità, costi, cooldown, attributi ed effetti; Gameplay Tags fornisce una tassonomia gerarchica utilizzabile per classificare e filtrare unità, superfici, status e requisiti. Epic usa questi sistemi anche nei propri sample, incluso Lyra. 3

##### Requisiti non funzionali
|Area|Criterio di accettazione|
|---|---|
|Determinismo|Stesso snapshot, seed e piani producono lo stesso hash finale|
|Privacy|Un client nemico non riceve strutture contenenti il piano avversario|
|Prestazioni path preview|Risposta tipica entro un frame interattivo|
|Prestazioni UI|Modifica del piano senza hitch percepibili|
|Networking|Riconnessione e ricostruzione dello stato del round|

|Area|Criterio di accettazione|
|---|---|
|Testabilità|Resolver eseguibile senza caricare un livello grafico|
|Manutenibilità|Le regole non dipendono dai Widget|
|Scalabilità|PvP e roguelike come moduli separati|
|Accessibilità|Nessuna informazione critica affidata soltanto al colore|
|Modding|Dati invalidi rifiutati prima del match|
|Replay|Hash dello stato verificato dopo ogni fase|
|Salvataggi|Versione schema e migrazioni esplicite|

##### Metriche di prodotto
La north-star proposta è la **collaborazione prodotta dagli intenti condivisi** :

Percentuale di round nei quali un giocatore modifica il piano dopo aver visualizzato il piano di un alleato, senza aumento di conflitti o timeout.

Metriche complementari:

|Categoria|Evento o misura|
|---|---|
|Onboarding|Tempo al primo percorso valido|
|Planning|Numero di revisioni piano|
|Collaborazione|Intenti visualizzati e modifiche successive|
|Conflitti|Collisioni alleate previste e risolte|
|UX|Annullamenti, errori di target e timeout|
|Bilanciamento|Pick rate, win-rate e impatto delle build|
|Mappa|Celle attraversate, zone ignorate e choke point|
|Rete|Latenza submit, perdita pacchetti e riconnessioni|
|Determinismo|Divergenze hash|
|Roguelike|Carte scelte, rimosse e trasformate|
|Mod|Errori schema, dipendenze e incompatibilità|

---

## PRD, roadmap e percorso didattico — Product requirements document

### RefactorTactics — PRD, roadmap e percorso didattico per Unreal Engine
#### Executive summary
**RefactorTactics** è un gioco tattico competitivo a turni simultanei, ispirato alla chiarezza decisionale di Atlas Reactor ma costruito attorno a tre elementi distintivi:

|Pilastro|Promessa al giocatore|
|---|---|
|Mappa come sistema di<br>gioco|Terreni, quote, collegamenti verticali, pericoli e oggetti modificano<br>movimento, tiro e abilità|
|Pianificazione<br>collaborativa privata|Gli alleati vedono in tempo reale il piano della squadra; i nemici non<br>ricevono questi dati|
|Personaggi modulari|Ogni eroe conserva una forte identità, ma può essere configurato<br>attraverso specializzazioni, modifiche alle abilità e gadget|

Il progetto viene concepito anche come **percorso per imparare Unreal Engine** partendo da una buona conoscenza di C# e da poca esperienza in C++. La strategia raccomandata è utilizzare:

- **C++ Unreal** per il modello tattico, il networking, il pathfinding e i sistemi che richiedono testabilità;

- **Blueprint** per composizione, prototipazione, animazioni, effetti e configurazione dei contenuti;

- **Data Assets, Gameplay Tags e JSON** per personaggi, abilità, terreni e mod;

- **Lua o un altro linguaggio sandboxato** soltanto nella fase finale dedicata alle mod;

- **C# come laboratorio opzionale** , non come dipendenza fondamentale del prodotto.

Unreal Engine 5.8 è stato pubblicato il 17 giugno 2026 e l’hotfix 5.8.1 è arrivato il 28 luglio 2026. Per il progetto è quindi sensato adottare **UE 5.8.1 come baseline iniziale** , bloccando poi la versione del motore durante ogni milestone importante. 1

##### Assunzioni progettuali
Gli elementi seguenti non sono stati specificati e vengono quindi trattati come ipotesi modificabili:

|Elemento|Assunzione di lavoro|
|---|---|
|Budget|Non specificato|
|Dimensione del team|Scenario A: sviluppatore singolo part-time; scenario B: piccolo<br>team da 3–5 FTE|
|Piattaforme|PC Windows iniziale; console non incluse nelle stime|
|Modalità principale|Competitivo 4 contro 4|

|Elemento|Assunzione di lavoro|
|---|---|
|Durata partita|Circa 20–35 minuti|
|Modello commerciale|Non specificato|
|Backend online|Non specificato; inizialmente sessioni dirette o servizio minimale|
|Live service|Non incluso nell’MVP|
|User-generated content<br>pubblico|Previsto soltanto dopo la stabilizzazione del gioco|

##### Pacchetto pronto per la stampa
Il report è stato impaginato per **A4** , con grafici a 300 DPI e versioni SVG vettoriali.

- <u>Scarica il PDF A4 stampabile</u>

- <u>Apri la versione HTML stampabile</u>

- <u>Scarica il kit completo ZIP con report, grafici e starter code</u>

#### Product requirements document
##### Visione
RefactorTactics deve offrire combattimenti tattici nei quali la qualità di una decisione non dipende soltanto dall’abilità scelta, ma dalla capacità di:

1. prevedere contemporaneamente le intenzioni avversarie;

2. coordinarsi con la propria squadra;

- sfruttare la struttura tridimensionale e gli stati della mappa;

4. costruire un personaggio adatto alla strategia senza ottenere vantaggi permanenti ingiusti.

La promessa sintetica è:

###### Pianifica insieme, sfrutta la mappa, sorprendi l’avversario.
##### Pubblico di riferimento
Il target primario comprende giocatori interessati a strategici tattici, hero game e combattimenti competitivi basati sulla previsione anziché sui riflessi. Il target secondario comprende creatori di mappe, modder e community interessate a modalità personalizzate.

Il prodotto dovrà essere leggibile anche senza chat vocale. Percorsi, aree d’effetto, intenzioni e conflitti tra alleati devono essere rappresentati direttamente sulla mappa.

##### Core loop della partita
Core loop del turno

Il ciclo fondamentale di un turno è:

<!-- Start of picture text -->
Pianifica movimento eazione Condividi il piano aglialleati Conferma Risoluzione simultanea<br>Osserva lo stato Effetti ambientali eobiettivi<br><!-- End of picture text -->

Durante la pianificazione, ciascun giocatore:

- seleziona un percorso;

- seleziona un’azione;

- definisce bersaglio, direzione o area;

- può aggiungere un’etichetta strategica;

- vede i piani aggiornati dei compagni;

- riceve avvisi sui possibili conflitti;

- conferma quando è pronto.

Il server chiude la fase allo scadere del timer o quando tutti i giocatori hanno confermato. La risoluzione avviene secondo fasi prestabilite e deterministiche.

##### Struttura della risoluzione
Una prima versione gestibile può utilizzare la seguente sequenza:

|Fase|Contenuto|
|---|---|
|Pre-turno|Trigger, stati, abilità preparate|
|Movimento|Movimento normale, dash, teletrasporti e collisioni|
|Reazioni|Overwatch, guardia, intercettazioni|
|Azioni principali|Attacchi, cure, abilità e interazioni|
|Impatti ambientali|Fuoco, elettricità, acqua, gas, crolli|
|Obiettivi|Controllo aree, punteggio, condizioni di vittoria|
|Cleanup|Cooldown, durata stati, rimozione entità|

Non è necessario simulare fisicamente ogni dettaglio. Il sistema autorevole produce un **event log** ; animazioni ed effetti visualizzano gli eventi senza ridefinirne il risultato.

##### Modalità iniziale
L’MVP dovrebbe includere una sola modalità:

###### Controllo tattico
Due squadre competono per uno o più punti di controllo. Le eliminazioni danno un vantaggio temporaneo, ma non devono essere l’unico modo per vincere. Questo permette alla mappa, al movimento e alle abilità di controllo di avere un ruolo centrale.

Una partita può terminare quando:

- una squadra raggiunge il punteggio obiettivo;

- termina il numero massimo di turni;

- una squadra non ha più unità recuperabili;

- una condizione specifica della mappa viene completata.

##### Priorità delle feature
|Priorità|Feature|Contenuto|Criterio di completamento|
|---|---|---|---|
|P0|Turni simultanei|Planning, commit, risoluzione,<br>aftermath|Stesso input e stesso seed<br>producono lo stesso event log|
|P0|Griglia<br>multilivello|Celle su più quote e<br>collegamenti verticali|Unità in grado di raggiungere<br>correttamente piani diversi|
|P0|Pathfinding<br>semantico|A* con cost provider e requisiti|Percorsi differenti per unità con<br>caratteristiche differenti|
|P0|LOS separata|Visibilità, cover e traiettorie<br>indipendenti dal movimento|Preview e risultato server<br>coincidono|
|P0|Planning privato|Intenti visibili solo agli alleati|Nessun payload del piano<br>presente sul client avversario|
|P1|Ambiente<br>interattivo|Terreni, hazard, oggetti,<br>trasformazioni|Almeno cinque interazioni<br>combinabili|
|P1|Personaggi<br>modulari|Kit, specializzazione, augment<br>e gadget|Due build dello stesso eroe<br>cambiano strategia, non potenza<br>assoluta|
|P1|Progressione<br>orizzontale|Sblocchi non<br>competitivamente superiori|Nessun bonus permanente a<br>danno, salute o velocità in ranked|
|P1|IA di test|Bot semplici per scenario e QA|Possono completare un match<br>senza bloccare il turno|
|P2|Editor di mappe|Posizionamento tile,<br>connessioni e validazione|Una mappa completa creabile<br>senza modificare il codice|
|P2|Data mod|JSON/Data Assets esterni|Nuovo terreno aggiunto senza<br>ricompilare il core|
|P2|Script mod|API sandboxata e versionata|Mod di esempio distribuita senza<br>codice nativo|
|P3|Workshop o<br>portale mod|Pubblicazione e download|Dipende da piattaforma e<br>modello commerciale|

##### Mappa multilivello
La mappa non deve essere trattata come una semplice matrice bidimensionale con una coordinata Z aggiunta. Il modello corretto è un **grafo tattico semantico** .

Ogni cella è identificata da:

```
USTRUCT(BlueprintType)
structFTileId
{
GENERATED_BODY()
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32X=0;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Y=0;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32Layer=0;
```

```
};
```

Una cella può contenere:

|Categoria|Esempi|
|---|---|
|Geometria|Altezza, inclinazione, volume occupato|
|Movimento|Costo, blocco, capacità richiesta|
|Superficie|Acqua, ghiaccio, metallo, fango, vegetazione|
|Protezione|Cover parziale, cover totale, parete distruttibile|
|Visibilità|Fumo, oscurità, riflessi, occlusione|
|Pericolo|Fuoco, elettricità, veleno, radiazione|
|Interazione|Console, porta, ascensore, ponte, valvola|
|Stato temporaneo|Congelata, allagata, incendiata, destabilizzata|
|Occupazione|Personaggio, oggetto, ostacolo dinamico|
|Trigger|Entrata, uscita, inizio turno, fine turno, impatto|

I collegamenti verticali sono archi del grafo:

```
Cella A, piano terreno
        │
        ├── scala: costo 20, requisito Movement.Climb
        ├── ascensore: costo 10, requisito State.Powered
        ├── salto: costo 15, requisito Ability.Jump
        └── portale: costo 5, requisito State.PortalActive
        │
Cella B, piano superiore
```

Questo modello permette di aggiungere collegamenti che non corrispondono a una normale adiacenza geometrica.

##### Pathfinding semantico
A* rimane una base appropriata perché ricerca un percorso minimo combinando il costo già accumulato con una stima euristica del costo rimanente. La formulazione classica risale al lavoro di Hart, Nilsson e Raphael. 2

Il costo di attraversamento consigliato è:

```
Costo totale =
```

```
    costo dell’arco
```

- `+ costo base della cella`

- `+ differenza di quota`

- `+ modificatore del terreno`

- `+ modificatore del personaggio`

- `+ modificatore ambientale`

- `+ modificatore temporaneo delle abilità`

Il pathfinder non dovrebbe conoscere direttamente concetti come “lava”, “robot” o “ghiaccio”. Deve interrogare provider indipendenti:

```
classITraversalCostProvider
{
public:
virtualint32GetAdditionalCost(
constFTileNode&From,
constFTileNode&To,
constFUnitTraversalContext&Unit)const=0;
```

```
virtualboolCanTraverse(
```

```
constFTileNode&From,
constFTileNode&To,
constFUnitTraversalContext&Unit)const=0;
};
```

Provider previsti:

|Provider|Responsabilità|
|---|---|
|TerrainCostProvider|Acqua, fango, ghiaccio, detriti|
|HeightCostProvider|Salita, discesa, caduta|
|HazardCostProvider|Fuoco, veleno, elettricità|
|UnitTraitProvider|Volo, isolamento, anfibio, pesante|
|AbilityCostProvider|Buff, debuff e campi temporanei|

|Provider|Responsabilità|
|---|---|
|OccupancyProvider|Unità, ostacoli e prenotazioni|
|InteractionProvider|Porte, ascensori e oggetti attivabili|

Per il prototipo è preferibile utilizzare costi interi, ordinamenti stabili e una euristica semplice. Questo rende i test e il determinismo più facili.

Quando la mappa cambia, si incrementa una `GraphRevision` . Le cache conservano la revisione con cui sono state prodotte e vengono invalidate soltanto per le regioni interessate.

Algoritmi come D _Lite o Anytime Dynamic A_ diventano interessanti quando i costi degli archi cambiano frequentemente e il costo del ricalcolo completo diventa misurabile. AD* combina ricerca anytime e riparazione incrementale, migliorando una soluzione quando è disponibile più tempo e riutilizzando il lavoro quando il grafo cambia. Non è però necessario introdurlo prima di avere dati di profiling reali. 3

##### Line of sight, cover e proiettili
LOS non deve essere incorporata nell’A*. Movimento e tiro sono query differenti.

```
Semantic Grid
    ├── Movement Query
    ├── Visibility Query
    ├── Cover Query
    ├── Projectile Query
    └── Area Query
```

Una query LOS riceve:

```
FLineOfSightResultQueryLineOfSight(
constFTileId&Source,
constFTileId&Target,
constFLineOfSightContext&Context);
```

E restituisce:

```
USTRUCT(BlueprintType)
structFLineOfSightResult
{
GENERATED_BODY()
```

```
UPROPERTY(BlueprintReadOnly)
boolbVisible=false;
```

```
UPROPERTY(BlueprintReadOnly)
floatCoverFraction=0.0f;
```

```
UPROPERTY(BlueprintReadOnly)
TArray<FTileId>TraversedTiles;
};
```

Le tracce fisiche di Unreal possono essere usate per validare geometria e occlusione, ma il risultato tattico deve dipendere da regole esplicite. Una ringhiera visivamente complessa, per esempio, dovrebbe avere un valore di cover definito e non produrre risultati casuali in base a piccoli dettagli della mesh.

##### Sistema dei personaggi
Ogni personaggio è composto da sei livelli:

###### `Personaggio`

- `├── Identità base ├── Kit caratteristico`

- `├── Specializzazione`

- `├── Augment delle abilità ├── Gadget └── Tratti e affinità`

L’identità base comprende silhouette, statistiche fondamentali, animazioni, ruolo narrativo e una o due meccaniche non sostituibili.

La specializzazione modifica il modo di utilizzare il kit:

|Eroe di esempio|Specializzazione|Effetto|
|---|---|---|
|Echo|Ricognitore|Sensori, mobilità e rivelazione|
|Echo|Sabotatore|Mine, controllo porte e dispositivi|
|Echo|Tiratore|Posizionamento, altezza e linee di tiro|

Gli augment cambiano la funzione di un’abilità:

```
Granata base
```

```
├── Fragmentation: danno immediato
├── Smoke: oscura la visibilità
├── Shock: elettrifica superfici conduttive
└── Cluster: crea tre mine più piccole
```

Il sistema deve evitare una combinazione completamente libera di ogni modulo. È preferibile utilizzare:

• slot limitati;

• moduli incompatibili;

• un budget di complessità;

- prerequisiti di specializzazione;

- tag di esclusione.

##### Progressione non distruttiva per il PvP
La progressione non deve assegnare incrementi permanenti come:

```
+10% danno
+20 punti salute
+1 movimento
```

Gli sblocchi possono invece includere:

|Categoria|Contenuto|
|---|---|
|Opzioni laterali|Specializzazioni, augment e gadget equivalenti|
|Preset|Loadout salvati e configurazioni per mappa|
|Mastery|Titoli, badge, statistiche e sfide|
|Cosmetici|Skin, VFX, animazioni, emote e voice line|
|Allenamento|Scenari, replay annotati e strumenti di analisi|
|Narrazione|Lore, dialoghi e contenuti personali|

In modalità classificata, tutte le opzioni gameplay possono essere disponibili fin dall’inizio oppure normalizzate. La progressione mantiene così motivazione e identità senza trasformarsi in un vantaggio competitivo cumulativo.

##### Pianificazione condivisa tra alleati
Durante la fase di planning, ogni giocatore vede per gli alleati:

- percorso;

- destinazione;

- posizione fantasma;

- direzione finale;

- bersaglio;

- area d’effetto;

- abilità prevista;

- stato pronto;

- intent label;

- eventuali conflitti.

Esempi di intent label:

```
Focus
Protezione
Scout
```

```
Trappola
Controllo
Ritirata
Attesa
```

Il sistema segnala, senza necessariamente bloccare:

```
Due alleati prevedono di terminare sulla stessa cella.
Il tiro attraversa una posizione alleata prevista.
La copertura potrebbe essere distrutta prima dell’azione.
Il bersaglio potrebbe uscire dalla linea di tiro.
L’abilità dipende da un ascensore non ancora attivato.
```

Quando un alleato modifica il piano, la visualizzazione deve aggiornarsi rapidamente, ma senza inviare un RPC a ogni singolo movimento del mouse. Una frequenza di circa 5–10 aggiornamenti al secondo, con invio soltanto in caso di cambiamento significativo, è una base da validare durante i test.

##### Privacy del planning
La privacy è un requisito di rete, non un’opzione grafica.

Il client nemico non deve ricevere:

- percorso pianificato;

- abilità selezionata;

- cella bersaglio;

- stato di conferma;

- etichetta d’intento;

- simulazione del risultato.

Nascondere un widget o un actor già replicato non è sufficiente. Unreal determina replicazione e rilevanza per connessione, supporta actor proprietari e condizioni come `COND_OnlyOwner` ; queste primitive possono essere utilizzate per costruire una distribuzione dei dati limitata al team. 4

Flusso raccomandato:

<!-- Start of picture text -->
Client giocatore Server Client alleato Client nemico<br>Calcola preview locale<br>ServerUpdatePlanningIntent<br>Valida ownership, team e dati<br>ClientReceiveAllyIntent<br>Nessun payload del planning inviato<br>ServerCommitIntent<br>Memorizza piano definitivo<br>Conferma commit<br>Client giocatore Server Client alleato Client nemico<br><!-- End of picture text -->

Durante l’editing:

```
UFUNCTION(Server,Unreliable)
voidServerUpdatePlanningIntent(constFPlanningIntent&Intent);
```

Durante la conferma:

```
UFUNCTION(Server,Reliable)
voidServerCommitPlanningIntent(constFPlanningIntent&Intent);
```

Il server inoltra l’anteprima mediante Client RPC soltanto ai `PlayerController` appartenenti allo stesso team:

```
UFUNCTION(Client,Unreliable)
voidClientReceiveAllyIntent(
int32AllyPlayerId,
constFPlanningIntent&Intent);
```

Il server deve validare:

- proprietà dell’unità;

- appartenenza al team;

- sequence number;

- frequenza degli aggiornamenti;

- disponibilità dell’abilità;

- compatibilità del percorso;

- dimensione massima del payload;

- contenuto dei tag;

- stato della fase di gioco.

Per l’MVP è preferibile utilizzare il sistema di replicazione stabile. Iris continua a essere indicato come sperimentale nella documentazione di UE 5.8, quindi non dovrebbe diventare una dipendenza critica prima di una spike separata. 5

##### Supporto alle mod
Il supporto alle mod è l’ultima grande feature, ma l’architettura deve prepararla fin dall’inizio.

La strategia è divisa in livelli:

|Livello|Cosa può modificare|Tecnologia|
|---|---|---|
|Data mod|Terreni, abilità, effetti, statistiche, regole|JSON, Data Assets, Gameplay Tags|
|Content mod|Mappe, mesh, materiali, audio, VFX|Unreal Editor, cooking, pak e chunk|
|Script mod|Trigger, regole e comportamenti|Lua sandboxata|
|Native mod|Codice C++ arbitrario|Non ammesso nel catalogo pubblico|

I Gameplay Tags forniscono etichette gerarchiche utili a descrivere abilità, terreni, requisiti e stati. I Primary Data Assets e l’Asset Manager offrono un sistema nativo per identificare, caricare e organizzare asset; cooking, chunking e pak possono essere utilizzati per separare contenuti aggiuntivi. 6

Esempio di manifest:

```
{
"schemaVersion":1,
"modId":"com.example.electrified-water",
"name":"Electrified Water",
"version":"0.1.0",
"gameApi":">=0.9 <1.0",
"author":"Example",
"content":[
"terrain/electrified_water.json"
],
"scripts":[
"Scripts/electrified_water.lua"
],
"permissions":[
"map.read",
"effects.apply"
],
```

```
"dependencies":[]
}
```

Esempio di terreno:

```
{
"id":"Terrain.ElectrifiedWater",
"displayName":"Acqua elettrificata",
"tags":[
"Terrain.Liquid",
"Hazard.Electric"
],
"baseMoveCost":18,
"blocksMovement":false,
"blocksVision":false,
"onEnterEffects":[
{
"effect":"Status.Shocked",
"magnitude":1,
"durationTurns":1
}
],
"interactions":[
{
"action":"Ability.Freeze",
"replaceWith":"Terrain.Ice"
}
]
}
```

L’API di scripting non dovrebbe esporre liberamente `UObject` , filesystem o rete. Deve fornire handle e capability specifiche:

```
functionmod.on_tile_enter(ctx)
ifctx.tile:has_tag("Hazard.Electric")
andnotctx.unit:has_tag("Trait.Insulated")then
ctx.effects:apply(
ctx.unit,
"Status.Shocked",
1,
)
end
end
```

Permessi possibili:

```
map.read
map.modify
unit.read
effects.apply
spawn.approved
ui.panel
storage.mod
```

Ogni script deve avere limiti di tempo, memoria e numero di operazioni per turno. Nel multiplayer, server e client devono condividere lo stesso manifest, la stessa versione API e gli stessi hash per le mod che influenzano il gameplay.
