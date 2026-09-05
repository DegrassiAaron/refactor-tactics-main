# RT3 — handoff di esempio

Esempio compilato del formato definito in [`RT3_CONTRACT.md`](RT3_CONTRACT.md) §9.

> ⚠️ **Wave fittizia.** Feature, SHA, path e numeri di questo documento sono inventati a scopo di esempio. Non sono una misura, non citarli come evidenza e non riusarli come valori reali.

L'esempio mostra deliberatamente un `FAIL`, un `BLOCKED` e un `USER_REQUIRED`, perché è nei casi non verdi che il formato serve.

---

## 1. Handoff EDITOR

```text
=== RT3 HANDOFF ===

FROM:          EDITOR
TO:            VALIDATION
FEATURE:       Preview del cono Overwatch in planning
WAVE_ID:       overwatch-cone-preview/1

BRANCH:        feat/2401-overwatch-cone-preview
PARENT_BRANCH: feat/2367-spinta-silenzia-overwatch
BASE_SHA:      a1b2c3d4e5f60718293a4b5c6d7e8f9012345678
PRODUCED_SHA:  b2c3d4e5f60718293a4b5c6d7e8f90123456789a

WRITE_SET:
  Content/RT/UI/WBP_PlanningOverlay.uasset
  Content/RT/Data/DA_OverwatchProfile_Default.uasset
  Content/RT/Maps/L_DevSandbox.umap

BINARY_ASSETS:
  Content/RT/UI/WBP_PlanningOverlay.uasset
  Content/RT/Data/DA_OverwatchProfile_Default.uasset
  Content/RT/Maps/L_DevSandbox.umap

## MATRICE

BLUEPRINT        PASS
  EVIDENCE_REF:  log: Saved/Logs/RT-Editor-2026-09-05.log#L4471
                 (compile WBP_PlanningOverlay: 0 error, 0 warning)

DATA             PASS
  EVIDENCE_REF:  asset: Content/RT/Data/DA_OverwatchProfile_Default.uasset@b2c3d4e
                 (riletto dopo reopen: ConeHalfAngleDeg 45.0, RangeCells 4)

DATA VALIDATORS  PASS
  EVIDENCE_REF:  suite: rtsuite -Filter RTDataValidation
                 -> exit 0, found 12, performed 12, passed 12, failed 0

MAP              PASS
  EVIDENCE_REF:  log: Saved/Logs/RT-MapCheck-2026-09-05.log#L88
                 (L_DevSandbox: 0 error, 0 warning; nessun singleton duplicato)

PLANNING         PASS
  EVIDENCE_REF:  turnlog: waves/overwatch-cone-preview/evidence/turnlog-seed-4711.json

TARGETING        PASS
  EVIDENCE_REF:  turnlog: waves/overwatch-cone-preview/evidence/turnlog-seed-4711.json

LOS/COVER        FAIL
  EVIDENCE_REF:  turnlog: waves/overwatch-cone-preview/evidence/turnlog-seed-4711.json
                 (cella 12,7,L1 dietro cover alto: preview la include,
                  il TurnLog la esclude da EligibleTargets)

UI/HUD           PASS
  EVIDENCE_REF:  shot: waves/overwatch-cone-preview/evidence/hud-planning-confirmed.png

PRIVACY          OBSERVED
  EVIDENCE_REF:  shot: waves/overwatch-cone-preview/evidence/client2-planning.png
                 (il cono avversario non compare nella UI del client 2;
                  tetto OBSERVED per contratto §7 — la prova richiede canary)

DETERMINISM      BLOCKED
  REASON:        PIE eseguito con SEED_SOURCE generated (seed 4711 osservato,
                 non dichiarato prima dell'esecuzione)
  UNBLOCK:       rieseguire lo scenario con seed fisso dichiarato

PERFORMANCE      OBSERVED
  EVIDENCE_REF:  log: Saved/Logs/RT-Editor-2026-09-05.log#L5210
                 (nessun rebuild UI per frame osservato in planning; misura a VALIDATION)

ERRORS           PASS
  EVIDENCE_REF:  log: Saved/Logs/RT-Editor-2026-09-05.log
                 (0 ensure, 0 check, 0 crash nella sessione)

SAVE/RELOAD      PASS
  EVIDENCE_REF:  asset: Content/RT/UI/WBP_PlanningOverlay.uasset@b2c3d4e
                 (Save -> Stop PIE -> Close Editor -> reopen: valori persistiti)

MOVEMENT         N/A
  REASON:        fuori write-set

DAMAGE           N/A
  REASON:        fuori write-set

PACKAGED         N/A
  REASON:        tetto di ruolo — contratto §7

## FINDINGS

FINDING_ID:   overwatch-cone-preview/1-F1
SEVERITY:     P1
EVIDENCE_REF: turnlog: waves/overwatch-cone-preview/evidence/turnlog-seed-4711.json
ROOT_CAUSE:   la preview interroga il raggio, non il predicato LOS del simulatore
OWNER:        DEV-LEAD
REQUIRED_FIX: la preview consuma lo stesso predicato LOS del resolver
REGRESSION:   test su cella occlusa da cover alto, preview == EligibleTargets
ATTEMPT:      1

## EVIDENCE

log:     Saved/Logs/RT-Editor-2026-09-05.log
log:     Saved/Logs/RT-MapCheck-2026-09-05.log
suite:   rtsuite -Filter RTDataValidation -> exit 0, 12/12
turnlog: waves/overwatch-cone-preview/evidence/turnlog-seed-4711.json
shot:    waves/overwatch-cone-preview/evidence/hud-planning-confirmed.png
shot:    waves/overwatch-cone-preview/evidence/client2-planning.png

## USER_REQUIRED

=== USER EDITOR CHECK ===

ID:            overwatch-cone-preview/1-U1
Mappa:         L_DevSandbox
Modo PIE:      2 client, listen server
Player:        2
Precondizioni: Wraith selezionato, Overwatch armato, cover alto in 12,7,L1

Passi:
1. arma Overwatch e osserva il cono in planning
2. ruota la camera di 180 gradi
3. cambia layer da L1 a L2

Atteso:        il cono resta leggibile e distinguibile dal range di movimento
Segnali di fallimento: cono confuso col range; bordo illeggibile su L2;
               affollamento con gli intenti alleati
Evidenza richiesta: screenshot per ciascuno dei 3 passi

Result: NOT RUN

STATUS: PARTIAL
```

---

## 2. Handoff VALIDATION

Segue lo stesso `WAVE_ID`. Nota il trattamento dell'ereditarietà: i sistemi lasciati `OBSERVED` o `BLOCKED` da EDITOR **non** diventano `PASS` per passaggio — contratto §11.

```text
=== RT3 HANDOFF ===

FROM:          VALIDATION
TO:            DEV-LEAD
FEATURE:       Preview del cono Overwatch in planning
WAVE_ID:       overwatch-cone-preview/1

BRANCH:        feat/2401-overwatch-cone-preview
PARENT_BRANCH: feat/2367-spinta-silenzia-overwatch
BASE_SHA:      b2c3d4e5f60718293a4b5c6d7e8f90123456789a
PRODUCED_SHA:  b2c3d4e5f60718293a4b5c6d7e8f90123456789a

WRITE_SET:     nessuno — VALIDATION è read-mostly
BINARY_ASSETS: nessuno

## MATRICE

BUILD            PASS
  EVIDENCE_REF:  suite: Build.bat RefactorTacticsEditor Win64 Development
                 -> exit 0

ARCHITECTURE     FAIL
  EVIDENCE_REF:  Source/RefactorTactics/UI/RTPlanningPreview.cpp:212
                 (la preview reimplementa il test di visibilità invece di
                  consumare il predicato del resolver — duplicazione di autorità)

AUTOMATION/SCENARIO  FAIL
  EVIDENCE_REF:  suite: rtsuite -Filter RTOverwatch
                 -> exit 1, found 18, performed 18, passed 17, failed 1
                 (RTOverwatchPreviewMatchesEligibility)

LOS/COVER        FAIL
  EVIDENCE_REF:  suite: rtsuite -Filter RTOverwatch
                 -> exit 1, failed 1 — conferma indipendente di F1

DETERMINISM      PASS
  EVIDENCE_REF:  suite: rtsuite -Filter RTOverwatch -Seed 1000 (x3)
                 -> exit 1, hash identico sulle 3 esecuzioni
                 (rieseguito con seed fisso: il BLOCKED di EDITOR è risolto qui)

PRIVACY          PASS
  EVIDENCE_REF:  suite: rtsuite -Filter RTPrivacyCanary
                 -> exit 0, found 6, performed 6, passed 6, failed 0
                 (canary sul cono avversario: assente sulla connessione client 2)

TURNLOG/REPLAY   PASS
  EVIDENCE_REF:  turnlog: waves/overwatch-cone-preview/evidence/replay-hash.txt

PERFORMANCE      PASS
  EVIDENCE_REF:  log: Saved/Profiling/planning-overlay-2026-09-05.csv
                 (nessuna allocazione calda nel path di preview)

UI/HUD           OBSERVED
  EVIDENCE_REF:  handoff EDITOR, shot hud-planning-confirmed.png
                 (tetto di ruolo — contratto §7)

PACKAGED         N/A
  REASON:        la Definition of Done viva non richiede packaged per questa wave

## FINDINGS

FINDING_ID:   overwatch-cone-preview/1-F1
SEVERITY:     P1
EVIDENCE_REF: suite: rtsuite -Filter RTOverwatch -> failed 1
ROOT_CAUSE:   RTPlanningPreview.cpp:212 duplica il test di visibilità
              invece di consumare il predicato del resolver
OWNER:        DEV-LEAD
REQUIRED_FIX: la preview consuma il predicato LOS del resolver
REGRESSION:   RTOverwatchPreviewMatchesEligibility deve passare
ATTEMPT:      1

## P0:
(nessuno)

## P1:
overwatch-cone-preview/1-F1

## P2:
(nessuno)

## P3:
(nessuno)

## USER_REQUIRED:
overwatch-cone-preview/1-U1   Result: NOT RUN

## EVIDENCE

suite:   Build.bat RefactorTacticsEditor Win64 Development -> exit 0
suite:   rtsuite -Filter RTOverwatch -> exit 1, 17/18
suite:   rtsuite -Filter RTPrivacyCanary -> exit 0, 6/6
suite:   rtsuite -Filter RTOverwatch -Seed 1000 (x3) -> hash identico
turnlog: waves/overwatch-cone-preview/evidence/replay-hash.txt
log:     Saved/Profiling/planning-overlay-2026-09-05.csv

RISULTATO: FAILED
```

---

## 3. Cosa mostra l'esempio

| Punto | Dove |
|---|---|
| `PASS` sempre con `EVIDENCE_REF` ad artefatto, mai prosa | tutte le voci verdi |
| `N/A` sempre con `REASON`, giustificato dal write-set | `MOVEMENT`, `DAMAGE` |
| `OBSERVED` non promosso a `PASS` dal ruolo successivo | `PRIVACY` in EDITOR → `PASS` solo con canary in VALIDATION |
| `BLOCKED` risolto misurando, non ereditando | `DETERMINISM` |
| `SEED_SOURCE: generated` che blocca il determinismo | matrice EDITOR |
| Tetto di ruolo rispettato in entrambe le direzioni | `PACKAGED` in EDITOR, `UI/HUD` in VALIDATION |
| `FINDING_ID` stabile fra i due handoff, con `ATTEMPT` | `overwatch-cone-preview/1-F1` |
| `USER_REQUIRED` che resta `NOT RUN` e non viene convertito | `overwatch-cone-preview/1-U1` |
| `PARENT_BRANCH` diverso da `main` | entrambi gli handoff |
