# La generazione v0.1 delle immagini Wiki — superata, non pubblicabile

> `HISTORICAL` · **Riordinata**: 2026-08-17 · **Autorità**: nessuna.
> Il clone pubblica la **v0.2**: altri nomi, altri contenuti, **zero immagini in comune** — verificato per
> hash. Niente di ciò che sta qui è in una pagina, e niente ci entrerà nella forma attuale.

Questa cartella si chiamava `to consolidate`, e il nome prometteva una coda di lavoro. Non lo era: è
l'archivio di **due generazioni di materiale già superate**, arrivato qui il 2026-08-17 da `docs/wiki/`.

## Le due sottocartelle rispondono a una domanda sola

| | Quante | Cosa sono |
|---|---|---|
| [`roster-neutro/`](roster-neutro/) | 12 | nessun nome di personaggio nei pixel: mostrano regole, fasi, mappa, reazioni |
| [`roster-legacy/`](roster-legacy/) | 24 | 🔴 mostrano nomi di personaggio che **nessun documento corrente usa più** |

Il criterio è verificabile e non opinabile: **ogni immagine è stata aperta**. Le 24 di `roster-legacy/`
portano i nomi nei pixel, quindi nessun rename le sana — l'unica chiusura possibile è rigenerarle. Le 12 di
`roster-neutro/` non hanno quel problema: restano datate, ma non contraddicono il canone sui nomi.

⚠️ **`roster-neutro/` non significa «pubblicabile».** Significa solo che il blocco dei nomi non si applica.
Il clone pubblica già la v0.2 degli stessi argomenti.

## Quali nomi, e perché non si riscrivono qui

I nomi in questione sono i quattro legacy rimossi da
[D-130](../../../decisions/RT_PDR_00_Decision_Log.md), più due quartetti ancora precedenti. Il censimento
completo di quelle generazioni **ha già un owner** — la riga `Roster` di
[`../../../DOC_CONFLICT_MATRIX.md`](../../../DOC_CONFLICT_MATRIX.md), che le dichiara `SUPERSEDED` verso il
roster canonico di [`../../../characters/index.md`](../../../characters/index.md). Ripeterlo qui creerebbe
la seconda copia di un elenco che diverge alla prima modifica.

🔴 **Le immagini si contraddicono anche sul formato**, e questo nessun documento lo registrava: fra i
poster convivono `2v2`, `3v3` e `4v4` come «formato core». Il canone è **2v2** offline vs bot
([`../../../../CLAUDE.md`](../../../../CLAUDE.md) §2). Due poster su tre sbagliano, e non è un refuso di
stampa: descrivono progetti diversi.

## L'indice del set numerato, restaurato

Le tavole `01`–`14` sono il pacchetto `RefactorTactics_Wiki_Infographics_v0.1`. Il suo README **esisteva**
ed è stato perso nel commit `273c76a6`, che ha spostato i file da `docs/wiki/` a `docs/src/` senza
portarselo dietro. Eccolo, dalla storia:

| # | Tavola | Dove sta ora |
|---|---|---|
| 01 | RefactorTactics in 60 secondi | `roster-legacy/` |
| 02 | Anatomia di un turno | `roster-neutro/` |
| 03 | Azioni universali & Action Economy | `roster-neutro/` |
| 04 | Famiglie di azioni | `roster-neutro/` |
| 05 | Reazioni & Decision Boundary | `roster-neutro/` |
| 06 | La mappa è un'arma | `roster-neutro/` |
| 07 | Combo ambientali | `roster-legacy/` |
| 08 | Facing, LOS, Cover & Percezione | `roster-neutro/` |
| 09 | Planning & coordinazione di squadra | `roster-neutro/` |
| 10–13 | le quattro schede eroe | `roster-legacy/` |
| 14 | TurnLog & determinismo | `roster-legacy/` |

⚠️ **Il README originale diceva «basato sul canone corrente della v0.1», ed è la frase che invecchia
peggio.** La bonifica di D-130 lo ha riscritto — le voci `10`–`13` nominano i quattro eroi canonici — ma
**le immagini non sono state rigenerate**: i file mostrano ancora i nomi vecchi. La prosa era conforme e i
pixel no, e poiché il README è stato cancellato subito dopo, la divergenza è rimasta senza testimoni per
quattro giorni. È la ragione per cui questa tabella dichiara *dove sta* ogni tavola invece di dichiararla
valida.

## Lo zip esiste, ma non è nel repository — e non è stato toccato

`docs/src/RefactorTactics_Wiki_Infographics_v0.1.zip` contiene i **14 PNG byte-identici** a quelli qui,
verificato hash per hash: zero byte unici di immagine. Porta però una cosa sua, e vale la pena saperlo
prima di riaprirlo: la versione del README **precedente alla bonifica di D-130**, con i nomi legacy ancora
scritti dentro.

⚠️ **È un file locale, non un file del repository**: `.gitignore` esclude `docs/src/*.zip`, quindi il
perimetro *«ovunque»* di D-130 non lo raggiunge e nessuna misura fatta con `git ls-files` lo vede. Resta
dov'è. Se un giorno quel pacchetto dovesse essere riaperto o ridistribuito, il suo README va rigenerato
prima — è l'ultima copia del testo che D-130 ha rimosso da tutto il resto.

> 🔑 **`docs/src/` qui è deliberato, e non è un residuo del rename** *(rivisto il 2026-08-25,
> [#1232](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1232))*. La cartella versionata si
> chiama `docs/research/` dal 2026-08-19, ma le due righe `docs/src/**/*.zip` e `docs/src/*.zip` sono
> ancora in `.gitignore` **apposta**, e il commento accanto dice perché: *«la cartella non ha più file
> versionati, ma il checkout dell'autore ci tiene ancora un export locale»*. Questa è l'unica menzione
> **al presente** sopravvissuta in `docs/research/`, e sopravvive perché è **verificabile e vera** — non
> perché è sfuggita.
