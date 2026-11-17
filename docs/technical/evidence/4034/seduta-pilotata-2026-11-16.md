# Seduta pilotata dall'assistente: il fumetto barrato e la sua riga nel Registro (2026-11-16)

**Che cosa è.** Due tavole di una finestra di gioco (`-game`, non la PIE dell'Editor) per la voce `PIE-HUD-FUMETTO-BARRATO` di
[`test-manuali-pie.md`](../../test-manuali-pie.md), che ne possiede l'esito. ⚠️ **Chi misura ha scritto il codice** di
[#4034](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4034) e di
[#4043](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4043), e ha disegnato il taglio nell'asset: è la meccanica, non
un verdetto indipendente né il giudizio dell'autore.

## Condizioni

* **Commit**: `main` `7672a15ad` più `WBP_RT_UnitOverlay.uasset` col taglio, sul ramo `feat/4034-taglio-asset` del clone principale,
  compilato prima dell'avvio.
* **Finestra**: `UnrealEditor.exe RefactorTactics.uproject /Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox -game -windowed -ResX=1920
  -ResY=1080 -WinX=0 -WinY=0 -RTScenario=Spec.Combat.InterruptCancelsTheShot -RTPlanningSeconds=3600 -LiveCoding=false -nosplash
  -abslog=<scratchpad della sessione> -dpcvars=t.MaxFPS=60`.
* **Banco**: lo scenario versionato
  [`Spec.Combat.InterruptCancelsTheShot`](../../../../Scenarios/Spec/Combat/InterruptCancelsTheShot.json), senza modifiche. Guarda la
  squadra 0. Al turno 1 B2 (Riktor), adiacente e quindi visto, interrompe il `Hero.Ivrin.PulseShot` di A1 (Ivrin); al turno 2 lo stesso
  tiro parte. Lo scenario ha chiuso `PASS (7/7 assertion, 2 turni)`.
* **Apparato**: catture `PrintWindow` per PID a raffica (circa una ogni 0,2 s), client 1920×1080, nessun input. Gli orari sono quelli
  del log (UTC).

## Le tavole

| Tavola | Quando | Che cosa si vede |
|---|---|---|
| [`tav_a_fumetto_barrato_e_riga.jpg`](tav_a_fumetto_barrato_e_riga.jpg) | turno 1, 19:40:41.620, dentro il playback (Risoluzione 19:40:41.058–19:40:42.059) | sopra Ivrin il fumetto «COLPO A IMPULSI» attraversato dal taglio rosso; sopra Riktor il fumetto «INTERRUPT». Nel Registro, sotto la riga «Riktor — Interrupt», la riga «Ivrin: Colpo a impulsi bloccato» e, sotto, «interrotto da Riktor» |
| [`tav_b_controllo_fumetto_normale.jpg`](tav_b_controllo_fumetto_normale.jpg) | turno 2, 19:40:43.938 | lo stesso tiro, non interrotto: il fumetto «COLPO A IMPULSI» senza taglio, e la riga «Ivrin — Colpo a impulsi» |

## Che cosa non misura

* La PIE dell'Editor, il pacchetto, 1440p e 4K.
* Le altre forme del blocco: linea di tiro (la riga sostituisce «Nessuna linea di tiro»), stordito, Prep e Dash. Le coprono i test
  headless, non lo schermo.
* L'interruttore nascosto a chi guarda: lo copre `RefactorTactics.Playback.HiddenInterrupterIsNotNamed`, non lo schermo.
* Il giudizio sulla resa del taglio: spessore, colore e inclinazione sono una prima stesura dell'assistente (capsula rossa di 6 px con
  bordo scuro di 1,5 px, inclinata di 12°, larga il nome più il 12%). ➕ L'autore l'ha giudicata il 2026-11-17 sulla tavola A, alla
  domanda «il taglio ti va bene così (spessore, rosso, inclinazione)?»: «si va bene».
