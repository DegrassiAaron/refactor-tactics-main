# Spec panel — la «Master Technical Specification» C# contro il contratto di RefactorTactics

> `SNAPSHOT` · fotografia del 2026-11-07: vale finché è l'ultima misura del suo oggetto.
>
> ➕ **2026-11-08**: i sette conflitti di §1 sono stati **decisi dall'autore** con sette domande a scelta: due
> confermano il contratto, cinque lo riaprono. Registrati in [D-583](../../decisions/RT_PDR_00_Decision_Log.md); esito,
> coerenza fra le scelte e letture del panel in **§16**. Le §4–§11 conservano lo stato alla scrittura del 2026-11-07.

**Data**: 2026-11-07 · **Misurato su**: `main` `8029e80ac` · **Modalità**: critique · discussion · **Focus**: requirements · architecture · testing
**Panel**: Wiegers (requisiti) · Cockburn (attore/goal) · Adzic (esempi) · Fowler (confini e modello dati) · Nygard (failure mode) · Crispin (testabilità), più un verificatore avversario delle premesse (vedi §15)

> Referto della richiesta dell'autore del 2026-11-07: *«aggiorniamo con queste info»*, seguita dal testo
> integrale di una «Master Technical Specification Document» per un prototipo C# / .NET. Il testo è
> conservato com'è stato ricevuto in
> [`../../research/handoff/master-technical-spec-csharp-input-2026-11-07.md`](../../research/handoff/master-technical-spec-csharp-input-2026-11-07.md)
> (`RESEARCH`, non normativo). Qui sotto *«la specifica»* è quel testo; *«il contratto»* è ciò che
> [`docs/README.md`](../../README.md) §«Gerarchia delle fonti» dichiara vigente.
>
> **Non è un owner.** Nessuna regola cambia con questo referto, e nessuna riga di codice. Le regole restano
> dei loro owner (Decision Log, ADR, spec di `gameplay/` e `technical/`, cataloghi di `balance/`); ciò che qui
> chiede una decisione sta in §11 come `DR-n` e aspetta l'autore.
>
> ⛔ **Perché il contratto non si è aggiornato «con queste info»**: la specifica è in conflitto con decisioni
> accettate su fasi, economia del turno, facing, posa nella cella, elementi, formula del danno e identità del
> roster. [`AGENTS.md`](../../../AGENTS.md) §2 vieta di scegliere per plausibilità fra due fonti incompatibili:
> si individua l'owner, si segnala la deriva, e si chiede. È quello che fa questo documento.
>
> Convenzione: niente totali che cambiano da soli. I numeri citati sono quelli della specifica (fissi, è un
> testo) o del contratto (con la sede che li possiede). Gli stati `PASS/FAIL/NOT RUN/N/A` di §15 dicono cosa è
> stato eseguito davvero.

---

## 0. Richiesta

L'autore ha consegnato una specifica unificata — filosofia, geometria a due livelli, movimento, un catalogo
di coperture, un sistema elementale a tre cariche, un roster di otto eroi con numeri, la direzione artistica
e un modello dati C# — e ha chiesto di «aggiornare» con essa. La domanda che il panel ha dovuto porre prima
di ogni altra è **che cosa aggiornare**: la specifica dichiara di essere *«riferimento assoluto per
l'aggiornamento del codice sorgente del prototipo in C# / .NET»*, e il repository non ha un prototipo
C# / .NET da aggiornare.

## 1. Verdetto in breve

- **Fuori piattaforma.** La specifica si rivolge a un prototipo C# / .NET. Il progetto è Unreal Engine 5.8.1
  in C++ ([`AGENTS.md`](../../../AGENTS.md) §1) e la fase C# è chiusa dal 2026-08-05
  ([`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §2). Il §9 della specifica (le classi
  `SkillData`, `CharacterState`, `WallSegment`) descrive tipi che non esistono; i loro omologhi reali stanno in
  §12.
- **In conflitto con il contratto sui pilastri**, non sui dettagli: quattro fasi con una «Reaction Phase» al
  posto delle cinque macro-fasi con il `Move` dopo il `Blast` (ADR-0003 §1); 6 Action Point al posto degli
  slot, respinti per nome da `D-114`; facing a 12 spicchi con bonus di fianco al posto delle sei direzioni
  di `D-126` e dell'emisfero posteriore di ADR-0005 §4a; micro-posizione in uno spicchio al posto della regola
  *«uno slot di occupancy, dodici settori come righello»* (`D-302`/`D-303`/`D-308`); dodici elementi e una
  combo a tre cariche al posto dell'ottagono a otto (`D-272`) e del catalogo delle 28 coppie (`D-275`); una
  formula moltiplicativa con HP a quattro cifre al posto dei numeri piatti dei cataloghi; otto nomi retail di
  Paragon come identità al posto di Aevik · Muiren · Branth · Ivrin · Ward · Rime · Vigil · Tethra (`D-334`).
- **Allineata nelle intenzioni che contano**: determinismo senza RNG, turni simultanei, un'unità per esagono,
  sei direzioni, `Prep → Dash → Blast`, `Overwatch · Guard · Brace` universali, un roster a otto nella v0.1
  (`D-576`, E35), coperture per bordo distruttibili, la riscrittura delle strutture in `Prep` (CP 9.5), il
  timer 40–45 s del 3v3, l'anteprima del piano, i fumetti in stile manga (`D-582`).
- **Internamente contraddittoria in due punti** che vanno risolti prima di qualunque recepimento: una
  reazione risolta in una fase *successiva* al Blast non può «bloccare il tiro» del Blast (§6 contro §1);
  e il budget 6 AP si esaurisce con `Prep 1 + Dash 2 + Blast 3`, quindi una Reaction da 2 AP costa sempre una
  delle tre (§1 contro §7). Sono di §9.
- **Che cosa si può fare ora**: conservare l'input (fatto), registrare le decisioni richieste (§11) e, se
  l'autore conferma, aprire le due voci `OPEN` che la specifica introduce davvero come novità — il bonus di
  danno alle spalle e la direzione artistica cel-shaded. Tutto il resto è **contenuto candidato** per epic
  esistenti (E52, E35, CP 9.x), non regola.

## 2. Attore, precondizioni, garanzie

- **Attore primario** (Cockburn): l'autore, che dal 2026-11-04 gioca la v0.1 ridefinita «giocabile» e ne
  emette il verdetto (`D-576`, gate `G17`). La specifica non nomina un giocatore: nomina Claude come lettore e
  il codice C# come oggetto. Il goal reale, letto dalla richiesta, è *«far entrare queste idee nel gioco»*.
- **Precondizione** che la specifica assume e che il repository nega: che esista una base C# da refattorizzare
  e che i suoi tipi siano l'autorità. Il contratto dice l'opposto: *«C++ definisce cosa è possibile; Data
  Asset e Blueprint configurano varianti»* ([`AGENTS.md`](../../../AGENTS.md) §3, invarianti 12–13).
- **Garanzia minima** che questo referto dà: ogni affermazione della specifica ha una riga in §4 con uno stato
  e una fonte, e nessuna è stata recepita in un documento `CANONICAL` o `CURRENT`.

## 3. Definizioni

| Termine | Qui significa |
|---|---|
| **ALLINEATO** | la specifica dice ciò che il contratto già dice, magari con altre parole |
| **PARZIALE** | l'intenzione coincide, la regola o il numero no; il contratto ha già una sede per la cosa |
| **CONTRACT CONFLICT** | la specifica contraddice una decisione accettata (`D-nnn`, ADR, spec owner) |
| **ASSENTE** | il contratto non ha la cosa, e nessuna decisione la vieta: è contenuto candidato o una voce `OPEN` |
| **FUORI PIATTAFORMA** | parla di C# / .NET o di un tipo che non esiste in Unreal |
| **NON VERIFICATO** | il panel non ha misurato la premessa; lo dice invece di dedurla |

## 4. Mappa delle premesse — la specifica contro il contratto

Ogni riga: che cosa dice la specifica · stato · che cosa dice il contratto, con la sede che lo possiede.

### 4.1 Filosofia e turno (specifica §1)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 1 | 100% deterministico, zero RNG, stesso input → stesso output | **ALLINEATO** | invarianti 2–6 di [`AGENTS.md`](../../../AGENTS.md) §3; ADR-0009 (replay logico canonico); hash e `TurnLog` |
| 2 | turni simultanei con finestra di 45 s, per 3v3 o 4v4 | **PARZIALE** | 3v3 è lo Standard (`D-256`); 4v4 è stress/Operations, non formato (`spec-durata-partita-e-scala-mappe.md` riga 18); la v0.1 è 2v2 contro bot. `PlanningMax` **40–45 s** è la baseline del 3v3, **30 s** il valore in codice del 2v2, costante e da tarare sul misurato, mai alzato per analogia ([`spec-pacing-turno.md`](../../gameplay/spec-pacing-turno.md) D2, D6; [`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) §7.1 «Timer massimo + Ready anticipato») |
| 3 | quattro fasi: Prep · Dash · Blast · DoT/Reaction | **CONTRACT CONFLICT** | cinque macro-fasi `Planning → Prep → Dash → Blast → Move → Cleanup`, `ERTMatchPhase` invariato e **il `Move` normale dopo il Blast è l'identità tattica** (ADR-0003 §1; [`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md) §1 regola 1). `Reaction` è una **causa**, non una fase ([`AGENTS.md`](../../../AGENTS.md) §5 «Non confondere»): le reazioni si risolvono come segmenti dentro le fasi (ADR-0004). I DoT e le scadenze stanno nel **Cleanup** ([`spec-stati-temporanei-cp82.md`](../../gameplay/spec-stati-temporanei-cp82.md) §4); le azioni ambientali (`Ignite`, `CreateWater`, `Electrify`) risolvono in Cleanup ([`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md) §6) |
| 4 | 6 Action Point per turno spendibili nelle fasi, 7° AP con l'Overclock Meter | **CONTRACT CONFLICT** | *«Non è un sistema di Action Point»*: un turno dà **uno slot movimento, uno principale, una reazione**; la legalità è strutturale, non una sottrazione (`D-028`; **`D-114` respinge per nome la capacità numerica**, 2026-08-12; [`spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) §1, §3). I tre budget sono slot · Movement Point · pivot, più il cooldown; **non esiste una risorsa firma universale** (`D-265`), quindi nemmeno un «meter» comune |
| 5 | Ultimate come categoria (3 AP, CD 3) | **PARZIALE** | una Super è una *Ability Definition* con gate e commit policy, risolta dal resolver che esiste: **«Non si costruisce un sistema chiamato `Ultimate`»** ([`brief-super-e-cooldown.md`](../../gameplay/brief-super-e-cooldown.md) §2–§3). La forma «abilità grossa con cooldown lungo» è già esprimibile; ciò che distingue una Super è il **gate**, che la specifica non dichiara per nessuna delle otto |

### 4.2 Geometria (specifica §2)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 6 | griglia esagonale, un personaggio per esagono, transito sulle sei direttrici | **ALLINEATO** | ADR-0002; `FRTCellId{X=q, Y=r, Layer}` (`Source/RefactorTactics/Map/RTCellId.h`, dalla riga 21), `ERTHexDirection` a sei valori (stesso file, righe 11-19); occupazione massima 1 unità per cella |
| 7 | lato L = 1,5 m, centro-centro 2,6 m, superficie ≈ 5,85 m², apotema 1,3 m, V→M 0,75 m | **PARZIALE** | i derivati sono **coerenti fra loro** per L = 1,5 m (ricalcolati dal panel: `L·√3 = 2,598`, `3√3/2·L² = 5,846`, `L·√3/2 = 1,299`, `L/2 = 0,75`). Ma la dimensione della cella **viene dall'asset mappa**, non dal canone ([`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §6 «Valori vigenti»); `HexSize = 100` è il valore di lavoro dell'authoring ([`spec-hex-geometry-authoring.md`](../../technical/systems/spec-hex-geometry-authoring.md) riga 160), e nessuna decisione fissa 1,5 m. Il repository ha una griglia stampabile «1 cm per 1,5 m» fra i materiali di ricerca (`docs/research/design/grid/`), **non normativa** |
| 8 | dodici spicchi da 30° generati da sei radiali C→V, sei C→M e sei sub-segmenti V→M | **PARZIALE** | i dodici settori **esistono** e sono *«un righello angolare, non dodici caselle»* ([`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md) §2); i punti notevoli e i quanti `(HexSize·√3/4, HexSize/4)` stanno in [`spec-hex-geometry-authoring.md`](../../technical/systems/spec-hex-geometry-authoring.md) riga 243. La geometria intra-cella è **autorevole** per copertura, blocco parziale del movimento e occlusione (`D-270`) |
| 9 | micro-posizionamento: l'eroe staziona in uno spicchio specifico | **CONTRACT CONFLICT** | ⛔ *«dodici sottocelle navigabili»*, ⛔ *«coordinate float autorevoli intra-hex»*, ⛔ *«un secondo slot di occupancy»* ([`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md) §2, §7). L'intenzione «incollato a un muro o in campo aperto» esiste come **`CoverSelection`**, stato discreto autorevole che entra nell'hash (`D-302`, §13.2–13.3), e come **regione di posa** (§3), non come indice di settore della posizione |
| 10 | facing come direzione vettoriale con arco primario 30°–60°, `FacingSector` t1–t12 | **CONTRACT CONFLICT** | la primitiva semantica del facing sono **le sei direzioni relative** (`D-126`, ADR-0005 §4-bis); `ARTUnit::Facing` è un `ERTHexDirection` (`Source/RefactorTactics/Unit/RTUnit.h:152`); il cono è la geometria di `HexCone` a 120°. La rotazione è una capacità per eroe misurata in step (ADR-0008, `MoveEndPivotMaxSteps` / `DashEndPivotMaxSteps` in `URTHeroData`) |
| 11 | flanking: dal cono posteriore +50% danno fisso e ignora il 50% della DEF | **CONTRACT CONFLICT** | l'emisfero posteriore **toglie una protezione invece di aggiungere danno**: annulla la copertura bassa (−10) e la `Guard` (−15), *«nessun modificatore nuovo, nessun numero da bilanciare»* (ADR-0005 §4a). L'alternativa *«retro = +danno»* è nella tabella delle **scartate** (ADR-0005 §«Alternative considerate»). Riaprirla è una decisione, **DR-3** |
| 12 | LoS «millimetrica» attraverso fessure fino a 30° | **PARZIALE / NON MISURABILE** | la LoS è un servizio distinto da pathfinding, targeting e traiettorie (invariante 11); un muro interno autorevole occlude vista e proiettili con **una sola primitiva deterministica** (`D-269`). «Millimetrico» non è un criterio verificabile (Wiegers, §5) |

### 4.3 Movimento e collisioni (specifica §3)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 13 | Micro-Shift (Prep, 1 AP): 1 esagono o rotazione fino a 180°, non attiva Overwatch | **PARZIALE** | la rotazione finale è il **pivot** (ADR-0008), budget in step per eroe, **non si paga in Movement Point** ([`spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) §3.2); il passo di una cella in fase `Dash` è `Action.Reposition` (2 celle, lineare). Nessuna azione del catalogo è «invisibile» all'Overwatch per definizione: l'Overwatch arma in `Prep` e il suo cono **è** il facing (ADR-0005 §4c) |
| 14 | Corsa Tattica (Dash, 2 AP, 2–3 esagoni); Sprint (Dash, 3 AP, 4–5 esagoni, «consuma metà AP», rivela la posizione | **CONTRACT CONFLICT** sulla fase · **PARZIALE** sul resto | il movimento normale è `Action.Move` a **5 MP** in fase **Move, dopo il Blast**; `Sprint` è un **profilo di Move** (×2 del budget, [`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md) riga 227; *«non è un Dash … risolve dopo il Blast»*, riga 254) che consuma **entrambi** gli slot (`ERTActionSlot::MovementAndMain`, `Source/RefactorTactics/Ability/RTActionDef.h:79-80`) — l'idea «consuma metà» è lì, in forma di slot. **Sprint ≠ Dash** ([`AGENTS.md`](../../../AGENTS.md) §5 Pin). Il rumore esiste: `HearingThreshold` per eroe in `URTHeroData` e la conoscenza parziale ([`conoscenza-parziale-visibile-spec.md`](../../technical/systems/conoscenza-parziale-visibile-spec.md)) |
| 15 | Disimpegno / Roll (Dash, 2 AP, 1 esagono, niente attacchi d'opportunità) | **PARZIALE** | `Action.Withdraw` (×0,25 del budget, fase Move) e la mobilità lineare `Dodge` in `Dash`; **non esistono attacchi d'opportunità** nel contratto, quindi la clausola non ha oggetto |
| 16 | transito attraverso un alleato in Dash, senza fermarsi | **PARZIALE** · resto **NON VERIFICATO** | lo **scambio** testa a testa passa, fra alleati e anche fra avversari (`D-443`, poi `D-445`; `RefactorTactics.HexSim.SwapPassesRegardlessOfTeam`, `Source/RefactorTactics/Tests/RTHexSimTests.cpp:3852`), e la destinazione occupata è una scommessa dichiarata nel movimento a budget (`D-445`). La **traversata** di un alleato che resta fermo non è stata misurata (§15) |
| 17 | contesa: chi anticipa in Prep prende la cella, l'altro «rimbalza nello spicchio precedente» con `Body Blocked` | **PARZIALE** | **arrivo tardivo** e **priorità** esistono: chi entra in un micro-step precedente rende la cella occupata; a priorità diverse l'esito è `BlockedByPriority` ([`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md) §7). Non c'è «spicchio precedente» su cui rimbalzare (riga 9): l'unità resta nella cella precedente |
| 18 | contesa perfetta: collisione al centro, entrambi tornano indietro e subiscono 50 danni | **CONTRACT CONFLICT** | due unità sulla stessa `FRTCellId` nello stesso micro-step ottengono `BlockedContested`, *«l'ordine di processing non sceglie mai un vincitore»*; nessun danno (§7 ibid.; [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §6 «destinazione contesa: entrambe restano ferme»). La destinazione occupata si **dichiara** nel movimento a budget e si **rifiuta** nella mobilità lineare (`D-451`, [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §2.0-zero). Aggiungere un danno da impatto è **DR-11** |

### 4.4 Coperture e riscrittura (specifica §4)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 19 | muri sui segmenti radiali o perimetrici dei dodici spicchi, con HP, LoS, DR e passaggio | **PARZIALE** | la copertura è **per bordo** della cella: `FRTHexCover{Edge, Type Low/High, Integrity, bGenerated}` in `FRTHexCellData` (`Source/RefactorTactics/Map/RTHexCellData.h:58`); è distruttibile (`Integrity` 30 e 50, CP 9.2) e il catalogo la possiede ([`RT_TerrainCatalog_v0.1.md`](../../balance/RT_TerrainCatalog_v0.1.md) righe 130–140). La geometria intra-cella autorevole (`D-270`) copre i radiali; porte (`FRTHexDoor`), guardie di bordo (`FRTHexEdgeGuard`) e transizioni (`FRTHexEdge`) esistono nello stesso header |
| 20 | DR **percentuale** per tipo, 15%–75% | **CONTRACT CONFLICT** sul modello | la copertura bassa riduce il danno di un valore **piatto** (`LowCoverDamageReduction`, −10) applicato **sul singolo colpo**, sul bordo dell'ultimo passo della linea attaccante→bersaglio; `Area` → 0 ([`spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) §3). L'alta **blocca** movimento, vista e proiettili. I numeri sono del catalogo; cambiare *modello* (percentuale) è una regola e tocca `D-408` (la Guardia è una riduzione per colpo, per personaggio) |
| 21 | «Flux riscrive» un segmento per 1 turno (Structure Shift di Gadget, Prep, 1 AP) | **PARZIALE** | la fase è **`Prep`**, l'operazione su struttura **è un dato**, non un `ActionId`, e `Reconfigure` *«rifiuta invece di indovinare»* ([`spec-coperture-temporanee-cp95.md`](../../gameplay/spec-coperture-temporanee-cp95.md) §2.1, §2.3, §2.5); la durata si conta dal turno di nascita (§2.4). L'idea c'è; il nome «Flux» no (riga 25) |
| 22 | dodici tipi con reazioni dedicate (trasparenza, liquefazione, riflessione, risonanza, lava, cure, ologramma, portale, silenzio, vapore, detonazione) | **ASSENTE — contenuto candidato** | nessuna decisione li vieta e nessuna li ha; sono materiale per **E52** (ambiente della mappa, ondata 1, [#4004](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4004)) e per le strutture di CP 9.x. Tre di essi toccano invarianti e vanno letti con §5: la *Holo-Wall* «ingannevole» e la *Force Shield* «libera per alleati, bloccante per nemici» chiedono **asimmetria di conoscenza** fra squadre (privacy, [`AGENTS.md`](../../../AGENTS.md) §4); la *Barriera Arcana* «silenzia le Reaction» tocca ADR-0004 |
| 23 | portale d'accelerazione come muro | **PARZIALE** | *«`Portal` è topologia del grafo»* ([`AGENTS.md`](../../../AGENTS.md) §5), cioè una transizione (`ERTHexTransitionKind`), non una copertura |

### 4.5 Elementi e combo (specifica §5–§6)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 24 | dodici elementi: Flux, Fuoco, Acqua, Terra, Fulmine, Freddo, Morte, Vita, Arcano, Scudo, Aria, Disintegrazione | **CONTRACT CONFLICT** | il vocabolario è a **otto**, in ottagono orario `Fire → Void → Air → Harmonic → Water → Electric → Earth → Acid` (`D-272`). Cinque coincidono (Fuoco, Acqua, Terra, Fulmine, Aria); `Arcano`≈`Harmonic` e `Morte`≈`Void` sono letture del panel, non decisioni; Freddo, Vita, Scudo, Disintegrazione e Flux **non esistono** come elementi. `Scudo` è un'azione e uno stato (`Action.Shield`, `ARTUnit::Shield`), non un elemento |
| 25 | il nome `Flux` | **CONTRACT CONFLICT** sul testo | `D-130` ha rimosso `Flux` da tutto il repository come nome del roster legacy, e [`paragon.md`](../../characters/paragon.md) riga 56 dichiara che quei nomi *«non tornano»*. Un elemento con lo stesso nome non è la stessa cosa, ma rimette la parola nel testo che quella decisione ha ripulito: va nominato diversamente o deciso esplicitamente |
| 26 | tre elementi su un bersaglio o una cella, in qualunque ordine, scatenano un Finisher; l'effetto dipende dalla fase del terzo | **CONTRACT CONFLICT** | l'ottagono **non produce automaticamente** reazione, danno o status: *«solo un `ReactionId` stabile con dati di effetto espliciti crea un comportamento»*; tutte e 28 le coppie hanno **una** voce di catalogo con stato di ciclo di vita (`D-273`, `D-275`). Niente `PairBonus`, `ComboAbility` o branch core per sinergie (invariante 16; ADR-0006); *«nessun secondo motore»* ([`brief-super-e-cooldown.md`](../../gameplay/brief-super-e-cooldown.md) §2). Gli elementi sono **verbi** (`Generate · Apply · Propagate · Transform · Consume`), non colori del danno ([`prd-damage-model-armor-shield.md`](../../research/prd/prd-damage-model-armor-shield.md) §«Gli elementi sono verbi»). Un «bonus 150–250» con zero RNG non dice **chi** sceglie dentro l'intervallo (Wiegers) |
| 27 | combo-breaking per disruption temporale, displacement, reazione | **ALLINEATO nell'intenzione** | `Interrupt`, il displacement forzato (`spec-tassonomia-movimento.md` §3), le reazioni componibili (CP 5.5) e la `Guard` esistono come **fatti di sistema** sotto `ActionId` propri; il contratto li vuole così, non come un sotto-sistema «combo-break» |

### 4.6 Roster e numeri (specifica §7)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 28 | otto eroi nella partita | **ALLINEATO** | la v0.1 ridefinita anticipa **E35, il roster a otto** (`D-576` (4)); i quattro ex v0.2 sono nella v0.1 ([`paragon.md`](../../characters/paragon.md) §«Mapping visuale») |
| 29 | i nomi: Riktor, Steel, Kallari, Feng Mao, Murdock, Gadget, Muriel, The Fey | **CONTRACT CONFLICT** | un nome identico allo slot Paragon **è** identità Paragon (`D-321`) e le identità RefactorTactics sono **Aevik · Muiren · Branth · Ivrin** (`D-334`) e **Ward · Rime · Vigil · Tethra** (#322); gli slot sono base visuale (`D-037`). Dei nomi della specifica quattro sono slot del roster (Gadget→Aevik, Riktor→Branth, Steel→Ward, Murdock→Vigil) e quattro sono **candidati `UNASSIGNED`** (Kallari, Feng Mao, Muriel, The Fey, in `docs/characters/candidates/`); mancano gli slot Phase, Wraith, Aurora e Kwang. La nota IP del canone chiede nomi e personaggi **originali** ([`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §2) |
| 30 | struttura del kit: attacco base + Prep + Dash + Blast + Ultimate + Reaction (sei voci) | **ALLINEATO nel conteggio** | un eroe ha attacco base, **quattro fondamentali** e al più **una** generica derivata: `ValidateHeroes` ammette 5–6 azioni ([`RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md) §«Struttura di un eroe»). La distribuzione per fase non è un vincolo del kit: la fase la dichiara **ogni azione** (`FRTActionDef::ResolutionPhase`) |
| 31 | HP 950–1900, DEF 15–60, RES 15–40, danni 110–520 | **CONTRACT CONFLICT** sulla scala e sull'owner | i valori vigenti sono dei cataloghi di `balance/` (`D-210`): `MaxHealth` 90–100, danni 18–24, scudo 25, `Guard` −15, copertura −10 ([`RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md) §1; [`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md) §4). `DEF`/`RES` come statistiche **non esistono** in `Source/`: `Armor`, `ArmorDEF`, `DamageResistance` e `Resistance` danno **zero** file negli header su `8029e80ac` (rimisurato il 2026-11-07; era già zero il 2026-08-28, PRD E49 §«La formula additiva») |
| 32 | `Danno = Base × Mod × 100/(100+DEF)` | **CONTRACT CONFLICT** con la proposta E49 · **ASSENTE** nel codice | il danno oggi è una **sottrazione piatta** per colpo; la proposta E49 è additiva (`FinalDamage = max(0, BaseDamage − Defense)`) e **non congelata** ([`prd-damage-model-armor-shield.md`](../../research/prd/prd-damage-model-armor-shield.md) §«La formula additiva»). Una formula moltiplicativa è una **terza** proposta: **DR-6**. Le percentuali della specifica sono coerenti con la sua formula (ricalcolate: DEF 45 → 31,0%, 60 → 37,5%, 15 → 13,0%, 35 → 25,9%; Kallari 285 = 190 × 1,5) |
| 33 | `Move: 2–4 esagoni/turno` per eroe | **PARZIALE** | `MovePoints` per eroe in `URTHeroData` (5 a catalogo), costo intero per cella, Sprint ×2 (`D-427`): la grandezza esiste, la scala no |
| 34 | la `Guard` di Riktor e Steel assorbe «fino a 300 / erige scudo 250» su un compagno | **PARZIALE** | `GuardReduction` è **per personaggio** e per colpo (`D-408`, `URTHeroData::GuardReduction = 15`); `Action.Guard` protegge chi la usa, in `Prep`; lo scudo di una reazione protegge dal colpo che l'ha innescata (CP 5.5 D3). Una guardia **su un compagno** è un'abilità d'eroe da dichiarare come tale (ADR-0006: un'abilità ha un solo owner) |
| 35 | Overwatch che spara al primo nemico in movimento nel cono | **ALLINEATO** | `Action.Overwatch` universale, arma in `Prep`, cono dal facing, compete con l'azione principale ([`AGENTS.md`](../../../AGENTS.md) §5; ADR-0005 §4c) |
| 36 | stati: Stun, Blind, Root, Freeze, Wet, DoT | **PARZIALE** | `Wet`, `Burning`, `Marked`, `Electrified`, `Obscured` esistono con regole di durata e Cleanup ([`spec-stati-temporanei-cp82.md`](../../gameplay/spec-stati-temporanei-cp82.md)); i tag di stato dichiarati in `Source/RefactorTactics/Core/RTGameplayTags.h` sono `Status.Stunned`, `Status.Root`, `Status.Prone`, `Status.Exposed`, `Status.Electrified`, `Status.UnbalancedIffSlid` (più le famiglie `Control` e `Movement`); una stordita non erige né cura (`D-564`/`D-565`); Freeze è nel brief del ghiaccio ([`brief-ghiaccio.md`](../../gameplay/brief-ghiaccio.md)), **non un tag**; `Blind` non esiste come tag, e l'offuscamento della vista è `Obscured` con la regola D4 di CP 8.2 |

### 4.7 Presentazione e HUD (specifica §8)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 37 | rendering cel-shaded, stile manga/anime | **ASSENTE — voce `OPEN` candidata** | *«Direzione artistica: inesistente; si usano placeholder e asset Paragon»* ([`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) riga 2271; [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §9). **E52** autora il look «lit» della mappa nell'ondata 1 (#4005: palette, atmosfera, riferimenti visivi *«scritti e accettati dall'autore prima di autorare»*): è la sede in cui una scelta cel-shaded entrerebbe. **DR-8** |
| 38 | onomatopee 3D sui punti di rottura; fumetti | **ALLINEATO nell'intenzione** | i **fumetti in stile manga** all'attivazione di una skill e sui blocchi sono decisione d'autore, **ondata 2**, con la regola *«seguono la visibilità degli eventi»* (`D-582` (3) e conferma (b)); nelle parole dell'autore, nella *«visibilità tattica (quella semplificata zoomando indietro)»* non si vedono e parla il log |
| 39 | split-screen diagonale quando due eroi agiscono nella stessa fase | **PARZIALE** con un vincolo | il ritmo cinematografico è taratura, un solo produttore, e *«il segmento non si rompe per far posto all'inquadratura»* ([`spec-anima-risoluzione.md`](../../gameplay/spec-anima-risoluzione.md) §14.4); la simultaneità si mostra **dentro** il segmento. La presentazione non decide mai l'esito (invariante 1) |
| 40 | bussola under-foot a 12 spicchi con coni coperti/aperti e arco di facing | **PARZIALE** | il righello a dodici settori esiste (riga 8), il facing è a sei lati (riga 10); la HUD ha le aree di contesto (vista, area colpita, portata) decise dai referti del 2026-10-06 e da `D-464`…`D-466`; una «bussola» sotto l'eroe **non esiste** nei documenti HUD: fuori da `archive/`, «bussola» o «compass» compaiono solo in [`spec-hex-geometry-authoring.md`](../../technical/systems/spec-hex-geometry-authoring.md) §2.1.1, dove nomina l'ancoraggio delle direzioni `E/NE/…` agli assi del mondo |
| 41 | timeline a otto tracce per fase, colorata | **PARZIALE** | la timeline di eventi risolti e il *Planned-vs-Actual* su **una sola** timeline sono decisi ([`spec-anima-risoluzione.md`](../../gameplay/spec-anima-risoluzione.md) §4, §14.6); il planning visuale ha un brief ([`brief-planning-visuale.md`](../../technical/systems/brief-planning-visuale.md)). «Otto tracce» presuppone otto eroi anche in 3v3 (riga 2) |
| 42 | ghost unit e nastri vettoriali prima di confermare | **ALLINEATO** | l'anteprima del piano (`RTPlanPreview.h`) e i «ghost delle azioni» sono già nel perimetro del Planning ([`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) §«Perché 40–45 s») |
| 43 | Tactical Ping sugli spicchi, verso gli alleati | **PARZIALE** | gli **Intenti condivisi** sono il sistema (`spec-intenti-condivisi.md`), completo in P1 (north-star); un marker **su uno spicchio** presuppone la riga 9 |
| 44 | Combo Snap che «allinea automaticamente l'elemento complementare nella fase corretta» | **CONTRACT CONFLICT** | presuppone la combo a tre cariche (riga 26) e un'assistenza che modifica il piano: il client **propone**, e ciò che propone è una scelta del giocatore (invariante di autorità, [`AGENTS.md`](../../../AGENTS.md) §4) |

### 4.8 Modello dati (specifica §9)

| # | La specifica dice | Stato | Il contratto dice (fonte) |
|---|---|---|---|
| 45 | `namespace RefactorTactics.Core`, classi C# | **FUORI PIATTAFORMA** | UE 5.8.1 C++; gli omologhi reali sono in §12 |
| 46 | `float` per HP, DEF, danno, DR nello stato canonico | **CONTRACT CONFLICT** | *«ID, costi, priorità, duration … espliciti/versionati»*, costi **interi** (invariante 14; ADR-0002 «costi interi, tie-break su ID»); `Health`, `Shield`, `MaxHealth` sono `int32` (`RTUnit.h:159-165`); l'hash del replay vuole interi (`spec-cover-placement-intra-hex.md` §11) |
| 47 | `ElementType` come enum chiuso | **PARZIALE** | affinità e stati viaggiano come `FName`/tag (`URTHeroData::Affinity`, `Status.*`): il vocabolario è dato, non codice |

## 5. Il panel — sei lenti sulla stessa specifica

**WIEGERS — requisiti.** La specifica è ricca di **numeri** e povera di **criteri**. «Tracciamento millimetrico
fino a soli 30°», «silhouette marcate», «altissima chiarezza visiva» non sono verificabili. Dove è precisa lo è
in un modo che il contratto non può ospitare: «+50% e ignora il 50% della DEF» è un requisito misurabile, ma
riferito a una statistica (`DEF`) che non esiste. Il difetto più grave è un intervallo con zero RNG: *«150–250
Danno Bonus»*, *«400–600»*. Un sistema deterministico che dichiara un intervallo deve dire **quale regola sceglie
il valore**, altrimenti o è casuale (vietato) o è incompleto. La specifica non lo dice da nessuna parte.
Raccomandazione: ogni numero diventi **un** valore con un owner (il catalogo), ogni «fino a» un criterio.

**COCKBURN — attore e goal.** Il testo nomina il proprio lettore («riferimento assoluto per Claude») e il
proprio oggetto («prototipo C# / .NET»), e nessuno dei due è l'attore del gioco. L'attore è l'autore che gioca
(`D-576`) e la domanda che `G17` gli fa è *«ti diverte?»*, non *«il codice segue la specifica?»*. Riletta così, la
specifica è una **collezione di desideri di gioco** — vedere il fianco scoperto, sentire il muro che cambia
stato, un roster di otto che si riconosce — e ciascuno ha già un posto: ADR-0005 per il fianco, CP 9.5 ed E52 per
il muro, E35 per il roster. Il goal «aggiorniamo» si serve meglio portando i desideri alle loro sedi che
sostituendo il contratto con il testo.

**ADZIC — esempi.** La specifica ha **un** esempio (Riktor combo-breaker) e quell'esempio **contraddice la
specifica stessa**: Chain Intercept è una Reaction, la Reaction Phase è la Fase 3, e il colpo che dovrebbe
«bloccare» passa nella Fase 2. Un esempio è la prova che una regola si tiene; qui è la prova che non si tiene.
Tre esempi che il repository saprebbe eseguire oggi, nella forma del suo harness di scenari (`-RTScenario=<Id>`,
`docs/technical/tooling/scenario-map.md`):

```text
Dato      due unità che pianificano Move verso la stessa cella nello stesso micro-step
Quando    il Move risolve
Allora    entrambe restano nella cella precedente, esito BlockedContested, nessun danno
          (contratto) — la specifica vorrebbe 50 danni a testa (DR-11)

Dato      un colpo la cui origine non è nei tre lati frontali del bersaglio, che è dietro copertura bassa
Quando    il colpo risolve
Allora    la riduzione −10 non si applica; il danno base resta quello
          (contratto, ADR-0005 §4a) — la specifica vorrebbe ×1,5 e metà DEF (DR-3)

Dato      Wet sul bersaglio, poi un'azione elettrica che lo colpisce
Quando    l'azione risolve
Allora    accade ciò che la voce di catalogo della coppia Water·Electric dichiara (propagazione), e nulla
          «si accumula» verso un terzo elemento (D-275) — la specifica vorrebbe un contatore a tre
```

**FOWLER — confini e modello dati.** Il §9 confonde quattro cose che il contratto tiene separate: *geometria*,
*occupancy*, *facing* e *copertura* finiscono tutte in `SubSectorIndex` / `FacingSector` / `SectorIndex`, cioè
nello stesso indice a dodici. [`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md)
§5.1 è nata **per impedire esattamente questo** (`SolidWedges · CoverArc · standability · occupancy · LOS`
come cinque domande distinte). `SkillData` fonde dati di catalogo (`Id`, `CooldownTurns`, `Phase`) con semantica
d'effetto (`IgnoresArmor`, `StatusEffectApplied` come stringa libera) che il contratto esprime come effetti
dichiarati e tipizzati (`ERTActionEffect`, CP 5.5 D2). `WallSegment` porta `IsRefactored` + `RefactorDurationTurns`
come **secondo** meccanismo di durata accanto agli stati: CP 9.5 §2.4 ha già deciso che la durata si conta dal
turno di nascita, con la stessa disciplina degli stati. E un enum `ElementType` chiuso in codice rende ogni
elemento nuovo una ricompilazione: il contratto li tiene come dati (`FName`). Il mapping di §12 dice dove vive
ciascun campo.

**NYGARD — failure mode.** Tre punti in cui la specifica si rompe sotto carico:
1. **La Reaction Phase è dopo il Blast.** Ogni reazione «che intercetta» (Chain Intercept, Bodyguard, Aegis,
   Shadow Dodge) arriva a colpo già applicato. O si anticipa la fase, o le reazioni non intercettano nulla: la
   specifica non può avere entrambe. ADR-0004 risolve con **segmenti** e boundary dentro la fase, non con una
   fase a parte.
2. **Il budget AP non chiude.** `Prep 1 + Dash 2 + Blast 3 = 6`: la Reaction da 2 AP non entra mai in un turno
   «pieno». È una scelta di design legittima (la reazione costa un'azione) ma non è scritta, e il 7° AP
   dell'Overclock non ha regola di accumulo: *«quanto cresce, per cosa, si azzera?»* non ha risposta.
3. **Due coperture chiedono conoscenza asimmetrica** — la Holo-Wall «ingannevole» e la Force Shield «libera per
   alleati, bloccante per nemici». In un modello *client propone → server valida* con Team Knowledge
   ([`AGENTS.md`](../../../AGENTS.md) §4) sono realizzabili, ma ciascuna è una **revisione del boundary di
   privacy** ([`CLAUDE.md`](../../../CLAUDE.md) §7): cosa vede il client avversario di una parete che per lui è
   opaca e per l'altro no? Va deciso prima di autorarle.

**CRISPIN — testabilità.** Il contratto ha un apparato di verifica che la specifica ignora: gate `G1`–`G14`
più `G16` della DoD, scenari eseguibili, golden del replay con hash, verifiche di mutazione dichiarate in ogni
spec di `gameplay/`. Tre domande che la specifica deve saper rispondere prima di entrare:
- **Come si testa «+50% ignora 50% DEF»** senza `DEF`? Prima la statistica (E49), poi la regola, poi il
  numero: nell'ordine inverso il test non ha oggetto.
- **Come resta stabile l'hash** con `float` nello stato canonico? Non resta: ADR-0002 e il §11 della spec di
  posa chiedono interi, e il golden del replay lo misura (`Replay.*`).
- **Qual è il criterio di accettazione** di «onomatopee nei punti di rottura»? `D-582` lo ha già scritto per i
  fumetti: *appare, segue la visibilità, sparisce nella vista strategica*. Lo stesso formato serve qui.

## 6. Dove le due fonti concordano — ciò che non richiede alcuna decisione

Sono i punti che l'autore può considerare **già nel gioco o già decisi**, e che la specifica conferma:

- determinismo senza RNG; `Prep → Dash → Blast` come ossatura; un'unità per esagono; sei direzioni;
- `Overwatch · Guard · Brace` come azioni universali; una **reazione** per turno accanto a movimento e
  principale (`ERTActionSlot::Reaction`);
- coperture **per bordo**, distruttibili, con un'operazione di riconfigurazione in `Prep` (CP 9.5);
- il roster a **otto** nella v0.1 (E35, `D-576`);
- `PlanningMax` 40–45 s come baseline del 3v3;
- l'anteprima del piano prima del Ready; la vista semplificata «zoomando indietro» che tace i fumetti e parla col log (`D-582`);
- il rumore del movimento come dato per eroe (`HearingThreshold`) e la conoscenza parziale.

## 7. Scope — che cosa si recepisce, e dove

| Classe | Elemento | Sede |
|---|---|---|
| `CURRENT REQUIRED` | conservare l'input come `RESEARCH` e misurarlo (questo referto) | fatto |
| `CURRENT OPTIONAL` | se l'autore conferma **DR-3** o **DR-8**, una riga ciascuna in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | dopo la risposta a §11 |
| `DEFERRED` | i dodici tipi di copertura come contenuto di E52 / CP 9.x, con la revisione di privacy per i due asimmetrici | [#4004](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4004) e figlie |
| `DEFERRED` | i kit degli otto eroi come **input di design** per le schede delle identità corrispondenti (Branth, Ward, Vigil, Aevik) e per i candidati `UNASSIGNED` (Kallari, Feng Mao, Muriel, The Fey) | `docs/characters/`, E35 |
| `DEFERRED` | la bussola a dodici spicchi e la timeline multi-traccia come proposte HUD | `progettazione-hud.md`, brief planning visuale |
| **non si recepisce** | AP, Reaction Phase, facing a dodici, micro-posizione, dodici elementi, combo a tre, formula moltiplicativa, nomi retail, `float` canonici | conflitti di §4 |

## 8. Esempi-chiave — come la stessa intenzione si scrive nel contratto

| La specifica | La forma del contratto |
|---|---|
| «Sprint: 3 AP, consuma metà AP» | `Action.Sprint`, profilo di Move ×2 del budget, slot `MovementAndMain`: chi sprinta non agisce |
| «Micro-Shift: ruota il Facing fino a 180°» | pivot di fine movimento, `MoveEndPivotMaxSteps` per eroe (0–3 step di 60°) |
| «Structure Shift (Flux): riscrive un segmento per 1 turno» | operazione `Reconfigure` su struttura, fase `Prep`, portata validata, durata dal turno di nascita (CP 9.5) |
| «Chain Intercept (Guard): assorbe fino a 300 su un compagno» | abilità d'eroe con slot `Reaction`, effetto `DamageReduction` dichiarato (CP 5.5 D2), bersaglio per **tipo** (D1); il valore nel catalogo |
| «Flanking +50%» | l'emisfero posteriore **toglie** copertura e `Guard` (ADR-0005 §4a); un bonus è DR-3 |
| «Wet + Fulmine» | la voce della coppia `Water·Electric` nel catalogo delle 28 (D-275): propagazione sulla rete conduttiva, `+8` dell'abilità di Aevik su `Status.Wet` (ownership dell'abilità, ADR-0006) |
| «Ultimate 3 AP CD 3» | Super = Ability Definition con almeno un **gate** (`Resource · State · Environment · Geometry · Prediction · Setup`) e una dichiarazione di rischio (brief §3) |

## 9. Difetti interni della specifica (indipendenti dal contratto)

1. **Ordine delle fasi contro semantica delle reazioni** (§1 vs §6, §7): una Reaction risolta in Fase 3 non
   intercetta un Blast di Fase 2. Vedi Nygard.
2. **Il budget AP non ammette una Reaction in un turno pieno** (§1 vs §7): `1 + 2 + 3 = 6`.
3. **Intervalli senza regola di scelta** con zero RNG (§5: 150–250, 250–350, 400–600).
4. **Overclock Meter** senza regola di accumulo, tetto o azzeramento (§1).
5. **Micro-Shift «non attiva Overwatch»** (§3) contro «Corsa innesca le Reaction Overwatch nel raggio»: che
   cosa l'Overwatch osservi — distanza, fase, tipo di azione — non è detto.
6. **Gadget, Prep**: due abilità nello stesso slot (*Structure Shift* «oppure» *Speed Gate*), senza regola di
   scelta.
7. **Steel, Force Shield**: «muro su sub-segmento» nel kit (§7.2) contro «perimetrico + radiale a L» nel
   catalogo delle coperture (§4 riga 5).
8. **Timeline a otto tracce** (§8) per un formato che la stessa specifica dà anche a sei (§1).
9. **«Spore Spores»** (§7.8): nome duplicato, probabile refuso.
10. **Il diagramma** dei dodici spicchi ha perso V3, V5, M3, M4 e i settori t2–t5, t8–t11 nella copia: va
    ridisegnato prima di citarlo.
11. **Muri «perimetrici»** sono per definizione **condivisi fra due celle**: la specifica li assegna a un
    `HexId` solo (`WallSegment.HexId`), e non dice chi li possiede quando le due celle stanno su layer o
    proprietari diversi. Il contratto risolve con `FRTHexCover.Edge` per cella e con la regola *«una sola sede
    per cosa c'è fra queste due celle»* (CP 9.2 §3).

## 10. Criteri di accettazione di questo referto

- [x] ogni sezione della specifica (§1–§9) ha almeno una riga in §4 con stato e fonte;
- [x] nessun documento `CANONICAL` o `CURRENT` è stato modificato;
- [x] l'input è conservato com'è stato ricevuto, con banner `RESEARCH`;
- [x] le decisioni richieste sono formulate come scelte con conseguenze (§11), non come raccomandazioni
      implicite;
- [x] le premesse **non misurate** sono dichiarate tali (riga 16 e §15), non dedotte.

## 11. Decisioni richieste (`BLOCKED — DECISION REQUIRED`)

Nessuna di queste blocca lavoro in corso: bloccano **il recepimento della specifica**. Dove c'è una
raccomandazione è del panel ed è ribaltabile.

| DR | Domanda | Opzioni | Conseguenze | Raccomandazione |
|---|---|---|---|---|
| **DR-1** | L'economia del turno torna agli **Action Point**? | (a) restano gli slot (`D-114`); (b) si riapre `D-114` | (b) riscrive validazione, catalogo, bot, HUD e la regola *«due leggere non valgono una pesante»*; tocca `#604` | **(a)**. I desideri della specifica (sprint che costa, reazione che costa) hanno già forma negli slot |
| **DR-2** | Le fasi diventano quattro con una **Reaction Phase**? | (a) cinque macro-fasi con Move dopo Blast (ADR-0003); (b) si riapre ADR-0003 | (b) cambia l'identità tattica dichiarata, il bot e ogni spec di `gameplay/`; e il §9.1 della specifica non si risolve spostando una fase | **(a)**; le reazioni restano segmenti (ADR-0004) |
| **DR-3** | Un colpo **alle spalle** aggiunge danno (o ignora difesa), oltre a togliere copertura e Guard? | (a) no (ADR-0005 §4a); (b) sì, con **un** valore nel catalogo; (c) sì, con un moltiplicatore | (b)/(c) riaprono una scartata di ADR-0005 e toccano `D-126` (quali dei sei lati sono «spalle») e il combat math; (c) introduce il primo moltiplicatore generico, che E49 sconsiglia | **(a)** oggi; se l'autore vuole il fianco «che si sente», **(b)** con valore piatto e voce `FAC-*` |
| **DR-4** | L'unità ha una **posizione intra-cella** (spicchio)? | (a) no: `CoverSelection` e regione di posa (`D-302`/`D-303`/`D-308`); (b) sì | (b) è il secondo slot di occupancy che la spec di posa vieta in §2 e §7: pathfinding, contesa, hash e privacy da rifare | **(a)** |
| **DR-5** | Il vocabolario elementale cambia (dodici elementi, combo a tre)? | (a) otto elementi e 28 coppie (`D-272`…`D-275`); (b) si estende l'ottagono; (c) si adotta la combo a tre | (b) rompe l'ottagono e la matrice simmetrica; (c) viola l'invariante 16 e *«nessun secondo motore»* | **(a)**; le idee «Freddo», «Vita», «Disintegrazione» possono vivere come **stati**, **verbi** o **tipi di danno** nel perimetro E49 |
| **DR-6** | La formula del danno è **moltiplicativa** con `DEF`/`RES`? | (a) piatta attuale; (b) additiva E49 (proposta); (c) moltiplicativa della specifica | (b) e (c) richiedono prima le statistiche in `Source/` (E49.1–49.2); (c) cambia la scala di tutti i cataloghi e tutti i golden | nessuna: è il perimetro di E49, e la scelta fra (b) e (c) è dell'autore quando E49 si apre |
| **DR-7** | I nomi degli eroi sono i **retail Paragon**? | (a) identità `D-334` e slot Paragon come base visuale; (b) si riapre `D-321`/`D-334` | (b) riapre la nota IP del canone e la migrazione `#2297` | **(a)**; i kit della specifica entrano come **input** nelle schede delle identità |
| **DR-8** | La **direzione artistica** è cel-shaded / manga? | (a) resta `OPEN`; (b) si decide ora, e E52.1 la recepisce nei riferimenti visivi | (b) costa materiali e post-process nell'ondata 1, dentro il tetto di `D-124` | **(b)** se l'autore la vuole: E52.1 chiede comunque *«palette, atmosfera, riferimenti visivi scritti e accettati prima di autorare»*, ed è il momento più economico |
| **DR-9** | Il timer passa a **45 s**? | (a) 30 s nel 2v2, 40–45 s nel 3v3, tarati sul misurato; (b) 45 s ovunque | (b) contraddice D2/D6 di pacing (*«costante, ma su dato misurato»*) | **(a)**: nessuna decisione nuova |
| **DR-10** | La copertura riduce in **percentuale** per tipo? | (a) piatta per colpo (`D-408`, CP 9.1); (b) percentuale | (b) cambia il modello di `Guard` e copertura insieme; i numeri restano del catalogo | **(a)**; i dodici tipi entrano come **contenuto** con `Integrity` e arco |
| **DR-11** | La contesa perfetta infligge **danno da impatto**? | (a) no (`BlockedContested`); (b) sì, un valore piatto | (b) è un fatto nuovo nel TurnLog e nel replay, e un incentivo a bloccare col corpo | **(a)**; se (b), il valore al catalogo e la voce nel log |

## 12. Mappatura del §9 della specifica sui tipi reali

| Tipo C# della specifica | Omologo reale | Note |
|---|---|---|
| `TemporalPhase {Prep, Dash, Blast, Reaction}` | `ERTMatchPhase {Planning, Prep, Dash, Blast, Move, Cleanup, MatchEnded}` (`Source/RefactorTactics/Turn/RTTurnRules.h:10-19`); per azione `ERTResolutionPhase` rimappato da `URTCatalogLibrary::MapResolutionPhase` | niente `Reaction`: `ERTActionSlot::Reaction` è uno **slot** |
| `ElementType` (enum) | `URTHeroData::Affinity` (`FName`), tag `Status.*`, vocabolario `D-272` | dato, non enum |
| `SkillTargetType` | `FRTActionDef::RangeCells` + `ERTMovementStyle` (`None · Budget · LinearDash · LinearCharge · LinearLeap · LinearPass`) + geometrie `URTHexLibrary::HexCone` / `HexLine` | `SectorTarget` non ha omologo: non esiste il bersaglio-spicchio |
| `SkillData.Id / Name` | `FRTActionDef::ActionId` (`FName` stabile, chiave del TurnLog), `BaseActionId`, `DerivedFromActionId` (`RTActionDef.h:523…`) | tre relazioni distinte, dichiarate per `D-033` |
| `SkillData.Phase / ApCost / CooldownTurns` | `ResolutionPhase`, **`Slot`** (`ERTActionSlot`), `CooldownTurns`, `Priority` | non esiste `ApCost`; la priorità intra-fase è un intero |
| `SkillData.RangeMin / RangeMax` | `RangeCells` (con tre significati dichiarati per `0`) | nessun minimo |
| `SkillData.BaseDamage (float)` | valori interi degli effetti dichiarati (`ERTActionEffect`, CP 5.5) e del catalogo | **interi** |
| `SkillData.IgnoresArmor / RequiresFacing / ModifiesWalls / StatusEffectApplied` | effetti e flag tipizzati dell'azione; `Fallback` (`ERTActionFallback`) e `DeclaresTracking()` | niente stringhe libere |
| `CharacterState.CurrentHexId / SubSectorIndex / FacingSector` | `ARTUnit::Cell` (`FRTCellId`), `CoverSelection` (`D-302`), `ARTUnit::Facing` (`ERTHexDirection`) (`RTUnit.h:142-152`) | nessun indice di settore della posizione |
| `CharacterState.CurrentHp / MaxHp (float)` | `ARTUnit::Health / MaxHealth / Shield` (`int32`, `RTUnit.h:159-165`); `URTHeroData::MaxHealth` | interi |
| `CharacterState.ArmorDEF / ResistanceRES` | **assenti**; perimetro E49 (`Armor`, `DamageResistance[DamageType]`) | vedi DR-6 |
| `CharacterState.AvailableAp / IsOverclocked` | **assenti** per decisione (`D-114`, `D-265`); i budget sono `MovePoints`, pivot steps, cooldown | vedi DR-1 |
| `WallSegment` | `FRTHexCover{Edge, Type, Integrity, bGenerated}` per cella; `FRTHexDoor`, `FRTHexEdgeGuard`, `FRTHexEdge`; geometria intra-cella autorevole (`D-270`) e strutture di CP 9.5 (`RTHexCellData.h:58, 252, 310, 354, 554`) | la copertura è **per bordo**; «ingannevole» e «asimmetrica» non esistono e chiedono la revisione di privacy |
| `WallSegment.DamageReductionDR (float %)` | `LowCoverDamageReduction` piatto; `High` blocca | vedi DR-10 |
| `WallSegment.IsRefactored / RefactorDurationTurns` | durata delle strutture temporanee contata dal turno di nascita (CP 9.5 §2.4), stessa disciplina degli stati | un solo meccanismo di durata |

## 13. Domande aperte per l'autore (non decisioni: chiarimenti)

- **Q1** La specifica è un **testo tuo** o un testo generato altrove e riportato? Cambia come leggerla: una
  decisione d'autore ha un peso che una bozza generata non ha (`D-582` registra alla lettera ciò che l'autore
  ha scritto; qui non c'è una riga di quel tipo).
- **Q2** «Aggiorniamo»: intendevi **sostituire** il contratto, oppure **portare dentro** le idee che mancano?
  Il referto ha assunto la seconda lettura; se è la prima, ciascuna `DR-n` di §11 è una decisione da prendere
  per nome.
- **Q3** Dei dodici tipi di copertura, quali vorresti vedere nella **prima** mappa «lit» (E52.2)? Il contenuto
  si autora una struttura alla volta, e le due asimmetriche (Holo-Wall, Force Shield) costano la revisione di
  privacy.
- **Q4** Il fianco: ti interessa che un colpo alle spalle **faccia più male**, o che **tolga la protezione**?
  Il contratto fa già la seconda cosa; la prima è DR-3.
- **Q5** Il «prototipo C# / .NET» esiste da qualche parte fuori da questo repository? Se sì, la specifica è il
  suo documento e va letta come tale; se no, il §9 va riscritto sui tipi di §12.

## 14. Difetti trovati fuori scope (candidati di follow-up, con evidenza)

- `docs/research/design/grid/RefactorTactics_HexGrid_A4_1cm_per_1_5m.svg` porta «1,5 m» nel nome mentre
  l'authoring lavora con `HexSize = 100`: materiale di ricerca, ma è plausibilmente la fonte del «L = 1,5 m»
  della specifica. Una riga nel README di `grid/` che dichiari la scala **non normativa** eviterebbe la stessa
  deriva alla prossima specifica.
- La tabella «Valori vigenti» di [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) §6 dice
  *«dimensione della cella dall'asset mappa»* senza indicare **dove** si legge: un puntatore a
  `spec-hex-geometry-authoring.md` chiuderebbe la domanda.

## 15. Provenienza, limiti e stati di verifica

- **Misurato su** `main` `8029e80ac`, working tree pulito salvo `docs/research/skill.af` (non tracciato,
  non toccato).
- **Come**: lettura di `AGENTS.md` §1–§5, Decision Log (prime frasi di `D-028`, `D-114`, `D-126`, `D-130`,
  `D-256`, `D-265`, `D-269`, `D-270`, `D-272`, `D-273`, `D-275`, `D-302`, `D-303`, `D-308`, `D-334`, `D-408`,
  `D-576`, `D-582`), ADR-0002/0003/0004/0005/0008, le spec citate per `file:sezione`, i cataloghi di `balance/`,
  e gli header `RTTurnRules.h`, `RTActionDef.h`, `RTCellId.h`, `RTHexCellData.h`, `RTHeroData.h`, `RTUnit.h`
  alle righe citate. I ricalcoli geometrici e percentuali di §4 righe 7 e 32 sono aritmetica del panel.
- **Verificatore avversario**: le premesse delle righe 3, 4, 9, 10, 11, 18, 20, 26, 29, 31, 46 sono state
  sottoposte a un lettore indipendente con contesto fresco e tetto di chiamate; le sue correzioni sono già
  integrate dove ha trovato un errore, e §15-bis ne riporta l'esito.
- **Stati**: Compile `N/A` · Tests `N/A` · Determinism `N/A` · Replay `N/A` · Privacy `N/A` · PIE `N/A` ·
  Packaged `N/A` — nessun codice o asset cambia con questo referto.
- **Misurato dopo la prima stesura** (gli stessi quattro punti che erano `NOT RUN`): (riga 16) lo scambio
  passa, la traversata di un alleato fermo resta **`NOT RUN`**; (riga 31) `Armor|ArmorDEF|DamageResistance|Resistance`
  → zero header in `Source/`; (riga 36) i tag `Status.*` letti da `RTGameplayTags.h`; (riga 40) `bussola|compass`
  in `docs/` fuori da `archive/` → solo `spec-hex-geometry-authoring.md`.
- **`NOT RUN`, dichiarati**: la traversata di una cella occupata da un alleato fermo (riga 16); la lettura dei
  documenti HUD per «timeline multi-traccia» e «ghost» oltre ai puntatori citati (righe 41–42).

### 15-bis. Esito del verificatore avversario

Un agente con contesto fresco, in sola lettura e con tetto di 30 chiamate (ne ha usate 22), ha provato a
smentire quattordici premesse (fasi, `D-114`, posa, facing, elementi, roster, copertura, tipi C++, CP 9.5,
timer, Super, `D-582`, 4v4, Sprint). Esito: **dodici confermate**, una **parziale** e una **confermata con
una sfumatura**, nessuna smentita. Ha trovato **cinque errori di attribuzione**, tutti corretti nel testo
sopra prima del commit:

| Dove | Errore | Correzione |
|---|---|---|
| §4 riga 8 | la formula dei quanti era attribuita alla spec di posa, riga 243 | è di `spec-hex-geometry-authoring.md` riga 243 |
| §4 riga 25 | la frase «non torna» era attribuita a `D-130` | è di `paragon.md` riga 56; `D-130` rimuove il nome |
| §12 | `ERTMovementStyle` elencava `Charge` | l'enumeratore è `LinearCharge` (con `LinearLeap`, `LinearPass`) |
| §4 riga 2 | citava una sezione «PlanningMax» inesistente | §7.1 «Timer massimo + Ready anticipato» |
| §4 riga 6 | `RTCellId.h:11-19` sembrava riferito a `FRTCellId` | le righe 11-19 sono l'enum; la struct inizia alla riga 21 |

Due osservazioni del verificatore restano come note: (a) la spec di posa §7 non nomina un danno da contesa,
e il «nessun danno» poggia su `piano-canonico-mvp.md` §6 («entrambe le unità restano ferme»); (b) la riga 19
di `spec-durata-partita-e-scala-mappe.md` dice ancora *«formato competitivo principale = APERTO»*, stantia
rispetto a `D-256`: candidata di follow-up per quel documento, non di questo referto.


## 16. Esito della sessione di decisioni del 2026-11-08

Le sette `DR` di §11 che l'autore aveva elencato come *«i conflitti»* sono state poste come domande a scelta, con
il testo di ogni opzione davanti; il testo scelto è registrato alla lettera in
[D-583](../../decisions/RT_PDR_00_Decision_Log.md). Le etichette «(Recommended)» erano del panel e sono state
seguite in due casi su sette. Qui la mappa, e ciò che ogni scelta apre.

| DR | Scelta dell'autore | Il contratto | Che cosa apre |
|---|---|---|---|
| `DR-2` fasi | Cinque fasi, reazioni come segmenti | **confermato** (ADR-0003, ADR-0004) | nulla |
| `DR-4` posa | Un solo slot di occupancy | **confermato** (`D-302`, `D-303`, `D-308`) | nulla |
| `DR-1` economia | Riaprire `D-114` verso gli Action Point | **riaperto**: `D-114`, `#604` | `MSP-1`: il modello AP su cinque fasi; `D-265` non riaperta |
| `DR-3` facing | Sei direzioni, più un bonus piatto alle spalle | **emendato**: ADR-0005 (scartata riaperta); `D-126` resta | `MSP-2` (`FAC-17`): valore e lati |
| `DR-5` elementi | Riaprire `D-272`: estendere il vocabolario | **riaperto**: `D-272`; `D-275` resta come regola | `MSP-3`: quali dodici, 66 coppie, il nome `Flux` |
| `DR-6` danno | Moltiplicativa ora, con riscalatura | **deciso in forma**, applicazione a E49 | `MSP-4`: statistiche, «Mod», scala, golden |
| `DR-7` roster | Riaprire `D-321`/`D-334` verso i nomi retail | **riaperto**: `D-321`, `D-334`; `#2297` ha già eseguito le fette B·C·D (`#2491`, 2026-09-10), quindi un nome nuovo è un **secondo rename secco** (`ResolveLegacyActionId` non esiste più, `D-134`) | `MSP-5`: quali otto, mappatura, licenza, ratchet |

Le istruttorie `MSP-1`…`MSP-5` stanno in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Le `DR-8`…`DR-11` di §11
(direzione artistica, timer, copertura percentuale, danno da contesa) **non erano nell'elenco dell'autore** e restano
come erano: aperte, con la raccomandazione del panel.

### 16.1 Coerenza fra le scelte — ciò che il panel segnala all'autore

- **Wiegers**: una riapertura non è un requisito. `DR-1`, `DR-5` e `DR-7` dicono *verso dove* senza dire *che cosa*:
  finché le `MSP-*` non producono un testo accettato, l'unico requisito verificabile è «il contratto attuale resta in
  vigore». Chi legge «AP» o «dodici elementi» in una spec prima di allora legge una `STALE ROADMAP`.
- **Fowler**: `DR-1` cambia il *tipo* della legalità, da strutturale (`D-114`, `D-190`: due principali non si sommano
  perché occupano la stessa cosa) a quantitativa (un totale che sfora). È il cambio più profondo dei sette e tocca
  `ValidatePlan`, il bot e l'HUD insieme; conviene scriverlo come ADR, non come riga di catalogo.
- **Nygard**: `DR-6` è una decisione che oggi non può fallire, perché nessuna statistica di difesa esiste in `Source/`.
  Il primo test che la rende falsificabile è il golden di un colpo con `DEF` nota: senza quello la formula è prosa.
- **Adzic**: `DR-3` e `DR-6` vanno letti insieme. Un bonus *piatto* dentro una formula *moltiplicativa* ha due
  posizioni possibili (prima o dopo `100/(100+DEF)`) con esiti diversi: `MSP-2` deve dire quale, con un esempio
  numerico.
- **Crispin**: le due conferme (`DR-2`, `DR-4`) sono le sole scelte già coperte da test (`RTHexSimTests.cpp`, i test
  di fase). Le cinque riaperture non hanno un test che possa diventare rosso finché il modello non esiste: il primo
  deliverable di ogni `MSP-*` è l'asserto, non il numero.
- **Cockburn**: `DR-7` riapre una scelta presa per una ragione esterna al gioco (la nota IP di `D-321`). La ragione non
  è sparita con la riapertura: `MSP-5` deve rileggerla prima del primo rename, non dopo.

### 16.2 Stati di verifica di questa sezione

Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. Nessun codice, asset o scenario cambia con
`D-583`; cambiano il Decision Log, `OPEN_DECISIONS.md`, l'emendamento in testa ad ADR-0005 e questo referto.
`NOT RUN`: la rilettura della licenza degli asset Paragon (`MSP-5`); la riapertura di `#604` e il riorientamento di
`#2297` su GitHub (scritture non eseguite in questa sessione).