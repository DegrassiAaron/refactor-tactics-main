# Obiettivi dinamici

> **Stato v0.1:** **pianificato, non ancora implementato** (E10)
> **Tipo:** guida giocatore, non normativa

## Perché servono

Gli obiettivi danno alla partita un motivo per **muoversi e contestare spazio**. Sono particolarmente importanti in un gioco con coperture e Overwatch: senza una pressione esterna, aspettare potrebbe diventare troppo conveniente.

## Interact / Activate

Il design di E10.1 prevede che un'unità possa interagire con un oggetto **adiacente** durante il Blast.

Gli oggetti previsti includono:

- porte;
- console;
- ascensori;
- generatori;
- sprinkler;
- ponti;
- obiettivi.

Il fatto che `Action.Interact/Activate` esista nel catalogo non significa che tutti questi oggetti siano già giocabili: E10 è ancora assente dalla build corrente.

## Obiettivo contestabile

La regola pianificata per E10.2:

- la verifica del controllo avviene nel **Cleanup**;
- anche una unità che usa `Wait` può contestare se soddisfa le condizioni di presenza;
- se le due squadre contestano in modo paritario, **nessuna ottiene progresso**.

## Fine partita

E10.3 prevede tre vie:

1. eliminazione della squadra avversaria;
2. raggiungimento dell'obiettivo;
3. raggiungimento del `RoundLimit`.

Al `RoundLimit`:

- vince chi ha più progresso obiettivo;
- progresso pari = **pareggio dichiarato**.

## RoundLimit

Non è una costante universale del gioco. È un dato del formato.

Per il **2v2 v0.1**:

- valore iniziale: **12**;
- banda di riferimento: **10–14** round;
- hard cap indicativo: **14–16**.

Cambiare il formato non deve richiedere di ricompilare il gioco.

## Stato reale

La roadmap corrente marca E10 come **assente**: non ci sono ancora test `Objectives.*`/l'implementazione completa degli oggetti contestabili. Questa pagina documenta il comportamento pianificato, non una feature già pronta.

## Fonti normative

- `docs/roadmap/roadmap-v0.1.md` §E10
- `docs/gameplay/spec-durata-partita-e-scala-mappe.md`
