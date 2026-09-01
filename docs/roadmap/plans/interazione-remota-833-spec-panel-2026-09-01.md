# Il grafo di interazione ha un dato e non un chiamante (`#833`) — spec panel sulla issue come specifica

> `CURRENT` · **Stato**: revisione chiusa, definizione `DNNN` consegnata alla issue ·
> **Data**: 2026-09-01
> **HEAD della revisione**: `6ba00191` (= `origin/main` al 2026-09-01, 20:13)
> **Oggetto**: la issue [`#833`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/833) letta
> **come specifica di implementazione** per la fetta che le resta aperta.
> **Panel**: Cockburn (lead) · Wiegers · Fowler · Nygard · Crispin
> **Modo**: critique · **Focus**: requirements, architecture, testing

---

## 1. Il verdetto in una riga

La issue è **onesta e ben mantenuta** — sette correzioni datate nel corpo, quattro decisioni d'autore
iscritte, e una nota del 2026-08-27 che dichiara con precisione cosa resta. Ma la fetta rimasta ha **una
domanda architetturale non risposta** e **una collocazione di release che si contraddice**:

| | Rilievo | Chi | Gravità |
|---|---|---|---|
| **R2** | **cinque owner, quattro dicono `v0.2`**: la fetta rimasta è bloccata da una decisione d'autore aperta da quindici giorni, e implementarla la prenderebbe per inerzia | Cockburn | 🛑 **bloccante** |
| **R1** | l'intento parla di **bordi**, lo Scope chiede che parli di **`SourceId`**: la traduzione non ha un posto, e la risoluzione inversa non esiste | Fowler | 🔴 critico |
| **R3** | `ApplyInteraction` produce rifiuti come **`FString`**, il DoD li vuole nel **TurnLog**: due vocabolari | Nygard | 🟡 |
| **R4** | il DoD ha 13 voci di cui **due già soddisfatte** e due ritratte: chi lo legge non sa cosa manca | Wiegers | 🟡 |
| **R5** | manca il test dell'**applicazione**, e il DoD lo dichiara da sé | Crispin | 🟡 |

---

## 2. Ciò che è già fatto, misurato su `6ba00191`

Va scritto per primo, perché tre quarti di questa issue **sono in `main`** e il DoD non lo mostra:

| | Stato |
|---|---|
| `ERTActionEffect::SetDoorState` dichiarato da un'azione | ✅ `Action.Interact`, verso `Open` e solo `Open` (`D-151`) |
| `ERTStructureOp` ha un valore per l'interazione | ✅ `None · CreateCover · MoveCover · SetDoorState` |
| il grafo `Source → Target` come **dato** | ✅ **23** test `RefactorTactics.InteractionGraph.*` verdi |
| `URTHexDoorLibrary::ApplyInteraction` | ✅ implementata — risolve, applica, raccoglie i rifiuti, **una sola** revisione |
| la catena **locale** (adiacente) | ✅ end-to-end: `Structures.Door.InteractFromKitOpensDoor` (C++) e `Spec.Map.InteractOpensDoor` (scenario, due turni) |
| le quattro decisioni `D-148/149/150/151` e `INT-7` | ✅ **iscritte** nel Decision Log e in `OPEN_DECISIONS.md` — il debito dichiarato nei commenti è saldato |

∴ **La fetta aperta è una sola riga di verità**: `grep -rn "ApplyInteraction" Source/` dà la definizione, la
dichiarazione e un commento. **Nessun chiamante di produzione.** In partita l'`Interact` passa da
`RTHexCombatLibrary` su `FirstDoorEdge` e apre la porta **adiacente**; l'apertura *remota* non esiste.

---

## 3. R1 — l'intento parla di bordi, lo Scope chiede `SourceId`

**FOWLER**: il percorso runtime esistente è, per intero:

```text
FRTHexCombatIntent{ bChangesDoor, DoorState, bHasDeclaredDoorEdge, DeclaredDoorEdge }
   └─ RTHexCombatLibrary  ──FirstDoorEdge(Attacker.Cell, AimCell)──▶  FRTDoorOp{ From, To, State }
        └─ RTTurnManager_Blast  ──ApplyDoorOps──▶  URTHexDoorLibrary::SetDoorState
```

Ogni anello parla di **bordi**. Lo Scope della issue dice invece:

> l'intento dichiara `Interact <SourceId>`; è il runtime a risolvere il mapping canonico. **Il client non
> sceglie bersagli interni**

E `ApplyInteraction(Map, SourceId, State, ActorId, OutRefusals)` prende infatti un **`FName`**, non un bordo.

🔴 **Fra i due c'è un buco che nessuna riga della issue nomina**: il giocatore punta un **bordo**
(`ARTPlayerController::HandleTargetEdge` → `PlannedCoverEdge`), e da un bordo a uno `StableId` **non esiste
una funzione**. `URTStructureIdentityLibrary` risolve solo nella direzione opposta — `FindDoorEdges(Map,
StableId)`, `ResolveInteractionTargets(Map, SourceId)` — perché finora nessuno ha avuto bisogno dell'inversa.

Le due uscite, e non sono equivalenti:

| | **(a)** il PlayerController traduce | **(b)** il runtime traduce |
|---|---|---|
| chi legge lo `StableId` | il client | l'autorità |
| l'intento porta | `SourceId` (`FName`) | il bordo puntato, come oggi |
| privacy | il client conosce il **nome** della sorgente | il client non conosce nemmeno quello |
| costo | un campo nuovo sull'intento + la funzione inversa | la sola funzione inversa, chiamata a valle |
| coerenza con lo Scope | ✅ *«l'intento dichiara Interact `<SourceId>`»* | ⚠️ contraddice la lettera, rispetta il principio |

⚠️ **Lo Scope sceglie (a) alla lettera, ma la ragione che porta è di (b)**: *«il client non sceglie bersagli
interni»*. In **(b)** il client non sceglie nemmeno la sorgente per nome — sceglie un bordo che vede, che è
strettamente meno informazione. Il panel raccomanda **(b)** e chiede che la scelta sia scritta, perché la
lettera dello Scope andrà corretta in un verso o nell'altro.

🔑 In entrambe serve **la funzione inversa**, ed è la stessa: dato un bordo, lo `StableId` della porta che ci
sta sopra. È piccola e appartiene a `URTStructureIdentityLibrary`, che è già l'autorità dell'identità.

---

## 4. R2 — `v0.2` nel corpo, `v0.1` nella milestone

**COCKBURN**: l'intestazione della issue dice **`Release: v0.2`** e la tabella delle dipendenze la conferma:

```text
v0.2  qui        il graph è un DATO            cardinalità, ordine deterministico, validator
v0.3  E27 #327   la relazione ha un PUBBLICO   Known/Unknown per squadra
v0.5  E40 #773   la privacy è VERIFICABILE     canary di leak
```

La milestone assegnata è invece **`v0.1 · Mondo giocabile`**, per la PR #1139 — *«la issue entra nella v0.1
su tre owner»*.

E la riga «qui» della tabella descrive **il dato**, che è fatto. Il **collegamento a runtime** — la fetta
davvero aperta — non compare in nessuna delle tre righe.

⚠️ **La domanda non è burocratica**, perché il corpo motiva il rinvio così:

> il controllo **remoto** sorgente → bersaglio, che richiede la privacy dei collegamenti (§8) e quindi la rete

**Il panel misura che la premessa non regge per la v0.1**, e lo scrive perché è la ragione per cui la fetta
sembrava bloccata: la privacy serve quando esiste **un client a cui tacere**. La v0.1 è *2v2 offline vs bot*
— non c'è. È la stessa osservazione che il corpo fa già per il verso opposto, sul test:

> uno scenario `NoHiddenRelationLeak` scritto in v0.2 **non fallirebbe mai**, perché passerebbe per assenza
> di rete, non per correttezza

∴ Se un leak non è *misurabile* senza rete, allora non è nemmeno *possibile*: l'apertura remota in v0.1
sarebbe implementabile **senza** decidere la privacy, e la decisione resterebbe a `E27`.

### 🔴 CORREZIONE (2026-09-01, stesso giorno): la contraddizione è **una sola**, non cinque

> ⛔ **Questa sezione affermava «cinque owner, quattro dicono `v0.2`» ed era falsa in tre punti su cinque.**
> Le righe venivano dalla PR [#1139](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1139) del
> 2026-08-17 e non erano state rimisurate contro lo stato attuale — l'errore che
> `checkout-stantio-produce-fatti-falsi` descrive: un'affermazione datata continua a leggersi come una misura.

Rimisurato su `ab5b07e0`:

| Owner | Dice davvero | Verdetto |
|---|---|---|
| GitHub milestone · epic `#324` | `v0.1 · Mondo giocabile` | ✅ allineato |
| Feature Registry | `v0.1` | ✅ allineato |
| `roadmap-v0.1.md` §2.1 e §5 | `E23` in v0.1, *«anticipata dalla v0.2 il 2026-08-17»* | ✅ allineato |
| `roadmap-post-v0.1.md:277` | *«⛔ **E23 NON È PIÙ DI QUESTA RELEASE**»* | ✅ allineato |
| `technical/tooling/scenario-map.md:632` | *«`RT-FEAT-MAP-INTERACTION-GRAPH` \| v0.1 · E23 · CP 23.4 (`#833`)»* | ✅ allineato |
| corpo di questa issue | `Release: v0.2` | ❌ **stantio** |
| `spec-interazioni-mappa-cp101.md` §11 | il controllo remoto è **fuori scope** | ❌ **l'unico ostacolo vero** |

**[`D-160`](../../decisions/RT_PDR_00_Decision_Log.md) del 2026-08-17 ha già preso la decisione** che #1139
aveva dichiarato aperta poche ore prima, e con lo stesso argomento: *«E23 è ANTICIPATA dalla v0.2 alla v0.1,
e per metà è una presa d'atto»* — elencando gli stessi cinque owner e riallineandoli.

∴ Resta **un solo** documento a dire il contrario, ed è `CP101 §11`. La sua motivazione è la privacy, e la
privacy serve quando c'è un client a cui tacere: la v0.1 è *2v2 offline vs bot*. È lo stesso argomento che
il corpo della issue porta già nel verso opposto — *«uno scenario `NoHiddenRelationLeak` scritto in v0.2 non
fallirebbe mai, perché passerebbe per assenza di rete»*.

### 🎁 E `scenario-map.md` non è solo allineato: **dichiara già gli scenari attesi, con i nomi**

```text
Spec.Map.Interaction.SwitchOpensDoor · …SwitchControlsMultipleDoors · …OpenFailsDependentMoveBlocks
        RT-FEAT-MAP-INTERACTION-GRAPH  |  v0.1 · E23 · CP 23.4 (#833)
```

Sono i tre casi del `DoD` — `1→1`, `1→N`, e il bersaglio che rifiuta — già battezzati da un owner. La
specifica eseguibile di questa issue esisteva e nessuna delle sette riletture l'aveva citata.

## 5. R3 — i rifiuti sono stringhe, il TurnLog vuole reason code

**NYGARD**: `ApplyInteraction` riporta gli esiti così:

```cpp
OutRefusals->Add(FString::Printf(
    TEXT("Refused: la sorgente '%s' dichiara un binding che non risolve nessuna porta"), ...));
```

Il DoD chiede invece:

> l'esito per-bersaglio va nel **TurnLog** o il giocatore preme senza sapere cosa è successo

Sono due vocabolari. Il TurnLog di questo repository non porta stringhe: porta **enum** — `ERTLogCategory`,
`ERTEnvironmentOutcome` con `DoorOpened`/`DoorClosed` e `SrcCell`/`TgtCell`. E la disciplina è scritta per
esteso in `ERTLineOfSightBlock`: *«un reason code, non una stringa, e la ragione è verificabile invece che
stilistica»*, perché la verifica di mutazione **non è asseribile** se due regole producono messaggi simili.

∴ Portare le `FString` nel TurnLog non è una traduzione: è la scelta che quella disciplina ha già scartato.
Le stringhe restano legittime per il **validator d'asset**, che gira a mano ed è per un umano; l'esito
per-bersaglio *in partita* vuole un enum.

⚠️ **Il panel non chiede di riscrivere `OutRefusals`**: chiede che la voce del DoD dica **quale dei due**
canali si sta consegnando, perché oggi la stessa riga può essere letta come «già fatto» (i rifiuti
esistono) o «tutto da fare» (nel TurnLog non c'è niente).

---

## 6. R4 — il DoD non dice più cosa manca

**WIEGERS**: tredici voci, di cui due `[x]` **ritrattate e non svolte**, due che il panel ha appena
misurato **soddisfatte** e non spuntate, e una che porta il proprio STOP dentro (poi rientrato). Chi apre la
issue oggi non ricava in quanto tempo la chiude.

Non è un difetto di rigore — è il contrario: ogni correzione è datata e argomentata, che è il motivo per cui
la misura è stata possibile. Ma la funzione «elenco di cose da fare» l'ha persa.

∴ La definizione `DNNN` che questo run consegna **non sostituisce** il DoD: lo affianca dichiarando lo stato
di ciascuna voce, e lascia la storia dov'è.

---

## 7. R5 — il test che manca è nominato dalla issue stessa

**CRISPIN**: il DoD dichiara la propria lacuna:

> ⚠️ **Resta da fare** il test dell'**applicazione**, gemello di `ResolutionIgnoresDoorState`: la risoluzione
> dice *chi* è comandato, l'applicazione *cosa è cambiato*

I 23 test `InteractionGraph.*` provano la **risoluzione** e la validazione. Nessuno prova che, applicando,
una porta remota si apra davvero e che l'esito arrivi a chi ha premuto.

E c'è un secondo test che il DoD chiede con una forma precisa, che va rispettata:

> Ripetere la stessa risoluzione **NON basta** […] nello stesso processo anche l'iterazione di una `TMap` è
> ripetibile. Serve una delle due: l'ordine atteso calcolato **indipendentemente**, oppure si **perturba
> l'ordine d'inserimento** dei binding e si pretende lo stesso esito.

⚠️ `InteractionGraph.BindingInsertionOrderDoesNotChangeHash` esiste già e fa la seconda — **ma sull'hash**,
non sull'ordine di applicazione. Il gemello per l'applicazione non c'è.

---

## 8. Il DoD che il panel consegna

Le voci della issue restano. Il panel dichiara quali sono chiuse, aggiunge tre voci e ne toglie una dal
perimetro:

| | Voce | Da |
|---|---|---|
| **+1** | la risoluzione **bordo → `StableId`** esiste in `URTStructureIdentityLibrary`, ed è l'unica sede | R1 |
| **+2** | è scritto **dove** avviene la traduzione e **cosa** porta l'intento, con la ragione | R1 |
| **+3** | l'esito per-bersaglio è un **reason code**, non una stringa, e il DoD dice su quale canale | R3 |
| **—** | la privacy dei collegamenti | resta a `E27` (`#327`): senza client non è né violabile né misurabile |

---

## 9. Chiusura

Questa issue non soffre di ciò di cui soffrono le issue vecchie: è stata riletta sette volte e ogni
correzione porta la data e la misura. Ciò che le manca è **il ponte fra due vocabolari** — un intento che
parla di bordi e una funzione che parla di nomi — e nessuna delle sette riletture lo ha nominato, perché
ciascuna guardava il proprio lato.
