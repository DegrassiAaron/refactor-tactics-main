# L'ispettore del mondo — sguardo, scheda e fuoco su unità, terreno e oggetti

> **Statuto**: design **proposto** il 2026-10-13, da una sessione di brainstorming con l'autore. Le decisioni d'autore
> prese in sessione sono in §0 (`S1`–`S7`); le tre sezioni del disegno sono state approvate in sessione una per volta.
> **In attesa della rilettura dell'autore su questo file**: fino ad allora non esiste un piano di implementazione.
> È la **prima di quattro fette** della richiesta d'autore *«dobbiamo aumentare il livello informativo di un giocatore,
> quante informazioni può vedere col mouse? click e altre funzioni del mouse su target (alleato, nemico, oggetto,
> terreno, …)»*. Le altre tre stanno in §10.
>
> **Owner che questa spec consuma, e non sostituisce**: il contratto del puntatore,
> [`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md)
> ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705), CP 11.8, epic E11); l'HUD,
> [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §18
> ([#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613)); la camera,
> [`spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md) §3.2. Dove questa spec chiede di
> cambiarli lo dice in §9, e il cambio avviene nei loro file, col lavoro.
>
> **Stato misurato**: ricognizione su `origin/main` = `77146b271`, riletta su `4e6d198cd` il 2026-10-13. Fra i due
> commit cambiano `UI/RTHudViewModel.*` ([#3629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3629))
> e `Perception/RTPerceptionLibrary.cpp` ([#3741](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3741),
> [D-510](../../decisions/RT_PDR_00_Decision_Log.md)). Tutto è letto dal codice: **nessuna verifica a schermo**
> (PIE `NOT RUN`). I fatti sono citati per **simbolo**, non per riga; chi rilegge più tardi rimisura. Nessun totale
> volatile: dove serve un elenco ci sono i nomi. Le scelte di chi scrive sono marcate `Ruling` (§8), ognuna col suo
> costo se è sbagliata, e sono revocabili dall'autore.

---

## 0. Decisioni d'autore prese in sessione

`S` sta per «sessione»: non sono voci del Decision Log. Quelle che devono diventarlo sono in §9.3.

| | Decisione | Come è stata presa |
|---|---|---|
| **S1** | Servono tutti e quattro gli scopi — leggere il campo, prevedere il gesto, capire i perché, leggere la squadra — *«e anche le skill del nemico»* | risposta libera alla prima domanda |
| **S2** | Di un nemico osservato si leggono **kit dell'eroe**, **equipaggiamento** e **ricariche dedotte**. Le ricariche reali restano fuori | scelta multipla; l'opzione «ricariche reali» non è stata selezionata |
| **S3** | Forma: **sguardo** breve all'hover, **scheda** completa in un pannello fisso | scelta fra tre forme, con anteprima |
| **S4** | Lo sguardo è **UI nel mondo**, ancorato al bersaglio: [D-497](../../decisions/RT_PDR_00_Decision_Log.md) resta com'è | scelta fra tre classificazioni |
| **S5** | *«aggiungi ctrl + LMB per centrare la telecamera sul personaggio, visualizza l'area di movimento e, se mouse passa su skill, attiva le varie informazioni sui esagoni raggiungibili»* | aggiunta d'autore alla sezione 1 |
| **S6** | *«sostituisci alt con ctrl»* | risposta alla domanda sulla sovrapposizione con `Alt` + click |
| **S7** | Le tre sezioni del disegno — gesti e contenuti, privacy e architettura, superfici test e taglio — sono approvate | *«si torna»*, *«ok»* |

---

## 1. Stato misurato

### 1.1 Che cosa fa il mouse oggi

| Bersaglio | Hover | `LMB` |
|---|---|---|
| Unità che il giocatore comanda | l'esagono evidenziato (`ARTHexMapActor::SetHoveredCell`) | seleziona; il doppio click seleziona e inquadra (`FocusCameraOnUnit`) |
| Unità non comandata, **osservata** | l'esagono evidenziato | fuori da `Targeting` è `Inspect` ([D-128](../../decisions/RT_PDR_00_Decision_Log.md)): `ARTPlayerController::DispatchUnitClick` scrive `InspectedUnit` |
| Nemica **non osservata** | come la cella su cui sta: la collisione dell'unità è spenta e il raggio cade sulla cella | come la cella ([D-466](../../decisions/RT_PDR_00_Decision_Log.md)) |
| Cella | l'esagono evidenziato; in `Targeting` anche lo stato `Invalid` dello slot | waypoint, bersaglio a terra o verso, secondo il contesto |
| Porta, ponte, obiettivo | nulla di proprio | il raycast sa solo `FRTPointerCandidates::bMapElement`, un booleano |
| Slot azione dell'HUD | tooltip (`URTHudViewModel::BuildActionTooltip`): frase d'autore, numeri, motivo se spento | arma l'azione |

Gli altri gesti: `RMB` è il Back a livelli di `URTPointerLibrary::ResolveBack`; la rotella zooma; il tasto centrale
orbita. Il modificatore della camera è `Alt` (`CameraModifierAction`), con la famiglia di
`spec-tactical-camera.md` §3.2: `Alt` + `LMB` click posa il pivot sulla cella, `Alt` + `LMB` trascinato orbita,
`Alt` + `RMB` trascinato fa il dolly, `Alt` + movimento è il peek, `Alt` + `MMB` trascinato è il pan di precisione.
`Shift` e `Ctrl` non sono mappati.

### 1.2 I buchi

- **Sul mondo l'hover non produce mai testo.** L'unico tooltip del gioco sta sugli slot azione.
- **L'ispezione di un nemico quasi non si vede.** In C++ `ARTPlayerController::GetInspectedUnit` ha un solo
  lettore, `URTSelectedUnitPanelWidget::GetSubject`, che lo usa solo se nessuna unità è comandata: *«il comando
  vince sull'ispezione»*. Con un'unità selezionata — lo stato normale della pianificazione — il click su un
  nemico non cambia nulla a schermo. Quando si vede, `FRTUnitCardView` porta nome, PV e scudo.
  ⚠️ La funzione è `BlueprintPure`: un widget Blueprint potrebbe leggerla altrove. Non misurato.
- **L'area di movimento esiste solo per l'unità comandata**, e risponde a «quanto le resta» dopo i waypoint posati
  (`RefreshPlanningPreview`, significato `ERTOverlayMeaning::Movement`).
- **La portata di un'azione si vede solo ad azione armata**
  ([#3507](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3507), `ERTOverlayMeaning::AbilityRange`).
- **Stati, superfici e ripari non hanno un testo.** Gli stati sono tag e icona (`FRTStatusBadgeView`); un nome
  leggibile e un effetto in una riga non li produce nessuno. Lo stesso vale per `ERTHexSurface` e
  `ERTHexCoverType`.
- **L'unità non conserva gli id dei pezzi equipaggiati.** `ARTUnit::EquipLoadout` applica la variante d'arma
  all'attacco base e accoda le azioni concesse da gadget e modulo, poi li dimentica.
- **Il registro a schermo copre solo l'ultimo turno** (`URTHudViewModel::BuildPlayerEventFeed`, tetto
  `MaxFeedLines`).

### 1.3 Che cosa è già scritto e non ha un produttore

- La colonna **Hover** della matrice di `spec-pointer-interaction.md` §5.1: quasi ogni riga dice `Inspect` o
  `Preview`. L'unica fetta aperta è l'area d'effetto
  ([#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512)).
- `progettazione-hud.md` §18, *Target / Hover Inspector*: nome, PV, scudo, stati visibili, copertura, quota,
  verso pubblico, resistenze pubbliche, relazione col bersaglio corrente.
- L'inspector «pinnato»: `ARTPlayerController::SetInspectorPinned` e il passo `ERTPointerBackStep::Inspector`
  esistono, e nessuno li accende.

### 1.4 Che cosa si riusa

| Serve | Esiste |
|---|---|
| sapere chi è noto a un osservatore | `FRTKnowledgeView` (`URTKnowledgeViewLibrary`): un soggetto ignoto **non ha una voce**; `ARTUnit::IsKnownToObserver`, scritto dal velo |
| una vista che non può portare dati grezzi | la disciplina di `FRTContextInspectorView`, provata per reflection |
| uno snapshot per osservatore | `URTHexSimLibrary::MakeSnapshot` e `AsObserver` |
| le celle raggiungibili | `URTHexSimLibrary::ReachableCells(Snapshot, UnitId)` |
| la portata di un'azione | `URTCombatLibrary::ClassifyHexTargeting`: riceve due celle e una mappa, non le unità |
| le righe di una skill | `URTHudViewModel::BuildActionTooltip`, `RTActionDescriptions::For` |
| gli eventi autorizzati a un osservatore | `URTPlayerEventProjector`, `FRTPlayerEvent` |
| la targa sopra l'unità | `URTUnitOverlayWidget`, `FRTUnitOverlayView` ([D-320](../../decisions/RT_PDR_00_Decision_Log.md)) |
| le fasce dell'HUD e il rettangolo della scena | `FRTHudBands`, `RTHudLayout` ([D-497](../../decisions/RT_PDR_00_Decision_Log.md)) |
| lo stile «non certo» sulla mappa | `Predicted` e `Uncertain` in `Map/RTOverlayArea.h` |

---

## 2. Perimetro

**Il principio che separa le fette.** Questa fetta risponde a *«che cos'è questa cosa»*. La fetta 2 risponde a
*«che cosa le fa il mio gesto»*. Portata rispetto all'unità comandata, copertura sul lato del tiro ed esito atteso
restano alla fetta 2 e atterrano sulle stesse superfici.

**Dentro:**

- lo **sguardo** all'hover su unità osservata, cella, porta, riparo, obiettivo;
- la **scheda** fissata, per gli stessi bersagli;
- il **fuoco**: camera, scheda e area di movimento di un personaggio;
- la **portata all'hover di una skill**, dalla barra o dalla scheda;
- il cambio di modificatore della camera da `Alt` a `Ctrl`;
- il **glossario** di stati, superfici e ripari.

**Fuori**, con il motivo:

| Fuori | Perché |
|---|---|
| l'elenco dei verbi di una porta (`OpenContext`) | è della spec delle interazioni e di `progettazione-hud.md` §18-bis |
| l'ispezione delle sagome di ultimo contatto (`ERTKnowledgeVisibility::Remembered`) | `FRTKnowledgeEntry` non porta la condizione, e la sagoma ha il suo owner |
| la parità da tastiera e controller | il contratto la impone per le Decision Window; per l'ispezione va decisa a parte |
| la zona di minaccia dopo il movimento | unione delle portate da ogni cella raggiungibile: un secondo calcolo, da chiedere se serve |
| i tooltip annidati sulle parole del glossario | il glossario nasce in questa fetta; l'annidamento è un'altra interazione |

---

## 3. Gesti

Nessun gesto esistente cambia **significato**. Cambia un tasto (`Alt` → `Ctrl`, `S6`), e il click col modificatore
guadagna un effetto.

| Gesto | Effetto |
|---|---|
| Hover, dopo un breve ritardo | **sguardo** sul bersaglio. Non cambia nessuno stato |
| Hover su una skill, nella barra o nella scheda | la **portata** di quella skill sulla mappa, da dove sta il suo personaggio |
| `LMB` su un'unità non comandata, fuori da `Targeting` | ispeziona, come oggi: **scheda** fissata e area di movimento. La camera resta ferma |
| `Ctrl` + `LMB` click | centra la camera sulla cella, come faceva `Alt` + click, e **mette a fuoco** ciò che c'è sotto: scheda fissata, più l'area di movimento se è un personaggio |
| `Ctrl` + `LMB` trascinato | orbita |
| `Ctrl` + `RMB` trascinato | dolly |
| `Ctrl` + movimento | peek |
| `Ctrl` + `MMB` trascinato | pan di precisione |
| `RMB` con un fuoco attivo | lo chiude: è il passo `Inspector` del Back. Tornano le anteprime di pianificazione |

**Fuoco** vuol dire: un bersaglio con la scheda fissata e, se è un personaggio, l'area di movimento sulla mappa. Lo
apre `LMB` su un'unità non comandata, senza muovere la camera, oppure `Ctrl` + `LMB` su qualunque bersaglio, con la
camera. Può essere a fuoco ogni personaggio che la squadra vede ora, propri compresi.

Regole:

1. **Ordine dello sguardo, uguale in ogni contesto**: unità osservata, poi porta o riparo sul bordo, poi cella.
   Che cosa farà il click lo annuncia l'anteprima sulla mappa, non lo sguardo.
2. **Il fuoco non tocca selezione né piano.** Vale la riga di `spec-tactical-camera.md`: il click col modificatore
   è un comando che chi stava pianificando ritrova tutto com'era.
3. **Un gesto, una operazione**: `Ctrl` + `LMB` è click oppure trascinamento, mai entrambi. La soglia è quella
   esistente (`ClickDragThreshold`).
4. **Il fuoco finisce** con `RMB` o con qualunque gesto di pianificazione.
5. **Contesti**: in `ReactionWindow` e `Modal` l'ispettore è inerte. In `ResolutionPlayback` funziona:
   `spec-pointer-interaction.md` §5.3 lì ammette `Inspect`.
6. **L'HUD consuma il puntatore prima del mondo**: click e rotella sopra il pannello non raggiungono la mappa.
   Sopra il pannello la rotella scorre la scheda.
7. **Le skill appartengono a un personaggio**: la barra a chi si comanda, la scheda a chi è a fuoco. La portata
   all'hover parte dalla cella di quel personaggio. Uscendo dalla skill torna la vista di prima.

---

## 4. Contenuti

| Bersaglio | Sguardo (al massimo quattro righe) | Scheda |
|---|---|---|
| Unità comandata o alleata | la targa di oggi, più nome ed effetto di ogni stato | carta, stati, kit con le ricariche **vere**, equipaggiamento. La selezione non cambia |
| Nemica osservata | come sopra, più il verso | carta, stati, verso, **kit dell'eroe**, **equipaggiamento**, **ricariche dedotte** |
| Nemica non osservata | l'etichetta della cella, identica a quella di una cella vuota | la scheda della cella, identica |
| Cella | superficie con effetto, costo d'ingresso se diverso da 1, piano, ripari per lato, «blocca la vista», obiettivo | ogni lato (riparo, porta, parapetto), regola intera della superficie, stato dell'obiettivo |
| Porta, obiettivo, salita | nome e stato | stato e che cosa blocca: passo, vista |

**Lo sguardo non taglia in silenzio.** Se gli stati non entrano nelle quattro righe, mostra i primi nell'ordine
che la targa usa già e dichiara che ce ne sono altri; la scheda li porta tutti.

**Una riga di kit** porta: nome, frase d'autore, fase, slot, portata, forma, danno, durata della ricarica — solo le
voci che per quell'azione hanno un valore. Viene dal solo catalogo.

**Una riga di ricarica dedotta** dice l'ultimo turno in cui la squadra ha visto usare l'azione e il turno da cui
torna disponibile. È una stima, e si legge come tale: un effetto non visto che allunghi o accorci la ricarica la
rende sbagliata. Un'azione mai vista usare non ha riga.

**Sulla mappa, a fuoco attivo:**

- l'area di movimento del personaggio, nello stile `Predicted` se è un nemico;
- all'hover di una sua skill, la portata: le celle a tiro libero distinte da quelle in portata ma senza linea di
  tiro. Sono i due verdetti che `ClassifyHexTargeting` già distingue: `ERTHexTargetReason::Ok` e `NoLineOfSight`.

**Del nemico non entrano mai**: il piano, gli slot del turno, l'azione armata, le ricariche reali.

**Il glossario** dà a ogni stato, superficie e tipo di riparo un nome e una riga d'effetto, testo d'autore senza
numeri, nella forma di `RTActionDescriptions`: una tabella, un documento che l'autore rilegge, un gate che
diventa rosso quando una voce manca.

---

## 5. Privacy

Tutto ciò che questa spec aggiunge è presentazione. Nessuno stato canonico, niente che entri in hash, TurnLog o
replay. Le regole:

| | Regola | Owner che si consuma |
|---|---|---|
| **P1** | **Non si costruisce ciò che non spetta.** Il compositore della scheda di un'unità non comandata riceve la proiezione per osservatore, non l'attore: piano, slot e ricariche vere non hanno un campo in cui entrare | la disciplina di `FRTContextInspectorView`; `FRTKnowledgeView` |
| **P2** | **Nemico non osservato = cella vuota**, su tutte le uscite: sguardo, scheda, area di movimento, portata | `FRTKnowledgeView`, `ARTUnit::IsKnownToObserver` |
| **P3** | **L'area di movimento di un nemico è il suo potenziale**: budget di catalogo e stati pubblici, su ciò che la squadra conosce. Non dipende dal suo piano né da unità ignote | `URTHexSimLibrary::AsObserver`, `ReachableCells` |
| **P4** | **Le ricariche dedotte sono funzione di due sole cose**: il catalogo e gli eventi che il proiettore del registro autorizza alla squadra, sull'intera partita | `URTPlayerEventProjector` |
| **P5** | **Terreno che cambia**: lo stato dinamico si mostra solo per le celle che la squadra vede adesso. Per le altre restano i dati statici della mappa | `URTPerceptionLibrary::VisibleCells` |
| **P6** | **Kit ed equipaggiamento di un nemico osservato sono pubblici per regola** (`S2`) | il catalogo |

Precisazioni:

- **Chi è noto non lo decide la linea di vista.** Da [D-510](../../decisions/RT_PDR_00_Decision_Log.md) ciò che
  una squadra sa passa dal contatto della percezione. L'ispettore legge il velo e la vista di conoscenza, e non
  chiama mai `HasLineOfSight` per deciderlo. La linea di tiro compare in un punto solo, la distinzione fra celle
  a tiro libero e bloccato nella portata, ed è geometria del tiro.
- **P4 sostituisce una frase detta in sessione.** In sessione la fonte era *«gli eventi che il registro ti ha già
  mostrato»*. Il registro a schermo copre solo l'ultimo turno, quindi la fonte è il proiettore che lo alimenta.
  L'argomento resta: una funzione di dati già autorizzati e di dati pubblici non apre un canale. Un uso fuori
  vista e un uso mai avvenuto danno la stessa scheda.
- **P6 rovescia un giudizio scritto nel codice.** Il commento su `ARTPlayerController::InspectedUnit` chiama fuga
  il mostrare *«il suo kit con i cooldown»*. Il kit diventa pubblico; i cooldown veri restano una fuga, e P1 li
  tiene fuori.

**Ogni regola da P2 a P5 ha un test a mondi gemelli**: stessa conoscenza autorizzata, stato nascosto diverso,
uscita identica. Ogni test ha la propria **mutazione**: tolta la guardia, deve diventare rosso.

---

## 6. Architettura

### 6.1 I pezzi

| Pezzo | Che cosa fa | Che cosa riusa o estende |
|---|---|---|
| **Risolutore del bersaglio** | dai candidati del raycast sceglie unità, bordo o cella, con l'ordine di §3 | `FRTPointerCandidates`; una funzione pura accanto a `URTPointerLibrary::ResolveTarget` |
| **Compositore** | produce sguardo e scheda già pronti per un osservatore: solo valori derivati | gli owner di §5 |
| **Glossario** | nome ed effetto in una riga per stati, superfici, ripari | la forma di `RTActionDescriptions` |
| **Kit statico** | le righe di una skill dal solo catalogo | `BuildActionTooltip`, divisa in parte fissa e parte di partita |
| **Ricariche dedotte** | per azione, ultimo uso visto e turno di ritorno | `URTPlayerEventProjector` |
| **Aree del fuoco** | area di movimento e portata di chi è a fuoco | `ReachableCells` su snapshot per osservatore; il produttore della portata di #3507 |
| **Arbitro dell'anteprima** | decide che cosa mostra la mappa fra piano, fuoco e hover su skill | `RefreshPlanningPreview`, che resta l'unico a scrivere i setter |
| **Superfici** | targa aperta, etichetta di cella, pannello | `URTUnitOverlayWidget`, `FRTHudBands` (§7) |

Ogni pezzo fino alle aree è una funzione pura: si prova senza viewport e senza mondo.

### 6.2 Il flusso

```text
raycast -> candidati -> risolutore -> bersaglio d'ispezione
                                            |
                 FRTKnowledgeView (osservatore) -> compositore -> sguardo | scheda
                                            |
   catalogo + glossario + eventi autorizzati -+

fuoco / hover su skill -> arbitro -> RefreshPlanningPreview -> setter della mappa
```

### 6.3 L'arbitro

È il pezzo più delicato. `RefreshPlanningPreview` è chiamata da molti punti del controller, e due sorgenti nuove
che scrivessero i setter per conto loro si contenderebbero la mappa col piano.

- **Una sola variabile** dice quale sorgente è attiva: il piano dell'unità comandata, il fuoco su un personaggio,
  l'hover su una skill.
- **Una sola funzione** scrive.
- L'hover su una skill è temporaneo: quando finisce torna la sorgente di prima.

È lo stesso obiettivo di [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511) — un solo
ingresso, un solo reset. Se quando parte F5 la issue è ancora aperta, F5 la realizza come primo passo e la chiude;
se è chiusa, F5 aggiunge le due sorgenti all'ingresso che trova.

### 6.4 Da verificare nel piano

Sono premesse plausibili e non misurate. Il piano le misura prima di poggiarci sopra.

- Quali campi dell'unità nello snapshot derivano dal **piano**. `RefreshPlanningPreview` sottrae i waypoint già
  posati; l'area di un nemico non deve leggere niente di simile.
- Come la scheda nomina **variante, gadget e modulo**, dato che l'unità non ne conserva gli id. Se serve un campo,
  lo scrive `EquipLoadout` una volta e la simulazione non lo legge.
- Che il proiettore degli eventi, applicato all'intera partita, renda il **turno** di ogni uso. Se non lo rende,
  F6 lo porta nell'evento proiettato dalla voce del TurnLog da cui nasce.
- Che ogni uso di un'azione produca un evento che nomina chi ha agito. Dove non lo produce, la riga dedotta non
  nasce: è un limite dichiarato, non un difetto.
- Come dividere `BuildActionTooltip` senza cambiare ciò che lo slot già mostra.

---

## 7. Superfici

| Superficie | Dove | Come |
|---|---|---|
| **Targa che si apre** | sopra l'unità sotto il puntatore | `FRTUnitOverlayView` guadagna le righe dello sguardo, composte dal view model; il widget le posa. La accende già `IsKnownToObserver` |
| **Etichetta di cella** | ancorata alla cella sotto il puntatore | un solo widget che si sposta, tenuto dentro il rettangolo della scena |
| **Pannello ISPEZIONE** | zona `ERTHudZone::MiddleRight`, colonna destra | compare quando una scheda è fissata; chiusa, la zona resta al registro |

Regole del pannello:

- **Fissata vince, in entrambe le fasi.** In Risoluzione il registro è promosso in quella zona: una scheda fissata
  lo copre finché `RMB` non la chiude.
- **Sta nella fascia.** La colonna destra ha la larghezza di `FRTHudBands::Right`. Il pannello non la allarga:
  allargarla stringerebbe la scena.
- **Scorre.** Kit ed equipaggiamento non entrano nell'altezza della colonna. La scheda scorre, e un taglio non è
  mai muto: la lezione è quella di `URTContextInspectorLibrary::VisibleLines`.
- **Prima consegna in C++, senza `.uasset`**, come `URTContextInspectorWidgetBase` e `URTPieVerdictOverlay`, che
  compongono la propria Slate in `RebuildWidget`. La pelle secondo le tavole viene dopo, e l'autore la giudica su
  tavole.

L'etichetta di cella è un **membro nuovo** della famiglia «UI nel mondo» che D-497 punto 6 elenca. La targa che si
apre no: è la targa.

⚠️ **La targa ha un `WBP`.** I suoi elementi nuovi chiedono authoring, e l'authoring appartiene al clone
principale.

⚠️ **Niente `DrawDebug*`**: in Shipping è vuota. Le superfici nel mondo passano da widget e dal canale di overlay
della mappa.

---

## 8. Ruling

Scelte di chi scrive, mostrate in sessione dentro le sezioni approvate (`S7`). Restano revocabili.

| | Ruling | Costo se è sbagliato |
|---|---|---|
| **R1** | `S6` vale per **tutta** la famiglia del modificatore, non per il solo click | se l'autore intendeva il solo click, click e trascinamento dello stesso tasto finiscono su due modificatori |
| **R2** | `Ctrl` + `LMB` su cella e oggetto centra **e** fissa la scheda | leggere la scheda di una cella muove la camera: da giudicare in PIE |
| **R3** | L'ordine dello sguardo è fisso in ogni contesto (§3 regola 1) | in `Targeting` / `Cell` lo sguardo parla dell'unità mentre il click mira la cella sotto |
| **R4** | `LMB` su un'unità non comandata fissa la scheda e mostra l'area, senza muovere la camera | due gesti quasi uguali, distinti dalla sola camera |
| **R5** | Una scheda fissata copre il registro anche in Risoluzione | si perde la spiegazione degli eventi mentre si ispeziona |
| **R6** | La portata di un nemico parte dalla cella in cui lo si vede ora | sottostima la minaccia di chi si muove prima di tirare; la zona di minaccia è in §2 |
| **R7** | Lo stato dinamico del terreno si mostra solo per le celle viste adesso | una cella vista un turno fa mostra i dati statici, non l'ultimo stato noto |
| **R8** | La fonte delle ricariche dedotte è il proiettore sull'intera partita | — è una correzione di fatto, vedi §5 |
| **R9** | Prima consegna del pannello in C++, senza asset | la resa non segue le tavole finché non arriva la pelle |
| **R10** | Il ritardo dell'hover è un dato: parte da 0,3 secondi e si ritara in PIE | troppo corto, la plancia lampeggia di etichette al passaggio; troppo lungo, lo sguardo sembra rotto |

---

## 9. Taglio, dipendenze, documenti

### 9.1 Le fette

Fette di [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705), in ordine di dipendenza.
**Il milestone lo assegna l'autore** quando si aprono le issue. Ogni fetta ha il proprio piano di implementazione;
il primo copre F1.

| | Fetta | Dipende da |
|---|---|---|
| **F1** | **Glossario**: testi d'autore per stati, superfici e ripari, col gate di completezza. Nessuna UI | — |
| **F2** | **Sguardo**: risolutore, compositore, targa che si apre, etichetta di cella | F1 |
| **F3** | **Scheda**: pannello, fissaggio, `RMB`, kit statico ed equipaggiamento. Chiude il buco dell'ispezione che non si vede | F2 |
| **F4** | **`Ctrl` al posto di `Alt`**: la famiglia della camera, e il fuoco sul click | F3 per il fuoco; il solo cambio di tasto è indipendente |
| **F5** | **Aree del fuoco e hover su skill**: arbitro, area di movimento, portata | F3, [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511) |
| **F6** | **Ricariche dedotte** | F3 |

### 9.2 Vicini

- [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512): l'area d'effetto sotto il cursore.
  È la fetta 2 del piano generale e usa l'arbitro di F5.
- [#3526](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3526): le frasi delle azioni. Il glossario
  ne segue la forma.
- [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613): la composizione dell'HUD, owner della
  zona che il pannello occupa.
- [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324): l'identità stabile degli oggetti.
  Questa fetta non la aspetta, perché porte e obiettivi sono dati di cella e di bordo.

### 9.3 Decisioni da scrivere nel Decision Log

Testo proposto. **Il numero si assegna al merge**, dopo `git fetch` e il controllo dei ref remoti.

1. **Di un nemico osservato sono pubblici il kit dell'eroe e l'equipaggiamento; le ricariche si leggono solo come
   deduzione dagli eventi autorizzati.** Il piano, gli slot del turno, l'azione armata e le ricariche reali restano
   privati. L'equipaggiamento resta pubblico anche quando diventerà una scelta.
2. **Lo sguardo all'hover è UI nel mondo, ancorato al bersaglio.** Precisa D-497 punto 6: la targa che si apre e
   l'etichetta di cella stanno sopra la mappa; il pannello della scheda è un inquilino della colonna destra.
3. **Il modificatore della camera è `Ctrl`, e `Ctrl` + `LMB` click mette a fuoco.** Sostituisce `Alt` in tutta la
   famiglia di `spec-tactical-camera.md` §3.2.
4. **L'area di movimento mostrata per un nemico è il suo potenziale, mai il suo piano**, calcolata su ciò che la
   squadra conosce e disegnata come previsione.

### 9.4 Documenti da aggiornare col lavoro

- `spec-pointer-interaction.md` §5: le righe di `Ctrl` + `LMB` e dell'hover sullo slot; §2 e §5.5 dove nominano
  l'inspector senza produttore.
- `spec-tactical-camera.md` §3.2: il modificatore.
- `progettazione-hud.md` §6 e §18: l'inquilino di `MiddleRight` e il contenuto dell'inspector.
- [D-458](../../decisions/RT_PDR_00_Decision_Log.md): il testo nomina `Alt` + destro per il dolly.
- `docs/technical/test-manuali-pie.md`: le voci nuove, e quelle della camera che descrivono `Alt`.
- `docs/technical/architecture/capability-map.yaml`.

---

## 10. Le altre fette della richiesta

| Fetta | Che cosa | Stato |
|---|---|---|
| 2 — Anteprima del gesto | con un'azione armata, che cosa succede se clicco: area, portata, copertura sul lato, esito atteso | ha un owner: #705, #3512 |
| 3 — Motivi del rifiuto sotto il cursore | il perché di un rifiuto, dove sta il puntatore | da disegnare |
| 4 — Leggere la squadra | intenti alleati in sola lettura | `DEFERRED`: [D-478](../../decisions/RT_PDR_00_Decision_Log.md) la rende `FUTURE` in v0.1, perché il giocatore comanda entrambi gli alleati |

---

## 11. Test e gate

**Automation**, per proprietà. I nomi dei test nascono con i test, non in questa spec.

- Il risolutore rispetta l'ordine di §3 su ogni combinazione di candidati.
- Il compositore non porta tipi grezzi, per reflection.
- Il glossario è completo: un gate rosso quando nasce uno stato, una superficie o un riparo senza testo.
- I mondi gemelli di P2, P3, P4 e P5, ciascuno con la propria mutazione.
- L'area di un nemico non cambia fra due piani diversi dello stesso nemico.
- L'arbitro: una sola funzione scrive i setter; chiuso il fuoco, la mappa mostra ciò che mostrava prima.
- La portata all'hover coincide, cella per cella, con quella che la stessa azione mostra armata.
- Il cambio di tasto: la famiglia risponde a `Ctrl`, e `Alt` non produce più nessuno dei suoi effetti.
- Click e rotella sul pannello non raggiungono il mondo.
- Il fuoco e l'ispezione non cambiano selezione, piano, né l'hash dello stato di partita.

**Suite intera prima della revisione**: l'arbitro e il percorso del click sono condivisi con la pianificazione, e
le famiglie da lanciare si scelgono dai lettori del dato, non dai file toccati.

**PIE**: voci nuove per sguardo, scheda, fuoco e hover su skill, una scena e una domanda per voce. Le voci della
camera che descrivono `Alt` si rieseguono dopo F4. I verdetti visivi l'autore li dà su tavole.

| Gate | Atteso |
|---|---|
| Compile | da eseguire per ogni fetta con codice |
| Tests | da eseguire |
| Determinism | `N/A`: nessuno stato canonico cambia. Lo prova il test sull'hash |
| Replay | `N/A`, per la stessa ragione |
| Privacy | da eseguire: i mondi gemelli di §5 |
| PIE | da eseguire, dopo ogni fetta con una superficie |
| Packaged | da eseguire per le superfici nel mondo |

---

## 12. Rischi

- **L'arbitro tocca un percorso condiviso.** Una regressione lì spegne o confonde le anteprime di pianificazione.
- **L'area di movimento di un nemico è la superficie di fuga più larga** di questa spec: due ingressi sbagliati, il
  piano o lo snapshot non filtrato, la trasformano in un rivelatore.
- **Il pannello sta in una colonna stretta e alta quanto resta.** A 16:10 le fasce tengono la misura in unità
  Slate: l'altezza utile cambia, e la scheda deve scorrere bene in entrambi i formati.
- **Il cambio di tasto invalida le voci PIE della camera**, verdi su gesti descritti tasto per tasto.
- **Il glossario è lavoro d'autore**, e F2 non parte senza.
- **Con `Ctrl` premuto la vista fa il peek.** Avvicinandosi a un bersaglio per il click, il bersaglio si sposta
  sotto il cursore. Succede già oggi con `Alt`; col fuoco il click diventa più frequente.
