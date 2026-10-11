# La finestra di reazione dopo `U74` — spec panel sui tre seguiti di #3634

> `SNAPSHOT` · fotografia del 2026-10-11: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-11.
> - **Misurato su** `origin/main` = `11e628c85` (il merge di [#3664](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3664)).
> - **Oggetto**: i tre seguiti che la cattura della seduta `U74` ([#3634](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3634)) ha lasciato:
>   - **S1**: la finestra si apre prima che il playback abbia mostrato l'ingresso (`PIE-V01-RXPLAYBACK`);
>   - **S2**: il titolo e le descrizioni come la tavola E — chi è in guardia, chi è entrato, «Attacca <bersaglio>» — con la revisione di privacy;
>   - **S3**: `freeRun` e le altre chiavi degli scenari, lette con `TryGetBoolField`, accettano la stringa `"true"`.
> - **Panel**: Wiegers (lead) · Fowler · Nygard · Adzic · Crispin. **Modo**: critique. **Focus**: requirements, architecture, testing.
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo referto.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce, ed è
> l'esito di questo passaggio.

---

## 1. Il verdetto in una riga

| | Verdetto | Classificazione | Owner |
|---|---|---|---|
| **S1** | La finestra si apre quando il **resolver** tocca il confine, non quando il **playback** lo ha mostrato. La lettera di [D-355] («durante il playback») è rispettata, perché il playback è partito; la sua ragione no. Il gate che doveva vederlo misura che il playback sia *partito*. La correzione non è uno spostamento di chiamate: va decisa a ogni sospensione, e cambia il significato della slow-motion | `IMPLEMENTATION DRIFT` contro la ragione di una decisione accettata, più una **decisione minima richiesta** (§3.3) | [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) |
| **S2** | Il titolo neutro «Reazione» è una **guardia voluta**, non una dimenticanza: il DTO porta un indice di snapshot, e risolverlo nel widget nominerebbe l'unità sbagliata. I nomi vanno nel DTO, risolti dove l'indice vale. La privacy regge per l'Overwatch; per il `Brace` serve una regola | la metà mancante di una casella aperta di #166 («UI `FIRE`/`HOLD` con countdown **e bersaglio**») | [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) |
| **S3** | La disciplina «un tipo sbagliato non è un'assenza» del loader è **nominale** per ogni chiave scalare: l'engine converte in quattro direzioni, e una stringa letta come intero dà sempre `true`. Il corpus non ha nessun valore col tipo sbagliato, quindi la correzione non tocca i dati | difetto di validazione del formato | harness degli scenari |

---

## 2. Le premesse, verificate prima del panel

La ricognizione è fatta qui, una volta. Ogni riga dice come è stata verificata.

| # | Premessa | Fonte | Esito |
|---|---|---|---|
| P1 | [D-355] vuole la finestra **durante** il playback, perché *«al giocatore si chiederebbe `FIRE`/`HOLD` su un movimento che il suo schermo non ha mostrato»* | Decision Log, voce `D-355` | ✅ vera |
| P2 | In PIE la finestra si apre circa 0,1 s dopo l'inizio della Risoluzione: header su PREP, Ivrin nella cella di partenza. L'ingresso si vede dopo la scadenza | seduta `U74`, raffica `r13` (59 fps): `Risoluzione` alle 06:53:32,797, finestra a 2,9 s alle 32,997, scadenza alle 35,78, header su MOVE alle 36,14 | ✅ misurata |
| P3 | `Reactions.WindowOpensWithPlaybackRunning` legge `IsResolving()` dentro il delegate: prova che il playback è **partito**, non che abbia mostrato l'ingresso | `Tests/RTMovementResumeTests.cpp:548`; il suo commento lo dichiara | ✅ vera |
| P4 | Il momento giusto esiste già: `FinishPlayback` alza `bPlaybackHeldByWindow` quando il playback parziale ha mostrato tutto il risolto, *«l'istante in cui il nemico è entrato nella zona»* | `Turn/RTTurnManager.cpp:9226` | ✅ vera |
| P5 | L'orologio della finestra corre dall'apertura, a prescindere dal playback | `TickReactionWindow`, `Turn/RTTurnManager.cpp:192` | ✅ vera |
| P6 | La slow-motion (0,35) parte con la finestra: l'avvicinamento si vede a un terzo della velocità mentre l'orologio corre in tempo reale. È così che la finestra scade prima dell'ingresso | `UpdateReactionSlowMotion`, `Turn/RTTurnManager.cpp:236`; P2 | ✅ vera |
| P7 | I siti di apertura sono due, e hanno la stessa forma | `Turn/RTTurnManager_Movement.cpp:1093` (Overwatch), `Turn/RTTurnManager_Blast.cpp:2741` (`Brace`) | ✅ vera |
| P8 | Il titolo neutro è voluto: *«il DTO porta un `TargetSnapshotIndex`, non un id stabile, e risolverlo qui nominerebbe l'unità sbagliata in ogni partita in cui qualcuno è già caduto»* | `URTFastDecisionWidget::GetPromptText`, `UI/RTScreenHudWidgets.cpp:1639` | ✅ vera |
| P9 | L'Overwatch scatta solo su un'unità **`Detected`** dalla squadra del watcher **e** in linea di vista dalla cella raggiunta | `Turn/RTReactionOpportunityTypes.cpp:589-599`; ADR-0004 §6 | ✅ vera |
| P10 | Il `Brace` non controlla la rilevazione di chi spinge, e la sua chiave non lo nomina: lo snapshot è `Omniscient`, e serve solo all'indice del proprietario | `Turn/RTTurnManager_Blast.cpp:2711` | ✅ vera |
| P11 | La vista va solo alla squadra del proprietario: l'avversario non riceve nulla, nemmeno nel turno in cui il proprietario è rivelato | `URTReactionWindowLibrary::FilterWindowForTeam`, `Turn/RTReactionWindowView.cpp:5` | ✅ vera |
| P12 | #166 ha aperta la casella «UI `FIRE`/`HOLD` con countdown **e bersaglio**, alimentata da un DTO sanitizzato» | corpo di #166 | ✅ vera |
| P13 | L'engine converte, e risponde sempre `true`: stringa→booleano col valore di `ToBool()` (`"yes"` → vero, `"false"` → falso); numero→booleano `!= 0`; stringa→intero con `LexFromString` (`"abc"` → 0); booleano→numero 1/0; numero e booleano→stringa. E un decimale letto come intero si **arrotonda** (`RoundHalfFromZero`: 2,5 → 3) | `Runtime/Json/Public/Dom/JsonValue.h:451-497`, `Private/Dom/JsonValue.cpp:95-107` e `440-492` | ✅ vera |
| P14 | Nel loader le letture scalari passano da `TryGet*Field`, e i controlli espliciti del tipo sono pochi | `grep -c` su `ScenarioHarness/RTScenarioLoader.cpp`, in questo passaggio: `TryGetBoolField` 8, `TryGetNumberField` 22, `TryGetStringField` 45; `Type != EJson::` / `Type == EJson::` / `HasTypedField` 4 | ✅ misurata |
| P15 | Nel corpus nessun valore ha il tipo sbagliato | script in §5.3, su ogni `Scenarios/**/*.json`: **zero** | ✅ misurata |
| P16 | Dopo una risposta, una seconda sospensione **non estende** il playback: `ResumeSuspendedResolution` torna ad attendere senza `BeginPlayback`, che gira solo alla conclusione. Il playback resta trattenuto sul **primo** ingresso mentre la seconda finestra chiede di un altro | `Turn/RTTurnManager_Movement.cpp:983-998`; `bPlaybackHeldByWindow` torna falso solo in `BeginPlayback` (`Turn/RTTurnManager.cpp:7938`) | ✅ vera (dalla verifica avversaria) |
| P17 | Il playback parziale può non partire: `BeginPartialPlayback` non fa niente se il playback è spento o già partito, o se la timeline è vuota | `Turn/RTTurnManager_Movement.cpp:1161` e `:1190`; `Turn/RTTurnManager.cpp:149-151` | ✅ vera (dalla verifica avversaria) |
| P18 | `Detected` significa identità nota: *«contatto pieno: la squadra vede la cella e chi la occupa»*. `Uncertain` è *«senza identità»* | `Perception/RTPerceptionLibrary.h:18-29` | ✅ vera |
| P19 | Il playback è acceso per default anche nei test: `bEnablePlayback = true` | `Turn/RTTurnManager.h:1278` | ✅ vera |

---

## 3. S1 — La finestra dopo l'ingresso

### 3.1 Il fatto

Il resolver risolve in `LockInAndResolve` fino al confine, e lì **sospende**. Nello stesso istante esegue
`OnReactionWindowOpened`, e l'orologio della finestra comincia a correre. Il playback parziale parte da capo: Prep, poi i
micro-step del Move fino all'ingresso. Con la slow-motion attiva, ci arriva a un terzo della velocità.

Nella scena del banco `Visual.Reaction.OverwatchWindow` la finestra è scaduta **prima** che il playback mostrasse
l'ingresso (P2). Chi decide guarda un countdown sopra un mondo in cui il nemico non si è ancora mosso: è esattamente il
caso che [D-355] esiste per escludere (P1).

### 3.2 Il panel

**WIEGERS** — Il requisito c'è, ma è scritto come intenzione (*«durante il playback»*) e implementato come condizione
debole («il playback è partito»). Va riscritto in una forma che si può falsificare: *la finestra diventa visibile, e il
suo countdown comincia, nel fotogramma in cui il playback parziale si trattiene sul confine*. Il confine è l'ultimo
micro-step risolto, cioè `bPlaybackHeldByWindow`.

**FOWLER** — Oggi due fatti diversi hanno un solo segnale. Il resolver è **sospeso** (stato logico, deciso dal
confine), e la finestra è **presentata** (stato di UI, deciso dal playback). Il delegate e l'orologio si agganciano al
primo, e invece appartengono al secondo. Servono due eventi e un flag, «presentata»:
- il resolver sospende come oggi: `OpenWindowOpportunityId`, `PendingOpportunity` e il replay non cambiano;
- **a ogni sospensione** si decide la presentazione, secondo **tre** stati del playback:
  - **spento, o timeline vuota** (P17): la finestra si presenta subito, come oggi;
  - **non ancora partito e avviabile**: parte il playback parziale, e la finestra si presenta quando si trattiene;
  - **già in corso o già trattenuto**, come nella seconda sospensione dopo una risposta (P16): la timeline si
    **estende** fino al confine nuovo, e la finestra si ripresenta al nuovo trattenimento. ⚠️ Non è il caso «non può
    partire»: `BeginPartialPlayback` qui esce senza fare niente (`bIsResolving` vero), e leggerlo come «spento»
    presenterebbe la seconda finestra sul primo ingresso, cioè il difetto di P16;
- `OnReactionWindowOpened` si esegue alla presentazione;
- `TickReactionWindow` fa scorrere `OpenWindowElapsed` solo a finestra presentata.

⚠️ La prima stesura di questo referto diceva *«nessuna struttura nuova: si sposta il punto di due chiamate»*. La
verifica avversaria l'ha smentita: senza la regola della seconda sospensione e quella del playback assente, la
seconda finestra non si presenterebbe mai e il turno resterebbe appeso.

**NYGARD** — Due vincoli che la correzione non può violare.
- **Autorità.** ADR-0004 §7-bis vuole il countdown *«server-authoritative: un client lento non allunga la
  finestra»*. In v0.1 il server e il client sono lo stesso processo ([D-155]), e l'istante in cui il playback locale
  si trattiene è un fatto del server. In **M10** non lo sarà più. Lì l'inizio va calcolato dal server: la durata del
  playback parziale è una funzione deterministica degli eventi risolti e della manopola di velocità. Si scrive come
  vincolo nella issue di M10 ([#759](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759)), non si
  costruisce ora.
- **Canale temporale.** La pausa osservata dall'avversario (ADR-0004 §7-bis) diventa «playback fino al confine + la
  finestra». Era già variabile per la risposta del proprietario, e la correzione non aggiunge un'incognita: la parte
  nuova è deterministica.

**ADZIC** — L'esempio che la issue deve portare, sul banco che esiste già:

> **Dato** il banco `Visual.Reaction.OverwatchWindow` (Aevik in guardia verso W, Ivrin che entra in (1,-1)),
> **quando** il turno si risolve in PIE con la finestra in primo piano,
> **allora**:
> 1. prima che il playback mostri l'ingresso, la finestra **non** è visibile, il countdown non corre, e l'header segue
>    la fase che il playback sta mostrando;
> 2. nel fotogramma in cui il playback si ferma su Ivrin dentro (1,-1), la finestra compare con 3,0 s e l'header dice
>    MOVE;
> 3. alla risposta, o alla scadenza, il movimento riprende da quel fotogramma, senza ripartire da capo.

**CRISPIN** — Il gate attuale è sulla superficie sbagliata (P3), e va sostituito, non affiancato.
- Il nuovo, headless: nel delegate, la fase mostrata dal playback è quella del confine (`Move` per l'Overwatch, `Blast`
  per il `Brace`), e il micro-step mostrato è quello della chiave dell'opportunità. Dopo N tick col playback ancora
  prima del confine, `OpenWindowElapsed` vale zero.
- La mutazione che lo deve uccidere è la forma di oggi: il delegate eseguito alla sospensione.
- La voce `PIE-V01-RXPLAYBACK` diventa eseguibile sul banco: la precondizione «partita 2v2 live con un nemico che
  percorre la rotta» si allestisce da sé.
- ⚠️ La cattura va fatta con la finestra **in primo piano**: fuori fuoco l'Editor gira a circa 3 fps, e la finestra
  si disegna a scatti (`U74`, raffica `r10`).

### 3.3 `DECISIONE MINIMA RICHIESTA` — che cosa rallenta la slow-motion

[D-350] e #166 dicono che la slow-motion avviene *«durante la finestra»*. Oggi parte alla sospensione
(`UpdateReactionSlowMotion` legge `GetOpenReactionWindowId`), e sospensione e finestra sono lo stesso istante. Con la
correzione si separano, e la frase deve scegliere fra due letture:

| | Lettura | Conseguenza |
|---|---|---|
| **(a)** | La slow-motion va dalla sospensione alla presentazione: rallenta l'avvicinamento, poi il mondo si ferma sull'ingresso | È ADR-0004 §5, *«il mondo fermarsi su un momento di tensione»*. L'attesa prima della finestra si allunga di circa tre volte sul tratto parziale: è pacing, e si misura in #166 con `ReactionWindowPlaybackSpeed` |
| **(b)** | La slow-motion vale da finestra presentata | Durante la finestra il mondo è già fermo: la slow-motion non rallenta più niente, ed è di fatto tolta |

**Raccomandazione del panel: (a).** Non cambia una riga di `UpdateReactionSlowMotion`, ma cambia ciò che
*«durante la finestra»* significa in [D-350] e in #166. Va scritto come emendamento a [D-355] prima di chiudere la
issue: è una decisione d'autore, non un dettaglio d'implementazione.

### 3.4 La specifica

- **R1.1** La finestra di reazione diventa visibile nel fotogramma in cui il playback parziale si trattiene sul
  confine (`bPlaybackHeldByWindow`), non quando il resolver sospende. Vale per i due siti, l'Overwatch e il `Brace`.
- **R1.2** A ogni sospensione la timeline parziale si estende fino al confine nuovo, anche dopo una risposta (P16).
- **R1.3** La presentazione segue tre stati del playback (§3.2): spento o con la timeline vuota, si presenta alla
  sospensione; non ancora partito, parte e si presenta al trattenimento; già in corso o trattenuto, si estende e si
  presenta al nuovo trattenimento. Il ritorno anticipato di `BeginPartialPlayback` non distingue gli ultimi due, e la
  implementazione deve farlo.
- **R1.4** Il countdown (`OpenWindowElapsed`) corre solo a finestra presentata, e prima vale zero.
- **R1.5** Il replay, il Verifier e il bot non cambiano. `RecordedDecisions` precede l'apertura come oggi, e senza
  delegate legato non c'è presentazione.
- **R1.6** In M10 l'istante di presentazione è un evento del server, calcolato dalla durata del playback parziale.
  Vincolo da scrivere in #759.
- **R1.7** Con il playback in pausa (`K`, o `rt.Debug.PlaybackStartPaused`), `TickPlayback` esce prima del
  trattenimento (`Turn/RTTurnManager.cpp:8755`): la presentazione si **rimanda** alla ripresa, e l'orologio non corre.
  È voluto, ed è fuori dalla partita normale: i comandi di playback vivono dietro `rt.Debug.PlaybackControls`. Non
  contraddice [D-351], che vieta di fermare un countdown **già visibile**: qui la finestra non c'è ancora.

**DoD**
- [ ] La decisione di §3.3 è registrata come emendamento a [D-355].
- [ ] `Reactions.WindowOpensWithPlaybackRunning` è sostituito da un test che asserisce fase e micro-step mostrati al
      momento dell'apertura, e l'orologio fermo prima. Muore con il delegate eseguito alla sospensione.
- [ ] Lo stesso test per il `Brace`.
- [ ] Un test per la **seconda finestra consecutiva**: la timeline arriva al secondo ingresso prima della presentazione.
- [ ] Un test per il **playback spento**: la finestra si presenta alla sospensione, e il turno si chiude.
- [ ] Un test per il **playback in pausa**: la presentazione aspetta la ripresa, e `OpenWindowElapsed` resta a zero.
- [ ] I test che legano il delegate e si aspettano l'apertura subito dopo `LockInAndResolve` pompano il playback fino
      al trattenimento: il playback è acceso per default (P19). Sono in `Tests/RTMovementResumeTests.cpp`,
      `Tests/RTDefensiveReactionTests.cpp` e `Tests/RTReactionWindowViewModelTests.cpp`, e l'elenco esatto si misura
      in implementazione.
- [ ] `PIE-V01-RXPLAYBACK` eseguita sul banco `Visual.Reaction.OverwatchWindow`, con la finestra in primo piano.
- [ ] Il commento di `RTMovementResumeTests.cpp:548` e quello di `FinishPlayback` sono riallineati.

---

## 4. S2 — Titolo e descrizioni come la tavola, con la revisione di privacy

### 4.1 Il fatto

La tavola E scrive tre cose:
- il titolo «Guardia reattiva — Aevik»;
- il sottotitolo «Ivrin è entrato nell'area controllata»;
- le opzioni «FIRE · Attacca Ivrin» e «HOLD · Non attaccare».

Il gioco scrive «Reazione», nessun sottotitolo, e le frasi del catalogo, che sono più lunghe
(`UI/RTReactionResponseText.cpp`). Il titolo neutro è una guardia voluta (P8): il widget non ha un id stabile da cui
ricavare un nome.

### 4.2 Il panel

**FOWLER** — La guardia ha ragione sul *dove*, non sul *se*. L'indice di snapshot vale **solo** nel momento in cui il
turn manager costruisce la vista. È lì, in `MakeReactionWindowView` (`Turn/RTTurnManager.cpp:7366`), che i nomi vanno
risolti e messi nel DTO come testo già pronto. Il widget non risolve niente.

⚠️ Oggi `MakeReactionWindowView` è solo un involucro di `FilterWindowForTeam`, e non vede né lo snapshot né le
unità: la firma cambia, e i due siti le passano. Al sito del `Brace` lo snapshot si costruisce solo quando serve
(`Turn/RTTurnManager_Blast.cpp:2707-2713`). Campi nuovi della vista, tutti `FText`:
- `ReactionName`: il nome di catalogo della reazione, «Guardia reattiva»;
- `ReactorName`: il nome dell'unità del proprietario;
- `TriggerNames`: chi ha fatto scattare la finestra;
- `TargetName` su ogni opzione `FIRE:<i>`.

**NYGARD — la revisione di privacy.** Il DTO attraversa il confine di privacy, e ogni campo va giustificato.
- **`ReactionName` e `ReactorName`**: sono della squadra del proprietario. La vista va solo a quella squadra (P11), e
  l'avversario non riceve nulla. Nessun rischio.
- **`TriggerNames` per l'Overwatch**: il trigger esiste solo per unità `Detected` dalla squadra del watcher, e in
  linea di vista (P9). Il nome è già noto a chi riceve la vista, e il bersaglio di `FIRE` è la stessa unità. Nessuna
  informazione nuova.
- **`TriggerNames` per il `Brace`**: è il punto aperto. Chi spinge non è soggetto alla rilevazione (P10), e può spingere
  da una cella che la squadra non vede. **Regola**: si nomina solo un'unità che la squadra del proprietario ha
  `Detected` in quel momento (P18). Il Blast non ha una conoscenza di squadra propria: il predicato è
  `URTPerceptionLibrary::TeamAwarenessOfCell` (`Perception/RTPerceptionLibrary.h:112`), con gli osservatori della
  squadra costruiti come per l'Overwatch (`Turn/RTTurnManager_Movement.cpp:566-585`, o `BuildRouteObserverTeams` in
  `Turn/RTTurnManagerInternal.h:59`). Altrimenti il sottotitolo è neutro: «Un nemico ti spinge».
- **Alleato in sola lettura**: riceve la stessa vista, e la conoscenza di squadra è condivisa. Nessun rischio.
- **Determinismo**: la vista non entra nel TurnLog né nell'hash, e i nomi non cambiano nessun esito.

**WIEGERS** — I testi devono stare nella specifica, non nella testa di chi la implementa:
- titolo: «{Reazione} — {Proprietario}»;
- sottotitolo, Overwatch: «{Unità} è entrato nell'area controllata»; con due bersagli sullo stesso micro-step: «{A} e
  {B} sono entrati nell'area controllata»;
- sottotitolo, `Brace`: «{Unità} ti spinge», o «Un nemico ti spinge»;
- `FIRE`: «Attacca {bersaglio}»; `HOLD`: «Non attaccare»;
- le risposte di profilo (`Hold Ground`, `SIDESTEP`) tengono le frasi del catalogo, perché la tavola non le mostra.

**ADZIC** — Gli esempi che la issue deve portare:

> 1. **Bersaglio singolo**: sul banco `Visual.Reaction.OverwatchWindow` la finestra dice «Guardia reattiva — Aevik» e
>    «Ivrin è entrato nell'area controllata», e le opzioni «Attacca Ivrin» e «Non attaccare».
> 2. **Indice spostato**: un'unità è caduta nel Blast prima del Move, e gli indici di risoluzione scalano. Il nome resta
>    quello dell'unità che è entrata. È il difetto che la guardia di P8 esiste per impedire.
> 3. **Spinta da un'unità non rilevata**: «Un nemico ti spinge», e nessun nome nel DTO.

**CRISPIN** — Tre test, e una dipendenza.
- **Headless**:
  - i nomi del DTO coincidono con le unità dell'esempio 2;
  - il DTO del `Brace` non porta il nome di un'unità non rilevata;
  - la vista dell'osservatore avversario resta chiusa (esiste già).
- **Widget**: `ScreenHud.FastDecisionLookFollowsTheBoard` si estende ai testi.
- **PIE**: la cattura sul banco, anche in scala di grigi. Il layout dell'opzione regge già i testi lunghi (`U74`).
- ⚠️ **Dipendenza**: [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661) (D-497, in arrivo con la PR [#3663](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3663) e non ancora su `main`)
  ricentra `WBP_RT_FastDecision` sul rettangolo della scena. Il sottotitolo aggiunge una riga alla finestra. Conviene
  che S2 arrivi dopo, o che la cattura si rifaccia dopo.

### 4.3 La specifica

- **R2.1** `FRTReactionWindowView` porta `ReactionName`, `ReactorName`, `TriggerNames` e, per ogni opzione,
  `TargetName`. Sono risolti in `MakeReactionWindowView` dallo snapshot del momento, mai dal widget.
- **R2.2** Un nome di un'unità avversaria entra nel DTO solo se la squadra del proprietario la ha `Detected` in quel
  momento. Per l'Overwatch è vero per costruzione; per il `Brace` si chiede a `TeamAwarenessOfCell` con gli
  osservatori della squadra del proprietario.
- **R2.3** I testi sono quelli di §4.2 (Wiegers). Le frasi di `FIRE` e `HOLD` escono dalla tabella statica e diventano
  per-opzione.
- **R2.4** Nessun campo nuovo nella vista dell'avversario: `FilterWindowForTeam` resta l'unico filtro.

**DoD**
- [ ] I tre esempi di §4.2 sono test headless, e quello dell'indice spostato muore se il nome si risolve dopo la
      costruzione della vista.
- [ ] `FastDecisionLookFollowsTheBoard` controlla i testi.
- [ ] La cattura in PIE sul banco, anche in scala di grigi, dopo [#3661].
- [ ] La casella di #166 «UI `FIRE`/`HOLD` con countdown e bersaglio» si spunta con il rimando a questa issue.

---

## 5. S3 — I tipi delle chiavi degli scenari

### 5.1 Il fatto

Il loader dichiara una disciplina: *«ogni chiave si legge in DUE tempi — c'è? è del tipo giusto? — e la seconda domanda
è quella che conta»* (`LeggiCampoTipizzato`). Ma la seconda domanda passa da `TryGet*Field`, e l'engine **converte**
(P13). Gli esempi che passano oggi:
- `"freeRun": "yes"` diventa `true`;
- `"maxTurns": "4O"`, con una lettera O al posto dello zero, diventa 4;
- `"version": "7"` diventa 7;
- `"team": "1"` diventa 1;
- `"bot": 1` diventa `true`.

Nessuno di questi produce un errore, e un decimale scritto dove serve un intero si arrotonda (2,5 → 3). Fra le chiavi
scalari, la nuova `playerReactionWindows` (#3660) è la sola letta col controllo del tipo sul valore. Il loader lo fa
già per alcuni elementi di array (`RTScenarioLoader.cpp:213`, `:281`, `:2356`), ed è la forma da generalizzare.

### 5.2 Il panel

**WIEGERS** — È il requisito di sempre, reso vero: *ogni chiave scalare accetta solo il proprio tipo JSON, e un tipo
diverso è un errore che nomina la chiave con il suo percorso* (`units[2].team`).
- I booleani accettano solo `true` e `false`.
- Gli interi accettano solo numeri JSON con valore intero: non `2.5`, non `"2"`, non `true`.
- Le stringhe accettano solo stringhe: non `42`.

**FOWLER** — Una famiglia di lettori, in un posto solo del loader: booleano, intero e stringa, ciascuno con oggetto,
chiave, uscita, percorso ed errore. Controllano `Value->Type` e sostituiscono ogni `TryGet*Field`. `LeggiCampoTipizzato`
diventa un loro uso, non una seconda strada. Gli altri lettori JSON del progetto (l'indice degli scenari, i manifest)
sono fuori da questa issue: si registrano come seguito.

**NYGARD** — La compatibilità decide la forma.
- Un loader più severo **rifiuta** file che oggi passano, quindi il rischio è il verso «build nuova, file vecchio».
- Misurato: nel corpus nessun valore ha il tipo sbagliato (P15), e il writer scrive sempre il tipo giusto. ∴ nessun
  file cambia, e nessun bump di versione: non c'è una semantica nuova da dichiarare.
- Un file fatto a mano fuori dal corpus che contava sulle conversioni riceve un errore che nomina la chiave. È il
  comportamento voluto.

**CRISPIN** — Il criterio di accettazione è uno zero misurato, più una tabella.
- Lo zero: `grep -cE 'TryGet(Bool|Number|String)Field' Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` →
  **0** dopo la correzione.
- La tabella, un test headless: per una chiave per tipo e per livello (radice, unità, cella, intent, `expect`) un
  valore del tipo sbagliato è rifiutato, e il messaggio nomina il percorso.
- Il controllo positivo: `ShippedScenariosAreValid` e `EveryShippedScenarioRuns` restano verdi.

**ADZIC** — Gli esempi del test sono quelli di §5.1, uno per riga, più i casi di confine:
- un intero scritto `2.0`: accettato, è un intero;
- un intero scritto `2.5`: rifiutato, mentre oggi diventa 3;
- il valore polimorfo di `expect.value`: intero per i conteggi, booleano per `UnitAlive`.

### 5.3 La misura di P15

```bash
PYTHONIOENCODING=utf-8 python - <<'EOF'
import json, glob
BOOL={'blocksMovement','blocksLineOfSight','objective','bot','freeRun','expectSameAcrossVariants','playerReactionWindows'}
NUM={'moveCost','occupancySurcharge','offset','alongStart','alongEnd','layer','team','health','shield','visionRange',
     'param','version','seed','mapRadius','maxTurns','repeatCount'}
bad=[]
def walk(o,f,p):
    if isinstance(o,dict):
        for k,v in o.items():
            if k.startswith('_'): continue
            if k in BOOL and not isinstance(v,bool): bad.append((f,p+'.'+k))
            if k in NUM and (isinstance(v,bool) or not isinstance(v,(int,float))): bad.append((f,p+'.'+k))
            walk(v,f,p+'.'+k)
    elif isinstance(o,list):
        for i,v in enumerate(o): walk(v,f,f'{p}[{i}]')
for f in glob.glob('Scenarios/**/*.json',recursive=True): walk(json.load(open(f,encoding='utf-8')),f,'')
print(len(bad), bad[:10])
EOF
```

In questo passaggio stampa `0 []`. ⚠️ Gli elenchi delle chiavi sono scritti a mano dal loader: la issue li sostituisce
con quelli dei lettori nuovi.

### 5.4 La specifica

- **R3.1** Ogni chiave scalare del formato scenario si legge con un lettore che controlla il tipo JSON del valore. Un
  tipo diverso è un errore che nomina il percorso della chiave.
- **R3.2** Nessun `TryGet*Field` resta in `RTScenarioLoader.cpp`.
- **R3.3** Nessun bump di versione e nessun file del corpus modificato (P15).

**DoD**
- [ ] `grep -cE 'TryGet(Bool|Number|String)Field' Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` → 0.
- [ ] Il test a tabella di §5.2, con almeno una chiave per tipo e per livello, muore se un lettore torna a convertire.
- [ ] `Scenario.*` verdi, compresi `ShippedScenariosAreValid` ed `EveryShippedScenarioRuns`.
- [ ] Il commento di `LeggiCampoTipizzato` dice ciò che il codice fa.

---

## 6. Le issue aperte

| | Issue | Titolo | Milestone | Collegamenti |
|---|---|---|---|---|
| S1 | [#3666](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3666) | La finestra di reazione si apre prima che il playback mostri l'ingresso: presentazione e countdown al trattenimento del playback | v0.1 | parte di [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166), epic [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152); vincolo M10 in [#759](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759) |
| S2 | [#3667](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3667) | Titolo, sottotitolo e bersaglio nella finestra di reazione: i nomi risolti nel DTO, con la regola di rilevazione per il `Brace` | v0.1 | la casella «e bersaglio» di [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166); dopo [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661) |
| S3 | [#3668](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3668) | Il loader degli scenari controlla il tipo JSON di ogni chiave scalare: `TryGet*Field` converte | v0.1 | harness degli scenari; seguito di [#3660](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3660) |

Aperte il 2026-10-11, con le DoD di §3.4, §4.3 e §5.4.

---

## 7. Fuori dalle tre, trovato durante la ricognizione

- **[#3047](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3047) ha la premessa invecchiata.** Diceva che
  `WBP_RT_TacticalHUD` non referenzia la finestra (`names.py … --filtro FastDecision` → 0 nomi). In questo passaggio la
  stessa misura trova `WBP_RT_FastDecisionCenter`, e la cattura di `U74` mostra la finestra a schermo quando la
  reazione si apre. La guida (`guida-screen-hud-umg.md` §3) cita ancora #3047 come scoperta. Va chiusa con questa
  evidenza, e va riallineata la riga della guida: è un atto su una issue che non è di questo referto, quindi resta
  raccomandato.
- **Gli altri lettori JSON del progetto** hanno lo stesso rischio di S3. Non sono misurati qui.

---

## 8. Dalla verifica avversaria

Un verificatore indipendente ha riletto le premesse sul codice, con un tetto di chiamate. Ha confermato P3-P7, P9-P11
e P14. Ha trovato cinque difetti, tutti corretti qui:
- **S1, alta**: la correzione proposta non reggeva la seconda sospensione (P16) né il playback assente (P17), e
  dimenticava i test che si aspettano l'apertura immediata (P19). §3.2 e §3.4 sono riscritti.
- **S1, media**: «nessuna decisione d'autore» era falso. Separare sospensione e presentazione cambia il significato di
  *«durante la finestra»* in [D-350] e in #166. §3.3 è ora una decisione minima richiesta.
- **S3, media**: P13 diceva «stringa→booleano sempre `true`»; è la *risposta* a essere sempre `true`, il valore è
  quello di `ToBool()`. Ed era sfuggito l'arrotondamento dei decimali.
- **S2, bassa**: la regola del `Brace` non nominava il predicato, e la firma di `MakeReactionWindowView` deve cambiare.
- **S3, bassa**: «la sola chiave letta col controllo del tipo» non diceva che il loader lo fa già per alcuni elementi
  di array.

Una seconda passata del verificatore ha confermato chiusi i cinque punti, e ne ha trovati altri due in S1, corretti:
il terzo stato del playback («già in corso», che `BeginPartialPlayback` tratta come «non parte»), e la pausa del
giocatore, ora R1.7.

Ciò che il verificatore ha lasciato non verificato l'ho misurato io: P2 è la cattura di `U74`, già riletta dalla
review di #3664; #3047 e `names.py` sono misurati in §7; P8, P12 e i link alle issue sono letti in questo passaggio.

## 9. Gate di questo passaggio

Solo documentazione: `doc-links`, `doc-tables`, `doc-coherence`, `decision-ids`, `issue-refs` e `docs-corpus` di
`tools/radar/`. Gli esiti sono nella PR.

[D-155]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-350]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-355]: ../../decisions/RT_PDR_00_Decision_Log.md
[#3661]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661
