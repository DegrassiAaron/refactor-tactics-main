# GitHub Epic / Issue live reconciliation — spec panel

> `CURRENT` · **Stato**: revisione chiusa · **16 mutazioni GitHub eseguite su autorizzazione** (`A1` ×12 ·
> `A3` ×1 · `A4` ×1 · `A2` ×2) · **Data**: 2026-08-31
> **HEAD della revisione**: `4396d2b2` (detached, **20 commit dietro** `origin/main` `ef0414bc`, 0 avanti).
> ⚠️ **E l’HEAD si è mosso durante la sessione**: un’altra sessione ha creato
> `docs/referti-spec-panel-2026-08-31` e committato questi file in `58e76cd7`, catturando lo stato a
> **15** mutazioni — una prima della seconda correzione a `#14`. Il working tree è avanti di quel passo,
> e non è stato committato: `CLAUDE.md` §9. Chi apre la PR da quel branch **rilegga il diff non
> committato**, o il referto in cronologia dichiarerà una mutazione in meno di quelle applicate.
> I documenti citati sono letti con `git show origin/main:<path>`, **non** dal working tree: l'albero è
> condiviso e indietro. GitHub è misurato live via `gh`, quindi non risente della divergenza.
> **Panel**: Wiegers (falsificabilità dei requisiti) · Cockburn (attore e obiettivo) · Nygard (che succede
> se lo applichi) · Adzic (esempio concreto) · Fowler (un solo owner). **Modo**: `critique`.
>
> **Sorgente revisionata**: il mandato *«REFACTORTACTICS — GITHUB EPIC / ISSUE LIVE RECONCILIATION»*
> (~560 righe), arrivato **inline** come argomento di `/sc:spec-panel`. Non essendo un file, è stato
> **trascritto** verbatim in
> [`../../archive/src/handoff/2026-08-31-github-epic-issue-reconciliation.md`](../../archive/src/handoff/2026-08-31-github-epic-issue-reconciliation.md)
> — ⚠️ è il primo sorgente di quella cartella che **non è verificabile con `diff`**, e il file lo dichiara
> in testa.
>
> **Cosa possiede**: il verdetto misurato sul kit, prescrizione per prescrizione, col comando che l'ha
> trovato. **Cosa non possiede**: autorità. **Zero** issue create, chiuse o riaperte; le **16** modifiche di
> corpo/label sono state applicate solo dopo autorizzazione esplicita, voce per voce (§7).

## 1. Il verdetto in una riga

Il kit è **accurato sui fatti e obsoleto sul lavoro**: quasi tutto ciò che prescrive era già stato fatto —
in parte **sei ore prima** che il kit fosse scritto — e l'unica sua prescrizione originale, il §12 sulla
milestone `#6`, è **falsa e distruttiva**. Il valore vero del kit è un difetto che non nomina: la frase
**«15 gate»** è una condizione di apertura *viva* in **dodici** epic.

## 2. Il conto

| Oggetto | Quanti | Esito |
|---|--:|---|
| Epic ispezionate (corpo integrale) | **47** | tutte le `label:epic` aperte |
| Issue ispezionate (metadati) | **63** | 47 epic + 16 figlie citate dal kit |
| Milestone ispezionate | **17** | 16 aperte + 1 chiusa |
| Ancore GitHub del kit verificate | **29** | ✅ **29 esistono**, zero numeri inventati |
| Seed del kit **confermati** | 8 | kit §4 root · §7 `D-286` · §8 `D-256` · §9 `D-259` · §10 (6 owner) |
| Seed del kit **già superati** dal live | 4 | kit §5 · §7 · §8 · §9 → *questo referto* §4 |
| Prescrizioni **falsificate** | **1** | kit §12, milestone `#6` → *questo referto* §5 |
| Difetti veri trovati | **5** | *questo referto* §6 |
| Mutazioni GitHub eseguite | **16** | `A1` su 12 epic · `A3` su `#775` · `A4` su `#1816` · `A2` su `#14` (×2) |
| Issue create / chiuse / riaperte | **0** | nessuna, in nessun momento |
| Azioni **non** eseguite | 3 | `A5` `A6` `A7` — `AMBIGUOUS` o di merito, §7 |

## 3. Cosa il kit misura correttamente

Verificato uno per uno, non dedotto:

| Seed | Misura live | Esito |
|---|---|---|
| `#14` OPEN, milestone `#6`, label `v0.1`/`epic`/`P0` | identico | ✅ |
| I gate vivi sono `G1`–`G14`, non `G1`–`G15` | il DoD ha `~~**G15**~~` **barrato**, ritirato da `D-181`, numero non riusabile | ✅ |
| `D-256`: 3v3 Standard · 2v2 Skirmish · 4v4+ Operations | Decision Log riga 268, testo identico | ✅ |
| `D-259`: ranked/rating/MMR fuori dalla v1.0 | Decision Log riga 271, chiude `RNK-1` | ✅ |
| `D-286`: promozione v0.1 di `#1834`–`#1838` | tutte e cinque **OPEN**, label `v0.1`, milestone v0.1 | ✅ |
| `#1769` è l'owner camera | 13 figlie `CAM-01`–`CAM-12` + `CAM-11a` mappate su `#1770`–`#1781`, `#1809` | ✅ |
| `#778` è il gate v1.0, `#934` è una epic v0.1 frontend shell | identico | ✅ |
| I sei owner cross-release del §10 | tutti e sei esistono e sono aperti | ✅ |

⚠️ **Un dettaglio che il kit sbaglia in modo innocuo**: `#1861` (Map Editor), `#1881` (Playback) e `#1937`
(Player Event Log) sono elencati fra i *«owner nuovi»* senza numero `E`, e il kit stesso lo dichiara
ammissibile. È corretto: nessuno dei tre ha un `E-number`, e non va inventato.

## 4. `ALREADY DONE` — quattro capitoli su cinque erano già chiusi

È il punto che cambia la natura del lavoro: **il kit è il quarto giro sullo stesso terreno**, e come i tre
precedenti non cita i propri predecessori.

| § kit | Chiede | Chi l'ha già fatto | Esito |
|---|---|---|---|
| §5 | riconciliare la checklist di `#14` | [`crud-epic-issue-mcp-spec-panel-2026-08-30.md`](crud-epic-issue-mcp-spec-panel-2026-08-30.md) | **NO-OP** |
| §7 | correggere i tre mapping `CAM-WP` del Drive | `#1769` stessa, §*«Zero Epic creati»* | **NO-OP** |
| §8 | portare `#333` su `D-256` | `#333`, aggiornata **oggi alle 06:30Z** | **NO-OP** |
| §9 | portare `#777` su `D-259` | `#777`, riconciliata il **2026-08-28** | **NO-OP** |

### 4.1 §5 — `#14` era già riconciliata, e meglio di come il kit chiede

Il kit ordina di *«non assumere che la checklist del 30/08 sia ancora aggiornata»*. È la raccomandazione
giusta, e la misura gli dà ragione **al contrario di come se l'aspetta**: la checklist del 30/08 è esatta.

✅ **Le nove caselle `[x]` corrispondono esattamente alle nove epic `CLOSED`** — `#15` `#17` `#18` `#19`
`#20` `#22` `#23` `#175` `#225` — verificate una per una con `gh issue list --label epic --state closed`.
Zero divergenze di stato. Il giro del 2026-08-30 aveva già aggiunto le quattro figlie mancanti (`#324`,
`#934`, `#952`, `#1408`) e tolto la quinta copia del totale.

### 4.2 §7 — le tre correzioni `CAM-WP` vivono dentro la issue che il kit chiede di riconciliare

Il kit dedica il capitolo più lungo a spiegare che `CAM-WP-01 → E11/E21/E13`, `CAM-WP-05 → E40/E42` e
`CAM-WP-08 → E46` sono mapping sbagliati. **Sono le stesse tre correzioni, con le stesse motivazioni, già
scritte nel corpo di `#1769`** (righe 264, 269, 271 del body), e prima ancora nel referto
[`tactical-camera-consolidamento-spec-panel-2026-08-30.md`](tactical-camera-consolidamento-spec-panel-2026-08-30.md).

✅ Anche l'ultima richiesta del §7 è già soddisfatta: la famiglia `OVL-01`–`OVL-04` (`#1941`–`#1944`) è
elencata sotto `#1769` §*«Famiglia OVL — Semantic Area Overlay»*, con le dipendenze fra le quattro e tre
collisioni di palette dichiarate. **Nessuna seconda epic «Tactical Grid Overlay» esiste.**

### 4.3 §8 — `#333` è stata corretta oggi, e la domanda sul titolo ha già una risposta ragionata

`#333` è stata aggiornata **2026-08-31T06:30:55Z**, ~sei ore prima che questo kit arrivasse. Porta già la
nota `D-256` con la tabella di ciò che è superato, e affronta esplicitamente la domanda del §8 sul rename:

> ⚠️ *Il titolo dice ancora «Formato 4v4 competitivo». Il Drive usa la stessa etichetta nella release ladder,
> quindi non è stato cambiato qui: rinominarla è una decisione da prendere sui due lati insieme.*

Il kit chiede di *«valutare se rinominare»*. La valutazione esiste, è di oggi, e conclude **no, non da un
lato solo**. ⛔ **Rinominare ora eseguirebbe metà di una decisione che il live state ha dichiarato bilaterale.**

### 4.4 §9 — `#777` è l'esempio del lavoro già fatto bene

Il titolo di sezione è **barrato**: `~~Ranked e rating stanno qui, non in E42~~ · **Ranked esce dalla v1.0**`,
e il blocco *«Fuori dalla v1.0»* elenca **esattamente** i cinque elementi del seed `D-259` del kit, incluso
il *«nessun MMR nascosto come sostituto implicito»*. Il perimetro è passato a `#1604`.

## 5. `FALSIFIED` — il §12 è distruttivo, e va disinnescato prima di ogni altra cosa

Il kit ordina, fra i difetti da cercare:

> * `v0.1` issue fuori milestone `#6`;

**La premessa è falsa.** La v0.1 non ha una milestone: ne ha **sette**.

| # | Titolo | Stato | Aperte | Chiuse |
|--:|---|---|--:|--:|
| 1 | `v0.1 · Fondamenta` | **closed** | — | — |
| 2 | `v0.1 · Mondo giocabile` | open | 15 | 15 |
| 3 | `v0.1 · Leggibilità` | open | 31 | 14 |
| 4 | `v0.1 · Percezione e reazioni` | open | 20 | 23 |
| 5 | `v0.1 · Prova integrata` | open | 6 | 8 |
| 6 | `v0.1 · Gate di release` | open | 15 | 26 |
| 7 | `v0.1 · Difetti e bilanciamento` | open | 5 | 36 |

🔴 **Applicare il §12 alla lettera sposterebbe ~92 issue aperte in `#6`**, cancellando una struttura di
release deliberata. Le cinque figlie di `D-286` che il kit stesso chiede di *confermare* (`#1834`–`#1838`)
stanno in `#3` e `#4`: la regola del §12 le dichiarerebbe difettose e la regola del §7 corrette, **nello
stesso documento**.

> **NYGARD**: «Il kit non ha una modalità di fallimento sicura. Le sue regole di correzione automatica non
> distinguono *«la milestone è assente»* da *«la milestone non è quella che mi aspettavo»*, e solo la
> prima è un difetto. La seconda è la struttura.»
>
> **WIEGERS**: «`#6` è un numero preso da un esempio del §4, dove compare come milestone della sola `#14`.
> Il §12 lo promuove a criterio per l'intera release senza che nulla lo autorizzi. È un requisito derivato
> da un'istanza — la classe di difetto che i suoi stessi §2 e §13 vietano.»

## 6. `CURRENT_STALE` — i difetti veri, con l'evidenza

Sono ciò che il kit avrebbe potuto trovare e in parte non nomina.

### 6.1 🔴 «15 gate» è una condizione di apertura viva in dodici epic — ed è stata riscritta **oggi**

È il difetto più sistemico del tracker, e il kit lo sfiora senza vederne la scala: cerca `15 gate` fra i
termini stale, ma lo elenca sotto la voce *Feature Registry/G15*, che è un'altra cosa.

```
$ grep -c "15 gate" <corpo di ogni epic aperta>
#322:2  #323:2  #324:2  #325:2  #326:1  #327:2  #328:2  #329:2
#330:2  #331:2  #332:2  #333:3        →  24 occorrenze, 12 epic
```

La frase non è una nota storica. È una **precondizione operativa**:

> ⚠️ *Non si apre prima dei **15 gate** della v0.1.*

I gate vivi sono **quattordici**. E il difetto si sta riproducendo: la nota `D-256` scritta in `#333`
**oggi alle 06:30Z** contiene *«il vincolo dei **15 gate** della v0.1 non è revocato»*.

> **ADZIC**: «Il numero è verificabile e sbagliato: il DoD elenca `G1`–`G14` più una riga barrata. Un
> autore che legge `#325` per sapere quando aprire E24 conta quindici gate e ne cerca uno che non esiste.»

⚠️ **`#324` (E23) è nella v0.1** — milestone `v0.1 · Mondo giocabile` — **e porta lo stesso vincolo**, che
la subordina ai gate della release di cui fa parte. È l'eredità del suo anticipo dalla v0.2 (`D-160`), ed
è un difetto di merito oltre che di conteggio.

### 6.2 🔴 Il corpo di `#14` pretende ancora quindici gate, e la nota che lo smentisce è in fondo

```
riga 229: I **15** gate di release `G1`–`G15` … verdi con evidenza allegata
riga 351: ⚠️ **`G15` non esiste più, e il criterio di chiusura qui sopra lo pretende ancora.**
```

Il difetto è **già dichiarato** dal giro del 2026-08-30, per scelta esplicita: la convenzione della casa è
la nota additiva, *«la storia di come è stato deciso vale più della sua riscrittura»*.

> **FOWLER**: «Additivo va bene per il rationale, non per il criterio di accettazione. *Criterio di chiusura*
> è la sezione che qualcuno esegue. Avere l'istruzione a riga 229 e la sua smentita a riga 351 significa che
> la issue ha **due** criteri di chiusura e nessun modo di sapere quale vince.»

È l'unico punto in cui il kit e la convenzione del repository sono in **conflitto reale**, e va deciso
dall'autore — non da questo referto. Vedi §7 A2.

### 6.3 ⚠️ Due epic con label `v0.1` non sono elencate in `#14`

```
#1881  Resolution Playback & Inspection — bot, replay e debug fino alla v1.0   lbl=v0.1,epic,P1  ms=NONE
#1937  Player Event Log & Explainability — dal TurnLog alla UI fino alla v1.0  lbl=v0.1,epic,P1  ms=NONE
```

È **la stessa classe di difetto** che il 2026-08-30 aveva chiuso per `#324`/`#934`/`#952`/`#1408`, ricomparsa
con due epic create dopo quella data. La riconciliazione precedente non è stata disfatta: è stata
**superata dai fatti**, ed è la prova che il difetto è strutturale — nessun gate confronta l'insieme delle
epic `label:v0.1` con la checklist di `#14`.

🔵 **AMBIGUOUS, e per questo non lo correggo di mia iniziativa**: entrambi i titoli dicono *«fino alla v1.0»*.
Sono owner **cross-release** con lavoro v0.1, non epic v0.1. Elencarli in `#14` senza dirlo li conta come
scope di release; ometterli li rende invisibili al parent della release che li finanzia. Il kit vieta
entrambe le mosse cieche, e ha ragione.

### 6.4 ⚠️ Rinvii che puntano a una casa che `D-259` ha svuotato

`#775` (E42), sotto `## Fuori perimetro`:

> *Matchmaking pubblico e ranked. … **il rating è E44**, dove le regole non cambiano più.*

L'**esclusione** è corretta e resta valida — va detto, perché una lettura frettolosa del termine `ranked`
in questa epic la classificherebbe come violazione di `D-259` mentre è il suo rispetto. Il **rinvio** no:
`D-259` ha tolto rating e ranked da `E44` portandoli post-v1.0, e `#777` lo dichiara col titolo barrato.
Chi segue il rinvio arriva a una sezione che dice *«non più qui»*.

🔴 **`#773` era un mio falso positivo, e va detto.** L'avevo elencata per lo stesso difetto sulla base di un
grep del termine `ranked`. Rileggendo il contesto completo — `## Fuori perimetro`: *«Matchmaking, ranked,
dedicated server e riconnessione. Il dedicated è **E42**…»* — l'epic **non rinvia da nessuna parte per
ranked**: lo esclude e basta, e l'unico rinvio che contiene (*«il dedicated è E42»*) è tuttora corretto.
`A3` è stata quindi applicata al **solo `#775`**. È la classe di errore che il §11 del kit vieta
esplicitamente — *«non fare search-and-replace ciechi»* — commessa sul kit invece che dal kit.

🔵 E un terzo caso **AMBIGUOUS** che segnalo senza toccarlo: `#1604` — `CP 44.5 · Ranked e rating` — vive in
milestone **`v0.9 · Release Candidate`**, cioè **pre-v1.0**, mentre `D-259` porta ranked post-v1.0. Può
essere corretto (è il checkpoint che *decide il perimetro*, non che implementa) o può essere un residuo.
Il kit vieta di cambiare milestone future senza owner: lo lascio all'autore.

### 6.5 ⚠️ Metadati mancanti sugli owner cross-release

| Issue | Difetto |
|---|---|
| `#1816` (E50) | ha **una sola** label: `epic`. Nessuna priorità, nessuna release. |
| `#1769` `#1861` `#1881` `#1937` `#1408` | nessuna milestone |

## 7. Azioni — quattro applicate su autorizzazione, tre lasciate all'autore

| # | Azione | Issue | Evidenza | Esito |
|---|---|---|---|---|
| **A1** | `15 gate` → `14 gate (G1–G14)`, nota additiva datata | `#322`–`#333` | DoD §3: `G15` barrato, `D-181` | ✅ **applicata ×12** |
| **A3** | Rinvio *«il rating è E44»* → *«post-v1.0, `D-259`»* | `#775` | `D-259`, `#777` barrata | ✅ **applicata ×1** |
| **A4** | Priorità mancante su E50 | `#1816` | §6.5 | ✅ **applicata** — `P1`, **senza release label** |
| **A2** | `#14` riga 229: correggere nel corpo o lasciare additiva | `#14` | §6.2 | ✅ **corretta nel corpo** — §7.2 |
| **A5** | Come `#1881`/`#1937` compaiono in `#14`, o se non devono | `#14` | §6.3 | ⛔ **AMBIGUOUS** |
| **A6** | `#324` deve davvero aspettare i gate della **propria** release? | `#324` | §6.1 | ⛔ **merito** — registrata nella nota |
| **A7** | Milestone di `#1604` | `#1604` | §6.4 | ⛔ **AMBIGUOUS** |
| ⛔ | §12 del kit (milestone `#6`) | — | §5 | **non applicato** — 🔴 distruttivo |
| ⛔ | Rename di `#333` | `#333` | §4.3 | **non applicato** — decisione bilaterale |

### 7.1 `A4` è stata applicata a metà, e la metà mancante è il punto

Avevo proposto *«`P?` **+ release label**»*. La release label **non** è stata aggiunta, e non per prudenza:
il Goal di `#1816` dichiara di rendere espliciti i confini architetturali *«senza toccare gameplay,
determinismo, replay o **scope di release**»*. Un'epic che si dichiara neutra rispetto alla release non
prende una label di release. La priorità `P1` è invece **derivata**, non inventata: le due sotto-issue che
portano il lavoro — `#1817` e `#1818` — sono entrambe `P1`. Precedenti di epic senza release label esistono
già: `#1105` e `#422`.

### 7.2 `A2` — il conflitto è stato sciolto a favore del corpo, e la storia è rimasta

Decisione d'autore del 2026-08-31: il *Criterio di chiusura* di `#14` **si corregge nel corpo**, non con
un'altra nota additiva. Il ragionamento che l'ha motivata è quello del §6.2 — è la sezione che qualcuno
**esegue**, e due criteri in contraddizione a 120 righe di distanza non sono una cronaca, sono un'ambiguità
operativa.

⚠️ **Ma «correggere nel corpo» non ha significato «riscrivere».** È stato usato il pattern che `#14` applica
già alle proprie correzioni — `~~**21 epic, 100 checkpoint**~~`, `~~**8 epic chiuse su 21**~~ → ✅ **9**` —
cioè **barrato più valore giusto**:

```
I ~~**15**~~ → **14** gate di release ~~`G1`–`G15`~~ → **`G1`–`G14`** … verdi con evidenza
allegata. ~~`G15` (tracciabilità delle feature) è stato aggiunto col Feature Registry~~ — **ritirato da
`D-181`** il 2026-08-21 …
```

✅ **E la nota del 2026-08-30 è stata portata al passato**, perché dopo la correzione avrebbe detto il falso:
diceva *«il criterio di chiusura qui sopra lo **pretende** ancora»*. Ora dice `pretendeva`, con
✅ *«Corretto nel corpo il 2026-08-31 — questa nota resta come **cronaca** del difetto e della sua durata,
non come descrizione dello stato corrente»*. È la parte che si dimentica: **correggere un difetto rende
falsa la nota che lo denunciava**, e una nota che descrive uno stato superato è lo stesso difetto in un'altra
riga.

✅ **E le due righe della stessa classe sono state chiuse subito dopo, con lo stesso criterio.** La riga
28 (*«Stato autorevole delle feature: `feature-registry.yaml`»*, nel **blocco delle fonti**) e la 255
(*«lo stato vive in `feature-registry.yaml` ed è derivato dai gate, che il validator verifica»*) erano le
**uniche due** menzioni fuori da una nota datata: stavano dove si leggono come indirizzo valido e come
regola da applicare. Ora sono barrate col loro sostituto — `roadmap-v0.1.md` + `roadmap-checkpoint.md` —
e la riga 255 dichiara in più ciò che il barrato da solo non direbbe: **nessun meccanismo automatico
deriva più lo stato**, perché registro, validator e viste generate sono usciti insieme.

⚠️ **Anche qui una nota è diventata falsa, e per la terza volta.** Quella del 2026-08-25 dichiarava
*«Il corpo non è stato riscritto»*. Non è stata riscritta né cancellata: le è stato premesso un
**addendum datato** che riduce l’ambito della frase — resta vera per tutto il resto del corpo — e dice
quali due menzioni sono uscite dal suo perimetro e perché. È lo stesso movimento fatto per la nota del
2026-08-30 al §7.2: **una correzione produce sempre una seconda correzione a monte**, e saltarla lascia
in piedi una descrizione dello stato che non è più vera.

### 7.3 🔴 Un incidente, e come è finito

Il primo tentativo di `A1` è stato eseguito con un loop in cui `gh issue view` ha restituito **corpi vuoti**
senza che nulla lo verificasse. Undici modifiche sono fallite, ma la dodicesima — rilanciata a mano per
diagnosticare l'errore — **è passata, e ha sovrascritto l'intero corpo di `#322` con la sola nota**.

✅ **Ripristinato entro il minuto** dal corpo scaricato all'inizio della sessione, e verificato con `diff`:
identico a meno di una riga vuota finale. Il secondo tentativo ha una guardia (`fetch < 500 byte → ABORT`) e
una verifica post-scrittura (`nuova dimensione ≥ attesa`), ed è quello che ha applicato tutte e dodici.

⚠️ **La lezione non è «controlla il fetch».** È che una pipeline di scrittura su GitHub senza guardia
sull'input **non fallisce in modo sicuro**: distrugge, e lo fa in silenzio. Lo stesso difetto che il §5 di
questo referto imputa al §12 del kit — *«nessuna modalità di fallimento sicura»* — l'ho commesso io per
primo, mentre lo scrivevo.

## 8. Cosa questa revisione NON fa

- **Non esegue il kit.** In questo repository *consumare* un kit significa **revisionare e archiviare**, non
  applicare. Le tre azioni eseguite (`A1`, `A3`, `A4`) sono state **autorizzate una per una** dopo che il
  referto le aveva elencate: nessuna discende dal fatto che il kit le prescrivesse — `A1` il kit non la
  chiede nemmeno, e la sua prescrizione più insistente (§12) è quella che **non** è stata applicata.
- **Non crea, chiude o riapre nulla.** Zero issue, zero epic, zero milestone, zero E-number.
- **Non committa.** `CLAUDE.md` §9: nessun commit o push senza richiesta esplicita. I tre file toccati —
  questo referto, il sorgente archiviato e `docs/archive/src/README.md` — restano `untracked`/modificati
  nell'albero di lavoro.
- **Non tocca il working tree oltre a quei tre file.** `HEAD` è **detached** e 20 commit dietro: i documenti
  citati sono stati letti da `origin/main` con `git show`, mai dal disco.

### 8.1 Gate eseguiti

| Gate | Esito |
|---|---|
| `doc-links.ts --check` | ✅ 4738 link in 326 documenti, tutti risolvono |
| `doc-tables.ts --check` | ✅ 2042 tabelle in 326 documenti |
| `doc-links.ts --check --with-archive` | ✅ **5321 link in 505 documenti**, tutti risolvono |
| `doc-tables.ts --check --with-archive` | 🟡 **10 righe rotte, tutte preesistenti e nessuna mia** |

⚠️ Le dieci segnalazioni con `--with-archive` sono **filtrate per nome file**, non ignorate in blocco: nove
sono diagrammi ASCII in `archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md`, la decima è in
`archive/src/handoff/2026-08-08-master-reaction-system.md`. Nessuno dei tre file scritti oggi compare.

## 9. `GITHUB_RECONCILIATION_REPORT`

```text
Repository:           DegrassiAaron/refactor-tactics-main
HEAD:                 ef0414bc (origin/main) · revisione condotta su 4396d2b2 (detached, -20)
Execution timestamp:  2026-08-31

Issues inspected:             63
Epics inspected:              47   (tutte le label:epic aperte, corpo integrale)
Milestones inspected:         17   (16 aperte + 1 chiusa)
Issues modified:              16   (15 corpi + 1 label)
Epics modified:               16
Issues created:                0
Issues closed:                 0
Issues reopened:               0
Milestones changed:            0
Labels changed:                1   (#1816 +P1)
Parent/child links repaired:   0   (nessuno rotto: il graph regge)

No-op verified:                5   (#14 #26 #333 #777 #1769)
Ambiguities:                   3   (#1881/#1937 in #14 · milestone #1604 · vincolo gate su #324)
Remaining blockers:            0   (A2 sciolta dall'autore: correzione nel corpo, §7.2)
```

| Issue | Before | After | Why | Evidence | Decision |
|---|---|---|---|---|---|
| [#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322) | «15 gate» | + nota: 14 gate `G1`–`G14` | conteggio falso in precondizione viva | DoD §3, `G15` barrato | `D-181` |
| [#323](https://github.com/DegrassiAaron/refactor-tactics-main/issues/323) | idem | idem | idem | idem | `D-181` |
| [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) | idem | + nota **estesa** (vincolo di merito) | è v0.1 e aspetta i gate v0.1 | milestone `v0.1 · Mondo giocabile` | `D-181` · `D-160` |
| [#325](https://github.com/DegrassiAaron/refactor-tactics-main/issues/325) | idem | idem | idem | idem | `D-181` |
| [#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326) | idem | idem | idem | idem | `D-181` |
| [#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327) | idem | idem | idem | idem | `D-181` |
| [#328](https://github.com/DegrassiAaron/refactor-tactics-main/issues/328) | idem | idem | idem | idem | `D-181` |
| [#329](https://github.com/DegrassiAaron/refactor-tactics-main/issues/329) | idem | idem | idem | idem | `D-181` |
| [#330](https://github.com/DegrassiAaron/refactor-tactics-main/issues/330) | idem | idem | idem | idem | `D-181` |
| [#331](https://github.com/DegrassiAaron/refactor-tactics-main/issues/331) | idem | idem | idem | idem | `D-181` |
| [#332](https://github.com/DegrassiAaron/refactor-tactics-main/issues/332) | idem | idem | idem | idem | `D-181` |
| [#333](https://github.com/DegrassiAaron/refactor-tactics-main/issues/333) | idem, ×3 | idem | il difetto si era riprodotto **oggi** | nota `D-256` del 06:30Z | `D-181` |
| [#775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/775) | «il rating è E44» | + nota: rating **post-v1.0**, perimetro `#1604` | rinvio senza destinazione | `#777` barrata | `D-259` |
| [#1816](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816) | label `epic` | label `epic,P1` | epic senza priorità | `#1817`/`#1818` sono `P1` | — |
| [#14](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14) | *Criterio di chiusura*: «i **15** gate `G1`–`G15`» | «~~15~~ → **14** gate ~~`G1`–`G15`~~ → **`G1`–`G14`**» + nota del 30/08 portata al passato | due criteri di chiusura in contraddizione nella stessa issue | DoD §3, `G15` barrato | `D-181` · **decisione d'autore** |
| [#14](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14) | *Fonti* r.28 e regola `DONE` r.255: `feature-registry.yaml` | barrate → `roadmap-v0.1.md` + `roadmap-checkpoint.md`, e «nessun meccanismo automatico deriva più lo stato»; addendum alla nota del 25/08 | erano le **uniche due** menzioni fuori da una nota datata | `D-181`: registro, validator e viste generate usciti insieme | `D-181` · **decisione d'autore** |

> ✅ **Nessun corpo è stato riscritto, nemmeno quello di `#14`.** Tredici modifiche sono **note additive
> datate** in coda; la quattordicesima — `#14` — è una correzione **in loco** col pattern `~~barrato~~` +
> valore giusto, che la issue applica già alle proprie correzioni. Le occorrenze storiche di «15 gate»
> sono ancora tutte lì (verificato su `#322`, `#324`, `#333`, `#14`).

## 10. `DRIVE_SYNC_PAYLOAD` — stato **dopo** le modifiche

```yaml
DRIVE_SYNC_PAYLOAD:
  verified_at: 2026-08-31
  repository: DegrassiAaron/refactor-tactics-main
  head_sha: ef0414bc2b6a8dac9e1ea31c041f6a7bfb137bec   # origin/main; revisione su 4396d2b2 (detached, -20)
  execution_status: APPLIED — 16 mutazioni (13 note additive + 2 correzioni nel corpo di #14 + 1 label).
                    Zero issue create, chiuse o riaperte.

  release:
    master_issue: 14
    state: OPEN
    milestone: "#6 v0.1 · Gate di release"
    milestones_v01: ["#1 Fondamenta (closed)", "#2 Mondo giocabile", "#3 Leggibilità",
                     "#4 Percezione e reazioni", "#5 Prova integrata", "#6 Gate di release",
                     "#7 Difetti e bilanciamento"]
    gates: "G1-G14 (14). G15 barrato e ritirato da D-181; numero non riusabile."
    gates_status: "7 verdi su 14 al 2026-08-25 (fonte: #26)"
    feature_registry_status: RETIRED — D-181, 2026-08-21

  changed_epics:
    - issue: [322, 323, 325, 326, 327, 328, 329, 330, 331, 332, 333]
      title: "Epic v0.2/v0.3/v0.4 con vincolo di apertura sui gate v0.1"
      old_state: OPEN
      new_state: OPEN            # invariato
      milestone: invariata
      labels: invariate
      parent: invariato
      change: "Nota additiva datata: il vincolo dice «15 gate», i gate sono 14 (G1-G14).
               Il vincolo NON e' revocato, cambia solo il conteggio. Occorrenze storiche preservate."
      decision: D-181
      url: https://github.com/DegrassiAaron/refactor-tactics-main/issues/322
    - issue: 324
      title: "[EPIC v0.1] E23 · Muri, porte e interaction graph"
      old_state: OPEN
      new_state: OPEN
      milestone: "v0.1 · Mondo giocabile"   # invariata
      labels: invariate
      parent: 14
      change: "Stessa nota, piu' un rilievo di merito: e' l'unica delle dodici a stare NELLA v0.1 e
               a dichiarare di non aprirsi prima dei gate della propria release. Registrato, non risolto."
      decision: D-181 · D-160
      url: https://github.com/DegrassiAaron/refactor-tactics-main/issues/324
    - issue: 775
      title: "[EPIC v0.7] E42 · Dedicated server e loop online reale"
      old_state: OPEN
      new_state: OPEN
      milestone: "v0.7 · Competitive Alpha"  # invariata
      labels: invariate
      parent: null
      change: "Nota additiva: il rinvio «il rating e' E44» non ha piu' destinazione dopo D-259.
               L'ESCLUSIONE di ranked da E42 resta valida e non cambia."
      decision: D-259
      url: https://github.com/DegrassiAaron/refactor-tactics-main/issues/775
    - issue: 1816
      title: "[EPIC] E50 · Architecture Hardening"
      old_state: OPEN
      new_state: OPEN
      milestone: NONE            # invariata, non assegnata
      labels: "epic → epic,P1"
      parent: null
      change: "Aggiunta la priorita' mancante, derivata dalle sotto-issue #1817/#1818 (entrambe P1).
               NESSUNA release label: il Goal dell'epic si dichiara fuori dallo scope di release."
      decision: null
      url: https://github.com/DegrassiAaron/refactor-tactics-main/issues/1816

  changed_issues: []         # nessuna issue non-epic toccata
  milestones_changed: []     # zero
  labels_changed:
    - issue: 1816
      change: "+P1"

  no_op_verified:
    - issue: 14
      reason: "Le 9 caselle [x] corrispondono esattamente alle 9 epic CLOSED. Riconciliata il 2026-08-30."
    - issue: 1769
      reason: "Owner camera. CAM-01..CAM-12 + CAM-11a mappate su #1770-#1781, #1809. Le tre correzioni
               CAM-WP del kit sono già nel corpo. Famiglia OVL #1941-#1944 già elencata."
    - issue: 333
      reason: "Nota D-256 già presente, aggiornata 2026-08-31T06:30:55Z. Il rename è dichiarato
               decisione bilaterale, non eseguibile da un lato."
    - issue: 777
      reason: "Riconciliata con D-259 il 2026-08-28: sezione barrata, 'Fuori dalla v1.0' completa,
               perimetro passato a #1604."
    - issue: 26
      reason: "Già su G1-G14 e dichiara esplicitamente che il perimetro 'non va corretto'."

  decisions_confirmed:
    - id: D-181
      effect: "Feature Registry e G15 ritirati. Gate vivi: G1-G14. CONFERMATA sul DoD live."
    - id: D-256
      effect: "3v3 Standard · 2v2 Skirmish/vertical slice · 4v4+ Operations. CONFERMATA (Decision Log 268)."
    - id: D-259
      effect: "Ranked/rating/MMR fuori dalla v1.0; chiude RNK-1. CONFERMATA (Decision Log 271)."
    - id: D-286
      effect: "Camera in v0.1: #1834-#1838 tutte OPEN, label v0.1, milestone v0.1. CONFERMATA."

  camera:
    owner_issue: 1769
    owner_state: "OPEN, milestone NONE, label epic/P2/post-v0.1"
    v01_children: [1834, 1835, 1836, 1837, 1838]
    v01_children_milestones: ["#3 Leggibilità x4", "#4 Percezione e reazioni x1"]
    cam_identifier_policy: "I CAM-ID autorevoli sono quelli di E49: CAM-01..CAM-12 = #1770..#1781,
                            CAM-11a = #1809, CAM-13..CAM-17 = #1834..#1838. I CAM-WP-01..08 del Drive
                            NON sono identità GitHub e le loro tre proposte di ownership sono già
                            confutate dentro il corpo di #1769."
    overlay_children: [1941, 1942, 1943, 1944]
    overlay_note: "Famiglia OVL sotto E49. Nessuna seconda epic 'Tactical Grid Overlay' esiste né va creata."

  stale_drive_claims:
    - document_or_claim: "kit §12 — 'v0.1 issue fuori milestone #6'"
      old: "la v0.1 ha una milestone, la #6"
      current: "la v0.1 ha SETTE milestone (#1 chiusa, #2-#7 aperte)"
      evidence: "gh api repos/.../milestones — ~92 issue v0.1 aperte fuori da #6"
      severity: DESTRUCTIVE — non applicare
    - document_or_claim: "kit §5 — 'la checklist del 30/08 potrebbe essere stale'"
      old: "checklist da riconciliare"
      current: "9 caselle [x] = 9 epic CLOSED, esatte"
      evidence: "gh issue list --label epic --state closed"
    - document_or_claim: "kit §7 — i tre mapping CAM-WP da correggere"
      old: "correzioni da applicare"
      current: "già scritte nel corpo di #1769 e nel referto del 2026-08-30"
      evidence: "body #1769 righe 242, 264, 269, 271"
    - document_or_claim: "kit §8 — '#333 mantiene titolo storico e riferimenti prescrittivi'"
      old: "da correggere"
      current: "nota D-256 presente dal 2026-08-31T06:30:55Z; rename dichiarato bilaterale"
      evidence: "gh issue view 333 --json updatedAt"
    - document_or_claim: "kit §9 — 'ranked da riconciliare in #777'"
      old: "da correggere"
      current: "riconciliata il 2026-08-28, sezione barrata"
      evidence: "body #777 righe 17-37"

  defects_fixed:
    - id: F-01
      what: "'15 gate' come precondizione viva in 12 epic (24 occorrenze); i gate sono 14"
      where: [322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333]
      fix: "nota additiva datata su tutte e 12; occorrenze storiche preservate"
      note: "si era riprodotto il 2026-08-31 dentro la nota D-256 di #333"
    - id: F-04
      what: "rinvio 'il rating e' E44' obsoleto dopo D-259"
      where: [775]
      fix: "nota additiva su #775"
      correction: "#773 era un falso positivo del referto: esclude ranked senza rinviare altrove"
    - id: F-05a
      what: "#1816 aveva una sola label: epic"
      where: [1816]
      fix: "+P1, derivata da #1817/#1818. Nessuna release label: l'epic si dichiara release-neutra."

    - id: F-02
      what: "#14 riga 229 pretendeva 'i 15 gate G1-G15'; la smentita era additiva a riga 351"
      where: [14]
      fix: "corretta NEL CORPO con ~~barrato~~ + G1-G14 (decisione d'autore, 2026-08-31);
            la nota del 2026-08-30 portata al passato perche' altrimenti direbbe il falso"
      residuo: "NESSUNO. Le righe 28 e 255 (feature-registry.yaml) sono state corrette con lo
                stesso criterio subito dopo. In #14 zero istruzioni correnti nominano G15 o il
                Feature Registry: ogni menzione e' barrata, in una nota datata, o al passato."

  open_defects_not_fixed:
    - id: F-03
      what: "#1881 e #1937 hanno label v0.1 e non sono elencate in #14"
      where: [14, 1881, 1937]
      classification: AMBIGUOUS — entrambe dichiarano scope 'fino alla v1.0'
      why_not_fixed: "A5 — elencarle le conta come scope di release, ometterle le rende invisibili."
    - id: F-05b
      what: "cinque owner cross-release senza milestone"
      where: [1769, 1861, 1881, 1937, 1408]
      why_not_fixed: "il kit vieta di cambiare milestone senza owner/decisione, ed e' la regola giusta."
    - id: F-06
      what: "#324 e' v0.1 e dichiara di non aprirsi prima dei gate della v0.1"
      where: [324]
      why_not_fixed: "A6 — verifica di merito, non di forma. Registrata nella nota applicata."

  unresolved_conflicts:
    - conflict: "Il kit pretende che 'nessuna istruzione corrente usi G15'. La convenzione del repository
                 è la nota additiva che preserva il corpo. Le due regole non stanno insieme su #14."
      recommended_action: "Decisione d'autore (A2). Non risolvibile da un referto."
    - conflict: "#1604 (CP 44.5 Ranked) è in milestone v0.9, pre-v1.0, mentre D-259 porta ranked post-v1.0."
      recommended_action: "Chiarire se è il checkpoint di perimetro o un residuo (A7)."
    - conflict: "#324 è v0.1 e dichiara di non aprirsi prima dei gate della v0.1."
      recommended_action: "Verifica di merito, eredità dell'anticipo D-160 (A6)."

  current_open_prs:
    - pr: 1978
      title: "docs(camera): §5.1 prescriveva una sorgente che esiste da ieri"
      url: https://github.com/DegrassiAaron/refactor-tactics-main/pull/1978
    - pr: 1977
      title: "fix(1970): la sovrapposizione entra nel dato e il resolver la dice"
      url: https://github.com/DegrassiAaron/refactor-tactics-main/pull/1977
    - pr: 1974
      title: "feat(fixture): BlockYard — i tre casi di blocco affiancati"
      url: https://github.com/DegrassiAaron/refactor-tactics-main/pull/1974
    - pr: 1969
      title: "feat(1953): i parametri di un'azione dicono anche da quale casa vengono"
      url: https://github.com/DegrassiAaron/refactor-tactics-main/pull/1969
    - pr: 1928
      title: "feat(1921): un esito solo copriva due difetti"
      url: https://github.com/DegrassiAaron/refactor-tactics-main/pull/1928

  drive_documents_to_update:
    - title: "Handoff GitHub Epic/Issue reconciliation (2026-08-31)"
      requested_change: "Rimuovere il §12 'v0.1 fuori milestone #6': la v0.1 ha sette milestone."
    - title: "Release ladder (00E)"
      requested_change: "Il rename di E32 è bilaterale: decidere Drive+GitHub insieme o lasciare entrambi."
    - title: "Piano camera CAM-WP-01..08"
      requested_change: "Marcare superato: l'owner è E49/#1769 e i CAM-ID autorevoli sono i suoi."
```

## 11. Prossimo passo

✅ **La DoD del kit su `#14` è soddisfatta**: *«nessuna istruzione corrente usa Feature Registry/G15 come
tracking corrente»*. Verificato una riga per volta con
`grep -niE "feature.registry|G15"` — **10 righe**, non una in meno:

| Quante | Come si presentano | Righe |
|--:|---|---|
| **4** | **barrate**, col sostituto accanto | 28 · 229 · 232 · 255 |
| **1** | al **passato**, come spiegazione del ritiro | 235 |
| **5** | dentro una **nota datata** | 303 · 315 · 379 · 382 · 383 |
| **0** | istruzioni correnti | — |

Restano tre voci, tutte da decidere e nessuna urgente:

| # | Cosa | Perché non l'ho fatta |
|---|---|---|
| `A5` | come `#1881`/`#1937` compaiono in `#14`, o se non devono | `AMBIGUOUS` — dichiarano scope *«fino alla v1.0»* |
| `A6` | `#324` deve aspettare i gate della **propria** release? | merito, non forma — registrata nella nota applicata |
| `A7` | milestone di `#1604` (`CP 44.5 Ranked`) in `v0.9`, pre-v1.0 | `AMBIGUOUS` — perimetro o residuo |

**Ma la cosa che vale davvero è un'altra, e nessuna delle sedici mutazioni la tocca.**

🔴 **Nessun gate confronta un numero o un percorso scritto in prosa dentro una issue con il documento che lo
possiede.** È il meccanismo, e la sua impronta è misurabile: «15 gate» è sopravvissuto a `D-181`, alle due
sweep del 2026-08-28 e a quella del 2026-08-30 — e si è **riprodotto** il 2026-08-31 dentro una nota scritta
lo stesso giorno; `feature-registry.yaml` era citato come **fonte autorevole** dieci giorni dopo la propria
rimozione. `doc-links.ts` non guarda i corpi delle issue, e `#962` lo aveva già scoperto sul totale delle
epic: *«il corpo di una issue GitHub non è in `docs/`, questa copia è sopravvissuta allo sweep che la
cercava»*.

Le correzioni di oggi tolgono **sedici istanze**. La diciassettesima la scriverà qualcuno la settimana
prossima, e nessuno se ne accorgerà — esattamente come le sedici precedenti.

> **WIEGERS**: «Un requisito che nessun controllo può falsificare non è un requisito: è una convenzione
> sperata. Finché il conteggio dei gate vive in prosa in dodici posti e in tabella in uno solo, la domanda
> non è *se* divergerà.»
