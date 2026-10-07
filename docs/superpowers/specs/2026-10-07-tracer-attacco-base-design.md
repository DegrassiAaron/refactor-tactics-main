# Il colpo che parte e arriva — il tracer degli attacchi base

> **Statuto**: design **accettato in sessione** il 2026-10-07, **implementato** con la PR [#3552](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3552). Il verdetto a schermo, `PIE-V01-TRACER`, è ancora ⏳. Owner del lavoro:
> [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454), righe `Single` e `Line` della
> grammatica *Basic Combat Cues* e criterio *«il momento dell'arrivo è distinto dal momento della partenza»*.
> Nessuna issue nuova: la grammatica ha già un owner aperto, e questa spec ne consegna una fetta.
> **Piano**: [`2026-10-07-tracer-attacco-base.md`](../plans/2026-10-07-tracer-attacco-base.md).
>
> **Provenienza**: `/sc:spec-panel` del 2026-10-07 sulla richiesta d'autore *«partiamo dagli attacchi base
> dei personaggi. lanciano qualcosa? un proiettile? si vede nel gameplay?»*, poi due decisioni d'autore (§0.2).
>
> **Revisione indipendente** del 2026-10-07 (agente revisore, letta su `0407a4901`): i conti di tempo reggono;
> accolti i findings su produttore, predicato di conoscenza, confine di rete, cursore unico, contatori, e le
> citazioni spostate. I cambiamenti sono marcati `➕ rev.`. 🔴 Il più importante rovescia la prima stesura: il
> verdetto **non** si calcola col predicato per cella delle rotte, ma con quello dei fatti puntuali (§0.3, P1).
>
> **Stato misurato**: 2026-10-07, `main` = `0407a4901` (`Source/` identico a `0157fff27`, su cui il panel ha
> letto). Ogni `file:riga` è stato letto su quel commit; chi la rilegge più tardi la **rimisura**. Nessun
> totale volatile: dove serve una misura c'è il comando che la produce.

---

## 0. Richiesta, risposta e decisioni

### 0.1 La risposta alla domanda, misurata

**Oggi un attacco base non lancia niente — né in simulazione né a schermo.**

| Attacco base | Cosa dice il gioco (`RTActionDescriptions.cpp`) | `Shape` | Payload a catalogo |
|---|---|---|---|
| `Hero.Aevik.ArcPulse` | «Una scarica elettrica a distanza su un bersaglio.» | `Single` | 22 · r4 |
| `Hero.Muiren.PressureJet` | «Un getto d'acqua in linea: bagna il bersaglio e lo spinge indietro.» | `Line` | 16 · r5 · `Wet` · `Push 1` |
| `Hero.Branth.ImpactShot` | «Un colpo cinetico che rallenta il bersaglio.» | `Single` | 8 · r3 · `Slow` |
| `Hero.Ivrin.PulseShot` | «Un colpo a impulsi a distanza su un bersaglio.» | `Single` | 21 · r4 |

- **Simulazione.** Nessun oggetto proiettile, nessun tempo di volo, nessuna intercettazione in volo: LOS dalla
  cella dell'attaccante (`Combat/RTHexCombatLibrary.cpp:573-576`), celle da `HexHitCells` (`:23-50`), primo
  bordo coperto che ferma il colpo (`:405-407`). Il catalogo non ha un campo traiettoria: l'asse è di
  [#2825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2825), post-v0.1.
- **Schermo.** Nel Blast, **nella stessa iterazione**, l'attaccante suona il ruolo `Attack` e il bersaglio il
  ruolo `Hit`, il numero e l'impulso della barra (`Turn/RTTurnManager.cpp:8449-8477`). Partenza e arrivo
  coincidono; fra i due non si disegna niente. La clip è `Cast`, la stessa per ogni azione dell'eroe:
  l'`ActionId` si perde a `:8462` (asse del sotto-progetto 3 del banco).

### 0.2 Decisioni d'autore (2026-10-07)

| # | Domanda | Decisione |
|---|---|---|
| V1 | Il colpo nel tempo | **Volo breve fisso**: il colpo parte col `Cast` e il numero compare **all'arrivo**. |
| V2 | Distinzione fra i quattro | **Per forma, poi per eroe**: `Single` = proiettile, `Line` = getto. La differenza per eroe è il sotto-progetto 4 del banco (tabella `ActionId → profilo`). |

### 0.3 Decisioni di contratto (dal panel e dalla revisione, non d'autore)

| # | Decisione | Perché |
|---|---|---|
| P1 | ➕ rev. **Fail-closed sulla conoscenza, col predicato dei fatti puntuali**: il tracer si disegna solo se la squadra di chi guarda era nel verdetto **dell'attaccante nella sua cella** e in quello **della vittima nella sua cella**, entrambi congelati con `ARTTurnManager::FreezeVerdictFor(FRTLogSubject::UnitAt(...))` nell'istante del colpo. | L'origine di un tracer è **occupazione**: il ramo dell'impronta in `BeginPlayback` è senza filtro perché le sue celle *«sono terreno, non occupazione»* (`RTTurnManager.cpp:7749-7756`), e un tracer non lo è. Un colpo è un **fatto puntuale**, e D-223 vuole **un** predicato: è quello che congela le righe di combattimento (`:266`, `:275-312`). Il predicato per cella delle rotte (`FreezeRouteCellVerdict`) esiste perché *«una rotta non è un fatto puntuale»*: usarlo qui sarebbe la «terza via» che D-223 vieta. CP 13.4 concede che l'attacco riveli *«almeno la direzione»*, non la cella, e `AttackRevealsDirection`/`ContactDirection` non esistono in `Source/`. |
| P2 | Il tracer **cavalca `Attack`**, non `AttackFootprint`. ➕ rev. La geometria la dà **`ResolveImpactOrigin`**, come terzo chiamante. | Impronte e colpi escono **intercalati** con indici non correlati (§1.2); un'unità può avere più intenti nello stesso Blast; `FRTResolvedEvent` non ha `IntentIndex`. `ResolveImpactOrigin` (`:2503-2554`) esiste *«perché i chiamanti sono DUE e devono restare d'accordo»*, e al sito di emissione la usa già la traccia del lato colpito, con la cella della vittima `HexUnits[Hit.TargetId].Cell` (`:6085-6086`). Una terza lettura dell'origine sarebbe la divergenza che `#1430`/D-199 esiste per togliere. |
| P3 | Il tempo di volo viene **dall'orologio del playback**, mai da un anim notify. | Le clip vivono in `Content/FabAsset/`, non versionato (`Unit/RTUnit.cpp:702`): senza clip niente notify, e senza notify il colpo non arriverebbe. La coordinata è quella di [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881). |
| P4 | Disegno col **line batcher del mondo**; niente Niagara, niente `.uasset`. | [D-124](../../decisions/RT_PDR_00_Decision_Log.md) e la decisione D1 del banco; [D-467](../../decisions/RT_PDR_00_Decision_Log.md) ha portato l'anteprima su `DisegnaLineaAnteprima`, che si vede anche in Shipping. Nessun asset vuol dire nessun authoring nel clone principale. |
| P5 | ➕ rev. **`FRTResolvedEvent` si dichiara `RTServerOnly`.** | Con la geometria e i verdetti di ogni squadra, l'evento porta l'informazione **completa**: filtrarlo all'arrivo non è un confine. Oggi la timeline è locale (nessuna replica di produzione) e il limite *«giocatore locale»* è dichiarato (`:7677-7684`); il marcatore lo fa **misurare** a `Privacy.ServerOnlyTypesAreNotReplicated` invece di lasciarlo scritto. Un client riceverà, il giorno che servirà, una **proiezione** — `CLAUDE.md` §7. |

### 0.4 Owner e confini

| Owner | Relazione |
|---|---|
| [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454) | **owner**: righe `Single`/`Line`, criterio partenza ≠ arrivo |
| [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) | epic Combat Feedback |
| [D-278](../../decisions/RT_PDR_00_Decision_Log.md) · #1801 | la voce `Attack` della tabella guadagna una cue; il gate non cambia |
| [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | coordinata di playback: si legge, non se ne crea una seconda |
| D-223 · D-380 · CP 13.4 | verdetto dei fatti puntuali; rivelazione della vittima a chi colpisce; «direzione» non implementata |
| D-302 punto 3 | `ResolveImpactOrigin`: l'origine dichiarata di un colpo, terzo chiamante |
| [#2825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2825) | traiettoria come asse di gameplay: **non** anticipata, il tracer è sempre retto |
| banco Ability Lab → PIE (#3532, spec `2026-10-07-banco-abilita-in-pie-design.md` sul branch `docs/banco-abilita-in-pie-spec`) | **non è una dipendenza**: aiuta a guardare, non serve a costruire. Questa spec è la prima fetta, *per forma*, del suo sotto-progetto 4 |

---

## 1. Stato di partenza

### 1.1 Il produttore

Un attacco base emette **un** `AttackFootprint` per intento (`RTTurnManager.cpp:5503-5524`) e **un** `Attack`
per vittima, il cui unico produttore è il ciclo su `Plan.Hits` (`:6254-6276`: sorgente, bersaglio, `Amount`,
`ActionId`/`BaseActionId` a `:6273-6274`, `Add` a `:6276`) — **senza** origine, forma né cella d'impatto.

Un'unità può possedere **più** intenti d'attacco nello stesso Blast: `CollectAttackIntents` ne crea uno per unità
(`Turn/RTTurnManager_Blast.cpp:601`) e `AppendChargeImpactIntents` aggiunge l'impatto di carica (`:1010-1043`).
La chiave `(SourceStableUnitId, ActionId)` non basta a ricondurre un colpo al suo intento. Ogni colpo di
`Plan.Hits` ha invece l'impronta del proprio intento (`Combat/RTHexCombatLibrary.cpp:612-647`).

### 1.2 Il ritmo del Blast

`URTPlaybackLibrary::AttacksToShow(N, t, A) = Min(N, 1 + Floor(Max(0,t) / A))`, con `A = AttackShowSeconds =
0.50` (`Turn/RTTurnManager.h:1265`, `Turn/RTPlaybackLibrary.cpp:31-45`). Il colpo *k* esce a `k·A`; la fase dura
`Max(1, Max(colpi, muri, impronte))·A` contro la spinta, con `Slack = 0` (`PhaseTime`,
`RTPlaybackLibrary.cpp:111-123`), e il budget non comprime il Blast (`RTTurnManager.cpp:8886-8901`).

Impronte, colpi a struttura e colpi leggono la **stessa** `AttacksToShow` sullo stesso `PlaybackPhaseElapsed`
(`:8430-8451`): escono **intercalati**, e l'impronta *k* e il colpo *k* possono appartenere ad azioni diverse —
le impronte sono ordinate per `IntentIndex`, i colpi per `(AttackerId, TargetId, Power, IntentIndex)`
(`RTHexCombatLibrary.cpp:653-678`).

Un solo contatore, `AttacksShown`, guida oggi sia il ciclo (`:8451-8477`) sia la rete di finalizzazione
(`:8540-8561`), e l'ultimo colpo mostrato è anche il riferimento della fermata (`:8143`).

### 1.3 Conoscenza e playback

- Il velo spegne i componenti di un'unità ignota (`SetKnownToObserver`, `Unit/RTUnit.cpp:438-446`): la clip di
  un attaccante ignoto suona su una mesh invisibile.
- Gli `Attack` **non** sono filtrati per conoscenza: in `BeginPlayback` lo è solo il `Move`
  (`RTTurnManager.cpp:7699-7714`).
- I fatti puntuali si congelano con `FreezeVerdictFor` (`:275-312`), che passa a
  `URTTeamKnowledgeLibrary::FreezeVerdict(TeamKnowledgeState, …)` con la **cella del fatto** portata dal soggetto
  (`FRTLogSubject::UnitAt`, `Turn/RTCombatLog.h:41`).
- `RevealHitTargetsToAttackers` (D-380) gira **prima** del ciclo dei colpi (`:5483`): quando il verdetto della
  vittima si congela, chi l'ha colpita la conosce già.
- `ResolvedTimeline` **non entra** in `StateHash` né nel formato di replay (`:2831-2833`).
- Chi guarda: `ViewerTeamId = ARTPlayerState::TeamIdOf(GetPlayerController(this, 0))` (`:7685`).

---

## 2. Il contratto visivo

### 2.1 Chi ha un tracer

Un evento `Attack` ha un tracer quando **tutte** valgono:

1. ➕ rev. `ActionId == Action.BasicAttack` **oppure** `BaseActionId == Action.BasicAttack` — la fetta è degli
   attacchi base, come chiesto: i profili d'eroe portano la base nel secondo campo
   (`Ability/RTHeroCatalogLibrary.cpp:168`), l'azione generica nel primo (`Ability/RTCatalogLibrary.cpp:1226`).
   Le altre azioni restano come oggi. ⚠️ È un'idoneità **provvisoria e dichiarata**: la sostituisce la tabella del
   sotto-progetto 4.
2. `Shape ∈ { Single, Line }` — `Area` e `Cone` restano senza tracer: le loro righe di grammatica (arrivo
   sull'`AimCell`, ventaglio) non sono di questa fetta.
3. ➕ rev. `HitGeometry.bResolved` — il produttore ha risolto l'origine (`ResolveImpactOrigin` ha risposto `true`)
   e la cella della vittima. ⛔ **Non** si giudica dalle celle: `FRTCellId()` è `(0,0,0)`, una cella **valida**
   (`Map/RTCellId.h:51`: `IsValid` vale `X + Y + CubeZ == 0`), quindi un estremo mancante sarebbe indistinguibile da uno vero.
4. La squadra di chi guarda è nel verdetto dell'origine **e** in quello dell'impatto (P1).

Le condizioni 1–3 decidono il **ritmo**; la 4 decide solo il **disegno**. ➕ rev. Il ritmo quindi **non dipende da
chi guarda**: entrambe le squadre vedono l'arrivo nello stesso istante. Non è *privo* di informazione — rivela che
un attaccante, anche non visto, ha usato un attacco base `Single` o `Line` — ma è informazione che il numero di
danno sul bersaglio dà già oggi.
⌫ ➕ #3549 *«Il ritmo quindi non dipende da chi guarda»* è **superato** sull'indice del colpo nella sequenza:
con l'attivazione per intento di [#3549](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3549) la
sequenza del Blast si costruisce per squadra (D6), e lo stesso colpo cade su un indice — quindi su un istante —
diverso per chi vede la sorgente e per chi no. Vale ancora per l'idoneità: le condizioni 1–3 non leggono chi
guarda. Governa la spec del momento §2.4 (`CONTRACT CONFLICT`), non questa.

### 2.2 Le due forme

Il canale non è il colore: è la **geometria**. Il colore è `URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack)`,
⛔ nessun `FColor` letterale nuovo.

| Forma | Proiettile | Cosa si vede durante il volo |
|---|---|---|
| `Single` | **proiettile** | un segmento corto che **si sposta** dall'origine alla cella d'impatto; dietro, nient'altro |
| `Line` | **getto** | un segmento che **si allunga** dall'origine fino alla cella d'impatto, ancorato all'origine |

Estremi: centro della cella (`URTHexLibrary::AxialToWorld` col contesto esagonale della mappa) più un'altezza
costante dichiarata e tarata in PIE. ⛔ Non `GetActorLocation()`: la posizione visiva di un'unità poco osservata
può essere stantia (`RTTurnManager.cpp:7699-7714`), e un estremo letto dall'attore sarebbe un dato di presentazione
che si fa passare per fatto.

### 2.3 Il tempo

Per il colpo *i* del Blast, con `F = TracerFlightSeconds` (nuova `UPROPERTY` accanto ad `AttackShowSeconds`,
default `0.25`):

```
Lancio(i)   = i · A
F_eff(i)    = idoneo(i) ? Min(F, 0.5 · A) : 0      // idoneo = condizioni 1–3 di §2.1
Arrivo(i)   = Lancio(i) + F_eff(i)
Alpha(i, t) = F_eff(i) > 0 ? Clamp((t − Lancio(i)) / F_eff(i), 0, 1) : 1
```

| Istante | Cosa succede |
|---|---|
| **Lancio** | `PlayPresentationRole(Attack)` sull'attaccante; il tracer comincia |
| **Arrivo** | `PlayPresentationRole(Hit)`, `ShowDamageToken`, `PulseHealthBar`, la riga `Colpo:` del log, `OnAttackResolved`, il confine `Next Action`, il riferimento della fermata; il tracer si spegne |

🔑 **Il Blast non si allunga.** `F_eff ≤ 0.5·A < A`, quindi `Arrivo(i) ≤ (i + ½)·A < Lancio(i+1)`, e l'ultimo arrivo
cade prima di `N·A ≤ PhaseDur`. Gli arrivi restano **monotoni** anche mescolando colpi idonei e non idonei.
⛔ `PhaseTime` non cambia firma né valore.

➕ rev. **Un cursore solo, sui battiti.** I lanci e gli arrivi formano una sola sequenza `L0, A0, L1, A1, …`, monotona
per la disuguaglianza qui sopra, e il tick la percorre con **un** cursore finché il prossimo battito è `≤ t`. Due
cicli separati — prima i lanci, poi gli arrivi — in un tick lungo (`Dt > 0.25`: velocità ×4, un fotogramma lento)
lancerebbero `i+1` prima che l'arrivo di `i` fermi `Next Action`.

🔑 **Il confine `Next Action` passa all'arrivo.** `#2855` vuole la fermata *«dopo aver mostrato il colpo»*
(`:8482-8484`): il colpo è mostrato quando arriva, non quando parte. Per la stessa ragione il riferimento della
fermata (`:8143`) diventa l'ultimo colpo **arrivato**.

➕ rev. **I tracer si consegnano alla mappa prima di ogni uscita del ramo Blast** (`:8430-8434`, `:8443-8447`,
`:8501`), con un `ON_SCOPE_EXIT` all'ingresso del ramo: altrimenti a una fermata il tracer resterebbe disegnato
nella posizione del tick prima.

🔑 **Pausa, `K`, `L`.** Il tracer è funzione dell'orologio: in pausa il tick esce subito (`:8175`) e il tracer
resta fermo a mezz'aria. ➕ rev. `L` (`Player/RTPlayerController.cpp:714`) chiama `StepMicroStep`, che in un Blast
senza spinte non avanza (`RTTurnManager.cpp:8100-8106`): lì il tracer resta fermo finché non si riprende.

---

## 3. Dati

➕ rev. Un campo solo, `HitGeometry`, di un tipo nuovo che dichiara di sé *«solo `Attack`, solo playback»*:

```cpp
USTRUCT(BlueprintType)
struct FRTHitGeometry
{
	UPROPERTY() bool bResolved = false;           // ⛔ unico indicatore di presenza: (0,0,0) e' una cella valida
	UPROPERTY() FRTCellId From;                   // ResolveImpactOrigin (D-302 punto 3)
	UPROPERTY() FRTCellId Impact;                 // HexUnits[Hit.TargetId].Cell, prima di ogni spostamento
	UPROPERTY() FRTKnowledgeVerdict FromVerdict;  // FreezeVerdictFor(UnitAt(Attaccante, From))
	UPROPERTY() FRTKnowledgeVerdict ImpactVerdict;// FreezeVerdictFor(UnitAt(Vittima, Impact))
};
```

| Campo di `FRTResolvedEvent` | Su `Attack` oggi | Dopo |
|---|---|---|
| `Shape` | default `Single` | `Intents[Hit.IntentIndex].Shape` — *«la forma si legge dall'INTENTO»* (`:2520`). ➕ rev. L'intestazione *«Solo per `AttackFootprint`»* di `RTResolvedEvent.h:334` si corregge: il campo vale anche per `Attack` |
| `HitGeometry` *(nuovo)* | — | valorizzato **solo** se `ResolveImpactOrigin` risponde `true` e la vittima esiste; altrimenti `bResolved = false` |

- ⛔ **Le soggettività sono dichiarate**: il verdetto dell'origine ha per soggetto l'**attaccante**, quello
  dell'impatto la **vittima**. `FreezeVerdict` concede ogni unità alla propria squadra (`ClassifyTarget`, `Perception/RTTeamKnowledge.cpp:191-196`), quindi chi spara vede
  sempre la propria origine e chi è colpito il proprio impatto.
- ⛔ **Nessun campo nuovo in `StateHash`, TurnLog, snapshot o replay.** Vivono in `ResolvedTimeline`, che ne è
  fuori per costruzione (`:2831-2833`). Il gate di determinismo lo conferma; non lo si assume.
- ⛔ `AimCell` e `Origin` **non** si riusano: sull'impronta significano *mira dichiarata* e *cella
  dell'attaccante*, mentre `ResolveImpactOrigin` per un'area rende il centro d'impatto. Due significati in un campo
  si pagano a ogni lettura.
- `FRTResolvedEvent` guadagna `meta = (RTServerOnly)` (P5).

---

## 4. Componenti e file

| File | Cosa cambia |
|---|---|
| `Turn/RTResolvedEvent.h` | `FRTHitGeometry`, il campo `HitGeometry`, `RTServerOnly` sul tipo, l'intestazione di `Shape` |
| `Turn/RTTurnManager.cpp` (emissione `Attack`, `:6254-6276`) | `Shape` dall'intento; `HitGeometry` da `ResolveImpactOrigin`, `HexUnits[Hit.TargetId].Cell` e due `FreezeVerdictFor` |
| `Map/RTPlaybackTracer.h` *(nuovo)* | `ERTTracerStyle` (`None` / `Projectile` / `Jet`) e `FRTPlaybackTracer` (estremi in celle, avanzamento, stile) |
| `Turn/RTPlaybackLibrary.{h,cpp}` | **pure**: `TracerSegment(Style, Da, A, Alpha, Dardo)`, `TracerFlightFor(bEligible, F, A)`, `AttackBeatSeconds(Beat, A, Flights)`, `AttackBeatsDue(t, A, Flights)`, `TracerAlpha(...)`, `IsTracerEligible(Ev)`, `TracerStyleFor(Ev, ViewerTeamId)` |
| `Turn/RTTurnManager.{h,cpp}` (tick del Blast) | ➕ rev. `AttacksShown` cede il posto a **un cursore di battiti**, azzerato in `EnterPlaybackPhase` (`:7982`) e in `FinishPlayback`, **mai** in `BeginPlayback` (l'estensione con `bPreserveClock` salta `EnterPlaybackPhase`); `ON_SCOPE_EXIT` che consegna i tracer; rete di finalizzazione per battiti; canale spento a fine Blast |
| `Map/RTHexMapActor.{h,cpp}` | canale `SetPlaybackTracers` / `ClearPlaybackTracers`, disegnato nel `Tick` con `DisegnaLineaAnteprima`, incluso in `HasAnythingToDraw` (`:1084-1097`) — separato dal canale dell'impronta come quello lo è dall'anteprima |
| `Turn/RTPresentationBinding.cpp` | la voce `Attack` dichiara anche `SetPlaybackTracers` |
| test (§6), `docs/technical/test-manuali-pie.md` e `docs/roadmap/editor-sessions.yaml` (voce `PIE-V01-TRACER`, scritta **con** la feature) | |

⛔ Nessun `.uasset`. Nessun cambio a resolver, LOS, `HexHitCells`, catalogo o formato scenario. La presentazione
**non** richiama `HexHitCells`, LOS o targeting (#2454, hard rule).

---

## 5. Errori e degrado

| Caso | Comportamento |
|---|---|
| Attaccante ignoto a chi guarda | niente tracer; `Hit` e numero **all'istante d'arrivo**, uguale per tutti |
| Tiro alla cieca che colpisce (D-415, PR #3230 aperta) | ➕ rev. **tracer visibile a chi spara**: D-380 gli rivela la vittima prima che il verdetto si congeli (`:5483`). Un tiro alla cieca che non colpisce nessuno non ha `Attack`: resta la sola impronta. ⚠️ Non testabile su `main` finché #3230 non atterra: oggi l'attacco base richiede la linea di tiro |
| `bResolved = false` (origine non risolvibile, vittima perduta) | niente tracer, `F_eff = 0`: ritmo di oggi |
| Colpo fermato da un muro alto: solo `StructureHit`, nessun `Attack` | niente tracer, ritmo invariato |
| `AttackShowSeconds ≤ 0` | `F_eff = 0`: tutti i colpi insieme, come oggi |
| `TracerFlightSeconds` oltre `0.5·A` | tagliato a `0.5·A` |
| Estensione del playback con `bPreserveClock` (finestra di reazione, il Brace può sospendere il Blast: `RTTurnManager_Blast.cpp:2622`) | ➕ rev. il cursore **sopravvive**: nessun lancio né arrivo ripetuto |
| Rete di finalizzazione (`RTTurnManager.cpp:8540-8561`) | ➕ rev. per battiti: un colpo **mai lanciato** riceve `Attack` e l'arrivo; uno **lanciato e non arrivato** riceve **solo** l'arrivo — mai un secondo `Attack` |
| `SkipPlayback`, `FinishPlayback` | **nessun tracer rigiocato**; il canale si spegne alla fine del Blast **e** in `FinishPlayback`, che esce presto se trattenuto da una finestra (`:8654-8658`). È la politica di seek che #2454 chiede di **scrivere** |
| `Line` con due vittime | ⚠️ **limite noto**: i colpi sono ordinati per `TargetId`, non per distanza (`RTHexCombatLibrary.cpp:653-659`), quindi il getto verso la vittima lontana può partire per primo e attraversare quella vicina. Va con l'ordine dei getti per distanza (§7) |
| Vittima spinta (`PressureJet`; `ImpactShot` col default `Weapon.Impact`) | ⚠️ **limite noto**: la spinta scivola con l'alpha di **fase** dall'inizio del Blast (`RTTurnManager.cpp:8245-8252`), quindi il colpo arriva sulla cella che la vittima sta lasciando. È la famiglia *«gli esiti precedono la scena»* di #2453; cambiarlo è una decisione separata, e il commento in loco lo dice |
| 🔴 Impronta `Line` di un attaccante ignoto | ⚠️ **limite preesistente, non introdotto qui**: `HexLine(From, Target)` meno `From` lascia la cella adiacente a chi spara (`RTHexCombatLibrary.cpp:34-38`), e `BeginPlayback` la disegna senza filtro (`RTTurnManager.cpp:7749-7757`). Per `PressureJet` il fail-closed del tracer **non toglie** ciò che l'impronta già mostra. Vale anche per `Cone`. È [#3551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3551) (§8) |
| Clip assenti (`FabAsset` non presente) | il tracer funziona lo stesso: non dipende da alcuna animazione |
| Server dedicato | `DisegnaLineaAnteprima` non disegna: nessun effetto |

---

## 6. Verifica

### 6.1 Automation, headless

| Test | Asserisce |
|---|---|
| `Playback.TracerSegmentShapes` | il getto resta ancorato all'origine, il proiettile se ne stacca; l'avanzamento oltre 1 non supera l'impatto |
| `HexMapActor.PlaybackTracerIsItsOwnChannel` | il canale **sostituisce** a ogni consegna, si spegne con `ClearPlaybackTracers`, non tocca impronta né anteprima. 🔴 **Mutazione**: `Append` invece di assegnazione → rosso |
| `Preview.TracerDrawsWithDebugDrawingOff` | col debug spento, come in Shipping, il tracer scrive **una** linea nel batcher Foreground, col colore `Attack` |
| `Playback.TracerArrivesAfterFlight` | arrivo = lancio + `F_eff`. 🔴 **Mutazione**: `F_eff = 0` → rosso |
| `Playback.TracerFlightNeverOutlastsTheSlot` | per ogni `F` e `A > 0`, `F_eff <= A/2`, battiti monotoni anche mescolando idonei e non idonei, ultimo arrivo `< N·A` |
| `Playback.EveryAttackArrivesByPhaseEnd` | ➕ rev. sulla durata **vera** di `PhaseTime` sul `Blast`, con il volo peggiore (`A/2`), a fine fase sono usciti lanci **e** arrivi; a `(N-1)·A` manca l'ultimo arrivo. `EveryChannelIsFullyRevealedByPhaseEnd` non lo copre: chiede `(N-1)·A`, l'arrivo vuole fino a `(N-½)·A`. 🔴 **Mutazione**: la durata del Blast in `PhaseTime` accorciata di un intervallo (`(max(1, canali) - 1)·A`) → rosso, e `EveryChannelIsFullyRevealedByPhaseEnd` resta verde |
| `Playback.TracerZeroFlightKeepsTodaysRhythm` | senza volo gli arrivi coincidono con `AttacksToShow`; con `A <= 0` escono tutti subito |
| `Playback.TracerStyleFollowsShapeForBasicAttack` | `Single` → `Projectile`, `Line` → `Jet`; `Area`/`Cone`, azioni non base e `bResolved = false` → `None`; l'azione generica `Action.BasicAttack` è idonea |
| `Privacy.TracerHiddenWhenOriginUnknown` | sul valore: squadra che non vedeva l'attaccante → `None`, squadra dell'attaccante → disegnato. 🔴 **Mutazione**: ignorare `FromVerdict` → rosso |
| `Privacy.TracerRhythmIsTheSameForEveryViewer` | ➕ rev. due squadre con disegni diversi hanno lo stesso volo |
| `Combat.AttackCarriesHitGeometry` | su un `Line` con due vittime ogni `Attack` porta l'origine di `ResolveImpactOrigin`, la forma dell'intento e la cella **della propria** vittima prima della spinta |
| `Combat.CoveredHitCarriesHitGeometry` | copertura bassa: colpo ridotto, geometria risolta, proiettile; copertura alta: nessun `Attack`, quindi nessun tracer |
| `Privacy.UnseenAttackerIsOutOfTheOriginVerdict` | un colpo alle spalle, oltre la consapevolezza ravvicinata: il verdetto dell'origine esclude la squadra colpita. 🔴 **Mutazione**: verdetto `Everyone()` → rosso. ⚠️ Il tiro alla cieca non è su `main` (PR #3230): il suo caso resta di `PIE-V01-BLINDFIRE` |
| `Determinism.HitGeometryStaysOutOfHashes` | ➕ rev. con un hook di test che lascia vuota `HitGeometry`, `StateHash` e `HashTurnLog` dello stesso turno sono identici; controllo positivo che il hook agisca |
| `Playback.HitArrivesAfterTheLaunch` | nel playback vero, l'arrivo cade in un tick successivo al lancio. 🔴 **Mutazione**: volo zero → rosso |
| `Playback.AttackBeatsStayOrderedInOneTick` | ➕ rev. un tick lungo produce `L0, A0, L1, A1`. 🔴 **Mutazione**: due cicli separati → rosso (l'ordine provato per mutazione è richiesto da #2454, *«Test attesi»*) |
| `Reactions.Brace.ExtendedBlastDoesNotReplayHits` | ➕ rev. l'estensione con `bPreserveClock` non ripete né lanci né arrivi. 🔴 **Mutazione**: azzerare il cursore in `BeginPlayback` → rosso |
| `Playback.TracerIsInFlightBetweenLaunchAndArrival` | fra lancio e arrivo la mappa ha **un** tracer, dalla cella dell'attaccante a quella della vittima; dopo l'arrivo nessuno |
| `Playback.TracerChannelClearsAtBlastEnd` | ➕ rev. (a) dopo `SkipPlayback` con un tracer in volo, il canale è spento: passa da `FinishPlayback`, che lo spegne per conto suo; (b) all'uscita dal Blast, il canale è spento: la consegna in uscita ha già lasciato il canale vuoto; (c) con `AttackShowSeconds` abbassato con un colpo in volo e un `Move` dopo il Blast: l'unico caso che arriva alla pulizia di fine Blast. 🔴 **Mutazione**: togliere `ClearPlaybackTracers` dalla finalizzazione di fase del Blast → rosso su (c), verde su (a) e (b) |
| `Playback.NextActionStopsAtTheActionBoundary` *(esistente, `RTPlaybackStopPredicateTests.cpp:283`)* | ➕ rev. **esteso**, non duplicato: la pausa cade dopo l'arrivo e nessun tracer resta a mezz'aria. La parte `#2454` gira su una fixture a **due atti** (`SetUpTwoActTurn`): su un turno a un atto un arrivo non è mai un confine di atto |
| `Privacy.UnseenAttackerTracerIsNotDelivered` | playback con le squadre scambiate: chi guarda è la vittima di un attaccante alle spalle; **nessun tracer su nessun tick**. Controllo positivo con le squadre invertite: chi spara lo vede. 🔴 **Mutazione**: togliere la guardia `Style != None` → rosso. ⚠️ Limite residuo: un viewer costante `0` non si vede headless, perché lo spettatore del test è fisso alla squadra 0 |

⚠️ **Il gate di D-278 non vede una cue mai chiamata**: `FindMissingBindings` conta i nomi non vuoti
(`RTPresentationBinding.cpp:276-370`). Per questo i test asseriscono **quale** cue, **su quale soggetto** e **in
quale istante** dalle funzioni pure — non la presenza di un nome nella tabella.

⛔ Gate che devono restare verdi **senza modifiche** (se uno cambia, la ragione si scrive nella PR):
`RTPlaybackLibraryTests` sul Blast (`AttacksToShowStagger`, `PhaseDurationBlastTakesTheLongerOfShotsAndKnockback`,
`EveryChannelIsFullyRevealedByPhaseEnd`), `RTPlaybackBudgetIntegrationTests`, `RTPlaybackStopPredicateTests`,
`Match.Autobattle.AttackShowSecondsStagesTheBlast`, `Match.Autobattle.EveryRevealedAttackLeavesItsDamageTokenOnTheTarget`,
`Presentation.*`, `Preview.DrawsWithDebugDrawingOff`, `Privacy.ServerOnlyTypesAreNotReplicated`, la suite di
determinismo e replay.

### 6.2 Seduta PIE — `PIE-V01-TRACER`

Una scena, una domanda binaria, per tasti e non per «passi»:

1. `Visual.Combat.Defeat` — Branth (`ImpactShot`) e Ivrin (`PulseShot`) colpiscono lo stesso bersaglio nel
   primo turno: *«Prima che compaia **ciascuno** dei due numeri, vedi qualcosa viaggiare dall'attaccante al
   bersaglio?»* ⚠️ Lo scenario non dichiara `loadout`, quindi Branth porta il default `Weapon.Impact` e
   il suo colpo spinge: il bersaglio che scivola è il limite noto di §5, non il criterio.
2. `Visual.Combat.WaterElectricCoordinated` — il `PressureJet` di Muiren: *«Vedi una linea che si allunga da
   Muiren fino al bersaglio, ancorata a Muiren?»*
3. una scena in cui la squadra di chi guarda **non vede** l'attaccante: *«Vedi una linea partire da una cella
   che non vedi?»* — risposta attesa **no**. Scenario: `Visual.Combat.TracerHiddenFromUnseenAttacker`.
   ⚠️ Per non confondere la domanda con il limite preesistente di §5, l'attaccante usa un `Single`.

La voce va nel registro PIE e in `editor-sessions.yaml`, con un rimando a `PIE-V01-BLINDFIRE` per il tiro alla
cieca. Il verdetto è di chi guarda, non di questa spec.

---

## 7. Limiti e non-goal

- **Per eroe**: scarica a zigzag, impulsi a tratti, proiettile pieno — sotto-progetto 4 (V2).
- **Area e Cone**, e ogni azione che non sia attacco base.
- **Arco balistico**: [#2825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2825). Il tracer è
  retto e non significa niente sul gameplay.
- **Direzione dell'attacco a chi non vede** (CP 13.4): non esiste nel codice, e il tracer non la sostituisce.
- **Niagara** e asset VFX: D-124.
- **Spinta che parte all'arrivo** e **ordine dei getti per distanza**: famiglia #2453, decisioni separate.
- `Weapon.Split`: `ExtraTargets` è dichiarato e non consumato (`Ability/RTEquipmentData.h:115-127`).

---

## 8. Follow-up candidates

- 🔴 **[#3551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3551)**: l'impronta `Line`/`Cone` di un attaccante ignoto ne rivela la cella (§5, ultima riga
  della parte privacy). Misurato sul codice. La geometria del colpo alle spalle c'è già per un `Single`
  (`Visual.Combat.TracerHiddenFromUnseenAttacker`); quella con un `Line` è un criterio di #3551.
- `docs/characters/v0.1/riktor.md` dichiara `Hero.Riktor.ImpactShot`; il codice ha `Hero.Branth.ImpactShot`
  (`git grep -c "Hero.Riktor.ImpactShot" -- Source` → nessuna occorrenza).
- `FindMissingBindings` che verifichi i nomi delle cue contro funzioni reali (§C4 del panel del 2026-09-10).
- Il tracer anche per `Area` (arrivo sull'`AimCell`) e `Cone` (ventaglio), quando il sotto-progetto 4 ne avrà il
  profilo.
