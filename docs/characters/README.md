# Character Wiki

Questa cartella contiene le pagine personaggio di RefactorTactics.

## Struttura

```text
v0.1/        Flux · Riva · Bastion · Vektor
v0.2/        Steel · Aurora · Murdock · Kwang
candidates/  34 asset Paragon candidati
images/      card / screenshot dei personaggi
paragon.md   indice completo dei 38 hero asset Paragon
```

Le pagine dei candidati **non assegnano una release** e non inventano kit o valori mancanti.

I nomi Paragon sono usati come **Asset Base**. L'identità finale RefactorTactics può essere rinominata quando CharacterId, display name e lore vengono approvati.

## Identità, asset e release sono tre cose

[D-037](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08) assegna a ogni eroe del roster uno **slot asset
Paragon** come base visuale. La tabella owner sta in [`paragon.md`](paragon.md#mapping-visuale-del-roster);
qui non si duplica.

| | Cos'è | Cosa non è |
|---|---|---|
| **Identità RefactorTactics** | `Hero.Flux`, kit, lore, fazione | non cambia perché cambia l'asset |
| **Slot asset Paragon** | mesh, scheletro, animazioni del prototipo | non è un personaggio del roster |
| **Release** | quando l'eroe entra nel roster operativo | non si assegna a un candidato |

Conseguenze pratiche: `Hero.Flux` **non** diventa `Hero.Gadget`; un asset Paragon usato come base visuale non
guadagna una release; e si scrive sempre `Paragon.Gadget`, mai `Gadget` nudo — che qui è già una categoria di
equipaggiamento.

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

Le quattro voci v0.1 non sono intercambiabili, ed è il test che il campo funziona: Vektor fallisce **nel
turno** (whiff, [D-016](../decisions/RT_PDR_00_Decision_Log.md)), Flux **in silenzio** (carica spesa senza
picco), Bastion **in modo persistente** (la struttura resta e ostacola gli alleati), Riva **a favore
dell'avversario** (`Wet` non sa chi l'ha applicato, [D-029](../decisions/RT_PDR_00_Decision_Log.md)). Quattro
modi diversi di sbagliare, non quattro modi di fare meno danno.
