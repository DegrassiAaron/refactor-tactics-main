# TD Trial / Scenario Sandbox — spec panel sul prompt di aggiornamento roadmap

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **consumato e archiviato**, non applicato ·
> **Data**: 2026-08-29
> **HEAD della revisione**: `8aa73027` (`origin/main`), letto in un worktree pulito — nessuno dei due
> checkout condivisi poteva ospitare la misura (§14).
> **Oggetto**: `Claude_TD_Trial_Roadmap_Update_Prompt.md` (936 righe, **diciassette** sezioni `§0`–`§16`), un mandato di
> riconciliazione documentale e di tracking che formalizza una **TD Trial / Scenario Sandbox** come primo
> loop utilizzabile del Tactical Designer. Letto contro `Source/RefactorTactics/ScenarioHarness/`,
> `Source/RefactorTacticsEditor/`, `Source/RefactorTactics/Replay/`, le dodici issue che nomina,
> `spec-tactical-designer.md`, `roadmap-checkpoint.md`, `roadmap-v0.1.md`, il Decision Log e ADR-0010.
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-29-td-trial-scenario-sandbox.md`](../../archive/src/handoff/2026-08-29-td-trial-scenario-sandbox.md)

---

## 1. Il verdetto in una riga

Il prompt è **il più disciplinato dell'archivio sui divieti e il più esposto sulla collocazione**: i suoi
guardrail — «no second simulator», «non trasformare #472», «non creare D-nnn automaticamente», «non infilare
la Trial nella v0.1 dalla porta di servizio» — descrivono esattamente le trappole che questo repository ha
già pagato; ma **la parola «Editor», che il prompt usa come collocazione del lavoro in tutte le sue slice, è
misurabilmente falsa**, e la decisione che chiede di prendere al §2 è già scritta in un ADR di due giorni fa
che il prompt non elenca fra le fonti.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **3** |
| 🟡 Medio | **6** |
| ➕ Trovato misurando, non è del prompt | **1** |

**Raccomandazione operativa**: **eseguire il prompt, ma non prima di aver corretto C1 e C2.** Sono i due
difetti che si propagano: `C1` non nomina la porta attraverso cui il lavoro deve passare, e `C2` fa
deliberare una scelta già fatta — il costo non è il tempo, è che la delibera può uscire diversa dall'ADR. Le
altre correzioni **riducono** il lavoro: quattro delle nove Trial Slice hanno un owner che esiste già.

> 🔵 **Questa sezione è stata riscritta dopo la code review della PR #1620, che ha trovato dieci difetti in
> questo referto.** Il conteggio diceva `4 · 4 · 5`, e tre voci non reggevano alla riverifica: `C3` grada
> come critica un'istruzione che il prompt condiziona correttamente, `A3` attribuiva a `TD 1.0` la
> descrizione di `TD 0.9`, e `M4` **dichiarava assente un test che esiste**. Le correzioni sono segnate
> 🔵 sul posto invece che applicate in silenzio: un referto che misura un sorgente e sbaglia le proprie
> misure deve mostrare dove, o insegna a fidarsi del formato invece che del numero.

⚠️ Nessuna suite eseguita, nessuna build, nessuna scrittura su GitHub, nessuna issue creata o modificata.
Issue lette lato server con `gh` il 2026-08-29; `Source/` e `docs/` a `8aa73027`.

---

## 2. Baseline misurata

La §2 del prompt dichiara una baseline e chiede di riverificarla. Verificata: **le dieci issue che elenca
sono tutte esatte**, più il conteggio delle sub-issue — undici affermazioni su undici. Va detto, perché è
raro in questo archivio.

> 🔵 **Corretto dopo la code review**: questa riga diceva «dieci righe su undici sono esatte», e il numero
> non corrispondeva a nessuna lettura della tabella qui sotto — che ha nove righe, o dodici espandendo la
> riga `#1114 · #1115 · #1116 · #1117`. Il conteggio giusto è sulle **affermazioni del §2**, che sono dieci
> issue più il «5 su 9»: **tutte vere**. Le due righe 🔴 della tabella vengono dal §10 del prompt, non dalla
> sua baseline, e sono marcate come tali.

| Issue | Prompt dice | Misurato 2026-08-29 | |
|---|---|---|---|
| #1105 Tactical Designer | OPEN | **OPEN** | ✅ |
| #1114 · #1115 · #1116 · #1117 | CLOSED | **CLOSED** | ✅ |
| #622 workspace grid | OPEN | **OPEN** | ✅ |
| #623 DevSandbox lights | CLOSED | **CLOSED** | ✅ |
| #695 door visualization | OPEN | **OPEN** | ✅ |
| #711 movement probe | OPEN | **OPEN** | ✅ |
| #472 replay viewer | OPEN | **OPEN** | ✅ |
| #1515 Scenario Harness validation | §10, non §2 | **OPEN** | ✅ |
| #1540 StateHash coverage | §10, condizionato | **CLOSED** | 🟡 `C3` |

L'affermazione «l'epic #1105 risultava con **5 sub-issue completate su 9**» **è esatta, e si misura in una
chiamata**: `subIssues` di #1105 risponde `totalCount: 9` con cinque `CLOSED` — #623, #1114, #1115, #1116,
#1117.

> 🔵 **Corretto dopo la code review**: questa riga diceva che l'affermazione «non è verificabile dal corpo
> dell'epic» ed era «coerente» — vero del *corpo*, ma il numero non viene dal corpo: viene dalla vista
> sub-issue, che è interrogabile. Dichiarare non verificabile ciò che non si è provato a misurare è il
> difetto che questo referto contesta ai sorgenti, e l'header dichiarava già di aver letto le issue lato
> server. Il rimando a `C4` era anch'esso sbagliato: `C4` parla della regola d'apertura e della tabella
> stale, non del 5/9.

⚠️ **E la nona sub-issue non era in questo referto**: **#1186** — *«Il pannello del mode non dice cosa
contiene la mappa: celle, layer e asset collegato si scoprono premendo qualcosa»*, `OPEN` — è dentro la
superficie della Trial (è il pannello dell'editor mode) e non compariva né qui, né in `C4`, né nella tabella
del §13. È anche assente dall'elenco «Il lavoro aperto oggi» del corpo di #1105, il che la rende **una voce
di `C4`**, non una sua eccezione.

---

## 3. 🔴 C1 — il Composer consegnato non vive nel modulo Editor, e il prompt lo colloca lì ovunque

Il prompt chiede issue «**Editor-only**» (§T1), «UI Editor» (§2), «Editor Scenario Playback» (§7.1),
«quali elementi sono Editor/PIE» (§7.2), e vieta il codice runtime in §0.6. Misurato:

```text
find Source/RefactorTacticsEditor -iname "*Scenario*" -o -iname "*Composer*"
→ (vuoto)
```

Il Composer consegnato da #1115–#1117 vive **interamente nel modulo runtime**:
`Source/RefactorTactics/ScenarioHarness/` — `RTScenarioDraft.{h,cpp}` (il ViewModel C++ puro),
`RTScenarioAuthoring.{h,cpp}` (la facade `UObject`), `RTScenarioSession.{h,cpp}`, `RTScenarioWriter.cpp`.
L'authoring visuale è **Blueprint/UMG che chiama quella facade**, non un `FEdMode`.

Questo non è un dettaglio di cartella: è **ADR-0010** (`CANONICAL`, 2026-08-27), che decide *«la porta è una
facade `UObject`, e il modello non passa mai per Blueprint»*, tiene le nove `USTRUCT` del formato
**non-`BlueprintType`**, e ha un test guardiano che verifica **entrambi i versi** —
`RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint`. ⚠️ Su **otto** delle nove: il §13-bis.

**Fowler**: il difetto non è la parola «Editor» — è che il prompt non nomina mai **la porta**. Un'issue
intitolata «Editor-only visual playback» sotto #1105 non dice a chi la esegue che il contratto da consumare
è `URTScenarioAuthoring` e che il modello è fuori portata: lascia aperte due strade che l'ADR vuole chiuse —
esporre di più (rompendo `non-BlueprintType`) o reimplementare in C++ d'editor ciò che la facade già fa. La
seconda **passa il test guardiano**, che verifica la raggiungibilità del contratto, non che nessuno lo
aggiri.

> 🔵 **Riformulato dopo la code review, che ha smontato la versione precedente.** Diceva che chi prende
> l'issue «ha due strade, e sono le due che l'ADR ha chiuso», e prescriveva di **sostituire ovunque** la
> parola «Editor». Falso: ADR-0010 §4 sancisce esplicitamente una **terza** strada — *«Se un giorno servirà
> un punto d'accesso unico in Editor, sarà un `UEditorSubsystem` di poche righe **sopra questa stessa
> facade**, nel modulo Editor, dove un punto d'accesso d'editor appartiene»*. E l'Editor è la sede giusta
> per la *superficie*: #1115 mette il Composer «nel Hex Map viewport», e la catena del §3 della spec owner
> termina in «visualizzazione d'editor». La correzione utile è **additiva** — nominare la facade — non
> sostitutiva.
>
> ⚠️ **E il modo in cui la versione precedente aveva sbagliato è quello contro cui la spec mette in guardia
> per nome**: un `find` vuoto su `RefactorTacticsEditor` prova che *oggi* lì non c'è niente di scenario, non
> che lì non debba starci — ed è lo stesso salto (*«in `Source/RefactorTacticsEditor/` non esiste alcun
> test»*) che `spec-tactical-designer.md` §3 dichiara **già costato quattro volte**. Questa era la quinta.

**Cockburn**: e c'è un effetto sull'attore. «Editor» dice *un programmatore con Visual Studio*; il Composer
consegnato è usabile da *un designer con l'Editor aperto e un widget*. La Trial dichiara al §6 di servire il
secondo. La collocazione la scrive per il primo.

⛔ **ADR-0010 non è nella lista fonti del §1**, che elenca dodici file e include ADR-0009. È l'unica
decisione che governa *come* il lavoro della Trial può essere scritto.

### Correzione
**Aggiungere ADR-0010 al §1**, e in ogni slice dire *attraverso cosa* passa il lavoro: la UI vive dove ha
senso — viewport, widget, all'occorrenza un `UEditorSubsystem` sottile — e **chiama `URTScenarioAuthoring`**,
mai il modello. È una riga per slice, e sposta il vincolo dal luogo alla porta, che è dove ADR-0010 lo mette.

> 🔵 **Ritirata dopo la code review una terza prescrizione**: «riformulare il divieto §0.6, che oggi vieta
> codice runtime/editor». Il §0.6 dice *«Non implementare codice runtime/editor **in questa attività**»* —
> il qualificatore c'era e la citazione lo aveva perso. È un divieto sulla **passata di riconciliazione**,
> non sulle slice future: corretto com'è, e questo referto lo ha rispettato (§15).

---

## 4. 🔴 C2 — il playback ha già il suo core, e la scelta A/B/C del §2 è già decisa

Il §2 chiede di **stabilire** se il core replay «può essere riusato» (A), se esiste già un'issue equivalente
(B), o se una porzione di #472 è condivisibile (C), ed esprime una preferenza: *«riuso del core, ownership
separata della UI»*.

Misurato, quella preferenza **è già l'architettura, non una proposta**:

| Capability chiesta da T1 | Dove esiste già |
|---|---|
| `NEXT/PREV PHASE` | `URTReplaySeekLibrary::SeekToPhase` |
| `NEXT/PREV TURN` | `SeekToTurn` · `SeekToTurnPhase` |
| esito tipizzato del seek | `ERTReplaySeekResult` |
| stato di riproduzione | `FRTReplayPosition` · `FRTReplayManifest` |
| logica separata dalla UI | `FRTReplayViewModel`, con test propri (`RTReplayViewModelTests.cpp`) |
| superficie per una UI | `URTReplayViewerSubsystem` — **36** `UFUNCTION` |

E ADR-0010 lo dichiara esplicitamente come **il precedente da cui prende la propria forma**:
*«`URTReplayViewerSubsystem` espone diciannove `UFUNCTION` a una UI di replay, e la logica non sta lì […]
Questo ADR non sceglie una forma nuova: dichiara che quella è la forma, e la estende allo Scenario Harness.»*

**Wiegers**: un requisito che chiede di *decidere* una cosa già decisa non è neutro. Ha tre esiti e due sono
peggiori dello stato attuale — l'esecutore può scegliere `B` e cercare a vuoto, o `C` e negoziare una fetta
di #472 che nessuno gli deve dare. Il §2 va riscritto da *domanda* a **vincolo**: la UI di playback del
Composer è un secondo consumer del ViewModel replay, come ADR-0010 ha già stabilito per l'authoring.

**Nygard**: ciò che resta davvero da costruire è più piccolo di quanto il §T1 lasci credere, ed è esattamente
la lista che il prompt mette in coda: `speed 0.25x…Instant` e la **resa graybox** degli eventi (attack line,
AoE, push, KO, status). Il trasporto c'è.

⚠️ **Ma la difesa di #472 nel prompt è corretta e va tenuta**: #472 è player-facing, e #1525 — *«il playback
anima il movimento di ogni unità, anche di quelle che la squadra non vede»* — dimostra che il playback di
partita ha vincoli di **privacy della conoscenza** che il playback d'authoring non ha. Fondere i due attori
romperebbe #1525 dal lato sbagliato. La separazione di ownership che il prompt chiede è giusta; è la
deliberazione sul riuso a essere già chiusa.

---

## 5. 🟡 C3 — #1540 è chiusa, e il gate hash della Trial poggia su di lei

> 🔵 **Declassato da 🔴 dopo la code review, e la ragione vale più del grado.** Il §10 dice *«**se** i test
> guardiani […] sono ancora aperti, dichiarali come rischio»*: è un'istruzione **condizionata**, il prompt
> non afferma nulla sullo stato di #1540, e chiedere di misurare è esattamente ciò che un kit deve fare.
> Non c'è niente da correggere nel prompt — quindi non è un difetto del prompt, e gonfiava il conteggio dei
> critici su cui poggia la raccomandazione del §1. Resta qui perché **la misura è utile**, non perché il
> sorgente abbia sbagliato.

Il §10 prescrive: *«#1540 — se i test guardiani per Shield/Energy/Layer sono ancora aperti, dichiarali come
rischio/dipendenza di verifica»*. Misurato: **#1540 è `CLOSED`**.

**Crispin**: la conseguenza non è una riga di prosa da togliere. Il gate `T0` del §5 chiede *«same
StateHash/LogHash quando previsti»*, e #1540 era precisamente il difetto per cui quello StateHash **non
discriminava** scudo, energia e layer. Con #1540 aperta, il gate `T0` sarebbe stato un oracolo cieco su tre
assi — verde su due esecuzioni che differiscono. Chiusa, il gate vale. Il prompt lo elenca fra i rischi;
misurato, è **la ragione per cui il gate è proponibile**.

---

## 6. 🔴 C4 — #1105 porta una regola d'apertura che i fatti hanno smentito, e il prompt non la nomina

Il §8 chiede genericamente di «eliminare stato palesemente stale». Lo stato stale principale non è una
casella: è una **regola**. Il corpo di #1105 dichiara, sotto «Cosa NON è stato aperto, e perché»:

> *«Le issue di TD 0.2 e TD 0.3 **non esistono**, e non è una dimenticanza […] Si aprono quando **TD 0.1
> chiude**, o la loro prima riga sarebbe "serve un consumatore che non esiste".»*

**TD 0.2 è** *«creare uno scenario senza scrivere JSON»*. È stato aperto e **chiuso** come #1114–#1117, con
TD 0.1 ancora `🟡 quasi chiuso` (#622, #695, #711 aperte). La regola non è stata rispettata — ed è andata
bene: il consumatore c'era. Nel corpo dell'epic restano quindi, tutte insieme:

- la tabella di maturità che marca **TD 0.2 `⬜`**, mentre il Composer Lite è consegnato;
- «Il lavoro aperto oggi» che elenca **#623**, chiusa;
- `Feature ID: RT-FEAT-TOOL-*` in testa e `known_roadmap_refs()` nel corpo — identificatori orfani da
  **D-181**, che il §0.9 del prompt vieta di riesumare **ma solo nei documenti**;
- nessuna menzione di #1114–#1117, che sono il lavoro più rilevante che l'epic abbia prodotto.

**Adzic**: la correzione utile non è spuntare TD 0.2. È scrivere **cosa si è imparato**: la regola «prima il
consumatore» è stata sospesa una volta e ha retto perché il formato esisteva già. Se resta scritta com'è, il
prossimo lettore la applicherà a TD 0.3 — lo Skill Workbench — dove il consumatore davvero non c'è, e non
saprà che è già stata piegata una volta e perché.

---

## 7. 🟠 A1 — `feature-registry` in `spec-tactical-designer.md`: tre occorrenze, non una, e una è un comando

Il §7.1 prevede il difetto («il documento contiene/ha contenuto testo che dice che lo stato vive nel
`feature-registry.yaml`») e ha ragione. Misurate **tre** occorrenze `CURRENT`, di gravità diversa:

| Riga | Testo | Gravità |
|---|---|---|
| 14 | *«Lo stato di implementazione vive nel `feature-registry.yaml` e nelle issue»* | la prescrizione che il §7.1 prevede |
| 294 | *«misurabile: `feature_registry.py validate`»* | 🔴 **un comando eseguibile che non esiste** — prescrive un metodo di verifica, non un puntatore |
| 372 | tabella: *«A che punto è una capability → `feature-registry.yaml` e le viste generate»* | riga di una tabella di owner: manda a un owner inesistente |
| 373 | tabella: *«Quale seduta d'editor fare, e in che ordine → `editormap.shortlist.md`»* | 🔵 **quarto puntatore morto, trovato in code review**: quel file **non esiste in nessun punto del repository** — è una delle cinque `*.shortlist.md` generate, uscite con **D-181**. L'owner giusto è `editor-sessions.yaml`, che la riga 14 dello stesso documento cita correttamente |

La riga 294 è la peggiore perché è un **criterio di misura**: chi verifica quel criterio non trova un link
rotto, trova un comando che fallisce, e non sa se il difetto è suo. Le righe 372 e 373 sono in una tabella —
e nessun gate le vede: `doc-tables.ts --check` controlla la larghezza delle righe, non il significato delle
celle, e `doc-links.ts` non può aiutare perché sono **code span, non link**.

Le quattro vanno corrette in **tre modi diversi**. La 14, la 372 e la 373 sostituendo l'autorità — issue
GitHub e `roadmap-checkpoint.md` per lo stato, `editor-sessions.yaml` per le sedute; la 294 sostituendo il
**metodo**, perché oggi quel criterio non ha una misura automatica e va detto, invece che rimpiazzato con un
comando inventato. Una `sed` sulle quattro produrrebbe una riga 294 che promette una misura che nessuno
esegue.

> ⚠️ **Che le occorrenze fossero quattro e non tre lo ha trovato la code review, non questa passata** — e la
> mancata era nella **riga immediatamente sotto** una che avevo citato. È la stessa lezione che questo
> referto rivolge altrove ai sorgenti: un elenco compilato leggendo *ciò che si cerca* non vede la riga
> accanto. Il grep era su `feature-registry|feature_registry|Control Center`, e `editormap.shortlist.md` non
> contiene nessuna delle tre parole pur essendo lo stesso difetto.

---

## 8. 🟠 A2 — il §T2 modella un selettore di azione, e il formato non ne ha uno

Il §T2 elenca otto «action» da authorare (`Wait · Move · Basic Attack · Ability · Dash · Brace · Overwatch ·
Interact`) e prescrive: *«Per ogni action la UI deve compilare il dato canonico esistente»*.

`FRTScenarioIntent`, misurata: **nessun enum azione**. I campi sono

```text
UnitId · Move[] · Ability · Target · Dash · DashCell · TargetCell · bTargetsCell
CoverEdge · bHasCoverEdge · Reaction · Condition · Facing · bDeclaresFacing
```

e il doc header della struct dice cosa sono:

> *«L'intento di una unità in un turno: movimento, abilità, **o entrambi**. Movimento e abilità convivono
> perché convivono nel gioco — un'unità ha uno **slot movimento** e uno **slot principale**, e usarli
> insieme è la norma, non un caso limite.»*

**Fowler**: il modello è **due slot che coesistono**, non una scelta fra otto. Una UI costruita sulla lista
del §T2 — un selettore, poi le proprietà dell'azione scelta — rende *inesprimibile* «muovi e attacca», che
la struct dichiara essere la norma. E la lista mescola **due vocabolari**: `Wait · Move · Basic Attack ·
Brace · Overwatch · Interact` sono **sei dei sette** `Action.*` generici di **D-025**, cioè il vocabolario
del **gioco**; `Ability` e `Dash` sono **campi del formato**. Nel formato i primi passano attraverso
`Ability` e `Reaction`: modellarli come enum di scenario creerebbe il vocabolario parallelo che l'ultima
riga dello stesso §T2 vieta.

⚠️ **E il settimo manca.** `Guard` non è nella lista del §T2, mentre `Brace` c'è — e sono la coppia che
`Spec.Brace.GuardAndBraceOnMixedHit` misura *al confine*, quello scenario essendo citato dalla struct stessa
per giustificare `bLoadoutDeclared`. Un authoring che espone l'una e non l'altra non può esprimere il
confine che il repository ha uno scenario apposta per misurare.

### Correzione
Riscrivere il §T2 come **due slot più i loro modificatori** (`Move[]` · slot principale `Ability`/`Dash` ·
`Reaction` + `Condition` · `Facing` · target per unità o per cella). La lista delle otto azioni resta utile
come *checklist di copertura* — «il designer riesce a esprimere ciascuna di queste?» — non come struttura
della UI.

---

## 9. 🟡 A3 — la scala è `TD 0.1 … TD 1.0`, non `TD 0.9`

Il §5 vieta di introdurre numerazioni che confliggano con «la scala di maturità Tactical Designer
`TD 0.1 ... TD 0.9`». **D-154** e il corpo di #1105 la definiscono `TD 0.1 … TD 1.0`: dieci stadi, e lo
stadio omesso è *«fare tutto il giro senza leggere il codice sorgente»* — la chiusura del cerchio che dà
senso a tutta la scala, e proprio il traguardo che la TD Trial insegue.

> 🔵 **Corretto e declassato da 🟠 dopo la code review, che ha trovato l'argomento rovesciato.** Questa voce
> diceva che `TD 1.0` è *«promuovere una variante a dato di produzione con un gate»* e quindi «il solo che
> tocca i dati di gioco reali». Quella frase è la riga **`TD 0.9`**, verbatim, della tabella di #1105 e di
> `spec-tactical-designer.md`. Lo stadio col rischio di produzione è dunque **dentro** il range che il
> prompt cita, non fuori: la citazione amputata resta un errore, ma costa meno di quanto questa voce
> affermava, ed è per questo che scende di grado invece di essere riscritta più forte.

---

## 10. 🟠 A4 — il referto owner esiste già, e il §11 non lo cita

Il §11 chiede di creare `docs/roadmap/plans/tactical-designer-trial-reconciliation-2026-08-29.md` con tredici
sezioni, fra cui «Stato reale del Tactical Designer» e «Gap misurati». Esiste già
[`tactical-designer-consolidamento-2026-08-17.md`](tactical-designer-consolidamento-2026-08-17.md), ed è il
referto **da cui `spec-tactical-designer.md` dichiara di essere nato**.

Non è un divieto — i referti sono datati e si accumulano, ed è questo il modello dell'archivio. È che le
sezioni 3 e 5 chieste dal §11 sono le stesse che quel referto ha già misurato: chi esegue senza averlo letto
le riscrive da zero, e le due misure divergeranno sui punti dove il codice si è mosso — senza che nessuna
delle due dica quale sia la più recente.

---

## 11. 🟡 Medi

| | Punto | Misura |
|---|---|---|
| M1 | ADR-0010 scrive *«diciannove `UFUNCTION`»* su `URTReplayViewerSubsystem` | misurate **36**. Un numero con una data, scaduto in due giorni. Non cambia la tesi dell'ADR |
| M2 | Il §0.9 vieta di riesumare il Feature Registry, **senza qualificatori** | e #1105 lo porta in testa (`Feature ID: RT-FEAT-TOOL-*`), quindi è **già dentro il divieto**: il §8, che riscrive l'epic, lo applica senza bisogno di estenderlo. 🔵 *Questa riga diceva «vieta nei documenti … il divieto va esteso all'epic»: il «nei documenti» non è nel prompt, e il lavoro che ne seguiva era creato dal misquote* |
| M3 | Il §13 chiede di cercare link rotti «con gli strumenti presenti nel repo» | `tools/radar/doc-links.ts --check` esiste, e cammina il **filesystem**: in regime **D-222** un verde su working directory condivisa non prova che l'albero regga. Va dichiarato l'albero su cui si misura |
| M4 | Il gate `T0` chiede `Editor Run == Headless Run` | ✅ **esiste ed è verde**: `RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun` ([`RTScenarioRunResetTests.cpp:91`](../../../Source/RefactorTactics/Tests/RTScenarioRunResetTests.cpp)) esegue lo stesso scenario per `URTScenarioRunner::Run` e per `URTScenarioAuthoring::Run` e confronta esito, turni, passed/failed e **StateHash**. Il suo commento lo chiama *«la dimostrazione dell'assenza del secondo simulatore»*, ed è arrivato con #1117. 🔴 **Questa riga diceva «non esiste: nessun test confronta le due esecuzioni», ed era falsa** — trovata in code review. Era l'errore peggiore del referto: dichiarare assente una prova esistente manda ad aprire lavoro già consegnato, che è precisamente ciò che la §16 del prompt vieta |
| M5 | Il §T5 elenca i gap del formato (status iniziali, ambiente, seed, override) | confermati dallo spec §5 e da #1105. ⚠️ `Seed` è un caso a parte: è *«dichiarato e NON consumato»* — authorarlo nella UI prima che il runtime lo consumi darebbe al designer una leva che non muove niente |

---

## 12. Cosa il prompt ha ragione, e va tenuto

Va detto perché è la parte maggiore del documento e sopravvive intera:

- ✅ Il §4 (invariante architetturale) **dice la stessa cosa** di `spec-tactical-designer.md` §3 e del corpo
  di #1105, e la dice senza deformarla. 🔵 *Corretto dopo la code review: questa riga affermava «è lo stesso
  testo, correttamente citato», e non lo è — il diagramma del prompt scrive «regole runtime» e «Runtime
  Query», termina in «Editor / TD Trial» invece che in «visualizzazione d'editor», e aggiunge un blocco «È
  vietato» a sei voci che la spec non ha. È una **riformulazione fedele**, che è un complimento più piccolo
  ma vero — e certificare un verbatim che non c'è, nella sezione intitolata «cosa il prompt ha ragione», era
  esattamente la classe di affermazione non guadagnata che il resto del documento contesta.*
- ✅ Il §7.3 — *«non infilare la TD Trial nella v0.1 dalla porta di servizio»* — è esattamente ciò che
  **D-154** decide e che `roadmap-v0.1.md` conferma: il tooling è `out_of_release_scope`, e `E46` lo
  ribadisce per le sezioni DEV/TEST del menu.
- ✅ Il §12 — non creare un `D-nnn` automatico — è la prescrizione giusta, e la verifica che chiede ha esito
  **negativo**: D-154 + ADR-0009 + ADR-0010 coprono già la decisione che il §12 propone come potenzialmente
  nuova (*«il playback è un consumer del replay canonico, non un percorso di simulazione»*).
  ⛔ **Nessun `D-nnn` va aperto.** L'ultimo assegnato al momento della revisione è **D-233**, e va
  riverificato sui ref remoti prima di qualunque merge (CLAUDE.md §7).
- ✅ Il §10 protegge correttamente #622, #695, #711 e #472 dallo scope creep, e la §16 («la riuscita non si
  misura dal numero di nuove issue») è la riga migliore del documento.
- ✅ Il §7.4 tiene la distinzione fra le quattro viste — spec / checkpoint / release / issue — che questo
  repository ha già pagato per confondere.

---

## 13. Trial Slice → owner misurato

Con le correzioni di §3–§10 applicate. **Stato** è ciò che esiste a `8aa73027`, non ciò che il prompt assume.

| Slice | Capability | Owner misurato | Stato | Azione |
|---|---|---|---|---|
| **T0** | baseline loop | `FRTScenarioSession` · `URTScenarioAuthoring` · #1114–#1117 | ✅ consegnato **e coperto** | **nessuna issue**: il confronto Blueprint↔headless è già `RunFromTheEditorMatchesTheHeadlessRun` (`M4`) |
| **T1** | visual playback | `FRTReplayViewModel` + `RTReplaySeekLibrary` (core) · UI da fare | 🟡 core c'è, resa graybox no | **issue nuova**, consumer Blueprint — non `#472`, non Editor module |
| **T2** | intent authoring | `FRTScenarioIntent` (due slot) | 🟡 formato c'è, UI Move/Wait sola | **issue nuova**, modellata sui due slot (`A2`) |
| **T3** | multi-turn | `FRTScenarioTurn` · `Turns[]` | 🟡 dato c'è, UI no | **issue nuova**, piccola |
| **T4** | reaction/decision | `FRTScenarioDecision` (`FIRE`/`HOLD`, `Target` vietato con `HOLD`) | 🟡 dato c'è, UI no | **issue nuova** · dipende da `T1` per il feedback visivo |
| **T5** | initial state / env | `FRTScenarioUnit` (+ `bLoadoutDeclared`) · gap in spec §5 | 🟡 parziale | **una issue per gap**, non una sola. `Seed` differito (`M5`) |
| **T6** | probes | #711 (movement) · LOS/targeting assenti | 🟡 uno su molti | **aggiornare #711** solo per movimento · sorelle sottili per il resto |
| **T7** | result inspector | `GetLastRunReport()` · `GetLastRunLog()` sulla facade · #1117 | 🟡 **result e TurnLog consegnati**, State Diff no | **issue nuova sul solo State Diff**. 🔵 *Questa riga diceva «nessun inspector equivalente trovato»: metà di `T7` è esposta e coperta da `RefactorTactics.Scenario.RunExposesAReadableTurnLog` — una issue indistinta avrebbe rispecificato una superficie consegnata* |
| **T8** | preset/template | — | ⬜ | **post-Trial**, come il prompt stesso suggerisce |
| **—** | pannello del mode | **#1186**, `OPEN` | 🟡 aperta | **già ha un owner**: nona sub-issue di #1105, dentro la superficie della Trial e assente dalla prima stesura di questo referto |

**Ordine**: quello proposto dal §15.E regge, con una precisazione — `T4` dipende da `T1` (una decisione
`FIRE`/`HOLD` che non si vede accadere non è authorabile a occhio). Il §15.E li mette già in quest'ordine: il
dependency graph lo conferma invece di correggerlo.

---

## 13-bis. ➕ Trovato misurando: il guardiano di ADR-0010 protegge otto struct su nove

Non è un difetto del prompt, ed è la cosa più utile emersa dall'intera passata — trovata in code review
verificando un'affermazione del §3 di questo referto.

`RTTestScenario.h` dichiara **nove** struct del formato. L'array `MustStayInternal` di
[`RTScenarioAuthoringTests.cpp:275`](../../../Source/RefactorTactics/Tests/RTScenarioAuthoringTests.cpp) ne
elenca **otto**:

```text
FRTTestScenario · FRTScenarioUnit · FRTScenarioIntent · FRTScenarioTurn
FRTTestExpectation · FRTScenarioVariant · FRTScenarioCell · FRTScenarioDecision
                                                          ⟵ manca FRTScenarioVariantUnit
```

**Nygard**: il commento del test dice cosa dovrebbe impedire — *«Se un giorno qualcuno marcasse
`FRTTestScenario` come `BlueprintType`, Blueprint potrebbe costruirne uno membro per membro […] e ADR-0010
cadrebbe senza che nulla diventi rosso. Questo lo rende rosso.»* Per `FRTScenarioVariantUnit` **non lo rende
rosso**: marcarla `BlueprintType` lascerebbe Blueprint costruire l'override di unità di una variante membro
per membro, e il guardiano resterebbe verde. È il caso che il test esiste per prendere, sulla sola struct che
la sua lista non nomina.

⚠️ **E non è una svista qualsiasi**: `FRTScenarioVariantUnit` è il pezzo di `FRTScenarioVariant` che porta il
*contenuto* della variante, cioè esattamente la superficie su cui `TD 0.3`/`TD 0.4` costruiranno. Il buco è
davanti alla direzione in cui la Trial si muove, non dietro.

**Azione**: una riga nell'array, e un'issue piccola che la porti. Non è lavoro della Trial e non va messo
nel suo gate — ma va aperto prima che qualcuno estenda le varianti, perché da quel momento il guardiano
starebbe mentendo su una struct che qualcuno sta toccando.

---

## 14. Come è stata protetta la misura

**D-222**: nessuno dei due checkout condivisi poteva ospitare questa revisione — `refactor-tactict-dev` era
**31 commit indietro** rispetto a `origin/main` con cinque file C++ modificati da un'altra attività;
`refactor-tactics-main` era 18 indietro, su un branch con lavoro non committato di un'altra sessione.
Misurare nel primo avrebbe prodotto un referto su un albero vecchio di 31 commit; scrivere nel secondo
avrebbe mescolato due lavori. La revisione è stata fatta in un **worktree** creato da `origin/main`
(`8aa73027`), che per lavoro puramente documentale non tocca il mutex del motore.

---

## 15. Cose non fatte

- ⛔ **Nessuna scrittura su GitHub.** Nessuna issue creata, modificata o chiusa; #1105 non è stato toccato. Il
  §8 e il §9 del prompt **non sono stati eseguiti**: sono la parte outward-facing del mandato e restano una
  decisione dell'autore.
- ⛔ **Nessun documento owner aggiornato.** `spec-tactical-designer.md` porta ancora le tre occorrenze di
  `A1`; `roadmap-checkpoint.md` M9.4 e `roadmap-v0.1.md` sono invariati.
- ⛔ **Nessun `D-nnn` assegnato** (§12: verificato che non serve).
- ⏸️ **Il documento chiesto dal §11 non è stato creato**: questo referto ne copre le sezioni 1–10 e 12–14 e ne
  differisce sul nome. Crearne un secondo con lo stesso contenuto sarebbe il difetto che il §7.4 vieta.
- ⏸️ **Non misurato**: se `Brace` e `Interact` siano esprimibili come intent — richiede di leggere la
  traduzione intent→azione, che non è in `RTScenarioRunner.cpp`. Il §T2 lo condiziona già correttamente a
  *«se il vocabolario runtime la tratta come intent»*.
- ⚠️ **Non consumato**: `CLAUDE_RefactorTactics_HUD_Mockup_Issue_Doc_Consolidation_2026-08-28_v0.3.md`, il
  secondo file untracked alla radice di `refactor-tactics-main`. È materia di un'altra sessione, che ha già
  archiviato i kit gemelli — *GrayKit v0.1* e *Icon Grammar* sono atterrati con
  [#1618](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1618).
  > 🔵 **Corretto dopo la code review**: questa riga nominava il branch `docs/consolidamento-combat-skillgrammar-delta`
  > come se fosse la rete. `git ls-remote --heads origin` elenca **nove** ref e quello non c'è: è un branch
  > **locale di un altro checkout**, e citarlo qui è lo stesso difetto che **CLAUDE.md §7** nomina —
  > *«la rete sono i ref REMOTI»* — commesso nel documento che lo cita.
