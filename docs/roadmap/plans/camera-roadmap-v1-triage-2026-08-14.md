# Camera Roadmap v1.0 — triage del consolidamento

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-14
> **Cosa è**: il referto di consumo di
> [`../../archive/src/RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md`](../../archive/src/RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md)
> — cosa di quell'handoff è entrato nel repository, cosa era già in vigore, cosa era falso e perché.
> **Cosa non è**: una roadmap. Gli owner restano [`../feature-registry.yaml`](../feature-registry.yaml),
> [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) e
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md).

## 1. Il verdetto in una riga

L'handoff descrive una camera che il repository **non ha ancora**, e mentre la descrive sbaglia lo stato di
quella che **ha già**: il difetto più grosso che ha fatto emergere non è nella sua roadmap ma nel Feature
Registry, che dichiara `automation: todo` e `tests: []` su una feature coperta da **quattro** test verdi.

## 2. Il conto

| Oggetto dell'handoff | Quanti | Entrati ora | Perché |
|---|--:|--:|---|
| Issue candidate | 51 | 3 | 48 descrivono release da v0.2 in poi: diventano scope di epic già esistenti, non issue aperte oggi |
| Scenari proposti | 34 | 0 nuovi ID | `scenario-map.md` non ha **nessuna** voce camera: il gap è reale, ma va aperto con la convenzione del repository, non con questi nomi |
| Decisioni da registrare | 9 | 2 | 4 sono già in vigore, 2 sono premesse false, 1 è tuning aperto |
| Feature ID citati | 13 | — | **6 non esistono** (§3) |
| Sezioni | 18 | — | 1257 righe |

## 3. `WRONG` — le premesse false, misurate

### 3.1 Sei Feature ID su tredici non esistono

L'handoff §1 chiede di auditare tredici `feature_id`. Cercati in
[`../feature-registry.yaml`](../feature-registry.yaml) — 110 feature, `grep` sul campo `feature_id`:

| Citato | Esiste | Nel repository |
|---|:--:|---|
| `RT-FEAT-UI-TACTICAL-CAMERA` | ✅ | `IMPLEMENTING`, v0.1, E11, P2 |
| `RT-FEAT-UI-PLANNING` | ✅ | `RELEASE_READY` |
| `RT-FEAT-UI-CERTAINTY` | ✅ | `IMPLEMENTING` |
| `RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE` | ✅ | `TESTABLE` |
| `RT-FEAT-PERCEPTION-MEMORY` | ✅ | `TESTABLE` |
| `RT-FEAT-PERCEPTION-NOISE` | ✅ | `SPECIFIED` |
| `RT-FEAT-UI-ACTION-GHOSTS` | ✅ | `SPECIFIED` — l'handoff lo chiama `RT-FEAT-UI-ACTION-GHOSTS` **e** `RT-FEAT-UI-AOE-GHOST` |
| `RT-FEAT-UI-CELL-SELECTION` | ❌ | il concetto è `RT-FEAT-UI-POINTER-INTERACTION` |
| `RT-FEAT-UI-LAYER-FILTER` | ❌ | nessuna feature layer nel registry |
| `RT-FEAT-UI-AOE-GHOST` | ❌ | vedi `ACTION-GHOSTS` |
| `RT-FEAT-UI-FAST-DECISION` | ❌ | il dominio è E14, non una feature UI |
| `RT-FEAT-UI-ACCESSIBILITY` | ❌ | nessuna feature accessibility |
| `RT-FEAT-PERCEPTION-SOUND-OVERLAY` | ❌ | il concetto è `RT-FEAT-PERCEPTION-NOISE` |

⚠️ Sono **nomi plausibili di feature che non ci sono**, e due di essi (`CELL-SELECTION`, `SOUND-OVERLAY`)
hanno un omonimo semantico con un altro nome. Prenderli per buoni avrebbe prodotto sei feature nuove
duplicate di cose esistenti — cioè esattamente la *Feature ID explosion* che l'handoff §17 vieta.

### 3.2 «`IMPLEMENTED`» non è uno stato di questo repository

L'handoff §1 riporta `RT-FEAT-UI-TACTICAL-CAMERA` come **`IMPLEMENTED`**. Gli stati ammessi sono elencati
nell'intestazione del registry: `IDEA · DESIGNED · SPECIFIED · IMPLEMENTING · TESTABLE · INTEGRATED ·
RELEASE_READY · DONE · DEFERRED · BLOCKED`. Lo stato reale è **`IMPLEMENTING`**, e la differenza non è
lessicale: `IMPLEMENTING` è derivato dai gate, e i gate dicono `automation: todo`.

### 3.3 Lo snap canonico: entrambe le opzioni sono già state scartate

L'handoff §8 lascia aperto come tuning *«canonical snap = 60° vs 90°»*. Il codice ha già deciso, e ha
scritto perché — [`../../../Source/RefactorTactics/Camera/RTCameraPawn.h`](../../../Source/RefactorTactics/Camera/RTCameraPawn.h):

> `YawStep = 45.f` — «45 è deliberatamente **non** un divisore di 60: sarebbe la scelta ovvia su una
> griglia esagonale, ma agganciare la vista agli assi della griglia rende impossibile guardare *fra* due
> file di celle, che è esattamente ciò che serve quando un cilindro copre quello che c'è dietro.»

60° è un divisore di 60 e 90° è un multiplo di 45 che riallinea agli assi ogni due passi: la motivazione
scritta **esclude** la prima e non sostiene la seconda. Una scelta fra due opzioni entrambe già respinte non
è una decisione aperta — è una domanda con una premessa falsa, e va chiusa dicendo cosa c'è.

### 3.4 Il problema SpringArm è già risolto

L'handoff §3.8 chiede di *«evitare comportamento standard SpringArm che accorcia continuamente la camera»*.
`RTCameraPawn.cpp` imposta `SpringArm->bDoCollisionTest = false` nel costruttore: il comportamento che la
sezione teme non può verificarsi. Resta vero il resto della sezione (safety collision assente), ma la
premessa — «la camera oggi popping per collisione» — è falsa.

## 4. `CURRENT` — quattro decisioni già in vigore

| § handoff | Cosa propone | Dove è già scritto |
|---|---|---|
| 3.13 | «nessuna minimappa tradizionale obbligatoria per v0.1» | [`../../technical/progettazione-hud.md`](../../technical/progettazione-hud.md) §30, con quattro motivi fra cui «overview separata più leggibile» |
| 3.1 | «pan relativo alla camera» | `AddPlanarMovement` lo implementa, con la motivazione in commento |
| 3.9 | «focus conserva zoom» | `FocusOn` «mantenendo la quota e lo zoom correnti» |
| 3.1 | «camera presentation-only, nessuna authority» | `FrameOwnTeam`: «Sola presentazione: legge lo stato, non lo cambia» |

⚠️ §3.13 chiede di *«risolvere il conflitto minimap/Strategic View»*. **Non c'è nessun conflitto**: il
documento owner nega già la minimap permanente, e per gli stessi motivi. Ciò che manca non è una decisione,
è il legame fra quella riga e l'overview — che oggi non ha né feature né issue.

## 5. Il difetto vero: il registry sottodichiara la camera

[`../../../Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`](../../../Source/RefactorTactics/Tests/RTCameraPawnTests.cpp)
contiene **quattro** automation test:

```text
RefactorTactics.Camera.YawTurnsInStepsAndStaysNormalized
RefactorTactics.Camera.RotatingDoesNotResetZoom
RefactorTactics.Camera.PanIsRelativeToTheView
RefactorTactics.Camera.RecenterAlsoResetsRotation
```

Il registry dichiara per la stessa feature `automation: todo`, `tests: []`, `scenarios: []`, e in `notes`:
«**nessun test automatico** la copre». È **falso dal 2026-08-08**, la data di `last_verified`.

> **Perché nessun gate se ne accorge.** `validate` verifica che i pattern dichiarati in `tests:` matchino
> test reali — cioè il verso *dichiarato → esistente*. Il verso opposto, *esistente → dichiarato*, non è
> controllato da nulla: una lista vuota è sempre coerente. È lo stesso difetto del gate che confronta un
> generato con sé stesso, applicato al registry.

Questa correzione è l'unica parte dell'handoff che chiude un difetto **misurato** invece di aprire lavoro
futuro, ed è entrata per prima.

## 6. `PROPOSED` — cosa entra ora, e perché proprio questo

### 6.1 Il registry dice la verità sulla camera

`RT-FEAT-UI-TACTICAL-CAMERA` passa `automation: todo → partial` con i quattro test dichiarati. **Non**
`done`: i quattro coprono yaw, zoom e pan relativo, non coprono focus, bounds né `FrameOwnTeam`.

### 6.2 Due decisioni, non nove

Delle nove decisioni §13, entrano le due che il repository non ha e che non dipendono da codice futuro:

- **lo yaw canonico è a 45°, e non si aggancia agli assi della griglia** — promuove a decisione la
  motivazione che oggi vive in un commento C++, dove nessun documento la trova;
- **la camera è presentation-only** — nessuna authority su gameplay, LOS, targeting o visibilità.

Le altre sette descrivono sistemi che non esistono (Strategic View, Camera Director, ActiveLayer,
cross-layer targeting): sono **premesse di design**, e una decisione presa prima del suo primo consumatore
è un dato che nessuna partita produce.

### 6.3 Tre issue, non cinquantuno

Aperte solo per i gap misurati sul codice v0.1, dentro E11 ([#25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25)):

| Issue | Gap | Misura |
|---|---|---|
| [#863](https://github.com/DegrassiAaron/refactor-tactics-main/issues/863) | yaw continuo, pitch a runtime | `AddYaw` avanza solo di `YawStep`, quindi otto orientamenti; `CameraPitch` è `EditAnywhere` e nessun input la tocca |
| [#864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/864) | zoom al cursore, soft bounds | `AddZoom` varia la sola arm length; `AddPlanarMovement` non ha limiti |
| [#865](https://github.com/DegrassiAaron/refactor-tactics-main/issues/865) | `FocusOn` e `FrameOwnTeam` senza test | è la ragione per cui `automation` è `partial` e non `done` |

⚠️ **La camera non aveva una issue propria.** Il registry dichiarava `issues: [77]`, ma il DoD di CP 11.1
riguarda l'HUD e non contiene nulla di camera: era il legame nominale che il registry stesso descrive nel
commento sui `pie_refs` — più feature che dichiarano la stessa issue. `77` resta perché
`checkpoints: ["11.1"]` lo implica, e accanto ora ci sono le tre che parlano di camera.

## 7. `PROPOSED` differito — cosa diventa scope di epic

Le 48 issue candidate restanti non si aprono oggi: descrivono release da v0.2 a v1.0 le cui epic
**esistono già** (E23, E27, E40, E42, E44, E45 verificate su GitHub). Aprirle ora produrrebbe 48 issue
ferme su dipendenze non scritte — e il loro contenuto è già nell'handoff archiviato, che resta consultabile.

## 8. Cosa questo triage NON decide

- **Le verifiche PIE della camera.** [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md)
  è nel `writable` della track `content_editor` (#451) in [`../parallel-batch.yaml`](../parallel-batch.yaml):
  non è mio, e una voce ⏳ scritta qui sarebbe una modifica al file di un'altra sessione.
- **Gli scenari camera.** Zero voci oggi in [`../../technical/scenario-map.md`](../../technical/scenario-map.md);
  i 34 nomi proposti non seguono la convenzione del corpus. Il gap è registrato, il vocabolario no.
- **Le soglie Strategic e le sensibilità di default.** Restano tuning senza consumatore.
- **La Wiki.** `RT-FEAT-UI-TACTICAL-CAMERA` dichiara già `wiki:come-si-gioca` fra i propri `wiki_refs` —
  una delle sette feature che citano quella pagina. Le pagine vivono in un **clone separato** (`D-076`) che
  non è in questo worktree: `docs/wiki/` qui contiene manifest e infografiche, non il testo. Quindi il
  legame esiste già e **se quella pagina copra la camera non è verificabile da qui**: dirlo in un verso o
  nell'altro sarebbe una deduzione dal nome del file.
