# `superpowers/` — design e piani delle sedute, non regole

> `CURRENT` · **Dichiarato il 2026-09-20** · Owner della IA: [`../README.md`](../README.md)
> §*Le quattro nature di un file*.
> **Cosa è**: la **provenienza** del lavoro fatto in seduta `superpowers` — il design che ha preceduto
> un'implementazione e il piano che l'ha eseguita.
> **Cosa non è**: una fonte. ⛔ **Nessun documento qui è owner di una regola.** Ogni file lo dichiara da sé,
> e il più esplicito lo scrive in prima riga: *«questa spec resta il documento di design: per la regola
> vigente vale il Decision Log, non questo file»*.

## Il criterio è il banner, non la data nel nome

È lo stesso di [`../roadmap/plans/`](../roadmap/plans/README.md), e **non se ne introduce un secondo**: ogni
file porta in testa il proprio stato e nomina l'owner a cui è subordinato. Aprire il file e leggere quella
riga è l'unica cosa da fare per sapere se vale ancora.

⚠️ **La data nel nome è quella della seduta**, non della validità. Un design del 2026-08-28 può essere
`implementato` e un piano del 2026-09-12 può non essere ancora partito: l'ordine cronologico non è un
ordine di maturità.

## Cosa c'è

| Design | Piano | Owner della regola, quando c'è |
|---|---|---|
| [`2026-08-28-scudo-base-e-sorgente-danno-design.md`](specs/2026-08-28-scudo-base-e-sorgente-danno-design.md) | [`2026-08-28-scudo-base-e-sorgente-danno.md`](plans/2026-08-28-scudo-base-e-sorgente-danno.md) | **`D-224`** nel [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) |
| [`2026-08-31-coordinate-cella-pavimento-design.md`](specs/2026-08-31-coordinate-cella-pavimento-design.md) | [`2026-08-31-coordinate-cella-pavimento.md`](plans/2026-08-31-coordinate-cella-pavimento.md) | issue **#1920**, epic **#1861** *(Map Editor 0.1)* |
| [`2026-09-11-hud-otto-zone-blockout-design.md`](specs/2026-09-11-hud-otto-zone-blockout-design.md) | [`2026-09-12-hud-otto-zone-blockout.md`](plans/2026-09-12-hud-otto-zone-blockout.md) | [`guida-screen-hud-umg.md`](../technical/runbooks/guida-screen-hud-umg.md) §3 — che il design dichiara **contratto corrente finché non atterra** |
| [`2026-09-04-vita-e-status-sopra-unita-design.md`](specs/2026-09-04-vita-e-status-sopra-unita-design.md) | — *(nessuno: è un brief di brainstorming)* | le issue che il brief ha aperto, nominate al suo §3.2 |
| [`2026-09-20-conduttore-seduta-pie-design.md`](specs/2026-09-20-conduttore-seduta-pie-design.md) | [`2026-09-20-conduttore-seduta-pie.md`](plans/2026-09-20-conduttore-seduta-pie.md) | issue **#3208**; il design dichiara scoperta la **prima seduta reale** (§4.5) |

> 🔴 **Questa tabella è invecchiata il giorno stesso in cui è stata scritta, ed è l'argomento della
> regola in fondo.** È entrata in `main` il 2026-09-20 con quattro righe; nello stesso giorno la PR **#3219**
> ne ha aggiunta una quinta — la coppia *conduttore di seduta PIE* — senza la riga, perché quel ramo era
> partito **prima** che questo `README` esistesse. ⛔ Non è una svista di chi ha aperto quel ramo: è la
> forma che un indice scritto a mano prende sempre. La riga è stata aggiunta subito dopo; la prossima
> toccherà a chi aggiunge il file.

⚠️ **Il brief del 2026-09-04 non ha un piano, e non è una lacuna**: è un brief `/sc:brainstorm`, cioè il gradino *prima*
del design. La colonna vuota dice che quella seduta si è fermata alle scelte, non che manchi un documento.

## ⚠️ Due case per la stessa categoria, dichiarato e non risolto

Questa cartella e [`../roadmap/plans/`](../roadmap/plans/README.md) contengono **lo stesso genere di
artefatto** — un piano consegnato, conservato per provenienza, subordinato a un owner altrove. A separarle
non è la natura del documento: è **chi lo ha prodotto**, cioè un dettaglio di tooling.

🔑 È la stessa forma di difetto che [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165)
apre citando `roadmap/plans/` e `archive/roadmap-plans/` — *«due case per la stessa categoria, e nessuno dei
due nomi dice quale»*. Qui il nome lo dice, quindi il costo è minore; ma la domanda *«dove cerco un piano?»*
ha ancora due risposte.

⛔ **Non si unificano qui.** Spostare questi file dentro `roadmap/plans/` è un `git mv` che riscrive
riferimenti in documenti lavorati in parallelo, e il beneficio — una casa invece di due — non è così grande
da farlo di passaggio. Si dichiara, così che chi cerca sappia di dover guardare in due posti, e si decide
quando qualcuno possiede la domanda.

## Se aggiungi un file qui

1. **un banner in testa**, con lo stato e l'owner a cui il documento è subordinato — senza, il file non è
   classificabile e la tabella sopra non può ospitarlo;
2. **una riga in quella tabella**, con il piano accanto al design o la colonna vuota dichiarata;
3. ⛔ **nessuna regola nuova**: se una seduta produce una decisione, quella va nel
   [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) o in un owner di
   [`../gameplay/`](../gameplay/) · [`../technical/`](../technical/). Questo resta il *come ci si è
   arrivati*.
