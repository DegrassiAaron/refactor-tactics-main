# Camera Roadmap v1.0 — triage del consolidamento

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-14
> **Cosa è**: il referto di consumo di
> [`../../archive/src/RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md`](../../archive/src/RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md)
> — cosa di quell'handoff è entrato nel repository, cosa era già in vigore, cosa era falso e perché.
> **Cosa non è**: una roadmap. Gli owner restano `../feature-registry.yaml`,
> [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) e
> [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md).

## 1. Il verdetto in una riga

L'handoff descrive una camera che il repository **non ha ancora**, e mentre la descrive sbaglia lo stato di
quella che **ha già**: il difetto più grosso che ha fatto emergere non è nella sua roadmap ma nel Feature
Registry, che dichiara `automation: todo` e `tests: []` su una feature coperta da **quattro** test verdi.

## 2. Il conto

| Oggetto dell'handoff | Quanti | Entrati ora | Perché |
|---|--:|--:|---|
| Issue candidate | 51 | 3 | delle 5 candidate **v0.1**, tre sono aperte (§6.3) e due hanno già un owner (§7); le altre **46** descrivono release da v0.2 in poi |
| Scenari proposti | 34 | 0 nuovi ID | `scenario-map.md` non ha **nessuna** voce camera: il gap è reale, ma va aperto con la convenzione del repository, non con questi nomi |
| Decisioni del §13 | 9 | **0** | nessuna delle nove entra come tale (§6.2) |
| Decisioni nuove registrate | — | 2 | `D-142` e `D-143`, che **non** vengono dal §13 ma dal §8 e dal §3.1 |
| Feature ID citati | 13 | — | **6 non esistono** (§3) |
| Sezioni | 19 | — | 1257 righe |

> ⚠️ **Le due righe sulle decisioni erano una sola, e il conto non tornava.** Diceva «9 candidate → 2
> entrate, 4 già in vigore, 2 premesse false, 1 tuning aperto»: un `4+2+1+2 = 9` che sommava voci prese da
> **tre sezioni diverse** dell'handoff. Le due decisioni effettivamente registrate non sono nell'elenco del
> §13 — nascono dal §8 (lo snap) e dal §3.1 (presentation-only). Trovato in code review.

## 3. `WRONG` — le premesse false, misurate

### 3.1 Sei Feature ID su tredici non esistono

L'handoff §1 chiede di auditare tredici `feature_id`. Cercati in
`../feature-registry.yaml` — 110 feature, `grep` sul campo `feature_id`:

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

## 4. `CURRENT` — tre proposte già in vigore

| § handoff | Cosa propone | Dove è già scritto |
|---|---|---|
| 3.13 | «nessuna minimappa tradizionale obbligatoria per v0.1» | [`../../technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §30, con quattro motivi fra cui «overview separata più leggibile» |
| 3.1 | «pan relativo alla camera» | `AddPlanarMovement` lo implementa, con la motivazione in commento |
| 3.9 | «focus conserva zoom» | `FocusOn` «mantenendo la quota e lo zoom correnti» |
⚠️ §3.13 chiede di *«risolvere il conflitto minimap/Strategic View»*. **Non c'è nessun conflitto**: il
documento owner nega già la minimap permanente, e per gli stessi motivi. Ciò che manca non è una decisione,
è il legame fra quella riga e l'overview — che oggi non ha né feature né issue.

> 🔵 **Una quarta riga è stata tolta da questa tabella: «camera presentation-only».** Ci stava per una
> ragione debole — il commento di `FrameOwnTeam` dice «Sola presentazione: legge lo stato, non lo cambia» —
> e la contava fra le decisioni *già in vigore* mentre §6.2 la registrava come `D-143`, cioè fra quelle che
> il repository **non ha**. Lo stesso item in due bucket opposti: il conto del §2 lo contava due volte.
> La distinzione che risolve: *vero nel codice* non è *dichiarato come vincolo*. Un commento in una
> funzione non è un invariante che qualcuno debba rispettare, ed è esattamente perché non lo era che
> `D-143` ha ragione di esistere. Trovato in code review.

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
«**nessun test automatico** la copre».

> 🔵 **Quella nota era vera quando è stata scritta, e la prima stesura di questo triage lo ha sbagliato.**
> Diceva «è falso dal 2026-08-08, la data di `last_verified`» — ma i due eventi distano **58 minuti**, non
> zero: `last_verified.commit` è `2094b867`, del 2026-08-08 alle `17:57:05`, e `RTCameraPawnTests.cpp` è
> nato con `07620c42` alle `18:55:16`. La verifica era corretta al momento in cui è stata fatta.
> **La causa non è una misura sbagliata, è un `last_verified` mai avanzato**: la data serve esattamente a
> dire «questo era vero *allora*», e chi legge il registry non ha modo di sapere che il mondo è cambiato
> un'ora dopo. Trovato in code review confrontando i *timestamp* invece delle date.

**Il gate gap resta, ed è una constatazione separata.** Una volta invecchiata, quella nota non aveva nessuna
rete: `validate` verifica che i pattern dichiarati in `tests:` matchino test reali — il verso *dichiarato →
esistente*. Il verso opposto, *esistente → dichiarato*, non è controllato da nulla, e una lista vuota è
sempre coerente. È lo stesso difetto del gate che confronta un generato con sé stesso, applicato al
registry, ed è la ragione per cui l'errore è sopravvissuto sei giorni invece di un'ora.

Questa correzione è l'unica parte dell'handoff che chiude un difetto **misurato** invece di aprire lavoro
futuro, ed è entrata per prima.

## 6. `PROPOSED` — cosa entra ora, e perché proprio questo

### 6.1 Il registry dice la verità sulla camera

`RT-FEAT-UI-TACTICAL-CAMERA` passa `automation: todo → partial` con i quattro test dichiarati. **Non**
`done`: i quattro coprono yaw, zoom e pan relativo, non coprono focus, bounds né `FrameOwnTeam`.

### 6.2 Due decisioni — e nessuna delle nove

Il §13 elenca nove decisioni «da verificare/registrare». **Nessuna entra come tale**: #8 (niente minimap) è
già in vigore (§4); #1 (yaw libero, pitch vincolato) e #2 (RMB contestuale) non sono decisioni ma
comportamenti da scrivere, e hanno un owner — [#863](https://github.com/DegrassiAaron/refactor-tactics-main/issues/863)
e [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705); le restanti sei (#3 Strategic
View, #4 targeting cross-layer, #5 `ActiveLayer`, #6 cutaway, #7 Camera Director, #9 replay/spectator)
descrivono sistemi che non esistono, e una decisione presa prima del suo primo consumatore è un dato che
nessuna partita produce.

Entrano invece **due decisioni che il §13 non contiene**, nate da altre due sezioni, perché riguardano
codice che esiste già oggi:

- **lo yaw canonico è a 45°, e non si aggancia agli assi della griglia** — promuove a decisione la
  motivazione che oggi vive in un commento C++, dove nessun documento la trova;
- **la camera è presentation-only** — nessuna authority su gameplay, LOS, targeting o visibilità.
  ⚠️ Con un limite che `D-143` dichiara da sé: `FrameOwnTeam` itera **tutte** le unità del mondo e legge
  `TeamId`/`IsAlive()` anche dei nemici. La proprietà «non modifica lo stato» è vera; «legge una vista già
  sanificata» **non lo è ancora**, ed è il primo debito che quella decisione crea invece di nascondere.

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

**46** delle 48 candidate restanti descrivono release da v0.2 a v1.0 le cui epic **esistono già** (E23, E27,
E40, E42, E44, E45 verificate su GitHub). Aprirle ora produrrebbe 46 issue ferme su dipendenze non scritte —
e il loro contenuto è già nell'handoff archiviato, che resta consultabile.

> ⚠️ **Le altre due sono v0.1, e questa sezione le copriva con una giustificazione che non le riguarda.**
> Diceva «48 … descrivono release da v0.2 in poi»: falso per due di esse, perché la sezione v0.1
> dell'handoff contiene **cinque** candidate, non tre. Trovato in code review. Le due hanno già un owner, e
> per ragioni diverse fra loro:
>
> | Candidata v0.1 | Owner | Perché non una issue nuova |
> |---|---|---|
> | 1. *Reconcile tactical camera baseline with current input and focus contract* | **questo triage** | «riconciliare la baseline» è il lavoro che il consolidamento ha appena fatto: il registry ora dichiara i test che esistono, e i tre gap residui sono #863/#864/#865. Non resta niente da riconciliare in una issue |
> | 4. *Integrate selection, focus and planning ability camera contexts* | [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) — CP 11.8 | il *Pointer Interaction Contract* possiede già selezione e contesto esplicito; agganciarci il framing prima che quel contratto sia chiuso creerebbe una seconda sede della stessa decisione |

## 8. Cosa questo triage NON decide

- **Le verifiche PIE della camera.** [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md)
  è nel `writable` della track `content_editor` (#451) in `../parallel-batch.yaml`:
  non è mio, e una voce ⏳ scritta qui sarebbe una modifica al file di un'altra sessione.
- **Gli scenari camera.** Zero voci oggi in [`../../technical/tooling/scenario-map.md`](../../technical/tooling/scenario-map.md);
  i 34 nomi proposti non seguono la convenzione del corpus. Il gap è registrato, il vocabolario no.
- **Le soglie Strategic e le sensibilità di default.** Restano tuning senza consumatore.
- **La Wiki.** `RT-FEAT-UI-TACTICAL-CAMERA` dichiara già `wiki:come-si-gioca` fra i propri `wiki_refs` —
  una delle sette feature che citano quella pagina. Le pagine vivono in un **clone separato** (`D-076`) che
  non è in questo worktree: `docs/wiki/` qui contiene manifest e infografiche, non il testo. Quindi il
  legame esiste già e **se quella pagina copra la camera non è verificabile da qui**: dirlo in un verso o
  nell'altro sarebbe una deduzione dal nome del file.
