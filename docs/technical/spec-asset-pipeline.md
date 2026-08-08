# Spec — Pipeline asset: selezione, import, uso (personaggi, animazioni, audio)

> **Due documenti in uno — leggili separati.**
>
> | Parte | Stato |
> |---|---|
> | **Principi della pipeline** — presentazione-only, riferimenti **soft** con fallback, Blueprint e AnimBP che *consumano* eventi autorevoli e non decidono nulla, licenze da registrare | `CURRENT`. Reggono, e discendono dall'invariante #1 |
> | **«Stato attuale» e pipeline a due archetipi** (Gideon/Sparrow, Guardian/Ranger) | `HISTORICAL`. Era l'esperimento del 2026-08-03: il roster canonico è **Flux · Riva · Bastion · Vektor**. La mappatura Paragon → eroe **non è più aperta**: [D-037](../decisions/RT_PDR_00_Decision_Log.md) del 2026-08-08 assegna `Gadget`, `Phase`, `Riktor` e `Wraith`, tabella owner in [`../characters/paragon.md`](../characters/paragon.md#mapping-visuale-del-roster) |
>
> **Non è l'owner di percorsi e naming**: quello è
> [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md), che è normativo. Dove i due divergono, vince
> quello. *(Cappello aggiunto il 2026-08-08.)*

> `/sc:spec-panel` del **2026-08-03**. Obiettivo utente: *«selezionare gli asset, inserirli e utilizzarli —
> personaggi, animazioni, suoni, musiche; decidere quali usare; essere guidato all'import e all'uso in Unreal»*.
> Panel: **Cockburn** (attore/goal), **Fowler** (architettura/aggancio), **Wiegers** (criteri di selezione),
> **Adzic** (DoD per esempi), **Nygard** (licenze/robustezza/LFS), **Doumont** (chiarezza).
> Ancorata al codice (`RTUnit`, `RTGameMode`, `RTTurnManager`, `RTAbilityData`), al canone
> ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)), alla roadmap ([`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md)) e
> al playback della risoluzione ([`spec-anima-risoluzione.md`](../gameplay/spec-anima-risoluzione.md)).
> Workflow UE verificati sulla **doc ufficiale Epic** (URL in §7); le incertezze sono marcate esplicitamente.
> **Documentale: nessuna modifica al codice in questo documento.**

> ⚠️ **Conflitto di fonte (dichiarato, non risolto in silenzio).** Il canone (§9) prevede per l'MVP
> *«asset placeholder / Starter Content»* e rimanda la **direzione artistica** al post-MVP; le unità sono
> **cilindri** (static mesh). La richiesta — **personaggi skeletali animati** — va **oltre lo scope MVP** ed è
> materiale **north-star**. Decisione dell'utente (2026-08-03) di procedere comunque: recepita e **gated** dietro
> [`adr-0001-skeletal-unit.md`](../decisions/adr-0001-skeletal-unit.md), con **fallback al cilindro** e **invarianti #1/#4
> intatti** (60 test verdi). L'MVP-core resta giocabile anche senza alcun asset.

---

## 1. Obiettivo

Dare al progetto una **pipeline ripetibile** per scegliere, importare e usare asset gratuiti (personaggi,
animazioni, audio, ambiente), a partire dall'**aspetto delle unità** (primo slice scelto dall'utente), **senza**
toccare la logica deterministica del gioco. L'asset è **presentazione**: riproduce ciò che le regole hanno già
deciso (invariante #1).

Non-obiettivi (fuori da questa spec): art direction definitiva, modello dati "ricco" north-star, GAS, VFX di
gameplay che influenzano l'esito.

---

## 2. Stato attuale (verificato dal codice)

| Fatto | Evidenza |
|---|---|
| Unità = **cilindro**: `Mesh = UStaticMeshComponent` root, mesh `/Engine/BasicShapes/Cylinder` | `RTUnit.cpp:15-25` |
| Il playback **muove l'Actor intero** via `SetVisualLocation`→`SetActorLocation` (non la mesh) | `RTUnit.cpp:92-95`, `RTTurnManager.cpp:528,1066,1097,1143` |
| Offset verticale hardcoded: `WorldForCell` aggiunge `UnitHalfHeight = 90` (pivot **al centro** del cilindro) | `RTUnit.cpp:86-90`, `RTUnit.h:209` |
| Identità team = MID con parametro `"Color"` su `M_Unit` (soft-ptr con fallback grigio + warning) | `RTUnit.cpp:54-75`, `RTUnit.h:250` |
| Selezione = `Mesh->SetRelativeScale3D` (ingrandimento 15%) | `RTUnit.cpp:97-111` |
| Spawn unità: `SpawnActorDeferred<ARTUnit>` per 4 unità 2v2, poi `ConfigureAsArchetype` | `RTGameMode.cpp:71-90` |
| **Hook di animazione già presenti** (delegate BlueprintAssignable) | `RTTurnManager.h:71-84` |
| `URTAbilityData` è data-driven ma **senza campi di presentazione** (no icona/VFX/SFX) | `RTAbilityData.h` |
| `Content/` quasi vuoto: solo `L_Prototype.umap`, `M_Unit.uasset` | glob `Content/**` |
| Convenzioni asset canoniche **non** coprono skeletal/anim/audio ricco | `piano-canonico-mvp.md §5` |

Delegate disponibili (`RTTurnManager.h:13-84`), i **punti d'aggancio** per l'animazione:

| Delegate | Firma | Uso in presentazione |
|---|---|---|
| `OnPhasePlaybackStarted` | `(ERTMatchPhase)` | SFX/telecamera per fase |
| `OnUnitMoveStarted` | `(ARTUnit*)` | avvia anim di corsa |
| `OnAttackResolved` | `(ARTUnit* Src, ARTUnit* Tgt, int32 Amount)` | montage attacco + SFX colpo + reazione bersaglio |
| `OnUnitDefeated` | `(ARTUnit*)` | anim/VFX/SFX di morte, poi `HideForDefeat` |
| `OnResolvePlaybackFinished` | `()` | ritorno a idle, sblocco input |

---

## 3. Principio fondante

> **Le regole decidono l'esito; gli asset riproducono.** (Invariante #1.)

Tre regole non negoziabili per **ogni** asset di presentazione:

1. **Mai nella logica né nei test.** I 60 automation test provano la logica pura: non devono dipendere da alcun
   `.uasset`. Un asset che influenza l'esito è un bug di architettura.
2. **Sempre riferimento soft con fallback.** `TSoftObjectPtr`/`TSoftClassPtr`; asset assente ⇒ **degrada**
   (cilindro / silenzio), non crasha. Stesso pattern di `UnitMaterial` (`RTUnit.cpp:65`).
3. **Agganciato al confine giusto.** Presentazione in **Blueprint** (canone): AnimBP + `BP_Unit`, pilotati dai
   delegate di playback. Il C++ resta autorevole.

---

## 4. Architettura di aggancio (Fowler)

### 4.1 Unità: da cilindro a skeletal (gated, con fallback)

Scelta raccomandata (canone "presentazione in Blueprint"): **`BP_Unit : ARTUnit`** aggiunge in Blueprint uno
`USkeletalMeshComponent` + AnimBP; `ARTGameMode` spawna il BP tramite `TSubclassOf<ARTUnit>` **configurabile per
archetipo**, non più `ARTUnit::StaticClass()` fisso (`RTGameMode.cpp:80`). Dettaglio della decisione in
[`adr-0001-skeletal-unit.md`](../decisions/adr-0001-skeletal-unit.md).

Punti che il refactor **deve** gestire (dal codice):

| Tema | Problema | Intervento |
|---|---|---|
| **Pivot** | il cilindro ha pivot al centro (`+90` Z); i personaggi UE hanno pivot **ai piedi** | esporre `VisualZOffset` (`UPROPERTY`), sostituire l'`UnitHalfHeight` hardcoded in `WorldForCell` |
| **Movimento** | `SetVisualLocation` muove l'Actor → **nessun refactor del playback** ✅ | nessuno; il personaggio segue l'Actor |
| **Identità team** | MID `"Color"` sullo static mesh (`RTUnit.cpp:69`) non si applica bene a un personaggio texturizzato | anello/decal a terra **o** outline per team (deciso in slice AS.5) |
| **Selezione** | scala della mesh (`RTUnit.cpp:101`) | scalare il `SkeletalMeshComponent` o mostrare un ring di selezione |
| **Fallback** | asset mancante | se lo skeletal soft-ptr è nullo, resta il cilindro (test verdi) |

### 4.2 Animazioni pilotate dai delegate (non da `GetVelocity`)

Poiché il movimento è `SetActorLocation` (teleport per-frame), **l'Actor non ha velocità reale** → l'AnimBP
**non** può usare lo `Speed` da `GetVelocity()`. Le animazioni si pilotano dagli eventi:

| Evento | Effetto in AnimBP/BP_Unit |
|---|---|
| `OnUnitMoveStarted` | `bIsMoving = true` → stato *Run* |
| `OnResolvePlaybackFinished` | `bIsMoving = false` → stato *Idle*; sblocco |
| `OnAttackResolved` (Src) | `PlayMontage(Attack)` |
| `OnAttackResolved` (Tgt) | `PlayMontage(Hit)` (reazione al colpo) |
| `OnUnitDefeated` | `PlayMontage(Death)` → `HideForDefeat` |

### 4.3 Abilità con presentazione (slice futuro, non slice 1)

Se le abilità dovranno avere icona/VFX/SFX propri, **estendere** `URTAbilityData` con soft-ptr
(`TSoftObjectPtr<UTexture2D> Icon`, `TSoftObjectPtr<UNiagaraSystem> HitVfx`, `TSoftObjectPtr<USoundBase> Sfx`) —
dati di presentazione referenziati soft, **mai** hard, e ignorati dalla logica.

### 4.4 Audio, musica, ambiente (slice futuri)

- **SFX**: `PlaySound2D`/`SpawnSoundAtLocation` sui delegate (§4.1); a bassissimo rischio (nessun refactor).
- **Musica**: avvio in `BeginPlay` del GameMode/HUD; opzionale ducking durante il playback.
- **Ambiente**: materiali/props/illuminazione su `L_Prototype` (indipendenti dalla logica).

---

## 5. Convenzioni asset — estensione del canone §5 (**proposta, da approvare**)

Il canone §5 copre `BP_ WBP_ BPI_ DA_ DT_ IA_ IMC_ L_ M_/MI_ T_ NS_ S_` ma non skeletal/anim/audio ricco.
Estensione proposta (coerente col set esistente; modificare il canone richiede approvazione):

| Prefisso | Tipo | Nota |
|---|---|---|
| `SK_` | Skeletal Mesh | il template UE5 usa `SKM_` (es. `SKM_Manny`): mantenere `SK_` per il progetto, `SKM_` per gli asset engine |
| `SKEL_` | Skeleton | asset scheletro condiviso |
| `PA_` | Physics Asset | ragdoll/collisioni |
| `ABP_` | Animation Blueprint | logica di stato dell'anim |
| `A_` | Animation Sequence | clip singola |
| `AM_` | Anim Montage | attacco/morte a evento |
| `BS_` | Blend Space | locomozione (se serve) |
| `IK_` / `RTG_` | IK Rig / IK Retargeter | retargeting |
| `MS_` | MetaSound | audio procedurale |
| `SA_` | Sound Attenuation | spazializzazione |
| (`S_`, `NS_`, `T_`, `M_/MI_` restano) | Sound / Niagara / Texture / Material | invariati |

Struttura cartelle proposta:

```
Content/
  Characters/<Nome>/       SK_, SKEL_, PA_, materiali, T_
  Animations/<Nome>/       A_, AM_, ABP_, BS_, IK_, RTG_
  Blueprints/Units/        BP_Unit, BP_Unit_Ranger, BP_Unit_Guardian
  Audio/SFX/  Audio/Music/ S_, MS_, SA_
  VFX/                     NS_
```

---

## 6. Criteri di selezione (Wiegers) — "bello" non è un requisito

Un asset è **accettabile** solo se soddisfa criteri verificabili:

- **Leggibilità dall'alto**: silhouette distinguibile con camera tattica (pitch -55°); i due team e i due
  archetipi (Ranger snello / Guardian tozzo) restano riconoscibili a colpo d'occhio.
- **Scheletro compatibile**: umanoide bipede retargetabile sullo scheletro **UE5 Manny** (per riusare/retargetare
  animazioni da fonti diverse). Vedi §7.3.
- **Budget ragionevole**: poligoni/texture adatti a più unità in scena (evitare MetaHuman full-quality per 4
  unità simultanee senza LOD).
- **Formato importabile**: FBX/gLTF per mesh+anim; audio WAV/OGG.
- **Licenza compatibile** con la nota IP (canone §2/§9): vedi §8. Un asset senza licenza tracciata **non** entra.

**Checklist di accettazione (per asset):** ☐ silhouette leggibile ☐ scheletro retargetabile ☐ budget ok
☐ importa senza errori ☐ licenza registrata ☐ fallback verificato.

---

## 7. Pipeline operativa — workflow UE 5.8 (verificati su doc ufficiale)

> Fonti: doc ufficiale Epic (URL per sezione). I punti marcati **[community/da verificare in editor]** non sono
> confermati dalla doc ufficiale: trattali come prassi, non come certezze.

### 7.1 Fab / Paragon (personaggi gratuiti Epic)

1. Apri **Window > Fab** (sezione *Get Content*), oppure il pulsante **Fab** nel **Content Drawer**.
2. Se assente: **Edit > Plugins** → cerca **Fab** → abilita (attivo di default).
3. Filtra **Price > Free**; i contenuti **Paragon** / **Infinity Blade** sono gratuiti Epic.
4. Sulla scheda prodotto: **Add to My Library**; poi **My Library** → icona **+** sulla tile → importa nel
   progetto aperto.
5. **[da verificare in editor]** nomi esatti dei pack Paragon ed etichette dei pulsanti in-editor: la doc conferma
   presenza e filtro *Free*, non i singoli nomi/label.
- Fonti: `dev.epicgames.com/documentation/en-us/unreal-engine/fab-window-in-unreal-engine` ·
  `.../free-epic-games-content-for-unreal-engine`

### 7.2 Import Mixamo (FBX → UE5)

1. **Content Browser > Import** → seleziona l'FBX → **FBX Import Options**.
2. **Skeleton**: lascia **vuoto** al primo import del character (crea nuovo Skeleton); per le clip successive
   **seleziona lo Skeleton** già creato.
3. Sezioni: **Import Mesh** (Skeletal Mesh), **Import Animations**, **Animation Length** (Exported/Animated/Set Range).
4. **Import Uniform Scale** (sezione *Transform*) per correggere la scala.
5. **Root motion**: **non** è un toggle di import → si abilita sull'Animation Sequence
   (**Enable Root Motion** + *Force Root Lock*) nell'editor dell'anim.
6. **[community/da verificare]** scala 100×, orientamento, T-pose/ref pose: **non** confermati dalla doc Epic;
   verifica caso per caso il valore di *Import Uniform Scale* e l'opzione *In Place* lato Mixamo.
- Fonti: `.../fbx-import-options-reference-in-unreal-engine` · `.../importing-skeletal-meshes-using-fbx-in-unreal-engine`
  · `.../root-motion-in-unreal-engine`

### 7.3 IK Rig + IK Retargeter (Mixamo/Paragon → Manny)

1. Crea due **IK Rig**: **Add (+) > Animation > IK Rig** per la mesh **sorgente** e per la **target**.
2. In ciascuno: **Hierarchy** → clic destro sul pelvis → **Set Retarget Root**.
3. **Retarget Chains**: seleziona spine/arms/legs/neck/head → **New Retarget Chain from Selected Bones**
   (o **Auto Create Retarget Chains**).
4. **Add (+) > Animation > IK Retargeter** → scegli l'IK Rig **sorgente**; apri l'editor.
5. Imposta **Target IKRig Asset** = IK Rig target; verifica il **Chain Mapping**.
6. **Asset Browser** → seleziona le anim → **Export Selected Animations** (suffisso `_Retargeted`).
- Fonti: `.../retargeting-bipeds-with-ik-rig-in-unreal-engine` · `.../ik-rig-animation-retargeting-in-unreal-engine`

### 7.4 MetaHuman (UE 5.6+)

1. **Versioni**: da **UE 5.6** MetaHuman Creator gira **in-editor** (5.5 e prima: web app). In 5.8 usare il flusso in-editor.
2. **Edit > Plugins** → abilita **MetaHuman Creator** (in installazione: **MetaHuman Creator Core Data**).
3. Crea un asset **MetaHuman Character** → doppio clic → editor MetaHuman.
4. Da Fab (doc 5.6): **Window > Fab** → canale MetaHuman → **Add to Project** (assemblati in `/Game/MetaHumans`).
   Prodotti MetaHuman richiedono **UE ≥ 5.6**.
5. **Costo/rischio**: alta fedeltà, pesante per 4 unità simultanee → valuta LOD o riservalo a scopo didattico.
- Fonti: `dev.epicgames.com/documentation/metahuman/metahuman-creator-in-unreal-engine` · `.../buying-metahumans-from-fab`

### 7.5 Collegare Skeletal Mesh + AnimBlueprint sull'Actor

1. UE5 mannequin: **SKM_Manny** / **SKM_Quinn** con IK Rig condiviso **IK_Mannequin** (target di retarget).
2. Sul **Skeletal Mesh Component** → **Details > Animation** → **Animation Mode = Use Animation Blueprint** →
   **Anim Class = ABP_Unit**.
3. **[da verificare in editor]** nome esatto dell'asset **Skeleton** condiviso da SKM_Manny/Quinn (confermati mesh
   e IK Rig, non il nome dello Skeleton).
- Fonti: `.../third-person-template-in-unreal-engine` · `.../skeletal-mesh-animation-system-in-unreal-engine` ·
  `.../animation-blueprints-in-unreal-engine`

---

## 8. Licenze & provenienza (Nygard) — obbligatorio per la nota IP §2/§9

| Fonte | Cosa è consentito | Fonte ufficiale |
|---|---|---|
| **Paragon / Fab (Epic)** | Fab Standard License: uso commerciale/privato, modifica, distribuzione **incorporata** nel progetto; **vietato** rivendere l'asset standalone. Tier Personal (<$100k) / Professional. | `dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab` · `fab.com/eula` |
| **Mixamo (Adobe)** | Character/anim **royalty-free** in progetti personali/commerciali/no-profit se **incorporati**; **vietato** rivendere i file grezzi; niente attribuzione. | `helpx.adobe.com/creative-cloud/faq/mixamo-faq.html` |
| **MetaHuman** | UE ≤ 5.5: MetaHuman Creator EULA; UE ≥ 5.6: **Unreal Engine EULA**. Uso **dentro UE**; **vietato** vendere/trasferire la tecnologia standalone. | `unrealengine.com/eula/mhc` · `unrealengine.com/eula/unreal` |
| **CC0 / CC-BY (itch.io, OpenGameArt, Freesound)** | CC0: libero; CC-BY: **richiede attribuzione**. Verificare per singolo asset. | pagina del singolo asset |

**Requisito `FR-ASSET-LIC-01`**: ogni asset importato ha una riga in un **registro di provenienza**
(`docs/technical/asset-licenze.md` o `DT_AssetProvenance`): nome, fonte, URL, licenza, data, note attribuzione.
*Verifica: nessun asset in `Content/` privo di riga nel registro.*

---

## 9. Robustezza & Git (Nygard)

- **Git LFS**: mesh/anim/audio/texture sono binari → `.gitattributes` deve tracciarli (`.uasset`/`.umap` già in
  LFS, roadmap CP 0.3). Verificare `git lfs ls-files` dopo l'import.
- **Bloat**: non versionare interi pack se ne serve una frazione; valutare cosa entra in `Content/`.
- **Redirectors**: rinominare/spostare asset crea redirector → **Content Browser > Fix Up Redirectors** prima del commit.
- **Fallback**: soft-ptr nulla ⇒ cilindro/silenzio (nessun crash).
- **Determinismo**: asset di presentazione **fuori** dai test → i 60 test non cambiano esito (invariante #4).

---

## 10. Testabilità & Definition of Done (Adzic / Crispin)

Esempi (Given/When/Then) per lo **slice 1 — aspetto unità**:

- *Dato* `BP_Unit_Ranger` con `SK_Ranger` + `ABP_Unit`, *quando* si avvia PIE, *allora* al posto del cilindro
  compare il personaggio, appoggiato a terra (nessun "fluttuamento").
- *Quando* la fase **Move** riproduce, *allora* i personaggi che si muovono giocano l'anim di corsa e tornano
  a idle a fine risoluzione.
- *Dato* uno skeletal **assente** (soft-ptr nullo), *quando* si avvia, *allora* resta il cilindro e **i 60 test
  restano verdi** (nessun crash).

**DoD dello slice 1:** ☐ 60 automation test ancora verdi ☐ PIE: personaggio visibile e appoggiato a terra
☐ corsa in Move + idle a fine playback ☐ fallback cilindro verificato ☐ licenze registrate (§8)
☐ nessun redirector/segreto/file generato committato ☐ `spec`/`adr` aggiornati.

---

## 11. Roadmap / slicing (primo slice = "aspetto unità")

| ID | Cosa | Verifica |
|----|------|----------|
| **AS.1** | Refactor `ARTUnit`: `SkeletalMeshComponent` opzionale + `VisualZOffset` + spawn via `TSubclassOf` per archetipo, **fallback cilindro** | Test C++ (fallback: senza skeletal l'unità resta valida) + 60 test verdi |
| **AS.2** | `BP_Unit` + `ABP_Unit` minimale (Idle+Run) su un archetipo (Ranger), da Fab/Paragon o Mixamo | PIE: **si vede il personaggio muoversi** in Move |
| **AS.3** | Set anim retargetato (Idle/Run/Attack/Hit/Death) su Manny (§7.3) | PIE: le clip girano correttamente |
| **AS.4** | Anim pilotate dai delegate (Attack/Hit/Death) sincronizzate col playback (§4.2) | PIE: colpo/morte coerenti con `spec-anima` |
| **AS.5** | Identità di team (anello/decal a terra o outline) | PIE: team leggibili a colpo d'occhio |
| **AS.6** | Secondo archetipo (Guardian) + tuning scala/offset | PIE: Ranger vs Guardian distinguibili |

Slice successivi (spec separata o estensione): **Audio** (SFX sui delegate + musica), **Ambiente/mappa**,
**Abilità** (icona/VFX/SFX via §4.3). L'ordine dopo l'aspetto unità è da decidere.

---

## 12. Decisioni

**Prese (2026-08-03):**
- Personaggi skeletali animati (north-star), **gated** dietro [`adr-0001-skeletal-unit.md`](../decisions/adr-0001-skeletal-unit.md).
- Presentazione in **Blueprint** (`BP_Unit : ARTUnit` + AnimBP), spawn via `TSubclassOf` configurabile.
- Animazioni **pilotate dai delegate**, non da `GetVelocity`.
- **Fallback cilindro** obbligatorio; asset fuori da logica/test.
- Primo slice = **aspetto delle unità** (AS.1–AS.2 come prima consegna verificabile).
- Fonti multiple ammesse: Fab/Paragon, Mixamo, MetaHuman, CC0.

**Aperte (da confermare, tipicamente in PIE/design):**
- Identità di team con personaggi reali: anello/decal vs outline vs MID.
- **Coerenza visiva** con fonti miste (Paragon + Mixamo + MetaHuman rischiano stili incoerenti) → può servire
  scegliere **una** fonte primaria per lo stile.
- Mappatura personaggio→archetipo (chi è il Ranger, chi il Guardian).
- Valori di `VisualZOffset`/scala per ogni personaggio.
- Se estendere `URTAbilityData` con la presentazione ora o in uno slice audio/VFX.

---

## 13. Riferimenti

- Canone: [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) — invarianti #1 (regole decidono), #4 (determinismo), §5 (convenzioni asset), §9 (art direction aperta).
- Roadmap: [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md).
- Playback e delegate: [`spec-anima-risoluzione.md`](../gameplay/spec-anima-risoluzione.md).
- Decisione architetturale: [`adr-0001-skeletal-unit.md`](../decisions/adr-0001-skeletal-unit.md).
- Codice: `RTUnit.cpp/.h`, `RTGameMode.cpp`, `RTTurnManager.cpp/.h`, `RTAbilityData.h`.
- Doc UE ufficiale: Fab, FBX Import, IK Rig/Retargeter, MetaHuman, Skeletal Mesh Animation (URL in §7–§8).

---

## 14. Stato di implementazione (2026-08-03)

Branch `feat/skeletal-units`. **Parte C++ di AS.1 completata in TDD** (editor chiuso, build/test headless):

- **AS.1a** ✅ — `URTGridLibrary::CellToWorldElevated` (funzione pura) + `VisualZOffset` (`UPROPERTY`, default 90 =
  retrocompat cilindro; 0 = pivot ai piedi) usato in `ARTUnit::WorldForCell`. **3 test** RED→GREEN
  (`RTGridElevationTests.cpp`: `AppliesZOffset`, `GroundedAtZeroOffset`, `AddsLayerHeight`). Il RED ha scoperto un
  test debole (Grounded controllava solo Z, passava con lo stub) → rafforzato a confronto pieno (X,Y,Z).
- **AS.1b** ✅ *(C++)* — `ARTGameMode` espone `RangerUnitClass`/`GuardianUnitClass` (`TSubclassOf<ARTUnit>`), usate
  in `SpawnUnit` con **fallback** ad `ARTUnit` (cilindro) se non assegnate. Lo `SkeletalMeshComponent` **non** è in
  C++: starà in `BP_Unit` (presentazione in Blueprint, canone), creato in AS.2. Wiring non testabile in automation
  (spawn/World): verifica = build + PIE con l'asset reale (AS.2).
- **AS.4 groundwork (facing)** ✅ — `URTGridLibrary::DirectionYaw` (pura, TDD RED→GREEN) + `ARTUnit::bFaceMovementDirection`
  (default false) in `SetVisualLocation`: le unità dei personaggi si orientano verso il movimento. Più fix ordine-spawn
  (`TurnManager` prima delle unità) così i `BP_Unit` si agganciano ai delegate senza Delay.

**Verifica**: build `RefactorTacticsEditor` Development → *Result: Succeeded*; `Automation RunTests RefactorTactics`
→ **70/70 verdi, 0 falliti** (4 test nuovi: 3 CellToWorldElevated + 1 DirectionYaw). Nessuna regressione (default
`VisualZOffset=90` e `bFaceMovementDirection=false` preservano il comportamento del cilindro).

**Aperto — AS.2 (editor, con guida)**: creare `BP_Unit` Ranger/Guardian con `SkeletalMeshComponent` + `ABP_Unit`,
`VisualZOffset=0`, e assegnarli a `RangerUnitClass`/`GuardianUnitClass`; importare il primo personaggio
(Fab/Mixamo); ridefinire l'identità di team con un personaggio texturizzato (§4.1).
