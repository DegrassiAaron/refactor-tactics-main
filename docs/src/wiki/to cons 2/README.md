# `RefactorTactics_Facing_Flows_v0.1` — sette diagrammi del Facing

> `CURRENT` · **Importati**: 2026-08-13 · **Sorgente**: `RefactorTactics_Facing_VisualDocs_Claude_Bundle_2026-08-13`
> (archiviato in [`../../../archive/src/`](../../../archive/src/README.md)) · **Decisione**: [D-076](../../../decisions/RT_PDR_00_Decision_Log.md) — qui stanno **asset**, non pagine.

Le pagine che li usano vivono nel clone `refactor-tactics-main.wiki`. Qui c'è la copia di riferimento, con
l'unica cosa che il pacchetto sorgente non diceva: **quale diagramma c'è davvero dentro ogni file**.

## 🔴 Due file arrivavano con il nome scambiato

Il pacchetto consegnava `F4_Overwatch_Reaction_Facing.png` contenente il diagramma **F6 – Privacy**, e
`F6_Privacy_Team_only_UI_del_Facing.png` contenente il diagramma **F4 – Overwatch**. I sette hash SHA-256
corrispondevano **tutti** al manifest §27 del pacchetto, e anche i byte: l'integrità era perfetta, il
contenuto no.

⚠️ **Nessun controllo automatico poteva accorgersene.** L'audit prescritto era *«confronta gli hash contro
gli asset già presenti»*, e quel confronto dà esito verde: gli hash sono giusti, sono i **nomi** a mentire.
Solo aprire le immagini lo rivela — ed è la ragione per cui l'audit visivo del pacchetto non è una formalità.

**Qui i file sono stati scambiati**, così che il nome descriva il contenuto. Di conseguenza la colonna hash
qui sotto **non coincide** con quella del manifest sorgente per F4 e F6: è la stessa coppia, invertita.

| File (nome corretto) | Contenuto reale | SHA-256 | Nel manifest sorgente era |
|---|---|---|---|
| `F1_Lifecycle_del_Facing_nel_Turno.png` | F1 — lifecycle nel turno | `5f9ad7b5…e81f1` | F1 ✅ |
| `F2_Move_Micro_step_e_Pivot.png` | F2 — move, micro-step, pivot | `1a794a15…7a37` | F2 ✅ |
| `F3_Attacco_e_Direzione_Relativa.png` | F3 — sei direzioni relative | `679e9531…6287f` | F3 ✅ |
| `F4_Overwatch_Reaction_Facing.png` | **F4 — Overwatch / reaction** | `20184b83…5a0fb` | ⚠️ era il file «F6» |
| `F5_Forced_Movement_Facing_Control.png` | F5 — forced movement | `0afda3be…53ee9` | F5 ✅ |
| `F6_Privacy_Team_only_UI_del_Facing.png` | **F6 — privacy / team-only** | `1eb43243…4d957e` | ⚠️ era il file «F4» |
| `F7_Scenario_Coverage_Map_Facing.png` | F7 — coverage scenari | `c108476c…62dc08` | F7 ✅ |

## Livello di autorità — non è uniforme

Le immagini sono **artefatti di design**, non fonti normative. Il canone resta negli ADR e nel Decision Log.

| | Autorità | Cosa verificare prima di citarla |
|---|---|---|
| **F1** | ✅ allineata | Il costo del pivot **non compare**: il pacchetto avvisava che potesse presentarlo come prezzo in MP (`FAC-12`, aperta), e la verifica del 2026-08-13 dice di no — mostra il pivot finale come **tetto**, cioè ADR-0008 §1 |
| **F2** | ✅ allineata | Idem: nessun costo MP. È la tavola migliore per «passo bloccato ⇒ facing invariato» e per il Decision Boundary |
| **F3** | ✅ **canonica** sulla rosa a sei lati ([D-126](../../../decisions/RT_PDR_00_Decision_Log.md)) | 🔴 **ma il pannello 1 no**: `FromSource · FromTrajectory · FromImpactCenter · ExplicitDirection · NonDirectional` è la policy della direzione d'impatto, cioè **`FAC-13`, aperta**. Il diagramma la presenta con la stessa grafica della parte decisa. Citando F3, dire quale metà è canone. Gli esempi `Shield Front Arc = {5,0,1}` e `Backstab = {3}` sono marcati «illustrativi» nell'immagine stessa: non sono regole |
| **F4** | ✅ allineata | `#164` (CP 14.4) è **chiusa**; il lavoro vivo è [#165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/165). Il diagramma dice correttamente che detection e LOS sono controlli **separati** dal facing e che non esiste snap a 180° |
| **F5** | ⚠️ **PROPOSED** | `RotateSteps ±N`, `SetDirection`, `FaceAwayFromSource` **non esistono** in `ERTActionEffect`: è `FAC-14`, aperta. Usabile solo come direzione di design, mai come catalogo |
| **F6** | ✅ allineata | Server authoritative, preview team sanitizzata, nessun planned facing avversario, last-known su perdita di LOS. Coerente con ADR-0005 §5 |
| **F7** | 🔴 **snapshot datata 2026-08-13**, non una dashboard | Conteneva già almeno due righe superate quando è stata importata: `#164` vi compare `PLANNED` ma è **chiusa**, e `Spec.Facing.TurningPathUsesLastCompletedStep` vi compare `MISSING` ma **esiste da oggi**. La legenda `GREEN/PLANNED/PARTIAL/MISSING/BLOCKED` è **visuale** e non è il vocabolario del registry: lo stato vivo si legge da [`feature-registry.yaml`](../../../roadmap/feature-registry.yaml) e da [`scenariomap.shortlist.md`](../../../roadmap/scenariomap.shortlist.md), che sono generati |

## Nel clone Wiki

Copiati con la convenzione di numerazione del clone (`NN_nome-kebab.png`), non con questi nomi:

| Qui | Nel clone |
|---|---|
| F1 | `images/wiki/gameplay/17_facing-lifecycle-turno.png` |
| F2 | `images/wiki/gameplay/18_facing-move-microstep-pivot.png` |
| F3 | `images/wiki/gameplay/19_facing-direzione-relativa.png` |
| F4 | `images/wiki/gameplay/20_facing-overwatch-reaction.png` |
| F5 | `images/wiki/gameplay/21_facing-forced-movement-control.png` |
| F6 | `images/wiki/gameplay/22_facing-privacy-team-only.png` |
| F7 | `images/wiki/technical/23_facing-scenario-coverage-snapshot.png` |

Zero duplicati: i sette hash sono stati confrontati contro i **19** PNG già presenti in `docs/wiki/`, e
l'intersezione è **vuota**. Come avverte [`../../../wiki/README.md`](../../../wiki/README.md), il confronto si fa per **hash** e mai
per nome — e questa cartella è la prova che il nome può sbagliare anche quando l'hash è giusto.
