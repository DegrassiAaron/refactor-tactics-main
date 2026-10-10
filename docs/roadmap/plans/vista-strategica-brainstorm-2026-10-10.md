# La vista strategica: dal brainstorming del 2026-10-10 ai quattro pezzi della v0.1

> `SNAPSHOT` · fotografia del 2026-10-10: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-10.
> - **Misurato su** `origin/main` = `12d26ba3`.
> - **Oggetto**: implementare la vista strategica dall'alto di [D-488](../../decisions/RT_PDR_00_Decision_Log.md), il seguito T9 del [referto delle tavole A–F](hud-tavole-a-f-spec-panel-2026-10-09.md).
> - **Issue**: [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774) (nucleo) · [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) (CAM-05a) · [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) (CAM-05b) · [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632) (CAM-06a), tutte in **E49** ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)).
> - **Decisione registrata**: [D-495](../../decisions/RT_PDR_00_Decision_Log.md).
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo brief.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Il verdetto in una riga

La scena delle tavole A–F **c'è già quasi tutta**:
- la board è fatta di prismi esagonali;
- le unità hanno già un segnalino, nascosto dallo skeletal dell'eroe.

La vista strategica quindi è **la stessa scena con un altro aspetto**, non una seconda scena. Cambiano le unità e la camera, la mappa no. Così la privacy si **eredita dal velo** invece di essere riscritta.

## 2. Che cosa era già deciso

- [D-252](../../decisions/RT_PDR_00_Decision_Log.md): la vista strategica è una conseguenza dello zoom, con isteresi. Non è una terza modalità della camera.
- [D-488](../../decisions/RT_PDR_00_Decision_Log.md) decide cinque cose:
  1. `Tab` porta lo zoom alla soglia e ritorno;
  2. al centro c'è l'isometrica semplificata, e l'HUD resta lo stesso;
  3. il ciclo della selezione passa a `N`, nello stesso commit di `Tab`;
  4. è sola presentazione;
  5. proiezione e soglie si tarano in `L_CameraFeatureLab`.

## 3. Che cosa esiste

Misurato su `12d26ba3`.

| Cosa | Stato | Dove |
|---|---|---|
| Stato strategico con isteresi | ✅ esiste, con i test `RefactorTactics.Camera.StrategicViewHasHysteresisAndDoesNotOscillate` e `…StrategicThresholdsAreOrderedInCodeNotOnlyInDocs` | `ARTCameraPawn::IsStrategicView`, `UpdateStrategicState` |
| Soglie | 🟡 provvisorie, `2400` per entrare e `1900` per uscire | `RTCameraPawn.h` |
| Consumatori dello stato | ❌ nessuno. Al cambio di stato parte solo un `UE_LOG`. Nessun asset lo legge: `grep -rlaF StrategicView --include=*.uasset --include=*.umap Content` sul clone principale non trova nessun file | `RTCameraPawn.cpp`, `UpdateStrategicState` |
| `Tab` | occupato dal ciclo della selezione | `RTPlayerController.cpp`, `CycleSelectionAction` |
| `N` | libero | `BuildInputMappings` |
| `M` | è Sneak | `SneakHotkey()` |
| Board | prismi esagonali ISM, una istanza per cella | `ARTHexMapActor` |
| Segnalino delle unità | cilindro, anello di squadra ed etichetta nome·PV; li nasconde lo skeletal dell'eroe | `ARTUnit::Mesh`, `TeamRing`, `OverlayWidget` |
| Predicati di visibilità | puri; la scelta del segnalino viene **dopo** `ShouldBeRendered`, cioè dopo il velo | `ShouldShowPlaceholderMesh`, `ShouldShowHeroSkeletal` |
| Overlay di pianificazione | già nel mondo: line batcher, `PlanGhosts`, canvas dell'HUD | `DrawPlanningPreview`, `ARTHUD::DrawHUD` |
| Proiezione | sempre prospettica; nessun uso dell'ortografica in `Source/` (`git grep -n -i Orthographic -- Source` non trova niente) | `ARTCameraPawn` |

## 4. Le decisioni d'autore di questa sessione

Registrate come [D-495](../../decisions/RT_PDR_00_Decision_Log.md).

1. **La vista strategica entra in v0.1.** [D-252](../../decisions/RT_PDR_00_Decision_Log.md) la collocava dopo, e l'epic E49 teneva le figlie post-v0.1 finché un playtest non dimostrasse il contrario. Ora serve in pianificazione. Entra **in quattro pezzi** (§6).
2. **Stessa scena, altro aspetto.** Le unità mostrano il segnalino al posto dello skeletal. Non si costruisce una seconda board, che sarebbe un secondo consumatore del velo da tenere allineato.
3. **Taglio netto.** Lo scambio avviene nel frame in cui lo stato cambia, senza dissolvenza: l'isteresi impedisce già lo sfarfallio, e una dissolvenza sarebbe uno stato in più.
4. **La taratura resta fuori.** Soglie e proiezione restano a [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) (`L_CameraFeatureLab`, post-v0.1). In v0.1 le soglie sono quelle provvisorie, dichiarate come tali.

## 5. Tre correzioni, trovate con una seconda fonte

La proposta fatta all'autore in sessione aveva tre errori. Li ha trovati la lettura del codice, non una rilettura della proposta.

### 5.1 La board non si semplifica

La proposta nascondeva `Relief`, `SurfaceVolumes` e `StructuralBodies`. Ma le famiglie ISM della board sono **canali di lettura** (commenti in `RTHexMapActor.h`):

- `Relief` è il costo di movimento;
- `SurfaceVolumes` è il fumo, che limita la portata della vista;
- `Blockers` sono le due regole di blocco;
- `EdgeFeatures` sono coperture e porte;
- `SurfaceGlyphs` e `CellBorders` dicono la superficie e dove finisce una cella.

Nasconderle toglierebbe informazione proprio mentre si pianifica. Solo `StructuralBodies` è riempimento visivo («geometria che riempie un vuoto visivo»), e non vale la pena di trattarlo a parte.

### 5.2 «Contatto incerto» e «Rumore» della tavola F non si possono mostrare

Il modello di conoscenza ha due soli livelli, `Live` e `Remembered` (`ERTKnowledgeVisibility`, `RTKnowledgeView.h`). La propagazione del rumore esiste (`RTAcousticPropagationLibrary`), ma fuori da `Perception/` nessuno la usa: `git grep -lE "FRTNoiseReception|ERTNoiseType" -- Source` trova solo file di `Perception/` e di test.

Disegnarli vorrebbe dire **inventare informazione**, e D-488 (4) lo vieta. Il nucleo mostra quindi:
- `Live` come segnalino;
- `Remembered` come segnalino con «×», cioè l'ultimo contatto.

Incerto e rumore arrivano quando il canale acustico arriva al giocatore.

### 5.3 La separazione dei piani non passa dal contesto della mappa

`ARTHexMapActor::GetHexContext` lo legge anche la simulazione: `RTTurnManager_Movement.cpp`, `RTTurnManager_Blast.cpp`, `RTMovementResolutionContext.h` (`git grep -lF GetHexContext -- Source`). Scalarne `LayerHeight` in Strategic cambierebbe la risoluzione.

Serve una quota **solo di presentazione**, e il **picking** deve usarla: da [D-255](../../decisions/RT_PDR_00_Decision_Log.md) la cella cliccata decide che cosa si pianifica.

### Un difetto preesistente, trovato strada facendo

`AddOrbit` scrive `CameraPitch` direttamente. `AddZoom`, `ZoomTowards` e `SetZoomAlpha` chiamano `RecomputePitchFromZoom`, che lo riscrive come «derivato + offset». Quindi l'inclinazione data con l'orbita si perde alla prima rotellata.

È letto nel codice e non provato a runtime. Lo corregge [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630), perché una rampa d'inclinazione lo renderebbe più visibile.

## 6. I quattro pezzi

| Pezzo | Issue | Che cosa | Dipende da |
|---|---|---|---|
| **Nucleo** | [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774), col tasto di [#3145](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3145) | evento di cambio stato sul pawn · `Tab` e `N` nello stesso commit · segnalino al posto dello skeletal · `Remembered` con «×» · test del velo · `spec-pointer-interaction.md` §6.6 e `spec-tactical-camera.md` §5 aggiornati | — |
| **Inclinazione** | [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) (CAM-05a) | rampa del pitch solo oltre `StrategicExitThreshold`, continua con la distanza, neutra di default · fix di `AddOrbit` | nucleo |
| **Prova ortografica** | [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) (CAM-05b) | interruttore spento di default · `OrthoWidth` derivata dal braccio · limiti del pivot e picking corretti in ortografica · la scelta resta dell'autore | nucleo |
| **Separazione dei piani** | [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632) (CAM-06a), scorporata da [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775) | quota di presentazione per piano, usata da board, unità, overlay, HUD e picking · nessuna `FRTCellId` e nessun esito cambia | nucleo |

**Ordine**: prima il nucleo, poi inclinazione e prova ortografica in parallelo, infine la separazione. La separazione va per ultima perché tocca il picking, e conviene farla quando la proiezione è già nota.

## 7. Punti aperti, da chiudere nella PR che li incontra

- **Che cosa fa `Tab` se nella vista strategica si è entrati con la rotella.** D-488 dice «riporta alla distanza da cui era partita», ma la rotella non lascia una distanza di partenza.
  - **Proposta**: il pawn ricorda l'ultima distanza tattica a ogni ingresso in Strategic, qualunque sia la via, e `Tab` ci ritorna.
  - Da confermare con l'autore nella PR del nucleo.
- **La forma della rampa d'inclinazione.** Fino a `StrategicEnterThreshold` oppure fino a `MaxArmLength`, e con quale valore: si sceglie in PIE (#3630).
- **La proiezione.** La sceglie l'autore guardando le due versioni (#3631, D-488 (5)).
- **Il valore della separazione dei piani.** Si sceglie in PIE (#3632).

## 8. Fuori da questo lavoro

| Che cosa | Classe | Dove resta |
|---|---|---|
| Contatto incerto e rumore nella vista strategica | `DEFERRED` | il canale acustico (CP 13.3/13.4), quando arriva al giocatore |
| Taratura di soglie e proiezione in `L_CameraFeatureLab` | `DEFERRED` | [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780), post-v0.1 |
| Indicatori sopra/sotto in Tactical | `DEFERRED` | [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775), post-v0.1 |
| Il resto di [#3145](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3145) (test sull'ordine, unità in lock-in o KO, dichiarazioni incomplete) | invariato | #3145 |

## 9. Gate di questo passaggio

- `node tools/radar/decision-ids.ts --check`
- `node tools/radar/doc-links.ts --check`
- `node tools/radar/doc-tables.ts --check`
- `node tools/radar/doc-coherence.ts --check`

I verdetti stanno nella PR, non qui. Compile, Automation e PIE: `N/A`, perché nessun file di `Source/` o di `Content/` cambia.
