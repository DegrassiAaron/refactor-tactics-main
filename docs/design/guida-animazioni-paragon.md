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

## Ripetere per il Ranger (Sparrow) — via duplicato

Sparrow ha uno **skeleton diverso** (`Sparrow_Skeleton`) → serve un nuovo AnimBP, ma si riusa il BP unità:

1. **Duplica** `BP_Unit_Guardian` → **`BP_Unit_Ranger`** (eredita cilindro nascosto, `VisualZOffset=0`, `Face Movement Direction` e il wiring dei delegati).
   - Skeletal Mesh Component ▸ **Skeletal Mesh Asset = `Sparrow`** (`/Game/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow`).
2. **`ABP_Sparrow`** (Animation Blueprint, skeleton `Sparrow_Skeleton`): State Machine Idle/Run come ABP_Gideon —
   Idle = `Travel_Mode_Idle_BowDown` (o `Idle_FrontEnd`), Run = `Jog_Fwd`, transizioni su `bIsMoving`.
3. `BP_Unit_Ranger` ▸ Skeletal Mesh Component ▸ **Anim Class = `ABP_Sparrow`**.
4. ⚠️ Nell'**Event Graph** del `BP_Unit_Ranger` (ereditato) i due nodi **Cast To ABP_Gideon** vanno cambiati in
   **Cast To ABP_Sparrow** (altrimenti il set `bIsMoving` non scatta su Sparrow). Ricollega exec + Set `bIsMoving`.
5. `BP_GameMode` ▸ **Ranger Unit Class = `BP_Unit_Ranger`** ▸ Compile ▸ Save.
6. PIE: i Ranger diventano Sparrow e corrono nel Move.

## Nota — fix ordine-spawn (applicato)
`ARTGameMode::BeginPlay` ora spawna il `TurnManager` **prima** delle unità: in `BP_Unit` puoi agganciarti ai
delegate direttamente in BeginPlay **senza Delay**. Verificato sicuro (il TurnManager non usa le unità al proprio
BeginPlay, fa solo `StartPlanningTimer`) e ricompilato (70 test verdi).

---

## AS.5 — Identità di team: crea `M_TeamRing` (anello a terra)

Il C++ è pronto (`TeamRing` + `TeamColorFor`, 71 test verdi): sotto ogni unità c'è un anello che si colora per
squadra **se** trova il materiale `M_TeamRing`; senza, resta nascosto (fallback → cilindro colorato come prima).
Serve creare **un solo materiale** in editor.

1. **Content Browser ▸ `Materials`** ▸ tasto destro ▸ **Material** ▸ nome **`M_TeamRing`**.
2. Aprilo ▸ **Details ▸ Material ▸ Shading Model = `Unlit`** (colore pieno e brillante dall'alto).
3. Nel grafo: tasto destro ▸ **Parameter ▸ VectorParameter** ▸ nome **`Color`** (esattamente `Color`: è il parametro che il C++ imposta).
4. Collega **`Color` → Emissive Color**.
5. *(Opzionale, anello vero col buco centrale)* **Blend Mode = `Masked`** + una maschera radiale sull'**Opacity Mask**.
6. **Compile ▸ Save**.
7. Assegnalo: su ogni **`BP_Unit`** ▸ **Class Defaults ▸ `Team Ring Material` = `M_TeamRing`**.

**Verifica (PIE-AS5)**: sotto ogni unità un anello **blu** (team 0) / **rosso** (team 1), visibile dall'alto.
Se il disco è troppo grande/piccolo è il `RelativeScale3D` del `TeamRing` (ora `1.6` ≈ raggio 80 cm): chiedi la taratura.
