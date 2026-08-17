# Wraith

![Wraith — Paragon asset base](../images/paragon/wraith.png)

> 🧩 **Stato RefactorTactics:** `CANDIDATE / SIGNATURE_DEFINED`  
> **Release:** `UNASSIGNED`  
> **Roster ufficiale:** `NO`  
> **Uso corrente:** base visuale del **roster v0.1** (`Hero.Wraith`) — [D-037](../../decisions/RT_PDR_00_Decision_Log.md). L'asset è impegnato; il concept di Wraith resta un candidato senza release.  
> **Asset base:** Paragon — Wraith  
> **RT Character ID:** `TBD`

## Panoramica

Nel Character Master Matrix, **Wraith** è uno slot asset Paragon candidato con macro ruolo **Recon / Predicter**. La Signature definita è **Insight**, affiancata da **Predictive shot**. Il design è legato ai framework **Prediction, Information** e richiede soprattutto **Partial knowledge, prediction intents, visibility**. Il kit completo, le statistiche competitive, il CharacterId e la release non sono ancora definiti.

Questa pagina descrive **il concept RefactorTactics**, non il kit originale del MOBA Paragon. L'asset Paragon è la base visiva/animativa; abilità, regole, numeri e identità finale di RefactorTactics devono restare originali e data-driven.

## Scheda dati

| Campo | Valore |
| --- | --- |
| Asset key | `ASSET_WRAITH` |
| Asset base | Wraith |
| Macro ruolo RT | Recon / Predicter |
| Signature primaria | **Insight** |
| Signature secondaria | Predictive shot |
| Framework principali | Prediction, Information |
| Complessità tecnica | 5/5 |
| Dipendenze tecniche | Partial knowledge, prediction intents, visibility |
| Design status | `SIGNATURE_DEFINED` |
| ReleaseVersion | `UNASSIGNED` |
| RT Character ID | `TBD` |
| Player question | Cosa farà l'avversario? |
| Provenienza asset | Final Paragon asset release (2018) |

## Descrizione della Signature

**Insight** è la Signature assegnata a Wraith nel Character Master Matrix. La meccanica secondaria associata è **Predictive shot**.

Il documento collega questa identità ai framework **Prediction, Information** e alle dipendenze **Partial knowledge, prediction intents, visibility**. Trigger, stato preciso, payoff, counterplay numerico e telegraphing non sono ancora definiti: non vengono completati qui per evitare di trasformare una pagina Wiki in una nuova fonte di design non approvata.

## Identità tattica attuale

Il ruolo di lavoro è **Recon / Predicter**. La Signature indica quale parte del sistema tattico questo slot dovrebbe mettere in primo piano, ma il comportamento completo verrà definito solo quando il personaggio passa da `SIGNATURE_DEFINED` a `KIT_DRAFT` / `DATA_SPEC`.

### Player question

> Cosa farà l'avversario?

Questa domanda è già esplicitamente definita nel Character Master Matrix.

> **Ownership del kit:** le abilità di questa pagina appartengono esclusivamente a questo personaggio. Le sinergie con altri eroi sono esempi derivati da stati, superfici, geometria e altre regole comuni; non sono abilità condivise. Vedi [Sinergie e combinazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni).

## Abilità / Skill

**Non definite.**

Il kit completo di Wraith non è ancora stato progettato per RefactorTactics. Non vengono importate automaticamente le abilità originali di Paragon e non vengono inventati nomi, danni, cooldown, range o effetti.

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

Partial knowledge, prediction intents, visibility.

La release non deve essere assegnata finché i framework necessari non sono abbastanza maturi da supportare uno scenario automatico, test deterministici e counterplay leggibile.

## Immagine

La card mostrata in cima è una **Wiki card generata dal dataset**, non l'artwork del personaggio. Il path è già stabile:

```text
docs/characters/images/paragon/wraith.png
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
