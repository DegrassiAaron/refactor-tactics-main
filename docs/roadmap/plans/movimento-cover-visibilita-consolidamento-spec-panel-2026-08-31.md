# Movimento, Cover e Visibilità — referto di consolidamento di una seduta d'autore

> **Referto di revisione**, non owner. Consuma il mandato di seduta *«Movement Resolver / micro-step / cover
> direzionale / exposure / Last Known / RiseToFire»* (2026-08-31) e ne misura ogni clausola contro il
> repository.
>
> **Data**: 2026-08-31 · **Base**: `origin/main` `3b3aafc4` · **Modo**: critique · **Focus**: requirements +
> architecture
>
> **Owner**: nessuno. Ogni regola citata appartiene all'owner indicato in colonna; se questo referto e il suo
> owner divergono, **vince l'owner**.
>
> ⛔ **Nessuna riga di `Source/` toccata, nessuna suite eseguita.** Tutti i verdi citati sono lo **stato
> dichiarato** del repository più una misura per nome su `Source/`, non una run presa qui.

---

## 1. Il verdetto in una riga

> **Delle ventuno clausole del mandato, undici sono già canone — quattro verbatim, col tipo e il test già in
> `Source/` — e la reticola a dodici anchor che il mandato «introduce» è un enum spedito, chiuso stamattina.
> I tre conflitti non sono sulla sostanza ma sulla scrittura: un campo serializzato nel TurnLog che il §3
> chiederebbe di togliere, un contratto di serializzazione che il §9 rovescerebbe, e un nome già occupato
> tre volte. Il valore reale sono cinque delta, di cui il maggiore è una regola implementata che nessun
> documento dichiara.**

Il mandato è scritto come se il perimetro fosse aperto. Non lo è: `D-289`, `D-294`, `D-295`, `D-302` e
`D-303` lo hanno attraversato **oggi**, e `D-302`/`D-303` sono entrate in `main` alle **18:25 UTC** con la
PR [#1984](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1984).

---

## 2. Ciò che è stato misurato

Ogni riga è un comando eseguito, non una lettura.

| Domanda | Comando | Esito |
|---|---|---|
| Baseline di lavoro | `git status` · `git log -1` | ✅ albero **pulito**, `HEAD` staccato su `3b3aafc4` = `origin/main` |
| Il micro-step avanza di quanto? | `RTHexSimLibrary.cpp:654` | ✅ `Target[i] = Paths[i][Prog[i] + 1]` — **un passo, per costruzione** |
| Esiste una priorità nella contesa? | `RTHexSimLibrary.cpp:636,679-701` | 🔴 **Sì**, ed è `FRTActionDef::Priority` del catalogo |
| È serializzata? | `RTTurnLog.h:362-368` | 🔴 **Sì** — `ERTMoveOutcome::BlockedByPriority`, formato **v7** |
| Il vocabolario di targeting esiste? | `RTTeamKnowledge.h:115-125` | ✅ `ERTTargetKnowledge { Allowed, CellOnly, Rejected }` — **verbatim** |
| `Exposed` è libero come termine? | `RTGameplayTags.h:9` · `RTVisibilityBorder.h:21` | 🔴 **No**: `TAG_Status_Exposed` (+5 al primo danno diretto) e `FRTExposedEdge` |
| `RiseToFire` esiste? | `grep -ril` su `Source/` e `docs/` | ⛔ **0** fuori da `docs/research/` |
| `Take Cover` esiste? | idem | ⛔ **0** fuori da `docs/research/` |
| `Sneak` esiste nel codice? | `grep -rn Sneak -- Source` | ⚠️ **1 occorrenza**, ed è un commento di test che cita `D-015` |
| Le sei direzioni si chiamano così? | `RTCellId.h:11-19` | ✅ `E · NE · NW · W · SW · SE`, anello coerente |
| La reticola a tredici anchor esiste? | `RTGeometryGrammar.h:229` | ✅ `ERTAnchorKind { Center, Vertex, EdgeMid }` + `FRTAnchorRef` — **chiusa oggi** da `D-288` |
| È serializzata? | `RTGeometryGrammar.h:249` | 🔴 **No, per contratto**: *«non è serializzato e non entra in `ComputeHash`»* |
| `CoverAnchor` (quello di copertura) è un tipo? | `spec-cover-placement-intra-hex.md` §5 | ⏳ **non rappresentato** — ed è *presentazione/authoring*, non gameplay |
| `COV-2` è ancora aperta? | `OPEN_DECISIONS.md` | 🔴 **Chiusa oggi** da `D-302` |
| Il prossimo `D-nnn` libero | registro a `HEAD` + diff di ogni PR aperta + ogni branch remoto | **`D-305`** — `D-304` è l'ultimo assegnato, nessuna PR aperta tocca il registro |
| Le issue candidate sono vive? | `gh issue view` ×8 | #1833 · #1830 · #1829 · #1466 · #152 · #324 · #1826 **OPEN** · **#1922 CLOSED** |

⚠️ **Una misura di stamattina è già stantia.** Il referto
[`movement-microsteps-facing-pivot-spec-panel-2026-08-31.md`](movement-microsteps-facing-pivot-spec-panel-2026-08-31.md)
§5 elenca swap e ciclo chiuso come gap aperti su [#1922](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1922).
**#1922 è chiusa**, e i tre test esistono per nome: `HexSim.ResolveSwapBlocked`,
`HexSim.ResolveSwapBlockedEvenWhenPassingThrough`, `HexSim.ResolveClosedCycleBlocked`. Il commento in
`RTHexSimTests.cpp:650` lo dichiara — *«questo test si chiamava `ResolveSwapAllowed` e asseriva l'opposto»*.

---

## 3. La mappa: ventuno clausole contro i loro owner

`✅` già canone · `⚠️` parziale o vocabolario diverso · `🔴` conflitto · `🆕` delta reale

| § | Clausola del mandato | Owner corrente | Esito |
|---|---|---|---|
| 1 | `MicroStep` = minima unità autorevole; **1 transizione di grafo** per unità | `StepHexMovement` (`RTHexSimLibrary.cpp:654`) | 🆕 **vera e non dichiarata** — §5.1 |
| 2 | Intenzioni raccolte prima di mutare lo stato; ordine stabile ≠ priorità di gioco | `StepHexMovement` punto fisso · `HexSim.ResolveOrderIndependent` | ✅ spedito |
| 3a | Stessa cella, stesso micro-step → **tutti bloccati** | `HexSim.ResolveContestedDestination` · `ResolvePriorityTieStillContested` | ⚠️ vero **a parità di priorità** |
| 3b | «non introdurre initiative/speed/UnitId priority» | `FRTActionDef::Priority` | 🔴 **conflitto** — §4.1 |
| 3c | Arrivo anticipato occupa per chi arriva dopo | `spec-cover-placement-intra-hex.md` §7 (2) | ✅ canone |
| 4 | `MoveBlocked`, residuo interrotto, **nessun reroute automatico** | `spec-tassonomia-movimento.md` §2 — riga *auto-reroute: **mai*** | ✅ canone su tutte e quattro le famiglie |
| 5 | `Move`/`Sneak`/`Sprint` = stessa cadenza, 1 arco/micro-step | `D-015` · `RT_ActionCatalog_v0.1.md` §2.1 | ✅ canone |
| 6 | Sprint **va più lontano**, non due celle in un micro-step | catalogo: `Move` 5 MP · `Sprint` 8 MP, `ERTMovementStyle::Budget` | ✅ canone, e i tradeoff sono già quelli elencati |
| 7 | **FAST MicroSteps** come opzione di playtest | *nessuno* | 🆕 **delta** → `MOV-3` |
| 8 | `CoverMask` sui sei bordi; cover fisica ≠ postura | `FRTHexCover` sui bordi (`D-129`) · `FRTOccupancyMask` a **dodici settori** | ⚠️ due maschere già esistono, §4.2 |
| 9 | `Take Cover` + dodici anchor (6 edge + 6 corner) + `CENTER` non-anchor | `ERTAnchorKind` (`RTGeometryGrammar.h:229`), chiuso da `D-288`/`GEO-8` | ⚠️ **la reticola esiste 1:1**; il conflitto è sul contratto — §4.2 |
| 10 | Validità degli anchor **derivata** dalla maschera, non hardcodata | `spec-cover-placement-intra-hex.md` §3, §11 (regioni derivate da un anello di bit) | ✅ è già la forma del modello |
| 11 | **Active Cover** = solo gli edge dell'anchor, non l'intera maschera | `FRTCoverOption` + `AccessMask` · `CoverPlacement.SelectingOneSourceDoesNotSuppressTheOthers` | ⚠️ stessa idea, altro nome — §4.2 |
| 12 | Exposure **relativa alla coppia** osservatore→bersaglio, mai globale | `FRTTeamKnowledge` per squadra · `Vision.TeamKnowledgeIsUnion` | ⚠️ vero della *conoscenza*, non esiste come **geometria** → `PER-5` |
| 13 | Baseline discreta per `Edge_X` | *nessuno* | 🆕 **delta** → `PER-5` |
| 14 | Baseline discreta per `Corner_X_Y` | *nessuno* | 🆕 **delta** → `PER-5` |
| 15 | Pipeline `Authoritative → … → Sanitized Client View`; nessun leak grafico | invariante #6 · `RTIntentPrivacyLibrary` · `UI.NoEnemyIntentExposed` | ✅ canone, e pinnato |
| 16 | `LastKnown` non insegue la posizione nascosta | `FRTLastKnownContact` · `Vision.PlaybackTroncaAlTrattoOsservato` · `LastContactExpiresAfterOneTurn` | ✅ canone, e pinnato |
| 17 | Targeting `Allowed` / `CellOnly` / `Rejected` | `ERTTargetKnowledge` (`RTTeamKnowledge.h:115`) | ✅ **verbatim**, commento incluso |
| 18 | `RiseToFire`: esposizione ricalcolata nella finestra, e il visto resta visto | *nessuno* | 🆕 **delta** → `OW-6` |
| 19 | Non modificare l'Overwatch standard; nuovi trigger = reaction distinta | `ADR-0004` · E14 [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152) | ✅ coerente con il canone |
| 20 | Cover dinamica: ricalcolo, nessun teletrasporto verso un nuovo anchor | `COV-8` **aperta** · `Cover.Destruction.*` (9 test) | ⚠️ metà spedita, metà è `COV-8` |
| 21 | La quota non concede bonus numerici astratti | `CLAUDE.md` §2 · `AGENTS.md` | ✅ pin esistente |

**Conteggio** su 23 righe (il §3 ne occupa tre): **11 ✅ · 6 ⚠️ · 1 🔴 · 5 🆕**.

⚠️ **I conflitti sono tre e non uno**, ma solo uno è una riga della tabella: `§4.1` è la riga `3b`, mentre
`§4.2` (contratto di serializzazione) e `§4.3` (vocabolario) attraversano righe marcate `⚠️`, perché in
entrambi i casi la clausola è **sostanzialmente accolta** e ciò che confligge è come si scrive.

---

## 4. I tre conflitti

### 4.1 🔴 «Nessuna priorità» toglierebbe un campo serializzato

Il mandato §3 chiede, per il vertical slice: *«Result = all conflicting entries blocked»*, e *«non
introdurre initiative score, speed priority o UnitId priority»*.

**Le due frasi non chiedono la stessa cosa, e il repository risponde in modo opposto alle due.**

La seconda è **già rispettata, e non per omissione**: initiative, velocità e `UnitId` non decidono nulla, e
tre test lo pinnano — `Actions.Collisions.NoPlayerIdBias`, `HexSim.ResolveOrderIndependent`,
`Movement.StepperIsDeterministicUnderPermutation`.

La prima **non è lo stato del repository**. Esiste una precedenza, ed è dichiarata dall'**azione** nel
catalogo:

```cpp
// RTHexSimLibrary.h:236 — numero PIU' BASSO vince
auto PriorityOf = [&State](int32 i) { return State.Priorities.IsValidIndex(i) ? State.Priorities[i] : 0; };
...
Reason = (Winners >= 2) ? ERTMoveOutcome::BlockedContested : ERTMoveOutcome::BlockedByPriority;
```

🔑 **`FRTActionDef::Priority` non è nessuna delle tre cose che il mandato vieta.** Non è iniziativa, non è
velocità, non è identità: è un **dato di catalogo dichiarato dall'azione**, cioè esattamente la forma
data-driven e deterministica che il mandato approva altrove. Il divieto colpisce tre meccanismi che non ci
sono e ne travolgerebbe un quarto che non nomina.

🔴 **E il costo non è una riga di codice.** `ERTMoveOutcome` è **serializzato nel TurnLog in formato v7**
(`RTTurnLog.h:362`, `Entry.Priority` a `RTHexSimLibrary.cpp:966`). Toglierlo è una migrazione di formato con
rigenerazione del corpus golden — la stessa conseguenza già pagata da `D-245`.

⚠️ **A parità di priorità il mandato è già vero**, e ha il suo test:
`HexSim.ResolvePriorityTieStillContested`. `Priority` vale `0` per chi non la dichiara, e con tutti gli array
vuoti *«l'esito è identico alla variante senza priorità»* — lo dichiara il commento della funzione stessa.

∴ **Non si tocca il resolver.** La clausola va riformulata come *«a parità di precedenza dichiarata»*, oppure
diventa una decisione esplicita di rimozione con la sua migrazione. È `MOV-4`.

### 4.2 🔴 La reticola del mandato non è nuova: è un tipo spedito, e il conflitto è su **chi la porta**

Il mandato §9 introduce dodici anchor — sei `Edge_X` e sei `Corner_X_Y` — più un `CENTER` che dichiara
*«posizione logica normale nella cella, ma non un Cover Anchor»*.

**Sono tredici punti, e il repository ne ha esattamente tredici, con lo stesso significato geometrico.**
`ERTAnchorKind` (`RTGeometryGrammar.h:229`) è chiuso da `D-288` **oggi**, tramite `GEO-8`:

| Mandato | `ERTAnchorKind` | Confine di settore |
|---|---|---|
| `Edge_NE` … `Edge_NW` (6) | `EdgeMid`, `Index 0..5` | `2 * Index + 1` |
| `Corner_NE_E` … `Corner_NW_NE` (6) | `Vertex`, `Index 0..5` | `2 * Index` |
| `CENTER` (non-anchor) | `Center`, `Index` sempre `0` | — |

E il commento del tipo previene per iscritto proprio l'errore che si sarebbe fatto qui:

> ⚠️ **Non è un alfabeto nuovo**: sono il centro più i dodici confini di settore che
> `URTHexOccupancyLibrary::SectorBoundaryPoints` già restituisce, che alternano vertice e punto medio di lato
> ogni `30` gradi. Questo enum dà loro un **NOME**; non aggiunge punti, e non ne toglie.

✅ **Quindi il §9 non viola il divieto di `D-304`** — *«né otto direzioni né un terzo concetto di settore»* —
e nemmeno il §10 va scritto: *«derivare dinamicamente invece di hardcodare le 64 maschere»* è già la forma
del modello, che enumera le regioni percorrendo un anello di bit (`spec-cover-placement-intra-hex.md` §3, §11).

🔴 **Il conflitto è un altro, ed è più preciso.** Oggi `FRTAnchorRef` nomina un punto della **geometria
d'authoring** — dove cade l'estremo di un segmento — e il suo contratto è esplicito:

> ⛔ **Non è serializzato e non entra in `ComputeHash`**: è un derivato di calcolo, come `FRTOccupancyPolyline`.
> Il formato dell'asset non cambia per averlo.

Il mandato ne fa lo **stato di un'unità**: dove sta, cosa usa come riparo, e da lì l'Active Cover e
l'Exposure. Ma `COV-4`, chiusa da `D-302` **lo stesso giorno**, ha deciso l'opposto per la scelta di
copertura:

> ✅ **Sì: entra in `FRTUnitStateDigest` e nell'hash di stato**, perché due stati che differiscono **solo**
> per la copertura scelta possono risolvere in modo diverso.

∴ **Se l'anchor dell'unità è la sua scelta di copertura, deve essere serializzato — e `FRTAnchorRef`
dichiara di non esserlo.** Le due frasi non possono valere insieme sullo stesso tipo. Le uscite sono tre —
un secondo tipo per lo stato d'unità, l'estensione del contratto di `FRTAnchorRef`, o l'aggancio della
scelta a `FRTCoverSourceId` (che è già intero, stabile e destinato all'hash) — e **la scelta non si fa
dentro un referto**: è `COV-9`.

⚠️ **Un ulteriore avvertimento del canone, che il mandato non nomina.** `D-288` chiude anche la nota:
`COV-2` chiedeva **chi produce** un'ancora di copertura, *«non come si nomina un punto della reticola»*. Il
mandato risponde a una terza domanda ancora — *cosa fa l'ancora scelta* — e questa è genuinamente senza
owner.

⚠️ **Il §11 invece non è in conflitto: è già vero, al contrario.** Il mandato dice che l'unità non sfrutta
tutta la cover della cella ma solo quella dell'anchor. La spec §8 dice la faccia complementare — scegliere
una sorgente **non spegne** le altre per LOS e proiettili — e
`CoverPlacement.SelectingOneSourceDoesNotSuppressTheOthers` la pinna. Sono i due lati della stessa regola, e
il mandato ne aggiunge il lato mancante: *quale mitigazione* riceve chi ha scelto. Va scritto **dentro**
`spec-cover-placement-intra-hex.md`, non accanto. È `COV-10`.

### 4.3 🔴 `Exposed` è un tag di combattimento, non un livello di visibilità

Il mandato §12 propone la terna `Hidden · Partial · Exposed` come esito di una query geometrica.

**`Exposed` è già preso, due volte:**

| Occorrenza | Significato attuale |
|---|---|
| `TAG_Status_Exposed` (`RTGameplayTags.h:9`) | *«scoperta: +5 al PRIMO danno diretto, scade nel Cleanup»* — è il prezzo che paga `Action.Sprint` |
| `FRTExposedEdge` (`RTVisibilityBorder.h:21`) | il **bordo** della regione visibile, `{Cella, Direzione}` |

Adottare `Exposed` come terzo livello di una scala di visibilità metterebbe **tre significati** sullo stesso
token, di cui due già serializzati e uno già letto dal resolver di combattimento
(`RTCombatResolver.h:78`). È il difetto che `D-291` ha respinto per `ResolutionLayer` — sei nomi su dieci che
duplicavano vocabolari esistenti — e la ragione non cambia perché il dominio è un altro.

⚠️ **La terna in sé non è in conflitto: lo è il suo terzo nome.** `Hidden` e `Partial` sono liberi.
Il vocabolario di contatto esistente — `Nascosto` · `ContattoIncerto` · `Rilevato` più `UltimoContatto`
([`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) §3) — vive su un **altro
asse**: è *percezione*, cioè l'esito a valle. La geometria a monte non ha oggi nessun nome, ed è il delta
vero del §12. È `PER-5`.

---

## 5. I cinque delta reali

### 5.1 🆕 La regola più importante del mandato è vera e non è scritta da nessuna parte

`MaxGraphTransitionsPerUnitPerMicroStep = 1` è **implementato**:

```cpp
// RTHexSimLibrary.cpp:654
Target[i] = Done[i] ? Pos[i] : Paths[i][Prog[i] + 1];
```

Un solo passo di `Paths` per micro-step, e `Paths` è prodotto dal pathfinder **sul grafo tattico**
(`URTHexMapAsset::Transitions`, `HexSim.ReachableUsesTransitions`), non dall'adiacenza esagonale. ✅ **Quindi
la precisazione semantica che il mandato chiede — *«una transizione/arco del Tactical Graph», così da valere
per rampe, scale, ponti, tunnel, porte e multilivello* — è già soddisfatta per costruzione**, perché il passo
è un arco del grafo e `FRTCellId::operator==` confronta anche il `Layer` (`RTCellId.h:47`).

⛔ **E nessun documento la dichiara.** `git grep` per *«una cella per micro-step»*, *«una transizione»*,
`MaxGraphTransitions` sugli owner gameplay dà **zero** enunciati normativi. La matrice di
`spec-tassonomia-movimento.md` §2 ha una riga *micro-step: sì/policy/no/sì*, che dice **se** una famiglia
genera micro-step, non **quanti archi** ne attraversa uno.

🔑 **È il caso opposto a quello di `ADR-0008`**: là una tabella elencava undici test attesi come se
esistessero; qui una regola esiste, è pinnata di fatto da `Movement.StepperMatchesBatchResolver`, e nessuno
la può citare. Un invariante non dichiarato è un invariante che la prossima ottimizzazione può rimuovere
senza che nulla protesti — ed è esattamente ciò che il §5 del mandato teme quando vieta a Sprint di fare
`A → B → C`.

∴ **È l'unico punto in cui questo referto raccomanda una scrittura normativa immediata**, e va in
`spec-tassonomia-movimento.md`, che possiede già la matrice delle famiglie.

### 5.2 🆕 `MOV-3` — FAST MicroSteps

Correttamente marcato `NON LOCKED` dal mandato. Non ha owner, e **non va implementato** finché i playtest non
lo giustificano. Registrato per non essere riscoperto come omissione.

⚠️ **Ha una dipendenza che il mandato non nomina.** Lo Sprint ha una **migrazione aperta** — oggi risolve in
macro-fase `20` (`FastMovement`, con Dash) mentre il catalogo lo dichiara profilo `Move` post-Blast, ed è la
divergenza documentata in `RT_ActionCatalog_v0.1.md` §2.1 con owner
[#199](https://github.com/DegrassiAaron/refactor-tactics-main/issues/199). Un modello a cadenza doppia
deciso **prima** di quella migrazione la deciderebbe per inerzia.

### 5.3 🆕 `PER-5` — l'exposure geometrica osservatore-relativa

Il delta è il **livello a monte della percezione**: oggi `URTHexVisionLibrary` risponde *vede / non vede*
cella-a-cella, e non esiste un grado intermedio derivato dalla geometria intra-cella.

✅ **Una verifica positiva, che vale la pena registrare**: le baseline discrete del mandato §13/§14 sono
**geometricamente coerenti** con l'anello canonico `E → NE → NW → W → SW → SE` di `ERTHexDirection`. Per
`Edge_NE` i due settori adiacenti sono `E` e `NW`; per `Corner_NE_E` gli immediatamente esterni sono `SE` e
`NW`. Entrambe le tabelle del mandato dicono esattamente questo. **Non c'è geometria da ri-derivare**: c'è
da decidere se la scala entra, e con quali nomi (§4.3).

⚠️ **Dipende da [#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830)**, che è
l'implementazione di `D-269`/`D-270` — LOS e proiettili che leggono la geometria intra-cella. Senza quel
consumatore l'exposure geometrica non ha da dove leggere.

### 5.4 🆕 `OW-6` — `RiseToFire`

Zero occorrenze fuori da `docs/research/`. Il mandato ne dà una forma precisa e **compatibile** con il
canone: §19 vieta esplicitamente di modificare l'Overwatch standard, che è la stessa disciplina di
`ADR-0004`.

🔑 **Il punto architetturale del §18 sopravvive anche senza la meccanica**: *«ritornare Hidden dopo Recover
non cancella ciò che il nemico ha appena osservato»*. Il repository lo rispetta già —
`FRTLastKnownContact` persiste un turno e `FRTKnowledgeVerdict` congela il diritto di lettura **quando il
fatto accade** (`D-223`, default *fail-closed*). ✅ La clausola non chiede un sistema nuovo: chiede che
`RiseToFire`, quando arriverà, usi quello.

### 5.5 🆕 `COV-9` / `COV-10` — l'anchor come stato d'unità, e la mitigazione che ne segue

`COV-9`: l'anchor scelto da un'unità è `FRTAnchorRef` — che per contratto **non** è serializzato — o un
secondo tipo, o un aggancio a `FRTCoverSourceId`? La domanda nasce perché `COV-4` ha appena deciso che la
scelta di copertura **entra** nell'hash (§4.2).

`COV-10`: quale mitigazione riceve chi ha scelto un'opzione, dato che la geometria **non** scelta continua ad
agire su LOS e proiettili? È il lato mancante del §11 — la spec §8 scrive l'altro.

🔑 **Nessuna delle due chiede di ridecidere `COV-2`.** Quella domanda era *chi produce* un'ancora di
copertura, e `D-302` ha risposto «ibrido». Queste due chiedono *cosa fa* l'ancora scelta, ed è la terza
domanda della serie — quella che né `GEO-5` né `COV-2` toccano, e lo dichiarano entrambe.

---

## 6. I quattordici test chiesti, contro quelli che esistono

Misurato per nome su `Source/RefactorTactics/Tests/`. **Questa tabella distingue i test presenti da quelli
attesi** — è la correzione che `ADR-0008` §Verifica non fa, e che ha già indotto in errore `D-295`.

| Test chiesto dal mandato | Copertura reale |
|---|---|
| `TwoUnits_SameDestination_SameMicroStep_AllBlocked` | ✅ `HexSim.ResolveContestedDestination` + `HexSim.ResolvePriorityTieStillContested` |
| `EarlierArrival_OccupiesCell_LaterArrivalBlocked` | ⚠️ `HexSim.ResolveBlockedByStationary` copre l'occupante **fermo**; l'arrivo anticipato è vero per costruzione e **non pinnato per nome** |
| `BlockedPath_DoesNotAutoReroute` | ⛔ **assente.** `HexSim.PathAvoidsOccupiedCell` è l'aggiramento **in pianificazione**, cioè il lato opposto |
| `OneTransitionMax_PerMicroStep` | ⛔ **assente per nome.** Vero per costruzione (§5.1); `Movement.StepperMatchesBatchResolver` lo esercita di fatto |
| `IterationOrder_DoesNotChangeOutcome` | ✅ **quattro** — `HexSim.ResolveOrderIndependent` · `Actions.Collisions.NoPlayerIdBias` · `Movement.StepperIsDeterministicUnderPermutation` · `HexSim.MoveLogPermutationInvariant` |
| `Replay_SameSnapshotRulesSeed_SameMovementResult` | ✅ `HexSim.ReplayDivergenceZero` · `Simulation.ChecksumStableAcrossPermutations` · `Simulation.GoldenCorpusMatches` |
| `EdgeAnchor_RequiresMatchingCoverEdge` | ⛔ **non scrivibile oggi.** `ERTAnchorKind::EdgeMid` esiste, ma nessuna unità porta un anchor e nessuna funzione ne valida la copertura — `COV-9` |
| `CornerAnchor_RequiresBothAdjacentCoverEdges` | ⛔ stessa ragione, con `ERTAnchorKind::Vertex` |
| `ActiveCover_UsesAnchorNotEntireCellMask` | ⚠️ `CoverPlacement.SelectingOneSourceDoesNotSuppressTheOthers` asserisce la **faccia complementare**, non questa — `COV-10` |
| `Exposure_IsObserverRelative` | ⚠️ `Vision.TeamKnowledgeIsUnion` · `Vision.AllySpottingExtendsTargeting` lo provano della **conoscenza**; della geometria no — `PER-5` |
| `HiddenTarget_DoesNotLeakCurrentPosition` | ✅ **quattro** — `Vision.PlaybackNonMostraLaPartenzaNonOsservata` · `Vision.PlaybackTroncaAlTrattoOsservato` · `Vision.CannotTargetUnknown` · `UI.NoEnemyIntentExposed` |
| `LastKnown_DoesNotTrackHiddenMovement` | ✅ `Vision.PlaybackTroncaAlTrattoOsservato` + `Vision.LastContactExpiresAfterOneTurn` |
| `CellAoE_CanHitHiddenUnitWithoutPreviewLeak` | ⚠️ `Vision.UncertainTargetsCellNotUnit` copre il **bersaglio-cella**; che l'AoE colpisca davvero l'unità nascosta senza leak di preview **non è pinnato** |
| `DynamicCoverChange_RecomputesExposure` | ⚠️ `Cover.Destruction.ReopensLOS` · `BumpsRevisionAndHash` · `Cover.AddCover.RemovedStopsProtecting` coprono geometria e LOS; la rivalutazione dell'**Active Cover** dipende da `COV-8`/`COV-9` |

**Conteggio**: 4 ✅ · 6 ⚠️ · **4 ⛔**, di cui **2 non scrivibili** finché `COV-9` è aperta.

🔑 **I due test scrivibili subito sono `BlockedPath_DoesNotAutoReroute` e `OneTransitionMax_PerMicroStep`**,
e non a caso: pinnano le due regole che il repository applica senza dichiarare. Sono la coda naturale di
§5.1.

---

## 7. Cosa è stato fatto, e cosa no

**Fatto** — tre file di canone toccati, **una** issue creata, quattro toccate su GitHub:

| Sede | Modifica |
|---|---|
| [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | Nuova sezione con **sei** ID: `MOV-3`, `MOV-4`, `COV-9`, `COV-10`, `PER-5`, `OW-6` |
| [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | **`D-305`** — ratifica ciò che non confligge, instrada il resto |
| [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) | **§2.0 nuova** — dichiarata la regola di §5.1, **l'unica scrittura normativa** di questo pass |

| Issue | Cosa |
|---|---|
| [#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000) **creata** | I due test scrivibili — `OneTransitionMax_PerMicroStep` e `BlockedPath_DoesNotAutoReroute`. `v0.1` · `P2` · ms *v0.1 · Mondo giocabile* |
| [#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833) | Corpo aggiornato: `COV-9` e `COV-10` aggiunte al registro, DoD da **6/8** a **6/10**. ⚠️ L'istruttoria esistente **non è stata riscritta** — la issue lo vieta esplicitamente |
| [#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830) | Commento: `PER-5` dipende da questa issue, e il nome `Exposed` non è disponibile |
| [#1829](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1829) | Commento: il gate decisionale si è riaperto in parte — `COV-9` tocca il campo che l'intento deve far viaggiare |
| [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152) | Commento: `OW-6` registrata sotto E14, **senza** scavalcare la prossima azione già raccomandata (#1933) |

**Non fatto**, e perché:

| Non fatto | Perché |
|---|---|
| Creare una «issue principale» Movement/Cover/Visibility | Il perimetro ne ha **quattro** owner vivi e distinti — #1833 (decisioni cover), #1830 (LOS intra-cella), #1466 (conoscenza parziale), #152 (E14 reazioni). Una issue-ombrello sopra owner sani è la duplicazione che `AGENTS.md` vieta. **#2000 non è un'ombrello**: ha uno scope di due test e non rivendica il perimetro |
| Creare issue per `MOV-3`, `MOV-4`, `PER-5`, `OW-6` | Sono **decisioni**, non lavoro: nessun test può rispondervi, ed è il criterio per cui `MOV-1`/`MOV-2` vissero in `OPEN_DECISIONS.md` senza owner dedicato fino a `D-118`/`D-119`. `COV-9`/`COV-10` fanno eccezione solo perché #1833 **è** il registro delle `COV-*` |
| Scrivere gli anchor come canone | `COV-2` è stata decisa **oggi** e in senso diverso (§4.2). Riscriverla sei ore dopo dentro un referto sarebbe deciderla per inerzia |
| Adottare `Hidden/Partial/Exposed` | `Exposed` è occupato da un tag serializzato e da un tipo di percezione (§4.3) |
| Togliere `BlockedByPriority` | Campo serializzato in formato v7; la rimozione è una migrazione con rigenerazione del corpus golden (§4.1) |
| Implementare `RiseToFire` o i FAST MicroStep | Il mandato stesso li marca da playtestare, e `MOV-3`/`OW-6` li registrano |
| Scrivere i dodici test mancanti | Due non sono scrivibili (`COV-9` aperta); i due scrivibili sono la prossima azione, non questo pass documentale |
| Toccare `Source/` | Il mandato lo esclude esplicitamente |

---

## 8. Verifiche eseguite

| Verifica | Esito |
|---|---|
| Baseline git, `origin/main`, working tree | ✅ misurata — albero pulito, `3b3aafc4` |
| Owner docs (Decision Log, `OPEN_DECISIONS`, spec cover/tassonomia/sequenza, brief conoscenza parziale, ADR-0004/0008/0009) | ✅ letti |
| Esistenza dei tipi e dei test citati | ✅ `grep` per nome su `Source/` |
| Issue/PR/milestone GitHub | ✅ interrogati lato server con `gh` |
| Unicità di `D-305` | ✅ registro a `HEAD` + diff di ogni PR aperta + ogni branch remoto |
| **Build** | ⛔ **NOT RUN** |
| **Automation suite** | ⛔ **NOT RUN** |
| **Scenario harness** | ⛔ **NOT RUN** |
| **PIE** | ⛔ **NOT RUN** |
| **Packaged** | ⛔ **NOT RUN** |

⚠️ **Nessuna suite è stata eseguita, e nessuna serviva**: questo pass non tocca `Source/`. Ma vale la nota di
`rt-suite`: scrivere in `docs/` **durante** una run altrui la rende non registrabile, perché il digest copre
l'albero e non i soli sorgenti.

---

## 9. La prossima azione

> **Scrivere i due test che pinnano le due regole non dichiarate** — `OneTransitionMax_PerMicroStep` e
> `BlockedPath_DoesNotAutoReroute` — che sono [#2000](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2000).
> La riga normativa è già scritta: `spec-tassonomia-movimento.md` **§2.0**.

Tre ragioni misurate. ① Sono le **uniche due** delle quattordici richieste che siano sia assenti sia
scrivibili oggi (§6). ② Pinnano regole che il codice **già applica**, quindi non cambiano nessun esito
serializzato: nessuna rigenerazione del corpus golden, nessuna interferenza con `#1800`. ③ Chiudono il delta
§5.1, che è l'unico punto in cui questa seduta d'autore trova qualcosa che il repository fa senza sapere di
averlo deciso.

⏭️ **Dopo, e in quest'ordine**: `COV-9` — perché `COV-10` e i due test degli anchor la aspettano — poi
`PER-5`, che a sua volta aspetta [#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830).
`MOV-3` e `OW-6` non hanno innesco: restano registrate finché un playtest non le chiama.
