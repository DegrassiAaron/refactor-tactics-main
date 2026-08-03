# Guida — Animare i personaggi Paragon (AS.3 / AS.4)

> Guida operativa per l'editor UE 5.8. Riferita a [`spec-asset-pipeline.md`](spec-asset-pipeline.md) (AS.3/AS.4).
> Presuppone la Fase 2 fatta: `BP_Unit_Guardian : RTUnit` con Skeletal Mesh = **Gideon**, cilindro nascosto,
> `VisualZOffset=0`. Tutto quello che segue è **Blueprint** (nessuna ricompilazione C++).

## AS.3 — Animazioni: con Paragon niente retargeting

I personaggi Paragon includono **le loro animazioni native sul loro scheletro** (`Gideon_Skeleton`). Non serve
l'IK Retargeter finché non vuoi condividere animazioni tra scheletri diversi o usare Mixamo (quello resta AS.3
"avanzato", rimandato). Clip utili di **Gideon** (in `/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/`):

| Ruolo nel gioco | Clip Paragon | Note |
|---|---|---|
| Idle | `Idle` | in piedi, loop |
| Corsa (Move) | `Jog_Fwd` | corsa in avanti, loop |
| Attacco (Blast) | `Cast` | Gideon è un caster → "lancio" |
| Colpito (Hit) | `HitReact_Front` | reazione al danno |
| Morte (Defeated) | `Death_Fwd` | caduta in avanti |

> Per **Sparrow** (Ranger), quando è scaricato, cerca gli equivalenti in
> `/Game/ParagonSparrow/.../Animations/` (`Idle`, `Jog_Fwd`, un attacco tipo `Primary`/`Fire`, `HitReact_Front`, `Death_Fwd`).

---

## AS.4a — Locomozione Idle ↔ Run (obiettivo: Gideon corre nel Move)

### 1. Crea l'Animation Blueprint
- Content Browser ▸ `Blueprints/Units` ▸ tasto destro ▸ **Animation ▸ Animation Blueprint**.
- Skeleton: **`Gideon_Skeleton`**. Nome: **`ABP_Gideon`**.

### 2. Variabile di stato
- In `ABP_Gideon` ▸ pannello **My Blueprint** ▸ **+ Variable** ▸ bool **`bIsMoving`**.

### 3. AnimGraph: macchina a stati Idle/Run
- Apri **AnimGraph** ▸ tasto destro ▸ **Add New State Machine** (nome `Locomotion`) ▸ collega la sua uscita a **Output Pose**.
- Doppio clic sulla state machine:
  - **Entry** → nuovo stato **Idle**: dentro trascina l'anim **`Idle`** ▸ collega a **Output Animation Pose**.
  - Nuovo stato **Run**: dentro l'anim **`Jog_Fwd`** ▸ Output Animation Pose.
  - Transizione **Idle → Run**: doppio clic sulla freccia ▸ condizione **`bIsMoving` == true** (trascina `bIsMoving` → `Return Value` del Can Enter Transition).
  - Transizione **Run → Idle**: condizione **`bIsMoving` == false** (usa un nodo **NOT**).
- **Compile ▸ Save**.

### 4. Collega l'AnimBP alla mesh
- Apri `BP_Unit_Guardian` ▸ seleziona lo **Skeletal Mesh Component** ▸ Details ▸ **Animation**:
  - **Animation Mode = Use Animation Blueprint**
  - **Anim Class = `ABP_Gideon`**
- (Rimuovi l'eventuale "Use Animation Asset ▸ Idle" messo prima: ora comanda l'AnimBP.)

### 5. Pilota `bIsMoving` dai delegate del playback (in `BP_Unit_Guardian`, Event Graph)
Il `TurnManager` emette gli eventi del playback. L'unità vi si aggancia e muove la propria animazione.

- **Event BeginPlay**:
  - **Get Actor Of Class** (`RTTurnManager`) → promuovi a variabile **`TurnManager`**.
    (Il `TurnManager` ora è spawnato **prima** delle unità — fix applicato — quindi esiste già: niente Delay.)
  - Da `TurnManager` trascina e **Bind Event to On Unit Move Started** → crea evento custom **`OnMoveStarted (Unit)`**.
  - Idem **Bind Event to On Resolve Playback Finished** → **`OnResolveFinished`**.
- **`OnMoveStarted (Unit)`**: **Branch** `Unit == self` (nodo *Equal (Object)*):
  - True → Skeletal Mesh Component ▸ **Get Anim Instance** ▸ **Cast To `ABP_Gideon`** ▸ **Set `bIsMoving` = true**.
- **`OnResolveFinished`**: Get Anim Instance ▸ Cast To `ABP_Gideon` ▸ **Set `bIsMoving` = false** (tutti tornano idle a fine round).
- **Compile ▸ Save**.

**Verifica (PIE)**: al lock-in del turno, durante la fase **Move** Gideon dovrebbe passare a **`Jog_Fwd`** mentre
scorre lungo il percorso, e tornare a **`Idle`** a fine risoluzione.

---

## AS.4b — Colpi e morte (secondo giro, schema)

Per le clip "una tantum" (`Cast`, `HitReact_Front`, `Death_Fwd`) serve un **Montage** + uno **Slot**:

1. Per ogni clip: tasto destro sull'anim ▸ **Create ▸ Create AnimMontage** (es. `AM_Gideon_Cast`, `AM_Gideon_Hit`, `AM_Gideon_Death`).
2. In `ABP_Gideon` ▸ AnimGraph: tra la state machine `Locomotion` e l'Output Pose inserisci un nodo **Slot 'DefaultSlot'** (così i montaggi vanno in override sopra la locomozione).
3. In `BP_Unit_Guardian` ▸ Event Graph, aggiungi i bind:
   - **On Attack Resolved (Source, Target, Amount)**: se **Source == self** → **Play Anim Montage** `AM_Gideon_Cast`; se **Target == self** → `AM_Gideon_Hit`.
   - **On Unit Defeated (Unit)**: se **Unit == self** → `AM_Gideon_Death`.

> Sincronia col resolver: questi eventi arrivano già al momento giusto del playback (Blast per gli attacchi,
> fine fase per la morte). L'animazione **riproduce**, non decide (invariante #1).

---

## Facing (fatto in C++)

`bFaceMovementDirection` (default off) è pronto su `RTUnit` (70 test verdi). Su `BP_Unit_Guardian` ▸ Class Defaults
spunta **`Face Movement Direction`**: Gideon si orienterà verso la direzione di corsa, così `Jog_Fwd` (che va in
avanti) è credibile in ogni direzione. Solo presentazione: non tocca la logica.

---

## Ripetere per il Ranger
Quando **Sparrow** è scaricato: `ABP_Sparrow` (skeleton `Sparrow_Skeleton`), stesse clip (Idle/Jog_Fwd/attacco/HitReact/Death),
`BP_Unit_Ranger` con Sparrow + `Anim Class = ABP_Sparrow`, e `RangerUnitClass = BP_Unit_Ranger` nel `BP_GameMode`.

## Nota — fix ordine-spawn (applicato)
`ARTGameMode::BeginPlay` ora spawna il `TurnManager` **prima** delle unità: in `BP_Unit` puoi agganciarti ai
delegate direttamente in BeginPlay **senza Delay**. Verificato sicuro (il TurnManager non usa le unità al proprio
BeginPlay, fa solo `StartPlanningTimer`) e ricompilato (70 test verdi).
