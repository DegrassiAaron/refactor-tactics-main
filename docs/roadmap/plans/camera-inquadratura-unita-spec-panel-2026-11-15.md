# Spec panel — l'inquadratura d'unità (`T`) e la camera del banco — 2026-11-15

> **Referto**, non owner. Il contratto sta in [`spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md)
> §4.2, la decisione in [D-618](../../decisions/RT_PDR_00_Decision_Log.md). Tracking:
> [#4121](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4121), sotto E49
> ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)).

## 1. Il mandato

Alla lettera (sessione `98b955f2-a994-4269-bd33-59fe4c63697f`, `/sc:spec-panel`):

> «dobbiamo definire una posizione della telecamera centrata sul personaggio, cosi possiamo con un tasto centrare la
> telecamera. sara' utilizzata come posizione nel replay action. deve essere posizionata per vedere il personaggio
> selezionato e una inclinazione verso dove e' voltato»

## 2. Cosa c'era, misurato su `main` (`df2ec3289`)

- **`F`** chiama `FocusOn` (`Source/RefactorTactics/Camera/RTCameraPawn.h:439`) sulla cella dell'unità selezionata
  (`ARTPlayerController::FocusCameraOnUnit`), e **conserva** yaw, pitch e zoom.
- **`Home`** chiama `FrameHome` (`RTCameraPawn.h:485`): selezione propria viva, poi squadra, poi centro mappa, con
  orientamento e zoom **azzerati**. È la scelta dell'autore del 2026-10-15, spec della camera §4.1.
- **Il verso** è `ARTUnit::Facing`, un `ERTHexDirection` (`Source/RefactorTactics/Map/RTCellId.h:11`).
  `URTHexLibrary::FacingRotation` (`Source/RefactorTactics/Map/RTHexLibrary.h:178`) lo porta in uno yaw di mondo: con
  la griglia pointy-top di `AxialToWorld`, E vale 0° e gli altri versi multipli di 60°.
- **Il playback non muove la camera**: `Grep "Camera|FocusOn"` su `Source/RefactorTactics/Turn/Playback/` non trova
  niente. La muovono i tasti del controller e, all'apertura di uno scenario, `ARTGameMode::RecenterCameraOnScenario`,
  che chiama `RecenterView` (la mappa intera).
- **Il banco** costruisce la scena `AbilityLab.<AbilityId>` con `URTAbilityLabLibrary::BuildFixture`: chi lancia è
  l'unità `CASTER` della squadra 0, in `(0,0)` col verso di default E, e il bersaglio è `TARGET`, in `(2,0)`.
- **Tasti**: `T` non è legato. `C` è Overwatch, `G` Guard, `B` Brace, `X` Interact, `Z` Wait
  (`Source/RefactorTactics/Player/Input/RTInputKeys.cpp`).

## 3. I rilievi, e che cosa ne resta dopo le risposte

| # | Lente | Rilievo | Dopo le risposte |
|---|---|---|---|
| 1 | Wiegers | «replay action» aveva due significati nel progetto (il playback del turno, il Replay Viewer), e il requisito non si poteva verificare | chiuso: è il banco Ability / Hero Lab, un terzo significato (D-618 (1)) |
| 2 | Fowler | muovere la camera nel playback di una partita è `CAM-12` ([#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781)), che [D-287](../../decisions/RT_PDR_00_Decision_Log.md) mette dopo la v0.1, con `KEEP` di default e senza rotazioni automatiche | resta vero per la partita. Il banco mette la camera una volta, all'apertura, come già faceva: non è `CAM-12`, lettura dell'assistente |
| 3 | Nygard | la camera è un canale di conoscenza (D-287 (5)) e nel Blast le azioni sono simultanee | non riguarda il banco, che è un'osservazione ([D-607](../../decisions/RT_PDR_00_Decision_Log.md)) con un'azione sola; in partita `T` inquadra solo un'unità propria |
| 4 | Wiegers | «verso dove è voltato» non diceva quando | il verso logico `Facing` al momento del tasto o dell'apertura: lettura dell'assistente |
| 5 | Adzic | mancavano gli esempi della posa e della distanza | l'unità in basso, il campo davanti, distanza fissa (D-618 (3)) |
| 6 | Cockburn | cambiare `F` o `Home` toglieva un contratto esistente | `T` nuovo, `F` e `Home` invariati (D-618 (2)) |
| 7 | Crispin | i casi di bordo: unità sul bordo, su un altro piano, vista strategica, nessuna selezione, unità morta o avversaria, peek | scritti in §4.2 della spec, coi criteri |
| 8 | — | D-142 tiene lo scatto a 45° per non agganciare la vista agli assi; uno yaw dal verso cade su un asse | frizione, non violazione: D-142 governa `Q`/`E` |
| 9 | Nygard | determinismo | è presentazione; il gate esiste già, `RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback` |

## 4. Le risposte

Domande, opzioni e risposte, alla lettera, sono nella riga [D-618](../../decisions/RT_PDR_00_Decision_Log.md). In breve:
il replay è il banco; il tasto è `T`; l'unità sta in basso col campo davanti, a distanza fissa; il banco apre così ogni
scena su chi lancia, e `T` ci riporta; tutto nella v0.1; il codice parte subito in un worktree, e la sua PR aspetta il
merge della spec.

## 5. Fuori da qui

- La camera del playback di una partita: `CAM-12`, [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781), dopo la v0.1.
- Il Replay Viewer in 3D: [#4089](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4089).
- La taratura della distanza e dello spostamento in avanti: [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780), con il primo valore scelto nel banco.
