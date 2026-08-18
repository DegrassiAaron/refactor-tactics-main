# `docs/src/wiki/` — gli originali degli asset, nessuna pagina

> `CURRENT` · **Riordinata**: 2026-08-17 · **Autorità**: nessuna. `docs/src/` è *input non ancora
> consumato* ([`../../../CLAUDE.md`](../../../CLAUDE.md) §1): qui stanno immagini, mai regole.

Le pagine della Wiki non vivono qui e non vivono nel repository: la fonte è il clone pubblicato
`refactor-tactics-main.wiki` ([D-076](../../decisions/RT_PDR_00_Decision_Log.md)). Questa cartella tiene gli
**originali** da cui quelle pagine pescano, più il materiale che non è mai stato pubblicato.

## Cosa c'è

| Cartella | Contenuto | Nel clone |
|---|---|---|
| [`facing/`](facing/README.md) | i sette diagrammi del Facing | ✅ tutti e sette |
| [`fazioni/`](fazioni/README.md) | i quattro dossier di fazione | ❌ nessuno |
| [`bot/`](bot/README.md) | i quattro pannelli del Bot AI | ❌ nessuno |
| `graybox/` | infografica pubblica e UML del GrayToolkit | ✅ entrambe |
| [`v0.1/`](v0.1/README.md) | la generazione v0.1 e le precedenti, superate | ❌ nessuno |
| `esempio-pagina-dettaglio.png` | mockup di layout: una infografica grande, poi testo breve e link | ❌ |

`graybox/` non ha un README **perché ce l'ha già**: [`../../archive/src/graytoolkit/README.md`](../../archive/src/graytoolkit/README.md)
dice cosa conteneva il bundle, cosa è entrato nel canone e cosa no. Scriverlo di nuovo qui creerebbe la
seconda copia di una spiegazione che diverge alla prima modifica.

## Il confronto si fa per hash, e il 2026-08-17 ha tolto dodici file

Misura su `docs/src` (356 PNG) contro il clone (69 PNG), poi di `docs/src/wiki` contro sé stessa.
**Dodici file erano byte-identici a un altro file già presente altrove** e sono stati rimossi:

| Quanti | Cosa erano | Dove resta l'originale |
|---|---|---|
| 5 | icone e overview delle fazioni | il clone, in `images/factions/` |
| 2 | le due immagini del GrayToolkit dentro il bundle sorgente | `graybox/` e il clone |
| 4 | rendering già presenti in questa cartella sotto un secondo nome generico | `v0.1/roster-neutro/` |
| 1 | la guida rapida, tenuta sia in `facing/` sia nella cartella v0.1 | `v0.1/roster-legacy/` |

Un tredicesimo è emerso fuori da questa cartella, nella stessa misura: `design/hud/hud-style.png` era
byte-identico a `design/ui/UI-style-guide.png`. Aprendolo, l'immagine si intitola **«UI STYLE GUIDE»** —
quindi dei due nomi quello sbagliato era il primo, ed è quello caduto.

Le cinque immagini di fazione sono la **seconda** volta che quel duplicato si forma: la prima è registrata
in [`../../wiki/README.md`](../../wiki/README.md), che le aveva già tolte da `docs/wiki/` per la stessa ragione. La terza copia
delle due immagini GrayToolkit era **segnalata e non rimossa** dall'archivio del bundle, che si fermava
davanti a `docs/src/` chiamandola *«la cartella d'ingresso dell'autore»*: quella segnalazione ora è chiusa.

⚠️ **I sette diagrammi di `facing/` e le due di `graybox/` sono anch'essi identici al clone e restano.**
Non è una dimenticanza: sono la copia di riferimento, e il loro README dichiara quale file del clone
corrisponde a quale. Un duplicato dichiarato è un indice; un duplicato taciuto è un bivio.

## Il nome ha mentito due volte, in due cartelle diverse

In `facing/` due diagrammi portavano il nome l'uno dell'altro; in `bot/` i quattro pannelli erano numerati
in un ordine che non era il loro. Entrambi i casi sono stati **verificati aprendo le immagini**, corretti, e
documentati nel README della rispettiva cartella. Nessun controllo automatico li avrebbe visti: gli hash
erano tutti giusti.

## Cosa non è pubblicabile, e perché

[D-130](../../decisions/RT_PDR_00_Decision_Log.md) ha rimosso dal repository i quattro nomi legacy del
roster; il roster canonico è quello di [D-120](../../decisions/RT_PDR_00_Decision_Log.md), fissato in
[`../../characters/index.md`](../../characters/index.md). **Quei nomi sono stampati nei pixel** di 26
immagini fra `v0.1/roster-legacy/` (24) e `fazioni/` (2), e nessun rename li toglie da lì.

Sono separate dalle altre e marcate nei due README di cartella. Rigenerarle è lavoro aperto: finché non
accade, non entrano in una pagina.
