# Wiki — audit player-first

> `SNAPSHOT` · **Data**: 2026-08-13 · **HEAD repo**: `d40ccf63` · **HEAD wiki**: `c89cfd5`
> **Cosa è**: l'inventario misurato del clone Wiki pubblicato, con la classificazione di ogni pagina e
> il lavoro che ne deriva. È la **Wave 0** dell'handoff
> `RefactorTactics_Wiki_PlayerFirst_Claude_Handoff_2026-08-13.md`, archiviato in
> [`../../archive/src/`](../../archive/src/) dopo il consumo.
> **Cosa non è**: la ristrutturazione. Nessuna pagina è stata riscritta qui dentro — le wave 1–8 sono
> issue, non prosa in questo file.

## Il metodo, prima dei numeri

Ogni riga è misurata sul clone reale, non letta a occhio. Per ciascuna pagina si contano righe,
parole, immagini, link in uscita e in ingresso, nomi legacy e **rumore tecnico** — cioè le occorrenze
dei pattern che l'handoff §17 vuole fuori dal corpo Player (`USTRUCT`/`UObject`/`TArray`, simboli
`URT*`/`ART*`, `.h`/`.cpp`, `RT-FEAT-*`, `#issue`, gate `G*`/`CP x.y`, Automation,
hash/determinismo). Lo script **non è versionato** — vedi [§ come si rimisura](#come-si-rimisura):
quel che deve restare riproducibile è il metodo, non il file.

⚠️ **Una misura sbagliata, corretta prima di scrivere.** Il primo passaggio dichiarava **89 pagine
orfane su 91**: il conteggio dei link leggeva solo la sintassi markdown `[testo](url)`, mentre questa
Wiki usa il wiki-style `[[Titolo|slug]]` — **369 link contro 84**, l'81% invisibile. Le orfane vere
sono **7**, e sono tutte pagine di servizio. Un numero assurdo è più facile da vedere di uno
plausibile: quello era assurdo.

## Quanto è grande

**91 pagine · 83 387 parole.** Non è una wiki piccola, e la sua distribuzione è il primo risultato
dell'audit:

| Cartella | Pagine | Parole | % parole |
|---|--:|--:|--:|
| `Personaggi` | 43 | 48 415 | **58%** |
| `Guida` | 16 | 12 450 | 15% |
| `(root)` | 10 | 8 805 | 11% |
| `Meccaniche` | 11 | 7 888 | 9% |
| `Fazioni` | 4 | 3 345 | 4% |
| `Meta` | 5 | 1 867 | 2% |
| `Tecnica` · `images` | 2 | 617 | 1% |

**Il 58% della Wiki sono schede personaggio, e la maggioranza descrive personaggi che non esistono nel
gioco**: 30 delle 43 pagine sono candidati Paragon non selezionati — **21 512 parole, il 26% del
totale**. Non è contenuto sbagliato: è materiale di selezione, che oggi vive nello stesso spazio del
manuale. È la ragione principale per cui la Wiki *legge* come documentazione di progetto.

## Classificazione

Assegnata da regole dichiarate, non pagina per pagina a mano — così si rigenera quando la Wiki cambia.

| Classe | Pagine | Parole | Cosa comprende |
|---|--:|--:|---|
| `PLAYER-REWRITE` | 30 | 17 934 | riscrittura col template Player, stesso slug |
| `HIDE` | 30 | 21 512 | resta nel clone, esce dalla sidebar Player |
| `MOVE-DEV` | 12 | 10 701 | si sposta sotto Developer Zone |
| `SPLIT` | 6 | 6 940 | una pagina diventa più pagine tematiche |
| `KEEP` | 5 | 10 064 | invariata |
| `STALE-AUDIT` | 4 | 3 345 | verifica di validità prima di ogni riscrittura |
| `REMOVE/ARCHIVE` | 4 | 12 891 | rimozione o redirect, **dopo** la issue che la possiede |
| **Totale** | **91** | **83 387** | |

**Le regole** — `Meta/`, `Tecnica/`, `images/`, `Stato-del-progetto`, `Stato-delle-feature`,
`Profilo-tattico`, `Percorso-di-release` → `MOVE-DEV`; `Fazioni/` → `STALE-AUDIT`; `Meccaniche/` e
`Guida/` → `PLAYER-REWRITE`, tranne le sei pagine che l'handoff §14 indica come `SPLIT`; in
`Personaggi/`: roster giocabile → `PLAYER-REWRITE`, roster v0.2 → `KEEP`, nomi legacy →
`REMOVE/ARCHIVE`, candidati → `HIDE`.

### Per cartella

| Cartella | Pagine | Classi |
|---|--:|---|
| `(root)` | 10 | PLAYER-REWRITE 5 · MOVE-DEV 4 · KEEP 1 |
| `Fazioni` | 4 | STALE-AUDIT 4 |
| `Guida` | 16 | PLAYER-REWRITE 10 · SPLIT 6 |
| `Meccaniche` | 11 | PLAYER-REWRITE 11 |
| `Meta` | 5 | MOVE-DEV 5 |
| `Personaggi` | 43 | HIDE 30 · KEEP 4 · REMOVE/ARCHIVE 4 · PLAYER-REWRITE 4 · MOVE-DEV 1 |
| `Tecnica` · `images` | 2 | MOVE-DEV 2 |

## Rumore tecnico nel percorso Player

**36 pagine Player su 41 contengono almeno un riferimento tecnico.** La densità dice quanto pesa, non
solo se c'è: occorrenze per 1000 parole.

| Pagina | Densità | Totale | Classe |
|---|--:|--:|---|
| `Guida/regole-fondamentali.md` | 35,4 | 14 | `PLAYER-REWRITE` |
| `Personaggi.md` | 34,1 | 17 | `PLAYER-REWRITE` |
| `Guida/come-si-gioca.md` | 31,3 | 31 | `PLAYER-REWRITE` |
| `Guida/mappa-terreni-e-ambiente.md` | 31,1 | 20 | `SPLIT` |
| `Meccaniche/collisioni.md` | 29,3 | 13 | `PLAYER-REWRITE` |
| `Guida/esempio-di-round.md` | 29,1 | 13 | `PLAYER-REWRITE` |
| `Guida/reazioni-overwatch-e-previsioni.md` | 27,8 | **52** | `SPLIT` |
| `Guida/combattimento-e-targeting.md` | 26,6 | 11 | `SPLIT` |
| `Guida/obiettivi-e-fine-partita.md` | 25,8 | 10 | `PLAYER-REWRITE` |
| `Guida/visibilita-rumore-e-informazione.md` | 25,3 | 21 | `SPLIT` |

⚠️ `come-si-gioca` e `reazioni-overwatch-e-previsioni` sono le due pagine con più rumore in assoluto
(31 e 52 occorrenze) e sono anche **le prime due che un giocatore nuovo apre**. La densità non è un
difetto di stile: è la misura di quanto la pagina parla a qualcun altro.

## Nomi legacy

| Pagina | Occorrenze | Classe | Chi la possiede |
|---|--:|---|---|
| `Personaggi/wraith.md` | 16 | `REMOVE/ARCHIVE` | **#757** |
| `Personaggi/gadget.md` | 14 | `REMOVE/ARCHIVE` | **#757** |
| `Personaggi/riktor.md` | 13 | `REMOVE/ARCHIVE` | **#757** |
| `Personaggi/phase.md` | 13 | `REMOVE/ARCHIVE` | **#757** |
| `Guida/sinergie-e-combinazioni.md` | 12 | `PLAYER-REWRITE` | questo lavoro |
| `Personaggi/paragon.md` | 6 | `MOVE-DEV` | mappatura pack: legittimi |
| `Meccaniche/acqua-e-elettricita.md` | 4 | `PLAYER-REWRITE` | questo lavoro |

**78 occorrenze in 7 pagine.** Le quattro schede legacy **non si toccano qui**: sono la fetta 7 di
D-130, tracciata da **#757**, e una seconda migrazione concorrente è esattamente ciò che l'handoff
§1.2 vieta. Le due occorrenze player-facing fuori da quel perimetro — `sinergie-e-combinazioni` e
`acqua-e-elettricita` — entrano invece nelle wave di riscrittura.

## ⚠️ La classificazione di `Personaggi/` descrive la destinazione, non lo stato

*(Correzione del 2026-08-14, dalla review di #757.)* Le regole qui sopra marcano `gadget`, `phase`,
`riktor`, `wraith` come `PLAYER-REWRITE` e `gadget`, `phase`, `riktor`, `wraith` come
`REMOVE/ARCHIVE`. **Rispetto allo stato pubblicato è rovesciato**: le pagine vive sono le legacy.

| Pagina raggiungibile | Parole | Link entranti | Pagina nuova | Parole | Link entranti |
|---|--:|---|---|--:|---|
| `gadget.md` | 3 269 | sidebar + 5 pagine | `gadget.md` | 729 | solo `paragon.md` |
| `phase.md` | 3 232 | sidebar + 5 | `phase.md` | 744 | solo `paragon.md` |
| `riktor.md` | 3 195 | sidebar + 5 | `riktor.md` | 735 | solo `paragon.md` |
| `wraith.md` | 3 195 | sidebar + 5 | `wraith.md` | 725 | solo `paragon.md` |

E la sidebar dichiara già i nomi nuovi puntando alle pagine vecchie — `[[Gadget|gadget]]` — quindi un
giocatore che clicca «Gadget» legge oggi una pagina che scrive «Gadget» quattordici volte.

**Cosa resta valido**: le classi dicono dove ogni pagina deve *arrivare*, e su questo non cambia
nulla. **Cosa era sbagliato**: leggerle come una fotografia dello stato. Una pagina `PLAYER-REWRITE`
con 729 parole e un solo link entrante non è «da riscrivere», è **da riempire**; e una
`REMOVE/ARCHIVE` con 3 269 parole e sei link entranti non si rimuove — prima si svuota, migrando il
contenuto altrove.

Il perimetro corretto è nella **#757**, riscritta il 2026-08-14: 12 891 parole da migrare, non
quattro file da rinominare.

## Pagine orfane

Sette, e nessuna è un problema di navigazione: sono file di servizio (`_Sidebar`, `_Footer`), guide di
processo in `Meta/` e il README delle immagini. Nessuna pagina di contenuto è orfana.

## Le due issue esistenti, riconciliate

| Issue | Stato | Decisione |
|---|---|---|
| **#422** — *Wiki: eseguire il consolidamento già pianificato* | aperta | **Da riscrivere, non da chiudere.** Descrive un modello precedente a **D-076** (il clone Wiki è la fonte unica): la parte «portare la prosa fuori dal ciclo di deploy» è già realizzata, quella che resta — un'informazione architecture leggibile — è ciò che questo lavoro sostituisce. Diventa la issue-epic della ristrutturazione, invece di generare un duplicato |
| **#757** — *[D-130 · fetta 7] Wiki: quattro schede rinominate* | aperta | **Invariata, e prerequisito della Wave 5.** Possiede le quattro pagine `REMOVE/ARCHIVE`. Le wave 1–4 e 6–8 non la toccano e possono procedere in parallelo |

## Il lavoro, in wave

Ogni wave è una issue indipendente, e le otto **esistono**: #821–#828, sotto l'epic **#422**. La stima è in **pagine**, l'unica unità che questo audit misura;
nessuna stima in giorni, coerentemente con la roadmap.

| Wave | Issue | Contenuto | Pagine | Dipende da |
|---|---|---|--:|---|
| **1** Fondamenta Player | [#821](https://github.com/DegrassiAaron/refactor-tactics-main/issues/821) | Home, Cos'è, Come si gioca, Il turno simultaneo, La tua prima partita, Sidebar | 6 | — |
| **2** Core gameplay | [#822](https://github.com/DegrassiAaron/refactor-tactics-main/issues/822) | Movimento, Attacco, Guard, Brace, Overwatch, Reazioni, Facing, Planning | 8 + 3 `SPLIT` | 1 |
| **3** Battlefield | [#823](https://github.com/DegrassiAaron/refactor-tactics-main/issues/823) | Griglia, Coperture, LOS, Altezza, Porte, Ponti, Tunnel, Geometria | 8 + 1 `SPLIT` | 1 |
| **4** Environment e informazione | [#824](https://github.com/DegrassiAaron/refactor-tactics-main/issues/824) | Acqua, Elettricità, Fuoco, Ghiaccio, Vapore, Terreni, Cadute, Interazioni + Visione, Rumore, Fog of War, Last contact, Certezza | 13 + 2 `SPLIT` | 3 |
| **5** Personaggi | [#825](https://github.com/DegrassiAaron/refactor-tactics-main/issues/825) | 4 schede col template B, 30 candidati fuori dalla sidebar | 4 + 30 `HIDE` | **#757** |
| **6** Strategia | [#826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/826) | 5 guide P0, 4 P1 | 9 (nuove) | 2 |
| **7** Developer Zone | [#827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/827) | 12 pagine `MOVE-DEV` + hub | 13 | 1 |
| **8** Cleanup | [#828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/828) | link, redirect, sidebar finale, verifica a schermo | — | tutte |

## Due pagine classificate che nessuna wave copre

Trovato dalla code review della PR, e non dalla tabella: **l'aritmetica dei `SPLIT` tornava** — 6
classificati, 6 distribuiti fra le wave 2, 3 e 4 — ma non erano gli stessi sei.

| Pagina | Classe | Problema |
|---|---|---|
| `Guida/combattimento-e-targeting.md` | `SPLIT` | densità di rumore **26,6**, top-10 della Wiki. Nessuna delle otto issue la nomina: la Wave 2 conta tre `SPLIT` che sono azioni-e-movimento, planning-e-coordinazione e reazioni-overwatch |
| `Guida/sinergie-e-combinazioni.md` | `PLAYER-REWRITE` | **12 occorrenze legacy**, e questo documento prometteva che «entra nelle wave di riscrittura». Nessuna wave la nomina: a wave completate le 12 occorrenze resterebbero pubblicate, contro D-130 |

Entrambe vanno aggiunte al perimetro: `combattimento-e-targeting` alla **Wave 2** (#822),
`sinergie-e-combinazioni` alla **Wave 5** (#825), dove stanno le sinergie fra personaggi.

⚠️ La lezione non è l'omissione: è che **un totale che torna non dimostra che gli elementi siano gli
stessi**. Sei classificati e sei assegnati erano due insiemi diversi con la stessa cardinalità.

## Cosa questo audit non ha fatto

- **Nessuna pagina è stata riscritta.** L'handoff vieta il big bang e prescrive wave; questa è la Wave 0.
- **Nessuna verifica a schermo.** Il §22 la richiede per le pagine *modificate*: non essendocene, non
  è ancora dovuta. Resta un gate delle wave 1–8.
- **Nessuna icona prodotta.** Il §12 chiede di riusare il catalogo runtime e di non crearne un secondo:
  l'insieme richiesto è derivato da `URTIconLibrary::RequiredIconIds()` e va letto eseguendo, non
  trascritto. Le editorial icon restano da decidere.
- **La roadmap non è stata toccata.** Il consolidamento del modello di release fino alla v1.0 è in
  corso nella **PR #818** (`D-136`): duplicarlo qui avrebbe prodotto conflitti su `feature-registry`,
  `project-graph` e `roadmap-*.md`.

## Come si rimisura

Lo script di audit vive nello scratchpad di sessione e non è versionato: quello che conta è che il
metodo sia riproducibile. Le tre misure che invecchiano più in fretta sono **91 pagine**, **83 387
parole** e **78 occorrenze legacy**; si rileggono contando sul clone, e ogni cifra di questo documento
viene da lì.
