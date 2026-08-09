# Ponti e collegamenti tra layer

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-SPECIAL-TRANSITIONS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-SPECIAL-TRANSITIONS` · Release: `v0.1` · Roadmap: `E9.4`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.MultiLevel`  
> Verificato il `2026-08-09` su `4a3fd20`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-SPECIAL-TRANSITIONS -->

## In breve

Un ponte è un **arco additivo del grafo**: crea un collegamento che senza di lui non esisterebbe, spesso tra layer diversi.

È l'opposto di una porta:

| | Porta | Ponte |
|---|---|---|
| Natura | sottrae un passaggio normale | aggiunge un collegamento |
| Se sparisce | spesso puoi aggirarla | il percorso può non esistere più |
| LOS | può bloccarla | l'arco non cambia da solo la LOS planare |

## Stati del ponte

| Stato | Significato |
|---|---|
| **Active** | collegamento percorribile |
| **Inactive** | non percorribile ma riattivabile |
| **Destroyed** | distrutto, stato terminale |

Un ponte standard usa **integrità 40**.

## Ponte bidirezionale

Un ponte logico può essere serializzato come due archi, uno per ogni direzione, ma il gioco lo tratta come **un solo oggetto**: danneggiarlo o cambiarne lo stato aggiorna entrambi i versi con una sola revisione della mappa.

## Ponte che cade prima del Move

`ModifyArc` risolve nel **Blast**. Il Move successivo vede quindi la nuova topologia.

Se il ponte su cui avevi pianificato di passare non esiste più, il gioco non cerca di “teletrasportarti” sull'altro layer. Se non c'è una via alternativa valida nel path già pianificato, il percorso fallisce o viene fermato secondo le regole di topologia.

## Ponti temporanei

Il modello supporta ponti temporanei creati da `ModifyArc`:

- durata corrente: **2 turni**;
- possono essere **conduttivi**;
- la scadenza è stato della partita, non dell'asset originale della mappa.

## Ponte conduttivo

Un ponte conduttivo può trasportare la propagazione elettrica anche tra layer. Un ponte `Inactive` o `Destroyed` **non conduce**, anche se il dato dichiara la conduttività.

Questo crea una scelta interessante: una scorciatoia verticale può diventare anche una via per una scarica elettrica.

## Cosa deve ricordare il giocatore

- Porta = chiude una strada esistente.
- Ponte = crea una strada che altrimenti non c'è.
- Rompere un ponte può eliminare completamente una rotta.
- Un ponte conduttivo collega anche il rischio elettrico tra livelli.

## Fonti normative

- `docs/gameplay/spec-ponti-cp94.md`
