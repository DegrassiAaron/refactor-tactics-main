# Kallari

> 🧪 **Stato repository:** `DESIGN_SPEC` tradotta dal «Master Game Design Document» del 2026-11-08 ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3)), per decisione d'autore [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2). **Nulla di questa pagina è a runtime**: il kit in vigore per questo slot è ancora quello di Ivrin (`Hero.Ivrin.*`), che si ritira con il rename di [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011). I numeri sono **input**, marcati tali (costi AP: `MSP-1.n`; HP, DEF, RES: E53 [#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)). Issue: [#4014](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4014).

> **Asset base:** Paragon — Kallari ([scheda candidata](../candidates/kallari.md))
> **Hero_Key:** `ASSET_KALLARI`
> **RT Character ID:** `Hero.Kallari` (dopo #4011; oggi `Hero.Ivrin`)
> **Release:** `v0.1` (prende il posto di Wraith / Ivrin, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (1))
> **Fazione:** Constrine (ereditata dallo slot sostituito, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (3))
> **Ruolo:** Infiltrator (Signature *Hunt / Trace*, *Silent movement*)
> **Provenienza visuale:** mesh e animazioni dallo slot Paragon **Kallari**; l'asset **non è nel checkout**: l'import è lavoro di #4011.

## Panoramica

Flanker che si avvicina senza farsi vedere, salta alle spalle e colpisce in mischia; il kit del GDD la costruisce sul Flanking a spicchi, sull'asse Z e sullo stealth termico. Nel contratto il colpo alle spalle è il **bonus per personaggio dell'attaccante** di [D-587](../../decisions/RT_PDR_00_Decision_Log.md): Kallari è la prima candidata a un valore alto (`MSP-2.n`), e nessuna sua abilità ha bisogno di una regola di Flanking propria.

## Come leggere la traduzione

| Nel GDD | Nel contratto, in questa pagina |
|---|---|
| Flanking «4 spicchi posteriori, +50%, ignora metà DEF» | bonus alle spalle **per personaggio dell'attaccante**, tre lati posteriori di D-126, sommato alla Base ([D-587](../../decisions/RT_PDR_00_Decision_Log.md)); il retro toglie copertura e Guard (ADR-0005 §4a) |
| «ignora Overwatch», «Stealth» | la percezione è E13 (vista a cono, consapevolezza ravvicinata); lo stealth è una **baseline** delle schede (Stealth 2), non uno stato a runtime: candidato |
| «danni critici», «+30% al primo colpo» | niente critici (0% RNG): un moltiplicatore di stato in `Mod` ([D-589](../../decisions/RT_PDR_00_Decision_Log.md) (3)), valore al catalogo |
| «ignora il 100% della DEF» | DEF trattata come 0 per quel colpo nella formula di D-589: esprimibile, valore e condizione al catalogo |
| «trapassa tutti i modelli sulla linea» | il transito fra alleati è ammesso, i nemici bloccano (D-302): trapassare i nemici è una regola **nuova**, scartata qui |
| «Freddo», «Aria», «Acqua», «Disintegrazione» | `Element.Frost`, `Element.Air`, `Element.Water`, `Element.Disintegration` ([D-593](../../decisions/RT_PDR_00_Decision_Log.md)) |
| «AP / CD» | costo AP del pool di D-584, **non deciso**; cooldown asse proprio |

## Identità tattica (input GDD §7)

- **Sinergie dichiarate**: Feng Mao e Steel; congela una cella da sola con Shadow Walk (`Frost`) + Mist Slash (`Water`): coppia `Element.Frost`+`Element.Water`, oggi `OPEN` nel [catalogo](../../balance/RT_ElementPairCatalog_v0.1.md).
- **Forte contro** (GDD): Murdock e Gadget. **Debole contro**: Riktor (aure e stordimenti ad area sui suoi pochi HP).

## Statistiche base (input GDD §5.1)

| Statistica | Valore GDD | Stato |
|---|---|---|
| HP | 950 | input per E53 |
| DEF | 15 (~13%) | input |
| RES | 20 (~17%) | input; nel contratto è per elemento |
| Move | 4 hex | input |
| Gittata base | 1 hex (mischia) | input |
| Affinità | `Element.Frost` · `Element.Air` · `Element.Water` · `Element.Disintegration` | input per #995 |

## Profilo di attacco base

### Mist Slash — `Hero.Kallari.MistSlash`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Water` |
| Costo AP / CD (GDD) | 2 / 0 · costo **non in vigore** |
| Portata | 1 (mischia) |
| Danno (GDD) | 210 fisico |
| Effetto | applica `Element.Water` sulla cella del bersaglio; riduce la vista del bersaglio |

**Traduzione**: *«riduce di 3 esagoni la LoS nemica»* si legge come uno stato di vista ridotta sul bersaglio (vicino a `Status.Obscured`, «attraverso il fumo si mira solo da vicino»), con il valore al catalogo e l'owner in E13. **Scartato**: nulla.

## Abilità

### Shadow Walk — `Hero.Kallari.ShadowWalk`

| Campo | Valore (input) |
|---|---|
| Fase | Prep |
| Elemento | `Element.Frost` |
| Costo AP / CD (GDD) | «1–2» / 2 · costo **non in vigore** |
| Effetto | entra in stealth per due turni; il primo colpo dallo stealth ha un moltiplicatore |

**Traduzione**: lo stealth come stato è **candidato** (E13 ha le baseline di Stealth e Detection, nessuno stato a runtime); il «+30% al primo colpo» è un moltiplicatore in `Mod`. **Scartato**: *«ignora Overwatch, non attiva trappole»* come regola assoluta: se lo stealth esiste, l'Overwatch non vede chi non percepisce (ADR-0004: il cono dell'Overwatch è il facing e la vista) e non serve un'esenzione scritta; il costo «1–2» variabile.

### Double Jump — `Hero.Kallari.DoubleJump`

| Campo | Valore (input) |
|---|---|
| Fase | Dash (mobilità speciale) |
| Elemento | `Element.Air` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Distanza (GDD) | 2 hex, scavalca muri e dislivelli |
| Effetto | salto che ignora gli ostacoli sul percorso |

**Traduzione**: famiglia di `Leap` (esiste, Dash, ignora il percorso); i dislivelli sono i `Layer` di `FRTCellId` con gli archi espliciti. **Scartato**: *«atterrando alle spalle del bersaglio per il Flanking»*: la destinazione la sceglie il giocatore, il bonus alle spalle è D-587, nessuna regola speciale.

### Death Toll — `Hero.Kallari.DeathToll`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Disintegration` |
| Costo AP / CD (GDD) | 3 / 2 · costo **non in vigore** |
| Portata | 1 |
| Danno (GDD) | 450 perforante |
| Effetto | contro un bersaglio con DEF 0 la DEF è ignorata del tutto |

**Traduzione**: «perforante» = la DEF trattata come 0 nella formula di D-589 per questo colpo. **Scartato**: *«triplica il danno […] in Flanking»*: il colpo alle spalle vale il bonus per personaggio di D-587, non un moltiplicatore per abilità; se Kallari deve far male alle spalle, il suo valore in `MSP-2.n` è alto.

### Phantom Strike — `Hero.Kallari.PhantomStrike`

| Campo | Valore (input) |
|---|---|
| Fase | Dash (mobilità speciale) |
| Elemento | `Element.Frost` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Distanza (GDD) | 5 hex in linea |
| Effetto | scatto rettilineo che applica `Element.Frost` e danno alle unità attraversate |

**Traduzione**: famiglia di `Charge` (mobilità con colpo) in linea. **Scartato**: *«trapassa tutti i modelli sulla linea»*: i nemici bloccano il movimento (D-302, contesa deterministica); una mobilità che attraversa i nemici è una regola nuova da decidere a parte, e *«danni critici»*: nessun critico nel contratto.

## Reazioni (lista, D-592)

### Acrobatic Evasion — `Hero.Kallari.AcrobaticEvasion`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un'area nel Blast |
| Elemento | `Element.Air` |
| Costo AP / CD (GDD) | 2 / 2 · costo **non in vigore** |
| Effetto | si sposta in una cella libera adiacente prima del danno |

**Traduzione**: è `Action.Evade` (esiste: «si sposta di una cella») con trigger ristretto alle aree. **Scartato**: nulla.

### Vengeful Shade — `Hero.Kallari.VengefulShade`

| Campo | Valore (input) |
|---|---|
| Trigger | un alleato che Kallari vede subisce danno |
| Elemento | `Element.Disintegration` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | colpo a distanza contro l'attaccante |

**Traduzione**: famiglia di `Counter` con trigger sull'alleato visibile (la vista è quella di E13, direzionale). **Scartato**: nulla.

## Che cosa si ritira con il rename (#4011)

Il kit di **Ivrin** (`Hero.Ivrin.PulseShot`, `InterceptShot`, `PassingBlade`, `Feint` e le altre voci a catalogo) si **ritira** con [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2); la lista nominale dei golden e degli scenari che lo portano è nella [scheda di Wraith](wraith.md), sezione «Ritiro del kit».

## Stato produzione

| Campo | Valore |
|---|---|
| Page Status | DESIGN_SPEC (input tradotto) |
| Stats | INPUT (GDD §5.1) |
| Abilities | TRANSLATED (4 + attacco base) |
| Reactions | TRANSLATED (2) |
| Numeri in vigore | **nessuno** |
| Release | v0.1 (dopo #4011 e `MSP-1.n`) |

## Governance

- **Fonte**: [`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) §5.1 e §7 (Kallari); decisioni [D-585](../../decisions/RT_PDR_00_Decision_Log.md), [D-586](../../decisions/RT_PDR_00_Decision_Log.md), [D-587](../../decisions/RT_PDR_00_Decision_Log.md), [D-591](../../decisions/RT_PDR_00_Decision_Log.md), [D-593](../../decisions/RT_PDR_00_Decision_Log.md).
- Un kit **copiato** dal GDD è un `IMPLEMENTATION DRIFT` (#4014); questa pagina è la traduzione, e dice che cosa ha scartato.
