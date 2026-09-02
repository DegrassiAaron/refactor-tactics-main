# La traversata autorata ha un produttore deciso e un dato che non esiste (`#1828`) — spec panel

> `CURRENT` · **Stato**: revisione chiusa, definizione `DNNN` consegnata alla issue ·
> **Data**: 2026-09-02
> **HEAD della revisione**: `92ae8eb9` (= `origin/main` al 2026-09-02)
> **Oggetto**: la issue [`#1828`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828) (`E23.7`)
> letta **come specifica di implementazione**.
> **Panel**: Fowler (lead) · Wiegers · Cockburn · Nygard · Crispin
> **Modo**: critique · **Focus**: requirements, architecture

---

## 1. Il verdetto in una riga

Dei sei acceptance criteria, **tre sono già verdi**, **uno è bloccato da una decisione aperta**, e i due che
restano dipendono da una scelta di modello che nessuna decisione ha fatto — benché `D-308` sembri averla
fatta.

| | Rilievo | Chi | Gravità |
|---|---|---|---|
| **R1** | il «produttore» che `D-308` nomina è **inter-cella**, l'enum che dovrebbe produrre è **intra-cella** | Fowler | 🔴 scelta di modello |
| **R2** | l'AC 4 (*swept clearance*) è bloccato da **`MAP-4`**, e `D-308` dichiara di non toccarla | Cockburn | 🛑 bloccante |
| **R3** | tre AC su sei sono **già coperti** da test in `main`, e il DoD non lo dice | Wiegers | 🟡 |

---

## 2. Ciò che è già verde, misurato su `92ae8eb9`

| AC | Stato |
|---|---|
| 1 · muro continuo senza traversata: `SideA → SideB` **rifiutata** | ✅ `CoverPlacement.ContinuousWallSeparatesSidesAndRejectsTransition` |
| 3 · muro centro-vertice: le due facce restano raggiungibili, **distinto** dal caso 1 | ✅ `CoverPlacement.CenterToVertexWallExposesBothSidesInOneRegion` |
| 6 · la capacità della cella resta **1** | ✅ `CoverPlacement.CoverOptionsDoNotIncreaseCellCapacity` |

∴ Metà della issue è la **conferma** di ciò che `D-289` ha già consegnato. Ciò che manca è il **terzo valore**
e il suo dato.

---

## 3. R1 — il produttore che `D-308` nomina non può produrre quel valore

**FOWLER**: `D-308` (2026-09-01) chiude `COV-7` e `COV-8` e si dichiara la risposta all'attesa di questa issue:

> il vault è una transizione **AUTORATA del grafo** — cioè **il produttore che il terzo valore di
> `ERTIntraCellTraversal` aspettava**. […] Questa voce lo **nomina**; non lo aggiunge, che resta lavoro di
> `#1828`.

Ma la stessa voce definisce il vault così:

> È una transizione esplicitamente autorata nel tactical graph, **fra celle adiacenti**, e costa `costo
> d'arco normale` + 1 MP Vault.

E `ERTIntraCellTraversal` risponde a *«si passa da questo settore a quest'altro **senza uscire dalla
cella**?»* — la sua firma è `(Mask, FromWedge, ToWedge)` e **non riceve nemmeno un `FRTCellId`**.

🔴 **Una transizione fra due celle non può emettere un valore di un enum che vive dentro una cella.** Un
vault da `C` a `D` non porta nessuno dal lato A al lato B di `C`: quello è *«un percorso reale attorno
all'estremo»*, che §6 elenca **separatamente** dal vault.

∴ Il produttore è nominato ma **non collegato**. Le uscite sono due, e non sono equivalenti:

| | **(a)** il vault vale anche DENTRO una cella | **(b)** il dato intra-cella è un altro |
|---|---|---|
| dove vive | un `FRTHexInteriorWall` porta la scavalcabilità | un tipo nuovo: apertura/porta interna |
| coerenza con `D-308` | estende *«fra celle adiacenti»* a *«fra due regioni»* | lascia `D-308` intatta e aggiunge accanto |
| coerenza con `D-289` | ✅ nessuna sottocella, nessun secondo slot | ✅ idem |
| costo | un campo su un tipo che esiste già | un tipo nuovo nel formato mappa |
| chi decide | è una **precisazione** di `D-308` | è una **decisione nuova** |

⚠️ **(a) è la più economica e la più fedele al principio che `D-308` scrive**: *«la scavalcabilità è un
**dato**, non una conseguenza dell'altezza»* — e un muro interno è precisamente il dato che divide la cella.
Ma resta una lettura: `D-308` dice *«fra celle adiacenti»*, e chi l'ha scritta potrebbe aver inteso
escludere il caso interno invece di non averlo considerato.

⛔ **Il panel non sceglie.** Implementare (a) significherebbe estendere una decisione d'autore di ieri
scrivendo codice invece di una riga di owner — l'errore che `#833` ha evitato.

---

## 4. R2 — la *swept clearance* è bloccata, e `D-308` lo dice

**COCKBURN**: l'AC 4 chiede che *«la swept clearance rifiuti un corridoio bloccato fra due estremi entrambi
validi»*, e lo Scope la elenca al punto 3. È la voce che `E23.7` porta dalla roadmap.

**Non è implementabile**, e la catena è scritta per intero:

- `D-071` punto (2) definiva il corridoio come *«il footprint traslato lungo A→B»*;
- `D-303` (2026-08-31) ha reso il footprint un **conteggio di settori**, e — testualmente — *«un conteggio
  non si trasla»*;
- ne è nata **`MAP-4`**, aperta: *«Con un footprint discreto, che cosa spazza il corridoio di transizione?»*,
  con due uscite dichiarate — o il corridoio si esprime in un terzo modo, o **la transizione smette di avere
  una clearance propria**;
- `D-308` chiude in modo esplicito: *«**non** tocca `MAP-4`, il corridoio spazzato, che `D-303` ha aperto e
  che un conteggio di settori non risolve»*.

∴ L'AC 4 **va scorporato**, non implementato. Scriverlo ora deciderebbe `MAP-4` per inerzia, e le sue due
uscite non sono equivalenti: la seconda **toglie** una regola invece di aggiungerla.

---

## 5. R3 — il DoD non dice che metà è fatta

**WIEGERS**: tre AC su sei sono verdi in `main` da `D-289`, e la issue li elenca come da fare. Chi la apre
oggi non ricava che il lavoro residuo è **un valore d'enum e il suo dato**.

Non è un difetto di rigore: la issue è del 2026-08-30 e i test sono arrivati con `#1827`, chiusa ieri. È
fotografia scaduta, e va aggiornata perché il perimetro reale si veda.

---

## 6. Il DoD che il panel consegna

| | Voce | Da |
|---|---|---|
| **=** | AC 1, 3, 6 | ✅ già verdi, da spuntare |
| **?** | AC 2, 5 — il terzo valore e il validator | ⏸ **dipendono da R1**: serve la precisazione su dove viva il dato |
| **—** | AC 4 — *swept clearance* | 🛑 **scorporato**: `MAP-4` è aperta e `D-308` dichiara di non toccarla |
| **+** | `ERTHexTransitionKind::Vault`, in coda, con `+1 MP` | `D-308` lo decide in ogni dettaglio: è implementabile **oggi**, ed è inter-cella |

---

## 7. Chiusura

`D-308` è stata scritta ieri e ha fatto quasi tutto: ha deciso il costo, ha vietato che `Low`/`High`
diventino un vocabolario di traversabilità, e ha nominato questa issue come proprio consumatore. Ciò che le
è sfuggito è di un solo grado: **il produttore che nomina opera su un dominio diverso da quello dell'enum
che deve produrre**, e nessuna delle due parti se ne accorge da sola.
