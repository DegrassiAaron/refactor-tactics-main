=== RT3 CONTRIB ===

FROM:            DEV-TEST:b1
TO:              DEV-LEAD
FEATURE:         ParseCell — arita' limitata e ramo di rifiuto misurato
WAVE_ID:         parsecell-arity/1
CREATED:         2026-09-05 20:05
SUPERSEDES:      none
BASE_SHA:        39f3ec95
PRODUCED_SHA:    = BASE_SHA (non committo: consolida DEV-LEAD)
WORKTREE:        uncommitted
SEED_SOURCE:     none — nessun RNG nel percorso coperto
ASSIGNED_SCOPE:  Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
WRITE_SET:       Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
                 docs/rt-three-terminals/waves/parsecell-arity/contrib/DEV-TEST-b1-01.md (questo contributo)

Nessun file di `Scenarios/` toccato, nessun file di `ScenarioHarness/` toccato, nessun `.uasset`/`.umap`.

## AUTOMATION

Due test nuovi e una fixture esistente migrata.

### 1. `FRTScenarioLoaderCellArityTest` — `RefactorTactics.Scenario.LoaderRejectsCellArityOtherThanThree`

Il dominio dell'arita' su un chiamante solo (`units`), esplorato da entrambi i lati.

| Blocco | Ingresso | Attesa |
|---|---|---|
| 1 | `"cell": [0, 0, 0]` | ACCETTATO, e `Units[0].Cell == (0,0,0)` |
| 2 | `[]`, `[0]`, `[0,0]`, `[0,0,0,0]`, `[0,0,0,0,0]` | rifiutato; il motivo nomina la lunghezza TROVATA **e** quella attesa |
| 3 | campo `cell` assente | rifiutato; il motivo nomina l'unita' (`ALFA`), **non** «trovati 0» |
| 4 | `fixture: TestArena`, `"cell": [0, 0, 1]` | ACCETTATO, e il layer arriva `1` |

⚠️ **`mapRadius: 6` e id senza cifre non sono arredamento.** L'assertion legge un NUMERO dentro il
messaggio, e quel numero deve poter arrivare da un posto solo: con `mapRadius: 3` un errore che
nominasse il raggio farebbe passare «nomina la lunghezza attesa» senza che la nomini. Sei non e'
nessuna delle arita' provate; `ALFA` non porta cifre echeggiabili.

🔑 **Il blocco 4 e' l'anti-vacuita' del caso felice.** Il blocco 1 usa `[0, 0, 0]`, e su quella cella
un parser che ignorasse del tutto il terzo elemento — scrivendo sempre `layer = 0` — darebbe lo
stesso risultato. Serve un `layer != 0` che arrivi intatto, e serve la fixture perche' su un'arena a
raggio il layer 1 non esiste (`#1796`).

⛔ **Il blocco 3 non asserisce «trovati 0».** Un campo che manca del tutto non ha lunghezza; dirgli
zero direbbe una cosa falsa su un array che non c'e'. Cio' che si asserisce e' che il rifiuto sia
ATTRIBUIBILE, con la convenzione che questo file usa gia' altrove («il motivo nomina l'unita'»).

### 2. `FRTScenarioLoaderCellArityEveryCallSiteTest` — `RefactorTactics.Scenario.EveryCellFieldRejectsWrongArity`

L'acceptance criterion 5 scritto come misura. Sette blocchi, uno per chiamante, ciascuno con
**controprova a tre elementi** e rifiuto a quattro:

| # | Chiamante | Dove si inietta `[0, 0, 0, 0]` |
|---|---|---|
| 1 | `cells` | cella modificata |
| 2 | `interiorWalls` | cella del muro (forma copiata da `Combat.BlockedByInteriorWall`) |
| 3 | `doors` | cella della porta (forma copiata da `Spec.Map.InteractClosesOpenDoor`, l'unica del corpus senza binding) |
| 4 | `units` | cella dell'unita' |
| 5 | passi di `move` | il passo del percorso |
| 6 | `expect UnitAtCell` | cella dell'assertion |
| 7 | unita' delle `variants` | cella della variante |

⛔ **La controprova non e' cerimonia.** Le fixture di `interiorWalls` e `doors` sono costruite a mano
qui dentro: senza un caricamento riuscito con `[0, 0, 0]`, un rifiuto potrebbe arrivare da un mio
refuso in un campo vicino e il test direbbe «l'arita' e' controllata» avendo misurato che ho scritto
male un muro.

⚠️ Nel blocco 7 la cella buona **non** puo' essere `[0, 0, 0]`: una variante che non sposta nulla e'
rifiutata a monte, e la controprova cadrebbe per quel motivo invece che per l'arita'.

### 3. Fixture migrata — `ScenarioLoaderValidJson` / `FRTScenarioLoaderValidTest`

`B1` portava `"cell": [2, 0]` e l'assertion diceva *«cella a due componenti -> layer 0»*: era la
riga che garantiva la forma che l'arbitrazione DEV-LEAD ha appena vietato. Migrata a `[2, 0, 0]`,
con il motivo scritto accanto (🔴 datato 2026-09-05).

🔑 **L'assertion non e' stata riparata aggiungendo uno zero: e' stata SPOSTATA.** Tenerla qui con
tre elementi ne avrebbe conservato la forma perdendone il mestiere — su `[2, 0, 0]` un parser che
ignorasse il terzo elemento darebbe lo stesso risultato. La coppia che il layer lo misura davvero
(tre elementi con `layer != 0` intatto, e due elementi rifiutati) sta nel test 1.

Misurato: era l'**unica** occorrenza a due elementi in tutto `Source/`. La migrazione e' quella riga
e nient'altro.

## SCENARIOS

**Nessuno.** Gli scenari malformati sono costruiti **inline** nel test, come fa il resto del file
(`Rejects`, `Load`, `Carica`, `Try`): un file malformato in `Scenarios/` sarebbe raccolto da
`RefactorTactics.Scenario.ShippedScenariosAreValid`, che pretende che ogni file versionato sia
valido, e romperebbe l'acceptance criterion 6.

`git grep -c '"cell"' Scenarios/` resta invariato per costruzione: non ho aperto nessun file del
corpus in scrittura.

## VALIDATORS

**Nessuno.** La validazione di dominio vive gia' in `ParseCell` (scope DEV-MAIN) e nel test sul
corpus. Non ho aggiunto validator in `tools/`: sarebbe una seconda tabella da tenere allineata alla
prima, cioe' il difetto che questo file combatte altrove.

## DIMENSIONS COVERED

| Dimensione | Stato | Dove / perche' |
|---|---|---|
| SUCCESS | COVERED | test 1 blocco 1 e blocco 4; le sette controprove del test 2 |
| INVALID | COVERED | test 1 blocco 2: rifiuto **letto nel messaggio**, non nel solo bool |
| BOUNDARY | COVERED | arita' 0, 1, 2, 3, 4, 5 e array assente; sette chiamanti a quattro elementi |
| FALLBACK | COVERED | test 1 blocco 4 (il layer si legge, non si deduce) + il rifiuto stesso di `[q,r]` e `[q,r,l,x]`: un ripiego avrebbe prodotto un caricamento RIUSCITO |
| DETERMINISM | **N/A** | write-set: il mio file esercita il solo `LoadFromString`. A valle di `ParseCell` arriva un `FRTCellId` gia' formato e il resolver non e' nel write-set di nessuno dei due ruoli — §8 punto 3 non tira dentro il resolver perche' nessuno lo tocca. Nessun RNG, nessun seed nel percorso |
| ORDERING | **N/A** | write-set: una chiamata a `ParseCell` produce al massimo un esito, e il percorso non itera `TMap`/`TSet`. Gli array JSON si leggono in ordine di file, che e' l'ordine del dato, non un tie-break |
| SERIALIZATION | **N/A** | write-set: il roundtrip e' writer→loader, e `RTScenarioWriter.cpp` non e' nel write-set di questa wave ne' nel mio scope assegnato (il suo test owner e' `RTScenarioWriterTests.cpp`). `WriteCellArray` scrive tre valori incondizionatamente, quindi il formato prodotto resta caricabile — **letto, non eseguito**; se DEV-LEAD vuole un gate sul roundtrip, la sede e' quel file |
| TURNLOG | **NONE** | contratto dell'handoff: il parsing precede la partita e non e' osservabile in essa. Nessun evento da asserire |
| REPLAY | **N/A** | write-set: nessun formato di replay toccato; a valle arriva una cella gia' formata |
| NETWORK | **N/A** | write-set: il caricamento e' locale e precede la partita; nessuna authority di simulazione coinvolta |
| PRIVACY | **N/A** | write-set: nessun dato di squadra attraversa `ParseCell`. Nessun canary da piazzare, e piazzarne uno qui misurerebbe un percorso che non esiste |

Nessuna dimensione omessa in silenzio. Nessun `N/A` motivato dal costo.

## COMMANDS

Da eseguire in **PowerShell**, con l'Editor chiuso. Nessuno di questi l'ho eseguito.

### C1 — build

```powershell
& "<engine>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -project="<repo>/RefactorTactics.uproject" -waitmutex
```

Dimostra che il file compila. ⚠️ **E' il gate che serve per primo**: i test sono authoring statico e
non ho potuto verificare nemmeno la sintassi.

### C2 — i due test nuovi

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario.LoaderRejectsCellArityOtherThanThree -LogName rt-2482-arity.log
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario.EveryCellFieldRejectsWrongArity -LogName rt-2482-callsites.log
```

Dimostrano: l'arita' diversa da tre e' rifiutata su tutto il dominio provato, il messaggio nomina la
lunghezza trovata e quella attesa, e la stessa regola vale sui sette chiamanti.

### C3 — la fixture migrata e il corpus

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario -LogName rt-2482-scenario.log
```

Dimostra due cose distinte: che `LoaderAcceptsValidScenario` non e' rimasto indietro alla migrazione
della fixture, e — via `ShippedScenariosAreValid` — che **nessuno** dei file del corpus smette di
caricare per la restrizione. ⚠️ Se questo diventa rosso, il corpus ha una coordinata che la grep non
ha visto: il rifiuto lo nominera' per file.

### C4 — la suite intera

```powershell
./scripts/rt-suite.ps1
```

Dimostra che nessun altro test costruiva scenari con celle a due elementi. Misura statica a
supporto, **non** sostitutiva: `"cell"`/`"targetCell"` a due elementi in `Source/` = 1 occorrenza,
quella che ho migrato; nel corpus = 0 array (le 4 corrispondenze a due elementi sono prosa dentro
`_nota_geometria` di `Spec/Reaction/DeflectionPoolSpansMultipleHits.json`).

### C5 — anti-vacuita' per MUTAZIONE (acceptance criterion 4)

Non e' asserita da nessun test: e' una procedura, e va eseguita **due volte** perche' la guardia ha
due lati e un test che ne coprisse uno solo sarebbe verde per meta'.

**File da mutare:** `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp`, dentro
`ParseCell` — la riga della guardia, che il contributo DEV-MAIN dichiara come `Arr->Num() != 3`
(`RTScenarioLoader.cpp:61` la firma; la guardia e' le righe subito sotto, oggi `if (Arr->Num() != 3)`).

| # | Mutazione | Righe rosse ATTESE | Cosa dimostra |
|---|---|---|---|
| M1 | `Arr->Num() != 3` → `Arr->Num() < 2`, e reintrodotto il ternario `Arr->Num() >= 3 ? (*Arr)[2]->AsNumber() : 0` | `LoaderRejectsCellArityOtherThanThree` sui casi **4** e **5**; `EveryCellFieldRejectsWrongArity` su **tutti e sette** i blocchi | il **limite superiore** e' cio' che rende rossi quei casi. E' la mutazione che il criterio 4 chiede alla lettera |
| M2 | `Arr->Num() != 3` → `Arr->Num() < 3` (solo il limite basso) | `LoaderRejectsCellArityOtherThanThree` sui casi **4** e **5** | isola il limite superiore dal ternario: M1 muove due cose insieme, M2 una sola |
| M3 | `Arr->Num() != 3` → `Arr->Num() > 3` | `LoaderRejectsCellArityOtherThanThree` sui casi **0, 1, 2** | il **limite inferiore**, cioe' l'arbitrazione (b). Se questa non diventa rossa, la decisione sulla forma a due elementi non e' misurata da nessuno |

⛔ **Dopo ogni mutazione: `git restore` del file mutato e ricompilare prima della misura
successiva.** Una mutazione lasciata in albero rende NON VALIDA ogni misura seguente — e la suite
non se ne accorge, perche' l'albero e' identico dall'inizio alla fine della run.

⚠️ Se sotto M1 **non** diventa rossa nessuna riga, il difetto non e' nella mutazione: e' che il test
sta leggendo qualcosa che non e' la guardia. Segnalatelo invece di rifare la mutazione.

## EXPECTED FAILURES

Nessun test e' scritto per restare rosso. Due condizionali, che diventano attese solo se accade la
premessa:

1. **Se il messaggio d'errore nomina le lunghezze a parole** («attesi tre elementi») invece che in
   cifre, tutte le assertion `Contains("3")` / `Contains("4")` / `Contains(<N>)` cadono. Il testo
   dichiarato da DEV-MAIN (`«attesi 3 elementi, trovati %d»`) le soddisfa — **letto dal suo
   contributo, non eseguito**. Se il testo cambia in consolidamento, cadono insieme e la sede della
   decisione e' DEV-LEAD, non un ammorbidimento del test.
2. **Se il ramo `nullptr` non nomina il `Where`**, cade la sola assertion del blocco 3 del test 1.
   Il contratto pretende un rifiuto attribuibile; se DEV-LEAD decidesse che l'array assente non deve
   nominare dove, e' quella riga a cadere e va rimossa con una decisione, non con una `Contains`
   piu' larga.

## RISKS

- 🔴 **Non ho compilato niente.** Il rischio dominante e' sintattico, non semantico: sette fixture
  JSON dentro `FString::Printf`, tre raw string annidate, una `struct` locale. Un `%s` di troppo o
  un delimitatore `)"` sbagliato e' un errore di compilazione, non un rosso di test — C1 e' il gate
  che serve per primo.

- ⚠️ **Le fixture di `interiorWalls`, `doors` e `variants` sono costruite a mano.** Le ho copiate da
  scenari del corpus che `ShippedScenariosAreValid` tiene validi, ma il corpus non contiene la
  combinazione esatta che ho scritto (porta senza binding **e** due squadre **e** una variante).
  Se cade una **controprova** in C2, il difetto e' quasi certamente nella mia fixture e non
  nell'arita': il messaggio del rifiuto lo dira'. In quel caso la riparazione e' la fixture, mai
  l'assertion.

- ⚠️ **`Contains(<numero>)` e' una lettura di sottostringa.** La ho resa attribuibile scegliendo
  `mapRadius: 6` e id senza cifre, cosi' che nel test 1 solo l'arita' possa produrre `3` o `4`. Nel
  test 2 resta un `1` nel JSON (`version`, `team`, `value`), che pero' non e' asserito da nessuna
  riga: le uniche cifre lette sono `4` e `3`, e nessuna delle due compare nel modello.

- ⚠️ **Migrazione di una fixture altrui non c'e', ma un rischio simmetrico si': il blocco 4 del test
  1 dipende dalla fixture `TestArena`.** E' la stessa che
  `LoaderRejectsLayerOutsideFlatArena` usa oggi per il caso multilivello; se quella fixture cambia
  forma, cadono entrambi i test insieme e non solo il mio.

- 🔴 **Tre siti di coordinate NON coperti, e non per dimenticanza.** `targetCell` e `dashTo` sono
  letti inline nell'intent con la vecchia guardia (`Num() >= 2`, terzo elemento opzionale) e non
  passano da `ParseCell`; DEV-MAIN li ha trovati per conto suo e li porta in
  `## INTEGRATION REQUIRED`. Convergiamo da due letture indipendenti. `targetCell` ha in piu' un
  difetto che nessuno dei due ha nel contratto: con meno di due elementi **non e' un errore**, e' un
  campo che sparisce — il ramo fallisce e `bTargetsCell` resta falso in silenzio, che e'
  precisamente la modalita' di fallimento dell'issue in un campo diverso. **Non ho scritto il test**:
  un test rosso su un ramo che nessuno ha deciso di cambiare verrebbe letto come regressione. E' un
  Finding per DEV-LEAD, ed e' scritto anche nel doc header del test 2 perche' il prossimo che conta
  i chiamanti lo trovi.

- ℹ️ **«Otto chiamanti» sono sette.** L'ottava occorrenza che `grep -c ParseCell` conta e' la
  DEFINIZIONE. Ne consegue che l'acceptance criterion 5 e' soddisfatto **per intero** (sette su
  sette esercitati), non per dichiarazione. Anche qui converge con DEV-MAIN, che lo ha contato
  separatamente.

- ℹ️ **Il corpus e' 548 array `cell`/`targetCell` a tre elementi**, non 532: il work order misurava
  su `5ee69775`, io su questo worktree. La conclusione non cambia — zero occorrenze a due o a
  quattro elementi, costo di migrazione zero — ma il numero nel work order e' vecchio.

## NOT RUN

| Gruppo | Stato | Motivo |
|---|---|---|
| BUILD | NOT RUN | authoring — esecuzione a VALIDATION; il ruolo DEV non occupa Unreal |
| AUTOMATION — `LoaderRejectsCellArityOtherThanThree` | NOT RUN | authoring — esecuzione a VALIDATION (C2) |
| AUTOMATION — `EveryCellFieldRejectsWrongArity` | NOT RUN | authoring — esecuzione a VALIDATION (C2) |
| AUTOMATION — `LoaderAcceptsValidScenario` (fixture migrata) | NOT RUN | authoring — esecuzione a VALIDATION (C3) |
| AUTOMATION — `ShippedScenariosAreValid` (regressione corpus) | NOT RUN | authoring — esecuzione a VALIDATION (C3) |
| SUITE INTERA | NOT RUN | authoring — esecuzione a VALIDATION (C4) |
| MUTATION TEST (criterio 4) | NOT RUN | e' un gate, non authoring: richiede build. Procedura in C5 |
| SCENARIO HARNESS | NOT RUN | occupa Unreal |
| PIE / EDITOR | NOT RUN | occupa Unreal; nessun `.uasset`/`.umap` nel write-set |
| PACKAGED | NOT RUN | fuori dal percorso toccato |
| `HEAD == BASE_SHA` | NOT RUN | il mandato vieta ogni comando git; `BASE_SHA 39f3ec95` assunto da `RT3-DEVLEAD-39f3ec9.md`, non misurato |
| `git grep -c '"cell"' Scenarios/` invariato (criterio 6) | NOT RUN | vietati i comandi git; garantito per COSTRUZIONE — nessun file del corpus e' nel write-set. Verifica a VALIDATION |

## NOTA DI PROCESSO

Ho letto `RTScenarioLoader.cpp` in due punti e dichiaro entrambi, perche' il mandato mi vieta di
derivare le assertion dall'implementazione:

1. `grep ParseCell` per **contare i chiamanti** — l'acceptance criterion 5 lo chiede alla lettera;
2. il doc header e le prime righe della guardia, per una **verifica di non-vacuita'**: sapere che le
   mie assertion possano fallire contro il codice VECCHIO. Non ho letto il corpo oltre la guardia e
   nessuna assertion e' accoppiata a una stringa letterale dell'implementazione.

Il contributo `DEV-MAIN-a1-01.md` l'ho letto **dopo** aver scritto i test — `WAVE_DEV_TEST.md`
lo elenca fra gli input di avvio, e leggerlo prima avrebbe reso i test derivati dall'implementazione
invece che dal contratto. L'unica modifica che ne e' seguita e' un commento: il doc header del test
2 nominava il solo `targetCell`, e ora nomina anche `dashTo`.

STATUS:   READY
