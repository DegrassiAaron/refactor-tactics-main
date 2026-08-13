# Feature Registry — owner del modello

> `CURRENT` · **Owner** del modello di tracciabilità delle feature. **Ultimo aggiornamento**: 2026-08-08.
> I dati vivono in [`feature-registry.yaml`](feature-registry.yaml); qui c'è **cosa significano** e
> **come si usano**. Lo stato di esecuzione resta in [`roadmap-checkpoint.md`](roadmap-checkpoint.md),
> lo scope di release in [`roadmap-v0.1.md`](roadmap-v0.1.md).

## 1. Il problema che risolve

Il repository ha già pagato quattro volte lo stesso difetto: **un numero scritto a mano in due viste
diverge**, e la seconda copia diventa una bugia con la data sbagliata. È successo col conteggio dei
test (172 → 179 → 324 → due valori entrambi corretti alla propria base al merge di CP 9.3) ed è
successo con lo stato delle epic (M8/M9 dichiarate ⏳ mentre E4–E6 erano chiuse).

La Wiki è il posto dove il difetto costa di più, perché è la vista che leggono gli altri: oggi
`Meccanica-overwatch.md` porta in testa un blocco `> **Stato v0.1:** …` scritto a mano, e nessuno lo
rigenera quando il codice cambia.

Il registry esiste perché **lo stato si aggiorni una volta sola**:

```text
FEATURE ID → SPEC → ROADMAP → ISSUE → CODE → TEST → SCENARIO → WIKI
```

Tutte le viste leggono; nessuna copia.

## 2. Cosa è e cosa non è

| È | Non è |
|---|---|
| La vista di **prodotto**: quali capacità esistono e a che punto sono | Una roadmap: l'ordine del lavoro resta nelle due roadmap |
| Un indice stabile di `feature_id` | Un elenco di issue: le issue si spezzano, gli id no |
| Un file **verificato da una macchina** | Un riassunto discorsivo |

Una feature può attraversare più checkpoint; una epic può implementare più feature. I due assi non
si comprimono in una lista sola.

## 3. Feature ID

`RT-FEAT-<AREA>-<NOME>`, stabile e indipendente dal titolo visuale. **Non si usa il numero di una
issue come id**: le issue cambiano, si spezzano, si chiudono per errore e si riaprono.

Rinominare un `feature_id` è un'operazione con costo: rompe i blocchi generati nelle pagine e i
riferimenti del workbook. Se serve, si rinomina e si rigenera tutto nello stesso commit.

## 4. I gate

| Gate | `done` quando |
|---|---|
| `spec` | Un documento owner descrive le regole e dichiara chi le possiede |
| `data` | Cataloghi, data asset e config sono coerenti con quelle regole |
| `runtime` | L'implementazione esiste in `Source/` e **qualcuno la consuma** |
| `log_debug` | L'esito è osservabile: TurnLog, reason code, comando di debug |
| `automation` | Test automatici pertinenti, verdi, che chiamano il gameplay reale |
| `scenario` | Almeno uno scenario in `Scenarios/` la **dimostra** — vedi la nota qui sotto |
| `ui_wiki` | È spiegata all'utente **e** leggibile in gioco — servono entrambe. Con una sola delle due: `partial` (regola 5) |
| `packaged` | Verificata nella build packaged **della release corrente** — chi lo dimostra sta in `pie_refs` (regola 6) |
| `network_privacy` | Corretta in rete: autorità server e nessuna fuga di intento |
| `replay_representable` | L'evento che produce **sopravvive alla traccia**: un Player lo ricostruisce senza ricalcolare — vedi la regola 7 |

Valori: `done` · `partial` · `todo` · `na`.

Sette regole che vale la pena scrivere, perché sono i modi in cui questo schema si corrompe:
*(erano «cinque» fino al 2026-08-12 mentre l'elenco ne contava già sei — il numero era scritto a mano.)*

1. **`na` è una risposta, `partial` no.** Una feature offline dichiara `network_privacy: na`; una
   feature che *dovrà* funzionare online dichiara `todo`, anche se oggi non è verificabile. Marcare
   `done` una privacy che è banale solo perché il gioco è offline è il modo più facile di mentire.
2. **`runtime: done` richiede un consumatore.** Il difetto ricorrente di questo progetto non è la
   formula sbagliata, è il dato che nessuno legge: `Marked` a catalogo senza codice che lo
   interroghi, `Effects` vuoto nelle reazioni d'eroe. Un dato senza consumatore è `partial`.
3. **`packaged` guarda la release corrente.** Il packaging di M7.4 (2026-08-06) ha verificato la
   partita, ma **precede** E8, E9 e gli scenari: non copre ciò che è arrivato dopo. Finché la
   release interna E12.5 non lo rifà, nessuna feature di gameplay dichiara `packaged: done`.
4. **Uno scenario che esce `BLOCKED` non è `scenario: done`.** Il corpus contiene scenari `Spec.*`
   scritti **prima** della capability — sono specifiche eseguibili, e lo dichiarano nelle proprie
   note: «esce BLOCKED finché `DecisionBoundary` non esiste, e va bene». Il loro valore è che la
   feature ha una forma eseguibile prima di essere costruita, e che il giorno in cui atterra si
   accendono da soli. Ma descrivere non è dimostrare: il gate resta `partial` finché lo scenario
   non passa davvero.
5. **`ui_wiki` conta due metà, e una sola vale `partial`.** «Spiegata all'utente» e «leggibile in
   gioco» sono indipendenti: una feature progettata ma non implementata può avere la pagina Wiki e
   nessuna UI (`partial`), una feature interna può avere la UI e nessuna pagina (`partial` anche
   lì). Il gate non dice *quale* metà manca — lo dicono `wiki_refs` e lo stato del runtime.
   **Il blocco `RT_FEATURE_STATUS` da solo non è spiegazione**: è generato, e comparirebbe anche
   su una pagina che della feature non parla. Serve testo che descriva la meccanica. Una pagina che
   dichiara l'*assenza* della feature — «il bot non fa ancora giocate a due» — documenta ciò che
   c'è oggi, non la feature futura: quel gate resta `todo`.

   Corollario aggiunto il 2026-08-10, perché l'avviso «nessuna pagina Wiki collegata» contraddiceva
   questa stessa regola: la Wiki è rivolta a chi gioca, e `wiki_refs` vuoto è **legittimo** per una
   feature `tooling`, `data` o `infra`. L'avviso ora vale solo per `gameplay`, `ui` e `content` — le
   sole categorie che la Wiki documenta, come i dati già dicevano (60 pagine collegate, nessuna a
   una feature interna). Restava invece scoperto il caso opposto, che ora è un **errore**:
   `ui_wiki: done` dichiara entrambe le metà, quindi senza `wiki_refs` afferma una spiegazione che
   non esiste.

6. **`packaged` dice che una persona ha verificato; `pie_refs` dice quale verifica.** Il gate vale
   `done` quando la feature è stata provata nella build packaged, e la prova è una voce `PIE-*` di
   [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md). Fino al 2026-08-10 quel
   legame **non era un dato**: viveva nel corpo delle issue GitHub, fuori dal repository. La
   conseguenza è la stessa famiglia del dato che nessun consumatore legge, al contrario — il giorno
   in cui `PIE-V01-HUD` diventa ✅, nessuno sa che quattro feature possono avanzare il gate.

   Ora il validator lo deriva: `packaged: done` con una voce citata non ✅ è un **errore**, e tutte
   le voci ✅ con il gate ancora `partial`/`todo` è un **avviso** che il gate può avanzare. Il campo
   è opzionale — assente significa «non ancora mappato», non «nessuna verifica manuale».

   **Come si assegna, e come non si assegna.** L'origine è la sezione «Test / verifica» dell'issue
   che la feature dichiara, ma quella citazione è un indizio, non una prova: quando più feature
   dichiarano la stessa issue, il criterio meccanico attacca la stessa voce a tutte. È costato tre
   legami falsi, trovati in review — `PIE-V01-HUD` verifica barre, timer, slot e cooldown, quindi
   dimostra l'HUD di planning, non la camera tattica né il playback della risoluzione. Prima di
   scrivere un legame si legge il **testo della voce** e si verifica che parli di quella feature.
   Una stessa voce può legittimamente dimostrarne più d'una; non lo fa per il solo fatto che l'issue
   è condivisa.

7. **`replay_representable` non è `log_debug` scritto due volte**, ed è la regola che serve di più
   perché i due gate si somigliano abbastanza da essere confusi al primo sguardo. `log_debug` chiede
   che l'esito sia **osservabile**: c'è una voce, un reason code, un comando che lo mostra. È
   diagnosticabilità. `replay_representable` chiede che quella voce **basti a ricostruire l'evento
   da sola**, dopo la serializzazione, senza che nessuno ricalcoli — è il vincolo che
   [ADR-0009](../decisions/adr-0009-replay-logico-canonico.md) impone al Player, *«non calcola
   collisioni, danni, reazioni, KO, legalità dei percorsi né targeting: non produce esiti, li legge»*.

   **I due gate divergono davvero, e il repository ha l'esempio: il danno da `Status.Burning`.**
   Nella Cleanup, un'unità che brucia subisce `BurningCleanupDamage`, perde HP e **può morire**. Il
   fatto è osservabile — c'è una riga che nomina unità, danno e cella — quindi `log_debug` regge.
   Ma quella riga è `ARTTurnManager::AddLogEvent`, cioè `UE_LOG` più il buffer circolare
   `RecentEvents` troncato a `MaxLogLines`: **non passa da `AppendLogEntry`**, quindi non esiste una
   voce canonica. Chi riproduce vede gli HP scendere — o un'unità sparire — senza un evento che lo
   spieghi, e `CompareSerializedTraces` non può nominare quel punto come divergenza. Il codice lo
   dice da sé: *«l'eliminazione da hazard non ha un beat di playback … la nasconde il catch-all di
   `ConcludeTurn`»*. `replay_representable` è `todo` per `RT-FEAT-ENV-STATUS` e `RT-FEAT-ENV-FIRE`.

   ⚠️ **La prima stesura di questa regola usava `GraphRevision` come esempio, ed era sbagliata.**
   Citava la nota di [D-067](../decisions/RT_PDR_00_Decision_Log.md) — «produttore assente», vera il
   2026-08-10 — senza verificarla sul codice: il produttore è atterrato con `908b84b`, e
   `ARTTurnManager::AppendLogEntry` valorizza `GraphRevision` (con `TurnNumber` e `UnitId`) su
   **ogni** voce, perché è l'unico `TurnLog.Add` di produzione del progetto. Il test
   `RefactorTactics.TurnLog.GraphRevisionRisesWithinTheTurn` lo pinna sul percorso reale. La lezione
   sta nella regola stessa: **si cerca chi produce un campo, non chi lo dichiara** — e una nota `⚠️`
   del Decision Log è datata quanto il giorno in cui è stata scritta.

   **Come si assegna.** L'oracolo è `ERTLogCategory` in `RTTurnLog.h` — `Move`, `Combat`,
   `Fallback`, `Reaction`, `Environment`, `Facing`, `Predictive`: le sette categorie in cui una voce
   può esistere. Se la feature non produce voci in nessuna di quelle, il gate è **`na`**, ed è la
   risposta giusta per le query (`LOS`, A\*), le proprietà derivate, la UI che legge, il tooling, i
   dati e i contenuti. Se le produce e qualcosa le pinna, `done`. Se le produce e nessuno le pinna,
   `todo` — ed è lì che il gate guadagna il suo posto.

   ⚠️ **Il gate sta nel gradino `DONE`**, accanto a `packaged` e `network_privacy`, e la ragione è
   misurata: quando è entrato (2026-08-12) una sola feature era `DONE`, quindi non ha fatto
   regredire nessuno stato dichiarato. Un gate nuovo che retrocede metà registry al primo giro non
   viene creduto, viene aggirato. Prima assegnazione: **14 `done` · 32 `todo` · 45 `na`**
   *(era 10/36/45: quattro feature di mappa erano `todo` per l'esempio sbagliato qui sopra —
   grafo, copertura dinamica, porte e ponti emettono tutti un `ERTEnvironmentOutcome` con un
   produttore reale, verificato uno per uno).*

### 4.1 Quando un pezzo lo porta un'altra feature

`completed_by:` dichiara **chi** completa ciò che a questa feature manca.

Nasce da un difetto trovato nella revisione di granularità del 2026-08-08:
`RT-FEAT-ACTION-GENERIC` aveva quattro gate su otto deformati da due mancanze —
`Overwatch` e `Interact` — che appartengono ad **altre due feature** già tracciate (E14.4 ed
E10.1). La stessa mancanza era contata tre volte, e chiudere E14 avrebbe richiesto di ricordarsi
di aggiornare due righe: esattamente il meccanismo che questo registry esiste per eliminare.

```yaml
completed_by:
  - RT-FEAT-REACTION-OVERWATCH
  - RT-FEAT-OBJECTIVE-SYSTEM
```

I gate **restano** aperti — la feature davvero non è completa — ma il rimando è un dato, non una
frase nelle note: compare nella §2.2 della roadmap e nei blocchi Wiki, e il validator verifica che
gli id esistano. Se una feature dichiara `completed_by` senza avere gate aperti, il validator
avvisa: il rimando è vecchio.

**Non è `dependencies`.** Una dipendenza è un prerequisito — ciò che deve esistere *prima*.
`completed_by` è il contrario: ciò che arriverà *dopo* e chiuderà il buco.

### 4.2 Le cinque domande, e quale non ha un gate

Lo [spec panel del 2026-08-12](plans/five-lane-roadmap-spec-panel-2026-08-11.md) ha portato una
checklist di copertura da applicare a ogni incremento — cinque domande, con `N/A + motivo` ammesso e
la clausola che le rende usabili: *«non inventare lavoro inutile solo per riempire tutte le
caselle»*. Non introduce identificatori: si legge sui gate che già esistono.

| La domanda | Il gate che la risponde |
|---|---|
| I **dati** e le query sono validi? | `data` |
| Il risultato è **deterministico e loggato**? | `log_debug` + `automation` |
| Il risultato è **leggibile**? | `ui_wiki` |
| È **configurabile e ispezionabile** senza hack? | ⚠️ **nessuno** |
| È **riproducibile** senza divergenza né fuga? | `replay_representable` + `network_privacy` |

⚠️ **La casella scoperta non è quella che il sorgente pensava.** Il documento insisteva sul replay
per tre milestone; il replay un gate ce l'ha (§4, regola 7). Quella senza è l'**authoring**: nessun
gate chiede se un designer possa configurare la feature e accorgersi di averla configurata male.
Oggi si vede in negativo — `RT-FEAT-TOOL-MAP-EDITOR` e `RT-FEAT-TOOL-VALIDATION` esistono come
*feature*, quindi la capacità è tracciata una volta sola, per il tool, e mai per le feature che
quel tool dovrebbe rendere componibili.

**Non è stato aperto un gate `authorable`**, e la ragione è la stessa che vale per ogni campo di
questo registry: un gate senza consumatore è un dato che nessuno legge. Serve prima decidere se la
domanda è per-feature o per-tool — e quella decisione non è ancora stata presa da nessuno.

## 5. Lo stato è derivato, non dichiarato

`status` **non è un giudizio**: è una funzione dei gate, e il validator la verifica. Se i due
divergono, `validate` esce con errore — in entrambe le direzioni, anche quando i gate dicono che la
feature è più avanti di quanto scritto.

```text
DONE            core + scenario + ui_wiki + packaged + network_privacy + replay_representable
RELEASE_READY   core + scenario + ui_wiki
INTEGRATED      core + scenario
TESTABLE        spec + runtime + automation
IMPLEMENTING    runtime done o partial
SPECIFIED       spec done
DESIGNED        spec partial
IDEA            nient'altro
```

dove `core` = `spec` + `data` + `runtime` + `log_debug` + `automation`, e un gate `na` conta come
soddisfatto.

`DEFERRED` e `BLOCKED` sono **fuori scala**: dichiarano una decisione, non un grado di
completezza, e il controllo di coerenza li salta. Vanno usati con una `notes` che dica chi ha deciso.

**Niente percentuali.** Il progresso è «`6/8` gate», che si verifica; «73%» si contratta.

## 6. Comandi

```bash
python scripts/feature_registry.py validate    # gate: esce 1 se ci sono errori
python scripts/feature_registry.py generate    # feature-registry.json + project-graph.json
python scripts/feature_registry.py wiki        # blocchi di stato + pagina Stato delle feature
                                               #   + tabella §2.2 di roadmap-v0.1.md
python scripts/feature_registry.py workbook    # sheet 15_Wiki_Feature_Refs del workbook character
python scripts/feature_registry.py shortlist   # le cinque viste corte di docs/roadmap/*.shortlist.md
python scripts/feature_registry.py report      # tabella di audit su stdout
```

`generate`, `wiki`, `workbook`, `shortlist` e `deploy` accettano `--check`: non scrivono e falliscono se
l'output è disallineato dalla sorgente. È la forma da usare in un gate automatico (**G15** della Definition
of Done).

### I due file generati

`generate` scrive **due** artefatti, e la divisione è deliberata:

| File | Contiene | Perché separato |
|---|---|---|
| [`feature-registry.json`](feature-registry.json) | le feature: gate, status derivato, riferimenti | è l'owner delle **feature** e ha già consumatori |
| [`project-graph.json`](project-graph.json) | tutto il resto: diagnostica del validator, gate di release, epic/milestone/checkpoint, sedute in editor con stato e coda, voci `PIE-*`, corpus scenari, capability | allargare il primo a queste cose gli darebbe una seconda responsabilità — lo stesso errore evitato sul `.yaml` |

`project-graph.json` **non ricalcola nulla**: è un secondo consumatore delle funzioni che alimentano le
shortlist. Serve a chi non legge markdown — oggi il [Project Control Center](plans/project-control-center-spec.md),
domani qualunque altro strumento. Non contiene il commit corrente **di proposito**: ce lo mettesse,
`--check` fallirebbe dopo ogni commit e diventerebbe rumore da ignorare.

### Le cinque shortlist

`shortlist` riscrive i blocchi marcati di
[`roadmap.shortlist.md`](roadmap.shortlist.md), [`featuremap.shortlist.md`](featuremap.shortlist.md),
[`scenariomap.shortlist.md`](scenariomap.shortlist.md),
[`milestonemap.shortlist.md`](milestonemap.shortlist.md) e
[`editormap.shortlist.md`](editormap.shortlist.md). Fuori dai marcatori non tocca niente.

Non inventa una seconda regola per lo stato: **ogni valore viene dal proprio owner**, misurato.

| Blocco | Cosa genera | Sorgente |
|---|---|---|
| `RT_SHORTLIST_EPICS` | stato e gate per epic | stato da [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1 · gate dal registry |
| `RT_SHORTLIST_FEATURES` | le feature per area | il registry |
| `RT_SHORTLIST_SCENARIOS` | corpus, `BLOCKED`, `planned` | `Scenarios/` + le capability di `RTScenarioSession.cpp` |
| `RT_SHORTLIST_MILESTONES` | stato per milestone | [`roadmap-checkpoint.md`](roadmap-checkpoint.md) |
| `RT_SHORTLIST_MILESTONES_GATES` | i gate `G1`–`G15` | [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3 |
| `RT_SHORTLIST_EDITOR` | le sedute in editor + **My Editor Queue** | [`editor-sessions.yaml`](editor-sessions.yaml) · voci `PIE-*` · `git ls-files` sugli artefatti |

**`My Editor Queue`** (`BLOCKING` · `READY` · `WAITING` · `DONE`) è derivata da tre campi che le sedute già
dichiarano — `unblocked_by`, `critical`, e lo stato — senza aggiungerne uno. Le due regole di risoluzione
sono diverse per un motivo: un **checkpoint** 🟡 conta come risolto (il codice è fatto, manca la verifica
che porti tu), una **seduta** 🟡 no (l'artefatto è a metà, e chi lo estende non può partire). I riferimenti
si scrivono prefissati — `M6.3`, `E1.3`, `E8`, `U13` — perché 20 numeri di checkpoint su 22 esistono in due
spazi: la forma nuda `CP 6.3` non risolve, e la vista la segnala.

**La colonna umana viene preservata.** L'ultima colonna di ogni tabella — la riga di descrizione — non è
derivabile: il generatore la rilegge dal blocco precedente e la reimpagina. Una feature nuova compare con
`—`, che è un buco visibile invece di una riga che sparisce.

**Due cose che il generatore fa e una persona no**: quando lo stesso oggetto è dichiarato due volte con
valori diversi (è successo con **M6** e **M7**, 🟡 in una tabella e ✅ in un'altra dello stesso file) lo
**nomina** e riporta la lettura più conservativa, invece di sceglierne una in silenzio; e quando un'epic
non ha stato nell'owner (**E18**–**E21** in §2.1) lo dichiara invece di dedurlo.

### Deploy verso il clone della Wiki

```bash
python scripts/feature_registry.py deploy --wiki-root <path>          # sola lettura: elenca
python scripts/feature_registry.py deploy --wiki-root <path> --check  # gate: esce 1 se disallineato
python scripts/feature_registry.py deploy --wiki-root <path> --write  # scrive davvero
```

La Wiki di GitHub è un **repository separato** (`refactor-tactics-main.wiki`, branch `master`) e
**pubblico**. Per questo `deploy` è in sola lettura di default: scrivere lì è un'azione che si chiede,
non che si assume — tanto più che altre sessioni possono avere quel clone in lavorazione.

#### Piatto è il namespace, non il filesystem

Fino al 2026-08-10 il deploy appiattiva tutto in root con un prefisso nel nome
(`Meccanica-coperture.md`), perché `DEPLOY.md` dichiarava che «la GitHub Wiki non ha gerarchia».
**Verificato, ed era sbagliato a metà**: le pagine in sottocartella *sono* servite e compaiono nel menu
*Pages*. Ciò che è piatto è il **namespace degli URL** — l'indirizzo è il solo nome del file, il
percorso non ci entra.

Tre conseguenze, tutte vincolanti:

1. i nomi delle pagine devono restare **globalmente unici**, anche fra cartelle diverse;
2. i `[[link]]` sono **per nome**, mai per percorso: i link markdown relativi a `cartella/pagina.md`
   non funzionano in modo affidabile;
3. le immagini con path relativo — quelle che puntano dentro `images/` — **non risentono** dello
   spostamento, perché la base relativa è l'URL della pagina, che è piatto, e non la posizione del
   file. Misurato: l'asset risponde `200` sia da `raw.githubusercontent.com/wiki/…` sia da
   `/wiki/images/…`.

Un `wiki_refs` ha quindi **due forme**, e la differenza non è di stile:

| Forma | Dove vive la pagina | Come si risolve |
|---|---|---|
| `wiki:<PageName>` | **solo nel clone** — le pagine di gioco, dopo D-076 | cercando il nome nel clone: serve `--wiki-root` |
| `docs/characters/…md` | **nel repository**, con deploy verso il clone | `deploy_name`, tabella qui sotto |

`wiki:<PageName>` nomina la pagina, **non** il suo percorso. È voluto: sopravvive a una
riorganizzazione delle cartelle, che è precisamente ciò che è successo il 2026-08-10 quando 89
pagine sono uscite dalla root del clone. Il prezzo è che senza `--wiki-root` quei riferimenti non si
possono verificare — e `validate` lo dice invece di tacerlo.

Il catalogo eroi resta invece nel repository, perché non è una sorgente Wiki: lo referenziano
cataloghi, ADR e test. Il clone ne è solo il deploy.

| Sorgente nel repo | Pagina Wiki | URL |
|---|---|---|
| `docs/characters/v0.1\|v0.2/<x>.md` | `Personaggi/<x>.md` | `/wiki/<x>` |
| `docs/characters/index.md` · `paragon.md` | `Personaggi.md` · `Personaggi/paragon.md` | |

Le pagine **indice restano in root** accanto a `Home`: sono hub, e soprattutto tre `index.md` in tre
cartelle diverse collidono — non possono chiamarsi tutte `index`.

`Stato-delle-feature.md` non ha più una sorgente: la genera `deploy` direttamente nel clone. Fino a
D-076 esisteva come `docs/wiki/feature-status.md`, il comando `wiki` la scriveva lì e il deploy la
copiava — con una finestra, fra i due, in cui la copia pubblicata era vecchia e nulla lo diceva.

Il confronto con le pagine del clone è **ricorsivo** e **case-sensitive** anche su Windows, dove
`os.path.isfile` non distingue le maiuscole. Senza la ricorsione le pagine in cartella risultano
assenti in blocco (misurato: 35 su 35); senza il case, un nome sbagliato nelle sole maiuscole passa in
locale e salta in silenzio i blocchi di quella pagina su un filesystem case-sensitive — è così che
`Sinergie-e-Combinazioni.md` è rimasta scoperta fino al 2026-08-10.

**Una sorgente che non raggiunge il clone è un errore, non un avviso.** Il gate `ui_wiki` della feature
afferma «spiegata all'utente», ma il lettore legge la Wiki *pubblicata*: se la pagina lì non esiste, il
registry sta dichiarando una copertura falsa invece di segnalare una lacuna — stessa famiglia dello
scenario orfano, e per la stessa ragione è bloccante.

`validate --wiki-root <path>` verifica anche i riferimenti nella forma `wiki:<PageName>`, quelli per
le pagine che vivono **solo** nel clone e non hanno una sorgente nel repository.

Errori (bloccanti): id duplicato o malformato · epic, milestone o checkpoint inesistenti ·
ScenarioId inesistente · **ScenarioId non rivendicato da nessuna feature** · owner spec inesistente ·
riferimento a un test che la suite non ha · dipendenza verso un id inesistente · valori fuori dominio ·
`status` che diverge dai gate · `last_verified` assente per stati `TESTABLE` o superiori · blocco generato
per un id non nel registry.

> **Lo scenario orfano è entrato il 2026-08-09**, e il controllo esisteva solo in un verso. Il registry
> verificava «lo `ScenarioId` che dichiaro esiste davvero?» e non «esiste uno scenario che nessuno
> dichiara?». Al momento dell'aggiunta erano **6 su 54**, fra cui `Visual.Map.HighCoverBlocks` e
> `Spec.Environment.ElectricPropagation` — tutti documentati in
> [`../technical/scenario-map.md`](../technical/scenario-map.md), tutti eseguiti, nessuno collegato a una
> feature. Uno scenario che nessuno rivendica passa e non dimostra niente a nessuno: è la stessa famiglia
> del dato che nessun consumatore legge.
>
> È un **errore** e non un avviso, per simmetria con «scenario dichiarato `planned` ma presente in
> `Scenarios/`»: in entrambi i casi il registry sta dicendo qualcosa di falso sulla copertura, non
> segnalando una lacuna. Se uno scenario davvero non appartiene a nessuna feature, la via d'uscita è
> dichiararlo in `notes`, non lasciarlo muto.

Warning (da vedere, non bloccanti): feature senza pagina Wiki · feature di gameplay testabile senza
scenario che la dimostri · `SPECIFIED` senza issue né assegnazione · scenari dichiarati `planned`.

## 7. Blocchi di stato nelle pagine

Ogni pagina referenziata da `wiki_refs` riceve un blocco delimitato:

```markdown
<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-OVERWATCH -->
...
<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-OVERWATCH -->
```

**Non si edita a mano.** Chi lo riscrive dipende da dove vive la pagina: `wiki` per quelle nel
repository (`docs/characters/`), `deploy --wiki-root` per quelle che vivono nel clone. Una pagina può
averne più d'uno (la pagina di Gadget ne ha tre: roster, elettricità, interazioni sistemiche). Il
blocco va dopo il titolo e la citazione introduttiva, prima del corpo.

Un blocco che cita un `feature_id` non più nel registry è un **errore**; un blocco per una feature
che il registry non referenzia più su quella pagina è un **warning** (`wiki` lo segnala, non lo
rimuove: cancellare testo da una pagina è una decisione dell'autore).

## 8. Come si aggiunge una feature

1. Scegli un `feature_id` che descriva la **capacità**, non l'implementazione né il titolo della
   pagina.
2. Compila i gate **guardando il repository**, non la memoria: `grep` nel codice, nomi dei test,
   `Scenarios/`, la pagina Wiki.
3. Lascia che `status` cada dove i gate lo mandano. Se ti sembra sbagliato, il difetto è nei gate.
4. Metti `last_verified` con la data e il commit su cui hai guardato.
5. `validate` → `generate` → `wiki` → `workbook`, nello stesso commit.

Una feature che nessun documento definisce resta `IDEA` con `owner_specs: []`. Registrarla è utile —
significa che qualcuno l'ha pensata — ma promuoverla senza una fonte è il modo in cui una seed list
diventa un impegno che nessuno ha preso.

## 9. Rapporto con gli altri documenti

| Documento | Ruolo |
|---|---|
| [`feature-registry.yaml`](feature-registry.yaml) | **Sorgente** dello stato: si edita qui |
| `feature-registry.json` | **Generato**: consumatori automatici, mai a mano |
| [`roadmap-checkpoint.md`](roadmap-checkpoint.md) | Vista di **esecuzione**: milestone e checkpoint |
| [`roadmap-v0.1.md`](roadmap-v0.1.md) | Vista di **release**: epic, scope, gate della v0.1 |
| [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) | Gate `G1`–`G15` della release (`G15` è questo registry) |
| [`v0.1-issue-plan.md`](v0.1-issue-plan.md) | **Snapshot** delle issue: non è fonte di verità |
| `Stato-delle-feature.md` nel clone della Wiki | **Generata** da `deploy`: vista pubblica del registry |
| `docs/characters/data/*.xlsx` → `15_Wiki_Feature_Refs` | **Generata**: riferimenti entità → feature, senza stato (richiede `--wiki-root`) |

Il workbook di bilanciamento (`docs/balance/*.xlsx`) **non** entra in questa catena: contiene target e
metodi di misura, non stato di implementazione. Non c'era niente da deduplicare.
