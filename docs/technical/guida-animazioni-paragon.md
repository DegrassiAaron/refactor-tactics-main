# Guida — Workflow prototipo Paragon: animare un personaggio (AS.3 / AS.4)

> `CURRENT` come **metodo**, `HISTORICAL` come **casting** · **Ultimo aggiornamento**: 2026-08-08
>
> **Il procedimento vale; i nomi dei personaggi no.** Questa guida usa **Gideon** e **Sparrow** con gli
> archetipi *Guardian* e *Ranger*: erano il prototipo del 2026-08-03, **non** il roster canonico, che è
> **Flux · Riva · Bastion · Vektor**.
>
> **Il casting non è più aperto** (2026-08-08, [D-037](../decisions/RT_PDR_00_Decision_Log.md)) e dal
> **2026-08-11 gli asset portano il nome del pack**, non dell'eroe
> ([`convenzioni-contenuti-ue.md` §5b](convenzioni-contenuti-ue.md)). Questa è la tabella da cui tradurre
> ogni esempio qui sotto — **verificata sul disco l'11-08**, perché tre nomi su otto non sono quelli che ci
> si aspetta:
>
> | Eroe | Pack | Blueprint / AnimBP | Skeletal Mesh | Skeleton |
> |---|---|---|---|---|
> | Flux | `Paragon.Gadget` | `BP_Unit_Gadget` · `ABP_Gadget` | `Gadget` | ⚠️ `gadget_bot_Skeleton` |
> | Riva | `Paragon.Phase` | `BP_Unit_Phase` · `ABP_Phase` | ⚠️ `Phase_GDC` | `phase_Skeleton` |
> | Bastion | `Paragon.Riktor` | `BP_Unit_Riktor` · `ABP_Riktor` | `Riktor` | ⚠️ `belly_Riktor_Skeleton` |
> | Vektor | `Paragon.Wraith` | `BP_Unit_Wraith` · `ABP_Wraith` | `Wraith` | `Wraith_Skeleton` |
>
> Cartelle: `/Game/RT/Characters/<Pack>/{Blueprints,Animation}/`. Pack in
> `/Game/FabAsset/Paragon/Paragon<Pack>/Characters/Heroes/<Pack>/…`.
>
> ⚠️ **La mesh di Riva non si chiama `Phase`**: è `Phase_GDC` (22,6 MB — gli altri file in quella cartella
> pesano 0,1 MB e sono extents, shadow e skeleton). E due scheletri su quattro non portano il nome del
> personaggio. Sono gli errori che costano una sessione di editor a cercare un asset che non esiste.
>
> Gli esempi restano scritti su **Gideon/Sparrow** perché sono il *procedimento* registrato nel prototipo del
> 2026-08-03: leggi «Gideon» come «il personaggio che stai importando» e traduci con la tabella. Riscriverli
> uno per uno introdurrebbe più errori di quanti ne toglierebbe, e la guida resterebbe comunque un esempio.

> Guida operativa per l'editor UE 5.8.1. Riferita a [`spec-asset-pipeline.md`](spec-asset-pipeline.md) (AS.3/AS.4).
> Presuppone la Fase 2 fatta: un `BP_Unit_<Pack> : ARTUnit` con Skeletal Mesh assegnata, cilindro **nascosto**
> (`ARTUnit::Mesh` è uno `UStaticMeshComponent` e resta il root: si aggiunge una skeletal accanto, non la si
> sostituisce), `VisualZOffset=0` e i due materiali degli anelli. I passi completi sono in
> [`../roadmap/editor-sessions.yaml`](../roadmap/editor-sessions.yaml), seduta **U7**.
> Tutto quello che segue è **Blueprint** (nessuna ricompilazione C++).

## AS.3 — Animazioni: con Paragon niente retargeting

I personaggi Paragon includono **le loro animazioni native sul loro scheletro** (`Gideon_Skeleton`). Non serve
l'IK Retargeter finché non vuoi condividere animazioni tra scheletri diversi o usare Mixamo (quello resta AS.3
"avanzato", rimandato). Clip utili di **Gideon** (in `/Game/FabAsset/Paragon/ParagonGideon/Characters/Heroes/Gideon/Animations/`):

| Ruolo nel gioco | Clip Paragon | Note |
|---|---|---|
| Idle | `Idle` | in piedi, loop |
| Corsa (Move) | `Jog_Fwd` | corsa in avanti, loop |
| Attacco (Blast) | `Cast` | Gideon è un caster → "lancio" |
| Colpito (Hit) | `HitReact_Front` | reazione al danno |
| Morte (Defeated) | `Death_Fwd` | caduta in avanti |

> Per **Sparrow** (Ranger), quando è scaricato, cerca gli equivalenti in
> `/Game/FabAsset/Paragon/ParagonSparrow/.../Animations/` (`Idle`, `Jog_Fwd`, un attacco tipo `Primary`/`Fire`, `HitReact_Front`, `Death_Fwd`).

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

## AS.4b — Colpi e morte (montaggi via eventi C++)

Il C++ è pronto (commit `feat(unit): eventi montaggio colpi/morte`): `RTUnit` espone 3 eventi —
`PlayAttackMontage`, `PlayHitMontage`, `PlayDefeatMontage` — che il `TurnManager` chiama **da solo**
sull'attaccante e sul bersaglio (colpi risolti) e sull'unità che muore. **Niente bind/branch/cast**: implementi
solo i 3 eventi nel BP (uniforme per ogni personaggio; se un evento non è implementato, nessun effetto — invariante #1).

1. **Montaggi** (tasto destro sull'anim ▸ **Create ▸ Create AnimMontage**):
   - Gideon: `Cast`→`AM_Gideon_Attack`, `HitReact_Front`→`AM_Gideon_Hit`, `Death_Fwd`→`AM_Gideon_Death`.
   - Sparrow: attacco (tiro), `HitReact`, `Death` → `AM_Sparrow_*`.
2. **Slot** in `ABP_Gideon`/`ABP_Sparrow` ▸ AnimGraph: inserisci un nodo **Slot 'DefaultSlot'** tra la State
   Machine `Locomotion` e l'Output Pose (i montaggi vanno in override su idle/run).
3. **BP_Unit** (Guardian/Ranger) ▸ Event Graph: aggiungi gli eventi (tasto destro ▸ cerca il nome) e collega
   ciascuno a un **Play Anim Montage** (target: lo Skeletal Mesh Component):
   - **Event Play Attack Montage** → `AM_…_Attack`
   - **Event Play Hit Montage** → `AM_…_Hit`
   - **Event Play Defeat Montage** → `AM_…_Death`
   - **Compile ▸ Save**.
4. **PIE**: colpo → attacco + reazione del bersaglio; morte → caduta, sincronizzati col playback.

> Sincronia col resolver: gli eventi arrivano già al momento giusto (Blast per i colpi, fine fase per la morte).
> L'animazione **riproduce**, non decide (invariante #1).

---

## Facing (fatto in C++)

`bFaceMovementDirection` (default off) è pronto su `RTUnit` (70 test verdi). Su `BP_Unit_Guardian` ▸ Class Defaults
spunta **`Face Movement Direction`**: Gideon si orienterà verso la direzione di corsa, così `Jog_Fwd` (che va in
avanti) è credibile in ogni direzione. Solo presentazione: non tocca la logica.

---

## Ripetere per il Ranger (Sparrow) — via duplicato

Sparrow ha uno **skeleton diverso** (`Sparrow_Skeleton`) → serve un nuovo AnimBP, ma si riusa il BP unità:

1. **Duplica** `BP_Unit_Guardian` → **`BP_Unit_Ranger`** (eredita cilindro nascosto, `VisualZOffset=0`, `Face Movement Direction` e il wiring dei delegati).
   - Skeletal Mesh Component ▸ **Skeletal Mesh Asset = `Sparrow`** (`/Game/FabAsset/Paragon/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow`).
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
