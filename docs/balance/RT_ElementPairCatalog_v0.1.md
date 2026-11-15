# RT — Catalogo delle coppie elementali v0.1

> **Owner** della regola *«ogni coppia non ordinata di elementi ha esattamente una voce, con uno stato»*
> ([D-275](../decisions/RT_PDR_00_Decision_Log.md)). Vocabolario: i **dodici** elementi di [D-588](../decisions/RT_PDR_00_Decision_Log.md),
> con gli identificatori di [D-593](../decisions/RT_PDR_00_Decision_Log.md). Issue: [#1803](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1803).
> **Creato**: 2026-11-08 su `main` = `78cf6b2f8` · **Validatore**: `python tools/balance/element_pairs_check.py` (coppie, unicita', stati, ReactionId).
>
> ⛔ **Una voce `OPEN` non porta effetti ne' numeri**: significa *non ancora progettata*. `NO_INTERACTION` significa *assenza voluta*.
> Solo `CURRENT` (a runtime) e `DESIGNED` (progettata, non implementata) portano un `Reaction.<Id>` stabile. Lo stato di supporto a runtime
> e' **per coppia**, non si deduce dal vocabolario (D-272 (2)). `Element`, `Surface`, `Status` e `Reaction` restano quattro cose distinte:
> `Terrain.Fire` e `Status.Burning` non sono `Element.Fire`.
>
> **Nessun ordine ciclico** fra gli elementi ([D-588](../decisions/RT_PDR_00_Decision_Log.md) (3)): la relazione fra due elementi vive solo nella sua voce.
> La colonna «note e input» cita come **candidato** la matrice di fusione del «Master Game Design Document» del 2026-11-08
> ([D-591](../decisions/RT_PDR_00_Decision_Log.md) (3), [`../research/handoff/master-game-design-document-input-2026-11-08.md`](../research/handoff/master-game-design-document-input-2026-11-08.md) §4.2),
> tradotta con D-593; i Finisher a tre cariche del GDD §4.3 **non** entrano (D-583 (8)).

## Gli elementi

| ID | Nome che il giocatore legge |
|---|---|
| `Element.Warp` | Warp |
| `Element.Fire` | Fuoco |
| `Element.Water` | Acqua |
| `Element.Earth` | Terra |
| `Element.Electric` | Fulmine |
| `Element.Frost` | Freddo |
| `Element.Decay` | Morte |
| `Element.Life` | Vita |
| `Element.Arcane` | Arcano |
| `Element.Aegis` | Scudo |
| `Element.Air` | Aria |
| `Element.Disintegration` | Disintegrazione |

## Le coppie

Ordine: per il primo elemento nell'ordine della tabella sopra, poi per il secondo. 12·11/2 = 66 righe.

| Elemento A | Elemento B | Stato | ReactionId | Note e input |
|---|---|---|---|---|
| `Element.Warp` | `Element.Fire` | OPEN | — | input GDD §4.2: «Terreno fuso (costo del movimento x2)» (candidato, nessun effetto in vigore) |
| `Element.Warp` | `Element.Water` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Earth` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Electric` | OPEN | — | input GDD §4.2: «Muro elettrificato (i muri conducono)» (candidato, nessun effetto in vigore) |
| `Element.Warp` | `Element.Frost` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Life` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Aegis` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Air` | OPEN | — | nessun input |
| `Element.Warp` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Fire` | `Element.Water` | CURRENT | `Reaction.WaterQuenchesFire` | l'acqua spegne il fuoco: `Action.CreateWater` «spegne il fuoco» (`RTCatalogLibrary.cpp`), `Status.Wet` «bagnarti spegne le fiamme» (glossario); scenario `Spec/Environment/WaterQuenchesFire.json`, golden `Spec.Environment.WaterQuenchesFire` · input GDD §4.2: «Vapore (acceca, annulla LoS)» (non in vigore: il comportamento a runtime e' quello descritto) |
| `Element.Fire` | `Element.Earth` | OPEN | — | input GDD §4.2: «Magma (costo del movimento x2)» (candidato, nessun effetto in vigore) |
| `Element.Fire` | `Element.Electric` | OPEN | — | input GDD §4.2: «Plasma (-50% RES energetica)» (candidato, nessun effetto in vigore) |
| `Element.Fire` | `Element.Frost` | OPEN | — | input GDD §4.2: «Shock termico (0% DEF fisica)» (candidato, nessun effetto in vigore) |
| `Element.Fire` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Fire` | `Element.Life` | OPEN | — | nessun input |
| `Element.Fire` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Fire` | `Element.Aegis` | OPEN | — | nessun input |
| `Element.Fire` | `Element.Air` | OPEN | — | input GDD §4.2: «Tempesta di fuoco (espande il danno ai vicini)» (candidato, nessun effetto in vigore) |
| `Element.Fire` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Water` | `Element.Earth` | OPEN | — | input GDD §4.2: «Fango (blocca il Dash)» (candidato, nessun effetto in vigore) |
| `Element.Water` | `Element.Electric` | CURRENT | `Reaction.ElectricPropagatesThroughWater` | l'elettricita' si propaga lungo l'acqua: `Action.Electrify` con `PropagationLimit = 3` risolve nel Cleanup «cosi' colpisce anche chi e' appena entrato nell'acqua» (`RTCatalogLibrary.cpp`), `Hero.Gadget.LinearDischarge` fa piu' danno a chi e' `Wet`; scenario `Spec/Environment/ElectricPropagation.json` · input GDD §4.2: «Acqua elettrificata (stun se attraversata)» (non in vigore: il comportamento a runtime e' quello descritto) |
| `Element.Water` | `Element.Frost` | OPEN | — | input GDD §4.2: «Ghiaccio (root assoluto)» (candidato, nessun effetto in vigore) |
| `Element.Water` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Water` | `Element.Life` | OPEN | — | nessun input |
| `Element.Water` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Water` | `Element.Aegis` | OPEN | — | input GDD §4.2: «Bolla rigenerante (cura e AP agli alleati)» (candidato, nessun effetto in vigore) |
| `Element.Water` | `Element.Air` | OPEN | — | input GDD §4.2: «Nebbia (schivata da lontano)» (candidato, nessun effetto in vigore) |
| `Element.Water` | `Element.Disintegration` | OPEN | — | input GDD §4.2: «Nebbia corrosiva (blocca le cure, degrada le coperture leggere)» (candidato, nessun effetto in vigore) |
| `Element.Earth` | `Element.Electric` | OPEN | — | input GDD §4.2: «Faglia ionizzata (attira i proiettili)» (candidato, nessun effetto in vigore) |
| `Element.Earth` | `Element.Frost` | OPEN | — | input GDD §4.2: «Permafrost (muro indistruttibile)» (candidato, nessun effetto in vigore) |
| `Element.Earth` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Earth` | `Element.Life` | OPEN | — | nessun input |
| `Element.Earth` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Earth` | `Element.Aegis` | OPEN | — | input GDD §4.2: «Bastione sismico (riduzione della cella al 75%)» (candidato, nessun effetto in vigore) |
| `Element.Earth` | `Element.Air` | OPEN | — | input GDD §4.2: «Tempesta di sabbia (blind, niente reazioni)» (candidato, nessun effetto in vigore) |
| `Element.Earth` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Electric` | `Element.Frost` | OPEN | — | input GDD §4.2: «Superconduttore (raggio delle catene aumentato)» (candidato, nessun effetto in vigore) |
| `Element.Electric` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Electric` | `Element.Life` | OPEN | — | nessun input |
| `Element.Electric` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Electric` | `Element.Aegis` | OPEN | — | input GDD §4.2: «Specchio elettrico (riflette il danno elettrico)» (candidato, nessun effetto in vigore) |
| `Element.Electric` | `Element.Air` | OPEN | — | input GDD §4.2: «Tempesta ionica (silenzia le abilita')» (candidato, nessun effetto in vigore) |
| `Element.Electric` | `Element.Disintegration` | OPEN | — | input GDD §4.2: «Griglia molecolare (silenzia le reazioni, +1 AP per esagono)» (candidato, nessun effetto in vigore) |
| `Element.Frost` | `Element.Decay` | OPEN | — | nessun input |
| `Element.Frost` | `Element.Life` | OPEN | — | nessun input |
| `Element.Frost` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Frost` | `Element.Aegis` | OPEN | — | input GDD §4.2: «Bolla ibernata (assorbe il prossimo attacco)» (candidato, nessun effetto in vigore) |
| `Element.Frost` | `Element.Air` | OPEN | — | input GDD §4.2: «Bufera (slow e DoT cumulativo)» (candidato, nessun effetto in vigore) |
| `Element.Frost` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Decay` | `Element.Life` | OPEN | — | nessun input |
| `Element.Decay` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Decay` | `Element.Aegis` | OPEN | — | nessun input |
| `Element.Decay` | `Element.Air` | OPEN | — | nessun input |
| `Element.Decay` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Life` | `Element.Arcane` | OPEN | — | nessun input |
| `Element.Life` | `Element.Aegis` | OPEN | — | nessun input |
| `Element.Life` | `Element.Air` | OPEN | — | nessun input |
| `Element.Life` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Arcane` | `Element.Aegis` | OPEN | — | nessun input |
| `Element.Arcane` | `Element.Air` | OPEN | — | nessun input |
| `Element.Arcane` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Aegis` | `Element.Air` | OPEN | — | nessun input |
| `Element.Aegis` | `Element.Disintegration` | OPEN | — | nessun input |
| `Element.Air` | `Element.Disintegration` | OPEN | — | nessun input |

## Come si aggiorna una voce

1. Si decide l'esito con il suo owner e si registra (una `D-nnn` o la issue che la possiede); la voce passa a `DESIGNED` con un `Reaction.<Id>`.
2. Quando il resolver la esegue e un test la pinna, passa a `CURRENT` e la riga cita il test o lo scenario.
3. `NO_INTERACTION` si scrive solo con la ragione accanto: e' una decisione, non un'assenza.
4. Dopo ogni modifica: `python tools/balance/element_pairs_check.py` deve rispondere 0.
