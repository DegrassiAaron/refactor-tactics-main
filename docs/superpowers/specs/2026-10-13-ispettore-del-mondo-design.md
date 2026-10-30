# L'ispettore del mondo — sguardo, scheda e fuoco su unità, terreno e oggetti

> **Statuto**: design **proposto** il 2026-10-13, da una sessione di brainstorming con l'autore. Le decisioni d'autore
> prese in sessione sono in §0 (`S1`–`S8`); le tre sezioni del disegno sono state approvate in sessione una per volta.
> Lo stesso giorno l'autore ha chiesto revisione, piano e implementazione (*«review spec, scrivi piano, review piano,
> implementa piano»*): ogni fetta ha il proprio piano.
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
>
> ➕ rev. **Rivista il 2026-10-13**, su richiesta dell'autore, da chi l'ha scritta: non è una revisione indipendente.
> Il metodo è stato il confronto di ogni regola con una seconda fonte — il codice su `4e6d198cd` e il Decision Log —
> e ha trovato i sette difetti `V1`–`V7` di §13. Le correzioni sono nel testo, marcate `➕ rev.`; dove ribaltano una
> frase della prima stesura lo dicono col `⌫`.
>
> ➕ rev2. **Rivista da un revisore avversario il 2026-10-13**, chiesto dall'autore: un agente con contesto fresco,
> in sola lettura e con un tetto di chiamate. Ha trovato i tredici rilievi `A1`–`A13` di §14. Chi scrive li ha
> verificati sul codice uno per uno prima di accoglierli. Le correzioni sono marcate `➕ rev2.`, e portano la
> decisione d'autore `S8`, presa leggendo i rilievi.
>
> ➕ rev3. **Rivista il 2026-10-30 per una decisione d'autore, non per una revisione** (`S9`, [D-562](../../decisions/RT_PDR_00_Decision_Log.md)):
> dopo le tavole di `PIE-HUD-SCHEDA` l'autore ha chiesto che la barra delle skill mostri le skill dell'unità
> cliccata, anche quando non la comanda, e che la scheda perda le righe di kit. Cambiano la regola 7 di §3, che
> ribalta una frase della prima stesura (col `⌫`), e nascono le regole 13 e 14; seguono le frasi di §2, §3, §4 e §7
> che dicevano che la scheda elenca il kit, e le voci nuove di §5, §8 (`R16`-`R19`), §11 e §12. Le correzioni sono
> marcate `➕ rev3.`. Il piano è
> [`2026-10-30-barra-in-sola-lettura.md`](../plans/2026-10-30-barra-in-sola-lettura.md); la resa a schermo non
> è stata vista (PIE `NOT RUN`).
>
> **Fetta `F1` implementata** in [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744), col piano
> [`2026-10-13-glossario-stati-superfici-ripari.md`](../plans/2026-10-13-glossario-stati-superfici-ripari.md): la
> tabella `RTGlossary`, il documento [`RT_Glossary_v0.1.md`](../../balance/RT_Glossary_v0.1.md), il radar e i test
> `RefactorTactics.UI.Glossary*`. I testi restano da accettare dall'autore.
>
> **Le altre fette hanno ciascuna la propria issue**, aperte il 2026-10-13 dopo il merge di `F1`: sono nella tabella
> di §9.1. Le due voci del Decision Log che `F2` aspetta sono scritte:
> [D-512](../../decisions/RT_PDR_00_Decision_Log.md) e [D-513](../../decisions/RT_PDR_00_Decision_Log.md) (§9.3).

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
| **S8** | ➕ rev2. L'area di movimento di un nemico mostra il movimento **normale** e quello **×2** della corsa, cioè il massimo che può raggiungere: *«il movimento di un nemico che si vede e' quello x2 e normale, massima area che raggiunge»* | risposta d'autore al rilievo `A1` |
| **S9** | ➕ rev3. La barra delle skill mostra le skill dell'**unità cliccata**, in sola lettura se non la si comanda; la scheda perde le righe di kit; il lavoro resta dentro [#3748](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3748): *«se seleziono un unita' (quella che controllo, quella del compagniuo di squadra o quella avversaria) la barra delle skill che vedo e' quella della unita' selezionata. questo mi permette di passare il mouse sopra una skill e vedere il raggio e altre informazioni.»* | messaggio libero dopo le tavole `T1`-`T4` della scheda, e le domande a scelta multipla che lo seguivano; il testo visto dall'autore è alla lettera in [D-562](../../decisions/RT_PDR_00_Decision_Log.md) |

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
- ➕ rev. **La geometria che la squadra non ha osservato è nascosta**
  ([D-225](../../decisions/RT_PDR_00_Decision_Log.md)), e quella esplorata resta nota
  ([D-227](../../decisions/RT_PDR_00_Decision_Log.md)): `FRTTeamKnowledge` porta `VisibleCells` ed
  `ExploredCells`, e il terzo stato è l'assenza da entrambe. `URTHexSimLibrary::MakeSnapshot` filtra per
  osservatore l'**occupazione**, non la geometria. Le celle esplorate sono un elenco di id: lo stato che avevano
  quando sono state viste non è conservato.
- ➕ rev. **Durante la risoluzione il click su un'unità è `Blocked`**, non `Inspect`:
  `URTPointerLibrary::ResolveOutcome` risponde così in `ResolutionPlayback`, `Modal` e `ReactionWindow`. La riga
  di `spec-pointer-interaction.md` §5.3 che lì ammette `Inspect` non ha quindi un produttore.
- ➕ rev2. **Il click sul mondo si ferma anche prima del router**: `ARTPlayerController::IsWorldReadOnly` è vero
  in `ResolutionPlayback`, `ReactionWindow` e `Modal`, e i punti che mutano il piano escono su di lui.
- ➕ rev2. **Il budget di movimento di un'unità nello snapshot viene dal suo piano.**
  `ARTTurnManager::MakeSimUnit` lo calcola con `URTMovementProfileLibrary::ProfileForPlan`, alle bande di
  [D-412](../../decisions/RT_PDR_00_Decision_Log.md): ripiegamento 25, furtivo 50, normale e fermo 100, corsa 200, in percentuale di
  `ARTUnit::GetEffectiveMoveRange`.
- ➕ rev2. **Il flag di conoscenza nasce acceso.** `ARTUnit::bKnownToObserver` vale `true` alla nascita, e lo
  aggiorna `ARTHUD::UpdateObserverVeil`. [D-466](../../decisions/RT_PDR_00_Decision_Log.md) lo dice del proprio perimetro: *«nasce `true`,
  quindi il nemico risulta osservato»*.
- ➕ rev2. **Il proiettore del registro è una narrazione, non un registro degli usi.**
  `URTPlayerEventProjector::Project` tiene una riga per unità per chiamata, scelta per dominanza, e
  `FRTPlayerEvent` non porta il turno. Chi sia il soggetto di una voce — chi agisce o chi subisce — cambia per
  esito: lo dice `URTTurnLogLibrary::IsSubjectTheSufferer`.
- ➕ rev2. **Il pivot della camera accetta ogni cella che la mappa contiene**, anche mai vista:
  `ARTPlayerController::OnSelectReleased` chiede solo `URTHexMapAsset::ContainsCell`.
- ➕ rev2. **La risoluzione applica tutto prima del playback**: stati, danni e posizioni sono già sull'unità, e
  sulla targa, quando la scena comincia. È il limite dichiarato dall'epic
  [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453).

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
| sapere chi è noto a un osservatore | `FRTKnowledgeView` (`URTKnowledgeViewLibrary`): un soggetto ignoto **non ha una voce**. ➕ rev2. `ARTUnit::IsKnownToObserver` accende i widget ma nasce vero: non è un cancello (§5, P2) |
| una vista che non può portare dati grezzi | la disciplina di `FRTContextInspectorView`, provata per reflection |
| uno snapshot per osservatore | `URTHexSimLibrary::MakeSnapshot` e `AsObserver` |
| le celle raggiungibili | `URTHexSimLibrary::ReachableCells(Snapshot, UnitId)` |
| la portata di un'azione | `URTCombatLibrary::ClassifyHexTargeting`: riceve due celle e una mappa, non le unità |
| le righe di una skill | `URTHudViewModel::BuildActionTooltip`, `RTActionDescriptions::For` |
| le voci autorizzate a un osservatore | ➕ rev2. `URTPlayerEventProjector::IsAuthorized`, sulle voci del TurnLog; `URTTurnLogLibrary::IsSubjectTheSufferer` |
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
- la **portata all'hover di una skill**, dalla barra. ➕ rev3. ⌫ *La prima stesura diceva «dalla barra o dalla scheda»*: la scheda non ha più righe di skill, e la barra mostra anche le skill di chi è a fuoco (regola 7);
- il cambio di modificatore della camera da `Alt` a `Ctrl`;
- il **glossario** di stati, superfici e ripari.

**Fuori**, con il motivo:

| Fuori | Perché |
|---|---|
| l'elenco dei verbi di una porta (`OpenContext`) | è della spec delle interazioni e di `progettazione-hud.md` §18-bis |
| l'ispezione delle sagome di ultimo contatto (`ERTKnowledgeVisibility::Remembered`) | `FRTKnowledgeEntry` non porta la condizione, e la sagoma ha il suo owner |
| la parità da tastiera e controller | il contratto la impone per le Decision Window; per l'ispezione va decisa a parte |
| la zona di minaccia dopo il movimento | unione delle portate da ogni cella raggiungibile: un secondo calcolo, da chiedere se serve |
| ➕ rev. lo stato **ricordato** di una cella esplorata e non più in vista | chiede una memoria per squadra dello stato delle celle, che oggi non esiste (§1.2): è un sistema di percezione, non di ispezione |
| i tooltip annidati sulle parole del glossario | il glossario nasce in questa fetta; l'annidamento è un'altra interazione |

---

## 3. Gesti

Nessun gesto esistente cambia **significato**. Cambia un tasto (`Alt` → `Ctrl`, `S6`), e il click col modificatore
guadagna un effetto.

| Gesto | Effetto |
|---|---|
| Hover, dopo un breve ritardo | **sguardo** sul bersaglio. Non cambia nessuno stato |
| Hover su una skill, nella barra, della propria unità o di quella fissata | la **portata** di quella skill sulla mappa, da dove sta il suo personaggio, e il tooltip del catalogo. ➕ rev3. ⌫ *La prima stesura diceva «nella barra o nella scheda»*: la scheda non ha più righe di skill |
| `LMB` su un'unità non comandata e osservata, in `IdleSelection` e `Planning` | ispeziona, come oggi ([D-128](../../decisions/RT_PDR_00_Decision_Log.md)): **scheda** fissata e area di movimento. La camera resta ferma. ➕ rev2. Per gli altri contesti vale la regola 9. ➕ rev3. **E la barra delle skill passa a quell'unità, in sola lettura** (regola 13) |
| `Ctrl` + `LMB` click | centra la camera sulla cella, come faceva `Alt` + click, e **mette a fuoco** ciò che c'è sotto: scheda fissata, più l'area di movimento se è un personaggio |
| `Ctrl` + `LMB` trascinato | orbita |
| `Ctrl` + `RMB` trascinato | dolly |
| `Ctrl` + movimento | peek |
| `Ctrl` + `MMB` trascinato | pan di precisione |
| `RMB` con un fuoco attivo | lo chiude: è il passo `Inspector` del Back. Tornano le anteprime di pianificazione. ➕ rev3. E la barra delle skill torna alla selezione |

**Fuoco** vuol dire: un bersaglio con la scheda fissata e, se è un personaggio, l'area di movimento sulla mappa. Lo
apre `LMB` su un'unità non comandata, senza muovere la camera, oppure `Ctrl` + `LMB` su qualunque bersaglio, con la
camera. Può essere a fuoco ogni personaggio che la squadra vede ora, propri compresi.

Regole:

1. ➕ rev2. **Lo sguardo è l'annotazione che il contratto prevede già, non un secondo bersaglio.**
   `spec-pointer-interaction.md` §4.1 dice quale candidato vince come bersaglio in ogni contesto, e chiama
   «annotazione» tutto il resto. Lo sguardo descrive ciò che sta sotto il cursore con un ordine solo, quello
   della riga `IdleSelection` / `Planning` di §4.1: unità comandabile, altra unità nota, oggetto logico od
   obiettivo, ghost proprio, bordo, cella. Non cambia mai quale candidato il click colpisce. E l'esito dell'hover
   resta quello che §5.1 assegna a quel contesto, `Inspect` o `Preview`: l'affordance mostrata prima del click
   resta quella che il click esegue ([D-128](../../decisions/RT_PDR_00_Decision_Log.md)).
   ⌫ *La prima stesura diceva «ordine dello sguardo, uguale in ogni contesto» e dava `Inspect` a ogni hover*: in
   `Pathing` e in `Targeting` / `Cell` contraddiceva §4.1 e la colonna Hover.
2. **Il fuoco non tocca selezione né piano.** Vale la riga di `spec-tactical-camera.md`: il click col modificatore
   è un comando che chi stava pianificando ritrova tutto com'era. ➕ rev3. Vale anche per la barra delle skill: mostra
   un'altra unità (regola 7), ma selezione, azione armata e piano di chi si comanda restano com'erano, e li ritrova
   chi riseleziona una propria unità.
3. **Un gesto, una operazione**: `Ctrl` + `LMB` è click oppure trascinamento, mai entrambi. La soglia è quella
   esistente (`ClickDragThreshold`).
4. ➕ rev. **La scheda resta, l'area sulla mappa cede.** ⌫ *La prima stesura diceva «il fuoco finisce con `RMB` o
   con qualunque gesto di pianificazione»*, e contraddiceva la decisione del 2026-09-11 scritta su
   `ARTPlayerController::InspectedUnit`: l'ispezione **convive** con la selezione e persiste finché non se ne
   ispeziona un'altra. Quindi:
   - la **scheda** si chiude con `RMB`, o quando se ne fissa un'altra;
   - l'**area di movimento** di chi è a fuoco lascia la mappa al primo gesto di pianificazione — armare, posare un
     waypoint, dichiarare un verso — e non torna da sola.

   ➕ rev3. **Restringe questa decisione, non la ribalta.** *Lo stato* convive ancora: ispezionare non fa perdere la
   selezione né l'azione armata, e non scrive il piano. Cambia **che cosa mostra la barra** (regola 7) e, finché la
   barra è in sola lettura, che cosa si può scrivere nel piano (regola 14).
5. ➕ rev. **Contesti**: in `ReactionWindow` e `Modal` l'ispettore è inerte. In `ResolutionPlayback` deve
   funzionare, come `spec-pointer-interaction.md` §5.3 chiede, e oggi non funziona (§1.2): F3 porta
   `ResolveOutcome` a rispondere `Inspect` per un'unità osservata in quel contesto. Tutto ciò che cambierebbe il
   piano resta `Blocked`. ➕ rev2. Non basta il router: va aperto all'ispezione anche il cancello
   `IsWorldReadOnly` (§1.2), senza aprirlo a ciò che scrive il piano. E in risoluzione la scheda mostra ciò che
   mostra la targa, anche quando anticipa la scena: è il limite di #2453, ereditato e non introdotto (R15).
6. **L'HUD consuma il puntatore prima del mondo**: click e rotella sopra il pannello non raggiungono la mappa.
   Sopra il pannello la rotella scorre la scheda.
7. ➕ rev3. **Le skill appartengono a un personaggio, e la barra mostra quelle dell'ultima unità cliccata.** La barra
   segue la selezione, cioè è di chi si comanda, finché l'ultimo click su un'unità non è stato l'ispezione di
   un'unità che **non** si comanda: allora mostra le skill di *quella*, dal solo catalogo (regola 13), in sola
   lettura. Senza tasti, senza anello di ricarica, senza azione armata, senza stato: ogni slot è «disponibile»,
   che vuol dire «niente stato» e non «pronta». Passare sullo slot mostra la portata sulla mappa, dalla cella in
   cui la squadra vede quel personaggio ora, e il tooltip del catalogo; uscendo dalla skill torna la vista di
   prima. La scheda non elenca più il kit: carta, stati, verso, equipaggiamento e ricariche dedotte. L'unità di un
   compagno si comanda, quindi si seleziona e la sua barra è quella di sempre: la sola lettura riguarda le unità
   che la regola di `URTCombatLibrary::CanPlayerControlUnitInGroup` non assegna a chi gioca, oggi quelle avversarie.
   ⌫ *La prima stesura diceva: «Le skill appartengono a un personaggio: la barra a chi si comanda, la scheda a chi è
   a fuoco. La portata all'hover parte dalla cella di quel personaggio»*: la scheda aveva le righe di kit, e passarci
   sopra mostrava la portata. L'autore ha chiesto che la barra mostri le skill dell'unità selezionata, anche non
   comandata ([D-562](../../decisions/RT_PDR_00_Decision_Log.md), `S9`).
8. ➕ rev. **`RMB` chiude la scheda prima di uscire da un targeting.** È l'ordine che `ResolveBack` ha già: il passo
   `Inspector` viene prima di `Declaration` e di `Waypoint`. Chi fissa la scheda di un nemico e poi arma un attacco
   preme `RMB` due volte per disarmare. L'ordine non si cambia qui: è un contratto con i suoi test, e il giudizio
   su quanto pesi è di una seduta PIE.
9. ➕ rev2. **`LMB` su un'unità non comandata, contesto per contesto.**
   - `IdleSelection` e `Planning`: ispeziona ([D-128](../../decisions/RT_PDR_00_Decision_Log.md), punto 1).
   - `Targeting`: conferma o rifiuta il bersaglio, come oggi.
   - `Facing`: col selettore aperto ogni click è una direzione, e le unità sono trasparenti
     (`URTPointerLibrary::ResolveTarget`). Non ispeziona.
   - `Pathing`: `IMPLEMENTATION DRIFT`. `ResolveOutcome` risponde `Inspect`, mentre §4.1 del contratto mette la
     cella prima di ogni unità non comandabile. Questa spec non lo risolve: lo segnala a #705.
   - In ogni contesto `Ctrl` + `LMB` resta il gesto che mette a fuoco.
10. ➕ rev2. **La scheda segue la conoscenza, non l'attore.** Scheda e area si ricompongono a ogni aggiornamento
    dalla vista di conoscenza. Se il soggetto non ha più una voce `Live` — è uscito di vista — la scheda si chiude
    e l'area lascia la mappa. Lo stesso se il soggetto cade: un'unità eliminata non ha una scheda (R13).
11. ➕ rev2. **Una finestra di reazione o un modale nascondono la scheda, senza chiuderla.** In `ResolveBack` quei
    due passi vengono prima di `Inspector`: finché sono aperti `RMB` è loro. La scheda torna quando si chiudono.
12. ➕ rev2. **Chi guarda è lo stesso insieme di squadre del registro**
    (`URTHudViewModel::ResolveObserverTeamIds`). In una sessione senza un giocatore che comanda —
    autobattle, spettatore — l'ispettore è inerte (R14): lì «comandata» e «non comandata» non esistono, e questa
    spec non le inventa.
13. ➕ rev3. **Il soggetto della barra è derivato, e finisce con il gesto che dice «torna alla mia unità».** È
    l'unità fissata dall'ultima ispezione **se** è viva, ha una voce `Live` nella vista di conoscenza e non si
    comanda; altrimenti è la selezione. L'ispezione che lo porta sulla barra è `LMB` con esito `Inspect` e, per lo
    stesso gesto, il click sul pavimento dell'esagono del bersaglio di una carica già pianificata
    ([D-528](../../decisions/RT_PDR_00_Decision_Log.md)). Lo riportano alla selezione: ogni `SelectUnit`, **anche
    sull'unità già selezionata** (il click sulla propria unità non cambia la selezione e non chiude la scheda, ma
    dice «torna alla mia»); `N`, quando la sola unità comandabile è la selezionata; `RMB`, che chiude la scheda
    (passo `Inspector`); l'uscita di vista del soggetto o la sua caduta. Il cancello `Live` lo applica chi legge, a
    ogni lettura, e non chi decide il soggetto: un nemico non osservato non ha scheda **né barra** (P2).
    `Ctrl` + click non fissa niente oggi, e non sposta la barra; se la fetta F4 lo farà passare da `PinInspection`,
    la porterà come il click, e va deciso che sia voluto. Resta aperto un caso: con nessuna unità ciclabile (tutte
    hanno dichiarato) `N` non riporta la barra; restano il click e `RMB`.
14. ➕ rev3. **Con la barra in sola lettura il piano non si scrive, e il log dice perché.** Un solo predicato,
    `IsPlanningWriteBlocked` (mondo in sola lettura **o** barra in sola lettura, [D-468](../../decisions/RT_PDR_00_Decision_Log.md)),
    guarda i punti che scrivono il piano: la dichiarazione con `Invio`, il click su un'unità e su una cella, l'armo
    da tasti, generiche e slot, `Sneak` (`M`) e il selettore del verso (apertura, scelta, hover). Restano vivi
    selezione, ispezione — compreso il click D-528 sul pavimento —, `RMB` e camera. La causa della barra nomina
    l'unità mostrata e il rimedio (chiudere la scheda con il tasto destro o selezionare una propria unità); con le
    due cause insieme dice quella del mondo. Il rifiuto sta nel controller, in un posto solo: lo slot confronta per
    chi è stato costruito con il soggetto della barra e non ha un rifiuto suo. È un rifiuto del client: il server
    valida comunque. Il costo è la scelta `R16`.

➕ rev. **Gli esiti**, nell'elenco chiuso di `spec-pointer-interaction.md` §5. Nessun gesto nuovo ne chiede un nono.

| Gesto | Esito |
|---|---|
| Hover su unità osservata, cella in vista, porta, riparo, obiettivo | ➕ rev2. quello di §5.1 per il contesto: `Inspect`, oppure `Preview` dove il contratto lo assegna. Lo sguardo non lo cambia |
| Hover su una skill | `Inspect`: la portata è informazione pubblica, non un esito non committato |
| `LMB` su un'unità non comandata e osservata, in `IdleSelection` e `Planning` | `Inspect`, come oggi |
| `Ctrl` + `LMB` click su un bersaglio che la squadra vede | `Inspect`, più il comando di vista, che non è un esito del puntatore |
| `Ctrl` + `LMB` click su una cella non in vista o su un'unità non osservata | il solo comando di vista: per il puntatore è `NoOp`. ➕ rev2. Il pivot accetta oggi ogni cella della mappa: è un canale ereditato (§5) |
| `RMB` con la scheda fissata | `Cancel` |

---

## 4. Contenuti

| Bersaglio | Sguardo (al massimo quattro righe) | Scheda |
|---|---|---|
| Unità comandata o alleata | la targa di oggi, più nome ed effetto di ogni stato | carta, stati, equipaggiamento. La selezione non cambia. ➕ rev3. ⌫ *La prima stesura aggiungeva «kit con le ricariche **vere**»*: il kit sta sulla barra, e per un'unità che si comanda la barra porta già le ricariche vere |
| Nemica osservata | come sopra, più il verso | carta, stati, verso, **equipaggiamento**, **ricariche dedotte**. ➕ rev3. ⌫ *La prima stesura aggiungeva «kit dell'eroe»*: il **kit dell'eroe** sta sulla barra, in sola lettura (regola 7) |
| Nemica non osservata | l'etichetta della cella, identica a quella di una cella vuota | la scheda della cella, identica |
| Cella **in vista** | superficie con effetto, costo d'ingresso se diverso da 1, piano, ripari per lato, «blocca la vista», obiettivo | ogni lato (riparo, porta, parapetto), regola intera della superficie, stato dell'obiettivo |
| ➕ rev. Cella **non in vista**, esplorata o mai vista | nulla | nulla |
| Porta, obiettivo, salita, su una cella in vista | nome e stato | stato e che cosa blocca: passo, vista |

**Lo sguardo non taglia in silenzio.** Se gli stati non entrano nelle quattro righe, mostra i primi nell'ordine
che la targa usa già e dichiara che ce ne sono altri; la scheda li porta tutti.

**Una riga di kit** porta: nome, frase d'autore, fase, slot, portata, forma, danno, durata della ricarica — solo le
voci che per quell'azione hanno un valore. Viene dal solo catalogo. ➕ rev3. ⌫ *La prima stesura la metteva sulla
scheda*: sta nel tooltip dello slot della barra (da guardare in PIE), e per un'unità in sola lettura la riga è
quella di `URTHudViewModel::BuildPublicBar`, che porta il catalogo e niente della partita.

**Una riga di ricarica dedotta** dice l'ultimo turno in cui la squadra ha visto usare l'azione e il turno da cui
torna disponibile. È una stima, e si legge come tale: un effetto non visto che allunghi o accorci la ricarica la
rende sbagliata. Un'azione mai vista usare non ha riga. ➕ rev2. Nemmeno un'azione la cui ricarica parte senza che
nessuno la veda agire — una reazione armata e mai scattata: la deduzione tace, non indovina.

**Sulla mappa, a fuoco attivo:**

- l'area di movimento del personaggio. ➕ rev2. Per un nemico sono **due** aree, nello stile `Predicted` (`S8`):
  dove arriva col movimento normale, e dove arriva con la corsa, che vale il doppio. La seconda è il massimo che
  può raggiungere. Se uno stato pubblico gli nega la corsa — oggi `Status.Unbalanced` — resta la sola area
  normale;
- all'hover di una sua skill, la portata. ➕ rev. Le celle in portata hanno **tre** stati, non due:
  - **tiro libero**: `ClassifyHexTargeting` risponde `ERTHexTargetReason::Ok`, e la squadra vede adesso ogni cella
    della linea;
  - **tiro bloccato**: risponde `NoLineOfSight`, e la squadra vede adesso la cella che ferma il tiro;
  - **in portata, senza verdetto**: la linea attraversa celle che la squadra non vede adesso. Dire «libero» o
    «bloccato» lì rivelerebbe geometria nascosta, o ciò che è cambiato fuori vista. ➕ rev2. ⌫ *La revisione
    precedente diceva «nota», che comprende le celle esplorate*: il loro stato di adesso è ciò che P5 vieta di
    leggere.

  ⌫ *La prima stesura aveva solo i primi due*, e li applicava a ogni cella in portata.

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
| **P1** | ➕ rev. **Non si costruisce ciò che non spetta, e l'attore passa da una porta sola.** Una sola funzione legge l'attore di un'unità non comandata e ne ricava il soggetto pubblico: identità, cella, verso, ciò che la targa mostra già, pezzi equipaggiati. Il compositore riceve quel soggetto e non vede mai l'attore: piano, slot e ricariche vere non hanno un campo in cui entrare | la disciplina di `FRTContextInspectorView`; `FRTKnowledgeView` per sapere **se** il soggetto esiste; `FRTUnitOverlayView` per la condizione |
| **P2** | **Nemico non osservato = cella vuota**, su tutte le uscite: sguardo, scheda, area di movimento, portata. ➕ rev2. Il cancello è **uno**: una voce `Live` nella vista di conoscenza. `ARTUnit::IsKnownToObserver` nasce vero, e qui non decide niente | `FRTKnowledgeView` |
| **P3** | ➕ rev2. **L'area di movimento di un nemico è il suo potenziale, dentro ciò che la squadra vede adesso.** Il budget è `ARTUnit::GetEffectiveMoveRange`, che legge catalogo e stati pubblici, alle due bande fisse di `S8`: normale e corsa. ⛔ Mai la banda del suo piano: `ARTTurnManager::MakeSimUnit` la legge, e per il soggetto a fuoco non si usa. Due regole pubbliche non passano da `GetEffectiveMoveRange` e vanno riportate nel calcolo costruito apposta: il costo in più per cella di `Status.Slow` (`MoveCostModifier`, in `MakeSimUnit`) e il divieto di corsa di `Status.Unbalanced` (in `URTMovementProfileLibrary::CeilingProfile(..., bRunDenied)`, che la validazione del piano e il controller già leggono). ⌫ *La prima stesura le dava entrambe a `MakeSimUnit`: lì `Unbalanced` diventa solo `ExtraSlideCells`, celle di scivolamento in più.* `GetEffectiveMoveRange` legge `MoveRange`, `Status.Root` e il prezzo del rialzo da `Prone`. Non dipende da unità ignote. Si disegna solo su celle in vista adesso, e una cella non in vista non è attraversabile nel calcolo | `URTHexSimLibrary::MakeSnapshot` per l'occupazione; `FRTTeamKnowledge::VisibleCells` per le celle |
| **P4** | ➕ rev2. **Le ricariche dedotte sono funzione di due sole cose**: il catalogo, e le voci del TurnLog che il filtro del registro autorizza alla squadra, sull'intera partita. Una voce conta solo se il suo soggetto è l'unità ispezionata **e** il soggetto è chi agisce. ⛔ Mai dedurre chi ha agito dall'`ActionId` o da una cella di una voce che parla di un altro | `URTPlayerEventProjector::IsAuthorized`; `URTTurnLogLibrary::IsSubjectTheSufferer` |
| **P5** | ➕ rev. **Di una cella si legge qualcosa solo se la squadra la vede adesso.** Una cella esplorata e non più in vista, come una mai vista, non dà sguardo né scheda | `FRTTeamKnowledge::VisibleCells` |
| **P6** | **Kit ed equipaggiamento di un nemico osservato sono pubblici per regola** (`S2`) | il catalogo |
| **P7** | ➕ rev. **Il verdetto di tiro nella portata non rivela ciò che la squadra non vede adesso**: i tre stati di §4 | `FRTTeamKnowledge::VisibleCells`; `ClassifyHexTargeting` |

Precisazioni:

- ➕ rev. **P5 e P3 correggono un errore di premessa.** ⌫ *La prima stesura diceva: «lo stato dinamico si mostra
  solo per le celle che la squadra vede adesso; per le altre restano i dati statici della mappa»*, cioè trattava
  la mappa come pubblica. Non lo è: D-225 nasconde la geometria non osservata. E per le celle esplorate non c'è
  uno «stato statico» da mostrare: superfici, ripari e porte cambiano nella stessa mappa di runtime, e lo stato
  che una cella aveva quando è stata vista non è conservato. Mostrare quello di adesso rivelerebbe ciò che è
  cambiato fuori vista. Da qui la regola stretta: si legge solo ciò che si vede ora.
- ➕ rev. **Un canale ereditato, dichiarato.** L'anteprima di pianificazione dell'unità comandata calcola già
  ventaglio e portata sulla geometria intera: `MakeSnapshot` filtra l'occupazione, non le celle. Questa spec non
  lo corregge e non lo estende: le sue uscite nuove — l'area di un nemico, il verdetto di tiro — seguono P3 e P7.
  Se il ventaglio proprio debba fare lo stesso è una domanda per l'owner della conoscenza parziale.
  ➕ rev2. Lo stesso vale per il pivot della camera: il click col modificatore accetta ogni cella che la mappa
  contiene, anche mai vista, quindi distingue già oggi «qui c'è una cella» da «qui non c'è niente».
- **Chi è noto non lo decide la linea di vista.** Da [D-510](../../decisions/RT_PDR_00_Decision_Log.md) ciò che
  una squadra sa passa dal contatto della percezione. L'ispettore legge il velo e la vista di conoscenza, e non
  chiama mai `HasLineOfSight` per deciderlo. La linea di tiro compare in un punto solo, la distinzione fra celle
  a tiro libero e bloccato nella portata, ed è geometria del tiro.
- ➕ rev2. **P4 ha cambiato fonte due volte.** In sessione era *«gli eventi che il registro ti ha già
  mostrato»*; la prima stesura l'ha spostata sul proiettore del registro. ⌫ Nemmeno quello regge: il proiettore
  è una narrazione, con una riga per unità e senza il turno (§1.2), e sull'intera partita lascerebbe un evento
  per unità. La fonte sta un livello sotto: le voci del TurnLog che passano `IsAuthorized`, col loro
  `TurnNumber`. L'argomento resta, con una condizione in più: una funzione di dati già autorizzati e di dati
  pubblici non apre un canale, **purché non attribuisca un'azione a chi non è il soggetto della voce**. Un uso
  fuori vista e un uso mai avvenuto danno la stessa scheda.
- ➕ rev2. **P6 rovescia un giudizio scritto in più punti del codice, e tocca una regola.** I punti sono in
  §9.4. La regola è quella di #2757: *«carta sì, slot mai, e nemmeno vuoti»*. Che cosa cambia: il **kit** —
  quali azioni un eroe ha, coi numeri di catalogo — e l'equipaggiamento diventano pubblici. Che cosa non cambia:
  gli **slot**, cioè il piano del turno, e le ricariche vere restano privati, e P1 li tiene fuori. La regola di
  #2757 resta vera per gli slot.
- ➕ rev3. **P6 ora la legge la barra, non la scheda.** La barra di un soggetto in sola lettura nasce da
  `URTHudViewModel::BuildPublicBar`, un elenco **positivo** di campi di catalogo: non porta il piano, le ricariche
  vere, l'azione armata né i tasti, e nemmeno `TotalTurns`, il denominatore dell'anello di carica (un valore
  maggiore di zero con la carica piena direbbe «carica» di una skill nemica la cui ricarica vera è privata). Lo
  prova un test con un controllo positivo: la stessa unità, letta da `BuildAbilityCooldowns`, li porta. Il dock non
  costruisce mai `BuildAbilityCooldowns` per un soggetto in sola lettura, e applica il cancello `Live` a ogni
  lettura.

**Le regole P2, P3, P4, P5 e P7 hanno ciascuna un test a mondi gemelli**: stessa conoscenza autorizzata, stato
nascosto diverso, uscita identica. Per P3 e P7 i due mondi differiscono in tre modi: nella geometria mai vista, nello stato di una cella esplorata e
non più in vista, e nel piano del nemico. ➕ rev2. Per P2 c'è un gemello in più: la finestra fra la nascita di
un'unità e il primo aggiornamento del velo. Ogni test ha la propria **mutazione**: tolta la guardia, deve
diventare rosso.

---

## 6. Architettura

### 6.1 I pezzi

| Pezzo | Che cosa fa | Che cosa riusa o estende |
|---|---|---|
| **Risolutore del bersaglio** | dai candidati del raycast sceglie unità, bordo o cella, con l'ordine di §3 | `FRTPointerCandidates`; una funzione pura accanto a `URTPointerLibrary::ResolveTarget` |
| **Compositore** | produce sguardo e scheda già pronti per un osservatore: solo valori derivati | gli owner di §5 |
| **Glossario** | nome ed effetto in una riga per stati, superfici, ripari | la forma di `RTActionDescriptions` |
| **Kit statico** | le righe di una skill dal solo catalogo | `BuildActionTooltip`, divisa in parte fissa e parte di partita. ➕ rev3. Lo legge la barra in sola lettura (`BuildPublicBar`), non la scheda |
| **Ricariche dedotte** | per azione, ultimo uso visto e turno di ritorno | ➕ rev2. le voci del TurnLog che passano `URTPlayerEventProjector::IsAuthorized` |
| **Aree del fuoco** | area di movimento e portata di chi è a fuoco | `ReachableCells` su uno snapshot per osservatore costruito apposta — ➕ rev2. non con `MakeSimUnit`, che legge il piano; il produttore della portata di #3507 |
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

- ➕ rev2. ✅ **Misurato dal revisore**: il budget dell'unità nello snapshot deriva dal piano (§1.2), ed è il
  motivo di P3. ✅ **Misurato in #3750** (`ARTTurnManager::MakeSimUnit` e `ARTUnit::GetEffectiveMoveRange`, su
  `origin/main` = `69e8a9cd3`): dei campi che `MakeSimUnit` scrive in `FRTHexSimUnit`, **solo `MoveBudget` e
  `StepBudget`** derivano dal piano, via `URTMovementProfileLibrary::ProfileForPlan`; `MoveCostModifier` (`Slow`),
  `ExtraSlideCells` (`Unbalanced`), `Facing`, `TeamId` e `StableUnitId` vengono dall'unità. `GetEffectiveMoveRange`
  parte da `MoveRange` e legge `Status.Root` e il prezzo del rialzo da `Prone` ([D-319](../../decisions/RT_PDR_00_Decision_Log.md)).
- ➕ rev. Come rendere una cella non in vista **non attraversabile** nel calcolo dell'area di un nemico (P3), dato
  che `ReachableCells` legge la mappa dello snapshot per intero. Ritagliare il risultato alle celle in vista non
  basta: una cella in vista raggiungibile solo passando per celle non in vista resterebbe nel ventaglio, e lo
  direbbe.
- Come la scheda nomina **variante, gadget e modulo**, dato che l'unità non ne conserva gli id. Se serve un campo,
  lo scrive `EquipLoadout` una volta e la simulazione non lo legge.
- ➕ rev2. Su chi si congela il verdetto di una voce di combattimento, e quale `ActionId` porta una voce di
  stato il cui soggetto è chi subisce. Il revisore non ha potuto controllarlo. Finché non è misurato, F6 conta
  solo le voci in cui il soggetto è chi agisce (P4).
  ✅ **Misurato in #3751** (`AppendLogEntry`, `MakeStatusBirthEntry`, `FRTTurnLogEntry::ActionId`): il verdetto di
  una voce si congela contro l'unità passata come attore, la stessa che finisce in `UnitId`
  (`Entry.VerdictSubject.StableUnitId == Entry.UnitId`). Vale **per costruzione** — `AppendLogEntry` legge
  `Actor->StableUnitId` in entrambi i punti — e il test `SeenUse.ARealSeenAttackLeavesARowAndAnUnseenOneDoesNot` tiene
  ferma la costruzione sulle voci vere di un turno; non e' una prova che il verdetto sia giusto, che e' il compito
  dell'audit di D-313. Una voce di stato porta in `ActionId` il **tag dello
  stato**, mai un'azione del kit. Il caso che invece accrediterebbe l'azione sbagliata è un altro, e si vede solo
  leggendo `ERTMoveOutcome::Displaced`: `UnitId` è chi viene spostato, `ActionId` è l'azione di **chi spinge**, e
  `IsSubjectTheSufferer` non lo riconosce come voce di chi subisce.
- Che ogni uso di un'azione produca una voce che nomina chi ha agito. Dove non la produce, la riga dedotta non
  nasce: è un limite dichiarato, non un difetto.
  ✅ **F6 conta**: il danno inflitto da chi agisce (`IsDamageInflictedByActor`: colpi, Overwatch che spara, previsione
  azzeccata) e la cura (`Combat/Healed`). **Non conta, e la scheda non ha riga**: movimento e Dash (la categoria
  `Move` ha l'inversione di `Displaced`), azioni ambientali, stati, fallback, un attacco senza linea di tiro (non è
  detto che la ricarica sia stata pagata) e una cura su se stessi (`IsSubjectTheSufferer` la legge come danno
  ambientale). Allargare l'insieme è un candidato a parte: ogni categoria nuova vuole la propria misura di chi sta
  in `UnitId`.
- ➕ rev2. Quale stato legge la scheda durante il playback, e se la targa ne mostra uno diverso (§3 regola 5).
- Come dividere `BuildActionTooltip` senza cambiare ciò che lo slot già mostra.

---

## 7. Superfici

| Superficie | Dove | Come |
|---|---|---|
| **Targa che si apre** | sopra l'unità sotto il puntatore | `FRTUnitOverlayView` guadagna le righe dello sguardo, composte dal view model; il widget le posa. ➕ rev2. Il componente si accende col velo; le righe dello sguardo si compongono solo se la vista di conoscenza ha una voce `Live` (P2) |
| **Etichetta di cella** | ancorata alla cella sotto il puntatore | un solo widget che si sposta, tenuto dentro il rettangolo della scena |
| **Pannello ISPEZIONE** | zona `ERTHudZone::MiddleRight`, colonna destra | compare quando una scheda è fissata; chiusa, la zona resta al registro |

Regole del pannello:

- **Fissata vince, in entrambe le fasi.** In Risoluzione il registro è promosso in quella zona: una scheda fissata
  lo copre finché `RMB` non la chiude.
- **Sta nella fascia.** La colonna destra ha la larghezza di `FRTHudBands::Right`. Il pannello non la allarga:
  allargarla stringerebbe la scena.
- **Scorre.** Stati ed equipaggiamento non entrano nell'altezza della colonna. La scheda scorre, e un taglio non è
  mai muto: la lezione è quella di `URTContextInspectorLibrary::VisibleLines`. ➕ rev3. ⌫ *La prima stesura diceva
  «Kit ed equipaggiamento»*: il kit non è più sulla scheda. Il gate `ScreenHud.TenantsFitTheirBands` misura una
  scheda piena (nome, stato, equipaggiamento, ricariche dedotte) ma non vede le righe che vanno a capo: l'altezza
  con righe a capo resta da guardare in PIE.
- **Prima consegna in C++, senza `.uasset`**, come `URTContextInspectorWidgetBase` e `URTPieVerdictOverlay`, che
  compongono la propria Slate in `RebuildWidget`. La pelle secondo le tavole viene dopo, e l'autore la giudica su
  tavole.

➕ rev2. L'etichetta di cella è un elemento **nuovo** sopra la scena, e D-497 non la autorizza da sola. Il suo punto 6
dice *che cosa non decide* — la UI nel mondo — e il suo titolo lascia la finestra di reazione come unica
esenzione. La scelta `S4` va quindi scritta nel Decision Log **prima** di F2, come precisazione di D-497 (§9.3,
voce 2). La targa che si apre non chiede niente: è la targa. Il pannello della scheda è un inquilino di fascia, e
entra nel test `ScreenHud.TenantsFitTheirBands`.

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
| **R3** | ➕ rev2. Lo sguardo è l'annotazione di §4.1 del contratto, con un ordine solo; bersaglio del click ed esito dell'hover restano quelli del contratto (§3 regola 1) | in `Targeting` / `Cell` lo sguardo parla dell'unità mentre l'anteprima mostra l'area sulla cella sotto: due informazioni insieme, da giudicare in PIE. Non resta un Ruling: è la voce 5 di §9.3 |
| **R4** | `LMB` su un'unità non comandata fissa la scheda e mostra l'area, senza muovere la camera | due gesti quasi uguali, distinti dalla sola camera |
| **R5** | Una scheda fissata copre il registro anche in Risoluzione | si perde la spiegazione degli eventi mentre si ispeziona |
| **R6** | La portata di un nemico parte dalla cella in cui lo si vede ora | sottostima la minaccia di chi si muove prima di tirare; la zona di minaccia è in §2 |
| **R7** | ➕ rev. Di una cella si legge qualcosa solo se è in vista adesso (P5) | una cella esplorata, dove si sta per andare, non dice i suoi ripari: il giocatore li legge solo dalla resa della mappa. Il rimedio è lo stato ricordato di §2 |
| **R11** | ➕ rev. L'ordine del Back non cambia: `RMB` chiude la scheda prima di uscire da un targeting (§3 regola 8) | un `RMB` in più per disarmare, ogni volta che una scheda è fissata |
| **R12** | ➕ rev2. Nel calcolo dell'area di un nemico una cella non in vista adesso vale come muro | l'area si ferma al bordo di ciò che la squadra vede: sottostima, non rivela |
| **R13** | ➕ rev2. Un soggetto che esce di vista o cade chiude la propria scheda (§3 regola 10) | la scheda sparisce mentre la si legge. L'alternativa, congelarla all'ultimo stato visto, è lo stato ricordato di §2 |
| **R14** | ➕ rev2. In una sessione senza un giocatore che comanda l'ispettore è inerte (§3 regola 12) | una seduta dimostrativa non lo può mostrare |
| **R15** | ➕ rev2. In risoluzione la scheda mostra ciò che mostra la targa, anche se anticipa la scena (§3 regola 5) | chi ispeziona durante il playback legge l'esito prima di vederlo: è il limite di #2453, ereditato |
| **R8** | ➕ rev2. La fonte delle ricariche dedotte sono le voci del TurnLog che passano il filtro del registro, non il proiettore | — è una correzione di fatto, fatta due volte: vedi §5 |
| **R16** | ➕ rev3. La sola lettura della barra ferma **tutti** i punti che scrivono il piano, mappa compresa (§3 regola 14) | in `Pathing` un click su un nemico sposta la barra e blocca i click-waypoint finché non si preme `RMB` o non si clicca una propria unità: da giudicare in PIE |
| **R17** | ➕ rev3. Gli slot di una barra in sola lettura si leggono «disponibile», che vuol dire «niente stato», non «pronta» | un valore nuovo di `ERTActionSlotState` tocca ogni `switch` sugli stati e chiede l'authoring delle tinte; senza, lo slot si legge «pronto» |
| **R18** | ➕ rev3. Nessuna etichetta «sola lettura» sulla barra: il segnale è l'assenza dei tasti sugli slot | il giocatore può non accorgersi che la barra è del nemico; il rimedio è un'etichetta, authoring del grafo o un `Ensure*` da costruire |
| **R19** | ➕ rev3. La scheda tiene la sezione «Ricariche (stima da cio' che hai visto)», che nomina i titoli di alcune abilità | è la stima di F6 ([#3751](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3751)), pubblica per costruzione (P4), e non una riga di kit; se l'autore la vuole sulla barra, è un marcatore sullo slot e un'uscita dalla scheda |
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
| **F1** | **Glossario**: testi d'autore per stati, superfici e ripari, col gate di completezza. Nessuna UI. Implementata in [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744) | — |
| **F2** | **Sguardo**: risolutore, compositore, targa che si apre, etichetta di cella. Issue: [#3747](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3747) | F1; ➕ rev2. le voci 2 e 5 di §9.3 scritte nel Decision Log: sono [D-512](../../decisions/RT_PDR_00_Decision_Log.md) e [D-513](../../decisions/RT_PDR_00_Decision_Log.md) |
| **F3** | **Scheda**: pannello, fissaggio, `RMB`, kit statico ed equipaggiamento. Chiude il buco dell'ispezione che non si vede. Issue: [#3748](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3748). ➕ rev3. Il kit statico non è più sulla scheda: lo mostra la barra, in sola lettura ([D-562](../../decisions/RT_PDR_00_Decision_Log.md)) | F2 |
| **F4** | **`Ctrl` al posto di `Alt`**: la famiglia della camera, e il fuoco sul click. Issue: [#3749](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3749). Il cambio di tasto è fatto, con [D-514](../../decisions/RT_PDR_00_Decision_Log.md); il fuoco sul click no | F3 per il fuoco; il solo cambio di tasto è indipendente |
| **F5** | **Aree del fuoco e hover su skill**: arbitro, area di movimento, portata. Issue: [#3750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3750) | F3, [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511) |
| **F6** | **Ricariche dedotte**. Issue: [#3751](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3751). Il registro (`FRTSeenUseLedger`) e le righe del compositore sono fatti; la riga la disegna il pannello di F3, che manca | F3 |

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

**Scritte**: la voce 2 è [D-512](../../decisions/RT_PDR_00_Decision_Log.md), la voce 5 è
[D-513](../../decisions/RT_PDR_00_Decision_Log.md), le due che `F2` aspetta. La voce 3 è
[D-514](../../decisions/RT_PDR_00_Decision_Log.md), scritta con la prima parte di `F4`. Fa fede il testo del
registro, non quello qui sotto. La voce 1 è [D-533](../../decisions/RT_PDR_00_Decision_Log.md), scritta con `F3`.
La voce 4 è [D-552](../../decisions/RT_PDR_00_Decision_Log.md), scritta prima di `F5` perché la DoD di
[#3750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3750) la chiede accettata prima del codice:
il suo testo vale dal merge della PR che la porta, cioè dall'approvazione dell'autore.
➕ rev3. La regola 7 riscritta e le regole 13 e 14 di §3 sono [D-562](../../decisions/RT_PDR_00_Decision_Log.md), scritta
con la correzione della barra sullo stesso lavoro di `F3`: non è una voce nuova di questo elenco, e fa fede il testo
del registro.

1. **Di un nemico osservato sono pubblici il kit dell'eroe e l'equipaggiamento; le ricariche si leggono solo come
   deduzione dagli eventi autorizzati.** Il piano, gli slot del turno, l'azione armata e le ricariche reali restano
   privati. L'equipaggiamento resta pubblico anche quando diventerà una scelta. ➕ rev2. Supera, per il solo kit, il
   giudizio dei commenti elencati in §9.4; la regola di #2757 sugli slot resta.
2. **Lo sguardo all'hover è UI nel mondo, ancorato al bersaglio.** ➕ rev2. È un emendamento, e un prerequisito di
   F2: D-497 punto 6 dichiara di non decidere la UI nel mondo. Precisa: la targa che si apre e
   l'etichetta di cella stanno sopra la mappa; il pannello della scheda è un inquilino della colonna destra.
3. **Il modificatore della camera è `Ctrl`, e `Ctrl` + `LMB` click mette a fuoco.** Sostituisce `Alt` in tutta la
   famiglia di `spec-tactical-camera.md` §3.2. ➕ rev2. Vale per entrambi i `Ctrl`, come oggi per entrambi gli
   `Alt`.
4. **L'area di movimento mostrata per un nemico è il suo potenziale, mai il suo piano**: ➕ rev2. due aree,
   movimento normale e corsa (`S8`), calcolate su ciò che la squadra vede adesso e disegnate come previsione.
5. ➕ rev2. **Lo sguardo è l'annotazione di `spec-pointer-interaction.md` §4.1**: descrive ciò che sta sotto il
   cursore con un ordine solo, e non cambia né il bersaglio del click né l'esito dell'hover. È un emendamento a
   §4.1, e un prerequisito di F2.

### 9.4 Documenti da aggiornare col lavoro

- `spec-pointer-interaction.md` §5: le righe di `Ctrl` + `LMB` e dell'hover sullo slot; §2 e §5.5 dove nominano
  l'inspector senza produttore. ➕ rev2. E §4.1, dove lo sguardo va nominato come annotazione.
- `spec-tactical-camera.md` §3.2: il modificatore.
- `progettazione-hud.md` §6 e §18: l'inquilino di `MiddleRight` e il contenuto dell'inspector.
- [D-458](../../decisions/RT_PDR_00_Decision_Log.md): il testo nomina `Alt` + destro per il dolly. ➕ rev2. E
  [D-499](../../decisions/RT_PDR_00_Decision_Log.md), punto 3, nomina `Alt` + click.
- ➕ rev2. `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp`: il test che pinna `CameraModifierAction`
  su entrambi gli `Alt`.
- ➕ rev2. I commenti che chiamano fuga il kit di un'avversaria: su `InspectedUnit` e `GetInspectedUnit`
  (`Player/RTPlayerController.h`), in `DispatchUnitClick` (`Player/RTPlayerController.cpp`), in `ResolveOutcome`
  (`Player/RTPointerInteraction.cpp`).
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

- Il risolutore dello **sguardo** rispetta l'ordine di §3 regola 1 su ogni combinazione di candidati. Il click di
  §3 regola 9 è un'altra cosa, e in `Pathing` resta fuori finché #705 non decide.
- Il compositore non porta tipi grezzi, per reflection.
- Il glossario è completo: un gate rosso quando nasce uno stato, una superficie o un riparo senza testo.
- I mondi gemelli di P2, P3, P4, P5 e P7, ciascuno con la propria mutazione.
- L'area di un nemico non cambia fra due piani diversi dello stesso nemico, né fra due mappe che differiscono
  solo in celle che la squadra non vede adesso, mai viste o esplorate. ➕ rev2. Ha due bande, normale e corsa, e
  la seconda manca per chi è sbilanciato.
- ➕ rev. Una cella esplorata e non in vista dà la stessa uscita di una mai vista: nulla.
- ➕ rev. In `ResolutionPlayback` il click su un'unità osservata ispeziona, e nessun gesto cambia il piano.
- ➕ rev. La scheda sopravvive a un gesto di pianificazione; l'area del fuoco no.
- ➕ rev2. Fra la nascita di un'unità e il primo aggiornamento del velo, sguardo, scheda e area non esistono.
- ➕ rev2. Un soggetto che esce di vista o cade chiude la propria scheda e toglie la propria area.
- ➕ rev2. Una ricarica dedotta nasce solo da una voce il cui soggetto agisce: una voce che nomina l'unità come
  altro capo non conta.
- L'arbitro: una sola funzione scrive i setter; chiuso il fuoco, la mappa mostra ciò che mostrava prima.
- ➕ rev2. L'**insieme** delle celle in portata all'hover coincide con quello che la stessa azione mostra armata.
  Il verdetto di tiro no: segue P7.
  ➕ 2026-10-29 ([D-559](../../decisions/RT_PDR_00_Decision_Log.md)): **una sola eccezione**, il Fumo di una cella esplorata e non più in vista, che
  l'azione armata legge (D-534 (3)) e l'hover no. Le celle in più dell'hover sono tutte «senza verdetto».
- Il cambio di tasto: la famiglia risponde a `Ctrl`, e i due `Alt` non sono più mappati. ➕ ⌫ *Diceva «`Alt` non
  produce più nessuno dei suoi effetti»*: su una tastiera italiana `AltGr` arriva come `Ctrl` sinistro più `Alt`
  destro, e arma il modificatore prima e dopo il cambio ([D-514](../../decisions/RT_PDR_00_Decision_Log.md) punto 8).
- Click e rotella sul pannello non raggiungono il mondo.
- Il fuoco e l'ispezione non cambiano selezione, piano, né l'hash dello stato di partita.
- ➕ rev3. La scheda non porta righe di kit: per ogni eroe del roster, con e senza equipaggiamento e senza un uso
  visto, nessuna riga è il titolo di una skill, o il titolo con la sua frase, e l'equipaggiamento c'è. Le ricariche
  dedotte nominano un titolo, ma nascono solo da un uso che la squadra ha visto (P4).
- ➕ rev3. La barra di un soggetto in sola lettura porta il catalogo e niente della partita: piano, ricarica vera,
  azione armata, tasto e `TotalTurns` assenti, con il controllo positivo sulla stessa unità letta da
  `BuildAbilityCooldowns`.
- ➕ rev3. Il soggetto della barra segue l'ultimo click su un'unità che non si comanda, e torna alla selezione con
  `SelectUnit` (anche sull'unità già selezionata), `N` con una sola comandabile, `RMB`, e quando il soggetto cade o
  esce di vista; un nemico senza voce `Live` non ha barra, anche prima che la scheda si rilasci.
- ➕ rev3. Con la barra in sola lettura nessun modo di scrivere il piano passa (tasti, generiche, slot, click su
  cella e su unità, `Sneak`, `Invio`, selettore del verso), e il log dice perché; a scheda chiusa lo stesso gesto
  scrive. Selezione, ispezione, `RMB` e il click D-528 sul pavimento restano vivi.
- ➕ rev3. Uno slot costruito per un'altra unità non arma e non registra hover; la portata dall'hover di uno slot del
  nemico fissato parte dalla cella in cui lo si vede, non dal piano.

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
- ➕ rev3. **La barra in sola lettura può sembrare rotta.** Nessuna etichetta (R18): se l'unità cliccata non è
  quella che si comanda, il segnale è l'assenza dei tasti, e la riga di stato del dock (`UpdateTargetPrompt`, nel
  grafo) segue ancora la selezione e può dire «Scegli un'azione». Il rimedio è authoring del grafo nel clone
  principale o una modifica a `GetPointerContext`, e non è fatto.
- **Con `Ctrl` premuto la vista fa il peek.** Avvicinandosi a un bersaglio per il click, il bersaglio si sposta
  sotto il cursore. Succede già oggi con `Alt`; col fuoco il click diventa più frequente.
- ➕ rev. **La regola stretta sulle celle (P5) può sembrare un difetto a chi gioca**: una cella che si vede sulla
  mappa, attenuata perché esplorata, non risponde all'hover. Va detto a schermo, o il giocatore conclude che
  l'ispettore sia rotto.

---

## 13. ➕ rev. I difetti trovati dalla revisione

| | Difetto della prima stesura | Seconda fonte che lo ha trovato | Dove è corretto |
|---|---|---|---|
| **V1** | Trattava la mappa statica come pubblica | D-225 e D-227; `FRTTeamKnowledge` | §1.2, §4, P3, P5, R7, R12 |
| **V2** | Dava per funzionante l'ispezione durante la risoluzione | `URTPointerLibrary::ResolveOutcome` | §1.2, §3 regola 5 |
| **V3** | Chiudeva il fuoco a ogni gesto di pianificazione | la decisione del 2026-09-11 nel commento di `InspectedUnit` | §3 regola 4 |
| **V4** | Non diceva quale degli otto esiti produce ogni gesto nuovo | `spec-pointer-interaction.md` §5: l'elenco è chiuso | §3, tabella degli esiti |
| **V5** | P1 chiedeva una «proiezione per osservatore» che porti la condizione, e non esiste: `FRTKnowledgeEntry` la esclude per scelta | `Perception/RTKnowledgeView.h` | P1 |
| **V6** | Non diceva che `RMB` chiude la scheda prima di un targeting | l'ordine di `ERTPointerBackStep` | §3 regola 8, R11 |
| **V7** | Distingueva tiro libero e bloccato su ogni cella in portata | D-225 | §4, P7 |

Che cosa la revisione **non** ha coperto: la resa a schermo di qualunque cosa; i widget Blueprint, che il confronto
col C++ non vede; e ogni premessa di §6.4, che resta da misurare nel piano della propria fetta.

---

## 14. ➕ rev2. I rilievi del revisore avversario

Un agente con contesto fresco, in sola lettura, con un tetto di trenta chiamate. Ogni rilievo è stato riletto sul
codice da chi scrive prima di entrare nel testo.

| | Gravità | Difetto | Seconda fonte | Dove è corretto |
|---|---|---|---|---|
| **A1** | critico | L'area di un nemico avrebbe letto la banda del suo piano | `ARTTurnManager::MakeSimUnit` | §1.2, P3, `S8`, §9.3 voce 4 |
| **A2** | critico | P3 e P7 dicevano «celle note», che comprende le esplorate: una porta aperta fuori vista avrebbe cambiato area e verdetto | D-227; `UI/RTPlayerEvent.h` | §4, P3, P7, R12, §11 |
| **A3** | critico | Il flag di conoscenza nasce acceso: usato come cancello, mostra ogni nemico fino al primo aggiornamento del velo | `ARTUnit::bKnownToObserver`; D-466 | §1.2, §1.4, P2, §7, §11 |
| **A4** | critico | «Ordine dello sguardo uguale in ogni contesto» contraddiceva §4.1 e la colonna Hover del contratto, e D-128 | `spec-pointer-interaction.md` | §3 regola 1, tabella degli esiti, R3, §9.3 voce 5 |
| **A5** | importante | Il proiettore del registro non può essere la fonte delle ricariche dedotte | `URTPlayerEventProjector::Project`; `IsSubjectTheSufferer` | §1.2, P4, R8, §6.1, §6.4 |
| **A6** | importante | La scheda fissata sopravviveva a un soggetto uscito di vista o caduto | `ARTPlayerController::InspectedUnit` | §3 regola 10, R13 |
| **A7** | importante | «`LMB` fuori da `Targeting` ispeziona» era troppo largo: non vale in `Facing`, e in `Pathing` codice e contratto divergono | D-128; `ResolveTarget` | §3 tabella e regola 9 |
| **A8** | importante | §11 chiedeva alla portata all'hover di coincidere con quella armata anche nel verdetto, contro P7 | §5 della spec stessa | §11 |
| **A9** | importante | L'osservatore può essere un insieme di squadre, e in una sessione non presidiata nessuno comanda | `ResolveObserverTeamIds` | §3 regola 12, R14 |
| **A10** | minore | Finestra di reazione e modale vengono prima di `Inspector` nel Back | `URTPointerLibrary::ResolveBack` | §3 regola 11 |
| **A11** | minore | §9.4 non elencava tutto ciò che nomina `Alt` | D-499; `RTPlayerInteractionTests.cpp` | §9.3 voce 3, §9.4 |
| **A12** | minore | D-497 punto 6 non elenca una famiglia: dichiara ciò che non decide | D-497 | §7, §9.1, §9.3 voce 2 |
| **A13** | minore | P6 rovescia più di un commento, e tocca la regola di #2757 | i commenti di §9.4 | §5, §9.3 voce 1, §9.4 |

**Controllato dal revisore, e regge**: l'ordine del Back (`V6`); `ResolveOutcome` in risoluzione (`V2`); `Shift` e
`Ctrl` non mappati; `SetInspectorPinned` senza produttore; D-225 e D-227 (`V1`); D-466; D-478; D-458; il registro
come inquilino di `Zone_MiddleRight`; nessuna decisione trovata che dichiari privati kit o equipaggiamento.

**Verificato da chi scrive dopo la revisione**, perché il revisore aveva finito il tetto: il cancello di `OnSelect`
in risoluzione (`IsWorldReadOnly`); il pivot che accetta ogni cella della mappa; la risoluzione che precede il
playback.

**Un secondo revisore avversario**, sul branch finito, ha aggiunto due cose a questa spec: il costo di `Status.Slow`
e il divieto di corsa di `Status.Unbalanced` non passano da `GetEffectiveMoveRange` (P3), e il test del risolutore
va distinto dal click in `Pathing` (§11). Ha anche confermato che le regole 10, 11 e 12 di §3, P2 e P4 non si
contraddicono fra loro né con §11.

**Ancora non verificato**, e scritto dove serve in §6.4:

- su chi si congela il verdetto di una voce di combattimento, e l'`ActionId` di una voce di stato;
- se tutti gli stati di un nemico osservato sono pubblici, e gli ingressi di `GetEffectiveMoveRange`;
- se `URTSelectedUnitPanelWidget::GetSubject` controlla la conoscenza;
- quale zona le tavole assegnano all'inspector del bersaglio;
- vista strategica, cambio del piano attivo, un'unità sopra un obiettivo.
