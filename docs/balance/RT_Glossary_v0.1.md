# RT — Glossario di stati, superfici e ripari v0.1

> **Owner**: [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744), fetta di
> [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) · **Stato**: **bozza, da accettare
> dall'autore**. Le voci le ha scritte chi ha implementato, a partire dalle regole nel codice.
>
> Il **nome** da leggere e l'**effetto in una riga** di ciò che il giocatore incontra sulla mappa: uno stato
> addosso a un'unità, la superficie di una cella, un riparo su un lato.
>
> - **Un testo non porta numeri.** Durata, danno e costo li compone il gioco dai dati: un numero scritto qui
>   invecchierebbe al primo ribilanciamento.
> - **Il testo è quello che il gioco mostrerà**, carattere per carattere, compreso l'apostrofo al posto
>   dell'accento (`piu'`, `e'`), che è la convenzione del testo a schermo del progetto.
> - **Le copie sono due**, questa e le tabelle in
>   [`RTGlossary.cpp`](../../Source/RefactorTactics/UI/RTGlossary.cpp). Il radar le confronta, e non dice quale
>   delle due correggere:
>
>   ```sh
>   node tools/radar/glossary.ts --check   # exit 1 se un nome o un effetto divergono
>   ```
>
> - **Niente resta senza voce**: i test `RefactorTactics.UI.Glossary*` falliscono se uno stato registrato, una
>   superficie o un tipo di riparo non ha una voce, o se una voce non nomina più niente.
>
> ⚠️ **Sorgente senza lettore.** Oggi nessuno mostra queste voci in partita. I lettori sono lo sguardo e la
> scheda dell'[ispettore del mondo](../superpowers/specs/2026-10-13-ispettore-del-mondo-design.md), fette `F2` e
> `F3`.
>
> **Come rivederle**: correggi le colonne *Nome* ed *Effetto*. Il cambio va riportato nelle tabelle C++ nella
> stessa PR; il radar lo pretende. La colonna *Voce* è la chiave della tabella, e non si cambia qui.

## Stati

La chiave è il nome del tag. Le regole stanno in
[`spec-stati-temporanei-cp82.md`](../gameplay/spec-stati-temporanei-cp82.md) e nel resolver.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Status.Root` | Immobilizzato | Non puoi muoverti ne' scattare. |  |
| `Status.Slow` | Rallentato | Ogni cella ti costa di piu': a piedi arrivi meno lontano. |  |
| `Status.Reveal` | Rivelato | Gli avversari vedono il tuo piano del turno, ma non la reazione che hai armato. | oggi nessuna azione lo applica |
| `Status.Exposed` | Esposto | Finche' dura, il primo colpo che ricevi in un turno fa piu' danno. |  |
| `Status.Guarded` | In guardia | Ogni colpo che arriva di fronte perde una parte del danno. Una spinta debole non ti sposta, a meno che tu sia sbilanciato. |  |
| `Status.Marked` | Marchiato | Il prossimo colpo della squadra che ti ha marchiato fa piu' danno, poi il marchio si consuma. |  |
| `Status.Wet` | Bagnato | La scarica lineare di Aevik ti fa piu' danno. Bagnarti spegne le fiamme. | il danno in piu' vale per una sola abilita' |
| `Status.Braced` | Irrigidito | Ogni colpo, da qualunque direzione, perde un po' di danno, e resisti alle spinte finche' non sei sbilanciato. |  |
| `Status.Burning` | In fiamme | Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne. |  |
| `Status.Obscured` | Nel fumo | Sei nel fumo: attraverso il fumo si mira solo da vicino, in entrambi i versi. |  |
| `Status.Electrified` | Folgorato | L'elettricita' corsa nel terreno ti ha colpito: sulla cella mirata, oppure lungo acqua e metallo. | non resta addosso: e' l'istante di una scarica |
| `Status.Unbalanced` | Sbilanciato | Non puoi correre, e guardia e irrigidimento non ti tengono fermo; spinte, trazioni e scivolate ti portano piu' lontano, e una spinta o una trazione che ti sposta ti butta a terra. |  |
| `Status.Prone` | A terra | Cadendo perdi cio' che avevi armato, e le tue reazioni non scattano. Rialzarti ti costa una parte del movimento. |  |
| `Status.Stunned` | Stordito | Non puoi usare l'azione principale, le tue reazioni non scattano, e perdi cio' che avevi gia' armato. | oggi nessuna azione lo applica |

## Superfici

La chiave è `Terrain.` più il nome del valore di `ERTHexSurface`: la stessa causa che il TurnLog scrive per un
danno da terreno. Le regole stanno in [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) §1.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Terrain.Floor` | Pavimento | Nessun effetto proprio. Puo' prendere fuoco. |  |
| `Terrain.ShallowWater` | Acqua bassa | Costa piu' movimento e conduce l'elettricita'. Chi ci sta dentro e' bagnato finche' ci resta. |  |
| `Terrain.Rough` | Accidentato | Costa piu' movimento, e non lo si attraversa ne' di scatto ne' in carica. Puo' prendere fuoco. |  |
| `Terrain.Fire` | Fuoco | Costa piu' movimento. Chi ci entra, o ci si trova quando divampa, subisce danno e prende fuoco. |  |
| `Terrain.Conductive` | Superficie metallica | Conduce l'elettricita'. |  |
| `Terrain.Ice` | Ghiaccio | Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato. | il catalogo dei terreni dice «all'ingresso»: il codice fa scivolare chi vi TERMINA il movimento, se gli resta piu' di un passo |
| `Terrain.Void` | Vuoto | Non e' un punto d'atterraggio: chi cade non si ferma qui. | non ha una riga nel catalogo dei terreni; il passo lo blocca la cella, non la superficie |
| `Terrain.Smoke` | Fumo | Chi ci sta dentro e' nel fumo: attraverso il fumo si mira solo da vicino. |  |
| `Terrain.HighGround` | Quota elevata | Nessun effetto proprio: indica una posizione rialzata, e il vantaggio viene dal piano della cella. | la mappa dimostrativa la mette anche su celle del piano terra: li' non indica niente |

## Ripari

La chiave è l'id di [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) §3. Un riparo sta su un **lato** della
cella, e vale per ciò che attraversa quel lato.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Structure.LowCover` | Copertura bassa | Riduce il danno dei colpi mirati che ti arrivano di fronte attraverso quel lato; non ripara dalle esplosioni. Non ferma la vista ne' il passo. | il catalogo dei terreni dice «blocca l'arco»: nel codice solo la copertura alta ferma il passo |
| `Structure.HighCover` | Copertura alta | Chiude quel lato: non passano la vista, il passo e i colpi. |  |
