# Gadget

![Gadget — Paragon asset base](../images/paragon/gadget.png)

> 🧩 **Stato RefactorTactics:** `CANDIDATE / SIGNATURE_DEFINED`  
> **Release:** `UNASSIGNED`  
> **Roster ufficiale:** `NO`  
> **Uso corrente:** base visuale di **Flux** (`Hero.Flux`, v0.1) — [D-037](../../decisions/RT_PDR_00_Decision_Log.md). L'asset è impegnato; il concept di Gadget resta un candidato senza release.  
> **Asset base:** Paragon — Gadget  
> **RT Character ID:** `TBD`

## Panoramica

Nel Character Master Matrix, **Gadget** è uno slot asset Paragon candidato con macro ruolo **Engineer**. La Signature definita è **Device Network**, affiancata da **Remote activation**. Il design è legato ai framework **PersistentEntity, Network, Reaction** e richiede soprattutto **Devices, graph links, remote triggers**. Il kit completo, le statistiche competitive, il CharacterId e la release non sono ancora definiti.

Questa pagina descrive **il concept RefactorTactics**, non il kit originale del MOBA Paragon. L'asset Paragon è la base visiva/animativa; abilità, regole, numeri e identità finale di RefactorTactics devono restare originali e data-driven.

## Scheda dati

| Campo | Valore |
| --- | --- |
| Asset key | `ASSET_GADGET` |
| Asset base | Gadget |
| Macro ruolo RT | Engineer |
| Signature primaria | **Device Network** |
| Signature secondaria | Remote activation |
| Framework principali | PersistentEntity, Network, Reaction |
| Complessità tecnica | 5/5 |
| Dipendenze tecniche | Devices, graph links, remote triggers |
| Design status | `SIGNATURE_DEFINED` |
| ReleaseVersion | `UNASSIGNED` |
| RT Character ID | `TBD` |
| Player question | Che rete di dispositivi costruisco? |
| Provenienza asset | Initial Paragon asset release (2018) |

## Descrizione della Signature

**Device Network** è la Signature assegnata a Gadget nel Character Master Matrix. La meccanica secondaria associata è **Remote activation**.

Il documento collega questa identità ai framework **PersistentEntity, Network, Reaction** e alle dipendenze **Devices, graph links, remote triggers**. Trigger, stato preciso, payoff, counterplay numerico e telegraphing non sono ancora definiti: non vengono completati qui per evitare di trasformare una pagina Wiki in una nuova fonte di design non approvata.

## Identità tattica attuale

Il ruolo di lavoro è **Engineer**. La Signature indica quale parte del sistema tattico questo slot dovrebbe mettere in primo piano, ma il comportamento completo verrà definito solo quando il personaggio passa da `SIGNATURE_DEFINED` a `KIT_DRAFT` / `DATA_SPEC`.

### Player question

> Che rete di dispositivi costruisco?

Questa domanda è già esplicitamente definita nel Character Master Matrix.

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](../../wiki/game/sinergie-e-combinazioni.md).

## Abilità / Skill

**Non definite.**

Il kit completo di Gadget non è ancora stato progettato per RefactorTactics. Non vengono importate automaticamente le abilità originali di Paragon e non vengono inventati nomi, danni, cooldown, range o effetti.

Quando il personaggio entra in design attivo questa sezione dovrà contenere almeno:

| Slot | Stato |
| --- | --- |
| Ability 1 | `TBD` |
| Ability 2 | `TBD` |
| Ability 3 | `TBD` |
| Ability 4 | `TBD` |
| Reaction / Overwatch profile | `TBD` |

## Statistiche

**Non definite.** HP, movimento, vista, armor, resistenze, iniziativa, precisione e budget non sono canonici per questo candidato.

## Dipendenze di implementazione

Devices, graph links, remote triggers.

La release non deve essere assegnata finché i framework necessari non sono abbastanza maturi da supportare uno scenario automatico, test deterministici e counterplay leggibile.

## Immagine

La card mostrata in cima è una **Wiki card generata dal dataset**, non l'artwork del personaggio. Il path è già stabile:

```text
docs/characters/images/paragon/gadget.png
```

Può essere sostituita in seguito con uno **screenshot in-engine dell'asset Paragon già presente nel progetto**, mantenendo lo stesso nome file e senza modificare la pagina.

## Stato produzione

| Area | Stato |
| --- | --- |
| Signature | `DEFINED` |
| Kit | `NOT DEFINED` |
| Statistiche | `NOT DEFINED` |
| Risorsa firma | `NOT DEFINED` |
| Reazioni | `NOT DEFINED` |
| Equipaggiamento specifico | `NOT DEFINED` |
| Varianti | `NOT DEFINED` |
| Release | `UNASSIGNED` |

## Governance

Promuovere questo personaggio richiede aggiornamento coordinato di Character Data, Signature Mechanics, Ability Data, scenari automatici, Wiki e roadmap. La pagina non deve essere usata per assegnare valori competitivi non presenti nei dati autorevoli.

## Fonti

- `RefactorTactics_Character_Master_Matrix.md`
- `docs/src/data/characters-wiki-data-v0.4.xlsx`
- Epic Games — Paragon asset release pages
