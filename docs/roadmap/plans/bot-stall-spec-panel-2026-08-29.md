# Catena BOT-STALL — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma le tre issue #1655 · #1551 · #1550.
>
> **Data**: 2026-08-29 · **Base**: `origin/main` @ `bc850e0b` · **Modo**: critique · **Focus**: requirements + testing
>
> `/sc:spec-panel` è task documentale ([`CLAUDE.md`](../../../CLAUDE.md) §6). Se una riga qui diverge da
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) o dal [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md),
> **ha ragione l'owner**.
>
> ⏱️ **Il referto è stato scritto come dry-run — nessuna issue toccata — e gli atti sono venuti dopo, su
> richiesta esplicita**: il §6 elenca cosa ne è nato. La sequenza conta, ed è quella che il §6.1-5 del
> [referto gemello](roadmap-issues-v01-v10-spec-panel-2026-08-29.md) prescrive: il dry-run precede la
> scrittura.
>
> ⛔ **Nessun oracolo è stato toccato**, e resta il punto: allineare i due oracoli divergenti distruggerebbe
> la prova che uno dei due porta.

---

## 1. Il verdetto in una riga

> **Le tre issue sono scritte meglio della media del repository e non sono bloccate da lavoro: sono bloccate
> da una decisione che nessuna delle tre può prendere. Il rischio non è che restino aperte — è che nessuno
> legga la riga che le chiuderebbe, perché è lunga 723 parole dentro una cella di tabella.**

E non è una catena. `#1655 → #1551 → #1550` suggerisce una sequenza; misurando, sono **due** cose: una
decisione non presa in tre facce (#1655, #1551) e una **domanda geometrica indipendente** (#1550), che
potrebbe essere lavorata oggi da chiunque senza aspettare nessuno.

---

## 2. Base di misura

| Affermazione | Misura | Esito |
|---|---|---|
| I due oracoli divergenti esistono coi nomi citati | `git grep` su `origin/main` | ✅ `RTMatchAutobattleTests.cpp` · `RTAuthoredMapEngagementTests.cpp` |
| `RTStallDefinitionMeasureTests.cpp` esiste | idem | ✅ **4** test |
| `FRTOrbitProbe` esiste | idem | ✅ in **6** file |
| La riga della tabella di mutazione è ancora aperta | `RTMatchAutobattleTests.cpp:2262` | 🔴 *«chi trovera' una mutazione del BOT che fa cadere…»* — **aperta** |
| `BOT-STALL-1` esiste in `OPEN_DECISIONS.md` | riga 78 | ✅ |
| Lunghezza della cella `BOT-STALL-1` | `wc` | 🔴 **4630 caratteri · 723 parole** |
| Mediana delle 85 voci del documento | `awk` su tutte | ~**600** caratteri — `BOT-STALL-1` è **7,7×** la mediana, seconda solo a `BAL-1` (4726) |
| Le tre issue hanno una milestone | `gh issue view` | 🔴 **nessuna delle tre** |
| La catena presidia `G10` | DoD v0.1, voce `G10` | ✅ *«un `RoundLimit` a zero eventi è lo stallo del bot che `RTBotStalemateProbeTests` misura, e resta 🔴»* |

⛔ **Non misurato**: nessuna suite eseguita per questo referto. Gli esiti citati sono quelli che le issue e il
DoD dichiarano.

---

## 3. Punteggi

| Dimensione | Voto | Ragione in una riga |
|---|---|---|
| **Chiarezza** delle issue | **8.5** / 10 | domanda, evidenza dei due lati, uscite con costo: è il formato che una decisione richiede |
| **Chiarezza** della riga che le chiude | **3.0** / 10 | 723 parole in una cella di tabella, senza sommario, senza spezzature |
| **Testabilità** (Wiegers · Adzic) | **7.5** / 10 | quasi ogni claim porta la propria misura; ma due AC su tre di #1655 e #1551 non sono chiudibili da chi li ha scritti |
| **Completezza** | **8.0** / 10 | il *non-lavoro* è dichiarato — raro e prezioso |
| **Consistenza** | **4.0** / 10 | la raccomandazione di `BOT-STALL-1` contraddice, alla lettera, la misura che sta nella stessa cella |
| **Complessivo** | **6.5** / 10 | istruttoria eccellente, veicolo sbagliato, proprietà mal collocata |

---

## 4. Findings

### 🔴 F-01 · La raccomandazione istruisce ad adottare ciò che la sua stessa cella sconsiglia — WIEGERS

`BOT-STALL-1` raccomanda: *«(c) se qualcuno la misura, altrimenti (d)»*. La misura **è stata fatta**
(#1645/#1551), e la stessa cella conclude che la (c) *«su questi dati non compra nessun verdetto diverso da
quello che la (a) già dà»*.

✅ **Il documento lo sa e lo dichiara** — *«L'ANTECEDENTE DI QUESTA RACCOMANDAZIONE È SCATTATO … Il conseguente
non è stato riscritto: riscriverlo sarebbe prendere la decisione»*. È la scelta giusta sulla proprietà.

🔴 **Ma il risultato resta una riga che, letta alla lettera, istruisce male.** È l'unico AC ancora aperto di
#1655, ed è aperto da quando la misura esiste.

**WIEGERS**: «Un requisito condizionale il cui antecedente è scattato non è più condizionale: è un'istruzione.
Se l'istruzione è sbagliata, marcarla “da rileggere” non la disinnesca — la lascia in vigore per chi non legge
la nota. La forma corretta di una raccomandazione la cui condizione si è consumata non è un avvertimento
accanto: è la **sospensione esplicita** del conseguente.»

📝 **Riscrittura**, che non prende la decisione e toglie l'istruzione sbagliata:
`Raccomandazione SOSPESA il 2026-08-29: la condizione «se qualcuno la misura» si è consumata, e la misura ha`
`mostrato che (c) non separa da (a) su queste board. Nessuna uscita è raccomandata finché PDR-00 non rilegge.`
🎯 **Priorità**: alta — è l'unico difetto della catena che può produrre lavoro sbagliato **oggi**.

### 🔴 F-02 · La decisione è illeggibile da chi deve prenderla — DOUMONT

**4630 caratteri, 723 parole, una cella di tabella Markdown.** Il documento ha 85 voci con mediana ~600
caratteri: questa è **7,7×** la mediana.

**DOUMONT**: «Il contenuto è eccellente e per questo il problema è serio: un'analisi che nessuno finisce di
leggere ha lo stesso effetto pratico di un'analisi che non esiste. Una cella di tabella non ha titoli, non ha
elenchi che respirano, non ha un punto in cui riprendere dopo un'interruzione. E il lettore designato — l'owner
di PDR-00 — è precisamente quello che aprirà il file una volta sola, in mezzo ad altro.»

**COCKBURN**: «Il *primary actor* di questo documento è chi decide, e il suo goal è **scegliere fra quattro
uscite**. Tutto ciò che non serve a quella scelta è materiale di supporto. Le quattro uscite col loro costo
stanno in venti righe; le derivazioni algebriche, i numeri di bilanciamento e la storia dei rilievi sono
**appendice**.»

📝 **Riscrittura**: la riga resta un puntatore di due frasi — domanda, uscita raccomandata, stato — e il corpo
si sposta in `docs/decisions/` come voce propria, dove i titoli sono ammessi. È già il pattern che il
repository usa per gli ADR.
🎯 **Priorità**: alta — è la ragione per cui la decisione non viene presa.

### 🟠 F-03 · Tre issue senza milestone presidiano un gate di release — NYGARD

Nessuna delle tre ha milestone. Ma `G10` del DoD v0.1 dichiara: *«un `RoundLimit` a zero eventi non è una
partita: è lo stallo del bot che `RTBotStalemateProbeTests` misura, e resta 🔴»*.

**NYGARD**: «La clausola di non-degenerazione di `G10` dipende da oracoli che nessuna milestone rivendica. In
un piano di release questo è il difetto peggiore, perché non produce un rosso: produce un **silenzio**. Chi
guarda la milestone `Gate di release` non vede queste tre, e chi guarda queste tre non vede che sopra c'è un
gate.»

⚠️ E il collegamento è già stato quasi perso una volta: il commento del 2026-08-29 su #1655 esiste
**apposta** per registrarlo, perché *«non è ovvio dal titolo di nessuna delle tre»*.

📝 **Riscrittura**: milestone `v0.1 · Gate di release` su tutte e tre, oppure una riga in `G10` che le nomina.
Una delle due, non zero.

### 🟠 F-04 · «Catena» è il nome sbagliato, e costa lavoro — FOWLER

`#1655 → #1551 → #1550` legge come una pipeline. Misurando:

| Issue | Da cosa dipende | Chi può chiuderla |
|---|---|---|
| **#1655** | 3 AC su 4 già chiusi; resta F-01 | chiunque, per la parte di riscrittura — **PDR-00** per la sostanza |
| **#1551** | istruttoria consegnata, 4 AC su 5 chiusi | **solo PDR-00** |
| **#1550** | 🟢 **niente** — è una domanda geometrica su due board | **chiunque, oggi** |

**FOWLER**: «Due di queste sono in attesa di una firma, la terza è lavoro disponibile. Chiamarle catena
suggerisce che la terza aspetti le prime due, e il risultato è che nessuno la prende. È la stessa classe di
errore di una dipendenza dichiarata e mai verificata.»

📝 **Riscrittura**: `#1655 + #1551` sono **BOT-STALL-1**, in attesa di PDR-00. `#1550` è indipendente e
lavorabile subito.

### 🟡 F-05 · Due issue con AC che il loro esecutore non può soddisfare — CRISPIN

#1551 ha `[ ] Chiude qu…` — l'AC finale è la chiusura da parte di PDR-00. #1655 ha F-01, che riscrivere per
intero significherebbe decidere.

**CRISPIN**: «Un'issue i cui criteri residui appartengono a un altro attore non è *aperta*: è **in attesa**, e
le due cose si gestiscono diversamente. Un backlog che non le distingue accumula issue che nessuno può
chiudere e che tutti scorrono.»

📝 Un'etichetta `blocked-on-decision` — o lo stato che il repository preferisce — separa «da fare» da «da
firmare».

---

## 5. Quello che va conservato, e vale più dei findings

**#1550 contiene il miglior rilievo di specifica che abbia letto in questo repository.** L'AC originale
chiedeva *«il test che dimostra **perché non esiste**»*: un'**esistenziale negativa su uno spazio non
limitato** — tutte le mutazioni possibili del bot. Nessun test può stabilirla.

E il commento non si ferma a dirlo: misura il danno. *«L'AC non ha soltanto mancato di impedire l'errore: **lo
ha richiesto**»* — la prima stesura di #1555 ha risposto letteralmente, con una tesi poi ritirata in review.

**ADZIC**: «Il rimedio proposto è quello giusto e ha un nome: **specificare per esempio**. Il consuntivo di
#1287 nominava già `(1,-1,L0) ↔ (3,-3,L1)`, un ciclo concreto. Un AC ancorato a quell'esempio avrebbe ucciso
al primo colpo entrambi i predicati sbagliati scritti prima di quello buono. È la dimostrazione, sul campo, che
un criterio per esempio batte un criterio per quantificatore.»

📌 **Va promosso a regola**: un AC che chiede di dimostrare la **non esistenza** di qualcosa in uno spazio non
enumerabile è malformato, e va riscritto come condizione **necessaria** verificabile su casi nominati. Questa
è materia da `AGENTS.md`, non da una issue.

---

## 6. La specifica riscritta — ciò che sopravvive

1. `BOT-STALL-1` **sospende** il conseguente della raccomandazione invece di annotarlo. *(F-01)*
2. Il corpo dell'istruttoria si sposta in `docs/decisions/`; la riga in `OPEN_DECISIONS.md` resta un
   **puntatore di due frasi**. *(F-02)*
3. Le tre issue ricevono la milestone del gate che presidiano, **oppure** `G10` le nomina. *(F-03)*
4. `#1550` viene scorporata dalla «catena» e dichiarata **lavorabile oggi**. *(F-04)*
5. Un AC che chiede una non-esistenza su spazio non enumerabile è malformato: si riscrive per **esempio
   nominato**. Regola generale, da `AGENTS.md`. *(§5)*

⛔ **Nessuna di queste prende la decisione BOT-STALL-1**, che resta di PDR-00 — ed è il punto: cinque
interventi utili, zero dei quali richiede la firma che manca.

### 6.1 Cosa ne è nato, il 2026-08-29

Due punti richiedevano una issue; tre erano azioni dirette. Aprirne cinque sarebbe stata burocrazia, non
tracciamento.

| § | Atto | Dove |
|---|---|---|
| 1 | sospensione della raccomandazione, col testo pronto | commento su [#1655](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1655) |
| 2 | corpo in `docs/decisions/`, puntatore in tabella | **issue [#1696](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1696)** |
| 3 | milestone `v0.1 · Gate di release` | applicata a #1655 · #1551 · #1550 |
| 4 | #1550 scorporata dalla «catena» | commento su [#1550](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1550) |
| 5 | regola sull'AC malformato in `AGENTS.md` | **issue [#1697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1697)** |

⚠️ **Sul §3 la scelta è stata di chi ha eseguito, non del referto**, che offriva due strade — milestone
*oppure* una riga in `G10`. È stata presa la milestone perché si disfa in un comando, mentre toccare il DoD è
un atto sul documento di gate.

📌 **Il contributo che il referto non aveva previsto** è emerso scrivendo la riscrittura del §1: esiste un
**terzo stato** fra «raccomandata (c)» e «raccomandata (d)» — *nessuna raccomandazione attiva*. Sospendere non
è scegliere: dichiara che il meccanismo di raccomandazione ha esaurito la propria validità. È l'unica forma
che toglie l'istruzione sbagliata **e** lascia intatta la scelta di `PDR-00`.

---

## 7. Limiti dichiarati

- ⛔ Nessuna suite eseguita, nessun build. Gli esiti sono quelli che issue e DoD dichiarano.
- ✅ **Verificato che i due oracoli divergano ancora**, ed era il limite che questo referto rischiava di
  lasciare aperto: `RTAuthoredMapEngagementTests.cpp:162` chiama `IsDamageInflictedByActor` per esentare chi
  ha colpito; `RTMatchAutobattleTests.cpp:1999-2004` porta la tabella che rifiuta l'esenzione
  (*«senza esenzione: sequenza 9 turni → Fail (falsifica) · esenzione globale: 2 → verde (cieco)»*).
  E il presidio di #1551 **regge**: i due file si nominano a vicenda (4 e 1 occorrenze) e **4** file di test
  nominano `BOT-STALL-1`, quindi da nessuno dei due si può allineare l'altro senza incontrare l'istruttoria.
- 🟡 La mediana di 600 caratteri è calcolata sulle 85 righe che iniziano con `` | ` `` : voci scritte in altra
  forma non entrano nel conto.
