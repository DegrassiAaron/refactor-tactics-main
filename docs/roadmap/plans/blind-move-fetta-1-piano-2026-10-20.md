# Blind Move, fetta 1 — pianificare un Move verso lo spazio non osservato

> `SNAPSHOT` · fotografia del 2026-10-20: vale finché è l'ultima misura del suo oggetto.

**Data**: 2026-10-20 · **Misurato su**: `main` `abe0dffd0` · **Issue**: [#2794](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2794) · **Decisioni**: [D-249](../../decisions/RT_PDR_00_Decision_Log.md), [D-371](../../decisions/RT_PDR_00_Decision_Log.md), [D-372](../../decisions/RT_PDR_00_Decision_Log.md), [D-432](../../decisions/RT_PDR_00_Decision_Log.md), [D-465](../../decisions/RT_PDR_00_Decision_Log.md)

> Non è una decisione: nessuna riga di questo documento cambia il Decision Log. Dice **come** si attua ciò che D-372 ha già deciso, e dove la misura dice di fermarsi.

## 1. Che cosa è già deciso, e che cosa no

| | Stato | Dove |
|---|---|---|
| L'occupazione autorevole non è pubblica, bot compreso | deciso | D-371 |
| La **forma** della board è pubblica, il **contenuto** no | deciso | D-373 |
| Un Move verso l'ignoto si pianifica su mappa ottimistica, tracciato oltre il noto dichiarato incerto | deciso | D-372 |
| L'osservatore è un ingresso di `MakeSnapshot` e nessun default silenzioso | deciso, attuato | D-432 |
| Nessun auto-reroute in Resolution | deciso | D-045, #2501 |
| Click e portata leggono la mappa piena finché #2794 non porta la vista ottimistica | canale dichiarato, a termine | D-465 |
| Celle **esplorate ma non visibili ora**: quale geometria? | **aperto** | `Q4` del referto delle aree di contesto |
| Quale osservatore sono i bot | non toccato qui | `OBS-1` |

Il blocco che #2794 dichiarava (`BLIND-2` → `OBS-1`) è caduto: D-432 chiude `BLIND-2` e passa l'osservatore come `int32` che porta la posizione nominata già esistente. `OBS-1` resta aperta con scadenza lontana e non blocca un Move cieco.

## 2. Che cosa esiste già

- **La regola d'arresto è la canonica e non si scrive.** In Resolution il percorso pianificato si cammina e `TruncatePathToTopology` (`Turn/RTTurnManager_Movement.cpp`, dopo `ApplyIceSliding`) lo ferma all'ultima cella valida, scrivendo `bStoppedByTopology` e l'esito `BlockedByTopology`.
- **Resolution è onnisciente.** Il suo snapshot nasce da `MakeSnapshotOmniscient`; cambiare la geometria dei soli snapshot per osservatore non la tocca.
- **Il precedente della copia con celle vuote** è `URTEnemyArcLibrary` (#3762): ogni cella mai vista esiste ancora ma è vuota. Non è una seconda visita del grafo.
- **Il canale oggi è misurato.** `RefactorTactics.BlindActions.NeverObservedWallBendsTheLitPath` è verde perché il canale c'è, e si rovescia in questa fetta.

## 3. Il disegno

1. **Geometria ottimistica nello snapshot di pianificazione.** `URTHexSimLibrary::MakeSnapshot` con un osservatore diverso da `RTObserver::Omniscient` e con conoscenza leggibile costruisce `Snapshot.Map` come copia della mappa in cui ogni cella **mai osservata** (né in `VisibleCells` né in `ExploredCells`) esiste ancora ma è vuota: non blocca il passo, costo base, niente copertura né porta né muro interno né superficie. È lo stesso `URTHexPathLibrary`, con un ingresso diverso.
2. **Le celle esplorate ma non visibili ora** (`Q4`, aperta) restano sulla mappa autorevole. Non esiste un modello «ultima superficie vista» e costruirlo è fuori scope. È un canale dichiarato, a termine, della stessa natura di D-465: un muro caduto o una porta aperta in una cella ricordata si riflette nel percorso.
3. **Resolution invariata.** Il piano si cammina e si ferma dove la verità lo ferma. Nessun reroute.
4. **Il motivo dell'arresto.** `BlockedByTopology` dice «fermo: il varco si è chiuso», falso per un muro mai visto. Serve un esito proprio, aggiunto **in coda** all'enum perché il valore serializzato è l'indice.
5. **Il tracciato incerto.** La parte oltre il noto si disegna diversa. È presentazione: una fetta a parte, misurata in PIE.

## 4. Sequenza e criteri

| Fetta | Contenuto | Criterio binario |
|---|---|---|
| **A** | Geometria ottimistica in `MakeSnapshot` per osservatore; rovesciamento del canary del movimento | Canary A/B/C: cella vuota, un nemico ignoto, due nemici ignoti → **stessa** anteprima, **stesso** costo, **stesso** reason code, con la premessa esplicita che il piano non è vuoto. Un muro mai osservato **non** piega più il tratto illuminato |
| **B** | Resolution con muro reale | Un impedimento reale ferma il movimento; un test diventa rosso se il Move ricalcola verso la destinazione (`D-045`, `#2501` restano veri) |
| **C** | Motivo dell'arresto leggibile a posteriori | Un esito in coda all'enum, un testo che non dica «varco chiuso», e nessuna informazione sull'occupante prima della Resolution |
| **D** | Resa del tracciato incerto | Verdetto dell'autore sulle tavole; non è una regola di gioco |
| **Fetta 2, a parte** | Click e portata con la stessa vista, riga di TurnLog per il tiro scartato in un Fumo ignoto, rovesciamento del canary di #3270 | Tocca schema, replay e digest: ha il proprio piano |

## 5. Rischi, con la misura che li chiude

- **I bot pianificano su uno snapshot per osservatore** (D-432): con A cambiano comportamento. È il rischio che il mortaio di #3093 ha appena mostrato: `Bot.StallDefinitionsOnTheGeneratedTestArena` e `Match.Autobattle` sono gate sensibili al margine anti-stallo. **Misura prima di A e dopo**, con la suite intera.
- **Costo di una copia della mappa per snapshot.** Si misura a motore libero, come chiede il referto delle aree di contesto.
- **Costi mostrati.** Una cella mai vista con costo reale 3 appare a costo 1: il percorso mostrato può costare meno del reale e `TruncatePathToBudget` in Resolution può fermarlo prima. Il motivo dell'arresto lo dichiara.
- **Hash e revisione della mappa.** Lo snapshot cattura hash e revisione della mappa autorevole: la copia ottimistica non deve cambiarli, o i confronti di coerenza fra snapshot si rompono.
- **Privacy.** L'insieme delle celle vuotate dipende solo dalla conoscenza dell'osservatore, mai dalla posizione delle unità nemiche: si verifica sulla firma della funzione, non sull'assenza a schermo.

## 6. Non misurato

- Quanti siti leggono `Snapshot.Map` assumendo che sia autorevole: il file `Turn/RTHexSimLibrary.cpp` ne ha una trentina, non classificati uno per uno. Il censimento è il primo passo di A.
- Che il click su una cella mai vista ordini oggi un movimento: nel controller non c'è una guardia esplicita, ma non è provato.
- Il costo della copia della mappa.

## 7. Decisioni che servono all'autore, con il default scelto qui

| Domanda | Default usato per A–C | Perché |
|---|---|---|
| Celle esplorate ma non visibili (`Q4`) | mappa autorevole, canale dichiarato a termine | non esiste un modello «ultima superficie vista» |
| Bot sulla stessa geometria del giocatore | sì, dopo la misura dei gate; se la misura la boccia, il bot resta sulla vista attuale e lo dichiara come D-532 | un bot che pianifica diversamente dal giocatore è il difetto che D-371 ha chiuso |
