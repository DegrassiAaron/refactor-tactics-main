# Player Event Log — work order «create/update GitHub issues + docs» — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma
> [`../../archive/src/handoff/2026-08-31-player-event-log-issue-epic-docs.md`](../../archive/src/handoff/2026-08-31-player-event-log-issue-epic-docs.md)
> (arrivato come `Claude_RefactorTactics_PlayerEventLog_Issue_Epic_Docs_2026-08-31.md`, **untracked**,
> 642 righe, 20 227 byte, marker `RT-PLAYER-EVENT-LOG-2026-08-31`).
>
> **Data**: 2026-08-31 · **Base**: `origin/main` @ `188183b9` dopo `git fetch --prune` ·
> **Modo**: critique · **Focus**: requirements + architecture + testing
>
> **Cosa è**: il verdetto su un work order che ordina di aprire una issue v0.1, un epic cross-release e
> una spec tecnica per un *Player Event Log* — il feed sintetico rivolto al giocatore, distinto dal log
> diagnostico di [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79).
> Il referto giudica il mandato e **non esegue il lavoro che ordina**: nessuna issue creata o modificata,
> nessun commento su GitHub, nessun documento owner riscritto, nessuna riga di gameplay toccata.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da
> [`../../technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md),
> dal [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) o dal corpo di
> [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613), **ha ragione l'owner**.
>
> ⚠️ **Il kit non è atterrato in questo checkout.** È arrivato in `D:\Repositories\refactor-tactict-dev`,
> un terzo clone dello stesso remote, allineato a `origin/main`; il checkout di sessione
> (`refactor-tactics-technical-designer/refactor-tactics-main`) era **41 commit indietro**. Tutte le misure
> qui sotto sono state prese sul clone allineato, e il referto è scritto in un **worktree** creato da
> `origin/main` per non toccare i due working tree in uso da altre sessioni.

---

## 1. Il verdetto in una riga

> **È il kit meglio misurato di questa famiglia — nove owner citati su nove aperti, dodici file citati su
> dodici esistenti — e sbaglia esattamente dove smette di proporre e crede di descrivere: la «vista
> autorizzata» che mette a monte del proiettore, nel repository, emette `TArray<FString>`; la sua regola di
> privacy vieta ciò che una decisione accettata rende obbligatorio; e il titolo che impone come vincolante
> non compare in nessuna delle 771 issue del repository.**

Le tre cose non si somigliano, ma hanno la stessa forma: il kit prende per **esistente** un contratto che
è **da costruire**, e prende per **da costruire** una convenzione che **esiste già**.

✅ **Ciò che vale, e va detto per primo.** Questo work order non ha il difetto ricorrente della famiglia —
non cita nemmeno una issue chiusa. `#25 #79 #613 #1466 #1496 #265 #268 #1881 #472`: nove su nove **aperte**.
E la pipeline che disegna (`Resolver → TurnLog canonico → vista autorizzata → proiettore → UI`) non è una
proposta: è **la stessa forma** che [#295](https://github.com/DegrassiAaron/refactor-tactics-main/issues/295)
ha già deciso chiudendo il difetto opposto («un TurnLog sanitizzato per squadra toglie il determinismo al
replay»). Il kit ha ragione sull'architettura. Ha torto su cosa di quell'architettura sia già in casa.

---

## 2. La verità del repository, misurata

Il kit dichiara la propria gerarchia di fonti (`ADR/decisioni → codice → issue → questo handoff`) e chiede
di preservare la verità del repository dove diverge. Ecco la divergenza, misurata su `188183b9`.

### 2.1 Gli owner citati

| Issue | Stato | Titolo | Il kit dice |
|---|---|---|---|
| #25 | **OPEN** | `[EPIC v0.1] E11 — HUD, log e debug` | parent della nuova issue ✅ |
| #79 | **OPEN** | `CP 11.3 — Combat log con reason code completi` | resta diagnostico, non si semplifica ✅ |
| #613 | **OPEN** | `CP 11.7 — Screen HUD in UMG (layer §4.1)` | possiede lo Screen HUD ✅ |
| #1466 | **OPEN** | `E13.6 · La conoscenza parziale diventa visibile…` | possiede conoscenza/privacy ✅ |
| #1496 | **OPEN** | `Il filtro di conoscenza chiede «lo conosco adesso?»…` | idem ✅ |
| #265 / #268 | **OPEN** | Icon Language / integrazione HUD | possiedono la grammatica icone ✅ |
| #1881 | **OPEN** | `[EPIC] Resolution Playback & Inspection — … fino alla v1.0` | precedente di epic cross-release ✅ |
| #472 | **OPEN** | `Replay R6: l'interfaccia…` | consumatore replay ✅ |

**Nove su nove aperte.** Nessuna issue nel repository porta il marker `RT-PLAYER-EVENT-LOG-2026-08-31`,
nessuna ha «Player Event Log» nel titolo, e la ricerca su `legacy` non restituisce nessun owner della
dismissione dei pannelli Canvas. Il `CREATE` che il kit ordina è, per la sua stessa regola *search first*,
**autorizzato**: le due issue non esistono.

### 2.2 I file citati

Dodici dei quattordici percorsi citati esistono; i due assenti sono esattamente quelli che il kit dichiara
**da creare**.

| Percorso | Esito |
|---|---|
| `docs/technical/systems/progettazione-hud.md` | ✅ 1 906 righe |
| `docs/technical/architecture/spec-turnlog.md` | ✅ 727 righe |
| `docs/roadmap/roadmap-v0.1.md` · `roadmap-post-v0.1.md` | ✅ 2 069 · 1 375 righe |
| `Source/…/UI/RTHUD.{h,cpp}` | ✅ 398 · 1 098 righe |
| `Source/…/UI/RTScreenHudWidgets.{h,cpp}` | ✅ 346 · 281 righe |
| `Source/…/Turn/RTTurnLog.h` · `RTTurnManager.h` | ✅ 1 051 · 1 803 righe |
| `Source/…/Tests/RTCombatLogTests.cpp` | ✅ 1 169 righe |
| `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` | ✅ esiste |
| `docs/technical/systems/spec-player-event-log.md` | ⛔ **assente** — da creare, come dichiarato |
| `Content/RT/UI/Match/WBP_RT_EventLog.uasset` | ⛔ **assente** — da creare, come dichiarato |

E le tre cose che il kit dà per vive, lo sono: `rt.HUD.CanvasPanels` è dichiarata a `RTHUD.cpp:42-43`;
`RefactorTactics.UI.LogContainsReasonAndCoords` e `RefactorTactics.UI.LogMatchesTurnLogOrder` sono a
`RTCombatLogTests.cpp:136` e `:174`. Il token `PlayerEvent` ha **zero** occorrenze in `Source/` e in
`docs/`: il contratto non esiste, e il kit non sta duplicando nulla.

---

## 3. Rilievi

### 🔴 C1 — La «vista autorizzata» che il kit mette a monte del proiettore emette **stringhe**

*(Fowler, Hohpe — architecture)*

Il kit costruisce due divieti che, presi insieme sulla produzione di oggi, non lasciano nessuna porta:

> Never: parse debug strings to infer player events · project private enemy planning and filter it only
> after text generation
>
> The projector **consumes the existing knowledge/privacy contract; it does not redefine it.**

La porta autorizzata esistente è una sola, ed è misurata — `RTTurnManager.h:471`:

```cpp
static TArray<FString> ComposeVisibleLogLines(const TArray<FRTCombatLogLine>& Lines, int32 ObserverTeamId);
```

Restituisce **righe di testo già composte**. Il filtro di conoscenza e la composizione della frase avvengono
insieme, dentro la stessa funzione. Quindi il proiettore proposto dal kit ha tre strade, e il kit ne vieta
due:

1. consumare l'output di `ComposeVisibleLogLines` → è **parsing di stringhe**, vietato dal kit;
2. consumare `FRTCombatLogLine` grezze e riapplicare il verdetto per conto proprio → **ridefinisce** il
   contratto di conoscenza, vietato dal kit e dal confine di #1466/#1496;
3. estrarre da `ComposeVisibleLogLines` un produttore **strutturato** che condivida il predicato e
   restituisca fatti invece di righe.

La terza è l'unica praticabile, ed è **l'unica che il kit non nomina**: non è nella `Scope v0.1`, non è
nella `Definition of Done`, non è nei `Suggested files`. Un work order che vieta le due strade facili senza
finanziare la terza produce una issue che si può chiudere solo violandola.

📝 **Raccomandazione.** Aggiungere alla `Scope v0.1` una voce `C-bis` e alla DoD la riga corrispondente:

> **C-bis — Estrarre la porta autorizzata strutturata.** `ComposeVisibleLogLines` si separa in
> *(a)* un predicato di autorizzazione riusabile e *(b)* una composizione testuale che lo chiama. Il
> proiettore chiama **(a)**, non **(b)**, e nessuno dei due riderivà il verdetto per conto proprio.
> Il predicato resta **uno**: è la condizione che D-223 pone («la regola atterra come UN predicato, non
> come prosa»), ed è il motivo per cui questa voce esiste.

🎯 **Priorità**: alta — senza questa voce le altre undici della DoD non sono raggiungibili in modo lecito.

---

### 🔴 C2 — La DoD di privacy chiede di rompere un test verde: **la morte è pubblica per decisione**

*(Wiegers, Nygard — requirements / failure modes)*

Il kit scrive:

> A private or unknown enemy fact must not leak through: text; icon; count; event type; hidden actor
> identity; timing metadata.
>
> - [ ] Privacy test proves **unauthorized enemy facts produce no player event**.

E classifica `KO` fra i `Critical`, cioè fra gli eventi che **devono** raggiungere il giocatore.

La produzione dice il contrario, e non per difetto: **D-223**, decisione d'autore accettata il 2026-08-27 e
emendata il 2026-08-28, stabilisce che *le righe di morte sono pubbliche*:

> **Emendamento del 2026-08-28 — le righe di morte sono PUBBLICHE.** Le cinque righe che annunciano
> un'eliminazione portano `World()`: la morte la leggono tutte le squadre, anche chi non vedeva la vittima.
> […] ⚠️ **Cosa si rivela**: nome e squadra, **mai una cella**.

Ed è protetta da un test **già verde** in produzione — `RTCombatLogTests.cpp:943`:

```
RefactorTactics.UI.DeathIsPublicEvenToWhoNeverSawIt
```

che verifica l'**asimmetria**: l'annuncio di morte passa il filtro per un osservatore che non ha mai visto
la vittima, mentre una riga ordinaria sulla stessa unità no.

Eseguito alla lettera, il work order apre una issue il cui criterio di chiusura è la rottura di quel test.

📝 **Raccomandazione.** Riscrivere la voce e la sezione `Privacy` così:

> Nessun fatto **non autorizzato** produce un evento giocatore, con l'unica eccezione già decisa da
> **D-223**: l'annuncio di eliminazione è pubblico e rivela **nome e squadra, mai una cella**. Il criterio
> non è *di cosa parla* la riga, è *cosa rivela*: la riga letale del canale derivato porta due celle e
> resta filtrata.
>
> - [ ] Test di privacy **asimmetrico**: un fatto ordinario su un nemico non autorizzato non produce
>       evento; la sua eliminazione sì, e senza cella.

🎯 **Priorità**: alta — è l'unico punto in cui il kit, applicato, produce una regressione misurabile.

⚠️ **Corollario che il kit non vede.** Se KO è `Critical` e l'annuncio è pubblico, il Player Event Log
eredita la **condizione di riapertura** che D-223 dichiara: ripetuta lungo la partita, la morte pubblica
enumera il roster avversario. Oggi è inerte (2v2 a roster noto, D-120); il giorno del draft non lo è più.
Va citata nell'epic, sotto il gate v1.0, non nella issue v0.1.

---

### 🟡 C3 — Il titolo «obbligatorio» non esiste in nessuna delle 771 issue

*(Cockburn, Wiegers — requirements)*

Il kit impone:

> ## Required title
> `[v0.1][HUD] Dismettere il Canvas Screen HUD legacy e introdurre il Player Event Log sintetico`

Misura su tutte le issue del repository (771, `--state all`):

| Pattern | Occorrenze |
|---|---:|
| titolo che inizia con `[v0.1]` | **0** |
| titolo che inizia con `[EPIC]` | 13 |
| titolo che inizia con `v0.1 ·` | 3 |
| frase italiana descrittiva senza prefisso | la larga maggioranza — le ultime 12 aperte sono tutte così |

Il prefisso `[v0.1][HUD]` è un'invenzione del kit. La release sta nella **label** (`v0.1`) e nel
**milestone** (`v0.1 · Leggibilità`), che #25, #79 e #613 portano tutte e tre.

📝 **Raccomandazione.** Titolo in convenzione, che dice il difetto invece della categoria:

> `Il log a schermo racconta le celle, non la partita: i pannelli Canvas legacy escono e il feed del giocatore entra`

E metadati: label `v0.1`, milestone `v0.1 · Leggibilità`, priorità `P1` — le stesse di #613, che è il
checkpoint dentro cui questo lavoro cade.

---

### 🟡 C4 — L'epic copia il **titolo** di #1881 e ignora la sua **struttura**

*(Newman — service boundaries)*

Il kit motiva la forma dell'epic citando un precedente, e la citazione è corretta:

> The cross-release epic must follow the repository precedent used by cross-release epics such as `#1881`:
> **`[EPIC]` without an `E<n>` number**.

Misurato: 13 titoli `[EPIC]`, di cui diversi senza `E<n>` (`Resolution Playback & Inspection`,
`Map Editor 0.1`, `Tactical Designer`, `Lavoro parallelo`). ✅ Il kit ha ragione.

Ma il precedente che cita ha anche una struttura, e lì il kit sbaglia:

| Epic | Sub-issue **native** |
|---|---:|
| **#1881** — il modello dichiarato dal kit | **3** |
| #25 — E11, l'epic vecchio | **0** |

Il kit chiede invece solo backlink testuali (`> **Parent**: #25`, `epic body links v0.1 issue`). Una riga
`Parent:` nel corpo **non collega niente**: non popola l'elenco dei figli, non entra nel progress dell'epic,
e non si vede da GitHub se non leggendo il testo. Se l'epic nuovo è modellato su #1881, va collegato come
#1881 — con l'API delle sub-issue, che vuole l'**id** del nodo, non il numero.

📝 **Raccomandazione.** Alla sezione `6. Backlinks / reconciliation` si aggiunge:

> Il legame epic → issue v0.1 si crea come **sub-issue nativa** (`addSubIssue`, GraphQL, che richiede
> l'`id` del nodo e non il numero), come già fa #1881 con i suoi tre figli. La riga `Parent:` nel corpo
> resta, ma come **prosa leggibile**, non come meccanismo.

---

### 🟡 C5 — Il kit dichiara che il layout non è suo, e poi lo decide

*(Fowler — interface segregation)*

Nello stesso documento:

> `docs/technical/systems/progettazione-hud.md` continues to own **layout and HUD composition**.

e, tre sezioni prima:

> - compact; bottom-right; **above** Undo/Confirm/plan controls; initial budget: max ~4 recent events;
>   no permanent large scroll list in normal match HUD.

Sono cinque decisioni di layout prese fuori dal documento che il kit stesso dichiara owner. E l'owner ha
già scritto qualcosa su questo oggetto — `progettazione-hud.md` **§14**:

> Il combat log deve essere: piccolo/collassato durante Planning; **più visibile durante Resolution**;
> guidato dal TurnLog autorevole; **espandibile**.

più la §14.1 `WHY?`, che apre la riga `26 DAMAGE [WHY?]` sulla scomposizione della formula.

Le due descrizioni non si contraddicono frontalmente — un feed compatto **è** uno stato collassato — ma il
kit non nomina §14, non nomina la variazione per **fase** (Planning vs Resolution) e con «no permanent
large scroll list» tocca il perimetro dell'`espandibile` senza dirlo. Il combat log, inoltre, è elencato
in §4.1 fra i contenuti dello Screen HUD: è **già** nel perimetro di #613.

📝 **Raccomandazione.** La issue non fissa il layout: lo **cita**. Una riga sola:

> Il feed è lo stato *collassato* del combat log di `progettazione-hud.md` §14 — compatto durante il
> Planning, più visibile in Resolution. L'espansione `[WHY?]` (§14.1) resta a #79/#613: questa issue non la
> costruisce e non la contraddice. Posizione e budget di righe si emendano in §14, non qui.

---

### 🟡 C6 — Il perimetro «legacy screen-space Canvas» cattura un elemento che nessuna delle due regole protegge

*(Nygard — failure modes)*

Il kit ordina di cancellare i pannelli Canvas screen-space e ne elenca quattro (turn/header, testo di
combattimento in basso a sinistra, barra abilità, terna di slot), preservando `ARTHUD::DrawHUD` **per il
Tactical World Overlay §4.2**.

Misurato in `RTHUD.cpp`, la guardia `bCanvasPanels` (letta una volta per frame a `:899`) copre esattamente
tre blocchi — `:902` barra di stato in alto, `:981` combat log in basso a sinistra, `:1000` barra abilità e
terna slot. L'elenco del kit combacia. ✅

Ma fra il primo e il secondo blocco, **fuori dalla guardia**, c'è un quarto disegno screen-space:

```cpp
// Banda «questa non e' una partita»: quando il GameMode sta eseguendo uno scenario, la partita normale non
// viene allestita e mancano unita' proprie, selezione e barra abilita'. Senza questa riga il sintomo non
// punta alla causa — la spiegazione esiste, ma solo nell'Output Log.
```

Non è un pannello di partita e non è §4.2: è una banda diagnostica. Le due regole del kit non lo coprono —
«preserve DrawHUD **for Tactical World Overlay**» non lo include, «delete legacy **screen-space** Canvas
panels» lo cattura. Chi esegue alla lettera lo cancella, e il prossimo che avvia uno scenario senza unità
non vede più *perché* lo schermo è vuoto.

📝 **Raccomandazione.** Nella `Scope v0.1 / A`, riga esplicita:

> **Preservato**: la banda «questa non è una partita» (`RTHUD.cpp`, fuori da `bCanvasPanels`). È
> diagnostica di allestimento, non un pannello di partita, e non è coperta né dal §4.1 né dal §4.2.

➕ E una nota di provenienza: la procedura PIE del kit già archiviato
[`2026-08-30-claudecloud-debughud-graybox.md`](../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md)
usa `rt.HUD.CanvasPanels 0` / `1` ai suoi §429, §440, §531. Rimossa la CVar, quella procedura archiviata
smette di essere eseguibile: non è un problema (l'archivio non è normativo), ma il commento che documenta
la CVar a `RTHUD.cpp:34` va rimosso **insieme** alla CVar, non lasciato a puntare a nulla.

---

### 🟢 C7 — I sei test proposti non provano la regola che il kit dichiara più importante

*(Crispin, Gregory — testing)*

I sei test coprono l'aggregazione (`CollapsesMoveCells`, `OmitsMinorMovement`, `KODominatesDamage`,
`GroupsEnvironment`, `PreservesSemanticOrder`) e l'omissione (`OmitsUnauthorizedFacts`). Sono buoni test di
**esito**. Nessuno prova l'**ordine** — che l'autorizzazione preceda la proiezione — e l'ordine è
precisamente l'invariante che il kit mette in cima («project first and sanitize later» fra i *Never*).

Un test che verifica «il fatto non autorizzato non compare» passa **anche** in un'implementazione che
proietta e poi cancella: è lo scenario che il kit vuole vietare, e i suoi test non lo distinguono.

📝 **Tre test da aggiungere**, tutti con un precedente vivo nel repository:

| Test | Prova | Precedente |
|---|---|---|
| `PlayerEventLog.AuthorizationMatchesLogLines` | il proiettore e `ComposeVisibleLogLines` accettano/negano **lo stesso insieme** di fatti per lo stesso osservatore | il predicato unico di D-223 |
| `PlayerEventLog.DeathIsPublicWithoutCell` | asimmetria: l'eliminazione passa, la riga ordinaria no, e l'evento **non porta una cella** | `DeathIsPublicEvenToWhoNeverSawIt` |
| `PlayerEventLog.HashIsIndependentOfPlayerEvents` | la derivazione degli eventi giocatore **non** entra nell'hash del TurnLog | `Noise.HashIsIndependentOfObserver`, suggerito in #295 |

Il terzo chiude in test ciò che il kit oggi affida alla prosa («It does not enter simulation hash/state
unless a future ADR explicitly changes that»): finché è prosa, la prima implementazione distratta la viola
senza che nessuno se ne accorga, ed è esattamente il modo in cui #295 descrive il difetto che ha chiuso.

---

### 🟢 C8 — Due criteri della DoD sono già criteri di #613

*(Wiegers — traceability)*

Il kit dichiara «This is **not** a new CP 11.7», e ha ragione a non mintare un checkpoint. Ma poi mette
nella DoD due voci che sono **testualmente** già nella DoD di #613, che è aperta:

| DoD del kit | DoD di #613 |
|---|---|
| «Player projector consumes structured authorized data, not formatted debug strings» | «I widget **non ricalcolano** formula, visibilità o reason code: leggono un view model sanitizzato» |
| «Debug/player views are distinct and debug is off by default» | «Debug spento di default nella vista giocatore» |

Due issue aperte con lo stesso criterio di chiusura significano che chiuderne una non dice niente
sull'altra: è la stessa forma di «due source of truth» che il kit denuncia al §1.

📝 **Raccomandazione.** Quei due criteri **restano a #613**. La issue nuova li cita come vincolo ereditato
(«vale il criterio di #613, non ripetuto qui») e tiene solo ciò che è suo: il contratto tipizzato, il
proiettore, l'aggregazione, la rimozione dei pannelli, il test di privacy asimmetrico.

---

### 🟢 C9 — Il sequenziamento con #1499 non è nominato, e lo è nella decisione che il kit consuma

*(Newman — evolution)*

D-223 chiude con una riga di sequenziamento:

> ➕ **Sequenziamento**: conviene dopo `#1499`, che tocca comunque tutti gli **80** call site di
> `AddLogEvent` per togliere il default fail-open; farli insieme evita di attraversarli due volte.

[#1499](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1499) è **aperta**. Il lavoro che il
kit ordina attraversa gli stessi call site — è lì che nascono i fatti da cui il proiettore deriverà gli
eventi. Il kit non nomina né #1499 né D-223.

📝 **Raccomandazione.** Nel corpo della issue v0.1, sotto i `Related`: `#1499` e la riga «il proiettore
nasce **dopo** o **insieme** al passaggio sui call site di `AddLogEvent`: attraversarli due volte costa il
doppio e li lascia divergere in mezzo».

---

## 4. Cosa il kit dice di sé e non è vero

Una riga sola, ma è quella che ne fissa lo statuto:

> If the repository disagrees with this file, **measure and preserve repository truth**, then document the
> delta.

È la regola giusta, ed è la ragione per cui questo referto esiste invece di un'esecuzione. Applicandola, il
delta è: **due voci della DoD vanno riscritte prima di aprire la issue** (C1, C2), **tre di forma** (C3, C4,
C5), **una di perimetro** (C6). Il resto del work order — la pipeline, i divieti, la tassonomia
`Minor/Important/Critical`, la dominanza `KO > Damage > Hit`, il rifiuto di semplificare #79, l'assenza di
`E<n>` sull'epic — regge alla misura.

---

## 5. Cosa fare adesso

> ✅ **Eseguito il 2026-08-31 su richiesta esplicita**, dopo la consegna del referto. Le due issue sono
> aperte — **#1936** (v0.1) e **#1937** (epic) — e il §7 registra cosa è stato scritto davvero, incluse le
> due voci di Decision Log. Ciò che segue è la specifica con cui sono state aperte.

### 5.1 Issue v0.1 — aperta come #1936

- **Titolo**: `Il log a schermo racconta le celle, non la partita: i pannelli Canvas legacy escono e il feed del giocatore entra`
- **Label**: `v0.1`, `P1` · **Milestone**: `v0.1 · Leggibilità`
- **Corpo**: quello del kit §3, con queste sei modifiche:
  1. `Scope v0.1` acquista **C-bis** (estrazione della porta autorizzata strutturata) — da C1;
  2. la sezione `Privacy` e la voce DoD corrispondente si riscrivono sull'eccezione di **D-223** — da C2;
  3. il blocco layout diventa un rimando a `progettazione-hud.md` §14 — da C5;
  4. `Scope A` dichiara **preservata** la banda «questa non è una partita» — da C6;
  5. i tre test di C7 si aggiungono ai sei;
  6. le due voci DoD duplicate da #613 escono, sostituite dal rimando — da C8.
- **Related**: `#613 #79 #1466 #1496 #1499`, e `#295` come precedente che decide la forma della pipeline.

### 5.2 Epic cross-release — aperto come #1937

- **Titolo**: `[EPIC] Player Event Log & Explainability — dal TurnLog alla UI fino alla v1.0` ✅ (in convenzione)
- **Corpo**: quello del kit §4, più:
  - sotto `v1.0 close gate`, la **condizione di riapertura di D-223** (la morte pubblica enumera il roster
    avversario il giorno del draft) — da C2;
  - sotto la tabella degli owner, `#295` e `D-276` (il replay è **due** prodotti: pubblico sanitizzato e
    audit privato) — che è il vincolo vero dietro il gate «history e replay consumano lo stesso contratto».
- **Legame**: sub-issue **nativa** verso la issue v0.1, come #1881 con i suoi tre figli — da C4.

### 5.3 Documentazione

`docs/technical/systems/spec-player-event-log.md` — la struttura in 12 punti proposta dal kit è buona e va
usata, con due correzioni: il §4 `Privacy` porta l'eccezione D-223, e il §3 `Pipeline` nomina il predicato
condiviso invece di una «vista autorizzata» generica che oggi non esiste come struttura.

---

## 6. Provenienza

| Cosa | Dove |
|---|---|
| Sorgente consumato | [`../../archive/src/handoff/2026-08-31-player-event-log-issue-epic-docs.md`](../../archive/src/handoff/2026-08-31-player-event-log-issue-epic-docs.md) |
| Base delle misure | `origin/main` @ `188183b9` |
| Kit affine, già consumato | [`../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md`](../../archive/src/handoff/2026-08-30-claudecloud-debughud-graybox.md) — sovrapposizione misurata: **16 righe condivise su 413** uniche del nuovo. Materiale nuovo, non un re-drop |
| Decisioni citate | D-223 (verdetto congelato, morti pubbliche), D-276 (replay in due prodotti), D-120 (roster noto) |
| Issue citate | #25 #79 #265 #268 #295 #472 #613 #1466 #1496 #1499 #1881 |

Le scritture su GitHub sono elencate al §7.

---

## 7. Esecuzione — cosa è stato scritto davvero

Il referto è stato consegnato senza scrivere su GitHub; l'esecuzione è arrivata subito dopo, su richiesta
esplicita, con i sei rilievi già applicati.

### 7.1 Issue

| | Titolo | Esito |
|---|---|---|
| **#1936** | *Il log a schermo racconta le celle, non la partita: i pannelli Canvas legacy escono e il feed del giocatore entra* | **creata** — label `v0.1`, `P1`; milestone `v0.1 · Leggibilità` |
| **#1937** | `[EPIC] Player Event Log & Explainability — dal TurnLog alla UI fino alla v1.0` | **creato** — label `v0.1`, `epic`, `P1`; nessuna milestone, come #1881 |

Entrambe scritte **in italiano**, come i corpi di tutte le issue del repository: il work order li dettava in
inglese, ed è un settimo scostamento dalla convenzione che il §3 non aveva censito perché riguarda la lingua,
non la struttura.

### 7.2 Legame e backlink

- **Sub-issue nativa** #1937 → #1936 (`addSubIssue` via GraphQL, che vuole l'`id` del nodo e non il numero):
  è il rilievo **C4** applicato — l'epic ha ora 1 figlio nativo, come #1881 ne ha 3.
- Backlink reciproci nelle due testate.
- Commenti **additivi** su #25, #613 e #79 — nessun corpo storico riscritto. Quello su #79 dichiara ciò che
  conta per lui: il contratto resta intero, e la separazione del predicato non cambia le righe che produce.

### 7.3 Decision Log

| Voce | Cosa registra |
|---|---|
| **D-299** *(nuova)* | Il log rivolto al giocatore è un canale **derivato** con vocabolario proprio: `Minor · Important · Critical` è tassonomia di **presentazione**, non di gameplay; il log dettagliato non si semplifica; la **dominanza** non si ottiene abbassando la verbosità; il canale non entra nell'hash, e il layout resta di §14 |
| **D-223** *(emendata)* | Il **quarto consumatore** del verdetto congelato, e la condizione di riapertura che **non** scatta. Con il reperto **C1**: la regola non è chiamabile come sta scritta, perché la porta autorizzata compone il testo dentro la stessa funzione che filtra |

⚠️ **Il numero è stato rimisurato due volte**: all'apertura della sessione il massimo era `D-296`, alla
scrittura `D-298`. Due voci sono entrate da altre sessioni nel frattempo.

**Non registrata**, e la ragione va detta: la separazione di `ComposeVisibleLogLines` non è una decisione ma
una **conseguenza forzata** — le altre due strade sono già vietate da decisioni esistenti, e dove non c'è
scelta non c'è nulla da decidere. Vive come lavoro nella voce `C-bis` di #1936.

### 7.4 Cosa resta al lavoro, non al tracking

`docs/technical/systems/spec-player-event-log.md` non è stato creato: è l'owner della semantica, e nasce
**con** l'implementazione in #1936, non prima. La struttura in 12 punti proposta dal kit resta valida con le
due correzioni del §5.3.
