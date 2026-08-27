# Menu, `PLAY` e la scelta di mappa e scenario — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **non applicata** — l'oggetto recensito non è un file versionato,
> e ciò che il panel propone chiede una **decisione di scope** prima di qualunque diff ·
> **Data**: 2026-08-27
> **HEAD della revisione**: `0323c4f3`, branch `claude/spec-panel-map-scenario-menu-gaybd3`, working tree pulito
> **Oggetto**: la richiesta *«voglio fare il menù e scegliere a play la mappa e lo scenario da testare»*,
> riscritta come specifica al [§1](#1-la-spec-sotto-esame) perché una frase non si recensisce.
> **Panel**: Wiegers (lead) · Cooper · Cockburn · Nygard · Adzic · Fowler
> **Modo**: critique · **Focus**: requisiti + architettura + interaction design
> **Contesto documentale**: `../../../Claude_RefactorTactics_Menu_Features_Issues_Tracking_v0.1_to_v1.0.md`
> (handoff, **non** autorità — `CLAUDE.md` §4), **E46** in [`../roadmap-v0.1.md`](../roadmap-v0.1.md),
> CP 46.4 (`#939`) e CP 46.6 (`#941`).

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
| **P7** — due scelte indipendenti | ❌ **falso.** Uno scenario **dichiara già la propria board**: `fixture` (19 file) oppure `mapRadius` (64) oppure `cells` (5). Scegliere anche una mappa significa dare due risposte alla stessa domanda | ❌ |
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
> (`Claude_RefactorTactics_Menu_Features_Issues_Tracking_v0.1_to_v1.0.md` §5) chiede di mostrare **`Map`**
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

## 5. Decisioni aperte — da portare al Decision Log, non risolte qui

1. **Le sezioni DEV/TEST rientrano nella v0.1?** Revoca o conferma parziale di [D-144](../../decisions/RT_PDR_00_Decision_Log.md) / scope E46 (F1). Senza questa, le altre quattro non hanno oggetto.
2. **Visibilità**: compilata fuori in Shipping, o presente e nascosta? (F11)
3. **Precedenza**: dove sta la scelta del menu rispetto a `rt.Test.Scenario`, `-RTScenario=` e la property? (F5)
4. **L'asse mappa si espone come fixture o come `.umap`?** Il panel raccomanda fixture (F2, F7), ma è una scelta di prodotto.
5. **Il formato scenario si estende?** Solo se servono davvero le colonne di F8.

## 6. Cosa il panel non ha fatto

- **Nessun codice, nessun `.uasset`, nessuna issue.** Task documentale (`CLAUDE.md` §3), e F1 dice che
  l'implementazione ha una precondizione che non è del panel.
- **Non ha verificato in editor** che `L_DevSandbox` e `L_Prototype` siano giocabili: i `.umap` sono binari e
  non si aprono da qui. Il §2 dice che esistono e dove stanno, non che siano mappe di partita.
- **Non ha misurato le issue aperte** su questo tema: `CLAUDE.md` §4 chiede `gh pr list --state open` prima
  del merge, e questa sessione non ha `gh`. Chi apre il lavoro cerchi i duplicati prima.
- **Non ha aggiornato la roadmap**: se la decisione di F1 passa, la riga di E46 che dichiara il Browser fuori
  scope va cambiata **lì**, che è il suo owner.
