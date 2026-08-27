# Piani, referti e triage — come si legge questa cartella

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-14
> **Cosa è**: l'indice del **criterio**, non dei contenuti. Dice come capire, aprendo un file di questa
> cartella, se quello che afferma vale ancora.
> **Cosa non è**: una fonte di stato. Nessun documento qui è owner di qualcosa — gli owner sono
> [`../../decisions/`](../../decisions/RT_PDR_00_Decision_Log.md), `../feature-registry.yaml`,
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

<!-- RT_PIANI_BANNER:BEGIN -->

| Banner | Significa | Quanti |
|---|---|--:|
| `CURRENT` | Vive: quello che dice vale, salvo verifica sull'owner | 43 |
| `SNAPSHOT` | Fotografia di una data. **Resta qui** finche' e' l'ultima misura del suo oggetto | 6 |
| `(nessun banner)` | Apre senza dichiararne uno: **un fatto, non un errore di formattazione** | 4 |
| `PLAN` | Piano gia' **consumato**: resta per la provenienza di cio' che ha prodotto | 1 |
| **totale** | | **54** |

In [`../../archive/roadmap-plans/`](../../archive/roadmap-plans/) ce ne sono **43**: quelli che il banner dichiarava gia' storici.

<!-- RT_PIANI_BANNER:END -->

> 🔴 **Rimisurata il 2026-08-26, e la deriva era di uno prima ancora che questo giro aggiungesse un file.**
> La cella diceva `CURRENT 41 · SNAPSHOT 6 · nessun banner 3 · PLAN 1 · totale 51`; la cartella ne conteneva
> **52**, con `nessun banner` a **4**. Gli addendi sono stati **riletti dai banner sul disco**, non
> incrementati — è la contromisura che questa pagina prescrive da nove voci — e col
> [`action-phases-economy-handoff-2026-08-26.md`](action-phases-economy-handoff-2026-08-26.md) di oggi
> diventano `CURRENT` **42** · `SNAPSHOT` 6 · nessun banner **4** · `PLAN` 1 = **53**, e la somma torna col
> totale contato a parte. L'archivio resta **43**: questo giro non ci ha spostato niente — il documento che
> supera è un **sorgente**, e i sorgenti vanno in
> [`../../archive/src/handoff/`](../../archive/src/README.md), che ha un conteggio suo.
>
> ⚠️ **Da `D-182` nessun `--check` può più dirlo**, e infatti nessuno l'ha detto per cinque giorni: la deriva
> su `nessun banner` risale a prima del 2026-08-25, quando l'ultima rimisura sistemò tre numeri su quattro e
> lasciò il quarto. Un addendo fermo non si vede finché qualcuno non conta la cartella intera.

> ⚠️ **Riletta dal disco il 2026-08-27, e aggiornata A MANO.** Il
> [`menu-play-mappa-scenario-spec-panel-2026-08-27.md`](menu-play-mappa-scenario-spec-panel-2026-08-27.md)
> porta `CURRENT` a **43** e il totale a **54**; gli altri tre addendi sono stati **ricontati sui banner**,
> non lasciati fermi, e non si sono mossi (`SNAPSHOT` 6 · nessun banner 4 · `PLAN` 1). L'archivio resta
> **43**, verificato escludendo il suo `README.md` — la cartella contiene 44 file.
>
> 🔴 **`scripts/docs_inventory.py` non è nel working tree**, quindi il comando che la nota qui sotto
> prescrive non è eseguibile e il divieto di scrivere dentro i marcatori non ha più un generatore che lo
> faccia rispettare. ⚠️ Il clone di questa sessione è **shallow** (237 commit), quindi *«lo script non
> esiste più»* è ciò che non si può concludere: qui si registra soltanto che **da questo albero non si
> rigenera**, e che la cella è stata scritta a mano leggendo i banner, come la pagina prescrive.

> 🔵 **Dal 2026-08-19 questa tabella è generata**, e le note qui sotto restano perché la storia di
> come ci si è arrivati vale più del numero. La cella è andata fuori sincrono **tre volte in un
> giorno**, e il 2026-08-19 si contraddiceva ancora: diceva `79` in tabella e `# 75` nel comando in
> fondo, con **1** documento senza banner dove i file ne avevano **2**. Il totale tornava lo stesso —
> ed è il caso peggiore, perché una somma giusta fa credere corretti gli addendi.
>
> Il banner sta nei file, quindi si legge dai file:
> `python scripts/docs_inventory.py --emit-plans`, e `--check` fallisce se qualcuno lo riscrive a
> mano. ⚠️ **Dentro i due marcatori non si scrive**: rigenerare cancella, e cancella in silenzio.

> ⌫ **E dal 2026-08-21 non è più generata.** `scripts/` è uscita dal repository con
> [D-182](../../decisions/RT_PDR_00_Decision_Log.md), `docs_inventory.py` compreso: il comando qui sopra
> non esiste, e `--check` non fallisce più perché non c'è. I due marcatori restano — sono l'unico posto in
> cui questi numeri stanno — ma dentro **ora si scrive a mano**, e l'unico oracolo rimasto è il blocco
> Python in fondo a questa pagina, che non dipende da niente di rimosso.
>
> 🔴 **Rimisurata il 2026-08-25, e tre numeri su quattro erano fermi.** La cella diceva `CURRENT 42 ·
> SNAPSHOT 12 · nessun banner 2 · totale 56`; la cartella ne conteneva **57**, con `CURRENT` **41**, senza
> banner **3**, e una **quarta categoria che la tabella non aveva**: `PLAN`, 1. È esattamente il caso che
> il capoverso qui sopra descrive come il peggiore — solo che stavolta nemmeno la somma tornava, e per
> quattro giorni nessun `--check` ha avuto modo di dirlo. Il totale è poi sceso a **51** con la rimozione
> dei sei doppioni delle lane ([#1350](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1350)):
> `SNAPSHOT 12 → 6`, il resto invariato.

> 🔵 **2026-08-19 — 23 documenti hanno raggiunto l'archivio.** Portavano `DELIVERED PLAN`,
> `AS-BUILT`, `DONE` o `BRIEF`, cioè i banner che questa pagina dichiara equivalenti a `HISTORICAL`,
> e il README dell'archivio raccoglie esattamente quelli. I **12 `SNAPSHOT` non si sono mossi**: la
> regola di questa pagina — *resta finché è l'ultima misura del suo oggetto* — è più fine di quella
> scritta là, e fra due owner della stessa regola ha vinto il più preciso.

> 🔵 **Rimisurato il 2026-08-17 dal ramo `docs/consolidamento-skill-plus`, e lo scarto era di uno.** La
> cella diceva `77`; la cartella ne conteneva **78** prima che questo giro aggiungesse
> [`skill-plus-consolidamento-2026-08-17.md`](skill-plus-consolidamento-2026-08-17.md). Il documento in
> più era arrivato da un altro ramo mentre la misura precedente veniva scritta — la deriva che questa
> cella dichiara di aver già subito tre volte in un giorno, e che si ripete ogni volta che qualcuno
> somma invece di contare. Le sei categorie sono state **rilette dai banner sul disco**: `CURRENT`
> 40 → **42**, le altre cinque invariate, e la somma torna col totale contato a parte.

> 🔵 **2026-08-17, e per una volta la misura precedente era esatta.** Il ramo
> `docs/consolidamento-replay-tactical-designer` ha trovato `75` sulla propria base — che è `main` dopo il
> merge di #1108 — e ne aggiunge **due** `CURRENT`:
> [`replay-canonical-intent-triage-2026-08-17.md`](replay-canonical-intent-triage-2026-08-17.md) e
> [`cinque-processi-paralleli-2026-08-17.md`](cinque-processi-paralleli-2026-08-17.md). Le sei categorie
> sono state **rilette dai banner sul disco**, non incrementate: `CURRENT` 38 → **40**, le altre cinque
> invariate, e la somma torna col totale contato a parte. È il controllo che questa cella chiede, applicato
> nel verso giusto — due metodi indipendenti, non uno più un'addizione.

> 🔴 **Rimisurato il 2026-08-17, e lo scarto era di OTTO.** La riga diceva `67` e la cartella ne conteneva
> **74** prima che questo giro ne aggiungesse uno. Non è una deriva nuova: è la stessa che questa cella
> dichiara di aver già subito tre volte in un giorno, e la contromisura scritta — *rimisurare dopo il merge* —
> non era stata eseguita dai rami successivi. Gli addendi sono stati ricalcolati **tutti**, non incrementati:
> `CURRENT` 35 → **38**, `SNAPSHOT` 8 → **12**.
>
> ➕ **E lo script ha prodotto una categoria che la tabella non aveva**: un documento —
> [`cp153b-decision-provider-plan-2026-08-16.md`](cp153b-decision-provider-plan-2026-08-16.md) — apre con
> `> **Per chi esegue:** …` e **nessun banner**. Non è stato riscritto per farlo rientrare in una casella:
> il criterio di questa pagina è che il banner sia l'unica cosa da guardare, e un documento che non ne ha
> uno è un fatto da dichiarare, non un errore di formattazione da nascondere sommandolo altrove. La riga
> nuova esiste perché la somma torni **senza** che nessuno debba indovinare dove sia finito l'ottavo.

I due totali si rimisurano eseguendo:

```sh
ls docs/roadmap/plans/*.md | grep -v README | wc -l          # il totale
ls docs/archive/roadmap-plans/*.md | grep -v README | wc -l  # e quello dell'archivio
# ⚠️ I due `#` non portano piu' un numero: erano commenti scritti a mano accanto al comando
# che li produce, ed erano sbagliati — `75` contro `79` reali. Un numero accanto al suo
# oracolo e' la forma piu' facile da lasciare indietro.

# La ripartizione per banner — il numero che fino al 2026-08-16 nessun comando produceva.
# Il banner sta nella PRIMA riga di citazione del file, e i vocabolari sono due.
python - <<'PY'
import os

CARTELLA = 'docs/roadmap/plans'
BANNER = ['CURRENT', 'SNAPSHOT', 'DELIVERED PLAN', 'AS-BUILT', 'HISTORICAL', 'DONE', 'BRIEF', 'PLAN']

conteggio = {}
for nome in sorted(os.listdir(CARTELLA)):
    if not nome.endswith('.md') or nome == 'README.md':
        continue
    with open(os.path.join(CARTELLA, nome), encoding='utf-8') as fh:
        prima_citazione = next((r for r in fh if r.startswith('>')), '')
    chiave = next((b for b in BANNER if b in prima_citazione), '?')
    conteggio[chiave] = conteggio.get(chiave, 0) + 1

for chiave, quanti in sorted(conteggio.items(), key=lambda kv: -kv[1]):
    print(f'{quanti:3}  {chiave}')
print(f'{sum(conteggio.values()):3}  TOTALE')
PY
```

⚠️ **Un `?` nel conteggio non è rumore: è un file il cui banner non è stato riconosciuto**, e va guardato
invece che ignorato. Il ciclo legge il file **riga per riga** e si ferma alla prima citazione, quindi non
ha un limite di caratteri da sbagliare — una prima stesura di questo blocco leggeva i primi 1200 e avrebbe
classificato `?` qualunque documento con un'intestazione lunga, cioè avrebbe sotto-riportato proprio la
deriva che questa sezione lo aggiunge a misurare.

> 🔵 **Rimisurate il 2026-08-16, e tre celle su sette erano ferme** — `53 → 67` documenti,
> `CURRENT 28 → 35`, `SNAPSHOT 1 → 8`. Il difetto è quello che questa sezione già descrive, applicato a sé
> stessa: la cella *dice* di rimisurarsi dopo ogni merge, e i due comandi che offriva davano il **totale**,
> mai la **ripartizione** — quindi cinque numeri su sette non avevano modo di essere verificati. Ora il
> terzo comando c'è, e la prossima deriva è visibile invece che ricordata.
>
> ⏱️ **E lo scarto era stato dichiarato prima di prodursi, poi verificato.** Il ramo
> `docs/mini01-consolidamento-autobattle` aveva scritto `66` con `CURRENT 34` e accanto: «`docs/menu-frontend-consolidamento`
> aggiunge `menu-frontend-spec-panel-2026-08-16.md` a questa stessa cartella, e nessuno dei due rami vede
> l'altro: quando atterrano entrambi sarà **67** con `CURRENT 35`». #948 è atterrata, e il comando eseguito
> **sull'albero unito** ha risposto esattamente `67` e `35`. Nessuna cella è stata incrementata a mano.

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
> ⌫ **Nessuno dei quattro esiste più** *(2026-08-25)*: `wip/icon-visual-language` è atterrato con
> [#909](https://github.com/DegrassiAaron/refactor-tactics-main/pull/909) il 2026-08-15, gli altri tre
> sono stati **potati** dopo aver misurato che il loro contenuto era già in `main` o in archivio. Il
> conteggio non si muove: la lezione qui sotto sì, ed è l'unica cosa che questo blocco doveva lasciare.
> 🔴 **E uno dei quattro ha fatto proprio ciò che questa riga temeva.** `wip/icon-visual-language` **è**
> atterrato, il giorno dopo che quei sei file erano stati archiviati, e li ha **riportati in questa
> cartella** — in doppione e in una versione più vecchia di quella archiviata, dove sono rimasti **dieci
> giorni** prima che [#1350](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1350) li
> rimuovesse il 2026-08-25. La previsione era giusta, il numero pure; a mancare è stato il controllo
> **dopo** il merge.
> **La lezione non è che il numero sia sbagliato — oggi è giusto — ma che la sua garanzia lo era**: una
> previsione si legge con **due** comandi, `gh pr list` *e* `git ls-remote`/`git diff` sui branch remoti.
> È scritto nell'intestazione di `../parallel-batch.yaml`, ed è stato ignorato
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
