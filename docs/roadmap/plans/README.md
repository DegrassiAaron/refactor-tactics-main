# Piani, referti e triage — come si legge questa cartella

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-14
> **Cosa è**: l'indice del **criterio**, non dei contenuti. Dice come capire, aprendo un file di questa
> cartella, se quello che afferma vale ancora.
> **Cosa non è**: una fonte di stato. Nessun documento qui è owner di qualcosa — gli owner sono
> [`../../decisions/`](../../decisions/RT_PDR_00_Decision_Log.md), [`../feature-registry.yaml`](../feature-registry.yaml),
> [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) e [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

## Il criterio è il banner, non la data

Ogni file qui porta un **banner di stato** nella prima riga dopo il titolo. È l'unica cosa da guardare.

> 🔴 **La data nel nome non è un criterio, ed è la trappola che questa cartella tende.** Misurato il
> 2026-08-14: **68** documenti su 69 portavano una data, ma solo **35** ce l'avevano nel *nome*. Chi avesse
> archiviato «i piani datati» ne avrebbe presi 35 su 68 — e di quei 35, **22 si dichiaravano `CURRENT`**.
> Un criterio che sbaglia in entrambi i versi contemporaneamente.

## Le due lingue, e cosa significano

⚠️ **I banner parlano due vocabolari.** Non è un difetto da sanare riscrivendo 24 documenti — è un fatto da
dichiarare, perché la mappa serve a chi legge e la riscrittura no.

| Banner | Vocabolario | Significa | Quanti *(2026-08-14)* |
|---|---|---|--:|
| `CURRENT` | canonico | Vive: quello che dice vale, salvo verifica sull'owner | 26 |
| `SNAPSHOT` | canonico | Fotografia di una data. Resta qui finché è **l'ultima misura** del suo oggetto | 1 |
| `📦 DELIVERED PLAN` | secondo | *Piano già eseguito, non normativo* — equivale a `HISTORICAL` | 14 |
| `🧱 AS-BUILT` | secondo | *Specifica di ciò che fu consegnato* — equivale a `HISTORICAL` | 7 |
| `DONE` · `PLAN`/consumato · `BRIEF` | secondo | Casi singoli, già consumati — equivalgono a `HISTORICAL` | 3 |

**51 documenti**, `README.md` escluso — ⚠️ **rimisurati dopo il merge**, non incrementati: questa cella e' andata fuori sincrono **due volte in un giorno** perche' tre rami hanno toccato la cartella senza vedersi. `26 + 1 + 14 + 7 + 3 = 51`, e la somma delle categorie e' il controllo che il totale da solo non offre. I due totali si rimisurano eseguendo:

```sh
ls docs/roadmap/plans/*.md | grep -v README | wc -l          # 51
ls docs/archive/roadmap-plans/*.md | grep -v README | wc -l  # 20
```

> 🔴 **Questa riga diceva 49, ed è stata falsa per venti minuti.** Il totale è stato scritto su un worktree
> e il merge ha portato `wiki-audit-player-first-2026-08-13.md` da una sessione parallela. È la regola che
> il repository ha già imparato più volte: **un totale scritto a mano si rimisura sull'albero mergiato**,
> non si incrementa e non si copia da prima del merge.

✅ **Nessun `HISTORICAL` canonico resta qui**: dal 2026-08-14 vivono tutti in
[`../../archive/roadmap-plans/`](../../archive/roadmap-plans/README.md).

⚠️ **Uno `SNAPSHOT` invece può restare, e la prima stesura di questo README lo negava.** Diceva *«nessuno
`SNAPSHOT` canonico resta qui»*, e l'arrivo di un audit Wiki **appena scritto** l'ha smentita in giornata.
La regola giusta è più stretta: `SNAPSHOT` significa *«sono una fotografia»*, non *«archiviami»*. Una
fotografia **fresca** è la misura più recente del suo oggetto e serve dov'è; si archivia quando **una misura
più nuova la sostituisce**, o quando ciò che fotografava non esiste più.

⚠️ **Ma 24 documenti restano storici nel secondo vocabolario**, ed è un arretrato **dichiarato**, non
nascosto: `DELIVERED PLAN` e `AS-BUILT` significano *già consegnato*, quindi per contenuto appartengono
all'archivio. Non sono stati spostati in questo giro perché l'archiviazione del 2026-08-14 ha applicato il
criterio del **banner canonico**, e mescolarci una traduzione di vocabolario avrebbe reso una
riorganizzazione meccanica una revisione di 24 documenti. Sono il lotto successivo, e sono contati.

**Un banner `CURRENT` non è una garanzia**: è una dichiarazione fatta il giorno in cui il file è stato
scritto. Vale finché l'owner non lo smentisce, e l'owner ha sempre ragione.

## Quando un piano si archivia

In [`../../archive/roadmap-plans/`](../../archive/roadmap-plans/README.md), e **solo** quando il suo banner
dice già:

- **`HISTORICAL`** — sempre. È già dichiarato superato o atterrato altrove.
- **`SNAPSHOT`** — *quando una misura più recente lo sostituisce*, non per il fatto di essere una
  fotografia. Un audit scritto ieri è ancora l'unica misura del suo oggetto.

> ⚠️ **Non si archivia riscrivendo un banner.** Se un documento dice `CURRENT` e lo si vuole archiviare,
> prima si dimostra che non è più corrente — e quella è una decisione, che ha un altro posto
> ([`../../decisions/`](../../decisions/RT_PDR_00_Decision_Log.md)). Archiviare è una riorganizzazione;
> dichiarare superato è un'affermazione. Confonderle è il modo in cui un archivio comincia a mentire.

I 23 piani `CURRENT` che hanno una data nel nome **restano qui apposta**: diversi sono l'istruttoria che
`OPEN_DECISIONS.md` e il Decision Log citano per decisioni ancora aperte. Spostarli non li renderebbe più
vecchi — allungherebbe solo il percorso per raggiungerli.

## Un piano non è un owner

Se un piano e un documento owner si contraddicono, **vince l'owner**, sempre, anche se il piano è più
recente. Un piano descrive cosa si voleva fare; l'owner descrive cosa vale.
