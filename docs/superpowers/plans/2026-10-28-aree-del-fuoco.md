# F5 — le aree del fuoco: piano di implementazione

**Issue**: [#3750](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3750) · **Spec**: `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, fetta `F5` (§4, §5 regole P3 e P7, §6.3) · **Decisione**: [D-552](../../decisions/RT_PDR_00_Decision_Log.md), approvata dall'autore il 2026-10-28.

**Goal:** di un nemico osservato si disegnano due aree di movimento (normale e corsa) calcolate sul suo potenziale e mai sul suo piano, e all'hover di una sua skill la portata a tre stati; tutto passa da un arbitro che dice quale sorgente dell'anteprima è attiva.

**Dipende da**: #3511 (un solo ingresso e un solo reset per le aree di pianificazione) — **chiusa il 2026-10-27**. Il pannello della scheda (F3, #3748) **non c'è**: la metà della DoD che parla della scheda («l'area del fuoco lascia la mappa al primo gesto di pianificazione, la scheda resta») non si chiude senza.

## Il taglio

Quattro fette, ciascuna verificabile in Automation e con la sua PR. L'ordine non è casuale: A e B sono funzioni pure e si provano a mondi gemelli senza cablaggio; C le collega; D produce un segnale che oggi non esiste.

| | Fetta | Che cosa | Stato |
|---|---|---|---|
| **A** | Il calcolo puro dell'area di un nemico | `URTFocusAreaLibrary` (`Player/RTFocusArea.*`): le due bande dal movimento base, le celle non in vista come muri nel calcolo, `Slow` e il divieto di corsa riportati, mai il piano | **fatta** |
| **B** | La portata a tre stati | una funzione accanto a `TargetableRangeCells` che per ogni cella dà *libero*, *bloccato* o *senza verdetto*, con `VisibleCells` in ingresso; l'insieme coincide cella per cella con quello dell'azione armata | da fare |
| **C** | L'arbitro e il cablaggio | un enum di sorgente (`Plan`, `Focus`, `SkillHover`) dentro `FRTPlanningOverlayInputs`, `RefreshPlanningPreview` come unico scrittore, il secondo canale di movimento sulla mappa | da fare |
| **D** | L'hover su una skill | il produttore dello stato: `NativeOnMouseEnter`/`NativeOnMouseLeave` sullo slot e sulla riga di kit, e una porta sul controller | da fare |

## Fatti misurati (su `origin/main` = `f666f389e`)

* L'arbitro di #3511 vive in `FRTPlanningOverlayInputs` (`Player/RTPlanningOverlay.h`): `GatherPlanningOverlayInputs` → `ApplyPlanningOverlay` → `RefreshPlanningPreview`, confronto con `LastPlanningOverlayInputs`. **Non esiste una variabile di sorgente**: l'unica sorgente implicita è l'unità selezionata. I due ingressi nuovi (il soggetto a fuoco, la skill in hover) vanno in `SamePlanLevel` e in `operator==`, perché cambiano cosa si calcola.
* `SetPreviewReachableCells` scrive **un solo** `FRTOverlayArea`: due aree richiedono un secondo membro o un array con `Certainty`. Il disegno di `RTHexMapActor` non legge `Certainty` per le aree di celle.
* **Per l'hover su una skill non esiste nessun produttore**: nessun `NativeOnMouseEnter` fuori dai test. Va inventato, ed è la fetta D.
* `ReachableCells` legge solo la mappa dello snapshot, la cella, `MoveBudget` e `MoveCostModifier`; l'occupazione è vuota (D-445, D-446). Per questo basta uno snapshot con la sola unità del soggetto.
* ⚠️ **Il rischio di privacy più concreto**: usare `PlanningSnapshotFor` per il nemico, che è comodo e ha già budget e tetto. Legge `PlannedMovementProfileId` e la riserva del piano, e la mappa ottimistica (celle non viste *libere*). A forma invariata nessun altro test se ne accorgerebbe. Per questo il soggetto della fetta A è fatto di valori, e c'è un test con due unità dal piano diverso.
* Il divieto di corsa di `Status.Unbalanced` vive in `URTMovementProfileLibrary::CeilingProfile(..., bRunDenied)`, non in `MakeSimUnit` (dove `Unbalanced` diventa solo celle di scivolamento): la spec è stata corretta con D-552.
* `URTEnemyTacticalQueryLibrary::RegionsFor` **non è riusabile**: usa il movimento di catalogo e un solo budget, calcola sulla mappa intera, e dichiara che `Slow` non restringe la regione. D-552 dice il contrario (lo `Slow` è pubblico e si riporta).

## Scelte della sessione dove la spec tace

Non sono decisioni d'autore, e valgono finché `PIE-HUD-SGUARDO` o l'autore non dicono altro.

1. **Un'unità propria o alleata a fuoco** non è coperta da D-552: ne parla solo per i nemici. La propria comandata ha già l'anteprima di pianificazione. Resta fuori da F5.
2. **Il fuoco in `ResolutionPlayback`** resta spento: `ShowsAreas()` spegne le aree in playback, e la scheda resta. Da giudicare a schermo.
3. **La corsa si disegna al netto della normale** (le celle raggiungibili *solo* correndo). La libreria restituisce entrambe per intero, quindi la scelta è del disegno e si cambia senza toccare il calcolo.
4. **Il verdetto di tiro del terzo stato si giudica sulla mappa autorevole, ma solo per le linee interamente su celle in vista**: dove la linea attraversa una cella non in vista il verdetto non si consulta affatto (è lo stato «senza verdetto»), quindi nessuna geometria nascosta viene letta.

## Test a mondi gemelli (P2, P3, P5, P7)

Ogni regola ha un test in cui due mondi differiscono *solo* in ciò che la squadra non vede adesso e l'uscita è identica, più un **controllo positivo** (un cambiamento in ciò che si vede *cambia* l'uscita) e una **mutazione** (tolta la guardia, il test diventa rosso).

* **A (fatta)**: geometria mai vista; celle esplorate e non più in vista; piano diverso dello stesso nemico; cella del soggetto non in vista; celle non in vista come muri *nel* calcolo e non come ritaglio del risultato (la cella in vista a 3 passi raggiungibile solo attraverso la cintura deve mancare).
* **B**: stessa `VisibleCells`, geometria nascosta diversa → stessi verdetti; una linea che attraversa una cella non in vista è sempre *senza verdetto*; mutazione `VisibleCells ∪ ExploredCells` → rosso.
* **C**: tabella pura di `ActiveSource`; il fuoco lascia la mappa al primo gesto di pianificazione e la scheda resta; chiuso il fuoco la mappa torna com'era; il contatore di #3511 non ricalcola se gli ingressi non cambiano.
* **D**: la porta e il segnale; l'hover che sfarfalla non ricalcola. La resa resta `PIE`, `NOT RUN` finché non si esegue.
* **Unità ignote**: non cambiano l'area (D-445, D-446: l'occupazione non entra nel ventaglio). Si pinna in C, con il controller vero.

## Fuori da questa fetta

La zona di minaccia dopo il movimento; l'area d'effetto sotto il cursore con un'azione armata (#3512, che usa l'arbitro di F5); la correzione del ventaglio dell'unità comandata, che oggi è calcolato sulla mappa ottimistica (domanda per l'owner della conoscenza parziale).
