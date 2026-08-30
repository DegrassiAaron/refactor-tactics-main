# Tactical Camera — referto del consolidamento (2026-08-30)

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-30
> **Cosa è**: il referto di consumo del prompt di consolidamento *«Tactical Camera, Epic e GitHub Issues»* —
> cosa è entrato, cosa esisteva già, cosa era prescritto e non poteva entrare così com'era.
> **Cosa non è**: una specifica. Gli owner sono
> [`../../technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md),
> [`../../technical/systems/spec-domini-spaziali-mappa.md`](../../technical/systems/spec-domini-spaziali-mappa.md)
> e il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> Il referto del consolidamento **precedente** resta
> [`camera-roadmap-v1-triage-2026-08-14.md`](camera-roadmap-v1-triage-2026-08-14.md), oggi storia
> (`D-254`).

## 1. Il verdetto in una riga

Il prompt descrive una camera in gran parte **già costruita** — pan relativo, zoom ancorato al cursore,
yaw continuo, pitch a runtime, orbita, focus che conserva lo zoom, soft bounds — e il valore del
consolidamento non sta nelle sue prescrizioni: sta nell'aver reso evidente che quel lavoro **non aveva una
sede documentale**, e che due delle prescrizioni non erano prescrizioni ma domande.

## 2. Il conto

| Oggetto | Quanti | Esito |
|---|--:|---|
| Requisiti del prompt (§1–§13) | 13 aree | 12 → issue, 1 assorbita (input dentro le altre) |
| Issue camera **esistenti** su GitHub | 6 | tutte **chiuse** — nessuna riaperta, nessuna duplicata |
| Issue camera **aperte** prima di oggi | **0** | — |
| Epic camera esistenti | **0** | creata **E49** ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)) |
| Issue create | 12 | [#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770)–[#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781) |
| Decisioni nuove | 5 | `D-250`–`D-254` |
| Domande aperte registrate | 2 | `CAM-A`, `CAM-B` |
| Spec create | 2 | camera · domini spaziali |
| Prescrizioni **rifiutate** come tali | 2 | vedi §5 |

## 3. `CURRENT` — cosa il prompt chiedeva e il codice aveva già

Misurato su `origin/main` il 2026-08-30, non dedotto:

| § prompt | Chiede | Dove è già |
|---|---|---|
| §3 | zoom continuo con rotella | `AddZoom` · `ZoomTowards` ancorato al cursore |
| §3 | `Q`/`E` snap rotazione | `AddYaw`, `YawStep = 45°` (`D-142`) |
| §3 | `F` focus selezione | `FocusOn` + `FocusCameraOnUnit` |
| §3 | `Home` reset orientamento | `RecenterView` |
| §3 | `LMB` select, `RMB` cancel | `SelectAction` · `UndoAction` |
| §2 | yaw, pitch, zoom come stato | `CameraYaw` · `CameraPitch` · `TargetArmLength` |
| §5 | **zoom continuo, non tre modalità** | è già così: nessuna modalità esiste |
| §8 | focus conserva zoom | `Camera.FocusMovesToTargetAndKeepsZoom` |
| §12 | non usare la collisione SpringArm | `bDoCollisionTest = false` dal costruttore |
| — | input rimappabile ed Enhanced Input | `BuildInputMappings`, **interamente C++** |

⚠️ **Il §12 chiedeva di *«non usare la collisione classica della SpringArm … se provoca continui cambi di
zoom»*.** È la stessa premessa che il triage del 2026-08-14 aveva già misurato falsa: la collisione è
spenta dal costruttore, e il comportamento temuto non può verificarsi. Due kit consecutivi hanno posto la
stessa domanda; ora la risposta è una decisione (`D-253`) invece che una riga di codice da ritrovare.

## 4. Il difetto vero: nessuna sede

`RT-FEAT-UI-TACTICAL-CAMERA` non esiste più — il Feature Registry è uscito con `D-181` — e nessun
documento aveva preso il suo posto. La camera viveva in:

- **codice** (`RTCameraPawn.h`, che documenta bene ma solo ciò che c'è);
- **due decisioni** (`D-142`, `D-143`);
- **cinque voci PIE** verdi;
- **un referto di consumo** del 2026-08-14, che per costruzione descrive il passato di una decisione.

Chi voleva sapere *cosa deve fare la camera* non aveva dove guardare. `D-254` chiude il buco con due
owner, separati per consumatore: la spec camera la leggono programmatori, quella dei domini spaziali chi
costruisce livelli.

> 🔴 **Il triage era invecchiato in un modo ingannevole, e va detto.** Dichiara **quattro** automation test
> camera — oggi sono **19** — e indica come owner `docs/roadmap/feature-registry.yaml`, rimosso da
> `26f6955a`. Entrambe le affermazioni erano vere quando furono scritte, ed è esattamente la ragione per
> cui un referto non può fare da specifica: registra un istante, e nessun gate lo rimisura.

## 5. `REJECTED AS PRESCRIPTION` — due cose che non potevano entrare così

Il prompt le presentava come requisiti. Applicarle sarebbe stato introdurle di nascosto.

### 5.1 `CAM-A` — il picking layer-aware **tocca il gameplay**

§7 prescrive `World Hit → X/Y → ActiveLayer → FRTCellId`. Il codice fa l'opposto, e lo dichiara:
`RTPlayerController.cpp:502` — *«Il layer viene dalla QUOTA del punto colpito»*.

Cambiarlo cambia **quale cella un click seleziona**, quindi quale cella si pianifica. Non è presentazione.
Richiede inoltre un `ActiveLayer` di gioco che **non esiste** (quello che c'è è authoring, guidato
dall'editor mode) e la matrice del puntatore appartiene a
[#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705).

→ registrata come domanda in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), lavorata da
[#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776), con commento di legame su #705.

### 5.2 `CAM-B` — il rebinding `MMB` **invalida una verifica passata**

§3 prescrive `MMB drag → Pan` e `Alt+LMB drag → Orbit`. Oggi `MMB` **è** il modificatore d'orbita, e
`PIE-CAM-ORBIT` è verde dal 2026-08-16 descrivendolo tasto per tasto — quella stessa sessione ha chiuso
una decisione aperta (`bInvertOrbitPitch`, provato con le mani).

Non è un'aggiunta: è un cambio con un costo. → domanda in `OPEN_DECISIONS.md`, blocca
[#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771).

## 6. `PROPOSED` — le cinque decisioni

| ID | Cosa fissa | Perché ora |
|---|---|---|
| `D-250` | i quattro domini spaziali della mappa | il vocabolario non esisteva: **zero** occorrenze in `Source/` |
| `D-251` | limiti del pivot **viewport-aware** | il clamp fisso risponde a *«non perdere la mappa»*, non a *«non mostrare il vuoto»* |
| `D-252` | Strategic View come conseguenza dello zoom, con isteresi | fissa la forma **prima** che qualcuno implementi tre modalità perché sono più facili da nominare |
| `D-253` | occlusione in presentazione; SpringArm solo safety | promuove a vincolo una riga di costruttore che due kit hanno già frainteso |
| `D-254` | la camera prende un owner documentale | il difetto misurato al §4 |

## 7. Cosa questo consolidamento NON fa

- **Non tocca `test-manuali-pie.md`.** Il file ha modifiche non committate di un'altra sessione
  (rimisurazione delle voci del 2026-08-30, sessione MCP): due sessioni sullo stesso file sono un
  conflitto garantito, e la regola di `D-222` vale anche per i documenti. Le voci `PIE-CAM-*` che vanno
  riscritte sono nella DoD delle issue che le invalidano, non qui.
- **Non apre scenari camera.** Zero oggi, e i nomi non si inventano: la convenzione di ScenarioId manca, e
  scriverla è parte di [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780).
- **Non ripara i riferimenti al Feature Registry** sparsi in altri documenti — `spec-pointer-interaction.md`
  ne cita uno. È un debito reale e **non è di questo consolidamento**: toccarlo qui allargherebbe il diff
  su file di altri owner.
- **Non modifica `.uasset` né `.umap`.** `L_CameraFeatureLab` è lavoro Editor, e resta una verifica manuale
  finché non eseguita.
- **Non riapre** #863 #864 #865 #873 #874 #887.

## 8. Ciò che resta davvero da decidere

Solo due cose, ed entrambe hanno un innesco scritto: `CAM-A` (picking layer-aware, insieme a #705) e
`CAM-B` (rebinding `MMB`). Tutto il resto è lavoro, non decisione.
