# Spec — Tassonomia del movimento

> `CURRENT` · **Owner** della domanda «di che tipo di movimento stiamo parlando, e cosa comporta».
> Creato il **2026-08-09** consolidando `RefactorTactics_Move_Consolidation_Claude.md` (kit d'autore).
> Non sostituisce nessun documento: [`spec-dash.md`](spec-dash.md) resta owner del **Dash**,
> [`spec-sequenza-turno.md`](spec-sequenza-turno.md) dell'**ordine delle fasi**,
> [`../technical/spec-pathfinding-pf3-pf4.md`](../technical/spec-pathfinding-pf3-pf4.md) del **grafo e dell'A\***.
> Qui vive ciò che nessuno di loro possiede: il confronto **fra le famiglie**.

## Perché esiste

Il progetto ha quattro modi diversi di spostare un'unità e li ha costruiti in momenti diversi — il Move a M6.2,
il Dash a M6.5, la spinta con il knockback esagonale, `Action.Reposition` col motore azioni. Ognuno ha il suo
documento; **nessuno risponde alla domanda che si fa chi progetta un'abilità nuova**: se sposto qualcuno, cosa
scatta? paga il movimento? attraversa il fuoco?

Rispondere caso per caso è il modo in cui nascono le regole per eroe. Questa pagina è la risposta unica.

## 1. Le famiglie

```text
Move       movimento volontario standard, fase Move, a budget
Dash       movimento volontario speciale, fase Dash, senza budget
Forced     spostamento subito: push, pull, knockback, correnti
Teleport   comparsa senza attraversamento          (NON ESISTE in v0.1)
Reaction   movimento causato da una reazione       (usa la policy di una delle altre)
```

`Reaction Movement` **non è una quinta meccanica**: è una delle quattro con una causa diversa. Ciò che deve
restare distinguibile è la *causa*, non il modo di muoversi — chi legge il TurnLog deve poter dire se
quell'unità si è mossa perché l'ha deciso, perché è stata spinta o perché una reazione l'ha fatta scattare.

## 2. La matrice

Riga per riga, cosa vale per ciascuna famiglia. **La colonna «stato» dice cosa è vero oggi nel codice**, non
cosa vorremmo: una matrice che descrive un sistema immaginario è peggio di nessuna matrice.

| Regola | Move | Dash | Forced | Teleport |
|---|---|---|---|---|
| volontario | sì | sì | **no** | (sì) |
| fase | Move | Dash | quella dell'effetto | quella dell'abilità |
| occupa lo slot movimento | sì | **sì** ([D-028](../decisions/RT_PDR_00_Decision_Log.md)) | no | dipende dall'abilità |
| micro-step | sì | sì | sì | no |
| attraversa le celle intermedie | sì | policy | sì | **no** |
| usa `MoveBudget` | sì | no | **no** | no |
| paga il costo del terreno | sì | no | **no** (ma vedi §3) | no |
| collisioni | sì | policy | sì | solo all'arrivo |
| hazard intermedi | sì | sì | **sì** | no |
| trigger spaziali | sì | sì | sì | solo all'arrivo |
| facing | derivato dal path | policy dell'azione | policy dell'effetto | policy dell'abilità |
| rumore | profilo | profilo | evento di impatto | nessun passo |
| auto-reroute | **mai** | mai | mai | n/a |
| consuma l'azione della vittima | n/a | n/a | **mai** | n/a |
| **stato nel codice** | implementato | implementato | implementato | **assente** |

**Teleport non esiste in v0.1** — nessuna azione del catalogo lo produce. La colonna resta perché la regola
serve *prima* che qualcuno lo scriva: è il caso in cui è più facile sbagliare, e la riga «non attraversa le
celle intermedie» è quella che distingue un teletrasporto da un movimento veloce.

Le policy del Dash sono già un **dato**, non un ramo: `ERTMovementStyle` vale
`Budget · LinearDash · LinearCharge · LinearLeap · LinearPass`. Aggiungere una famiglia di mobilità significa
aggiungere un valore lì, non un `if` nel resolver.

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
dentro l'area controllata non lo farebbe scattare — e la combo *Riva spinge → il nemico entra nell'area di un
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
| §19 collisioni (contesa e swap bloccano entrambi) | **già implementato** | `ERTMoveOutcome::BlockedContested` |
| §23 mai auto-reroute | **già implementato** | `TruncatePathToTopology` |
| §6 micro-step | **già implementato** | `ResolveHexPaths` |
| §36 dieci reason code nuovi | **duplicati**: sette esistono con altri nomi, in un enum **serializzato nei replay** | `ERTMoveOutcome` |
| §29 `MovementKind` | **duplicato** di `ERTMovementStyle`, che ha già sei valori | `RTActionDef.h` |
| §40 macro-roadmap `F0–F6` da preservare | ⚠️ **contraddice** la vista viva (M6–M11 + 21 epic); nel canone `F0–F6` è *direzione, non scope* | [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) |
| §37/§38 «Product Map», «Feature Map» | **non esistono** con quei nomi | [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml), [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) |
| §10 matrice, §15 trigger, §22 forced/terreno, §25 domanda aperta | ✅ **nuovi e recepiti** | questa pagina |

**La lezione, che vale oltre il movimento**: un kit scritto in parallelo al lavoro non conosce ciò che è
atterrato nel frattempo. Prima di consolidarne uno, ogni sezione va marcata *già deciso / già implementato /
nuovo / contraddice* — altrimenti si retrocede una decisione chiusa a «candidata» e nessuno se ne accorge,
perché il documento sembra più recente.

## 7. Cosa resta fuori

- **Teleport**: nessuna azione lo produce in v0.1. Le sue righe in matrice sono una regola preventiva.
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
| [`../technical/spec-pathfinding-pf3-pf4.md`](../technical/spec-pathfinding-pf3-pf4.md) | grafo, A\*, costi |
| [`spec-economia-del-turno.md`](spec-economia-del-turno.md) | come il movimento **pesa sul resto del turno**: quale slot occupa una famiglia, e la proposta **aperta** (`AE-2`) che il profilo scelto cambi legalità ed efficacia delle azioni. Qui il profilo cambia distanza, rumore ed esposizione, e finisce lì |
| *questa pagina* | il confronto **fra le famiglie** di movimento |
