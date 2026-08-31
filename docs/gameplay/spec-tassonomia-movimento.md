# Spec — Tassonomia del movimento

> `CURRENT` · **Owner** della domanda «di che tipo di movimento stiamo parlando, e cosa comporta».
> Creato il **2026-08-09** consolidando `RefactorTactics_Move_Consolidation_Claude.md` (kit d'autore).
> Non sostituisce nessun documento: [`spec-dash.md`](spec-dash.md) resta owner del **Dash**,
> [`spec-sequenza-turno.md`](spec-sequenza-turno.md) dell'**ordine delle fasi**,
> [`../technical/architecture/spec-pathfinding-pf3-pf4.md`](../technical/architecture/spec-pathfinding-pf3-pf4.md) del **grafo e dell'A\***.
> Qui vive ciò che nessuno di loro possiede: il confronto **fra le famiglie**.

## Perché esiste

Il progetto ha quattro modi diversi di spostare un'unità e li ha costruiti in momenti diversi — il Move a M6.2,
il Dash a M6.5, la spinta con il knockback esagonale, `Action.Reposition` col motore azioni. Ognuno ha il suo
documento; **nessuno risponde alla domanda che si fa chi progetta un'abilità nuova**: se sposto qualcuno, cosa
scatta? paga il movimento? attraversa il fuoco?

Rispondere caso per caso è il modo in cui nascono le regole per eroe. Questa pagina è la risposta unica.

## 1. Le famiglie

Le famiglie si dividono in **due**, e la linea che le separa non è la velocità:

> **Traversal percorre lo spazio. Transfer cambia posizione senza percorrerlo.**

```text
TRAVERSAL — produce una sequenza di celle percorse, e ogni cella è un fatto
  Move       movimento volontario standard, fase Move, a budget
  Dash       movimento volontario speciale, fase Dash, senza budget
  Forced     spostamento subito: push, pull, knockback, correnti

TRANSFER — esistono un'origine e una destinazione, e nient'altro
  Leap       `ERTMovementStyle::LinearLeap`   (esiste, ma nessun eroe ce l'ha — #645)
  Blink      destinazione libera entro range   (v0.2, E39)
  Swap       scambio atomico di due posizioni  (v0.2, E39)
  Recall     ritorno a un punto dichiarato     (v0.2, E39)

Reaction     NON è una famiglia: è una causa, e usa la policy di una delle sopra
```

> 🔴 **La partizione è una decisione, presa il 2026-08-12**:
> [D-118](../decisions/RT_PDR_00_Decision_Log.md) chiude `MOV-1`. Non introduce un tipo nuovo — la riga che
> separa le due famiglie **si verifica già in codice**, ed è cosa contiene `Result.Entered`: per un
> `Traversal` è la sequenza percorsa, per un `Transfer` è la sola destinazione. La colonna «Teleport» di §2
> era una regola preventiva per una famiglia che si credeva assente; era invece la descrizione di
> `LinearLeap`, che il repository possiede da sempre.

✅ **`Swap` è qui, e questo risponde a una domanda che il resolver del Move non deve porsi.**
`AUTHOR-MOVE-001` ([D-295](../decisions/RT_PDR_00_Decision_Log.md)) decide che lo **scambio diretto e i cicli
chiusi bloccano** nel Move *«salvo permesso esplicito»*. Quel permesso è **questa riga**: lo scambio lecito è
un `Transfer`, `v0.2`/`E39`, e un Transfer non percorre celle intermedie — non passa dalle regole di
traversal. ⛔ Ne segue che in **v0.1 non esiste alcuno scambio lecito** e la regola del Move è
**incondizionata**: un flag di permesso dentro `StepHexMovement` sarebbe un secondo owner per una famiglia che
ne ha già uno. L'implementazione della regola è
[#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922).

`Reaction Movement` **non è una famiglia**: è una delle altre con una causa diversa. Ciò che deve restare
distinguibile è la *causa*, non il modo di muoversi — chi legge il TurnLog deve poter dire se quell'unità si è
mossa perché l'ha deciso, perché è stata spinta o perché una reazione l'ha fatta scattare.

⚠️ **`Portal` non è un `Transfer`**, e va detto qui perché è l'errore più naturale: un portale è
**topologia del grafo** — `URTHexMapAsset::Transitions`, `ERTHexArcState`, `GraphNeighbors()` — non un modo
di spostare un'unità. Chi lo implementasse come `Unit->SetCell(Uscita)` bypasserebbe il grafo che il
pathfinding legge, e la raggiungibilità smetterebbe di essere una proprietà della mappa.

## 2. La matrice

Riga per riga, cosa vale per ciascuna famiglia. **La colonna «stato» dice cosa è vero oggi nel codice**, non
cosa vorremmo: una matrice che descrive un sistema immaginario è peggio di nessuna matrice.

| Regola | Move | Dash | **Transfer** | Forced |
|---|---|---|---|---|
| volontario | sì | sì | sì | **no** |
| fase | Move | Dash | quella dell'azione (`Leap`: Dash) | quella dell'effetto |
| occupa lo slot movimento | sì | **sì** ([D-028](../decisions/RT_PDR_00_Decision_Log.md)) | sì per `Leap`, che è nella fase Dash | no |
| micro-step | sì | **policy** | **no** | sì |
| attraversa le celle intermedie | sì | policy | **no** | sì |
| usa `MoveBudget` | sì | no | no | **no** |
| paga il costo del terreno | sì | no | no | **no** (ma vedi §3) |
| collisioni | sì | policy | solo all'arrivo | sì |
| hazard intermedi | sì | **policy** | **no** | **sì** |
| trigger spaziali | sì | sì | solo all'arrivo | sì |
| facing | derivato dal path | policy dell'azione | **policy dell'azione**, mai dal path (§2.1) | policy dell'effetto |
| rumore | profilo | profilo | partenza e arrivo, **nessun passo** | evento di impatto |
| auto-reroute | **mai** | mai | n/a | mai |
| consuma l'azione della vittima | n/a | n/a | n/a | **mai** |
| **stato nel codice** | implementato | implementato | **`LinearLeap`**, dentro il Dash · irraggiungibile dal roster ([#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645)) | implementato |

### 2.0 Un micro-step, un arco — la regola che il codice applica e che nessun documento diceva

> ✅ **Aggiunta il 2026-08-31 da [`D-305`](../decisions/RT_PDR_00_Decision_Log.md).** Non cambia niente:
> **registra** un invariante già implementato che nessuna sede dichiarava.

La riga *micro-step* della matrice dice **se** una famiglia genera micro-step. Non diceva **quanti passi**
ne attraversa uno, e la risposta è:

```text
MaxGraphTransitionsPerUnitPerMicroStep = 1
```

**Una unità avanza al massimo di un arco del grafo tattico per micro-step.** Vale per tutti i profili di
`Move` — `Sneak`, `Move`, `Sprint` ([D-015](../decisions/RT_PDR_00_Decision_Log.md)) — e per il `Forced`.
Lo `Sprint` **va più lontano** perché ha più budget (`8 MP` contro `5`, [`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) §2.1),
**non** perché percorra due celle nello stesso micro-step.

🔑 **«Arco del grafo», non «esagono adiacente»**, ed è la formulazione che conta. Il passo si legge da
`Paths`, che il pathfinder produce sulle `URTHexMapAsset::Transitions` (`HexSim.ReachableUsesTransitions`) e
non dall'adiacenza esagonale — quindi la regola vale senza riscritture per **rampe, scale, ponti, tunnel,
porte e transizioni multilivello**, e `FRTCellId::operator==` confronta anche il `Layer`.

È vero per costruzione, in una riga:

```cpp
// RTHexSimLibrary.cpp:654 — StepHexMovement
Target[i] = Done[i] ? Pos[i] : Paths[i][Prog[i] + 1];
```

⛔ **Perché scriverlo se il codice lo fa già.** Un `Transfer` non genera micro-step intermedi **per
decisione** ([D-118](../decisions/RT_PDR_00_Decision_Log.md)); questo invece lo faceva **per abitudine
d'implementazione**. Un invariante non dichiarato è ciò che la prossima ottimizzazione rimuove senza che
nulla protesti — e sotto c'è tutto quanto assume una sola transizione per volta: risoluzione delle
collisioni, hazard attraversati, finestra di Overwatch, cambi di LOS, leggibilità del replay.

⚠️ **Sotto il micro-step non esistono ulteriori istanti simulativi.** Possono esistere sotto-fasi
deterministiche di elaborazione dello **stesso** micro-step, ma non devono creare un ordine temporale fra
le unità: l'ordinamento stabile serve a processing, TurnLog, replay e hash, **mai** come precedenza di
gioco — ed è pinnato da `HexSim.ResolveOrderIndependent`, `Actions.Collisions.NoPlayerIdBias` e
`Movement.StepperIsDeterministicUnderPermutation`.

⏳ **Cosa questa riga NON decide.** Se alcuni profili possano diventare eleggibili a micro-step *alternati*
più rapidi è `MOV-3` in [`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), **aperta** e da playtestare; anche
in quel modello il tetto di **un arco per unità per micro-step** resterebbe. La precedenza fra due unità
che contendono la stessa cella è `MOV-4`, e oggi è `FRTActionDef::Priority`.

⛔ **Due test che pinnerebbero questa riga non esistono**, ed è dichiarato invece che sottinteso:
`OneTransitionMax_PerMicroStep` e `BlockedPath_DoesNotAutoReroute` (la riga *auto-reroute: mai* qui sopra)
hanno **zero** occorrenze in `Source/`. Oggi la regola è esercitata di fatto da
`Movement.StepperMatchesBatchResolver`, che non la nomina. Owner:
[#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000).

🔑 **Questa tabella dichiara test ATTESI, non presenti** — ed è la distinzione che
[ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) §Verifica non fa, con undici nomi di cui
zero esistono, cosa che ha già indotto in errore [D-295](../decisions/RT_PDR_00_Decision_Log.md).

### 2.1 Il `Transfer` esiste già, e vive dentro il Dash

`ERTMovementStyle::LinearLeap` — *«ignora unità e celle intermedie, conta solo dove si atterra»* — produce
`Result.Entered = { destinazione }` e nient'altro (`RTMovementActionLibrary.cpp`). Poiché
`ApplyTerrainOnEnterEffects` legge **esattamente** `Entered`, un `Action.Leap` non prende gli hazard
intermedi, non genera micro-step intermedi e collide solo all'arrivo: **le tre righe della colonna
`Transfer`**. Il test che lo prova è `RefactorTactics.Actions.Leap.IgnoresIntermediateCells`
(`RTMovementActionTests.cpp`).

È il motivo per cui due righe della colonna **Dash** sono passate da «sì» a «policy»: erano vere di
`LinearDash`, `LinearCharge` e `LinearPass`, e **false di `LinearLeap`**.

Quindi la frase «un movimento molto veloce non è un teletrasporto» è giusta, ma **`Leap` non è un movimento
veloce**: è un trasferimento, archiviato sotto Dash perché ne condivide **fase e slot** — non la semantica.
Questa distinzione è [D-118](../decisions/RT_PDR_00_Decision_Log.md).

> ⚠️ **Due righe della colonna `Transfer` sono regola preventiva, non descrizione.** `facing` e `rumore`
> non hanno oggi un consumatore: `LinearLeap` è irraggiungibile dal roster, quindi nessuno le esercita.
> Sono scritte perché sono i due punti in cui la scorciatoia è più tentante — un trasferimento **non ha un
> ultimo passo**, quindi `FacingFromPath(...)` su di esso produce un orientamento inventato, e un rumore
> «lungo il percorso» descriverebbe celle che non sono state attraversate.

> 🔴 **Ciò che il codice ancora non separa.** `Transfer` è oggi una famiglia **semantica**: `LinearLeap` è
> un valore di `ERTMovementStyle`, e quell'enum è **serializzato negli asset** (`FRTActionDef`). Il giorno
> in cui un `Blink` dovrà scegliere una cella qualsiasi entro range — e non una delle **sei direzioni
> lineari** che `LinearLeap` richiede — la separazione diventa una migrazione di formato, con
> compatibilità, validator e test. [D-118](../decisions/RT_PDR_00_Decision_Log.md) non sceglie fra «valore
> in coda» e «asse separato»: stabilisce che quella scelta appartiene a un checkpoint di **E39**, non a chi
> scriverà il primo Blink.

> ⚠️ Nulla di tutto questo si vede in partita: `Action.Leap` **non è nel kit di nessun eroe**, come
> `Action.Dodge` prima di lui ([#425](https://github.com/DegrassiAaron/refactor-tactics-main/issues/425)).

Le policy del Dash sono già un **dato**, non un ramo: `ERTMovementStyle` ha **sei** valori —
`None · Budget · LinearDash · LinearCharge · LinearLeap · LinearPass` — di cui `None` significa *«l'azione
non sposta chi la usa»* e i cinque restanti sono i modi di spostarsi. Aggiungere una famiglia di mobilità
significa aggiungere un valore lì, non un `if` nel resolver. *(Il conteggio è precisato il 2026-08-12: §6 di
questa pagina diceva già «sei valori», questo paragrafo ne elencava cinque, e da D-118 in poi la differenza
smette di essere un dettaglio.)*

## 3. Forced Movement — cosa ignora, e cosa no

> **Lo spostamento forzato ignora il costo *volontario* del terreno. Non ignora la geometria, e non ignora gli
> hazard.**

È la formulazione che mancava, e la distinzione è tutta qui: chi viene spinto attraverso `asciutto → fuoco →
fuoco → asciutto` **ha attraversato quelle due celle di fuoco**, e ne subisce le conseguenze, anche se non ha
speso un solo punto movimento. Il costo è ciò che si paga per *scegliere* di passare; la geometria è ciò che
c'è, e non chiede il permesso.

Ne segue, e vale come regola:

- lo spostamento forzato **non consuma** il `MoveBudget` della vittima, né la sua azione, né il suo Dash;
- **non** annulla il Move che ha già pianificato (per quello vedi §5, che è aperta);
- la **causa** va registrata: chi ha spinto, con quale azione. Un knockback che nel TurnLog appare come uno
  spostamento senza sorgente è indistinguibile da un difetto del resolver.

`PushResistance` resta una **soglia** e non una sottrazione ([D-038](../decisions/RT_PDR_00_Decision_Log.md)):
chi la possiede regge le spinte fino a quel valore e cede intere a quelle più forti.

> ✅ **Implementato il 2026-08-09** ([#308](https://github.com/DegrassiAaron/refactor-tactics-main/issues/308)).
> La verifica ha dato **no**: gli hazard non si applicavano affatto — né alle celle intermedie né a quella
> d'arrivo. La linea percorsa dalla spinta era calcolata «per l'animazione», e un commento nel resolver
> rimandava il danno da attraversamento «all'ambiente attivo (epic E8)». E8 è atterrata — sei checkpoint
> `INTEGRATED` — e quel punto non è stato ripassato: era un **rinvio scaduto**, non una decisione.
>
> Corretto per **entrambe** le vie dello spostamento forzato, spinta e trazione: la regola qui sopra parla di
> spostamento *forzato*, e trattarle in modo diverso rifarebbe l'asimmetria `ModifyArc`/`DamageArc` corretta
> in [#302](https://github.com/DegrassiAaron/refactor-tactics-main/issues/302), dove la stessa disciplina era
> mantenuta per una via di uscita e non per l'altra.
>
> Test: `Actions.Push.CrossesHazardsOfEveryCell` — **misura differenziale**, la stessa scena due volte con la
> sola cella di mezzo che cambia superficie, così il danno dell'azione che spinge esce dal conto da solo. La
> cella d'arrivo **non** brucia, deliberatamente: con l'arrivo in fiamme il test passerebbe anche applicando
> i soli effetti della destinazione, cioè col difetto mezzo corretto. E
> `Actions.Push.DoesNotSpendTheVictimMove`, che rende osservabile il terzo punto della regola — una vittima
> spinta nel Blast si muove comunque nel Move dello stesso turno.
>
> ⚠️ **Resta scoperto**: il danno da terreno non produce una voce di **TurnLog**, né per la spinta né per il
> movimento volontario — solo una riga di combat log. Chi legge un replay vede la salute calare senza un
> evento che lo spieghi. È preesistente e vale per entrambe le vie, quindi non è stato corretto qui.

## 3-bis. Quanto ci si può girare alla fine — non è una regola del movimento

Il facing **deriva** dal movimento (ADR-0005 §1 come emendata); *quanto* lo si possa correggere dopo è una
**statistica dell'eroe**, non una proprietà dello stile di mobilità.

La regola, la scala in step e i valori per personaggio vivono in
[**ADR-0008**](../decisions/adr-0008-rotazione-e-policy-di-facing.md) §1 ([D-060](../decisions/RT_PDR_00_Decision_Log.md)),
che su questo **supera** ADR-0005 §1. Non si duplicano qui: due tabelle degli stessi numeri sono due verità
in attesa di divergere.

Ciò che riguarda *questa* spec è la conseguenza sul movimento: **la stessa cella d'arrivo vale diversamente a
seconda del lato da cui la si raggiunge**, perché l'ultimo passo decide l'orientamento di partenza e il
budget di pivot decide quanto lo si può correggere. Un percorso più lungo può quindi essere la mossa
migliore. Per un eroe con pivot 0 alla fine di un Dash, l'ingresso **è** l'orientamento.

E durante il movimento — ADR-0008 §2 — il facing a ogni micro-step è quello dell'**ultimo passo compiuto**: è
ciò che leggono Overwatch, reazioni e cover direzionale ai boundary che cadono dentro il Move. Il pivot finale
si applica dopo, e **non retroattivamente**.

## 4. Trigger geografici e trigger semantici

Due domande diverse, che oggi il progetto tiene insieme e che conviene separare:

| Tipo | Risponde a | Esempi |
|---|---|---|
| **Geografico** | *cosa* è successo nello spazio | è entrato nell'area · ha attraversato quel bordo · ha lasciato l'adiacenza |
| **Semantico** | *come* è successo | si è mosso · ha scattato · è stato spinto · è comparso |

Il valore sta nel **combinarli**, ed è ciò che rende esprimibili trappole e controlli di zona senza codice
speciale:

```text
QUANDO  attraversa il bordo (E12 → E13)
E       il tipo di movimento è Dash
→       trappola anti-scatto

QUANDO  entra nell'area controllata
E       il tipo di movimento è QUALUNQUE
→       controllo di zona generico
```

**Conseguenza sull'Overwatch**, che è la ragione pratica per cui questa distinzione conta: la condizione
standard deve essere **spaziale**, non «il nemico ha usato Move». Altrimenti una spinta che porta un avversario
dentro l'area controllata non lo farebbe scattare — e la combo *Phase spinge → il nemico entra nell'area di un
alleato → opportunità di reazione* smetterebbe di esistere senza che nessuno l'abbia decisa.

Owner dell'Overwatch: [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) e
[ADR-0004](../decisions/adr-0004-finestre-di-reazione.md). Questa sezione dice **quale forma** deve avere il
trigger, non come si risolve la finestra.

## 5. Il Move pianificato quando l'unità viene spostata prima

**Decisa il 2026-08-10**: [D-045](../decisions/RT_PDR_00_Decision_Log.md) adotta **`Model A`** come baseline
della v0.1, esplicitamente rivedibile.

> Pianifico `H5 → H6 → H7`; una spinta nel Blast mi porta in `G5`; poi comincia il Move.
> **Se l'origine effettiva è diversa da quella su cui il percorso era stato pianificato, il Move decade.**

Le due alternative sono respinte per ragioni diverse:

- **`B`** — ricalcolare il percorso verso la stessa destinazione — è **escluso**, non rinviato: contraddice
  *«mai auto-reroute durante la risoluzione»*, già in vigore. Un percorso inventato dal computer cambia
  esposizione all'Overwatch, rumore, hazard e linea di tiro che il giocatore aveva scelto;
- **`C`** — rieseguire la sequenza di direzioni dalla nuova origine — conserva meglio l'intenzione, ma è
  complesso con porte e layer ed è il modello da provare **dopo**, non per primo.

**Criterio di uscita, quantificato**: se in playtest un Move viene annullato più di **una volta ogni due
round**, si prova `C`. È ciò che rende la revisione un fatto misurabile invece che un'opinione.

Il caso: pianifico `H5 → H6 → H7`; nel Blast una spinta mi porta in `G5`; poi comincia la fase Move.

| Modello | Cosa fa | Perché sì | Perché no |
|---|---|---|---|
| **A — annulla** | origine diversa da quella attesa → Move invalidato | semplice, leggibile, deterministico; dà peso agli spostamenti | anche una spinta di una cella annulla un turno di movimento |
| **B — ricalcola** | tiene la destinazione, ricalcola il percorso | preserva l'obiettivo | **il computer inventa un percorso che il giocatore non ha scelto**, e può portarlo dentro pericoli che aveva evitato |
| **C — preserva la sequenza** | riesegue le direzioni pianificate dalla nuova origine | conserva l'intenzione senza inventare; interessante nei turni simultanei | complesso con porte, layer, scale, archi |

**Baseline da testare: A. Alternativa: C. B è escluso** — contraddice la regola già in vigore *«mai
auto-reroute durante la risoluzione»*, che è implementata (`TruncatePathToTopology`, reason code
`BlockedByTopology`) e nata dalla stessa preoccupazione: un percorso che cambia da solo cambia anche
esposizione all'Overwatch, rumore, hazard e linea di tiro.

Criterio di uscita — perché una domanda aperta senza criterio non si chiude mai:

- si decide dopo uno scenario dedicato **e** una sessione di playtest, non prima;
- il segnale che A è troppo punitivo è **quante volte per partita** un Move viene annullato da una spinta:
  sopra una volta ogni due round, si prova C;
- chi decide è l'autore, e la decisione diventa una `D-0xx`.

## 6. Riconciliazione col kit d'autore

Il kit `RefactorTactics_Move_Consolidation_Claude.md` propone 59 sezioni. Non tutte erano nuove; **due erano
già decise** e consolidarle alla lettera avrebbe **regredito** il canone. Registrato qui perché la stessa cosa
succederà al prossimo kit.

| Sezione del kit | Verdetto | Dove vive davvero |
|---|---|---|
| §2 ordine delle fasi | **già canone** | [`spec-sequenza-turno.md`](spec-sequenza-turno.md) |
| §7 «Sprint non è Dash» | **già deciso** ([D-015](../decisions/RT_PDR_00_Decision_Log.md)); migrazione aperta nella issue `#199` | catalogo azioni |
| §18 facing: derivazione **e** limite di pivot | **da distinguere**, e la prima stesura di questa riga sbagliava dandolo per «superato dai fatti». La *derivazione dal movimento* è canone (E16 chiusa, 13 test `Facing.*`, 5 scenari `Spec.Facing.*`). Il *limite di rotazione* **non lo era**: ADR-0005 lo dichiarava fuori perimetro e viveva come `FAC-1`, in attesa dell'autore. È stato **recepito** il 2026-08-10: la rotazione è una capacità del personaggio, misurata in step, con due valori per eroe — [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) §1, [D-060](../decisions/RT_PDR_00_Decision_Log.md) | [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md), che **supera** ADR-0005 §1 |
| §2 e §11 «`Dash → Attack → Move` può essere legale» | ⚠️ **contraddice** [D-028](../decisions/RT_PDR_00_Decision_Log.md): lo scatto **occupa lo slot movimento**, quindi la sequenza non è legale come regola generale — si sceglie *schivo e sparo* **oppure** *sparo e muovo*. Un eroe può dichiararla come eccezione **nel proprio kit**; il ruleset no | [`spec-dash.md`](spec-dash.md) |
| §19 collisioni (contesa e swap bloccano entrambi) | ✅ **implementato** dal 2026-08-31 ([#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922)): la **contesa** lo era già; lo **swap** e il **ciclo chiuso** bloccano ora con `ERTMoveOutcome::BlockedByCycle`, e il **convoy a coda libera** continua ad avanzare. 🔄 Questa riga diceva «già implementato» per entrambi, e la seconda metà è stata misurata falsa il 2026-08-31 ([D-295](../decisions/RT_PDR_00_Decision_Log.md)): allora `HexSim.ResolveSwapAllowed` era verde e asseriva che lo scambio riusciva. Ora è vera per misura, non per dichiarazione. | `ERTMoveOutcome::BlockedContested` (contesa) · `ERTMoveOutcome::BlockedByCycle` (scambio e ciclo) · `HexSim.ResolveSwapBlocked` |
| §23 mai auto-reroute | **già implementato** | `TruncatePathToTopology` |
| §6 micro-step | **già implementato** | `ResolveHexPaths` |
| §36 dieci reason code nuovi | **duplicati**: sette esistono con altri nomi, in un enum **serializzato nei replay** | `ERTMoveOutcome` |
| §29 `MovementKind` | **duplicato** di `ERTMovementStyle`, che ha già sei valori | `RTActionDef.h` |
| §40 macro-roadmap `F0–F6` da preservare | ⚠️ **contraddice** la vista viva (M6–M11 + le epic della v0.1); nel canone `F0–F6` è *direzione, non scope* | [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) |
| §37/§38 «Product Map», «Feature Map» | **non esistono** con quei nomi | `../roadmap/feature-registry.yaml`, [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) |
| §10 matrice, §15 trigger, §22 forced/terreno, §25 domanda aperta | ✅ **nuovi e recepiti** | questa pagina |

**La lezione, che vale oltre il movimento**: un kit scritto in parallelo al lavoro non conosce ciò che è
atterrato nel frattempo. Prima di consolidarne uno, ogni sezione va marcata *già deciso / già implementato /
nuovo / contraddice* — altrimenti si retrocede una decisione chiusa a «candidata» e nessuno se ne accorge,
perché il documento sembra più recente.

## 7. Cosa resta fuori

- **Il contenuto di `Transfer` oltre `Leap`** — Blink, Swap, Recall, forced transfer: è **E39**, milestone
  **v0.2** ([D-119](../decisions/RT_PDR_00_Decision_Log.md)). Questa pagina possiede la **regola** di cosa
  fa un trasferimento; non possiede quali azioni lo useranno né quando. Le due domande che stavano qui
  aperte — `MOV-1` e `MOV-2` — sono chiuse il 2026-08-12 da
  [D-118](../decisions/RT_PDR_00_Decision_Log.md) e [D-119](../decisions/RT_PDR_00_Decision_Log.md).
- **Il resolver dei trasferimenti simultanei**: due unità che si trasferiscono sulla stessa destinazione nello
  stesso turno. Oggi non c'è conflitto da risolvere perché non c'è un trasferimento pianificabile;
  l'invariante da rispettare quando ci sarà è già canone e non è nuova — *l'ordine di iterazione non decide
  un esito* (`AGENTS.md`, invariante 3). È **CP 39.2**.
- **Rinominare i reason code** sul vocabolario del kit: sarebbe una migrazione di formato del TurnLog con il
  corpus golden dentro, e non porta nulla al giocatore.
- **Le 15 issue `MOV-CORE-*` del kit**: tre coincidono con checkpoint già aperti — `#162` (micro-step
  step-able), `#163` (opportunity → commit), `#164` (overwatch a micro-step) — e vanno consolidate lì, non
  duplicate.
- **La pipeline a 16 passi del kit §28**: va confrontata con l'invariante *raccogli poi applica* e con
  ADR-0004 prima di poter essere scritta come canone. Non lo faccio qui.

## Rapporto con gli altri documenti

| Documento | Cosa possiede |
|---|---|
| [`spec-dash.md`](spec-dash.md) | il Dash: modello dati, risoluzione, bot, playback |
| [`spec-sequenza-turno.md`](spec-sequenza-turno.md) | l'ordine delle fasi e dei boundary |
| [`brief-azioni-generiche-overwatch.md`](brief-azioni-generiche-overwatch.md) | Overwatch e le sue policy |
| [ADR-0005](../decisions/adr-0005-orientamento.md) | il facing come stato di gioco |
| [`../technical/architecture/spec-pathfinding-pf3-pf4.md`](../technical/architecture/spec-pathfinding-pf3-pf4.md) | grafo, A\*, costi |
| [`spec-economia-del-turno.md`](spec-economia-del-turno.md) | come il budget di movimento convive con gli **altri tre** limiti del turno. Quale slot occupa ciascuna famiglia resta la **§2** di questa pagina |
| [`spec-compatibilita-azioni-movimento.md`](spec-compatibilita-azioni-movimento.md) | che il profilo scelto cambi **legalità ed efficacia** delle azioni: `AE-2`, chiusa da [D-116](../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-12 col modello a **soglia** (`MinStability` contro `Stability`) |
| *questa pagina* | il confronto **fra le famiglie** di movimento |
