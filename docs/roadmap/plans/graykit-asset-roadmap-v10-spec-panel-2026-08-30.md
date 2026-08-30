# GrayKit — roadmap ASSET-ONLY fino alla v1.0

> `CURRENT` · **Stato**: audit chiuso, due issue create, quattro aggiornate con commento · **Data**: 2026-08-30
> **HEAD della revisione**: `9434b950` (`main`, albero pulito) · **Motore misurato**: **UE 5.8.1**
> (`D:/EpicGames/UE_5.8/Engine/Build/Build.version` → `Major 5 · Minor 8 · Patch 1 · ++UE5+Release-5.8`;
> `RefactorTactics.uproject` dichiara `EngineAssociation: "5.8"`, che è l'associazione e non la patch)
> **Panel**: Wiegers (lead) · Cockburn · Fowler · Nygard · Crispin · Adzic
> **Modo**: critique · **Perimetro**: asset, integrazione, leggibilità, sostituibilità. Nessuna feature gameplay.

---

## 1. Il verdetto in una riga

Il GrayKit v0.1 **esiste, è generato, è testato e non lo usa nessuno**: nessun `.umap` versionato e nessuna
riga di `Source/` referenzia le sei `SM_Graybox_*` — l'unico file che le cita è il file stesso. La roadmap
candidata A01–A11 pianifica cook budget, LOD e minimum-spec per un kit che ha **zero istanze in gioco**, e
distribuisce undici nodi su una ladder di release che nel progetto è organizzata per **temi di sistema**
(v0.3 Informazione, v0.5 Online Foundation, v0.6 Ability Runtime), non per stadi di maturità dell'arte.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **4** |
| 🟡 Medio | **3** |

**Raccomandazione operativa**: **due** issue nuove su undici proposte. Otto sezioni hanno già un owner vivo,
una va lasciata cadere. Il percorso critico non passa dai pack Fab.

⚠️ Nessuna suite eseguita, nessuna build, nessun `.uasset` toccato. Issue lette lato server con `gh` a
`9434b950`; `Source/`, `Content/` e `docs/` con `git grep`/`git ls-files` sullo stesso `HEAD`.

⏱️ **`HEAD` è avanzato a `7c57d255` durante la revisione** — un'altra sessione condivide questa working
directory (`D-222`). Il commit tocca **un solo file**, `docs/roadmap/plans/tactical-grid-overlay-roadmap-2026-08-30.md`,
e nessun percorso del perimetro: `git diff --name-only 9434b950..HEAD` non porta né `Content/RT/World/Graybox/`,
né il commandlet, né `asset-map.md`, né `.gitignore`. La misura regge; il fatto è dichiarato perché una
revisione che non dice su quale albero è stata presa non è verificabile.

---

## 2. Stato reale del GrayKit — misurato, non ricordato

### 2.1 Cosa esiste

`git ls-files 'Content/RT/World/Graybox/*'` dà **7**:

```text
Cover/     SM_Graybox_Cover_Low     SM_Graybox_Cover_High
Doors/     SM_Graybox_Door_Panel    SM_Graybox_Door_Locked
Surfaces/  SM_Graybox_Surface_Water SM_Graybox_Surface_Ice
Volumes/   BP_Graybox_CellPlacementVolume
```

Il percorso è quello di **`D-173`**, e la riga d'allowlist esiste: `.gitignore:201` porta
`!Content/RT/World/Graybox/**/*.uasset`, e l'oracolo lo conferma —
`git check-ignore -q Content/RT/World/Graybox/Cover/SM_Graybox_Cover_Low.uasset` esce **`1`**, cioè non
ignorato, cioè committabile. Nessun untracked in `Content/`.

### 2.2 Come sono fatte

`Source/RefactorTacticsEditor/Private/Content/RTBuildGrayboxMeshesCommandlet.cpp` — 518 righe,
`-run=RTBuildGrayboxMeshes`, con `-DryRun`, `-Package=`, `-Only=`. È **`D-229`**: la geometria vive in una
funzione che si diffa, il `.uasset` è il suo output.

| Proprietà | Misurato | Nota |
|---|---|---|
| **Scala** | letta dal CDO di `URTHexMapAsset` (`HexSize`, `LayerHeight`) | ✅ mai ricopiata: un cambio di scala si propaga rigenerando |
| **Normali** | **per faccia, scritte esplicitamente**; `bRecomputeNormals = false` | ✅ è la mitigazione che `D-229` prescriveva come primo passo |
| **Tangenti / lightmap UV** | ricalcolate / generate | — |
| **UV** | proiezione planare, `UvScale = 0.01` → **un texel ogni 100 uu** | 🟠 il numero è **hardcoded** e non compare in nessun documento — vedi §4, `A05` |
| **Material slot** | **uno, e vuoto**: `GetStaticMaterials().Add(FStaticMaterial())` | 🔴 è [#1714](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714) |
| **LOD** | **uno solo**: un `AddSourceModel()` per mesh | corretto a graybox; da dichiarare, non da correggere |
| **Collision** | il commandlet **non ne imposta nessuna** | ⚠️ non misurato in Editor: senza `AggGeom` una StaticMesh resta colpibile dai trace *complex*. Vedi §4, `A06` |
| **Pivot** | `EdgeBound`: centro del segmento, alla base — pinnato dai test | ✅ §4 del contratto |

### 2.3 Cosa li protegge davvero

Cinque test in `Source/RefactorTacticsEditor/Private/Tests/RTGrayboxMeshTests.cpp`, sotto il prefisso
`RefactorTactics.Graybox.`:

```text
CoverLowMatchesContract     spessore 0.10 · lato 0.92 · altezza 0.28 H · base a Z=0 · pivot centrato
CoverPairSeparatesInPlan    il fattore IN PIANTA è 2 — l'invariante che previene #1246
LockedDoorCarriesTraverse   D-171: Locked è una mesh diversa, non ricolorata
SurfacesSeparateByFracture  acqua piatta vs ghiaccio a sei settori
MeshesHaveFaceNormals       la garanzia su cui #1714 poggia
```

✅ **I valori attesi si derivano dal CDO**, non si scrivono in uu: un test che confrontasse `15.0` con `15.0`
passerebbe anche dopo un cambio di `HexSize`, cioè esattamente quando dovrebbe fallire.

⚠️ **`AllKitMeshes[]` (`:38`) è una lista letterale di sei path.** Non itera una cartella: un asset nuovo non
entra nel gate finché non lo si aggiunge lì. È lo stesso difetto che `CONTEXT_INDEX.md` dichiara per gli
elenchi di strumenti scritti a mano.

### 2.4 🔴 C1 — Il kit non ha consumer, e questo precede ogni altro nodo

Misurato in due direzioni indipendenti su `9434b950`:

```bash
# 1. nessun riferimento in codice, fuori dal generatore e dal test
grep -rn "SM_Graybox" Source/ Plugins/ Config/ | grep -v Commandlet | grep -v Tests   # → 0

# 2. nessun binario versionato le cita, tranne loro stesse
for f in $(git ls-files "Content/RT" | grep -E '\.(uasset|umap)$'); do
  grep -qa "SM_Graybox" "$f" && echo "$f"; done
# → solo i sei SM_Graybox_*.uasset
```

**Cockburn**: il progetto ha pagato un commandlet, sei binari, cinque test e una decisione d'autore per
produrre asset che **nessuna scena posa**. Non è un difetto di qualità — la qualità è alta — è che
l'artefatto non è ancora agganciato al suo attore. `L_DevSandbox.umap` è versionato e non li nomina.

⚠️ **Non è un gap senza owner**: [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095)
(seduta U25) li posa in `L_DevSandbox`, [#1753](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1753)
li posa nel viewport del Tactical Designer, e le sei voci `PIE-GBX-*` di
[`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) sono ⏳ *«eseguibili dal
2026-08-28 — l'asset c'è»*. **L'owner è vivo, il lavoro no.** Ed è la ragione per cui ogni nodo che parla di
cook, LOD, draw call e minimum-spec sta misurando zero.

### 2.5 🔴 C2 — Una contraddizione dentro `#1095`, che va sciolta prima di eseguirla

`#1095` chiede due cose che, prese alla lettera, si escludono:

| Criterio | Testo |
|---|---|
| **Scope 2** | *«La scena di validazione. In `L_DevSandbox`, e guardata a tre distanze di camera»* |
| **Ultimo AC** | *«`git status` sul `.umap` è pulito dopo la seduta»* |

Una scena allestita e **salvata** sporca il `.umap`; una scena allestita e non salvata non esiste al
prossimo che apre il livello, e le sei voci `PIE-GBX-*` la ripetono da capo ogni volta. L'AC nasce da
`PIE-GEO-RESIDUI` — che vuole nessun **residuo involontario** — ma scritto così vieta anche il risultato.

**Wiegers**: due criteri che non possono essere veri insieme non sono un requisito rigoroso, sono un
requisito che chi esegue scioglierà a caso. Va detto quale delle due letture vale: *«il `.umap` cambia, e il
diff contiene solo gli attori della scena di validazione»* oppure *«la scena si allestisce e non si salva»*.

### 2.6 🟠 A1 — `asset-map.md` §2.1 è stale su questa famiglia

La riga della tabella dice che il kit graybox degli oggetti è *«la famiglia decisa e senza percorso — l'unica
delle quattro a cui manchi davvero, oltre alla riga d'allowlist»*. Falso da `D-173` (2026-08-18), e **lo
stesso documento lo smentisce trenta righe più su**, dove registra il percorso, l'allowlist, l'oracolo e i
sette file tracciati. Una tabella che contraddice la prosa del proprio documento è il difetto che
`DOC_CONFLICT_MATRIX` esiste per registrare.

---

## 3. Gli asset esterni: cosa dice la misura

### 3.1 Il progetto ha già asset di terze parti, e **non ha un registro**

`spec-asset-pipeline.md:266` prescrive:

> **Requisito `FR-ASSET-LIC-01`**: ogni asset importato ha una riga in un **registro di provenienza**
> (`docs/technical/asset-licenze.md` o `DT_AssetProvenance`): nome, fonte, URL, licenza, data, note
> attribuzione. *Verifica: nessun asset in `Content/` privo di riga nel registro.*

Misurato:

```bash
ls docs/technical/asset-licenze.md          # → No such file or directory
grep -rn "DT_AssetProvenance" Source/ docs/ # → solo la riga del requisito
```

E **nessuna delle 296 issue aperte** nomina il registro: la ricerca semantica sui corpi — non sui titoli —
per `asset-licenze|AssetProvenance|FR-ASSET-LIC|registro di provenienza|Fab Standard License` dà **zero**.

**Nygard**: il requisito non è teorico. I quattro `BP_Unit_*` versionati referenziano Skeletal Mesh Paragon
che stanno sotto `Content/FabAsset/` — escluso dal repository — e
[#1663](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) misura *«756 asset Paragon
cotti»* dentro un pacchetto. Il progetto **distribuisce già** contenuto di terze parti e non ha una riga che
dica sotto quale licenza. È un debito presente, non un rischio futuro.

⛔ **`Content/FabAsset/` non esiste su questo checkout**: `ls Content/` dà
`Collections · Developers · Icons · RT · RT_UI_AssetPack_FromHUD`. Chiunque parta da qui non ha i pack.

### 3.2 🟠 A2 — I tre pack proposti non hanno un consumer, e il kit è **generato**

I candidati sono *Modular SciFi Season 1 Starter Bundle*, *Modular Industrial Catwalk Kit [Free]*,
*Factory Environment Collection*. Contro il repository:

| Uso candidato | Cosa lo blocca oggi |
|---|---|
| materiali/reference per ghiaccio e superfici | il master graybox è [#1714](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714), che chiede *«niente final art, niente texture autorate: un master parametrico e istanze»*. Un materiale donor non è un master parametrico |
| passerelle, scale, junction | **`D-229`**: le mesh del kit si **generano in C++**. Una mesh donor non entra nel kit senza contraddire la decisione che lo governa |
| libreria industriale di reference | reference visiva non richiede import: richiede una licenza verificata e un posto dove guardarla |

E le lane di `spec-asset-pipeline.md` §11-bis dicono dove sta il progetto: gli oggetti di mappa sono
**`AE0`/`AE1`** — *primitive* e *gameplay graybox*. I donor servono da **`AE2`** (art blockout) in avanti,
che nessuna release fra la v0.1 e la v0.9 dichiara.

🔑 **La regola dell'epic v1.0 [#778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/778) si
applica identica**: *«si aprono quando il percorso critico è chiuso, perché prima la loro prima riga sarebbe
"serve un consumatore che non esiste"»*.

∴ **`A01` e `A02` non diventano issue di intake.** Diventano: una issue sul **registro** (che manca davvero,
e serve comunque), più una **checklist operativa** (§8) da eseguire il giorno in cui un consumer esiste.
Nessun pack entra nel repository oggi.

---

## 4. FASE 1 — Audit A01…A11

| Area | Stato reale misurato | Owner | Issue | Gap | Azione |
|---|---|---|---|---|---|
| **#1714** materiali del kit | slot materiale vuoto in `:426`; cinque test `Graybox.*` verdi; nessun gate distingue «slot vuoto» da «slot pieno» | contratto §7 · `D-146` · `D-171` | **#1714** OPEN, `v0.1 · Leggibilità` | nessuno: il DoD è già falsificabile | **LINK** — resta il nodo v0.1 |
| **A01** intake & staging | `docs/technical/asset-licenze.md` **non esiste**; `FR-ASSET-LIC-01` orfano; zero issue lo nominano; `Content/FabAsset/` assente da questo checkout | `spec-asset-pipeline.md` §8 | — | **il registro** | **CREATE** (solo il registro) + **DEFER** (l'intake dei pack) |
| **A02** donor selection | il kit è generato (`D-229`); oggetti di mappa a `AE0`/`AE1`; nessun consumer per un donor | `spec-asset-pipeline.md` §11-bis | — | nessun gap misurabile oggi | **DEFER** con criterio d'ingresso scritto (§6) |
| **A03** transizioni multilivello | il dato esiste per intero — `ERTHexTransitionKind` {Stair·Ramp·Bridge·Tunnel·Elevator·Jump} + `ERTHexArcState` + `Integrity 40` + `StableId`; `RTHexArchTool` li authora **e li disegna** (`:297-317`, freccia PDI, `TransitionKindColor`). Ma quel disegno vive **solo mentre il tool Arch è attivo**, in PIE non esiste, separa i sei `Kind` per **sola tinta** e non mostra `State` affatto | epic **#324** (E23), CP `E23.1`/`E23.7` | — (#921 e #996 sono difetti del *tool*, non la presentazione del dato) | **il renderer fuori dal tool, prima della mesh** | **CREATE** |
| **A04** stati strutturali | macerie = §8 **#9** `DEFER`, muro sfondato = §8 **#10** `DEFER`, entrambi su `RT-FEAT-MAP-STRUCTURAL` (`IDEA`, release `future`); il producer manca ed è già tracciato | **#1132** | **#1132** OPEN, dichiara *«Assets: DEFER»* | nessuno | **DROP** + LINK a #1132 |
| **A05** material/UV contract | `#1714` copre master, istanze, tiling, roughness, tint, grayscale, «zero gameplay logic». **Non coperto**: il texel `0.01` hardcoded nel commandlet e assente da §6 del contratto | contratto §6 · #1714 | **#1714** | una riga di documento | **MERGE INTO #1714** (commento con la riga UV) |
| **A06** identity & binding | naming/path = `D-173` + §8.1; placement class = §3; pivot = §4; `StableId` esiste su `FRTHexDoor`/`FRTHexEdge` (#832 chiusa). **Non coperto**: quale dato ogni mesh rappresenta, e collision/LOD dichiarate | contratto §3-§4-§8.1 | **#1095** + contratto | una colonna nel contratto | **MERGE INTO #1095** (commento) |
| **A07** cook & content budget | il gate esiste già: **#815** (validazione contenuto, v1.0), **#816** (smoke packaged), **#803** (freeze, v0.9). L'`EditorOnly` del volume è già `PIE-GBX-VOLUME` | **#815** · **#803** | #815 · #816 · #803 OPEN | nessuno strutturale | **MERGE INTO #815** (commento che nomina il kit) |
| **A08** runtime performance | **#801** (budget su packaged, v0.8) e **#814** (certificazione, v1.0). Il kit ha **zero istanze in gioco**: misurarne le draw call oggi misura zero | **#801** · **#814** | #801 · #814 OPEN | nessuno | **DROP** — si innesta quando il kit è posato |
| **A09** minimum-spec | nessuna issue aperta nomina scalability/minimum spec (ricerca sui corpi: **0**). Ma senza materiale (#1714 aperta) non c'è complessità da degradare | — | — | reale ma **prematuro** | **DEFER** — innesto su #814/#816 |
| **A10** accessibility | **#269** (CP 25.4, v0.2) copre le **icone**, non il kit. Il kit ha il proprio canale: le sei `PIE-GBX-*` chiedono grayscale a tre distanze, e `#1714` lo porta nel DoD. Il colorblind sul kit non è di nessuno | **#269** · **#1714** · `D-146` | #269 · #1714 | il solo colorblind | **MERGE INTO #1714** + LINK #269 |
| **A11** beta content audit | **#803** (freeze: elenco + criterio + gate), **#815** (nessun asset invalido), **#816** (smoke). `tools/asset-refs/check.ts` è **verde oggi**: 104 asset, 0 riferimenti non versionati | **#803** · **#815** | OPEN | nessuno | **MERGE INTO #803/#815** |
| **v1.0** freeze del contratto | **#778 lo dichiara già**, e dichiara anche di non volere una issue: *«Non nasce una issue per questo, e non è una svista … il punto di innesto naturale è #815»*. Congela `Footprint · Pivot · Snap · Placement class · Height/reference class · presentazione dello stato · binding · identità stabile` | **#778** | #778 · #815 | nessuno | **LINK** — nessuna `A12` |

### 4.1 🟠 A3 — La ladder candidata non è la ladder del progetto

La struttura proposta mette `A05` in v0.3, `A06` in v0.4, `A07` in v0.5, `A08` in v0.6, `A09` in v0.7,
`A10` in v0.8. Le milestone reali, lette lato server:

```text
v0.2 Struttura e finestre   v0.3 Informazione        v0.4 Operations
v0.5 Online Foundation      v0.6 Ability Runtime     v0.7 Competitive Alpha
v0.8 Beta e bilanciamento   v0.9 Release Candidate   v1.0 Launch
```

Un *«Material & Surface Contract»* nella release **Informazione** — percezione, vista, memoria — e un
*«Cook & Content Budget»* dentro **Online Foundation** non hanno alcun rapporto col tema che quella release
possiede. **Fowler**: la roadmap candidata assume che le release siano stadi di maturazione dell'arte; nel
progetto sono temi di sistema, e le fasi di maturità hanno già un vocabolario proprio — le lane `AE0`→`AE5`
di §11-bis. Sovrapporre le due produce esattamente la roadmap parallela che il mandato vieta.

### 4.2 🟠 A4 — Il percorso critico non passa dai pack

`A01`/`A02` aprono la struttura candidata, cioè occupano il primo posto. Ma il kit ha un difetto misurato
(#1714), un consumer mancante (§2.4) e sei voci PIE pronte da eseguire. Tre lavori con owner vivo, tutti
davanti a un intake che nessuno consuma.

---

## 5. FASE 2 — Controllo duplicati, per semantica

Ricerca sui **corpi** delle 296 issue aperte, non sui titoli:

| Sonda | Riscontri | Conclusione |
|---|---|---|
| `asset-licenze` · `AssetProvenance` · `FR-ASSET-LIC` · «registro di provenienza» · «Fab Standard License» | **0** | gap senza owner → `CREATE` |
| transizioni/archi ↔ disegno/render/presentazione | #996, #921 (difetti del **tool** Arch), #324 (epic, senza CP-issue) | la sola presentazione è `URTHexArchTool::Render`, per-tool e d'editor: nessuna issue la possiede come **presentazione del dato** → `CREATE` |
| `SM_Graybox` · «sei mesh» · «kit graybox» | #1714, #1753 | coperto |
| `EditorOnly` ↔ packaged | #1095 | coperto (`PIE-GBX-VOLUME`) |
| «minima spec» · `scalability` · «configurazione grafica minima» | **0** | gap reale, **prematuro** → `DEFER` |
| `LOD` · `Nanite` · `HISM` · «draw call» | #1763 (animazione, non asset di mappa) | gap reale, **prematuro** → `DEFER` |

⚠️ **Nessuna PR aperta e nessun branch remoto tocca il GrayKit.** Le sei PR aperte sono su animazione,
scenari, LOS, showcase e referti; #1756 *apre* #1753 ma non ci lavora.

---

## 6. FASE 3 — Roadmap asset, corretta

```text
v0.1 · Leggibilità
  #1714  GrayKit Material Master + istanze, assegnati dal commandlet        [ESISTENTE]
  #1095  Seduta U25 — Cell Placement Volume + scena di validazione          [ESISTENTE]
         └── scioglie prima la contraddizione §2.5 (.umap pulito vs scena salvata)
  PIE    le sei voci PIE-GBX-* eseguite in seduta                           [ESISTENTE, ⏳]
  #1753  consumer: lo scenario si materializza nel viewport TD              [ESISTENTE]

Trasversale (nessuna release: è debito documentale)
  NEW-A  Registro di provenienza asset — FR-ASSET-LIC-01 non ha owner       [CREATA]

v0.1/v0.2 · da triage — epic #324 (E23)
  NEW-B  Un arco si vede solo mentre il tool Arch e' aperto                 [CREATA]
         └── il primo lavoro e' il RENDERER fuori dal tool, non la mesh
         └── §8 #11 «Rampa» resta DEFER: qui si disegna l'arco che ESISTE

v0.2 · Struttura e finestre
  #269   accessibility (icone) — LINK per il colorblind del kit             [ESISTENTE]
  #1132  detriti: nessun producer ⇒ nessun asset canonico                   [ESISTENTE, Assets: DEFER]

v0.8 · Beta e bilanciamento
  #801   budget di performance su packaged — il kit ci entra se è posato    [ESISTENTE]

v0.9 · Release Candidate
  #803   freeze del contenuto: elenco, criterio, gate                       [ESISTENTE]

v1.0 · Launch
  #815   validazione del contenuto ⟵ innesto del FREEZE DEL CONTRATTO       [ESISTENTE]
  #816   matrice di smoke su packaged                                       [ESISTENTE]
  #814   certificazione di performance sulla build di rilascio              [ESISTENTE]
  #778   epic: dichiara il freeze e dichiara di NON volere una issue        [ESISTENTE]
```

### 6.1 Dipendenze e percorso critico

```text
#1714 -> #1095 -> PIE-GBX-* -> #1753
#1714 -> #815   (un asset senza materiale è un asset che il validator non sa giudicare)
NEW-A -> (qualunque intake di pack esterni)
NEW-B -> #324/E23.1  ·  NEW-B -> §8 #11 (la rampa si modella DOPO che l'arco si vede)
#1095 -> #803 -> #815 -> #778
```

**Percorso critico, sette passi:**

1. **#1714** — il materiale: senza, `#1095` giudica la forma e metà di ciò che deve misurare resta fuori.
2. **#1095** — sciogliere §2.5, poi la seduta U25: il volume e la scena.
3. **le sei `PIE-GBX-*`** — grayscale a tre distanze; `GBX-1` (l'inset) si decide **guardando**.
4. **#1753** — il primo consumer che non sia una seduta: lo scenario si materializza.
5. **#803** — il kit entra nell'elenco del freeze.
6. **#815** — il freeze del contratto si innesta qui, come `#778` prescrive.
7. **#816/#814** — smoke e certificazione sulla build che si spedisce.

⛔ **Fuori dal percorso critico**: `A02` donor selection, `A08` performance del kit, `A09` minimum-spec.
Rientrano quando il kit è **posato in una scena versionata**, non prima.

### 6.2 Criteri d'ingresso per ciò che è differito

Perché un `DEFER` non diventi un dimenticato, ciascuno porta la condizione che lo riapre:

| Differito | Si riapre quando |
|---|---|
| intake dei tre pack Fab | esiste un consumer **nominato** per un asset specifico, e **NEW-A** è chiusa |
| `A02` donor selection | un elemento del catalogo §8 raggiunge `AE2` (art blockout) in una release dichiarata |
| `A09` minimum-spec | `#1714` è chiusa e il kit ha un materiale di cui misurare la complessità |
| `A08` performance del kit | un `.umap` versionato posa le mesh del kit, cioè `#1095` è chiusa |
| §8 #9/#10 macerie e muro sfondato | `RT-FEAT-MAP-STRUCTURAL` esce da `IDEA` — oggi tracciato da **#1132** |
| §8 #11 rampa | **NEW-B** è chiusa: prima si vede l'arco, poi si modella la rampa |

---

## 7. Le due issue create, e le quattro aggiornate

Vedi §9 per numeri e titoli. Le esistenti sono state aggiornate **con un commento**, non riscrivendone il
corpo: un corpo d'autore riscritto da fuori perde la provenienza, e questo referto non è autorità.

---

## 8. FASE 6 — Checklist di acquisizione dei pack (non eseguita)

⛔ **Nessun pack entra nel repository oggi**, e questa checklist non autorizza a farlo: descrive il giorno in
cui un consumer esiste. «Gratis» non significa «necessario».

```text
PRECONDIZIONE
[ ] NEW-A chiusa: il registro di provenienza esiste e ha uno schema
[ ] esiste un consumer NOMINATO — issue, asset di destinazione, release

PER OGNI PACK
[ ] aggiunto alla Fab Library dell'account
[ ] registrato: nome · publisher · URL Fab · licenza applicabile · data · versione ·
    versione UE supportata · dimensione · categorie
[ ] licenza VERIFICATA sulla pagina del pack — non dedotta dal fatto che sia gratuito.
    Se la redistribuzione non è chiara: STOP sul riuso, reference visiva solo se consentita
[ ] scaricato in locale, FUORI dal repository (Content/FabAsset/ è ignorato per scelta)
[ ] aperto in un progetto MAGAZZINO separato — convenzioni-contenuti-ue.md §B.2a:
    Fab non lascia scegliere la destinazione, quindi si rinomina LÌ e si migra da lì
[ ] inventario di ciò che serve: classificare REUSE DIRECT | ADAPT | REFERENCE ONLY | REJECT
    valutando scala, pivot, silhouette, polycount, LOD, UV, materiali, collision, snap,
    adattabilità all'esagono (C = √3 · 150 uu), costo cooked, licenza, dipendenze
[ ] REJECT esplicito per tutto il resto: il default è NON importare
[ ] migrato SOLO il selezionato, con il path virtuale già corretto nel magazzino
[ ] rinominato e collocato secondo convenzioni-contenuti-ue.md §5/§5b
[ ] riga in .gitignore PRIMA del git add — senza, git add tace (asset-map.md §6)
[ ] git check-ignore -q <file> esce 1
[ ] riga nel registro di provenienza
[ ] tools/asset-refs/check.ts verde
[ ] Fix Up Redirectors prima del commit
[ ] il cook non cresce di ciò che non si usa
```

⚠️ **`git clean -fdx` cancella i pack**, e l'esclusione a una cartella sola non basta:
`git clean -fdx -e Content/FabAsset -e 'Content/Paragon*'`.

---

## 9. Esito

### 9.1 Issue create

| # | Titolo | Release |
|---|---|---|
| **#1767** | Il registro di provenienza degli asset non esiste: `FR-ASSET-LIC-01` prescrive un file che non c'è, e il progetto distribuisce già contenuto di terze parti | nessuna — debito documentale |
| **#1768** | Un arco di transizione si vede solo mentre il tool Arch è aperto: sei `Kind` distinti dalla sola tinta, lo `State` da niente, e in PIE non c'è nulla | da triage — epic #324 |

### 9.2 Issue riusate

`#1714` · `#1095` · `#1753` · `#1132` · `#269` · `#801` · `#803` · `#814` · `#815` · `#816` · `#778` · `#324`

### 9.3 Proposte NON create, e perché

| Proposta | Perché no |
|---|---|
| `A01` intake dei tre pack | nessun consumer; `D-229` genera il kit; la disciplina di #778 lo vieta esplicitamente |
| `A02` donor selection | stesso motivo, più le lane `AE0`/`AE1`: i donor servono da `AE2` |
| `A04` structural state | §8 #9/#10 sono `DEFER` su feature `IDEA`; **#1132** già traccia il producer mancante |
| `A05` material/UV contract | duplicherebbe il DoD di **#1714** |
| `A06` identity & binding | il contratto §3/§4/§8.1 già possiede naming, path, placement class, pivot |
| `A07` cook & budget | **#815** · **#816** · **#803** lo sono già |
| `A08` runtime performance | **#801** · **#814**; e oggi misurerebbe zero istanze |
| `A09` minimum-spec | gap reale ma prematuro: senza materiale non c'è complessità da degradare |
| `A10` accessibility pass | grayscale è già nel DoD di **#1714** e nelle `PIE-GBX-*`; il colorblind è **#269** |
| `A11` beta content audit | **#803** + **#815**; `asset-refs` è già verde |
| `A12` freeze v1.0 | **#778** dichiara di non volerla, e nomina **#815** come innesto |

---

## 10. Limiti dichiarati di questo referto

- **Nessuna suite eseguita, nessuna build.** I cinque test `RefactorTactics.Graybox.*` sono stati **letti**,
  non fatti girare: dire che sono verdi sarebbe un'affermazione che questo referto non ha misurato.
- **La collision delle sei mesh non è misurata.** Il commandlet non ne imposta; che cosa risponda un trace
  *complex* su una `UStaticMesh` senza `AggGeom` si verifica in Editor, e nessuno l'ha fatto.
- **Le licenze dei tre pack non sono state lette.** Nessun dato di licenza è stato inventato: la checklist
  §8 le fa verificare sulla pagina del pack, non le riporta.
- **`#1095` non è stata riscritta.** La contraddizione di §2.5 è segnalata in un commento; scioglierla è una
  decisione d'autore.
- Questo documento **non è autorità**: gli owner restano
  [`spec-graybox-placement-contract.md`](../../technical/systems/spec-graybox-placement-contract.md),
  [`spec-asset-pipeline.md`](../../technical/architecture/spec-asset-pipeline.md),
  [`asset-map.md`](../../technical/tooling/asset-map.md) e il
  [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
