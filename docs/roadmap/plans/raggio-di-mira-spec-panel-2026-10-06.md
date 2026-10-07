# Spec panel — il raggio di mira all'armo di un'azione (`Ability Range`)

> `SNAPSHOT` · fotografia del 2026-10-06: vale finché è l'ultima misura del suo oggetto.

**Data**: 2026-10-06 · **Misurato su**: `main` `903154a41` (ricognizione su `d9a22b75c`); lo stato della portata e dei rilievi aperti è rimisurato su `160c8679d`, dopo il merge di #3513 · **Modalità**: discussion · critique · **Focus**: requirements · architecture · testing
**Panel**: Wiegers (requisiti) · Cockburn (attore/goal) · Adzic (esempi) · Crispin (testabilità) · Fowler (confini) · Nygard (failure mode), più un verificatore avversario delle premesse

> ✅ **Esiti — decisioni d'autore del 2026-10-06 sulle DR di §11.** Il corpo resta la fotografia del panel.
> - **DR-1** → [`D-464`](../../decisions/RT_PDR_00_Decision_Log.md): una sola origine di mira per fase, dalla cella dello scatto per `Attack` e `Control` (#3509).
> - **DR-2** → [`D-465`](../../decisions/RT_PDR_00_Decision_Log.md): per click e portata l'autore ha scelto la (i) dichiarata fino a #2794 — la via che Fowler e Crispin indicavano se non si decide per la v0.1 —, con l'ottimismo per la sola vista dalla destinazione (#3270, #3420).
> - **DR-6** → [`D-466`](../../decisions/RT_PDR_00_Decision_Log.md): il click su una cella vuota si rifiuta **finché il bersaglio non è dichiarato**, non sempre come raccomandato qui (#705). Risponde a **Q1**: il waypoint con un'azione armata resta possibile dopo la dichiarazione, senza disarmo implicito.
> - **DR-8** → nessuna portata finché resta armata un'azione in ricarica; rifiutare l'armo è una decisione a parte. **DR-5** → armare porta il piano attivo a quello da cui si mira; il disarmo (**Q6**) resta aperto. Entrambe in [#3517](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3517), fetta di #1944.
> - **DR-4** → prima la misura sul pacchetto Shipping, poi il canale (#3508).
> - **FR-1** «in ogni altro stato l'armo non produce `R`» e l'armo nel playback di **FR-5** → [`D-468`](../../decisions/RT_PDR_00_Decision_Log.md) (2026-10-07): con `IsWorldReadOnly()` vero nessun ordine da tastiera passa, e il log ne dice la causa (#3510).
> - **DR-14** → confermata la regola di #3513 (commento su #3507): la portata mostra anche le celle coperte, che il click rifiuta col motivo — un'eccezione dichiarata alla lettura «area = click» di D-128. **DR-3** era già decisa in #3507.
> - Restano aperte **DR-7** — su `main` la portata resta accesa dopo la dichiarazione, e la PIE d'autore di #3507 la giudica distinguibile dall'area colpita; si decide insieme a D-466 punto 3 — e le domande **Q2**…**Q6**.

> Referto della sessione chiesta dall'autore il 2026-10-05: *«dobbiamo rendere visibili le aree quando si
> seleziona una abilita'. per esempio una a target singolo, mostra con un area sulla mappa, fin dove riuscirebbe
> a colpire da quella posizione»*.
>
> **Non è un owner.** Le regole restano del Decision Log (D-128, D-364, D-368, D-373, D-393, D-459). La portata è
> [#3507](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3507), della famiglia [#1944](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1944) (`OVL-04`), già implementata dalla PR [#3513](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3513); restano
> aperti [#3509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3509), [#3510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3510) e [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511), e [#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508) è un difetto che la portata
> eredita (in Shipping non si vedrebbe). Le altre aree di contesto stanno nel
> referto gemello [`aree-di-contesto-spec-panel-2026-10-06.md`](aree-di-contesto-spec-panel-2026-10-06.md).
> ⛔ Nessuna riga di codice è cambiata con questo referto.
>
> 🔑 **Dopo il panel è nata #3507**, con due decisioni d'autore del 2026-10-06 su punti che qui erano aperti: la
> portata è viola `#AF52DE` (DR-3) e in targeting il ventaglio verde sparisce (FR-9). #3507 sceglie anche il
> calcolo — in portata le celle `Ok` **e** `NoLineOfSight` — diverso da FR-2 qui sotto (DR-14).
>
> ✅ **E #3507 è già su `main`**: la PR [#3513](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3513), mergiata il 2026-10-06 (`160c8679d`), porta
> `ERTOverlayMeaning::AbilityRange`, il produttore `URTCombatLibrary::TargetableRangeCells` (`Ok` ∪ `NoLineOfSight`
> dalla cella attuale), la sostituzione del ventaglio in targeting e il ridisegno all'armo e al Back; la issue resta
> aperta per la verifica a schermo. Il corpo di questo referto è la misura del panel su `903154a41`; quali rilievi
> #3513 ha chiuso e quali restano aperti lo dice §13, rimisurato su `160c8679d`.
>
> Convenzione: niente totali che cambiano da soli. Le gittate sono dati di catalogo, citati con la loro sede; gli
> esempi di §8 sono ipotesi «da confermare» finché un test headless non li rende verdi sul C++. Le decisioni (DR)
> sono numerate in uno spazio condiviso col referto gemello: DR-1…DR-8 e DR-14 qui, DR-9…DR-13 e DR-15 là. I
> riferimenti al codice citano funzioni, non righe: i commit entrati fra `d9a22b75c` e `903154a41` (#291, #2167)
> hanno spostato le righe di `Player/RTPlayerController.cpp`.

---

## 0. Richiesta

- Utente, 2026-10-05: *«dobbiamo rendere visibili le aree quando si seleziona una abilita'. per esempio una a
  target singolo, mostra con un area sulla mappa, fin dove riuscirebbe a colpire da quella posizione»*.
- Verdetto d'autore in seduta `U60` (2026-10-04, #3459): *«quando si seleziona una skill, si dovrebbe capire
  l'area dove si riesce a colpire. magari con delle aree viola»*.
- Lettura: **B · raggio di mira** — dove l'azione armata *può* essere puntata, prima di scegliere il bersaglio.
  Non è **A · area d'effetto**, che esiste già (`ERTOverlayMeaning::Attack`/`FriendlyFire`, `MakeBlastPreview`,
  `PIE-PREVIEW-AREA`) e compare dopo il bersaglio.

## 1. Obiettivo

Il giocatore che arma un'azione con bersaglio vede sulla board, senza hover né click, fin dove quell'azione
arriva dalla posizione da cui risolverà. Così sa se il nemico che gli interessa è raggiungibile **prima** di
impegnarsi, e decide se colpire, muoversi prima o cambiare azione.

## 2. Attore, precondizioni, garanzie

- **Attore primario**: giocatore umano in `Planning`, con un'unità propria viva e comandabile selezionata.
- **Precondizioni**: mappa esagonale presente; mondo non in sola lettura (`!IsWorldReadOnly()`); input di
  planning non inerte; contesto del puntatore `Targeting`.
- **Garanzia minima**: nessuna mutazione di stato competitivo (`FRTMapState`, snapshot, TurnLog, hash).
- **Garanzia di successo** — formulata per famiglia, perché per il bersaglio unità l'equivalenza
  «cella mostrata ⇔ click accettato» **non esiste** nemmeno in linea di principio (rilievo ADZ-2, verificato):
  in `Targeting` un click su un'unità propria comandabile la **riseleziona** (`ResolveOutcome` → `Select`), la
  cella del tiratore non è un bersaglio, un nemico ignoto dà `NoOp` per privacy, e una cella vuota oggi posa un
  waypoint.
  - **F2 (centro d'area, kind `Cell`)**: `c ∈ R` ⇔ `HandleTargetCell(c)` posa il piano.
  - **F1 (bersaglio unità, kind `Unit`)**: per ogni `c ∈ R`, un nemico **Live, noto e non comandabile** su `c`
    sarebbe accettato da `HandleClickOnUnit`; per ogni `c ∉ R` non lo sarebbe. `R` è la **portata**, non la
    mappa dei click.
  - Entrambe valgono solo con DR-1 chiusa (stessa origine per click e area, #3509) e, per F1, con DR-6 chiusa.
  - ⚠️ Con la regola di #3507 (`Ok` ∪ `NoLineOfSight`, DR-14) `R` è la **portata**, non la mappa dei bersagli
    accettati: le due garanzie valgono per distanza, piano e Fumo, mentre su una cella `NoLineOfSight` dentro `R` il
    click rifiuta per copertura e mostra dove si ferma il tiro (#3085).

## 3. Definizioni

- **Azione in scope** — predicato unico con nome, `ShowsAimRange(Unit, Index)`, vero se valgono **tutti**:
  `TargetKindForAction ∈ {Unit, Cell}` ∧ `Def.Slot != Reaction` ∧ `!bSelfTarget` ∧ `ActionId != Action.Wait`
  ∧ `Def.PredictiveTargeting != LockCell` ∧ non è mobilità rapida ∧ `Unit->CanUseAbility(Index)` (DR-8)
  ∧ l'azione non è già nel piano con un bersaglio (`PlannedAbilityIndex != Index`, DR-7).
  Lo stato armato si legge da `SelectedAbilityIndex`, non da `PlannedAbilityIndex`. Le azioni `Line`
  (`LinearDischarge`, `PressureJet`) stanno in F1: si mira sull'unità.
- **Origine di mira `O`** — la cella da cui l'azione risolverà dato il piano corrente, restituita da **una sola**
  funzione consapevole della fase (§6, FR-4).
- **Raggio di mira `R`** — l'insieme di FR-2, sul solo `Layer` di `O`, ordinato per `FRTCellId`.
- **Famiglie del catalogo spedito**: F1 bersaglio unità · F2 centro d'area su cella (con variante cieca
  `NotRequired`) · F3 bordo · F4 destinazioni di mobilità lineare · F5 cella predittiva · F6 linea Overwatch ·
  F7 nessun raggio. Non esistono nel catalogo: anello min–max, cono, salto/Blink, multi-bersaglio, cross-layer.

## 4. Scope

**Dentro**: F2 subito; F1 con la precondizione DR-6.

**Fuori, con il motivo**:
- F3 (`KineticPanel`, `Reconfigure`, `PortableCover`, `Interact`): `HandleTargetEdge` non ha chiamanti di
  produzione — dipende da #705;
- F4 (`FluidTrail`, `PassingBlade`, `Ram`): è movimento, il contesto non è `Targeting`;
- F5 (`InterceptShot`): la cella predittiva non è dichiarabile dal click (kind `Unit` contro `LockCell`);
- F6 (Overwatch): zona propria (D-387), nessuna mira; F7: niente da mostrare;
- **Ora area E, non più DEFERRED**: l'anteprima d'area d'effetto all'hover (righe `Hover = Preview` di
  `spec-pointer-interaction.md` §5.1) ha un owner, [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512), ed è specificata nel referto gemello (§5);
- **DEFERRED**: evidenziazione dei nemici in portata (andrebbe letta da `FRTKnowledgeView`, mai
  da `Snapshot.Units`); legenda e tooltip della portata (#3419); attenuazione del ventaglio fuori da `Targeting` (in `Targeting` sparisce, FR-9) e resa di
  `Predicted` (nessun consumatore di `AlphaFor`/`PriorityFor`/`Certainty`, #1943); anteprima da posizione
  ipotetica.

## 5. Caso d'uso UC-1 · Capire fin dove colpisco prima di impegnarmi

Livello user-goal. Attore: giocatore in `Planning`.

**Scenario principale**
1. Il giocatore seleziona un'unità propria.
2. Arma un'azione in scope (tasto, slot del dock o generica).
3. La board mostra `R` da `O`; lo slot è `Selected`.
4. Il giocatore vede che il nemico noto sta in `R`.
5. Lo clicca: il piano si accetta, `R` si spegne e subentra l'anteprima A, calcolata dalla **stessa** origine.

**Estensioni**
- 2a. Azione in ricarica → nessun `R`; la ricarica la dice lo slot (DR-8).
- 2b. Playback in corso o turno già confermato → l'armo non produce `R` (FR-1, FR-5).
- 2c. Azione fuori scope (reazione, `Wait`, `Guard`, `Brace`, `Overwatch`, mobilità, bordo, `InterceptShot`) →
  nessun `R`.
- 3a. Scatto già pianificato e azione `Control`/`Attack` → `O` è la cella d'arrivo dello scatto; azione
  `Preparation` → `O` è la cella corrente; azione `Environment` → la cella corrente per scelta dichiarata (FR-4).
- 3b. Linea che tocca il Fumo, estremi inclusi → la portata effettiva su quella linea scende a 2: `R` è un disco
  col buco, non «un disco di 2» (vedi E4–E6).
- 3c. Tiratore su un piano diverso da quello attivo → armare allinea il piano attivo a `Layer(O)` (DR-5).
- 3d. Selettore del verso aperto (#291) → armare lo chiude, poi passo 3.
- 4a. Nessun nemico noto in `R` → il giocatore disarma, cambia azione, oppure si muove e ri-arma. `R` **non**
  cambia coi waypoint: per `Control`/`Attack` il Move risolve dopo il Blast (ADR-0003); per `Environment` la portata
  non si ricontrolla in risoluzione.
- 4b. Nemico nascosto in `R` → `R` identico al caso senza nemico; un click sulla sua cella non pianifica.
- 5a. Click su una cella vuota di `R` con un'azione F1 → `NoOp` con frase di rifiuto (precondizione DR-6; oggi
  posa un waypoint, oppure esce in silenzio se il verso è dichiarato).
- 5b. Click su un nemico fuori da `R` → rifiuto con lo stesso motivo che lo slot già mostra (`Invalid`, D-459).
- 5c. Back o RMB prima del bersaglio → `R` si spegne, **con** ridisegno.

## 6. Requisiti funzionali

- **FR-1 · Innesco.** Al ritorno di `SelectAbilityForCurrent` su un'azione in scope, con `!IsWorldReadOnly()` e
  input non inerte, l'oracolo dell'actor `NumPreviewAbilityRange()` vale `|R|`. In ogni altro stato l'armo non
  produce `R`. Su `903154a41` l'armo di un'azione senza riserva **non** ricalcolava l'anteprima, per scelta dichiarata
  in `SelectAbilityForCurrent`; ✅ #3513 l'ha cambiato: l'armo di un'azione a bersaglio ora ridisegna. ⚠️ La metà
  «in ogni altro stato l'armo non produce `R`» resta aperta: l'armo non ha ancora una guardia di fase (#3510).
  ✅ **Chiusa da #3510** ([`D-468`](../../decisions/RT_PDR_00_Decision_Log.md)): durante la risoluzione l'armo è un no-op, e `HexMatch.PlaybackKeyboardArmIsANoOp` lo prova su tutte le superfici di pianificazione, portata compresa.
- **FR-2 · Contenuto.** Per ogni `c ∈ HexArea(O, Ability->RangeCells)` sul `Layer` di `O` con
  `Map->ContainsCell(c)`: `c ∈ R` ⇔ `DescribeCellTargetRefusal(Map, O, c, Ability->RangeCells,
  Ability->Def.LineOfSightPolicy)` non rifiuta — la domanda che D-459 assegna a un'area, cioè
  `ClassifyHexTargeting == Ok`. Portata = `Ability->RangeCells` **dell'istanza** (dopo le varianti del
  loadout: `Weapon.Impact` porta `PressureJet` a 4 e `ImpactShot` a 2). `R` non legge mai l'occupazione e
  **non** applica alcuna regola di «cella ospitabile», che sarebbe una regola nuova inventata dalla
  presentazione (F-07). Uscita ordinata per `FRTCellId`.
  ⚠️ **CONTRACT CONFLICT con #3507 (DR-14).** #3507 mette in portata le celle il cui verdetto è `Ok` **o**
  `NoLineOfSight`: la portata dice fin dove si arriva, e la linea di vista la dicono il click e il tratto di #3085.
  Il panel aveva raccomandato `Ok` soltanto, per tenere l'area uguale al click (D-128). La scelta di #3507 ha un
  vantaggio che il panel non aveva pesato: senza la LOS, `R` non disegna i buchi dei muri nelle celle mai osservate,
  e del canale di D-373 resta solo il cap del Fumo (#3270). ✅ **Chiuso nei fatti da #3513**, che implementa la regola di
  #3507 e la prova col test `Combat.TargetableRangeCellsAgreeWithTheClickVerdict`.
- **FR-3 · Una sola sede.** `R` lo produce una funzione pura di libreria (`Combat`), senza dipendenze da
  `FRTOverlayArea`. Ingressi: `Map`, `O`, `RangeCells`, `Policy`, più la vista della geometria quando DR-2 sarà
  decisa, che entra **nello stesso commit** per click ed enumeratore. Uscita: celle ordinate. L'adattatore in
  `RefreshPlanningPreview` la converte in `FRTOverlayArea{Meaning = AbilityRange}`. Non si creano subsystem,
  componenti, delegate d'armo, né copie della regola (`BuildCandidates` del bot e
  `AddMainSlotFootprint`/`RegionsFor` esclusi: ignorano policy, MinRange e, il secondo, il Fumo). Quando
  `MinRangeCells` arriverà a un ingresso del giocatore (#3093), arriverà insieme a click su unità, click su
  cella ed enumeratore.
- **FR-4 · Origine** ([#3509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3509)). `O` la
  restituisce **una** funzione pura, estratta dal calcolo del Warning
  (`URTHudViewModel`, che compone `BlastOriginCell` col solo attaccante: EXTEND, non CREATE) e consapevole della
  fase (`MapResolutionPhase`):
  - `Control`/`Attack`: `BlastOriginCell` (cella d'arrivo dello scatto se lo scatto si applica);
  - `Preparation`: cella corrente — risolve prima del Dash;
  - `Environment`: cella corrente **per scelta dichiarata**, non per fase. `MapResolutionPhase` la manda al `Cleanup`,
    dopo il Move, ma il resolver non ne ricontrolla la portata (mira su `PlannedAttackCell` o sulla cella del
    bersaglio): il click è l'unica regola, quindi `O` è la cella da cui giudica il click. È un'eccezione dichiarata
    alla definizione di `O` di §3.
  La consumano senza eccezioni: click su unità, click su cella, stato `Invalid`, stato `Warning`,
  `MakeBlastPreview`, produttore di `R`. Conseguenza da registrare come emendamento a D-459: dopo
  l'allineamento il Warning copre solo il caso «piano cambiato dopo il bersaglio dichiarato».
  **Limite ereditato**: con `LinearCharge` e impatto, `PlannedDashCell` è la cella del nemico e non quella
  d'arrivo (D-296); il caso spedito noto (`Hero.Branth.Ram`) è del bot, quindi resta fuori dagli AC.
- **FR-5 · Ciclo di vita** (reset unico e ingressi: [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511);
  armo durante il playback: [#3510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3510)). `R` è una
  **funzione dello stato**, ricalcolata a ogni `RefreshPlanningPreview`:
  `R ≠ ∅` solo se l'unità selezionata è viva, `GetPointerContext() == Targeting` (che risponde
  `ResolutionPlayback` prima di `Targeting`), `ShowsAimRange` è vero e la mappa esiste. In ogni altro caso il
  produttore **svuota** `R`. Lo spegnimento del ramo `Unit == nullptr` diventa **un solo reset** dell'actor che
  azzera ogni area di anteprima: oggi si fa elencando i setter, e il codice registra tre difetti della stessa
  classe (#2390, #2555, #3064).

  | Evento | Effetto su `R` |
  |---|---|
  | armo in scope | calcolato |
  | armo durante playback, dopo il commit, input inerte | nessun `R` |
  | armo di un'azione in ricarica | `R = ∅` |
  | disarmo da slot, Back (voce `Targeting` dell'ordine di #291), RMB | svuotato, **con** ridisegno |
  | bersaglio dichiarato | svuotato; subentra l'anteprima A dalla stessa origine |
  | cambio di unità verso un'unità senza armo | svuotato |
  | ritorno su un'unità che conserva l'armo (D-397 §5) | ricalcolato |
  | scatto pianificato | ricalcolato da `O'` |
  | waypoint, verso dichiarato, `Sneak` | **invariato** (`Control`/`Attack`: il Move risolve dopo il Blast; `Environment`: portata non ricontrollata) |
  | cambio di piano attivo | invariato, resta su `Layer(O)` |
  | commit, inizio risoluzione o playback, Cleanup, `Modal`, fine partita, KO dell'unità | svuotato |

- **FR-6 · Privacy delle unità.** `R` non dipende da presenza o posizione di nessuna unità. È vero per firma del
  classificatore, quindi si prova **sulla catena** con una mutazione (AC-9), non sulla funzione pura.
- **FR-7 · Geometria mai osservata.** Il classificatore legge muri, porte, coperture alte e Fumo anche nelle
  celle mai osservate, e D-373 dice che il loro **contenuto** non entra nel Planning. La regola si decide per
  **click e area insieme** (DR-2): un filtro sulla sola area sarebbe una seconda autorità e violerebbe D-128.
  Qualunque esito lascia un segnale osservabile (AC-11).
- **FR-8 · Multilivello.** `R` sta sul solo `Layer(O)` (D-393). Armare un'azione in scope allinea il piano attivo
  a `Layer(O)` (DR-5); se il giocatore cambia piano a mano dopo l'armo, `R` resta dov'è e un click altrove riceve
  il rifiuto `OtherLayer` esistente.
- **FR-9 · Composizione.** `R` si disegna e **il ventaglio `Movement` sparisce** finché l'azione resta armata:
  decisione d'autore in #3507 (2026-10-06), che supera l'«invariato» raccomandato dal panel e toglie il bisogno di
  un'attenuazione (#1943). Disarmando, il ventaglio torna. Se l'unità ha già un'azione A pianificata con bersaglio e il giocatore arma B: `R(B)` è il
  significato primario e l'anteprima di A si spegne finché B resta armata (da confermare: armare B revoca il piano
  di A? — §12, Q3).
- **FR-10 · Resa.** Un valore nuovo `ERTOverlayMeaning::AbilityRange`, **in coda** all'enum (il valore
  serializzato è l'indice), entra insieme al suo produttore e alle sue voci in `URTOverlayPalette` (colore, priorità, scala e profondità). Tinta:
  `#AF52DE`, decisa in #3507 (DR-3). Canale: DR-4 (#3508), da decidere per **tutta** l'anteprima di pianificazione. Separazione non cromatica da `Attack`:
  il perimetro (`URTRegionBoundaryLibrary::ExtractBoundaryEdges`, OVL-02); non si crea una terza primitiva di
  perimetro.
  ⚠️ **CONTRACT CONFLICT con un non-goal di #3507**, che dichiara *«Riempimento o materiale delle celle: resta il
  contorno, come gli altri significati»*. Se DR-4 sceglie un canale ISM con riempimento, quel non-goal va emendato;
  altrimenti la portata entra a contorno e migra col resto dell'anteprima dentro #3508. Decide l'autore.
- **FR-11 · Ingressi degeneri.** `RangeCells <= 0` su un'azione in scope → nessun `R` e una riga di log (lo zero
  ha tre significati, e «eredita la portata del portatore» vale nel resolver ma non nel click). Unità non viva →
  `R` vuoto. Mappa assente → `R` vuoto (fail-closed). Un test di catalogo garantisce che ogni azione in scope
  dichiari `RangeCells > 0`.

## 7. Requisiti non funzionali

- **NFR-1 Presentation-only**: nessuna mutazione; si estende `AreaOverlay.DrawingMutatesNothing`.
- **NFR-2 Costo**: il calcolo avviene in `RefreshPlanningPreview`, che gira su evento; al più una chiamata del
  classificatore per cella di `HexArea(O, RangeCells)`. Niente calcolo per Tick.
- **NFR-3 Accessibilità**: un secondo canale non cromatico è obbligatorio (D-146). Lo impone la misura:
  `#AF52DE` e `Attack #FF453A` hanno luma quasi uguale (Δ ≈ 2,4, Rec.601), e nessun gate misura la luma
  sugli overlay.
- **NFR-4 Build distribuita** ([#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508)): l'area
  usa lo stesso canale del resto dell'anteprima (renderer unico di OVL-01).
  ⚠️ `DrawDebugLine` è una funzione **vuota** in Shipping/Test senza editor:
  `UE_ENABLE_DEBUG_DRAWING = !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR` (`EngineDefines.h`), e le
  versioni vuote stanno in `DrawDebugHelpers.h`; i `Target.cs` del progetto non ridefiniscono la macro.
  Finché DR-4 non è decisa, NFR-4 è `NOT RUN` e non è un criterio d'accettazione.

## 8. Esempi-chiave

Coordinate `(X, Y, Layer)`. Tutti gli esiti sono **ipotesi da confermare**: ricavati a mano e con un emulatore
locale non versionato, non hanno una fonte rieseguibile; ogni riga diventa «confermata» solo col suo test headless
verde sul C++. ⚠️ La colonna «Fuori da `R`» elenca anche celle `NoLineOfSight`: con la regola
di #3507 (DR-14) quelle celle **sono** in `R`, e a escluderle resta il click.

| # | Fixture | Azione (portata, policy) | `O` | Dentro `R` | Fuori da `R` (motivo) |
|---|---|---|---|---|---|
| E1 portata | `MakeTestArena` | Aevik `ArcPulse` (4, Required) | (-4,0,0) | (-4,4,0) d4 · (0,-4,0) d4 | (-3,4,0) OutOfRange d5 |
| E2 LOS | `MakeTestArena` | `ArcPulse` | (-1,0,0) | (0,0,0), cella del muro alla vista (estremo escluso dalla LOS) · (-2,0,0) | (1,0,0) · (0,2,0) · (1,2,0) NoLineOfSight |
| E3 bordo | `MakeCoverYardArena` | `ArcPulse` | (-1,0,0) | (0,0,0) · (1,1,0) | (1,0,0) · (2,0,0) NoLineOfSight (bordo alto) |
| E4 Fumo, origine fuori | `MakeShowcaseRelayLiteArena` | `ArcPulse` | (1,4,0) | (1,2,0) d2 · (2,2,0) | (1,1,0) d3 · (1,0,0) d4 OutOfRange (portata effettiva 2) |
| E5 Fumo, origine dentro | RelayLite | `ArcPulse` | (1,2,0) | (3,2,0) d2 · (1,0,0) d2 | (4,1,0) d3 OutOfRange |
| E6 Fumo, bersaglio dentro | RelayLite | `ArcPulse` | (-1,1,0) | (-1,-1,0) d2 | (-1,-2,0) d3 OutOfRange |
| E7 piano | `MakeTestArena` | `ArcPulse` | (2,0,1) | esattamente (2,-1,1) (2,0,1) (3,-1,1) (3,0,1) | (2,0,0) · (1,0,0) OtherLayer; dopo l'armo `ActiveLayer == 1` |
| E8 cieca | `MakeTestArena` | Muiren `MistVeil` (4, NotRequired) | (-2,0,0) | (1,0,0) e (2,-1,0) dietro il muro | (2,1,0) OutOfRange d5 |
| E9 controllo di E8 | `MakeTestArena` | Aevik `Overload` (3, Required) | (-2,1,0) | (0,0,0) | (1,0,0) NoLineOfSight |
| E10 scatto, fase Attack | `MakeTestArena` | `FluidTrail` (-1,0)→(2,0), poi `PressureJet` (4) | (2,0,0) | (4,0,0) · (4,-1,0) | (-2,0,0) NoLineOfSight. Con l'origine `Unit->Cell` gli esiti si invertono |
| E11 scatto, fase Environment | `MakeTestArena` | stesso scatto, poi `Gadget.Sprinkler` (4) | (-1,0,0), per scelta (FR-4) | (-4,0,0) | (4,-1,0) OutOfRange |
| E12 carica | — | Branth `Ram` | — | esclusa finché `PlannedDashCell` non è corretto (FR-4, §14; dentro #3509) | — |
| E13 impercorribile | `MakeVisionSplitArena` | `ArcPulse` | (-2,-1,0) | (-2,1,0), con `bBlocksMovement`: resta in `R` (FR-2) | (-2,2,0) NoLineOfSight |
| E14 ricarica | `MakeTestArena` | `ArcPulse` in ricarica | (-1,0,0) | `R = ∅` (DR-8) | — |
| E15 tasca buia | `MakeFlatArena` r4, muro in una cella mai osservata | `ArcPulse` | (0,0,0) | con DR-2 (ii): (3,0,0) (3,1,0) (4,-1,0) (4,0,0) in entrambi i mondi | con DR-2 (i) quelle celle escono: è il leak. Con la regola di #3507 (DR-14) il muro non conta più, e il caso da provare è un Fumo nel buio (#3270) |
| E16 esclusioni | qualunque | `Guard`, `Brace`, `Overwatch`, una reazione, `Wait`, `InterceptShot`, `FluidTrail`, `Interact` | — | `NumPreviewAbilityRange() == 0` | — |

## 9. Criteri di accettazione

Ognuno binario, ognuno col suo oracolo in §10.

- **AC-1** Le righe E1–E11 ed E13 valgono sul C++ (celle scritte a mano). L'azione con LOS richiesta si sceglie
  leggendo `LineOfSightPolicy == Required` dal `Def` (oggi `ArcPulse`): se entra la PR #3230, che rende cieco
  l'attacco base, se ne sceglie un'altra invece di vedere AC-1 rosso per una ragione estranea. Con la regola di #3507 (DR-14) le
  celle `NoLineOfSight` delle colonne «Fuori» di E2, E3, E9, E10 ed E13 passano «Dentro».
- **AC-2 (F2, click vero)** Per ogni `c ∈ HexArea(O, RangeCells + 1)` sul piano di `O`, `HandleTargetCell(c)` posa
  il piano ⇔ `c ∈ R`. L'anello in più prova il bordo. Con la regola di #3507 (DR-14) l'equivalenza vale sulle celle
  non `NoLineOfSight`; su quelle il click rifiuta e disegna il tratto di #3085.
- **AC-3 (F1, click vero)** Con un nemico Live noto posato su `c`, `HandleClickOnUnit` lo accetta ⇔ `c ∈ R`; con
  lo stesso nemico ignoto, nessun piano e nessuna differenza in `R`. Con la regola di #3507 l'equivalenza vale sulle
  celle non `NoLineOfSight`.
- **AC-4 (origine)** E10 ed E11 con DR-1 allineato: click, `R`, anteprima A e Warning usano la stessa origine.
- **AC-5 (scope)** Un test itera ogni azione spedita (kit, generiche, loadout di default) e asserisce in/out di
  `ShowsAimRange` contro una tabella attesa scritta a mano.
- **AC-6 (ciclo di vita)** Ogni riga della tabella di FR-5 è un passo del banco `FRTLockInPreviewBench`, con
  `NumPreviewAbilityRange()` atteso; dopo ogni uscita `NumPreview(m) == 0` per **ogni** significato di
  pianificazione in `AllMeanings`.
- **AC-7 (playback)** Dopo il commit, un tasto abilità durante il playback lascia `NumPreviewAbilityRange() == 0`
  e non riaccende nessun'altra superficie.
- **AC-8 (piano attivo)** E7: dopo l'armo `ActiveLayer == Layer(O)`, e un click su una cella di `R` per un'azione
  ad area produce un piano, non il log «Su un altro piano».
- **AC-9 (privacy unità, sulla catena)** `SelectUnit → ArmKitAbility → R` identico con e senza nemici nascosti
  in portata (mondi a tre, `ThreeWorldsAgree*`). La mutazione «il produttore filtra per `Snapshot.Units`» lo
  deve far diventare rosso.
- **AC-10 (ricarica, degeneri)** E14; `RangeCells <= 0`, unità morta, mappa assente → `R` vuoto, e nel primo caso
  una riga di log.
- **AC-11 (tasca buia)** E15: l'esito sulle celle illuminate è quello dichiarato da DR-2 — con (i) verde perché
  il canale c'è, sul modello di `BlindActions.NeverObservedWallBendsTheLitPath`; con (ii) o (iii) `R` invariato. Con la
  regola di #3507 il muro nel buio non cambia `R`: AC-11 si prova col Fumo.
- **AC-12 (presentation-only)** Hash dell'asset e stato competitivo invariati dopo il disegno.
- **AC-13 (PIE)** La voce di §10 ha un verdetto registrato.
- **AC-14 (pacchetto)** `R` visibile nel pacchetto Shipping in stage — vale solo dopo DR-4.

## 10. Piano di verifica

**Tre oracoli, nessuno uguale al produttore.** Confrontare il produttore col classificatore che il produttore
stesso chiama è una tautologia: resterebbe verde con l'origine sbagliata, con la portata del `Def` invece dello
specchio, senza la guardia di ricarica (CRI-1).
1. **Celle scritte a mano** sulle fixture spedite (§8).
2. **Il click vero** (AC-2, AC-3).
3. **Il resolver**, per le azioni `Attack` con DR-1 allineato: un bersaglio su `c` viene colpito da
   `CollectHexAttacks` ⇔ `c ∈ R`. È l'oracolo che vede la divergenza d'origine (E10).

**Catena**: `RefreshPlanningPreview` vive in un namespace anonimo, quindi si prova dal controller — banco
`FSlotRefusalBench` su `MakeTestArena`, `SelectUnit` (non `SelectActorForTest`), `ArmKitAbility`, poi
`NumPreviewAbilityRange()`/`IsPreviewAbilityRange(c)` sull'actor.

**Mutazioni, sulla riga del CHIAMANTE**, ognuna col test che la uccide: origine `Unit->Cell` al posto della
funzione per fase (E10) · policy omessa, che ricade sul default `Required` (E8; con la regola di #3507 diventa **equivalente**, perché le
celle `NoLineOfSight` entrano comunque) · `Def.RangeCells` al posto
dello specchio (E10, solo su fixture **con** loadout `Weapon.Impact`: va dichiarato) · filtro `ContainsCell`
tolto · guardia di ricarica tolta (E14) · un congiunto di `ShowsAimRange` tolto (E16) · svuotamento nel ramo
`nullptr` tolto (AC-6). Il filtro di `Layer` è una mutazione **equivalente** (il classificatore dà già
`OtherLayer`): si dichiara tale invece di contarla.

**Ciclo di vita**: prima si estende `PreviewCellsLit` del banco alla superficie nuova, altrimenti ogni test di
spegnimento è cieco.

**PIE — una voce, una domanda.** Da `L_DevSandbox`, `rt.Test.Scenario <fixture con Muiren e un muro alla vista>`.
L'allestimento `previewUnit` oggi prepara solo la lettura A (scarta i bersagli a cella e clicca sempre il
bersaglio): va esteso perché si fermi dopo l'armo, altrimenti la ricetta è manuale. Domanda, contata per tasti:
*«Con Muiren selezionata, dopo il primo tasto dello slot di `MistVeil` e senza muovere il mouse, gli esagoni
oltre il muro alla vista sono evidenziati come quelli davanti al muro?»* — **verde**: sì, nessun buco dietro il
muro; **rosso**: buchi dietro il muro, oppure nessuna area. Verdetto in `docs/technical/test-manuali-pie.md`;
seduta in `docs/roadmap/editor-sessions.yaml` sotto `sessions:` (accodata in fondo finirebbe in
`not_schedulable:` senza errore). ⚠️ Con la regola di #3507 (DR-14) questa domanda non distingue più la policy — le
celle dietro il muro sono in portata per ogni azione — e va sostituita da una sul Fumo: *«armato `ArcPulse` vicino al
Fumo di `MakeShowcaseRelayLiteArena`, la portata si accorcia sulle linee che lo attraversano?»*

**Stati attesi a fine implementazione**: Compile, Tests, Privacy (headless) misurabili · PIE dopo la seduta ·
**Packaged `NOT RUN`** finché DR-4 non è decisa e una seduta sul pacchetto Shipping (modello `U17`) non ha dato
il verdetto: nessun test sul modello prova che l'area arrivi allo schermo.

## 11. Decisioni richieste (BLOCKED — DECISION REQUIRED)

Decide l'autore. Il registro è un `D-nnn` nel Decision Log più un commento sulla issue indicata. L'ordine è
quello della tabella.

| DR | Domanda | Raccomandazione del panel | Registro | Chiusura binaria |
|---|---|---|---|---|
| **DR-6** | Click su cella vuota in `Targeting/Unit` | **Chiudere la deriva da §5.1** (`NoOp` con frase di rifiuto) come prerequisito di F1. Dissenso (Adzic): F1 come portata informativa, con #705 indipendente | `D-nnn` + #705 | in `Targeting/Unit` su cella vuota `PlannedWaypoints` resta invariato |
| **DR-1** | Origine | **Una funzione d'origine per fase**, consumata da click, `Invalid`, Warning, anteprima A e `R`, in una issue propria **prima** della fetta di #1944, con emendamento a D-459 (unanime) | `D-nnn` + #3509 | E10/E11 verdi su click, `R`, anteprima A e Warning |
| **DR-2** | Geometria mai osservata, per click **e** area | Tre opzioni reali: **(i)** mappa piena per entrambi, leak dichiarato con canary · **(ii)** ottimistica per entrambi come D-372 (si mira, il resolver può rifiutare) · **(iii)** solo noto per entrambi (non si mira oltre il buio: cambia la semantica). «Solo noto sulla sola area» è dominata: lascia aperto il canale indiretto. Fowler e Crispin: **(ii)** se si decide per la v0.1, altrimenti **(i)** dichiarata fino a #2794 (v0.3). ⚠️ Con la regola di #3507 (DR-14) l'area perde il canale dei muri e resta quello del Fumo | `D-nnn` in #3270 (allargata alla LOS) o #2791 | AC-11 con l'esito dichiarato |
| **DR-4** | Canale di resa | Deciderlo per **tutta** l'anteprima di pianificazione: **ISM con riempimento** (precedente `PlanGhosts`; `M_HexOverlayFill` è su `main` e inerte), con un oracolo headless sul numero di istanze. In alternativa dichiarare Development come build di release, il che contraddice G12, D-214 e i documenti che chiamano Shipping la build distribuita. Prima di tutto una misura: giocare il pacchetto Shipping. ⚠️ Il riempimento contraddice il non-goal «resta il contorno» di #3507 (FR-10) | `D-nnn` + #3508 | AC-14 |
| **DR-3** | Tinta | ✅ **DECISA dall'autore in #3507** (2026-10-06): `#AF52DE`, il valore che D-364 §4 determina per riferimento; il rosso resta all'area colpita. Restano gli obblighi del panel: un canale non cromatico contro il collasso di luma con `Attack` (Δ ≈ 2,4), e la divergenza dichiarata dal viola Reaction dello Screen HUD (`#7C5CFF`, D-234 §4) | #3507 | `PaletteIsDistinguishable` verde col significato nuovo (DoD di #3507) |
| **DR-5** | Piano attivo | **Armare allinea il piano attivo a `Layer(O)`** (unanime). Resta da decidere se al disarmo torna il piano di prima | commento #1944 | AC-8 |
| **DR-7** | Dopo il bersaglio | **`R` si spegne** e subentra l'anteprima A: l'attenuazione non ha un canale. Dissenso (Adzic): attenuata se il contesto resta `Targeting` e un secondo click ridichiara (che il contesto resti `Targeting` è verificato sul percorso cella — referto gemello §2 —, da verificare sul percorso unità) | commento #1944 | AC-6 |
| **DR-8** | Azione in ricarica | **`R = ∅`**: lo slot dice già la ricarica, e un'area piena su cui nessun click viene accettato viola D-128 (unanime) | commento #1944 | E14 |
| **DR-14** | Portata o mira | ✅ **Chiusa nei fatti da #3513**: implementa `Ok` ∪ `NoLineOfSight`, come lo scope di #3507, e lo prova cella per cella con `Combat.TargetableRangeCellsAgreeWithTheClickVerdict` (la portata concorda col verdetto del click, e almeno una cella in portata il click la rifiuta per copertura). Il panel aveva raccomandato `Ok` soltanto (area = click, D-128) e non la contesta: la scelta toglie il canale dei muri nel buio. Da registrare come decisione se l'autore la conferma | #3507 | il test citato |

## 12. Domande aperte per l'autore

- **Q1** Chiudere §5.1 toglie un gesto che oggi esiste: posare un waypoint con un'azione armata. È una perdita
  accettata, o serve un disarmo implicito?
- **Q2** La build di release v0.1 è Shipping? La risposta decide DR-4 per tutta l'anteprima, non solo per l'area.
- **Q3** Armare B quando A è già pianificata con bersaglio revoca il piano di A? Se no, quale anteprima prevale?
- **Q4** Il click deve continuare ad accettare come centro d'area una cella-muro (G1, E13)? Oggi la accetta, e `R`
  la segue.
- **Q5** `Gadget.Sprinkler`, `ConductiveNode` e `Feint` hanno kind `Unit` ma agiscono su celle o non hanno
  effetto: restano in F1 come il click li tratta oggi, o si corregge `TargetKindForAction` (questione di
  contratto, non di presentazione)?
- **Q6** Con DR-5, al disarmo il piano attivo torna a quello scelto dal giocatore?

## 13. Sequenza e ownership — stato su `160c8679d`

Ogni issue ha un owner solo; le altre relazioni sono dipendenze.

1. ✅ **La portata è su `main`** ([#3513](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3513), mergiata il 2026-10-06), con [#3507](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3507) aperta per la
   verifica a schermo. Rilievi di questo referto che #3513 ha **chiuso**: l'innesco all'armo (FR-1) e lo spegnimento
   col Back e col disarmo (FR-5), la sostituzione del ventaglio (FR-9), la palette (DR-3), la regola `Ok` ∪
   `NoLineOfSight` (DR-14), l'esclusione di scatti e reazioni, il test di concordanza col click e le mutazioni sulla
   riga del chiamante.
2. **Rilievi che restano aperti**, verificati su `160c8679d` e riassunti in un commento su #3507:
   - azione in ricarica: la portata si mostra piena e il click la rifiuta, perché il produttore non guarda
     `CanUseAbility` (DR-8);
   - azioni a portata 0 che chiedono un bersaglio — `Action.Wait` — accendono la portata della sola cella dell'unità e
     spengono il ventaglio (FR-11; dedotto leggendo il codice, non misurato);
   - origine con uno scatto pianificato: limite dichiarato da #3513 → [#3509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3509) (DR-1);
   - armo durante il playback: ora che l'armo ridisegna, il rischio è concreto → [#3510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3510);
   - un solo ingresso e un solo reset per le aree → [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511);
   - piano attivo non allineato al tiratore (DR-5);
   - `NoOp` su cella vuota in `Targeting/Unit` — fetta di #705, dopo la decisione DR-6 (solo per F1).
   ⚠️ **Non è un prerequisito, ma un difetto ereditato**: la portata è disegnata con `DrawDebugLine`, vuota in Shipping
   → [#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508) (DR-4). Finché il canale non è deciso, `Packaged: NOT RUN` con questo motivo.
   ℹ️ Dopo la dichiarazione del bersaglio la portata **resta accesa** finché l'azione è armata, annidata nell'area
   colpita (scala `0,60`): è la composizione scelta da #3513, e supera la raccomandazione di DR-7. Che l'azione resti
   armata dopo la dichiarazione è verificato sul percorso cella, non su quello unità.
3. **[#3459](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3459)**: chiusa come duplicato istruito di #3507, con un rimando a questo referto.
4. **DEFERRED**: vedi §4.

## 14. Difetti trovati fuori scope (candidati di follow-up, con evidenza)

- 🔴 **Tutta l'anteprima di pianificazione è probabilmente invisibile in Shipping**: ventaglio, rotta, area
  d'effetto e linea di tiro passano da `DrawDebugLine`, vuota in Shipping/Test senza editor (`EngineDefines.h`,
  `DrawDebugHelpers.h` di UE 5.8). #1944 prevede già un riempimento che sostituisca `DrawDebugLine`, ma per
  l'alpha e per il costo; nessuna issue registrava il caso Shipping: ora è [#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508). **Non misurato su un binario.**
- L'anteprima d'area parte dalla cella del nemico dopo una carica, mentre l'arrivo reale è la cella adiacente
  (`PlannedDashCell` contro `ResolveLinearMove`, D-296) — dentro #3509.
- `BlastOriginCell` ignora la fase: un'azione `Environment` o `Preparation` dopo uno scatto viene anteprimata
  da una cella diversa da quella da cui il click la giudica (`RefreshPlanningPreview` applica `BlastOriginCell` a qualunque azione pianificata) — dentro #3509.
- Si arma durante la risoluzione e il playback: `SelectAbilityForCurrent` non ha una guardia di fase — #3510.
  ✅ Chiuso da #3510 ([`D-468`](../../decisions/RT_PDR_00_Decision_Log.md)), insieme a `Sneak` e `Invio`, che avevano la stessa forma.
- `InterceptShot` non è dichiarabile dal click (kind `Unit`, ma vuole una cella): l'abilità si consuma senza
  armarsi.
- Commenti stantii: `RTOverlayPalette.cpp` dice che un significato nuovo «rompe la COMPILAZIONE» (falso con i
  default di UBT 5.8); `RTOverlayPalette.h` e il corpo di #1944 dicono che `DrawDebugLine` ignora l'alpha (falso
  per le linee spesse, che passano da `SE_BLEND_AlphaBlend`; non verificato a schermo); `RTOverlayArea.h` dice
  «Sei tinte / Sono SEI», scritto prima che `Vision` entrasse nell'enum.
- `PIE-PREVIEW-AREA` resta ✅ benché D-364/D-368 ne prescrivano la rimisura.
- `RTEnemyTacticalQuery.h` tratta il terreno come pubblico, contro D-373 (CONTRACT CONFLICT).
- Il resolver non applica `OtherLayer` (`ValidateInstance` considera solo `NoLineOfSight`).

⚠️ **Le voci senza numero non sono state aperte come issue**: sono difetti con evidenza ma fuori dal percorso di
questa feature, e restano citabili qui finché un owner non le prende.

## 15. Provenienza e limiti

- Ricognizione: otto lettori indipendenti più un critico su `d9a22b75c`; premesse centrali riverificate dal
  coordinatore su `903154a41` (armo senza riserva, origine del click, click su cella vuota in `Targeting/Unit`,
  righe di §5.1, no-op di `DrawDebugLine` sugli header dell'Engine, D-128/D-373/D-364/D-368/D-376/D-393/D-459).
- Panel: tre coppie di esperti più un verificatore avversario. Fra i rilievi CRITICAL, il verificatore ha
  confermato sul codice tutte le premesse fattuali tranne quella del no-op in Shipping (F-05): non aveva accesso
  agli header dell'Engine, e il coordinatore l'ha verificata direttamente. CRI-1 (oracolo vacuo) è un rilievo di
  metodo senza premessa fattuale. Alcune premesse MAJOR restano parziali o non verificabili per esaurimento del
  budget, ed è dichiarato in ciascun rilievo. Le portate col loadout di default (`PressureJet` 4, `ImpactShot` 2)
  vengono dalla ricognizione e il verificatore non le ha riprodotte.
- Gli esempi di §8 sono ipotesi senza una fonte rieseguibile (ricavati a mano e con un emulatore locale non
  versionato): valgono finché un test headless non li conferma o li smentisce.
- Nessun codice è stato modificato e nessun gate è stato eseguito: Compile, Tests, Determinism, Replay,
  Privacy, PIE e Packaged sono tutti `N/A` per questo passaggio.
