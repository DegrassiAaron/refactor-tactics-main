# Il rallentatore del banco vale per tutto, e R rifà la scena — disegno

> **Data**: 2026-11-20. **Stato**: approvato dall'autore il 2026-11-20 («si»). **Issue**: [#4166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4166). **Decisione**: `D-632`
> (numero libero su `main` = `4bef64516` e sui rami remoti il 2026-11-20; si rimisura quando si scrive la riga).
> Emenda [`D-607`](../../decisions/RT_PDR_00_Decision_Log.md) (1), la velocità del banco, e corregge un'affermazione
> di [`D-621`](../../decisions/RT_PDR_00_Decision_Log.md) su `R`.
>
> ➕ **2026-11-20, nota dopo l'implementazione: una premessa di questo disegno era sbagliata** (il testo approvato non si
> riscrive). Il disegno dà per scontato che i Niagara di unità e arco seguano `CustomTimeDilation` (§2, «effetti attaccati
> alle unità»; §3, «stesso risultato a schermo»; §4, «Le unità» e «L'arco»). Letto nel sorgente di Engine 5.8, nel modo di
> default non è così: in modalità batch Niagara ignora la dilatazione dell'attore. Vale invece:
>
> - **L'arco**: con dilatazione minore di 1 i componenti Niagara che l'arco ha alla nascita sono forzati in modalità «solo»
>   (`SetForceSolo(true)`, chiamata per riflessione: il C++ resta senza il modulo Niagara) con la propria dilatazione a 1, e
>   secondo il sorgente di Engine 5.8 il totale è la dilatazione dell'attore. `Playback.ChainArcNiagaraIsForcedSoloInTheBench`
>   misura lo stato (modalità «solo» forzata, dilatazione del componente a 1), non l'effetto a schermo: quello lo giudica
>   `PIE-LAB-BERSAGLI` (7).
> - **I Niagara che le notify delle clip attaccano alle unità** (per esempio il rilascio `NS_Gadget_LinearDischarge_Release`)
>   nascono a metà animazione e non sono raggiunti: nel banco restano a velocità normale. È un limite dichiarato in
>   [`D-632`](../../decisions/RT_PDR_00_Decision_Log.md) e un seguito candidato, non lavoro di questo disegno.
>
> Le clip delle unità e i componenti ordinari dell'arco, come la sua Timeline, seguono la dilatazione come previsto.

## 1. Perché

L'autore, alla lettera, il 2026-11-19 e il 2026-11-20, guardando LinearDischarge nel banco:

1. «C'e' un animazione dove Gadget muove il braccio (vorrei poterla far bene), poi un secondo fermo, e poi parte la scarica»
2. Dopo la prova a velocità normale: «R non fa ripartire la scena ma a x1 sembra ok l'animazione»
3. Alle due domande: «R rifà la scena» e «Rallenta tutto».

**Le cause, misurate**:
- **Il secondo fermo esiste solo nel banco.** La velocità del banco, x0,5, scala solo l'orologio del playback
  (`RTPlaybackDirector.cpp:1390`, `Dt = DeltaSeconds * EffectivePlaybackSpeed`). Le clip partono a velocità normale
  (`ARTUnit`, `PlaySlotAnimationAsDynamicMontage`, `RTUnit.cpp:1102`), e così il Niagara e la Timeline dell'arco.
  Ogni battito del Blast (`AttackShowSeconds` = 0,5 s) nel banco dura 1 s. La `Cast` dell'attivazione finisce presto,
  e il battito dell'impronta resta vuoto. Log della seduta (3 bersagli): `Attiva:` a 23,90, primo colpo arrivato a
  26,39, poi uno al secondo. A x1 l'autore non vede la pausa.
- **`R` nel banco non fa niente.** `ARTPlayerController::OnRestart` ricarica il livello solo se la fase è `MatchEnded`.
  Una scena del banco gioca un turno e finisce senza che la partita finisca. `D-621`, il §2.10 del profilo FX, la
  voce `PIE-LAB-BERSAGLI` e #4164 dicono invece che `R` ricarica la scena: la premessa era stata letta dal solo
  binding del tasto.

## 2. Obiettivi, non obiettivi, anti-obiettivi

**Obiettivi**
- In una scena del banco tutto ciò che si vede va a x0,5 insieme: battiti, animazioni delle unità, effetti attaccati
  alle unità, archi della scarica. A x0,5 si vede la stessa sequenza di x1, più lenta.
- In una scena del banco `R` rifà la scena in qualsiasi momento, con lo stesso file, quindi con lo stesso numero di
  bersagli.

**Non obiettivi**
- Le velocità della partita (`x1 · x2 · x4`, [`D-350`]) e il rallentatore della finestra di reazione fuori dal banco.
- L'animazione del braccio in sé (una `Cast` propria di LinearDischarge e il rilascio al fotogramma giusto): sono i
  passi 3 e 4 della guida VFX, lavoro d'Editor dell'autore.

**Anti-obiettivi**
- `SetGlobalTimeDilation` o `SetPause`: li vieta CP 46.6 (#941, `RTFrontendNavigator.h`) con un criterio che conta zero
  chiamate in `Source/`. In rete nessuno deve poter rallentare il tempo di tutti.
- Qualunque cosa entri nel TurnLog, nell'hash o nello stato canonico.
- `R` che riavvia una partita normale a metà.

## 3. Approcci

**Come rallentare tutto.** L'autore ha scelto il risultato: «Rallenta tutto».
- *Tempo del mondo rallentato* (`SetGlobalTimeDilation`), quello descritto nella domanda: scartato come mezzo, perché
  vietato dalla regola di #941, e avrebbe rallentato anche la camera.
- *Una dilatazione per attore* (`AActor::CustomTimeDilation`) sulle unità e sugli archi del banco (scelta
  dell'assistente): stesso risultato a schermo, la camera resta pronta, nessuna regola toccata.
- *Solo le clip* (scartato dall'autore): Niagara e arco resterebbero più veloci dei colpi rispetto alla partita.

**Come rifare la scena.** L'autore ha scelto `R`.

## 4. Disegno

- **I battiti restano come oggi**: `D-607` (1), x0,5 tramite `rt.Debug.PlaybackSpeed`, `ViewerPlaybackSpeed` e
  `BenchPlaybackSpeed` del TurnManager.
- **Le unità**: in una scena del banco ogni unità ha `CustomTimeDilation` = `BenchPlaybackSpeed`. Così rallentano le
  animazioni e i Niagara attaccati all'unità, come il rilascio. Fuori dal banco (`BenchPlaybackSpeed` = 0) resta 1.
- **L'arco**: nel banco anche l'attore dell'arco nasce con la stessa dilatazione. Il timer della vita
  (`SetLifeSpan`) non segue la dilatazione dell'attore, quindi la vita diventa `LifeSeconds / BenchPlaybackSpeed`:
  altrimenti l'arco sparirebbe a metà crescita.
- **`R`**: se la partita è finita, ricarica come oggi. Altrimenti chiede al GameMode di rifare la scena del banco, che
  ricarica il livello con lo stesso `rt.Test.Scenario`. Stesso file, quindi stesso tag `lab-targets-N`, e la riga di
  log `[RT-Test] banco: la scena riparte`. Fuori dal banco è inerte, come `,`, `.` e `U`: decide il GameMode.
- **Documenti**: la riga `D-632`; la nota in `D-607` (1) e in `D-621` su `R`; il §2.10 del profilo FX; la voce
  `PIE-LAB-BERSAGLI`, che guadagna l'osservazione «a x0,5 nessuna pausa che a x1 non ci sia».

## 5. Prove

- **Automatiche**:
  - in una scena del banco le unità hanno la dilatazione del banco, fuori dal banco hanno 1;
  - l'arco nato nel banco ha la dilatazione e la vita allungata;
  - `R` fuori dal banco e a partita in corso non ricarica, e nel banco sceglie di rifare la scena;
  - il criterio di #941 (`SetGlobalTimeDilation(` / `SetPause(` fuori dai commenti) resta a zero;
  - la suite intera resta verde.
- **PIE**: `PIE-LAB-BERSAGLI`, LinearDischarge a x0,5 senza il secondo fermo, e `R` che rifà la scena.

## 6. Lasciato a chi costruisce

- Dove si imposta la dilatazione delle unità (all'avvio della scena o del playback), purché preceda la prima clip.
- I nomi delle funzioni nuove.
