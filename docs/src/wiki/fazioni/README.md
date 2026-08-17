# I quattro dossier di fazione

> `CURRENT` · **Spostati qui**: 2026-08-17 da `docs/src/design/fazioni/`, dove i nomi portavano un prefisso
> in parentesi quadre (`[CON]`, `[CNS]`, `[SED]`, `[RES]`) che nessun documento del repository usava e che
> rompe la sintassi delle immagini in Markdown. **Autorità**: nessuna — sono artefatti di design.

Ogni file è un dossier a piena pagina: i due personaggi della fazione, ruolo, arma, focus delle abilità,
palette e motto. Sono le immagini che un handoff del 2026-08-17 chiedeva di pubblicare come *faction
overview*; due su quattro non possono esserlo, e il perché è sotto.

## Cosa contiene ciascuna

Verificato aprendo i file, non deducendo dal nome.

| File | Fazione | Personaggi mostrati | Versione stampata | Palette primaria |
|---|---|---|---|---|
| `sentinel-directorate-dossier.png` | Sentinel Directorate | Steel · Murdock | `v0.2` | `#356FF6` |
| `resonance-dossier.png` | Resonance | Aurora · Kwang | `v0.2` | `#725CF4` |
| `conflux-dossier.png` | Conflux | 🔴 i due nomi legacy | `v0.1` | `#16C7B7` |
| `constrine-dossier.png` | Constrine | 🔴 i due nomi legacy | `v0.1` | `#D7A83E` |

✅ **Le quattro palette coincidono** con i `primaryColor` che la pagina `Fazioni` del clone pubblica, e le
quattro palette secondarie sono dichiarate nell'immagine stessa. È l'unica parte di questi dossier che si
può citare senza riserve.

## 🔴 Due dossier su quattro non sono pubblicabili

`conflux-dossier.png` e `constrine-dossier.png` mostrano, a caratteri cubitali e in ogni scheda abilità, i
quattro nomi che [D-130](../../../decisions/RT_PDR_00_Decision_Log.md) ha rimosso dal repository — quelli
che [D-120](../../../decisions/RT_PDR_00_Decision_Log.md) aveva già declassato il giorno prima. Il roster
canonico sta in [`../../../characters/index.md`](../../../characters/index.md).

⚠️ **Nessun rename risolve questo caso.** Nelle altre parti del repository D-130 si applica riscrivendo del
testo; qui il testo è nei **pixel**. L'unica chiusura possibile è rigenerare le due immagini con i nomi
canonici, ed è lavoro aperto: finché non accade, questi due file restano provenienza e non materiale
editoriale.

⚠️ **La versione stampata lo conferma e non è un dettaglio grafico**: i due dossier bloccati dichiarano
`v0.1`, i due validi `v0.2`. Non è una coincidenza — sono di due generazioni diverse, e la linea che le
separa è la stessa che separa il roster legacy da quello canonico.

## Cosa è rimasto in `design/`

I quattro `*.icon.png` di [`../../design/fazioni/`](../../design/fazioni/) **non** sono le versioni piccole di
questi dossier: sono fogli di esplorazione dell'emblema, sei varianti `A`–`F` per fazione, senza personaggi.
Sono materiale di design puro e non hanno il problema del roster. Restano dove sono perché è lì che
servono.

Le icone compatte che la Wiki usa davvero sono nel clone, in `images/factions/` — cinque PNG che questa
cartella conteneva in copia identica fino al 2026-08-17, quando sono stati rimossi come duplicati
([`../README.md`](../README.md)).
