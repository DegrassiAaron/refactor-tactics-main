# Muriel

> 🧪 **Stato repository:** `DESIGN_SPEC` tradotta dal «Master Game Design Document» del 2026-11-08 ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3)), per decisione d'autore [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2): *«i quattro entranti ricevono un kit nuovo progettato a partire da […] della specifica, tradotto nelle regole del contratto»*. **Nulla di questa pagina è a runtime**: il kit in vigore nel codice per questo slot è ancora quello di Muiren (`Hero.Muiren.*`), che si ritira con il rename di [#4011](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4011). I numeri sono **input**, marcati tali; nessuno è in vigore prima della sua `D-nnn` (costi AP: `MSP-1.n`; HP, DEF, RES: epic E53 [#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)). Issue: [#4014](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4014).

> **Asset base:** Paragon — Muriel ([scheda candidata](../candidates/muriel.md))
> **Hero_Key:** `ASSET_MURIEL`
> **RT Character ID:** `Hero.Muriel` (dopo #4011; oggi `Hero.Muiren`)
> **Release:** `v0.1` (prende il posto di Phase / Muiren, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (1))
> **Fazione:** Conflux (ereditata dallo slot sostituito, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (3))
> **Ruolo:** Support (Macro ruolo Signature: Support, Signature *Guardian Link / Remote protection*)
> **Provenienza visuale:** mesh e animazioni dallo slot Paragon **Muriel**; l'asset **non è nel checkout** (`Content/FabAsset/Paragon/` non esiste, misurato in #2297): l'import è lavoro di #4011, non di questa pagina.

## Panoramica

Supporto predittivo che legge in anticipo il Blast avversario: scudi di area e su singolo alleato, un'uscita di emergenza verso un compagno, una sfera di gelo che rallenta, e un campo che restituisce tempo agli alleati. Nel GDD «salva i compagni con scudi globali, sollevamenti sull'asse Z e campi di accelerazione AP privi di debito»: le tre cose sottolineate sono tradotte o scartate qui sotto, una per una.

## Come leggere la traduzione

| Nel GDD | Nel contratto, in questa pagina |
|---|---|
| quattro fasi (Prep, Dash, Blast, Reaction) | cinque macro-fasi (ADR-0003): Prep, Dash, Blast, Move; le reazioni sono segmenti con trigger (ADR-0004), una **lista** per unità ([D-592](../../decisions/RT_PDR_00_Decision_Log.md)) |
| «AP / CD» per abilità | il costo AP è del pool di [D-584](../../decisions/RT_PDR_00_Decision_Log.md), valore **non deciso** (`MSP-1.n`); il cooldown resta un asse proprio (D-584 (4)) |
| «3×3» | area esagonale di raggio 1 (sette celle): la griglia è esagonale (ADR-0002) |
| «Scudo», «Freddo», «Aria» come elementi | `Element.Aegis`, `Element.Frost`, `Element.Air` ([D-593](../../decisions/RT_PDR_00_Decision_Log.md)) |
| HP / DEF / RES | scala ×10 e RES **per elemento** ([D-589](../../decisions/RT_PDR_00_Decision_Log.md)); i valori del GDD sono input per E53 |
| «+N AP», «senza debito Overclock», «Phase Lock» | l'Overclock e il Phase Lock **non esistono** (D-584 (5), D-265); un effetto sul budget AP è ammesso in forma, il valore è di `MSP-1.n` |
| «Z+2», «Airborne» | `FRTCellId::Layer` esiste, uno stato «in volo» no: `GDD-2` ([#4016](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4016)) |
| «Reaction (Guard)» | reazione di protezione dell'alleato, trigger «alleato bersagliato nel Blast» (famiglia di `Intercept`); la `Guard` del contratto è una principale del Prep |
| «Reaction (Evasion)» | reazione che sposta di una cella prima del danno (famiglia di `Evade`) |

## Identità tattica (input GDD §7, da verificare in playtest)

- **Sinergie dichiarate**: The Fey (Frost Binding + Nettle Shot: `Frost` + `Water` sulla stessa cella è la coppia `Element.Frost`+`Element.Water`, oggi `OPEN` nel [catalogo](../../balance/RT_ElementPairCatalog_v0.1.md)); Kallari (Alacrity Field le dà budget in più).
- **Forte contro** (GDD): Riktor e Steel, perché Consecrated Ground nega spinte e trazioni. **Debole contro**: detonazioni e blocchi di cura o scudo.
- ⚠️ Queste righe sono **input**, non bilanciamento: `Heroes.RosterIsBalanced` deciderà con i numeri.

## Statistiche base (input GDD §5.1, scala ×10 di D-589)

| Statistica | Valore GDD | Stato |
|---|---|---|
| HP | 1100 | input per E53 (CP 53.1–53.3); la scala ×10 del contratto porta i quattro di oggi a 900–1200 |
| DEF | 25 (~20% con 100/(100+DEF)) | input; la statistica non esiste in `Source/` |
| RES | 40 (~28%) | input; nel contratto è **per elemento**: il GDD ne dà una sola |
| Move | 3 hex | input; i Movement Point restano la distanza (D-584 (2)) |
| Gittata base | 5 hex | input |
| Affinità | `Element.Aegis` · `Element.Frost` · `Element.Air` | input per #995 |

## Profilo di attacco base

Il GDD marca **Frost Binding** come «Base (0-CD)»: è l'attacco base di Muriel ([ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md), un attacco base per eroe).

### Frost Binding — `Hero.Muriel.FrostBinding`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Frost` |
| Costo AP (GDD) | 2 · **non in vigore** |
| Cooldown (GDD) | 0 |
| Portata (GDD) | 5 hex |
| Danno (GDD) | 160 energetico |
| Effetto | applica `Element.Frost` sulla cella del bersaglio; rallenta il bersaglio |

**Traduzione**: il GDD dice *«aumenta di +1 AP il costo dei movimenti del bersaglio»*. Nel contratto i Movement Point sono la distanza e gli AP dicono *se* ti muovi (D-584 (2)): una tassa sugli AP del movimento è un numero di `MSP-1.n`. La resa che non inventa nulla è **`Status.Slow`** (*«ogni cella ti costa di più»*, glossario), con durata al catalogo. **Scartato**: la tassa in AP come effetto diretto, finché `MSP-1.n` non ha un vettore.

## Abilità

### Consecrated Ground — `Hero.Muriel.ConsecratedGround`

| Campo | Valore (input) |
|---|---|
| Fase | Prep |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | 2 / 2 · costo **non in vigore** |
| Area | raggio 1 (il «3×3» del GDD) |
| Effetto | scudo temporaneo di area (GDD: 250) agli occupanti; immunità a spinte e trazioni finché dura |

**Traduzione**: lo scudo temporaneo esiste nel PRD di E53 (`TemporaryShield`, consumato per primo); l'immunità a `Push`/`Pull` è oggi la reazione `Anchor` («annulla lo spostamento») resa stato di area: **candidato** `Status.Anchored`, da decidere con E53. **Scartato**: nulla.

### Reversal of Fortune — `Hero.Muriel.ReversalOfFortune`

| Campo | Valore (input) |
|---|---|
| Fase | Dash (mobilità speciale) |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | «2–3» / 2 · costo **non in vigore** |
| Effetto | si sposta in una cella adiacente a un alleato; all'arrivo scudo pesante all'alleato (GDD: 450) e onda d'urto protettiva |

**Traduzione**: una mobilità speciale con destinazione «cella libera adiacente a un alleato entro portata», come `Leap` con un vincolo di destinazione; l'«onda d'urto protettiva» si legge come `Push` dei nemici adiacenti all'arrivo (esiste). **Scartato**: *«vola globalmente verso un alleato ovunque sulla mappa»*: la portata illimitata non ha precedenti e va al catalogo come `RangeCells` finito; il costo «2–3 AP» variabile: un'azione ha un costo (D-584).

### Alacrity Field — `Hero.Muriel.AlacrityField`

| Campo | Valore (input) |
|---|---|
| Fase | Prep |
| Elemento | `Element.Air` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Area | raggio 1 |
| Effetto | gli alleati che iniziano il turno successivo nell'area hanno un budget AP maggiore in quel turno |

**Traduzione**: con il pool di D-584 un effetto sul budget del turno è esprimibile; il **valore** («+2») è di `MSP-1.n` e qui non esiste. **Scartato**: *«senza debito Overclock»*, perché l'Overclock non esiste (D-265).

## Reazioni (lista, D-592)

### Aegis — `Hero.Muriel.Aegis`

| Campo | Valore (input) |
|---|---|
| Trigger | un alleato è bersaglio di un colpo nel Blast |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | «1–2» / 2 · costo **non in vigore** |
| Effetto | scudo istantaneo all'alleato (GDD: 220) oppure trasferimento dei propri HP |

**Traduzione**: famiglia di `Intercept` (protezione dell'alleato). ⚠️ Il nome coincide con `Element.Aegis` ([D-593](../../decisions/RT_PDR_00_Decision_Log.md)): nei token i namespace li separano, nella prosa va detto quale dei due. **Scartato**: la doppia forma «HP oppure scudo» va sciolta in una: due effetti alternativi nello stesso ID sono due azioni.

### Harmonic Resonance — `Hero.Muriel.HarmonicResonance`

| Campo | Valore (input) |
|---|---|
| Trigger | un alleato entro 3 celle perde più di una quota dei suoi HP in un colpo (GDD: 30%) |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | scudo rigenerante d'emergenza all'alleato |

**Traduzione**: una reazione con **condizione dichiarata** ([D-109](../../decisions/RT_PDR_00_Decision_Log.md)) sulla soglia di danno. ⚠️ «Harmonic» non è più un elemento ([D-588](../../decisions/RT_PDR_00_Decision_Log.md)); come nome di abilità è del GDD e resta, segnalato. **Scartato**: «rigenerante» senza numero: la rigenerazione è un valore per turno al catalogo.

### Angelic Ascent — `Hero.Muriel.AngelicAscent`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un'area a terra |
| Elemento | `Element.Air` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | si sposta di una cella prima del danno, insieme a un alleato adiacente |

**Traduzione**: famiglia di `Evade` per sé e per un alleato adiacente. **Scartato**: *«solleva a Livello Z+2 (Airborne)»*: uno stato «in volo» non esiste e la quota è `GDD-2`.

## Che cosa si ritira con il rename (#4011)

Il kit di **Muiren** (`Hero.Muiren.PressureJet`, `CircularTide`, `FluidTrail`, `MistVeil`, `FlowReaction`) si **ritira** con [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2), non si rinomina. I golden e gli scenari che lo portano si ritirano con esso e **non si rigenerano**: la lista nominale è nella [scheda di Muiren](muiren.md), sezione «Ritiro del kit».

## Stato produzione

| Campo | Valore |
|---|---|
| Page Status | DESIGN_SPEC (input tradotto) |
| Stats | INPUT (GDD §5.1) |
| Abilities | TRANSLATED (4 + attacco base) |
| Reactions | TRANSLATED (3) |
| Numeri in vigore | **nessuno** |
| Release | v0.1 (dopo #4011 e `MSP-1.n`) |

## Governance

- **Fonte**: [`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) §5.1 e §7 (Muriel); decisioni [D-585](../../decisions/RT_PDR_00_Decision_Log.md), [D-586](../../decisions/RT_PDR_00_Decision_Log.md), [D-591](../../decisions/RT_PDR_00_Decision_Log.md), [D-593](../../decisions/RT_PDR_00_Decision_Log.md).
- Un kit **copiato** dal GDD è un `IMPLEMENTATION DRIFT` (#4014); questa pagina è la traduzione, e dice che cosa ha scartato.
- La Wiki è una vista: il runtime non legge Markdown. I valori competitivi entrano solo con la loro `D-nnn` e il catalogo.
