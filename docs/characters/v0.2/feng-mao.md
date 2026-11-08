# Feng Mao

> 🧪 **Stato repository:** `DESIGN_SPEC` tradotta dal «Master Game Design Document» del 2026-11-08 ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3)), per decisione d'autore [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (2). **Nulla di questa pagina è a runtime.** Release **v0.2**: con [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (4) E35.2 cambia soggetto (The Fey e Feng Mao al posto di Aurora e Kwang) e scorre in v0.2. I numeri sono **input**, marcati tali (costi AP: `MSP-1.n`; HP, DEF, RES: E53 [#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)). Issue: [#4014](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4014).

> **Asset base:** Paragon — Feng Mao ([scheda candidata](../candidates/feng-mao.md))
> **Hero_Key:** `ASSET_FENG_MAO`
> **RT Character ID:** `Hero.FengMao` (da assegnare con E35.2; lo slot sostituito, Kwang, aveva `Hero.Tethra` senza runtime)
> **Release:** `v0.2` (E35.2)
> **Fazione:** Resonance (ereditata dallo slot sostituito, [D-586](../../decisions/RT_PDR_00_Decision_Log.md) (3))
> **Ruolo:** Skirmisher / Bruiser (Signature *Flow*, *Directional momentum*)
> **Provenienza visuale:** mesh e animazioni dallo slot Paragon **Feng Mao**; l'asset **non è nel checkout**: l'import è lavoro di E35.2.

## Panoramica

Duellante di mischia che converte la difesa in tempo (uno scudo che, se infranto, restituisce budget), scatta lasciando fuoco dietro di sé e spazza le coperture con fendenti termici e sismici. Nel GDD il suo tratto è l'economia d'azione: nel contratto quel tratto è un effetto sul budget AP di [D-584](../../decisions/RT_PDR_00_Decision_Log.md), ammesso in forma e **senza numeri** finché `MSP-1.n` non li ha.

## Come leggere la traduzione

| Nel GDD | Nel contratto, in questa pagina |
|---|---|
| «rimborsa +1 AP» | effetto sul budget AP del turno: forma ammessa dal pool di D-584, valore di `MSP-1.n` |
| «cono frontale di 60°», «spazzata di 180°» | i lati di [D-126](../../decisions/RT_PDR_00_Decision_Log.md): un lato (`Front`), tre lati (`FrontLeft`, `Front`, `FrontRight`) |
| «polverizza le coperture», «distrugge l'armatura» | coperture con Integrity ([D-590](../../decisions/RT_PDR_00_Decision_Log.md) (3), E53 CP 53.5); una riduzione di DEF è uno stato per E53 |
| «Dazed (disabilita le Reaction)» | stato **nuovo**: oggi `Status.Prone` nega le reazioni («le tue reazioni non scattano»); candidato da decidere con ADR-0004 |
| «Fuoco», «Terra», «Scudo», «Disintegrazione» | `Element.Fire`, `Element.Earth`, `Element.Aegis`, `Element.Disintegration` ([D-593](../../decisions/RT_PDR_00_Decision_Log.md)) |
| «AP / CD» | costo AP del pool di D-584, **non deciso**; cooldown asse proprio |

## Identità tattica (input GDD §7)

- **Sinergie dichiarate**: The Fey e Muriel; da solo chiude `Element.Fire`+`Element.Earth` (Reaping Dash + Seismic Slice): coppia oggi `OPEN` nel [catalogo](../../balance/RT_ElementPairCatalog_v0.1.md), con il «Magma» del GDD come candidato.
- **Forte contro** (GDD): Murdock e gli assassini da mischia. **Debole contro**: The Fey (il fango blocca lo scatto): dipende dalla coppia `Water`+`Earth`, oggi `OPEN`.

## Statistiche base (input GDD §5.1)

| Statistica | Valore GDD | Stato |
|---|---|---|
| HP | 1350 | input per E53 |
| DEF | 35 (~26%) | input |
| RES | 25 (~20%) | input; per elemento nel contratto |
| Move | 3 hex | input |
| Gittata base | 1–2 hex | input |
| Affinità | `Element.Fire` · `Element.Earth` · `Element.Aegis` · `Element.Disintegration` | input per #995 |

## Profilo di attacco base

### Seismic Slice — `Hero.FengMao.SeismicSlice`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Earth` |
| Costo AP / CD (GDD) | 2 / 0 · costo **non in vigore** |
| Forma | il GDD ne dà due: cono su un lato («60°») **oppure** linea di 3 celle: da scegliere una |
| Danno (GDD) | 260 fisico |

**Traduzione**: la linea esiste (`Action.LineAttack`); il cono su un lato è l'arco di `Front` di D-126. **Da decidere**: una sola forma per l'attacco base (ADR-0007). **Scartato**: nulla.

## Abilità

### Conversion Shield — `Hero.FengMao.ConversionShield`

| Campo | Valore (input) |
|---|---|
| Fase | Prep |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | scudo temporaneo personale (GDD: 220 per due turni); se viene infranto nel turno corrente, il budget AP del turno successivo cresce |

**Traduzione**: lo scudo temporaneo è nel PRD di E53; il rimborso è un effetto sul budget (forma ammessa, valore `MSP-1.n`). **Scartato**: il «+1» come numero.

### Reaping Dash — `Hero.FengMao.ReapingDash`

| Campo | Valore (input) |
|---|---|
| Fase | Dash (mobilità speciale) |
| Elemento | `Element.Fire` |
| Costo AP / CD (GDD) | «1–2» / 2 · costo **non in vigore** |
| Distanza (GDD) | 3 hex |
| Effetto | applica `Element.Fire` alla cella d'arrivo e lascia fuoco sulle celle attraversate (GDD: 90 per turno per due turni) |

**Traduzione**: `Terrain.Fire` esiste (`Action.Ignite`): la scia è un `Ignite` sulle celle del percorso. **Scartato**: il costo «1–2» variabile.

### Earth Shatter — `Hero.FengMao.EarthShatter`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Disintegration` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Forma | linea di 5 celle |
| Danno (GDD) | 480 energetico perforante |
| Effetto | riduce a zero l'Integrity delle coperture leggere sulla linea |

**Traduzione**: «perforante» = DEF trattata come 0 (D-589); le coperture con Integrity sono CP 53.5. **Scartato**: nulla.

### Dragon's Breath — `Hero.FengMao.DragonsBreath`

| Campo | Valore (input) |
|---|---|
| Fase | Blast |
| Elemento | `Element.Fire` |
| Costo AP / CD (GDD) | 3 / 3 · costo **non in vigore** |
| Forma | tre lati frontali, profondità 2 celle (il «180°» del GDD) |
| Effetto | applica `Element.Fire` e riduce la DEF dei bersagli |

**Traduzione**: la riduzione di DEF è uno stato per E53, valore al catalogo. **Scartato**: *«distrugge l'armatura»* come azzeramento senza numero.

## Reazioni (lista, D-592)

### Parry & Riposte — `Hero.FengMao.ParryAndRiposte`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un attacco in mischia da uno dei tre lati frontali |
| Elemento | `Element.Aegis` |
| Costo AP / CD (GDD) | «1–2» / 2 · costo **non in vigore** |
| Effetto | devia il colpo (famiglia di `Deflect`) e risponde (famiglia di `Counter`, GDD: 200); applica all'attaccante uno stato che nega le reazioni |

**Traduzione**: `Deflect` + `Counter` esistono; lo stato «Dazed» è **candidato** (ADR-0004). **Scartato**: il costo variabile.

### Smoldering Step — `Hero.FengMao.SmolderingStep`

| Campo | Valore (input) |
|---|---|
| Trigger | è bersaglio di un attacco a distanza |
| Elemento | `Element.Fire` |
| Costo AP / CD (GDD) | 1 / 1 · costo **non in vigore** |
| Effetto | si sposta di una cella (famiglia di `Evade`) e lascia `Element.Fire` sulla cella d'origine |

**Scartato**: nulla.

## Stato produzione

| Campo | Valore |
|---|---|
| Page Status | DESIGN_SPEC (input tradotto) |
| Stats | INPUT (GDD §5.1) |
| Abilities | TRANSLATED (4 + attacco base) |
| Reactions | TRANSLATED (2) |
| Numeri in vigore | **nessuno** |
| Release | v0.2 (E35.2) |

## Governance

- **Fonte**: [`master-game-design-document-input-2026-11-08.md`](../../research/handoff/master-game-design-document-input-2026-11-08.md) §5.1 e §7 (Feng Mao); decisioni [D-585](../../decisions/RT_PDR_00_Decision_Log.md), [D-586](../../decisions/RT_PDR_00_Decision_Log.md), [D-591](../../decisions/RT_PDR_00_Decision_Log.md), [D-593](../../decisions/RT_PDR_00_Decision_Log.md).
- Un kit **copiato** dal GDD è un `IMPLEMENTATION DRIFT` (#4014); questa pagina è la traduzione, e dice che cosa ha scartato.
