# Obiettivi dinamici

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-OBJECTIVE-SYSTEM -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-OBJECTIVE-SYSTEM` · Release: `v0.1` · Roadmap: `E10.1, E10.2`  
> Stato: **IMPLEMENTING** · Gate: `2/8`  
> Scenario: `Spec.Objective.PointSurvivesKO`  
> La partita **puo' finire** per obiettivo, ma in mappa **non esiste ancora un oggetto da attivare**: manca il consumatore, non la regola.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-OBJECTIVE-SYSTEM -->

## Perché servono

Gli obiettivi danno alla partita un motivo per **muoversi e contestare spazio**. Sono particolarmente importanti in un gioco con coperture e Overwatch: senza una pressione esterna, aspettare potrebbe diventare troppo conveniente.

## Interact

Il design di E10.1 prevede che un'unità possa interagire con un oggetto **adiacente** durante il Blast.

Gli oggetti previsti includono:

- porte;
- console;
- ascensori;
- generatori;
- sprinkler;
- ponti;
- obiettivi.

Il fatto che `Action.Interact` esista nel catalogo non significa che tutti questi oggetti siano già giocabili: E10 è ancora assente dalla build corrente.

> **L'azione è una sola.** Fino al 2026-08-10 il catalogo ne spediva due, `Interact` e `Activate`, e questa pagina le nominava entrambe. `Action.Activate` è stata **ritirata**: «attivare un dispositivo» *è* un'interazione, e tenerle separate era una doppia verità senza differenza. Lo Stable ID vecchio non è stato cancellato — resta interpretabile nelle tracce già registrate — ma non è più il nome di un'azione che qualcuno userà.

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
