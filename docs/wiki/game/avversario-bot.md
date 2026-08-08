# L'avversario controllato dal gioco

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-BOT-TACTICAL -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-BOT-TACTICAL` · Release: `v0.2` · Roadmap: `—`  
> Stato: **IDEA** · Gate: `0/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-BOT-TACTICAL -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-BOT-BASE -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-BOT-BASE` · Release: `v0.1` · Roadmap: `E2 · CP 2.6`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `RT_Showcase_Relay_v01`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-BOT-BASE -->

Nella v0.1 giochi **contro il bot**: non c'è multiplayer. Vale la pena sapere come ragiona, perché il gioco è a
informazione incompleta e la prima domanda di ogni giocatore è sempre la stessa — *sta barando?*

## Il bot non vede più di te

Il bot gioca con le stesse regole di percezione degli altri. Non legge le posizioni che non ha osservato, non
conosce i tuoi intenti prima della risoluzione e non consulta lo stato nascosto della partita per decidere.

Questa non è una gentilezza: è un vincolo di progetto. Un avversario che conosce l'informazione che ti è
nascosta renderebbe inutili proprio i sistemi su cui il gioco è costruito — linea di vista, rumore, memoria
dell'ultima posizione nota.

Quando il bot sembra «indovinare» dove sei, di solito ha fatto una di queste cose:

- ti ha **visto** in un round precedente e sta agendo sull'ultima posizione nota;
- ti ha **sentito**: azioni rumorose lasciano una traccia percepibile;
- non sta cercando te ma un **obiettivo**, e tu sei sulla strada.

## Come decide

Il bot valuta le opzioni legali che ha e assegna a ciascuna un punteggio: quanto avvicina a un obiettivo,
quanta copertura offre la cella d'arrivo, quanto espone a una minaccia nota, se un bersaglio è eliminabile
adesso. Poi sceglie il punteggio più alto.

Non simula la partita in avanti e non costruisce un modello di te come giocatore. Fa una valutazione della
situazione presente, con criteri dichiarati.

Due conseguenze pratiche:

- **È prevedibile, e va bene così.** Se capisci cosa premia, puoi ingannarlo: lasciare una via apparentemente
  conveniente che porta in un campo di tiro funziona.
- **A parità di situazione fa la stessa cosa.** Il gioco è deterministico: stesso stato e stesso seed
  producono la stessa partita. Se ripeti un turno e l'esito cambia, è un difetto — vale la pena segnalarlo.

## Cosa non fa ancora

Il bot della v0.1 coordina la squadra in modo elementare: evita di ostacolarsi e di colpirsi, ma non costruisce
giocate a due come faresti tu con un compagno umano. Non tiene una mappa di probabilità su dove *potresti*
essere, e non adatta il proprio stile al tuo.

Sono cose previste per le versioni successive, non promesse mancate della v0.1.

## Se il bot fa una mossa che non capisci

Non è detto che abbia sbagliato: spesso l'obiettivo pesa più di quanto sembri dal tuo punto di vista. Il gioco
può mostrare **perché** ha scelto una cella invece di un'altra, voce per voce del punteggio. È lo stesso
strumento che usiamo per verificare che non stia barando.

---

Vedi anche: [Visibilità, rumore e informazione](visibilita-rumore-e-informazione.md) ·
[Obiettivi e fine partita](obiettivi-e-fine-partita.md) · [Come si gioca](come-si-gioca.md)
