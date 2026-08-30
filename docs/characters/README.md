# Character Wiki

Questa cartella contiene le pagine personaggio di RefactorTactics.

## Struttura

```text
v0.1/        Gadget · Phase · Riktor · Wraith
v0.2/        Steel · Aurora · Murdock · Kwang
candidates/  34 asset Paragon candidati
images/      card / screenshot dei personaggi
paragon.md   indice completo dei 38 hero asset Paragon

spec-radar-profilo-personaggio.md   owner del modello radar Profile / Balance (D-105)
matrici-stati-personaggio.md        candidature di stato e trasformazione
```

Le pagine dei candidati **non assegnano una release** e non inventano kit o valori mancanti.

I nomi Paragon sono usati come **Asset Base**. L'identità finale RefactorTactics può essere rinominata quando CharacterId, display name e lore vengono approvati.

## Identità, asset e release sono tre cose

[D-037](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08) assegna a ogni eroe del roster uno **slot asset
Paragon** come base visuale. La tabella owner sta in [`paragon.md`](paragon.md#mapping-visuale-del-roster);
qui non si duplica.

| | Cos'è | Cosa non è |
|---|---|---|
| **Identità RefactorTactics** | nome canonico (**Gadget**), kit, lore, fazione | non cambia perché cambia l'asset |
| **Stable ID** | `Hero.Gadget` — chiave tecnica di codice, scenari e replay | **non è il nome** del personaggio |
| **Slot asset Paragon** | mesh, scheletro, animazioni del prototipo | non è un personaggio del roster |
| **Release** | quando l'eroe entra nel roster operativo | non si assegna a un candidato |

> 🔴 **`Steel`, `Aurora`, `Murdock` e `Kwang` sono CANDIDATI, non il secondo quartetto canonico**
> ([D-258](../decisions/RT_PDR_00_Decision_Log.md), che sincronizza la decisione d'autore
> `AUTHOR-ROSTER-001`, 2026-08-30). Il roster **può** espandersi a otto, ma le quattro identità aggiuntive
> si decidono con **`E35`**, e fino ad allora ⛔ **non si coniano `HeroId` stabili** dai loro nomi — né in
> `Source/`, né in un catalogo di [`../balance/`](../balance/README.md), né in `Content/` versionato.
> Il motivo è misurato: uno Stable ID si ritira a caro prezzo, e
> [D-130](../decisions/RT_PDR_00_Decision_Log.md) lo ha già pagato una volta per il primo quartetto.
> ✅ Le pagine in `v0.2/` restano quindi ciò che questo documento già dichiara — schede di candidatura senza
> release assegnata — e `D-258` ne rende la regola tracciabile invece che implicita.
>
> ✅ **`Gadget`, `Phase`, `Riktor` e `Wraith` sono invece identità di PRODOTTO definitive**
> ([D-257](../decisions/RT_PDR_00_Decision_Log.md) ← `AUTHOR-ID-001`): la domanda *«sono identità di sviluppo
> legate ai pack Paragon o identità di prodotto?»* è risposta, e la risposta non cambia nulla nel codice.
> `Flux`, `Riva`, `Bastion` e `Vektor` restano **nomi storici e superati**, e le loro occorrenze si
> conservano dove documentano una provenienza.

🔄 **Riga aggiornata il 2026-08-13 da [D-120](../decisions/RT_PDR_00_Decision_Log.md).** Prima di quella
decisione le colonne erano **tre** e il nome canonico coincideva con lo Stable ID; oggi sono quattro perché
i due si sono separati.

<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
Conseguenze pratiche: `Hero.Flux` continua a **non** diventare `Hero.Gadget` — nessun ID si rinomina — ma la
ragione non è più che «lo slot non è l'identità»: è che la migrazione ha un costo proprio e un blocker
misurato, [#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716). Un asset Paragon usato
come base visuale non guadagna una release. ⚠️ **E `Gadget` nudo ora è ambiguo, non vietato**: nomina il
personaggio in prosa player-facing, mentre `Gadget.<Oggetto>` resta il namespace dell'equipaggiamento
(`Gadget.Medkit`, `ERTEquipmentSlot::Gadget`). Nei contesti tecnici si continua a scrivere `Paragon.Gadget`
per lo slot e `Hero.Gadget` per l'ID.

## Ownership dei kit

Ogni abilità appartiene a un singolo personaggio/definizione. Le sinergie sono esempi esterni al kit: [Sinergie e combinazioni](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni).

## Matrici di design

[`matrici-stati-personaggio.md`](matrici-stati-personaggio.md) — candidature di stato/trasformazione per
l'intero roster, costo sistemico, budget di complessità e stato di validazione. È **tracciabilità**, non una
fonte di regole: l'owner è [`../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../gameplay/brief-stati-personaggio-e-trasformazioni.md).
Nessun personaggio ha uno stato assegnato.

## `Misplay / Failure State` — copertura, 2026-08-08

[D-032](../decisions/RT_PDR_00_Decision_Log.md) ha aggiunto allo schema della Signature un campo che mancava:
*cosa resta in mano al giocatore che usa la meccanica correttamente ma legge male il turno*. È distinto dal
`Counterplay`, che descrive invece cosa fa l'avversario — vedi [`_Template.md`](_Template.md).

| Livello | Pagine | Stato del campo |
|---|---:|---|
| `v0.1/` canonico | 4 | ✅ **compilato** su tutte e quattro |
| `v0.2/` | 4 | ⏳ da compilare quando il kit esce da `DESIGN_SPEC` |
| `candidates/` | 34 | ⛔ **non si compila**: la Signature è una riga del Character Master Matrix, senza kit né trigger. Dedurne un failure state significherebbe inventare la meccanica |

Il campo si compila **dal kit esistente**, non dall'intuizione: se la meccanica non ha ancora trigger e payoff
definiti, non ha nemmeno un modo definito di fallire. Vale la regola generale della cartella — *i campi
mancanti restano mancanti*.

Le quattro voci v0.1 non sono intercambiabili, ed è il test che il campo funziona: Wraith fallisce **nel
turno** (whiff, [D-016](../decisions/RT_PDR_00_Decision_Log.md)), Gadget **in silenzio** (carica spesa senza
picco), Riktor **in modo persistente** (la struttura resta e ostacola gli alleati), Phase **a favore
dell'avversario** (`Wet` non sa chi l'ha applicato, [D-029](../decisions/RT_PDR_00_Decision_Log.md)). Quattro
modi diversi di sbagliare, non quattro modi di fare meno danno.
