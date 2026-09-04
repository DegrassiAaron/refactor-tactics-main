# Partita bot 2v2 osservata, e audit del kit «Scenari di test: danni, bot, pacing»

**Data:** 2026-09-04 · **Branch:** `fix/1793-2167-posa-e-offset` · **HEAD della misura:** `b063a60f`
**Binario:** `RefactorTacticsEditor` ricompilato qui alle 10:04 (`Result: Succeeded`, 83 s) — la partita gira sul
sorgente di **questo** checkout, non su un DLL altrui.
**Sorgente consumato:** `RT_Scenari_Test_Danni_Bot_Pacing.md` (bozza esterna del 2026-09-03), letto per intero e
rimosso a fine sessione.
**Evidenza:** `docs/technical/evidence/2269/partita-2v2-autobattle-2026-09-04.log` — le 469 righe `LogRT`
della partita, committate insieme a questo referto perché ogni numero della Parte B si possa ricontare.
**Modalità:** osservazione e diagnosi. Nessuna modifica a `Source/`, nessun peso ritoccato.

⚠️ **L'albero misurato non è `main`, e la differenza va dichiarata invece che lasciata dedurre.** `b063a60f`
non è un antenato di `origin/main`: porta due modifiche in più, `Content/RT/.../BP_Unit_Riktor.uasset` e
`docs/technical/test-manuali-pie.md` (`#2167`). La prima tocca **proprio l'unità** che il referto misura
inerte, quindi la si guarda invece di scartarla: è il flag `bFaceMovementDirection`, che
`Source/RefactorTactics/Unit/RTUnit.h:91-95` dichiara **presentazione** — interpola lo yaw della mesh — mentre
l'orientamento che le regole leggono è `Facing`. I suoi due soli lettori fuori dai test stanno nel playback
(`RTUnit.cpp:739`). ∴ non può spostare una decisione del bot; ma il delta esiste e chi rifà la misura su
`main` deve saperlo.

---

# Parte A — Il kit di scenari proposto, misurato contro il repository

Il documento proponeva 13 scenari (A-01..A-05, B-01..B-03, C-01..C-02, D-01..D-03) e dichiarava in apertura di
non conoscere lo schema reale dell'harness. La misura riga per riga dice che **la maggior parte del kit è già in
repository**, e che quattro sue premesse sono false al 2026-09-04.

## A.1 Le quattro premesse falsificate

| Voce del kit | Cosa afferma | Cosa misura il repository |
|---|---|---|
| **D-01** — swap `A↔B` deve bloccare, «oggi lo scambio avviene (#1922)» | bug aperto | 🔴 **Falso.** `#1922` è **CLOSED** (2026-08-31, PR #1986). `RefactorTactics.HexSim.ResolveSwapBlocked` (`RTHexSimTests.cpp:658`) asserisce il blocco; il test si chiamava `ResolveSwapAllowed` e **asseriva l'opposto** |
| **D-02** — ciclo chiuso a 3, «test mancante» | da scrivere | 🔴 **Già scritto.** `HexSim.ResolveClosedCycleBlocked` (`:680`), più `ResolveFreeTailConvoyStillAdvances` (`:709`) e `ResolveSwapBlockedEvenWhenPassingThrough` (`:739`). Il convoy a coda libera — che il kit **non nomina** — è il test che distingue una regola corretta da una che rompe la catena |
| **D-03** — «Seed non consumato»: due seed diversi, stessi hash | regressione da tenere | ⛔ **Tautologia dichiarata.** Non esiste RNG nel runtime: il determinismo è strutturale (`RNG-1`/`RNG-2` in `docs/OPEN_DECISIONS.md`, e il `_nota_seed` di `Scenarios/AutoBattle/OpenField.json`). Uno scenario così sarebbe verde per costruzione |
| **A-04** — Deflect pool commutativa, «collegato a #1918» | da verificare | 🔴 **Già deciso e pinnato.** `#1918` **CLOSED** (2026-09-01) → **D-309**; `RTCombatResolverTests.cpp:188` asserisce *«il totale sul bersaglio con Deflect non dipende da quale colpo arriva per primo»*, e `:190` che vale `40-20` senza avanzi persi nel clamp |

Il kit è datato 2026-09-03 e lavora su uno stato del repository **anteriore al 2026-08-31**, giorno in cui
`#1922` ha chiuso; `#1918` il giorno dopo.

⚠️ **`D-309` e non `D-312`**, e la distinzione non è pedanteria: `D-309` decide che `Deflect` **è** una
pool commutativa (l'uscita B della domanda di `#1918`); `D-312` decide l'**ordine** fra `Deflect` e `Guard`
quando due pool coprono lo stesso colpo. Chi cerca l'owner della commutatività e atterra su `D-312` legge una
decisione diversa.

## A.2 Errori di nomenclatura

- **«E27» per `ScenarioDefinition`/`Runner`/`Result`**: `E27` è *Percezione completa* (v0.3, `#327`), e con
  l'harness non c'entra. ⚠️ **Qui il referto non sostituisce un numero all'altro**: `ADR-0010` governa solo
  la *facciata Blueprint* dell'harness, e `CP 47.4` (E47) le sole chiavi del free-run; i test `Scenario.*`
  sono contati sotto **E12**. Chi cerca «l'epic dell'harness» non trova una risposta secca, e affermarne una
  sarebbe lo stesso difetto che questa sezione contesta al kit.
- **«E26 Objective System»**: `E26` è *Tactical Bot v1* (v0.2, `#326`). Gli obiettivi sono **E10** — *Obiettivi
  dinamici e fine partita*, **chiusa** — e E31 per i multipli. L'inversione conta: il kit rimanda B-02 aspettando
  che «E26 esponga l'obiettivo», mentre l'obiettivo **esiste già in partita** (§B.6 lo misura) e ciò che manca è
  proprio E26.
- **Coordinate `{"q":..,"r":..,"layer":..}`**: lo schema reale usa la **tripla posizionale** `[q, r, layer]`
  (`RTScenarioLoader.cpp`, `ParseCell`). Il kit prescrive il contrario di ciò che il repository fa.

## A.3 Ciò che il kit chiede e che esiste già

| Voce | Stato | Dove |
|---|---|---|
| **A-01** TTK sotto fuoco concentrato | ✅ misurato | `Scenarios/Visual/Combat/Defeat.json`: Riktor+Wraith (8+21) su Gadget (90 HP) → **KO al turno 6**, con la ragione scritta (`BaseShield` 5/turno di D-224 + `ReactiveCapacitor`). Fuori dalla banda 3-5 che il kit proponeva a priori |
| **A-03** «High è mitigazione o occlusione?» | ✅ risposto dai dati | `ERTHexCoverType::High` **nega** vista, passo e proiettili (`RTHexCellData.h:46`); scenario `Visual/Map/HighCoverBlocks.json` |
| **A-02** riduzione Low | ⚠️ **coperta a metà** | `-10` sul danno diretto dal lato riparato, arco frontale (`Visual/Map/LowCoverEdge.json`). Il Δ **sul TTK** non è misurato da nessuno |
| **B-01/B-03** bot che non attacca, partite che scadono | ⚠️ **una causa chiusa, non il fenomeno** | `#1088` **CLOSED** (2026-08-23) e `RTBotStalemateProbeTests.cpp` ha 11 test dedicati — ma la Parte B di questo stesso referto misura una partita **12/12 round decisa allo scadere, con una unità a 0 danni**. La chiusura ha tolto il deadlock geometrico; l'attesa numerica è rimasta |
| **C-01** standoff/stallo | ✅ coperto | stesso file, `StalemateProbe*` |
| **C-02** durata reale del turno | ✅ sistema esistente | telemetria di pacing completa: `FRTPacingSample`, `URTPacingLibrary::SummarizeSamples`, `CountOpenedReactionWindows`, comando `rt.Debug.Pacing`, owner `docs/gameplay/spec-pacing-turno.md` |
| «estrattore di metriche dal TurnLog» | ✅ due già presenti | `FRTTestResult` (`TurnTraces`, `StateDiff`, assertion) e la telemetria di pacing |

## A.4 🔴 Il vincolo che il kit ignora: **D-102**

Blocchi A-05, B-01, B-02, B-03 producono numeri da partite **bot contro bot** e ne ricavano soglie di design
(«sotto il 50% il problema è il bot», «se TTK attivo ≈ TTK nudo la difesa non conta»).

> **D-102** — *«Un risultato bot-contro-bot non è evidenza di bilanciamento finché il bot non è certificato sulle
> capability che quel risultato produce.»* Se una capability decisiva è `FAIL` o `UNTESTED`, la conclusione
> ammessa è **«il bot non sa giocarla»**, non «l'eroe è debole». Gate aperto: `#543`, schema di competenza `#798`.

La partita della Parte B è l'istanza esatta: **una delle quattro unità non ha attaccato mai**. Qualunque soglia di
danno tarata su quella partita misurerebbe il pianificatore, non i numeri.

∴ Sopravvivono all'audit **due** cose, e il criterio è lo stesso: non passano da un bot non certificato.

1. Il **Δ del TTK con copertura Low** (A-02): scriptato, deterministico, nessun bot nel circuito. Non eseguito
   in questa sessione — §NOT RUN.
2. La **domanda** di B-01/B-03, che il kit poneva male ma poneva. La Parte B la riapre con un dato: la partita
   giocata qui finisce allo scadere. ⚠️ Ciò che `D-102` vieta non è **misurare** quel comportamento, è
   ricavarne una soglia di **bilanciamento**: «utilizzo ≥ 70% degli attacchi legali» resta inammissibile,
   «il bot resta fermo 26 volte su 43» è un fatto e si registra.

---

# Parte B — Partita 2v2 bot vs bot, osservata

## B.0 Come è stata avviata (procedura ufficiale, non inventata)

```
UnrealEditor-Cmd.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_HexArena/L_HexArena
  -game -RTAutobattle -useFixedTimeStep -fps=60 -unattended -nopause -nosplash -nullrhi -NoLiveCoding
```

- `-RTAutobattle` è la sorgente di mezzo delle tre dichiarate in `RTGameMode.cpp:130`
  (`proprietà < -RTAutobattle < rt.Match.Autobattle`): **entrambe** le squadre passano al bot.
- Formato in vigore: `Format.Skirmish2v2` — RoundLimit **12**, soglia obiettivo **5**, 2 unità per squadra.
- Formazioni spedite: **team 0** Gadget + Phase, **team 1** Riktor + Wraith (`RTMatchSetupLibrary`).
- Percezione, pathfinding, abilità, reazioni e `ARTTurnManager::PlanBots()` sono quelli del gioco: l'autobattle
  cambia **chi** è segnato come bot, non **come** decide (`RTGameMode.h:136`).
- Log integrale committato come evidenza:
  [`docs/technical/evidence/2269/partita-2v2-autobattle-2026-09-04.log`](../../technical/evidence/2269/partita-2v2-autobattle-2026-09-04.log)
  — 469 righe, il flusso `LogRT` ripulito dei soli prefissi di timestamp, sorgente `Saved/Logs/RefactorTactics.log`.
  ⚠️ Il **kit consumato** invece non è recuperabile: era un file non tracciato, letto per intero e rimosso
  secondo il mandato. Ciò che la Parte A ne cita è verificabile solo contro questo referto.

## B.1 Executive summary

I bot sembrano stupidi perché **la partita è giocata da due unità su quattro**. Riktor (team 1) attraversa dodici
turni senza sferrare un colpo, e Gadget (team 0) resta fermo per due turni mentre la compagna viene uccisa
**una cella più in là** — le quattro celle sono la distanza dal nemico, non dall'alleata. Non è un difetto di esecuzione: è la forma del punteggio. `WThreat` vale **100**, il
bonus «da qui potrò ingaggiare» vale **15** e decade, e un attacco vale `WDamage × danno` **solo se è legale
adesso**. Ne segue una regola implicita — *avanza soltanto se spari in questo turno* — che produce attesa quando
la linea di tiro manca, e oscillazione quando il contatto si perde e si riprende. Sopra questo, il piano è
compilato contro le posizioni del **momento della pianificazione**: in un turno simultaneo le due decisioni più
costose della partita (la carica di Riktor, la scarica di Gadget) hanno mancato perché il bersaglio si era mosso.
L'obiettivo, infine, **non entra nella decisione**: team 1 ha vinto 3-0 ai punti perché Riktor è finito tre volte
sulla cella `(0,-3)` come **migliore candidata di solo movimento** (`score=-40`, `-45`, `-40`) — cioè per
avvicinamento, minaccia e quota, mai per l'obiettivo. Ha vinto per caso.

## B.2 Timeline

Posizioni in `(q,r)`, layer 0. «piano» è il piano **loggato** per il turno successivo, col punteggio della
candidata vincente (le altre candidate non sono loggate: §B.9).

⚠️ **È un riassunto, non l'enumerazione di ogni colpo**: una riga porta ciò che quel turno decide, non tutte
le voci del TurnLog. I totali di §B.7 sono contati sul log integrale — 18 righe `Colpo:` — non su questa
tabella, che ne mostra meno.

| T | Gadget (t0) | Phase (t0) | Wraith (t1) | Riktor (t1) | Esito del turno |
|---|---|---|---|---|---|
| 1 | →(-1,1) | →(-2,2) | →(1,-1) | →(1,1) | nessun contatto, nessun piano loggato |
| 2 | resta | →(-2,1) | →(-1,-1) | resta | idem |
| 3 | resta | resta | →(-2,2) | →(1,-1) | primi piani: Gadget 130 · Phase 50 · Wraith 100 · Riktor **-40** |
| 4 | resta, **tiro fallito (no LOS)** | subisce 20+21 | scatto (-2,0), 2 colpi | →(0,-3) | **obiettivo team 1 +1** (0-1) |
| 5 | **→(-3,3)**, si sfila | scatto (-2,-1), colpisce 16 | subisce 16 | →(-1,-1) | Phase resta sola contro Wraith |
| 6 | resta (-3,3) | →(-4,3) | scatto (-1,-3), colpisce 21 | **carica a vuoto** | Riktor «resta» con `Ram` |
| 7 | →(-2,2) | scatto (-2,1), 16+16 | subisce 16 | →(0,-3) | **obiettivo +1** (0-2); piano di Riktor **non loggato** |
| 8 | resta, score **-50** | subisce 21 | scatto (-1,-2) | →(1,-1) | Wraith trova il letale: **score 9970** |
| 9 | resta, score **-50** | **eliminata** | uccide Phase | →(0,-3) | **obiettivo +1** (0-3) |
| 10 | attacca (190), reazione | — | scatto (-2,-2) | →(-1,-1) | primo scambio Gadget↔Wraith |
| 11 | attacca (**10080**) | — | attacca (70) | resta | Deflection di Wraith attiva |
| 12 | attacca (**10100**) | — | scatto (-2,1), muore | resta | **Wraith eliminata**; fine round 12 |

**Fine partita:** *«Vince il team 1 (rosso) — allo scadere dei round (round 12/12, obiettivo 0-3)»*.
1 KO per parte; a decidere sono i **3 punti obiettivo** di Riktor.

## B.3 Le cinque decisioni peggiori

### 1. T3→T4 — Gadget spara dove il bersaglio non è più

- **Stato**: Gadget `(-1,1)`, Wraith conosciuto a `(-2,2)` — **adiacente**, distanza 1. Il 2 è la distanza
  dalla cella in cui Wraith sarebbe finita **dopo** lo scatto, ed è già il difetto che questo caso descrive.
- **Decisione**: `utility -> (-1,1) attacca Wraith score=130` — resta e usa `LinearDischarge`.
- **Risultato**: `(-1,1) -> (-2,0): nessuna linea di tiro (Hero.Gadget.LinearDischarge)`. Turno perso.
- **Perché sembra stupida**: era il turno in cui il team 0 aveva il focus fire (Phase 50 + Gadget 130 sullo stesso
  bersaglio). Ne è arrivata metà.
- **Perché il codice l'ha scelta**: `ScorePlan` valuta gittata e LOS contro `Context.Enemies`, che è la posizione
  **al momento del planning**. Wraith ha scattato in fase **Dash**, che risolve *prima* del Blast. Nessun termine
  del punteggio modella il movimento avversario — e non può: l'intento nemico è informazione privata
  (`RTHexBotLibrary.h`, nota su `EnemyFacings`). **MODELLO V0.1.**

### 2. T4→T5 — Gadget si sfila mentre la compagna viene focalizzata

- **Stato**: Phase ha appena incassato 41 in un turno; Gadget `(-1,1)` è a 2 celle dal contatto.
- **Decisione**: `utility -> (-3,3) score=-35` — indietro di due celle, nessun attacco.
- **Risultato**: Gadget resta fuori dalla partita per T5-T6; Phase combatte da sola per cinque turni e muore.
- **Perché sembra stupida**: un secondo tiratore su Wraith avrebbe dimezzato il tempo di uccisione; ritirarsi
  regala il duello.
- **Perché il codice l'ha scelto**: nel punteggio **non esiste un termine di supporto all'alleato**. Gli alleati
  compaiono in `ScorePlan` solo come **penalità di fuoco amico** (`Context.Allies` → `WAllyDamage`). Restare sotto
  tiro costa `-WThreat = -100` per nemico che ha gittata **e** LOS; senza una candidata d'attacco legale, ogni
  cella vicina al combattimento è pura perdita. `-35` era il massimo disponibile. **MODELLO V0.1 + TUNING.**

### 3. T5→T6 — la carica di Riktor a vuoto

- **Stato**: Riktor `(-1,-1)`, `Ram` pronto (20 danni, `Push 1`), Phase conosciuta a `(-2,-1)`, adiacente.
- **Decisione**: `utility -> CARICA su Phase (impatto da (-1,-1)) score=90` — l'unico piano offensivo di Riktor in
  tutta la partita.
- **Risultato**: `Riktor: resta (-1,-1) (Hero.Riktor.Ram, p35)`. Phase si era spostata di 4 celle a `(-4,3)`.
- **Perché sembra stupida**: brucia il cooldown 2 dell'unica abilità con cui Riktor fa danno reale, e non muove
  nemmeno l'unità.
- **Perché il codice l'ha scelta**: identica alla n.1 — bersaglio valutato dove **era**. Aggravante strutturale:
  una carica è `Fast Movement` e risolve in **Dash**, quindi compete con lo scatto difensivo dell'avversario nella
  stessa finestra. **MODELLO V0.1.**

### 4. T7→T9 — Gadget guarda morire la compagna da fermo

- **Stato**: Gadget `(-2,2)` con `score=-50 (resta)` per due turni consecutivi; Wraith a `(-1,-3)` e poi
  `(-1,-2)` — distanza esagonale **5** e poi **4**; Phase a `(-2,1)`, **adiacente a Gadget**, che incassa 21
  a turno.
- **Decisione**: restare, due volte.
- **Risultato**: Phase eliminata al T9. Gadget attacca per la prima volta al T10 — **il turno dopo** il KO.
- **Perché sembra stupida**: la portata di `ArcPulse` è 4, e **sul secondo dei due turni** la distanza era 4
  — a tiro. Sul primo era 5, quindi lì il non-attacco è legale e non discutibile: resta discutibile lo
  **stare fermo** mentre l'alleata adiacente incassa 21 a turno.
- **Perché il codice l'ha scelta**: il piano d'attacco nasce solo se la cella di partenza ha gittata **e** LOS;
  `bQualcunoDaIngaggiare` era vero (qualche cella raggiungibile vedeva il nemico) quindi il ramo di ricerca del
  contatto **non** è scattato, ma tutte quelle celle costano `-WThreat = -100`, mentre restare costava `-50`. Il
  contrappeso previsto è `WEngage = 15`, che **decade** con `IdleTurns` (`RTHexBotLibrary.h:171`) e vale al più un
  sesto della minaccia. È il residuo dello stato assorbente di `#1088` spostato dal piano geometrico a quello
  numerico. **TUNING** (rapporto `WThreat`/`WEngage`), su una struttura sana.

### 5. Tutta la partita — Riktor non gioca

- **Stato**: 12 turni, 8 movimenti, 4 «resta», 1 piano d'attacco (quello a vuoto), **0 danni inflitti**.
- **Decisione**: piani di solo movimento con punteggio sempre negativo (`-20 · -30 · -30 · -30 · -40 · -40 · -45`),
  e **tre** turni senza alcun piano loggato (T1 e T2 con contatto assente, T7 con contatto noto ma non
  ingaggiabile).
- **Risultato**: oscillazione fra `(0,-3)`, `(-1,-1)` e `(1,-1)` per otto turni.
- **Perché sembra stupida**: è il 50% della potenza di fuoco della squadra, ferma a fare la spola.
- **Perché il codice l'ha scelta**: tre cause che si sommano, tutte verificabili.
  1. `ImpactShot` ha **gittata 3** e 8 danni (ADR-0007): la finestra in cui una candidata d'attacco esiste è la più
     stretta del roster, e Riktor ha 4 MP — il più lento.
  2. `DeriveKiteStandoff` (`RTHexBotLibrary.h:309`) restituisce 0 sotto gittata 5: Riktor è trattato come
     mischia e paga `-WApproach × MinDist`. Con `WApproach = 10` contro `WThreat = 100`, avvicinarsi a chi ti
     vede è sempre in perdita. ⚠️ **Non è l'unico termine positivo disponibile**: `WEngage` si applica
     proprio ai piani senza attacco (`RTHexBotLibrary.cpp:426`), cioè a tutti quelli di Riktor tranne la
     carica — ma vale al più 15 e **decade**, quindi non ribalta un −100.
  3. Quando il contatto non è ingaggiabile entra il ramo silenzioso di `RTTurnManager.cpp:1103` — punto di
     osservazione, altrimenti baricentro — che **non logga nulla** e non ha isteresi: la meta cambia appena il
     contatto si sposta, e l'unità torna indietro. **MODELLO V0.1**, con un pezzo di **TUNING**.

## B.4 Problemi confermati, per impatto

| Sev | Problema | Evidenza |
|---|---|---|
| **CRITICAL** | Nessuna coordinazione: il punteggio è per unità, gli alleati entrano solo come penalità di fuoco amico. Team 1 ha combattuto con una unità sola | Riktor 0 danni / 12 turni; `ScorePlan` non ha termini di supporto |
| **CRITICAL** | L'obiettivo non entra nella decisione: `FRTHexBotContext` (`RTHexBotLibrary.h:66`) non ha alcun campo obiettivo, zero occorrenze di `Objective` nel bot | 3 punti su 3 segnati da candidate di **solo movimento**; vittoria decisa da lì |
| **HIGH** | `WThreat` (100) schiaccia `WEngage` (15): avvicinarsi senza poter sparare **subito** è sempre in perdita → attesa e stallo | Gadget `-50 (resta)` ×2 con la compagna sotto fuoco |
| **HIGH** | Nessuna isteresi sulla meta di ricerca: oscillazione fra due celle | Riktor `(0,-3)`↔`(-1,-1)` per 8 turni; `#534` (CP 26.4) è aperta |
| **MEDIUM** | Piano compilato su posizioni stantie: attacchi e cariche a vuoto | 2 azioni sprecate su 18 colpi risolti |
| **MEDIUM** | Il ramo di ricerca del contatto è **muto**: 9 decisioni su 41 non compaiono nel log | T1 e T2 (4 unità ciascuno) più T7 (Riktor) |
| **LOW** | La riga `Pesi bot:` non stampa `WEngage`, `WEngageDecay`, `WAllyDamage` — proprio i pesi che spiegano l'attesa | `RTTurnManager.cpp:587` |

## B.5 Bug o limite

| Problema | Categoria | Evidenza | Codice |
|---|---|---|---|
| Nessun focus fire coordinato | **E26** | Riktor 0 danni mentre Wraith duella | `ScorePlan` per unità · D-097 |
| Obiettivo ignorato | **E26** | 3-0 per stazionamento accidentale | `FRTHexBotContext` · `spec-bot-tattico.md` §Objective `+120` |
| Attesa fuori portata | **TUNING** | `score=-50 (resta)` ×2 | `WThreat=100` vs `WEngage=15` (`RTHexBotLibrary.h:171`) |
| Oscillazione della meta | **E26** (CP 26.4) | `(0,-3)`↔`(-1,-1)` ×8 | `RTTurnManager.cpp:1103-1203` |
| Carica/tiro a vuoto | **MODELLO V0.1** | T4 e T6 | posizioni al planning; l'intento nemico è privato |
| Ramo di ricerca muto | **BUG (di osservabilità)** | 9 piani mancanti nel log | `RTTurnManager.cpp:1203` — `continue` senza `AddLogEvent` |
| `DecideReactionResponse` sceglie il primo `FIRE` | **non esercitato** | nessuna finestra Overwatch aperta | `RTHexBotLibrary.cpp:702` |

Nessun **BUG** di decisione: non è stato osservato un caso in cui il piano loggato fosse diverso da quello che il
modello corrente avrebbe scelto. L'unico bug è di **osservabilità**.

## B.6 Le otto anomalie chieste, una per una

- **A · Mancato focus fire** — confermato per il team 1 (Riktor mai sul bersaglio di Wraith). Il team 0 *ha*
  focalizzato al T3-T4 (Gadget 130 + Phase 50 su Wraith), ma per convergenza sul nemico più vicino, non per
  coordinamento: nessun termine del punteggio lega le due scelte.
- **B · Coordinazione** — nessun conflitto di prenotazione osservato: `ReservePlannedRoute` non ha mai dovuto
  arbitrare, perché le due unità di ogni squadra sono rimaste in aree diverse. Il difetto qui è l'opposto della
  contesa: **nessun setup/payoff, nessun crossfire, nessuna divisione di ruoli**.
- **C · Kiter panic** — **mai scattato**, 0 occorrenze di `arretra`/`scatto difensivo`. La guardia
  (`RTTurnManager.cpp:1283`) richiede `NearestDistance <= Standoff/2`, e `DeriveKiteStandoff` dà standoff > 0 solo
  a chi ha gittata ≥ 5: nel roster **solo Phase** (gittata 5 → standoff 3 → soglia **1**). Il ramo sospettato non
  è la causa di questa partita — misurato, non assunto.
- **D · Stalli e «resta»** — fase Move: **26 «resta» contro 17 movimenti** su 43 unità-turno. Sequenze senza
  ragione tattica evidente: Gadget T8-T9 (`-50` due volte, compagna sotto fuoco) e Riktor T11-T12.
- **E · Ricerca del contatto** — al T1-T2 tutte e quattro puntano il centro: è il ramo baricentro di
  `RTTurnManager.cpp:1148`, il cui commento dichiara il difetto — *«due compagne che cercano il contatto puntano
  ENTRAMBE la cella più vicina al centro, che è una sola»*. Nessun settore, nessuna memoria di dove si è guardato.
- **F · Objective blindness** — **confermata al livello del codice**, non dedotta: `FRTHexBotContext`
  (`RTHexBotLibrary.h:66`) non ha campi obiettivo e `RTHexBotLibrary.{h,cpp}` non contiene la parola, in nessuna
  delle due lingue. I 3 punti sono l'effetto collaterale di **candidate di solo movimento** scelte dal ramo
  utility (`utility -> (0,-3) score=-40` al T3, `-45` al T6, `-40` al T8): la cella vince per approach/minaccia,
  e il punto arriva dopo. Il resolver **sa** contare le presenze (`URTTurnRules::ResolveObjectiveControl`,
  chiamato nel Cleanup): è il bot a non guardare.
- **G · Threat evaluation** — `WThreat` è **binario per nemico**: `-100` se quel nemico ha gittata e LOS sulla
  cella, indipendentemente da quanto quel nemico possa fare. Un `ImpactShot` da **8**, un `PulseShot` da **21**
  e un `PassingBlade` da **20** (`RTHeroCatalogLibrary.cpp:862`) generano la stessa identica penalità.
  Confronto diretto in questa partita: Riktor e Wraith pesano uguale sulla cella che entrambi coprono, con un
  fattore 2,6 di danno fra loro.
- **H · Uso delle reaction** — **42 armamenti** registrati e **4 attivazioni** (ReactiveCapacitor
  di Gadget al T10, Deflection di Wraith al T11). Riktor ha armato `Interposition` in tutti i 12 turni senza mai
  essere vicino all'alleato che avrebbe dovuto coprire. Nessuna finestra Overwatch si è aperta, quindi
  `DecideReactionResponse` — che restituisce il **primo `FIRE` legale** — **non è stato esercitato**: qui non è
  misurabile se esistesse un bersaglio migliore.

## B.7 Metriche

| Metrica | Valore |
|---|---|
| Turni | **12** (RoundLimit raggiunto) |
| Vincitore | team 1 (Riktor+Wraith), **allo scadere**, obiettivo 0-3 |
| Danno inflitto team 0 → team 1 | **166** (Phase 96, Gadget 70) |
| Danno inflitto team 1 → team 0 | **188** (Wraith 188, **Riktor 0**) |
| KO | 2 — Phase (T9), Wraith (T12) |
| Colpi risolti | **18** |
| Movimenti (fase Move) | **17** |
| «resta» (fase Move) | **26** |
| Scatti | **9** |
| Cariche | **1** (a vuoto) |
| Reazioni armate / attivate | **42 / 4** |
| Turni senza danno | **3** (T1-T3) |
| Turni senza contatto noto | **2** (T1-T2) |
| Punti obiettivo | 3, tutti del team 1, tutti da Riktor su `(0,-3)` |
| Occasioni di focus fire perse | **non misurabile** — richiede l'insieme delle candidate, che non viene loggato |
| Overkill | **non significativo**: entrambi i colpi letali sono arrivati in un 1v1 senza bersagli alternativi |
| Conflitti fra piani alleati | **0 osservati** |
| Fughe del kiter | **0** |

## B.8 Confronto con E26

I quattro difetti CRITICAL/HIGH cadono **esattamente** dentro il perimetro già deciso di E26 (`#326`), il cui
owner tecnico è `docs/gameplay/spec-bot-tattico.md`:

| Rimedio previsto | Sede | Lo giustifica questa partita? |
|---|---|---|
| Top-K per unità | CP 26.1 (`#531`) | ✅ senza alternative in gioco, il bot non può scambiare un piano peggiore per sé con uno migliore per la squadra |
| Combinazione centrale dei piani | CP 26.1 · **D-097** | ✅ è il rimedio diretto a «Riktor non gioca» |
| Hard/soft conflicts | CP 26.3 (`#533`) | ⚠️ **non giustificato da questa partita**: zero conflitti osservati. Resta previsto, ma non è ciò che rompe qui |
| Overkill avoidance | CP 26.3 | ⚠️ idem: nessun overkill significativo |
| Temporal synergy | CP 26.2 (`#532`) · **D-098** | ✅ la carica in Dash contro il tiro in Blast è precisamente una compatibilità di fase |
| Objective utility | `spec-bot-tattico.md` — bucket `Objective`, categoria **+120** | ✅ **la voce più giustificata di tutte**: ha deciso la partita |
| Reaction policy | testo dell'epic | ⛔ non giustificato: la policy non è stata esercitata |

Isteresi (CP 26.4, `#534`) non è nell'elenco chiesto ma è **misurata qui**: l'oscillazione di Riktor.

## B.9 Cosa il logging non permette di sapere

Dichiarato invece che aggirato:

- **Le candidate e i loro punteggi**: viene loggata solo la vincente (`RTTurnManager.cpp:1496 e seguenti`). Non è
  ricostruibile se una candidata migliore esistesse — quindi nessun «BUG» di selezione è affermabile.
- **Le decisioni del ramo di ricerca del contatto**: 9 su 41 non compaiono affatto (T1, T2, e Riktor al T7).
- **HP turno per turno**: derivabili dai colpi applicati, non asseriti dal log. La derivazione per Phase
  (95 HP, scudo 5/turno) coincide con l'eliminazione al T9, il che dà fiducia — ma resta derivata.
- **`WEngage` / `WEngageDecay` / `WAllyDamage`**: non stampati fra i pesi.

## B.10 Le tre modifiche con più impatto

1. **Dare al bot l'obiettivo.** È l'unica voce che ha deciso il risultato di questa partita e la più economica:
   un campo nel contesto e un termine nel punteggio, con il valore già scritto in `spec-bot-tattico.md`
   (`Objective +120`). Oggi la condizione di vittoria del formato spedito è invisibile a chi gioca.
2. **Passare dalla scelta per unità alla combinazione di squadra** (D-097, CP 26.1). «Riktor non gioca» non è
   tarabile: nessun peso su un punteggio per unità può far preferire a Riktor una posizione che vale poco *a lui*
   e molto *alla squadra*. **È architetturale, e va detto chiaramente.**
3. **Dare isteresi alla meta di ricerca del contatto** — CP 26.4 (`#534`). È la metà tarabile del terzo
   problema: toglie l'andirivieni di Riktor senza toccare pesi già misurati.

   🔴 **E NON «ritarare il rapporto `WThreat`/`WEngage`», che è la prima cosa che verrebbe da fare e che il
   codice ha già escluso.** Il docstring di `WEngageDecay` (`RTHexBotLibrary.h:172-191`) lo scrive:
   *«la coppia si tara sull'ESITO, non con una formula: non è il rapporto a decidere»*. Quattro punti
   misurati — `15/5` ✅, `20/10` ✅, `20/5` 🔴, `30/10` 🔴 — e senza decadimento la finestra è **vuota**:
   `Match.Autobattle.EngagesOnTheGeneratedTestArena` cade da `W = 7`, `NobodyParksOnTheAuthoredMap` si sblocca
   solo da `W = 11`, e fra 7 e 10 sono rossi entrambi. Alzare `WEngage` contro `WThreat` come rapporto esce
   dalla finestra misurata e rende rossi due oracoli di parcheggio.

   ⚠️ L'invariante che questo referto citava qui — `WElevation × MaxLayer < WApproach`
   (`HexBot.ElevationNeverOutweighsClosingOneCell`) — è vera ma di **un'altra coppia**: vincola l'elevazione,
   non l'ingaggio. Il presidio giusto è `HexBot.EngageBonusFadesWithIdleTurns`, sull'esito di
   `ChooseBestPlan`. La taratura fine resta bilanciamento: `#149` e `D-102`.

⚠️ Le tre proposte sono **osservazioni, non un mandato**: D-102 chiede che una metrica bot-contro-bot porti con sé
lo stato di competenza delle capability che la producono, e quello schema (`#798`) non esiste ancora.

## B.11 Incrocio con il brief esterno comparso durante la sessione

Alle 09:57, mentre questa misura era in corso, nell'albero è comparso un secondo artefatto non tracciato:
`RefactorTactics_Bot_Analysis_Claude_Consolidation.md` (1087 righe). **Non è stato consumato** — non era il
mandato di questa sessione — e non è stato modificato. Dichiara nel proprio corpo di riferirsi a
`DegrassiAaron/refactor-tactics-main` e di **non essere un'autorità** sul repository vivo.

Vale registrare l'incrocio, perché le due analisi sono indipendenti e di natura diversa: quella è una lettura
del **codice**, questa è una **misura** di una partita.

⚠️ Le citazioni qui sotto vengono tutte dal **§1 Executive summary** del brief, che è dove quelle
formulazioni compaiono: le sezioni indicate nella prima stesura di questa tabella rimandavano ai capitoli che
sviluppano il tema, non al testo citato.

| Tesi del brief | Cosa aggiunge questa misura |
|---|---|
| «No true joint team planning» (§1) | ✅ confermata con un numero: **una unità su quattro non attacca mai** in 12 turni |
| «One-turn greedy reasoning» (§1) | ✅ confermata: due azioni a vuoto (T4 tiro, T6 carica) su 18 colpi risolti |
| «Objective reasoning is weak/not integrated into the current core utility» (§1) | ✅ confermata e **rafforzata**: non è debole, è **assente** dal contesto del bot — e in questa partita ha deciso il risultato (3-0) |
| «No-contact search is **intentionally** primitive» (§1; §8 aggiunge *«Do not "fix" this…»*) | ⚠️ **Non è una debolezza che confermo: è una scelta dichiarata**, e la prima stesura di questa riga la citava senza l'avverbio che la regge. Ciò che aggiungo è un'altra cosa: quel ramo è anche **muto** — 9 decisioni su 41 non compaiono nel log — e l'osservabilità non è una scelta di design |
| «Kiter panic … may cause tactically poor escapes» (§1), e §6 ne chiede verifica comportamentale | 🔴 **corretta**: in questa partita il ramo **non è mai scattato**. `DeriveKiteStandoff` lo abilita solo sopra gittata 5 — nel roster **solo Phase** — e la soglia `Standoff/2` vale **1**. Prima di scrivere test su «kiter panic vs letale garantito» conviene misurare se quel ramo sia raggiungibile nel formato spedito |
| «Reaction response selection is primitive» (§1) | ⚠️ **non falsificabile qui**: nessuna finestra Overwatch si è aperta, quindi `DecideReactionResponse` non è stato esercitato |

---

## Verifiche eseguite

| Cosa | Esito |
|---|---|
| Build `RefactorTacticsEditor` Win64 Development | ✅ `Result: Succeeded`, 83,6 s, su `b063a60f` |
| Partita 2v2 bot-vs-bot su `L_HexArena`, autobattle | ✅ completata, 12/12 round, log integrale conservato |
| Stato di `#1922`, `#1918`, `#1088`, `#326`, `#543` su GitHub | ✅ interrogati (`gh issue view`) |
| Presenza dei test `HexSim.ResolveSwapBlocked` / `ResolveClosedCycleBlocked` / `ResolveFreeTailConvoyStillAdvances` | ✅ verificata nel sorgente |

## NOT RUN

- `./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario` — **NON AVVIATA**: il motore è stato preso da un'altra
  sessione (`D:\Repositories\rt-wt-2260`, run di automation viva). Non terminata: è lavoro di qualcun altro.
- **A-02** — Δ del TTK con copertura Low: l'unica voce del kit che sopravvive all'audit, non eseguita.
- Nessuna seduta PIE: la partita è stata osservata headless.
- `DecideReactionResponse` non esercitato: nessuna finestra Overwatch nella partita.

## Rischi e aperti

- La partita è **una** run. Il determinismo è strutturale (nessun RNG), quindi ripeterla identica darebbe lo stesso
  log: variare significa cambiare spawn, mappa o formazione — non «rilanciare».
- I numeri di questa partita **non sono evidenza di bilanciamento** (D-102): tre delle quattro unità hanno usato una
  frazione del proprio kit.
- Il ramo muto di `RTTurnManager.cpp:1203` limita ogni analisi futura sullo stesso materiale.
