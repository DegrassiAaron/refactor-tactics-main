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

| Banner | Vocabolario | Significa | Quanti *(2026-08-16)* |
|---|---|---|--:|
| `CURRENT` | canonico | Vive: quello che dice vale, salvo verifica sull'owner | 34 |
| `SNAPSHOT` | canonico | Fotografia di una data. Resta qui finché è **l'ultima misura** del suo oggetto | 8 |
| `📦 DELIVERED PLAN` | secondo | *Piano già eseguito, non normativo* — equivale a `HISTORICAL` | 14 |
| `🧱 AS-BUILT` | secondo | *Specifica di ciò che fu consegnato* — equivale a `HISTORICAL` | 7 |
| `DONE` · `PLAN`/consumato · `BRIEF` | secondo | Casi singoli, già consumati — equivalgono a `HISTORICAL` | 3 |

**66 documenti**, `README.md` escluso — ⚠️ **rimisurati dopo il merge**, non incrementati: questa cella e' andata fuori sincrono **tre volte in un giorno** perche' quattro rami hanno toccato la cartella senza vedersi. `34 + 8 + 14 + 7 + 3 = 66`, e la somma delle categorie e' il controllo che il totale da solo non offre. I due totali si rimisurano eseguendo:

```sh
ls docs/roadmap/plans/*.md | grep -v README | wc -l          # 66
ls docs/archive/roadmap-plans/*.md | grep -v README | wc -l  # 20

# La ripartizione per banner — il numero che fino al 2026-08-16 nessun comando produceva.
# Il banner sta nella PRIMA riga di citazione, e i vocabolari sono due.
python -c "import os,sys; d='docs/roadmap/plans'; K=['CURRENT','SNAPSHOT','DELIVERED PLAN','AS-BUILT','HISTORICAL','DONE','BRIEF','PLAN']; c={}; [c.__setitem__(next((k for k in K if k in next((l for l in open(os.path.join(d,f),encoding='utf-8').read(1200).split(chr(10)) if l.startswith('>')),'')),'?'), c.get(next((k for k in K if k in next((l for l in open(os.path.join(d,f),encoding='utf-8').read(1200).split(chr(10)) if l.startswith('>')),'')),'?'),0)+1) for f in sorted(os.listdir(d)) if f.endswith('.md') and f!='README.md']; print(c, chr(183), 'totale', sum(c.values()))"
```

> 🔵 **Rimisurate il 2026-08-16, e tre celle su sette erano ferme** — `53 → 66` documenti,
> `CURRENT 28 → 34`, `SNAPSHOT 1 → 8`. Il difetto è quello che questa sezione già descrive, applicato a sé
> stessa: la cella *dice* di rimisurarsi dopo ogni merge, e i due comandi che offriva davano il **totale**,
> mai la **ripartizione** — quindi cinque numeri su sette non avevano modo di essere verificati. Ora il
> terzo comando c'è, e la prossima deriva è visibile invece che ricordata.
>
> ⏱️ **E lo scarto successivo è dichiarato prima di prodursi**: `docs/menu-frontend-consolidamento`
> aggiunge `menu-frontend-spec-panel-2026-08-16.md` a questa stessa cartella, e nessuno dei due rami vede
> l'altro. Quando atterrano entrambi sarà **67** con `CURRENT 35` — da **rimisurare col comando**, non da
> sommare.

> 🔴 **Questa riga diceva 49, ed è stata falsa per venti minuti.** Il totale è stato scritto su un worktree
> e il merge ha portato `wiki-audit-player-first-2026-08-13.md` da una sessione parallela. È la regola che
> il repository ha già imparato più volte: **un totale scritto a mano si rimisura sull'albero mergiato**,
> non si incrementa e non si copia da prima del merge.
>
> 🟡 **E succede di nuovo oggi — previsto, e poi verificato.** `docs/consolidamento-4-processi` aveva
> scritto `51` sulla propria base e accanto: «#836 aggiunge un piano alla stessa cartella: dopo il merge di
> entrambe sarà **52** e i `CURRENT` **27**». #836 è atterrata mentre quella PR era aperta, e il comando
> eseguito sull'albero unito ha risposto **52** e **27**. La previsione si legge con
> `gh pr list --state open`; il numero si scrive col comando, **dopo**. Le due cose non si sostituiscono.
>
> 🔴 **Quarto giro, e la previsione era giustificata con il comando sbagliato.** `feat/telecamera` aggiunge
> `camera-roadmap-v1-triage-2026-08-14.md`: `52 → 53`, `CURRENT 27 → 28`, rimisurati sull'albero dopo il
> merge di `origin/main`. Ma la prima stesura di questa riga concludeva «nessun altro ramo può portare un
> file in questa cartella» da `gh pr list --state open` **vuota** — e quel comando non vede i branch
> pushati senza PR. Misurati con `git diff --name-only origin/main...origin/<branch>`, **quattro** rami
> vivi aggiungono file proprio qui: `wip/icon-visual-language` **8** (fra cui `roadmap_lane_1..5.md` e
> `roadmap-lane-index.md`), `docs/lane-6-7` **8**, `docs/five-lane-roadmap` **6**, `docs/lane-7-vault`
> **2**. Se uno solo atterrasse, questa cella direbbe `53` con la cartella a `59`.
> **La lezione non è che il numero sia sbagliato — oggi è giusto — ma che la sua garanzia lo era**: una
> previsione si legge con **due** comandi, `gh pr list` *e* `git ls-remote`/`git diff` sui branch remoti.
> È scritto nell'intestazione di [`../parallel-batch.yaml`](../parallel-batch.yaml), ed è stato ignorato
> nel documento che quel file esiste per proteggere. Trovato in code review.

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
