# Spec Panel — Revisione dell'assessment «v0.1 non ancora giocabile/consegnabile»

**Documento sotto revisione**: assessment di readiness v0.1 consegnato in sessione (non presente nel repository), che si dichiara misurato su `f339f537`
**Modalità**: critique
**Focus**: requirements · testing
**Panel**: Wiegers (lead requisiti), Adzic, Nygard, Crispin, Cockburn, Meadows, Doumont
**Data revisione**: 2026-09-09
**Punto di osservazione della revisione**: `main`, HEAD `9415d7de` (2026-09-09 17:06 +0200)
**Reperti riverificati prima della pubblicazione su**: `5a560500` — vedi § 9

> Questa è una revisione della *forma e della verificabilità* dell'assessment, non l'esecuzione delle sue
> priorità. Nessuna issue creata o chiusa, nessun asset toccato, nessuna misura di suite eseguita.

---

## 0. Evidenza raccolta prima della revisione

L'assessment apre con una sezione intitolata «FATTI verificati». Sono stati rimisurati, perché una
revisione che accetta le premesse del documento che sta revisionando non è una revisione.

| # | Affermazione del documento | Comando | Esito |
|---|---|---|---|
| F1 | «`main` è arrivato al commit `f339f537`» | `git rev-list --count f339f537..HEAD` → `50` | ❌ **smentito**. `f339f537` è antenato di HEAD, ma non ne è il vertice: è delle 14:17, HEAD `9415d7de` delle 17:06 dello stesso giorno |
| F2 | «la PR #2739 appena mergeata» | `gh pr view 2739` | ✅ `MERGED`, base `main`, `2026-09-09T12:17:15Z` |
| F3 | «La PR #2740 è ancora draft e intenzionalmente rossa» | `gh pr view 2740` | ❌ **smentito**. `state: MERGED`, `isDraft: false`, `mergedAt: 2026-09-09T13:05:08Z` |
| F4 | «binding nell'Editor … è il prossimo micro-step più netto» | `git log f339f537..HEAD -- '*FastDecision*'` | ❌ **già eseguito**. `baf0b95e` — «feat(166): i tre binding sono collegati, e il gate lo conferma». Oltre: `9661b1fa` — «i bottoni della finestra esistono, e il grafo non decide nulla» |
| F5 | «#2395 è chiusa», «#2615 è chiusa» | `gh issue view` | ✅ `CLOSED` alle `10:12:58Z` e `10:45:14Z` |
| F6 | #166, #2616, #2620, #2556, #2551, #2697 aperte | `gh issue view` | ✅ tutte `OPEN` |
| F7 | «catalogo icone non rigenerato» (#2551) | `git log -1 -- Content/RT/UI/DA_IconCatalog.uasset` | ✅ **confermato**. Ultima modifica `dda91286`, **2026-09-04**; nessun commit lo tocca dopo |
| F8 | «due rossi del bot sotto Model A: #2556» | `git show --stat 57112a47` | ✅ confermato, **con precisazione**: D-361 è stata accettata (`1d4ddfa9`), ma la sua PR tocca solo `docs/OPEN_DECISIONS.md`, il Decision Log, un'istruttoria e un runbook. Nessun file sotto `Source/`. La decisione non ha spostato un test |
| F9 | «Suite completa 2231/2234» | — | ⏸️ **NOT RUN**. La suite non è stata eseguita in questa passata. Le macro `IMPLEMENT_*_TEST` dichiarate nel sorgente sono `2241` (`grep -rIoE "IMPLEMENT_(SIMPLE_)?AUTOMATION_TEST" Source/ \| wc -l`), ma è una misura **diversa** — dichiarate, non eseguite — e non conferma né smentisce il rapporto citato |
| F10 | (non dichiarato dal documento) | `git log f339f537..HEAD --merges \| wc -l` → `20` | Fra `f339f537` e HEAD sono state mergiate venti PR e chiuse fra le altre #2741, #2747, #2755, #2759, #2760, #2771. Nessuna compare nell'assessment |

**Conseguenza immediata**: il «Blocco immediato» dell'assessment e la sua priorità numero uno erano
entrambi già chiusi al momento in cui il documento è stato letto. Non per un errore di analisi — per la
forma con cui il documento dichiara il proprio istante.

---

## 1. Valutazione di qualità

| Dimensione | Punteggio | Motivazione |
|---|---|---|
| Chiarezza | 8/10 | La prosa è precisa e la diagnosi finale è la frase migliore del documento |
| Completezza | 6/10 | Copre i domini giusti, ma omette l'intera finestra `f339f537..HEAD` |
| **Testabilità** | **3/10** | Una sola riga della tabella di stato è falsificabile; le sette voci di #166 non hanno oracolo |
| Coerenza interna | 5/10 | «decisione presa» viene contata come avanzamento verso «suite verde» |
| **Ancoraggio temporale** | **2/10** | Lo stato è dichiarato come vertice («è arrivato a»), non come misura datata |

---

## 2. Findings critici

### C-01 · Il documento data se stesso con un vertice, e il vertice scade — sezione «FATTI verificati»

**WIEGERS.** «`main` **è arrivato** al commit `f339f537`» non è una misura: è un'affermazione di stato al
presente, e lo stato di `main` non è una proprietà del documento. In tre ore l'affermazione è diventata
falsa (F1), e nulla nel testo avvisa il lettore che possa esserlo.

La forma corretta esiste già ed è di una riga: *«misurato su `f339f537`, 2026-09-09 14:17 +0200»*. La
differenza non è stilistica. «È arrivato a X» invita il lettore a trattare X come corrente; «misurato su
X» lo obbliga a chiedersi dove sia il vertice adesso.

📝 **Raccomandazione**: ogni assessment porta in testata `sha` + timestamp del proprio punto di
osservazione, e nessuna riga del corpo riafferma quel punto al presente.

### C-02 · La priorità numero uno era già eseguita al momento della lettura — «Blocco immediato»

**DOUMONT.** Il documento dedica al blocco una tabella di tre righe, lo definisce «il prossimo micro-step
più netto», e lo colloca al primo posto dell'ordine di lavoro. Misurato: la PR #2740 è mergiata (F3) e i
tre binding sono collegati da `baf0b95e` (F4), con `9661b1fa` che va oltre — i bottoni della finestra
esistono.

Un lettore che avesse eseguito il documento avrebbe aperto l'Editor per collegare binding già collegati.
Il costo non è l'errore in sé: è che quel lettore avrebbe **salvato l'asset**, e in RefactorTactics
salvare un `.uasset` non è mai un'operazione neutra.

📝 **Raccomandazione**: la lista delle priorità di un assessment va riverificata contro `gh pr list
--state merged` prima di essere consegnata, non prima di essere scritta.

### C-03 · La colonna «Stato» non contiene criteri, contiene umori — tabella di sintesi

**WIEGERS.** «🟡 In integrazione», «🟠 Indietro rispetto al core», «🟢 Solido» non sono falsificabili.
Nessuna misura può contraddirli, quindi nessuna misura può confermarli. La sola riga che ho potuto tentare
di verificare è «Suite: 2231/2234» — ed è l'unica scritta come numero.

Peggio: «La maggior parte dei contratti è implementata e coperta» accanto a 🟢 Solido. *La maggior parte*
non è un criterio di superamento in un progetto che ha un Definition of Done binario
([`v0.1-definition-of-done.md`](../../roadmap/v0.1-definition-of-done.md)).

📝 **Raccomandazione**: ogni riga di stato diventa `predicato + comando + esito su <sha>`. Esempio:
*«Reaction Window: binding confermati dal gate in `RTMatchWidgetAssetTests`, verde su `baf0b95e`; pacing
p50/p90 NOT RUN»*.

### C-04 · «Decisa» viene contata come «risolta» — sezione «Suite ancora non completamente verde»

**CRISPIN.** Il documento prescrive: *«riportare la suite a verde risolvendo #2551 e **la decisione di**
#2556»*. Ma D-361 è stata accettata oggi e la sua PR tocca **solo quattro file sotto `docs/`** (F8).
Nessun test è cambiato. I due rossi del bot sono esattamente dove erano.

Un lettore che vedesse D-361 accettata nel [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e
leggesse questa riga concluderebbe che #2556 è chiusa. È aperta (F6).

📝 **Raccomandazione**: due colonne distinte, mai fuse — *decisione accettata* (`sha` del Decision Log) e
*test verde* (`sha` della misura). Sono due momenti, e in questo caso li separano zero righe di codice.

### C-05 · Il gate che invalida tutti gli altri è ultimo nella lista — «Priorità corretta ora»

**NYGARD.** L'ordine proposto mette «produrre una build packaged aggiornata» **al quinto posto**, dopo tre
voci di UI. Ma packaged è l'unico gate le cui rotture non si manifestano in Editor — e questo stesso
documento ne porta la prova storica: #2395, «la build Shipping è rotta su main», chiusa **stamattina**
(F5). Quella rottura è vissuta su `main` senza che nessun gate in Editor la vedesse.

Un fallimento packaged scoperto al quinto passo invalida il lavoro dei primi quattro. Un fallimento
packaged scoperto al primo passo costa una build.

📝 **Raccomandazione**: packaged si esegue **presto e si rompe presto**, perché ha il ciclo di feedback più
lungo. Non è la cerimonia finale: è il sensore che va acceso per primo.

---

## 3. Findings maggiori

### M-01 · «p50/p90 su almeno 10 partite» ha una soglia campionaria ma nessun oracolo — #166

**ADZIC.** Dieci partite dicono *quanto* misurare, non *cosa decide*. p50 e p90 di quale grandezza — il
tempo di apertura della finestra, la latenza fra trigger e prompt, la durata del countdown? E sotto quale
valore è `PASS`? Come scritta, la misura si esegue, produce due numeri, e non chiude nulla.

### M-02 · «completare *veramente* la UI FIRE/HOLD» ammette una dichiarazione precedente falsa e non dice cosa la distingue — #166

**ADZIC.** L'avverbio è il sintomo. Se serve *veramente*, allora esiste una versione che fu dichiarata
completa e non lo era. Il documento non dice quale predicato separa le due, quindi il criterio non esiste
e la voce non è eseguibile.

### M-03 · «giocabile/consegnabile» non nomina l'attore — titolo e verdetto

**COCKBURN.** Consegnabile *a chi*, e per fare cosa? Se il destinatario è un playtester interno, packaged
più G13 bastano. Se è un pubblico esterno, mancano voci che il documento non elenca affatto. Il verdetto
finale — *«dimostriamo che tutto ciò che abbiamo costruito arriva davvero al giocatore»* — è la
definizione corretta di v0.1, ma sta in fondo come morale invece che in testa come criterio.

### M-04 · La finestra `f339f537..HEAD` è invisibile al documento

**WIEGERS.** Venti PR mergiate e sei issue chiuse (F10) non compaiono. Fra queste, #2741 («il rifiuto di
bersaglio non arriva al giocatore») e #2759 («`ZoneBottom` è disegnata sotto il bordo dello schermo»)
appartengono esattamente all'area che il documento classifica 🟠 *indietro*. La valutazione può restare
corretta; il suo fondamento no.

### M-05 · I tre rossi non sono dello stesso genere e vanno separati — #2551 vs #2556

**CRISPIN.** #2551 ha causa nota, scritta nel titolo della issue — *«è nel manifest, non nel catalogo»* —
e rimedio meccanico: rigenerare l'asset con il commandlet che già esiste
(`RTBuildIconCatalogCommandlet`). #2556 richiede lavoro di gameplay su un comportamento appena deciso.
Trattarli come «due voci per tornare verdi» nasconde che una costa un comando e l'altra una feature.

### M-06 · «2231/2234» non è ancorata a un commit

**WIEGERS.** Il rapporto è l'unico numero falsificabile del documento, e non dice su quale `sha` è stato
prodotto né con quale filtro. Non l'ho eseguito (F9), quindi non lo contesto: osservo che non è
verificabile da nessuno tranne chi lo ha misurato, e solo finché ricorda quando.

---

## 4. Findings minori

- **m-01** · «slow-motion solo presentazionale» era marcato correttamente ed era l'unico punto in cui il
  documento difendeva il confine simulazione/presentazione. **Chiuso durante la stesura di questa
  revisione**: `83914a6f` — «feat(166): la slow-motion della finestra, e il suo ripristino e' strutturale»,
  con copertura in `RTReactionWindowViewModelTests.cpp`. Il confine è stato difeso nel codice, non solo
  nella lista.
- **m-02** · «reason code `NotTriggered`» e «`TimeoutReason = Abandoned`» sono le due voci di #166 già in
  forma verificabile: nominano un simbolo. Le altre cinque no.
- **m-03** · La tabella dei binding (widget / proprietà / funzione) è ben formata e resta utile come
  documentazione di ciò che è stato collegato, anche ora che il lavoro è chiuso.

---

## 5. Quello che va difeso dalla riscrittura

Quattro elementi dell'assessment sono corretti e il repository li conferma. Una riscrittura che li perde
peggiora il documento.

1. **La diagnosi centrale.** *«Motore/core abbastanza solido, UI e prova reale molto più indietro»* è
   confermata: le issue chiuse nella finestra invisibile al documento (#2741, #2759, #2760) sono tutte di
   presentazione e di arrivo del dato al giocatore.
2. **Il rifiuto della percentuale unica.** *«Non darei una percentuale unica, perché falserebbe la
   situazione»* è la scelta metodologicamente più solida del documento. Una media fra un core coperto e
   un'acceptance rossa non descrive nulla.
3. **«Mostrare il bersaglio corretto, oggi non ancora esprimibile dal DTO».** È la voce migliore di #166:
   nomina il difetto strutturale (il DTO non porta il campo) invece dello stato d'animo. È la forma da
   replicare sulle altre sei.
4. **La frase finale.** *«Non è più la fase "riscriviamo il sistema"; è la fase "dimostriamo che tutto
   arriva davvero al giocatore e funziona fuori dall'Editor"»* — è la definizione di v0.1 che manca in
   testa al documento.

---

## 6. Consenso del panel

**🤝 Convergenza.** Il giudizio qualitativo dell'assessment è corretto e il repository lo conferma. Il
difetto non sta nella diagnosi: sta nella **forma**. Uno stato dichiarato al presente invece che datato
(C-01), criteri non falsificabili (C-03), e una lista di priorità la cui voce numero uno una PR mergiata
tre ore prima aveva già consumato (C-02).

**⚡ Tensione produttiva — Nygard contro l'ordine proposto.** Il documento mette la UI prima della build
packaged perché la UI è *visibile*: se ne apprezza il progresso guardando. Ma packaged è l'unico gate che,
fallendo, invalida gli altri, e ha la prova storica di nascondere rotture (#2395). Wiegers e Crispin
appoggiano l'inversione. La tensione non si risolve dando ragione a uno dei due: si risolve osservando
che i due criteri ottimizzano cose diverse — *percezione di avanzamento* contro *costo di scoperta
tardiva* — e che in fase di acceptance vince il secondo.

**🕸️ MEADOWS — la struttura del difetto.** Il documento e il difetto che descrive hanno la stessa forma:
uno stato affermato senza ancoraggio a una misura datata, che invecchia in silenzio mentre continua a
essere letto come corrente. Non è un incidente isolato. È lo stesso difetto di **#2771**, chiusa oggi,
dove `roadmap-pia.md` §4 ha tenuto G12 come «già soddisfatto» per tre giorni dopo essere passato stantio.

> **Punto di leva**: non correggere questo documento. Correggere il **formato** di cui questo documento è
> un'istanza. Un assessment senza `sha` e timestamp nel corpo produrrà lo stesso difetto la prossima
> volta, con un autore diverso e la stessa buona fede.

**⚠️ Punto cieco collettivo.** Nessuna riga dell'assessment prevede *cosa fare quando la propria misura
invecchia*. Non è una svista di contenuto: è la ragione per cui il documento è nato già scaduto.

---

## 7. Ordine di lavoro riscritto sui fatti misurati

Sostituisce la sezione «Priorità corretta ora». Misurato su `9415d7de`.

| # | Voce | Stato rispetto al documento | Perché qui |
|---|---|---|---|
| — | ~~binding #2740~~ | **già fatto** (`baf0b95e`, PR mergiata `13:05Z`) | rimosso dalla lista |
| 1 | **#2551** — rigenerare `DA_IconCatalog` | invariato | causa nota, rimedio meccanico, sblocca un rosso su tre |
| 2 | **Build packaged** | risalito dal quinto posto | ciclo di feedback più lungo; rompe presto o invalida tutto dopo (C-05) |
| 3 | **#2556** | riclassificato | la decisione esiste ma non ha toccato test: serve lavoro su `Source/`, non un'altra decisione (C-04) |
| 4 | **T8 showcase (#2616)** | invariato | coreografia T5/T7, golden e oracoli |
| 5 | **Seduta Editor/PIE + G13 (#2620)** | invariato | vedi [`roadmap-pia.md`](../../roadmap/roadmap-pia.md) |
| 6 | **#166** | riclassificato, **una voce chiusa** | la slow-motion è stata implementata durante la stesura (`83914a6f`, m-01). Delle restanti: riscrivere ciascuna con il proprio oracolo **prima** di lavorarla; oggi solo il bersaglio non esprimibile dal DTO e i due reason code sono azionabili (M-01, M-02, m-02) |

---

## 8. Nota di metodo

Questa revisione è il **duale** di
[`spec-panel-td-handoff-2026-08-30.md`](spec-panel-td-handoff-2026-08-30.md), e vale la pena metterle
accanto.

Quella passata produsse due reperti falsi perché fu eseguita su un checkout **141 commit indietro**: il
punto di osservazione era stantio, e un punto di osservazione stantio non produce incertezza — produce
fatti falsi, ordinati e citabili. La sua errata concludeva che la misura mancante era una riga:
`git rev-list --count HEAD..origin/main`.

Qui i ruoli sono invertiti: il punto di osservazione della revisione è fresco, ed è il **documento
revisionato** a essere stantio. Ma la misura mancante è la stessa riga, applicata all'oggetto invece che
al soggetto — `git rev-list --count f339f537..HEAD` → `50`.

Ne segue la regola generale che nessuna delle due passate, da sola, avrebbe potuto formulare:

> La freschezza va misurata **da entrambi i lati**. Chi revisiona verifica dove si trova; chi è
> revisionato dichiara dove si trovava. Un solo controllo lascia scoperta metà del difetto, e la metà
> scoperta si presenta con le stesse credenziali di quella coperta.

---

## 9. Riverifica prima della pubblicazione — la regola applicata a sé stessa

Fra la stesura di questa revisione e il suo merge, `origin/main` è avanzato da `9415d7de` a `5a560500`:
**venti commit** (`git rev-list --count 9415d7de..5a560500`). Pubblicare senza rimisurare avrebbe
riprodotto esattamente C-01 — un documento che afferma uno stato al presente mentre lo stato si muove.

I reperti sono stati ricontrollati uno per uno.

| Reperto | Esito della riverifica su `5a560500` |
|---|---|
| **F7** — `DA_IconCatalog` non rigenerato (#2551) | ✅ **regge**. `git log 9415d7de..5a560500 -- Content/RT/UI/DA_IconCatalog.uasset` → vuoto. La priorità 1 di § 7 resta valida |
| **F8** — #2556, decisione senza test | ✅ **regge**. Issue `OPEN`, nessun commit sui test dei bot |
| **F6** — #166, #2616, #2620, #2697 aperte | ✅ **reggono**. Tutte `OPEN`. In particolare #2697: `cc5ca967` predispone la zona destra a ospitare il feed, ma il messaggio di commit stesso dichiara «**ma il feed e' ancora vuoto**» — predisposto non è consumato, e il finding regge |
| **m-01** — slow-motion presentazionale | ❌ **superato**. `83914a6f` la implementa con copertura in `RTReactionWindowViewModelTests.cpp`. Il testo di m-01 e la riga 6 di § 7 sono stati corretti prima del merge |
| C-01 … C-05, M-01 … M-06 | ✅ **reggono**. Nessuno dei venti commit tocca la forma dell'assessment revisionato |

**Ciò che il caso aggiunge alla nota di metodo.** La § 8 formula la regola; questa sezione la esegue, e nel
farlo mostra il costo reale — una riverifica di dieci minuti che ha invalidato **un** reperto su dodici.
Non è un tasso alto: è il punto. Il difetto non è che i documenti invecchino in fretta, è che invecchino
**in silenzio e a macchie**, lasciando intatti undici reperti su dodici e dando al dodicesimo le stesse
credenziali degli altri. Una revisione che si fida della propria freschezza perché *la maggior parte*
regge commette l'errore che C-03 contesta alla tabella di stato.
