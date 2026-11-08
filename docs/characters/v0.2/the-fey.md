# The Fey

> 🧪 **Stato repository:** `DESIGN_SPEC` tradotta dal «Master Game Design Document» del 2026-11-08 ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3)), per decisione d'autore [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2). **Nulla di questa pagina è a runtime.** Release **v0.2**: con [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (4) E35.2 cambia soggetto (The Fey e Feng Mao al posto di Aurora e Kwang) e scorre in v0.2. I numeri sono **input**, marcati tali (costi AP: `MSP-1.n`; HP, DEF, RES: E53 [#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)). Issue: [#4014](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4014).

> **Asset base:** Paragon — The Fey ([scheda candidata](../candidates/the-fey.md))
> **Hero_Key:** `ASSET_THE_FEY`
> **RT Character ID:** `Hero.TheFey` (da assegnare con E35.2; lo slot sostituito, Aurora, aveva `Hero.Rime` senza runtime)
> **Release:** `v0.2` (E35.2)
> **Fazione:** Resonance (ereditata dallo slot sostituito, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (3))
> **Ruolo:** Terrain Controller / Zoner (Signature *Growth*, *Propagation*)
> **Provenienza visuale:** mesh e animazioni dallo slot Paragon **The Fey**; l'asset **non è nel checkout**: l'import è lavoro di E35.2.

## Panoramica

Alchimista del terreno: allaga le celle, alza rovi, trascina i nemici al centro e li inchioda nel gelo. Nel GDD «strangola l'economia AP e blocca la Dash Phase avversaria»: nel contratto le sue tasse in AP diventano costi di terreno (Movement Point, [D-117](../../decisions/RT_PDR_00_Decision_Log.md)) o stati, e il blocco del Dash passa dalle coppie elementali, oggi `OPEN`.

## Come leggere la traduzione

| Nel GDD | Nel contratto, in questa pagina |
|---|---|
| «attraversarlo costa +2 AP», «+1 AP al costo dei movimenti» | il costo **per cella** è dei Movement Point e del terreno ([D-117](../../decisions/RT_PDR_00_Decision_Log.md), [D-190](../../decisions/RT_PDR_00_Decision_Log.md)); gli AP dicono se ti muovi (D-584 (2)): le tasse del GDD sono costi di terreno o `Status.Slow` |
| «Fango (Acqua+Terra)» | coppia `Element.Water`+`Element.Earth`, oggi `OPEN` nel [catalogo](../../balance/RT_ElementPairCatalog_v0.1.md) con il «Fango» del GDD come candidato: nessun effetto in vigore |
| «rianima un alleato caduto» | il K.O. è definitivo nel contratto (vittoria per eliminazione, ADR-0003): la rianimazione è una regola **nuova**, scartata qui |
| «Phase Lock» | non esiste (D-584 (5), D-265) |
| «Blind / no LoS» | stato **candidato** (oggi `Status.Obscured` riduce la mira attraverso il fumo); owner E13 |
| «50% di riduzione danno» | la riduzione per colpo è **piatta** ([D-590](../../decisions/RT_PDR_00_Decision_Log.md) (3), D-408): valore al catalogo, non percentuale |
| «Acqua», «Terra», «Freddo», «Scudo» | `Element.Water`, `Element.Earth`, `Element.Frost`, `Element.Aegis` ([D-593](../../decisions/RT_PDR_00_Decision_Log.md)) |
| «AP / CD» | costo AP del pool di D-584, **non deciso**; cooldown asse proprio |

## Identità tattica (input GDD §7)

- **Sinergie dichiarate**: Riktor e Gadget; da sola crea `Water`+`Earth` (Untamed Growth + Bramble Patch), preparando la coppia con `Electric` (GDD: «Arco Voltaico»): tutte `OPEN`.
- **Forte contro** (GDD): Feng Mao e Murdock. **Debole contro**: Kallari (scavalca rovi e fango con Double Jump).

## Statistiche base (input GDD §5.1)

| Statistica | Valore GDD | Stato |
|---|---|---|
| HP | 1150 | input per E53 |
| DEF | 20 (~17%) | input |
| RES | 30 (~23%) | input; per elemento nel contratto |
| Move | 3 hex | input |
| Gittata base | 5 hex | input |
| Affinità | `Element.Water` · `Element.Earth` · `Element.Frost` · `Element.Aegis` | input per #995 |

## Profilo di attacco base

### Nettle Shot — `Hero.TheFey.NettleShot`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Water` |
| Costo AP / CD (GDD) | 1 / 0 · costo **non in vigore** |
| Portata (GDD) | 5 hex |
| Danno (GDD) | 150 perforante |
| Effetto | se il bersaglio sta su una cella `Water`+`Earth`, il budget AP del turno successivo di The Fey cresce |

**Traduzione**: «perforante» = DEF trattata come 0 (D-589); il rimborso è un effetto sul budget, forma ammessa e valore `MSP-1.n`; la condizione dipende dalla coppia `Water`+`Earth`, oggi `OPEN`. **Scartato**: il «+1» come numero.

## Abilità

### Untamed Growth — `Hero.TheFey.UntamedGrowth`

| Campo | Valore (input) |
|---|---|
| Fase | Prep (l'esplosione risolve nel Cleanup, come le azioni ambientali) |
| Elemento | `Element.Water` |
| Costo AP / CD (GDD) | 2 / 2 · costo **non in vigore** |
| Area | raggio 1 (il «3×3» del GDD) |
| Effetto | applica `Element.Water` all'area; a fine turno il bocciolo esplode (GDD: 180) |

**Traduzione**: `Action.CreateWater` esiste (fase `Environment`, Cleanup); l'esplosione a fine turno risolve nello stesso segmento. **Scartato**: nulla.

### Bramble Patch — `Hero.TheFey.BramblePatch`

| Campo | Valore (input) |
|---|---|
| Fase | Dash nel GDD · ⚠️ nel contratto la creazione di coperture è nel Prep (`Action.CreateCover`): **da decidere** |
| Elemento | `Element.Earth` |
| Costo AP / CD (GDD) | 2 / 2 · costo **non in vigore** |
| Forma | muro di rovi lungo 4 celle |
| Effetto | le celle dei rovi costano di più ai nemici (costo per cella, D-117); con `Element.Water` sulla stessa cella forma la coppia `Water`+`Earth` |

**Traduzione**: «+2 AP extra ai nemici» è un **costo di terreno** per fazione avversa (il terreno `Rough` esiste; un costo che distingue amici e nemici è un candidato per E8). **Scartato**: la tassa in AP; la fase Dash per una copertura, finché l'owner non decide.

### Flytrap — `Hero.TheFey.Flytrap`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Frost` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Area | raggio 3 attorno alla cella bersaglio |
| Effetto | trascina i nemici verso il centro (`Pull`, esiste), danno (GDD: 280), applica `Status.Root` e `Element.Frost` |

**Traduzione**: `Action.Pull` e `Status.Root` esistono; il trascinamento di più unità verso la stessa cella passa dalla contesa deterministica di D-302. **Scartato**: nulla.

### Nature's Vengeance — `Hero.TheFey.NaturesVengeance`

| Campo | Valore (input) |
|---|---|
| Fase | Prep |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Effetto (GDD) | rianima un alleato caduto al 25% degli HP in un bozzolo protettivo; l'alleato subisce Phase Lock |

**Traduzione**: **nessuna oggi**. La rianimazione contraddice il K.O. definitivo del contratto e il Phase Lock non esiste: l'abilità resta **input** per una decisione d'autore («esiste la rianimazione?»), che non è in nessuna istruttoria aperta e va aperta come `GDD-4` se la si vuole. Senza quella decisione, The Fey ha un'Ultimate sola.

## Reazioni (lista, D-592)

### Thorned Carapace — `Hero.TheFey.ThornedCarapace`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un attacco a distanza |
| Elemento | `Element.Water` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | il tiratore perde la vista per il resto del turno (stato candidato «Blind», owner E13) |

**Traduzione**: famiglia di `Counter` con effetto di stato invece che danno. **Scartato**: *«annullando i suoi colpi successivi»* come effetto separato: se il tiratore non vede, i suoi colpi successivi cadono nel fallback (`ERTActionFallback`), che esiste.

### Bramble Shield — `Hero.TheFey.BrambleShield`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un colpo nel Blast |
| Elemento | `Element.Earth` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | si radica (immune a spinte e trazioni, come `Anchor`) e riduce il danno dei colpi del Blast di un valore piatto |

**Traduzione**: `Anchor` esiste; la riduzione è piatta per colpo (D-408, D-590 (3)) con il valore al catalogo. **Scartato**: il «50%».

## Stato produzione

| Campo | Valore |
|---|---|
| Page Status | DESIGN_SPEC (input tradotto) |
| Stats | INPUT (GDD §5.1) |
| Abilities | TRANSLATED (3 + attacco base; 1 senza traduzione: Nature's Vengeance) |
| Reactions | TRANSLATED (2) |
| Numeri in vigore | **nessuno** |
| Release | v0.2 (E35.2) |

## Governance

- **Fonte**: [`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) §5.1 e §7 (The Fey); decisioni [D-585](../../decisions/RT_PDR_00_Decision_Log.md), [D-586](../../decisions/RT_PDR_00_Decision_Log.md), [D-591](../../decisions/RT_PDR_00_Decision_Log.md), [D-593](../../decisions/RT_PDR_00_Decision_Log.md).
- Un kit **copiato** dal GDD è un `IMPLEMENTATION DRIFT` (#4014); questa pagina è la traduzione, e dice che cosa ha scartato.
