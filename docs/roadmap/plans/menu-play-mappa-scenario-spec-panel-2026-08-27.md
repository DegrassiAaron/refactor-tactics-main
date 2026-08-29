# Menu, `PLAY` e la scelta di mappa e scenario — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **non applicata** — l'oggetto recensito non è un file versionato,
> e ciò che il panel propone chiede una `D-nnn` prima di qualunque diff ·
> **Due giri**: critique ([§1](#1-la-spec-sotto-esame)–[§6](#6-cosa-il-panel-non-ha-fatto)) e **decide**
> ([§7](#7-secondo-giro--le-cinque-decisioni-con-una-posizione), che prende posizione sulle cinque
> decisioni aperte e **corregge il peso dato a F1** dal primo giro) ·
> **Data**: 2026-08-27
> **HEAD della revisione**: `0323c4f3`, branch `claude/spec-panel-map-scenario-menu-gaybd3`, working tree pulito
> **Oggetto**: la richiesta *«voglio fare il menù e scegliere a play la mappa e lo scenario da testare»*,
> riscritta come specifica al [§1](#1-la-spec-sotto-esame) perché una frase non si recensisce.
> **Panel**: Wiegers (lead) · Cooper · Cockburn · Nygard · Adzic · Fowler
> **Modo**: `critique` (1° giro) → `decide` (2° giro) · **Focus**: requisiti + architettura + interaction design
> **Contesto documentale**: l'handoff *Menu/Frontend* del 2026-08-16 — **non** autorità, `CLAUDE.md` §4 —
> che vive in [`../../archive/src/handoff/2026-08-16-menu-frontend-tracking.md`](../../archive/src/handoff/2026-08-16-menu-frontend-tracking.md).
> *(Fino al 2026-08-30 questa riga puntava a `../../../Claude_RefactorTactics_Menu_Features_Issues_Tracking_v0.1_to_v1.0.md`,
> cioè alla **copia di radice**: era lo stesso testo dell'archiviato, rientrato per errore con un commit di
> massa sette ore dopo l'archiviazione, ed è stato rimosso.)* **E46** in
> [`../roadmap-v0.1.md`](../roadmap-v0.1.md), CP 46.4 (`#939`) e CP 46.6 (`#941`).

## 0. Cosa è questo documento

Il panel ha ricevuto una richiesta di **prodotto**, non una specifica: una riga sola, con dentro due
sostantivi (*mappa*, *scenario*) che in questo repository nominano più cose ciascuno. Il §1 è la richiesta
riscritta come spec — con le premesse rese esplicite — ed è **quello** che il panel ha recensito. Se il §1
non dice ciò che l'autore intendeva, il referto va riletto da lì: le premesse sbagliate sono l'unico posto
dove questo documento può essere fuori bersaglio senza saperlo.

Una regola prima di parlare, la stessa dei referti gemelli: **ogni premessa verificabile è stata misurata su
`0323c4f3`**, e nessun giudizio poggia su ciò che la richiesta dà per scontato. Sette premesse su nove non
reggono nella forma in cui erano implicite — e non perché la richiesta sia sbagliata, ma perché descrive
una funzione che il progetto ha già **tre volte**, in tre posti che non si parlano.

⚠️ **Il panel non ha scritto codice.** Il task è documentale (`CLAUDE.md` §3: panel → *«produci l'output
richiesto e non passare automaticamente al codice»*), e il §4 è una proposta, non un piano approvato.

## 1. La spec sotto esame

Il flusso richiesto, nella lettura letterale:

```text
MAIN
└── PLAY
     └── Setup
          ├── [ MAPPA    ▾ ]
          ├── [ SCENARIO ▾ ]
          └── START  →  la cosa scelta, da provare
```

Le premesse che quel disegno porta con sé, numerate perché il §2 le misura una per una:

| # | Premessa implicita |
|---|---|
| **P1** | Esiste un menu, con dentro un `PLAY` che oggi avvia qualcosa |
| **P2** | `PLAY` oggi non offre scelte, e il punto in cui aggiungerle è lui |
| **P3** | «Mappa» = uno dei livelli del progetto, e ce n'è più d'uno fra cui scegliere |
| **P4** | Il livello scelto decide la board su cui si gioca |
| **P5** | «Scenario» = una voce del catalogo `Scenarios/`, e sceglierne uno è oggi impossibile |
| **P6** | Il catalogo è disponibile al gioco, non solo all'editor |
| **P7** | Mappa e scenario sono **due** scelte indipendenti, che si combinano |
| **P8** | Avviare uno scenario e avviare una partita sono la stessa cosa con un parametro diverso |
| **P9** | È scope aperto: basta implementarlo |

## 2. Le premesse, misurate

Tutte le misure su `0323c4f3`, working tree pulito. I comandi sono al §2.1.

| Premessa | Misura | Esito |
|---|---|---|
| **P1** — c'è un menu con `PLAY` | `RTScreenIds` dichiara `Main · Settings · Error · Result · Pause · Match`; `StartMatch()` esiste da CP 46.4; **74** test `RefactorTactics.Frontend.*` distinti | ✅ |
| **P2** — `PLAY` non offre scelte | `URTFrontendNavigator::StartMatch()` **non prende parametri**; apre `MatchLevel`, costante di `DefaultGame.ini`; `FRTOnMatchRequested` porta **un** `FString LevelName` | ✅ |
| **P3** — più mappe fra cui scegliere | **4** `.umap` in tutto il progetto. Uno è il frontend stesso (`L_Frontend`); i tre restanti stanno **tutti** sotto `Content/RT/Maps/Dev/` | ⚠️ vera per due, e sono mappe di sviluppo |
| **P4** — il livello decide la board | ❌ **falso.** La board la decide `ERTMapSource` (`LevelAsset · GeneratedDemoArena · GeneratedTestArena`), scavalcabile da `rt.Map.Source`, a sua volta scavalcata da `rt.Map.Fixture`. Il livello è il **contenitore**; sul default l'asset del livello può essere buttato — è [#1267](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1267) | ❌ |
| **P5** — scegliere uno scenario è impossibile | ❌ **falso, ed è il punto.** Si sceglie già in **tre** modi: la property `ScenarioToRun` del GameMode (con tendina `GetOptions` e due filtri per tag), la console variable `rt.Test.Scenario`, il flag di riga di comando `-RTScenario=`. Nessuno dei tre è raggiungibile **dal gioco** | ❌ |
| **P6** — il catalogo arriva al gioco | ✅ `+DirectoriesToAlwaysStageAsUFS=(Path="../Scenarios")`; `ScenariosRoot()` = `ProjectDir()/Scenarios`, letta via `IFileManager`, quindi passa dal pak. **83** scenari, **49** tag distinti, indice con filtro a intersezione già scritto e **puro** | ✅ |
| **P7** — due scelte indipendenti | ❌ **falso.** Uno scenario **dichiara già la propria board**: `fixture` (19 file) oppure `mapRadius` (64) **con** `cells` (5) come modificatore. ⚠️ **La prima stesura scriveva «oppure» tre volte, e i suoi stessi numeri lo smentivano**: 19 + 64 + 5 = 88 su **83** scenari, cioe' cinque ne dichiarano due insieme. Rimisurato il 2026-08-28 sugli 88 di allora: `mapRadius` 62 · `fixture` 21 · `mapRadius + cells` 5 · `cells` da solo **mai**. Gli assi sono `fixture` **XOR** (`mapRadius` [`+ cells`]), e la differenza conta per chi deriva la colonna `Map` ([D-217](../../decisions/RT_PDR_00_Decision_Log.md)). Scegliere anche una mappa significa dare due risposte alla stessa domanda | ❌ |
| **P8** — scenario e partita sono la stessa cosa | ❌ **falso.** Con `ScenarioToRun` valorizzato `BeginPlay` esegue lo scenario **al posto** della partita normale: l'allestimento 2v2 non avviene affatto, e l'HUD mostra un banner apposta perché il sintomo non punterebbe alla causa | ❌ |
| **P9** — è scope aperto | ❌ **falso, ed è dichiarato in roadmap.** E46 dice: *«Le sezioni DEV/TEST del menu — Scenario Browser/Detail/Runner UI, Bot Simulation — restano **fuori** perché sono tooling già classificato `out_of_release_scope`»* ([D-144](../../decisions/RT_PDR_00_Decision_Log.md)) | ❌ |

Due premesse su nove reggono intere. **Nessuna delle sette che cadono è un fraintendimento dell'autore**:
sei nascono dal fatto che *mappa* e *scenario* sono omonimi di più assi (§3, F2), e la settima da una
decisione di scope che la richiesta, legittimamente, non sapeva di riaprire.

### 2.1 Le misure, per esteso

```sh
# P2 — la firma, e cosa viaggia
grep -n "ERTNavResult StartMatch\|FRTOnMatchRequested\|MatchLevel=" \
  Source/RefactorTactics/Frontend/RTFrontendNavigator.h Config/DefaultGame.ini
#  .h:60   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTOnMatchRequested, const FString&, LevelName);
#  .h:234  ERTNavResult StartMatch();          ← nessun parametro
#  .ini:63 MatchLevel=/Game/RT/Maps/Dev/L_HexArena/L_HexArena

# P3 — l'elenco completo dei livelli
find Content -name '*.umap'
#  Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap
#  Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap
#  Content/RT/Maps/Dev/L_Prototype/L_Prototype.umap
#  Content/RT/Maps/Shared/L_Frontend/L_Frontend.umap   ← è il menu, non una partita

# P4 — chi decide davvero la board: quattro autorità in fila
#  RTGameMode.h:66      ERTMapSource MapSource = ERTMapSource::LevelAsset;   (property, vive nel .uasset)
#  RTTestConsole.cpp:51 rt.Map.Source     scavalca la property
#  RTTestConsole.cpp:~  rt.Map.Fixture    «Vince su rt.Map.Source»
#  Scenarios/**.json    fixture | mapRadius | cells                          (quando gira uno scenario)

# P5 — le tre vie già esistenti, e la loro precedenza
grep -n "ScenarioToRun\|CVarRTTestScenario\|FromCommandLine" Source/RefactorTactics/RTGameMode.cpp
#  ResolveScenarioToRun(): rt.Test.Scenario  >  -RTScenario=  >  ScenarioToRun (property)

# P6/P7 — cosa dichiara davvero un file di scenario (83 file, chiavi top-level)
#  83 scenarioId · 83 tags · 83 version · 83 seed · 83 units · 83 expect · 78 turns
#  64 mapRadius  · 19 fixture · 5 cells · 5 freeRun
#  ZERO con un campo `map`, `level` o equivalente.
```

> 🔴 **Zero scenari su 83 dichiarano una mappa**, e la spec del Menu Browser
> ([handoff Menu/Frontend](../../archive/src/handoff/2026-08-16-menu-frontend-tracking.md) §5) chiede di mostrare **`Map`**
> in ogni riga dell'elenco. È una colonna che oggi non ha una fonte: o si deriva da `fixture`/`mapRadius`,
> o si aggiunge al formato — e sono due lavori diversi con due costi diversi.

## 3. Findings

### 🔴 F1 — La richiesta riapre una decisione, e va portata al Decision Log prima che a un branch

**WIEGERS**: E46 non ha omesso il Browser degli scenari: lo ha **escluso**, con una motivazione scritta
(*tooling già classificato `out_of_release_scope`*) e un ID di decisione ([D-144](../../decisions/RT_PDR_00_Decision_Log.md)).
Una richiesta che chiede esattamente quella funzione non è il riempimento di una lacuna, è la revoca di una
scelta — e trattarla come lacuna produce il caso peggiore: il lavoro entra, la roadmap continua a dire che è
fuori, e fra due settimane nessuno dei due documenti sa quale dei due vale.

**COCKBURN**: c'è anche l'attore da nominare. La richiesta dice *«da testare»*: chi la fa non è il giocatore
del vertical slice, è **l'autore che vuole guardare una regola**. E46 esiste per il primo. Sono due prodotti
nello stesso eseguibile, e la domanda di scope è *quale dei due riceve una voce nel menu spedito*.

**Cosa serve**: una `D-nnn` nuova che dica se le sezioni DEV/TEST rientrano, con quale visibilità
(Development-only o spedita) — letta dall'ultimo ID assegnato nel Decision Log e **riverificata prima del
merge**, come prescrive `CLAUDE.md` §4. Non applicato qui: non è una decisione del panel.

### 🔴 F2 — «Mappa» nomina cinque cose, e il menu deve dire quale sceglie

**NYGARD**: le misure del §2 danno cinque referenti distinti per la parola *mappa*, e sono tutti vivi
contemporaneamente:

| # | Cosa | Chi lo decide oggi |
|---|---|---|
| 1 | il `.umap` che si apre | `MatchLevel` in `DefaultGame.ini` |
| 2 | la sorgente della board | `ERTMapSource` (property del GameMode) |
| 3 | l'override della sorgente | `rt.Map.Source` |
| 4 | la fixture per nome | `rt.Map.Fixture` — vince sul 3 |
| 5 | la board dichiarata dallo scenario | il campo `fixture`/`mapRadius`/`cells` del `.json` |

Una tendina intitolata `MAPPA` che agisse sull'asse 1 sarebbe **la scelta che si vede senza essere quella
che conta**: `L_HexArena` e `L_DevSandbox` possono produrre la stessa arena generata, perché con
`MapSource` diverso da `LevelAsset` il contenuto del livello non entra nella partita. È già successo, ha già
un numero — [#1267](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1267) — e la controparte
d'editor lo dice esplicitamente: *«senza `rt.Map.Source = LevelAsset` il GameMode la butta all'avvio e a
schermo torna l'arena di prova»*.

**FOWLER**: il rimedio non è documentare l'ambiguità nella UI. È che la scelta esposta sia **l'asse 2+4**
(sorgente e fixture), che è ciò che cambia il gioco, e che l'asse 1 resti un dettaglio di caricamento. Se
un giorno esisteranno mappe d'autore vere, l'asse 1 diventerà interessante — ma allora sarà una scelta di
*contenuto*, con nomi di contenuto, non tre percorsi sotto `Maps/Dev/`.

### 🔴 F3 — Mappa **e** scenario è sovradeterminato: una delle due scelte deve perdere, e va detto quale

**NYGARD**: 19 scenari nominano una fixture, 64 dichiarano un raggio. Un menu che offra `[MAPPA] + [SCENARIO]`
crea una combinazione illegale per costruzione — `Spec.Map.BridgeBreaksThePath` **richiede** `TestArena`,
perché la sua asserzione poggia sull'unica transizione fra layer che quella fixture ha. Eseguirlo su
un'arena demo non lo fa fallire in modo interessante: lo fa fallire per il motivo sbagliato.

**COOPER**: e un menu che permette una combinazione illegale ha solo tre uscite, tutte peggiori del non
offrirla: la vieta a posteriori (l'utente ha scelto e viene contraddetto), la accetta e ignora una delle due
(la scelta che non conta, di nuovo), o la accetta e rompe. La forma corretta è che **selezionare uno
scenario spenga la scelta della mappa** mostrando quale board lo scenario porta con sé — informazione, non
controllo.

**ADZIC**: il criterio d'accettazione che rende questo verificabile senza aprire l'editor: *dato uno
scenario con `fixture`, la board effettiva a fine allestimento è quella fixture, qualunque cosa fosse
selezionata nell'altra tendina*. È headless, e l'harness lo sa già fare.

### 🔴 F4 — La scelta non attraversa `OpenLevel`, e oggi il canale trasporta una stringa sola

**FOWLER**: il confine è già progettato bene — il navigatore *chiede* e chi ha il mondo *apre* — ma il
messaggio che passa è `FString PendingMatchLevel`, e `ConsumePendingMatchLevel()` lo azzera. Uno scenario
scelto nel menu deve arrivare al `BeginPlay` del **livello successivo**, e il posto dove oggi vive quella
scelta è una `UPROPERTY` dentro il `.uasset` del GameMode: non la si scrive da runtime, e comunque
apparterrebbe al livello, non alla sessione.

**NYGARD**: la guardia `MatchRequestNotConsumed` è la parte da non perdere nel cambio. Esiste perché *«una
richiesta ancora pendente è la prova che nessuno l'ha consumata»*, ed è il difetto che vivrebbe
nell'**assenza** di una chiamata. Allargando il messaggio da `FString` a struct, la guardia deve continuare
a essere una domanda sola («c'è una richiesta pendente?»), non tre campi da controllare a mano.

**Proposta al §4.2.** Non applicata: tocca `RTFrontendNavigator` e `RTGameMode`, e nessun DoD lo chiede.

### 🟠 F5 — Sarebbe la **quarta** autorità sullo scenario, e la precedenza va dichiarata prima, non scoperta dopo

**WIEGERS**: `ResolveScenarioToRun()` implementa già una precedenza a tre — `rt.Test.Scenario` >
`-RTScenario=` > property — con la regola *«il più specifico vince»* e, cosa che va riconosciuta, **un
warning quando una sorgente ne scavalca un'altra**. Un menu è la quarta. Dov'è nell'ordine?

Il panel non lo decide, ma isola il caso che rende la domanda urgente: la CI e i playtest da riga di comando
usano `-RTScenario=`. Se il menu vince su tutto, un pacchetto lanciato con quel flag e poi toccato nel menu
esegue altro; se perde, chi sceglie nel menu di una build lanciata da uno script si vede ignorare la scelta.
Entrambe le risposte sono difendibili, **nessuna delle due è difendibile in silenzio**.

**ADZIC**: e il canale per dirlo esiste già ed è tipizzato — `ERTStartupOutcome` (14 valori) più il banner
d'avvio, nato apposta perché *«quelle condizioni finivano solo nel log»*. Un conflitto di precedenza è
esattamente una di quelle condizioni.

### 🟠 F6 — `PLAY` con due esiti diversi è una modalità nascosta

**COOPER**: il bottone `PLAY` oggi promette una cosa sola. Con una tendina scenario, lo stesso bottone porta
o a una partita 2v2 allestita da `ARTGameMode`, o a una run di scenario che **non allestisce la partita** e
scorre turni scritti in un file. Sono due prodotti dietro un'etichetta. L'utente che ha lasciato la tendina
su un valore di ieri preme `PLAY` e ottiene qualcos'altro — e il progetto ha già dovuto inventare un banner
a schermo (`GetScenarioBannerText`) proprio perché *«il sintomo non punta alla causa»*.

**COCKBURN**: il documento handoff alla radice l'aveva già risolto, e la sua struttura regge meglio della
richiesta letterale: `PLAY` e `SCENARIOS` sono **due voci di primo livello**, non due tendine dentro una.
Il panel raccomanda quella forma.

### 🟠 F7 — L'elenco delle mappe promette una varietà che non esiste

**COOPER**: due voci, entrambe sotto `Maps/Dev/`, di cui una è la sola che il progetto ha mai giocato. Una
tendina con due elementi non è una scelta: è un ornamento che invita a cercare la terza. Il codice del
progetto ha già scritto questa regola per i tag degli scenari — *«una categoria vuota nel menu è un invito
a cercare qualcosa che non c'è»* — e vale identica qui.

**Raccomandazione**: finché le mappe d'autore non esistono, l'asse mappa si espone come **fixture**
(`ArenaV01 · RelayBasin · RelayLite · TestArena · CoverYard` — cinque, e differiscono davvero: `RelayBasin`
è l'unica con otto superfici) e non come elenco di `.umap`.

### 🟠 F8 — La colonna `Map` del Browser non ha una fonte

**ADZIC**: la spec del Browser chiede per ogni riga `Scenario ID · Display Name · Category · Map · Mode ·
Expected turns · Last result · Run`. Misurato: `scenarioId` ✅ (83/83), `tags` ✅ (83/83, e sono già la
`Category` con due assi incrociabili), `Map` ❌ (0/83), `Display Name` ❌, `Mode` ❌, `Expected turns` —
derivabile da `turns`/`maxTurns` ma non dichiarato, `Last result` ❌ (nessuna persistenza di esito).

Metà delle colonne è un formato nuovo. Va detto adesso: è la differenza fra «esponiamo l'indice che c'è» e
«estendiamo il formato degli scenari», e solo la prima è piccola.

### 🟡 F9 — L'unico pezzo pronto è l'indice, e il rischio è che la UI se ne scriva un secondo

**FOWLER**: `URTScenarioIndex` è la parte del lavoro già fatta e fatta bene: `ReadHeader`, `BuildFrom`,
`ApplyRedirects` sono **pure**, si provano headless, `ListIds` filtra per intersezione di due tag, `ListTags`
deriva il vocabolario dai file invece di dichiararlo, e `ResolvePath` **rifiuta** un ID ambiguo invece di
sceglierne uno. Un widget che si costruisse una propria lista perderebbe tutte e quattro le proprietà.

**ADZIC**: criterio d'accettazione, quello che la spec handoff già chiede (*«non hard-coda una seconda lista
scenario»*), reso misurabile: **zero** letterali di `scenarioId` nel codice della UI, e l'elenco mostrato
uguale a `URTScenarioIndex::ListIds(filtroA, filtroB)` per ogni coppia di filtri — verificabile headless,
senza viewport.

### 🟡 F10 — L'errore ha già un canale tipizzato: non aprirne un secondo

**NYGARD**: un ID inesistente o ambiguo, una fixture sconosciuta, un file illeggibile sono i casi normali di
questa funzione, non l'eccezione — `BuildFrom` restituisce apposta un elenco di problemi *senza svuotare
l'indice*, perché «un file rotto non deve nascondere gli altri». Il modo giusto di mostrarli esiste:
`ERTStartupOutcome` + `DescribeOutcome` + `WBP_RT_ErrorModal`, con la regola già scritta nel codice — *«non
esiste un accessor che restituisca una `FString` libera da mostrare»*, perché una FString libera perde
localizzazione, filtro sui non fatali e `PhaseWhenArmed`. Servono valori nuovi nell'enum, non un secondo
canale.

### 🟡 F11 — Visibilità in Shipping: va decisa con la D-nnn di F1, non lasciata al `#if` di chi implementa

**WIEGERS**: la spec handoff dice *«`SCENARIOS` è una sezione DEV/TEST, anche se può essere visibile in
Development build»* — un «può» che non è un requisito. Le due letture (compilata fuori in Shipping, oppure
presente e nascosta) hanno costi di test diversi: la prima aggiunge una configurazione in cui il menu ha una
forma diversa da quella provata, e `RefactorTactics.Frontend.EveryConfiguredScreenLoads` itera le voci del
`.ini`. Va scelta con la decisione di scope, non dopo.

## 4. La forma che il panel propone

⚠️ **Proposta, non piano.** Nulla di qui sotto è approvato, e il §5 elenca ciò che va deciso prima.

### 4.1 Tre assi, tre nomi diversi

La richiesta ne nomina due e ne tocca tre. Separarli è la mossa che scioglie F2, F3 e F6 insieme:

| Asse | Domanda | Dove vive nel menu |
|---|---|---|
| **Cosa gioco** | una partita, o uno scenario da guardare? | `PLAY` **oppure** `SCENARIOS`: due voci, mai una tendina |
| **Su quale board** | quale terreno, con quali superfici | dentro `PLAY`: sorgente + fixture. Dentro `SCENARIOS`: **mostrata, non scelta** — la porta lo scenario |
| **Quale livello** | quale `.umap` caricare | **nessuna**: resta `MatchLevel`, dettaglio di caricamento |

### 4.2 Il contratto tecnico minimo

Il canale del navigatore smette di essere una stringa e diventa una richiesta:

```cpp
USTRUCT(BlueprintType)
struct FRTMatchRequest
{
    FString       Level;        // oggi: sempre MatchLevel
    FString       ScenarioId;   // vuoto = partita normale
    ERTMapSource  MapSource;    // ignorato quando ScenarioId è valorizzato
    FString       Fixture;      // idem — la fixture dello scenario vince (F3)
};
```

- `StartMatch(const FRTMatchRequest&)` sostituisce `StartMatch()`; `ConsumePendingMatchRequest()` sostituisce
  `ConsumePendingMatchLevel()`, con **la stessa** guardia `MatchRequestNotConsumed` su una richiesta sola
  (F4);
- `ARTGameMode::ResolveScenarioToRun()` acquisisce una quarta sorgente e la **dichiara** nell'ordine, con il
  warning che già emette quando una scavalca l'altra (F5);
- il conflitto scelta-vs-effettivo arriva a schermo dal `FRTStartupReport` che esiste, con valori nuovi di
  `ERTStartupOutcome` (F10).

**Costo dichiarato**: tocca `RTFrontendNavigator`, `RTFrontendGameMode`, `RTGameMode` — cioè il confine che
CP 46.4/46.6 hanno appena chiuso, e i **74** test `Frontend.*` che lo pinnano. Non è un widget in più.

### 4.3 Se si decide di farlo, in quest'ordine

| # | Fetta | Perché prima | Verificabile con |
|---|---|---|---|
| 1 | Il canale a struct, **senza UI**: `PLAY` continua a fare esattamente ciò che fa | è l'unico pezzo che tocca codice pinnato; passarlo da solo tiene il rischio separato dalla funzione nuova | i 74 `Frontend.*` restano verdi, più i test della nuova guardia |
| 2 | Voce `SCENARIOS` → elenco da `URTScenarioIndex::ListIds`, due filtri per tag | l'indice è pronto e puro; è la fetta con più valore e meno codice nuovo | zero letterali di ID nella UI; elenco == `ListIds` per ogni coppia di filtri (F9) |
| 3 | `Run` di uno scenario dal menu | dipende da 1 e 2 | uno scenario con `fixture` gira sulla propria board qualunque cosa sia selezionata altrove (F3) |
| 4 | Board scegliibile in `PLAY` (sorgente + fixture) | indipendente dalle altre tre; ha valore da sola per `PIE-V01-BOARD` | la board a fine allestimento è quella scelta, e il banner lo dice |
| 5 | Colonne ricche del Browser (`Display Name`, `Map`, `Expected turns`, `Last result`) | è **formato nuovo** (F8): costo diverso, decisione diversa | migrazione degli 83 file, o derivazione dichiarata |

Le fette 1–4 non estendono il formato degli scenari. La 5 sì, ed è la ragione per cui è ultima.

## 5. Decisioni aperte — da portare al Decision Log

> ✅ **Il [§7](#7-secondo-giro--le-cinque-decisioni-con-una-posizione) prende posizione su tutte e cinque**
> (secondo giro, stesso giorno, stesso HEAD), e **tutte e cinque sono poi state decise** nel Decision Log —
> `D-214`, `D-215`, `D-216`, `D-217`: vedi il [§7.8](#78-cosa-resta-allautore-e-cosa-il-panel-continua-a-non-fare).
> Questo elenco resta com'era scritto, perché è il punto da cui si è partiti. ⚠️ Il §7.0 corregge questo elenco in un punto — **la 1 e la 2 sono la stessa
> decisione**, e trattarle come due sopravvalutava la precondizione.

1. **Le sezioni DEV/TEST rientrano nella v0.1?** Revoca o conferma parziale di [D-144](../../decisions/RT_PDR_00_Decision_Log.md) / scope E46 (F1). Senza questa, le altre quattro non hanno oggetto.
2. **Visibilità**: compilata fuori in Shipping, o presente e nascosta? (F11)
3. **Precedenza**: dove sta la scelta del menu rispetto a `rt.Test.Scenario`, `-RTScenario=` e la property? (F5)
4. **L'asse mappa si espone come fixture o come `.umap`?** Il panel raccomanda fixture (F2, F7), ma è una scelta di prodotto.
5. **Il formato scenario si estende?** Solo se servono davvero le colonne di F8.

## 6. Cosa il panel non ha fatto

- **Nessun codice, nessun `.uasset`, nessuna issue.** Task documentale (`CLAUDE.md` §3), e F1 dice che
  l'implementazione ha una precondizione che non è del panel. ⚠️ Il [§7.1](#71-d1--scope--posizione-non-si-revoca-d-144-la-si-delimita-e-la-delimitazione-è-d2)
  ridimensiona quella precondizione: è una **delimitazione**, non una revoca, e non richiede di cambiare
  nemmeno una riga di roadmap.
- **Non ha verificato in editor** che `L_DevSandbox` e `L_Prototype` siano giocabili: i `.umap` sono binari e
  non si aprono da qui. Il §2 dice che esistono e dove stanno, non che siano mappe di partita.
- **Non ha misurato le issue aperte** su questo tema: `CLAUDE.md` §4 chiede `gh pr list --state open` prima
  del merge, e questa sessione non ha `gh`. Chi apre il lavoro cerchi i duplicati prima.
- **Non ha aggiornato la roadmap**: se la decisione di F1 passa, la riga di E46 che dichiara il Browser fuori
  scope va cambiata **lì**, che è il suo owner.

---

# 7. Secondo giro — le cinque decisioni, con una posizione

> **Data**: 2026-08-27, stesso giorno · **HEAD**: `0323c4f3`, invariato · **Panel**: lo stesso
> **Modo**: `decide` — il primo giro isolava le domande e dichiarava di non rispondere; qui il panel
> risponde, e registra il dissenso dove c'è. ⛔ **Le posizioni non sono decisioni**: il Decision Log è
> l'owner, e gli ID sono **D-214**, **D-215**, **D-216** e **D-217**.
> 🔴 **Questa riga diceva `D-210`, e la riverifica prescritta l'ha falsificata poche ore dopo.** Era il primo
> libero *su questo branch* (ultimo assegnato: `D-209`); nel frattempo `origin/main` è avanzato di **22
> commit** e ne ha presi tre — `D-210` (gerarchia delle fonti, `355ef055`), `D-211`, `D-212`. Rinumerata
> **prima del merge** con `git fetch --prune origin` e la lista delle PR aperte (**zero**), come
> `CLAUDE.md` §4 prescrive. 🔴 **E venticinque minuti dopo è successo di nuovo, stavolta per davvero**: `feat/1479-cleanse-vede-il-controllo` rivendicava `D-213` per una tesi diversa, e nessuna PR aperta lo mostrava — l'ha visto una scansione di **tutti i ref remoti**. ✅ Gli ID scritti sono quindi **`D-214`** (la delimitazione del §7.1) e **`D-215`** (l'asse mappa del §7.4); la nota di `D-214` nel Decision Log racconta entrambi i giri.

## 7.0 Prima: una correzione al primo giro

**WIEGERS**: il §3 ha letto D-144 **attraverso la roadmap**, non nel Decision Log. Letta per intero, la voce
dice due cose che cambiano il peso di F1:

1. le ragioni dell'esclusione erano **due, dichiaratamente indipendenti**, e la seconda — *«`Scenarios/` non
   è staged, quindi una UI che legge il catalogo reale in packaged non avrebbe catalogo»* — è **già caduta**
   ([#935](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935) e #945, con #926 chiusa il
   2026-08-15). D-144 lo registra da sé, barrata, e conclude: *«la decisione poggia sulla sola (a) […] chi
   la rilegge deve sapere che oggi ne ha **una**»*;
2. la (a) superstite non parla di *build*, parla di *release*: `RT-FEAT-UI-SCENARIO-BROWSER` porta
   `out_of_release_scope` con la motivazione **«serve a chi sviluppa, non è contenuto della release»**.

∴ **la richiesta e D-144 non rispondono alla stessa domanda.** D-144 dice cosa il giocatore riceve; la
richiesta chiede un'affordance per l'autore. Sono compatibili **se e solo se** la visibilità le tiene
separate — cioè **D1 e D2 sono una decisione sola**, e il primo giro le ha contate due. Il §3 F1 diceva
«revoca o conferma parziale»: la seconda metà era quella giusta, e il referto non le ha dato lo stesso peso.
La precondizione resta, ma è **una delimitazione, non una revoca**, e costa molto meno di quanto il §6
lasciasse intendere.

### 7.0.1 Le misure del secondo giro

| Domanda | Misura | Dove |
|---|---|---|
| Ultimo `D-nnn` assegnato | **D-209** sul branch · **D-212** su `origin/main` · **D-213** preso da un branch senza PR — gli ID scritti sono **D-214** e **D-215** | `RT_PDR_00_Decision_Log.md`, riletto su **tutti i ref remoti** |
| Esiste una CI che userebbe `-RTScenario=`? | **no**: `.github/` contiene solo `ISSUE_TEMPLATE` | `ls .github` |
| Quante `#if UE_BUILD_SHIPPING` nel codice? | **1 direttiva, 1 file** (`RTFrontendWidgets.cpp:164`) | `grep -rn '^\s*#if.*UE_BUILD_SHIPPING' Source/` |
| Versioni di formato scenario coesistenti | **quattro**: v1 × 74 · v2 × 3 · v3 × 1 · v4 × 5 | parsing degli 83 `.json` |
| Esiste un elenco autorevole di fixture? | **sì**: `URTMatchSetupLibrary::KnownFixtureIds()`, derivato dalla stessa tabella che le dispaccia (**5**) | `RTMatchSetupLibrary.cpp:431-449` |
| Esiste una persistenza degli esiti? | **sì**: `rt.Test.Run` scrive `Saved/RTTests/<Id>/<Run>/result.json` | `RTTestConsole.cpp:224` |
| `feature-registry.yaml` in questo albero | **assente** — `out_of_release_scope` è verificabile solo attraverso la citazione di D-144 | `find . -name '*.yaml'` |

> 🔴 **Il commento di `GetDetail()` dice *«il progetto usa già questa guardia in dodici file»*, e i file sono
> uno.** Misurato: una sola direttiva `#if UE_BUILD_SHIPPING` in tutto `Source/`; le altre tre occorrenze
> sono prosa che descrive la guardia **di Unreal** in `DeviceProfileManager.cpp`. Non cambia che il pattern
> sia giusto — cambia che chiamarlo *pattern stabilito del progetto* sia un argomento più debole di quanto
> il commento faccia credere. È la stessa classe di difetto che questo panel ha trovato al §2: un numero in
> un commento che nessun gate confronta con la realtà.

## 7.1 D1 — Scope · **Posizione: non si revoca D-144. La si delimita, e la delimitazione è D2**

**Unanime.**

D-144 (a) esclude il Browser dal **contenuto di release**. Una voce di menu che non raggiunge il giocatore
non è contenuto di release, quindi non contraddice nulla: la `D-214` che serve non dice *«il Browser entra
in v0.1»*, dice *«un'affordance di sviluppo può vivere nel frontend spedito purché non sia raggiungibile in
Shipping, e `out_of_release_scope` continua a valere su ciò che il giocatore riceve»*.

**COCKBURN**: e va nominato l'attore, perché è ciò che rende la delimitazione verificabile invece che
retorica: **l'autore in sessione di sviluppo**. Non «il tester», non «il giocatore curioso». Il criterio di
accettazione della delimitazione è che in una build Shipping quell'attore non esista, e la voce nemmeno.

⚠️ **Trappola già segnalata da D-144 punto (3), e va ripetuta qui perché è il tipo di errore che si fa una
volta sola ma costa**: `RT-FEAT-UI-SCENARIO-BROWSER` **non è libero** — è l'indice C++ di
[`#209`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/209), quello del §2 P6, e nel registry
risulta `INTEGRATED`. Chi cercasse lì lo stato del widget concluderebbe che è fatto. Le feature nuove vanno
sotto `RT-FEAT-UI-FRONTEND-*`.

**Costo della posizione**: una voce nel Decision Log. Zero righe di roadmap cambiate — E46 continua a dire il
vero, perché continua a essere vero.

## 7.2 D2 — Visibilità · **Posizione: una sola forma di menu, e il CONTENUTO compilato fuori in Shipping**

**Maggioranza 5–1** (dissenso di Cooper al §7.7).

Le due opzioni non sono simmetriche, e il progetto ha già scelto una volta su un caso identico:

```cpp
// RTFrontendWidgets.cpp:164 — il DETTAGLIO tecnico dell'error modal
#if UE_BUILD_SHIPPING
    // «In Shipping il dettaglio NON ESISTE, non e' nascosto: e' la differenza fra una guardia nel
    //  Blueprint — che qualcuno puo' dimenticare — e una stringa che non arriva.»
    return FString();
#else
    return Detail;
#endif
```

**FOWLER**: la forma è esattamente trasportabile. Il widget non cambia forma fra configurazioni; cambia
**cosa gli risponde il C++**, e una funzione — `ShouldShowDetails()` — decide se il pulsante esiste. Per il
menu: la voce `SCENARIOS` è dichiarata in `DefaultGame.ini` in ogni configurazione, `URTScenarioIndex` non
risponde in Shipping, e uno `ShouldShowScenarios()` collassa la voce.

**NYGARD**: il motivo tecnico per cui la simmetrica è peggiore è misurabile.
`RefactorTactics.Frontend.EveryConfiguredScreenLoads` **itera le voci del `.ini`**: togliere la riga in
Shipping produce una configurazione in cui il menu ha una forma che nessun test ha mai visto, e la scoperta
avverrebbe sul pacchetto. Tenere la riga e svuotare il contenuto lascia **una** forma provata.

**ADZIC**: criteri, entrambi headless:
1. `URTScenarioIndex::ListIds("","")` restituisce **vuoto** sotto `UE_BUILD_SHIPPING`, e non per una guardia
   nel widget;
2. la voce `SCENARIOS` è fra le registrate in **tutte** le configurazioni — cioè `EveryConfiguredScreenLoads`
   non cambia numero.

## 7.3 D3 — Precedenza · **Posizione: la scelta del menu vince su tutto, e la regola era già scritta**

**Unanime nel merito, con una riserva di Nygard sulla forma** (§7.7). ✅ **Decisa**: [`D-216`](../../decisions/RT_PDR_00_Decision_Log.md) — e la riserva di Nygard è entrata nella voce come vincolo, non come nota: il banner deve **nominare il flag scavalcato**.

Il panel non introduce un ordine: ne **applica** uno che il codice dichiara. `RTGameMode.cpp:30-38`:

```text
proprieta' del GameMode  <  -RTScenario=<Id>  <  rt.Test.Scenario
(configurazione            (intento di          (intento di adesso, e si puo'
 persistente)               questo avvio)        digitare a meta' sessione)
```

> *«La regola e' quella di sempre — il piu' specifico vince — applicata al **TEMPO**: la console si puo'
> digitare dopo l'avvio, quindi deve poter scavalcare cio' che l'avvio aveva chiesto. Se fosse il contrario,
> un flag di riga di comando renderebbe impossibile cambiare scenario senza riavviare.»*

**WIEGERS**: una scelta fatta nel menu è un intento espresso **dopo** l'avvio, da una persona, dentro la
sessione. Sull'asse che il progetto ha già scelto — il tempo — è la più specifica che esista. Metterla sotto
la console produrrebbe letteralmente il difetto che quel commento dice di voler evitare, con la console al
posto del flag.

**L'argomento contrario è più debole di quanto sembri, e va detto perché sembra forte**: «la CI usa
`-RTScenario=`, e un menu che vince la rompe». Misurato: **non c'è una CI** in questo albero (`.github/`
contiene solo `ISSUE_TEMPLATE`). E anche se ci fosse, una CI non apre menu: il conflitto richiede un umano
che scelga in una build lanciata da uno script, e in quel caso *l'umano che ha appena scelto* è l'intento
più recente.

**FOWLER**: e l'estensione è a costo quasi nullo, perché il punto singolo esiste già. `RTScenarioEntry`
dichiara `enum class EWinner { Property, CommandLine, ConsoleVariable }` e una `Winner()` sola, con la
motivazione scritta: *«il log dell'auto-run e la banda a schermo dicono entrambi la fonte […] prima
calcolavano la risposta ciascuno per conto proprio, con due ternari identici: aggiungere una terza sorgente
li avrebbe fatti divergere»*. Aggiungere `MenuRequest` in testa a `Winner()` fa seguire log e banner da soli.
⚠️ Esiste un gemello identico — `RTAutobattleEntry`, stessa forma e stesso ordine — e chi tocca l'uno
guardi l'altro: una sola delle due catene estesa è un'asimmetria che nessun test vede.

## 7.4 D4 — L'asse mappa · **Posizione: fixture. E l'obiezione che il panel si aspettava è già chiusa**

**Unanime.** ✅ **Decisa**: [`D-215`](../../decisions/RT_PDR_00_Decision_Log.md) porta questa posizione nel Decision Log, con l'avvertenza che non si applica a una run di scenario — lo scenario costruisce la propria board senza passare da `ApplyMapSource`.

Il primo giro raccomandava le fixture per due ragioni di prodotto (F2: il `.umap` non decide la board; F7: due
voci non sono una scelta). Il secondo giro cercava l'obiezione tecnica — *«un elenco di fixture nella UI è la
seconda lista di F9, scritta a mano»* — e **non regge**, perché l'antidoto esiste già:

```cpp
// RTMatchSetupLibrary.cpp:440 — l'elenco DERIVA dalla tabella che dispaccia, non la affianca
TArray<FString> URTMatchSetupLibrary::KnownFixtureIds();   // RelayBasin · RelayLite · TestArena
                                                           // ArenaV01 · CoverYard
```

`RTHexMapActor.h:186` lo dichiara autorità in prosa — *«l'autorità è `KnownFixtureIds()`, non questa riga:
è un tooltip»* — ed è stato chiuso in un posto solo da
[#1459](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1459) esattamente per questo motivo.

∴ l'asse mappa ha **la stessa forma** dell'asse scenario: una funzione C++ che deriva l'elenco dalla fonte,
una UI che la chiama, zero letterali. `ListTags()` e `KnownFixtureIds()` sono lo stesso pattern applicato due
volte, e il menu userebbe entrambe.

**COOPER**: e sono **cinque** voci che differiscono davvero — `RelayBasin` porta otto superfici, `TestArena`
l'unica transizione fra layer, `CoverYard` la copertura alta. Contro due `.umap` sotto `Maps/Dev/`.

**Effetto collaterale che vale da solo**: `PIE-V01-BOARD` oggi richiede
`-dpcvars=rt.Map.Source=LevelAsset` su un pacchetto, e
[#1267](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1267) esiste perché senza quel flag il
GameMode butta la mappa d'autore. Una tendina fixture nel menu rende quella verifica manuale **eseguibile
senza riga di comando** — cioè la fetta 4 del §4.3 ha valore anche se nessuna delle altre viene fatta.

## 7.5 D5 — Formato scenario · **Posizione: zero campi obbligatori nuovi. Sei colonne su sette sono derivabili**

**Maggioranza 5–1** (dissenso di Adzic al §7.7, e il panel gliene concede metà). ✅ **Decisa**: [`D-217`](../../decisions/RT_PDR_00_Decision_Log.md), che porta anche la mezza concessione ad Adzic (`displayName` opzionale) e un'avvertenza che il §7.5 non aveva isolato — `Last result` legge `Saved/`, che **non è versionato**: è diagnostica per-macchina, e nessun gate può appoggiarcisi.

Il primo giro diceva *«metà delle colonne è un formato nuovo»* e stimava una migrazione di 83 file. **Il
secondo giro ha misurato, e la stima era pessimistica in due modi.**

Primo: il formato **già convive con quattro versioni** — v1 × 74, v2 × 3, v3 × 1, v4 × 5. Aggiungere un
campo non ha mai significato migrare i file esistenti, in questo progetto. Secondo, e più importante: le
colonne hanno quasi tutte una fonte che non è il `.json`.

| Colonna chiesta dalla spec §5 | Fonte | Campo nuovo? |
|---|---|---|
| `Scenario ID` | `scenarioId`, 83/83 | no |
| `Category` | `tags`, 83/83, **due assi incrociabili** — già più espressiva di una categoria | no |
| `Map` | `fixture` → il nome; `mapRadius` → «arena generata r=N»; `cells` → «+N celle modificate» | no, **derivata** |
| `Expected turns` | `turns.Num()`, oppure `maxTurns` per i 5 `freeRun` | no, derivata |
| `Last result` | `Saved/RTTests/<Id>/<Run>/result.json`, che `rt.Test.Run` **già scrive** | no |
| `Mode` | la v0.1 ha una modalità sola: una colonna costante è rumore | no — **si toglie** |
| `Display Name` | derivabile da `scenarioId`, ma male (§7.7) | **l'unico caso aperto** |

**NYGARD**: e la ragione di principio per preferire la derivazione non è il costo, è la **divergenza**. Un
campo `map` scritto a mano accanto a un `fixture` che decide davvero la board sono due verità sullo stesso
fatto, ed è la famiglia di difetti che questo referto ha già trovato tre volte oggi — le cinque «mappe» di
F2, i due elenchi di F9, i dodici file di §7.0.1.

## 7.6 Cosa cambia nel §4.3

Le cinque fette restano, l'ordine pure. Cambia il **costo dichiarato** di due di esse:

| Fetta | Prima | Dopo il secondo giro |
|---|---|---|
| 1 · canale a struct | invariata | invariata — resta la sola che tocca codice pinnato |
| 2 · voce `SCENARIOS` + elenco | invariata | **+ `ShouldShowScenarios()` e l'indice vuoto in Shipping** (D2) |
| 3 · `Run` dal menu | invariata | **+ `EWinner::MenuRequest` in testa a `Winner()`** (D3), e il gemello autobattle guardato |
| 4 · board in `PLAY` | «indipendente» | **promossa**: `KnownFixtureIds()` è pronta, e sblocca `PIE-V01-BOARD` senza riga di comando (D4) |
| 5 · colonne ricche | «formato nuovo, costo diverso» | **ridimensionata a derivazione**, tranne `Display Name` (D5) |

## 7.7 Il dissenso, in chiaro

**COOPER, su D2** — *«una voce che collassa è una voce che qualcuno ha visto in uno screenshot»*. Il modello
`GetDetail()` funziona per un **pulsante dentro un modale d'errore**, che nessuno fotografa; una voce di
primo livello del menu principale entra in ogni cattura di schermo e in ogni video. Preferirebbe che
l'affordance non stia nel menu principale ma dietro un ingresso che il giocatore non incontra — una console
command, o una schermata raggiunta da tastiera. **Il panel non lo segue** perché l'ingresso nascosto è
esattamente ciò che esiste già (`rt.Test.Run`) ed è il motivo per cui la richiesta è stata fatta. Ma la
riserva resta registrata: se in v0.5 esisterà un pubblico, la voce va spostata prima, non dopo.

**NYGARD, su D3** — accetta l'ordine, non il silenzio. Se il menu scavalca un `-RTScenario=` presente sulla
riga di comando, il banner d'avvio deve **nominare il flag scavalcato**, non limitarsi a dire quale scenario
sta girando: chi ha lanciato con quel flag deve capire perché non sta ottenendo ciò che ha chiesto. È la
stessa forma dei due warning che `ResolveScenarioToRun` già emette, e non è opzionale.

**ADZIC, su D5** — `Display Name` derivato da `scenarioId` produce
`Spec.Facing.TurningPathUsesLastCompletedStep` in una lista di 83 righe, ed è illeggibile. **Il panel gli
concede metà**: un `displayName` **opzionale** è ammesso — le quattro versioni coesistenti dimostrano che
non costa una migrazione, e i file senza il campo ripiegano sull'ultimo segmento dell'ID. Resta vietato
renderlo **obbligatorio**, che è ciò che trasformerebbe la fetta 5 in un lavoro sugli 83 file.

## 7.8 Cosa resta all'autore, e cosa il panel continua a non fare

Il panel ha una posizione su tutte e cinque. **Nessuna è una decisione**: le decisioni le prende il Decision
Log, non questo documento, che non è owner di niente (`plans/README.md`).

✅ **Sono state prese tutte e cinque**, in [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md),
e in **quattro** voci separate perché sono decisioni separate:

| Voce | Cosa decide | §  |
|---|---|---|
| `D-214` | la **delimitazione**: un'affordance di sviluppo vive nel frontend spedito purché in Shipping non esista — e la forma è quella di `GetDetail()`. **E46 non è toccata** | §7.1 + §7.2 |
| `D-215` | l'**asse mappa**: fixture, non `.umap`. `MatchLevel` resta un dettaglio di caricamento | §7.4 |
| `D-216` | la **precedenza**: il menu vince, e il banner nomina il flag scavalcato | §7.3 |
| `D-217` | il **formato**: derivazione, zero campi obbligatori, `displayName` opzionale | §7.5 |

⛔ **Quello che nessuna delle quattro fa è autorizzare l'implementazione**, e lo dicono tutte: il canale del
navigatore trasporta un `FString`, quindi una scelta fatta nel menu **non attraversa `OpenLevel`** (§3 F4).
La fetta 1 del §4.3 resta la precondizione tecnica, come lo era prima che ci fosse una decisione.

Restano fuori, e restano vere dal primo giro:

- **nessun codice**, e ora anche nessuna riga di roadmap: la posizione su D1 è *scelta apposta* per non
  richiederne;
- ✅ **fatto dopo la stesura, ed è servito**: `git fetch --prune origin` e la lista delle PR aperte (zero,
  letta dal server) hanno mostrato che `D-210` non era più libero — vedi il banner del §7 e la nota di
  `D-214` nel Decision Log. Le issue aperte sul tema restano **non cercate**: niente `gh` in questa
  sessione, e chi apre il lavoro cerchi i duplicati prima;
- **`L_DevSandbox` e `L_Prototype` restano non aperti**, ed è ora meno rilevante: la posizione su D4 dice
  che l'asse `.umap` non va esposto affatto.
