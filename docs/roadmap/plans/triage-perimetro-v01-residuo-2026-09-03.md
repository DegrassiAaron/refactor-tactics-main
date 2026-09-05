# Il perimetro della v0.1 è più grande di quello dichiarato — triage misurato

> **Referto di misura**, non owner. Nessuna regola citata qui appartiene a questo documento.
>
> **Data**: 2026-09-03 · **Base della misura**: `origin/main` `e90edfa8` · **Metodo**: misura per issue,
> non deduzione
>
> **Gemello**: [`triage-issue-aperte-con-commit-2026-09-02.md`](triage-issue-aperte-con-commit-2026-09-02.md)
> misura *le aperte che hanno già un commit* (56 su 335). Questo misura *la v0.1*, incluse le issue su cui
> nessuno ha ancora scritto una riga. Perimetri diversi, metodo e trappole in comune.
>
> ⛔ **Nessuna riga di `Source/` toccata.** L'unica scrittura è l'assegnazione di sei milestone, §6, ognuna
> con l'evidenza citata dal corpo della issue che la riceve.
>
> 🔁 **Le due decisioni di §9 sono state prese lo stesso giorno: l'esito è in §10**, e corregge §7.2.

---

## 1. Il verdetto in una riga

> **Il perimetro v0.1 aperto non è 63 ma 83, il residuo di lavoro atomico è 67, e per 60 di esse — il 90 % —
> ciò che manca non è codice: è qualcuno che guardi lo schermo.**

E «senza milestone» **non significa dimenticata**: delle sedici misurate, sei lo erano, e le altre dieci
sono scelte dichiarate, padri orfani o casi che chiedono una decisione.

---

## 2. Perché esiste

Il gemello del 2026-09-02 ha misurato le aperte *che hanno un commit*. Restava senza risposta la domanda
opposta, che è quella della release: **quante ne restano in v0.1, e di che tipo è il lavoro che chiedono.**

La misura ovvia — `gh issue list --milestone` — dà **63**. È il numero sbagliato, e lo è del 24 %.

---

## 3. Le trappole, e quali sono nuove

Le tre del gemello valgono qui identiche: un `docs(N)` è una nota e non un'implementazione; un limite di
paginazione produce una risposta plausibile e sbagliata; un contatore di caselle sottostima il gate visivo.
Se ne aggiungono tre.

### 3.1 🔴 Il perimetro non si legge dalla milestone

| Misura | Valore |
|---|---|
| Issue aperte con **milestone** `v0.1 · *` | 63 |
| Issue aperte con **label** `v0.1` | 82 |
| **Unione — il perimetro reale** | **83** |
| — con label ma **senza** milestone | **20** |
| — con milestone ma senza label | 1 |

∴ Chi pianifica dalla milestone **non vede un quarto del proprio residuo**, e la vista di release è
esattamente lo strumento con cui si decide se spedire.

### 3.2 ⚠️ La settima milestone della v0.1 è chiusa, e le query non la vedono

`gh api …/milestones` restituisce per default **solo le aperte**. Con `state=all` compare
**`v0.1 · Fondamenta`** (milestone `1`): **chiusa, 52 issue chiuse, zero aperte**.

Non è un dettaglio d'archivio. La descrizione di `v0.1 · Gate di release` dice *«si chiude quando **le altre
sei** sono chiuse»*, e contate fra le sole aperte le altre sono **cinque**: il testo sembra stantio ed è
invece esatto — è la query a essere parziale. 🔑 **Un conteggio che contraddice una descrizione scritta va
sospettato prima di correggere la descrizione.**

### 3.3 ⚠️ Un padre fuori release rende la figlia indecidibile

Una issue con label `v0.1` la cui epic dichiarata sta in `v0.2` non ha una milestone mancante: ha una
**contraddizione**, e per [`AGENTS.md`](../../../AGENTS.md) §2 non si risolve per plausibilità. Misurate
quattro (§7.3).

---

## 4. La misura

**323** issue aperte · **83** nel perimetro v0.1 · **4507** commit su `main`.

| | # |
|---|---|
| Perimetro v0.1 aperto | 83 |
| — **epic** (contenitori: si chiudono coi figli) | 16 |
| **= lavoro atomico residuo** | **67** |

### 4.1 Di che tipo è il residuo

| Cosa serve per chiuderla | # su 67 | |
|---|---|---|
| 👁️ **una seduta editor / occhio umano** | **60** | **90 %** |
| — di cui nominano il **pacchetto** | 15 | seduta diversa, e più costosa |
| 🔧 misurabile a macchina | 7 | |

| Stato del codice | # su 67 |
|---|---|
| ha già un commit **produttivo** (`feat`/`fix`/`refactor`/`test`) su `main` | 23 |
| solo `docs` o un riferimento — nota, non implementazione | 15 |
| **nessun commit la cita** — lavoro non avviato | **29** |

➕ **L'incrocio è il residuo vero: 26 issue non hanno codice *e* chiedono di guardare.** Concentrate in
`Leggibilità` (8) e nelle sette che al momento della misura non avevano milestone.

Altri due numeri, misurati e non commentati altrove: **52 su 67 senza assegnatario**, **16 su 67 senza
milestone**.

### 4.2 ⚠️ Il 90 % è una stima per difetto

Il filtro per parole chiave — `PIE`, `packaged`, «a schermo», «si vede», «solo l'editor», `viewport`,
`playtest` — dichiarava pulite otto issue. Leggendole a mano, la verifica che il gemello impone sul
**complemento**, almeno due portano il gate senza usare nessuna di quelle parole:

- [`#1317`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1317) — *«chiudere anche l'editor richiede…»*
- [`#2149`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2149) — battute della coreografia dello showcase che *«non sono prodotte»*

∴ **Un elenco di parole chiave non può dimostrare la propria completezza**, e la prova sta nelle righe che
scarta.

---

## 5. Avanzamento, e perché non se ne ricava una data

| Misura | Valore |
|---|---|
| Issue v0.1 **chiuse** | 289 |
| Issue v0.1 **aperte** | 83 |
| Avanzamento | **289 / 372 = 78 %** |
| Ritmo ultimi 7 giorni | 64 chiusure v0.1 (~9/giorno) |
| Ritmo ultimi 14 giorni | 102 chiusure v0.1 (~7/giorno) |

🔴 **Da questi numeri non si estrapola una data, e la ragione è nella misura stessa.** Le 289 chiusure sono
state prodotte da lavoro di **codice**; il residuo è per il 90 % lavoro di **verifica**. Sono due risorse
diverse, e la seconda non scala col numero di sessioni: le sedute editor e packaged sono seriali, dipendono
da una persona davanti a un viewport, e il mutex globale del motore ne consente una per volta.

**Un ritmo misurato su un tipo di lavoro non predice il completamento di un altro.**

---

## 6. Cosa questo referto ha cambiato — sei milestone, con l'evidenza

Assegnate perché la issue **dichiara nel proprio corpo** l'epic di appartenenza, e quell'epic ha già una
milestone. Nessuna inferenza.

| Issue | Evidenza citata dal corpo | Epic → milestone |
|---|---|---|
| [`#1859`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1859) | *«**Epic**: #25 (E11 — HUD, log e debug)»* | `#25` → **Leggibilità** |
| [`#637`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/637) | *«**Epic**: #217 (E20 · HUD Icon Language)»* | `#217` → **Leggibilità** |
| [`#1392`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1392) | tabella di ripartizione: parti 1-2 → **E11** (`#25`), parte 3 → **E21** (`#286`) | `#25`/`#286` → **Leggibilità** |
| [`#1665`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1665) | *«**Epic**: #26 (E12 — Determinismo, QA e release) · **Gate**: `G13`»* | `#26` → **Gate di release** |
| [`#1663`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) | *«**Epic**: #26 (E12 …) · **Gate**: `G13`»* | `#26` → **Gate di release** |
| [`#1805`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1805) | *«`RT-FEAT-REPLAY-ARCHIVE` è **scope v0.1** sotto l'epic **`E12`**»* (`D-277`) | `#26` → **Gate di release** |

🔴 **`#1665` e `#1663` sono il motivo per cui questa passata vale più delle altre quattro righe.** Sono due
difetti **del pacchetto** — *«la board è nera»*, *«gli eroi non hanno animazioni»* — entrambi dichiarano
`Gate G13`, ed erano **senza milestone e senza priorità** mentre `v0.1 · Gate di release` contiene le issue
che chiedono proprio la verifica packaged. Due bloccanti noti della release, invisibili alla vista di
release.

⚠️ **Restano senza label di priorità**: `P0`–`P3` non sono assegnate, e questo referto non le assegna —
la priorità è un giudizio, non una misura.

---

## 7. Le dieci che NON si assegnano, e perché

### 7.1 La issue dichiara di non volerla — due

| Issue | Dichiarazione letterale |
|---|---|
| [`#2167`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2167) | *«⚠️ Milestone non assegnata: l'area è quella di #1793, ma **lo scope è da confermare**»* |
| [`#921`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/921) | *«Milestone — N/A: difetto di tooling editor, **fuori dal percorso v0.1**»* |

🔑 **Per `#921` il difetto non è la milestone assente: è la label `v0.1` presente.** Il corpo dice che sta
fuori dalla release, la label dice che sta dentro. Una delle due va tolta, e non da qui.

### 7.2 Il padre è orfano — due

[`#1880`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1880) e
[`#1879`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1879) dichiarano entrambe
*«**Epic**: #1881»*, e [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881)
*(`[EPIC] Resolution Playback & Inspection`)* è **senza milestone**. Dare alle figlie una collocazione che
il padre non ha significa deciderla per loro.

⚠️ **Le epic orfane con label `v0.1` sono tre**, e nessuna è coperta da questo referto:
`#1881` · [`#1937`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937) *(Player Event Log
& Explainability)* · [`#1408`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408)
*(E48 · Il giocatore raggiunge il modello)*. **Decidere queste tre colloca da sola almeno due delle figlie.**

### 7.3 Il padre sta fuori dalla release — due

| Issue | Padre dichiarato | Dove sta il padre |
|---|---|---|
| [`#1410`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410) | `E38` = [`#609`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/609) | `[EPIC v0.2]`, milestone **`v0.2 · Struttura e finestre`**, label `post-v0.1` |
| [`#695`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/695) | *«**Milestone/epic**: #1105 · checkpoint M9.4»* | [`#1105`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) *(Tactical Designer)*: **senza milestone**, e **senza label `v0.1`** |

🔴 **`#1410` porta `v0.1` e `P1` mentre la sua epic porta `post-v0.1`.** È una contraddizione fra due fonti,
non una lacuna: si registra e si escala, non si chiude scegliendo la più comoda —
[`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) §«Chi prevale».

### 7.4 Nessuna evidenza dichiarata — quattro

| Issue | Cosa si sa | Cosa manca |
|---|---|---|
| [`#1896`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) | bug: `WBP_RT_UnitCard` divide per zero nel `TacticalHUD` | nessuna epic citata; `E11` sarebbe un'inferenza, non una citazione |
| [`#995`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/995) | `[DOCS/DESIGN]` Elemental Proficiency; owner da creare in `docs/characters/` | è debito documentale o contenuto di release? |
| [`#931`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/931) | *«seduta U1 / #451, durante `PIE-HEX-MODE-H`»*; `RTHexArchTool` | stessa famiglia di `#921`, che si dichiara fuori v0.1 |
| [`#871`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/871) | *«seduta U1 / #451, durante `PIE-HEX-MODE-N`»*; `RTHexFillTool` | idem |

➕ **`#931`, `#921` e `#871` sono un unico gruppo**: tre difetti dei tool di `RefactorTacticsEditor` trovati
nella stessa seduta [`#451`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451) *(chiusa,
essa stessa senza milestone)*. Uno dei tre dichiara di stare fuori dalla v0.1. **O ci stanno tutti e tre, o
nessuno** — ed è una decisione sola, non tre.

---

## 8. Cosa questo referto NON copre

- **Le 67 DoD non sono lette voce per voce.** Il gemello lo ha fatto per le 56 con commit; le **29 non
  avviate** non hanno una misura equivalente, e qui sono contate, non istruite.
- **Non è verificato che le 23 con commit produttivo siano complete.** Un `feat(N)` mergiato non chiude `N`:
  è la trappola n. 2 del gemello, e vale identica qui.
- **Build, Editor, suite, PIE e packaged: `NOT RUN`.** Nessuna misura di questo referto tocca il motore.
- **Le priorità non sono assegnate**, nemmeno alle due bloccanti del pacchetto: `P0`–`P3` sono un giudizio.
- **Le 289 chiuse sono contate, non riviste.** Un DoD non spuntato non è lavoro mancante: in questo
  repository la motivazione di chiusura vive nel commento, non nella casella.
- **I conteggi valgono al fetch.** Durante la sessione `origin/main` è avanzato da `e90edfa8` a `6aa51ec7`
  e le aperte sono passate da 320 a 323: il repository è lavorato in parallelo, e nessun numero qui è
  stabile per definizione.

---

## 9. Le due decisioni che sbloccano di più

Non sono attività: sono decisioni, e nessuna delle due si prende misurando.

1. **Collocare le tre epic orfane** (`#1881`, `#1937`, `#1408`). Da sola colloca almeno `#1880` e `#1879`,
   e toglie tre contenitori dalla zona invisibile della pianificazione.
2. **Decidere se i tre difetti dei tool editor** (`#931`, `#921`, `#871`) sono v0.1. `#921` dice di no; le
   label dicono di sì. Una risposta sola chiude tre righe e una contraddizione.

E una che invece è un'attività: **raggruppare le 60 che chiedono l'occhio in due sedute** — una **editor**
(45) e una **packaged** (15) — invece che in sessanta. È lo stesso lavoro che il gemello ha raggruppato
guardandolo dal lato dei commit; qui è visto dal lato della release.

---

## 10. Esito — le due decisioni, prese il 2026-09-03

Entrambe le domande di §9 hanno avuto risposta dall'autore lo stesso giorno. Quattro collocazioni in più, e
una correzione a questo referto.

### 10.1 I tre difetti dei tool editor **sono v0.1**

Decisione d'autore: `#931` · `#921` · `#871` → **`v0.1 · Difetti e bilanciamento`**.

È la milestone dei *«difetti trasversali, bilanciamento e debito dei test **che non appartengono a una
singola epic**»*, e il criterio combacia: `#921` dichiara nella propria §Tracking `| Epic | N/A |`.

La riga contraddittoria di `#921` — *«Milestone N/A — fuori dal percorso v0.1»* — è stata **barrata nel
corpo** con la collocazione che la sostituisce, secondo la forma che quella stessa issue documenta:
*«si barra nel corpo invece di aggiungere una nota in fondo: chi legge la riga deve vedere subito che è
morta»*. `#931` e `#871` portano la decisione in un commento.

⚠️ **Tensione dichiarata e non risolta**: [`#1861`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861)
*(Map Editor 0.1)* porta `**Release**: out_of_release_scope`, e il tool a cui questi difetti appartengono è
quello. La decisione colloca i **difetti** in v0.1 senza pronunciarsi sull'**epic** che ospita il tool.

### 10.2 🔴 §7.2 chiamava «orfane» tre epic, e due non lo sono

Questa è una **correzione a questo referto**, emersa misurando prima di applicare.

`#1881` e `#1937` **dichiarano nel proprio corpo di essere trasversali**, e la milestone assente è coerente
con una convenzione che il repository applica:

> *«Questa capability attraversa più release e più consumer, quindi segue la convenzione delle epic
> trasversali già in uso — `[EPIC]` senza numero, come #1861, #1105, #422.»* — `#1881`

La misura conferma la convenzione: **delle 9 epic aperte senza milestone, 8 sono trasversali** — `#422`
(Wiki), `#1105` (Tactical Designer), `#1769` (E49), `#1816` (E50), `#1861` (Map Editor), `#1881`, `#1937`,
`#1990` (Gray Kit Playground). Dare loro una milestone di release contraddirebbe il testo che le governa.

∴ **L'unica vera orfana era `#1408`** *(`[EPIC v0.1] E48`)*, che dichiara la release nel titolo e non
dichiara trasversalità. Collocata in **`v0.1 · Gate di release`**, con l'evidenza:
[`#14`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14) — l'epic master della release, che
sta in quella milestone — **la elenca fra le proprie figlie**, annotando che è *«fuori dalla tabella §3 di
`roadmap-v0.1.md` e senza serie `CP 48.x`: riusa owner esistenti»*.

🔑 **La lezione vale oltre il caso**: «senza milestone» ha almeno tre cause diverse — dimenticanza,
dichiarazione esplicita (§7.1), e **convenzione di categoria**. Solo la prima è un difetto, e §7.2 le aveva
fuse.

### 10.3 Il conteggio aggiornato

| | al 2026-09-03, dopo le decisioni |
|---|---|
| delle 16 senza milestone: assegnate da §6 | 6 |
| assegnate da §10.1 | 3 |
| **restano** | **7** — `#2167` `#1896` `#1880` `#1879` `#1410` `#995` `#695` |
| epic senza milestone: assegnate | 1 (`#1408`) |
| confermate trasversali per convenzione | 2 (`#1881`, `#1937`) |

Delle sette che restano, **quattro non sono decidibili qui e il referto lo ha già detto**: `#2167` chiede
conferma di scope, `#1880` e `#1879` hanno per padre `#1881` — ora accertata trasversale, quindi il padre
non fornirà mai una milestone — e `#695` ha per padre `#1105`, trasversale per la stessa convenzione.
Restano tre casi senza evidenza: `#1896`, `#995`, `#1410`.
