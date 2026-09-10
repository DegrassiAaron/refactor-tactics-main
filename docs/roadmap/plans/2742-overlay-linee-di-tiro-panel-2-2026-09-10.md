# #2742 — Le linee di tiro · panel di specifica **(seconda passata)** · 2026-09-10

> **Comando**: `/issue-run 2742` → `sc:spec-panel`, modalità **critique**, focus `architecture · requirements`.
> **Panel**: Fowler (lead) · Cockburn · Wiegers · Nygard · Adzic.
> **Base**: `origin/main = 18d85a56`, worktree isolato `rt-wt-2742`.
> **Contesto**: la prima passata (2026-09-09) si era chiusa `BLOCKED` su #1941. Quel blocco è **caduto** con `D-368`.

## §0 — ⛔ Il blocco è caduto, ma ne emerge un altro, e non è tecnico

**Questa issue e [#1944](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1944) hanno lo stesso deliverable, e #1944 dichiara per iscritto di esserne l'unico owner.**

Lo scope di #1944 (OVL-04, **OPEN**) dice:

> *«Aprire i tre che **mancano**: **Vision/LOS**, Hazard, Objective.»*

e la riga che lo motiva è esplicita:

> *«non è più #1712 a dover consumare il canale di #1941 — è **#1941 che ha ereditato l'overlay LOS**, per deferimento esplicito, e **questa famiglia è l'unico owner rimasto**. La `Resa` della LOS in partita **non esiste ancora**.»*

📊 #1944 è stata aperta il **2026-08-31**. #2742 il **2026-09-09**, nove giorni dopo, dallo spec panel su una richiesta dell'autore — senza che nessuna delle due nomini l'altra.

🔴 **`CLAUDE.md` §12 è netto**: *«Non duplicare issue o responsabilità»*, e *«una issue ha un solo primary owner»*. Due issue aperte che costruiscono la resa della LOS sono esattamente il caso che quella regola vieta.

⚠️ **Non è una formalità di tracciamento.** Se entrambe procedono, due implementazioni disegneranno la stessa informazione con due grammatiche, e la seconda a atterrare troverà la prima — che è il modo in cui nasce il *«sesto blocco di `FColor` letterali»* che tutta la famiglia OVL esiste per evitare.

## §1 — La lettura che POTREBBE separarle, e perché non regge da sola

**Fowler**: c'è una distinzione tecnica reale fra i due deliverable —

| | Forma | Dato |
|---|---|---|
| **Vision/LOS** come lo intende v0.2 | un'**area**: quali celle vedo | insieme di `FRTCellId` |
| **Linee di tiro** come le chiede #2742 | una **linea**: la traiettoria e dove si interrompe | `FRTLineOfSightResult` per coppia |

∴ si potrebbe dire: #1944 possiede l'*area*, #2742 possiede la *linea*.

⛔ **Ma #1944 cita `DescribeLineOfSight` per nome**, cioè il produttore della linea, non dell'area. La separazione non è nei testi: sarebbe una distinzione introdotta adesso per far coesistere due issue, e andrebbe **decisa e scritta**, non dedotta.

📝 È una decisione a basso costo e alto valore — ma resta una decisione.

## §2 — I rilievi, validi in qualunque modo si risolva l'ownership

### 🔴 WIEGERS — il D010 conta su un produttore che non esiste, e questo non è cambiato

> *«il **produttore esiste già ed è esercitato**, perché consegnato da #2741»*

Rimisurato su `main = 18d85a56`: `git grep -lE 'AuthorizedLines|LinesForObserver|VisibleLines|LineOfSightFor' -- Source/` → **niente**. #2741 ha consegnato `RefusalForObserver`, che collassa **un** verdetto al click.

∴ chiunque possieda questa resa deve **anche** costruire il produttore. Il D001 lo aveva escluso dallo scope contando su un fornitore che non ha consegnato.

### 🟡 FOWLER — il modello di #1941 non ha ancora un consumatore lato disegno

Misurato: `RTHexMapActor.cpp` usa `URTOverlayPalette` (**7** chiamate: `ColorFor`, `PriorityFor`, `DrawsThroughUnits`) ma **zero** volte `FRTOverlayArea`.

∴ la **palette** è entrata nel renderer; la **struttura area** no. Chi porta la LOS sul canale sarebbe il primo a consumare il modello per intero — che è precisamente il lavoro che #1944 si intesta.

⚠️ Il rischio non è duplicare codice: è che due issue definiscano *come* si consuma il modello, e la seconda debba disfare.

### 🟢 COCKBURN — le quattro domande di forma hanno risposte suggerite dal repository

Non serve inventarle: tre delle quattro hanno già un precedente misurabile.

| Domanda | Cosa dice il repository |
|---|---|
| **Durata** | `ARTTurnManager::OnLockInCommitted` **esiste** (`RTTurnManager.h:976`) ed è già il segnale che spegne le anteprime. ⛔ Inventarne un secondo sarebbe il difetto. |
| **Forma del blocco** | `FRTLineOfSightResult` porta `Block`, `BlockedAt`, `BlockedFrom` **e** `StepIndex`: entrambe le forme — troncata al punto d'urto, o intera con l'ostacolo marcato — sono costruibili **senza nuovo calcolo**. È scelta di resa, non di dato. |
| **Costo** | `DescribeLineOfSight` cammina `HexLine(From, To)` cella per cella. Una chiamata per nemico per frame di hover è un profilo diverso da una al click, e `DrawPlanningPreview` è già chiamata dal `Tick`. |
| **Cardinalità** | 🔴 **l'unica senza precedente**: una linea sotto il cursore, o tutte le linee dall'unità selezionata? La seconda è `O(unità × nemici)` — fino a **16** linee simultanee in un 2v2. |

### 🟢 NYGARD — la privacy ha già la sua forma, e non è un filtro nel disegno

Il precedente corretto esiste ed è `ARTHUD::ComputeBlockerMarks`, che **non rifiltra** perché il feed le arriva già filtrato, e il cui commento dichiara che un secondo contratto di conoscenza sarebbe *il* difetto.

∴ il produttore delle linee deve nascere **già autorizzato** — riceve l'osservatore e restituisce solo ciò che quell'osservatore può sapere — invece di produrre tutto e lasciare che il renderer scarti. `D-225` non è una clausola da aggiungere: è la firma della funzione.

### 🟢 ADZIC — il canary di privacy va scritto come uguaglianza, non come assenza

Il D010 chiede *«due stati nascosti diversi, con la stessa conoscenza autorizzata, producono lo stesso insieme di linee»*. ✅ È la forma giusta, ed è la stessa già usata da `Combat.RefusalHidesTheUnknownTarget`: si asserisce l'**uguaglianza fra i due esiti**, non che ciascuno sia vuoto — due insiemi entrambi non vuoti ma diversi sarebbero essi stessi il canale.

## §3 — Consenso

1. ⛔ **L'ownership va risolta prima del codice.** Non è un dettaglio di tracciamento: decide chi definisce come si consuma il modello di #1941.
2. ✅ **Il blocco su #1941 è davvero caduto** — le tre precondizioni sono misurate e soddisfatte, `D-368` compresa.
3. 🔴 **Il produttore delle linee autorizzate non esiste** e va costruito da chiunque possieda la resa.
4. ✅ **Tre delle quattro domande di forma hanno già una risposta** nel repository; solo la **cardinalità** è aperta, ed è quella che decide il costo.
5. 🔑 **La privacy sta nella firma del produttore**, non in un filtro a valle.

## §4 — La decisione, presa in sessione

✅ **Uscita (b): #2742 possiede la LINEA, #1944 possiede l'AREA.** Decisione d'autore del 2026-09-10.

| | Forma | Domanda del giocatore |
|---|---|---|
| **#1944** — Vision/LOS come area | quali celle vedo | *«cosa copro da qui?»* |
| **#2742** — linee di tiro | la traiettoria verso un bersaglio, e dove si interrompe | *«posso colpire QUESTO?»* |

Stessa sorgente — `URTHexVisionLibrary` resta il produttore unico — due rese con forma, costo e grammatica diversi.

⛔ **La distinzione va scritta in ENTRAMBE le issue**, ed è la condizione della decisione: oggi nessuna delle due la nomina, e senza quella scrittura il conflitto si ripresenta al prossimo panel che legga #1944 e trovi *«questa famiglia è l'unico owner rimasto»*.

### 🔑 La decisione risponde anche alla quarta domanda

La **cardinalità** era l'unica senza precedente nel repository. Ora ce l'ha, e discende dal taglio invece di essere scelta a parte:

> se la **panoramica** — tutte le celle che vedo — appartiene all'area di #1944, allora la linea di #2742 è quella verso **il bersaglio puntato**.

∴ **cardinalità 1**, costo `O(1)` per frame invece di `O(unità × nemici)`. Le «fino a 16 linee simultanee in un 2v2» che la issue temeva non si presentano: quel caso è l'area, e ha un'altra resa e un altro owner.

⚠️ **Conseguenza da non perdere**: senza un bersaglio puntato non c'è linea da disegnare. Il caso *«voglio vedere tutte le mie linee di tiro insieme»* **non è servito da questa issue** — è la panoramica, ed è di #1944.

## §5 — Le tre risposte che restano, e da dove vengono

| Domanda | Risposta | Fonte |
|---|---|---|
| **Cardinalità** | una linea, verso il bersaglio puntato | §4, discende dal taglio |
| **Durata** | fino a `OnLockInCommitted` | `RTTurnManager.h:976`, già il segnale che spegne le anteprime |
| **Forma del blocco** | ⏳ resa, da decidere in implementazione: `BlockedAt` **e** `StepIndex` rendono entrambe costruibili senza nuovo calcolo |
| **Costo** | una chiamata per frame di hover, `O(1)` | `DescribeLineOfSight` cammina `HexLine`; con cardinalità 1 il profilo è lo stesso di un hover qualsiasi |
